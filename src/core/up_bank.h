// upright — up_bank.h  (plan: module layout, §4, §5, §8)
// the bank: N damped rotation oscillators, not filters (N a template
// parameter: the main string's kMaxModes, a secondary's kUnisonModes, the
// board's kBoardModes). each partial is a phasor z = (re, im) turned by r
// e^(-i w dt), r = exp(-decay dt), in increment form: z += z (ca -
// i cb), ca = r cos(w dt) - 1 from expm1 and sin²(w dt / 2), cb = r sin(w
// dt), so the decay and the pitch hold to the float's precision where w dt
// is small and r near 1. it turns clockwise, so a drive into im pushes re
// positive: re is the partial's modal displacement (mm on a string, um on
// the board), im its quadrature, w im its velocity (less sigma re, which the
// sums leave out). no doublets: a piano's partial is one phasor.
//
// per block, per partial (setPartial): on or off (off: zeroed and skipped),
// hz and decay (the sample's pair and the substep's, dt / subRate, computed
// only where they moved), the drive scale (force to im, per substep), the
// readback weight (the hammer's: the strike point's shape) and the output
// weight, a ramp across the block from the last block's value; a slot under
// kTineModes carries a second output weight beside it, the tip's, the out
// held to ±1 (a pickup reads the tip's displacement, whose modal weights are
// bounded by the shape's 1; the bridge's slope grows with n), ramped alike.
// the live list holds the partials on in slot order; the first of them under
// kTineModes are the tip's.
//
// a partial that moves grows in neither output: one ringing since the last
// block whose hz moved has its phasor scaled by min(1, hz_old / hz_new) (re
// and im, one division, no libm). going up it keeps its velocity w |z| (its
// displacement shrinks); going down it keeps its displacement |z| (its
// velocity shrinks). the outputs read both: the pickup's voltage and the
// board's modal sum read velocity, the bridge force reads displacement, so
// a string retuned up at its old millimetres would read w_new / w_old louder
// on a velocity output, and one retuned down at its old velocity would push
// the bridge hz_old / hz_new harder. no piano gets louder when a string is
// retuned. a partial coming on from off starts from rest, as before; the
// board's modes under a body write take the same rule.
//
// the per-sample path, multiplies and adds on what the block set up, no
// branch per partial but the live list, no libm:
//   readback   y = sum readback re, ydot = sum readback w im: the string under the hammer
//   inject     im += force drive (every partial on), or with a weight per partial (the board's voices)
//   turnSub    every partial turns by the substep's pair (the hammer's loop; no output)
//   step       every partial turns by the sample's pair; the output weights advance; the sums
//   sums       the same sums without a turn (after the substeps): the output weights advance
//   imAt, reAt a partial's im by reference and its re: the strings' coupling through the bridge (up_string.h)
// the sums, by the template's flags: the bridge's sum out re over every
// partial on (or, the step's kVelocity, sum out w im: the board's modes'
// velocity), and the tip's sum tip re and sum tip w im over the first
// kTineModes slots (the pickup's, at the tip's weights). beginBlock aims
// the ramps and computes what moved (rotationOf, up_bank.cpp: one expm1 and
// a sin pair a pair, per block, only where a partial moved).
#pragma once

#include <cmath>

#include "up_config.h"
#include "up_law.h"

namespace up {

constexpr float kMaxT60 = 1000.0f;           // s: the longest ring the bank keeps (a slower decay is this one)

// the turn over dt at hz and a decay rate (1/s), r e^(-i w dt) - 1 as (ca, cb): libm, per block where moved
void rotationOf(float hz, float decayPerSec, float dt, float& ca, float& cb);

template <int N>
class Bank {
public:
    void init(float fs, int subRate = kContactRate) {
        fs_ = fs > 1.0f ? fs : kDefaultFs;
        subRate_ = subRate < 1 ? 1 : subRate;
        for (Partial& p : m_) p = Partial();
        count_ = tip_ = 0;
    }

    // every phasor zero, every ramp at its target
    void reset() {
        for (Partial& p : m_) {
            p.re = p.im = 0.0f;
            p.outV = p.outT;
            p.outD = 0.0f;
            p.tipV = p.tipT;
            p.tipD = 0.0f;
        }
    }

