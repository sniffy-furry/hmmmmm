// ─── ui/CarVinylPanel.cpp ─────────────────────────────────────────────────────
// Browses the car's real VINYLS.BIN decal textures (a standard TPK), with
// thumbnails, metadata, and DDS export. Sources from CarContext::vinylTPKs.
#include "ui/CarVinylPanel.h"
#include "ui/CarPanel.h"
#include "renderer/TextureThumbnail.h"
#include "renderer/GLCompat.h"
#include <vangui.h>
#include <algorithm>

namespace nfsmw {

CarVinylPanel::CarVinylPanel()  = default;
CarVinylPanel::~CarVinylPanel() { /* GL freed via Shutdown() */ }

void CarVinylPanel::Shutdown() {
    ClearThumbCache();
    DestroyPreview();
}

void CarVinylPanel::ClearThumbCache() {
    for (auto& [hash, gl] : thumbCache_)
        if (gl) glDeleteTextures(1, &gl);
    thumbCache_.clear();
}

void CarVinylPanel::DestroyPreview() {
    if (previewGL_) { glDeleteTextures(1, &previewGL_); previewGL_ = 0; }
    previewW_ = previewH_ = 0;
}

void CarVinylPanel::OnCarLoaded(const CarContext& /*ctx*/) {
    selTPK_ = selTex_ = -1;
    ClearThumbCache();
    DestroyPreview();
    status_.clear();
}

uint32_t CarVinylPanel::GetOrCreateThumb(const Texture& tex) {
    auto it = thumbCache_.find(tex.nameHash);
    if (it != thumbCache_.end()) return it->second;
    uint32_t h = CreateThumbnailGL(tex);
    thumbCache_[tex.nameHash] = h;
    return h;
}

void CarVinylPanel::Select(int tpkIdx, int texIdx, const CarContext& ctx) {
    selTPK_ = tpkIdx;
    selTex_ = texIdx;
    DestroyPreview();
    if (tpkIdx < 0 || tpkIdx >= (int)ctx.vinylTPKs.size()) return;
    const auto& tpk = ctx.vinylTPKs[tpkIdx];
    if (texIdx < 0 || texIdx >= (int)tpk.textures.size()) return;

    const auto& tex = tpk.textures[texIdx];
    auto rgba = DecodeTextureToRGBA(tex);
    if (rgba.empty() || tex.width == 0 || tex.height == 0) return;

    glGenTextures(1, &previewGL_);
    glBindTexture(GL_TEXTURE_2D, previewGL_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, tex.width, tex.height, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
    glBindTexture(GL_TEXTURE_2D, 0);
    previewW_ = tex.width;
    previewH_ = tex.height;
}

void CarVinylPanel::Draw(CarContext& ctx, TaskQueue& /*tasks*/) {
    if (!ctx.vinylReady || ctx.vinylTPKs.empty()) {
        VanGui::TextDisabled("No vinyl textures found.");
        VanGui::TextWrapped(
            "This car has no readable VINYLS.BIN, or it contained no textures.");
        return;
    }

    size_t total = 0;
    for (const auto& tpk : ctx.vinylTPKs) total += tpk.textures.size();
    VanGui::Text("Vinyl decals: %zu", total);
    VanGui::Separator();

    const float totalH = VanGui::GetContentRegionAvail().y;
    const float listW  = 240.f;

    // ── Left: decal list with thumbnails ────────────────────────────────────
    (void)VanGui::BeginChild("##vinyllist", VanVec2(listW, totalH), true);
    for (int ti = 0; ti < (int)ctx.vinylTPKs.size(); ++ti) {
        const auto& tpk = ctx.vinylTPKs[ti];
        for (int xi = 0; xi < (int)tpk.textures.size(); ++xi) {
            const auto& tex = tpk.textures[xi];
            VanGui::PushID(ti * 100000 + xi);

            uint32_t thumb = GetOrCreateThumb(tex);
            bool sel = (ti == selTPK_ && xi == selTex_);

            if (thumb) {
                VanGui::Image((VanTextureID)(uintptr_t)thumb, VanVec2(28, 28));
                VanGui::SameLine();
            }
            const std::string label =
                tex.name.empty() ? ("0x" + std::to_string(tex.nameHash)) : tex.name;
            if (VanGui::Selectable(label.c_str(), sel,
                                  VanGuiSelectableFlags_None, VanVec2(0, 28)))
                Select(ti, xi, ctx);

            VanGui::PopID();
        }
    }
    VanGui::EndChild();

    VanGui::SameLine();

    // ── Right: preview + metadata + export ──────────────────────────────────
    (void)VanGui::BeginChild("##vinylpreview", VanVec2(0, totalH), false);
    if (selTPK_ >= 0 && selTex_ >= 0 && selTPK_ < (int)ctx.vinylTPKs.size() &&
        selTex_ < (int)ctx.vinylTPKs[selTPK_].textures.size()) {
        const auto& tex = ctx.vinylTPKs[selTPK_].textures[selTex_];

        if (previewGL_) {
            float aspect = previewH_ > 0 ? (float)previewW_ / (float)previewH_ : 1.f;
            float dispW  = std::min(320.f, VanGui::GetContentRegionAvail().x);
            float dispH  = aspect > 0 ? dispW / aspect : dispW;
            VanGui::Image((VanTextureID)(uintptr_t)previewGL_, {dispW, dispH});
            VanGui::Separator();
        }

        VanGui::Text("Name:   %s", tex.name.empty() ? "(unnamed)" : tex.name.c_str());
        VanGui::Text("Hash:   0x%08X", tex.nameHash);
        VanGui::Text("Size:   %d x %d", tex.width, tex.height);
        VanGui::Text("Format: %s", TexFormatName(tex.format));
        VanGui::Text("Mips:   %d", tex.mipmaps);
        VanGui::Separator();

        if (VanGui::Button("Export DDS...")) {
            exportDialog_.Show("Export Vinyl Decal", FileDialog::Mode::Save, {".dds"},
                [&tex, this](const std::filesystem::path& dest) {
                    auto r = DDSCodec::Export(tex, tex.data, dest);
                    status_ = r ? ("Exported to " + dest.filename().string())
                                : ("Export failed: " + r.error);
                });
        }
        if (!status_.empty()) {
            VanGui::Spacing();
            VanGui::TextWrapped("%s", status_.c_str());
        }
    } else {
        VanGui::TextDisabled("Select a decal to preview it.");
    }
    VanGui::EndChild();

    exportDialog_.Draw();
}

} // namespace nfsmw
