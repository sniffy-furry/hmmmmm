#include "formats/StreamBundle.h"
#include "formats/SolidList.h"
#include "formats/TPKBlock.h"
#include "core/BINFile.h"
#include "core/LZCDecompressor.h"
#include "core/ChunkReader.h"
#include "core/Logger.h"
#include <cstring>
#include <fstream>
#include <vector>

namespace nfsmw {

// ─────────────────────────────────────────────────────────────────────────────
// StreamBundleLoader::ParseBlob
//
// Walks the top-level chunk sequence and dispatches:
//   0x80134000  GeometryContainer  → SolidListParser::Parse
//   0xB3300000  TPKContainer       → TPKBlockParser::Parse
// All other chunk IDs are skipped (ScenerySection, EventTriggers, etc. are
// out of scope for Phase 6 — see plan.md §6.1).
// ─────────────────────────────────────────────────────────────────────────────
Result<StreamSection> StreamBundleLoader::ParseBlob(std::span<const uint8_t> data,
                                                    uint64_t absOffset) {
    StreamSection section;

    size_t off = 0;
    while (off + sizeof(ChunkHeader) <= data.size()) {
        ChunkHeader hdr{};
        std::memcpy(&hdr, data.data() + off, sizeof(hdr));

        if (hdr.size == 0) {
            // Alignment filler or null chunk; skip past the header.
            off += sizeof(hdr);
            continue;
        }

        if (off + sizeof(hdr) + hdr.size > data.size()) {
            LOG_WARN("StreamBundle: chunk 0x{:08X} overruns blob at file offset {}",
                     hdr.id, absOffset + off);
            ++section.parseWarnings;
            break;
        }

        auto payload       = data.subspan(off + sizeof(hdr), hdr.size);
        const uint64_t payloadAbs = absOffset + off + sizeof(hdr);

        switch (hdr.id) {
            case ChunkID::GeometryContainer: {
                auto r = SolidListParser::Parse(payload, payloadAbs);
                if (r) {
                    section.solidLists.push_back(std::move(r.value));
                } else {
                    LOG_WARN("StreamBundle: SolidList parse error: {}", r.error);
                    ++section.parseWarnings;
                }
                break;
            }
            case ChunkID::TPKContainer: {
                auto r = TPKBlockParser::Parse(payload, payloadAbs);
                if (r) {
                    section.texturePacks.push_back(std::move(r.value));
                } else {
                    LOG_WARN("StreamBundle: TPKBlock parse error: {}", r.error);
                    ++section.parseWarnings;
                }
                break;
            }
            default:
                // Skip all other chunks (ScenerySection, EventSequences, etc.)
                break;
        }

        off += sizeof(hdr) + hdr.size;
    }

    return Result<StreamSection>::Ok(std::move(section));
}

// ─────────────────────────────────────────────────────────────────────────────
// StreamBundleLoader::Load
//
// Load a .BUN/.BIN from disk.  If the file starts with the JDLZ magic it is
// decompressed first via LZCDecompressor::Decompress.
// ─────────────────────────────────────────────────────────────────────────────
Result<StreamSection> StreamBundleLoader::Load(const std::filesystem::path& path) {
    // Read raw bytes.
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file)
        return Result<StreamSection>::Err("Cannot open '" + path.string() + "'");

    const std::streamsize sz = file.tellg();
    if (sz <= 0)
        return Result<StreamSection>::Err("Empty file: " + path.string());

    file.seekg(0);
    std::vector<uint8_t> raw(static_cast<size_t>(sz));
    if (!file.read(reinterpret_cast<char*>(raw.data()), sz))
        return Result<StreamSection>::Err("Read error: " + path.string());

    // JDLZ detection: magic bytes 'J','D','L','Z' at offset 0.
    std::span<const uint8_t> data;
    std::vector<uint8_t> decompressed;

    if (raw.size() >= 4 &&
        raw[0] == 'J' && raw[1] == 'D' && raw[2] == 'L' && raw[3] == 'Z') {
        auto r = LZCDecompressor::Decompress(raw);
        if (!r)
            return Result<StreamSection>::Err("JDLZ decompress failed for '" +
                                              path.string() + "': " + r.error);
        decompressed = std::move(r.value);
        data = decompressed;
        // Note: decompressed buffers lose their original file offsets so
        // absolute offsets passed to sub-parsers start at 0 (plan.md §3.2).
    } else {
        data = raw;
    }

    auto result = ParseBlob(data, /*absOffset=*/0);
    if (result) {
        result.value.name = path.stem().string();
        LOG_INFO("StreamBundle: loaded '{}': {} solid lists, {} texture packs, {} warnings",
                 path.filename().string(),
                 result.value.solidLists.size(),
                 result.value.texturePacks.size(),
                 result.value.parseWarnings);
    }
    return result;
}

} // namespace nfsmw
