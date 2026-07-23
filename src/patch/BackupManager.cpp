#include "patch/BackupManager.h"
#include "core/Logger.h"

#include <cstring>
#include <fstream>
#include <algorithm>

namespace nfsmw {

// ── Internal helpers ──────────────────────────────────────────────────────────

static std::string NormKey(const std::filesystem::path& p) {
    std::string s = p.string();
#ifdef _WIN32
    // Normalise to lowercase so A:\Foo and a:\foo compare equal.
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return (char)std::tolower(c); });
#endif
    return s;
}

// ── File-level backup ─────────────────────────────────────────────────────────

bool BackupManager::EnsureFileBak(const std::filesystem::path& src) {
    const std::string key = NormKey(src);

    {
        std::lock_guard lk(mtx_);
        if (bakedThisSession_.count(key)) return true; // already done this session
    }

    const auto bak = BakPath(src);

    // If the .bak already exists on disk (from a previous run), don't overwrite it —
    // it represents the true original. Just record that we're covered.
    if (std::filesystem::exists(bak)) {
        std::lock_guard lk(mtx_);
        bakedThisSession_.insert(key);
        LOG_DEBUG("BackupManager: .bak already exists for '{}'", src.filename().string());
        return true;
    }

    // Copy to a temp file first, then atomically rename into place. A plain
    // copy_file() straight to <file>.bak is not atomic: if the process is
    // killed, crashes, or loses power mid-copy, a truncated .bak can be left
    // on disk. Because EnsureFileBak only ever checks exists() (never size
    // or a checksum), that truncated file would be trusted as "the original"
    // forever after, with no way to recover the real original. Renaming only
    // after a successful full copy means a crash mid-backup leaves no .bak
    // at all (safe — we'll just retry next launch) instead of a corrupt one.
    const auto tmp = std::filesystem::path(bak.string() + ".tmp");

    std::error_code ec;
    std::filesystem::copy_file(src, tmp, std::filesystem::copy_options::overwrite_existing, ec);
    if (ec) {
        LOG_ERROR("BackupManager: failed to create .bak for '{}': {}",
                  src.string(), ec.message());
        std::filesystem::remove(tmp, ec);
        return false;
    }

    // Sanity-check the copy actually has the right size before trusting it.
    const uintmax_t srcSize = std::filesystem::file_size(src, ec);
    const uintmax_t tmpSize = ec ? 0 : std::filesystem::file_size(tmp, ec);
    if (ec || srcSize != tmpSize) {
        LOG_ERROR("BackupManager: .bak copy size mismatch for '{}' — discarding",
                  src.string());
        std::filesystem::remove(tmp, ec);
        return false;
    }

    std::filesystem::rename(tmp, bak, ec);
    if (ec) {
        LOG_ERROR("BackupManager: failed to finalize .bak for '{}': {}",
                  src.string(), ec.message());
        std::filesystem::remove(tmp, ec);
        return false;
    }

    {
        std::lock_guard lk(mtx_);
        bakedThisSession_.insert(key);
    }
    LOG_INFO("BackupManager: created '{}'", bak.filename().string());
    return true;
}

bool BackupManager::HasBak(const std::filesystem::path& src) const {
    const std::string key = NormKey(src);
    std::lock_guard lk(mtx_);
    return bakedThisSession_.count(key) > 0 || std::filesystem::exists(BakPath(src));
}

// ── Per-region manifest ───────────────────────────────────────────────────────
//
// Binary layout (appended records):
//   [uint16_t pathLen][pathLen bytes UTF-8 path]
//   [uint64_t fileOffset]
//   [uint32_t byteLen]
//   [byteLen bytes of original data]

