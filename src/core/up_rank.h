// upright — up_rank.h  (plan §6)
// the rank: a keyboard of strings is a scale design, so the instrument's
// channels describe the string at c4 and the note chooses one. pure, once
// per note and once per instrument write, never per sample.
//
//   length        L = min(length (f_c4 / f0)^rankLength, bass): pure scaling
//                 until the case's longest string stops it
//   mass          every string is strung to `tension`: mu = tension / (4 L² f0²),
//                 the impedance Z = tension / (2 L f0)
//   the wire      a plain wire of that mass has d = sqrt(4 mu / (pi kRho)) x
//                 gauge; past dMax the string is wrapped: a core of `core` x
//                 (mu / muBreak)^coreGrow (muBreak = pi kRho dMax² / 4, the
//                 plain wire's mass at the break: the core thickens as the
//                 winding grows), the rest of the mass winding, the stiffness
//                 the core's
//   stiffness     B = pi³ kE dCore⁴ / (64 tension L²)
//   the hammer    mass = hammerMass (f_c4 / f0)^rankMass; the felt's
//                 stiffness x (f0 / f_c4)^rankHardLo below c4 and
//                 ^rankHardHi from c4 up, and its power + rankP
//                 log2(f0 / f_c4) (the treble's hammers are harder); the
//                 strike share strikeShare (f_c4 / f0)^rankStrike, falling up
//                 the keyboard (it may pass 0.5 at the top from a share
//                 near 0.5: the string is symmetric, the tine longer)
//   decays        every t60 scales by (f_c4 / f0)^rankLoss, held under
//                 t60ScaleMax
//   the bridge    pi tension / L: the bridge force's scale on the law's
//                 dimensionless outs, N per metre of modal displacement; and
//                 the string's loss into it, tension x bridgeMobility / L
//                 (1/s): Z = tension / (2 L f0) into a mobility Y loses 2 Z Y
//                 f0 a second on every partial (the unison's coupling)
//   the damper    a key above damperTop has none
//   longitudinal  the wire's longitudinal waves at c_L = sqrt(kE A_core / mu),
//                 A_core = pi dCore² / 4 (a winding adds mass, not stiffness;
//                 sqrt(kE / kRho) = 5048 m/s on a plain wire): the free
//                 longitudinal fundamental c_L / (2 L), the attack's clank
//                 (up_attack.h)
//
// what the fixed tension implies: at a fixed pitch and tension the plain
// wire's mass goes as 1 / L², its diameter's fourth power as 1 / L⁴, and B
// as 1 / L⁶; on a wound string the core goes as mu^coreGrow, so B goes as
// 1 / L^(2 + 8 coreGrow). the length and bass
// channels move B by those powers, and move the bass break (where the plain
// wire passes dMax) with it. on the electric side the rank computes the same
// numbers: the beam's ratios are the law's business, its t60s scale as the
// string's.
#pragma once

#include "up_channels.h"
#include "up_law.h"

namespace up {

struct RankState {
    float f0;                            // hz at the note
    float L, mu, Z;                      // m, kg/m, kg/s
    float d;                             // m: the plain wire's diameter at that mass (x gauge)
    float dCore;                         // m: the wire the stiffness is the wire's (the core when wrapped)
    float B;                             // the inharmonicity coefficient
    float tension;                       // N
    bool  wrapped;                       // the plain wire would pass dMax: a wound string on a core
    float hammerMass;                    // kg at the note
    float hardnessScale;                 // (f0 / f_c4)^rankHardLo or ^rankHardHi: the felt's stiffness at the note over the channel's
    float pAdd;                          // rankP log2(f0 / f_c4): the felt's power at the note over the channel's
    float strikeShare;                   // the hammer's landing as a share of L at the note
    float t60Scale;                      // (f_c4 / f0)^rankLoss held under t60ScaleMax: every t60 channel times this
    float bridgeScale;                   // pi tension / L, N per metre of modal displacement
    float bridgeLoss;                    // 1/s: tension x bridgeMobility / L, one string's amplitude decay into the bridge
    bool  damped;                        // the key has a damper (midi <= damperTop)
    float longHz;                        // hz: the free longitudinal fundamental, c_L / (2 L)
};

RankState rankAt(const Shape& s, float midi);                           // the instrument channels read at a note
RankState rankAt(const Shape& s, float midi, const LawConstants& c);

}  // namespace up
