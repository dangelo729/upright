// upright — up_analysis.h  (plan §15)
// the rig's instruments, shared by uptool and the tests. host only.
//
//   fft            radix-2, in place, double, forward (e^-i)
//   magnitude      |X(k)| of a stretch, windowed (hann for a steady sound,
//                  the falling half of a hann for a struck one), zero-padded
//   peaks          the largest local maximum within so many cents of a
//                  frequency, its bin parabola-interpolated on the log
//                  magnitude
//   bands          a narrow band's analytic signal from one fft of the whole
//                  record (its bins kept, tapered at the edges, moved to the
//                  bottom of a smaller transform and back), its rms over
//                  frames: a partial's envelope
//   t60            least squares on the envelope in db from 6 db under its
//                  peak to 40 db under (or as far as it falls, 14 db at least)
//   two slopes     the envelope's slope (db/s) by least squares over its
//                  first second after the peak (the early) and over the rest
//                  down to 60 db under the peak (the late): the two-stage decay
//   the beat       the envelope's ripple: the db envelope less its own
//                  smooth trend (a moving mean over a window), the ripple's
//                  depth (its largest swing) and its period from the spacing
//                  of its troughs (one a dip under the mean, at its lowest,
//                  placed by a parabola)
//   the tracker    each expected partial's measured peak within 50 cents, its
//                  cents off, its level as the strike leaves it (the
//                  envelope's first frame, db full scale), its t60
//   thd            the harmonics 2 .. 8 of f1 against the first, each by a
//                  hann-windowed dft at its exact frequency
//   levels         the rms, the peak, the largest first difference; the click
//                  measure: the worst 10 ms frame's largest first difference
//                  over that frame's rms (a sampled sine at f reads 2 sqrt(2)
//                  sin(pi f / fs) there, so the bound for a sound whose top
//                  partial is f is 1.5 times that)
#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

