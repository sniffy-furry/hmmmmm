#include "ui/FileDialog.h"
#include <vangui.h>
#include <algorithm>
#include <cstring>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <commdlg.h>
#include <shobjidl.h>   // IFileOpenDialog, FOS_PICKFOLDERS
#include <shlobj.h>     // SHCreateItemFromParsingName
#ifdef _MSC_VER
#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "shell32.lib")
#endif
#endif

namespace nfsmw {

namespace fs = std::filesystem;

#ifdef _WIN32
// --- Native Windows open/save dialog ------------------------------------------
namespace {

std::wstring Widen(const std::string& s) {
    if (s.empty()) return {};
    int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    std::wstring w(n ? n - 1 : 0, L'\0');
    if (n) MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, w.data(), n);
    return w;
}

/// Build a comdlg filter like: "Supported files\0*.bun;*.bin\0All files\0*.*\0\0"
std::wstring BuildFilter(const std::vector<std::string>& extensions) {
    std::wstring patterns;
    for (const auto& e : extensions) {
        if (!patterns.empty()) patterns += L";";
        patterns += L"*" + Widen(e);
    }
    std::wstring f;
    if (!patterns.empty()) {
        f += L"Supported files (" + patterns + L")";
        f.push_back(L'\0');
        f += patterns;
        f.push_back(L'\0');
    }
    f += L"All files (*.*)";
    f.push_back(L'\0');
    f += L"*.*";
    f.push_back(L'\0');
    f.push_back(L'\0');
    return f;
}

} // namespace

void FileDialog::Show(std::string title, Mode mode,
                      std::vector<std::string> extensions,
                      std::function<void(const fs::path&)> onAccept,
                      std::string defaultName) {
    // Synchronous native dialog; the render loop resumes afterwards.
    const std::wstring wTitle  = Widen(title);
    const std::wstring wFilter = BuildFilter(extensions);

    wchar_t fileBuf[MAX_PATH] = L"";
    if (!defaultName.empty()) {
        std::wstring w = Widen(defaultName);
        wcsncpy_s(fileBuf, w.c_str(), MAX_PATH - 1);
    }

    OPENFILENAMEW ofn{};
    ofn.lStructSize  = sizeof(ofn);
    // Issue #40 fix: GetActiveWindow() returns NULL when the app is not the
    // foreground window, causing the dialog to appear without a parent and
    // potentially land behind the main window on multi-monitor setups.
    // The caller should supply the GLFW window's HWND via SetOwnerHWND(); fall
    // back to GetActiveWindow() only if it was never set.
    ofn.hwndOwner    = ownerHWND_ ? ownerHWND_ : GetActiveWindow();
    ofn.lpstrFilter  = wFilter.c_str();
    ofn.nFilterIndex = 1;
    ofn.lpstrFile    = fileBuf;
    ofn.nMaxFile     = MAX_PATH;
    ofn.lpstrTitle   = wTitle.c_str();
    ofn.Flags        = OFN_NOCHANGEDIR | OFN_EXPLORER;

    std::wstring defExt;
    if (!extensions.empty() && extensions[0].size() > 1) {
        defExt = Widen(extensions[0].substr(1)); // drop the dot
        ofn.lpstrDefExt = defExt.c_str();
    }

    BOOL ok;
    if (mode == Mode::Folder) {
        // Use the modern IFileOpenDialog COM picker (Vista+) for folder selection.
        IFileOpenDialog* pfd = nullptr;
        HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr,
                                      CLSCTX_INPROC_SERVER,
                                      IID_PPV_ARGS(&pfd));
        if (SUCCEEDED(hr)) {
            DWORD dwFlags = 0;
            pfd->GetOptions(&dwFlags);
            pfd->SetOptions(dwFlags | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM
                                    | FOS_PATHMUSTEXIST | FOS_NOCHANGEDIR);
            pfd->SetTitle(wTitle.c_str());
            if (ownerHWND_ ? ownerHWND_ : GetActiveWindow())
                hr = pfd->Show(ownerHWND_ ? ownerHWND_ : GetActiveWindow());
            else
                hr = pfd->Show(nullptr);
            if (SUCCEEDED(hr)) {
                IShellItem* psi = nullptr;
                if (SUCCEEDED(pfd->GetResult(&psi))) {
                    PWSTR pszPath = nullptr;
                    if (SUCCEEDED(psi->GetDisplayName(SIGDN_FILESYSPATH, &pszPath))) {
                        if (onAccept)
                            onAccept(fs::path(pszPath));
                        CoTaskMemFree(pszPath);
                    }
                    psi->Release();
                }
            }
            pfd->Release();
        }
        return;
    }

    if (mode == Mode::Open) {
        ofn.Flags |= OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
        ok = GetOpenFileNameW(&ofn);
    } else {
        ofn.Flags |= OFN_OVERWRITEPROMPT;
        ok = GetSaveFileNameW(&ofn);
    }

    if (ok && onAccept)
        onAccept(fs::path(fileBuf));
}

void FileDialog::Draw() {
    // Native dialogs are synchronous; nothing to draw per-frame.
}

bool FileDialog::MatchesFilter(const fs::path&) const { return true; }

#else
// --- Portable VanGui fallback (non-Windows builds) ------------------------------

