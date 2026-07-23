---
name: water-atmosphere-volumetrics
description: >
  Water rendering (Gerstner waves, FFT oceans, caustics, foam, shore interaction,
  underwater, Fresnel, rain ripples), atmospheric rendering (Rayleigh/Mie scattering,
  ozone, aerial perspective, sky models, dynamic time of day, sun/moon), volumetrics
  (volumetric fog, clouds, smoke, fire, god rays, light shafts, dust). Full DX9 raymarch
  and depth-based approximation pipelines.
tags: [water, ocean, fog, volumetrics, atmosphere, rayleigh, mie, god-rays, clouds, dx9-fog, gerstner, fft-ocean]
related_skills: [09_Water_Atmosphere_Volumetrics, 04_Lighting_Systems, 03_HLSL_Shaders, 11_Post_Processing]
---

# Water, Atmosphere, and Volumetrics

---

## WATER RENDERING

### Gerstner Waves
Physically-based wave model. Each wave has amplitude (A), wavelength (λ), speed (c), direction (D).

```hlsl
float3 GerstnerWave(float3 pos, float2 direction, float amplitude, float wavelength, float speed, float time)
{
    float k = 2.0f * 3.14159f / wavelength;
    float c = sqrt(9.81f / k);  // deep water dispersion
    float2 d = normalize(direction);
    float f = k * (dot(d, pos.xz) - c * time);
    float a = amplitude;
    return float3(
        d.x * a * sin(f),
        a * cos(f),
        d.y * a * sin(f)
    );
}
```

Sum multiple waves (4–8) with varying parameters for realistic surface.

### FFT Ocean
Simulates ocean via inverse Fast Fourier Transform on Philips spectrum.
Produces highly realistic large-scale ocean surfaces.
Requires compute shaders (SM5.0+). Not DX9 compatible.
**DX9 substitute:** Multiple Gerstner waves + scrolling normal maps.

### Caustics
Projected light pattern from water surface refraction onto sea floor.
```hlsl
// Animated caustic approximation
float2 c1 = tex2D(causticMap, uv * 2.0f + float2(time * 0.05f, 0.0f)).rg;
float2 c2 = tex2D(causticMap, uv * 3.0f - float2(time * 0.03f, time * 0.04f)).rg;
float caustic = min(c1.r + c1.g, c2.r + c2.g);
```

### Foam Generation
White foam at wave crests where wave steepness exceeds threshold.
Foam density = saturate(waveHeight / foamThreshold).

### Shore Interaction
Depth-based foam and transparency near shoreline.
```hlsl
float shoreDepth = waterDepth - sceneDepth;  // from depth buffer
float foam = 1.0f - saturate(shoreDepth / shoreWidth);
```

### Underwater Rendering
- Depth fog (blue-green tint, increasing with depth)
- Caustic projection on geometry
- Bubble particles
- Visibility falloff

### Fresnel Reflection on Water
```hlsl
float waterFresnel = FresnelSchlick(saturate(dot(viewDir, waterNormal)), 0.02f).x;
float3 waterColor = lerp(refraction, reflection, waterFresnel);
```

### Rain Ripples
Animated expanding ring normals on water surface.
Pack multiple ring animations with phase offsets into a single texture.

---

## ATMOSPHERIC RENDERING

### Rayleigh Scattering
Scattering by gas molecules (N₂, O₂). Wavelength-dependent (λ⁻⁴).
Produces blue sky and red sunsets.

**Scattering coefficient:**
β_R = (8π³(n²-1)²) / (3·N·λ⁴)

Short wavelengths (blue) scatter much more than long (red).

### Mie Scattering
Scattering by aerosols and particles. Less wavelength-dependent.
Produces sun halo (white/grey glow around sun).

### Ozone Absorption
Absorbs portions of the spectrum, contributing to sky color depth.
Wavelength-dependent absorption coefficient.

### Aerial Perspective
Objects far from camera appear desaturated and bluish due to accumulated scattering.
```hlsl
float3 AerialPerspective(float3 color, float depth, float3 skyColor)
{
    float amount = 1.0f - exp(-depth * aerialDensity);
    return lerp(color, skyColor, amount);
}
```
**DX9 compatible.** Simple depth-based lerp toward sky color.

