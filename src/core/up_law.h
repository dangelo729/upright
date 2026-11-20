// upright — up_law.h
// the law: from the instrument's channels, at the note the rank gives
// (up_rank.h), every partial's ratio to f0, its t60, its gain at the strike
// point and its output weight. pure, no state but the writer's sequence,
// nothing on the heap; it runs once per instrument write and once per note,
// never per sample, so libm is fine inside it.
//
//   the string (generator -1)  ratios n sqrt(1 + B n²) with the rank's B;
//                      decays sigma_n = lossRate / t60Scale + lossRate (f_n /
//                      lossKnee)^lossSlope at the note's partial frequencies:
//                      the floor (the bridge and the air) scales with the
//                      rank, the frequency term (the wire's own bending and
//                      viscoelastic loss, nearly the same at every note) does
//                      not. the table holds t60 = 6.907755 / (lossRate (1 +
//                      t60Scale (f_n / lossKnee)^lossSlope)), so the voice's
//                      x t60Scale lands sigma_n; gains sin(n pi share) at the rank's strike
//                      share; outs n (-1)^(n+1), the slope at the bridge
//                      read with the tip's sign (dimensionless: the rank's
//                      bridgeScale makes it newtons), so a string's out and
//                      the beam's share a sign in each of the first six slots
//   the tine (generator 0)     the clamped-free beam: ratios (beta_n /
//                      beta_1)², the first kTineModes slots on and the rest
//                      off (the beam's series continued so the table stays
//                      ordered), the overtones (n >= 2) x the tineLoad
//                      channel, the tuning spring's load (a nan or a load at
//                      or under 0 the bare beam's 1); decays the tineT60
//                      channel x ratio^-tineSlope (held over 1 ms, a nan
//                      too); gains the beam's shape at the strike share and
//                      outs its shape at the tip, both over the tip's 2 (the
//                      tip at ±1)
//   the reed (generator +1)    the tine's ratios and shapes, the overtones'
//                      gains at kReedOvertone of the beam's, decays kReedT60
//                      times the tine's
//   between the anchors        from -1 to 0 every slot's ratio and t60
//                      log-interpolated (the geometric mean's path), the first
//                      kTineModes slots' gains and outs linear, the string's
//                      slots above them faded by clamp(-g, 0, 1); from 0 to
//                      +1 the overtones' gains toward the reed's, the t60s
//                      log-interpolated toward the reed's
//   the ceiling                a partial over tuning.ceiling x kDefaultFs at
//                      the note gets gain 0 and out 0 (off)
//
// the fitted constants are a LawConstants, so the tool varies them without a
// rebuild; lawConstants() is the block of up_law.cpp the engine runs on.
#pragma once

#include <cstdint>

#include "up_channels.h"
#include "up_tuning.h"

