---
name: lighting-systems
description: >
  Complete lighting reference: ambient, directional, point, spot, area, tube, disc lights;
  IBL, probe lighting, SSGI, Voxel GI, Radiance Cascades, Light Propagation Volumes, photon
  mapping, light field systems. Covers DX9 light management, CPU light lists, and modern
  clustered light evaluation.
tags: [lighting, ibl, pbr-lighting, probes, ssgi, voxel-gi, radiance-cascades, lpv, area-lights, dx9-lighting]
related_skills: [06_PBR, 07_GI_and_AO, 12_Deferred_Rendering, 13_ForwardPlus_Clustered, 04_Lighting_Systems]
---

# Lighting Systems

## Light Type Reference

| Type | Attenuation | Shadow | DX9 Compatible |
|------|------------|--------|----------------|
| Ambient | None | No | ✓ |
| Directional | None (infinite) | Yes (CSM) | ✓ |
| Point | 1/r² | Yes (cubemap) | ✓ |
| Spot | 1/r² + cone | Yes | ✓ |
| Area (Rect) | Analytical | Approximation | Approx only |
| Tube | LTC approx | No | Approx only |
| Disc | LTC approx | No | Approx only |

---

## Direct Lighting Math

### Lambert Diffuse
```hlsl
float3 LambertDiffuse(float3 albedo, float NdotL)
{
    return albedo * max(NdotL, 0.0f) / 3.14159265f;
}
```

### Point Light Attenuation
```hlsl
float PointLightAttenuation(float distance, float radius)
{
    float d = max(distance, 0.0001f);
    float att = 1.0f / (d * d);
    // Smooth falloff at radius boundary
    float falloff = saturate(1.0f - pow(d / radius, 4.0f));
    return att * falloff * falloff;
}
```

### Spot Light Cone
```hlsl
float SpotLightCone(float3 lightDir, float3 spotDir, float innerAngle, float outerAngle)
{
    float cosAngle = dot(-lightDir, spotDir);
    return saturate((cosAngle - outerAngle) / (innerAngle - outerAngle));
}
```

---

## Image-Based Lighting (IBL)

IBL uses environment maps (cubemaps) to represent distant lighting.

### Diffuse IBL (Irradiance Map)
Pre-convolve environment cubemap into a diffuse irradiance map.
Sample with surface normal:
```hlsl
float3 irradiance = texCUBE(irradianceMap, worldNormal).rgb;
float3 diffuseIBL = irradiance * albedo * (1.0f - metallic);
```

### Specular IBL (Prefiltered Environment Map)
Pre-filter environment at multiple mip levels corresponding to roughness.
```hlsl
float3 R = reflect(-viewDir, worldNormal);
float mipLevel = roughness * MAX_REFLECTION_LOD;
float3 specularIBL = texCUBElod(envMap, float4(R, mipLevel)).rgb;
```

### BRDF LUT (Smith GGX)
Pre-baked 2D texture: x = NdotV, y = roughness → output: scale, bias for F0.
```hlsl
float2 brdfLUT = tex2D(brdfLutMap, float2(NdotV, roughness)).rg;
float3 specular = (F0 * brdfLUT.x + brdfLUT.y) * specularIBL;
```

**DX9 note:** Full IBL requires multiple cubemap lookups. Feasible but expensive. Use single static cubemap + Fresnel approximation for performance.

---

## Probe Lighting

Reflection/irradiance probes placed throughout the scene.

**Types:**
- **Static probes**: Baked offline, read from cubemap array
- **Dynamic probes**: Re-rendered each frame (very expensive, hero assets only)
- **Parallax-corrected cubemaps**: Apply box or sphere correction to reduce swimming

**Probe blending:**
- Blend between nearest probes based on proximity and volume overlap
- Weight by projection relevance

---

## Screen-Space Global Illumination (SSGI)

Ray marches in screen space to gather indirect lighting from visible surfaces.

```
G-Buffer (normals, albedo, depth)
  ↓
Generate hemisphere samples per pixel
  ↓
Ray march in screen space
  ↓
Gather lit surface colors from hit positions
  ↓
Temporal accumulation + spatial denoise
  ↓
Composite
```

**DX9 approximation:** Not truly feasible. Use SSAO as a proxy for indirect occlusion. Full SSGI requires compute shaders.

---

## Voxel GI

Voxelizes scene geometry into a 3D grid, injects light, and propagates.

**Steps:**
1. Voxelize scene (geometry shader or compute)
2. Inject direct light into voxel grid
3. Mipmap voxel grid (cone tracing preparation)
4. Cone trace from surface for diffuse/specular GI

**DX9:** Not feasible without compute. Use lightmaps or probes as substitute.

---

## Radiance Cascades

Hierarchical probe-based GI. Cascades of probe grids at increasing radii.
Inner cascades = fine detail, outer = long-range bounce.
Modern alternative to SSGI for dynamic GI without path tracing.

---

## Light Propagation Volumes (LPV)

Injects direct light into a 3D grid and propagates for one bounce of GI.
- Grid resolution: typically 32³ to 64³
- Propagation steps: 4–8 iterations
- Each cell stores spherical harmonic (SH) coefficients

---

## DX9 Light Management (CPU-Driven)

Since DX9 has no compute shaders, light culling is CPU-side.

**Pattern for CPU-driven tiled lighting:**
```
CPU: Divide screen into 16×16 pixel tiles
CPU: For each tile, frustum-cull all scene lights
CPU: Build per-tile light index lists
CPU: Upload light lists as texture or constant array
PS:  For each pixel, iterate tile's light list
```

**Practical limits for DX9:**
- Up to ~32 lights in constant registers (realistic for SM3.0)
- Up to ~100–300 lights with texture-packed light data

---

## Advanced Light Types (SM5.0+)

### Area Lights (LTC — Linearly Transformed Cosines)
Analytical area light shading using LTC matrix approximation.
Requires LUT texture lookups for M and magnitude.

### Tube Lights
Approximated as a line segment; closest point on line used for specular.

### Disc Lights
Approximated via representative point on disc for specular evaluation.

---

## Lighting in Deferred vs Forward

| System | Deferred | Forward | Forward+ |
|--------|---------|---------|---------|
| Light loop | Per-pixel, post G-buffer | Per-object, per-draw | Per-tile, per-draw |
| Material access | Full G-buffer | In-shader | In-shader |
| Transparency | Separate pass | Native | Native |
| MSAA | Hard | Native | Native |

---

## Related Skills
- Skill 06: PBR — BRDF theory powering all direct/IBL evaluation
- Skill 07: GI and AO — global illumination and occlusion techniques
- Skill 12: Deferred Rendering — lighting pass design
- Skill 13: Forward+ / Clustered — light culling architecture
