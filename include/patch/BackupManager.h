#pragma once
// ─── patch/BackupManager.h ────────────────────────────────────────────────────
// Reversible write layer for BUN/BIN patching.
//
// Two complementary safety mechanisms, mirroring mwml_1::FilePatcher:
//
//   1. File-level backup (.bak):
//      Before the first write to a file this session, copy it to <file>.bak.
//      Subsequent writes to the same file skip the copy (idempotent). The .bak
//      is a complete original and can be used to restore a file even after
//      catastrophic corruption.
//
//   2. Per-region manifest:
//      Each overwritten byte range is recorded as:
//        [u16 pathLen][path UTF-8][u64 fileOffset][u32 byteLen][original bytes]
//      The manifest file lives next to the source BUN/BIN as <file>.manifest.
//      RevertAll() reads the manifest, seeks to each recorded offset, writes the
//      original bytes back, then deletes the manifest.
//
// Usage:
//   BackupManager bm;
//   bm.EnsureFileBak(path);          // once, before first write
//   bm.RecordRegion(path, off, origBytes);   // for every overwritten slice
//   ...
//   bm.RevertAll(path);              // undo all per-region patches
// ─────────────────────────────────────────────────────────────────────────────
#include "Common.h"
#include <filesystem>
#include <mutex>
#include <string>
#include <unordered_set>

namespace nfsmw {

class BackupManager {
public:
    BackupManager()  = default;
    ~BackupManager() = default;

    // Non-copyable (mutex, set of paths)
    BackupManager(const BackupManager&)            = delete;
    BackupManager& operator=(const BackupManager&) = delete;

    // ── File-level backup ────────────────────────────────────────────────────

    /// Copy `src` to `src.bak` if no .bak exists yet for this path this
    /// session (and no .bak file already exists on disk).
    /// Returns true on success or if backup was already done/exists.
    bool EnsureFileBak(const std::filesystem::path& src);

    /// True if a .bak has been created (or already existed) for `src`.
    bool HasBak(const std::filesystem::path& src) const;

    // ── Per-region manifest ──────────────────────────────────────────────────

    /// Append one region record to the manifest for `path`.
    /// `originalBytes` must be the bytes that were in the file BEFORE the
    /// write — call this before writing.
    void RecordRegion(const std::filesystem::path& path,
                      uint64_t fileOffset,
                      const std::vector<uint8_t>& originalBytes);

    /// Seek to every recorded region in `path` and restore the original bytes.
    /// Deletes the manifest file on success. Returns number of regions reverted.
    /// Returns -1 on manifest open error.
    int RevertAll(const std::filesystem::path& path);

    /// True if a manifest exists for `path` (i.e. patches have been applied).
    bool HasManifest(const std::filesystem::path& path) const;

    // ── Convenience ─────────────────────────────────────────────────────────

    /// Returns the .bak path for a given source file.
    static std::filesystem::path BakPath(const std::filesystem::path& src) {
        return std::filesystem::path(src.string() + ".bak");
    }

    /// Returns the .manifest path for a given source file.
    static std::filesystem::path ManifestPath(const std::filesystem::path& src) {
        return std::filesystem::path(src.string() + ".manifest");
    }

private:
    mutable std::mutex           mtx_;
    std::unordered_set<std::string> bakedThisSession_; ///< normalised lowercase paths
};

} // namespace nfsmw