void BackupManager::RecordRegion(const std::filesystem::path& path,
                                  uint64_t fileOffset,
                                  const std::vector<uint8_t>& originalBytes)
{
    if (originalBytes.empty()) return;

    const auto manifest = ManifestPath(path);
    std::ofstream f(manifest, std::ios::binary | std::ios::app);
    if (!f) {
        LOG_WARN("BackupManager: cannot open manifest '{}'", manifest.string());
        return;
    }

    const std::string ps = path.string();
    const uint16_t pathLen = (uint16_t)std::min(ps.size(), (size_t)0xFFFF);
    const uint32_t byteLen = (uint32_t)originalBytes.size();

    f.write(reinterpret_cast<const char*>(&pathLen), 2);
    f.write(ps.data(), pathLen);
    f.write(reinterpret_cast<const char*>(&fileOffset), 8);
    f.write(reinterpret_cast<const char*>(&byteLen), 4);
    f.write(reinterpret_cast<const char*>(originalBytes.data()), byteLen);

    // Flush and verify the write before closing so that a crash immediately
    // after RecordRegion does not leave a truncated manifest record.
    // An incomplete record on disk would cause RevertAll to under-revert.
    f.flush();
    if (!f) {
        LOG_ERROR("BackupManager: failed to write manifest record for '{}' "
                  "at offset 0x{:X} ({} bytes) — revert may be incomplete",
                  path.filename().string(), fileOffset, byteLen);
    }
}

int BackupManager::RevertAll(const std::filesystem::path& path) {
    const auto manifest = ManifestPath(path);

    std::ifstream f(manifest, std::ios::binary);
    if (!f) {
        LOG_WARN("BackupManager::RevertAll: no manifest for '{}'", path.filename().string());
        return -1;
    }

    // Open the target file for random-access writing.
    std::fstream tgt(path, std::ios::binary | std::ios::in | std::ios::out);
    if (!tgt) {
        LOG_ERROR("BackupManager::RevertAll: cannot open '{}' for writing", path.string());
        return -1;
    }

    // Read every record first, then apply them in REVERSE order. The manifest is
    // append-only and a region can be recorded more than once in a session (each
    // record holds the bytes that existed *before* that patch). Replaying oldest
    // → newest would finish on an intermediate (post-first-patch) state; replaying
    // newest → oldest ends on the genuinely-original bytes, so a doubly-patched
    // region reverts correctly.
    struct Region { uint64_t offset; std::vector<uint8_t> bytes; };
    std::vector<Region> records;
    for (;;) {
        uint16_t pathLen = 0;
        f.read(reinterpret_cast<char*>(&pathLen), 2);
        if (f.gcount() != 2 || pathLen == 0) break;

        std::string recordedPath(pathLen, '\0');
        f.read(recordedPath.data(), pathLen);
        if (f.gcount() != pathLen) break;

        uint64_t fileOffset = 0;
        f.read(reinterpret_cast<char*>(&fileOffset), 8);
        if (f.gcount() != 8) break;

        uint32_t byteLen = 0;
        f.read(reinterpret_cast<char*>(&byteLen), 4);
        if (f.gcount() != 4 || byteLen == 0 || byteLen > 64u * 1024 * 1024) break;

        std::vector<uint8_t> orig(byteLen);
        f.read(reinterpret_cast<char*>(orig.data()), byteLen);
        if ((uint32_t)f.gcount() != byteLen) break;

        records.push_back({ fileOffset, std::move(orig) });
    }

    int reverted = 0;
    for (auto it = records.rbegin(); it != records.rend(); ++it) {
        tgt.seekp(static_cast<std::streamoff>(it->offset), std::ios::beg);
        tgt.write(reinterpret_cast<const char*>(it->bytes.data()),
                  static_cast<std::streamsize>(it->bytes.size()));
        if (!tgt) {
            LOG_WARN("BackupManager::RevertAll: write failed at offset 0x{:X}", it->offset);
            tgt.clear();   // clear the failbit so later regions can still revert
            continue;
        }
        ++reverted;
    }

    tgt.close();
    f.close();

    if (reverted > 0) {
        std::error_code ec;
        std::filesystem::remove(manifest, ec);
        LOG_INFO("BackupManager::RevertAll: reverted {} region(s) in '{}'",
                 reverted, path.filename().string());
    }
    return reverted;
}

bool BackupManager::HasManifest(const std::filesystem::path& path) const {
    return std::filesystem::exists(ManifestPath(path));
}

} // namespace nfsmw
