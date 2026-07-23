#pragma once
// ─── StreamBundle.h ──────────────────────────────────────────────────────────
// Phase 6: load STREAM*.BUN files to enumerate SolidLists and their
// associated TPKBlocks for the ObjectExportPanel.
//
// This is a simplified port of the reference NFSMapEditor StreamBundle (see
// reference/nfsmapeditor_phase6/formats/StreamBundle.h). The full editor
// carries ScenerySection, EventTriggers, TrafficPath, VaultPack, and the
// streaming-section offset table for the master file. For the export panel
// we only need solid geometry + texture packs, so those subsystems are
// omitted (plan.md §6.1 — "preview + export only").
//
// FindTexture() is provided as a free helper that mirrors the lookup
// described in plan.md §3.3 and §6.1: section TPKs first, then the optional
// extra (global) packs.
// ─────────────────────────────────────────────────────────────────────────────
#include "Common.h"
#include "formats/SolidList.h"
#include "formats/TPKBlock.h"
#include "core/ChunkReader.h"
#include <filesystem>
#include <vector>
#include <string>
#include <span>

namespace nfsmw {

// ─────────────────────────────────────────────────────────────────────────────
/// Contents of one parsed stream section blob or a loaded .BUN/.BIN file.
/// Populated by StreamBundleLoader::ParseBlob / Load.
struct StreamSection {
    std::string              name;          ///< stem of the source file, or "section_N"
    uint32_t                 number = 0;    ///< section number from the file table (if known)

    std::vector<SolidList>   solidLists;    ///< all 0x80134000 containers found
    std::vector<TPKBlock>    texturePacks;  ///< all TPK blocks found in this section
    int                      parseWarnings = 0;

    // ── Convenience accessors ────────────────────────────────────────────────
    size_t TotalObjects() const {
        size_t n = 0;
        for (const auto& sl : solidLists) n += sl.objects.size();
        return n;
    }
    size_t TotalTextures() const {
        size_t n = 0;
        for (const auto& tp : texturePacks) n += tp.textures.size();
        return n;
    }

    /// Search this section's TPKBlocks for a texture by Joaat hash.
    const Texture* FindTexture(uint32_t hash) const {
        for (const auto& tp : texturePacks)
            for (const auto& tex : tp.textures)
                if (tex.nameHash == hash) return &tex;
        return nullptr;
    }
};

// ─────────────────────────────────────────────────────────────────────────────
/// Find a texture by hash; section-local TPKs searched first, then extras
/// (e.g. GLOBAL packs loaded separately). Returns nullptr if not found.
inline const Texture* FindTexture(uint32_t hash,
                                  const StreamSection& section,
                                  const std::vector<TPKBlock>* extraTPKs = nullptr) {
    if (const Texture* t = section.FindTexture(hash)) return t;
    if (extraTPKs)
        for (const auto& tp : *extraTPKs)
            for (const auto& tex : tp.textures)
                if (tex.nameHash == hash) return &tex;
    return nullptr;
}

// ─────────────────────────────────────────────────────────────────────────────
/// Parses raw chunk data (a section blob or a whole .BUN/.BIN file) into a
/// StreamSection. For Phase 6 this walks the top-level chunk tree and collects
/// every GeometryContainer (0x80134000) and TPKContainer (0xB3300000).
///
/// This replaces the full-fat TrackDatabase / StreamBundleLoader from the
/// reference editor (which also parses ScenerySection, EventTriggers, etc.)
/// with a targeted pass — same chunk-walk entry point, narrower dispatch.
class StreamBundleLoader {
public:
    /// Parse raw chunk data.
    /// `absOffset` is the absolute offset of `data` within its file so
    /// sub-parsers can record correct patch offsets.
    static Result<StreamSection> ParseBlob(std::span<const uint8_t> data,
                                           uint64_t absOffset = 0);

    /// Convenience: load + (if JDLZ) decompress + parse a whole file.
    /// Sets `result.name` to the file stem.
    static Result<StreamSection> Load(const std::filesystem::path& path);
};

} // namespace nfsmw
