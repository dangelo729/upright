// upright — tests/test_rank.cpp  (plan §6)
//
// the length by pure scaling until the case's longest string caps it; the
// mass and the impedance from f0 and the tension; wrapped below the break and
// plain above; the hammer's mass and the strike share falling up the
// keyboard; t60Scale 1 at c4; the bridge's scale; the damper.
#include <cmath>
#include <initializer_list>

#include "core/up_law.h"
#include "core/up_presets.h"
#include "core/up_rank.h"
#include "up_fit.h"
#include "up_shapes.h"
#include "up_test.h"

using namespace up;

// L = min(length (f_c4 / f0)^rankLength, bass), at every note on the spinet's strings, grand's and a concert grand's
UP_TEST(rank_length_capped_by_bass) {
    const LawConstants& c = lawConstants();
    int capped = 0, worstNote = 0;
    double worst = 0.0;
    for (int sc : {0, 2, 4}) {
        const Shape s = withStrings(grand(), sc);
        for (int m = 21; m <= 108; ++m) {
            const RankState r = rankAt(s, (float)m);
            const double scaled = s[Channel::length] * std::pow(261.6255653 / r.f0, (double)c.rankLength);
            const double want = std::fmin(scaled, (double)s[Channel::bass]);
            const double e = std::fabs(r.L / want - 1.0);
            if (e > worst) { worst = e; worstNote = m; }
            capped += r.L == s[Channel::bass];
        }
    }
    CHECK_MSG(worst < 1e-5, "L %.3g off at midi %d", worst, worstNote);
    const Shape& g = grand();
    CHECK(rankAt(g, 21.0f).L == g[Channel::bass] && std::fabs(rankAt(g, 60.0f).L - g[Channel::length]) < 1e-6f);
    printf("    three sets of strings, every note: L within %.2g of the rule; %d notes capped by the case\n", worst, capped);
}

// mu = T / (4 L² f0²) and Z = T / (2 L f0): the string strung to the tension
UP_TEST(rank_mass_and_impedance) {
    const Shape& s = grand();
    double worst = 0.0;
    for (int m = 21; m <= 108; ++m) {
        const RankState r = rankAt(s, (float)m);
        const double mu = r.tension / (4.0 * r.L * r.L * r.f0 * r.f0), z = r.tension / (2.0 * r.L * r.f0);
        worst = std::fmax(worst, std::fmax(std::fabs(r.mu / mu - 1.0), std::fabs(r.Z / z - 1.0)));
        // the wave speed on it is the string's own: sqrt(T / mu) = 2 L f0
        worst = std::fmax(worst, std::fabs(std::sqrt(r.tension / (double)r.mu) / (2.0 * r.L * r.f0) - 1.0));
    }
    CHECK_MSG(worst < 1e-5, "%.3g off", worst);
    const RankState c4 = rankAt(s, 60.0f);
    printf("    grand's c4: mu %.3f g/m (a plain wire of %.3f mm), Z %.3f kg/s\n", c4.mu * 1e3, c4.d * 1e3, c4.Z);
}

// wrapped below the break and plain above it, on five sets of strings from
// the spinet's to a concert grand's; a wrapped
// string's core is core x (mu / muBreak)^coreGrow, at least core, and its
// plain wire's diameter over kDmax
UP_TEST(rank_wrapped_below_the_break) {
    const LawConstants& c = lawConstants();
    for (int sc = 0; sc < 5; ++sc) {
        const Shape s = withStrings(grand(), sc);
        bool seenPlain = false, ok = true;
        for (int m = 21; m <= 108; ++m) {
            const RankState r = rankAt(s, (float)m);
            if (!r.wrapped) seenPlain = true;
            const float muBreak = 0.25f * kPi * kRho * c.dMax * c.dMax;
            ok = ok && !(seenPlain && r.wrapped) &&
                 (r.wrapped ? (r.d > c.dMax && r.dCore == c.core * std::pow(r.mu / muBreak, c.coreGrow) && r.dCore >= c.core)
                            : (r.d <= c.dMax && r.dCore == r.d));
        }
        CHECK_MSG(ok, "length %.4f, bass %.4f: a wrapped note above a plain one", kStringSizes[sc].length, kStringSizes[sc].bass);
    }
    const Shape& g = grand();
    CHECK(rankAt(g, 21.0f).wrapped && !rankAt(g, 72.0f).wrapped);
    Shape thick = g;
    thick[Channel::gauge] = 1.2f;                          // a thicker wire is wrapped sooner
    int bg = 0, bt = 0;
    for (int m = 21; m <= 108 && (!bg || !bt); ++m) {
        if (!bg && !rankAt(g, (float)m).wrapped) bg = m;
        if (!bt && !rankAt(thick, (float)m).wrapped) bt = m;
    }
    CHECK_MSG(bt > bg, "the break at gauge 1.2: %d, at 1: %d", bt, bg);
}

