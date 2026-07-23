#include "ui/MinimapPanel.h"
#include "ui/AppShell.h"
#include "core/Logger.h"
#include "core/StringHash.h"

#include <vangui.h>
#include "renderer/GLCompat.h"

// stb_image_write for PNG export (header-only, compiled once here).
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

// stb_image for PNG import (used by MinimapRepacker::PrepareSourceImage).
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>

namespace nfsmw {

// ─────────────────────────────────────────────────────────────────────────────
// DXT3 software decode (main-thread; called once on load)
// ─────────────────────────────────────────────────────────────────────────────

/*static*/ std::vector<uint8_t> MinimapPanel::DecodeDXT3(const MinimapTile& tile) {
    const uint32_t W = kTileWidth, H = kTileHeight;
    std::vector<uint8_t> rgba(W * H * 4, 0xFF);

    if (tile.dxt3Data.size() < kTilePixelBytes)
        return rgba;

    auto unpack565 = [](uint16_t v, uint8_t& r, uint8_t& g, uint8_t& b) {
        r = static_cast<uint8_t>(((v >> 11) & 0x1F) * 255 / 31);
        g = static_cast<uint8_t>(((v >>  5) & 0x3F) * 255 / 63);
        b = static_cast<uint8_t>(((v      ) & 0x1F) * 255 / 31);
    };

    const uint8_t* src = tile.dxt3Data.data();
    const uint32_t bw = (W + 3) / 4, bh = (H + 3) / 4;

    for (uint32_t by = 0; by < bh; ++by) {
        for (uint32_t bx = 0; bx < bw; ++bx) {
            const uint8_t* blk = src + (by * bw + bx) * 16;

            // 8 bytes of 4-bit alpha values (two per byte, low nibble first).
            uint8_t alpha[16];
            for (int i = 0; i < 8; ++i) {
                alpha[i * 2    ] = static_cast<uint8_t>((blk[i] & 0x0F) * 17);
                alpha[i * 2 + 1] = static_cast<uint8_t>(((blk[i] >> 4) & 0x0F) * 17);
            }

            // 8 bytes of DXT1 colour data.
            const uint8_t* cblk = blk + 8;
            uint16_t c0, c1;
            std::memcpy(&c0, cblk,     2);
            std::memcpy(&c1, cblk + 2, 2);
            uint32_t lut;
            std::memcpy(&lut, cblk + 4, 4);

            uint8_t cr[4], cg[4], cb[4];
            unpack565(c0, cr[0], cg[0], cb[0]);
            unpack565(c1, cr[1], cg[1], cb[1]);
            cr[2] = static_cast<uint8_t>((2 * cr[0] + cr[1]) / 3);
            cg[2] = static_cast<uint8_t>((2 * cg[0] + cg[1]) / 3);
            cb[2] = static_cast<uint8_t>((2 * cb[0] + cb[1]) / 3);
            cr[3] = static_cast<uint8_t>((cr[0] + 2 * cr[1]) / 3);
            cg[3] = static_cast<uint8_t>((cg[0] + 2 * cg[1]) / 3);
            cb[3] = static_cast<uint8_t>((cb[0] + 2 * cb[1]) / 3);

            for (int i = 0; i < 16; ++i) {
                const uint32_t px = bx * 4 + (i % 4);
                const uint32_t py = by * 4 + (i / 4);
                if (px >= W || py >= H) continue;
                const int idx = (lut >> (i * 2)) & 3;
                uint8_t* out = rgba.data() + (py * W + px) * 4;
                out[0] = cr[idx];
                out[1] = cg[idx];
                out[2] = cb[idx];
                out[3] = alpha[i];
            }
        }
    }
    return rgba;
}

// ─────────────────────────────────────────────────────────────────────────────
// GL resource management
// ─────────────────────────────────────────────────────────────────────────────

void MinimapPanel::FreeGLResources() {
    if (!tileTextures_.empty()) {
        glDeleteTextures(static_cast<GLsizei>(tileTextures_.size()),
                         tileTextures_.data());
        tileTextures_.clear();
    }
    if (atlasTexture_) {
        glDeleteTextures(1, &atlasTexture_);
        atlasTexture_ = 0;
    }
}

void MinimapPanel::UploadToGL() {
    FreeGLResources();

    const int n     = static_cast<int>(file_.tiles.size());
    tileTextures_.resize(n, 0);
    glGenTextures(n, tileTextures_.data());

    for (int i = 0; i < n; ++i) {
        std::vector<uint8_t> rgba = DecodeDXT3(file_.tiles[i]);

        glBindTexture(GL_TEXTURE_2D, tileTextures_[i]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
                     static_cast<GLsizei>(kTileWidth),
                     static_cast<GLsizei>(kTileHeight),
                     0, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
    }
    glBindTexture(GL_TEXTURE_2D, 0);

    RebuildAtlas();
}

void MinimapPanel::RebuildAtlas() {
    atlasW_ = file_.gridCols * static_cast<int>(kTileWidth);
    atlasH_ = file_.gridRows * static_cast<int>(kTileHeight);

    // Build a full RGBA atlas from decoded tiles.
    std::vector<uint8_t> atlasBuf(atlasW_ * atlasH_ * 4, 0);

    const int n = static_cast<int>(file_.tiles.size());
    for (int i = 0; i < n; ++i) {
        std::vector<uint8_t> rgba = DecodeDXT3(file_.tiles[i]);
        const int col = i % file_.gridCols;
        const int row = i / file_.gridCols;
        const int dstX = col * static_cast<int>(kTileWidth);
        const int dstY = row * static_cast<int>(kTileHeight);
        for (int y = 0; y < static_cast<int>(kTileHeight); ++y) {
            const uint8_t* src = rgba.data() + y * kTileWidth * 4;
            uint8_t*       dst = atlasBuf.data() +
                                 ((dstY + y) * atlasW_ + dstX) * 4;
            std::memcpy(dst, src, kTileWidth * 4);
        }
    }

    if (!atlasTexture_) glGenTextures(1, &atlasTexture_);
    glBindTexture(GL_TEXTURE_2D, atlasTexture_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
                 atlasW_, atlasH_, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, atlasBuf.data());
    glBindTexture(GL_TEXTURE_2D, 0);
}

// ─────────────────────────────────────────────────────────────────────────────
// Load
// ─────────────────────────────────────────────────────────────────────────────

void MinimapPanel::Load(const std::filesystem::path& path, TaskQueue& tasks) {
    Close();
    state_       = State::Loading;
    hoveredTile_ = selectedTile_ = -1;
    zoom_        = 1.0f;
    pan_         = VanVec2(0.0f, 0.0f);
    replaceStatus_.clear();
    exportStatus_.clear();

    auto result = std::make_shared<Result<MinimapFile>>();
    tasks.Submit("Loading minimap", [this, path, result](ProgressState& p) {
        p.fraction = -1.f;
        *result = MinimapParser::Load(path);
    }, [this, result] {
        if (!*result) {
            errorMsg_ = result->error;
            state_    = State::Error;
            return;
        }
        file_     = std::move(result->value);
        gridCols_ = file_.gridCols;
        gridRows_ = file_.gridRows;
        // Register names on the main thread where HashResolver access is safe.
        for (const auto& t : file_.tiles)
            HashResolver::Instance().Register(t.name);
        UploadToGL();
        state_    = State::Ready;
    });
}

void MinimapPanel::Close() {
    FreeGLResources();
    file_         = {};
    gridCols_     = gridRows_ = 0;
    state_        = State::Empty;
    errorMsg_.clear();
    hoveredTile_  = selectedTile_ = -1;
    replaceBusy_  = false;
    replaceStatus_.clear();
    exportStatus_.clear();
}

// ─────────────────────────────────────────────────────────────────────────────
// Top-level Draw dispatch
// ─────────────────────────────────────────────────────────────────────────────

void MinimapPanel::Draw(float w, float h, FileDialog& fd, TaskQueue& tasks) {
    switch (state_) {
        case State::Empty:   DrawEmptyState(w, h);                  return;
        case State::Loading: DrawLoadingState(w, h);                return;
        case State::Error:   DrawErrorState(w, h);                  return;
        case State::Ready:   DrawReadyState(w, h, fd, tasks);       return;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Empty / Loading / Error states
// ─────────────────────────────────────────────────────────────────────────────

void MinimapPanel::DrawEmptyState(float w, float h) {
    // Centre a drop-zone card.
    const float cardW = 360.0f, cardH = 200.0f;
    const float cx = (w - cardW) * 0.5f, cy = (h - cardH) * 0.5f;

    VanDrawList* dl   = VanGui::GetWindowDrawList();
    VanVec2      base = VanGui::GetCursorScreenPos();

    // Dashed rounded border.
    const VanU32 borderCol = VanGui::ColorConvertFloat4ToU32(
        VanVec4(g_border.x, g_border.y, g_border.z, 0.7f));
    dl->AddRect(VanVec2(base.x + cx, base.y + cy),
                VanVec2(base.x + cx + cardW, base.y + cy + cardH),
                borderCol, 8.0f, 0, 1.5f);

    // Icon and prompt text.
    VanGui::SetCursorPos(VanVec2(cx, cy + 44.0f));
    const char* icon = "[ MAP ]";
    const float iconW = VanGui::CalcTextSize(icon).x;
    VanGui::SetCursorPosX(cx + (cardW - iconW) * 0.5f);
    VanGui::PushStyleColor(VanGuiCol_Text,
        VanVec4(g_text.x, g_text.y, g_text.z, 0.25f));
    VanGui::TextUnformatted(icon);
    VanGui::PopStyleColor();

    VanGui::SetCursorPosY(VanGui::GetCursorPosY() + 16.0f);
    const char* prompt = "No minimap loaded";
    const float promptW = VanGui::CalcTextSize(prompt).x;
    VanGui::SetCursorPosX(cx + (cardW - promptW) * 0.5f);
    VanGui::PushStyleColor(VanGuiCol_Text,
        VanVec4(g_text.x, g_text.y, g_text.z, 0.55f));
    VanGui::TextUnformatted(prompt);
    VanGui::PopStyleColor();

    VanGui::SetCursorPosY(VanGui::GetCursorPosY() + 12.0f);
    const char* hint = "Use  File > Open Minimap  or drag MINI_MAP.BIN here";
    const float hintW = VanGui::CalcTextSize(hint).x;
    VanGui::SetCursorPosX(cx + (cardW - hintW) * 0.5f);
    VanGui::PushStyleColor(VanGuiCol_Text,
        VanVec4(g_text.x, g_text.y, g_text.z, 0.35f));
    VanGui::TextUnformatted(hint);
    VanGui::PopStyleColor();
}

void MinimapPanel::DrawLoadingState(float w, float h) {
    const char* msg = "Loading minimap\xe2\x80\xa6";
    const float tw  = VanGui::CalcTextSize(msg).x;
    VanGui::SetCursorPos(VanVec2((w - tw) * 0.5f, (h - VanGui::GetTextLineHeight()) * 0.5f));
    VanGui::PushStyleColor(VanGuiCol_Text,
        VanVec4(g_text.x, g_text.y, g_text.z, 0.6f));
    VanGui::TextUnformatted(msg);
    VanGui::PopStyleColor();
}

void MinimapPanel::DrawErrorState(float w, float h) {
    const float tw = VanGui::CalcTextSize(errorMsg_.c_str()).x;
    VanGui::SetCursorPos(VanVec2((w - tw) * 0.5f, (h * 0.5f) - 24.0f));
    VanGui::PushStyleColor(VanGuiCol_Text, VanVec4(0.95f, 0.35f, 0.35f, 1.0f));
    VanGui::TextUnformatted(errorMsg_.c_str());
    VanGui::PopStyleColor();
}

// ─────────────────────────────────────────────────────────────────────────────
// Ready state: toolbar + viewer + tile info
// ─────────────────────────────────────────────────────────────────────────────

void MinimapPanel::DrawReadyState(float w, float h, FileDialog& fd, TaskQueue& tasks) {
    // Toolbar (fixed height at top).
    constexpr float kToolbarH    = 40.0f;
    constexpr float kInfoPanelH  = 80.0f;

    DrawToolbar(fd, tasks);
    VanGui::Dummy(VanVec2(0.0f, 4.0f));

    // Separator.
    {
        VanDrawList* dl  = VanGui::GetWindowDrawList();
        VanVec2 pos      = VanGui::GetCursorScreenPos();
        dl->AddLine(pos, VanVec2(pos.x + w, pos.y),
                    VanGui::ColorConvertFloat4ToU32(
                        VanVec4(g_border.x, g_border.y, g_border.z, 0.5f)));
    }
    VanGui::Dummy(VanVec2(0.0f, 4.0f));

    const float viewH = h - kToolbarH - kInfoPanelH - 24.0f;
    DrawViewer(w, viewH);
    DrawTileInfo();
}

// ─────────────────────────────────────────────────────────────────────────────
// Toolbar
// ─────────────────────────────────────────────────────────────────────────────

void MinimapPanel::DrawToolbar(FileDialog& fd, TaskQueue& tasks) {
    // File info.
    VanGui::PushStyleColor(VanGuiCol_Text,
        VanVec4(g_text.x, g_text.y, g_text.z, 0.55f));
    VanGui::Text("%d tiles  ·  %d×%d px  ·  %s",
        static_cast<int>(file_.TileCount()),
        static_cast<int>(file_.CompositeW()),
        static_cast<int>(file_.CompositeH()),
        file_.path.filename().string().c_str());
    VanGui::PopStyleColor();

    VanGui::SameLine();

    // Replace tile button (only enabled when a tile is selected).
    const bool canReplace = (selectedTile_ >= 0) && !replaceBusy_;
    if (!canReplace) VanGui::BeginDisabled();
    if (VanGui::Button("Replace Tile\xe2\x80\xa6")) {
        const int idx = selectedTile_;
        fd.Show("Replace tile image (DDS or PNG)", FileDialog::Mode::Open,
                { "*.dds", "*.png" },
                [this, idx, &tasks](const std::filesystem::path& p) {
                    DoReplace(idx, p, tasks);
                });
    }
    if (!canReplace) VanGui::EndDisabled();
    if (selectedTile_ < 0) {
        if (VanGui::IsItemHovered(VanGuiHoveredFlags_AllowWhenDisabled))
            VanGui::SetTooltip("Click a tile in the viewer first");
    }

    VanGui::SameLine();

    // Export full map.
    if (VanGui::Button("Export PNG\xe2\x80\xa6")) {
        DoExportPNG(fd);
    }
    if (VanGui::IsItemHovered())
        VanGui::SetTooltip("Save the full composited map as a PNG file");

    VanGui::SameLine();

    // Zoom controls.
    VanGui::PushStyleColor(VanGuiCol_Text,
        VanVec4(g_text.x, g_text.y, g_text.z, 0.55f));
    VanGui::Text("Zoom:");
    VanGui::PopStyleColor();
    VanGui::SameLine();
    if (VanGui::Button("-##zoom")) zoom_ = std::max(0.25f, zoom_ - 0.25f);
    VanGui::SameLine();
    VanGui::PushStyleColor(VanGuiCol_Text,
        VanVec4(g_text.x, g_text.y, g_text.z, 0.75f));
    VanGui::Text("%.0f%%", zoom_ * 100.0f);
    VanGui::PopStyleColor();
    VanGui::SameLine();
    if (VanGui::Button("+##zoom")) zoom_ = std::min(4.0f, zoom_ + 0.25f);
    VanGui::SameLine();
    if (VanGui::Button("Fit"))     { zoom_ = 1.0f; pan_ = {0.0f, 0.0f}; }

    // Status messages.
    if (!replaceStatus_.empty()) {
        VanGui::SameLine();
        const bool isErr = replaceStatus_.rfind("Error", 0) == 0;
        VanGui::PushStyleColor(VanGuiCol_Text,
            isErr ? VanVec4(0.95f, 0.35f, 0.35f, 1.0f)
                  : VanVec4(0.45f, 0.85f, 0.45f, 1.0f));
        VanGui::TextUnformatted(replaceStatus_.c_str());
        VanGui::PopStyleColor();
    }
    if (!exportStatus_.empty()) {
        VanGui::SameLine();
        VanGui::PushStyleColor(VanGuiCol_Text,
            VanVec4(0.45f, 0.85f, 0.45f, 1.0f));
        VanGui::TextUnformatted(exportStatus_.c_str());
        VanGui::PopStyleColor();
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Zoomable, pannable atlas viewer
// ─────────────────────────────────────────────────────────────────────────────

void MinimapPanel::DrawViewer(float viewW, float viewH) {
    if (!atlasTexture_) return;

    // Invisible child window to capture mouse input within the viewer rect.
    (void)VanGui::BeginChild("##MinimapViewer",
                      VanVec2(viewW, viewH), false,
                      VanGuiWindowFlags_NoScrollbar |
                      VanGuiWindowFlags_NoScrollWithMouse);

    const VanVec2 canvasMin = VanGui::GetCursorScreenPos();
    const VanVec2 canvasMax = VanVec2(canvasMin.x + viewW, canvasMin.y + viewH);
    VanDrawList*  dl        = VanGui::GetWindowDrawList();

    // Dark canvas background.
    dl->AddRectFilled(canvasMin, canvasMax,
                      VanGui::ColorConvertFloat4ToU32(
                          VanVec4(g_bg0.x, g_bg0.y, g_bg0.z, 1.0f)));
    dl->PushClipRect(canvasMin, canvasMax, true);

    // Compute displayed image size and top-left position.
    const float fitScale = std::min(viewW / static_cast<float>(atlasW_),
                                    viewH / static_cast<float>(atlasH_));
    const float scale    = fitScale * zoom_;
    const float imgW     = atlasW_ * scale;
    const float imgH     = atlasH_ * scale;

    // Centre when smaller than the viewport; allow panning when larger.
    const float centreX  = canvasMin.x + (viewW - imgW) * 0.5f + pan_.x;
    const float centreY  = canvasMin.y + (viewH - imgH) * 0.5f + pan_.y;

    const VanVec2 imgMin(centreX, centreY);
    const VanVec2 imgMax(centreX + imgW, centreY + imgH);

    // Draw atlas texture.
    dl->AddImage((VanTextureID)(uintptr_t)(atlasTexture_),
                 imgMin, imgMax);

    // Draw tile grid overlay.
    const float tileW = kTileWidth  * scale;
    const float tileH = kTileHeight * scale;

    const VanU32 gridCol     = VAN_COL32(255, 255, 255, 18);
    const VanU32 hoverCol    = VAN_COL32(255, 200,  80, 140);
    const VanU32 selectCol   = VanGui::ColorConvertFloat4ToU32(
        VanVec4(g_accent.x, g_accent.y, g_accent.z, 0.85f));

    for (int row = 0; row <= gridRows_; ++row) {
        const float y = imgMin.y + row * tileH;
        dl->AddLine(VanVec2(imgMin.x, y), VanVec2(imgMax.x, y), gridCol);
    }
    for (int col = 0; col <= gridCols_; ++col) {
        const float x = imgMin.x + col * tileW;
        dl->AddLine(VanVec2(x, imgMin.y), VanVec2(x, imgMax.y), gridCol);
    }

    // Hovered / selected tile highlights.
    const VanVec2 mouse = VanGui::GetMousePos();
    hoveredTile_ = -1;

    if (mouse.x >= canvasMin.x && mouse.x < canvasMax.x &&
        mouse.y >= canvasMin.y && mouse.y < canvasMax.y) {
        hoveredTile_ = HitTestTile(mouse, imgMin, scale);
    }

    auto drawTileHighlight = [&](int idx, VanU32 col, float thickness) {
        if (idx < 0 || idx >= static_cast<int>(file_.tiles.size())) return;
        const int c = idx % gridCols_, r = idx / gridCols_;
        const VanVec2 tmin(imgMin.x + c * tileW, imgMin.y + r * tileH);
        const VanVec2 tmax(tmin.x + tileW,        tmin.y + tileH);
        dl->AddRect(tmin, tmax, col, 0.0f, 0, thickness);
    };

    if (hoveredTile_ >= 0 && hoveredTile_ != selectedTile_)
        drawTileHighlight(hoveredTile_, hoverCol, 1.5f);
    if (selectedTile_ >= 0)
        drawTileHighlight(selectedTile_, selectCol, 2.0f);

    dl->PopClipRect();

    // Handle mouse input.
    (void)VanGui::InvisibleButton("##canvas", VanVec2(viewW, viewH));

    if (VanGui::IsItemClicked(VanGuiMouseButton_Left) && hoveredTile_ >= 0)
        selectedTile_ = hoveredTile_;

    // Middle-mouse or right-mouse pan.
    if (VanGui::IsItemActive() &&
        (VanGui::IsMouseDown(VanGuiMouseButton_Middle) ||
         VanGui::IsMouseDown(VanGuiMouseButton_Right))) {
        if (!panning_) {
            panning_   = true;
            panStart_  = VanGui::GetMousePos();
            panOrigin_ = pan_;
        }
        const VanVec2 delta(VanGui::GetMousePos().x - panStart_.x,
                           VanGui::GetMousePos().y - panStart_.y);
        pan_ = VanVec2(panOrigin_.x + delta.x, panOrigin_.y + delta.y);
    } else {
        panning_ = false;
    }

    // Scroll wheel zoom.
    const float wheel = VanGui::GetIO().MouseWheel;
    if (VanGui::IsItemHovered() && wheel != 0.0f) {
        zoom_ = std::clamp(zoom_ + wheel * 0.1f, 0.25f, 4.0f);
    }

    VanGui::EndChild();
}

// ─────────────────────────────────────────────────────────────────────────────
// Tile info bar
// ─────────────────────────────────────────────────────────────────────────────

void MinimapPanel::DrawTileInfo() {
    VanGui::Dummy(VanVec2(0.0f, 4.0f));

    VanDrawList* dl  = VanGui::GetWindowDrawList();
    VanVec2 pos      = VanGui::GetCursorScreenPos();
    const float w   = VanGui::GetContentRegionAvail().x;
    dl->AddLine(pos, VanVec2(pos.x + w, pos.y),
                VanGui::ColorConvertFloat4ToU32(
                    VanVec4(g_border.x, g_border.y, g_border.z, 0.4f)));
    VanGui::Dummy(VanVec2(0.0f, 6.0f));

    const int idx = (selectedTile_ >= 0) ? selectedTile_
                  : (hoveredTile_  >= 0) ? hoveredTile_
                  : -1;

    if (idx < 0 || idx >= static_cast<int>(file_.tiles.size())) {
        VanGui::PushStyleColor(VanGuiCol_Text,
            VanVec4(g_text.x, g_text.y, g_text.z, 0.35f));
        VanGui::TextUnformatted("Hover or click a tile to inspect it");
        VanGui::PopStyleColor();
        return;
    }

    const MinimapTile& t = file_.tiles[idx];
    const int col = idx % gridCols_, row = idx / gridCols_;

    // Thumbnail from per-tile GL texture.
    if (idx < static_cast<int>(tileTextures_.size()) && tileTextures_[idx]) {
        VanGui::Image((VanTextureID)(uintptr_t)(tileTextures_[idx]),
                     VanVec2(48.0f, 48.0f));
        VanGui::SameLine(0.0f, 12.0f);
    }

    VanGui::BeginGroup();
    VanGui::PushStyleColor(VanGuiCol_Text, g_accent);
    VanGui::TextUnformatted(t.name.c_str());
    VanGui::PopStyleColor();

    VanGui::PushStyleColor(VanGuiCol_Text,
        VanVec4(g_text.x, g_text.y, g_text.z, 0.65f));
    VanGui::Text("Tile %d  ·  Grid (%d, %d)  ·  %ux%u px  ·  DXT3  ·  hash 0x%08X",
        idx, col, row, kTileWidth, kTileHeight, t.nameHash);
    VanGui::Text("File offset 0x%08llX  ·  Compressed chunk %u B",
        static_cast<unsigned long long>(t.fileChunkOffset), t.fileChunkSize);
    VanGui::PopStyleColor();
    VanGui::EndGroup();
}

// ─────────────────────────────────────────────────────────────────────────────
// Hit test
// ─────────────────────────────────────────────────────────────────────────────

int MinimapPanel::HitTestTile(VanVec2 mouse, VanVec2 imageOrigin, float scale) const {
    const float tileW = kTileWidth  * scale;
    const float tileH = kTileHeight * scale;

    const float relX = mouse.x - imageOrigin.x;
    const float relY = mouse.y - imageOrigin.y;

    if (relX < 0.0f || relY < 0.0f) return -1;

    const int col = static_cast<int>(relX / tileW);
    const int row = static_cast<int>(relY / tileH);

    if (col >= gridCols_ || row >= gridRows_) return -1;

    const int idx = row * gridCols_ + col;
    if (idx >= static_cast<int>(file_.tiles.size())) return -1;
    return idx;
}

// ─────────────────────────────────────────────────────────────────────────────
// Tile replacement
// ─────────────────────────────────────────────────────────────────────────────

void MinimapPanel::DoReplace(int tileIdx,
                              const std::filesystem::path& srcPath,
                              TaskQueue& tasks) {
    replaceBusy_   = true;
    replaceStatus_ = "Replacing\xe2\x80\xa6";

    struct ReplaceState {
        Result<std::vector<uint8_t>> prep;
        TileReplaceResult            replace;
    };
    auto state = std::make_shared<ReplaceState>();

    tasks.Submit("Replacing tile", [this, tileIdx, srcPath, state](ProgressState& p) {
        p.fraction = -1.f;
        state->prep = MinimapRepacker::PrepareSourceImage(srcPath);
        if (!state->prep) return;
        state->replace = MinimapRepacker::ReplaceTile(
            file_, static_cast<uint32_t>(tileIdx), state->prep.value, backup_);
    }, [this, tileIdx, state] {
        replaceBusy_ = false;
        if (!state->prep) {
            replaceStatus_ = "Error: " + state->prep.error;
            return;
        }
        if (!state->replace.ok) {
            replaceStatus_ = "Error: " + state->replace.error;
            return;
        }
        replaceStatus_ = "Tile replaced.";
        if (tileIdx < static_cast<int>(tileTextures_.size()) &&
            tileTextures_[tileIdx]) {
            std::vector<uint8_t> rgba = DecodeDXT3(file_.tiles[tileIdx]);
            glBindTexture(GL_TEXTURE_2D, tileTextures_[tileIdx]);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
                            static_cast<GLsizei>(kTileWidth),
                            static_cast<GLsizei>(kTileHeight),
                            GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
            glBindTexture(GL_TEXTURE_2D, 0);
        }
        RebuildAtlas();
    });
}

// ─────────────────────────────────────────────────────────────────────────────
// PNG export
// ─────────────────────────────────────────────────────────────────────────────

void MinimapPanel::DoExportPNG(FileDialog& fd) {
    fd.Show("Export minimap as PNG", FileDialog::Mode::Save,
            { "*.png" },
            [this](const std::filesystem::path& dst) {
                // Rebuild RGBA atlas from current tile data.
                std::vector<uint8_t> atlasBuf(atlasW_ * atlasH_ * 4, 0);
                const int n = static_cast<int>(file_.tiles.size());
                for (int i = 0; i < n; ++i) {
                    std::vector<uint8_t> rgba = DecodeDXT3(file_.tiles[i]);
                    const int col = i % gridCols_, row = i / gridCols_;
                    const int dstX = col * static_cast<int>(kTileWidth);
                    const int dstY = row * static_cast<int>(kTileHeight);
                    for (int y = 0; y < static_cast<int>(kTileHeight); ++y) {
                        const uint8_t* src = rgba.data() + y * kTileWidth * 4;
                        uint8_t*       dp  = atlasBuf.data() +
                                             ((dstY + y) * atlasW_ + dstX) * 4;
                        std::memcpy(dp, src, kTileWidth * 4);
                    }
                }

                const int ok = stbi_write_png(
                    dst.string().c_str(),
                    atlasW_, atlasH_, 4,
                    atlasBuf.data(),
                    atlasW_ * 4);

                exportStatus_ = ok ? "Exported." : "Error: PNG write failed.";
            });
}

} // namespace nfsmw