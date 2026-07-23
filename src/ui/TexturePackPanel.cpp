#include "ui/TexturePackPanel.h"
#include "core/BINFile.h"
#include "core/Logger.h"
#include "ui/FilterBox.h"
#include "core/StringUtil.h"

#include "renderer/GLCompat.h"
#include <vangui.h>

#include <algorithm>
#include <cctype>
#include <cstring>
#include <filesystem>
#include <unordered_map>

namespace nfsmw {

// ─────────────────────────────────────────────────────────────────────────────
// Internal helpers
// ─────────────────────────────────────────────────────────────────────────────

namespace {

std::vector<uint8_t> DecodeToRGBA(const Texture& tex) {
    if (tex.data.empty() || tex.width == 0 || tex.height == 0)
        return {};

    const uint32_t W = tex.width, H = tex.height;
    std::vector<uint8_t> rgba(W * H * 4, 0xFF);

    if (tex.format == TexFormat::ARGB32) {
        const size_t pixels = W * H;
        for (size_t i = 0; i < pixels && (i + 1) * 4 <= tex.data.size(); ++i) {
            const uint8_t* src = tex.data.data() + i * 4;
            uint8_t* dst = rgba.data() + i * 4;
            dst[0] = src[1]; dst[1] = src[2]; dst[2] = src[3]; dst[3] = src[0];
        }
        return rgba;
    }

    const uint32_t bw = (W + 3) / 4, bh = (H + 3) / 4;
    const size_t blockBytes = (tex.format == TexFormat::DXT1) ? 8 : 16;
    if (tex.data.size() < bw * bh * blockBytes) return rgba;

    auto unpack565 = [](uint16_t v, uint8_t& r, uint8_t& g, uint8_t& b) {
        r = (uint8_t)(((v >> 11) & 0x1F) * 255 / 31);
        g = (uint8_t)(((v >>  5) & 0x3F) * 255 / 63);
        b = (uint8_t)(((v      ) & 0x1F) * 255 / 31);
    };

    for (uint32_t by = 0; by < bh; ++by) {
        for (uint32_t bx = 0; bx < bw; ++bx) {
            const uint8_t* blk = tex.data.data() + (by * bw + bx) * blockBytes;

            uint8_t alphaBlock[16];
            std::fill(alphaBlock, alphaBlock + 16, 0xFFu);

            if (tex.format == TexFormat::DXT3) {
                for (int i = 0; i < 8; ++i) {
                    alphaBlock[i * 2    ] = (uint8_t)((blk[i] & 0x0F) * 17);
                    alphaBlock[i * 2 + 1] = (uint8_t)(((blk[i] >> 4) & 0x0F) * 17);
                }
                blk += 8;
            } else if (tex.format == TexFormat::DXT5) {
                uint8_t a0 = blk[0], a1 = blk[1];
                uint64_t codes = 0;
                for (int i = 2; i < 8; ++i)
                    codes |= (uint64_t)blk[i] << ((i - 2) * 8);
                uint8_t atable[8];
                atable[0] = a0; atable[1] = a1;
                if (a0 > a1) {
                    for (int i = 1; i <= 6; ++i)
                        atable[i + 1] = (uint8_t)(((7 - i) * a0 + i * a1) / 7);
                } else {
                    for (int i = 1; i <= 4; ++i)
                        atable[i + 1] = (uint8_t)(((5 - i) * a0 + i * a1) / 5);
                    atable[6] = 0; atable[7] = 255;
                }
                for (int i = 0; i < 16; ++i)
                    alphaBlock[i] = atable[(codes >> (i * 3)) & 0x7];
                blk += 8;
            }

            uint16_t c0, c1;
            std::memcpy(&c0, blk, 2); std::memcpy(&c1, blk + 2, 2);
            uint32_t indices; std::memcpy(&indices, blk + 4, 4);

            uint8_t cr[4], cg[4], cb_[4], ca[4];
            unpack565(c0, cr[0], cg[0], cb_[0]); ca[0] = 0xFF;
            unpack565(c1, cr[1], cg[1], cb_[1]); ca[1] = 0xFF;

            if (tex.format == TexFormat::DXT1) {
                if (c0 > c1) {
                    cr[2]=(uint8_t)((2*cr[0]+cr[1])/3); cg[2]=(uint8_t)((2*cg[0]+cg[1])/3); cb_[2]=(uint8_t)((2*cb_[0]+cb_[1])/3); ca[2]=0xFF;
                    cr[3]=(uint8_t)((cr[0]+2*cr[1])/3); cg[3]=(uint8_t)((cg[0]+2*cg[1])/3); cb_[3]=(uint8_t)((cb_[0]+2*cb_[1])/3); ca[3]=0xFF;
                } else {
                    cr[2]=(uint8_t)((cr[0]+cr[1])/2); cg[2]=(uint8_t)((cg[0]+cg[1])/2); cb_[2]=(uint8_t)((cb_[0]+cb_[1])/2); ca[2]=0xFF;
                    cr[3]=0; cg[3]=0; cb_[3]=0; ca[3]=0;
                }
            } else {
                cr[2]=(uint8_t)((2*cr[0]+cr[1])/3); cg[2]=(uint8_t)((2*cg[0]+cg[1])/3); cb_[2]=(uint8_t)((2*cb_[0]+cb_[1])/3); ca[2]=0xFF;
                cr[3]=(uint8_t)((cr[0]+2*cr[1])/3); cg[3]=(uint8_t)((cg[0]+2*cg[1])/3); cb_[3]=(uint8_t)((cb_[0]+2*cb_[1])/3); ca[3]=0xFF;
            }

            for (int py = 0; py < 4; ++py) {
                for (int px = 0; px < 4; ++px) {
                    const uint32_t px_ = bx*4+px, py_ = by*4+py;
                    if (px_ >= W || py_ >= H) continue;
                    const int idx = (indices >> ((py*4+px)*2)) & 0x3;
                    const int pi  = py*4+px;
                    uint8_t* dst  = rgba.data() + (py_*W+px_)*4;
                    dst[0]=cr[idx]; dst[1]=cg[idx];
                    dst[2]=cb_[idx]; dst[3]=(tex.format==TexFormat::DXT1)?ca[idx]:alphaBlock[pi];
                }
            }
        }
    }
    return rgba;
}

std::string FormatBytes(size_t b) {
    if (b < 1024)         return std::to_string(b) + " B";
    if (b < 1024*1024)    return std::to_string(b/1024) + " KB";
    return std::to_string(b/(1024*1024)) + " MB";
}

// PassesFilter removed: replaced by FilterBox::Matches / ContainsCIStr (audit 4.1/4.2).

} // namespace

// ─────────────────────────────────────────────────────────────────────────────
// TexturePackPanel
// ─────────────────────────────────────────────────────────────────────────────

TexturePackPanel::TexturePackPanel()  = default;

TexturePackPanel::~TexturePackPanel() {
    DestroyGLTexture();
}

void TexturePackPanel::Shutdown() {
    DestroyGLTexture();
}

// ─── OpenFile ────────────────────────────────────────────────────────────────
void TexturePackPanel::OpenFile(const std::filesystem::path& path, TaskQueue& taskQueue) {
    if (loading_) return;
    loading_   = true;
    lastError_ = {};
    writeStatus_ = {};

    // Fix 1.1/1.2: worker builds LoadedFile entirely locally; all member writes
    // (files_.push_back, lastError_) happen only in onDone on the UI thread.
    struct OpenResult { LoadedFile lf; std::string error; bool ok = false; };
    auto sharedResult = std::make_shared<OpenResult>();

    taskQueue.Submit(
        "Loading " + path.filename().string(),
        [path, sharedResult](ProgressState& p) {
            p.fraction = -1.f;
            p.SetDetail("Reading file…");

            auto binResult = BINFile::Open(path);
            if (!binResult) {
                sharedResult->error = binResult.error;
                LOG_ERROR("TexturePackPanel: {}", binResult.error);
                return;
            }
            BINFile& bin = binResult.value;

            p.SetDetail("Parsing chunks…");

            LoadedFile lf;
            lf.path       = path;
            lf.compressed = bin.WasDecompressed();

            auto& tpks = lf.tpks;
            bin.ForEachTPK([&](std::span<const uint8_t> payload, uint64_t absOff) {
                auto r = TPKBlockParser::Parse(payload, absOff);
                if (r) {
                    auto& blk = r.value;
                    blk.sourceFile       = path;
                    blk.compressedSource = lf.compressed;
                    tpks.push_back(std::move(blk));
                } else {
                    LOG_WARN("TexturePackPanel: TPK parse failed: {}", r.error);
                }
            });

            p.fraction = 1.0f;
            LOG_INFO("TexturePackPanel: loaded '{}' → {} TPK(s)", path.string(), tpks.size());
            sharedResult->lf = std::move(lf);
            sharedResult->ok = true;
        },
        [this, sharedResult]() {
            // UI thread only — safe to write member fields here.
            loading_ = false;
            if (!sharedResult->ok) {
                lastError_ = sharedResult->error;
                return;
            }
            files_.push_back(std::move(sharedResult->lf));
            auto& f = files_.back();
            if (!f.tpks.empty() && !f.tpks[0].textures.empty()) {
                selFile_    = (int)(files_.size()-1);
                selTPK_     = 0;
                selTexture_ = 0;
                SelectTexture(selFile_, selTPK_, selTexture_);
            }
        }
    );
}

// ─── TotalTextures ───────────────────────────────────────────────────────────
size_t TexturePackPanel::TotalTextures() const {
    size_t n = 0;
    for (auto& f : files_)
        for (auto& t : f.tpks)
            n += t.textures.size();
    return n;
}

// ─── DrawTree ────────────────────────────────────────────────────────────────
void TexturePackPanel::DrawTree() {
    // Filter bar
    texFilter_.Draw("##filter", "Filter textures\xe2\x80\xa6");

    VanGui::Separator();

    if (files_.empty()) {
        VanGui::TextDisabled("Open a .BUN or .BIN file to begin.");
        return;
    }

    // Thumbnail icon size: match one text line so rows stay compact.
    const float iconSz = VanGui::GetTextLineHeight() + 4.0f;

    // Deferred close: erasing files_ mid-iteration would invalidate indices.
    int closeRequest = -1;

    for (int fi = 0; fi < (int)files_.size(); ++fi) {
        auto& lf = files_[fi];

        // ── Detect stream bundle ──────────────────────────────────────────────
        // A "stream bundle" (e.g. STREAML2RA.BUN) has many TPKs that all share
        // the same name (typically "Region"). We flatten these into a single
        // deduplicated list instead of showing Region(76), Region(90) ... nodes.
        bool isStreamBundle = (lf.tpks.size() > 1);
        if (isStreamBundle) {
            const std::string& first = lf.tpks[0].name;
            for (auto& t : lf.tpks)
                if (t.name != first) { isStreamBundle = false; break; }
        }

        const std::string fileLabel = lf.path.filename().string() +
            (lf.compressed ? " [lzc]" : "") +
            "###file" + std::to_string(fi);

        VanGuiTreeNodeFlags fileFlags = VanGuiTreeNodeFlags_DefaultOpen |
                                       VanGuiTreeNodeFlags_OpenOnArrow |
                                       VanGuiTreeNodeFlags_SpanFullWidth;

        const bool fileNodeOpen = VanGui::TreeNodeEx(fileLabel.c_str(), fileFlags);

        // ── Close (✕) button, right-aligned on the file row ──────────────────
        {
            VanGui::PushID(fi);
            const float btnW = VanGui::GetFrameHeight();
            VanGui::SameLine();
            VanGui::SetCursorPosX(VanGui::GetWindowContentRegionMax().x - btnW);
            VanGui::PushStyleColor(VanGuiCol_Button, VanVec4(0, 0, 0, 0));
            if (VanGui::SmallButton("\xc3\x97"))   // ×
                closeRequest = fi;
            VanGui::PopStyleColor();
            if (VanGui::IsItemHovered())
                VanGui::SetTooltip("Close %s", lf.path.filename().string().c_str());
            VanGui::PopID();
        }

        if (!fileNodeOpen) continue;

        // ── Lambda: draw one texture row (thumbnail + label) ─────────────────
        auto drawTexRow = [&](int logicalTi, int logicalXi, const Texture& tex) {
            if (!texFilter_.Matches(tex.name)) return;

            const bool selected = (fi == selFile_ &&
                                   logicalTi == selTPK_ &&
                                   logicalXi == selTexture_);

            // Use the texture's address as the VanGui ID: it is unique and stable
            // for the frame. The old arithmetic ID (fi*100000 + ti*1000 + xi)
            // collided whenever a pack held >=1000 textures or a file held >=100
            // TPKs — common in flattened STREAM bundles — and colliding IDs
            // corrupt VanGui's persistent per-widget state (selection, context
            // popups, open/closed), which is the search/list "glitching out and
            // breaking" after some scrolling/filtering.
            VanGui::PushID(static_cast<const void*>(&tex));

            // Thumbnail (lazy-uploaded, 32x32 box-filtered).
            const uint32_t thumb = GetOrCreateThumb(tex);
            if (thumb) {
                VanGui::Image((VanTextureID)(uintptr_t)thumb, VanVec2(iconSz, iconSz));
            } else {
                // Placeholder for PAL8 / unknown / empty textures.
                VanVec2 p = VanGui::GetCursorScreenPos();
                VanGui::GetWindowDrawList()->AddRectFilled(
                    p, VanVec2(p.x + iconSz, p.y + iconSz),
                    VanGui::GetColorU32(VanGuiCol_FrameBg));
                VanGui::GetWindowDrawList()->AddRect(
                    p, VanVec2(p.x + iconSz, p.y + iconSz),
                    VanGui::GetColorU32(VanGuiCol_Border));
                VanGui::Dummy(VanVec2(iconSz, iconSz));
            }

            VanGui::SameLine();

            const std::string texLabel = tex.name + "  " +
                std::to_string(tex.width) + "\xc3\x97" +
                std::to_string(tex.height) + " " + TexFormatName(tex.format);

            if (VanGui::Selectable(texLabel.c_str(), selected,
                                  VanGuiSelectableFlags_SpanAllColumns,
                                  VanVec2(0, iconSz))) {
                SelectTexture(fi, logicalTi, logicalXi);
            }

            if (VanGui::BeginPopupContextItem("##ctx")) {
                if (VanGui::MenuItem("Export to DDS\xe2\x80\xa6")) {
                    selFile_ = fi; selTPK_ = logicalTi; selTexture_ = logicalXi;
                    exportDialog_.Show("Export DDS", FileDialog::Mode::Save, {".dds"},
                        [this, fi, logicalTi, logicalXi](const std::filesystem::path& dest) {
                            auto* f_ = (fi < (int)files_.size()) ? &files_[fi] : nullptr;
                            if (!f_ || logicalTi >= (int)f_->tpks.size()) return;
                            auto& t_ = f_->tpks[logicalTi];
                            if (logicalXi >= (int)t_.textures.size()) return;
                            auto& tex_ = t_.textures[logicalXi];
                            const auto finalPath = dest.extension().empty()
                                ? std::filesystem::path(dest.string() + ".dds") : dest;
                            auto r = DDSCodec::Export(tex_,
                                std::span<const uint8_t>(tex_.data), finalPath);
                            if (!r) lastError_ = r.error;
                        },
                        (tex.name.empty() ? "texture" : tex.name) + ".dds");
                }
                if (!lf.compressed) {
                    VanGui::Separator();
                    if (VanGui::MenuItem("Replace from DDS\xe2\x80\xa6")) {
                        selFile_ = fi; selTPK_ = logicalTi; selTexture_ = logicalXi;
                        if (taskQueue_) {
                            replaceDialog_.Show("Select replacement DDS",
                                FileDialog::Mode::Open, {".dds"},
                                [this, fi, logicalTi, logicalXi](
                                    const std::filesystem::path& dds) {
                                    if (taskQueue_)
                                        DoReplace(fi, logicalTi, logicalXi, dds, *taskQueue_);
                                });
                        }
                    }
                    if (backupMgr_.HasManifest(lf.path)) {
                        if (VanGui::MenuItem("Revert to original")) {
                            if (taskQueue_) DoRevert(fi, *taskQueue_);
                        }
                    }
                }
                VanGui::EndPopup();
            }

            VanGui::PopID();
        };

        // ── Stream bundle: flat deduplicated list (cached, virtually-scrolled) ──
        if (isStreamBundle) {
            // Only rebuild the flat list when the file or its version changes.
            if (cachedFlatFileIdx_ != fi || cachedFlatVersion_ != flatVersion_) {
                cachedFlat_.clear();
                std::unordered_map<uint32_t, bool> seen;
                for (int ti = 0; ti < (int)lf.tpks.size(); ++ti) {
                    auto& tpk = lf.tpks[ti];
                    for (int xi = 0; xi < (int)tpk.textures.size(); ++xi) {
                        const auto& tex = tpk.textures[xi];
                        if (seen.count(tex.nameHash)) continue;
                        seen[tex.nameHash] = true;
                        cachedFlat_.push_back({ ti, xi, &tex });
                    }
                }
                cachedFlatFileIdx_ = fi;
                cachedFlatVersion_ = flatVersion_;
            }

            VanGui::TextDisabled("%zu texture(s) \xe2\x80\x94 stream bundle, regions merged",
                                cachedFlat_.size());
            VanGui::Separator();

            // Virtual scrolling: only render rows visible in the current viewport.
            const float rowH = iconSz + VanGui::GetStyle().ItemSpacing.y;
            VanGuiListClipper clipper;
            clipper.Begin((int)cachedFlat_.size(), rowH);
            while (clipper.Step())
                for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i)
                    drawTexRow(cachedFlat_[i].ti, cachedFlat_[i].xi, *cachedFlat_[i].tex);
            clipper.End();

        } else {
            // ── Regular file: two-level tree (TPK -> texture) ─────────────────
            for (int ti = 0; ti < (int)lf.tpks.size(); ++ti) {
                auto& tpk = lf.tpks[ti];
                const std::string tpkLabel = tpk.name + " (" +
                    std::to_string(tpk.textures.size()) + ")###tpk" +
                    std::to_string(fi * 1000 + ti);

                VanGuiTreeNodeFlags tpkFlags = VanGuiTreeNodeFlags_OpenOnArrow |
                                              VanGuiTreeNodeFlags_SpanFullWidth;

                if (VanGui::TreeNodeEx(tpkLabel.c_str(), tpkFlags)) {
                    for (int xi = 0; xi < (int)tpk.textures.size(); ++xi)
                        drawTexRow(ti, xi, tpk.textures[xi]);
                    VanGui::TreePop();
                }
            }
        }

        // ── Batch replace (per file) ──────────────────────────────────────────
        if (!lf.compressed) {
            VanGui::Spacing();
            VanGui::PushStyleColor(VanGuiCol_Text, VanGui::GetStyleColorVec4(VanGuiCol_SliderGrab));
            if (VanGui::SmallButton(("Batch Replace\xe2\x80\xa6##" + std::to_string(fi)).c_str())) {
                const int capFi = fi;
                batchDialog_.Show("Select folder of DDS files",
                    FileDialog::Mode::Open, {},
                    [this, capFi](const std::filesystem::path& folder) {
                        if (taskQueue_) DoBatchReplace(folder, capFi, *taskQueue_);
                    });
            }
            VanGui::PopStyleColor();
        }

        VanGui::TreePop();
    }

