// upright — up_wav.cpp  (plan §15)
#include "up_wav.h"

#include <cstdint>
#include <cstdio>

namespace up {
namespace {

// riff is little-endian
void le32(std::FILE* f, uint32_t v) {
    const uint8_t b[4] = {(uint8_t)(v & 0xFFu), (uint8_t)((v >> 8) & 0xFFu), (uint8_t)((v >> 16) & 0xFFu), (uint8_t)(v >> 24)};
    std::fwrite(b, 1, 4, f);
}
void le16(std::FILE* f, uint16_t v) {
    const uint8_t b[2] = {(uint8_t)(v & 0xFFu), (uint8_t)(v >> 8)};
    std::fwrite(b, 1, 2, f);
}

}  // namespace

bool writeWav(const char* path, const float* samples, size_t n, int fs, float gain) {
    std::FILE* f = std::fopen(path, "wb");
    if (!f) return false;
    const uint32_t bytes = (uint32_t)(n * 2u);
    std::fwrite("RIFF", 1, 4, f);
    le32(f, 36u + bytes);
    std::fwrite("WAVEfmt ", 1, 8, f);
    le32(f, 16u);                                        // the format chunk's size
    le16(f, 1u);                                         // pcm
    le16(f, 1u);                                         // mono
    le32(f, (uint32_t)fs);
    le32(f, (uint32_t)fs * 2u);                          // bytes a second
    le16(f, 2u);                                         // bytes a frame
    le16(f, 16u);                                        // bits a sample
    std::fwrite("data", 1, 4, f);
    le32(f, bytes);
    for (size_t i = 0; i < n; ++i) {
        float v = samples[i] * gain;
        v = v > 1.0f ? 1.0f : (v < -1.0f ? -1.0f : v);
        const float q = v * 32767.0f;
        le16(f, (uint16_t)(int16_t)(q >= 0.0f ? q + 0.5f : q - 0.5f));
    }
    const bool ok = std::ferror(f) == 0;
    return std::fclose(f) == 0 && ok;
}

}  // namespace up
