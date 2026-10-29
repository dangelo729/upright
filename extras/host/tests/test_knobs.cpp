// upright — tests/test_knobs.cpp  (plan §2; the tests' "the knobs" bullet)
//
// the eight knobs: their names and ends; a nan as 0 and past the end the end;
// each knob writes the channels its recipe names and no other; every written
// channel inside its span over the grid in fifths; knobsOf reads pianoOf
// back within 1e-3 over the grid in fifths, and an instrument set by hand to
// its nearest point; print's line reads back to the bit and compiles.
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include "core/up_knobs.h"
#include "up_fit.h"
#include "up_test.h"

using namespace up;

namespace {

bool sameBits(float a, float b) {
    uint32_t x, y;
    std::memcpy(&x, &a, sizeof x);
    std::memcpy(&y, &b, sizeof y);
    return x == y;
}

Knobs with(Knobs k, Knob which, float x) {
    k.v[(int)which] = x;
    return k;
}

std::vector<Channel> differing(const Shape& a, const Shape& b) {
    std::vector<Channel> out;
    for (int i = 0; i < kChannelCount; ++i)
        if (!sameBits(a.v[i], b.v[i])) out.push_back((Channel)i);
    return out;
}

std::string names(const std::vector<Channel>& set) {
    std::string s;
    for (Channel c : set) s += (s.empty() ? "" : " ") + std::string(channelName(c));
    return s.empty() ? "none" : s;
}

}  // namespace

// the names go both ways; the ends are the ear's; a nan knob reads as 0 and
// ±3 as ±1 at every anchor, every channel compared by its bits; the partial
// and player channels nan
UP_TEST(knobs_names_and_ends) {
    const char* ends[kKnobCount][3] = {{"spinet", "grand", "concert"}, {"soft", "medium", "hard"},  {"end", "eighth", "middle"},
                                       {"one", "three", "wide"},       {"upright", "parlor", "concert"}, {"string", "tine", "reed"},
                                       {"far", "set", "close"},        {"heavy", "felt", "light"}};
    bool all = true;
    for (int i = 0; i < kKnobCount; ++i) {
        Knob k = Knob::count;
        CHECK_MSG(knobByName(knobName((Knob)i), k) && k == (Knob)i, "%s does not read back", knobName((Knob)i));
        for (int e = 0; e < 3; ++e) all = all && std::strcmp(knobEnd((Knob)i, e), ends[i][e]) == 0;
        CHECK(std::strcmp(knobEnd((Knob)i, 3), "") == 0 && std::strcmp(knobEnd((Knob)i, -1), "") == 0);
        printf("    %-9s  %s · %s · %s\n", knobName((Knob)i), knobEnd((Knob)i, 0), knobEnd((Knob)i, 1), knobEnd((Knob)i, 2));
    }
    CHECK(all);
    Knob k;
    CHECK(!knobByName("brightness", k) && !knobByName(nullptr, k) && std::strcmp(knobName(Knob::count), "") == 0);
    int differ = 0;
    for (int a = 0; a < kAnchorCount; ++a) {
        const Knobs base = knobAnchors()[a].k;
        const Shape s = pianoOf(base);
        bool nans = std::isnan(s[Channel::midi]) && std::isnan(s[Channel::glide]) && std::isnan(s[Channel::pedal]);
        for (int m = 0; m < kMaxModes; ++m) nans = nans && std::isnan(s[ratioChannel(m)]) && std::isnan(s[outChannel(m)]);
        CHECK(nans);
        for (int i = 0; i < kKnobCount; ++i) {
            const auto n = differing(pianoOf(with(base, (Knob)i, std::nanf(""))), pianoOf(with(base, (Knob)i, 0.0f)));
            const auto p = differing(pianoOf(with(base, (Knob)i, 3.0f)), pianoOf(with(base, (Knob)i, 1.0f)));
            const auto m = differing(pianoOf(with(base, (Knob)i, -3.0f)), pianoOf(with(base, (Knob)i, -1.0f)));
            CHECK_MSG(n.empty(), "%s: %s nan is not 0 (%s)", knobAnchors()[a].name, knobName((Knob)i), names(n).c_str());
            CHECK_MSG(p.empty() && m.empty(), "%s: %s at ±3 is not ±1", knobAnchors()[a].name, knobName((Knob)i));
            differ += (int)(n.size() + p.size() + m.size());
        }
    }
    printf("    at the three anchors, each knob nan against 0 and ±3 against ±1: %d channels differ\n", differ);
}

