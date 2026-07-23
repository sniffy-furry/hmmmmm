---
name: global-illumination-and-ambient-occlusion
description: >
  Global illumination and ambient occlusion techniques: lightmaps, dynamic GI,
  Voxel GI, Radiance Cascades, RTX GI, probe grids, SSGI, sparse voxel octrees;
  SSAO, HBAO, HBAO+, GTAO, SSDO, Ray-Traced AO, Horizon AO. DX9-compatible
  depth-based AO approximation pipelines.
tags: [gi, global-illumination, ao, ssao, hbao, gtao, voxel-gi, radiance-cascades, lightmaps, ssdo, rtx-gi, dx9-ao]
related_skills: [04_Lighting_Systems, 12_Deferred_Rendering, 03_HLSL_Shaders]
---

# Global Illumination and Ambient Occlusion

## Global Illumination Techniques

| Technique | Dynamic | Quality | DX9 | Cost |
|-----------|---------|---------|-----|------|
| Lightmaps | No | High | ✓ | Bake cost |
| Probe Grids | Partial | Medium | ✓ | Medium |
| SSGI | Yes | Screen-limited | Compute needed | High |
| Voxel GI (VXGI) | Yes | High | ✗ | Very High |
| Radiance Cascades | Yes | High | ✗ | High |
| RTX GI | Yes | Excellent | ✗ | Very High |
| Sparse Voxel Octrees | Yes | High | ✗ | High |
| LPV | Yes | Medium | ✗ | Medium |

---

## Lightmaps

Pre-baked static GI encoded into UV-unwrapped textures per mesh.

**Requirements:**
- Unique UV2 channel (no overlapping UVs)
- High-quality offline bake (path tracer recommended)
- Texel density matched to world-space surface area

**DX9:** Fully compatible. Sample lightmap in pixel shader and multiply by albedo.
```hlsl
float3 lightmapColor = tex2D(lightmapSampler, lightmapUV).rgb;
float3 finalColor = albedo * lightmapColor;
```

**Limitations:** No dynamic object GI, large texture memory.

---

## Probe Grids

Irradiance probes placed at regular intervals in the world (or adaptively).
Each probe stores a low-resolution spherical harmonic (L1 or L2) representation.

**L2 SH (9 coefficients per color channel):**
Sufficient to represent smooth low-frequency irradiance.

**Runtime evaluation:**
```hlsl
// SH L1 irradiance evaluation (4 coefficients)
float3 SHEval(float3 sh[4], float3 N)
{
    float4 b = float4(0.282095f, 0.488603f * N.y,
                      0.488603f * N.z, 0.488603f * N.x);
    return sh[0]*b.x + sh[1]*b.y + sh[2]*b.z + sh[3]*b.w;
}
```

**DX9:** Store SH coefficients in constant registers or a texture lookup table.

---

## SSGI — Screen Space Global Illumination

Uses depth and color buffer to approximate one-bounce indirect lighting.

**Pipeline:**
```
G-Buffer (normal, albedo, depth, lit scene color)
  ↓
Generate hemisphere directions per pixel
  ↓
Ray march in screen space using depth buffer
  ↓
Sample lit color buffer at hit position
  ↓
Weight by cosine and distance
  ↓
Temporal accumulation + bilateral denoise
  ↓
Add to diffuse lighting term
```

**Not DX9 compatible.** Requires compute shader for efficient hemisphere sampling.
DX9 substitute: SSAO for occlusion, probes for indirect color.

---

## Voxel GI (VXGI)

**Steps:**
1. Voxelize scene into 3D grid (geometry shader or compute)
2. Inject direct light per voxel
3. Mipmap voxel grid (anisotropic mips per face)
4. Cone trace from surface:
   - 1 specular cone: narrow, along reflection vector
   - 3–5 diffuse cones: hemisphere, wider

**Requires SM5.0.** Not DX9 compatible.

---

## Radiance Cascades

Replaces traditional probe systems with a hierarchical cascade structure.
- Multiple cascades at increasing spatial frequency
- Inner cascades: dense probes, short range
- Outer cascades: sparse probes, long range
- Merged from inner to outer for final irradiance

Developed for DX12-class hardware, not DX9 compatible.

---

## LPV — Light Propagation Volumes

Injects direct lighting into a 3D grid of SH volumes, then propagates for 1 bounce.

**Pipeline:**
```
Reflective Shadow Map → inject light into LPV grid → propagate N times → sample at runtime
```

