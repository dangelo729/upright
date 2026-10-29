// upright — tests/test_law.cpp  (plan §3; the tests' "the string law" bullet, with the rank's own powers)
//
// the string's ratios n sqrt(1 + B n²) exact; B at grand's c4 the formula's at
// the rank's own L, core and tension, and where measured grands sit; B
// against L on the scale knob at c4 (plain wire) and a0 (wound), measured
// against the rank's own powers -6 and -2; the bass break where the plain
// wire passes kDmax; the 8th partial's gain at strike 0; the decays positive
// and falling; the tine's six ratios against the cantilever table; the
// reed's overtones; the generator's path ordered; the grid in tenths over
// the knobs the law reads at a0, c4 and c7 finite, ordered, decaying, inside
// -1 .. 1 and off over the ceiling, and the five it does not read leaving the
// table alone; the damper's rate; the strings' cap.
#include <cmath>
#include <cstring>
#include <initializer_list>
#include <string>

#include "core/up_channels.h"
#include "core/up_knobs.h"
#include "core/up_law.h"
#include "core/up_rank.h"
#include "up_fit.h"
#include "up_test.h"

using namespace up;

namespace {

const Knobs kGrand = {{0.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f}};

Knobs with(Knobs k, Knob which, float x) {
    k.v[(int)which] = x;
    return k;
}

PartialTable tableAt(const Knobs& k, float midi, RankState* rOut = nullptr) {
    const Shape s = pianoOf(k);
    const RankState r = rankAt(s, midi);
    if (rOut) *rOut = r;
    return partialsOf(s, r);
}

// the exponent of B against L between two points
double powerOf(const RankState& a, const RankState& b) { return std::log((double)b.B / a.B) / std::log((double)b.L / a.L); }

}  // namespace

// ratio_n = n sqrt(1 + B n²) for every slot, to the bit against the piece and
// within 1e-6 of the formula in double
UP_TEST(law_string_ratios) {
    RankState r;
    const PartialTable p = tableAt(kGrand, 60.0f, &r);
    bool bits = true;
    double worst = 0.0;
    for (int k = 0; k < kMaxModes; ++k) {
        const int n = k + 1;
        bits = bits && p.ratio[k] == stringRatio(n, r.B);
        worst = std::fmax(worst, std::fabs(p.ratio[k] / (n * std::sqrt(1.0 + (double)r.B * n * n)) - 1.0));
    }
    CHECK(bits);
    CHECK_MSG(worst < 1e-6, "worst %.3g against the formula", worst);
    printf("    %d slots at grand's c4: the piece's bits, %.2g at worst against the formula in double\n", kMaxModes, worst);
}

// B at grand's c4: the formula pi³ E d⁴ / (64 T L²) at the rank's own L, core
// and tension within 1e-5, and between 1e-4 and 1e-3 (measured grands near 3
// .. 4 x 10^-4)
UP_TEST(law_b_at_grand_c4) {
    const Shape s = pianoOf(kGrand);
    const RankState r = rankAt(s, 60.0f);
    const double pi = 3.14159265358979;
    const double d = r.dCore, want = pi * pi * pi * 2.0e11 * d * d * d * d / (64.0 * r.tension * (double)r.L * r.L);
    CHECK_MSG(std::fabs(r.B / want - 1.0) < 1e-5, "B %.6g against the formula's %.6g", r.B, want);
    CHECK_MSG(r.B > 1e-4f && r.B < 1e-3f, "B %.4g", r.B);
    CHECK(!r.wrapped && r.tension == s[Channel::tension]);
    printf("    grand's c4: L %.4f m, d %.4f mm, T %.0f N, B %.4g (the formula %.4g)\n", r.L, r.dCore * 1e3, r.tension, r.B, want);
}

