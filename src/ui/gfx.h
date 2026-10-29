// DeathAxe UI - tiny anti-aliased software renderer (no platform or graphics-library dependencies).
// Everything is drawn into a 32-bit 0xAARRGGBB framebuffer (little endian = BGRA bytes, ready for a Windows DIB).
// All coordinates passed to the drawing functions are *logical*; the canvas multiplies them by its scale factor.
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>

#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_STATIC
#include "third_party/stb_truetype.h"
#include "font_data.h"

namespace dg { namespace ui {

struct Col { uint8_t r = 0, g = 0, b = 0, a = 255; };
constexpr Col rgb(uint32_t hex, uint8_t a = 255) { return Col{(uint8_t)(hex >> 16), (uint8_t)(hex >> 8), (uint8_t)hex, a}; }
inline Col mix(Col a, Col b, float t) {
    t = std::max(0.f, std::min(1.f, t));
    return Col{(uint8_t)(a.r + (b.r - a.r) * t + 0.5f), (uint8_t)(a.g + (b.g - a.g) * t + 0.5f),
               (uint8_t)(a.b + (b.b - a.b) * t + 0.5f), (uint8_t)(a.a + (b.a - a.a) * t + 0.5f)};
}
inline Col withAlpha(Col c, float a) { c.a = (uint8_t)std::max(0.f, std::min(255.f, c.a * a)); return c; }

struct RectI {
    int x0 = 0, y0 = 0, x1 = 0, y1 = 0;
    bool empty() const { return x1 <= x0 || y1 <= y0; }
    void add(const RectI& o) {
        if (o.empty()) return;
        if (empty()) { *this = o; return; }
        x0 = std::min(x0, o.x0); y0 = std::min(y0, o.y0); x1 = std::max(x1, o.x1); y1 = std::max(y1, o.y1);
    }
};

struct Glyph { int w = 0, h = 0, xo = 0, yo = 0; float adv = 0; std::vector<uint8_t> bmp; };

enum Face { kRegular = 0, kBold = 1 };
enum Align { kLeft = 0, kCenter = 1, kRight = 2 };

class Fonts {
public:
    static Fonts& get() { static Fonts f; return f; }
    stbtt_fontinfo info[2];
    bool ok = false;
    std::unordered_map<uint64_t, Glyph> cache;
    uint8_t lut[256];

    Fonts() {
        const unsigned char* d[2] = {kFontRegular, kFontBold};
        ok = true;
        for (int i = 0; i < 2; ++i)
            if (!stbtt_InitFont(&info[i], d[i], stbtt_GetFontOffsetForIndex(d[i], 0))) ok = false;
        for (int i = 0; i < 256; ++i) lut[i] = (uint8_t)(255.0 * std::pow(i / 255.0, 0.82) + 0.5);
    }
    const Glyph& glyph(int face, float px, int cp) {
        const uint64_t key = ((uint64_t)face << 56) | ((uint64_t)(int)std::lround(px * 8) << 24) | (uint64_t)cp;
        auto it = cache.find(key);
        if (it != cache.end()) return it->second;
        Glyph g;
        if (ok) {
            const float sc = stbtt_ScaleForMappingEmToPixels(&info[face], px);
            int adv, lsb;  stbtt_GetCodepointHMetrics(&info[face], cp, &adv, &lsb);
            g.adv = adv * sc;
            int x0, y0, x1, y1;
            stbtt_GetCodepointBitmapBox(&info[face], cp, sc, sc, &x0, &y0, &x1, &y1);
            g.w = x1 - x0; g.h = y1 - y0; g.xo = x0; g.yo = y0;
            if (g.w > 0 && g.h > 0) {
                g.bmp.assign((size_t)g.w * g.h, 0);
                stbtt_MakeCodepointBitmap(&info[face], g.bmp.data(), g.w, g.h, g.w, sc, sc, cp);
            }
        }
        return cache.emplace(key, std::move(g)).first->second;
    }
};

inline int utf8Next(const char*& p) {
    const unsigned char c = (unsigned char)*p++;
    if (c < 0x80) return c;
    if ((c >> 5) == 6 && (*p & 0xC0) == 0x80) { const int r = ((c & 31) << 6) | (*p & 63); ++p; return r; }
    if ((c >> 4) == 14 && (p[0] & 0xC0) == 0x80 && (p[1] & 0xC0) == 0x80) { const int r = ((c & 15) << 12) | ((p[0] & 63) << 6) | (p[1] & 63); p += 2; return r; }
    return '?';
}

class Canvas {
public:
    int w = 0, h = 0;
    float s = 1.0f;
    std::vector<uint32_t> px;
    RectI clip;

