// upright — tests/test_unison.cpp
//
// the coupled unison (up_string.h: weinreich's strings on one bridge whose
// admittance has a resistive part and a reactive one), on the voice's pair
// (the bridge force and the pickup) and a partial's band envelope (10 ms
// frames, cut where it last sits within 90 db of its peak). every expected
// number comes from the constants as the voice reads them: sigma_n the law's
// single-string rate at the partial (6.907755 / (t60 t60Scale)), sigma_b the
// rank's bridgeLoss, sigma_i = sigma_n - sigma_b, k = sigma_b (1 +
// i bridgeReactive), eps = pi df at the partial's own detune in hz, and two
// equal strings' modes at sigma_n -+ sqrt(k² - eps²).
//
//   one string at c6        no beat; one slope, -8.686 sigma_n db/s
//   three in phase at c6    detune 0: no beat; one slope, -8.686 (sigma_n +
//                           2 sigma_b): the coupling's rate directly
//   two locked at c4        0.3 cents: no dip (the reactive part turns the
//                           two decays against each other); the early slope
//                           the fast decay, the late the slow
//   two beating at c6       2 cents: the beat at |Im sqrt(k² - eps²)| / pi,
//                           under the free rate
//   three tuned             grand's detune at c4: no dip; the prompt
//                           sound's slope and the aftersound's; the prompt
//                           sound's pitch under the aftersound's; c6 printed
//   a beating partial       the 4th at c4, two strings at grand's detune:
//                           the formula's beat at its own eps; three strings
//                           printed
//   the comb                grand at c4 as play reads it: the 8th partial 60
//                           db down within 3 s; each partial's tracked t60
//                           and ripple printed
//   a fractional string     strings 2.5: struck, coupled and heard at half:
//                           finite, its energy between the 2 and 3 strings'
//   under a note            one string to three wide at rateStrings over
//                           a ringing c4: finite, under the click bound,
//                           nothing restarts
//   the tine side           rhodes at e4: nothing couples; the tonebar beats
//                           at the free rate of the preset's own detune (the
//                           two strings' frequency difference); its decay
//                           kAftersound x the tine's, printed
#include <cmath>
#include <complex>
#include <vector>

#include "core/up_presets.h"
#include "core/up_voice.h"
#include "up_analysis.h"
#include "up_play.h"
#include "up_test.h"

using namespace up;

