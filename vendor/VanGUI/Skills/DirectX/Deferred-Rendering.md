---
name: deferred-rendering-architecture
description: >
  Complete deferred rendering reference: G-buffer layouts, depth prepass, normal encoding,
  light accumulation passes, light volumes (sphere/cone/capsule), stencil optimization,
  tiled deferred, clustered deferred, transparency handling, shadow integration, screen-space
  effect integration, memory considerations, bandwidth reduction. Full DX9 SM3.0 deferred
  pipeline with MRT constraints and stencil light volume techniques.
tags: [deferred, g-buffer, gbuffer, mrt, depth-prepass, light-volumes, stencil, tiled-deferred, clustered-deferred, dx9-deferred]
related_skills: [01_Rendering_Pipelines, 13_ForwardPlus_Clustered, 05_Shadow_Systems, 07_GI_and_AO, 02_DX9_Architecture]
---

# Deferred Rendering Architecture

## Why Deferred?

Traditional forward rendering complexity:

```
O(Objects × Lights)
```

Deferred rendering shifts cost to:

```
O(Visible Pixels × Lights)
```

For dense scenes with many dynamic lights, this is significantly more efficient.
The key insight: geometry is processed once; lighting is evaluated in screen space.

---

## Core Pipeline

```
Scene Geometry
  ↓
[Depth Prepass] (optional but recommended)
  ↓
G-Buffer Pass (write material/geometry data to MRTs)
  ↓
[SSAO / Screen-Space Effects] (uses G-buffer)
  ↓
Lighting Pass (read G-buffer, evaluate BRDF per light)
  ↓
[Shadow Map Sampling in Lighting Pass]
  ↓
Transparent / Forward Pass
  ↓
Post Processing
  ↓
Present
```

---

## G-Buffer Layouts

### Modern Engine (DX11/DX12) — Full Layout
```
RT0: Albedo.rgb       | AO
RT1: Normal.xyz       | Roughness
RT2: Metallic         | Specular | Material Flags
RT3: Emissive.rgb     | Custom Data
Depth: Hardware Depth Buffer (D24S8 or D32F)
Velocity: Motion Vectors (RG16F or RG8)
```

### DX9 SM3.0 — Constrained Layout (4 MRTs max)
```
RT0: Albedo.rgb       | Specular.a         (A8R8G8B8)
RT1: Normal.xy        | Roughness.a        (A8R8G8B8 or A16B16G16R16F)
RT2: Material Flags   | AO | Custom Data   (A8R8G8B8)
Depth: D24S8          (shared for depth + stencil)
```

**No dedicated emissive RT in DX9 (budget too tight).** Pack emissive data in Material Flags or add a forward pass for emissive objects.

---

## Depth Prepass

Renders only depth values before the G-buffer pass.

**Benefits:**
- Early-Z rejection: pixels failing depth test skip G-buffer writes entirely
- Reduces overdraw: only the frontmost pixel's material data is written
- Improves cache coherence: depth buffer warmed up
- More stable GPU performance

**Implementation:**
```
SetColorWriteMask(0)        // disable color writes
Set depth write = TRUE
Render all opaque geometry
Set depth write = FALSE (or EQUAL test)
Proceed to G-buffer pass
```

---

## Normal Encoding

Efficient normal storage is critical for G-buffer memory.

### Option A: Full XYZ (24 bits / component)
Stores all 3 components. Simple but wastes space.
Normals are unit vectors — Z is redundant.

### Option B: Two-Component Reconstruction
Store only X and Y, reconstruct Z:
```hlsl
float z = sqrt(1.0f - saturate(x*x + y*y));
```
Requires care near Z=0 (grazing angles can be inaccurate).

### Option C: Octahedral Encoding (Recommended)
Best quality-per-bit. Maps hemisphere to octahedron, then to square.
```hlsl
// Encode
float2 OctEncode(float3 n)
{
    n /= (abs(n.x) + abs(n.y) + abs(n.z));
    if (n.z < 0)
        n.xy = (1.0f - abs(n.yx)) * sign(n.xy);
    return n.xy * 0.5f + 0.5f;
}

// Decode
float3 OctDecode(float2 e)
{
    float2 f = e * 2.0f - 1.0f;
    float3 n = float3(f.x, f.y, 1.0f - abs(f.x) - abs(f.y));
    if (n.z < 0)
        n.xy = (1.0f - abs(n.yx)) * sign(n.xy);
    return normalize(n);
}
```

### DX9 Option: Two-Component with Z Reconstruction
```hlsl
// Per DX9 Supplement source material:
float3 DecodeNormal(float2 enc)
{
    float2 f = enc * 2.0f - 1.0f;
    float z = sqrt(1.0f - saturate(dot(f, f)));
    return normalize(float3(f.x, f.y, z));
}
```

---

## G-Buffer Memory Budget

At 1920×1080 with 4 RTs at RGBA8:
- Each RT: 1920 × 1080 × 4 bytes = ~8 MB
- 4 RTs: ~32 MB
- Plus depth (D24S8): ~8 MB
- Plus velocity: ~4 MB
- Total: **~44 MB per frame**

