// Randomised stress test: extreme parameter values + random MIDI; checks for NaN/Inf/overs and crashes.
#include "../src/engine.h"
#include "../src/presets.h"
#include <cstdio>
using namespace dg;
int main() {
    Rng r; r.s = 12345;
    int bad = 0; double worst = 0;
    for (int trial = 0; trial < 40; ++trial) {
        const int sr = (trial % 3 == 0) ? 48000 : (trial % 3 == 1) ? 96000 : 44100;
        Engine e; e.init(sr);
        float v[kNumParams];
        presetValues(trial % numPresets(), v);
        for (int i = 0; i < kNumParams; ++i) {
            const ParamDef& d = kParams[i];
            const float u = r.uni01();
            if (u < 0.25f) v[i] = d.minV; else if (u < 0.5f) v[i] = d.maxV; else if (u < 0.75f) v[i] = toPlain(i, r.uni01());
        }
        e.loadPreset(v);
        std::vector<float> L(1024), R(1024);
        for (int blk = 0; blk < (sr * 8) / 1024; ++blk) {
            if (r.uni01() < 0.15f) e.noteOn(24 + (int)(r.uni01() * 70), 0.05f + 0.95f * r.uni01());
            if (r.uni01() < 0.15f) e.noteOff(24 + (int)(r.uni01() * 70));
            if (r.uni01() < 0.05f) e.setParam((int)(r.uni01() * kNumParams), 0.f + 0 * 0);   // abrupt param jumps
            if (r.uni01() < 0.03f) e.setPitchBend(r.uni01());
            if (r.uni01() < 0.03f) e.setModWheel(r.uni01());
            const int n = 1 + (int)(r.uni01() * 1023);
            e.process(L.data(), R.data(), n);
            for (int i = 0; i < n; ++i) {
                if (!std::isfinite(L[i]) || !std::isfinite(R[i])) ++bad;
                worst = std::max(worst, (double)std::max(std::fabs(L[i]), std::fabs(R[i])));
            }
        }
    }
    std::printf("stress: nonfinite=%d  worst peak=%.3f\n", bad, worst);
    return bad ? 1 : 0;
}
