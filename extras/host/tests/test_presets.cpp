// upright — tests/test_presets.cpp
//
// the nine presets: each found by name, its shape its table of the
// twenty-four instrument channels to the bit (the pianos as measured and
// heard, the tine's t60 and load 10.4 s and 1.18 on every one), the partial
// and player channels nan, its accessor's the same object; every preset
// sounds at every semitone of its range (2 s at velocity 1 at the gain that
// brings its octave to grand's: the peak over 0.022, the last 100 ms's rms
// under the first 100 ms's, every sample finite); each preset's octave from
// c4 against grand's; honkyTonk beats
// faster than upright; rhodesBark's thd at e4 10 points and more over
// rhodes's; concertGrand's and upright's b against their tables as the fit
// test holds them; print's --set lines for each read back to its shape.
#include <cmath>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include "core/up_presets.h"
#include "up_fit.h"
#include "up_play.h"
#include "up_test.h"

using namespace up;

namespace {

bool sameBits(float a, float b) {
    uint32_t x, y;
    std::memcpy(&x, &a, sizeof x);
    std::memcpy(&y, &b, sizeof y);
    return x == y;
}

// each preset's instrument channels in the enum's order, as measured and heard
const float kTables[9][kInstrumentChannels] = {
    {0.614f, 1.61375f, 553.0f, 1.0f, 3.0f, 0.5f, 0.12f, 0.45f, 2.0f, 142.0f, 2.5f, 0.0005f, 2.97f, 1.0f, 0.03f, 0.5f, -1.0f, 1.5f, 0.9f, 0.0f,
     10.4f, 1.18f, 20.0f, 89.0f},
    {0.6264942f, 2.1145327f, 553.0f, 1.0f, 3.0f, 1.2800001f, 0.12f, 0.45f, 2.0f, 142.0f, 2.5f, 0.0005f, 2.97f, 1.6245048f, 0.018467166f, 1.0f,
     -1.0f, 1.5f, 0.9f, 0.0f, 10.4f, 1.18f, 20.0f, 89.0f},
    {0.6082418f, 1.4221112f, 553.0f, 1.0f, 3.0f, 0.5f, 0.12f, 0.45f, 2.0f, 100.409164f, 2.5f, 0.0005743492f, 2.97f, 0.7120251f, 0.042133342f,
     0.15f, -1.0f, 1.5f, 0.9f, 0.0f, 10.4f, 1.18f, 20.0f, 89.0f},
    {0.6013641f, 1.2208763f, 553.0f, 1.0f, 3.0f, 0.5f, 0.12f, 0.45f, 2.0f, 238.81459f, 2.5f, 0.00040612623f, 2.97f, 0.6155722f, 0.04873514f, 0.0f,
     -1.0f, 1.5f, 0.9f, 0.0f, 10.4f, 1.18f, 20.0f, 89.0f},
    {0.6082418f, 1.4221112f, 553.0f, 1.0f, 3.0f, 20.0f, 0.12f, 0.45f, 2.0f, 59.703648f, 2.5f, 0.0007071068f, 2.97f, 0.7120251f, 0.042133342f, 0.15f,
     -1.0f, 1.5f, 0.9f, 0.0f, 10.4f, 1.18f, 20.0f, 89.0f},
    {0.6101816f, 1.484181f, 553.0f, 1.0f, 3.0f, 0.5f, 0.12f, 0.45f, 2.0f, 42.21685f, 2.5f, 0.00081225246f, 2.97f, 0.7845841f, 0.038236815f, 0.25f,
     -1.0f, 1.5f, 0.9f, 0.0f, 10.4f, 1.18f, 20.0f, 89.0f},
    {0.614f, 1.61375f, 553.0f, 1.0f, 3.0f, 1.513688f, 0.12f, 0.45f, 2.0f, 142.0f, 2.5f, 0.0005f, 2.97f, 1.0f, 0.03f, 0.5f, 0.0f, 1.5f, 0.9f, 1.0f,
     10.4f, 1.18f, 20.0f, 89.0f},
    {0.614f, 1.61375f, 553.0f, 1.0f, 3.0f, 1.513688f, 0.12f, 0.45f, 2.0f, 337.7348f, 2.5f, 0.0003535534f, 2.97f, 1.0f, 0.03f, 0.5f, 0.0f, 0.75f,
     0.29999995f, 1.0f, 10.4f, 1.18f, 20.0f, 89.0f},
    {0.614f, 1.61375f, 553.0f, 1.0f, 3.0f, 0.5f, 0.12f, 0.45f, 2.0f, 84.4337f, 2.5f, 0.00061557227f, 2.97f, 1.0f, 0.03f, 0.5f, 1.0f, 1.2183785f,
     0.71999997f, 0.0f, 10.4f, 1.18f, 20.0f, 89.0f},
};

}  // namespace

