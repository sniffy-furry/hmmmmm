#pragma once
// ─── formats/WorldData.h ──────────────────────────────────────────────────────
// Structural reader for the world / track-stream layers that StreamBundleLoader
// deliberately skips (geometry + TPK are handled there). This recovers the
// *record boundaries* — sections, scenery model definitions, scenery instances,
// and trigger regions — plus the presence/size of the preserve-raw subsystems
// (AI road network, lights, weather).
//
// Scope (roadmap W1/W2/W3, read side): like VaultFile does for NtaD, this
// exposes verified record boundaries (counts, offsets, first dword) without
// claiming a byte-exact field map for the records whose internal layout is not
// yet confirmed. That alone lets callers answer "how many props / sections /
// triggers does this bundle have, and where are they" — the foundation for an
// editor — and is fully round-trip-safe because nothing is rewritten.
//
// Chunk IDs (verified, see Common.h / the encyclopedia chunk-ID table):
//   0x00034110 StreamingSections   92 bytes/entry  (section table)
//   0x00034102 SceneryInfos        72 bytes/entry  (model definitions)
//   0x00034103 SceneryInstances    64 bytes/entry  (placements)
//   0x0003414A TriggerRegions      typed 2-D regions (under 0x80034147)
//   0x0003B800 WorldMapData        'CARP' road network (preserve raw)
//   0x80036000 LightSections / 0x00037080 WeathermanChunk (preserve raw)
//
// Deliberately GL/GLM-free (plain floats) so it builds and unit-tests anywhere
// the core library does.
// ─────────────────────────────────────────────────────────────────────────────
#include "Common.h"
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace nfsmw {

/// One fixed-stride record located inside a world chunk. `word0` is the first
/// u32 of the record (often an index/number/hash — interpretation depends on
/// the record type and is left to the caller / future schema work).
struct WorldRecord {
    uint64_t offset = 0;  ///< absolute file offset of the record's first byte
    uint32_t index  = 0;  ///< 0-based position within its table
    uint32_t word0  = 0;  ///< first u32 of the record
};

/// A trigger region, decoded best-effort: the type tag and the chunk-relative
/// span. Footprint floats are exposed raw (not interpreted) until confirmed.
struct WorldTrigger {
    uint64_t offset = 0;
    uint32_t typeTag = 0; ///< first u32 — region type/category
    uint32_t word1   = 0; ///< second u32 (often a flags/id field)
};

/// Structural contents of a world / track-stream bundle.
struct WorldData {
    std::vector<WorldRecord>  sections;          ///< 0x00034110 (92B each)
    std::vector<WorldRecord>  sceneryInfos;      ///< 0x00034102 (72B each)
    std::vector<WorldRecord>  sceneryInstances;  ///< 0x00034103 (64B each)
    std::vector<WorldTrigger> triggers;          ///< 0x0003414A

    // Preserve-raw subsystems: presence + payload byte size (0 = absent).
    uint32_t roadNetworkBytes = 0;   ///< 0x0003B800 WorldMapData (CARP)
    uint32_t lightBytes       = 0;   ///< 0x80036000 LightSections
    uint32_t weatherBytes     = 0;   ///< 0x00037080 WeathermanChunk
    int      parseWarnings    = 0;

    size_t TotalRecords() const {
        return sections.size() + sceneryInfos.size() + sceneryInstances.size() + triggers.size();
    }
};

class WorldDataParser {
public:
    /// Parse a raw (already JDLZ-decompressed) chunk buffer. `absOffset` is the
    /// file offset of `data` so reported offsets are absolute. Recurses into
    /// container chunks. Never fails on unknown chunks (they are ignored);
    /// returns Err only on a structurally impossible buffer.
    static Result<WorldData> Parse(std::span<const uint8_t> data, uint64_t absOffset = 0);

    // Record strides (verified).
    static constexpr uint32_t kSectionStride  = 92;
    static constexpr uint32_t kSceneryInfoStride     = 72;
    static constexpr uint32_t kSceneryInstanceStride = 64;
};

} // namespace nfsmw
