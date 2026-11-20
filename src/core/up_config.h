// upright — up_config.h
// the build's sizes and the small helpers every module shares: clamp, the
// one smoothing law, the per-sample ramp, xorshift32, and the note's
// frequency.
//
// the sizes: kMaxModes partial slots on a note's main string (-D UP_MAX_MODES,
// 8 .. 96), kUnisonModes partials on each secondary string (-D
// UP_UNISON_MODES, 4 .. kMaxModes, 16 or the build's slots), kBoardModes plate modes on the soundboard
// (-D UP_BOARD_MODES, 4 .. 64), kContactRate substeps a sample while the
// hammer touches a string (-D UP_CONTACT_RATE, 1 .. 16); kLongModes free
// longitudinal modes a string rings in its attack (up_attack.h), no flag.
//
// src/core/ includes nothing but the c++ standard library. approachCoef and
// midiToHz call libm: once per block and once per write, never per sample.
#pragma once

#include <cmath>
#include <cstdint>

#ifndef UP_MAX_MODES
#define UP_MAX_MODES 64
#endif
#ifndef UP_UNISON_MODES
#define UP_UNISON_MODES (UP_MAX_MODES < 16 ? UP_MAX_MODES : 16)
#endif
#ifndef UP_BOARD_MODES
#define UP_BOARD_MODES 32
#endif
#ifndef UP_CONTACT_RATE
#define UP_CONTACT_RATE 2
#endif

namespace up {

// ---- the engine's shape --------------------------------------------------------
constexpr float kDefaultFs   = 44100.0f;
constexpr int   kBlock       = 128;                 // control block, samples (2.9 ms)
constexpr float kInvBlock    = 1.0f / (float)kBlock;
constexpr int   kMaxModes    = UP_MAX_MODES;        // the main string's partial slots: four channels each
constexpr int   kUnisonModes = UP_UNISON_MODES;     // the partials a secondary string carries
constexpr int   kBoardModes  = UP_BOARD_MODES;      // the plate's modes
constexpr int   kContactRate = UP_CONTACT_RATE;     // the strings' turns a sample while the hammer touches: 2, the
                                                    //   fewest within 5 % on the contact and 10 % on the peak of the
                                                    //   hammer's truth at every octave a0 .. c8 over the velocity ladder
constexpr int   kLongModes   = 4;                   // the string's free longitudinal modes in the attack (up_attack.h)
constexpr float kScaleMidi   = 60.0f;               // c4: the note the instrument's dimensions are given at

static_assert(kMaxModes >= 8 && kMaxModes <= 96, "UP_MAX_MODES must be 8..96");
static_assert(kUnisonModes >= 4 && kUnisonModes <= kMaxModes, "UP_UNISON_MODES must be 4..UP_MAX_MODES");
static_assert(kBoardModes >= 4 && kBoardModes <= 64, "UP_BOARD_MODES must be 4..64");
static_assert(kContactRate >= 1 && kContactRate <= 16, "UP_CONTACT_RATE must be 1..16");

// ---- the guards ----------------------------------------------------------------
constexpr float kPi      = 3.14159265358979f;
constexpr float kCeiling = 0.45f;                   // a partial over this share of fs is off

// ---- helpers ----------------------------------------------------------------------
inline float clamp(float x, float lo, float hi) { return x < lo ? lo : (x > hi ? hi : x); }
inline float lerp(float a, float b, float t) { return a + (b - a) * t; }

// the one smoothing law, once per block: x += (target - x) * (1 - exp(-rate * dt)).
// a rate at or below zero is a jump, and lands exactly on the target.
inline float approachCoef(float ratePerSec, float dt) {
    return ratePerSec <= 0.0f ? 1.0f : 1.0f - std::exp(-ratePerSec * dt);
}
inline float approach(float x, float target, float ratePerSec, float dt) {
    if (ratePerSec <= 0.0f) return target;
    return x + (target - x) * approachCoef(ratePerSec, dt);
}

// a value that moves linearly across one block, one step per sample.
// to() aims it at the block's end; snap() puts it there now.
struct Ramp {
    float v = 0.0f, dv = 0.0f;
    void to(float target) { dv = (target - v) * kInvBlock; }
    void snap(float target) { v = target; dv = 0.0f; }
    inline float next() { v += dv; return v; }
};

// one seeded xorshift32 per voice: the same calls give the same samples
struct Rng {
    uint32_t s = 0x6D2B79F5u;
    void seed(uint32_t v) { s = v ? v : 0x6D2B79F5u; }
    inline uint32_t next() {
        s ^= s << 13;
        s ^= s >> 17;
        s ^= s << 5;
        return s;
    }
    inline float bipolar() { return (float)(int32_t)next() * (1.0f / 2147483648.0f); }   // [-1, 1)
};

// the equal-tempered note's frequency, a4 = 440 hz
inline float midiToHz(float midi) { return 440.0f * std::exp2((midi - 69.0f) * (1.0f / 12.0f)); }

}  // namespace up
