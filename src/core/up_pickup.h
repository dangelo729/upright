// upright — up_pickup.h  (plan §7)
// the electric side's output: a curve of the tip's position differentiated
// in time. the tip's displacement x and its velocity x' come from the bank's
// two sums; the voltage is v = C'(x) x' (faraday's law for the magnet, the
// bias times the capacitance's rate for the plate), then the amp's one pole.
//
//   magnetic (generator 0)        C(x) = 1 / (1 + ((x - offset) / gap)²): a
//                                 lorentzian in the tip's lateral position past
//                                 the pole. centred the output is even in x and
//                                 the fundamental vanishes; far off-centre it
//                                 is linear (a pure tone); between, the rhodes
//   electrostatic (generator +1)  C(x) = 1 / (1 + ((x - gap) / gap)²): the same
//                                 lorentzian centred at the gap, the reed at
//                                 rest a gap from the plate's plane where the
//                                 capacitance peaks; bounded, |C'| under 0.65 /
//                                 gap everywhere
//   between                       one lorentzian whose offset runs from the
//                                 magnet's to the gap: offset + w (gap - offset),
//                                 w = clamp(g, 0, 1)
//
// x and offset and gap in mm, x' in mm/s. per sample: one division and a few
// multiplies, no libm; AmpPole::set() runs per tuning write and calls libm.
//
// the stage the voice runs (Pickup): the block's eased generator, gap and
// offset set the curve's offset once a block; sample(x, x') is u = (x -
// offset) / gap, v = -2 u x' / (gap (1 + u²)²): pickupVoltage's value with one
// division; then the pole and kPickupGain.
#pragma once

namespace up {

// the pickup's voltage to the voice's output: rhodes at e4 and velocity 1 peaks near 0.3 of full scale
constexpr float kPickupGain = 1.36e-4f;

float magneticC(float xMm, float gapMm, float offsetMm);      // 1 / (1 + ((x - offset) / gap)²)
float magneticDeriv(float xMm, float gapMm, float offsetMm);  // dC/dx, per mm
float electroC(float xMm, float gapMm);                       // 1 / (1 + ((x - gap) / gap)²)
float electroDeriv(float xMm, float gapMm);                   // dC/dx, per mm
float pickupOffset(float generator, float gapMm, float offsetMm);   // the curve's centre: offset + clamp(g, 0, 1) (gap - offset)
// the lorentzian at the crossfaded offset, its derivative times x'
float pickupVoltage(float xMm, float xdotMmps, float generator, float gapMm, float offsetMm);

// the amp's one pole, a lowpass whose -3 db point sits at hz exactly:
// y = (1 - b) x + b y, b from cos(2 pi hz / fs)
struct AmpPole {
    void  set(float hz, float fs);
    float next(float x) { y_ = a_ * x + b_ * y_; return y_; }
    void  reset() { y_ = 0.0f; }
    float coef() const { return b_; }

private:
    float a_ = 1.0f, b_ = 0.0f, y_ = 0.0f;
};

// the pickup as the voice runs it: one block's curve, then the pole and the gain
struct Pickup {
    void setBlock(float generator, float gapMm, float offsetMm);   // per block: the eased channels
    void setPole(float hz, float fs) { pole.set(hz, fs); }         // per tuning write
    void reset() { pole.reset(); }
    // one sample: the tip's displacement (mm) and velocity (mm/s) to the output
    inline float sample(float x, float xdot) {
        const float u = (x - centre) * invGap;
        const float q = 1.0f + u * u;
        const float v = minusTwoOverGap * u * xdot / (q * q);
        return pole.next(v) * kPickupGain;
    }

    float   centre = 0.0f, invGap = 1.0f, minusTwoOverGap = -2.0f;   // the curve's offset, 1 / gap, -2 / gap
    AmpPole pole;
};

}  // namespace up
