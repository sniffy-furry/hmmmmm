---
name: shadow-systems
description: >
  Complete shadow rendering reference: shadow maps, Cascaded Shadow Maps (CSM), PCF,
  VSM, EVSM, Moment Shadow Maps (MSM), contact shadows, capsule shadows, distance field
  shadows, ray-traced shadows, soft shadows, penumbra simulation. Includes DX9 shadow
  implementation with resolution budgets and stencil optimizations.
tags: [shadows, csm, pcf, vsm, evsm, msm, contact-shadows, ray-traced-shadows, dx9-shadows, soft-shadows]
related_skills: [12_Deferred_Rendering, 03_HLSL_Shaders, 05_Shadow_Systems]
---

# Shadow Systems

## Shadow Technique Comparison

| Technique | Quality | Cost | DX9 | Transparency | Soft |
|-----------|---------|------|-----|--------------|------|
| Shadow Maps | Medium | Low | ✓ | ✗ | ✗ |
| CSM | High | Medium | ✓ | ✗ | ✗ |
| PCF | Good | Medium | ✓ | ✗ | Approx |
| VSM | Good | Medium | ✓ | ✗ | ✓ |
| EVSM | Better | Medium+ | ✓ | ✗ | ✓ |
| MSM | Best | High | ✗ (SM5+) | ✗ | ✓ |
| Contact Shadows | Screen-space | Low | ✓ | ✗ | ✗ |
| Ray-Traced | Best | Very High | ✗ | ✓ | ✓ |

---

## Shadow Maps — Fundamentals

A shadow map is a depth buffer rendered from the light's perspective.

**Basic algorithm:**
1. Render scene from light POV → depth texture
2. During main render, transform pixel to light space
3. Compare pixel depth vs stored shadow depth
4. If pixel is farther → in shadow

**Shadow bias:** Small offset to avoid self-shadowing acne.
```hlsl
float shadowDepth = tex2D(shadowMap, shadowUV).r;
float inShadow = (pixelDepth - bias > shadowDepth) ? 0.0f : 1.0f;
```

**Resolution guide (DX9):**
- Low-end: 1024×1024
- Mid-range: 2048×2048
- High-end: 4096×4096

---

## Cascaded Shadow Maps (CSM)

Splits camera frustum into multiple cascades, each with its own shadow map.
Provides high-quality near shadows and acceptable far shadows.

**Typical cascade splits:**
- Cascade 0: 0–5m (very sharp, 2048×2048)
- Cascade 1: 5–20m (sharp, 2048×2048)
- Cascade 2: 20–80m (medium, 1024×1024)
- Cascade 3: 80–300m (coarse, 1024×1024)

**Cascade selection in pixel shader:**
```hlsl
int cascade = 0;
if      (viewDepth > cascadeSplits[2]) cascade = 3;
else if (viewDepth > cascadeSplits[1]) cascade = 2;
else if (viewDepth > cascadeSplits[0]) cascade = 1;
// Sample cascade[cascade] shadow map
```

**DX9:** Feasible. Store each cascade in a separate texture. SM3.0 supports texture array emulation via multiple sampler slots.

---

## PCF — Percentage Closer Filtering

Samples the shadow map at multiple offsets and averages results.
Produces soft shadow edges.

**PCF 3×3 (DX9 SM3.0):**
```hlsl
float ShadowPCF3x3(sampler2D shadowMap, float2 uv, float depth, float texelSize)
{
    float shadow = 0.0f;
    for (int x = -1; x <= 1; x++)
    for (int y = -1; y <= 1; y++)
    {
        float2 offset = float2(x, y) * texelSize;
        float s = tex2D(shadowMap, uv + offset).r;
        shadow += (depth - 0.001f < s) ? 1.0f : 0.0f;
    }
    return shadow / 9.0f;
}
```

**PCF 5×5:** 25 samples, noticeably softer, higher cost.

---

## VSM — Variance Shadow Maps

Stores depth and depth² per texel. Uses Chebyshev's inequality to compute
shadow probability without per-sample comparison.

**Filtering:** Can be blurred with standard bilinear/Gaussian (no hardware PCF needed).
**Artifact:** Light bleeding in regions near fully-lit but occluded areas.

```hlsl
float2 moments = tex2D(vsmMap, uv).rg; // r=depth, g=depth²
float p = (depth <= moments.x) ? 1.0f : 0.0f;
float variance = moments.y - (moments.x * moments.x);
float d = depth - moments.x;
float pMax = variance / (variance + d * d);
return max(p, pMax);
```

---

## EVSM — Exponential Variance Shadow Maps

Extends VSM with exponential warping to reduce light bleeding.
Stores: exp(c·d) and exp(-c·d) plus their squares.
Higher quality than VSM at moderate extra cost.

---

## Contact Shadows

Screen-space ray march along the surface to detect nearby occluders.
Used to capture micro-shadows under characters, objects, crevices.

```
Per pixel:
  March a short ray from surface toward light in screen space
  If ray hits geometry (depth test) → in shadow
  Blend with main shadow term
```

**DX9 compatible:** Yes. Uses depth buffer. 8–16 steps typically sufficient.

---

## Capsule Shadows

Analytical shadow from a capsule primitive. Efficient for character self-shadowing.
Computes shadow based on closest point on capsule spine to shading point.

---

## Distance Field Shadows

Traces rays through a signed distance field (SDF) volume. Allows soft, accurate
shadows from any angle without shadow maps. Requires compute shaders (SM5.0+).
Not DX9-compatible.

---

## Ray-Traced Shadows (DXR)

True area light shadows with correct penumbra. One or more rays per pixel to light.
Denoised with spatiotemporal filtering (SVGF, ReLAX, etc.).
Only available on DX12 / DXR hardware.

---

## DX9 Shadow Pipeline (Recommended)

```
1. Render depth prepass (optional, improves cache)
2. Render shadow maps (1–4 cascades)
3. In G-buffer / forward shading:
   a. Transform pixel to light space
   b. Sample shadow map with PCF 3×3
   c. Apply cascade blend at split boundaries
4. Optionally add contact shadows via depth-buffer raymarch
```

**Stencil optimization for shadow volumes:**
```
Pass 1: Render light volume front faces, depth fail → stencil++
Pass 2: Render light volume back faces, depth fail → stencil--
Shade only pixels where stencil != 0
```

---

## Soft Shadows / Penumbra Simulation

PCSS (Percentage Closer Soft Shadows):
1. Search for blockers in shadow map region
2. Estimate penumbra width based on blocker distance
3. PCF with kernel size = penumbra width

PCSS is SM5.0-class but can be approximated in SM3.0 with fixed-radius PCF
and depth-based kernel scaling.

---

## Temporal Shadow Stabilization

Shadows flicker without temporal accumulation.
- Track shadow samples across frames using motion vectors
- Exponential moving average: `shadow = lerp(prevShadow, currShadow, 0.1)`
- Reject samples where motion vector indicates position has moved

---

## Related Skills
- Skill 12: Deferred Rendering — shadow integration in deferred pipeline
- Skill 03: HLSL Shaders — PCF filter HLSL code
- Skill 04: Lighting Systems — directional/point/spot light shadow relationship
