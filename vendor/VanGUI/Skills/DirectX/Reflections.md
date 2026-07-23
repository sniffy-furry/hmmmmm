---
name: reflections-and-refraction
description: >
  Reflections: SSR, stochastic SSR, temporal SSR, reflection probes, parallax-corrected
  cubemaps, planar reflections, dynamic cubemaps, ray-traced reflections, hybrid reflections.
  Refraction: water refraction, glass refraction, heat distortion, shockwave distortion,
  atmospheric refraction. DX9 approximation strategies for all techniques.
tags: [ssr, reflections, refraction, cubemap, planar-reflections, ray-traced-reflections, parallax-corrected, dx9-reflections, heat-distortion]
related_skills: [06_PBR, 04_Lighting_Systems, 09_Water_Atmosphere_Volumetrics, 03_HLSL_Shaders]
---

# Reflections and Refraction

## Reflection Technique Comparison

| Technique | Accuracy | Dynamic | DX9 | Cost |
|-----------|---------|---------|-----|------|
| Static Cubemap | Low | No | ✓ | Very Low |
| Dynamic Cubemap | Medium | Yes | ✓ | Very High |
| Parallax-Corrected Cubemap | Good | No | ✓ | Low |
| Planar Reflection | Accurate | Yes | ✓ | High |
| SSR | Good (screen) | Yes | ✓ (approx) | Medium |
| Stochastic SSR | Better | Yes | ✗ | High |
| Temporal SSR | Best (screen) | Yes | ✗ | High |
| Ray-Traced | Excellent | Yes | ✗ | Very High |
| Hybrid (SSR+Probe fallback) | Excellent | Yes | ✗ | High |

---

## Cubemap-Based Reflections

Standard environment reflection using a pre-captured cubemap.

```hlsl
// Static cubemap reflection (DX9 SM3.0)
float3 R = reflect(-viewDir, worldNormal);
float3 envColor = texCUBE(envCubemap, R).rgb;
float3 F = FresnelSchlick(saturate(dot(viewDir, worldNormal)), F0);
float3 reflection = envColor * F;
```

**Parallax Correction (box volume):**
Adjusts the reflection vector to account for the probe's finite box extent.
Prevents the "sliding" artifact where reflections don't match surroundings.

```hlsl
float3 ParallaxCorrect(float3 R, float3 worldPos, float3 probeCenter, float3 boxMin, float3 boxMax)
{
    float3 rbmax = (boxMax - worldPos) / R;
    float3 rbmin = (boxMin - worldPos) / R;
    float3 rbminmax = (R > 0.0f) ? rbmax : rbmin;
    float fa = min(min(rbminmax.x, rbminmax.y), rbminmax.z);
    float3 posonbox = worldPos + R * fa;
    return normalize(posonbox - probeCenter);
}
```

---

## Planar Reflections

Renders the scene from a mirrored viewpoint and uses result as reflection texture.

**Pipeline:**
```
Compute mirror view matrix (flip view across plane)
  ↓
Render scene from mirror viewpoint → reflection texture
  ↓
In surface shader: project reflection texture with matching UV
  ↓
Blend using Fresnel term
```

**DX9 compatible.** High cost (second full render pass). Reserve for hero surfaces (e.g., water, mirrors).

---

## Screen Space Reflections (SSR)

Traces reflection rays through the depth buffer in screen space.

**Pipeline:**
```
G-Buffer (depth, normals, lit scene color)
  ↓
Per pixel: compute reflection vector R
  ↓
Step R through screen space using depth buffer (depth marching)
  ↓
Detect hit when sampled depth > ray depth
  ↓
Sample lit scene color at hit UV
  ↓
Fade by screen edge / ray length / roughness
  ↓
Blend with probe fallback for misses
```

