// DeathAxe - complete instrument engine (strings -> gate -> boost -> preamp -> power amp -> cab -> fx)
// Pure C++17, no platform or plugin-SDK dependencies. Used by the VST3 wrapper and the offline renderer.
#pragma once
#include "params.h"
#include "dsp_basic.h"
#include "dsp_string.h"
#include "dsp_amp.h"
#include "dsp_fx.h"

namespace dg {

class Engine {
public:
    static constexpr int kMaxVoices = 32;
    static constexpr int kChunk = 256;

    void init(double sampleRate) {
        fs = sampleRate;
        for (int i = 0; i < kMaxVoices; ++i)
            for (int k = 0; k < 2; ++k) {
                voices[i].s[k].P = &sp;
                voices[i].s[k].init(fs, 0x9E3779B9u * (uint32_t)(i * 2 + k + 1));
            }
        for (int c = 0; c < 2; ++c) { amp[c].init(); cab[c].init(fs, c == 1); }
        delay.init(fs);  reverb.init(fs);
        gainS.setTime(fs, 0.02);  outS.setTime(fs, 0.02);
        for (int i = 0; i < kNumParams; ++i) pv[i] = kParams[i].defV;
        rng.s = 0xC0FFEEu;
        dirty = true;  applyParams();
        gainS.set(pv[pGain], true);  outS.set(dbToLin(pv[pOutput]), true);
        reset();
    }

    void reset() {
        for (auto& v : voices) { v.active = false; for (auto& s : v.s) { s.active = s.sounding = false; s.pending = -1; } }
        for (int c = 0; c < 2; ++c) { amp[c].reset(); cab[c].reset(); }
        delay.reset();  reverb.reset();  heldCount = 0;
        gate.env = 0; gate.gain = 1; gate.open = true;
    }

    // plain (denormalised) parameter value
    void setParam(int id, float v) { if (id >= 0 && id < kNumParams) { pv[id] = v; dirty = true; } }
    float getParam(int id) const { return pv[id]; }
    void loadPreset(const float* values) { for (int i = 0; i < kNumParams; ++i) pv[i] = values[i]; dirty = true; }
    void setPitchBend(float n01) { bend01 = n01; dirty = true; }
    void setModWheel(float n01) { mod01 = n01; dirty = true; }

    // ------------------------------------------------------------ MIDI
    void noteOn(int note, float vel) {
        applyParams();
        const int root = note + (int)pv[pTranspose];
        const bool mono = (int)pv[pVoiceMode] == 1;
        if (mono) {
            if (heldCount < 16) held[heldCount++] = note;
            if (monoActive) { monoLegato(root); return; }
            monoActive = true;
        }
        int iv[4], n = chordIntervals(iv);
        float lv[4];  for (int i = 0; i < n; ++i) lv[i] = iv[i] == -12 && (int)pv[pChord] != 7 ? pv[pSub] : (i == 0 ? 1.0f : 0.8f);
        const float strumSamples = pv[pStrum] * 0.060f * (float)fs;
        const float glide = pv[pGlide] * 0.001f * (float)fs;
        // order by pitch so the strum goes low -> high
        int order[4] = {0, 1, 2, 3};
        for (int a = 0; a < n; ++a) for (int b = a + 1; b < n; ++b) if (iv[order[b]] < iv[order[a]]) std::swap(order[a], order[b]);
        for (int k = 0; k < n; ++k) {
            const int idx = order[k];
            Voice* v = allocVoice();
            v->note = note;  v->monoMember = mono ? k : -1;  v->interval = iv[idx];
            v->held = true;  v->vel = vel;  v->level = lv[idx];  v->tremDown = false;
            const float f = noteFreq(root + iv[idx]);
            const int strum = (int)(k * strumSamples * (0.8f + 0.4f * rng.uni01()));
            for (int c = 0; c < 2; ++c) {
                const float det = 1.0f + (c ? 1 : -1) * 0.0008f * (0.4f + rng.uni01());
                v->s[c].start(f, det, glide);
                const int dbl = c ? (int)(pv[pWidth] * (0.001f + 0.008f * rng.uni01()) * (float)fs) : 0;
                v->s[c].trigger(vel, strum + dbl, vel * 127.0f >= 120.0f);
            }
            v->active = true;
            v->tremCount = tremPeriod();
        }
    }