    // per block, per partial: on, hz, decay (1/s), the drive scale (force to im, per substep), the readback
    // weight, the output weight (and the tip's, the out held to ±1). recomputes only what moved; a ringing
    // partial whose hz moved grows in neither output (its phasor x min(1, hz_old / hz_new))
    void setPartial(int k, bool on, float hz, float decayPerSec, float drive, float readback, float out) {
        if (k < 0 || k >= N) return;
        Partial& p = m_[k];
        if (!on || !(hz > 0.0f)) {
            p.on = false;
            return;
        }
        p.on = true;
        const float slowest = 6.907755f / kMaxT60;
        const float decay = decayPerSec > slowest ? decayPerSec : slowest;
        if (hz != p.hz || decay != p.decay) {
            if (p.was && hz > p.hz) {                         // ringing, moved up: w |z| kept (down: |z| kept)
                const float keep = p.hz / hz;
                p.re *= keep;
                p.im *= keep;
            }
            p.hz = hz;
            p.decay = decay;
            p.omega = 2.0f * kPi * hz;
            p.tuned = true;
        }
        p.drive = drive;
        p.readback = readback;
        p.rbw = readback * p.omega;
        p.next = out;
        p.tipNext = clamp(out, -1.0f, 1.0f);
    }

    // the live list, the moved partials' turns, the output ramps aimed at this block's weights
    void beginBlock() {
        count_ = tip_ = 0;
        for (int k = 0; k < N; ++k) {
            Partial& p = m_[k];
            if (!p.on) {
                if (p.was) p.re = p.im = 0.0f;           // dropped: zeroed, and skipped from here
                p.was = false;
                continue;
            }
            if (p.tuned) {
                rotationOf(p.hz, p.decay, 1.0f / fs_, p.ca, p.cb);
                if (subRate_ == 1) {
                    p.caSub = p.ca;
                    p.cbSub = p.cb;
                } else {
                    rotationOf(p.hz, p.decay, 1.0f / (fs_ * (float)subRate_), p.caSub, p.cbSub);
                }
                p.tuned = false;
            }
            if (p.was) {
                p.outV = p.outT;                          // from where the last block aimed
                p.outD = (p.next - p.outT) * kInvBlock;
                p.tipV = p.tipT;
                p.tipD = (p.tipNext - p.tipT) * kInvBlock;
            } else {
                p.re = p.im = 0.0f;                       // coming on: from rest, the weights at once
                p.outV = p.next;
                p.outD = 0.0f;
                p.tipV = p.tipNext;
                p.tipD = 0.0f;
            }
            p.outT = p.next;
            p.tipT = p.tipNext;
            p.was = true;
            live_[count_++] = k;
            if (k < kTineModes) tip_ = count_;
        }
    }

    void endBlock() {}

    // the string under the hammer, as the phasors stand: y = sum readback re, ydot = sum readback w im
    inline void readback(float& y, float& ydot) const {
        float a = 0.0f, b = 0.0f;
        for (int j = 0; j < count_; ++j) {
            const Partial& p = m_[live_[j]];
            a += p.readback * p.re;
            b += p.rbw * p.im;
        }
        y = a;
        ydot = b;
    }
    inline float readback() const {
        float y, ydot;
        readback(y, ydot);
        return y;
    }

    // the injection: im += force drive for every partial on
    inline void inject(float force) {
        for (int j = 0; j < count_; ++j) {
            Partial& p = m_[live_[j]];
            p.im += force * p.drive;
        }
    }
    // the same with a weight per slot (the board's: each voice at its bridge point)
    inline void injectWeighted(float force, const float* w) {
        for (int j = 0; j < count_; ++j) {
            const int k = live_[j];
            Partial& p = m_[k];
            p.im += force * w[k] * p.drive;
        }
    }

    // one substep: every partial on turns by the substep's pair (no output)
    inline void turnSub() {
        for (int j = 0; j < count_; ++j) {
            Partial& p = m_[live_[j]];
            const float re = p.re, im = p.im;
            p.re = re + (p.caSub * re + p.cbSub * im);
            p.im = im + (p.caSub * im - p.cbSub * re);
        }
    }

