// DeathAxe UI - platform independent editor: layout, preset bar, popups, event handling and rendering.
// The host side (VST3 controller / test mock) implements Host; the window side implements Platform.
#pragma once
#include "widgets.h"
#include "../presets.h"
#include "../userpresets.h"
#include <algorithm>
#include <cstring>

namespace dg { namespace ui {

struct PresetState {                 // lives as long as the plug-in instance, survives closing the editor
    std::string name, path;  bool user = false, valid = false;  int factory = -1;
    float ref[kNumParams] = {};
};

struct Host {
    virtual ~Host() {}
    virtual float get(int pid) = 0;
    virtual void begin(int pid) = 0;
    virtual void set(int pid, float plain) = 0;
    virtual void end(int pid) = 0;
    virtual PresetState& presetState() = 0;
};

struct Platform {
    virtual ~Platform() {}
    virtual bool saveDialog(const std::string& dir, const std::string& defaultName, std::string& outPath) = 0;
    virtual bool openDialog(const std::string& dir, std::string& outPath) = 0;
    virtual bool confirm(const std::string& text) = 0;
    virtual void message(const std::string& text) = 0;
};

constexpr int kEditorW = 980, kEditorH = 520;

class Editor : public Ctx {
public:
    Editor(Host* h, Platform* p) : host(h), plat(p) {}

    // ------------------------------------------------------------ setup
    void open(float scale) {
        scaleF = scale;
        cv.init((int)std::lround(kEditorW * scale), (int)std::lround(kEditorH * scale), scale);
        bg.init(cv.w, cv.h, scale);
        refreshUser();
        if (!host->presetState().valid) detectPreset();
        build();
        paintBackground();
        invalidateAll();
    }
    const Canvas& canvas() const { return cv; }
    int pixelW() const { return cv.w; }
    int pixelH() const { return cv.h; }

    // ------------------------------------------------------------ rendering
    // renders pending dirty area, returns it in device pixels (empty when nothing to do)
    RectI flush() {
        RectI r = dirty;
        if (r.x1 <= r.x0 || r.y1 <= r.y0) return RectI{0, 0, 0, 0};
        dirty = RectI{cv.w, cv.h, 0, 0};
        cv.copyRect(bg, r);
        cv.clip = r;
        for (auto& w : widgets) {
            const RectI wr = cv.toDevice(w->x - 4, w->y - 4, w->w + 8, w->h + 8);
            if (wr.x1 <= r.x0 || wr.x0 >= r.x1 || wr.y1 <= r.y0 || wr.y0 >= r.y1) continue;
            w->draw(cv, *this);
        }
        if (popup) { popup->draw(cv); }
        cv.resetClip();
        return r;
    }
    void renderAll() { invalidateAll(); flush(); }

    void invalidate(float x, float y, float w, float h) override {
        const RectI r = cv.toDevice(x, y, w, h);
        dirty.x0 = std::min(dirty.x0, r.x0); dirty.y0 = std::min(dirty.y0, r.y0);
        dirty.x1 = std::max(dirty.x1, r.x1); dirty.y1 = std::max(dirty.y1, r.y1);
    }
    void invalidateAll() { dirty = RectI{0, 0, cv.w, cv.h}; }

    // ------------------------------------------------------------ periodic (call ~30 Hz)
    void tick() {
        for (auto& w : widgets) w->poll(*this);
        const bool d = isDirty();
        if (d != lastDirty || host->presetState().name != lastName) { lastDirty = d; lastName = host->presetState().name; presetBox->touch(*this); }
        if (msgTicks > 0 && --msgTicks == 0) { statusMsg.clear(); refreshStatus(); }
    }

