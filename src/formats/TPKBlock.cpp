#include "formats/TPKBlock.h"
#include "core/Logger.h"
#include "core/StringHash.h"
#include "core/LZCDecompressor.h"
#include <cstring>
#include <cmath>
#include <fstream>
#include <algorithm>

namespace nfsmw {

namespace {

constexpr size_t kEntrySize    = 124;
constexpr size_t kCompInfoSize = 32;

TexFormat FourCCToFormat(const uint8_t* fcc) {
    if (std::memcmp(fcc, "DXT1", 4) == 0) return TexFormat::DXT1;
    if (std::memcmp(fcc, "DXT3", 4) == 0) return TexFormat::DXT3;
    if (std::memcmp(fcc, "DXT5", 4) == 0) return TexFormat::DXT5;
    uint32_t v;
    std::memcpy(&v, fcc, 4);
    if (v == 0x15) return TexFormat::ARGB32; // D3DFMT_A8R8G8B8
    if (v == 0x29) return TexFormat::PAL8;   // D3DFMT_P8
    return TexFormat::Unknown;
}

void IterateChunks(std::span<const uint8_t> data,
                   const std::function<void(uint32_t, std::span<const uint8_t>)>& fn) {
    size_t off = 0;
    while (off + sizeof(ChunkHeader) <= data.size()) {
        ChunkHeader hdr{};
        std::memcpy(&hdr, data.data() + off, sizeof(hdr));
        if (off + sizeof(hdr) + hdr.size > data.size()) break;
        fn(hdr.id, data.subspan(off + sizeof(hdr), hdr.size));
        off += sizeof(hdr) + hdr.size;
    }
}

} // namespace

Result<TPKBlock> TPKBlockParser::Parse(std::span<const uint8_t> payload,
                                       uint64_t absOffset) {
    TPKBlock block;
    block.chunkOffset = absOffset ? absOffset - 8 : 0;   // back to chunk header
    block.chunkSize   = (uint32_t)payload.size();

    std::span<const uint8_t> entryData, compData, rawData, compEntryData;
    uint64_t rawDataRel = 0;   // payload-relative offset of post-pad pixel data
    bool isCompressedVariant = false;

    IterateChunks(payload, [&](uint32_t id, std::span<const uint8_t> p) {
        if (id == ChunkID::TPKInfo) {
            IterateChunks(p, [&](uint32_t id2, std::span<const uint8_t> p2) {
                switch (id2) {
                    case ChunkID::TPKInfoHeader:
                        if (p2.size() >= 100) {
                            const char* nm = reinterpret_cast<const char*>(p2.data() + 4);
                            block.name.assign(nm, strnlen(nm, 28));
                            const char* pt = reinterpret_cast<const char*>(p2.data() + 32);
                            block.path.assign(pt, strnlen(pt, 64));
                            std::memcpy(&block.fileHash, p2.data() + 96, 4);
                        }
                        break;
                    case ChunkID::TPKEntries:     entryData     = p2; break;
                    case ChunkID::TPKCompInfo:    compData      = p2; break;
                    case ChunkID::TPKCompEntries: compEntryData = p2;
                                                   isCompressedVariant = true;
                                                   break;
                    default: break;
                }
            });
        } else if (id == ChunkID::TPKData) {
            const uint8_t* payloadBase = payload.data();
            IterateChunks(p, [&](uint32_t id2, std::span<const uint8_t> p2) {
                if (id2 == ChunkID::TPKDataRaw) {
                    rawData = StripAlignPad(p2);
                    rawDataRel = (uint64_t)(rawData.data() - payloadBase);
                }
            });
        }
    });
    if (absOffset && !rawData.empty())
        block.dataRawAbs = absOffset + rawDataRel;

    // ── Compressed variant (e.g. CARS/TEXTURES.BIN) ─────────────────────────
    // 0x33310003 descriptor table + per-entry JDLZ blocks inside the
    // 0x33320002 payload, addressed by ABSOLUTE file offset rather than
    // relative to the raw-data chunk. See TPKBlock.h for the format notes.
    if (isCompressedVariant && !compEntryData.empty()) {
        constexpr size_t kCompEntrySize = 24;
        const size_t entryCount = compEntryData.size() / kCompEntrySize;
        block.textures.reserve(entryCount);
        block.compressedSource = true;

        for (size_t i = 0; i < entryCount; ++i) {
            const uint8_t* e = compEntryData.data() + i * kCompEntrySize;
            uint32_t nameHash = 0, absOff = 0, compSz = 0, decompSz = 0;
            std::memcpy(&nameHash, e +  0, 4);
            std::memcpy(&absOff,   e +  4, 4);
            std::memcpy(&compSz,   e +  8, 4);
            std::memcpy(&decompSz, e + 12, 4);
            // flags at +16, padding at +20 — not needed for parsing.

            // Convert absolute file offset to payload-relative.
            if (absOff < (uint32_t)absOffset) {
                ++block.parseWarnings;
                LOG_WARN("TPKBlock '{}': compressed entry {} has absFileOffset "
                         "{} before payload start {}", block.name, i, absOff, absOffset);
                continue;
            }
            const size_t relOff = (size_t)absOff - (size_t)absOffset;
            if (relOff + compSz > payload.size()) {
                ++block.parseWarnings;
                LOG_WARN("TPKBlock '{}': compressed entry {} out of range "
                         "(relOff={}, compSz={}, payload={})",
                         block.name, i, relOff, compSz, payload.size());
                continue;
            }

            auto jdlzSpan = payload.subspan(relOff, compSz);

            // Verify JDLZ magic before attempting to decompress.
            if (jdlzSpan.size() < 4 || !LZCDecompressor::IsCompressed(jdlzSpan)) {
                ++block.parseWarnings;
                LOG_WARN("TPKBlock '{}': compressed entry {} (hash 0x{:08X}) "
                         "missing JDLZ magic", block.name, i, nameHash);
                continue;
            }

            auto decResult = LZCDecompressor::Decompress(jdlzSpan);
            if (!decResult) {
                ++block.parseWarnings;
                LOG_WARN("TPKBlock '{}': failed to decompress entry {} (hash "
                         "0x{:08X}): {}", block.name, i, nameHash, decResult.error);
                continue;
            }
            const auto& dec = decResult.value;
            constexpr size_t kInternalHeaderSize = 156;
            if (dec.size() <= kInternalHeaderSize) {
                ++block.parseWarnings;
                LOG_WARN("TPKBlock '{}': entry {} (hash 0x{:08X}) decompressed "
                         "too small ({} bytes)", block.name, i, nameHash, dec.size());
                continue;
            }

            Texture tex;
            tex.nameHash = nameHash;
            char hashStr[16];
            std::snprintf(hashStr, sizeof(hashStr), "%08X", nameHash);
            tex.name = hashStr;

            // The 156-byte internal NFS texture header carries width/height,
            // but its field layout is not yet reverse-engineered (see
            // TPKBlock.h notes). Infer dimensions from the pixel payload
            // size instead, assuming ARGB32 (the format observed in every
            // sample so far). If textures come out the wrong size/garbled,
            // this heuristic — not the JDLZ decompression — is the place to
            // fix once the header is mapped.
            const size_t pixelSz = dec.size() - kInternalHeaderSize;
            tex.data.assign(dec.data() + kInternalHeaderSize, dec.data() + dec.size());

            if      (pixelSz == 65536) { tex.width = 256; tex.height = 256; tex.format = TexFormat::ARGB32; }
            else if (pixelSz == 32768) { tex.width = 256; tex.height = 128; tex.format = TexFormat::ARGB32; }
            else if (pixelSz == 16384) { tex.width = 128; tex.height = 128; tex.format = TexFormat::ARGB32; }
            else if (pixelSz ==  4096) { tex.width =  64; tex.height =  64; tex.format = TexFormat::ARGB32; }
            else if (pixelSz ==  2048) { tex.width =  64; tex.height =  32; tex.format = TexFormat::ARGB32; }
            else {
                // Fallback: assume square ARGB32 and take the integer sqrt.
                uint32_t side = (uint32_t)std::sqrt((double)(pixelSz / 4));
                tex.width = tex.height = (uint16_t)side;
                tex.format = TexFormat::ARGB32;
            }
            tex.mipmaps = 1;
            tex.fileDataOffset = (uint32_t)relOff;
            tex.fileDataSize   = (uint32_t)compSz;
            tex.compressedSource = true; // read-only: no in-place patch slot

            HashResolver::Instance().RegisterHash(tex.nameHash, tex.name);
            block.hashIndex[tex.nameHash] = block.textures.size();
            block.textures.push_back(std::move(tex));
        }

        LOG_DEBUG("TPKBlock '{}' ({}): {} compressed textures parsed, {} warnings",
                  block.name, block.path, block.textures.size(), block.parseWarnings);
        return Result<TPKBlock>::Ok(std::move(block));
    }

    // ── Standard variant (0x33310004 entries + raw 0x33320002 pixel blob) ──
    const size_t count = entryData.size() / kEntrySize;
    const size_t compCount = compData.size() / kCompInfoSize;
    block.textures.reserve(count);

    for (size_t i = 0; i < count; ++i) {
        const uint8_t* e = entryData.data() + i * kEntrySize;
        Texture tex;

        const char* nm = reinterpret_cast<const char*>(e + 12);
        tex.name.assign(nm, strnlen(nm, 24));
        std::memcpy(&tex.nameHash, e + 36, 4);

        uint32_t dataOffset = 0, dataSize = 0;
        std::memcpy(&dataOffset, e + 48, 4);
        std::memcpy(&dataSize,   e + 56, 4);
        std::memcpy(&tex.width,  e + 68, 2);
        std::memcpy(&tex.height, e + 70, 2);
        tex.mipmaps = e[78];
        if (tex.mipmaps == 0 || tex.mipmaps > 13) tex.mipmaps = 1;

        if (i < compCount)
            tex.format = FourCCToFormat(compData.data() + i * kCompInfoSize + 20);

        tex.fileDataOffset = dataOffset;
        tex.fileDataSize   = dataSize;

        if (dataOffset + uint64_t(dataSize) <= rawData.size()) {
            tex.data.assign(rawData.data() + dataOffset,
                            rawData.data() + dataOffset + dataSize);
        } else {
            ++block.parseWarnings;
            LOG_WARN("TPKBlock '{}': texture '{}' data out of range (offset={}, size={}, avail={})",
                     block.name, tex.name, dataOffset, dataSize, rawData.size());
        }

        HashResolver::Instance().RegisterHash(tex.nameHash, tex.name);
        block.hashIndex[tex.nameHash] = block.textures.size();
        block.textures.push_back(std::move(tex));
    }

    LOG_DEBUG("TPKBlock '{}' ({}): {} textures, {} bytes payload",
              block.name, block.path, block.textures.size(), rawData.size());
    return Result<TPKBlock>::Ok(std::move(block));
}

// ─── DDS codec ────────────────────────────────────────────────────────────────
namespace {

#pragma pack(push, 1)
struct DDSPixelFormat {
    uint32_t size, flags, fourCC, rgbBitCount;
    uint32_t rMask, gMask, bMask, aMask;
};
struct DDSHeader {
    uint32_t magic;            // 'DDS '
    uint32_t size;             // 124
    uint32_t flags;
    uint32_t height, width;
    uint32_t pitchOrLinearSize;
    uint32_t depth, mipMapCount;
    uint32_t reserved[11];
    DDSPixelFormat pf;
    uint32_t caps, caps2, caps3, caps4, reserved2;
};
#pragma pack(pop)
static_assert(sizeof(DDSHeader) == 128);

constexpr uint32_t FOURCC(char a, char b, char c, char d) {
    return (uint32_t)a | ((uint32_t)b << 8) | ((uint32_t)c << 16) | ((uint32_t)d << 24);
}

size_t LevelBytes(TexFormat fmt, uint32_t w, uint32_t h) {
    switch (fmt) {
        case TexFormat::DXT1: return size_t((w + 3) / 4) * ((h + 3) / 4) * 8;
        case TexFormat::DXT3:
        case TexFormat::DXT5: return size_t((w + 3) / 4) * ((h + 3) / 4) * 16;
        case TexFormat::ARGB32: return size_t(w) * h * 4;
        default: return 0;
    }
}

size_t ChainBytes(TexFormat fmt, uint32_t w, uint32_t h, uint32_t mips) {
    size_t total = 0;
    for (uint32_t i = 0; i < mips; ++i) {
        total += LevelBytes(fmt, w, h);
        if (w == 1 && h == 1) break;
        w = std::max(1u, w >> 1);
        h = std::max(1u, h >> 1);
    }
    return total;
}

} // namespace

Result<void> DDSCodec::Import(Texture& tex, const std::filesystem::path& path) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f) return Result<void>::Err("Cannot open " + path.string());
    const auto fileSize = (size_t)f.tellg();
    if (fileSize < sizeof(DDSHeader))
        return Result<void>::Err("Not a DDS file (too small)");

    f.seekg(0);
    DDSHeader hdr{};
    f.read(reinterpret_cast<char*>(&hdr), sizeof(hdr));
    if (hdr.magic != FOURCC('D','D','S',' ') || hdr.size != 124)
        return Result<void>::Err("Not a DDS file (bad header)");

    TexFormat fmt = TexFormat::Unknown;
    if (hdr.pf.fourCC == FOURCC('D','X','T','1')) fmt = TexFormat::DXT1;
    else if (hdr.pf.fourCC == FOURCC('D','X','T','3')) fmt = TexFormat::DXT3;
    else if (hdr.pf.fourCC == FOURCC('D','X','T','5')) fmt = TexFormat::DXT5;
    else if (hdr.pf.fourCC == FOURCC('D','X','1','0'))
        return Result<void>::Err("DX10 extended DDS not supported - save as "
                                 "legacy DXT1/3/5 or 32-bit ARGB");
    else if (hdr.pf.rgbBitCount == 32) fmt = TexFormat::ARGB32;
    if (fmt == TexFormat::Unknown)
        return Result<void>::Err("Unsupported DDS pixel format (use DXT1/3/5 "
                                 "or 32-bit ARGB)");

    if (hdr.width == 0 || hdr.height == 0 ||
        hdr.width > 4096 || hdr.height > 4096)
        return Result<void>::Err("Implausible DDS dimensions");

    uint32_t mips = std::max(1u, hdr.mipMapCount);
    size_t need = ChainBytes(fmt, hdr.width, hdr.height, mips);
    const size_t avail = fileSize - sizeof(DDSHeader);
    if (avail < need) {
        // Tolerate headers that overstate the mip count.
        while (mips > 1 && ChainBytes(fmt, hdr.width, hdr.height, mips) > avail)
            --mips;
        need = ChainBytes(fmt, hdr.width, hdr.height, mips);
        if (avail < need)
            return Result<void>::Err("DDS pixel data truncated");
    }

    tex.width   = (uint16_t)hdr.width;
    tex.height  = (uint16_t)hdr.height;
    tex.mipmaps = (uint8_t)std::min(mips, 13u);
    tex.format  = fmt;
    tex.data.resize(need);
    f.read(reinterpret_cast<char*>(tex.data.data()), (std::streamsize)need);
    if (!f) return Result<void>::Err("Read error: " + path.string());
    tex.replaced = true;
    tex.glHandle = 0;

    LOG_INFO("DDS: imported {} ({}x{} {} mips={}, {} bytes) over '{}'",
             path.filename().string(), tex.width, tex.height,
             TexFormatName(fmt), tex.mipmaps, need, tex.name);
    return Result<void>::Ok();
}

