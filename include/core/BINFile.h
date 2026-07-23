#pragma once
#include "Common.h"
#include "core/ChunkReader.h"
#include <filesystem>
#include <functional>
#include <span>
#include <vector>

namespace nfsmw {

/// Represents a loaded .BIN or .BUN file (raw uncompressed chunk data).
///
/// Phase 1 additions:
///   - Open()           : factory that auto-detects JDLZ and decompresses.
///   - WasDecompressed(): true when the file required JDLZ decompression.
///   - ForEachTPK()     : walks the chunk tree and calls back for every
///                        0xB3300000 TPKContainer payload.
class BINFile {
public:
    BINFile() = default;

    // ── Phase 1 API ──────────────────────────────────────────────────────────

    /// Factory: open `path` from disk, auto-detect JDLZ compression, and
    /// decompress if necessary. Returns a ready-to-use BINFile or an error.
    static Result<BINFile> Open(const std::filesystem::path& path);

    /// True when the file was JDLZ-compressed and has been decompressed in
    /// memory. In-place patching is not possible for compressed sources.
    bool WasDecompressed() const { return wasDecompressed_; }

    /// Walk the chunk tree and invoke `cb` for every 0xB3300000 TPKContainer
    /// payload. `absOff` passed to `cb` is the absolute byte offset of the
    /// payload (after the 8-byte ChunkHeader) within the *decompressed* buffer
    /// (use for bookkeeping / rebuild; for compressed sources it is logical
    /// only and in-place patching is gated by `compressedSource`).
    void ForEachTPK(std::function<void(std::span<const uint8_t> payload,
                                       uint64_t absOff)> cb) const;

    // ── Legacy API (unchanged) ───────────────────────────────────────────────

    /// Load from disk. Returns error string if failed.
    Result<void> Load(const std::filesystem::path& path);

    /// Load a byte range [offset, offset+size) from a file on disk.
    Result<void> LoadSlice(const std::filesystem::path& path,
                           uint64_t offset, uint64_t size);

    /// Load from an already-decompressed byte buffer.
    void LoadFromBuffer(std::vector<uint8_t> buffer) {
        data_             = std::move(buffer);
        wasDecompressed_  = false;
    }

    /// Access the raw buffer.
    std::span<const uint8_t> Data()         const { return data_; }
    std::vector<uint8_t>&    MutableData()        { return data_; }

    /// Is the buffer non-empty?
    bool Valid() const { return !data_.empty(); }

    /// File path (if loaded from disk).
    const std::filesystem::path& Path() const { return path_; }

    /// Size in bytes.
    size_t Size() const { return data_.size(); }

private:
    std::vector<uint8_t>   data_;
    std::filesystem::path  path_;
    bool                   wasDecompressed_ = false;

    // TPK container chunk ID (0xB3300000).
    static constexpr uint32_t kTPKContainerID = 0xB3300000u;
};

} // namespace nfsmw
