#pragma once
#include "Common.h"
#include <filesystem>
#include <functional>
#include <string>
#include <vector>

// Forward-declare HWND so FileDialog.h compiles without pulling in <windows.h>
// in every translation unit. The member and method are guarded by #ifdef _WIN32
// so non-Windows builds are unaffected.
#ifdef _WIN32
struct HWND__;
using HWND = HWND__*;
#endif

namespace nfsmw {

/// Minimal portable VanGui file browser (no native dialogs, no extra deps).
class FileDialog {
public:
    enum class Mode { Open, Save, Folder };

    /// Open the dialog. `extensions` filter like {".bun", ".bin"} (lower-case,
    /// empty = everything). `onAccept` is invoked with the chosen path.
    void Show(std::string title, Mode mode,
              std::vector<std::string> extensions,
              std::function<void(const std::filesystem::path&)> onAccept,
              std::string defaultName = "");

    /// Draw the modal; call once per frame.
    void Draw();

    bool IsOpen() const { return open_; }

#ifdef _WIN32
    // Issue #40 fix: store the owner HWND explicitly so the dialog always has
    // a valid parent even when the app is not the active foreground window.
    // Call SetOwnerHWND(glfwGetWin32Window(window)) at startup.
    void SetOwnerHWND(HWND hwnd) { ownerHWND_ = hwnd; }
#endif

private:
    bool        open_ = false;
    bool        justOpened_ = false;
    Mode        mode_ = Mode::Open;
    std::string title_;
    std::vector<std::string> extensions_;
    std::function<void(const std::filesystem::path&)> onAccept_;

    std::filesystem::path cwd_;
    std::string fileName_;
    std::string error_;

    // Issue #33: cache the directory listing so it's only rescanned when
    // `cwd_` changes (or Refresh() is explicitly requested), instead of on
    // every Draw() call while the dialog is open.
    std::vector<std::filesystem::directory_entry> cachedDirs_;
    std::vector<std::filesystem::directory_entry> cachedFiles_;
    std::filesystem::path cachedCwd_;
    bool dirCacheValid_ = false;
    void RefreshDirCache();

#ifdef _WIN32
    HWND ownerHWND_ = nullptr;  // Issue #40 fix
#endif

    bool MatchesFilter(const std::filesystem::path& p) const;
};

} // namespace nfsmw
