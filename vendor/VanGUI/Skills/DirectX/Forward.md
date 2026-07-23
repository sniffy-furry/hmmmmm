---
name: forwardplus-and-clustered-rendering
description: >
  Forward+ and Clustered Rendering: tile frustum construction, compute shader light culling,
  light bounding volumes, depth-aware tiling, cluster construction (X/Y/Z), logarithmic depth
  slicing, GPU data structures, memory layout, Forward+ vs Clustered vs Deferred comparison,
  transparency advantages, volumetric integration, reflection probe management, GPU-driven
  rendering. DX9 CPU-driven Forward+ approximation.
tags: [forward-plus, clustered, tiled, light-culling, compute, cluster, tile-frustum, depth-slicing, gpu-driven, dx9-tiled]
related_skills: [01_Rendering_Pipelines, 12_Deferred_Rendering, 04_Lighting_Systems, 14_Ray_Tracing]
---

# Forward+ and Clustered Rendering

## Motivation

Traditional forward: O(Objects × Lights)
Forward+:           O(Objects × TileLights)
Clustered:          O(Objects × ClusterLights)

The culling reduces the effective light count per-draw from all scene lights to only those relevant to the screen region.

---

## Forward+ Architecture

### Pipeline
```
Depth Prepass (required for accurate tile frustums)
  ↓
Build Tile Light Lists (GPU compute or CPU)
  ↓
Forward Lighting Pass (each draw uses its tile's light list)
  ↓
Post Processing
```

### Tile Dimensions
Common sizes: 8×8, 16×16, 32×32 pixels.

**At 1920×1080 with 16×16 tiles:**
- Tiles X: 120
- Tiles Y: 68
- Total: 8,160 tiles

Each tile maintains its own light index list.

---

## Tile Frustum Construction

Each tile corresponds to a frustum in view space.

**For tile at grid position (tileX, tileY):**
```hlsl
// Compute 4 frustum planes from tile corner NDC positions
float2 tileScale = float2(screenWidth, screenHeight) / float2(TILE_SIZE, TILE_SIZE);
float2 tileBias = float2(tileX, tileY);

// Convert to clip space corners
float4 corners[4];
corners[0] = invProj × (NDC from tile top-left)
corners[1] = invProj × (NDC from tile top-right)
corners[2] = invProj × (NDC from tile bottom-left)
corners[3] = invProj × (NDC from tile bottom-right)

// Build 4 side planes + 2 depth planes (from depth prepass min/max)
```

---

## Compute Shader Light Culling (SM5.0)

```hlsl
// Dispatch one thread group per tile
[numthreads(TILE_SIZE, TILE_SIZE, 1)]
void CullLights(uint3 groupID : SV_GroupID, uint3 localID : SV_GroupThreadID)
{
    // Shared memory for tile's light list
    groupshared uint tileLightCount = 0;
    groupshared uint tileLightIndices[MAX_LIGHTS_PER_TILE];

    // Each thread reads one light and tests against tile frustum
    uint lightIndex = localID.x + localID.y * TILE_SIZE;
    if (lightIndex < totalLightCount)
    {
        if (LightIntersectsTile(lights[lightIndex], tileFrustum))
        {
            uint slot;
            InterlockedAdd(tileLightCount, 1, slot);
            if (slot < MAX_LIGHTS_PER_TILE)
                tileLightIndices[slot] = lightIndex;
        }
    }
    GroupMemoryBarrierWithGroupSync();

    // Write tile light list to global buffer
    if (localID.x == 0 && localID.y == 0)
    {
        uint tileIndex = groupID.x + groupID.y * tileCountX;
        tileLightListBuffer[tileIndex].Count = tileLightCount;
        for (uint i = 0; i < tileLightCount; i++)
            tileLightListBuffer[tileIndex].Indices[i] = tileLightIndices[i];
    }
}
```

**DX9: No compute shader.** See CPU-driven section below.

---

## Light Bounding Volumes

| Light Type | Volume Shape | Test |
|-----------|-------------|------|
| Point | Sphere | Sphere vs frustum planes |
| Spot | Cone | Cone vs frustum |
| Area | Box or OBB | Box vs frustum |
| Directional | Infinite | Always visible |

**Sphere vs frustum plane test:**
```hlsl
bool SphereInFrustum(float3 center, float radius, float4 planes[6])
{
    for (int i = 0; i < 6; i++)
        if (dot(float4(center, 1.0f), planes[i]) < -radius)
            return false;
    return true;
}
```

---

## Depth-Aware Tiling

Basic Forward+ uses only screen-space tile bounds (4 planes).
Problem: a tile spanning near and far objects bundles their lights together.

