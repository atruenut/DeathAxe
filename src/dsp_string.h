// DeathAxe - physically modelled electric guitar string (extended Karplus-Strong)
//  * triangle "pluck displacement" + filtered noise excitation, pickup comb filter
//  * palm mute (shorter + darker), pinch harmonics (loop filled with a single harmonic),
//    legato glide, fast release (finger lift), re-picking for tremolo picking.
#pragma once
#include "dsp_basic.h"

namespace dg {

// live parameters shared by every string (owned by the engine)
struct StringParams {
    float attack = 0.7f, pickPos = 0.3f, noise = 0.25f;
    float sustain = 4.0f, damping = 0.35f, palm = 0.0f;
    float pinch = 0.0f;
    float bend = 1.0f;          // pitch bend ratio
};

struct StringVoice {
    std::vector<float> buf;  int mask = 0, w = 0;
    double fs = 44100;
    const StringParams* P = nullptr;

    bool active = false, released = false, sounding = false;
    float freq = 110, freqTarget = 110, slew = 1.0f, detune = 1.0f;
    float Dtotal = 100, eta = 0, b0 = 0.5f, g = 0.999f, gRel = 0.5f;
    int   dInt = 90, pickM = 10;
    float lpZ = 0, apX1 = 0, apY1 = 0;
    float relEnv = 1, relCoef = 0.999f, envF = 0, fadeIn = 1, fadeStep = 0.01f;
    float clickEnv = 0, clickAmt = 0, clickZ = 0;
    int   pending = -1, ctl = 0, age = 0;
    float pendVel = 1;  bool pendPinch = false;
    Rng   rng;

    void init(double sampleRate, uint32_t seed) {
        fs = sampleRate;
        int n = 1;  const int need = (int)(fs / 15.0) + 16;
        while (n < need) n <<= 1;
        buf.assign(n, 0.f);  mask = n - 1;  rng.s = seed ? seed : 1u;
        scratch.reserve(n);
        fadeStep = (float)(1.0 / (0.0015 * fs));
        active = sounding = false;
    }

    // allocate this string to a note
    void start(float f, float detuneRatio, float glideSamples) {
        freq = freqTarget = f;  detune = detuneRatio;  slew = 1.0f;
        std::fill(buf.begin(), buf.end(), 0.f);
        w = 0; lpZ = apX1 = apY1 = 0; relEnv = 1; envF = 1; released = false;
        active = true; sounding = false; pending = -1; age = 0; fadeIn = 0; clickEnv = 0;
        (void)glideSamples;
        updateLoop();
    }

    void glideTo(float f, float glideSamples) {
        freqTarget = f;
        slew = glideSamples < 2.f ? 1.0f : 1.0f - std::exp(-32.0f / glideSamples);   // applied every 32 samples
    }

    // schedule a pick (delay in samples for double-tracking offsets / strumming)
    void trigger(float vel, int delaySamples, bool pinchHit) {
        pending = delaySamples < 0 ? 0 : delaySamples;  pendVel = vel;  pendPinch = pinchHit;
        released = false;  relEnv = 1;
    }

    void release() { released = true; updateLoop(); }

    void updateLoop() {
        const StringParams& p = *P;
        const float f = std::max(15.0f, freq * detune * p.bend);
        Dtotal = (float)(fs / f);
        const float palm = p.palm;
        b0 = 0.95f - 0.72f * p.damping;
        b0 *= (1.0f - 0.55f * palm);
        b0 = std::max(b0, 1.0f / (Dtotal - 1.0f));   // keep loop delay positive
        b0 = std::min(b0, 1.0f);
        const float lpDelay = (1.0f - b0) / b0;
        float D = Dtotal - lpDelay;
        if (D < 2.0f) D = 2.0f;
        dInt = (int)std::floor(D - 0.5f);
        const float frac = D - dInt;                       // 0.5 .. 1.5
        eta = (1.0f - frac) / (1.0f + frac);
        const float t60 = std::max(0.05f, p.sustain * (1.0f - 0.93f * palm));
        g = std::pow(10.0f, -3.0f * Dtotal / ((float)fs * t60));
        const float rel = 0.05f * (1.0f - 0.7f * palm) + 0.012f;
        gRel = std::pow(10.0f, -3.0f * Dtotal / ((float)fs * rel));
        relCoef = std::exp(-1.0f / (0.04f * (float)fs));
        const float pf = 0.06f + 0.26f * p.pickPos;         // bridge .. neck pickup
        pickM = std::max(1, std::min(mask - 1, (int)(pf * Dtotal)));
    }

