#pragma once
#include "Common.h"
#include "ui/FileDialog.h"
#include "ui/TexturePackPanel.h"
#include "ui/ObjectExportPanel.h"
#include "ui/AudioPanel.h"
#include "ui/SoundBankPanel.h"
#include "ui/MusicPanel.h"
#include "ui/VideoPanel.h"
#include "ui/UIHudPanel.h"
#include "ui/CarPanel.h"
#include "ui/MinimapPanel.h"
#include "ui/EffectsPanel.h"
#include "ui/NisPanel.h"
#include "ui/RecentFiles.h"
#include "ui/UISounds.h"
#include "async/TaskQueue.h"
#include <vangui_signals.h>    // Phase 7: panel-to-panel signals
#include <filesystem>
#include <string>
#include <vector>
#include <functional>
#include <cstdint>

#if defined(__ANDROID__)
struct android_app;
#else
struct GLFWwindow;
struct GLFWcursor;
#endif

namespace nfsmw {

/// Application shell: GLFW + VanGui (no docking) + OpenGL3 bootstrap.
///
/// Layout: fixed left nav sidebar (180px) + full-height content area.
/// Nav sidebar has 3 buttons: Textures | Objects | Audio.
/// Switching nav items swaps which panel(s) draw into the content area.
class AppShell {
public:
#if defined(__ANDROID__)
    Result<void> InitAndroid(android_app* app, int width = 1400, int height = 900);
    void RenderAndroidFrame(int framebufferWidth, int framebufferHeight);
    void ShutdownAndroid();
    bool AndroidExitRequested() const { return androidExitRequested_; }
#else
    Result<void> Init(int width = 1400, int height = 900);

    /// Blocks until the window closes.
    void Run();
#endif

private:
#if defined(__ANDROID__)
    android_app*        androidApp_           = nullptr;
    bool                androidInitialized_   = false;
    bool                androidExitRequested_ = false;
#else
    GLFWwindow*        glfwWindow_     = nullptr;
    GLFWcursor*        customCursor_   = nullptr;
    bool               comInitialized_ = false;
#endif
    bool               micaActive_     = false;

    // ── Panels ───────────────────────────────────────────────────────────────
    FileDialog         fileDialog_;
    TexturePackPanel   texPanel_;
    TaskQueue          taskQueue_;
    ObjectExportPanel  exportPanel_;
    AudioPanel         audioPanel_;
    SoundBankPanel     soundBankPanel_;
    MusicPanel         musicPanel_;
    VideoPanel         videoPanel_;
    UIHudPanel         uiHudPanel_;
    CarPanel           carPanel_;
    MinimapPanel       minimapPanel_;
    EffectsPanel       effectsPanel_;
    NisPanel           nisPanel_;
    bool               exportPanelReady_ = false;
    bool               carPanelReady_    = false;

    // ── Polish ───────────────────────────────────────────────────────────────
    RecentFiles        recentFiles_;
    UISounds           uiSounds_;
    std::string        statusMessage_ = "Ready";

    // ── Signals (Phase 7) ─────────────────────────────────────────────────────
    // Emitted (on the main thread) after a BUN/BIN is loaded into the texture
    // panel. Decouples downstream panels from AppShell's open handlers.
    VanGui::VanSignal<const std::filesystem::path&> onBunLoaded_;
    VanGui::VanConnection onBunLoadedConn_;

    // ── Nav state ────────────────────────────────────────────────────────────
    enum class NavSection { Textures, Objects, Cars, Effects, Audio, Videos, Cutscenes, UIHud, Minimap };
    NavSection         activeSection_ = NavSection::Textures;

    enum class ThemeMode { Dark, Light };
    ThemeMode          themeMode_ = ThemeMode::Dark;

    // Audio has 2 sub-sections consolidated under one nav item.
    enum class AudioSubTab { SoundBank, Music };
    AudioSubTab        audioSubTab_ = AudioSubTab::SoundBank;

    // Videos: only VP6 movies now. NIS cutscenes have their own nav section.
    // (enum kept minimal; tab bar removed from DrawVideosContent)


    // ── Layout constants ─────────────────────────────────────────────────────
    static constexpr float kNavWidth       = 180.0f;
    static constexpr float kMenuBarHeight  = 0.0f; // no main menu bar in new design

    // ── Frame functions ───────────────────────────────────────────────────────
    void DrawFrame();
    void DrawNavSidebar(float sidebarTop, float height);
    void DrawContentArea(float x, float y, float w, float h);

    void DrawTexturesContent(float w, float h);
    void DrawObjectsContent(float w, float h);
    void DrawCarsContent(float w, float h);
    void DrawEffectsContent(float w, float h);
    void DrawAudioContent(float w, float h);
    void DrawVideosContent(float w, float h);
    void DrawCutscenesContent(float w, float h);
    void DrawUIHudContent(float w, float h);
    void DrawMinimapContent(float w, float h);

    void DrawStatusBar();
    void DrawMenuBar();
    void DrawAboutDialogs();

    // ── Custom (borderless) window controls ───────────────────────────────────
    enum class WinBtn { Minimize, Maximize, Restore, Close };
    void DrawWindowControls();
    bool WindowButton(const char* id, WinBtn type, float w, float h);

    void ApplyTheme(float dpiScale);
    void LoadFont(float dpiScale);
    void ToggleTheme();
    void ApplyDarkTitleBar(bool dark);

    // ── Nav sidebar helpers ───────────────────────────────────────────────────
    /// Draw one nav button; returns true if it was clicked.
    bool NavButton(const char* label, NavSection section, float width);

    // ── File open handlers ────────────────────────────────────────────────────
    void OnOpenBUN(const std::filesystem::path& path);
    void OnOpenBUNForExport(const std::filesystem::path& path);
    void OnOpenSNR(const std::filesystem::path& path);
    void OnOpenABK(const std::filesystem::path& path);
    void OnOpenGIN(const std::filesystem::path& path);
    void OnOpenMPF(const std::filesystem::path& mpfPath,
                   const std::filesystem::path& musPath = {});
    void OnOpenVideo(const std::filesystem::path& path);
    void OnOpenNis(const std::filesystem::path& path);
    void OnOpenUIHud(const std::filesystem::path& path);
    void OnOpenMinimap(const std::filesystem::path& path);
    void OnOpenCars(const std::filesystem::path& path);

    void RecordOpen(RecentKind kind, const std::filesystem::path& path,
                    const char* verb);
    void DrawRecentSubmenu(const char* label, RecentKind kind,
                           const std::function<void(const std::filesystem::path&)>& onPick);

    void OnFilesDropped(const std::vector<std::filesystem::path>& paths);

    // ── Shortcuts + command palette (Phase 5) ─────────────────────────────────
    void RegisterShortcutsAndCommands();
    void ActionOpenBUN();
    void ActionExportSelectedTexture();
    void ActionRevertSelected();

#if !defined(__ANDROID__)
    static void DropCallback(GLFWwindow* window, int count, const char** paths);
#endif

    void UpdateStatusMessage();
};

// Theme colours — defined in AppShell.cpp, shared with any panel that
// includes this header.
#include <vangui.h>
extern VanVec4 g_accent;
extern VanVec4 g_bg0;
extern VanVec4 g_bg1;
extern VanVec4 g_bg2;
extern VanVec4 g_bg3;
extern VanVec4 g_border;
extern VanVec4 g_text;

} // namespace nfsmw
