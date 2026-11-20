// upright — tests/test_board.cpp
//
// the plate's frequencies from the formula; doubling the area lowers every
// mode an octave; doubling eta doubles every decay rate; a closed back
// lowers the 60 hz radiation; the modes in frequency order with no
// duplicate; the pairs found once are the full search's at every board, and a
// rebuild's cost; the bridge points inside the plate for every key. then the
// board as a stage: two voices in are the sum of each; a mode rings at its
// table's frequency and t60; a closed back lowers a 60 hz drive's output.
// then the field: its impulse energy the shelf's share at every board; an
// impulse's energy spread over milliseconds; its lines' loss the plate's
// per-pass loss at dc and at kFieldFitHz, its t60 at 1.5 f_top and its late
// decay printed beside the plate's; finite and falling silent after a hard
// input at every board and rate; the board's mean power over three octaves
// above the modes a flat shelf's within 3 db.
#include <chrono>
#include <cmath>
#include <set>
#include <vector>
#include <utility>

#include "core/up_board.h"
#include "up_analysis.h"
#include "core/up_presets.h"
#include "core/up_voice.h"
#include "up_fit.h"
#include "up_play.h"
#include "up_shapes.h"
#include "up_test.h"

using namespace up;

namespace {

// the five boards' names (up_shapes.h)
const char* const kBoardName[5] = {"the spinet's", "fortepiano's", "grand's", "a larger", "the concert grand's"};

// a board at one of the five plates
void plateAt(Board& b, int pl, float fs = kDefaultFs) {
    b.init(fs);
    b.setPlate(kBoardPlates[pl].area, kBoardPlates[pl].loss, kBoardPlates[pl].open);
}

// the board's output for 1 N for one sample at c4's bridge point at a plate, n samples; flat: the modes alone (the
// shelf's mobility 0) plus the force through the two highpasses x kBoardShelf, a flat shelf's
std::vector<float> impulseAt(int pl, int n, bool flat) {
    Board b;
    plateAt(b, pl);
    b.setShelf(flat ? 0.0f : kBoardShelf);
    float w[kBoardModes];
    b.inputWeights(60.0f, w);
    std::vector<float> f((size_t)n, 0.0f), y((size_t)n);
    f[0] = 1.0f;
    const float* fp[1] = {f.data()};
    const float* wp[1] = {w};
    b.render(fp, wp, 1, y.data(), n);
    if (!flat) return y;
    const double a = 1.0 - std::exp(-2.0 * kPi * b.modeInfo(kBoardModes - 1).hz / kDefaultFs);
    double low1 = 0.0, low2 = 0.0;
    for (int i = 0; i < n; ++i) {
        low1 += a * (f[(size_t)i] - low1);
        const double high1 = f[(size_t)i] - low1;
        low2 += a * (high1 - low2);
        y[(size_t)i] += (float)((high1 - low2) * kBoardShelf * kBoardOut);
    }
    return y;
}

// schroeder's backward integration on a band's envelope from fromSec on: each frame's power summed from the end, in
// db against the sum at fromSec; the t60 from its least-squares slope from 5 db under to 25 db under (s; 0 with under
// 3 frames)
double schroederT60(const Envelope& e, double fromSec) {
    std::vector<double> back(e.db.size());
    double sum = 0.0;
    for (size_t i = e.db.size(); i-- > 0;) back[i] = sum += std::pow(10.0, e.db[i] / 10.0);
    const size_t from = (size_t)std::ceil(fromSec / e.frameSec);
    double st = 0, sy = 0, stt = 0, sty = 0;
    int n = 0;
    for (size_t i = from; i < back.size(); ++i) {
        const double d = 10.0 * std::log10(back[i] / back[from] + 1e-300), t = (double)i * e.frameSec;
        if (d > -5.0) continue;
        if (d < -25.0) break;
        st += t;
        sy += d;
        stt += t * t;
        sty += t * d;
        ++n;
    }
    const double slope = n >= 3 ? (n * sty - st * sy) / (n * stt - st * st) : 0.0;
    return slope < 0.0 ? -60.0 / slope : 0.0;
}

}  // namespace

