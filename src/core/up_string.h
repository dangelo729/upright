// upright — up_string.h  (plan §5, §4)
// one note's strings: the main string, a bank of kMaxModes in the hammer's
// loop, and two secondary strings, banks of kUnisonModes on the main's first
// slots, driven by the same hammer force and coupled to it through the
// bridge (weinreich, "coupled piano strings", jasa 62, 1977).
//
//   the main     the voice hands every slot over per block: hz, its own
//                decay and the damper's, the drive scale (force to im per
//                substep), the readback weight (the strike point's shape),
//                the output weight
//   the unison   setUnison: the secondaries carry the main's first
//                kUnisonModes slots at hz 2^(+detune / 1200) (the second,
//                sharp) and 2^(-kThirdDetune detune / 1200) (the third,
//                flat by less: it never shares the second's frequency), the
//                main's own decay and the damper's alike. on the string side
//                they are the main's equals at their presence w_j (the
//                strings channel's fractions, the second x the tonebar from
//                the generator 0 up): struck, coupled and heard by it, the
//                drive the main's x w_j x the main's w over theirs and the
//                out the main's x w_j, so a string fading in is the same
//                fraction of a string throughout; toward the tine, at u
//                = clamp(1 + g, 0, 1), their outs x lerp(1, kUnisonGain, u)
//                and their decays over kAftersound^u: the tonebar heard
//                softer than the tine and ringing longer. a secondary under
//                kUnisonFloor of the main's weight is off and costs nothing
//   the bridge   setCoupling: the strings end on one bridge, an admittance
//                Y (m/s per N, flat above the plate's modes) with a resistive
//                part and a reactive one, bridgeReactive of it. each string
//                pushes it with a force its impedance times its velocity, the
//                bridge moves at Y times the sum, and each string feels that
//                motion: per partial under kUnisonModes, before every turn,
//                S = im_1 + w_2 im_2 + w_3 im_3 (the velocities' sum), R =
//                re_1 + w_2 re_2 + w_3 re_3 (the displacements'), and im_j -=
//                w_j (c S - c_r R) (w_1 = 1): c = 2 sigma_b dt, a loss on the
//                velocity, sigma_b the rank's bridgeLoss (T Y / L, one
//                string's amplitude decay into the bridge, the same on every
//                partial); c_r = 2 delta dt, delta = sigma_b bridgeReactive, a
//                force on the displacement that lowers the frequency (a
//                mass-like bridge: the strings' common motion carries its
//                mass); both x clamp(-g, 0, 1): the tine and its tonebar
//                share no bridge. the loss's matrix w w^T is symmetric, so it
//                only dissipates. the voice hands the main sigma_i = sigma_n -
//                sigma_b on these slots (sigma_n the law's single-string
//                rate; held at a tenth of sigma_n at least), so one string
//                alone decays at sigma_n; three in phase, as the hammer
//                leaves them, at sigma_n + 2 sigma_b: the prompt sound; the
//                strings against each other (S = 0) at sigma_i alone: the
//                aftersound, which barely moves the bridge. two equal
//                strings detuned by df (eps = pi df), k = sigma_b (1 + i
//                bridgeReactive), have modes at sigma_n -+ sqrt(k² - eps²):
//                while eps is under sigma_b they lock, two decays (the real
//                part) and no beat; past it they beat at |Im sqrt(k² -
//                eps²)| / pi, under the free detune's rate. on a resistive
//                bridge a symmetric strike puts the locked slow mode at the
//                bridge in opposite phase to the fast one, so their sum
//                would fall through a null where they cross; the reactive
//                part shifts the in-phase mode's frequency and not the
//                others' (n strings in phase sit about n delta, rad/s, under
//                the aftersound: at grand's c4, three strings, about 0.5
//                cents), so the prompt sound and the aftersound turn against
//                each other and never cancel: no null, a shallow wobble while
//                they are comparable. the detune in hz grows with the partial
//                and the coupling does not: the low partials lock, the upper
//                ones beat
//   the hammer   strike() places it at the string (at the rest, or where the
//                string stands if it is nearer the hammer than the rest:
//                never placed into the felt) with v0 toward it; while it
//                touches, every sample is kContactRate substeps of { y, ydot
//                = the main's readback as the phasors stand (mm to m); F =
//                hammer.step(y, ydot, dt / kContactRate) (up_hammer.h: the
//                felt's loss solved against the point mass, setPointMass);
//                every bank injects F; the coupling at the substep's c; every
//                bank turns by the substep's pair }, then the sums without a
//                turn; after it leaves, every sample the coupling at the
//                sample's c and every bank's step. the hammer sits on the
//                three strings and reads the main alone: over a contact of a
//                few ms the coupling moves nothing. a strike while the hammer
//                still touches places a fresh one; a strike on a sounding
//                string meets it as it moves (the readback holds its state):
//                nothing restarts
//
// step<kTouching, kBridge, kTip>: one sample; the bridge's sum (every bank's
// out re) and the tip's (tip re and tip w im over the first kTineModes slots
// of every bank, tip the out held to ±1: on the electric side the second
// string is the tonebar the pickup hears with the tine), each compiled out by
// its flag. the per-sample path calls no libm: the coupling is two sums and
// three multiply-adds a partial and the displacements' sum, over the slots
// beginBlock lists (the main's on under kUnisonModes; none while sigma_b and
// delta are 0). setUnison runs exp2 and pow per block.
#pragma once

