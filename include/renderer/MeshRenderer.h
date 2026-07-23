#pragma once
#include "Common.h"
#include "formats/SolidList.h"
#include "renderer/ShaderProgram.h"
#include <unordered_map>

namespace nfsmw {

class TextureManager;

/// GPU-side representation of a single sub-mesh.
/// Non-copyable: copies would duplicate raw OpenGL handles, leading to
/// double-delete corruption when the original or copy is destroyed.
/// Move-only: the move constructor/assignment zero the source handles.
struct GPUMesh {
    uint32_t vao = 0;
    uint32_t vbo = 0;
    uint32_t ebo = 0;
    uint32_t indexCount  = 0;
    uint32_t vertexCount = 0;
    uint32_t textureHash = 0;
    uint8_t  blendMode   = 0; ///< 0=opaque, 1=shadow decal, 2=additive glow
    bool     uploaded    = false;

    GPUMesh() = default;
    ~GPUMesh() { Delete(); }

    GPUMesh(const GPUMesh&)            = delete;
    GPUMesh& operator=(const GPUMesh&) = delete;

    GPUMesh(GPUMesh&& o) noexcept
        : vao(o.vao), vbo(o.vbo), ebo(o.ebo),
          indexCount(o.indexCount), vertexCount(o.vertexCount),
          textureHash(o.textureHash), blendMode(o.blendMode),
          uploaded(o.uploaded)
    { o.vao = o.vbo = o.ebo = 0; o.uploaded = false; }

    GPUMesh& operator=(GPUMesh&& o) noexcept {
        if (this != &o) {
            Delete();
            vao = o.vao; vbo = o.vbo; ebo = o.ebo;
            indexCount = o.indexCount; vertexCount = o.vertexCount;
            textureHash = o.textureHash; blendMode = o.blendMode;
            uploaded = o.uploaded;
            o.vao = o.vbo = o.ebo = 0; o.uploaded = false;
        }
        return *this;
    }

    void Delete();
};

/// Uploaded geometry for a single SolidObject.
struct GPUGeometry {
    uint64_t              handle = 0;
    std::vector<GPUMesh>  meshes;
    AABB                  localBounds;

    void Delete() { for (auto& m : meshes) m.Delete(); }
};

// ─────────────────────────────────────────────────────────────────────────────
/// Phase 6 preview-only mesh renderer.
/// Manages GPU mesh upload for ObjectExportPanel's live preview viewport.
/// Instance management, picking, raycasting, and LOD are intentionally omitted
/// (see plan.md §6.1: "Drop instance management ... this panel only ever
/// renders a single uploaded geometry at a time").
class MeshRenderer {
public:
    MeshRenderer() = default;
    ~MeshRenderer();

    /// Upload a SolidObject's meshes. Returns a geometry handle (0 on failure).
    uint64_t UploadGeometry(const SolidObject& obj);

    /// Delete a specific geometry by handle. Call when switching the previewed
    /// object to free GPU memory promptly.
    void DeleteGeometry(uint64_t handle);

    /// Remove all uploaded geometry.
    void Clear();

    /// Render one uploaded geometry immediately with the given camera matrices.
    /// Used by ObjectExportPanel's live preview and the thumbnail cache.
    /// `model` lets callers apply a per-frame transform (e.g. the wind-sway bend
    /// preview); it defaults to identity so existing call sites are unchanged.
    void RenderGeometryImmediate(uint64_t handle,
                                 const ShaderProgram& shader,
                                 const glm::mat4& view,
                                 const glm::mat4& proj,
                                 const TextureManager* textures = nullptr,
                                 const glm::mat4& model = glm::mat4(1.0f),
                                 const std::vector<uint8_t>* meshVisible = nullptr) const;

    /// Render the same geometry as a wireframe overlay (second pass).
    void RenderGeometryWireframe(uint64_t handle,
                                 const ShaderProgram& wireShader,
                                 const glm::mat4& view,
                                 const glm::mat4& proj,
                                 const glm::mat4& model = glm::mat4(1.0f)) const;

    const GPUGeometry* FindGeometry(uint64_t handle) const;
    size_t GeometryCount() const { return geometries_.size(); }

    /// Skip drawing blendMode==1 ("shadow decal") meshes - the flat baked
    /// blob-shadow plane bundled in the original car/object meshes. Additive
    /// glow (blendMode==2, e.g. brake lights) is unaffected. Off by default:
    /// the real-time preview shows geometry/textures without the baked
    /// shadow decal underneath.
    void SetShowShadowDecals(bool show) { showShadowDecals_ = show; }
    bool ShowShadowDecals() const { return showShadowDecals_; }

private:
    std::unordered_map<uint64_t, GPUGeometry> geometries_;
    uint64_t nextHandle_ = 1;
    bool showShadowDecals_ = false;

    static GPUMesh UploadMesh(const SolidMesh& mesh);
};

} // namespace nfsmw