// every mode's frequency against the orthotropic plate's formula in double,
// within 0.01 %; grand's first three modes printed
UP_TEST(board_formula) {
    const BoardTable t = boardOf(1.0f, 0.03f, 0.5f);
    const double h = kBoardH, h3 = h * h * h, dx = kBoardEx * h3 / 12.0, dy = kBoardEy * h3 / 12.0, hh = std::sqrt(dx * dy);
    const double a = std::sqrt(1.0 * kBoardAspect), b = std::sqrt(1.0 / kBoardAspect);
    double worst = 0.0;
    for (int k = 0; k < kBoardModes; ++k) {
        const double ma = t.mode[k].m / a, nb = t.mode[k].n / b;
        const double f = 0.5 * 3.14159265358979 * std::sqrt(1.0 / (kBoardRho * h)) *
                         std::sqrt(dx * std::pow(ma, 4) + 2.0 * hh * ma * ma * nb * nb + dy * std::pow(nb, 4));
        worst = std::fmax(worst, std::fabs(t.mode[k].hz / f - 1.0));
    }
    CHECK_MSG(worst < 1e-4, "%.3g off the formula", worst);
    CHECK_MSG(t.mode[0].hz > 40.0f && t.mode[0].hz < 60.0f && t.mode[2].hz < 120.0f, "the first three %.1f %.1f %.1f hz", t.mode[0].hz,
              t.mode[1].hz, t.mode[2].hz);
    printf("    1 m²: (%d,%d) %.1f, (%d,%d) %.1f, (%d,%d) %.1f hz; %.2g off the formula at worst\n", t.mode[0].m, t.mode[0].n, t.mode[0].hz,
           t.mode[1].m, t.mode[1].n, t.mode[1].hz, t.mode[2].m, t.mode[2].n, t.mode[2].hz, worst);
}

// doubling the area lowers every mode by an octave within 0.1 cent, the same
// (m, n) in each slot
UP_TEST(board_area_octave) {
    const BoardTable a = boardOf(0.8f, 0.03f, 0.5f), b = boardOf(1.6f, 0.03f, 0.5f);
    double worst = 0.0;
    bool same = true;
    for (int k = 0; k < kBoardModes; ++k) {
        worst = std::fmax(worst, std::fabs(1200.0 * std::log2((double)a.mode[k].hz / b.mode[k].hz) - 1200.0));
        same = same && a.mode[k].m == b.mode[k].m && a.mode[k].n == b.mode[k].n;
    }
    CHECK(same);
    CHECK_MSG(worst < 0.1, "%.3f cents off an octave", worst);
}

// doubling eta doubles every decay rate (sigma = pi f eta)
UP_TEST(board_loss) {
    const BoardTable a = boardOf(1.0f, 0.02f, 0.5f), b = boardOf(1.0f, 0.04f, 0.5f);
    double worst = 0.0;
    for (int k = 0; k < kBoardModes; ++k) {
        worst = std::fmax(worst, std::fabs((double)a.mode[k].t60 / b.mode[k].t60 - 2.0));
        worst = std::fmax(worst, std::fabs(6.907755 / a.mode[k].t60 / (3.14159265358979 * a.mode[k].hz * 0.02) - 1.0));
    }
    CHECK_MSG(worst < 1e-4, "%.3g off", worst);
}

// open 0 against 1 lowers the 60 hz radiation weight by 6 db and more
UP_TEST(board_open) {
    const double closed = 10.0 * std::log10(radiationOf(60.0f, 0.0f)), open = 10.0 * std::log10(radiationOf(60.0f, 1.0f));
    CHECK_MSG(open - closed >= 6.0, "the 60 hz weight %.1f db closed, %.1f open", closed, open);
    CHECK(std::fabs(radiationOf(radiationEdge(0.3f), 0.3f) - 0.5f) < 1e-6f);
    printf("    the 60 hz radiation %.1f db closed, %.1f db open (%.1f db); the edge %.0f hz closed, %.0f open\n", closed, open, open - closed,
           radiationEdge(0.0f), radiationEdge(1.0f));
}

