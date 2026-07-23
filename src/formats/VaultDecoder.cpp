#include "formats/VaultDecoder.h"
#include "core/StringHash.h"
#include "core/VaultSchema.h"
#include "core/Logger.h"

#include <cstring>
#include <fstream>

namespace nfsmw {

namespace {

bool LoadBytes(const std::filesystem::path& p, std::vector<uint8_t>& out) {
    std::ifstream f(p, std::ios::binary | std::ios::ate);
    if (!f) return false;
    auto sz = static_cast<size_t>(f.tellg());
    f.seekg(0);
    out.resize(sz);
    return static_cast<bool>(f.read(reinterpret_cast<char*>(out.data()),
                                    static_cast<std::streamsize>(sz)));
}

uint32_t RU32(const std::vector<uint8_t>& b, size_t off) {
    uint32_t v = 0;
    if (off + 4 <= b.size()) std::memcpy(&v, b.data() + off, 4);
    return v;
}

float RF32(const std::vector<uint8_t>& b, size_t off) {
    float v = 0;
    if (off + 4 <= b.size()) std::memcpy(&v, b.data() + off, 4);
    return v;
}

// An EA::Reflection value type-id sits in the 0x0020xxxx band (C7.6). This is
// what separates a real {field, value, type} triple from the trailing
// {key, class, parent} hash slots — do not blind-float-scan.
bool IsValueTypeId(uint32_t t) { return (t & 0xFFFF0000u) == 0x00200000u; }

} // namespace

Result<std::vector<VaultDecodedRecord>> VaultDecoder::Decode(
    const std::filesystem::path& path) {

    auto vfr = VaultParser::Load(path);
    if (!vfr) return Result<std::vector<VaultDecodedRecord>>::Err(vfr.error);
    const VaultFile& vf = vfr.value;
    VaultParser::SeedHashResolver(vf);   // so key/field hashes resolve to names

    std::vector<uint8_t> buf;
    if (!LoadBytes(path, buf))
        return Result<std::vector<VaultDecodedRecord>>::Err(
            "Cannot re-read vault bytes: " + path.string());

    auto& res = HashResolver::Instance();
    std::vector<VaultDecodedRecord> out;

    for (const auto& rec : vf.records) {
        VaultDecodedRecord dr;
        dr.offset  = rec.offset;
        dr.size    = rec.size;
        dr.keyHash = rec.word0;                 // first hash after the marker
        if (const std::string* kn = res.TryResolve(dr.keyHash)) dr.keyName = *kn;

        // Scan the record body (after marker+key) for inline triples. A triple
        // is 12 bytes: {field u32, value f32, type u32} with type in 0x0020xxxx.
        const uint64_t begin = rec.offset + 8;             // skip marker + key
        const uint64_t end   = rec.offset + rec.size;
        for (uint64_t o = begin; o + 12 <= end; ) {
            const uint32_t typeId = RU32(buf, static_cast<size_t>(o + 8));
            if (IsValueTypeId(typeId)) {
                VaultTriple t;
                t.fieldHash   = RU32(buf, static_cast<size_t>(o));
                t.value       = RF32(buf, static_cast<size_t>(o + 4));
                t.typeId      = typeId;
                t.valueOffset = o + 4;
                if (const char* fn = ResolveVaultFieldName(t.fieldHash))
                    t.fieldName = fn;
                else if (const std::string* rn = res.TryResolve(t.fieldHash))
                    t.fieldName = *rn;
                dr.triples.push_back(std::move(t));
                o += 12;                          // consume the whole triple
            } else {
                o += 4;                           // step one word and retry
            }
        }

        if (!dr.triples.empty()) out.push_back(std::move(dr));
    }

    LOG_INFO("VaultDecoder: {} records with inline values in {} ({} total records)",
             out.size(), path.filename().string(), vf.records.size());
    return Result<std::vector<VaultDecodedRecord>>::Ok(std::move(out));
}

Result<void> VaultDecoder::PatchFloat(const std::filesystem::path& path,
                                      uint64_t valueOffset, float newValue,
                                      BackupManager& bm, const float* expectedOld) {
    std::vector<uint8_t> buf;
    if (!LoadBytes(path, buf))
        return Result<void>::Err("Cannot read vault: " + path.string());
    if (valueOffset + 4 > buf.size())
        return Result<void>::Err("Value offset past end of vault");

    float cur = 0;
    std::memcpy(&cur, buf.data() + valueOffset, 4);
    if (expectedOld) {
        // Bit-exact compare (NaN-safe, and avoids float-eq surprises).
        uint32_t a, b;
        std::memcpy(&a, &cur, 4);
        std::memcpy(&b, expectedOld, 4);
        if (a != b)
            return Result<void>::Err("On-disk value changed; reload before saving");
    }

    std::memcpy(buf.data() + valueOffset, &newValue, 4);

    bm.EnsureFileBak(path);
    const auto tmp = std::filesystem::path(path.string() + ".tmp");
    {
        std::ofstream o(tmp, std::ios::binary | std::ios::trunc);
        if (!o) return Result<void>::Err("Cannot write temp file");
        o.write(reinterpret_cast<const char*>(buf.data()),
                static_cast<std::streamsize>(buf.size()));
    }
    std::error_code ec;
    std::filesystem::rename(tmp, path, ec);
    if (ec) {
        std::filesystem::remove(tmp, ec);
        return Result<void>::Err("Could not replace vault file: " + ec.message());
    }

    LOG_INFO("VaultDecoder: patched value at 0x{:X} -> {} in {}",
             valueOffset, newValue, path.filename().string());
    return Result<void>::Ok();
}

Result<int> VaultDecoder::PatchFloats(const std::filesystem::path& path,
                                      const std::vector<Edit>& edits,
                                      BackupManager& bm) {
    if (edits.empty()) return Result<int>::Ok(0);

    std::vector<uint8_t> buf;
    if (!LoadBytes(path, buf))
        return Result<int>::Err("Cannot read vault: " + path.string());

    int changed = 0;
    for (const auto& e : edits) {
        if (e.valueOffset + 4 > buf.size()) continue;
        float cur = 0;
        std::memcpy(&cur, buf.data() + e.valueOffset, 4);
        uint32_t a, b;
        std::memcpy(&a, &cur, 4);
        std::memcpy(&b, &e.newValue, 4);
        if (a == b) continue;
        std::memcpy(buf.data() + e.valueOffset, &e.newValue, 4);
        ++changed;
    }
    if (changed == 0) return Result<int>::Ok(0);

    bm.EnsureFileBak(path);
    const auto tmp = std::filesystem::path(path.string() + ".tmp");
    {
        std::ofstream o(tmp, std::ios::binary | std::ios::trunc);
        if (!o) return Result<int>::Err("Cannot write temp file");
        o.write(reinterpret_cast<const char*>(buf.data()),
                static_cast<std::streamsize>(buf.size()));
    }
    std::error_code ec;
    std::filesystem::rename(tmp, path, ec);
    if (ec) {
        std::filesystem::remove(tmp, ec);
        return Result<int>::Err("Could not replace vault file: " + ec.message());
    }
    LOG_INFO("VaultDecoder: batch-patched {} values in {}",
             changed, path.filename().string());
    return Result<int>::Ok(changed);
}

} // namespace nfsmw