void FileDialog::Show(std::string title, Mode mode,
                      std::vector<std::string> extensions,
                      std::function<void(const fs::path&)> onAccept,
                      std::string defaultName) {
    title_      = std::move(title);
    mode_       = mode;
    extensions_ = std::move(extensions);
    onAccept_   = std::move(onAccept);
    fileName_   = std::move(defaultName);
    error_.clear();
    if (cwd_.empty() || !fs::exists(cwd_))
        cwd_ = fs::current_path();
    open_ = true;
    justOpened_ = true;
}

bool FileDialog::MatchesFilter(const fs::path& p) const {
    if (extensions_.empty()) return true;
    std::string ext = p.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c) { return (char)std::tolower(c); });
    return std::find(extensions_.begin(), extensions_.end(), ext) != extensions_.end();
}

void FileDialog::RefreshDirCache() {
    cachedDirs_.clear();
    cachedFiles_.clear();
    std::error_code ec;
    for (auto& entry : fs::directory_iterator(cwd_, ec)) {
        if (entry.is_directory(ec)) cachedDirs_.push_back(entry);
        else if (MatchesFilter(entry.path())) cachedFiles_.push_back(entry);
    }
    auto byName = [](const fs::directory_entry& a, const fs::directory_entry& b) {
        return a.path().filename() < b.path().filename();
    };
    std::sort(cachedDirs_.begin(), cachedDirs_.end(), byName);
    std::sort(cachedFiles_.begin(), cachedFiles_.end(), byName);
    cachedCwd_ = cwd_;
    dirCacheValid_ = true;
}

void FileDialog::Draw() {
    if (!open_) return;
    if (justOpened_) {
        VanGui::OpenPopup(title_.c_str());
        justOpened_ = false;
    }

    VanGui::SetNextWindowSize(VanVec2(640, 460), VanGuiCond_FirstUseEver);
    if (!VanGui::BeginPopupModal(title_.c_str(), &open_)) return;

    if (VanGui::Button("Up")) {
        if (cwd_.has_parent_path() && cwd_.parent_path() != cwd_)
            cwd_ = cwd_.parent_path();
    }
    VanGui::SameLine();
    VanGui::TextWrapped("%s", cwd_.string().c_str());
    VanGui::Separator();

    if (!dirCacheValid_ || cachedCwd_ != cwd_) RefreshDirCache();

    const float footer = VanGui::GetFrameHeightWithSpacing() * 2.4f;
    if (VanGui::BeginChild("##files", VanVec2(0, -footer), VanGuiChildFlags_Borders)) {
        for (auto& d : cachedDirs_) {
            std::string label = "[dir] " + d.path().filename().string();
            bool selDir = (mode_ == Mode::Folder &&
                           d.path().filename().string() == fileName_);
            if (VanGui::Selectable(label.c_str(), selDir,
                                  VanGuiSelectableFlags_AllowDoubleClick)) {
                if (mode_ == Mode::Folder)
                    fileName_ = d.path().filename().string();
                if (VanGui::IsMouseDoubleClicked(VanGuiMouseButton_Left)) {
                    if (mode_ == Mode::Folder) {
                        // Double-click navigates AND selects the folder
                        cwd_ = d.path();
                        fileName_.clear();
                    } else {
                        cwd_ = d.path();
                    }
                }
            }
        }
        for (auto& f : cachedFiles_) {
            std::string name = f.path().filename().string();
            bool selected = (name == fileName_);
            if (VanGui::Selectable(name.c_str(), selected,
                                  VanGuiSelectableFlags_AllowDoubleClick)) {
                fileName_ = name;
                if (VanGui::IsMouseDoubleClicked(VanGuiMouseButton_Left)) {
                    auto chosen = cwd_ / fileName_;
                    open_ = false;
                    VanGui::CloseCurrentPopup();
                    if (onAccept_) onAccept_(chosen);
                }
            }
        }
    }
    VanGui::EndChild();

    char buf[512];
    std::snprintf(buf, sizeof(buf), "%s", fileName_.c_str());
    VanGui::SetNextItemWidth(-130);
    if (VanGui::InputText("##filename", buf, sizeof(buf)))
        fileName_ = buf;
    VanGui::SameLine();

    const char* acceptLabel = (mode_ == Mode::Open) ? "Open"
                            : (mode_ == Mode::Save) ? "Save"
                                                    : "Select Folder";
    bool accept = VanGui::Button(acceptLabel, VanVec2(56, 0));
    VanGui::SameLine();
    if (VanGui::Button("Cancel", VanVec2(56, 0))) {
        open_ = false;
        VanGui::CloseCurrentPopup();
    }

    if (!error_.empty())
        VanGui::TextColored(VanVec4(1, 0.4f, 0.4f, 1), "%s", error_.c_str());

    if (accept) {
        fs::path chosen;
        if (mode_ == Mode::Folder) {
            chosen = fileName_.empty() ? cwd_ : cwd_ / fileName_;
            if (!fs::is_directory(chosen)) {
                error_ = "Please select a folder";
            } else {
                open_ = false;
                VanGui::CloseCurrentPopup();
                if (onAccept_) onAccept_(chosen);
            }
        } else if (!fileName_.empty()) {
            chosen = cwd_ / fileName_;
            if (mode_ == Mode::Open && !fs::exists(chosen)) {
                error_ = "File does not exist";
            } else {
                open_ = false;
                VanGui::CloseCurrentPopup();
                if (onAccept_) onAccept_(chosen);
            }
        }
    }

    VanGui::EndPopup();
}

#endif // _WIN32

} // namespace nfsmw
