// upright — up_fit.h
// what uptool and the tests share: the three anchors (the presets grand,
// rhodes, wurlitzer), a shape's --set lines and their parser, the span a
// tool draws, sweeps and searches a channel over, random's draw, note names;
// the reference tables read; each table's error against the law at a shape;
// the channel fit and the constants' fit. host only.
//
// a reference table (extras/reference/*.txt, README.txt skipped) is plain
// text: `#` comments, a `# units:`, a `# source:`, a `# kind:` (B, decay,
// hammer, tine, cantilever, board) and one or more `# columns:` lines, each
// naming the columns of the rows after it; a data line is `key value ...`,
// the key a midi number, a note name (A0, C#4, Bb2), a partial number or a
// name, a value a number or `-` (none). `# hold: CHANNEL=V ...` holds
// channels in the constants' fit (the fit of one table searches freely); `#
// midi: M` names the note a tine table's partials were measured at.
//
// a table's error, by kind, at a shape under constants:
//   B        the rms of ln(B_law / B_table) over its notes
//   decay    the rms of log2(t60_law / t60_table) over its partials; a loss
//            block (midi b1 b3) stands for its partials 1 .. 10 at sigma =
//            b1 + b3 w², w = 2 pi n f0
//   hammer   the rms over its notes of ln(m_law / m), ln(K_law / K) (K in
//            N/mm^p: the force at 1 mm) and p_law - p, the law's K and p at
//            the note through the rank (hardnessScale, pAdd)
//   tine     the rms of its partials' ratios in octaves (cents / 1200) plus a
//            quarter of the rms log2 t60 where it has them, at its # midi:
// the channels a table's fit moves: B and decay the strings' length and
// bass, hammer the hardness (the hammer tables measure the mass, K and p:
// the felt's loss is not in them), tine the generator and the tine's t60;
// the rest at the anchor (grand, or rhodes for a tine) unless held (a
// table's holds, or a channel --set names). each channel moves along its
// span (channelSpan) as a coordinate 0 .. 1: a grid in fifths over the free
// channels, then coordinate descent with the step halving from 0.25 to
// 0.001.
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
#include "core/up_law.h"
#include "core/up_presets.h"
#include "core/up_rank.h"