**Grid size:** 32³ to 64³. **Steps:** 4–8 propagation passes.
Can be approximated in DX9 with ping-pong textures, but expensive.

---

## Ambient Occlusion Techniques

| Technique | Quality | Temporal | DX9 | Notes |
|-----------|---------|----------|-----|-------|
| SSAO | Medium | No | ✓ | Classic, sample hemisphere around normal |
| HBAO | Good | No | Partial | Horizon-based, better quality |
| HBAO+ | Better | Yes | ✗ | NVIDIA specific |
| GTAO | Best screen-space | Yes | ✗ | Ground-truth AO |
| SSDO | High | Yes | ✗ | Adds indirect color |
| Horizon AO | Medium | No | ✓ | Simple, fast |
| Ray-Traced AO | Excellent | Yes | ✗ | DXR required |

---

## SSAO — Screen Space Ambient Occlusion

Samples hemisphere in view space around each pixel's normal to estimate occlusion.

**Pipeline:**
```
Depth Buffer + Normal Buffer
  ↓
For each pixel: sample N random hemisphere points
  ↓
Project each sample to screen space
  ↓
Compare sample depth vs depth buffer
  ↓
Accumulate occlusion factor
  ↓
Bilateral blur (preserve edges)
  ↓
Composite (multiply AO term into lighting)
```

**DX9 implementation:**
```hlsl
// Simplified SSAO sample loop (SM3.0)
float ao = 0.0f;
float3 normal = DecodeNormal(tex2D(normalMap, uv).xy);
for (int i = 0; i < NUM_SAMPLES; i++)
{
    float3 sampleDir = reflect(samples[i], randVec);
    if (dot(sampleDir, normal) < 0.0f) sampleDir = -sampleDir;
    float4 samplePos = mul(float4(viewPos + sampleDir * radius, 1.0f), projMatrix);
    samplePos.xy /= samplePos.w;
    float2 sampleUV = samplePos.xy * 0.5f + 0.5f;
    float sampleDepth = tex2D(depthMap, sampleUV).r;
    ao += (sampleDepth < viewPos.z - 0.05f) ? 1.0f : 0.0f;
}
ao = 1.0f - (ao / NUM_SAMPLES);
```

**Recommended:** 16–32 samples, half-resolution, followed by bilateral blur.

---

## HBAO — Horizon-Based Ambient Occlusion

Marches along directions on the hemisphere, tracking the maximum horizon angle.
More physically accurate than SSAO. Avoids halo artifacts.

**Steps per pixel:**
1. For N directions: march ray along screen
2. Track max horizon elevation
3. Integrate occlusion using sine of horizon angle

**DX9 approximation:** Feasible with 4–8 directions, 4–8 steps each. More expensive than SSAO.

---

## GTAO — Ground Truth Ambient Occlusion

State-of-the-art screen-space AO. Integrates over the hemisphere using bent normals.
Produces the most accurate AO of any screen-space technique.

**Requires:** Compute shader or extensive PS loop. SM5.0 ideal. Not DX9 native.

---

## SSDO — Screen Space Directional Occlusion

Extends SSAO with indirect color bleeding. Samples lit scene color at occluded positions.
Adds indirect light color in addition to occlusion factor.

---

## Bilateral Blur (Edge-Preserving)

All AO techniques require bilateral filtering to denoise:
```hlsl
float BilateralBlur(sampler2D aoMap, sampler2D depthMap, float2 uv, float2 dir)
{
    float center = tex2D(depthMap, uv).r;
    float ao = 0.0f;
    float weight = 0.0f;
    for (int i = -BLUR_RADIUS; i <= BLUR_RADIUS; i++)
    {
        float2 offset = dir * i * texelSize;
        float d = tex2D(depthMap, uv + offset).r;
        float w = exp(-abs(d - center) * depthWeight) * gaussianWeight[abs(i)];
        ao += tex2D(aoMap, uv + offset).r * w;
        weight += w;
    }
    return ao / weight;
}
```

---

## DX9 AO Pipeline (Recommended)

```
Depth Buffer → Reconstruct Normal (from depth differentials or normal RT)
  ↓
SSAO (16 samples, half-resolution)
  ↓
Horizontal Bilateral Blur
  ↓
Vertical Bilateral Blur
  ↓
Composite (multiply into ambient lighting term)
```

---

## Related Skills
- Skill 04: Lighting Systems — how AO and GI integrate with light evaluation
- Skill 12: Deferred Rendering — AO composited during lighting pass
- Skill 03: HLSL Shaders — AO sample loop and normal reconstruction code
