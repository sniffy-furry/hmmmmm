// ─── formats/ABKFile.cpp ─────────────────────────────────────────────────────
// EA ABK (ABKC/BNKl) sound-effect bank parser and EA-XA R2 decoder.
//
// ABKC container layout (all little-endian):
//   [0x00] char[4]    "ABKC"
//   [0x04] uint8[16]  version/flags (skipped)
//   [0x14] uint32     totalFileSize   (patch on repack)
//   [0x18] uint8[8]   (skipped)
//   [0x20] uint32     sfxBankOffset   (offset of embedded BNKl bank)
//   [0x24] uint32     sfxBankSize     (patch on repack)
//   [0x28] uint8[8]   (skipped)
//   [0x30] uint32     funcFixupOffset (patch on repack)
//   [0x34] uint32     staticDataFixupOffset (patch on repack)
//   [0x38] uint32     interfaceOffset (patch on repack + fixup table)
//
// BNKl header (inside ABKC at sfxBankOffset):
//   [+0x00] char[4]     "BNKl"
//   [+0x04] int16       unk
//   [+0x06] int16       numSounds
//   [+0x08] int32       bnkSize
//   [+0x0C] int32       (zero)
//   [+0x10] int32       (zero)
//   [+0x14] int32[N]    PT offsets: each value is relative to the position of that entry
//                       (i.e. ptAbs = bnkBase + 0x14 + i*4 + rel[i])
//
// PT chunk (per sound, big-endian tag-length-value):
//   magic: "PT" (0x5450 LE, but in file as 50 54 = PT)
//   Tags (all big-endian):
//     0x84  SampleRate
//     0x85  NumSamples
//     0x86  LoopOffset
//     0x87  LoopLength
//     0x88  DataStart1   (audio data offset relative to BNKl base)
//     0x80  Split
//     0x82  Channels
//     0x83  Compression  (0=EA-XA R2, else see codecs enum)
//     0xA0  SplitCompression (8=PCM S16LE)
//     0xFF  End
//
// EA-XA R2 block format (per channel, mono only in practice):
//   Block header byte:
//     bits [7:4]  predictor index into ea_adpcm_table (row)
//     bits [3:0]  shift exponent (used directly as left-shift amount)
//   Followed by 14 bytes of nibble data -> 28 samples:
//     Each byte: high nibble = sample N, low nibble = sample N+1
//     next = (nibble_signed << shift) + (cur * coeff1 + prev * coeff2) >> 8
//   Special: first byte == 0xEE -> uncompressed block:
//     2 BE int16 (current, previous), then 28 BE int16 samples
//   Block size: 15 bytes (compressed) or 61 bytes (0xEE uncompressed)
//   Each block = 28 samples.
//
// Reference: CrabJournal/ABK_Insert (Program.cs), FFmpeg adpcm.c AV_CODEC_ID_ADPCM_EA_R2
// ─────────────────────────────────────────────────────────────────────────────
#include "formats/ABKFile.h"
#include "core/Logger.h"

#include <fstream>
#include <cstring>
#include <algorithm>
#include <cmath>

