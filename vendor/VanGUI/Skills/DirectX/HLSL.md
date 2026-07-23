---
name: hlsl-shaders
description: >
  Complete HLSL shader reference for all shader stages (VS, PS, GS, HS, DS, CS),
  shader techniques (normal mapping, parallax, POM, tessellation, triplanar, virtual texturing,
  environment mapping, shadow mapping), constant buffers, permutation systems, and SM3.0
  DX9-compatible shader library patterns. Use for any HLSL authoring, SM3.0 constraints,
  or shader system design question.
tags: [hlsl, shader, sm3, vertex-shader, pixel-shader, compute-shader, normal-mapping, pbr-shader, dx9-hlsl]
related_skills: [06_PBR, 03_HLSL_Shaders, 15_DX9_Injector_Architecture, 16_Shader_Replacement]
---

# HLSL Shaders

## Shader Stage Overview

| Stage | SM3.0 (DX9) | SM4.0 (DX10) | SM5.0 (DX11) | SM6.x (DX12) |
|-------|------------|--------------|--------------|---------------|
| Vertex Shader (VS) | ✓ | ✓ | ✓ | ✓ |
| Pixel Shader (PS) | ✓ | ✓ | ✓ | ✓ |
| Geometry Shader (GS) | ✗ | ✓ | ✓ | ✓ |
| Hull Shader (HS) | ✗ | ✗ | ✓ | ✓ |
| Domain Shader (DS) | ✗ | ✗ | ✓ | ✓ |
| Compute Shader (CS) | ✗ | ✗ | ✓ | ✓ |
| Mesh Shader | ✗ | ✗ | ✗ | ✓ |
| Amplification/Task | ✗ | ✗ | ✗ | ✓ |

---

## SM3.0 Pixel Shader — Core Patterns

### Simple Diffuse Lighting
```hlsl
float3 ApplySimpleLighting(
    float3 normal,
    float3 lightDir,
    float3 lightColor)
{
    float NdotL = saturate(dot(normal, lightDir));
    return lightColor * NdotL;
}
```

### Normal Map Decode (SM3.0)
```hlsl
float3 DecodeNormal(float2 enc)
{
    float2 f = enc * 2.0f - 1.0f;
    float z = sqrt(1.0f - saturate(dot(f, f)));
    return normalize(float3(f.x, f.y, z));
}
```

### Octahedral Normal Encode
```hlsl
float2 OctahedralEncode(float3 n)
{
    n /= (abs(n.x) + abs(n.y) + abs(n.z));
    if (n.z < 0.0f)
    {
        float2 s = sign(n.xy);
        n.xy = (1.0f - abs(n.yx)) * s;
    }
    return n.xy * 0.5f + 0.5f;
}
```

### Fresnel-Schlick Approximation
```hlsl
float3 FresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0f - F0) * pow(1.0f - cosTheta, 5.0f);
}
```

### Cook-Torrance BRDF (SM3.0 compatible)
```hlsl
float GGX_D(float NdotH, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float denom = (NdotH * NdotH * (a2 - 1.0f) + 1.0f);
    return a2 / (3.14159265f * denom * denom);
}

float SchlickGGX(float NdotV, float roughness)
{
    float r = roughness + 1.0f;
    float k = (r * r) / 8.0f;
    return NdotV / (NdotV * (1.0f - k) + k);
}

float3 CookTorranceBRDF(
    float3 N, float3 V, float3 L,
    float3 albedo, float metallic, float roughness)
{
    float3 H = normalize(V + L);
    float NdotL = saturate(dot(N, L));
    float NdotV = saturate(dot(N, V));
    float NdotH = saturate(dot(N, H));
    float VdotH = saturate(dot(V, H));

    float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), albedo, metallic);
    float3 F = FresnelSchlick(VdotH, F0);
    float D = GGX_D(NdotH, roughness);
    float G = SchlickGGX(NdotL, roughness) * SchlickGGX(NdotV, roughness);

    float3 spec = (D * G * F) / max(4.0f * NdotL * NdotV, 0.001f);
    float3 kD = (1.0f - F) * (1.0f - metallic);
    float3 diff = kD * albedo / 3.14159265f;

    return (diff + spec) * NdotL;
}
```

---

## Shader Techniques Reference