    // Apply any deferred file close requested via the ✕ button.
    if (closeRequest >= 0)
        CloseFile(closeRequest);

    exportDialog_.Draw();
    replaceDialog_.Draw();
    batchDialog_.Draw();
}
// ─── DrawPreview ─────────────────────────────────────────────────────────────
void TexturePackPanel::DrawPreview() {
    const Texture* tex = SelectedTexture();
    if (!tex) {
        VanGui::TextDisabled("Select a texture to preview.");
        return;
    }

    // ── Thumbnail ───────────────────────────────────────────────────────────
    if (previewGLHandle_ != 0) {
        const float avail  = VanGui::GetContentRegionAvail().x;
        const float aspect = (previewH_ > 0) ? (float)previewW_/(float)previewH_ : 1.f;
        const float dispW  = std::min(avail, 400.f);
        const float dispH  = dispW / aspect;

        VanGui::Image((VanTextureID)(uintptr_t)previewGLHandle_, VanVec2(dispW, dispH));

        if (VanGui::IsItemHovered()) {
            (void)VanGui::BeginTooltip();
            const float zoom = 128.f;
            VanVec2 pos   = VanGui::GetMousePos();
            VanVec2 bbMin = VanGui::GetItemRectMin();
            VanVec2 bbMax = VanGui::GetItemRectMax();
            float  u = (pos.x-bbMin.x)/(bbMax.x-bbMin.x);
            float  v = (pos.y-bbMin.y)/(bbMax.y-bbMin.y);
            float  regionSize = zoom/(float)previewW_;
            float  u0 = std::clamp(u-regionSize*0.5f, 0.f, 1.f-regionSize);
            float  v0 = std::clamp(v-regionSize*0.5f, 0.f, 1.f-regionSize);
            VanGui::Image((VanTextureID)(uintptr_t)previewGLHandle_,
                         VanVec2(zoom*aspect, zoom),
                         VanVec2(u0, v0),
                         VanVec2(u0+regionSize, v0+regionSize*aspect));
            VanGui::EndTooltip();
        }
    } else {
        VanGui::TextDisabled("(preview unavailable for format %s)", TexFormatName(tex->format));
    }

    VanGui::Separator();

    if (taskQueue_)
        DrawMetadataCard(*tex, *taskQueue_);
}

