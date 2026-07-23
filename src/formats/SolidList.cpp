#include "formats/SolidList.h"
#include "core/Logger.h"
#include "core/StringHash.h"
#include <cstring>
#include <algorithm>

namespace nfsmw {

namespace {

/// Shading group record, 104 bytes (see SolidList.h for field map).
struct ShadingGroupRec {
    glm::vec3 bbMin{}, bbMax{};
    uint8_t   texIdx[5]{};
    uint8_t   shaderIdx = 0;
    uint32_t  texCount  = 0;
    uint32_t  fvfFlags  = 0;
    uint32_t  vertexCount = 0;
    uint32_t  triCount    = 0;
    uint32_t  firstIndex  = 0;
    uint32_t  indexCount  = 0;
};
constexpr size_t kGroupRecSize = 104;

ShadingGroupRec ReadGroup(std::span<const uint8_t> d) {
    ShadingGroupRec g;
    auto rdF = [&](size_t off) { float v; std::memcpy(&v, d.data() + off, 4); return v; };
    auto rdU = [&](size_t off) { uint32_t v; std::memcpy(&v, d.data() + off, 4); return v; };
    g.bbMin = { rdF(0), rdF(4), rdF(8) };
    g.bbMax = { rdF(12), rdF(16), rdF(20) };
    for (int i = 0; i < 5; ++i) g.texIdx[i] = d[24 + i];
    g.shaderIdx   = d[29];
    g.texCount    = rdU(48);
    g.fvfFlags    = rdU(56);
    g.vertexCount = rdU(60);
    g.triCount    = rdU(64);
    g.firstIndex  = rdU(68);
    g.indexCount  = rdU(92);
    return g;
}

/// Per-FVF vertex strides verified against retail data. Low flag bits mark
/// groups that share one vertex buffer; the base layout is unchanged.
uint32_t StrideForFVF(uint32_t fvf) {
    switch (fvf & ~0x1Au) {
        case 0x004000:  return 36;
        case 0x00C000:  return 72;  // skinned/blend data variant
        case 0x2A4000:
        case 0x224000:  return 60;
        default:        return 0; // unknown - derive from buffer size
    }
}

/// Plausible world-vertex strides (multiples of 4, position-first layouts).
constexpr uint32_t kStrideCandidates[] = { 36, 60, 24, 48, 72, 28, 32, 40, 44, 52, 56, 64, 80, 96 };

/// Assignment of a run of shading groups to one vertex buffer.
struct VBAssignment {
    size_t   firstGroup = 0;
    size_t   groupCount = 0;
    uint32_t stride     = 0;
};

/// Greedy exact-fit: starting at group `gi`, find a stride and a consecutive
/// run of groups whose summed vertex counts exactly fill `vbSize` bytes.
/// @param slack  Maximum trailing padding (bytes) to tolerate on the vertex
///               buffer when no exact fit exists. 0 = exact only (retail data).
///               Our SolidWriter rounds a rebuilt object to a 128-byte boundary,
///               which can leave <128 pad bytes on the shared VB; tolerating that
///               lets the re-saved file's object re-parse (and thus display).
bool FitGroupsToVB(const std::vector<ShadingGroupRec>& groups, size_t gi,
                   size_t vbSize, VBAssignment& out, uint32_t slack = 0) {
    // Prefer the stride hinted by the first group's FVF.
    std::vector<uint32_t> candidates;
    if (uint32_t hint = StrideForFVF(groups[gi].fvfFlags))
        candidates.push_back(hint);
    for (uint32_t s : kStrideCandidates)
        if (candidates.empty() || s != candidates[0])
            candidates.push_back(s);

    // Pass 1 — exact fit (always preferred; identical to the original behaviour).
    for (uint32_t s : candidates) {
        uint64_t sum = 0;
        for (size_t k = 0; gi + k < groups.size(); ++k) {
            sum += groups[gi + k].vertexCount;
            const uint64_t need = sum * s;
            if (need == vbSize) {
                out = { gi, k + 1, s };
                return true;
            }
            if (need > vbSize) break;
        }
    }
    // Pass 2 — tolerate up to `slack` bytes of trailing alignment padding.
    if (slack) {
        for (uint32_t s : candidates) {
            uint64_t sum = 0;
            size_t   bestK = 0;
            for (size_t k = 0; gi + k < groups.size(); ++k) {
                sum += groups[gi + k].vertexCount;
                const uint64_t need = sum * s;
                if (need > vbSize) break;
                if (vbSize - need < slack) bestK = k + 1;  // largest run within tolerance
            }
            if (bestK) { out = { gi, bestK, s }; return true; }
        }
    }
    return false;
}

void DecodeVertices(std::span<const uint8_t> vb, uint32_t stride, uint32_t count,
                    std::vector<SolidMeshVertex>& out) {
    out.resize(count);
    const bool hasNormal = stride >= 36;
    const size_t colorOff = hasNormal ? 24 : 12;
    const size_t uvOff    = hasNormal ? 28 : 16;

    for (uint32_t i = 0; i < count; ++i) {
        const uint8_t* src = vb.data() + size_t(i) * stride;
        SolidMeshVertex& v = out[i];
        std::memcpy(v.pos, src, 12);
        if (hasNormal) std::memcpy(v.normal, src + 12, 12);
        else { v.normal[0] = 0; v.normal[1] = 0; v.normal[2] = 1; }
        // D3DCOLOR is stored B,G,R,A - swizzle to RGBA.
        const uint8_t* c = src + colorOff;
        v.color[0] = c[2]; v.color[1] = c[1]; v.color[2] = c[0]; v.color[3] = c[3];
        std::memcpy(v.uv, src + uvOff, 8);
    }
}

SimSurface ClassifySurface(const std::string& mn) {
    // Issue #10 fix: avoid heap-allocating a lowercased copy for every mesh.
    // Use a case-insensitive search with a comparator projection instead.
    auto iCmp = [](unsigned char a, unsigned char b) {
        return std::tolower(a) == std::tolower(b);
    };
    auto hasL = [&](const char* needle) {
        std::string_view n(needle);
        return std::search(mn.begin(), mn.end(), n.begin(), n.end(), iCmp) != mn.end();
    };
    if (hasL("asphalt") || hasL("road")) return SimSurface::Asphalt;
    if (hasL("concrete") || hasL("cement")) return SimSurface::Concrete;
    if (hasL("grass"))   return SimSurface::Grass;
    if (hasL("dirt"))    return SimSurface::Dirt;
    if (hasL("gravel"))  return SimSurface::Gravel;
    if (hasL("sand"))    return SimSurface::Sand;
    if (hasL("water"))   return SimSurface::Water;
    if (hasL("metal"))   return SimSurface::Metal;
    if (hasL("wood"))    return SimSurface::Wood;
    if (hasL("brick"))   return SimSurface::Concrete;
    return SimSurface::Unknown;
}

void IterateChunks(std::span<const uint8_t> data,
                   const std::function<void(uint32_t, std::span<const uint8_t>, size_t)>& fn) {
    size_t off = 0;
    while (off + sizeof(ChunkHeader) <= data.size()) {
        ChunkHeader hdr{};
        std::memcpy(&hdr, data.data() + off, sizeof(hdr));
        if (off + sizeof(hdr) + hdr.size > data.size()) break;
        fn(hdr.id, data.subspan(off + sizeof(hdr), hdr.size), off + sizeof(hdr));
        off += sizeof(hdr) + hdr.size;
    }
}

} // namespace

// ----------------------------------------------------------------------------
Result<SolidList> SolidListParser::Parse(std::span<const uint8_t> payload, uint64_t absOffset) {
    SolidList list;
    int warnings = 0;

    IterateChunks(payload, [&](uint32_t id, std::span<const uint8_t> p, size_t rel) {
        switch (id) {
            case ChunkID::GeometryInfo: {
                IterateChunks(p, [&](uint32_t id2, std::span<const uint8_t> p2, size_t) {
                    if (id2 == ChunkID::GeometryInfoHeader && p2.size() > 16) {
                        // Class/file name is a C string at +16.
                        const char* s = reinterpret_cast<const char*>(p2.data() + 16);
                        size_t maxLen = p2.size() - 16;
                        list.sectionName.assign(s, strnlen(s, maxLen));
                    } else if (id2 == ChunkID::GeometryHashTable) {
                        size_t n = p2.size() / 8;
                        list.hashTable.reserve(n);
                        for (size_t i = 0; i < n; ++i) {
                            uint32_t h;
                            std::memcpy(&h, p2.data() + i * 8, 4);
                            list.hashTable.push_back(h);
                        }
                    }
                });
                break;
            }
            case ChunkID::GeometryObject: {
                auto obj = ParseObject(p, absOffset + rel, warnings);
                if (obj) {
                    list.objects.push_back(std::move(obj.value));
                } else {
                    ++warnings;
                    LOG_WARN("SolidList: object parse error: {}", obj.error);
                }
                break;
            }
            default:
                break; // null/alignment chunks
        }
    });

    list.parseWarnings = warnings;
    LOG_DEBUG("SolidList '{}': {} objects, {} verts, {} tris, {} warnings",
              list.sectionName, list.objects.size(),
              list.TotalVertexCount(), list.TotalTriCount(), warnings);
    return Result<SolidList>::Ok(std::move(list));
}

// ----------------------------------------------------------------------------
Result<SolidObject> SolidListParser::ParseObject(std::span<const uint8_t> payload,
                                                 uint64_t absOffset, int& warnings) {
    SolidObject obj;

    IterateChunks(payload, [&](uint32_t id, std::span<const uint8_t> p, size_t rel) {
        switch (id) {
            case ChunkID::GeometryObjectHeader: {
                // Strip 0x11 alignment padding, remember the patch offset.
                size_t pad = 0;
                while (pad < p.size() && p[pad] == kChunkPadByte) ++pad;
                auto d = p.subspan(pad);
                if (d.size() < 160) { ++warnings; return; }

                obj.headerFileOffset = absOffset + rel + pad;

                BinaryReader r(d);
                r.Skip(12);                       // zeros
                r.ReadU32();                      // version (0x16)
                obj.nameHash = r.ReadU32();
                obj.numTris  = r.ReadU32();
                obj.flags    = r.ReadU32();
                r.ReadU32();                      // zero
                glm::vec4 bmin = r.ReadVec4();
                glm::vec4 bmax = r.ReadVec4();
                obj.bbox.min = glm::vec3(bmin);
                obj.bbox.max = glm::vec3(bmax);
                obj.transform = r.ReadMat4();
                r.Skip(32);                       // runtime pointers / params
                // Name: remainder of the chunk, null-terminated.
                obj.name = r.ReadCString(d.size() - r.Pos()); // Issue #12 fix: removed +1 overshoot
                if (obj.name.empty()) {
                    char buf[16];
                    std::snprintf(buf, sizeof(buf), "0x%08X", obj.nameHash);
                    obj.name = buf;
                }
                HashResolver::Instance().RegisterHash(obj.nameHash, obj.name);
                break;
            }
            case ChunkID::GeometryTextureRefs: {
                size_t n = p.size() / 8;
                obj.textureHashes.reserve(n);
                for (size_t i = 0; i < n; ++i) {
                    uint32_t h;
                    std::memcpy(&h, p.data() + i * 8, 4);
                    obj.textureHashes.push_back(h);
                }
                break;
            }
            case ChunkID::GeometryMesh:
                ParseMeshContainer(p, obj, warnings);
                break;
            default:
                break;
        }
    });

    if (obj.name.empty() && obj.meshes.empty())
        return Result<SolidObject>::Err("empty 0x80134010 container");
    return Result<SolidObject>::Ok(std::move(obj));
}

// ----------------------------------------------------------------------------
void SolidListParser::ParseMeshContainer(std::span<const uint8_t> payload, SolidObject& obj,
                                         int& warnings) {
    std::vector<ShadingGroupRec>          groups;
    std::vector<std::span<const uint8_t>> vertexBuffers;
    std::span<const uint8_t>              indexData;
    std::vector<std::string>              materialNames;

    IterateChunks(payload, [&](uint32_t id, std::span<const uint8_t> p, size_t) {
        switch (id) {
            case ChunkID::MeshShadingGroups: {
                auto d = StripAlignPad(p);
                for (size_t off = 0; off + kGroupRecSize <= d.size(); off += kGroupRecSize)
                    groups.push_back(ReadGroup(d.subspan(off, kGroupRecSize)));
                break;
            }
            case ChunkID::MeshVertices:
                vertexBuffers.push_back(StripAlignPad(p));
                break;
            case ChunkID::MeshIndices:
                indexData = StripAlignPad(p);
                break;
            case ChunkID::MeshMaterialName: {
                const char* s = reinterpret_cast<const char*>(p.data());
                std::string name(s, strnlen(s, p.size()));
                if (!name.empty()) materialNames.push_back(std::move(name));
                break;
            }
            default:
                break;
        }
    });

    if (groups.empty()) return;

    const size_t totalIndices = indexData.size() / 2;

    // -- Assign vertex buffers to consecutive group runs (exact byte fit) ----
    struct GroupSlot {
        std::span<const uint8_t> vb;
        uint32_t stride      = 0;
        uint32_t vertexStart = 0;   ///< first vertex within the shared VB
        bool     valid       = false;
    };
    std::vector<GroupSlot> slots(groups.size());

    std::vector<bool> vbUsed(vertexBuffers.size(), false);
    size_t gi = 0;
    while (gi < groups.size()) {
        bool assigned = false;
        // Try every unused vertex buffer (order is usually sequential, but a
        // few retail objects interleave them).
        for (size_t vbi = 0; vbi < vertexBuffers.size() && !assigned; ++vbi) {
            if (vbUsed[vbi]) continue;
            VBAssignment assign;
            if (FitGroupsToVB(groups, gi, vertexBuffers[vbi].size(), assign)) {
                uint32_t cursor = 0;
                for (size_t k = 0; k < assign.groupCount; ++k) {
                    auto& slot = slots[gi + k];
                    slot.vb          = vertexBuffers[vbi];
                    slot.stride      = assign.stride;
                    slot.vertexStart = cursor;
                    slot.valid       = true;
                    cursor += groups[gi + k].vertexCount;
                }
                vbUsed[vbi] = true;
                gi += assign.groupCount;
                assigned = true;
            }
        }
        if (assigned) continue;

        // Second pass: accept a vertex buffer with up to one 128-byte alignment
        // unit of trailing padding. SolidWriter rounds each rebuilt object to a
        // 128-byte boundary, leaving <128 pad bytes on the shared VB; without
        // this the exact-fit test fails and the just-replaced object silently
        // vanishes from the tree. Retail files fit exactly (handled above), so
        // this only ever affects files we ourselves re-saved.
        for (size_t vbi = 0; vbi < vertexBuffers.size() && !assigned; ++vbi) {
            if (vbUsed[vbi]) continue;
            VBAssignment assign;
            if (FitGroupsToVB(groups, gi, vertexBuffers[vbi].size(), assign, 128)) {
                uint32_t cursor = 0;
                for (size_t k = 0; k < assign.groupCount; ++k) {
                    auto& slot = slots[gi + k];
                    slot.vb          = vertexBuffers[vbi];
                    slot.stride      = assign.stride;
                    slot.vertexStart = cursor;
                    slot.valid       = true;
                    cursor += groups[gi + k].vertexCount;
                }
                vbUsed[vbi] = true;
                gi += assign.groupCount;
                assigned = true;
            }
        }
        if (assigned) continue;

        // No exact fit anywhere: salvage a single group if a division works.
        const auto& g = groups[gi];
        for (size_t vbi = 0; vbi < vertexBuffers.size() && !slots[gi].valid; ++vbi) {
            if (vbUsed[vbi] || !g.vertexCount) continue;
            if (vertexBuffers[vbi].size() % g.vertexCount == 0) {
                uint32_t s = (uint32_t)(vertexBuffers[vbi].size() / g.vertexCount);
                // Issue #15 fix: document the bounds.
                // 24 = minimum plausible stride (pos[12] + uv[8] = 20, rounded up to 24).
                // 128 = maximum known stride (full FVF skinning layout).
                if (s >= 24 && s <= 128) {
                    slots[gi] = { vertexBuffers[vbi], s, 0, true };
                    vbUsed[vbi] = true;
                }
            }
        }
        if (!slots[gi].valid) {
            LOG_WARN("SolidObject '{}': no vertex-buffer fit for group {} "
                     "(fvf 0x{:X}, {} VBs, {} verts)",
                     obj.name, gi, g.fvfFlags, vertexBuffers.size(),
                     g.vertexCount);
            ++warnings;
        }
        ++gi;
    }

    // -- Build one SolidMesh per shading group --------------------------------
    for (size_t i = 0; i < groups.size(); ++i) {
        const auto& g    = groups[i];
        const auto& slot = slots[i];
        if (!slot.valid) { ++warnings; continue; }

        SolidMesh mesh;
        mesh.bbox.min     = g.bbMin;
        mesh.bbox.max     = g.bbMax;
        mesh.fvfFlags     = g.fvfFlags;
        mesh.strideInFile = slot.stride;

        const size_t availVerts = slot.vb.size() / slot.stride;
        uint32_t count = g.vertexCount;
        if (slot.vertexStart + count > availVerts) {
            count = (uint32_t)(availVerts > slot.vertexStart ? availVerts - slot.vertexStart : 0);
            ++warnings;
        }
        DecodeVertices(slot.vb.subspan(size_t(slot.vertexStart) * slot.stride,
                                       size_t(count) * slot.stride),
                       slot.stride, count, mesh.vertices);

        // -- Resolve index range --
        size_t first  = g.firstIndex;
        size_t icount = g.indexCount ? g.indexCount : size_t(g.triCount) * 3;
        if (first + icount > totalIndices) {
            icount = totalIndices > first ? totalIndices - first : 0;
            ++warnings;
        }
        mesh.indices.resize(icount);
        if (icount)
            std::memcpy(mesh.indices.data(), indexData.data() + first * 2, icount * 2);

        // Indices may be group-relative or VB-absolute; normalise to relative.
        if (!mesh.indices.empty() && !mesh.vertices.empty()) {
            uint16_t maxIdx = 0, minIdx = 0xFFFF;
            for (uint16_t idx : mesh.indices) {
                maxIdx = std::max(maxIdx, idx);
                minIdx = std::min(minIdx, idx);
            }
            if (maxIdx >= mesh.vertices.size() && slot.vertexStart > 0 &&
                minIdx >= slot.vertexStart &&
                maxIdx < slot.vertexStart + mesh.vertices.size()) {
                for (auto& idx : mesh.indices)
                    idx = (uint16_t)(idx - slot.vertexStart);
                maxIdx = (uint16_t)(maxIdx - slot.vertexStart);
            }
            if (maxIdx >= mesh.vertices.size()) {
                mesh.indexOutOfRange = true;
                ++warnings;
            }
        }

        // -- Texture / material --
        if (g.texIdx[0] < obj.textureHashes.size())
            mesh.textureHash = obj.textureHashes[g.texIdx[0]];
        if (i < materialNames.size())
            mesh.materialName = materialNames[i];
        else if (!materialNames.empty())
            mesh.materialName = materialNames.front();
        mesh.surface = ClassifySurface(mesh.materialName);

        obj.meshes.push_back(std::move(mesh));
    }
}

} // namespace nfsmw
