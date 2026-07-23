// VanGUI Module Verification Test
// Verifies that `import vangui;` (C++23 module) works end-to-end.
// Displays the current PC date/time in a live VanGUI window.
//
// C++23 pattern for mixing module imports with legacy headers:
//   - All non-modular headers go into the global module fragment (before the
//     module declaration).  This prevents C2572 "redefinition of default
//     argument" when the module IFC and the classic header both declare the
//     same functions with default arguments.
//   - `import vangui;` appears AFTER the module declaration, safely consuming
//     the module without seeing any conflicting re-declarations.

module; // ← begin global module fragment: classic headers go here

// Backends: not part of the module, compiled as regular TUs
#include "vangui_impl_win32.h"
#include "vangui_impl_dx11.h"

#include <d3d11.h>
#include <tchar.h>
#include <chrono>
#include <ctime>

// WndProcHandler forward declaration must live here in the global module fragment
// (not in the module purview) so it gets standard external linkage mangling.
// The backend header intentionally hides this inside #if 0 — we copy it as documented.
extern VANGUI_IMPL_API LRESULT
VanGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

export module vangui_clock_test; // ← end global module fragment; name this TU

import vangui; // <-- C++23 module import: this is what we are testing

// ---------------------------------------------------------------------------
// D3D11 globals
// ---------------------------------------------------------------------------
static ID3D11Device*           g_pd3dDevice          = nullptr;
static ID3D11DeviceContext*    g_pd3dDeviceContext    = nullptr;
static IDXGISwapChain*         g_pSwapChain           = nullptr;
static bool                    g_SwapChainOccluded    = false;
static UINT                    g_ResizeWidth          = 0;
static UINT                    g_ResizeHeight         = 0;
static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;

// ---------------------------------------------------------------------------
// Forward declarations
// ---------------------------------------------------------------------------
static bool CreateDeviceD3D(HWND hWnd);
static void CleanupDeviceD3D();
static void CreateRenderTarget();
static void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// ---------------------------------------------------------------------------
// Helper: get the current local time as a struct tm
// ---------------------------------------------------------------------------
static std::tm GetLocalTime_tm()
{
    auto now   = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm_local{};
    localtime_s(&tm_local, &t);
    return tm_local;
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------
extern "C" int main(int, char**)
{
    // DPI awareness
    VanGui_ImplWin32_EnableDpiAwareness();
    float scale = VanGui_ImplWin32_GetDpiScaleForMonitor(
        ::MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY));

    // Create window
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L,
                       GetModuleHandle(nullptr), nullptr, nullptr,
                       nullptr, nullptr, L"VanGuiClockTest", nullptr };
    ::RegisterClassExW(&wc);
    HWND hwnd = ::CreateWindowW(
        wc.lpszClassName,
        L"VanGUI Clock Test  —  import vangui; verified",
        WS_OVERLAPPEDWINDOW,
        100, 100,
        static_cast<int>(700 * scale),
        static_cast<int>(340 * scale),
        nullptr, nullptr, wc.hInstance, nullptr);

    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    // VanGUI setup  (via imported module — no #include "vangui.h" in this TU)
    VANGUI_CHECKVERSION();
    VanGui::CreateContext();

    VanGuiIO& io = VanGui::GetIO();
    io.ConfigFlags |= VanGuiConfigFlags_NavEnableKeyboard;

    VanGui::StyleColorsDark();

    VanGuiStyle& style = VanGui::GetStyle();
    style.ScaleAllSizes(scale);
    style.FontScaleDpi = scale;

    VanGui_ImplWin32_Init(hwnd);
    VanGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    VanVec4 clear_color = VanVec4(0.10f, 0.10f, 0.12f, 1.00f);

    bool done = false;
    while (!done)
    {
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        if (g_SwapChainOccluded &&
            g_pSwapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED)
        {
            ::Sleep(10);
            continue;
        }
        g_SwapChainOccluded = false;

        if (g_ResizeWidth != 0 && g_ResizeHeight != 0)
        {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight,
                                        DXGI_FORMAT_UNKNOWN, 0);
            g_ResizeWidth = g_ResizeHeight = 0;
            CreateRenderTarget();
        }

        // --- Frame ---
        VanGui_ImplDX11_NewFrame();
        VanGui_ImplWin32_NewFrame();
        VanGui::NewFrame();

        // Clock window
        VanGui::SetNextWindowPos(VanVec2(30, 30), VanGuiCond_Once);
        VanGui::SetNextWindowSize(VanVec2(380, 220), VanGuiCond_Once);
        VanGui::Begin("Clock", nullptr, VanGuiWindowFlags_NoResize);
        {
            std::tm t = GetLocalTime_tm();

            // Large time display
            VanGui::SetWindowFontScale(2.2f);
            VanGui::Text("%02d:%02d:%02d",
                t.tm_hour, t.tm_min, t.tm_sec);
            VanGui::SetWindowFontScale(1.0f);

            VanGui::Spacing();
            VanGui::Separator();
            VanGui::Spacing();

            // Date
            VanGui::Text("Date   : %04d-%02d-%02d",
                t.tm_year + 1900, t.tm_mon + 1, t.tm_mday);

            // Day of week
            static const char* kDays[] = {
                "Sunday","Monday","Tuesday","Wednesday",
                "Thursday","Friday","Saturday"
            };
            VanGui::Text("Day    : %s", kDays[t.tm_wday]);

            VanGui::Spacing();
            VanGui::Separator();
            VanGui::Spacing();

            VanGui::TextDisabled("%.1f FPS  |  import vangui; OK",
                io.Framerate);
        }
        VanGui::End();

        // --- Render ---
        VanGui::Render();
        const float cc[4] = {
            clear_color.x * clear_color.w,
            clear_color.y * clear_color.w,
            clear_color.z * clear_color.w,
            clear_color.w
        };
        g_pd3dDeviceContext->OMSetRenderTargets(
            1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(
            g_mainRenderTargetView, cc);
        VanGui_ImplDX11_RenderDrawData(VanGui::GetDrawData());

        HRESULT hr = g_pSwapChain->Present(1, 0);
        g_SwapChainOccluded = (hr == DXGI_STATUS_OCCLUDED);
    }

    // Cleanup
    VanGui_ImplDX11_Shutdown();
    VanGui_ImplWin32_Shutdown();
    VanGui::DestroyContext();
    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
    return 0;
}