// at each anchor each knob alone at -1 and +1 moves exactly the channels its
// recipe names (by their bits):
//   scale length and bass; hammer hardness and felt; strike
//   strikeShare; unison strings under 0 and detune over it (from an anchor
//   on the other side of 0, both); body boardArea,
//   boardLoss and open; generator generator and tonebar; pickup gap and
//   offset; damper dampRate
UP_TEST(knobs_write_what_they_name) {
    typedef std::vector<Channel> Set;
    const Set want[kKnobCount][2] = {
        {{Channel::length, Channel::bass}, {Channel::length, Channel::bass}},
        {{Channel::hardness, Channel::felt}, {Channel::hardness, Channel::felt}},
        {{Channel::strikeShare}, {Channel::strikeShare}},
        {{Channel::strings}, {Channel::detune}},
        {{Channel::boardArea, Channel::boardLoss, Channel::open}, {Channel::boardArea, Channel::boardLoss, Channel::open}},
        {{Channel::generator, Channel::tonebar}, {Channel::generator, Channel::tonebar}},
        {{Channel::gap, Channel::offset}, {Channel::gap, Channel::offset}},
        {{Channel::dampRate}, {Channel::dampRate}},
    };
    int rows = 0, right = 0;
    for (int a = 0; a < kAnchorCount; ++a) {
        const Knobs base = knobAnchors()[a].k;
        for (int i = 0; i < kKnobCount; ++i)
            for (int e = 0; e < 2; ++e) {
                const float x = e == 0 ? -1.0f : 1.0f;
                if (base.v[i] == x) continue;                      // the anchor is already at this end
                Set got = differing(pianoOf(with(base, (Knob)i, x)), pianoOf(base));
                Set expect = want[i][e];
                // unison from an anchor under 0 moves the strings toward +1 too, and from one over 0 the detune toward -1
                if (i == (int)Knob::unison && e == 1 && base.v[i] < 0.0f) expect = {Channel::strings, Channel::detune};
                if (i == (int)Knob::unison && e == 0 && base.v[i] > 0.0f) expect = {Channel::strings, Channel::detune};
                // the generator from -1 to +1 keeps |g|: the tonebar does not move
                if (i == (int)Knob::generator && std::fabs(base.v[i]) == 1.0f) expect = {Channel::generator};
                auto sorted = [](Set s) { std::sort(s.begin(), s.end()); return s; };
                const bool ok = sorted(got) == sorted(expect);
                ++rows;
                right += ok;
                CHECK_MSG(ok, "%s: %s %+.0f moves %s, not %s", knobAnchors()[a].name, knobName((Knob)i), x, names(got).c_str(), names(expect).c_str());
            }
    }
    printf("    %d of %d rows move exactly what they name\n", right, rows);
}

// knobsOf(pianoOf(k)) is k within 1e-3 over the grid in fifths over all
// eight (390,625 vectors), and every written channel inside its span
UP_TEST(knobs_round_trip) {
    const float fifths[5] = {-1.0f, -0.5f, 0.0f, 0.5f, 1.0f};
    double worst[kKnobCount] = {};
    int idx[kKnobCount] = {};
    long points = 0, outside = 0;
    float lo[kInstrumentChannels], hi[kInstrumentChannels];
    for (int c = 0; c < kInstrumentChannels; ++c) channelRange((Channel)c, lo[c], hi[c]);
    for (;;) {
        Knobs k;
        for (int i = 0; i < kKnobCount; ++i) k.v[i] = fifths[idx[i]];
        const Shape s = pianoOf(k);
        const Knobs back = knobsOf(s);
        ++points;
        for (int i = 0; i < kKnobCount; ++i) worst[i] = std::fmax(worst[i], std::fabs((double)back.v[i] - k.v[i]));
        for (int c = 0; c < kInstrumentChannels; ++c)
            if (!(s.v[c] >= lo[c] && s.v[c] <= hi[c])) {
                if (outside < 5) CHECK_MSG(false, "%s %g outside %g .. %g", channelName((Channel)c), s.v[c], lo[c], hi[c]);
                ++outside;
            }
        int d = 0;
        while (d < kKnobCount && ++idx[d] == 5) idx[d++] = 0;
        if (d == kKnobCount) break;
    }
    for (int i = 0; i < kKnobCount; ++i) {
        CHECK_MSG(worst[i] <= 1e-3, "%s reads back %.3g off", knobName((Knob)i), worst[i]);
        printf("    %-9s worst %.2g\n", knobName((Knob)i), worst[i]);
    }
    CHECK(outside == 0);
    printf("    %ld vectors; %ld channels written outside their span\n", points, outside);
}

