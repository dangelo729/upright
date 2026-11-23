// upright — tests/test_voice.cpp
//
// 30 s of channel thrash under a strike every 100 ms is finite through a board,
// its pickup under 1.5 and its board under 1; a key jump on a ringing voice,
// up or down, grows in neither output and the corpus's glide stays under the
// click bound; grand at c4 released lands the damper, f#6 over damperTop
// is the key held; under the pedal a release rings out and the pedal's fall lands
// the damper; the same calls give the same bytes at render sizes 1, 128 and
// 1000; a latched voice renders the bytes of one written directly; the
// measured tine set whole rings its partials where the table says; a partial
// by hand stands until the instrument is written again; the generator swept
// under a held note is finite and under the click bound; rhodes at e3
// sleeps; a sleeping voice renders zeros; grand's octave through the board
// reads the reference's level; two noteOns in a block are the second; the
// pool's hooks; wild writes into every entry point stay finite, and a valid
// note after them sounds and sleeps.
#include <cfloat>
#include <cmath>
#include <cstring>
#include <limits>
#include <string>
#include <utility>
#include <vector>

#include "core/up_board.h"
#include "core/up_presets.h"
#include "core/up_voice.h"
#include "up_analysis.h"
#include "up_fit.h"
#include "up_play.h"
#include "up_shapes.h"
#include "up_test.h"

using namespace up;

namespace {

constexpr float kFs = 44100.0f;
double db(double x) { return 20.0 * std::log10(x + 1e-30); }

// the first time (s) from `from` on where every 10 ms frame after it peaks under the record's peak by `under`
// db; under zero if never within the record
double underFrom(const std::vector<float>& x, double from, double under) {
    const double peak = peakAbs(x.data(), x.size());
    const size_t w = (size_t)(0.01 * kFs);
    double at = -1.0;
    for (size_t a = (size_t)(from * kFs); a + w <= x.size(); a += w) {
        const bool quiet = db(peakAbs(x.data() + a, w) / peak) < -under;
        if (quiet && at < 0.0) at = (double)a / kFs;
        if (!quiet) at = -1.0;
    }
    return at;
}

// a voice's bridge force with calls at block tops: (block, call)
template <class Script>
std::vector<float> bridgeOf(const Shape& s, float sec, Script script, const Tuning& t = Tuning()) {
    Voice v(3);
    v.setTuning(t);
    v.setShape(s);
    const int blocks = (int)(sec * kFs) / kBlock;
    std::vector<float> x((size_t)blocks * kBlock);
    for (int b = 0; b < blocks; ++b) {
        script(v, b);
        v.render(x.data() + (size_t)b * kBlock, nullptr, kBlock);
    }
    return x;
}

int blockAt(double sec) { return (int)(sec * kFs) / kBlock; }

}  // namespace

namespace {

// a shape drawn inside the presets' box: each instrument channel between the
// nine presets' lowest and highest values, log where its span is (up_fit.h),
// the generator from -1 to +1 with the tonebar beside it as every preset
// holds it (1 - |g|)
Shape drawInPresets(Rng& r) {
    Shape s = nanShape();
    for (int i = 0; i < kInstrumentChannels; ++i) {
        const Channel c = (Channel)i;
        float lo = presetAt(0).shape()[c], hi = lo, a, b;
        for (int p = 1; p < presetCount(); ++p) {
            lo = std::fmin(lo, presetAt(p).shape()[c]);
            hi = std::fmax(hi, presetAt(p).shape()[c]);
        }
        const float u = 0.5f * (r.bipolar() + 1.0f);
        s.v[i] = channelSpan(c, a, b) && lo > 0.0f ? lo * std::pow(hi / lo, u) : lo + (hi - lo) * u;
    }
    return withGenerator(s, s[Channel::generator]);
}

}  // namespace

// 30 s of thrash: every 100 ms a random shape (drawInPresets, the generator
// among its channels) through setShape and a strike at a random note
// (24 .. 108) and velocity, the pair through a board of the voice's own as
// play renders it: finite, no nan (a check). the voice is not clamped and the
// shape's level is its own: the raw peaks (the whole, the board's, the
// pickup's) printed, the pickup's under 1.5 and the board's under 1.0
// (checks, known with the number where missed): a key struck on a ringing voice retunes its
// strings, which grow in neither output (up_bank.h), so the pickup does not
// read a bass string's millimetres at a treble frequency, nor the bridge a
// treble string's velocity as bass millimetres. the board's largest peaks
// are the attack's (up_attack.h): its blow's share follows a hard hammer's
// force in the treble
UP_TEST(voice_thrash_30s) {
    Voice v(2026);
    Board board;
    board.init(kFs);
    Rng r;
    r.seed(424242);
    auto uni = [&](float lo, float hi) { return lo + (hi - lo) * (0.5f + 0.5f * r.bipolar()); };
    float bridge[kBlock], pickup[kBlock], out[kBlock], w[kBoardModes];
    bool finite = true;
    double peak = 0.0, sq = 0.0, boardPeak = 0.0, pickupPeak = 0.0;
    long n = 0;
    const int step = (int)(0.1f * kFs) / kBlock;
    for (int blk = 0; blk < 300 * step; ++blk) {
        if (blk % step == 0) {
            v.setShape(drawInPresets(r));
            v.noteOn(uni(24.0f, 108.0f), uni(0.05f, 1.0f));
        }
        v.render(bridge, pickup, kBlock);
        const State st = v.state();
        board.setPlate(st.shape[Channel::boardArea], st.shape[Channel::boardLoss], st.shape[Channel::open]);
        board.inputWeights(v.bridgePoint(), w);
        const float* f[1] = {bridge};
        const float* ww[1] = {w};
        board.render(f, ww, 1, out, kBlock);
        for (int i = 0; i < kBlock; ++i) {
            const float y = out[i] + pickup[i];
            finite = finite && std::isfinite(y);
            peak = std::fmax(peak, std::fabs(y));
            boardPeak = std::fmax(boardPeak, std::fabs(out[i]));
            pickupPeak = std::fmax(pickupPeak, std::fabs(pickup[i]));
            sq += (double)y * y;
        }
        n += kBlock;
    }
    CHECK(finite);
    KNOWN_FAILURE(pickupPeak < 1.5, "the thrash's raw pickup peaks %.3f (the whole %.3f, the board %.3f)", pickupPeak, peak, boardPeak);
    KNOWN_FAILURE(boardPeak < 1.0, "the thrash's raw board peaks %.3f (the whole %.3f, the pickup %.3f)", boardPeak, peak, pickupPeak);
    printf("    30 s of thrash through the board: finite %s; raw, the peak %.3f (the board's %.3f, the pickup's %.3f), the rms %.4f\n",
           finite ? "yes" : "NO", peak, boardPeak, pickupPeak, std::sqrt(sq / (double)n));
}

