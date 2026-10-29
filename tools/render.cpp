// DeathAxe offline renderer / analyser.
//   render [--preset N] [--out file.wav] [--sr 44100] [--set Name=value ...] [--quiet]
// Plays a built-in test riff (palm-muted chugs, open power chord, tremolo picking, pinch harmonic),
// writes a WAV and prints level / sanity / octave-band statistics so presets can be checked without a DAW.
#include "../src/engine.h"
#include "../src/presets.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <complex>
#include <string>

using namespace dg;

static void fft(std::vector<std::complex<double>>& a) {
    const size_t n = a.size();
    for (size_t i = 1, j = 0; i < n; ++i) {
        size_t bit = n >> 1;
        for (; j & bit; bit >>= 1) j ^= bit;
        j ^= bit;
        if (i < j) std::swap(a[i], a[j]);
    }
    for (size_t len = 2; len <= n; len <<= 1) {
        const double ang = -2 * kPi / len;
        std::complex<double> wl(std::cos(ang), std::sin(ang));
        for (size_t i = 0; i < n; i += len) {
            std::complex<double> w(1);
            for (size_t j = 0; j < len / 2; ++j) {
                auto u = a[i + j], v = a[i + j + len / 2] * w;
                a[i + j] = u + v;  a[i + j + len / 2] = u - v;  w *= wl;
            }
        }
    }
}

struct Ev { double t; int type; int a; float b; };   // 0 on, 1 off, 2 param(a=id,b=val)

static void writeWav(const char* path, const std::vector<float>& l, const std::vector<float>& r, int sr) {
    FILE* f = std::fopen(path, "wb");
    if (!f) return;
    const uint32_t n = (uint32_t)l.size(), dataBytes = n * 4, riff = 36 + dataBytes, fmtLen = 16, rate = sr, br = sr * 4;
    const uint16_t pcm = 1, ch = 2, ba = 4, bits = 16;
    std::fwrite("RIFF", 1, 4, f); std::fwrite(&riff, 4, 1, f); std::fwrite("WAVEfmt ", 1, 8, f);
    std::fwrite(&fmtLen, 4, 1, f); std::fwrite(&pcm, 2, 1, f); std::fwrite(&ch, 2, 1, f);
    std::fwrite(&rate, 4, 1, f); std::fwrite(&br, 4, 1, f); std::fwrite(&ba, 2, 1, f); std::fwrite(&bits, 2, 1, f);
    std::fwrite("data", 1, 4, f); std::fwrite(&dataBytes, 4, 1, f);
    for (uint32_t i = 0; i < n; ++i) {
        int16_t s[2] = {(int16_t)std::lrint(std::max(-1.f, std::min(1.f, l[i])) * 32767),
                        (int16_t)std::lrint(std::max(-1.f, std::min(1.f, r[i])) * 32767)};
        std::fwrite(s, 2, 2, f);
    }
    std::fclose(f);
}

static void analyse(const char* label, const float* l, const float* r, int n, int sr, bool bands) {
    double sum = 0, peak = 0;  int bad = 0;
    for (int i = 0; i < n; ++i) {
        for (float v : {l[i], r[i]}) {
            if (!std::isfinite(v)) { ++bad; continue; }
            sum += (double)v * v;  peak = std::max(peak, (double)std::fabs(v));
        }
    }
    const double rms = std::sqrt(sum / (2.0 * std::max(1, n)));
    std::printf("  %-14s rms %6.1f dBFS  peak %6.1f dBFS  nonfinite %d\n", label,
                20 * std::log10(rms + 1e-12), 20 * std::log10(peak + 1e-12), bad);
    if (!bands || n < 16384) return;
    const int N = 16384;  std::vector<double> mag(N / 2, 0.0);  int frames = 0;
    for (int s = 0; s + N <= n; s += N / 2) {
        std::vector<std::complex<double>> a(N);
        for (int i = 0; i < N; ++i) a[i] = 0.5 * (l[s + i] + r[s + i]) * (0.5 - 0.5 * std::cos(2 * kPi * i / N));
        fft(a);
        for (int k = 0; k < N / 2; ++k) mag[k] += std::norm(a[k]);
        ++frames;
    }
    static const double edges[] = {45, 90, 180, 355, 710, 1400, 2800, 5600, 11200, 22400};
    std::printf("    octave bands (rel. dB, 45-90 up to 11-22k Hz):");
    double e[9], mx = 1e-30;
    for (int b = 0; b < 9; ++b) {
        e[b] = 0;
        for (int k = 1; k < N / 2; ++k) { const double f = (double)k * sr / N; if (f >= edges[b] && f < edges[b + 1]) e[b] += mag[k]; }
        mx = std::max(mx, e[b]);
    }
    for (int b = 0; b < 9; ++b) std::printf(" %5.1f", 10 * std::log10(e[b] / mx + 1e-12));
    std::printf("\n");
}

