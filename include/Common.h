#pragma once

// ─── NFSMW Map Editor — Common Types ──────────────────────────────────────────
// Shared types used across all subsystems.
//
// World coordinate system: the game world is Z-up (verified against
// TRACKS/STREAML2RA.BUN). The editor keeps ALL data in game coordinates;
// the renderer camera/grid are Z-up aware, so no conversion ever happens.

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <string>
#include <string_view>
#include <vector>
#include <array>
#include <memory>
#include <functional>
#include <optional>
#include <span>
#include <unordered_map>
#include <cassert>

// ─── GLM ──────────────────────────────────────────────────────────────────────
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/component_wise.hpp>

namespace nfsmw {

// ─── Result type ──────────────────────────────────────────────────────────────
template<typename T>
struct Result {
    T           value{};
    std::string error{};
    bool        ok = false;

    static Result Ok(T v)                   { return { std::move(v), {}, true }; }
    static Result Err(std::string msg)       { return { {}, std::move(msg), false }; }
    explicit operator bool() const           { return ok; }
};

template<>
struct Result<void> {
    std::string error{};
    bool        ok = false;

    static Result Ok()                       { return { {}, true }; }
    static Result Err(std::string msg)       { return { std::move(msg), false }; }
    explicit operator bool() const           { return ok; }
};

// ─── AABB ─────────────────────────────────────────────────────────────────────
struct AABB {
    glm::vec3 min{ 1e30f,  1e30f,  1e30f };
    glm::vec3 max{-1e30f, -1e30f, -1e30f };

    void Expand(const glm::vec3& p) {
        min = glm::min(min, p);
        max = glm::max(max, p);
    }
    void Expand(const AABB& o) {
        if (!o.Valid()) return;
        Expand(o.min); Expand(o.max);
    }
    glm::vec3 Center()  const { return (min + max) * 0.5f; }
    glm::vec3 Extents() const { return (max - min) * 0.5f; }
    bool      Valid()   const { return min.x <= max.x; }
};

/// Transform an AABB by a matrix (returns the AABB of the 8 transformed corners).
inline AABB TransformAABB(const AABB& b, const glm::mat4& m) {
    AABB out;
    if (!b.Valid()) return out;
    for (int i = 0; i < 8; ++i) {
        glm::vec3 c{ (i & 1) ? b.max.x : b.min.x,
                     (i & 2) ? b.max.y : b.min.y,
                     (i & 4) ? b.max.z : b.min.z };
        out.Expand(glm::vec3(m * glm::vec4(c, 1.0f)));
    }
    return out;
}

// ─── Chunk IDs ────────────────────────────────────────────────────────────────
// All IDs below were verified by scanning the retail PC v1.3 files
// (TRACKS/L2RA.BUN, TRACKS/STREAML2RA.BUN, GLOBAL/GlobalB.lzc).
// Container chunks have bit 31 set and hold nested chunks.
namespace ChunkID {
    // ── Geometry (solids) — STREAM*.BUN / GEOMETRY.BIN ──
    constexpr uint32_t GeometryContainer    = 0x80134000; ///< One solid list
    constexpr uint32_t GeometryInfo         = 0x80134001;
    constexpr uint32_t GeometryInfoHeader   = 0x00134002; ///< class/file name at +16
    constexpr uint32_t GeometryHashTable    = 0x00134003; ///< 8B entries (hash, 0)
    constexpr uint32_t GeometryEmpty        = 0x80134008;
    constexpr uint32_t GeometryObject       = 0x80134010; ///< One solid object
    constexpr uint32_t GeometryObjectHeader = 0x00134011; ///< 160B header + name
    constexpr uint32_t GeometryTextureRefs  = 0x00134012; ///< 8B entries (hash, 0)
    constexpr uint32_t GeometryLightMaterial= 0x00134013;
    constexpr uint32_t GeometryMesh         = 0x80134100; ///< Mesh container
    constexpr uint32_t MeshHeader           = 0x00134900; ///< counts + FVF flags
    constexpr uint32_t MeshShadingGroups    = 0x00134B02; ///< 104B per group
    constexpr uint32_t MeshIndices          = 0x00134B03; ///< u16 triangle list
    constexpr uint32_t MeshVertices         = 0x00134B01; ///< vertex buffer
    constexpr uint32_t MeshMaterialName     = 0x00134C02; ///< C string

    // ── Scenery (prop placement) — STREAM*.BUN ──
    constexpr uint32_t ScenerySection       = 0x80034100;
    constexpr uint32_t SceneryHeader        = 0x00034101; ///< section number at +12
    constexpr uint32_t SceneryInfos         = 0x00034102; ///< 72B per info (model defs)
    constexpr uint32_t SceneryInstances     = 0x00034103; ///< 64B per instance
    constexpr uint32_t SceneryTreeNodes     = 0x00034105; ///< preserved raw
    constexpr uint32_t SceneryOverrideHooks = 0x00034107; ///< preserved raw

