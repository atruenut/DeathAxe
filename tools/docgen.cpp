// Prints markdown tables for the parameter reference and the preset list (keeps the docs in sync with the code).
#include "../src/params.h"
#include "../src/presets.h"
#include <cstdio>
using namespace dg;
int main(int argc, char** argv) {
    const bool presets = argc > 1 && std::string(argv[1]) == "presets";
    if (presets) {
        std::printf("| # | Preset | What it is |\n|---|---|---|\n");
        for (int i = 0; i < numPresets(); ++i) std::printf("| %d | **%s** | %s |\n", i + 1, presetName(i), presetDesc(i));
        return 0;
    }
    std::printf("| Parameter | Range | Default |\n|---|---|---|\n");
    for (int i = 0; i < kNumParams; ++i) {
        const ParamDef& d = kParams[i];
        char range[512], def[64];
        if (d.type == T_ENUM) {
            std::string s; const int n = enumCount(d);
            for (int k = 0; k < n; ++k) { char c[64]; enumChoice(d, k, c, 64); s += (k ? " / " : ""); s += c; }
            std::snprintf(range, sizeof range, "%s", s.c_str());
            char c[64]; enumChoice(d, (int)d.defV, c, 64); std::snprintf(def, sizeof def, "%s", c);
        } else if (d.type == T_BOOL) {
            std::snprintf(range, sizeof range, "Off / On"); std::snprintf(def, sizeof def, "%s", d.defV > 0.5f ? "On" : "Off");
        } else {
            const bool pct = d.maxV <= 1.0f && d.unit[0] == 0;
            if (pct) { std::snprintf(range, sizeof range, "%.0f–%.0f %%", d.minV * 100, d.maxV * 100); std::snprintf(def, sizeof def, "%.0f %%", d.defV * 100); }
            else { std::snprintf(range, sizeof range, "%g–%g %s%s", d.minV, d.maxV, d.unit, d.type == T_LOG ? " (log)" : ""); std::snprintf(def, sizeof def, "%g %s", d.defV, d.unit); }
        }
        std::printf("| %s | %s | %s |\n", d.name, range, def);
    }
    return 0;
}
