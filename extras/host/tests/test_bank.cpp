// upright — tests/test_bank.cpp
//
// one partial driven by one sample rings at its frequency (zero crossings
// over the first second: within 0.1 cent at 440 hz and 15 khz, 0.5 at 27.5
// hz) and decays at its t60 (the envelope's slope, within 1 %); one sample's
// drive gives the amplitude the drive scale says; the tip reads the out held
// to ±1 where the bridge reads the out; a partial off renders
// exactly zero; kContactRate substeps land where one sample's turn lands;
// the sums without a turn are the step's sums bit for bit where the substep
// is the sample; the readback's velocity is the displacement's rate; a decay
// slower than kMaxT60 is held there and never grows; a partial that moves
// grows in neither output (up, w |z| kept; down, |z| kept).
#include <cmath>
#include <cstring>
#include <initializer_list>
#include <vector>

#include "core/up_bank.h"
#include "up_analysis.h"
#include "up_test.h"

using namespace up;

namespace {

constexpr float kFs = 44100.0f;

// one partial at hz with t60, driven by `drive` at sample 0 (the force 1), n samples of its output (out 1)
std::vector<float> ringOne(float hz, float t60, int n, float drive = 1.0f) {
    Bank<4> bank;
    bank.init(kFs);
    std::vector<float> out((size_t)n);
    for (int i = 0; i < n; ++i) {
        if (i % kBlock == 0) {
            bank.setPartial(0, true, hz, 6.907755f / t60, drive, 0.0f, 1.0f);
            bank.beginBlock();
        }
        if (i == 0) bank.inject(1.0f);
        float s, x, v;
        bank.step<true, false>(s, x, v);
        out[(size_t)i] = s;
        if (i % kBlock == kBlock - 1) bank.endBlock();
    }
    return out;
}

// hz from the positive-going zero crossings over the first `sec` seconds, each placed between its samples
double crossingHz(const std::vector<float>& x, double sec) {
    const size_t n = std::min(x.size(), (size_t)(sec * kFs));
    double first = -1.0, last = -1.0;
    int count = 0;
    for (size_t i = 1; i < n; ++i)
        if (x[i - 1] < 0.0f && x[i] >= 0.0f) {
            const double t = (double)(i - 1) + (double)x[i - 1] / ((double)x[i - 1] - (double)x[i]);
            if (first < 0.0) first = t;
            last = t;
            ++count;
        }
    return count > 1 ? (double)(count - 1) * kFs / (last - first) : 0.0;
}

}  // namespace

// one partial at 440 hz, 27.5 hz and 15 khz, t60 1 s, driven by one sample:
// its frequency from the zero crossings over the first second within 0.1
// cent (0.5 at 27.5 hz, where 27 crossings measure it); its decay from the
// envelope's slope within 1 % of 1 s; its first sample the drive scale x
// the turn (re after one clockwise turn of i drive: drive r sin w)
UP_TEST(bank_one_partial_rings_true) {
    const float hzs[3] = {440.0f, 27.5f, 15000.0f};
    const double tol[3] = {0.1, 0.5, 0.1};
    for (int i = 0; i < 3; ++i) {
        const std::vector<float> x = ringOne(hzs[i], 1.0f, (int)(2.0f * kFs));
        const double c = 1200.0 * std::log2(crossingHz(x, 1.0) / hzs[i]);
        const BandAnalyser b(x.data(), x.size(), kFs);
        const double hw = std::max(10.0, 0.3 * hzs[i]);
        const double t60 = t60Of(b.envelope(hzs[i] - hw, hzs[i] + hw)).t60;
        CHECK_MSG(std::fabs(c) < tol[i], "%g hz: %+.4f cents from the crossings", hzs[i], c);
        CHECK_MSG(std::fabs(t60 - 1.0) < 0.01, "%g hz: t60 %.4f s", hzs[i], t60);
        const double w = kTwoPi * hzs[i] / kFs, r = std::exp(-6.907755 / kFs);
        double worst = 0.0;
        for (size_t n = 0; n < (size_t)kBlock; ++n) worst = std::fmax(worst, std::fabs((double)x[n] - std::pow(r, (double)(n + 1)) * std::sin(w * (double)(n + 1))));
        CHECK_MSG(worst < 1e-4, "%g hz: %.3g from the damped sine", hzs[i], worst);
        printf("    %7.1f hz: %+.5f cents (crossings), t60 %.5f s (envelope), the first block %.2e from the damped sine\n", hzs[i], c, t60, worst);
    }
}

