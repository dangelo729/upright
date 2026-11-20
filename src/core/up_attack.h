// upright — up_attack.h
// the hammer's attack: the sounds a strike makes that are not the string's
// partials. each is a force in newtons added to the voice's bridge force on
// the string side (x clamp(-g, 0, 1), as the bridge's weight is), so it
// reaches the board as the string's own force does (its modes through the
// note's bridge-point weights, the direct share and the field), and the
// pickup never hears it.
// tuning.attack (0 .. 2) scales the whole family; at 0 none of it runs, at 1
// it is as calibrated (below).
//
//   the knock   the blow's share reaching the board: the hammer's force
//               itself, kKnockForce x F (the mean of the sample's substeps)
//               while the hammer touches: the reaction through the shank,
//               the action rail and the frame, the hammer's own pulse at the
//               felt's own velocity dependence. at grand's c4, velocity 1,
//               it reads through the board over the first 10 ms 10 db under
//               the string's own
//   the thud    the key reaching its bed a few milliseconds after the hammer
//               meets the string in a normal touch (askenfelt & jansson,
//               "from touch to string vibrations"): a raised cosine kThudMs
//               wide from kThudDelayMs after the strike, its peak kThud m_h
//               v0 / kThudMs (m_h the rank's hammer mass, v0 the throw), into
//               the plate's lowest modes: at grand's c4, velocity 1, the
//               board's band under 150 hz over 5 .. 40 ms 4 db over the
//               string's own
//   the clank   the string's free longitudinal modes. its longitudinal waves
//               travel at c_L = sqrt(E A_core / mu) (5048 m/s on a plain
//               wire; a winding adds mass, not stiffness), so mode k sits at
//               k c_L / (2 L), k times the rank's longHz (1.2 khz at c2, 4.1
//               at c4, 14.6 at c6, over the ceiling from c7 up, off as every
//               partial over it is), decaying at 6.907755 / kLongT60. the
//               string's stretching drives them: the body force (E A / 2)
//               d/dx (dy/dx)², with the slope s = F / (2 T) over a share
//               kLongShare of L about the hammer, projects on mode k (shape
//               sin(k pi x / L), modal mass mu L / 2, omega_k = k pi c_L / L)
//               as an injection into im per substep of
//                   F² cos(k pi beta) x E A_core kLongShare dtSub / (4 T² mu c_L)
//               (beta the strike share: the k and the L cancel), so a blow
//               twice as hard drives it four times as hard; the bridge reads
//               the longitudinal force E A du/dx at x = L through the
//               string's bearing angle over the bridge:
//                   F_L = kLongAngle x E A_core (pi / L) sum k (-1)^k re_k
//               re in metres. kLongAngle is physical; kLongShare is set so
//               grand's a0, velocity 1, reads its clank through the board 10
//               db under the note's own over the first 50 ms
//
// not built: the phantom partials (the forced longitudinal response at the
// sums and differences of the transverse partials).
//
// a tuning.attack moved to 0 under a ringing clank ramps the outs to 0 across
// one block; a thud already armed finishes. per rate (init): the thud's width
// and turn; per block (setBlock): the modes' hz, decays, drives (a cos a
// mode) and outs, the bank's turns where they moved; per strike: the thud
// armed; per sample: the blow's mean, the thud's phasor, the bank's
// injection, turn and sum: multiplies and adds, no libm. parts() holds the
// last sample's three parts apart (the host's window).
#pragma once

#include "up_bank.h"
#include "up_config.h"
#include "up_rank.h"

namespace up {

constexpr float kKnockForce  = 0.983f;   // the blow's share: the hammer's force reaching the board through the action
constexpr float kThud        = 6.75f;    // the thud's share of the hammer's momentum, over its width
constexpr float kThudMs      = 10.0f;    // ms: the thud's width
constexpr float kThudDelayMs = 2.0f;     // ms: from the strike to the key's bed
constexpr float kLongT60     = 0.4f;     // s: the longitudinal modes' ring (bank & sujbert 2005: a few hundred ms; an estimate)
constexpr float kLongAngle   = 0.08f;    // the string's bearing angle over the bridge (about 4.6 degrees): a longitudinal force's
                                         //   share on the board
constexpr float kLongShare   = 0.0912f;  // the stretched share of the length about the hammer while it touches

// the last sample's parts of the attack's force (N), before the voice's weight
struct AttackParts {
    float blow = 0.0f, thud = 0.0f, clank = 0.0f;
};

class Attack {
public:
    void init(float fs);                                  // the rate: the thud's width and turn
    void reset();                                         // every phasor zero, nothing due
    // per block: the rank at the note (A_core, mu, L, T, longHz, the strike share), the ceiling (hz) and the family's
    // scale (0 .. 2: tuning.attack on the string side); the bank's modes set, on under the ceiling while anything sounds
    void setBlock(const RankState& r, float top, float scale);
    // a strike: the hammer's momentum m_h v0 (kg m/s); the thud from the next sample at this block's scale
    void strike(float momentum);
    bool running() const { return on_; }                  // this block: a mode on, the thud due, or the outs ramping
    // one sample while the hammer touches: its substeps' forces (N) into the blow's share and, squared, into the bank;
    // the force (N)
    inline float touching(const float* forces);
    // one sample after it: the bank's turn and the thud; the force (N)
    inline float step();

    const AttackParts& parts() const { return parts_; }
    float amplitude(int k) const { return long_.amplitude(k); }   // |z_k| of the bank, m
    const Bank<kLongModes>& bank() const { return long_; }

private:
    inline float rest();                                  // the thud, the parts summed

    Bank<kLongModes> long_;
    float dtSub_ = 1.0f / (kDefaultFs * kContactRate);
    float scale_ = 0.0f;                                  // this block's
    float blow_ = 0.0f;                                   // kKnockForce x the scale over kContactRate: a sample's substeps' sum to N
    bool  on_ = false;
    AttackParts parts_;
    // the thud: samples since the strike, its start and end; a sample's turn; its phasor; its peak over 2 (N)
    int   at_ = 0, thudFrom_ = 0, thudTo_ = 1;
    float thudCos_ = 1.0f, thudSin_ = 0.0f, tx_ = 1.0f, ty_ = 0.0f, thudHalf_ = 0.0f;
};

inline float Attack::rest() {
    float d = 0.0f;
    if (at_ < thudTo_) {
        if (at_ >= thudFrom_) {
            d = thudHalf_ - thudHalf_ * tx_;
            const float x = tx_ * thudCos_ - ty_ * thudSin_;
            ty_ = ty_ * thudCos_ + tx_ * thudSin_;
            tx_ = x;
        }
        ++at_;
    }
    parts_.thud = d;
    return parts_.blow + d + parts_.clank;
}

inline float Attack::touching(const float* forces) {
    float f = 0.0f;
    for (int r = 0; r < kContactRate; ++r) {
        f += forces[r];
        long_.inject(forces[r] * forces[r]);
        long_.turnSub();
    }
    float s, x, v;
    long_.sums<true, false>(s, x, v);
    parts_.blow = blow_ * f;
    parts_.clank = s;
    return rest();
}

inline float Attack::step() {
    float s, x, v;
    long_.step<true, false>(s, x, v);
    parts_.blow = 0.0f;
    parts_.clank = s;
    return rest();
}

}  // namespace up
