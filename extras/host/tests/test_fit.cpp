// upright — tests/test_fit.cpp  (plan §15; the tests' "the fit" bullet)
//
// the rank's B per note against the reference grand and upright over each
// table's notes from c1 to c7 at the vector fit finds: every note within 50
// %, the rms ln residual under a bound; the fit on a table made from the
// law's own B at scale +0.4 finds it; every reference table reads.
#include <cmath>
#include <filesystem>
#include <initializer_list>
#include <string>

#include "core/up_knobs.h"
#include "core/up_rank.h"
#include "up_fit.h"
#include "up_test.h"

using namespace up;

namespace {

// a B table's notes from c1 (24) to c7 (96) at the fitted vector: the worst miss and its note, the rms of
// ln(B_law / B_table) over those notes, and how many notes
struct Miss { double worst = INFINITY, rms = INFINITY, worstMidi = 0.0; int notes = 0; Knobs at{}; };
Miss missOf(const char* file) {
    Miss m;
    RefTable t;
    std::string err;
    if (!readTable(std::string(UP_REF_DIR) + "/" + file, t, err)) {
        CHECK_MSG(false, "%s", err.c_str());
        return m;
    }
    if (bPoints(t).empty()) { m.worst = NAN; return m; }   // a table the sources left empty
    const KnobFit f = fitTable(t, lawConstants(), FitSpec());
    m.at = f.k;
    const Shape s = shapeOf(f.k, lawConstants());
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

// the grand's and the upright's measured B from c1 to c7 at the vector the
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
        printf("    %s: %d notes from c1 to c7 at scale %+.3f: the worst %s %.1f %% off, the rms ln residual %.3f\n", files[i], m.notes,
               m.at.v[(int)Knob::scale], noteName(m.worstMidi).c_str(), 100.0 * m.worst, m.rms);
        KNOWN_FAILURE(m.worst <= 0.5, "%s: the worst note %s %.1f %% off the measured B (the gate 50 %%)", files[i], noteName(m.worstMidi).c_str(),
                      100.0 * m.worst);
        CHECK_MSG(m.rms < rmsBound[i], "%s: the rms ln residual %.3f (the gate %.2f)", files[i], m.rms, rmsBound[i]);
    }
}

// a table made from the law's own B at scale +0.4 (a0 to c8 by thirds):
// the fit returns scale within 0.05
UP_TEST(fit_finds_its_own_scale) {
    Knobs k = knobAnchors()[0].k;
    k.v[(int)Knob::scale] = 0.4f;
    const Shape s = pianoOf(k);
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
    const KnobFit f = fitTable(t, lawConstants(), FitSpec());
    CHECK_MSG(std::fabs(f.k.v[(int)Knob::scale] - 0.4f) <= 0.05f, "the fit found scale %+.3f", f.k.v[(int)Knob::scale]);
    printf("    the law's own B at scale +0.4: the fit finds %+.3f (rms %.2g)\n", f.k.v[(int)Knob::scale], f.error.rms);
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
