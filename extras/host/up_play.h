// upright — up_play.h
// a note rendered the way uptool play renders it, and what play reads from
// it; how loud a piano sounds over an octave; a shape heard as random, fit
// and the presets' test hear it; the board's swept transfer: shared by
// uptool (play, corpus, random, fit, chart --audio, board) and the tests.
// host only.
//
// the render: a voice (seed 1) with the tuning, the instrument (a shape's
// instrument channels over the default's, --set's instrument channels over
// those, the partial channels left to the law), a measured table set whole
// over it when given (setPartials), --set's partial and player channels
// after, the pedal down when asked, a strike at the note and
// velocity at sample 0; at their times a release, more strikes (the same
// note, each at its velocity), the pedal's fall and a glide (the glide
// channel to 4 / its length per second, then the note: 98 % of the way at
// its end). an event lands at the top of the block it falls in. rendered in
// blocks of 128: the voice's pair, the bridge force through a board of the
// voice's own (its plate from the voice's eased body, its weights at the
// voice's bridge point), the pickup added: the output, not clamped.
//
// the reading, over the voice's pair (the bridge force plus the pickup's
// voltage, before the board) up to the first event: the law's table as the
// voice eased it at the strike, each partial on tracked (its peak within 50
// cents, its cents, its level, its t60); the fundamental's envelope: the beat
// on it and the two-stage decay's slopes; and over the output: the peak, the
// first half second's rms; the thd of the pair over the 100 ms after the
// hammer leaves (the electric side); from the telemetry the strike's contact
// and its peak force; after a release, the fundamental's t60 from there; the
// sleep.
#pragma once

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <utility>
#include <vector>

#include "core/up_board.h"
#include "core/up_channels.h"
#include "core/up_presets.h"
#include "core/up_voice.h"
#include "up_analysis.h"

