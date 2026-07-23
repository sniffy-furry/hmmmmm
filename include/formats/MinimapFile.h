#pragma once
// ─── formats/MinimapFile.h ────────────────────────────────────────────────────
// Parser and repacker for MINI_MAP.BIN — the tiled overhead-map texture used
// by the in-game GPS/minimap overlay.
//
// ── On-disk layout ───────────────────────────────────────────────────────────
//
//   File = N contiguous outer chunks, each:
//     [+0]  uint32  chunkID   = 0x0003A100  (not in Common.h ChunkID namespace;
//                                            unique to this file type)
//     [+4]  uint32  chunkSize              (payload byte count, excl. header)
//     [+8]  <chunkSize bytes>              JDLZ-compressed TPK block
//
//   Each JDLZ payload decompresses (via LZCDecompressor) to a standard
//   0xB3300000 TPK block containing exactly one texture entry:
//     name:    MINI_MAP_CHOPn   (n = 0-based tile index)
//     size:    128 × 128 pixels
//     format:  DXT3  (comprType byte = 0x24)
//     mipmaps: 0 (base level only)
//
//   Tile grid layout (verified against retail Rockport minimap):
//     gridCols = 8,  gridRows = 8  (64 tiles total)
//     tile n → col = n % 8,  row = n / 8
//     composited image = 1024 × 1024 px (8 × 128)
//
// ── TPK entry field offsets (124-byte TexInfoEntry) ─────────────────────────
//   These match BunRepacker::TexEntry and are verified against this file:
//     +0   pad[12]
//     +12  name[24]          "MINI_MAP_CHOPn\0"
//     +36  nameHash (u32)    Joaat of name
//     +40  classNameHash (u32)
//     +44  imageParentHash (u32)
//     +48  imagePlacement (u32)   byte offset from pixel base
//     +52  palettePlacement (u32)
//     +56  imageSize (u32)        total pixel bytes (= 16384 for 128×128 DXT3)
//     +60  paletteSize (u32)
//     +64  baseImageSize (u32)    first mip (= imageSize when mips = 0)
//     +68  width (u16)            128
//     +70  height (u16)           128
//     +72  shiftWidth (u8)        7  (log2 128)
//     +73  shiftHeight (u8)       7
//     +74  comprType (u8)         0x24 = DXT3
//     +75  palComprType (u8)
//     +76  numPaletteEntries (u16)
//     +78  numMipLevels (u8)      0
//     +79  pad[45]
//
// ── Chunk ID note ────────────────────────────────────────────────────────────
//   0x0003A100 is NOT a container chunk (bit 31 clear) in the usual sense —
//   it acts as a flat envelope for one compressed TPK payload.  Common.h does
//   not list it; we define kChunkMiniMapTile below.
// ─────────────────────────────────────────────────────────────────────────────
#include "Common.h"
#include "core/LZCDecompressor.h"
#include "patch/BackupManager.h"
#include <filesystem>
#include <string>
#include <vector>
#include <cstdint>

namespace nfsmw {

// ─────────────────────────────────────────────────────────────────────────────
// Constants
// ─────────────────────────────────────────────────────────────────────────────

/// Outer chunk ID wrapping each JDLZ-compressed TPK tile payload.
constexpr uint32_t kChunkMiniMapTile = 0x0003A100u;

/// DXT3 comprType byte stored in the TexInfoEntry.
constexpr uint8_t kDXT3ComprType = 0x24u;

/// Pixel dimensions of each tile.
constexpr uint32_t kTileWidth  = 128u;
constexpr uint32_t kTileHeight = 128u;

/// Bytes for one 128×128 DXT3 tile (4 bits per pixel, block-compressed).
// DXT3 uses 16 bytes per 4×4 block (8-byte explicit-alpha block + 8-byte DXT1
// colour block), giving 1 byte per pixel.  The previous formula (/2) was the
// DXT1 (4 bits/pixel) size — half what DXT3 actually requires.  Using the wrong
// value caused ParseTPK to store only 8 192 bytes per tile and DecodeDXT3 to
// read 16 384 bytes from that buffer, an out-of-bounds read that crashed the app
// on every MINI_MAP.BIN load.
constexpr uint32_t kTilePixelBytes = kTileWidth * kTileHeight; // 16384  (DXT3: 1 byte/pixel)

// ─────────────────────────────────────────────────────────────────────────────
// MinimapTile — one decoded tile
// ─────────────────────────────────────────────────────────────────────────────
struct MinimapTile {
    std::string name;          ///< e.g. "MINI_MAP_CHOP0"
    uint32_t    nameHash = 0;  ///< Joaat of name (from TexInfoEntry)
    uint32_t    index    = 0;  ///< 0-based tile index within the file

    /// Raw DXT3 pixel data (kTilePixelBytes bytes).
    /// Populated by MinimapParser::Load(); replaced in-memory by ReplaceTile().
    std::vector<uint8_t> dxt3Data;

    /// Absolute byte range of the compressed outer chunk within the source file.
    /// Used by MinimapRepacker to overwrite just this tile's chunk on disk.
    uint64_t fileChunkOffset = 0;   ///< offset of the 0x0003A100 header
    uint32_t fileChunkSize   = 0;   ///< total bytes of header + compressed payload
};

// ─────────────────────────────────────────────────────────────────────────────
// MinimapFile — parsed representation of a full MINI_MAP.BIN
// ─────────────────────────────────────────────────────────────────────────────
struct MinimapFile {
    std::filesystem::path    path;
    std::vector<MinimapTile> tiles;    ///< in file order (tile 0 → top-left)

