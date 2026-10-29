// upright — up_bank.cpp  (plan: module layout, §4)
// the bank's one libm function: a partial's turn, per block and only where
// its hz or its decay moved. the turn itself, the readback, the injection
// and the sums are the header's, multiplies and adds.
#include "up_bank.h"

#include <cmath>

namespace up {

// r e^(-i w dt) - 1: ca = r cos(w dt) - 1 from expm1 and sin²(w dt / 2), so it
// keeps its digits where w dt is small and r near 1; cb = r sin(w dt)
void rotationOf(float hz, float decayPerSec, float dt, float& ca, float& cb) {
    const float w = 2.0f * kPi * hz * dt;
    const float em1 = std::expm1(-decayPerSec * dt);
    const float r = 1.0f + em1;
    const float h = std::sin(0.5f * w);
    ca = em1 - 2.0f * r * h * h;
    cb = r * std::sin(w);
}

}  // namespace up
