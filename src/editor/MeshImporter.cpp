#include "editor/MeshImporter.h"
#include "core/Logger.h"
#include "core/StringUtil.h"

#include <nlohmann/json.hpp>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <array>
#include <cstdlib>
#include <cstring>
#include <cstdint>

namespace nfsmw {
namespace {

using json = nlohmann::json;

void SetWhite(SolidMeshVertex& v) { v.color[0] = v.color[1] = v.color[2] = v.color[3] = 255; }

// ─── OBJ ─────────────────────────────────────────────────────────────────────

Result<ImportModel> ParseOBJ(const std::filesystem::path& path) {
    std::ifstream in(path);
    if (!in) return Result<ImportModel>::Err("Cannot open '" + path.string() + "'");

    std::vector<std::array<float,3>> positions, normals;
    std::vector<std::array<float,2>> uvs;

    ImportModel model;
    ImportMesh* cur = nullptr;
    std::unordered_map<std::string, uint16_t> dedup; // "v/t/n" -> local index (per group)

    auto ensureGroup = [&](const std::string& mat) {
        model.meshes.push_back(ImportMesh{});
        cur = &model.meshes.back();
        cur->materialName = mat.empty() ? "imported" : mat;
        dedup.clear();
    };

    auto corner = [&](const std::string& tok) -> uint16_t {
        // tok = v[/vt][/vn] (1-based, negative allowed)
        auto it = dedup.find(tok);
        if (it != dedup.end()) return it->second;
        // Parse "v", "v/vt", "v/vt/vn" or "v//vn" (1-based, negatives allowed).
        int vi = 0, ti = 0, ni = 0;
        const size_t s1 = tok.find('/');
        if (s1 == std::string::npos) {
            vi = std::atoi(tok.c_str());
        } else {
            vi = std::atoi(tok.substr(0, s1).c_str());
            const size_t s2 = tok.find('/', s1 + 1);
            const std::string mid = (s2 == std::string::npos)
                                  ? tok.substr(s1 + 1) : tok.substr(s1 + 1, s2 - s1 - 1);
            if (!mid.empty()) ti = std::atoi(mid.c_str());
            if (s2 != std::string::npos && s2 + 1 < tok.size())
                ni = std::atoi(tok.substr(s2 + 1).c_str());
        }
        auto resolve = [](int idx, size_t n) -> int {
            if (idx > 0) return idx - 1;
            if (idx < 0) return int(n) + idx;
            return -1;
        };
        SolidMeshVertex v{};
        int p = resolve(vi, positions.size());
        if (p >= 0 && p < (int)positions.size())
            { v.pos[0]=positions[p][0]; v.pos[1]=positions[p][1]; v.pos[2]=positions[p][2]; }
        int t = resolve(ti, uvs.size());
        if (t >= 0 && t < (int)uvs.size()) { v.uv[0]=uvs[t][0]; v.uv[1]=1.0f-uvs[t][1]; } // undo export flip
        int nn = resolve(ni, normals.size());
        if (nn >= 0 && nn < (int)normals.size())
            { v.normal[0]=normals[nn][0]; v.normal[1]=normals[nn][1]; v.normal[2]=normals[nn][2]; }
        else { v.normal[2] = 1.0f; }
        SetWhite(v);
        uint16_t local = uint16_t(cur->vertices.size());
        cur->vertices.push_back(v);
        dedup.emplace(tok, local);
        return local;
    };

    std::string line;
    while (std::getline(in, line)) {
        std::istringstream ss(line);
        std::string kw; ss >> kw;
        if (kw == "v") { std::array<float,3> a{}; ss>>a[0]>>a[1]>>a[2]; positions.push_back(a); }
        else if (kw == "vt") { std::array<float,2> a{}; ss>>a[0]>>a[1]; uvs.push_back(a); }
        else if (kw == "vn") { std::array<float,3> a{}; ss>>a[0]>>a[1]>>a[2]; normals.push_back(a); }
        else if (kw == "usemtl") { std::string m; ss>>m; ensureGroup(m); }
        else if (kw == "f") {
            if (!cur) ensureGroup("imported");
            std::vector<uint16_t> poly; std::string tok;
            while (ss >> tok) poly.push_back(corner(tok));
            for (size_t i = 1; i + 1 < poly.size(); ++i) {  // fan triangulation
                cur->indices.push_back(poly[0]);
                cur->indices.push_back(poly[i]);
                cur->indices.push_back(poly[i + 1]);
            }
        }
    }

    // Drop empty groups.
    std::vector<ImportMesh> kept;
    for (auto& m : model.meshes)
        if (!m.vertices.empty() && m.indices.size() >= 3) kept.push_back(std::move(m));
    model.meshes = std::move(kept);
    if (model.meshes.empty())
        return Result<ImportModel>::Err("OBJ contained no triangles");
    return Result<ImportModel>::Ok(std::move(model));
}

// ─── glTF ──────────────────────────────────────────────────────────────────

std::vector<uint8_t> DecodeBase64(const std::string& s) {
    static const std::string T =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::vector<int> rev(256, -1);
    for (int i = 0; i < 64; ++i) rev[uint8_t(T[i])] = i;
    std::vector<uint8_t> out;
    int val = 0, bits = -8;
    for (uint8_t c : s) {
        if (rev[c] < 0) continue;
        val = (val << 6) + rev[c]; bits += 6;
        if (bits >= 0) { out.push_back(uint8_t((val >> bits) & 0xFF)); bits -= 8; }
    }
    return out;
}

template <typename T>
T ReadLE(const uint8_t* p) { T v; std::memcpy(&v, p, sizeof(T)); return v; }

Result<ImportModel> ParseGLTF(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary | std::ios::ate);
    if (!in) return Result<ImportModel>::Err("Cannot open '" + path.string() + "'");
    std::vector<uint8_t> file(size_t(in.tellg()));
    in.seekg(0); in.read(reinterpret_cast<char*>(file.data()), file.size());

