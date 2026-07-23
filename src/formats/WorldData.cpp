// ─── formats/WorldData.cpp ────────────────────────────────────────────────────
// See WorldData.h. Self-contained chunk walk (no glm) so the module is
// independently unit-testable; mirrors the recursive descent ChunkReader uses.
#include "formats/WorldData.h"
#include <cstring>

namespace nfsmw {

namespace {

constexpr uint32_t kStreamingSections = 0x00034110u;
constexpr uint32_t kSceneryInfos      = 0x00034102u;
constexpr uint32_t kSceneryInstances  = 0x00034103u;
constexpr uint32_t kTriggerRegions    = 0x0003414Au;
constexpr uint32_t kWorldMapData      = 0x0003B800u;
constexpr uint32_t kLightSections     = 0x80036000u;
constexpr uint32_t kWeathermanChunk   = 0x00037080u;

inline uint32_t rd_u32(std::span<const uint8_t> p, size_t off) {
    uint32_t v = 0;
    if (off + 4 <= p.size()) std::memcpy(&v, p.data() + off, 4);
    return v;
}

// Collect fixed-stride {offset, index, word0} records from a leaf payload.
void collect(std::span<const uint8_t> payload, uint64_t absPayloadOff,
             uint32_t stride, std::vector<WorldRecord>& out) {
    if (stride == 0) return;
    size_t n = payload.size() / stride;
    for (size_t k = 0; k < n; ++k) {
        WorldRecord r;
        r.offset = absPayloadOff + k * stride;
        r.index  = static_cast<uint32_t>(k);
        r.word0  = rd_u32(payload, k * stride);
        out.push_back(r);
    }
}

void walk(std::span<const uint8_t> data, uint64_t base, WorldData& w, int depth) {
    if (depth > 8) { ++w.parseWarnings; return; }
    size_t off = 0;
    while (off + 8 <= data.size()) {
        uint32_t id, size;
        std::memcpy(&id,   data.data() + off,     4);
        std::memcpy(&size, data.data() + off + 4, 4);
        if (id == 0 && size == 0) { off += 8; continue; }  // null/pad
        if (static_cast<size_t>(off) + 8 + size > data.size()) { ++w.parseWarnings; break; }

        auto payload   = data.subspan(off + 8, size);
        uint64_t paOff = base + off + 8;

        switch (id) {
            case kStreamingSections: collect(payload, paOff, WorldDataParser::kSectionStride,         w.sections);         break;
            case kSceneryInfos:      collect(payload, paOff, WorldDataParser::kSceneryInfoStride,     w.sceneryInfos);     break;
            case kSceneryInstances:  collect(payload, paOff, WorldDataParser::kSceneryInstanceStride, w.sceneryInstances); break;
            case kTriggerRegions: {
                // Best-effort: many region records open with a type tag + flags.
                // Expose the chunk as one trigger entry per 16 bytes if it tiles
                // evenly, else a single entry capturing the chunk head.
                if (size >= 16 && (size % 16) == 0) {
                    for (size_t k = 0; k + 16 <= size; k += 16) {
                        WorldTrigger t;
                        t.offset  = paOff + k;
                        t.typeTag = rd_u32(payload, k);
                        t.word1   = rd_u32(payload, k + 4);
                        w.triggers.push_back(t);
                    }
                } else {
                    WorldTrigger t;
                    t.offset  = paOff;
                    t.typeTag = rd_u32(payload, 0);
                    t.word1   = rd_u32(payload, 4);
                    w.triggers.push_back(t);
                }
                break;
            }
            case kWorldMapData:    w.roadNetworkBytes = size; break;
            case kLightSections:   w.lightBytes       = size; break;
            case kWeathermanChunk: w.weatherBytes     = size; break;
            default:
                if (id & 0x80000000u) walk(payload, paOff, w, depth + 1);  // descend containers
                break;
        }
        off += 8 + size;
    }
}

} // namespace

Result<WorldData> WorldDataParser::Parse(std::span<const uint8_t> data, uint64_t absOffset) {
    if (data.size() < 8)
        return Result<WorldData>::Err("WorldData: buffer too small to contain a chunk");
    WorldData w;
    walk(data, absOffset, w, 0);
    return Result<WorldData>::Ok(std::move(w));
}

} // namespace nfsmw
