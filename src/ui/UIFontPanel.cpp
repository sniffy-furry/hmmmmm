// ─── ui/UIFontPanel.cpp ───────────────────────────────────────────────────────
// Phase 8 — GLOBALB.LZC font sheet browser.
//
// Layout:
//   Left column  — list of FontSheets found in the LZC archive (with filter).
//   Right column — tabs:
//     "Atlas"   — atlas thumbnail + metadata card + "Export Atlas DDS…"
//     "Glyphs"  — read-only glyph table (codepoint, U0/V0/U1/V1, advance).
//                 If glyphsDecoded=false a note is shown instead of the table.
//
// Atlas export uses the same DDSCodec::Export path as every other texture panel.
// Glyph UV editing is deferred to a future patch; the table is display-only.
// ─────────────────────────────────────────────────────────────────────────────
#include "ui/UIFontPanel.h"
#include "core/Logger.h"
#include <vangui.h>
#include <vangui_notify.h>
#include "renderer/GLCompat.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <memory>
#include <span>

namespace nfsmw {

// ─── DXT software decoder (shared inline helper) ──────────────────────────────

namespace {

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

const char* FormatName(TexFormat f) {
    switch (f) {
        case TexFormat::DXT1:   return "DXT1";
        case TexFormat::DXT3:   return "DXT3";
        case TexFormat::DXT5:   return "DXT5";
        case TexFormat::ARGB32: return "ARGB32";
        default:                return "Unknown";
    }
}

} // namespace

// ─── Constructor / Destructor ─────────────────────────────────────────────────

UIFontPanel::UIFontPanel()  = default;
UIFontPanel::~UIFontPanel() { DestroyGLTexture(); ClearThumbCache(); }

void UIFontPanel::Shutdown() { DestroyGLTexture(); ClearThumbCache(); }

// ─── Open ─────────────────────────────────────────────────────────────────────

void UIFontPanel::Open(const std::filesystem::path& lzcPath, TaskQueue& tasks)
{
    if (loading_) return;
    loading_   = true;
    loaded_    = false;
    lastError_.clear();
    sheets_.clear();
    selSheet_  = -1;
    DestroyGLTexture();
    ClearThumbCache();
    sourcePath_ = lzcPath;

    auto result = std::make_shared<Result<std::vector<FontSheet>>>(
        Result<std::vector<FontSheet>>::Err("not started"));

    tasks.Submit("Loading " + lzcPath.filename().string(),
        [lzcPath, result](ProgressState& p) {
            p.fraction = -1.f;
            *result = FontSheetParser::Load(lzcPath);
            p.fraction = 1.f;
        },
        [this, lzcPath, result]() {
            loading_ = false;
            if (!*result) {
                lastError_ = result->error;
                LOG_ERROR("UIFontPanel: load failed — {}", lastError_);
                VanGui::NotifyError("%s", std::string("Font load failed: " + lastError_).c_str());
                return;
            }
            sheets_   = std::move(result->value);
            loaded_   = true;
            selSheet_ = sheets_.empty() ? -1 : 0;
            if (selSheet_ >= 0) SelectSheet(0);
            LOG_INFO("UIFontPanel: loaded {} font sheet(s) from {}",
                     sheets_.size(), lzcPath.filename().string());
            VanGui::NotifyInfo("%s", std::string("Loaded " + lzcPath.filename().string()).c_str());
        });
}

// ─── GL helpers ───────────────────────────────────────────────────────────────

void UIFontPanel::UploadGLTexture(const Texture& tex)
{
    DestroyGLTexture();
    auto rgba = DecodeToRGBA(tex);
    if (rgba.empty()) return;
    glGenTextures(1, &previewGL_);
    glBindTexture(GL_TEXTURE_2D, previewGL_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                 static_cast<GLsizei>(tex.width),
                 static_cast<GLsizei>(tex.height),
                 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
    glBindTexture(GL_TEXTURE_2D, 0);
    previewW_ = static_cast<int>(tex.width);
    previewH_ = static_cast<int>(tex.height);
}

void UIFontPanel::DestroyGLTexture()
{
    if (previewGL_) { glDeleteTextures(1, &previewGL_); previewGL_ = 0; }
    previewW_ = previewH_ = 0;
}

uint32_t UIFontPanel::GetOrCreateThumb(const Texture& tex)
{
    const uint32_t key = tex.nameHash;
    auto it = thumbCache_.find(key);
    if (it != thumbCache_.end()) return it->second;

    auto rgba = DecodeToRGBA(tex);
    if (rgba.empty()) { thumbCache_[key] = 0; return 0; }

    constexpr int kThumbSize = 48;
    const uint32_t W = tex.width, H = tex.height;
    std::vector<uint8_t> thumb(kThumbSize * kThumbSize * 4);
    for (int ty = 0; ty < kThumbSize; ++ty) {
        for (int tx = 0; tx < kThumbSize; ++tx) {
            uint32_t sx = (uint32_t)((float)tx / kThumbSize * W);
            uint32_t sy = (uint32_t)((float)ty / kThumbSize * H);
            sx = std::min(sx, W - 1); sy = std::min(sy, H - 1);
            const uint8_t* s = rgba.data() + (sy * W + sx) * 4;
            uint8_t* d = thumb.data() + (ty * kThumbSize + tx) * 4;
            d[0]=s[0]; d[1]=s[1]; d[2]=s[2]; d[3]=s[3];
        }
    }

    GLuint tex2 = 0;
    glGenTextures(1, &tex2);
    glBindTexture(GL_TEXTURE_2D, tex2);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                 kThumbSize, kThumbSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, thumb.data());
    glBindTexture(GL_TEXTURE_2D, 0);
    thumbCache_[key] = tex2;
    return tex2;
}

void UIFontPanel::ClearThumbCache()
{
    for (auto& [k, t] : thumbCache_) if (t) glDeleteTextures(1, &t);
    thumbCache_.clear();
}

// ─── SelectSheet ──────────────────────────────────────────────────────────────

void UIFontPanel::SelectSheet(int idx)
{
    selSheet_ = idx;
    DestroyGLTexture();
    if (idx < 0 || idx >= static_cast<int>(sheets_.size())) return;
    const FontSheet& fs = sheets_[idx];
    if (!fs.atlasPack.textures.empty())
        UploadGLTexture(fs.atlasPack.textures[0]);
}

// ─── Draw ─────────────────────────────────────────────────────────────────────

void UIFontPanel::Draw(TaskQueue& tasks)
{
    if (loading_) {
        VanGui::SetCursorPosY(VanGui::GetCursorPosY() + 40.f);
        const float w = VanGui::GetContentRegionAvail().x;
        constexpr char kMsg[] = "Loading font sheets\xe2\x80\xa6";
        VanGui::SetCursorPosX((w - VanGui::CalcTextSize(kMsg).x) * 0.5f);
        VanGui::TextDisabled("%s", kMsg);
        return;
    }

    if (!loaded_) {
        if (!lastError_.empty()) {
            VanGui::Spacing();
            VanGui::PushStyleColor(VanGuiCol_Text, VanVec4(1,0.3f,0.3f,1));
            VanGui::TextWrapped("Error: %s", lastError_.c_str());
            VanGui::PopStyleColor();
        } else {
            const float w = VanGui::GetContentRegionAvail().x;
            const float h = VanGui::GetContentRegionAvail().y;
            constexpr char kMsg[] = "Open GLOBAL/GLOBALB.LZC via File \xe2\x80\x93 Open UI/HUD\xe2\x80\xa6";
            VanGui::SetCursorPos(VanVec2((w - VanGui::CalcTextSize(kMsg).x) * 0.5f,
                                       (h - VanGui::GetTextLineHeight()) * 0.5f));
            VanGui::TextDisabled("%s", kMsg);
        }
        return;
    }

    const float totalW = VanGui::GetContentRegionAvail().x;
    const float totalH = VanGui::GetContentRegionAvail().y;
    const float listW  = kSheetListW;
    const float detailW = totalW - listW - 6.f;

    // Left: sheet list
    if (VanGui::BeginChild("##FontSheetList", VanVec2(listW, totalH), true)) {
        DrawSheetList();
    }
    VanGui::EndChild();

    VanGui::SameLine(0, 6.f);

    // Right: detail pane
    if (VanGui::BeginChild("##FontDetail", VanVec2(detailW, totalH), false)) {
        DrawDetailPane(tasks);
    }
    VanGui::EndChild();
}

// ─── DrawSheetList ────────────────────────────────────────────────────────────

void UIFontPanel::DrawSheetList()
{
    glyphFilter_.Draw("##FontFilter", "Filter sheets\xe2\x80\xa6");
    VanGui::Spacing();

    for (int i = 0; i < static_cast<int>(sheets_.size()); ++i) {
        const FontSheet& fs = sheets_[i];
        const std::string label = fs.name.empty()
            ? ("Sheet " + std::to_string(i))
            : fs.name;

        if (!glyphFilter_.Matches(label)) continue;

        VanGui::PushID(i);

        // Thumbnail
        if (!fs.atlasPack.textures.empty()) {
            const uint32_t th = GetOrCreateThumb(fs.atlasPack.textures[0]);
            if (th) {
                VanGui::Image((VanTextureID)(uintptr_t)th,
                             VanVec2(20.f, 20.f));
                VanGui::SameLine(0, 4.f);
            }
        }

        const bool selected = (selSheet_ == i);
        if (VanGui::Selectable(label.c_str(), selected,
                               VanGuiSelectableFlags_None,
                               VanVec2(0, 20.f))) {
            SelectSheet(i);
            detailTab_ = 0;
        }
        VanGui::PopID();
    }
}

// ─── DrawDetailPane ───────────────────────────────────────────────────────────

void UIFontPanel::DrawDetailPane(TaskQueue& tasks)
{
    if (selSheet_ < 0 || selSheet_ >= static_cast<int>(sheets_.size())) {
        VanGui::TextDisabled("Select a font sheet on the left.");
        return;
    }

    if (VanGui::BeginTabBar("##FontDetailTabs")) {
        if (VanGui::BeginTabItem("Atlas")) {
            detailTab_ = 0;
            DrawAtlasTab(tasks);
            VanGui::EndTabItem();
        }
        if (VanGui::BeginTabItem("Glyphs")) {
            detailTab_ = 1;
            DrawGlyphsTab();
            VanGui::EndTabItem();
        }
        VanGui::EndTabBar();
    }
}

// ─── DrawAtlasTab ─────────────────────────────────────────────────────────────

void UIFontPanel::DrawAtlasTab(TaskQueue& /*tasks*/)
{
    const FontSheet& fs = sheets_[selSheet_];

    // Metadata card
    VanGui::Spacing();
    VanGui::Text("Font:   %s", fs.name.empty() ? "(unnamed)" : fs.name.c_str());
    VanGui::Text("Sheets: %d texture(s) in atlas pack",
                static_cast<int>(fs.atlasPack.textures.size()));
    if (!fs.atlasPack.textures.empty()) {
        const Texture& t0 = fs.atlasPack.textures[0];
        VanGui::Text("Atlas:  %u × %u  %s",
                    t0.width, t0.height, FormatName(t0.format));
        VanGui::Text("Glyphs: %s (%d entries)",
                    fs.glyphsDecoded ? "decoded" : "raw (not decoded)",
                    static_cast<int>(fs.glyphs.size()));
    }

    VanGui::Spacing();
    VanGui::Separator();
    VanGui::Spacing();

    // Export button
    const bool hasAtlas = !fs.atlasPack.textures.empty();
    if (!hasAtlas) VanGui::BeginDisabled();
    if (VanGui::Button("Export Atlas DDS\xe2\x80\xa6")) {
        if (fileDialog_) {
            fileDialog_->Show("Save Atlas DDS", FileDialog::Mode::Save, {".dds"},
                [this, idx = selSheet_](const std::filesystem::path& dest) {
                    DoExportAtlas(idx, dest);
                });
        }
    }
    if (!hasAtlas) VanGui::EndDisabled();

    VanGui::Spacing();
    VanGui::Separator();
    VanGui::Spacing();

    // Atlas preview
    if (previewGL_ && previewW_ > 0 && previewH_ > 0) {
        const float avail = VanGui::GetContentRegionAvail().x;
        const float aspect = static_cast<float>(previewH_) / previewW_;
        const float dispW = std::min(avail, static_cast<float>(previewW_));
        const float dispH = dispW * aspect;
        VanGui::Image((VanTextureID)(uintptr_t)previewGL_,
                     VanVec2(dispW, dispH));
        if (VanGui::IsItemHovered()) {
            (void)VanGui::BeginTooltip();
            VanGui::Text("%d × %d", previewW_, previewH_);
            VanGui::EndTooltip();
        }
    } else {
        VanGui::TextDisabled("(no atlas preview)");
    }
}

// ─── DrawGlyphsTab ────────────────────────────────────────────────────────────

void UIFontPanel::DrawGlyphsTab()
{
    const FontSheet& fs = sheets_[selSheet_];

    VanGui::Spacing();

    if (!fs.glyphsDecoded) {
        VanGui::TextWrapped(
            "Glyph table could not be decoded: the on-disk chunk ID has not "
            "been confirmed against the retail binary.  Raw bytes are "
            "preserved in the FontSheet for future patching.\n\n"
            "Raw glyph blob: %zu byte(s).",
            fs.rawGlyphBlob.size());
        return;
    }

    if (fs.glyphs.empty()) {
        VanGui::TextDisabled("No glyphs decoded.");
        return;
    }

    // Filter
    glyphFilter_.Draw("##GlyphFilter", "Filter glyphs\xe2\x80\xa6");
    VanGui::Spacing();

    // Table
    constexpr VanGuiTableFlags kTableFlags =
        VanGuiTableFlags_Borders       |
        VanGuiTableFlags_RowBg         |
        VanGuiTableFlags_ScrollY       |
        VanGuiTableFlags_SizingFixedFit;

    const float tableH = VanGui::GetContentRegionAvail().y - 4.f;
    if (VanGui::BeginTable("##GlyphTable", 6, kTableFlags, VanVec2(0, tableH))) {
        VanGui::TableSetupScrollFreeze(0, 1);
        VanGui::TableSetupColumn("CP",      VanGuiTableColumnFlags_WidthFixed, 50.f);
        VanGui::TableSetupColumn("Char",    VanGuiTableColumnFlags_WidthFixed, 36.f);
        VanGui::TableSetupColumn("U0",      VanGuiTableColumnFlags_WidthFixed, 72.f);
        VanGui::TableSetupColumn("V0",      VanGuiTableColumnFlags_WidthFixed, 72.f);
        VanGui::TableSetupColumn("U1/V1",   VanGuiTableColumnFlags_WidthFixed, 100.f);
        VanGui::TableSetupColumn("Advance", VanGuiTableColumnFlags_WidthStretch);
        VanGui::TableHeadersRow();

        for (const auto& g : fs.glyphs) {
            // Filter by codepoint decimal or hex
            char cpBuf[16];
            std::snprintf(cpBuf, sizeof(cpBuf), "%u", g.codepoint);
            if (!glyphFilter_.Matches(cpBuf)) continue;

            VanGui::TableNextRow();
            (void)VanGui::TableSetColumnIndex(0);
            VanGui::Text("U+%04X", g.codepoint);
            (void)VanGui::TableSetColumnIndex(1);
            if (g.codepoint >= 0x20 && g.codepoint < 0x7F) {
                char ch[2] = { static_cast<char>(g.codepoint), 0 };
                VanGui::TextUnformatted(ch);
            } else {
                VanGui::TextDisabled("·");
            }
            (void)VanGui::TableSetColumnIndex(2);
            VanGui::Text("%.4f", g.u0);
            (void)VanGui::TableSetColumnIndex(3);
            VanGui::Text("%.4f", g.v0);
            (void)VanGui::TableSetColumnIndex(4);
            VanGui::Text("%.4f  %.4f", g.u1, g.v1);
            (void)VanGui::TableSetColumnIndex(5);
            VanGui::Text("%.2f", g.advance);
        }

        VanGui::EndTable();
    }
}

// ─── DoExportAtlas ────────────────────────────────────────────────────────────

void UIFontPanel::DoExportAtlas(int sheetIdx, const std::filesystem::path& destPath)
{
    if (sheetIdx < 0 || sheetIdx >= static_cast<int>(sheets_.size())) return;
    const FontSheet& fs = sheets_[sheetIdx];
    if (fs.atlasPack.textures.empty()) {
        VanGui::NotifyWarning("%s", std::string("No atlas texture to export.").c_str());
        return;
    }

    const Texture& tex = fs.atlasPack.textures[0];
    auto result = DDSCodec::Export(tex, std::span<const uint8_t>(tex.data), destPath);
    if (!result) {
        const std::string msg = "Export failed: " + result.error;
        LOG_ERROR("UIFontPanel: {}", msg);
        VanGui::NotifyError("%s", std::string(msg).c_str());
        return;
    }

    LOG_INFO("UIFontPanel: exported atlas → {}", destPath.string());
    VanGui::NotifyInfo("%s", std::string("Exported " + destPath.filename().string()).c_str());
}

} // namespace nfsmw
