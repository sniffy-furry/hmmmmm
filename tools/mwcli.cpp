// ─── tools/mwcli.cpp ──────────────────────────────────────────────────────────
//
// MWTools headless command-line verifier.
//
// A GUI-free companion to the toolkit that links only against `nfsmw_core`
// (no OpenGL / ImGui / FFmpeg / miniaudio). Its job is to validate the format
// parsers and patchers against real game files without launching the full
// application — the iteration loop the roadmap (P0-3) asks for.
//
// Subcommands:
//   info <file>        dump the container/chunk tree (auto JDLZ-decompress)
//   chunks <file>      flat top-level chunk listing with offsets
//   jdlz <file>        report JDLZ compression state + decompressed size
//   tpk <file>         list every texture pack and its textures
//   solid <file>       list every geometry container, its objects and meshes
//   roundtrip <file>   verify the chunk walker tiles the buffer losslessly
//
// Exit code is 0 on success, 1 on any error or failed verification, so the
// tool is usable from scripts / CI.

#include "core/BINFile.h"
#include "core/ChunkReader.h"
#include "core/LZCDecompressor.h"
#include "formats/TPKBlock.h"
#include "formats/SolidList.h"
#include "formats/VaultFile.h"
#include "formats/NisAnim.h"
#include "formats/WorldData.h"
#include "formats/StreamBundle.h"
#include "core/DdsWriter.h"
#include "patch/MwMod.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <functional>
#include <fstream>
#include <map>
#include <span>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using namespace nfsmw;

namespace {

// ── Pretty names for the chunk IDs we know about ────────────────────────────
const char* ChunkName(uint32_t id) {
    switch (id) {
        case ChunkID::GeometryContainer:    return "GeometryContainer";
        case ChunkID::GeometryInfo:         return "GeometryInfo";
        case ChunkID::GeometryInfoHeader:   return "GeometryInfoHeader";
        case ChunkID::GeometryHashTable:    return "GeometryHashTable";
        case ChunkID::GeometryEmpty:        return "GeometryEmpty";
        case ChunkID::GeometryObject:       return "GeometryObject";
        case ChunkID::GeometryObjectHeader: return "GeometryObjectHeader";
        case ChunkID::GeometryTextureRefs:  return "GeometryTextureRefs";
        case ChunkID::GeometryLightMaterial:return "GeometryLightMaterial";
        case ChunkID::GeometryMesh:         return "GeometryMesh";
        case ChunkID::MeshHeader:           return "MeshHeader";
        case ChunkID::MeshShadingGroups:    return "MeshShadingGroups";
        case ChunkID::MeshIndices:          return "MeshIndices";
        case ChunkID::MeshVertices:         return "MeshVertices";
        case ChunkID::MeshMaterialName:     return "MeshMaterialName";
        case ChunkID::ScenerySection:       return "ScenerySection";
        case ChunkID::TPKContainer:         return "TPKContainer";
        case ChunkID::TPKInfo:              return "TPKInfo";
        case ChunkID::TPKInfoHeader:        return "TPKInfoHeader";
        case ChunkID::TPKHashTable:         return "TPKHashTable";
        case ChunkID::TPKCompEntries:       return "TPKCompEntries";
        case ChunkID::TPKEntries:           return "TPKEntries";
        case ChunkID::TPKCompInfo:          return "TPKCompInfo";
        case ChunkID::TPKData:              return "TPKData";
        case ChunkID::TPKDataHeader:        return "TPKDataHeader";
        case ChunkID::TPKDataRaw:           return "TPKDataRaw";
        case ChunkID::TPKAnimBlock:         return "TPKAnimBlock";
        case ChunkID::Null:                 return "Null/pad";
        default:                            return "(unknown)";
    }
}

// ── Load a file fully into memory, JDLZ-decompressing if needed ─────────────
// Returns the decompressed bytes plus a flag for whether decompression ran.
struct Loaded {
    std::vector<uint8_t> bytes;
    bool                 wasCompressed = false;
    size_t               rawSize       = 0;
};

bool LoadRaw(const fs::path& path, std::vector<uint8_t>& out) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f) return false;
    auto sz = static_cast<size_t>(f.tellg());
    f.seekg(0);
    out.resize(sz);
    return static_cast<bool>(f.read(reinterpret_cast<char*>(out.data()),
                                    static_cast<std::streamsize>(sz)));
}

bool Load(const fs::path& path, Loaded& out) {
    std::vector<uint8_t> raw;
    if (!LoadRaw(path, raw)) {
        std::fprintf(stderr, "error: cannot read %s\n", path.string().c_str());
        return false;
    }
    out.rawSize = raw.size();
    if (LZCDecompressor::IsCompressed(raw)) {
        auto r = LZCDecompressor::Decompress(raw);
        if (!r) {
            std::fprintf(stderr, "error: JDLZ decompress failed: %s\n", r.error.c_str());
            return false;
        }
        out.bytes = std::move(r.value);
        out.wasCompressed = true;
    } else {
        out.bytes = std::move(raw);
        out.wasCompressed = false;
    }
    return true;
}

