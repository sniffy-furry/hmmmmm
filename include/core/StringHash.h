#pragma once
#include <string_view>
#include <string>
#include <cstdint>
#include <unordered_map>

namespace nfsmw {

/// Jenkins One-at-a-Time hash — the identifier algorithm used throughout EAGL.
inline uint32_t JoaatHash(std::string_view key) noexcept {
    uint32_t hash = 0;
    for (unsigned char c : key) {
        hash += c;
        hash += (hash << 10);
        hash ^= (hash >> 6);
    }
    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);
    return hash;
}

/// Case-insensitive variant (game uses lower-case keys internally).
inline uint32_t JoaatHashCI(std::string_view key) noexcept {
    uint32_t hash = 0;
    for (unsigned char c : key) {
        if (c >= 'A' && c <= 'Z') c += 32; // tolower
        hash += c;
        hash += (hash << 10);
        hash ^= (hash >> 6);
    }
    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);
    return hash;
}

/// Bin (sum) hash, used by some attribute systems.
inline uint32_t BinHash(std::string_view key) noexcept {
    uint32_t hash = 0xFFFFFFFFu;
    for (unsigned char c : key)
        hash = c + 33 * hash;
    return hash;
}

/// Bob Jenkins `lookup2` (1996) with EA's seed 0xABCDEF00 — the engine's
/// reflection/attribute key hash (verified against the inline constant in
/// speed.exe: lookup2("default") == 0xEEC2271A). Used by the vault attribute
/// system and the NIS CARP event-sequence script for symbolic keys.
inline uint32_t Lookup2Hash(std::string_view key,
                            uint32_t initval = 0xABCDEF00u) noexcept {
    auto mix = [](uint32_t& a, uint32_t& b, uint32_t& c) noexcept {
        a -= b; a -= c; a ^= (c >> 13);
        b -= c; b -= a; b ^= (a << 8);
        c -= a; c -= b; c ^= (b >> 13);
        a -= b; a -= c; a ^= (c >> 12);
        b -= c; b -= a; b ^= (a << 16);
        c -= a; c -= b; c ^= (b >> 5);
        a -= b; a -= c; a ^= (c >> 3);
        b -= c; b -= a; b ^= (a << 10);
        c -= a; c -= b; c ^= (b >> 15);
    };
    const auto*    k   = reinterpret_cast<const unsigned char*>(key.data());
    const uint32_t len = static_cast<uint32_t>(key.size());
    uint32_t a = 0x9E3779B9u, b = 0x9E3779B9u, c = initval;
    uint32_t i = 0, ln = len;
    while (ln >= 12) {
        a += static_cast<uint32_t>(k[i])   | (static_cast<uint32_t>(k[i + 1]) << 8)
           | (static_cast<uint32_t>(k[i + 2])  << 16) | (static_cast<uint32_t>(k[i + 3])  << 24);
        b += static_cast<uint32_t>(k[i + 4]) | (static_cast<uint32_t>(k[i + 5]) << 8)
           | (static_cast<uint32_t>(k[i + 6])  << 16) | (static_cast<uint32_t>(k[i + 7])  << 24);
        c += static_cast<uint32_t>(k[i + 8]) | (static_cast<uint32_t>(k[i + 9]) << 8)
           | (static_cast<uint32_t>(k[i + 10]) << 16) | (static_cast<uint32_t>(k[i + 11]) << 24);
        mix(a, b, c);
        i += 12; ln -= 12;
    }
    c += len;
    switch (ln) {
        case 11: c += static_cast<uint32_t>(k[i + 10]) << 24; [[fallthrough]];
        case 10: c += static_cast<uint32_t>(k[i + 9])  << 16; [[fallthrough]];
        case 9:  c += static_cast<uint32_t>(k[i + 8])  << 8;  [[fallthrough]];
        case 8:  b += static_cast<uint32_t>(k[i + 7])  << 24; [[fallthrough]];
        case 7:  b += static_cast<uint32_t>(k[i + 6])  << 16; [[fallthrough]];
        case 6:  b += static_cast<uint32_t>(k[i + 5])  << 8;  [[fallthrough]];
        case 5:  b += static_cast<uint32_t>(k[i + 4]);        [[fallthrough]];
        case 4:  a += static_cast<uint32_t>(k[i + 3])  << 24; [[fallthrough]];
        case 3:  a += static_cast<uint32_t>(k[i + 2])  << 16; [[fallthrough]];
        case 2:  a += static_cast<uint32_t>(k[i + 1])  << 8;  [[fallthrough]];
        case 1:  a += static_cast<uint32_t>(k[i]);            break;
        default: break; // ln == 0
    }
    mix(a, b, c);
    return c;
}

/// Global reverse-lookup table: hash → original string, populated as names
/// are encountered while parsing (object names, texture names, ...).
class HashResolver {
public:
    static HashResolver& Instance();

    void Register(std::string_view name);
    void RegisterHash(uint32_t hash, std::string_view name);

    /// Returns the known name, or a hex placeholder like "0x0743CFB1".
    /// Issue #34: returns a reference (no per-call std::string copy) for
    /// registered names; unregistered hashes format into a thread-local
    /// buffer reused across calls on the same thread.
    const std::string& Resolve(uint32_t hash) const;

    /// Returns nullptr if unknown.
    const std::string* TryResolve(uint32_t hash) const;

    size_t Count() const { return table_.size(); }

private:
    std::unordered_map<uint32_t, std::string> table_;
};

} // namespace nfsmw