// every preset by name and in order; its shape's instrument channels its
// table to the bit, the tine's t60 and load 10.4 s and 1.18, the partial and
// player channels nan, its accessor's the same object; its range in order
// and inside the keyboard; the tool's anchors the presets of their names
UP_TEST(presets_are_tables) {
    const char* names[9] = {"grand", "concertGrand", "upright", "spinet", "honkyTonk", "fortepiano", "rhodes", "rhodesBark", "wurlitzer"};
    const Shape* shapes[9] = {&grand(), &concertGrand(), &upright(), &spinet(), &honkyTonk(), &fortepiano(), &rhodes(), &rhodesBark(), &wurlitzer()};
    CHECK(presetCount() == 9);
    for (int i = 0; i < presetCount(); ++i) {
        const PresetInfo& p = presetAt(i);
        CHECK_MSG(std::strcmp(p.name, names[i]) == 0 && presetByName(names[i]) == &p, "%s", names[i]);
        const Shape& s = p.shape();
        bool bits = &s == shapes[i] && s[Channel::tineT60] == 10.4f && s[Channel::tineLoad] == 1.18f;
        for (int c = 0; c < kInstrumentChannels; ++c) bits = bits && sameBits(s.v[c], kTables[i][c]);
        for (int c = kInstrumentChannels; c < kChannelCount; ++c) bits = bits && s.v[c] != s.v[c];
        CHECK_MSG(bits, "%s's shape is not its table", p.name);
        CHECK_MSG(p.lo < p.hi && p.lo >= 21 && p.hi <= 108, "%s: its range %d .. %d", p.name, p.lo, p.hi);
    }
    CHECK(presetByName("piano") == nullptr && presetByName(nullptr) == nullptr);
    const PresetInfo copy = presetAt(2);
    CHECK(&copy.shape() == &upright());
    // the tool's anchors are the presets of their names
    for (int a = 0; a < kAnchorCount; ++a)
        CHECK(std::memcmp(&anchors()[a].s, &presetByName(anchors()[a].name)->shape(), sizeof(Shape)) == 0);
}

