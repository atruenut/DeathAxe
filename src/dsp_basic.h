// DeathAxe - basic DSP building blocks (header only, no dependencies)
#pragma once
#include <cmath>
#include <cstdint>
#include <vector>
#include <algorithm>

namespace dg {

constexpr double kPi = 3.14159265358979323846;

inline float dbToLin(float db) { return std::pow(10.0f, db * 0.05f); }

struct Rng {
    uint32_t s = 0x1234567u;
    inline uint32_t next() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return s; }
    inline float uni()  { return (next() >> 8) * (1.0f / 8388608.0f) - 1.0f; }   // -1..1
    inline float uni01() { return (next() >> 8) * (1.0f / 16777216.0f); }        // 0..1
};

enum class BQ { LP, HP, BP, Peak, LowShelf, HighShelf, Notch };

// RBJ biquad, transposed direct form II, double precision state
struct Biquad {
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;

    void reset() { z1 = z2 = 0; }

    void design(BQ t, double fs, double f0, double Q, double gainDb = 0) {
        f0 = std::min(f0, fs * 0.45);
        f0 = std::max(f0, 1.0);
        const double w0 = 2 * kPi * f0 / fs, cw = std::cos(w0), sw = std::sin(w0);
        const double alpha = sw / (2 * Q);
        const double A = std::pow(10.0, gainDb / 40.0);
        double B0, B1, B2, A0, A1, A2;
        switch (t) {
            case BQ::LP:
                B0 = (1 - cw) / 2; B1 = 1 - cw; B2 = B0; A0 = 1 + alpha; A1 = -2 * cw; A2 = 1 - alpha; break;
            case BQ::HP:
                B0 = (1 + cw) / 2; B1 = -(1 + cw); B2 = B0; A0 = 1 + alpha; A1 = -2 * cw; A2 = 1 - alpha; break;
            case BQ::BP:
                B0 = alpha; B1 = 0; B2 = -alpha; A0 = 1 + alpha; A1 = -2 * cw; A2 = 1 - alpha; break;
            case BQ::Notch:
                B0 = 1; B1 = -2 * cw; B2 = 1; A0 = 1 + alpha; A1 = -2 * cw; A2 = 1 - alpha; break;
            case BQ::Peak:
                B0 = 1 + alpha * A; B1 = -2 * cw; B2 = 1 - alpha * A;
                A0 = 1 + alpha / A; A1 = -2 * cw; A2 = 1 - alpha / A; break;
            case BQ::LowShelf: {
                const double s = 2 * std::sqrt(A) * alpha;
                B0 = A * ((A + 1) - (A - 1) * cw + s); B1 = 2 * A * ((A - 1) - (A + 1) * cw);
                B2 = A * ((A + 1) - (A - 1) * cw - s);
                A0 = (A + 1) + (A - 1) * cw + s; A1 = -2 * ((A - 1) + (A + 1) * cw);
                A2 = (A + 1) + (A - 1) * cw - s; break; }
            default: { // HighShelf
                const double s = 2 * std::sqrt(A) * alpha;
                B0 = A * ((A + 1) + (A - 1) * cw + s); B1 = -2 * A * ((A - 1) + (A + 1) * cw);
                B2 = A * ((A + 1) + (A - 1) * cw - s);
                A0 = (A + 1) - (A - 1) * cw + s; A1 = 2 * ((A - 1) - (A + 1) * cw);
                A2 = (A + 1) - (A - 1) * cw - s; break; }
        }
        b0 = B0 / A0; b1 = B1 / A0; b2 = B2 / A0; a1 = A1 / A0; a2 = A2 / A0;
    }

    inline float process(float x) {
        const double y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return (float)y;
    }
};

// 4th order Butterworth low/high pass (two cascaded biquads)
struct Butter4 {
    Biquad s[2];
    void design(bool highpass, double fs, double f0) {
        s[0].design(highpass ? BQ::HP : BQ::LP, fs, f0, 0.5412);
        s[1].design(highpass ? BQ::HP : BQ::LP, fs, f0, 1.3066);
    }
    void reset() { s[0].reset(); s[1].reset(); }
    inline float process(float x) { return s[1].process(s[0].process(x)); }
};

