// upright — AudioSynthUprightPool.h
// m voices and one board for the teensy audio library, one piano on a
// keyboard: an AudioStream with no inputs and one output, 0, the board on
// the voices' bridge forces with their pickups added (the pool's render). it
// owns an up::Pool<M> (m voices, their note table, the board), an
// up::PoolQueue (the calls waiting for the next block) and one scratch
// block, all inside the object: nothing on the heap. a class template on M,
// so it lives whole in this header (the one place the house's .h/.cpp rule
// bends; this header holds only the template).
//
// the whole file, includes too, sits inside #if defined(ARDUINO): on the host
// this header is empty, and the tests and the tool use up::Pool and
// up::PoolQueue directly.
//
// the handoff:
//   setters   queue one command each on the up::PoolQueue (64), inside an
//             interrupt mask (the caller's mask put back as it was): a note
//             on or off, the pedal, all notes off, a channel or a rate for
//             every voice, one partial by hand, and a shape, a partial
//             table or a tuning, whose payloads wait in small rings beside
//             the queue (a partial table or a tuning straight after one of
//             its kind merges into it; a shape never does, up_pool.h says
//             why). commands keep their order: a note lands on the writes
//             made before it, as on the host. nothing is lost:
//             a setter that finds no room waits, unmasked, for update() to
//             drain the queue (a block, 2.9 ms), and gives up only after
//             kWaitUs, counting it in dropped() (an audio library that never
//             runs update(), or a setter called with interrupts off, which
//             cannot wait). call it from the main loop, or from an interrupt
//             below the audio library's: one above it would wait the whole
//             kWaitUs and drop, update() unable to run
//   update()  drains the queue into the pool in order with interrupts
//             masked (the board's table runs there once per body write:
//             kBoardModes frequencies from the mode pairs found once, no grid
//             search; the moved modes' turns follow in the render), then
//             renders the block unmasked, clamps the sum, scales it to int16
//             and sends it; when allocate() returns NULL the block is
//             rendered all the same, so the pool keeps time
//   readback  state(i), slot(i) and sounding() copy under the same mask, as
//             of the end of the last block
//
// denormals: as AudioSynthUpright, flush-to-zero in FPDSCR and the thread's
// FPSCR at construction.
#pragma once

#if defined(ARDUINO)

#include <Arduino.h>
#include <AudioStream.h>

#include "AudioSynthUpright.h"                     // up::Masked
#include "core/up_channels.h"
#include "core/up_pool.h"
#include "core/up_telemetry.h"
#include "core/up_tuning.h"
#include "core/up_voice.h"

template <int M>
class AudioSynthUprightPool : public AudioStream {
public:
    // the pool's seed (each voice's follows from it). allocate statically
    // and never destroy, as every AudioStream
    explicit AudioSynthUprightPool(uint32_t seed = 1);

    // ---- the player (each returns false only when its command was dropped) -----------------
    bool noteOn(float midi, float vel01);            // the key's own voice again, else a free one, else the quietest stolen
    bool noteOff(float midi);                        // the key released: its damper lands (unless the pedal holds it)
    bool pedal(float p01);                           // the sustain pedal on every voice, and on the notes that land under it
    bool allNotesOff();

    // ---- every voice, and later notes -------------------------------------------------------
    bool set(up::Channel c, float target);
    bool setRate(up::Channel c, float perSec);
    bool setShape(const up::Shape& s);               // a nan entry is no write
    bool setPartial(int k, float ratio, float t60, float gain, float out);   // one partial by hand
    bool setPartials(const up::PartialTable& t);     // a measured string, whole (a nan skips)
    bool setTuning(const up::Tuning& t);

    // ---- readback, as of the end of the last block --------------------------------------------
    static constexpr int voices() { return M; }
    up::State    state(int voice) const;
    up::PoolSlot slot(int voice) const;
    int          sounding() const;                   // voices awake
    uint32_t     dropped() const;                    // commands given up on (see above)

