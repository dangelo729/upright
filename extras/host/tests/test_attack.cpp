// upright — tests/test_attack.cpp  (the attack: the knock, the thud, the clank; up_attack.h)
//
// grand rendered as play renders it at tuning.attack 1 and 0. the attack
// feeds nothing back into the strings, so the strings are the same bits
// either way, and the output at 1 less the output at 0 is the attack's own;
// its parts apart are read from the voice rendered a sample at a time
// (Voice::attack().parts(), at the attack's weight, 1 on grand), each through
// a board of its own at the note's bridge point (the board is linear). every
// level is against the string's own (the output at attack 0) over the same
// window.
//
//   attack 0 runs nothing    at attack 0 the attack never runs and its long
//                            bank stays at zero; at 0 and 1 the strings the
//                            same bits every block; the bridge force's first
//                            1.5 ms at 0 printed
//   the knock                grand's c4, velocity 1, over the first 10 ms:
//                            the blow's share 10 db under the string's own,
//                            the attack's own 9.99 db under (each within
//                            1.5 db); at velocity 0.5 the knock falls at
//                            least as far as the string; c2 and c6 printed
//   the thud                 the output's band under 150 hz over 5 .. 40 ms
//                            at c4, attack 1 against 0: 4 +- 1 db; c2 printed
//   the clank's pitch        the attack's own force over 24 .. 84 ms (the
//                            clank alone) peaks within 2 % of the rank's
//                            longHz at c2 and c4; a0 and c6 printed; c7's
//                            modes over the ceiling: never on
//   the clank's level        grand's a0, velocity 1: the clank's own output
//                            over the first 50 ms 10 +- 1 db under the
//                            note's own; a0, c2, c4, c6 at velocity 1 and 0.5
//                            against the note and against the strongest
//                            partial printed; the square law at c2: the
//                            clank's band from velocity 0.5 to 1 at least 1.7
//                            x the strongest partial's move
//   the bottom notes         every acoustic preset's bottom note at velocity
//                            1 peaks under 1.0
//   the tine side            rhodes at e4 and wurlitzer at a3: the same bytes
//                            at attack 0 and 1
//   the pool                 a chord of four at attack 1: finite and under the
//                            clamp
//   the cost                 a ringing grand c4's voice block at attack 1
//                            against 0, and the attack's own block, printed
#include <chrono>
#include <cmath>
#include <cstring>
#include <vector>

#include "Upright.h"
#include "up_analysis.h"
#include "up_fit.h"
#include "up_play.h"
#include "up_test.h"

using namespace up;