// ── Recursive chunk-tree printer for `info` ─────────────────────────────────
// Walks chunks directly so we control formatting and depth. Container chunks
// (bit 31 set) are descended into; leaf chunks just print their size.
void PrintTree(std::span<const uint8_t> data, int depth, size_t baseOff,
               size_t& chunkCount) {
    size_t pos = 0;
    while (pos + 8 <= data.size()) {
        ChunkHeader h{};
        std::memcpy(&h, data.data() + pos, 8);
        // A zero id with zero size is alignment filler at the tail.
        if (h.id == 0 && h.size == 0) break;
        size_t payloadOff = pos + 8;
        size_t payloadEnd = payloadOff + h.size;
        if (payloadEnd > data.size()) {
            std::printf("%*s! truncated chunk 0x%08X size %u at +0x%zX "
                        "(only %zu bytes left)\n",
                        depth * 2, "", h.id, h.size, baseOff + pos,
                        data.size() - payloadOff);
            break;
        }
        ++chunkCount;
        std::printf("%*s0x%08X  %-22s  size=%-8u  @+0x%zX\n",
                    depth * 2, "", h.id, ChunkName(h.id), h.size,
                    baseOff + pos);
        if (ChunkReader::IsContainer(h.id) && h.size >= 8 && depth < 12) {
            PrintTree(data.subspan(payloadOff, h.size), depth + 1,
                      baseOff + payloadOff, chunkCount);
        }
        pos = payloadEnd;
    }
}

// ── Subcommands ─────────────────────────────────────────────────────────────

int CmdInfo(const fs::path& path) {
    Loaded l;
    if (!Load(path, l)) return 1;
    std::printf("file: %s\n", path.string().c_str());
    std::printf("on-disk size : %zu bytes%s\n", l.rawSize,
                l.wasCompressed ? " (JDLZ-compressed)" : "");
    if (l.wasCompressed)
        std::printf("decompressed : %zu bytes\n", l.bytes.size());
    std::printf("----------------------------------------\n");
    size_t count = 0;
    PrintTree(l.bytes, 0, 0, count);
    std::printf("----------------------------------------\n");
    std::printf("%zu chunk(s)\n", count);
    return 0;
}

int CmdChunks(const fs::path& path) {
    Loaded l;
    if (!Load(path, l)) return 1;
    size_t pos = 0, count = 0;
    while (pos + 8 <= l.bytes.size()) {
        ChunkHeader h{};
        std::memcpy(&h, l.bytes.data() + pos, 8);
        if (h.id == 0 && h.size == 0) break;
        if (pos + 8 + h.size > l.bytes.size()) {
            std::printf("! truncated at +0x%zX\n", pos);
            return 1;
        }
        std::printf("+0x%08zX  0x%08X  %-22s  size=%u\n",
                    pos, h.id, ChunkName(h.id), h.size);
        pos += 8 + h.size;
        ++count;
    }
    std::printf("%zu top-level chunk(s), %zu bytes\n", count, l.bytes.size());
    return 0;
}

int CmdJdlz(const fs::path& path) {
    std::vector<uint8_t> raw;
    if (!LoadRaw(path, raw)) {
        std::fprintf(stderr, "error: cannot read %s\n", path.string().c_str());
        return 1;
    }
    if (!LZCDecompressor::IsCompressed(raw)) {
        std::printf("%s: not JDLZ-compressed (%zu bytes)\n",
                    path.string().c_str(), raw.size());
        return 0;
    }
    auto r = LZCDecompressor::Decompress(raw);
    if (!r) {
        std::fprintf(stderr, "error: decompress failed: %s\n", r.error.c_str());
        return 1;
    }
    double ratio = raw.empty() ? 0.0
                 : 100.0 * static_cast<double>(raw.size()) / static_cast<double>(r.value.size());
    std::printf("%s: JDLZ ok\n", path.string().c_str());
    std::printf("  compressed   : %zu bytes\n", raw.size());
    std::printf("  decompressed : %zu bytes\n", r.value.size());
    std::printf("  ratio        : %.1f%%\n", ratio);
    return 0;
}

int CmdTpk(const fs::path& path) {
    auto bf = BINFile::Open(path);
    if (!bf) {
        std::fprintf(stderr, "error: %s\n", bf.error.c_str());
        return 1;
    }
    int packs = 0, fail = 0;
    bf.value.ForEachTPK([&](std::span<const uint8_t> payload, uint64_t absOff) {
        auto r = TPKBlockParser::Parse(payload, absOff);
        if (!r) {
            std::printf("[TPK @+0x%llX] parse failed: %s\n",
                        static_cast<unsigned long long>(absOff), r.error.c_str());
            ++fail;
            return;
        }
        const TPKBlock& tpk = r.value;
        std::printf("TPK \"%s\"  (%zu textures)  @+0x%llX\n",
                    tpk.name.c_str(), tpk.textures.size(),
                    static_cast<unsigned long long>(absOff));
        for (const auto& t : tpk.textures) {
            std::printf("    %-28s  %4ux%-4u  hash=0x%08X  %zu bytes\n",
                        t.name.empty() ? "(unnamed)" : t.name.c_str(),
                        t.width, t.height, t.nameHash, t.data.size());
        }
        ++packs;
    });
    std::printf("%d texture pack(s)%s\n", packs,
                fail ? " (some failed to parse)" : "");
    return fail ? 1 : 0;
}

