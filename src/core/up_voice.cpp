// upright — up_voice.cpp
// libm runs in block (the law and the rank through their own modules, the
// tine's tip and the strike's speed, each per block or per strike and only
// where its inputs moved; the attack's cosines through its own) and in
// stringShape and stringAt (the app's call rate, through up_telemetry.cpp);
// samples and run are multiplies, adds and the pickup's one division;
// publish takes a sqrt a partial.
#include "up_voice.h"

#include <cmath>
#include <cstring>

namespace up {
namespace {

constexpr float kNepers60 = 6.907755f;         // ln(1000): a t60 to a decay rate, 6.907755 / t60
constexpr float kThrowMax = 1000.0f;           // m/s: the most a strike throws the hammer, far past a fortissimo's 7

}  // namespace

Voice::Voice(uint32_t seed, float fs) : seed_(seed) {
    rng_.seed(seed);
    init(fs);
}

void Voice::init(float fs) {
    fs_ = fs > 1.0f ? fs : kDefaultFs;
    dt_ = (float)kBlock / fs_;
    pendingTuning_ = clamped(pendingTuning_);
    if (pending_) pending_->tuning = clamped(pending_->tuning);
    strings_.init(fs_);
    attack_.init(fs_);
    pitch_.init(fs_);
    pickup_.reset();
    law_.reset();
    lawNote_ = -1.0f;
    for (float& k : rankKey_) k = std::nanf("");
    for (float& k : hammerKey_) k = std::nanf("");
    tuningPending_ = true;
    phase_ = 0;
    strikeQueued_ = false;
    sleep();
}

// ---- the player -------------------------------------------------------------------------

void Voice::noteOn(float midi, float vel01) {
    set(Channel::midi, midi);
    const float v = std::isfinite(vel01) ? clamp(vel01, 0.0f, 1.0f) : 0.0f;
    if (pending_) {
        pending_->strike = true;
        pending_->vel = v;
        pending_->released = false;
        return;
    }
    strikeQueued_ = true;
    vel_ = v;
    released_ = false;
}

void Voice::noteOff() {
    if (pending_) pending_->released = true;
    else released_ = true;
}

void Voice::setPitch(float midi) { set(Channel::midi, midi); }
void Voice::setGlide(float perSec) { set(Channel::glide, perSec); }
void Voice::setPedal(float p01) { set(Channel::pedal, p01); }

// ---- the instrument ------------------------------------------------------------------------

void Voice::set(Channel c, float target) {
    if ((int)c < kChannelCount) writes().set(c, target);
}
void Voice::setRate(Channel c, float perSec) {
    if ((int)c < kChannelCount) writes().rate(c, perSec);
}
void Voice::setShape(const Shape& s) { writes().setShape(s); }

// attached, a target written since the last latch reads back over the voice's
Shape Voice::shape() const {
    Shape s = ch_.shape();
    if (pending_) {
        const Channels& p = pending_->ch;
        for (int i = 0; i < kChannelCount; ++i) {
            const Channel c = (Channel)i;
            if (p.seq(c) > ch_.seq(c)) s.v[i] = p.target(c);
        }
    }
    return s;
}

void Voice::setStrings(float length, float bass, float tension, float gauge) {
    set(Channel::length, length);
    set(Channel::bass, bass);
    set(Channel::tension, tension);
    set(Channel::gauge, gauge);
}

void Voice::setUnison(float strings, float detune) {
    set(Channel::strings, strings);
    set(Channel::detune, detune);
}

void Voice::setHammer(float hardness, float exponent, float felt, float massG) {
    set(Channel::hardness, hardness);
    set(Channel::exponent, exponent);
    set(Channel::felt, felt);
    set(Channel::hammerMass, massG);
}

void Voice::setStrike(float share) { set(Channel::strikeShare, share); }

void Voice::setBody(float area, float loss, float open) {
    set(Channel::boardArea, area);
    set(Channel::boardLoss, loss);
    set(Channel::open, open);
}

void Voice::setGenerator(float g, float gap, float offset) {
    set(Channel::generator, g);
    set(Channel::gap, gap);
    set(Channel::offset, offset);
}

void Voice::setDamper(float rate, float top) {
    set(Channel::dampRate, rate);
    set(Channel::damperTop, top);
}

// the four channels under one sequence: they stand until the instrument is written again
void Voice::setPartial(int k, float ratio, float t60, float gain, float out) {
    if (k < 0 || k >= kMaxModes) return;
    Shape s;
    for (int i = 0; i < kChannelCount; ++i) s.v[i] = std::nanf("");
    s[ratioChannel(k)] = ratio;
    s[t60Channel(k)] = t60;
    s[gainChannel(k)] = gain;
    s[outChannel(k)] = out;
    writes().setShape(s);
}

void Voice::setPartials(const PartialTable& t) {
    Shape s;
    for (int i = 0; i < kChannelCount; ++i) s.v[i] = std::nanf("");
    toShape(t, s);
    writes().setShape(s);
}

void Voice::setTuning(const Tuning& t) {
    if (pending_) {
        pending_->tuning = clamped(t);
        pending_->tuningSet = true;
        return;
    }
    pendingTuning_ = clamped(t);
    tuningPending_ = true;
}

// ---- readback ------------------------------------------------------------------------------

int Voice::partials(PartialTable& out) const {
    out = fromShape(state_.shape);
    return kMaxModes;
}

float Voice::stringAt(float share) const { return ::up::stringAt(re_, kMaxModes, state_.shape, share); }

int Voice::stringShape(float* y, int n) const {
    if (!y || n <= 0) return 0;
    for (int i = 0; i < n; ++i) y[i] = stringAt(n > 1 ? (float)i / (float)(n - 1) : 0.0f);
    return n;
}

// ---- the handoff ---------------------------------------------------------------------------

// the table starts as a copy of the voice's own, so the two share one
// sequence: a write there is newer than anything here
void Voice::attach(Pending* p) {
    pending_ = p;
    if (!p) return;
    p->ch = ch_;
    p->tuning = pendingTuning_;
    p->tuningSet = false;
    p->strike = false;
    p->vel = 0.0f;
    p->released = released_;
    latched_ = ch_.lastSeq();
}

void Voice::latch() {
    Pending* const p = pending_;
    if (!p) return;
    if (p->ch.lastSeq() != latched_) {
        ch_.latch(p->ch);
        latched_ = p->ch.lastSeq();
    }
    if (p->tuningSet) {
        pendingTuning_ = p->tuning;
        tuningPending_ = true;
        p->tuningSet = false;
    }
    if (p->strike) {
        strikeQueued_ = true;
        vel_ = p->vel;
        p->strike = false;
    }
    released_ = p->released;
}

// ---- for a pool ------------------------------------------------------------------------------

void Voice::reseed(uint32_t seed) {
    seed_ = seed;
    rng_.seed(seed);
}

void Voice::stop() {
    strikeQueued_ = false;
    sleep();
}

// ---- the engine ------------------------------------------------------------------------------

void Voice::render(float* bridge, float* pickup, int n) {
    int i = 0;
    while (i < n) {
        if (phase_ == 0) block();
        const int m = n - i < kBlock - phase_ ? n - i : kBlock - phase_;
        if (asleep_) {
            for (int j = 0; j < m; ++j) {
                if (bridge) bridge[i + j] = 0.0f;
                if (pickup) pickup[i + j] = 0.0f;
            }
        } else {
            samples(bridge ? bridge + i : nullptr, pickup ? pickup + i : nullptr, m);
        }
        i += m;
        phase_ += m;
        if (phase_ == kBlock) {
            phase_ = 0;
            if (!asleep_) endBlock();
        }
    }
}

void Voice::samples(float* bridge, float* pickup, int n) {
    if (doBridge_) {
        if (doAttack_) {
            if (doTip_) run<true, true, true>(bridge, pickup, n);
            else run<true, false, true>(bridge, pickup, n);
        } else {
            if (doTip_) run<true, true, false>(bridge, pickup, n);
            else run<true, false, false>(bridge, pickup, n);
        }
    } else {
        if (doTip_) run<false, true, false>(bridge, pickup, n);
        else run<false, false, false>(bridge, pickup, n);
    }
}

// the hammer's loop while it touches, every bank's turn from the sample it leaves; the bridge force at its ramped
// weight with the attack's force at its, the pickup's curve and pole at its
template <bool kBridge, bool kTip, bool kAttack>
void Voice::run(float* bridge, float* pickup, int n) {
    float sb = 0.0f, sp = 0.0f, s = 0.0f;
    auto emit = [&](int j, float b, float a, float x, float v) {
        const float bo = kBridge ? (kAttack ? b * bridgeW_.next() + a * attackW_.next() : b * bridgeW_.next()) : 0.0f;
        const float po = kTip ? pickup_.sample(x, v) * pickupW_.next() : 0.0f;
        if (bridge) bridge[j] = bo;
        if (pickup) pickup[j] = po;
        sb += bo * bo;
        sp += po * po;
        s += (bo + po) * (bo + po);
    };
    int j = 0;
    for (; j < n && strings_.touching(); ++j) {
        float b, x, v;
        strings_.step<true, kBridge, kTip>(b, x, v);
        emit(j, b, kAttack ? attack_.touching(strings_.substepForces()) : 0.0f, x, v);
    }
    for (; j < n; ++j) {
        float b, x, v;
        strings_.step<false, kBridge, kTip>(b, x, v);
        emit(j, b, kAttack ? attack_.step() : 0.0f, x, v);
    }
    sqBridge_ += sb;
    sqPickup_ += sp;
    sq_ += s;
}

void Voice::wake() {
    asleep_ = false;
    woke_ = true;
    ch_.snap();                                          // every channel on its target, the law's partials with them
    strings_.reset();
    attack_.reset();
    pitch_.jump();
    pickup_.reset();
}

void Voice::sleep() {
    asleep_ = true;
    strings_.reset();
    attack_.reset();
    pickup_.reset();
    energy_ = 0.0f;
    sqBridge_ = sqPickup_ = sq_ = 0.0f;
    modesOn_ = 0;
    for (float& r : re_) r = 0.0f;
    publish();
}

void Voice::block() {
    if (tuningPending_) {
        tuningPending_ = false;
        tuning_ = pendingTuning_;
        ch_.setTuning(tuning_);
        gain_ = tuning_.outputGain;
        pickup_.setPole(tuning_.ampHz, fs_);
        top_ = tuning_.ceiling * fs_;
        lawNote_ = -1.0f;                                // the law's ceiling is the tuning's: written again
    }

    // the law at the target note, then wake or tick. asleep with no strike queued, nothing
    const float target = ch_.target(Channel::midi);
    const float lawNote = std::isfinite(target) ? clamp(target, 0.0f, 127.0f) : kScaleMidi;
    const bool noteMoved = lawNote != lawNote_;
    woke_ = false;
    if (asleep_ && !strikeQueued_) return;
    if (noteMoved || law_.pending(ch_)) {
        law_.update(ch_, rankAt(ch_.shape(), lawNote), noteMoved, tuning_);
        lawNote_ = lawNote;
    }
    if (asleep_) wake();
    else ch_.tick(dt_);

    // the eased instrument this block
    State& st = state_;
    for (int i = 0; i < kChannelCount; ++i) st.shape.v[i] = ch_.value((Channel)i);
    const Shape& es = st.shape;

    // the pitch axis and the rank at the note
    const float f0 = pitch_.block(es[Channel::midi], es[Channel::glide]);
    const float note = pitch_.note();
    const float rk[8] = {note, es[Channel::length], es[Channel::bass], es[Channel::tension], es[Channel::gauge], es[Channel::strikeShare],
                         es[Channel::hammerMass], es[Channel::damperTop]};
    if (woke_ || std::memcmp(rk, rankKey_, sizeof rk) != 0) {
        st.rank = rankAt(es, note);
        std::memcpy(rankKey_, rk, sizeof rk);
    }
    const RankState& r = st.rank;

    // the hammer at the note: the felt through the rank, the tine's tip by the generator
    const float g = clamp(std::isfinite(es[Channel::generator]) ? es[Channel::generator] : -1.0f, -1.0f, 1.0f);
    const float hk[7] = {es[Channel::hardness], es[Channel::exponent], es[Channel::felt], g, r.hammerMass, r.hardnessScale, r.pAdd};
    if (woke_ || std::memcmp(hk, hammerKey_, sizeof hk) != 0) {
        strings_.setHammer(hammerHardnessAt(es[Channel::hardness], g) * r.hardnessScale, hammerExponentAt(es[Channel::exponent], r.pAdd),
                           hammerFeltAt(es[Channel::felt], g), r.hammerMass * 1000.0f);
        std::memcpy(hammerKey_, hk, sizeof hk);
    }

    // per slot: hz, on, the decays, the drive, the readback, the out. a slot under kUnisonModes loses sigma_b
    // through the bridge (the coupling, up_string.h), so its own decay is sigma_n - sigma_b, held at a tenth of
    // sigma_n at least: a hand-written table or a small lossRate rings no longer than its own t60 says
    const float pedal = clamp(std::isfinite(es[Channel::pedal]) ? es[Channel::pedal] : 0.0f, 0.0f, 1.0f);
    const bool damping = released_ && r.damped;
    const float muL = r.mu * r.L;
    const float driveScale = 2.0f * strings_.dtSub() / muL * 1000.0f;
    const LawConstants& lc = lawConstants();
    const float sigmaB = r.bridgeLoss * clamp(-g, 0.0f, 1.0f);   // the string side's bridge; none at the tine
    const float delta = sigmaB * lc.bridgeReactive;              // its reactive part's rate
    int on = 0;
    for (int k = 0; k < kMaxModes; ++k) {
        const float ratio = es[ratioChannel(k)], t60 = es[t60Channel(k)], gain = es[gainChannel(k)], out = es[outChannel(k)];
        const float hz = ratio * f0;
        const bool live = hz > 0.0f && hz < top_ && (gain != 0.0f || out != 0.0f) && t60 > 0.0f;
        if (!live) {
            strings_.setPartial(k, false, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
            continue;
        }
        ++on;
        const float omega = 2.0f * kPi * hz;
        const float damp = damping ? dampRateOf(hz, es, pedal, lc) : 0.0f;
        const float sigma = kNepers60 / (t60 * r.t60Scale);
        const float own = k < kUnisonModes ? sigma - (sigmaB < 0.9f * sigma ? sigmaB : 0.9f * sigma) : sigma;
        strings_.setPartial(k, true, hz, own, damp, gain * driveScale / omega, gain, out);
    }
    // the unison: the capped strings channel's fractions, the second x the tonebar from the generator 0 up; the
    // bridge's loss on one string alone
    const float strings = stringsOf(es);
    float w2 = clamp(strings - 1.0f, 0.0f, 1.0f);
    const float w3 = clamp(strings - 2.0f, 0.0f, 1.0f);
    if (g >= 0.0f) w2 *= clamp(std::isfinite(es[Channel::tonebar]) ? es[Channel::tonebar] : 1.0f, 0.0f, 1.0f);
    strings_.setUnison(es[Channel::detune], w2, w3, g);
    strings_.setCoupling(sigmaB, delta);
    const float g2 = strings_.pointMass2();
    strings_.setPointMass(g2 > 0.0f ? 0.5f * muL / g2 : 1.0f);
    strings_.beginBlock();

    // a strike due: the hammer at the string, thrown at hammerScale vel^velCurve, held under kThrowMax
    float v0 = -1.0f;
    if (strikeQueued_) {
        strikeQueued_ = false;
        v0 = vel_ > 0.0f ? tuning_.hammerScale * std::pow(vel_, tuning_.velCurve) : 0.0f;
        if (v0 > kThrowMax) v0 = kThrowMax;
        strings_.strike(v0);
    }

    // the pair's weights: the bridge's in N per mm of out-weighted displacement, the pickup's; the attack's, the
    // bridge's without the rank's scale (its force is in N)
    const float bw = clamp(-g, 0.0f, 1.0f) * r.bridgeScale * 1e-3f * gain_;
    const float pw = clamp(1.0f + g, 0.0f, 1.0f) * gain_;
    const float aw = clamp(-g, 0.0f, 1.0f) * gain_;
    if (woke_) {
        bridgeW_.snap(bw);
        pickupW_.snap(pw);
        attackW_.snap(aw);
    } else {
        bridgeW_.to(bw);
        pickupW_.to(pw);
        attackW_.to(aw);
    }
    bridgeT_ = bw;
    pickupT_ = pw;
    attackT_ = aw;
    doBridge_ = bridgeW_.v != 0.0f || bw != 0.0f;
    doTip_ = pickupW_.v != 0.0f || pw != 0.0f;

    // the attack where the bridge is heard (off it nothing is armed and nothing rings): the long bank at the rank's
    // string, a strike's thud at the rank's hammer mass x the throw
    if (!doBridge_) attack_.reset();
    attack_.setBlock(r, top_, doBridge_ ? tuning_.attack : 0.0f);
    if (v0 >= 0.0f) attack_.strike(r.hammerMass * v0);
    doAttack_ = doBridge_ && attack_.running();
    pickup_.setBlock(g, es[Channel::gap], es[Channel::offset]);

    st.f0 = f0;
    modesOn_ = on;
    sqBridge_ = sqPickup_ = sq_ = 0.0f;
}

// the telemetry; the silence gate
void Voice::endBlock() {
    bridgeW_.snap(bridgeT_);                             // the weights land exactly (a weight at 0 stays compiled out)
    pickupW_.snap(pickupT_);
    attackW_.snap(attackT_);
    strings_.endBlock();
    energy_ = strings_.energySq();
    publish();
    if (!strings_.touching() && !strikeQueued_ && energy_ < tuning_.silence) sleep();
}

void Voice::publish() {
    State& st = state_;
    if (asleep_)
        for (int i = 0; i < kChannelCount; ++i) st.shape.v[i] = ch_.value((Channel)i);
    float most = -1.0f;
    const Bank<kMaxModes>& main = strings_.main();
    for (int k = 0; k < kMaxModes; ++k) {
        const float a = main.amplitude(k);
        st.amp[k] = a;
        float im;
        main.phasor(k, re_[k], im);
        if (a > most) {
            most = a;
            st.loudest = k;
        }
    }
    for (int k = 0; k < kUnisonModes; ++k) {
        st.amp2[k] = strings_.second().amplitude(k);
        st.amp3[k] = strings_.third().amplitude(k);
    }
    for (int k = 0; k < kLongModes; ++k) st.ampLong[k] = attack_.amplitude(k);
    const Hammer& h = strings_.hammer();
    st.hammerTouching = h.touching();
    st.hammerHeight = h.touching() ? h.height() : 0.0f;
    st.hammerVelocity = h.touching() ? h.velocity() : 0.0f;
    st.hammerPeakForce = strings_.blockPeak();
    st.hammerContact = strings_.contactSec();
    st.bridgeRms = std::sqrt(sqBridge_ * kInvBlock);
    st.pickupRms = std::sqrt(sqPickup_ * kInvBlock);
    st.rms = std::sqrt(sq_ * kInvBlock);
    st.modesOn = asleep_ ? 0 : modesOn_;
    st.released = released_;
    st.damped = released_ && st.rank.damped;
    st.pedal = st.shape[Channel::pedal];
    st.asleep = asleep_;
}

}  // namespace up