namespace {

constexpr float kFs = 44100.0f;

double db(double x) { return 20.0 * std::log10(x + 1e-30); }

constexpr double kOwnC4 = -9.99;   // db: grand's c4, velocity 1, the attack's own over 0 .. 10 ms against the string's, as measured

PlaySpec specAt(float midi, float vel, float attack, float sec = 1.0f, const Shape& s = grand()) {
    PlaySpec sp;
    sp.instrument = s;
    sp.midi = midi;
    sp.vel = vel;
    sp.sec = sec;
    sp.tuning.attack = attack;
    return sp;
}

size_t at(double sec) { return (size_t)std::lround(sec * kFs); }
double rmsOver(const std::vector<float>& x, double t0, double t1) { return rms(x.data() + at(t0), at(t1) - at(t0)); }

// a force through a board of its own at grand's plate and the note's bridge point
std::vector<float> throughBoard(const std::vector<float>& f, float midi) {
    const Shape& s = grand();
    Board b;
    b.init(kFs);
    b.setPlate(s[Channel::boardArea], s[Channel::boardLoss], s[Channel::open]);
    float w[kBoardModes];
    b.inputWeights(midi, w);
    std::vector<float> y(f.size());
    for (size_t a = 0; a < f.size(); a += kBlock) {
        const int n = (int)std::min((size_t)kBlock, f.size() - a);
        const float* fp[1] = {f.data() + a};
        const float* wp[1] = {w};
        b.render(fp, wp, 1, y.data() + a, n);
    }
    return y;
}

// a note at attack 1 and 0 as play renders it; the attack's own output and force; its parts apart (N, from the voice
// rendered a sample at a time) and each through the board
struct Pair {
    PlayRender one, zero;
    std::vector<float> own, ownForce;
    std::vector<float> blow, thud, clank;                     // the parts' forces
    std::vector<float> xBlow, xThud, xClank;                  // each through the board
};
Pair pairAt(float midi, float vel, float sec = 1.0f) {
    Pair p;
    p.one = renderPlay(specAt(midi, vel, 1.0f, sec));
    p.zero = renderPlay(specAt(midi, vel, 0.0f, sec));
    const size_t n = p.one.x.size();
    p.own.resize(n);
    p.ownForce.resize(n);
    for (size_t i = 0; i < n; ++i) {
        p.own[i] = p.one.x[i] - p.zero.x[i];
        p.ownForce[i] = p.one.bridge[i] - p.zero.bridge[i];
    }
    Voice v(1);
    v.setShape(grand());
    v.noteOn(midi, vel);
    p.blow.resize(n);
    p.thud.resize(n);
    p.clank.resize(n);
    for (size_t i = 0; i < n; ++i) {
        float b, q;
        v.render(&b, &q, 1);
        const AttackParts& a = v.attack().parts();
        p.blow[i] = a.blow;
        p.thud[i] = a.thud;
        p.clank[i] = a.clank;
    }
    p.xBlow = throughBoard(p.blow, midi);
    p.xThud = throughBoard(p.thud, midi);
    p.xClank = throughBoard(p.clank, midi);
    return p;
}

// the strongest transverse partial's level over the first 50 ms (the tracker's first frame) on a bridge force, db
double strongestPartial(const PlayRender& r, int* which = nullptr) {
    std::vector<double> expect;
    std::vector<int> slot;
    for (int k = 0; k < kMaxModes; ++k) {
        const double hz = (double)r.table.ratio[k] * r.f0;
        if (hz < kCeiling * kFs && (r.table.gain[k] != 0.0f || r.table.out[k] != 0.0f)) {
            expect.push_back(hz);
            slot.push_back(k);
        }
    }
    const std::vector<Tracked> t = trackPartials(r.bridge.data(), r.bridge.size(), kFs, expect);
    double best = -300.0;
    for (size_t i = 0; i < t.size(); ++i)
        if (t[i].levelDb > best) {
            best = t[i].levelDb;
            if (which) *which = slot[i] + 1;
        }
    return best;
}

// a record's level in the band hz +- 2 % over the first 50 ms (the band analyser's first frame), db
double bandDb(const std::vector<float>& x, double hz) {
    const Envelope e = BandAnalyser(x.data(), x.size(), kFs).envelope(0.98 * hz, 1.02 * hz);
    return e.db.empty() ? -300.0 : e.db[0];
}

// the output's band under 150 hz over 5 .. 40 ms: the mean power of seven 5 ms frames, db
double lowBandDb(const std::vector<float>& x) {
    const Envelope e = BandAnalyser(x.data(), x.size(), kFs).envelope(1.0, 150.0, 0.005);
    double p = 0.0;
    for (size_t i = 1; i < 8 && i < e.db.size(); ++i) p += std::pow(10.0, e.db[i] / 10.0);
    return 10.0 * std::log10(p / 7.0 + 1e-30);
}

// the knock over the first 10 ms against the string's own: the blow's share, the attack's own; the string's rms
struct Knock {
    double blow, own, string;
};
Knock knockOf(const Pair& p) {
    const double s = rmsOver(p.zero.x, 0.0, 0.01);
    return {db(rmsOver(p.xBlow, 0.0, 0.01) / s), db(rmsOver(p.own, 0.0, 0.01) / s), s};
}

}  // namespace