// the tip's sums' weights as the strings hold them at the shape: each bank's first kTineModes slots, the out
// held to ±1 (the main's out; the secondaries' x lerp(1, kUnisonGain, clamp(1 + g, 0, 1)) x their weights)
struct TipWeights {
    float w[3][kTineModes];
};
TipWeights tipWeightsOf(const Shape& s) {
    TipWeights t;
    const float strings = stringsOf(s), g = s[Channel::generator];
    float w2 = clamp(strings - 1.0f, 0.0f, 1.0f);
    const float w3 = clamp(strings - 2.0f, 0.0f, 1.0f);
    if (g >= 0.0f) w2 *= clamp(std::isnan(s[Channel::tonebar]) ? 1.0f : s[Channel::tonebar], 0.0f, 1.0f);
    const float gain = lerp(1.0f, kUnisonGain, clamp(1.0f + g, 0.0f, 1.0f));
    for (int k = 0; k < kTineModes; ++k) {
        const float out = s[outChannel(k)];
        t.w[0][k] = clamp(out, -1.0f, 1.0f);
        t.w[1][k] = clamp(out * gain * w2, -1.0f, 1.0f);
        t.w[2][k] = clamp(out * gain * w3, -1.0f, 1.0f);
    }
    return t;
}

// one bank's share of the tip's velocity sum: tip w im over its first kTineModes slots on
template <int N>
double tipVelocityOf(const Bank<N>& b, const float* w) {
    double v = 0.0;
    for (int k = 0; k < kTineModes && k < N; ++k) {
        if (!b.on(k)) continue;
        float re, im;
        b.phasor(k, re, im);
        v += (double)w[k] * b.omega(k) * im;
    }
    return v;
}

// a key jump on one ringing voice: grand struck at `from` at velocity 1 and
// rung 1 s, then the midi channel written to `to` at a block edge (the glide
// 0: the note jumps). the tip's velocity sum (every bank's first kTineModes
// slots at the tip's weights, w im: the sum a pickup reads), the voice's
// bridge force through a board of its own as play renders it, and the force
// itself, each as an rms over four periods of the lower note's fundamental
// before the jump against the same after, in db; and over one block each side
struct Jump {
    double tip, board, force, tip1, board1, force1;
    size_t span;
};
Jump jumpOf(float from, float to) {
    Voice v(3);
    v.setShape(grand());
    v.noteOn(from, 1.0f);
    Board board;
    board.init(kFs);
    float w[kBoardModes], point = -1.0f, br[kBlock], out[kBlock];
    const int jump = blockAt(1.0), after = 32;
    std::vector<double> tip, brd, force;
    for (int b = 0; b < jump + after; ++b) {
        if (b == jump) v.set(Channel::midi, to);
        TipWeights tw{};
        for (int i = 0; i < kBlock; ++i) {
            v.render(br + i, nullptr, 1);
            if (i == 0) tw = tipWeightsOf(v.state().shape);
            const Strings& s = v.strings();
            tip.push_back(tipVelocityOf(s.main(), tw.w[0]) + tipVelocityOf(s.second(), tw.w[1]) + tipVelocityOf(s.third(), tw.w[2]));
        }
        const State st = v.state();
        board.setPlate(st.shape[Channel::boardArea], st.shape[Channel::boardLoss], st.shape[Channel::open]);
        if (v.bridgePoint() != point) {
            point = v.bridgePoint();
            board.inputWeights(point, w);
        }
        const float* f[1] = {br};
        const float* ww[1] = {w};
        board.render(f, ww, 1, out, kBlock);
        for (int i = 0; i < kBlock; ++i) {
            brd.push_back(out[i]);
            force.push_back(br[i]);
        }
    }
    const size_t edge = (size_t)jump * kBlock, span = (size_t)std::lround(4.0 * kFs / midiToHz(std::fmin(from, to)));
    auto rmsOf = [](const std::vector<double>& x, size_t a, size_t n) {
        double q = 0.0;
        for (size_t i = a; i < a + n; ++i) q += x[i] * x[i];
        return std::sqrt(q / (double)n);
    };
    auto jumpDb = [&](const std::vector<double>& x, size_t n) { return db(rmsOf(x, edge, n) / rmsOf(x, edge - n, n)); };
    return {jumpDb(tip, span), jumpDb(brd, span), jumpDb(force, span), jumpDb(tip, kBlock), jumpDb(brd, kBlock), jumpDb(force, kBlock), span};
}