**DX9 approximation (half-resolution):**
```hlsl
float2 SSR_March(float3 rayOrigin, float3 rayDir, sampler2D depthMap,
                 float4x4 proj, int steps, float stepSize)
{
    float3 pos = rayOrigin;
    for (int i = 0; i < steps; i++)
    {
        pos += rayDir * stepSize;
        float4 projected = mul(float4(pos, 1.0f), proj);
        float2 uv = projected.xy / projected.w * 0.5f + 0.5f;
        float sceneDepth = tex2D(depthMap, uv).r;
        if (pos.z > sceneDepth + 0.01f)
            return uv;  // Hit found
    }
    return float2(-1, -1);  // Miss
}
```

**Recommended SSR settings (DX9):**
- Steps: 16–32
- Step size: 0.05–0.2 world units
- Half resolution
- Temporal accumulation if velocity buffer available

---

## Stochastic SSR

Uses random ray jitter + temporal accumulation for high-quality soft reflections.
Requires multiple frames to converge. Not DX9 compatible (needs compute/structured buffers).

---

## Temporal SSR

Reprojects SSR results from previous frames using motion vectors.
Dramatically improves SSR stability. Requires velocity buffer (motion vectors).

---

## Ray-Traced Reflections (DXR)

Fires reflection rays via DXR. Handles off-screen content unlike SSR.
One ray per pixel minimum, denoised with spatiotemporal filter.

---

## Hybrid Reflections (Production)

AAA recommended approach:
```
Trace SSR ray
  If hit found → use SSR result
  If miss → sample reflection probe (cubemap)
  Blend boundary based on confidence
  + (optional) Ray-traced fallback for glossy surfaces
```

---

## Refraction

### Water Refraction
Distorts the scene behind water surface using normal map offset.

```hlsl
float2 RefractUV(float2 screenUV, float3 normal, float strength)
{
    return screenUV + normal.xy * strength;
}
float3 refracted = tex2D(sceneColorMap, RefractUV(screenUV, waterNormal, 0.05f)).rgb;
```

**DX9 compatible:** Uses a captured scene color render target sampled with offset UVs.

### Glass Refraction
Similar to water but with higher index of refraction and material-specific distortion.
IOR for glass ≈ 1.5. Refraction offset ≈ 0.02–0.1 depending on glass thickness.

### Heat Distortion
Animated noise-based UV distortion simulating heat shimmer.

```hlsl
float2 heatOffset = tex2D(noiseMap, uv + float2(time * 0.1f, 0.0f)).xy * 2.0f - 1.0f;
float3 heatScene = tex2D(sceneColor, uv + heatOffset * heatIntensity).rgb;
```

**DX9 compatible.** Common in vehicle exhaust, desert environments, explosions.

### Shockwave Distortion
Expanding ring of distortion from explosion origin. Uses distance from ring center.

```hlsl
float dist = length(uv - shockwaveCenter);
float ring = abs(dist - shockwaveRadius);
float intensity = max(0.0f, 1.0f - ring / shockwaveWidth);
float2 offset = normalize(uv - shockwaveCenter) * intensity * strength;
float3 scene = tex2D(sceneColor, uv + offset).rgb;
```

### Atmospheric Refraction
Bends light rays near horizon due to atmospheric density gradient.
Simulated via analytical aerial perspective + depth-based color shift.

---

## NFS MW Reflection Architecture (Reference)

Per the NFSMW chapter, the game uses:

```
Static Cubemap → Vehicle Shader → Fresnel approximation → Final reflection
```

Approximate pipeline:
```hlsl
float3 R = reflect(-viewDir, worldNormal);
float3 cubeSample = texCUBE(carEnvMap, R).rgb;
float fresnel = FresnelSchlick(saturate(dot(viewDir, worldNormal)), float3(0.04f)).x;
float3 reflection = cubeSample * fresnel;
// Blend with diffuse
float3 finalColor = lerp(diffuse, reflection, fresnel);
```

---

## Related Skills
- Skill 06: PBR — Fresnel for reflection weight
- Skill 04: Lighting Systems — probe placement and blending
- Skill 09: Water/Atmosphere — water-specific reflection + refraction
- Skill 18: NFSMW Renderer — cubemap reflection architecture in NFS MW