    void doPluck() {
        const StringParams& p = *P;
        const int N = std::max(4, (int)Dtotal);
        const float vn = pendVel;                                  // 0..1
        const float attack = p.attack * (0.55f + 0.45f * vn);
        const float vg = 0.12f + 0.88f * std::pow(vn, 1.35f);
        std::vector<float>& e = scratch;  e.assign(N, 0.f);
        const float pp = 0.11f + 0.08f * rng.uni01();
        const int pk = std::max(1, (int)(pp * N));
        float z = 0;  const float na = 0.12f + 0.85f * attack;
        const float ng = 0.10f + 1.20f * attack;
        for (int k = 0; k < N; ++k) {
            const float tri = k < pk ? (float)k / pk : (float)(N - k) / (N - pk);
            z += na * (rng.uni() - z);
            e[k] = tri * (1.0f - 0.4f * attack) + z * ng;
        }
        if (pendPinch && p.pinch > 0.001f) {
            static const int hs[4] = {3, 4, 5, 6};
            const int h = hs[rng.next() & 3];
            for (int k = 0; k < N; ++k) {
                const float hv = std::sin(2.0f * (float)kPi * h * k / N);
                e[k] = (1.0f - p.pinch) * e[k] * 0.35f + p.pinch * (hv * 1.3f + 0.15f * (rng.uni()));
            }
        }
        double mean = 0;  for (float v : e) mean += v;  mean /= N;
        float pk2 = 1e-6f;  for (float& v : e) { v -= (float)mean; pk2 = std::max(pk2, std::fabs(v)); }
        const float sc = 0.5f * vg / pk2;
        const bool fresh = !sounding;
        for (int k = 0; k < N; ++k) {
            float& dst = buf[(w - 1 - k) & mask];
            dst = fresh ? e[k] * sc : dst * 0.35f + e[k] * sc;
        }
        sounding = true;  fadeIn = std::min(fadeIn, 0.5f);
        clickEnv = 1.0f;  clickAmt = p.noise * (0.05f + 0.25f * vg);
        updateLoop();
    }

    inline float process() {
        if (!active) return 0.f;
        if (pending >= 0 && pending-- == 0) doPluck();
        if (!sounding) return 0.f;
        if ((++ctl & 31) == 0) {
            if (freq != freqTarget) {
                freq += (freqTarget - freq) * slew;
                if (std::fabs(freq - freqTarget) < 0.01f * freqTarget * 0.01f) freq = freqTarget;
            }
            updateLoop();
        }
        const float x = buf[(w - dInt) & mask];
        const float y = eta * x + apX1 - eta * apY1;
        apX1 = x; apY1 = y;
        lpZ += b0 * (y - lpZ);
        const float nv = lpZ * (released ? gRel : g);
        buf[w] = nv;
        float out = nv - buf[(w - pickM) & mask];
        w = (w + 1) & mask;

        // pick click / scrape, outside the loop
        if (clickEnv > 1e-3f) {
            const float n = rng.uni();  clickZ += 0.6f * (n - clickZ);
            out += (n - clickZ) * clickEnv * clickAmt;
            clickEnv *= 0.9965f;
        }
        if (released) { relEnv *= relCoef; out *= relEnv; }
        if (fadeIn < 1.0f) { fadeIn = std::min(1.0f, fadeIn + fadeStep); out *= fadeIn; }
        const float a = std::fabs(out);
        envF = a > envF ? a : envF * 0.9995f;
        if (pending < 0 && envF < 2e-5f && ++age > 64) { active = sounding = false; }
        return out;
    }

    std::vector<float> scratch;
};

} // namespace dg
