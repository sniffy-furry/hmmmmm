// ─── ui/UIFrontendPanel.cpp ───────────────────────────────────────────────────
// Phase 8 — FRONTEND/*.BUN texture browse / export / replace.
//
// Code pattern follows CarTexturePanel verbatim; the only structural difference
// is multi-file support (files_ vector instead of a single CarContext).
// ─────────────────────────────────────────────────────────────────────────────
#include "ui/UIFrontendPanel.h"
#include "core/Logger.h"
#include <vangui.h>
#include <vangui_notify.h>
#include "renderer/GLCompat.h"
#include <algorithm>
#include <cstdio>

namespace nfsmw {

// ─── DecodeToRGBA (local, same as TexturePackPanel) ───────────────────────────
// A full DXT1/3/5/ARGB32 software decoder is embedded in TexturePackPanel.cpp
// and duplicated here to keep each panel self-contained.  If a shared utility
// header is introduced later both callers can be redirected.
namespace {

std::vector<uint8_t> DecodeToRGBA(const Texture& tex)
{
    if (tex.data.empty() || tex.width == 0 || tex.height == 0) return {};
    const uint32_t W = tex.width, H = tex.height;
    std::vector<uint8_t> rgba(W * H * 4, 0xFF);

    if (tex.format == TexFormat::ARGB32) {
        const size_t pixels = W * H;
        for (size_t i = 0; i < pixels && (i + 1) * 4 <= tex.data.size(); ++i) {
            const uint8_t* s = tex.data.data() + i * 4;
            uint8_t*       d = rgba.data() + i * 4;
            d[0]=s[1]; d[1]=s[2]; d[2]=s[3]; d[3]=s[0];
        }
        return rgba;
    }

    const uint32_t bw = (W+3)/4, bh = (H+3)/4;
    const size_t blkB = (tex.format == TexFormat::DXT1) ? 8 : 16;
    if (tex.data.size() < bw * bh * blkB) return rgba;

    auto u565 = [](uint16_t v, uint8_t& r, uint8_t& g, uint8_t& b){
        r = uint8_t(((v>>11)&0x1F)*255/31);
        g = uint8_t(((v>> 5)&0x3F)*255/63);
        b = uint8_t(((v    )&0x1F)*255/31);
    };

    for (uint32_t by=0; by<bh; ++by) for (uint32_t bx=0; bx<bw; ++bx) {
        const uint8_t* blk = tex.data.data() + (by*bw+bx)*blkB;
        uint8_t alpha[16]; std::fill(alpha,alpha+16,0xFFu);

        if (tex.format==TexFormat::DXT3) {
            for (int i=0;i<8;++i){alpha[i*2]=(blk[i]&0x0F)*17;alpha[i*2+1]=((blk[i]>>4)&0x0F)*17;}
            blk+=8;
        } else if (tex.format==TexFormat::DXT5) {
            uint8_t a0=blk[0],a1=blk[1]; uint64_t c=0;
            for(int i=2;i<8;++i) c|=(uint64_t)blk[i]<<((i-2)*8);
            uint8_t at[8]; at[0]=a0; at[1]=a1;
            if(a0>a1){for(int i=1;i<=6;++i) at[i+1]=(uint8_t)(((7-i)*a0+i*a1)/7);}
            else{for(int i=1;i<=4;++i) at[i+1]=(uint8_t)(((5-i)*a0+i*a1)/5);at[6]=0;at[7]=255;}
            for(int i=0;i<16;++i) alpha[i]=at[(c>>(i*3))&7];
            blk+=8;
        }

        uint16_t c0,c1; std::memcpy(&c0,blk,2); std::memcpy(&c1,blk+2,2);
        uint32_t idx; std::memcpy(&idx,blk+4,4);
        uint8_t cr[4],cg[4],cb[4],ca[4];
        u565(c0,cr[0],cg[0],cb[0]); ca[0]=0xFF;
        u565(c1,cr[1],cg[1],cb[1]); ca[1]=0xFF;
        if(tex.format==TexFormat::DXT1){
            if(c0>c1){cr[2]=(2*cr[0]+cr[1])/3;cg[2]=(2*cg[0]+cg[1])/3;cb[2]=(2*cb[0]+cb[1])/3;ca[2]=0xFF;
                      cr[3]=(cr[0]+2*cr[1])/3;cg[3]=(cg[0]+2*cg[1])/3;cb[3]=(cb[0]+2*cb[1])/3;ca[3]=0xFF;}
            else{cr[2]=(cr[0]+cr[1])/2;cg[2]=(cg[0]+cg[1])/2;cb[2]=(cb[0]+cb[1])/2;ca[2]=0xFF;cr[3]=cg[3]=cb[3]=0;ca[3]=0;}
        } else {
            cr[2]=(2*cr[0]+cr[1])/3;cg[2]=(2*cg[0]+cg[1])/3;cb[2]=(2*cb[0]+cb[1])/3;ca[2]=0xFF;
            cr[3]=(cr[0]+2*cr[1])/3;cg[3]=(cg[0]+2*cg[1])/3;cb[3]=(cb[0]+2*cb[1])/3;ca[3]=0xFF;
        }
        for(int py=0;py<4;++py) for(int px=0;px<4;++px){
            uint32_t ox=bx*4+px, oy=by*4+py;
            if(ox>=W||oy>=H) continue;
            int i=(idx>>((py*4+px)*2))&3, pi=py*4+px;
            uint8_t* d=rgba.data()+(oy*W+ox)*4;
            d[0]=cr[i];d[1]=cg[i];d[2]=cb[i];
            d[3]=(tex.format==TexFormat::DXT1)?ca[i]:alpha[pi];
        }
    }
    return rgba;
}

} // namespace

// ─── Constructor / Destructor ─────────────────────────────────────────────────

UIFrontendPanel::UIFrontendPanel()  = default;
UIFrontendPanel::~UIFrontendPanel() { DestroyGLTexture(); ClearThumbCache(); }

void UIFrontendPanel::Shutdown() { DestroyGLTexture(); ClearThumbCache(); }

// ─── Open ─────────────────────────────────────────────────────────────────────

void UIFrontendPanel::Open(const std::filesystem::path& path, TaskQueue& tasks)
{
    // Avoid loading the same file twice.
    for (const auto& f : files_)
        if (f.path == path) { return; }

    LoadedFile lf;
    lf.path    = path;
    lf.stem    = path.stem().string();
    lf.loading = true;
    files_.push_back(std::move(lf));
    const int idx = static_cast<int>(files_.size()) - 1;

    tasks.Submit("Loading " + path.filename().string(),
        [this, path, idx](nfsmw::ProgressState&)
        {
            auto res = StreamBundleLoader::Load(path);
            if (!res) {
                LOG_WARN("UIFrontendPanel: load error '{}': {}", path.string(), res.error);
                files_[idx].loading = false;
                return;
            }
            files_[idx].tpks    = std::move(res.value.texturePacks);
            files_[idx].loading = false;
            LOG_INFO("UIFrontendPanel: '{}' — {} TPK(s)",
                     path.filename().string(), files_[idx].tpks.size());
        });
}

// ─── GL helpers ───────────────────────────────────────────────────────────────

void UIFrontendPanel::UploadGLTexture(const Texture& tex)
{
    DestroyGLTexture();
    auto rgba = DecodeToRGBA(tex);
    if (rgba.empty()) return;

    GLuint h = 0;
    glGenTextures(1, &h);
    glBindTexture(GL_TEXTURE_2D, h);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, tex.width, tex.height,
                 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
    glBindTexture(GL_TEXTURE_2D, 0);

    previewGL_ = h;
    previewW_  = tex.width;
    previewH_  = tex.height;
}

void UIFrontendPanel::DestroyGLTexture()
{
    if (previewGL_) { glDeleteTextures(1, &previewGL_); previewGL_ = 0; }
    previewW_ = previewH_ = 0;
}

uint32_t UIFrontendPanel::GetOrCreateThumb(const Texture& tex)
{
    if (tex.format == TexFormat::PAL8 || tex.format == TexFormat::Unknown) return 0;
    if (tex.data.empty()) return 0;

    auto it = thumbCache_.find(tex.nameHash);
    if (it != thumbCache_.end()) return it->second;

    auto rgba = DecodeToRGBA(tex);
    if (rgba.empty()) { thumbCache_[tex.nameHash] = 0; return 0; }

    constexpr int kT = 32;
    const int sw = tex.width, sh = tex.height;
    std::vector<uint8_t> thumb(kT * kT * 4, 0);
    for (int ty=0; ty<kT; ++ty) for (int tx=0; tx<kT; ++tx) {
        int sx0=tx*sw/kT, sx1=(tx+1)*sw/kT;
        int sy0=ty*sh/kT, sy1=(ty+1)*sh/kT;
        uint32_t r=0,g=0,b=0,a=0,n=0;
        for (int sy=sy0; sy<std::max(sy0+1,sy1); ++sy)
            for (int sx=sx0; sx<std::max(sx0+1,sx1); ++sx) {
                if(sx>=sw||sy>=sh) continue;
                const uint8_t* p=rgba.data()+(sy*sw+sx)*4;
                r+=p[0];g+=p[1];b+=p[2];a+=p[3];++n;
            }
        if(!n) n=1;
        uint8_t* d=thumb.data()+(ty*kT+tx)*4;
        d[0]=static_cast<uint8_t>(r/n); d[1]=static_cast<uint8_t>(g/n); d[2]=static_cast<uint8_t>(b/n); d[3]=static_cast<uint8_t>(a/n);
    }

    GLuint h=0;
    glGenTextures(1,&h);
    glBindTexture(GL_TEXTURE_2D,h);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,kT,kT,0,GL_RGBA,GL_UNSIGNED_BYTE,thumb.data());
    glBindTexture(GL_TEXTURE_2D,0);