**Solution:** Extract per-tile depth min/max from depth prepass.
Add 2 depth planes (near/far) to tile frustum.
Requires per-tile atomic min/max reduction (SM5.0 compute).

---

## Clustered Rendering Architecture

Extends tiles into 3D clusters (X, Y, Z).

### Cluster Grid
```
cluster(x, y, z) where:
  x = tile column
  y = tile row
  z = depth slice
```

**Example grid:** 120 × 68 × 24 = 195,840 clusters

### Logarithmic Depth Slicing (Preferred)

Linear depth slicing wastes precision — too coarse near camera, too fine at distance.

Logarithmic slice formula:
```
slice = floor(log(linearDepth / zNear) * (numSlices / log(zFar / zNear)))
```

Benefits:
- Dense slices near camera (where detail matters)
- Coarse slices far away (where lights are rare)

---

## GPU Data Structures

### Cluster Buffer
```hlsl
struct Cluster
{
    uint Offset;    // offset into LightIndexList
    uint Count;     // number of lights in this cluster
};
StructuredBuffer<Cluster> clusterBuffer;
```

### Light Index List
```hlsl
StructuredBuffer<uint> lightIndexList;
// Access: lightIndexList[cluster.Offset + i]
```

### Compact cluster data (memory efficient):
```
Total memory = numClusters × 8 bytes (Cluster struct)
             + maxLightRefs × 4 bytes (index list)
```

---

## Memory Considerations

Clustered rendering trades memory for culling quality.

**Memory breakdown for 120×68×24 grid:**
- Cluster buffer: 195,840 × 8 bytes ≈ 1.5 MB
- Light index list: varies (128 avg lights/cluster × 4 bytes → up to ~100 MB worst case)
- Use sparse allocation: pre-allocate only for average occupancy

**Optimization:**
- Compress cluster data (pack count+offset into uint2)
- Dynamic resizing of light index list
- Cull empty clusters (no geometry)

---

## Forward+ vs Deferred vs Clustered Comparison

| Property | Forward+ | Deferred | Clustered |
|----------|---------|---------|-----------|
| Transparency | ✓ Native | ✗ Hard | ✓ Native |
| MSAA | ✓ Native | ✗ Complex | ✓ Native |
| Memory | Low | High (G-buffer) | Medium |
| Many lights | Good | Excellent | Excellent |
| Depth precision | Per-tile | N/A | Per-cluster |
| Volumetrics | Approx | Good | ✓ Native |
| GPU complexity | Medium | Medium | High |

---

## Transparency in Forward+

Forward+ naturally handles transparency because shading occurs during rasterization.

Transparent objects: glass, particles, smoke, hair, water surfaces.
Simply sort transparents back-to-front and render after opaques, using the same tile light lists.

---

## Volumetric Rendering Integration

Clusters naturally store volumetric data per-region:
- Fog density
- Participating media coefficients
- Scattering parameters

Applications: volumetric fog, cloud rendering, atmospheric effects.
Each raymarch step samples the cluster at its world position.

---

## Reflection Probe Management in Clusters

Each cluster can store probe indices:
```hlsl
struct Cluster
{
    uint LightOffset;   uint LightCount;
    uint ProbeOffset;   uint ProbeCount;
    uint DecalOffset;   uint DecalCount;
};
```

Benefits: efficient probe lookup, avoids costly distance-based search per pixel.

---

## DX9 CPU-Driven Forward+ Approximation

Since DX9 lacks compute shaders:

```
CPU: Divide screen into 16×16 tiles
CPU: Frustum-cull all scene lights against each tile frustum
CPU: Build per-tile light index arrays (max 32–64 lights/tile)
CPU: Upload light lists via SetPixelShaderConstantF
  OR pack into a texture (RGBA8 → 4 light indices per texel)
PS:  For each pixel, determine tile index from UV
     Iterate tile's light list
     Evaluate lighting per light
```

**Practical limits:**
- DX9 PS constant registers: 224 float4 slots
- Max lights encodable: ~56 lights as float4 (position + color + radius)
- Per-tile lists via texture: up to ~256 lights total, 32 per tile

**Suitable for:** 100–300 dynamic lights at low-to-mid framerate targets.

---

## GPU-Driven Rendering Integration

Modern engines combine clustered with GPU-driven workflows:
- Indirect draw calls (no CPU per-draw overhead)
- Mesh shaders (SM6.5+: replaces VS+GS)
- GPU visibility systems (Nanite-style)
- Clustered light culling on GPU (mandatory with GPU-driven)

---

## Related Skills
- Skill 01: Rendering Pipelines — pipeline selection context
- Skill 12: Deferred Rendering — alternative architecture comparison
- Skill 04: Lighting Systems — light types and evaluation
- Skill 20: Optimization — occupancy and cluster memory profiling