    void init(int width, int height, float scale) {
        w = width; h = height; s = scale; px.assign((size_t)w * h, 0xFF000000u); resetClip();
    }
    void resetClip() { clip = RectI{0, 0, w, h}; }
    void setClipLogical(float x, float y, float cw, float ch) {
        clip = RectI{std::max(0, (int)std::floor(x * s)), std::max(0, (int)std::floor(y * s)),
                     std::min(w, (int)std::ceil((x + cw) * s)), std::min(h, (int)std::ceil((y + ch) * s))};
    }
    RectI toDevice(float x, float y, float rw, float rh) const {
        return RectI{std::max(0, (int)std::floor(x * s) - 1), std::max(0, (int)std::floor(y * s) - 1),
                     std::min(w, (int)std::ceil((x + rw) * s) + 1), std::min(h, (int)std::ceil((y + rh) * s) + 1)};
    }

    void clear(Col c) { const uint32_t v = 0xFF000000u | (c.r << 16) | (c.g << 8) | c.b; std::fill(px.begin(), px.end(), v); }

    void copyRect(const Canvas& src, const RectI& r) {
        const int x0 = std::max(0, r.x0), y0 = std::max(0, r.y0), x1 = std::min(std::min(w, src.w), r.x1), y1 = std::min(std::min(h, src.h), r.y1);
        if (x1 <= x0) return;
        for (int y = y0; y < y1; ++y) std::memcpy(&px[(size_t)y * w + x0], &src.px[(size_t)y * src.w + x0], (size_t)(x1 - x0) * 4);
    }

    inline void blend(int x, int y, Col c, float cov) {
        if (x < clip.x0 || y < clip.y0 || x >= clip.x1 || y >= clip.y1) return;
        const float a = c.a * (1.0f / 255.0f) * cov;
        if (a <= 0.0f) return;
        uint32_t& d = px[(size_t)y * w + x];
        const float dr = (float)((d >> 16) & 255), dg = (float)((d >> 8) & 255), db = (float)(d & 255);
        const int r = (int)(dr + (c.r - dr) * a + 0.5f), g = (int)(dg + (c.g - dg) * a + 0.5f), b = (int)(db + (c.b - db) * a + 0.5f);
        d = 0xFF000000u | (r << 16) | (g << 8) | b;
    }

    // generic signed-distance shader. bbox in device pixels; sdf returns distance in device pixels (negative inside).
    template <class SDF, class COL>
    void shade(float bx0, float by0, float bx1, float by1, SDF&& sdf, COL&& col) {
        const int x0 = std::max(clip.x0, (int)std::floor(bx0) - 1), y0 = std::max(clip.y0, (int)std::floor(by0) - 1);
        const int x1 = std::min(clip.x1, (int)std::ceil(bx1) + 1), y1 = std::min(clip.y1, (int)std::ceil(by1) + 1);
        for (int y = y0; y < y1; ++y)
            for (int x = x0; x < x1; ++x) {
                const float d = sdf(x + 0.5f, y + 0.5f);
                const float cov = 0.5f - d;
                if (cov <= 0.0f) continue;
                blend(x, y, col(x + 0.5f, y + 0.5f), cov > 1.0f ? 1.0f : cov);
            }
    }

    static float sdRoundBox(float px, float py, float cx, float cy, float hw, float hh, float r) {
        const float qx = std::fabs(px - cx) - (hw - r), qy = std::fabs(py - cy) - (hh - r);
        const float ox = std::max(qx, 0.f), oy = std::max(qy, 0.f);
        return std::sqrt(ox * ox + oy * oy) + std::min(std::max(qx, qy), 0.f) - r;
    }

