// upright example 99: benchmark
// what a piano costs on a teensy at its worst case: every partial of the
// build on, three strings a note, a chord re-struck every two seconds, and
// the strings gliding from the spinet's to the concert grand's and back over
// 20 s, written fifty times a second, so the law runs on every write. four
// phases, a line a second, then a summary of each pool's processor usage
// and a voice's share of it:
//
//   pool8   60 s   a pool of eight playing grand
//   rhodes  60 s   the same pool playing rhodes
//   pool4   60 s   a pool of four playing grand
//   ring    30 s   no strikes, the pool of four ringing out: a voice costs
//                  the same at any level, so the usage holds while it rings
//                  and falls as its voices sleep; a rise is denormals
//
// teensy 4.x and the audio shield (sgtl5000): both pools into one mixer, on
// both channels. the build's partial count is printed first; a 48-partial
// build takes -DUP_MAX_MODES=48 in its build flags, never a #define in the
// sketch, so the library's sources see the same count.
//
// make board: -DUP_MAX_MODES=64

#include <Audio.h>
#include <Upright.h>

// every member of the pool wrapper compiled, the ones this sketch does not
// call too, so make board checks the whole template
template class AudioSynthUprightPool<8>;

AudioSynthUprightPool<8> pool8;
AudioSynthUprightPool<4> pool4;
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

void strikeAll() {
    if (phase == kPool8 || phase == kRhodes)
        for (float n : kNotes8) pool8.noteOn(n, kVel);
    if (phase == kPool4)
        for (float n : kNotes4) pool4.noteOn(n, kVel);
}

// the strings from the spinet's to the concert grand's and back over 20 s, each channel along the geometric mean's path
void glide(uint32_t now) {
    const float t = (float)((now - phaseMs) % 20000u) / 10000.0f;          // 0 .. 2
    const float u = t < 1.0f ? t : 2.0f - t;
    const up::Shape& from = up::spinet();
    const up::Shape& to = up::concertGrand();
    up::Shape s = up::nanShape();                                        // every other channel stands
    s[up::Channel::length] = from[up::Channel::length] * powf(to[up::Channel::length] / from[up::Channel::length], u);
    s[up::Channel::bass] = from[up::Channel::bass] * powf(to[up::Channel::bass] / from[up::Channel::bass], u);
    if (phase == kPool8 || phase == kRhodes) pool8.setShape(s);
    if (phase == kPool4) pool4.setShape(s);
}

void enter(int p) {
    phase = p;
    second = 0;
    phaseMs = millis();
    phaseMax[p] = max8[p] = max4[p] = 0.0f;
    const up::Shape& piano = p == kRhodes ? up::rhodes() : up::grand();
    if (p == kPool8 || p == kRhodes) {
        pool4.allNotesOff();                  // the other pool's dampers land: it sleeps
        pool8.setShape(piano);
    } else if (p == kPool4) {
        pool8.allNotesOff();
        pool4.setShape(piano);
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
    Serial.print("upright benchmark: grand (then rhodes), three strings, a chord of eight re-struck every 2 s, the strings gliding; ");
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
