#pragma once
// ─── ui/CarVinylPanel.h ───────────────────────────────────────────────────────
// Vinyl decal browser for the car's real VINYLS.BIN texture pack.
//
// VINYLS.BIN is a standard TPK of the per-car vinyl/decal textures. This panel
// browses those real textures with thumbnails, shows metadata, and can export
// any decal to DDS. (The earlier CARSLOT layer-stack concept did not match
// stock game files and has been retired.)
// ─────────────────────────────────────────────────────────────────────────────
#include "Common.h"
#include "formats/TPKBlock.h"
#include "ui/FileDialog.h"
#include "async/TaskQueue.h"
#include <unordered_map>
#include <string>
#include <cstdint>

namespace nfsmw {

struct CarContext;

class CarVinylPanel {
public:
    CarVinylPanel();
    ~CarVinylPanel();

    /// Release GL resources (thumbnails/preview) while the GL context is still
    /// valid — the destructor runs after glfwTerminate(), too late for GL.
    void Shutdown();

    CarVinylPanel(const CarVinylPanel&)            = delete;
    CarVinylPanel& operator=(const CarVinylPanel&) = delete;

    /// Called when a new car is loaded — resets selection and thumb cache.
    void OnCarLoaded(const CarContext& ctx);

    void Draw(CarContext& ctx, TaskQueue& tasks);

private:
    int selTPK_ = -1;
    int selTex_ = -1;

    uint32_t previewGL_ = 0;
    int      previewW_  = 0;
    int      previewH_  = 0;

    std::unordered_map<uint32_t, uint32_t> thumbCache_;  ///< keyed by nameHash
    FileDialog  exportDialog_;
    std::string status_;

    void Select(int tpkIdx, int texIdx, const CarContext& ctx);
    uint32_t GetOrCreateThumb(const Texture& tex);
    void ClearThumbCache();
    void DestroyPreview();
};

} // namespace nfsmw