// the modes in frequency order, no (m, n) twice, at the spinet's, grand's and the concert grand's boards
UP_TEST(board_order) {
    for (int pl : {0, 2, 4}) {
        const BoardPlate& p = kBoardPlates[pl];
        const BoardTable t = boardOf(p.area, p.loss, p.open);
        std::set<std::pair<int, int>> seen;
        bool ordered = true;
        for (int i = 0; i < kBoardModes; ++i) {
            ordered = ordered && (i == 0 || t.mode[i].hz >= t.mode[i - 1].hz);
            seen.insert({t.mode[i].m, t.mode[i].n});
        }
        CHECK(ordered && (int)seen.size() == kBoardModes);
        printf("    %s board: %.1f, %.1f, %.1f hz\n", kBoardName[pl], t.mode[0].hz, t.mode[1].hz, t.mode[2].hz);
    }
}

// the plate's (m, n) pairs are found once (the full search's at 1 m², on the
// first call) and kept: at the five boards from the spinet's to the concert
// grand's boardOf's table is boardSearch's, the same m and n and the same hz, t60 and radiation to
// the bit (a check). the host time of a rebuild printed against a voice's
// block (grand at c4, 64 partials, three strings): setPlate with every mode
// moved (the table, the output weights, the bank's setPartial), and that
// with the board's next block, whose beginBlock turns every moved mode,
// less a block where nothing moved
UP_TEST(board_pairs_found_once) {
    bool same = true;
    for (int pl = 0; pl < 5; ++pl) {
        const float area = kBoardPlates[pl].area, loss = kBoardPlates[pl].loss, open = kBoardPlates[pl].open;
        const BoardTable a = boardOf(area, loss, open), b = boardSearch(area, loss, open);
        bool here = a.a == b.a && a.b == b.b;
        for (int i = 0; i < kBoardModes; ++i)
            here = here && a.mode[i].m == b.mode[i].m && a.mode[i].n == b.mode[i].n && a.mode[i].hz == b.mode[i].hz && a.mode[i].t60 == b.mode[i].t60 &&
                   a.mode[i].radiation == b.mode[i].radiation;
        CHECK_MSG(here, "%s board (%.4f m²): the pairs found once differ from the search", kBoardName[pl], area);
        same = same && here;
    }
    // a rebuild's host time: the plate between two areas, every mode moved
    using clock = std::chrono::steady_clock;
    volatile float sink = 0.0f;
    Board board;
    board.init(kDefaultFs);
    float w[kBoardModes], f[kBlock] = {}, out[kBlock];
    board.inputWeights(60.0f, w);
    const float* fp[1] = {f};
    const float* wp[1] = {w};
    const int n = 4000;
    auto t0 = clock::now();
    for (int i = 0; i < n; ++i) board.setPlate(i % 2 ? 0.8f : 1.2f, 0.03f, 0.5f);
    const double plate = std::chrono::duration<double, std::micro>(clock::now() - t0).count() / n;
    t0 = clock::now();
    for (int i = 0; i < n; ++i) {
        board.setPlate(i % 2 ? 0.8f : 1.2f, 0.03f, 0.5f);
        board.render(fp, wp, 1, out, kBlock);
        sink = sink + out[0];
    }
    const double moved = std::chrono::duration<double, std::micro>(clock::now() - t0).count() / n;
    t0 = clock::now();
    for (int i = 0; i < n; ++i) {
        board.render(fp, wp, 1, out, kBlock);
        sink = sink + out[0];
    }
    const double still = std::chrono::duration<double, std::micro>(clock::now() - t0).count() / n;
    Voice v(1);
    v.setShape(grand());
    float br[kBlock], pk[kBlock];
    v.noteOn(60.0f, 1.0f);
    for (int i = 0; i < 50; ++i) v.render(br, pk, kBlock);
    t0 = clock::now();
    for (int i = 0; i < n; ++i) {
        v.render(br, pk, kBlock);
        sink = sink + br[0];
    }
    const double voice = std::chrono::duration<double, std::micro>(clock::now() - t0).count() / n;
    t0 = clock::now();
    for (int i = 0; i < 200; ++i) {
        const BoardTable t = boardSearch(0.8f + 0.002f * i, 0.03f, 0.5f);
        sink = sink + t.mode[kBoardModes - 1].hz;
    }
    const double search = std::chrono::duration<double, std::micro>(clock::now() - t0).count() / 200;
    printf("    the five boards: the pairs found once %s the search's; on this host a rebuild (setPlate, every mode moved) %.2f us, "
           "with the board's block that turns them %.2f us against %.2f us for a block where nothing moved (the rebuild %.2f us); a voice's "
           "block %.2f us: the rebuild %.2f of a voice's block (the full search %.1f us, %.1f blocks)%s\n",
           same ? "are" : "are NOT", plate, moved, still, moved - still, voice, (moved - still) / voice, search, search / voice, sink == 1e30f ? " " : "");
}

