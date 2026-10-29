// upright — up_pool.cpp  (plan §13, the pool)
// no libm here: the fade's step is approachCoef's (up_config.h), per tuning;
// the board's table and weights are up_board.cpp's, per body write and per
// note.
#include "up_pool.h"

namespace up {
namespace {

constexpr float kFadeFloor = 0.01f;        // a stolen voice's fade under this goes to zero across one block

inline bool isNan(float x) { return x != x; }
inline bool sounds(const PoolSlot& s) { return s.use == PoolSlot::Use::held || s.use == PoolSlot::Use::released; }

}  // namespace

uint32_t PoolBase::seedOf(uint32_t seed, int i) {
    const uint32_t s = (seed ^ 0x5bd1e995u) + 0x9E3779B9u * (uint32_t)(i + 1);
    return s ? s : 1u;
}

PoolBase::PoolBase(Voice* voices, PoolSlot* slots, PoolLane* lanes, const float** forces, const float** weights, int m, uint32_t seed, float fs)
    : v_(voices), s_(slots), lane_(lanes), forces_(forces), weights_(weights), m_(m < 1 ? 1 : m), fs_(fs > 1.0f ? fs : kDefaultFs) {
    board_.init(fs_);
    for (int i = 0; i < m_; ++i) {
        v_[i].reseed(seedOf(seed, i));
        if (fs_ != kDefaultFs) v_[i].init(fs_);
        s_[i] = PoolSlot();
        s_[i].level.snap(1.0f);
        PoolLane& l = lane_[i];
        for (int j = 0; j < kBlock; ++j) l.bridge[j] = 0.0f;
        l.point = -1.0f;                           // no note yet: the weights are computed at the first render
        board_.inputWeights(kScaleMidi, l.weights);
        forces_[i] = l.bridge;
        weights_[i] = l.weights;
    }
    for (int j = 0; j < kBlock; ++j) pickup_[j] = picks_[j] = boardOut_[j] = 0.0f;
    setTuning(Tuning());
}

// ---- the player ----------------------------------------------------------------------------

void PoolBase::noteOn(float midi, float vel01) {
    if (isNan(midi)) return;
    const float vel = isNan(vel01) ? 0.0f : clamp(vel01, 0.0f, 1.0f);
    // the key's own voice, still sounding: struck again, a fresh hammer on the moving string
    for (int i = 0; i < m_; ++i) {
        PoolSlot& s = s_[i];
        if (!sounds(s) || s.key != midi) continue;
        v_[i].noteOn(midi, vel);
        s.use = PoolSlot::Use::held;
        s.vel = vel;
        s.age = ++clock_;
        s.struck = true;
        return;
    }
    // a stolen voice already waiting for the key: the newer velocity
    for (int i = 0; i < m_; ++i) {
        PoolSlot& s = s_[i];
        if (s.use != PoolSlot::Use::stolen || !s.waiting || s.nextKey != midi) continue;
        s.nextVel = vel;
        s.nextOff = false;
        return;
    }
    // the free voice longest asleep, else the quietest sounding one, else the oldest stolen one's waiting note
    int at = oldest(PoolSlot::Use::free);
    if (at >= 0) {
        land(at, midi, vel, false);
        return;
    }
    at = quietest();
    if (at < 0) at = oldest(PoolSlot::Use::stolen);
    if (at >= 0) steal(at, midi, vel);
}

void PoolBase::noteOff(float midi) {
    for (int i = 0; i < m_; ++i) {
        PoolSlot& s = s_[i];
        if (s.use == PoolSlot::Use::held && s.key == midi) {
            v_[i].noteOff();
            s.use = PoolSlot::Use::released;
        }
        if (s.use == PoolSlot::Use::stolen && s.waiting && s.nextKey == midi) s.nextOff = true;
    }
}

void PoolBase::pedal(float p01) {
    if (isNan(p01)) return;
    pedal_ = p01;
    for (int i = 0; i < m_; ++i) v_[i].setPedal(p01);
}

void PoolBase::allNotesOff() {
    for (int i = 0; i < m_; ++i) {
        PoolSlot& s = s_[i];
        if (s.use == PoolSlot::Use::held) {
            v_[i].noteOff();
            s.use = PoolSlot::Use::released;
        }
        if (s.use == PoolSlot::Use::stolen && s.waiting) s.nextOff = true;
    }
}

// ---- every voice -----------------------------------------------------------------------------

// the pedal channel written pool-wide is the pool's pedal; a body channel is the board's plate
void PoolBase::set(Channel c, float target) {
    if ((int)c >= kChannelCount || isNan(target)) return;
    if (c == Channel::pedal) pedal_ = target;
    if (c == Channel::boardArea) plate(target, loss_, open_);
    if (c == Channel::boardLoss) plate(area_, target, open_);
    if (c == Channel::open) plate(area_, loss_, target);
    for (int i = 0; i < m_; ++i) v_[i].set(c, target);
}

void PoolBase::setRate(Channel c, float perSec) {
    if ((int)c >= kChannelCount || isNan(perSec)) return;
    for (int i = 0; i < m_; ++i) v_[i].setRate(c, perSec);
}

void PoolBase::setShape(const Shape& s) {
    if (!isNan(s[Channel::pedal])) pedal_ = s[Channel::pedal];
    plate(s[Channel::boardArea], s[Channel::boardLoss], s[Channel::open]);
    for (int i = 0; i < m_; ++i) v_[i].setShape(s);
}

void PoolBase::setKnobs(const Knobs& k) { setShape(pianoOf(k, tuning_)); }

void PoolBase::setPartial(int k, float ratio, float t60, float gain, float out) {
    for (int i = 0; i < m_; ++i) v_[i].setPartial(k, ratio, t60, gain, out);
}

void PoolBase::setPartials(const PartialTable& t) {
    for (int i = 0; i < m_; ++i) v_[i].setPartials(t);
}

// the fade a block is the one law's toward zero at stealRate: x (1 - (1 - e^(-rate dt)))
void PoolBase::setTuning(const Tuning& t) {
    tuning_ = clamped(t);
    for (int i = 0; i < m_; ++i) v_[i].setTuning(t);
    fadeK_ = 1.0f - approachCoef(tuning_.stealRate, (float)kBlock / fs_);
}

int PoolBase::sounding() const {
    int n = 0;
    for (int i = 0; i < m_; ++i) n += v_[i].asleep() ? 0 : 1;
    return n;
}

// the plate from the written body (a nan keeps the plate's own); a plate whose sides moved puts its modes
// elsewhere, so every voice's weights are computed again at its next render
void PoolBase::plate(float area, float loss, float open) {
    if (!isNan(area)) area_ = area;
    if (!isNan(loss)) loss_ = loss;
    if (!isNan(open)) open_ = open;
    const float a = board_.table().a, b = board_.table().b;
    board_.setPlate(area_, loss_, open_);
    if (board_.table().a != a || board_.table().b != b)
        for (int i = 0; i < m_; ++i) lane_[i].point = -1.0f;
}

// ---- the note table --------------------------------------------------------------------------

// the pool's pedal, then the note; the voice is asleep, so it lands whole at
// the next block (every channel on its target, the banks from zero)
void PoolBase::land(int i, float key, float vel, bool off) {
    Voice& v = v_[i];
    PoolSlot& s = s_[i];
    v.setPedal(pedal_);
    v.noteOn(key, vel);
    if (off) v.noteOff();
    s.use = off ? PoolSlot::Use::released : PoolSlot::Use::held;
    s.key = key;
    s.vel = vel;
    s.age = ++clock_;
    s.struck = true;
    s.waiting = false;
    s.nextOff = false;
    s.fade = 1.0f;
    s.levelT = 1.0f;
    s.level.snap(1.0f);
}

// the voice fades out of the sum; the note waits for it
void PoolBase::steal(int i, float key, float vel) {
    PoolSlot& s = s_[i];
    s.use = PoolSlot::Use::stolen;
    s.waiting = true;
    s.nextKey = key;
    s.nextVel = vel;
    s.nextOff = false;
}

// the sounding voice with the least energy; a voice whose strike is still
// queued is passed over while another sounds; ties to the oldest note
int PoolBase::quietest() const {
    int at = -1;
    for (int pass = 0; pass < 2 && at < 0; ++pass) {
        for (int i = 0; i < m_; ++i) {
            const PoolSlot& s = s_[i];
            if (!sounds(s) || (pass == 0 && s.struck)) continue;
            if (at < 0) {
                at = i;
                continue;
            }
            const float e = v_[i].energy(), best = v_[at].energy();
            if (e < best || (e == best && (int32_t)(s.age - s_[at].age) < 0)) at = i;
        }
    }
    return at;
}

int PoolBase::oldest(PoolSlot::Use use) const {
    int at = -1;
    for (int i = 0; i < m_; ++i) {
        if (s_[i].use != use) continue;
        if (at < 0 || (int32_t)(s_[i].age - s_[at].age) < 0) at = i;
    }
    return at;
}

// ---- the engine --------------------------------------------------------------------------------

void PoolBase::block() {
    for (int i = 0; i < m_; ++i) {
        PoolSlot& s = s_[i];
        if (s.use == PoolSlot::Use::stolen && s.levelT == 0.0f) {
            // faded to zero: stopped, and the waiting note lands on it
            v_[i].stop();
            s.use = PoolSlot::Use::free;
            s.age = ++clock_;
            if (s.waiting) land(i, s.nextKey, s.nextVel, s.nextOff);
            s.waiting = false;
        } else if (sounds(s) && !s.struck && v_[i].asleep()) {
            // rung out (or damped) and asleep: free from now
            s.use = PoolSlot::Use::free;
            s.age = ++clock_;
        }
        // the voices take every strike queued so far at this block's top
        s.struck = false;
    }

    // each voice's level across the block: 1, or a stolen one's fade by the
    // one law, held at zero once under the floor
    for (int i = 0; i < m_; ++i) {
        PoolSlot& s = s_[i];
        float target = 1.0f;
        if (s.use == PoolSlot::Use::stolen) {
            s.fade *= fadeK_;
            if (s.fade < kFadeFloor) s.fade = 0.0f;
            target = s.fade;
        }
        s.level.snap(s.levelT);
        s.level.to(target);
        s.levelT = target;
    }
}

void PoolBase::render(float* out, int n) { renderWith<false>(out, n, -1, nullptr, nullptr); }

template <bool kOne>
void PoolBase::renderWith(float* out, int n, int which, float* oneBridge, float* onePickup) {
    int i = 0;
    while (i < n) {
        if (phase_ == 0) block();
        const int m = n - i < kBlock - phase_ ? n - i : kBlock - phase_;
        for (int j = 0; j < m; ++j) picks_[j] = 0.0f;
        // every voice renders every sample (its blocks stay the pool's): its pair at its level, the force kept
        // for the board, the pickup summed; its weights where its note moved
        for (int k = 0; k < m_; ++k) {
            Voice& v = v_[k];
            PoolLane& l = lane_[k];
            PoolSlot& s = s_[k];
            v.render(l.bridge, pickup_, m);
            for (int j = 0; j < m; ++j) {
                const float level = s.level.next();
                l.bridge[j] *= level;
                picks_[j] += level * pickup_[j];
                if (kOne && k == which) {
                    if (oneBridge) oneBridge[i + j] = l.bridge[j];
                    if (onePickup) onePickup[i + j] = level * pickup_[j];
                }
            }
            const float point = v.bridgePoint();
            if (point != l.point) {
                l.point = point;
                board_.inputWeights(point, l.weights);
            }
        }
        board_.render(forces_, weights_, m_, boardOut_, m);
        if (out)
            for (int j = 0; j < m; ++j) out[i + j] = boardOut_[j] + picks_[j];
        i += m;
        phase_ += m;
        if (phase_ == kBlock) phase_ = 0;
    }
}

template void PoolBase::renderWith<false>(float*, int, int, float*, float*);
template void PoolBase::renderWith<true>(float*, int, int, float*, float*);

}  // namespace up
