// DeathAxe - noise gate, boost pedal, 4-stage high-gain preamp, tone stack, power amp, cabinet
#pragma once
#include "dsp_basic.h"

namespace dg {

struct AmpParams {
    float gateThresh = -55, gateRel = 60;
    bool  boostOn = true;  float boostDrive = 0.35f, boostTight = 180, boostLevel = 0;
    int   voicing = 0;     float gain = 0.7f, bright = 0.3f, bias = 0.5f, sag = 0.3f;
    float bass = 0, mid = 0, midFreq = 650, treble = 0, presence = 0, resonance = 0;
    int   cab = 0;         float micPos = 0.35f, micDist = 0.25f, lowCut = 45, highCut = 12000;
};

// ---------------------------------------------------------------- voicings
struct Voicing {
    const char* name;
    int   nStages;
    float baseDb[4], rangeDb[4];     // stage gain = base + range * Gain
    float hp[4], lp[4];              // coupling high-pass / inter-stage low-pass (Hz)
    int   shape[4];                  // 0 tanh, 1 algebraic, 2 hard
    float asym[4];                   // bias offset scale
    int   toneAfter;                 // tone stack sits after this stage (1..n)
    float bassHz, trebHz, midQ, presHz, brightHz, brightDb;
    float trimDb, sagScale, powerDrive;
};

inline const Voicing kVoicings[5] = {
    {"Modern Scoop", 4, { 8, 8, 6, 4}, {14, 12, 10, 6}, {110, 160, 120,  90}, {9000, 8000, 7000, 6000},
        {0, 0, 1, 1}, {0.35f, 0.30f, 0.20f, 0.10f}, 3, 90, 3200, 0.7f, 3800, 2400, 6, -6, 1.0f, 1.5f},
    {"Brutal Mid",   4, {10, 8, 6, 2}, {14, 12,  8,  0}, {140, 200, 160, 100}, {8000, 6500, 5500, 5000},
        {0, 1, 0, 1}, {0.30f, 0.35f, 0.25f, 0.10f}, 2, 100, 3000, 0.9f, 3500, 2200, 5, -4, 1.2f, 1.5f},
    {"Old School Tampa", 3, {10, 6, 2, 0}, {18, 12, 0, 0}, {80, 100, 70, 0}, {7500, 6500, 5500, 0},
        {1, 1, 0, 0}, {0.50f, 0.40f, 0.20f, 0.0f}, 2, 120, 2800, 0.8f, 3200, 2000, 8, -4, 0.8f, 1.2f},
    {"Swedish Chainsaw", 3, {12, 6, 0, 0}, {18, 10, 0, 0}, {70, 60, 50, 0}, {10000, 9000, 7000, 0},
        {2, 2, 0, 0}, {0.10f, 0.05f, 0.0f, 0.0f}, 2, 80, 3400, 1.2f, 4000, 3000, 3, -6.5f, 0.5f, 1.3f},
    {"Tech Tight",   4, { 8, 8, 8, 2}, {12, 12, 12,  0}, {180, 220, 160, 120}, {9500, 9000, 8000, 7000},
        {0, 0, 0, 1}, {0.15f, 0.10f, 0.10f, 0.05f}, 3, 110, 3600, 0.8f, 4200, 2600, 5, -6, 0.6f, 1.6f},
};

// cheap tanh (Pade-type), exact +-1 saturation for |x| >= 3
inline float fastTanh(float x) {
    if (x > 3.0f) return 1.0f;
    if (x < -3.0f) return -1.0f;
    const float x2 = x * x;
    return x * (27.0f + x2) / (27.0f + 9.0f * x2);
}

inline float shapeFn(int k, float x) {
    switch (k) {
        case 0:  return fastTanh(x);
        case 1:  return x / std::sqrt(1.0f + x * x);
        default: { const float x2 = x * x; return x / std::sqrt(std::sqrt(1.0f + x2 * x2)); }
    }
}

// ---------------------------------------------------------------- noise gate (linked stereo)
struct Gate {
    float env = 0, gain = 1, hold = 0;  bool open = true;
    float aEnv = 0, rEnv = 0, aGain = 0, rGain = 0;  int holdSamples = 0;
    float thr = 0.001f;
    void configure(double fs, float threshDb, float relMs) {
        thr = dbToLin(threshDb);
        aEnv = 1.0f - std::exp(-1.0f / (0.0002f * (float)fs));
        rEnv = 1.0f - std::exp(-1.0f / (0.02f * (float)fs));
        aGain = 1.0f - std::exp(-1.0f / (0.0007f * (float)fs));
        rGain = 1.0f - std::exp(-1.0f / (relMs * 0.001f * (float)fs));
        holdSamples = (int)(0.03 * fs);
    }
    inline float tick(float l, float r) {
        const float a = std::max(std::fabs(l), std::fabs(r));
        env += (a > env ? aEnv : rEnv) * (a - env);
        if (env > thr) { open = true; hold = (float)holdSamples; }
        else if (env < thr * 0.6f) { if (hold > 0) hold -= 1; else open = false; }
        const float t = open ? 1.0f : 0.0f;
        gain += (t > gain ? aGain : rGain) * (t - gain);
        return gain;
    }
};

// ---------------------------------------------------------------- one amp channel (runs at 4x)
struct AmpChannel {
    Oversampler4 os;
    Biquad boostHp, boostLp, boostHump;
    Biquad bright, tBass, tMid, tTreb;
    Biquad hp[4], lp[4];
    Biquad dc;
    float  g[4] = {1, 1, 1, 1}, bias[4] = {0, 0, 0, 0};
    float  boostGain = 1, boostOut = 1, sagEnv = 0, sagAtk = 0.01f, sagRel = 0.0001f;
    float  sagAmt = 0, powerDrive = 1.5f, trim = 0.5f;
    bool   boostOn = true;
    int    nStages = 4, toneAfter = 3;