// the exponent of B against L, measured on the scale knob: at c4 over the
// part of the knob where c4's wire stays plain (end to end where it does)
// and at a0 (wound) end to end, against the rank's own -6 and -(2 + 8
// coreGrow) within 0.05 (the core grows as mu^coreGrow, mu as 1 / L²); c4's
// end-to-end figure printed beside (the plan's L⁴ is not the fixed-tension
// rank's law)
UP_TEST(law_b_against_l) {
    auto at = [](float s, float midi) { return rankAt(pianoOf(with(kGrand, Knob::scale, s)), midi); };
    // c4: from scale +1 down to the last tenth where c4 is still plain
    float lowPlain = 1.0f;
    for (int i = 20; i >= 0; --i) {
        const float s = (float)(i - 10) / 10.0f;
        if (at(s, 60.0f).wrapped) break;
        lowPlain = s;
    }
    const double plain = powerOf(at(lowPlain, 60.0f), at(1.0f, 60.0f));
    const double c4End = powerOf(at(-1.0f, 60.0f), at(1.0f, 60.0f));
    const double a0 = powerOf(at(-1.0f, 21.0f), at(1.0f, 21.0f));
    CHECK_MSG(lowPlain < 0.9f, "c4 plain only at scale %.1f", lowPlain);
    CHECK_MSG(std::fabs(plain + 6.0) < 0.05, "c4's plain wire: B as L^%.3f", plain);
    const double wound = -(2.0 + 8.0 * lawConstants().coreGrow);
    CHECK_MSG(at(-1.0f, 21.0f).wrapped && at(1.0f, 21.0f).wrapped && std::fabs(a0 - wound) < 0.05, "a0's wound string: B as L^%.3f (want %.3f)", a0,
              wound);
    printf("    c4, plain from scale %+.1f to +1: B as L^%.3f; c4 end to end: L^%.3f; a0 (wound) end to end: L^%.3f (the rank's %.3f at coreGrow "
           "%.2f)\n",
           lowPlain, plain, c4End, a0, wound, lawConstants().coreGrow);
}

// the bass break: the lowest plain-wire note, where the wire would pass
// kDmax; every note under it wrapped (its core core x (mu / muBreak)^coreGrow)
// and every note over it plain; printed at scale -1, 0, +1 with a0's core
// and B
UP_TEST(law_bass_break) {
    const LawConstants& c = lawConstants();
    for (float s : {-1.0f, 0.0f, 1.0f}) {
        const Shape sh = pianoOf(with(kGrand, Knob::scale, s));
        int brk = -1;
        bool clean = true;
        for (int m = 21; m <= 108; ++m) {
            const RankState r = rankAt(sh, (float)m);
            const float muBreak = 0.25f * kPi * kRho * c.dMax * c.dMax;
            clean = clean && (r.wrapped == (r.d > c.dMax)) && (r.wrapped ? r.dCore == c.core * std::pow(r.mu / muBreak, c.coreGrow) : r.dCore == r.d);
            if (brk < 0 && !r.wrapped) brk = m;
            if (brk >= 0) clean = clean && !r.wrapped;
        }
        CHECK_MSG(clean, "scale %+.0f: a wrapped note over the break, or a core not the rule's", s);
        if (s == 0.0f) CHECK_MSG(brk > 21 && brk < 60, "grand's break at %d", brk);
        const RankState a0 = rankAt(sh, 21.0f);
        printf("    scale %+.0f: the bass break at %s (midi %d); a0's core %.3f mm, B %.3g\n", s, brk >= 0 ? noteName(brk).c_str() : "none", brk,
               a0.dCore * 1e3, a0.B);
    }
}

// the 8th partial's gain at strike 0 (an eighth at 0.12): under -15 db
// (sin(8 pi 0.12) is -18 db; -30 db belongs to 0.125 exactly)
UP_TEST(law_strike_gains) {
    const PartialTable p = tableAt(kGrand, 60.0f);
    const double g8 = 20.0 * std::log10(std::fabs(p.gain[7]));
    CHECK_MSG(g8 < -15.0, "the 8th at %.1f db", g8);
    bool sines = true;
    for (int k = 0; k < kMaxModes; ++k) sines = sines && (!partialOn(p, k) || p.gain[k] == strikeGain(k + 1, 0.12f));
    CHECK(sines);
    const PartialTable mid = tableAt(with(kGrand, Knob::strike, 1.0f), 60.0f);
    CHECK_MSG(std::fabs(mid.gain[1]) < 1e-6f && std::fabs(mid.gain[3]) < 1e-6f, "the middle: evens %g %g", mid.gain[1], mid.gain[3]);
    printf("    the 8th partial at strike 0: %.1f db; at the middle the evens %.1g, %.1g\n", g8, mid.gain[1], mid.gain[3]);
}

