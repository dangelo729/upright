// upright — tests/test_arduino.cpp
//
// the umbrella on an arduino core without the teensy audio library: ARDUINO
// defined and no <AudioStream.h> (this host has none). the teensy wrappers'
// headers declare nothing, so their names are free for this file's own, and
// the core compiles and plays.
#define ARDUINO 10819

#include <vector>

#include "Upright.h"
#include "up_analysis.h"
#include "up_test.h"

struct AudioSynthUpright {};                     // a second definition if the wrapper's header had declared one
template <int M> struct AudioSynthUprightPool {};

using namespace up;

UP_TEST(arduino_core_without_the_audio_library) {
    Voice v;
    v.setShape(grand());
    v.noteOn(60.0f, 0.8f);
    std::vector<float> x((size_t)(8 * kBlock));
    v.render(x.data(), nullptr, (int)x.size());
    CHECK(rms(x.data(), x.size()) > 0.0 && allFinite(x.data(), x.size()));
    printf("    ARDUINO defined, no AudioStream.h: Upright.h gives the core alone, and grand's c4 sounds\n");
}
