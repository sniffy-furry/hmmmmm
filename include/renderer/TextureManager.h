#pragma once
#include "Common.h"
#include "formats/TPKBlock.h"
#include <unordered_map>

namespace nfsmw {

/// Manages OpenGL texture objects. Uploads DXT/ARGB data, caches by hash.
class TextureManager {
public:
    TextureManager()  = default;
    ~TextureManager();

    /// Upload a Texture's data to OpenGL. Returns GL handle (0 on failure).
    uint32_t Upload(Texture& tex);

    /// Upload all textures in a TPKBlock.
    void UploadBlock(TPKBlock& block);

    /// Replace an existing GPU texture with new pixel data (hot-swap).
    uint32_t Replace(Texture& tex);

    /// Find a GL handle by Joaat hash. Returns 0 if not found.
    uint32_t FindGL(uint32_t nameHash) const;

    /// Bind a texture to a texture unit (0-based).
    void Bind(uint32_t nameHash, int unit = 0) const;

    /// Release all GL textures.
    void Clear();

    size_t Count() const { return cache_.size(); }

private:
    std::unordered_map<uint32_t, uint32_t> cache_;  ///< nameHash → GL handle

    static uint32_t UploadTexture(const Texture& tex);
};

} // namespace nfsmw
