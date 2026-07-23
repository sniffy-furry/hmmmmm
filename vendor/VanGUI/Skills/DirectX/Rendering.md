---
name: rendering-pipelines
description: >
  Covers all major real-time rendering pipeline architectures: Forward, Deferred,
  Forward+, Clustered, Hybrid, Tiled, Software, Rasterization, Ray Tracing, Path Tracing,
  Photon Tracing, Voxel Rendering. Includes frame lifecycle, CPU/GPU responsibilities,
  and render graph systems. Use this skill for any pipeline selection, design, or
  architecture question.
tags: [rendering, pipeline, forward, deferred, clustered, forward-plus, rasterization, ray-tracing, frame-graph]
related_skills: [12_Deferred_Rendering, 13_ForwardPlus_Clustered, 14_Ray_Tracing, 20_Optimization_Memory]
---

# Rendering Pipelines

## Pipeline Taxonomy

### Rasterization-Based
| Pipeline | Complexity | Transparency | Many-Light Perf | Memory |
|----------|-----------|--------------|-----------------|--------|
| Forward | O(Objects × Lights) | Native | Poor | Low |
| Deferred | O(Pixels × Lights) | Hard | Excellent | High |
| Forward+ | O(Objects × TileLights) | Native | Good | Medium |
| Clustered Forward | O(Objects × ClusterLights) | Native | Very Good | Medium+ |
| Tiled Deferred | O(Pixels × TileLights) | Hard | Excellent | High |
| Clustered Deferred | O(Pixels × ClusterLights) | Hard | Best | High |
| Hybrid | Mixed | Mixed | Excellent | High |

### Ray/Path Based
- **Ray Tracing**: DXR hardware, BVH traversal, reflection/shadow/GI rays
- **Path Tracing**: Full Monte Carlo integration, ReSTIR, denoisers
- **Photon Tracing**: Bidirectional, photon maps
- **Voxel Rendering**: SVO-based, sparse voxel octrees

---

## Frame Lifecycle (Modern Engine)

```
Input
  ↓
Simulation / Game Logic (CPU)
  ↓
Visibility Determination (Frustum + Occlusion Culling)
  ↓
Render Graph Construction
  ↓
Shadow Passes
  ↓
Depth Prepass
  ↓
Geometry / G-Buffer Passes
  ↓
Lighting Passes
  ↓
Screen-Space Effects (SSAO, SSR, SSGI)
  ↓
Transparency / Forward Pass
  ↓
Post Processing (Bloom, DoF, Motion Blur, Color Grade)
  ↓
Temporal Reconstruction (TAA / TSR / DLSS / FSR)
  ↓
Presentation
```

---

## CPU vs GPU Responsibilities

**CPU:**
- Game logic and simulation
- Scene graph updates
- Resource streaming decisions
- Render command generation
- Visibility culling (broad phase)
- CPU-side light list management (DX9)

**GPU:**
- Rasterization and shading
- Compute workloads (light culling, particles)
- Ray tracing (DXR)
- Texture sampling and filtering
- All per-pixel operations

---

## Render Graph Systems

Modern engines use a render graph (frame graph) to manage pass dependencies.

**Benefits:**
- Automatic resource lifetime tracking
- Automatic pipeline barrier insertion
- Pass scheduling and reordering
- Memory aliasing between transient resources
- Parallel pass execution

**Typical pass chain:**
```
Depth Prepass → GBuffer → SSAO → Lighting → SSR → Bloom → Tone Mapping → TAA → Present
```

---

## Pipeline Selection Guide

**Choose Deferred when:**
- Many dynamic lights (50+)
- Complex material evaluation needed
- Screen-space effects are central (SSAO, SSR)
- MSAA is not required

**Choose Forward+ when:**
- Transparency is heavy (particles, hair, glass)
- MSAA is required
- Memory budget is tight
- Simpler materials

**Choose Clustered when:**
- Open-world scenes with volumetrics
- Depth range is large
- More than 200 visible lights
- Volumetric fog/probes needed per cluster

**Choose Hybrid when:**
- Opaque = deferred, transparent = forward
- Most AAA production scenarios

---

## DX9-Specific Pipeline Constraints

DX9 / Shader Model 3.0 lacks:
- Compute shaders → no GPU-driven light culling
- UAVs → no read/write buffers in shaders
- Descriptor heaps → manual state management
- Mesh shaders → traditional vertex/index pipeline only

DX9 pipeline recommendation:
```
Depth Prepass → G-Buffer (MRT, ≤4 RTs) → Stencil Light Volumes → Light Accumulation → HDR Composition → Post Processing → Present
```

---

## Related Skills
- Skill 12: Deferred Rendering — full G-buffer and lighting pass detail
- Skill 13: Forward+ and Clustered — tile/cluster construction and GPU culling
- Skill 14: Ray Tracing — DXR, BVH, ReSTIR
- Skill 20: Optimization — LOD, culling, batching integration with pipelines
