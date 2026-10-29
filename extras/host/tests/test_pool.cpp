// upright — tests/test_pool.cpp  (plan §13; the tests' "the pool" bullet)
//
// Pool<4> of grand: five keys steal the quietest (not the oldest), and the
// stolen voice's share of the output (its bridge force through the board,
// its pickup) is continuous through its fade (the click measure under the
// stage 1 bound); the same key struck twice meets the moving string on one
// voice (finite, its onset within 2 db of a fresh strike's); noteOff lands
// the damper and the pedal holds it; the pool's output is its voices' pairs
// through one board, summed by hand, bit for bit; two pools share nothing;
// a pool-wide shape reaches a later note and moves the board; the same calls
// give the same bytes at render sizes 1, 128 and 1000; a stolen voice's
// waiting note lands whole with the pool's pedal. and the umbrella header
// compiles on the host, its teensy wrappers empty.
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>

#include "Upright.h"
#include "up_analysis.h"
#include "up_pool_access.h"
#include "up_shapes.h"
#include "up_test.h"

using namespace up;

namespace {

constexpr float kFs = 44100.0f;

int blocks(float sec) { return (int)(sec * kFs) / kBlock; }

// a pool's output, and one voice's share (its bridge force and pickup at its level), appended
struct Record {
    std::vector<float> out, bridge, pickup;
};
void renderInto(PoolBase& p, Record& r, int n, int which = 0) {
    const size_t at = r.out.size();
    r.out.resize(at + (size_t)n);
    r.bridge.resize(at + (size_t)n);
    r.pickup.resize(at + (size_t)n);
    PoolTestAccess::render(p, r.out.data() + at, n, which, r.bridge.data() + at, r.pickup.data() + at);
}

// the slot sounding a key (held or released), or -1
int slotOf(const PoolBase& p, float key) {
    for (int i = 0; i < p.voices(); ++i) {
        const PoolSlot& s = p.slot(i);
        if ((s.use == PoolSlot::Use::held || s.use == PoolSlot::Use::released) && s.key == key) return i;
    }
    return -1;
}

// the top partial a voice rings, hz: its main string's partials with an amplitude, as its state holds them
double topHz(const Voice& v) {
    const State st = v.state();
    double top = 0.0;
    for (int k = 0; k < kMaxModes; ++k) {
        const double hz = (double)st.shape[ratioChannel(k)] * st.f0;
        if (st.amp[k] > 0.0f && hz > top) top = hz;
    }
    return top;
}

// a voice's share of the pool's output: its bridge force through a board of the share's own at the plate
// and the bridge point, its pickup added
std::vector<float> shareOf(const Record& r, const Shape& body, float point) {
    Board b;
    b.init(kFs);
    b.setPlate(body[Channel::boardArea], body[Channel::boardLoss], body[Channel::open]);
    float w[kBoardModes];
    b.inputWeights(point, w);
    std::vector<float> y(r.bridge.size());
    for (size_t a = 0; a < y.size(); a += kBlock) {
        const int n = (int)std::min((size_t)kBlock, y.size() - a);
        const float* f[1] = {r.bridge.data() + a};
        const float* ww[1] = {w};
        b.render(f, ww, 1, y.data() + a, n);
    }
    for (size_t i = 0; i < y.size(); ++i) y[i] += r.pickup[i];
    return y;
}

double db(double x) { return 20.0 * std::log10(x + 1e-30); }

}  // namespace

