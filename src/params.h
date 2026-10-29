// DeathAxe - parameter table shared by the DSP engine, the VST3 wrapper and the test tools.
#pragma once
#include <cmath>
#include <cstring>

namespace dg {

enum ParamId : int {
    // --- string / performance ---
    pTranspose = 0, pBendRange, pVoiceMode, pGlide, pChord, pStrum, pSub, pWidth,
    pPickAttack, pPickPos, pPickNoise, pSustain, pDamping, pPalmMute, pTremRate, pPinch,
    // --- gate / boost ---
    pGateThresh, pGateRelease, pBoostOn, pBoostDrive, pBoostTight, pBoostLevel,
    // --- amp ---
    pVoicing, pGain, pBright, pBias, pSag, pBass, pMid, pMidFreq, pTreble, pPresence, pResonance,
    // --- cab / output filters ---
    pCabType, pMicPos, pMicDist, pLowCut, pHighCut,
    // --- fx ---
    pDelayMix, pDelayTime, pDelayFb, pRevMix, pRevSize, pRevDamp, pOutput,
    kNumParams
};

// Parameters that are not part of the preset table
enum : unsigned { kPidPitchBend = 200, kPidModWheel = 201, kPidProgram = 300 };

enum ParamType { T_LIN, T_LOG, T_INT, T_ENUM, T_BOOL };

struct ParamDef {
    const char* name;
    const char* unit;
    ParamType   type;
    float       minV, maxV, defV;
    const char* choices;   // '|' separated for T_ENUM
};

inline const ParamDef kParams[kNumParams] = {
    /* pTranspose   */ {"Transpose",      "st",  T_INT,  -24, 24, 0, nullptr},
    /* pBendRange   */ {"Bend Range",     "st",  T_INT,    0, 12, 2, nullptr},
    /* pVoiceMode   */ {"Voice Mode",     "",    T_ENUM,   0,  1, 0, "Poly|Mono Legato"},
    /* pGlide       */ {"Glide",          "ms",  T_LIN,    0, 500, 0, nullptr},
    /* pChord       */ {"Chord",          "",    T_ENUM,   0,  7, 0, "Single|Power 5th|Power + Octave|Octave|Power 4th|Tritone|Minor|Sub Power"},
    /* pStrum       */ {"Strum",          "",    T_LIN,    0,  1, 0.15f, nullptr},
    /* pSub         */ {"Sub Octave",     "",    T_LIN,    0,  1, 0, nullptr},
    /* pWidth       */ {"Double Track",   "",    T_LIN,    0,  1, 0.8f, nullptr},
    /* pPickAttack  */ {"Pick Attack",    "",    T_LIN,    0,  1, 0.7f, nullptr},
    /* pPickPos     */ {"Pickup Pos",     "",    T_LIN,    0,  1, 0.3f, nullptr},
    /* pPickNoise   */ {"Pick Noise",     "",    T_LIN,    0,  1, 0.25f, nullptr},
    /* pSustain     */ {"Sustain",        "s",   T_LOG,  0.2f, 20, 4, nullptr},
    /* pDamping     */ {"String Damping", "",    T_LIN,    0,  1, 0.35f, nullptr},
    /* pPalmMute    */ {"Palm Mute",      "",    T_LIN,    0,  1, 0, nullptr},
    /* pTremRate    */ {"Tremolo Pick",   "",    T_LIN,    0,  1, 0, nullptr},
    /* pPinch       */ {"Pinch Harmonic", "",    T_LIN,    0,  1, 0, nullptr},

    /* pGateThresh  */ {"Gate Threshold", "dB",  T_LIN,  -80, -10, -55, nullptr},
    /* pGateRelease */ {"Gate Release",   "ms",  T_LOG,    5, 500, 60, nullptr},
    /* pBoostOn     */ {"Boost",          "",    T_BOOL,   0,  1, 1, nullptr},
    /* pBoostDrive  */ {"Boost Drive",    "",    T_LIN,    0,  1, 0.35f, nullptr},
    /* pBoostTight  */ {"Boost Tight",    "Hz",  T_LOG,   60, 500, 180, nullptr},
    /* pBoostLevel  */ {"Boost Level",    "dB",  T_LIN,  -12, 12, 0, nullptr},

    /* pVoicing     */ {"Amp Voicing",    "",    T_ENUM,   0,  4, 0, "Modern Scoop|Brutal Mid|Old School Tampa|Swedish Chainsaw|Tech Tight"},
    /* pGain        */ {"Gain",           "",    T_LIN,    0,  1, 0.7f, nullptr},
    /* pBright      */ {"Bright",         "",    T_LIN,    0,  1, 0.3f, nullptr},
    /* pBias        */ {"Bias",           "",    T_LIN,    0,  1, 0.5f, nullptr},
    /* pSag         */ {"Sag",            "",    T_LIN,    0,  1, 0.3f, nullptr},
    /* pBass        */ {"Bass",           "dB",  T_LIN,  -12, 12, 0, nullptr},
    /* pMid         */ {"Mid",            "dB",  T_LIN,  -15, 15, 0, nullptr},
    /* pMidFreq     */ {"Mid Freq",       "Hz",  T_LOG,  200, 2500, 650, nullptr},
    /* pTreble      */ {"Treble",         "dB",  T_LIN,  -12, 12, 0, nullptr},
    /* pPresence    */ {"Presence",       "dB",  T_LIN,  -10, 10, 0, nullptr},
    /* pResonance   */ {"Resonance",      "dB",  T_LIN,   -8, 10, 0, nullptr},

    /* pCabType     */ {"Cab Type",       "",    T_ENUM,   0,  5, 0, "4x12 V30|4x12 Greenback|2x12 Tight|1x12 Open|8x10 Beast|Direct (no cab)"},
    /* pMicPos      */ {"Mic Position",   "",    T_LIN,    0,  1, 0.35f, nullptr},
    /* pMicDist     */ {"Mic Distance",   "",    T_LIN,    0,  1, 0.25f, nullptr},
    /* pLowCut      */ {"Low Cut",        "Hz",  T_LOG,   20, 400, 45, nullptr},
    /* pHighCut     */ {"High Cut",       "Hz",  T_LOG, 2000, 20000, 12000, nullptr},

    /* pDelayMix    */ {"Delay Mix",      "",    T_LIN,    0,  1, 0, nullptr},
    /* pDelayTime   */ {"Delay Time",     "ms",  T_LIN,   20, 1000, 320, nullptr},
    /* pDelayFb     */ {"Delay Feedback", "",    T_LIN,    0, 0.95f, 0.3f, nullptr},
    /* pRevMix      */ {"Reverb Mix",     "",    T_LIN,    0,  1, 0.08f, nullptr},
    /* pRevSize     */ {"Reverb Size",    "",    T_LIN,    0,  1, 0.4f, nullptr},
    /* pRevDamp     */ {"Reverb Damping", "",    T_LIN,    0,  1, 0.5f, nullptr},
    /* pOutput      */ {"Output Level",   "dB",  T_LIN,  -24, 12, -6, nullptr},
};

inline int enumCount(const ParamDef& d) {
    if (!d.choices) return 0;
    int n = 1;
    for (const char* c = d.choices; *c; ++c) if (*c == '|') ++n;
    return n;
}

// number of discrete steps (0 = continuous)
inline int stepCount(const ParamDef& d) {
    switch (d.type) {
        case T_INT:  return (int)(d.maxV - d.minV + 0.5f);
        case T_ENUM: return enumCount(d) - 1;
        case T_BOOL: return 1;
        default:     return 0;
    }
}

inline float toPlain(int id, double n) {
    const ParamDef& d = kParams[id];
    if (n < 0) n = 0; if (n > 1) n = 1;
    switch (d.type) {
        case T_LIN:  return (float)(d.minV + n * (d.maxV - d.minV));
        case T_LOG:  return (float)(d.minV * std::pow((double)d.maxV / d.minV, n));
        default: {
            int s = stepCount(d);
            return d.minV + (float)(int)(n * s + 0.5);
        }
    }
}

inline double toNorm(int id, float p) {
    const ParamDef& d = kParams[id];
    double n;
    switch (d.type) {
        case T_LIN: n = (p - d.minV) / (d.maxV - d.minV); break;
        case T_LOG: n = std::log((double)p / d.minV) / std::log((double)d.maxV / d.minV); break;
        default: { int s = stepCount(d); n = s ? (p - d.minV) / s : 0.0; }
    }
    if (n < 0) n = 0; if (n > 1) n = 1;
    return n;
}

// copy the n-th '|' separated choice into out
inline void enumChoice(const ParamDef& d, int idx, char* out, int cap) {
    const char* c = d.choices;
    for (int i = 0; i < idx && c; ++i) { c = std::strchr(c, '|'); if (c) ++c; }
    int n = 0;
    while (c && *c && *c != '|' && n < cap - 1) out[n++] = *c++;
    out[n] = 0;
}

} // namespace dg