// ─── DrawMetadataCard ────────────────────────────────────────────────────────
void TexturePackPanel::DrawMetadataCard(const Texture& tex, TaskQueue& taskQueue) {
    auto row = [](const char* key, const char* val) {
        VanGui::TableNextRow();
        (void)VanGui::TableSetColumnIndex(0); VanGui::TextDisabled("%s", key);
        (void)VanGui::TableSetColumnIndex(1); VanGui::TextUnformatted(val);
    };
    auto rowS = [](const char* key, const std::string& val) {
        VanGui::TableNextRow();
        (void)VanGui::TableSetColumnIndex(0); VanGui::TextDisabled("%s", key);
        (void)VanGui::TableSetColumnIndex(1); VanGui::TextUnformatted(val.c_str());
    };

    if (VanGui::BeginTable("##meta", 2,
        VanGuiTableFlags_RowBg | VanGuiTableFlags_BordersInnerV |
        VanGuiTableFlags_SizingStretchSame)) {
        VanGui::TableSetupColumn("Property", VanGuiTableColumnFlags_WidthFixed, 100.f);
        VanGui::TableSetupColumn("Value");

        rowS("Name",    tex.name);
        row("Hash",     [&]{ static char buf[32]; std::snprintf(buf,sizeof(buf),"0x%08X",tex.nameHash); return buf; }());
        row("Dims",     [&]{ static char buf[32]; std::snprintf(buf,sizeof(buf),"%u × %u",tex.width,tex.height); return buf; }());
        row("Format",   TexFormatName(tex.format));
        row("Mipmaps",  [&]{ static char buf[8];  std::snprintf(buf,sizeof(buf),"%u",tex.mipmaps); return buf; }());
        row("Size",     [&]{ static char buf[48]; std::snprintf(buf,sizeof(buf),"%s (%zu B)",FormatBytes(tex.data.size()).c_str(),tex.data.size()); return buf; }());
        row("Offset",   [&]{ static char buf[32]; std::snprintf(buf,sizeof(buf),"0x%08X",tex.fileDataOffset); return buf; }());

        // Repack eligibility
        const LoadedFile* lf = SelectedFile();
        if (lf) {
            row("Editable", lf->compressed ? "No (compressed source)" : "Yes");
            if (backupMgr_.HasBak(lf->path))
                row("Backup",   ".bak exists");
            if (backupMgr_.HasManifest(lf->path))
                row("Manifest", "patches applied (revert available)");
        }

        VanGui::EndTable();
    }

    // ── Write-status banner ─────────────────────────────────────────────────
    if (!writeStatus_.empty()) {
        VanGui::Spacing();
        const bool isErr = writeStatus_.rfind("Error", 0) == 0 ||
                           writeStatus_.rfind("Failed", 0) == 0;
        VanGui::PushStyleColor(VanGuiCol_Text,
            isErr ? VanVec4(1,0.4f,0.4f,1) : VanVec4(0.4f,1,0.6f,1));
        VanGui::TextWrapped("%s", writeStatus_.c_str());
        VanGui::PopStyleColor();
    }

    VanGui::Spacing();

    // ── Phase 1: Export ─────────────────────────────────────────────────────
    if (VanGui::Button("Export to DDS…")) {
        const Texture* selForName = SelectedTexture();
        const std::string defName =
            (selForName && !selForName->name.empty() ? selForName->name : "texture") + ".dds";
        exportDialog_.Show("Export DDS", FileDialog::Mode::Save, {".dds"},
            [this](const std::filesystem::path& dest) {
                auto* t = SelectedTexture();
                if (!t) return;
                const auto finalPath = dest.extension().empty()
                    ? std::filesystem::path(dest.string()+".dds") : dest;
                auto r = DDSCodec::Export(*t, std::span<const uint8_t>(t->data), finalPath);
                writeStatus_ = r ? "Exported: " + finalPath.filename().string() : "Error: " + r.error;
                if (!r) lastError_ = r.error;
            },
            defName);
    }

    exportDialog_.Draw();

    // ── Phase 2: Replace / Revert ───────────────────────────────────────────
    const LoadedFile* lf = SelectedFile();
    if (!lf || lf->compressed) {
        VanGui::SameLine();
        VanGui::TextDisabled("(compressed — replace unavailable)");
        return;
    }

    VanGui::SameLine();
    if (VanGui::Button("Replace from DDS…")) {
        const int capFi = selFile_, capTi = selTPK_, capXi = selTexture_;
        replaceDialog_.Show("Select replacement DDS",
            FileDialog::Mode::Open, {".dds"},
            [this, capFi, capTi, capXi](const std::filesystem::path& dds) {
                DoReplace(capFi, capTi, capXi, dds, *taskQueue_);
            });
    }
    replaceDialog_.Draw();

    if (backupMgr_.HasManifest(lf->path)) {
        VanGui::SameLine();
        if (VanGui::Button("Revert to Original")) {
            DoRevert(selFile_, taskQueue);
        }
    }

    // Draw Algorithm-B confirmation dialog if pending
    DrawRepackConfirmDialog(taskQueue);
}

