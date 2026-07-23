#include "editor/Exporter.h"
#include "core/Logger.h"
#include "core/StringHash.h"
#include <fstream>
#include <unordered_set>
#include <unordered_map>

namespace nfsmw {

namespace {

/// Sanitize a name for use as an OBJ object/material name or filename
/// fragment (alnum + underscore only).
std::string SanitizeName(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        if (std::isalnum((unsigned char)c) || c == '_' || c == '-')
            out.push_back(c);
        else
            out.push_back('_');
    }
    if (out.empty()) out = "object";
    return out;
}

std::string TextureFileBaseName(const Texture* tex, uint32_t hash) {
    if (tex && !tex->name.empty())
        return SanitizeName(tex->name);
    char buf[16];
    std::snprintf(buf, sizeof(buf), "tex_%08X", hash);
    return buf;
}

/// Dump every unique texture referenced by `obj` to `<dir>/<name>.dds`.
/// Returns the set of hashes that were successfully written (so the MTL
/// writer knows which `map_Kd` paths are valid).
std::unordered_map<uint32_t, std::string> DumpTextures(
        const SolidObject& obj, const StreamSection& section,
        const std::vector<TPKBlock>* extraTPKs,
        const std::filesystem::path& outDir,
        Exporter::ExportReport& report) {
    std::unordered_map<uint32_t, std::string> written;
    std::unordered_set<uint32_t> hashes;
    for (const auto& mesh : obj.meshes)
        if (mesh.textureHash) hashes.insert(mesh.textureHash);
    for (uint32_t h : obj.textureHashes) hashes.insert(h);

    if (hashes.empty()) return written;

    std::error_code ec;
    std::filesystem::create_directories(outDir, ec);
    if (ec) {
        report.warnings.push_back("Could not create texture directory '" +
                                   outDir.string() + "': " + ec.message());
        return written;
    }

    for (uint32_t hash : hashes) {
        const Texture* tex = FindTexture(hash, section, extraTPKs);
        if (!tex || tex->data.empty()) {
            char buf[64];
            std::snprintf(buf, sizeof(buf), "0x%08X", hash);
            report.warnings.push_back(std::string("Texture ") + buf +
                                       " not found in loaded TPKs - skipped");
            continue;
        }
        const std::string base = TextureFileBaseName(tex, hash);
        const auto path = outDir / (base + ".dds");
        auto r = DDSCodec::Export(*tex, tex->data, path);
        if (!r) {
            report.warnings.push_back("Export '" + path.string() + "': " + r.error);
            continue;
        }
        written[hash] = base + ".dds";
        ++report.texturesWritten;
    }
    return written;
}

/// Write the .mtl sidecar. One material per unique textureHash across the
/// exported meshes, plus an untextured fallback material. Returns a map
/// from textureHash -> material name (0 = fallback) for the OBJ writer.
std::unordered_map<uint32_t, std::string> WriteMTL(
        const std::vector<const SolidObject*>& objs,
        const std::unordered_map<uint32_t, std::string>& texFiles,
        const std::filesystem::path& mtlPath,
        Exporter::ExportReport& report) {
    std::unordered_map<uint32_t, std::string> matNames;

    std::ofstream out(mtlPath);
    if (!out) {
        report.warnings.push_back("Cannot create '" + mtlPath.string() + "'");
        return matNames;
    }

    out << "# NFSMWToolkit object export material library\n";

    // Fallback material for meshes with no resolved texture.
    out << "\nnewmtl default\n";
    out << "Kd 0.8 0.8 0.8\n";
    out << "d 1.0\n";
    matNames[0] = "default";
    ++report.materialsWritten;

    std::unordered_set<uint32_t> seen;
    for (const auto* obj : objs) {
        for (const auto& mesh : obj->meshes) {
            if (!mesh.textureHash || seen.count(mesh.textureHash)) continue;
            seen.insert(mesh.textureHash);

            std::string matName = "mat_";
            if (const auto* name = HashResolver::Instance().TryResolve(mesh.textureHash))
                matName += SanitizeName(*name);
            else {
                char buf[16];
                std::snprintf(buf, sizeof(buf), "%08X", mesh.textureHash);
                matName += buf;
            }
            matNames[mesh.textureHash] = matName;

            out << "\nnewmtl " << matName << "\n";
            out << "Kd 1.0 1.0 1.0\n";
            out << "d 1.0\n";

            auto texIt = texFiles.find(mesh.textureHash);
            if (texIt != texFiles.end()) {
                // Most external tools (Blender, etc.) prefer PNG/JPG for
                // map_Kd, but accept DDS via plugins; reference the dumped
                // .dds path relative to the OBJ/MTL location.
                out << "map_Kd textures/" << texIt->second << "\n";
            }
            ++report.materialsWritten;
        }
    }

    return matNames;
}

/// Write `obj`'s geometry (all shading groups) into the OBJ stream, using
/// `vertexBase` as the running 1-based vertex index and `matNames` to emit
/// `usemtl` per shading group. Updates `vertexBase` and the report counters.
void WriteObjectGeometry(std::ofstream& out, const SolidObject& obj,
                          const std::string& objName, bool applyTransform,
                          const std::unordered_map<uint32_t, std::string>& matNames,
                          size_t& vertexBase, Exporter::ExportReport& report) {
    if (obj.meshes.empty()) return;

    out << "o " << objName << "\n";

    const glm::mat4 xf = applyTransform ? obj.transform : glm::mat4(1.0f);
    const glm::mat3 normalMat = glm::mat3(glm::transpose(glm::inverse(xf)));

    for (const auto& mesh : obj.meshes) {
        if (mesh.indexOutOfRange) {
            report.warnings.push_back(objName + ": skipped a mesh group with "
                                       "out-of-range indices (corrupt/unparsed data)");
            continue;
        }
        if (mesh.vertices.empty() || mesh.indices.size() < 3) continue;

        for (const auto& v : mesh.vertices) {
            glm::vec4 p = xf * glm::vec4(v.pos[0], v.pos[1], v.pos[2], 1.0f);
            out << "v " << p.x << ' ' << p.y << ' ' << p.z << "\n";
        }
        for (const auto& v : mesh.vertices) {
            glm::vec3 n = glm::normalize(normalMat *
                glm::vec3(v.normal[0], v.normal[1], v.normal[2]));
            out << "vn " << n.x << ' ' << n.y << ' ' << n.z << "\n";
        }
        for (const auto& v : mesh.vertices)
            out << "vt " << v.uv[0] << ' ' << (1.0f - v.uv[1]) << "\n";

        auto matIt = matNames.find(mesh.textureHash);
        out << "usemtl " << (matIt != matNames.end() ? matIt->second : "default") << "\n";

        for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
            size_t a = vertexBase + mesh.indices[i];
            size_t b = vertexBase + mesh.indices[i + 1];
            size_t c = vertexBase + mesh.indices[i + 2];
            out << "f " << a << '/' << a << '/' << a << ' '
                        << b << '/' << b << '/' << b << ' '
                        << c << '/' << c << '/' << c << "\n";
            ++report.trianglesWritten;
        }

        report.verticesWritten += mesh.vertices.size();
        vertexBase += mesh.vertices.size();
    }
}

} // namespace