    thumbCache_[tex.nameHash] = h;
    return h;
}

void UIFrontendPanel::ClearThumbCache()
{
    for (auto& [h,gl] : thumbCache_) if(gl) glDeleteTextures(1,&gl);
    thumbCache_.clear();
}

// ─── SelectTexture ────────────────────────────────────────────────────────────

void UIFrontendPanel::SelectTexture(int fi, int ti, int xi)
{
    selFile_    = fi;
    selTPK_     = ti;
    selTexture_ = xi;
    if (fi < 0 || fi >= (int)files_.size()) return;
    const auto& tpks = files_[fi].tpks;
    if (ti < 0 || ti >= (int)tpks.size()) return;
    const auto& tpk  = tpks[ti];
    if (xi < 0 || xi >= (int)tpk.textures.size()) return;
    UploadGLTexture(tpk.textures[xi]);
}

// ─── DoExport ─────────────────────────────────────────────────────────────────

void UIFrontendPanel::DoExport(int fi, int ti, int xi,
                                const std::filesystem::path& dest)
{
    if (fi<0||fi>=(int)files_.size()) return;
    const auto& tpks = files_[fi].tpks;
    if (ti<0||ti>=(int)tpks.size()) return;
    const auto& tex  = tpks[ti].textures[xi];
    auto r = DDSCodec::Export(tex, std::span<const uint8_t>(tex.data), dest);
    writeStatus_ = r ? "Exported: " + dest.filename().string()
                     : "Export failed: " + r.error;
    if (!r) VanGui::NotifyError("%s", std::string(r.error).c_str());
}

