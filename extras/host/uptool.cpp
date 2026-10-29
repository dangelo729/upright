// upright — uptool.cpp  (plan §15)
// the one cli.
//
//   uptool partials --knob NAME=VALUE ... | --preset NAME [--zero] [--midi M] [--set CHANNEL=VALUE ...] [--tune NAME=VALUE]
//       the law's partial table at midi M (60) for a vector: slot, partial,
//       ratio to f0, hz, cents off the harmonic n f0, t60 at the note (the
//       law's x the rank's t60Scale), gain, out, on or off (the ceiling); and
//       the rank's line (L, d, wrapped, mu, Z, B, the hammer's mass, the
//       strike share, the damper). --set lands on the instrument channels
//       before the law and on a partial channel after it
//   uptool partials --table FILE
//       a reference table's partials as read (extras/reference/), placed on
//       slots by partial number, beside the law's at the table's vector
//   uptool string --knob ... [--from M --to M]
//       the rank across the keyboard, a row a semitone (21 .. 108): f0, L, d,
//       wrapped, mu, Z, B, the hammer's mass, its felt's K and p at the
//       note (the rank's hardnessScale and pAdd), the strike share, the damper;
//       B against each B table's note in percent where it has the note; the
//       bass break (the lowest plain-wire note)
//   uptool hammer --knob ... [--midi M --vel V]
//       the contact alone through the offline simulator (up_contact.h) at
//       32 fs, the truth: the force every 0.1 ms, the contact length, the
//       peak, the impulse, the hammer's momentum change, the force's spectrum
//       at every partial on (db against the first's), the band (partials 8 ..
//       16 against 1 .. 3); a velocity ladder (0.1, 0.2, 0.35, 0.5, 0.7, 1,
//       each with its speed in m/s) through the engine's scheme (kContactRate
//       substeps, the loss implicit) and the truth, kth's c4 contact beside;
//       the felt table's c^p against powf's, its worst relative error over
//       0.01 .. 4 mm
//   uptool pickup --knob ... [--amp A] [--from-hammer --midi M --vel V]
//       the curve C(x) and C'(x) over ±3 mm at the vector's gap and offset
//       (magnetic, electrostatic, and the one curve at the generator); a sine
//       of amplitude A mm (0.2) through the voltage: harmonics 1 .. 8 in db
//       and the thd; a ladder of amplitudes 0.05, 0.1, 0.2, 0.5, 1, 2 mm.
//       --from-hammer: the tip from the simulator at --vel over the first 100
//       ms after the hammer leaves, in place of the sine
//   uptool board --knob ... [--sweep --out FILE]
//       the plate's modes: m, n, hz, t60, radiation; the bridge-line weights
//       per octave (a0, a1 .. a7, c8); the listening weights; the radiation's
//       -3 db low edge; the shelf: the top mode, kBoardShelf, the infinite
//       plate's mobility, the swept transfer's mean over the top mode's
//       octave and over 2 .. 4 f_top, the shelf's mobility that makes them
//       equal; the field: each line's length, g and a, its gain, its
//       impulse energy against the shelf's share and the kFieldGain that
//       matches, the energy's arrival for 1 N at c4's bridge point, the
//       swept transfer's means over f_top / 2 .. f_top, 2 .. 4 and 4 .. 8
//       f_top with the field and with the modes alone, a flat shelf's level.
//       --sweep: the transfer at 1/12 octave from 20 hz to 10 khz at body
//       -1, 0 and +1 into FILE (board_sweep.txt)
//   uptool print --knob ... | --preset NAME [--shape]
//       the vector as an up_presets.cpp line, {"vector", {sc, ha, st, un, bo,
//       ge, pi, da}, 21, 108},; a preset's own line, its name and range;
//       --shape every instrument channel the recipe writes and the read-back
//   uptool random [--seed S --count N --midi M]
//       N vectors uniform in the box, each as print prints it, with its B at
//       the note, its first partial's t60, its 8th partial's gain in db and
//       its strings count; each heard 2 s at M (60), velocity 1, its level
//       undone in the gain: the peak, the first half second's rms against
//       grand's octave, levelDb, the level undone, sounds or not (the peak
//       over 0.022, the last 100 ms's rms under the first 100 ms's, finite);
//       beside it the octave from M up as the level table reads one, and
//       that undone; both undone levels' spread over the N
//   uptool fit --table FILE [--lock KNOB=V ...] [--set CHANNEL=VALUE ...]
//       the vector nearest a reference table (up_fit.h: by kind, the scale
//       for B and decay tables, the hammer for a hammer table, the generator
//       for a tine table; a grid in fifths, then descent to 0.01): the
//       vector, each entry's residual, the line; the vector heard as random
//       hears it
//   uptool fit --constants --tables DIR [--kind KIND] [--out FILE] [--fix NAME,...]
//       the LawConstants against every table in DIR at once (--kind: those of
//       one kind; those --fix names held where they stand): each table its
//       own vector, the constants by coordinate descent on the sum of the
//       tables' errors, the tables refitted until it settles; the constants
//       and each table's residual before and after, each B table's own best
//       tension; the proposed block into FILE (constants.txt).
//       nothing in src/ changes
//   uptool chart [DIR]
//       each knob alone in tenths at the three anchors (grand, rhodes,
//       wurlitzer) and the pairs scale x hammer, strike x unison, body x
//       scale at grand, generator x pickup, hammer x pickup at rhodes, in
//       fifths, as text tables into DIR; summary.txt, the cells that fail a
//       check; floors.txt, the audibility floors the table and the simulator
//       can read. each cell's strikes at c4 through the engine's scheme
//       (velocity 0.3 and 1) and the truth (velocity 1): contact, peak, band;
//       the electric side's thd from the engine's tip
//   uptool chart --audio [DIR]
//       the 1-d sweeps (each knob in tenths at the three anchors) rendered
//       2 s at velocity 1, c4 (e3 at rhodes and wurlitzer): the table's 10th
//       partial's cents, fundamental's t60, contact, peak and thd beside the
//       render's (the tracker, the telemetry, play's thd), audio_KNOB.txt
//       each, and audio_summary.txt with the worst disagreements
//   uptool level [--write] [--out PATH]
//       the level table measured (up_play.h): grand over the octave c4 ..
//       b4, each note at velocity 1, 0.6 s as play renders it, its first
//       half second's rms, the octave the mean of the twelve powers; each
//       knob alone at eleven points about the generator's four anchors
//       (string, middle, tine, reed), db against the anchor's own level, the
//       generator's row once against the reference; each cell's largest
//       peak, the run time; --write writes src/core/up_level_table.inc (or
//       PATH)
//   uptool play --preset NAME | --knob ... | --table FILE [--midi M --vel V --sec S --off-at S --restrike-at S
//               --pedal --pedal-off-at S --glide-to M2 [--glide-at S --glide-sec S] --undo-level -o x.wav --png p.png]
//       one note through a voice and a board of its own (up_play.h), S
//       seconds (4): the law's table beside what the voice's pair shows
//       (each partial's measured hz and cents off the table, its level, its
//       t60 where the ring allows), the output's peak and first half
//       second's rms, the first three partials' t60s, the beat and the two
//       slopes on the fundamental, the hammer's contact and peak force (the
//       telemetry), the thd on the electric side, the release's t60, the
//       sleep. --table: a tine table set whole over the vector (rhodes's when
//       none is named). --undo-level: the level table undone in the gain
//       (outputGain x 10^(-levelDb / 20)). a render that peaks over full
//       scale is written to the wav at the gain that brings it to 0.99, and
//       the gain printed
//   uptool corpus DIR [--tune NAME=VALUE ...]
//       the byte baseline: the three anchors at 36, 60, 84 at velocity 0.5
//       and 1; grand at 60 re-struck at 1 s, released at 1 s, released
//       under the pedal (up at 2.5 s), gliding to 67 from 1 s over 2 s;
//       rhodes at 52 and 64; wurlitzer at 57; the measured tine whole at 64;
//       every preset at the bottom, middle and top of its range at velocity
//       0.5 and 1, the level table undone (preset_NAME_MIDI_vVEL); 4 s each;
//       and a Pool<4> of grand, 6 s each: a chord (c3 e3 g3 c4, the pedal
//       from 2 s, the keys up at 3 s) and five keys on four voices (the
//       fifth steals the quietest): a wav and a png each, index.txt with what
//       play read (for the pool: the calls, the peak, the steal). --tune
//       lands on every render (a preset's gain over it, the pool's tuning)
//   uptool size
//       sizeof(up::Voice) by part; the board; the pending table; Pool<4> and
//       Pool<8>
//
// on every command: --knob NAME=VALUE ... (scale, hammer, strike, unison,
// body, generator, pickup, damper, each -1 .. 1; repeatable, each taking
// every pair up to the next option, by spaces or commas; a knob not named is
// grand's: 0, the generator -1)  --preset NAME (up_presets.h: grand,
// concertGrand, upright, spinet, honkyTonk, fortepiano, rhodes, rhodesBark,
// wurlitzer; a --knob after it wins)  --zero (all eight at 0)  --set
// CHANNEL=VALUE (any channel, up_channels.h)  --tune NAME=VALUE (any Tuning
// field)  --midi M.
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <initializer_list>
#include <string>
#include <utility>
#include <vector>

#include "core/up_board.h"
#include "core/up_channels.h"
#include "core/up_hammer.h"
#include "core/up_knobs.h"
#include "core/up_law.h"
#include "core/up_pickup.h"
#include "core/up_pool.h"
#include "core/up_rank.h"
#include "core/up_voice.h"
#include "up_analysis.h"
#include "up_contact.h"
#include "up_fit.h"
#include "up_play.h"
#include "up_png.h"
#include "up_wav.h"

using namespace up;

