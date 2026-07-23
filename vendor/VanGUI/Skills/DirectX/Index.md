---
name: graphics-engine-bible-index
description: >
  Master navigation index for the Graphics Engine Bible 2026 skill set.
  Use this file FIRST to locate the correct sub-skill for any graphics
  programming, DX9 engineering, injector, shader, or legacy modernization task.
  Read this before any other skill in this collection.
tags: [index, navigation, graphics, dx9, rendering, hlsl, injector, pbr, nfsmw]
---

# Graphics Engine Bible 2026 — Skill Navigation Index

## HOW TO USE THIS INDEX

1. Identify your task domain from the table below.
2. Navigate to the referenced sub-skill directory.
3. Read the sub-skill's SKILL.md before writing any code or guidance.
4. Cross-reference related skills listed at the bottom of each sub-skill.

---

## SKILL MAP

| Skill # | Directory | Covers |
|---------|-----------|--------|
| 01 | `01_Rendering_Pipelines/` | Forward, Deferred, Forward+, Clustered, Hybrid, Rasterization, Ray Tracing overview |
| 02 | `02_DX9_Architecture/` | DX9 device creation, swap chains, MRTs, SM3.0 constraints, DX9→DX12 migration |
| 03 | `03_HLSL_Shaders/` | All shader stages (VS/PS/GS/HS/DS/CS), techniques, constant buffers, permutations |
| 04 | `04_Lighting_Systems/` | Ambient, directional, point, spot, area, IBL, probe lighting, LPV, SSGI |
| 05 | `05_Shadow_Systems/` | Shadow maps, CSM, PCF, VSM, EVSM, MSM, contact shadows, ray-traced shadows |
| 06 | `06_PBR/` | Cook-Torrance BRDF, GGX, Fresnel-Schlick, metallic/roughness/specular workflows |
| 07 | `07_GI_and_AO/` | Lightmaps, Voxel GI, Radiance Cascades, SSAO, HBAO, GTAO, SSDO, RTX AO |
| 08 | `08_Reflections_Refraction/` | SSR, stochastic SSR, probe reflections, parallax cubemaps, planar, ray-traced; refraction |
| 09 | `09_Water_Atmosphere_Volumetrics/` | Gerstner waves, FFT oceans, Rayleigh/Mie scattering, volumetric fog/clouds/god rays |
| 10 | `10_Terrain_Vegetation_Characters/` | Heightmaps, splatmaps, virtual textures, foliage, skin/hair/cloth shading |
| 11 | `11_Post_Processing/` | HDR, bloom, DoF, motion blur, color grading, LUT, tonemapping, TAA, FXAA, DLSS, FSR |
| 12 | `12_Deferred_Rendering/` | G-buffer layouts, depth prepass, light accumulation, tiled/clustered deferred, transparency |
| 13 | `13_ForwardPlus_Clustered/` | Tile frustums, compute light culling, cluster construction, depth slicing, GPU data structures |
| 14 | `14_Ray_Tracing/` | DXR, BVH, ray queries, ReSTIR, path tracing, denoisers, hybrid raster+RT |
| 15 | `15_DX9_Injector_Architecture/` | VTable hooks, Present/EndScene/Reset, state blocks, render target capture, depth acquisition |
| 16 | `16_Shader_Replacement/` | Shader hashing, runtime swap, material classification, road/vehicle/reflection shader upgrades |
| 17 | `17_ENB_ReShade_Frameworks/` | Effect chains, buffer sharing, resource lifetimes, post-processing orchestration, plugin systems |
| 18 | `18_NFSMW_Renderer/` | NFS Most Wanted 2005 full render pipeline, vehicle/road/shadow/particle/streaming analysis |
| 19 | `19_Legacy_Engine_Modernization/` | NFSMW, Drift City, GTA SA, Prism3D — phased upgrade roadmaps and constraints |
| 20 | `20_Optimization_Memory/` | Batching, instancing, LOD, HLOD, texture/mesh streaming, GPU profiling, bandwidth |
| 21 | `21_Future_Rendering/` | Neural rendering, radiance fields, mesh shaders, path-traced engines, hybrid neural pipelines |

---

## QUICK LOOKUP — COMMON TASKS

| Task | Go To |
|------|-------|
| Hook IDirect3DDevice9::Present | Skill 15 |
| Write a DX9 G-buffer | Skill 12 + Skill 02 |
| Implement SSAO | Skill 07 |
| Implement SSR | Skill 08 |
| Build ENB-style post chain | Skill 17 |
| PBR shader in SM3.0 | Skill 06 + Skill 03 |
| Tiled/Clustered light culling | Skill 13 |
| Shadow maps + CSM | Skill 05 |
| Deferred renderer design | Skill 12 |
| NFSMW renderer analysis | Skill 18 |
| Modernize Prism3D / Drift City / GTA SA | Skill 19 |
| HLSL shader library (DX9 SM3.0) | Skill 03 |
| Volumetric fog raymarch | Skill 09 |
| HDR + bloom pipeline | Skill 11 |
| Optimize draw calls / memory | Skill 20 |
| Future / neural rendering | Skill 21 |

---

## DX9 MODERNIZATION STACK (canonical order)

```
HDR → Bloom → GTAO → SSR → Volumetric Fog → LUT Color Grading → TAA → Sharpening → Present
```

Each stage has a dedicated skill. Follow Skill 19 for phased rollout guidance per target engine.

---

## SOURCE DOCUMENTS (fully ingested)

- `Graphics_Programming_Encyclopedia_2026.txt` — 36-section master taxonomy
- `Graphics_Engine_Bible_2026_Volume_I.docx` — Full engineering reference outline
- `Graphics_Engine_Bible_2026_Volume_II_Chapter_1.txt` — Modern rendering architecture
- `Graphics_Engine_Bible_2026_Volume_II_Chapter_2_*.txt` — Deferred rendering deep dive
- `Graphics_Engine_Bible_2026_Volume_II_Chapter_3_*.txt` — Forward+ and Clustered rendering
- `Graphics_Engine_Bible_DX9_Supplement.txt` — DX9 deferred, HDR, ENB, stencil volumes
- `DX9_Compatible_Implementation_Guide.txt` — SM3.0 pipeline recipes
- `Graphics_Engine_Bible_NFSMW_DX9_Roadmap.txt` — Chapter roadmap + NFS MW focus
- `NFSMW_2005_Renderer_Architecture_Chapter.txt` — Full NFS MW renderer analysis
- `DX9_Volume_A_Hook_Architecture_and_NFSMW_Shader_RE.docx` — Injector + shader RE
- `DX9_Volume_B_Deferred_Rendering_and_ENB_Frameworks.docx` — DX9 deferred + ENB
- `DX9_Volume_C_HLSL_Shader_Library.docx` — SM3.0 shader library
- `Graphics_Engine_Bible_2026_DX9_Master_Edition.docx` — Compiled master reference
- `Graphics_Engine_Bible_2026_DX9_Encyclopedia_Expanded.docx` — 11-chapter DX9 encyclopedia
