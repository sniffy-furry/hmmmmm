// ─── editor/GLTFExporter.cpp ─────────────────────────────────────────────────
// Self-contained binary glTF (.glb) export for SolidObjects.
//
// glTF 2.0 is the modern interchange format that both Blender (File > Import >
// glTF 2.0) and 3ds Max 2020+ import natively, with materials and textures in a
// single file. In NFSMW the render geometry *is* the collision geometry (the
// game collides against the visual triangles and resolves the surface type at
// runtime — see collision-export-investigation), so exporting the full mesh and
// tagging each material with its SimSurface type is how an object's collision
// travels with it.
//
//   • geometry   → POSITION / NORMAL / TEXCOORD_0 + indices, one primitive per
//                  shading group
//   • textures   → DXT decoded to RGBA, re-encoded as PNG (Windows WIC) and
//                  embedded in the .glb buffer
//   • collision  → each material carries its SimSurface name in `name` and in
//                  `extras.simSurface`
//   • axes       → game Z-up converted to glTF Y-up so it lands upright
// ─────────────────────────────────────────────────────────────────────────────
#include "editor/Exporter.h"
#include "core/Logger.h"
#include "core/StringHash.h"
#include "formats/StreamBundle.h"

#include <nlohmann/json.hpp>
#include <fstream>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <string>
#include <vector>

#ifdef _WIN32
#  define WIN32_LEAN_AND_MEAN
#  define NOMINMAX
#  include <windows.h>
#  include <wincodec.h>
#endif

