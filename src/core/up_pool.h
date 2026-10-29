// upright — up_pool.h  (plan §13, the pool)
// m voices, a note table and one board: one piano on a keyboard. a key is a
// strike of the piano the pool was last written, at that note (the rank
// chooses the string at the note, up_rank.h).
//
// the note table. a voice is free (asleep, no note), held (its key down),
// released (its key up: its damper on, unless the pedal holds it, or ringing
// out above damperTop) or stolen (fading out for a note that waits for it).
// noteOn strikes the key's own voice again if it still sounds (held or
// released: Voice::noteOn, a fresh hammer on the moving string, nothing
// restarts); else a stolen voice already waiting for the key takes the newer
// velocity; else a free voice lands the note whole (the one longest asleep);
// else it steals the quietest sounding voice, held or released (the least
// Voice::energy(), the last block's summed string energy: a cut is least
// heard where the string is quietest; a voice whose strike is still queued
// counts as loud while another sounds, ties go to the oldest note), and when
// every voice is already stolen it replaces the note the oldest one waits
// for. a stolen voice's share fades at tuning.stealRate by the one law,
// block by block, to under 1 % of its level, then linearly to exactly zero
// across one block; at the next block it is stopped (Voice::stop: its share
// is already zero, so nothing is cut) and takes the waiting note, which lands
// whole: 27 blocks, 78 ms, at the default 60 a second. the board it fed rings
// on, as a soundboard does.
//
// noteOff releases the key's voice (Voice::noteOff: its damper lands unless
// the pedal holds it); a note still waiting lands released. pedal writes
// every voice's pedal channel and is the pool's pedal: a note that lands
// takes it. allNotesOff releases every key.
//
// pool-wide writes (set, setRate, setShape, setPartial, setPartials,
// setTuning) go to every voice now, in the caller's order, and stand over
// later notes: a voice keeps its channel table across notes, so a note
// lands on the piano the pool last wrote, the last writer winning per
// channel by the sequence, exactly as on one voice (a partial written by hand
// pool-wide stands on a later note until the instrument is written again).
// the board's plate is the written body: setShape sets it from the shape's
// boardArea, boardLoss and open where they are not nan, set from the one
// channel written; the plate lands at once, not eased (Board::setPlate, its
// table rebuilt only when it moved).
//
// render: control runs once per 128 samples whatever size render() is called
// with: the pool's block (the stolen voices, the frees, the levels), then
// every voice's pair (the bridge force and the pickup's voltage) into
// scratch, each at its level; each voice's bridge force into the board at
// its bridge point's weights (Board::inputWeights at Voice::bridgePoint(),
// recomputed when its note moved or the plate did); the board rendered once
// on every voice's force; the pickups summed in voice order; the board's
// output and the pickups' sum added: one output, not clamped (a wrapper
// clamps what it sends). the same calls give the same bytes at any render
// size. the voices' seeds follow from the pool's (seedOf). nothing on the
// heap, no libm: the fade's step is the one law's coefficient (up_config.h),
// set per tuning; the board's table runs per body write (up_board.cpp).
//
// Pool<M> holds its m voices, their slots and their scratch in itself: the
// storage is the template's, the logic PoolBase's, here and in up_pool.cpp.
// PoolBase runs over storage it is given, for a host tool that sizes a pool
// at run time.
#pragma once

#include <cstdint>

#include "up_board.h"
#include "up_channels.h"
#include "up_config.h"
#include "up_tuning.h"
#include "up_voice.h"

namespace up {

struct PoolTestAccess;             // the tests' window: one voice's share of the output, beside it

// a voice's line in the note table
struct PoolSlot {
    enum class Use : uint8_t { free, held, released, stolen };
    Use      use = Use::free;
    float    key = 0.0f;                           // the key it sounds, midi
    float    vel = 0.0f;
    uint32_t age = 0;                              // when its use began, in the pool's events: its strike, or its sleep when free
    bool     struck = false;                       // a strike queued that its voice takes at the next block
    // a stolen voice's waiting note
    bool     waiting = false;
    float    nextKey = 0.0f, nextVel = 0.0f;
    bool     nextOff = false;                      // its key already up: it lands released
    // its level in the sum: 1, or the fade of a stolen voice
    float    fade = 1.0f, levelT = 1.0f;
    Ramp     level;
};

// a voice's scratch: its bridge force for the block (at its level, the board's input), its bridge point's
// weights on the board and the note they were computed at
struct PoolLane {
    float bridge[kBlock];
    float weights[kBoardModes];
    float point;
};

class PoolBase {
public:
    // m voices, their slots and their lanes, and m force and weight pointers for the board, held by the
    // caller (Pool<M> holds them)
    PoolBase(Voice* voices, PoolSlot* slots, PoolLane* lanes, const float** forces, const float** weights, int m, uint32_t seed = 1,
             float fs = kDefaultFs);
    PoolBase(const PoolBase&) = delete;
    PoolBase& operator=(const PoolBase&) = delete;

