#include "formats/VaultFile.h"
#include "core/StringHash.h"

#include <cstring>
#include <fstream>
#include <unordered_set>

namespace nfsmw {

namespace {

constexpr uint8_t kVPAK[4] = {'V','P','A','K'};

bool LoadBytes(const std::filesystem::path& p, std::vector<uint8_t>& out) {
    std::ifstream f(p, std::ios::binary | std::ios::ate);
    if (!f) return false;
    auto sz = static_cast<size_t>(f.tellg());
    f.seekg(0);
    out.resize(sz);
    return static_cast<bool>(f.read(reinterpret_cast<char*>(out.data()),
                                    static_cast<std::streamsize>(sz)));
}

bool IsPrintableTag(const uint8_t* p) {
    for (int i = 0; i < 4; ++i)
        if (p[i] < 0x20 || p[i] > 0x7E) return false;
    return true;
}

uint32_t ReadU32(const std::vector<uint8_t>& b, size_t off) {
    uint32_t v = 0;
    if (off + 4 <= b.size()) std::memcpy(&v, b.data() + off, 4);
    return v;
}

// The four block tags we have positively identified in retail vaults. We locate
// blocks by scanning for these signatures rather than chaining tag+size, because
// the mini-tables near the tail (NpeD/NrtS/NtaD) pack with sizes that don't
// chain cleanly — a signature scan is robust to that and is all the foundation
// reader needs to report structure.
const char* const kKnownTags[] = {"ErtS", "NpeD", "NrtS", "NtaD"};

} // namespace

Result<VaultFile> VaultParser::Load(const std::filesystem::path& path) {
    std::vector<uint8_t> buf;
    if (!LoadBytes(path, buf))
        return Result<VaultFile>::Err("Cannot read " + path.string());
    if (buf.size() < 0x80 || std::memcmp(buf.data(), kVPAK, 4) != 0)
        return Result<VaultFile>::Err("Not a VPAK vault: " + path.string());

    VaultFile vf;
    vf.path         = path;
    vf.fileSize     = buf.size();
    vf.version      = ReadU32(buf, 0x04);
    vf.headerSize   = ReadU32(buf, 0x08);
    vf.sectionCount = ReadU32(buf, 0x0C);

    // ── Locate tagged blocks by signature ────────────────────────────────────
    for (const char* tag : kKnownTags) {
        for (size_t i = 0; i + 8 <= buf.size(); ++i) {
            if (std::memcmp(buf.data() + i, tag, 4) != 0) continue;
            VaultBlock b;
            b.tag    = tag;
            b.size   = ReadU32(buf, i + 4);
            b.offset = i + 8;
            vf.blocks.push_back(std::move(b));
            break; // first occurrence is the block header
        }
    }

    // ── Recover the string table ─────────────────────────────────────────────
    // Every name/key/class/type in the vault is a null-terminated printable
    // run. Scan the whole buffer for runs of length >= 2 of printable ASCII
    // (tab/newline excluded) ending in NUL, deduplicating in encounter order.
    std::unordered_set<std::string> seen;
    seen.reserve(8192);
    std::string cur;
    cur.reserve(64);
    auto flush = [&]() {
        if (cur.size() >= 2 && seen.insert(cur).second)
            vf.strings.push_back(cur);
        cur.clear();
    };
    for (uint8_t c : buf) {
        if (c >= 0x20 && c <= 0x7E) {
            cur.push_back(static_cast<char>(c));
        } else {
            if (c == 0x00) flush();
            else cur.clear(); // run broken by a non-NUL control byte
        }
    }
    flush();

    // ── Enumerate NtaD records by marker ─────────────────────────────────────
    // Every record opens with kVaultRecordMagic; its length is the gap to the
    // next marker (or the block end). We only record verified boundaries here —
    // not a typed interpretation of each field.
    if (const VaultBlock* nta = vf.FindBlock("NtaD")) {
        const uint64_t start = nta->offset;
        uint64_t end = start + nta->size;
        if (end > buf.size()) end = buf.size();

        std::vector<uint64_t> marks;
        for (uint64_t i = start; i + 4 <= end; ++i) {
            uint32_t w;
            std::memcpy(&w, buf.data() + i, 4);
            if (w == kVaultRecordMagic) { marks.push_back(i); i += 3; }
        }
        vf.records.reserve(marks.size());
        for (size_t m = 0; m < marks.size(); ++m) {
            VaultRecord rec;
            rec.offset = marks[m];
            uint64_t next = (m + 1 < marks.size()) ? marks[m + 1] : end;
            rec.size  = static_cast<uint32_t>(next - marks[m]);
            rec.word0 = ReadU32(buf, static_cast<size_t>(marks[m] + 4));
            vf.records.push_back(rec);
        }
    }

    return Result<VaultFile>::Ok(std::move(vf));
}

size_t VaultParser::SeedHashResolver(const VaultFile& vf) {
    auto& res = HashResolver::Instance();
    for (const auto& s : vf.strings) {
        // The attribute system keys on Bin hashes; chunk/object names elsewhere
        // use Joaat. Register both so either resolves back to this name.
        res.RegisterHash(BinHash(s), s);
        res.RegisterHash(JoaatHash(s), s);
    }
    return vf.strings.size();
}

} // namespace nfsmw
