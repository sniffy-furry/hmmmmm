#include "core/BINFile.h"
#include "core/LZCDecompressor.h"
#include "core/Logger.h"

#include <fstream>
#include <cstring>

namespace nfsmw {

// ─── JDLZ magic ──────────────────────────────────────────────────────────────
// JDLZ header layout (16 bytes):
//   [0..3]  'JDLZ'
//   [4]     version  (1)
//   [5]     unknown  (1)
//   [6..7]  headerSize (0x10 LE)
//   [8..11] decompressedSize (LE)
//   [12..15] compressedSize  (LE)
static constexpr uint8_t  kJDLZMagic[4] = { 'J','D','L','Z' };
static constexpr uint32_t kJDLZHeaderSize = 16;

static bool IsJDLZ(const std::vector<uint8_t>& data) {
    if (data.size() < kJDLZHeaderSize) return false;
    return std::memcmp(data.data(), kJDLZMagic, 4) == 0;
}

// ─── Open (Phase 1 factory) ──────────────────────────────────────────────────
Result<BINFile> BINFile::Open(const std::filesystem::path& path) {
    // 1. Read raw bytes.
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f)
        return Result<BINFile>::Err("Cannot open file: " + path.string());

    const std::streamsize rawSize = f.tellg();
    if (rawSize <= 0)
        return Result<BINFile>::Err("File is empty: " + path.string());

    f.seekg(0, std::ios::beg);
    std::vector<uint8_t> raw(static_cast<size_t>(rawSize));
    if (!f.read(reinterpret_cast<char*>(raw.data()), rawSize))
        return Result<BINFile>::Err("Read error: " + path.string());

    BINFile bin;
    bin.path_ = path;

    // 2. Auto-detect and decompress JDLZ.
    if (IsJDLZ(raw)) {
        LOG_INFO("BINFile::Open: JDLZ detected in '{}' ({} bytes compressed)",
                 path.filename().string(), raw.size());

        auto decompressed = LZCDecompressor::Decompress(raw);
        if (!decompressed)
            return Result<BINFile>::Err(
                "JDLZ decompression failed: " + path.string());

        bin.data_            = std::move(decompressed.value);
        bin.wasDecompressed_ = true;

        LOG_INFO("BINFile::Open: decompressed to {} bytes", bin.data_.size());
    } else {
        bin.data_            = std::move(raw);
        bin.wasDecompressed_ = false;

        LOG_INFO("BINFile::Open: '{}' ({} bytes, uncompressed)",
                 path.filename().string(), bin.data_.size());
    }

    return Result<BINFile>::Ok(std::move(bin));
}

// ─── ForEachTPK ──────────────────────────────────────────────────────────────
void BINFile::ForEachTPK(
    std::function<void(std::span<const uint8_t> payload, uint64_t absOff)> cb) const
{
    if (data_.empty() || !cb) return;

    // Walk the flat chunk list at the root level.  Each 0xB3300000 container
    // is handed directly to the caller (TPKBlockParser expects the container
    // payload, i.e. the bytes *after* the 8-byte ChunkHeader).
    const uint8_t* p   = data_.data();
    const uint8_t* end = p + data_.size();

    static constexpr size_t kHdrSize = 8; // sizeof(ChunkHeader)

    while (p + kHdrSize <= end) {
        uint32_t id, size;
        std::memcpy(&id,   p,     4);
        std::memcpy(&size, p + 4, 4);

        const uint8_t* payloadPtr = p + kHdrSize;

        if (payloadPtr + size > end) {
            LOG_WARN("BINFile::ForEachTPK: chunk 0x{:08X} overruns buffer, stopping", id);
            break;
        }

        if (id == kTPKContainerID) {
            const uint64_t absOff = static_cast<uint64_t>(payloadPtr - data_.data());
            cb(std::span<const uint8_t>(payloadPtr, size), absOff);
        }

        p = payloadPtr + size;
    }
}

// ─── Legacy API ──────────────────────────────────────────────────────────────

Result<void> BINFile::Load(const std::filesystem::path& path) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f)
        return Result<void>::Err("Cannot open file: " + path.string());

    const std::streamsize size = f.tellg();
    if (size <= 0)
        return Result<void>::Err("File is empty: " + path.string());

    f.seekg(0, std::ios::beg);
    data_.resize(static_cast<size_t>(size));

    if (!f.read(reinterpret_cast<char*>(data_.data()), size))
        return Result<void>::Err("Read error: " + path.string());

    path_ = path;
    wasDecompressed_ = false;
    LOG_INFO("BINFile: loaded {} ({} bytes)", path.filename().string(), data_.size());
    return Result<void>::Ok();
}

Result<void> BINFile::LoadSlice(const std::filesystem::path& path,
                                 uint64_t offset, uint64_t size) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f)
        return Result<void>::Err("Cannot open file: " + path.string());

    const uint64_t fileSize = static_cast<uint64_t>(f.tellg());
    if (offset + size > fileSize)
        return Result<void>::Err(
            "Slice out of range: " + path.string() +
            " offset=" + std::to_string(offset) +
            " size=" + std::to_string(size) +
            " fileSize=" + std::to_string(fileSize));

    f.seekg(static_cast<std::streamoff>(offset), std::ios::beg);
    data_.resize(static_cast<size_t>(size));

    if (!f.read(reinterpret_cast<char*>(data_.data()),
                static_cast<std::streamsize>(size)))
        return Result<void>::Err("Read error: " + path.string());

    path_ = path;
    wasDecompressed_ = false;
    LOG_DEBUG("BINFile: loaded slice of {} (offset={}, {} bytes)",
              path.filename().string(), offset, data_.size());
    return Result<void>::Ok();
}

} // namespace nfsmw
