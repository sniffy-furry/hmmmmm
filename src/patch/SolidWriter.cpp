#include "patch/SolidWriter.h"
#include "core/Logger.h"
#include "core/LZCDecompressor.h"

#include <fstream>
#include <cstring>
#include <cstdint>
#include <algorithm>
#include <array>

namespace nfsmw {
namespace {

constexpr uint32_t kGeometryContainer = 0x80134000;
constexpr uint32_t kGeometryObject    = 0x80134010;
constexpr uint32_t kObjectHeader      = 0x00134011;
constexpr uint32_t kMeshContainer     = 0x80134100;
constexpr uint32_t kMeshHeader        = 0x00134900;
constexpr uint32_t kMeshGroups        = 0x00134B02;
constexpr uint32_t kMeshIndices       = 0x00134B03;
constexpr uint32_t kMeshVertices      = 0x00134B01;
constexpr uint32_t kMeshMaterialName  = 0x00134C02;
constexpr uint8_t  kPad = 0x11;
constexpr size_t   kGroupRec = 104;

uint32_t RU32(const uint8_t* p) { uint32_t v; std::memcpy(&v, p, 4); return v; }
void     WU32(uint8_t* p, uint32_t v) { std::memcpy(p, &v, 4); }

void PutU32(std::vector<uint8_t>& b, uint32_t v) {
    b.push_back(uint8_t(v)); b.push_back(uint8_t(v >> 8));
    b.push_back(uint8_t(v >> 16)); b.push_back(uint8_t(v >> 24));
}
void PutF32(std::vector<uint8_t>& b, float f) {
    uint32_t v; std::memcpy(&v, &f, 4); PutU32(b, v);
}

/// One original group's preserved shader/texture metadata.
struct OrigGroupMeta {
    std::array<uint8_t, 5> texIdx{ {0,0,0,0,0} };
    uint8_t  shaderIdx = 0;
    uint32_t texCount  = 1;
    uint32_t fvfFlags  = 0x4180;
};

/// Walk a flat chunk sequence, invoking fn(id, headerOffset, payloadOffset, size).
template <typename F>
void Walk(const uint8_t* buf, size_t begin, size_t end, F&& fn) {
    size_t off = begin;
    while (off + 8 <= end) {
        uint32_t id = RU32(buf + off), size = RU32(buf + off + 4);
        if (off + 8 + size > end) break;
        fn(id, off, off + 8, size);
        off += 8 + size;
    }
}

size_t LeadingPad(const uint8_t* p, size_t n) {
    size_t i = 0; while (i < n && p[i] == kPad) ++i; return i;
}

} // namespace

Result<void> SolidWriter::ReplaceObject(const std::filesystem::path& path,
                                        uint32_t nameHash, const ImportModel& model,
                                        BackupManager& bm) {
    if (model.meshes.empty())
        return Result<void>::Err("Imported model has no meshes");
    for (const auto& m : model.meshes)
        if (m.vertices.size() > 0xFFFF)
            return Result<void>::Err("A sub-mesh exceeds 65535 vertices "
                                     "(u16 index limit) — split it before import");
    // All sub-meshes are concatenated into ONE shared, 16-bit-indexed vertex
    // buffer and the indices we emit are VB-absolute (see below), so the *total*
    // vertex count — not just each sub-mesh — must stay within u16 range. Without
    // this guard the absolute indices wrap past 65535 and the game reads garbage
    // vertices → crash.
    {
        size_t totalVerts = 0;
        for (const auto& m : model.meshes) totalVerts += m.vertices.size();
        if (totalVerts > 0x10000)
            return Result<void>::Err(
                "Imported model has " + std::to_string(totalVerts) +
                " vertices total; one object shares a single 16-bit-indexed vertex "
                "buffer (max 65536). Split it before import");
    }

    // ── Read file (decompress JDLZ in memory if needed) ──────────────────────
    std::ifstream in(path, std::ios::binary | std::ios::ate);
    if (!in) return Result<void>::Err("Cannot open '" + path.string() + "'");
    std::vector<uint8_t> raw(size_t(in.tellg()));
    in.seekg(0);
    in.read(reinterpret_cast<char*>(raw.data()), raw.size());
    in.close();

    // JDLZ-compressed files are decompressed, edited, and written back
    // UNCOMPRESSED — the game's loader reads raw chunks when the 'JDLZ' magic is
    // absent, and we have no JDLZ encoder. The original (compressed) file is
    // preserved in the .bak.
    std::vector<uint8_t> buf;
    bool wasCompressed = false;
    if (raw.size() >= 4 && std::memcmp(raw.data(), "JDLZ", 4) == 0) {
        auto dr = LZCDecompressor::Decompress(raw);
        if (!dr) return Result<void>::Err("JDLZ decompress failed: " + dr.error);
        buf = std::move(dr.value);
        wasCompressed = true;
    } else {
        buf = std::move(raw);
    }
    const size_t fileSize = buf.size();

    // ── Locate the target object chunk (and its ancestor containers) ──────────
    size_t objHdr = SIZE_MAX, objPayload = 0, objSize = 0;
    std::vector<size_t> ancestorHdrs; // container headers whose payload holds objHdr

    // Recursive descent collecting container ancestors as we find the object.
    struct Finder {
        const uint8_t* buf;
        uint32_t targetHash;
        size_t objHdr = SIZE_MAX, objPayload = 0, objSize = 0;
        std::vector<size_t> ancestors;

        void Descend(size_t begin, size_t end, std::vector<size_t>& stack) {
            Walk(buf, begin, end, [&](uint32_t id, size_t hOff, size_t pOff, size_t sz) {
                if (objHdr != SIZE_MAX) return;
                if (id == kGeometryObject) {
                    // Read its 0x00134011 nameHash to test the match.
                    uint32_t h = 0; bool found = false;
                    Walk(buf, pOff, pOff + sz, [&](uint32_t id2, size_t, size_t p2, size_t s2) {
                        if (id2 == kObjectHeader && !found) {
                            size_t pad = LeadingPad(buf + p2, s2);
                            if (s2 - pad >= 20) { h = RU32(buf + p2 + pad + 16); found = true; }
                        }
                    });
                    if (found && h == targetHash) {
                        objHdr = hOff; objPayload = pOff; objSize = sz;
                        ancestors = stack;
                    }
                } else if (id & 0x80000000u) {
                    stack.push_back(hOff);
                    Descend(pOff, pOff + sz, stack);
                    stack.pop_back();
                }
            });
        }
    } finder{ buf.data(), nameHash };
    std::vector<size_t> stack;
    finder.Descend(0, fileSize, stack);
    if (finder.objHdr == SIZE_MAX)
        return Result<void>::Err("Object not found in file (hash mismatch)");
    objHdr = finder.objHdr; objPayload = finder.objPayload; objSize = finder.objSize;
    ancestorHdrs = finder.ancestors;

    // ── Inventory the object's sub-chunks (preserve order) ────────────────────
    struct Sub { uint32_t id; size_t payload; size_t size; };
    std::vector<Sub> subs;
    std::vector<OrigGroupMeta> origGroups;
    uint8_t  meshHeaderTemplate[48] = {0};
    bool     haveMeshHeader = false;
    size_t   origTotalVerts = 0;   // sum of original group vertexCounts
    size_t   origVBdataLen  = 0;   // 0x00134B01 payload bytes after leading pad
    Walk(buf.data(), objPayload, objPayload + objSize,
         [&](uint32_t id, size_t, size_t pOff, size_t sz) {
        subs.push_back({ id, pOff, sz });
        if (id == kMeshContainer) {
            Walk(buf.data(), pOff, pOff + sz, [&](uint32_t id2, size_t, size_t p2, size_t s2) {
                if (id2 == kMeshHeader && s2 >= 48) {
                    std::memcpy(meshHeaderTemplate, buf.data() + p2, 48);
                    haveMeshHeader = true;
                } else if (id2 == kMeshGroups) {
                    size_t pad = LeadingPad(buf.data() + p2, s2);
                    for (size_t o = p2 + pad; o + kGroupRec <= p2 + s2; o += kGroupRec) {
                        OrigGroupMeta g;
                        for (int i = 0; i < 5; ++i) g.texIdx[i] = buf[o + 24 + i];
                        g.shaderIdx = buf[o + 29];
                        g.texCount  = RU32(buf.data() + o + 48);
                        g.fvfFlags  = RU32(buf.data() + o + 56);
                        origGroups.push_back(g);
                        origTotalVerts += RU32(buf.data() + o + 60);
                    }
                } else if (id2 == kMeshVertices) {
                    origVBdataLen = s2 - LeadingPad(buf.data() + p2, s2);
                }
            });
        }
    });

    // Original vertex stride, recovered from the retail VB size / vertex count.
    // The game derives the read stride from each group's FVF; if we kept the
    // original shaderIdx but rewrote a 36-byte buffer, a shader expecting the
    // 60-byte (tangent) layout would read past each vertex → garbage / crash.
    // So we re-emit at the *original* stride (our 36 data bytes share the same
    // pos/normal/color/uv offsets; trailing tangent bytes are zero-filled) and
    // preserve the original FVF, keeping stride ⋄ FVF ⋄ shader consistent.
    size_t origStride = 0;
    if (origTotalVerts > 0 && origVBdataLen >= origTotalVerts)
        origStride = origVBdataLen / origTotalVerts;

    // ── Compute object bbox over all imported vertices ────────────────────────
    float bbMin[3] = { 1e30f, 1e30f, 1e30f }, bbMax[3] = { -1e30f, -1e30f, -1e30f };
    for (const auto& m : model.meshes)
        for (const auto& v : m.vertices)
            for (int c = 0; c < 3; ++c) {
                bbMin[c] = std::min(bbMin[c], v.pos[c]);
                bbMax[c] = std::max(bbMax[c], v.pos[c]);
            }
    uint32_t totalTris = uint32_t(model.TotalTriangles());

    const bool reuseGroups = (origGroups.size() == model.meshes.size());

    // Choose the vertex stride to write. Prefer the original object's stride so a
    // preserved shaderIdx still finds the vertex layout it expects. Our normalised
    // vertex carries 36 meaningful bytes (pos/normal/color/uv); a larger original
    // stride (e.g. 60B with tangents) is honoured by zero-padding the tail.
    const size_t kBaseVertex = 36;
    size_t writeStride = kBaseVertex;
    if (!origGroups.empty() && origStride >= kBaseVertex)
        writeStride = origStride;
    // Fallback shader/texture template for the "different model" case where the
    // imported group count doesn't match: reuse the first original group's shader
    // so the rebuilt object still references a valid shader from this section.
    const bool haveOrigTemplate = !origGroups.empty();

    // ── Build the new object payload, tracking absolute offsets for alignment ─
    std::vector<uint8_t> obj;                 // bytes after the 0x80134010 header
    const size_t baseAbs = objHdr + 8;        // absolute offset of obj[0]
    auto curAbs = [&] { return baseAbs + obj.size(); };

    size_t vbChunkHdrPos = SIZE_MAX, vbDataEndPos = SIZE_MAX, meshContHdrPos = SIZE_MAX;

    auto emitChunk = [&](uint32_t id, const std::vector<uint8_t>& data, size_t align) {
        const size_t dataAbs = curAbs() + 8;
        size_t pad = (align > 1) ? (align - (dataAbs % align)) % align : 0;
        PutU32(obj, id);
        PutU32(obj, uint32_t(pad + data.size()));
        obj.insert(obj.end(), pad, kPad);
        obj.insert(obj.end(), data.begin(), data.end());
    };

    for (const auto& s : subs) {
        if (s.id == kMeshContainer) {
            // Rebuild the mesh container in place (children need correct abs offsets).
            meshContHdrPos = obj.size();
            PutU32(obj, kMeshContainer);
            PutU32(obj, 0);                    // size placeholder, backpatched below
            const size_t contDataStart = obj.size();

            // 0x00134900 mesh header (copy template, patch counts)
            std::vector<uint8_t> mh(meshHeaderTemplate, meshHeaderTemplate + 48);
            if (!haveMeshHeader) {
                std::fill(mh.begin(), mh.end(), 0);
                WU32(mh.data() + 8, 0x12); WU32(mh.data() + 12, 0x14180);
            }
            // Keep the header FVF consistent with the stride we emit. If we fell
            // back to a 36-byte vertex, advertise the 36-byte layout here too.
            if (writeStride == kBaseVertex) WU32(mh.data() + 12, 0x14180);
            WU32(mh.data() + 16, uint32_t(model.meshes.size())); // numGroups
            WU32(mh.data() + 24, 1);                             // numVertexBuffers
            WU32(mh.data() + 44, uint32_t([&]{ size_t n=0; for (auto& m:model.meshes) n+=m.indices.size(); return n; }()));
            emitChunk(kMeshHeader, mh, 0);

            // 0x00134B02 shading groups
            std::vector<uint8_t> groups;
            uint32_t runningIndex = 0;
            for (size_t i = 0; i < model.meshes.size(); ++i) {
                const auto& m = model.meshes[i];
                float gmn[3] = { 1e30f,1e30f,1e30f }, gmx[3] = { -1e30f,-1e30f,-1e30f };
                for (const auto& v : m.vertices)
                    for (int c = 0; c < 3; ++c) {
                        gmn[c] = std::min(gmn[c], v.pos[c]); gmx[c] = std::max(gmx[c], v.pos[c]);
                    }
                std::vector<uint8_t> g(kGroupRec, 0);
                std::memcpy(g.data() + 0, gmn, 12);
                std::memcpy(g.data() + 12, gmx, 12);
                OrigGroupMeta meta;
                if (reuseGroups)            meta = origGroups[i];  // texIdx/shader/fvf
                else if (haveOrigTemplate)  meta = origGroups[0];  // best-effort shader reuse
                // The advertised FVF must match the stride we actually write. We
                // keep the original FVF only when we're emitting at the original
                // stride; otherwise we wrote a plain 36-byte vertex, so advertise
                // the 36-byte layout to stay consistent with the shaderIdx.
                if (writeStride == kBaseVertex) meta.fvfFlags = 0x4180;
                for (int k = 0; k < 5; ++k) g[24 + k] = meta.texIdx[k];
                g[29] = meta.shaderIdx;
                WU32(g.data() + 48, meta.texCount);
                WU32(g.data() + 56, meta.fvfFlags);
                WU32(g.data() + 60, uint32_t(m.vertices.size()));
                WU32(g.data() + 64, uint32_t(m.indices.size() / 3));
                WU32(g.data() + 68, runningIndex);
                WU32(g.data() + 92, uint32_t(m.indices.size()));
                groups.insert(groups.end(), g.begin(), g.end());
                runningIndex += uint32_t(m.indices.size());
            }
            emitChunk(kMeshGroups, groups, 16);

            // 0x00134B03 index buffer. The game indexes the single shared vertex
            // buffer with VB-ABSOLUTE indices (SolidList.cpp detects this and
            // subtracts each group's vertexStart to normalise on read). The
            // importer hands us group-relative (0-based per sub-mesh) indices, so
            // we add each sub-mesh's running vertex base here. Writing the raw
            // group-relative values made every group after the first reference
            // group 0's vertices → corrupt geometry / crash in-game.
            std::vector<uint8_t> idx;
            uint32_t vbBase = 0;
            for (const auto& m : model.meshes) {
                for (uint16_t v : m.indices) {
                    const uint32_t a = uint32_t(v) + vbBase;
                    idx.push_back(uint8_t(a)); idx.push_back(uint8_t(a >> 8));
                }
                vbBase += uint32_t(m.vertices.size());
            }
            emitChunk(kMeshIndices, idx, 16);

            // 0x00134B01 single shared vertex buffer (pos, normal, color BGRA, uv).
            // Written at `writeStride`: the first 36 bytes match the retail 36- and
            // 60-byte layouts (pos@0, normal@12, color@24, uv@28); any extra bytes
            // up to the original stride (e.g. tangents @36) are zero-filled.
            std::vector<uint8_t> vb;
            for (const auto& m : model.meshes)
                for (const auto& v : m.vertices) {
                    const size_t vStart = vb.size();
                    PutF32(vb, v.pos[0]); PutF32(vb, v.pos[1]); PutF32(vb, v.pos[2]);
                    PutF32(vb, v.normal[0]); PutF32(vb, v.normal[1]); PutF32(vb, v.normal[2]);
                    vb.push_back(v.color[2]); vb.push_back(v.color[1]);   // RGBA -> BGRA
                    vb.push_back(v.color[0]); vb.push_back(v.color[3]);
                    PutF32(vb, v.uv[0]); PutF32(vb, v.uv[1]);
                    vb.resize(vStart + writeStride, 0);   // zero-pad to stride
                }
            vbChunkHdrPos = obj.size();
            emitChunk(kMeshVertices, vb, 128);
            vbDataEndPos = obj.size();

            // 0x00134C02 material names (one per group, in order)
            for (const auto& m : model.meshes) {
                std::vector<uint8_t> nb(m.materialName.begin(), m.materialName.end());
                nb.push_back(0);
                while (nb.size() % 4) nb.push_back(0);
                emitChunk(kMeshMaterialName, nb, 0);
            }

            WU32(obj.data() + meshContHdrPos + 4, uint32_t(obj.size() - contDataStart));
        } else {
            // Preserve verbatim (copy header + payload), patching only 0x00134011.
            const size_t hdr = s.payload - 8;
            obj.insert(obj.end(), buf.begin() + hdr, buf.begin() + s.payload + s.size);
            if (s.id == kObjectHeader) {
                const size_t base = obj.size() - s.size; // payload start in obj
                size_t pad = LeadingPad(obj.data() + base, s.size);
                if (s.size - pad >= 64) {
                    WU32(obj.data() + base + pad + 20, totalTris);
                    for (int c = 0; c < 3; ++c) {
                        std::memcpy(obj.data() + base + pad + 32 + c * 4, &bbMin[c], 4);
                        std::memcpy(obj.data() + base + pad + 48 + c * 4, &bbMax[c], 4);
                    }
                }
            }
        }
    }

    // ── Round the size delta to a multiple of 128 so trailing sub-chunks keep
    //    their alignment (16- and 128-aligned chunks both survive a 128 shift). ─
    const size_t oldPayload = objSize;
    long long delta = (long long)obj.size() - (long long)oldPayload;
    long long roundPad = ((128 - (delta % 128)) % 128 + 128) % 128;
    if (roundPad && vbDataEndPos != SIZE_MAX) {
        obj.insert(obj.begin() + vbDataEndPos, size_t(roundPad), kPad);
        WU32(obj.data() + vbChunkHdrPos + 4, RU32(obj.data() + vbChunkHdrPos + 4) + uint32_t(roundPad));
        WU32(obj.data() + meshContHdrPos + 4, RU32(obj.data() + meshContHdrPos + 4) + uint32_t(roundPad));
    }
    const long long sizeDelta = (long long)obj.size() - (long long)oldPayload;

    // ── Splice the rebuilt object chunk back into the file ────────────────────
    std::vector<uint8_t> out;
    out.reserve(buf.size() + size_t(sizeDelta > 0 ? sizeDelta : 0));
    out.insert(out.end(), buf.begin(), buf.begin() + objHdr);
    PutU32(out, kGeometryObject);
    PutU32(out, uint32_t(obj.size()));
    out.insert(out.end(), obj.begin(), obj.end());
    out.insert(out.end(), buf.begin() + objPayload + objSize, buf.end());

    // ── Fix ancestor container sizes (their header offsets precede objHdr) ────
    for (size_t h : ancestorHdrs)
        WU32(out.data() + h + 4, uint32_t(int64_t(RU32(out.data() + h + 4)) + sizeDelta));

    // ── Back up once, then write atomically (temp + replace) ──────────────────
    bm.EnsureFileBak(path);
    const auto tmp = std::filesystem::path(path.string() + ".tmp");
    {
        std::ofstream o(tmp, std::ios::binary | std::ios::trunc);
        if (!o) return Result<void>::Err("Cannot write temp file");
        o.write(reinterpret_cast<const char*>(out.data()), out.size());
    }
    std::error_code ec;
    std::filesystem::rename(tmp, path, ec);
    if (ec) {
        std::filesystem::remove(tmp, ec);
        return Result<void>::Err("Could not replace original file: " + ec.message());
    }

    LOG_INFO("SolidWriter: replaced object 0x{:08X} in {} ({} meshes, {} verts, {} tris, "
             "stride {} (orig {}), groups {}, delta {}, source {})",
             nameHash, path.filename().string(), model.meshes.size(),
             model.TotalVertices(), totalTris, writeStride, origStride,
             reuseGroups ? "reused" : (haveOrigTemplate ? "templated" : "default"),
             sizeDelta,
             wasCompressed ? "JDLZ->written uncompressed" : "uncompressed");
    return Result<void>::Ok();
}

// ─── Flag-only in-place patch ───────────────────────────────────────────────
Result<void> SolidWriter::PatchObjectFlags(const std::filesystem::path& path,
                                           uint32_t nameHash, uint32_t newFlags,
                                           BackupManager& bm,
                                           const uint32_t* expectedOld) {
    // ── Read file (decompress JDLZ in memory if needed) ──────────────────────
    std::ifstream in(path, std::ios::binary | std::ios::ate);
    if (!in) return Result<void>::Err("Cannot open '" + path.string() + "'");
    std::vector<uint8_t> raw(size_t(in.tellg()));
    in.seekg(0);
    in.read(reinterpret_cast<char*>(raw.data()), raw.size());
    in.close();

    std::vector<uint8_t> buf;
    bool wasCompressed = false;
    if (raw.size() >= 4 && std::memcmp(raw.data(), "JDLZ", 4) == 0) {
        auto dr = LZCDecompressor::Decompress(raw);
        if (!dr) return Result<void>::Err("JDLZ decompress failed: " + dr.error);
        buf = std::move(dr.value);
        wasCompressed = true;
    } else {
        buf = std::move(raw);
    }
    const size_t fileSize = buf.size();

    // ── Locate the target object's 0x00134011 header and its flags field ──────
    // flags sit at +24 within the (post-pad) 0x00134011 payload:
    //   +0 zeros[12] · +12 version · +16 nameHash · +20 numTris · +24 flags.
    size_t flagsPos = SIZE_MAX;
    struct Finder {
        const uint8_t* buf;
        uint32_t targetHash;
        size_t flagsPos = SIZE_MAX;
        void Descend(size_t begin, size_t end) {
            Walk(buf, begin, end, [&](uint32_t id, size_t, size_t pOff, size_t sz) {
                if (flagsPos != SIZE_MAX) return;
                if (id == kGeometryObject) {
                    Walk(buf, pOff, pOff + sz, [&](uint32_t id2, size_t, size_t p2, size_t s2) {
                        if (flagsPos != SIZE_MAX) return;
                        if (id2 == kObjectHeader) {
                            size_t pad = LeadingPad(buf + p2, s2);
                            if (s2 - pad >= 28 &&
                                RU32(buf + p2 + pad + 16) == targetHash)
                                flagsPos = p2 + pad + 24;
                        }
                    });
                } else if (id & 0x80000000u) {
                    Descend(pOff, pOff + sz);
                }
            });
        }
    } finder{ buf.data(), nameHash };
    finder.Descend(0, fileSize);
    if (finder.flagsPos == SIZE_MAX)
        return Result<void>::Err("Object not found in file (hash mismatch)");
    flagsPos = finder.flagsPos;

    const uint32_t curFlags = RU32(buf.data() + flagsPos);
    if (expectedOld && *expectedOld != curFlags) {
        char msg[128];
        std::snprintf(msg, sizeof msg,
            "On-disk flags 0x%08X differ from expected 0x%08X — file changed; "
            "reload before saving.", curFlags, *expectedOld);
        return Result<void>::Err(msg);
    }
    if (curFlags == newFlags && !wasCompressed)
        return Result<void>::Ok();   // nothing to do (and no recompress needed)

    WU32(buf.data() + flagsPos, newFlags);

    // ── Back up once, then write atomically (temp + replace) ──────────────────
    bm.EnsureFileBak(path);
    const auto tmp = std::filesystem::path(path.string() + ".tmp");
    {
        std::ofstream o(tmp, std::ios::binary | std::ios::trunc);
        if (!o) return Result<void>::Err("Cannot write temp file");
        o.write(reinterpret_cast<const char*>(buf.data()), buf.size());
    }
    std::error_code ec;
    std::filesystem::rename(tmp, path, ec);
    if (ec) {
        std::filesystem::remove(tmp, ec);
        return Result<void>::Err("Could not replace original file: " + ec.message());
    }

    LOG_INFO("SolidWriter: patched flags of object 0x{:08X} in {} (0x{:08X} -> 0x{:08X}, {})",
             nameHash, path.filename().string(), curFlags, newFlags,
             wasCompressed ? "JDLZ->written uncompressed" : "in-place");
    return Result<void>::Ok();
}

// ─── Texture-slot in-place patch ────────────────────────────────────────────
Result<void> SolidWriter::PatchObjectTexture(const std::filesystem::path& path,
                                             uint32_t nameHash, uint32_t slotIndex,
                                             uint32_t newTexHash, BackupManager& bm,
                                             const uint32_t* expectedOld) {
    // 0x00134012 texture-reference table: { u32 nameHash, u32 pad } per entry.

    // ── Read file (decompress JDLZ in memory if needed) ──────────────────────
    std::ifstream in(path, std::ios::binary | std::ios::ate);
    if (!in) return Result<void>::Err("Cannot open '" + path.string() + "'");
    std::vector<uint8_t> raw(size_t(in.tellg()));
    in.seekg(0);
    in.read(reinterpret_cast<char*>(raw.data()), raw.size());
    in.close();

    std::vector<uint8_t> buf;
    bool wasCompressed = false;
    if (raw.size() >= 4 && std::memcmp(raw.data(), "JDLZ", 4) == 0) {
        auto dr = LZCDecompressor::Decompress(raw);
        if (!dr) return Result<void>::Err("JDLZ decompress failed: " + dr.error);
        buf = std::move(dr.value);
        wasCompressed = true;
    } else {
        buf = std::move(raw);
    }
    const size_t fileSize = buf.size();

    // ── Locate the object, then its 0x00134012 table entry `slotIndex` ────────
    size_t texPos = SIZE_MAX;
    struct Finder {
        const uint8_t* buf;
        uint32_t targetHash;
        uint32_t slot;
        size_t texPos = SIZE_MAX;
        void Descend(size_t begin, size_t end) {
            Walk(buf, begin, end, [&](uint32_t id, size_t, size_t pOff, size_t sz) {
                if (texPos != SIZE_MAX) return;
                if (id == kGeometryObject) {
                    // Confirm this object's name-hash, then find its tex table.
                    bool match = false;
                    Walk(buf, pOff, pOff + sz, [&](uint32_t id2, size_t, size_t p2, size_t s2) {
                        if (id2 == kObjectHeader) {
                            size_t pad = LeadingPad(buf + p2, s2);
                            if (s2 - pad >= 20 && RU32(buf + p2 + pad + 16) == targetHash)
                                match = true;
                        }
                    });
                    if (!match) return;
                    Walk(buf, pOff, pOff + sz, [&](uint32_t id2, size_t, size_t p2, size_t s2) {
                        if (id2 == 0x00134012 && texPos == SIZE_MAX) {
                            const size_t off = size_t(slot) * 8;
                            if (off + 4 <= s2) texPos = p2 + off;
                        }
                    });
                } else if (id & 0x80000000u) {
                    Descend(pOff, pOff + sz);
                }
            });
        }
    } finder{ buf.data(), nameHash, slotIndex };
    finder.Descend(0, fileSize);
    if (finder.texPos == SIZE_MAX)
        return Result<void>::Err("Object or texture slot not found (hash/slot mismatch)");
    texPos = finder.texPos;

    const uint32_t curTex = RU32(buf.data() + texPos);
    if (expectedOld && *expectedOld != curTex) {
        char msg[128];
        std::snprintf(msg, sizeof msg,
            "On-disk texture 0x%08X differs from expected 0x%08X — file changed; "
            "reload before saving.", curTex, *expectedOld);
        return Result<void>::Err(msg);
    }
    if (curTex == newTexHash && !wasCompressed)
        return Result<void>::Ok();

    WU32(buf.data() + texPos, newTexHash);

    // ── Back up once, then write atomically (temp + replace) ──────────────────
    bm.EnsureFileBak(path);
    const auto tmp = std::filesystem::path(path.string() + ".tmp");
    {
        std::ofstream o(tmp, std::ios::binary | std::ios::trunc);
        if (!o) return Result<void>::Err("Cannot write temp file");
        o.write(reinterpret_cast<const char*>(buf.data()), buf.size());
    }
    std::error_code ec;
    std::filesystem::rename(tmp, path, ec);
    if (ec) {
        std::filesystem::remove(tmp, ec);
        return Result<void>::Err("Could not replace original file: " + ec.message());
    }

    LOG_INFO("SolidWriter: retextured object 0x{:08X} slot {} in {} (0x{:08X} -> 0x{:08X}, {})",
             nameHash, slotIndex, path.filename().string(), curTex, newTexHash,
             wasCompressed ? "JDLZ->written uncompressed" : "in-place");
    return Result<void>::Ok();
}

// ─── Transform in-place patch ───────────────────────────────────────────────
Result<void> SolidWriter::PatchObjectTransform(const std::filesystem::path& path,
                                               uint32_t nameHash,
                                               const float* xform16,
                                               BackupManager& bm) {
    if (!xform16) return Result<void>::Err("null transform");

    std::ifstream in(path, std::ios::binary | std::ios::ate);
    if (!in) return Result<void>::Err("Cannot open '" + path.string() + "'");
    std::vector<uint8_t> raw(size_t(in.tellg()));
    in.seekg(0);
    in.read(reinterpret_cast<char*>(raw.data()), raw.size());
    in.close();

    std::vector<uint8_t> buf;
    bool wasCompressed = false;
    if (raw.size() >= 4 && std::memcmp(raw.data(), "JDLZ", 4) == 0) {
        auto dr = LZCDecompressor::Decompress(raw);
        if (!dr) return Result<void>::Err("JDLZ decompress failed: " + dr.error);
        buf = std::move(dr.value);
        wasCompressed = true;
    } else {
        buf = std::move(raw);
    }
    const size_t fileSize = buf.size();

    // Transform is 64 bytes at +64 within the (post-pad) 0x00134011 payload:
    //   +0 zeros[12] +12 version +16 nameHash +20 numTris +24 flags +28 zero
    //   +32 bboxMin(v4) +48 bboxMax(v4) +64 transform(mat4).
    size_t xfPos = SIZE_MAX;
    struct Finder {
        const uint8_t* buf; uint32_t targetHash; size_t xfPos = SIZE_MAX;
        void Descend(size_t begin, size_t end) {
            Walk(buf, begin, end, [&](uint32_t id, size_t, size_t pOff, size_t sz) {
                if (xfPos != SIZE_MAX) return;
                if (id == kGeometryObject) {
                    Walk(buf, pOff, pOff + sz, [&](uint32_t id2, size_t, size_t p2, size_t s2) {
                        if (xfPos != SIZE_MAX) return;
                        if (id2 == kObjectHeader) {
                            size_t pad = LeadingPad(buf + p2, s2);
                            if (s2 - pad >= 128 && RU32(buf + p2 + pad + 16) == targetHash)
                                xfPos = p2 + pad + 64;
                        }
                    });
                } else if (id & 0x80000000u) {
                    Descend(pOff, pOff + sz);
                }
            });
        }
    } finder{ buf.data(), nameHash };
    finder.Descend(0, fileSize);
    if (finder.xfPos == SIZE_MAX)
        return Result<void>::Err("Object not found in file (hash mismatch)");
    xfPos = finder.xfPos;

    std::memcpy(buf.data() + xfPos, xform16, 64);

    bm.EnsureFileBak(path);
    const auto tmp = std::filesystem::path(path.string() + ".tmp");
    {
        std::ofstream o(tmp, std::ios::binary | std::ios::trunc);
        if (!o) return Result<void>::Err("Cannot write temp file");
        o.write(reinterpret_cast<const char*>(buf.data()), buf.size());
    }
    std::error_code ec;
    std::filesystem::rename(tmp, path, ec);
    if (ec) {
        std::filesystem::remove(tmp, ec);
        return Result<void>::Err("Could not replace original file: " + ec.message());
    }
    LOG_INFO("SolidWriter: patched transform of object 0x{:08X} in {} ({})",
             nameHash, path.filename().string(),
             wasCompressed ? "JDLZ->written uncompressed" : "in-place");
    return Result<void>::Ok();
}

} // namespace nfsmw