    // one sample: every partial on turns by the sample's pair; the sums (kBridge: sum out re over every
    // partial on, or with kVelocity sum out w im, the board's; kTip: sum tip re and sum tip w im over the tip's)
    template <bool kBridge, bool kTip, bool kVelocity = false>
    inline void step(float& sum, float& tipX, float& tipV) {
        float s = 0.0f, x = 0.0f, v = 0.0f;
        int j = 0;
        for (; j < tip_; ++j) {
            Partial& p = m_[live_[j]];
            const float re = p.re, im = p.im;
            const float nre = re + (p.ca * re + p.cb * im);
            const float nim = im + (p.ca * im - p.cb * re);
            p.re = nre;
            p.im = nim;
            const float w = p.outV += p.outD;
            const float t = p.tipV += p.tipD;
            if (kBridge || kTip) {
                const float o = w * nre, ot = t * nre;       // each product its own statement: no fused multiply-add
                if (kBridge && kVelocity) s += w * p.omega * nim;
                else if (kBridge) s += o;
                if (kTip) {
                    x += ot;
                    v += t * p.omega * nim;
                }
            }
        }
        for (; j < count_; ++j) {
            Partial& p = m_[live_[j]];
            const float re = p.re, im = p.im;
            const float nre = re + (p.ca * re + p.cb * im);
            const float nim = im + (p.ca * im - p.cb * re);
            p.re = nre;
            p.im = nim;
            const float w = p.outV += p.outD;
            if (kBridge && kVelocity) s += w * p.omega * nim;
            else if (kBridge) s += w * nre;
        }
        sum = s;
        tipX = x;
        tipV = v;
    }

    // the same sums without a turn (after the substeps): the output weights advance a sample
    template <bool kBridge, bool kTip>
    inline void sums(float& sum, float& tipX, float& tipV) {
        float s = 0.0f, x = 0.0f, v = 0.0f;
        int j = 0;
        for (; j < tip_; ++j) {
            Partial& p = m_[live_[j]];
            const float w = p.outV += p.outD;
            const float t = p.tipV += p.tipD;
            if (kBridge || kTip) {
                const float o = w * p.re, ot = t * p.re;
                if (kBridge) s += o;
                if (kTip) {
                    x += ot;
                    v += t * p.omega * p.im;
                }
            }
        }
        for (; j < count_; ++j) {
            Partial& p = m_[live_[j]];
            const float w = p.outV += p.outD;
            if (kBridge) s += w * p.re;
        }
        sum = s;
        tipX = x;
        tipV = v;
    }

    // a partial's im by reference (the strings' coupling, per sample, over the slots the strings list per block)
    inline float& imAt(int k) { return m_[k].im; }
    inline float  reAt(int k) const { return m_[k].re; }

    // readback (per block, the caller's)
    float amplitude(int k) const {                           // |z_k| (a sqrt)
        if (k < 0 || k >= N) return 0.0f;
        return std::sqrt(m_[k].re * m_[k].re + m_[k].im * m_[k].im);
    }
    void  phasor(int k, float& re, float& im) const {
        re = im = 0.0f;
        if (k < 0 || k >= N) return;
        re = m_[k].re;
        im = m_[k].im;
    }
    float energySq() const {                                 // sum re² + im² over the partials on: the silence gate's
        float e = 0.0f;
        for (int j = 0; j < count_; ++j) {
            const Partial& p = m_[live_[j]];
            e += p.re * p.re + p.im * p.im;
        }
        return e;
    }
    int   count() const { return count_; }                   // partials on (as of beginBlock)
    float pointMass2() const {                               // sum readback² over the partials on (as set): the point's
        float g = 0.0f;
        for (const Partial& p : m_)
            if (p.on) g += p.readback * p.readback;
        return g;
    }
    bool  on(int k) const { return k >= 0 && k < N && m_[k].on; }
    float drive(int k) const { return k >= 0 && k < N ? m_[k].drive : 0.0f; }
    float omega(int k) const { return k >= 0 && k < N ? m_[k].omega : 0.0f; }

private:
    struct Partial {
        // the per-sample state and coefficients
        float re = 0.0f, im = 0.0f;
        float ca = 0.0f, cb = 0.0f;                          // the sample's turn, r cos - 1 and r sin
        float caSub = 0.0f, cbSub = 0.0f;                    // the substep's
        float drive = 0.0f, readback = 0.0f, rbw = 0.0f;     // the injection, the readback, readback x omega
        float omega = 0.0f;                                  // rad/s
        float outV = 0.0f, outD = 0.0f;                      // the output weight's ramp
        float outT = 0.0f, next = 0.0f;                      // the weight the last block aimed at; this block's as set
        float tipV = 0.0f, tipD = 0.0f;                      // the tip's weight's ramp (the out held to ±1)
        float tipT = 0.0f, tipNext = 0.0f;
        // what was asked, to see what moved
        float hz = 0.0f, decay = 0.0f;
        bool  on = false, was = false, tuned = true;
    };

    float   fs_ = kDefaultFs;
    int     subRate_ = kContactRate;
    Partial m_[N];
    int     live_[N];                                        // the partials on, in slot order
    int     count_ = 0;
    int     tip_ = 0;                                        // how many of them sit under kTineModes
};

}  // namespace up