    json doc;
    std::vector<uint8_t> glbBin;        // BIN chunk from a .glb
    if (file.size() >= 12 && ReadLE<uint32_t>(file.data()) == 0x46546C67) {
        // .glb container: JSON chunk then optional BIN chunk.
        size_t off = 12;
        while (off + 8 <= file.size()) {
            uint32_t len = ReadLE<uint32_t>(file.data() + off);
            uint32_t typ = ReadLE<uint32_t>(file.data() + off + 4);
            const uint8_t* p = file.data() + off + 8;
            if (off + 8 + len > file.size()) break;
            if (typ == 0x4E4F534A) doc = json::parse(std::string(p, p + len), nullptr, false);
            else if (typ == 0x004E4942) glbBin.assign(p, p + len);
            off += 8 + len;
        }
    } else {
        doc = json::parse(std::string(file.begin(), file.end()), nullptr, false);
    }
    if (doc.is_discarded() || !doc.contains("meshes"))
        return Result<ImportModel>::Err("Not a valid glTF document");

    // Resolve buffers (GLB bin, base64 data URIs, or sibling files).
    std::vector<std::vector<uint8_t>> buffers;
    for (const auto& b : doc.value("buffers", json::array())) {
        std::string uri = b.value("uri", "");
        if (uri.empty()) { buffers.push_back(glbBin); continue; }
        const std::string base64Tag = "base64,";
        auto pos = uri.find(base64Tag);
        if (uri.rfind("data:", 0) == 0 && pos != std::string::npos) {
            buffers.push_back(DecodeBase64(uri.substr(pos + base64Tag.size())));
        } else {
            std::ifstream bf(path.parent_path() / uri, std::ios::binary | std::ios::ate);
            if (!bf) return Result<ImportModel>::Err("Missing glTF buffer '" + uri + "'");
            std::vector<uint8_t> d(size_t(bf.tellg())); bf.seekg(0);
            bf.read(reinterpret_cast<char*>(d.data()), d.size());
            buffers.push_back(std::move(d));
        }
    }

    const auto& views    = doc.value("bufferViews", json::array());
    const auto& accs     = doc.value("accessors", json::array());
    const auto& mats     = doc.value("materials", json::array());

    // Returns a pointer into the resolved buffer plus the number of bytes
    // available from it (`avail`). Returns nullptr on any out-of-range index so
    // callers can skip gracefully instead of reading out of bounds.
    auto accessorPtr = [&](int ai, size_t& count, int& comps, int& compType,
                           size_t& avail) -> const uint8_t* {
        count = 0; comps = 0; compType = 0; avail = 0;
        if (ai < 0 || ai >= static_cast<int>(accs.size())) return nullptr;
        const auto& a = accs[ai];
        count = a.value("count", 0u);
        const std::string t = a.value("type", "SCALAR");
        comps = (t == "SCALAR") ? 1 : (t == "VEC2") ? 2 : (t == "VEC3") ? 3 : (t == "VEC4") ? 4 : 1;
        compType = a.value("componentType", 5126);
        const int bvIdx = a.value("bufferView", -1);
        if (bvIdx < 0 || bvIdx >= static_cast<int>(views.size())) return nullptr;
        const auto& bv = views[bvIdx];
        const int bufIdx = bv.value("buffer", -1);
        if (bufIdx < 0 || bufIdx >= static_cast<int>(buffers.size())) return nullptr;
        const size_t off = bv.value("byteOffset", 0u) + a.value("byteOffset", 0u);
        if (off > buffers[bufIdx].size()) return nullptr;
        avail = buffers[bufIdx].size() - off;
        return buffers[bufIdx].data() + off;
    };