### Sky Models
- **Preetham model**: Analytical sky, fast, good quality
- **Hosek-Wilkie model**: More accurate, also analytical
- **Bruneton model**: Most accurate, uses lookup textures
- **DX9 compatible**: Preetham and simple gradient + sun disk

**Simple procedural sky (DX9):**
```hlsl
float3 SkyGradient(float3 viewDir, float3 sunDir)
{
    float sunAmount = max(dot(viewDir, sunDir), 0.0f);
    float3 skyColor = lerp(horizonColor, zenithColor, saturate(viewDir.y));
    float3 sunColor = sunHaloColor * pow(sunAmount, 64.0f);
    return skyColor + sunColor;
}
```

### Dynamic Time of Day
Drive sky color, sun direction, and light intensity from a time-of-day parameter.
Interpolate between keyframed sky states (dawn, noon, dusk, night).

---

## VOLUMETRICS

### Volumetric Fog (Depth-Based Raymarch)

Recommended DX9 approach: 16–32 ray march steps from camera to fragment.

**Pipeline:**
```
Scene Depth
  ↓
Raymarch from eye to fragment position
  ↓
Accumulate fog density per step (with optional density noise)
  ↓
Compute light scattering per step (toward sun direction)
  ↓
Composite fog over scene color
```

```hlsl
float3 VolumetricFog(float3 rayOrigin, float3 rayDir, float rayLength,
                      float3 lightDir, float3 fogColor, int steps)
{
    float stepSize = rayLength / steps;
    float fogAccum = 0.0f;
    float3 lightAccum = float3(0,0,0);
    for (int i = 0; i < steps; i++)
    {
        float t = (i + 0.5f) * stepSize;
        float3 pos = rayOrigin + rayDir * t;
        float density = FogDensity(pos);           // height-based or noise
        float scatter = PhaseFunction(dot(rayDir, lightDir)); // Henyey-Greenstein
        lightAccum += density * scatter * LightAtPos(pos) * stepSize;
        fogAccum += density * stepSize;
    }
    float transmittance = exp(-fogAccum);
    return lerp(fogColor * lightAccum, float3(0,0,0), transmittance);
}
```

**Henyey-Greenstein phase function:**
```hlsl
float PhaseHG(float cosTheta, float g)
{
    float g2 = g * g;
    return (1.0f - g2) / (4.0f * 3.14159f * pow(1.0f + g2 - 2.0f * g * cosTheta, 1.5f));
}
```

### Volumetric Clouds
Full volumetric clouds require 3D noise textures and many raymarch steps (64–256).
**DX9:** Use billboard card stacks or low-step (16-step) raymarching with pre-baked density.

### God Rays / Light Shafts
Radial blur from sun position in screen space.

```hlsl
float3 GodRays(sampler2D occlusionMap, float2 screenUV, float2 lightScreenPos, int samples)
{
    float2 delta = (screenUV - lightScreenPos) / samples;
    float2 uv = screenUV;
    float3 color = float3(0,0,0);
    for (int i = 0; i < samples; i++)
    {
        uv -= delta;
        float occlusion = tex2D(occlusionMap, uv).r;
        color += occlusion * godRayIntensity;
    }
    return color / samples;
}
```

**DX9 compatible.** Typically 16–32 samples. Composite additively over scene.

### Volumetric Smoke / Fire
- Smoke: 3D noise texture animated with flow field, raymarched
- Fire: Same as smoke but with emission and color ramp (cool=red, hot=yellow/white)
- Dust: Small particle density volume, usually pre-baked

---

## DX9 Volumetric Fog Pipeline (Canonical)

```
Capture Depth Buffer
  ↓
Raymarch at half-resolution (16 steps)
  ↓
Accumulate fog density + light scattering
  ↓
Upscale (depth-aware bilateral)
  ↓
Composite over scene color
  ↓
Present
```

---

## Related Skills
- Skill 04: Lighting Systems — light scattering integration
- Skill 11: Post Processing — god rays, fog composite
- Skill 03: HLSL Shaders — raymarch and phase function code
