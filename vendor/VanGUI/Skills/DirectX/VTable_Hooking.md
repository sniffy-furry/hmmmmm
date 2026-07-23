# DirectX VTable Hooking

## Why VTable Hooking for DirectX
DirectX interfaces are COM objects — their vtable pointers are shared across all
instances of the same interface. Patch one entry in the vtable and you intercept
every call to that method from every caller in the process.

Two strategies:
1. **vtable pointer replacement** — swap the whole vtable (more fragile, easily detected)
2. **vtable entry patching** — write-protect off, patch one function pointer, re-protect (preferred)

---

## Step 1: Obtain a Valid Interface Instance

For in-process tools, create a dummy device using a hidden window:

```cpp
bool GetDXInterfaces(ID3D11Device** ppDevice,
                     ID3D11DeviceContext** ppCtx,
                     IDXGISwapChain** ppSC) {
    // Dummy window — never shown
    WNDCLASSEX wc{};
    wc.cbSize        = sizeof(wc);
    wc.lpfnWndProc   = DefWindowProcA;
    wc.lpszClassName = "DXDummy";
    RegisterClassEx(&wc);
    HWND hWnd = CreateWindowA("DXDummy", "", WS_OVERLAPPED,
                               0, 0, 100, 100, nullptr, nullptr, nullptr, nullptr);

    DXGI_SWAP_CHAIN_DESC scd{};
    scd.BufferCount       = 1;
    scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    scd.BufferUsage       = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scd.OutputWindow      = hWnd;
    scd.SampleDesc.Count  = 1;
    scd.Windowed          = TRUE;

    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
        0, nullptr, 0, D3D11_SDK_VERSION,
        &scd, ppSC, ppDevice, nullptr, ppCtx);

    DestroyWindow(hWnd);
    return SUCCEEDED(hr);
}
```

---

## Step 2: Extract vtable Function Pointers

```cpp
// vtable is the first pointer in the COM object
// Each entry is a 64-bit function pointer on x64
void** GetVTable(void* comObj) {
    return *reinterpret_cast<void***>(comObj);
}

// Capture Present (index 8) and ResizeBuffers (index 13)
void* oPresent       = GetVTable(pSwapChain)[8];
void* oResizeBuffers = GetVTable(pSwapChain)[13];
```

---

## Step 3: Patch vtable Entry (Write-Protect Aware)

```cpp
template<typename T>
void VTablePatch(void** vtable, int index, T detour, T* original) {
    void** entry = &vtable[index];

    // Remove write protection from the vtable page
    DWORD oldProtect;
    VirtualProtect(entry, sizeof(void*), PAGE_EXECUTE_READWRITE, &oldProtect);

    *original = reinterpret_cast<T>(*entry);   // save original
    *entry    = reinterpret_cast<void*>(detour); // install detour

    // Restore protection
    VirtualProtect(entry, sizeof(void*), oldProtect, &oldProtect);

    // Flush instruction cache so CPUs pick up the new pointer
    FlushInstructionCache(GetCurrentProcess(), entry, sizeof(void*));
}
```

---

## Step 4: Present Detour

```cpp
typedef HRESULT (WINAPI* PFN_Present)(IDXGISwapChain*, UINT, UINT);
PFN_Present oPresent = nullptr;
bool g_imgui_init    = false;

HRESULT WINAPI hk_Present(IDXGISwapChain* pSC, UINT SyncInterval, UINT Flags) {
    if (!g_imgui_init) {
        // First call — get device from swap chain
        ID3D11Device* device = nullptr;
        pSC->GetDevice(__uuidof(ID3D11Device), (void**)&device);

        ID3D11DeviceContext* ctx = nullptr;
        device->GetImmediateContext(&ctx);

        // Get the back buffer HWND via DXGI
        DXGI_SWAP_CHAIN_DESC desc{};
        pSC->GetDesc(&desc);

        ImGui::CreateContext();
        ImGui_ImplWin32_Init(desc.OutputWindow);
        ImGui_ImplDX11_Init(device, ctx);

        device->Release();
        ctx->Release();
        g_imgui_init = true;
    }

    // --- Your per-frame logic here ---
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    DrawToolWindows();

    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    // ---------------------------------

    return oPresent(pSC, SyncInterval, Flags);
}
```

---

## Step 5: ResizeBuffers Detour (Required for Window Resize)

Forgetting this causes a crash or black screen when the window is resized:

```cpp
typedef HRESULT (WINAPI* PFN_ResizeBuffers)(IDXGISwapChain*, UINT, UINT, UINT,
                                             DXGI_FORMAT, UINT);
PFN_ResizeBuffers oResizeBuffers = nullptr;

HRESULT WINAPI hk_ResizeBuffers(IDXGISwapChain* pSC, UINT count,
                                 UINT w, UINT h, DXGI_FORMAT fmt, UINT flags) {
    // Must release all references to the back buffer before resize
    ImGui_ImplDX11_InvalidateDeviceObjects();
    HRESULT hr = oResizeBuffers(pSC, count, w, h, fmt, flags);
    ImGui_ImplDX11_CreateDeviceObjects();
    return hr;
}
```

---

## Full Hook Installation Sequence

```cpp
void InstallDXHooks() {
    ID3D11Device*        device  = nullptr;
    ID3D11DeviceContext* ctx     = nullptr;
    IDXGISwapChain*      sc      = nullptr;

    if (!GetDXInterfaces(&device, &ctx, &sc)) return;

    void** vtbl = GetVTable(sc);

    VTablePatch(vtbl, 8,  hk_Present,       &oPresent);
    VTablePatch(vtbl, 13, hk_ResizeBuffers, &oResizeBuffers);

    // These interfaces are only needed to get the vtable — release immediately
    sc->Release();
    ctx->Release();
    device->Release();
}
```

---

## DirectX 12 — Additional Considerations

D3D12 uses `IDXGISwapChain3::Present` (same vtable index 8 on the DXGI side)
but rendering requires managing descriptor heaps manually for ImGui:

```cpp
// D3D12 requires a descriptor heap for ImGui's SRV
D3D12_DESCRIPTOR_HEAP_DESC desc{};
desc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
desc.NumDescriptors = 1;
desc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&g_srvHeap));

ImGui_ImplDX12_Init(device, NUM_FRAMES_IN_FLIGHT,
                     DXGI_FORMAT_R8G8B8A8_UNORM,
                     g_srvHeap,
                     g_srvHeap->GetCPUDescriptorHandleForHeapStart(),
                     g_srvHeap->GetGPUDescriptorHandleForHeapStart());
```

---

## Common Issues

| Issue | Cause | Fix |
|---|---|---|
| Black screen after hook | Back buffer still referenced during resize | Implement `ResizeBuffers` detour |
| Crash in Present detour | ImGui init failed silently | Check every HR; log device removal |
| Hook fires but nothing renders | Wrong command list / render target bound | Ensure OMSetRenderTargets before ImGui draw |
| DEVICE_REMOVED (0x887A0005) | Debug layer enabled in Release | Remove `D3D11_CREATE_DEVICE_DEBUG` flag |
| vtable patch reverted | Anti-tamper re-checks vtable integrity | Use MinHook on Present export instead |
