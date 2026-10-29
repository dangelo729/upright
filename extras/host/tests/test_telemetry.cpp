// upright — tests/test_telemetry.cpp  (plan §10; the tests' "telemetry" bullet)
//
// after a strike each main partial's amplitude is its drive times the
// force's spectrum at its frequency (the force read at every substep), the
// decay since undone, within 1 db; the hammer's force integrates to its
// momentum change within 1 %; the string's shape at the strike point is the
// hammer's readback within 1e-6 mm; the state reads back the note, the rank,
// the partials on and the hammer.
#include <cmath>
#include <complex>
#include <initializer_list>
#include <vector>

#include "core/up_knobs.h"
#include "core/up_telemetry.h"
#include "core/up_voice.h"
#include "up_fit.h"
#include "up_test.h"

using namespace up;

namespace {

const Knobs kGrand = {{0.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f}};

}  // namespace

// grand at c3, c4 and c5, velocity 0.4 and 1: the voice rendered a sample at
// a time while the hammer touches, its force read at every substep; at the
// end of the block it leaves in, every main partial on with a drive: |z_k| =
// drive_k |sum_n F_n e^((sigma_k + i w_k) n dtSub)| e^(-sigma_k t) within 1
// db (n the substeps since the strike, t the time since it, sigma_k its
// decay)
UP_TEST(telemetry_amplitude_is_drive_times_force) {
    double worst = 0.0;
    int partials = 0, strikes = 0;
    for (float midi : {48.0f, 60.0f, 72.0f})
        for (float vel : {0.4f, 1.0f}) {
            Voice v(4);
            v.setShape(pianoOf(kGrand));
            v.noteOn(midi, vel);
            std::vector<double> force;
            int samples = 0;
            float b, p;
            do {
                v.render(&b, &p, 1);
                ++samples;
                for (int r = 0; r < kContactRate; ++r) force.push_back(v.strings().sampleTouched() ? v.strings().substepForces()[r] : 0.0);
            } while (v.strings().touching() || samples % kBlock != 0);
            ++strikes;
            const Bank<kMaxModes>& main = v.strings().main();
            const State st = v.state();
            const double dtSub = v.strings().dtSub(), elapsed = (double)samples / kDefaultFs;
            for (int k = 0; k < kMaxModes; ++k) {
                if (!main.on(k) || main.drive(k) == 0.0f) continue;
                const double w = main.omega(k);
                const double sigma = 6.907755 / (st.shape[t60Channel(k)] * st.rank.t60Scale);
                std::complex<double> sum = 0.0;
                for (size_t n = 0; n < force.size(); ++n) sum += force[n] * std::exp(sigma * dtSub * (double)n) * std::polar(1.0, w * dtSub * (double)n);
                const double want = main.drive(k) * std::abs(sum) * std::exp(-sigma * elapsed);
                if (!(want > 1e-12)) continue;
                const double off = 20.0 * std::log10(st.amp[k] / want);
                worst = std::fmax(worst, std::fabs(off));
                ++partials;
                CHECK_MSG(std::fabs(off) < 1.0, "midi %.0f velocity %.1f partial %d: %+.3f db off drive x the force's spectrum", midi, vel, k + 1, off);
            }
        }
    printf("    %d strikes, %d partials: each amplitude within %.4f db of drive x |F(w)| x its decay\n", strikes, partials, worst);
}

// the hammer's force, integrated over its substeps from the strike to its leaving (the strings' impulse), is
// its momentum change m (v0 - v_leaving) within 1 %, at c2, c4, c6, velocity 0.3 and 1
UP_TEST(telemetry_force_is_the_momentum) {
    double worst = 0.0;
    for (float midi : {36.0f, 60.0f, 84.0f})
        for (float vel : {0.3f, 1.0f}) {
            Voice v(4);
            v.setShape(pianoOf(kGrand));
            v.noteOn(midi, vel);
            float b[kBlock], p[kBlock];
            v.render(b, p, kBlock);
            for (int i = 0; i < 20 && v.strings().touching(); ++i) v.render(b, p, kBlock);
            const Hammer& h = v.strings().hammer();
            const double v0 = Tuning().hammerScale * std::pow(vel, Tuning().velCurve);
            const double momentum = h.mass() * (v0 - h.velocity());
            const double e = v.strings().impulse() / momentum - 1.0;
            worst = std::fmax(worst, std::fabs(e));
            CHECK_MSG(std::fabs(e) < 0.01, "midi %.0f velocity %.1f: the impulse %.6g against the momentum %.6g", midi, vel, v.strings().impulse(),
                      momentum);
        }
    printf("    the force's integral against the hammer's momentum change: %.2e at worst\n", worst);
}

// the main string's shape at the strike point (stringAt at the rank's share)
// is the hammer's readback (sum gain re over the partials on) within 1e-6 mm,
// at the end of every block of a strike's first 50 ms; stringShape's ends
// are the string's fixed ends; the state's note, rank and partials on
UP_TEST(telemetry_shape_at_the_strike_point) {
    Voice v(4);
    v.setShape(pianoOf(kGrand));
    v.noteOn(60.0f, 0.8f);
    float b[kBlock], p[kBlock];
    double worst = 0.0, largest = 0.0;
    for (int i = 0; i < 17; ++i) {
        v.render(b, p, kBlock);
        const double ours = v.stringAt(v.rank().strikeShare), hammer = v.strings().main().readback();
        worst = std::fmax(worst, std::fabs(ours - hammer));
        largest = std::fmax(largest, std::fabs(hammer));
    }
    CHECK_MSG(worst < 1e-6, "the shape at the strike point %.3g mm off the readback (it reaches %.3g mm)", worst, largest);
    float y[33];
    CHECK(v.stringShape(y, 33) == 33 && std::fabs(y[0]) < 1e-6f && std::fabs(y[32]) < 1e-4f * (float)largest);
    const State st = v.state();
    int on = 0;
    for (int k = 0; k < kMaxModes; ++k) on += v.strings().main().on(k);
    CHECK(st.f0 == midiToHz(60.0f) && st.modesOn == on && st.rank.f0 == st.f0 && !st.asleep && st.hammerContact > 0.0f);
    printf("    the shape at the strike point against the readback: %.2e mm at worst over 17 blocks (the string to %.3f mm); %d partials on\n", worst,
           largest, on);
}
