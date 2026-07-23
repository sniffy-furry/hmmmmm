// ─── ui/CarTexturePanel.cpp ───────────────────────────────────────────────────
#include "ui/CarTexturePanel.h"
#include "ui/CarPanel.h"
#include "renderer/TextureThumbnail.h"
#include <vangui.h>
#include "renderer/GLCompat.h"

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

CarTexturePanel::CarTexturePanel()  = default;
CarTexturePanel::~CarTexturePanel() { DestroyGLTexture(); ClearThumbCache(); }

void CarTexturePanel::Shutdown() { DestroyGLTexture(); ClearThumbCache(); }

void CarTexturePanel::OnCarLoaded(const CarContext& /*ctx*/) {
    selTPK_     = -1;
    selTexture_ = -1;
    DestroyGLTexture();
    ClearThumbCache();
    writeStatus_.clear();
}

// ── GL helpers ────────────────────────────────────────────────────────────────

void CarTexturePanel::UploadGLTexture(const Texture& tex) {
    DestroyGLTexture();
    if (tex.data.empty() || tex.width == 0 || tex.height == 0)
        return;

    glGenTextures(1, &previewGL_);
    glBindTexture(GL_TEXTURE_2D, previewGL_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    bool ok = true;
    switch (tex.format) {
        case TexFormat::ARGB32: {
            // NFS stores pixels as A8R8G8B8 (BGRA in memory) — upload as
            // GL_BGRA rather than swizzling channels ourselves.
            const size_t needed = size_t(tex.width) * tex.height * 4;
            if (tex.data.size() < needed) { ok = false; break; }
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, tex.width, tex.height,
                         0, GL_BGRA, GL_UNSIGNED_BYTE, tex.data.data());
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glGenerateMipmap(GL_TEXTURE_2D);
            break;
        }
        case TexFormat::DXT1:
        case TexFormat::DXT3:
        case TexFormat::DXT5: {
            GLenum internalFmt = 0;
            switch (tex.format) {
                case TexFormat::DXT1: internalFmt = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT; break;
                case TexFormat::DXT3: internalFmt = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT; break;
                case TexFormat::DXT5: internalFmt = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT; break;
                default: break;
            }
            // Only the top mip level is shown in the preview.
            glCompressedTexImage2D(GL_TEXTURE_2D, 0, internalFmt,
                                   tex.width, tex.height, 0,
                                   (GLsizei)tex.data.size(), tex.data.data());
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            break;
        }
        default:
            ok = false;
            break;
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    if (!ok) {
        glDeleteTextures(1, &previewGL_);
        previewGL_ = 0;
        previewW_ = previewH_ = 0;
        return;
    }
    previewW_ = tex.width;
    previewH_ = tex.height;
}

void CarTexturePanel::DestroyGLTexture() {
    if (previewGL_) { glDeleteTextures(1, &previewGL_); previewGL_ = 0; }
    previewW_ = previewH_ = 0;
}

uint32_t CarTexturePanel::GetOrCreateThumb(const Texture& tex) {
    auto it = thumbCache_.find(tex.nameHash);
    if (it != thumbCache_.end()) return it->second;
    // Real DXT-decode → box-filter → GL-upload via the shared helper (P1-5).
    // Returns 0 for PAL8/Unknown/empty, in which case the grid falls back to a
    // plain text row; we still cache the 0 so we don't retry every frame.
    uint32_t handle = CreateThumbnailGL(tex);
    thumbCache_[tex.nameHash] = handle;
    return handle;
}

void CarTexturePanel::ClearThumbCache() {
    for (auto& [h, gl] : thumbCache_)
        if (gl) glDeleteTextures(1, &gl);
    thumbCache_.clear();
}

void CarTexturePanel::SelectTexture(int tpkIdx, int texIdx, const CarContext& ctx) {
    selTPK_     = tpkIdx;
    selTexture_ = texIdx;
    if (tpkIdx < 0 || tpkIdx >= (int)ctx.bodyTPKs.size()) return;
    const auto& tpk = ctx.bodyTPKs[tpkIdx];
    if (texIdx < 0 || texIdx >= (int)tpk.textures.size()) return;
    UploadGLTexture(tpk.textures[texIdx]);
}

// ── Draw ──────────────────────────────────────────────────────────────────────

void CarTexturePanel::Draw(CarContext& ctx, TaskQueue& tasks) {
    if (!ctx.texturesReady) {
        VanGui::TextDisabled("Textures not loaded.");
        return;
    }
    if (ctx.bodyTPKs.empty()) {
        VanGui::TextDisabled("No texture packs in body BUN.");
        return;
    }

    const float totalH = VanGui::GetContentRegionAvail().y;
    const float treeW  = 220.f;

    // ── Tree (left) ───────────────────────────────────────────────────────
    (void)VanGui::BeginChild("##texbrowser", VanVec2(treeW, totalH), true);
    for (int ti = 0; ti < (int)ctx.bodyTPKs.size(); ++ti) {
        const auto& tpk = ctx.bodyTPKs[ti];
        char label[64];
        std::snprintf(label, sizeof(label), "TPK %d (%zu textures)##tpk%d",
                      ti, tpk.textures.size(), ti);
        if (VanGui::TreeNodeEx(label, VanGuiTreeNodeFlags_DefaultOpen)) {
            for (int xi = 0; xi < (int)tpk.textures.size(); ++xi) {
                const auto& tex = tpk.textures[xi];
                bool sel = (ti == selTPK_ && xi == selTexture_);
                if (VanGui::Selectable(tex.name.c_str(), sel))
                    SelectTexture(ti, xi, ctx);
            }
            VanGui::TreePop();
        }
    }
    VanGui::EndChild();

    VanGui::SameLine();

    // ── Preview / metadata (right) ────────────────────────────────────────
    (void)VanGui::BeginChild("##texpreview", VanVec2(0, totalH), false);

    if (selTPK_ >= 0 && selTexture_ >= 0 &&
        selTPK_ < (int)ctx.bodyTPKs.size()) {
        const auto& tex = ctx.bodyTPKs[selTPK_].textures[selTexture_];
        DrawMetadataCard(tex, ctx, tasks);
    } else {
        VanGui::TextDisabled("Select a texture.");
    }

    VanGui::EndChild();

    // Repack confirmation dialog
    if (showRepackConfirm_)
        DrawRepackConfirmDialog(ctx, tasks);
}

void CarTexturePanel::DrawMetadataCard(const Texture& tex, CarContext& ctx,
                                        TaskQueue& tasks) {
    // Preview image
    if (previewGL_) {
        float aspect = previewH_ > 0 ? (float)previewW_ / (float)previewH_ : 1.f;
        float dispW  = std::min(256.f, VanGui::GetContentRegionAvail().x);
        float dispH  = dispW / aspect;
        VanGui::Image((VanTextureID)(uintptr_t)previewGL_,
                     {dispW, dispH});
    }
    VanGui::Separator();

    // Metadata
    VanGui::Text("Name:   %s",   tex.name.c_str());
    VanGui::Text("Hash:   0x%08X", tex.nameHash);
    VanGui::Text("Size:   %d x %d", tex.width, tex.height);
    VanGui::Text("Format: %s",   TexFormatName(tex.format));
    VanGui::Text("Mips:   %d",   tex.mipmaps);

    VanGui::Separator();

    // Export
    if (VanGui::Button("Export DDS...")) {
        exportDialog_.Show("Export Texture", FileDialog::Mode::Save, {".dds"},
            [&tex](const std::filesystem::path& dest) {
                DDSCodec::Export(tex, tex.data, dest);
            });
    }

    // Replace
    VanGui::SameLine();
    if (VanGui::Button("Replace from DDS...")) {
        replaceDialog_.Show("Replace Texture", FileDialog::Mode::Open, {".dds"},
            [this, &ctx, &tasks](const std::filesystem::path& src) {
                DoReplace(selTPK_, selTexture_, src, ctx, tasks);
            });
    }

    if (!writeStatus_.empty()) {
        VanGui::Spacing();
        VanGui::TextWrapped("%s", writeStatus_.c_str());
    }
}

void CarTexturePanel::DoReplace(int tpkIdx, int texIdx,
                                 const std::filesystem::path& ddsPath,
                                 CarContext& ctx, TaskQueue& tasks) {
    if (tpkIdx < 0 || tpkIdx >= (int)ctx.bodyTPKs.size()) return;
    const auto& tex = ctx.bodyTPKs[tpkIdx].textures[texIdx];
    auto bodyBUN    = ctx.carDir / (ctx.id + ".BUN");

    ReplaceRequest req{ tex.nameHash, ddsPath };
    tasks.Submit("Replacing texture", [this, bodyBUN, req](ProgressState&) {
        backup_.EnsureFileBak(bodyBUN);
        auto results = repacker_.PatchFile(bodyBUN, {req}, backup_);
        if (!results.empty() && results[0].ok)
            writeStatus_ = "Replaced successfully.";
        else if (!results.empty())
            writeStatus_ = "Replace failed: " + results[0].error;
    });
}

void CarTexturePanel::DrawRepackConfirmDialog(CarContext& ctx, TaskQueue& tasks) {
    VanGui::OpenPopup("Repack required");
    if (VanGui::BeginPopupModal("Repack required", nullptr,
                               VanGuiWindowFlags_AlwaysAutoResize)) {
        VanGui::TextWrapped(
            "The replacement DDS is larger than the original slot.\n"
            "A full repack of the body BUN is required.\n"
            "This may increase the file size.");
        VanGui::Spacing();
        if (VanGui::Button("Repack")) {
            DoReplace(confirmTPK_, confirmTex_, pendingReplaceDDS_, ctx, tasks);
            showRepackConfirm_ = false;
            VanGui::CloseCurrentPopup();
        }
        VanGui::SameLine();
        if (VanGui::Button("Cancel")) {
            showRepackConfirm_ = false;
            VanGui::CloseCurrentPopup();
        }
        VanGui::EndPopup();
    }
}

} // namespace nfsmw