    // ---- rectangles
    void fillRoundRectV(float x, float y, float rw, float rh, float r, Col top, Col bottom) {
        const float X = x * s, Y = y * s, W = rw * s, H = rh * s, R = std::min(r * s, std::min(W, H) * 0.5f);
        shade(X, Y, X + W, Y + H, [&](float px_, float py_) { return sdRoundBox(px_, py_, X + W * 0.5f, Y + H * 0.5f, W * 0.5f, H * 0.5f, R); },
              [&](float, float py_) { return mix(top, bottom, (py_ - Y) / H); });
    }
    void fillRoundRect(float x, float y, float rw, float rh, float r, Col c) { fillRoundRectV(x, y, rw, rh, r, c, c); }
    void fillRect(float x, float y, float rw, float rh, Col c) { fillRoundRectV(x, y, rw, rh, 0, c, c); }
    void strokeRoundRect(float x, float y, float rw, float rh, float r, float thick, Col c) {
        const float t = thick * s, X = x * s + t * 0.5f, Y = y * s + t * 0.5f, W = rw * s - t, H = rh * s - t, R = std::max(0.f, std::min(r * s - t * 0.5f, std::min(W, H) * 0.5f));
        shade(X - t, Y - t, X + W + t, Y + H + t,
              [&](float px_, float py_) { return std::fabs(sdRoundBox(px_, py_, X + W * 0.5f, Y + H * 0.5f, W * 0.5f, H * 0.5f, R)) - t * 0.5f; },
              [&](float, float) { return c; });
    }
    void hline(float x, float y, float len, float thick, Col c) { fillRect(x, y, len, thick, c); }

    // ---- circles / arcs / lines
    void fillCircleV(float cx, float cy, float r, Col top, Col bottom) {
        const float X = cx * s, Y = cy * s, R = r * s;
        shade(X - R, Y - R, X + R, Y + R, [&](float px_, float py_) { return std::hypot(px_ - X, py_ - Y) - R; },
              [&](float, float py_) { return mix(top, bottom, (py_ - (Y - R)) / (2 * R)); });
    }
    void fillCircle(float cx, float cy, float r, Col c) { fillCircleV(cx, cy, r, c, c); }
    void strokeCircle(float cx, float cy, float r, float thick, Col c) {
        const float X = cx * s, Y = cy * s, R = r * s, T = thick * s;
        shade(X - R - T, Y - R - T, X + R + T, Y + R + T, [&](float px_, float py_) { return std::fabs(std::hypot(px_ - X, py_ - Y) - R) - T * 0.5f; },
              [&](float, float) { return c; });
    }
    void line(float x0, float y0, float x1, float y1, float thick, Col c) {
        const float ax = x0 * s, ay = y0 * s, bx = x1 * s, by = y1 * s, T = thick * s;
        const float dx = bx - ax, dy = by - ay, l2 = std::max(1e-6f, dx * dx + dy * dy);
        shade(std::min(ax, bx) - T, std::min(ay, by) - T, std::max(ax, bx) + T, std::max(ay, by) + T,
              [&](float px_, float py_) {
                  float t = ((px_ - ax) * dx + (py_ - ay) * dy) / l2;  t = std::max(0.f, std::min(1.f, t));
                  return std::hypot(px_ - (ax + t * dx), py_ - (ay + t * dy)) - T * 0.5f;
              },
              [&](float, float) { return c; });
    }
    // arc from angle a0 to a1 (radians, screen coords: 0 = +x, clockwise), round caps, colour gradient c0->c1 over [refA0, refA0+refSpan]
    void strokeArc(float cx, float cy, float r, float thick, float a0, float a1, Col c0, Col c1, float refA0, float refSpan) {
        const float X = cx * s, Y = cy * s, R = r * s, T = thick * s;
        const float span = a1 - a0;
        if (span < 0) return;
        const float e0x = X + R * std::cos(a0), e0y = Y + R * std::sin(a0), e1x = X + R * std::cos(a1), e1y = Y + R * std::sin(a1);
        const float twoPi = 6.28318530718f;
        shade(X - R - T, Y - R - T, X + R + T, Y + R + T,
              [&](float px_, float py_) {
                  const float dx = px_ - X, dy = py_ - Y, len = std::hypot(dx, dy);
                  float rel = std::atan2(dy, dx) - a0;
                  rel = std::fmod(rel + 4 * twoPi, twoPi);
                  if (rel <= span || span >= twoPi) return std::fabs(len - R) - T * 0.5f;
                  return std::min(std::hypot(px_ - e0x, py_ - e0y), std::hypot(px_ - e1x, py_ - e1y)) - T * 0.5f;
              },
              [&](float px_, float py_) {
                  float ang = std::atan2(py_ - Y, px_ - X) - refA0;
                  ang = std::fmod(ang + 4 * twoPi, twoPi);
                  if (ang > refSpan + 0.5f * (twoPi - refSpan)) ang = 0;        // beyond the sweep: first colour (cap region)
                  return mix(c0, c1, refSpan > 0 ? std::min(1.f, ang / refSpan) : 0.f);
              });
    }
    void strokeArc(float cx, float cy, float r, float thick, float a0, float a1, Col c) { strokeArc(cx, cy, r, thick, a0, a1, c, c, a0, std::max(0.01f, a1 - a0)); }

