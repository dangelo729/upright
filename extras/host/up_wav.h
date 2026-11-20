// upright — up_wav.h
// a mono float buffer as a 16-bit pcm wav. host only.
#pragma once

#include <cstddef>

namespace up {

// n samples, each scaled by gain and held inside [-1, 1] before it is rounded to 16 bits
bool writeWav(const char* path, const float* samples, size_t n, int fs, float gain = 1.0f);

}  // namespace up
