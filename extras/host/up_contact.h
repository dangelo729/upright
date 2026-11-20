// upright — up_contact.h
// the offline contact simulator: the engine's ground truth for the hammer,
// host-side, in double precision. a modal string of the law's partials at a
// note: each partial a damped oscillator at hz_n = ratio_n f0, decaying at
// 6.907755 / (t60_n t60Scale), of modal mass mu L / 2 (the rank's), driven at
// its strike-point shape gain_n; the string under the hammer y = sum gain_n q_n.
// an up::Hammer (the hammer channels at the rank's mass, its felt's
// stiffness x the rank's hardnessScale and its power + the rank's pAdd, the
// tip by the generator) steps against it at dt = 1 / fs with stepExact
// (powf, not the table, and the felt's loss explicit: the truth at a high
// rate), from place until it leaves and then on, free, for as long as the
// caller asks. useEngineStep() runs the engine's step instead (the table and
// the loss solved against the string's point mass, mu L / 2 over the sum of
// gain², with the point's own velocity sum gain w p): the engine's scheme at
// R substeps is this at R fs.
//
// one sample: the hammer reads y (the last sample's), its force F; every
// partial takes the impulse qdot += F gain_n dt / M, then turns by the exact
// damped rotation over dt (q and p = (qdot + sigma q) / w rotate by
// e^(-sigma dt) at w dt). the tip, where the pickup sits, is x = sum tip_n
// q_n and x' = sum tip_n qdot_n over the first kTineModes slots, tip_n the
// out held to ±1 as the engine's bank holds it, in metres (a string's upper
// partials belong to the bridge); the pickup's curve takes millimetres.
//
// it is what uptool hammer, pickup --from-hammer and chart, and the hammer
// tests run; the engine's float bank is held to it.
#pragma once

#include <cmath>
#include <vector>

#include "core/up_channels.h"
#include "core/up_hammer.h"
#include "core/up_law.h"
#include "core/up_pickup.h"
#include "core/up_rank.h"
#include "core/up_tuning.h"

namespace up {

constexpr double kPiD = 3.14159265358979323846;

struct ContactRun {
    std::vector<double> force;          // N, a sample each from place()
    std::vector<double> y;              // m: the string under the hammer
    std::vector<double> tip, tipVel;    // m, m/s: the tip (sum tip q, sum tip qdot)
    int    firstContact = -1;           // the first sample the felt is compressed
    int    left = -1;                   // the sample the hammer left, -1 when it never did
    double peak = 0.0;                  // N
    double impulse = 0.0;               // N s: the force's integral
    double momentum = 0.0;              // kg m/s: the hammer's m (v0 - v_end)
    double v0 = 0.0, vOut = 0.0;        // m/s: thrown, and leaving
    bool   finite = true;
    double fs = kDefaultFs;
    double contactMs() const {
        if (firstContact < 0) return 0.0;
        const int end = left >= 0 ? left : (int)force.size();
        return 1000.0 * (end - firstContact) / fs;
    }
};

class Contact {
public:
    struct Mode {
        double hz, sigma, gain, out;    // the partial (out: the tip's weight, the out held to ±1)
        double c, s, e;                 // the rotation over dt
        double q = 0.0, p = 0.0;        // displacement and (qdot + sigma q) / w
        double w;
    };

    // the law's table for s at the note (s's partial channels where they are
    // set, the law's where they are nan), the rank there, the hammer from s
    Contact(const Shape& s, float midi, const Tuning& t = Tuning(), const LawConstants& c = lawConstants(),
            double fs = kDefaultFs)
        : fs_(fs), dt_(1.0 / fs) {
        const Tuning tc = clamped(t);
        rank_ = rankAt(s, midi, c);
        table_ = partialsOf(s, rank_, tc, c);
        const PartialTable hand = fromShape(s);
        for (int k = 0; k < kMaxModes; ++k) {
            if (hand.ratio[k] == hand.ratio[k]) table_.ratio[k] = hand.ratio[k];
            if (hand.t60[k] == hand.t60[k]) table_.t60[k] = hand.t60[k];
            if (hand.gain[k] == hand.gain[k]) table_.gain[k] = hand.gain[k];
            if (hand.out[k] == hand.out[k]) table_.out[k] = hand.out[k];
        }
        const double nyq = 0.5 * fs;
        mass_ = 0.5 * (double)rank_.mu * (double)rank_.L;
        for (int k = 0; k < kMaxModes; ++k) {
            if (!partialOn(table_, k)) continue;
            Mode m;
            m.hz = (double)table_.ratio[k] * rank_.f0;
            if (!(m.hz > 0.0) || m.hz >= nyq) continue;
            const double t60 = (double)table_.t60[k] * rank_.t60Scale;
            m.sigma = 6.907755 / (t60 > 1e-9 ? t60 : 1e-9);
            m.gain = table_.gain[k];
            m.out = clamp(table_.out[k], -1.0f, 1.0f);      // the tip's weight: the out held to ±1
            m.w = 2.0 * kPiD * m.hz;
            m.c = std::cos(m.w * dt_);
            m.s = std::sin(m.w * dt_);
            m.e = std::exp(-m.sigma * dt_);
            modes_.push_back(m);
            slot_.push_back(k);
        }
        generator_ = s[Channel::generator] == s[Channel::generator] ? s[Channel::generator] : -1.0f;
        hammer_.set(hammerHardnessAt(s[Channel::hardness], generator_) * rank_.hardnessScale, hammerExponentAt(s[Channel::exponent], rank_.pAdd),
                    hammerFeltAt(s[Channel::felt], generator_), rank_.hammerMass * 1000.0f);
        double g2 = 0.0;
        for (const Mode& m : modes_) g2 += m.gain * m.gain;
        hammer_.setPointMass((float)(g2 > 0.0 ? mass_ / g2 : mass_));
        vScale_ = tc.hammerScale;
        velCurve_ = tc.velCurve;
    }

