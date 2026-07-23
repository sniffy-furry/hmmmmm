#include "core/LZCDecompressor.h"
#include "core/Logger.h"
#include <cstring>
#include <algorithm>
#include <unordered_map>

namespace nfsmw {

// ─── Magic constants ──────────────────────────────────────────────────────────
static constexpr uint32_t kMagicJDLZ = 0x5A4C444A; // 'JDLZ'
static constexpr uint32_t kMagicHUFF = 0x46465548; // 'HUFF' (unsupported)
static constexpr uint32_t kMagicRAWW = 0x57574152; // 'RAWW' (stored)
static constexpr uint32_t kMagicCOMP = 0x504D4F43; // 'COMP' (unsupported)

bool LZCDecompressor::IsCompressed(std::span<const uint8_t> data) {
    if (data.size() < 4) return false;
    uint32_t magic = 0;
    std::memcpy(&magic, data.data(), 4);
    return magic == kMagicJDLZ || magic == kMagicHUFF ||
           magic == kMagicRAWW || magic == kMagicCOMP;
}

Result<std::vector<uint8_t>> LZCDecompressor::Decompress(std::span<const uint8_t> data) {
    if (data.size() < 16)
        return Result<std::vector<uint8_t>>::Err("LZC: buffer too small for header");

    uint32_t magic = 0;
    std::memcpy(&magic, data.data(), 4);

    switch (magic) {
        case kMagicJDLZ:
            return DecompressJDLZ(data);
        case kMagicRAWW: {
            // Stored, not compressed: payload follows the 16-byte header.
            std::vector<uint8_t> out(data.begin() + 16, data.end());
            return Result<std::vector<uint8_t>>::Ok(std::move(out));
        }
        case kMagicHUFF:
            return Result<std::vector<uint8_t>>::Err("LZC: HUFF compression not supported");
        case kMagicCOMP:
            return Result<std::vector<uint8_t>>::Err("LZC: COMP compression not supported");
        default:
            return Result<std::vector<uint8_t>>::Err("LZC: unknown magic");
    }
}

// ─── JDLZ ────────────────────────────────────────────────────────────────────
// Two interleaved flag streams (flags1/flags2), refilled a byte at a time with
// a 0x100 sentinel. flags1 selects literal vs back-reference; flags2 selects
// the back-reference encoding:
//   near: length = ((b0 & 0xF0) << 4 | b1) + 3,  distance = (b0 & 0x0F) + 1
//   far:  length = (b0 & 0x1F) + 3,              distance = ((b0 & 0xE0) << 3 | b1) + 17
Result<std::vector<uint8_t>> LZCDecompressor::DecompressJDLZ(std::span<const uint8_t> data) {
    uint32_t outLen = 0;
    std::memcpy(&outLen, data.data() + 8, 4);

    // Guard against absurd headers (corrupt files).
    if (outLen > (1u << 30))
        return Result<std::vector<uint8_t>>::Err("JDLZ: implausible decompressed size");

    std::vector<uint8_t> out(outLen);

    size_t pos    = 16;
    size_t outPos = 0;
    uint32_t flags1 = 1, flags2 = 1;

    const size_t n = data.size();

    while (pos < n && outPos < outLen) {
        if (flags1 == 1) {
            if (pos >= n) break;
            flags1 = data[pos++] | 0x100u;
        }
        if (flags2 == 1) {
            if (pos >= n) break;
            flags2 = data[pos++] | 0x100u;
        }

        if (flags1 & 1) {
            if (pos + 1 >= n) break;
            size_t length, dist;
            if (flags2 & 1) { // near reference
                length = ((static_cast<size_t>(data[pos]) & 0xF0) << 4 | data[pos + 1]) + 3;
                dist   = (static_cast<size_t>(data[pos]) & 0x0F) + 1;
            } else {          // far reference
                length = (static_cast<size_t>(data[pos]) & 0x1F) + 3;
                dist   = ((static_cast<size_t>(data[pos]) & 0xE0) << 3 | data[pos + 1]) + 17;
            }
            pos += 2;

            if (dist > outPos)
                return Result<std::vector<uint8_t>>::Err("JDLZ: back-reference before start of output");

            for (size_t i = 0; i < length && outPos < outLen; ++i, ++outPos)
                out[outPos] = out[outPos - dist];

            flags2 >>= 1;
        } else {
            if (pos >= n) break;
            if (outPos < outLen)
                out[outPos++] = data[pos++];
        }
        flags1 >>= 1;
    }

    if (outPos != outLen) {
        LOG_WARN("JDLZ: expected {} bytes, produced {}", outLen, outPos);
        return Result<std::vector<uint8_t>>::Err("JDLZ: truncated stream");
    }

    LOG_DEBUG("JDLZ: decompressed {} -> {} bytes", data.size(), out.size());
    return Result<std::vector<uint8_t>>::Ok(std::move(out));
}

// ─── JDLZ compression ─────────────────────────────────────────────────────────
// Inverse of DecompressJDLZ. A greedy hash-chain LZ77 matcher that emits the two
// interleaved flag streams (flags1 = literal/reference, flags2 = near/far) in the
// EXACT order the decoder refills them, so the output is byte-exact round-trippable
// and the game loads it. Window/length limits are dictated by the decoder's
// encodings (see header):
//   near: dist 1..16,    len 3..4098   →  b0 = ((len-3)>>8 & 0xF)<<4 | (dist-1),  b1 = (len-3)&0xFF
//   far : dist 17..2064, len 3..34     →  b0 = ((dist-17)>>8 & 0x7)<<5 | (len-3), b1 = (dist-17)&0xFF
// Matches farther than 2064 bytes cannot be encoded and fall back to literals.
namespace {

constexpr uint32_t kNearMaxLen  = 0xFFFu + 3;  // 4098
constexpr uint32_t kFarMaxLen   = 0x1Fu  + 3;  // 34
constexpr uint32_t kFarMaxDist  = 0x7FFu + 17; // 2064
constexpr int      kMaxChain    = 128;         // hash-chain search depth (ratio/speed knob)

struct JdlzTok {
    bool     isRef = false;
    uint8_t  lit   = 0;
    bool     near_ = false;
    uint8_t  b0 = 0, b1 = 0;
    uint32_t len = 1;  // bytes of input consumed (1 for a literal)
};

inline JdlzTok EncodeRef(uint32_t dist, uint32_t length) {
    JdlzTok t; t.isRef = true;
    if (dist <= 16) {
        uint32_t L  = std::min(length, kNearMaxLen);
        uint32_t ln = L - 3;
        t.near_ = true;
        t.b0 = static_cast<uint8_t>((((ln >> 8) & 0xF) << 4) | (dist - 1));
        t.b1 = static_cast<uint8_t>(ln & 0xFF);
        t.len = L;
    } else {
        uint32_t L  = std::min(length, kFarMaxLen);
        uint32_t ln = L - 3, dd = dist - 17;
        t.near_ = false;
        t.b0 = static_cast<uint8_t>((((dd >> 8) & 0x7) << 5) | (ln & 0x1F));
        t.b1 = static_cast<uint8_t>(dd & 0xFF);
        t.len = L;
    }
    return t;
}

} // namespace

std::vector<uint8_t> LZCDecompressor::CompressJDLZ(std::span<const uint8_t> d) {
    const size_t n = d.size();

    // ── greedy parse into a token list ──
    std::vector<JdlzTok> toks;
    toks.reserve(n / 2 + 16);
    std::unordered_map<uint32_t, long> head;
    std::vector<long> prev(n, -1);
    auto h3 = [&](size_t p) -> uint32_t {
        return (static_cast<uint32_t>(d[p]) << 16) |
               (static_cast<uint32_t>(d[p + 1]) << 8) | d[p + 2];
    };

    size_t i = 0;
    while (i < n) {
        uint32_t bestLen = 0, bestDist = 0;
        if (i + 3 <= n) {
            auto it = head.find(h3(i));
            long j  = (it == head.end()) ? -1 : it->second;
            int  chain = 0;
            long lo = static_cast<long>(i > kFarMaxDist ? i - kFarMaxDist : 0);
            while (j >= lo && chain < kMaxChain) {
                uint32_t maxl = static_cast<uint32_t>(std::min<size_t>(kNearMaxLen, n - i));
                uint32_t l = 0;
                while (l < maxl && d[static_cast<size_t>(j) + l] == d[i + l]) ++l;
                uint32_t dist = static_cast<uint32_t>(i - static_cast<size_t>(j));
                uint32_t enc  = (dist <= 16)         ? std::min(l, kNearMaxLen)
                              : (dist <= kFarMaxDist) ? std::min(l, kFarMaxLen)
                                                      : 0;
                if (enc > bestLen) { bestLen = enc; bestDist = dist; }
                j = prev[static_cast<size_t>(j)];
                ++chain;
            }
        }

        uint32_t adv;
        if (bestLen >= 3) { JdlzTok t = EncodeRef(bestDist, bestLen); adv = t.len; toks.push_back(t); }
        else              { JdlzTok t; t.isRef = false; t.lit = d[i]; t.len = 1; adv = 1; toks.push_back(t); }

        // insert hash entries for every position we advanced over
        size_t end = (n >= 2) ? std::min(i + adv, n - 2) : 0;
        for (size_t k = i; k < end; ++k) {
            uint32_t key = h3(k);
            auto it = head.find(key);
            prev[k] = (it == head.end()) ? -1 : it->second;
            head[key] = static_cast<long>(k);
        }
        i += adv;
    }

    // ── serialise: 16-byte header + interleaved flag/data stream ──
    std::vector<const JdlzTok*> refs;
    refs.reserve(toks.size());
    for (const auto& t : toks) if (t.isRef) refs.push_back(&t);

    std::vector<uint8_t> out = { 'J', 'D', 'L', 'Z', 0x02, 0x10, 0x00, 0x00 };
    uint32_t outLen = static_cast<uint32_t>(n);
    for (int s = 0; s < 4; ++s) out.push_back(static_cast<uint8_t>((outLen >> (8 * s)) & 0xFF));
    size_t csPos = out.size();
    out.insert(out.end(), { 0, 0, 0, 0 });  // compressed size (patched at end)

    auto f1Byte = [&](size_t base) -> uint8_t {
        uint8_t b = 0;
        for (int k = 0; k < 8; ++k) { size_t idx = base + k; if (idx < toks.size() && toks[idx].isRef) b |= (1u << k); }
        return b;
    };
    auto f2Byte = [&](size_t base) -> uint8_t {
        uint8_t b = 0;
        for (int k = 0; k < 8; ++k) { size_t idx = base + k; if (idx < refs.size() && refs[idx]->near_) b |= (1u << k); }
        return b;
    };

    uint32_t f1 = 1, f2 = 1;
    size_t   ti = 0, rcount = 0;
    while (ti < toks.size()) {
        if (f1 == 1) { uint8_t b = f1Byte(ti);     out.push_back(b); f1 = b | 0x100u; }
        if (f2 == 1) { uint8_t b = f2Byte(rcount); out.push_back(b); f2 = b | 0x100u; }
        const JdlzTok& t = toks[ti];
        if (t.isRef) { out.push_back(t.b0); out.push_back(t.b1); f2 >>= 1; ++rcount; }
        else         { out.push_back(t.lit); }
        f1 >>= 1;
        ++ti;
    }

    uint32_t cs = static_cast<uint32_t>(out.size());
    for (int s = 0; s < 4; ++s) out[csPos + s] = static_cast<uint8_t>((cs >> (8 * s)) & 0xFF);
    return out;
}

Result<std::vector<uint8_t>> LZCDecompressor::Compress(std::span<const uint8_t> data) {
    if (data.empty())
        return Result<std::vector<uint8_t>>::Err("JDLZ: cannot compress empty buffer");
    auto out = CompressJDLZ(data);
    LOG_DEBUG("JDLZ: compressed {} -> {} bytes", data.size(), out.size());
    return Result<std::vector<uint8_t>>::Ok(std::move(out));
}

} // namespace nfsmw
