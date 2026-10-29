// upright — up_presets.h  (plan §12)
// the named pianos, each a knob vector (up_knobs.h) and a playing range in
// midi notes. a preset's shape is pianoOf(its knobs), built once on first
// use: the twenty-two instrument channels, the partial and player channels
// nan (the law writes the partials at the note; setShape leaves the note,
// the glide and the pedal alone). a vector is found with uptool fit against
// a reference table where one exists (the b tables on the scale knob), else
// set by hand and heard; uptool print --preset NAME writes it as a line of
// the table.
//
//   grand         the eight at 0 but generator -1: the level reference
//   concertGrand  the steinway d's b fitted on the scale; a concert board, a little wider unison
//   upright       the upright's b fitted on the scale; a small damped board, a softer hammer
//   spinet        the shortest scale, the smallest board, a harder hammer
//   honkyTonk     the upright pulled wide, its hammers soft
//   fortepiano    a shorter scale, soft leather-like hammers, a small board
//   rhodes        the tine through the magnet, set; the tonebar beside it
//   rhodesBark    the tine close to the magnet, a hard tip: the bark
//   wurlitzer     the reed through the plate, a softer hammer
#pragma once

#include "up_channels.h"
#include "up_knobs.h"

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

// the table: name, vector, and the playing range in midi notes
struct PresetInfo {
    const char* name;
    Knobs knobs;
    int lo, hi;
    const Shape& shape() const;                     // pianoOf(knobs), built once
};
int presetCount();
const PresetInfo& presetAt(int i);
const PresetInfo* presetByName(const char* name);   // null when there is none

}  // namespace up