namespace nfsmw {
namespace {

using json = nlohmann::json;

// ─── Texture decode (DXT/ARGB → BGRA top mip) ────────────────────────────────

inline uint32_t PackBGRA(uint8_t b, uint8_t g, uint8_t r, uint8_t a) {
    return uint32_t(b) | (uint32_t(g) << 8) | (uint32_t(r) << 16) | (uint32_t(a) << 24);
}

void DecodeBC1Block(const uint8_t* s, uint32_t out[16]) {
    uint16_t c0 = uint16_t(s[0] | (s[1] << 8));
    uint16_t c1 = uint16_t(s[2] | (s[3] << 8));
    auto expand = [](uint16_t c, uint8_t& r, uint8_t& g, uint8_t& b) {
        r = uint8_t((c >> 11) & 0x1F); r = uint8_t((r << 3) | (r >> 2));
        g = uint8_t((c >> 5)  & 0x3F); g = uint8_t((g << 2) | (g >> 4));
        b = uint8_t( c        & 0x1F); b = uint8_t((b << 3) | (b >> 2));
    };
    uint8_t r[4], g[4], b[4], a[4] = { 255, 255, 255, 255 };
    expand(c0, r[0], g[0], b[0]);
    expand(c1, r[1], g[1], b[1]);
    if (c0 > c1) {
        r[2] = uint8_t((2 * r[0] + r[1]) / 3); g[2] = uint8_t((2 * g[0] + g[1]) / 3); b[2] = uint8_t((2 * b[0] + b[1]) / 3);
        r[3] = uint8_t((r[0] + 2 * r[1]) / 3); g[3] = uint8_t((g[0] + 2 * g[1]) / 3); b[3] = uint8_t((b[0] + 2 * b[1]) / 3);
    } else {
        r[2] = uint8_t((r[0] + r[1]) / 2); g[2] = uint8_t((g[0] + g[1]) / 2); b[2] = uint8_t((b[0] + b[1]) / 2);
        r[3] = g[3] = b[3] = 0; a[3] = 0; // 1-bit alpha
    }
    uint32_t bits = uint32_t(s[4]) | (uint32_t(s[5]) << 8) | (uint32_t(s[6]) << 16) | (uint32_t(s[7]) << 24);
    for (int i = 0; i < 16; ++i) {
        int idx = (bits >> (2 * i)) & 3;
        out[i] = PackBGRA(b[idx], g[idx], r[idx], a[idx]);
    }
}

// Overwrites the alpha byte of each texel from a DXT3 (BC2) alpha block.
void ApplyBC2Alpha(const uint8_t* s, uint32_t out[16]) {
    for (int i = 0; i < 16; ++i) {
        uint8_t nib = (s[i / 2] >> ((i & 1) * 4)) & 0x0F;
        uint8_t a   = uint8_t(nib * 17);
        out[i] = (out[i] & 0x00FFFFFFu) | (uint32_t(a) << 24);
    }
}

// Overwrites the alpha byte of each texel from a DXT5 (BC3) alpha block.
void ApplyBC3Alpha(const uint8_t* s, uint32_t out[16]) {
    uint8_t a0 = s[0], a1 = s[1];
    uint8_t a[8];
    a[0] = a0; a[1] = a1;
    if (a0 > a1) {
        for (int i = 1; i < 7; ++i) a[i + 1] = uint8_t(((7 - i) * a0 + i * a1) / 7);
    } else {
        for (int i = 1; i < 5; ++i) a[i + 1] = uint8_t(((5 - i) * a0 + i * a1) / 5);
        a[6] = 0; a[7] = 255;
    }
    uint64_t bits = 0;
    for (int i = 0; i < 6; ++i) bits |= uint64_t(s[2 + i]) << (8 * i);
    for (int i = 0; i < 16; ++i) {
        int idx = int((bits >> (3 * i)) & 7);
        out[i] = (out[i] & 0x00FFFFFFu) | (uint32_t(a[idx]) << 24);
    }
}

/// Decode the top mip of `tex` to tightly-packed BGRA8. Returns false for
/// formats we can't decode (palettised / unknown).
bool DecodeTextureBGRA(const Texture& tex, std::vector<uint8_t>& bgra,
                       int& outW, int& outH) {
    const int w = tex.width, h = tex.height;
    if (w <= 0 || h <= 0 || tex.data.empty()) return false;
    outW = w; outH = h;
    bgra.assign(size_t(w) * h * 4, 0);
    auto* dst = reinterpret_cast<uint32_t*>(bgra.data());

    const bool isDXT = tex.format == TexFormat::DXT1 ||
                       tex.format == TexFormat::DXT3 ||
                       tex.format == TexFormat::DXT5;
    if (isDXT) {
        const int blockBytes = (tex.format == TexFormat::DXT1) ? 8 : 16;
        const int bw = (w + 3) / 4, bh = (h + 3) / 4;
        const uint8_t* src = tex.data.data();
        const size_t need = size_t(bw) * bh * blockBytes;
        if (tex.data.size() < need) return false;
        for (int by = 0; by < bh; ++by) {
            for (int bx = 0; bx < bw; ++bx) {
                const uint8_t* blk = src + (size_t(by) * bw + bx) * blockBytes;
                uint32_t texel[16];
                const uint8_t* colorBlk = blk + (blockBytes == 16 ? 8 : 0);
                DecodeBC1Block(colorBlk, texel);
                if (tex.format == TexFormat::DXT3) ApplyBC2Alpha(blk, texel);
                else if (tex.format == TexFormat::DXT5) ApplyBC3Alpha(blk, texel);
                for (int py = 0; py < 4; ++py) {
                    int y = by * 4 + py; if (y >= h) break;
                    for (int px = 0; px < 4; ++px) {
                        int x = bx * 4 + px; if (x >= w) break;
                        dst[size_t(y) * w + x] = texel[py * 4 + px];
                    }
                }
            }
        }
        return true;
    }

    if (tex.format == TexFormat::ARGB32) {
        // D3D A8R8G8B8 is stored B,G,R,A in memory — already BGRA.
        const size_t need = size_t(w) * h * 4;
        if (tex.data.size() < need) return false;
        std::memcpy(bgra.data(), tex.data.data(), need);
        return true;
    }
    return false; // PAL8 / Unknown
}

// ─── PNG encode (Windows WIC) ────────────────────────────────────────────────
#ifdef _WIN32
bool EncodePNG(const std::vector<uint8_t>& bgra, int w, int h,
               std::vector<uint8_t>& png) {
    HRESULT hrInit = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    const bool didInit = (hrInit == S_OK || hrInit == S_FALSE);

    bool ok = false;
    IWICImagingFactory* fac = nullptr;
    IWICBitmapEncoder*  enc = nullptr;
    IWICBitmapFrameEncode* frame = nullptr;
    IStream* stream = nullptr;
    IPropertyBag2* props = nullptr;

    if (SUCCEEDED(CoCreateInstance(CLSID_WICImagingFactory, nullptr,
            CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&fac))) &&
        SUCCEEDED(CreateStreamOnHGlobal(nullptr, TRUE, &stream)) &&
        SUCCEEDED(fac->CreateEncoder(GUID_ContainerFormatPng, nullptr, &enc)) &&
        SUCCEEDED(enc->Initialize(stream, WICBitmapEncoderNoCache)) &&
        SUCCEEDED(enc->CreateNewFrame(&frame, &props)) &&
        SUCCEEDED(frame->Initialize(props))) {
        WICPixelFormatGUID fmt = GUID_WICPixelFormat32bppBGRA;
        frame->SetSize(UINT(w), UINT(h));
        frame->SetPixelFormat(&fmt);
        const UINT stride = UINT(w) * 4;
        if (SUCCEEDED(frame->WritePixels(UINT(h), stride, stride * UINT(h),
                          const_cast<BYTE*>(bgra.data()))) &&
            SUCCEEDED(frame->Commit()) && SUCCEEDED(enc->Commit())) {
            HGLOBAL hg = nullptr;
            if (SUCCEEDED(GetHGlobalFromStream(stream, &hg)) && hg) {
                const size_t sz = GlobalSize(hg);
                if (void* p = GlobalLock(hg)) {
                    png.assign(static_cast<uint8_t*>(p),
                               static_cast<uint8_t*>(p) + sz);
                    GlobalUnlock(hg);
                    ok = !png.empty();
                }
            }
        }
    }