namespace nfsmw {

// ─────────────────────────────────────────────────────────────────────────────
// Internal helpers
// ─────────────────────────────────────────────────────────────────────────────
namespace {

Result<std::vector<uint8_t>> ReadFileBytes(const std::filesystem::path& p) {
    std::ifstream f(p, std::ios::binary | std::ios::ate);
    if (!f) return Result<std::vector<uint8_t>>::Err("Cannot open: " + p.string());
    auto sz = static_cast<size_t>(f.tellg());
    f.seekg(0);
    std::vector<uint8_t> buf(sz);
    if (!f.read(reinterpret_cast<char*>(buf.data()), static_cast<std::streamsize>(sz)))
        return Result<std::vector<uint8_t>>::Err("Read failed: " + p.string());
    return Result<std::vector<uint8_t>>::Ok(std::move(buf));
}

inline uint16_t ReadU16LE(const uint8_t* p) {
    return static_cast<uint16_t>(p[0]) | static_cast<uint16_t>(p[1]) << 8;
}
inline int16_t ReadS16BE(const uint8_t* p) {
    return static_cast<int16_t>(static_cast<uint16_t>(p[0]) << 8 | p[1]);
}
inline uint32_t ReadU32LE(const uint8_t* p) {
    return static_cast<uint32_t>(p[0])
         | static_cast<uint32_t>(p[1]) << 8
         | static_cast<uint32_t>(p[2]) << 16
         | static_cast<uint32_t>(p[3]) << 24;
}

inline int16_t ClampS16(int32_t v) {
    if (v >  32767) return  32767;
    if (v < -32768) return -32768;
    return static_cast<int16_t>(v);
}

} // anonymous namespace

// ─────────────────────────────────────────────────────────────────────────────
// DecodeEAXA — EA-XA v2 decoder (NFSMW ABK "eaxa" PCM-blocks revision)
// ─────────────────────────────────────────────────────────────────────────────
// Coefficient table from vgmstream coding/ea_xa_decoder.c (EA_XA_TABLE[20]).
// coef1 = EA_XA_TABLE[(hdr >> 4) + 0], coef2 = EA_XA_TABLE[(hdr >> 4) + 4].
// shift = (hdr & 0x0F) + 8, applied as (nibble << 28) >> shift; then the whole
// (scaled + coef1*hist1 + coef2*hist2) >> 8.  v2 does NOT add 128 (unlike v1).
// NFSMW engine/SFX banks are split-mono; we decode a single mono stream.
static const int32_t EA_XA_TABLE[20] = {
    0,  240,  460,  392,
    0,    0, -208, -220,
    0,    1,    3,    4,
    7,    8,   10,   11,
    0,   -1,   -3,   -4
};

std::vector<int16_t> DecodeEAXA(const uint8_t* src, size_t srcSize,
                                  uint32_t /*channels*/, uint32_t numSamples) {
    std::vector<int16_t> out;
    if (srcSize == 0) return out;
    if (numSamples) out.reserve(numSamples);

    const size_t cap = numSamples ? numSamples : SIZE_MAX;
    int32_t hist1 = 0, hist2 = 0;
    size_t pos = 0;

    while (out.size() < cap && pos < srcSize) {
        uint8_t hdr = src[pos];
        if (hdr == 0xEE) {
            // PCM block: 1 header + 2 BE s16 hist + 28 BE s16 samples (61 bytes)
            if (pos + 61 > srcSize) break;
            hist1 = ReadS16BE(src + pos + 1);
            hist2 = ReadS16BE(src + pos + 3);
            for (int i = 0; i < 28 && out.size() < cap; ++i)
                out.push_back(ReadS16BE(src + pos + 5 + i * 2));
            pos += 61;
        } else {
            // ADPCM block: 1 header + 14 nibble bytes (15 bytes) -> 28 samples
            if (pos + 15 > srcSize) break;
            int32_t coef1 = EA_XA_TABLE[(hdr >> 4) + 0];
            int32_t coef2 = EA_XA_TABLE[(hdr >> 4) + 4];
            int shift = (hdr & 0x0F) + 8;
            for (int i = 0; i < 28 && out.size() < cap; ++i) {
                uint8_t byte = src[pos + 1 + i / 2];
                int nibShift = (!(i & 1)) ? 4 : 0;          // high nibble first
                uint8_t nib  = (byte >> nibShift) & 0x0F;
                int32_t s = static_cast<int32_t>(
                                static_cast<uint32_t>(nib) << 28) >> shift;
                s = (s + coef1 * hist1 + coef2 * hist2) >> 8;
                s = ClampS16(s);
                out.push_back(static_cast<int16_t>(s));
                hist2 = hist1;
                hist1 = s;
            }
            pos += 15;
        }
    }
    return out;
}

// ─────────────────────────────────────────────────────────────────────────────
// EncodeEAXA — EA-XA v2 encoder (inverse of DecodeEAXA)
// ─────────────────────────────────────────────────────────────────────────────
// Per 28-sample frame, brute-force predictor (0..3) × shift (8..23), choosing
// the combination with least reconstruction error. Reconstruction is bit-exact
// with the decoder, so history carries forward correctly. All-ADPCM output
// (no 0xEE PCM frames): 15 bytes / 28 samples.
std::vector<uint8_t> EncodeEAXA(const int16_t* pcm, size_t numSamples,
                                 const std::function<void(float)>& onProgress) {
    std::vector<uint8_t> out;
    if (numSamples == 0) return out;
    const size_t totalFrames = (numSamples + 27) / 28;
    out.reserve(totalFrames * 15);

    int32_t hist1 = 0, hist2 = 0;
    size_t frameIdx = 0;
    for (size_t f = 0; f < numSamples; f += 28) {
        const size_t cnt = std::min<size_t>(28, numSamples - f);

        int     bestP = 0, bestS = 8;
        double  bestErr = 1e30;
        int32_t bestH1 = hist1, bestH2 = hist2;
        uint8_t bestNibs[28] = {0};

        for (int p = 0; p < 4; ++p) {
            const int32_t coef1 = EA_XA_TABLE[p + 0];
            const int32_t coef2 = EA_XA_TABLE[p + 4];
            for (int shift = 8; shift <= 23; ++shift) {
                int32_t t1 = hist1, t2 = hist2;
                double  err = 0;
                uint8_t nibs[28];
                for (size_t i = 0; i < cnt; ++i) {
                    const int32_t target = pcm[f + i];
                    const int32_t pred   = coef1 * t1 + coef2 * t2;
                    const double  df     = static_cast<double>(target) * 256.0 - pred;
                    double nf            = df / static_cast<double>(1u << (28 - shift));
                    int nib              = static_cast<int>(std::lround(nf));
                    if (nib < -8) nib = -8;
                    if (nib >  7) nib =  7;
                    const int32_t d   = static_cast<int32_t>(
                                            static_cast<uint32_t>(nib & 0xF) << 28) >> shift;
                    const int32_t dec = ClampS16((d + pred) >> 8);
                    const double  e   = static_cast<double>(dec) - target;
                    err += e * e;
                    nibs[i] = static_cast<uint8_t>(nib & 0xF);
                    t2 = t1; t1 = dec;
                }
                if (err < bestErr) {
                    bestErr = err; bestP = p; bestS = shift;
                    bestH1 = t1; bestH2 = t2;
                    std::memcpy(bestNibs, nibs, sizeof(nibs));
                }
            }
        }

        out.push_back(static_cast<uint8_t>((bestP << 4) | ((bestS - 8) & 0x0F)));
        uint8_t bytes[14] = {0};
        for (size_t i = 0; i < 28; ++i) {
            const uint8_t nib = (i < cnt) ? bestNibs[i] : 0;
            if (!(i & 1)) bytes[i / 2] |= static_cast<uint8_t>(nib << 4);
            else          bytes[i / 2] |= nib;
        }
        out.insert(out.end(), bytes, bytes + 14);
        hist1 = bestH1; hist2 = bestH2;

        ++frameIdx;
        if (onProgress && (frameIdx % 32 == 0 || frameIdx == totalFrames))
            onProgress(static_cast<float>(frameIdx) / static_cast<float>(totalFrames));
    }
    return out;
}

// ─────────────────────────────────────────────────────────────────────────────
// BNKl / PT chunk parser
// ─────────────────────────────────────────────────────────────────────────────
namespace {

// EA codec2 identifiers (from vgmstream ea_schl.c).
namespace EACodec2 {
    constexpr int NONE      = -1;
    constexpr int S16LE_INT = 0x00;
    constexpr int S16BE_INT = 0x01;
    constexpr int EAXA_INT  = 0x03;  // EA-XA stereo (interleaved)
    constexpr int S16BE     = 0x07;
    constexpr int S16LE     = 0x08;
    constexpr int EAXA      = 0x0A;  // EA-XA split mono (NFSMW default)
}

struct PTData {
    uint32_t sampleRate  = 0;
    uint32_t numSamples  = 0;
    uint32_t loopStart   = 0;
    uint32_t loopEnd     = 0;
    uint32_t dataStart   = 0;  // relative to BNKl base (tag 0x88)
    uint16_t channels    = 0;
    uint16_t platform    = 0;  // 0 = PC
    int      version     = -1; // tag 0x80
    int      codec1      = -1; // tag 0x83
    int      codec2      = -1; // tag 0xA0
    bool     valid       = false;
};

// read_patch — EA PT/GSTR value reader (vgmstream ea_schl.c::read_patch).
// First byte is a length: 0xFF = 32-bit-sized custom blob (skip), >4 = raw
// blob of that many bytes (skip), 0..4 = big-endian integer of that width.
uint32_t ReadPatch(const uint8_t* data, size_t dataSize, size_t& pos) {
    if (pos >= dataSize) return 0;
    uint8_t byteCount = data[pos++];
    if (byteCount == 0xFF) {
        if (pos + 4 > dataSize) { pos = dataSize; return 0; }
        uint32_t sz = static_cast<uint32_t>(ReadU32LE(data + pos)); // size is BE in spec but only skipped
        // spec reads BE size; use BE to be faithful
        sz = (static_cast<uint32_t>(data[pos]) << 24) | (data[pos+1] << 16)
           | (data[pos+2] << 8) | data[pos+3];
        pos += 4 + sz;
        return 0;
    }
    if (byteCount > 4) { pos += byteCount; return 0; }
    uint32_t result = 0;
    for (; byteCount > 0 && pos < dataSize; --byteCount)
        result = (result << 8) + data[pos++];
    return result;
}

// Parse a single PT chunk (EA variable header). Returns false if magic missing.
bool ParsePT(const uint8_t* data, size_t dataSize, size_t ptAbs, PTData& out) {
    if (ptAbs + 4 > dataSize) return false;
    // PT magic: "PT\0\0" (0x50 0x54 ..)
    if (data[ptAbs] != 0x50 || data[ptAbs + 1] != 0x54) return false;

    out = PTData{};
    out.platform = ReadU16LE(data + ptAbs + 2);   // platform id (PC=0)
    size_t pos = ptAbs + 4;
    bool end = false;

    while (!end && pos < dataSize) {
        uint8_t tag = data[pos++];
        switch (tag) {
            case 0xFF: end = true; out.valid = true; break;  // header end
            case 0xFE: end = true; out.valid = true; break;  // next subsection
            case 0xFC: case 0xFD: break;                     // markers, no value
            case 0x80: out.version    = static_cast<int>(ReadPatch(data, dataSize, pos)); break;
            case 0x82: out.channels   = static_cast<uint16_t>(ReadPatch(data, dataSize, pos)); break;
            case 0x83: out.codec1     = static_cast<int>(ReadPatch(data, dataSize, pos)); break;
            case 0xA0: out.codec2     = static_cast<int>(ReadPatch(data, dataSize, pos)); break;
            case 0x84: out.sampleRate = ReadPatch(data, dataSize, pos); break;
            case 0x85: out.numSamples = ReadPatch(data, dataSize, pos); break;
            case 0x86: out.loopStart  = ReadPatch(data, dataSize, pos); break;
            case 0x87: out.loopEnd    = ReadPatch(data, dataSize, pos) + 1; break;
            case 0x88: out.dataStart  = ReadPatch(data, dataSize, pos); break;
            default:   ReadPatch(data, dataSize, pos); break; // skip any other tag's value
        }
    }
    return out.valid;
}

// Resolve EA codec1/platform defaults to a codec2 value (subset of ea_schl.c).
int ResolveCodec2(const PTData& pt) {
    int codec2 = pt.codec2;
    if (codec2 == EACodec2::NONE && pt.codec1 >= 0) {
        switch (pt.codec1) {
            case 0x00: codec2 = (pt.platform == 0) ? EACodec2::S16LE_INT : EACodec2::S16LE; break; // PCM
            case 0x07: codec2 = (pt.platform == 0 || pt.platform == 3)
                                ? EACodec2::EAXA_INT : EACodec2::EAXA; break;                       // EAXA
            default:   codec2 = EACodec2::EAXA; break;
        }
    }
    if (codec2 == EACodec2::NONE) codec2 = EACodec2::EAXA; // PC default
    return codec2;
}

// Parse all sounds from a BNKl block.
// `bnkBase` is absolute offset of BNKl in the full file buffer.
// `nameTable` is an optional list of null-terminated names read from the ABKC
// interface section ([0x38]).  When non-empty the i-th name is assigned to the
// i-th valid PT entry; if it runs out the fallback bankName+"_"+index is used.
bool ParseBNKl(const uint8_t* data, size_t dataSize, size_t bnkBase,
               const std::string& bankName,
               const std::vector<std::string>& nameTable,
               ABKFile& bank) {
    if (bnkBase + 0x18 > dataSize) return false;
    if (std::memcmp(data + bnkBase, "BNKl", 4) != 0) return false;

    uint8_t  bnkVersion = data[bnkBase + 0x04];
    uint16_t numSounds  = ReadU16LE(data + bnkBase + 0x06);
    if (numSounds == 0 || numSounds > 4096) return false;

    // PT pointer table: BNKl+0x14 for v4/v5, BNKl+0x0C for v2 (vgmstream parse_bnk_header).
    const size_t tableBase = bnkBase + (bnkVersion >= 4 ? 0x14u : 0x0Cu);

    bank.name = bankName;
    bank.entries.reserve(numSounds);

    int realIndex = 0;
    for (uint16_t i = 0; i < numSounds; ++i) {
        const size_t entryPos = tableBase + static_cast<size_t>(i) * 4;
        if (entryPos + 4 > dataSize) break;

        // Pointer is relative to its own position; zero entries are dummies, skip.
        uint32_t rel = ReadU32LE(data + entryPos);
        if (rel == 0) continue;
        size_t ptAbs = entryPos + rel;

        PTData pt;
        if (ptAbs >= dataSize || !ParsePT(data, dataSize, ptAbs, pt)) {
            LOG_WARN("[ABK] Sound {} in '{}': PT parse failed at offset {}", i, bankName, ptAbs);
            continue;
        }

        ABKEntry entry;
        entry.id         = static_cast<uint32_t>(realIndex);
        // Prefer the null-terminated name from the ABKC interface section
        // (parsed and passed in via nameTable).  Fall back to the generic
        // bankName_index label when the table is absent or too short.
        if (realIndex < (int)nameTable.size() && !nameTable[realIndex].empty())
            entry.name = nameTable[realIndex];
        else
            entry.name = bankName + "_" + std::to_string(realIndex);
        entry.dataOffset = static_cast<uint32_t>(bnkBase) + pt.dataStart; // tag 0x88 is BNKl-relative
        entry.numSamples = pt.numSamples;
        entry.channels   = (pt.channels >= 1 && pt.channels <= 2) ? pt.channels : 1;
        entry.sampleRate = pt.sampleRate ? pt.sampleRate : 22050;  // PC default
        bool loop        = (pt.loopEnd > 0);
        entry.loopStart  = loop ? pt.loopStart : 0xFFFFFFFFu;
        entry.loopLength = loop && pt.loopEnd > pt.loopStart ? (pt.loopEnd - pt.loopStart) : 0;

        const int codec2 = ResolveCodec2(pt);
        if (codec2 == EACodec2::S16LE_INT || codec2 == EACodec2::S16LE ||
            codec2 == EACodec2::S16BE_INT || codec2 == EACodec2::S16BE) {
            entry.codecTag = ABKCodec::PCM;
            entry.dataSize = pt.numSamples * entry.channels * 2;
        } else {
            entry.codecTag = ABKCodec::EAXA;
            // Walk EA-XA v2 blocks to measure exact byte length (28 samples/block).
            uint32_t samplesLeft = pt.numSamples;
            size_t   pos         = entry.dataOffset;
            const size_t start   = pos;
            while (samplesLeft > 0 && pos < dataSize) {
                uint8_t b = data[pos];
                size_t blockSize = (b == 0xEE) ? 61u : 15u;
                if (pos + blockSize > dataSize) { pos = dataSize; break; }
                pos += blockSize;
                samplesLeft = (samplesLeft >= 28) ? samplesLeft - 28 : 0;
            }
            entry.dataSize = static_cast<uint32_t>(pos - start);
        }

        bank.entries.push_back(std::move(entry));
        ++realIndex;
    }

    return !bank.entries.empty();
}

} // anonymous namespace

// ─────────────────────────────────────────────────────────────────────────────
// ABKParser::Load
// ─────────────────────────────────────────────────────────────────────────────
Result<ABKFile> ABKParser::Load(const std::filesystem::path& path) {
    auto readRes = ReadFileBytes(path);
    if (!readRes)
        return Result<ABKFile>::Err(readRes.error);
    const auto& data = readRes.value;
    const size_t fileSize = data.size();

    if (fileSize < 0x40)
        return Result<ABKFile>::Err("ABK file too small");

    // Magic: "ABKC"
    if (std::memcmp(data.data(), "ABKC", 4) != 0)
        return Result<ABKFile>::Err(
            "Not an ABKC file (got: " +
            std::string(reinterpret_cast<const char*>(data.data()), 4) + ")");

    // Header fields (see layout above)
    // [0x14] totalFileSize
    // [0x20] sfxBankOffset  (absolute byte offset of BNKl within file)
    // [0x24] sfxBankSize
    const uint32_t sfxBankOffset = ReadU32LE(data.data() + 0x20);
    const uint32_t sfxBankSize   = ReadU32LE(data.data() + 0x24);
    (void)sfxBankSize;

    if (sfxBankOffset >= fileSize)
        return Result<ABKFile>::Err("ABK sfxBankOffset out of range");

    // ── Parse interface section: null-terminated sound names ─────────────────
    // ABKC [0x38] holds a file-relative offset to the interface section, which
    // begins with an array of numSounds null-terminated ASCII name strings.
    // These are the "real" names used by the engine (e.g. "TIRE_SQUEAL_01").
    // We read them here and pass to ParseBNKl so ABKEntry::name is populated
    // correctly instead of the generic bankName_N fallback.
    std::vector<std::string> nameTable;
    {
        const uint32_t interfaceOffset = ReadU32LE(data.data() + 0x38);
        // interfaceOffset of 0 means the section is absent (stripped banks).
        if (interfaceOffset != 0 && interfaceOffset < fileSize) {
            const uint8_t* p   = data.data() + interfaceOffset;
            const uint8_t* end = data.data() + fileSize;
            // Walk null-terminated strings until we hit the BNKl region or
            // the end of file.  Guard against corrupt offsets by capping at
            // 4 096 entries (no real bank has that many sounds).
            while (p < end && nameTable.size() < 4096) {
                // An empty string (double-NUL / alignment padding) marks the
                // end of the name list.
                if (*p == '\0') break;
                const uint8_t* nameStart = p;
                while (p < end && *p != '\0') ++p;
                nameTable.emplace_back(reinterpret_cast<const char*>(nameStart),
                                       static_cast<size_t>(p - nameStart));
                ++p; // skip NUL terminator
            }
        }
    }

    ABKFile bank;
    bank.path = path;

    if (!ParseBNKl(data.data(), fileSize, sfxBankOffset,
                   path.stem().string(), nameTable, bank))
        return Result<ABKFile>::Err("ABK: BNKl parse failed — no valid PT chunks found");

    LOG_INFO("[ABK] Loaded '{}' — {} sounds", bank.name, bank.entries.size());
    return Result<ABKFile>::Ok(std::move(bank));
}

// ─────────────────────────────────────────────────────────────────────────────
// ABKParser::ReadPayload
// ─────────────────────────────────────────────────────────────────────────────
Result<std::vector<uint8_t>> ABKParser::ReadPayload(const ABKFile& bank,
                                                      const ABKEntry& entry) {
    std::ifstream f(bank.path, std::ios::binary);
    if (!f) return Result<std::vector<uint8_t>>::Err("Cannot open ABK: " + bank.path.string());

    f.seekg(static_cast<std::streamoff>(entry.dataOffset));
    if (!f) return Result<std::vector<uint8_t>>::Err("Seek failed in ABK");

    std::vector<uint8_t> buf(entry.dataSize);
    if (!f.read(reinterpret_cast<char*>(buf.data()),
                static_cast<std::streamsize>(entry.dataSize)))
        return Result<std::vector<uint8_t>>::Err("Read payload failed");

    return Result<std::vector<uint8_t>>::Ok(std::move(buf));
}

// ─────────────────────────────────────────────────────────────────────────────
// ABKParser::DecodePCM
// ─────────────────────────────────────────────────────────────────────────────
Result<std::vector<int16_t>> ABKParser::DecodePCM(const std::vector<uint8_t>& raw,
                                                    const ABKEntry& entry,
                                                    uint32_t& outRate,
                                                    uint32_t& outChannels) {
    outRate     = entry.sampleRate;
    outChannels = entry.channels;

    if (raw.empty())
        return Result<std::vector<int16_t>>::Err("Empty payload");

    switch (entry.codecTag) {

        case ABKCodec::PCM: {
            const size_t samples = raw.size() / 2;
            std::vector<int16_t> out(samples);
            std::memcpy(out.data(), raw.data(), samples * 2);
            return Result<std::vector<int16_t>>::Ok(std::move(out));
        }

        case ABKCodec::EAXA: {
            auto out = DecodeEAXA(raw.data(), raw.size(), entry.channels, entry.numSamples);
            if (out.empty())
                return Result<std::vector<int16_t>>::Err("EA-XA: no samples decoded");
            return Result<std::vector<int16_t>>::Ok(std::move(out));
        }

        default:
            return Result<std::vector<int16_t>>::Err(
                "Unknown ABK codec tag: 0x" + std::to_string(entry.codecTag));
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// ABKParser::ReplacePayload
// ─────────────────────────────────────────────────────────────────────────────
// Replaces one sound's data in the ABKC file, updating all size/offset fields.
// Strategy follows ABK_Insert (CrabJournal): patch totalFileSize, sfxBankSize,
// funcFixupOffset, staticDataFixupOffset, interfaceOffset in ABKC header, plus
// the BNKl bnkSize field and PT DataStart1 within BNKl.
//
// Only supports same-size replacement for now (simplest/safest).
Result<void> ABKParser::ReplacePayload(ABKFile& bank,
                                        ABKEntry& entry,
                                        const std::vector<uint8_t>& newRaw,
                                        const std::filesystem::path& /*backupDir*/) {
    if (newRaw.size() != entry.dataSize)
        return Result<void>::Err(
            "ReplacePayload: new size (" + std::to_string(newRaw.size()) +
            ") != original (" + std::to_string(entry.dataSize) +
            "). Variable-size replacement not yet implemented.");

    auto readRes = ReadFileBytes(bank.path);
    if (!readRes) return Result<void>::Err(readRes.error);
    auto& data = readRes.value;

    const size_t end = static_cast<size_t>(entry.dataOffset) + entry.dataSize;
    if (end > data.size())
        return Result<void>::Err("Entry data extends beyond file");

    std::memcpy(data.data() + entry.dataOffset, newRaw.data(), newRaw.size());

    // Same-size: no header patching needed (file size unchanged).
    // (On variable-size, we'd patch 0x14, 0x24, 0x30, 0x34, 0x38, BNKl+0x08,
    //  PT DataStart1, and the interfaceOffset fixup table — see ABK_Insert Main().)

    std::ofstream f(bank.path, std::ios::binary | std::ios::trunc);
    if (!f) return Result<void>::Err("Cannot open ABK for writing: " + bank.path.string());
    if (!f.write(reinterpret_cast<const char*>(data.data()),
                 static_cast<std::streamsize>(data.size())))
        return Result<void>::Err("Write failed: " + bank.path.string());

    LOG_INFO("[ABK] Replaced payload for '{}' in {}", entry.name, bank.name);
    return Result<void>::Ok();
}

// ─────────────────────────────────────────────────────────────────────────────
// ABKParser::ReplaceFromPCM — import mono PCM (game-safe in-place EA-XA re-encode)
//
// Phase 5: the PT chunk's SampleRate tag is never patched on replace, so a
// source WAV recorded at a different rate than `entry.sampleRate` would
// previously play back at the wrong pitch/speed in-game with no warning.
// We now resample to the entry's declared rate first, fit/pad the result to
// the entry's original sample count as before, and report what happened
// (and whether the re-encoded payload decodes back to something close to
// what went in) so the panel can surface a single consistent toast.
// ─────────────────────────────────────────────────────────────────────────────
Result<AudioCodecs::ReplaceReport> ABKParser::ReplaceFromPCM(
        ABKFile& bank,
        ABKEntry& entry,
        const std::vector<int16_t>& pcmMono,
        uint32_t srcRate,
        const std::filesystem::path& backupDir,
        const AudioCodecs::ProgressFn& onProgress) {
    if (entry.codecTag != ABKCodec::EAXA)
        return Result<AudioCodecs::ReplaceReport>::Err(
            "Import currently supports EA-XA sound entries only");
    if (entry.numSamples == 0 || entry.dataSize == 0)
        return Result<AudioCodecs::ReplaceReport>::Err(
            "Entry has no sample/data length to fit to");
    if (pcmMono.empty())
        return Result<AudioCodecs::ReplaceReport>::Err("Empty PCM input");

    AudioCodecs::ReplaceReport report;
    const auto rateCheck = AudioCodecs::CheckRates(srcRate, 1, entry.sampleRate, 1);
    report.srcRate     = srcRate;
    report.dstRate     = entry.sampleRate;
    report.srcChannels = 1;
    report.dstChannels = 1;

    // ── Resample to the entry's fixed rate if the source WAV doesn't match ──
    const std::vector<int16_t>& srcPCM = pcmMono;
    std::vector<int16_t> resampled;
    const std::vector<int16_t>* working = &srcPCM;
    if (rateCheck.rateMismatch) {
        resampled = AudioCodecs::Resample(srcPCM, 1, srcRate, entry.sampleRate);
        working   = &resampled;
        report.resampled = true;
    }

    // Fit the (possibly resampled) PCM to the entry's original sample count
    // (truncate / pad with silence) so the re-encoded payload fits the
    // existing slot exactly.
    std::vector<int16_t> fit(entry.numSamples, 0);
    const size_t copyN = std::min<size_t>(working->size(), entry.numSamples);
    std::copy_n(working->begin(), copyN, fit.begin());

    auto encRes = AudioCodecs::Encode(AudioCodecs::Codec::EA_XA_V2, fit.data(), fit.size(),
                                       1, onProgress);
    if (!encRes) return Result<AudioCodecs::ReplaceReport>::Err(encRes.error);
    std::vector<uint8_t> enc = std::move(encRes.value);

    if (enc.size() > entry.dataSize)
        return Result<AudioCodecs::ReplaceReport>::Err(
            "Re-encoded payload (" + std::to_string(enc.size()) +
            " B) exceeds original slot (" + std::to_string(entry.dataSize) +
            " B); variable-size repack not supported");

    report.verify = AudioCodecs::VerifyRoundTrip(AudioCodecs::Codec::EA_XA_V2, enc, 1,
                                                  entry.numSamples, fit, entry.sampleRate);

    // Zero-pad the tail to the exact original byte size (decoder stops at
    // numSamples, so trailing bytes are never read).
    enc.resize(entry.dataSize, 0);

    auto repRes = ReplacePayload(bank, entry, enc, backupDir);
    if (!repRes) return Result<AudioCodecs::ReplaceReport>::Err(repRes.error);
    return Result<AudioCodecs::ReplaceReport>::Ok(report);
}

} // namespace nfsmw
