// upright — up_hammer.cpp  (plan §4)
// set() runs once per hammer write and forceExact() on the host only: powf
// is fine in them. the step calls no libm: the felt's power comes from the
// table and sqrtf.
#include "up_hammer.h"

#include <cmath>

namespace up {
namespace {

inline float finiteOr(float x, float fallback) { return std::isfinite(x) ? x : fallback; }
constexpr float kTableScale = (float)(kFeltPoints - 1);

}  // namespace

void Hammer::set(float hardnessNmmP, float exponent, float feltSperMm, float massG) {
    p_ = clamp(finiteOr(exponent, 2.5f), 1.0f, 4.0f);
    const float hard = finiteOr(hardnessNmmP, 1.0f);
    k_ = (hard > 0.0f ? hard : 0.0f) * std::pow(1000.0f, p_);       // F = H (1000 c)^p
    const float felt = finiteOr(feltSperMm, 0.0f);
    lambda_ = (felt > 0.0f ? felt : 0.0f) * 1000.0f;                 // s/mm to s/m
    const float m = finiteOr(massG, 6.0f);
    mass_ = (m > 0.05f ? m : 0.05f) * 1e-3f;
    uScale_ = kTableScale / std::sqrt(kCmax);
    whole_ = (int)std::floor(2.0f * p_);
    const float frac = p_ - 0.5f * (float)whole_;
    for (int i = 0; i < kFeltPoints; ++i) {
        const float u = (float)i / kTableScale;
        table_[i] = std::pow(kCmax * u * u, frac);
    }
}

void Hammer::setPointMass(float kg) {
    point_ = kg > 1e-9f ? kg : 1e-9f;
    halfOverPoint_ = 0.5f / point_;
}

void Hammer::place(float v0, float y0) {
    y_ = finiteOr(y0, 0.0f);
    v_ = finiteOr(v0, 0.0f);
    force_ = 0.0f;
    time_ = 0.0f;
    active_ = true;
    contacted_ = false;
    first_ = true;
}

float Hammer::forceOf(float c, float cdot) const {
    if (!(c > 0.0f)) return 0.0f;
    const float s = std::sqrt(c);
    float u = s * uScale_;
    int i = (int)u;
    if (i > kFeltPoints - 2) i = kFeltPoints - 2;                    // past kCmax: the last segment, extended
    const float g = table_[i] + (table_[i + 1] - table_[i]) * (u - (float)i);
    float pw = 1.0f;
    for (int j = 0; j < whole_; ++j) pw *= s;
    const float f = k_ * pw * g * (1.0f + lambda_ * cdot);
    return f > 0.0f ? f : 0.0f;
}

float Hammer::forceExact(float c, float cdot) const {
    if (!(c > 0.0f)) return 0.0f;
    const float f = k_ * std::pow(c, p_) * (1.0f + lambda_ * cdot);
    return f > 0.0f ? f : 0.0f;
}

float Hammer::step(float yString, float ydotString, float dt) { return advance(yString, ydotString, dt, false); }
float Hammer::stepExact(float yString, float dt) { return advance(yString, 0.0f, dt, true); }

float Hammer::advance(float y, float ydotIn, float dt, bool exact) {
    if (!active_) {
        force_ = 0.0f;
        return 0.0f;
    }
    // the string's velocity under the hammer: the caller's, or (the truth) from the last two readbacks
    const float ydot = exact ? (first_ ? 0.0f : (y - yPrev_) / dt) : ydotIn;
    yPrev_ = y;
    first_ = false;
    const float c = y_ - y;
    const float cdot = v_ - ydot;
    if ((contacted_ && c <= 0.0f && v_ < 0.0f) || time_ >= kMaxContact) {
        active_ = false;
        force_ = 0.0f;
        return 0.0f;
    }
    float f = 0.0f;
    if (c > 0.0f) {
        contacted_ = true;
        if (exact) {
            f = forceExact(c, cdot);
        } else {
            // a = K c^p by the table; the loss against the string's point, the trapezoid across the step
            const float a = forceOf(c, 0.0f);
            f = a * (1.0f + lambda_ * cdot) / (1.0f + lambda_ * a * dt * halfOverPoint_);
            if (!(f > 0.0f)) f = 0.0f;
        }
        if (f > kForceCap) f = kForceCap;
    }
    v_ -= f / mass_ * dt;
    y_ += v_ * dt;
    time_ += dt;
    force_ = f;
    return f;
}

}  // namespace up
