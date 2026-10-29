// upright — AudioSynthUpright.h  (plan §14, §11 the public surface)
// one voice for the teensy audio library: an AudioStream with no inputs and
// one output, 0: the voice's bridge force through a board of its own, its
// pickup's voltage added (as uptool play renders a note). it owns an
// up::Voice, the pending table the voice is attached to, an up::Board and
// its scratch, all inside the object: nothing on the heap. its calls are the
// voice's (src/core/up_voice.h), one for one.
//
// the whole file, includes too, sits inside #if defined(ARDUINO): on the host
// this header is empty, and the tests and the tool use up::Voice directly.
//
// the handoff:
//   setters   write the pending table (an up::Channels: every target and
//             rate under one running sequence), the tuning, a queued strike
//             and the key's release, inside an interrupt mask, so a call that
//             writes several channels (setShape, setPartials, setHammer) is
//             never seen half done. the caller's mask is put back as it was,
//             so a setter called with interrupts already off leaves them off
//   update()  latches the table with interrupts masked: every target and
//             rate newer than the voice's, by the sequence, so the last
//             writer wins per channel exactly as on the host; nothing when
//             the sequence has not moved. then, unmasked, it renders the
//             voice's pair, runs the board on the bridge force (its plate
//             from the voice's eased body channels when they moved, its
//             weights at the voice's bridge point when the note moved), adds
//             the pickup, clamps, scales to int16 and sends it
//   readback  state(), shape(), partials(), stringShape() and rank() copy
//             under the same mask, as of the end of the last block
// a call lands at the top of the next block (128 samples, 2.9 ms): one
// latency, for everything, a strike too.
//
// the plate's table (up_board.cpp's boardOf) is rebuilt in update() whenever
// the eased body channels moved: a body write eases at rateBody, so the table
// is rebuilt every block for a while after one. a rebuild is kBoardModes
// frequencies, decays and radiations from the mode pairs found once, the
// field's loss a line, and the moved modes' turns at the board's next block:
// about a twentieth of a voice's block on the host, never the grid search.
//
// denormals: update() runs in an interrupt, whose floating-point context is
// loaded from FPDSCR, not from the thread's FPSCR; the constructor sets
// flush-to-zero in both. the voice's silence gate puts a rung-out string to
// sleep before its phasors would reach denormal range anyway.
#pragma once

#if defined(ARDUINO)

#include <Arduino.h>
#include <AudioStream.h>

#include "core/up_board.h"
#include "core/up_channels.h"
#include "core/up_rank.h"
#include "core/up_telemetry.h"
#include "core/up_tuning.h"
#include "core/up_voice.h"

class AudioSynthUpright : public AudioStream {
public:
    // one seeded xorshift32 per voice, drawn by nothing in the engine: any
    // seed renders the same bytes. allocate statically and never destroy, as
    // every AudioStream
    explicit AudioSynthUpright(uint32_t seed = 1);

    // ---- the player ---------------------------------------------------------------------
    void noteOn(float midi, float vel01);    // the note, and a hammer at the next block
    void noteOff();                          // the key released: the damper lands (unless the pedal holds it, or the key has none)
    void setPitch(float midi);
    void setGlide(float perSec);
    void setPedal(float p01);                // the sustain pedal, eased at ratePedal

    // ---- the instrument: every channel, by name ---------------------------------------------
    void      set(up::Channel c, float target);
    void      setRate(up::Channel c, float perSec);
    void      setShape(const up::Shape& s);  // every target at once; a nan entry is no write
    up::Shape shape() const;                 // the targets as held

    // ---- conveniences that write channels -----------------------------------------------------
    void setStrings(float length, float bass, float tension, float gauge);
    void setUnison(float strings, float detune);
    void setHammer(float hardness, float exponent, float felt, float massG);
    void setStrike(float share);
    void setBody(float area, float loss, float open);
    void setGenerator(float g, float gap, float offset);
    void setDamper(float rate, float top);
    void setPartial(int k, float ratio, float t60, float gain, float out);   // one partial by hand
    void setPartials(const up::PartialTable& t);   // a measured string, whole (a nan skips)

    // ---- who is playing ---------------------------------------------------------------------------
    void       setTuning(const up::Tuning& t);
    up::Tuning tuning() const;               // the last one asked for, as clamped

    // ---- readback, as of the end of the last block ----------------------------------------------
    up::State     state() const;
    int           partials(up::PartialTable& out) const;
    int           stringShape(float* y, int n) const;
    up::RankState rank() const;

    // the audio interrupt: latch (masked), render one block (unmasked), the
    // board on the bridge, the pickup added, clamp, scale to int16, transmit
    // it on output 0. when allocate() returns NULL the block is rendered all
    // the same, so the voice and its board keep time
    void update(void) override;

private:
    static void flushDenormals();
    void send(const float* x);

    up::Pending pending_;
    up::Voice   voice_;
    up::Board   board_;
    float       weights_[up::kBoardModes];
    float       point_ = -1.0f;              // the note the weights are at
    float       bridge_[AUDIO_BLOCK_SAMPLES], pickup_[AUDIO_BLOCK_SAMPLES], out_[AUDIO_BLOCK_SAMPLES];
};

#endif  // defined(ARDUINO)
