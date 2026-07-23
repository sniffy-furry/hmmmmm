#pragma once
// ─── formats/HUDLayout.h ──────────────────────────────────────────────────────
// GLOBALHUD.BUN layout table parser.
//
// GLOBALHUD.BUN contains a TPK texture atlas (parsed by TPKBlockParser via
// StreamBundleLoader as usual) plus a proprietary layout chunk that positions
// each HUD element (speedometer needle, minimap frame, heat meter, etc.) in
// normalised screen space.
//
// Chunk ID reconnaissance
// ───────────────────────
// The layout chunk ID has not yet been confirmed against the retail binary.
// HUDLayoutParser::Parse() is therefore built as a best-effort decoder:
//   • It registers a catch-all unknown-chunk handler on ChunkReader.
//   • For any chunk whose payload matches the fixed-stride layout record
//     format (stride = 32 bytes, divisible count, sensible anchor values),
//     it attempts a decode and records the results.
//   • If no layout chunk is found the rawLayoutBlob is still populated (all
//     non-TPK chunk bytes concatenated) so the UI can display a hex view
//     and the decode can be wired once the chunk ID is confirmed.
//
// HUDElement anchor/size editing is intentionally read-only in Phase 8v1.
// The Save() path is scaffolded but returns an error until the chunk ID is
// confirmed; this keeps the UI wired without risking blind writes.
// ─────────────────────────────────────────────────────────────────────────────
#include "Common.h"
#include "core/ChunkReader.h"
#include <vector>
#include <string>
#include <span>

namespace nfsmw {

// ─────────────────────────────────────────────────────────────────────────────
// HUDElement — one positioned screen-space widget
// ─────────────────────────────────────────────────────────────────────────────
struct HUDElement {
    uint32_t  nameHash = 0;    ///< Joaat — resolves into the HUD atlas TPK
    std::string name;          ///< ASCII name from the record (may be empty)

    glm::vec2 anchor{0.f, 0.f}; ///< normalised screen position [0..1]
    glm::vec2 size{0.f, 0.f};   ///< normalised extent [0..1]
    float     rotation = 0.f;   ///< radians

    uint32_t  flags = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
// HUDLayout — parsed contents of the HUD layout chunk
// ─────────────────────────────────────────────────────────────────────────────
struct HUDLayout {
    std::vector<HUDElement>  elements;

    /// All non-TPK chunk bytes from the BUN, concatenated, for the hex view
    /// and for future in-place patching once the chunk ID is confirmed.
    std::vector<uint8_t>     rawLayoutBlob;

    /// Absolute file offset of rawLayoutBlob's first byte (0 = unknown).
    uint64_t rawOffset = 0;

    bool decoded = false;   ///< true if elements was successfully populated
};

// ─────────────────────────────────────────────────────────────────────────────
// HUDLayoutParser
// ─────────────────────────────────────────────────────────────────────────────
class HUDLayoutParser {
public:
    /// Walk all non-TPK chunks in `data` (a raw GLOBALHUD.BUN buffer).
    /// Always populates rawLayoutBlob; also populates elements if the layout
    /// chunk is recognised by its stride pattern.
    /// `absOffset` is the absolute file position of the start of `data`.
    static Result<HUDLayout> Parse(std::span<const uint8_t> data,
                                   uint64_t absOffset = 0);

    /// Patch anchor/size of a single element back into rawLayoutBlob.
    /// Returns Err() until the chunk ID is confirmed (Phase 8v1 is read-only).
    static Result<void> Save(HUDLayout& layout, size_t elementIndex,
                              glm::vec2 anchor, glm::vec2 size);

private:
    /// Stride of one on-disk HUD record, as inferred from retail recon.
    static constexpr uint32_t kRecordStride = 32;

    /// Try to decode a raw payload as a sequence of HUDElement records.
    /// Returns false if the payload does not match the expected pattern.
    static bool TryDecodePayload(std::span<const uint8_t> payload,
                                 std::vector<HUDElement>& out);
};

} // namespace nfsmw