namespace {

constexpr float  kFs = 44100.0f;
constexpr double kDbPerNeper = 8.685889638;   // 20 / ln 10
constexpr double kPiD = 3.14159265358979;

struct Note {
    Envelope e;                   // the slot's band envelope, 10 ms frames, cut 90 db under its peak
    std::vector<float> x;         // the voice's pair
    double hz = 0.0;              // the slot's frequency
    double sigN = 0.0, sigB = 0.0, sigI = 0.0, delta = 0.0;
    float  t60Scale = 1.0f;
};

// the shape at midi with the strings and the detune written, struck at velocity 1, sec seconds of the voice's
// pair; the slot's band envelope (half way to its neighbours, at most 0.2 of its frequency)
Note noteOf(const Shape& s, float midi, float strings, float detune, float sec, int slot = 0) {
    Voice v;
    v.setShape(s);
    v.set(Channel::strings, strings);
    v.set(Channel::detune, detune);
    v.noteOn(midi, 1.0f);
    Note n;
    std::vector<float> p((size_t)(sec * kFs));
    n.x.assign(p.size(), 0.0f);
    v.render(n.x.data(), p.data(), (int)p.size());
    for (size_t i = 0; i < p.size(); ++i) n.x[i] += p[i];
    const State st = v.state();
    PartialTable t;
    v.partials(t);
    n.hz = (double)t.ratio[slot] * st.f0;
    const double lo = slot ? (double)t.ratio[slot - 1] * st.f0 : 0.0, hi = (double)t.ratio[slot + 1] * st.f0;
    const double hw = std::fmin(0.4 * std::fmin(hi - n.hz, slot ? n.hz - lo : 1e9), 0.2 * n.hz);
    const BandAnalyser ba(n.x.data(), n.x.size(), kFs);
    n.e = ba.envelope(n.hz - hw, n.hz + hw, 0.01);
    double peak = -1e9;
    for (double d : n.e.db) peak = std::fmax(peak, d);
    size_t last = 0;
    for (size_t i = 0; i < n.e.db.size(); ++i)
        if (n.e.db[i] > peak - 90.0) last = i;
    n.e.db.resize(last + 1);
    const float g = clamp(std::isnan(s[Channel::generator]) ? -1.0f : s[Channel::generator], -1.0f, 1.0f);
    n.t60Scale = st.rank.t60Scale;
    n.sigN = 6.907755 / ((double)t.t60[slot] * st.rank.t60Scale);
    n.sigB = (double)st.rank.bridgeLoss * clamp(-g, 0.0f, 1.0f);
    n.sigI = n.sigN - std::fmin(n.sigB, 0.9 * n.sigN);
    n.delta = n.sigB * lawConstants().bridgeReactive;
    return n;
}

const Shape& grandShape() { return grand(); }

// grand's tuned detune
double tunedDetune() { return grandShape()[Channel::detune]; }

// pi df at a partial of hz, detuned by cents
double epsOf(double hz, double cents) { return kPiD * hz * (std::exp2(cents / 1200.0) - 1.0); }

// sqrt(k² - eps²), k = sigma_b (1 + i bridgeReactive): two equal strings' modes at sigma_n -+ it
std::complex<double> rootOf(double sigB, double eps) {
    const std::complex<double> k(sigB, sigB * lawConstants().bridgeReactive);
    return std::sqrt(k * k - eps * eps);
}

// the beat reader's ripple: the db envelope less its moving mean over windowSec, at frames centred in t0 .. t1
double rippleOf(const Envelope& e, double t0, double t1, double windowSec) {
    const int half = (int)std::lround(0.5 * windowSec / e.frameSec);
    double lo = 1e9, hi = -1e9;
    for (int i = (int)(t0 / e.frameSec); i < (int)(t1 / e.frameSec); ++i) {
        if (i - half < 0 || i + half >= (int)e.db.size()) continue;
        double sum = 0.0;
        for (int j = i - half; j <= i + half; ++j) sum += e.db[(size_t)j];
        const double r = e.db[(size_t)i] - sum / (2 * half + 1);
        lo = std::fmin(lo, r);
        hi = std::fmax(hi, r);
    }
    return hi > lo ? hi - lo : 0.0;
}

// the largest dip over t0 .. t1: each frame's db against the lower of the frames spanSec before and after it
double dipOf(const Envelope& e, double t0, double t1, double spanSec, double* at = nullptr) {
    const int span = (int)std::lround(spanSec / e.frameSec);
    double worst = -1e9;
    for (int i = (int)(t0 / e.frameSec); i < (int)(t1 / e.frameSec); ++i) {
        if (i - span < 0 || i + span >= (int)e.db.size()) continue;
        const double dip = std::fmin(e.db[(size_t)(i - span)], e.db[(size_t)(i + span)]) - e.db[(size_t)i];
        if (dip > worst) {
            worst = dip;
            if (at) *at = ((double)i + 0.5) * e.frameSec;
        }
    }
    return worst;
}

// a partial's frequency over t0 .. t1 s of a record: the peak near hz of its hann-windowed spectrum, zero-padded
// to 2^21 points, parabola-interpolated
double pitchOver(const std::vector<float>& x, double t0, double t1, double hz) {
    const size_t a = (size_t)(t0 * kFs), n = (size_t)((t1 - t0) * kFs);
    return peakNear(magnitude(x.data() + a, n, (size_t)1 << 21, Window::hann), kFs, hz, 30.0).hz;
}

}  // namespace

// one string at c6: no beat (the ripple under 0.5 db) and one slope, early
// and late within 10 % of -8.686 sigma_n db/s: sigma_i + sigma_b = sigma_n
UP_TEST(unison_one_string) {
    const Note n = noteOf(grandShape(), 84.0f, 1.0f, (float)tunedDetune(), 3.0f);
    const Beat b = beatOf(n.e, 0.2, 1.5);
    const TwoSlopes s = twoSlopes(n.e);
    const double want = -kDbPerNeper * n.sigN;
    CHECK_MSG(b.depthDb < 0.5, "the ripple %.3f db", b.depthDb);
    CHECK_MSG(std::fabs(s.early / want - 1.0) < 0.1 && std::fabs(s.late / want - 1.0) < 0.1, "early %.3f, late %.3f db/s against %.3f", s.early,
              s.late, want);
    printf("    one string at c6 (sigma_n %.4f, sigma_b %.4f): the ripple %.3f db; early %.3f, late %.3f db/s against %.3f (sigma_i + sigma_b)\n",
           n.sigN, n.sigB, b.depthDb, s.early, s.late, want);
}