    // ------------------------------------------------------------ mouse
    void mouseMove(float mx, float my, Mods m) {
        lastMods = m;
        if (popup) { popupMove(mx, my); return; }
        if (captured) { captured->mouseDrag(*this, mx, my, m); return; }
        Widget* h = widgetAt(mx, my);
        if (h != hovered) {
            if (hovered) { hovered->hover = false; hovered->touch(*this); }
            hovered = h;
            if (hovered) { hovered->hover = true; hovered->touch(*this); }
            hoverHelp = hovered ? hovered->helpText() : "";
            refreshStatus();
        }
    }
    void mouseLeave() {
        if (captured) return;
        if (hovered) { hovered->hover = false; hovered->touch(*this); hovered = nullptr; }
        hoverHelp.clear();  refreshStatus();
    }
    void mouseDown(float mx, float my, Mods m) {
        lastMods = m;
        if (popup) { popupDown(mx, my); return; }
        Widget* w = widgetAt(mx, my);
        if (!w) return;
        captured = w;
        w->mouseDown(*this, mx, my, m);
    }
    void mouseUp(float mx, float my, Mods m) {
        lastMods = m;
        if (popup) return;
        if (captured) { Widget* w = captured; captured = nullptr; w->mouseUp(*this, mx, my, m); mouseMove(mx, my, m); }
    }
    void doubleClick(float mx, float my, Mods m) {
        if (popup) { popupDown(mx, my); return; }
        Widget* w = widgetAt(mx, my);
        if (w) { w->dblClick(*this, mx, my, m); captured = nullptr; }
    }
    void mouseWheel(float mx, float my, float dir, Mods m) {
        if (popup) {
            if (popup->contentH > popup->viewH) {
                popup->scroll = std::max(0.f, std::min(popup->contentH - popup->viewH, popup->scroll - dir * 48.0f));
                popupMove(mx, my);  invalidateAll();
            }
            return;
        }
        Widget* w = widgetAt(mx, my);
        if (w) w->wheel(*this, mx, my, dir, m);
    }
    void keyDown(int key) {   // 27 = escape
        if (key == 27 && popup) closePopup();
    }
    bool popupOpen() const { return popup != nullptr; }

    // ------------------------------------------------------------ Ctx
    float get(int pid) override { return host->get(pid); }
    void beginEdit(int pid) override { host->begin(pid); }
    void set(int pid, float v) override { host->set(pid, v); }
    void endEdit(int pid) override { host->end(pid); }
    bool disabled(int pid) override {
        if (pid >= pBoostDrive && pid <= pBoostLevel) return host->get(pBoostOn) < 0.5f;
        if (pid == pGlide) return host->get(pVoiceMode) < 0.5f;
        if (pid == pStrum) return host->get(pChord) < 0.5f && host->get(pSub) < 0.001f && false;
        return false;
    }
    void help(const std::string& s) override { hoverHelp = s; refreshStatus(); }
    void openPopup(float ax, float ay, float aw, float ah, float width, std::vector<Item> items,
                   std::function<void(int)> onSelect, std::function<void(int)> onDelete,
                   const std::string& footer, std::function<void()> onFooter) override {
        popup.reset(new Popup());
        popup->items = std::move(items);  popup->footer = footer;
        popup->onSelect = std::move(onSelect);  popup->onDelete = std::move(onDelete);  popup->onFooter = std::move(onFooter);
        popup->layout((float)kEditorW, (float)kEditorH, ax, ay, aw, ah, width);
        if (hovered) { hovered->hover = false; hovered = nullptr; }
        invalidateAll();
    }

