// Tests the user preset library: save, list, parse robustness, file-name rules, unique names.
#include "../src/userpresets.h"
#include "../src/presets.h"
#include <cstdio>
using namespace dg;
static int fails = 0;
#define CHECK(c) do { if (!(c)) { std::printf("FAIL line %d: %s\n", __LINE__, #c); ++fails; } } while (0)
int main() {
    const fs::path dir = fs::temp_directory_path() / "daxe_test_presets";
    fs::remove_all(dir);
    setenv("DEATHAXE_PRESET_DIR", dir.string().c_str(), 1);
    CHECK(listUserPresets().empty());                       // folder does not exist yet
    float v[kNumParams];  presetValues(5, v);
    CHECK(writePresetFile(dir / "Zed Riff.daxe", v));
    presetValues(9, v);
    CHECK(writePresetFile(dir / "alpha tone.daxe", v));
    auto l = listUserPresets();
    CHECK(l.size() == 2 && l[0].name == "alpha tone" && l[1].name == "Zed Riff");     // case-insensitive sort
    float w[kNumParams];  presetValues(5, w);
    for (int i = 0; i < kNumParams; ++i) if (std::fabs(l[1].v[i] - w[i]) > 1e-4f * (1 + std::fabs(w[i]))) { std::printf("mismatch %s\n", kParams[i].name); ++fails; }
    // unique naming
    CHECK(uniquePresetPath(dir, "Zed Riff").filename() == "Zed Riff 2.daxe");
    CHECK(uniquePresetPath(dir, "Fresh").filename() == "Fresh.daxe");
    CHECK(sanitizeFileStem("a/b:c*?") == "a_b_c__");
    CHECK(sanitizeFileStem("  ...  ") == "Untitled");
    // garbage / hostile files are ignored or clamped
    { FILE* f = openFile(dir / "bad1.daxe", "wb"); std::fputs("not a preset\nGain=0.5\n", f); std::fclose(f); }
    { FILE* f = openFile(dir / "odd.daxe", "wb"); std::fputs("DeathAxe-preset 1\nGain=999\nAmp Voicing=-4\nBass=nan\nSustain=abc\nTranspose=3.7\n", f); std::fclose(f); }
    { FILE* f = openFile(dir / "bin.daxe", "wb"); for (int i = 0; i < 100000; ++i) std::fputc(i % 251, f); std::fclose(f); }
    { FILE* f = openFile(dir / "notes.txt", "wb"); std::fputs("DeathAxe-preset 1\n", f); std::fclose(f); }
    l = listUserPresets();
    CHECK(l.size() == 3);                                   // alpha tone, odd, Zed Riff
    for (auto& u : l) if (u.name == "odd") {
        CHECK(u.v[pGain] == 1.0f && u.v[pVoicing] == 0 && u.v[pBass] == 0 && u.v[pSustain] == 4 && u.v[pTranspose] == 4);
    }
    // unicode file names survive
    presetValues(2, v);  CHECK(writePresetFile(dir / fs::u8path("R\xC3\xA9sum\xC3\xA9 \xE2\x98\xA0.daxe"), v));
    l = listUserPresets();  bool found = false;  for (auto& u : l) if (u.name == "R\xC3\xA9sum\xC3\xA9 \xE2\x98\xA0") found = true;
    CHECK(found);
    fs::remove_all(dir);
    std::printf(fails ? "userpreset test FAILED\n" : "userpreset test passed\n");
    return fails ? 1 : 0;
}
