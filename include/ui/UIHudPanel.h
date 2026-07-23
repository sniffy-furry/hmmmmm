#pragma once
// ─── ui/UIHudPanel.h ──────────────────────────────────────────────────────────
// Phase 8 — Top-level UI / HUD panel.
//
// Hosts three sub-panels in a tab bar:
//   Frontend   — browse / export / replace textures in FRONTEND/*.BUN
//   HUD Atlas  — browse GLOBALHUD.BUN atlas; display HUDLayout element table
//   Fonts      — browse GLOBALB.LZC font sheets; display glyph table
//
// Opening a file:
//   • Any FRONTEND/*.BUN   → UIFrontendPanel::Open()
//   • GLOBAL/GLOBALHUD.BUN → UIHudAtlasPanel::Open()
//   • GLOBAL/GLOBALB.LZC   → UIFontPanel::Open()
//   UIHudPanel::Open() inspects the filename and routes accordingly,
//   switching the active tab to match the loaded file.
//
// AppShell integration:
//   AppShell owns one UIHudPanel* and calls:
//     Draw(tasks_)         — once per frame inside DrawUIHudContent()
//     Open(path, tasks_)   — from the "Open UI/HUD…" menu item and drop handler
// ─────────────────────────────────────────────────────────────────────────────
#include "Common.h"
#include "ui/UIFrontendPanel.h"
#include "ui/UIHudAtlasPanel.h"
#include "ui/UIFontPanel.h"
#include "async/TaskQueue.h"
#include <filesystem>
#include <string>

namespace nfsmw {

class UIHudPanel {
public:
    UIHudPanel();
    ~UIHudPanel() = default;

    UIHudPanel(const UIHudPanel&)            = delete;
    UIHudPanel& operator=(const UIHudPanel&) = delete;

    /// Call once after GL context is current (if sub-panels need GL init).
    void Init(FileDialog* fileDialog);

    /// Release the sub-panels' GL resources. Must be called before the GL
    /// context is torn down at app shutdown.
    void Shutdown();

    /// Route `path` to the appropriate sub-panel and switch to its tab.
    /// Accepted:
    ///   *.BUN in FRONTEND/      → UIFrontendPanel
    ///   GLOBALHUD.BUN           → UIHudAtlasPanel
    ///   *.LZC                   → UIFontPanel
    void Open(const std::filesystem::path& path, TaskQueue& tasks);

    /// Draw the full panel into the current content region.
    void Draw(TaskQueue& tasks);

private:
    UIFrontendPanel frontendPanel_;
    UIHudAtlasPanel hudAtlasPanel_;
    UIFontPanel     fontPanel_;

    enum class Tab { Frontend = 0, HudAtlas = 1, Fonts = 2 };
    Tab  activeTab_     = Tab::Frontend;
    bool selectPending_ = false; // true for one frame after Open() to force-select the tab

    std::string statusMessage_;
};

} // namespace nfsmw