    if (props)  props->Release();
    if (frame)  frame->Release();
    if (enc)    enc->Release();
    if (stream) stream->Release();
    if (fac)    fac->Release();
    if (didInit) CoUninitialize();
    return ok;
}
#else
bool EncodePNG(const std::vector<uint8_t>&, int, int, std::vector<uint8_t>&) {
    return false; // PNG embedding only implemented on Windows (WIC).
}
#endif

// ─── glTF / GLB builder ──────────────────────────────────────────────────────

std::string SanitizeName(const std::string& s) {
    std::string out;
    for (char c : s)
        out.push_back((std::isalnum((unsigned char)c) || c == '_' || c == '-') ? c : '_');
    if (out.empty()) out = "object";
    return out;
}

/// Accumulates binary data and the parallel glTF JSON arrays, then serialises a
/// single self-contained .glb.
struct GlbBuilder {
    std::vector<uint8_t> bin;
    json bufferViews = json::array();
    json accessors   = json::array();
    json meshes      = json::array();
    json materials   = json::array();
    json images      = json::array();
    json textures    = json::array();
    json nodes       = json::array();
    json sceneNodes  = json::array();

    void Align4() { while (bin.size() % 4) bin.push_back(0); }

    int AddView(const void* data, size_t bytes, int target) {
        Align4();
        const size_t off = bin.size();
        const auto* p = static_cast<const uint8_t*>(data);
        bin.insert(bin.end(), p, p + bytes);
        json v = { {"buffer", 0}, {"byteOffset", off}, {"byteLength", bytes} };
        if (target) v["target"] = target;
        bufferViews.push_back(v);
        return int(bufferViews.size()) - 1;
    }

