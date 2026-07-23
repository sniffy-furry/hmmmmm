#pragma once
// ─── ui/CarPerfPanel.h ────────────────────────────────────────────────────────
// Performance & handling read-only viewer for CARINFO.BIN.
//
// Organised into collapsible VanGui sections: Engine | Gearbox | Handling |
// Suspension | Tyres. Shown only once real values can be read from the game
// (ctx.perfReady) — no editable placeholder fields, no JSON import/export.
// ─────────────────────────────────────────────────────────────────────────────
#include "Common.h"
#include "formats/CarInfo.h"
#include "formats/VaultDecoder.h"
#include "core/VaultSchema.h"
#include "patch/BackupManager.h"
#include "ui/FileDialog.h"
#include "async/TaskQueue.h"
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

namespace nfsmw {

struct CarContext;

class CarPerfPanel {
public:
    /// Called when a new car is loaded — pulls perfData into edit copy.
    void OnCarLoaded(const CarContext& ctx);

    void Draw(CarContext& ctx, TaskQueue& tasks);

private:
    CarInfo     edit_;           ///< working copy of perfData
    bool        dirty_    = false;
    bool        stubMode_ = true;  ///< true until CarInfoParser::Load() is implemented

    std::string writeStatus_;
    BackupManager backup_;

    void DrawEngineSection();
    void DrawGearboxSection();
    void DrawHandlingSection();
    void DrawSuspensionSection(CarContext& ctx);   ///< also pokes viewer wheel positions
    void DrawTyreSection();
    void DrawToolbar(CarContext& ctx, TaskQueue& tasks);
    void DoSave(CarContext& ctx);

    // ── Attribute-vault editor (real per-car tuning from attributes.bin) ──────
    FileDialog                          fileDlg_;
    std::filesystem::path               vaultPath_;
    std::vector<VaultDecodedRecord>     vaultRecs_;
    std::vector<const VaultClassSchema*> recClass_;   ///< best class per record
    std::unordered_map<uint64_t, float> edits_;       ///< valueOffset -> pending value
    std::string                         vaultStatus_;
    char                                vaultSearch_[64] = {0};
    std::filesystem::path               autoTriedFor_;  ///< carDir we auto-probed

    void DrawVaultEditor(CarContext& ctx);
    void LoadVaultFile(const std::filesystem::path& p);
    void SaveVaultEdits();
};

} // namespace nfsmw