namespace up {

constexpr double kTwoPi = 6.283185307179586;

inline size_t nextPow2(size_t n) {
    size_t p = 1;
    while (p < n) p <<= 1;
    return p;
}

// in place, n a power of two: the bit-reversal shuffle, then the butterflies
inline void fft(std::vector<double>& re, std::vector<double>& im) {
    const size_t n = re.size();
    if (n < 2) return;
    for (size_t i = 1, j = 0; i < n; ++i) {
        size_t bit = n >> 1;
        for (; j & bit; bit >>= 1) j ^= bit;
        j |= bit;
        if (i < j) {
            std::swap(re[i], re[j]);
            std::swap(im[i], im[j]);
        }
    }
    std::vector<double> wr(n / 2), wi(n / 2);
    for (size_t k = 0; k < n / 2; ++k) {
        wr[k] = std::cos(-kTwoPi * (double)k / (double)n);
        wi[k] = std::sin(-kTwoPi * (double)k / (double)n);
    }
    for (size_t len = 2; len <= n; len <<= 1) {
        const size_t half = len >> 1, step = n / len;
        for (size_t i = 0; i < n; i += len)
            for (size_t k = 0; k < half; ++k) {
                const double cr = wr[k * step], ci = wi[k * step];
                const size_t a = i + k, b = a + half;
                const double tr = re[b] * cr - im[b] * ci, ti = re[b] * ci + im[b] * cr;
                re[b] = re[a] - tr;
                im[b] = im[a] - ti;
                re[a] += tr;
                im[a] += ti;
            }
    }
}

enum class Window { none, hann, fallingHann };

// |X(k)| for k = 0 .. size / 2 of x[0 .. count) windowed over the count, zero-padded to size (a power of two)
inline std::vector<double> magnitude(const float* x, size_t count, size_t size, Window window) {
    std::vector<double> re(size, 0.0), im(size, 0.0);
    const size_t n = count < size ? count : size;
    for (size_t i = 0; i < n; ++i) {
        double w = 1.0;
        if (window == Window::hann && n > 1) w = 0.5 - 0.5 * std::cos(kTwoPi * (double)i / (double)(n - 1));
        if (window == Window::fallingHann && n > 1) w = 0.5 + 0.5 * std::cos(0.5 * kTwoPi * (double)i / (double)(n - 1));
        re[i] = (double)x[i] * w;
    }
    fft(re, im);
    std::vector<double> mag(size / 2 + 1);
    for (size_t k = 0; k < mag.size(); ++k) mag[k] = std::sqrt(re[k] * re[k] + im[k] * im[k]);
    return mag;
}

// a peak's centre between bins: the parabola through the log magnitudes of bin k and its neighbours
inline double interpolatedBin(const std::vector<double>& mag, size_t k) {
    if (k == 0 || k + 1 >= mag.size()) return (double)k;
    const double a = std::log(mag[k - 1] + 1e-300), b = std::log(mag[k] + 1e-300), c = std::log(mag[k + 1] + 1e-300);
    const double den = a - 2.0 * b + c;
    return den < 0.0 ? (double)k + 0.5 * (a - c) / den : (double)k;
}

struct Peak {
    double hz = 0.0;            // interpolated; 0: none
    double db = -300.0;         // the bin's magnitude in db
};

// the largest local maximum within +-cents of hz
inline Peak peakNear(const std::vector<double>& mag, double fs, double hz, double cents) {
    const double binHz = fs / (double)((mag.size() - 1) * 2);
    const double r = std::exp2(cents / 1200.0);
    const size_t a = (size_t)std::max(1.0, std::ceil(hz / r / binHz));
    size_t b = (size_t)std::floor(hz * r / binHz);
    if (b + 1 >= mag.size()) b = mag.size() - 2;
    Peak p;
    size_t best = 0;
    for (size_t k = a; k <= b; ++k)
        if (mag[k] > mag[k - 1] && mag[k] >= mag[k + 1] && (best == 0 || mag[k] > mag[best])) best = k;
    if (best) {
        p.hz = interpolatedBin(mag, best) * binHz;
        p.db = 20.0 * std::log10(mag[best] + 1e-300);
    }
    return p;
}

// ---- envelopes -------------------------------------------------------------------------------------

struct Envelope {
    double frameSec = 0.05;
    std::vector<double> db;     // each frame's band rms as a sine's amplitude, db full scale
};

// one forward fft of a record (zero-padded to twice its length, so the
// bands' analytic signals do not wrap); each band's envelope from its bins
class BandAnalyser {
public:
    BandAnalyser(const float* x, size_t n, double fs) : n_(n), fs_(fs), size_(nextPow2(2 * (n > 0 ? n : 1))), re_(size_, 0.0), im_(size_, 0.0) {
        for (size_t i = 0; i < n; ++i) re_[i] = x[i];
        fft(re_, im_);
    }

