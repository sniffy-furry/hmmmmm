#include "ui/AppShell.h"
#include "core/Logger.h"
#include "core/StringUtil.h"
#include "core/KnownNames.h"

#include "renderer/GLCompat.h"
#include <vangui.h>
#if defined(__ANDROID__)
#  include <android_native_app_glue.h>
#  include <vangui_impl_android.h>
#else
#  include <GLFW/glfw3.h>
#  include <vangui_impl_glfw.h>
#endif
#include <vangui_impl_opengl3.h>
#include <vangui_enhance.h>   // VanGui::NewFrameExtras/RenderExtras drivers
#include <vangui_shortcuts.h> // VanGui::RegisterShortcut/DispatchShortcuts
#include <vangui_command_palette.h>
#include <vangui_themes.h>
#include <vangui_theme_engine.h>

#include <filesystem>
#include <algorithm>

#ifdef _WIN32
#  define WIN32_LEAN_AND_MEAN
#  define NOMINMAX
#  include <windows.h>
#  include <windowsx.h>
#  include <dwmapi.h>
#  include <wincodec.h>
#  include <shobjidl.h>
#  pragma comment(lib, "dwmapi.lib")
#  pragma comment(lib, "windowscodecs.lib")
#  pragma comment(lib, "shell32.lib")
#  define GLFW_EXPOSE_NATIVE_WIN32
#  include <GLFW/glfw3native.h>
#endif

#include <vector>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <span>

namespace nfsmw {

// ─── Embedded-resource PNG loader (Windows only) ─────────────────────────────
// Reads an RCDATA resource by integer ID, decodes the PNG bytes with WIC,
// and returns raw RGBA pixels. COM must already be initialised by the caller.
#ifdef _WIN32
static bool LoadPNGResource(WORD id, std::vector<uint8_t>& rgba,
                             int& outW, int& outH) {
    HMODULE      hMod  = GetModuleHandleW(nullptr);
    HRSRC        hRes  = FindResourceW(hMod, MAKEINTRESOURCEW(id), MAKEINTRESOURCEW(10)); // 10 = RT_RCDATA
    if (!hRes) return false;
    HGLOBAL      hGlob = LoadResource(hMod, hRes);
    if (!hGlob) return false;
    const void*  pData = LockResource(hGlob);
    DWORD        dSize = SizeofResource(hMod, hRes);
    if (!pData || !dSize) return false;

    // RAII wrapper so every COM interface gets Released on return.
    struct WIC {
        IWICImagingFactory*    fac{};
        IWICStream*            stm{};
        IWICBitmapDecoder*     dec{};
        IWICBitmapFrameDecode* frm{};
        IWICFormatConverter*   cvt{};
        ~WIC() {
            if (cvt) cvt->Release();
            if (frm) frm->Release();
            if (dec) dec->Release();
            if (stm) stm->Release();
            if (fac) fac->Release();
        }
    } w;

    if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr,
                                CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&w.fac))))
        return false;
    if (FAILED(w.fac->CreateStream(&w.stm))) return false;
    if (FAILED(w.stm->InitializeFromMemory(
            reinterpret_cast<WICInProcPointer>(const_cast<void*>(pData)),
            static_cast<DWORD>(dSize)))) return false;
    if (FAILED(w.fac->CreateDecoderFromStream(
            w.stm, nullptr, WICDecodeMetadataCacheOnLoad, &w.dec))) return false;
    if (FAILED(w.dec->GetFrame(0, &w.frm))) return false;
    if (FAILED(w.fac->CreateFormatConverter(&w.cvt))) return false;
    if (FAILED(w.cvt->Initialize(w.frm, GUID_WICPixelFormat32bppRGBA,
            WICBitmapDitherTypeNone, nullptr, 0.0,
            WICBitmapPaletteTypeCustom))) return false;

    UINT pw = 0, ph = 0;
    if (FAILED(w.cvt->GetSize(&pw, &ph))) return false;
    outW = static_cast<int>(pw);
    outH = static_cast<int>(ph);
    rgba.resize(pw * ph * 4);
    return SUCCEEDED(w.cvt->CopyPixels(nullptr, pw * 4,
                                        static_cast<UINT>(rgba.size()),
                                        rgba.data()));
}
#endif

namespace {
    bool   g_showAbout     = false;
    bool   g_showShortcuts = false;
}

// Theme colours — defined here, declared extern in ui/Theme.h.
// Updated by ApplyTheme(); read by NavButton, DrawNavSidebar,
// DrawContentArea, the TaskQueue overlay, and any other panel that
// includes ui/Theme.h.
VanVec4 g_accent = VanVec4(0.976f, 0.451f, 0.086f, 1.0f); // orange-500 (dark default)
VanVec4 g_bg0    = VanVec4(0.071f, 0.071f, 0.071f, 1.0f); // #121212 neutral
VanVec4 g_bg1    = VanVec4(0.110f, 0.110f, 0.110f, 1.0f); // #1C1C1C
VanVec4 g_bg2    = VanVec4(0.157f, 0.157f, 0.157f, 1.0f); // #282828
VanVec4 g_bg3    = VanVec4(0.204f, 0.204f, 0.204f, 1.0f); // #343434
VanVec4 g_border = VanVec4(0.275f, 0.275f, 0.275f, 0.55f);
VanVec4 g_text   = VanVec4(0.918f, 0.918f, 0.918f, 1.0f);

// ─── Borderless window: custom hit-testing (Win32) ───────────────────────────
// With GLFW_DECORATED off we draw our own title bar. We chain the GLFW window
// proc and override WM_NCHITTEST so Windows still gives us native edge-resize,
// caption drag, aero-snap and double-click-maximize for the frameless window.
#ifdef _WIN32
namespace {
    WNDPROC g_glfwWndProc        = nullptr;
    float   g_captionHeight      = 32.0f;   // updated each frame by DrawMenuBar
    bool    g_captionInteractive = false;   // true when hovering a title-bar widget

    LRESULT CALLBACK AppShellWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        // Strip the OS frame visually while keeping WS_THICKFRAME's resize/snap
        // behaviour: report the whole window rect as client area. When maximized,
        // clamp the client to the monitor work area so we don't cover the taskbar.
        if (msg == WM_NCCALCSIZE && wParam == TRUE) {
            auto& params = *reinterpret_cast<NCCALCSIZE_PARAMS*>(lParam);
            if (IsZoomed(hwnd)) {
                HMONITOR mon = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
                MONITORINFO mi{ sizeof(mi) };
                if (GetMonitorInfo(mon, &mi))
                    params.rgrc[0] = mi.rcWork;
            }
            return 0;
        }
        if (msg == WM_NCHITTEST) {
            POINT pt{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            RECT rc; GetWindowRect(hwnd, &rc);
            const int border = 8;
            const int x = pt.x - rc.left, y = pt.y - rc.top;
            const int w = rc.right - rc.left, h = rc.bottom - rc.top;

            if (!IsZoomed(hwnd)) {
                const bool l = x < border, r = x >= w - border;
                const bool t = y < border, b = y >= h - border;
                if (t && l) return HTTOPLEFT;
                if (t && r) return HTTOPRIGHT;
                if (b && l) return HTBOTTOMLEFT;
                if (b && r) return HTBOTTOMRIGHT;
                if (l)      return HTLEFT;
                if (r)      return HTRIGHT;
                if (t)      return HTTOP;
                if (b)      return HTBOTTOM;
            }
            if (y < static_cast<int>(g_captionHeight) && !g_captionInteractive)
                return HTCAPTION;
            return HTCLIENT;
        }
        return CallWindowProc(g_glfwWndProc, hwnd, msg, wParam, lParam);
    }
}
#endif

// ─── Theme ───────────────────────────────────────────────────────────────────

void AppShell::LoadFont(float dpiScale) {
    VanGuiIO& io = VanGui::GetIO();
    const float size = 17.0f * (dpiScale > 1.0f ? dpiScale : 1.0f);
#ifdef _WIN32
    const char* candidates[] = {
        "C:\\Windows\\Fonts\\segoeui.ttf",
        "C:\\Windows\\Fonts\\arial.ttf",
        "C:\\Windows\\Fonts\\tahoma.ttf",
    };
    for (const char* path : candidates) {
        if (std::filesystem::exists(path)) {
            if (io.Fonts->AddFontFromFileTTF(path, size)) {
                io.FontGlobalScale = 1.0f;
                return;
            }
        }
    }
#endif
    (void)size;
    io.FontGlobalScale = dpiScale > 1.0f ? dpiScale : 1.15f;
}

// ─── MWThemeTokens ────────────────────────────────────────────────────────────
// Semantic token set for the VanGUI theme-engine. Drives the animated transition
// in ToggleTheme(): TransitionToTokenSet() interpolates the token-mapped style
// colours from the old theme to the new one over a few frames. The full,
// high-fidelity palette is still applied instantly by ApplyTheme(); the engine
// only animates the core semantic colours on top.
static VanGui::VanThemeTokenSet MWThemeTokens(bool dark) {
    using namespace VanGui;
    VanThemeTokenSet ts{};
    auto& C = ts.Colors;
    C[VanThemeToken_Background]  = dark ? VanVec4(0.071f,0.071f,0.071f,1.0f) : VanVec4(0.980f,0.973f,0.961f,1.0f);
    C[VanThemeToken_Surface]     = dark ? VanVec4(0.110f,0.110f,0.110f,1.0f) : VanVec4(0.941f,0.929f,0.910f,1.0f);
    C[VanThemeToken_Border]      = dark ? VanVec4(0.275f,0.275f,0.275f,0.55f): VanVec4(0.722f,0.690f,0.647f,0.80f);
    C[VanThemeToken_Primary]     = dark ? VanVec4(0.976f,0.451f,0.086f,1.0f) : VanVec4(0.776f,0.263f,0.047f,1.0f);
    C[VanThemeToken_Secondary]   = dark ? VanVec4(0.984f,0.573f,0.235f,1.0f) : VanVec4(0.918f,0.345f,0.047f,1.0f);
    C[VanThemeToken_TextPrimary] = dark ? VanVec4(0.918f,0.918f,0.918f,1.0f) : VanVec4(0.102f,0.078f,0.059f,1.0f);
    C[VanThemeToken_TextDim]     = dark ? VanVec4(0.50f,0.50f,0.50f,1.0f)    : VanVec4(0.45f,0.40f,0.35f,1.0f);
    C[VanThemeToken_Error]       = VanVec4(0.901f,0.235f,0.235f,1.0f);
    C[VanThemeToken_Warning]     = VanVec4(0.952f,0.643f,0.110f,1.0f);
    C[VanThemeToken_Success]     = VanVec4(0.298f,0.733f,0.376f,1.0f);
    C[VanThemeToken_Info]        = dark ? VanVec4(0.976f,0.451f,0.086f,1.0f) : VanVec4(0.776f,0.263f,0.047f,1.0f);
    C[VanThemeToken_Overlay]     = VanVec4(0,0,0,0.45f);
    return ts;
}

