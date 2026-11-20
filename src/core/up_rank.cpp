// upright — up_rank.cpp
// the rank runs once per note and once per instrument write, never per
// sample: powf and sqrtf are fine here.
#include "up_rank.h"

#include <cmath>

namespace up {
namespace {

inline float finiteOr(float x, float fallback) { return std::isfinite(x) ? x : fallback; }
inline float atLeast(float x, float lo) { return x > lo ? x : lo; }

}  // namespace

RankState rankAt(const Shape& s, float midi) { return rankAt(s, midi, lawConstants()); }

RankState rankAt(const Shape& s, float midi, const LawConstants& c) {
    RankState r;
    const float note = finiteOr(midi, kScaleMidi);
    const float fC4 = midiToHz(kScaleMidi);
    r.f0 = midiToHz(note);
    const float below = fC4 / r.f0;                       // f_c4 / f0: over 1 down the keyboard
    const float length = atLeast(finiteOr(s[Channel::length], c.l0), 1e-3f);
    const float bass = atLeast(finiteOr(s[Channel::bass], c.bass0), 1e-3f);
    const float tension = atLeast(finiteOr(s[Channel::tension], c.tension), 1.0f);
    const float gauge = atLeast(finiteOr(s[Channel::gauge], 1.0f), 1e-3f);

    const float scaled = length * std::pow(below, c.rankLength);
    r.L = scaled < bass ? scaled : bass;
    r.tension = tension;
    r.mu = tension / (4.0f * r.L * r.L * r.f0 * r.f0);
    r.Z = tension / (2.0f * r.L * r.f0);
    r.d = std::sqrt(4.0f * r.mu / (kPi * kRho)) * gauge;
    r.wrapped = r.d > c.dMax;
    const float muBreak = 0.25f * kPi * kRho * c.dMax * c.dMax;
    r.dCore = r.wrapped ? c.core * std::pow(r.mu / muBreak, c.coreGrow) : r.d;
    const float d2 = r.dCore * r.dCore;
    r.B = kPi * kPi * kPi * kE * d2 * d2 / (64.0f * tension * r.L * r.L);

    r.hammerMass = atLeast(finiteOr(s[Channel::hammerMass], c.massC4), 1e-3f) * 1e-3f * std::pow(below, c.rankMass);
    r.hardnessScale = std::pow(below, below > 1.0f ? -c.rankHardLo : -c.rankHardHi);   // f0 under c4's: the lower power
    r.pAdd = -c.rankP * std::log2(below);
    r.strikeShare = atLeast(finiteOr(s[Channel::strikeShare], 0.12f), 0.0f) * std::pow(below, c.rankStrike);
    const float t60Scale = std::pow(below, c.rankLoss);
    r.t60Scale = t60Scale < c.t60ScaleMax ? t60Scale : c.t60ScaleMax;
    r.bridgeScale = kPi * tension / r.L;
    r.bridgeLoss = tension * c.bridgeMobility / r.L;
    r.damped = note <= finiteOr(s[Channel::damperTop], 89.0f);
    r.longHz = std::sqrt(kE * 0.25f * kPi * d2 / r.mu) / (2.0f * r.L);   // c_L / (2 L), the core's stiffness on the whole mass
    return r;
}

}  // namespace up