    // the engine's step (the felt table, the loss implicit) in place of the truth's
    void useEngineStep(bool on = true) { engine_ = on; }

    // the hammer placed at the string's rest, thrown at hammerScale vel^velCurve
    void place(float vel01) {
        const double v = vel01 > 0.0f ? vScale_ * std::pow((double)vel01, (double)velCurve_) : 0.0;
        hammer_.place((float)v);
        run_ = ContactRun();
        run_.fs = fs_;
        run_.v0 = hammer_.velocity();
        n_ = 0;
    }

    // one sample: the force on the string
    double step() {
        double y = 0.0, ydot = 0.0;
        for (const Mode& m : modes_) {
            y += m.gain * m.q;
            ydot += m.gain * m.w * m.p;                     // the point's velocity as the engine reads it (sigma q left out)
        }
        const bool was = hammer_.touching();
        const double f = engine_ ? hammer_.step((float)y, (float)ydot, (float)dt_) : hammer_.stepExact((float)y, (float)dt_);
        if (was && !hammer_.touching() && run_.left < 0) {
            run_.left = n_;
            run_.vOut = hammer_.velocity();
            run_.momentum = (double)hammer_.mass() * (run_.v0 - run_.vOut);
        }
        if (f > 0.0 && run_.firstContact < 0) run_.firstContact = n_;
        const double kick = f * dt_ / mass_;
        double x = 0.0, xd = 0.0;
        for (size_t i = 0; i < modes_.size(); ++i) {
            Mode& m = modes_[i];
            // the impulse on qdot, then the exact damped turn
            m.p += kick * m.gain / m.w;
            const double q = m.e * (m.q * m.c + m.p * m.s);
            const double p = m.e * (m.p * m.c - m.q * m.s);
            m.q = q;
            m.p = p;
            if (slot_[i] < kTineModes) {
                x += m.out * q;
                xd += m.out * (m.w * p - m.sigma * q);
            }
        }
        run_.force.push_back(f);
        run_.y.push_back(y);
        run_.tip.push_back(x);
        run_.tipVel.push_back(xd);
        run_.peak = f > run_.peak ? f : run_.peak;
        run_.impulse += f * dt_;
        run_.finite = run_.finite && std::isfinite(f) && std::isfinite(y) && std::isfinite(x);
        ++n_;
        return f;
    }

    // place, then run until the hammer leaves (or kMaxContact), then `freeSec` more
    const ContactRun& strike(float vel01, double freeSec) {
        place(vel01);
        const int cap = (int)std::ceil((kMaxContact + 0.002) * fs_);
        while (hammer_.touching() && n_ < cap) step();
        const int extra = (int)std::lround(freeSec * fs_);
        for (int i = 0; i < extra; ++i) step();
        return run_;
    }