void AppShell::ApplyTheme(float dpiScale) {
    const bool dark = (themeMode_ == ThemeMode::Dark);

    // ─── Dark  — true neutral grey/black + NFS orange accent ──────────────────
    // ─── Light — warm off-white + deep orange accent ───────────────────────────

    // Window/child bg alpha: slightly transparent when Mica is active so the
    // DWM blur shows through for the glassmorphism effect.
    const float bgA = micaActive_ ? 0.82f : 1.0f;

    const VanVec4 bg0    = dark ? VanVec4(0.071f, 0.071f, 0.071f, bgA)  // #121212
                               : VanVec4(0.980f, 0.973f, 0.961f, 1.0f);
    const VanVec4 bg1    = dark ? VanVec4(0.110f, 0.110f, 0.110f, bgA)  // #1C1C1C
                               : VanVec4(0.941f, 0.929f, 0.910f, 1.0f);
    const VanVec4 bg2    = dark ? VanVec4(0.157f, 0.157f, 0.157f, 1.0f) // #282828
                               : VanVec4(0.890f, 0.871f, 0.843f, 1.0f);
    const VanVec4 bg3    = dark ? VanVec4(0.204f, 0.204f, 0.204f, 1.0f) // #343434
                               : VanVec4(0.831f, 0.808f, 0.773f, 1.0f);
    const VanVec4 border = dark ? VanVec4(0.275f, 0.275f, 0.275f, 0.55f)
                               : VanVec4(0.722f, 0.690f, 0.647f, 0.80f);
    const VanVec4 accent = dark ? VanVec4(0.976f, 0.451f, 0.086f, 1.0f)   // orange-500
                               : VanVec4(0.776f, 0.263f, 0.047f, 1.0f);  // orange-700
    const VanVec4 accentH= dark ? VanVec4(0.984f, 0.573f, 0.235f, 1.0f)
                               : VanVec4(0.918f, 0.345f, 0.047f, 1.0f);
    const VanVec4 accentA= dark ? VanVec4(0.898f, 0.373f, 0.047f, 1.0f)
                               : VanVec4(0.604f, 0.204f, 0.047f, 1.0f);
    const VanVec4 text   = dark ? VanVec4(0.918f, 0.918f, 0.918f, 1.0f)  // near-white neutral
                               : VanVec4(0.102f, 0.078f, 0.059f, 1.0f);
    const VanVec4 textDis= dark ? VanVec4(0.50f,  0.50f,  0.50f,  1.0f)
                               : VanVec4(0.45f,  0.40f,  0.35f,  1.0f);

    // Propagate to global cache used by NavButton / DrawNavSidebar / etc.
    g_bg0 = bg0; g_bg1 = bg1; g_bg2 = bg2; g_bg3 = bg3;
    g_border = border; g_accent = accent; g_text = text;

    VanGuiStyle& style = VanGui::GetStyle();
    style = VanGuiStyle();

    style.WindowRounding    = 0.0f;
    style.ChildRounding     = 6.0f;
    style.FrameRounding     = 5.0f;
    style.GrabRounding      = 5.0f;
    style.PopupRounding     = 7.0f;
    style.ScrollbarRounding = 6.0f;
    style.TabRounding       = 5.0f;
    style.WindowPadding     = VanVec2(10, 10);
    style.FramePadding      = VanVec2(8, 5);
    style.ItemSpacing       = VanVec2(8, 5);
    style.ItemInnerSpacing  = VanVec2(6, 4);
    style.IndentSpacing     = 14.0f;
    style.ScrollbarSize     = 10.0f;
    style.GrabMinSize       = 10.0f;
    style.TabBarBorderSize  = 0.0f;
    style.WindowBorderSize  = 0.0f;
    style.ChildBorderSize   = 0.0f;   // panels use bg color differences, not outlines
    style.FrameBorderSize   = 0.0f;
    style.SeparatorTextBorderSize = 1.0f;

    VanVec4* c = style.Colors;

    c[VanGuiCol_Text]                 = text;
    c[VanGuiCol_TextDisabled]         = textDis;
    c[VanGuiCol_WindowBg]             = bg0;
    c[VanGuiCol_ChildBg]              = bg1;  // panels read as distinct surfaces without borders
    c[VanGuiCol_PopupBg]              = VanVec4(bg1.x, bg1.y, bg1.z, 0.97f);
    c[VanGuiCol_MenuBarBg]            = bg0;
    c[VanGuiCol_Border]               = border;
    c[VanGuiCol_BorderShadow]         = VanVec4(0, 0, 0, 0);
    c[VanGuiCol_FrameBg]              = bg1;
    c[VanGuiCol_FrameBgHovered]       = bg2;
    c[VanGuiCol_FrameBgActive]        = bg3;
    c[VanGuiCol_TitleBg]              = bg0;
    c[VanGuiCol_TitleBgActive]        = bg1;
    c[VanGuiCol_TitleBgCollapsed]     = bg0;
    c[VanGuiCol_Tab]                  = bg1;
    c[VanGuiCol_TabHovered]           = bg3;
    c[VanGuiCol_TabActive]            = VanVec4(bg3.x * (dark ? 1.15f : 0.93f),
                                              bg3.y * (dark ? 1.15f : 0.93f),
                                              bg3.z * (dark ? 1.15f : 0.93f), 1.0f);
    c[VanGuiCol_TabUnfocused]         = bg0;
    c[VanGuiCol_TabUnfocusedActive]   = bg1;
    c[VanGuiCol_Header]               = bg2;
    c[VanGuiCol_HeaderHovered]        = bg3;
    c[VanGuiCol_HeaderActive]         = VanVec4(accent.x, accent.y, accent.z, 0.45f);
    c[VanGuiCol_Button]               = bg2;
    c[VanGuiCol_ButtonHovered]        = bg3;
    c[VanGuiCol_ButtonActive]         = accentA;
    c[VanGuiCol_CheckMark]            = accent;
    c[VanGuiCol_SliderGrab]           = accent;   // also used by TaskQueue overlay
    c[VanGuiCol_SliderGrabActive]     = accentH;
    c[VanGuiCol_ScrollbarBg]          = bg0;
    c[VanGuiCol_ScrollbarGrab]        = bg2;
    c[VanGuiCol_ScrollbarGrabHovered] = bg3;
    c[VanGuiCol_ScrollbarGrabActive]  = accent;
    c[VanGuiCol_Separator]            = VanVec4(border.x, border.y, border.z, 0.50f);
    c[VanGuiCol_SeparatorHovered]     = VanVec4(accent.x, accent.y, accent.z, 0.50f);
    c[VanGuiCol_SeparatorActive]      = accent;
    c[VanGuiCol_ResizeGrip]           = VanVec4(accent.x, accent.y, accent.z, 0.15f);
    c[VanGuiCol_ResizeGripHovered]    = VanVec4(accent.x, accent.y, accent.z, 0.50f);
    c[VanGuiCol_ResizeGripActive]     = accent;
    c[VanGuiCol_PlotLines]            = accentH;
    c[VanGuiCol_PlotHistogram]        = accent;
    c[VanGuiCol_TextSelectedBg]       = VanVec4(accent.x, accent.y, accent.z, 0.30f);
    c[VanGuiCol_NavHighlight]         = accentH;
    c[VanGuiCol_NavWindowingHighlight]= VanVec4(1, 1, 1, 0.70f);
    c[VanGuiCol_NavWindowingDimBg]    = VanVec4(0, 0, 0, 0.20f);
    c[VanGuiCol_ModalWindowDimBg]     = VanVec4(0, 0, 0, 0.45f);

    style.ScaleAllSizes(dpiScale > 1.0f ? dpiScale : 1.0f);
}

// ─── ApplyDarkTitleBar / ToggleTheme ─────────────────────────────────────────

void AppShell::ApplyDarkTitleBar(bool dark) {
#ifdef _WIN32
    if (!glfwWindow_) return;
    HWND hwnd = glfwGetWin32Window(glfwWindow_);
    if (!hwnd) return;
    BOOL d = dark ? TRUE : FALSE;
    DwmSetWindowAttribute(hwnd, 20, &d, sizeof(d));
    DwmSetWindowAttribute(hwnd, 19, &d, sizeof(d));

    // DWMWA_CAPTION_COLOR = 35. The DWMSBT_MAINWINDOW backdrop (Mica) makes DWM
    // paint its own fixed-colour caption strip across the top ~32-40px of the
    // window — independent of WM_NCCALCSIZE, which only governs the window's
    // OWN non-client geometry, not DWM's compositor overlay. DWMWA_COLOR_NONE
    // doesn't reliably suppress that strip, so instead we colour-match it to
    // our VanGui-drawn menu bar background (VanGuiCol_MenuBarBg == bg0) so the
    // seam is invisible. Re-applied on every theme toggle.
    const VanVec4& capBg = g_bg0;
    const COLORREF capColor = RGB(
        static_cast<BYTE>(capBg.x * 255.0f + 0.5f),
        static_cast<BYTE>(capBg.y * 255.0f + 0.5f),
        static_cast<BYTE>(capBg.z * 255.0f + 0.5f));
    DwmSetWindowAttribute(hwnd, 35, &capColor, sizeof(capColor));
#else
    (void)dark;
#endif
}

void AppShell::ToggleTheme() {
    themeMode_ = (themeMode_ == ThemeMode::Dark) ? ThemeMode::Light : ThemeMode::Dark;
    const bool dark = (themeMode_ == ThemeMode::Dark);

    // Kick off an animated transition of the core semantic colours FIRST — it
    // captures the current (outgoing) style as the start keyframe. ApplyTheme()
    // then applies the full new palette instantly; RenderThemeTransition()
    // (driven each frame by VanGui::NewFrameExtras) animates the token-mapped
    // subset from old → new on top of it.
    VanGui::TransitionToTokenSet(MWThemeTokens(dark), 220.0f);

#if defined(__ANDROID__)
    ApplyTheme(1.0f);
#else
    float xs = 1.0f, ys = 1.0f;
    glfwGetWindowContentScale(glfwWindow_, &xs, &ys);
    (void)ys;
    ApplyTheme(xs > 0.0f ? xs : 1.0f);
#endif
    ApplyDarkTitleBar(dark);
}

// ─── Lifecycle ───────────────────────────────────────────────────────────────