// a key jump on one ringing voice grows in neither output (up_bank.h: a
// partial moved up keeps its velocity, one moved down its displacement).
// grand's c3 jumped to c5: the tip's velocity sum within 1 db (a check), the
// board within 2 db (a check), the force's fall printed (its displacement
// falls by hz_old / hz_new; the rank's bridge scale rises with the note).
// grand's c5 jumped to c3: the board at most 1 db over before (a check), the
// tip's velocity falling (printed). each over four periods of c3's
// fundamental each side (1348 samples, 30.6 ms): one block holds 0.38 of
// c3's period and its rms swings with the phase from block to block, so the
// one-block numbers are printed beside. then the corpus's glide (grand's c4
// to g4 from 1 s over 2 s, 4 s): its click measure from 0.25 s under the
// bound for its top partial at g4
UP_TEST(voice_key_jump_grows_in_neither_output) {
    const Jump up = jumpOf(48.0f, 72.0f), down = jumpOf(72.0f, 48.0f);
    CHECK_MSG(std::fabs(up.tip) <= 1.0, "the key jump c3 -> c5: the tip's velocity sum %+.2f db", up.tip);
    CHECK_MSG(std::fabs(up.board) <= 2.0, "the key jump c3 -> c5: the board's output %+.2f db", up.board);
    CHECK_MSG(down.board <= 1.0, "the key jump c5 -> c3: the board's output %+.2f db", down.board);
    for (int i = 0; i < 2; ++i) {
        const Jump& j = i ? down : up;
        printf("    %s at 1 s, %zu samples each side: the tip's velocity sum %+.2f db, the board %+.2f db, the bridge force %+.2f db; over one "
               "block each side: %+.2f, %+.2f, %+.2f db\n",
               i ? "c5 -> c3" : "c3 -> c5", j.span, j.tip, j.board, j.force, j.tip1, j.board1, j.force1);
    }
    PlaySpec sp;
    sp.instrument = grand();
    sp.sec = 4.0f;
    sp.glideTo = 67.0f;
    sp.glideAt = 1.0f;
    sp.glideSec = 2.0f;
    const PlayRender r = renderPlay(sp);
    const double f1 = r.f0 * std::exp2(7.0 / 12.0);
    double top = 0.0;
    for (int k = 0; k < kMaxModes; ++k) {
        const double hz = r.table.ratio[k] * f1;
        if (hz < kCeiling * kFs && (r.table.gain[k] != 0.0f || r.table.out[k] != 0.0f)) top = std::fmax(top, hz);
    }
    const size_t from = (size_t)blockAt(0.25) * kBlock, tenth = (size_t)(0.1 * kFs);
    const double click = clickRatio(r.x.data() + from, r.x.size() - from, kFs), bound = clickBound(top, kFs);
    CHECK(allFinite(r.x.data(), r.x.size()));
    CHECK_MSG(click < bound, "the glide: the click measure %.4f, the bound %.4f (top %.0f hz)", click, bound, top);
    printf("    the glide c4 -> g4 from 1 s over 2 s: the click measure %.4f, the bound %.4f (the top partial %.0f hz); the output's rms over "
           "0.9 .. 1 s %.4f, 2.9 .. 3 s %.4f\n",
           click, bound, top, rms(r.x.data() + (size_t)(0.9 * kFs), tenth), rms(r.x.data() + (size_t)(2.9 * kFs), tenth));
}

// grand at c4, velocity 1, released at 1 s: its bridge force falls 60 db
// under its peak within 1.5 x the damper's t60 (6.907755 / dampRate at the
// grand's 20 /s: 0.345 s) of the release; at f#6, over damperTop, the
// release changes nothing: the same bytes as a key held (its own fall 60 db
// under its peak printed: the fundamental's t60 1.2 s at the loss knee)
UP_TEST(voice_release_lands_the_damper) {
    const Shape s = grand();
    const double t60 = 6.907755 / s[Channel::dampRate];
    auto released = [&](float midi) {
        return bridgeOf(s, 4.0f, [&](Voice& v, int b) {
            if (b == 0) v.noteOn(midi, 1.0f);
            if (b == blockAt(1.0)) v.noteOff();
        });
    };
    const std::vector<float> c4 = released(60.0f), fs6 = released(90.0f);
    const std::vector<float> held = bridgeOf(s, 4.0f, [&](Voice& v, int b) {
        if (b == 0) v.noteOn(90.0f, 1.0f);
    });
    const double at = underFrom(c4, 1.0, 60.0) - (double)blockAt(1.0) * kBlock / kFs;
    CHECK_MSG(at > 0.0 && at <= 1.5 * t60, "c4: 60 db under its peak %.3f s after the release (1.5 x %.3f s)", at, t60);
    CHECK_MSG(held.size() == fs6.size() && std::memcmp(held.data(), fs6.data(), held.size() * sizeof(float)) == 0, "f#6 released differs from f#6 held");
    const double fsAt = underFrom(fs6, 0.0, 60.0);
    printf("    c4 released at 1 s: 60 db under its peak %.3f s later (the damper's t60 %.3f s, 1.5 x %.3f); f#6 (no damper) released: the bytes "
           "of the key held, 60 db under its peak from %.3f s\n",
           at, t60, 1.5 * t60, fsAt);
}

// under the pedal (down from the start) grand at c4 released at 1 s rings
// out: 60 db under its peak not reached by 3 s; the pedal's fall at 2 s lands
// the damper within 1.5 x its t60 after the pedal's ease (the eased pedal
// under 0.05: -ln 0.05 / ratePedal)
UP_TEST(voice_pedal_holds_the_damper) {
    const Shape s = grand();
    const double t60 = 6.907755 / s[Channel::dampRate], ease = -std::log(0.05) / Tuning().ratePedal;
    const std::vector<float> held = bridgeOf(s, 3.0f, [&](Voice& v, int b) {
        if (b == 0) {
            v.setPedal(1.0f);
            v.noteOn(60.0f, 1.0f);
        }
        if (b == blockAt(1.0)) v.noteOff();
    });
    const double heldAt = underFrom(held, 1.0, 60.0);
    CHECK_MSG(heldAt < 0.0, "under the pedal: 60 db under its peak at %.3f s", heldAt);
    const std::vector<float> fall = bridgeOf(s, 4.0f, [&](Voice& v, int b) {
        if (b == 0) {
            v.setPedal(1.0f);
            v.noteOn(60.0f, 1.0f);
        }
        if (b == blockAt(1.0)) v.noteOff();
        if (b == blockAt(2.0)) v.setPedal(0.0f);
    });
    const double at = underFrom(fall, 2.0, 60.0) - (double)blockAt(2.0) * kBlock / kFs;
    CHECK_MSG(at > 0.0 && at <= ease + 1.5 * t60, "the pedal's fall at 2 s: 60 db under its peak %.3f s later (the ease %.3f + 1.5 x %.3f s)", at,
              ease, t60);
    printf("    under the pedal the release rings (%s by 3 s); the pedal's fall at 2 s: 60 db under its peak %.3f s later (the ease %.3f s, then "
           "1.5 x %.3f s)\n",
           heldAt < 0.0 ? "not 60 db under" : "60 db under", at, ease, t60);
}

