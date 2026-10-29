// ----------------------------------------------------------------------------
// upright example 99 — Benchmark  (plan: the acceptance gate, hardware;
// memory and time budget)
//
// what a piano costs on a teensy 4.1, at the worst case the plan names: a
// pool of eight, every partial of the build on (64 by default: the notes are
// e1 .. g3, under which every partial of every scale sits below the
// ceiling), three strings a note coupled through the bridge on their first
// sixteen partials (unison 0), a chord of eight re-struck every two
// seconds, and the scale knob gliding from spinet to concert and back over
// 20 s, written fifty times a second through setKnobs, so the law runs
// at every write and every partial's coefficients are computed again every
// block (an expm1 and a sin pair a partial a block, for the main string and
// its two secondaries: the libm the plan budgeted for a glide). then the
// pool of eight at rhodes (the electric side's cost: six partials, the
// tonebar, the pickup), then the same as the first on a pool of four.
//
// four phases, a line a second, then a summary, then again:
//
//   pool8   60 s   the AudioSynthUprightPool<8> playing grand; the pool of
//                  four asleep (its column is what four idle voices and an
//                  idle board cost)
//   rhodes  60 s   the pool of eight playing rhodes (the vector written at
//                  the phase's start, eased in under the ringing keys)
//   pool4   60 s   the AudioSynthUprightPool<4> playing grand; the pool of
//                  eight let go to sleep at the phase's start
//   ring    30 s   no strike, no glide: the pool of four rings out. a voice
//                  costs the same awake whatever its level, so the usage must
//                  stay flat while it rings and fall as the voices sleep; if
//                  it rises, denormals are getting through (flush-to-zero:
//                  AudioSynthUpright.cpp)
//
// columns: phase, second, total % now, total % max this second, the phase's
// max so far, the pool of eight's max this second, the pool of four's, the
// voices sounding in each, audio memory blocks in use at most. the second a
// phase begins in is left out of its max (the change lands inside it). the
// summary gives each pool's max and a voice's share of it (the pool's max
// over its voices): a pool is sized from that, beside the rest of the
// sketch's frame.
//
// the partial count is the build's, printed at the top: the line below
// builds the default (64) for make board. a 48-partial build is this sketch
// with -DUP_MAX_MODES=48 in its build flags (platformio's build_flags, or
// the line below changed), never a #define in the sketch: the library's own
// sources must see the same count.
//
// the output gain is 0.35 a voice and the mixer halves each pool, holding
// the mix under 0.1 of full scale: measured on the host as the phases play
// (20 s, the scale gliding, the level not undone), at a gain of 1 the pool
// of eight peaks 0.47 at grand and 0.54 at rhodes, the pool of four 0.35.
// the sum is for hearing that it rings, not for listening.
//
// teensy 4.1 + the audio shield (sgtl5000), 600 mhz. both pools into one
// mixer, to both channels.
//
// make board: -DUP_MAX_MODES=64
// ----------------------------------------------------------------------------

#include <Audio.h>
#include <Upright.h>

// every member of the pool wrapper compiled, the ones this sketch does not
// call too, so make board checks the whole template
template class AudioSynthUprightPool<8>;

AudioSynthUprightPool<8> pool8(1);
AudioSynthUprightPool<4> pool4(2);
AudioMixer4              mix;
AudioOutputI2S           i2s;
AudioConnection          c8(pool8, 0, mix, 0), c4(pool4, 0, mix, 1);
AudioConnection          cL(mix, 0, i2s, 0), cR(mix, 0, i2s, 1);
AudioControlSGTL5000     codec;

const float kNotes8[8] = {28.0f, 33.0f, 36.0f, 40.0f, 43.0f, 48.0f, 52.0f, 55.0f};   // e1 .. g3
const float kNotes4[4] = {36.0f, 43.0f, 48.0f, 55.0f};
const float kVel       = 0.8f;
const float kGain      = 0.35f;

// the phases, by index
const int         kPool8 = 0, kRhodes = 1, kPool4 = 2, kRing = 3, kPhases = 4;
const char* const kPhaseName[kPhases] = {"pool8 ", "rhodes", "pool4 ", "ring  "};
const uint32_t    kPhaseSec[kPhases]  = {60, 60, 60, 30};

int       phase = kPool8;
uint32_t  second = 0, lastPrintMs = 0, lastStrikeMs = 0, lastGlideMs = 0, phaseMs = 0;
float     phaseMax[kPhases], max8[kPhases], max4[kPhases], lastRing = 0.0f;
uint16_t  memMax = 0;
up::Knobs knobs;                            // grand or rhodes, the scale gliding

void strikeAll() {
    if (phase == kPool8 || phase == kRhodes)
        for (float n : kNotes8) pool8.noteOn(n, kVel);
    if (phase == kPool4)
        for (float n : kNotes4) pool4.noteOn(n, kVel);
}

// scale from spinet to concert and back over 20 s
void glide(uint32_t now) {
    const float t = (float)((now - phaseMs) % 20000u) / 10000.0f;          // 0 .. 2
    knobs.v[(int)up::Knob::scale] = t < 1.0f ? 2.0f * t - 1.0f : 3.0f - 2.0f * t;
    if (phase == kPool8 || phase == kRhodes) pool8.setKnobs(knobs);
    if (phase == kPool4) pool4.setKnobs(knobs);
}

