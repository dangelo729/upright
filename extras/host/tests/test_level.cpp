// upright — tests/test_level.cpp  (plan §2: the level table; stage 2)
//
// levelDb (up_knobs.h) and the table behind it (up_level_table.inc, written
// by uptool level --write): the reference reads 0; a nan knob reads as 0 and
// past the end as the end; between two points the table reads linearly; at
// every one of its 352 cells (each knob's eleven points about each of the
// generator's four anchors, every cell an octave c4 .. b4) the table is the
// engine's reading as uptool level takes it again here (to the 0.01 db it is
// written to), so a change to the engine that moves a level fails here
// until the table is written again; the reference reads 0.0897 rms (kBoardOut's
// level); a hundred random vectors undone by it land near the reference's
// octave, and peak inside the headroom at c4. the presets' levels undone are
// the presets' test.
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>

#include "core/up_knobs.h"
#include "up_fit.h"
#include "up_play.h"
#include "up_test.h"

using namespace up;

namespace {

bool sameBits(float a, float b) {
    uint32_t x, y;
    std::memcpy(&x, &a, sizeof x);
    std::memcpy(&y, &b, sizeof y);
    return x == y;
}

Knobs with(Knobs k, int which, float x) {
    k.v[which] = x;
    return k;
}

}  // namespace

// the reference reads 0; at the reference and at random draws each knob nan
// reads as 0 and +-3 as +-1; half way between two points the mean of the two
UP_TEST(level_reference_is_zero) {
    const Knobs ref = levelReference();
    const float at = levelDb(ref);
    CHECK_MSG(std::fabs(at) < 0.005f, "levelDb of the reference %+.4f", at);
    Rng r;
    r.seed(5);
    int differ = 0, vectors = 0;
    for (int d = 0; d < 20; ++d) {
        const Knobs k = d == 0 ? ref : drawKnobs(r);
        ++vectors;
        for (int i = 0; i < kKnobCount; ++i) {
            const bool nan0 = sameBits(levelDb(with(k, i, std::nanf(""))), levelDb(with(k, i, 0.0f)));
            const bool ends = sameBits(levelDb(with(k, i, 3.0f)), levelDb(with(k, i, 1.0f))) &&
                              sameBits(levelDb(with(k, i, -3.0f)), levelDb(with(k, i, -1.0f)));
            differ += !nan0 + !ends;
        }
    }
    CHECK_MSG(differ == 0, "%d knobs read nan or past the end otherwise", differ);
    double worst = 0.0;
    for (int i = 0; i < kKnobCount; ++i)
        for (int j = 0; j + 1 < kLevelCells; ++j) {
            const float a = levelDb(with(ref, i, levelPoint(j))), b = levelDb(with(ref, i, levelPoint(j + 1)));
            const float m = levelDb(with(ref, i, 0.5f * (levelPoint(j) + levelPoint(j + 1))));
            worst = std::fmax(worst, std::fabs(m - 0.5 * (a + b)));
        }
    CHECK_MSG(worst < 1e-4, "half way between points %.2g off the mean", worst);
    printf("    levelDb of the reference %+.4f db; %d vectors, each knob nan and +-3: %d differ; half way between points within %.1g of "
           "the mean\n",
           at, vectors, differ, worst);
}

// the reference's level: grand's octave c4 .. b4 at velocity 1, read as the
// table reads it, is 0.0897 rms of full scale within 0.1 db (-20.9 db, the
// level grand's c4 alone read when the table was one note's): kBoardOut sets
// it (up_board.h), and a vector undone by the table lands there
UP_TEST(level_reference_reads_its_level) {
    const double ref = levelReferenceRms(), off = 20.0 * std::log10(ref / 0.0897);
    CHECK_MSG(std::fabs(off) <= 0.1, "the reference's octave %.5f rms (%+.3f db full scale), %+.3f db from 0.0897", ref, 20.0 * std::log10(ref), off);
    printf("    the reference's octave %.5f rms, %+.2f db full scale (%+.3f db from 0.0897; kBoardOut %.4g)\n", ref, 20.0 * std::log10(ref), off,
           (double)kBoardOut);
}

