// upright — up_png.h
// a spectrogram as a png with nothing but the standard library: 8-bit rgb
// rows in a zlib stream of stored (uncompressed) deflate blocks. host only.
#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace up {

// rgb: width x height x 3 bytes, the top row first
bool writePng(const char* path, const std::vector<uint8_t>& rgb, int width, int height);

// 0 .. topHz up the image, time across it: 2048-point hann frames, the log
// magnitude over 80 db under the loudest bin; a tick under the image every
// 100 ms (a long one every second) and a dotted line every khz
bool writeSpectrogram(const char* path, const float* x, size_t n, double fs, double topHz = 8000.0);

}  // namespace up
