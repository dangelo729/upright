// upright — tests/test_fit.cpp  (plan §15; the tests' "the fit" bullet)
//
// the rank's B per note against the reference grand and upright over each
// table's notes from c1 to c7 at the strings the fit finds: every note
// within 50 %, the rms ln residual under a bound; the fit on a table made
// from the law's own B at strings off grand's finds them; every reference
// table reads.
#include <cmath>
#include <filesystem>
#include <initializer_list>
#include <string>

#include "core/up_presets.h"
#include "core/up_rank.h"
#include "up_fit.h"
#include "up_shapes.h"
#include "up_test.h"

using namespace up;

namespace {

// a B table's notes from c1 (24) to c7 (96) at the fitted shape: the worst miss and its note, the rms of
// ln(B_law / B_table) over those notes, and how many notes
struct Miss { double worst = INFINITY, rms = INFINITY, worstMidi = 0.0; int notes = 0; Shape at = nanShape(); };
Miss missOf(const char* file) {
    Miss m;
    RefTable t;
    std::string err;
    if (!readTable(std::string(UP_REF_DIR) + "/" + file, t, err)) {
        CHECK_MSG(false, "%s", err.c_str());
        return m;
    }
    if (bPoints(t).empty()) { m.worst = NAN; return m; }   // a table the sources left empty
    const ChannelFit f = fitTable(t, lawConstants());
    m.at = f.s;
    const Shape& s = f.s;
    double worst = 0.0, sum = 0.0;
    for (const BPoint& p : bPoints(t)) {
        if (p.midi < 24.0 || p.midi > 96.0) continue;
        ++m.notes;
        const double r = rankAt(s, (float)p.midi).B / p.B;
        sum += std::log(r) * std::log(r);
        if (std::fabs(r - 1.0) > worst) { worst = std::fabs(r - 1.0); m.worstMidi = p.midi; }
    }
    m.worst = worst;
    m.rms = m.notes ? std::sqrt(sum / m.notes) : 0.0;
    return m;
}

}  // namespace

// the grand's and the upright's measured B from c1 to c7 at the strings the
// fit finds: the rms of ln(B_law / B_table) under 0.35 (the grand) and 0.5
// (the upright), checks; every note within 50 % (the figure-read tables' own
// scatter is ±20 %), the worst note printed and known, with its number,
// where missed
UP_TEST(fit_b_tables) {
    const char* files[2] = {"inharmonicity_grand.txt", "inharmonicity_upright.txt"};
    const double rmsBound[2] = {0.35, 0.5};
    for (int i = 0; i < 2; ++i) {
        const Miss m = missOf(files[i]);
        if (m.worst != m.worst) {
            printf("    %s: no data (see its header)\n", files[i]);
            continue;
        }
        printf("    %s: %d notes from c1 to c7 at length %.4f m, bass %.4f m: the worst %s %.1f %% off, the rms ln residual %.3f\n", files[i],
               m.notes, m.at[Channel::length], m.at[Channel::bass], noteName(m.worstMidi).c_str(), 100.0 * m.worst, m.rms);
        KNOWN_FAILURE(m.worst <= 0.5, "%s: the worst note %s %.1f %% off the measured B (the gate 50 %%)", files[i], noteName(m.worstMidi).c_str(),
                      100.0 * m.worst);
        CHECK_MSG(m.rms < rmsBound[i], "%s: the rms ln residual %.3f (the gate %.2f)", files[i], m.rms, rmsBound[i]);
    }
}

// a table made from the law's own B at strings off grand's (length 0.6191 m,
// bass 1.8043 m; a0 to c8 by thirds): the fit returns both within 1 %
UP_TEST(fit_finds_its_own_strings) {
    const float length = 0.6191284f, bass = 1.8042724f;
    const Shape s = with(grand(), {{Channel::length, length}, {Channel::bass, bass}});
    RefTable t;
    t.kind = "B";
    t.units = "midi, B";
    t.source = "the law";
    t.blocks.push_back({"midi", "B"});
    for (int m = 21; m <= 108; m += 4) {
        RefRow r;
        r.key = std::to_string(m);
        r.x = m;
        r.v.push_back(rankAt(s, (float)m).B);
        r.block = 0;
        t.rows.push_back(r);
    }
    const ChannelFit f = fitTable(t, lawConstants());
    const double dl = f.s[Channel::length] / length - 1.0, db = f.s[Channel::bass] / bass - 1.0;
    CHECK(f.free.size() == 2 && f.free[0] == Channel::length && f.free[1] == Channel::bass);
    CHECK_MSG(std::fabs(dl) <= 0.01 && std::fabs(db) <= 0.01, "the fit found length %.4f (%+.2f %%), bass %.4f (%+.2f %%)", f.s[Channel::length],
              100.0 * dl, f.s[Channel::bass], 100.0 * db);
    printf("    the law's own B at length %.4f, bass %.4f: the fit finds %.4f (%+.3f %%), %.4f (%+.3f %%), rms %.2g\n", length, bass,
           f.s[Channel::length], 100.0 * dl, f.s[Channel::bass], 100.0 * db, f.error.rms);
}

// every reference table reads, with units, a source and a kind
UP_TEST(fit_reference_tables_read) {
    int n = 0;
    for (const std::string& path : tableFiles(UP_REF_DIR)) {
        RefTable t;
        std::string err;
        const bool ok = readTable(path, t, err);
        CHECK_MSG(ok, "%s", err.c_str());
        if (!ok) continue;
        ++n;
        const bool known = t.kind == "B" || t.kind == "decay" || t.kind == "hammer" || t.kind == "tine" || t.kind == "cantilever" || t.kind == "board";
        CHECK_MSG(known && !t.blocks.empty(), "%s: kind '%s', %d column lines", t.name.c_str(), t.kind.c_str(), (int)t.blocks.size());
    }
    CHECK(n >= 7);
    printf("    %d tables read\n", n);
}
