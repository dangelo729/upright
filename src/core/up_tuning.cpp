// upright — up_tuning.cpp
// runs once per tuning write, never per sample.
#include "up_tuning.h"

#include "up_config.h"

namespace up {
namespace {

inline float real(float x, float fallback) { return std::isfinite(x) ? x : fallback; }

}  // namespace

Tuning clamped(const Tuning& in) {
    const Tuning d;
    Tuning t;
    // a rate at or under zero is a jump, and stands
    t.rateStrings = real(in.rateStrings, d.rateStrings);
    t.rateHammer  = real(in.rateHammer, d.rateHammer);
    t.rateBody    = real(in.rateBody, d.rateBody);
    t.rateModes   = real(in.rateModes, d.rateModes);
    t.ratePitch   = real(in.ratePitch, d.ratePitch);
    t.ratePedal   = real(in.ratePedal, d.ratePedal);
    // a hammer that never moves strikes nothing; held over 0
    t.hammerScale = std::isfinite(in.hammerScale) ? (in.hammerScale > 1e-3f ? in.hammerScale : 1e-3f) : d.hammerScale;
    t.velCurve    = real(in.velCurve, d.velCurve);
    // a level near 1e30 overflows the board's sums; held inside ±1e6
    t.outputGain  = clamp(real(in.outputGain, d.outputGain), -1e6f, 1e6f);
    t.silence     = real(in.silence, d.silence);
    t.stealRate   = real(in.stealRate, d.stealRate);
    // under 0.01 no partial rings; over 0.5 a partial aliases
    t.ceiling     = std::isfinite(in.ceiling) ? clamp(in.ceiling, 0.01f, 0.5f) : d.ceiling;
    // a pole at or under 0 hz passes nothing; held over 1 hz
    t.ampHz       = std::isfinite(in.ampHz) ? (in.ampHz > 1.0f ? in.ampHz : 1.0f) : d.ampHz;
    // the attack's scale: none at 0, the calibration at 1, twice it at 2
    t.attack      = std::isfinite(in.attack) ? clamp(in.attack, 0.0f, 2.0f) : d.attack;
    return t;
}

}  // namespace up
