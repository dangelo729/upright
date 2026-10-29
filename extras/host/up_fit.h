// upright — up_fit.h  (plan §15)
// what uptool and the tests share: the three anchors (the presets grand,
// rhodes, wurlitzer), the line print writes and its parser, random's draw,
// note names; the reference tables read; each
// table's error against the law at a knob vector; the knob fit and the
// constants' fit. host only.
//
// a reference table (extras/reference/*.txt, README.txt skipped) is plain
// text: `#` comments, a `# units:`, a `# source:`, a `# kind:` (B, decay,
// hammer, tine, cantilever, board) and one or more `# columns:` lines, each
// naming the columns of the rows after it; a data line is `key value ...`,
// the key a midi number, a note name (A0, C#4, Bb2), a partial number or a
// name, a value a number or `-` (none). `# hold: KNOB=V ...` holds knobs in
// the constants' fit (the knob fit of one table searches freely); `# midi: M`
// names the note a tine table's partials were measured at.
//
// a table's error, by kind, at a knob vector under constants:
//   B        the rms of ln(B_law / B_table) over its notes
//   decay    the rms of log2(t60_law / t60_table) over its partials; a loss
//            block (midi b1 b3) stands for its partials 1 .. 10 at sigma =
//            b1 + b3 w², w = 2 pi n f0
//   hammer   the rms over its notes of ln(m_law / m), ln(K_law / K) (K in
//            N/mm^p: the force at 1 mm) and p_law - p, the law's K and p at
//            the note through the rank (hardnessScale, pAdd)
//   tine     the rms of its partials' ratios in octaves (cents / 1200) plus a
//            quarter of the rms log2 t60 where it has them, at its # midi:
// the knobs a table's fit moves: B and decay the scale, hammer the hammer,
// tine the generator; the rest at the anchor (grand, or rhodes for a tine)
// unless locked. a grid in fifths over the free knobs, then coordinate
// descent with the step halving from 0.25 to 0.01.
#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "core/up_channels.h"
#include "core/up_config.h"
#include "core/up_knobs.h"
#include "core/up_law.h"
#include "core/up_presets.h"
#include "core/up_rank.h"

