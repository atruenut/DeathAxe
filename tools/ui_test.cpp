// Headless UI test: renders snapshots and simulates mouse interaction against a mock host.
#include "../src/ui/editor.h"
#include <cstdio>
#include <cstdlib>
using namespace dg; using namespace dg::ui;
static void bmp(const Canvas& c, const char* path) {
    FILE* f = fopen(path, "wb"); const int w = c.w, h = c.h, rs = w * 3, ps = (rs + 3) & ~3;
    unsigned char hd[54] = {'B','M'}; unsigned sz = 54 + ps * h; memcpy(hd+2,&sz,4); hd[10]=54; hd[14]=40; memcpy(hd+18,&w,4); int nh=-h; memcpy(hd+22,&nh,4); hd[26]=1; hd[28]=24;
    fwrite(hd,1,54,f); std::vector<unsigned char> row(ps,0);
    for (int y=0;y<h;++y){ for(int x=0;x<w;++x){ uint32_t p=c.px[(size_t)y*w+x]; row[x*3]=p&255; row[x*3+1]=(p>>8)&255; row[x*3+2]=(p>>16)&255;} fwrite(row.data(),1,ps,f);} fclose(f);
}
struct MockHost : Host {
    float v[kNumParams]; PresetState ps; int edits = 0, open = 0;
    MockHost() { for (int i = 0; i < kNumParams; ++i) v[i] = kParams[i].defV; }
    float get(int p) override { return v[p]; }
    void begin(int) override { ++open; }
    void set(int p, float x) override { v[p] = x; ++edits; }
    void end(int) override { --open; }
    PresetState& presetState() override { return ps; }
};
struct MockPlat : Platform {
    std::string savePath; bool conf = true; std::string loadPath;
    bool saveDialog(const std::string&, const std::string&, std::string& o) override { o = savePath; return !savePath.empty(); }
    bool openDialog(const std::string&, std::string& o) override { o = loadPath; return !loadPath.empty(); }
    bool confirm(const std::string&) override { return conf; }
    void message(const std::string& t) override { printf("  [message] %s\n", t.c_str()); }
};
static int fails = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); ++fails; } } while (0)
int main(int argc, char** argv) {
    const char* tmp = getenv("TMPDIR_UI") ? getenv("TMPDIR_UI") : "build/ui_tmp";
    std::string d = tmp; system(("rm -rf " + d + " && mkdir -p " + d).c_str());
    setenv("DEATHAXE_PRESET_DIR", d.c_str(), 1);
    MockHost h; MockPlat pl; Editor ed(&h, &pl);
    const float scale = argc > 1 ? (float)atof(argv[1]) : 1.0f;
    ed.open(scale); ed.renderAll(); bmp(ed.canvas(), "build/ui_1.bmp");
    CHECK(h.ps.valid);
    Mods m;
    // knob drag
    Widget* k = ed.findKnob(pGain); CHECK(k);
    float cx = k->x + k->w * 0.5f, cy = k->y + 27;
    float g0 = h.v[pGain];
    ed.mouseMove(cx, cy, m); ed.mouseDown(cx, cy, m); ed.mouseMove(cx, cy - 36, m); ed.mouseUp(cx, cy - 36, m);
    CHECK(h.v[pGain] > g0 + 0.15f); CHECK(h.open == 0);
    ed.doubleClick(cx, cy, m); CHECK(std::fabs(h.v[pGain] - kParams[pGain].defV) < 1e-6f);
    // load a factory preset through the popup
    ed.openPresetList(); CHECK(ed.popupOpen()); ed.renderAll(); bmp(ed.canvas(), "build/ui_popup.bmp");
    ed.keyDown(27); CHECK(!ed.popupOpen());
    ed.loadEntry(5); CHECK(h.ps.factory == 5); CHECK(!ed.isDirty());
    float gv = h.v[pGain]; h.v[pGain] = 0.123f; CHECK(ed.isDirty()); h.v[pGain] = gv; CHECK(!ed.isDirty());
    ed.stepPreset(1); CHECK(h.ps.factory == 6); ed.stepPreset(-1); CHECK(h.ps.factory == 5);
    // save
    h.v[pGain] = 0.321f;
    pl.savePath = d + "/My Test.daxe"; ed.savePreset();
    CHECK(h.ps.user && h.ps.name == "My Test"); CHECK(ed.users().size() == 1); CHECK(!ed.isDirty());
    // load another, then back to user
    ed.loadEntry(0); CHECK(std::fabs(h.v[pGain] - 0.321f) > 1e-3f);
    ed.loadEntry(numPresets()); CHECK(std::fabs(h.v[pGain] - 0.321f) < 1e-5f);
    ed.renderAll(); bmp(ed.canvas(), "build/ui_user.bmp");
    // popup with user entry then delete
    ed.openPresetList(); ed.renderAll(); bmp(ed.canvas(), "build/ui_popup2.bmp"); ed.keyDown(27);
    pl.conf = false; ed.deleteUser(0); CHECK(ed.users().size() == 1);
    pl.conf = true; ed.deleteUser(0); CHECK(ed.users().empty()); CHECK(h.ps.name == "Custom");
    // load from file
    pl.savePath = d + "/Other.daxe"; ed.savePreset(); pl.loadPath = d + "/Other.daxe"; ed.loadFromFile(); CHECK(h.ps.name == "Other");
    pl.loadPath = d + "/nope.daxe"; ed.loadFromFile();
    // dropdown + toggle + segmented via mouse
    ed.renderAll();
    printf("%s (%d failures)\n", fails ? "UI TEST FAILED" : "UI TEST OK", fails);
    return fails ? 1 : 0;
}