    void fillConvex(const float* xy, int n, Col c) {   // convex polygon, vertices in logical coords, any winding
        float v[16][2];  n = std::min(n, 8);
        float mx = 0, my = 0, minx = 1e9f, miny = 1e9f, maxx = -1e9f, maxy = -1e9f;
        for (int i = 0; i < n; ++i) { v[i][0] = xy[2 * i] * s; v[i][1] = xy[2 * i + 1] * s; mx += v[i][0]; my += v[i][1]; minx = std::min(minx, v[i][0]); maxx = std::max(maxx, v[i][0]); miny = std::min(miny, v[i][1]); maxy = std::max(maxy, v[i][1]); }
        mx /= n; my /= n;
        float nx[8], ny[8], off[8];
        for (int i = 0; i < n; ++i) {
            const int j = (i + 1) % n;
            float ex = v[j][0] - v[i][0], ey = v[j][1] - v[i][1];
            const float l = std::max(1e-6f, std::hypot(ex, ey));
            float ax = ey / l, ay = -ex / l;
            if (ax * (mx - v[i][0]) + ay * (my - v[i][1]) > 0) { ax = -ax; ay = -ay; }   // outward
            nx[i] = ax; ny[i] = ay; off[i] = ax * v[i][0] + ay * v[i][1];
        }
        shade(minx, miny, maxx, maxy, [&](float px_, float py_) {
            float d = -1e9f;
            for (int i = 0; i < n; ++i) d = std::max(d, nx[i] * px_ + ny[i] * py_ - off[i]);
            return d;
        }, [&](float, float) { return c; });
    }

    // ---- text
    float measure(int face, float size, const std::string& str, float spacing = 0) const {
        Fonts& F = Fonts::get();
        float wsum = 0;  const char* p = str.c_str();  int n = 0;
        while (*p) { const int cp = utf8Next(p); wsum += F.glyph(face, size * s, cp).adv + spacing * s; ++n; }
        if (n) wsum -= spacing * s;
        return wsum / s;
    }
    // draws text with its baseline at y (logical). returns drawn width (logical)
    float text(int face, float size, float x, float y, const std::string& str, Col c, int align = kLeft, float spacing = 0) {
        Fonts& F = Fonts::get();
        const float total = measure(face, size, str, spacing);
        float cx = (align == kCenter ? x - total * 0.5f : align == kRight ? x - total : x) * s;
        const int by = (int)std::lround(y * s);
        const char* p = str.c_str();
        while (*p) {
            const int cp = utf8Next(p);
            const Glyph& g = F.glyph(face, size * s, cp);
            const int gx = (int)std::lround(cx) + g.xo, gy = by + g.yo;
            for (int yy = 0; yy < g.h; ++yy)
                for (int xx = 0; xx < g.w; ++xx) {
                    const uint8_t v = g.bmp[(size_t)yy * g.w + xx];
                    if (v) blend(gx + xx, gy + yy, c, F.lut[v] * (1.0f / 255.0f));
                }
            cx += g.adv + spacing * s;
        }
        return total;
    }
    std::string fit(int face, float size, const std::string& str, float maxW, float spacing = 0) const {
        if (measure(face, size, str, spacing) <= maxW) return str;
        std::string t = str;
        while (!t.empty()) {
            t.pop_back();
            if (measure(face, size, t + "\xE2\x80\xA6", spacing) <= maxW) return t + "\xE2\x80\xA6";
        }
        return "";
    }
};

}} // namespace dg::ui