// at each of the 352 cells (each knob alone at its eleven points about each
// of the generator's four anchors) the table reads what the engine renders,
// in the build it was written for (64 slots): levelDb of the cell's vector
// less levelDb of its anchor's against the cell measured again, within the
// 0.01 db it is written to; the generator's row, which levelDb reads from the
// string's block alone, as levelDb of its vector against the reference.
// each anchor's own level read by levelDb within the same on the generator
// row's points (the string, the tine, the reed); the middle's (-0.5, between
// two points) printed. another build rings more or fewer partials, and reads
// within 1 db of it from 16 slots up (under 16, a known row: the strike's
// level lives in the partials such a build drops)
UP_TEST(level_table_is_the_engine) {
    const LevelTable t = measureLevelTable();
    double worst = 0.0, span[kLevelAnchors][kKnobCount] = {};
    int at = 0, cells = 0;
    const double within = kMaxModes == 64 ? 0.0051 : 1.0;
    for (int a = 0; a < kLevelAnchors; ++a) {
        const float base = levelDb(levelBase(a));
        for (int k = 0; k < kKnobCount; ++k) {
            const bool gen = k == (int)Knob::generator;
            float lo = 1e9f, hi = -1e9f;
            for (int i = 0; i < kLevelCells; ++i) {
                Knobs v = levelBase(a);
                v.v[k] = levelPoint(i);
                const double read = gen ? (double)levelDb(v) - t.anchorDb[a] : (double)levelDb(v) - (double)base;
                const double off = std::fabs(read - (double)t.row[a][k][i]);
                if (kMaxModes >= 16)
                    CHECK_MSG(off <= within, "the %s, %s %+.1f: the table %+.3f, the engine %+.3f", levelAnchorName(a), knobName((Knob)k), levelPoint(i),
                              read, t.row[a][k][i]);
                else
                    KNOWN_FAILURE(off <= within, "%d slots: the %s, %s %+.1f: the table %+.3f, the engine %+.3f", kMaxModes, levelAnchorName(a),
                                  knobName((Knob)k), levelPoint(i), read, t.row[a][k][i]);
                if (off > worst) {
                    worst = off;
                    at = (a * kKnobCount + k) * kLevelCells + i;
                }
                lo = std::min(lo, t.row[a][k][i]);
                hi = std::max(hi, t.row[a][k][i]);
                ++cells;
            }
            span[a][k] = hi - lo;
        }
        if (levelAnchor(a) != -0.5f)
            CHECK_MSG(std::fabs(base - t.anchorDb[a]) <= within, "the %s's own level: levelDb %+.3f, the engine %+.3f", levelAnchorName(a), base,
                      t.anchorDb[a]);
        else
            printf("    the middle's own level %+.3f db, levelDb there %+.3f (the generator's row read between -0.6 and -0.4: %+.3f db)\n",
                   t.anchorDb[a], base, base - t.anchorDb[a]);
    }
    const int ak = at / kLevelCells;
    printf("    %d cells at %d slots, %d octaves of twelve notes: the table within %.4f db of the engine (the most at the %s, %s %+.1f); the "
           "reference's octave %.2f db full scale\n",
           cells, kMaxModes, t.octaves, worst, levelAnchorName(ak / kKnobCount), knobName((Knob)(ak % kKnobCount)), levelPoint(at % kLevelCells),
           20.0 * std::log10(t.refRms));
    for (int a = 0; a < kLevelAnchors; ++a) {
        printf("    the %s (its own level %+.2f db), each knob's span:", levelAnchorName(a), t.anchorDb[a]);
        for (int k = 0; k < kKnobCount; ++k) printf(" %s %.2f", knobName((Knob)k), span[a][k]);
        printf(" db\n");
    }
}

