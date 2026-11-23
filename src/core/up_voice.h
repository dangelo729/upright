// upright — up_voice.h
// one voice: one key's strings, the whole public surface. every call writes
// channel targets (or the tuning, or a strike, or the key's state) and lands
// at the top of the next block; the last writer wins, per channel, in the
// caller's order.
//
// one block, in order:
//   the tuning, when one was set (the group rates, the pole, the ceiling)
//   the law: when an instrument channel was written since its last run, or
//         the note's target moved, every partial channel's target is written
//         under the sequence of the newest instrument write, from the
//         targets at the target note (a partial written by hand later stands
//         until the instrument is written again)
//   wake: a sleeping voice with a strike queued wakes: every channel lands
//         on its target (the law's partials with them), the banks are zeroed,
//         the note jumps. a sleeping voice with none does nothing: its
//         channels do not tick, its render is zeros
//   the channels tick (awake, and not on the block it woke)
//   the pitch axis: the eased note at the glide -> f0
//   the rank at the eased note (when it or a channel it reads moved): L, mu,
//         the hammer's mass, its felt's scale and power, the strike share,
//         the t60 scale, the bridge's scale, the damper
//   the hammer: the eased felt channels at the rank's scale and power, the
//         tine's tip by the generator, the rank's mass (the table rebuilt only
//         where they moved)
//   per slot, from the eased partial channels: hz = ratio f0; on under
//         tuning.ceiling x fs with a gain or an out and a t60 over 0; its own
//         decay sigma_n = 6.907755 / (t60 t60Scale), less sigma_b under
//         kUnisonModes (the rank's bridgeLoss x clamp(-g, 0, 1), held at
//         0.9 sigma_n: the bridge takes the rest), and the damper's rate
//         (dampRateOf, the eased pedal) once released on a key with one; the
//         drive scale gain 2 dtSub / (mu L w) x 1000; the readback weight the
//         gain; the output weight the out. then the secondaries (up_string.h)
//         at the detune, their weights from the capped strings channel (the
//         second x the tonebar from the generator 0 up), the generator; the
//         coupling at sigma_b and its reactive delta = sigma_b x
//         bridgeReactive; the hammer's point mass, (mu L / 2) / sum gain²;
//         every bank's beginBlock
//   a strike due: the hammer placed at the string, thrown at hammerScale
//         vel^velCurve, held under 1000 m/s
//   the pair's weights ramped across the block: the bridge's clamp(-g, 0, 1)
//         x the rank's bridge scale / 1000 (mm to N), the pickup's clamp(1 +
//         g, 0, 1), both x outputGain; a weight at 0 at both ends compiles its
//         sum out
//   the attack (up_attack.h) where the bridge is heard: its long bank at the
//         rank's string and strike share at tuning.attack; a strike due arms
//         its thud at the rank's hammer mass x the throw (the blow's share
//         follows the hammer's force while it touches); its weight ramped
//         across the block, clamp(-g, 0, 1) x outputGain (the bridge's
//         without the rank's scale: the attack is in newtons). off the
//         string side, or with nothing to add, it is compiled out
//   128 samples: while the hammer touches, kContactRate substeps a sample
//         (the hammer's loop); from the sample it leaves, every bank's turn
//         (the block split there); the bridge force with the attack's added,
//         and the pickup's curve and pole on the tip's sums
//   the telemetry: every amplitude, the attack's long bank's, the hammer, the
//         levels, the loudest; the silence gate: every string bank's energy
//         (mm²) under tuning.silence at the end of a block with no hammer
//         touching and no strike queued: the banks and the attack are zeroed
//         and the voice sleeps
//
// control runs every 128 samples whatever size render() is called with, so
// the same calls give the same bytes at any render size. no heap, no libm per
// sample: the law per instrument write and per note, the rank per note, the
// felt's table per hammer write, the coefficients per block where they
// moved, the attack's cosines per block, the strike's speed per strike. one
// seeded xorshift32 per voice (the constructor's seed, reseed()), drawn by
// nothing in the engine: any seed renders the same bytes.
//
// a strike lands at the next block's top, one block's latency, as every
// write. two noteOns in one block: the second replaces the first's strike
// (its note too, by the channel's last-writer rule). a strike while the last
// hammer still touches places a fresh one; a key struck again while its
// string sounds meets the moving string. noteOff marks the key released: the
// damper's rate is added from the next block, scaled by 1 - the eased pedal
// (a pedalled release rings; the pedal's fall lands the damper at its ease);
// a key above damperTop has no damper. a noteOn clears the release.
//
// the handoff, for a voice written from one thread and rendered in another
// (the teensy wrapper): attach() seeds a Pending table with the voice's own
// targets, rates and sequence, and every write lands there from then on.
// latch(), called where no writer can run, takes every target and rate newer
// than the voice's, the tuning when one was set, a queued strike and the
// key's release; a latched voice renders the same bytes as one written
// directly. unattached, a voice writes its own table, as on the host.
#pragma once

#include <cstdint>

#include "up_attack.h"
#include "up_channels.h"
#include "up_config.h"
#include "up_law.h"
#include "up_pickup.h"
#include "up_pitch.h"
#include "up_rank.h"
#include "up_string.h"
#include "up_telemetry.h"
#include "up_tuning.h"

