// ─── formats/FontSheet.cpp ────────────────────────────────────────────────────
#include "formats/FontSheet.h"
#include "core/LZCDecompressor.h"
#include "core/Logger.h"
#include <cstring>
#include <cmath>
#include <fstream>

namespace nfsmw {

// ─────────────────────────────────────────────────────────────────────────────
// TryDecodeGlyphs
//
// Expected on-disk glyph record layout (20 bytes, little-endian, unverified):
//   +0   u16  codepoint
//   +2   u16  reserved
//   +4   f32  u0
//   +8   f32  v0
//   +12  f32  u1
//   +16  f32  v1
//   (advance inferred as (u1 - u0) * atlasWidth; stored separately elsewhere)
//
// Heuristic acceptance:
//   • size divisible by kGlyphStride
//   • all UV values finite and in [0..1]
//   • at least 16 entries (a minimal ASCII subset)
// ─────────────────────────────────────────────────────────────────────────────
bool FontSheetParser::TryDecodeGlyphs(std::span<const uint8_t> payload,
                                       std::vector<GlyphEntry>& out)
{
    if (payload.size() < kGlyphStride * 16) return false;
    if (payload.size() % kGlyphStride != 0) return false;

    const size_t count = payload.size() / kGlyphStride;
    std::vector<GlyphEntry> tmp;
    tmp.reserve(count);

    auto readU16 = [&](size_t off) -> uint16_t {
        uint16_t v; std::memcpy(&v, payload.data() + off, 2); return v;
    };
    auto readF32 = [&](size_t off) -> float {
        float v; std::memcpy(&v, payload.data() + off, 4); return v;
    };
    auto inRange = [](float v) { return std::isfinite(v) && v >= 0.f && v <= 1.f; };

    for (size_t i = 0; i < count; ++i) {
        const size_t base = i * kGlyphStride;
        float u0 = readF32(base + 4);
        float v0 = readF32(base + 8);
        float u1 = readF32(base + 12);
        float v1 = readF32(base + 16);

        if (!inRange(u0) || !inRange(v0) || !inRange(u1) || !inRange(v1))
            return false;

        GlyphEntry g;
        g.codepoint = readU16(base + 0);
        g.u0 = u0; g.v0 = v0;
        g.u1 = u1; g.v1 = v1;
        g.advance = (u1 - u0);   // atlas-fraction units; UI scales by atlas width
        tmp.push_back(g);
    }

    out = std::move(tmp);
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// Parse
//
// Walk the decompressed chunk stream.
// Association heuristic: each non-TPK unknown chunk immediately follows the
// TPK it annotates (font0_TPK, font0_glyphs, font1_TPK, font1_glyphs, …).
// If a second non-TPK chunk appears before the next TPK, it is appended to
// the raw blob of the current sheet without replacing an already-decoded table.
// ─────────────────────────────────────────────────────────────────────────────
Result<std::vector<FontSheet>> FontSheetParser::Parse(std::span<const uint8_t> data,
                                                       uint64_t absOffset)
{
    if (data.size() < 8)
        return Result<std::vector<FontSheet>>::Err("FontSheet: buffer too small");

    std::vector<FontSheet> sheets;
    FontSheet*             current = nullptr;   // sheet currently being built

    ChunkReader reader;

    // ── TPK container — start a new FontSheet ──
    reader.RegisterHandler(ChunkID::TPKContainer,
        [&](uint32_t /*id*/, std::span<const uint8_t> payload)
        {
            auto res = TPKBlockParser::Parse(payload, absOffset);
            if (!res) {
                LOG_WARN("FontSheet: TPK parse error: {}", res.error);
                return;
            }
            FontSheet sheet;
            sheet.name      = res.value.name;
            sheet.atlasPack = std::move(res.value);
            sheets.push_back(std::move(sheet));
            current = &sheets.back();
            LOG_DEBUG("FontSheet: TPK '{}' — {} textures",
                      current->name, current->atlasPack.textures.size());
        });

    // ── Everything else — try glyph decode, always keep raw bytes ──
    reader.SetUnknownHandler(
        [&](uint32_t /*id*/, std::span<const uint8_t> payload, size_t /*off*/)
        {
            if (!current) return;   // stray chunk before any TPK — ignore

            // Always accumulate raw bytes.
            current->rawGlyphBlob.insert(current->rawGlyphBlob.end(),
                                         payload.begin(), payload.end());

            // Attempt glyph decode if not already done for this sheet.
            if (!current->glyphsDecoded) {
                auto stripped = StripAlignPad(payload);
                if (TryDecodeGlyphs(stripped, current->glyphs)) {
                    current->glyphsDecoded = true;
                    LOG_INFO("FontSheet '{}': decoded {} glyphs",
                             current->name, current->glyphs.size());
                }
            }
        });

    reader.ParseRecursive(data, absOffset);

    LOG_INFO("FontSheet: parse complete — {} sheets", sheets.size());
    return Result<std::vector<FontSheet>>::Ok(std::move(sheets));
}

// ─────────────────────────────────────────────────────────────────────────────
// Load — decompress LZC then parse
// ─────────────────────────────────────────────────────────────────────────────
Result<std::vector<FontSheet>> FontSheetParser::Load(
    const std::filesystem::path& lzcPath)
{
    // Read file.
    std::ifstream f(lzcPath, std::ios::binary);
    if (!f)
        return Result<std::vector<FontSheet>>::Err(
            "FontSheet: cannot open '" + lzcPath.string() + "'");

    std::vector<uint8_t> raw((std::istreambuf_iterator<char>(f)),
                              std::istreambuf_iterator<char>());

    // Decompress if needed.
    std::vector<uint8_t> data;
    if (LZCDecompressor::IsCompressed(raw)) {
        auto res = LZCDecompressor::Decompress(raw);
        if (!res)
            return Result<std::vector<FontSheet>>::Err(
                "FontSheet: decompression failed: " + res.error);
        data = std::move(res.value);
    } else {
        data = std::move(raw);
    }

    return Parse(std::span<const uint8_t>(data), 0);
}

} // namespace nfsmw
