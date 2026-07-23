# D3D11 Device, Context & Rendering Pipeline

## The Three Core Objects

| Object | Role | Obtained Via |
|---|---|---|
| `ID3D11Device` | Resource creation (textures, buffers, shaders) | `D3D11CreateDeviceAndSwapChain` |
| `ID3D11DeviceContext` | Draw calls, state setting, resource binding | `device->GetImmediateContext()` |
| `IDXGISwapChain` | Frame presentation, back buffer management | `D3D11CreateDeviceAndSwapChain` |

---

## Full Device Initialization

```cpp
struct D3D11State {
    ID3D11Device*           device      = nullptr;
    ID3D11DeviceContext*    context     = nullptr;
    IDXGISwapChain*         swapChain   = nullptr;
    ID3D11RenderTargetView* backBufferRTV = nullptr;
};

bool InitD3D11(HWND hWnd, D3D11State& s) {
    DXGI_SWAP_CHAIN_DESC scd{};
    scd.BufferCount                 = 2;
    scd.BufferDesc.Width            = 0;  // 0 = use window size
    scd.BufferDesc.Height           = 0;
    scd.BufferDesc.Format           = DXGI_FORMAT_R8G8B8A8_UNORM;
    scd.BufferDesc.RefreshRate      = { 60, 1 };
    scd.BufferUsage                 = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scd.OutputWindow                = hWnd;
    scd.SampleDesc.Count            = 1;
    scd.Windowed                    = TRUE;
    scd.SwapEffect                  = DXGI_SWAP_EFFECT_DISCARD;

    UINT flags = 0;
#ifdef _DEBUG
    flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_FEATURE_LEVEL featureLevel;
    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr,                    // default adapter
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr, flags,
        nullptr, 0,                 // feature levels (uses highest available)
        D3D11_SDK_VERSION,
        &scd, &s.swapChain,
        &s.device, &featureLevel, &s.context
    );
    if (FAILED(hr)) return false;

    // Create render target view from back buffer
    ID3D11Texture2D* backBuf = nullptr;
    s.swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuf);
    s.device->CreateRenderTargetView(backBuf, nullptr, &s.backBufferRTV);
    backBuf->Release();

    return true;
}
```

---

## Recreating Resources on Resize

```cpp
void OnResize(D3D11State& s, UINT width, UINT height) {
    // Release RTV before resize
    s.context->OMSetRenderTargets(0, nullptr, nullptr);
    s.backBufferRTV->Release();
    s.backBufferRTV = nullptr;

    // Resize swap chain buffers
    s.swapChain->ResizeBuffers(0, width, height,
                                DXGI_FORMAT_UNKNOWN, 0);

    // Recreate RTV
    ID3D11Texture2D* backBuf = nullptr;
    s.swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuf);
    s.device->CreateRenderTargetView(backBuf, nullptr, &s.backBufferRTV);
    backBuf->Release();

    // Reset viewport
    D3D11_VIEWPORT vp{};
    vp.Width    = (float)width;
    vp.Height   = (float)height;
    vp.MaxDepth = 1.0f;
    s.context->RSSetViewports(1, &vp);
}
```

---

## Per-Frame Render Loop

```cpp
void RenderFrame(D3D11State& s) {
    // Clear back buffer
    const float clearColor[4] = { 0.08f, 0.08f, 0.10f, 1.0f };
    s.context->ClearRenderTargetView(s.backBufferRTV, clearColor);
    s.context->OMSetRenderTargets(1, &s.backBufferRTV, nullptr);

    // --- Your draw calls here ---
    DrawScene(s.context);

    // ImGui (if integrated)
    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    // Present: SyncInterval=1 → vsync, 0 → uncapped
    s.swapChain->Present(1, 0);
}
```

---

## Resource Creation Patterns

### Constant Buffer (per-object transforms, uniforms)
```cpp
struct SceneConstants {
    float mvp[16];      // model-view-projection
    float color[4];
};

ID3D11Buffer* CreateConstantBuffer(ID3D11Device* dev, SIZE_T byteSize) {
    D3D11_BUFFER_DESC bd{};
    bd.Usage          = D3D11_USAGE_DYNAMIC;
    bd.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    bd.ByteWidth      = (UINT)((byteSize + 15) & ~15);  // must be multiple of 16

    ID3D11Buffer* buf = nullptr;
    dev->CreateBuffer(&bd, nullptr, &buf);
    return buf;
}

// Upload data each frame:
void UpdateConstantBuffer(ID3D11DeviceContext* ctx,
                           ID3D11Buffer* buf, const void* data, SIZE_T size) {
    D3D11_MAPPED_SUBRESOURCE mapped{};
    ctx->Map(buf, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    memcpy(mapped.pData, data, size);
    ctx->Unmap(buf, 0);
}
```

### Texture from Memory
```cpp
ID3D11ShaderResourceView* CreateTextureFromRGBA(ID3D11Device* dev,
                                                  const uint8_t* pixels,
                                                  int width, int height) {
    D3D11_TEXTURE2D_DESC td{};
    td.Width            = width;
    td.Height           = height;
    td.MipLevels        = 1;
    td.ArraySize        = 1;
    td.Format           = DXGI_FORMAT_R8G8B8A8_UNORM;
    td.SampleDesc.Count = 1;
    td.Usage            = D3D11_USAGE_DEFAULT;
    td.BindFlags        = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA init{ pixels, (UINT)(width * 4), 0 };
    ID3D11Texture2D* tex = nullptr;
    dev->CreateTexture2D(&td, &init, &tex);

    ID3D11ShaderResourceView* srv = nullptr;
    dev->CreateShaderResourceView(tex, nullptr, &srv);
    tex->Release();
    return srv;
}
```

---

## Debug Layer Usage

```cpp
// Enable at device creation:
flags |= D3D11_CREATE_DEVICE_DEBUG;

// At shutdown, check for leaks:
ID3D11Debug* debug = nullptr;
device->QueryInterface(__uuidof(ID3D11Debug), (void**)&debug);
if (debug) {
    debug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL);
    debug->Release();
}
// Output window will list every leaked resource with its creation callstack
```

---

## Shader Compilation

```cpp
ID3DBlob* CompileShader(const char* src, const char* entry,
                          const char* profile) {
    ID3DBlob* code  = nullptr;
    ID3DBlob* errors = nullptr;
    HRESULT hr = D3DCompile(src, strlen(src), nullptr, nullptr, nullptr,
                              entry, profile,
                              D3DCOMPILE_OPTIMIZATION_LEVEL3, 0,
                              &code, &errors);
    if (FAILED(hr) && errors) {
        OutputDebugStringA((char*)errors->GetBufferPointer());
        errors->Release();
    }
    return code;  // caller owns; Release() when done
}

// Usage:
ID3DBlob* vsBlob = CompileShader(vsSource, "VSMain", "vs_5_0");
ID3D11VertexShader* vs = nullptr;
device->CreateVertexShader(vsBlob->GetBufferPointer(),
                            vsBlob->GetBufferSize(), nullptr, &vs);
```
