// ----------------------------------------------------------------------------
// upright example 02 — Knobs  (plan: module layout)
//
// one voice and the eight knobs from the serial monitor (115200, newline).
// the piano starts as grand; each change writes the vector whole through
// up::pianoOf, with the level table undone in the voice's gain (as
// 01_Grand), and prints the vector and the rank's line at the note: the
// string's speaking length, its wire (plain, or wound on a core), its
// inharmonicity B, the hammer's mass and its felt's stiffness K and power p.
//
//   k NAME VALUE    a knob, -1 .. 1: scale hammer strike unison body
//                   generator pickup damper (the ends are printed)
//   n MIDI VEL      a strike: the note, its velocity 0 .. 1
//   o               the key up: its damper lands (unless the pedal holds it)
//   p AMOUNT        the sustain pedal, 0 .. 1 (0 up, 1 down, between half)
//   print           the vector as a line of up_presets.cpp
//   random SEED     a vector drawn from a seed, each knob uniform in -1 .. 1
//   ladder          c4 at velocities 0.2, 0.4, 0.6, 0.8 and 1, a second apart,
//                   each let go after 0.9 s: velocity is brightness
//
// teensy 4.1 and the audio shield (sgtl5000). the voice's one output to both
// channels. the output gain is 0.55 before the level is undone, holding
// every vector undone under full scale: a vector undone lands at the level
// table's reference (grand's octave c4 .. b4 at -20.9 db rms), and measured
// on the host over three hundred random vectors (random 1 .. 300) struck at
// c4 and velocity 1 through the voice and its board, the level undone at a
// gain of 1, the median peak is 0.46 of full scale and the largest 1.18 (a
// vector in the generator's middle, where the attack's knock is undone with
// the quiet string), so here 0.25 and 0.65; the ladder at grand peaks 0.10
// here.
// ----------------------------------------------------------------------------

#include <Audio.h>
#include <Upright.h>

AudioSynthUpright    piano(1);
AudioOutputI2S       i2s;
AudioConnection      cordLeft(piano, 0, i2s, 0);
AudioConnection      cordRight(piano, 0, i2s, 1);
AudioControlSGTL5000 codec;

const float kGain = 0.55f;                  // the output gain before the level is undone

up::Knobs knobs;
float     note = 60.0f;
char      line[64];
int       used = 0;
int       ladder = -1;                      // the ladder's next step, -1 when none
bool      ladderDown = false;
uint32_t  ladderMs = 0;

// the vector written whole, and the level it gives undone in the gain
void writePiano() {
    piano.setShape(up::pianoOf(knobs));
    up::Tuning t;
    t.outputGain = kGain * powf(10.0f, -up::levelDb(knobs) / 20.0f);
    piano.setTuning(t);
}

void printVector() {
    Serial.printf("{\"vector\", {%.3ff, %.3ff, %.3ff, %.3ff, %.3ff, %.3ff, %.3ff, %.3ff}, 21, 108},\n", knobs.v[0], knobs.v[1], knobs.v[2],
                  knobs.v[3], knobs.v[4], knobs.v[5], knobs.v[6], knobs.v[7]);
}

// the vector, each knob by its ends, and the rank's line at the note
void report() {
    for (int k = 0; k < up::kKnobCount; ++k) {
        const float x = knobs.v[k];
        Serial.printf("  %-9s %+5.2f  (%s .. %s .. %s)\n", up::knobName((up::Knob)k), x, up::knobEnd((up::Knob)k, 0),
                      up::knobEnd((up::Knob)k, 1), up::knobEnd((up::Knob)k, 2));
    }
    const up::Shape s = up::pianoOf(knobs);
    const up::RankState r = up::rankAt(s, note);
    const float g = s[up::Channel::generator];
    const float hardness = up::hammerHardnessAt(s[up::Channel::hardness], g) * r.hardnessScale;
    const float power = up::hammerExponentAt(s[up::Channel::exponent], r.pAdd);
    Serial.printf("  at midi %.1f (%.2f hz): L %.3f m, d %.3f mm%s, B %.3g, the hammer %.2f g, K %.1f N/mm^%.2f; level %+.1f db against grand, "
                  "undone in the gain\n",
                  note, r.f0, r.L, 1000.0f * (r.wrapped ? r.dCore : r.d), r.wrapped ? " (the core: wound)" : " (plain)", r.B, 1000.0f * r.hammerMass,
                  hardness, power, up::levelDb(knobs));
    printVector();
}

// one typed line
void command(char* text) {
    char* word = strtok(text, " \t");
    if (!word) return;
    char* arg = strtok(NULL, " \t");
    char* arg2 = strtok(NULL, " \t");
    up::Knob k;
    if (!strcmp(word, "k") && arg && arg2 && up::knobByName(arg, k)) {
        knobs.v[(int)k] = constrain(atof(arg2), -1.0f, 1.0f);
        writePiano();
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
        printVector();
    } else if (!strcmp(word, "random") && arg) {
        up::Rng r;
        r.seed((uint32_t)strtoul(arg, NULL, 10));
        for (int i = 0; i < up::kKnobCount; ++i) knobs.v[i] = r.bipolar();
        writePiano();
        report();
    } else if (!strcmp(word, "ladder")) {
        note = 60.0f;
        ladder = 0;
        ladderMs = millis() - 1000u;
    } else {
        Serial.println("k NAME VALUE | n MIDI VEL | o | p AMOUNT | print | random SEED | ladder");
    }
}

void setup() {
    Serial.begin(115200);

    AudioMemory(8);
    codec.enable();
    codec.volume(0.5f);

    knobs = up::presetByName("grand")->knobs;
    writePiano();
    while (!Serial && millis() < 3000) {}
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

    // the ladder: a strike a second, each let go 0.9 s after it
    const uint32_t now = millis();
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
