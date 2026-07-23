#pragma once
// ─── ui/UIFrontendPanel.h ─────────────────────────────────────────────────────
// Phase 8 — FRONTEND/*.BUN texture browse / export / replace.
//
// Reuses the TexturePackPanel browse/export/replace pipeline verbatim;
// UIFrontendPanel is essentially a thin routing wrapper:
//   • Open() calls StreamBundleLoader::Load to extract TPKBlocks, then
//     stores them for the tree/preview.
//   • The grid thumbnail display, metadata card, DDS export, and BunRepacker
//     replace path follow the pattern established in CarTexturePanel.
//
// Multiple FRONTEND BUNs can be open simultaneously (listed in the tree).
// ─────────────────────────────────────────────────────────────────────────────
#include "Common.h"
#include "formats/TPKBlock.h"
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
#include <cstdint>

namespace nfsmw {

class UIFrontendPanel {
public:
    UIFrontendPanel();
    ~UIFrontendPanel();

    /// Release GL resources (textures/thumbnails) while the GL context is
    /// still valid. Must be called before the context is torn down at app
    /// shutdown -- the destructor runs too late for that (after
    /// glfwTerminate()).
    void Shutdown();

    UIFrontendPanel(const UIFrontendPanel&)            = delete;
    UIFrontendPanel& operator=(const UIFrontendPanel&) = delete;

    void SetFileDialog(FileDialog* fd) { fileDialog_ = fd; }

    /// Async-load `path` (a FRONTEND/*.BUN) and add it to the file list.
    void Open(const std::filesystem::path& path, TaskQueue& tasks);

    /// Draw tree (left) + preview (right) side by side.
    void Draw(TaskQueue& tasks);

    bool IsEmpty() const { return files_.empty(); }

private:
    // ── Loaded state ─────────────────────────────────────────────────────────
    struct LoadedFile {
        std::filesystem::path path;
        std::string           stem;
        std::vector<TPKBlock> tpks;
        bool                  loading = false;
    };

    std::vector<LoadedFile> files_;

    // ── Selection ─────────────────────────────────────────────────────────────
    int selFile_    = -1;
    int selTPK_     = -1;
    int selTexture_ = -1;

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
    int  confirmFile_        = -1;
    int  confirmTPK_         = -1;
    int  confirmTex_         = -1;
    std::filesystem::path pendingReplaceDDS_;

    // ── UI helpers ────────────────────────────────────────────────────────────
    FilterBox      filter_;
    FileDialog*    fileDialog_ = nullptr;

    // ── Private methods ───────────────────────────────────────────────────────
    void SelectTexture(int fileIdx, int tpkIdx, int texIdx);
    void UploadGLTexture(const Texture& tex);
    void DestroyGLTexture();
    uint32_t GetOrCreateThumb(const Texture& tex);
    void ClearThumbCache();

    void DrawTree(TaskQueue& tasks);
    void DrawPreview(TaskQueue& tasks);
    void DrawMetadataCard(const Texture& tex, int fileIdx, int tpkIdx, int texIdx,
                          TaskQueue& tasks);
    void DrawRepackConfirmDialog(TaskQueue& tasks);

    void DoReplace(int fileIdx, int tpkIdx, int texIdx,
                   const std::filesystem::path& ddsPath, TaskQueue& tasks);
    void DoExport(int fileIdx, int tpkIdx, int texIdx,
                  const std::filesystem::path& destPath);
};

} // namespace nfsmw