    void init() { os.init(); }

    void reset() {
        os.reset();
        for (Biquad* b : {&boostHp, &boostLp, &boostHump, &bright, &tBass, &tMid, &tTreb, &dc}) b->reset();
        for (int i = 0; i < 4; ++i) { hp[i].reset(); lp[i].reset(); }
        sagEnv = 0;
    }

    void configure(double fs, const AmpParams& p) {
        const double F = fs * 4.0;
        const Voicing& v = kVoicings[p.voicing];
        boostOn = p.boostOn;
        boostHp.design(BQ::HP, F, p.boostTight, 0.707);
        boostLp.design(BQ::LP, F, 5200, 0.707);
        boostHump.design(BQ::Peak, F, 720, 0.9, 4.0);
        boostGain = 1.0f + 39.0f * p.boostDrive * p.boostDrive;
        boostOut = dbToLin(p.boostLevel);
        bright.design(BQ::HighShelf, F, v.brightHz, 0.7, v.brightDb * p.bright);
        nStages = v.nStages;  toneAfter = v.toneAfter;
        for (int i = 0; i < nStages; ++i) {
            hp[i].design(BQ::HP, F, v.hp[i], 0.707);
            lp[i].design(BQ::LP, F, v.lp[i], 0.707);
            bias[i] = v.asym[i] * (0.2f + 1.6f * p.bias) * 1.5f;
        }
        tBass.design(BQ::LowShelf, F, v.bassHz, 0.7, p.bass);
        tMid.design(BQ::Peak, F, p.midFreq, v.midQ, p.mid);
        tTreb.design(BQ::HighShelf, F, v.trebHz, 0.7, p.treble);
        dc.design(BQ::HP, F, 18.0, 0.707);
        sagAmt = p.sag * v.sagScale;
        sagAtk = 1.0f - std::exp(-1.0f / (0.004f * (float)F));
        sagRel = 1.0f - std::exp(-1.0f / (0.12f * (float)F));
        powerDrive = v.powerDrive;
        trim = dbToLin(v.trimDb);
    }

    inline void setGains(const Voicing& v, float gain) {
        for (int i = 0; i < nStages; ++i) g[i] = dbToLin(v.baseDb[i] + v.rangeDb[i] * gain);
    }

    inline float hi(float x, const Voicing& v) {
        if (boostOn) {
            x = boostHp.process(x);
            x = fastTanh(x * boostGain * 1.5f) * 0.6f;
            x = boostLp.process(x);
            x = boostHump.process(x) * boostOut;
        }
        x = bright.process(x);
        for (int i = 0; i < nStages; ++i) {
            x = hp[i].process(x);
            x *= g[i];
            const float b = bias[i];
            x = shapeFn(v.shape[i], x + b) - shapeFn(v.shape[i], b);
            x = lp[i].process(x);
            if (i + 1 == toneAfter) x = tTreb.process(tMid.process(tBass.process(x)));
        }
        // power amp: sag + saturation
        const float a = std::fabs(x);
        sagEnv += (a > sagEnv ? sagAtk : sagRel) * (a - sagEnv);
        x *= 1.0f / (1.0f + sagAmt * 1.5f * sagEnv);
        x = fastTanh(x * powerDrive) / fastTanh(powerDrive);
        x = dc.process(x);
        return x * trim;
    }

