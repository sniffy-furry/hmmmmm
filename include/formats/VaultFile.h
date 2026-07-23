#pragma once
#include "Common.h"
#include <filesystem>
#include <string>
#include <vector>

namespace nfsmw {

// ─────────────────────────────────────────────────────────────────────────────
// VaultFile — reader for the EA Attribulator "vault" container (GLOBAL/
// attributes.bin, FE_ATTRIB.bin, gameplay.bin …). These files, not any per-car
// CARINFO.BIN, are where NFSMW actually stores car performance and pursuit
// tuning (see the carinfo-format-mismatch note).
//
// Container layout (verified against retail attributes.bin):
//   +0x00  'VPAK' magic
//   +0x04  u32 version (1)
//   +0x08  u32 header size (0x40)
//   +0x0C  u32 section count (3)
//   +0x10  u32[]  section offset/size words
//   +0x80  tagged blocks: a 4-char tag + u32 size + payload. Observed tags:
//            'ErtS'  string table  — null-terminated key / class / type names
//            'NpeD'  dependency table
//            'NrtS'  (small) string/symbol table
//            'NtaD'  data table     — typed attribute records
//          The EA::Reflection / Attrib::* type names and the game class names
//          (CopCountRecord, LeaderSupport, GRace::CopDensity, pvehicle, …) all
//          live in the string region.
//
// SCOPE: this is the *foundation* reader. It recovers the container structure
// and the complete name/string table (verified, byte-exact) and seeds the
// global HashResolver from it. Decoding the typed NtaD records into a
// collection→field→value map (the layer that would feed real numbers into the
// Performance / Pursuit tabs) is the next phase and is intentionally NOT
// claimed here — see VaultFile.cpp's notes.
// ─────────────────────────────────────────────────────────────────────────────

/// One tagged block located inside the vault.
struct VaultBlock {
    std::string tag;        ///< 4-char block tag (e.g. "ErtS")
    uint64_t    offset = 0; ///< file offset of the payload (after tag+size)
    uint32_t    size   = 0; ///< declared payload size in bytes
};

/// Marker that opens every record inside the NtaD data block (verified vs
/// retail attributes.bin: 4732 occurrences, variable spacing).
constexpr uint32_t kVaultRecordMagic = 0xEFFECADDu;

/// One typed attribute record inside the NtaD block. The record opens with
/// kVaultRecordMagic; its length is the distance to the next marker. The
/// smallest form is magic + keyHash + value (12 bytes); larger forms carry
/// additional EA::Reflection-typed fields and class-reference hashes. The
/// semantic layer (grouping records into car collections and naming each field)
/// is not decoded yet — this exposes the verified record boundaries only.
struct VaultRecord {
    uint64_t offset = 0; ///< file offset of the marker
    uint32_t size   = 0; ///< record length in bytes, marker included
    uint32_t word0  = 0; ///< first u32 after the marker (key/type/value)
};

/// A parsed vault container.
struct VaultFile {
    std::filesystem::path     path;
    uint32_t                  version    = 0;
    uint32_t                  headerSize = 0;
    uint32_t                  sectionCount = 0;
    uint64_t                  fileSize   = 0;
    std::vector<VaultBlock>   blocks;   ///< tagged blocks located by signature
    std::vector<std::string>  strings;  ///< unique null-terminated names, in order
    std::vector<VaultRecord>  records;  ///< NtaD records, by marker (structural only)

    /// Find the first block with the given 4-char tag, or nullptr.
    const VaultBlock* FindBlock(std::string_view tag) const {
        for (const auto& b : blocks) if (b.tag == tag) return &b;
        return nullptr;
    }
};

class VaultParser {
public:
    /// Load and parse `path`. Fails only on unreadable file or missing 'VPAK'
    /// magic; an unrecognised block layout still returns Ok with whatever
    /// blocks/strings were recovered.
    static Result<VaultFile> Load(const std::filesystem::path& path);

    /// Register every recovered string into the global HashResolver under both
    /// its Joaat and Bin hash, so vault names resolve to readable text in the
    /// browser panels (roadmap P3-2). Returns the number of strings registered.
    static size_t SeedHashResolver(const VaultFile& vf);
};

} // namespace nfsmw
