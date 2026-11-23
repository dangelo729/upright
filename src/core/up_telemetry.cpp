// upright — up_telemetry.cpp
// the string's shape for the picture: a sin a partial (and the beam's shape
// on the electric side) a point, at the app's call rate, never per sample.
#include "up_telemetry.h"

#include "up_law.h"

namespace up {

float stringAt(const float* re, int count, const Shape& eased, float share) {
    const float g = clamp(eased[Channel::generator] == eased[Channel::generator] ? eased[Channel::generator] : -1.0f, -1.0f, 1.0f);
    const float u = clamp(g + 1.0f, 0.0f, 1.0f);             // 0 the string, 1 the tine and the reed
    float y = 0.0f;
    for (int k = 0; k < count && k < kMaxModes; ++k) {
        if (re[k] == 0.0f) continue;
        const int n = k + 1;
        const float s = strikeGain(n, share);                 // sin(n pi share): the string's shape
        float shape = s;
        if (n <= kTineModes && u > 0.0f) {
            const float b = 0.5f * beamShape(n, share);       // the beam's, over the tip's 2
            shape = u >= 1.0f ? b : s + (b - s) * u;
        }
        y += re[k] * shape;
    }
    return y;
}

int stringShape(const State& st, float* y, int n) {
    if (!y || n <= 0) return 0;
    for (int i = 0; i < n; ++i) y[i] = stringAt(st.re, kMaxModes, st.shape, n > 1 ? (float)i / (float)(n - 1) : 0.0f);
    return n;
}

}  // namespace up
