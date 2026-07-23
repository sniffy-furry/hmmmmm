---
name: dx9-architecture
description: >
  DirectX 9 API architecture: device creation, swap chains, render targets, MRTs,
  Shader Model 3.0 constraints, state management, resource lifetimes, and migration
  paths toward DX11/DX12. Essential reference for DX9 injector and legacy engine work.
tags: [dx9, directx9, sm3, shader-model-3, device, swap-chain, mrt, render-target, state-management, dx12]
related_skills: [15_DX9_Injector_Architecture, 12_Deferred_Rendering, 03_HLSL_Shaders]
---

# DirectX 9 Architecture

## API Version Comparison

| Feature | DX9 (SM3.0) | DX10 (SM4.0) | DX11 (SM5.0) | DX12 (SM6.x) |
|---------|------------|--------------|--------------|---------------|
| Compute Shaders | ✗ | ✗ | ✓ | ✓ |
| UAVs | ✗ | Limited | ✓ | ✓ |
| Geometry Shaders | ✗ | ✓ | ✓ | ✓ |
| Hull/Domain Shaders | ✗ | ✗ | ✓ | ✓ |
| Mesh Shaders | ✗ | ✗ | ✗ | ✓ |
| Descriptor Heaps | ✗ | ✗ | ✗ | ✓ |
| Command Queues | ✗ | ✗ | ✗ | ✓ |
| MRT Count (max) | 4 | 8 | 8 | 8 |
| Typed UAV Loads | ✗ | ✗ | Limited | ✓ |
| DXR (Ray Tracing) | ✗ | ✗ | ✗ | ✓ |

---

## Core DX9 Objects

### IDirect3DDevice9
The central interface. All rendering operations go through this object.

Key methods relevant to injectors and renderers:
```
IDirect3DDevice9::Present          — final frame submission
IDirect3DDevice9::EndScene         — end of render frame
IDirect3DDevice9::Reset            — device reset (fullscreen toggle, resize)
IDirect3DDevice9::SetPixelShader   — bind pixel shader
IDirect3DDevice9::SetVertexShader  — bind vertex shader
IDirect3DDevice9::SetRenderTarget  — bind render target (MRT index 0–3)
IDirect3DDevice9::SetDepthStencil  — bind depth/stencil surface
IDirect3DDevice9::GetBackBuffer    — acquire backbuffer surface
IDirect3DDevice9::CreateTexture    — allocate texture resource
IDirect3DDevice9::CreateSurface    — allocate standalone surface
IDirect3DDevice9::DrawPrimitive    — draw call
IDirect3DDevice9::DrawIndexedPrimitive — indexed draw call
```

### Swap Chain
Controls presentation to the display. Injectors capture the backbuffer during or before `Present`.

### Render Targets
DX9 supports up to 4 simultaneous render targets (MRT0–MRT3).

Recommended formats for deferred rendering:
```
A16B16G16R16F   — FP16 HDR, normals, high-precision data
A8R8G8B8        — LDR albedo, packed material flags
R5G6B5          — fallback for low-memory systems
D24S8           — depth + stencil (shared resource)
```

### Depth and Stencil Buffers
`D24S8` is the standard DX9 format (24-bit depth, 8-bit stencil).
Stencil is heavily used for light volume culling in DX9 deferred pipelines.

---

## Shader Model 3.0 Constraints

| Limit | SM3.0 Value |
|-------|------------|
| VS instruction count | 512 |
| PS instruction count | 512 |
| Texture samplers (PS) | 16 |
| Texture samplers (VS) | 4 |
| Constant registers (VS) | 256 |
| Constant registers (PS) | 224 |
| Interpolators | 10 |
| Dynamic branching | Limited |
| Integer math | ✗ (float only) |
| Bitwise ops | ✗ |
| Compute shaders | ✗ |
| UAVs / RWBuffers | ✗ |

**Critical implication:** Clustered light culling and Forward+ light indexing cannot be done on the GPU in DX9. Light lists must be built on the CPU and uploaded via constant buffers or textures.

---

## State Management

DX9 uses a legacy per-draw state model (no PSOs). State changes are expensive.

**Key render states:**
```cpp
SetRenderState(D3DRS_ZENABLE, TRUE)
SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE)
SetRenderState(D3DRS_STENCILENABLE, TRUE)
SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW)
```

**Optimization rules:**
- Batch by material state to minimize state changes
- Use `IDirect3DStateBlock9` to capture/restore full state in injectors
- Sort draw calls by shader, then by texture, then by material flags

---

## Resource Lifetime Management

DX9 resources are invalidated on device reset (alt-tab, resolution change).

**Pattern:**
```
OnDeviceLost:
  Release all D3DPOOL_DEFAULT resources
  Release all render targets and surfaces

OnDeviceReset:
  Recreate all D3DPOOL_DEFAULT resources
  Recreate render targets and surfaces
  Re-upload shaders and constant data
```

Injectors **must** handle `Reset` or risk crashes on alt-tab or resolution changes.

---

## Texture Formats (DX9 FOURCC)

| Format | Bits | Use |
|--------|------|-----|
| A8R8G8B8 | 32 | Albedo, LDR color |
| A16B16G16R16F | 64 | HDR, normals |
| D24S8 | 32 | Depth + Stencil |
| R32F | 32 | Single-channel float (shadow maps) |
| A32B32G32R32F | 128 | Maximum precision (rare, expensive) |

---

## Migration Notes: DX9 → DX11 / DX12

| DX9 Concept | DX11 Equivalent | DX12 Equivalent |
|-------------|----------------|----------------|
| SetRenderState | DepthStencilState / RasterizerState / BlendState objects | Pipeline State Object (PSO) |
| SetTexture | SRV bound via context | Descriptor Heap + Root Signature |
| DrawPrimitive | Draw / DrawIndexed | ExecuteCommandList |
| Reset | SwapChain::ResizeBuffers | Manual recreation |
| State blocks | Not needed (state objects) | Not needed (PSOs) |

---

## Related Skills
- Skill 15: DX9 Injector Architecture — hooking, VTable discovery, state capture
- Skill 12: Deferred Rendering — MRT G-buffer design under DX9
- Skill 03: HLSL Shaders — SM3.0 shader authoring
