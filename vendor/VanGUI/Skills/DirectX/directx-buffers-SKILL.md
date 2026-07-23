---
name: directx-buffers
description: >
  Deep reference skill for DirectX-style GPU buffer types, their roles in the rendering pipeline,
  and how they interact during frame production. Trigger this skill whenever the user asks about
  color buffers, back/front buffers, depth buffers, stencil buffers, shadow maps, depth maps,
  alpha channels, render targets, swap chains, or any GPU memory surface in a DirectX or broadly
  Direct3D-style rendering context. Also trigger for questions about buffer management, resource
  binding, per-pixel operations, framebuffer composition, and GPU memory layout — even if DirectX
  is not mentioned by name. Use proactively when the user is debugging z-fighting, transparency
  sorting, shadow artifacts, stencil masking, or render-to-texture pipelines.
---

# DirectX-Style Rendering Buffers — Deep Reference

This skill covers the purpose, layout, hardware semantics, and production-grade usage patterns
of every major buffer type in a DirectX-style rasterization pipeline. Content is written at a
level appropriate for graphics programmers, engine developers, and computer science students
studying real-time rendering.

---

## Conceptual Foundation

Before examining individual buffers, establish the mental model: a **framebuffer** is not a
single monolithic block of memory. It is a **collection of typed surfaces**, each storing a
distinct per-pixel quantity. The GPU's output-merger (OM) stage reads from and writes to these
surfaces simultaneously during rasterization, combining depth tests, stencil tests, and blending
operations to produce the final composited image.

Every surface described below occupies a region of GPU-accessible memory (VRAM or unified
memory on integrated hardware). In Direct3D 11/12 and Vulkan-adjacent APIs, these are exposed
as **resources** — typed memory allocations with format descriptors, dimension metadata, and
binding flags that determine which pipeline stages may read or write them.

---

## 1. Color Buffer

### Definition
The color buffer (also called the **render target** in its generic form, or the **framebuffer
color attachment** in Vulkan terminology) stores the RGBA color value for every pixel in the
output surface. It is the final destination for all shading computations.

### Format
Typically `DXGI_FORMAT_R8G8B8A8_UNORM` (32 bpp, 8 bits per channel, normalized unsigned
integer) for standard dynamic range, or `DXGI_FORMAT_R16G16B16A16_FLOAT` (64 bpp) for HDR
pipelines where values exceed [0, 1]. Choosing the wrong format introduces **quantization
error** (banding) in gradients or clamps HDR values prematurely.

### Hardware Semantics
The rasterizer outputs per-fragment color from the **Pixel Shader** (PS) stage. The
output-merger then executes **alpha blending** between the incoming fragment color and the
existing color buffer value before writing back. This entire operation occurs in
**render-target-view (RTV)** bound memory.

### Production Notes
- In **deferred shading** pipelines, multiple color buffers (the **G-buffer**) are written
  simultaneously via **Multiple Render Targets (MRT)** — up to 8 simultaneous RTVs in D3D11,
  bounded by `D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT`.
- **Tile-based deferred renderers** (common on mobile GPUs) keep color data in on-chip tile
  memory to avoid expensive DRAM round-trips; incorrect `loadAction`/`storeAction` flags waste
  this bandwidth advantage.
- Clear the color buffer each frame with `ClearRenderTargetView` unless you intentionally
  accumulate (e.g., temporal accumulation passes).

---

## 2. Back Buffer

### Definition
The back buffer is the **off-screen render target** into which the current frame is drawn. It
is never displayed directly while rendering is in progress — that would produce **tearing**,
where the display scans out a partially-written frame.

### Role in the Swap Chain
The back buffer is one slot in a **swap chain** (`IDXGISwapChain`), a circular queue of
color surfaces managed jointly by the GPU driver and the OS compositor (DXGI on Windows). A
typical swap chain configuration:

```
Swap Chain (double-buffered):
  Slot 0 — Front Buffer  →  displayed by monitor
  Slot 1 — Back Buffer   →  GPU is rendering here
```

In triple buffering, a third slot decouples GPU throughput from display refresh timing.

### Present Semantics
When rendering is complete, `IDXGISwapChain::Present(SyncInterval, Flags)` is called:
- `SyncInterval = 1` — wait for vertical blanking interval (**VSync**), prevents tearing.
- `SyncInterval = 0` — present immediately, allows tearing, minimizes latency.