    int AddFloatAccessor(const std::vector<float>& data, int comps,
                         const char* type, bool withMinMax) {
        const int view = AddView(data.data(), data.size() * sizeof(float), 34962);
        json a = { {"bufferView", view}, {"componentType", 5126},
                   {"count", data.size() / comps}, {"type", type} };
        if (withMinMax && comps == 3 && !data.empty()) {
            float mn[3] = { data[0], data[1], data[2] };
            float mx[3] = { data[0], data[1], data[2] };
            for (size_t i = 0; i < data.size(); i += 3)
                for (int c = 0; c < 3; ++c) {
                    mn[c] = std::min(mn[c], data[i + c]);
                    mx[c] = std::max(mx[c], data[i + c]);
                }
            a["min"] = { mn[0], mn[1], mn[2] };
            a["max"] = { mx[0], mx[1], mx[2] };
        }
        accessors.push_back(a);
        return int(accessors.size()) - 1;
    }

    int AddIndexAccessor(const std::vector<uint16_t>& idx) {
        const int view = AddView(idx.data(), idx.size() * sizeof(uint16_t), 34963);
        accessors.push_back({ {"bufferView", view}, {"componentType", 5123},
                              {"count", idx.size()}, {"type", "SCALAR"} });
        return int(accessors.size()) - 1;
    }

    int AddPNGImage(const std::vector<uint8_t>& png, const std::string& name) {
        const int view = AddView(png.data(), png.size(), 0);
        images.push_back({ {"bufferView", view}, {"mimeType", "image/png"},
                           {"name", name} });
        const int img = int(images.size()) - 1;
        textures.push_back({ {"source", img} });
        return int(textures.size()) - 1; // texture index
    }