namespace up {

// ---- the shapes on the host -----------------------------------------------------------------------

// the three anchors: the string, the tine, the reed, each the preset of its name (up_presets.h)
struct Anchor {
    const char* name;
    Shape s;
};
inline const Anchor* anchors() {
    static const Anchor a[3] = {
        {"grand", presetByName("grand")->shape()},
        {"rhodes", presetByName("rhodes")->shape()},
        {"wurlitzer", presetByName("wurlitzer")->shape()},
    };
    return a;
}
constexpr int kAnchorCount = 3;
inline const Anchor* anchorByName(const char* name) {
    for (int i = 0; i < kAnchorCount; ++i)
        if (name && std::strcmp(anchors()[i].name, name) == 0) return &anchors()[i];
    return nullptr;
}

// the span a tool draws, sweeps and searches a channel over: its channelRange, on a log scale where the range
// spans a decade or more (the three that start at 0 and do, detune, felt and dampRate, from a thousandth of the
// top); true when log
inline bool channelSpan(Channel c, float& lo, float& hi) {
    channelRange(c, lo, hi);
    if (c == Channel::detune || c == Channel::felt || c == Channel::dampRate) {
        lo = hi * 1e-3f;
        return true;
    }
    return lo > 0.0f && hi >= 10.0f * lo;
}
// the value at a coordinate 0 .. 1 along the span, and a value's coordinate (held inside 0 .. 1)
inline float spanAt(Channel c, float u) {
    float lo, hi;
    const bool lg = channelSpan(c, lo, hi);
    u = clamp(u, 0.0f, 1.0f);
    return lg ? lo * std::pow(hi / lo, u) : lo + (hi - lo) * u;
}
inline float spanOf(Channel c, float x) {
    float lo, hi;
    const bool lg = channelSpan(c, lo, hi);
    const float u = lg ? std::log(x / lo) / std::log(hi / lo) : (x - lo) / (hi - lo);
    return u == u ? clamp(u, 0.0f, 1.0f) : 0.0f;
}

// a float written short enough to read back to the same bits, without an exponent where %g can do without
inline std::string floatText(float x) {
    std::string first;
    for (int p = 1; p <= 9; ++p) {
        char b[48];
        std::snprintf(b, sizeof b, "%.*g", p, (double)x);
        const float y = std::strtof(b, nullptr);
        uint32_t ux, uy;
        std::memcpy(&ux, &x, sizeof ux);
        std::memcpy(&uy, &y, sizeof uy);
        if (ux != uy) continue;
        if (!std::strchr(b, 'e')) return b;
        if (first.empty()) first = b;
    }
    return first;
}

// a shape's instrument channels as --set lines, a line a group (the strings, the hammer, the body, the generator,
// the damper), each value read back to the same bits; a nan channel left out
inline std::string setLines(const Shape& s) {
    const Channel first[6] = {Channel::length, Channel::hardness, Channel::boardArea, Channel::generator, Channel::dampRate,
                              (Channel)kInstrumentChannels};
    std::string out;
    for (int g = 0; g < 5; ++g) {
        std::string line;
        for (int i = (int)first[g]; i < (int)first[g + 1]; ++i)
            if (s.v[i] == s.v[i]) line += std::string(" ") + channelName((Channel)i) + "=" + floatText(s.v[i]);
        if (!line.empty()) out += "--set" + line + "\n";
    }
    return out;
}

// the lines read back: every NAME=V after a --set, by spaces, onto the shape. false on anything else
inline bool parseSetLines(const std::string& text, Shape& s) {
    std::istringstream ss(text);
    std::string tok;
    while (ss >> tok) {
        if (tok == "--set") continue;
        const size_t eq = tok.find('=');
        Channel c;
        if (eq == std::string::npos || !channelByName(tok.substr(0, eq).c_str(), c)) return false;
        char* end = nullptr;
        const float v = std::strtof(tok.c_str() + eq + 1, &end);
        if (end == tok.c_str() + eq + 1 || *end != '\0') return false;
        s[c] = v;
    }
    return true;
}

// random's draw: every instrument channel inside its span, log-uniform where the span is log and uniform
// otherwise, from a seeded xorshift32; the partial and player channels nan
inline Shape drawShape(Rng& r) {
    Shape s = nanShape();
    for (int i = 0; i < kInstrumentChannels; ++i) s.v[i] = spanAt((Channel)i, 0.5f * (r.bipolar() + 1.0f));
    return s;
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

// ---- a shape with channels set over it ------------------------------------------------------------

typedef std::vector<std::pair<Channel, float>> Spec;

// the shape's instrument channels, --set's instrument channels over them (the partial channels nan, for the
// law; a partial channel set lands over the law's in partialsAt)
inline Shape withSets(const Shape& base, const Spec& sets = Spec()) {
    Shape s = nanShape();
    for (int i = 0; i < kInstrumentChannels; ++i) s.v[i] = base.v[i];
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

// ---- a table's error at a shape -------------------------------------------------------------------

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

inline TableError tableError(const RefTable& t, const Shape& at, const LawConstants& c, const Spec& sets = Spec()) {
    TableError e;
    const Shape s = withSets(at, sets);
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

// ---- the channel fit ------------------------------------------------------------------------------

// a spec's value for a channel: true when it names it
inline bool specHas(const Spec& s, Channel c, float* v = nullptr) {
    for (const auto& p : s)
        if (p.first == c) {
            if (v) *v = p.second;
            return true;
        }
    return false;
}
// a channel written into a spec, over its own entry where it has one
inline void specSet(Spec& s, Channel c, float v) {
    for (auto& p : s)
        if (p.first == c) { p.second = v; return; }
    s.push_back({c, v});
}

// the table's own holds ("# hold: CHANNEL=V ...", the constants' fit's)
inline Spec holdsOf(const RefTable& t) {
    Spec s;
    std::istringstream ss(t.hold);
    std::string pair;
    while (ss >> pair) {
        const size_t eq = pair.find('=');
        Channel c;
        if (eq == std::string::npos || !channelByName(pair.substr(0, eq).c_str(), c)) continue;
        specSet(s, c, std::strtof(pair.c_str() + eq + 1, nullptr));
    }
    return s;
}

// the constants' fit's holds for a table: its own, and a tine table's generator at its anchor's (the tine's
// constants are the anchor's)
inline Spec constantsHolds(const RefTable& t) {
    Spec s = holdsOf(t);
    if (t.kind == "tine" && !specHas(s, Channel::generator)) specSet(s, Channel::generator, anchors()[1].s[Channel::generator]);
    return s;
}

inline bool fitsChannels(const RefTable& t) { return t.kind == "B" || t.kind == "decay" || t.kind == "hammer" || t.kind == "tine"; }

// the channels a table's kind moves, those held or set left out
inline std::vector<Channel> freeChannels(const RefTable& t, const Spec& held, const Spec& sets) {
    std::vector<Channel> kind;
    if (t.kind == "B" || t.kind == "decay") kind = {Channel::length, Channel::bass};
    else if (t.kind == "hammer") kind = {Channel::hardness};
    else if (t.kind == "tine") kind = {Channel::generator, Channel::tineT60};
    std::vector<Channel> out;
    for (Channel c : kind)
        if (!specHas(held, c) && !specHas(sets, c)) out.push_back(c);
    return out;
}

// the table's starting shape: its anchor, its holds over it
inline Shape startOf(const RefTable& t, const Spec& held) {
    Shape s = anchors()[t.kind == "tine" ? 1 : 0].s;
    for (const auto& p : held) s[p.first] = p.second;
    return s;
}

// coordinate descent from s over the free channels, each along its span, the step halving from `step` to about
// 0.001 of it
inline double descend(const RefTable& t, Shape& s, const std::vector<Channel>& free, const LawConstants& c, const Spec& sets,
                      float step) {
    double best = tableError(t, s, c, sets).value;
    for (float st = step; st > 0.0009f; st *= 0.5f) {
        bool moved = true;
        int rounds = 0;
        while (moved && rounds++ < 60) {
            moved = false;
            for (Channel ch : free) {
                for (int dir = -1; dir <= 1; dir += 2) {
                    Shape x = s;
                    x[ch] = spanAt(ch, spanOf(ch, s[ch]) + dir * st);
                    if (x[ch] == s[ch]) continue;
                    const double e = tableError(t, x, c, sets).value;
                    if (e < best - 1e-12) { best = e; s = x; moved = true; }
                }
            }
        }
    }
    return best;
}

struct ChannelFit {
    Shape s = nanShape();       // the shape found
    std::vector<Channel> free;  // the channels it moved
    TableError error;           // at the shape found
    TableError atAnchor;        // at the starting shape
};

// the shape nearest a table: a grid in fifths over the free channels' spans, then descent to 0.001
inline ChannelFit fitTable(const RefTable& t, const LawConstants& c, const Spec& held = Spec(), const Spec& sets = Spec()) {
    ChannelFit f;
    f.free = freeChannels(t, held, sets);
    const Shape start = startOf(t, held);
    f.atAnchor = tableError(t, start, c, sets);
    const float fifths[5] = {0.0f, 0.25f, 0.5f, 0.75f, 1.0f};
    Shape best = start;
    double bestE = f.atAnchor.value;
    std::vector<int> idx(f.free.size(), 0);
    for (;;) {
        Shape x = start;
        for (size_t i = 0; i < f.free.size(); ++i) x[f.free[i]] = spanAt(f.free[i], fifths[idx[i]]);
        const double e = tableError(t, x, c, sets).value;
        if (e < bestE) { bestE = e; best = x; }
        size_t d = 0;
        while (d < f.free.size() && ++idx[d] == 5) idx[d++] = 0;
        if (d == f.free.size()) break;
    }
    descend(t, best, f.free, c, sets, 0.25f);
    f.s = best;
    f.error = tableError(t, best, c, sets);
    return f;
}

// ---- the constants' fit ---------------------------------------------------------------------------

// a constant of the law, or a channel every table shares (its value written over every table's shape)
struct ConstantSpec {
    const char* name;
    float LawConstants::*field; // the law's constant; null for a channel
    Channel channel;            // the shared channel where field is null
    float lo, hi, step;         // the search's span and first step (a factor 2^step when `log`)
    bool  log;
    const char* kind;           // the table kind that moves it ("none": no table kind, so --fix and the print name it only)
};
inline const std::vector<ConstantSpec>& constantSpecs() {
    constexpr Channel none = Channel::count;
    static const std::vector<ConstantSpec> s = {
        {"tension", nullptr, Channel::tension, 450.0f, 700.0f, 50.0f, false, "B"},
        {"kRankLength", &LawConstants::rankLength, none, 0.85f, 1.0f, 0.04f, false, "B"},
        {"kDmax", &LawConstants::dMax, none, 0.6e-3f, 1.6e-3f, 0.05e-3f, false, "B"},
        {"kCore", &LawConstants::core, none, 0.5e-3f, 1.8e-3f, 0.05e-3f, false, "B"},
        {"kCoreGrow", &LawConstants::coreGrow, none, 0.0f, 0.25f, 0.02f, false, "B"},
        {"lossRate", nullptr, Channel::lossRate, 0.05f, 4.0f, 0.5f, true, "decay"},
        {"kLossKnee", &LawConstants::lossKnee, none, 300.0f, 30000.0f, 0.5f, true, "decay"},
        {"lossSlope", nullptr, Channel::lossSlope, 0.5f, 4.0f, 0.1f, false, "decay"},
        {"kRankLoss", &LawConstants::rankLoss, none, 0.0f, 1.5f, 0.05f, false, "decay"},
        {"hardness", nullptr, Channel::hardness, 1.0f, 10000.0f, 1.0f, true, "hammer"},
        {"exponent", nullptr, Channel::exponent, 2.0f, 3.5f, 0.1f, false, "hammer"},
        {"hammerMass", nullptr, Channel::hammerMass, 1.0f, 20.0f, 0.25f, false, "hammer"},
        {"kRankMass", &LawConstants::rankMass, none, 0.0f, 1.0f, 0.02f, false, "hammer"},
        {"kRankHardLo", &LawConstants::rankHardLo, none, 0.0f, 2.5f, 0.1f, false, "hammer"},
        {"kRankHardHi", &LawConstants::rankHardHi, none, 0.0f, 2.5f, 0.1f, false, "hammer"},
        {"kRankP", &LawConstants::rankP, none, 0.0f, 0.5f, 0.02f, false, "hammer"},
        {"kTineSlope", &LawConstants::tineSlope, none, 0.0f, 3.0f, 0.05f, false, "tine"},
        {"kBridgeMobility", &LawConstants::bridgeMobility, none, 1e-4f, 2e-3f, 0.5f, true, "none"},   // no table measures it: named, never fitted
        {"kBridgeReactive", &LawConstants::bridgeReactive, none, 0.0f, 2.0f, 0.1f, false, "none"},   // the same
    };
    return s;
}

// the shared channels at the anchor's values (grand's), where the fit starts
inline Spec sharedStart() {
    Spec s;
    for (const ConstantSpec& cs : constantSpecs())
        if (!cs.field) specSet(s, cs.channel, anchors()[0].s[cs.channel]);
    return s;
}

// a spec's value as the law's constant or the shared channel
inline float constantOf(const ConstantSpec& cs, const LawConstants& c, const Spec& shared) {
    float v = NAN;
    if (cs.field) return c.*(cs.field);
    specHas(shared, cs.channel, &v);
    return v;
}

struct TableFitState {
    RefTable table;
    Spec held;
    ChannelFit fit;
};

// the sum of the tables' errors under constants and the shared channels, each table's shape descended from where
// it stands (fine steps); the shapes are left where the descent took them
inline double constantsError(std::vector<TableFitState>& tables, const LawConstants& c, const Spec& shared,
                             std::vector<double>* each = nullptr) {
    double sum = 0.0;
    if (each) each->clear();
    for (TableFitState& t : tables) {
        Shape s = t.fit.s;
        const double e = descend(t.table, s, t.fit.free, c, shared, 0.05f);
        t.fit.s = s;
        sum += e;
        if (each) each->push_back(e);
    }
    return sum;
}

// coordinate descent on the constants and the shared channels the tables' kinds move, the steps halving three
// times; then every table refitted whole at the new values and the descent run again, until nothing moves
// (`fixed` names those held where they are, by spaces or commas). the shared channels' values come back in shared
inline LawConstants fitConstants(std::vector<TableFitState>& tables, const LawConstants& from, Spec& shared, int* passesOut = nullptr,
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
        for (TableFitState& t : tables) t.fit = fitTable(t.table, c, t.held, shared);
        double best = constantsError(tables, c, shared);
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
                        Spec xs = shared;
                        const float v = constantOf(*cs, c, shared);
                        float nv = cs->log ? v * std::exp2(dir * cs->step * scale) : v + dir * cs->step * scale;
                        nv = clamp(nv, cs->lo, cs->hi);
                        if (nv == v) continue;
                        if (cs->field) x.*(cs->field) = nv;
                        else specSet(xs, cs->channel, nv);
                        std::vector<TableFitState> trial = tables;
                        const double e = constantsError(trial, x, xs);
                        if (e < best - 1e-9) {
                            best = e;
                            c = x;
                            shared = xs;
                            tables = trial;
                            moved = movedAny = true;
                        }
                    }
                }
            }
        }
        if (!movedAny) break;
    }
    for (TableFitState& t : tables) t.fit = fitTable(t.table, c, t.held, shared);
    if (passesOut) *passesOut = passes;
    return c;
}

}  // namespace up