### Normal Mapping
Transforms tangent-space normal into world/view space using TBN matrix.
Required inputs: normal map (RG), tangent, bitangent, vertex normal.

### Parallax Mapping
Offsets UV coordinates based on height map to simulate surface depth.
Simple parallax uses single height sample + view vector.

### Parallax Occlusion Mapping (POM)
Iterative raymarching through height field. Gives correct silhouette self-occlusion.
Step count: 8–32 steps. More steps = better quality, higher cost.
SM3.0 compatible with limited step counts.

### Relief Mapping
Binary search refinement after POM ray finds intersection. Higher quality than POM.

### Displacement Mapping
Actual geometry displacement. Requires tessellation (SM5.0+). Not available in SM3.0.

### Tessellation (SM5.0+)
Hull Shader (HS) → tessellator → Domain Shader (DS). Adds geometry dynamically.
Not available in DX9/SM3.0.

### Triplanar Mapping
Projects texture from 3 world-space axes, blends by surface normal.
Eliminates UV seams on terrain and organic geometry.
```hlsl
float3 TriplanarSample(sampler2D tex, float3 worldPos, float3 worldNormal, float scale)
{
    float3 blendWeight = abs(worldNormal);
    blendWeight = pow(blendWeight, 4.0f);
    blendWeight /= dot(blendWeight, float3(1,1,1));
    float3 xProj = tex2D(tex, worldPos.yz * scale) * blendWeight.x;
    float3 yProj = tex2D(tex, worldPos.xz * scale) * blendWeight.y;
    float3 zProj = tex2D(tex, worldPos.xy * scale) * blendWeight.z;
    return xProj + yProj + zProj;
}
```

### Cube Mapping / Environment Mapping
```hlsl
float3 envSample = texCUBE(envMap, reflect(-viewDir, worldNormal)).rgb;
```

### Shadow Mapping (PCF, SM3.0)
```hlsl
float SampleShadowPCF(sampler2D shadowMap, float4 shadowCoord, float texelSize)
{
    float shadow = 0.0f;
    for (int x = -1; x <= 1; x++)
    {
        for (int y = -1; y <= 1; y++)
        {
            float2 offset = float2(x, y) * texelSize;
            float depth = tex2D(shadowMap, shadowCoord.xy + offset).r;
            shadow += (depth < shadowCoord.z - 0.001f) ? 0.0f : 1.0f;
        }
    }
    return shadow / 9.0f;
}
```

---

## Constant Buffer Layout (SM3.0)

In DX9, constant registers are set via:
```cpp
device->SetVertexShaderConstantF(0, (float*)&cbData, sizeof(cbData)/16);
device->SetPixelShaderConstantF(0, (float*)&cbData, sizeof(cbData)/16);
```

Recommended layout (VS):
- c0–c3: World matrix
- c4–c7: ViewProjection matrix
- c8–c11: WorldViewProjection
- c12: Camera position
- c13: Light direction + intensity
- c14–c15: Shadow matrix row 0–1
- c16–c17: Shadow matrix row 2–3

---

## Shader Permutation Systems

To handle material variants without branching overhead:
- Maintain a shader hash map keyed by feature flags
- Compile permutations offline: `#define NORMAL_MAP`, `#define SPECULAR`, etc.
- Select correct permutation at material bind time
- Avoid dynamic branching on SM3.0 (costly, limited)

---

## DX9 SM3.0 Shader Library — Module List

Per the source material (DX9 Volume C), a complete SM3.0 library covers:
- Lighting functions (Lambert, Phong, Cook-Torrance approx)
- Shadow filtering (PCF 3×3, PCF 5×5)
- SSAO generation and bilateral blur
- SSR ray march and hit detection
- Bloom extraction and downsample
- Tone mapping (Reinhard, ACES approx, Filmic)
- Fog and depth-based volumetric approximations
- Reflection utilities (Fresnel, cubemap blend)
- Normal encode/decode (2-component, octahedral)
- Utility math (saturate, remap, luminance, gamma)

---

## Related Skills
- Skill 06: PBR — full BRDF theory and texture workflows
- Skill 16: Shader Replacement — runtime shader swapping in DX9
- Skill 15: DX9 Injector Architecture — shader interception pipeline
- Skill 12: Deferred Rendering — G-buffer shader design