// grand on a pool of four: c4 at velocity 1, e4 at 0.15 (quiet), g4 and c5 at
// 1, a quarter second apart; at 1.25 s a fifth key (e5) steals the quietest
// sounding voice, e4's, not the oldest (c4's). the stolen voice's share
// (its bridge force at the level it is summed at, through a board of its
// own at the pool's plate, and its pickup) from 0.1 s before the steal to
// the sample before its new note: the click measure under the stage 1 bound
// for its top partial, the same window without the steal printed beside;
// its force fades to under 1 % and to zero across one block, and the new
// note lands on it at the next block
UP_TEST(pool_steals_the_quietest_without_a_cut) {
    const float keys[4] = {60.0f, 64.0f, 67.0f, 72.0f}, vels[4] = {1.0f, 0.15f, 1.0f, 1.0f};
    const int quarter = blocks(0.25f) * kBlock, lead = blocks(0.1f) * kBlock;
    float energy[4] = {};
    int quiet = -1, oldest = -1, landed = -1, stealAt = 0;
    double top = 0.0;
    float lastLevel = 1.0f;
    auto run = [&](bool steal) {
        Pool<4> pool(3);
        pool.setShape(grand());
        Record r;
        for (int k = 0; k < 4; ++k) {
            pool.noteOn(keys[k], vels[k]);
            renderInto(pool, r, quarter, 1);
        }
        renderInto(pool, r, 2 * quarter, 1);
        if (steal) {
            CHECK(pool.sounding() == 4);
            quiet = 0;
            for (int i = 0; i < 4; ++i) {
                energy[i] = pool.voice(i).energy();
                if (energy[i] < energy[quiet]) quiet = i;
            }
            oldest = slotOf(pool, 60.0f);
            CHECK(quiet == slotOf(pool, 64.0f) && quiet == 1 && quiet != oldest);
            top = topHz(pool.voice(quiet));
            stealAt = (int)r.out.size();
            pool.noteOn(76.0f, 1.0f);
            CHECK(pool.slot(quiet).use == PoolSlot::Use::stolen && pool.slot(quiet).waiting && pool.slot(quiet).nextKey == 76.0f);
            CHECK(slotOf(pool, 60.0f) == oldest);
        }
        const int start = (int)r.out.size();
        for (int b = 0; b < 200; ++b) {
            renderInto(pool, r, kBlock, 1);
            if (steal && slotOf(pool, 76.0f) == quiet) {
                landed = b;
                r.out.resize(r.out.size() - kBlock);
                r.bridge.resize(r.out.size());
                r.pickup.resize(r.out.size());
                break;
            }
            if (steal) lastLevel = pool.slot(quiet).levelT;
            if (!steal && (int)r.out.size() - start >= (landed + 1) * kBlock) break;
        }
        return r;
    };
    const Record stolen = run(true), rung = run(false);
    CHECK_MSG(landed > 0, "the new note never landed on the stolen voice");
    const Shape& body = grand();
    const std::vector<float> a = shareOf(stolen, body, 64.0f), b = shareOf(rung, body, 64.0f);
    const size_t from = (size_t)(stealAt - lead);
    const double click = clickRatio(a.data() + from, a.size() - from, kFs), ring = clickRatio(b.data() + from, a.size() - from, kFs);
    const double bound = clickBound(top, kFs);
    CHECK_MSG(click < bound, "the stolen voice's share through the steal: the click measure %.4f, the bound %.4f (top %.0f hz)", click, bound, top);
    const double before = peakAbs(stolen.bridge.data() + from, (size_t)lead), tail = std::fabs((double)stolen.bridge.back());
    CHECK_MSG(lastLevel == 0.0f && tail < 1e-6 * before, "the fade ends at level %g, the force %.3g against %.3g", lastLevel, tail, before);
    CHECK(allFinite(stolen.out.data(), stolen.out.size()));
    printf("    the four voices' energies at the steal (mm^2): %.3g (c4, the oldest), %.3g (e4), %.3g (g4), %.3g (c5): e4's, the quietest, stolen\n",
           energy[0], energy[1], energy[2], energy[3]);
    printf("    its fade: %d blocks (%.1f ms) to zero, the new note on it the block after; its share through the board: the click measure %.4f "
           "through the steal, %.4f ringing on unstolen, the bound %.4f (its top partial %.0f hz)\n",
           landed, 1000.0 * landed * kBlock / kFs, click, ring, bound, top);
}

// the same key struck twice: grand's c4 at 0.8 and again at 0.8 a second
// later, on one voice: the render finite; the second strike's onset (the
// output's rms over the 50 ms after it) within 2 db of a fresh strike's (a
// pool struck once, its first 50 ms); the voice's energy after the first
// strike's block, before the second and after it printed beside a fresh
// strike's (a strike on a moving string: not the sum of two)
UP_TEST(pool_restrike_meets_the_moving_string) {
    Pool<4> pool(5);
    pool.setShape(grand());
    Record r;
    pool.noteOn(60.0f, 0.8f);
    renderInto(pool, r, kBlock);
    const float first = pool.voice(0).energy();
    const int second = blocks(1.0f) * kBlock;
    renderInto(pool, r, second - kBlock);
    const float before = pool.voice(0).energy();
    pool.noteOn(60.0f, 0.8f);
    renderInto(pool, r, kBlock);
    const float after = pool.voice(0).energy();
    CHECK(pool.sounding() == 1 && slotOf(pool, 60.0f) == 0);
    renderInto(pool, r, blocks(1.0f) * kBlock);
    CHECK(allFinite(r.out.data(), r.out.size()));
    Pool<4> fresh(5);
    fresh.setShape(grand());
    Record f;
    fresh.noteOn(60.0f, 0.8f);
    renderInto(fresh, f, blocks(0.1f) * kBlock);
    const size_t w = (size_t)(0.05 * kFs);
    const double again = rms(r.out.data() + second, w), once = rms(f.out.data(), w);
    CHECK_MSG(std::fabs(db(again / once)) <= 2.0, "the second strike's onset %.4f against a fresh strike's %.4f (%+.2f db)", again, once,
              db(again / once));
    printf("    c4 at 0.8 struck again a second later on one voice: its onset %+.2f db against a fresh strike's; the energy (mm^2) %.4g after the "
           "first strike's block, %.4g before the second, %.4g after it (a fresh strike %.4g)\n",
           db(again / once), first, before, after, first);
}