#if !defined(__ANDROID__)
Result<void> AppShell::Init(int width, int height) {
    Logger::Init("nfsmwtoolkit.log");
    LOG_INFO("MWTools starting");

    size_t knownNames = RegisterKnownNames();
    LOG_INFO("Registered " + std::to_string(knownNames) + " known hash names");

#ifdef _WIN32
    // Give the process a stable, explicit AppUserModelID. Without one, Windows
    // derives taskbar identity/icon-cache keys from the exe's path, and during
    // dev iteration (same path rebuilt repeatedly) Explorer's icon cache can get
    // stuck showing a blank/generic tile even when WM_SETICON succeeds. A fixed
    // AppID gives the taskbar its own un-cached identity for this app.
    SetCurrentProcessExplicitAppUserModelID(L"TeamVanilla.MWTools");

    // Initialise COM for WIC (icon/cursor decode) and any later COM usage.
    HRESULT hrCom = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    comInitialized_ = SUCCEEDED(hrCom); // S_FALSE = already init, still counts
#endif

    if (!glfwInit())
        return Result<void>::Err("glfwInit failed");

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    // Transparent framebuffer lets DWM Mica/Acrylic show through on Windows 11.
    // On older systems this hint is a no-op and we fall back to opaque rendering.
    glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);
    // Borderless: no OS title bar — we draw our own caption + window controls.
    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);

    GLFWwindow* window = glfwCreateWindow(
        width, height,
        "MWTools",
        nullptr, nullptr);
    if (!window)
        return Result<void>::Err("glfwCreateWindow failed");

    glfwWindow_ = window;

#ifdef _WIN32
    // Apply dark chrome immediately — prevents the white title-bar flash before
    // VanGui is set up and ToggleTheme runs the full ApplyTheme call.
    ApplyDarkTitleBar(true);

    {
        HWND hwnd = glfwGetWin32Window(glfwWindow_);

        // ── DWM Mica backdrop (Windows 11 build 22000+) ──────────────────────
        // DWMWA_SYSTEMBACKDROP_TYPE = 38, DWMSBT_MAINWINDOW = 2
        // DwmExtendFrameIntoClientArea lets GL render over the DWM compositor.
        if (hwnd) {
            DWORD backdropType = 2;
            if (SUCCEEDED(DwmSetWindowAttribute(hwnd, 38, &backdropType,
                                                sizeof(backdropType)))) {
                MARGINS m{ -1, -1, -1, -1 };
                DwmExtendFrameIntoClientArea(hwnd, &m);
                micaActive_ = true;
                LOG_INFO("DWM Mica backdrop enabled");
            }

            // Re-apply now that the backdrop type is set: DWMWA_CAPTION_COLOR
            // must be set after DWMWA_SYSTEMBACKDROP_TYPE to actually take over
            // the Mica caption strip's colour (see ApplyDarkTitleBar()).
            ApplyDarkTitleBar(true);
        }

        // ── Window icon (WIC path — reliable across all Windows versions) ────
        // We decode the embedded PNG (RCDATA 1) with WIC, then:
        //   a) Pass to glfwSetWindowIcon  (window chrome + ALT+TAB thumbnail)
        //   b) Build an HICON via CreateIconIndirect for WM_SETICON (taskbar)
        // This avoids the ICO file entirely and is immune to ICO format quirks.
        if (comInitialized_ && hwnd) {
            // ── Window / taskbar icon ────────────────────────────────────────
            // Load the real multi-resolution ICO straight from the embedded
            // ICON resource (id 1000 in resources.rc). Far more reliable than
            // hand-rolling an HICON from a decoded PNG — Windows picks the right
            // sub-image for the taskbar (32px) and small/Alt-Tab (16px).
            HMODULE hInst = GetModuleHandleW(nullptr);
            HICON hBig = static_cast<HICON>(LoadImageW(
                hInst, MAKEINTRESOURCEW(1000), IMAGE_ICON,
                GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON),
                LR_DEFAULTCOLOR));
            HICON hSml = static_cast<HICON>(LoadImageW(
                hInst, MAKEINTRESOURCEW(1000), IMAGE_ICON,
                GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON),
                LR_DEFAULTCOLOR));
            // Fallback: let the loader pick the default size from the group.
            if (!hBig) hBig = LoadIconW(hInst, MAKEINTRESOURCEW(1000));
            if (!hSml) hSml = hBig;
            // Set BOTH the window icons (WM_SETICON) and the window-class icons
            // (GCLP_HICON). The taskbar button reads the class/large icon, so
            // setting only WM_SETICON can leave a generic taskbar tile.
            if (hBig) {
                SendMessageW(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hBig);
                SetClassLongPtrW(hwnd, GCLP_HICON, (LONG_PTR)hBig);
                LOG_INFO("App icon set from embedded ICO (resource 1000)");
            } else {
                LOG_WARN("Could not load embedded app icon (resource 1000)");
            }
            if (hSml) {
                SendMessageW(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hSml);
                SetClassLongPtrW(hwnd, GCLP_HICONSM, (LONG_PTR)hSml);
            }

            // ── Custom cursor (resource ID 2 = mouse.png) ──────────────────
            std::vector<uint8_t> cursorPx;
            int cursorW = 0, cursorH = 0;
            if (LoadPNGResource(2, cursorPx, cursorW, cursorH)) {
                GLFWimage cimg{ cursorW, cursorH, cursorPx.data() };
                customCursor_ = glfwCreateCursor(&cimg, 0, 0);
                if (customCursor_) {
                    glfwSetCursor(glfwWindow_, customCursor_);
                    LOG_INFO("Custom cursor set ({}x{})", cursorW, cursorH);
                }
            } else {
                LOG_WARN("Could not load embedded cursor (resource 2)");
            }
        }

        // ── Borderless hit-testing ───────────────────────────────────────────
        // A GLFW undecorated window is a bare WS_POPUP with no sizing frame, so
        // Windows ignores the resize hit-test codes. Add WS_THICKFRAME (+ min/max
        // box for Aero Snap) back, then strip the frame's visuals in WM_NCCALCSIZE.
        // Subclass the window proc so WM_NCHITTEST gives native caption drag +
        // edge resize even though the window shows no OS chrome.
        if (hwnd) {
            LONG_PTR style = GetWindowLongPtrW(hwnd, GWL_STYLE);
            style |= WS_THICKFRAME | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_CAPTION;
            SetWindowLongPtrW(hwnd, GWL_STYLE, style);

            g_glfwWndProc = reinterpret_cast<WNDPROC>(
                GetWindowLongPtrW(hwnd, GWLP_WNDPROC));
            SetWindowLongPtrW(hwnd, GWLP_WNDPROC,
                              reinterpret_cast<LONG_PTR>(AppShellWndProc));

            // Re-apply frame so WM_NCCALCSIZE runs and the new style takes effect.
            SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
                         SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE |
                         SWP_NOZORDER | SWP_NOACTIVATE);
        }
    }
#endif

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        return Result<void>::Err("gladLoadGLLoader failed");

    (void)VANGUI_CHECKVERSION();
    (void)VanGui::CreateContext();
    VanGui::InitThreadPool();   // shared worker pool (replaces TaskQueue thread-per-task)
    VanGuiIO& io = VanGui::GetIO();
    // Disable vangui.ini — the layout is fixed/custom, so there's nothing to
    // persist, and we don't want a stray settings file appearing next to the exe.
    io.IniFilename = nullptr;
    // No docking — clean single-window layout driven by nav sidebar.
    io.ConfigFlags |= VanGuiConfigFlags_NavEnableKeyboard;
    // Prevent VanGui from resetting our custom GLFW cursor every frame.
    io.ConfigFlags |= VanGuiConfigFlags_NoMouseCursorChange;

    float dpiScale = 1.0f;
    {
        float xs, ys;
        glfwGetWindowContentScale(window, &xs, &ys);
        dpiScale = xs > 0 ? xs : 1.0f;
    }

    // ── Signals (Phase 7): downstream reaction to a freshly loaded BUN ────────
    onBunLoadedConn_ = onBunLoaded_.connect_queued(
        [this](const std::filesystem::path& p) {
            statusMessage_ = "Loaded " + p.filename().string();
        });

    LoadFont(dpiScale);
    ApplyTheme(dpiScale);
    ApplyDarkTitleBar(themeMode_ == ThemeMode::Dark);

    VanGui_ImplGlfw_InitForOpenGL(window, true);
    VanGui_ImplOpenGL3_Init("#version 330");

    glfwSetWindowUserPointer(window, this);
    glfwSetDropCallback(window, &AppShell::DropCallback);

    recentFiles_.Load();
    recentFiles_.PruneMissing();
    uiSounds_.Init();

    if (auto r = exportPanel_.Init(); !r)
        LOG_WARN("ObjectExportPanel shader init failed: {} (preview disabled)", r.error);
    else
        exportPanelReady_ = true;

    uiHudPanel_.Init(&fileDialog_);

    if (auto r = carPanel_.Init(); !r)
        LOG_WARN("CarPanel init failed: {} (car viewer disabled)", r.error);
    else
        carPanelReady_ = true;

    if (auto r = nisPanel_.Init(); !r)
        LOG_WARN("NisPanel shader init failed: {} (skeleton viewer disabled)", r.error);

    RegisterShortcutsAndCommands();   // Phase 5: shortcuts + command palette

    LOG_INFO("MWTools initialised ({}x{}, dpi={})", width, height, dpiScale);
    return Result<void>::Ok();
}
#endif // !defined(__ANDROID__)

// ─── Nav sidebar button ──────────────────────────────────────────────────────
//
// Each button is a full-width selectable showing the section name. The active
// section gets an accent-coloured left border rendered as a filled rect before
// the button.

bool AppShell::NavButton(const char* label, NavSection section, float width) {
    const bool active = (activeSection_ == section);
    const VanVec2 btnSize(width, 48.0f);

    // Accent bar for the active item (3 px wide left stripe).
    if (active) {
        const VanVec2 pos = VanGui::GetCursorScreenPos();
        VanDrawList* dl   = VanGui::GetWindowDrawList();
        dl->AddRectFilled(pos,
                          VanVec2(pos.x + 3.0f, pos.y + btnSize.y),
                          VanGui::ColorConvertFloat4ToU32(g_accent));
    }

    // Dim text = 60% of the base text brightness (adapts to dark/light).
    const VanVec4 dimText(g_text.x * 0.60f, g_text.y * 0.60f, g_text.z * 0.60f, 1.0f);

    if (active) {
        VanGui::PushStyleColor(VanGuiCol_Button,        g_bg2);
        VanGui::PushStyleColor(VanGuiCol_ButtonHovered, g_bg2);
        VanGui::PushStyleColor(VanGuiCol_ButtonActive,  g_bg2);
        VanGui::PushStyleColor(VanGuiCol_Text,          g_accent);
    } else {
        VanGui::PushStyleColor(VanGuiCol_Button,        VanVec4(0, 0, 0, 0));
        VanGui::PushStyleColor(VanGuiCol_ButtonHovered, VanVec4(g_bg2.x, g_bg2.y, g_bg2.z, 0.60f));
        VanGui::PushStyleColor(VanGuiCol_ButtonActive,  g_bg3);
        VanGui::PushStyleColor(VanGuiCol_Text,          dimText);
    }

    VanGui::PushStyleVar(VanGuiStyleVar_FrameRounding, 0.0f);
    VanGui::PushStyleVar(VanGuiStyleVar_ButtonTextAlign, VanVec2(0.5f, 0.5f));

    const bool clicked = VanGui::Button(label, btnSize);

    VanGui::PopStyleVar(2);
    VanGui::PopStyleColor(4);

    return clicked;
}

