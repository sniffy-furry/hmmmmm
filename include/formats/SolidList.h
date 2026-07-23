#pragma once
#include "Common.h"
#include "core/ChunkReader.h"
#include <vector>
#include <string>
#include <memory>

namespace nfsmw {

// ─────────────────────────────────────────────────────────────────────────────
// Real NFSMW (PC v1.3) solid geometry format, verified against retail
// TRACKS/STREAML2RA.BUN:
//
//   0x80134000 GeometryContainer
//     0x80134001 GeometryInfo
//       0x00134002 header: u32[4], then class/file name (C string at +16)
//       0x00134003 hash table: { u32 hash, u32 pad } per object
//     0x80134010 GeometryObject (one per solid)
//       0x00134011 header (160 bytes after 0x11-padding, then name):
//          +0   u8  zero[12]
//          +12  u32 version (0x16)
//          +16  u32 nameHash (Joaat of name)
//          +20  u32 numTris
//          +24  u32 flags
//          +28  u32 zero
//          +32  vec4 bboxMin (LOCAL space)
//          +48  vec4 bboxMax
//          +64  float4x4 transform (D3D row-major, translation in row 3)
//          +128 u8[32] runtime pointers / params
//          +160 char name[] (null-terminated)
//       0x00134012 texture refs: { u32 hash, u32 pad } per texture
//       0x80134100 GeometryMesh
//          0x00134900 header: u32[2] zero, u32 version(0x12), u32 fvfFlags,
//                     u32 numGroups, u32, u32 numVertexBuffers, ...,
//                     u32 totalIndexCount (last dword)
//          0x00134B02 shading groups, 104 bytes each:
//             +0  vec3 bboxMin   +12 vec3 bboxMax
//             +24 u8 texIdx[5]   +29 u8 shaderIdx
//             +48 u32 texCount
//             +56 u32 fvfFlags (0x4000 → 36B stride, 0x2A4000/0x224000 → 60B)
//             +60 u32 vertexCount
//             +64 u32 triCount
//             +68 u32 firstIndex (offset into the index buffer)
//             +92 u32 indexCount
//          0x00134B03 index buffer (u16 triangle list, group-relative)
//          0x00134B01 vertex buffer(s) (one per group when counts match)
//          0x00134C02 material/texture usage names (C strings)
//
// Vertex layouts (little-endian):
//   36-byte: pos float3 @0, normal float3 @12, color BGRA @24, uv float2 @28
//   60-byte: same as 36 + tangent data @36 (ignored by the editor)
//   24-byte: pos float3 @0, color BGRA @12, uv float2 @16 (rare)
// ─────────────────────────────────────────────────────────────────────────────

/// Editor-normalised vertex (matches renderer attribute layout).
#pragma pack(push, 1)
struct SolidMeshVertex {
    float   pos[3];       ///< XYZ position (game Z-up, object-local)
    float   normal[3];    ///< Surface normal
    float   uv[2];        ///< Texture coordinates
    uint8_t color[4];     ///< RGBA baked lighting colour
};
#pragma pack(pop)
static_assert(sizeof(SolidMeshVertex) == 36);

/// One shading group = one renderable sub-mesh.
struct SolidMesh {
    std::vector<SolidMeshVertex> vertices;
    std::vector<uint16_t>        indices;     ///< triangle list, group-relative

    AABB        bbox;                         ///< from the shading group record
    uint32_t    fvfFlags     = 0;
    uint32_t    strideInFile = 0;
    uint32_t    textureHash  = 0;             ///< diffuse texture (Joaat)
    std::string materialName;                 ///< from 0x00134C02 (best effort)
    SimSurface  surface = SimSurface::Unknown;

    // Parse diagnostics
    bool indexOutOfRange = false;

    /// Editor-generated meshes (trigger fences etc.) can force a renderer
    /// blend mode instead of the texture-name classification.
    /// -1 = classify from texture/material name; 0/1/2 = force.
    int8_t blendOverride = -1;

    AABB ComputeAABB() const {
        AABB box;
        for (const auto& v : vertices)
            box.Expand({ v.pos[0], v.pos[1], v.pos[2] });
        return box;
    }
};

/// A single named solid object containing one or more sub-meshes.
struct SolidObject {
    std::string            name;
    uint32_t               nameHash = 0;
    uint32_t               numTris  = 0;
    uint32_t               flags    = 0;
    std::vector<SolidMesh> meshes;
    std::vector<uint32_t>  textureHashes;     ///< from 0x00134012
    glm::mat4              transform{ 1.0f }; ///< world placement (game coords)
    AABB                   bbox;              ///< LOCAL-space bounds from header

    /// Absolute file offset of the 0x00134011 payload (after 0x11 padding),
    /// recorded so the exporter can patch the transform in place. 0 = unknown.
    uint64_t               headerFileOffset = 0;

    // Editor state (not serialized to game format)
    ObjectID               editorId = kInvalidID;
    bool                   selected = false;
    bool                   visible  = true;

    size_t VertexCount() const {
        size_t n = 0;
        for (const auto& m : meshes) n += m.vertices.size();
        return n;
    }
    size_t TriCount() const {
        size_t n = 0;
        for (const auto& m : meshes) n += m.indices.size() / 3;
        return n;
    }
};

/// Parsed 0x80134000 container — a collection of SolidObjects.
struct SolidList {
    std::string               sectionName;   ///< from 0x00134002 (+16)
    std::vector<uint32_t>     hashTable;     ///< from 0x00134003
    std::vector<SolidObject>  objects;
    int                       parseWarnings = 0;

    size_t TotalVertexCount() const {
        size_t n = 0;
        for (const auto& o : objects) n += o.VertexCount();
        return n;
    }
    size_t TotalTriCount() const {
        size_t n = 0;
        for (const auto& o : objects) n += o.TriCount();
        return n;
    }

    const SolidObject* FindByHash(uint32_t hash) const {
        for (const auto& o : objects)
            if (o.nameHash == hash) return &o;
        return nullptr;
    }
};

// ─────────────────────────────────────────────────────────────────────────────
/// Parser for 0x80134000 GeometryContainer payloads.
class SolidListParser {
public:
    /// Parse a GeometryContainer payload (without the outer 8-byte header).
    /// `absOffset` = absolute file offset of the payload start (for patching).
    static Result<SolidList> Parse(std::span<const uint8_t> payload, uint64_t absOffset = 0);

private:
    static Result<SolidObject> ParseObject(std::span<const uint8_t> payload, uint64_t absOffset,
                                           int& warnings);
    static void ParseMeshContainer(std::span<const uint8_t> payload, SolidObject& obj,
                                   int& warnings);
};

} // namespace nfsmw
