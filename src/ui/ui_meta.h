// DeathAxe UI - short display names, help text and value formatting for every parameter.
#pragma once
#include "../params.h"
#include <cstdio>
#include <string>

namespace dg { namespace ui {

inline const char* uiName(int pid) {
    static const char* n[kNumParams] = {
        "Transpose", "Bend", "Mode", "Glide", "Chord", "Strum", "Sub Oct", "Double",
        "Attack", "Pickup", "Noise", "Sustain", "Damping", "Palm Mute", "Tremolo", "Pinch",
        "Threshold", "Release", "Boost", "Drive", "Tight", "Level",
        "Voicing", "Gain", "Bright", "Bias", "Sag", "Bass", "Mid", "Mid Freq", "Treble", "Presence", "Resonance",
        "Cabinet", "Mic Pos", "Mic Dist", "Low Cut", "High Cut",
        "Mix", "Time", "Feedback", "Mix", "Size", "Damping", "Output"};
    return n[pid];
}

inline const char* uiHelp(int pid) {
    static const char* h[kNumParams] = {
        "Transpose: shifts everything you play, in semitones. Use negative values for lower tunings.",
        "Bend range: how far the pitch-bend wheel bends, in semitones.",
        "Voice mode: Poly lets notes overlap. Mono plays one note at a time and overlapping notes slur like hammer-ons.",
        "Glide: slide time between slurred notes in Mono mode.",
        "Chord: every key plays extra strings (power chords, octaves, fourths, tritone, minor, sub power).",
        "Strum: time spread between the strings of a chord, lowest string first.",
        "Sub octave: adds a string one octave below the note, at this level.",
        "Double track: two separately picked strings panned left and right. 0 = mono.",
        "Pick attack: brightness and snap of the pick hitting the string.",
        "Pickup position: bridge pickup (bright and bitey) to neck pickup (warm and round).",
        "Pick noise: amount of pick click and scrape.",
        "Sustain: how long the string rings out.",
        "String damping: darkens the string as it fades. Higher = duller.",
        "Palm mute: mutes the string with the palm, shorter and darker. The mod wheel adds to it.",
        "Tremolo picking: while a key is held the note is re-picked automatically. Rate in Hz = 3 + 19 x value.",
        "Pinch harmonic: squeal on notes played at velocity 120 or higher.",
        "Gate threshold: noise gate before the amp. Higher = tighter and choppier.",
        "Gate release: how fast the gate closes after a note. Short = staccato chop.",
        "Boost: overdrive pedal in front of the amp. Click to switch it on or off.",
        "Boost drive: how hard the pedal clips.",
        "Boost tight: cuts bass before the distortion for a tighter low end.",
        "Boost level: output of the pedal. Pushes the amp harder.",
        "Amp voicing: the preamp character. Use the arrows or click the name to pick one.",
        "Gain: amount of preamp distortion.",
        "Bright: high-end bite added before the clipping stages.",
        "Bias: clipping asymmetry. More even harmonics and a rougher edge.",
        "Sag: power-amp compression. Higher feels spongier and looser.",
        "Bass: low end of the tone stack (inside the preamp, so it changes how the distortion reacts).",
        "Mid: mid-range boost or cut at the Mid Freq frequency. Cut it for a scooped sound.",
        "Mid frequency: centre frequency of the Mid control.",
        "Treble: high end of the tone stack.",
        "Presence: high-frequency lift in the power amp.",
        "Resonance: low-frequency thump in the power amp.",
        "Cabinet: speaker cabinet model. Use the arrows or click the name to pick one.",
        "Mic position: centre of the speaker (bright) to its edge (dark).",
        "Mic distance: close (more bass, dry) to far (more room sound).",
        "Low cut: removes rumble below this frequency.",
        "High cut: removes fizz above this frequency.",
        "Delay mix: how much echo is added.",
        "Delay time: time between echoes.",
        "Delay feedback: how many repeats you hear.",
        "Reverb mix: how much reverb is added.",
        "Reverb size: from small room to huge hall.",
        "Reverb damping: higher = darker, shorter reverb tail.",
        "Output level. A soft limiter after this keeps the signal under 0 dBFS."};
    return h[pid];
}

inline std::string formatValue(int pid, float p) {
    const ParamDef& d = kParams[pid];
    char b[64];
    if (d.type == T_ENUM) { char c[64]; enumChoice(d, (int)(p - d.minV + 0.5f), c, 64); return c; }
    if (d.type == T_BOOL) return p > 0.5f ? "On" : "Off";
    if (d.type == T_INT) { std::snprintf(b, sizeof b, "%+d %s", (int)std::lround(p), d.unit); if ((int)std::lround(p) == 0) std::snprintf(b, sizeof b, "0 %s", d.unit); return b; }
    if (d.unit[0] == 0) { std::snprintf(b, sizeof b, "%.0f%%", p * 100.0f); return b; }
    const std::string u = d.unit;
    if (u == "Hz") { if (p >= 1000) std::snprintf(b, sizeof b, "%.2f kHz", p / 1000.0f); else std::snprintf(b, sizeof b, "%.0f Hz", p); }
    else if (u == "ms") std::snprintf(b, sizeof b, "%.0f ms", p);
    else if (u == "s") std::snprintf(b, sizeof b, "%.1f s", p);
    else if (u == "dB") { if (std::fabs(p) < 0.05f) std::snprintf(b, sizeof b, "0.0 dB"); else std::snprintf(b, sizeof b, "%+.1f dB", p); }
    else std::snprintf(b, sizeof b, "%.1f %s", p, d.unit);
    return b;
}

}} // namespace dg::ui