int CmdSolid(const fs::path& path) {
    Loaded l;
    if (!Load(path, l)) return 1;
    int lists = 0, fail = 0;
    // Walk top-level + one level of containers looking for GeometryContainer.
    std::function<void(std::span<const uint8_t>, size_t)> walk =
        [&](std::span<const uint8_t> data, size_t base) {
        size_t pos = 0;
        while (pos + 8 <= data.size()) {
            ChunkHeader h{};
            std::memcpy(&h, data.data() + pos, 8);
            if (h.id == 0 && h.size == 0) break;
            size_t payloadOff = pos + 8;
            if (payloadOff + h.size > data.size()) break;
            auto payload = data.subspan(payloadOff, h.size);
            if (h.id == ChunkID::GeometryContainer) {
                auto r = SolidListParser::Parse(payload, base + payloadOff);
                if (!r) {
                    std::printf("[SolidList @+0x%zX] parse failed: %s\n",
                                base + payloadOff, r.error.c_str());
                    ++fail;
                } else {
                    const SolidList& sl = r.value;
                    std::printf("SolidList @+0x%zX  (%zu object(s))\n",
                                base + payloadOff, sl.objects.size());
                    for (const auto& o : sl.objects) {
                        size_t tris = 0, verts = 0;
                        for (const auto& m : o.meshes) {
                            tris  += m.indices.size() / 3;
                            verts += m.vertices.size();
                        }
                        std::printf("    %-28s  meshes=%-3zu verts=%-6zu tris=%-6zu "
                                    "hash=0x%08X\n",
                                    o.name.empty() ? "(unnamed)" : o.name.c_str(),
                                    o.meshes.size(), verts, tris, o.nameHash);
                    }
                    ++lists;
                }
            } else if (ChunkReader::IsContainer(h.id) && h.size >= 8) {
                walk(payload, base + payloadOff);
            }
            pos = payloadOff + h.size;
        }
    };
    walk(l.bytes, 0);
    std::printf("%d geometry container(s)%s\n", lists,
                fail ? " (some failed to parse)" : "");
    return fail ? 1 : 0;
}

