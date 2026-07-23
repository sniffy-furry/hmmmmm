// ─── ui/UIHudAtlasPanel.cpp ───────────────────────────────────────────────────
// Phase 8 — GLOBALHUD.BUN texture atlas browser + HUD element table.
// ─────────────────────────────────────────────────────────────────────────────
#include "ui/UIHudAtlasPanel.h"
#include "core/Logger.h"
#include "core/LZCDecompressor.h"
#include <vangui.h>
#include <vangui_notify.h>
#include "renderer/GLCompat.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <span>

namespace nfsmw {

namespace {

// DXT software decoder — same inline helper as UIFrontendPanel.
std::vector<uint8_t> DecodeToRGBA(const Texture& tex)
{
    if (tex.data.empty() || tex.width == 0 || tex.height == 0) return {};
    const uint32_t W = tex.width, H = tex.height;
    std::vector<uint8_t> rgba(W * H * 4, 0xFF);

    if (tex.format == TexFormat::ARGB32) {
        for (size_t i = 0, n = W*H; i < n && (i+1)*4 <= tex.data.size(); ++i) {
            const uint8_t* s = tex.data.data() + i*4;
            uint8_t* d = rgba.data() + i*4;
            d[0]=s[1]; d[1]=s[2]; d[2]=s[3]; d[3]=s[0];
        }
        return rgba;
    }

    const uint32_t bw=(W+3)/4, bh=(H+3)/4;
    const size_t blkB = (tex.format==TexFormat::DXT1) ? 8 : 16;
    if (tex.data.size() < bw*bh*blkB) return rgba;

    auto u565=[](uint16_t v,uint8_t& r,uint8_t& g,uint8_t& b){
        r=uint8_t(((v>>11)&0x1F)*255/31);
        g=uint8_t(((v>>5)&0x3F)*255/63);
        b=uint8_t(((v)&0x1F)*255/31);
    };

    for (uint32_t by=0;by<bh;++by) for (uint32_t bx=0;bx<bw;++bx){
        const uint8_t* blk=tex.data.data()+(by*bw+bx)*blkB;
        uint8_t alpha[16]; std::fill(alpha,alpha+16,0xFFu);

        if(tex.format==TexFormat::DXT3){
            for(int i=0;i<8;++i){alpha[i*2]=(blk[i]&0x0F)*17;alpha[i*2+1]=((blk[i]>>4)&0x0F)*17;}
            blk+=8;
        } else if(tex.format==TexFormat::DXT5){
            uint8_t a0=blk[0],a1=blk[1]; uint64_t c=0;
            for(int i=2;i<8;++i) c|=(uint64_t)blk[i]<<((i-2)*8);
            uint8_t at[8];at[0]=a0;at[1]=a1;
            if(a0>a1){for(int i=1;i<=6;++i)at[i+1]=(uint8_t)(((7-i)*a0+i*a1)/7);}
            else{for(int i=1;i<=4;++i)at[i+1]=(uint8_t)(((5-i)*a0+i*a1)/5);at[6]=0;at[7]=255;}
            for(int i=0;i<16;++i)alpha[i]=at[(c>>(i*3))&7];
            blk+=8;
        }

        uint16_t c0,c1; std::memcpy(&c0,blk,2);std::memcpy(&c1,blk+2,2);
        uint32_t idx; std::memcpy(&idx,blk+4,4);
        uint8_t cr[4],cg[4],cb[4],ca[4];
        u565(c0,cr[0],cg[0],cb[0]);ca[0]=0xFF;
        u565(c1,cr[1],cg[1],cb[1]);ca[1]=0xFF;
        if(tex.format==TexFormat::DXT1){
            if(c0>c1){cr[2]=(2*cr[0]+cr[1])/3;cg[2]=(2*cg[0]+cg[1])/3;cb[2]=(2*cb[0]+cb[1])/3;ca[2]=0xFF;
                      cr[3]=(cr[0]+2*cr[1])/3;cg[3]=(cg[0]+2*cg[1])/3;cb[3]=(cb[0]+2*cb[1])/3;ca[3]=0xFF;}
            else{cr[2]=(cr[0]+cr[1])/2;cg[2]=(cg[0]+cg[1])/2;cb[2]=(cb[0]+cb[1])/2;ca[2]=0xFF;
                 cr[3]=cg[3]=cb[3]=0;ca[3]=0;}
        } else {
            cr[2]=(2*cr[0]+cr[1])/3;cg[2]=(2*cg[0]+cg[1])/3;cb[2]=(2*cb[0]+cb[1])/3;ca[2]=0xFF;
            cr[3]=(cr[0]+2*cr[1])/3;cg[3]=(cg[0]+2*cg[1])/3;cb[3]=(cb[0]+2*cb[1])/3;ca[3]=0xFF;
        }
        for(int py=0;py<4;++py) for(int px=0;px<4;++px){
            uint32_t ox=bx*4+px,oy=by*4+py;
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

UIHudAtlasPanel::UIHudAtlasPanel()  = default;
UIHudAtlasPanel::~UIHudAtlasPanel() { DestroyGLTexture(); ClearThumbCache(); }

void UIHudAtlasPanel::Shutdown() { DestroyGLTexture(); ClearThumbCache(); }

// ─── Open ─────────────────────────────────────────────────────────────────────

void UIHudAtlasPanel::Open(const std::filesystem::path& path, TaskQueue& tasks)
{
    loaded_    = false;
    loading_   = true;
    sourcePath_ = path;
    selTPK_    = selTex_ = selElement_ = -1;
    DestroyGLTexture();
    ClearThumbCache();
    lastError_.clear();

    tasks.Submit("Loading " + path.filename().string(),
        [this, path](nfsmw::ProgressState&)
        {
            // ── Load raw bytes ──
            std::ifstream f(path, std::ios::binary);
            if (!f) {
                lastError_ = "Cannot open: " + path.string();
                loading_ = false;
                return;
            }
            std::vector<uint8_t> raw((std::istreambuf_iterator<char>(f)),
                                      std::istreambuf_iterator<char>());

            std::vector<uint8_t> data;
            if (LZCDecompressor::IsCompressed(raw)) {
                auto res = LZCDecompressor::Decompress(raw);
                if (!res) { lastError_ = res.error; loading_ = false; return; }
                data = std::move(res.value);
            } else {
                data = std::move(raw);
            }

            // ── Extract TPKs via StreamBundleLoader ──
            auto sec = StreamBundleLoader::ParseBlob(data, 0);
            if (sec) {
                atlasTPKs_ = std::move(sec.value.texturePacks);
            } else {
                LOG_WARN("UIHudAtlasPanel: StreamBundle error: {}", sec.error);
            }

            // ── Parse HUD layout from the same buffer ──
            auto lay = HUDLayoutParser::Parse(std::span<const uint8_t>(data), 0);
            if (lay) {
                hudLayout_ = std::move(lay.value);
            } else {
                LOG_WARN("UIHudAtlasPanel: HUDLayout error: {}", lay.error);
            }

            loaded_  = true;
            loading_ = false;
            LOG_INFO("UIHudAtlasPanel: {} TPK(s), {} element(s), decoded={}",
                     atlasTPKs_.size(), hudLayout_.elements.size(),
                     hudLayout_.decoded);
        });
}

// ─── GL helpers ───────────────────────────────────────────────────────────────

void UIHudAtlasPanel::UploadGLTexture(const Texture& tex)
{
    DestroyGLTexture();
    auto rgba = DecodeToRGBA(tex);
    if (rgba.empty()) return;
    GLuint h=0;
    glGenTextures(1,&h);
    glBindTexture(GL_TEXTURE_2D,h);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,tex.width,tex.height,0,GL_RGBA,GL_UNSIGNED_BYTE,rgba.data());
    glBindTexture(GL_TEXTURE_2D,0);
    previewGL_=h; previewW_=tex.width; previewH_=tex.height;
}

void UIHudAtlasPanel::DestroyGLTexture()
{
    if (previewGL_) { glDeleteTextures(1,&previewGL_); previewGL_=0; }
    previewW_=previewH_=0;
}

uint32_t UIHudAtlasPanel::GetOrCreateThumb(const Texture& tex)
{
    if (tex.format==TexFormat::PAL8||tex.format==TexFormat::Unknown) return 0;
    if (tex.data.empty()) return 0;
    auto it=thumbCache_.find(tex.nameHash);
    if (it!=thumbCache_.end()) return it->second;
    auto rgba=DecodeToRGBA(tex);
    if (rgba.empty()){thumbCache_[tex.nameHash]=0;return 0;}
    constexpr int kT=32;
    const int sw=tex.width, sh=tex.height;
    std::vector<uint8_t> thumb(kT*kT*4,0);
    for(int ty=0;ty<kT;++ty) for(int tx=0;tx<kT;++tx){
        int sx0=tx*sw/kT,sx1=(tx+1)*sw/kT,sy0=ty*sh/kT,sy1=(ty+1)*sh/kT;
        uint32_t r=0,g=0,b=0,a=0,n=0;
        for(int sy=sy0;sy<std::max(sy0+1,sy1);++sy)
            for(int sx=sx0;sx<std::max(sx0+1,sx1);++sx){
                if(sx>=sw||sy>=sh) continue;
                const uint8_t* p=rgba.data()+(sy*sw+sx)*4;
                r+=p[0];g+=p[1];b+=p[2];a+=p[3];++n;
            }
        if(!n)n=1;
        uint8_t* d=thumb.data()+(ty*kT+tx)*4;
        d[0]=static_cast<uint8_t>(r/n);d[1]=static_cast<uint8_t>(g/n);d[2]=static_cast<uint8_t>(b/n);d[3]=static_cast<uint8_t>(a/n);
    }
    GLuint h=0;
    glGenTextures(1,&h);glBindTexture(GL_TEXTURE_2D,h);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,kT,kT,0,GL_RGBA,GL_UNSIGNED_BYTE,thumb.data());
    glBindTexture(GL_TEXTURE_2D,0);
    thumbCache_[tex.nameHash]=h;
    return h;
}

void UIHudAtlasPanel::ClearThumbCache()
{
    for(auto&[h,gl]:thumbCache_) if(gl) glDeleteTextures(1,&gl);
    thumbCache_.clear();
}

void UIHudAtlasPanel::SelectTexture(int ti, int xi)
{
    selTPK_=ti; selTex_=xi;
    if(ti<0||ti>=(int)atlasTPKs_.size()) return;
    const auto& tpk=atlasTPKs_[ti];
    if(xi<0||xi>=(int)tpk.textures.size()) return;
    UploadGLTexture(tpk.textures[xi]);
}

// ─── DoExport / DoReplace ─────────────────────────────────────────────────────

void UIHudAtlasPanel::DoExport(int ti, int xi,
                                const std::filesystem::path& dest)
{
    if(ti<0||ti>=(int)atlasTPKs_.size()) return;
    const auto& tex=atlasTPKs_[ti].textures[xi];
    auto r=DDSCodec::Export(tex,std::span<const uint8_t>(tex.data),dest);
    writeStatus_=r?"Exported: "+dest.filename().string():"Export failed: "+r.error;
    if(!r) VanGui::NotifyError("%s", std::string(r.error).c_str());
}

void UIHudAtlasPanel::DoReplace(int ti, int xi,
                                 const std::filesystem::path& ddsPath,
                                 TaskQueue& tasks)
{
    if(ti<0||ti>=(int)atlasTPKs_.size()) return;
    const auto& tex=atlasTPKs_[ti].textures[xi];
    const auto  src=sourcePath_;
    ReplaceRequest req{tex.nameHash,ddsPath};
    tasks.Submit("Replacing HUD texture "+tex.name,
        [this,src,req](nfsmw::ProgressState&){
            backup_.EnsureFileBak(src);
            auto res=repacker_.PatchFile(src,{req},backup_);
            if(!res.empty()&&res[0].ok){
                writeStatus_="Replaced: "+req.ddsPath.filename().string();
                VanGui::NotifySuccess("%s", std::string(writeStatus_).c_str());
            } else {
                const std::string e=res.empty()?"unknown":res[0].error;
                writeStatus_="Replace failed: "+e;
                VanGui::NotifyError("%s", std::string(writeStatus_).c_str());
            }
        });
}

// ─── Draw ─────────────────────────────────────────────────────────────────────

void UIHudAtlasPanel::Draw(TaskQueue& tasks)
{
    if (loading_) { VanGui::TextDisabled("Loading…"); return; }
    if (!loaded_) {
        if (!lastError_.empty()) VanGui::TextColored({1,0.3f,0.3f,1}, "%s", lastError_.c_str());
        else VanGui::TextDisabled("No HUD BUN loaded. Use Open UI/HUD → GLOBALHUD.BUN.");
        return;
    }

    const float totalH = VanGui::GetContentRegionAvail().y;

    (void)VanGui::BeginChild("##hud_tree", VanVec2(kAtlasTreeW, totalH), true);
    DrawAtlasTree();
    VanGui::EndChild();

    VanGui::SameLine();

    (void)VanGui::BeginChild("##hud_detail", VanVec2(0, totalH), false);
    DrawDetailPane(tasks);
    VanGui::EndChild();

    if (showRepackConfirm_)
        DrawRepackConfirmDialog(tasks);
}

void UIHudAtlasPanel::DrawAtlasTree()
{
    filter_.Draw("##hud_filter", "Filter textures…");
    VanGui::Separator();

    for (int ti = 0; ti < (int)atlasTPKs_.size(); ++ti) {
        const auto& tpk = atlasTPKs_[ti];
        char lbl[64];
        std::snprintf(lbl,sizeof(lbl),"TPK %d (%zu)##ht%d",ti,tpk.textures.size(),ti);
        if (VanGui::TreeNodeEx(lbl,VanGuiTreeNodeFlags_DefaultOpen)) {
            for (int xi = 0; xi < (int)tpk.textures.size(); ++xi) {
                const auto& tex = tpk.textures[xi];
                if (!filter_.Matches(tex.name)) continue;
                uint32_t thumb=GetOrCreateThumb(tex);
                if(thumb) VanGui::Image((VanTextureID)(uintptr_t)thumb,{16,16});
                else VanGui::Dummy({16,16});
                VanGui::SameLine();
                bool sel=(ti==selTPK_&&xi==selTex_);
                if(VanGui::Selectable(tex.name.c_str(),sel,VanGuiSelectableFlags_AllowOverlap))
                    SelectTexture(ti,xi);
            }
            VanGui::TreePop();
        }
    }
}

void UIHudAtlasPanel::DrawDetailPane(TaskQueue& tasks)
{
    if (VanGui::BeginTabBar("##hud_tabs")) {
        if (VanGui::BeginTabItem("Texture")) {
            detailTab_=0;
            DrawTextureTab(tasks);
            VanGui::EndTabItem();
        }
        if (VanGui::BeginTabItem("Elements")) {
            detailTab_=1;
            DrawElementsTab();
            VanGui::EndTabItem();
        }
        VanGui::EndTabBar();
    }
}

void UIHudAtlasPanel::DrawTextureTab(TaskQueue& tasks)
{
    if (selTPK_ < 0 || selTex_ < 0) {
        VanGui::TextDisabled("Select a texture from the atlas.");
        return;
    }
    if (selTPK_ >= (int)atlasTPKs_.size()) return;
    const auto& tex = atlasTPKs_[selTPK_].textures[selTex_];

    if (previewGL_) {
        float aspect = previewH_ > 0 ? (float)previewW_/(float)previewH_ : 1.f;
        float dispW  = std::min(320.f, VanGui::GetContentRegionAvail().x);
        VanGui::Image((VanTextureID)(uintptr_t)previewGL_,
                     {dispW, dispW/aspect});
    }
    VanGui::Separator();
    VanGui::Text("Name:   %s",     tex.name.c_str());
    VanGui::Text("Hash:   0x%08X", tex.nameHash);
    VanGui::Text("Size:   %d x %d", tex.width, tex.height);
    VanGui::Text("Format: %s",     TexFormatName(tex.format));
    VanGui::Text("Mips:   %d",     tex.mipmaps);
    VanGui::Separator();

    if (VanGui::Button("Export Atlas DDS…")) {
        if (fileDialog_)
            fileDialog_->Show("Export HUD Texture", FileDialog::Mode::Save, {".dds"},
                [this](const std::filesystem::path& d){ DoExport(selTPK_,selTex_,d); },
                tex.name+".dds");
    }
    VanGui::SameLine();
    if (VanGui::Button("Replace from DDS…")) {
        if (fileDialog_)
            fileDialog_->Show("Replace HUD Texture", FileDialog::Mode::Open, {".dds"},
                [this,&tasks](const std::filesystem::path& s){ DoReplace(selTPK_,selTex_,s,tasks); });
    }

    if (!writeStatus_.empty()) {
        VanGui::Spacing();
        VanGui::TextWrapped("%s", writeStatus_.c_str());
    }
}

void UIHudAtlasPanel::DrawElementsTab()
{
    if (!hudLayout_.decoded) {
        VanGui::TextDisabled("HUD layout chunk not yet recognised.");
        VanGui::TextDisabled("Raw bytes are preserved (%zu bytes).",
                            hudLayout_.rawLayoutBlob.size());
        VanGui::TextDisabled("Element editing will be enabled once the chunk ID");
        VanGui::TextDisabled("is confirmed against the retail binary.");
        return;
    }

    VanGui::Text("%zu element(s)", hudLayout_.elements.size());
    VanGui::TextDisabled("(read-only in Phase 8v1)");
    VanGui::Separator();

    if (VanGui::BeginTable("##elems", 6,
            VanGuiTableFlags_Borders | VanGuiTableFlags_RowBg |
            VanGuiTableFlags_ScrollY | VanGuiTableFlags_SizingFixedFit,
            VanVec2(0, VanGui::GetContentRegionAvail().y))) {
        VanGui::TableSetupScrollFreeze(0, 1);
        VanGui::TableSetupColumn("Hash",     VanGuiTableColumnFlags_WidthFixed,  90.f);
        VanGui::TableSetupColumn("Name",     VanGuiTableColumnFlags_WidthStretch);
        VanGui::TableSetupColumn("Anchor X", VanGuiTableColumnFlags_WidthFixed,  65.f);
        VanGui::TableSetupColumn("Anchor Y", VanGuiTableColumnFlags_WidthFixed,  65.f);
        VanGui::TableSetupColumn("Size W",   VanGuiTableColumnFlags_WidthFixed,  60.f);
        VanGui::TableSetupColumn("Size H",   VanGuiTableColumnFlags_WidthFixed,  60.f);
        VanGui::TableHeadersRow();

        for (int i = 0; i < (int)hudLayout_.elements.size(); ++i) {
            const auto& e = hudLayout_.elements[i];
            VanGui::TableNextRow();
            bool sel = (i == selElement_);
            (void)VanGui::TableSetColumnIndex(0);
            char hashStr[16]; std::snprintf(hashStr,sizeof(hashStr),"0x%08X",e.nameHash);
            if (VanGui::Selectable(hashStr, sel,
                    VanGuiSelectableFlags_SpanAllColumns | VanGuiSelectableFlags_AllowOverlap,
                    VanVec2(0, 0)))
                selElement_ = i;
            (void)VanGui::TableSetColumnIndex(1); VanGui::TextUnformatted(e.name.empty() ? "—" : e.name.c_str());
            (void)VanGui::TableSetColumnIndex(2); VanGui::Text("%.3f", e.anchor.x);
            (void)VanGui::TableSetColumnIndex(3); VanGui::Text("%.3f", e.anchor.y);
            (void)VanGui::TableSetColumnIndex(4); VanGui::Text("%.3f", e.size.x);
            (void)VanGui::TableSetColumnIndex(5); VanGui::Text("%.3f", e.size.y);
        }
        VanGui::EndTable();
    }
}

void UIHudAtlasPanel::DrawRepackConfirmDialog(TaskQueue& tasks)
{
    VanGui::OpenPopup("Repack required##hud");
    if (VanGui::BeginPopupModal("Repack required##hud",nullptr,
                               VanGuiWindowFlags_AlwaysAutoResize)) {
        VanGui::TextWrapped(
            "The replacement DDS is larger than the original slot.\n"
            "A full repack of GLOBALHUD.BUN is required.\n"
            "This may increase the file size.");
        VanGui::Spacing();
        if (VanGui::Button("Repack")) {
            DoReplace(confirmTPK_,confirmTex_,pendingReplaceDDS_,tasks);
            showRepackConfirm_=false; VanGui::CloseCurrentPopup();
        }
        VanGui::SameLine();
        if (VanGui::Button("Cancel")) { showRepackConfirm_=false; VanGui::CloseCurrentPopup(); }
        VanGui::EndPopup();
    }
}

} // namespace nfsmw