Result<Exporter::ExportReport> Exporter::ExportObjectOBJ(
        const SolidObject& obj, const StreamSection& section,
        const std::filesystem::path& objPath, const ExportOptions& options,
        const std::vector<TPKBlock>* extraTPKs) {
    if (obj.meshes.empty())
        return Result<ExportReport>::Err("Object '" + obj.name + "' has no mesh data");

    ExportReport report;
    const std::string objName = SanitizeName(obj.name.empty() ? "object" : obj.name);

    const auto outDir = objPath.parent_path();
    std::error_code ec;
    if (!outDir.empty()) std::filesystem::create_directories(outDir, ec);

    std::unordered_map<uint32_t, std::string> texFiles;
    if (options.exportTextures) {
        const auto texDir = options.textureSubdir.empty()
                           ? outDir
                           : outDir / options.textureSubdir;
        texFiles = DumpTextures(obj, section, extraTPKs, texDir, report);
    }

    const auto mtlPath = objPath;
    const auto mtlFile = std::filesystem::path(mtlPath).replace_extension(".mtl");
    auto matNames = WriteMTL({ &obj }, texFiles, mtlFile, report);

    std::ofstream out(objPath);
    if (!out)
        return Result<ExportReport>::Err("Cannot create '" + objPath.string() + "'");

    out << "# NFSMWToolkit object export\n";
    out << "# Coordinates are in game space (Z-up) - import with \"Z up\" in Blender\n";
    out << "mtllib " << mtlFile.filename().string() << "\n";

    size_t vertexBase = 1;
    WriteObjectGeometry(out, obj, objName, options.applyWorldTransform,
                        matNames, vertexBase, report);

    report.objectsExported = 1;
    LOG_INFO("Exporter: '{}' -> {} ({} verts, {} tris, {} materials, {} textures)",
             obj.name, objPath.string(), report.verticesWritten,
             report.trianglesWritten, report.materialsWritten, report.texturesWritten);
    return Result<ExportReport>::Ok(std::move(report));
}

