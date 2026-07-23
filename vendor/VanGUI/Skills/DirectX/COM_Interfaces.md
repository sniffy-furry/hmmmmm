# DirectX COM Interfaces

## What COM Is (At the ABI Level)
Component Object Model is a binary interface standard. Every COM object is a struct
whose first member is a pointer to a **vtable** — an array of function pointers.
The vtable layout is fixed by the interface definition and never changes between versions.
This is what makes hooking COM interfaces both reliable and predictable.

```
Object in memory:
┌─────────────┐
│  vtbl_ptr ──┼──► ┌──────────────────┐
│  field_1    │    │ QueryInterface() │  [vtable index 0]
│  field_2    │    │ AddRef()         │  [vtable index 1]
│  field_3    │    │ Release()        │  [vtable index 2]
└─────────────┘    │ Method_A()       │  [vtable index 3]
                   │ Method_B()       │  [vtable index 4]
                   └──────────────────┘
```

---

## Obtaining a COM Interface

### Method 1: CoCreateInstance (Out-of-Process / System COM)
```cpp
#include <d3d11.h>
#pragma comment(lib, "d3d11.lib")

// Always initialize COM first in the thread
CoInitializeEx(nullptr, COINIT_MULTITHREADED);

// Direct creation (no CoCreateInstance needed for D3D)
ID3D11Device* device = nullptr;
ID3D11DeviceContext* context = nullptr;
IDXGISwapChain* swapChain = nullptr;

DXGI_SWAP_CHAIN_DESC scd{};
scd.BufferCount       = 1;
scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
scd.BufferUsage       = DXGI_USAGE_RENDER_TARGET_OUTPUT;
scd.OutputWindow      = hWnd;
scd.SampleDesc.Count  = 1;
scd.Windowed          = TRUE;

D3D11CreateDeviceAndSwapChain(
    nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
    0, nullptr, 0, D3D11_SDK_VERSION,
    &scd, &swapChain, &device, nullptr, &context
);
```

### Method 2: QueryInterface (Interface Casting)
```cpp
// IDXGISwapChain → IDXGISwapChain1 upgrade
IDXGISwapChain1* swapChain1 = nullptr;
HRESULT hr = swapChain->QueryInterface(
    __uuidof(IDXGISwapChain1),
    reinterpret_cast<void**>(&swapChain1)
);
if (SUCCEEDED(hr)) {
    // swapChain1 is valid; remember to Release() when done
}

// Get DXGI factory from existing device
IDXGIDevice* dxgiDevice = nullptr;
device->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDevice);

IDXGIAdapter* adapter = nullptr;
dxgiDevice->GetAdapter(&adapter);

IDXGIFactory* factory = nullptr;
adapter->GetParent(__uuidof(IDXGIFactory), (void**)&factory);
```

---

## COM Reference Counting
Every COM interface derives from `IUnknown`. Reference counting is manual:

```cpp
// AddRef is called by QueryInterface and assignment automatically
// You must Release() every interface you obtained

struct ComGuard {
    IUnknown* ptr;
    explicit ComGuard(IUnknown* p) : ptr(p) {}
    ~ComGuard() { if (ptr) ptr->Release(); }
};

// Or use Microsoft::WRL::ComPtr (preferred in modern code)
#include <wrl/client.h>
using Microsoft::WRL::ComPtr;

ComPtr<ID3D11Device>      device;
ComPtr<ID3D11DeviceContext> context;
ComPtr<IDXGISwapChain>    swapChain;
// Release() called automatically in destructor
```

---

## Key DirectX Interface Hierarchy

```
IUnknown
├── IDXGIObject
│   ├── IDXGIAdapter  ← physical GPU
│   ├── IDXGIOutput   ← monitor
│   ├── IDXGIFactory  → creates SwapChains
│   └── IDXGISwapChain  ← Present() lives here
│       └── IDXGISwapChain1/2/3/4  (extended versions)
└── ID3D11Device         ← resource creation
    └── ID3D11DeviceContext ← draw calls, state setting
        └── ID3D11DeviceContext1/2/3/4
```

---

## Getting vtable Indices (for Hooking)

The vtable index of any method is determined by its declaration order in the header,
starting from `IUnknown` methods at index 0.

```
IUnknown:
  [0] QueryInterface
  [1] AddRef
  [2] Release

IDXGIObject (extends IUnknown, so continues at 3):
  [3] SetPrivateData
  [4] SetPrivateDataInterface
  [5] GetPrivateData
  [6] GetParent

IDXGISwapChain (extends IDXGIDeviceSubObject which extends IDXGIObject):
  [8]  Present          ← most commonly hooked
  [9]  GetBuffer
  [10] SetFullscreenState
  [11] GetFullscreenState
  [12] GetDesc
  [13] ResizeBuffers
  [14] ResizeTarget
  [15] GetContainingOutput
  [16] GetFrameStatistics
  [17] GetLastPresentCount
```

To verify indices for any interface:
```cpp
// Dump vtable entries at runtime
void PrintVTable(void* comObj, int count) {
    void** vtbl = *reinterpret_cast<void***>(comObj);
    for (int i = 0; i < count; i++) {
        printf("vtbl[%d] = %p\n", i, vtbl[i]);
    }
}
```

---

## HRESULT Error Handling

```cpp
#define CHECK_HR(hr, msg) \
    if (FAILED(hr)) { \
        char buf[256]; \
        sprintf_s(buf, "%s: HRESULT 0x%08X", msg, (unsigned)hr); \
        OutputDebugStringA(buf); \
        return false; \
    }

// Common HRESULTs
// S_OK          = 0x00000000  success
// E_FAIL        = 0x80004005  generic failure
// E_INVALIDARG  = 0x80070057  bad argument
// E_OUTOFMEMORY = 0x8007000E
// DXGI_ERROR_DEVICE_REMOVED = 0x887A0005  GPU reset
// DXGI_ERROR_DEVICE_HUNG    = 0x887A0006
```
