#pragma once
// ─── ui/CarPursuitPanel.h ─────────────────────────────────────────────────────
// Pursuit aggression / heat table read-only viewer for CARINFO.BIN.
//
// Displays a 5-row VanGui table (one row per heat level). Shown only once
// real values can be read from the game (ctx.pursuitReady) — no editable
// placeholder fields, no JSON import/export.
// ─────────────────────────────────────────────────────────────────────────────
#include "Common.h"
#include "formats/CarInfo.h"
#include "patch/BackupManager.h"
#include "async/TaskQueue.h"
#include <string>

namespace nfsmw {

struct CarContext;

class CarPursuitPanel {
public:
    /// Called when a new car is loaded — clones pursuit data into edit copy.
    void OnCarLoaded(const CarContext& ctx);

    void Draw(CarContext& ctx, TaskQueue& tasks);

private:
    CarPursuit  edit_;
    bool        dirty_    = false;
    bool        stubMode_ = true;  ///< true until CarPursuitParser::Load() implemented

    BackupManager backup_;
    std::string   writeStatus_;

    void DrawBaseFields();
    void DrawHeatTable();
    void DrawToolbar(CarContext& ctx, TaskQueue& tasks);
    void DoSave(CarContext& ctx);
};

} // namespace nfsmw