struct OnePole {
    float a = 0, z = 0;
    void setCutoff(double fs, double fc) { a = 1.0f - std::exp(-2.0 * kPi * fc / fs); }
    inline float lp(float x) { z += a * (x - z); return z; }
    inline float hp(float x) { z += a * (x - z); return x - z; }
};

// exponential parameter smoother
struct Smooth {
    float v = 0, target = 0, coef = 0.001f;
    void setTime(double fs, double seconds) { coef = 1.0f - std::exp(-1.0 / (fs * seconds)); }
    void set(float t, bool immediate = false) { target = t; if (immediate) v = t; }
    inline float tick() { v += coef * (target - v); return v; }
};

// ---------------- 2x polyphase-style halfband-ish FIR resamplers ----------------
inline std::vector<float> designHalfband(int L) {   // L odd; lowpass at 1/4 of the high rate, DC gain = 1
    std::vector<float> h(L);
    const int c = (L - 1) / 2;
    double sum = 0;
    for (int n = 0; n < L; ++n) {
        const double m = n - c;
        const double sinc = (m == 0) ? 0.5 : std::sin(kPi * 0.5 * m) / (kPi * m);
        const double x = (double)n / (L - 1);
        const double w = 0.42 - 0.5 * std::cos(2 * kPi * x) + 0.08 * std::cos(4 * kPi * x);  // Blackman
        h[n] = (float)(sinc * w);
        sum += h[n];
    }
    for (auto& v : h) v = (float)(v / sum);
    return h;
}

struct Up2 {
    std::vector<float> h, hist;  int pos = 0, K = 0;
    void init(int L) { h = designHalfband(L); K = (L + 1) / 2; hist.assign(K * 2, 0.f); pos = 0; }
    // pushes one sample, returns two
    inline void process(float x, float& y0, float& y1) {
        if (--pos < 0) pos = K - 1;
        hist[pos] = hist[pos + K] = x;
        const float* p = &hist[pos];
        float a = 0, b = 0;
        const int L = (int)h.size();
        for (int k = 0; 2 * k < L; ++k) a += h[2 * k] * p[k];
        for (int k = 0; 2 * k + 1 < L; ++k) b += h[2 * k + 1] * p[k];
        y0 = 2 * a; y1 = 2 * b;
    }
    void reset() { std::fill(hist.begin(), hist.end(), 0.f); }
};

struct Down2 {
    std::vector<float> h, hist;  int pos = 0, L = 0;
    void init(int len) { h = designHalfband(len); L = len; hist.assign(L * 2, 0.f); pos = 0; }
    inline void push(float x) { if (--pos < 0) pos = L - 1; hist[pos] = hist[pos + L] = x; }
    inline float out() const { const float* p = &hist[pos]; float a = 0; for (int k = 0; k < L; ++k) a += h[k] * p[k]; return a; }
    void reset() { std::fill(hist.begin(), hist.end(), 0.f); }
};

// 4x oversampler: run a per-sample nonlinear function at 4x the host rate
struct Oversampler4 {
    Up2 u1, u2;  Down2 d1, d2;
    void init() { u1.init(63); u2.init(31); d2.init(31); d1.init(63); }
    void reset() { u1.reset(); u2.reset(); d1.reset(); d2.reset(); }
    template <class F> inline float run(float x, F&& fn) {
        float a0, a1;  u1.process(x, a0, a1);
        float r[2];
        for (int i = 0; i < 2; ++i) {
            float b0, b1;  u2.process(i ? a1 : a0, b0, b1);
            d2.push(fn(b0)); d2.push(fn(b1));
            r[i] = d2.out();
        }
        d1.push(r[0]); d1.push(r[1]);
        return d1.out();
    }
};

} // namespace dg
