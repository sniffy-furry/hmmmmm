#pragma once
// ─── ui/CarTexturePanel.h ─────────────────────────────────────────────────────
// DDS texture browse / export / replace for the car body BUN.
// Mirrors TexturePackPanel almost verbatim; the only difference is it sources
// its TPKBlocks from CarContext::bodyTPKs rather than a standalone file.
// ─────────────────────────────────────────────────────────────────────────────
#include "Common.h"
#include "formats/TPKBlock.h"
#include "patch/BunRepacker.h"
#include "patch/BackupManager.h"
#include "ui/FileDialog.h"
#include "async/TaskQueue.h"
#include <unordered_map>
#include <string>
#include <cstdint>

namespace nfsmw {

struct CarContext;

class CarTexturePanel {
public:
    CarTexturePanel();
    ~CarTexturePanel();

    /// Release GL resources (textures/thumbnails) while the GL context is
    /// still valid. Must be called before the context is torn down at app
    /// shutdown -- the destructor runs too late for that (after
    /// glfwTerminate()).
    void Shutdown();

    CarTexturePanel(const CarTexturePanel&)            = delete;
    CarTexturePanel& operator=(const CarTexturePanel&) = delete;

    /// Called when a new car is selected — resets selection and thumb cache.
    void OnCarLoaded(const CarContext& ctx);

    void Draw(CarContext& ctx, TaskQueue& tasks);

private:
    // Selection
    int selTPK_     = -1;
    int selTexture_ = -1;

    // GL preview
    uint32_t previewGL_ = 0;
    int      previewW_  = 0;
    int      previewH_  = 0;

    std::unordered_map<uint32_t, uint32_t> thumbCache_;

    // File dialogs
    FileDialog exportDialog_;
    FileDialog replaceDialog_;

    // Patch state
    BunRepacker   repacker_;
    BackupManager backup_;
    std::string   writeStatus_;

    // Repack confirmation
    bool showRepackConfirm_ = false;
    int  confirmTPK_        = -1;
    int  confirmTex_        = -1;
    std::filesystem::path pendingReplaceDDS_;

    void SelectTexture(int tpkIdx, int texIdx, const CarContext& ctx);
    void UploadGLTexture(const Texture& tex);
    void DestroyGLTexture();
    uint32_t GetOrCreateThumb(const Texture& tex);
    void ClearThumbCache();
    void DrawMetadataCard(const Texture& tex, CarContext& ctx, TaskQueue& tasks);
    void DrawRepackConfirmDialog(CarContext& ctx, TaskQueue& tasks);
    void DoReplace(int tpkIdx, int texIdx, const std::filesystem::path& ddsPath,
                   CarContext& ctx, TaskQueue& tasks);
};

} // namespace nfsmw
