// DeathAxe UI - widgets: knob, dropdown selector, segmented switch, toggle, button, popup menu.
#pragma once
#include "gfx.h"
#include "ui_meta.h"
#include <functional>
#include <memory>

namespace dg { namespace ui {

namespace theme {
constexpr Col bg = rgb(0x0b0b0d), bar = rgb(0x131315), panelTop = rgb(0x1d1d20), panelBot = rgb(0x151517), panelEdge = rgb(0x2b2b30), panelHi = rgb(0x38383e);
constexpr Col text = rgb(0xe9e4d9), dim = rgb(0x8f8c86), faint = rgb(0x5d5b57);
constexpr Col red = rgb(0xd4202c), ember = rgb(0xff7a1f), redDark = rgb(0x7d1219);
constexpr Col track = rgb(0x2a2a2f), knobTop = rgb(0x4b4b52), knobBot = rgb(0x1b1b1e), rim = rgb(0x5c5c64);
constexpr Col field = rgb(0x0f0f11), fieldEdge = rgb(0x303036), hover = rgb(0x2b2b31);
}

struct Mods { bool shift = false, ctrl = false; };

// services the widgets need from the editor
struct Ctx {
    virtual ~Ctx() {}
    virtual float get(int pid) = 0;                       // plain value
    virtual void beginEdit(int pid) = 0;
    virtual void set(int pid, float plain) = 0;
    virtual void endEdit(int pid) = 0;
    virtual bool disabled(int pid) = 0;
    virtual void invalidate(float x, float y, float w, float h) = 0;
    virtual void help(const std::string& s) = 0;
    struct Item { std::string label; bool header = false; bool selected = false; bool deletable = false; };
    virtual void openPopup(float ax, float ay, float aw, float ah, float width, std::vector<Item> items,
                           std::function<void(int)> onSelect, std::function<void(int)> onDelete,
                           const std::string& footer, std::function<void()> onFooter) = 0;
};

struct Widget {
    float x = 0, y = 0, w = 0, h = 0;
    bool dirty = true, hover = false;
    virtual ~Widget() {}
    bool hit(float mx, float my) const { return mx >= x && mx < x + w && my >= y && my < y + h; }
    void touch(Ctx& c) { dirty = true; c.invalidate(x - 4, y - 4, w + 8, h + 8); }
    virtual void draw(Canvas& cv, Ctx& c) = 0;
    virtual void mouseDown(Ctx&, float, float, Mods) {}
    virtual void mouseDrag(Ctx&, float, float, Mods) {}
    virtual void mouseUp(Ctx&, float, float, Mods) {}
    virtual void dblClick(Ctx&, float, float, Mods) {}
    virtual void wheel(Ctx&, float, float, float, Mods) {}
    virtual void poll(Ctx&) {}
    virtual std::string helpText() { return ""; }
};

// ---------------------------------------------------------------- knob
struct Knob : Widget {
    int pid;  float last = -1e9f, dragStartNorm = 0, dragStartY = 0;  bool dragging = false;
    explicit Knob(int p) : pid(p) {}
    static constexpr float kA0 = 2.35619449f, kSweep = 4.71238898f;   // 135 deg, 270 deg sweep

    void poll(Ctx& c) override { const float v = c.get(pid); if (v != last) touch(c); }
    std::string helpText() override { return uiHelp(pid); }

