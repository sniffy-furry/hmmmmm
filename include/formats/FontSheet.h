#pragma once
// ─── formats/FontSheet.h ──────────────────────────────────────────────────────
// Font bitmap parser for GLOBAL/GLOBALB.LZC.
//
// After JDLZ decompression, GLOBALB.LZC is a standard EAGL chunk stream.
// Each font is represented as:
//   • A TPK texture pack containing the glyph atlas DDS (typically DXT1 or
//     DXT3, power-of-two, 256×256 or 512×256).
//   • A glyph table chunk (chunk ID not yet confirmed; see reconnaissance note
//     below) containing one GlyphEntry per codepoint.
//
// Reconnaissance note
// ───────────────────
// The glyph table chunk ID has not been confirmed from retail data.
// FontSheetParser::Parse() uses the same heuristic approach as HUDLayoutParser:
// unknown chunks whose payload divides evenly by kGlyphStride and contains
// plausible UV values are decoded; otherwise the raw bytes are preserved.
// Atlas export works regardless of whether the glyph table decodes.
//
// Phase 8v1: glyph UV editing is read-only (displayed in a table, not editable).
// ─────────────────────────────────────────────────────────────────────────────
#include "Common.h"
#include "formats/TPKBlock.h"
#include "core/ChunkReader.h"
#include <vector>
#include <string>
#include <span>

namespace nfsmw {

// ─────────────────────────────────────────────────────────────────────────────
// GlyphEntry — one character's UV rect in the atlas
// ─────────────────────────────────────────────────────────────────────────────
struct GlyphEntry {
    uint16_t codepoint = 0;   ///< Unicode codepoint (or game-specific index)
    float    u0 = 0.f;        ///< left UV  [0..1]
    float    v0 = 0.f;        ///< top UV   [0..1]
    float    u1 = 0.f;        ///< right UV [0..1]
    float    v1 = 0.f;        ///< bottom UV [0..1]
    float    advance = 0.f;   ///< horizontal advance in atlas-pixel units
};

// ─────────────────────────────────────────────────────────────────────────────
// FontSheet — one parsed font (atlas TPK + glyph table)
// ─────────────────────────────────────────────────────────────────────────────
struct FontSheet {
    std::string              name;       ///< from the TPK block name field
    TPKBlock                 atlasPack;  ///< the texture pack for this font
    std::vector<GlyphEntry>  glyphs;     ///< decoded glyph table (may be empty)
    std::vector<uint8_t>     rawGlyphBlob; ///< raw glyph chunk bytes (for hex view)
    bool                     glyphsDecoded = false;
};

// ─────────────────────────────────────────────────────────────────────────────
// FontSheetParser
// ─────────────────────────────────────────────────────────────────────────────
class FontSheetParser {
public:
    /// Parse the decompressed GLOBALB.LZC chunk stream.
    /// Returns one FontSheet per TPKBlock found in the stream.
    /// Glyph tables are associated with the preceding TPKBlock by order
    /// (the game interleaves them: TPK, glyphs, TPK, glyphs, …).
    static Result<std::vector<FontSheet>> Parse(std::span<const uint8_t> data,
                                                uint64_t absOffset = 0);

    /// Convenience: decompress `lzcPath` via LZCDecompressor, then call Parse().
    static Result<std::vector<FontSheet>> Load(const std::filesystem::path& lzcPath);

private:
    /// Expected stride of one on-disk glyph record.
    static constexpr uint32_t kGlyphStride = 20;

    /// Try to decode `payload` as a packed glyph table.
    static bool TryDecodeGlyphs(std::span<const uint8_t> payload,
                                 std::vector<GlyphEntry>& out);
};

} // namespace nfsmw