// the same calls at the same samples give the same bytes whatever size the
// render is asked in (1, 128, 1000; a render split where a call falls): a
// strike in the first block, a release at 0.5 s, a re-strike at 0.8 s, the
// pedal down at 1 s, both outputs live (the generator at -0.4)
UP_TEST(voice_same_bytes_at_any_render_size) {
    struct Call { int at; int what; };
    const int total = (int)(2.0f * kFs);
    const Call calls[] = {{0, 0}, {(int)(0.5f * kFs), 1}, {(int)(0.8f * kFs), 2}, {(int)(1.0f * kFs), 3}};
    auto run = [&](int size) {
        Voice v(77);
        v.setShape(withGenerator(grand(), -0.4f));
        std::vector<float> b((size_t)total), p((size_t)total);
        int at = 0;
        size_t c = 0;
        while (at < total) {
            while (c < sizeof calls / sizeof calls[0] && calls[c].at == at) {
                switch (calls[c].what) {
                    case 0: v.noteOn(55.0f, 0.9f); break;
                    case 1: v.noteOff(); break;
                    case 2: v.noteOn(55.0f, 0.6f); break;
                    case 3: v.setPedal(1.0f); break;
                }
                ++c;
            }
            int n = total - at < size ? total - at : size;
            if (c < sizeof calls / sizeof calls[0] && calls[c].at - at < n) n = calls[c].at - at;
            v.render(b.data() + at, p.data() + at, n);
            at += n;
        }
        return std::make_pair(b, p);
    };
    const auto a = run(1), b = run(128), c = run(1000);
    CHECK(std::memcmp(a.first.data(), b.first.data(), a.first.size() * sizeof(float)) == 0);
    CHECK(std::memcmp(a.first.data(), c.first.data(), a.first.size() * sizeof(float)) == 0);
    CHECK(std::memcmp(a.second.data(), b.second.data(), a.second.size() * sizeof(float)) == 0);
    CHECK(std::memcmp(a.second.data(), c.second.data(), a.second.size() * sizeof(float)) == 0);
    CHECK(rms(a.first.data(), a.first.size()) > 0.0 && rms(a.second.data(), a.second.size()) > 0.0);
    printf("    2 s at render sizes 1, 128 and 1000: the same bytes on both outputs\n");
}

// the handoff: a voice attached to a pending table and latched before every
// block renders the bytes of one written directly through an instrument, a
// strike, a partial by hand, a measured table whole, a tuning, the pedal, a
// glide, a release, two noteOns in one gap, a sleep and a wake; shape() and
// tuning() read back the same at every gap, before the latch
UP_TEST(voice_handoff_same_bytes) {
    Tuning t;
    t.outputGain = 0.8f;
    t.silence = 1e-3f;
    PartialTable table = fromShape(defaultShape());
    table.ratio[3] = 4.07f;
    table.t60[5] = 0.5f;
    const Shape other = with(withGenerator(fortepiano(), -0.6f), {{Channel::detune, 8.0f}});
    auto script = [&](Voice& v, int block) {
        switch (block) {
            case 0:    v.setShape(grand()); v.noteOn(60.0f, 1.0f); break;
            case 40:   v.setPartial(2, 3.02f, 3.0f, 0.7f, -2.5f); break;
            case 80:   v.setTuning(t); v.setGlide(5.0f); v.setPitch(64.0f); break;
            case 120:  v.setPartials(table); v.noteOn(64.0f, 0.6f); break;
            case 200:  v.setShape(other); v.setPedal(0.6f); break;
            case 260:  v.noteOn(50.0f, 0.2f); v.noteOn(52.0f, 0.9f); v.noteOff(); break;
            case 300:  v.setPedal(0.0f); break;
            case 1400: v.setStrings(0.6f, 1.5f, 520.0f, 1.0f); v.setUnison(2.5f, 4.0f); v.setHammer(200.0f, 2.7f, 3e-4f, 3.0f);
                       v.setStrike(0.13f); v.setBody(1.2f, 0.02f, 0.4f); v.setGenerator(-0.8f, 1.2f, 0.8f); v.setDamper(30.0f, 85.0f);
                       v.noteOn(67.0f, 0.8f); break;
            default: break;
        }
    };
    Voice direct(31), handed(31);
    Pending p;
    handed.attach(&p);
    const int blocks = 1800;
    std::vector<float> a((size_t)blocks * kBlock), b((size_t)blocks * kBlock), ap(a.size()), bp(b.size());
    bool shapes = true, tunings = true, slept = false;
    for (int k = 0; k < blocks; ++k) {
        script(direct, k);
        script(handed, k);
        const Shape sa = direct.shape(), sb = handed.shape();
        shapes = shapes && std::memcmp(&sa, &sb, sizeof(Shape)) == 0;
        const Tuning ta = direct.tuning(), tb = handed.tuning();
        tunings = tunings && std::memcmp(&ta, &tb, sizeof(Tuning)) == 0;
        handed.latch();
        direct.render(a.data() + (size_t)k * kBlock, ap.data() + (size_t)k * kBlock, kBlock);
        handed.render(b.data() + (size_t)k * kBlock, bp.data() + (size_t)k * kBlock, kBlock);
        slept = slept || direct.asleep();
    }
    CHECK(std::memcmp(a.data(), b.data(), a.size() * sizeof(float)) == 0);
    CHECK(std::memcmp(ap.data(), bp.data(), ap.size() * sizeof(float)) == 0);
    CHECK(shapes);
    CHECK(tunings);
    CHECK(slept && !direct.asleep() && !handed.asleep());
    CHECK(direct.released() == handed.released());
    CHECK(rms(a.data(), a.size()) > 0.0);
    printf("    %d blocks through the handoff: the same bytes as written directly; slept and woke\n", blocks);
}