    // ---- the player -------------------------------------------------------------------
    void noteOn(float midi, float vel01);          // the key's own voice struck again; else a free one; else the quietest, stolen
    void noteOff(float midi);                      // the key released: its damper lands (unless the pedal holds it)
    void pedal(float p01);                         // the sustain pedal on every voice, and on every note that lands
    void allNotesOff();

    // ---- every voice, and later notes -------------------------------------------------------
    void set(Channel c, float target);
    void setRate(Channel c, float perSec);
    void setShape(const Shape& s);                 // a nan entry is no write
    void setPartial(int k, float ratio, float t60, float gain, float out);   // one partial by hand
    void setPartials(const PartialTable& t);       // a measured string, whole (a nan skips)
    void setTuning(const Tuning& t);

    // ---- readback ----------------------------------------------------------------------------
    int             voices() const     { return m_; }
    int             sounding() const;              // voices awake
    const Voice&    voice(int i) const { return v_[i < 0 ? 0 : (i >= m_ ? m_ - 1 : i)]; }
    const PoolSlot& slot(int i) const  { return s_[i < 0 ? 0 : (i >= m_ ? m_ - 1 : i)]; }
    Tuning          tuning() const     { return tuning_; }
    const Board&    board() const      { return board_; }   // the plate: amplitude(k), modeInfo(k), shape(x, y)
    static uint32_t seedOf(uint32_t seed, int i);  // voice i's seed in a pool of this seed: distinct, never zero

    // ---- the engine ---------------------------------------------------------------------------
    void render(float* out, int n);                // the board on the voices' forces, plus their pickups, each at its level

private:
    friend struct PoolTestAccess;
    // the one render: with kOne, voice `which`'s bridge force and pickup beside it, each at the level it is
    // summed at (the tests' window); either may be null
    template <bool kOne> void renderWith(float* out, int n, int which, float* oneBridge, float* onePickup);
    void block();
    void land(int i, float key, float vel, bool off);   // a note on a sleeping voice, whole
    void steal(int i, float key, float vel);
    int  quietest() const;
    int  oldest(PoolSlot::Use use) const;
    void plate(float area, float loss, float open);     // the board's plate from the written body (a nan keeps its own)

    Voice*        v_;
    PoolSlot*     s_;
    PoolLane*     lane_;
    const float** forces_;
    const float** weights_;
    int           m_;
    float         fs_;
    Tuning        tuning_;
    Board         board_;
    float         area_ = 1.0f, loss_ = 0.03f, open_ = 0.5f;   // the plate as written (the default shape's)
    float         fadeK_ = 0.0f;                   // e^(-stealRate dt): a stolen voice's fade a block
    float         pedal_ = 0.0f;                   // the pool's pedal: the one a landing note takes
    uint32_t      clock_ = 0;
    int           phase_ = 0;
    float         pickup_[kBlock];                 // one voice's pickup, before the sum
    float         picks_[kBlock];                  // the pickups summed
    float         boardOut_[kBlock];               // the board's output
};

// the storage a Pool<M> holds, ahead of its PoolBase (bases are built in
// order, so the voices exist when the pool starts them)
template <int M>
struct PoolStorage {
    static_assert(M >= 1, "a pool holds at least one voice");
    Voice        voices[M];
    PoolSlot     slots[M];
    PoolLane     lanes[M];
    const float* forces[M];
    const float* weights[M];
};

// m voices, exactly: M * sizeof(Voice), their slots and lanes, the board and the pool's own state
template <int M>
class Pool : private PoolStorage<M>, public PoolBase {
public:
    explicit Pool(uint32_t seed = 1, float fs = kDefaultFs)
        : PoolStorage<M>(),
          PoolBase(PoolStorage<M>::voices, PoolStorage<M>::slots, PoolStorage<M>::lanes, PoolStorage<M>::forces, PoolStorage<M>::weights, M, seed,
                   fs) {}
    Pool(const Pool&) = delete;
    Pool& operator=(const Pool&) = delete;
};

}  // namespace up