namespace up {

struct PlaySpec {
    Shape instrument;                            // the instrument channels (nan: the default's); the rest unread
    bool  table = false;                         // a measured table set whole over the instrument
    PartialTable tableP{};
    std::vector<std::pair<Channel, float>> sets; // written after the instrument (and the table), before the strike
    Tuning tuning;
    float midi = 60.0f, vel = 1.0f, sec = 4.0f;
    float offAt = -1.0f;                         // s: the key released; under zero none
    std::vector<std::pair<float, float>> strikes;   // more strikes: (s, velocity), the same note
    bool  pedal = false;                         // the pedal down from the start
    float pedalOffAt = -1.0f;                    // s: the pedal's fall; under zero none
    float glideTo = -1.0f, glideAt = 1.0f, glideSec = -1.0f;   // a note; under zero none. glideSec under zero: the rest
    float fs = kDefaultFs;
};

struct PlayRender {
    std::vector<float> x;                        // the output: the board's on the bridge force, and the pickup
    std::vector<float> pair;                     // the voice's pair summed (the bridge force and the pickup), before the board
    std::vector<float> bridge, pickup;           // the pair apart
    PartialTable table{};                        // the eased table at the strike's block
    State struck;                                // the state after the strike's block
    float f0 = 0.0f;
    float contactMs = 0.0f, peakForce = 0.0f;    // the first strike's, from the telemetry
    float sleptAt = -1.0f;                       // s; under zero: still ringing at the end
    float energyAtEnd = 0.0f;
    float firstEvent = 0.0f;                     // s: the end of the stretch the reading reads
};

// the instrument: the default's channels, the spec's over them, the spec's sets of instrument channels over
// those (all under one write, before a table: an instrument write after a table would rewrite its partials)
inline Shape instrumentOf(const PlaySpec& sp, const Tuning& t) {
    Shape s = defaultShape(t);
    for (int i = 0; i < kInstrumentChannels; ++i)
        if (sp.instrument.v[i] == sp.instrument.v[i]) s.v[i] = sp.instrument.v[i];
    for (const auto& p : sp.sets)
        if (isInstrumentChannel(p.first)) s[p.first] = p.second;
    for (int i = kInstrumentChannels; i < kChannelCount; ++i) s.v[i] = std::nanf("");
    return s;
}

// the voice as the spec writes it before the first strike: the instrument, the table over it, the sets of
// partial and player channels over that, the pedal
inline void prepare(Voice& v, const PlaySpec& sp) {
    const Tuning t = clamped(sp.tuning);
    v.setTuning(t);
    v.setShape(instrumentOf(sp, t));
    if (sp.table) v.setPartials(sp.tableP);
    for (const auto& p : sp.sets)
        if (!isInstrumentChannel(p.first)) v.set(p.first, p.second);
    if (sp.pedal) v.setPedal(1.0f);
}

inline PlayRender renderPlay(const PlaySpec& sp) {
    PlayRender r;
    Voice v(1, sp.fs);
    prepare(v, sp);
    v.noteOn(sp.midi, sp.vel);
    Board board;
    board.init(sp.fs);
    float w[kBoardModes];
    float lastPoint = std::nanf("");

    const int total = (int)std::lround(sp.sec * sp.fs);
    auto at = [&](float s) { return s >= 0.0f ? (int)std::lround(s * sp.fs) : -1; };
    const int offAt = at(sp.offAt), pedalOffAt = at(sp.pedalOffAt), glideAt = sp.glideTo >= 0.0f ? at(sp.glideAt) : -1;
    const float glideSec = sp.glideSec > 0.0f ? sp.glideSec : sp.sec - sp.glideAt;
    int first = total;
    for (int e : {offAt, pedalOffAt, glideAt})
        if (e >= 0 && e < first) first = e;
    for (const auto& s : sp.strikes) first = std::min(first, at(s.first));
    r.firstEvent = (float)first / sp.fs;
    r.x.assign((size_t)total, 0.0f);
    r.pair = r.bridge = r.pickup = r.x;
    std::vector<float> boardOut((size_t)kBlock);
    bool awake = false, left = false;
    for (int a = 0; a < total; a += kBlock) {
        const int end = a + kBlock;
        if (offAt >= a && offAt < end) v.noteOff();
        for (const auto& s : sp.strikes) {
            const int e = at(s.first);
            if (e >= a && e < end) v.noteOn(sp.midi, s.second);
        }
        if (pedalOffAt >= a && pedalOffAt < end) v.setPedal(0.0f);
        if (glideAt >= a && glideAt < end) {
            v.setGlide(glideSec > 0.0f ? 4.0f / glideSec : 0.0f);
            v.setPitch(sp.glideTo);
        }
        const int n = total - a < kBlock ? total - a : kBlock;
        v.render(r.bridge.data() + a, r.pickup.data() + a, n);
        const State st = v.state();
        board.setPlate(st.shape[Channel::boardArea], st.shape[Channel::boardLoss], st.shape[Channel::open]);
        if (v.bridgePoint() != lastPoint) {
            lastPoint = v.bridgePoint();
            board.inputWeights(lastPoint, w);
        }
        const float* forces[1] = {r.bridge.data() + a};
        const float* weights[1] = {w};
        board.render(forces, weights, 1, boardOut.data(), n);
        for (int i = 0; i < n; ++i) {
            r.pair[(size_t)(a + i)] = r.bridge[(size_t)(a + i)] + r.pickup[(size_t)(a + i)];
            r.x[(size_t)(a + i)] = boardOut[(size_t)i] + r.pickup[(size_t)(a + i)];
        }
        if (a == 0) {
            r.struck = st;
            v.partials(r.table);
            r.f0 = st.f0;
        }
        if (a < first && !left) {
            r.peakForce = std::max(r.peakForce, st.hammerPeakForce);
            if (!st.hammerTouching) {
                r.contactMs = 1000.0f * st.hammerContact;
                left = true;
            }
        }
        awake = awake || !v.asleep();
        if (awake && v.asleep() && r.sleptAt < 0.0f) r.sleptAt = (float)(a + n) / sp.fs;
    }
    r.energyAtEnd = v.energy();
    return r;
}

// a voice struck once and rendered on until it sleeps: the time it sleeps (s), or under zero if not within maxSec
inline float sleepTime(const PlaySpec& sp, float maxSec) {
    Voice v(1, sp.fs);
    prepare(v, sp);
    v.noteOn(sp.midi, sp.vel);
    const long blocks = (long)(maxSec * sp.fs / kBlock);
    for (long b = 0; b < blocks; ++b) {
        v.render(nullptr, nullptr, kBlock);
        if (b > 0 && v.asleep()) return (float)((b + 1) * kBlock) / sp.fs;
    }
    return -1.0f;
}

struct PlayReading {
    std::vector<int> slot;                       // the partials on, by hz
    std::vector<Tracked> tracked;
    double peak = 0.0, peakAt = 0.0, rmsHalf = 0.0;
    Beat beat;                                   // on the fundamental
    TwoSlopes slopes;                            // the fundamental's two-stage decay, db/s
    double thd = NAN;                            // the pair's, the 100 ms after the hammer leaves (the electric side)
    double releaseT60 = 0.0;                     // s: the fundamental's from the release on
};

// the partials on in the eased table at f0, sorted by hz
inline std::vector<int> partialsOn(const PartialTable& p, float f0, float ceilingHz) {
    std::vector<int> out;
    for (int k = 0; k < kMaxModes; ++k) {
        const float hz = p.ratio[k] * f0;
        if (hz > 0.0f && hz < ceilingHz && (p.gain[k] != 0.0f || p.out[k] != 0.0f) && p.t60[k] > 0.0f) out.push_back(k);
    }
    std::stable_sort(out.begin(), out.end(), [&](int a, int b) { return p.ratio[a] < p.ratio[b]; });
    return out;
}

inline PlayReading readPlay(const PlayRender& r, const PlaySpec& sp) {
    PlayReading out;
    const Tuning t = clamped(sp.tuning);
    out.slot = partialsOn(r.table, r.f0, t.ceiling * sp.fs);
    std::vector<double> expect;
    for (int k : out.slot) expect.push_back((double)r.table.ratio[k] * r.f0);
    const size_t n = std::min(r.pair.size(), (size_t)std::lround(r.firstEvent * sp.fs));
    out.tracked = trackPartials(r.pair.data(), n, sp.fs, expect);
    for (size_t i = 0; i < r.x.size(); ++i)
        if (std::fabs(r.x[i]) > out.peak) {
            out.peak = std::fabs(r.x[i]);
            out.peakAt = (double)i / sp.fs;
        }
    out.rmsHalf = rms(r.x.data(), std::min(r.x.size(), (size_t)(0.5 * sp.fs)));
    if (!out.slot.empty() && n > 0) {
        // the fundamental (the slot nearest f0)'s band, half way to the next partial
        const double f1 = expect[0], next = expect.size() > 1 ? expect[1] : 2.0 * f1;
        const double hw = std::min(0.45 * (next - f1), 0.25 * f1);
        const BandAnalyser bands(r.pair.data(), n, sp.fs);
        const Envelope e = bands.envelope(f1 - hw, f1 + hw, 0.01);
        out.beat = beatOf(e, 0.1, 2.0);
        out.slopes = twoSlopes(e);
        // the electric side's thd over the 100 ms after the hammer leaves
        const float g = r.struck.shape[Channel::generator];
        const size_t from = (size_t)std::lround(0.001f * r.contactMs * sp.fs), count = (size_t)(0.1 * sp.fs);
        if (g > -1.0f && from + count <= n) out.thd = thdOf(r.pair.data(), from, count, f1, sp.fs);
        if (sp.offAt >= 0.0f) {
            const size_t a = (size_t)std::lround(sp.offAt * sp.fs);
            if (a < r.pair.size()) {
                const BandAnalyser after(r.pair.data() + a, r.pair.size() - a, sp.fs);
                out.releaseT60 = t60Of(after.envelope(f1 - hw, f1 + hw, 0.01), 3.0, 40.0, 10.0).t60;
            }
        }
    }
    return out;
}

// what play prints, into f
inline void printPlay(std::FILE* f, const PlayRender& r, const PlayReading& rd, const PlaySpec& sp) {
    const RankState& rk = r.struck.rank;
    std::fprintf(f, "  midi %.2f, velocity %.2f: f0 %.2f hz; the string: L %.3f m, B %.4g, the hammer %.2f g; %d partials on\n", sp.midi, sp.vel,
                 r.f0, rk.L, rk.B, rk.hammerMass * 1e3, (int)rd.slot.size());
    std::fprintf(f, "  the hammer (the telemetry): contact %.3f ms, peak force %.2f N\n", r.contactMs, r.peakForce);
    std::fprintf(f, "  the table as the voice eased it at the strike, and the voice's pair over its first %.2f s:\n", r.firstEvent);
    std::fprintf(f, "  slot   ratio        hz     gain      out   t60 law  |  measured hz    cents  level db  t60 measured\n");
    for (size_t i = 0; i < rd.slot.size(); ++i) {
        const int k = rd.slot[i];
        const Tracked& t = rd.tracked[i];
        std::fprintf(f, "  %4d %7.4f  %8.2f  %+7.4f  %+7.3f  %8.3f  |  ", k, r.table.ratio[k], t.expect, r.table.gain[k], r.table.out[k],
                     r.table.t60[k] * rk.t60Scale);
        if (t.hz > 0.0) std::fprintf(f, "%10.2f  %+7.2f  %8.1f", t.hz, t.cents, t.levelDb);
        else std::fprintf(f, "%10s  %7s  %8.1f", "-", "-", t.levelDb);
        if (t.decay.t60 > 0.0) std::fprintf(f, "  %8.3f\n", t.decay.t60);
        else std::fprintf(f, "  %8s\n", "-");
    }
    std::fprintf(f, "  the output (through the board, the pickup added): the peak %.4f (%+.1f dbfs) at %.3f s; the first half second's rms %.4f "
                    "(%+.1f dbfs)\n",
                 rd.peak, dbfs(rd.peak), rd.peakAt, rd.rmsHalf, dbfs(rd.rmsHalf));
    std::fprintf(f, "  the first three partials' t60:");
    for (size_t i = 0; i < 3 && i < rd.tracked.size(); ++i) {
        const int k = rd.slot[i];
        std::fprintf(f, " %d: %s s (law %.2f)", k + 1, rd.tracked[i].decay.t60 > 0.0 ? std::to_string(rd.tracked[i].decay.t60).substr(0, 6).c_str() : "-",
                     r.table.t60[k] * rk.t60Scale);
    }
    std::fprintf(f, "\n  the fundamental: the beat %.3f hz (%d troughs, the ripple %.2f db); the two-stage decay %.2f db/s early, %.2f late (%.2f)\n",
                 rd.beat.rate, rd.beat.troughs, rd.beat.depthDb, rd.slopes.early, rd.slopes.late,
                 rd.slopes.late != 0.0 ? rd.slopes.early / rd.slopes.late : 0.0);
    if (rd.thd == rd.thd) std::fprintf(f, "  the thd of the pair over the 100 ms after the hammer leaves: %.2f %%\n", 100.0 * rd.thd);
    if (sp.offAt >= 0.0f) std::fprintf(f, "  released at %.2f s: the fundamental's t60 from there %.3f s\n", sp.offAt, rd.releaseT60);
    if (r.sleptAt >= 0.0f) std::fprintf(f, "  slept at %.2f s\n", r.sleptAt);
    else std::fprintf(f, "  still ringing at %.2f s (every bank's energy %.3g mm²)\n", (double)r.x.size() / sp.fs, r.energyAtEnd);
}

// ---- the level ---------------------------------------------------------------------------------------
// how loud a piano sounds: struck at each of the twelve semitones c4 .. b4
// (midi 60 .. 71), velocity 1, each rendered 0.6 s as play renders it (the
// voice's pair, the bridge force through a board of its own, the pickup
// added: the sum a listener hears) at the default tuning, the rms over the
// first half second after the strike; the octave's level is the mean of the
// twelve powers (one note's level is the board's coincidence of partials and
// plate modes; an octave's is the piano's). the reference is grand's octave

constexpr float kLevelMidi = 60.0f;              // the octave's first note, c4
constexpr int   kLevelNotes = 12;                // its twelve semitones, c4 .. b4
constexpr float kLevelSec = 0.6f, kLevelWindow = 0.5f;

struct LevelReading {
    double rms = 0.0, peak = 0.0;
};

// a piano struck at one note at velocity 1, 0.6 s: the first half second's rms and the peak
inline LevelReading levelReading(const Shape& instrument, float midi = kLevelMidi, const Tuning& t = Tuning()) {
    PlaySpec sp;
    sp.instrument = instrument;
    sp.tuning = t;
    sp.midi = midi;
    sp.vel = 1.0f;
    sp.sec = kLevelSec;
    const PlayRender r = renderPlay(sp);
    LevelReading out;
    out.rms = rms(r.x.data(), std::min(r.x.size(), (size_t)(kLevelWindow * sp.fs)));
    out.peak = peakAbs(r.x.data(), r.x.size());
    return out;
}

// the same at each of the twelve semitones from `from` up: the rms of the twelve powers' mean, the largest peak
inline LevelReading octaveReading(const Shape& instrument, float from = kLevelMidi, const Tuning& t = Tuning()) {
    double power = 0.0;
    LevelReading out;
    for (int i = 0; i < kLevelNotes; ++i) {
        const LevelReading r = levelReading(instrument, from + (float)i, t);
        power += r.rms * r.rms;
        out.peak = std::max(out.peak, r.peak);
    }
    out.rms = std::sqrt(power / kLevelNotes);
    return out;
}

// the reference's octave, grand's, once
inline double levelReferenceRms() {
    static const double r = octaveReading(grand()).rms;
    return r;
}

// a shape's octave from `from` up against the reference, db, and its largest peak
struct OctaveHeard {
    double level = 0.0, peak = 0.0;
};
inline OctaveHeard hearOctave(const Shape& s, float from = kLevelMidi, const Tuning& t = Tuning()) {
    OctaveHeard h;
    const LevelReading r = octaveReading(s, from, t);
    h.level = 20.0 * std::log10(r.rms / levelReferenceRms() + 1e-30);
    h.peak = r.peak;
    return h;
}

// ---- a shape heard ------------------------------------------------------------------------------------
// a shape struck at a note (velocity 1, sec seconds as play renders it, at
// the tuning) and read: the peak, the first half second's rms and its level
// against the reference (grand's octave at the tuning's own gain), and
// whether it sounds: every sample finite, the peak over kSoundPeak, the last
// 100 ms's rms under the first 100 ms's. kSoundPeak is about a quarter of
// the reference's octave rms (0.0897): 33 db under full scale, 17 db under
// grand's c4 peak

constexpr double kSoundPeak = 0.022;

struct Sounds {
    double peak = 0.0;
    bool   finite = true, decaying = true, sounds = true;
};

// whether a render sounds: every sample finite, the peak over kSoundPeak, the last 100 ms's rms under the first 100 ms's
inline Sounds soundsOf(const std::vector<float>& x, float fs) {
    Sounds g;
    const size_t w = (size_t)(0.1 * fs), n = x.size();
    g.peak = peakAbs(x.data(), n);
    g.finite = allFinite(x.data(), n);
    g.decaying = n >= 2 * w && rms(x.data() + n - w, w) < rms(x.data(), w);
    g.sounds = g.finite && g.decaying && g.peak > kSoundPeak;
    return g;
}

struct Heard {
    PlayRender r;
    PlayReading rd;
    double level = 0.0;                          // the first half second's rms against the reference's, db
    Sounds sounds;
};

// a shape heard at a note at the tuning, its level read against the reference at the tuning's own gain
inline Heard hearShape(const Shape& s, float midi, float sec, const Tuning& t = Tuning()) {
    Heard h;
    PlaySpec sp;
    sp.instrument = s;
    sp.tuning = t;
    sp.midi = midi;
    sp.vel = 1.0f;
    sp.sec = sec;
    h.r = renderPlay(sp);
    h.rd = readPlay(h.r, sp);
    h.level = 20.0 * std::log10(h.rd.rmsHalf / levelReferenceRms() + 1e-30);
    h.sounds = soundsOf(h.r.x, sp.fs);
    return h;
}

// ---- the board's swept transfer ----------------------------------------------------------------------
// a unit-amplitude force sine (N) in at a note's bridge point, a board at a body's plate (its eased channels:
// area, loss, open) and a shelf's gain, run to steady state (4 s under 100 hz, 2 s under 400, 1 s
// above): the output's rms over whole cycles of the last half against the force's (1 / sqrt 2), |out / F| in
// full scale per N. the board is linear in its shelf's gain, so a run kept at the shelf 0 and at 1 gives any
// gain's output as their blend

constexpr double kSweepLo = 20.0, kSweepHi = 10000.0;

// the sweep's points: 1/12 octave from 20 hz to 10 khz
inline std::vector<double> sweepHz() {
    std::vector<double> out;
    for (int i = 0;; ++i) {
        const double hz = kSweepLo * std::exp2(i / 12.0);
        if (hz > kSweepHi * 1.0001) break;
        out.push_back(hz);
    }
    return out;
}

// the steady stretch of a board driven by a unit sine at hz: whole cycles of the run's last half
inline std::vector<float> boardRun(float area, float loss, float open, float midi, double hz, float shelf, float fs = kDefaultFs) {
    Board b;
    b.init(fs);
    b.setPlate(area, loss, open);
    b.setShelf(shelf);
    float w[kBoardModes];
    b.inputWeights(midi, w);
    const double sec = hz < 100.0 ? 4.0 : (hz < 400.0 ? 2.0 : 1.0);
    const int total = (int)(sec * fs);
    std::vector<float> f((size_t)total), out((size_t)total);
    for (int i = 0; i < total; ++i) f[(size_t)i] = (float)std::sin(kTwoPi * hz * i / fs);
    for (int a = 0; a < total; a += kBlock) {
        const int n = total - a < kBlock ? total - a : kBlock;
        const float* fp[1] = {f.data() + a};
        const float* wp[1] = {w};
        b.render(fp, wp, 1, out.data() + a, n);
    }
    const int cycles = std::max(1, (int)std::floor(0.5 * sec * hz));
    const int count = std::min(total / 2, (int)std::lround(cycles * fs / hz));
    return std::vector<float>(out.end() - count, out.end());
}

// |out / F| from a steady stretch
inline double transferOf(const std::vector<float>& y) { return rms(y.data(), y.size()) * std::sqrt(2.0); }

// |out / F| at a body's plate, a note's bridge point and a shelf
inline double boardTransfer(float area, float loss, float open, float midi, double hz, float shelf) {
    return transferOf(boardRun(area, loss, open, midi, hz, shelf));
}

// the mean of the transfer in db over the sweep's points in lo .. hi, at a shelf, from runs kept at 0 and 1
struct SweepRuns { std::vector<double> hz; std::vector<std::vector<float>> at0, at1; };
inline SweepRuns sweepRuns(float area, float loss, float open, float midi, double lo, double hi) {
    SweepRuns r;
    for (double hz : sweepHz()) {
        if (hz < lo * 0.9999 || hz > hi * 1.0001) continue;
        r.hz.push_back(hz);
        r.at0.push_back(boardRun(area, loss, open, midi, hz, 0.0f));
        r.at1.push_back(boardRun(area, loss, open, midi, hz, 1.0f));
    }
    return r;
}
inline double bandAverageDb(const SweepRuns& r, double shelf) {
    double sum = 0.0;
    for (size_t i = 0; i < r.hz.size(); ++i) {
        std::vector<float> y(r.at0[i].size());
        for (size_t j = 0; j < y.size(); ++j) y[j] = r.at0[i][j] + (float)shelf * (r.at1[i][j] - r.at0[i][j]);
        sum += dbfs(transferOf(y));
    }
    return r.hz.empty() ? NAN : sum / (double)r.hz.size();
}

// the shelf whose upper band (2 f_top .. 4 f_top) averages what the top mode's octave (f_top / 2 .. f_top) does,
// by bisection on its log
inline double shelfFor(const SweepRuns& under, const SweepRuns& over) {
    double lo = 1e-3, hi = 1e7;
    for (int i = 0; i < 80; ++i) {
        const double mid = std::sqrt(lo * hi);
        (bandAverageDb(under, mid) - bandAverageDb(over, mid) > 0.0 ? lo : hi) = mid;
    }
    return std::sqrt(lo * hi);
}

// the infinite plate's mobility 1 / (8 sqrt(H rho h)), H = sqrt(Dx Dy), m/s per N
inline double plateMobility() {
    const double h3 = (double)kBoardH * kBoardH * kBoardH, dx = kBoardEx * h3 / 12.0, dy = kBoardEy * h3 / 12.0;
    return 1.0 / (8.0 * std::sqrt(std::sqrt(dx * dy) * kBoardRho * kBoardH));
}

}  // namespace up