// ─── DrawRepackConfirmDialog ──────────────────────────────────────────────────
void TexturePackPanel::DrawRepackConfirmDialog(TaskQueue& taskQueue) {
    if (!showRepackConfirm_) return;

    VanGui::OpenPopup("Repack Confirmation##repack");

    VanGui::SetNextWindowSize(VanVec2(480, 0), VanGuiCond_Always);
    if (VanGui::BeginPopupModal("Repack Confirmation##repack", nullptr,
                                VanGuiWindowFlags_AlwaysAutoResize)) {
        VanGui::TextWrapped(
            "The replacement DDS is larger than the current texture slot.\n\n"
            "Algorithm B (full TPK repack) is required. This will:\n"
            "  • Rebuild the entire TPK pixel data block\n"
            "  • Shift offsets for all sibling textures\n"
            "  • Grow the .BUN file on disk\n\n"
            "A .bak backup will be made before writing.\n\n"
            "For STREAM*.BUN files this may affect streaming performance.\n"
            "Proceed?");

        VanGui::Spacing();

        VanGui::PushStyleColor(VanGuiCol_Button, VanVec4(0.7f,0.2f,0.2f,1.f));
        if (VanGui::Button("Proceed with Repack", VanVec2(200,0))) {
            VanGui::CloseCurrentPopup();
            showRepackConfirm_ = false;
            // Run the actual repack (no size check this time)
            DoReplace(repackConfirmFile_, repackConfirmTPK_, repackConfirmTex_,
                      pendingReplaceDDS_, taskQueue);
            pendingReplaceDDS_.clear();
        }
        VanGui::PopStyleColor();

        VanGui::SameLine();
        if (VanGui::Button("Cancel", VanVec2(120,0))) {
            VanGui::CloseCurrentPopup();
            showRepackConfirm_    = false;
            pendingReplaceDDS_.clear();
            writeStatus_ = "Repack cancelled.";
        }

        VanGui::EndPopup();
    }
}