// the decays: positive, finite and falling with n at a0, c4 and c7 on the
// grand; at the note (sigma_n = 0.45 / t60Scale + 0.45 (f_n / 800)²: the
// floor scales with the rank, the wire's loss does not) the fundamental's
// t60 at c2 37.7 s, c4 13.9 s, c6 3.23 s, c7 1.26 s, c8 0.413 s, and c4's
// 8th partial 1.92 s and 16th 0.498 s, each within 1 %; key 127's printed
UP_TEST(law_decays) {
    for (float m : {21.0f, 60.0f, 96.0f}) {
        RankState r;
        const PartialTable p = tableAt(kGrand, m, &r);
        bool ok = true;
        for (int k = 0; k < kMaxModes; ++k) ok = ok && p.t60[k] > 0.0f && std::isfinite(p.t60[k]) && (k == 0 || p.t60[k] < p.t60[k - 1]);
        CHECK_MSG(ok, "midi %.0f: a t60 not positive or not falling", m);
        printf("    %s: the first partial's t60 %.2f s at the note (law %.2f x %.3f), the top slot's %.2f s\n", noteName(m).c_str(),
               p.t60[0] * r.t60Scale, p.t60[0], r.t60Scale, p.t60[kMaxModes - 1] * r.t60Scale);
    }
    struct Pin { float midi; int slot; double want; };
    const Pin pins[7] = {{36.0f, 0, 37.7}, {60.0f, 0, 13.9}, {84.0f, 0, 3.23}, {96.0f, 0, 1.26}, {108.0f, 0, 0.413}, {60.0f, 7, 1.92},
                         {60.0f, 15, 0.498}};
    printf("    at the note:");
    for (const Pin& p : pins) {
        if (p.slot >= kMaxModes) continue;
        RankState r;
        const PartialTable t = tableAt(kGrand, p.midi, &r);
        const double t60 = t.t60[p.slot] * r.t60Scale;
        CHECK_MSG(std::fabs(t60 / p.want - 1.0) < 0.01, "%s partial %d: %.4f s against %.4g", noteName(p.midi).c_str(), p.slot + 1, t60, p.want);
        printf(" %s %d: %.3f s;", noteName(p.midi).c_str(), p.slot + 1, t60);
    }
    RankState r127;
    const PartialTable k127 = tableAt(kGrand, 127.0f, &r127);
    printf(" key 127's fundamental %.4f s\n", k127.t60[0] * r127.t60Scale);
}

