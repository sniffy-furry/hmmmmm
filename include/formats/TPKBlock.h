#pragma once
#include "Common.h"
#include "core/ChunkReader.h"
#include <vector>
#include <string>
#include <filesystem>
#include <span>

namespace nfsmw {

// ─────────────────────────────────────────────────────────────────────────────
// Real NFSMW texture pack format (verified against retail STREAML2RA.BUN):
//
//   0xB3300000 TPKContainer
//     0xB3310000 TPKInfo
//       0x33310001 header: u32 version(5), char name[28]@4, char path[64]@32, u32 hash@96
//       0x33310002 hash table: { u32 hash, u32 pad } per texture
//       0x33310004 texture entries, 124 bytes each:
//          +12  char name[24]
//          +36  u32 nameHash
//          +40  u32 classHash
//          +48  u32 dataOffset (into 0x33320002 payload)
//          +52  u32 paletteOffset
//          +56  u32 dataSize
//          +68  u16 width      +70 u16 height
//          +72  u8 log2(width) +73 u8 log2(height)
//          +78  u8 mipmapCount
//       0x33310005 compression info, 32 bytes each: fourCC at +20
//     0xB3320000 TPKData
//       0x33320001 header
//       0x33320002 raw texture data (DXT blocks / ARGB)
//
// COMPRESSED variant (verified against CARS/TEXTURES.BIN — shared car
// template texture packs, e.g. CarTemplateTextures_SUPRA.tpk). Used instead
// of the standard 124-byte-entry layout above; chunk IDs under 0xB3310000
// differ accordingly:
//
//   0xB3300000 TPKContainer
//     0xB3310000 TPKInfo
//       0x33310001 header (same as standard variant)
//       0x33310002 hash table (reused for the compressed variant too)
//       0x33310003 descriptor entries, 24 bytes each:
//          +0   u32 nameHash
//          +4   u32 absFileOffset   (ABSOLUTE offset into the file, not
//                                    relative to any chunk — convert via
//                                    payload-relative = absFileOffset - absOffset)
//          +8   u32 jdlzCompSize
//          +12  u32 jdlzDecompSize  (= 156-byte internal header + pixel data)
//          +16  u32 flags          (observed: always 0x00000100)
//          +20  u32 padding
//     0xB3320000 TPKData
//       0x33320001 header
//       0x33320002 contains the JDLZ-compressed per-texture blobs referenced
//                  by the 0x33310003 descriptors (rather than one big raw
//                  pixel blob as in the standard variant)
//
//   Each JDLZ block decompresses to a 156-byte internal NFS texture header
//   (width/height/format fields not yet mapped) followed by raw pixel data.
//   Width/height here are inferred from the decompressed pixel payload size
//   (assumes ARGB32; see TPKBlockParser::Parse() for the heuristic and its
//   caveats). Textures parsed this way are flagged `compressedSource = true`
//   and are read-only as far as BunRepacker is concerned.
// ─────────────────────────────────────────────────────────────────────────────

/// Texture pixel format.
enum class TexFormat : uint8_t {
    Unknown = 0,
    DXT1,
    DXT3,
    DXT5,
    ARGB32,   ///< uncompressed
    PAL8,     ///< palettised (not GPU-uploaded by the editor)
};

inline const char* TexFormatName(TexFormat f) {
    switch (f) {
        case TexFormat::DXT1:   return "DXT1";
        case TexFormat::DXT3:   return "DXT3";
        case TexFormat::DXT5:   return "DXT5";
        case TexFormat::ARGB32: return "ARGB";
        case TexFormat::PAL8:   return "PAL8";
        default:                return "????";
    }
}

/// A single texture (raw payload + metadata).
struct Texture {
    std::string              name;
    uint32_t                 nameHash = 0;
    uint16_t                 width    = 0;
    uint16_t                 height   = 0;
    uint8_t                  mipmaps  = 1;
    TexFormat                format   = TexFormat::Unknown;
    std::vector<uint8_t>     data;     ///< raw DXT/ARGB bytes (all mip levels)

    /// Position of the original pixel data inside the pack's raw-data payload
    /// (relative offsets, recorded at parse for export / rebuild).
    uint32_t fileDataOffset = 0;
    uint32_t fileDataSize   = 0;

    /// True when the editor imported new pixel data (kept in `data` until
    /// the pack is saved).
    bool replaced = false;

    /// True when this texture's pixel data came from the compressed TPK
    /// variant (0x33310003 descriptors + per-entry JDLZ blocks, e.g.
    /// CARS/TEXTURES.BIN). `data` already holds fully-decompressed pixels,
    /// but there is no in-place slot to patch on disk — BunRepacker must
    /// refuse or full-repack rather than attempt Algorithm A on these.
    bool                      compressedSource = false;

    // OpenGL texture handle (0 = not uploaded)
    uint32_t glHandle = 0;
};

/// Parsed texture pack.
struct TPKBlock {
    std::string              name;      ///< e.g. "Region"
    std::string              path;      ///< e.g. "Location2\\TempTextures.tpk"
    uint32_t                 fileHash = 0;
    std::vector<Texture>     textures;
    std::unordered_map<uint32_t, size_t> hashIndex; ///< nameHash → textures index
    int                      parseWarnings = 0;

    // ── Source location (for export / rebuild) ───────────────────────────────
    std::filesystem::path sourceFile;        ///< file the chunk came from
    uint64_t  chunkOffset = 0;               ///< abs offset of the 8B chunk header
    uint32_t  chunkSize   = 0;               ///< payload size
    uint64_t  dataRawAbs  = 0;               ///< abs offset of pixel data (post-pad)
    bool      compressedSource = false;      ///< from a JDLZ file: read-only
    bool      structureDirty   = false;      ///< has replaced textures

    bool CanEdit() const {
        return !compressedSource && !sourceFile.empty() && chunkOffset != 0;
    }
    size_t ReplacedCount() const {
        size_t n = 0;
        for (const auto& t : textures) if (t.replaced) ++n;
        return n;
    }

    const Texture* FindByHash(uint32_t hash) const {
        auto it = hashIndex.find(hash);
        if (it == hashIndex.end()) return nullptr;
        return &textures[it->second];
    }
};

// ─────────────────────────────────────────────────────────────────────────────
class TPKBlockParser {
public:
    /// Parse a 0xB3300000 TPKContainer payload. `absOffset` is the absolute
    /// file offset of the payload (for export/rebuild bookkeeping).
    static Result<TPKBlock> Parse(std::span<const uint8_t> payload,
                                  uint64_t absOffset = 0);
};

// ─────────────────────────────────────────────────────────────────────────────
/// Minimal DDS reader/writer for DXT1/3/5 and 32-bit ARGB textures.
class DDSCodec {
public:
    /// Load a .dds file into `tex` (sets width/height/mips/format/data and
    /// marks it replaced). The texture name/hash are preserved.
    static Result<void> Import(Texture& tex, const std::filesystem::path& dds);

    /// Write a texture's pixel data as a .dds file. `pixels` must hold the
    /// full mip chain (use Texture::data or bytes re-read from the pack).
    static Result<void> Export(const Texture& tex, std::span<const uint8_t> pixels,
                               const std::filesystem::path& dds);
};

} // namespace nfsmw
