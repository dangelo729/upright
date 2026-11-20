// upright — up_tuning.h
// up::Tuning: the rates, the hammer's throw, the level and the guards. a
// tuning says who is playing, not what is played: the channels are the
// piano, the tuning is how the machine under it behaves. a voice nobody
// tunes is exactly the field initializers.
//
// clamped() holds only what would end the voice: a nan falls back to the
// field's default, the ceiling stays inside 0.01 .. 0.5, the hammer's throw
// and the amp's pole over 0, the attack inside 0 .. 2. a channel rate at or
// under zero stands (a jump); nothing else is limited.
#pragma once

namespace up {

struct Tuning {
    // ---- channel rates by group, per second (0 = jump) --------------------------------
    float rateStrings = 8.0f;     // the strings group: length, bass, tension, gauge, unison, strike, losses
    float rateHammer  = 8.0f;     // the hammer group: hardness, exponent, felt, mass
    float rateBody    = 4.0f;     // the body, the generator and the damper groups: the instrument's slow things
    float rateModes   = 8.0f;     // every partial's ratio, t60, gain and out
    float ratePitch   = 0.0f;     // the note lands at once; the glide channel eases it
    float ratePedal   = 30.0f;    // the sustain pedal, down and up

    // ---- the hammer's throw ---------------------------------------------------------------
    float hammerScale = 7.0f;     // kVmax: the hammer's speed at velocity 1, m/s (a fortissimo hammer)
    float velCurve    = 2.0f;     // v = hammerScale vel^velCurve: velocity 0.3 is 0.63 m/s (pp), 0.5 is 1.75 (mf)

    // ---- the voice ----------------------------------------------------------------------
    float outputGain  = 1.0f;     // the voice's pair (bridge force, pickup voltage) to full scale, before the clamp
    float silence     = 1e-9f;    // mm²: every bank's summed re² + im² under which a voice with nothing queued sleeps
    float stealRate   = 60.0f;    // per second: the pool's fade of a stolen voice
    float ceiling     = 0.45f;    // a partial over this share of fs is off, 0.01 .. 0.5
    float ampHz       = 6000.0f;  // the pickup amp's one pole, hz

    // ---- the attack (up_attack.h) ---------------------------------------------------------
    float attack      = 1.0f;     // the knock, the thud and the longitudinal clank together, 0 .. 2: 0 is none, 1 the
                                  //   calibration (grand's c4: the attack's own 10 db under the string over its first
                                  //   10 ms, the thud 4 db over the band under 150 hz; grand's a0: the clank 10 db under
                                  //   the note)
};

// the fields that have a limit held inside it, a nan as its default
Tuning clamped(const Tuning& t);

}  // namespace up