// the bridge points strictly inside the plate for every key; the listening
// weights finite and none of the low modes at a node
UP_TEST(board_bridge_and_listening) {
    const BoardTable t = boardOf(1.0f, 0.03f, 0.5f);
    bool inside = true;
    for (int m = 21; m <= 108; ++m) {
        float x, y;
        bridgePoint((float)m, t, x, y);
        inside = inside && x > 0.02f * t.a && x < 0.98f * t.a && y > 0.02f * t.b && y < 0.98f * t.b;
    }
    CHECK(inside);
    float w[kBoardModes];
    outputWeights(t, w);
    double least = 1.0;
    for (int k = 0; k < 12 && k < kBoardModes; ++k) least = std::fmin(least, std::fabs(plateShape(t.mode[k].m, t.mode[k].n, kListenX * t.a, kListenY * t.b, t)));
    CHECK_MSG(least > 0.1, "a low mode at %.3f of its peak at the listening point", least);
    printf("    the listening point (%.2f, %.2f): the twelve lowest modes' shapes at %.3f of their peak or more\n", kListenX, kListenY, least);
}

// two voices' forces in at their bridge points are the sum of each voice's alone, within 1e-6 of full scale
UP_TEST(board_two_voices_sum) {
    const int n = 4000;
    std::vector<float> fa((size_t)n), fb((size_t)n), zero((size_t)n, 0.0f);
    for (int i = 0; i < n; ++i) {
        fa[(size_t)i] = i < 40 ? (float)std::sin(0.08 * i) * 20.0f : 0.0f;
        fb[(size_t)i] = 7.0f * (float)std::sin(kTwoPi * 180.0 * i / kDefaultFs) * (float)std::exp(-i / 1500.0);
    }
    auto run = [&](const std::vector<float>& a, const std::vector<float>& b) {
        Board board;
        board.init(kDefaultFs);
        board.setPlate(1.2f, 0.025f, 0.6f);
        float wa[kBoardModes], wb[kBoardModes];
        board.inputWeights(36.0f, wa);
        board.inputWeights(79.0f, wb);
        const float* f[2] = {a.data(), b.data()};
        const float* w[2] = {wa, wb};
        std::vector<float> out((size_t)n);
        board.render(f, w, 2, out.data(), n);
        return out;
    };
    const std::vector<float> both = run(fa, fb), one = run(fa, zero), two = run(zero, fb);
    double worst = 0.0, peak = 0.0;
    for (int i = 0; i < n; ++i) {
        worst = std::fmax(worst, std::fabs((double)both[(size_t)i] - one[(size_t)i] - two[(size_t)i]));
        peak = std::fmax(peak, std::fabs((double)both[(size_t)i]));
    }
    CHECK_MSG(worst <= 1e-6, "%.3g apart (the peak %.3g)", worst, peak);
    printf("    two voices against each alone, summed: %.2e of full scale apart (the peak %.3f)\n", worst, peak);
}