// ─── Nav sidebar ─────────────────────────────────────────────────────────────

void AppShell::DrawNavSidebar(float sidebarTop, float height) {
    VanGuiViewport* vp = VanGui::GetMainViewport();

    VanGui::SetNextWindowPos(VanVec2(vp->Pos.x, vp->Pos.y + sidebarTop));
    VanGui::SetNextWindowSize(VanVec2(kNavWidth, height));
    VanGui::SetNextWindowBgAlpha(1.0f);

    constexpr VanGuiWindowFlags flags =
        VanGuiWindowFlags_NoTitleBar    | VanGuiWindowFlags_NoResize  |
        VanGuiWindowFlags_NoMove        | VanGuiWindowFlags_NoScrollbar|
        VanGuiWindowFlags_NoSavedSettings | VanGuiWindowFlags_NoBringToFrontOnFocus;

    VanGui::PushStyleVar(VanGuiStyleVar_WindowPadding,  VanVec2(0, 0));
    VanGui::PushStyleVar(VanGuiStyleVar_WindowRounding, 0.0f);
    // Slightly different from bg0 so the sidebar reads as a distinct surface.
    VanGui::PushStyleColor(VanGuiCol_WindowBg, g_bg1);

    (void)VanGui::Begin("##NavSidebar", nullptr, flags);
    VanGui::PopStyleVar(2);
    VanGui::PopStyleColor(1);

    const VanU32 sepCol = VanGui::ColorConvertFloat4ToU32(
        VanVec4(g_border.x, g_border.y, g_border.z, 0.78f));

    // App logo / title at the top.
    VanGui::SetCursorPosY(14.0f);
    VanGui::PushStyleColor(VanGuiCol_Text, g_text);
    const float logoW = VanGui::CalcTextSize("MW").x;
    VanGui::SetCursorPosX((kNavWidth - logoW) * 0.5f);
    VanGui::TextUnformatted("MW");
    const float subtitleW = VanGui::CalcTextSize("Tools").x;
    VanGui::SetCursorPosX((kNavWidth - subtitleW) * 0.5f);
    VanGui::PushStyleColor(VanGuiCol_Text, g_accent);
    VanGui::TextUnformatted("Tools");
    VanGui::PopStyleColor(2);

    VanGui::SetCursorPosY(80.0f);

    // Hairline separator between logo and nav items.
    {
        VanDrawList* dl  = VanGui::GetWindowDrawList();
        VanVec2 scrPos   = VanGui::GetCursorScreenPos();
        dl->AddLine(VanVec2(scrPos.x + 12.0f, scrPos.y),
                    VanVec2(scrPos.x + kNavWidth - 12.0f, scrPos.y),
                    sepCol);
    }
    VanGui::Dummy(VanVec2(kNavWidth, 8.0f));

    // ── Nav items ────────────────────────────────────────────────────────────
    struct { const char* label; NavSection section; } items[] = {
        { "Textures",  NavSection::Textures   },
        { "Objects",   NavSection::Objects    },
        { "Cars",      NavSection::Cars       },
        { "Effects",   NavSection::Effects    },
        { "Audio",     NavSection::Audio      },
        { "Videos",    NavSection::Videos     },
        { "Cutscenes", NavSection::Cutscenes  },
        { "UI / HUD",  NavSection::UIHud      },
        { "Minimap",   NavSection::Minimap    },
    };

    for (auto& item : items) {
        if (NavButton(item.label, item.section, kNavWidth)) {
            if (activeSection_ != item.section)
                uiSounds_.Play(UISounds::Nav);
            activeSection_ = item.section;
        }
        VanGui::Dummy(VanVec2(0, 2.0f));
    }

    // ── Bottom: TeamVanilla credit + File shortcut ───────────────────────────
    // TeamVanilla credit — subtle, centred above the separator.
    {
        VanGui::SetCursorPosY(height - 76.0f);
        VanDrawList* dl  = VanGui::GetWindowDrawList();
        VanVec2 scrPos   = VanGui::GetCursorScreenPos();
        dl->AddLine(VanVec2(scrPos.x + 12.0f, scrPos.y),
                    VanVec2(scrPos.x + kNavWidth - 12.0f, scrPos.y),
                    sepCol);
    }
    VanGui::Dummy(VanVec2(kNavWidth, 4.0f));
    {
        const VanVec4 creditCol(g_text.x, g_text.y, g_text.z, 0.38f);
        VanGui::PushStyleColor(VanGuiCol_Text, creditCol);
        const float tvW = VanGui::CalcTextSize("by TeamVanilla").x;
        VanGui::SetCursorPosX((kNavWidth - tvW) * 0.5f);
        VanGui::TextUnformatted("by TeamVanilla");
        VanGui::PopStyleColor();
    }

    VanGui::End();
}

// ─── Content area routing ─────────────────────────────────────────────────────

void AppShell::DrawContentArea(float x, float y, float w, float h) {
    VanGuiViewport* vp = VanGui::GetMainViewport();

    VanGui::SetNextWindowPos(VanVec2(vp->Pos.x + x, vp->Pos.y + y));
    VanGui::SetNextWindowSize(VanVec2(w, h));

    constexpr VanGuiWindowFlags flags =
        VanGuiWindowFlags_NoTitleBar    | VanGuiWindowFlags_NoResize  |
        VanGuiWindowFlags_NoMove        | VanGuiWindowFlags_NoCollapse |
        VanGuiWindowFlags_NoSavedSettings | VanGuiWindowFlags_NoBringToFrontOnFocus;

    VanGui::PushStyleVar(VanGuiStyleVar_WindowPadding,  VanVec2(0, 0));
    VanGui::PushStyleVar(VanGuiStyleVar_WindowRounding, 0.0f);

    (void)VanGui::Begin("##ContentArea", nullptr, flags);
    VanGui::PopStyleVar(2);

    // Inner padding for content panels.
    VanGui::SetCursorPos(VanVec2(8, 8));
    const float innerW = w - 16.0f;
    const float innerH = h - 16.0f;

    switch (activeSection_) {
        case NavSection::Textures:  DrawTexturesContent(innerW, innerH);  break;
        case NavSection::Objects:   DrawObjectsContent(innerW, innerH);   break;
        case NavSection::Cars:      DrawCarsContent(innerW, innerH);      break;
        case NavSection::Effects:   DrawEffectsContent(innerW, innerH);   break;
        case NavSection::Audio:     DrawAudioContent(innerW, innerH);     break;
        case NavSection::Videos:    DrawVideosContent(innerW, innerH);    break;
        case NavSection::Cutscenes: DrawCutscenesContent(innerW, innerH); break;
        case NavSection::UIHud:     DrawUIHudContent(innerW, innerH);     break;
        case NavSection::Minimap:   DrawMinimapContent(innerW, innerH);   break;
    }

    VanGui::End();
}

// ─── Section: Textures ───────────────────────────────────────────────────────
//
// Left half: asset tree  |  Right half: DDS preview
// Split is a fixed 40/60 child layout — no docking required.

void AppShell::DrawTexturesContent(float w, float h) {
    texPanel_.SetTaskQueue(taskQueue_);

    // ── Quick-open button row ─────────────────────────────────────────────────
    if (VanGui::Button("Open BUN/BIN…")) {
        fileDialog_.Show("Open Textures (BUN/BIN)", FileDialog::Mode::Open,
                          {".bun", ".bin"},
                          [this](const std::filesystem::path& p) { OnOpenBUN(p); });
    }
    VanGui::SameLine();
    const bool canRevert = texPanel_.CanRevertSelectedFile();
    if (!canRevert) VanGui::BeginDisabled();
    if (VanGui::Button("Revert Patch")) {
        texPanel_.RevertSelectedFile(taskQueue_);
        VanGui::NotifyInfo("%s", std::string("Reverting last patch…").c_str());
    }
    if (!canRevert) VanGui::EndDisabled();

    VanGui::SameLine();
    const bool hasTexFiles = texPanel_.HasFiles();
    if (!hasTexFiles) VanGui::BeginDisabled();
    if (VanGui::Button("Close All")) {
        texPanel_.CloseAll();
        VanGui::NotifyInfo("%s", std::string("Closed all texture files.").c_str());
    }
    if (!hasTexFiles) VanGui::EndDisabled();

    VanGui::Spacing();

    // ── Two-column layout ─────────────────────────────────────────────────────
    const float treeW    = w * 0.40f;
    const float previewW = w - treeW - 8.0f;

    if (VanGui::BeginChild("##TexTree", VanVec2(treeW, h - 46.0f), false)) {
        texPanel_.DrawTree();
    }
    VanGui::EndChild();

    VanGui::SameLine(0, 8);

    if (VanGui::BeginChild("##TexPreview", VanVec2(previewW, h - 46.0f), false)) {
        texPanel_.DrawPreview();
    }
    VanGui::EndChild();
}

// ─── Section: Objects ────────────────────────────────────────────────────────

void AppShell::DrawObjectsContent(float w, float h) {
    if (VanGui::Button("Open BUN/BIN for 3D Export…")) {
        fileDialog_.Show("Open for 3D Export (BUN/BIN)", FileDialog::Mode::Open,
                          {".bun", ".bin"},
                          [this](const std::filesystem::path& p) { OnOpenBUNForExport(p); });
    }
    VanGui::SameLine();
    // Import / replace the selected object's geometry (full rebuild).
    const bool canReplace = exportPanel_.HasSelectedObject();
    if (!canReplace) VanGui::BeginDisabled();
    if (VanGui::Button("Import / Replace Object…")) {
        fileDialog_.Show("Import mesh (OBJ / glTF)", FileDialog::Mode::Open,
                          {".obj", ".glb", ".gltf"},
                          [this](const std::filesystem::path& p) {
                              exportPanel_.ImportReplaceSelected(p, taskQueue_);
                          });
    }
    if (!canReplace) VanGui::EndDisabled();
    if (!canReplace && VanGui::IsItemHovered(VanGuiHoveredFlags_AllowWhenDisabled))
        VanGui::SetTooltip("Select an object in the tree first, then import a mesh to replace it.");

    VanGui::SameLine();
    const bool hasObjFile = exportPanel_.HasContent();
    if (!hasObjFile) VanGui::BeginDisabled();
    if (VanGui::Button("Close File")) {
        exportPanel_.CloseFile();
        VanGui::NotifyInfo("%s", std::string("Closed the loaded object file.").c_str());
    }
    if (!hasObjFile) VanGui::EndDisabled();
    VanGui::Spacing();

    if (VanGui::BeginChild("##ObjPanel", VanVec2(w, h - 46.0f), false)) {
        exportPanel_.Draw(taskQueue_);
    }
    VanGui::EndChild();
}