    // ── Texture packs — STREAM*.BUN / TEXTURES.BIN ──
    constexpr uint32_t TPKContainer         = 0xB3300000;
    constexpr uint32_t TPKInfo              = 0xB3310000;
    constexpr uint32_t TPKInfoHeader        = 0x33310001; ///< version, name, path
    constexpr uint32_t TPKHashTable         = 0x33310002; ///< 8B entries (standard variant);
                                                            ///< also reused as the hash table in
                                                            ///< the compressed variant (TEXTURES.BIN)
    constexpr uint32_t TPKCompEntries       = 0x33310003; ///< 24B per texture — compressed
                                                            ///< variant descriptor table (JDLZ
                                                            ///< per-texture blobs; see TPKBlock.h)
    constexpr uint32_t TPKEntries           = 0x33310004; ///< 124B per texture
    constexpr uint32_t TPKCompInfo          = 0x33310005; ///< 32B per texture (fourCC)
    constexpr uint32_t TPKData              = 0xB3320000;
    constexpr uint32_t TPKDataHeader        = 0x33320001;
    constexpr uint32_t TPKDataRaw           = 0x33320002; ///< raw DXT payload
    constexpr uint32_t TPKAnimBlock         = 0xB0300100;

    // ── Master track file (e.g. TRACKS/L2RA.BUN) ──
    constexpr uint32_t StreamingFileHeader  = 0x00034112;
    constexpr uint32_t StreamingBarrierInfo = 0x00034191;
    constexpr uint32_t StreamingSections    = 0x00034110; ///< 92B per section entry
    constexpr uint32_t TrackPositionMarkers = 0x00034146; ///< pad + 48B point markers
    constexpr uint32_t TriggerRegionParent  = 0x80034147; ///< wraps one 0x0003414A
    constexpr uint32_t TriggerRegions       = 0x0003414A; ///< typed 2D trigger regions
    constexpr uint32_t TrackPathManager     = 0x80034150;
    constexpr uint32_t VisibleSectionManager= 0x8003B900;
    constexpr uint32_t VisibleSectionBounds = 0x0003B901;
    /// 'CARP' attribute blob holding the GPS/AI road network (RNnd nodes,
    /// RNsg segments, cost grids). Previously misidentified as an event
    /// trigger pack — recon against retail data corrected this. Preserved raw.
    constexpr uint32_t WorldMapData         = 0x0003B800;
    constexpr uint32_t EventSequencePack    = 0x8003B810; ///< holds 0x0003B811 children
    constexpr uint32_t EventSequenceChunk   = 0x0003B811; ///< one 'CARP' script blob
    constexpr uint32_t LightSections        = 0x80036000;
    constexpr uint32_t WeathermanChunk      = 0x00037080;

    constexpr uint32_t Null                 = 0x00000000; ///< alignment filler

    // ── NIS cutscenes — NIS/Scene_*_BundleB.bun ──
    /// Payload is an 8B '0x11' sentinel followed by a little-endian MIPS
    /// ELF32 relocatable object (EA's "EAGL4Anim" toolchain animation export;
    /// confirmed by the literal "EAGL4::SymbolPool::mpSymbolTable" string in
    /// speed.exe). Its .symtab holds per-bone symbols named
    /// "__Bone:::<skeleton>.<bone>" and "__Skeleton:::<name>". Each bone
    /// symbol's st_value points into a 48-entry bone-index table in .data
    /// (just the bone's own index, st_size==4) -- NOT the real keyframe
    /// curve. The actual rotation keyframe stream lives elsewhere in .data
    /// but its byte encoding is NOT decoded (hex inspection + an empirical
    /// unit-quaternion framing scan both failed to converge on a layout;
    /// see formats/NisAnim.h for what is and isn't recovered).
    constexpr uint32_t NisAnimation         = 0x00E34009;
}

/// Alignment padding byte used inside many payloads (rows of 0x11).
constexpr uint8_t kChunkPadByte = 0x11;

// ─── SimSurface material enum (editor-side classification) ───────────────────
enum class SimSurface : uint8_t {
    Asphalt  = 0,
    Concrete = 1,
    Dirt     = 2,
    Grass    = 3,
    Gravel   = 4,
    Sand     = 5,
    Water    = 6,
    Metal    = 7,
    Wood     = 8,
    Unknown  = 0xFF,
};

inline const char* SimSurfaceName(SimSurface s) {
    switch (s) {
        case SimSurface::Asphalt:  return "Asphalt";
        case SimSurface::Concrete: return "Concrete";
        case SimSurface::Dirt:     return "Dirt";
        case SimSurface::Grass:    return "Grass";
        case SimSurface::Gravel:   return "Gravel";
        case SimSurface::Sand:     return "Sand";
        case SimSurface::Water:    return "Water";
        case SimSurface::Metal:    return "Metal";
        case SimSurface::Wood:     return "Wood";
        default:                   return "Unknown";
    }
}

// ─── Editor object ID ─────────────────────────────────────────────────────────────
using ObjectID = uint64_t;
constexpr ObjectID kInvalidID = 0;

} // namespace nfsmw
