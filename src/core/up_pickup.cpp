// upright — up_pickup.cpp  (plan §7)
// the curves run per sample and call no libm; AmpPole::set() runs once per
// tuning write: cosf and sqrtf are fine there; Pickup::setBlock once per
// block (a division).
#include "up_pickup.h"

#include <cmath>

#include "up_config.h"

namespace up {
namespace {

inline float safeGap(float gap) { return gap > 1e-3f ? gap : 1e-3f; }

}  // namespace

float magneticC(float x, float gap, float offset) {
    const float u = (x - offset) / safeGap(gap);
    return 1.0f / (1.0f + u * u);
}

float magneticDeriv(float x, float gap, float offset) {
    const float g = safeGap(gap);
    const float u = (x - offset) / g;
    const float q = 1.0f + u * u;
    return -2.0f * u / (g * q * q);
}

float electroC(float x, float gap) { return magneticC(x, gap, safeGap(gap)); }

float electroDeriv(float x, float gap) { return magneticDeriv(x, gap, safeGap(gap)); }

float pickupOffset(float generator, float gap, float offset) {
    const float w = clamp(generator == generator ? generator : 0.0f, 0.0f, 1.0f);
    return offset + w * (safeGap(gap) - offset);
}

float pickupVoltage(float x, float xdot, float generator, float gap, float offset) {
    return magneticDeriv(x, gap, pickupOffset(generator, gap, offset)) * xdot;
}

void Pickup::setBlock(float generator, float gapMm, float offsetMm) {
    const float gap = safeGap(gapMm == gapMm ? gapMm : 1.5f);
    centre = pickupOffset(generator, gap, offsetMm == offsetMm ? offsetMm : 0.0f);
    invGap = 1.0f / gap;
    minusTwoOverGap = -2.0f * invGap;
}

// |H|² = (1 - b)² / (1 - 2 b cos w + b²) = 1 / 2 at w = 2 pi hz / fs:
// b² - (4 - 2 cos w) b + 1 = 0, the root under 1
void AmpPole::set(float hz, float fs) {
    const float nyq = 0.49f * (fs > 1.0f ? fs : kDefaultFs);
    const float f = clamp(hz == hz ? hz : 6000.0f, 1.0f, nyq);
    const float c = std::cos(2.0f * kPi * f / (fs > 1.0f ? fs : kDefaultFs));
    const float h = 2.0f - c;
    b_ = h - std::sqrt(h * h - 1.0f);
    a_ = 1.0f - b_;
}

}  // namespace up
