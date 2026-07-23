// vangui_theme_engine.cpp
// Semantic token system, animated theme transitions, per-scope token overrides,
// and hot-reload support for VanGUI.

#include "vangui_theme_engine.h"
#include "vangui_anim.h"

#include <string.h>  // memcpy, strncpy, strcmp

#ifdef _MSC_VER
#include <sys/types.h>
#endif
#include <sys/stat.h>  // stat() / _stat() for hot-reload mtime tracking

// ---------------------------------------------------------------------------
// File-scope helpers (static, not exposed in header)
// ---------------------------------------------------------------------------

namespace
{

static inline float Clamp01(float v)
{
    return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
}

// Component-wise linear interpolation between two colors (including alpha).
static VanVec4 Lerp4(VanVec4 a, VanVec4 b, float t)
{
    return VanVec4(
        a.x + (b.x - a.x) * t,
        a.y + (b.y - a.y) * t,
        a.z + (b.z - a.z) * t,
        a.w + (b.w - a.w) * t
    );
}

// Alias for Lerp4 (readability at call sites that think in "mix").
static VanVec4 Mix(VanVec4 a, VanVec4 b, float t)
{
    return Lerp4(a, b, t);
}

// Subtract amount from RGB channels and clamp; alpha unchanged.
static VanVec4 Darken(VanVec4 c, float amount)
{
    return VanVec4(
        Clamp01(c.x - amount),
        Clamp01(c.y - amount),
        Clamp01(c.z - amount),
        c.w
    );
}

// Add amount to RGB channels and clamp; alpha unchanged.
static VanVec4 Lighten(VanVec4 c, float amount)
{
    return VanVec4(
        Clamp01(c.x + amount),
        Clamp01(c.y + amount),
        Clamp01(c.z + amount),
        c.w
    );
}

// Return color with alpha replaced by a.
static VanVec4 WithAlpha(VanVec4 c, float a)
{
    return VanVec4(c.x, c.y, c.z, a);
}

// Convert a packed 0xRRGGBB integer to a normalized VanVec4 with alpha = 1.
static VanVec4 HexRGB(unsigned int hex)
{
    return VanVec4(
        static_cast<float>((hex >> 16) & 0xFF) / 255.0f,
        static_cast<float>((hex >>  8) & 0xFF) / 255.0f,
        static_cast<float>((hex      ) & 0xFF) / 255.0f,
        1.0f
    );
}

// ---------------------------------------------------------------------------
// Transition state
// ---------------------------------------------------------------------------

struct TransitionState
{
    VanGuiStyle FromStyle;
    VanGuiStyle ToStyle;
    float       StartTime;   // (legacy) kept for ABI; progress now from vangui_anim
    float       DurationMs;  // duration in milliseconds
    bool        Active;
    bool        NeedSeed;    // prime the anim slot to 0 on the first render frame
};

// Stable id for the single theme-transition tween in the anim pool ("zmTE").
static const VanGuiID k_ThemeAnimId = (VanGuiID)0x7A6D5445u;

static TransitionState s_Transition = {};

// ---------------------------------------------------------------------------
// Per-scope override stack
// ---------------------------------------------------------------------------

struct OverrideEntry
{
    int ColCount;  // number of PushStyleColor calls made for this entry
};

static OverrideEntry s_OverrideStack[32];
static int           s_OverrideStackDepth = 0;

// ---------------------------------------------------------------------------
// Hot-reload state
// ---------------------------------------------------------------------------

struct HotReloadState
{
    char   FilePath[512];
    time_t LastMTime;
    bool   Watching;
};

static HotReloadState s_HotReload = {};

// Return the modification time of filepath, or 0 on failure.
static time_t GetFileMTime(const char* filepath)
{
#ifdef _MSC_VER
    struct _stat st;
    if (_stat(filepath, &st) != 0)
        return 0;
    return st.st_mtime;
#else
    struct stat st;
    if (stat(filepath, &st) != 0)
        return 0;
    return st.st_mtime;
#endif
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// Token -> VanGuiCol_ mapping tables
// For each token, list the VanGuiCol_ indices it affects.
// ---------------------------------------------------------------------------

namespace
{

// Maximum number of VanGuiCol_ entries a single token can drive.
// Primary drives the most (22 entries), so size to 24 for headroom.
static const int k_MaxColsPerToken = 24;

struct TokenColMapping
{
    int Cols[k_MaxColsPerToken];
    int Count;
};

// File-scope statics — not inside any function, so no local-static races.
static bool            s_MappingBuilt = false;
static TokenColMapping s_MappingTable[VanGui::VanThemeToken_COUNT];

static void BuildTokenMappingTable()
{
    if (s_MappingBuilt) return;
    s_MappingBuilt = true;

    memset(s_MappingTable, 0, sizeof(s_MappingTable));

    // Helper macro to add a col to a token's list.
    #define ADD(tok, col) \
        do { \
            TokenColMapping& m = s_MappingTable[tok]; \
            VAN_ASSERT(m.Count < k_MaxColsPerToken); \
            m.Cols[m.Count++] = (col); \
        } while (0)

    // Background
    ADD(VanGui::VanThemeToken_Background, VanGuiCol_WindowBg);
    ADD(VanGui::VanThemeToken_Background, VanGuiCol_ChildBg);
    ADD(VanGui::VanThemeToken_Background, VanGuiCol_TitleBg);
    ADD(VanGui::VanThemeToken_Background, VanGuiCol_MenuBarBg);
    ADD(VanGui::VanThemeToken_Background, VanGuiCol_ScrollbarBg);
    ADD(VanGui::VanThemeToken_Background, VanGuiCol_TitleBgCollapsed);
    ADD(VanGui::VanThemeToken_Background, VanGuiCol_TableRowBg);
    ADD(VanGui::VanThemeToken_Background, VanGuiCol_DockingEmptyBg);

    // Surface
    ADD(VanGui::VanThemeToken_Surface, VanGuiCol_PopupBg);
    ADD(VanGui::VanThemeToken_Surface, VanGuiCol_FrameBg);
    ADD(VanGui::VanThemeToken_Surface, VanGuiCol_Header);
    ADD(VanGui::VanThemeToken_Surface, VanGuiCol_Button);
    ADD(VanGui::VanThemeToken_Surface, VanGuiCol_Tab);
    ADD(VanGui::VanThemeToken_Surface, VanGuiCol_TableHeaderBg);
    ADD(VanGui::VanThemeToken_Surface, VanGuiCol_TableRowBgAlt);
    ADD(VanGui::VanThemeToken_Surface, VanGuiCol_TabDimmed);
    ADD(VanGui::VanThemeToken_Surface, VanGuiCol_TabDimmedSelected);

    // Border
    ADD(VanGui::VanThemeToken_Border, VanGuiCol_Border);
    ADD(VanGui::VanThemeToken_Border, VanGuiCol_BorderShadow);
    ADD(VanGui::VanThemeToken_Border, VanGuiCol_ScrollbarGrab);
    ADD(VanGui::VanThemeToken_Border, VanGuiCol_Separator);
    ADD(VanGui::VanThemeToken_Border, VanGuiCol_ResizeGrip);
    ADD(VanGui::VanThemeToken_Border, VanGuiCol_TableBorderStrong);
    ADD(VanGui::VanThemeToken_Border, VanGuiCol_TableBorderLight);
    ADD(VanGui::VanThemeToken_Border, VanGuiCol_TreeLines);
    ADD(VanGui::VanThemeToken_Border, VanGuiCol_TabDimmedSelectedOverline);

    // Primary
    ADD(VanGui::VanThemeToken_Primary, VanGuiCol_CheckMark);
    ADD(VanGui::VanThemeToken_Primary, VanGuiCol_CheckboxSelectedBg);
    ADD(VanGui::VanThemeToken_Primary, VanGuiCol_SliderGrab);
    ADD(VanGui::VanThemeToken_Primary, VanGuiCol_ButtonActive);
    ADD(VanGui::VanThemeToken_Primary, VanGuiCol_HeaderActive);
    ADD(VanGui::VanThemeToken_Primary, VanGuiCol_SeparatorActive);
    ADD(VanGui::VanThemeToken_Primary, VanGuiCol_ResizeGripActive);
    ADD(VanGui::VanThemeToken_Primary, VanGuiCol_TabSelected);
    ADD(VanGui::VanThemeToken_Primary, VanGuiCol_TabSelectedOverline);
    ADD(VanGui::VanThemeToken_Primary, VanGuiCol_SliderGrabActive);
    ADD(VanGui::VanThemeToken_Primary, VanGuiCol_TitleBgActive);
    ADD(VanGui::VanThemeToken_Primary, VanGuiCol_PlotLinesHovered);
    ADD(VanGui::VanThemeToken_Primary, VanGuiCol_PlotHistogramHovered);
    ADD(VanGui::VanThemeToken_Primary, VanGuiCol_TextLink);
    ADD(VanGui::VanThemeToken_Primary, VanGuiCol_TextSelectedBg);
    ADD(VanGui::VanThemeToken_Primary, VanGuiCol_NavCursor);
    ADD(VanGui::VanThemeToken_Primary, VanGuiCol_NavWindowingHighlight);
    ADD(VanGui::VanThemeToken_Primary, VanGuiCol_DockingPreview);
    ADD(VanGui::VanThemeToken_Primary, VanGuiCol_FrameBgActive);
    ADD(VanGui::VanThemeToken_Primary, VanGuiCol_ScrollbarGrabActive);
    ADD(VanGui::VanThemeToken_Primary, VanGuiCol_PlotHistogram);
    ADD(VanGui::VanThemeToken_Primary, VanGuiCol_HeaderHovered);

    // Secondary
    ADD(VanGui::VanThemeToken_Secondary, VanGuiCol_ScrollbarGrabHovered);
    ADD(VanGui::VanThemeToken_Secondary, VanGuiCol_ButtonHovered);
    ADD(VanGui::VanThemeToken_Secondary, VanGuiCol_SeparatorHovered);
    ADD(VanGui::VanThemeToken_Secondary, VanGuiCol_ResizeGripHovered);
    ADD(VanGui::VanThemeToken_Secondary, VanGuiCol_TabHovered);
    ADD(VanGui::VanThemeToken_Secondary, VanGuiCol_PlotLines);
    ADD(VanGui::VanThemeToken_Secondary, VanGuiCol_FrameBgHovered);

    // TextPrimary
    ADD(VanGui::VanThemeToken_TextPrimary, VanGuiCol_Text);
    ADD(VanGui::VanThemeToken_TextPrimary, VanGuiCol_InputTextCursor);

    // TextDim
    ADD(VanGui::VanThemeToken_TextDim, VanGuiCol_TextDisabled);

    // Error  (no direct VanGuiCol_ entry in the spec mapping; reserved for custom widgets)

    // Warning
    ADD(VanGui::VanThemeToken_Warning, VanGuiCol_DragDropTarget);
    ADD(VanGui::VanThemeToken_Warning, VanGuiCol_DragDropTargetBg);
    ADD(VanGui::VanThemeToken_Warning, VanGuiCol_UnsavedMarker);

    // Success / Info / Error — no dedicated VanGuiCol_ slots; reserved.

    // Overlay
    ADD(VanGui::VanThemeToken_Overlay, VanGuiCol_NavWindowingDimBg);
    ADD(VanGui::VanThemeToken_Overlay, VanGuiCol_ModalWindowDimBg);

    #undef ADD
}

// Build the mapping table at init time so the logic lives in one place.
// This mirrors the forward-map logic in ApplyTokenSet exactly.
static const TokenColMapping& GetTokenMapping(VanGui::VanThemeToken token)
{
    BuildTokenMappingTable();
    return s_MappingTable[token];
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// VanGui namespace implementations
// ---------------------------------------------------------------------------

namespace VanGui
{

// ---------------------------------------------------------------------------
// ApplyTokenSetImpl — writes derived colors into an arbitrary VanVec4 array.
// ApplyTokenSet     — public API: writes into GetStyle().Colors.
// ---------------------------------------------------------------------------

// Internal helper: fills the provided VanVec4 array (must have VanGuiCol_COUNT
// entries) from the token set. Never touches GetStyle() directly.
static void ApplyTokenSetImpl(const VanThemeTokenSet& tok, VanVec4* c)
{
    const VanVec4& bg      = tok.Colors[VanThemeToken_Background];
    const VanVec4& surf    = tok.Colors[VanThemeToken_Surface];
    const VanVec4& border  = tok.Colors[VanThemeToken_Border];
    const VanVec4& prim    = tok.Colors[VanThemeToken_Primary];
    const VanVec4& sec     = tok.Colors[VanThemeToken_Secondary];
    const VanVec4& textP   = tok.Colors[VanThemeToken_TextPrimary];
    const VanVec4& textD   = tok.Colors[VanThemeToken_TextDim];
    const VanVec4& warn    = tok.Colors[VanThemeToken_Warning];
    const VanVec4& overlay = tok.Colors[VanThemeToken_Overlay];

    // Text
    c[VanGuiCol_Text]           = textP;
    c[VanGuiCol_TextDisabled]   = textD;
    c[VanGuiCol_TextLink]       = Lighten(prim, 0.08f);
    c[VanGuiCol_TextSelectedBg] = WithAlpha(prim, 0.35f);
    c[VanGuiCol_InputTextCursor]= textP;

    // Window / child / popup backgrounds
    c[VanGuiCol_WindowBg] = bg;
    c[VanGuiCol_ChildBg]  = Darken(surf, 0.04f);  // slightly darker surface
    c[VanGuiCol_PopupBg]  = Lighten(surf, 0.04f); // slightly lighter surface

    // Borders
    c[VanGuiCol_Border]       = border;
    c[VanGuiCol_BorderShadow] = WithAlpha(border, 0.08f);

    // Frame (input, slider, checkbox backgrounds)
    c[VanGuiCol_FrameBg]        = Mix(bg, surf, 0.6f);
    c[VanGuiCol_FrameBgHovered] = Mix(surf, sec, 0.3f);
    c[VanGuiCol_FrameBgActive]  = Mix(surf, prim, 0.4f);

    // Title bar
    c[VanGuiCol_TitleBg]          = Darken(bg, 0.15f);
    c[VanGuiCol_TitleBgActive]    = Mix(bg, prim, 0.25f);
    c[VanGuiCol_TitleBgCollapsed] = WithAlpha(bg, 0.75f);
    c[VanGuiCol_MenuBarBg]        = Darken(bg, 0.15f);

    // Scrollbar
    c[VanGuiCol_ScrollbarBg]          = bg;
    c[VanGuiCol_ScrollbarGrab]        = border;
    c[VanGuiCol_ScrollbarGrabHovered] = sec;
    c[VanGuiCol_ScrollbarGrabActive]  = prim;

    // Checkbox / radio
    c[VanGuiCol_CheckMark]         = prim;
    c[VanGuiCol_CheckboxSelectedBg]= prim;

    // Slider
    c[VanGuiCol_SliderGrab]       = prim;
    c[VanGuiCol_SliderGrabActive] = Lighten(prim, 0.15f);

    // Button
    c[VanGuiCol_Button]        = Mix(surf, prim, 0.35f);
    c[VanGuiCol_ButtonHovered] = Mix(surf, sec,  0.5f);
    c[VanGuiCol_ButtonActive]  = prim;

    // Header (CollapsingHeader, TreeNode, Selectable, MenuItem)
    c[VanGuiCol_Header]        = Mix(surf, prim, 0.2f);
    c[VanGuiCol_HeaderHovered] = Mix(surf, sec,  0.35f);
    c[VanGuiCol_HeaderActive]  = Mix(surf, prim, 0.45f);

    // Separator
    c[VanGuiCol_Separator]        = border;
    c[VanGuiCol_SeparatorHovered] = sec;
    c[VanGuiCol_SeparatorActive]  = prim;

    // Resize grip
    c[VanGuiCol_ResizeGrip]        = WithAlpha(border, 0.2f);
    c[VanGuiCol_ResizeGripHovered] = sec;
    c[VanGuiCol_ResizeGripActive]  = prim;

    // Tabs
    c[VanGuiCol_Tab]                       = Mix(bg, surf, 0.5f);
    c[VanGuiCol_TabHovered]                = sec;
    c[VanGuiCol_TabSelected]               = prim;
    c[VanGuiCol_TabSelectedOverline]       = Lighten(prim, 0.1f);
    c[VanGuiCol_TabDimmed]                 = Darken(Mix(bg, surf, 0.5f), 0.08f);
    c[VanGuiCol_TabDimmedSelected]         = Darken(Mix(bg, surf, 0.5f), 0.04f);
    c[VanGuiCol_TabDimmedSelectedOverline] = WithAlpha(border, 0.6f);

    // Plots
    c[VanGuiCol_PlotLines]            = sec;
    c[VanGuiCol_PlotLinesHovered]     = prim;
    c[VanGuiCol_PlotHistogram]        = Mix(prim, sec, 0.5f);
    c[VanGuiCol_PlotHistogramHovered] = prim;

    // Tables
    c[VanGuiCol_TableHeaderBg]     = Darken(surf, 0.1f);
    c[VanGuiCol_TableBorderStrong] = border;
    c[VanGuiCol_TableBorderLight]  = WithAlpha(border, 0.5f);
    c[VanGuiCol_TableRowBg]        = WithAlpha(bg,   0.0f);
    c[VanGuiCol_TableRowBgAlt]     = WithAlpha(surf, 0.15f);

    // Tree
    c[VanGuiCol_TreeLines] = border;

    // Drag-drop
    c[VanGuiCol_DragDropTarget]   = warn;
    c[VanGuiCol_DragDropTargetBg] = WithAlpha(warn, 0.15f);

    // Navigation
    c[VanGuiCol_NavCursor]              = prim;
    c[VanGuiCol_NavWindowingHighlight]  = WithAlpha(prim, 0.7f);
    c[VanGuiCol_NavWindowingDimBg]      = WithAlpha(overlay, 0.2f);

    // Modal / docking
    c[VanGuiCol_ModalWindowDimBg] = WithAlpha(overlay, 0.35f);
    c[VanGuiCol_DockingPreview]   = WithAlpha(prim, 0.7f);
    c[VanGuiCol_DockingEmptyBg]   = bg;

    // Unsaved marker
    c[VanGuiCol_UnsavedMarker] = warn;
}

VANGUI_API void ApplyTokenSet(const VanThemeTokenSet& tok)
{
    BuildTokenMappingTable();
    ApplyTokenSetImpl(tok, GetStyle().Colors);
}

// ---------------------------------------------------------------------------
// ExtractTokenSet
// ---------------------------------------------------------------------------

VANGUI_API VanThemeTokenSet ExtractTokenSet()
{
    const VanGuiStyle& style = GetStyle();
    const VanVec4*     c     = style.Colors;

    VanThemeTokenSet out = {};

    // Best-fit reverse mapping — approximate, not perfectly invertible.
    out.Colors[VanThemeToken_Background] = c[VanGuiCol_WindowBg];
    out.Colors[VanThemeToken_Surface]    = c[VanGuiCol_PopupBg];
    out.Colors[VanThemeToken_Border]     = c[VanGuiCol_Border];
    out.Colors[VanThemeToken_Primary]    = c[VanGuiCol_ButtonActive];
    out.Colors[VanThemeToken_Secondary]  = c[VanGuiCol_ButtonHovered];
    out.Colors[VanThemeToken_TextPrimary]= c[VanGuiCol_Text];
    out.Colors[VanThemeToken_TextDim]    = c[VanGuiCol_TextDisabled];
    out.Colors[VanThemeToken_Warning]    = c[VanGuiCol_DragDropTarget];
    out.Colors[VanThemeToken_Overlay]    = VanVec4(0.0f, 0.0f, 0.0f, 1.0f); // no direct slot — default black

    // Error / Success / Info have no dedicated VanGuiCol_ slots.
    // Provide sensible defaults derived from Primary.
    out.Colors[VanThemeToken_Error]   = VanVec4(0.9f, 0.2f, 0.2f, 1.0f);
    out.Colors[VanThemeToken_Success] = VanVec4(0.2f, 0.8f, 0.4f, 1.0f);
    out.Colors[VanThemeToken_Info]    = VanVec4(0.3f, 0.8f, 0.9f, 1.0f);

    return out;
}

// ---------------------------------------------------------------------------
// GetBuiltinTokenSet
// ---------------------------------------------------------------------------

VANGUI_API VanThemeTokenSet GetBuiltinTokenSet(VanThemeID theme)
{
    VanThemeTokenSet t = {};

    switch (theme)
    {
    default:
    case VanTheme_Dark:
        // VanGUI default dark — near-black background, grey surface
        t.Colors[VanThemeToken_Background] = VanVec4(0.06f, 0.06f, 0.06f, 1.00f);
        t.Colors[VanThemeToken_Surface]    = VanVec4(0.14f, 0.14f, 0.14f, 1.00f);
        t.Colors[VanThemeToken_Border]     = VanVec4(0.43f, 0.43f, 0.50f, 0.50f);
        t.Colors[VanThemeToken_Primary]    = VanVec4(0.26f, 0.59f, 0.98f, 1.00f);
        t.Colors[VanThemeToken_Secondary]  = VanVec4(0.26f, 0.59f, 0.98f, 0.67f);
        t.Colors[VanThemeToken_TextPrimary]= VanVec4(1.00f, 1.00f, 1.00f, 1.00f);
        t.Colors[VanThemeToken_TextDim]    = VanVec4(0.50f, 0.50f, 0.50f, 1.00f);
        t.Colors[VanThemeToken_Error]      = VanVec4(0.90f, 0.20f, 0.20f, 1.00f);
        t.Colors[VanThemeToken_Warning]    = VanVec4(1.00f, 0.70f, 0.10f, 1.00f);
        t.Colors[VanThemeToken_Success]    = VanVec4(0.20f, 0.80f, 0.40f, 1.00f);
        t.Colors[VanThemeToken_Info]       = VanVec4(0.30f, 0.80f, 0.90f, 1.00f);
        t.Colors[VanThemeToken_Overlay]    = VanVec4(0.00f, 0.00f, 0.00f, 1.00f);
        break;

    case VanTheme_Light:
        // VanGUI default light — near-white background
        t.Colors[VanThemeToken_Background] = VanVec4(0.94f, 0.94f, 0.94f, 1.00f);
        t.Colors[VanThemeToken_Surface]    = VanVec4(1.00f, 1.00f, 1.00f, 1.00f);
        t.Colors[VanThemeToken_Border]     = VanVec4(0.60f, 0.60f, 0.60f, 0.60f);
        t.Colors[VanThemeToken_Primary]    = VanVec4(0.26f, 0.59f, 0.98f, 1.00f);
        t.Colors[VanThemeToken_Secondary]  = VanVec4(0.26f, 0.59f, 0.98f, 0.67f);
        t.Colors[VanThemeToken_TextPrimary]= VanVec4(0.00f, 0.00f, 0.00f, 1.00f);
        t.Colors[VanThemeToken_TextDim]    = VanVec4(0.60f, 0.60f, 0.60f, 1.00f);
        t.Colors[VanThemeToken_Error]      = VanVec4(0.85f, 0.15f, 0.15f, 1.00f);
        t.Colors[VanThemeToken_Warning]    = VanVec4(0.90f, 0.60f, 0.05f, 1.00f);
        t.Colors[VanThemeToken_Success]    = VanVec4(0.10f, 0.70f, 0.30f, 1.00f);
        t.Colors[VanThemeToken_Info]       = VanVec4(0.10f, 0.60f, 0.80f, 1.00f);
        t.Colors[VanThemeToken_Overlay]    = VanVec4(0.20f, 0.20f, 0.20f, 1.00f);
        break;

    case VanTheme_Classic:
        // VanGUI classic — medium grey background
        t.Colors[VanThemeToken_Background] = VanVec4(0.20f, 0.20f, 0.20f, 1.00f);
        t.Colors[VanThemeToken_Surface]    = VanVec4(0.29f, 0.29f, 0.29f, 1.00f);
        t.Colors[VanThemeToken_Border]     = VanVec4(0.50f, 0.50f, 0.50f, 0.50f);
        t.Colors[VanThemeToken_Primary]    = VanVec4(0.40f, 0.40f, 0.80f, 1.00f);
        t.Colors[VanThemeToken_Secondary]  = VanVec4(0.40f, 0.40f, 0.80f, 0.67f);
        t.Colors[VanThemeToken_TextPrimary]= VanVec4(0.90f, 0.90f, 0.90f, 1.00f);
        t.Colors[VanThemeToken_TextDim]    = VanVec4(0.60f, 0.60f, 0.60f, 1.00f);
        t.Colors[VanThemeToken_Error]      = VanVec4(0.90f, 0.20f, 0.20f, 1.00f);
        t.Colors[VanThemeToken_Warning]    = VanVec4(1.00f, 0.70f, 0.10f, 1.00f);
        t.Colors[VanThemeToken_Success]    = VanVec4(0.20f, 0.80f, 0.40f, 1.00f);
        t.Colors[VanThemeToken_Info]       = VanVec4(0.30f, 0.80f, 0.90f, 1.00f);
        t.Colors[VanThemeToken_Overlay]    = VanVec4(0.00f, 0.00f, 0.00f, 1.00f);
        break;

    case VanTheme_Dracula:
        // Palette: https://draculatheme.com/contribute
        // Background: #282a36  Surface/CurrentLine: #44475a  Comment: #6272a4
        // Primary: #bd93f9 (purple)  Secondary: #ff79c6 (pink)
        t.Colors[VanThemeToken_Background] = VanVec4(0.157f, 0.165f, 0.212f, 1.00f); // #282a36
        t.Colors[VanThemeToken_Surface]    = VanVec4(0.267f, 0.278f, 0.353f, 1.00f); // #44475a
        t.Colors[VanThemeToken_Border]     = VanVec4(0.384f, 0.447f, 0.643f, 0.50f); // #6272a4 @ 0.5
        t.Colors[VanThemeToken_Primary]    = VanVec4(0.741f, 0.576f, 0.976f, 1.00f); // #bd93f9
        t.Colors[VanThemeToken_Secondary]  = VanVec4(1.000f, 0.475f, 0.776f, 1.00f); // #ff79c6
        t.Colors[VanThemeToken_TextPrimary]= VanVec4(0.973f, 0.973f, 0.949f, 1.00f); // #f8f8f2
        t.Colors[VanThemeToken_TextDim]    = VanVec4(0.384f, 0.447f, 0.643f, 1.00f); // #6272a4
        t.Colors[VanThemeToken_Error]      = VanVec4(1.000f, 0.333f, 0.333f, 1.00f); // #ff5555
        t.Colors[VanThemeToken_Warning]    = VanVec4(1.000f, 0.722f, 0.424f, 1.00f); // #ffb86c
        t.Colors[VanThemeToken_Success]    = VanVec4(0.314f, 0.980f, 0.482f, 1.00f); // #50fa7b
        t.Colors[VanThemeToken_Info]       = VanVec4(0.545f, 0.914f, 0.992f, 1.00f); // #8be9fd
        t.Colors[VanThemeToken_Overlay]    = VanVec4(0.000f, 0.000f, 0.000f, 0.60f);
        break;

    case VanTheme_Nord:
        // Palette: https://www.nordtheme.com
        // Background: nord0 #2e3440  Surface: nord1 #3b4252  Border: nord2 #434c5e
        // Primary: nord10 #5e81ac  Secondary: nord9 #81a1c1
        t.Colors[VanThemeToken_Background] = VanVec4(0.180f, 0.204f, 0.251f, 1.00f); // #2e3440
        t.Colors[VanThemeToken_Surface]    = VanVec4(0.231f, 0.259f, 0.322f, 1.00f); // #3b4252
        t.Colors[VanThemeToken_Border]     = VanVec4(0.263f, 0.298f, 0.369f, 0.60f); // #434c5e @ 0.6
        t.Colors[VanThemeToken_Primary]    = VanVec4(0.369f, 0.506f, 0.675f, 1.00f); // #5e81ac
        t.Colors[VanThemeToken_Secondary]  = VanVec4(0.506f, 0.631f, 0.757f, 1.00f); // #81a1c1
        t.Colors[VanThemeToken_TextPrimary]= VanVec4(0.925f, 0.937f, 0.957f, 1.00f); // #eceff4
        t.Colors[VanThemeToken_TextDim]    = VanVec4(0.298f, 0.337f, 0.416f, 1.00f); // #4c566a
        t.Colors[VanThemeToken_Error]      = VanVec4(0.749f, 0.380f, 0.416f, 1.00f); // #bf616a
        t.Colors[VanThemeToken_Warning]    = VanVec4(0.816f, 0.529f, 0.439f, 1.00f); // #d08770
        t.Colors[VanThemeToken_Success]    = VanVec4(0.639f, 0.745f, 0.549f, 1.00f); // #a3be8c
        t.Colors[VanThemeToken_Info]       = VanVec4(0.533f, 0.753f, 0.816f, 1.00f); // #88c0d0
        t.Colors[VanThemeToken_Overlay]    = VanVec4(0.263f, 0.298f, 0.369f, 1.00f); // #434c5e
        break;

    case VanTheme_Monokai:
        // Palette: https://monokai.pro
        // Background: #272822  Surface: #3e3d32  Comment: #75715e
        // Primary: #ae81ff (purple)  Secondary: #f92672 (pink)
        t.Colors[VanThemeToken_Background] = VanVec4(0.153f, 0.157f, 0.133f, 1.00f); // #272822
        t.Colors[VanThemeToken_Surface]    = VanVec4(0.243f, 0.239f, 0.196f, 1.00f); // #3e3d32
        t.Colors[VanThemeToken_Border]     = VanVec4(0.459f, 0.443f, 0.369f, 0.60f); // #75715e @ 0.6
        t.Colors[VanThemeToken_Primary]    = VanVec4(0.682f, 0.506f, 1.000f, 1.00f); // #ae81ff
        t.Colors[VanThemeToken_Secondary]  = VanVec4(0.976f, 0.149f, 0.447f, 1.00f); // #f92672
        t.Colors[VanThemeToken_TextPrimary]= VanVec4(0.973f, 0.973f, 0.949f, 1.00f); // #f8f8f2
        t.Colors[VanThemeToken_TextDim]    = VanVec4(0.459f, 0.443f, 0.369f, 1.00f); // #75715e
        t.Colors[VanThemeToken_Error]      = VanVec4(0.976f, 0.149f, 0.447f, 1.00f); // #f92672 (pink = error)
        t.Colors[VanThemeToken_Warning]    = VanVec4(0.992f, 0.592f, 0.122f, 1.00f); // #fd971f
        t.Colors[VanThemeToken_Success]    = VanVec4(0.651f, 0.886f, 0.180f, 1.00f); // #a6e22e
        t.Colors[VanThemeToken_Info]       = VanVec4(0.400f, 0.851f, 0.910f, 1.00f); // #66d9e8
        t.Colors[VanThemeToken_Overlay]    = VanVec4(0.243f, 0.239f, 0.196f, 1.00f); // #3e3d32
        break;

    case VanTheme_GruvboxDark:
        // Palette: https://github.com/morhetz/gruvbox
        // Background: bg0 #282828  Surface: bg1 #3c3836  Border: bg2 #504945
        // Primary: normBlue #458588  Secondary: brightBlue #83a598
        t.Colors[VanThemeToken_Background] = VanVec4(0.157f, 0.157f, 0.157f, 1.00f); // #282828
        t.Colors[VanThemeToken_Surface]    = VanVec4(0.235f, 0.220f, 0.212f, 1.00f); // #3c3836
        t.Colors[VanThemeToken_Border]     = VanVec4(0.314f, 0.286f, 0.271f, 0.60f); // #504945 @ 0.6
        t.Colors[VanThemeToken_Primary]    = VanVec4(0.271f, 0.522f, 0.533f, 1.00f); // #458588
        t.Colors[VanThemeToken_Secondary]  = VanVec4(0.514f, 0.647f, 0.596f, 1.00f); // #83a598
        t.Colors[VanThemeToken_TextPrimary]= VanVec4(0.922f, 0.859f, 0.698f, 1.00f); // #ebdbb2
        t.Colors[VanThemeToken_TextDim]    = VanVec4(0.659f, 0.600f, 0.518f, 1.00f); // #a89984
        t.Colors[VanThemeToken_Error]      = VanVec4(0.984f, 0.286f, 0.204f, 1.00f); // #fb4934
        t.Colors[VanThemeToken_Warning]    = VanVec4(0.980f, 0.741f, 0.184f, 1.00f); // #fabd2f
        t.Colors[VanThemeToken_Success]    = VanVec4(0.722f, 0.733f, 0.149f, 1.00f); // #b8bb26
        t.Colors[VanThemeToken_Info]       = VanVec4(0.557f, 0.753f, 0.486f, 1.00f); // #8ec07c
        t.Colors[VanThemeToken_Overlay]    = VanVec4(0.314f, 0.286f, 0.271f, 1.00f); // #504945
        break;
    }

    return t;
}

// ---------------------------------------------------------------------------
// Animated transitions
// ---------------------------------------------------------------------------

VANGUI_API void TransitionToTheme(VanThemeID target, float duration_ms)
{
    TransitionToTokenSet(GetBuiltinTokenSet(target), duration_ms);
}

VANGUI_API void TransitionToTheme(const char* name, float duration_ms)
{
    // Case-insensitive ASCII comparison helper (avoids locale dependency).
    struct Local {
        static bool StrEqI(const char* a, const char* b)
        {
            for (;;)
            {
                char ca = *a >= 'A' && *a <= 'Z' ? *a + 32 : *a;
                char cb = *b >= 'A' && *b <= 'Z' ? *b + 32 : *b;
                if (ca != cb) return false;
                if (ca == '\0') return true;
                ++a; ++b;
            }
        }
    };

    // Map name string to VanThemeID. Mirrors the table in vangui_themes.cpp.
    struct NameEntry { const char* name; VanThemeID id; };
    static const NameEntry k_Names[] =
    {
        { "dark",         VanTheme_Dark        },
        { "light",        VanTheme_Light       },
        { "classic",      VanTheme_Classic     },
        { "dracula",      VanTheme_Dracula     },
        { "nord",         VanTheme_Nord        },
        { "monokai",      VanTheme_Monokai     },
        { "gruvbox_dark", VanTheme_GruvboxDark },
    };
    for (int i = 0; i < (int)(sizeof(k_Names) / sizeof(k_Names[0])); ++i)
    {
        if (Local::StrEqI(name, k_Names[i].name))
        {
            TransitionToTheme(k_Names[i].id, duration_ms);
            return;
        }
    }
    VAN_ASSERT(false && "VanGui::TransitionToTheme: unknown theme name");
}

VANGUI_API void TransitionToTokenSet(const VanThemeTokenSet& target, float duration_ms)
{
    if (duration_ms <= 0.0f)
    {
        // Zero duration — apply instantly, no animation needed.
        ApplyTokenSet(target);
        s_Transition.Active = false;
        return;
    }

    // Snapshot the current style as the "from" state.
    s_Transition.FromStyle = GetStyle();

    // Build the target style: apply the token set into a temporary copy of the
    // current style so non-color style properties are preserved, then restore.
    {
        VanGuiStyle& live = GetStyle();
        VanGuiStyle  saved = live;          // full copy (colors + sizes + flags)
        ApplyTokenSet(target);              // writes derived colors into live style
        s_Transition.ToStyle = live;        // capture result
        live = saved;                       // restore live style (no visible change this frame)
    }

    s_Transition.StartTime  = static_cast<float>(GetTime());
    s_Transition.DurationMs = duration_ms;
    s_Transition.Active     = true;
    s_Transition.NeedSeed   = true;
}

VANGUI_API void RenderThemeTransition()
{
    if (!s_Transition.Active)
        return;

    // Progress is owned by the shared animation substrate: one scalar tween from
    // 0 to 1, shaped by an easing curve. No bespoke time integration here.
    VanGui::Anim::VanAnimParams ap;
    ap.Duration = s_Transition.DurationMs / 1000.0f;
    ap.Easing   = VanGui::Anim::VanEasing_CubicInOut;   // matches old smoothstep feel

    float t;
    if (s_Transition.NeedSeed)
    {
        VanGui::Anim::Reset(k_ThemeAnimId);
        t = VanGui::Anim::AnimFloat(k_ThemeAnimId, 0.0f, ap);   // prime at 0
        s_Transition.NeedSeed = false;
    }
    else
    {
        t = VanGui::Anim::AnimFloat(k_ThemeAnimId, 1.0f, ap);
    }

    if (t >= 0.9999f)
    {
        // Transition complete — snap to final state and release the tween.
        GetStyle() = s_Transition.ToStyle;
        s_Transition.Active = false;
        VanGui::Anim::Reset(k_ThemeAnimId);
        return;
    }

    VanGuiStyle& style = GetStyle();
    for (int i = 0; i < VanGuiCol_COUNT; ++i)
    {
        style.Colors[i] = Lerp4(
            s_Transition.FromStyle.Colors[i],
            s_Transition.ToStyle.Colors[i],
            t
        );
    }
}

VANGUI_API bool IsThemeTransitioning()
{
    return s_Transition.Active;
}

// ---------------------------------------------------------------------------
// Per-scope token overrides
// ---------------------------------------------------------------------------

// For a given token, collect all VanGuiCol_ indices that ApplyTokenSet writes
// from that token, push the given color for each, return count pushed.
static int PushTokenColor(VanThemeToken token, VanVec4 color)
{
    // Build a token set from the current style, override the target token.
    VanThemeTokenSet ts = ExtractTokenSet();
    ts.Colors[token] = color;

    // Derive the full color array into a local stack buffer — never touches
    // GetStyle(), so no save/restore is needed.
    VanVec4 local_colors[VanGuiCol_COUNT];
    // Initialise from the live style so any cols not written by the token set
    // retain their current values (consistent with the old behavior).
    const VanVec4* live = GetStyle().Colors;
    for (int i = 0; i < VanGuiCol_COUNT; ++i)
        local_colors[i] = live[i];
    ApplyTokenSetImpl(ts, local_colors);

    // Push only the cols that this token contributes to.
    const TokenColMapping& mapping = GetTokenMapping(token);
    for (int i = 0; i < mapping.Count; ++i)
    {
        int col = mapping.Cols[i];
        PushStyleColor(col, local_colors[col]);
    }
    return mapping.Count;
}

VANGUI_API void PushThemeToken(VanThemeToken token, VanVec4 color)
{
    VAN_ASSERT(s_OverrideStackDepth < 32 && "VanGui::PushThemeToken: override stack overflow");
    int pushed = PushTokenColor(token, color);
    s_OverrideStack[s_OverrideStackDepth].ColCount = pushed;
    ++s_OverrideStackDepth;
}

VANGUI_API void PopThemeToken(int count)
{
    for (int n = 0; n < count; ++n)
    {
        VAN_ASSERT(s_OverrideStackDepth > 0 && "VanGui::PopThemeToken: underflow");
        --s_OverrideStackDepth;
        PopStyleColor(s_OverrideStack[s_OverrideStackDepth].ColCount);
    }
}

VANGUI_API void PushThemeTokenSet(const VanThemeTokenSet& tokens)
{
    VAN_ASSERT(s_OverrideStackDepth < 32 && "VanGui::PushThemeTokenSet: override stack overflow");

    // Compute derived colors for the whole set, push all 63.
    VanGuiStyle saved = GetStyle();
    ApplyTokenSet(tokens);
    VanGuiStyle derived = GetStyle();
    GetStyle() = saved;

    for (int i = 0; i < VanGuiCol_COUNT; ++i)
        PushStyleColor(i, derived.Colors[i]);

    s_OverrideStack[s_OverrideStackDepth].ColCount = VanGuiCol_COUNT;
    ++s_OverrideStackDepth;
}

VANGUI_API void PopThemeTokenSet()
{
    PopThemeToken(1);
}

// ---------------------------------------------------------------------------
// Hot-reload
// ---------------------------------------------------------------------------

VANGUI_API void WatchThemeFile(const char* filepath)
{
    if (!filepath)
    {
        s_HotReload.Watching  = false;
        s_HotReload.FilePath[0] = '\0';
        return;
    }
    snprintf(s_HotReload.FilePath, sizeof(s_HotReload.FilePath), "%s", filepath);
    s_HotReload.LastMTime = GetFileMTime(filepath);
    s_HotReload.Watching  = true;
}

VANGUI_API void PollThemeFileChanges(float transition_ms)
{
    if (!s_HotReload.Watching)
        return;

    time_t mtime = GetFileMTime(s_HotReload.FilePath);
    if (mtime == 0)
        return; // stat failed — file may be mid-write, silently skip

    if (mtime == s_HotReload.LastMTime)
        return; // no change

    s_HotReload.LastMTime = mtime;

    // Snapshot the style before loading so transitions can start from here.
    VanGuiStyle preLoadStyle = GetStyle();

    // Reload the file into the current style.
    if (!LoadThemeFromFile(s_HotReload.FilePath))
        return; // parse error — leave style unchanged

    if (transition_ms > 0.0f)
    {
        // LoadThemeFromFile overwrote the live style with the new colors.
        // Capture the loaded result as the target, restore the pre-load style so
        // RenderThemeTransition() can animate from the old appearance this frame.
        VanGuiStyle loadedStyle = GetStyle();
        GetStyle() = preLoadStyle;

        s_Transition.FromStyle  = preLoadStyle;
        s_Transition.ToStyle    = loadedStyle;
        s_Transition.StartTime  = static_cast<float>(GetTime());
        s_Transition.DurationMs = transition_ms;
        s_Transition.Active     = true;
    }
    // If transition_ms == 0, the loaded style is already applied — nothing more to do.
}

} // namespace VanGui