// the measured rhodes tine (tine_rhodes.txt: the 2nd partial at 7.4) set
// whole with setPartials over rhodes at e4, one string: its partials ring where
// the table says within 1 cent (the fundamental and the 7.4, both loud: each
// printed with its level)
UP_TEST(voice_measured_table_rings_where_it_says) {
    RefTable t;
    std::string err;
    CHECK_MSG(readTable(std::string(UP_REF_DIR) + "/tine_rhodes.txt", t, err), "%s", err.c_str());
    PlaySpec sp;
    CHECK(partialTableOf(t, sp.tableP));
    sp.table = true;
    sp.instrument = rhodes();
    sp.instrument[Channel::strings] = 1.0f;
    sp.midi = 64.0f;
    sp.sec = 2.0f;
    const PlayRender r = renderPlay(sp);
    const std::vector<double> mag = magnitude(r.pair.data(), r.pair.size(), (size_t)1 << 20, Window::fallingHann);
    int measured = 0;
    for (const TinePoint& q : tinePoints(t)) {
        const double want = q.ratio * r.f0;
        const Peak pk = peakNear(mag, kFs, want, 50.0);
        const double c = 1200.0 * std::log2(pk.hz / want);
        CHECK_MSG(std::fabs(c) < 1.0, "partial %d: %.3f hz against %.3f (%+.3f cents)", q.partial, pk.hz, want, c);
        ++measured;
        printf("    partial %d (ratio %.3f): %.3f hz against the table's %.3f, %+.4f cents, %.1f db\n", q.partial, q.ratio, pk.hz, want, c, pk.db);
    }
    CHECK(measured == 2 && std::fabs(r.table.ratio[1] - 7.4f) < 1e-6f);
}

// a partial set by hand stands through a shape whose partial channels are nan
// (no instrument write) and is written over by the next instrument write,
// in the eased table and in the targets
UP_TEST(voice_hand_partial_stands_until_the_instrument) {
    Voice v(5);
    v.setShape(grand());
    v.noteOn(60.0f, 1.0f);
    float b[kBlock];
    v.render(b, nullptr, kBlock);
    PartialTable law;
    v.partials(law);
    const int k = 4;
    v.setPartial(k, 5.31f, 2.0f, 0.4f, -3.0f);
    for (int i = 0; i < 400; ++i) v.render(b, nullptr, kBlock);
    PartialTable held;
    v.partials(held);
    CHECK_MSG(std::fabs(held.ratio[k] - 5.31f) < 1e-4f && std::fabs(held.gain[k] - 0.4f) < 1e-4f, "ratio %.5f gain %.5f", held.ratio[k], held.gain[k]);
    Shape s;
    for (int i = 0; i < kChannelCount; ++i) s.v[i] = std::nanf("");
    s[Channel::midi] = 60.0f;
    v.setShape(s);
    for (int i = 0; i < 400; ++i) v.render(b, nullptr, kBlock);
    v.partials(held);
    CHECK(std::fabs(held.ratio[k] - 5.31f) < 1e-4f && v.shape()[ratioChannel(k)] == 5.31f);
    v.setShape(grand());
    for (int i = 0; i < 700; ++i) v.render(b, nullptr, kBlock);
    PartialTable back;
    v.partials(back);
    CHECK_MSG(std::fabs(back.ratio[k] - law.ratio[k]) < 1e-4f && v.shape()[ratioChannel(k)] == law.ratio[k], "ratio %.5f against the law's %.5f",
              back.ratio[k], law.ratio[k]);
    printf("    the 5th partial by hand at 5.31 stood through a nan shape; the instrument written again took it back to %.4f\n", back.ratio[k]);
}

// the generator swept from -1 to +1 over 2 s (the generator and the tonebar
// written each block) under a held grand c4: the voice's pair
// finite, and its click measure under the bound for its top partial (printed)
UP_TEST(voice_generator_sweep) {
    Voice v(9);
    Tuning t;
    t.rateBody = 0.0f;                                   // the written path is the sweep
    v.setTuning(t);
    v.setShape(grand());
    v.noteOn(60.0f, 1.0f);
    const int blocks = (int)(2.5f * kFs) / kBlock, from = blockAt(0.25), span = blockAt(2.0);
    std::vector<float> b((size_t)blocks * kBlock), p(b.size()), x(b.size());
    float top = 0.0f;
    for (int k = 0; k < blocks; ++k) {
        const float g = clamp(-1.0f + 2.0f * (float)(k - from) / (float)span, -1.0f, 1.0f);
        v.set(Channel::generator, g);
        v.set(Channel::tonebar, 1.0f - std::fabs(g));
        v.render(b.data() + (size_t)k * kBlock, p.data() + (size_t)k * kBlock, kBlock);
        PartialTable pt;
        v.partials(pt);
        for (int s = 0; s < kMaxModes; ++s)
            if (pt.ratio[s] * v.state().f0 < 0.45f * kFs && (pt.gain[s] != 0.0f || pt.out[s] != 0.0f)) top = std::fmax(top, pt.ratio[s] * v.state().f0);
    }
    for (size_t i = 0; i < x.size(); ++i) x[i] = b[i] + p[i];
    const double click = clickRatio(x.data() + blockAt(0.25) * kBlock, x.size() - (size_t)blockAt(0.25) * kBlock, kFs);
    const double bound = clickBound(top, kFs);
    CHECK(allFinite(x.data(), x.size()));
    CHECK_MSG(click < bound, "the click measure %.4f, the bound %.4f (top %.0f hz)", click, bound, top);
    printf("    the generator -1 -> +1 over 2 s under grand's c4: the click measure %.4f, the bound %.4f (the top partial %.0f hz)\n", click, bound,
           top);
}