// ─── Section: Cars ───────────────────────────────────────────────────────────
//
// Browse the game's CARS/ directory, then inspect the selected car across tabs:
// Viewer | Mesh | Textures | Vinyls | Performance | Pursuit | Engine Audio.

void AppShell::DrawCarsContent(float w, float h) {
    if (VanGui::Button("Open CARS Folder\xe2\x80\xa6")) {
        fileDialog_.Show("Select the game's CARS folder", FileDialog::Mode::Folder, {},
                          [this](const std::filesystem::path& p) { OnOpenCars(p); });
    }
    VanGui::SameLine();
    VanGui::TextDisabled("Pick the CARS directory (contains per-car sub-folders).");
    VanGui::Spacing();

    if (!carPanelReady_) {
        VanGui::TextColored(VanVec4(1, 0.4f, 0.4f, 1),
                           "Car viewer unavailable (shader init failed).");
    }

    if (VanGui::BeginChild("##CarPanel", VanVec2(w, h - 46.0f), false))
        carPanel_.Draw(taskQueue_);
    VanGui::EndChild();
}

// ─── Section: Effects ────────────────────────────────────────────────────────
// Browse & edit the game's particle effects (attributes.bin EmitterData) with a
// live looping preview.

void AppShell::DrawEffectsContent(float w, float h) {
    if (VanGui::BeginChild("##EffectsPanel", VanVec2(w, h), false))
        effectsPanel_.Draw();
    VanGui::EndChild();
}

// ─── Section: Audio ──────────────────────────────────────────────────────────
//
// Two sub-tabs (Sound Banks | Music) under the single "Audio" nav item.
// Sound Banks covers both ABK (sound effects) and GIN (engine RPM audio)
// since they share the same SoundBankPanel browser/preview layout.

void AppShell::DrawAudioContent(float w, float h) {
    VanGui::PushStyleVar(VanGuiStyleVar_FramePadding, VanVec2(12, 5));
    if (VanGui::BeginTabBar("##AudioSubTabs")) {
        if (VanGui::BeginTabItem("Sound Banks")) {
            audioSubTab_ = AudioSubTab::SoundBank;
            VanGui::EndTabItem();
        }
        if (VanGui::BeginTabItem("Music")) {
            audioSubTab_ = AudioSubTab::Music;
            VanGui::EndTabItem();
        }
        VanGui::EndTabBar();
    }
    VanGui::PopStyleVar();

    VanGui::Spacing();

    const float panelH = h - 65.0f;

    if (audioSubTab_ == AudioSubTab::SoundBank) {
        if (VanGui::Button("Open Sound Bank (ABK)\xe2\x80\xa6")) {
            fileDialog_.Show("Open ABK Sound Bank", FileDialog::Mode::Open,
                              {".abk", ".ABK"},
                              [this](const std::filesystem::path& p) { OnOpenABK(p); });
        }
        VanGui::SameLine();
        if (VanGui::Button("Open Engine Sound (GIN)\xe2\x80\xa6")) {
            fileDialog_.Show("Open GIN Engine Audio", FileDialog::Mode::Open,
                              {".gin", ".GIN"},
                              [this](const std::filesystem::path& p) { OnOpenGIN(p); });
        }
        VanGui::Spacing();

        const float browserW = w * 0.45f;
        const float previewW = w - browserW - 8.0f;

        if (VanGui::BeginChild("##SBBrowser", VanVec2(browserW, panelH), false))
            soundBankPanel_.DrawBrowser();
        VanGui::EndChild();
        VanGui::SameLine(0, 8);
        if (VanGui::BeginChild("##SBPreview", VanVec2(previewW, panelH), false))
            soundBankPanel_.DrawPreview(taskQueue_);
        VanGui::EndChild();

    } else { // Music
        if (VanGui::Button("Open Music / Stream (MPF/MUS/BIG)\xe2\x80\xa6")) {
            fileDialog_.Show("Open Music or Stream (MPF / MUS / BIG)", FileDialog::Mode::Open,
                              {".mpf", ".MPF", ".mus", ".MUS", ".big", ".BIG"},
                              [this](const std::filesystem::path& p) { OnOpenMPF(p, {}); });
        }
        VanGui::Spacing();

        const float browserW = w * 0.45f;
        const float previewW = w - browserW - 8.0f;

        if (VanGui::BeginChild("##MusBrowser", VanVec2(browserW, panelH), false))
            musicPanel_.DrawBrowser();
        VanGui::EndChild();
        VanGui::SameLine(0, 8);
        if (VanGui::BeginChild("##MusPreview", VanVec2(previewW, panelH), false))
            musicPanel_.DrawPreview(taskQueue_);
        VanGui::EndChild();
    }
}

// ─── Section: Videos ─────────────────────────────────────────────────────────
//
// Browse + play NFSMW movie files (MOVIES/*.vp6 — On2 VP6 video + EA audio),
// export a raw copy, or replace one with a backup/revert.

void AppShell::DrawVideosContent(float w, float h) {
    videoPanel_.SetFileDialog(&fileDialog_);

    if (VanGui::Button("Open Video (VP6)\xe2\x80\xa6")) {
        fileDialog_.Show("Open Video (VP6)", FileDialog::Mode::Open,
                          {".vp6", ".VP6"},
                          [this](const std::filesystem::path& p) { OnOpenVideo(p); });
    }
    VanGui::Spacing();

    const float panelH  = h - 46.0f;
    const float browserW = w * 0.30f;
    const float previewW = w - browserW - 8.0f;

    if (VanGui::BeginChild("##VidBrowser", VanVec2(browserW, panelH), false))
        videoPanel_.DrawBrowser();
    VanGui::EndChild();
    VanGui::SameLine(0, 8);
    if (VanGui::BeginChild("##VidPreview", VanVec2(previewW, panelH), false))
        videoPanel_.DrawPreview(taskQueue_);
    VanGui::EndChild();
}

// ─── Section: Cutscenes (NIS) ────────────────────────────────────────────────
//
// Full NIS cutscene editor: open NIS/Scene_*_BundleB.bun, browse EAGLAnim
// clips (skeleton + bone list), export raw chunk payloads for external editing
// (e.g. icebreaker), import edited payloads back, and save the modified BUN.

void AppShell::DrawCutscenesContent(float w, float h) {
    if (VanGui::Button("Open NIS Cutscene (BUN)\xe2\x80\xa6")) {
        fileDialog_.Show("Open NIS Cutscene", FileDialog::Mode::Open,
                          {".bun", ".BUN"},
                          [this](const std::filesystem::path& p) { OnOpenNis(p); });
    }
    if (nisPanel_.IsLoaded()) {
        VanGui::SameLine();
        if (VanGui::Button("Close")) nisPanel_.Close();
    }
    VanGui::Spacing();

    const float panelH = h - 46.0f;
    if (VanGui::BeginChild("##NisPanel", VanVec2(w, panelH), false))
        nisPanel_.Draw(w, panelH, fileDialog_, taskQueue_);
    VanGui::EndChild();
}

// ─── Section: UI / HUD ───────────────────────────────────────────────────────
//
// Frontend menu textures (FRONTEND/*.BUN), the HUD atlas (GLOBALHUD.BUN), and
// the in-game fonts (GLOBALB.LZC). UIHudPanel routes the opened file to the
// matching sub-tab automatically.

void AppShell::DrawUIHudContent(float w, float h) {
    if (VanGui::Button("Open Frontend / HUD (BUN)\xe2\x80\xa6")) {
        fileDialog_.Show("Open Frontend or HUD (BUN/BIN)", FileDialog::Mode::Open,
                          {".bun", ".bin"},
                          [this](const std::filesystem::path& p) { OnOpenUIHud(p); });
    }
    VanGui::SameLine();
    if (VanGui::Button("Open Fonts (LZC)\xe2\x80\xa6")) {
        fileDialog_.Show("Open Font Sheets (GLOBALB.LZC)", FileDialog::Mode::Open,
                          {".lzc", ".LZC"},
                          [this](const std::filesystem::path& p) { OnOpenUIHud(p); });
    }
    VanGui::Spacing();

    if (VanGui::BeginChild("##UIHudPanel", VanVec2(w, h - 46.0f), false))
        uiHudPanel_.Draw(taskQueue_);
    VanGui::EndChild();
}

// ─── Custom window controls (borderless title bar) ───────────────────────────

bool AppShell::WindowButton(const char* id, WinBtn type, float w, float h) {
    const VanVec2 p0 = VanGui::GetCursorScreenPos();
    const bool clicked = VanGui::InvisibleButton(id, VanVec2(w, h));
    const bool hovered = VanGui::IsItemHovered();
    const VanVec2 p1(p0.x + w, p0.y + h);
    VanDrawList* dl = VanGui::GetWindowDrawList();

    if (hovered) {
        const VanU32 bg = (type == WinBtn::Close)
            ? VAN_COL32(232, 17, 35, 255)                       // Windows-style red
            : VanGui::ColorConvertFloat4ToU32(g_bg3);
        dl->AddRectFilled(p0, p1, bg);
    }

    const VanU32 col = (hovered && type == WinBtn::Close)
        ? VAN_COL32(255, 255, 255, 255)
        : VanGui::ColorConvertFloat4ToU32(g_text);
    const VanVec2 c((p0.x + p1.x) * 0.5f, (p0.y + p1.y) * 0.5f);
    const float s = 5.0f;        // glyph half-extent
    const float t = 1.3f;        // stroke thickness

    switch (type) {
        case WinBtn::Minimize:
            dl->AddLine(VanVec2(c.x - s, c.y), VanVec2(c.x + s, c.y), col, t);
            break;
        case WinBtn::Maximize:
            dl->AddRect(VanVec2(c.x - s, c.y - s), VanVec2(c.x + s, c.y + s), col, 0, 0, t);
            break;
        case WinBtn::Restore:
            dl->AddRect(VanVec2(c.x - s + 2, c.y - s), VanVec2(c.x + s, c.y + s - 2), col, 0, 0, t);
            dl->AddRectFilled(VanVec2(c.x - s, c.y - s + 2), VanVec2(c.x + s - 2, c.y + s),
                              VanGui::ColorConvertFloat4ToU32(g_bg1));
            dl->AddRect(VanVec2(c.x - s, c.y - s + 2), VanVec2(c.x + s - 2, c.y + s), col, 0, 0, t);
            break;
        case WinBtn::Close:
            dl->AddLine(VanVec2(c.x - s, c.y - s), VanVec2(c.x + s, c.y + s), col, t);
            dl->AddLine(VanVec2(c.x - s, c.y + s), VanVec2(c.x + s, c.y - s), col, t);
            break;
    }
    return clicked;
}

