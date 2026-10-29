// upright — up_knobs.h  (plan §2)
// eight signed knobs named for the ear, each -1 .. 1: a piano as a point.
// pianoOf is a pure mapping from the eight to the twenty-two instrument
// channels (the partial channels and the player's nan: the law writes the
// partials at the note); knobsOf reads an instrument back to the eight.
// nothing in the audio code knows knobs exist: a caller holds the eight and
// writes the piano they make with setShape.
//
//   scale      spinet · grand · concert: length = l0 2^(s scale), bass =
//              bass0 2^(s scaleBass); tension and gauge as designed
//   hammer     soft · medium · hard: hardness = k 2^(h kHard), felt =
//              kLambda 2^(-h kLamSpan); exponent = p and the mass massC4
//   strike     end · eighth · middle: strikeShare piecewise linear through
//              kStrikeEnd, kStrikeEighth, kStrikeMid
//   unison     one · three · wide: strings = 1 + 2 clamp(u + 1, 0, 1) (1 at
//              -1, 3 from 0 up); detune from kDetune at and under 0 to
//              kDetuneWide at +1 along clamp(u, 0, 1)²
//   body       upright · parlor · concert: boardArea = kArea0 2^(b
//              kAreaSpan), boardLoss = kEta 2^(-b kEtaSpan), open = (b + 1) / 2
//   generator  string · tine · reed: generator = g, tonebar = 1 - |g|
//   pickup     far · set · close: gap = kGap0 2^(-p kGapSpan), offset from
//              kOffFar to kOffClose (never zero); written on the acoustic
//              side too, where they are inert, so the read-back holds
//   damper     heavy · felt · light: dampRate = kDamp 2^(-d kDampSpan),
//              damperTop kDamperTop
//
// the knobs' constants that a fit may move are the law's (LawConstants); the
// rest sit at the top of up_knobs.cpp with what set each.
//
// levelDb(knobs): how loud a vector sounds, db against grand over the octave
// c4 .. b4 (each note at velocity 1, the rms over the first half second after
// the strike, the board's output and the pickup's summed, the octave the mean
// of the twelve powers), from the measured table in up_level_table.inc
// (uptool level --write): the generator's own row, then each other knob's
// row at the two of the generator's four anchors (string -1, middle -0.5,
// tine 0, reed +1) about the vector's generator, each linear between its
// eleven points and held past either end, each anchor's seven summed and the
// two sums linear in the generator between the anchors. a caller undoes it
// with a gain of 10^(-db / 20), which lands the vector at grand's octave
// level.
//
// every written channel is held inside channelRange. no state, nothing on
// the heap: it runs once per write, never per sample, so libm is fine.
#pragma once

#include <cstdint>

#include "up_channels.h"
#include "up_law.h"
#include "up_tuning.h"

namespace up {

enum class Knob : uint8_t { scale, hammer, strike, unison, body, generator, pickup, damper, count };
constexpr int kKnobCount = (int)Knob::count;
struct Knobs { float v[kKnobCount]; };               // each -1 .. 1 (a nan reads as 0, past the end is the end)

const char* knobName(Knob k);                        // "scale" .. "damper"; "" past the last
bool        knobByName(const char* name, Knob& out);
const char* knobEnd(Knob k, int which);              // 0 the -1 end, 1 the middle, 2 the +1 end
Shape       pianoOf(const Knobs& k, const Tuning& t = Tuning());   // pure; the instrument channels written, the partial channels and the player's nan
Shape       pianoOf(const Knobs& k, const Tuning& t, const LawConstants& c);   // under other constants (the fit's)
Knobs       knobsOf(const Shape& s);                 // the eight read back from the instrument channels; exact on a pianoOf
float       levelDb(const Knobs& k);                 // db against grand's octave c4 .. b4: undo with a gain of 10^(-db / 20)

}  // namespace up
