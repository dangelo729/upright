// ----------------------------------------------------------------------------
// upright example 01 — Grand  (plan: module layout)
//
// one voice, grand (the eight knobs at 0 but generator -1), struck every two
// seconds at velocity 0.8, alternating c3 and c5, each released after a
// second (its damper lands). a pot on pin a0 is the hammer knob: fully down a
// soft felt, round, mellow even when loud; fully up a hard one, bright and
// percussive. a piano is a vector of eight: the pot moves one of them, and
// the whole vector is written again through up::pianoOf, so the voice eases
// its hammer there under the ring (nothing restarts).
//
// the level undone: a knob can move the loudness too, so the voice's gain
// undoes it. up::levelDb(knobs) is the level the measured table gives the
// vector against grand's (over the octave c4 .. b4); the tuning's outputGain
// is kGain times 10^(-levelDb / 20), written beside the vector, so turning
// the pot moves the hammer, not the volume (the hammer's row moves the level
// half a db end to end; the same two lines undo any knob). the gain lands at
// the next block, unramped: a pot's small steps move it a fraction of a db
// at a time.
//
// one voice is one key's strings: the next note asks the voice for the
// string at c5, so whatever the c3 string still rings retunes there under
// the new strike, keeping its velocity (its damper landed a second before,
// so little does). for keys that ring on beside each other, a pool
// (03_Keyboard).
//
// teensy 4.1 and the audio shield (sgtl5000). the voice's one output to both
// channels. the pot's wiper on a0, its ends on 3.3 v and ground. once a
// second the serial monitor shows the hammer, the level undone, the note,
// the last strike's contact and the cpu.
//
// the output gain is 4 before the level is undone, holding the largest
// peak near 0.51 of full scale: measured on the host at eleven hammers, c3
// and c5 at 0.8 as here for 8 s through the voice and its board, the largest
// peak is 0.128 of full scale at a gain of 1 (hammer +1.0), so here 0.51.
// ----------------------------------------------------------------------------

#include <Audio.h>
#include <Upright.h>

AudioSynthUpright    piano(1);
AudioOutputI2S       i2s;
AudioConnection      cordLeft(piano, 0, i2s, 0);
AudioConnection      cordRight(piano, 0, i2s, 1);
AudioControlSGTL5000 codec;

const float kNotes[2] = {48.0f, 72.0f};     // c3, c5
const float kVel      = 0.8f;
const float kGain     = 4.0f;               // the output gain before the level is undone

up::Knobs knobs;                            // the piano as a vector of eight
float     lastPot = -1.0f;
int       next = 0;
bool      down = false;                     // the key is held
uint32_t  lastStrikeMs = 0, lastReadMs = 0, lastPrintMs = 0;

float readPot() {
    return (float)analogRead(A0) / 1023.0f;
}

// the vector written whole, and the level it gives undone in the gain
void writePiano() {
    piano.setShape(up::pianoOf(knobs));
    up::Tuning t;
    t.outputGain = kGain * powf(10.0f, -up::levelDb(knobs) / 20.0f);
    piano.setTuning(t);
}

void setup() {
    Serial.begin(115200);

    AudioMemory(8);
    codec.enable();
    codec.volume(0.5f);

    knobs = up::presetByName("grand")->knobs;
    lastPot = readPot();
    knobs.v[(int)up::Knob::hammer] = 2.0f * lastPot - 1.0f;
    writePiano();
}

void loop() {
    const uint32_t now = millis();

    // a strike every two seconds, alternating c3 and c5; the key let go a second later
    if (now - lastStrikeMs >= 2000u) {
        lastStrikeMs = now;
        piano.noteOn(kNotes[next], kVel);
        next = 1 - next;
        down = true;
    }
    if (down && now - lastStrikeMs >= 1000u) {
        piano.noteOff();
        down = false;
    }

    // the pot, a hundred times a second; a move of a few counts is noise
    if (now - lastReadMs >= 10u) {
        lastReadMs = now;
        const float pot = readPot();
        if (fabsf(pot - lastPot) > 0.004f) {
            lastPot = pot;
            knobs.v[(int)up::Knob::hammer] = 2.0f * pot - 1.0f;
            writePiano();
        }
    }

    if (now - lastPrintMs >= 1000u) {
        lastPrintMs = now;
        const up::State s = piano.state();
        const float h = knobs.v[(int)up::Knob::hammer];
        Serial.print("hammer ");
        Serial.print(h, 2);
        Serial.print(" (");
        Serial.print(up::knobEnd(up::Knob::hammer, h < -0.33f ? 0 : (h > 0.33f ? 2 : 1)));
        Serial.print(")   level undone ");
        Serial.print(-up::levelDb(knobs), 2);
        Serial.print(" db   f0 ");
        Serial.print(s.f0, 1);
        Serial.print(" hz   contact ");
        Serial.print(1000.0f * s.hammerContact, 2);
        Serial.print(" ms");
        Serial.print(s.asleep ? "   asleep" : "");
        Serial.print("   cpu max ");
        Serial.print(AudioProcessorUsageMax(), 2);
        Serial.println(" %");
        AudioProcessorUsageMaxReset();
    }
}
