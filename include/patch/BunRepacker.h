#pragma once
// ─── patch/BunRepacker.h ──────────────────────────────────────────────────────
// Offline BUN/BIN texture patcher — two algorithms, ported from
// mwml_1/src/core/bun_patcher.cpp and adapted for the standalone tool
// (no Windows.h / DLL injection; operates on std::vector<uint8_t> buffers and
// std::fstream for file I/O).
//
// Algorithm A — Same-size in-place patch (fast path, default):
//   Overwrite the pixel bytes at buf[pixelBase + entry.imagePlacement] with the
//   replacement DDS pixel data.  Updates TexInfoEntry metadata (width, height,
//   format, mip count) in the in-memory buffer.  The buffer is then written
//   back to disk atomically via a rename-over-original, with BackupManager
//   ensuring a .bak copy is taken once per session.
//
//   Precondition: replacement DDS pixel data size <= entry.imageSize.
//   Gating: enforced by CanReplaceInPlace().
//
// Algorithm B — Larger-replacement full repack (opt-in):
//   When the replacement DDS needs more space than the original slot, the entire
//   TPK pixel data block is rebuilt:
//     1. Compute new imagePlacement values for all textures (0x80-aligned).
//     2. Allocate a new buffer = old buffer size + growth delta.
//     3. Copy header region, write new pixel block (replacements + originals),
//        copy suffix (chunks after the old pixel block).
//     4. Walk ancestor chunk headers and add delta to their size fields.
//   Algorithm from BunPatcher::repack_tpk in mwml_1.
//
// Struct layout assumptions come from bun_format.hpp (TexInfoEntry, 124 bytes)
// and are cross-checked against TPKBlock.h (both describe the same on-disk
// format).
// ─────────────────────────────────────────────────────────────────────────────
#include "Common.h"
#include "formats/TPKBlock.h"
#include "patch/BackupManager.h"
#include <filesystem>
#include <functional>
#include <span>
#include <vector>

namespace nfsmw {

// ─────────────────────────────────────────────────────────────────────────────
// ReplaceRequest — one texture replacement job.
// ─────────────────────────────────────────────────────────────────────────────
struct ReplaceRequest {
    uint32_t             nameHash;   ///< BinHash of the target texture
    std::filesystem::path ddsPath;   ///< Source .dds file
};

// ─────────────────────────────────────────────────────────────────────────────
// PatchResult — outcome of a single patch attempt.
// ─────────────────────────────────────────────────────────────────────────────
struct PatchResult {
    uint32_t nameHash   = 0;
    bool     ok         = false;
    bool     usedRepack = false;   ///< true = algorithm B was used
    std::string error;
};

// ─────────────────────────────────────────────────────────────────────────────
// BunRepacker
// ─────────────────────────────────────────────────────────────────────────────
class BunRepacker {
public:
    // ─── Core entry points ──────────────────────────────────────────────────

    /// Apply `requests` to `filePath`.  Reads the whole file into memory,
    /// applies all patches (repack as needed), writes result back to disk, and
    /// records original regions in `bm` for revert.
    ///
    /// `progress` is called with [0, 1] fraction as work progresses so callers
    /// can update a ProgressState.  May be null.
    ///
    /// Returns one PatchResult per request, in submission order.
    std::vector<PatchResult> PatchFile(
        const std::filesystem::path& filePath,
        const std::vector<ReplaceRequest>& requests,
        BackupManager& bm,
        std::function<void(float)> progress = {});

    /// Revert all patches in `filePath` by replaying its .manifest file.
    /// Returns number of regions reverted, -1 on error.
    int RevertFile(const std::filesystem::path& filePath, BackupManager& bm);

    // ─── Static helpers (exposed for unit tests) ─────────────────────────────

    /// True when the replacement DDS fits inside the existing slot (Algorithm A
    /// is safe).  pixelDataSize = total bytes of the replacement's pixel chain.
    static bool CanReplaceInPlace(const Texture& tex, size_t pixelDataSize) {
        return pixelDataSize <= tex.fileDataSize;
    }

    /// 0x80-aligned value >= x.
    static size_t Align80(size_t x) {
        return (x + 0x7Fu) & ~size_t(0x7F);
    }

private:
    // ─── Internal BUN buffer representation ─────────────────────────────────