// ─── DoReplace ────────────────────────────────────────────────────────────────

void UIFrontendPanel::DoReplace(int fi, int ti, int xi,
                                 const std::filesystem::path& ddsPath,
                                 TaskQueue& tasks)
{
    if (fi<0||fi>=(int)files_.size()) return;
    const auto& tpks = files_[fi].tpks;
    if (ti<0||ti>=(int)tpks.size()) return;
    const auto& tex = tpks[ti].textures[xi];
    const auto  src = files_[fi].path;

    ReplaceRequest req{ tex.nameHash, ddsPath };
    tasks.Submit("Replacing " + tex.name,
        [this, src, req](nfsmw::ProgressState&)
        {
            backup_.EnsureFileBak(src);
            auto results = repacker_.PatchFile(src, {req}, backup_);
            if (!results.empty() && results[0].ok) {
                writeStatus_ = "Replaced: " + req.ddsPath.filename().string();
                VanGui::NotifySuccess("%s", std::string(writeStatus_).c_str());
            } else {
                const std::string err = results.empty() ? "unknown error" : results[0].error;
                writeStatus_ = "Replace failed: " + err;
                VanGui::NotifyError("%s", std::string(writeStatus_).c_str());
            }
        });
}

// ─── Draw ─────────────────────────────────────────────────────────────────────

void UIFrontendPanel::Draw(TaskQueue& tasks)
{
    if (files_.empty()) {
        VanGui::TextDisabled("No Frontend BUN loaded.");
        VanGui::TextDisabled("Use Open UI/HUD to load a file from FRONTEND/.");
        return;
    }

    const float totalH = VanGui::GetContentRegionAvail().y;
    const float treeW  = 240.f;

    (void)VanGui::BeginChild("##fe_tree", VanVec2(treeW, totalH), true);
    DrawTree(tasks);
    VanGui::EndChild();

    VanGui::SameLine();

    (void)VanGui::BeginChild("##fe_preview", VanVec2(0, totalH), false);
    DrawPreview(tasks);
    VanGui::EndChild();

    if (showRepackConfirm_)
        DrawRepackConfirmDialog(tasks);
}