    // ------------------------------------------------------------ presets (also used by tests)
    struct Entry { bool user; int idx; };
    std::vector<Entry> entries() const {
        std::vector<Entry> e;
        for (int i = 0; i < numPresets(); ++i) e.push_back({false, i});
        for (int i = 0; i < (int)userList.size(); ++i) e.push_back({true, i});
        return e;
    }
    int currentEntry() const {
        const PresetState& s = host->presetState();
        if (!s.valid) return -1;
        if (!s.user && s.factory >= 0) return s.factory;
        if (s.user) for (int i = 0; i < (int)userList.size(); ++i) if (userList[i].path == s.path) return numPresets() + i;
        return -1;
    }
    void loadEntry(int i) {
        if (i < 0) return;
        if (i < numPresets()) {
            float v[kNumParams];  presetValues(i, v);
            applyValues(v, presetName(i), false, "", i);
            setStatusMsg(std::string(presetName(i)) + " - " + presetDesc(i));
        } else if (i - numPresets() < (int)userList.size()) {
            const UserPreset& u = userList[i - numPresets()];
            applyValues(u.v, u.name, true, u.path, -1);
            setStatusMsg("Loaded " + u.name);
        }
    }
    void stepPreset(int dir) {
        const int n = numPresets() + (int)userList.size();
        int c = currentEntry();
        if (c < 0) c = dir > 0 ? -1 : n;
        loadEntry(std::max(0, std::min(n - 1, c + dir)));
    }
    void savePreset() {
        PresetState& s = host->presetState();
        std::string def = s.valid && s.name != "Custom" ? s.name : "My Preset";
        if (s.valid && !s.user) def = s.name + " (mine)";
        def = sanitizeFileStem(def);
        std::string out;
        if (!plat->saveDialog(userPresetDir().u8string(), def, out) || out.empty()) return;
        std::filesystem::path p = std::filesystem::u8path(out);
        if (p.extension() != ".daxe") p = std::filesystem::u8path(out + ".daxe");
        float v[kNumParams];  for (int i = 0; i < kNumParams; ++i) v[i] = host->get(i);
        if (!writePresetFile(p, v)) { plat->message("Could not write the preset file:\n" + p.u8string()); return; }
        refreshUser();
        std::memcpy(s.ref, v, sizeof v);
        s.valid = true;  s.user = true;  s.factory = -1;  s.path = p.u8string();  s.name = p.stem().u8string();
        // the file may live outside the library folder; then it is simply "current" without being listed
        setStatusMsg("Saved \"" + s.name + "\" to " + p.parent_path().u8string());
        presetBox->touch(*this);
    }
    void loadFromFile() {
        std::string out;
        if (!plat->openDialog(userPresetDir().u8string(), out) || out.empty()) return;
        float v[kNumParams];
        const std::filesystem::path p = std::filesystem::u8path(out);
        if (!parsePresetFile(p, v)) { plat->message("That file is not a valid DeathAxe preset:\n" + out); return; }
        applyValues(v, p.stem().u8string(), true, p.u8string(), -1);
        setStatusMsg("Loaded " + p.stem().u8string());
    }
    void deleteUser(int ui) {
        if (ui < 0 || ui >= (int)userList.size()) return;
        const UserPreset u = userList[ui];
        if (!plat->confirm("Delete the preset \"" + u.name + "\"?\nThe file will be removed from disk.")) return;
        std::error_code ec;  std::filesystem::remove(std::filesystem::u8path(u.path), ec);
        if (ec) { plat->message("Could not delete:\n" + u.path); return; }
        PresetState& s = host->presetState();
        if (s.user && s.path == u.path) s.user = false, s.path.clear(), s.name = "Custom", s.factory = -1;
        refreshUser();
        setStatusMsg("Deleted " + u.name);
    }
    bool isDirty() {
        const PresetState& s = host->presetState();
        if (!s.valid) return false;
        for (int i = 0; i < kNumParams; ++i) if (std::fabs(host->get(i) - s.ref[i]) > 1e-4f * std::max(1.0f, std::fabs(s.ref[i]))) return true;
        return false;
    }
    void refreshUser() { userList = listUserPresets(); }
    const std::vector<UserPreset>& users() const { return userList; }
    const std::string& status() const { return statusText; }
    Widget* findKnob(int pid) { for (auto& w : widgets) if (auto* k = dynamic_cast<Knob*>(w.get())) if (k->pid == pid) return k; return nullptr; }
    Widget* widgetAtPublic(float x, float y) { return widgetAt(x, y); }
    PresetState& pstate() { return host->presetState(); }

