// upright — up_tuning.cpp  (plan §11, module layout)
// runs once per tuning write, never per sample.
#include "up_tuning.h"

#include "up_config.h"

namespace up {
namespace {

inline bool isNan(float x) { return x != x; }
inline float real(float x, float fallback) { return isNan(x) ? fallback : x; }

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
    t.hammerScale = isNan(in.hammerScale) ? d.hammerScale : (in.hammerScale > 1e-3f ? in.hammerScale : 1e-3f);
    t.velCurve    = real(in.velCurve, d.velCurve);
    t.outputGain  = real(in.outputGain, d.outputGain);
    t.silence     = real(in.silence, d.silence);
    t.stealRate   = real(in.stealRate, d.stealRate);
    // under 0.01 no partial rings; over 0.5 a partial aliases
    t.ceiling     = isNan(in.ceiling) ? d.ceiling : clamp(in.ceiling, 0.01f, 0.5f);
    // a pole at or under 0 hz passes nothing; held over 1 hz
    t.ampHz       = isNan(in.ampHz) ? d.ampHz : (in.ampHz > 1.0f ? in.ampHz : 1.0f);
    // the attack's scale: none at 0, the calibration at 1, twice it at 2
    t.attack      = isNan(in.attack) ? d.attack : clamp(in.attack, 0.0f, 2.0f);
    return t;
}

}  // namespace up