#include "up_bank.h"
#include "up_config.h"
#include "up_hammer.h"

namespace up {

constexpr float kUnisonFloor = 1e-4f;       // a secondary under this share of the main's weight is off (-80 db)
constexpr float kThirdDetune = 0.7f;        // the third string sits at -0.7 x the detune (the second at +1 x): asymmetric

class Strings {
public:
    void init(float fs);
    void reset();                                       // every phasor zero, the hammer off

    // per block, the main string's slot k: hz, its own decay (1/s), the damper's (1/s, every string alike),
    // the drive scale per substep, the readback weight, the output weight
    void setPartial(int k, bool on, float hz, float decay, float damp, float drive, float readback, float out);
    // per block, after every slot: the secondaries at the detune (cents), their presences (0 .. 1) and the generator
    void setUnison(float detuneCents, float weight2, float weight3, float generator);
    // per block: sigma_b, one string's loss into the bridge (1/s), and delta, the reactive part's rate (1/s); 0 and
    // 0 couple nothing
    void setCoupling(float bridgeLoss, float bridgeReactiveRate);
    // the felt in the channels' units (per write: the table) and the string's point mass under it (kg, per block)
    void setHammer(float hardnessNmmP, float exponent, float feltSperMm, float massG);
    void setPointMass(float kg) { hammer_.setPointMass(kg); }
    void beginBlock();
    void endBlock() {}

    // a strike: the hammer placed at the string with velocity v0 (m/s), touching from the next sample
    void strike(float v0);

    // one sample; the bridge's sum (mm, x the out weights) and the tip's (mm, mm/s)
    template <bool kTouching, bool kBridge, bool kTip>
    inline void step(float& bridge, float& tipX, float& tipV);

    bool          touching() const { return hammer_.touching(); }
    const Hammer& hammer() const { return hammer_; }
    float         energySq() const { return main_.energySq() + second_.energySq() + third_.energySq(); }
    float         pointMass2() const { return main_.pointMass2(); }
    const Bank<kMaxModes>&    main() const { return main_; }
    const Bank<kUnisonModes>& second() const { return second_; }
    const Bank<kUnisonModes>& third() const { return third_; }
    int           coupled() const { return coupled_; }  // the slots the coupling runs over this block

