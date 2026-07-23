#pragma once
// ─── NisAnim.h ───────────────────────────────────────────────────────────────
// Parser for the 0x00E34009 chunk found in NIS/Scene_*_BundleB.bun cutscene
// bundles (Common.h::ChunkID::NisAnimation).
//
// The payload is an 8-byte 0x11 sentinel followed by a little-endian MIPS32
// ELF relocatable object produced by EA's "EAGL" toolchain.
//
// .data section layout (FIFA O-file format, confirmed via fifa.miraheze.org/O):
//   [0x000 .. numBones*16-1]  Bone objects: {uint32 index, uint32 0, uint32 0, uint32 0}
//   [numBones*16 .. +16)      Skeleton header: checksum(u16), numDOFs(u16),
//                              coreChecksum(u16), flags(u16), numBones(u32), invScales(u32=0)
//   [numBones*16+16 .. end]   Per-bone bind-pose data (112 bytes each):
//                              scale(V3) + parent(I32) + rotation(V4 xyzw) +
//                              translation(V3) + leftRight(I32) + invMatrix(M4x4)
// ─────────────────────────────────────────────────────────────────────────────
#include "Common.h"
#include <glm/gtc/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace nfsmw {

/// Per-bone bind-pose data decoded from the Skeleton object in .data.
struct NisAnimBone {
    glm::vec3 scale       { 1.f, 1.f, 1.f };
    int32_t   parent      { -1 };
    glm::quat rotation    { 1.f, 0.f, 0.f, 0.f }; // (w,x,y,z) — glm convention
    glm::vec3 translation { 0.f, 0.f, 0.f };
};

/// Bind-pose skeleton decoded from the __Skeleton::: object in .data.
struct NisAnimSkeleton {
    uint32_t                 numBones = 0;
    std::vector<NisAnimBone> bones;
};

/// One named data symbol from the embedded ELF's .symtab (a bone, skeleton
/// marker, or toollib-version marker — anything starting with "__").
struct NisAnimSymbol {
    std::string name;
    uint32_t    sectionIndex = 0;   ///< st_shndx
    uint32_t    value        = 0;   ///< st_value (offset into its section, for data symbols)
    uint32_t    size         = 0;   ///< st_size (bytes)
};

/// One ELF section (kept generically; .data holds the actual keyframe bytes).
struct NisAnimSection {
    std::string          name;
    uint32_t             type   = 0;
    uint32_t             offset = 0; ///< file offset within the embedded ELF
    std::vector<uint8_t> bytes;      ///< raw section contents
};

/// Result of parsing one 0x00E34009 chunk.
struct NisAnimClip {
    std::vector<NisAnimSection>  sections;
    std::vector<NisAnimSymbol>   symbols;      ///< all "__*" data symbols
    std::vector<size_t>          boneSymbols;  ///< indices into `symbols` named "__Bone:::..."
    std::string                  skeletonName; ///< from a "__Skeleton:::<name>" symbol, if present
    std::optional<NisAnimSkeleton> skeleton;   ///< bind-pose skeleton if successfully decoded

    /// Interpret a bone symbol's raw bytes as a flat float array (best-effort;
    /// no semantic decode of the float layout is implied beyond "real bytes").
    std::vector<float> BoneFloats(size_t boneSymbolIdx) const;
};

class NisAnimParser {
public:
    /// Parse one chunk payload (the bytes after the 8-byte chunk header,
    /// i.e. starting at the 8-byte 0x11 sentinel).
    static Result<NisAnimClip> Parse(std::span<const uint8_t> payload);
};

// ─── Skeletal posing ──────────────────────────────────────────────────────────
// How the skeleton is posed at a given time in the NIS player.
enum class NisPoseMode {
    BindPose,      ///< Rest pose for all t. The authored motion is an 8-bit
                   ///< quantised stream whose dequantisation math is still
                   ///< unsolved (encyclopedia C10.3), so there is no real sample
                   ///< to apply — this is the honest default.
    SyntheticIdle, ///< A small, obviously-synthetic sway used only to exercise
                   ///< the posing pipeline end-to-end. NOT the game's authored
                   ///< animation — the UI labels it as such.
};

/// Evaluate the skeleton at time `t` (seconds), writing world-space bone
/// matrices to `outWorld` (resized to numBones). Forward kinematics over the
/// bind pose; a per-bone channel *sample* (rotation/translation/scale delta)
/// would be composed with the bind transform at the marked hook once the
/// keyframe dequantiser is pinned — until then BindPose applies no delta.
/// Returns false if the skeleton is empty.
bool EvaluateNisPose(const NisAnimSkeleton& skel, float t, NisPoseMode mode,
                     std::vector<glm::mat4>& outWorld);

} // namespace nfsmw