namespace up {

// the writer's side of an attached voice
struct Pending {
    Channels ch;                   // the targets and rates as written, under the shared sequence
    Tuning   tuning;               // the last one asked for, clamped
    bool     tuningSet = false;    // a tuning since the last latch
    bool     strike = false;       // a noteOn since the last latch
    float    vel = 0.0f;           // its velocity, 0 .. 1
    bool     released = false;     // the key as the writer last left it: noteOff sets it, noteOn clears it
};

class Voice {
public:
    explicit Voice(uint32_t seed = 1, float fs = kDefaultFs);
    void init(float fs);                     // a new rate: asleep, every phasor zero, the channels kept

    // ---- the player ---------------------------------------------------------------------
    void noteOn(float midi, float vel01);    // the note, and a hammer at the next block
    void noteOff();                          // the key released: the damper lands (unless the pedal holds it, or the key has none)
    void setPitch(float midi);
    void setGlide(float perSec);
    void setPedal(float p01);                // the sustain pedal, eased at ratePedal

    // ---- the instrument: every channel, by name ---------------------------------------
    void  set(Channel c, float target);
    void  setRate(Channel c, float perSec);
    void  setShape(const Shape& s);          // every target at once; a nan or an infinite entry is no write
    Shape shape() const;                     // the targets as held

    // ---- conveniences that write channels ---------------------------------------------
    void setStrings(float length, float bass, float tension, float gauge);
    void setUnison(float strings, float detune);
    void setHammer(float hardness, float exponent, float felt, float massG);
    void setStrike(float share);
    void setBody(float area, float loss, float open);
    void setGenerator(float g, float gap, float offset);
    void setDamper(float rate, float top);
    void setPartial(int k, float ratio, float t60, float gain, float out);   // one partial by hand, under one sequence
    void setPartials(const PartialTable& t); // a measured string, whole (a nan or an infinity skips), under one sequence

    // ---- who is playing -----------------------------------------------------------------
    void   setTuning(const Tuning& t);       // lands at the top of the next block
    Tuning tuning() const { return pending_ ? pending_->tuning : pendingTuning_; }

    // ---- readback -------------------------------------------------------------------------
    State     state() const { return state_; }   // the eased instrument, f0, the rank, each partial's amplitude, the hammer, the levels, asleep
    int       partials(PartialTable& out) const;  // the eased table; returns kMaxModes
    int       stringShape(float* y, int n) const; // the main string's displacement (mm) at n points from the agraffe to the bridge
    float     stringAt(float share) const;        // the same at one share of the length
    RankState rank() const { return state_.rank; }
    const Strings& strings() const { return strings_; }   // the strings as they stand (the host's window)
    const Attack&  attack() const { return attack_; }     // the attack as it stands (the host's window)

    // ---- the engine -----------------------------------------------------------------------
    // the pair a pool mixes, each already at its generator weight, x outputGain, not clamped: the bridge force
    // (N) and the pickup's voltage. either may be null
    void render(float* bridge, float* pickup, int n);

    // ---- the handoff ----------------------------------------------------------------------
    void attach(Pending* p);                 // every write to p from now on; null writes here again
    void latch();                            // take p's newer writes; lands at the top of the next block

    // ---- for a pool -------------------------------------------------------------------------
    void  reseed(uint32_t seed);             // the voice's generator starts again from seed (nothing in the engine draws it)
    void  stop();                            // asleep now: every phasor zero, the hammer off, no strike queued
    bool  asleep() const { return asleep_; }
    float energy() const { return energy_; } // every bank's summed energy (mm²) as of the last block
    bool  released() const { return released_; }
    float bridgePoint() const { return state_.shape[Channel::midi]; }   // the note as eased, for the board's weights

private:
    Channels& writes() { return pending_ ? pending_->ch : ch_; }
    void block();
    template <bool kBridge, bool kTip, bool kAttack> void run(float* bridge, float* pickup, int n);
    void samples(float* bridge, float* pickup, int n);
    void endBlock();
    void wake();
    void sleep();
    void publish();

    float    fs_ = kDefaultFs;
    uint32_t seed_ = 1;
    Tuning   tuning_, pendingTuning_;
    bool     tuningPending_ = true;
    Channels ch_;
    Law      law_;
    Pitch    pitch_;
    Strings  strings_;
    Attack   attack_;
    Pickup   pickup_;
    Rng      rng_;

    float dt_ = (float)kBlock / kDefaultFs;
    float top_ = kCeiling * kDefaultFs;      // the ceiling, hz
    float lawNote_ = -1.0f;                  // the note the law last wrote at
    int   phase_ = 0;
    bool  asleep_ = true, woke_ = false;
    bool  strikeQueued_ = false;
    float vel_ = 0.0f;
    bool  released_ = false;
    float gain_ = 1.0f;                      // tuning.outputGain
    Ramp  bridgeW_, pickupW_, attackW_;      // the pair's weights and the attack's across the block
    float bridgeT_ = 0.0f, pickupT_ = 0.0f, attackT_ = 0.0f;   // where they land at its end
    bool  doBridge_ = false, doTip_ = false, doAttack_ = false;
    float sqBridge_ = 0.0f, sqPickup_ = 0.0f, sq_ = 0.0f;   // this block's, summed
    float energy_ = 0.0f;
    int   modesOn_ = 0;
    // what the rank and the hammer were last computed from
    float rankKey_[8] = {};
    float hammerKey_[7] = {};
    float re_[kMaxModes] = {};               // the main string's phasors' re as the block ended (for the picture)

    State state_;

    // the handoff: the writer's table, and its sequence as last latched
    Pending* pending_ = nullptr;
    uint32_t latched_ = 0;
};

}  // namespace up
