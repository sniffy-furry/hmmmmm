#include "renderer/TextureManager.h"
#include "core/Logger.h"

#include "renderer/GLCompat.h"
#include <algorithm>

// S3TC constants (from GL_EXT_texture_compression_s3tc; not in core glad headers)
#ifndef GL_COMPRESSED_RGBA_S3TC_DXT1_EXT
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT 0x83F1
#endif
#ifndef GL_COMPRESSED_RGBA_S3TC_DXT3_EXT
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT 0x83F2
#endif
#ifndef GL_COMPRESSED_RGBA_S3TC_DXT5_EXT
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT 0x83F3
#endif

namespace nfsmw {

namespace {

size_t DXTLevelSize(TexFormat fmt, uint32_t w, uint32_t h) {
    const uint32_t bw = (w + 3) / 4, bh = (h + 3) / 4;
    const uint32_t blockBytes = (fmt == TexFormat::DXT1) ? 8 : 16;
    return size_t(bw) * bh * blockBytes;
}

GLenum DXTFormatToGL(TexFormat fmt) {
    switch (fmt) {
        case TexFormat::DXT1: return GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
        case TexFormat::DXT3: return GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
        case TexFormat::DXT5: return GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
        default:              return 0;
    }
}

} // namespace

TextureManager::~TextureManager() {
    Clear();
}

// static
uint32_t TextureManager::UploadTexture(const Texture& tex) {
    if (tex.width == 0 || tex.height == 0 || tex.data.empty())
        return 0;

    GLuint handle = 0;
    glGenTextures(1, &handle);
    glBindTexture(GL_TEXTURE_2D, handle);

    int levelsUploaded = 0;

    if (tex.format == TexFormat::DXT1 || tex.format == TexFormat::DXT3 ||
        tex.format == TexFormat::DXT5) {
        const GLenum glFmt = DXTFormatToGL(tex.format);
        size_t offset = 0;
        uint32_t w = tex.width, h = tex.height;
        for (int level = 0; level < tex.mipmaps; ++level) {
            const size_t levelSize = DXTLevelSize(tex.format, w, h);
            if (offset + levelSize > tex.data.size()) break;
            glCompressedTexImage2D(GL_TEXTURE_2D, level, glFmt,
                                   (GLsizei)w, (GLsizei)h, 0,
                                   (GLsizei)levelSize, tex.data.data() + offset);
            offset += levelSize;
            ++levelsUploaded;
            if (w == 1 && h == 1) break;
            w = std::max(1u, w >> 1);
            h = std::max(1u, h >> 1);
        }
    } else if (tex.format == TexFormat::ARGB32) {
        const size_t needed = size_t(tex.width) * tex.height * 4;
        if (tex.data.size() >= needed) {
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, tex.width, tex.height, 0,
                         GL_BGRA, GL_UNSIGNED_BYTE, tex.data.data());
            levelsUploaded = 1;
        }
    } else {
        // PAL8 / unknown: not GPU-uploaded by the editor.
        glDeleteTextures(1, &handle);
        return 0;
    }

    if (levelsUploaded == 0) {
        glDeleteTextures(1, &handle);
        return 0;
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    if (levelsUploaded > 1) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, levelsUploaded - 1);
    } else {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
    }
    glBindTexture(GL_TEXTURE_2D, 0);

    return handle;
}

uint32_t TextureManager::Upload(Texture& tex) {
    auto it = cache_.find(tex.nameHash);
    if (it != cache_.end()) {
        // Issue #14 fix: log CRC32 hash collisions so they are visible in debug
        // output. Two textures from different packs can share a nameHash; the
        // second silently reuses the first one's GL handle.
        if (!tex.name.empty()) {
            LOG_DEBUG("TextureManager: hash 0x{:08X} already cached ('{}'); reusing existing GL handle",
                      tex.nameHash, tex.name);
        }
        tex.glHandle = it->second;
        return it->second;
    }

    uint32_t handle = UploadTexture(tex);
    if (handle) {
        cache_[tex.nameHash] = handle;
        tex.glHandle = handle;
    }
    return handle;
}

void TextureManager::UploadBlock(TPKBlock& block) {
    size_t ok = 0;
    for (auto& tex : block.textures)
        if (Upload(tex)) ++ok;
    LOG_DEBUG("TextureManager: uploaded {}/{} textures from '{}'",
              ok, block.textures.size(), block.name);
}

uint32_t TextureManager::Replace(Texture& tex) {
    auto it = cache_.find(tex.nameHash);
    if (it != cache_.end()) {
        glDeleteTextures(1, &it->second);
        cache_.erase(it);
    }
    tex.glHandle = 0;
    return Upload(tex);
}

uint32_t TextureManager::FindGL(uint32_t nameHash) const {
    auto it = cache_.find(nameHash);
    return it != cache_.end() ? it->second : 0;
}

void TextureManager::Bind(uint32_t nameHash, int unit) const {
    uint32_t handle = FindGL(nameHash);
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, handle);
}

void TextureManager::Clear() {
    for (auto& [hash, handle] : cache_)
        glDeleteTextures(1, &handle);
    cache_.clear();
}

} // namespace nfsmw