// one sample's drive of 0.37 at a drive scale of 2.5: the amplitude 0.925
// after the turn (less one sample's decay), within 0.1 %; the force times the
// drive scale, at any force
UP_TEST(bank_drive_scale) {
    for (float force : {0.37f, 12.0f}) {
        Bank<4> bank;
        bank.init(kFs);
        bank.setPartial(1, true, 1000.0f, 6.907755f / 0.5f, 2.5f, 0.0f, 1.0f);
        bank.beginBlock();
        bank.inject(force);
        float s, x, v;
        bank.step<true, false>(s, x, v);
        const double want = force * 2.5 * std::exp(-6.907755 / 0.5 / kFs);
        CHECK_MSG(std::fabs(bank.amplitude(1) / want - 1.0) < 1e-3, "force %g: amplitude %.6g against %.6g", force, bank.amplitude(1), want);
    }
}

// the tip's weight is the out held to ±1: a partial under kTineModes at out 3
// reads 3 re on the bridge and re, w im on the tip, every sample of a block;
// the next block at out -2.5 ramps the bridge's weight to -2.5 and the tip's
// to -1, landing there at the block's end
UP_TEST(bank_tip_weight_is_the_out_held) {
    Bank<8> bank;
    bank.init(kFs);
    bank.setPartial(1, true, 880.0f, 2.0f, 1.0f, 0.0f, 3.0f);
    bank.beginBlock();
    bank.inject(1.0f);
    double worst = 0.0;
    for (int i = 0; i < kBlock; ++i) {
        float s, x, v, re, im;
        bank.step<true, true>(s, x, v);
        bank.phasor(1, re, im);
        worst = std::fmax(worst, std::fabs(s - 3.0 * re) + std::fabs(x - re) + std::fabs(v - bank.omega(1) * im) / bank.omega(1));
    }
    bank.setPartial(1, true, 880.0f, 2.0f, 1.0f, 0.0f, -2.5f);
    bank.beginBlock();
    float s = 0.0f, x = 0.0f, v = 0.0f, re = 0.0f, im = 0.0f;
    for (int i = 0; i < kBlock; ++i) bank.step<true, true>(s, x, v);
    bank.phasor(1, re, im);
    CHECK_MSG(worst < 1e-6, "at out 3: the bridge and the tip %.3g off 3 re and re", worst);
    CHECK_MSG(std::fabs(s / re + 2.5) < 1e-4 && std::fabs(x / re + 1.0) < 1e-4, "at out -2.5: the bridge %.5f re, the tip %.5f re", s / re, x / re);
    printf("    out 3: the bridge 3 re, the tip re (within %.1e); out -2.5 a block later: the bridge %.4f re, the tip %.4f re\n", worst, s / re,
           x / re);
}

// a partial off renders exactly zero: never on, and turned off while ringing
// (zero from that block on, and from rest when it comes on again)
UP_TEST(bank_partial_off_is_zero) {
    Bank<8> bank;
    bank.init(kFs);
    bool zero = true, rang = false;
    for (int i = 0; i < 4 * kBlock; ++i) {
        if (i % kBlock == 0) {
            const int blk = i / kBlock;
            bank.setPartial(0, blk < 2, 440.0f, 1.0f, 1.0f, 0.0f, 1.0f);
            bank.setPartial(1, blk == 3, 660.0f, 1.0f, 1.0f, 0.0f, 1.0f);
            bank.setPartial(2, false, 880.0f, 1.0f, 1.0f, 0.0f, 1.0f);
            bank.beginBlock();
        }
        if (i == 0) bank.inject(1.0f);
        float s, x, v;
        bank.step<true, true>(s, x, v);
        if (i < 2 * kBlock) rang = rang || s != 0.0f;
        else zero = zero && s == 0.0f && x == 0.0f && v == 0.0f && bank.energySq() == 0.0f;
        if (i % kBlock == kBlock - 1) bank.endBlock();
    }
    CHECK(rang && zero);
    CHECK(bank.count() == 1 && bank.amplitude(0) == 0.0f);
}

