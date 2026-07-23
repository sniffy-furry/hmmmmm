---
name: post-processing
description: >
  Complete post-processing reference: HDR pipeline, bloom, lens dirt, lens flare,
  anamorphic flares, depth of field, motion blur, film grain, chromatic aberration,
  vignette, color grading, LUT processing, tone mapping (Reinhard, ACES, Filmic),
  eye adaptation, TAA, TSR, MSAA, SMAA, FXAA, DLSS, FSR, XeSS, frame generation.
  DX9/SM3.0 implementation of entire post chain.
tags: [hdr, bloom, tonemap, color-grading, lut, taa, fxaa, smaa, dlss, fsr, dof, motion-blur, dx9-post, film-grain, aces]
related_skills: [11_Post_Processing, 03_HLSL_Shaders, 02_DX9_Architecture, 17_ENB_ReShade_Frameworks]
---

# Post Processing

## Canonical DX9 Post-Processing Chain (Order Matters)

```
Scene Render (FP16 HDR buffer)
  ↓
SSAO composite
  ↓
SSR composite
  ↓
Bloom extraction + downsample + blur + upsample
  ↓
Lens dirt + lens flare (optional)
  ↓
Depth of Field
  ↓
Motion Blur
  ↓
Tone Mapping (HDR → LDR)
  ↓
LUT Color Grading
  ↓
Film Grain
  ↓
Chromatic Aberration
  ↓
Vignette
  ↓
Sharpening
  ↓
Anti-Aliasing (FXAA/SMAA)
  ↓
Present
```

**TAA / Upscalers** (DX11+) replace the final AA step with temporal reconstruction.

---

## HDR Pipeline

### Render to FP16 Buffer
All scene rendering targets `A16B16G16R16F` (or `R11G11B10F` on DX11+).
Scene values can exceed 1.0 — this headroom is critical for correct bloom and tonemapping.

**DX9 HDR formats:**
- `A16B16G16R16F` → preferred
- `A8R8G8B8` → LDR fallback only

### Tone Mapping (HDR → LDR)

**Reinhard:**
```hlsl
float3 ToneMapReinhard(float3 color, float exposure)
{
    color *= exposure;
    return color / (1.0f + color);
}
```

**ACES (Approximate, DX9 compatible):**
```hlsl
float3 ToneMapACES(float3 x)
{
    float a = 2.51f, b = 0.03f, c = 2.43f, d = 0.59f, e = 0.14f;
    return saturate((x*(a*x+b))/(x*(c*x+d)+e));
}
```

**Filmic (Hable / Uncharted 2):**
```hlsl
float3 Uncharted2Tonemap(float3 x)
{
    float A=0.15f, B=0.50f, C=0.10f, D=0.20f, E=0.02f, F=0.30f;
    return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
}
float3 ToneMapFilmic(float3 color, float exposure)
{
    float3 curr = Uncharted2Tonemap(color * exposure * 2.0f);
    float3 whiteScale = 1.0f / Uncharted2Tonemap(float3(11.2f,11.2f,11.2f));
    return curr * whiteScale;
}
```

---

## Bloom

### Classic DX9 Bloom Pipeline
```
Scene (FP16)
  ↓
Bright Pass (threshold extract: luminance > threshold)
  ↓
Downsample (4× bilinear)
  ↓
Horizontal Gaussian Blur
  ↓
Vertical Gaussian Blur
  ↓
Upsample
  ↓
Additive composite over scene
```

**Bright pass:**
```hlsl
float3 BrightPass(float3 color, float threshold, float knee)
{
    float lum = dot(color, float3(0.2126f, 0.7152f, 0.0722f));
    float rq = clamp(lum - threshold + knee, 0.0f, 2.0f * knee);
    rq = (rq * rq) / (4.0f * knee + 0.00001f);
    return color * max(rq, lum - threshold) / max(lum, 0.00001f);
}
```

**Modern dual kawase / physical bloom:** Progressively downsample (6–8 mip levels), blur each level, upsample and sum. More physically plausible than single-level bloom.

---

## Color Grading

### LUT (Look-Up Table)
3D LUT: 32³ or 64³ cube texture mapping [R,G,B] input → [R,G,B] output.

