// ----------------------------------------------------------------------------
// upright example 01 — Grand  (plan: module layout)
//
// one voice, grand, struck every two seconds at velocity 0.8, alternating c3
// and c5, each released after a second (its damper lands). a pot on pin a0
// is the hammer's hardness channel, log from 25 to 800 N/mm^p (chaigne &
// askenfelt's c2 hammer to their c6): fully down a soft felt, round, mellow
// even when loud; fully up a hard one, bright and percussive. the piano is
// grand's shape, written once with setShape; the pot writes the one channel
// with set, and the voice eases its hammer there under the ring at the
// tuning's rateHammer (nothing restarts).
//
// one voice is one key's strings: the next note asks the voice for the
// string at c5, so whatever the c3 string still rings retunes there under
// the new strike, keeping its velocity (its damper landed a second before,
// so little does). for keys that ring on beside each other, a pool
// (03_Keyboard).
//
// teensy 4.1 and the audio shield (sgtl5000). the voice's one output to both
// channels. the pot's wiper on a0, its ends on 3.3 v and ground. once a
// second the serial monitor shows the hardness, the note, the last strike's
// contact and the cpu.
//
// the output gain is 4, holding the largest peak near 0.54 of full scale:
// measured on the host at eleven hardnesses from 25 to 800, c3 and c5 at 0.8
// as here for 8 s through the voice and its board, the largest peak is 0.134
// of full scale at a gain of 1 (hardness 800), so here 0.54; the softest
// peaks 0.105, a db and a half under it.
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
const float kGain     = 4.0f;               // the voice's output gain
const float kSoft     = 25.0f;              // N/mm^p: the hardness with the pot fully down ...
const float kHard     = 800.0f;             // ... and fully up

float     hardness = 142.0f;
float     lastPot = -1.0f;
int       next = 0;
bool      down = false;                     // the key is held
uint32_t  lastStrikeMs = 0, lastReadMs = 0, lastPrintMs = 0;

float readPot() {
    return (float)analogRead(A0) / 1023.0f;
}

// the pot's 0 .. 1 as a hardness, log between the two hammers
float hardnessOf(float pot) {
    return kSoft * powf(kHard / kSoft, pot);
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
    lastPot = readPot();
    hardness = hardnessOf(lastPot);
    piano.set(up::Channel::hardness, hardness);
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
            hardness = hardnessOf(pot);
            piano.set(up::Channel::hardness, hardness);
        }
    }

    if (now - lastPrintMs >= 1000u) {
        lastPrintMs = now;
        const up::State s = piano.state();
        Serial.print("hardness ");
        Serial.print(hardness, 1);
        Serial.print(" N/mm^p (eased ");
        Serial.print(s.shape[up::Channel::hardness], 1);
        Serial.print(")   f0 ");
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