// kContactRate substeps of turnSub land where one sample's turn lands over a
// block, within the float's own rounding: 4e-6 of the amplitude (the
// rounding of kContactRate x 128 turns against 128, about 2e-6 at 8) plus
// the frequency's (each pair's frequency rounded to about 1.2e-7 of itself,
// independently: 2 pi f (block / fs) 2.4e-7 of phase over a block), each
// partial printed; the sums without a turn are the step's sums bit for bit at
// a substep rate of 1 (the substep's pair is the sample's)
UP_TEST(bank_substeps_land_on_the_step) {
    const float hzs[6] = {27.5f, 261.6f, 1000.0f, 4186.0f, 12000.0f, 19000.0f};
    auto set = [&](Bank<8>& b) {
        for (int k = 0; k < 6; ++k) b.setPartial(k, true, hzs[k], 3.0f + k, 1.0f, 0.0f, 0.7f + 0.1f * k);
        b.beginBlock();
    };
    Bank<8> a, b;
    a.init(kFs);
    b.init(kFs);
    set(a);
    set(b);
    a.inject(1.0f);
    b.inject(1.0f);
    double gap[6] = {};
    for (int i = 0; i < kBlock; ++i) {
        float s, x, v;
        a.step<true, true>(s, x, v);
        for (int r = 0; r < kContactRate; ++r) b.turnSub();
        b.sums<true, true>(s, x, v);
        for (int k = 0; k < 6; ++k) {
            float ra, ia, rb, ib;
            a.phasor(k, ra, ia);
            b.phasor(k, rb, ib);
            gap[k] = std::fmax(gap[k], std::sqrt((double)(ra - rb) * (ra - rb) + (double)(ia - ib) * (ia - ib)) / a.amplitude(k));
        }
    }
    printf("    %d substeps against one step over a block, of the amplitude:", kContactRate);
    for (int k = 0; k < 6; ++k) {
        const double bound = 4e-6 + kTwoPi * hzs[k] * kBlock / kFs * 2.4e-7;
        CHECK_MSG(gap[k] < bound, "%g hz: %d substeps %.3g off one step (the bound %.3g)", hzs[k], kContactRate, gap[k], bound);
        printf(" %g hz %.1e%s", hzs[k], gap[k], k == 5 ? "\n" : ",");
    }
    // at a substep rate of 1 the pair is the sample's: the turn, then the sums, the step's bit for bit
    Bank<8> c, d;
    c.init(kFs, 1);
    d.init(kFs, 1);
    set(c);
    set(d);
    c.inject(0.5f);
    d.inject(0.5f);
    bool same = true;
    for (int i = 0; i < 3 * kBlock; ++i) {
        if (i % kBlock == 0 && i > 0) {
            set(c);
            set(d);
        }
        float s1, x1, v1, s2, x2, v2;
        c.step<true, true>(s1, x1, v1);
        d.turnSub();
        d.sums<true, true>(s2, x2, v2);
        same = same && std::memcmp(&s1, &s2, sizeof s1) == 0 && std::memcmp(&x1, &x2, sizeof x1) == 0 && std::memcmp(&v1, &v2, sizeof v1) == 0;
    }
    CHECK(same);
    printf("    at a rate of 1 the sums are the step's to the bit\n");
}

// the readback: y = sum readback re; its velocity sum readback w im is the
// string point's velocity, the phasors' own rate sum readback (w im - sigma
// re) less the decays' term, within 0.2 % of its peak on three partials
UP_TEST(bank_readback_velocity) {
    Bank<8> bank;
    bank.init(kFs);
    const float hz[3] = {220.0f, 660.0f, 1500.0f}, rb[3] = {0.8f, -0.5f, 0.3f}, decay = 2.0f;
    for (int k = 0; k < 3; ++k) bank.setPartial(k, true, hz[k], decay, 1.0f, rb[k], 1.0f);
    bank.beginBlock();
    bank.inject(1.0f);
    double worst = 0.0, peak = 0.0, ysum = 0.0, yread = 0.0;
    for (int i = 0; i < kBlock; ++i) {
        float s, x, xv, yy, vv;
        bank.step<false, false>(s, x, xv);
        bank.readback(yy, vv);
        double exact = 0.0, y = 0.0;
        for (int k = 0; k < 3; ++k) {
            float re, im;
            bank.phasor(k, re, im);
            exact += rb[k] * (kTwoPi * hz[k] * im - decay * re);
            y += rb[k] * re;
        }
        worst = std::fmax(worst, std::fabs(exact - vv));
        peak = std::fmax(peak, std::fabs(exact));
        ysum = y;
        yread = yy;
    }
    CHECK_MSG(worst < 2e-3 * peak, "the velocity readback %.3g off the phasors' rate (peak %.3g)", worst, peak);
    CHECK(std::fabs(bank.readback() - yread) < 1e-9f && std::fabs(yread - ysum) < 1e-6 * std::fabs(ysum) + 1e-9);
    printf("    the velocity readback within %.2e of the phasors' rate (peak %.3g)\n", worst / peak, peak);
}

