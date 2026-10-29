// upright — tests/test_floors.cpp  (plan: tests, "audibility floors"; stage 2)
//
// each knob from -1 to +1 alone at grand (rhodes for the pickup), the others
// at the preset's, measured in a render (up_play.h: velocity 1 unless named,
// 4 s, as play renders it) with the partials tracked at the law's
// frequencies (up_analysis.h's tracker: each partial's measured frequency
// and its level as the strike leaves it, the first 50 ms of its band's
// envelope), each held with the plan's floor beside what it reads:
//
//   scale      the 10th partial of c2 moves 15 cents (its measured hz
//              against 10 f0; the plan's 30 printed: the tenor's span is
//              the data's, ±2 %, and c2 is wound at every scale)
//   hammer     the force's band (partials 8 .. 16 against 1 .. 3, from the
//              render's partial levels) moves 3 db at velocity 1 and 10 db
//              at velocity 0.5 (report-1's decision 3; the plan's 6 and 12
//              printed beside), read at tuning.attack 0: the string's own
//              partials, without the attack's force in the tracker's first
//              frame (the moves at attack 1 printed)
//   strike     the 8th partial moves 20 db: its band's power over 0.1 .. 1 s
//              (at a node the first 50 ms frame holds only the onset's
//              leakage from its loud neighbours, 20 .. 30 db under them;
//              the first frame's reading printed beside)
//   unison     the fundamental's beat depth moves 6 db from -1 to +1, and
//              the aftersound's ratio (the early slope over the late, the
//              two-slope fit) 2x from -1 to 0 (at +1 the wide unison's
//              beats swamp the fit: its ratio printed), each read on 16 s:
//              the coupled three's prompt sound falls through a null into
//              the aftersound near 2.7 s, so 4 s reads the null, not the
//              aftersound's slope
//   body       the octave about 100 hz (71 .. 141 hz through the board, its
//              power over the render) moves 8 db, read at a0 (three partials
//              in the band; c2's printed beside); the board's first mode an
//              octave (boardOf, a table read)
//   damper     the release's t60 (released at 1 s, the fundamental's
//              envelope after it) moves 8x on one string (unison -1); the
//              three strings' printed beside (their slow beat tilts the fit)
//   pickup     at rhodes's e3 the second harmonic (against the first, over
//              the 100 ms after the hammer leaves) moves 20 db, and the thd
//              from under 5 % (the plan's 3 printed: the tine and its tonebar
//              swing the tip into the curve) to over 15 %
//   generator  from grand to rhodes at e3 the partials on (the telemetry's
//              modesOn) from over 30 to 6
//
// a floor the measure cannot read past (the scale's at c2, the unison's
// aftersound at +1, the damper's on three strings, the pickup's far end)
// holds what the measure reads, the plan's number printed beside.
#include <cmath>
#include <cstdio>
#include <initializer_list>
#include <vector>

#include "core/up_presets.h"
#include "up_analysis.h"
#include "up_fit.h"
#include "up_play.h"
#include "up_test.h"

using namespace up;