// ─── DoReplace ───────────────────────────────────────────────────────────────
void TexturePackPanel::DoReplace(int fileIdx, int tpkIdx, int texIdx,
                                  const std::filesystem::path& ddsPath,
                                  TaskQueue& taskQueue)
{
    if (fileIdx < 0 || fileIdx >= (int)files_.size()) return;
    auto& lf = files_[fileIdx];
    if (lf.compressed) { writeStatus_ = "Error: cannot patch compressed source."; return; }

    // Check whether the new DDS fits in-place (Algorithm A) or needs repack (Algorithm B).
    // We do this on the UI thread (just reading the DDS header) to decide whether to
    // show the confirmation dialog before launching the worker.
    Texture tmpTex;
    auto ddsResult = DDSCodec::Import(tmpTex, ddsPath);
    if (!ddsResult) {
        writeStatus_ = "Error loading DDS: " + ddsResult.error;
        lastError_   = ddsResult.error;
        return;
    }

    // Find the existing texture entry so we can compare sizes.
    if (tpkIdx >= (int)lf.tpks.size()) return;
    auto& tpk = lf.tpks[tpkIdx];
    if (texIdx >= (int)tpk.textures.size()) return;
    const Texture& existing = tpk.textures[texIdx];

    const bool needsRepack = tmpTex.data.size() > existing.data.size();

    if (needsRepack && !showRepackConfirm_) {
        // Show confirmation dialog — don't proceed yet.
        repackConfirmFile_ = fileIdx;
        repackConfirmTPK_  = tpkIdx;
        repackConfirmTex_  = texIdx;
        pendingReplaceDDS_ = ddsPath;
        showRepackConfirm_ = true;
        return;
    }

    // Launch the async patch.
    // Fix 1.2: worker writes status/error into a shared struct; onDone (UI thread)
    // assigns them to member fields. writeStatus_.rfind() in onDone is therefore
    // safe — it reads a value that was written earlier on the same thread (fix 1.4).
    writeStatus_ = "Patching…";

    struct ReplaceResult {
        std::string status;
        std::string error;   // non-empty only on failure
        bool        needsRefresh = false;
    };
    auto sharedReplace = std::make_shared<ReplaceResult>();

    const std::filesystem::path filePath = lf.path;
    const uint32_t targetHash = existing.nameHash;

    taskQueue.Submit(
        "Patching " + existing.name + " in " + lf.path.filename().string(),
        [sharedReplace, filePath, targetHash, ddsPath, this](ProgressState& p) {
            p.fraction = 0.f;
            p.SetDetail("Preparing backup…");

            if (!backupMgr_.EnsureFileBak(filePath)) {
                sharedReplace->status = "Error: could not create .bak for " + filePath.filename().string();
                return;
            }

            p.fraction = 0.2f;
            p.SetDetail("Applying patch…");

            ReplaceRequest req;
            req.nameHash = targetHash;
            req.ddsPath  = ddsPath;

            auto results = repacker_.PatchFile(filePath, {req}, backupMgr_,
                [&](float frac) { p.fraction = 0.2f + frac * 0.7f; });

            p.fraction = 1.f;

            if (results.empty()) {
                sharedReplace->status = "Error: no patch results returned.";
                return;
            }
            const auto& pr = results[0];
            if (!pr.ok) {
                sharedReplace->status = "Error: " + pr.error;
                sharedReplace->error  = pr.error;
                LOG_ERROR("DoReplace: {}", pr.error);
            } else {
                sharedReplace->status = std::string("Patched (") +
                    (pr.usedRepack ? "Algorithm B repack" : "in-place") + "). Refreshing…";
                sharedReplace->needsRefresh = true;
                LOG_INFO("DoReplace: OK, usedRepack={}", pr.usedRepack);
            }
        },
        [this, fileIdx, sharedReplace, &taskQueue]() {
            // UI thread only — safe to assign member fields here.
            writeStatus_ = sharedReplace->status;
            if (!sharedReplace->error.empty())
                lastError_ = sharedReplace->error;
            if (sharedReplace->needsRefresh)
                RefreshFile(fileIdx, taskQueue);
        }
    );
}