// the tine (generator 0): its six ratios against the cantilever table
// (beta_n L, extras/reference/cantilever.txt), the overtones (n >= 2) times
// kTineLoad, within 0.1 %; the 2nd against the measured rhodes tine's 7.4
// (tine_rhodes.txt) within 1 %; the slots over six off and still ordered;
// gains the beam's shape at the strike share over 2, outs ±1 alternating
UP_TEST(law_tine) {
    RefTable t;
    std::string err;
    const bool read = readTable(std::string(UP_REF_DIR) + "/cantilever.txt", t, err);
    CHECK_MSG(read, "%s", err.c_str());
    double beta[7] = {};
    int n = 0;
    for (const RefRow& r : t.rows) {
        const int c = t.column(r.block, "betaL");
        if (c > 0 && r.x >= 1.0 && r.x <= 6.0) { beta[(int)r.x] = t.at(r, c); ++n; }
    }
    CHECK(n == 6);
    RankState r;
    const PartialTable p = tableAt(knobAnchors()[1].k, 60.0f, &r);
    double worst = 0.0;
    for (int k = 1; k <= 6 && n == 6; ++k) {
        const double want = (beta[k] / beta[1]) * (beta[k] / beta[1]) * (k >= 2 ? kTineLoad : 1.0);
        worst = std::fmax(worst, std::fabs(p.ratio[k - 1] / want - 1.0));
        printf("    tine %d: %.4f, the table's %.4f (the bare beam's %.4f)\n", k, p.ratio[k - 1], want, (beta[k] / beta[1]) * (beta[k] / beta[1]));
    }
    CHECK_MSG(worst < 1e-3, "the tine's ratios %.3g off the cantilever table x kTineLoad", worst);
    RefTable rt;
    CHECK(readTable(std::string(UP_REF_DIR) + "/tine_rhodes.txt", rt, err));
    double measured = 0.0;
    for (const TinePoint& tp : tinePoints(rt))
        if (tp.partial == 2) measured = tp.ratio;
    CHECK_MSG(std::fabs(p.ratio[1] / p.ratio[0] / measured - 1.0) < 0.01, "the 2nd ratio %.4f against the measured %.2f", p.ratio[1] / p.ratio[0], measured);
    printf("    the 2nd ratio %.4f against the measured rhodes tine's %.2f (%+.1f cents)\n", p.ratio[1] / p.ratio[0], measured,
           1200.0 * std::log2(p.ratio[1] / p.ratio[0] / measured));
    bool off = true, ordered = true, shapes = true;
    for (int k = 0; k < kMaxModes; ++k) {
        if (k >= kTineModes) off = off && !partialOn(p, k);
        if (k > 0) ordered = ordered && p.ratio[k] > p.ratio[k - 1] && std::isfinite(p.ratio[k]);
        if (k < kTineModes && (double)p.ratio[k] * r.f0 < kCeiling * kDefaultFs)
            shapes = shapes && p.gain[k] == 0.5f * beamShape(k + 1, r.strikeShare) && p.out[k] == ((k & 1) ? -1.0f : 1.0f);
    }
    CHECK(off && ordered && shapes);
    CHECK(std::fabs(beamShape(1, 1.0f) - 2.0f) < 1e-5f && std::fabs(beamShape(2, 1.0f) + 2.0f) < 1e-5f && std::fabs(beamShape(40, 1.0f) + 2.0f) < 1e-4f);
}

// the reed (generator +1): the tine's ratios, its fundamental's gain, the
// overtones at kReedOvertone of the beam's, the decays kReedT60 of the tine's
UP_TEST(law_reed) {
    const Knobs tine = with(kGrand, Knob::generator, 0.0f), reed = with(kGrand, Knob::generator, 1.0f);
    const PartialTable t = tableAt(tine, 48.0f), r = tableAt(reed, 48.0f);
    bool ok = r.gain[0] == t.gain[0];
    for (int k = 0; k < kMaxModes; ++k) {
        ok = ok && r.ratio[k] == t.ratio[k] && std::fabs(r.t60[k] / t.t60[k] - kReedT60) < 1e-5f;
        if (k > 0 && k < kTineModes && t.gain[k] != 0.0f) ok = ok && std::fabs(r.gain[k] / t.gain[k] - kReedOvertone) < 1e-5f;
    }
    CHECK(ok);
    printf("    the reed at c3: the 2nd partial's gain %.4f against the tine's %.4f\n", r.gain[1], t.gain[1]);
}

// the generator's path, at every tenth at c4: ratios finite and rising,
// t60s positive, gains inside -1 .. 1; at -0.5 the first six slots' ratios
// the geometric mean of the string's and the tine's; the string's slots
// over six faded by -g, off from 0
UP_TEST(law_generator_path) {
    int bad = 0;
    for (int i = 0; i <= 20; ++i) {
        const PartialTable p = tableAt(with(kGrand, Knob::generator, (float)(i - 10) / 10.0f), 60.0f);
        for (int k = 0; k < kMaxModes; ++k) {
            const bool ok = std::isfinite(p.ratio[k]) && (k == 0 || p.ratio[k] > p.ratio[k - 1]) && p.t60[k] > 0.0f && p.gain[k] >= -1.0f &&
                            p.gain[k] <= 1.0f;
            bad += !ok;
        }
    }
    CHECK_MSG(bad == 0, "%d slots off the path's order", bad);
    const PartialTable s = tableAt(kGrand, 60.0f), t = tableAt(with(kGrand, Knob::generator, 0.0f), 60.0f);
    const PartialTable h = tableAt(with(kGrand, Knob::generator, -0.5f), 60.0f);
    double worst = 0.0;
    for (int k = 0; k < kMaxModes; ++k) worst = std::fmax(worst, std::fabs(h.ratio[k] / std::sqrt((double)s.ratio[k] * t.ratio[k]) - 1.0));
    CHECK_MSG(worst < 1e-5, "the middle %.3g off the geometric mean", worst);
    const int up = kMaxModes > 10 ? 10 : kMaxModes - 1;  // a string slot over the tine's six
    CHECK(h.gain[up] == s.gain[up] * 0.5f && t.gain[up] == 0.0f);
    printf("    every tenth ordered; at -0.5: the 2nd partial %.3f (the string's %.3f, the tine's %.3f), %d of %d partials on\n", h.ratio[1],
           s.ratio[1], t.ratio[1], [&] { int n = 0; for (int k = 0; k < kMaxModes; ++k) n += partialOn(h, k); return n; }(), kMaxModes);
}

