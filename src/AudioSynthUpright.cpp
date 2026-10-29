// upright — AudioSynthUpright.cpp  (plan §14)
// the header holds the whole file inside #if defined(ARDUINO); on the host
// this translation unit is empty.
#include "AudioSynthUpright.h"

#if defined(ARDUINO)

namespace {

constexpr uint32_t kFlushToZero = 1u << 24;          // FPSCR and FPDSCR: FZ

// interrupts off for the length of a scope, and back as the caller had them:
// PRIMASK is read first, and only re-enabled when it was clear
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

}  // namespace

// the voice runs its control once per up::kBlock samples whatever size it is
// rendered at, and a latch only moves targets, which the voice reads at the
// top of its next block; so a build with another AUDIO_BLOCK_SAMPLES still
// lands every call, a block of 128 later at most
static_assert(AUDIO_BLOCK_SAMPLES > 0, "AUDIO_BLOCK_SAMPLES must be positive");

// init-only, in flash. the thread's FPSCR covers setup() and loop(); FPDSCR
// is what an interrupt's floating-point context starts from, update()'s
FLASHMEM void AudioSynthUpright::flushDenormals() {
    SCB_FPDSCR |= kFlushToZero;
    __builtin_arm_set_fpscr(__builtin_arm_get_fpscr() | kFlushToZero);
}

FLASHMEM AudioSynthUpright::AudioSynthUpright(uint32_t seed) : AudioStream(0, NULL), voice_(seed, AUDIO_SAMPLE_RATE_EXACT) {
    flushDenormals();
    voice_.attach(&pending_);
    board_.init(AUDIO_SAMPLE_RATE_EXACT);
    board_.inputWeights(up::kScaleMidi, weights_);
    for (int i = 0; i < AUDIO_BLOCK_SAMPLES; ++i) bridge_[i] = pickup_[i] = out_[i] = 0.0f;
}

// ---- the setters: the pending table, masked ----------------------------------------------------

void AudioSynthUpright::noteOn(float midi, float vel01) { Masked m; voice_.noteOn(midi, vel01); }
void AudioSynthUpright::noteOff() { Masked m; voice_.noteOff(); }
void AudioSynthUpright::setPitch(float midi) { Masked m; voice_.setPitch(midi); }
void AudioSynthUpright::setGlide(float perSec) { Masked m; voice_.setGlide(perSec); }
void AudioSynthUpright::setPedal(float p01) { Masked m; voice_.setPedal(p01); }

void AudioSynthUpright::set(up::Channel c, float target) { Masked m; voice_.set(c, target); }
void AudioSynthUpright::setRate(up::Channel c, float perSec) { Masked m; voice_.setRate(c, perSec); }
void AudioSynthUpright::setShape(const up::Shape& s) { Masked m; voice_.setShape(s); }

void AudioSynthUpright::setStrings(float length, float bass, float tension, float gauge) {
    Masked m;
    voice_.setStrings(length, bass, tension, gauge);
}
void AudioSynthUpright::setUnison(float strings, float detune) { Masked m; voice_.setUnison(strings, detune); }
void AudioSynthUpright::setHammer(float hardness, float exponent, float felt, float massG) {
    Masked m;
    voice_.setHammer(hardness, exponent, felt, massG);
}
void AudioSynthUpright::setStrike(float share) { Masked m; voice_.setStrike(share); }
void AudioSynthUpright::setBody(float area, float loss, float open) { Masked m; voice_.setBody(area, loss, open); }
void AudioSynthUpright::setGenerator(float g, float gap, float offset) { Masked m; voice_.setGenerator(g, gap, offset); }
void AudioSynthUpright::setDamper(float rate, float top) { Masked m; voice_.setDamper(rate, top); }
void AudioSynthUpright::setPartial(int k, float ratio, float t60, float gain, float out) {
    Masked m;
    voice_.setPartial(k, ratio, t60, gain, out);
}
void AudioSynthUpright::setPartials(const up::PartialTable& t) { Masked m; voice_.setPartials(t); }

void AudioSynthUpright::setTuning(const up::Tuning& t) { Masked m; voice_.setTuning(t); }

// the pending tuning is written only by the setters, on this side: no mask
up::Tuning AudioSynthUpright::tuning() const { return voice_.tuning(); }

// ---- readback: the same mask, the other way ---------------------------------------------------------

up::Shape AudioSynthUpright::shape() const {
    Masked m;
    return voice_.shape();
}

up::State AudioSynthUpright::state() const {
    Masked m;
    return voice_.state();
}

int AudioSynthUpright::partials(up::PartialTable& out) const {
    Masked m;
    return voice_.partials(out);
}

int AudioSynthUpright::stringShape(float* y, int n) const {
    Masked m;
    return voice_.stringShape(y, n);
}

up::RankState AudioSynthUpright::rank() const {
    Masked m;
    return voice_.rank();
}

// ---- the audio interrupt -----------------------------------------------------------------------------

void AudioSynthUpright::update(void) {
    {
        Masked m;
        voice_.latch();
    }
    voice_.render(bridge_, pickup_, AUDIO_BLOCK_SAMPLES);
    const up::State& st = voice_.state();
    board_.setPlate(st.shape[up::Channel::boardArea], st.shape[up::Channel::boardLoss], st.shape[up::Channel::open]);
    const float point = voice_.bridgePoint();
    if (point != point_) {
        point_ = point;
        board_.inputWeights(point, weights_);
    }
    const float* forces[1] = {bridge_};
    const float* weights[1] = {weights_};
    board_.render(forces, weights, 1, out_, AUDIO_BLOCK_SAMPLES);
    for (int i = 0; i < AUDIO_BLOCK_SAMPLES; ++i) out_[i] += pickup_[i];
    send(out_);
}

// the output's block: clamped, scaled to int16, sent on output 0. a failed
// allocate drops this block only
void AudioSynthUpright::send(const float* x) {
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