// the hammer's mass and the strike share falling up the keyboard, the
// felt's stiffness and power rising; t60Scale 1 at c4, over 1 under it,
// falling up the keyboard and held at t60ScaleMax in the deep bass (a0's
// 2.5, c3's under it and the rank's power unchanged); the bridge's pi T / L
UP_TEST(rank_hammer_strike_decay) {
    const Shape& s = grand();
    bool falling = true;
    RankState last = rankAt(s, 21.0f);
    for (int m = 22; m <= 108; ++m) {
        const RankState r = rankAt(s, (float)m);
        falling = falling && r.hammerMass < last.hammerMass && r.strikeShare < last.strikeShare && r.hardnessScale > last.hardnessScale &&
                  r.pAdd > last.pAdd && (r.t60Scale < last.t60Scale || r.t60Scale == lawConstants().t60ScaleMax);
        last = r;
    }
    CHECK(falling);
    const LawConstants& lc = lawConstants();
    const RankState c3 = rankAt(s, 48.0f);
    CHECK_MSG(rankAt(s, 21.0f).t60Scale == lc.t60ScaleMax, "a0's t60 scale %.4f", rankAt(s, 21.0f).t60Scale);
    CHECK_MSG(c3.t60Scale < lc.t60ScaleMax && c3.t60Scale == std::pow(midiToHz(60.0f) / c3.f0, lc.rankLoss), "c3's t60 scale %.4f", c3.t60Scale);
    int capped = 0;
    for (int m = 21; m <= 108; ++m) capped += rankAt(s, (float)m).t60Scale == lc.t60ScaleMax;
    const RankState c4 = rankAt(s, 60.0f), a0 = rankAt(s, 21.0f), c8 = rankAt(s, 108.0f);
    CHECK(std::fabs(c4.t60Scale - 1.0f) < 1e-6f && std::fabs(c4.hammerMass - s[Channel::hammerMass] * 1e-3f) < 1e-9f &&
          std::fabs(c4.strikeShare - s[Channel::strikeShare]) < 1e-7f);
    CHECK(std::fabs(c4.bridgeScale - kPi * c4.tension / c4.L) < 1e-3f);
    CHECK(c4.damped && !c8.damped);
    printf("    the hammer %.2f g at a0, %.2f at c4, %.2f at c8; the felt's K x %.3f, 1, %.2f and p %+.3f, 0, %+.3f; the strike share %.4f, %.4f, "
           "%.4f; t60 x %.2f (held at %.1f under %d keys), %.3f at c3, 1, %.3f at c8\n",
           a0.hammerMass * 1e3, c4.hammerMass * 1e3, c8.hammerMass * 1e3, a0.hardnessScale, c8.hardnessScale, a0.pAdd, c8.pAdd, a0.strikeShare,
           c4.strikeShare, c8.strikeShare, a0.t60Scale, lc.t60ScaleMax, capped, c3.t60Scale, c8.t60Scale);
    printf("    the felt's K at the note (N/mm^p, rankHardLo %.2f under c4, rankHardHi %.2f from c4):", lc.rankHardLo, lc.rankHardHi);
    for (float m : {21.0f, 36.0f, 60.0f, 84.0f, 108.0f}) printf(" %s %.1f", noteName(m).c_str(), s[Channel::hardness] * rankAt(s, m).hardnessScale);
    printf("\n");
}