    int gridCols = 0;   ///< filled by MinimapParser::Load()
    int gridRows = 0;

    uint32_t TileCount()    const { return static_cast<uint32_t>(tiles.size()); }
    uint32_t CompositeW()   const { return static_cast<uint32_t>(gridCols) * kTileWidth;  }
    uint32_t CompositeH()   const { return static_cast<uint32_t>(gridRows) * kTileHeight; }

    const MinimapTile* FindByName(std::string_view name) const {
        for (const auto& t : tiles)
            if (t.name == name) return &t;
        return nullptr;
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// MinimapParser
// ─────────────────────────────────────────────────────────────────────────────
class MinimapParser {
public:
    /// Parse MINI_MAP.BIN from `path`.
    /// Decompresses each JDLZ chunk and extracts DXT3 pixel data.
    /// Returns Err() on any unrecoverable IO or format error.
    static Result<MinimapFile> Load(const std::filesystem::path& path);

    /// Walk a chunk tree starting at `buf[offset]` to find `targetID`.
    /// Returns the offset of the payload (after the 8-byte header), or
    /// std::string::npos on failure.
    /// Public so MinimapRepacker helpers can reuse it.
    static size_t FindChunk(const uint8_t* buf, size_t bufLen,
                            size_t offset, uint32_t targetID);

private:
    /// Parse one decompressed TPK payload and populate `tile`.
    /// Returns false on format error.
    static bool ParseTPK(const std::vector<uint8_t>& decompressed,
                         MinimapTile& tile);

    /// Compute grid dimensions from tile count:
    ///   cols = ceil(sqrt(n)),  rows = ceil(n / cols)
    static void ComputeGrid(int n, int& cols, int& rows);
};

// ─────────────────────────────────────────────────────────────────────────────
// MinimapRepacker — write a replacement tile back to MINI_MAP.BIN
// ─────────────────────────────────────────────────────────────────────────────

/// Result of a single tile replacement.
struct TileReplaceResult {
    uint32_t    tileIndex = 0;
    bool        ok        = false;
    std::string error;
};

class MinimapRepacker {
public:
    /// Replace the DXT3 pixel data for `tile` in the on-disk file `path`.
    ///
    /// `newDXT3` must be exactly kTilePixelBytes bytes (16384) of raw DXT3 data
    /// for a 128×128 texture.  If the replacement is the same size as the
    /// original (always true for same-dimension tiles), Algorithm A from
    /// BunRepacker is used: re-compress the payload with JDLZ, then overwrite
    /// the outer chunk region at tile.fileChunkOffset in-place.
    ///
    /// If the re-compressed size exceeds the original compressed chunk size, a
    /// full-file rewrite is performed (all other chunks are preserved verbatim).
    ///
    /// `bm` records a .bak before the first write in a session.
    /// `file.tiles[tile.index].dxt3Data` is updated in-memory on success.
    ///
    /// Returns error string on failure.
    static TileReplaceResult ReplaceTile(MinimapFile& file,
                                         uint32_t tileIndex,
                                         const std::vector<uint8_t>& newDXT3,
                                         BackupManager& bm);

    /// Decompress PNG or DDS bytes to raw 128×128 DXT3 (kTilePixelBytes bytes).
    ///
    /// Accepted inputs:
    ///   • .dds  — must be 128×128 DXT3; pixel data extracted directly.
    ///   • .png  — decoded to RGBA then block-compressed to DXT3 via
    ///             a simple fixed-quality encoder.  Alpha is preserved.
    ///
    /// Returns Err() if the image is the wrong size or format is unsupported.
    static Result<std::vector<uint8_t>> PrepareSourceImage(
        const std::filesystem::path& srcPath);

private:
    /// Re-compress `decompressed` with JDLZ and write one outer chunk
    /// (0x0003A100 header + compressed payload) to `dest`.
    static std::vector<uint8_t> BuildChunk(const std::vector<uint8_t>& decompressed);

    /// JDLZ compress `src` → compressed bytes (16-byte header + LZ77 stream).
    static std::vector<uint8_t> CompressJDLZ(const std::vector<uint8_t>& src);

    /// Patch the TexInfoEntry inside a decompressed TPK with new pixel data.
    /// Returns the updated decompressed payload.
    static std::vector<uint8_t> PatchTPKPixels(
        const std::vector<uint8_t>& decompressed,
        const std::vector<uint8_t>& newDXT3);

    // ── Simple DXT3 encoder (RGBA → DXT3) ───────────────────────────────────
    // Produces output compatible with the game's texture decoder.
    // Quality is adequate for map tiles (flat overhead art with large colour
    // regions); it is not a high-quality encoder for photographic content.
    static std::vector<uint8_t> EncodeDXT3(
        const uint8_t* rgba, uint32_t w, uint32_t h);

    static void EncodeDXT3Block(
        const uint8_t rgba[4][4][4],   ///< [row][col][RGBA]
        uint8_t out[16]);
};

} // namespace nfsmw