// ─── DoRevert ────────────────────────────────────────────────────────────────
void TexturePackPanel::DoRevert(int fileIdx, TaskQueue& taskQueue) {
    if (fileIdx < 0 || fileIdx >= (int)files_.size()) return;
    auto& lf = files_[fileIdx];
    if (!backupMgr_.HasManifest(lf.path)) {
        writeStatus_ = "No manifest — nothing to revert.";
        return;
    }

    writeStatus_ = "Reverting…";
    const std::filesystem::path filePath = lf.path;

    // Fix 1.2: worker computes status locally; onDone assigns to member field.
    struct RevertResult { std::string status; };
    auto sharedRevert = std::make_shared<RevertResult>();

    taskQueue.Submit(
        "Reverting " + lf.path.filename().string(),
        [sharedRevert, filePath, this](ProgressState& p) {
            p.fraction = -1.f;
            p.SetDetail("Restoring original bytes…");
            const int n = backupMgr_.RevertAll(filePath);
            p.fraction = 1.f;
            if (n < 0) {
                sharedRevert->status = "Error: could not open manifest.";
            } else {
                sharedRevert->status = "Reverted " + std::to_string(n) + " region(s).";
                LOG_INFO("DoRevert: reverted {} regions in '{}'", n, filePath.filename().string());
            }
        },
        [this, fileIdx, sharedRevert, &taskQueue]() {
            // UI thread only — safe to assign writeStatus_ here.
            writeStatus_ = sharedRevert->status;
            RefreshFile(fileIdx, taskQueue);
        }
    );
}

// ─── Phase 5: global revert (Ctrl+Z) ─────────────────────────────────────────
bool TexturePackPanel::CanRevertSelectedFile() const {
    if (selFile_ < 0 || selFile_ >= (int)files_.size())
        return false;
    return backupMgr_.HasManifest(files_[selFile_].path);
}

void TexturePackPanel::RevertSelectedFile(TaskQueue& taskQueue) {
    if (selFile_ < 0 || selFile_ >= (int)files_.size())
        return;
    DoRevert(selFile_, taskQueue);
}