// one mode alone (its input weight 1, the rest 0; the shelf's mobility 0, so neither the direct share nor the
// field) driven by one sample rings at its table's frequency within 0.1 cent (the zero crossings over 20 ms ..
// 1.5 s) and at its t60 within 1 % (the envelope's slope), for the first mode, the fifth and the last
UP_TEST(board_mode_rings_true) {
    Board probe;
    probe.init(kDefaultFs);
    probe.setPlate(1.0f, 0.01f, 1.0f);
    for (int k : {0, 4, kBoardModes - 1}) {
        Board board;
        board.init(kDefaultFs);
        board.setPlate(1.0f, 0.01f, 1.0f);
        board.setShelf(0.0f);
        float w[kBoardModes] = {};
        w[k] = 1.0f;
        const int n = (int)(3.0 * kDefaultFs);
        std::vector<float> f((size_t)n, 0.0f), out((size_t)n);
        f[0] = 1000.0f;
        const float* fp[1] = {f.data()};
        const float* wp[1] = {w};
        board.render(fp, wp, 1, out.data(), n);
        const BoardMode m = board.modeInfo(k);
        const size_t span = (size_t)(1.5 * kDefaultFs);
        double first = -1.0, last = -1.0;
        int count = 0;
        for (size_t i = (size_t)(0.02 * kDefaultFs); i < span; ++i)
            if ((out[i - 1] < 0.0f) != (out[i] < 0.0f)) {
                const double t = (double)(i - 1) + (double)out[i - 1] / ((double)out[i - 1] - (double)out[i]);
                if (first < 0.0) first = t;
                last = t;
                ++count;
            }
        const double hz = 0.5 * (count - 1) * kDefaultFs / (last - first);
        const BandAnalyser b(out.data(), out.size(), kDefaultFs);
        const double t60 = t60Of(b.envelope(0.7 * m.hz, 1.3 * m.hz, 0.02)).t60;
        const double c = 1200.0 * std::log2(hz / m.hz);
        CHECK_MSG(std::fabs(c) < 0.1, "mode %d: %.4f hz against %.4f (%+.3f cents)", k, hz, m.hz, c);
        CHECK_MSG(std::fabs(t60 / m.t60 - 1.0) < 0.01, "mode %d: t60 %.4f s against %.4f", k, t60, m.t60);
        printf("    mode %d (%d,%d): %.3f hz (%+.4f cents), t60 %.4f s against %.4f\n", k, m.m, m.n, hz, c, t60, m.t60);
    }
}

// a 60 hz force at a0's bridge point through the board: open 0 (the back closed) against 1 lowers the output's
// rms over the last second by 6 db and more
UP_TEST(board_open_lowers_60hz) {
    auto level = [](float open) {
        Board board;
        board.init(kDefaultFs);
        board.setPlate(1.0f, 0.03f, open);
        float w[kBoardModes];
        board.inputWeights(21.0f, w);
        const int n = (int)(2.0 * kDefaultFs);
        std::vector<float> f((size_t)n), out((size_t)n);
        for (int i = 0; i < n; ++i) f[(size_t)i] = (float)std::sin(kTwoPi * 60.0 * i / kDefaultFs);
        const float* fp[1] = {f.data()};
        const float* wp[1] = {w};
        board.render(fp, wp, 1, out.data(), n);
        return rms(out.data() + n / 2, (size_t)(n / 2));
    };
    const double closed = level(0.0f), open = level(1.0f);
    const double db = 20.0 * std::log10(open / closed);
    CHECK_MSG(db >= 6.0, "the 60 hz drive %.2f db louder open", db);
    printf("    a 60 hz drive: %.2f db lower with the back closed\n", db);
}

// the field's impulse energy (a unit impulse into the field alone, no highpass, 0.5 s) against the shelf's share,
// (1 - kFieldDirect²) kBoardShelf²: within 1 db at the spinet's, grand's and the concert grand's boards (kFieldGain
// set at grand's, the same gain at every board): the field and the direct share together carry the shelf's mean power
UP_TEST(board_field_carries_the_shelf) {
    const double share = (1.0 - (double)kFieldDirect * kFieldDirect) * (double)kBoardShelf * kBoardShelf;
    for (int pl : {0, 2, 4}) {
        Board b;
        plateAt(b, pl);
        double e = 0.0;
        for (int i = 0; i < (int)(0.5 * kDefaultFs); ++i) {
            const double y = b.fieldStep(i == 0 ? 1.0f : 0.0f);
            e += y * y;
        }
        const double db = 10.0 * std::log10(e / share);
        const BoardMode top = b.modeInfo(kBoardModes - 1);
        CHECK_MSG(std::fabs(db) <= 1.0, "%s board: the field's impulse energy %.4g, %+.2f db from the shelf's share %.4g", kBoardName[pl], e, db, share);
        printf("    %s board (the top mode %.0f hz, its t60 %.1f ms): the field's impulse energy %.4g (um/s per N)², %+.2f db from (1 - "
               "kFieldDirect²) kBoardShelf² %.4g\n",
               kBoardName[pl], top.hz, 1000.0 * top.t60, e, db, share);
    }
}

