// vangui_style_sheet.cpp — Pillar 3 .vss front-end. See vangui_style_sheet.h.
// Empty TU unless VANGUI_ENABLE_STYLESHEET is defined.

#include "vangui_style_sheet.h"

#ifdef VANGUI_ENABLE_STYLESHEET

#include "vangui_style_sheet_parser.inl"   // pure parser (Detail::ParseStyleSheet)
#include <cstdio>
#include <vector>
#include <string>

static std::FILE* open_file(const char* path, const char* mode)
{
#if defined(_MSC_VER)
    std::FILE* f = nullptr;
    fopen_s(&f, path, mode);
    return f;
#else
    return std::fopen(path, mode);
#endif
}

#if defined(_WIN32)
#  include <sys/types.h>
#  include <sys/stat.h>
#else
#  include <sys/stat.h>
#endif

namespace VanGui {

namespace {

void ApplyParsed(const Detail::ParsedSheet& sheet, float transition_ms)
{
    // Start from the live tokens so unspecified tokens are preserved.
    VanThemeTokenSet ts = ExtractTokenSet();
    for (int i = 0; i < VanThemeToken_COUNT; ++i)
        if (sheet.hasToken[i]) ts.Colors[i] = sheet.tokens.Colors[i];

    if (transition_ms > 0.0f) TransitionToTokenSet(ts, transition_ms);
    else                      ApplyTokenSet(ts);

    VanGuiStyle& st = GetStyle();
    if (sheet.frameRounding  >= 0.0f) st.FrameRounding  = sheet.frameRounding;
    if (sheet.windowRounding >= 0.0f) st.WindowRounding = sheet.windowRounding;
    if (sheet.framePaddingX  >= 0.0f)
        st.FramePadding = VanVec2(sheet.framePaddingX,
                                  sheet.framePaddingY >= 0.0f ? sheet.framePaddingY : st.FramePadding.y);
}

std::expected<void, const char*> LoadFromText(const char* text, size_t len, float transition_ms)
{
    Detail::ParsedSheet sheet;
    const char* err = nullptr;
    if (!Detail::ParseStyleSheet(text, len, sheet, &err))
        return std::unexpected(err ? err : "stylesheet parse error");
    ApplyParsed(sheet, transition_ms);
    return {};
}

// Hot-reload bookkeeping.
struct WatchState { std::string path; long long mtime = 0; bool active = false; };
static WatchState s_Watch;

long long FileMTime(const char* path)
{
    struct stat stbuf;
    if (stat(path, &stbuf) != 0) return 0;
    return (long long)stbuf.st_mtime;
}

bool ReadWholeFile(const char* path, std::string& out)
{
    std::FILE* f = open_file(path, "rb");
    if (!f) return false;
    std::fseek(f, 0, SEEK_END);
    long n = std::ftell(f);
    std::fseek(f, 0, SEEK_SET);
    if (n < 0) { std::fclose(f); return false; }
    out.resize((size_t)n);
    size_t rd = std::fread(out.data(), 1, (size_t)n, f);
    std::fclose(f);
    out.resize(rd);
    return true;
}

} // anonymous namespace

std::expected<void, const char*> LoadStyleSheetFromMemory(const char* text, size_t len, float transition_ms)
{
    if (!text) return std::unexpected("null stylesheet text");
    return LoadFromText(text, len, transition_ms);
}

std::expected<void, const char*> LoadStyleSheet(const char* path, float transition_ms)
{
    std::string buf;
    if (!ReadWholeFile(path, buf))
        return std::unexpected("could not open stylesheet file");
    return LoadFromText(buf.c_str(), buf.size(), transition_ms);
}

// --- scoped overrides (minimal v1) -----------------------------------------

namespace {
struct ScopeFrame { int pushedColors; };
static ScopeFrame s_ScopeStack[16];
static int        s_ScopeDepth = 0;
}

void PushStyleSheetScope(const char* selector)
{
    // v1: map a selector to its primary token color and push it onto the
    // widgets it most affects. Button -> Button colors, Window -> WindowBg, etc.
    VanThemeTokenSet ts = ExtractTokenSet();
    int pushed = 0;
    if (selector && std::strcmp(selector, "Button") == 0)
    {
        PushStyleColor(VanGuiCol_Button, ts.Colors[VanThemeToken_Primary]);
        PushStyleColor(VanGuiCol_ButtonHovered, ts.Colors[VanThemeToken_Secondary]);
        pushed = 2;
    }
    else if (selector && std::strcmp(selector, "Window") == 0)
    {
        PushStyleColor(VanGuiCol_WindowBg, ts.Colors[VanThemeToken_Background]);
        pushed = 1;
    }
    else if (selector && std::strcmp(selector, "Text") == 0)
    {
        PushStyleColor(VanGuiCol_Text, ts.Colors[VanThemeToken_TextPrimary]);
        pushed = 1;
    }
    if (s_ScopeDepth < 16) s_ScopeStack[s_ScopeDepth++] = { pushed };
}

void PopStyleSheetScope()
{
    if (s_ScopeDepth <= 0) return;
    const ScopeFrame f = s_ScopeStack[--s_ScopeDepth];
    if (f.pushedColors > 0) PopStyleColor(f.pushedColors);
}

// --- hot reload -------------------------------------------------------------

void WatchStyleSheet(const char* path)
{
    if (!path) { s_Watch.active = false; s_Watch.path.clear(); return; }
    s_Watch.path   = path;
    s_Watch.mtime  = FileMTime(path);
    s_Watch.active = true;
}

void PollStyleSheetChanges(float transition_ms)
{
    if (!s_Watch.active) return;
    const long long m = FileMTime(s_Watch.path.c_str());
    if (m != 0 && m != s_Watch.mtime)
    {
        s_Watch.mtime = m;
        (void)LoadStyleSheet(s_Watch.path.c_str(), transition_ms);
    }
}

} // namespace VanGui

#endif // VANGUI_ENABLE_STYLESHEET