    ImportModel model;
    for (const auto& mesh : doc["meshes"]) {
        for (const auto& prim : mesh.value("primitives", json::array())) {
            if (prim.value("mode", 4) != 4) continue; // triangles only
            const auto& attr = prim.value("attributes", json::object());
            if (!attr.contains("POSITION")) continue;

            ImportMesh im;
            if (prim.contains("material") && prim["material"].is_number_integer()) {
                int mi = prim["material"];
                if (mi >= 0 && mi < (int)mats.size())
                    im.materialName = mats[mi].value("name", "imported");
            }
            if (im.materialName.empty()) im.materialName = "imported";

            size_t count, avail; int comps, ctype;
            const uint8_t* pp = accessorPtr(attr["POSITION"], count, comps, ctype, avail);
            if (!pp || comps < 3 || ctype != 5126 || count == 0 || avail < count * 12)
                continue; // malformed POSITION — skip this primitive
            im.vertices.resize(count);
            for (size_t i = 0; i < count; ++i) {
                SolidMeshVertex& v = im.vertices[i];
                const float* f = reinterpret_cast<const float*>(pp + i * 12);
                // glTF Y-up -> game Z-up: (x, -z, y)
                v.pos[0] = f[0]; v.pos[1] = -f[2]; v.pos[2] = f[1];
                v.normal[2] = 1.0f; SetWhite(v);
            }
            const size_t vcount = im.vertices.size();
            if (attr.contains("NORMAL")) {
                size_t nc, navail; int ncomps, nctype;
                const uint8_t* np = accessorPtr(attr["NORMAL"], nc, ncomps, nctype, navail);
                if (np && nctype == 5126 && nc >= vcount && navail >= vcount * 12)
                    for (size_t i = 0; i < vcount; ++i) {
                        const float* f = reinterpret_cast<const float*>(np + i * 12);
                        im.vertices[i].normal[0] = f[0];
                        im.vertices[i].normal[1] = -f[2];
                        im.vertices[i].normal[2] = f[1];
                    }
            }
            if (attr.contains("TEXCOORD_0")) {
                size_t uc, uavail; int ucomps, uctype;
                const uint8_t* up = accessorPtr(attr["TEXCOORD_0"], uc, ucomps, uctype, uavail);
                if (up && uctype == 5126 && uc >= vcount && uavail >= vcount * 8)
                    for (size_t i = 0; i < vcount; ++i) {
                        const float* f = reinterpret_cast<const float*>(up + i * 8);
                        im.vertices[i].uv[0] = f[0]; im.vertices[i].uv[1] = f[1];
                    }
            }
            // Indices
            if (prim.contains("indices")) {
                size_t ic, iavail; int icomps, ictype;
                const uint8_t* ip = accessorPtr(prim["indices"], ic, icomps, ictype, iavail);
                const size_t isz = (ictype == 5125) ? 4 : (ictype == 5123) ? 2 : 1;
                if (ip && iavail >= ic * isz) {
                    im.indices.reserve(ic);
                    for (size_t i = 0; i < ic; ++i) {
                        uint32_t idx = 0;
                        if (ictype == 5125) idx = ReadLE<uint32_t>(ip + i * 4);
                        else if (ictype == 5123) idx = ReadLE<uint16_t>(ip + i * 2);
                        else idx = ip[i];
                        im.indices.push_back(uint16_t(idx));
                    }
                }
            } else {
                for (uint16_t i = 0; i < vcount; ++i) im.indices.push_back(i);
            }
            if (!im.vertices.empty() && im.indices.size() >= 3)
                model.meshes.push_back(std::move(im));
        }
    }
    if (model.meshes.empty())
        return Result<ImportModel>::Err("glTF contained no triangle meshes");
    return Result<ImportModel>::Ok(std::move(model));
}

} // namespace

Result<ImportModel> MeshImporter::ImportOBJ(const std::filesystem::path& p)  { return ParseOBJ(p); }
Result<ImportModel> MeshImporter::ImportGLTF(const std::filesystem::path& p) { return ParseGLTF(p); }

Result<ImportModel> MeshImporter::Import(const std::filesystem::path& path) {
    std::string ext = ToLowerAscii(path.extension().string());
    if (ext == ".obj")  return ImportOBJ(path);
    if (ext == ".glb" || ext == ".gltf") return ImportGLTF(path);
    return Result<ImportModel>::Err("Unsupported import format: " + ext);
}

} // namespace nfsmw