// three strings at detune 0 at c6: in phase for good (S = 3 im), so no beat
// and one slope, early and late within 10 % of -8.686 (sigma_n + 2 sigma_b)
UP_TEST(unison_three_in_phase) {
    const Note n = noteOf(grandShape(), 84.0f, 3.0f, 0.0f, 3.0f);
    const Beat b = beatOf(n.e, 0.2, 1.5);
    const TwoSlopes s = twoSlopes(n.e);
    const double want = -kDbPerNeper * (n.sigN + 2.0 * n.sigB);
    CHECK_MSG(b.depthDb < 0.5, "the ripple %.3f db", b.depthDb);
    CHECK_MSG(std::fabs(s.early / want - 1.0) < 0.1 && std::fabs(s.late / want - 1.0) < 0.1, "early %.3f, late %.3f db/s against %.3f", s.early,
              s.late, want);
    printf("    three strings at c6, detune 0: the ripple %.3f db; early %.3f, late %.3f db/s against %.3f (sigma_n + 2 sigma_b; one string's %.3f)\n",
           b.depthDb, s.early, s.late, want, -kDbPerNeper * n.sigN);
}

// two strings locked: strings 2 at 0.3 cents at c4 (under the critical 551
// sigma_b / f0 cents), 16 s. on a resistive bridge the slow mode reached the
// bridge in opposite phase to the fast one and the sum fell through a null
// where they crossed; the reactive part turns them against each other, so
// held: no dip (each frame against the lower of the frames 1 s either side,
// the largest over 0.5 .. 15 s under 6 db), the early slope (the first
// second) within 15 % of -8.686 (sigma_n + Re sqrt(k² - eps²)), the late
// (10 .. 16 s) within 15 % of -8.686 (sigma_n - Re sqrt(k² - eps²)), the
// early steeper than the late by 2 at least; the beat reader's ripple over 2
// .. 6 s printed
UP_TEST(unison_two_strings_lock) {
    const Note n = noteOf(grandShape(), 60.0f, 2.0f, 0.3f, 16.0f);
    const double eps = epsOf(n.hz, 0.3);
    const std::complex<double> root = rootOf(n.sigB, eps);
    const double fast = -kDbPerNeper * (n.sigN + root.real()), slow = -kDbPerNeper * (n.sigN - root.real());
    const double resistive = std::sqrt(n.sigB * n.sigB - eps * eps);
    const TwoSlopes ts = twoSlopes(n.e);
    const double late = slopeOf(n.e, 10.0, 16.0);
    double at = 0.0;
    const double dip = dipOf(n.e, 0.5, 15.0, 1.0, &at), old = rippleOf(n.e, 2.0, 6.0, 2.0);
    CHECK_MSG(dip < 6.0, "the largest dip %.2f db at %.2f s", dip, at);
    CHECK_MSG(std::fabs(ts.early / fast - 1.0) < 0.15, "the early slope %.3f db/s against the fast decay's %.3f", ts.early, fast);
    CHECK_MSG(std::fabs(late / slow - 1.0) < 0.15, "the late slope %.3f db/s against the slow decay's %.3f", late, slow);
    CHECK_MSG(ts.early / late >= 2.0, "early %.3f, late %.3f db/s", ts.early, late);
    printf("    two strings at 0.3 cents at c4 (eps %.4f, sigma_b %.4f, delta %.4f): the largest dip %.2f db at %.2f s; the decays expected "
           "%.3f and %.3f db/s (Re sqrt %.4f; a resistive bridge's %.4f), read %.3f (the first second) and %.3f (10 .. 16 s); the beat "
           "reader's ripple over 2 .. 6 s %.2f db\n",
           eps, n.sigB, n.delta, dip, at, fast, slow, root.real(), resistive, ts.early, late, old);
}