    void draw(Canvas& cv, Ctx& c) override {
        using namespace theme;
        dirty = false;
        const float v = c.get(pid);  last = v;
        const bool off = c.disabled(pid);
        const float a = off ? 0.38f : 1.0f;
        const double norm = toNorm(pid, v);
        const float cx = x + w * 0.5f, cy = y + 27.0f;
        const ParamDef& d = kParams[pid];
        const bool bipolar = d.minV < 0 && d.maxV > 0 && (d.type == T_LIN || d.type == T_INT);
        const float zero = bipolar ? (float)toNorm(pid, 0.0f) : 0.0f;
        // soft shadow + track
        cv.fillCircle(cx, cy + 2.5f, 21.0f, withAlpha(rgb(0x000000), 0.45f * a));
        cv.strokeArc(cx, cy, 24.5f, 3.6f, kA0, kA0 + kSweep, withAlpha(track, a));
        // value arc
        const float n0 = bipolar ? std::min(zero, (float)norm) : 0.0f, n1 = bipolar ? std::max(zero, (float)norm) : (float)norm;
        if (n1 - n0 > 0.002f)
            cv.strokeArc(cx, cy, 24.5f, 3.6f, kA0 + kSweep * n0, kA0 + kSweep * n1, withAlpha(red, a), withAlpha(ember, a), kA0, kSweep);
        else if (bipolar) cv.fillCircle(cx + 24.5f * std::cos(kA0 + kSweep * zero), cy + 24.5f * std::sin(kA0 + kSweep * zero), 1.8f, withAlpha(dim, a));
        // body
        const bool lit = (hover || dragging) && !off;
        cv.fillCircleV(cx, cy, 18.5f, lit ? rgb(0x5a5a62) : knobTop, knobBot);
        cv.strokeCircle(cx, cy, 18.2f, 1.0f, withAlpha(lit ? rgb(0x7a7a84) : rim, a));
        cv.fillCircleV(cx, cy, 14.0f, rgb(0x232326), rgb(0x2f2f34));
        const float ang = kA0 + kSweep * (float)norm;
        cv.line(cx + 5.0f * std::cos(ang), cy + 5.0f * std::sin(ang), cx + 14.5f * std::cos(ang), cy + 14.5f * std::sin(ang), 2.6f, withAlpha(lit ? rgb(0xffffff) : text, a));
        // labels
        const float tw = w - 4;
        cv.text(kBold, 9.5f, cx, y + 66.0f, cv.fit(kBold, 9.5f, uiName(pid), tw, 0.4f), withAlpha(lit ? text : rgb(0xc9c4b9), a), kCenter, 0.4f);
        cv.text(kRegular, 10.0f, cx, y + 80.0f, cv.fit(kRegular, 10.0f, formatValue(pid, v), tw), withAlpha(dragging ? ember : rgb(0xd9a064), a), kCenter);
    }