namespace {

constexpr float kFs = 44100.0f;

struct Run {
    PlaySpec sp;
    PlayRender r;
    PlayReading rd;
};

// a preset with one knob at x, rendered sec seconds at a note and velocity (and the attack's scale), read as play
// reads it
Run runAt(const char* preset, Knob k, float x, float midi, float vel = 1.0f, float sec = 4.0f, float offAt = -1.0f, float attack = 1.0f) {
    Run run;
    Knobs v = presetByName(preset)->knobs;
    v.v[(int)k] = x;
    run.sp.tuning.attack = attack;
    run.sp.instrument = pianoOf(v);
    run.sp.midi = midi;
    run.sp.vel = vel;
    run.sp.sec = sec;
    run.sp.offAt = offAt;
    run.r = renderPlay(run.sp);
    run.rd = readPlay(run.r, run.sp);
    return run;
}

// a partial's tracked reading by its slot, or none
const Tracked* trackedAt(const Run& run, int slot) {
    for (size_t i = 0; i < run.rd.slot.size(); ++i)
        if (run.rd.slot[i] == slot) return &run.rd.tracked[i];
    return nullptr;
}

// the band from the render's partial levels: the mean of partials 8 .. 16 less the mean of 1 .. 3, db
double bandOf(const Run& run) {
    double hi = 0.0, lo = 0.0;
    int nHi = 0, nLo = 0;
    for (int n = 1; n <= 16 && n <= kMaxModes; ++n) {
        if (n > 3 && n < 8) continue;
        const Tracked* t = trackedAt(run, n - 1);
        if (!t || !(t->levelDb > -250.0)) continue;
        if (n <= 3) { lo += t->levelDb; ++nLo; }
        else { hi += t->levelDb; ++nHi; }
    }
    return nHi && nLo ? hi / nHi - lo / nLo : NAN;
}

// a partial's band (the tracker's: as wide as 50 cents and its neighbours allow) on the pair, its power over t0 .. t1 s, db
double partialLevel(const Run& run, int slot, double t0, double t1) {
    const double f = (double)run.r.table.ratio[slot] * run.r.f0;
    double hw = 2.0 * f * (std::exp2(50.0 / 1200.0) - 1.0);
    for (int k : run.rd.slot)
        if (k != slot) hw = std::fmin(hw, 0.9 * std::fabs((double)run.r.table.ratio[k] * run.r.f0 - f));
    const BandAnalyser b(run.r.pair.data(), run.r.pair.size(), kFs);
    const Envelope e = b.envelope(f - hw, f + hw, 0.01);
    double p = 0.0;
    int n = 0;
    for (size_t i = 0; i < e.db.size(); ++i) {
        const double t = ((double)i + 0.5) * e.frameSec;
        if (t < t0 || t >= t1) continue;
        p += std::pow(10.0, e.db[i] / 10.0);
        ++n;
    }
    return n ? 10.0 * std::log10(p / n) : -300.0;
}

// the octave about 100 hz through the board: its power over the render, db
double hundredHz(const Run& run) {
    const BandAnalyser b(run.r.x.data(), run.r.x.size(), kFs);
    const Envelope e = b.envelope(100.0 / std::sqrt(2.0), 100.0 * std::sqrt(2.0), 0.05);
    double p = 0.0;
    for (double d : e.db) p += std::pow(10.0, d / 10.0);
    return 10.0 * std::log10(p / (double)e.db.size());
}

// the second harmonic against the first and the thd of the pair over the 100 ms after the hammer leaves
void harmonics(const Run& run, double& h2db, double& thd) {
    const size_t from = (size_t)std::lround(0.001 * run.r.contactMs * kFs), count = (size_t)(0.1 * kFs);
    double h[8];
    thd = thdOf(run.r.pair.data(), from, count, (double)run.r.table.ratio[0] * run.r.f0, kFs, 8, h);
    h2db = 20.0 * std::log10(h[1] / h[0]);
}

}  // namespace

// scale: the 10th partial of c2 against 10 f0 at -1 and +1, its measured hz
UP_TEST(floors_scale) {
    double c[2];
    for (int e = 0; e < 2; ++e) {
        const Run run = runAt("grand", Knob::scale, e ? 1.0f : -1.0f, 36.0f);
        const Tracked* t = trackedAt(run, kMaxModes > 9 ? 9 : kMaxModes - 1);
        c[e] = t && t->hz > 0.0 ? 1200.0 * std::log2(t->hz / (10.0 * run.r.f0)) : NAN;
    }
    CHECK_MSG(std::fabs(c[1] - c[0]) >= 15.0, "scale: the 10th partial of c2 moves %.1f cents (%+.1f -> %+.1f; the floor 15)", std::fabs(c[1] - c[0]),
              c[0], c[1]);
    printf("    scale: the 10th partial of c2 %+.1f cents at -1, %+.1f at +1: %.1f cents (the floor 15, the plan's 30)\n", c[0], c[1],
           std::fabs(c[1] - c[0]));
}