// noteOff lands the damper: grand's c4 at velocity 1 released at 1 s, its
// voice's bridge force falls 60 db under its peak within 1.5 x the damper's
// t60 (6.907755 / dampRate) of the release; under the pool's pedal (1, from
// the start) the same release rings on (not 60 db under by 3 s), and the
// pedal's fall at 2 s lands the damper within the pedal's ease and 1.5 x the
// damper's t60
UP_TEST(pool_note_off_lands_the_damper) {
    const double t60 = 6.907755 / grand()[Channel::dampRate], ease = -std::log(0.05) / Tuning().ratePedal;
    auto run = [&](bool pedal, bool fall, float sec) {
        Pool<4> pool(6);
        pool.setShape(grand());
        if (pedal) pool.pedal(1.0f);
        Record r;
        pool.noteOn(60.0f, 1.0f);
        renderInto(pool, r, blocks(1.0f) * kBlock);
        pool.noteOff(60.0f);
        CHECK(pool.slot(0).use == PoolSlot::Use::released);
        renderInto(pool, r, blocks(1.0f) * kBlock);
        if (fall) pool.pedal(0.0f);
        renderInto(pool, r, blocks(sec - 2.0f) * kBlock);
        return r.bridge;
    };
    auto under = [&](const std::vector<float>& x, double from) {
        const double peak = peakAbs(x.data(), x.size());
        const size_t w = (size_t)(0.01 * kFs);
        double at = -1.0;
        for (size_t a = (size_t)(from * kFs); a + w <= x.size(); a += w) {
            const bool quiet = db(peakAbs(x.data() + a, w) / peak) < -60.0;
            if (quiet && at < 0.0) at = (double)a / kFs;
            if (!quiet) at = -1.0;
        }
        return at;
    };
    const double one = (double)(blocks(1.0f) * kBlock) / kFs, two = 2.0 * one;
    const double released = under(run(false, false, 3.0f), one) - one;
    const double held = under(run(true, false, 3.0f), one);
    const double fallen = under(run(true, true, 4.0f), two) - two;
    CHECK_MSG(released > 0.0 && released <= 1.5 * t60, "released: 60 db under %.3f s later (1.5 x %.3f s)", released, t60);
    CHECK_MSG(held < 0.0, "under the pedal: 60 db under at %.3f s", held);
    CHECK_MSG(fallen > 0.0 && fallen <= ease + 1.5 * t60, "the pedal's fall: 60 db under %.3f s later (%.3f + 1.5 x %.3f s)", fallen, ease, t60);
    printf("    released at 1 s: the voice's force 60 db under its peak %.3f s later (the damper's t60 %.3f s); under the pedal it rings (%s by 3 s); "
           "the pedal's fall at 2 s lands it %.3f s later\n",
           released, t60, held < 0.0 ? "not 60 db under" : "60 db under", fallen);
}