// 1 N for one sample at c4's bridge point at grand's board: the board's output energy within the first sample under 15 % of
// the first 300 ms's, within 5 ms over 60 %: the field spreads the force's arrival over milliseconds. printed beside
// a flat shelf's: the modes alone (the shelf's mobility 0) plus the force through the two highpasses x kBoardShelf
UP_TEST(board_field_arrives_over_milliseconds) {
    const int n = (int)(0.3 * kDefaultFs), at5 = (int)std::lround(0.005 * kDefaultFs);
    std::vector<float> f((size_t)n, 0.0f), board((size_t)n), modes((size_t)n), flat((size_t)n);
    f[0] = 1.0f;
    for (float shelf : {kBoardShelf, 0.0f}) {
        Board b;
        plateAt(b, 2);
        b.setShelf(shelf);
        float w[kBoardModes];
        b.inputWeights(60.0f, w);
        const float* fp[1] = {f.data()};
        const float* wp[1] = {w};
        b.render(fp, wp, 1, (shelf > 0.0f ? board : modes).data(), n);
    }
    Board t;
    plateAt(t, 2);
    const double a = 1.0 - std::exp(-2.0 * kPi * t.modeInfo(kBoardModes - 1).hz / kDefaultFs);
    double low1 = 0.0, low2 = 0.0;
    for (int i = 0; i < n; ++i) {
        low1 += a * (f[(size_t)i] - low1);
        const double high1 = f[(size_t)i] - low1;
        low2 += a * (high1 - low2);
        flat[(size_t)i] = modes[(size_t)i] + (float)((high1 - low2) * kBoardShelf * kBoardOut);
    }
    // the energy's share in the first sample and within 5 ms, percent of the first 300 ms's
    auto shares = [&](const std::vector<float>& y, double& first, double& five) {
        double e = 0.0, e5 = 0.0;
        for (int i = 0; i < n; ++i) {
            e += (double)y[(size_t)i] * y[(size_t)i];
            if (i < at5) e5 = e;
        }
        first = 100.0 * (double)y[0] * y[0] / e;
        five = 100.0 * e5 / e;
    };
    double first, five, flatFirst, flatFive;
    shares(board, first, five);
    shares(flat, flatFirst, flatFive);
    CHECK_MSG(first < 15.0, "the first sample %.1f %% of the energy", first);
    CHECK_MSG(five > 60.0, "within 5 ms %.1f %% of the energy", five);
    printf("    1 N for one sample at c4's bridge point, grand's board: the first sample %.1f %% of the first 300 ms's energy, within 5 ms %.1f %%; a "
           "flat shelf's %.1f %%, %.1f %%\n",
           first, five, flatFirst, flatFive);
}