After `Present`, the back buffer and front buffer **swap roles** (or are **flipped** via page
flipping at the hardware level — no pixel copy occurs on modern hardware).

### Production Notes
- The back buffer format must match the swap chain's declared `DXGI_FORMAT`. Mismatches cause
  validation errors.
- In **flip model** swap chains (preferred since Windows 10), the OS compositor reads directly
  from GPU surfaces, reducing latency vs. the legacy **bitblt model**.
- Variable Refresh Rate (VRR / G-Sync / FreeSync) only activates in flip model with
  `DXGI_SWAP_EFFECT_FLIP_DISCARD` or `FLIP_SEQUENTIAL`.

---

## 3. Front Buffer

### Definition
The front buffer is the color surface **currently being scanned out to the display**. Its
pixels are read sequentially by the display controller at the monitor's refresh rate (e.g.,
144 Hz = once every ~6.94 ms).

### Why You Don't Write Here Directly
Writing to the front buffer while it is being scanned produces **tearing** — a horizontal
discontinuity where the top portion of the screen shows the previous frame and the bottom
shows the new one. The back buffer / swap chain abstraction exists precisely to eliminate this.

### Exceptions
Legacy applications sometimes wrote to the front buffer intentionally for **overlay UIs**
or **debug overlays** using `GetDC` on the swap chain surface. This practice is deprecated
in modern DXGI flip-model swap chains.

### Production Notes
- The front buffer is **read-only** from the application's perspective in well-formed
  rendering code.
- **Screenshot capture** typically copies the front buffer (or back buffer post-present) to
  a CPU-readable staging texture via `CopyResource`.

---

## 4. Depth Buffer (Z-Buffer)

### Definition
The depth buffer stores a **scalar depth value per pixel** representing the normalized
distance from the camera's near plane to the nearest resolved geometry at that screen
location. It is the canonical solution to the **visibility problem** — determining which
surface is closest to the camera for each pixel.

### Format
Common formats in Direct3D:
| Format | Bit Depth | Notes |
|---|---|---|
| `DXGI_FORMAT_D32_FLOAT` | 32-bit float | Maximum precision; preferred for production |
| `DXGI_FORMAT_D24_UNORM_S8_UINT` | 24-bit fixed + 8-bit stencil | Combined depth-stencil |
| `DXGI_FORMAT_D16_UNORM` | 16-bit fixed | Memory-efficient; low precision |
| `DXGI_FORMAT_D32_FLOAT_S8X24_UINT` | 32-bit float + 8-bit stencil | Highest precision + stencil |

### Depth Test
For each incoming fragment at screen position (x, y) with NDC depth `z_fragment`, the
output-merger compares it to `depth_buffer[x][y]`:
```
if (z_fragment [ComparisonFunc] depth_buffer[x][y]):
    write fragment color and update depth_buffer[x][y] = z_fragment
else:
    discard fragment
```
The default comparison function is `D3D11_COMPARISON_LESS` (nearer fragments win).

### Depth Precision and Z-Fighting
Depth values in NDC space are **non-linearly distributed** — the majority of precision clusters
near the near plane due to the perspective divide. This causes **z-fighting**: two coplanar
or nearly coplanar surfaces flickering as floating-point rounding alternately selects one over
the other.

Mitigations:
- **Reverse-Z** (`D3D11_COMPARISON_GREATER`): Map far plane to 0.0 and near plane to 1.0 in
  NDC. Floating-point density is highest near 0.0, so this distributes precision toward the
  far plane where z-fighting is worst. Widely adopted in production engines.
- **Logarithmic depth buffer**: Linearizes depth distribution. Requires custom vertex shader
  output but virtually eliminates z-fighting at large depth ranges.
- Increase the near plane distance; precision degrades as `far/near` ratio grows.

### Production Notes
- Bind as a **Depth-Stencil View (DSV)** with `ClearDepthStencilView` each frame.
- Depth prepass: render geometry depth-only in a first pass, then render shading with
  `COMPARISON_EQUAL` — eliminates overdraw (pixel shader invocations on occluded fragments).
- Early-Z hardware optimization: the GPU may test depth before invoking the pixel shader if
  no depth modification (`SV_Depth` write or `discard`) occurs in the shader.

---

## 5. Stencil Buffer

### Definition
The stencil buffer stores an **8-bit unsigned integer per pixel**, independent of color or
depth, used as a per-pixel conditional mask to include or exclude regions from subsequent
rendering operations. It enables effects that require knowledge of which pixels belong to a
particular geometric region.

