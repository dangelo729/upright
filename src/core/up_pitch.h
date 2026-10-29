// upright — up_pitch.h  (plan §6)
// the pitch axis, once per block: the midi channel's eased value eased again
// at the glide channel's rate, in semitones, by the one smoothing law; a
// glide at or under zero jumps, and so does the first note after a wake
// (jump()). f0 = midiToHz(note). no vibrato: a bend is a glide. the rank at
// the note is up_rank.h's rankAt, which the voice runs when the note or a
// channel it reads moved.
//
// approachCoef and midiToHz run here, once per block and only when their
// inputs move.
#pragma once

#include "up_config.h"

namespace up {

class Pitch {
public:
    void  init(float fs);
    void  jump() { jump_ = true; }                          // the next block lands the note at once
    float block(float midi, float glidePerSec);             // once per block: f0, hz
    float note() const { return note_; }                    // the eased note, midi
    float f0() const { return f0_; }

private:
    float dt_ = (float)kBlock / kDefaultFs;
    float note_ = kScaleMidi, f0_ = 0.0f;                   // f0 0: the next block computes it
    float coefRate_ = -1.0f, coef_ = 1.0f;                  // the glide's 1 - exp(-rate dt), for coefRate_
    bool  jump_ = true;
};

}  // namespace up
