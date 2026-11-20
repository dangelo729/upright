// upright example 02: channels
// one voice set by its channels from the serial monitor (115200, newline),
// starting as the grand preset:
//
//   preset NAME     a preset, written whole: grand concertGrand upright
//                   spinet honkyTonk fortepiano rhodes rhodesBark wurlitzer
//   set NAME VALUE  one instrument channel by name (its range is printed)
//   tine T60 LOAD   the generator at the tine, the tine's t60 at c4 (s) and
//                   its load (the overtones' stretch, 1 the bare beam):
//                   "tine 2 1" pings, "tine 30 1.6" rings long
//   n MIDI VEL      a strike: the note, its velocity 0 .. 1
//   o               the key let go
//   p AMOUNT        the sustain pedal, 0 up .. 1 down
//   print           the twenty-four instrument channels as --set lines
//   ladder          c4 at velocities 0.2, 0.4, 0.6, 0.8 and 1, a second apart
//
// after each change it prints the string at the note: its length, its wire,
// its inharmonicity b, the hammer's mass and its felt's stiffness.
//
// teensy 4.x and the audio shield (sgtl5000): the voice on both channels. a
// pot on a0, its ends on 3.3 v and ground, is the hammer's hardness, log
// from 25 to 800 N/mm^p: turned, it writes the hardness channel.

#include <Audio.h>
#include <Upright.h>

AudioSynthUpright    piano;
AudioOutputI2S       i2s;
AudioConnection      cordLeft(piano, 0, i2s, 0);
AudioConnection      cordRight(piano, 0, i2s, 1);
AudioControlSGTL5000 codec;

const float kGain = 1.2f;                   // the voice's output gain: every preset at c2 .. c6 under full scale
const float kSoft = 25.0f;                  // N/mm^p: the hardness with the pot fully down ...
const float kHard = 800.0f;                 // ... and fully up

float     note = 60.0f;
char      line[64];
int       used = 0;
int       ladder = -1;                      // the ladder's next step, -1 when none
bool      ladderDown = false;
uint32_t  ladderMs = 0, potMs = 0;
float     lastPot = -1.0f;

// the instrument channels as --set lines, a line a group
void printShape() {
    const up::Shape s = piano.shape();
    const up::Channel first[6] = {up::Channel::length, up::Channel::hardness, up::Channel::boardArea, up::Channel::generator,
                                  up::Channel::dampRate, (up::Channel)up::kInstrumentChannels};
    for (int g = 0; g < 5; ++g) {
        Serial.print("--set");
        for (int i = (int)first[g]; i < (int)first[g + 1]; ++i) Serial.printf(" %s=%g", up::channelName((up::Channel)i), s.v[i]);
        Serial.println();
    }
}

// the rank's line at the note, from the targets
void report() {
    const up::Shape s = piano.shape();
    const up::RankState r = up::rankAt(s, note);
    const float g = s[up::Channel::generator];
    const float hardness = up::hammerHardnessAt(s[up::Channel::hardness], g) * r.hardnessScale;
    const float power = up::hammerExponentAt(s[up::Channel::exponent], r.pAdd);
    Serial.printf("  at midi %.1f (%.2f hz): L %.3f m, d %.3f mm%s, B %.3g, the hammer %.2f g, K %.1f N/mm^%.2f\n", note, r.f0, r.L,
                  1000.0f * (r.wrapped ? r.dCore : r.d), r.wrapped ? " (the core: wound)" : " (plain)", r.B, 1000.0f * r.hammerMass, hardness,
                  power);
}

// one typed line
void command(char* text) {
    char* word = strtok(text, " \t");
    if (!word) return;
    char* arg = strtok(NULL, " \t");
    char* arg2 = strtok(NULL, " \t");
    up::Channel c;
    if (!strcmp(word, "preset") && arg && up::presetByName(arg)) {
        piano.setShape(up::presetByName(arg)->shape());
        Serial.printf("  %s\n", arg);
        report();
    } else if (!strcmp(word, "set") && arg && arg2 && up::channelByName(arg, c) && up::isInstrumentChannel(c)) {
        float lo, hi;
        up::channelRange(c, lo, hi);
        piano.set(c, atof(arg2));
        Serial.printf("  %s %g (its range %g .. %g)\n", arg, atof(arg2), lo, hi);
        report();
    } else if (!strcmp(word, "tine") && arg && arg2) {
        up::Shape s = up::nanShape();
        s[up::Channel::generator] = 0.0f;
        s[up::Channel::tineT60] = atof(arg);
        s[up::Channel::tineLoad] = atof(arg2);
        piano.setShape(s);
        Serial.printf("  the tine: t60 %g s at c4, load %g\n", atof(arg), atof(arg2));
        report();
    } else if (!strcmp(word, "n") && arg) {
        note = constrain(atof(arg), 0.0f, 127.0f);
        piano.noteOn(note, arg2 ? constrain(atof(arg2), 0.0f, 1.0f) : 1.0f);
        report();
    } else if (!strcmp(word, "o")) {
        piano.noteOff();
    } else if (!strcmp(word, "p") && arg) {
        piano.setPedal(constrain(atof(arg), 0.0f, 1.0f));
    } else if (!strcmp(word, "print")) {
        printShape();
    } else if (!strcmp(word, "ladder")) {
        note = 60.0f;
        ladder = 0;
        ladderMs = millis() - 1000u;
    } else {
        Serial.println("preset NAME | set NAME VALUE | tine T60 LOAD | n MIDI VEL | o | p AMOUNT | print | ladder");
    }
}

// the pot, a hundred times a second: a move of more than a few counts writes the hardness, log between the two
void readPot(uint32_t now) {
    if (now - potMs < 10u) return;
    potMs = now;
    const float pot = (float)analogRead(A0) / 1023.0f;
    if (fabsf(pot - lastPot) <= 0.004f) return;
    lastPot = pot;
    piano.set(up::Channel::hardness, kSoft * powf(kHard / kSoft, pot));
}

void setup() {
    Serial.begin(115200);

    AudioMemory(8);
    codec.enable();
    codec.volume(0.5f);

    up::Tuning t;
    t.outputGain = kGain;
    piano.setTuning(t);
    piano.setShape(up::grand());
    while (!Serial && millis() < 3000) {}
    printShape();
    report();
}

void loop() {
    while (Serial.available()) {
        const char ch = (char)Serial.read();
        if (ch == '\n' || ch == '\r') {
            line[used] = '\0';
            if (used > 0) command(line);
            used = 0;
        } else if (used < (int)sizeof line - 1) {
            line[used++] = ch;
        }
    }

    const uint32_t now = millis();
    readPot(now);

    // the ladder: a strike a second, each let go 0.9 s after it
    if (ladder >= 0 && ladder < 5 && now - ladderMs >= 1000u) {
        ladderMs = now;
        const float vel = 0.2f * (float)(ladder + 1);
        piano.noteOn(60.0f, vel);
        ladderDown = true;
        Serial.printf("  ladder: c4 at velocity %.1f\n", vel);
        ++ladder;
    }
    if (ladderDown && now - ladderMs >= 900u) {
        piano.noteOff();
        ladderDown = false;
        if (ladder >= 5) ladder = -1;
    }
}
