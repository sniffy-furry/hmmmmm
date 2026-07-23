#pragma once
// ─── ui/UIFontPanel.h ─────────────────────────────────────────────────────────
// Phase 8 — GLOBALB.LZC font sheet browser.
//
// Layout:
//   Left column (kSheetListW)  — list of FontSheets found in the LZC archive
//   Right column               — tabs:
//     "Atlas"   — thumbnail of the selected sheet's atlas texture + metadata
//                 (name, dimensions, format, mip count) + "Export Atlas DDS…"
//     "Glyphs"  — read-only table of GlyphEntry rows (codepoint, U0/V0/U1/V1,
//                 advance). If glyphsDecoded=false a note explains the raw bytes
//                 are preserved but the table could not be decoded.
//
// Atlas export uses DDSCodec::Export exactly as in the other texture panels.
// Glyph UV editing is deferred to a future patch.
// ─────────────────────────────────────────────────────────────────────────────
#include "Common.h"
#include "formats/FontSheet.h"
#include "ui/FileDialog.h"
#include "ui/FilterBox.h"
#include "async/TaskQueue.h"
#include <filesystem>
#include <vector>
#include <string>
#include <unordered_map>

namespace nfsmw {

class UIFontPanel {
public:
    UIFontPanel();
    ~UIFontPanel();

    /// Release GL resources (textures/thumbnails) while the GL context is
    /// still valid. Must be called before the context is torn down at app
    /// shutdown -- the destructor runs too late for that (after
    /// glfwTerminate()).
    void Shutdown();

    UIFontPanel(const UIFontPanel&)            = delete;
    UIFontPanel& operator=(const UIFontPanel&) = delete;

    void SetFileDialog(FileDialog* fd) { fileDialog_ = fd; }

    /// Async-load and decompress `lzcPath`, then parse all FontSheets.
    void Open(const std::filesystem::path& lzcPath, TaskQueue& tasks);

    void Draw(TaskQueue& tasks);

    bool IsLoaded() const { return loaded_; }

private:
    // ── Loaded state ─────────────────────────────────────────────────────────
    std::filesystem::path        sourcePath_;
    std::vector<FontSheet>       sheets_;
    bool                         loaded_  = false;
    bool                         loading_ = false;
    std::string                  lastError_;

    // ── Selection ─────────────────────────────────────────────────────────────
    int selSheet_  = -1;
    int detailTab_ = 0;   // 0=Atlas, 1=Glyphs

    // ── GL atlas preview ──────────────────────────────────────────────────────
    uint32_t previewGL_ = 0;
    int      previewW_  = 0;
    int      previewH_  = 0;

    std::unordered_map<uint32_t, uint32_t> thumbCache_;

    // ── UI helpers ────────────────────────────────────────────────────────────
    FilterBox      glyphFilter_;
    FileDialog*    fileDialog_ = nullptr;

    static constexpr float kSheetListW = 180.f;

    // ── Private methods ───────────────────────────────────────────────────────
    void SelectSheet(int idx);
    void UploadGLTexture(const Texture& tex);
    void DestroyGLTexture();
    uint32_t GetOrCreateThumb(const Texture& tex);
    void ClearThumbCache();

    void DrawSheetList();
    void DrawDetailPane(TaskQueue& tasks);
    void DrawAtlasTab(TaskQueue& tasks);
    void DrawGlyphsTab();

    void DoExportAtlas(int sheetIdx, const std::filesystem::path& destPath);
};

} // namespace nfsmw