// ─── DoBatchReplace ──────────────────────────────────────────────────────────
void TexturePackPanel::DoBatchReplace(const std::filesystem::path& folderPath,
                                       int fileIdx, TaskQueue& taskQueue)
{
    if (fileIdx < 0 || fileIdx >= (int)files_.size()) return;
    auto& lf = files_[fileIdx];
    if (lf.compressed) { writeStatus_ = "Error: compressed source not patchable."; return; }

    // Collect all .dds files in the folder; parse hash from filename "name_<hash>.dds".
    std::vector<ReplaceRequest> requests;
    std::error_code ec;
    for (auto& entry : std::filesystem::directory_iterator(folderPath, ec)) {
        if (!entry.is_regular_file()) continue;
        const auto& p = entry.path();
        if (p.extension() != ".dds") continue;

        // Try to parse a trailing hex hash from the stem: "whatever_AABBCCDD"
        const std::string stem = p.stem().string();
        const auto under = stem.rfind('_');
        if (under != std::string::npos && under + 1 < stem.size()) {
            const std::string maybeHash = stem.substr(under+1);
            try {
                uint32_t hash = (uint32_t)std::stoul(maybeHash, nullptr, 16);
                requests.push_back({hash, p});
                continue;
            } catch (...) {}
        }
        // No hash suffix — try matching by name later (skip for now, need hash).
        LOG_WARN("DoBatchReplace: skipping '{}' — no _<hash> suffix", p.filename().string());
    }

    if (requests.empty()) {
        writeStatus_ = "No matching DDS files found (need name_<HASH>.dds format).";
        return;
    }

    writeStatus_ = "Batch patching " + std::to_string(requests.size()) + " texture(s)…";
    const std::filesystem::path filePath = lf.path;

    // Fix 1.2: worker accumulates status locally; onDone assigns to writeStatus_.
    struct BatchResult { std::string status; };
    auto sharedBatch = std::make_shared<BatchResult>();

    taskQueue.Submit(
        "Batch replace in " + lf.path.filename().string(),
        [sharedBatch, filePath, requests, this](ProgressState& p) {
            p.fraction = 0.f;
            p.SetDetail("Preparing backup…");
            if (!backupMgr_.EnsureFileBak(filePath)) {
                sharedBatch->status = "Error: could not create .bak.";
                return;
            }
            p.fraction = 0.1f;
            p.SetDetail("Patching " + std::to_string(requests.size()) + " textures…");

            auto results = repacker_.PatchFile(filePath, requests, backupMgr_,
                [&](float frac) { p.fraction = 0.1f + frac * 0.85f; });

            p.fraction = 1.f;

            int ok = 0, fail = 0;
            for (auto& r : results) { if (r.ok) ++ok; else ++fail; }

            if (fail > 0)
                sharedBatch->status = "Batch complete: " + std::to_string(ok) + " OK, " +
                                      std::to_string(fail) + " failed.";
            else
                sharedBatch->status = "Batch complete: all " + std::to_string(ok) + " textures patched.";

            LOG_INFO("DoBatchReplace: {} ok, {} fail in '{}'", ok, fail, filePath.filename().string());
        },
        [this, fileIdx, sharedBatch, &taskQueue]() {
            // UI thread only — safe to assign writeStatus_ here.
            writeStatus_ = sharedBatch->status;
            RefreshFile(fileIdx, taskQueue);
        }
    );
}

// ─── CloseFile / CloseAll ─────────────────────────────────────────────────────
void TexturePackPanel::CloseFile(int fileIdx) {
    if (fileIdx < 0 || fileIdx >= (int)files_.size()) return;

    const bool wasSelected = (selFile_ == fileIdx);
    files_.erase(files_.begin() + fileIdx);

    // Invalidate the cached flat list and bump its version.
    cachedFlatFileIdx_ = -1;
    ++flatVersion_;

    // Adjust selection indices to track the removal.
    if (wasSelected) {
        selFile_ = -1; selTPK_ = -1; selTexture_ = -1;
        DestroyGLTexture();
    } else if (selFile_ > fileIdx) {
        --selFile_;
    }
}

void TexturePackPanel::CloseAll() {
    files_.clear();
    cachedFlatFileIdx_ = -1;
    ++flatVersion_;
    selFile_ = -1; selTPK_ = -1; selTexture_ = -1;
    DestroyGLTexture();
}

// ─── RefreshFile ─────────────────────────────────────────────────────────────
void TexturePackPanel::RefreshFile(int fileIdx, TaskQueue& taskQueue) {
    if (fileIdx < 0 || fileIdx >= (int)files_.size()) return;
    const auto path = files_[fileIdx].path;

    // Save the selected texture's hash so we can re-select it after reload.
    const bool wasSelected = (selFile_ == fileIdx);
    uint32_t savedHash = 0;
    if (wasSelected) {
        const Texture* cur = SelectedTexture();
        if (cur) savedHash = cur->nameHash;
    }

    // Evict the stale entry and reload it.
    files_.erase(files_.begin() + fileIdx);

    // Invalidate cached flat list for this file.
    if (cachedFlatFileIdx_ == fileIdx) { cachedFlatFileIdx_ = -1; }
    ++flatVersion_;

    // Adjust selection indices.
    if (wasSelected) { selFile_ = -1; selTPK_ = -1; selTexture_ = -1; DestroyGLTexture(); }
    else if (selFile_ > fileIdx) --selFile_;

    // Fix 1.1/1.2: worker builds LoadedFile locally; all member writes happen in onDone.
    struct RefreshResult { LoadedFile lf; std::string error; bool ok = false; };
    auto sharedRefresh = std::make_shared<RefreshResult>();

    loading_ = true;
    taskQueue.Submit(
        "Refreshing " + path.filename().string(),
        [path, sharedRefresh](ProgressState& p) {
            p.fraction = -1.f;
            p.SetDetail("Re-reading file…");

            auto binResult = BINFile::Open(path);
            if (!binResult) {
                sharedRefresh->error = binResult.error;
                LOG_ERROR("RefreshFile: {}", binResult.error);
                return;
            }
            BINFile& bin = binResult.value;
            p.SetDetail("Parsing chunks…");

            LoadedFile lf;
            lf.path       = path;
            lf.compressed = bin.WasDecompressed();

            bin.ForEachTPK([&](std::span<const uint8_t> payload, uint64_t absOff) {
                auto r = TPKBlockParser::Parse(payload, absOff);
                if (r) {
                    r.value.sourceFile       = path;
                    r.value.compressedSource = lf.compressed;
                    lf.tpks.push_back(std::move(r.value));
                }
            });
            p.fraction = 1.f;
            sharedRefresh->lf = std::move(lf);
            sharedRefresh->ok = true;
        },
        [this, sharedRefresh, savedHash, wasSelected]() {
            // UI thread only — safe to write member fields here.
            loading_ = false;
            ++flatVersion_; // invalidate any cached flat list
            if (!sharedRefresh->ok) {
                lastError_ = sharedRefresh->error;
                return;
            }
            files_.push_back(std::move(sharedRefresh->lf));
            if (selFile_ < 0 && !files_.empty()) {
                selFile_ = (int)files_.size() - 1;
                auto& f  = files_.back();
                // Try to restore the previously selected texture by its name hash.
                if (wasSelected && savedHash != 0) {
                    for (int ti = 0; ti < (int)f.tpks.size(); ++ti) {
                        auto& tpk = f.tpks[ti];
                        for (int xi = 0; xi < (int)tpk.textures.size(); ++xi) {
                            if (tpk.textures[xi].nameHash == savedHash) {
                                SelectTexture(selFile_, ti, xi);
                                return;
                            }
                        }
                    }
                }
                // Hash not found (e.g. after repack changed layout) — fall back to index 0.
                if (!f.tpks.empty() && !f.tpks[0].textures.empty()) {
                    selTPK_ = 0; selTexture_ = 0;
                    SelectTexture(selFile_, 0, 0);
                }
            }
        }
    );
}

