// upright — up_board.cpp
// the plate's table and the field's loss run once per body write and once
// per note (the input weights), never per sample: sqrtf, exp2f, sinf, powf
// and cosf are fine there; the board's render and the field's step are
// multiplies and adds, its shape() the app's call rate.
// the 40 x 40 search runs once, on the first boardOf; every later table is
// kBoardModes frequencies from the pairs it found.
#include "up_board.h"

#include <cmath>

namespace up {
namespace {

constexpr int kGrid = 40;                                  // m and n searched to here for the lowest modes

static_assert([] {
    int n = 0;
    for (int l : kFieldLengths) n += l;
    return n;
}() == kFieldSamples, "the field's lines fill its buffer");

inline float finiteOr(float x, float fallback) { return std::isfinite(x) ? x : fallback; }

// the lowest kBoardModes (m, n) in frequency order, the full search's at 1 m²: area-independent (up_board.h),
// filled on the first call and kept. zero-initialised statics, no constructor: no initialisation order to meet
struct ModePairs {
    int m[kBoardModes], n[kBoardModes];
};
const ModePairs& modePairs() {
    static ModePairs pairs;
    static bool found = false;
    if (!found) {
        const BoardTable t = boardSearch(1.0f, 0.03f, 0.5f);
        for (int k = 0; k < kBoardModes; ++k) {
            pairs.m[k] = t.mode[k].m;
            pairs.n[k] = t.mode[k].n;
        }
        found = true;
    }
    return pairs;
}

}  // namespace

float plateHz(int m, int n, float a, float b) {
    const float h3 = kBoardH * kBoardH * kBoardH;
    const float dx = kBoardEx * h3 / 12.0f, dy = kBoardEy * h3 / 12.0f;
    const float hh = std::sqrt(dx * dy);
    const float ma = (float)m / a, nb = (float)n / b;
    const float ma2 = ma * ma, nb2 = nb * nb;
    return 0.5f * kPi * std::sqrt(1.0f / (kBoardRho * kBoardH)) * std::sqrt(dx * ma2 * ma2 + 2.0f * hh * ma2 * nb2 + dy * nb2 * nb2);
}

float radiationEdge(float open) {
    return kFc0 * std::exp2(kFcSpan * (1.0f - clamp(finiteOr(open, 0.5f), 0.0f, 1.0f)));
}

float radiationOf(float hz, float open) {
    const float x = hz / radiationEdge(open);
    return x * x / (1.0f + x * x);
}

BoardTable boardOf(float areaM2, float loss, float open, const LawConstants&) {
    BoardTable t;
    const float area = finiteOr(areaM2, 1.0f) > 1e-3f ? finiteOr(areaM2, 1.0f) : 1e-3f;
    const float eta = finiteOr(loss, 0.03f) > 1e-5f ? finiteOr(loss, 0.03f) : 1e-5f;
    t.a = std::sqrt(area * kBoardAspect);
    t.b = std::sqrt(area / kBoardAspect);
    const ModePairs& p = modePairs();
    for (int k = 0; k < kBoardModes; ++k) {
        BoardMode& md = t.mode[k];
        md.m = p.m[k];
        md.n = p.n[k];
        md.hz = plateHz(md.m, md.n, t.a, t.b);
        md.t60 = 6.907755f / (kPi * md.hz * eta);
        md.radiation = radiationOf(md.hz, open);
    }
    return t;
}

BoardTable boardSearch(float areaM2, float loss, float open) {
    BoardTable t;
    const float area = finiteOr(areaM2, 1.0f) > 1e-3f ? finiteOr(areaM2, 1.0f) : 1e-3f;
    const float eta = finiteOr(loss, 0.03f) > 1e-5f ? finiteOr(loss, 0.03f) : 1e-5f;
    t.a = std::sqrt(area * kBoardAspect);
    t.b = std::sqrt(area / kBoardAspect);
    // the lowest kBoardModes of the grid: each pass takes the lowest not yet taken
    float last = -1.0f;
    int lastM = 0, lastN = 0;
    for (int k = 0; k < kBoardModes; ++k) {
        float best = 1e30f;
        int bm = 1, bn = 1;
        for (int m = 1; m <= kGrid; ++m)
            for (int n = 1; n <= kGrid; ++n) {
                const float f = plateHz(m, n, t.a, t.b);
                // after the last taken, in (hz, m, n) order
                const bool after = f > last || (f == last && (m > lastM || (m == lastM && n > lastN)));
                if (after && f < best) { best = f; bm = m; bn = n; }
            }
        BoardMode& md = t.mode[k];
        md.m = bm;
        md.n = bn;
        md.hz = best;
        md.t60 = 6.907755f / (kPi * best * eta);
        md.radiation = radiationOf(best, open);
        last = best;
        lastM = bm;
        lastN = bn;
    }
    return t;
}

void bridgePoint(float midi, const BoardTable& t, float& x, float& y) {
    const float u = clamp((finiteOr(midi, kScaleMidi) - 21.0f) / (108.0f - 21.0f), 0.0f, 1.0f);
    x = t.a * lerp(kBassX, kTrebleX, u);
    y = t.b * lerp(kBassY, kTrebleY, u);
}

float plateShape(int m, int n, float x, float y, const BoardTable& t) {
    return std::sin((float)m * kPi * x / t.a) * std::sin((float)n * kPi * y / t.b);
}

void inputWeights(const BoardTable& t, float x, float y, float* w) {
    for (int k = 0; k < kBoardModes; ++k) w[k] = plateShape(t.mode[k].m, t.mode[k].n, x, y, t);
}

void outputWeights(const BoardTable& t, float* w) {
    const float x = kListenX * t.a, y = kListenY * t.b;
    for (int k = 0; k < kBoardModes; ++k) w[k] = t.mode[k].radiation * plateShape(t.mode[k].m, t.mode[k].n, x, y, t);
}

// ---- the board ---------------------------------------------------------------------------------

void Board::init(float fs) {
    fs_ = fs > 1.0f ? fs : kDefaultFs;
    bank_.init(fs_, 1);
    area_ = loss_ = open_ = -1.0f;
    shelf_ = kBoardShelf;
    low1_ = low2_ = 0.0f;
    phase_ = 0;
    for (float& s : field_) s = 0.0f;
    for (int j = 0; j < kFieldLines; ++j) fieldAt_[j] = 0, fieldLow_[j] = 0.0f;
    setPlate(1.0f, 0.03f, 0.5f);
}

void Board::reset() {
    bank_.reset();
    low1_ = low2_ = 0.0f;
    phase_ = 0;
    for (float& s : field_) s = 0.0f;
    for (int j = 0; j < kFieldLines; ++j) fieldAt_[j] = 0, fieldLow_[j] = 0.0f;
}

void Board::setPlate(float areaM2, float loss, float open) {
    if (areaM2 == area_ && loss == loss_ && open == open_) return;
    area_ = areaM2;
    loss_ = loss;
    open_ = open;
    table_ = boardOf(areaM2, loss, open);
    float out[kBoardModes];
    outputWeights(table_, out);
    const float mass = 0.25f * kBoardRho * kBoardH * table_.a * table_.b;   // kg: the plate's modal mass
    const float dt = 1.0f / fs_;
    for (int k = 0; k < kBoardModes; ++k) {
        const BoardMode& m = table_.mode[k];
        const float omega = 2.0f * kPi * m.hz;
        const bool on = m.hz > 0.0f && m.hz < kCeiling * fs_;
        bank_.setPartial(k, on, m.hz, 6.907755f / m.t60, dt / (mass * omega) * 1e6f, 0.0f, out[k]);
    }
    shelfA_ = 1.0f - std::exp(-2.0f * kPi * table_.mode[kBoardModes - 1].hz * dt);   // the shelf's poles at the top mode
    // the field's loss, a line of L samples: g, the one-pole's gain at dc, a pass at the top mode's t60 (60 db in
    // t60); r, its |H| / g at kFieldFitHz, a pass at the plate's t60 there (t60 goes as 1 / f) over g. (1 - a)² =
    // r² (1 - 2 a cos w + a²) has its root in [0, 1) at q / (p + sqrt(p² - q²)), p = 1 - r² cos w, q = 1 - r²;
    // a = 0 where r >= 1
    const float t60 = table_.mode[kBoardModes - 1].t60, t60Fit = t60 * table_.mode[kBoardModes - 1].hz / kFieldFitHz;
    const float c = std::cos(2.0f * kPi * kFieldFitHz * dt);
    for (int j = 0; j < kFieldLines; ++j) {
        const float pass = -3.0f * (float)kFieldLengths[j] * dt;
        const float g = std::pow(10.0f, pass / t60), r = std::pow(10.0f, pass / t60Fit) / g;
        const float r2 = r * r, q = 1.0f - r2;
        fieldG_[j] = g;
        fieldA_[j] = q > 0.0f ? q / (1.0f - r2 * c + std::sqrt(r2 * (1.0f - c) * (2.0f - r2 * (1.0f + c)))) : 0.0f;
    }
    fieldGain_ = kFieldGain / kBoardShelf;
}

void Board::inputWeights(float midi, float* w) const {
    float x, y;
    bridgePoint(midi, table_, x, y);
    up::inputWeights(table_, x, y, w);
}

void Board::render(const float* const* forces, const float* const* w, int voices, float* out, int n) {
    const float direct = shelf_ * kFieldDirect;
    for (int i = 0; i < n; ++i) {
        if (phase_ == 0) bank_.beginBlock();
        float f = 0.0f;
        for (int v = 0; v < voices; ++v) {
            bank_.injectWeighted(forces[v][i], w[v]);
            f += forces[v][i];
        }
        float sum, x, xd;
        bank_.step<true, false, true>(sum, x, xd);
        // the shelf: the summed force less its one-pole lowpass, twice, into the direct share and the field
        low1_ += shelfA_ * (f - low1_);
        const float high1 = f - low1_;
        low2_ += shelfA_ * (high1 - low2_);
        const float high2 = high1 - low2_;
        out[i] = (sum + direct * high2 + fieldStep(high2)) * kBoardOut;
        if (++phase_ == kBlock) {
            phase_ = 0;
            bank_.endBlock();
        }
    }
}

float Board::fieldStep(float force) {
    // the lines' outputs, each line at its own offset in the buffer
    float d[kFieldLines], sum = 0.0f;
    for (int j = 0, at = 0; j < kFieldLines; at += kFieldLengths[j], ++j) {
        d[j] = field_[at + fieldAt_[j]];
        sum += d[j];
    }
    // householder: each output less 2 / N of their sum, through its loss, back in with the force; out the signed sum
    const float mix = (2.0f / (float)kFieldLines) * sum;
    float o = 0.0f;
    for (int j = 0, at = 0; j < kFieldLines; at += kFieldLengths[j], ++j) {
        fieldLow_[j] += (1.0f - fieldA_[j]) * (fieldG_[j] * (d[j] - mix) - fieldLow_[j]);
        field_[at + fieldAt_[j]] = force + fieldLow_[j];
        fieldAt_[j] = fieldAt_[j] + 1 == kFieldLengths[j] ? 0 : fieldAt_[j] + 1;
        o += (j & 1) ? -d[j] : d[j];
    }
    return o * shelf_ * fieldGain_;
}

void Board::fieldLine(int i, float& g, float& a) const {
    const int j = i < 0 ? 0 : (i >= kFieldLines ? kFieldLines - 1 : i);
    g = fieldG_[j];
    a = fieldA_[j];
}

float Board::shape(float x, float y) const {
    float sum = 0.0f;
    for (int k = 0; k < kBoardModes; ++k) {
        float re, im;
        bank_.phasor(k, re, im);
        if (re != 0.0f) sum += re * plateShape(table_.mode[k].m, table_.mode[k].n, x, y, table_);
    }
    return sum;
}

}  // namespace up
