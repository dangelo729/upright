// upright — up_channels.h  (plan §1; calls: the slots are the partials in order)
// the channel table: every number of the piano, each a target, an eased
// value, a rate and a sequence. a write carries a sequence number; between
// two tables (a writer's pending one and the audio side's) the newer write
// wins, per channel. a Shape is every target at once, and lands whole; a nan
// is no write. once per block, tick() moves every value toward its target by
// the one law; a rate at or below zero jumps.
//
// the channels: the instrument's twenty-four in five groups (the strings, the
// hammer, the body, the generator, the damper; physical units but the
// generator), then four contiguous blocks of kMaxModes (each partial's ratio
// to f0, its t60, its gain at the strike point, its output weight), then the
// player's three. the law (up_law.h) writes the partial blocks from the
// instrument's channels at the note the rank gives (up_rank.h).
//
// the slots: slot k is partial k + 1 in every build, so a channel is always
// the same partial; a smaller kMaxModes drops the top. a build of 8 keeps
// the first eight partials and loses everything over them: a piano with no
// brightness (the strike's ninth partial and up, the tine's beam has its six
// either way).
#pragma once

#include <cstdint>

#include "up_config.h"
#include "up_tuning.h"

namespace up {

// every channel of the plan. units: metres, newtons, seconds, cents, hz,
// shares; the hammer's in its channel units (N/mm^p, s/mm, grams)
enum class Channel : uint16_t {
    // the strings (§1, §6): the speaking length at c4 (m), the longest string
    // the case holds (m), the design tension (N), the wire's gauge (1 as
    // designed), how many strings a note has (1 .. 3), the unison's spread
    // (cents), where the hammer lands at c4 (a share of the length), the
    // string's own loss (1/s at c4) and its power in frequency
    length, bass, tension, gauge, strings, detune, strikeShare, lossRate, lossSlope,
    // the hammer (§4): the felt's stiffness at c4 (N/mm^p), its power, its
    // velocity loss (s/mm), the hammer's mass at c4 (g)
    hardness, exponent, felt, hammerMass,
    // the body (§8): the soundboard's area (m²), its loss factor, the back
    // closed (0) to open (1)
    boardArea, boardLoss, open,
    // the generator (§3, §7): string -1, tine 0, reed +1; the pickup's gap
    // and the tip's offset off its axis (mm); the tonebar's weight (0 .. 1);
    // the tine's fundamental t60 at c4 (s; the reed's decays are kReedT60
    // times the tine's); the beam's overtone ratios (n >= 2) times this, the
    // tuning spring's mass at the tip (1 the bare beam)
    generator, gap, offset, tonebar, tineT60, tineLoad,
    // the damper (§9): the decay rate a released key's damper adds (1/s at
    // c4); the highest key with a damper (midi)
    dampRate, damperTop,
    // the partials (§3): four contiguous blocks of kMaxModes, ratioChannel(k) ..
#define UP_MODE(k) ratio##k,
#include "up_modes.inc"
#undef UP_MODE
#define UP_MODE(k) t60_##k,
#include "up_modes.inc"
#undef UP_MODE
#define UP_MODE(k) gain##k,
#include "up_modes.inc"
#undef UP_MODE
#define UP_MODE(k) out##k,
#include "up_modes.inc"
#undef UP_MODE
    // the player (§1, §9): the note, the glide's rate per second (0 a jump),
    // the sustain pedal (0 .. 1)
    midi, glide, pedal,
    count
};

constexpr int kChannelCount = (int)Channel::count;
constexpr int kInstrumentChannels = (int)Channel::damperTop + 1;
static_assert((int)Channel::ratio0 == kInstrumentChannels, "the partial blocks follow the instrument");
static_assert((int)Channel::t60_0 - (int)Channel::ratio0 == kMaxModes, "the ratios are contiguous");
static_assert((int)Channel::gain0 - (int)Channel::t60_0 == kMaxModes, "the t60s are contiguous");
static_assert((int)Channel::out0 - (int)Channel::gain0 == kMaxModes, "the gains are contiguous");
static_assert((int)Channel::midi - (int)Channel::out0 == kMaxModes, "the outs are contiguous");
static_assert(kInstrumentChannels == 24 && kChannelCount == kInstrumentChannels + 4 * kMaxModes + 3, "twenty-four instrument, four blocks, three player");

inline Channel ratioChannel(int k) { return (Channel)((int)Channel::ratio0 + k); }
inline Channel t60Channel(int k) { return (Channel)((int)Channel::t60_0 + k); }
inline Channel gainChannel(int k) { return (Channel)((int)Channel::gain0 + k); }
inline Channel outChannel(int k) { return (Channel)((int)Channel::out0 + k); }
inline bool isInstrumentChannel(Channel c) { return (int)c < kInstrumentChannels; }
inline bool isPartialChannel(Channel c) { return c >= Channel::ratio0 && (int)c < (int)Channel::midi; }
inline int partialOf(Channel c) { return isPartialChannel(c) ? ((int)c - (int)Channel::ratio0) % kMaxModes : -1; }

// which tuning rate a channel takes by default: the generator and the damper
// groups take the body's (the instrument's slow things)
enum class Group : uint8_t { strings, hammer, body, modes, pitch, pedal };
Group groupOf(Channel c);
float groupRate(Group g, const Tuning& t);

const char* channelName(Channel c);                  // "length" .. "ratio7", "t60_7", "gain7", "out7" .. "pedal"
bool channelByName(const char* name, Channel& out);

// each channel's span, in its own units: what a control can sensibly ask
// for, and what the tests drive it between. every value inside is legal.
void channelRange(Channel c, float& lo, float& hi);

// every channel's target, indexed by Channel
struct Shape {
    float v[kChannelCount];
    float& operator[](Channel c) { return v[(int)c]; }
    float operator[](Channel c) const { return v[(int)c]; }
};

// a string's partials, slot by slot: ratio to f0, t60 (s), gain at the
// strike point (signed), output weight (signed)
struct PartialTable {
    float ratio[kMaxModes];
    float t60[kMaxModes];
    float gain[kMaxModes];
    float out[kMaxModes];
};
void         toShape(const PartialTable& p, Shape& s);   // the partial channels written from the table (a nan stays nan)
PartialTable fromShape(const Shape& s);                  // the table the partial channels hold (nan where a channel is)

// the shape a voice holds before anyone sets one: grand's (up_presets.h),
// its partials the law's at middle c, no glide, the pedal up
Shape defaultShape(const Tuning& t = Tuning());

// every channel nan, so no write: a shape to fill in by hand
Shape nanShape();

class Channels {
public:
    explicit Channels(const Tuning& t = Tuning());