    void openPresetList() { presetBox->mouseDown(*this, presetBox->x + 40, presetBox->y + 5, Mods{}); }

private:
    Host* host;  Platform* plat;
    float scaleF = 1;
    Canvas cv, bg;
    RectI dirty{0, 0, 0, 0};
    std::vector<std::unique_ptr<Widget>> widgets;
    Widget *hovered = nullptr, *captured = nullptr;
    std::unique_ptr<Popup> popup;
    std::vector<UserPreset> userList;
    std::string hoverHelp, statusMsg, statusText, lastName;  int msgTicks = 0;  bool lastDirty = false;  Mods lastMods;
    struct PresetBoxW;  Widget* presetBox = nullptr;  struct StatusW;  StatusW* statusW = nullptr;

    Widget* widgetAt(float mx, float my) {
        for (auto it = widgets.rbegin(); it != widgets.rend(); ++it) if ((*it)->hit(mx, my)) return it->get();
        return nullptr;
    }
    void setStatusMsg(const std::string& s) { statusMsg = s; msgTicks = 150; refreshStatus(); }
    void refreshStatus();
    void closePopup() { popup.reset(); invalidateAll(); mouseMove(-1, -1, lastMods); }

    void applyValues(const float* v, const std::string& name, bool user, const std::string& path, int factory) {
        for (int i = 0; i < kNumParams; ++i) {
            if (std::fabs(host->get(i) - v[i]) < 1e-7f) continue;
            host->begin(i);  host->set(i, v[i]);  host->end(i);
        }
        PresetState& s = host->presetState();
        std::memcpy(s.ref, v, sizeof(float) * kNumParams);
        s.valid = true;  s.name = name;  s.user = user;  s.path = path;  s.factory = factory;
        invalidateAll();
    }
    void detectPreset() {
        PresetState& s = host->presetState();
        float cur[kNumParams];  for (int i = 0; i < kNumParams; ++i) cur[i] = host->get(i);
        auto same = [&](const float* a) { for (int i = 0; i < kNumParams; ++i) if (std::fabs(a[i] - cur[i]) > 1e-3f * std::max(1.0f, std::fabs(a[i]))) return false; return true; };
        std::memcpy(s.ref, cur, sizeof cur);
        s.valid = true;  s.name = "Custom";  s.user = false;  s.factory = -1;  s.path.clear();
        for (int i = 0; i < numPresets(); ++i) { float v[kNumParams]; presetValues(i, v); if (same(v)) { s.name = presetName(i); s.factory = i; return; } }
        for (auto& u : userList) if (same(u.v)) { s.name = u.name; s.user = true; s.path = u.path; return; }
    }

    void popupMove(float mx, float my) {
        Popup& p = *popup;
        const int hi = p.indexAt(mx, my);
        bool hf = p.footH > 0 && mx >= p.x && mx < p.x + p.w && my >= p.y + p.h - p.footH && my < p.y + p.h;
        bool hd = hi >= 0 && p.items[hi].deletable && mx > p.x + p.w - 42 && mx < p.x + p.w - 14;
        if (hi != p.hoverIdx || hf != p.hoverFooter || hd != p.hoverDelete) { p.hoverIdx = hi; p.hoverFooter = hf; p.hoverDelete = hd; invalidate(p.x - 6, p.y - 6, p.w + 12, p.h + 14); }
    }
    void popupDown(float mx, float my) {
        Popup& p = *popup;
        if (!p.inside(mx, my)) { closePopup(); return; }
        if (p.footH > 0 && my >= p.y + p.h - p.footH) { auto f = p.onFooter; closePopup(); if (f) f(); return; }
        const int i = p.indexAt(mx, my);
        if (i < 0) return;
        const bool del = p.items[i].deletable && mx > p.x + p.w - 42 && mx < p.x + p.w - 14;
        auto sel = p.onSelect;  auto dl = p.onDelete;
        closePopup();
        if (del) { if (dl) dl(i); } else if (sel) sel(i);
    }