// ─── SelectedFile ────────────────────────────────────────────────────────────
TexturePackPanel::LoadedFile* TexturePackPanel::SelectedFile() {
    if (selFile_ < 0 || selFile_ >= (int)files_.size()) return nullptr;
    return &files_[selFile_];
}

// ─── SelectTexture ───────────────────────────────────────────────────────────
void TexturePackPanel::SelectTexture(int fi, int ti, int xi) {
    selFile_    = fi;
    selTPK_     = ti;
    selTexture_ = xi;
    DestroyGLTexture();
    const Texture* tex = SelectedTexture();
    if (tex) UploadGLTexture(*tex);
}

// ─── SelectedTexture ─────────────────────────────────────────────────────────
const Texture* TexturePackPanel::SelectedTexture() const {
    if (selFile_ < 0 || selFile_ >= (int)files_.size()) return nullptr;
    auto& f = files_[selFile_];
    if (selTPK_ < 0 || selTPK_ >= (int)f.tpks.size()) return nullptr;
    auto& tpk = f.tpks[selTPK_];
    if (selTexture_ < 0 || selTexture_ >= (int)tpk.textures.size()) return nullptr;
    return &tpk.textures[selTexture_];
}

// ─── UploadGLTexture ─────────────────────────────────────────────────────────
void TexturePackPanel::UploadGLTexture(const Texture& tex) {
    if (tex.format == TexFormat::PAL8 || tex.format == TexFormat::Unknown) return;
    if (tex.data.empty()) return;

    auto rgba = DecodeToRGBA(tex);
    if (rgba.empty()) return;

    GLuint handle = 0;
    glGenTextures(1, &handle);
    glBindTexture(GL_TEXTURE_2D, handle);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
                 tex.width, tex.height, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
    glBindTexture(GL_TEXTURE_2D, 0);

    previewGLHandle_ = handle;
    previewW_        = tex.width;
    previewH_        = tex.height;
}

// ─── Thumbnail helpers ────────────────────────────────────────────────────────
// Returns a tiny 32×32 GL texture for `tex`, uploading it on first call.
// The cache is keyed by nameHash (unique per texture within a session).
// PAL8/Unknown formats return 0 — the tree falls back to a plain text row.
uint32_t TexturePackPanel::GetOrCreateThumb(const Texture& tex) {
    if (tex.format == TexFormat::PAL8 || tex.format == TexFormat::Unknown) return 0;
    if (tex.data.empty()) return 0;

    auto it = thumbCache_.find(tex.nameHash);
    if (it != thumbCache_.end()) return it->second;

    auto rgba = DecodeToRGBA(tex);
    if (rgba.empty()) { thumbCache_[tex.nameHash] = 0; return 0; }

    // Scale to thumb size (32×32) using a simple box filter.
    constexpr int kThumb = 32;
    const int srcW = tex.width, srcH = tex.height;
    std::vector<uint8_t> thumb(kThumb * kThumb * 4, 0);

    for (int ty = 0; ty < kThumb; ++ty) {
        for (int tx = 0; tx < kThumb; ++tx) {
            // Map thumb pixel back to source region.
            const int sx0 = tx     * srcW / kThumb;
            const int sx1 = (tx+1) * srcW / kThumb;
            const int sy0 = ty     * srcH / kThumb;
            const int sy1 = (ty+1) * srcH / kThumb;
            uint32_t r=0, g=0, b=0, a=0, n=0;
            for (int sy = sy0; sy < std::max(sy0+1, sy1); ++sy)
                for (int sx = sx0; sx < std::max(sx0+1, sx1); ++sx) {
                    if (sx >= srcW || sy >= srcH) continue;
                    const uint8_t* p = rgba.data() + (sy*srcW+sx)*4;
                    r+=p[0]; g+=p[1]; b+=p[2]; a+=p[3]; ++n;
                }
            if (n == 0) n = 1;
            uint8_t* dst = thumb.data() + (ty*kThumb+tx)*4;
            dst[0]=(uint8_t)(r/n); dst[1]=(uint8_t)(g/n);
            dst[2]=(uint8_t)(b/n); dst[3]=(uint8_t)(a/n);
        }
    }

    GLuint handle = 0;
    glGenTextures(1, &handle);
    glBindTexture(GL_TEXTURE_2D, handle);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, kThumb, kThumb, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, thumb.data());
    glBindTexture(GL_TEXTURE_2D, 0);

    thumbCache_[tex.nameHash] = handle;
    return handle;
}

void TexturePackPanel::ClearThumbCache() {
    for (auto& [hash, h] : thumbCache_)
        if (h) glDeleteTextures(1, &h);
    thumbCache_.clear();
}


void TexturePackPanel::DestroyGLTexture() {
    if (previewGLHandle_) {
        glDeleteTextures(1, &previewGLHandle_);
        previewGLHandle_ = 0;
        previewW_ = previewH_ = 0;
    }
}

// ─── ExportSelected ──────────────────────────────────────────────────────────
void TexturePackPanel::ExportSelected(const std::filesystem::path& destPath) {
    const Texture* t = SelectedTexture();
    if (!t) return;
    auto r = DDSCodec::Export(*t, std::span<const uint8_t>(t->data), destPath);
    writeStatus_ = r ? "Exported: " + destPath.filename().string() : "Error: " + r.error;
    if (!r) lastError_ = r.error;
}

} // namespace nfsmw