    // 124-byte on-disk TexInfoEntry matching bun_format.hpp::TexInfoEntry.
    // We replicate it here (no dependency on the mwml reference headers).
#pragma pack(push, 1)
    struct TexEntry {
        uint8_t  pad0[12];
        char     name[24];
        uint32_t nameHash;
        uint32_t classNameHash;
        uint32_t imageParentHash;
        uint32_t imagePlacement;   // byte offset from pixel base to this texture
        uint32_t palettePlacement;
        uint32_t imageSize;        // total size of all mip levels
        uint32_t paletteSize;
        uint32_t baseImageSize;    // first mip level size
        uint16_t width;
        uint16_t height;
        uint8_t  shiftWidth;
        uint8_t  shiftHeight;
        uint8_t  compressionType; // TexCompression enum
        uint8_t  palCompressionType;
        uint16_t numPaletteEntries;
        uint8_t  numMipLevels;
        uint8_t  pad1[25];
        uint8_t  pad2[20];
    };
    static_assert(sizeof(TexEntry) == 124, "TexEntry must be 124 bytes");
#pragma pack(pop)

    // Per-TPK context located within the in-memory buffer.
    struct TpkCtx {
        size_t texInfoOff  = 0;  // absolute offset of TPK_TEX_INFO payload (after 8B hdr)
        size_t texInfoSize = 0;  // byte length of TPK_TEX_INFO
        size_t texFmtOff   = 0;  // absolute offset of TPK_TEX_FORMAT payload (0 if absent)
        size_t texFmtSize  = 0;  // byte length of TPK_TEX_FORMAT
        size_t pixelBase   = 0;  // 0x80-aligned absolute offset of pixel data
        size_t tpkDataHdrOff = 0; // absolute offset of 0xB3320000 chunk header (for size patching)
        size_t texDataHdrOff = 0; // absolute offset of 0x33320002 chunk header (for size patching)
        bool   valid       = false;
    };

    // ─── Buffer walking ──────────────────────────────────────────────────────

    void CollectTPKs(const uint8_t* buf, size_t sz,
                     std::vector<TpkCtx>& out) const;

    void WalkContainer(const uint8_t* buf, size_t sz,
                       size_t absBase,
                       TpkCtx& pending, bool pendingValid,
                       std::vector<TpkCtx>& out) const;

    // ─── Patch algorithms ────────────────────────────────────────────────────

    // Algorithm A: in-place pixel overwrite + TexEntry metadata update.
    // Returns true on success.
    bool PatchInPlace(std::vector<uint8_t>& buf,
                      const TpkCtx& ctx,
                      const ReplaceRequest& req,
                      std::vector<uint8_t>& outOriginalBytes,
                      size_t& outFileOffset,
                      std::string& outErr);

    // Algorithm B: full pixel-block repack when new pixels don't fit.
    // Updates buf in place (may reallocate). Returns true on success.
    bool RepackAndPatch(std::vector<uint8_t>& buf,
                        const TpkCtx& ctx,
                        const std::vector<ReplaceRequest>& requests,
                        std::string& outErr);

    // Fix every ancestor chunk size field that spans `modPoint` (called after
    // repack to propagate the size delta upward through the chunk tree).
    static void PatchAncestorSizes(uint8_t* buf, size_t off, size_t end,
                                    size_t modPoint, int64_t delta);

    // ─── Portable LE read/write helpers ──────────────────────────────────────
    static uint32_t RU32(const uint8_t* p) {
        uint32_t v; std::memcpy(&v, p, 4); return v;
    }
    static void WU32(uint8_t* p, uint32_t v) {
        std::memcpy(p, &v, 4);
    }

    // ─── DDS pixel size helpers ───────────────────────────────────────────────
    static size_t DxtMipBytes(TexFormat fmt, uint32_t w, uint32_t h);
    static size_t DxtChainBytes(TexFormat fmt, uint32_t w, uint32_t h, uint32_t mips);

    // Convert TexFormat ↔ TexEntry::compressionType byte.
    static uint8_t  FormatToCompType(TexFormat f);
    static TexFormat CompTypeToFormat(uint8_t ct);
    static size_t   Mip0Size(TexFormat fmt, uint32_t w, uint32_t h);

    // Shift for log2 helpers.
    static uint8_t ShiftFor(uint32_t v) {
        uint8_t s = 0;
        while (v > 1) { v >>= 1; ++s; }
        return s;
    }

    // ─── Chunk ID constants (from bun_format.hpp) ────────────────────────────
    static constexpr uint32_t kChunkContainer = 0xB3300000u;
    static constexpr uint32_t kTPKRoot        = 0xB3310000u;
    static constexpr uint32_t kTPKData        = 0xB3320000u;
    static constexpr uint32_t kTPKTexInfo     = 0x33310004u;
    static constexpr uint32_t kTPKTexFormat   = 0x33310005u;
    static constexpr uint32_t kTPKTexData     = 0x33320002u;
};

} // namespace nfsmw