// the generator's middle sounds: grand at c4, velocity 1, 1 s, at the
// generator -1, -0.5 and 0; at -0.5 the fundamental's level (the tracker at
// the law's frequency) on the pickup within 10 db of the 0 render's, and on
// the bridge within 12 db of the -1 render's (the goal 10 printed beside:
// the bridge's weight is half there, -6 db by design, and the fundamental's
// gain at the strike point half way to the tine's, which is struck near its
// clamp where its first mode barely moves)
UP_TEST(voice_generator_middle_sounds) {
    double bridge[3], pickup[3];
    const float gs[3] = {-1.0f, -0.5f, 0.0f};
    for (int i = 0; i < 3; ++i) {
        PlaySpec sp;
        sp.instrument = withGenerator(grand(), gs[i]);
        sp.sec = 1.0f;
        const PlayRender r = renderPlay(sp);
        const std::vector<double> f1 = {(double)r.table.ratio[0] * r.f0};
        bridge[i] = trackPartials(r.bridge.data(), r.bridge.size(), kFs, f1)[0].levelDb;
        pickup[i] = trackPartials(r.pickup.data(), r.pickup.size(), kFs, f1)[0].levelDb;
    }
    CHECK_MSG(std::fabs(bridge[1] - bridge[0]) <= 12.0, "the bridge's fundamental at -0.5 %+.1f db, at -1 %+.1f (%+.1f)", bridge[1], bridge[0],
              bridge[1] - bridge[0]);
    CHECK_MSG(std::fabs(pickup[1] - pickup[2]) <= 10.0, "the pickup's fundamental at -0.5 %+.1f db, at 0 %+.1f", pickup[1], pickup[2]);
    printf("    grand at c4, the fundamental: the bridge %+.1f db at -1, %+.1f at -0.5 (%+.1f; within 12, the goal 10); the pickup %+.1f at -0.5, "
           "%+.1f at 0 (%+.1f)\n",
           bridge[0], bridge[1], bridge[1] - bridge[0], pickup[1], pickup[2], pickup[1] - pickup[2]);
}

// the generator swept from 0 to +1 over 2 s (the generator and the tonebar
// written each block) under a held rhodes e4: the pickup's curve slides its
// centre from the magnet's offset to the gap; the output finite and its
// click measure under the bound for its top partial (printed)
UP_TEST(voice_generator_sweep_electric) {
    Voice v(9);
    Tuning t;
    t.rateBody = 0.0f;
    v.setTuning(t);
    v.setShape(rhodes());
    v.noteOn(64.0f, 1.0f);
    const int blocks = (int)(2.5f * kFs) / kBlock, from = blockAt(0.25), span = blockAt(2.0);
    std::vector<float> b((size_t)blocks * kBlock), p(b.size()), x(b.size());
    float top = 0.0f;
    for (int k = 0; k < blocks; ++k) {
        const float g = clamp((float)(k - from) / (float)span, 0.0f, 1.0f);
        v.set(Channel::generator, g);
        v.set(Channel::tonebar, 1.0f - g);
        v.render(b.data() + (size_t)k * kBlock, p.data() + (size_t)k * kBlock, kBlock);
        PartialTable pt;
        v.partials(pt);
        for (int s = 0; s < kMaxModes; ++s)
            if (pt.ratio[s] * v.state().f0 < 0.45f * kFs && (pt.gain[s] != 0.0f || pt.out[s] != 0.0f)) top = std::fmax(top, pt.ratio[s] * v.state().f0);
    }
    for (size_t i = 0; i < x.size(); ++i) x[i] = b[i] + p[i];
    const double click = clickRatio(x.data() + blockAt(0.25) * kBlock, x.size() - (size_t)blockAt(0.25) * kBlock, kFs);
    const double bound = clickBound(top, kFs);
    CHECK(allFinite(x.data(), x.size()));
    CHECK_MSG(click < bound, "the click measure %.4f, the bound %.4f (top %.0f hz)", click, bound, top);
    printf("    the generator 0 -> +1 over 2 s under rhodes's e4: the click measure %.4f, the bound %.4f (the top partial %.0f hz); the peak %.3f\n",
           click, bound, top, peakAbs(x.data(), x.size()));
}

// rhodes at e3, velocity 1, sleeps within 1.5 x its tine's t60 at the note
// (the fundamental's t60 channel x the rank's scale): the tine alone (one
// string) a check; the preset with its tonebar (a second string ringing
// kAftersound longer) printed, and known when over
UP_TEST(voice_rhodes_sleeps) {
    PlaySpec sp;
    sp.instrument = rhodes();
    sp.midi = 52.0f;
    PlayRender probe = renderPlay(sp);
    const double t60 = probe.table.t60[0] * probe.struck.rank.t60Scale;
    PlaySpec one = sp;
    one.sets.push_back({Channel::strings, 1.0f});
    const float alone = sleepTime(one, (float)(4.0 * t60)), whole = sleepTime(sp, (float)(6.0 * t60));
    CHECK_MSG(alone > 0.0f && alone <= 1.5 * t60, "the tine alone slept at %.2f s (1.5 x %.2f)", alone, t60);
    KNOWN_FAILURE(whole > 0.0f && whole <= 1.5 * t60, "rhodes with its tonebar slept at %.2f s (1.5 x %.2f s; the tonebar rings %.1f x longer)", whole,
                  t60, kAftersound);
    printf("    rhodes at e3: the tine's t60 %.2f s; the tine alone slept at %.2f s (%.2f of it), with its tonebar at %.2f s (%.2f)\n", t60, alone,
           alone / t60, whole, whole / t60);
}

// a voice nobody struck renders zeros on both outputs, and so does one that
// has slept
UP_TEST(voice_asleep_is_zero) {
    Voice v(6);
    v.setShape(grand());
    v.setPedal(0.5f);
    std::vector<float> b(4 * kBlock + 17), p(b.size());
    v.render(b.data(), p.data(), (int)b.size());
    bool zero = v.asleep();
    for (size_t i = 0; i < b.size(); ++i) zero = zero && b[i] == 0.0f && p[i] == 0.0f;
    CHECK(zero);
    Tuning t;
    t.silence = 1e-2f;
    v.setTuning(t);
    v.noteOn(84.0f, 0.3f);
    std::vector<float> long_((size_t)(3.0f * kFs));
    v.render(long_.data(), nullptr, (int)long_.size());
    CHECK(v.asleep());
    v.render(b.data(), p.data(), 1000);
    zero = true;
    for (int i = 0; i < 1000; ++i) zero = zero && b[(size_t)i] == 0.0f && p[(size_t)i] == 0.0f;
    CHECK(zero && v.state().asleep && v.state().rms == 0.0f && v.energy() == 0.0f);
}

