// upright — up_pitch.cpp
// once per block: the glide's coefficient when its rate moved, the note's
// frequency when the note moved.
#include "up_pitch.h"

namespace up {

void Pitch::init(float fs) {
    dt_ = (float)kBlock / (fs > 1.0f ? fs : kDefaultFs);
    coefRate_ = -1.0f;
    f0_ = 0.0f;
    jump_ = true;
}

float Pitch::block(float midi, float glidePerSec) {
    const float target = midi == midi ? clamp(midi, 0.0f, 127.0f) : note_;
    float next = target;
    if (!jump_ && glidePerSec > 0.0f) {
        if (glidePerSec != coefRate_) {
            coef_ = approachCoef(glidePerSec, dt_);
            coefRate_ = glidePerSec;
        }
        next = note_ + (target - note_) * coef_;
    }
    jump_ = false;
    if (next != note_ || !(f0_ > 0.0f)) {
        note_ = next;
        f0_ = midiToHz(note_);
    }
    return f0_;
}

}  // namespace up
