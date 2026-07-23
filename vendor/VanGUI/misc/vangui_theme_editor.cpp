#include "vangui_theme_editor.h"
// Theme engine types (defined in vangui_theme_engine.h — include that header
// in any translation unit that uses both editor and engine together).
#include "vangui_theme_engine.h"

#include "vangui_themes.h"

#include <string.h> // strlen

namespace VanGui
{

// ---------------------------------------------------------------------------
// File-scope state
// ---------------------------------------------------------------------------

namespace
{
    static VanThemeTokenSet s_WorkingSet;
    static bool             s_Initialized    = false;
    static float            s_TransitionMs   = 300.0f;
    static char             s_FilePath[512]  = "theme.vgtheme";
    static bool             s_Watching       = false;
    static VanThemeID       s_ActivePreset   = VanTheme_Dark;

    static const char* s_TokenNames[VanThemeToken_COUNT] = {
        "Background",   // VanThemeToken_Background
        "Surface",      // VanThemeToken_Surface
        "Border",       // VanThemeToken_Border
        "Primary",      // VanThemeToken_Primary
        "Secondary",    // VanThemeToken_Secondary
        "TextPrimary",  // VanThemeToken_TextPrimary
        "TextDim",      // VanThemeToken_TextDim
        "Error",        // VanThemeToken_Error
        "Warning",      // VanThemeToken_Warning
        "Success",      // VanThemeToken_Success
        "Info",         // VanThemeToken_Info
        "Overlay",      // VanThemeToken_Overlay
    };