    void mouseDown(Ctx& c, float, float my, Mods m) override {
        if (m.ctrl) { c.beginEdit(pid); c.set(pid, kParams[pid].defV); c.endEdit(pid); touch(c); return; }
        dragging = true;  dragStartY = my;  dragStartNorm = (float)toNorm(pid, c.get(pid));
        c.beginEdit(pid);  touch(c);
    }
    void mouseDrag(Ctx& c, float, float my, Mods m) override {
        if (!dragging) return;
        const int steps = stepCount(kParams[pid]);
        const float per = (m.shift ? 900.0f : 180.0f) * (steps > 0 && steps < 20 ? 0.45f : 1.0f);   // stepped params need less travel
        float n = dragStartNorm + (dragStartY - my) / per;
        n = std::max(0.f, std::min(1.f, n));
        const float p = toPlain(pid, n);
        if (p != c.get(pid)) { c.set(pid, p); touch(c); }
    }
    void mouseUp(Ctx& c, float, float, Mods) override { if (dragging) { dragging = false; c.endEdit(pid); touch(c); } }
    void dblClick(Ctx& c, float, float, Mods) override {
        dragging = false;  c.beginEdit(pid);  c.set(pid, kParams[pid].defV);  c.endEdit(pid);  touch(c);
    }
    void wheel(Ctx& c, float, float, float dir, Mods m) override {
        const int steps = stepCount(kParams[pid]);
        double n = toNorm(pid, c.get(pid));
        n += dir * (steps > 0 ? 1.0 / steps : (m.shift ? 0.005 : 0.03));
        c.beginEdit(pid);  c.set(pid, toPlain(pid, std::max(0.0, std::min(1.0, n))));  c.endEdit(pid);  touch(c);
    }
};

// ---------------------------------------------------------------- dropdown with prev/next arrows
struct Dropdown : Widget {
    int pid;  std::string last;
    explicit Dropdown(int p) : pid(p) {}
    std::string helpText() override { return uiHelp(pid); }
    void poll(Ctx& c) override { if (formatValue(pid, c.get(pid)) != last) touch(c); }
    void draw(Canvas& cv, Ctx& c) override {
        using namespace theme;
        dirty = false;
        last = formatValue(pid, c.get(pid));
        cv.fillRoundRectV(x, y, w, h, 5, rgb(0x0e0e10), rgb(0x161619));
        cv.strokeRoundRect(x, y, w, h, 5, 1, hover ? rgb(0x55555d) : fieldEdge);
        const float ay = y + h * 0.5f;
        const Col ac = hover ? text : dim;
        const float l[6] = {x + 15, ay, x + 21, ay - 4.5f, x + 21, ay + 4.5f};
        const float r[6] = {x + w - 15, ay, x + w - 21, ay - 4.5f, x + w - 21, ay + 4.5f};
        cv.fillConvex(l, 3, ac);  cv.fillConvex(r, 3, ac);
        cv.text(kBold, 11.0f, x + w * 0.5f, ay + 4.0f, cv.fit(kBold, 11.0f, last, w - 60, 0.3f), text, kCenter, 0.3f);
    }
    void step(Ctx& c, int dir) {
        const ParamDef& d = kParams[pid];
        const int cur = (int)(c.get(pid) - d.minV + 0.5f), n = enumCount(d);
        const int nx = std::max(0, std::min(n - 1, cur + dir));
        if (nx != cur) { c.beginEdit(pid); c.set(pid, d.minV + nx); c.endEdit(pid); }
        touch(c);
    }
    void mouseDown(Ctx& c, float mx, float, Mods) override {
        if (mx < x + 30) { step(c, -1); return; }
        if (mx > x + w - 30) { step(c, +1); return; }
        const ParamDef& d = kParams[pid];
        std::vector<Ctx::Item> items;
        const int cur = (int)(c.get(pid) - d.minV + 0.5f), n = enumCount(d);
        for (int i = 0; i < n; ++i) { char t[64]; enumChoice(d, i, t, 64); Ctx::Item it; it.label = t; it.selected = (i == cur); items.push_back(it); }
        const int p = pid;  Ctx* cc = &c;
        c.openPopup(x, y, w, h, std::max(w, 170.0f), items, [p, cc](int i) { cc->beginEdit(p); cc->set(p, kParams[p].minV + i); cc->endEdit(p); }, nullptr, "", nullptr);
    }
    void wheel(Ctx& c, float, float, float dir, Mods) override { step(c, dir > 0 ? 1 : -1); }
};

// ---------------------------------------------------------------- two/three way segmented switch (enum)
struct Segmented : Widget {
    int pid;  std::vector<std::string> labels;  int last = -1;
    Segmented(int p, std::vector<std::string> l) : pid(p), labels(std::move(l)) {}
    std::string helpText() override { return uiHelp(pid); }
    void poll(Ctx& c) override { if ((int)(c.get(pid) - kParams[pid].minV + 0.5f) != last) touch(c); }
    void draw(Canvas& cv, Ctx& c) override {
        using namespace theme;
        dirty = false;
        const int cur = (int)(c.get(pid) - kParams[pid].minV + 0.5f);  last = cur;
        cv.fillRoundRectV(x, y, w, h, 5, rgb(0x0e0e10), rgb(0x161619));
        const float sw = w / labels.size();
        for (size_t i = 0; i < labels.size(); ++i) {
            const float sx = x + sw * i;
            if ((int)i == cur) { cv.fillRoundRectV(sx + 2, y + 2, sw - 4, h - 4, 4, rgb(0xe02a36), redDark); }
            cv.text(kBold, 10.0f, sx + sw * 0.5f, y + h * 0.5f + 3.6f, labels[i], (int)i == cur ? rgb(0xffffff) : (hover ? text : dim), kCenter, 0.8f);
        }
        cv.strokeRoundRect(x, y, w, h, 5, 1, fieldEdge);
    }
    void mouseDown(Ctx& c, float mx, float, Mods) override {
        const int i = std::max(0, std::min((int)labels.size() - 1, (int)((mx - x) / (w / labels.size()))));
        c.beginEdit(pid);  c.set(pid, kParams[pid].minV + i);  c.endEdit(pid);  touch(c);
    }
};

// ---------------------------------------------------------------- on/off switch (panel header)
struct Toggle : Widget {
    int pid;  int last = -1;
    explicit Toggle(int p) : pid(p) {}
    std::string helpText() override { return uiHelp(pid); }
    void poll(Ctx& c) override { if ((int)(c.get(pid) > 0.5f) != last) touch(c); }
    void draw(Canvas& cv, Ctx& c) override {
        using namespace theme;
        dirty = false;
        const bool on = c.get(pid) > 0.5f;  last = on;
        cv.fillRoundRectV(x, y, w, h, h * 0.5f, on ? redDark : rgb(0x0e0e10), on ? rgb(0xa51923) : rgb(0x1a1a1d));
        cv.strokeRoundRect(x, y, w, h, h * 0.5f, 1, hover ? rgb(0x66666e) : (on ? rgb(0xe02a36) : fieldEdge));
        const float kx = on ? x + w - h * 0.5f : x + h * 0.5f;
        cv.fillCircleV(kx, y + h * 0.5f, h * 0.5f - 2.5f, on ? rgb(0xffe2c4) : rgb(0x77777f), on ? rgb(0xff9a4d) : rgb(0x4a4a50));
    }
    void mouseDown(Ctx& c, float, float, Mods) override {
        c.beginEdit(pid);  c.set(pid, c.get(pid) > 0.5f ? 0.0f : 1.0f);  c.endEdit(pid);  touch(c);
    }
};

// ---------------------------------------------------------------- push button
struct Button : Widget {
    std::string label, help;  std::function<void()> onClick;  bool pressed = false;  bool accent = false;
    std::function<bool()> accentFn;
    void draw(Canvas& cv, Ctx&) override {
        using namespace theme;
        dirty = false;
        const bool hot = accentFn ? accentFn() : accent;
        cv.fillRoundRectV(x, y + (pressed ? 1 : 0), w, h, 6, pressed ? rgb(0x222226) : (hover ? rgb(0x38383f) : rgb(0x2b2b31)), pressed ? rgb(0x1a1a1d) : rgb(0x1e1e22));
        cv.strokeRoundRect(x, y + (pressed ? 1 : 0), w, h, 6, 1, hot ? red : (hover ? rgb(0x66666e) : rgb(0x3d3d44)));
        cv.text(kBold, 10.5f, x + w * 0.5f, y + h * 0.5f + 3.8f + (pressed ? 1 : 0), label, hot ? rgb(0xffb08a) : text, kCenter, 1.2f);
    }
    std::string helpText() override { return help; }
    void mouseDown(Ctx& c, float, float, Mods) override { pressed = true; touch(c); }
    void mouseUp(Ctx& c, float mx, float my, Mods) override { const bool fire = pressed && hit(mx, my); pressed = false; touch(c); if (fire && onClick) onClick(); }
};

// ---------------------------------------------------------------- popup menu (dropdown list / preset browser)
struct Popup {
    float x = 0, y = 0, w = 0, h = 0, scroll = 0, contentH = 0, viewH = 0, rowH = 24, headH = 22, footH = 0;
    std::vector<Ctx::Item> items;  std::string footer;
    std::function<void(int)> onSelect, onDelete;  std::function<void()> onFooter;
    int hoverIdx = -1;  bool hoverFooter = false, hoverDelete = false;

