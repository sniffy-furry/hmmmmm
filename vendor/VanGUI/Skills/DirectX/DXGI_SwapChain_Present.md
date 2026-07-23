# DXGI SwapChain & Present Hooking

## Why Present Is the Central Hook Point
`IDXGISwapChain::Present` is called once per rendered frame by virtually every
DirectX application. Hooking it gives you a guaranteed per-frame callback with
access to the swap chain, from which you can QueryInterface to the device and context.
It is the standard entry point for overlays, frame analyzers, and screenshot tools.

---

## DXGI Hierarchy

```
IDXGIFactory
  └─ Creates IDXGISwapChain
       ├─ IDXGISwapChain   (DX10/11 flip or discard)
       ├─ IDXGISwapChain1  (adds DXGI 1.2 features: stereo, composition)
       ├─ IDXGISwapChain2  (trim, source size)
       ├─ IDXGISwapChain3  (color space, current back buffer index for DX12)
       └─ IDXGISwapChain4  (HDR metadata)
```

Always hook at `IDXGISwapChain` (index 8) — works for all versions.
If the app uses `IDXGISwapChain3`, it still calls through the same vtable slot.

---

## Present Vtable Index Breakdown

```
IDXGISwapChain vtable (0-indexed):
[0]  QueryInterface          ← IUnknown
[1]  AddRef
[2]  Release
[3]  SetPrivateData          ← IDXGIObject
[4]  SetPrivateDataInterface
[5]  GetPrivateData
[6]  GetParent
[7]  GetDevice               ← IDXGIDeviceSubObject
[8]  Present                 ← IDXGISwapChain  ← HOOK THIS
[9]  GetBuffer
[10] SetFullscreenState
[11] GetFullscreenState
[12] GetDesc
[13] ResizeBuffers            ← Also hook — called on window resize
[14] ResizeTarget
[15] GetContainingOutput
[16] GetFrameStatistics
[17] GetLastPresentCount
```

---

## Reliable Present Hook (MinHook on vtable function)

Using MinHook on the raw function pointer from the vtable is more robust than
raw vtable patching — MinHook manages the trampoline:

```cpp
typedef HRESULT(WINAPI* PFN_Present)(IDXGISwapChain*, UINT, UINT);
PFN_Present oPresent = nullptr;

// Get address of Present from a dummy swap chain's vtable:
void* presentAddr = (*reinterpret_cast<void***>(pDummySC))[8];

MH_CreateHook(presentAddr, &hk_Present,
              reinterpret_cast<void**>(&oPresent));
MH_EnableHook(presentAddr);
```

---

## State Machine Pattern Inside Present Hook

Avoid doing heavy work (resource creation, COM init) every frame.
Use a state machine with `std::atomic`:

```cpp
enum class OverlayState { Uninitialized, Initializing, Ready, Failed };
std::atomic<OverlayState> g_state{ OverlayState::Uninitialized };

HRESULT WINAPI hk_Present(IDXGISwapChain* pSC, UINT sync, UINT flags) {
    switch (g_state.load()) {
    case OverlayState::Uninitialized:
        g_state = OverlayState::Initializing;
        if (InitOverlay(pSC))
            g_state = OverlayState::Ready;
        else
            g_state = OverlayState::Failed;
        break;

    case OverlayState::Ready:
        RenderOverlay();
        break;

    case OverlayState::Failed:
    case OverlayState::Initializing:
        break;  // skip
    }

    return oPresent(pSC, sync, flags);
}
```

---

## Extracting Device & Context from SwapChain

```cpp
struct OverlayResources {
    ID3D11Device*           device  = nullptr;
    ID3D11DeviceContext*    context = nullptr;
    ID3D11RenderTargetView* rtv     = nullptr;
};

bool InitOverlay(IDXGISwapChain* pSC) {
    OverlayResources& r = g_overlayRes;

    // Device from swap chain
    HRESULT hr = pSC->GetDevice(__uuidof(ID3D11Device), (void**)&r.device);
    if (FAILED(hr)) return false;

    r.device->GetImmediateContext(&r.context);

    // RTV on the back buffer
    ID3D11Texture2D* backBuf = nullptr;
    pSC->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuf);
    r.device->CreateRenderTargetView(backBuf, nullptr, &r.rtv);
    backBuf->Release();

    // ImGui
    DXGI_SWAP_CHAIN_DESC desc{};
    pSC->GetDesc(&desc);
    ImGui::CreateContext();
    ImGui_ImplWin32_Init(desc.OutputWindow);
    ImGui_ImplDX11_Init(r.device, r.context);

    return true;
}
```

---

## Handling Present1 (Windows 8+ Optimized Present)

Some apps call `IDXGISwapChain1::Present1` instead of `Present`.
Hook both to be safe:

```cpp
// Present1 is at vtable index 22 on IDXGISwapChain1
typedef HRESULT(WINAPI* PFN_Present1)(IDXGISwapChain1*, UINT, UINT,
                                       const DXGI_PRESENT_PARAMETERS*);
PFN_Present1 oPresent1 = nullptr;

HRESULT WINAPI hk_Present1(IDXGISwapChain1* pSC, UINT sync, UINT flags,
                             const DXGI_PRESENT_PARAMETERS* params) {
    // Same overlay logic as hk_Present
    RenderOverlayIfReady();
    return oPresent1(pSC, sync, flags, params);
}
```

---

## Window Message Interception (Input to Overlay)

ImGui needs to receive `WM_*` messages. Hook `WndProc` to forward them:

```cpp
WNDPROC oWndProc = nullptr;

LRESULT CALLBACK hk_WndProc(HWND hWnd, UINT msg, WPARAM wP, LPARAM lP) {
    // Forward to ImGui when overlay is visible
    if (g_overlayVisible) {
        if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wP, lP))
            return 1;  // ImGui consumed the message

        // Block mouse input from reaching the game when ImGui wants it
        if (ImGui::GetIO().WantCaptureMouse &&
            (msg == WM_MOUSEMOVE || msg == WM_LBUTTONDOWN ||
             msg == WM_LBUTTONUP || msg == WM_RBUTTONDOWN))
            return 1;
    }
    return CallWindowProc(oWndProc, hWnd, msg, wP, lP);
}

// Install after getting the HWND from swap chain desc:
oWndProc = reinterpret_cast<WNDPROC>(
    SetWindowLongPtr(hWnd, GWLP_WNDPROC,
                     reinterpret_cast<LONG_PTR>(hk_WndProc)));
```

---

## Cleanup on Detach

```cpp
void CleanupOverlay() {
    // Restore WndProc
    if (oWndProc) SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)oWndProc);

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    if (g_overlayRes.rtv)     { g_overlayRes.rtv->Release();     }
    if (g_overlayRes.context) { g_overlayRes.context->Release();  }
    if (g_overlayRes.device)  { g_overlayRes.device->Release();   }
}
```
