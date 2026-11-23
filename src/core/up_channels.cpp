// upright — up_channels.cpp
// runs per write and per block (tick), never per sample; defaultShape runs
// the law once.
#include "up_channels.h"

#include <cstring>
#include <limits>

#include "up_law.h"
#include "up_presets.h"
#include "up_rank.h"

namespace up {
namespace {

// in the enum's order
const char* const kNames[kChannelCount] = {
    "length", "bass", "tension", "gauge", "strings", "detune", "strikeShare", "lossRate", "lossSlope",
    "hardness", "exponent", "felt", "hammerMass",
    "boardArea", "boardLoss", "open",
    "generator", "gap", "offset", "tonebar", "tineT60", "tineLoad",
    "dampRate", "damperTop",
#define UP_MODE(k) "ratio" #k,
#include "up_modes.inc"
#undef UP_MODE
#define UP_MODE(k) "t60_" #k,
#include "up_modes.inc"
#undef UP_MODE
#define UP_MODE(k) "gain" #k,
#include "up_modes.inc"
#undef UP_MODE
#define UP_MODE(k) "out" #k,
#include "up_modes.inc"
#undef UP_MODE
    "midi", "glide", "pedal",
};

// a finite target held inside its channel's span
inline float held(Channel c, float x) {
    float lo, hi;
    channelRange(c, lo, hi);
    return clamp(x, lo, hi);
}

}  // namespace

// ---- the table ------------------------------------------------------------------------

Group groupOf(Channel c) {
    if (c <= Channel::lossSlope) return Group::strings;
    if (c <= Channel::hammerMass) return Group::hammer;
    if (isInstrumentChannel(c)) return Group::body;           // the body, the generator, the damper
    if (isPartialChannel(c)) return Group::modes;
    if (c == Channel::pedal) return Group::pedal;
    return Group::pitch;
}

float groupRate(Group g, const Tuning& t) {
    switch (g) {
        case Group::strings: return t.rateStrings;
        case Group::hammer:  return t.rateHammer;
        case Group::body:    return t.rateBody;
        case Group::modes:   return t.rateModes;
        case Group::pitch:   return t.ratePitch;
        case Group::pedal:   return t.ratePedal;
    }
    return 0.0f;
}

const char* channelName(Channel c) {
    const int i = (int)c;
    return i >= 0 && i < kChannelCount ? kNames[i] : "";
}

bool channelByName(const char* name, Channel& out) {
    if (!name) return false;
    for (int i = 0; i < kChannelCount; ++i) {
        if (std::strcmp(name, kNames[i]) == 0) { out = (Channel)i; return true; }
    }
    return false;
}

void channelRange(Channel c, float& lo, float& hi) {
    lo = 0.0f;
    hi = 1.0f;
    if (isPartialChannel(c)) {
        const int block = ((int)c - (int)Channel::ratio0) / kMaxModes;
        if (block == 0) { lo = 0.25f; hi = 32768.0f; }       // ratio to f0 (the beam's series runs high, off)
        else if (block == 1) { lo = 1e-6f; hi = 2000.0f; }   // t60, s
        else if (block == 2) { lo = -1.0f; }                 // gain, signed
        else { lo = -128.0f; hi = 128.0f; }                  // out, signed: n (-1)^(n+1) on the string
        return;
    }
    switch (c) {
        case Channel::length:      lo = 0.1f; hi = 2.0f; break;
        case Channel::bass:        lo = 0.3f; hi = 4.0f; break;
        case Channel::tension:     lo = 100.0f; hi = 3000.0f; break;
        case Channel::gauge:       lo = 0.25f; hi = 4.0f; break;
        case Channel::strings:     lo = 1.0f; hi = 3.0f; break;
        case Channel::detune:      hi = 100.0f; break;
        case Channel::strikeShare: hi = 0.5f; break;
        case Channel::lossRate:    lo = 0.01f; hi = 50.0f; break;
        case Channel::lossSlope:   hi = 4.0f; break;
        case Channel::hardness:    lo = 1.0f; hi = 10000.0f; break;
        case Channel::exponent:    lo = 2.0f; hi = 3.5f; break;
        case Channel::felt:        hi = 0.005f; break;
        case Channel::hammerMass:  lo = 0.5f; hi = 50.0f; break;
        case Channel::boardArea:   lo = 0.1f; hi = 4.0f; break;
        case Channel::boardLoss:   lo = 0.001f; hi = 0.3f; break;
        case Channel::generator:   lo = -1.0f; break;
        case Channel::gap:         lo = 0.1f; hi = 10.0f; break;
        case Channel::offset:      hi = 5.0f; break;
        case Channel::tineT60:     lo = 0.05f; hi = 100.0f; break;
        case Channel::tineLoad:    lo = 0.5f; hi = 4.0f; break;
        case Channel::dampRate:    hi = 500.0f; break;
        case Channel::damperTop:   hi = 127.0f; break;
        case Channel::midi:        hi = 127.0f; break;
        case Channel::glide:       hi = 100.0f; break;
        default: break;                                      // the 0..1 ones: open, tonebar, pedal
    }
}

void toShape(const PartialTable& p, Shape& s) {
    for (int k = 0; k < kMaxModes; ++k) {
        s[ratioChannel(k)] = p.ratio[k];
        s[t60Channel(k)] = p.t60[k];
        s[gainChannel(k)] = p.gain[k];
        s[outChannel(k)] = p.out[k];
    }
}

PartialTable fromShape(const Shape& s) {
    PartialTable p;
    for (int k = 0; k < kMaxModes; ++k) {
        p.ratio[k] = s[ratioChannel(k)];
        p.t60[k] = s[t60Channel(k)];
        p.gain[k] = s[gainChannel(k)];
        p.out[k] = s[outChannel(k)];
    }
    return p;
}

Shape defaultShape(const Tuning& t) {
    Shape s = grand();
    toShape(partialsOf(s, rankAt(s, kScaleMidi), t), s);
    s[Channel::midi] = kScaleMidi;
    s[Channel::glide] = 0.0f;
    s[Channel::pedal] = 0.0f;
    return s;
}

Shape nanShape() {
    Shape s;
    for (int i = 0; i < kChannelCount; ++i) s.v[i] = std::numeric_limits<float>::quiet_NaN();
    return s;
}

// ---- the channels ---------------------------------------------------------------------

Channels::Channels(const Tuning& t) {
    const Shape s = defaultShape(t);
    for (int i = 0; i < kChannelCount; ++i) {
        target_[i] = value_[i] = s.v[i];
        rate_[i] = groupRate(groupOf((Channel)i), t);
        coef_[i] = 1.0f;
        coefRate_[i] = -1.0f;
        seq_[i] = rateSeq_[i] = 0;
    }
}

void Channels::set(Channel c, float target) {
    if (!std::isfinite(target)) return;
    const int i = (int)c;
    target_[i] = held(c, target);
    seq_[i] = ++counter_;
}

void Channels::rate(Channel c, float perSec) {
    if (!std::isfinite(perSec)) return;
    const int i = (int)c;
    rate_[i] = perSec;
    rateSeq_[i] = ++counter_;
}

void Channels::setShape(const Shape& s) {
    const uint32_t q = ++counter_;
    for (int i = 0; i < kChannelCount; ++i) {
        if (!std::isfinite(s.v[i])) continue;
        target_[i] = held((Channel)i, s.v[i]);
        seq_[i] = q;
    }
}

void Channels::write(Channel c, float target, uint32_t seq) {
    const int i = (int)c;
    if (!std::isfinite(target) || seq_[i] > seq) return;
    target_[i] = target;
    seq_[i] = seq;
    if (seq > counter_) counter_ = seq;
}

void Channels::latch(const Channels& from) {
    for (int i = 0; i < kChannelCount; ++i) {
        if (from.seq_[i] > seq_[i]) {
            target_[i] = from.target_[i];
            seq_[i] = from.seq_[i];
        }
        if (from.rateSeq_[i] > rateSeq_[i]) {
            rate_[i] = from.rate_[i];
            rateSeq_[i] = from.rateSeq_[i];
        }
    }
    if (from.counter_ > counter_) counter_ = from.counter_;
}

void Channels::setTuning(const Tuning& t) {
    for (int i = 0; i < kChannelCount; ++i) {
        if (rateSeq_[i] == 0) rate_[i] = groupRate(groupOf((Channel)i), t);
    }
}

bool Channels::tick(float dt) {
    if (dt != coefDt_) {
        coefDt_ = dt;
        for (int i = 0; i < kChannelCount; ++i) coefRate_[i] = -1.0f;
    }
    bool moving = false;
    constexpr int pitchFrom = (int)Channel::midi, pitchTo = (int)Channel::glide;   // the pitch axis
    for (int i = 0; i < kChannelCount; ++i) {
        const float target = target_[i];
        if (value_[i] == target) continue;
        const float r = rate_[i];
        if (r <= 0.0f) { value_[i] = target; continue; }   // a jump lands exactly
        if (r != coefRate_[i]) {
            coef_[i] = approachCoef(r, dt);
            coefRate_[i] = r;
        }
        value_[i] += (target - value_[i]) * coef_[i];
        if (i < pitchFrom || i > pitchTo) {
            const float d = target - value_[i], t = target < 0.0f ? -target : target;
            moving = moving || (d < 0.0f ? -d : d) > 1e-3f * t + 1e-9f;
        }
    }
    return moving;
}

void Channels::snap() {
    for (int i = 0; i < kChannelCount; ++i) value_[i] = target_[i];
}

Shape Channels::shape() const {
    Shape s;
    for (int i = 0; i < kChannelCount; ++i) s.v[i] = target_[i];
    return s;
}

}  // namespace up
