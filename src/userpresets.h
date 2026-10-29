// DeathAxe - user preset library: one small text file (.daxe) per preset in a folder.
//   Folder:  Windows  %APPDATA%\DeathAxe\Presets
//            Linux    $XDG_CONFIG_HOME/DeathAxe/Presets  (default ~/.config/DeathAxe/Presets)
//            macOS    ~/Library/Application Support/DeathAxe/Presets
//   Override with the DEATHAXE_PRESET_DIR environment variable.
//   The preset's name is its file name. File format (plain text, safe to edit or share):
//       DeathAxe-preset 1
//       name=My Riff Tone      <- informational; the file name is what is shown
//       Gain=0.72              <- one line per parameter, by name; missing ones fall back to defaults
#pragma once
#include "params.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>
#include <vector>

namespace dg {

namespace fs = std::filesystem;

struct UserPreset {
    std::string name;      // file stem
    std::string path;      // UTF-8 full path
    float v[kNumParams];
};

inline fs::path userPresetDir() {
    if (const char* o = std::getenv("DEATHAXE_PRESET_DIR")) if (*o) return fs::u8path(o);
#if defined(_WIN32)
    if (const wchar_t* a = _wgetenv(L"APPDATA")) return fs::path(a) / L"DeathAxe" / L"Presets";
    return fs::path(L".") / L"DeathAxePresets";
#elif defined(__APPLE__)
    const char* h = std::getenv("HOME");
    return fs::path(h ? h : ".") / "Library" / "Application Support" / "DeathAxe" / "Presets";
#else
    if (const char* x = std::getenv("XDG_CONFIG_HOME")) if (*x) return fs::path(x) / "DeathAxe" / "Presets";
    const char* h = std::getenv("HOME");
    return fs::path(h ? h : ".") / ".config" / "DeathAxe" / "Presets";
#endif
}

inline FILE* openFile(const fs::path& p, const char* mode) {
#if defined(_WIN32)
    std::wstring m;  for (const char* c = mode; *c; ++c) m += (wchar_t)*c;
    return _wfopen(p.c_str(), m.c_str());
#else
    return std::fopen(p.c_str(), mode);
#endif
}

// characters Windows (and most other systems) do not allow in file names are replaced
inline std::string sanitizeFileStem(const std::string& in) {
    std::string out;
    for (unsigned char c : in) {
        if (c < 32 || std::strchr("<>:\"/\\|?*", (int)c)) c = '_';
        out += (char)c;
    }
    while (!out.empty() && (out.back() == ' ' || out.back() == '.')) out.pop_back();
    while (!out.empty() && out.front() == ' ') out.erase(out.begin());
    if (out.size() > 80) out.resize(80);
    if (out.empty()) out = "Untitled";
    return out;
}

// parse a preset file; false if it is not a DeathAxe preset
inline bool parsePresetFile(const fs::path& path, float* vals) {
    FILE* f = openFile(path, "rb");
    if (!f) return false;
    char line[256];
    bool header = false;
    for (int i = 0; i < kNumParams; ++i) vals[i] = kParams[i].defV;
    int lines = 0;
    while (std::fgets(line, sizeof line, f) && ++lines < 200) {
        char* e = line + std::strlen(line);
        while (e > line && (e[-1] == '\n' || e[-1] == '\r')) *--e = 0;
        if (!header) { if (std::strncmp(line, "DeathAxe-preset", 15) != 0) { std::fclose(f); return false; } header = true; continue; }
        char* eq = std::strchr(line, '=');
        if (!eq) continue;
        *eq = 0;  const char* key = line;  const char* val = eq + 1;
        for (int i = 0; i < kNumParams; ++i)
            if (!std::strcmp(key, kParams[i].name)) {
                char* end = nullptr;  float x = std::strtof(val, &end);
                if (end == val || x != x) break;
                const ParamDef& d = kParams[i];
                if (x < d.minV) x = d.minV;
                if (x > d.maxV) x = d.maxV;
                if (d.type == T_INT || d.type == T_ENUM || d.type == T_BOOL) x = d.minV + (float)(int)(x - d.minV + 0.5f);
                vals[i] = x;  break;
            }
    }
    std::fclose(f);
    return header;
}

inline bool writePresetFile(const fs::path& path, const float* vals) {
    std::error_code ec;
    fs::create_directories(path.parent_path(), ec);
    const fs::path tmp = path.string() + ".tmp";
    FILE* f = openFile(tmp, "wb");
    if (!f) return false;
    std::fprintf(f, "DeathAxe-preset 1\nname=%s\n", sanitizeFileStem(path.stem().u8string()).c_str());
    for (int i = 0; i < kNumParams; ++i) std::fprintf(f, "%s=%.6g\n", kParams[i].name, (double)vals[i]);
    const bool ok = std::fflush(f) == 0;
    std::fclose(f);
    if (!ok) { fs::remove(tmp, ec); return false; }
    fs::rename(tmp, path, ec);
    if (ec) { fs::remove(path, ec); ec.clear(); fs::rename(tmp, path, ec); }
    return !ec;
}

inline bool ciLess(const std::string& a, const std::string& b) {
    return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end(), [](unsigned char x, unsigned char y) { return std::tolower(x) < std::tolower(y); });
}

// all valid presets in the library folder, sorted by name
inline std::vector<UserPreset> listUserPresets() {
    std::vector<UserPreset> out;
    std::error_code ec;
    for (fs::directory_iterator it(userPresetDir(), ec), end; !ec && it != end; it.increment(ec)) {
        const fs::path p = it->path();
        if (!it->is_regular_file(ec) || p.extension() != ".daxe") continue;
        UserPreset u;
        if (!parsePresetFile(p, u.v)) continue;
        u.name = p.stem().u8string();  u.path = p.u8string();
        out.push_back(std::move(u));
        if (out.size() >= 2000) break;
    }
    std::sort(out.begin(), out.end(), [](const UserPreset& a, const UserPreset& b) { return ciLess(a.name, b.name); });
    return out;
}

// a path in dir for stem that does not exist yet ("Name.daxe", "Name 2.daxe", ...)
inline fs::path uniquePresetPath(const fs::path& dir, const std::string& stem) {
    std::error_code ec;
    fs::path p = dir / fs::u8path(stem + ".daxe");
    for (int n = 2; fs::exists(p, ec) && n < 1000; ++n) p = dir / fs::u8path(stem + " " + std::to_string(n) + ".daxe");
    return p;
}

} // namespace dg
