#include "renderer/MeshRenderer.h"
#include "renderer/TextureManager.h"
#include "renderer/ShaderProgram.h"
#include "core/Logger.h"

#include "renderer/GLCompat.h"
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>

namespace nfsmw {

// ─── GPUMesh ─────────────────────────────────────────────────────────────────

void GPUMesh::Delete() {
    if (vao) { glDeleteVertexArrays(1, &vao); vao = 0; }
    if (vbo) { glDeleteBuffers(1, &vbo);       vbo = 0; }
    if (ebo) { glDeleteBuffers(1, &ebo);       ebo = 0; }
    uploaded = false;
}

// ─── MeshRenderer ────────────────────────────────────────────────────────────

MeshRenderer::~MeshRenderer() {
    Clear();
}

// static
GPUMesh MeshRenderer::UploadMesh(const SolidMesh& mesh) {
    if (mesh.vertices.empty() || mesh.indices.size() < 3)
        return GPUMesh{};

    GPUMesh gm;
    gm.indexCount  = static_cast<uint32_t>(mesh.indices.size());
    gm.vertexCount = static_cast<uint32_t>(mesh.vertices.size());
    gm.textureHash = mesh.textureHash;
    gm.blendMode   = (mesh.blendOverride >= 0)
                        ? static_cast<uint8_t>(mesh.blendOverride)
                        : 0;

    glGenVertexArrays(1, &gm.vao);
    glGenBuffers(1, &gm.vbo);
    glGenBuffers(1, &gm.ebo);

    glBindVertexArray(gm.vao);

    glBindBuffer(GL_ARRAY_BUFFER, gm.vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(mesh.vertices.size() * sizeof(SolidMeshVertex)),
                 mesh.vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gm.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(mesh.indices.size() * sizeof(uint16_t)),
                 mesh.indices.data(), GL_STATIC_DRAW);

    // SolidMeshVertex layout (36 bytes, matches plan.md §3.5):
    //   pos    float3 @0
    //   normal float3 @12
    //   uv     float2 @24   (note: color is at @24 in the raw game struct but
    //                         SolidMeshVertex reorders to pos/normal/uv/color)
    //   color  u8[4]  @32
    static_assert(sizeof(SolidMeshVertex) == 36,
                  "SolidMeshVertex layout changed — update attribute offsets");

    // location 0: aPos
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(SolidMeshVertex),
                          reinterpret_cast<void*>(offsetof(SolidMeshVertex, pos)));
    glEnableVertexAttribArray(0);

    // location 1: aNormal
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(SolidMeshVertex),
                          reinterpret_cast<void*>(offsetof(SolidMeshVertex, normal)));
    glEnableVertexAttribArray(1);

    // location 2: aUV
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(SolidMeshVertex),
                          reinterpret_cast<void*>(offsetof(SolidMeshVertex, uv)));
    glEnableVertexAttribArray(2);

    // location 3: aColor (RGBA byte, normalised)
    glVertexAttribPointer(3, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(SolidMeshVertex),
                          reinterpret_cast<void*>(offsetof(SolidMeshVertex, color)));
    glEnableVertexAttribArray(3);

    glBindVertexArray(0);

    gm.uploaded = true;
    return gm;
}

uint64_t MeshRenderer::UploadGeometry(const SolidObject& obj) {
    GPUGeometry geom;
    geom.handle = nextHandle_++;

    // Compute local bounds across all meshes.
    for (const auto& mesh : obj.meshes)
        geom.localBounds.Expand(mesh.ComputeAABB());

    // Upload each non-degenerate mesh.
    size_t uploaded = 0;
    for (const auto& mesh : obj.meshes) {
        if (mesh.indexOutOfRange || mesh.vertices.empty() ||
            mesh.indices.size() < 3)
            continue;
        auto gm = UploadMesh(mesh);
        if (gm.uploaded) {
            geom.meshes.push_back(std::move(gm));
            ++uploaded;
        }
    }

    if (uploaded == 0) {
        LOG_WARN("MeshRenderer: no uploadable mesh groups in '{}'", obj.name);
        return 0;
    }

    const uint64_t handle = geom.handle;
    geometries_.emplace(handle, std::move(geom));
    LOG_DEBUG("MeshRenderer: uploaded '{}' ({} groups) -> handle {}",
              obj.name, uploaded, handle);
    return handle;
}