int main(int argc, char** argv) {
    int sr = 44100, preset = 0;  std::string out;  bool quiet = false, all = false;
    std::vector<std::pair<int, float>> sets;
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--preset" && i + 1 < argc) preset = std::atoi(argv[++i]);
        else if (a == "--out" && i + 1 < argc) out = argv[++i];
        else if (a == "--sr" && i + 1 < argc) sr = std::atoi(argv[++i]);
        else if (a == "--quiet") quiet = true;
        else if (a == "--all") all = true;
        else if (a == "--set" && i + 1 < argc) {
            std::string kv = argv[++i];  const size_t eq = kv.find('=');
            const std::string name = kv.substr(0, eq);  const float val = (float)std::atof(kv.c_str() + eq + 1);
            bool found = false;
            for (int p = 0; p < kNumParams; ++p) if (name == kParams[p].name) { sets.push_back({p, val}); found = true; }
            if (!found) std::fprintf(stderr, "unknown param %s\n", name.c_str());
        }
    }

    int rc = 0;
    const int first = all ? 0 : preset, last = all ? numPresets() - 1 : preset;
    for (int pi = first; pi <= last; ++pi) {
        Engine eng;  eng.init(sr);
        float vals[kNumParams];  presetValues(pi, vals);
        for (auto& s : sets) vals[s.first] = s.second;
        eng.loadPreset(vals);
        const float palm = vals[pPalmMute], trem = vals[pTremRate];

        std::vector<Ev> ev;
        // 1) palm-muted chugs on low E (E2 = 40) - uses the preset's own palm mute, boosted to at least 0.75
        ev.push_back({0.0, 2, pPalmMute, std::max(palm, 0.75f)});
        ev.push_back({0.0, 2, pTremRate, 0.0f});
        for (int k = 0; k < 8; ++k) { ev.push_back({0.2 + k * 0.2, 0, 40, 0.8f}); ev.push_back({0.2 + k * 0.2 + 0.14, 1, 40, 0}); }
        // 2) open power chord ring (A2 = 45) 2.5 s
        ev.push_back({2.2, 2, pPalmMute, 0.0f});
        ev.push_back({2.3, 0, 45, 0.85f});  ev.push_back({4.8, 1, 45, 0});
        // 3) tremolo picking on a mid note (E3 = 52)
        ev.push_back({5.2, 2, pTremRate, std::max(trem, 0.6f)});
        ev.push_back({5.3, 0, 52, 0.8f});  ev.push_back({7.3, 1, 52, 0});
        ev.push_back({7.3, 2, pTremRate, trem});
        // 4) pinch harmonic (high velocity)
        ev.push_back({7.8, 2, pPinch, std::max(vals[pPinch], 0.8f)});
        ev.push_back({7.9, 0, 57, 1.0f});  ev.push_back({9.6, 1, 57, 0});
        const double total = 11.0;
        std::stable_sort(ev.begin(), ev.end(), [](const Ev& a, const Ev& b) { return a.t < b.t; });

        const int N = (int)(total * sr);
        std::vector<float> L(N), R(N);
        size_t e = 0;  int pos = 0;
        while (pos < N) {
            while (e < ev.size() && (int)(ev[e].t * sr) <= pos) {
                const Ev& x = ev[e++];
                if (x.type == 0) eng.noteOn(x.a, x.b);
                else if (x.type == 1) eng.noteOff(x.a);
                else eng.setParam(x.a, x.b);
            }
            int next = N;  if (e < ev.size()) next = std::min(N, (int)(ev[e].t * sr));
            const int m = std::max(1, std::min(next - pos, 512));
            eng.process(&L[pos], &R[pos], m);
            pos += m;
        }
        if (!quiet || all) std::printf("[%2d] %s\n", pi, presetName(pi));
        auto seg = [&](const char* nm, double a, double b, bool bands) {
            const int s = (int)(a * sr), n = (int)((b - a) * sr);
            analyse(nm, &L[s], &R[s], n, sr, bands);
        };
        seg("chugs", 0.2, 2.0, true);  seg("power chord", 2.4, 4.7, true);
        seg("tremolo pick", 5.4, 7.2, true);  seg("pinch", 8.0, 9.5, false);
        seg("tail (10-11s)", 10.0, 11.0, false);
        std::string path = out.empty() ? "" : out;
        if (all) path = std::string("renders/preset_") + (pi < 10 ? "0" : "") + std::to_string(pi) + ".wav";
        if (!path.empty()) writeWav(path.c_str(), L, R, sr);
    }
    return rc;
}