**DX9 LUT emulation (3D texture not natively supported):**
Decompose into 32 slices stored as a 512×32 or 1024×32 2D texture.
```hlsl
float3 ApplyLUT(sampler2D lut, float3 color, float lutSize)
{
    float sliceSize = 1.0f / lutSize;
    float slicePixelSize = sliceSize / lutSize;
    float sliceInnerSize = slicePixelSize * (lutSize - 1.0f);
    float xOffset = 0.5f * slicePixelSize + color.r * sliceInnerSize;
    float yOffset = 0.5f * slicePixelSize + color.g * sliceInnerSize;
    float zOffset = color.b * (lutSize - 1.0f);
    float zSlice0 = floor(zOffset) * sliceSize;
    float zSlice1 = zSlice0 + sliceSize;
    float3 s0 = tex2D(lut, float2(xOffset + zSlice0, yOffset)).rgb;
    float3 s1 = tex2D(lut, float2(xOffset + zSlice1, yOffset)).rgb;
    return lerp(s0, s1, frac(zOffset));
}
```

---

## Eye Adaptation (Auto-Exposure)

Computes average scene luminance over time and adjusts exposure.

**DX9 approach:**
1. Downsample scene to 1×1 via bilinear chain
2. Read luminance from CPU (or use occlusion query trick)
3. Lerp exposure toward target: `exposure = lerp(exposure, targetExposure, adaptSpeed * dt)`

---

## Depth of Field

### Bokeh DoF (Simplified, DX9)
```
COC (Circle of Confusion) map from depth
  ↓
Near blur (gather)
  ↓
Far blur (gather)
  ↓
Composite: near over in-focus over far
```

**COC calculation:**
```hlsl
float COC(float depth, float focalDistance, float focalLength, float aperture)
{
    float d = abs(depth - focalDistance);
    return (focalLength * focalLength * d) / (aperture * depth * (focalDistance - focalLength));
}
```

---

## Motion Blur

### Camera Motion Blur
Reconstruct velocity from reprojection matrix:
```hlsl
float4 prevPos = mul(worldPos, prevViewProj);
float4 currPos = mul(worldPos, currViewProj);
float2 velocity = (currPos.xy / currPos.w) - (prevPos.xy / prevPos.w);
```
Blur along velocity vector in PS.

### NFS MW Motion Blur Reference
Per the source material:
```
Scene Render → Velocity Approximation → Fullscreen Blur → Composite
```
Modern velocity buffers were not standard; approximations used (camera delta or fixed blur).

---

## Anti-Aliasing

| Technique | Quality | Cost | DX9 |
|-----------|---------|------|-----|
| MSAA | Excellent (geometry) | High | ✓ (2×/4×/8×) |
| SSAA | Excellent | Very High | ✓ |
| FXAA | Fast, blurry | Very Low | ✓ |
| SMAA | Good | Low | ✓ |
| CMAA | Good | Low | ✓ |
| TAA | Excellent | Medium | Approx only |
| TSR | Best | High | ✗ |

**FXAA (DX9 compatible):**
Detects edges by luminance contrast, blends along edge.
Single-pass, very fast. Slight blur on fine geometry.

**SMAA:** Edge detection → blending weight calculation → neighborhood blending. Better than FXAA, still DX9 compatible.

**TAA:** Accumulates sub-pixel samples over time using motion vectors. Requires velocity buffer. Can be approximated in DX9.

---

## Upscaling Technologies

| Tech | Vendor | DX Req |
|------|--------|--------|
| DLSS | NVIDIA | DX11/12 |
| FSR 1/2/3 | AMD | DX11+ (FSR1 any) |
| XeSS | Intel | DX12 |
| TSR | Epic | DX11+ |

**DX9:** None of these apply. Use FXAA or SMAA only.

---

## Film Effects

**Film Grain:**
```hlsl
float grain = frac(sin(dot(uv + time, float2(127.1f, 311.7f))) * 43758.5f);
color += (grain - 0.5f) * grainStrength;
```

**Chromatic Aberration:**
```hlsl
float r = tex2D(scene, uv + aberrationOffset).r;
float g = tex2D(scene, uv).g;
float b = tex2D(scene, uv - aberrationOffset).b;
```

**Vignette:**
```hlsl
float2 center = uv - 0.5f;
float vignette = 1.0f - dot(center, center) * vignetteStrength;
color *= saturate(vignette);
```

**Sharpening (Unsharp Mask):**
```hlsl
float3 blurred = tex2D(scene, uv + float2(texelSize, 0)) + tex2D(scene, uv - float2(texelSize, 0));
blurred += tex2D(scene, uv + float2(0, texelSize)) + tex2D(scene, uv - float2(0, texelSize));
blurred /= 4.0f;
color += (color - blurred) * sharpStrength;
```

---

## Related Skills
- Skill 17: ENB/ReShade Frameworks — effect chain orchestration
- Skill 03: HLSL Shaders — all PS implementations
- Skill 02: DX9 Architecture — render target management for post chain
