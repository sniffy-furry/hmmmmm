#pragma once
// ─── ui/CarPanel.h ────────────────────────────────────────────────────────────
// Top-level car section: owns CarContext and lays out the browser/detail split.
//
// Layout:
//   Left pane  (kBrowserWidth) — CarBrowserPanel: lists cars found in CARS/
//   Right pane                 — tab bar: Viewer | Mesh | Textures | Vinyls |
//                                         Performance | Pursuit | Engine Audio
//
// CarContext is owned here and passed (as const*) to sub-panels for reading.
// Only write-capable panels (Mesh, Textures, Vinyls, Perf, Pursuit) mutate
// files on disk; they receive a non-const pointer only at the point of save.
// ─────────────────────────────────────────────────────────────────────────────
#include "Common.h"
#include "formats/StreamBundle.h"
#include "formats/TPKBlock.h"
#include "formats/CarInfo.h"
#include "formats/CarSlot.h"
#include "formats/GINFile.h"
#include "ui/CarBrowserPanel.h"
#include "ui/CarViewerPanel.h"
#include "ui/CarMeshPanel.h"
#include "ui/CarTexturePanel.h"
#include "ui/CarVinylPanel.h"
#include "ui/CarPerfPanel.h"
#include "ui/CarPursuitPanel.h"
#include "ui/CarEngineAudioPanel.h"
#include "async/TaskQueue.h"
#include <atomic>
#include <filesystem>
#include <string>

namespace nfsmw {

// ─────────────────────────────────────────────────────────────────────────────
// CarContext — shared loaded state for the selected car
//
// CarPanel owns one instance and refreshes it when a new car is selected in
// CarBrowserPanel.  All other sub-panels hold a const CarContext* and read
// from it.  The loaded flag guards against panels drawing before a car is
// selected.
// ─────────────────────────────────────────────────────────────────────────────
struct CarContext {
    std::string           id;          ///< three-letter identifier, e.g. "M3G"
    std::filesystem::path carDir;      ///< CARS/<id>/

    // Geometry (populated by Phase A)
    StreamSection  bodySection;        ///< parsed from <id>.BUN
    StreamSection  wheelSection;       ///< parsed from <id>_WHEEL.BUN

    // Textures (populated by Phase B)
    std::vector<TPKBlock> bodyTPKs;    ///< TPK blocks from <id>.BUN
    TPKBlock              skinsTPK;    ///< from GLOBAL/CARSKINS.BUN
    std::vector<TPKBlock> vinylTPKs;   ///< real vinyl decal textures from VINYLS.BIN

    // Performance / pursuit (populated by Phase C / D)
    CarInfo    perfData;
    CarSlot    slotData;
    CarPursuit pursuitData;

    // Audio (populated by Phase E)
    GINFile    engineGIN;
    std::filesystem::path    engineDir;        ///< SOUND/ENGINE/ (where the GINs live)
    std::vector<std::string> engineGINs;       ///< all *.gin filenames in engineDir
    std::string              engineGINName;    ///< chosen GIN filename for this car

    // State flags — set to true when the corresponding async load succeeds
    bool loaded        = false;
    bool geometryReady = false;
    bool texturesReady = false;
    bool perfReady     = false;
    bool pursuitReady  = false;
    bool audioReady    = false;
    bool vinylReady    = false;
};

// ─────────────────────────────────────────────────────────────────────────────
// CarPanel
// ─────────────────────────────────────────────────────────────────────────────
class CarPanel {
public:
    CarPanel();
    ~CarPanel() = default;

    CarPanel(const CarPanel&)            = delete;
    CarPanel& operator=(const CarPanel&) = delete;

    /// Call once after the GL context is current (forwards to sub-panels).
    Result<void> Init();

    /// Release GL resources.
    void Shutdown();

    /// Open the CARS/ root directory.  Triggers async scan.
    void Open(const std::filesystem::path& carsRoot, TaskQueue& tasks);

    /// Draw the full panel into the content area [x,y,w,h].
    void Draw(TaskQueue& tasks);

    bool HasCars() const;

private:
    // ── Sub-panels ──────────────────────────────────────────────────────────
    CarBrowserPanel     browser_;
    CarViewerPanel      viewer_;
    CarMeshPanel        meshPanel_;
    CarTexturePanel     texPanel_;
    CarVinylPanel       vinylPanel_;
    CarPerfPanel        perfPanel_;
    CarPursuitPanel     pursuitPanel_;
    CarEngineAudioPanel audioPanel_;

    // ── Shared state ────────────────────────────────────────────────────────
    CarContext     ctx_;
    std::filesystem::path carsRoot_;

    // Guards against two overlapping LoadCar() async tasks writing into the
    // same CarContext from different threads (e.g. the user clicking a
    // second car in the browser before the first finishes loading). While a
    // load is in flight, new requests are remembered in pendingCarId_ and
    // started once the current one's onDone callback runs (on the UI
    // thread), instead of being submitted concurrently.
    std::atomic<bool> loading_    = false;
    std::string       pendingCarId_;
    bool              hasPending_ = false;

    // Active detail tab (0=Viewer … 6=EngineAudio)
    int activeTab_ = 0;

    // Layout: browser pane width, user-draggable via VanGuiChildFlags_ResizeX
    // (persisted across frames, not across restarts — see Draw()).
    static constexpr float kBrowserWidthDefault = 200.f;
    static constexpr float kBrowserWidthMin     = 140.f;
    float browserWidth_ = kBrowserWidthDefault;

    // ── Helpers ─────────────────────────────────────────────────────────────
    /// Called by CarBrowserPanel when the user selects a different car.
    void LoadCar(const std::string& carId, TaskQueue& tasks);

    void DrawDetailTabs(TaskQueue& tasks, float w, float h);
};

} // namespace nfsmw
