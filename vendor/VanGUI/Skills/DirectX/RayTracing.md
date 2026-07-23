---
name: ray-tracing-and-path-tracing
description: >
  DXR ray tracing: BVH construction, ray generation, any-hit/closest-hit/miss shaders,
  ray queries; reflection rays, shadow rays, GI rays. Path tracing: Monte Carlo integration,
  importance sampling, denoisers (SVGF, ReLAX), ReSTIR (reservoir sampling), hybrid
  raster+RT pipelines. Not DX9-compatible.
tags: [ray-tracing, dxr, bvh, path-tracing, restir, denoiser, shadow-rays, gi-rays, reflection-rays, monte-carlo]
related_skills: [01_Rendering_Pipelines, 04_Lighting_Systems, 08_Reflections_Refraction, 05_Shadow_Systems]
---

# Ray Tracing and Path Tracing

> **DX9 Note:** Ray tracing (DXR) requires DirectX 12 and Turing/RDNA2+ GPU hardware.
> None of the techniques in this skill apply to DX9/SM3.0 engines.
> For DX9 approximations of RT effects, see Skills 05 (Shadows), 07 (AO), 08 (Reflections).

---

## DXR — DirectX Raytracing

### Ray Tracing Pipeline Stages

| Shader | Purpose |
|--------|---------|
| Ray Generation | Fires rays from camera / effect origin |
| Intersection | Tests custom geometry (spheres, etc.) |
| Any-Hit | Called on potential hit, can accept/reject (e.g., alpha test) |
| Closest-Hit | Called at final intersection, evaluates shading |
| Miss | Called when ray hits nothing |

### Dispatch
```hlsl
DispatchRays(rayGenShader, hitGroup, missShader, width, height, 1);
```

---

## BVH — Bounding Volume Hierarchy

DXR manages BVH automatically. Two levels:

**TLAS (Top-Level Acceleration Structure):**
- Contains instance transforms + references to BLASes
- Rebuilt each frame (fast) when instances move

**BLAS (Bottom-Level Acceleration Structure):**
- Contains triangle geometry
- Built once per mesh (slow)
- Can be updated (deformed meshes) or compacted

```cpp
// Build BLAS from mesh
D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS blasInputs{};
blasInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
blasInputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
```

---

## Ray Queries (Inline Ray Tracing, SM6.5)

Allows any shader stage (including PS, CS) to fire rays without a full RT pipeline.

```hlsl
RayQuery<RAY_FLAG_CULL_BACK_FACING_TRIANGLES> query;
query.TraceRayInline(accelerationStructure, 0, 0xFF, ray);
query.Proceed();
if (query.CommittedStatus() == COMMITTED_TRIANGLE_HIT)
{
    float t = query.CommittedRayT();
    // hit found
}
```

---

## Ray Types

### Reflection Rays
Fire one ray per pixel along the reflection vector.
Use closest-hit shader to shade the hit surface, return color to caller.

```hlsl
// In ray generation shader
float3 R = reflect(-viewDir, normal);
RayDesc ray;
ray.Origin = worldPos + normal * 0.01f;
ray.Direction = R;
ray.TMin = 0.001f;
ray.TMax = maxReflectionDistance;
TraceRay(TLAS, 0, 0xFF, 0, 1, 0, ray, payload);
reflectionColor = payload.color;
```

### Shadow Rays
Fire a ray from surface toward light. Any intersection = in shadow.

```hlsl
RayDesc shadowRay;
shadowRay.Origin = worldPos + normal * bias;
shadowRay.Direction = normalize(lightPos - worldPos);
shadowRay.TMin = 0.001f;
shadowRay.TMax = length(lightPos - worldPos);
TraceRay(TLAS, RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER,
         0xFF, SHADOW_HIT_GROUP, 1, SHADOW_MISS, shadowRay, shadowPayload);
float visibility = shadowPayload.missed ? 1.0f : 0.0f;
```

### GI Rays
Bounce rays across scene surfaces to gather indirect illumination.
Each hit evaluates diffuse albedo × incoming radiance.

---

## Path Tracing

Full Monte Carlo integration of the rendering equation:

```
L(x, ω) = Le(x, ω) + ∫ f(x, ω, ω') L(x', ω') (ω' · n) dω'
```

### Algorithm
```
For each pixel:
  Fire primary ray
  At hit: sample BRDF to get new direction
  Fire secondary ray
  Repeat for N bounces
  Accumulate radiance
Average over many samples (frames or per-frame multi-sample)
```

### Monte Carlo Integration
Estimate integral by random sampling:
```
Estimate = (1/N) × Σ f(xᵢ) / p(xᵢ)
```
Where p(x) is the probability density of the sample.

### Importance Sampling
Sample directions according to BRDF shape to reduce variance.
GGX NDF importance sampling (see Skill 06) produces much faster convergence than cosine sampling.

---

## ReSTIR — Reservoir-Based Spatial-Temporal Importance Resampling

Dramatically improves path tracing quality for many-light scenes.

**Core idea:** Maintain a reservoir of candidate samples per pixel. Combine with neighbors (spatial reuse) and previous frames (temporal reuse).

**Stages:**
1. **Initial candidate sampling:** Sample M candidate lights per pixel
2. **Temporal reuse:** Combine current reservoir with previous frame's (motion-reprojected)
3. **Spatial reuse:** Combine with neighboring pixels' reservoirs
4. **Final evaluation:** Evaluate full visibility/shading for accepted sample

Results in effectively millions of light samples per pixel at the cost of a few dozen reservoir operations.

---

## Denoisers

Path tracing produces noisy output at 1 sample/pixel. Denoising is mandatory.

| Denoiser | Technique | Notes |
|----------|-----------|-------|
| SVGF | Spatiotemporal Variance-Guided Filtering | AMD/Academic |
| ReLAX | Recurrent Least Squares | NVIDIA, in NRD |
| ReBLUR | Blur-based radius | NVIDIA, in NRD |
| Optix Denoiser | CNN-based | NVIDIA hardware |
| Intel OIDN | CNN-based, CPU+GPU | Open-source |

**Denoiser inputs:** noisy color, albedo buffer, normal buffer, motion vectors.
**Output:** denoised frame, temporally stable.

---

## Hybrid Raster + Ray Tracing (Production Approach)

Most production engines use:

```
Rasterization for primary visibility (G-buffer)
  ↓
Ray-Traced Shadows (1 ray/pixel + denoiser)
  ↓
Ray-Traced Reflections (1 ray/pixel + denoiser)
  ↓
Ray-Traced AO (1-2 rays/pixel + denoiser)
  ↓
Ray-Traced GI (ReSTIR DI + ReSTIR GI + denoiser)
```

This provides near-photorealistic results while maintaining real-time performance by using rasterization for the expensive primary visibility step.

---

## BVH Update Strategies

| Scenario | Strategy | Cost |
|----------|---------|------|
| Static mesh | Build once, never update | Very low |
| Skinned character | Refit BLAS each frame | Low |
| Fully deforming | Rebuild BLAS each frame | High |
| Moving instances | Rebuild TLAS each frame | Very low |

**Compaction:** After building, compact BLAS to reduce memory by 30–60%.

---

## Related Skills
- Skill 01: Rendering Pipelines — hybrid pipeline context
- Skill 08: Reflections — RT vs SSR comparison
- Skill 05: Shadows — RT shadows vs shadow maps
- Skill 07: GI and AO — RTXGI, RT AO