// the field's loss as designed, at the spinet's, grand's and the concert grand's boards, every line's one-pole H_i = g_i (1 - a_i) / (1 - a_i z^-1) from
// fieldLine against the plate's per-pass loss 10^(-3 L_i / (t60 fs)), t60 = 6.907755 / (pi f eta) (f_top the
// plate's top mode, eta the shape's boardLoss): its gain at dc, g_i, the pass at the top mode's t60, and |H_i| at
// kFieldFitHz the pass at the plate's t60 there, each within 1e-4 relative. printed: the one-pole's t60 at f_top and
// at 1.5 f_top, t60_i(f) = -3 L_i / (fs log10 |H_i(f)|) averaged over the lines, beside the plate's and over it (its
// loss falls faster than 1 / f between the crossover and kFieldFitHz); and the field's late decay at 1.5 f_top, its
// impulse response (the field alone, 1 s) over the octave about it, 0.25 ms frames, by schroeder's backward
// integration from 10 ms (after the first pass)
UP_TEST(board_field_decays_as_the_plate) {
    for (int pl : {0, 2, 4}) {
        Board b;
        plateAt(b, pl);
        const double eta = kBoardPlates[pl].loss, top = b.modeInfo(kBoardModes - 1).hz;
        const double fs = kDefaultFs, plateTop = 6.907755 / (kPi * top * eta), plateFit = 6.907755 / (kPi * kFieldFitHz * eta);
        // a line's |H_i| at f
        auto mag = [&](int i, double f) {
            float g, a;
            b.fieldLine(i, g, a);
            const double w = kTwoPi * f / fs;
            return g * (1.0 - a) / std::sqrt(1.0 - 2.0 * a * std::cos(w) + (double)a * a);
        };
        // the lines' mean t60 at f
        auto own = [&](double f) {
            double t = 0.0;
            for (int i = 0; i < kFieldLines; ++i) t += -3.0 * kFieldLengths[i] / (fs * std::log10(mag(i, f))) / kFieldLines;
            return t;
        };
        double worstDc = 0.0, worstFit = 0.0;
        for (int i = 0; i < kFieldLines; ++i) {
            const double dc = mag(i, 0.0), fit = mag(i, kFieldFitHz);
            const double wantDc = std::pow(10.0, -3.0 * kFieldLengths[i] / (plateTop * fs)), wantFit = std::pow(10.0, -3.0 * kFieldLengths[i] / (plateFit * fs));
            CHECK_MSG(std::fabs(dc / wantDc - 1.0) <= 1e-4, "%s board, line %d: its gain at dc %.6f against the pass at the top mode's t60 %.6f",
                      kBoardName[pl], kFieldLengths[i], dc, wantDc);
            CHECK_MSG(std::fabs(fit / wantFit - 1.0) <= 1e-4, "%s board, line %d: |H| at %.0f hz %.6f against the pass at the plate's t60 there %.6f",
                      kBoardName[pl], kFieldLengths[i], (double)kFieldFitHz, fit, wantFit);
            worstDc = std::fmax(worstDc, std::fabs(dc / wantDc - 1.0));
            worstFit = std::fmax(worstFit, std::fabs(fit / wantFit - 1.0));
        }
        const double atTop = own(top), at15 = own(1.5 * top), plate15 = 6.907755 / (kPi * 1.5 * top * eta);
        const int n = (int)fs;
        std::vector<float> y((size_t)n);
        for (int i = 0; i < n; ++i) y[(size_t)i] = b.fieldStep(i == 0 ? 1.0f : 0.0f);
        const double late = schroederT60(BandAnalyser(y.data(), y.size(), fs).envelope(1.5 * top / std::sqrt(2.0), 1.5 * top * std::sqrt(2.0), 0.00025), 0.01);
        printf("    %s board (the top mode %.0f hz, eta %.4f): the lines' loss off the plate's per-pass loss by %.1e at dc, %.1e at %.0f hz; the one-pole's "
               "t60 at the top mode %.1f ms against the plate's %.1f (x %.2f), at 1.5 f_top (%.0f hz) %.1f against %.1f (x %.2f); the late decay "
               "there from 10 ms %.1f ms (x %.2f of the one-pole's, x %.2f of the plate's)\n",
               kBoardName[pl], top, eta, worstDc, worstFit, (double)kFieldFitHz, 1000.0 * atTop, 1000.0 * plateTop, atTop / plateTop, 1.5 * top, 1000.0 * at15,
               1000.0 * plate15, at15 / plate15, 1000.0 * late, late / at15, late / plate15);
    }
}

// white noise at 100 N rms for half a second into the board, the modes' weights 0 (the direct share and the field
// alone: the modes' own ring is board_mode_rings_true's), at the spinet's, grand's and the concert grand's boards and
// at 22.05, 44.1 and 96 khz: every
// sample finite, and after the input stops the output falls under -100 dbfs within 1 s
UP_TEST(board_field_is_stable) {
    bool finite = true;
    double latest = 0.0, loudest = 0.0;
    for (float fs : {22050.0f, 44100.0f, 96000.0f})
        for (int pl : {0, 2, 4}) {
            Board b;
            plateAt(b, pl, fs);
            const int drive = (int)(0.5 * fs), n = (int)(2.0 * fs);
            std::vector<float> f((size_t)n, 0.0f), out((size_t)n);
            Rng r;
            r.seed(7);
            for (int i = 0; i < drive; ++i) f[(size_t)i] = 100.0f * std::sqrt(3.0f) * r.bipolar();
            const float w[kBoardModes] = {};
            const float* fp[1] = {f.data()};
            const float* wp[1] = {w};
            b.render(fp, wp, 1, out.data(), n);
            int last = drive;
            for (int i = 0; i < n; ++i) {
                finite = finite && std::isfinite(out[(size_t)i]);
                if (i >= drive && std::fabs(out[(size_t)i]) >= 1e-5f) last = i;
            }
            const double after = (double)(last - drive) / fs, peak = peakAbs(out.data(), (size_t)drive);
            CHECK_MSG(after < 1.0, "%.0f hz, %s board: over -100 dbfs %.3f s after the input stops", fs, kBoardName[pl], after);
            latest = std::fmax(latest, after);
            loudest = std::fmax(loudest, peak);
        }
    CHECK(finite);
    printf("    100 N rms of white noise, three boards at 22.05, 44.1 and 96 khz: every sample %s; the loudest peak %.3f (%+.1f dbfs), under -100 "
           "dbfs %.3f s after the input stops at the latest\n",
           finite ? "finite" : "NOT finite", loudest, dbfs(loudest), latest);
}

