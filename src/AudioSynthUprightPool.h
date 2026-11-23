// upright — AudioSynthUprightPool.h
// m voices and one board for the teensy audio library, one piano on a
// keyboard: an AudioStream with no inputs and one output, 0, the board on
// the voices' bridge forces with their pickups added (the pool's render). it
// owns an up::Pool<M> (m voices, their note table, the board), two
// up::PoolQueues (the calls waiting for the next block) and one scratch
// block, all inside the object: nothing on the heap. a class template on M,
// so its members are defined in this header, not in a .cpp beside it as the
// voice wrapper's are (this header holds only the template).
//
// the whole file, includes too, sits inside #if defined(ARDUINO) and
// __has_include(<AudioStream.h>), as AudioSynthUpright.h: elsewhere (the
// host, another arduino core) this header is empty, and the tests and the
// tool use up::Pool and up::PoolQueue directly.
//
// the handoff:
//   setters   queue one command each on the up::PoolQueue being written
//             (64), inside an interrupt mask (the caller's mask put back as
//             it was): a note on or off, the pedal, all notes off, a channel
//             or a rate for every voice, one partial by hand, and a shape, a
//             partial table or a tuning, whose payloads wait in small rings
//             beside the queue (a partial table or a tuning straight after
//             one of its kind merges into it; a shape never does, up_pool.h
//             says why). commands keep their order: a note lands on the
//             writes made before it, as on the host. nothing is lost:
//             a setter that finds no room waits, unmasked, for update() to
//             take the queue (a block, 2.9 ms), and gives up only after
//             kWaitUs, counting it in dropped() (an audio library that never
//             runs update(), or a setter called with interrupts off, which
//             cannot wait). call it from the main loop, or from an interrupt
//             below the audio library's: one above it would wait the whole
//             kWaitUs and drop, update() unable to run
//   update()  with interrupts masked, turns the setters to the other queue
//             (an index: nothing is copied); then, unmasked, drains the queue
//             they wrote into the pool in order (the board's table runs there
//             once per body write: kBoardModes frequencies from the mode pairs
//             found once, no grid search; the moved modes' turns follow in the
//             render), renders the block, clamps the sum, scales it to int16
//             and sends it; when allocate() returns NULL the block is
//             rendered all the same, so the pool keeps time. no setter writes
//             the queue being drained: they write the other until the next
//             update(), and update() never preempts itself, so the drain
//             needs no mask and its queue is empty when the setters come back
//             to it
//   readback  state(i), slot(i) and sounding() copy under the same mask, as
//             of the end of the last block
//
// denormals: as AudioSynthUpright, flush-to-zero in FPDSCR and the thread's
// FPSCR at construction (up::Wrapper).
#pragma once

#if defined(ARDUINO) && __has_include(<AudioStream.h>)

#include <Arduino.h>
#include <AudioStream.h>

#include "AudioSynthUpright.h"                     // up::Masked, up::Wrapper
#include "core/up_channels.h"
#include "core/up_pool.h"
#include "core/up_telemetry.h"
#include "core/up_tuning.h"
#include "core/up_voice.h"

template <int M>
class AudioSynthUprightPool : public up::Wrapper {
public:
    // allocate statically and never destroy, as every AudioStream
    AudioSynthUprightPool();

    // ---- the player (each returns false only when its command was dropped) -----------------
    bool noteOn(float midi, float vel01);            // the key's own voice again, else a free one, else the quietest stolen
    bool noteOff(float midi);                        // the key released: its damper lands (unless the pedal holds it)
    bool setPedal(float p01);                        // the sustain pedal on every voice, and on the notes that land under it
    bool allNotesOff();

    // ---- every voice, and later notes -------------------------------------------------------
    bool set(up::Channel c, float target);
    bool setRate(up::Channel c, float perSec);
    bool setShape(const up::Shape& s);               // a nan or an infinite entry is no write
    bool setPartial(int k, float ratio, float t60, float gain, float out);   // one partial by hand
    bool setPartials(const up::PartialTable& t);     // a measured string, whole (a nan or an infinity skips)
    bool setTuning(const up::Tuning& t);

    // ---- readback, as of the end of the last block --------------------------------------------
    static constexpr int voices() { return M; }
    up::State    state(int voice) const;
    up::PoolSlot slot(int voice) const;
    int          sounding() const;                   // voices awake
    uint32_t     dropped() const;                    // commands given up on (see above)

    // the audio interrupt: the setters turned to the other queue (masked),
    // the one they wrote drained and one block rendered (unmasked), clamp,
    // scale to int16, transmit the sum on output 0
    void update(void) override;

private:
    using Masked = up::Masked;
    static constexpr uint32_t kWaitUs = 20000;       // a setter waits this long for room at most

    static bool interruptsOff();
    // one command queued by `queue` (one of up::PoolQueue's setters on the
    // queue it is given, false when it finds no room), masked, on the queue
    // being written; waits for room as above
    template <typename Queue>
    bool push(Queue queue);