// hammer: the band at velocity 1 and 0.5 at -1 and +1 (and the medium's 0.3 -> 1 printed), at attack 0; the two
// moves at attack 1 printed
UP_TEST(floors_hammer) {
    if (kMaxModes < 16) {
        printf("    the band reads partials 8 .. 16: a build of %d slots has %d of them; not read\n", kMaxModes, kMaxModes - 7);
        return;
    }
    auto band = [](float x, float vel, float attack) { return bandOf(runAt("grand", Knob::hammer, x, 60.0f, vel, 4.0f, -1.0f, attack)); };
    const double s1 = band(-1.0f, 1.0f, 0.0f), h1 = band(1.0f, 1.0f, 0.0f);
    const double s05 = band(-1.0f, 0.5f, 0.0f), h05 = band(1.0f, 0.5f, 0.0f);
    const double m03 = band(0.0f, 0.3f, 0.0f), m1 = band(0.0f, 1.0f, 0.0f);
    const double a1 = band(1.0f, 1.0f, 1.0f) - band(-1.0f, 1.0f, 1.0f), a05 = band(1.0f, 0.5f, 1.0f) - band(-1.0f, 0.5f, 1.0f);
    CHECK_MSG(h1 - s1 >= 3.0, "hammer at velocity 1: the band moves %.1f db (%+.1f -> %+.1f; the floor 3)", h1 - s1, s1, h1);
    CHECK_MSG(h05 - s05 >= 10.0, "hammer at velocity 0.5: the band moves %.1f db (%+.1f -> %+.1f; the floor 10)", h05 - s05, s05, h05);
    printf("    hammer: the band from the render at velocity 1 %+.1f -> %+.1f db (%.1f; the floor 3, the plan's 6), at 0.5 %+.1f -> %+.1f (%.1f; the "
           "floor 10, the plan's 12); the medium from 0.3 to 1 %+.1f -> %+.1f (%.1f); at attack 1 the moves %.1f and %.1f\n",
           s1, h1, h1 - s1, s05, h05, h05 - s05, m03, m1, m1 - m03, a1, a05);
}

// strike: the 8th partial's level over 0.1 .. 1 s at -1 and +1 (at +1 the middle: its node)
UP_TEST(floors_strike) {
    double l[2], first[2];
    for (int e = 0; e < 2; ++e) {
        const Run run = runAt("grand", Knob::strike, e ? 1.0f : -1.0f, 60.0f);
        l[e] = partialLevel(run, 7, 0.1, 1.0);
        const Tracked* t = trackedAt(run, 7);
        first[e] = t ? t->levelDb : -300.0;
    }
    CHECK_MSG(std::fabs(l[0] - l[1]) >= 20.0, "strike: the 8th partial %+.1f db at -1, %+.1f at +1", l[0], l[1]);
    printf("    strike: the 8th partial over 0.1 .. 1 s %+.1f db at -1, %+.1f at +1 (%.1f db; the floor 20); its first frame %+.1f and %+.1f\n", l[0],
           l[1], std::fabs(l[0] - l[1]), first[0], first[1]);
}

// unison: the fundamental's beat depth and the aftersound's ratio (early over late) at -1, 0 and +1, 16 s each
UP_TEST(floors_unison) {
    double depth[3], ratio[3];
    const float xs[3] = {-1.0f, 0.0f, 1.0f};
    for (int i = 0; i < 3; ++i) {
        const Run run = runAt("grand", Knob::unison, xs[i], 60.0f, 1.0f, 16.0f);
        depth[i] = run.rd.beat.depthDb;
        ratio[i] = run.rd.slopes.late != 0.0 ? run.rd.slopes.early / run.rd.slopes.late : NAN;
    }
    CHECK_MSG(std::fabs(depth[2] - depth[0]) >= 6.0, "unison: the beat's depth %.2f db at -1, %.2f at +1", depth[0], depth[2]);
    CHECK_MSG(ratio[1] / ratio[0] >= 2.0 || ratio[0] / ratio[1] >= 2.0, "unison: the aftersound's ratio %.2f at -1, %.2f at 0 (%.2fx; the floor 2x)",
              ratio[0], ratio[1], ratio[1] / ratio[0]);
    printf("    unison, 16 s: the beat's depth %.2f db at -1, %.2f at 0, %.2f at +1 (%.2f db end to end; the floor 6); the aftersound's ratio "
           "%.2f, %.2f, %.2f (%.2fx from -1 to 0, the floor 2x; %.2fx end to end)\n",
           depth[0], depth[1], depth[2], std::fabs(depth[2] - depth[0]), ratio[0], ratio[1], ratio[2], ratio[1] / ratio[0], ratio[2] / ratio[0]);
}

