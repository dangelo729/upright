// upright — up_knobs.cpp  (plan §2)
// the recipes run once per write, never per sample: exp2f, log2f and sqrtf
// are fine here.
#include "up_knobs.h"

#include <cmath>
#include <cstring>

namespace up {
namespace {

// ---- the constants ---------------------------------------------------------------------------------
// each with what sets it: the plan's figure, a source, or the chart (plans/upright-engine/out0). the
// scale's (l0, scale, bass0, scaleBass), the medium hammer's (k, p, massC4), the tension and the
// string's loss are the law's block (LawConstants: the fit moves them)

// hammer
constexpr float kHard = 2.5f;                // hardness = k 2^(h 2.5): 25 .. 803 N/mm^p at c4, chaigne & askenfelt's c2 hammer to their
                                             //   c6 hammer; the felt's power is the rank's (p at c4, rankP along the keyboard)
constexpr float kLambda = 5e-4f;             // s/mm (0.5 s/m): the felt's hunt & crossley loss (an estimate)
constexpr float kLamSpan = 1.0f;             // a soft felt loses twice the medium's, a hard one half
// strike: shares of the length from the agraffe at c4
constexpr float kStrikeEnd = 0.04f;
constexpr float kStrikeEighth = 0.12f;       // real pianos a little under an eighth (fletcher & rossing, by memory)
constexpr float kStrikeMid = 0.5f;
// unison: cents
constexpr float kDetune = 0.5f;              // a tuned unison: under the lock's critical detune at every note; tuners set a
                                             //   clean unison inside half a cent
constexpr float kDetuneWide = 20.0f;         // a honky-tonk at +1 (an estimate)
// body
constexpr float kArea0 = 1.0f;               // m²: 0.6 to 1.6 m² over the knob (by memory for an upright's and a grand's board)
constexpr float kAreaSpan = 0.7f;
constexpr float kEta = 0.03f;                // spruce's loss factor, 0.049 damped to 0.018 lively (by memory)
constexpr float kEtaSpan = 0.7f;
// pickup: mm
constexpr float kGap0 = 1.5f;                // 3 mm far to 0.75 close (falaize & helie's 0.5, 1.5, 3 mm)
constexpr float kGapSpan = 1.0f;
constexpr float kOffFar = 1.5f;              // pure tone to the realistic offset (falaize & helie's 0, 0.5, 1.5 mm)
constexpr float kOffClose = 0.3f;
// damper
constexpr float kDamp = 20.0f;               // 1/s at c4: 60 at -1, 20 at 0, 6 at +1 (t60 0.12, 0.35, 1.2 s; an estimate)
constexpr float kDampSpan = 1.6f;
constexpr float kDamperTop = 89.0f;          // f6: the top octave and a half has no dampers (by memory)

const char* const kNames[kKnobCount] = {"scale", "hammer", "strike", "unison", "body", "generator", "pickup", "damper"};
const char* const kEnds[kKnobCount][3] = {
    {"spinet", "grand", "concert"}, {"soft", "medium", "hard"},  {"end", "eighth", "middle"},  {"one", "three", "wide"},
    {"upright", "parlor", "concert"}, {"string", "tine", "reed"}, {"far", "set", "close"},     {"heavy", "felt", "light"},
};

inline bool isNan(float x) { return x != x; }
inline float held(float x) { return isNan(x) ? 0.0f : clamp(x, -1.0f, 1.0f); }

// a value held inside its channel's span
inline float inRange(Channel c, float x) {
    float lo, hi;
    channelRange(c, lo, hi);
    return clamp(x, lo, hi);
}

inline float shareOf(float st) {
    return st < 0.0f ? lerp(kStrikeEighth, kStrikeEnd, -st) : lerp(kStrikeEighth, kStrikeMid, st);
}

// the level table: at each of the generator's four anchors, each knob's row
// of its measured octave level over its eleven points, db against the
// anchor's own level (the string's is the reference, grand's octave c4 .. b4)
#include "up_level_table.inc"

// a row read where x sits on it: linear between its points, held past either end
inline float rowAt(const float* row, float x) {
    const float u = (held(x) + 1.0f) * 0.5f * (float)(kLevelPoints - 1);
    int i = (int)u;
    if (i > kLevelPoints - 2) i = kLevelPoints - 2;
    return row[i] + (row[i + 1] - row[i]) * (u - (float)i);
}

}  // namespace

const char* knobName(Knob k) {
    const int i = (int)k;
    return i >= 0 && i < kKnobCount ? kNames[i] : "";
}

bool knobByName(const char* name, Knob& out) {
    if (!name) return false;
    for (int i = 0; i < kKnobCount; ++i) {
        if (std::strcmp(name, kNames[i]) == 0) { out = (Knob)i; return true; }
    }
    return false;
}

const char* knobEnd(Knob k, int which) {
    const int i = (int)k;
    return i >= 0 && i < kKnobCount && which >= 0 && which < 3 ? kEnds[i][which] : "";
}

Shape pianoOf(const Knobs& k, const Tuning& t) { return pianoOf(k, t, lawConstants()); }

Shape pianoOf(const Knobs& kn, const Tuning&, const LawConstants& c) {
    const float s = held(kn.v[(int)Knob::scale]), h = held(kn.v[(int)Knob::hammer]);
    const float st = held(kn.v[(int)Knob::strike]), u = held(kn.v[(int)Knob::unison]);
    const float b = held(kn.v[(int)Knob::body]), g = held(kn.v[(int)Knob::generator]);
    const float p = held(kn.v[(int)Knob::pickup]), d = held(kn.v[(int)Knob::damper]);
    Shape sh;
    for (int i = 0; i < kChannelCount; ++i) sh.v[i] = std::nanf("");

    // the strings
    sh[Channel::length] = c.l0 * std::exp2(s * c.scale);
    sh[Channel::bass] = c.bass0 * std::exp2(s * c.scaleBass);
    sh[Channel::tension] = c.tension;
    sh[Channel::gauge] = 1.0f;
    sh[Channel::strings] = 1.0f + 2.0f * clamp(u + 1.0f, 0.0f, 1.0f);
    const float wide = clamp(u, 0.0f, 1.0f);
    sh[Channel::detune] = lerp(kDetune, kDetuneWide, wide * wide);
    sh[Channel::strikeShare] = shareOf(st);
    sh[Channel::lossRate] = c.loss;
    sh[Channel::lossSlope] = c.lossSlope;
    // the hammer
    sh[Channel::hardness] = c.k * std::exp2(h * kHard);
    sh[Channel::exponent] = c.p;
    sh[Channel::felt] = kLambda * std::exp2(-h * kLamSpan);
    sh[Channel::hammerMass] = c.massC4;
    // the body
    sh[Channel::boardArea] = kArea0 * std::exp2(b * kAreaSpan);
    sh[Channel::boardLoss] = kEta * std::exp2(-b * kEtaSpan);
    sh[Channel::open] = (b + 1.0f) * 0.5f;
    // the generator and the pickup
    sh[Channel::generator] = g;
    sh[Channel::tonebar] = 1.0f - std::fabs(g);
    sh[Channel::gap] = kGap0 * std::exp2(-p * kGapSpan);
    sh[Channel::offset] = lerp(kOffFar, kOffClose, (p + 1.0f) * 0.5f);
    // the damper
    sh[Channel::dampRate] = kDamp * std::exp2(-d * kDampSpan);
    sh[Channel::damperTop] = kDamperTop;

    for (int i = 0; i < kInstrumentChannels; ++i) sh.v[i] = inRange((Channel)i, sh.v[i]);
    return sh;
}

// the read-back: each recipe inverted. a knob that writes several channels
// reads the mean of their reads (scale: length and bass; hammer: hardness
// and felt; body: area, loss and open; pickup: gap and offset);
// strike through its two segments; unison from the strings under 3 and the
// detune otherwise; generator as written. so an instrument set by hand
// reads its nearest point
Knobs knobsOf(const Shape& sh) {
    const LawConstants& c = lawConstants();
    Knobs k;
    const float s1 = std::log2(sh[Channel::length] / c.l0) / c.scale;
    const float s2 = std::log2(sh[Channel::bass] / c.bass0) / c.scaleBass;
    k.v[(int)Knob::scale] = 0.5f * (s1 + s2);

    const float h1 = std::log2(sh[Channel::hardness] / c.k) / kHard;
    const float h2 = -std::log2(sh[Channel::felt] / kLambda) / kLamSpan;
    k.v[(int)Knob::hammer] = 0.5f * (h1 + h2);

    const float share = sh[Channel::strikeShare];
    k.v[(int)Knob::strike] = share < kStrikeEighth ? -(kStrikeEighth - share) / (kStrikeEighth - kStrikeEnd)
                                                   : (share - kStrikeEighth) / (kStrikeMid - kStrikeEighth);

    const float strings = sh[Channel::strings];
    if (strings < 3.0f) {
        k.v[(int)Knob::unison] = (strings - 1.0f) * 0.5f - 1.0f;
    } else {
        const float w = (sh[Channel::detune] - kDetune) / (kDetuneWide - kDetune);
        k.v[(int)Knob::unison] = std::sqrt(w > 0.0f ? w : 0.0f);
    }

    const float b1 = std::log2(sh[Channel::boardArea] / kArea0) / kAreaSpan;
    const float b2 = -std::log2(sh[Channel::boardLoss] / kEta) / kEtaSpan;
    const float b3 = 2.0f * sh[Channel::open] - 1.0f;
    k.v[(int)Knob::body] = (b1 + b2 + b3) * (1.0f / 3.0f);

    k.v[(int)Knob::generator] = sh[Channel::generator];

    const float p1 = -std::log2(sh[Channel::gap] / kGap0) / kGapSpan;
    const float p2 = 2.0f * (sh[Channel::offset] - kOffFar) / (kOffClose - kOffFar) - 1.0f;
    k.v[(int)Knob::pickup] = 0.5f * (p1 + p2);

    k.v[(int)Knob::damper] = -std::log2(sh[Channel::dampRate] / kDamp) / kDampSpan;

    for (int i = 0; i < kKnobCount; ++i) k.v[i] = held(k.v[i]);
    return k;
}

// the generator's row at the string (against the reference), then the other
// seven knobs' rows at the two anchors about the generator, each summed, the
// two sums linear in the generator between the anchors: db against the
// reference, for a caller to undo with a gain of 10^(-db / 20)
float levelDb(const Knobs& kn) {
    const float g = held(kn.v[(int)Knob::generator]);
    int a = 0;
    while (a < kLevelAnchors - 2 && g > kLevelAnchor[a + 1]) ++a;
    const float t = (g - kLevelAnchor[a]) / (kLevelAnchor[a + 1] - kLevelAnchor[a]);
    float lo = 0.0f, hi = 0.0f;
    for (int k = 0; k < kKnobCount; ++k) {
        if (k == (int)Knob::generator) continue;
        lo += rowAt(kLevelRows[a][k], kn.v[k]);
        hi += rowAt(kLevelRows[a + 1][k], kn.v[k]);
    }
    return rowAt(kLevelRows[0][(int)Knob::generator], g) + lo + (hi - lo) * t;
}

}  // namespace up