namespace {

struct Args {
    Knobs knobs = knobAnchors()[0].k;
    const PresetInfo* preset = nullptr;
    bool  shapeOut = false, constants = false, fromHammer = false, sweep = false, write = false, audio = false, undoLevel = false;
    Spec  sets;
    Tuning tuning;
    float midi = 60.0f, vel = 1.0f, amp = 0.2f;
    float from = 21.0f, to = 108.0f;
    float sec = -1.0f, offAt = -1.0f, restrikeAt = -1.0f, pedalOffAt = -1.0f;
    float glideTo = -1.0f, glideAt = 1.0f, glideSec = -1.0f;
    bool  pedal = false, knobsGiven = false;
    std::string table, tables, out, dir, fix, kind, wav, png;
    FitSpec lock;
    uint32_t seed = 1;
    int count = 8;
};

struct TuneField {
    const char* name;
    float Tuning::*field;
};
const TuneField kTune[] = {
    {"rateStrings", &Tuning::rateStrings}, {"rateHammer", &Tuning::rateHammer}, {"rateBody", &Tuning::rateBody},
    {"rateModes", &Tuning::rateModes},     {"ratePitch", &Tuning::ratePitch},   {"ratePedal", &Tuning::ratePedal},
    {"hammerScale", &Tuning::hammerScale}, {"velCurve", &Tuning::velCurve},     {"outputGain", &Tuning::outputGain},
    {"silence", &Tuning::silence},         {"stealRate", &Tuning::stealRate},   {"ceiling", &Tuning::ceiling},
    {"ampHz", &Tuning::ampHz},             {"attack", &Tuning::attack},
};

bool parsePair(const std::string& token, Channel& c, float& v) {
    const size_t eq = token.find('=');
    if (eq == std::string::npos) return false;
    if (!channelByName(token.substr(0, eq).c_str(), c)) {
        std::fprintf(stderr, "no channel named '%s'\n", token.substr(0, eq).c_str());
        return false;
    }
    v = (float)std::atof(token.c_str() + eq + 1);
    return true;
}

bool parseSpec(const std::string& text, Spec& out) {
    std::string token;
    for (size_t i = 0; i <= text.size(); ++i) {
        const char ch = i < text.size() ? text[i] : ' ';
        if (ch == ' ' || ch == ',' || ch == '\t') {
            if (!token.empty()) {
                Channel c;
                float v;
                if (!parsePair(token, c, v)) return false;
                out.push_back({c, v});
                token.clear();
            }
        } else {
            token += ch;
        }
    }
    return true;
}

// name=value pairs of knobs, by spaces or commas, into a vector (and a lock's flags)
bool parseKnobs(const std::string& text, Knobs& out, bool* flags = nullptr) {
    std::string token;
    for (size_t i = 0; i <= text.size(); ++i) {
        const char ch = i < text.size() ? text[i] : ' ';
        if (ch != ' ' && ch != ',' && ch != '\t') { token += ch; continue; }
        if (token.empty()) continue;
        const size_t eq = token.find('=');
        Knob k;
        if (eq == std::string::npos || !knobByName(token.substr(0, eq).c_str(), k)) {
            std::fprintf(stderr, "no knob named '%s' (scale, hammer, strike, unison, body, generator, pickup, damper)\n",
                         token.substr(0, eq).c_str());
            return false;
        }
        out.v[(int)k] = (float)std::atof(token.c_str() + eq + 1);
        if (flags) flags[(int)k] = true;
        token.clear();
    }
    return true;
}

bool parseTune(const char* text, Tuning& t) {
    const char* eq = std::strchr(text, '=');
    if (!eq) return false;
    const std::string name(text, (size_t)(eq - text));
    for (const TuneField& f : kTune) {
        if (name == f.name) { t.*(f.field) = (float)std::atof(eq + 1); return true; }
    }
    std::fprintf(stderr, "no tunable named '%s'\n", name.c_str());
    return false;
}

bool parseArgs(int argc, char** argv, int first, Args& a) {
    for (int i = first; i < argc; ++i) {
        const std::string s = argv[i];
        auto next = [&](const char*& v) { if (i + 1 >= argc) return false; v = argv[++i]; return true; };
        auto more = [&]() { return i + 1 < argc && std::strncmp(argv[i + 1], "--", 2) != 0; };
        const char* v = nullptr;
        if (s == "--knob") {
            a.knobsGiven = true;
            if (!next(v) || !parseKnobs(v, a.knobs)) return false;
            while (more()) if (!parseKnobs(argv[++i], a.knobs)) return false;
        } else if (s == "--lock") {
            Knobs at{};
            bool flags[kKnobCount] = {};
            if (!next(v) || !parseKnobs(v, at, flags)) return false;
            while (more()) if (!parseKnobs(argv[++i], at, flags)) return false;
            for (int k = 0; k < kKnobCount; ++k)
                if (flags[k]) { a.lock.locked[k] = true; a.lock.lockAt[k] = knobHeld(at.v[k]); }
        } else if (s == "--preset") {
            if (!next(v)) return false;
            a.preset = presetByName(v);
            if (!a.preset) {
                std::fprintf(stderr, "no preset named '%s' (", v);
                for (int p = 0; p < presetCount(); ++p) std::fprintf(stderr, "%s%s", p ? ", " : "", presetAt(p).name);
                std::fprintf(stderr, ")\n");
                return false;
            }
            a.knobs = a.preset->knobs;
            a.knobsGiven = true;
        } else if (s == "--zero") {
            a.knobs = Knobs{};
            a.knobsGiven = true;
        } else if (s == "--set") {
            if (!next(v) || !parseSpec(v, a.sets)) return false;
            while (more()) if (!parseSpec(argv[++i], a.sets)) return false;
        } else if (s == "--tune") { if (!next(v) || !parseTune(v, a.tuning)) return false; }
        else if (s == "--midi") { if (!next(v)) return false; a.midi = (float)std::atof(v); }
        else if (s == "--vel") { if (!next(v)) return false; a.vel = (float)std::atof(v); }
        else if (s == "--amp") { if (!next(v)) return false; a.amp = (float)std::atof(v); }
        else if (s == "--from") { if (!next(v)) return false; a.from = (float)std::atof(v); }
        else if (s == "--to") { if (!next(v)) return false; a.to = (float)std::atof(v); }
        else if (s == "--table") { if (!next(v)) return false; a.table = v; }
        else if (s == "--tables") { if (!next(v)) return false; a.tables = v; }
        else if (s == "--out") { if (!next(v)) return false; a.out = v; }
        else if (s == "--fix") { if (!next(v)) return false; a.fix = v; }
        else if (s == "--kind") { if (!next(v)) return false; a.kind = v; }
        else if (s == "--sec") { if (!next(v)) return false; a.sec = (float)std::atof(v); }
        else if (s == "--off-at") { if (!next(v)) return false; a.offAt = (float)std::atof(v); }
        else if (s == "--restrike-at") { if (!next(v)) return false; a.restrikeAt = (float)std::atof(v); }
        else if (s == "--pedal") { a.pedal = true; }
        else if (s == "--pedal-off-at") { if (!next(v)) return false; a.pedalOffAt = (float)std::atof(v); }
        else if (s == "--glide-to") { if (!next(v)) return false; a.glideTo = (float)std::atof(v); }
        else if (s == "--glide-at") { if (!next(v)) return false; a.glideAt = (float)std::atof(v); }
        else if (s == "--glide-sec") { if (!next(v)) return false; a.glideSec = (float)std::atof(v); }
        else if (s == "-o") { if (!next(v)) return false; a.wav = v; }
        else if (s == "--png") { if (!next(v)) return false; a.png = v; }
        else if (s == "--constants") { a.constants = true; }
        else if (s == "--shape") { a.shapeOut = true; }
        else if (s == "--from-hammer") { a.fromHammer = true; }
        else if (s == "--sweep") { a.sweep = true; }
        else if (s == "--write") { a.write = true; }
        else if (s == "--audio") { a.audio = true; }
        else if (s == "--undo-level") { a.undoLevel = true; }
        else if (s == "--seed") { if (!next(v)) return false; a.seed = (uint32_t)std::strtoul(v, nullptr, 10); }
        else if (s == "--count") { if (!next(v)) return false; a.count = std::atoi(v); }
        else if (s.compare(0, 2, "--") != 0 && a.dir.empty()) { a.dir = s; }
        else { std::fprintf(stderr, "unknown option %s\n", s.c_str()); return false; }
    }
    return true;
}

std::string vectorText(const Knobs& k) {
    char b[200];
    std::snprintf(b, sizeof b, "scale %+.2f hammer %+.2f strike %+.2f unison %+.2f body %+.2f generator %+.2f pickup %+.2f damper %+.2f",
                  k.v[0], k.v[1], k.v[2], k.v[3], k.v[4], k.v[5], k.v[6], k.v[7]);
    return b;
}

void makeDirs(const std::string& path) {
    std::error_code ec;
    std::filesystem::create_directories(path, ec);
}

std::string refDir() { return UP_REF_DIR; }

// the reference tables in the repository, read; the ones of a kind
std::vector<RefTable> refTables(const char* kind = nullptr) {
    std::vector<RefTable> out;
    for (const std::string& f : tableFiles(refDir())) {
        RefTable t;
        std::string err;
        if (!readTable(f, t, err)) { std::fprintf(stderr, "%s\n", err.c_str()); continue; }
        if (!kind || t.kind == kind) out.push_back(t);
    }
    return out;
}

// the vector's instrument with --set over it, the rank and the table at the note
struct At {
    Shape s;
    RankState r;
    PartialTable p;
};
At atNote(const Knobs& k, const Spec& sets, float midi, const Tuning& t = Tuning(), const LawConstants& c = lawConstants()) {
    At a;
    a.s = shapeOf(k, c, sets, t);
    a.r = rankAt(a.s, midi, c);
    a.p = partialsAt(a.s, a.r, c, sets, t);
    return a;
}

int partialsOnOf(const PartialTable& p) {
    int n = 0;
    for (int k = 0; k < kMaxModes; ++k) n += partialOn(p, k);
    return n;
}

void printRank(const RankState& r, float midi) {
    std::printf("the rank at %s (midi %.1f, %.2f hz): L %.4f m, d %.3f mm%s, core %.3f mm, mu %.3f g/m, Z %.2f kg/s, B %.4g, "
                "hammer %.2f g, felt K x %.3f, p %+.3f, strike share %.4f, t60 x %.3f, bridge %.0f N/m, bridge loss %.3f /s, longitudinal %.1f hz, "
                "%s\n",
                noteName(midi).c_str(), midi, r.f0, r.L, r.d * 1e3, r.wrapped ? " (wrapped)" : "", r.dCore * 1e3, r.mu * 1e3, r.Z, r.B,
                r.hammerMass * 1e3, r.hardnessScale, r.pAdd, r.strikeShare, r.t60Scale, r.bridgeScale, r.bridgeLoss, r.longHz,
                r.damped ? "damped" : "no damper");
}

std::string num(double x, const char* fmt);

// ---- partials --------------------------------------------------------------------------------------

int cmdPartialsTable(const Args& a);

int cmdPartials(const Args& a) {
    if (!a.table.empty()) return cmdPartialsTable(a);
    const Tuning t = clamped(a.tuning);
    const At x = atNote(a.knobs, a.sets, a.midi, t);
    std::printf("%s\n", vectorText(a.knobs).c_str());
    printRank(x.r, a.midi);
    std::printf("strings %.3f (capped %.3f), detune %.2f cents, generator %+.2f, ceiling %.0f hz\n", x.s[Channel::strings],
                stringsOf(x.s), x.s[Channel::detune], x.s[Channel::generator], t.ceiling * kDefaultFs);
    std::printf("slot  n      ratio          hz   cents/n f0    t60 (s)       gain        out  \n");
    for (int k = 0; k < kMaxModes; ++k) {
        const int n = k + 1;
        const double hz = (double)x.p.ratio[k] * x.r.f0;
        std::printf("%4d %2d %10.5f %11.2f %+12.2f %10.4f %+10.5f %+10.4f  %s\n", k, n, x.p.ratio[k], hz, centsOf(x.p.ratio[k] / n),
                    x.p.t60[k] * x.r.t60Scale, x.p.gain[k], x.p.out[k], partialOn(x.p, k) ? "on" : "off");
    }
    std::printf("%d of %d partials on; the pickup reads slots 0 .. %d (partials 1 .. %d), the bridge every slot on\n", partialsOnOf(x.p),
                kMaxModes, kTineModes - 1, kTineModes);
    return 0;
}

int cmdPartialsTable(const Args& a) {
    RefTable t;
    std::string err;
    if (!readTable(a.table, t, err)) { std::fprintf(stderr, "%s\n", err.c_str()); return 1; }
    std::printf("%s\n  source: %s\n%s%s  units: %s, kind %s\n", t.title.c_str(), t.source.c_str(), t.via.empty() ? "" : "  via: ",
                t.via.empty() ? "" : (t.via + "\n").c_str(), t.units.c_str(), t.kind.c_str());
    if (t.kind == "tine") {
        const KnobFit f = fitTable(t, lawConstants(), FitSpec());
        const At x = atNote(f.k, Spec(), (float)tableMidi(t));
        std::printf("measured at %s; the law at %s\n", noteName(tableMidi(t)).c_str(), vectorText(f.k).c_str());
        std::printf("slot  partial  ratio (table)  ratio (law)   cents   t60 (table)  t60 (law)\n");
        for (const TinePoint& p : tinePoints(t)) {
            const int k = p.partial - 1;
            if (k < 0 || k >= kMaxModes) continue;
            const double law = x.p.ratio[k] / x.p.ratio[0];
            std::printf("%4d %8d %14.4f %12.4f %+8.1f %12s %10.3f\n", k, p.partial, p.ratio, law, centsOf(law / p.ratio),
                        p.t60 > 0.0 ? std::to_string(p.t60).substr(0, 8).c_str() : "-", x.p.t60[k] * x.r.t60Scale);
        }
    } else if (t.kind == "decay") {
        const KnobFit f = fitTable(t, lawConstants(), FitSpec());
        std::printf("the law at %s\n", vectorText(f.k).c_str());
        std::printf("note   slot partial  t60 (table)  t60 (law)  log2\n");
        for (const Residual& r : f.error.items) std::printf("%-22s %10.3f %10.3f %+6.2f\n", r.label.c_str(), r.ref, r.law, r.err);
    } else if (t.kind == "B") {
        for (const BPoint& p : bPoints(t)) std::printf("%-4s midi %5.1f  B %.4g\n", noteName(p.midi).c_str(), p.midi, p.B);
    } else {
        for (const RefRow& r : t.rows) {
            std::printf("%-10s", r.key.c_str());
            for (double v : r.v) std::printf(" %12.6g", v);
            std::printf("%s%s\n", r.note.empty() ? "" : "   # ", r.note.c_str());
        }
    }
    return 0;
}

// ---- string ----------------------------------------------------------------------------------------

int cmdString(const Args& a) {
    const Shape s = shapeOf(a.knobs, lawConstants(), a.sets, clamped(a.tuning));
    const std::vector<RefTable> bt = refTables("B");
    std::printf("%s\n", vectorText(a.knobs).c_str());
    std::printf("note  midi       f0       L (m)   d (mm)  wound core  mu (g/m)  Z (kg/s)          B   hammer g   K N/mm^p      p   share  damper");
    for (const RefTable& t : bt) std::printf("  %s %%", t.name.c_str());
    std::printf("\n");
    int brk = -1;
    for (float m = a.from; m <= a.to + 1e-3f; m += 1.0f) {
        const RankState r = rankAt(s, m);
        if (brk < 0 && !r.wrapped) brk = (int)m;
        std::printf("%-4s %5.0f %9.2f %10.4f %8.3f %5s %5.2f %9.3f %9.2f %10.4g %10.2f %11.1f %6.3f %7.4f  %6s", noteName(m).c_str(), m, r.f0, r.L,
                    r.d * 1e3, r.wrapped ? "yes" : "no", r.dCore * 1e3, r.mu * 1e3, r.Z, r.B, r.hammerMass * 1e3,
                    hammerHardnessAt(s[Channel::hardness], s[Channel::generator]) * r.hardnessScale, hammerExponentAt(s[Channel::exponent], r.pAdd),
                    r.strikeShare, r.damped ? "yes" : "no");
        for (const RefTable& t : bt) {
            double ref = NAN;
            for (const BPoint& p : bPoints(t))
                if (std::fabs(p.midi - m) < 0.01) ref = p.B;
            if (ref == ref) std::printf("  %+*.1f", (int)t.name.size() + 2, 100.0 * (r.B / ref - 1.0));
            else std::printf("  %*s", (int)t.name.size() + 2, "-");
        }
        std::printf("\n");
    }
    std::printf("the bass break: the lowest plain-wire note %s (midi %d)\n", brk >= 0 ? noteName(brk).c_str() : "none", brk);
    return 0;
}

// ---- hammer ----------------------------------------------------------------------------------------

const float kLadder[6] = {0.1f, 0.2f, 0.35f, 0.5f, 0.7f, 1.0f};
constexpr int kTenth = kMaxModes > 9 ? 9 : kMaxModes - 1;   // the 10th partial's slot (the top in a smaller build)
constexpr double kTruthRate = 32.0;   // the simulator's own rate as the truth: 32 fs against 64 fs within 1 % (test_hammer)

// the engine's scheme: the simulator at kContactRate fs with the engine's step (the felt table, the loss
// implicit against the string's point mass); the truth: the simulator at kTruthRate fs, the loss explicit
void engineScheme(Contact& sim) { sim.useEngineStep(); }
double schemeRate(bool truth) { return truth ? kTruthRate : (double)kContactRate; }

// kth's c4 contact (hammer_params.txt: velocity_mps, contact_ms at midi 60) at a hammer speed, linear between
// its points; nan off their span
double kthContact(double mps) {
    static std::vector<std::pair<double, double>> pts;
    if (pts.empty()) {
        RefTable t;
        std::string err;
        if (readTable(refDir() + "/hammer_params.txt", t, err))
            for (const RefRow& r : t.rows) {
                const int vc = t.column(r.block, "velocity_mps"), cc = t.column(r.block, "contact_ms");
                if (vc > 0 && cc > 0 && std::fabs(r.x - 60.0) < 0.01) pts.push_back({t.at(r, vc), t.at(r, cc)});
            }
        std::sort(pts.begin(), pts.end());
    }
    for (size_t i = 1; i < pts.size(); ++i)
        if (mps >= pts[i - 1].first && mps <= pts[i].first) {
            const double u = (mps - pts[i - 1].first) / (pts[i].first - pts[i - 1].first + 1e-12);
            return pts[i - 1].second + u * (pts[i].second - pts[i - 1].second);
        }
    return NAN;
}

// the felt table's worst relative error against powf over 0.01 .. 4 mm at a power
double feltTableError(float p, float* worstAtMm = nullptr) {
    Hammer h;
    h.set(100.0f, p, 0.0f, 6.0f);
    double worst = 0.0;
    for (int i = 0; i <= 4000; ++i) {
        const float cmm = 0.01f * std::pow(400.0f, (float)i / 4000.0f);
        const float c = cmm * 1e-3f;
        const double e = std::fabs((double)h.forceOf(c, 0.0f) / h.forceExact(c, 0.0f) - 1.0);
        if (e > worst) { worst = e; if (worstAtMm) *worstAtMm = cmm; }
    }
    return worst;
}

int cmdHammer(const Args& a) {
    const Tuning t = clamped(a.tuning);
    const Shape s = shapeOf(a.knobs, lawConstants(), a.sets, t);
    Contact sim(s, a.midi, t, lawConstants(), kTruthRate * kDefaultFs);
    const ContactRun& r = sim.strike(a.vel, 0.0);
    std::printf("%s\n", vectorText(a.knobs).c_str());
    printRank(sim.rank(), a.midi);
    std::printf("the hammer at the note: K %.4g N/mm^p (%.4g N/m^p), p %.3f, lambda %.4g s/m, mass %.3f g; %zu partials in the string, "
                "modal mass %.4g g, the point under the hammer %.4g g\n",
                hammerHardnessAt(s[Channel::hardness], s[Channel::generator]) * sim.rank().hardnessScale, sim.hammer().stiffness(),
                sim.hammer().power(), sim.hammer().loss(), sim.hammer().mass() * 1e3, sim.modes().size(), sim.modalMass() * 1e3,
                sim.hammer().pointMass() * 1e3);
    std::printf("velocity %.2f: thrown at %.3f m/s (hammerScale %.2f x vel^%.2f), left at %+.3f m/s; the truth at %.0f fs\n", a.vel, r.v0,
                t.hammerScale, t.velCurve, r.vOut, kTruthRate);
    std::printf("  t (ms)   force (N)   string under the hammer (mm)\n");
    const int end = r.left >= 0 ? r.left : (int)r.force.size();
    for (double ms = 0.0; ms * 1e-3 * r.fs < end + 1; ms += 0.1) {
        const int k = (int)std::lround(ms * 1e-3 * r.fs);
        if (k >= (int)r.force.size()) break;
        std::printf("  %6.2f %11.3f %12.5f\n", ms, r.force[(size_t)k], 1e3 * r.y[(size_t)k]);
    }
    std::printf("contact %.3f ms, peak %.3f N, impulse %.5f N s, the hammer's momentum change %.5f kg m/s (%+.4f %%)%s\n", r.contactMs(), r.peak,
                r.impulse, r.momentum, 100.0 * (r.impulse / r.momentum - 1.0), r.left < 0 ? ", NEVER LEFT" : "");
    std::printf("the force's spectrum at each partial on, db against the first's:\n");
    const double first = dftAt(r.force, sim.modes().empty() ? 1.0 : sim.modes()[0].hz, r.fs);
    for (size_t i = 0; i < sim.modes().size(); ++i) {
        const double v = dftAt(r.force, sim.modes()[i].hz, r.fs);
        std::printf("  n %2d %9.1f hz %+7.1f db%s", sim.slots()[i] + 1, sim.modes()[i].hz, dbOf(v / first), (i % 3 == 2) ? "\n" : "");
    }
    std::printf("\nthe band (partials 8 .. 16 against 1 .. 3): %+.2f db\n", bandDb(r, sim.table(), sim.rank().f0));
    std::printf("\nthe ladder: the engine's scheme (the felt table, the loss implicit, %d substeps a sample) and the truth (%.0f fs, explicit);\n"
                "kth's c4 contact at the same speed beside (askenfelt & jansson, fig. 6)\n"
                "   vel   v (m/s) | engine: contact (ms)  peak (N)  band (db) | truth: contact (ms)  peak (N)  band (db) | kth (ms)\n",
                kContactRate, kTruthRate);
    for (float vel : kLadder) {
        Contact le(s, a.midi, t, lawConstants(), kContactRate * kDefaultFs), lt(s, a.midi, t, lawConstants(), kTruthRate * kDefaultFs);
        engineScheme(le);
        const ContactRun& q = le.strike(vel, 0.0);
        const ContactRun& qt = lt.strike(vel, 0.0);
        std::printf("  %4.2f %9.3f | %20.3f %9.3f %+10.2f | %19.3f %9.3f %+10.2f | %8s%s\n", vel, q.v0, q.contactMs(), q.peak,
                    bandDb(q, le.table(), le.rank().f0), qt.contactMs(), qt.peak, bandDb(qt, lt.table(), lt.rank().f0),
                    std::fabs(a.midi - 60.0f) < 0.01f ? num(kthContact(q.v0), "%.2f").c_str() : "-", q.left < 0 ? "  never left" : "");
    }
    float at = 0.0f;
    std::printf("the felt table's c^p against powf over 0.01 .. 4 mm: at p %.3f worst %.4f %%", sim.hammer().power(),
                100.0 * feltTableError(sim.hammer().power(), &at));
    std::printf(" (at %.3f mm);", at);
    for (float p : {2.0f, 2.25f, 2.5f, 2.75f, 3.0f, 3.25f, 3.5f}) std::printf(" p %.2f %.4f %%", p, 100.0 * feltTableError(p));
    std::printf("\n");
    return 0;
}

// ---- pickup ----------------------------------------------------------------------------------------

int cmdPickup(const Args& a) {
    const Tuning t = clamped(a.tuning);
    const Shape s = shapeOf(a.knobs, lawConstants(), a.sets, t);
    const float g = s[Channel::generator], gap = s[Channel::gap], off = s[Channel::offset];
    std::printf("%s\n", vectorText(a.knobs).c_str());
    std::printf("gap %.3f mm, offset %.3f mm, generator %+.2f: the curve's centre %.3f mm (the offset slid toward the gap by %.2f)\n", gap, off, g,
                pickupOffset(g, gap, off), clamp(g, 0.0f, 1.0f));
    std::printf("   x (mm)   C mag     C' mag    C elec    C' elec   C' at g\n");
    for (int i = -12; i <= 12; ++i) {
        const float x = 0.25f * i;
        const float cg = pickupVoltage(x, 1.0f, g, gap, off);
        std::printf("  %+6.2f %9.4f %+9.4f %9.4f %+9.4f %+9.4f\n", x, magneticC(x, gap, off), magneticDeriv(x, gap, off), electroC(x, gap),
                    electroDeriv(x, gap), cg);
    }
    double h[8];
    if (a.fromHammer) {
        Contact sim(s, a.midi, t);
        const ContactRun& r = sim.strike(a.vel, 0.101);
        const int from = r.left >= 0 ? r.left : 0, count = (int)(0.1 * sim.fs());
        double pk = 0.0;
        for (int k = from; k < from + count && k < (int)r.tip.size(); ++k) pk = std::fmax(pk, std::fabs(1e3 * r.tip[(size_t)k]));
        const std::vector<double> v = pickupOf(r, g, gap, off, from, count);
        const double f1 = (double)sim.table().ratio[0] * sim.rank().f0;
        const double thd = thdOf(v, f1, sim.fs(), 8, h);
        std::printf("the tip from the simulator at %s, velocity %.2f: contact %.3f ms, peak tip swing %.4f mm over the first 100 ms after it left\n",
                    noteName(a.midi).c_str(), a.vel, r.contactMs(), pk);
        std::printf("harmonics of %.2f hz, db against the first:", f1);
        for (int i = 0; i < 8; ++i) std::printf(" %+.1f", dbOf(h[i] / h[0]));
        std::printf("\nthd %.3f %%\n", 100.0 * thd);
        return 0;
    }
    const double thd = sineThd(a.amp, g, gap, off, h);
    std::printf("a sine of %.3f mm: harmonics db against the first:", a.amp);
    for (int i = 0; i < 8; ++i) std::printf(" %+.1f", dbOf(h[i] / h[0]));
    std::printf("\n  the first %.4g, thd %.3f %%\n", h[0], 100.0 * thd);
    std::printf("the ladder:\n  amp (mm)   thd (%%)   h2 (db re h1)   h1      h2\n");
    for (float amp : {0.05f, 0.1f, 0.2f, 0.5f, 1.0f, 2.0f}) {
        const double th = sineThd(amp, g, gap, off, h);
        std::printf("  %8.2f %9.3f %+13.1f %9.4g %9.4g\n", amp, 100.0 * th, dbOf(h[1] / h[0]), h[0], h[1]);
    }
    return 0;
}

// ---- board -----------------------------------------------------------------------------------------

int cmdBoard(const Args& a) {
    const Shape s = shapeOf(a.knobs, lawConstants(), a.sets, clamped(a.tuning));
    const BoardTable t = boardOf(s[Channel::boardArea], s[Channel::boardLoss], s[Channel::open]);
    std::printf("%s\n", vectorText(a.knobs).c_str());
    std::printf("area %.3f m², a %.3f m along the grain, b %.3f m across; eta %.4f, open %.2f; the radiation's -3 db low edge %.1f hz\n",
                s[Channel::boardArea], t.a, t.b, s[Channel::boardLoss], s[Channel::open], radiationEdge(s[Channel::open]));
    float out[kBoardModes];
    outputWeights(t, out);
    std::printf("mode   m   n        hz    t60 (s)  radiation (db)  listening weight\n");
    for (int k = 0; k < kBoardModes; ++k) {
        const BoardMode& m = t.mode[k];
        std::printf("%4d %3d %3d %9.2f %10.3f %15.2f %+17.4f\n", k, m.m, m.n, m.hz, m.t60, dbOf(m.radiation), out[k]);
    }
    std::printf("the bridge-line weights (the first twelve modes; the rms over all %d):\n", kBoardModes);
    const float notes[9] = {21, 33, 45, 57, 69, 81, 93, 105, 108};
    for (float n : notes) {
        float x, y, w[kBoardModes];
        bridgePoint(n, t, x, y);
        inputWeights(t, x, y, w);
        double ss = 0.0;
        for (int k = 0; k < kBoardModes; ++k) ss += (double)w[k] * w[k];
        std::printf("  %-3s (%.3f, %.3f) m:", noteName(n).c_str(), x, y);
        for (int k = 0; k < 12 && k < kBoardModes; ++k) std::printf(" %+.2f", w[k]);
        std::printf("   rms %.3f\n", std::sqrt(ss / kBoardModes));
    }
    // the shelf: the swept transfer at c4's bridge point over the top mode's octave and over 2 .. 4 f_top
    const float area = s[Channel::boardArea], loss = s[Channel::boardLoss], open = s[Channel::open];
    const double top = t.mode[kBoardModes - 1].hz;
    const SweepRuns under = sweepRuns(area, loss, open, 60.0f, 0.5 * top, top), over = sweepRuns(area, loss, open, 60.0f, 2.0 * top, 4.0 * top);
    const double atBuilt[2] = {bandAverageDb(under, kBoardShelf), bandAverageDb(over, kBoardShelf)};
    const double want = shelfFor(under, over);
    std::printf("the shelf: the top mode %.1f hz; kBoardShelf %.5g um/s per N; the infinite plate's mobility 1 / (8 sqrt(H rho h)) %.4g m/s per N "
                "(%.5g um/s per N)\n",
                top, (double)kBoardShelf, plateMobility(), 1e6 * plateMobility());
    std::printf("  the swept transfer |out / F| at c4's bridge point, the mean of its db: over %.0f .. %.0f hz (%zu points) %+.2f db, over %.0f .. %.0f "
                "hz (%zu points) %+.2f db (%+.2f apart); the shelf's mobility that makes them equal here %.5g um/s per N\n",
                0.5 * top, top, under.hz.size(), atBuilt[0], 2.0 * top, 4.0 * top, over.hz.size(), atBuilt[1], atBuilt[0] - atBuilt[1], want);
    // the field: its lines, its gain, its impulse energy against the shelf's share (no highpass, 0.5 s)
    Board fb;
    fb.init(kDefaultFs);
    fb.setPlate(area, loss, open);
    const double t60 = t.mode[kBoardModes - 1].t60;
    std::printf("the field: %d lines, %d samples (length, g at dc, a):", kFieldLines, kFieldSamples);
    for (int i = 0; i < kFieldLines; ++i) {
        float g, p;
        fb.fieldLine(i, g, p);
        std::printf("%s%3d (%.4f, %.4f)", i % 4 ? "  " : "\n  ", kFieldLengths[i], g, p);
    }
    std::printf("\n  the top mode's t60 %.1f ms, the plate's at %.0f hz %.1f ms; its gain kFieldGain %.5g um/s per N at every body\n", 1000.0 * t60,
                (double)kFieldFitHz, 1000.0 * t60 * top / kFieldFitHz, (double)kFieldGain);
    double energy = 0.0;
    for (int i = 0; i < (int)(0.5 * kDefaultFs); ++i) {
        const double y = fb.fieldStep(i == 0 ? 1.0f : 0.0f);
        energy += y * y;
    }
    const double share = (1.0 - (double)kFieldDirect * kFieldDirect) * (double)kBoardShelf * kBoardShelf;
    std::printf("  its impulse energy %.5g (um/s per N)² against the shelf's share (1 - kFieldDirect²) kBoardShelf² %.5g (%+.2f db): kFieldGain %.5g "
                "matches here\n",
                energy, share, 10.0 * std::log10(energy / share), kFieldGain * std::sqrt(share / energy));
    // 1 N for one sample at c4's bridge point: the output's energy by each time, of the first 300 ms's
    {
        Board rb;
        rb.init(kDefaultFs);
        rb.setPlate(area, loss, open);
        float w[kBoardModes];
        rb.inputWeights(60.0f, w);
        const int n = (int)(0.3 * kDefaultFs);
        std::vector<float> f((size_t)n, 0.0f), y((size_t)n);
        f[0] = 1.0f;
        const float* fp[1] = {f.data()};
        const float* wp[1] = {w};
        rb.render(fp, wp, 1, y.data(), n);
        std::vector<double> sum((size_t)n);
        double e = 0.0;
        for (int i = 0; i < n; ++i) sum[(size_t)i] = e += (double)y[(size_t)i] * y[(size_t)i];
        auto by = [&](double sec) { return 100.0 * sum[(size_t)std::max(0, (int)std::lround(sec * kDefaultFs) - 1)] / e; };
        std::printf("  1 N for one sample at c4's bridge point, the output's energy of the first 300 ms's: the first sample %.1f %%, 1 ms %.1f %%, "
                    "5 ms %.1f %%, 20 ms %.1f %%, 50 ms %.1f %%\n",
                    100.0 * sum[0] / e, by(0.001), by(0.005), by(0.02), by(0.05));
    }
    // the swept transfer's means with the field (the shelf's mobility at kBoardShelf) and with the modes alone (at 0)
    const SweepRuns above = sweepRuns(area, loss, open, 60.0f, 4.0 * top, 8.0 * top);
    std::printf("  the swept transfer's means (db) over f_top / 2 .. f_top, 2 .. 4 f_top, 4 .. 8 f_top: with the field %+.2f, %+.2f, %+.2f; the modes "
                "alone %+.2f, %+.2f, %+.2f; a flat shelf at kBoardShelf %+.2f\n",
                atBuilt[0], atBuilt[1], bandAverageDb(above, kBoardShelf), bandAverageDb(under, 0.0), bandAverageDb(over, 0.0),
                bandAverageDb(above, 0.0), dbfs((double)kBoardShelf * kBoardOut));
    if (!a.sweep) return 0;
    // the table at the three bodies
    const std::string path = a.out.empty() ? "board_sweep.txt" : a.out;
    makeDirs(std::filesystem::path(path).parent_path().string());
    FILE* f = std::fopen(path.c_str(), "w");
    if (!f) { std::fprintf(stderr, "cannot write %s\n", path.c_str()); return 2; }
    std::fprintf(f, "# the board's swept transfer |out / F| (db full scale per N): a unit force sine in at c4's bridge point, 1/12 octave from %.0f\n"
                    "# hz to %.0f khz, each run to steady state, the output's rms read; at body -1, 0 and +1 (the eight at grand's but the body),\n"
                    "# kBoardShelf %.5g um/s per N, kBoardOut %.5g; the infinite plate's mobility %.4g m/s per N\n",
                 kSweepLo, kSweepHi / 1000.0, (double)kBoardShelf, (double)kBoardOut, plateMobility());
    float bodies[3][3];
    double tops[3];
    for (int b = 0; b < 3; ++b) {
        Knobs k = a.knobs;
        k.v[(int)Knob::body] = (float)(b - 1);
        const Shape sb = pianoOf(k);
        bodies[b][0] = sb[Channel::boardArea];
        bodies[b][1] = sb[Channel::boardLoss];
        bodies[b][2] = sb[Channel::open];
        tops[b] = boardOf(bodies[b][0], bodies[b][1], bodies[b][2]).mode[kBoardModes - 1].hz;
        std::fprintf(f, "# body %+d: area %.3f m2, loss %.4f, open %.2f, the top mode %.1f hz\n", b - 1, bodies[b][0], bodies[b][1], bodies[b][2], tops[b]);
    }
    std::fprintf(f, "%10s %10s %10s %10s\n", "hz", "body -1", "body 0", "body +1");
    for (double hz : sweepHz()) {
        std::fprintf(f, "%10.2f", hz);
        for (int b = 0; b < 3; ++b) std::fprintf(f, " %+10.2f", dbfs(boardTransfer(bodies[b][0], bodies[b][1], bodies[b][2], 60.0f, hz, kBoardShelf)));
        std::fprintf(f, "\n");
    }
    for (int b = 0; b < 3; ++b) {
        const SweepRuns u = sweepRuns(bodies[b][0], bodies[b][1], bodies[b][2], 60.0f, 0.5 * tops[b], tops[b]);
        const SweepRuns o = sweepRuns(bodies[b][0], bodies[b][1], bodies[b][2], 60.0f, 2.0 * tops[b], 4.0 * tops[b]);
        std::fprintf(f, "# body %+d: the mean over f_top / 2 .. f_top %+.2f db, over 2 .. 4 f_top %+.2f db (%+.2f apart)\n", b - 1,
                     bandAverageDb(u, kBoardShelf), bandAverageDb(o, kBoardShelf), bandAverageDb(u, kBoardShelf) - bandAverageDb(o, kBoardShelf));
    }
    std::fclose(f);
    std::printf("written: %s\n", path.c_str());
    return 0;
}

// ---- print, random ---------------------------------------------------------------------------------

int cmdPrint(const Args& a) {
    const bool own = a.preset && std::memcmp(&a.knobs, &a.preset->knobs, sizeof a.knobs) == 0;
    std::printf("%s\n", own ? knobLine(a.preset->name, a.preset->knobs, a.preset->lo, a.preset->hi).c_str() : knobLine("vector", a.knobs, 21, 108).c_str());
    if (!a.shapeOut) return 0;
    const Shape s = pianoOf(a.knobs, clamped(a.tuning));
    for (int i = 0; i < kInstrumentChannels; ++i) std::printf("  %-12s %.6g\n", channelName((Channel)i), s.v[i]);
    const Knobs back = knobsOf(s);
    std::printf("  read back:  %s\n", vectorText(back).c_str());
    return 0;
}

int cmdRandom(const Args& a) {
    Rng r;
    r.seed(a.seed);
    const int n = a.count > 0 ? a.count : 1;
    std::printf("%d vectors drawn from seed %u; each with its B at midi %.0f, its first partial's t60, its 8th partial's gain, its strings\n"
                "and partials on; then each heard 2 s at midi %.0f, velocity 1, its level undone in the gain (10^(-levelDb / 20)): the peak\n"
                "(undone), the first half second's rms against grand's octave c4 .. b4 (db, before the undoing), levelDb, the level undone\n"
                "(the rms's db less levelDb), and whether it sounds (the peak undone over %.3f, the last 100 ms's rms under the first 100\n"
                "ms's, finite); beside it the octave from midi %.0f up as the level table reads one (the mean of the twelve notes' powers)\n"
                "and that undone\n",
                n, (unsigned)a.seed, a.midi, a.midi, kSoundPeak, a.midi);
    int sounding = 0;
    double lo = 1e9, hi = -1e9, sum = 0.0, sq = 0.0, olo = 1e9, ohi = -1e9, osum = 0.0, osq = 0.0;
    for (int i = 0; i < n; ++i) {
        const Knobs k = drawKnobs(r);
        const At x = atNote(k, Spec(), a.midi);
        std::printf("%s\n", knobLine("vector", k, 21, 108).c_str());
        std::printf("    at %s: B %.4g, partial 1's t60 %.3f s, partial 8's gain %s, strings %.2f, %d partials on\n",
                    noteName(a.midi).c_str(), x.r.B, x.p.t60[0] * x.r.t60Scale,
                    x.p.gain[7] != 0.0f ? num(dbOf(x.p.gain[7]), "%+.1f db").c_str() : "off", stringsOf(x.s), partialsOnOf(x.p));
        const Heard h = hearVector(k, a.midi, 2.0f, true, a.tuning);
        sounding += h.sounds.sounds;
        lo = std::fmin(lo, h.undone);
        hi = std::fmax(hi, h.undone);
        sum += h.undone;
        sq += h.undone * h.undone;
        const OctaveHeard o = hearOctave(k, a.midi, a.tuning);
        olo = std::fmin(olo, o.undone);
        ohi = std::fmax(ohi, o.undone);
        osum += o.undone;
        osq += o.undone * o.undone;
        std::printf("    heard: peak %.3f, rms %+.1f db, levelDb %+.1f, undone %+.1f db; %s; the octave %+.1f db, undone %+.1f db\n", h.sounds.peak,
                    h.level, h.table, h.undone,
                    h.sounds.sounds ? "sounds" : (!h.sounds.finite ? "NOT FINITE" : (!h.sounds.decaying ? "does not decay" : "under the peak")),
                    o.level, o.undone);
    }
    const double mean = sum / n, omean = osum / n;
    std::printf("%d of %d sound; the level undone %+.1f .. %+.1f db (mean %+.2f, standard deviation %.2f); the octave undone %+.1f .. %+.1f db "
                "(mean %+.2f, standard deviation %.2f)\n",
                sounding, n, lo, hi, mean, std::sqrt(std::fmax(0.0, sq / n - mean * mean)), olo, ohi, omean,
                std::sqrt(std::fmax(0.0, osq / n - omean * omean)));
    return 0;
}

// ---- fit -------------------------------------------------------------------------------------------

void printFit(const RefTable& t, const KnobFit& f) {
    std::printf("%s (%s)\n  source: %s\n", t.title.c_str(), t.kind.c_str(), t.source.c_str());
    std::printf("  at the anchor: %.5f; fitted: %.5f (rms %.4g %s)\n", f.atAnchor.value, f.error.value, f.error.rms, residualUnit(t));
    std::printf("  vector: %s\n", vectorText(f.k).c_str());
    std::printf("  entry                         table          law     residual\n");
    for (const Residual& r : f.error.items) std::printf("  %-26s %12.5g %12.5g %+12.4f\n", r.label.c_str(), r.ref, r.law, r.err);
    if (t.kind == "B") {
        double worst = 0.0;
        for (const Residual& r : f.error.items) worst = std::fmax(worst, std::fabs(std::exp(r.err) - 1.0));
        std::printf("  the worst note %.1f %% off\n", 100.0 * worst);
    }
    std::printf("  %s\n", knobLine("vector", f.k, 21, 108).c_str());
}

void printConstants(FILE* f, const char* title, const LawConstants& c, const LawConstants* before = nullptr) {
    std::fprintf(f, "%s\n", title);
    for (const ConstantSpec& cs : constantSpecs()) {
        std::fprintf(f, "  %-12s %12.6g", cs.name, (double)(c.*(cs.field)));
        if (before) std::fprintf(f, "   (from %.6g)", (double)(before->*(cs.field)));
        std::fprintf(f, "\n");
    }
}

// a B table's own best tension: the tension and the table's scale knob fitted together
double tensionFor(const RefTable& t, const LawConstants& c, double& errOut, Knobs& kOut) {
    LawConstants x = c;
    double best = 1e30, bestT = c.tension;
    for (float T = 200.0f; T <= 1600.0f; T += 10.0f) {
        x.tension = T;
        const KnobFit f = fitTable(t, x, constantsSpec(t));
        if (f.error.value < best) { best = f.error.value; bestT = T; kOut = f.k; }
    }
    errOut = best;
    return bestT;
}

int cmdFitConstants(const Args& a) {
    const std::string dir = a.tables.empty() ? refDir() : a.tables;
    std::vector<TableFitState> tables;
    for (const std::string& path : tableFiles(dir)) {
        RefTable t;
        std::string err;
        if (!readTable(path, t, err)) { std::fprintf(stderr, "%s\n", err.c_str()); continue; }
        if (!fitsKnobs(t) || (!a.kind.empty() && t.kind != a.kind)) continue;
        TableFitState s;
        s.table = t;
        s.spec = constantsSpec(t);
        tables.push_back(s);
    }
    if (tables.empty()) { std::fprintf(stderr, "no fit tables in %s\n", dir.c_str()); return 1; }
    const LawConstants before = lawConstants();
    std::vector<TableFitState> start = tables;
    for (TableFitState& t : start) t.fit = fitTable(t.table, before, t.spec);
    int passes = 0;
    const LawConstants after = fitConstants(tables, before, &passes, a.fix);
    if (!a.fix.empty()) std::printf("held where they stand: %s\n", a.fix.c_str());
    printConstants(stdout, "the constants before:", before);
    printConstants(stdout, "the constants after:", after, &before);
    std::printf("the tables, each at its own vector (the error's value; rms in its units):\n");
    for (size_t i = 0; i < tables.size(); ++i) {
        std::printf("  %-26s before %9.5f (rms %.4g)  after %9.5f (rms %.4g)   %s\n", tables[i].table.name.c_str(), start[i].fit.error.value,
                    start[i].fit.error.rms, tables[i].fit.error.value, tables[i].fit.error.rms, vectorText(tables[i].fit.k).c_str());
    }
    std::printf("each B table's own tension (the tension fitted with the table's free knobs at its holds, the other constants held):\n");
    for (int which = 0; which < 2; ++which) {
        const LawConstants& c = which ? after : before;
        std::printf("  under the constants %s:\n", which ? "after" : "before");
        for (size_t i = 0; i < start.size(); ++i) {
            const TableFitState& t = which ? tables[i] : start[i];
            if (t.table.kind != "B" || bPoints(t.table).empty()) continue;
            double e;
            Knobs k{};
            const double T = tensionFor(t.table, c, e, k);
            std::printf("    %-26s %6.0f N, error %.5f at scale %+.3f (at %.0f N: %.5f)\n", t.table.name.c_str(), T, e, k.v[0], c.tension,
                        t.fit.error.value);
        }
    }
    std::printf("the entries before and after:\n");
    for (size_t i = 0; i < tables.size(); ++i) {
        std::printf("  %s\n", tables[i].table.name.c_str());
        const auto& b = start[i].fit.error.items;
        const auto& c = tables[i].fit.error.items;
        for (size_t j = 0; j < c.size(); ++j)
            std::printf("    %-26s table %11.5g  law %11.5g -> %11.5g   residual %+9.4f -> %+9.4f\n", c[j].label.c_str(), c[j].ref,
                        j < b.size() ? b[j].law : NAN, c[j].law, j < b.size() ? b[j].err : NAN, c[j].err);
    }
    std::printf("%d passes\n", passes);
    const std::string outPath = a.out.empty() ? "constants.txt" : a.out;
    makeDirs(std::filesystem::path(outPath).parent_path().string());
    FILE* f = std::fopen(outPath.c_str(), "w");
    if (!f) { std::fprintf(stderr, "cannot write %s\n", outPath.c_str()); return 1; }
    std::fprintf(f, "# the LawConstants block uptool fit --constants proposes against %s%s%s\n", dir.c_str(), a.kind.empty() ? "" : ", the tables of kind ",
                 a.kind.c_str());
    printConstants(f, "# before:", before);
    printConstants(f, "# proposed:", after, &before);
    std::fclose(f);
    std::printf("written: %s\n", outPath.c_str());
    return 0;
}

int cmdFit(const Args& a) {
    if (a.constants) return cmdFitConstants(a);
    RefTable t;
    std::string err;
    if (!readTable(a.table, t, err)) { std::fprintf(stderr, "%s\n", err.c_str()); return 1; }
    if (!fitsKnobs(t)) { std::fprintf(stderr, "%s: a %s table has nothing to fit\n", a.table.c_str(), t.kind.c_str()); return 1; }
    const KnobFit f = fitTable(t, lawConstants(), a.lock, a.sets);
    printFit(t, f);
    const Heard h = hearVector(f.k, a.midi, 2.0f, true, a.tuning);
    std::printf("  heard 2 s at %s, velocity 1, its level undone: peak %.3f, rms %+.1f db against grand's octave, levelDb %+.1f, undone %+.1f db; "
                "%s\n",
                noteName(a.midi).c_str(), h.sounds.peak, h.level, h.table, h.undone,
                h.sounds.sounds ? "sounds" : (!h.sounds.finite ? "NOT FINITE" : (!h.sounds.decaying ? "does not decay" : "under the peak")));
    return 0;
}

// ---- chart -----------------------------------------------------------------------------------------

const float kChartNotes[3] = {21.0f, 60.0f, 96.0f};   // a0, c4, c7: the ceiling's columns and the checks

struct Strike {
    double contactMs = 0.0, peak = 0.0, thd = NAN, band = NAN;
    bool left = false, finite = true;
};

// a strike at the note: the engine's scheme, or the truth (schemeRate); the band; on the electric side (the
// engine's scheme) the tip through the pickup over the 100 ms after the hammer leaves, read at the sample rate
Strike strikeAt(const Shape& s, float midi, float vel, bool truth = false) {
    Strike st;
    const int rate = (int)schemeRate(truth);
    const bool electric = !truth && s[Channel::generator] > -1.0f;
    Contact sim(s, midi, Tuning(), lawConstants(), rate * kDefaultFs);
    if (!truth) engineScheme(sim);
    const ContactRun& r = sim.strike(vel, electric ? 0.101 : 0.0);
    st.contactMs = r.contactMs();
    st.peak = r.peak;
    st.left = r.left >= 0;
    st.finite = r.finite;
    st.band = bandDb(r, sim.table(), sim.rank().f0);
    if (electric && r.left >= 0) {
        ContactRun d;                                     // the state after each whole sample
        for (size_t k = (size_t)rate - 1; k < r.tip.size(); k += (size_t)rate) {
            d.tip.push_back(r.tip[k]);
            d.tipVel.push_back(r.tipVel[k]);
        }
        const int count = (int)(0.1 * kDefaultFs);
        const std::vector<double> v = pickupOf(d, sim.generator(), s[Channel::gap], s[Channel::offset], r.left / rate, count);
        st.thd = thdOf(v, (double)sim.table().ratio[0] * sim.rank().f0, kDefaultFs);
    }
    return st;
}

struct Cell {
    double B[3];                // c2, c4, c6
    double c10;                 // the 10th partial's cents off 10 f0 at c4
    double t60[3];              // partials 1, 2, 10 at c4, at the note
    double g8;                  // db
    double strings, detune;
    Strike v03, v1, v1t;        // the engine's scheme at 0.3 and 1, the truth at 1
    double board1, edge;
    int on[3];                  // a0, c4, c7
    // the checks
    int disorder = 0, t60bad = 0, gainBad = 0, ceilingBad = 0, notLeft = 0, notFinite = 0, offRate = 0;
};

void checkTable(const PartialTable& p, float f0, Cell& c) {
    float last = 0.0f;
    for (int k = 0; k < kMaxModes; ++k) {
        const float r = p.ratio[k];
        if (!std::isfinite(r) || r <= last) ++c.disorder;
        last = r;
        if (!(p.t60[k] > 0.0f) || !std::isfinite(p.t60[k])) ++c.t60bad;
        if (!(p.gain[k] >= -1.0f && p.gain[k] <= 1.0f)) ++c.gainBad;
        if ((double)r * f0 > kCeiling * kDefaultFs && partialOn(p, k)) ++c.ceilingBad;
    }
}

Cell cellOf(const Knobs& k) {
    Cell c;
    const Shape s = pianoOf(k);
    const float bn[3] = {36.0f, 60.0f, 84.0f};
    for (int i = 0; i < 3; ++i) c.B[i] = rankAt(s, bn[i]).B;
    const RankState r4 = rankAt(s, 60.0f);
    const PartialTable p4 = partialsOf(s, r4);
    c.c10 = centsOf(p4.ratio[kTenth] / (kTenth + 1.0));
    c.t60[0] = p4.t60[0] * r4.t60Scale;
    c.t60[1] = p4.t60[1] * r4.t60Scale;
    c.t60[2] = p4.t60[kTenth] * r4.t60Scale;
    c.g8 = p4.gain[7] != 0.0f ? dbOf(p4.gain[7]) : NAN;          // off: "-"
    c.strings = stringsOf(s);
    c.detune = s[Channel::detune];
    c.v03 = strikeAt(s, 60.0f, 0.3f);
    c.v1 = strikeAt(s, 60.0f, 1.0f);
    c.v1t = strikeAt(s, 60.0f, 1.0f, true);
    c.offRate = std::fabs(c.v1.peak / c.v1t.peak - 1.0) > 0.1 || std::fabs(c.v1.contactMs / c.v1t.contactMs - 1.0) > 0.1;
    c.notLeft = !c.v03.left + !c.v1.left + !c.v1t.left;
    c.notFinite = !c.v03.finite + !c.v1.finite + !c.v1t.finite;
    const BoardTable bt = boardOf(s[Channel::boardArea], s[Channel::boardLoss], s[Channel::open]);
    c.board1 = bt.mode[0].hz;
    c.edge = radiationEdge(s[Channel::open]);
    for (int i = 0; i < 3; ++i) {
        const RankState r = rankAt(s, kChartNotes[i]);
        const PartialTable p = partialsOf(s, r);
        c.on[i] = partialsOnOf(p);
        checkTable(p, r.f0, c);
    }
    return c;
}

std::string num(double x, const char* fmt) {
    if (!std::isfinite(x)) return "-";
    char b[32];
    std::snprintf(b, sizeof b, fmt, x);
    return b;
}

void cellHeader(FILE* f) {
    std::fprintf(f, "%6s %10s %10s %10s %8s %7s %7s %7s %7s %5s %6s %5s %7s %7s %7s %7s %7s %7s %7s %7s %6s %6s %6s %6s %3s %3s %3s\n", "x",
                 "B c2", "B c4", "B c6", "c10 c", "t60 n1", "n2", "n10", "g8 db", "str", "detune", "after", "ms .3", "N .3", "band .3", "ms 1",
                 "N 1", "band 1", "ms 1t", "N 1t", "thd .3", "thd 1", "brd1", "edge", "a0", "c4", "c7");
}

void cellRow(FILE* f, double x, const Cell& c) {
    std::fprintf(f,
                 "%+6.2f %10.4g %10.4g %10.4g %8.1f %7.2f %7.2f %7.3f %7s %5.2f %6.2f %5.2f %7.3f %7.1f %7s %7.3f %7.1f %7s %7.3f %7.1f %6s %6s "
                 "%6.1f %6.1f %3d %3d %3d\n",
                 x, c.B[0], c.B[1], c.B[2], c.c10, c.t60[0], c.t60[1], c.t60[2], num(c.g8, "%.1f").c_str(), c.strings, c.detune, kAftersound,
                 c.v03.contactMs, c.v03.peak, num(c.v03.band, "%+.1f").c_str(), c.v1.contactMs, c.v1.peak, num(c.v1.band, "%+.1f").c_str(),
                 c.v1t.contactMs, c.v1t.peak, num(100.0 * c.v03.thd, "%.2f").c_str(), num(100.0 * c.v1.thd, "%.2f").c_str(), c.board1, c.edge,
                 c.on[0], c.on[1], c.on[2]);
}

struct Tally {
    long cells = 0, disorder = 0, t60bad = 0, gainBad = 0, ceilingBad = 0, notLeft = 0, notFinite = 0, offRate = 0;
    double peak = 0.0, peak8 = 0.0;
    std::vector<std::string> first[7];
    void add(const Cell& c, const std::string& name) {
        ++cells;
        const int bad[7] = {c.disorder, c.t60bad, c.gainBad, c.ceilingBad, c.notLeft, c.notFinite, c.offRate};
        long* counts[7] = {&disorder, &t60bad, &gainBad, &ceilingBad, &notLeft, &notFinite, &offRate};
        for (int i = 0; i < 7; ++i)
            if (bad[i]) {
                ++*counts[i];
                if (first[i].size() < 5) first[i].push_back(name);
            }
        peak = std::fmax(peak, std::fmax(c.v03.peak, c.v1.peak));
        peak8 = std::fmax(peak8, c.v1t.peak);
    }
};

// ---- chart --audio ------------------------------------------------------------------------------------

// one cell of the audio chart: the table's measures at the note (the law's 10th partial in cents off 10 f0, the
// fundamental's t60 at the note, the contact and the peak at velocity 1 through the engine's scheme, the thd of
// the engine's tip on the electric side) and the render's (2 s at velocity 1: the tracker's 10th partial and
// fundamental's t60, the telemetry's contact and peak, play's thd)
struct AudioCell {
    double c10 = NAN, t60 = NAN, contact = NAN, peak = NAN, thd = NAN;      // the table's
    double rc10 = NAN, rt60 = NAN, rcontact = NAN, rpeak = NAN, rthd = NAN; // the render's
};

AudioCell audioCellOf(const Knobs& k, float midi) {
    AudioCell c;
    const Shape s = pianoOf(k);
    const RankState r = rankAt(s, midi);
    const PartialTable p = partialsOf(s, r);
    if (partialOn(p, kTenth) && (double)p.ratio[kTenth] * r.f0 < kCeiling * kDefaultFs) c.c10 = centsOf(p.ratio[kTenth] / (kTenth + 1.0));
    c.t60 = p.t60[0] * r.t60Scale;
    const Strike st = strikeAt(s, midi, 1.0f);
    c.contact = st.contactMs;
    c.peak = st.peak;
    c.thd = st.thd;
    PlaySpec sp;
    sp.instrument = s;
    sp.midi = midi;
    sp.sec = 2.0f;
    const PlayRender pr = renderPlay(sp);
    const PlayReading rd = readPlay(pr, sp);
    for (size_t i = 0; i < rd.slot.size(); ++i) {
        if (rd.slot[i] == kTenth && rd.tracked[i].hz > 0.0) c.rc10 = centsOf(rd.tracked[i].hz / ((kTenth + 1.0) * pr.f0));
        if (rd.slot[i] == 0 && rd.tracked[i].decay.t60 > 0.0) c.rt60 = rd.tracked[i].decay.t60;
    }
    c.rcontact = pr.contactMs;
    c.rpeak = pr.peakForce;
    c.rthd = rd.thd;
    return c;
}

int cmdChartAudio(const std::string& dir) {
    makeDirs(dir);
    struct Worst {
        double by = 0.0;
        std::string at, text;
    };
    Worst worst[5];
    const char* what[5] = {"the 10th partial's cents (render - table)", "the fundamental's t60 (render / table, in octaves)",
                           "the contact (render / table - 1)", "the peak force (render / table - 1)", "the thd (render - table, points)"};
    int cells = 0;
    for (int k = 0; k < kKnobCount; ++k) {
        const std::string path = dir + "/audio_" + knobName((Knob)k) + ".txt";
        FILE* f = std::fopen(path.c_str(), "w");
        if (!f) { std::fprintf(stderr, "cannot write %s\n", path.c_str()); return 2; }
        std::fprintf(f, "# %s alone in tenths at the three anchors, rendered 2 s at velocity 1 at c4 (e3 at rhodes and wurlitzer), as play renders\n"
                        "# it: beside the table's measures at the note (t: the law's 10th partial in cents off 10 f0, the fundamental's t60 at\n"
                        "# the note, the contact and the peak at velocity 1 through the engine's scheme, the thd of its tip on the electric\n"
                        "# side), the render's (r: the tracker's 10th partial and fundamental's t60 (- where 2 s cannot read it), the\n"
                        "# telemetry's contact and peak, play's thd over the 100 ms after the hammer leaves)\n",
                     knobName((Knob)k));
        for (int an = 0; an < kAnchorCount; ++an) {
            const float midi = an == 0 ? 60.0f : 52.0f;
            std::fprintf(f, "\n# at %s, %s: %s\n", knobAnchors()[an].name, noteName(midi).c_str(), vectorText(knobAnchors()[an].k).c_str());
            std::fprintf(f, "%6s %8s %8s %8s %8s %8s %8s %8s %8s %8s %8s\n", "x", "c10 t", "c10 r", "t60 t", "t60 r", "ms t", "ms r", "N t", "N r", "thd t",
                         "thd r");
            for (int j = 0; j <= 20; ++j) {
                const float x = (float)(j - 10) / 10.0f;
                Knobs v = knobAnchors()[an].k;
                v.v[k] = x;
                const AudioCell c = audioCellOf(v, midi);
                ++cells;
                std::fprintf(f, "%+6.1f %8s %8s %8s %8s %8.3f %8.3f %8.2f %8.2f %8s %8s\n", x, num(c.c10, "%+.1f").c_str(), num(c.rc10, "%+.1f").c_str(),
                             num(c.t60, "%.3f").c_str(), num(c.rt60, "%.3f").c_str(), c.contact, c.rcontact, c.peak, c.rpeak,
                             num(100.0 * c.thd, "%.2f").c_str(), num(100.0 * c.rthd, "%.2f").c_str());
                char where[96];
                std::snprintf(where, sizeof where, "%s %+.1f at %s (%s)", knobName((Knob)k), x, knobAnchors()[an].name, noteName(midi).c_str());
                const double d[5] = {c.rc10 - c.c10, std::log2(c.rt60 / c.t60), c.rcontact / c.contact - 1.0, c.rpeak / c.peak - 1.0, 100.0 * (c.rthd - c.thd)};
                for (int m = 0; m < 5; ++m) {
                    if (!std::isfinite(d[m]) || std::fabs(d[m]) <= std::fabs(worst[m].by)) continue;
                    worst[m].by = d[m];
                    worst[m].at = where;
                    char b[160];
                    if (m == 0) std::snprintf(b, sizeof b, "table %+.1f, render %+.1f cents", c.c10, c.rc10);
                    if (m == 1) std::snprintf(b, sizeof b, "table %.3f s, render %.3f s", c.t60, c.rt60);
                    if (m == 2) std::snprintf(b, sizeof b, "table %.3f ms, render %.3f ms", c.contact, c.rcontact);
                    if (m == 3) std::snprintf(b, sizeof b, "table %.2f N, render %.2f N", c.peak, c.rpeak);
                    if (m == 4) std::snprintf(b, sizeof b, "table %.2f %%, render %.2f %%", 100.0 * c.thd, 100.0 * c.rthd);
                    worst[m].text = b;
                }
            }
        }
        std::fclose(f);
        std::printf("  %s\n", path.c_str());
    }
    const std::string path = dir + "/audio_summary.txt";
    FILE* f = std::fopen(path.c_str(), "w");
    if (!f) return 2;
    std::fprintf(f, "# the table's measures against the render's over the 1-d sweeps: %d cells, each rendered 2 s at velocity 1 (c4 at grand, e3 at\n"
                    "# rhodes and wurlitzer). the worst disagreement of each measure, and where\n", cells);
    for (int m = 0; m < 5; ++m) std::fprintf(f, "%-52s %+9.3f  %s: %s\n", what[m], worst[m].by, worst[m].at.c_str(), worst[m].text.c_str());
    std::fclose(f);
    std::printf("chart --audio: %d cells rendered into %s\n", cells, dir.c_str());
    for (int m = 0; m < 5; ++m) std::printf("  the worst %s %+.3f: %s (%s)\n", what[m], worst[m].by, worst[m].at.c_str(), worst[m].text.c_str());
    return 0;
}

int cmdChart(const Args& a) {
    const std::string dir = a.dir.empty() ? "chart" : a.dir;
    if (a.audio) return cmdChartAudio(dir);
    makeDirs(dir);
    Tally tally;
    // each knob alone in tenths at the three anchors
    Cell ends[kAnchorCount][kKnobCount][2];
    for (int k = 0; k < kKnobCount; ++k) {
        const std::string path = dir + "/" + knobName((Knob)k) + ".txt";
        FILE* f = std::fopen(path.c_str(), "w");
        if (!f) { std::fprintf(stderr, "cannot write %s\n", path.c_str()); return 1; }
        std::fprintf(f, "# %s: %s · %s · %s, alone in tenths at each anchor\n", knobName((Knob)k), knobEnd((Knob)k, 0), knobEnd((Knob)k, 1),
                     knobEnd((Knob)k, 2));
        std::fprintf(f, "# B at c2, c4, c6; the 10th partial's cents off 10 f0 at c4; t60 (s, at the note) of partials 1, 2, 10 at c4; the\n"
                        "# 8th partial's gain at c4 (db); the strings (capped), the detune (cents), the tonebar's aftersound (kAftersound); the\n"
                        "# hammer's contact (ms) and peak force (N) at c4 at velocity 0.3 and 1 (up_contact.h); the thd (%%) at velocity\n"
                        "# 0.3 and 1 of the tip through the pickup over the first 100 ms after the hammer leaves (the electric side);\n"
                        "# the board's first mode and the radiation's -3 db low edge (hz); the partials on at a0, c4, c7\n");
        for (int an = 0; an < kAnchorCount; ++an) {
            std::fprintf(f, "\n# at %s: %s\n", knobAnchors()[an].name, vectorText(knobAnchors()[an].k).c_str());
            cellHeader(f);
            for (int i = 0; i <= 20; ++i) {
                Knobs kv = knobAnchors()[an].k;
                const float x = (float)(i - 10) / 10.0f;
                kv.v[k] = x;
                const Cell c = cellOf(kv);
                cellRow(f, x, c);
                char name[96];
                std::snprintf(name, sizeof name, "%s %s %+.1f", knobAnchors()[an].name, knobName((Knob)k), x);
                tally.add(c, name);
                if (i == 0) ends[an][k][0] = c;
                if (i == 20) ends[an][k][1] = c;
            }
        }
        std::fclose(f);
    }
    // the pairs in fifths
    struct Pair { Knob a, b; int anchor; };
    const Pair pairs[5] = {{Knob::scale, Knob::hammer, 0}, {Knob::strike, Knob::unison, 0}, {Knob::body, Knob::scale, 0},
                           {Knob::generator, Knob::pickup, 1}, {Knob::hammer, Knob::pickup, 1}};
    const float fifths[5] = {-1.0f, -0.5f, 0.0f, 0.5f, 1.0f};
    for (const Pair& p : pairs) {
        const std::string path = dir + "/pair_" + knobName(p.a) + "_" + knobName(p.b) + ".txt";
        FILE* f = std::fopen(path.c_str(), "w");
        if (!f) return 1;
        std::fprintf(f, "# %s x %s in fifths at %s: B at c4, the 10th partial's cents, the 8th's gain (db), the contact (ms), the\n"
                        "# peak force (N) and the band (db) at velocity 1 through the engine's scheme, the contact and the peak by the\n"
                        "# truth (32 fs), the thd (%%) at velocity 1\n",
                     knobName(p.a), knobName(p.b), knobAnchors()[p.anchor].name);
        std::fprintf(f, "%6s %6s %10s %8s %7s %7s %8s %7s %7s %8s %7s\n", knobName(p.a), knobName(p.b), "B c4", "c10 c", "g8 db", "ms 1", "N 1",
                     "band 1", "ms 1t", "N 1t", "thd 1");
        for (float x : fifths)
            for (float y : fifths) {
                Knobs kv = knobAnchors()[p.anchor].k;
                kv.v[(int)p.a] = x;
                kv.v[(int)p.b] = y;
                const Cell c = cellOf(kv);
                std::fprintf(f, "%+6.1f %+6.1f %10.4g %8.1f %7s %7.3f %8.1f %7s %7.3f %8.1f %7s\n", x, y, c.B[1], c.c10, num(c.g8, "%.1f").c_str(),
                             c.v1.contactMs, c.v1.peak, num(c.v1.band, "%+.1f").c_str(), c.v1t.contactMs, c.v1t.peak,
                             num(100.0 * c.v1.thd, "%.2f").c_str());
                char name[96];
                std::snprintf(name, sizeof name, "%s %s %+.1f %s %+.1f", knobAnchors()[p.anchor].name, knobName(p.a), x, knobName(p.b), y);
                tally.add(c, name);
            }
        std::fclose(f);
    }
    // the summary
    {
        const std::string path = dir + "/summary.txt";
        FILE* f = std::fopen(path.c_str(), "w");
        if (!f) return 1;
        std::fprintf(f, "# every cell of every table: its partial tables at a0, c4 and c7, its three strikes at c4 (the engine's scheme at\n"
                        "# velocity 0.3 and 1, the truth at 1)\n");
        std::fprintf(f, "cells %ld\n", tally.cells);
        const char* what[7] = {"a ratio not finite or not rising in slot", "a t60 not positive", "a gain outside -1 .. 1",
                               "a partial over the ceiling on", "the hammer not leaving within kMaxContact", "a force not finite",
                               "the engine's scheme over 10 % off the truth at velocity 1 (peak or contact)"};
        const long counts[7] = {tally.disorder, tally.t60bad, tally.gainBad, tally.ceilingBad, tally.notLeft, tally.notFinite, tally.offRate};
        for (int i = 0; i < 7; ++i) {
            std::fprintf(f, "%-44s %6ld", what[i], counts[i]);
            for (const std::string& n : tally.first[i]) std::fprintf(f, "  [%s]", n.c_str());
            std::fprintf(f, "\n");
        }
        std::fprintf(f, "the largest peak force over every cell %.1f N through the engine's scheme (%.1f N by the truth, velocity 1); "
                        "kForceCap %.0f N (%.1f x the largest)\n",
                     tally.peak, tally.peak8, kForceCap, kForceCap / tally.peak);
        std::fclose(f);
    }
    // the floors
    {
        const std::string path = dir + "/floors.txt";
        FILE* f = std::fopen(path.c_str(), "w");
        if (!f) return 1;
        const Knobs grand = knobAnchors()[0].k, rhodes = knobAnchors()[1].k;
        auto with = [](Knobs k, Knob which, float x) { k.v[(int)which] = x; return k; };
        // scale: the 10th partial of c2 in cents end to end
        auto c10at = [](const Knobs& k, float midi) {
            const Shape s = pianoOf(k);
            return centsOf(partialsOf(s, rankAt(s, midi)).ratio[kTenth] / (kTenth + 1.0));
        };
        const double scaleSpan = std::fabs(c10at(with(grand, Knob::scale, 1.0f), 36.0f) - c10at(with(grand, Knob::scale, -1.0f), 36.0f));
        // hammer: the band (partials 8 .. 16 against 1 .. 3) by the truth (32 fs) and by the engine's scheme
        struct Bands { double soft03, soft05, soft1, med03, med05, med1, hard05, hard1; };
        auto bands = [&](bool truth) {
            const Shape soft = pianoOf(with(grand, Knob::hammer, -1.0f)), med = pianoOf(grand), hard = pianoOf(with(grand, Knob::hammer, 1.0f));
            Bands x;
            x.soft03 = strikeAt(soft, 60.0f, 0.3f, truth).band;
            x.soft05 = strikeAt(soft, 60.0f, 0.5f, truth).band;
            x.soft1 = strikeAt(soft, 60.0f, 1.0f, truth).band;
            x.med03 = strikeAt(med, 60.0f, 0.3f, truth).band;
            x.med05 = strikeAt(med, 60.0f, 0.5f, truth).band;
            x.med1 = strikeAt(med, 60.0f, 1.0f, truth).band;
            x.hard05 = strikeAt(hard, 60.0f, 0.5f, truth).band;
            x.hard1 = strikeAt(hard, 60.0f, 1.0f, truth).band;
            return x;
        };
        const Bands bt = bands(true), be = bands(false);
        // strike: the 8th partial's gain end to end
        auto g8at = [](const Knobs& k) {
            const Shape s = pianoOf(k);
            return dbOf(partialsOf(s, rankAt(s, 60.0f)).gain[7]);
        };
        const double g8lo = g8at(with(grand, Knob::strike, -1.0f)), g8hi = g8at(with(grand, Knob::strike, 1.0f));
        // unison: the strings and the detune end to end
        const Shape uLo = pianoOf(with(grand, Knob::unison, -1.0f)), uHi = pianoOf(with(grand, Knob::unison, 1.0f));
        // body: the first mode end to end
        const Shape bLo = pianoOf(with(grand, Knob::body, -1.0f)), bHi = pianoOf(with(grand, Knob::body, 1.0f));
        const double f1lo = boardOf(bLo[Channel::boardArea], bLo[Channel::boardLoss], bLo[Channel::open]).mode[0].hz;
        const double f1hi = boardOf(bHi[Channel::boardArea], bHi[Channel::boardLoss], bHi[Channel::open]).mode[0].hz;
        // damper: the release t60 of c4's first partial end to end, the string's own loss plus the damper's
        auto releaseT60 = [](const Knobs& k) {
            Shape s = pianoOf(k);
            s[Channel::midi] = 60.0f;
            const RankState r = rankAt(s, 60.0f);
            const PartialTable p = partialsOf(s, r);
            const double own = 6.907755 / (p.t60[0] * r.t60Scale);
            return 6.907755 / (own + dampRateOf(r.f0 * p.ratio[0], s, 0.0f, lawConstants()));
        };
        const double relHeavy = releaseT60(with(grand, Knob::damper, -1.0f)), relLight = releaseT60(with(grand, Knob::damper, 1.0f));
        // pickup: thd far against close at velocity 1 on rhodes, at e3 (the plan's) and c4
        const double farE3 = strikeAt(pianoOf(with(rhodes, Knob::pickup, -1.0f)), 52.0f, 1.0f).thd;
        const double closeE3 = strikeAt(pianoOf(with(rhodes, Knob::pickup, 1.0f)), 52.0f, 1.0f).thd;
        const double farC4 = strikeAt(pianoOf(with(rhodes, Knob::pickup, -1.0f)), 60.0f, 1.0f).thd;
        const double closeC4 = strikeAt(pianoOf(with(rhodes, Knob::pickup, 1.0f)), 60.0f, 1.0f).thd;
        // generator: the partials on at grand against rhodes, at c4
        auto onAt = [](const Knobs& k) {
            const Shape s = pianoOf(k);
            return partialsOnOf(partialsOf(s, rankAt(s, 60.0f)));
        };
        const int onGrand = onAt(grand), onRhodes = onAt(rhodes);
        std::fprintf(f, "# the audibility floors the table and the simulator read, at grand's c4 and velocity 1 unless named; the hammer's\n"
                        "# band by the truth (32 fs) and by the engine's scheme (%d substeps, the loss implicit)\n", kContactRate);
        std::fprintf(f, "%-10s %-62s %-28s %-34s %s\n", "knob", "the measure", "the floor", "reads", "");
        auto row = [&](const char* k, const char* what, const char* floor, const std::string& reads, const char* met) {
            std::fprintf(f, "%-10s %-62s %-28s %-34s %s\n", k, what, floor, reads.c_str(), met);
        };
        char b[128];
        std::snprintf(b, sizeof b, "%.1f c (%+.1f -> %+.1f)", scaleSpan, c10at(with(grand, Knob::scale, -1.0f), 36.0f), c10at(with(grand, Knob::scale, 1.0f), 36.0f));
        row("scale", "the 10th partial of c2, cents, end to end", "30 c", b, scaleSpan >= 30.0 ? "met" : "NOT MET");
        for (int which = 0; which < 2; ++which) {
            const Bands& x = which ? be : bt;
            const char* by = which ? ", the engine" : ", the truth";
            char what[96];
            std::snprintf(b, sizeof b, "%.1f db (%+.1f -> %+.1f)", x.med1 - x.med03, x.med03, x.med1);
            std::snprintf(what, sizeof what, "the band, medium, velocity 0.3 -> 1%s", by);
            row("hammer", what, "12 db (a check)", b, x.med1 - x.med03 >= 12.0 ? "met" : "NOT MET");
            std::snprintf(b, sizeof b, "%.1f db (%+.1f -> %+.1f)", x.hard1 - x.soft1, x.soft1, x.hard1);
            std::snprintf(what, sizeof what, "the band, soft -> hard, velocity 1%s", by);
            row("hammer", what, "3 db (the plan's 6)", b, x.hard1 - x.soft1 >= 3.0 ? "met" : "NOT MET");
            std::snprintf(b, sizeof b, "%.1f db (%+.1f -> %+.1f)", x.hard05 - x.soft05, x.soft05, x.hard05);
            std::snprintf(what, sizeof what, "the band, soft -> hard, velocity 0.5%s", by);
            row("hammer", what, "10 db (the plan's 12)", b, x.hard05 - x.soft05 >= 10.0 ? "met" : "NOT MET");
            std::snprintf(b, sizeof b, "%.1f db (%+.1f -> %+.1f)", x.soft1 - x.soft03, x.soft03, x.soft1);
            std::snprintf(what, sizeof what, "the band, soft, velocity 0.3 -> 1%s", by);
            row("hammer", what, "printed", b, "");
        }
        std::snprintf(b, sizeof b, "%.1f db (%+.1f -> %s)", std::fabs(g8lo - std::fmax(g8hi, -120.0)), g8lo, num(g8hi, "%+.1f").c_str());
        row("strike", "the 8th partial's gain end to end, db (a node at +1)", "20 db", b, std::fabs(g8lo - std::fmax(g8hi, -120.0)) >= 20.0 ? "met" : "NOT MET");
        std::snprintf(b, sizeof b, "strings %.0f -> %.0f, detune %.1f -> %.1f c", uLo[Channel::strings], uHi[Channel::strings], uLo[Channel::detune],
                      uHi[Channel::detune]);
        row("unison", "the strings and the detune end to end", "beat 6 db, after 2 (render)", b, "stage 1 renders it");
        std::snprintf(b, sizeof b, "%.2f octaves (%.1f -> %.1f hz)", std::log2(f1lo / f1hi), f1lo, f1hi);
        row("body", "the board's first mode end to end", "an octave", b, std::log2(f1lo / f1hi) >= 1.0 ? "met" : "NOT MET");
        std::snprintf(b, sizeof b, "%.1f x (%.3f -> %.3f s)", relLight / relHeavy, relHeavy, relLight);
        row("damper", "the release t60 of c4's first partial end to end (dampRateOf)", "8 x", b, relLight / relHeavy >= 8.0 ? "met" : "NOT MET");
        std::snprintf(b, sizeof b, "%.2f %% -> %.2f %%", 100.0 * farE3, 100.0 * closeE3);
        row("pickup", "thd far -> close, velocity 1, rhodes at e3", "under 3 %, over 15 %", b, farE3 < 0.03 && closeE3 > 0.15 ? "met" : "NOT MET");
        std::snprintf(b, sizeof b, "%.2f %% -> %.2f %%", 100.0 * farC4, 100.0 * closeC4);
        row("pickup", "the same at c4", "under 3 %, over 15 %", b, farC4 < 0.03 && closeC4 > 0.15 ? "met" : "NOT MET");
        std::snprintf(b, sizeof b, "%d -> %d", onGrand, onRhodes);
        row("generator", "the partials on at c4, grand -> rhodes", "over 30 -> 6", b, onGrand > 30 && onRhodes <= 6 ? "met" : "NOT MET");
        std::fclose(f);
    }
    std::printf("chart: %ld cells into %s; %ld out of order, %ld t60s not positive, %ld gains outside, %ld over the ceiling on, %ld hammers "
                "not leaving, %ld forces not finite, %ld engine strikes off the truth; the largest peak force %.1f N (%.1f by the truth)\n",
                tally.cells, dir.c_str(), tally.disorder, tally.t60bad, tally.gainBad, tally.ceilingBad, tally.notLeft, tally.notFinite, tally.offRate,
                tally.peak, tally.peak8);
    return 0;
}

// ---- play, corpus, size ----------------------------------------------------------------------------

// the render's spec from the arguments: the vector's instrument (--set's instrument channels over it); with
// --table, a tine table set whole over it (rhodes's when no vector was named); --set's partial and player
// channels after; the note, the velocity, the length and the events. what: the source, for a reader
bool playSpecOf(const Args& a, PlaySpec& sp, std::string& what) {
    sp = PlaySpec();
    sp.tuning = a.tuning;
    const Tuning t = clamped(a.tuning);
    Knobs k = a.knobs;
    if (!a.table.empty()) {
        RefTable rt;
        std::string err;
        if (!readTable(a.table, rt, err)) { std::fprintf(stderr, "%s\n", err.c_str()); return false; }
        if (!partialTableOf(rt, sp.tableP)) { std::fprintf(stderr, "%s holds no partials to set whole\n", a.table.c_str()); return false; }
        sp.table = true;
        if (!a.knobsGiven) k = knobAnchors()[1].k;
        what = "the table " + std::filesystem::path(a.table).filename().string() + " set whole over " + knobLine("vector", k, 21, 108);
    } else {
        what = a.preset && std::memcmp(&a.knobs, &a.preset->knobs, sizeof a.knobs) == 0 ? knobLine(a.preset->name, k, a.preset->lo, a.preset->hi)
                                                                                       : knobLine("vector", k, 21, 108);
    }
    sp.instrument = shapeOf(k, lawConstants(), a.sets, t);
    for (const auto& p : a.sets)
        if (!isInstrumentChannel(p.first)) sp.sets.push_back(p);
    sp.midi = a.midi;
    sp.vel = a.vel;
    sp.sec = a.sec > 0.0f ? a.sec : 4.0f;
    sp.offAt = a.offAt;
    if (a.restrikeAt >= 0.0f) sp.strikes.push_back({a.restrikeAt, a.vel});
    sp.pedal = a.pedal;
    sp.pedalOffAt = a.pedalOffAt;
    sp.glideTo = a.glideTo;
    sp.glideAt = a.glideAt;
    sp.glideSec = a.glideSec;
    return true;
}

std::string eventsText(const PlaySpec& sp) {
    std::string e;
    char b[160];
    if (sp.pedal) e += ", the pedal down";
    for (const auto& st : sp.strikes) { std::snprintf(b, sizeof b, ", a strike at %.2f s (velocity %.2f)", st.first, st.second); e += b; }
    if (sp.offAt >= 0.0f) { std::snprintf(b, sizeof b, ", released at %.2f s", sp.offAt); e += b; }
    if (sp.pedalOffAt >= 0.0f) { std::snprintf(b, sizeof b, ", the pedal up at %.2f s", sp.pedalOffAt); e += b; }
    if (sp.glideTo >= 0.0f) {
        std::snprintf(b, sizeof b, ", a glide to midi %.2f from %.2f s over %.2f s", sp.glideTo, sp.glideAt, sp.glideSec > 0.0f ? sp.glideSec : sp.sec - sp.glideAt);
        e += b;
    }
    for (const auto& p : sp.sets) { std::snprintf(b, sizeof b, ", %s=%g", channelName(p.first), (double)p.second); e += b; }
    return e;
}

// the tuning's fields that are not the default's, as --tune would write them
std::string tuningText(const Tuning& t) {
    const Tuning d, c = clamped(t);
    std::string out;
    for (const TuneField& f : kTune) {
        if (c.*(f.field) == d.*(f.field)) continue;
        char b[64];
        std::snprintf(b, sizeof b, "%s%s=%g", out.empty() ? "" : " ", f.name, (double)(c.*(f.field)));
        out += b;
    }
    return out;
}

// one render: rendered, read, printed into f, written to a wav and a png when named
int playInto(std::FILE* f, const PlaySpec& sp, const std::string& what, const std::string& wav, const std::string& png) {
    const PlayRender r = renderPlay(sp);
    const PlayReading rd = readPlay(r, sp);
    const std::string tune = tuningText(sp.tuning);
    std::fprintf(f, "  %s\n  %.2f s%s%s%s\n", what.c_str(), sp.sec, eventsText(sp).c_str(), tune.empty() ? "" : "; the tuning: ", tune.c_str());
    printPlay(f, r, rd, sp);
    // a render over full scale is written at the gain that brings its peak to 0.99, said so
    const float gain = rd.peak > 1.0 ? (float)(0.99 / rd.peak) : 1.0f;
    if (!wav.empty() && gain != 1.0f) std::fprintf(f, "  the render peaks over full scale: the wav written at gain %.4f (%+.1f db)\n", gain, dbfs(gain));
    if (!wav.empty() && !writeWav(wav.c_str(), r.x.data(), r.x.size(), (int)sp.fs, gain)) {
        std::fprintf(stderr, "cannot write %s\n", wav.c_str());
        return 2;
    }
    if (!png.empty() && !writeSpectrogram(png.c_str(), r.x.data(), r.x.size(), sp.fs)) {
        std::fprintf(stderr, "cannot write %s\n", png.c_str());
        return 2;
    }
    return 0;
}

int cmdPlay(const Args& a) {
    PlaySpec sp;
    std::string what;
    if (!playSpecOf(a, sp, what)) return 2;
    std::printf("play\n");
    if (a.undoLevel && a.table.empty()) {
        const float db = levelDb(a.knobs);
        sp.tuning.outputGain = clamped(sp.tuning).outputGain * std::pow(10.0f, -db / 20.0f);
        std::printf("  levelDb %+.2f db, undone: outputGain %.4f\n", db, sp.tuning.outputGain);
    }
    return playInto(stdout, sp, what, a.wav, a.png);
}

// the library's byte baseline: the three anchors at three notes and two velocities, grand's re-strike,
// release, pedalled release and glide, rhodes at two notes, wurlitzer at one, the measured tine whole; a wav
// and a png each, and index.txt with what play read
int cmdCorpus(const Args& a) {
    const std::string dir = a.dir.empty() ? "corpus" : a.dir;
    makeDirs(dir);
    const std::string indexPath = dir + "/index.txt";
    FILE* index = std::fopen(indexPath.c_str(), "w");
    if (!index) { std::fprintf(stderr, "cannot write %s\n", indexPath.c_str()); return 2; }
    std::fprintf(index, "# uptool corpus: every render, its source, its note, velocity and events, and what play read (uptool play)\n");
    int files = 0;
    auto one = [&](const std::string& name, Args args) {
        PlaySpec sp;
        std::string what;
        if (!playSpecOf(args, sp, what)) return 2;
        const std::string wav = dir + "/" + name + ".wav", png = dir + "/" + name + ".png";
        std::fprintf(index, "\n%s.wav, %s.png\n", name.c_str(), name.c_str());
        const int rc = playInto(index, sp, what, wav, png);
        files += 2;
        std::printf("  %s\n", name.c_str());
        return rc;
    };
    auto base = [&](int anchor, float midi, float vel) {
        Args x;
        x.tuning = a.tuning;
        x.preset = presetByName(knobAnchors()[anchor].name);
        x.knobs = x.preset->knobs;
        x.knobsGiven = true;
        x.midi = midi;
        x.vel = vel;
        x.sec = 4.0f;
        return x;
    };
    const float notes[3] = {36.0f, 60.0f, 84.0f}, vels[2] = {0.5f, 1.0f};
    for (int an = 0; an < kAnchorCount; ++an)
        for (float note : notes)
            for (float vel : vels) {
                char name[96];
                std::snprintf(name, sizeof name, "%s_%d_v%.1f", knobAnchors()[an].name, (int)note, vel);
                if (one(name, base(an, note, vel))) return 2;
            }
    Args x = base(0, 60.0f, 1.0f);
    x.restrikeAt = 1.0f;
    if (one("grand_60_restrike", x)) return 2;
    x = base(0, 60.0f, 1.0f);
    x.offAt = 1.0f;
    if (one("grand_60_release", x)) return 2;
    x = base(0, 60.0f, 1.0f);
    x.pedal = true;
    x.offAt = 1.0f;
    x.pedalOffAt = 2.5f;
    if (one("grand_60_pedal", x)) return 2;
    x = base(0, 60.0f, 1.0f);
    x.glideTo = 67.0f;
    x.glideAt = 1.0f;
    x.glideSec = 2.0f;
    if (one("grand_60_glide", x)) return 2;
    if (one("rhodes_52", base(1, 52.0f, 1.0f))) return 2;
    if (one("rhodes_64", base(1, 64.0f, 1.0f))) return 2;
    if (one("wurlitzer_57", base(2, 57.0f, 1.0f))) return 2;
    x = base(1, 64.0f, 1.0f);
    x.table = refDir() + "/tine_rhodes.txt";
    if (one("tine_table_64", x)) return 2;
    // every preset at the bottom, the middle and the top of its range, the level table undone in the render's gain
    std::fprintf(index, "\n# the presets: each at the bottom, middle and top of its range, velocities 0.5 and 1, 4 s, at an outputGain of\n"
                        "# 10^(-levelDb / 20), the level table undone\n");
    for (int i = 0; i < presetCount(); ++i) {
        const PresetInfo& p = presetAt(i);
        const int notes3[3] = {p.lo, (p.lo + p.hi) / 2, p.hi};
        for (int note : notes3)
            for (float vel : vels) {
                Args y;
                y.preset = &p;
                y.knobs = p.knobs;
                y.knobsGiven = true;
                y.midi = (float)note;
                y.vel = vel;
                y.sec = 4.0f;
                y.tuning = a.tuning;
                y.tuning.outputGain = a.tuning.outputGain * std::pow(10.0f, -levelDb(p.knobs) / 20.0f);
                char name[96];
                std::snprintf(name, sizeof name, "preset_%s_%d_v%.1f", p.name, note, vel);
                if (one(name, y)) return 2;
            }
    }
    // the pool: grand on four voices, a chord with the pedal and five keys on four voices (the fifth steals the quietest)
    std::fprintf(index, "\n# the pool: up::Pool<4> (seed 1, the default tuning), grand (setKnobs), each call landing at the top of the block its\n"
                        "# time falls in; the sum, not clamped, written at the gain that brings its peak to 0.99 where it passes full scale\n");
    struct PoolCall { float at; int what; float key, vel; };     // what: 0 a note on, 1 a note off, 2 the pedal (key: its value)
    auto pool = [&](const std::string& name, const char* what, const std::vector<PoolCall>& calls, float sec) {
        Pool<4> p(1);
        p.setTuning(a.tuning);
        p.setKnobs(knobAnchors()[0].k);
        const float fs = kDefaultFs;
        const int total = (int)std::lround(sec * fs);
        std::vector<float> x((size_t)total, 0.0f);
        size_t next = 0;
        int stolen = -1, stealAt = -1, landedAt = -1;
        float stolenKey = 0.0f, energy[4] = {0.0f, 0.0f, 0.0f, 0.0f};
        for (int at = 0; at < total; at += kBlock) {
            while (next < calls.size() && (int)std::lround(calls[next].at * fs) < at + kBlock) {
                const PoolCall& c = calls[next];
                if (c.what == 0) {
                    for (int i = 0; i < 4; ++i) energy[i] = p.voice(i).energy();
                    p.noteOn(c.key, c.vel);
                    for (int i = 0; i < 4 && stolen < 0; ++i)
                        if (p.slot(i).use == PoolSlot::Use::stolen) {
                            stolen = i;
                            stealAt = at;
                            stolenKey = p.slot(i).key;
                        }
                } else if (c.what == 1) {
                    p.noteOff(c.key);
                } else {
                    p.pedal(c.key);
                }
                ++next;
            }
            const int n = total - at < kBlock ? total - at : kBlock;
            p.render(x.data() + at, n);
            if (stolen >= 0 && landedAt < 0 && p.slot(stolen).use != PoolSlot::Use::stolen) landedAt = at;
        }
        const double peak = peakAbs(x.data(), x.size()), half = rms(x.data(), std::min(x.size(), (size_t)(0.5 * fs)));
        const float gain = peak > 1.0 ? (float)(0.99 / peak) : 1.0f;
        const std::string wav = dir + "/" + name + ".wav", png = dir + "/" + name + ".png";
        if (!writeWav(wav.c_str(), x.data(), x.size(), (int)fs, gain) || !writeSpectrogram(png.c_str(), x.data(), x.size(), fs)) {
            std::fprintf(stderr, "cannot write %s\n", wav.c_str());
            return 2;
        }
        files += 2;
        std::fprintf(index, "\n%s.wav, %s.png\n  %s, %.2f s\n  the calls (s, what, midi or the pedal, velocity):", name.c_str(), name.c_str(), what, sec);
        for (const PoolCall& c : calls) std::fprintf(index, " %.2f %s %.2f %.2f;", c.at, c.what == 0 ? "on" : (c.what == 1 ? "off" : "pedal"), c.key, c.vel);
        std::fprintf(index, "\n  the peak %.4f (%+.1f dbfs); the first half second's rms %.4f (%+.1f dbfs); %d voices sounding at the end\n", peak,
                     dbfs(peak), half, dbfs(half), p.sounding());
        if (gain != 1.0f) std::fprintf(index, "  the render peaks over full scale: the wav written at gain %.4f (%+.1f db)\n", gain, dbfs(gain));
        if (stolen >= 0)
            std::fprintf(index, "  the steal at %.3f s: the voices' energies %.3g, %.3g, %.3g, %.3g mm^2; voice %d (midi %.0f), the quietest, faded out "
                                "and took the new key %.1f ms later\n",
                         (double)stealAt / fs, energy[0], energy[1], energy[2], energy[3], stolen, stolenKey, 1000.0 * (landedAt - stealAt) / fs);
        std::printf("  %s\n", name.c_str());
        return 0;
    };
    if (pool("pool4_chord", "c3 e3 g3 c4 struck at once at velocity 1, the pedal down at 2 s, the keys up at 3 s (they ring on under the pedal)",
             {{0.0f, 0, 48.0f, 1.0f}, {0.0f, 0, 52.0f, 1.0f}, {0.0f, 0, 55.0f, 1.0f}, {0.0f, 0, 60.0f, 1.0f}, {2.0f, 2, 1.0f, 0.0f},
              {3.0f, 1, 48.0f, 0.0f}, {3.0f, 1, 52.0f, 0.0f}, {3.0f, 1, 55.0f, 0.0f}, {3.0f, 1, 60.0f, 0.0f}},
             6.0f) ||
        pool("pool4_steal", "five keys on four voices, half a second apart: c4, e4 (softly), g4, c5, then e5 steals the quietest",
             {{0.0f, 0, 60.0f, 1.0f}, {0.5f, 0, 64.0f, 0.4f}, {1.0f, 0, 67.0f, 0.8f}, {1.5f, 0, 72.0f, 1.0f}, {2.0f, 0, 76.0f, 1.0f}}, 6.0f))
        return 2;
    std::fclose(index);
    std::printf("corpus: %d files and index.txt in %s\n", files, dir.c_str());
    return 0;
}

// ---- level -----------------------------------------------------------------------------------------

// the level table as up_level_table.inc holds it: no date, so the same engine writes the same bytes
bool writeLevelTable(const std::string& path, const LevelTable& t) {
    FILE* out = std::fopen(path.c_str(), "wb");
    if (!out) return false;
    const int floats = kLevelAnchors * kKnobCount * kLevelCells;
    std::fprintf(out,
                 "// upright — up_level_table.inc  (plan §2, the level table)\n"
                 "// generated by `uptool level --write`; never edited by hand.\n"
                 "// how loud a vector sounds, db against the reference: grand (the eight at 0 but generator -1) over the\n"
                 "// octave c4 .. b4 (midi %.0f .. %.0f), each note struck at velocity 1 and rendered %.1f s as uptool play renders\n"
                 "// it (the voice's bridge force through a board of its own, the pickup added) at the default tuning, its rms\n"
                 "// over the first %.1f s after the strike, the octave's level the mean of the twelve powers (%.2f db full\n"
                 "// scale), to 0.01 db. four anchors along the generator, the string, the middle, the tine and the reed: at\n"
                 "// each, the eight at 0 but the generator at the anchor's, and a row a knob, that knob alone at each of its\n"
                 "// eleven points from -1 to +1, db against the anchor's own level (each row 0 where the anchor sits on its\n"
                 "// knob; the generator's row, measured once against the reference, is held in each block against that\n"
                 "// anchor's level). levelDb reads the string's generator row, and the other seven rows at the two anchors\n"
                 "// about the vector's generator, each read linearly between points and held past either end, summed, the\n"
                 "// two anchors' sums linear in the generator between them (%d floats, %zu bytes)\n",
                 kLevelMidi, kLevelMidi + (float)(kLevelNotes - 1), kLevelSec, kLevelWindow, 20.0 * std::log10(t.refRms), floats,
                 sizeof(float) * (size_t)floats);
    std::fprintf(out, "constexpr int kLevelPoints = %d;                 // a knob's points: eleven evenly from -1 to +1\n", kLevelCells);
    std::fprintf(out, "constexpr int kLevelAnchors = %d;                 // the generator's anchors, each a block of rows\n", kLevelAnchors);
    std::fprintf(out, "constexpr float kLevelAnchor[kLevelAnchors] = {");
    for (int a = 0; a < kLevelAnchors; ++a) std::fprintf(out, "%s%+.1ff", a ? ", " : "", (double)levelAnchor(a));
    std::fprintf(out, "};   // the string, the middle, the tine, the reed\n");
    std::fprintf(out, "constexpr float kLevelRows[kLevelAnchors][kKnobCount][kLevelPoints] = {\n");
    for (int a = 0; a < kLevelAnchors; ++a) {
        std::fprintf(out, "  {  // the %s: generator %+.1f, its own level %+.2f db against the reference\n", levelAnchorName(a), (double)levelAnchor(a),
                     t.anchorDb[a]);
        for (int k = 0; k < kKnobCount; ++k) {
            std::fprintf(out, "    {");
            for (int i = 0; i < kLevelCells; ++i) std::fprintf(out, "%s%+.2ff", i ? ", " : "", (double)t.row[a][k][i]);
            std::fprintf(out, "},  // %s\n", knobName((Knob)k));
        }
        std::fprintf(out, "  },\n");
    }
    std::fprintf(out, "};\n");
    std::fclose(out);
    return true;
}

int cmdLevel(const Args& a) {
    const auto t0 = std::chrono::steady_clock::now();
    const LevelTable t = measureLevelTable();
    const double took = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
    std::printf("the level table: the reference, grand over the octave c4 .. b4 (midi %.0f .. %.0f), each note at velocity 1, %.1f s as play\n"
                "renders it, its rms over the first %.1f s, the octave the mean of the twelve powers: %.4f (%+.2f db full scale); at each of\n"
                "the generator's four anchors, each knob alone at eleven points, db against the anchor's own level (the cell's largest peak\n"
                "under it), the generator's row measured once; %d octaves, %d renders in %.2f s\n",
                kLevelMidi, kLevelMidi + (float)(kLevelNotes - 1), kLevelSec, kLevelWindow, t.refRms, 20.0 * std::log10(t.refRms), t.octaves,
                t.octaves * kLevelNotes, took);
    double loudest = 0.0;
    for (int an = 0; an < kLevelAnchors; ++an) {
        std::printf("\nthe %s (generator %+.1f, the eight at 0 but the generator): its own level %+.2f db against the reference\n", levelAnchorName(an),
                    (double)levelAnchor(an), t.anchorDb[an]);
        std::printf("  %-9s", "");
        for (int i = 0; i < kLevelCells; ++i) std::printf("  %+6.1f", levelPoint(i));
        std::printf("   span\n");
        for (int k = 0; k < kKnobCount; ++k) {
            std::printf("  %-9s", knobName((Knob)k));
            float lo = 1e9f, hi = -1e9f;
            for (int i = 0; i < kLevelCells; ++i) {
                std::printf("  %+6.2f", (double)t.row[an][k][i]);
                lo = std::min(lo, t.row[an][k][i]);
                hi = std::max(hi, t.row[an][k][i]);
            }
            std::printf("  %5.2f\n  %-9s", (double)(hi - lo), "");
            for (int i = 0; i < kLevelCells; ++i) {
                std::printf("  %6.3f", t.peak[an][k][i]);
                loudest = std::max(loudest, t.peak[an][k][i]);
            }
            std::printf("\n");
        }
    }
    std::printf("\nthe largest cell's peak %.3f full scale%s\n", loudest, loudest >= 1.0 ? " (over full scale: a wav would clamp)" : "");
    if (!a.write) return 0;
    const std::string path =
        a.out.empty() ? std::filesystem::path(refDir() + "/../../src/core/up_level_table.inc").lexically_normal().string() : a.out;
    if (!writeLevelTable(path, t)) {
        std::fprintf(stderr, "cannot write %s\n", path.c_str());
        return 2;
    }
    std::printf("written: %s\n", path.c_str());
    return 0;
}

int cmdSize(const Args&) {
    const size_t voice = sizeof(Voice), ch = sizeof(Channels), main = sizeof(Bank<kMaxModes>), sec = 2 * sizeof(Bank<kUnisonModes>);
    const size_t hammer = sizeof(Hammer), pickup = sizeof(Pickup), state = sizeof(State), attack = sizeof(Attack);
    std::printf("sizeof(up::Voice) %zu bytes at %d slots, %d unison partials, %d substeps (%d channels):\n", voice, kMaxModes, kUnisonModes,
                kContactRate, kChannelCount);
    std::printf("  the channels      %6zu  (%d channels x seven arrays)\n", ch, kChannelCount);
    std::printf("  the main string   %6zu  (%zu a partial)\n", main, (main - sizeof(int) * (kMaxModes + 4)) / kMaxModes);
    std::printf("  the secondaries   %6zu  (two of %d)\n", sec, kUnisonModes);
    std::printf("  the hammer        %6zu  (its felt table %d points)\n", hammer, kFeltPoints);
    std::printf("  the attack        %6zu  (%d longitudinal modes, the blow and the thud)\n", attack, kLongModes);
    std::printf("  the pickup        %6zu\n", pickup);
    std::printf("  the state         %6zu\n", state);
    std::printf("  the rest          %6zu  (two tunings, the law, the pitch, the strings' own, the block's)\n",
                voice - ch - main - sec - hammer - attack - pickup - state);
    std::printf("sizeof(up::Board) %zu bytes at %d modes; sizeof(up::Pending) %zu bytes (an attached voice's writer's table)\n", sizeof(Board),
                kBoardModes, sizeof(Pending));
    std::printf("sizeof(up::Pool<4>) %zu bytes, sizeof(up::Pool<8>) %zu bytes (the voices, a slot of %zu and a lane of %zu each, the board and the "
                "pool's own %zu)\n",
                sizeof(Pool<4>), sizeof(Pool<8>), sizeof(PoolSlot), sizeof(PoolLane),
                sizeof(Pool<8>) - 8 * (sizeof(Voice) + sizeof(PoolSlot) + sizeof(PoolLane) + 2 * sizeof(float*)));
    return 0;
}

void usage() {
    std::fprintf(stderr,
                 "uptool partials --knob NAME=VALUE ... | --preset NAME [--zero] [--midi M] [--set CHANNEL=VALUE ...] [--tune NAME=VALUE]\n"
                 "uptool partials --table FILE\n"
                 "uptool string --knob ... [--from M --to M]\n"
                 "uptool hammer --knob ... [--midi M --vel V]\n"
                 "uptool pickup --knob ... [--amp A] [--from-hammer --midi M --vel V]\n"
                 "uptool board --knob ... [--sweep --out FILE]\n"
                 "uptool print --knob ... | --preset NAME [--shape]\n"
                 "uptool random [--seed S --count N --midi M]\n"
                 "uptool fit --table FILE [--lock KNOB=VALUE ...]\n"
                 "uptool fit --constants --tables DIR [--kind KIND] [--out FILE] [--fix NAME,...]\n"
                 "uptool chart [DIR]\n"
                 "uptool chart --audio [DIR]\n"
                 "uptool level [--write] [--out PATH]\n"
                 "uptool play --preset NAME | --knob ... | --table FILE [--midi M --vel V --sec S --off-at S --restrike-at S --pedal\n"
                 "            --pedal-off-at S --glide-to M2 [--glide-at S --glide-sec S] --set ... --tune ... --undo-level -o x.wav --png p.png]\n"
                 "uptool corpus DIR [--tune NAME=VALUE ...]\n"
                 "uptool size\n"
                 "  --knob NAME=VALUE (scale hammer strike unison body generator pickup damper, -1 .. 1; a knob not named is grand's: 0,\n"
                 "  the generator -1)  --preset NAME (grand concertGrand upright spinet honkyTonk fortepiano rhodes rhodesBark wurlitzer;\n"
                 "  a --knob after it wins)  --zero (all eight at 0)  --set CHANNEL=VALUE  --tune NAME=VALUE  --midi M\n");
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) { usage(); return 2; }
    const std::string cmd = argv[1];
    Args a;
    if (!parseArgs(argc, argv, 2, a)) { usage(); return 2; }
    if (cmd == "partials") return cmdPartials(a);
    if (cmd == "string") return cmdString(a);
    if (cmd == "hammer") return cmdHammer(a);
    if (cmd == "pickup") return cmdPickup(a);
    if (cmd == "board") return cmdBoard(a);
    if (cmd == "print") return cmdPrint(a);
    if (cmd == "random") return cmdRandom(a);
    if (cmd == "fit") return cmdFit(a);
    if (cmd == "chart") return cmdChart(a);
    if (cmd == "play") return cmdPlay(a);
    if (cmd == "corpus") return cmdCorpus(a);
    if (cmd == "size") return cmdSize(a);
    if (cmd == "level") return cmdLevel(a);
    usage();
    return 2;
}