// two strings beating: strings 2 at 2 cents at c6, past the lock: the beat
// within 2 % of |Im sqrt(k² - eps²)| / pi; the free rate and a resistive
// bridge's sqrt(eps² - sigma_b²) / pi printed beside
UP_TEST(unison_two_strings_beat) {
    const Note n = noteOf(grandShape(), 84.0f, 2.0f, 2.0f, 4.0f);
    const double eps = epsOf(n.hz, 2.0), want = std::fabs(rootOf(n.sigB, eps).imag()) / kPiD, freeRate = eps / kPiD;
    const double resistive = std::sqrt(eps * eps - n.sigB * n.sigB) / kPiD;
    const Beat b = beatOf(n.e, 0.2, 0.5);
    CHECK_MSG(std::fabs(b.rate / want - 1.0) < 0.02, "the beat %.4f hz against %.4f (%d troughs)", b.rate, want, b.troughs);
    printf("    two strings at 2 cents at c6: the beat %.4f hz against the coupled %.4f (%+.2f %%), %d troughs; the free rate %.4f (the coupled "
           "%+.2f %% under it), a resistive bridge's %.4f\n",
           b.rate, want, 100.0 * (b.rate / want - 1.0), b.troughs, freeRate, 100.0 * (want / freeRate - 1.0), resistive);
}

// three strings at grand's tuned detune (+d, -0.7 d) at c4, 16 s: no dip
// (as the pair's: the largest over 0.5 .. 15 s under 6 db); the early slope
// (the first second) within 15 % of -8.686 (sigma_n + 2 sigma_b), the in-
// phase prompt sound, and the late (10 .. 16 s) within 15 % of -8.686
// sigma_i, the aftersound; the prompt sound's pitch (the fundamental over 0.2
// .. 1 s) under the aftersound's (over 8 .. 12 s) by 0.2 .. 1 cents (about 3
// delta / 2 pi: the reactive part lowers the in-phase mode, a mass-like
// bridge); the beat reader's ripple over 2 .. 6 s printed (the resistive
// bridge's null read 16 db there); c6 (4 s) printed
UP_TEST(unison_three_strings_tuned) {
    const double d = tunedDetune();
    const Note n = noteOf(grandShape(), 60.0f, 3.0f, (float)d, 16.0f);
    const double early = -kDbPerNeper * (n.sigN + 2.0 * n.sigB), after = -kDbPerNeper * n.sigI;
    const TwoSlopes s = twoSlopes(n.e);
    const double late = slopeOf(n.e, 10.0, 16.0);
    double at = 0.0, atCross = 0.0;
    const double dip = dipOf(n.e, 0.5, 15.0, 1.0, &at), cross = dipOf(n.e, 0.5, 5.0, 1.0, &atCross), old = rippleOf(n.e, 2.0, 6.0, 2.0);
    const double prompt = pitchOver(n.x, 0.2, 1.0, n.hz), aft = pitchOver(n.x, 8.0, 12.0, n.hz);
    const double cents = 1200.0 * std::log2(prompt / aft), expect = 1200.0 * std::log2(1.0 - 3.0 * n.delta / (2.0 * kPiD * n.hz));
    CHECK_MSG(dip < 6.0, "the largest dip %.2f db at %.2f s", dip, at);
    CHECK_MSG(std::fabs(s.early / early - 1.0) < 0.15, "the early slope %.3f db/s against the prompt sound's %.3f", s.early, early);
    CHECK_MSG(std::fabs(late / after - 1.0) < 0.15, "the late slope %.3f db/s against the aftersound's %.3f", late, after);
    CHECK_MSG(cents <= -0.2 && cents >= -1.0, "the prompt sound %.4f hz, the aftersound %.4f hz: %+.3f cents", prompt, aft, cents);
    const Note c6 = noteOf(grandShape(), 84.0f, 3.0f, (float)d, 4.0f);
    const TwoSlopes s6 = twoSlopes(c6.e);
    printf("    three strings at c4 (+%.2f, -%.2f cents): the largest dip %.2f db at %.2f s (%.2f db at %.2f s over 0.5 .. 5 s, where the prompt "
           "sound and the aftersound cross); early %.3f against the prompt sound's %.3f, late (10 .. 16 s) %.3f against the aftersound's "
           "%.3f db/s; the prompt sound %.4f hz, the aftersound %.4f hz: %+.3f cents (3 delta / 2 pi: %+.3f); the beat reader's ripple "
           "over 2 .. 6 s %.2f db\n",
           d, d * kThirdDetune, dip, at, cross, atCross, s.early, early, late, after, prompt, aft, cents, expect, old);
    printf("    three strings at c6: early %.3f (in phase %.3f), late %.3f db/s (the aftersound's %.3f, one string's %.3f)\n", s6.early,
           -kDbPerNeper * (c6.sigN + 2.0 * c6.sigB), s6.late, -kDbPerNeper * c6.sigI, -kDbPerNeper * c6.sigN);
}