// nisanim: parse every 0x00E34009 chunk (NIS cutscene animation/EAGL ELF)
// and list the named bone/skeleton symbols it carries.
int CmdNisAnim(const fs::path& path) {
    Loaded l;
    if (!Load(path, l)) return 1;
    int clips = 0, fail = 0;
    std::function<void(std::span<const uint8_t>, size_t)> walk =
        [&](std::span<const uint8_t> data, size_t base) {
        size_t pos = 0;
        while (pos + 8 <= data.size()) {
            ChunkHeader h{};
            std::memcpy(&h, data.data() + pos, 8);
            if (h.id == 0 && h.size == 0) break;
            size_t payloadOff = pos + 8;
            if (payloadOff + h.size > data.size()) break;
            auto payload = data.subspan(payloadOff, h.size);
            if (h.id == ChunkID::NisAnimation) {
                auto r = NisAnimParser::Parse(payload);
                if (!r) {
                    std::printf("[NisAnim @+0x%zX] parse failed: %s\n",
                                base + payloadOff, r.error.c_str());
                    ++fail;
                } else {
                    const NisAnimClip& clip = r.value;
                    std::printf("NisAnim @+0x%zX  skeleton=%s  bones=%zu  "
                                "sections=%zu  symbols=%zu\n",
                                base + payloadOff,
                                clip.skeletonName.empty() ? "?" : clip.skeletonName.c_str(),
                                clip.boneSymbols.size(), clip.sections.size(),
                                clip.symbols.size());
                    for (size_t si = 0; si < clip.sections.size(); ++si)
                        std::printf("    [section %zu] name=%-12s type=%u bytes=%zu\n",
                                    si, clip.sections[si].name.c_str(),
                                    clip.sections[si].type, clip.sections[si].bytes.size());
                    // Sort by record offset so the table reads in physical
                    // .data order (the symbol table order is something else).
                    std::vector<size_t> byOffset(clip.boneSymbols.begin(), clip.boneSymbols.end());
                    std::sort(byOffset.begin(), byOffset.end(), [&](size_t a, size_t b) {
                        return clip.symbols[a].value < clip.symbols[b].value;
                    });
                    for (size_t bi : byOffset) {
                        const NisAnimSymbol& s = clip.symbols[bi];
                        if (s.sectionIndex >= clip.sections.size()) continue;
                        const auto& bytes = clip.sections[s.sectionIndex].bytes;
                        // Dump the full 16-byte stride (4 floats / 4 u32s), not
                        // just the symbol's own 4-byte slot, in case the other
                        // 12 bytes hold a pointer/count into the keyframe data.
                        uint32_t rec[4] = {};
                        if (s.value + 16 <= bytes.size())
                            std::memcpy(rec, bytes.data() + s.value, 16);
                        float recf[4];
                        std::memcpy(recf, rec, 16);
                        std::printf("    [0x%-4X] %-44s  u=(%u,%u,%u,%u) f=(%g,%g,%g,%g)\n",
                                    s.value, s.name.c_str(),
                                    rec[0], rec[1], rec[2], rec[3],
                                    recf[0], recf[1], recf[2], recf[3]);
                    }
                    // DEBUG: sliding-window unit-quaternion detector over the
                    // unindexed tail of .data, to find the real stride/alignment
                    // empirically instead of guessing a fixed grouping by eye.
                    if (!clip.sections.empty()) {
                        uint32_t maxEnd = 0;
                        for (size_t bi : clip.boneSymbols) {
                            const NisAnimSymbol& s = clip.symbols[bi];
                            maxEnd = std::max(maxEnd, s.value + s.size);
                        }
                        for (size_t si = 0; si < clip.sections.size(); ++si) {
                            if (clip.sections[si].name != ".data") continue;
                            const auto& bytes = clip.sections[si].bytes;
                            const size_t tailBytes = bytes.size() - maxEnd;
                            std::printf("    -- .data tail from 0x%X to 0x%zX (%zu bytes unindexed) --\n",
                                        maxEnd, bytes.size(), tailBytes);
                            const size_t nFloats = tailBytes / 4;
                            std::vector<float> f(nFloats);
                            std::memcpy(f.data(), bytes.data() + maxEnd, nFloats * 4);

                            // For every float-aligned offset, test if the next 4
                            // floats form a unit quaternion (sum of squares ~ 1).
                            std::printf("    -- unit-quaternion hits (any 4-float window summing |q|^2 in [0.98,1.02]) --\n");
                            size_t hits = 0;
                            for (size_t i = 0; i + 4 <= nFloats; ++i) {
                                double m = 0;
                                bool finite = true;
                                for (int k = 0; k < 4; ++k) {
                                    if (!std::isfinite(f[i+k])) { finite = false; break; }
                                    m += double(f[i+k]) * double(f[i+k]);
                                }
                                if (finite && m > 0.98 && m < 1.02) {
                                    std::printf("    [+0x%zX] (%g,%g,%g,%g) |q|^2=%g\n",
                                                maxEnd + i*4, f[i], f[i+1], f[i+2], f[i+3], m);
                                    ++hits;
                                }
                            }
                            std::printf("    -- %zu/%zu windows hit, stride analysis: --\n", hits, nFloats);
                            // Histogram of gaps between consecutive hit start indices
                            // (in floats) to infer the real stride.
                            std::vector<size_t> hitIdx;
                            for (size_t i = 0; i + 4 <= nFloats; ++i) {
                                double m = 0; bool finite = true;
                                for (int k = 0; k < 4; ++k) {
                                    if (!std::isfinite(f[i+k])) { finite = false; break; }
                                    m += double(f[i+k]) * double(f[i+k]);
                                }
                                if (finite && m > 0.98 && m < 1.02) hitIdx.push_back(i);
                            }
                            std::map<size_t,int> gapHist;
                            for (size_t i = 1; i < hitIdx.size(); ++i)
                                gapHist[hitIdx[i] - hitIdx[i-1]]++;
                            for (auto& [gap, count] : gapHist)
                                std::printf("    gap=%zu floats (%zu bytes): %d times\n", gap, gap*4, count);

                            // DEBUG: dump candidate 5-float (20-byte) keyframe
                            // records [scalar, quat] starting at every offset
                            // 0..15 (mod 16... actually mod 20 bytes) to find
                            // true phase alignment of the 20-byte stride.
                            for (size_t phase = 0; phase < 5; ++phase) {
                                std::printf("    == phase %zu (start +0x%zX) ==\n", phase, phase*4);
                                for (size_t i = phase; i + 5 <= nFloats; i += 5) {
                                    double m = double(f[i+1])*f[i+1] + double(f[i+2])*f[i+2] +
                                               double(f[i+3])*f[i+3] + double(f[i+4])*f[i+4];
                                    std::printf("    [+0x%zX] s=%-12g q=(%g,%g,%g,%g) |q|^2=%g\n",
                                                maxEnd + i*4, f[i], f[i+1], f[i+2], f[i+3], f[i+4], m);
                                }
                            }
                        }
                    }
                    ++clips;
                }
            } else if (ChunkReader::IsContainer(h.id) && h.size >= 8) {
                walk(payload, base + payloadOff);
            }
            pos = payloadOff + h.size;
        }
    };
    walk(l.bytes, 0);
    std::printf("%d NisAnim clip(s)%s\n", clips, fail ? " (some failed to parse)" : "");
    return fail ? 1 : 0;
}