// the pool's output is its voices' pairs through one board, summed by hand:
// four keys, a release, the pedal down and up, a pool-wide shape that
// moves the plate, a key's own voice again, no steal; each voice driven alone
// the same way (its seed the pool's for it, the same writes: at a landing
// note on the voice the pool chose, the pool's pedal, then the note; a voice
// whose key was released and has slept is free, and may take a later key),
// its bridge force through one board
// of the test's (the plate as the pool writes it, each voice's weights at
// its bridge point when its note or the plate moved), its pickup added in
// voice order: the same bytes; and the window's share of one voice is that
// voice's force alone, bit for bit
UP_TEST(pool_is_its_voices_through_one_board) {
    const uint32_t seed = 9;
    Pool<4> pool(seed);
    Voice v[4];
    for (int i = 0; i < 4; ++i) {
        v[i].reseed(PoolBase::seedOf(seed, i));
        v[i].setTuning(Tuning());
    }
    Board board;
    board.init(kFs);
    float w[4][kBoardModes], point[4] = {-1.0f, -1.0f, -1.0f, -1.0f}, pedal = 0.0f;
    // upright's piano with the generator at -0.6: its board moves the plate, both outputs live
    const Shape other = with(upright(), {{Channel::generator, -0.6f}, {Channel::tonebar, 0.4f}});
    auto write = [&](const Shape& s) {
        pool.setShape(s);
        for (Voice& x : v) x.setShape(s);
        const float a = board.table().a, b = board.table().b;
        board.setPlate(s[Channel::boardArea], s[Channel::boardLoss], s[Channel::open]);
        if (board.table().a != a || board.table().b != b)
            for (float& p : point) p = -1.0f;
    };
    // a note on the pool, and on the voice the pool chose for it, alone
    int landed[4] = {-1, -1, -1, -1}, keys = 0;
    auto land = [&](float key, float vel) {
        pool.noteOn(key, vel);
        const int i = slotOf(pool, key);
        CHECK(i >= 0 && pool.slot(i).struck);
        landed[keys++] = i;
        v[i].setPedal(pedal);
        v[i].noteOn(key, vel);
    };
    auto pedals = [&](float p) {
        pool.pedal(p);
        pedal = p;
        for (Voice& x : v) x.setPedal(p);
    };
    const int span = 64 * kBlock, total = 9 * span;
    Record r;
    std::vector<float> sum((size_t)total), alone2((size_t)total);
    for (int step = 0; step < 9; ++step) {
        switch (step) {
            case 0: write(grand()); land(48.0f, 1.0f); break;
            case 1: land(55.0f, 0.7f); break;
            case 2: land(60.0f, 0.9f); pool.noteOff(48.0f); v[0].noteOff(); break;
            case 3: pedals(0.6f); break;
            case 4: write(other); break;
            case 5: land(64.0f, 1.0f); break;
            case 6: pedals(0.0f); break;
            case 7: pool.noteOn(60.0f, 0.5f); v[2].noteOn(60.0f, 0.5f); break;      // the key's own voice again
            default: break;
        }
        renderInto(pool, r, span, 2);
        for (int a = 0; a < span; a += kBlock) {
            float bridge[4][kBlock], pick[4][kBlock], picks[kBlock], out[kBlock];
            for (int i = 0; i < 4; ++i) {
                v[i].render(bridge[i], pick[i], kBlock);
                if (v[i].bridgePoint() != point[i]) {
                    point[i] = v[i].bridgePoint();
                    board.inputWeights(point[i], w[i]);
                }
            }
            for (int j = 0; j < kBlock; ++j) picks[j] = 0.0f;
            for (int i = 0; i < 4; ++i)
                for (int j = 0; j < kBlock; ++j) picks[j] += pick[i][j];
            const float* f[4] = {bridge[0], bridge[1], bridge[2], bridge[3]};
            const float* ww[4] = {w[0], w[1], w[2], w[3]};
            board.render(f, ww, 4, out, kBlock);
            for (int j = 0; j < kBlock; ++j) {
                sum[(size_t)(step * span + a + j)] = out[j] + picks[j];
                alone2[(size_t)(step * span + a + j)] = bridge[2][j];
            }
        }
    }
    CHECK(landed[0] == 0 && landed[1] == 1 && landed[2] == 2 && keys == 4);
    CHECK(std::memcmp(r.out.data(), sum.data(), sum.size() * sizeof(float)) == 0);
    CHECK(std::memcmp(r.bridge.data(), alone2.data(), alone2.size() * sizeof(float)) == 0);
    CHECK(rms(r.out.data(), r.out.size()) > 0.0 && rms(r.pickup.data(), r.pickup.size()) > 0.0);
    printf("    four keys on voices %d, %d, %d, %d over %.2f s (a release, the pedal, a shape moving the plate, a re-strike; both outputs live): "
           "the pool's output and its voices through one board by hand are the same bytes, and the window's share is its voice's force alone\n",
           landed[0], landed[1], landed[2], landed[3], (double)total / kFs);
}

