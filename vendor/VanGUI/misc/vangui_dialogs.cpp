// vangui_dialogs.cpp — Pillar 3 standard dialogs. See vangui_dialogs.h.
// Empty TU unless VANGUI_ENABLE_DIALOGS is defined.

#include "vangui_dialogs.h"

#ifdef VANGUI_ENABLE_DIALOGS

#include "vangui_anim.h"
#include <filesystem>
#include <vector>
#include <string>
#include <algorithm>
#include <cstring>
#include <cstdio>

namespace fs = std::filesystem;

namespace VanGui {

// ---------------------------------------------------------------------------
// Message box
// ---------------------------------------------------------------------------

namespace {
// Pending-open requests, keyed by title hash. Tiny ring; titles are few.
struct PendingOpen { VanGuiID id; bool open; };
constexpr int kMaxPending = 16;
static PendingOpen s_Pending[kMaxPending] = {};

bool ConsumePending(VanGuiID id)
{
    for (auto& p : s_Pending)
        if (p.id == id && p.open) { p.open = false; return true; }
    return false;
}
void SetPending(VanGuiID id)
{
    for (auto& p : s_Pending)
        if (p.id == id) { p.open = true; return; }
    for (auto& p : s_Pending)
        if (!p.open)    { p.id = id; p.open = true; return; }
    s_Pending[0].id = id; s_Pending[0].open = true; // overflow: reuse slot 0
}
} // anonymous namespace

void OpenMessageBox(const char* title)
{
    SetPending(GetID(title));
}

VanDialogResult MessageBox(const char* title, const char* message, VanDialogButtons buttons)
{
    const VanGuiID id = GetID(title);
    if (ConsumePending(id))
        OpenPopup(title);

    VanDialogResult result = VanDialog_None;

    // Animated fade-in: AnimBool keyed by the title drives content alpha.
    const float a = Anim::AnimBool(id, IsPopupOpen(title),
                                   { .Duration = 0.16f, .Easing = Anim::VanEasing_QuadOut });

    SetNextWindowSize(VanVec2(0, 0), VanGuiCond_Always);
    if (a > 0.001f) PushStyleVar(VanGuiStyleVar_Alpha, a);
    if (BeginPopupModal(title, nullptr,
                        VanGuiWindowFlags_AlwaysAutoResize | VanGuiWindowFlags_NoSavedSettings))
    {
        TextWrapped("%s", message);
        Spacing();
        Separator();
        Spacing();

        const VanVec2 bsz(110.0f, 0.0f);
        switch (buttons)
        {
        case VanDialogButtons_OkCancel:
            if (Button("OK", bsz))     { result = VanDialog_Ok;     CloseCurrentPopup(); }
            SameLine();
            if (Button("Cancel", bsz)) { result = VanDialog_Cancel; CloseCurrentPopup(); }
            break;
        case VanDialogButtons_YesNo:
            if (Button("Yes", bsz))    { result = VanDialog_Yes;    CloseCurrentPopup(); }
            SameLine();
            if (Button("No", bsz))     { result = VanDialog_No;     CloseCurrentPopup(); }
            break;
        case VanDialogButtons_Ok:
        default:
            if (Button("OK", bsz))     { result = VanDialog_Ok;     CloseCurrentPopup(); }
            break;
        }
        EndPopup();
    }
    if (a > 0.001f) PopStyleVar();
    return result;
}

// ---------------------------------------------------------------------------
// File dialogs (in-GUI browser fallback)
// ---------------------------------------------------------------------------
// NOTE: a native OS picker is an open roadmap question (§14.3). This module ships
// the portable in-GUI browser; a native backend can later be selected here behind
// a platform check without changing the public API.

namespace {

struct FileDialogState
{
    bool        active   = false;
    bool        saveMode = false;
    std::string cwd;
    std::string filterExt;          // e.g. ".txt" (empty = all)
    char        nameBuf[256]  = {};
    std::vector<std::string> dirs;
    std::vector<std::string> files;
    bool        needsRefresh = true;
};

static FileDialogState s_Open;
static FileDialogState s_Save;

void ParseFirstFilterExt(const char* filters, std::string& out)
{
    out.clear();
    if (!filters) return;
    // Accept forms like "*.txt", ".txt", "txt". Take the extension after the last '.'.
    const char* dot = std::strrchr(filters, '.');
    if (dot && dot[1]) out = dot;          // ".txt"
    else if (filters[0] && filters[0] != '*') { out = "."; out += filters; }
}

void Refresh(FileDialogState& st)
{
    st.dirs.clear();
    st.files.clear();
    std::error_code ec;
    if (st.cwd.empty() || !fs::exists(st.cwd, ec))
        st.cwd = fs::current_path(ec).string();
    for (auto it = fs::directory_iterator(st.cwd, fs::directory_options::skip_permission_denied, ec);
         !ec && it != fs::directory_iterator(); it.increment(ec))
    {
        const fs::directory_entry& e = *it;
        std::error_code ec2;
        if (e.is_directory(ec2))
            st.dirs.push_back(e.path().filename().string());
        else if (e.is_regular_file(ec2))
        {
            if (st.filterExt.empty() || e.path().extension().string() == st.filterExt)
                st.files.push_back(e.path().filename().string());
        }
    }
    std::sort(st.dirs.begin(), st.dirs.end());
    std::sort(st.files.begin(), st.files.end());
    st.needsRefresh = false;
}

// Render the browser; returns: None=open, Ok=path in out, Cancel=closed.
VanDialogResult RenderBrowser(FileDialogState& st, const char* title, std::string& out)
{
    if (!st.active)
    {
        std::error_code ec;
        st.cwd = fs::current_path(ec).string();
        st.active = true;
        st.needsRefresh = true;
        OpenPopup(title);
    }
    if (st.needsRefresh) Refresh(st);

    VanDialogResult res = VanDialog_None;
    SetNextWindowSize(VanVec2(560, 420), VanGuiCond_FirstUseEver);
    if (BeginPopupModal(title, nullptr, VanGuiWindowFlags_NoSavedSettings))
    {
        TextUnformatted(st.cwd.c_str());
        Separator();

        if (BeginChild("##entries", VanVec2(0, -64), VanGuiChildFlags_Borders))
        {
            if (Button(".."))
            {
                std::error_code ec;
                fs::path p(st.cwd);
                st.cwd = p.parent_path().string();
                st.needsRefresh = true;
            }
            for (const std::string& d : st.dirs)
            {
                PushStyleColor(VanGuiCol_Text, GetColorU32(VanGuiCol_NavCursor));
                const std::string label = "[DIR] " + d;
                if (Selectable(label.c_str()))
                {
                    st.cwd = (fs::path(st.cwd) / d).string();
                    st.needsRefresh = true;
                }
                PopStyleColor();
            }
            for (const std::string& f : st.files)
            {
                if (Selectable(f.c_str()))
                {
                    std::snprintf(st.nameBuf, sizeof(st.nameBuf), "%s", f.c_str());
                }
            }
        }
        EndChild();

        SetNextItemWidth(-120.0f);
        (void)InputText("##name", st.nameBuf, sizeof(st.nameBuf));
        SameLine();
        const char* accept = st.saveMode ? "Save" : "Open";
        if (Button(accept, VanVec2(54, 0)) && st.nameBuf[0])
        {
            out = (fs::path(st.cwd) / st.nameBuf).string();
            res = VanDialog_Ok;
            CloseCurrentPopup();
        }
        SameLine();
        if (Button("Cancel", VanVec2(54, 0)))
        {
            res = VanDialog_Cancel;
            CloseCurrentPopup();
        }
        EndPopup();
    }

    if (res != VanDialog_None)
        st.active = false;
    return res;
}

} // anonymous namespace

std::expected<std::string, VanDialogResult> GetOpenFileName(const char* filters)
{
    if (!s_Open.active)
        ParseFirstFilterExt(filters, s_Open.filterExt);
    std::string out;
    const VanDialogResult r = RenderBrowser(s_Open, "Open File##van", out);
    if (r == VanDialog_Ok) return out;
    return std::unexpected(r);          // None (open) or Cancel
}

std::expected<std::string, VanDialogResult> GetSaveFileName(const char* default_name, const char* filters)
{
    if (!s_Save.active)
    {
        s_Save.saveMode = true;
        ParseFirstFilterExt(filters, s_Save.filterExt);
        if (default_name) std::snprintf(s_Save.nameBuf, sizeof(s_Save.nameBuf), "%s", default_name);
    }
    std::string out;
    const VanDialogResult r = RenderBrowser(s_Save, "Save File##van", out);
    if (r == VanDialog_Ok) return out;
    return std::unexpected(r);
}

} // namespace VanGui

#endif // VANGUI_ENABLE_DIALOGS