// a beating partial: grand's c4, two strings at its detune: the 4th
// partial's envelope (its band) beats within 5 % of |Im sqrt(k² - eps²)| /
// pi at its own eps; the same partial with three strings printed (three
// beat rates at once)
UP_TEST(unison_partial_beats) {
    const double d = tunedDetune();
    const Note n = noteOf(grandShape(), 60.0f, 2.0f, (float)d, 10.0f, 3);
    const double eps = epsOf(n.hz, d), want = std::fabs(rootOf(n.sigB, eps).imag()) / kPiD;
    const Beat b = beatOf(n.e, 0.2, 1.5);
    CHECK_MSG(std::fabs(b.rate / want - 1.0) < 0.05, "the 4th partial beats %.4f hz against %.4f (%d troughs)", b.rate, want, b.troughs);
    const Note three = noteOf(grandShape(), 60.0f, 3.0f, (float)d, 10.0f, 3);
    const Beat b3 = beatOf(three.e, 0.2, 1.5);
    printf("    the 4th partial at c4 (%.1f hz), two strings at %.2f cents: the beat %.4f hz against %.4f (%+.2f %%; free %.4f), %d troughs, the "
           "ripple %.1f db; three strings: %.4f hz, %d troughs, the ripple %.1f db\n",
           n.hz, d, b.rate, want, 100.0 * (b.rate / want - 1.0), eps / kPiD, b.troughs, b.depthDb, b3.rate, b3.troughs, b3.depthDb);
}

// the comb: grand at c4, velocity 1, 4 s as play renders and reads it: the
// 8th partial 60 db under its peak within 3 s (the last frame of its band
// within 60 db, before the record's last 0.25 s, where its cut splashes
// into every band; the tracker's t60 printed); each of partials 2 .. 8 its
// tracked t60 against the law's and its ripple over 0.3 .. 3 s against a 1
// s mean, printed (not held)
UP_TEST(unison_comb) {
    PlaySpec sp;
    sp.instrument = grandShape();
    sp.sec = 4.0f;
    const PlayRender r = renderPlay(sp);
    const PlayReading rd = readPlay(r, sp);
    const BandAnalyser bands(r.pair.data(), r.pair.size(), kFs);
    double worst = 0.0, eighth = 0.0, gone = 0.0;
    int worstAt = 0;
    printf("    grand at c4, velocity 1, partials 2 .. 8: ");
    for (int k = 1; k <= 7 && k + 1 < kMaxModes; ++k) {
        const double f = (double)r.table.ratio[k] * r.f0, lo = (double)r.table.ratio[k - 1] * r.f0, hi = (double)r.table.ratio[k + 1] * r.f0;
        const double hw = std::fmin(0.4 * std::fmin(hi - f, f - lo), 0.2 * f);
        const Envelope e = bands.envelope(f - hw, f + hw, 0.01);
        const double rip = rippleOf(e, 0.3, 3.0, 1.0);
        double t60 = 0.0;
        for (size_t i = 0; i < rd.slot.size(); ++i)
            if (rd.slot[i] == k) t60 = rd.tracked[i].decay.t60;
        if (rip > worst) { worst = rip; worstAt = k + 1; }
        if (k == 7) {
            eighth = t60;
            double peak = -1e9;
            for (double v : e.db) peak = std::fmax(peak, v);
            for (size_t i = 0; i < e.db.size() && ((double)i + 1.0) * e.frameSec <= sp.sec - 0.25; ++i)
                if (e.db[i] > peak - 60.0) gone = ((double)i + 1.0) * e.frameSec;
        }
        printf("%d: t60 %.2f s (law %.2f), ripple %.1f db%s", k + 1, t60, r.table.t60[k] * r.struck.rank.t60Scale, rip, k < 7 ? "; " : "\n");
    }
    CHECK_MSG(gone > 0.0 && gone < 3.0, "the 8th partial within 60 db of its peak until %.2f s", gone);
    printf("    the 8th partial 60 db under its peak from %.2f s (under 3), the tracker's t60 %.2f s; the largest ripple over 2 .. 8 within 0.3 .. "
           "3 s %.1f db (the %dth)\n",
           gone, eighth, worst, worstAt);
}