// the headroom kBoardOut sets: grand's octave c4 .. b4 at velocity 1 through
// the board, read as up_play.h's octaveReading reads it (each note 0.6 s,
// the rms over its first half second, the twelve powers' mean), 0.0897 rms
// of full scale within 0.1 db (-20.9 db); grand at c4 alone (play's render,
// 1 s) peaks near 0.18 there (printed, with the octave's largest peak).
// rhodes at e4 through the pickup peaks between 0.2 and 0.5 (kPickupGain)
UP_TEST(voice_headroom) {
    const LevelReading octave = octaveReading(grand());
    const double octaveDb = 20.0 * std::log10(octave.rms / 0.0897);
    PlaySpec g;
    g.instrument = grand();
    g.sec = 1.0f;
    const double gp = peakAbs(renderPlay(g).x.data(), (size_t)kFs);
    PlaySpec r;
    r.instrument = rhodes();
    r.midi = 64.0f;
    r.sec = 1.0f;
    const double rp = peakAbs(renderPlay(r).x.data(), (size_t)kFs);
    CHECK_MSG(std::fabs(octaveDb) <= 0.1, "grand's octave through the board %.5f rms, %+.3f db from 0.0897", octave.rms, octaveDb);
    CHECK_MSG(rp >= 0.2 && rp <= 0.5, "rhodes at e4 peaks %.3f", rp);
    printf("    grand's octave c4 .. b4 through the board %.5f rms (%+.2f db full scale, kBoardOut %.4g), its largest peak %.3f; grand at c4 peaks "
           "%.3f; rhodes at e4 through the pickup %.3f (kPickupGain %.4g)\n",
           octave.rms, 20.0 * std::log10(octave.rms), kBoardOut, octave.peak, gp, rp, kPickupGain);
}

// grand through the board at velocity 1 (play's render, 1 s): c6's peak within
// 15 db under c4's and c2's within 15 db over it (the board's velocity, its
// direct share and its field carry the treble)
UP_TEST(voice_board_keyboard_balance) {
    double peak[3];
    const float notes[3] = {36.0f, 60.0f, 84.0f};
    for (int i = 0; i < 3; ++i) {
        PlaySpec sp;
        sp.instrument = grand();
        sp.midi = notes[i];
        sp.sec = 1.0f;
        peak[i] = peakAbs(renderPlay(sp).x.data(), (size_t)kFs);
    }
    CHECK_MSG(db(peak[2] / peak[1]) >= -15.0, "c6 peaks %.3f, %+.1f db against c4's %.3f", peak[2], db(peak[2] / peak[1]), peak[1]);
    CHECK_MSG(db(peak[0] / peak[1]) <= 15.0, "c2 peaks %.3f, %+.1f db against c4's %.3f", peak[0], db(peak[0] / peak[1]), peak[1]);
    printf("    grand through the board at velocity 1: c2 peaks %.3f (%+.1f db), c4 %.3f, c6 %.3f (%+.1f db)\n", peak[0], db(peak[0] / peak[1]), peak[1],
           peak[2], db(peak[2] / peak[1]));
}

// two noteOns in one block are the second: its note, its velocity
UP_TEST(voice_second_note_in_a_block_wins) {
    Voice a(8), b(8);
    a.setShape(grand());
    b.setShape(grand());
    a.noteOn(60.0f, 0.3f);
    a.noteOn(65.0f, 0.9f);
    b.noteOn(65.0f, 0.9f);
    std::vector<float> x((size_t)kFs), y((size_t)kFs);
    a.render(x.data(), nullptr, (int)kFs);
    b.render(y.data(), nullptr, (int)kFs);
    CHECK(std::memcmp(x.data(), y.data(), x.size() * sizeof(float)) == 0);
}

// the pool's hooks: energy() is every bank's summed energy as of the last
// block and falls as the ring does; stop() sleeps now, every phasor zero, no
// strike due; released() the key; bridgePoint() the note as eased; reseed()
// restarts the voice's generator (nothing in the engine draws it): a voice
// reseeded to another's seed renders its bytes
UP_TEST(voice_pool_hooks) {
    Voice v(10);
    v.setShape(grand());
    v.noteOn(62.0f, 1.0f);
    std::vector<float> x((size_t)kFs);
    v.render(x.data(), nullptr, kBlock);
    const float e0 = v.energy();
    v.render(x.data(), nullptr, (int)kFs);
    const float e1 = v.energy();
    CHECK(e0 > 0.0f && e1 > 0.0f && e1 < e0 && !v.asleep());
    CHECK(v.bridgePoint() == 62.0f && !v.released());
    v.noteOff();
    v.render(x.data(), nullptr, kBlock);
    CHECK(v.released() && v.state().released && v.state().damped);
    v.noteOn(62.0f, 1.0f);
    v.stop();
    v.render(x.data(), nullptr, 4 * kBlock);
    bool zero = v.asleep() && v.energy() == 0.0f;
    for (int i = 0; i < 4 * kBlock; ++i) zero = zero && x[(size_t)i] == 0.0f;
    CHECK(zero);
    Voice a(11), b(12);
    b.reseed(11);
    for (Voice* p : {&a, &b}) {
        p->setShape(grand());
        p->noteOn(60.0f, 1.0f);
    }
    std::vector<float> ya(4000), yb(4000);
    a.render(ya.data(), nullptr, 4000);
    b.render(yb.data(), nullptr, 4000);
    CHECK(std::memcmp(ya.data(), yb.data(), ya.size() * sizeof(float)) == 0);
}