    void noteOff(int note) {
        const bool mono = (int)pv[pVoiceMode] == 1;
        if (mono) {
            int j = 0;  for (int i = 0; i < heldCount; ++i) if (held[i] != note) held[j++] = held[i];
            const bool had = j != heldCount;  heldCount = j;
            if (!had) return;
            if (heldCount > 0) { monoLegato(held[heldCount - 1] + (int)pv[pTranspose]); return; }
            monoActive = false;
        }
        for (auto& v : voices)
            if (v.active && v.held && v.note == note) { v.held = false; for (auto& s : v.s) s.release(); }
    }

    void allNotesOff() {
        heldCount = 0;  monoActive = false;
        for (auto& v : voices) if (v.active) { v.held = false; for (auto& s : v.s) s.release(); }
    }

    // ------------------------------------------------------------ audio
    // renders n samples (any n) of stereo audio
    void process(float* outL, float* outR, int n) {
        int pos = 0;
        while (pos < n) {
            const int m = std::min(kChunk, n - pos);
            processChunk(outL + pos, outR + pos, m);
            pos += m;
        }
    }

private:
    struct Voice {
        StringVoice s[2];
        bool active = false, held = false;
        int note = 0, interval = 0, monoMember = -1, age = 0, tremCount = 0;
        float vel = 1, level = 1;  bool tremDown = false;
    };

    double fs = 44100;
    Voice voices[kMaxVoices];
    StringParams sp;
    Gate gate;
    AmpChannel amp[2];
    CabChannel cab[2];
    StereoDelay delay;  Reverb reverb;
    AmpParams ap;
    Smooth gainS, outS;
    Rng rng;
    float pv[kNumParams];
    float bend01 = 0.5f, mod01 = 0.0f;
    bool dirty = true;
    int held[16];  int heldCount = 0;  bool monoActive = false;
    int ageCounter = 0;
    float sL[kChunk], sR[kChunk];

    static float noteFreq(int n) { return 440.0f * std::pow(2.0f, (n - 69) / 12.0f); }

    int tremPeriod() const {
        const float r = pv[pTremRate];
        if (r < 0.01f) return 0;
        return (int)(fs / (3.0f + 19.0f * r));
    }

    int chordIntervals(int* iv) const {
        static const int tab[8][3] = {{0, 99, 99}, {0, 7, 99}, {0, 7, 12}, {0, 12, 99}, {0, 5, 99}, {0, 6, 99}, {0, 3, 7}, {-12, 0, 7}};
        const int c = (int)pv[pChord];
        int n = 0;
        for (int i = 0; i < 3 && tab[c][i] != 99; ++i) iv[n++] = tab[c][i];
        if (pv[pSub] > 0.01f && c != 7) iv[n++] = -12;
        return n;
    }

    Voice* allocVoice() {
        Voice* best = nullptr;  int bestScore = -1;
        for (auto& v : voices) {
            if (!v.active) { best = &v; break; }
            int score = (v.held ? 0 : 1000000) + v.age;       // prefer released, then oldest
            if (score > bestScore) { bestScore = score; best = &v; }
        }
        best->age = 0;  ++ageCounter;
        return best;
    }

    void monoLegato(int root) {
        const float glide = pv[pGlide] * 0.001f * (float)fs;
        for (auto& v : voices)
            if (v.active && v.monoMember >= 0) {
                v.note = root;  v.held = true;
                for (auto& s : v.s) { s.released = false; s.relEnv = 1; s.glideTo(noteFreq(root + v.interval), glide); s.updateLoop(); }
            }
    }

