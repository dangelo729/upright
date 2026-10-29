// upright — up_png.cpp  (plan §15)
#include "up_png.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include "up_analysis.h"

namespace up {
namespace {

// crc-32 with png's reflected polynomial 0xedb88320, the table built on first use
uint32_t crcOf(const uint8_t* p, size_t n) {
    static uint32_t table[256];
    static bool built = false;
    if (!built) {
        for (uint32_t i = 0; i < 256; ++i) {
            uint32_t c = i;
            for (int k = 0; k < 8; ++k) c = (c & 1u) ? 0xEDB88320u ^ (c >> 1) : c >> 1;
            table[i] = c;
        }
        built = true;
    }
    uint32_t c = 0xFFFFFFFFu;
    for (size_t i = 0; i < n; ++i) c = table[(c ^ p[i]) & 0xFFu] ^ (c >> 8);
    return c ^ 0xFFFFFFFFu;
}

void be32(std::vector<uint8_t>& v, uint32_t x) {
    for (int s = 24; s >= 0; s -= 8) v.push_back((uint8_t)((x >> s) & 0xFFu));
}

// a chunk: its length, its type and data, the crc over the type and the data
void chunk(std::FILE* f, const char* type, const std::vector<uint8_t>& data) {
    std::vector<uint8_t> b;
    be32(b, (uint32_t)data.size());
    b.insert(b.end(), type, type + 4);
    b.insert(b.end(), data.begin(), data.end());
    be32(b, crcOf(b.data() + 4, b.size() - 4));
    std::fwrite(b.data(), 1, b.size(), f);
}

// dark blue through red to pale yellow, t in [0, 1]
void colour(double t, uint8_t* px) {
    static const double stops[5][3] = {{0, 0, 10}, {40, 20, 120}, {180, 40, 80}, {245, 140, 40}, {255, 245, 190}};
    t = std::min(1.0, std::max(0.0, t));
    const double s = t * 4.0;
    const int i = std::min(3, (int)s);
    const double u = s - (double)i;
    for (int c = 0; c < 3; ++c) px[c] = (uint8_t)std::lround(stops[i][c] + u * (stops[i + 1][c] - stops[i][c]));
}

}  // namespace

bool writePng(const char* path, const std::vector<uint8_t>& rgb, int width, int height) {
    // the rows, each with filter byte 0
    std::vector<uint8_t> raw;
    const size_t row = (size_t)width * 3;
    for (int y = 0; y < height; ++y) {
        raw.push_back(0);
        raw.insert(raw.end(), rgb.begin() + (std::ptrdiff_t)((size_t)y * row), rgb.begin() + (std::ptrdiff_t)((size_t)(y + 1) * row));
    }
    // zlib: its header, stored blocks of up to 65535 bytes, the adler-32 of the rows
    std::vector<uint8_t> z = {0x78, 0x01};
    for (size_t at = 0; at < raw.size();) {
        const size_t len = std::min<size_t>(65535, raw.size() - at);
        z.push_back(at + len == raw.size() ? 1 : 0);
        z.push_back((uint8_t)(len & 0xFFu));
        z.push_back((uint8_t)(len >> 8));
        z.push_back((uint8_t)(~len & 0xFFu));
        z.push_back((uint8_t)((~len >> 8) & 0xFFu));
        z.insert(z.end(), raw.begin() + (std::ptrdiff_t)at, raw.begin() + (std::ptrdiff_t)(at + len));
        at += len;
    }
    uint32_t a = 1, b = 0;
    for (uint8_t c : raw) {
        a = (a + c) % 65521u;
        b = (b + a) % 65521u;
    }
    be32(z, (b << 16) | a);
    std::FILE* f = std::fopen(path, "wb");
    if (!f) return false;
    const uint8_t signature[8] = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n'};
    std::fwrite(signature, 1, 8, f);
    std::vector<uint8_t> head;
    be32(head, (uint32_t)width);
    be32(head, (uint32_t)height);
    const uint8_t rest[5] = {8, 2, 0, 0, 0};             // 8 bits, rgb, deflate, filters, no interlace
    head.insert(head.end(), rest, rest + 5);
    chunk(f, "IHDR", head);
    chunk(f, "IDAT", z);
    chunk(f, "IEND", {});
    const bool ok = std::ferror(f) == 0;
    return std::fclose(f) == 0 && ok;
}

bool writeSpectrogram(const char* path, const float* x, size_t n, double fs, double topHz) {
    const size_t frame = 2048;
    const size_t hop = std::max<size_t>(64, n / 1600 + 1);
    const int width = (int)(n / hop);
    const int bins = (int)(topHz * (double)frame / fs);
    const int scale = 2, strip = 14;                     // rows a bin; the tick strip under the image
    const int height = bins * scale + strip;
    if (width < 2 || bins < 2) return false;
    std::vector<double> db((size_t)width * (size_t)bins);
    std::vector<float> buf(frame);
    double top = -400.0;
    for (int c = 0; c < width; ++c) {
        for (size_t i = 0; i < frame; ++i) {
            const long t = (long)((size_t)c * hop + i) - (long)(frame / 2);
            buf[i] = t >= 0 && t < (long)n ? x[t] : 0.0f;
        }
        const std::vector<double> mag = magnitude(buf.data(), frame, frame, Window::hann);
        for (int k = 0; k < bins; ++k) {
            const double v = 20.0 * std::log10(mag[(size_t)k] + 1e-20);
            db[(size_t)c * (size_t)bins + (size_t)k] = v;
            top = std::max(top, v);
        }
    }
    std::vector<uint8_t> rgb((size_t)width * (size_t)height * 3, 0);
    for (int c = 0; c < width; ++c) {
        for (int k = 0; k < bins; ++k) {
            uint8_t px[3];
            colour((db[(size_t)c * (size_t)bins + (size_t)k] - (top - 80.0)) / 80.0, px);
            const bool khz = (int)((double)k * fs / (double)frame / 1000.0) != (int)((double)(k + 1) * fs / (double)frame / 1000.0);
            if (khz && c % 4 == 0) px[0] = px[1] = px[2] = 100;
            for (int r = 0; r < scale; ++r) {
                const size_t o = (((size_t)(bins - 1 - k) * scale + (size_t)r) * (size_t)width + (size_t)c) * 3;
                rgb[o] = px[0];
                rgb[o + 1] = px[1];
                rgb[o + 2] = px[2];
            }
        }
        // a tick every 100 ms, long every second
        const long ms = (long)((double)c * (double)hop * 1000.0 / fs), before = (long)((double)(c - 1) * (double)hop * 1000.0 / fs);
        if (c == 0 || ms / 100 == before / 100) continue;
        const bool second = ms / 1000 != before / 1000;
        for (int y = bins * scale + (second ? 0 : strip / 2); y < height; ++y) {
            const size_t o = ((size_t)y * (size_t)width + (size_t)c) * 3;
            rgb[o] = rgb[o + 1] = rgb[o + 2] = 230;
        }
    }
    return writePng(path, rgb, width, height);
}

}  // namespace up