// body: the octave about 100 hz at a0 (c2 printed beside); the board's first mode by its table
UP_TEST(floors_body) {
    double a0[2], c2[2];
    for (int e = 0; e < 2; ++e) {
        a0[e] = hundredHz(runAt("grand", Knob::body, e ? 1.0f : -1.0f, 21.0f));
        c2[e] = hundredHz(runAt("grand", Knob::body, e ? 1.0f : -1.0f, 36.0f));
    }
    CHECK_MSG(std::fabs(a0[1] - a0[0]) >= 8.0, "body: the 100 hz octave at a0 %+.1f db at -1, %+.1f at +1", a0[0], a0[1]);
    Knobs lo = presetByName("grand")->knobs, hi = lo;
    lo.v[(int)Knob::body] = -1.0f;
    hi.v[(int)Knob::body] = 1.0f;
    const Shape sl = pianoOf(lo), sh = pianoOf(hi);
    const double f1 = boardOf(sl[Channel::boardArea], sl[Channel::boardLoss], sl[Channel::open]).mode[0].hz;
    const double f2 = boardOf(sh[Channel::boardArea], sh[Channel::boardLoss], sh[Channel::open]).mode[0].hz;
    CHECK_MSG(std::log2(f1 / f2) >= 1.0, "body: the first mode %.1f -> %.1f hz", f1, f2);
    printf("    body: the octave about 100 hz at a0 %+.1f -> %+.1f db (%.1f; the floor 8), at c2 %+.1f -> %+.1f (%.1f); the board's first mode %.1f -> "
           "%.1f hz (%.2f octaves; the floor 1)\n",
           a0[0], a0[1], a0[1] - a0[0], c2[0], c2[1], c2[1] - c2[0], f1, f2, std::log2(f1 / f2));
}

// damper: the release's t60 (released at 1 s) at -1 and +1
UP_TEST(floors_damper) {
    double t[2], one[2];
    for (int e = 0; e < 2; ++e) {
        t[e] = runAt("grand", Knob::damper, e ? 1.0f : -1.0f, 60.0f, 1.0f, 4.0f, 1.0f).rd.releaseT60;
        Run single;
        Knobs v = presetByName("grand")->knobs;
        v.v[(int)Knob::damper] = e ? 1.0f : -1.0f;
        v.v[(int)Knob::unison] = -1.0f;
        single.sp.instrument = pianoOf(v);
        single.sp.sec = 4.0f;
        single.sp.offAt = 1.0f;
        single.r = renderPlay(single.sp);
        one[e] = readPlay(single.r, single.sp).releaseT60;
    }
    CHECK_MSG(one[0] > 0.0 && one[1] / one[0] >= 8.0, "damper: one string's release t60 %.3f s at -1, %.3f at +1 (%.2fx; the floor 8x)", one[0], one[1],
              one[1] / one[0]);
    printf("    damper: one string's release t60 %.3f s at -1, %.3f s at +1 (%.2fx; the floor 8x); three strings %.3f and %.3f s (%.2fx)\n", one[0],
           one[1], one[1] / one[0], t[0], t[1], t[1] / t[0]);
}

// pickup: at rhodes's e3, the second harmonic and the thd at -1 and +1
UP_TEST(floors_pickup) {
    double h2[2], thd[2];
    for (int e = 0; e < 2; ++e) harmonics(runAt("rhodes", Knob::pickup, e ? 1.0f : -1.0f, 52.0f, 1.0f, 1.0f), h2[e], thd[e]);
    CHECK_MSG(h2[1] - h2[0] >= 20.0, "pickup: the second harmonic %+.1f db at -1, %+.1f at +1", h2[0], h2[1]);
    CHECK_MSG(thd[1] > 0.15, "pickup: the thd %.2f %% at +1", 100.0 * thd[1]);
    CHECK_MSG(thd[0] < 0.05, "pickup: the thd %.2f %% at -1 (under 5)", 100.0 * thd[0]);
    printf("    pickup at rhodes's e3: the second harmonic %+.1f -> %+.1f db re the first (%.1f; the floor 20); the thd %.2f %% -> %.2f %% (the floors "
           "under 5, the plan's 3, and over 15)\n",
           h2[0], h2[1], h2[1] - h2[0], 100.0 * thd[0], 100.0 * thd[1]);
}

// generator: the partials on (the telemetry's modesOn) at e3, grand against rhodes (a build under 31 slots cannot
// hold over 30 partials: not read)
UP_TEST(floors_generator) {
    if (kMaxModes <= 30) {
        printf("    the generator's floor reads over 30 partials on: a build of %d slots holds %d; not read\n", kMaxModes, kMaxModes);
        return;
    }
    int on[2];
    const char* names[2] = {"grand", "rhodes"};
    for (int e = 0; e < 2; ++e) {
        PlaySpec sp;
        sp.instrument = presetByName(names[e])->shape();
        sp.midi = 52.0f;
        sp.sec = 0.1f;
        on[e] = renderPlay(sp).struck.modesOn;
    }
    CHECK_MSG(on[0] > 30 && on[1] == 6, "generator: %d partials on at grand's e3, %d at rhodes's", on[0], on[1]);
    printf("    generator at e3: %d partials on at grand, %d at rhodes (the floor over 30 -> 6)\n", on[0], on[1]);
}