// at attack 0 the attack never runs and its long bank stays at zero; at 0 and 1 the strings the same bits every
// block (the attack feeds nothing back); the bridge force differs from the strike's first sample on; at 0 its first
// 1.5 ms printed: the modal string's sum and its stiff upper partials, which run ahead of the fundamental's wave
UP_TEST(attack_zero_runs_nothing) {
    Voice a(1), z(1);
    Tuning t1, t0;
    t0.attack = 0.0f;
    a.setTuning(t1);
    z.setTuning(t0);
    a.setShape(grand());
    z.setShape(grand());
    a.noteOn(60.0f, 1.0f);
    z.noteOn(60.0f, 1.0f);
    const int blocks = (int)(0.5f * kFs) / kBlock;
    std::vector<float> ba((size_t)blocks * kBlock), bz(ba.size());
    bool quiet = true, same = true, ran = false;
    for (int b = 0; b < blocks; ++b) {
        a.render(ba.data() + (size_t)b * kBlock, nullptr, kBlock);
        z.render(bz.data() + (size_t)b * kBlock, nullptr, kBlock);
        const State sa = a.state(), sz = z.state();
        for (int k = 0; k < kLongModes; ++k) quiet = quiet && sz.ampLong[k] == 0.0f;
        quiet = quiet && !z.attack().running();
        ran = ran || a.attack().running();
        same = same && std::memcmp(sa.amp, sz.amp, sizeof sa.amp) == 0 && std::memcmp(sa.amp2, sz.amp2, sizeof sa.amp2) == 0 &&
               std::memcmp(sa.amp3, sz.amp3, sizeof sa.amp3) == 0 && sa.hammerPeakForce == sz.hammerPeakForce;
    }
    CHECK_MSG(quiet, "at attack 0 the attack ran or its long bank rang");
    CHECK(ran);
    CHECK_MSG(same, "the strings differ between attack 0 and 1");
    CHECK(ba[1] != bz[1] && std::isfinite(ba[1]));
    const size_t half = at(0.0005);
    printf("    grand c4, velocity 1, 0.5 s: at attack 0 the attack never ran; the strings the same bits at 0 and 1 every block; at attack 0 the "
           "bridge force peaks %.3f N over 0 .. 0.5 ms, %.3f over 0.5 .. 1, %.3f over 1 .. 1.5 (the string's own; %.1f N later)\n",
           peakAbs(bz.data(), half), peakAbs(bz.data() + half, half), peakAbs(bz.data() + 2 * half, half), peakAbs(bz.data(), bz.size()));
}

// the knock at grand's c4, velocity 1, over the first 10 ms against the string's own: the blow's share -10 +- 1.5 db
// (kKnockForce's calibration); the attack's own (the blow, the thud from 2 ms, the clank) -9.99 +- 1.5, as measured;
// the parts' sum against the attack's own force within 1e-4 N; from velocity 1 to 0.5 the knock falls at least as far
// as the string's own (12.63 db against 12.44); c2 and c6 at both velocities printed
UP_TEST(attack_knock) {
    for (float midi : {60.0f, 36.0f, 84.0f}) {
        Knock k[2];
        for (int i = 0; i < 2; ++i) {
            const Pair p = pairAt(midi, i ? 0.5f : 1.0f, 0.5f);
            k[i] = knockOf(p);
            if (midi == 60.0f && i == 0) {
                double worst = 0.0;
                for (size_t j = 0; j < p.ownForce.size(); ++j)
                    worst = std::fmax(worst, std::fabs((double)p.ownForce[j] - ((double)p.blow[j] + p.thud[j] + p.clank[j])));
                CHECK_MSG(worst < 1e-4, "the parts' sum against the attack's own force: %.3g N", worst);
                CHECK_MSG(std::fabs(k[0].blow + 10.0) <= 1.5, "c4: the blow's share %+.2f db", k[0].blow);
                CHECK_MSG(std::fabs(k[0].own - kOwnC4) <= 1.5, "c4: the attack's own over 0 .. 10 ms %+.2f db against the string's, held at %+.2f",
                          k[0].own, kOwnC4);
            }
        }
        const double knockFall = db(std::pow(10.0, k[0].blow / 20.0) * k[0].string / (std::pow(10.0, k[1].blow / 20.0) * k[1].string)),
                     stringFall = db(k[0].string / k[1].string);
        if (midi == 60.0f)
            CHECK_MSG(knockFall >= stringFall, "c4 from velocity 1 to 0.5: the knock falls %.2f db, the string's own %.2f", knockFall, stringFall);
        printf("    %s over 0 .. 10 ms against the string's own: at velocity 1 the blow's share %+.2f db, the attack's own %+.2f; at 0.5 %+.2f, "
               "%+.2f; from 1 to 0.5 the knock falls %.2f db, the string %.2f\n",
               noteName(midi).c_str(), k[0].blow, k[0].own, k[1].blow, k[1].own, knockFall, stringFall);
    }
    printf("    c4, velocity 1, held at: the blow's share -10.00 +- 1.5 db, the attack's own %+.2f +- 1.5\n", kOwnC4);
}