    // writes. each takes the next sequence number; a nan is no write.
    void set(Channel c, float target);
    void rate(Channel c, float perSec);
    void setShape(const Shape& s);                    // every target under one sequence number
    // a writer that carries its own sequence (the law, for the partials):
    // lands unless the channel's last write is newer
    void write(Channel c, float target, uint32_t seq);

    // take every target and rate written in `from` more recently than here
    void latch(const Channels& from);
    // the group rates, for every channel whose rate nobody set
    void setTuning(const Tuning& t);

    // once per block. true while a channel outside the pitch axis is more
    // than 0.1 % from its target: the piano is still moving
    bool tick(float dt);
    void snap();                                      // every value to its target

    Shape    shape() const;                           // the targets as held
    float    target(Channel c) const  { return target_[(int)c]; }
    float    value(Channel c) const   { return value_[(int)c]; }
    float    rateOf(Channel c) const  { return rate_[(int)c]; }
    uint32_t seq(Channel c) const     { return seq_[(int)c]; }
    bool     rateWritten(Channel c) const { return rateSeq_[(int)c] != 0; }   // a rate somebody set
    uint32_t lastSeq() const          { return counter_; }

private:
    float    target_[kChannelCount];
    float    value_[kChannelCount];
    float    rate_[kChannelCount];
    float    coef_[kChannelCount];                    // 1 - exp(-rate dt), for coefRate_ and coefDt_
    float    coefRate_[kChannelCount];
    float    coefDt_ = -1.0f;
    uint32_t seq_[kChannelCount];
    uint32_t rateSeq_[kChannelCount];
    uint32_t counter_ = 0;
};

}  // namespace up