void enter(int p) {
    phase = p;
    second = 0;
    phaseMs = millis();
    phaseMax[p] = max8[p] = max4[p] = 0.0f;
    knobs = up::presetByName(p == kRhodes ? "rhodes" : "grand")->knobs;
    if (p == kPool8 || p == kRhodes) {
        pool4.allNotesOff();                  // the other pool's dampers land: it sleeps
        pool8.setKnobs(knobs);
    } else if (p == kPool4) {
        pool8.allNotesOff();
        pool4.setKnobs(knobs);
    }
    if (p != kRing) {
        strikeAll();
        lastStrikeMs = phaseMs;
    }
}

void summary() {
    Serial.println("== summary (percent of the cpu at F_CPU_ACTUAL)");
    Serial.print("   partials a voice:           ");
    Serial.println(up::kMaxModes);
    Serial.print("   the pool of eight, grand:   max ");
    Serial.print(phaseMax[kPool8], 2);
    Serial.print(" total, the pool ");
    Serial.print(max8[kPool8], 2);
    Serial.print(", a voice ");
    Serial.println(max8[kPool8] / 8.0f, 2);
    Serial.print("   the pool of four, grand:    max ");
    Serial.print(phaseMax[kPool4], 2);
    Serial.print(" total, the pool ");
    Serial.print(max4[kPool4], 2);
    Serial.print(", a voice ");
    Serial.println(max4[kPool4] / 4.0f, 2);
    Serial.print("   the pool of eight, rhodes:  max ");
    Serial.print(phaseMax[kRhodes], 2);
    Serial.print(" total, the pool ");
    Serial.print(max8[kRhodes], 2);
    Serial.print(", a voice ");
    Serial.println(max8[kRhodes] / 8.0f, 2);
    Serial.print("   four idle voices (pool8):   max ");
    Serial.println(max4[kPool8], 2);
    Serial.print("   ring, its last second:      ");
    Serial.println(lastRing, 2);
    Serial.print("   audio memory, blocks:       max ");
    Serial.println(memMax);
    Serial.print("   commands dropped:           ");
    Serial.println(pool8.dropped() + pool4.dropped());
}

void setup() {
    Serial.begin(115200);

    AudioMemory(16);
    codec.enable();
    codec.volume(0.4f);
    mix.gain(0, 0.5f);
    mix.gain(1, 0.5f);

    up::Tuning t;
    t.outputGain = kGain;
    pool8.setTuning(t);
    pool4.setTuning(t);

    while (!Serial && millis() < 3000) {}
    Serial.print("upright benchmark: grand (then rhodes), three strings, a chord of eight re-struck every 2 s, the scale gliding; ");
    Serial.print(up::kMaxModes);
    Serial.println(" partials a voice");
    Serial.print("F_CPU_ACTUAL ");
    Serial.println(F_CPU_ACTUAL);
    Serial.print("sizeof: up::Voice ");
    Serial.print(sizeof(up::Voice));
    Serial.print(", up::Board ");
    Serial.print(sizeof(up::Board));
    Serial.print(", up::Pool<8> ");
    Serial.print(sizeof(up::Pool<8>));
    Serial.print(", AudioSynthUprightPool<8> ");
    Serial.print(sizeof(pool8));
    Serial.print(", AudioSynthUprightPool<4> ");
    Serial.println(sizeof(pool4));
    Serial.println("phase   s   total   max     phase max  pool8  pool4  sounding  mem");

    enter(kPool8);
    lastPrintMs = millis();
    AudioProcessorUsageMaxReset();
    AudioMemoryUsageMaxReset();
}

void loop() {
    const uint32_t now = millis();

    if (phase != kRing && now - lastStrikeMs >= 2000u) {
        lastStrikeMs = now;
        strikeAll();
    }
    if (phase != kRing && now - lastGlideMs >= 20u) {
        lastGlideMs = now;
        glide(now);
    }

    if (now - lastPrintMs < 1000u) return;
    lastPrintMs = now;
    ++second;

    const float total = AudioProcessorUsage();
    const float totalMax = AudioProcessorUsageMax();
    const float m8 = pool8.processorUsageMax();
    const float m4 = pool4.processorUsageMax();
    const uint16_t mem = AudioMemoryUsageMax();
    if (second > 1) {
        if (totalMax > phaseMax[phase]) phaseMax[phase] = totalMax;
        if (m8 > max8[phase]) max8[phase] = m8;
        if (m4 > max4[phase]) max4[phase] = m4;
    }
    if (mem > memMax) memMax = mem;
    if (phase == kRing) lastRing = totalMax;

    Serial.print(kPhaseName[phase]);
    Serial.print(second < 10 ? "  " : " ");
    Serial.print(second);
    Serial.print("   ");
    Serial.print(total, 2);
    Serial.print("   ");
    Serial.print(totalMax, 2);
    Serial.print("   ");
    Serial.print(phaseMax[phase], 2);
    Serial.print("      ");
    Serial.print(m8, 2);
    Serial.print("   ");
    Serial.print(m4, 2);
    Serial.print("   ");
    Serial.print(pool8.sounding());
    Serial.print(" + ");
    Serial.print(pool4.sounding());
    Serial.print("     ");
    Serial.println(mem);

    pool8.processorUsageMaxReset();
    pool4.processorUsageMaxReset();
    AudioProcessorUsageMaxReset();

    if (second >= kPhaseSec[phase]) {
        Serial.print("== ");
        Serial.print(kPhaseName[phase]);
        Serial.print(" max ");
        Serial.print(phaseMax[phase], 2);
        Serial.print(" % total, the pool of eight ");
        Serial.print(max8[phase], 2);
        Serial.print(" %, of four ");
        Serial.print(max4[phase], 2);
        Serial.println(" %");
        if (phase == kRing) {
            summary();
            enter(kPool8);
        } else {
            enter(phase + 1);
        }
    }
}