// an instrument set by hand reads its nearest point: the hard end's
// hardness with the felt at the medium's reads half the way; past the ends
// the ends; the unison's two pieces
UP_TEST(knobs_read_the_nearest_point) {
    Shape s = pianoOf(knobAnchors()[0].k);
    Knobs hard = knobAnchors()[0].k;
    hard.v[(int)Knob::hammer] = 1.0f;
    s[Channel::hardness] = pianoOf(hard)[Channel::hardness];   // the hard end's hardness alone
    const float h = knobsOf(s).v[(int)Knob::hammer];
    CHECK_MSG(std::fabs(h - 0.5f) < 1e-3f, "hammer read %.4f", h);
    Shape u = pianoOf(knobAnchors()[0].k);
    u[Channel::length] = 40.0f;
    u[Channel::bass] = 40.0f;
    u[Channel::strikeShare] = -1.0f;
    u[Channel::dampRate] = 1e-6f;
    const Knobs k = knobsOf(u);
    CHECK(k.v[(int)Knob::scale] == 1.0f && k.v[(int)Knob::strike] == -1.0f && k.v[(int)Knob::damper] == 1.0f);
    Shape w = pianoOf(knobAnchors()[0].k);
    w[Channel::strings] = 2.0f;
    CHECK(std::fabs(knobsOf(w).v[(int)Knob::unison] + 0.5f) < 1e-6f);
    w[Channel::strings] = 3.0f;
    w[Channel::detune] = 0.5f + 0.25f * 19.5f;
    CHECK(std::fabs(knobsOf(w).v[(int)Knob::unison] - 0.5f) < 1e-4f);
    printf("    the hard end's hardness alone reads hammer %+.4f\n", h);
}

// print's line read back is the same vector to the bit, at the anchors and
// over 1000 draws; grand's line as print writes it, and that line compiled;
// random draws the same vectors from a seed
UP_TEST(knobs_print_round_trips) {
    int lines = 0, same = 0;
    auto roundTrip = [&](const Knobs& k) {
        const std::string line = knobLine("vector", k, 21, 108);
        std::string name;
        Knobs back{};
        int lo = 0, hi = 0;
        const bool ok = parseKnobLine(line, name, back, lo, hi);
        bool bits = ok && name == "vector" && lo == 21 && hi == 108;
        for (int i = 0; i < kKnobCount; ++i) bits = bits && sameBits(back.v[i], knobHeld(k.v[i]));
        CHECK_MSG(bits, "%s does not read back", line.c_str());
        ++lines;
        same += bits;
    };
    for (int a = 0; a < kAnchorCount; ++a) roundTrip(knobAnchors()[a].k);
    Rng r;
    r.seed(12345);
    for (int i = 0; i < 1000; ++i) roundTrip(drawKnobs(r));
    printf("    %d lines printed and read back: %d the same vector to the bit\n", lines, same);
    const std::string line = knobLine("grand", knobAnchors()[0].k, 21, 108);
    struct Row { const char* name; Knobs k; int lo, hi; };
    const Row compiled = {"grand", {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f}, 21, 108};
    CHECK_MSG(line == "{\"grand\", {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f}, 21, 108},", "print wrote %s", line.c_str());
    bool bits = compiled.lo == 21 && compiled.hi == 108;
    for (int i = 0; i < kKnobCount; ++i) bits = bits && sameBits(compiled.k.v[i], knobAnchors()[0].k.v[i]);
    CHECK(bits);
    Rng a, b, c;
    a.seed(1);
    b.seed(1);
    c.seed(2);
    int twice = 0, other = 0;
    for (int i = 0; i < 8; ++i) {
        const Knobs x = drawKnobs(a), y = drawKnobs(b), z = drawKnobs(c);
        bool eq = true, ne = false;
        for (int j = 0; j < kKnobCount; ++j) {
            eq = eq && sameBits(x.v[j], y.v[j]);
            ne = ne || !sameBits(x.v[j], z.v[j]);
        }
        twice += eq;
        other += ne;
    }
    CHECK(twice == 8 && other == 8);
}
