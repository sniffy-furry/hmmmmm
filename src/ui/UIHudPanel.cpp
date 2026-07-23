// ─── ui/UIHudPanel.cpp ────────────────────────────────────────────────────────
// Phase 8 — Top-level UI / HUD panel.
//
// Routes Open() calls to the appropriate sub-panel based on the filename:
//   FRONTEND/*.BUN  → UIFrontendPanel
//   GLOBALHUD.BUN   → UIHudAtlasPanel
//   *.LZC           → UIFontPanel
//
// Draws a three-tab bar (Frontend / HUD Atlas / Fonts) and delegates each
// tab's content to the matching sub-panel.
// ─────────────────────────────────────────────────────────────────────────────
#include "ui/UIHudPanel.h"
#include "core/Logger.h"
#include <vangui.h>
#include <algorithm>

namespace nfsmw {

// ─── Helpers ──────────────────────────────────────────────────────────────────

namespace {

/// Lower-case ASCII copy of `s` (extension matching).
std::string ToLower(std::string s) {
    for (auto& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

/// True when the path's stem (case-insensitive) matches `name`.
bool StemIs(const std::filesystem::path& p, const char* name) {
    return ToLower(p.stem().string()) == name;
}

} // namespace

// ─── Constructor ──────────────────────────────────────────────────────────────

UIHudPanel::UIHudPanel() = default;

void UIHudPanel::Shutdown() {
    frontendPanel_.Shutdown();
    hudAtlasPanel_.Shutdown();
    fontPanel_.Shutdown();
}

// ─── Init ─────────────────────────────────────────────────────────────────────

void UIHudPanel::Init(FileDialog* fileDialog)
{
    frontendPanel_.SetFileDialog(fileDialog);

    hudAtlasPanel_.SetFileDialog(fileDialog);

    fontPanel_.SetFileDialog(fileDialog);
}

// ─── Open ─────────────────────────────────────────────────────────────────────

void UIHudPanel::Open(const std::filesystem::path& path, TaskQueue& tasks)
{
    const std::string ext  = ToLower(path.extension().string());
    const std::string stem = ToLower(path.stem().string());

    // *.LZC → font sheets (GLOBALB.LZC is the canonical file)
    if (ext == ".lzc") {
        fontPanel_.Open(path, tasks);
        activeTab_ = Tab::Fonts;
        selectPending_ = true;
        LOG_INFO("UIHudPanel: routing {} to UIFontPanel", path.filename().string());
        return;
    }

    // *.BUN — distinguish GLOBALHUD from FRONTEND/*.BUN by stem
    if (ext == ".bun" || ext == ".bin") {
        if (stem == "globalhud") {
            hudAtlasPanel_.Open(path, tasks);
            activeTab_ = Tab::HudAtlas;
        } else {
            frontendPanel_.Open(path, tasks);
            activeTab_ = Tab::Frontend;
        }
        selectPending_ = true;
        LOG_INFO("UIHudPanel: routing {} to {}",
                 path.filename().string(),
                 activeTab_ == Tab::HudAtlas ? "UIHudAtlasPanel" : "UIFrontendPanel");
        return;
    }

    LOG_WARN("UIHudPanel::Open — unrecognised extension: {}", path.string());
    statusMessage_ = "Unsupported file type: " + path.filename().string();
}

// ─── Draw ─────────────────────────────────────────────────────────────────────

void UIHudPanel::Draw(TaskQueue& tasks)
{
    // ── Tab bar ───────────────────────────────────────────────────────────────
    constexpr VanGuiTabBarFlags kTabFlags =
        VanGuiTabBarFlags_NoCloseWithMiddleMouseButton;

    if (VanGui::BeginTabBar("##UIHudTabs", kTabFlags)) {

        // selectPending_ is set for one frame by Open() to force-select the new tab.
        // After that frame it is cleared so the user can freely click between tabs.
        auto tabFlags = [&](Tab t) -> VanGuiTabItemFlags {
            return (selectPending_ && activeTab_ == t)
                   ? VanGuiTabItemFlags_SetSelected
                   : VanGuiTabItemFlags_None;
        };

        if (VanGui::BeginTabItem("Frontend", nullptr, tabFlags(Tab::Frontend))) {
            activeTab_ = Tab::Frontend;
            frontendPanel_.Draw(tasks);
            VanGui::EndTabItem();
        }

        if (VanGui::BeginTabItem("HUD Atlas", nullptr, tabFlags(Tab::HudAtlas))) {
            activeTab_ = Tab::HudAtlas;
            hudAtlasPanel_.Draw(tasks);
            VanGui::EndTabItem();
        }

        if (VanGui::BeginTabItem("Fonts", nullptr, tabFlags(Tab::Fonts))) {
            activeTab_ = Tab::Fonts;
            fontPanel_.Draw(tasks);
            VanGui::EndTabItem();
        }

        selectPending_ = false; // consumed — clear after all tabs have been processed
        VanGui::EndTabBar();
    }

    // Status message (transient)
    if (!statusMessage_.empty()) {
        VanGui::Separator();
        VanGui::TextDisabled("%s", statusMessage_.c_str());
    }
}

} // namespace nfsmw