Result<void> DDSCodec::Export(const Texture& tex, std::span<const uint8_t> pixels,
                              const std::filesystem::path& path) {
    if (pixels.empty())
        return Result<void>::Err("No pixel data available for '" + tex.name + "'");

    DDSHeader hdr{};
    hdr.magic  = FOURCC('D','D','S',' ');
    hdr.size   = 124;
    hdr.flags  = 0x1 | 0x2 | 0x4 | 0x1000;          // caps|height|width|pixelformat
    if (tex.mipmaps > 1) hdr.flags |= 0x20000;       // mipmapcount
    hdr.height = tex.height;
    hdr.width  = tex.width;
    hdr.mipMapCount = tex.mipmaps;
    hdr.pf.size = 32;
    switch (tex.format) {
        case TexFormat::DXT1: hdr.pf.flags = 0x4; hdr.pf.fourCC = FOURCC('D','X','T','1');
            hdr.flags |= 0x80000;
            hdr.pitchOrLinearSize = (uint32_t)LevelBytes(tex.format, tex.width, tex.height);
            break;
        case TexFormat::DXT3: hdr.pf.flags = 0x4; hdr.pf.fourCC = FOURCC('D','X','T','3');
            hdr.flags |= 0x80000;
            hdr.pitchOrLinearSize = (uint32_t)LevelBytes(tex.format, tex.width, tex.height);
            break;
        case TexFormat::DXT5: hdr.pf.flags = 0x4; hdr.pf.fourCC = FOURCC('D','X','T','5');
            hdr.flags |= 0x80000;
            hdr.pitchOrLinearSize = (uint32_t)LevelBytes(tex.format, tex.width, tex.height);
            break;
        case TexFormat::ARGB32:
            hdr.pf.flags = 0x41;                     // RGB | ALPHAPIXELS
            hdr.pf.rgbBitCount = 32;
            hdr.pf.rMask = 0x00FF0000; hdr.pf.gMask = 0x0000FF00;
            hdr.pf.bMask = 0x000000FF; hdr.pf.aMask = 0xFF000000;
            break;
        default:
            return Result<void>::Err("'" + tex.name + "' has an unsupported format");
    }
    hdr.caps = 0x1000;                               // TEXTURE
    if (tex.mipmaps > 1) hdr.caps |= 0x400008;       // COMPLEX | MIPMAP

    std::ofstream f(path, std::ios::binary | std::ios::trunc);
    if (!f) return Result<void>::Err("Cannot create " + path.string());
    f.write(reinterpret_cast<const char*>(&hdr), sizeof(hdr));
    f.write(reinterpret_cast<const char*>(pixels.data()),
            (std::streamsize)pixels.size());
    if (!f) return Result<void>::Err("Write error: " + path.string());

    LOG_INFO("DDS: exported '{}' -> {}", tex.name, path.string());
    return Result<void>::Ok();
}

} // namespace nfsmw