// two pools share nothing: one renders the same bytes whether or not a
// second plays beside it, struck and written between its blocks
UP_TEST(pool_two_pools_independent) {
    auto run = [](Pool<4>* other) {
        Pool<4> p(11);
        Record o, x;
        for (int s = 0; s < 6; ++s) {
            p.noteOn(55.0f + 3.0f * (float)s, 1.0f);
            if (other) {
                other->noteOn(40.0f + (float)s, 1.0f);
                other->setShape(with(concertGrand(), {{Channel::generator, -0.5f}, {Channel::tonebar, 0.5f}}));
                other->pedal(0.8f);
            }
            renderInto(p, o, 6000);
            if (other) renderInto(*other, x, 6000);
        }
        return o.out;
    };
    Pool<4> beside(11);
    const std::vector<float> a = run(nullptr), b = run(&beside);
    CHECK(std::memcmp(a.data(), b.data(), a.size() * sizeof(float)) == 0);
    CHECK(beside.sounding() > 0 && rms(a.data(), a.size()) > 0.0);
}

// pool-wide writes reach later notes and the board: a shape written after
// c4 sounds is c4's voice's instrument now and the instrument of a note
// struck after it, and the board's plate is the shape's body; a partial
// written by hand pool-wide stands on a later note until the instrument is
// written again
UP_TEST(pool_writes_reach_later_notes_and_the_board) {
    Pool<4> pool(12);
    Record r;
    pool.noteOn(60.0f, 1.0f);
    renderInto(pool, r, kBlock);
    const float hz0 = pool.board().modeInfo(0).hz;
    const Shape want = with(concertGrand(), {{Channel::strings, 2.0f}, {Channel::generator, -0.7f}, {Channel::tonebar, 0.3f}});
    pool.setShape(want);
    pool.noteOn(67.0f, 0.8f);
    renderInto(pool, r, kBlock);
    const int c4 = slotOf(pool, 60.0f), g4 = slotOf(pool, 67.0f);
    CHECK(c4 >= 0 && g4 >= 0 && c4 != g4);
    bool same = true;
    for (int i : {c4, g4}) {
        const Shape s = pool.voice(i).shape();
        for (int c = 0; c < kInstrumentChannels; ++c) same = same && s.v[c] == want.v[c];
    }
    CHECK(same);
    const BoardTable t = boardOf(want[Channel::boardArea], want[Channel::boardLoss], want[Channel::open]);
    CHECK_MSG(pool.board().modeInfo(0).hz == t.mode[0].hz && pool.board().table().a == t.a && hz0 != t.mode[0].hz,
              "the board's first mode %.2f hz, the shape's %.2f (before %.2f)", pool.board().modeInfo(0).hz, t.mode[0].hz, hz0);
    pool.setPartial(4, 5.31f, 2.0f, 0.4f, -3.0f);
    pool.noteOn(72.0f, 1.0f);
    renderInto(pool, r, kBlock);
    const int c5 = slotOf(pool, 72.0f);
    CHECK(c5 >= 0 && pool.voice(c5).shape()[ratioChannel(4)] == 5.31f && pool.voice(c5).state().shape[ratioChannel(4)] == 5.31f);
    pool.setShape(grand());
    renderInto(pool, r, kBlock);
    CHECK(pool.voice(c5).shape()[ratioChannel(4)] != 5.31f && pool.board().modeInfo(0).hz == hz0);
    printf("    a shape written under c4 is c4's and g4's (struck after); the board's first mode %.1f -> %.1f hz with its body; the 5th partial by "
           "hand at 5.31 stood on c5, struck after; the instrument written again took it back\n",
           hz0, t.mode[0].hz);
}

