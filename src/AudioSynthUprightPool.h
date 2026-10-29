// upright — AudioSynthUprightPool.h  (plan §14, §13 the pool)
// m voices and one board for the teensy audio library, one piano on a
// keyboard: an AudioStream with no inputs and one output, 0, the board on
// the voices' bridge forces with their pickups added (the pool's render). it
// owns an up::Pool<M> (m voices, their note table, the board), a command
// queue and one scratch block, all inside the object: nothing on the heap. a
// class template on M, so it lives whole in this header (the one place the
// house's .h/.cpp rule bends; this header holds only the template and
// includes the core).
//
// the whole file, includes too, sits inside #if defined(ARDUINO): on the host
// this header is empty, and the tests and the tool use up::Pool directly.
//
// the handoff:
//   setters   push one command each onto a fixed queue (64), inside an
//             interrupt mask (the caller's mask put back as it was): a note
//             on or off, the pedal, all notes off, a channel or a rate for
//             every voice, one partial by hand, and a shape, a knob vector, a
//             partial table or a tuning, whose payloads wait in small rings
//             beside the queue (a second of the same kind straight after the
//             first merges into it: nothing ran between them). a command
//             carries a 16-bit index (a channel, or a partial's slot) and four
//             floats (setPartial's). commands keep their order: a note lands
//             on the writes made before it, as on the host. nothing is lost:
//             a setter that finds no room waits, unmasked, for update() to
//             drain the queue (a block, 2.9 ms), and gives up only after
//             kWaitUs, counting it in dropped() (an audio library that never
//             runs update(), or a setter called with interrupts off, which
//             cannot wait). call it from the main loop, or from an interrupt
//             below the audio library's: one above it would wait the whole
//             kWaitUs and drop, update() unable to run
//   update()  drains the queue into the pool in order with interrupts
//             masked (a knob vector's pianoOf runs there, once per vector,
//             and the board's table once per body write: kBoardModes
//             frequencies from the mode pairs found once, no grid search;
//             the moved modes' turns follow in the render), then renders the
//             block unmasked, clamps the sum, scales it to int16 and sends
//             it; when allocate() returns NULL the block is rendered all the
//             same, so the pool keeps time
//   readback  state(i), slot(i) and sounding() copy under the same mask, as
//             of the end of the last block
//
// denormals: as AudioSynthUpright, flush-to-zero in FPDSCR and the thread's
// FPSCR at construction.
#pragma once

#if defined(ARDUINO)

#include <Arduino.h>
#include <AudioStream.h>

#include "core/up_channels.h"
#include "core/up_knobs.h"
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
    bool setKnobs(const up::Knobs& k);               // pianoOf, then setShape (pianoOf runs in update())
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
    enum class Op : uint8_t { noteOn, noteOff, pedal, allOff, set, rate, partial, shape, knobs, partials, tuning };
    struct Command {
        Op       op;
        uint16_t index;                              // a channel, or a partial's slot
        float    a, b, c, d;
    };
    static constexpr int      kQueue = 64, kShapes = 2, kKnobs = 4, kTables = 2, kTunings = 2;
    static constexpr uint32_t kWaitUs = 20000;       // a setter waits this long for room at most

    // interrupts off for the length of a scope, and back as the caller had them
    class Masked {
    public:
        Masked() {
            __asm__ volatile("mrs %0, primask" : "=r"(primask_) : : "memory");
            __disable_irq();
        }
        ~Masked() {
            if ((primask_ & 1u) == 0u) __enable_irq();
        }
        Masked(const Masked&) = delete;
        Masked& operator=(const Masked&) = delete;

    private:
        uint32_t primask_ = 0;
    };

    // a payload ring: filled in the queue's order, taken in it
    template <typename T, int N>
    struct Ring {
        T   item[N];
        int head = 0, count = 0;
        bool full() const { return count == N; }
        T&   last() { return item[(head + count - 1) % N]; }
        void put(const T& x) { item[(head + count++) % N] = x; }
        const T& take() {
            const T& x = item[head];
            head = (head + 1) % N;
            --count;
            return x;
        }
    };

    static void flushDenormals();
    static bool interruptsOff();
    // one command, with its payload written by `fill` once room is found;
    // `merge` folds a payload into the last one of its kind when that
    // command is the queue's last
    template <typename Fill, typename Merge>
    bool push(const Command& c, Fill fill, Merge merge);
    bool pushSmall(const Command& c);
    void apply(const Command& c);
    void send(const float* x);

    up::Pool<M> pool_;
    Command     queue_[kQueue];
    int         head_ = 0, count_ = 0;
    Ring<up::Shape, kShapes>         shapes_;
    Ring<up::Knobs, kKnobs>          knobs_;
    Ring<up::PartialTable, kTables>  tables_;
    Ring<up::Tuning, kTunings>       tunings_;
    uint32_t    dropped_ = 0;
    float       out_[AUDIO_BLOCK_SAMPLES];
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
template <typename Fill, typename Merge>
bool AudioSynthUprightPool<M>::push(const Command& c, Fill fill, Merge merge) {
    const bool canWait = !interruptsOff();
    const uint32_t start = ARM_DWT_CYCCNT;
    const uint32_t waitCycles = (uint32_t)((uint64_t)F_CPU_ACTUAL * kWaitUs / 1000000u);
    for (;;) {
        {
            Masked m;
            if (merge(count_ > 0 && queue_[(head_ + count_ - 1) % kQueue].op == c.op)) return true;
            if (count_ < kQueue && fill()) {
                queue_[(head_ + count_) % kQueue] = c;
                ++count_;
                return true;
            }
            if (!canWait || ARM_DWT_CYCCNT - start > waitCycles) {
                ++dropped_;
                return false;
            }
        }
        delayMicroseconds(50);                       // unmasked: update() drains meanwhile
    }
}

