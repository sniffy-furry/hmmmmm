#pragma once
#include "Common.h"
#include <unordered_map>

namespace nfsmw {

class MeshRenderer;
class TextureManager;
class ShaderProgram;

/// Renders small preview images of models into GL textures (offscreen FBO),
/// for the Asset Library. Previews are generated lazily with a small
/// per-frame budget so the UI never hitches, and the cache is LRU-capped.
class ThumbnailCache {
public:
    ~ThumbnailCache();

    /// Reset the per-frame render budget. Call once per frame.
    void BeginFrame() { budget_ = kPerFrameBudget; ++frame_; }

    /// GL texture for a model preview, or 0 if it is not rendered yet
    /// (it will be queued within the per-frame budget).
    /// `geometry` is the MeshRenderer geometry handle, `localBounds` the
    /// model-space bounds used to frame the camera.
    uint32_t Get(uint32_t modelHash, uint64_t geometry, const AABB& localBounds,
                 const MeshRenderer& renderer, const ShaderProgram& shader,
                 const TextureManager* textures);

    /// Drop everything (map reload / shutdown).
    void Clear();

    static constexpr int kSize = 96;            ///< thumbnail edge (px)

private:
    static constexpr int    kPerFrameBudget = 3;
    static constexpr size_t kMaxEntries     = 768;

    struct Entry {
        uint32_t tex      = 0;
        uint64_t lastUsed = 0;
    };

    std::unordered_map<uint32_t, Entry> cache_;   ///< model hash -> texture
    uint32_t fbo_ = 0;
    uint32_t depthRbo_ = 0;
    int      budget_ = 0;
    uint64_t frame_  = 0;

    bool EnsureFBO();
    void EvictIfNeeded();
};

} // namespace nfsmw
