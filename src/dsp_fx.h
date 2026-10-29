// DeathAxe - stereo delay and Freeverb-style reverb
#pragma once
#include "dsp_basic.h"

namespace dg {

struct StereoDelay {
    std::vector<float> bl, br;  int mask = 0, w = 0;
    float lpL = 0, lpR = 0, lpA = 0.4f;
    Smooth time;  double fs = 44100;

    void init(double sampleRate) {
        fs = sampleRate;
        int n = 1;  while (n < (int)(fs * 1.05)) n <<= 1;
        bl.assign(n, 0.f);  br.assign(n, 0.f);  mask = n - 1;  w = 0;
        time.setTime(fs, 0.08);  time.set(0.3f * (float)fs, true);
        lpA = 1.0f - std::exp(-2.0 * kPi * 4500.0 / fs);
    }
    void reset() { std::fill(bl.begin(), bl.end(), 0.f);  std::fill(br.begin(), br.end(), 0.f);  lpL = lpR = 0; }
    void setTimeMs(float ms) { time.set(ms * 0.001f * (float)fs); }

    inline void process(float& l, float& r, float mix, float fb) {
        const float d = time.tick();
        const float rp = (float)w - d;
        const int i0 = (int)std::floor(rp);  const float fr = rp - i0;
        auto rd = [&](const std::vector<float>& b) {
            return b[i0 & mask] * (1 - fr) + b[(i0 + 1) & mask] * fr;
        };
        const float dl = rd(bl), dr = rd(br);
        lpL += lpA * (dl - lpL);  lpR += lpA * (dr - lpR);
        // ping-pong feedback
        bl[w] = l + lpR * fb;
        br[w] = r + lpL * fb;
        w = (w + 1) & mask;
        l += dl * mix;  r += dr * mix;
    }
};

struct Reverb {
    struct Comb {
        std::vector<float> b;  int i = 0;  float store = 0;
        void init(int n) { b.assign(n, 0.f); i = 0; store = 0; }
        inline float process(float in, float fb, float damp) {
            const float o = b[i];
            store = o * (1 - damp) + store * damp;
            b[i] = in + store * fb;
            if (++i >= (int)b.size()) i = 0;
            return o;
        }
    };
    struct AP {
        std::vector<float> b;  int i = 0;
        void init(int n) { b.assign(n, 0.f); i = 0; }
        inline float process(float in) {
            const float bo = b[i];
            const float o = -in + bo;
            b[i] = in + bo * 0.5f;
            if (++i >= (int)b.size()) i = 0;
            return o;
        }
    };
    Comb cL[8], cR[8];  AP aL[4], aR[4];
    float fb = 0.8f, damp = 0.4f;

    void init(double fs) {
        static const int ct[8] = {1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617};
        static const int at[4] = {556, 441, 341, 225};
        const double s = fs / 44100.0;
        for (int i = 0; i < 8; ++i) { cL[i].init((int)(ct[i] * s)); cR[i].init((int)((ct[i] + 23) * s)); }
        for (int i = 0; i < 4; ++i) { aL[i].init((int)(at[i] * s)); aR[i].init((int)((at[i] + 23) * s)); }
    }
    void reset() {
        for (auto& c : cL) std::fill(c.b.begin(), c.b.end(), 0.f);
        for (auto& c : cR) std::fill(c.b.begin(), c.b.end(), 0.f);
        for (auto& a : aL) std::fill(a.b.begin(), a.b.end(), 0.f);
        for (auto& a : aR) std::fill(a.b.begin(), a.b.end(), 0.f);
    }
    void set(float size, float dmp) { fb = 0.70f + 0.28f * size; damp = 0.15f + 0.7f * dmp; }

    inline void process(float& l, float& r, float mix) {
        const float in = (l + r) * 0.015f;
        float wl = 0, wr = 0;
        for (int i = 0; i < 8; ++i) { wl += cL[i].process(in, fb, damp); wr += cR[i].process(in, fb, damp); }
        for (int i = 0; i < 4; ++i) { wl = aL[i].process(wl); wr = aR[i].process(wr); }
        l += wl * 3.0f * mix;  r += wr * 3.0f * mix;
    }
};

} // namespace dg
