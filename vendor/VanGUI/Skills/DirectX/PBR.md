---
name: physically-based-rendering
description: >
  Full PBR reference: Cook-Torrance BRDF, GGX NDF, Smith geometry term, Fresnel-Schlick,
  energy conservation, microfacet theory, importance sampling, metallic/roughness and
  specular workflows, all texture inputs (albedo, normal, roughness, metallic, AO, height,
  emissive, opacity). Includes DX9/SM3.0 approximation strategies.
tags: [pbr, brdf, cook-torrance, ggx, fresnel, metallic, roughness, specular-workflow, energy-conservation, sm3-pbr]
related_skills: [03_HLSL_Shaders, 04_Lighting_Systems, 08_Reflections_Refraction]
---

# Physically Based Rendering (PBR)

## Core Principles

1. **Energy conservation**: Reflected light ≤ incoming light. Diffuse + specular ≤ 1.
2. **Microfacet theory**: Surfaces are composed of micro-facets. Roughness controls distribution.
3. **Physically correct BRDF**: Bidirectional Reflectance Distribution Function must be physically plausible.
4. **Reciprocity**: BRDF(L, V) = BRDF(V, L).

---

## Workflow Comparison

### Metallic/Roughness (preferred, glTF standard)
| Input | Range | Meaning |
|-------|-------|---------|
| Albedo (BaseColor) | 0–1 RGB | Diffuse color (non-metals) or F0 (metals) |
| Metallic | 0–1 | 0 = dielectric, 1 = conductor |
| Roughness | 0–1 | 0 = mirror, 1 = fully diffuse |
| Normal | tangent-space XY | Surface normal perturbation |
| AO | 0–1 | Ambient occlusion pre-baked |
| Emissive | 0–∞ RGB | Self-emitted light (HDR capable) |
| Height/Displacement | 0–1 | For POM or tessellation |
| Opacity | 0–1 | Transparency (forward pass only) |

### Specular/Glossiness (legacy, Allegorithmic)
| Input | Meaning |
|-------|---------|
| Diffuse | Surface color |
| Specular | F0 per-channel directly |
| Glossiness | 1 - roughness |

---

## Cook-Torrance BRDF

```
f(L, V) = D(H) · G(L, V, H) · F(V, H)
          ────────────────────────────
              4 · (N·L) · (N·V)
```

Where:
- **D** = Normal Distribution Function (NDF) — microfacet alignment
- **G** = Geometric shadowing/masking — microfacet occlusion
- **F** = Fresnel — reflectance at viewing angle

---

## D — GGX Normal Distribution Function

Models probability of microfacet normals aligned with half-vector H.

```
D_GGX(N, H, α) = α² / (π · ((N·H)² · (α² - 1) + 1)²)
```

```hlsl
float D_GGX(float NdotH, float roughness)
{
    float a  = roughness * roughness;
    float a2 = a * a;
    float d  = (NdotH * NdotH * (a2 - 1.0f) + 1.0f);
    return a2 / (3.14159265f * d * d);
}
```

High roughness → wide lobe. Low roughness → tight specular highlight.

---

## G — Smith Geometry Function

Accounts for microfacet self-shadowing and masking.

**Schlick-GGX (single direction):**
```hlsl
float G_SchlickGGX(float NdotV, float roughness)
{
    float r = roughness + 1.0f;
    float k = (r * r) / 8.0f;
    return NdotV / (NdotV * (1.0f - k) + k);
}
```

**Smith combined (light + view):**
```hlsl
float G_Smith(float NdotL, float NdotV, float roughness)
{
    return G_SchlickGGX(NdotL, roughness) * G_SchlickGGX(NdotV, roughness);
}
```

---

## F — Fresnel-Schlick

Describes how reflectance increases at grazing angles.

```
F(V, H) = F0 + (1 - F0) · (1 - V·H)^5
```

```hlsl
float3 F_Schlick(float VdotH, float3 F0)
{
    return F0 + (1.0f - F0) * pow(1.0f - VdotH, 5.0f);
}
```

**F0 values (common materials):**
| Material | F0 (linear) |
|----------|------------|
| Water | 0.02 |
| Plastic | 0.04 |
| Glass | 0.04–0.08 |
| Iron | 0.56, 0.57, 0.58 |
| Gold | 1.00, 0.71, 0.29 |
| Aluminum | 0.91, 0.92, 0.92 |
| Copper | 0.95, 0.64, 0.54 |

**Metallic workflow F0:**
```hlsl
float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), albedo, metallic);
```

---

## Energy Conservation

Diffuse and specular must sum to ≤ 1:
```hlsl
float3 kS = F;                    // specular fraction
float3 kD = (1.0f - kS) * (1.0f - metallic);  // diffuse fraction
// Metals have no diffuse
```

---

## Importance Sampling

Monte Carlo integration over the hemisphere weighted by the BRDF's NDF.
Used in offline pre-filtering (IBL, BRDF LUT baking) and real-time path tracing.

**GGX importance sampling:**
```hlsl
float3 ImportanceSampleGGX(float2 Xi, float roughness, float3 N)
{
    float a = roughness * roughness;
    float phi = 2.0f * 3.14159265f * Xi.x;
    float cosTheta = sqrt((1.0f - Xi.y) / (1.0f + (a*a - 1.0f) * Xi.y));
    float sinTheta = sqrt(1.0f - cosTheta * cosTheta);
    float3 H = float3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);
    // Transform H to world space using N
    return TangentToWorld(H, N);
}
```

---

## DX9 SM3.0 PBR Approximations

Full GGX PBR is achievable in SM3.0 with care:

**Feasible:**
- Cook-Torrance with GGX D, Schlick G, Schlick-Fresnel → Yes
- IBL with single static cubemap → Yes
- Metallic/roughness texture inputs → Yes
- BRDF LUT lookup → Yes (2D texture sample)

**Not feasible without workarounds:**
- Real-time pre-filtered environment mips → Approximate with fixed LOD sampling
- Multiple bounce GI → Use lightmaps or probes

**Approximation fallback for SM3.0:**
```hlsl
// Simplified specular without full Cook-Torrance
float3 H = normalize(L + V);
float NdotH = saturate(dot(N, H));
float specPower = (1.0f - roughness) * 128.0f;
float3 specular = F0 * pow(NdotH, specPower);
```

---

## PBR Texture Checklist

Before authoring PBR materials, ensure:
- [ ] Albedo has no baked lighting or AO
- [ ] Normal map is tangent-space (not object-space unless explicitly handled)
- [ ] Roughness is perceptual (0 = smooth, 1 = rough)
- [ ] Metallic is binary in practice (0.0 or 1.0; intermediate = painted metal)
- [ ] AO is baked (cavity/crevice only, no directional shadow)
- [ ] Emissive is in HDR range if bloom is expected

---

## Related Skills
- Skill 03: HLSL Shaders — complete BRDF HLSL code
- Skill 04: Lighting Systems — IBL and probe integration with PBR
- Skill 08: Reflections — specular environment sampling