// a fractional string: strings 2.5 at c4 (the third at half presence:
// struck, coupled and heard at half): finite; its energy at 1 s between the
// 2 and the 3 strings' (a check), and falling from 1 s to 2 s; its energy
// never rising from block to block once the hammer has left (no growth);
// its bridge force's rms over 0.9 .. 1.1 s and 1.9 .. 2.1 s within 1 db of
// the span the 2 and 3 strings' make
UP_TEST(unison_fractional_string) {
    struct Run { double r1, r2, e1, e2, rise; bool finite; };
    auto run = [](float strings) {
        Voice v;
        v.setShape(grandShape());
        v.set(Channel::strings, strings);
        v.noteOn(60.0f, 1.0f);
        const int blocks = (int)(2.2f * kFs) / kBlock, at1 = (int)kFs / kBlock, at2 = (int)(2.0f * kFs) / kBlock;
        std::vector<float> x((size_t)blocks * kBlock);
        Run r{0, 0, 0, 0, 0, true};
        double last = -1.0;
        for (int b = 0; b < blocks; ++b) {
            v.render(x.data() + (size_t)b * kBlock, nullptr, kBlock);
            const double e = v.energy();
            if (!v.state().hammerTouching && last > 0.0) r.rise = std::fmax(r.rise, e / last - 1.0);
            last = e;
            if (b == at1) r.e1 = e;
            if (b == at2) r.e2 = e;
        }
        const size_t w = (size_t)(0.2 * kFs);
        r.r1 = rms(x.data() + (size_t)(0.9 * kFs), w);
        r.r2 = rms(x.data() + (size_t)(1.9 * kFs), w);
        r.finite = allFinite(x.data(), x.size());
        return r;
    };
    const Run two = run(2.0f), half = run(2.5f), three = run(3.0f);
    auto within = [](double x, double a, double b) {
        const double lo = std::fmin(a, b) * std::pow(10.0, -1.0 / 20.0), hi = std::fmax(a, b) * std::pow(10.0, 1.0 / 20.0);
        return x >= lo && x <= hi;
    };
    CHECK(half.finite);
    CHECK_MSG(half.e1 >= std::fmin(two.e1, three.e1) && half.e1 <= std::fmax(two.e1, three.e1), "the energy at 1 s %.4g against %.4g .. %.4g",
              half.e1, two.e1, three.e1);
    CHECK_MSG(half.rise <= 1e-6 && half.e2 < half.e1, "the energy rose %.3g at most; %.4g at 1 s, %.4g at 2 s", half.rise, half.e1, half.e2);
    CHECK_MSG(within(half.r1, two.r1, three.r1) && within(half.r2, two.r2, three.r2), "the rms %.4f, %.4f against %.4f .. %.4f, %.4f .. %.4f",
              half.r1, half.r2, two.r1, three.r1, two.r2, three.r2);
    printf("    strings 2, 2.5, 3 at c4: the energy at 1 s %.3f, %.3f, %.3f (2.5's largest rise from block to block %.2g); the bridge force's "
           "rms at 1 s %.3f, %.3f, %.3f, at 2 s %.3f, %.3f, %.3f\n",
           two.e1, half.e1, three.e1, half.rise, two.r1, half.r1, three.r1, two.r2, half.r2, three.r2);
}

// the unison under a note: grand's c4 on one string struck, and at 1 s the
// instrument written with three strings 20 cents wide and eased at
// rateStrings: the bridge force finite, its click measure from
// 0.25 s under the bound for the top partial, and nothing restarts: every
// bank's energy never rises from block to block once the hammer has left
UP_TEST(unison_under_a_note) {
    Shape one = grand(), wide = grand();
    one[Channel::strings] = 1.0f;
    wide[Channel::detune] = 20.0f;
    Voice v;
    v.setShape(one);
    v.noteOn(60.0f, 1.0f);
    const int blocks = (int)(4.0f * kFs) / kBlock, at = (int)kFs / kBlock;
    std::vector<float> x((size_t)blocks * kBlock);
    double rise = 0.0, last = -1.0;
    float top = 0.0f;
    for (int b = 0; b < blocks; ++b) {
        if (b == at) v.setShape(wide);
        v.render(x.data() + (size_t)b * kBlock, nullptr, kBlock);
        const double e = v.energy();
        if (!v.state().hammerTouching && last > 0.0) rise = std::fmax(rise, e / last - 1.0);
        last = e;
        PartialTable pt;
        v.partials(pt);
        for (int s = 0; s < kMaxModes; ++s)
            if (pt.ratio[s] * v.state().f0 < kCeiling * kFs && (pt.gain[s] != 0.0f || pt.out[s] != 0.0f)) top = std::fmax(top, pt.ratio[s] * v.state().f0);
    }
    const size_t from = (size_t)(0.25 * kFs);
    const double click = clickRatio(x.data() + from, x.size() - from, kFs), bound = clickBound(top, kFs);
    CHECK(allFinite(x.data(), x.size()));
    CHECK_MSG(click < bound, "the click measure %.4f, the bound %.4f (top %.0f hz)", click, bound, top);
    CHECK_MSG(rise <= 1e-6, "the energy rose %.3g from one block to the next", rise);
    printf("    one string -> three 20 cents wide over a ringing c4 from 1 s: the click measure %.4f, the bound %.4f; the energy's largest rise %.2g; the force's rms "
           "0.9 .. 1 s %.3f, 1.4 .. 1.5 s %.3f, 3.9 .. 4 s %.3f\n",
           click, bound, rise, rms(x.data() + (size_t)(0.9 * kFs), 4410), rms(x.data() + (size_t)(1.4 * kFs), 4410),
           rms(x.data() + (size_t)(3.9 * kFs), 4410));
}