// ---------------------------------------------------------------------------
// D3D11 helpers
// ---------------------------------------------------------------------------
static bool CreateDeviceD3D(HWND hWnd)
{
    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferCount                        = 2;
    sd.BufferDesc.Format                  = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator   = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags                              = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage                        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow                       = hWnd;
    sd.SampleDesc.Count                   = 1;
    sd.Windowed                           = TRUE;
    sd.SwapEffect                         = DXGI_SWAP_EFFECT_DISCARD;

    UINT flags = 0;
    D3D_FEATURE_LEVEL fl;
    const D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_0,
                                         D3D_FEATURE_LEVEL_10_0 };
    HRESULT res = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags,
        levels, 2, D3D11_SDK_VERSION,
        &sd, &g_pSwapChain, &g_pd3dDevice, &fl, &g_pd3dDeviceContext);
    if (res == DXGI_ERROR_UNSUPPORTED)
        res = D3D11CreateDeviceAndSwapChain(
            nullptr, D3D_DRIVER_TYPE_WARP, nullptr, flags,
            levels, 2, D3D11_SDK_VERSION,
            &sd, &g_pSwapChain, &g_pd3dDevice, &fl, &g_pd3dDeviceContext);
    if (res != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

static void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain)         { g_pSwapChain->Release();         g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext)  { g_pd3dDeviceContext->Release();  g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice)         { g_pd3dDevice->Release();         g_pd3dDevice = nullptr; }
}

static void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer = nullptr;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    if (pBackBuffer)
    {
        g_pd3dDevice->CreateRenderTargetView(
            pBackBuffer, nullptr, &g_mainRenderTargetView);
        pBackBuffer->Release();
    }
}

static void CleanupRenderTarget()
{
    if (g_mainRenderTargetView)
    {
        g_mainRenderTargetView->Release();
        g_mainRenderTargetView = nullptr;
    }
}

// ---------------------------------------------------------------------------
// Win32 message handler
// (VanGui_ImplWin32_WndProcHandler is declared in the global module fragment
//  via #include "vangui_impl_win32.h" — no re-declaration needed here)
// ---------------------------------------------------------------------------
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (VanGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED) return 0;
        g_ResizeWidth  = LOWORD(lParam);
        g_ResizeHeight = HIWORD(lParam);
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}