// the board's impulse response at c4's bridge point (1 N for one sample, 16384 samples) at the spinet's, grand's and the
// concert grand's boards: the
// power mean of |H(f)|² over f_top .. 2 f_top, 2 .. 4 f_top and 4 .. 8 f_top (each under the ceiling) within 3 db of
// a flat shelf's there (impulseAt's flat: the modes and kBoardShelf through the two highpasses): the field and the
// direct share carry the shelf's power in band. a known row at the concert grand's, where the field's power follows its t60 (253
// ms at the top mode, 30 at 4 khz) over the lower bands: the tilt. the power mean, the mean of the db and its
// standard deviation (the grain) printed per band
UP_TEST(board_field_mean_power_is_the_shelfs) {
    const int n = 16384;
    for (int pl : {0, 2, 4}) {
        const std::vector<float> board = impulseAt(pl, n, false), flat = impulseAt(pl, n, true);
        std::vector<double> br(board.begin(), board.end()), bi((size_t)n, 0.0), fr(flat.begin(), flat.end()), fi((size_t)n, 0.0);
        fft(br, bi);
        fft(fr, fi);
        Board t;
        plateAt(t, pl);
        const double top = t.modeInfo(kBoardModes - 1).hz, binHz = kDefaultFs / n;
        double lo[3], hi[3], power[3], shelf[3], mean[3], sd[3];
        for (int j = 0; j < 3; ++j) {
            lo[j] = top * (1 << j);
            hi[j] = std::fmin(2.0 * lo[j], kCeiling * kDefaultFs);
            double p = 0.0, q = 0.0, d = 0.0, dd = 0.0;
            int count = 0;
            for (int k = (int)std::ceil(lo[j] / binHz); k * binHz < hi[j]; ++k) {
                const double h2 = br[(size_t)k] * br[(size_t)k] + bi[(size_t)k] * bi[(size_t)k];
                const double l = 10.0 * std::log10(h2 + 1e-300);
                p += h2;
                q += fr[(size_t)k] * fr[(size_t)k] + fi[(size_t)k] * fi[(size_t)k];
                d += l;
                dd += l * l;
                ++count;
            }
            power[j] = 10.0 * std::log10(p / count);
            shelf[j] = 10.0 * std::log10(q / count);
            mean[j] = d / count;
            sd[j] = std::sqrt(std::fmax(0.0, dd / count - mean[j] * mean[j]));
            if (pl < 4)
                CHECK_MSG(std::fabs(power[j] - shelf[j]) <= 3.0, "%s board, %.0f .. %.0f hz: the power mean %+.2f db, a flat shelf's %+.2f",
                          kBoardName[pl], lo[j], hi[j], power[j], shelf[j]);
            else
                KNOWN_FAILURE(std::fabs(power[j] - shelf[j]) <= 3.0, "%s board, %.0f .. %.0f hz: the power mean %+.2f db, a flat shelf's %+.2f (the "
                              "tilt)", kBoardName[pl], lo[j], hi[j], power[j], shelf[j]);
        }
        printf("    %s board (the top mode %.0f hz):", kBoardName[pl], top);
        for (int j = 0; j < 3; ++j)
            printf("%s %.0f .. %.0f hz the power mean %+.2f db against a flat shelf's %+.2f (%+.2f), the mean of the db %+.2f, sd %.2f", j ? ";" : "",
                   lo[j], hi[j], power[j], shelf[j], power[j] - shelf[j], mean[j], sd[j]);
        printf("\n");
    }
    printf("    kBoardShelf %.0f um/s per N; the infinite plate's mobility %.0f um/s per N (%+.2f db)\n", (double)kBoardShelf, 1e6 * plateMobility(),
           20.0 * std::log10(kBoardShelf / (1e6 * plateMobility())));
}
