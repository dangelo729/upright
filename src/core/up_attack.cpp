// upright — up_attack.cpp
// libm runs in init (the thud's turn, per rate) and in setBlock (a cos a
// mode, per block); the per-sample path is the header's.
#include "up_attack.h"

#include <cmath>

namespace up {

void Attack::init(float fs) {
    const float f = fs > 1.0f ? fs : kDefaultFs;
    long_.init(f, kContactRate);
    dtSub_ = 1.0f / (f * (float)kContactRate);
    thudFrom_ = (int)std::lround(kThudDelayMs * 1e-3f * f);
    const int thudN = (int)std::lround(kThudMs * 1e-3f * f);
    thudTo_ = thudFrom_ + (thudN > 1 ? thudN : 2);
    thudCos_ = std::cos(2.0f * kPi / (float)(thudTo_ - thudFrom_));
    thudSin_ = std::sin(2.0f * kPi / (float)(thudTo_ - thudFrom_));
    scale_ = blow_ = 0.0f;
    on_ = false;
    reset();
}

void Attack::reset() {
    long_.reset();
    parts_ = AttackParts();
    at_ = thudTo_;
    thudHalf_ = 0.0f;
}

// on while the scale is over 0, for the block after it falls to 0 (the outs ramp to 0 across it), or while the thud
// is due; off, every mode off (zeroed at beginBlock)
void Attack::setBlock(const RankState& r, float top, float scale) {
    const bool was = scale_ > 0.0f;
    scale_ = scale > 0.0f ? scale : 0.0f;
    blow_ = kKnockForce * scale_ / (float)kContactRate;
    on_ = scale_ > 0.0f || was || at_ < thudTo_;
    const float area = 0.25f * kPi * r.dCore * r.dCore;                  // m²: the core's
    const float cL = 2.0f * r.L * r.longHz;                               // m/s
    const float drive = kE * area * kLongShare * dtSub_ / (4.0f * r.tension * r.tension * r.mu * cL);
    const float out = kLongAngle * kE * area * kPi / r.L * scale_;
    const float decay = 6.907755f / kLongT60;
    const bool real = drive == drive && out == out && cL > 0.0f;
    for (int k = 0; k < kLongModes; ++k) {
        const int n = k + 1;
        const float hz = (float)n * r.longHz;
        if (!(on_ && real && hz > 0.0f && hz < top)) {
            long_.setPartial(k, false, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
            continue;
        }
        long_.setPartial(k, true, hz, decay, std::cos((float)n * kPi * r.strikeShare) * drive, 0.0f,
                         (float)n * ((n & 1) ? -out : out));
    }
    long_.beginBlock();
}

void Attack::strike(float momentum) {
    if (!(scale_ > 0.0f) || !(momentum > 0.0f)) return;
    thudHalf_ = 0.5f * kThud * momentum / (kThudMs * 1e-3f) * scale_;
    at_ = 0;
    tx_ = 1.0f;
    ty_ = 0.0f;
    on_ = true;
}

}  // namespace up
