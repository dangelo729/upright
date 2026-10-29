// upright — up_law.cpp  (plan §3, §4 the tip, §9 the damper's rate)
// the law runs once per instrument write and once per note, never per
// sample: exp2f, logf, powf, sinf, coshf are fine here.
#include "up_law.h"

#include <cmath>

#include "up_rank.h"

namespace up {
namespace {

// ---- the constants ---------------------------------------------------------------------------------
// each with what sets it: the plan's figure, a source, or the fit (uptool fit --constants,
// plans/upright-engine/out0/fit)
constexpr LawConstants kLaw = {
    553.0f,     // tension: the design tension, about the same the keyboard over (the measured b tables' fit with kScale
                //   held at 0.03, plans/upright-engine/out1/planner/scale_fit_003.txt)
    0.915f,     // rankLength: the length goes as (f_c4 / f0)^0.915 until the case stops it (the same fit)
    1.08875e-3f,  // dMax: the thickest plain wire (the same fit)
    1.0e-3f,    // core: a wound string's steel core at the bass break (the same fit)
    0.45f,      // loss: the c4 fundamental's t60 14.4 s, one string (an estimate)
    800.0f,     // lossKnee: hz, where the wire's own loss (not scaled by the rank) equals lossRate. against chaigne 2016's
                //   measured strings (decay_grand.txt block 1: d#1 8.3 s, c4 4.84 s, e6 1.38 s at 1 khz or the fundamental)
                //   it reads 7.8, 6.0, 2.4 s; c4's 8th partial 1.9 s, c8's fundamental 0.42 s
    2.0f,       // lossSlope (an estimate)
    0.8f,       // rankLoss: the bass rings tens of seconds, the top under two (an estimate)
    0.25f,      // rankMass: chaigne & askenfelt's hammers, 4.9 g at c2, 2.97 at c4, 2.2 at c6
    0.03f,      // rankStrike: the share falls 0.12 to 0.115 over the keyboard (chabassier & durufle's steinway d holds
                //   0.120 from c1 to c6; chaigne 2016's eight strings 0.105 .. 0.125)
    0.614f,     // l0: a grand's c4 speaking length (a steinway d's 0.657; the same fit)
    0.03f,      // scale: 2^±0.03 on c4's length: a concert grand's and an upright's c4 measure the same b (3.16e-4,
                //   3.41e-4); the size lives in the bass, which scaleBass carries
    1.61375f,   // bass0: the longest string, 1.22 m at -1 and 2.13 at +1 ...
    0.4025f,    // scaleBass: ... a spinet's and a concert grand's a0 (the same fit)
    2.97f,      // massC4: g, chaigne & askenfelt's c4 hammer (measured with its k and p)
    142.0f,     // k: N/mm^p (the force at 1 mm), chaigne & askenfelt's c4: 4.5e9 N/m^2.5
    2.5f,       // p: chaigne & askenfelt's c4 (the knob leaves it; rankP moves it along the keyboard)
    10.4f,      // tineT60: s at c4, the fit against shear's q for a 1974 mark i (eb2 26.8 s .. eb6 3.1 s at the rank's
                //   t60 slope)
    1.2f,       // tineSlope: the overtones' ping (an estimate)
    0.75f,      // rankHardLo: the felt's stiffness below c4 (chaigne & askenfelt's k at 1 mm: 50 at c2 against 142 at c4)
    1.41f,      // rankHardHi: the felt's stiffness from c4 up (their 1000 at c6): their three points bend at c4
    0.17f,      // rankP: the felt's power up the keyboard (chaigne & askenfelt's p: 2.3, 2.5, 3.0 at c2, c4, c6)
    0.1725f,    // coreGrow: the core thickens as the winding grows, so b rises toward a0 (chabassier & durufle's c1 core
                //   1.48 mm; every measured piano's b rises 2 .. 9x from the break to a0; the same fit)
    2.5f,       // t60ScaleMax: a0's fundamental 37 s, not 93 (the rank's power alone)
    3.5e-4f,    // bridgeMobility: m/s per N, the bridge's driving-point mobility; giordano's soundboard impedance 1 .. 3 x 10³
                //   kg/s puts it 3e-4 .. 1e-3, the bridge stiffer than the board; an estimate: grand's c4 string loses 0.315 /s
                //   into it, the plan's 0.45 floor minus its internal loss
    0.5f,       // bridgeReactive: an estimate: enough that the prompt sound and the aftersound have turned 1.3 rad against
                //   each other where they cross at c4, so no null; a 45 degree bridge would be 1
};

// the clamped-free beam's beta_n L for n = 1 .. 6; from n = 7 the asymptote (2n - 1) pi / 2
constexpr double kBeta[6] = {1.875104069, 4.694091133, 7.854757438, 10.995540735, 14.137168391, 17.278759532};

inline float finiteOr(float x, float fallback) { return std::isfinite(x) ? x : fallback; }
inline float sign(int n) { return (n & 1) ? -1.0f : 1.0f; }   // (-1)^n

// a to b along the geometric mean's path, landing on each exactly at its end
inline float logLerp(float a, float b, float u) {
    if (u <= 0.0f) return a;
    if (u >= 1.0f) return b;
    return std::exp(lerp(std::log(a), std::log(b), u));
}
// a to b linearly, landing on each exactly at its end
inline float lineLerp(float a, float b, float u) { return u <= 0.0f ? a : (u >= 1.0f ? b : lerp(a, b, u)); }

}  // namespace

const LawConstants& lawConstants() { return kLaw; }

float stringRatio(int n, float B) {
    const float nf = (float)n;
    return nf * std::sqrt(1.0f + (B > 0.0f ? B : 0.0f) * nf * nf);
}

// the floor scales with the rank and the wire's loss does not: the table's rate is lossRate (1 + t60Scale (f_n /
// knee)^slope), so the voice's x t60Scale lands lossRate / t60Scale + lossRate (f_n / knee)^slope
float stringT60(int, float ratio, float f0, const Shape& s, const LawConstants& c, float t60Scale) {
    const float rate = finiteOr(s[Channel::lossRate], c.loss);
    const float slope = finiteOr(s[Channel::lossSlope], c.lossSlope);
    const float scale = finiteOr(t60Scale, 1.0f) > 0.0f ? finiteOr(t60Scale, 1.0f) : 1.0f;
    const float fn = ratio * f0;
    const float knee = c.lossKnee > 1.0f ? c.lossKnee : 1.0f;
    const float sigma = (rate > 1e-4f ? rate : 1e-4f) * (1.0f + scale * std::pow(fn / knee, slope > 0.0f ? slope : 0.0f));
    return 6.907755f / sigma;
}

float strikeGain(int n, float share) { return std::sin((float)n * kPi * share); }

float bridgeOut(int n) { return (float)n * sign(n + 1); }

float beamBeta(int n) {
    if (n < 1) n = 1;
    return n <= 6 ? (float)kBeta[n - 1] : (float)((2.0 * n - 1.0) * 3.14159265358979323846 * 0.5);
}

float tineRatio(int n, const LawConstants&) {
    const double b = n <= 6 ? kBeta[(n < 1 ? 1 : n) - 1] : (2.0 * n - 1.0) * 3.14159265358979323846 * 0.5;
    const double load = n >= 2 ? (double)kTineLoad : 1.0;
    return (float)((b / kBeta[0]) * (b / kBeta[0]) * load);
}

// phi(x) = cosh x - cos x - s (sinh x - sin x), x = beta xi, s = (sinh b - sin b) / (cosh b + cos b),
// written as ((1 - s) / 2) e^x + ((1 + s) / 2) e^-x - cos x + s sin x with 1 - s from e^-b, so a high
// mode's shape stays exact where cosh and sinh would cancel
float beamShape(int n, float share) {
    const double b = n <= 6 ? kBeta[(n < 1 ? 1 : n) - 1] : (2.0 * n - 1.0) * 3.14159265358979323846 * 0.5;
    const double xi = share < 0.0f ? 0.0 : (share > 1.0f ? 1.0 : (double)share);
    const double x = b * xi;
    const double r = std::exp(-b), cb = std::cos(b), sb = std::sin(b);
    const double den = 1.0 + r * r + 2.0 * r * cb;                  // (cosh b + cos b) 2 e^-b
    const double oneMinusS = 2.0 * r * (r + cb + sb) / den;          // 1 - s
    const double s = 1.0 - oneMinusS;
    const double grow = r * (r + cb + sb) / den * std::exp(x);       // ((1 - s) / 2) e^x, from e^(x - b)
    return (float)(grow + 0.5 * (1.0 + s) * std::exp(-x) - std::cos(x) + s * std::sin(x));
}

float stringsOf(const Shape& s) {
    const float g = clamp(finiteOr(s[Channel::generator], -1.0f), -1.0f, 1.0f);
    const float cap = g <= -0.5f ? 3.0f : (g < 0.0f ? 2.0f - 2.0f * g : 2.0f);
    const float n = finiteOr(s[Channel::strings], 3.0f);
    return n < cap ? n : cap;
}

// the note is the shape's midi channel (c4 when nan): over damperTop no damper
float dampRateOf(float hz, const Shape& s, float pedal, const LawConstants&) {
    const float note = finiteOr(s[Channel::midi], kScaleMidi);
    if (note > finiteOr(s[Channel::damperTop], 89.0f)) return 0.0f;
    const float rate = finiteOr(s[Channel::dampRate], 0.0f);
    const float held = 1.0f - clamp(finiteOr(pedal, 0.0f), 0.0f, 1.0f);
    return (rate > 0.0f ? rate : 0.0f) * (1.0f + (hz > 0.0f ? hz : 0.0f) / kDampKnee) * held;
}

float hammerHardnessAt(float hardness, float generator) {
    const float g = clamp(finiteOr(generator, -1.0f), -1.0f, 1.0f);
    return hardness * std::exp2(kTipHard * (1.0f - std::fabs(g)));
}

float hammerFeltAt(float felt, float generator) {
    const float g = clamp(finiteOr(generator, -1.0f), -1.0f, 1.0f);
    return felt * std::exp2(-kTipLoss * (1.0f - std::fabs(g)));
}

float hammerExponentAt(float exponent, float pAdd) {
    return clamp(finiteOr(exponent, 2.5f) + finiteOr(pAdd, 0.0f), kExponentLo, kExponentHi);
}

PartialTable partialsOf(const Shape& s, const RankState& r, const Tuning& t) { return partialsOf(s, r, t, kLaw); }

PartialTable partialsOf(const Shape& s, const RankState& r, const Tuning& t, const LawConstants& c) {
    const float g = clamp(finiteOr(s[Channel::generator], -1.0f), -1.0f, 1.0f);
    const float share = finiteOr(r.strikeShare, 0.12f);
    const float f0 = finiteOr(r.f0, midiToHz(kScaleMidi));
    const float ceilingHz = clamp(finiteOr(t.ceiling, kCeiling), 0.01f, 0.5f) * kDefaultFs;
    const float tineT60 = c.tineT60 > 1e-3f ? c.tineT60 : 1e-3f;
    const float fade = clamp(-g, 0.0f, 1.0f);                  // the string's slots over the tine's
    PartialTable p;
    for (int k = 0; k < kMaxModes; ++k) {
        const int n = k + 1;
        // the string
        const float sr = stringRatio(n, r.B);
        const float st = stringT60(n, sr, f0, s, c, r.t60Scale);
        const float sg = strikeGain(n, share), so = bridgeOut(n);
        // the beam, its shapes over the tip's 2
        const float br = tineRatio(n, c);
        const float bt = tineT60 * std::pow(br, -c.tineSlope);
        const bool beamOn = n <= kTineModes;
        const float bg = beamOn ? 0.5f * beamShape(n, share) : 0.0f;
        const float bo = beamOn ? sign(n + 1) : 0.0f;
        float ratio, t60, gain, out;
        if (g <= 0.0f) {
            const float u = g + 1.0f;                            // 0 the string, 1 the tine
            ratio = logLerp(sr, br, u);
            t60 = logLerp(st, bt, u);
            if (beamOn) {
                gain = lineLerp(sg, bg, u);
                out = lineLerp(so, bo, u);
            } else {
                gain = sg * fade;
                out = so * fade;
            }
        } else {
            ratio = br;
            t60 = logLerp(bt, bt * kReedT60, g);                // toward the reed's
            gain = n == 1 ? bg : bg * lineLerp(1.0f, kReedOvertone, g);
            out = bo;
        }
        if (ratio * f0 > ceilingHz) gain = out = 0.0f;
        p.ratio[k] = ratio;
        p.t60[k] = t60;
        p.gain[k] = gain + 0.0f;                                 // no negative zero
        p.out[k] = out + 0.0f;
    }
    return p;
}

bool Law::pending(const Channels& ch) const {
    if (fresh_) return true;
    for (int i = 0; i < kInstrumentChannels; ++i)
        if (ch.seq((Channel)i) > seq_) return true;
    return false;
}

bool Law::update(Channels& ch, const RankState& r, bool noteMoved, const Tuning& t) {
    uint32_t newest = 0;
    for (int i = 0; i < kInstrumentChannels; ++i) {
        const uint32_t q = ch.seq((Channel)i);
        if (q > newest) newest = q;
    }
    if (!fresh_ && newest == seq_ && !noteMoved) return false;
    const PartialTable p = partialsOf(ch.shape(), r, t);
    for (int k = 0; k < kMaxModes; ++k) {
        ch.write(ratioChannel(k), p.ratio[k], newest);
        ch.write(t60Channel(k), p.t60[k], newest);
        ch.write(gainChannel(k), p.gain[k], newest);
        ch.write(outChannel(k), p.out[k], newest);
    }
    seq_ = newest;
    fresh_ = false;
    return true;
}

}  // namespace up