// the string's outs are n (-1)^(n+1) (the slope at the bridge read with the
// tip's sign) at grand's c4; along the generator from -1 to 0 in steps of
// 0.05 at a0, c4 and c7 each of the first six slots' gain and out keeps the
// sign it has at -1 wherever the slot is on (under the ceiling): nothing
// crosses zero on the way from the string to the tine
UP_TEST(law_outs_keep_their_signs) {
    const PartialTable g = tableAt(kGrand, 60.0f);
    bool outs = true;
    for (int k = 0; k < kMaxModes; ++k)
        if (partialOn(g, k)) outs = outs && g.out[k] == bridgeOut(k + 1) && bridgeOut(k + 1) == (float)(k + 1) * ((k & 1) ? -1.0f : 1.0f);
    CHECK(outs);
    int read = 0, flips = 0;
    for (float m : {21.0f, 60.0f, 96.0f}) {
        const PartialTable a = tableAt(kGrand, m);
        for (int i = 0; i <= 20; ++i) {
            const PartialTable p = tableAt(with(kGrand, Knob::generator, -1.0f + 0.05f * (float)i), m);
            for (int k = 0; k < kTineModes && k < kMaxModes; ++k) {
                if (!partialOn(p, k) || !partialOn(a, k)) continue;
                ++read;
                const bool same = (p.gain[k] > 0.0f) == (a.gain[k] > 0.0f) && p.gain[k] != 0.0f && (p.out[k] > 0.0f) == (a.out[k] > 0.0f) && p.out[k] != 0.0f;
                if (!same && flips < 5) CHECK_MSG(false, "%s, generator %+.2f, slot %d: gain %+.4f out %+.4f against %+.4f %+.4f at -1", noteName(m).c_str(),
                                                  -1.0 + 0.05 * i, k, p.gain[k], p.out[k], a.gain[k], a.out[k]);
                flips += !same;
            }
        }
    }
    CHECK(flips == 0 && read > 0);
    printf("    the outs n (-1)^(n+1); along the generator -1 .. 0 by 0.05 at a0, c4, c7: %d slot readings, %d crossing zero\n", read, flips);
}

namespace {

struct Grid {
    long cells = 0, disorder = 0, t60s = 0, gains = 0, ceiling = 0;
    void check(const PartialTable& p, float f0) {
        ++cells;
        bool ord = true, dec = true, gn = true, cl = true;
        for (int k = 0; k < kMaxModes; ++k) {
            ord = ord && std::isfinite(p.ratio[k]) && (k == 0 || p.ratio[k] > p.ratio[k - 1]);
            dec = dec && p.t60[k] > 0.0f && std::isfinite(p.t60[k]);
            gn = gn && p.gain[k] >= -1.0f && p.gain[k] <= 1.0f;
            cl = cl && ((double)p.ratio[k] * f0 <= kCeiling * kDefaultFs || !partialOn(p, k));
        }
        disorder += !ord;
        t60s += !dec;
        gains += !gn;
        ceiling += !cl;
    }
};

}  // namespace