Result<Exporter::ExportReport> Exporter::ExportSectionOBJ(
        const StreamSection& section, const std::filesystem::path& objPath,
        const ExportOptions& options, const std::vector<TPKBlock>* extraTPKs) {
    std::vector<const SolidObject*> objs;
    for (const auto& sl : section.solidLists)
        for (const auto& obj : sl.objects)
            if (!obj.meshes.empty()) objs.push_back(&obj);

    if (objs.empty())
        return Result<ExportReport>::Err("Section '" + section.name + "' has no exportable objects");

    ExportReport report;
    const auto outDir = objPath.parent_path();
    std::error_code ec;
    if (!outDir.empty()) std::filesystem::create_directories(outDir, ec);

    std::unordered_map<uint32_t, std::string> texFiles;
    if (options.exportTextures) {
        const auto texDir = options.textureSubdir.empty()
                           ? outDir
                           : outDir / options.textureSubdir;
        for (const auto* obj : objs) {
            auto written = DumpTextures(*obj, section, extraTPKs, texDir, report);
            texFiles.insert(written.begin(), written.end());
        }
    }

    const auto mtlFile = std::filesystem::path(objPath).replace_extension(".mtl");
    auto matNames = WriteMTL(objs, texFiles, mtlFile, report);

    std::ofstream out(objPath);
    if (!out)
        return Result<ExportReport>::Err("Cannot create '" + objPath.string() + "'");

    out << "# NFSMWToolkit section export: " << section.name << "\n";
    out << "# Coordinates are in game space (Z-up) - import with \"Z up\" in Blender\n";
    out << "mtllib " << mtlFile.filename().string() << "\n";

    size_t vertexBase = 1;
    std::unordered_map<std::string, int> nameCounts;
    for (const auto* obj : objs) {
        std::string base = SanitizeName(obj->name.empty() ? "object" : obj->name);
        int& count = nameCounts[base];
        std::string objName = count == 0 ? base : (base + "_" + std::to_string(count));
        ++count;
        WriteObjectGeometry(out, *obj, objName, options.applyWorldTransform,
                            matNames, vertexBase, report);
        ++report.objectsExported;
    }

    LOG_INFO("Exporter: section '{}' -> {} ({} objects, {} verts, {} tris, "
             "{} materials, {} textures)",
             section.name, objPath.string(), report.objectsExported,
             report.verticesWritten, report.trianglesWritten,
             report.materialsWritten, report.texturesWritten);
    return Result<ExportReport>::Ok(std::move(report));
}

} // namespace nfsmw