namespace up {

// ---- the knobs on the host ------------------------------------------------------------------------

// a knob held as the layer holds it: a nan as 0, past the end the end
inline float knobHeld(float x) { return x != x ? 0.0f : (x < -1.0f ? -1.0f : (x > 1.0f ? 1.0f : x)); }

// the three anchors: the string, the tine, the reed, each the preset of its name (up_presets.h)
struct KnobAnchor {
    const char* name;
    Knobs k;
};
inline const KnobAnchor* knobAnchors() {
    static const KnobAnchor a[3] = {
        {"grand", presetByName("grand")->knobs},
        {"rhodes", presetByName("rhodes")->knobs},
        {"wurlitzer", presetByName("wurlitzer")->knobs},
    };
    return a;
}
constexpr int kAnchorCount = 3;
inline const KnobAnchor* anchorByName(const char* name) {
    for (int i = 0; i < kAnchorCount; ++i)
        if (name && std::strcmp(knobAnchors()[i].name, name) == 0) return &knobAnchors()[i];
    return nullptr;
}

// a float written short enough to read back to the same bits, as c++
inline std::string knobNumber(float x) {
    char b[48];
    for (int p = 1; p <= 9; ++p) {
        std::snprintf(b, sizeof b, "%.*g", p, (double)x);
        const float y = std::strtof(b, nullptr);
        uint32_t ux, uy;
        std::memcpy(&ux, &x, sizeof ux);
        std::memcpy(&uy, &y, sizeof uy);
        if (ux == uy) break;
    }
    std::string s = b;
    if (s.find_first_of(".eE") == std::string::npos) s += ".0";
    return s + "f";
}

// the vector as an up_presets.cpp line: {"name", {sc, ha, st, un, bo, ge, pi, da}, lo, hi},
// each knob held as the layer holds it
inline std::string knobLine(const char* name, const Knobs& k, int lo, int hi) {
    std::string s = std::string("{\"") + name + "\", {";
    for (int i = 0; i < kKnobCount; ++i) s += (i ? ", " : "") + knobNumber(knobHeld(k.v[i]));
    char tail[48];
    std::snprintf(tail, sizeof tail, "}, %d, %d},", lo, hi);
    return s + tail;
}

// the line read back: the name, the eight, the range. false on anything else
inline bool parseKnobLine(const std::string& line, std::string& name, Knobs& k, int& lo, int& hi) {
    size_t at = line.find('"');
    if (at == std::string::npos) return false;
    const size_t end = line.find('"', at + 1);
    if (end == std::string::npos) return false;
    name = line.substr(at + 1, end - at - 1);
    at = line.find('{', end);
    if (at == std::string::npos) return false;
    const char* p = line.c_str() + at + 1;
    for (int i = 0; i < kKnobCount; ++i) {
        char* q = nullptr;
        k.v[i] = std::strtof(p, &q);
        if (q == p) return false;
        p = q;
        if (*p == 'f') ++p;
        while (*p == ' ') ++p;
        if (*p != (i + 1 < kKnobCount ? ',' : '}')) return false;
        ++p;
    }
    char* q = nullptr;
    while (*p == ' ' || *p == ',') ++p;
    lo = (int)std::strtol(p, &q, 10);
    if (q == p) return false;
    p = q;
    while (*p == ' ' || *p == ',') ++p;
    hi = (int)std::strtol(p, &q, 10);
    if (q == p) return false;
    p = q;
    while (*p == ' ') ++p;
    return *p == '}';
}

// random's draw: each knob uniform in -1 .. 1, from a seeded xorshift32
inline Knobs drawKnobs(Rng& r) {
    Knobs k;
    for (int i = 0; i < kKnobCount; ++i) k.v[i] = r.bipolar();
    return k;
}

inline double centsOf(double ratio) { return 1200.0 * std::log2(ratio); }
inline double dbOf(double x) { return 20.0 * std::log10(std::fabs(x) > 1e-30 ? std::fabs(x) : 1e-30); }

// ---- notes ----------------------------------------------------------------------------------------

inline std::string noteName(double midi) {
    static const char* const names[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    const int m = (int)std::lround(midi);
    const int pc = ((m % 12) + 12) % 12;
    return std::string(names[pc]) + std::to_string(m / 12 - 1 - (m < 0 && m % 12 ? 1 : 0));
}

// a midi number, or a note name: a letter, # or b, an octave (c4 = 60)
inline bool parseNote(const std::string& s, double& midi) {
    char* end = nullptr;
    const double x = std::strtod(s.c_str(), &end);
    if (end != s.c_str() && *end == '\0') { midi = x; return true; }
    if (s.empty()) return false;
    static const int base[7] = {9, 11, 0, 2, 4, 5, 7};   // a b c d e f g
    const char l = (char)std::tolower((unsigned char)s[0]);
    if (l < 'a' || l > 'g') return false;
    int pc = base[l - 'a'];
    size_t i = 1;
    if (i < s.size() && (s[i] == '#' || s[i] == 's')) { ++pc; ++i; }
    else if (i < s.size() && s[i] == 'b') { --pc; ++i; }
    if (i >= s.size()) return false;
    const long oct = std::strtol(s.c_str() + i, &end, 10);
    if (*end != '\0') return false;
    midi = 12.0 * (double)(oct + 1) + pc;
    return true;
}

// ---- a vector's instrument with channels set over it ----------------------------------------------

typedef std::vector<std::pair<Channel, float>> Spec;

// the vector's instrument under constants, --set's instrument channels over it (the partial
// channels nan, for the law; a partial channel set lands over the law's in partialsAt)
inline Shape shapeOf(const Knobs& k, const LawConstants& c, const Spec& sets = Spec(), const Tuning& t = Tuning()) {
    Shape s = pianoOf(k, t, c);
    for (const auto& p : sets)
        if (isInstrumentChannel(p.first)) s[p.first] = p.second;
    return s;
}

// the law's table at a note, --set's partial channels over it
inline PartialTable partialsAt(const Shape& s, const RankState& r, const LawConstants& c, const Spec& sets = Spec(),
                               const Tuning& t = Tuning()) {
    PartialTable p = partialsOf(s, r, t, c);
    for (const auto& q : sets) {
        if (!isPartialChannel(q.first)) continue;
        const int k = partialOf(q.first);
        const int block = ((int)q.first - (int)Channel::ratio0) / kMaxModes;
        float* col = block == 0 ? p.ratio : block == 1 ? p.t60 : block == 2 ? p.gain : p.out;
        col[k] = q.second;
    }
    return p;
}

// ---- the reference tables -------------------------------------------------------------------------

struct RefRow {
    std::string key;            // as written
    double x = NAN;             // the key as a number (a midi note from a note name), nan for a name
    std::vector<double> v;      // the values after the key, nan for `-`
    int block = -1;             // which `# columns:` line it follows
    std::string note;           // the line's comment
};

struct RefTable {
    std::string path, name, title, source, via, units, kind, hold;
    double midi = NAN;          // "# midi:": the note a tine table was measured at
    std::vector<std::vector<std::string>> blocks;
    std::vector<RefRow> rows;
    // a column's index in a block, the key at 0; -1 when the block has none
    int column(int block, const char* col) const {
        if (block < 0 || block >= (int)blocks.size()) return -1;
        for (size_t i = 0; i < blocks[(size_t)block].size(); ++i)
            if (blocks[(size_t)block][i] == col) return (int)i;
        return -1;
    }
    double at(const RefRow& r, int col) const {
        if (col == 0) return r.x;
        return col > 0 && col - 1 < (int)r.v.size() ? r.v[(size_t)(col - 1)] : NAN;
    }
};

inline bool isTableFile(const std::string& path) {
    const size_t slash = path.find_last_of('/');
    const std::string name = slash == std::string::npos ? path : path.substr(slash + 1);
    return name.size() > 4 && name.compare(name.size() - 4, 4, ".txt") == 0 && name != "README.txt";
}

inline std::string trim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\r\n"), b = s.find_last_not_of(" \t\r\n");
    return a == std::string::npos ? "" : s.substr(a, b - a + 1);
}

inline bool readTable(const std::string& path, RefTable& out, std::string& err) {
    std::ifstream in(path);
    if (!in) { err = "cannot open " + path; return false; }
    out = RefTable();
    out.path = path;
    const size_t slash = path.find_last_of('/');
    out.name = path.substr(slash == std::string::npos ? 0 : slash + 1);
    if (out.name.size() > 4) out.name = out.name.substr(0, out.name.size() - 4);
    std::string line;
    int lineNo = 0;
    while (std::getline(in, line)) {
        ++lineNo;
        const std::string t = trim(line);
        if (t.empty()) continue;
        if (t[0] == '#') {
            const std::string c = trim(t.substr(1));
            if (out.title.empty()) out.title = c;
            auto field = [&](const char* key, std::string& to) {
                const size_t n = std::strlen(key);
                if (c.compare(0, n, key) != 0) return false;
                to = trim(c.substr(n));
                return true;
            };
            std::string v;
            if (field("units:", v)) out.units = v;
            else if (field("source:", v)) out.source = v;
            else if (field("via:", v)) out.via = v;
            else if (field("kind:", v)) {
                std::istringstream ss(v);
                ss >> out.kind;
            } else if (field("hold:", v)) {
                const size_t paren = v.find('(');
                out.hold = trim(paren == std::string::npos ? v : v.substr(0, paren));
            } else if (field("midi:", v)) {
                double m;
                std::istringstream ss(v);
                std::string tok;
                ss >> tok;
                if (parseNote(tok, m)) out.midi = m;
            } else if (field("columns:", v)) {
                std::istringstream ss(v);
                std::vector<std::string> cols;
                std::string tok;
                while (ss >> tok) cols.push_back(tok);
                out.blocks.push_back(cols);
            }
            continue;
        }
        RefRow r;
        std::string data = t;
        const size_t hash = t.find('#');
        if (hash != std::string::npos) {
            data = trim(t.substr(0, hash));
            r.note = trim(t.substr(hash + 1));
        }
        std::istringstream ss(data);
        std::string tok;
        if (!(ss >> r.key)) continue;
        double m;
        if (parseNote(r.key, m)) r.x = m;
        while (ss >> tok) {
            if (tok == "-") { r.v.push_back(NAN); continue; }
            char* end = nullptr;
            const double x = std::strtod(tok.c_str(), &end);
            if (end == tok.c_str()) { err = path + ":" + std::to_string(lineNo) + ": bad value '" + tok + "'"; return false; }
            r.v.push_back(x);
        }
        r.block = (int)out.blocks.size() - 1;
        if (r.block < 0) { err = path + ":" + std::to_string(lineNo) + ": a data line before any '# columns:'"; return false; }
        out.rows.push_back(r);
    }
    if (out.units.empty()) { err = path + ": no '# units:' line"; return false; }
    if (out.source.empty()) { err = path + ": no '# source:' line"; return false; }
    if (out.kind.empty()) { err = path + ": no '# kind:' line"; return false; }
    return true;
}

inline std::vector<std::string> tableFiles(const std::string& dir) {
    std::vector<std::string> out;
    std::error_code ec;
    for (const auto& e : std::filesystem::directory_iterator(dir, ec))
        if (isTableFile(e.path().string())) out.push_back(e.path().string());
    std::sort(out.begin(), out.end());
    return out;
}

// ---- the tables' points ---------------------------------------------------------------------------

struct BPoint { double midi, B; };
inline std::vector<BPoint> bPoints(const RefTable& t) {
    std::vector<BPoint> out;
    for (const RefRow& r : t.rows) {
        const int b = t.column(r.block, "B");
        if (b < 0) continue;
        const double B = t.at(r, b);
        if (r.x == r.x && B > 0.0) out.push_back({r.x, B});
    }
    return out;
}

struct DecayPoint {
    double midi;
    int partial;                // the partial's number, 0 for a decay given at a frequency
    double hz;                  // the frequency a decay is given at (partial 0)
    double t60;
    bool fromLoss;              // from a loss block's b1, b3
};
// a decay block's rows: (midi partial t60), (midi freq_hz t60) or a loss block (midi b1 b3); a t60
// column may be named t60 or t60_s
inline std::vector<DecayPoint> decayPoints(const RefTable& t) {
    std::vector<DecayPoint> out;
    for (const RefRow& r : t.rows) {
        const int pc = t.column(r.block, "partial"), fc = t.column(r.block, "freq_hz");
        const int tc = t.column(r.block, "t60") > 0 ? t.column(r.block, "t60") : t.column(r.block, "t60_s");
        const int b1c = t.column(r.block, "b1"), b3c = t.column(r.block, "b3");
        if (r.x != r.x) continue;
        if (pc > 0 && tc > 0) {
            const double t60 = t.at(r, tc), n = t.at(r, pc);
            if (t60 > 0.0 && n >= 1.0) out.push_back({r.x, (int)std::lround(n), NAN, t60, false});
        } else if (fc > 0 && tc > 0) {
            const double t60 = t.at(r, tc), hz = t.at(r, fc);
            if (t60 > 0.0 && hz > 0.0) out.push_back({r.x, 0, hz, t60, false});
        } else if (b1c > 0 && b3c > 0) {
            const double b1 = t.at(r, b1c), b3 = t.at(r, b3c);
            const double f0 = 440.0 * std::exp2((r.x - 69.0) / 12.0);
            for (int n = 1; n <= 10; ++n) {
                const double w = 2.0 * 3.14159265358979 * n * f0;
                const double sigma = b1 + b3 * w * w;
                if (sigma > 0.0) out.push_back({r.x, n, NAN, 6.907755 / sigma, true});
            }
        }
    }
    return out;
}

struct HammerPoint { double midi, massG, kNmm, p; };
// K in the table's units: a column "K_Nmp" (N/m^p) is converted to N/mm^p, "K_Nmmp" read as is
inline std::vector<HammerPoint> hammerPoints(const RefTable& t) {
    std::vector<HammerPoint> out;
    for (const RefRow& r : t.rows) {
        const int mc = t.column(r.block, "mass_g"), pc = t.column(r.block, "p");
        const int ks = t.column(r.block, "K_Nmp"), km = t.column(r.block, "K_Nmmp");
        if (r.x != r.x || mc < 0 || pc < 0 || (ks < 0 && km < 0)) continue;
        const double p = t.at(r, pc);
        const double k = km >= 0 ? t.at(r, km) : t.at(r, ks) * std::pow(1e-3, p);
        out.push_back({r.x, t.at(r, mc), k, p});
    }
    return out;
}

struct TinePoint { int partial; double ratio, t60; };
// ratios to the first partial: a column "ratio", or "hz" over the first row's
inline std::vector<TinePoint> tinePoints(const RefTable& t) {
    std::vector<TinePoint> out;
    double first = NAN;
    for (const RefRow& r : t.rows) {
        const int rc = t.column(r.block, "ratio"), hc = t.column(r.block, "hz"), tc = t.column(r.block, "t60");
        if (r.x != r.x || (rc < 0 && hc < 0) || t.column(r.block, "partial") != 0) continue;
        double ratio = NAN;
        if (rc >= 0) ratio = t.at(r, rc);
        else {
            const double hz = t.at(r, hc);
            if (first != first && (int)std::lround(r.x) == 1) first = hz;
            ratio = first == first ? hz / first : NAN;
        }
        if (ratio > 0.0) out.push_back({(int)std::lround(r.x), ratio, tc >= 0 ? t.at(r, tc) : NAN});
    }
    return out;
}

// a measured string as a partial table, to set whole: a tine table's partials on their slots by number (the
// ratio, the t60 where it has one), every other entry nan (the law's). false for a table that holds no partials
inline bool partialTableOf(const RefTable& t, PartialTable& p) {
    for (int k = 0; k < kMaxModes; ++k) p.ratio[k] = p.t60[k] = p.gain[k] = p.out[k] = NAN;
    if (t.kind != "tine") return false;
    int n = 0;
    for (const TinePoint& q : tinePoints(t)) {
        if (q.partial < 1 || q.partial > kMaxModes) continue;
        p.ratio[q.partial - 1] = (float)q.ratio;
        if (q.t60 > 0.0) p.t60[q.partial - 1] = (float)q.t60;
        ++n;
    }
    return n > 0;
}

// a tine table's fundamental decays per note: a block keyed by midi with a t60 (or t60_s) column
struct TineDecay { double midi, t60; };
inline std::vector<TineDecay> tineDecays(const RefTable& t) {
    std::vector<TineDecay> out;
    for (const RefRow& r : t.rows) {
        if (t.column(r.block, "midi") != 0) continue;
        const int tc = t.column(r.block, "t60") > 0 ? t.column(r.block, "t60") : t.column(r.block, "t60_s");
        const double t60 = tc > 0 ? t.at(r, tc) : NAN;
        if (r.x == r.x && t60 > 0.0) out.push_back({r.x, t60});
    }
    return out;
}

// ---- a table's error at a vector ------------------------------------------------------------------

struct Residual {
    std::string label;
    double ref = 0.0, law = 0.0, err = 0.0;    // err in the measure's units: ln, log2, cents, or p
};
struct TableError {
    double value = 0.0;                        // the fit's figure (see the header)
    double rms = 0.0;                          // the rms of the residuals in the measure's units
    std::vector<Residual> items;
};

inline double tableMidi(const RefTable& t) { return t.midi == t.midi ? t.midi : 60.0; }

inline TableError tableError(const RefTable& t, const Knobs& k, const LawConstants& c, const Spec& sets = Spec()) {
    TableError e;
    const Shape s = shapeOf(k, c, sets);
    double sum = 0.0;
    int n = 0;
    if (t.kind == "B") {
        for (const BPoint& p : bPoints(t)) {
            const RankState r = rankAt(s, (float)p.midi, c);
            const double err = std::log((double)r.B / p.B);
            e.items.push_back({noteName(p.midi), p.B, r.B, err});
            sum += err * err;
            ++n;
        }
        e.rms = e.value = n ? std::sqrt(sum / n) : 0.0;
    } else if (t.kind == "decay") {
        double lastMidi = NAN;
        RankState r{};
        PartialTable pt{};
        for (const DecayPoint& p : decayPoints(t)) {
            if (p.midi != lastMidi) {
                r = rankAt(s, (float)p.midi, c);
                pt = partialsAt(s, r, c, sets);
                lastMidi = p.midi;
            }
            double law;
            std::string label = noteName(p.midi);
            if (p.partial == 0) {
                // the law's decay at the frequency: the string's loss law there, at the note
                law = (double)stringT60(0, (float)(p.hz / r.f0), r.f0, s, c, r.t60Scale) * r.t60Scale;
                label += " " + std::to_string((int)std::lround(p.hz)) + " hz";
            } else {
                if (p.partial > kMaxModes) continue;
                law = (double)pt.t60[p.partial - 1] * r.t60Scale;
                label += " n" + std::to_string(p.partial) + (p.fromLoss ? " (b1 b3)" : "");
            }
            const double err = std::log2(law / p.t60);
            e.items.push_back({label, p.t60, law, err});
            sum += err * err;
            ++n;
        }
        e.rms = e.value = n ? std::sqrt(sum / n) : 0.0;
    } else if (t.kind == "hammer") {
        for (const HammerPoint& p : hammerPoints(t)) {
            const RankState r = rankAt(s, (float)p.midi, c);
            const double m = (double)r.hammerMass * 1000.0, kk = (double)s[Channel::hardness] * r.hardnessScale;
            const double pp = hammerExponentAt(s[Channel::exponent], r.pAdd);
            const double em = std::log(m / p.massG), ek = std::log(kk / p.kNmm), ep = pp - p.p;
            e.items.push_back({noteName(p.midi) + " mass g", p.massG, m, em});
            e.items.push_back({noteName(p.midi) + " K N/mm^p", p.kNmm, kk, ek});
            e.items.push_back({noteName(p.midi) + " p", p.p, pp, ep});
            sum += em * em + ek * ek + ep * ep;
            n += 3;
        }
        e.rms = e.value = n ? std::sqrt(sum / n) : 0.0;
    } else if (t.kind == "tine") {
        const RankState r = rankAt(s, (float)tableMidi(t), c);
        const PartialTable pt = partialsAt(s, r, c, sets);
        double sumT = 0.0;
        int nT = 0;
        for (const TinePoint& p : tinePoints(t)) {
            if (p.partial < 1 || p.partial > kMaxModes) continue;
            const double law = pt.ratio[p.partial - 1] / pt.ratio[0];
            const double cents = centsOf(law / p.ratio);
            e.items.push_back({"partial " + std::to_string(p.partial) + " ratio", p.ratio, law, cents});
            sum += (cents / 1200.0) * (cents / 1200.0);
            ++n;
            if (p.t60 > 0.0) {
                const double lt = (double)pt.t60[p.partial - 1] * r.t60Scale;
                const double err = std::log2(lt / p.t60);
                e.items.push_back({"partial " + std::to_string(p.partial) + " t60", p.t60, lt, err});
                sumT += err * err;
                ++nT;
            }
        }
        for (const TineDecay& d : tineDecays(t)) {
            const RankState rd = rankAt(s, (float)d.midi, c);
            const PartialTable pd = partialsAt(s, rd, c, sets);
            const double lt = (double)pd.t60[0] * rd.t60Scale;
            const double err = std::log2(lt / d.t60);
            e.items.push_back({noteName(d.midi) + " partial 1 t60", d.t60, lt, err});
            sumT += err * err;
            ++nT;
        }
        const double rc = n ? std::sqrt(sum / n) : 0.0, rt = nT ? std::sqrt(sumT / nT) : 0.0;
        e.rms = rc * 1200.0;
        e.value = rc + 0.25 * rt;
    }
    return e;
}

// what a residual is in, by kind, for printing
inline const char* residualUnit(const RefTable& t) {
    if (t.kind == "B") return "ln(law / table)";
    if (t.kind == "decay") return "log2(law / table)";
    if (t.kind == "hammer") return "ln, ln, p";
    if (t.kind == "tine") return "cents; log2 t60";
    return "";
}

// ---- the knob fit ---------------------------------------------------------------------------------

struct FitSpec {
    bool  locked[kKnobCount] = {};
    float lockAt[kKnobCount] = {};
};

// the table's own holds ("# hold: KNOB=V ...", the constants' fit's) and the caller's locks over them
inline FitSpec holdSpec(const RefTable& t, const FitSpec& over = FitSpec()) {
    FitSpec s;
    std::istringstream ss(t.hold);
    std::string pair;
    while (ss >> pair) {
        const size_t eq = pair.find('=');
        Knob k;
        if (eq == std::string::npos || !knobByName(pair.substr(0, eq).c_str(), k)) continue;
        s.locked[(int)k] = true;
        s.lockAt[(int)k] = knobHeld((float)std::atof(pair.c_str() + eq + 1));
    }
    for (int i = 0; i < kKnobCount; ++i)
        if (over.locked[i]) { s.locked[i] = true; s.lockAt[i] = over.lockAt[i]; }
    return s;
}

// the constants' fit's spec for a table: its holds, and the knob its kind's constants trade with held
// at the anchor (a hammer table's hammer at 0: the medium hammer is the reference hammers' own; a tine
// table's generator at 0: the tine's constants are the anchor's)
inline FitSpec constantsSpec(const RefTable& t) {
    FitSpec s = holdSpec(t);
    if (t.kind == "hammer" && !s.locked[(int)Knob::hammer]) { s.locked[(int)Knob::hammer] = true; s.lockAt[(int)Knob::hammer] = 0.0f; }
    if (t.kind == "tine" && !s.locked[(int)Knob::generator]) { s.locked[(int)Knob::generator] = true; s.lockAt[(int)Knob::generator] = 0.0f; }
    return s;
}

inline bool fitsKnobs(const RefTable& t) { return t.kind == "B" || t.kind == "decay" || t.kind == "hammer" || t.kind == "tine"; }

// the table's starting vector: its anchor, then its locks
inline Knobs startOf(const RefTable& t, const FitSpec& spec) {
    Knobs k = knobAnchors()[t.kind == "tine" ? 1 : 0].k;
    for (int i = 0; i < kKnobCount; ++i)
        if (spec.locked[i]) k.v[i] = spec.lockAt[i];
    return k;
}

inline void freeKnobs(const RefTable& t, const FitSpec& spec, std::vector<int>& out) {
    out.clear();
    Knob k = Knob::count;
    if (t.kind == "B" || t.kind == "decay") k = Knob::scale;
    else if (t.kind == "hammer") k = Knob::hammer;
    else if (t.kind == "tine") k = Knob::generator;
    if (k != Knob::count && !spec.locked[(int)k]) out.push_back((int)k);
}

// coordinate descent from k over the free knobs, the step halving from `step` to 0.01
inline double descend(const RefTable& t, Knobs& k, const std::vector<int>& free, const LawConstants& c, const Spec& sets,
                      float step) {
    double best = tableError(t, k, c, sets).value;
    for (float s = step; s >= 0.01f - 1e-6f; s *= 0.5f) {
        bool moved = true;
        int rounds = 0;
        while (moved && rounds++ < 60) {
            moved = false;
            for (int i : free) {
                for (int dir = -1; dir <= 1; dir += 2) {
                    Knobs x = k;
                    x.v[i] = clamp(k.v[i] + dir * s, -1.0f, 1.0f);
                    if (x.v[i] == k.v[i]) continue;
                    const double e = tableError(t, x, c, sets).value;
                    if (e < best - 1e-12) { best = e; k = x; moved = true; }
                }
            }
        }
    }
    return best;
}

struct KnobFit {
    Knobs k{};
    TableError error;           // at the vector found
    TableError atAnchor;        // at the starting vector
};

// the vector nearest a table: a grid in fifths over the free knobs, then descent to 0.01
inline KnobFit fitTable(const RefTable& t, const LawConstants& c, const FitSpec& spec, const Spec& sets = Spec()) {
    KnobFit f;
    std::vector<int> free;
    freeKnobs(t, spec, free);
    const Knobs start = startOf(t, spec);
    f.atAnchor = tableError(t, start, c, sets);
    const float fifths[5] = {-1.0f, -0.5f, 0.0f, 0.5f, 1.0f};
    Knobs best = start;
    double bestE = f.atAnchor.value;
    std::vector<int> idx(free.size(), 0);
    for (;;) {
        Knobs k = start;
        for (size_t i = 0; i < free.size(); ++i) k.v[free[i]] = fifths[idx[i]];
        const double e = tableError(t, k, c, sets).value;
        if (e < bestE) { bestE = e; best = k; }
        size_t d = 0;
        while (d < free.size() && ++idx[d] == 5) idx[d++] = 0;
        if (d == free.size()) break;
    }
    descend(t, best, free, c, sets, 0.25f);
    f.k = best;
    f.error = tableError(t, best, c, sets);
    return f;
}

// ---- the constants' fit ---------------------------------------------------------------------------

struct ConstantSpec {
    const char* name;
    float LawConstants::*field;
    float lo, hi, step;         // the search's span and first step (a factor 2^step when `log`)
    bool  log;
    const char* kind;           // the table kind that moves it ("none": no table kind, so --fix and the print name it only)
};
inline const std::vector<ConstantSpec>& constantSpecs() {
    static const std::vector<ConstantSpec> s = {
        {"kTension", &LawConstants::tension, 450.0f, 700.0f, 50.0f, false, "B"},
        {"kRankLength", &LawConstants::rankLength, 0.85f, 1.0f, 0.04f, false, "B"},
        {"kDmax", &LawConstants::dMax, 0.6e-3f, 1.6e-3f, 0.05e-3f, false, "B"},
        {"kCore", &LawConstants::core, 0.5e-3f, 1.8e-3f, 0.05e-3f, false, "B"},
        {"kCoreGrow", &LawConstants::coreGrow, 0.0f, 0.25f, 0.02f, false, "B"},
        {"kL0", &LawConstants::l0, 0.55f, 0.7f, 0.02f, false, "B"},
        {"kScale", &LawConstants::scale, 0.0f, 0.8f, 0.05f, false, "B"},
        {"kBass0", &LawConstants::bass0, 0.8f, 2.4f, 0.05f, false, "B"},
        {"kScaleBass", &LawConstants::scaleBass, 0.1f, 1.0f, 0.05f, false, "B"},
        {"kLoss", &LawConstants::loss, 0.05f, 4.0f, 0.5f, true, "decay"},
        {"kLossKnee", &LawConstants::lossKnee, 300.0f, 30000.0f, 0.5f, true, "decay"},
        {"kLossSlope", &LawConstants::lossSlope, 0.5f, 4.0f, 0.1f, false, "decay"},
        {"kRankLoss", &LawConstants::rankLoss, 0.0f, 1.5f, 0.05f, false, "decay"},
        {"kK", &LawConstants::k, 1.0f, 2.0e6f, 1.0f, true, "hammer"},
        {"kP", &LawConstants::p, 2.0f, 3.5f, 0.1f, false, "hammer"},
        {"kMassC4", &LawConstants::massC4, 1.0f, 20.0f, 0.25f, false, "hammer"},
        {"kRankMass", &LawConstants::rankMass, 0.0f, 1.0f, 0.02f, false, "hammer"},
        {"kRankHardLo", &LawConstants::rankHardLo, 0.0f, 2.5f, 0.1f, false, "hammer"},
        {"kRankHardHi", &LawConstants::rankHardHi, 0.0f, 2.5f, 0.1f, false, "hammer"},
        {"kRankP", &LawConstants::rankP, 0.0f, 0.5f, 0.02f, false, "hammer"},
        {"kTineT60", &LawConstants::tineT60, 0.5f, 60.0f, 0.5f, true, "tine"},
        {"kTineSlope", &LawConstants::tineSlope, 0.0f, 3.0f, 0.05f, false, "tine"},
        {"kBridgeMobility", &LawConstants::bridgeMobility, 1e-4f, 2e-3f, 0.5f, true, "none"},   // no table measures it: named, never fitted
        {"kBridgeReactive", &LawConstants::bridgeReactive, 0.0f, 2.0f, 0.1f, false, "none"},   // the same
    };
    return s;
}

struct TableFitState {
    RefTable table;
    FitSpec spec;
    KnobFit fit;
};

// the sum of the tables' errors under constants, each table's vector descended from where it
// stands (fine steps); the vectors are left where the descent took them
inline double constantsError(std::vector<TableFitState>& tables, const LawConstants& c, std::vector<double>* each = nullptr) {
    double sum = 0.0;
    if (each) each->clear();
    for (TableFitState& t : tables) {
        std::vector<int> free;
        freeKnobs(t.table, t.spec, free);
        Knobs k = t.fit.k;
        const double e = descend(t.table, k, free, c, Spec(), 0.1f);
        t.fit.k = k;
        sum += e;
        if (each) each->push_back(e);
    }
    return sum;
}

// coordinate descent on the constants the tables' kinds move, the steps halving three times; then
// every table refitted whole at the new constants and the descent run again, until nothing moves
// (`fixed` names constants held where they are, by spaces or commas)
inline LawConstants fitConstants(std::vector<TableFitState>& tables, const LawConstants& from, int* passesOut = nullptr,
                                 const std::string& fixed = std::string()) {
    LawConstants c = from;
    std::vector<const ConstantSpec*> live;
    for (const ConstantSpec& cs : constantSpecs()) {
        std::string list = " " + fixed + " ";
        for (char& ch : list)
            if (ch == ',') ch = ' ';
        if (list.find(std::string(" ") + cs.name + " ") != std::string::npos) continue;
        for (const TableFitState& t : tables)
            if (t.table.kind == cs.kind) { live.push_back(&cs); break; }
    }
    int passes = 0;
    for (int round = 0; round < 4; ++round) {
        ++passes;
        for (TableFitState& t : tables) t.fit = fitTable(t.table, c, t.spec);
        double best = constantsError(tables, c);
        bool movedAny = false;
        for (int level = 0; level < 4; ++level) {
            const float scale = std::ldexp(1.0f, -level);
            bool moved = true;
            int sweeps = 0;
            while (moved && sweeps++ < 30) {
                moved = false;
                for (const ConstantSpec* cs : live) {
                    for (int dir = -1; dir <= 1; dir += 2) {
                        LawConstants x = c;
                        const float v = c.*(cs->field);
                        float nv = cs->log ? v * std::exp2(dir * cs->step * scale) : v + dir * cs->step * scale;
                        nv = clamp(nv, cs->lo, cs->hi);
                        if (nv == v) continue;
                        x.*(cs->field) = nv;
                        std::vector<TableFitState> trial = tables;
                        const double e = constantsError(trial, x);
                        if (e < best - 1e-9) {
                            best = e;
                            c = x;
                            tables = trial;
                            moved = movedAny = true;
                        }
                    }
                }
            }
        }
        if (!movedAny) break;
    }
    for (TableFitState& t : tables) t.fit = fitTable(t.table, c, t.spec);
    if (passesOut) *passesOut = passes;
    return c;
}

}  // namespace up