template <int M>
bool AudioSynthUprightPool<M>::pushSmall(const Command& c) {
    return push(c, [] { return true; }, [](bool) { return false; });
}

// ---- the setters ------------------------------------------------------------------------------------

template <int M>
bool AudioSynthUprightPool<M>::noteOn(float midi, float vel01) { return pushSmall({Op::noteOn, 0, midi, vel01, 0.0f, 0.0f}); }
template <int M>
bool AudioSynthUprightPool<M>::noteOff(float midi) { return pushSmall({Op::noteOff, 0, midi, 0.0f, 0.0f, 0.0f}); }
template <int M>
bool AudioSynthUprightPool<M>::pedal(float p01) { return pushSmall({Op::pedal, 0, p01, 0.0f, 0.0f, 0.0f}); }
template <int M>
bool AudioSynthUprightPool<M>::allNotesOff() { return pushSmall({Op::allOff, 0, 0.0f, 0.0f, 0.0f, 0.0f}); }
template <int M>
bool AudioSynthUprightPool<M>::set(up::Channel c, float target) { return pushSmall({Op::set, (uint16_t)c, target, 0.0f, 0.0f, 0.0f}); }
template <int M>
bool AudioSynthUprightPool<M>::setRate(up::Channel c, float perSec) { return pushSmall({Op::rate, (uint16_t)c, perSec, 0.0f, 0.0f, 0.0f}); }
template <int M>
bool AudioSynthUprightPool<M>::setPartial(int k, float ratio, float t60, float gain, float out) {
    if (k < 0 || k >= up::kMaxModes) return true;
    return pushSmall({Op::partial, (uint16_t)k, ratio, t60, gain, out});
}

template <int M>
bool AudioSynthUprightPool<M>::setShape(const up::Shape& s) {
    return push(
        {Op::shape, 0, 0.0f, 0.0f, 0.0f, 0.0f},
        [&] {
            if (shapes_.full()) return false;
            shapes_.put(s);
            return true;
        },
        [&](bool lastIsShape) {
            if (!lastIsShape) return false;
            up::Shape& to = shapes_.last();
            for (int i = 0; i < up::kChannelCount; ++i)
                if (s.v[i] == s.v[i]) to.v[i] = s.v[i];
            return true;
        });
}

template <int M>
bool AudioSynthUprightPool<M>::setKnobs(const up::Knobs& k) {
    return push(
        {Op::knobs, 0, 0.0f, 0.0f, 0.0f, 0.0f},
        [&] {
            if (knobs_.full()) return false;
            knobs_.put(k);
            return true;
        },
        [&](bool lastIsKnobs) {
            if (!lastIsKnobs) return false;
            knobs_.last() = k;
            return true;
        });
}

template <int M>
bool AudioSynthUprightPool<M>::setPartials(const up::PartialTable& t) {
    return push(
        {Op::partials, 0, 0.0f, 0.0f, 0.0f, 0.0f},
        [&] {
            if (tables_.full()) return false;
            tables_.put(t);
            return true;
        },
        [&](bool lastIsTable) {
            if (!lastIsTable) return false;
            up::PartialTable& to = tables_.last();
            for (int k = 0; k < up::kMaxModes; ++k) {
                if (t.ratio[k] == t.ratio[k]) to.ratio[k] = t.ratio[k];
                if (t.t60[k] == t.t60[k]) to.t60[k] = t.t60[k];
                if (t.gain[k] == t.gain[k]) to.gain[k] = t.gain[k];
                if (t.out[k] == t.out[k]) to.out[k] = t.out[k];
            }
            return true;
        });
}

template <int M>
bool AudioSynthUprightPool<M>::setTuning(const up::Tuning& t) {
    return push(
        {Op::tuning, 0, 0.0f, 0.0f, 0.0f, 0.0f},
        [&] {
            if (tunings_.full()) return false;
            tunings_.put(t);
            return true;
        },
        [&](bool lastIsTuning) {
            if (!lastIsTuning) return false;
            tunings_.last() = t;
            return true;
        });
}

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
void AudioSynthUprightPool<M>::apply(const Command& c) {
    switch (c.op) {
        case Op::noteOn:   pool_.noteOn(c.a, c.b); break;
        case Op::noteOff:  pool_.noteOff(c.a); break;
        case Op::pedal:    pool_.pedal(c.a); break;
        case Op::allOff:   pool_.allNotesOff(); break;
        case Op::set:      pool_.set((up::Channel)c.index, c.a); break;
        case Op::rate:     pool_.setRate((up::Channel)c.index, c.a); break;
        case Op::partial:  pool_.setPartial((int)c.index, c.a, c.b, c.c, c.d); break;
        case Op::shape:    pool_.setShape(shapes_.take()); break;
        case Op::knobs:    pool_.setKnobs(knobs_.take()); break;
        case Op::partials: pool_.setPartials(tables_.take()); break;
        case Op::tuning:   pool_.setTuning(tunings_.take()); break;
    }
}

template <int M>
void AudioSynthUprightPool<M>::update(void) {
    {
        Masked m;
        while (count_ > 0) {
            apply(queue_[head_]);
            head_ = (head_ + 1) % kQueue;
            --count_;
        }
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