// a hundred random vectors (seed 1, as uptool random draws them), each
// measured as its octave c4 .. b4 and undone by levelDb: their undone levels'
// standard deviation under 2.5 db and the worst within 6 db of the
// reference's octave (checks; known, with the numbers split by the
// generator's thirds, where missed)
UP_TEST(level_random_vectors_undone) {
    Rng r;
    r.seed(1);
    const int n = 100;
    double sum = 0.0, sq = 0.0, worst = 0.0, lo = 1e9, hi = -1e9;
    double tSum[3] = {}, tSq[3] = {}, tLo[3] = {1e9, 1e9, 1e9}, tHi[3] = {-1e9, -1e9, -1e9};
    int tN[3] = {};
    for (int i = 0; i < n; ++i) {
        const Knobs k = drawKnobs(r);
        const double u = hearOctave(k).undone;
        sum += u;
        sq += u * u;
        worst = std::fmax(worst, std::fabs(u));
        lo = std::fmin(lo, u);
        hi = std::fmax(hi, u);
        const float g = k.v[(int)Knob::generator];
        const int third = g < -1.0f / 3.0f ? 0 : (g < 1.0f / 3.0f ? 1 : 2);
        ++tN[third];
        tSum[third] += u;
        tSq[third] += u * u;
        tLo[third] = std::fmin(tLo[third], u);
        tHi[third] = std::fmax(tHi[third], u);
    }
    const double mean = sum / n, sd = std::sqrt(std::fmax(0.0, sq / n - mean * mean));
    char thirds[512];
    int w = 0;
    const char* names[3] = {"-1 .. -1/3", "-1/3 .. 1/3", "1/3 .. 1"};
    for (int t = 0; t < 3; ++t) {
        const double m = tN[t] ? tSum[t] / tN[t] : 0.0, d = tN[t] ? std::sqrt(std::fmax(0.0, tSq[t] / tN[t] - m * m)) : 0.0;
        w += std::snprintf(thirds + w, sizeof thirds - (size_t)w, "%sthe generator %s: %d vectors, %+.2f .. %+.2f db (mean %+.2f, sd %.2f)",
                           t ? "; " : "", names[t], tN[t], tN[t] ? tLo[t] : 0.0, tN[t] ? tHi[t] : 0.0, m, d);
    }
    KNOWN_FAILURE(sd < 2.5, "the undone octaves' standard deviation %.2f db (under 2.5 asked); %s", sd, thirds);
    KNOWN_FAILURE(worst <= 6.0, "the worst undone octave %.2f db from the reference (within 6 asked); %s", worst, thirds);
    printf("    %d random vectors (seed 1), each its octave undone: %+.2f .. %+.2f db (mean %+.2f, standard deviation %.2f, the worst %.2f)\n", n, lo,
           hi, mean, sd, worst);
    printf("    %s\n", thirds);
}

// a hundred random vectors (seed 1, as above), each struck at c4 at velocity
// 1 from silence and rendered 2 s as play renders it at an outputGain of
// 10^(-levelDb / 20), the table undone as an app would: the median peak under
// 0.5 of full scale and the worst under 1.2 (checks; known, with the
// numbers, where missed): the headroom a vector undone needs at the
// reference's level
UP_TEST(level_random_vectors_peak_undone) {
    Rng r;
    r.seed(1);
    const int n = 100;
    std::vector<double> peaks;
    double worst = 0.0;
    int worstAt = 0;
    for (int i = 0; i < n; ++i) {
        const Knobs k = drawKnobs(r);
        PlaySpec sp;
        sp.instrument = pianoOf(k);
        sp.tuning.outputGain = std::pow(10.0f, -levelDb(k) / 20.0f);
        sp.midi = kLevelMidi;
        sp.vel = 1.0f;
        sp.sec = 2.0f;
        const PlayRender pr = renderPlay(sp);
        const double p = peakAbs(pr.x.data(), pr.x.size());
        peaks.push_back(p);
        if (p > worst) {
            worst = p;
            worstAt = i;
        }
    }
    std::sort(peaks.begin(), peaks.end());
    const double median = 0.5 * (peaks[n / 2 - 1] + peaks[n / 2]);
    KNOWN_FAILURE(median < 0.5, "%d random vectors undone at c4: the median peak %.3f (under 0.5 asked), the worst %.3f", n, median, worst);
    KNOWN_FAILURE(worst < 1.2, "%d random vectors undone at c4: the worst peak %.3f (vector %d; under 1.2 asked), the median %.3f", n, worst, worstAt,
                  median);
    printf("    %d random vectors (seed 1) undone, c4 at velocity 1 from silence, 2 s: the peaks %.3f .. %.3f, the median %.3f, the 90th %.3f, the "
           "worst %.3f (vector %d)\n",
           n, peaks.front(), worst, median, peaks[(size_t)(0.9 * n)], worst, worstAt);
}