    // the audio interrupt: drain the queue (masked), render one block
    // (unmasked), clamp, scale to int16, transmit the sum on output 0
    void update(void) override;

private:
    using Masked = up::Masked;
    static constexpr uint32_t kWaitUs = 20000;       // a setter waits this long for room at most

    static void flushDenormals();
    static bool interruptsOff();
    // one command queued by `queue` (one of up::PoolQueue's setters, false
    // when it finds no room), masked; waits for room as above
    template <typename Queue>
    bool push(Queue queue);
    void send(const float* x);

    up::Pool<M>   pool_;
    up::PoolQueue queue_;
    uint32_t      dropped_ = 0;
    float         out_[AUDIO_BLOCK_SAMPLES];
};

// ---- construction ---------------------------------------------------------------------------------
// flushDenormals is in flash. the constructor is not: gcc emits a template's
// constructors as aliases of one body, and an alias must sit in its
// target's section, which FLASHMEM on a template constructor breaks (an
// error, not a warning)

template <int M>
FLASHMEM void AudioSynthUprightPool<M>::flushDenormals() {
    SCB_FPDSCR |= (1u << 24);
    __builtin_arm_set_fpscr(__builtin_arm_get_fpscr() | (1u << 24));
}

template <int M>
AudioSynthUprightPool<M>::AudioSynthUprightPool(uint32_t seed) : AudioStream(0, NULL), pool_(seed, AUDIO_SAMPLE_RATE_EXACT) {
    flushDenormals();
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
            if (queue()) return true;
            if (!canWait || ARM_DWT_CYCCNT - start > waitCycles) {
                ++dropped_;
                return false;
            }
        }
        delayMicroseconds(50);                       // unmasked: update() drains meanwhile
    }
}

// ---- the setters ------------------------------------------------------------------------------------

template <int M>
bool AudioSynthUprightPool<M>::noteOn(float midi, float vel01) { return push([&] { return queue_.noteOn(midi, vel01); }); }
template <int M>
bool AudioSynthUprightPool<M>::noteOff(float midi) { return push([&] { return queue_.noteOff(midi); }); }
template <int M>
bool AudioSynthUprightPool<M>::pedal(float p01) { return push([&] { return queue_.pedal(p01); }); }
template <int M>
bool AudioSynthUprightPool<M>::allNotesOff() { return push([&] { return queue_.allNotesOff(); }); }
template <int M>
bool AudioSynthUprightPool<M>::set(up::Channel c, float target) { return push([&] { return queue_.set(c, target); }); }
template <int M>
bool AudioSynthUprightPool<M>::setRate(up::Channel c, float perSec) { return push([&] { return queue_.setRate(c, perSec); }); }
template <int M>
bool AudioSynthUprightPool<M>::setPartial(int k, float ratio, float t60, float gain, float out) {
    if (k < 0 || k >= up::kMaxModes) return true;
    return push([&] { return queue_.setPartial(k, ratio, t60, gain, out); });
}
template <int M>
bool AudioSynthUprightPool<M>::setShape(const up::Shape& s) { return push([&] { return queue_.setShape(s); }); }
template <int M>
bool AudioSynthUprightPool<M>::setPartials(const up::PartialTable& t) { return push([&] { return queue_.setPartials(t); }); }
template <int M>
bool AudioSynthUprightPool<M>::setTuning(const up::Tuning& t) { return push([&] { return queue_.setTuning(t); }); }

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
    {
        Masked m;
        queue_.drain(pool_);
    }
    pool_.render(out_, AUDIO_BLOCK_SAMPLES);
    send(out_);
}

// the sum's block: clamped, scaled to int16, sent on output 0. a failed
// allocate drops this block only
template <int M>
void AudioSynthUprightPool<M>::send(const float* x) {
    audio_block_t* block = allocate();
    if (!block) return;
    for (int i = 0; i < AUDIO_BLOCK_SAMPLES; ++i) {
        float y = x[i];
        if (y > 1.0f) y = 1.0f;
        if (y < -1.0f) y = -1.0f;
        block->data[i] = (int16_t)(y * 32767.0f);
    }
    transmit(block, 0);
    release(block);
}

#endif  // defined(ARDUINO)