With RGBA16F:
- Each RT: ~16 MB
- 4 RTs: ~64 MB
- Total: **~76 MB per frame** (without considering MSAA)

**Optimization strategies:**
- Pack data carefully (normal X/Y, roughness, AO into one RGBA8 RT)
- Use R11G11B10F for normals + material data where precision allows
- Skip velocity buffer if no TAA

---

## Lighting Pass — Position Reconstruction

Rather than storing world position in a G-buffer RT (expensive), reconstruct from depth.

```hlsl
float3 ReconstructWorldPos(float2 screenUV, float depth, float4x4 invViewProj)
{
    float4 ndcPos = float4(screenUV * 2.0f - 1.0f, depth, 1.0f);
    float4 worldPos = mul(ndcPos, invViewProj);
    return worldPos.xyz / worldPos.w;
}
```

---

## Light Volumes (DX9 Deferred)

Rather than fullscreen lighting, restrict lighting calculations to light-affected regions using geometry volumes.

### Sphere (Point Light)
```
Render sphere mesh at light position with radius = light range
  ↓
Stencil test restricts pixel shader to sphere interior
  ↓
Evaluate point light BRDF on affected pixels only
  ↓
Additive blend into accumulation buffer
```

### Cone (Spot Light)
```
Render cone mesh aligned to spot light direction
  ↓
Same stencil + lighting process
```

---

## Stencil Light Volume Optimization

Critical DX9 deferred optimization:

```
Pass 1 (FRONT FACES, depth FAIL → stencil++):
  SetCullMode(CCW)
  SetStencilOp(KEEP, INCR, KEEP)
  Render light volume → marks stencil where camera is INSIDE volume

Pass 2 (BACK FACES, depth FAIL → stencil--):
  SetCullMode(CW)
  SetStencilOp(KEEP, DECR, KEEP)
  Render light volume

Pass 3 (Lighting):
  SetStencilFunc(NOTEQUAL, 0)  // only shade marked pixels
  Fullscreen quad → lighting shader
  → skips pixels outside volume
```

Benefits:
- Dramatically reduces pixel shader invocations
- Scales to 50–100+ dynamic lights in DX9

---

## Tiled Deferred Rendering

Screen divided into 16×16 pixel tiles.
Per-tile light lists built via compute shader (SM5.0+) or CPU (DX9).

**Tiled vs Light Volume:**
| | Light Volume | Tiled Deferred |
|-|-------------|----------------|
| Draw calls | 1 per light | 1 fullscreen + compute |
| Overdraw | Minimal | None |
| Compute shader | No | Yes (SM5+) |
| DX9 | Yes | Approx (CPU-side only) |

---

## Clustered Deferred Rendering

Extends tiled to 3D clusters (X, Y, Z). See Skill 13 for full detail.

---

## Transparency Challenges

Deferred rendering stores only 1 surface per pixel in the G-buffer.
Multiple transparent layers cannot coexist.

**Solutions:**
1. **Forward transparency pass**: After deferred lighting, render transparent objects with forward shading
2. **Weighted Blended OIT (WBOIT)**: Accumulate weighted alpha contributions, resolve in separate pass
3. **Per-pixel linked lists** (SM5.0+): Store all transparent fragments per pixel, sort and resolve

**Most AAA engines use Hybrid Deferred + Forward Transparent.**

---

## Shadow Integration in Deferred

During the lighting pass:
```hlsl
// For each active light:
float shadow = SampleShadowMap(shadowMap, worldPos);
float3 lightContrib = EvaluateBRDF(gbuffer, lightParams) * shadow;
accum += lightContrib;
```

Shadow maps are sampled per-light, per-pixel during the lighting pass. Works naturally with deferred.

---

## Screen-Space Effect Integration

G-buffer provides ideal inputs for:
- **SSAO**: depth + normals already available
- **SSR**: depth + normals + lit color buffer
- **SSGI**: depth + normals + lit color
- **TAA**: velocity buffer (motion vectors)

---

## DX9 Deferred — Production Checklist

```
✓ Use D24S8 for depth + stencil (shared resource)
✓ Pack normals as 2-component (save RT bandwidth)
✓ Prefer A8R8G8B8 over A16B16G16R16F where precision allows
✓ Use stencil light volumes for point/spot lights
✓ Accumulate lighting additively into FP16 HDR buffer
✓ Handle device Reset: recreate all D3DPOOL_DEFAULT render targets
✓ Cap MRT count at 4 (DX9 hard limit)
✓ Build light lists on CPU, upload as constants (no compute)
✓ Half-resolution SSAO, full-resolution composite
✓ Forward pass for transparencies after deferred lighting
```

---

## Related Skills
- Skill 01: Rendering Pipelines — pipeline selection context
- Skill 13: Forward+ / Clustered — light culling in deferred
- Skill 02: DX9 Architecture — MRT format selection, state management
- Skill 05: Shadow Systems — shadow integration with deferred