    void Write(const std::filesystem::path& path) {
        json doc;
        doc["asset"] = { {"version", "2.0"}, {"generator", "NFSMWToolkit"} };
        doc["scene"] = 0;
        doc["scenes"] = json::array({ json{ {"nodes", sceneNodes} } });
        doc["nodes"] = nodes;
        doc["meshes"] = meshes;
        if (!materials.empty()) doc["materials"] = materials;
        if (!images.empty())    doc["images"]    = images;
        if (!textures.empty()) {
            doc["textures"] = textures;
            doc["samplers"] = json::array({ json::object() });
        }
        doc["accessors"] = accessors;
        doc["bufferViews"] = bufferViews;
        doc["buffers"] = json::array({ json{ {"byteLength", bin.size()} } });

        std::string jsonStr = doc.dump();
        while (jsonStr.size() % 4) jsonStr.push_back(' ');
        std::vector<uint8_t> binPadded = bin;
        while (binPadded.size() % 4) binPadded.push_back(0);

        const uint32_t jsonLen = uint32_t(jsonStr.size());
        const uint32_t binLen  = uint32_t(binPadded.size());
        const uint32_t total   = 12 + 8 + jsonLen + 8 + binLen;

        std::ofstream out(path, std::ios::binary);
        auto w32 = [&](uint32_t v) { out.write(reinterpret_cast<const char*>(&v), 4); };
        w32(0x46546C67); w32(2); w32(total);          // "glTF", version, length
        w32(jsonLen); w32(0x4E4F534A);                // JSON chunk
        out.write(jsonStr.data(), jsonLen);
        w32(binLen); w32(0x004E4942);                 // BIN chunk
        out.write(reinterpret_cast<const char*>(binPadded.data()), binLen);
    }
};

/// Convert a game-space (Z-up) position to glTF (Y-up): (x, z, -y).
inline void ToYUp(float x, float y, float z, float& ox, float& oy, float& oz) {
    ox = x; oy = z; oz = -y;
}

/// Build one glTF material per unique textureHash referenced by `objs`, dumping
/// + embedding each texture as PNG. Returns map textureHash -> material index.
std::unordered_map<uint32_t, int> BuildMaterials(
        const std::vector<const SolidObject*>& objs, const StreamSection& section,
        const std::vector<TPKBlock>* extraTPKs, GlbBuilder& b,
        Exporter::ExportReport& report, bool exportTextures) {
    std::unordered_map<uint32_t, int> matIndex;
    std::unordered_map<uint32_t, SimSurface> surfaceOf;
    std::unordered_set<uint32_t> hashes;
    for (const auto* obj : objs)
        for (const auto& m : obj->meshes) {
            hashes.insert(m.textureHash);
            if (!surfaceOf.count(m.textureHash)) surfaceOf[m.textureHash] = m.surface;
        }

    std::unordered_map<uint32_t, int> texIndex; // textureHash -> glTF texture idx
    if (exportTextures) {
        for (uint32_t h : hashes) {
            if (!h) continue;
            const Texture* tex = FindTexture(h, section, extraTPKs);
            if (!tex) continue;
            std::vector<uint8_t> bgra; int tw = 0, th = 0;
            if (!DecodeTextureBGRA(*tex, bgra, tw, th)) {
                char hexId[16];
                std::snprintf(hexId, sizeof(hexId), "%08X", h);
                report.warnings.push_back(std::string("Texture 0x") + hexId +
                    " could not be decoded (" + TexFormatName(tex->format) + ") - skipped");
                continue;
            }
            std::vector<uint8_t> png;
            if (!EncodePNG(bgra, tw, th, png)) continue;
            texIndex[h] = b.AddPNGImage(png, tex->name.empty() ? "tex" : tex->name);
            ++report.texturesWritten;
        }
    }

    for (uint32_t h : hashes) {
        const SimSurface surf = surfaceOf[h];
        std::string name = "mat_";
        if (h) {
            if (const auto* n = HashResolver::Instance().TryResolve(h))
                name += SanitizeName(*n);
            else { char buf[16]; std::snprintf(buf, sizeof(buf), "%08X", h); name += buf; }
        } else {
            name += "untextured";
        }
        name += std::string("_") + SimSurfaceName(surf);

        json pbr = { {"baseColorFactor", { 1.0, 1.0, 1.0, 1.0 }},
                     {"metallicFactor", 0.0}, {"roughnessFactor", 1.0} };
        auto it = texIndex.find(h);
        if (it != texIndex.end())
            pbr["baseColorTexture"] = { {"index", it->second} };

        json mat = { {"name", name},
                     {"pbrMetallicRoughness", pbr},
                     {"doubleSided", true},
                     {"extras", { {"simSurface", SimSurfaceName(surf)} }} };
        b.materials.push_back(mat);
        matIndex[h] = int(b.materials.size()) - 1;
        ++report.materialsWritten;
    }
    return matIndex;
}

/// Append one object as a glTF mesh + node. Geometry is converted to Y-up and
/// (optionally) baked with the object's world transform.
void AddObjectNode(const SolidObject& obj, const std::string& nodeName,
                   bool applyTransform,
                   const std::unordered_map<uint32_t, int>& matIndex,
                   GlbBuilder& b, Exporter::ExportReport& report) {
    const glm::mat4 xf = applyTransform ? obj.transform : glm::mat4(1.0f);
    const glm::mat3 nrm = glm::mat3(glm::transpose(glm::inverse(xf)));

    json primitives = json::array();
    for (const auto& mesh : obj.meshes) {
        if (mesh.indexOutOfRange || mesh.vertices.empty() || mesh.indices.size() < 3)
            continue;

        std::vector<float> pos, nor, uv;
        pos.reserve(mesh.vertices.size() * 3);
        nor.reserve(mesh.vertices.size() * 3);
        uv.reserve(mesh.vertices.size() * 2);
        for (const auto& v : mesh.vertices) {
            glm::vec4 p = xf * glm::vec4(v.pos[0], v.pos[1], v.pos[2], 1.0f);
            float ox, oy, oz; ToYUp(p.x, p.y, p.z, ox, oy, oz);
            pos.push_back(ox); pos.push_back(oy); pos.push_back(oz);
            glm::vec3 n = nrm * glm::vec3(v.normal[0], v.normal[1], v.normal[2]);
            ToYUp(n.x, n.y, n.z, ox, oy, oz);
            nor.push_back(ox); nor.push_back(oy); nor.push_back(oz);
            uv.push_back(v.uv[0]); uv.push_back(v.uv[1]);
        }

        json attribs;
        attribs["POSITION"]   = b.AddFloatAccessor(pos, 3, "VEC3", true);
        attribs["NORMAL"]     = b.AddFloatAccessor(nor, 3, "VEC3", false);
        attribs["TEXCOORD_0"] = b.AddFloatAccessor(uv, 2, "VEC2", false);

        json prim = { {"attributes", attribs},
                      {"indices", b.AddIndexAccessor(mesh.indices)},
                      {"mode", 4} };
        auto mIt = matIndex.find(mesh.textureHash);
        if (mIt != matIndex.end()) prim["material"] = mIt->second;
        primitives.push_back(prim);

        report.verticesWritten += mesh.vertices.size();
        report.trianglesWritten += mesh.indices.size() / 3;
    }
    if (primitives.empty()) return;

    b.meshes.push_back({ {"name", nodeName}, {"primitives", primitives} });
    const int meshIdx = int(b.meshes.size()) - 1;
    b.nodes.push_back({ {"name", nodeName}, {"mesh", meshIdx} });
    b.sceneNodes.push_back(int(b.nodes.size()) - 1);
}

Result<Exporter::ExportReport> ExportGLB(
        const std::vector<const SolidObject*>& objs, const StreamSection& section,
        const std::filesystem::path& glbPath, const Exporter::ExportOptions& options,
        const std::vector<TPKBlock>* extraTPKs) {
    if (objs.empty())
        return Result<Exporter::ExportReport>::Err("No exportable objects");

    Exporter::ExportReport report;
    const auto outDir = glbPath.parent_path();
    std::error_code ec;
    if (!outDir.empty()) std::filesystem::create_directories(outDir, ec);

    GlbBuilder b;
    auto matIndex = BuildMaterials(objs, section, extraTPKs, b, report,
                                   options.exportTextures);

    std::unordered_map<std::string, int> nameCounts;
    for (const auto* obj : objs) {
        std::string base = SanitizeName(obj->name.empty() ? "object" : obj->name);
        int& n = nameCounts[base];
        std::string nodeName = n == 0 ? base : (base + "_" + std::to_string(n));
        ++n;
        AddObjectNode(*obj, nodeName, options.applyWorldTransform, matIndex, b, report);
        ++report.objectsExported;
    }

    if (b.meshes.empty())
        return Result<Exporter::ExportReport>::Err("No mesh data to export");

    std::ofstream probe(glbPath, std::ios::binary);
    if (!probe)
        return Result<Exporter::ExportReport>::Err("Cannot create '" + glbPath.string() + "'");
    probe.close();
    b.Write(glbPath);

    LOG_INFO("GLTFExporter: {} -> {} ({} objects, {} verts, {} tris, {} materials, {} textures)",
             objs.size(), glbPath.string(), report.objectsExported,
             report.verticesWritten, report.trianglesWritten,
             report.materialsWritten, report.texturesWritten);
    return Result<Exporter::ExportReport>::Ok(std::move(report));
}

} // namespace

Result<Exporter::ExportReport> Exporter::ExportObjectGLTF(
        const SolidObject& obj, const StreamSection& section,
        const std::filesystem::path& glbPath, const ExportOptions& options,
        const std::vector<TPKBlock>* extraTPKs) {
    if (obj.meshes.empty())
        return Result<ExportReport>::Err("Object '" + obj.name + "' has no mesh data");
    return ExportGLB({ &obj }, section, glbPath, options, extraTPKs);
}

Result<Exporter::ExportReport> Exporter::ExportSectionGLTF(
        const StreamSection& section, const std::filesystem::path& glbPath,
        const ExportOptions& options, const std::vector<TPKBlock>* extraTPKs) {
    std::vector<const SolidObject*> objs;
    for (const auto& sl : section.solidLists)
        for (const auto& obj : sl.objects)
            if (!obj.meshes.empty()) objs.push_back(&obj);
    return ExportGLB(objs, section, glbPath, options, extraTPKs);
}

} // namespace nfsmw