// roundtrip: verify the chunk walker tiles the decompressed buffer with no
// gaps, overlaps, or leftover bytes (other than trailing alignment filler).
// A malformed parse — the kind that produces a file the game rejects — shows
// up here as a coverage gap before it ever reaches a real install.
int CmdRoundtrip(const fs::path& path) {
    Loaded l;
    if (!Load(path, l)) return 1;

    size_t pos = 0, covered = 0, count = 0;
    bool ok = true;
    while (pos + 8 <= l.bytes.size()) {
        ChunkHeader h{};
        std::memcpy(&h, l.bytes.data() + pos, 8);
        if (h.id == 0 && h.size == 0) break; // trailing filler
        size_t end = pos + 8 + h.size;
        if (end > l.bytes.size()) {
            std::printf("FAIL: chunk 0x%08X at +0x%zX overruns buffer "
                        "(size %u, %zu bytes left)\n",
                        h.id, pos, h.size, l.bytes.size() - (pos + 8));
            ok = false;
            break;
        }
        covered += 8 + h.size;
        pos = end;
        ++count;
    }

    size_t trailing = l.bytes.size() - pos; // alignment filler after last chunk
    std::printf("roundtrip %s\n", path.string().c_str());
    std::printf("  chunks parsed : %zu\n", count);
    std::printf("  bytes covered : %zu / %zu\n", covered, l.bytes.size());
    if (trailing) std::printf("  trailing pad  : %zu bytes\n", trailing);

    // Verify trailing region is pure filler (zero or 0x11), not lost data.
    for (size_t i = pos; i < l.bytes.size(); ++i) {
        uint8_t b = l.bytes[i];
        if (b != 0x00 && b != kChunkPadByte) {
            std::printf("FAIL: non-filler byte 0x%02X in trailing region at +0x%zX\n",
                        b, i);
            ok = false;
            break;
        }
    }

    if (ok && count == 0) {
        std::printf("FAIL: no chunks parsed (not a chunked container?)\n");
        ok = false;
    }
    std::printf("%s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}

// vpak: dump an EA Attribulator vault (attributes.bin etc.) — header, located
// blocks, and a categorised view of the recovered name table. This is where
// car performance/pursuit tuning actually lives (see VaultFile.h).
int CmdVpak(const fs::path& path) {
    auto r = VaultParser::Load(path);
    if (!r) {
        std::fprintf(stderr, "error: %s\n", r.error.c_str());
        return 1;
    }
    const VaultFile& vf = r.value;
    std::printf("vault: %s\n", path.string().c_str());
    std::printf("  version       : %u\n", vf.version);
    std::printf("  header size   : 0x%X\n", vf.headerSize);
    std::printf("  section count : %u\n", vf.sectionCount);
    std::printf("  file size     : %llu bytes\n",
                static_cast<unsigned long long>(vf.fileSize));
    std::printf("  blocks        :\n");
    for (const auto& b : vf.blocks)
        std::printf("      '%s'  payload @+0x%llX  size=%u\n", b.tag.c_str(),
                    static_cast<unsigned long long>(b.offset), b.size);

    std::printf("  strings       : %zu unique names\n", vf.strings.size());

    // Categorise: EA::Reflection / Attrib:: type names vs game class names
    // (anything with "::") vs pursuit/race-relevant names.
    size_t types = 0, classes = 0;
    std::vector<std::string> pursuit;
    for (const auto& s : vf.strings) {
        bool scoped = s.find("::") != std::string::npos;
        if (s.rfind("EA::Reflection", 0) == 0 || s.rfind("Attrib::", 0) == 0)
            ++types;
        else if (scoped)
            ++classes;
        for (const char* kw : {"Cop", "Pursuit", "pursuit", "Heat", "heat",
                               "Traffic", "Chase", "Bust"})
            if (s.find(kw) != std::string::npos) { pursuit.push_back(s); break; }
    }
    std::printf("    reflection/attrib types : %zu\n", types);
    std::printf("    scoped class names      : %zu\n", classes);
    std::printf("    pursuit/traffic-related : %zu\n", pursuit.size());
    size_t show = std::min<size_t>(pursuit.size(), 24);
    for (size_t i = 0; i < show; ++i)
        std::printf("        %s\n", pursuit[i].c_str());
    if (pursuit.size() > show)
        std::printf("        ... and %zu more\n", pursuit.size() - show);

    // NtaD record enumeration (structural — verified boundaries only).
    if (!vf.records.empty()) {
        uint32_t minSz = 0xFFFFFFFFu, maxSz = 0;
        for (const auto& rec : vf.records) {
            minSz = std::min(minSz, rec.size);
            maxSz = std::max(maxSz, rec.size);
        }
        std::printf("  NtaD records  : %zu (size %u..%u bytes)\n",
                    vf.records.size(), minSz, maxSz);
        size_t rshow = std::min<size_t>(vf.records.size(), 6);
        for (size_t i = 0; i < rshow; ++i)
            std::printf("        @+0x%llX  size=%-3u  word0=0x%08X\n",
                        static_cast<unsigned long long>(vf.records[i].offset),
                        vf.records[i].size, vf.records[i].word0);
    }

    size_t seeded = VaultParser::SeedHashResolver(vf);
    std::printf("  seeded %zu names into the hash resolver (P3-2)\n", seeded);
    return 0;
}

// jdlzc <in> [out]  — JDLZ-compress `in`. If `in` is already JDLZ, its
// decompressed form is (re)compressed. Always round-trips its own output
// (Decompress(Compress(x)) == x) and refuses to write on mismatch, so it is
// safe in scripts / CI. With no `out`, only verifies and reports the ratio.
int CmdJdlzCompress(const fs::path& in, const fs::path& out) {
    std::vector<uint8_t> raw;
    if (!LoadRaw(in, raw)) {
        std::fprintf(stderr, "error: cannot read %s\n", in.string().c_str());
        return 1;
    }
    std::vector<uint8_t> src = raw;
    if (LZCDecompressor::IsCompressed(raw)) {
        auto d = LZCDecompressor::Decompress(raw);
        if (!d) { std::fprintf(stderr, "error: source decompress failed: %s\n", d.error.c_str()); return 1; }
        src = std::move(d.value);
    }
    auto c = LZCDecompressor::Compress(src);
    if (!c) { std::fprintf(stderr, "error: compress failed: %s\n", c.error.c_str()); return 1; }

    auto back = LZCDecompressor::Decompress(c.value);
    if (!back || back.value != src) {
        std::fprintf(stderr, "error: round-trip verification FAILED — output not written\n");
        return 1;
    }
    double ratio = src.empty() ? 0.0
                 : 100.0 * static_cast<double>(c.value.size()) / static_cast<double>(src.size());
    std::printf("%s: compressed %zu -> %zu bytes (%.1f%%), round-trip OK\n",
                in.string().c_str(), src.size(), c.value.size(), ratio);

    if (!out.empty()) {
        std::ofstream f(out, std::ios::binary);
        if (!f) { std::fprintf(stderr, "error: cannot write %s\n", out.string().c_str()); return 1; }
        f.write(reinterpret_cast<const char*>(c.value.data()),
                static_cast<std::streamsize>(c.value.size()));
        std::printf("  wrote %s\n", out.string().c_str());
    }
    return 0;
}

// ── vault NtaD record helpers (roadmap V1/V4: schema-discovery bootstrap) ──
// Locate every typed record by its 0xEFFECADD marker; a record runs from one
// marker to the next.
static std::vector<std::pair<size_t,size_t>> VaultRecordSpans(const std::vector<uint8_t>& b) {
    std::vector<std::pair<size_t,size_t>> spans;  // (offset, size)
    std::vector<size_t> marks;
    for (size_t i = 0; i + 4 <= b.size(); ++i)
        if (b[i]==0xDD && b[i+1]==0xCA && b[i+2]==0xFE && b[i+3]==0xEF) marks.push_back(i);
    for (size_t k = 0; k < marks.size(); ++k) {
        size_t end = (k + 1 < marks.size()) ? marks[k+1] : b.size();
        spans.emplace_back(marks[k], end - marks[k]);
    }
    return spans;
}

// vpakrecords <file>  — emit every NtaD record as a JSON line (offset/size/keyHash).
int CmdVpakRecords(const fs::path& path) {
    std::vector<uint8_t> b;
    if (!LoadRaw(path, b)) { std::fprintf(stderr, "error: cannot read %s\n", path.string().c_str()); return 1; }
    auto spans = VaultRecordSpans(b);
    std::printf("{ \"file\": \"%s\", \"records\": %zu, \"items\": [\n",
                path.filename().string().c_str(), spans.size());
    for (size_t k = 0; k < spans.size(); ++k) {
        uint32_t key = 0;
        if (spans[k].first + 8 <= b.size()) std::memcpy(&key, b.data() + spans[k].first + 4, 4);
        std::printf("  { \"i\": %zu, \"off\": %zu, \"size\": %zu, \"keyHash\": \"0x%08X\" }%s\n",
                    k, spans[k].first, spans[k].second, key, (k + 1 < spans.size()) ? "," : "");
    }
    std::printf("] }\n");
    return 0;
}

// vpakdiff <a> <b>  — align two vaults by record marker and report which
// records changed and the first differing byte (the field-localisation step
// of dynamic schema recovery, Chapter 14 §6.2).
int CmdVpakDiff(const fs::path& pa, const fs::path& pb) {
    std::vector<uint8_t> a, b;
    if (!LoadRaw(pa, a) || !LoadRaw(pb, b)) { std::fprintf(stderr, "error: cannot read inputs\n"); return 1; }
    auto sa = VaultRecordSpans(a), sb = VaultRecordSpans(b);
    std::printf("vpakdiff: %s (%zu records) vs %s (%zu records)\n",
                pa.filename().string().c_str(), sa.size(), pb.filename().string().c_str(), sb.size());
    if (sa.size() != sb.size())
        std::printf("  NOTE: record counts differ (%zu vs %zu) — alignment is positional\n", sa.size(), sb.size());
    size_t n = std::min(sa.size(), sb.size()), changed = 0;
    for (size_t k = 0; k < n; ++k) {
        size_t la = sa[k].second, lb = sb[k].second;
        size_t m = std::min(la, lb);
        size_t diffAt = m;
        for (size_t i = 0; i < m; ++i)
            if (a[sa[k].first + i] != b[sb[k].first + i]) { diffAt = i; break; }
        if (diffAt != m || la != lb) {
            ++changed;
            uint32_t va = 0, vb = 0;
            if (diffAt < m) {
                std::memcpy(&va, a.data() + sa[k].first + (diffAt & ~size_t(3)), std::min<size_t>(4, la - (diffAt & ~size_t(3))));
                std::memcpy(&vb, b.data() + sb[k].first + (diffAt & ~size_t(3)), std::min<size_t>(4, lb - (diffAt & ~size_t(3))));
            }
            std::printf("  record %zu @+0x%llX: first diff at intra-offset +%zu  (0x%08X -> 0x%08X)%s\n",
                        k, static_cast<unsigned long long>(sa[k].first),
                        (diffAt & ~size_t(3)), va, vb, (la != lb) ? "  [size changed]" : "");
        }
    }
    std::printf("  %zu of %zu aligned records changed\n", changed, n);
    return 0;
}

// mod <manifest>  — parse a .mwmod manifest, print the resolved edit plan and
// flag conflicts (dry-run; roadmap D1). Actual application delegates to the
// patch layer (BunRepacker / MinimapRepacker / audio) via BackupManager.
int CmdMod(const fs::path& manifest) {
    std::ifstream f(manifest, std::ios::binary);
    if (!f) { std::fprintf(stderr, "error: cannot read %s\n", manifest.string().c_str()); return 1; }
    std::string text((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());

    std::vector<std::string> warnings;
    auto r = MwModParser::Parse(text, &warnings);
    if (!r) { std::fprintf(stderr, "error: %s\n", r.error.c_str()); return 1; }
    const MwMod& m = r.value;

    std::printf("mod: %s\n", m.name.empty() ? "(unnamed)" : m.name.c_str());
    if (!m.author.empty())        std::printf("  author : %s\n", m.author.c_str());
    if (!m.targetVersion.empty()) std::printf("  target : %s\n", m.targetVersion.c_str());
    std::printf("  edits  : %zu\n", m.edits.size());
    for (const auto& e : m.edits)
        std::printf("    [%-20s] %s  (%s)  %s\n", MwModParser::ActionName(e.action),
                    e.file.c_str(), e.locText.c_str(), e.asset.c_str());
    for (const auto& w : warnings) std::printf("  WARNING: %s\n", w.c_str());

    auto conflicts = MwModParser::FindConflicts(m);
    if (conflicts.empty()) {
        std::printf("  conflicts: none\n");
    } else {
        std::printf("  conflicts: %zu\n", conflicts.size());
        for (const auto& c : conflicts)
            std::printf("    edits %zu and %zu both target %s\n", c.a, c.b, c.key.c_str());
    }
    std::printf("  (dry-run: no files were modified)\n");
    return conflicts.empty() ? 0 : 1;
}

// tpkextract <file> [outdir]  — export every texture in every pack to .dds
// (roadmap X1, read-only export). outdir defaults to "<stem>_textures".
int CmdTpkExtract(const fs::path& path, fs::path outdir) {
    auto sec = StreamBundleLoader::Load(path);
    if (!sec) { std::fprintf(stderr, "error: %s\n", sec.error.c_str()); return 1; }
    if (outdir.empty()) outdir = path.stem().string() + "_textures";
    std::error_code ec; fs::create_directories(outdir, ec);

    auto mapFmt = [](TexFormat f, DdsFormat& out) -> bool {
        switch (f) {
            case TexFormat::DXT1:   out = DdsFormat::DXT1;   return true;
            case TexFormat::DXT3:   out = DdsFormat::DXT3;   return true;
            case TexFormat::DXT5:   out = DdsFormat::DXT5;   return true;
            case TexFormat::ARGB32: out = DdsFormat::ARGB32; return true;
            default: return false;  // PAL8 / Unknown: skip
        }
    };

    size_t written = 0, skipped = 0, pi = 0;
    for (const auto& tpk : sec.value.texturePacks) {
        for (const auto& t : tpk.textures) {
            DdsFormat df;
            if (t.data.empty() || !mapFmt(t.format, df)) { ++skipped; continue; }
            auto dds = BuildDDS(t.width, t.height, df, t.data, t.mipmaps);
            std::string fname = "tpk" + std::to_string(pi) + "_" +
                                (t.name.empty() ? ("tex" + std::to_string(written)) : t.name) + ".dds";
            fs::path op = outdir / fname;
            std::ofstream f(op, std::ios::binary);
            if (!f) { ++skipped; continue; }
            f.write(reinterpret_cast<const char*>(dds.data()), static_cast<std::streamsize>(dds.size()));
            ++written;
        }
        ++pi;
    }
    std::printf("%s: wrote %zu texture(s) to %s (%zu skipped: PAL8/unknown/empty)\n",
                path.string().c_str(), written, outdir.string().c_str(), skipped);
    return 0;
}

// world <file>  — structural inventory of a world/track-stream bundle
// (sections, scenery models/instances, trigger regions, preserve-raw systems).
int CmdWorld(const fs::path& path) {
    auto bin = BINFile::Open(path);
    if (!bin) {
        std::fprintf(stderr, "error: %s\n", bin.error.c_str());
        return 1;
    }
    auto r = WorldDataParser::Parse(bin.value.Data());
    if (!r) { std::fprintf(stderr, "error: %s\n", r.error.c_str()); return 1; }
    const auto& w = r.value;
    std::printf("%s%s\n", path.string().c_str(),
                bin.value.WasDecompressed() ? " (JDLZ-decompressed)" : "");
    std::printf("  streaming sections : %zu\n", w.sections.size());
    std::printf("  scenery models     : %zu\n", w.sceneryInfos.size());
    std::printf("  scenery instances  : %zu\n", w.sceneryInstances.size());
    std::printf("  trigger regions    : %zu\n", w.triggers.size());
    std::printf("  road network (CARP): %s\n", w.roadNetworkBytes ? "present" : "absent");
    if (w.roadNetworkBytes) std::printf("                       %u bytes (preserved raw)\n", w.roadNetworkBytes);
    std::printf("  light sections     : %s\n", w.lightBytes ? "present" : "absent");
    std::printf("  weather/time chunk : %s\n", w.weatherBytes ? "present" : "absent");
    if (w.parseWarnings) std::printf("  parse warnings     : %d\n", w.parseWarnings);
    size_t show = std::min<size_t>(w.sceneryInstances.size(), 8);
    for (size_t i = 0; i < show; ++i)
        std::printf("    instance[%zu] @+0x%llX  word0=0x%08X\n", i,
                    static_cast<unsigned long long>(w.sceneryInstances[i].offset),
                    w.sceneryInstances[i].word0);
    return 0;
}

// identify <file>  — classify a file by magic / first chunk id (roadmap X2).
// Mirrors the encyclopedia's "what is this file?" decision tree so unknown
// files can be triaged from scripts.
int CmdIdentify(const fs::path& path) {
    std::vector<uint8_t> b;
    if (!LoadRaw(path, b)) {
        std::fprintf(stderr, "error: cannot read %s\n", path.string().c_str());
        return 1;
    }
    auto say = [&](const char* what) { std::printf("%s: %s\n", path.string().c_str(), what); };
    auto m4  = [&](const char* s) { return b.size() >= 4 && std::memcmp(b.data(), s, 4) == 0; };

    if (m4("JDLZ")) { say("JDLZ-compressed stream (decompress, then re-identify)"); return 0; }
    if (m4("RAWW")) { say("RAWW stored (uncompressed body after 16-byte header)");  return 0; }
    if (m4("VPAK")) { say("VPAK attribute vault");                                   return 0; }
    if (m4("DDS ")) { say("DDS texture");                                            return 0; }
    if (m4("ABKC") || m4("BNKl")) { say("EA sound bank (ABK/BNKl)");                 return 0; }
    if (m4("SCHl")) { say("MUS / EA audio stream");                                  return 0; }
    if (m4("MPFF")) { say("MPF music routing graph");                               return 0; }
    if (m4("LOCH")) { say("LOCH / memory-card UI / localized container");           return 0; }
    if (b.size() >= 2 && b[0] == 'M' && b[1] == 'Z') { say("Windows executable/DLL (PE)"); return 0; }

    if (b.size() >= 8) {
        uint32_t id; std::memcpy(&id, b.data(), 4);
        switch (id) {
            case 0xB3300000u: say("EAGL chunk tree: TPK texture pack (0xB3300000)");          return 0;
            case 0x80134000u: say("EAGL chunk tree: solid geometry (0x80134000)");            return 0;
            case 0x0003A100u: say("EAGL chunk tree: minimap tiles (0x0003A100 envelopes)");   return 0;
            case 0x00034112u:
            case 0x00034110u:
            case 0x80034100u:
            case 0x0003414Au:
            case 0x0003B800u: say("EAGL chunk tree: world / stream bundle");                  return 0;
            default: break;
        }
        if ((id & 0x80000000u) != 0) {
            std::printf("%s: EAGL chunk tree (first id 0x%08X is a container) — run 'mwcli chunks'\n",
                        path.string().c_str(), id);
            return 0;
        }
        uint32_t sz; std::memcpy(&sz, b.data() + 4, 4);
        if (sz < b.size()) {
            std::printf("%s: likely EAGL chunk tree (first id 0x%08X size %u) — run 'mwcli chunks'\n",
                        path.string().c_str(), id, sz);
            return 0;
        }
    }
    std::printf("%s: unrecognised (%zu bytes) — inspect with a hex viewer\n",
                path.string().c_str(), b.size());
    return 0;
}

void Usage() {
    std::printf(
        "MWTools CLI — headless format verifier\n"
        "usage: mwcli <command> <file>\n\n"
        "  identify   <file>   classify a file by magic / first chunk id\n"
        "  info       <file>   dump the full chunk tree\n"
        "  chunks     <file>   flat top-level chunk listing with offsets\n"
        "  jdlz       <file>   report JDLZ compression state\n"
        "  jdlzc      <in> [out]  JDLZ-compress (self-verifies; writes [out] if given)\n"
        "  tpk        <file>   list texture packs and textures\n"
        "  tpkextract <file> [outdir]  export every texture to .dds\n"
        "  solid      <file>   list geometry containers, objects, meshes\n"
        "  world      <file>   inventory sections / scenery / triggers in a world bundle\n"
        "  nisanim    <file>   list NIS cutscene animation bones (0x00E34009)\n"
        "  vpak       <file>   dump an EA Attribulator vault (attributes.bin)\n"
        "  vpakrecords<file>   emit NtaD records as JSON (offset/size/keyHash)\n"
        "  vpakdiff   <a> <b>  diff two vaults by record marker (schema discovery)\n"
        "  mod        <manifest>  parse a .mwmod, print the edit plan + conflicts (dry-run)\n"
        "  roundtrip  <file>   verify the chunk walk tiles the buffer losslessly\n");
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 2) { Usage(); return 2; }
    std::string cmd = argv[1];
    if (cmd == "-h" || cmd == "--help" || cmd == "help") { Usage(); return 0; }
    if (argc < 3) {
        std::fprintf(stderr, "error: '%s' needs a file argument\n", cmd.c_str());
        return 2;
    }
    fs::path file = argv[2];
    if (!fs::exists(file)) {
        std::fprintf(stderr, "error: no such file: %s\n", file.string().c_str());
        return 1;
    }

    if (cmd == "identify")  return CmdIdentify(file);
    if (cmd == "info")      return CmdInfo(file);
    if (cmd == "chunks")    return CmdChunks(file);
    if (cmd == "jdlz")      return CmdJdlz(file);
    if (cmd == "jdlzc")     return CmdJdlzCompress(file, argc > 3 ? fs::path(argv[3]) : fs::path());
    if (cmd == "tpk")       return CmdTpk(file);
    if (cmd == "tpkextract") return CmdTpkExtract(file, argc > 3 ? fs::path(argv[3]) : fs::path());
    if (cmd == "solid")     return CmdSolid(file);
    if (cmd == "world")     return CmdWorld(file);
    if (cmd == "nisanim")   return CmdNisAnim(file);
    if (cmd == "vpak")      return CmdVpak(file);
    if (cmd == "vpakrecords") return CmdVpakRecords(file);
    if (cmd == "vpakdiff")  return CmdVpakDiff(file, argc > 3 ? fs::path(argv[3]) : fs::path());
    if (cmd == "mod")       return CmdMod(file);
    if (cmd == "roundtrip") return CmdRoundtrip(file);

    std::fprintf(stderr, "error: unknown command '%s'\n", cmd.c_str());
    Usage();
    return 2;
}