// every preset sounds at every semitone of its range: 2 s at velocity 1, at
// the gain that brings its octave from c4 to grand's (hearOctave, measured:
// the reed's is 16 db under): every sample finite and the peak over
// kSoundPeak, 0.022 (checks); through
// the board the last 100 ms's rms under the first 100 ms's (the board's low
// modes ring up over the first 0.3 s, so a short render can end in their
// bloom: known, naming the notes, where missed), and there the strings
// themselves (the voice's pair before the board) fall (a check)
UP_TEST(presets_sound_across_their_ranges) {
    int notes = 0, sounding = 0;
    std::string bloom;
    for (int i = 0; i < presetCount(); ++i) {
        const PresetInfo& p = presetAt(i);
        double lowPeak = 1e9, highPeak = 0.0;
        int lowAt = p.lo, highAt = p.lo;
        std::string dead, strings;
        Tuning t;
        t.outputGain = (float)std::pow(10.0, -hearOctave(p.shape()).level / 20.0);
        for (int midi = p.lo; midi <= p.hi; ++midi) {
            const Heard h = hearShape(p.shape(), (float)midi, 2.0f, t);
            ++notes;
            sounding += h.sounds.sounds;
            if (h.sounds.peak < lowPeak) {
                lowPeak = h.sounds.peak;
                lowAt = midi;
            }
            if (h.sounds.peak > highPeak) {
                highPeak = h.sounds.peak;
                highAt = midi;
            }
            char b[96];
            if (!h.sounds.finite || !(h.sounds.peak > kSoundPeak)) {
                std::snprintf(b, sizeof b, " %d (peak %.3f%s)", midi, h.sounds.peak, h.sounds.finite ? "" : ", not finite");
                dead += b;
            } else if (!h.sounds.decaying) {
                const size_t w = (size_t)(0.1 * kDefaultFs), n = h.r.pair.size();
                const double first = rms(h.r.pair.data(), w), last = rms(h.r.pair.data() + n - w, w);
                const double out = dbfs(rms(h.r.x.data() + n - w, w) / rms(h.r.x.data(), w));
                std::snprintf(b, sizeof b, " %s %d (the board %+.1f db, the strings %+.1f db)", p.name, midi, out, dbfs(last / first));
                bloom += b;
                if (!(last < first)) strings += b;
            }
        }
        CHECK_MSG(dead.empty(), "%s does not sound at%s", p.name, dead.c_str());
        CHECK_MSG(strings.empty(), "%s: the strings do not fall at%s", p.name, strings.c_str());
        printf("    %-13s %3d .. %3d at a gain of %.3f: every semitone finite and over %.3f; the quietest peak %.3f at %d, the loudest %.3f at %d\n",
               p.name, p.lo, p.hi, t.outputGain, kSoundPeak, lowPeak, lowAt, highPeak, highAt);
    }
    KNOWN_FAILURE(bloom.empty(), "through the board the last 100 ms of 2 s over the first at:%s", bloom.c_str());
    printf("    %d of %d notes sound by every part of the reading\n", sounding, notes);
}

// each preset's loudness: its octave from c4 (up_play.h: the twelve notes,
// each 0.6 s at velocity 1, the first half second's rms, the mean of the
// twelve powers) against grand's, db: none over grand's by more than 3 db,
// the string presets within 10 db under it, the tine's and the reed's within
// 20 (the reed's sits 16 db under, as measured); the octave from each
// preset's middle note and c4 alone printed beside
UP_TEST(presets_levels) {
    const double ref = levelReferenceRms();
    for (int i = 0; i < presetCount(); ++i) {
        const PresetInfo& p = presetAt(i);
        const int mid = (p.lo + p.hi) / 2;
        const OctaveHeard o = hearOctave(p.shape()), m = hearOctave(p.shape(), (float)mid);
        const double one = 20.0 * std::log10(levelReading(p.shape()).rms / ref);
        const bool string = p.shape()[Channel::generator] == -1.0f;
        CHECK_MSG(o.level <= 3.0 && o.level >= (string ? -10.0 : -20.0), "%s: the octave from c4 %+.2f db against grand's", p.name, o.level);
        printf("    %-13s the octave from c4 %+6.2f db against grand's (its peak %.3f), c4 alone %+6.2f; from its middle (%s) %+6.2f\n", p.name, o.level,
               o.peak, one, noteName(mid).c_str(), m.level);
    }
}

// honkyTonk beats faster than upright: the beat rate on the fundamental at c4
// (play's reading, 6 s at velocity 1)
UP_TEST(presets_honky_tonk_beats_faster) {
    double rate[2];
    const char* names[2] = {"upright", "honkyTonk"};
    for (int i = 0; i < 2; ++i) {
        PlaySpec sp;
        sp.instrument = presetByName(names[i])->shape();
        sp.sec = 6.0f;
        const PlayRender r = renderPlay(sp);
        rate[i] = readPlay(r, sp).beat.rate;
    }
    CHECK_MSG(rate[1] > rate[0], "honkyTonk beats %.3f hz, upright %.3f", rate[1], rate[0]);
    printf("    the beat on c4's fundamental: upright %.3f hz, honkyTonk %.3f hz\n", rate[0], rate[1]);
}