// the grid in tenths over the knobs the law reads (scale, strike, generator)
// at a0, c4 and c7: every ratio finite and rising, every t60 positive, every
// gain inside -1 .. 1, every partial over the ceiling off. the other five
// (hammer, unison, body, pickup, damper) leave the table alone: at each
// anchor each of them over its tenths gives the anchor's table to the bit
UP_TEST(law_grid_in_tenths) {
    Grid g;
    auto tenth = [](int i) { return (float)(i - 10) / 10.0f; };
    for (int s = 0; s <= 20; ++s)
        for (int st = 0; st <= 20; ++st)
            for (int ge = 0; ge <= 20; ++ge) {
                Knobs k = kGrand;
                k.v[(int)Knob::scale] = tenth(s);
                k.v[(int)Knob::strike] = tenth(st);
                k.v[(int)Knob::generator] = tenth(ge);
                const Shape sh = pianoOf(k);
                for (float m : {21.0f, 60.0f, 96.0f}) {
                    const RankState r = rankAt(sh, m);
                    g.check(partialsOf(sh, r), r.f0);
                }
            }
    CHECK_MSG(g.disorder == 0 && g.t60s == 0 && g.gains == 0 && g.ceiling == 0, "%ld of %ld out of order, %ld t60s, %ld gains, %ld over the ceiling",
              g.disorder, g.cells, g.t60s, g.gains, g.ceiling);
    const Knob others[5] = {Knob::hammer, Knob::unison, Knob::body, Knob::pickup, Knob::damper};
    int moved = 0;
    for (int a = 0; a < kAnchorCount; ++a) {
        const PartialTable base = tableAt(knobAnchors()[a].k, 60.0f);
        for (Knob o : others)
            for (int i = 0; i <= 20; ++i) {
                const PartialTable p = tableAt(with(knobAnchors()[a].k, o, tenth(i)), 60.0f);
                moved += std::memcmp(&p, &base, sizeof p) != 0;
            }
    }
    CHECK_MSG(moved == 0, "%d tables moved by a knob the law does not read", moved);
    printf("    %ld tables (scale x strike x generator in tenths, at a0, c4, c7): %ld out of order, %ld t60s, %ld gains, %ld over the ceiling; "
           "the other five knobs moved %d tables\n",
           g.cells, g.disorder, g.t60s, g.gains, g.ceiling, moved);
}

// the damper's rate: rising with frequency, zero above damperTop and at
// pedal 1, half at pedal 0.5
UP_TEST(law_damp_rate) {
    const LawConstants& c = lawConstants();
    Shape s = pianoOf(kGrand);
    s[Channel::midi] = 60.0f;
    float last = 0.0f;
    bool rising = true;
    for (float hz = 50.0f; hz < 20000.0f; hz *= 1.5f) {
        const float d = dampRateOf(hz, s, 0.0f, c);
        rising = rising && d > last;
        last = d;
    }
    CHECK(rising);
    CHECK(std::fabs(dampRateOf(2000.0f, s, 0.0f, c) - 2.0f * s[Channel::dampRate]) < 1e-3f);
    CHECK(dampRateOf(1000.0f, s, 1.0f, c) == 0.0f && std::fabs(dampRateOf(1000.0f, s, 0.5f, c) / dampRateOf(1000.0f, s, 0.0f, c) - 0.5f) < 1e-6f);
    s[Channel::midi] = s[Channel::damperTop] + 1.0f;
    CHECK(dampRateOf(1000.0f, s, 0.0f, c) == 0.0f);
    s[Channel::midi] = s[Channel::damperTop];
    CHECK(dampRateOf(1000.0f, s, 0.0f, c) > 0.0f);
    CHECK(!rankAt(s, s[Channel::damperTop] + 1.0f).damped && rankAt(s, s[Channel::damperTop]).damped);
}

// the strings' cap: 3 at the generator -0.5 and under, falling to 2 at 0, 2
// over it; continuous
UP_TEST(law_strings_cap) {
    Shape s = pianoOf(kGrand);
    float last = 3.0f;
    bool ok = true;
    for (int i = 0; i <= 200; ++i) {
        s[Channel::generator] = -1.0f + 0.01f * i;
        const float n = stringsOf(s);
        ok = ok && std::fabs(n - last) < 0.021f && n <= 3.0f && n >= 2.0f;
        last = n;
    }
    CHECK(ok);
    s[Channel::generator] = -0.5f;
    CHECK(stringsOf(s) == 3.0f);
    s[Channel::generator] = 0.0f;
    CHECK(stringsOf(s) == 2.0f);
    s[Channel::strings] = 1.0f;
    CHECK(stringsOf(s) == 1.0f);
}
