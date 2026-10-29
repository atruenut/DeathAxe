// DeathAxe UI - thin Win32 window shim around the portable Editor (child window of the host's parent HWND).
#pragma once
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <commdlg.h>
#include <windowsx.h>
#include "editor.h"

namespace dg { namespace ui {

inline std::wstring widen(const std::string& s) {
    if (s.empty()) return L"";
    const int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0);
    std::wstring w(n, 0);  MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), &w[0], n);
    return w;
}
inline std::string narrow(const std::wstring& w) {
    if (w.empty()) return "";
    const int n = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), nullptr, 0, nullptr, nullptr);
    std::string s(n, 0);  WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), &s[0], n, nullptr, nullptr);
    return s;
}

inline float windowScale(HWND hwnd) {
    typedef UINT(WINAPI * GetDpiForWindowFn)(HWND);
    static GetDpiForWindowFn fn = (GetDpiForWindowFn)GetProcAddress(GetModuleHandleW(L"user32.dll"), "GetDpiForWindow");
    UINT dpi = fn ? fn(hwnd) : 0;
    if (!dpi) { HDC dc = GetDC(nullptr); dpi = GetDeviceCaps(dc, LOGPIXELSX); ReleaseDC(nullptr, dc); }
    if (dpi < 96) dpi = 96;
    return std::max(1.0f, std::min(3.0f, dpi / 96.0f));
}

class WinWindow : public Platform {
public:
    WinWindow(Host* host) : ed(host, this) {}
    ~WinWindow() { destroy(); }

    // returns false on failure; pixel size is available afterwards
    bool create(HWND parent, float extraScale) {
        registerClass();
        HWND ref = parent;
        scale = windowScale(ref) * (extraScale > 0 ? extraScale : 1.0f);
        scale = std::max(1.0f, std::min(3.0f, scale));
        ed.open(scale);
        hwnd = CreateWindowExW(0, kClass, L"DeathAxe", WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, 0, 0,
                               ed.pixelW(), ed.pixelH(), parent, nullptr, hInst(), this);
        if (!hwnd) return false;
        createDib();
        SetTimer(hwnd, 1, 33, nullptr);
        return true;
    }
    void destroy() {
        if (hwnd) { KillTimer(hwnd, 1); HWND h = hwnd; hwnd = nullptr; DestroyWindow(h); }
        if (dib) { DeleteObject(dib); dib = nullptr; }
        if (memDc) { DeleteDC(memDc); memDc = nullptr; }
    }
    int pixelW() const { return ed.pixelW(); }
    int pixelH() const { return ed.pixelH(); }
    float getScale() const { return scale; }
    Editor ed;

    // ---- Platform (dialogs)
    bool saveDialog(const std::string& dir, const std::string& defName, std::string& out) override {
        wchar_t file[MAX_PATH * 2] = {};
        const std::wstring dn = widen(defName);
        wcsncpy(file, dn.c_str(), MAX_PATH - 1);
        const std::wstring wd = widen(dir);
        CreateDirectoryW(wd.c_str(), nullptr);          // parent may not exist; harmless if it does
        OPENFILENAMEW o = {};
        o.lStructSize = sizeof o;  o.hwndOwner = hwnd;
        o.lpstrFilter = L"DeathAxe preset (*.daxe)\0*.daxe\0\0";
        o.lpstrFile = file;  o.nMaxFile = MAX_PATH * 2;
        o.lpstrInitialDir = wd.c_str();  o.lpstrDefExt = L"daxe";  o.lpstrTitle = L"Save DeathAxe preset";
        o.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR | OFN_EXPLORER;
        modal = true;  const BOOL ok = GetSaveFileNameW(&o);  modal = false;
        if (!ok) return false;
        out = narrow(file);  return true;
    }
    bool openDialog(const std::string& dir, std::string& out) override {
        wchar_t file[MAX_PATH * 2] = {};
        const std::wstring wd = widen(dir);
        OPENFILENAMEW o = {};
        o.lStructSize = sizeof o;  o.hwndOwner = hwnd;
        o.lpstrFilter = L"DeathAxe preset (*.daxe)\0*.daxe\0All files\0*.*\0\0";
        o.lpstrFile = file;  o.nMaxFile = MAX_PATH * 2;
        o.lpstrInitialDir = wd.c_str();  o.lpstrTitle = L"Load DeathAxe preset";
        o.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR | OFN_EXPLORER;
        modal = true;  const BOOL ok = GetOpenFileNameW(&o);  modal = false;
        if (!ok) return false;
        out = narrow(file);  return true;
    }
    bool confirm(const std::string& t) override {
        modal = true;  const int r = MessageBoxW(hwnd, widen(t).c_str(), L"DeathAxe", MB_YESNO | MB_ICONQUESTION);  modal = false;
        return r == IDYES;
    }
    void message(const std::string& t) override {
        modal = true;  MessageBoxW(hwnd, widen(t).c_str(), L"DeathAxe", MB_OK | MB_ICONINFORMATION);  modal = false;
    }

private:
    static constexpr const wchar_t* kClass = L"DeathAxeEditorWnd";
    HWND hwnd = nullptr;  HBITMAP dib = nullptr;  HDC memDc = nullptr;  void* bits = nullptr;
    float scale = 1;  bool tracking = false, modal = false;

