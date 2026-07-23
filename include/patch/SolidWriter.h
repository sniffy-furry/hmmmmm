#pragma once
// ─── patch/SolidWriter.h ──────────────────────────────────────────────────────
// Object import / rebuild: replace a SolidObject's geometry inside an
// uncompressed BUN/BIN with an imported mesh (full topology rebuild).
//
// Strategy (see memory: solid-format-writer) — preserve-and-swap:
//   • Keep the object's 0x00134011 header (patch only numTris + bbox), its
//     texture refs (0x00134012) and every unparsed sub-chunk verbatim.
//   • Rebuild the 0x80134100 mesh container from the imported meshes:
//       0x00134900 header · 0x00134B02 groups · 0x00134B03 indices ·
//       0x00134B01 single shared vertex buffer · 0x00134C02 material names.
//   • Splice the rebuilt 0x80134010 back into the file, fix ancestor chunk
//     sizes, and write atomically with a BackupManager .bak.
//
// JDLZ-compressed files are decompressed in memory, edited, and written back
// UNCOMPRESSED (the game reads raw chunks when the 'JDLZ' magic is absent; we
// have no JDLZ encoder). The original compressed file is kept in the .bak.
// EXPERIMENTAL: byte layout is reverse-engineered; rebuilt files need in-game
// validation.
// ─────────────────────────────────────────────────────────────────────────────
#include "Common.h"
#include "formats/SolidList.h"
#include "patch/BackupManager.h"
#include <filesystem>
#include <string>
#include <vector>

namespace nfsmw {

/// One imported sub-mesh (one material/shading group).
struct ImportMesh {
    std::vector<SolidMeshVertex> vertices;  ///< pos/normal/uv/color (game space)
    std::vector<uint16_t>        indices;   ///< group-relative triangle list
    std::string                  materialName;
};

struct ImportModel {
    std::vector<ImportMesh> meshes;

    size_t TotalVertices() const {
        size_t n = 0; for (auto& m : meshes) n += m.vertices.size(); return n;
    }
    size_t TotalTriangles() const {
        size_t n = 0; for (auto& m : meshes) n += m.indices.size() / 3; return n;
    }
};

class SolidWriter {
public:
    /// Replace the geometry of the object whose name-hash is `nameHash` in the
    /// solid file at `path` with `model`. Backs the file up via `bm` first.
    static Result<void> ReplaceObject(const std::filesystem::path& path,
                                      uint32_t nameHash,
                                      const ImportModel& model,
                                      BackupManager& bm);

    /// Patch ONLY the u32 flags field of the object whose name-hash is
    /// `nameHash` (the SolidObject.flags at +24 in its 0x00134011 header). No
    /// chunk sizes change, so this is a byte-exact in-place edit. JDLZ files are
    /// decompressed and written back uncompressed (same policy as ReplaceObject);
    /// the original is preserved in the .bak via `bm`. `expectedOld`, when
    /// non-null, is verified against the on-disk value first as a safety check.
    static Result<void> PatchObjectFlags(const std::filesystem::path& path,
                                         uint32_t nameHash,
                                         uint32_t newFlags,
                                         BackupManager& bm,
                                         const uint32_t* expectedOld = nullptr);

    /// Reassign one entry of an object's texture-reference table (0x00134012)
    /// so every mesh using that slot draws with a different texture — no
    /// geometry re-export needed. `slotIndex` is the entry position in the
    /// object's textureHashes[] list; `newTexHash` is the Joaat name-hash of the
    /// replacement texture (must exist in a TPK the game loads). Byte-exact
    /// in-place patch (u32 at slot*8), JDLZ-aware, backed up via `bm`.
    /// `expectedOld`, when non-null, is verified against the on-disk hash first.
    static Result<void> PatchObjectTexture(const std::filesystem::path& path,
                                           uint32_t nameHash,
                                           uint32_t slotIndex,
                                           uint32_t newTexHash,
                                           BackupManager& bm,
                                           const uint32_t* expectedOld = nullptr);

    /// Overwrite an object's 4x4 world transform in place (the 64-byte matrix at
    /// +64 in its 0x00134011 header). `xform16` is 16 floats in the file's
    /// column-major order (== glm::value_ptr(mat4)). Byte-exact, JDLZ-aware,
    /// backed up via `bm`.
    static Result<void> PatchObjectTransform(const std::filesystem::path& path,
                                             uint32_t nameHash,
                                             const float* xform16,
                                             BackupManager& bm);
};

} // namespace nfsmw