    inline float process(float x, const Voicing& v) {
        return os.run(x, [this, &v](float s) { return hi(s, v); });
    }
};

// ---------------------------------------------------------------- cabinet + mic
struct CabDef {
    const char* name;
    float hp, resHz, resDb, resQ, dipHz, dipDb, dipQ, p1Hz, p1Db, p1Q, p2Hz, p2Db, p2Q, lpCenter, lpEdge;
};

inline const CabDef kCabs[6] = {
    {"4x12 V30",        70, 110, 4, 1.6f, 380, -4, 1.2f, 2000, 4, 1.2f, 3500, 5, 2.0f, 6200, 4300},
    {"4x12 Greenback",  75, 100, 3, 1.4f, 600, -2, 0.8f, 1800, 4, 1.0f, 2800, 2, 2.0f, 5200, 3800},
    {"2x12 Tight",      95, 130, 2, 1.2f, 350, -3, 1.0f, 2200, 5, 1.5f, 3800, 4, 2.0f, 6500, 4800},
    {"1x12 Open",       90, 160, 2, 1.0f, 400, -1, 1.0f, 1200, 4, 0.9f, 3000, 2, 2.0f, 5000, 3600},
    {"8x10 Beast",      55,  80, 6, 1.3f, 450, -5, 1.0f, 2500, 3, 1.2f, 4500, 3, 2.0f, 6000, 4400},
    {"Direct",          30, 100, 0, 1.0f, 400,  0, 1.0f, 2000, 0, 1.0f, 3500, 0, 2.0f, 9000, 9000},
};

struct CabChannel {
    Biquad hp, res, dip, p1, p2, prox, post_hp, post_lp, presence, reson;
    Butter4 lp;
    std::vector<float> er;  int erMask = 0, erW = 0;
    int   tap[4] = {1, 1, 1, 1};  float tapG[4] = {0, 0, 0, 0};

    void init(double fs, bool right) {
        int n = 1;  while (n < (int)(fs * 0.025)) n <<= 1;
        er.assign(n, 0.f);  erMask = n - 1;  erW = 0;
        const float ms[2][4] = {{2.3f, 4.1f, 7.9f, 11.3f}, {2.9f, 4.9f, 6.7f, 12.1f}};
        for (int i = 0; i < 4; ++i) tap[i] = std::max(1, (int)(ms[right ? 1 : 0][i] * 0.001 * fs));
    }

    void reset() {
        for (Biquad* b : {&hp, &res, &dip, &p1, &p2, &prox, &post_hp, &post_lp, &presence, &reson}) b->reset();
        lp.reset();  std::fill(er.begin(), er.end(), 0.f);
    }

    void configure(double fs, const AmpParams& p) {
        const CabDef& c = kCabs[p.cab];
        const float m = p.micPos;
        hp.design(BQ::HP, fs, c.hp, 0.8);
        res.design(BQ::Peak, fs, c.resHz, c.resQ, c.resDb);
        dip.design(BQ::Peak, fs, c.dipHz, c.dipQ, c.dipDb * (0.7f + 0.6f * m));
        const float pg = 1.0f - 0.55f * m;
        p1.design(BQ::Peak, fs, c.p1Hz * (1.0f - 0.15f * m), c.p1Q, c.p1Db * pg);
        p2.design(BQ::Peak, fs, c.p2Hz * (1.0f - 0.2f * m), c.p2Q, c.p2Db * pg);
        lp.design(false, fs, c.lpCenter + (c.lpEdge - c.lpCenter) * m);
        prox.design(BQ::LowShelf, fs, 180, 0.7, 5.0 * (1.0 - p.micDist));
        for (int i = 0; i < 4; ++i) tapG[i] = p.micDist * p.micDist * 0.5f / (1.0f + i);
        post_hp.design(BQ::HP, fs, p.lowCut, 0.707);
        post_lp.design(BQ::LP, fs, p.highCut, 0.707);
        const Voicing& v = kVoicings[p.voicing];
        presence.design(BQ::HighShelf, fs, v.presHz, 0.7, p.presence);
        reson.design(BQ::LowShelf, fs, 95, 0.7, p.resonance);
    }

    inline float process(float x) {
        x = presence.process(reson.process(x));
        float y = hp.process(x);
        y = res.process(y);  y = dip.process(y);  y = p1.process(y);  y = p2.process(y);
        y = lp.process(y);   y = prox.process(y);
        er[erW & erMask] = y;
        float o = y;
        for (int i = 0; i < 4; ++i) o += tapG[i] * er[(erW - tap[i]) & erMask];
        ++erW;
        return post_lp.process(post_hp.process(o));
    }
};

} // namespace dg