    up::Pool<M>   pool_;
    up::PoolQueue queues_[2];
    int           writing_ = 0;                      // the queue the setters write; the other is update()'s
    uint32_t      dropped_ = 0;
    float         out_[AUDIO_BLOCK_SAMPLES];
};

// ---- construction ---------------------------------------------------------------------------------
// up::Wrapper's constructor is in flash. this one is not: gcc emits a
// template's constructors as aliases of one body, and an alias must sit in
// its target's section, which FLASHMEM on a template constructor breaks (an
// error, not a warning)

template <int M>
AudioSynthUprightPool<M>::AudioSynthUprightPool() : pool_(AUDIO_SAMPLE_RATE_EXACT) {
    for (int i = 0; i < AUDIO_BLOCK_SAMPLES; ++i) out_[i] = 0.0f;
}

template <int M>
bool AudioSynthUprightPool<M>::interruptsOff() {
    uint32_t primask;
    __asm__ volatile("mrs %0, primask" : "=r"(primask) : : "memory");
    return (primask & 1u) != 0u;
}

// ---- the queue ------------------------------------------------------------------------------------

template <int M>
template <typename Queue>
bool AudioSynthUprightPool<M>::push(Queue queue) {
    const bool canWait = !interruptsOff();
    const uint32_t start = ARM_DWT_CYCCNT;
    const uint32_t waitCycles = (uint32_t)((uint64_t)F_CPU_ACTUAL * kWaitUs / 1000000u);
    for (;;) {
        {
            Masked m;
            if (queue(queues_[writing_])) return true;
            if (!canWait || ARM_DWT_CYCCNT - start > waitCycles) {
                ++dropped_;
                return false;
            }
        }
        delayMicroseconds(50);                       // unmasked: update() takes the queue meanwhile
    }
}

// ---- the setters ------------------------------------------------------------------------------------

template <int M>
bool AudioSynthUprightPool<M>::noteOn(float midi, float vel01) { return push([&](up::PoolQueue& q) { return q.noteOn(midi, vel01); }); }
template <int M>
bool AudioSynthUprightPool<M>::noteOff(float midi) { return push([&](up::PoolQueue& q) { return q.noteOff(midi); }); }
template <int M>
bool AudioSynthUprightPool<M>::setPedal(float p01) { return push([&](up::PoolQueue& q) { return q.setPedal(p01); }); }
template <int M>
bool AudioSynthUprightPool<M>::allNotesOff() { return push([&](up::PoolQueue& q) { return q.allNotesOff(); }); }
template <int M>
bool AudioSynthUprightPool<M>::set(up::Channel c, float target) { return push([&](up::PoolQueue& q) { return q.set(c, target); }); }
template <int M>
bool AudioSynthUprightPool<M>::setRate(up::Channel c, float perSec) { return push([&](up::PoolQueue& q) { return q.setRate(c, perSec); }); }
template <int M>
bool AudioSynthUprightPool<M>::setPartial(int k, float ratio, float t60, float gain, float out) {
    if (k < 0 || k >= up::kMaxModes) return true;
    return push([&](up::PoolQueue& q) { return q.setPartial(k, ratio, t60, gain, out); });
}
template <int M>
bool AudioSynthUprightPool<M>::setShape(const up::Shape& s) { return push([&](up::PoolQueue& q) { return q.setShape(s); }); }
template <int M>
bool AudioSynthUprightPool<M>::setPartials(const up::PartialTable& t) { return push([&](up::PoolQueue& q) { return q.setPartials(t); }); }
template <int M>
bool AudioSynthUprightPool<M>::setTuning(const up::Tuning& t) { return push([&](up::PoolQueue& q) { return q.setTuning(t); }); }

// ---- readback ------------------------------------------------------------------------------------------

template <int M>
up::State AudioSynthUprightPool<M>::state(int voice) const {
    Masked m;
    return pool_.voice(voice).state();
}
template <int M>
up::PoolSlot AudioSynthUprightPool<M>::slot(int voice) const {
    Masked m;
    return pool_.slot(voice);
}
template <int M>
int AudioSynthUprightPool<M>::sounding() const {
    Masked m;
    return pool_.sounding();
}
template <int M>
uint32_t AudioSynthUprightPool<M>::dropped() const {
    Masked m;
    return dropped_;
}

// ---- the audio interrupt ----------------------------------------------------------------------------------

template <int M>
void AudioSynthUprightPool<M>::update(void) {
    int drained;
    {
        Masked m;
        drained = writing_;
        writing_ = 1 - writing_;
    }
    queues_[drained].drain(pool_);
    pool_.render(out_, AUDIO_BLOCK_SAMPLES);
    send(out_);
}

#endif  // defined(ARDUINO) && __has_include(<AudioStream.h>)
