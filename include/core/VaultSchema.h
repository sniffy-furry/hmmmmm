#pragma once
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace nfsmw {

// ─────────────────────────────────────────────────────────────────────────────
// VaultSchema — the attribute-vault field-name schema the tuning panels need.
//
// NFSMW's car performance and pursuit tuning live in the global attribute vault
// (GLOBAL/attributes.bin, gameplay.bin), not in per-car files. Each record is
// keyed by a lookup2/0xABCDEF00 hash and carries inline {field_hash, value,
// type} triples (MWEncyclopedia C7.3/C7.6). The field NAMES were long thought to
// be compile-stripped into speed.exe — but the encyclopedia's RE pass recovered
// them by exact-hashing every string in the binaries (vault_class_layouts.json,
// "all entries are EXACT hash matches, not guesses").
//
// This module ships those verified (class, field-hash, field-name, type) tables
// so the Performance / Pursuit panels can resolve a raw attrib key to a real,
// human-readable field name — no hex, no guesswork. It is GL/VanGUI-free
// (nfsmw_core, C++20) and pairs with VaultFile.cpp (record locator) to decode
// and edit real values. Field hashes are the lookup2/0xABCDEF00 attrib keys.
// ─────────────────────────────────────────────────────────────────────────────

/// One tunable field of a vault class.
struct VaultField {
    uint32_t    hash;   ///< lookup2/0xABCDEF00 attrib key (AttribCollection::GetField)
    const char* name;   ///< verified field name (e.g. "TORQUE", "BustSpeed")
    const char* type;   ///< EA::Reflection type ("Float", "Int32", "AxlePair", …)
};

/// One vault class (the tuning "struct") with its named fields.
struct VaultClassSchema {
    const char*             group;      ///< "Performance" or "Pursuit"
    const char*             className;  ///< vault class name (e.g. "engine")
    uint32_t                classHash;  ///< lookup2 hash of the class name
    std::vector<VaultField> fields;     ///< named, verified fields (schema order)

    const VaultField* FindField(uint32_t fieldHash) const {
        for (const auto& f : fields) if (f.hash == fieldHash) return &f;
        return nullptr;
    }
};

/// The full catalogue of classes we carry a verified named schema for.
const std::vector<VaultClassSchema>& VaultSchemaCatalog();

/// Every class in `group` ("Performance" / "Pursuit"), in catalogue order.
std::vector<const VaultClassSchema*> VaultClassesInGroup(std::string_view group);

/// Look up a class by name (nullptr if not in the catalogue).
const VaultClassSchema* FindVaultClass(std::string_view className);

/// Resolve a field name from its attrib hash across every known class.
/// Returns nullptr if the hash is not in the catalogue.
const char* ResolveVaultFieldName(uint32_t fieldHash);

/// Find a field (name + type) by its attrib hash across every known class.
const VaultField* FindVaultField(uint32_t fieldHash);

/// True if the type name denotes a plain 32-bit scalar we can edit in place
/// (Float / Int32 / UInt32 / Int8 / …) rather than a nested reflection record.
bool IsScalarVaultType(std::string_view type);

} // namespace nfsmw