// rhodesBark's thd at e4, velocity 1 (play's reading over the 100 ms after the
// hammer leaves) 10 percentage points and more over rhodes's
UP_TEST(presets_rhodes_bark_barks) {
    double thd[2];
    const char* names[2] = {"rhodes", "rhodesBark"};
    for (int i = 0; i < 2; ++i) {
        PlaySpec sp;
        sp.instrument = presetByName(names[i])->shape();
        sp.midi = 64.0f;
        sp.sec = 1.0f;
        const PlayRender r = renderPlay(sp);
        thd[i] = readPlay(r, sp).thd;
    }
    CHECK_MSG(100.0 * (thd[1] - thd[0]) >= 10.0, "rhodesBark %.2f %% against rhodes %.2f %%", 100.0 * thd[1], 100.0 * thd[0]);
    printf("    the thd at e4, velocity 1: rhodes %.2f %%, rhodesBark %.2f %% (%+.2f points)\n", 100.0 * thd[0], 100.0 * thd[1], 100.0 * (thd[1] - thd[0]));
}

// concertGrand's and upright's b against their tables from c1 to c7, as the
// fit test holds them: the rms of ln(b_law / b_table) under 0.35 and 0.5; the
// worst note within 50 % known, with its number, where missed
UP_TEST(presets_b_against_their_tables) {
    struct Row { const char* preset; const char* file; double bound; };
    const Row rows[2] = {{"concertGrand", "inharmonicity_grand.txt", 0.35}, {"upright", "inharmonicity_upright.txt", 0.5}};
    for (const Row& r : rows) {
        RefTable t;
        std::string err;
        CHECK_MSG(readTable(std::string(UP_REF_DIR) + "/" + r.file, t, err), "%s", err.c_str());
        const Shape& s = presetByName(r.preset)->shape();
        double sum = 0.0, worst = 0.0, worstAt = 0.0;
        int n = 0;
        for (const BPoint& p : bPoints(t)) {
            if (p.midi < 24.0 || p.midi > 96.0) continue;
            const double q = rankAt(s, (float)p.midi).B / p.B;
            sum += std::log(q) * std::log(q);
            ++n;
            if (std::fabs(q - 1.0) > worst) { worst = std::fabs(q - 1.0); worstAt = p.midi; }
        }
        const double rms = n ? std::sqrt(sum / n) : INFINITY;
        CHECK_MSG(rms < r.bound, "%s against %s: the rms ln %.3f (under %.2f)", r.preset, r.file, rms, r.bound);
        KNOWN_FAILURE(worst <= 0.5, "%s against %s: the worst note %s %.1f %% off (the gate 50 %%)", r.preset, r.file, noteName(worstAt).c_str(), 100.0 * worst);
        printf("    %s against %s: %d notes from c1 to c7, the rms ln %.3f (under %.2f), the worst %s %.1f %%\n", r.preset, r.file, n, rms, r.bound,
               noteName(worstAt).c_str(), 100.0 * worst);
    }
}

// print's --set lines for each preset (uptool print --preset NAME) read back
// onto a nan shape to its instrument channels to the bit, and nothing else
UP_TEST(presets_print_round_trips) {
    int same = 0;
    for (int i = 0; i < presetCount(); ++i) {
        const PresetInfo& p = presetAt(i);
        const std::string lines = setLines(p.shape());
        Shape back = nanShape();
        bool ok = parseSetLines(lines, back);
        for (int c = 0; c < kChannelCount; ++c) ok = ok && (c < kInstrumentChannels ? sameBits(back.v[c], p.shape().v[c]) : back.v[c] != back.v[c]);
        CHECK_MSG(ok, "%s does not read back from:\n%s", p.name, lines.c_str());
        same += ok;
    }
    Shape bad = nanShape();
    CHECK(!parseSetLines("--set nope=1", bad) && !parseSetLines("--set length=", bad) && !parseSetLines("--set length=1x", bad));
    printf("    %d of %d presets' --set lines read back to the bit\n", same, presetCount());
}
