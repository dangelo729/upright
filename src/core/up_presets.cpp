// upright — up_presets.cpp  (plan §12)
// each preset's shape is built once, on first use (pianoOf calls libm: once
// a preset, never per sample).
#include "up_presets.h"

#include <cstring>

namespace up {
namespace {

// the vectors: scale, hammer, strike, unison, body, generator, pickup, damper.
// each with where it came from: the fit's residual (uptool fit --table) or by
// hand, heard (plans/upright-engine/out2/listen)

// the level reference, by definition: the eight at 0 but generator -1
constexpr Knobs kGrand = {{0.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f}};
// fit --table inharmonicity_grand.txt: scale +0.97, rms ln 0.191 over the steinway d's 77 keys; body +1, unison +0.2 by hand, heard
constexpr Knobs kConcertGrand = {{0.96875f, 0.0f, 0.0f, 0.2f, 1.0f, -1.0f, 0.0f, 0.0f}};
// fit --table inharmonicity_upright.txt: scale -0.45, rms ln 0.301 over its 97 keys; body -0.7, hammer -0.2 by hand, heard
constexpr Knobs kUpright = {{-0.453125f, -0.2f, 0.0f, 0.0f, -0.7f, -1.0f, 0.0f, 0.0f}};
// by hand (the shortest scale, the smallest board, a harder hammer), heard
constexpr Knobs kSpinet = {{-1.0f, 0.3f, 0.0f, 0.0f, -1.0f, -1.0f, 0.0f, 0.0f}};
// upright with unison +1 and hammer -0.5 (bank's recipe: detuned, inharmonic, soft), heard
constexpr Knobs kHonkyTonk = {{-0.453125f, -0.5f, 0.0f, 1.0f, -0.7f, -1.0f, 0.0f, 0.0f}};
// by hand (scale -0.3, hammer -0.7, body -0.5; the gauge as the knobs write it), heard
constexpr Knobs kFortepiano = {{-0.3f, -0.7f, 0.0f, 0.0f, -0.5f, -1.0f, 0.0f, 0.0f}};
// by hand (the tine, the pickup set, damper 0), heard; unison +0.228: the tonebar 1.51 cents from the tine (0.5 + 19.5 x
// 0.228²), the 1.5 its beat was heard at, under a tuned detune of 0.5 (the strings capped at 2 on the tine side)
constexpr Knobs kRhodes = {{0.0f, 0.0f, 0.0f, 0.228f, 0.0f, 0.0f, 0.0f, 0.0f}};
// by hand (the tine, the pickup close, a harder tip), heard; unison +0.228: the tonebar at rhodes's 1.51 cents
constexpr Knobs kRhodesBark = {{0.0f, 0.5f, 0.0f, 0.228f, 0.0f, 0.0f, 1.0f, 0.0f}};
// by hand (the reed, the pickup a little close, a softer hammer), heard
constexpr Knobs kWurlitzer = {{0.0f, -0.3f, 0.0f, 0.0f, 0.0f, 1.0f, 0.3f, 0.0f}};

}  // namespace

// each preset's shape, built once from its vector
const Shape& grand() { static const Shape s = pianoOf(kGrand); return s; }
const Shape& concertGrand() { static const Shape s = pianoOf(kConcertGrand); return s; }
const Shape& upright() { static const Shape s = pianoOf(kUpright); return s; }
const Shape& spinet() { static const Shape s = pianoOf(kSpinet); return s; }
const Shape& honkyTonk() { static const Shape s = pianoOf(kHonkyTonk); return s; }
const Shape& fortepiano() { static const Shape s = pianoOf(kFortepiano); return s; }
const Shape& rhodes() { static const Shape s = pianoOf(kRhodes); return s; }
const Shape& rhodesBark() { static const Shape s = pianoOf(kRhodesBark); return s; }
const Shape& wurlitzer() { static const Shape s = pianoOf(kWurlitzer); return s; }

namespace {
const PresetInfo kPresets[] = {
    {"grand", kGrand, 21, 108},           {"concertGrand", kConcertGrand, 21, 108}, {"upright", kUpright, 21, 108},
    {"spinet", kSpinet, 21, 108},         {"honkyTonk", kHonkyTonk, 21, 108},       {"fortepiano", kFortepiano, 29, 101},
    {"rhodes", kRhodes, 28, 100},         {"rhodesBark", kRhodesBark, 28, 100},     {"wurlitzer", kWurlitzer, 33, 96},
};
// the named accessors, in the table's order
const Shape& (*const kShapes[])() = {grand, concertGrand, upright, spinet, honkyTonk, fortepiano, rhodes, rhodesBark, wurlitzer};
static_assert(sizeof kShapes / sizeof kShapes[0] == sizeof kPresets / sizeof kPresets[0], "an accessor a preset");
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

// a table entry's shape is its accessor's; a copy of one (its name and
// vector the same) too; any other PresetInfo's is rebuilt into one slot at
// each call
const Shape& PresetInfo::shape() const {
    for (int i = 0; i < presetCount(); ++i) {
        const PresetInfo& p = kPresets[i];
        if (this == &p || (std::strcmp(name, p.name) == 0 && std::memcmp(&knobs, &p.knobs, sizeof knobs) == 0)) return kShapes[i]();
    }
    static Shape other;
    other = pianoOf(knobs);
    return other;
}

}  // namespace up