### Stencil Operation Pipeline
The stencil test precedes the depth test in the OM stage:

```
1. Read stencil_buffer[x][y]
2. Apply stencil read mask: (stencil_buffer[x][y] & ReadMask)
3. Compare against StencilRef value using StencilFunc
4. Pass → proceed to depth test; Fail → apply StencilFailOp and discard
5. Depth fail → apply DepthFailOp; Depth pass → apply PassOp
```

Operations (`D3D11_STENCIL_OP`): `KEEP`, `ZERO`, `REPLACE`, `INCR_SAT`, `DECR_SAT`,
`INVERT`, `INCR`, `DECR`.

### Common Use Cases

**Portal / Mirror Rendering**
1. Render the mirror quad with `StencilOp = REPLACE`, writing value `1` to the stencil
   buffer everywhere the mirror is visible.
2. Set `StencilFunc = EQUAL, StencilRef = 1` and render the reflected scene — pixels
   outside the mirror are masked out by the stencil test.

**Shadow Volumes (Carmack's Reverse / Stencil Shadow)**
Increment stencil on depth-fail front faces, decrement on depth-fail back faces. Lit pixels
have stencil value 0; shadowed pixels are nonzero.

**Outline / Silhouette Effects**
1. Render object normally.
2. Render object scaled up, writing stencil.
3. Render outline color only where stencil differs from the inner object.

**Decals**
Restrict decal projection to pixels belonging to a specific surface by tagging that surface's
stencil value during the G-buffer pass.

### Production Notes
- Stencil is almost always packed with depth in a combined `D24_UNORM_S8_UINT` or
  `D32_FLOAT_S8X24_UINT` resource — they share the same `DSV`.
- `StencilReadMask` and `StencilWriteMask` allow multiple independent stencil channels within
  the 8-bit value using bit fields.
- Stencil operations are configured via `D3D11_DEPTH_STENCIL_DESC` and set with
  `CreateDepthStencilState`.

---

## 6. Alpha Channel

### Definition
The alpha channel is the **fourth component (A) of an RGBA color value**, typically occupying
8 bits in standard formats. It encodes **opacity** — how much the surface's color contributes
to the final composited pixel versus what was previously in the color buffer.

### Blending Equation
Standard **source-over** alpha blending (Porter-Duff):
```
C_out = C_src × α_src + C_dst × (1 − α_src)
```
Where `C_src` is the incoming fragment color, `α_src` is the fragment's alpha, and `C_dst`
is the existing color buffer value.

Configured via `D3D11_BLEND_DESC`, binding a `BlendState` object. Each render target
supports independent blend state in D3D11.

### Pre-multiplied Alpha
In **pre-multiplied alpha** representation, the RGB channels are already multiplied by alpha:
`C_premul = C × α`. The blend equation simplifies to additive for the source term:
```
C_out = C_premul_src + C_dst × (1 − α_src)
```
Pre-multiplied alpha is the standard in compositing (DX, WIC, Direct2D) because it avoids
"dark halo" artifacts at transparent edges and is mathematically correct for filtering.

### Alpha Testing vs. Alpha Blending
| | Alpha Test | Alpha Blending |
|---|---|---|
| Mechanism | Discard fragments below threshold | Blend fragment with destination |
| Sorting | Not required | Required (back-to-front) |
| Overdraw | None (discarded early) | Accumulates |
| Use case | Foliage, fences, cutouts | Glass, particles, UI |

Alpha test is performed via `clip(alpha - threshold)` in HLSL (invokes `discard`). In
D3D11+, the fixed-function alpha test state is gone — implement in pixel shader.

### Alpha in Textures
Alpha may be stored:
- **Inline** in the color texture's A channel (`RGBA8`)
- **Separately** in a dedicated single-channel texture (`R8_UNORM`) for independent
  compression control
- **Computed** procedurally in the pixel shader (e.g., distance-field alpha for text)

---

## 7. Render Target

### Definition
A render target is any GPU surface that can be bound as the **write destination** for
rasterization output. It is the general category; the back buffer and G-buffer textures are
both render targets. The term specifically refers to a surface bound via a
**Render Target View (RTV)** in D3D11/12.

### Render-to-Texture (RTT)
The most powerful use of render targets: bind a `Texture2D` resource as an RTV, render a
scene into it, then unbind and bind it as a **Shader Resource View (SRV)** for sampling in
a subsequent pass. This enables:
- **Post-processing**: bloom, depth of field, motion blur, SSAO, FXAA
- **Reflections**: render the reflected viewpoint into a texture, sample during main render
- **Shadow mapping** (see §8)
- **Deferred shading G-buffer** construction
- **Cascaded effects**: chain multiple passes where each consumes the previous pass's output

### MRT (Multiple Render Targets)
Bind up to 8 RTVs simultaneously. A single PS invocation can write to all 8 via
`SV_Target0` through `SV_Target7`. Essential for G-buffer construction in deferred pipelines.

### Resource Flags
A texture used as a render target must be created with `D3D11_BIND_RENDER_TARGET`. To also
sample it as a texture, add `D3D11_BIND_SHADER_RESOURCE`. These flags must be set at
resource creation time — they cannot be changed dynamically.

### Production Notes
- **Resolve**: MSAA render targets cannot be directly sampled. Use `ResolveSubresource` to
  downsample to a non-MSAA texture before sampling.
- **Mip generation**: Render to mip 0, then call `GenerateMips` on the SRV to build the
  full mip chain (requires `D3D11_RESOURCE_MISC_GENERATE_MIPS`).
- Avoid **GPU-CPU round trips**: reading back render target data to CPU is expensive
  (pipeline stall). Use staging textures and async readback with fences (D3D12) or
  `D3D11_MAP_READ` on a staging resource.

---

## 8. Shadow Map / Depth Map

### Definition
A shadow map is a **depth texture** rendered from the point of view of a light source.
During the main camera render, each pixel's world-space position is transformed into the
light's clip space and its depth is compared against the shadow map value at that location.
If the pixel's depth exceeds the stored value, it is occluded from the light — it is in shadow.

### Algorithm (Percentage Closer Filtering variant)

**Pass 1 — Shadow Map Generation:**
```
Camera = Light source (directional: orthographic; spot: perspective; point: cubemap)
Bind depth-only render target (no color buffer needed)
Render scene geometry → writes depth to shadow map texture
```

**Pass 2 — Shadow Lookup in Main Render:**
```hlsl
// In pixel shader:
float4 shadowPos = mul(worldPos, lightViewProjection);
shadowPos.xyz /= shadowPos.w;                      // perspective divide
float2 shadowUV = shadowPos.xy * 0.5 + 0.5;        // NDC → [0,1]
float sceneDepth = shadowPos.z;

float shadowMapDepth = shadowTex.Sample(shadowSampler, shadowUV).r;
float inShadow = (sceneDepth > shadowMapDepth + bias) ? 0.0 : 1.0;
```

### Shadow Acne and the Depth Bias Problem
**Shadow acne**: Self-shadowing artifacts — a surface shadowing itself due to depth
imprecision. The surface's depth in light space approximately equals the shadow map value,
but floating-point rounding makes it slightly greater, marking it erroneously as in shadow.

**Fixes:**
- **Constant depth bias** (`DepthBias` in `D3D11_RASTERIZER_DESC`): offset depth values
  during shadow map generation. Too much bias → **Peter Panning** (shadows detach from casters).
- **Slope-scaled bias** (`SlopeScaledDepthBias`): adapts bias magnitude to surface slope
  relative to light direction. More robust across varying geometry angles.
- **Normal offset**: shift shadow lookup position along the surface normal in world space
  before projecting into light space. Arguably the most robust technique.

### Filtering Techniques

| Technique | Quality | Cost | Notes |
|---|---|---|---|
| Hard shadows (nearest) | Low | Minimal | Aliased edges |
| PCF (Percentage Closer Filtering) | Medium | Low–Medium | Average depth comparisons in neighborhood |
| PCSS (Percentage Closer Soft Shadows) | High | Medium–High | Variable kernel size based on blocker distance |
| VSM (Variance Shadow Maps) | Medium–High | Medium | Store mean + variance; enables hardware bilinear filtering |
| EVSM (Exponential VSM) | High | Medium | Better light bleeding control than VSM |
| Ray-traced shadows | Ground truth | High | DXR / RT cores |

### Cascaded Shadow Maps (CSM)
For directional lights (sun), a single shadow map cannot cover the entire view frustum at
sufficient resolution. **CSM** partitions the camera frustum into N depth slices
(cascades), each rendered with its own shadow map, sized to cover progressively larger world
regions. The pixel shader selects the tightest cascade that contains the current pixel.

Typical 4-cascade split: C0 covers 0–5 m (sharp, high res), C1 5–20 m, C2 20–80 m,
C3 80–500 m (coarse, low res).

### Cube Shadow Maps (Omnidirectional Point Lights)
Point lights cast light in all directions. Six shadow maps (one per face of a cube) are
rendered, or a single pass using a geometry shader to select the appropriate cube face via
`SV_RenderTargetArrayIndex`.

### Production Notes
- Shadow map resolution directly controls edge quality. Common sizes: 1024² (low), 2048²
  (medium), 4096² (high). Memory: a `D32_FLOAT` 2048² map = 16 MB; CSM with 4 cascades = 64 MB.
- Cull back faces during shadow map generation (reverse culling) to further reduce acne on
  closed meshes: `D3D11_CULL_FRONT` in the shadow-pass rasterizer state.
- **Stable CSM**: snap cascade split positions to texel boundaries in light space each frame
  to prevent shadow edge shimmer as the camera moves.
- Parallax correction for cube shadow maps: adjust lookup vector for non-infinitely-distant
  point lights to reduce distortion on nearby surfaces.

---

## Buffer Interaction Summary

The following describes how these buffers cooperate within a single draw call in a standard
forward rendering pass:

```
Vertex Shader → Rasterizer
                    ↓
             [Early-Z test against Depth Buffer]   ← discard occluded fragments
                    ↓
             Pixel Shader executes
                    ↓
             Stencil Test   ← read/write Stencil Buffer
                    ↓
             Depth Test     ← read/write Depth Buffer
                    ↓
             Alpha Blending ← read Color Buffer (dst), write Color Buffer (result)
                    ↓
             Color Buffer (Back Buffer) updated
                    ↓
             IDXGISwapChain::Present()
                    ↓
             Back Buffer ↔ Front Buffer (flip)
                    ↓
             Monitor scans out Front Buffer
```

---

## Format Quick Reference

| Buffer | Typical DXGI Format | Bits/Pixel | Binding |
|---|---|---|---|
| Color (SDR) | `R8G8B8A8_UNORM` | 32 | RTV, SRV |
| Color (HDR) | `R16G16B16A16_FLOAT` | 64 | RTV, SRV |
| Depth only | `D32_FLOAT` | 32 | DSV |
| Depth + Stencil | `D24_UNORM_S8_UINT` | 32 | DSV |
| Depth + Stencil (HP) | `D32_FLOAT_S8X24_UINT` | 64 | DSV |
| Shadow Map | `R32_FLOAT` / `D32_FLOAT` | 32 | DSV (write), SRV (read) |
| G-Buffer Normal | `R16G16B16A16_FLOAT` | 64 | RTV, SRV |
| G-Buffer Albedo | `R8G8B8A8_UNORM` | 32 | RTV, SRV |

---

## Glossary

| Term | Definition |
|---|---|
| **DSV** | Depth-Stencil View — API object binding a texture as depth/stencil target |
| **RTV** | Render Target View — API object binding a texture as a color render target |
| **SRV** | Shader Resource View — API object binding a texture for shader sampling |
| **MRT** | Multiple Render Targets — simultaneous write to multiple RTVs |
| **Overdraw** | Multiple pixel shader invocations writing the same pixel; wastes GPU cycles |
| **Swap Chain** | Queue of back/front buffer surfaces managed by DXGI for tear-free display |
| **Page Flip** | Hardware swap of front/back buffer pointers; no pixel copy, zero cost |
| **Resolve** | Downsample MSAA surface to single-sample surface |
| **Staging Texture** | CPU-readable GPU resource used for async readback |
| **Early-Z** | Hardware optimization: depth test before pixel shader to skip occluded work |
| **Reverse-Z** | Depth convention remapping near→1.0, far→0.0 for superior float precision |
| **PCF** | Percentage Closer Filtering — shadow softening via averaged depth comparisons |
| **CSM** | Cascaded Shadow Maps — frustum-split directional shadow technique |
| **Peter Panning** | Artifact where excessive depth bias detaches shadows from casters |
| **Shadow Acne** | Self-shadowing artifact from depth precision limitations |
| **Z-Fighting** | Depth flicker between coplanar surfaces caused by float precision loss |
| **Tile-Based GPU** | Mobile GPU architecture processing screen tiles in on-chip fast memory |
| **G-Buffer** | Set of MRT textures storing deferred shading inputs (normals, albedo, etc.) |