namespace up {

struct RankState;

// ---- the constants the fit does not move ---------------------------------------------------------
constexpr float kE            = 2.0e11f;   // Pa: steel's young's modulus (fletcher & rossing)
constexpr float kRho          = 7850.0f;   // kg/m³: steel
constexpr int   kTineModes    = 6;         // the beam's slots on at the tine and the reed
constexpr float kReedOvertone = 0.05f;     // the reed's overtones' gains over the beam's (pfeifle: no significant higher modes)
constexpr float kReedT60      = 0.6f;      // the reed's decays over the tine's (an estimate)
constexpr float kDampKnee     = 2000.0f;   // hz: the damper's added rate doubles here (an estimate)
constexpr float kAftersound   = 2.5f;      // the tine side's: the tonebar rings this much longer than the tine (the secondaries'
                                           //   decays over kAftersound^clamp(1 + g, 0, 1): 1 on the string side, up_string.h)
constexpr float kUnisonGain   = 0.5f;      // the tine side's: the tonebar heard at this of the tine (the secondaries' outs x
                                           //   lerp(1, kUnisonGain, clamp(1 + g, 0, 1)): equal strings on the string side)
constexpr float kTipHard      = 2.0f;      // the tine's tip: hardness x 2^(kTipHard (1 - |g|)) at the generator
constexpr float kTipLoss      = 1.0f;      //   and felt x 2^(-kTipLoss (1 - |g|)): harder and less lossy than felt
constexpr float kExponentLo   = 1.5f;      // the felt's power at a note is held inside these (the rank's pAdd on the channel's)
constexpr float kExponentHi   = 4.0f;

// the constants a fit may move (uptool fit --constants), each with its default in up_law.cpp
struct LawConstants {
    float tension;      // N: the design tension where a shape's tension channel is nan
    float rankLength;   // the length goes as (f_c4 / f0)^rankLength down the keyboard
    float dMax;         // m: the thickest plain wire; past it the string is wrapped
    float core;         // m: a wrapped string's steel core
    float loss;         // 1/s at c4: the string's own loss where a shape's lossRate is nan
    float lossKnee;     // hz: the loss doubles here at slope 1
    float lossSlope;    // the loss's power in frequency where a shape's lossSlope is nan
    float rankLoss;     // every t60 goes as (f_c4 / f0)^rankLoss
    float rankMass;     // the hammer's mass goes as (f_c4 / f0)^rankMass
    float rankStrike;   // the strike share goes as (f_c4 / f0)^rankStrike
    float l0;           // m: the speaking length at c4 where a shape's length is nan
    float bass0;        // m: the longest string where a shape's bass is nan
    float massC4;       // g: the hammer's mass at c4 where a shape's hammerMass is nan
    float tineSlope;    // the tine's t60 goes as ratio^-tineSlope
    float rankHardLo;   // the felt's stiffness goes as (f0 / f_c4)^rankHardLo below c4 ...
    float rankHardHi;   // ... and as (f0 / f_c4)^rankHardHi from c4 up
    float rankP;        // the felt's power adds rankP log2(f0 / f_c4) up the keyboard
    float coreGrow;     // a wound string's core goes as (mu / muBreak)^coreGrow, muBreak the plain wire's mass at dMax
    float t60ScaleMax;  // the rank's t60 scale is held under this (the deep bass)
    float bridgeMobility;   // m/s per N: the bridge's driving-point mobility, the unison's coupling (the rank's bridgeLoss)
    float bridgeReactive;   // the reactive part of the bridge's admittance over its resistive part (the coupling's pitch)
};
const LawConstants& lawConstants();

// pure: the partial table the instrument channels of s make at the string the rank gives
PartialTable partialsOf(const Shape& s, const RankState& r, const Tuning& t = Tuning());
PartialTable partialsOf(const Shape& s, const RankState& r, const Tuning& t, const LawConstants& c);

// the writer: when an instrument channel's target is newer than the last
// call, or the note moved (noteMoved), writes every partial channel under
// the sequence of the newest instrument write (a partial written by hand
// later stands); true when it wrote
class Law {
public:
    bool update(Channels& ch, const RankState& r, bool noteMoved, const Tuning& t = Tuning());
    bool pending(const Channels& ch) const;              // an instrument write newer than the last update (or none yet)
    void reset() { fresh_ = true; }

private:
    uint32_t seq_ = 0;
    bool     fresh_ = true;
};

// the pieces, for the tool and the tests
float stringRatio(int n, float B);                                                // n sqrt(1 + B n²)
float stringT60(int n, float ratio, float f0, const Shape& s, const LawConstants& c,
                float t60Scale);                                                  // the table's t60 at f_n = ratio f0 (the header's form)
float strikeGain(int n, float share);                                             // sin(n pi share), signed
float bridgeOut(int n);                                                           // n (-1)^(n+1)
float tineRatio(int n, float load);                                               // (beta_n / beta_1)², x load for n >= 2
float beamBeta(int n);                                                            // the clamped-free beam's beta_n L
float beamShape(int n, float share);                                              // the beam's shape at a share of its length, tip = ±2
float stringsOf(const Shape& s);                                                  // the strings channel capped on the electric side
float dampRateOf(float hz, const Shape& s, float pedal, const LawConstants& c);   // a released damper's added rate at hz
float hammerHardnessAt(float hardness, float generator);                          // the felt's stiffness with the tine's tip
float hammerFeltAt(float felt, float generator);                                  // the felt's loss with the tine's tip
float hammerExponentAt(float exponent, float pAdd);                               // the felt's power at a note, held kExponentLo .. Hi
inline bool partialOn(const PartialTable& p, int k) { return p.gain[k] != 0.0f || p.out[k] != 0.0f; }

}  // namespace up
