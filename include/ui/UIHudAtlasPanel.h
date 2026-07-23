#pragma once
// ─── ui/UIHudAtlasPanel.h ─────────────────────────────────────────────────────
// Phase 8 — GLOBALHUD.BUN texture atlas browser + HUD layout element table.
//
// Layout:
//   Left column (kAtlasTreeW)  — texture grid: thumbnails of every texture
//                                in the HUD atlas TPK, with a filter box.
//   Right column               — tabs:
//     "Texture"   — metadata card + Export DDS / Replace DDS toolbar
//     "Elements"  — read-only HUDElement table (name, anchor XY, size WH,
//                   rotation, flags). Selected element highlighted in the
//                   texture if hash resolves to an atlas entry.
//
// Both Replace DDS and element editing call the same BunRepacker/HUDLayoutParser
// paths as the other texture panels. Element editing returns an error toast in
// Phase 8v1 (HUDLayoutParser::Save is disabled until chunk ID confirmed).
// ─────────────────────────────────────────────────────────────────────────────
#include "Common.h"
#include "formats/TPKBlock.h"
#include "formats/HUDLayout.h"
#include "formats/StreamBundle.h"
#include "patch/BunRepacker.h"
#include "patch/BackupManager.h"
#include "ui/FileDialog.h"
#include "ui/FilterBox.h"
#include "async/TaskQueue.h"
#include <filesystem>
#include <vector>
#include <string>
#include <unordered_map>

namespace nfsmw {

class UIHudAtlasPanel {
public:
    UIHudAtlasPanel();
    ~UIHudAtlasPanel();

    /// Release GL resources (textures/thumbnails) while the GL context is
    /// still valid. Must be called before the context is torn down at app
    /// shutdown -- the destructor runs too late for that (after
    /// glfwTerminate()).
    void Shutdown();

    UIHudAtlasPanel(const UIHudAtlasPanel&)            = delete;
    UIHudAtlasPanel& operator=(const UIHudAtlasPanel&) = delete;

    void SetFileDialog(FileDialog* fd) { fileDialog_ = fd; }

    /// Async-load GLOBALHUD.BUN.  Extracts TPKBlocks via StreamBundleLoader
    /// and runs HUDLayoutParser on the same buffer.
    void Open(const std::filesystem::path& path, TaskQueue& tasks);

    void Draw(TaskQueue& tasks);

    bool IsLoaded() const { return loaded_; }

private:
    // ── Loaded state ─────────────────────────────────────────────────────────
    std::filesystem::path   sourcePath_;
    std::vector<TPKBlock>   atlasTPKs_;
    HUDLayout               hudLayout_;
    bool                    loaded_   = false;
    bool                    loading_  = false;
    std::string             lastError_;

    // ── Selection ─────────────────────────────────────────────────────────────
    int selTPK_     = -1;
    int selTex_     = -1;
    int selElement_ = -1;
    int detailTab_  = 0;    // 0=Texture, 1=Elements

    // ── GL preview ────────────────────────────────────────────────────────────
    uint32_t previewGL_ = 0;
    int      previewW_  = 0;
    int      previewH_  = 0;

    std::unordered_map<uint32_t, uint32_t> thumbCache_;

    // ── Patch state ───────────────────────────────────────────────────────────
    BunRepacker   repacker_;
    BackupManager backup_;
    std::string   writeStatus_;

    bool showRepackConfirm_  = false;
    int  confirmTPK_         = -1;
    int  confirmTex_         = -1;
    std::filesystem::path pendingReplaceDDS_;

    // ── UI helpers ────────────────────────────────────────────────────────────
    FilterBox      filter_;
    FileDialog*    fileDialog_ = nullptr;

    static constexpr float kAtlasTreeW = 200.f;

    // ── Private methods ───────────────────────────────────────────────────────
    void SelectTexture(int tpkIdx, int texIdx);
    void UploadGLTexture(const Texture& tex);
    void DestroyGLTexture();
    uint32_t GetOrCreateThumb(const Texture& tex);
    void ClearThumbCache();

    void DrawAtlasTree();
    void DrawDetailPane(TaskQueue& tasks);
    void DrawTextureTab(TaskQueue& tasks);
    void DrawElementsTab();
    void DrawRepackConfirmDialog(TaskQueue& tasks);

    void DoReplace(int tpkIdx, int texIdx,
                   const std::filesystem::path& ddsPath, TaskQueue& tasks);
    void DoExport(int tpkIdx, int texIdx,
                  const std::filesystem::path& destPath);
};

} // namespace nfsmw
