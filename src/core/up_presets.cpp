// upright — up_presets.cpp  (plan §12)
// each preset's shape is built once, on first use, from its table.
#include "up_presets.h"

#include <cstring>

namespace up {
namespace {

// the tables: the twenty-four instrument channels in the enum's order, a line a group. each with where it came
// from: the fit's residual (uptool fit --table) or by hand, heard (plans/upright-engine/out2/listen)
typedef float Table[kInstrumentChannels];

// the reference. the strings: a grand's c4 speaking length and its longest string, at the design tension (the measured
// b tables' fit, plans/upright-engine/out1/planner/scale_fit_003.txt); three strings tuned inside half a cent (under
// the lock's critical detune at every note); the strike at 0.12 of the length, a little under an eighth (fletcher &
// rossing, by memory); the c4 fundamental's t60 14.4 s, one string, the loss's power 2 (estimates). the hammer:
// chaigne & askenfelt's c4 (4.5e9 N/m^2.5, 2.97 g), the felt's hunt & crossley loss 0.5 s/m (an estimate). the body: a
// 1 m² spruce board, its loss 0.03 (by memory), half open. the string (generator -1); the pickup's gap falaize &
// helie's 1.5 mm, its offset 0.9 mm, and the tine's t60 and load rhodes's, all inert on the string. the damper 20 /s at
// c4 (t60 0.35 s, an estimate) up to f6 (the top octave and a half has none, by memory)
constexpr Table kGrand = {
    0.614f, 1.61375f, 553.0f, 1.0f, 3.0f, 0.5f, 0.12f, 0.45f, 2.0f,
    142.0f, 2.5f, 0.0005f, 2.97f,
    1.0f, 0.03f, 0.5f,
    -1.0f, 1.5f, 0.9f, 0.0f, 10.4f, 1.18f,
    20.0f, 89.0f,
};
// fit --table inharmonicity_grand.txt: length and bass, rms ln 0.191 over the steinway d's 77 keys; the board (1.62 m²,
// loss 0.0185, open) and the detune (1.28 cents) by hand, heard
constexpr Table kConcertGrand = {
    0.6264942f, 2.1145327f, 553.0f, 1.0f, 3.0f, 1.2800001f, 0.12f, 0.45f, 2.0f,
    142.0f, 2.5f, 0.0005f, 2.97f,
    1.6245048f, 0.018467166f, 1.0f,
    -1.0f, 1.5f, 0.9f, 0.0f, 10.4f, 1.18f,
    20.0f, 89.0f,
};
// fit --table inharmonicity_upright.txt: length and bass, rms ln 0.301 over its 97 keys; the board (0.71 m², loss
// 0.042, nearly closed) and a softer hammer (hardness 100, felt 5.7e-4) by hand, heard
constexpr Table kUpright = {
    0.6082418f, 1.4221112f, 553.0f, 1.0f, 3.0f, 0.5f, 0.12f, 0.45f, 2.0f,
    100.409164f, 2.5f, 0.0005743492f, 2.97f,
    0.7120251f, 0.042133342f, 0.15f,
    -1.0f, 1.5f, 0.9f, 0.0f, 10.4f, 1.18f,
    20.0f, 89.0f,
};
// by hand (the shortest strings, the smallest board, closed, a harder hammer), heard
constexpr Table kSpinet = {
    0.6013641f, 1.2208763f, 553.0f, 1.0f, 3.0f, 0.5f, 0.12f, 0.45f, 2.0f,
    238.81459f, 2.5f, 0.00040612623f, 2.97f,
    0.6155722f, 0.04873514f, 0.0f,
    -1.0f, 1.5f, 0.9f, 0.0f, 10.4f, 1.18f,
    20.0f, 89.0f,
};
// upright with its unison 20 cents wide and its hammer soft (hardness 60, felt 7.1e-4) (bank's recipe: detuned,
// inharmonic, soft), heard
constexpr Table kHonkyTonk = {
    0.6082418f, 1.4221112f, 553.0f, 1.0f, 3.0f, 20.0f, 0.12f, 0.45f, 2.0f,
    59.703648f, 2.5f, 0.0007071068f, 2.97f,
    0.7120251f, 0.042133342f, 0.15f,
    -1.0f, 1.5f, 0.9f, 0.0f, 10.4f, 1.18f,
    20.0f, 89.0f,
};
// by hand (strings a little shorter, soft hammers: hardness 42, felt 8.1e-4; a small board: 0.78 m², loss 0.038,
// open 0.25; the gauge 1), heard
constexpr Table kFortepiano = {
    0.6101816f, 1.484181f, 553.0f, 1.0f, 3.0f, 0.5f, 0.12f, 0.45f, 2.0f,
    42.21685f, 2.5f, 0.00081225246f, 2.97f,
    0.7845841f, 0.038236815f, 0.25f,
    -1.0f, 1.5f, 0.9f, 0.0f, 10.4f, 1.18f,
    20.0f, 89.0f,
};
// by hand (the tine, the tonebar beside it, the pickup set: gap 1.5 mm, offset 0.9 mm), heard; the detune 1.51 cents:
// the tonebar's beat as heard at 1.5 (the strings capped at 2 on the tine side). the tine's t60 10.4 s at c4: the fit
// against shear's q for a 1974 mark i (eb2 26.8 s .. eb6 3.1 s at the rank's t60 slope); its load 1.18: the tuning
// spring's mass at the tip (shear 2011, fig. 2.2: the 2nd partial at 7.4, the bare beam's 6.27)
constexpr Table kRhodes = {
    0.614f, 1.61375f, 553.0f, 1.0f, 3.0f, 1.513688f, 0.12f, 0.45f, 2.0f,
    142.0f, 2.5f, 0.0005f, 2.97f,
    1.0f, 0.03f, 0.5f,
    0.0f, 1.5f, 0.9f, 1.0f, 10.4f, 1.18f,
    20.0f, 89.0f,
};
// rhodes by hand with the pickup close (gap 0.75 mm, offset 0.3 mm) and a harder tip (hardness 338, felt 3.5e-4),
// heard; the tonebar at rhodes's 1.51 cents
constexpr Table kRhodesBark = {
    0.614f, 1.61375f, 553.0f, 1.0f, 3.0f, 1.513688f, 0.12f, 0.45f, 2.0f,
    337.7348f, 2.5f, 0.0003535534f, 2.97f,
    1.0f, 0.03f, 0.5f,
    0.0f, 0.75f, 0.29999995f, 1.0f, 10.4f, 1.18f,
    20.0f, 89.0f,
};
// by hand (the reed, the pickup a little close: gap 1.22 mm, offset 0.72 mm; a softer hammer: hardness 84, felt
// 6.2e-4), heard
constexpr Table kWurlitzer = {
    0.614f, 1.61375f, 553.0f, 1.0f, 3.0f, 0.5f, 0.12f, 0.45f, 2.0f,
    84.4337f, 2.5f, 0.00061557227f, 2.97f,
    1.0f, 0.03f, 0.5f,
    1.0f, 1.2183785f, 0.71999997f, 0.0f, 10.4f, 1.18f,
    20.0f, 89.0f,
};

// a table's shape: the instrument channels, the rest nan
Shape shapeOf(const Table& t) {
    Shape s = nanShape();
    for (int i = 0; i < kInstrumentChannels; ++i) s.v[i] = t[i];
    return s;
}

}  // namespace

// each preset's shape, built once from its table
const Shape& grand() { static const Shape s = shapeOf(kGrand); return s; }
const Shape& concertGrand() { static const Shape s = shapeOf(kConcertGrand); return s; }
const Shape& upright() { static const Shape s = shapeOf(kUpright); return s; }
const Shape& spinet() { static const Shape s = shapeOf(kSpinet); return s; }
const Shape& honkyTonk() { static const Shape s = shapeOf(kHonkyTonk); return s; }
const Shape& fortepiano() { static const Shape s = shapeOf(kFortepiano); return s; }
const Shape& rhodes() { static const Shape s = shapeOf(kRhodes); return s; }
const Shape& rhodesBark() { static const Shape s = shapeOf(kRhodesBark); return s; }
const Shape& wurlitzer() { static const Shape s = shapeOf(kWurlitzer); return s; }

namespace {
const PresetInfo kPresets[] = {
    {"grand", grand, 21, 108},           {"concertGrand", concertGrand, 21, 108}, {"upright", upright, 21, 108},
    {"spinet", spinet, 21, 108},         {"honkyTonk", honkyTonk, 21, 108},       {"fortepiano", fortepiano, 29, 101},
    {"rhodes", rhodes, 28, 100},         {"rhodesBark", rhodesBark, 28, 100},     {"wurlitzer", wurlitzer, 33, 96},
};
}  // namespace

int presetCount() { return (int)(sizeof kPresets / sizeof kPresets[0]); }
const PresetInfo& presetAt(int i) { return kPresets[i < 0 ? 0 : (i >= presetCount() ? presetCount() - 1 : i)]; }
const PresetInfo* presetByName(const char* name) {
    if (!name) return nullptr;
    for (const PresetInfo& p : kPresets) {
        if (std::strcmp(p.name, name) == 0) return &p;
    }
    return nullptr;
}

}  // namespace up