    void applyParams() {
        if (!dirty) return;
        dirty = false;
        sp.attack = pv[pPickAttack];  sp.pickPos = pv[pPickPos];  sp.noise = pv[pPickNoise];
        sp.sustain = pv[pSustain];    sp.damping = pv[pDamping];
        sp.palm = std::min(1.0f, pv[pPalmMute] + mod01);
        sp.pinch = pv[pPinch];
        sp.bend = std::pow(2.0f, (bend01 - 0.5f) * 2.0f * pv[pBendRange] / 12.0f);
        ap.gateThresh = pv[pGateThresh];  ap.gateRel = pv[pGateRelease];
        ap.boostOn = pv[pBoostOn] > 0.5f; ap.boostDrive = pv[pBoostDrive];
        ap.boostTight = pv[pBoostTight];  ap.boostLevel = pv[pBoostLevel];
        ap.voicing = (int)pv[pVoicing];   ap.gain = pv[pGain];  ap.bright = pv[pBright];
        ap.bias = pv[pBias];  ap.sag = pv[pSag];  ap.bass = pv[pBass];  ap.mid = pv[pMid];
        ap.midFreq = pv[pMidFreq];  ap.treble = pv[pTreble];  ap.presence = pv[pPresence];
        ap.resonance = pv[pResonance];  ap.cab = (int)pv[pCabType];  ap.micPos = pv[pMicPos];
        ap.micDist = pv[pMicDist];  ap.lowCut = pv[pLowCut];  ap.highCut = pv[pHighCut];
        gate.configure(fs, ap.gateThresh, ap.gateRel);
        for (int c = 0; c < 2; ++c) { amp[c].configure(fs, ap); cab[c].configure(fs, ap); }
        delay.setTimeMs(pv[pDelayTime]);
        reverb.set(pv[pRevSize], pv[pRevDamp]);
        gainS.set(ap.gain);  outS.set(dbToLin(pv[pOutput]));
        for (auto& v : voices) if (v.active) for (auto& s : v.s) if (s.active) s.updateLoop();
    }

    void processChunk(float* L, float* R, int n) {
        applyParams();
        // ---- strings
        const int trem = tremPeriod();
        for (int i = 0; i < n; ++i) {
            float l = 0, r = 0;
            for (auto& v : voices) {
                if (!v.active) continue;
                if (trem > 0 && v.held && v.tremCount > 0 && --v.tremCount == 0) {
                    v.tremCount = trem;  v.tremDown = !v.tremDown;
                    const float vv = v.vel * (v.tremDown ? 0.78f : 1.0f);
                    v.s[0].trigger(vv, 0, false);
                    v.s[1].trigger(vv, (int)(pv[pWidth] * 0.003f * rng.uni01() * (float)fs), false);
                }
                l += v.s[0].process() * v.level;
                r += v.s[1].process() * v.level;
                if (!v.s[0].active && !v.s[1].active) v.active = false;
                ++v.age;
            }
            sL[i] = l;  sR[i] = r;
        }
        // ---- stereo width (double tracking)
        const float w = pv[pWidth];
        for (int i = 0; i < n; ++i) {
            const float mid = 0.5f * (sL[i] + sR[i]), side = 0.5f * (sL[i] - sR[i]);
            sL[i] = mid + side * w;  sR[i] = mid - side * w;
        }
        // ---- amp (skipped while silent: no voices and gate closed)
        bool anyVoice = false;  for (auto& v : voices) if (v.active) { anyVoice = true; break; }
        const Voicing& vc = kVoicings[ap.voicing];
        const float dm = pv[pDelayMix], dfb = pv[pDelayFb], rm = pv[pRevMix];
        for (int i = 0; i < n; ++i) {
            const float gn = gate.tick(sL[i], sR[i]);
            const float gp = gainS.tick();
            float l = 0, r = 0;
            if (anyVoice || gn > 1e-4f) {
                amp[0].setGains(vc, gp);  amp[1].setGains(vc, gp);
                l = amp[0].process(sL[i] * gn, vc);
                r = amp[1].process(sR[i] * gn, vc);
            }
            l = cab[0].process(l);  r = cab[1].process(r);
            if (dm > 0.001f) delay.process(l, r, dm, dfb);
            if (rm > 0.001f) reverb.process(l, r, rm);
            const float o = outS.tick();
            L[i] = softLimit(l * o);  R[i] = softLimit(r * o);
        }
    }

    static inline float softLimit(float x) {
        const float a = std::fabs(x);
        if (a < 0.9f) return x;
        const float y = 0.9f + 0.1f * std::tanh((a - 0.9f) * 10.0f);
        return x < 0 ? -y : y;
    }
};

} // namespace dg