void UIFrontendPanel::DrawTree(TaskQueue& /*tasks*/)
{
    filter_.Draw("##fe_filter", "Filter textures…");
    VanGui::Separator();

    for (int fi = 0; fi < (int)files_.size(); ++fi) {
        const auto& lf = files_[fi];
        if (lf.loading) {
            VanGui::TextDisabled("[loading] %s", lf.stem.c_str());
            continue;
        }

        char fileLabel[128];
        std::snprintf(fileLabel, sizeof(fileLabel), "%s##file%d", lf.stem.c_str(), fi);

        if (VanGui::TreeNodeEx(fileLabel, VanGuiTreeNodeFlags_DefaultOpen)) {
            for (int ti = 0; ti < (int)lf.tpks.size(); ++ti) {
                const auto& tpk = lf.tpks[ti];
                char tpkLabel[64];
                std::snprintf(tpkLabel, sizeof(tpkLabel),
                              "TPK %d (%zu)##tpk%d_%d", ti, tpk.textures.size(), fi, ti);

                if (VanGui::TreeNodeEx(tpkLabel, VanGuiTreeNodeFlags_DefaultOpen)) {
                    for (int xi = 0; xi < (int)tpk.textures.size(); ++xi) {
                        const auto& tex = tpk.textures[xi];
                        if (!filter_.Matches(tex.name)) continue;

                        // Tiny thumbnail inline
                        uint32_t thumb = GetOrCreateThumb(tex);
                        if (thumb)
                            VanGui::Image((VanTextureID)(uintptr_t)thumb,
                                         VanVec2(16, 16));
                        else
                            VanGui::Dummy(VanVec2(16, 16));
                        VanGui::SameLine();

                        bool sel = (fi == selFile_ && ti == selTPK_ && xi == selTexture_);
                        if (VanGui::Selectable(tex.name.c_str(), sel,
                                              VanGuiSelectableFlags_AllowOverlap))
                            SelectTexture(fi, ti, xi);
                    }
                    VanGui::TreePop();
                }
            }
            VanGui::TreePop();
        }
    }
}

void UIFrontendPanel::DrawPreview(TaskQueue& tasks)
{
    if (selFile_ < 0 || selFile_ >= (int)files_.size() ||
        selTPK_  < 0 || selTexture_ < 0) {
        VanGui::TextDisabled("Select a texture.");
        return;
    }
    const auto& tpks = files_[selFile_].tpks;
    if (selTPK_ >= (int)tpks.size()) return;
    const auto& tpk = tpks[selTPK_];
    if (selTexture_ >= (int)tpk.textures.size()) return;

    DrawMetadataCard(tpk.textures[selTexture_], selFile_, selTPK_, selTexture_, tasks);
}

void UIFrontendPanel::DrawMetadataCard(const Texture& tex, int fi, int ti, int xi,
                                        TaskQueue& tasks)
{
    // Preview image
    if (previewGL_) {
        float aspect = previewH_ > 0 ? (float)previewW_ / (float)previewH_ : 1.f;
        float dispW  = std::min(320.f, VanGui::GetContentRegionAvail().x);
        float dispH  = dispW / aspect;
        VanGui::Image((VanTextureID)(uintptr_t)previewGL_,
                     {dispW, dispH});
    }
    VanGui::Separator();

    VanGui::Text("Name:   %s",     tex.name.c_str());
    VanGui::Text("Hash:   0x%08X", tex.nameHash);
    VanGui::Text("Size:   %d x %d", tex.width, tex.height);
    VanGui::Text("Format: %s",     TexFormatName(tex.format));
    VanGui::Text("Mips:   %d",     tex.mipmaps);

    VanGui::Separator();

    if (VanGui::Button("Export DDS…")) {
        if (fileDialog_)
            fileDialog_->Show("Export Texture", FileDialog::Mode::Save,
                {".dds"},
                [this, fi, ti, xi](const std::filesystem::path& dest){
                    DoExport(fi, ti, xi, dest);
                },
                tex.name + ".dds");
    }
    VanGui::SameLine();
    if (VanGui::Button("Replace from DDS…")) {
        if (fileDialog_)
            fileDialog_->Show("Replace Texture", FileDialog::Mode::Open,
                {".dds"},
                [this, fi, ti, xi, &tasks](const std::filesystem::path& src){
                    DoReplace(fi, ti, xi, src, tasks);
                });
    }

    if (!writeStatus_.empty()) {
        VanGui::Spacing();
        VanGui::TextWrapped("%s", writeStatus_.c_str());
    }
}

void UIFrontendPanel::DrawRepackConfirmDialog(TaskQueue& tasks)
{
    VanGui::OpenPopup("Repack required##fe");
    if (VanGui::BeginPopupModal("Repack required##fe", nullptr,
                               VanGuiWindowFlags_AlwaysAutoResize)) {
        VanGui::TextWrapped(
            "The replacement DDS is larger than the original slot.\n"
            "A full repack of the BUN is required.\n"
            "This may increase the file size.");
        VanGui::Spacing();
        if (VanGui::Button("Repack")) {
            DoReplace(confirmFile_, confirmTPK_, confirmTex_,
                      pendingReplaceDDS_, tasks);
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
