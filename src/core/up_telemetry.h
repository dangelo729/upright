// upright — up_telemetry.h
// nothing is measured from the signal: the banks are the telemetry. per
// block the voice publishes a State: the eased channels, f0, the rank at the
// note, every partial's amplitude |z_k| (the main string's and the
// secondaries' first kUnisonModes, mm, as the block ends; the attack's
// longitudinal modes' in their drive's unit), the hammer (its
// height and velocity, the block's peak force, whether it touches, the
// strike's contact so far), the block's levels (the bridge force's rms in N,
// the pickup's voltage's rms in its unit, the voice's pair summed at their
// weights), the loudest partial, the partials on, the key and the pedal,
// asleep.
//
// for the picture: stringAt(re, shape, share) is the main string's
// displacement at a share of its length from the phasors and the mode
// shapes, sum re_k shape_k(x): sin(n pi x) on the string, the beam's shape
// over the tip's 2 on the electric side (the first kTineModes slots), blended
// along the generator as the law blends the gains, so at the strike point it
// is the hammer's readback where the gains are the law's. a sin a partial a
// point (and the beam's shape): the app's call rate. stringShape(state, y, n)
// is the same at n points from a State alone (its phasors' re and its eased
// generator), so a copy of the state draws the string as it was. the hammer
// draws at strikeShare x L, at its height.
#pragma once

#include "up_channels.h"
#include "up_config.h"
#include "up_rank.h"

namespace up {

struct State {
    Shape     shape;                           // the eased values (every channel)
    float     f0 = 0.0f;                       // hz
    RankState rank{};                          // the string at the note
    float     amp[kMaxModes] = {};             // the main string's partial amplitudes, |z_k|, mm, as of the block's end
    float     re[kMaxModes] = {};              // the main string's phasors' re, mm, as of the block's end (the picture's)
    float     amp2[kUnisonModes] = {}, amp3[kUnisonModes] = {};   // the secondaries'
    float     ampLong[kLongModes] = {};        // the attack's longitudinal modes, |z_k| in its drive's unit (up_attack.h)
    float     hammerHeight = 0.0f, hammerVelocity = 0.0f, hammerPeakForce = 0.0f;   // m, m/s, N (the block's peak)
    bool      hammerTouching = false;
    float     hammerContact = 0.0f;            // s: the last strike's contact, from the first compression, so far
    float     bridgeRms = 0.0f, pickupRms = 0.0f, rms = 0.0f;   // this block's: N, the pickup's unit, the pair summed at their weights
    int       loudest = 0, modesOn = 0;
    bool      released = false, damped = false;   // the key let go; the damper on the strings (a key with one, released)
    float     pedal = 0.0f;                    // the eased pedal
    bool      asleep = true;
};

// the main string's displacement (mm) at a share of its length (0 the agraffe, 1 the bridge), from its
// phasors' re (count slots) and the mode shapes at the eased generator and share
float stringAt(const float* re, int count, const Shape& eased, float share);

// the same from a state (its re and its eased shape) at n points from the agraffe to the bridge, evenly (one
// point: the agraffe); returns n, or 0 for no points
int stringShape(const State& st, float* y, int n);

}  // namespace up
