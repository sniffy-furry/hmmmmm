#pragma once
// ─── ui/TexturePackPanel.h ────────────────────────────────────────────────────
// Phase 1+2 — TPK Texture Browser & Texture Replace (read + write path).
//
// Phase 1 features:
//   • Open any .BUN/.BIN, walk the chunk tree, collect every TPKBlock.
//   • Left tree:  file → TPK → texture list (name, dims, format, mip count).
//   • Centre:     selected texture rendered as an OpenGL thumbnail + metadata card.
//   • Export selected texture → .dds (DDSCodec::Export).
//   • File load + chunk walk wrapped in TaskQueue::Submit with progress bar.
//
// Phase 2 features (write path):
//   • "Replace from DDS…" per texture: same-size in-place (Algorithm A) or
//     full repack (Algorithm B) via BunRepacker, with BackupManager .bak safety.
//   • "Revert to original" per texture: replays .manifest to restore original bytes.
//   • Confirmation dialog for Algorithm B (warns about file growth).
//   • Batch replace: scan a folder for name_<hash>.dds files → patch all in one
//     TaskQueue operation with progress bar.
// ─────────────────────────────────────────────────────────────────────────────
#include "Common.h"
#include "formats/TPKBlock.h"
#include "async/TaskQueue.h"
#include "patch/BunRepacker.h"
#include "patch/BackupManager.h"
#include "ui/FileDialog.h"
#include <filesystem>
#include <vector>
#include <string>
#include <cstdint>
#include <unordered_map>
#include "ui/FilterBox.h"

namespace nfsmw {

class TexturePackPanel {
public:
    TexturePackPanel();
    ~TexturePackPanel();

    /// Release GL resources (textures/thumbnails) while the GL context is
    /// still valid. Must be called before the context is torn down at app
    /// shutdown -- the destructor runs too late for that (after
    /// glfwTerminate()).
    void Shutdown();

    TexturePackPanel(const TexturePackPanel&)            = delete;
    TexturePackPanel& operator=(const TexturePackPanel&) = delete;

    // ── Phase 1 ─────────────────────────────────────────────────────────────

    /// Draw the left-side asset tree (call inside a docked window).
    void DrawTree();

    /// Draw the centre preview + metadata card (call inside a docked window).
    void DrawPreview();

    /// Queue an async open of `path`. Progress is shown via taskQueue.
    void OpenFile(const std::filesystem::path& path, TaskQueue& taskQueue);

    bool IsLoading()  const { return loading_; }
    const std::string& LastError() const { return lastError_; }
    size_t TotalTextures() const;

    /// Must be called once per frame before DrawTree()/DrawPreview() so that
    /// Phase 2 write operations have access to the TaskQueue.
    void SetTaskQueue(TaskQueue& tq) { taskQueue_ = &tq; }

    // ── Phase 5 ──────────────────────────────────────────────────────────────

    /// True if the currently selected file has a pending .manifest (i.e.
    /// patches have been applied and can be reverted). Used to gate the
    /// global Ctrl+Z shortcut and "Revert last patch" menu item.
    bool CanRevertSelectedFile() const;

    /// Revert all patches on the currently selected file via its .manifest
    /// (Ctrl+Z global shortcut). No-op if CanRevertSelectedFile() is false.
    void RevertSelectedFile(TaskQueue& taskQueue);

    /// Export the currently selected texture to a .dds file (Ctrl+S).
    void ExportSelected(const std::filesystem::path& destPath);

    /// Returns a pointer to the currently selected Texture, or nullptr.
    const Texture* SelectedTexture() const;

    /// Remove one loaded file from the browser (the ✕ button in the tree).
    /// Lets users unload a file they're done with without restarting the app.
    void CloseFile(int fileIdx);

    /// Remove every loaded file and reset the selection.
    void CloseAll();

    /// True when at least one file is loaded.
    bool HasFiles() const { return !files_.empty(); }

private:
    // ── Loaded file state ────────────────────────────────────────────────────
    struct LoadedFile {
        std::filesystem::path    path;
        std::vector<TPKBlock>    tpks;
        bool                     compressed = false;
    };

    std::vector<LoadedFile>  files_;
    FilterBox                texFilter_;  ///< tree filter box (audit 4.2)
    std::string              lastError_;

    // Selection
    int selFile_    = -1;
    int selTPK_     = -1;
    int selTexture_ = -1;

    // Async
    bool loading_ = false;

    // Preview GL texture (full-size, for selected texture)
    uint32_t previewGLHandle_ = 0;
    int      previewW_        = 0;
    int      previewH_        = 0;

    // Per-texture thumbnail GL handles (tiny 32×32 previews, lazy-uploaded).
    // Key: nameHash.  Value: GL texture handle (0 = not yet uploaded / unsupported).
    std::unordered_map<uint32_t, uint32_t> thumbCache_;

    // File dialogs
    FileDialog exportDialog_;
    FileDialog replaceDialog_;
    FileDialog batchDialog_;

    // ── Phase 2 state ────────────────────────────────────────────────────────

    BunRepacker   repacker_;
    BackupManager backupMgr_;

    // Confirmation dialog for Algorithm B (repack).
    bool     showRepackConfirm_    = false;
    int      repackConfirmFile_    = -1;
    int      repackConfirmTPK_     = -1;
    int      repackConfirmTex_     = -1;
    std::filesystem::path pendingReplaceDDS_;

    // Status / result of last write operation.
    std::string writeStatus_;  // shown in metadata card after a replace/revert

    // ── Helpers ─────────────────────────────────────────────────────────────
    void SelectTexture(int fileIdx, int tpkIdx, int texIdx);
    void UploadGLTexture(const Texture& tex);
    void DestroyGLTexture();
    /// Upload (or return cached) a 32×32 thumbnail GL handle for `tex`.
    /// Returns 0 for unsupported formats (PAL8, Unknown) or empty data.
    uint32_t GetOrCreateThumb(const Texture& tex);
    /// Free all thumbnail GL textures (call when files_ changes).
    void ClearThumbCache();
    void DrawMetadataCard(const Texture& tex, TaskQueue& taskQueue);
    LoadedFile*    SelectedFile();

    // Phase 2 helpers
    void DrawRepackConfirmDialog(TaskQueue& taskQueue);
    void DoReplace(int fileIdx, int tpkIdx, int texIdx,
                   const std::filesystem::path& ddsPath,
                   TaskQueue& taskQueue);
    // Second parameter was historically named tpkIdx/texIdx but is
    // unused — DoRevert always operates at file level (audit 3.1/3.2).
    void DoRevert(int fileIdx, TaskQueue& taskQueue);
    void DoBatchReplace(const std::filesystem::path& folderPath,
                        int fileIdx, TaskQueue& taskQueue);
    void RefreshFile(int fileIdx, TaskQueue& taskQueue);

    // Store the taskQueue reference between DrawTree/DrawPreview calls
    // (set at the top of DrawTree, valid for one frame).
    TaskQueue* taskQueue_ = nullptr;

    // Cached flat list for stream-bundle files (avoids O(n) rebuild every frame).
    struct FlatEntry { int ti; int xi; const Texture* tex; };
    std::vector<FlatEntry>           cachedFlat_;
    int                              cachedFlatFileIdx_ = -1;  // file index flat_ was built for
    size_t                           cachedFlatVersion_ = 0;   // bumped on refresh to invalidate
    size_t                           flatVersion_       = 0;   // current version counter
};

} // namespace nfsmw