// the thud: the output's band under 150 hz over 5 .. 40 ms at grand's c4, velocity 1, 4 +- 1 db over attack 0 (the
// thud alone, and the blow's share alone, against the string's own printed: the two arrive 2 ms apart and partly
// cancel in the plate's lowest modes); c2 printed
UP_TEST(attack_thud) {
    for (float midi : {60.0f, 36.0f}) {
        const Pair p = pairAt(midi, 1.0f);
        const double l1 = lowBandDb(p.one.x), l0 = lowBandDb(p.zero.x), lt = lowBandDb(p.xThud), lb = lowBandDb(p.xBlow);
        if (midi == 60.0f) CHECK_MSG(std::fabs(l1 - l0 - 4.0) <= 1.0, "the thud at c4: under 150 hz over 5 .. 40 ms %+.2f db at attack 1 against 0", l1 - l0);
        printf("    %s, velocity 1: the output under 150 hz over 5 .. 40 ms %.2f db at attack 1, %.2f at 0 (%+.2f); the thud alone %+.2f against the "
               "string's own, the blow's share alone %+.2f\n",
               noteName(midi).c_str(), l1, l0, l1 - l0, lt - l0, lb - l0);
    }
}

// the clank's pitch: the attack's own force over 24 .. 84 ms (after the thud's end at 12 ms: the clank alone) peaks
// within 2 % of the rank's longHz, within 2 % at c2 and c4, a0 and c6 printed; c7's modes sit over the ceiling: its
// long bank never on, its amplitudes zero
UP_TEST(attack_clank_pitch) {
    for (float midi : {21.0f, 36.0f, 60.0f, 84.0f}) {
        const PlayRender one = renderPlay(specAt(midi, 1.0f, 1.0f)), zero = renderPlay(specAt(midi, 1.0f, 0.0f));
        std::vector<float> own(one.bridge.size());
        for (size_t i = 0; i < own.size(); ++i) own[i] = one.bridge[i] - zero.bridge[i];
        const double fl = one.struck.rank.longHz;
        const size_t from = at(0.024);
        const std::vector<double> mag = magnitude(own.data() + from, at(0.06), (size_t)1 << 18, Window::hann);
        const Peak pk = peakNear(mag, kFs, fl, 1200.0 * std::log2(1.02));
        const double off = pk.hz / fl - 1.0;
        if (midi == 36.0f || midi == 60.0f)
            CHECK_MSG(std::fabs(off) <= 0.02, "%s: the clank at %.2f hz against the rank's %.2f (%+.2f %%)", noteName(midi).c_str(), pk.hz, fl, 100.0 * off);
        printf("    %s: the rank's c_L / (2 L) %.2f hz; the attack's own force over 24 .. 84 ms peaks at %.2f hz (%+.3f %%, %.1f db)\n",
               noteName(midi).c_str(), fl, pk.hz, 100.0 * off, pk.db);
    }
    Voice v(1);
    v.setShape(grand());
    v.noteOn(96.0f, 1.0f);
    bool off = true;
    float b[kBlock];
    for (int k = 0; k < 40; ++k) {
        v.render(b, nullptr, kBlock);
        off = off && v.attack().bank().count() == 0;
        for (int j = 0; j < kLongModes; ++j) off = off && v.state().ampLong[j] == 0.0f;
    }
    CHECK_MSG(off, "c7: a longitudinal mode on (longHz %.0f hz)", v.state().rank.longHz);
    printf("    c7: the rank's c_L / (2 L) %.0f hz, over the ceiling (%.0f hz): the long bank never on\n", v.state().rank.longHz, kCeiling * kFs);
}