    // ColorEdit4 flags used for all token pickers.
    static const VanGuiColorEditFlags k_ColorEditFlags =
        VanGuiColorEditFlags_AlphaBar         |
        VanGuiColorEditFlags_AlphaPreviewHalf |
        VanGuiColorEditFlags_PickerHueWheel;

} // anonymous namespace

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

static void BuildTokenID(char* buf, int buf_size, int idx)
{
    // Writes "##token_N" (or "##token_NN") into buf.
    // buf must be at least 12 bytes.
    (void)buf_size;
    buf[0] = '#'; buf[1] = '#';
    buf[2] = 't'; buf[3] = 'o'; buf[4] = 'k'; buf[5] = 'e'; buf[6] = 'n'; buf[7] = '_';
    int w = 8;
    if (idx >= 10) buf[w++] = (char)('0' + idx / 10);
    buf[w++] = (char)('0' + idx % 10);
    buf[w]   = '\0';
}

// ---------------------------------------------------------------------------

static void EnsureInitialized()
{
    if (!s_Initialized)
    {
        s_WorkingSet  = ExtractTokenSet();
        s_Initialized = true;
    }
}

// ---------------------------------------------------------------------------
// Tab 1 — Tokens
// ---------------------------------------------------------------------------

static void DrawTokensTab()
{
    // Scrollable child region for the color grid.
    // Reserve space below for the Apply / Reset buttons (~60 px).
    const float footer_h = 60.0f;
    (void)VanGui::BeginChild("##tokens_scroll", VanVec2(0.0f, -footer_h), 0, 0);

    // -- Base group --
    VanGui::SeparatorText("Base");
    for (int i = VanThemeToken_Background; i <= VanThemeToken_Border; ++i)
    {
        char id_buf[12];
        BuildTokenID(id_buf, sizeof(id_buf), i);

        (void)VanGui::ColorEdit4(id_buf, &s_WorkingSet.Colors[i].x, k_ColorEditFlags);
        VanGui::SameLine();
        VanGui::Text("%s", s_TokenNames[i]);
    }

    // -- Interactive group --
    VanGui::SeparatorText("Interactive");
    for (int i = VanThemeToken_Primary; i <= VanThemeToken_Secondary; ++i)
    {
        char id_buf[12];
        BuildTokenID(id_buf, sizeof(id_buf), i);

        (void)VanGui::ColorEdit4(id_buf, &s_WorkingSet.Colors[i].x, k_ColorEditFlags);
        VanGui::SameLine();
        VanGui::Text("%s", s_TokenNames[i]);
    }

    // -- Text group --
    VanGui::SeparatorText("Text");
    for (int i = VanThemeToken_TextPrimary; i <= VanThemeToken_TextDim; ++i)
    {
        char id_buf[12];
        BuildTokenID(id_buf, sizeof(id_buf), i);

        (void)VanGui::ColorEdit4(id_buf, &s_WorkingSet.Colors[i].x, k_ColorEditFlags);
        VanGui::SameLine();
        VanGui::Text("%s", s_TokenNames[i]);
    }

    // -- Semantic group --
    VanGui::SeparatorText("Semantic");
    for (int i = VanThemeToken_Error; i <= VanThemeToken_Info; ++i)
    {
        char id_buf[12];
        BuildTokenID(id_buf, sizeof(id_buf), i);

        (void)VanGui::ColorEdit4(id_buf, &s_WorkingSet.Colors[i].x, k_ColorEditFlags);
        VanGui::SameLine();
        VanGui::Text("%s", s_TokenNames[i]);
    }

    // -- Overlay group --
    VanGui::SeparatorText("Overlay");
    {
        const int i = VanThemeToken_Overlay;
        char id_buf[12];
        BuildTokenID(id_buf, sizeof(id_buf), i);
        (void)VanGui::ColorEdit4(id_buf, &s_WorkingSet.Colors[i].x, k_ColorEditFlags);
        VanGui::SameLine();
        VanGui::Text("%s", s_TokenNames[i]);
    }

    VanGui::EndChild();

    // Footer buttons.
    VanGui::Separator();
    if (VanGui::SmallButton("Apply"))
    {
        TransitionToTokenSet(s_WorkingSet, 250.0f);
    }
    VanGui::SameLine();
    if (VanGui::SmallButton("Apply Instantly"))
    {
        ApplyTokenSet(s_WorkingSet);
    }
    VanGui::SameLine();
    if (VanGui::SmallButton("Reset"))
    {
        s_WorkingSet = ExtractTokenSet();
    }
}

// ---------------------------------------------------------------------------
// Tab 2 — Presets
// ---------------------------------------------------------------------------

static void DrawPresetsTab()
{
    VanGui::Text("Current: %s", GetThemeName(s_ActivePreset));
    VanGui::Separator();

    const float avail  = VanGui::GetContentRegionAvail().x;
    const float pad    = 4.0f;
    const float btn_w  = avail / 2.0f - pad;

    int col = 0;
    for (int id = 0; id < (int)VanTheme_COUNT; ++id)
    {
        const char* name = GetThemeName((VanThemeID)id);
        if (VanGui::Button(name, VanVec2(btn_w, 28.0f)))
        {
            s_ActivePreset = (VanThemeID)id;
            TransitionToTheme(s_ActivePreset, s_TransitionMs);
            s_WorkingSet = GetBuiltinTokenSet(s_ActivePreset);
        }
        ++col;
        if (col % 2 != 0)
            VanGui::SameLine(0.0f, pad * 2.0f);
        else
            col = 0; // reset parity counter — already on new line
    }

    VanGui::Separator();
    (void)VanGui::SliderFloat("Transition ms", &s_TransitionMs, 0.0f, 1000.0f, "%.0f ms");
}

// ---------------------------------------------------------------------------
// Tab 3 — Import / Export
// ---------------------------------------------------------------------------

static void DrawImportExportTab()
{
    VanGui::Text("File path");
    VanGui::SameLine();
    (void)VanGui::InputText("##filepath", s_FilePath, sizeof(s_FilePath));

    VanGui::Separator();

    if (VanGui::SmallButton("Save to File"))
    {
        SaveThemeToFile(s_FilePath);
    }
    VanGui::SameLine();
    if (VanGui::SmallButton("Load from File"))
    {
        if (LoadThemeFromFile(s_FilePath))
            s_WorkingSet = ExtractTokenSet();
    }
    VanGui::SameLine();
    if (VanGui::SmallButton(s_Watching ? "Stop Watching" : "Watch File"))
    {
        s_Watching = !s_Watching;
        WatchThemeFile(s_Watching ? s_FilePath : nullptr);
    }

    if (s_Watching)
        VanGui::TextDisabled("Watching: %s", s_FilePath);
    else
        VanGui::TextDisabled("Not watching");

    VanGui::Separator();

    if (VanGui::SmallButton("Copy to Clipboard"))
    {
        size_t sz = 0;
        char*  buf = SaveThemeToMemory(&sz);
        if (buf)
        {
            VanGui::SetClipboardText(buf);
            VanGui::MemFree(buf);
        }
    }
    VanGui::SameLine();
    if (VanGui::SmallButton("Paste from Clipboard"))
    {
        // GetClipboardText() returns a pointer to an internal VanGUI buffer valid
        // only until the next clipboard/render call. Consume it immediately.
        const char* text = VanGui::GetClipboardText();
        if (text)
        {
            if (LoadThemeFromMemory(text, strlen(text)))
                s_WorkingSet = ExtractTokenSet();
        }
    }
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

VANGUI_API void ShowThemeEditor(bool* p_open)
{
    // Honour p_open at panel level if provided.
    if (p_open && !(*p_open))
        return;

    EnsureInitialized();

    // Header row: label on the left, optional close button on the right.
    VanGui::Text("Theme Editor");
    if (p_open != nullptr)
    {
        // Right-align a small [x] close button.
        float close_btn_w = VanGui::CalcTextSize("x").x + VanGui::GetStyle().FramePadding.x * 2.0f;
        VanGui::SameLine(VanGui::GetContentRegionAvail().x - close_btn_w + VanGui::GetCursorPosX());
        if (VanGui::SmallButton("x"))
            *p_open = false;
    }
    VanGui::Separator();

    if (VanGui::BeginTabBar("##theme_editor_tabs"))
    {
        if (VanGui::BeginTabItem("Tokens"))
        {
            DrawTokensTab();
            VanGui::EndTabItem();
        }
        if (VanGui::BeginTabItem("Presets"))
        {
            DrawPresetsTab();
            VanGui::EndTabItem();
        }
        if (VanGui::BeginTabItem("Import / Export"))
        {
            DrawImportExportTab();
            VanGui::EndTabItem();
        }
        VanGui::EndTabBar();
    }
}

VANGUI_API void ShowThemeEditorWindow(bool* p_open)
{
    VanGui::SetNextWindowSize(VanVec2(360.0f, 520.0f), VanGuiCond_FirstUseEver);
    if (VanGui::Begin("Theme Editor##vangui", p_open))
        ShowThemeEditor();
    VanGui::End();
}

} // namespace VanGui
