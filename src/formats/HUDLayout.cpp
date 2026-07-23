// ─── formats/HUDLayout.cpp ────────────────────────────────────────────────────
#include "formats/HUDLayout.h"
#include "formats/TPKBlock.h"   // ChunkID::TPKContainer
#include "core/Logger.h"
#include <cstring>
#include <cmath>

namespace nfsmw {

// ─────────────────────────────────────────────────────────────────────────────
// TryDecodePayload
//
// Attempt to decode `payload` as a packed array of HUDElement records.
// Expected on-disk record layout (32 bytes, little-endian, unverified):
//   +0   u32  nameHash
//   +4   f32  anchorX
//   +8   f32  anchorY
//   +12  f32  sizeW
//   +16  f32  sizeH
//   +20  f32  rotation
//   +24  u32  flags
//   +28  u32  reserved
//
// Heuristic acceptance criteria (all must pass):
//   • payload size divisible by kRecordStride
//   • at least one record
//   • all anchor values in [-0.1 .. 1.1] and size values in [0 .. 2]
//     (a small tolerance handles sub-pixel offsets)
//   • no NaN / Inf floats
// ─────────────────────────────────────────────────────────────────────────────
bool HUDLayoutParser::TryDecodePayload(std::span<const uint8_t> payload,
                                       std::vector<HUDElement>& out)
{
    if (payload.size() < kRecordStride) return false;
    if (payload.size() % kRecordStride != 0) return false;

    const size_t count = payload.size() / kRecordStride;
    std::vector<HUDElement> tmp;
    tmp.reserve(count);

    auto readF32 = [&](size_t off) -> float {
        float v; std::memcpy(&v, payload.data() + off, 4); return v;
    };
    auto readU32 = [&](size_t off) -> uint32_t {
        uint32_t v; std::memcpy(&v, payload.data() + off, 4); return v;
    };
    auto finite = [](float v) { return std::isfinite(v); };

    for (size_t i = 0; i < count; ++i) {
        const size_t base = i * kRecordStride;

        float ax = readF32(base + 4);
        float ay = readF32(base + 8);
        float sw = readF32(base + 12);
        float sh = readF32(base + 16);
        float rot= readF32(base + 20);

        if (!finite(ax) || !finite(ay) || !finite(sw) || !finite(sh) || !finite(rot))
            return false;
        if (ax < -0.1f || ax > 1.1f || ay < -0.1f || ay > 1.1f)
            return false;
        if (sw < 0.f || sw > 2.f || sh < 0.f || sh > 2.f)
            return false;

        HUDElement e;
        e.nameHash = readU32(base + 0);
        e.anchor   = {ax, ay};
        e.size     = {sw, sh};
        e.rotation = rot;
        e.flags    = readU32(base + 24);
        tmp.push_back(e);
    }

    out = std::move(tmp);
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// Parse
// ─────────────────────────────────────────────────────────────────────────────
Result<HUDLayout> HUDLayoutParser::Parse(std::span<const uint8_t> data,
                                         uint64_t absOffset)
{
    HUDLayout layout;
    layout.rawOffset = absOffset;

    if (data.size() < 8) {
        return Result<HUDLayout>::Err("HUDLayout: buffer too small");
    }

    // Walk the top-level chunk tree.
    // TPK containers are skipped (StreamBundleLoader handles them separately).
    // Everything else is appended to rawLayoutBlob, and each payload is
    // tested as a candidate layout record array.

    ChunkReader reader;

    reader.SetUnknownHandler(
        [&](uint32_t id, std::span<const uint8_t> payload, size_t /*off*/)
        {
            // Skip TPK containers — those belong to StreamBundleLoader.
            if (id == ChunkID::TPKContainer) return;

            // Append raw bytes for the hex view.
            const size_t prevSize = layout.rawLayoutBlob.size();
            layout.rawLayoutBlob.insert(layout.rawLayoutBlob.end(),
                                        payload.begin(), payload.end());

            // Attempt layout decode if not already found.
            if (!layout.decoded) {
                std::vector<HUDElement> candidates;
                auto stripped = StripAlignPad(payload);
                if (TryDecodePayload(stripped, candidates)) {
                    layout.elements = std::move(candidates);
                    layout.decoded  = true;
                    LOG_INFO("HUDLayout: decoded {} elements from chunk 0x{:08X}",
                             layout.elements.size(), id);
                }
            }

            (void)prevSize;
        });

    reader.ParseRecursive(data, absOffset);

    if (layout.decoded) {
        LOG_INFO("HUDLayout: parse complete — {} elements, {} raw bytes",
                 layout.elements.size(), layout.rawLayoutBlob.size());
    } else {
        LOG_WARN("HUDLayout: layout chunk not recognised — {} raw bytes preserved",
                 layout.rawLayoutBlob.size());
    }

    return Result<HUDLayout>::Ok(std::move(layout));
}

// ─────────────────────────────────────────────────────────────────────────────
// Save — Phase 8v1: read-only; wired but disabled until chunk ID confirmed
// ─────────────────────────────────────────────────────────────────────────────
Result<void> HUDLayoutParser::Save(HUDLayout& /*layout*/, size_t /*elementIndex*/,
                                   glm::vec2 /*anchor*/, glm::vec2 /*size*/)
{
    return Result<void>::Err(
        "HUDLayout: element editing is read-only in Phase 8v1. "
        "The chunk ID must be confirmed against the retail binary before "
        "in-place patching is enabled.");
}

} // namespace nfsmw
