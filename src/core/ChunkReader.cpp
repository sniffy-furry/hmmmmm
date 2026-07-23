#include "core/ChunkReader.h"
#include "core/Logger.h"
#include <cstring>

namespace nfsmw {

void ChunkReader::Parse(std::span<const uint8_t> data, size_t baseOffset) const {
    size_t offset = 0;
    while (offset + sizeof(ChunkHeader) <= data.size()) {
        ChunkHeader hdr{};
        std::memcpy(&hdr, data.data() + offset, sizeof(ChunkHeader));

        const size_t payloadEnd = offset + sizeof(ChunkHeader) + hdr.size;
        if (payloadEnd > data.size()) {
            LOG_WARN("ChunkReader: chunk 0x{:08X} overruns buffer (offset={}, size={})",
                     hdr.id, baseOffset + offset, hdr.size);
            break;
        }

        auto payload = data.subspan(offset + sizeof(ChunkHeader), hdr.size);
        const size_t absOffset = baseOffset + offset + sizeof(ChunkHeader);

        auto it = handlers_.find(hdr.id);
        if (it != handlers_.end()) {
            it->second(hdr.id, payload, absOffset);
        } else if (unknownHandler_) {
            unknownHandler_(hdr.id, payload, absOffset);
        }

        offset = payloadEnd;
    }
}

void ChunkReader::ParseRecursive(std::span<const uint8_t> data, size_t baseOffset, int depth) const {
    if (depth > 16) return; // sanity guard
    size_t offset = 0;
    while (offset + sizeof(ChunkHeader) <= data.size()) {
        ChunkHeader hdr{};
        std::memcpy(&hdr, data.data() + offset, sizeof(ChunkHeader));

        const size_t payloadEnd = offset + sizeof(ChunkHeader) + hdr.size;
        if (payloadEnd > data.size()) {
            LOG_WARN("ChunkReader::ParseRecursive: chunk 0x{:08X} overruns buffer at {}",
                     hdr.id, baseOffset + offset);
            break;
        }

        auto payload = data.subspan(offset + sizeof(ChunkHeader), hdr.size);
        const size_t absOffset = baseOffset + offset + sizeof(ChunkHeader);

        auto it = handlers_.find(hdr.id);
        if (it != handlers_.end()) {
            it->second(hdr.id, payload, absOffset);
        } else if (IsContainer(hdr.id) && hdr.size >= sizeof(ChunkHeader)) {
            ParseRecursive(payload, absOffset, depth + 1);
        } else if (unknownHandler_) {
            unknownHandler_(hdr.id, payload, absOffset);
        }

        offset = payloadEnd;
    }
}

// static
void ChunkReader::Dump(std::span<const uint8_t> data, int depth) {
    const std::string indent(static_cast<size_t>(depth) * 2, ' ');
    size_t offset = 0;
    while (offset + sizeof(ChunkHeader) <= data.size()) {
        ChunkHeader hdr{};
        std::memcpy(&hdr, data.data() + offset, sizeof(ChunkHeader));

        const size_t payloadEnd = offset + sizeof(ChunkHeader) + hdr.size;
        if (payloadEnd > data.size()) break;

        LOG_DEBUG("{}[0x{:08X}] size={}", indent, hdr.id, hdr.size);

        if (IsContainer(hdr.id) && hdr.size >= sizeof(ChunkHeader)) {
            auto sub = data.subspan(offset + sizeof(ChunkHeader), hdr.size);
            Dump(sub, depth + 1);
        }

        offset = payloadEnd;
    }
}

} // namespace nfsmw
