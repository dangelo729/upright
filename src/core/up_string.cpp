// upright — up_string.cpp
// per block and per strike: the slots handed over, the secondaries derived
// from them (exp2 for the detune and pow for the tonebar's aftersound, per
// block), the coupling's rate and its slots, the felt's table (per hammer
// write), the hammer placed. the per-sample loop is the header's.
#include "up_string.h"

#include <cmath>

#include "up_law.h"

namespace up {

void Strings::init(float fs) {
    const float f = fs > 1.0f ? fs : kDefaultFs;
    main_.init(f);
    second_.init(f);
    third_.init(f);
    dt_ = 1.0f / f;
    dtSub_ = 1.0f / (f * (float)kContactRate);
    for (Slot& s : slot_) s = Slot();
    w2_ = w3_ = c_ = cSub_ = cr_ = crSub_ = 0.0f;
    coupled_ = 0;
    reset();
}

void Strings::reset() {
    main_.reset();
    second_.reset();
    third_.reset();
    hammer_.stop();
    for (float& f : forces_) f = 0.0f;
    blockPeak_ = strikePeak_ = impulse_ = 0.0f;
    contactSubs_ = 0;
    contacted_ = touchedLast_ = false;
}

void Strings::setPartial(int k, bool on, float hz, float decay, float damp, float drive, float readback, float out) {
    main_.setPartial(k, on, hz, decay + damp, drive, readback, out);
    if (k >= 0 && k < kUnisonModes) {
        Slot& s = slot_[k];
        s.on = on;
        s.hz = hz;
        s.decay = decay;
        s.damp = damp;
        s.drive = drive;
        s.out = out;
    }
}

// the secondaries at the detune, each struck and heard at its presence (the drive and the out x w_j); their outs
// x lerp(1, kUnisonGain, u) and their own decays over kAftersound^u, u = clamp(1 + g, 0, 1): the main's equals on
// the string side, the tonebar's at the tine
void Strings::setUnison(float detuneCents, float weight2, float weight3, float generator) {
    const float d = detuneCents == detuneCents ? detuneCents : 0.0f;
    const float up = std::exp2(d * (1.0f / 1200.0f)), upInv = 1.0f / up;
    const float down = std::exp2(-kThirdDetune * d * (1.0f / 1200.0f)), downInv = 1.0f / down;
    const float u = clamp(1.0f + (generator == generator ? generator : -1.0f), 0.0f, 1.0f);
    const float gain = lerp(1.0f, kUnisonGain, u);
    const float slower = std::pow(kAftersound, -u);
    const bool on2 = weight2 >= kUnisonFloor, on3 = weight3 >= kUnisonFloor;
    w2_ = on2 ? weight2 : 0.0f;
    w3_ = on3 ? weight3 : 0.0f;
    for (int k = 0; k < kUnisonModes; ++k) {
        const Slot& s = slot_[k];
        second_.setPartial(k, on2 && s.on, s.hz * up, s.decay * slower + s.damp, s.drive * upInv * weight2, 0.0f, s.out * gain * weight2);
        third_.setPartial(k, on3 && s.on, s.hz * down, s.decay * slower + s.damp, s.drive * downInv * weight3, 0.0f,
                          s.out * gain * weight3);
    }
}

// c = 2 sigma_b dt a sample and 2 sigma_b dtSub a substep: the amplitude's rate on a lone string; c_r the same of
// delta: a lone string's frequency lowered by delta (rad/s)
void Strings::setCoupling(float bridgeLoss, float bridgeReactiveRate) {
    const float loss = bridgeLoss > 0.0f ? bridgeLoss : 0.0f;     // a nan couples nothing
    const float delta = bridgeReactiveRate == bridgeReactiveRate ? bridgeReactiveRate : 0.0f;
    c_ = 2.0f * loss * dt_;
    cSub_ = 2.0f * loss * dtSub_;
    cr_ = 2.0f * delta * dt_;
    crSub_ = 2.0f * delta * dtSub_;
}

void Strings::setHammer(float hardnessNmmP, float exponent, float feltSperMm, float massG) { hammer_.set(hardnessNmmP, exponent, feltSperMm, massG); }

// every bank's block, then the coupling's slots: the main's on under kUnisonModes, each secondary's presence
// there (0 where it is off: its im is zero and stays so)
void Strings::beginBlock() {
    main_.beginBlock();
    second_.beginBlock();
    third_.beginBlock();
    blockPeak_ = 0.0f;
    coupled_ = 0;
    if (!(c_ > 0.0f) && cr_ == 0.0f) return;
    for (int k = 0; k < kUnisonModes; ++k) {
        if (!main_.on(k)) continue;
        slotOf_[coupled_] = k;
        cw2_[coupled_] = second_.on(k) ? w2_ : 0.0f;
        cw3_[coupled_] = third_.on(k) ? w3_ : 0.0f;
        ++coupled_;
    }
}

// the hammer at the string's rest, or where the string stands when it is nearer the hammer (below the rest):
// never placed into the felt
void Strings::strike(float v0) {
    const float y = main_.readback() * 1e-3f;
    hammer_.place(v0, y < 0.0f ? y : 0.0f);
    strikePeak_ = impulse_ = 0.0f;
    contactSubs_ = 0;
    contacted_ = false;
}

}  // namespace up