// the clank's level: grand's a0, velocity 1, its own output through the board over the first 50 ms 10 +- 1 db under
// the note's own (kLongShare); at a0, c2, c4, c6 and velocities 1 and 0.5 against the note and, its own force in
// f_L,1 +- 2 % over the first 50 ms, against the strongest partial on the bridge force at attack 0, printed; the
// square law at c2: from velocity 0.5 to 1 the clank's band moves at least 1.7 x the strongest partial
UP_TEST(attack_clank_level) {
    for (float midi : {21.0f, 36.0f, 60.0f, 84.0f}) {
        double note[2], part[2], band[2];
        for (int i = 0; i < 2; ++i) {
            const Pair p = pairAt(midi, i ? 0.5f : 1.0f);
            note[i] = db(rmsOver(p.xClank, 0.0, 0.05) / rmsOver(p.zero.x, 0.0, 0.05));
            band[i] = bandDb(p.clank, p.one.struck.rank.longHz);
            part[i] = strongestPartial(p.zero);
        }
        const double moveC = band[0] - band[1], moveP = part[0] - part[1];
        if (midi == 21.0f) CHECK_MSG(std::fabs(note[0] + 10.0) <= 1.0, "a0: the clank %+.2f db against the note over the first 50 ms", note[0]);
        if (midi == 36.0f)
            CHECK_MSG(moveC >= 1.7 * moveP, "c2: the clank moves %.2f db from velocity 0.5 to 1, the strongest partial %.2f (%.2f x)", moveC, moveP,
                      moveC / moveP);
        printf("    %s: the clank against the note over the first 50 ms %+.2f db at velocity 1, %+.2f at 0.5; against the strongest partial %+.2f, "
               "%+.2f; from 0.5 to 1 it moves %.2f db, the partial %.2f (%.2f x)\n",
               noteName(midi).c_str(), note[0], note[1], band[0] - part[0], band[1] - part[1], moveC, moveP, moveC / moveP);
    }
}

// every acoustic preset's bottom note at velocity 1, 2 s as play renders it: the peak under 1.0
UP_TEST(attack_bottom_notes) {
    for (const char* name : {"grand", "concertGrand", "upright", "spinet", "honkyTonk", "fortepiano"}) {
        const PresetInfo* p = presetByName(name);
        double peak[2];
        for (int a = 0; a < 2; ++a) {
            const PlayRender r = renderPlay(specAt((float)p->lo, 1.0f, a ? 0.0f : 1.0f, 2.0f, p->shape()));
            peak[a] = peakAbs(r.x.data(), r.x.size());
        }
        CHECK_MSG(peak[0] < 1.0, "%s at %s peaks %.3f", name, noteName(p->lo).c_str(), peak[0]);
        printf("    %-13s at %s, velocity 1: the peak %.3f at attack 1, %.3f at 0\n", name, noteName(p->lo).c_str(), peak[0], peak[1]);
    }
}