// the same calls at the same samples give the same bytes whatever size the
// pool is rendered in (1, 128, 1000; a render split where a call falls):
// notes, a steal, a release, the pedal, a shape, a tuning, a partial by hand
UP_TEST(pool_same_bytes_at_any_render_size) {
    struct Call { int at; int what; };
    const int total = (int)(4.0f * kFs);
    const Call calls[] = {{0, 0}, {3001, 1}, {9000, 2}, {15013, 3}, {22050, 4}, {40000, 5}, {61111, 6}, {90000, 7}, {120000, 8}};
    auto run = [&](int size) {
        Pool<4> p(13);
        std::vector<float> x((size_t)total);
        int at = 0;
        size_t c = 0;
        while (at < total) {
            while (c < sizeof calls / sizeof calls[0] && calls[c].at == at) {
                switch (calls[c].what) {
                    case 0: p.setShape(grand()); p.noteOn(48.0f, 1.0f); p.noteOn(52.0f, 0.6f); break;
                    case 1: p.noteOn(55.0f, 0.9f); p.noteOn(60.0f, 0.3f); break;
                    case 2: p.noteOn(64.0f, 1.0f); break;                                  // a fifth key: a steal
                    case 3: p.noteOff(48.0f); p.pedal(0.5f); break;
                    case 4: p.setShape(with(spinet(), {{Channel::generator, -0.3f}, {Channel::tonebar, 0.7f}})); p.noteOn(67.0f, 0.8f); break;
                    case 5: { Tuning t; t.stealRate = 90.0f; p.setTuning(t); p.allNotesOff(); } break;
                    case 6: p.pedal(0.0f); p.setPartial(2, 3.05f, 1.5f, 0.5f, 2.0f); break;
                    case 7: p.noteOn(72.0f, 1.0f); p.noteOn(72.0f, 0.5f); break;
                    case 8: p.noteOn(76.0f, 1.0f); break;
                }
                ++c;
            }
            int n = total - at < size ? total - at : size;
            if (c < sizeof calls / sizeof calls[0] && calls[c].at - at < n) n = calls[c].at - at;
            p.render(x.data() + at, n);
            at += n;
        }
        return x;
    };
    const std::vector<float> a = run(1), b = run(128), c = run(1000);
    CHECK(std::memcmp(a.data(), b.data(), a.size() * sizeof(float)) == 0);
    CHECK(std::memcmp(a.data(), c.data(), a.size() * sizeof(float)) == 0);
    CHECK(rms(a.data(), a.size()) > 0.0 && allFinite(a.data(), a.size()));
    printf("    4 s of notes, a steal, a release, the pedal, two shapes, a tuning and a partial by hand at render sizes 1, 128 and 1000: the same "
           "bytes\n");
}

// a stolen voice's waiting note lands whole with the pool's pedal: four keys
// at the pedal 0, a fifth steals; while the stolen voice fades the pedal goes
// to 0.7 and the fifth key is let go; the note lands released, and at its
// first block its voice's eased pedal is the pool's 0.7 (whole: every channel
// on its target), its note the fifth key's
UP_TEST(pool_steal_lands_whole_with_the_pool_pedal) {
    Pool<4> pool(14);
    pool.setShape(grand());
    Record r;
    for (float key : {48.0f, 52.0f, 55.0f, 60.0f}) pool.noteOn(key, 1.0f);
    renderInto(pool, r, blocks(0.3f) * kBlock);
    pool.noteOn(64.0f, 0.9f);
    int stolen = -1;
    for (int i = 0; i < 4; ++i)
        if (pool.slot(i).use == PoolSlot::Use::stolen) stolen = i;
    CHECK(stolen >= 0);
    renderInto(pool, r, 4 * kBlock);
    pool.pedal(0.7f);
    pool.noteOff(64.0f);
    CHECK(pool.slot(stolen).nextOff);
    int landed = -1;
    for (int b = 0; b < 200 && landed < 0; ++b) {
        renderInto(pool, r, kBlock);
        if (slotOf(pool, 64.0f) == stolen) landed = b;
    }
    const State st = pool.voice(stolen).state();
    CHECK(landed > 0 && pool.slot(stolen).use == PoolSlot::Use::released && pool.voice(stolen).released());
    CHECK_MSG(st.shape[Channel::pedal] == 0.7f && st.shape[Channel::midi] == 64.0f && !st.asleep, "the landed voice: pedal %.4f, midi %.2f, asleep %d",
              st.shape[Channel::pedal], st.shape[Channel::midi], (int)st.asleep);
    printf("    the stolen voice took e4 %d blocks after its key, released, its pedal 0.7 and its note at once\n", landed + 4);
}

// the sizes, for the record
UP_TEST(pool_sizes) {
    printf("    sizeof: up::Voice %zu, up::PoolSlot %zu, up::PoolLane %zu, up::Board %zu, up::Pool<4> %zu, up::Pool<8> %zu (bytes; %d partials, %d "
           "board modes)\n",
           sizeof(Voice), sizeof(PoolSlot), sizeof(PoolLane), sizeof(Board), sizeof(Pool<4>), sizeof(Pool<8>), kMaxModes, kBoardModes);
    CHECK(sizeof(Pool<8>) < 8 * (sizeof(Voice) + sizeof(PoolSlot) + sizeof(PoolLane)) + sizeof(Board) + 4096);
}