void AppShell::DrawWindowControls() {
#if defined(__ANDROID__)
    // Android has no desktop window chrome. Activity lifecycle/back navigation
    // owns minimize/close behavior, so the custom desktop buttons are hidden.
    return;
#else
    const float barW = VanGui::GetWindowWidth();
    const float barH = VanGui::GetWindowHeight();
    const VanVec2 barPos = VanGui::GetWindowPos();
    const float bw = 46.0f;

    const bool maxed = glfwGetWindowAttrib(glfwWindow_, GLFW_MAXIMIZED) != 0;

    VanGui::PushStyleVar(VanGuiStyleVar_ItemSpacing, VanVec2(0, 0));

    VanGui::SetCursorScreenPos(VanVec2(barPos.x + barW - bw * 3.0f, barPos.y));
    if (WindowButton("##min", WinBtn::Minimize, bw, barH))
        glfwIconifyWindow(glfwWindow_);

    VanGui::SetCursorScreenPos(VanVec2(barPos.x + barW - bw * 2.0f, barPos.y));
    if (WindowButton("##max", maxed ? WinBtn::Restore : WinBtn::Maximize, bw, barH)) {
        if (maxed) glfwRestoreWindow(glfwWindow_);
        else       glfwMaximizeWindow(glfwWindow_);
    }

    VanGui::SetCursorScreenPos(VanVec2(barPos.x + barW - bw, barPos.y));
    if (WindowButton("##close", WinBtn::Close, bw, barH))
        glfwSetWindowShouldClose(glfwWindow_, GLFW_TRUE);

    VanGui::PopStyleVar();
#endif
}

// ─── Section: Minimap ────────────────────────────────────────────────────────

void AppShell::DrawMinimapContent(float w, float h) {
    // ── Toolbar / file-open strip at the very top ─────────────────────────
    if (VanGui::Button("Open Minimap (MINI_MAP.BIN)\xe2\x80\xa6")) {
        fileDialog_.Show("Open Minimap", FileDialog::Mode::Open,
                         { "*.BIN", "*.bin" },
                         [this](const std::filesystem::path& p) {
                             OnOpenMinimap(p);
                         });
    }

    if (minimapPanel_.IsLoaded()) {
        VanGui::SameLine();
        if (VanGui::Button("Close")) {
            minimapPanel_.Close();
            statusMessage_ = "Ready";
        }
    }

    VanGui::Dummy(VanVec2(0.0f, 4.0f));

    // ── Delegate all further drawing to the panel ─────────────────────────
    minimapPanel_.Draw(w, h - 40.0f, fileDialog_, taskQueue_);
}

// ─── Title bar (menus + window controls) ──────────────────────────────────────

void AppShell::DrawMenuBar() {
    // BeginMainMenuBar() positions itself from the viewport's WORK rect
    // (Pos + WorkInsetMin), and registers its own height into WorkInsetMin
    // for the *next* frame (vendor/VanGUI/vangui_widgets.cpp, BeginViewportSideBar).
    // Since nothing else in this app reads/contributes to that inset, frame 2
    // onward it reads back its OWN previous-frame height as an inset and
    // re-positions itself that far down — a self-feedback loop that pushes the
    // whole menu bar (and everything in it) down by its own height forever,
    // leaving an empty gap above where nothing is drawn. Reset the work rect to
    // the full viewport every frame so the menu bar always anchors at Pos (0,0).
    VanGuiViewport* mainVp = VanGui::GetMainViewport();
    mainVp->WorkPos  = mainVp->Pos;
    mainVp->WorkSize = mainVp->Size;

    if (VanGui::BeginMainMenuBar()) {
#ifdef _WIN32
        // Feed the Win32 hit-test: bar height = caption drag zone; track whether
        // a title-bar widget is under the cursor so dragging doesn't eat clicks.
        g_captionHeight = VanGui::GetWindowHeight();
#endif
        if (VanGui::BeginMenu("File")) {
            if (VanGui::BeginMenu("Open Recent")) {
                DrawRecentSubmenu("Textures",     RecentKind::Texture,
                                  [this](const std::filesystem::path& p) { OnOpenBUN(p); });
                DrawRecentSubmenu("3D Geometry",  RecentKind::Export,
                                  [this](const std::filesystem::path& p) { OnOpenBUNForExport(p); });
                DrawRecentSubmenu("Sound Banks",  RecentKind::ABK,
                                  [this](const std::filesystem::path& p) { OnOpenABK(p); });
                DrawRecentSubmenu("Engine Sound", RecentKind::GIN,
                                  [this](const std::filesystem::path& p) { OnOpenGIN(p); });
                DrawRecentSubmenu("Music",        RecentKind::MPF,
                                  [this](const std::filesystem::path& p) { OnOpenMPF(p, {}); });
                DrawRecentSubmenu("Videos",       RecentKind::Video,
                                  [this](const std::filesystem::path& p) { OnOpenVideo(p); });
                DrawRecentSubmenu("UI / HUD",     RecentKind::UIHud,
                                  [this](const std::filesystem::path& p) { OnOpenUIHud(p); });
                DrawRecentSubmenu("Minimap",      RecentKind::Minimap,
                                  [this](const std::filesystem::path& p) { OnOpenMinimap(p); });
                VanGui::EndMenu();
            }
            VanGui::Separator();
            if (VanGui::MenuItem("Exit")) {
#if defined(__ANDROID__)
                androidExitRequested_ = true;
#else
                glfwSetWindowShouldClose(glfwWindow_, GLFW_TRUE);
#endif
            }
            VanGui::EndMenu();
        }
        if (VanGui::BeginMenu("View")) {
            const bool isDark = (themeMode_ == ThemeMode::Dark);
            if (VanGui::MenuItem(isDark ? "Switch to Light Mode" : "Switch to Dark Mode"))
                ToggleTheme();
            VanGui::Separator();
            if (VanGui::MenuItem("Textures", nullptr, activeSection_ == NavSection::Textures))
                activeSection_ = NavSection::Textures;
            if (VanGui::MenuItem("Objects",  nullptr, activeSection_ == NavSection::Objects))
                activeSection_ = NavSection::Objects;
            if (VanGui::MenuItem("Audio",    nullptr, activeSection_ == NavSection::Audio))
                activeSection_ = NavSection::Audio;
            if (VanGui::MenuItem("Videos",   nullptr, activeSection_ == NavSection::Videos))
                activeSection_ = NavSection::Videos;
            if (VanGui::MenuItem("Minimap",  nullptr, activeSection_ == NavSection::Minimap))
                activeSection_ = NavSection::Minimap;
            VanGui::EndMenu();
        }
        if (VanGui::BeginMenu("Help")) {
            if (VanGui::MenuItem("Keyboard Shortcuts"))  g_showShortcuts = true;
            if (VanGui::MenuItem("About MWTools")) g_showAbout     = true;
            VanGui::EndMenu();
        }

        DrawWindowControls();

#ifdef _WIN32
        // A title-bar widget (menu or window button) is "interactive" when it's
        // hovered or a menu popup is open — Win32 then returns HTCLIENT there so
        // the click lands on the widget instead of starting a window drag.
        g_captionInteractive =
            VanGui::IsAnyItemHovered() ||
            VanGui::IsPopupOpen("", VanGuiPopupFlags_AnyPopupId | VanGuiPopupFlags_AnyPopupLevel);
#endif
        VanGui::EndMainMenuBar();
    }
    DrawAboutDialogs();
}

// ─── About / Shortcuts dialogs ────────────────────────────────────────────────

void AppShell::DrawAboutDialogs() {
    if (g_showAbout)     { VanGui::OpenPopup("About MWTools"); g_showAbout     = false; }
    if (g_showShortcuts) { VanGui::OpenPopup("Keyboard Shortcuts");  g_showShortcuts = false; }

    VanGui::SetNextWindowSize(VanVec2(440, 0), VanGuiCond_Appearing);
    if (VanGui::BeginPopupModal("About MWTools", nullptr,
                               VanGuiWindowFlags_NoResize | VanGuiWindowFlags_AlwaysAutoResize)) {
        VanGui::TextUnformatted("MWTools");
        VanGui::TextDisabled("Need for Speed: Most Wanted (2005) asset editor");
        VanGui::Separator();
        VanGui::TextWrapped(
            "Edit and export game assets:\n"
            "  \xe2\x80\xa2  Textures \xe2\x80\x94 TPK/BUN browse, preview, replace from DDS\n"
            "  \xe2\x80\xa2  Objects  \xe2\x80\x94 SolidObject preview + OBJ/glTF export\n"
            "  \xe2\x80\xa2  Audio    \xe2\x80\x94 Sound Banks (ABK), Engine Sound (GIN), Music (MUS)\n"
            "  \xe2\x80\xa2  Videos   \xe2\x80\x94 VP6 movie playback, export and replace\n"
            "  \xe2\x80\xa2  UI/HUD   \xe2\x80\x94 Frontend textures, HUD atlas, font sheets");
        VanGui::Separator();
        VanGui::Spacing();
        VanGui::TextColored(g_accent, "Created by TeamVanilla");
        VanGui::Spacing();
        VanGui::TextDisabled("Tip: drag a .BUN / .ABK / .GIN / .MUS onto the window to open it.");
        VanGui::Spacing();
        if (VanGui::Button("Close", VanVec2(120, 0))) VanGui::CloseCurrentPopup();
        VanGui::EndPopup();
    }

    VanGui::SetNextWindowSize(VanVec2(420, 0), VanGuiCond_Appearing);
    if (VanGui::BeginPopupModal("Keyboard Shortcuts", nullptr,
                               VanGuiWindowFlags_NoResize | VanGuiWindowFlags_AlwaysAutoResize)) {
        auto row = [](const char* k, const char* d) {
            VanGui::TextColored(VanGui::GetStyleColorVec4(VanGuiCol_SliderGrab), "%-12s", k);
            VanGui::SameLine(130); VanGui::TextUnformatted(d);
        };
        row("Ctrl+O", "Open Textures (BUN/BIN)");
        row("Ctrl+S", "Export selected texture as DDS");
        row("Ctrl+Z", "Revert last patch for the selected file");
        VanGui::Separator();
        VanGui::TextDisabled("Drag & drop .BUN/.BIN/.ABK/.GIN/.MUS to open.");
        VanGui::Spacing();
        if (VanGui::Button("Close", VanVec2(120, 0))) VanGui::CloseCurrentPopup();
        VanGui::EndPopup();
    }
}