    void build();
    void paintBackground();

    struct Panel { std::string title; float x, y, w, h; };
    std::vector<Panel> panels;
};

// ---------------------------------------------------------------- composite widgets that need Editor
struct Editor::PresetBoxW : Widget {
    Editor* ed;
    explicit PresetBoxW(Editor* e) : ed(e) {}
    std::string helpText() override { return "Click to browse all presets. Use the arrows to step through them."; }
    void draw(Canvas& cv, Ctx&) override {
        using namespace theme;
        dirty = false;
        const PresetState& s = ed->pstate();
        const bool d = ed->isDirty();
        cv.fillRoundRectV(x, y, w, h, 6, rgb(0x0c0c0e), rgb(0x151518));
        cv.strokeRoundRect(x, y, w, h, 6, 1, hover ? rgb(0x66666e) : fieldEdge);
        std::string name = s.valid ? s.name : "Custom";
        const float tag = s.user ? 62.f : 0.f;
        float right = x + w - 14;
        // dropdown chevron
        const float cy = y + h * 0.5f;
        const float tri[6] = {right - 5, cy - 2.5f, right + 5, cy - 2.5f, right, cy + 3.5f};
        cv.fillConvex(tri, 3, hover ? text : dim);
        right -= 18;
        if (s.user) { cv.text(kBold, 8.5f, right, cy + 3.2f, "MY PRESET", rgb(0xd9a064), kRight, 0.8f); right -= 74; }
        (void)tag;
        const std::string shown = cv.fit(kBold, 14.0f, name, right - (x + 16) - (d ? 90 : 0), 0.3f);
        const float tw = cv.text(kBold, 14.0f, x + 16, cy + 5.0f, shown, text, kLeft, 0.3f);
        if (d) cv.text(kRegular, 10.0f, x + 16 + tw + 10, cy + 4.0f, "(modified)", rgb(0xe0883a), kLeft);
    }
    void mouseDown(Ctx&, float, float, Mods) override {
        std::vector<Ctx::Item> items;
        std::vector<Entry> ent = ed->entries();
        const int cur = ed->currentEntry();
        Ctx::Item h1;  h1.label = "FACTORY PRESETS";  h1.header = true;  items.push_back(h1);
        std::vector<int> map;   map.push_back(-1);
        bool userHeader = false;
        for (size_t k = 0; k < ent.size(); ++k) {
            if (ent[k].user && !userHeader) {
                Ctx::Item h2;  h2.label = "MY PRESETS";  h2.header = true;  items.push_back(h2);  map.push_back(-1);  userHeader = true;
            }
            Ctx::Item it;
            it.label = ent[k].user ? ed->users()[ent[k].idx].name : presetName(ent[k].idx);
            it.selected = ((int)k == cur);  it.deletable = ent[k].user;
            items.push_back(it);  map.push_back((int)k);
        }
        if (!userHeader) {
            Ctx::Item h2;  h2.label = "MY PRESETS";  h2.header = true;  items.push_back(h2);  map.push_back(-1);
            Ctx::Item e;  e.label = "(none yet - press SAVE to add yours)";  e.header = true;  items.push_back(e);  map.push_back(-1);
        }
        Editor* e = ed;
        ed->openPopup(x, y, w, h, std::max(w, 300.0f), items,
                      [e, map](int i) { e->loadEntry(map[i]); },
                      [e, map](int i) { const int k = map[i]; if (k >= numPresets()) e->deleteUser(k - numPresets()); },
                      "Load preset from file...", [e]() { e->loadFromFile(); });
    }
};

struct Editor::StatusW : Widget {
    std::string text;
    void draw(Canvas& cv, Ctx&) override {
        dirty = false;
        cv.text(kRegular, 10.5f, x, y + h * 0.5f + 3.8f, cv.fit(kRegular, 10.5f, text, w), theme::dim, kLeft);
    }
};

inline void Editor::refreshStatus() {
    std::string t = !statusMsg.empty() ? statusMsg : (!hoverHelp.empty() ? hoverHelp :
        "Drag a knob up or down to change it.  Shift = fine.  Double-click = reset.  Mouse wheel works too.");
    if (!statusMsg.empty() && !hoverHelp.empty() && hovered) t = hoverHelp;     // hovering a control shows its help first
    if (t != statusW->text) { statusW->text = t; statusW->touch(*this); }
    statusText = t;
}

inline void Editor::build() {
    using namespace theme;
    widgets.clear();  panels.clear();  hovered = captured = nullptr;  popup.reset();
    const float M = 14, G = 10, topH = 66, panH = 128, statusH = 30;
    float y = topH;

    // ---- top bar
    {
        auto prev = new Button();  prev->label = "<";  prev->help = "Previous preset";
        auto next = new Button();  next->label = ">";  next->help = "Next preset";
        auto pb = new PresetBoxW(this);
        auto save = new Button();  save->label = "SAVE";  save->help = "Save the current sound as your own preset (opens a Save dialog). Choose the same name again to overwrite.";
        auto load = new Button();  load->label = "LOAD";  load->help = "Load a preset file (.daxe) from anywhere on your computer.";
        const float by = 16, bh = 34;
        prev->x = 340; prev->y = by; prev->w = 36; prev->h = bh;
        pb->x = 382; pb->y = by; pb->w = 330; pb->h = bh;
        next->x = 718; next->y = by; next->w = 36; next->h = bh;
        save->x = 772; save->y = by; save->w = 92; save->h = bh;
        load->x = 870; load->y = by; load->w = 92; load->h = bh;
        Editor* e = this;
        prev->onClick = [e]() { e->stepPreset(-1); };
        next->onClick = [e]() { e->stepPreset(+1); };
        save->onClick = [e]() { e->savePreset(); };
        load->onClick = [e]() { e->loadFromFile(); };
        save->accentFn = [e]() { return e->isDirty(); };
        presetBox = pb;
        for (Widget* w : std::initializer_list<Widget*>{prev, pb, next, save, load}) widgets.emplace_back(w);
    }

    struct Spec { std::string title; std::vector<int> knobs; int headerSel = -1; int headerToggle = -1; int headerSeg = -1; };
    auto row = [&](std::vector<Spec> specs) {
        int cells = 0;  for (auto& s : specs) cells += (int)s.knobs.size();
        const float avail = kEditorW - 2 * M - G * (specs.size() - 1);
        const float cell = (avail - 16.0f * specs.size()) / cells;
        float x = M;
        for (auto& s : specs) {
            const float pw = cell * s.knobs.size() + 16;
            panels.push_back({s.title, x, y, pw, panH});
            // header controls (right aligned)
            float hx = x + pw - 10;
            if (s.headerToggle >= 0) { auto t = new Toggle(s.headerToggle); t->w = 38; t->h = 18; t->x = hx - t->w; t->y = y + 6; hx = t->x - 10; widgets.emplace_back(t); }
            if (s.headerSel >= 0) {
                auto d = new Dropdown(s.headerSel); d->w = std::min(190.0f, pw - 110); d->h = 22; d->x = hx - d->w; d->y = y + 4; hx = d->x - 8; widgets.emplace_back(d);
            }
            if (s.headerSeg >= 0) {
                auto g = new Segmented(s.headerSeg, {"POLY", "MONO"}); g->w = 104; g->h = 22; g->x = hx - g->w; g->y = y + 4; hx = g->x - 8; widgets.emplace_back(g);
            }
            float kx = x + 8;
            for (int pid : s.knobs) { auto k = new Knob(pid); k->x = kx; k->y = y + 32; k->w = cell; k->h = 90; kx += cell; widgets.emplace_back(k); }
            x += pw + G;
        }
        y += panH + G;
    };
    row({{"PLAYING", {pTranspose, pBendRange, pGlide, pStrum, pSub, pWidth}, pChord, -1, pVoiceMode},
         {"STRING", {pPickAttack, pPickPos, pPickNoise, pSustain, pDamping, pPalmMute, pTremRate, pPinch}}});
    row({{"GATE + BOOST", {pGateThresh, pGateRelease, pBoostDrive, pBoostTight, pBoostLevel}, -1, pBoostOn},
         {"AMP", {pGain, pBright, pBias, pSag, pBass, pMid, pMidFreq, pTreble, pPresence, pResonance}, pVoicing}});
    row({{"CABINET", {pMicPos, pMicDist, pLowCut, pHighCut}, pCabType},
         {"DELAY", {pDelayMix, pDelayTime, pDelayFb}},
         {"REVERB", {pRevMix, pRevSize, pRevDamp}},
         {"OUTPUT", {pOutput}}});
    // the PLAYING panel title is short, so give the CAB panel's dropdown its room too (already clamped above)

    auto st = new StatusW();  st->x = M + 4; st->y = kEditorH - statusH; st->w = kEditorW - 2 * M - 8; st->h = statusH;
    statusW = st;  widgets.emplace_back(st);
    refreshStatus();
    lastName = host->presetState().name;  lastDirty = isDirty();
}

inline void Editor::paintBackground() {
    using namespace theme;
    bg.clear(theme::bg);
    // subtle top glow
    for (int yy = 0; yy < bg.h; ++yy) {
        const float t = std::max(0.0f, 1.0f - yy / (150.0f * scaleF));
        if (t <= 0) break;
        const Col c = withAlpha(redDark, 0.28f * t * t);
        for (int xx = 0; xx < bg.w; ++xx) bg.blend(xx, yy, c, 1.0f);
    }
    bg.fillRect(0, 0, (float)kEditorW, 62, bar);
    bg.hline(0, 62, (float)kEditorW, 1, rgb(0x2a2a2e));
    bg.hline(0, 63, (float)kEditorW, 1, rgb(0x050506));
    // logo
    bg.text(kBold, 27.0f, 18, 40, "DEATH", rgb(0xeee8dc), kLeft, 2.0f);
    const float dw = bg.measure(kBold, 27.0f, "DEATH", 2.0f);
    bg.text(kBold, 27.0f, 18 + dw + 3, 40, "AXE", rgb(0xe8212e), kLeft, 2.0f);
    bg.text(kRegular, 9.0f, 20, 55, "DEATH METAL GUITAR INSTRUMENT", faint, kLeft, 1.8f);
    bg.text(kBold, 8.5f, 340, 12, "PRESET", faint, kLeft, 1.6f);
    for (auto& p : panels) {
        bg.fillRoundRect(p.x, p.y + 2, p.w, p.h, 8, withAlpha(rgb(0x000000), 0.35f));
        bg.fillRoundRectV(p.x, p.y, p.w, p.h, 8, panelTop, panelBot);
        bg.strokeRoundRect(p.x, p.y, p.w, p.h, 8, 1, panelEdge);
        bg.hline(p.x + 8, p.y + 1.5f, p.w - 16, 1, withAlpha(panelHi, 0.5f));
        bg.fillRoundRect(p.x + 10, p.y + 10, 3, 12, 1.5f, red);
        bg.text(kBold, 10.5f, p.x + 19, p.y + 20, p.title, rgb(0xd8d3c8), kLeft, 1.6f);
    }
    // status bar
    bg.hline(14.0f, kEditorH - 31.0f, kEditorW - 2 * 14.0f, 1, rgb(0x232327));
}

}} // namespace dg::ui