    // the hammer's telemetry
    float blockPeak() const { return blockPeak_; }      // N: this block's largest force
    float strikePeak() const { return strikePeak_; }    // N: the strike's largest force so far
    float impulse() const { return impulse_; }          // N s: the strike's force integrated so far
    float contactSec() const { return contactSubs_ * dtSub_; }   // s: from the first compression to the hammer leaving
    float dtSub() const { return dtSub_; }
    // the last sample's substep forces (N), and whether it was a touching sample
    const float* substepForces() const { return forces_; }
    bool         sampleTouched() const { return touchedLast_; }

private:
    // the bridge over the listed slots: S = im_1 + w_2 im_2 + w_3 im_3, R = re_1 + w_2 re_2 + w_3 re_3,
    // im_j -= w_j (c S - cr R)
    inline void couple(float c, float cr);

    Bank<kMaxModes>    main_;
    Bank<kUnisonModes> second_, third_;
    Hammer             hammer_;
    // the main's first kUnisonModes slots as set this block, for the secondaries
    struct Slot { float hz = 0.0f, decay = 0.0f, damp = 0.0f, drive = 0.0f, out = 0.0f; bool on = false; };
    Slot  slot_[kUnisonModes];
    float dt_ = 1.0f / kDefaultFs;
    float dtSub_ = 1.0f / (kDefaultFs * kContactRate);
    // the coupling: the presences as set, the per-sample and per-substep c and c_r, the slots listed this block
    float w2_ = 0.0f, w3_ = 0.0f;
    float c_ = 0.0f, cSub_ = 0.0f, cr_ = 0.0f, crSub_ = 0.0f;
    int   slotOf_[kUnisonModes] = {};
    float cw2_[kUnisonModes] = {}, cw3_[kUnisonModes] = {};
    int   coupled_ = 0;
    float forces_[kContactRate] = {};
    float blockPeak_ = 0.0f, strikePeak_ = 0.0f, impulse_ = 0.0f;
    int   contactSubs_ = 0;
    bool  contacted_ = false, touchedLast_ = false;
};

inline void Strings::couple(float c, float cr) {
    for (int j = 0; j < coupled_; ++j) {
        const int k = slotOf_[j];
        float& a = main_.imAt(k);
        float& b = second_.imAt(k);
        float& d = third_.imAt(k);
        const float w2 = cw2_[j], w3 = cw3_[j];
        const float r = main_.reAt(k) + w2 * second_.reAt(k) + w3 * third_.reAt(k);
        const float s = c * (a + w2 * b + w3 * d) - cr * r;
        a -= s;
        b -= w2 * s;
        d -= w3 * s;
    }
}

template <bool kTouching, bool kBridge, bool kTip>
inline void Strings::step(float& bridge, float& tipX, float& tipV) {
    float b1, x1, v1, b2, x2, v2, b3, x3, v3;
    if (kTouching) {
        for (int r = 0; r < kContactRate; ++r) {
            float y, ydot;
            main_.readback(y, ydot);
            const float f = hammer_.step(y * 1e-3f, ydot * 1e-3f, dtSub_);
            forces_[r] = f;
            if (f > 0.0f) {
                contacted_ = true;
                blockPeak_ = f > blockPeak_ ? f : blockPeak_;
                strikePeak_ = f > strikePeak_ ? f : strikePeak_;
                impulse_ += f * dtSub_;
            }
            if (contacted_ && hammer_.touching()) ++contactSubs_;
            main_.inject(f);
            second_.inject(f);
            third_.inject(f);
            couple(cSub_, crSub_);
            main_.turnSub();
            second_.turnSub();
            third_.turnSub();
        }
        main_.sums<kBridge, kTip>(b1, x1, v1);
        second_.sums<kBridge, kTip>(b2, x2, v2);
        third_.sums<kBridge, kTip>(b3, x3, v3);
    } else {
        couple(c_, cr_);
        main_.step<kBridge, kTip>(b1, x1, v1);
        second_.step<kBridge, kTip>(b2, x2, v2);
        third_.step<kBridge, kTip>(b3, x3, v3);
    }
    touchedLast_ = kTouching;
    bridge = b1 + b2 + b3;
    tipX = x1 + x2 + x3;
    tipV = v1 + v2 + v3;
}

}  // namespace up