// ─── Status bar ──────────────────────────────────────────────────────────────

void AppShell::DrawStatusBar() {
    VanGuiViewport* vp    = VanGui::GetMainViewport();
    const float height   = VanGui::GetFrameHeight() * 1.2f;

    VanGui::SetNextWindowPos(VanVec2(vp->Pos.x,
                                   vp->Pos.y + vp->Size.y - height));
    VanGui::SetNextWindowSize(VanVec2(vp->Size.x, height));

    constexpr VanGuiWindowFlags flags =
        VanGuiWindowFlags_NoTitleBar   | VanGuiWindowFlags_NoResize  |
        VanGuiWindowFlags_NoMove       | VanGuiWindowFlags_NoScrollbar|
        VanGuiWindowFlags_NoSavedSettings | VanGuiWindowFlags_NoBringToFrontOnFocus;

    if (VanGui::Begin("##statusbar", nullptr, flags)) {
        VanGui::Text("%s", statusMessage_.c_str());
    }
    VanGui::End();
}

// ─── Status message update ───────────────────────────────────────────────────

void AppShell::UpdateStatusMessage() {
    const std::string& panelErr = texPanel_.LastError();
    const std::string& audioErr = audioPanel_.LastError();

    if (!panelErr.empty())
        statusMessage_ = "Error: " + panelErr;
    else if (!audioErr.empty())
        statusMessage_ = "Audio error: " + audioErr;
    else if (audioPanel_.IsLoaded())
        statusMessage_ = "Audio bank loaded";
    else if (soundBankPanel_.IsLoaded())
        statusMessage_ = "Sound bank loaded";
    else if (musicPanel_.IsLoaded())
        statusMessage_ = "Music loaded";
    else if (videoPanel_.IsLoaded())
        statusMessage_ = "Video loaded";
    else if (nisPanel_.IsLoaded())
        statusMessage_ = nisPanel_.IsModified()
                         ? "NIS cutscene loaded (unsaved changes)"
                         : "NIS cutscene loaded";
    else if (!texPanel_.IsLoading() && texPanel_.TotalTextures() > 0)
        statusMessage_ = std::to_string(texPanel_.TotalTextures()) + " texture(s) loaded";
    else if (statusMessage_.empty() ||
             statusMessage_.find("...") == std::string::npos) {
        if (!texPanel_.IsLoading())
            statusMessage_ = "Ready \xe2\x80\x94 open a file from the sidebar or drag a .BUN / .ABK / .GIN / .MUS onto the window.";
    }
}

// ─── Main frame ──────────────────────────────────────────────────────────────

void AppShell::DrawFrame() {
    DrawMenuBar();

    VanGuiViewport* vp   = VanGui::GetMainViewport();
    const float menuH   = VanGui::GetFrameHeight(); // height of the menu bar
    const float statusH = VanGui::GetFrameHeight() * 1.2f;
    const float totalH  = vp->Size.y;

    const float sidebarTop = menuH;
    const float sidebarH   = totalH - menuH - statusH;

    DrawNavSidebar(sidebarTop, sidebarH);

    const float contentX = kNavWidth;
    const float contentW = vp->Size.x - kNavWidth;

    DrawContentArea(contentX, menuH, contentW, sidebarH);

    DrawStatusBar();
    UpdateStatusMessage();

    taskQueue_.PumpAndDraw();
}

// ─── Open handlers ────────────────────────────────────────────────────────────

void AppShell::OnOpenBUN(const std::filesystem::path& path) {
    statusMessage_ = "Loading " + path.filename().string() + "...";
    texPanel_.OpenFile(path, taskQueue_);
    RecordOpen(RecentKind::Texture, path, "Opened");
    activeSection_ = NavSection::Textures;
    uiSounds_.Play(UISounds::FileLoaded);
    onBunLoaded_.emit(path);   // (Phase 7) notify any connected slots
}

void AppShell::OnOpenBUNForExport(const std::filesystem::path& path) {
    statusMessage_ = "Loading " + path.filename().string() + " for export...";
    exportPanel_.LoadFileAsync(path, taskQueue_);
    RecordOpen(RecentKind::Export, path, "Opened for export");
    activeSection_ = NavSection::Objects;
    uiSounds_.Play(UISounds::FileLoaded);
}

void AppShell::OnOpenSNR(const std::filesystem::path& path) {
    statusMessage_ = "Loading audio bank " + path.filename().string() + "...";
    audioPanel_.SetFileDialog(&fileDialog_);
    audioPanel_.OpenSNR(path, taskQueue_);
    RecordOpen(RecentKind::Audio, path, "Opened audio");
    activeSection_ = NavSection::Audio;
    audioSubTab_   = AudioSubTab::SoundBank;
}

void AppShell::OnOpenABK(const std::filesystem::path& path) {
    statusMessage_ = "Loading sound bank " + path.filename().string() + "...";
    soundBankPanel_.SetFileDialog(&fileDialog_);
    soundBankPanel_.OpenABK(path, taskQueue_);
    RecordOpen(RecentKind::ABK, path, "Opened sound bank");
    activeSection_ = NavSection::Audio;
    audioSubTab_   = AudioSubTab::SoundBank;
}

void AppShell::OnOpenGIN(const std::filesystem::path& path) {
    statusMessage_ = "Loading engine audio " + path.filename().string() + "...";
    soundBankPanel_.SetFileDialog(&fileDialog_);
    soundBankPanel_.OpenGIN(path, taskQueue_);
    RecordOpen(RecentKind::GIN, path, "Opened engine audio");
    activeSection_ = NavSection::Audio;
    audioSubTab_   = AudioSubTab::SoundBank;
}

void AppShell::OnOpenMPF(const std::filesystem::path& mpfPath,
                          const std::filesystem::path& musPath) {
    statusMessage_ = "Loading audio " + mpfPath.filename().string() + "...";
    musicPanel_.SetFileDialog(&fileDialog_);

    // .mus and .big are themselves EA SCHl stream containers — open the file
    // directly as the MUS instead of looking for a companion .mus by stem.
    const std::string ext = ToLowerAscii(mpfPath.extension().string());
    const bool isStreamContainer = (ext == ".mus" || ext == ".big");
    musicPanel_.OpenMPF(mpfPath, isStreamContainer ? mpfPath : musPath, taskQueue_);

    RecordOpen(RecentKind::MPF, mpfPath, "Opened audio");
    activeSection_ = NavSection::Audio;
    audioSubTab_   = AudioSubTab::Music;
}

void AppShell::OnOpenVideo(const std::filesystem::path& path) {
    statusMessage_ = "Loading video " + path.filename().string() + "...";
    videoPanel_.SetFileDialog(&fileDialog_);
    videoPanel_.OpenFile(path, taskQueue_);
    RecordOpen(RecentKind::Video, path, "Opened video");
    activeSection_ = NavSection::Videos;
    uiSounds_.Play(UISounds::FileLoaded);
}

void AppShell::OnOpenNis(const std::filesystem::path& path) {
    statusMessage_ = "Loading NIS cutscene " + path.filename().string() + "...";
    activeSection_ = NavSection::Cutscenes;
    nisPanel_.Load(path, taskQueue_);
    uiSounds_.Play(UISounds::FileLoaded);
}

void AppShell::OnOpenUIHud(const std::filesystem::path& path) {
    statusMessage_ = "Loading UI/HUD " + path.filename().string() + "...";
    uiHudPanel_.Open(path, taskQueue_);
    RecordOpen(RecentKind::UIHud, path, "Opened UI/HUD");
    activeSection_ = NavSection::UIHud;
    uiSounds_.Play(UISounds::FileLoaded);
}

void AppShell::OnOpenMinimap(const std::filesystem::path& path) {
    statusMessage_ = "Loading " + path.filename().string() + "\xe2\x80\xa6";
    activeSection_ = NavSection::Minimap;
    minimapPanel_.Load(path, taskQueue_);
    RecordOpen(RecentKind::Minimap, path, "Opened");
}

void AppShell::OnOpenCars(const std::filesystem::path& path) {
    // The native dialog returns a file; accept a folder too. Derive the CARS
    // root either way (the directory that holds the per-car sub-folders).
    std::error_code ec;
    const std::filesystem::path root =
        std::filesystem::is_directory(path, ec) ? path : path.parent_path();
    statusMessage_ = "Scanning cars in " + root.filename().string() + "...";
    carPanel_.Open(root, taskQueue_);
    activeSection_ = NavSection::Cars;
    uiSounds_.Play(UISounds::FileLoaded);
}

// ─── Helpers ─────────────────────────────────────────────────────────────────

void AppShell::RecordOpen(RecentKind kind,
                           const std::filesystem::path& path,
                           const char* verb) {
    recentFiles_.Add(kind, path);
    recentFiles_.Save();
    VanGui::NotifyInfo("%s", std::string(std::string(verb) + " " + path.filename().string()).c_str());
}

void AppShell::DrawRecentSubmenu(
        const char* label, RecentKind kind,
        const std::function<void(const std::filesystem::path&)>& onPick) {
    const auto& entries = recentFiles_.Entries(kind);
    if (VanGui::BeginMenu(label, !entries.empty())) {
        // Two recents can share a filename (e.g. GEOMETRY.BIN from different
        // folders). Push a per-row ID so VanGui doesn't flag an ID conflict.
        int rowId = 0;
        for (const auto& p : entries) {
            VanGui::PushID(rowId++);
            if (VanGui::MenuItem(p.filename().string().c_str()))
                onPick(p);
            if (VanGui::IsItemHovered())
                VanGui::SetTooltip("%s", p.string().c_str());
            VanGui::PopID();
        }
        VanGui::Separator();
        if (VanGui::MenuItem("Clear Recent")) {
            recentFiles_.Clear(kind);
            recentFiles_.Save();
        }
        VanGui::EndMenu();
    }
}