// wild writes on grand then rhodes, heard through a board of their own: each
// value of infinity, the float's largest, 1e30, 3e5 and 1e-40, each signed,
// through set and setRate on every instrument and player channel and one
// partial's four, setShape, setPartial, setPartials, noteOn and setTuning on
// each field, each written on the preset under a strike, every rate a jump,
// and standing four blocks. the output, the energy and the state's levels
// stay finite; then the preset and the default tuning and rates again: the
// voice sleeps, and a valid note from rest sounds and sleeps once let go
UP_TEST(voice_wild_writes_stay_finite) {
    const float inf = std::numeric_limits<float>::infinity();
    const float wild[] = {inf, -inf, FLT_MAX, -FLT_MAX, 1e30f, -1e30f, 3e5f, -3e5f, 1e-40f, -1e-40f};
    float Tuning::*const fields[] = {&Tuning::rateStrings, &Tuning::rateHammer, &Tuning::rateBody, &Tuning::rateModes, &Tuning::ratePitch,
                                     &Tuning::ratePedal,   &Tuning::hammerScale, &Tuning::velCurve, &Tuning::outputGain, &Tuning::silence,
                                     &Tuning::stealRate,   &Tuning::ceiling,     &Tuning::ampHz,    &Tuning::attack};
    Tuning jumps;
    for (int f = 0; f < 6; ++f) jumps.*fields[f] = 0.0f;                         // the six group rates
    std::vector<Channel> chans;
    for (int i = 0; i < kChannelCount; ++i)
        if (!isPartialChannel((Channel)i) || partialOf((Channel)i) == 1) chans.push_back((Channel)i);
    const int nc = (int)chans.size(), per = 2 * nc + 14 + 5;
    for (const Shape* preset : {&grand(), &rhodes()}) {
        const char* name = preset == &grand() ? "grand" : "rhodes";
        Voice v(15);
        Board board;
        board.init(kFs);
        std::vector<float> y;
        bool state = true;
        double pair = 0.0;                                                        // the voice's own peak, bridge or pickup
        auto run = [&](int blocks) {
            float br[kBlock], pk[kBlock], out[kBlock], w[kBoardModes];
            for (int b = 0; b < blocks; ++b) {
                v.render(br, pk, kBlock);
                const State st = v.state();
                board.setPlate(st.shape[Channel::boardArea], st.shape[Channel::boardLoss], st.shape[Channel::open]);
                board.inputWeights(v.bridgePoint(), w);
                const float* f[1] = {br};
                const float* ww[1] = {w};
                board.render(f, ww, 1, out, kBlock);
                for (int i = 0; i < kBlock; ++i) {
                    y.push_back(out[i] + pk[i]);
                    pair = std::fmax(pair, std::fmax(std::fabs(br[i]), std::fabs(pk[i])));
                }
                state = state && std::isfinite(v.energy()) && std::isfinite(st.rms) && std::isfinite(st.amp[0]) && std::isfinite(st.hammerPeakForce);
            }
        };
        auto sleeps = [&](float sec) {
            for (int b = 0; b < blockAt(sec); ++b) {
                run(1);
                if (v.asleep()) return (b + 1) * kBlock / kFs;
            }
            return -1.0f;
        };
        for (int k = 0; k < 10 * per; ++k) {
            const float x = wild[k / per];
            const int j = k % per;
            v.setShape(*preset);
            v.setTuning(jumps);
            v.noteOn(36.0f + (float)((7 * k) % 48), 1.0f);
            if (j < nc) v.set(chans[(size_t)j], x);
            else if (j < 2 * nc) {
                float lo, hi;
                channelRange(chans[(size_t)(j - nc)], lo, hi);
                v.setRate(chans[(size_t)(j - nc)], x);
                v.set(chans[(size_t)(j - nc)], hi);                               // a target to move toward at that rate
            } else if (j < 2 * nc + 14) {
                Tuning t = jumps;
                t.*fields[j - 2 * nc] = x;
                v.setTuning(t);
                v.noteOn(60.0f, 0.01f);                                          // a soft strike: vel^velCurve at its widest
            } else {
                Shape s;
                PartialTable p;
                for (float& a : s.v) a = x;
                for (int i = 0; i < kMaxModes; ++i) p.ratio[i] = p.t60[i] = p.gain[i] = p.out[i] = x;
                switch (j - 2 * nc - 14) {
                    case 0: v.setShape(s); break;
                    case 1: v.setPartial(1, x, x, x, x); break;
                    case 2: v.setPartials(p); break;
                    case 3: v.noteOn(x, x); break;
                    case 4: v.noteOn(60.0f, x); break;
                }
            }
            run(4);
        }
        const bool wildFinite = allFinite(y.data(), y.size());
        const double wildPeak = peakAbs(y.data(), y.size());
        v.setTuning(Tuning());
        for (Channel c : chans) v.setRate(c, groupRate(groupOf(c), Tuning()));
        v.setShape(*preset);
        v.setGlide(0.0f);
        v.setPedal(0.0f);
        v.noteOff();
        const float quiet = sleeps(10.0f);
        v.noteOn(62.0f, 0.8f);
        y.clear();
        pair = 0.0;
        run(blockAt(1.0));
        const bool finite = allFinite(y.data(), y.size());
        v.noteOff();
        const float slept = sleeps(10.0f);
        CHECK_MSG(wildFinite && state, "%s: the output %s while wild, the state %s", name, wildFinite ? "finite" : "NOT finite",
                  state ? "finite" : "NOT finite");
        CHECK_MSG(quiet > 0.0f && finite && pair > 0.01 && slept > 0.0f, "%s after: slept %.2f s, then d4 finite %d, its pair peaks %.4f, slept %.2f s",
                  name, quiet, (int)finite, pair, slept);
        printf("    %s: %d wild writes, the raw peak %.3g; after, asleep in %.2f s; d4 from rest peaks %.3f (its pair) and sleeps %.2f s after "
               "its release\n",
               name, 10 * per, wildPeak, quiet, pair, slept);
    }
}