    // the band [lo, hi] hz: its bins doubled (the analytic signal), tapered
    // over the outer quarter at each edge (a raised cosine), moved to the
    // bottom of a transform of m points and back: |z| across the record; the
    // frames' mean of |z|², as a sine's amplitude in db
    Envelope envelope(double lo, double hi, double frameSec = 0.05) const {
        Envelope e;
        e.frameSec = frameSec;
        const double binHz = fs_ / (double)size_;
        const size_t a = (size_t)std::max(1.0, std::ceil(lo / binHz));
        const size_t b = std::min((size_t)std::floor(hi / binHz), size_ / 2 - 1);
        if (b < a) return e;
        const size_t bins = b - a + 1;
        size_t m = nextPow2(std::max<size_t>(bins, 4096));
        while ((double)size_ / (double)m / fs_ > frameSec / 8.0) m <<= 1;   // eight samples a frame at least
        std::vector<double> zr(m, 0.0), zi(m, 0.0);
        for (size_t j = 0; j < bins; ++j) {
            const double u = ((double)j + 0.5) / (double)bins, edge = std::min(u, 1.0 - u);
            const double w = edge >= 0.25 ? 1.0 : 0.5 - 0.5 * std::cos(0.5 * kTwoPi * edge / 0.25);
            zr[j] = 2.0 * w * re_[a + j];
            zi[j] = -2.0 * w * im_[a + j];                // conjugated: the inverse by the forward
        }
        fft(zr, zi);
        const double scale = 1.0 / (double)size_;
        const double dt = (double)size_ / (double)m / fs_;
        const size_t frames = (size_t)((double)n_ / fs_ / frameSec);
        for (size_t f = 0; f < frames; ++f) {
            const size_t s0 = (size_t)std::ceil((double)f * frameSec / dt), s1 = (size_t)std::ceil((double)(f + 1) * frameSec / dt);
            double sum = 0.0;
            size_t cnt = 0;
            for (size_t s = s0; s < s1 && s < m; ++s, ++cnt) sum += (zr[s] * zr[s] + zi[s] * zi[s]) * scale * scale;
            e.db.push_back(20.0 * std::log10((cnt ? std::sqrt(sum / (double)cnt) : 0.0) + 1e-30));
        }
        return e;
    }

private:
    size_t n_;
    double fs_;
    size_t size_;
    std::vector<double> re_, im_;
};

struct DecayFit {
    double t60 = 0.0;           // s; 0: the ring is too short (or too long) to measure here
    double spanDb = 0.0;
    int    frames = 0;
    double peakDb = -300.0;
};

// least squares on db against time over the frames after the peak from hiDb under it to loDb under it
inline DecayFit t60Of(const Envelope& e, double hiDb = 6.0, double loDb = 40.0, double minSpanDb = 14.0) {
    DecayFit d;
    if (e.db.empty()) return d;
    size_t pk = 0;
    for (size_t i = 1; i < e.db.size(); ++i)
        if (e.db[i] > e.db[pk]) pk = i;
    d.peakDb = e.db[pk];
    double st = 0, sy = 0, stt = 0, sty = 0, lowest = d.peakDb;
    int n = 0;
    for (size_t i = pk; i < e.db.size(); ++i) {
        const double rel = e.db[i] - d.peakDb;
        if (rel < -loDb) break;
        if (rel > -hiDb) continue;
        const double t = ((double)i + 0.5) * e.frameSec;
        st += t;
        sy += e.db[i];
        stt += t * t;
        sty += t * e.db[i];
        ++n;
        lowest = std::min(lowest, e.db[i]);
    }
    d.frames = n;
    d.spanDb = d.peakDb - hiDb - lowest;
    if (n < 4 || d.peakDb - lowest < hiDb + minSpanDb) return d;
    const double slope = (n * sty - st * sy) / (n * stt - st * st);
    if (slope < 0.0) d.t60 = -60.0 / slope;
    return d;
}

// the least-squares slope (db/s) of an envelope over [t0, t1) s, frames under floorDb ignored; 0 with under 3 frames
inline double slopeOf(const Envelope& e, double t0, double t1, double floorDb = -1e9) {
    double st = 0, sy = 0, stt = 0, sty = 0;
    int n = 0;
    for (size_t i = 0; i < e.db.size(); ++i) {
        const double t = ((double)i + 0.5) * e.frameSec;
        if (t < t0 || t >= t1 || e.db[i] < floorDb) continue;
        st += t;
        sy += e.db[i];
        stt += t * t;
        sty += t * e.db[i];
        ++n;
    }
    return n >= 3 ? (n * sty - st * sy) / (n * stt - st * st) : 0.0;
}

// the two-stage decay: the early slope over the first second after the peak, the late over the rest down to
// 60 db under the peak (db/s, negative as it falls)
struct TwoSlopes { double early = 0.0, late = 0.0; };
inline TwoSlopes twoSlopes(const Envelope& e, double splitSec = 1.0) {
    TwoSlopes s;
    if (e.db.empty()) return s;
    size_t pk = 0;
    for (size_t i = 1; i < e.db.size(); ++i)
        if (e.db[i] > e.db[pk]) pk = i;
    const double t0 = ((double)pk + 0.5) * e.frameSec, floorDb = e.db[pk] - 60.0;
    s.early = slopeOf(e, t0, t0 + splitSec, floorDb);
    s.late = slopeOf(e, t0 + splitSec, 1e9, floorDb);
    return s;
}

// the beat on an envelope: the db envelope less its moving mean over windowSec, from fromSec on; the ripple's
// depth (its largest swing, db) and its period (s) from the spacing of its troughs (one a dip under the mean,
// each by a parabola); the rate 1 / period, 0 with fewer than two troughs or a ripple under 0.1 db
struct Beat { double rate = 0.0, depthDb = 0.0; int troughs = 0; };
inline Beat beatOf(const Envelope& e, double fromSec, double windowSec) {
    Beat b;
    const int half = std::max(1, (int)std::lround(0.5 * windowSec / e.frameSec));
    const int n = (int)e.db.size();
    std::vector<double> r;
    std::vector<double> t;
    for (int i = (int)(fromSec / e.frameSec) + half; i + half < n; ++i) {
        double sum = 0.0;
        for (int j = i - half; j <= i + half; ++j) sum += e.db[(size_t)j];
        r.push_back(e.db[(size_t)i] - sum / (2 * half + 1));
        t.push_back(((double)i + 0.5) * e.frameSec);
    }
    if (r.size() < 5) return b;
    double lo = r[0], hi = r[0];
    for (double v : r) {
        lo = std::min(lo, v);
        hi = std::max(hi, v);
    }
    b.depthDb = hi - lo;
    // the troughs: one a dip under the moving mean, at its lowest frame, placed by a parabola
    std::vector<double> at;
    size_t low = 0;
    bool in = false;
    for (size_t i = 0; i < r.size(); ++i) {
        if (r[i] < 0.0) {
            if (!in || r[i] < r[low]) low = i;
            in = true;
            continue;
        }
        if (in && low > 0 && low + 1 < r.size()) {
            const double a = r[low - 1], c = r[low], d = r[low + 1], den = a - 2.0 * c + d;
            at.push_back(t[low] + (den > 0.0 ? 0.5 * (a - d) / den : 0.0) * e.frameSec);
        }
        in = false;
    }
    b.troughs = (int)at.size();
    if (at.size() >= 2 && b.depthDb >= 0.1) b.rate = (double)(at.size() - 1) / (at.back() - at.front());
    return b;
}

// ---- the tracker -----------------------------------------------------------------------------------

struct Tracked {
    double expect = 0.0;        // hz, where the table says
    double hz = 0.0;            // the measured peak within the window; 0: none
    double cents = 0.0;         // measured against expected
    double levelDb = -300.0;    // the envelope's first frame, db full scale
    DecayFit decay;
};

// each expected partial in x: the peak within windowCents of it in the record's falling-hann spectrum
// (zero-padded to at least 2^19 points), and its envelope in a band about it, as wide as the window and its
// neighbours allow
inline std::vector<Tracked> trackPartials(const float* x, size_t n, double fs, const std::vector<double>& expect, double windowCents = 50.0) {
    std::vector<Tracked> out(expect.size());
    if (n < 2) return out;
    const std::vector<double> mag = magnitude(x, n, std::max<size_t>(nextPow2(n), (size_t)1 << 19), Window::fallingHann);
    const BandAnalyser bands(x, n, fs);
    for (size_t i = 0; i < expect.size(); ++i) {
        Tracked& t = out[i];
        t.expect = expect[i];
        if (!(expect[i] > 0.0) || expect[i] >= 0.5 * fs) continue;
        const Peak p = peakNear(mag, fs, expect[i], windowCents);
        t.hz = p.hz;
        t.cents = p.hz > 0.0 ? 1200.0 * std::log2(p.hz / expect[i]) : 0.0;
        double hw = 2.0 * expect[i] * (std::exp2(windowCents / 1200.0) - 1.0);
        for (size_t j = 0; j < expect.size(); ++j)
            if (j != i && expect[j] > 0.0) hw = std::min(hw, 0.9 * std::fabs(expect[j] - expect[i]));
        hw = std::max(hw, 1.0);
        const Envelope e = bands.envelope(expect[i] - hw, expect[i] + hw);
        if (!e.db.empty()) t.levelDb = e.db[0];
        t.decay = t60Of(e);
    }
    return out;
}

// ---- harmonics -------------------------------------------------------------------------------------

// the magnitude of a record's hann-windowed dft at hz, from `from`, `count` samples
inline double dftHann(const float* x, size_t from, size_t count, double hz, double fs) {
    double re = 0.0, im = 0.0;
    const double w = kTwoPi * hz / fs;
    for (size_t k = 0; k < count; ++k) {
        const double win = 0.5 - 0.5 * std::cos(kTwoPi * ((double)k + 0.5) / (double)count);
        re += win * x[from + k] * std::cos(w * (double)k);
        im -= win * x[from + k] * std::sin(w * (double)k);
    }
    return std::sqrt(re * re + im * im);
}

// the thd of a record against f1: the rms of the harmonics 2 .. count over the first's (each under fs / 2),
// into h (count values) when given
inline double thdOf(const float* x, size_t from, size_t n, double f1, double fs, int count = 8, double* h = nullptr) {
    double first = 0.0, sum = 0.0;
    for (int i = 1; i <= count; ++i) {
        const double a = i * f1 < 0.5 * fs ? dftHann(x, from, n, i * f1, fs) : 0.0;
        if (h) h[i - 1] = a;
        if (i == 1) first = a;
        else sum += a * a;
    }
    return first > 0.0 ? std::sqrt(sum) / first : 0.0;
}

// ---- levels ----------------------------------------------------------------------------------------

inline double rms(const float* x, size_t n) {
    double s = 0.0;
    for (size_t i = 0; i < n; ++i) s += (double)x[i] * x[i];
    return n ? std::sqrt(s / (double)n) : 0.0;
}

inline double peakAbs(const float* x, size_t n) {
    double m = 0.0;
    for (size_t i = 0; i < n; ++i) m = std::max(m, std::fabs((double)x[i]));
    return m;
}

inline bool allFinite(const float* x, size_t n) {
    for (size_t i = 0; i < n; ++i)
        if (!std::isfinite(x[i])) return false;
    return true;
}

// the click measure: over 10 ms frames whose rms is over floorRms, the largest of a frame's largest first
// difference over its rms
inline double clickRatio(const float* x, size_t n, double fs, double floorRms = 1e-6) {
    const size_t w = (size_t)std::max(8.0, 0.01 * fs);
    double worst = 0.0;
    for (size_t a = 0; a + w <= n; a += w) {
        const double r = rms(x + a, w);
        if (!(r > floorRms)) continue;
        double d = 0.0;
        for (size_t i = a + (a > 0 ? 0 : 1); i < a + w; ++i) d = std::max(d, std::fabs((double)x[i] - (double)x[i - 1]));
        worst = std::max(worst, d / r);
    }
    return worst;
}

// the click bound for a sound whose top partial is hz: 1.5 x 2 sqrt(2) sin(pi hz / fs)
inline double clickBound(double hz, double fs) { return 1.5 * 2.0 * std::sqrt(2.0) * std::sin(0.5 * kTwoPi * std::min(hz, 0.5 * fs) / fs); }

inline double dbfs(double x) { return 20.0 * std::log10(x + 1e-30); }

}  // namespace up