void AppShell::OnFilesDropped(const std::vector<std::filesystem::path>& paths) {
    for (const auto& p : paths) {
        std::string ext = ToLowerAscii(p.extension().string());

        // MINI_MAP.BIN — route by filename before the generic BIN handler.
        if (ToLowerAscii(p.stem().string()) == "mini_map" &&
            ext == ".bin") {
            OnOpenMinimap(p);
            continue;
        }

        if (ext == ".bun" || ext == ".bin") {
            OnOpenBUN(p);
        } else if (ext == ".abk") {
            OnOpenABK(p);
        } else if (ext == ".gin") {
            OnOpenGIN(p);
        } else if (ext == ".mpf" || ext == ".mus" || ext == ".big") {
            OnOpenMPF(p, {});
        } else if (ext == ".vp6") {
            OnOpenVideo(p);
        } else if (ext == ".lzc") {
            OnOpenUIHud(p);
        } else if (ext == ".dds") {
            VanGui::NotifyInfo("%s", std::string("Drop a .dds onto a selected texture's "
                          "\"Replace from DDS\xe2\x80\xa6\" dialog to patch it.").c_str());
        } else {
            VanGui::NotifyWarning("%s", std::string("Unsupported file: " + p.filename().string()).c_str());
        }
    }
}

#if !defined(__ANDROID__)
void AppShell::DropCallback(GLFWwindow* window, int count, const char** paths) {
    auto* self = static_cast<AppShell*>(glfwGetWindowUserPointer(window));
    if (!self) return;
    std::vector<std::filesystem::path> dropped;
    dropped.reserve(static_cast<size_t>(count));
    for (int i = 0; i < count; ++i)
        dropped.emplace_back(paths[i]);
    self->OnFilesDropped(dropped);
}
#endif

// ─── Shortcut actions (Phase 5) ──────────────────────────────────────────────
// Each action runs on the main thread (dispatched by VanGui::DispatchShortcuts
// or selected from the command palette).

void AppShell::ActionOpenBUN() {
    fileDialog_.Show("Open BUN/BIN", FileDialog::Mode::Open,
                      {".bun", ".bin"},
                      [this](const std::filesystem::path& p) { OnOpenBUN(p); });
}

void AppShell::ActionExportSelectedTexture() {
    if (const Texture* sel = texPanel_.SelectedTexture()) {
        const std::string defName =
            (sel->name.empty() ? "texture" : sel->name) + ".dds";
        fileDialog_.Show("Export texture as DDS", FileDialog::Mode::Save,
                          {".dds"},
                          [this](const std::filesystem::path& p) {
                              texPanel_.ExportSelected(p);
                              VanGui::NotifyInfo("%s", std::string("Exported to " + p.filename().string()).c_str());
                          },
                          defName);
    } else {
        VanGui::NotifyInfo("%s", std::string("Select a texture first, then press Ctrl+S to export it as DDS.").c_str());
    }
}

void AppShell::ActionRevertSelected() {
    if (texPanel_.CanRevertSelectedFile()) {
        texPanel_.RevertSelectedFile(taskQueue_);
        VanGui::NotifyInfo("%s", std::string("Reverting last patch\xe2\x80\xa6").c_str());
    } else {
        VanGui::NotifyWarning("%s", std::string("Nothing to revert for the selected file.").c_str());
    }
}

// Register all chords with the VanGUI shortcut registry; each is mirrored into
// the command palette (Ctrl+Shift+P). Captureless thunks receive `this` via the
// shortcut user-data pointer.
void AppShell::RegisterShortcutsAndCommands() {
    auto open   = [](void* ud){ static_cast<AppShell*>(ud)->ActionOpenBUN(); };
    auto expsel = [](void* ud){ static_cast<AppShell*>(ud)->ActionExportSelectedTexture(); };
    auto revert = [](void* ud){ static_cast<AppShell*>(ud)->ActionRevertSelected(); };
    auto theme  = [](void* ud){ static_cast<AppShell*>(ud)->ToggleTheme(); };
    auto palette= [](void*  ){ VanGui::OpenCommandPalette(); };

    VanGui::RegisterShortcut(VanGuiMod_Ctrl | VanGuiKey_O, "Open BUN/BIN\xe2\x80\xa6",        open,    this);
    VanGui::RegisterShortcut(VanGuiMod_Ctrl | VanGuiKey_S, "Export selected texture (DDS)", expsel,  this);
    VanGui::RegisterShortcut(VanGuiMod_Ctrl | VanGuiKey_Z, "Revert selected file",          revert,  this);
    VanGui::RegisterShortcut(VanGuiMod_Ctrl | VanGuiKey_T, "Toggle dark/light theme",       theme,   this);
    VanGui::RegisterShortcut(VanGuiMod_Ctrl | VanGuiMod_Shift | VanGuiKey_P,
                             "Command palette\xe2\x80\xa6", palette, this);

    // Palette commands (a chord-less few are palette-only).
    VanGui::RegisterCommand({"File: Open BUN/BIN\xe2\x80\xa6",        "open load import bun bin", open,   this});
    VanGui::RegisterCommand({"Texture: Export selected as DDS",      "export save dds texture",  expsel, this});
    VanGui::RegisterCommand({"Edit: Revert selected file",           "revert undo restore",      revert, this});
    VanGui::RegisterCommand({"View: Toggle dark/light theme",        "theme dark light toggle",  theme,  this});
}

// ─── Main loop ───────────────────────────────────────────────────────────────

#if !defined(__ANDROID__)
void AppShell::Run() {
    GLFWwindow* window = glfwWindow_;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        VanGui_ImplOpenGL3_NewFrame();
        VanGui_ImplGlfw_NewFrame();
        VanGui::NewFrame();
        VanGui::NewFrameExtras();   // drain worker results, advance theme transition

        DrawFrame();
        fileDialog_.Draw();
        VanGui::DispatchShortcuts();  // (Phase 5) declarative shortcut registry

        VanGui::RenderExtras();     // draw toast overlays + command palette
        VanGui::Render();

        int fbw, fbh;
        glfwGetFramebufferSize(window, &fbw, &fbh);
        glViewport(0, 0, fbw, fbh);
        // With Mica active, clear to fully transparent so DWM blur shows through.
        // Without it, clear to opaque dark background.
        glClearColor(g_bg0.x, g_bg0.y, g_bg0.z, micaActive_ ? 0.0f : 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        VanGui_ImplOpenGL3_RenderDrawData(VanGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    VanGui::ShutdownThreadPool();     // drain + join workers after the render loop
    recentFiles_.Save();
    texPanel_.Shutdown();
    exportPanel_.Shutdown();
    carPanel_.Shutdown();
    uiHudPanel_.Shutdown();
    videoPanel_.Shutdown();
    uiSounds_.Shutdown();

    VanGui_ImplOpenGL3_Shutdown();
    VanGui_ImplGlfw_Shutdown();
    VanGui::DestroyContext();

    if (customCursor_) {
        glfwDestroyCursor(customCursor_);
        customCursor_ = nullptr;
    }
    glfwDestroyWindow(window);
    glfwTerminate();

#ifdef _WIN32
    if (comInitialized_) CoUninitialize();
#endif

    LOG_INFO("MWTools exiting");
    Logger::Shutdown();
}
#endif

#if defined(__ANDROID__)
Result<void> AppShell::InitAndroid(android_app* app, int width, int height) {
    androidApp_ = app;
    androidExitRequested_ = false;

    Logger::Init("nfsmwtoolkit.log");
    LOG_INFO("MWTools Android starting");

    size_t knownNames = RegisterKnownNames();
    LOG_INFO("Registered " + std::to_string(knownNames) + " known hash names");

    (void)VANGUI_CHECKVERSION();
    (void)VanGui::CreateContext();
    VanGui::InitThreadPool();

    VanGuiIO& io = VanGui::GetIO();
    io.IniFilename = nullptr;
    io.ConfigFlags |= VanGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= VanGuiConfigFlags_NoMouseCursorChange;

    // Android reports framebuffer size via EGL/native window; keep initial
    // display size sane until the first frame updates it.
    io.DisplaySize = VanVec2(static_cast<float>(width), static_cast<float>(height));

    LoadFont(1.0f);
    ApplyTheme(1.0f);
    ApplyDarkTitleBar(themeMode_ == ThemeMode::Dark);

    onBunLoadedConn_ = onBunLoaded_.connect_queued(
        [this](const std::filesystem::path& p) {
            statusMessage_ = "Loaded " + p.filename().string();
        });

    if (!VanGui_ImplAndroid_Init(app ? app->window : nullptr))
        return Result<void>::Err("VanGui_ImplAndroid_Init failed");
    if (!VanGui_ImplOpenGL3_Init("#version 300 es"))
        return Result<void>::Err("VanGui_ImplOpenGL3_Init failed");

    recentFiles_.Load();
    recentFiles_.PruneMissing();
    uiSounds_.Init();

    if (auto r = exportPanel_.Init(); !r)
        LOG_WARN("ObjectExportPanel shader init failed: {} (preview disabled)", r.error);
    else
        exportPanelReady_ = true;

    uiHudPanel_.Init(&fileDialog_);

    if (auto r = carPanel_.Init(); !r)
        LOG_WARN("CarPanel init failed: {} (car viewer disabled)", r.error);
    else
        carPanelReady_ = true;

    if (auto r = nisPanel_.Init(); !r)
        LOG_WARN("NisPanel shader init failed: {} (skeleton viewer disabled)", r.error);

    RegisterShortcutsAndCommands();

    androidInitialized_ = true;
    LOG_INFO("MWTools Android initialised ({}x{})", width, height);
    return Result<void>::Ok();
}

void AppShell::RenderAndroidFrame(int framebufferWidth, int framebufferHeight) {
    if (!androidInitialized_)
        return;

    VanGuiIO& io = VanGui::GetIO();
    io.DisplaySize = VanVec2(static_cast<float>(framebufferWidth),
                             static_cast<float>(framebufferHeight));

    VanGui_ImplOpenGL3_NewFrame();
    VanGui_ImplAndroid_NewFrame();
    VanGui::NewFrame();
    VanGui::NewFrameExtras();

    DrawFrame();
    fileDialog_.Draw();
    VanGui::DispatchShortcuts();

    VanGui::RenderExtras();
    VanGui::Render();

    glViewport(0, 0, framebufferWidth, framebufferHeight);
    glClearColor(g_bg0.x, g_bg0.y, g_bg0.z, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    VanGui_ImplOpenGL3_RenderDrawData(VanGui::GetDrawData());
}

void AppShell::ShutdownAndroid() {
    if (!androidInitialized_)
        return;

    VanGui::ShutdownThreadPool();
    recentFiles_.Save();
    texPanel_.Shutdown();
    exportPanel_.Shutdown();
    carPanel_.Shutdown();
    uiHudPanel_.Shutdown();
    videoPanel_.Shutdown();
    uiSounds_.Shutdown();

    VanGui_ImplOpenGL3_Shutdown();
    VanGui_ImplAndroid_Shutdown();
    VanGui::DestroyContext();

    androidInitialized_ = false;
    androidApp_ = nullptr;
    LOG_INFO("MWTools Android exiting");
    Logger::Shutdown();
}
#endif

} // namespace nfsmw