void MeshRenderer::DeleteGeometry(uint64_t handle) {
    auto it = geometries_.find(handle);
    if (it == geometries_.end()) return;
    it->second.Delete();
    geometries_.erase(it);
}

void MeshRenderer::Clear() {
    for (auto& [handle, geom] : geometries_)
        geom.Delete();
    geometries_.clear();
}

const GPUGeometry* MeshRenderer::FindGeometry(uint64_t handle) const {
    auto it = geometries_.find(handle);
    return it != geometries_.end() ? &it->second : nullptr;
}

void MeshRenderer::RenderGeometryImmediate(uint64_t handle,
                                            const ShaderProgram& shader,
                                            const glm::mat4& view,
                                            const glm::mat4& proj,
                                            const TextureManager* textures,
                                            const glm::mat4& model,
                                            const std::vector<uint8_t>* meshVisible) const {
    const GPUGeometry* geom = FindGeometry(handle);
    if (!geom) return;

    // `model` is identity for the plain preview (geometry is already in world
    // space); the sway preview passes a base-pivot bend transform.
    const glm::mat3 normalMat  = glm::mat3(glm::transpose(glm::inverse(model)));

    shader.SetMat4("uModel",     model);
    shader.SetMat3("uNormalMat", normalMat);
    shader.SetMat4("uView",      view);
    shader.SetMat4("uProj",      proj);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Two passes: opaque first, then transparent (shadow decals + additive).
    for (int pass = 0; pass < 2; ++pass) {
        for (size_t mi = 0; mi < geom->meshes.size(); ++mi) {
            const auto& gm = geom->meshes[mi];
            if (meshVisible && mi < meshVisible->size() && !(*meshVisible)[mi]) continue;
            if (gm.blendMode == 1 && !showShadowDecals_) continue; // skip baked shadow decal
            const bool isTransparent = (gm.blendMode == 1 || gm.blendMode == 2);
            if (pass == 0 && isTransparent) continue;
            if (pass == 1 && !isTransparent) continue;

            bool hasTex = false;
            if (textures && gm.textureHash) {
                textures->Bind(gm.textureHash, 0);
                hasTex = textures->FindGL(gm.textureHash) != 0;
            }
            shader.SetBool("uHasTex",   hasTex);
            shader.SetInt ("uBlendMode", gm.blendMode);
            shader.SetInt ("uTex",       0);

            glBindVertexArray(gm.vao);
            glDrawElements(GL_TRIANGLES, gm.indexCount, GL_UNSIGNED_SHORT, nullptr);
        }
    }

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void MeshRenderer::RenderGeometryWireframe(uint64_t handle,
                                            const ShaderProgram& wireShader,
                                            const glm::mat4& view,
                                            const glm::mat4& proj,
                                            const glm::mat4& model) const {
    const GPUGeometry* geom = FindGeometry(handle);
    if (!geom) return;

    wireShader.SetMat4("uModel", model);
    wireShader.SetMat4("uView",  view);
    wireShader.SetMat4("uProj",  proj);
    wireShader.SetVec4("uColor", glm::vec4(0.9f, 0.7f, 0.2f, 0.7f));

#if !defined(__ANDROID__)
    glEnable(GL_POLYGON_OFFSET_LINE);
    glPolygonOffset(-1.0f, -1.0f);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
#endif
    glDisable(GL_CULL_FACE);

    for (const auto& gm : geom->meshes) {
        glBindVertexArray(gm.vao);
#if defined(__ANDROID__)
        // GLES has no glPolygonMode; keep the draw as filled geometry in the
        // initial Android port instead of failing to compile.
#endif
        glDrawElements(GL_TRIANGLES, gm.indexCount, GL_UNSIGNED_SHORT, nullptr);
    }

#if !defined(__ANDROID__)
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glDisable(GL_POLYGON_OFFSET_LINE);
#endif
    glEnable(GL_CULL_FACE);
    glBindVertexArray(0);
}

} // namespace nfsmw