    const ContactRun& run() const { return run_; }
    const RankState& rank() const { return rank_; }
    const PartialTable& table() const { return table_; }
    const Hammer& hammer() const { return hammer_; }
    const std::vector<Mode>& modes() const { return modes_; }
    const std::vector<int>& slots() const { return slot_; }
    double modalMass() const { return mass_; }
    double fs() const { return fs_; }
    float generator() const { return generator_; }

private:
    double fs_, dt_, mass_ = 1.0;
    RankState rank_;
    PartialTable table_;
    std::vector<Mode> modes_;
    std::vector<int> slot_;
    Hammer hammer_;
    float generator_ = -1.0f, vScale_ = 7.0f, velCurve_ = 2.0f;
    bool engine_ = false;
    ContactRun run_;
    int n_ = 0;
};

// the magnitude of a record's dft at hz, sum x_k e^(-j 2 pi hz k / fs), from `from`, `count` samples
// (all to the end at -1), under a hann window when `hann`
inline double dftAt(const std::vector<double>& x, double hz, double fs, int from = 0, int count = -1, bool hann = false) {
    const int n = count < 0 ? (int)x.size() - from : count;
    double re = 0.0, im = 0.0;
    const double w = 2.0 * kPiD * hz / fs;
    for (int k = 0; k < n && from + k < (int)x.size(); ++k) {
        const double win = hann ? 0.5 - 0.5 * std::cos(2.0 * kPiD * (k + 0.5) / n) : 1.0;
        re += win * x[(size_t)(from + k)] * std::cos(w * k);
        im -= win * x[(size_t)(from + k)] * std::sin(w * k);
    }
    return std::sqrt(re * re + im * im);
}

// the hammer's brightness: the force spectrum's mean level over partials 8 .. 16 against its mean over 1 .. 3,
// db, each partial's level by dftAt at its frequency over the run's force record; a partial off or over the
// ceiling (kCeiling kDefaultFs) skipped
inline double bandDb(const ContactRun& r, const PartialTable& table, double f0) {
    double hi = 0.0, lo = 0.0;
    int nHi = 0, nLo = 0;
    for (int n = 1; n <= 16 && n <= kMaxModes; ++n) {
        if (n > 3 && n < 8) continue;
        const int k = n - 1;
        const double hz = (double)table.ratio[k] * f0;
        if (!partialOn(table, k) || !(hz > 0.0) || hz > (double)kCeiling * kDefaultFs) continue;
        const double v = dftAt(r.force, hz, r.fs);
        const double db = 20.0 * std::log10(v > 1e-30 ? v : 1e-30);
        if (n <= 3) { lo += db; ++nLo; }
        else { hi += db; ++nHi; }
    }
    return nHi && nLo ? hi / nHi - lo / nLo : NAN;
}

// the pickup's voltage of a run's tip, from sample `from`, `count` samples
inline std::vector<double> pickupOf(const ContactRun& r, float generator, float gapMm, float offsetMm, int from, int count) {
    std::vector<double> v;
    for (int k = from; k < from + count && k < (int)r.tip.size(); ++k)
        v.push_back(pickupVoltage((float)(1000.0 * r.tip[(size_t)k]), (float)(1000.0 * r.tipVel[(size_t)k]), generator, gapMm, offsetMm));
    return v;
}

// the harmonics 1 .. count of f1 in a record (hann), and the thd: the rms of 2 .. count over the first
inline double thdOf(const std::vector<double>& v, double f1, double fs, int count = 8, double* h = nullptr) {
    double sum = 0.0, first = 0.0;
    for (int i = 1; i <= count; ++i) {
        const double a = i * f1 < 0.5 * fs ? dftAt(v, i * f1, fs, 0, -1, true) : 0.0;
        if (h) h[i - 1] = a;
        if (i == 1) first = a;
        else sum += a * a;
    }
    return first > 0.0 ? std::sqrt(sum) / first : 0.0;
}

// a sine of amplitude A mm at 100 hz, 100 cycles at 44.1 khz, through the pickup's voltage: the
// harmonics 1 .. 8 at their exact bins into h, the thd returned (the rms of 2 .. 8 over the first)
inline double sineThd(float ampMm, float generator, float gapMm, float offsetMm, double* h) {
    const double fs = 44100.0, f = 100.0;
    std::vector<double> v;
    for (int k = 0; k < 44100; ++k) {
        const double w = 2.0 * kPiD * f * k / fs;
        v.push_back(pickupVoltage((float)(ampMm * std::sin(w)), (float)(ampMm * 2.0 * kPiD * f * std::cos(w)), generator, gapMm, offsetMm));
    }
    double sum = 0.0;
    for (int i = 1; i <= 8; ++i) {
        h[i - 1] = dftAt(v, i * f, fs);
        if (i > 1) sum += h[i - 1] * h[i - 1];
    }
    return h[0] > 0.0 ? std::sqrt(sum) / h[0] : INFINITY;
}

// a strike at a note through the simulator and its tip through the pickup over the first
// `windowSec` after the hammer leaves: the thd over harmonics 2 .. 8 of the first partial
inline double tipThd(const Shape& s, float midi, float vel, const Tuning& t = Tuning(), const LawConstants& c = lawConstants(),
                     double windowSec = 0.1, double* peakTipMm = nullptr) {
    Contact sim(s, midi, t, c);
    const ContactRun& r = sim.strike(vel, windowSec + 0.001);
    const int from = r.left >= 0 ? r.left : (int)r.force.size() - (int)(windowSec * sim.fs());
    const int count = (int)(windowSec * sim.fs());
    if (peakTipMm) {
        double pk = 0.0;
        for (int k = from; k < from + count && k < (int)r.tip.size(); ++k) pk = std::fmax(pk, std::fabs(1000.0 * r.tip[(size_t)k]));
        *peakTipMm = pk;
    }
    const std::vector<double> v = pickupOf(r, sim.generator(), s[Channel::gap], s[Channel::offset], from, count);
    const double f1 = (double)sim.table().ratio[0] * sim.rank().f0;
    return thdOf(v, f1, sim.fs());
}

}  // namespace up