// a decay asked slower than kMaxT60 (or at or under zero) is kMaxT60's: the
// partial decays, never grows, at 19 khz and 27.5 hz
UP_TEST(bank_never_grows) {
    for (float hz : {27.5f, 19000.0f})
        for (float decay : {0.0f, -5.0f, 1e-6f}) {
            Bank<4> bank;
            bank.init(kFs);
            float first = 0.0f, last = 0.0f;
            for (int i = 0; i < 20 * kBlock; ++i) {
                if (i % kBlock == 0) {
                    bank.setPartial(0, true, hz, decay, 1.0f, 0.0f, 1.0f);
                    bank.beginBlock();
                }
                if (i == 0) bank.inject(1.0f);
                float s, x, v;
                bank.step<true, false>(s, x, v);
                if (i % kBlock == kBlock - 1) {
                    bank.endBlock();
                    if (i == kBlock - 1) first = bank.amplitude(0);
                    last = bank.amplitude(0);
                }
            }
            CHECK_MSG(last < first && last > 0.0f, "%g hz, decay %g: %.9f after %.9f", hz, decay, last, first);
        }
}

// a partial that moves grows in neither output: one partial at 220 hz at
// |z| 1 (im 1 from rest, out 0.8) moved up to 880 hz reads |z| 0.25 and its
// velocity sum's amplitude, out w |z|, unchanged within 1e-5 (relative); a
// block stepped at 880 hz, the velocity sum's largest sample that amplitude
// (within the sampling of its peak, 0.3 %); moved back down to 220 hz, |z|
// kept (0.25 less the block's decay) and the velocity a quarter. another at
// 220 hz moved down to 110 hz keeps |z| 1 and its velocity halves. a decay
// moved alone scales nothing
UP_TEST(bank_moved_partial_grows_in_neither_output) {
    Bank<4> bank;
    bank.init(kFs);
    const float out = 0.8f;
    auto velocity = [&](int k) { return (double)out * bank.omega(k) * bank.amplitude(k); };
    bank.setPartial(0, true, 220.0f, 0.0f, 1.0f, 0.0f, out);
    bank.setPartial(1, true, 220.0f, 0.0f, 1.0f, 0.0f, out);
    bank.beginBlock();
    bank.inject(1.0f);
    const double a0 = bank.amplitude(0), v0 = velocity(0), d0 = velocity(1);
    bank.setPartial(0, true, 880.0f, 0.0f, 1.0f, 0.0f, out);
    bank.setPartial(1, true, 110.0f, 0.0f, 1.0f, 0.0f, out);
    const double a1 = bank.amplitude(0), v1 = velocity(0), b1 = bank.amplitude(1), d1 = velocity(1);
    bank.beginBlock();
    double peak = 0.0;
    for (int i = 0; i < kBlock; ++i) {
        float s, x, v;
        bank.step<true, false, true>(s, x, v);
        float re0, im0, re1, im1;
        bank.phasor(0, re0, im0);
        bank.phasor(1, re1, im1);
        peak = std::fmax(peak, std::fabs((double)s - (double)out * bank.omega(1) * im1));
    }
    const double a2 = bank.amplitude(0), v2 = velocity(0);
    bank.setPartial(0, true, 220.0f, 0.0f, 1.0f, 0.0f, out);
    const double a3 = bank.amplitude(0), v3 = velocity(0);
    bank.beginBlock();
    bank.setPartial(0, true, 220.0f, 5.0f, 1.0f, 0.0f, out);
    const double a4 = bank.amplitude(0);
    CHECK_MSG(a0 == 1.0 && std::fabs(a1 - 0.25) < 1e-6, "up: |z| %.7f at 220 hz, %.7f moved to 880", a0, a1);
    CHECK_MSG(std::fabs(v1 / v0 - 1.0) < 1e-5, "up: the velocity's amplitude %.7g at 220 hz, %.7g moved to 880 (%.2g)", v0, v1, v1 / v0 - 1.0);
    CHECK_MSG(std::fabs(peak / v1 - 1.0) < 3e-3, "a block at 880 hz: the velocity sum peaks %.7g against out w |z| %.7g", peak, v1);
    CHECK_MSG(a3 == a2 && std::fabs(v3 / v2 - 0.25) < 1e-6, "back down: |z| %.7f from %.7f, the velocity %.4f of before", a3, a2, v3 / v2);
    CHECK_MSG(b1 == 1.0 && std::fabs(d1 / d0 - 0.5) < 1e-6, "down 220 -> 110 hz: |z| %.7f, the velocity %.6f of before", b1, d1 / d0);
    CHECK(a4 == a3);
    printf("    up 220 -> 880 hz: |z| %.6f -> %.6f, out w |z| %.6g -> %.6g (%+.1e); a block at 880 hz peaks %.6g; back down to 220 hz: |z| %.6f "
           "kept, out w |z| x %.4f; down 220 -> 110 hz: |z| %.6f kept, out w |z| x %.4f\n",
           a0, a1, v0, v1, v1 / v0 - 1.0, peak, a3, v3 / v2, b1, d1 / d0);
}
