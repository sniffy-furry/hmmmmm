---
name: terrain-vegetation-character-rendering
description: >
  Terrain: heightmaps, tessellation, blending, splatmaps, virtual textures, terrain AO,
  procedural terrain. Vegetation: grass/tree systems, wind simulation, leaf shading,
  subsurface scattering, seasonal variations, procedural foliage. Characters: skin shading,
  subsurface scattering, hair rendering, anisotropic hair, eye rendering, corneal reflections,
  cloth shading.
tags: [terrain, vegetation, grass, foliage, character, skin, sss, hair, anisotropic, cloth, heightmap, splat]
related_skills: [06_PBR, 04_Lighting_Systems, 03_HLSL_Shaders]
---

# Terrain, Vegetation, and Character Rendering

---

## TERRAIN RENDERING

### Heightmaps
Terrain geometry stored as 2D texture where each texel represents surface height.
- Format: R16 (16-bit single channel) preferred for precision
- Sampled in VS to displace vertex positions
- Normal reconstructed from neighboring texels or pre-baked

**Height → normal reconstruction:**
```hlsl
float3 HeightToNormal(sampler2D heightMap, float2 uv, float texelSize, float heightScale)
{
    float hL = tex2D(heightMap, uv - float2(texelSize, 0)).r;
    float hR = tex2D(heightMap, uv + float2(texelSize, 0)).r;
    float hD = tex2D(heightMap, uv - float2(0, texelSize)).r;
    float hU = tex2D(heightMap, uv + float2(0, texelSize)).r;
    float3 normal = normalize(float3((hL - hR) * heightScale, 2.0f, (hD - hU) * heightScale));
    return normal;
}
```

### Terrain Tessellation (SM5.0)
Hull + Domain shaders add geometry dynamically based on camera distance.
Not DX9 compatible — use pre-tessellated LOD meshes instead.

### Terrain Blending / Splatmaps
Splat map = RGBA texture where R/G/B/A channels are blend weights for 4 layers.

```hlsl
float3 TerrainBlend(float4 weights, sampler2D tex0, sampler2D tex1, sampler2D tex2, sampler2D tex3, float2 uv)
{
    float3 c0 = tex2D(tex0, uv).rgb;
    float3 c1 = tex2D(tex1, uv).rgb;
    float3 c2 = tex2D(tex2, uv).rgb;
    float3 c3 = tex2D(tex3, uv).rgb;
    return c0*weights.r + c1*weights.g + c2*weights.b + c3*weights.a;
}
```

**Height-blend improvement:** Multiply weights by height channel from each layer to get sharper blend boundaries.

### Virtual Textures (SVT)
Streams terrain texture tiles on demand from a virtual texture atlas.
Requires indirection table texture + physical texture atlas.
High complexity. DX9 feasible with careful implementation.

### Terrain AO
Pre-baked horizon-based AO stored in a texture. Sampled at runtime.
Essential for terrain in deferred pipelines.

### Procedural Terrain
Generated via noise functions (Perlin, Simplex, Worley) at runtime.
Heightmap written from procedural formula, then rendered as above.

---

## VEGETATION RENDERING

### Grass Systems
- Geometry generated per-patch via geometry shader (SM4+) or pre-instanced (DX9)
- Each blade: a quad billboard or 2-3 crossing quads
- Wind: sine-wave offset based on world position + time

**DX9 grass approach:** Pre-generate instanced grass quads, animate in VS.

### Tree Systems
- LOD0: Full geometry with leaves
- LOD1–2: Simplified branches + billboard leaves
- LOD3+: Single billboard card
- Impostors: Pre-rendered views, faded in at far distances

### Wind Simulation
```hlsl
float3 ApplyWind(float3 worldPos, float3 windDir, float windStrength, float time, float flexibility)
{
    float phase = dot(worldPos.xz, windDir.xz) * 0.5f + time;
    float sway = sin(phase) * windStrength * flexibility;
    return worldPos + float3(windDir.x * sway, sway * 0.2f, windDir.z * sway);
}
```

### Leaf Shading (Two-sided)
Leaves are thin; light transmits through them.
```hlsl
float NdotL_front = saturate(dot(normal, lightDir));
float NdotL_back  = saturate(dot(-normal, lightDir)) * translucentAmount;
float3 leafLight = (NdotL_front + NdotL_back) * lightColor * albedo;
```

### Subsurface Scattering (Vegetation)
Simplified SSS for leaves: wrap lighting + translucency.
```hlsl
float wrap = 0.5f;
float wrapNdotL = saturate((dot(normal, lightDir) + wrap) / (1.0f + wrap));
float3 sssLight = wrapNdotL * lightColor * subsurfaceColor;
```

---

## CHARACTER RENDERING

### Skin Shading
Human skin requires subsurface scattering to look realistic.

**Pre-integrated SSS (fast, DX9 compatible):**
Pre-bake curvature-based SSS into a 2D lookup texture:
- X axis: NdotL
- Y axis: curvature (1/radius)
- Output: scattered diffuse color

```hlsl
float curvature = length(fwidth(worldNormal)) / length(fwidth(worldPos));
float2 sssUV = float2(NdotL * 0.5f + 0.5f, curvature);
float3 sssColor = tex2D(sssLUT, sssUV).rgb * lightColor;
```

### Screen-Space SSS (SM5.0+)
Blur diffuse lighting in screen space, weighted by depth similarity.
Not DX9 compatible in full form. Use pre-integrated LUT instead.

### Hair Rendering
Hair requires anisotropic specular (Kajiya-Kay or Marschner model).

**Kajiya-Kay (DX9 compatible):**
```hlsl
float3 HairSpecular(float3 tangent, float3 lightDir, float3 viewDir, float shiftAmount, float roughness)
{
    float3 T = normalize(tangent + shiftAmount * worldNormal);
    float TdotL = dot(T, lightDir);
    float TdotV = dot(T, viewDir);
    float sinTL = sqrt(1.0f - TdotL * TdotL);
    float sinTV = sqrt(1.0f - TdotV * TdotV);
    return pow(max(0, sinTL * sinTV - TdotL * TdotV), roughness);
}
```

Two specular lobes used in practice (primary + secondary shift).

### Eye Rendering
- **Cornea**: Highly specular, wet surface. Fresnel at grazing angles.
- **Iris**: Parallax depth illusion using POM or simple depth offset.
- **Sclera**: Subtle SSS, red vein detail texture, slightly wet.
- **Corneal reflections**: Environment cubemap sample with small roughness.

### Cloth Shading
Fabrics have distinct BRDF behavior.

**Velvet / microfiber:**
```hlsl
// Ashikhmin BRDF approximation for cloth
float3 ClothSpec(float3 N, float3 H, float roughness)
{
    float NdotH = saturate(dot(N, H));
    float sin2 = 1.0f - NdotH * NdotH;
    return float3(1,1,1) * pow(sin2, 1.0f / (roughness + 0.001f));
}
```

Denim: anisotropic along weave direction.
Silk: high-gloss directional specular.

---

## Related Skills
- Skill 06: PBR — base BRDF for all surface types
- Skill 04: Lighting Systems — IBL for character/vegetation
- Skill 03: HLSL Shaders — shader code for all techniques above
