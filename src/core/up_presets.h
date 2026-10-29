// upright — up_presets.h  (plan §12)
// the named pianos, each a table of the twenty-four instrument channels (in
// the enum's order, up_channels.h) and a playing range in midi notes. a
// preset's shape is its table, built once on first use: the instrument
// channels written, the partial and player channels nan (the law writes the
// partials at the note; setShape leaves the note, the glide and the pedal
// alone). a table's strings are found with uptool fit against a reference
// table where one exists (the b tables), the rest set by hand and heard;
// uptool print --preset NAME writes a preset's channels as --set lines.
//
//   grand         the reference, and every voice's start (defaultShape)
//   concertGrand  the steinway d's b fitted on the strings; a concert board, a little wider unison
//   upright       the upright's b fitted on the strings; a small damped board, a softer hammer
//   spinet        the shortest strings, the smallest board, a harder hammer
//   honkyTonk     the upright pulled wide, its hammers soft
//   fortepiano    shorter strings, soft leather-like hammers, a small board
//   rhodes        the tine through the magnet, set; the tonebar beside it
//   rhodesBark    the tine close to the magnet, a hard tip: the bark
//   wurlitzer     the reed through the plate, a softer hammer
#pragma once

#include "up_channels.h"

namespace up {

const Shape& grand();
const Shape& concertGrand();
const Shape& upright();
const Shape& spinet();
const Shape& honkyTonk();
const Shape& fortepiano();
const Shape& rhodes();
const Shape& rhodesBark();
const Shape& wurlitzer();

// the table: name, shape, and the playing range in midi notes
struct PresetInfo {
    const char* name;
    const Shape& (*shape)();                        // its accessor: the shape, built once
    int lo, hi;
};
int presetCount();
const PresetInfo& presetAt(int i);
const PresetInfo* presetByName(const char* name);   // null when there is none

}  // namespace up
