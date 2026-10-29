// upright — up_hammer.h  (plan §4)
// the hammer: a mass on a nonlinear felt spring that meets the string at a
// point. with c the felt's compression (the hammer's height over the
// string's under it, positive while they touch), the hunt & crossley force
//
//     F = K c^p (1 + lambda dc/dt)   while c > 0, never negative (a felt cannot pull)
//
// pushes the string and throws the hammer back. si inside (metres,
// kilograms, newtons, seconds); the channels' units (N/mm^p, s/mm, grams)
// are converted in set().
//
// one step: the force is read from the compression before the step, the
// string's displacement the caller passes being the last step's (the
// one-step delay that breaks the delay-free loop); then symplectic euler,
// v -= F / m dt, y += v dt. the contact ends when c <= 0 after having been
// positive and the hammer moves back (its velocity under 0: a string that
// runs ahead of a hammer still coming is met again), or kMaxContact after
// place() whatever happens. a guard caps the force at kForceCap.
//
// the felt's loss in the engine's step is solved against the string's reply:
// with a = K c^p, the string point's own velocity ydot (the caller's: the
// modal string's quadratures read at the strike point, no lag) and m_point
// its point mass under the hammer (setPointMass: the modal string's mu L / 2
// over the sum of its gains² there), the string's velocity across the step
// is ydot + F dt / (2 m_point) on the mean, so with u = v_h - ydot
//
//     F = a (1 + lambda u) / (1 + lambda a dt / (2 m_point))
//
// one division: the loop with the light string point closed in closed form,
// stable at any step and second order (the trapezoid), where the explicit
// lambda cdot (stepExact, ydot from the last two readbacks: the host's truth
// at a high rate) rings at the sample rate. with lambda 0 the force is the
// explicit one bit for bit.
//
// c^p without a per-sample libm call: c^p = sqrt(c)^w x g(c), w = floor(2p)
// by multiplications, g(c) = c^(p - w / 2) from a kFeltPoints table over 0 ..
// kCmax spaced in the square root of c (dense near zero), linear between
// points in the square root; the table is built in set(), per hammer write.
// set() and forceExact() call libm; step() calls none (sqrtf indexes the
// table, a single instruction).
#pragma once

#include "up_config.h"

namespace up {

constexpr float kCmax       = 4e-3f;    // m: the felt table's reach (4 mm)
constexpr int   kFeltPoints = 64;       // the felt table's points
constexpr float kMaxContact = 0.02f;    // s: a strike ends here whatever happens
constexpr float kForceCap   = 2000.0f;  // N: the force in one step is held here: five times the converged chart's largest peak

struct Hammer {
    // the felt and the mass in the channels' units; builds the table (per write)
    void set(float hardnessNmmP, float exponent, float feltSperMm, float massG);
    // the string's point mass under the hammer (kg), for the felt's implicit loss
    void setPointMass(float kg);
    // at height y0 (m, the string's rest by default) with velocity v0 (m/s) toward the string; touching from the next step
    void place(float v0, float y0 = 0.0f);
    // off the string at once: not touching, no force
    void stop() { active_ = false; force_ = 0.0f; }
    // one step: the string's displacement and velocity under the hammer (m, m/s), dt; returns
    // the force on the string (N), 0 when not touching. the felt's loss implicit
    float step(float yString, float ydotString, float dt);
    // a step with the force by powf and the loss explicit, ydot from the last two readbacks: the host's
    // ground truth at a high rate (up_contact.h)
    float stepExact(float yString, float dt);

    bool  touching() const { return active_; }      // placed and not yet left
    bool  contacted() const { return contacted_; }  // the felt has been compressed since place()
    float height() const { return y_; }              // m, from the string's rest
    float velocity() const { return v_; }            // m/s, toward the string positive
    float lastForce() const { return force_; }       // N
    float elapsed() const { return time_; }          // s since place()

    float forceOf(float cM, float cdotMps) const;    // K c^p (1 + lambda cdot) by the table, at 0 under no compression
    float forceExact(float cM, float cdotMps) const; // the same by powf, for the tests

    float stiffness() const { return k_; }           // N/m^p
    float power() const { return p_; }
    float loss() const { return lambda_; }           // s/m
    float mass() const { return mass_; }             // kg
    float pointMass() const { return point_; }       // kg

private:
    float advance(float yString, float ydotString, float dt, bool exact);

    float k_ = 0.0f, p_ = 2.5f, lambda_ = 0.0f, mass_ = 6e-3f, point_ = 1e-3f, halfOverPoint_ = 500.0f;
    int   whole_ = 5;                                // floor(2p): the power's half-steps by multiplication
    float uScale_ = 0.0f;                            // the table's index per sqrt(m)
    float table_[kFeltPoints] = {};                  // c^(p - whole / 2) at c_i = kCmax (i / (kFeltPoints - 1))²
    float y_ = 0.0f, v_ = 0.0f, force_ = 0.0f, yPrev_ = 0.0f, time_ = 0.0f;
    bool  active_ = false, contacted_ = false, first_ = false;
};

}  // namespace up