// the tine side: rhodes at e4 couples nothing (the generator 0: the
// strings' list is empty); as the preset stands (its tonebar at its own
// detune, 1.5 cents) the tonebar beats with the tine at the free rate f1
// (2^(detune / 1200) - 1) within 1 %: the beat is the two strings'
// frequency difference, the tine's from the tine alone (strings 1) and the
// tonebar's from the pair less the tine alone, each the peak of the
// pickup's fundamental over 0.2 .. 8 s. the beat reader's trough spacing
// printed beside (a 1.5 s mean on 12 s: it reads the tine dying under the
// tonebar, about 5 % over the rate, the build before the coupling alike);
// the tonebar's late decay (10 .. 16 s, the tine 60 db gone) against
// kAftersound x the tine's t60, and the tine alone, printed
UP_TEST(unison_tine_side_uncoupled) {
    const Shape& rh = presetByName("rhodes")->shape();
    Voice v;
    v.setShape(rh);
    v.noteOn(64.0f, 1.0f);
    float scratch[kBlock];
    v.render(scratch, scratch, kBlock);
    CHECK_MSG(v.strings().coupled() == 0, "rhodes couples %d slots", v.strings().coupled());
    const float detune = rh[Channel::detune];
    const Note n = noteOf(rh, 64.0f, rh[Channel::strings], detune, 12.0f), one = noteOf(rh, 64.0f, 1.0f, detune, 12.0f);
    const double want = n.hz * (std::exp2(detune / 1200.0) - 1.0);
    std::vector<float> bar(n.x.size());
    for (size_t i = 0; i < bar.size(); ++i) bar[i] = n.x[i] - one.x[i];
    const double ft = pitchOver(one.x, 0.2, 8.0, n.hz), fb = pitchOver(bar, 0.2, 8.0, n.hz * std::exp2(detune / 1200.0)), rate = fb - ft;
    CHECK_MSG(std::fabs(rate / want - 1.0) < 0.01, "the tonebar %.5f hz, the tine %.5f: %.5f apart against the free %.5f", fb, ft, rate, want);
    const Beat b = beatOf(n.e, 0.2, 1.5);
    const Note whole = noteOf(rh, 64.0f, rh[Channel::strings], detune, 16.0f), alone = noteOf(rh, 64.0f, 1.0f, detune, 6.0f);
    const double late = slopeOf(whole.e, 10.0, 16.0), tine = slopeOf(alone.e, 0.5, 6.0);
    printf("    rhodes at e4: %d slots coupled; at its %.3f cents the tonebar %.5f hz and the tine %.5f: they beat %.5f hz against the free %.5f "
           "(%+.3f %%); the beat reader's troughs %.4f hz (%+.2f %%, %d troughs); the tonebar's late slope %.3f db/s against %.3f (the tine's "
           "over kAftersound), the tine alone %.3f against %.3f\n",
           v.strings().coupled(), detune, fb, ft, rate, want, 100.0 * (rate / want - 1.0), b.rate, 100.0 * (b.rate / want - 1.0), b.troughs, late,
           -kDbPerNeper * n.sigN / kAftersound, tine, -kDbPerNeper * n.sigN);
}