    float itemY(int i) const { float yy = 0; for (int k = 0; k < i; ++k) yy += items[k].header ? headH : rowH; return yy; }
    float itemH(int i) const { return items[i].header ? headH : rowH; }
    void layout(float winW, float winH, float ax, float ay, float aw, float ah, float width) {
        contentH = itemY((int)items.size());
        footH = footer.empty() ? 0.f : 28.f;
        const float maxH = std::min(winH - 80.0f, 470.0f);
        viewH = std::min(contentH, maxH - footH - 8);
        w = width;  h = viewH + footH + 8;
        x = std::max(8.0f, std::min(ax + (aw - w) * 0.5f, winW - w - 8));
        y = ay + ah + 4;
        if (y + h > winH - 8) y = std::max(8.0f, ay - h - 4);
        scroll = 0;
        for (int i = 0; i < (int)items.size(); ++i) if (items[i].selected) { scroll = std::max(0.f, std::min(contentH - viewH, itemY(i) - viewH * 0.4f)); break; }
    }
    bool inside(float mx, float my) const { return mx >= x && mx < x + w && my >= y && my < y + h; }
    int indexAt(float mx, float my) const {
        if (mx < x || mx >= x + w - 8 || my < y + 4 || my >= y + 4 + viewH) return -1;
        const float yy = my - (y + 4) + scroll;
        float acc = 0;
        for (int i = 0; i < (int)items.size(); ++i) { acc += itemH(i); if (yy < acc) return items[i].header ? -1 : i; }
        return -1;
    }
    void draw(Canvas& cv) {
        using namespace theme;
        cv.fillRoundRect(x + 1, y + 5, w, h, 8, withAlpha(rgb(0x000000), 0.35f));
        cv.fillRoundRect(x - 2, y - 1, w + 4, h + 4, 10, withAlpha(rgb(0x000000), 0.25f));
        cv.fillRoundRectV(x, y, w, h, 7, rgb(0x232327), rgb(0x1a1a1d));
        cv.strokeRoundRect(x, y, w, h, 7, 1, rgb(0x45454d));
        const RectI saved = cv.clip;
        cv.setClipLogical(x + 1, y + 4, w - 2, viewH);
        float yy = y + 4 - scroll;
        for (int i = 0; i < (int)items.size(); ++i) {
            const Ctx::Item& it = items[i];
            const float ih = itemH(i);
            if (yy + ih > y && yy < y + h) {
                if (it.header) {
                    cv.text(kBold, 9.0f, x + 14, yy + 15, it.label, faint, kLeft, 1.6f);
                    cv.hline(x + 14 + cv.measure(kBold, 9.0f, it.label, 1.6f) + 8, yy + 11.5f, w - 40 - cv.measure(kBold, 9.0f, it.label, 1.6f), 1, rgb(0x2f2f35));
                } else {
                    const bool hot = (i == hoverIdx);
                    if (hot) cv.fillRoundRect(x + 4, yy + 1, w - 16, ih - 2, 4, hover);
                    if (it.selected) { cv.fillRoundRect(x + 4, yy + 4, 3, ih - 8, 1.5f, red); }
                    const float maxw = w - 56 - (it.deletable ? 22 : 0);
                    cv.text(it.selected ? kBold : kRegular, 11.5f, x + 16, yy + ih * 0.5f + 4.2f, cv.fit(it.selected ? kBold : kRegular, 11.5f, it.label, maxw), it.selected ? rgb(0xffb08a) : (hot ? rgb(0xffffff) : text));
                    if (it.deletable && hot) {
                        const float dx = x + w - 28, dy = yy + ih * 0.5f;
                        const Col xc = hoverDelete ? rgb(0xff5a4a) : dim;
                        cv.line(dx - 4, dy - 4, dx + 4, dy + 4, 1.8f, xc);  cv.line(dx - 4, dy + 4, dx + 4, dy - 4, 1.8f, xc);
                    }
                }
            }
            yy += ih;
        }
        cv.clip = saved;
        if (contentH > viewH) {   // scrollbar
            const float th = std::max(24.0f, viewH * viewH / contentH), ty = y + 4 + (viewH - th) * (scroll / (contentH - viewH));
            cv.fillRoundRect(x + w - 8, ty, 4, th, 2, rgb(0x55555d));
        }
        if (footH > 0) {
            cv.hline(x + 10, y + h - footH - 2, w - 20, 1, rgb(0x34343a));
            cv.text(kRegular, 10.5f, x + w * 0.5f, y + h - 10, footer, hoverFooter ? rgb(0xffb08a) : dim, kCenter);
        }
    }
};

}} // namespace dg::ui
