#pragma once
#include "Common.h"
#include "formats/VaultFile.h"
#include "patch/BackupManager.h"
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace nfsmw {

// ─────────────────────────────────────────────────────────────────────────────
// VaultDecoder — reads the actual numbers out of an attribute vault.
//
// Building on VaultFile (which locates records by the 0xEFFECADD marker) and
// VaultSchema (which names the fields), this decodes each record's inline
// value triples exactly as MWEncyclopedia C7.6 documents them:
//
//     { field_hash : u32 }   the lookup2/0xABCDEF00 attrib key of the field
//     { value      : f32 }   the actual number (IEEE-754 little-endian)
//     { type_id    : u32 }   an EA::Reflection type tag in the 0x0020xxxx range
//
// The type-tag range is what distinguishes a real triple from the trailing
// {key, class, parent} hash slots — never blind-float-scan (the Float type-id
// 0x3c16ec5e reinterpreted as a float is 0.009212, and appears everywhere).
//
// Writing a value that already carries an inline triple is a fixed-length,
// in-place f32 patch — the safe edit. Adding an override where a key currently
// inherits `default` would change record length (repack); that is intentionally
// NOT done here.
//
// GL/VanGUI-free (nfsmw_core). Values decoded here still warrant an in-game /
// hex-diff check before shipping a mod, per the encyclopedia's guidance.
// ─────────────────────────────────────────────────────────────────────────────

/// One inline {field, value, type} triple located inside a record.
struct VaultTriple {
    uint32_t fieldHash     = 0;  ///< lookup2 attrib key
    float    value         = 0;  ///< decoded IEEE-754 value
    uint32_t typeId        = 0;  ///< EA::Reflection type tag (0x0020xxxx)
    uint64_t valueOffset   = 0;  ///< absolute file offset of the 4 value bytes
    std::string fieldName;       ///< resolved via VaultSchema (empty if unknown)
};

/// One decoded record: its key plus every inline value it overrides.
struct VaultDecodedRecord {
    uint64_t                 offset = 0;   ///< file offset of the 0xEFFECADD marker
    uint32_t                 size   = 0;   ///< record length (to next marker)
    uint32_t                 keyHash = 0;  ///< first hash after the marker
    std::string              keyName;      ///< resolved key (e.g. "default", car)
    std::vector<VaultTriple> triples;      ///< inline value overrides
};

class VaultDecoder {
public:
    /// Load `path` (a VPAK vault) and decode every record's inline value
    /// triples. Only records that carry ≥1 triple are returned.
    static Result<std::vector<VaultDecodedRecord>> Decode(
        const std::filesystem::path& path);

    /// In-place patch a single decoded value: overwrite the 4 f32 bytes at
    /// `valueOffset` with `newValue`. `expectedOld` (if given) is verified
    /// against the on-disk bytes first. Backed up via `bm`. No length change.
    /// NOTE: `valueOffset` must be an offset into the ON-DISK (uncompressed)
    /// vault — attribute vaults ship uncompressed, so this holds for them.
    static Result<void> PatchFloat(const std::filesystem::path& path,
                                   uint64_t valueOffset,
                                   float newValue,
                                   BackupManager& bm,
                                   const float* expectedOld = nullptr);

    /// One value edit for a batch save.
    struct Edit { uint64_t valueOffset; float newValue; };

    /// Apply many value edits in a single read/modify/write (one backup, one
    /// rewrite). Returns the number of values actually changed.
    static Result<int> PatchFloats(const std::filesystem::path& path,
                                   const std::vector<Edit>& edits,
                                   BackupManager& bm);
};

} // namespace nfsmw
