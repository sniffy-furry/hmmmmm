#include "renderer/ThumbnailCache.h"
#include "renderer/MeshRenderer.h"
#include "renderer/ShaderProgram.h"
#include "core/Logger.h"

#include "renderer/GLCompat.h"
#include <algorithm>

namespace nfsmw {

ThumbnailCache::~ThumbnailCache() {
    Clear();
    if (depthRbo_) { glDeleteRenderbuffers(1, &depthRbo_); depthRbo_ = 0; }
    if (fbo_)      { glDeleteFramebuffers(1, &fbo_);       fbo_ = 0; }
}

void ThumbnailCache::Clear() {
    for (auto& [hash, e] : cache_)
        if (e.tex) glDeleteTextures(1, &e.tex);
    cache_.clear();
}

bool ThumbnailCache::EnsureFBO() {
    if (fbo_) return true;
    glGenFramebuffers(1, &fbo_);
    glGenRenderbuffers(1, &depthRbo_);
    glBindRenderbuffer(GL_RENDERBUFFER, depthRbo_);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, kSize, kSize);
    // Issue #8: glRenderbufferStorage can fail (e.g. GL_OUT_OF_MEMORY) and
    // silently leave the renderbuffer with zero size. Check for a GL error
    // and tear everything back down so a later FBO-complete check doesn't
    // pass with a bogus depth attachment.
    const GLenum err = glGetError();
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    if (err != GL_NO_ERROR || fbo_ == 0 || depthRbo_ == 0) {
        LOG_WARN("ThumbnailCache: failed to allocate FBO depth buffer (GL error 0x{:X})", err);
        if (depthRbo_) { glDeleteRenderbuffers(1, &depthRbo_); depthRbo_ = 0; }
        if (fbo_)      { glDeleteFramebuffers(1, &fbo_);       fbo_ = 0; }
        return false;
    }
    return true;
}

void ThumbnailCache::EvictIfNeeded() {
    if (cache_.size() < kMaxEntries) return;
    // Evict the least-recently-used quarter in one sweep.
    std::vector<std::pair<uint64_t, uint32_t>> byAge;   // (lastUsed, hash)
    byAge.reserve(cache_.size());
    for (const auto& [hash, e] : cache_) byAge.push_back({ e.lastUsed, hash });
    std::sort(byAge.begin(), byAge.end());
    const size_t evict = cache_.size() / 4;
    for (size_t i = 0; i < evict; ++i) {
        auto it = cache_.find(byAge[i].second);
        if (it != cache_.end()) {
            if (it->second.tex) glDeleteTextures(1, &it->second.tex);
            cache_.erase(it);
        }
    }
}

uint32_t ThumbnailCache::Get(uint32_t modelHash, uint64_t geometry,
                             const AABB& localBounds,
                             const MeshRenderer& renderer,
                             const ShaderProgram& shader,
                             const TextureManager* textures) {
    if (auto it = cache_.find(modelHash); it != cache_.end()) {
        it->second.lastUsed = frame_;
        return it->second.tex;
    }
    if (budget_ <= 0 || geometry == 0 || !EnsureFBO()) return 0;
    --budget_;
    EvictIfNeeded();

    // Destination texture.
    uint32_t tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, kSize, kSize, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Save state we are about to change.
    GLint prevFbo = 0, prevViewport[4] = {};
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFbo);
    glGetIntegerv(GL_VIEWPORT, prevViewport);

    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D, tex, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                              GL_RENDERBUFFER, depthRbo_);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        glBindFramebuffer(GL_FRAMEBUFFER, (GLuint)prevFbo);
        glDeleteTextures(1, &tex);
        LOG_WARN("ThumbnailCache: FBO incomplete");
        return 0;
    }

    glViewport(0, 0, kSize, kSize);
    glClearColor(0.16f, 0.16f, 0.20f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Frame the model: 3/4 view, Z-up, padded to fit.
    AABB b = localBounds;
    if (!b.Valid()) { b.min = glm::vec3(-1.0f); b.max = glm::vec3(1.0f); }
    const glm::vec3 center  = b.Center();
    const float     radius  = std::max(glm::length(b.Extents()), 0.05f);
    const glm::vec3 viewDir = glm::normalize(glm::vec3(-1.0f, -1.0f, -0.55f));
    const float     dist    = radius * 2.4f;
    const glm::vec3 eye     = center - viewDir * dist;

    const glm::mat4 view = glm::lookAt(eye, center, glm::vec3(0, 0, 1));
    const glm::mat4 proj = glm::perspective(glm::radians(32.0f), 1.0f,
                                            dist * 0.01f, dist * 10.0f);

    shader.Bind();
    shader.SetVec3("uLightDir",   glm::normalize(glm::vec3(0.4f, 0.3f, -0.8f)));
    shader.SetVec3("uLightColor", { 0.9f, 0.87f, 0.8f });
    shader.SetVec3("uAmbient",    { 0.48f, 0.48f, 0.52f });
    shader.SetBool("uSelected",   false);
    shader.SetMat4("uModel",      glm::mat4(1.0f));
    shader.SetFloat("uFogDensity", 0.0f);   // no atmosphere in previews

    renderer.RenderGeometryImmediate(geometry, shader, view, proj, textures);

    // Restore.
    glBindFramebuffer(GL_FRAMEBUFFER, (GLuint)prevFbo);
    glViewport(prevViewport[0], prevViewport[1],
               prevViewport[2], prevViewport[3]);

    cache_[modelHash] = { tex, frame_ };
    return tex;
}

} // namespace nfsmw