    static HINSTANCE hInst() {
        HMODULE m = nullptr;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCWSTR)&WinWindow::wndProc, &m);
        return (HINSTANCE)m;
    }
    static void registerClass() {
        static bool done = false;
        if (done) return;
        WNDCLASSEXW wc = {};
        wc.cbSize = sizeof wc;  wc.style = CS_DBLCLKS;  wc.lpfnWndProc = &WinWindow::wndProc;
        wc.hInstance = hInst();  wc.hCursor = LoadCursor(nullptr, IDC_ARROW);  wc.lpszClassName = kClass;
        RegisterClassExW(&wc);
        done = true;
    }
    void createDib() {
        HDC wdc = GetDC(hwnd);
        memDc = CreateCompatibleDC(wdc);
        BITMAPINFO bi = {};
        bi.bmiHeader.biSize = sizeof bi.bmiHeader;  bi.bmiHeader.biWidth = ed.pixelW();  bi.bmiHeader.biHeight = -ed.pixelH();
        bi.bmiHeader.biPlanes = 1;  bi.bmiHeader.biBitCount = 32;  bi.bmiHeader.biCompression = BI_RGB;
        dib = CreateDIBSection(wdc, &bi, DIB_RGB_COLORS, &bits, nullptr, 0);
        SelectObject(memDc, dib);
        ReleaseDC(hwnd, wdc);
        ed.renderAll();  copyToDib(RectI{0, 0, ed.pixelW(), ed.pixelH()});
    }
    void copyToDib(const RectI& r) {
        if (!bits) return;
        const Canvas& c = ed.canvas();
        const int x0 = std::max(0, r.x0), x1 = std::min(c.w, r.x1);
        for (int y = std::max(0, r.y0); y < std::min(c.h, r.y1); ++y)
            std::memcpy((uint32_t*)bits + (size_t)y * c.w + x0, &c.px[(size_t)y * c.w + x0], (size_t)(x1 - x0) * 4);
    }
    void pump() {
        const RectI r = ed.flush();
        if (r.x1 > r.x0 && r.y1 > r.y0) {
            copyToDib(r);
            RECT rc = {r.x0, r.y0, r.x1, r.y1};
            InvalidateRect(hwnd, &rc, FALSE);
        }
    }
    Mods mods() const { Mods m; m.shift = GetKeyState(VK_SHIFT) < 0; m.ctrl = GetKeyState(VK_CONTROL) < 0; return m; }
    static LRESULT CALLBACK wndProc(HWND h, UINT msg, WPARAM wp, LPARAM lp) {
        WinWindow* self = (WinWindow*)GetWindowLongPtrW(h, GWLP_USERDATA);
        if (msg == WM_NCCREATE) { self = (WinWindow*)((CREATESTRUCTW*)lp)->lpCreateParams; SetWindowLongPtrW(h, GWLP_USERDATA, (LONG_PTR)self); }
        if (!self || self->hwnd != h) return DefWindowProcW(h, msg, wp, lp);
        return self->handle(msg, wp, lp);
    }
    LRESULT handle(UINT msg, WPARAM wp, LPARAM lp) {
        const float s = scale;
        switch (msg) {
        case WM_ERASEBKGND: return 1;
        case WM_PAINT: {
            PAINTSTRUCT ps;  HDC dc = BeginPaint(hwnd, &ps);
            const RectI r = ed.flush();  if (r.x1 > r.x0) copyToDib(r);
            BitBlt(dc, ps.rcPaint.left, ps.rcPaint.top, ps.rcPaint.right - ps.rcPaint.left, ps.rcPaint.bottom - ps.rcPaint.top, memDc, ps.rcPaint.left, ps.rcPaint.top, SRCCOPY);
            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_TIMER: if (!modal) { ed.tick(); pump(); } return 0;
        case WM_MOUSEMOVE:
            if (!tracking) { TRACKMOUSEEVENT t = {sizeof t, TME_LEAVE, hwnd, 0}; TrackMouseEvent(&t); tracking = true; }
            ed.mouseMove(GET_X_LPARAM(lp) / s, GET_Y_LPARAM(lp) / s, mods());  pump();  return 0;
        case WM_MOUSELEAVE: tracking = false; ed.mouseLeave(); pump(); return 0;
        case WM_LBUTTONDOWN: SetCapture(hwnd); SetFocus(hwnd); ed.mouseDown(GET_X_LPARAM(lp) / s, GET_Y_LPARAM(lp) / s, mods()); pump(); return 0;
        case WM_LBUTTONDBLCLK: SetCapture(hwnd); ed.doubleClick(GET_X_LPARAM(lp) / s, GET_Y_LPARAM(lp) / s, mods()); pump(); return 0;
        case WM_LBUTTONUP: { if (GetCapture() == hwnd) ReleaseCapture(); ed.mouseUp(GET_X_LPARAM(lp) / s, GET_Y_LPARAM(lp) / s, mods()); pump(); return 0; }
        case WM_CAPTURECHANGED: return 0;
        case WM_MOUSEWHEEL: {
            POINT p = {GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};  ScreenToClient(hwnd, &p);
            ed.mouseWheel(p.x / s, p.y / s, GET_WHEEL_DELTA_WPARAM(wp) > 0 ? 1.0f : -1.0f, mods());  pump();  return 0;
        }
        case WM_KEYDOWN: if (wp == VK_ESCAPE) { ed.keyDown(27); pump(); } return 0;
        case WM_GETDLGCODE: return DLGC_WANTALLKEYS;
        }
        return DefWindowProcW(hwnd, msg, wp, lp);
    }
};

}} // namespace dg::ui
#endif