// the tine side is untouched: rhodes at e4 and wurlitzer at a3, velocity 1, 2 s: the same bytes at attack 0 and 1
UP_TEST(attack_tine_side_untouched) {
    const Shape& rhodes = anchors()[1].s;
    const Shape& wurli = anchors()[2].s;
    for (int i = 0; i < 2; ++i) {
        const float midi = i ? 57.0f : 64.0f;
        const PlayRender a = renderPlay(specAt(midi, 1.0f, 1.0f, 2.0f, i ? wurli : rhodes)), z = renderPlay(specAt(midi, 1.0f, 0.0f, 2.0f, i ? wurli : rhodes));
        const bool same = std::memcmp(a.x.data(), z.x.data(), a.x.size() * sizeof(float)) == 0 &&
                          std::memcmp(a.pair.data(), z.pair.data(), a.pair.size() * sizeof(float)) == 0;
        CHECK_MSG(same && rms(a.x.data(), a.x.size()) > 0.0, "%s at %s: attack 0 and 1 differ", i ? "wurlitzer" : "rhodes", noteName(midi).c_str());
    }
    printf("    rhodes at e4 and wurlitzer at a3, 2 s: the same bytes at attack 0 and 1\n");
}

// a chord of four on a pool at attack 1 (grand, c3 e3 g3 c4 at velocity 1, 3 s): finite, its peak under the
// wrappers' clamp; the same at attack 0 printed
UP_TEST(attack_pool_chord) {
    double peak[2] = {0.0, 0.0};
    bool finite = true;
    for (int i = 0; i < 2; ++i) {
        Pool<4> pool(1);
        Tuning t;
        t.attack = i ? 0.0f : 1.0f;
        pool.setTuning(t);
        pool.setShape(grand());
        for (float key : {48.0f, 52.0f, 55.0f, 60.0f}) pool.noteOn(key, 1.0f);
        std::vector<float> x(at(3.0));
        pool.render(x.data(), (int)x.size());
        finite = finite && allFinite(x.data(), x.size());
        peak[i] = peakAbs(x.data(), x.size());
    }
    CHECK(finite);
    CHECK_MSG(peak[0] < 1.0, "the chord at attack 1 peaks %.3f", peak[0]);
    printf("    c3 e3 g3 c4 at velocity 1 on a pool of four, 3 s: finite; the peak %.3f at attack 1, %.3f at 0\n", peak[0], peak[1]);
}

// the cost on this host: a ringing grand c4's voice block over its first 2 s at attack 1 against attack 0 (the long
// bank's turn and sum a sample; the blow and the thud are a strike's first 12 ms), and the attack's own block
// (setBlock and 128 steps); printed
UP_TEST(attack_cost) {
    using clock = std::chrono::steady_clock;
    volatile float sink = 0.0f;
    auto voiceUs = [&](float attack) {
        double best = 1e30;
        for (int rep = 0; rep < 5; ++rep) {
            Voice v(1);
            Tuning t;
            t.attack = attack;
            v.setTuning(t);
            v.setShape(grand());
            v.noteOn(60.0f, 1.0f);
            float b[kBlock];
            for (int k = 0; k < 8; ++k) v.render(b, nullptr, kBlock);
            const int n = 680;
            const auto t0 = clock::now();
            for (int k = 0; k < n; ++k) {
                v.render(b, nullptr, kBlock);
                sink = sink + b[0];
            }
            best = std::fmin(best, std::chrono::duration<double, std::micro>(clock::now() - t0).count() / n);
        }
        return best;
    };
    const double on = voiceUs(1.0f), off = voiceUs(0.0f);
    const RankState r = rankAt(grand(), 60.0f);
    Attack a;
    a.init(kFs);
    const int n = 20000;
    const auto t0 = clock::now();
    for (int k = 0; k < n; ++k) {
        a.setBlock(r, kCeiling * kFs, 1.0f);
        if (k % 400 == 0) a.strike(0.02f);
        for (int i = 0; i < kBlock; ++i) sink = sink + a.step();
    }
    const double own = std::chrono::duration<double, std::micro>(clock::now() - t0).count() / n;
    printf("    on this host a ringing grand c4's voice block (its first 2 s): %.2f us at attack 1, %.2f at 0: the attack %.2f us (%.1f %%); the "
           "attack's own block (setBlock and 128 samples, %d modes) %.2f us%s\n",
           on, off, on - off, 100.0 * (on - off) / off, kLongModes, own, sink == 1e30f ? " " : "");
}
