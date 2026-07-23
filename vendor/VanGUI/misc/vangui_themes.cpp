// vangui_themes.cpp
// Named theme preset system with INI-like serialization for VanGUI.

#include "vangui_themes.h"

#include <stdio.h>   // sscanf, snprintf
#include <string.h>  // strcmp, strncmp, memcpy, strlen

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

namespace
{

// Convert a packed 0xRRGGBB integer to a normalized VanVec4 with alpha = 1.0f.
static VanVec4 HexColor(unsigned int hex)
{
    return VanVec4(
        static_cast<float>((hex >> 16) & 0xFF) / 255.0f,
        static_cast<float>((hex >>  8) & 0xFF) / 255.0f,
        static_cast<float>((hex      ) & 0xFF) / 255.0f,
        1.0f
    );
}

// Same as HexColor but with an explicit alpha in [0,1].
static VanVec4 HexColorA(unsigned int hex, float a)
{
    VanVec4 c = HexColor(hex);
    c.w = a;
    return c;
}

// Blend two VanVec4 colors: result = a*(1-t) + b*t (component-wise, alpha kept from a).
static VanVec4 BlendColor(VanVec4 a, VanVec4 b, float t)
{
    return VanVec4(
        a.x + (b.x - a.x) * t,
        a.y + (b.y - a.y) * t,
        a.z + (b.z - a.z) * t,
        a.w
    );
}

// ---------------------------------------------------------------------------
// Dracula theme
// Palette reference: https://draculatheme.com/contribute
// ---------------------------------------------------------------------------
static void StyleColorsDracula(VanGuiStyle* dst)
{
    VanGuiStyle* style = dst ? dst : &VanGui::GetStyle();

    // Dracula palette
    const VanVec4 bg          = HexColor(0x282a36); // Background
    const VanVec4 currentLine = HexColor(0x44475a); // Current Line
    const VanVec4 selection   = HexColor(0x44475a); // Selection (same as Current Line)
    const VanVec4 fg          = HexColor(0xf8f8f2); // Foreground
    const VanVec4 comment     = HexColor(0x6272a4); // Comment
    const VanVec4 cyan        = HexColor(0x8be9fd);
    const VanVec4 green       = HexColor(0x50fa7b);
    const VanVec4 orange      = HexColor(0xffb86c);
    const VanVec4 pink        = HexColor(0xff79c6);
    const VanVec4 purple      = HexColor(0xbd93f9);
    const VanVec4 red         = HexColor(0xff5555);
    const VanVec4 yellow      = HexColor(0xf1fa8c);
    const VanVec4 titleBg     = HexColor(0x21222c);

    VanVec4* c = style->Colors;

    c[VanGuiCol_Text]                       = fg;
    c[VanGuiCol_TextDisabled]               = comment;
    c[VanGuiCol_WindowBg]                   = bg;
    c[VanGuiCol_ChildBg]                    = HexColorA(0x21222c, 1.0f);
    c[VanGuiCol_PopupBg]                    = HexColor(0x21222c);
    c[VanGuiCol_Border]                     = currentLine;
    c[VanGuiCol_BorderShadow]               = HexColorA(0x000000, 0.0f);
    c[VanGuiCol_FrameBg]                    = currentLine;
    c[VanGuiCol_FrameBgHovered]             = BlendColor(currentLine, purple, 0.25f);
    c[VanGuiCol_FrameBgActive]              = BlendColor(currentLine, purple, 0.50f);
    c[VanGuiCol_TitleBg]                    = titleBg;
    c[VanGuiCol_TitleBgActive]              = HexColor(0x6272a4);
    c[VanGuiCol_TitleBgCollapsed]           = HexColorA(0x21222c, 0.75f);
    c[VanGuiCol_MenuBarBg]                  = titleBg;
    c[VanGuiCol_ScrollbarBg]                = HexColorA(0x21222c, 0.60f);
    c[VanGuiCol_ScrollbarGrab]              = comment;
    c[VanGuiCol_ScrollbarGrabHovered]       = purple;
    c[VanGuiCol_ScrollbarGrabActive]        = pink;
    c[VanGuiCol_CheckMark]                  = green;
    c[VanGuiCol_CheckboxSelectedBg]         = currentLine;
    c[VanGuiCol_SliderGrab]                 = purple;
    c[VanGuiCol_SliderGrabActive]           = pink;
    c[VanGuiCol_Button]                     = BlendColor(currentLine, purple, 0.35f);
    c[VanGuiCol_ButtonHovered]              = BlendColor(currentLine, pink, 0.55f);
    c[VanGuiCol_ButtonActive]               = pink;
    c[VanGuiCol_Header]                     = BlendColor(currentLine, purple, 0.30f);
    c[VanGuiCol_HeaderHovered]              = BlendColor(currentLine, purple, 0.55f);
    c[VanGuiCol_HeaderActive]               = purple;
    c[VanGuiCol_Separator]                  = comment;
    c[VanGuiCol_SeparatorHovered]           = cyan;
    c[VanGuiCol_SeparatorActive]            = cyan;
    c[VanGuiCol_ResizeGrip]                 = HexColorA(0xbd93f9, 0.25f);
    c[VanGuiCol_ResizeGripHovered]          = HexColorA(0xbd93f9, 0.67f);
    c[VanGuiCol_ResizeGripActive]           = pink;
    c[VanGuiCol_InputTextCursor]            = fg;
    c[VanGuiCol_TabHovered]                 = BlendColor(currentLine, purple, 0.50f);
    c[VanGuiCol_Tab]                        = BlendColor(titleBg, purple, 0.15f);
    c[VanGuiCol_TabSelected]                = BlendColor(currentLine, purple, 0.35f);
    c[VanGuiCol_TabSelectedOverline]        = purple;
    c[VanGuiCol_TabDimmed]                  = HexColorA(0x21222c, 0.90f);
    c[VanGuiCol_TabDimmedSelected]          = BlendColor(titleBg, purple, 0.20f);
    c[VanGuiCol_TabDimmedSelectedOverline]  = comment;
    c[VanGuiCol_PlotLines]                  = cyan;
    c[VanGuiCol_PlotLinesHovered]           = yellow;
    c[VanGuiCol_PlotHistogram]              = orange;
    c[VanGuiCol_PlotHistogramHovered]       = yellow;
    c[VanGuiCol_TableHeaderBg]              = titleBg;
    c[VanGuiCol_TableBorderStrong]          = currentLine;
    c[VanGuiCol_TableBorderLight]           = HexColorA(0x44475a, 0.60f);
    c[VanGuiCol_TableRowBg]                 = HexColorA(0x000000, 0.0f);
    c[VanGuiCol_TableRowBgAlt]              = HexColorA(0xf8f8f2, 0.04f);
    c[VanGuiCol_TextLink]                   = cyan;
    c[VanGuiCol_TextSelectedBg]             = HexColorA(0xbd93f9, 0.35f);
    c[VanGuiCol_TreeLines]                  = comment;
    c[VanGuiCol_DragDropTarget]             = yellow;
    c[VanGuiCol_DragDropTargetBg]           = HexColorA(0xf1fa8c, 0.15f);
    c[VanGuiCol_UnsavedMarker]              = orange;
    c[VanGuiCol_NavCursor]                  = purple;
    c[VanGuiCol_NavWindowingHighlight]      = HexColorA(0xf8f8f2, 0.70f);
    c[VanGuiCol_NavWindowingDimBg]          = HexColorA(0x44475a, 0.20f);
    c[VanGuiCol_ModalWindowDimBg]           = HexColorA(0x44475a, 0.35f);
    c[VanGuiCol_DockingPreview]             = HexColorA(0xbd93f9, 0.70f);
    c[VanGuiCol_DockingEmptyBg]             = HexColor(0x21222c);
}

// ---------------------------------------------------------------------------
// Nord theme
// Palette reference: https://www.nordtheme.com/docs/colors-and-palettes
// ---------------------------------------------------------------------------
static void StyleColorsNord(VanGuiStyle* dst)
{
    VanGuiStyle* style = dst ? dst : &VanGui::GetStyle();

    // Polar Night
    const VanVec4 nord0  = HexColor(0x2e3440);
    const VanVec4 nord1  = HexColor(0x3b4252);
    const VanVec4 nord2  = HexColor(0x434c5e);
    const VanVec4 nord3  = HexColor(0x4c566a);
    // Snow Storm
    const VanVec4 nord4  = HexColor(0xd8dee9);
    const VanVec4 nord5  = HexColor(0xe5e9f0);
    const VanVec4 nord6  = HexColor(0xeceff4);
    // Frost
    const VanVec4 nord7  = HexColor(0x8fbcbb);
    const VanVec4 nord8  = HexColor(0x88c0d0);
    const VanVec4 nord9  = HexColor(0x81a1c1);
    const VanVec4 nord10 = HexColor(0x5e81ac);
    // Aurora
    const VanVec4 auroraRed    = HexColor(0xbf616a);
    const VanVec4 auroraOrange = HexColor(0xd08770);
    const VanVec4 auroraYellow = HexColor(0xebcb8b);
    const VanVec4 auroraGreen  = HexColor(0xa3be8c);
    const VanVec4 auroraPurple = HexColor(0xb48ead);

    VanVec4* c = style->Colors;

    c[VanGuiCol_Text]                       = nord6;
    c[VanGuiCol_TextDisabled]               = nord3;
    c[VanGuiCol_WindowBg]                   = nord0;
    c[VanGuiCol_ChildBg]                    = HexColorA(0x2e3440, 0.0f);
    c[VanGuiCol_PopupBg]                    = nord1;
    c[VanGuiCol_Border]                     = nord2;
    c[VanGuiCol_BorderShadow]               = HexColorA(0x000000, 0.0f);
    c[VanGuiCol_FrameBg]                    = nord1;
    c[VanGuiCol_FrameBgHovered]             = nord2;
    c[VanGuiCol_FrameBgActive]              = nord3;
    c[VanGuiCol_TitleBg]                    = nord0;
    c[VanGuiCol_TitleBgActive]              = nord10;
    c[VanGuiCol_TitleBgCollapsed]           = HexColorA(0x2e3440, 0.75f);
    c[VanGuiCol_MenuBarBg]                  = nord1;
    c[VanGuiCol_ScrollbarBg]                = HexColorA(0x2e3440, 0.60f);
    c[VanGuiCol_ScrollbarGrab]              = nord3;
    c[VanGuiCol_ScrollbarGrabHovered]       = nord9;
    c[VanGuiCol_ScrollbarGrabActive]        = nord8;
    c[VanGuiCol_CheckMark]                  = auroraGreen;
    c[VanGuiCol_CheckboxSelectedBg]         = nord1;
    c[VanGuiCol_SliderGrab]                 = nord9;
    c[VanGuiCol_SliderGrabActive]           = nord8;
    c[VanGuiCol_Button]                     = nord10;
    c[VanGuiCol_ButtonHovered]              = nord9;
    c[VanGuiCol_ButtonActive]               = nord8;
    c[VanGuiCol_Header]                     = HexColorA(0x5e81ac, 0.31f);
    c[VanGuiCol_HeaderHovered]              = HexColorA(0x5e81ac, 0.80f);
    c[VanGuiCol_HeaderActive]               = nord10;
    c[VanGuiCol_Separator]                  = nord2;
    c[VanGuiCol_SeparatorHovered]           = nord7;
    c[VanGuiCol_SeparatorActive]            = nord8;
    c[VanGuiCol_ResizeGrip]                 = HexColorA(0x81a1c1, 0.20f);
    c[VanGuiCol_ResizeGripHovered]          = HexColorA(0x81a1c1, 0.67f);
    c[VanGuiCol_ResizeGripActive]           = nord8;
    c[VanGuiCol_InputTextCursor]            = nord6;
    c[VanGuiCol_TabHovered]                 = nord9;
    c[VanGuiCol_Tab]                        = nord1;
    c[VanGuiCol_TabSelected]                = nord10;
    c[VanGuiCol_TabSelectedOverline]        = nord8;
    c[VanGuiCol_TabDimmed]                  = HexColorA(0x2e3440, 0.90f);
    c[VanGuiCol_TabDimmedSelected]          = HexColorA(0x3b4252, 0.80f);
    c[VanGuiCol_TabDimmedSelectedOverline]  = nord3;
    c[VanGuiCol_PlotLines]                  = nord7;
    c[VanGuiCol_PlotLinesHovered]           = auroraYellow;
    c[VanGuiCol_PlotHistogram]              = auroraOrange;
    c[VanGuiCol_PlotHistogramHovered]       = auroraYellow;
    c[VanGuiCol_TableHeaderBg]              = nord0;
    c[VanGuiCol_TableBorderStrong]          = nord2;
    c[VanGuiCol_TableBorderLight]           = HexColorA(0x434c5e, 0.60f);
    c[VanGuiCol_TableRowBg]                 = HexColorA(0x000000, 0.0f);
    c[VanGuiCol_TableRowBgAlt]              = HexColorA(0xeceff4, 0.04f);
    c[VanGuiCol_TextLink]                   = nord8;
    c[VanGuiCol_TextSelectedBg]             = HexColorA(0x5e81ac, 0.35f);
    c[VanGuiCol_TreeLines]                  = nord3;
    c[VanGuiCol_DragDropTarget]             = auroraYellow;
    c[VanGuiCol_DragDropTargetBg]           = HexColorA(0xebcb8b, 0.15f);
    c[VanGuiCol_UnsavedMarker]              = auroraOrange;
    c[VanGuiCol_NavCursor]                  = nord9;
    c[VanGuiCol_NavWindowingHighlight]      = HexColorA(0xeceff4, 0.70f);
    c[VanGuiCol_NavWindowingDimBg]          = HexColorA(0x434c5e, 0.20f);
    c[VanGuiCol_ModalWindowDimBg]           = HexColorA(0x434c5e, 0.35f);
    c[VanGuiCol_DockingPreview]             = HexColorA(0x81a1c1, 0.70f);
    c[VanGuiCol_DockingEmptyBg]             = nord0;
}

// ---------------------------------------------------------------------------
// Monokai theme
// Palette reference: https://monokai.pro
// ---------------------------------------------------------------------------
static void StyleColorsMonokai(VanGuiStyle* dst)
{
    VanGuiStyle* style = dst ? dst : &VanGui::GetStyle();

    const VanVec4 bg      = HexColor(0x272822);
    const VanVec4 bgLight = HexColor(0x3e3d32);
    const VanVec4 bgDark  = HexColor(0x1e1f1c);
    const VanVec4 fg      = HexColor(0xf8f8f2);
    const VanVec4 comment = HexColor(0x75715e);
    const VanVec4 green   = HexColor(0xa6e22e);
    const VanVec4 pink    = HexColor(0xf92672);
    const VanVec4 cyan    = HexColor(0x66d9e8);
    const VanVec4 orange  = HexColor(0xfd971f);
    const VanVec4 purple  = HexColor(0xae81ff);
    const VanVec4 yellow  = HexColor(0xe6db74);

    VanVec4* c = style->Colors;

    c[VanGuiCol_Text]                       = fg;
    c[VanGuiCol_TextDisabled]               = comment;
    c[VanGuiCol_WindowBg]                   = bg;
    c[VanGuiCol_ChildBg]                    = HexColorA(0x272822, 0.0f);
    c[VanGuiCol_PopupBg]                    = bgDark;
    c[VanGuiCol_Border]                     = bgLight;
    c[VanGuiCol_BorderShadow]               = HexColorA(0x000000, 0.0f);
    c[VanGuiCol_FrameBg]                    = bgLight;
    c[VanGuiCol_FrameBgHovered]             = BlendColor(bgLight, purple, 0.25f);
    c[VanGuiCol_FrameBgActive]              = BlendColor(bgLight, purple, 0.50f);
    c[VanGuiCol_TitleBg]                    = bgDark;
    c[VanGuiCol_TitleBgActive]              = BlendColor(bgDark, pink, 0.40f);
    c[VanGuiCol_TitleBgCollapsed]           = HexColorA(0x272822, 0.75f);
    c[VanGuiCol_MenuBarBg]                  = bgDark;
    c[VanGuiCol_ScrollbarBg]               = HexColorA(0x1e1f1c, 0.60f);
    c[VanGuiCol_ScrollbarGrab]              = comment;
    c[VanGuiCol_ScrollbarGrabHovered]       = purple;
    c[VanGuiCol_ScrollbarGrabActive]        = pink;
    c[VanGuiCol_CheckMark]                  = green;
    c[VanGuiCol_CheckboxSelectedBg]         = bgLight;
    c[VanGuiCol_SliderGrab]                 = purple;
    c[VanGuiCol_SliderGrabActive]           = pink;
    c[VanGuiCol_Button]                     = BlendColor(bgLight, purple, 0.30f);
    c[VanGuiCol_ButtonHovered]              = BlendColor(bgLight, pink, 0.55f);
    c[VanGuiCol_ButtonActive]               = pink;
    c[VanGuiCol_Header]                     = BlendColor(bgLight, purple, 0.25f);
    c[VanGuiCol_HeaderHovered]              = BlendColor(bgLight, purple, 0.55f);
    c[VanGuiCol_HeaderActive]               = purple;
    c[VanGuiCol_Separator]                  = comment;
    c[VanGuiCol_SeparatorHovered]           = cyan;
    c[VanGuiCol_SeparatorActive]            = cyan;
    c[VanGuiCol_ResizeGrip]                 = HexColorA(0xae81ff, 0.25f);
    c[VanGuiCol_ResizeGripHovered]          = HexColorA(0xae81ff, 0.67f);
    c[VanGuiCol_ResizeGripActive]           = pink;
    c[VanGuiCol_InputTextCursor]            = fg;
    c[VanGuiCol_TabHovered]                 = BlendColor(bgLight, purple, 0.50f);
    c[VanGuiCol_Tab]                        = BlendColor(bgDark, purple, 0.15f);
    c[VanGuiCol_TabSelected]                = BlendColor(bgLight, purple, 0.35f);
    c[VanGuiCol_TabSelectedOverline]        = pink;
    c[VanGuiCol_TabDimmed]                  = HexColorA(0x1e1f1c, 0.90f);
    c[VanGuiCol_TabDimmedSelected]          = BlendColor(bgDark, purple, 0.20f);
    c[VanGuiCol_TabDimmedSelectedOverline]  = comment;
    c[VanGuiCol_PlotLines]                  = cyan;
    c[VanGuiCol_PlotLinesHovered]           = yellow;
    c[VanGuiCol_PlotHistogram]              = orange;
    c[VanGuiCol_PlotHistogramHovered]       = yellow;
    c[VanGuiCol_TableHeaderBg]              = bgDark;
    c[VanGuiCol_TableBorderStrong]          = bgLight;
    c[VanGuiCol_TableBorderLight]           = HexColorA(0x3e3d32, 0.60f);
    c[VanGuiCol_TableRowBg]                 = HexColorA(0x000000, 0.0f);
    c[VanGuiCol_TableRowBgAlt]              = HexColorA(0xf8f8f2, 0.04f);
    c[VanGuiCol_TextLink]                   = cyan;
    c[VanGuiCol_TextSelectedBg]             = HexColorA(0xae81ff, 0.35f);
    c[VanGuiCol_TreeLines]                  = comment;
    c[VanGuiCol_DragDropTarget]             = yellow;
    c[VanGuiCol_DragDropTargetBg]           = HexColorA(0xe6db74, 0.15f);
    c[VanGuiCol_UnsavedMarker]              = orange;
    c[VanGuiCol_NavCursor]                  = purple;
    c[VanGuiCol_NavWindowingHighlight]      = HexColorA(0xf8f8f2, 0.70f);
    c[VanGuiCol_NavWindowingDimBg]          = HexColorA(0x3e3d32, 0.20f);
    c[VanGuiCol_ModalWindowDimBg]           = HexColorA(0x3e3d32, 0.35f);
    c[VanGuiCol_DockingPreview]             = HexColorA(0xae81ff, 0.70f);
    c[VanGuiCol_DockingEmptyBg]             = bgDark;
}

// ---------------------------------------------------------------------------
// Gruvbox Dark theme
// Palette reference: https://github.com/morhetz/gruvbox
// ---------------------------------------------------------------------------
static void StyleColorsGruvboxDark(VanGuiStyle* dst)
{
    VanGuiStyle* style = dst ? dst : &VanGui::GetStyle();

    // Background tones
    const VanVec4 bg0Hard  = HexColor(0x1d2021);
    const VanVec4 bg0      = HexColor(0x282828);
    const VanVec4 bg0Soft  = HexColor(0x32302f);
    const VanVec4 bg1      = HexColor(0x3c3836);
    const VanVec4 bg2      = HexColor(0x504945);
    const VanVec4 bg3      = HexColor(0x665c54);
    const VanVec4 bg4      = HexColor(0x7c6f64);
    // Foreground
    const VanVec4 fg0      = HexColor(0xfbf1c7);
    const VanVec4 fg1      = HexColor(0xebdbb2);
    const VanVec4 fg2      = HexColor(0xd5c4a1);
    const VanVec4 fg3      = HexColor(0xbdae93);
    const VanVec4 fg4      = HexColor(0xa89984);
    // Accents (bright)
    const VanVec4 brightRed    = HexColor(0xfb4934);
    const VanVec4 brightGreen  = HexColor(0xb8bb26);
    const VanVec4 brightYellow = HexColor(0xfabd2f);
    const VanVec4 brightBlue   = HexColor(0x83a598);
    const VanVec4 brightPurple = HexColor(0xd3869b);
    const VanVec4 brightAqua   = HexColor(0x8ec07c);
    const VanVec4 brightOrange = HexColor(0xfe8019);
    // Accents (normal)
    const VanVec4 normRed    = HexColor(0xcc241d);
    const VanVec4 normYellow = HexColor(0xd79921);
    const VanVec4 normBlue   = HexColor(0x458588);
    const VanVec4 normOrange = HexColor(0xd65d0e);

    VanVec4* c = style->Colors;

    c[VanGuiCol_Text]                       = fg1;
    c[VanGuiCol_TextDisabled]               = fg4;
    c[VanGuiCol_WindowBg]                   = bg0;
    c[VanGuiCol_ChildBg]                    = HexColorA(0x282828, 0.0f);
    c[VanGuiCol_PopupBg]                    = bg0Hard;
    c[VanGuiCol_Border]                     = bg2;
    c[VanGuiCol_BorderShadow]               = HexColorA(0x000000, 0.0f);
    c[VanGuiCol_FrameBg]                    = bg1;
    c[VanGuiCol_FrameBgHovered]             = bg2;
    c[VanGuiCol_FrameBgActive]              = bg3;
    c[VanGuiCol_TitleBg]                    = bg0Hard;
    c[VanGuiCol_TitleBgActive]              = normBlue;
    c[VanGuiCol_TitleBgCollapsed]           = HexColorA(0x1d2021, 0.75f);
    c[VanGuiCol_MenuBarBg]                  = bg0Soft;
    c[VanGuiCol_ScrollbarBg]                = HexColorA(0x1d2021, 0.60f);
    c[VanGuiCol_ScrollbarGrab]              = bg3;
    c[VanGuiCol_ScrollbarGrabHovered]       = normBlue;
    c[VanGuiCol_ScrollbarGrabActive]        = brightBlue;
    c[VanGuiCol_CheckMark]                  = brightGreen;
    c[VanGuiCol_CheckboxSelectedBg]         = bg1;
    c[VanGuiCol_SliderGrab]                 = normBlue;
    c[VanGuiCol_SliderGrabActive]           = brightBlue;
    c[VanGuiCol_Button]                     = BlendColor(bg1, normBlue, 0.40f);
    c[VanGuiCol_ButtonHovered]              = normBlue;
    c[VanGuiCol_ButtonActive]               = brightBlue;
    c[VanGuiCol_Header]                     = HexColorA(0x458588, 0.31f);
    c[VanGuiCol_HeaderHovered]              = HexColorA(0x458588, 0.80f);
    c[VanGuiCol_HeaderActive]               = normBlue;
    c[VanGuiCol_Separator]                  = bg3;
    c[VanGuiCol_SeparatorHovered]           = brightAqua;
    c[VanGuiCol_SeparatorActive]            = brightAqua;
    c[VanGuiCol_ResizeGrip]                 = HexColorA(0x458588, 0.20f);
    c[VanGuiCol_ResizeGripHovered]          = HexColorA(0x458588, 0.67f);
    c[VanGuiCol_ResizeGripActive]           = brightBlue;
    c[VanGuiCol_InputTextCursor]            = fg1;
    c[VanGuiCol_TabHovered]                 = normBlue;
    c[VanGuiCol_Tab]                        = bg1;
    c[VanGuiCol_TabSelected]                = BlendColor(bg1, normBlue, 0.50f);
    c[VanGuiCol_TabSelectedOverline]        = brightBlue;
    c[VanGuiCol_TabDimmed]                  = HexColorA(0x1d2021, 0.90f);
    c[VanGuiCol_TabDimmedSelected]          = HexColorA(0x3c3836, 0.80f);
    c[VanGuiCol_TabDimmedSelectedOverline]  = bg4;
    c[VanGuiCol_PlotLines]                  = brightAqua;
    c[VanGuiCol_PlotLinesHovered]           = brightYellow;
    c[VanGuiCol_PlotHistogram]              = brightOrange;
    c[VanGuiCol_PlotHistogramHovered]       = brightYellow;
    c[VanGuiCol_TableHeaderBg]              = bg0Hard;
    c[VanGuiCol_TableBorderStrong]          = bg2;
    c[VanGuiCol_TableBorderLight]           = HexColorA(0x504945, 0.60f);
    c[VanGuiCol_TableRowBg]                 = HexColorA(0x000000, 0.0f);
    c[VanGuiCol_TableRowBgAlt]              = HexColorA(0xfbf1c7, 0.04f);
    c[VanGuiCol_TextLink]                   = brightBlue;
    c[VanGuiCol_TextSelectedBg]             = HexColorA(0x458588, 0.35f);
    c[VanGuiCol_TreeLines]                  = bg4;
    c[VanGuiCol_DragDropTarget]             = brightYellow;
    c[VanGuiCol_DragDropTargetBg]           = HexColorA(0xfabd2f, 0.15f);
    c[VanGuiCol_UnsavedMarker]              = normOrange;
    c[VanGuiCol_NavCursor]                  = brightBlue;
    c[VanGuiCol_NavWindowingHighlight]      = HexColorA(0xebdbb2, 0.70f);
    c[VanGuiCol_NavWindowingDimBg]          = HexColorA(0x504945, 0.20f);
    c[VanGuiCol_ModalWindowDimBg]           = HexColorA(0x504945, 0.35f);
    c[VanGuiCol_DockingPreview]             = HexColorA(0x83a598, 0.70f);
    c[VanGuiCol_DockingEmptyBg]             = bg0Hard;
}

// ---------------------------------------------------------------------------
// Name table
// ---------------------------------------------------------------------------

struct ThemeEntry
{
    VanGui::VanThemeID id;
    const char*        name;
};

static const ThemeEntry s_ThemeTable[] =
{
    { VanGui::VanTheme_Dark,        "dark"         },
    { VanGui::VanTheme_Light,       "light"        },
    { VanGui::VanTheme_Classic,     "classic"      },
    { VanGui::VanTheme_Dracula,     "dracula"      },
    { VanGui::VanTheme_Nord,        "nord"         },
    { VanGui::VanTheme_Monokai,     "monokai"      },
    { VanGui::VanTheme_GruvboxDark, "gruvbox_dark" },
};
static const int s_ThemeTableCount = (int)(sizeof(s_ThemeTable) / sizeof(s_ThemeTable[0]));

// ---------------------------------------------------------------------------
// Serialization: color name table
// Each entry maps a VanGuiCol_ enum value to the string used in the INI file.
// ---------------------------------------------------------------------------

struct ColEntry
{
    int         col;
    const char* name;
};

static const ColEntry s_ColTable[] =
{
    { VanGuiCol_Text,                        "Text"                        },
    { VanGuiCol_TextDisabled,                "TextDisabled"                },
    { VanGuiCol_WindowBg,                    "WindowBg"                    },
    { VanGuiCol_ChildBg,                     "ChildBg"                     },
    { VanGuiCol_PopupBg,                     "PopupBg"                     },
    { VanGuiCol_Border,                      "Border"                      },
    { VanGuiCol_BorderShadow,                "BorderShadow"                },
    { VanGuiCol_FrameBg,                     "FrameBg"                     },
    { VanGuiCol_FrameBgHovered,              "FrameBgHovered"              },
    { VanGuiCol_FrameBgActive,               "FrameBgActive"               },
    { VanGuiCol_TitleBg,                     "TitleBg"                     },
    { VanGuiCol_TitleBgActive,               "TitleBgActive"               },
    { VanGuiCol_TitleBgCollapsed,            "TitleBgCollapsed"            },
    { VanGuiCol_MenuBarBg,                   "MenuBarBg"                   },
    { VanGuiCol_ScrollbarBg,                 "ScrollbarBg"                 },
    { VanGuiCol_ScrollbarGrab,               "ScrollbarGrab"               },
    { VanGuiCol_ScrollbarGrabHovered,        "ScrollbarGrabHovered"        },
    { VanGuiCol_ScrollbarGrabActive,         "ScrollbarGrabActive"         },
    { VanGuiCol_CheckMark,                   "CheckMark"                   },
    { VanGuiCol_CheckboxSelectedBg,          "CheckboxSelectedBg"          },
    { VanGuiCol_SliderGrab,                  "SliderGrab"                  },
    { VanGuiCol_SliderGrabActive,            "SliderGrabActive"            },
    { VanGuiCol_Button,                      "Button"                      },
    { VanGuiCol_ButtonHovered,               "ButtonHovered"               },
    { VanGuiCol_ButtonActive,                "ButtonActive"                },
    { VanGuiCol_Header,                      "Header"                      },
    { VanGuiCol_HeaderHovered,               "HeaderHovered"               },
    { VanGuiCol_HeaderActive,                "HeaderActive"                },
    { VanGuiCol_Separator,                   "Separator"                   },
    { VanGuiCol_SeparatorHovered,            "SeparatorHovered"            },
    { VanGuiCol_SeparatorActive,             "SeparatorActive"             },
    { VanGuiCol_ResizeGrip,                  "ResizeGrip"                  },
    { VanGuiCol_ResizeGripHovered,           "ResizeGripHovered"           },
    { VanGuiCol_ResizeGripActive,            "ResizeGripActive"            },
    { VanGuiCol_InputTextCursor,             "InputTextCursor"             },
    { VanGuiCol_TabHovered,                  "TabHovered"                  },
    { VanGuiCol_Tab,                         "Tab"                         },
    { VanGuiCol_TabSelected,                 "TabSelected"                 },
    { VanGuiCol_TabSelectedOverline,         "TabSelectedOverline"         },
    { VanGuiCol_TabDimmed,                   "TabDimmed"                   },
    { VanGuiCol_TabDimmedSelected,           "TabDimmedSelected"           },
    { VanGuiCol_TabDimmedSelectedOverline,   "TabDimmedSelectedOverline"   },
    { VanGuiCol_PlotLines,                   "PlotLines"                   },
    { VanGuiCol_PlotLinesHovered,            "PlotLinesHovered"            },
    { VanGuiCol_PlotHistogram,               "PlotHistogram"               },
    { VanGuiCol_PlotHistogramHovered,        "PlotHistogramHovered"        },
    { VanGuiCol_TableHeaderBg,               "TableHeaderBg"               },
    { VanGuiCol_TableBorderStrong,           "TableBorderStrong"           },
    { VanGuiCol_TableBorderLight,            "TableBorderLight"            },
    { VanGuiCol_TableRowBg,                  "TableRowBg"                  },
    { VanGuiCol_TableRowBgAlt,               "TableRowBgAlt"               },
    { VanGuiCol_TextLink,                    "TextLink"                    },
    { VanGuiCol_TextSelectedBg,              "TextSelectedBg"              },
    { VanGuiCol_TreeLines,                   "TreeLines"                   },
    { VanGuiCol_DragDropTarget,              "DragDropTarget"              },
    { VanGuiCol_DragDropTargetBg,            "DragDropTargetBg"            },
    { VanGuiCol_UnsavedMarker,               "UnsavedMarker"               },
    { VanGuiCol_NavCursor,                   "NavCursor"                   },
    { VanGuiCol_NavWindowingHighlight,       "NavWindowingHighlight"       },
    { VanGuiCol_NavWindowingDimBg,           "NavWindowingDimBg"           },
    { VanGuiCol_ModalWindowDimBg,            "ModalWindowDimBg"            },
    { VanGuiCol_DockingPreview,              "DockingPreview"              },
    { VanGuiCol_DockingEmptyBg,              "DockingEmptyBg"              },
};
static const int s_ColTableCount = (int)(sizeof(s_ColTable) / sizeof(s_ColTable[0]));

// ---------------------------------------------------------------------------
// Growable write buffer (stack-allocated with a fixed ceiling)
// ---------------------------------------------------------------------------

struct WriteBuffer
{
    // 64 KB is enough for any VanGuiStyle serialization.
    static const int CAPACITY = 65536;
    char  data[CAPACITY];
    int   pos;

    WriteBuffer() : pos(0) { data[0] = '\0'; }

    void AppendStr(const char* s)
    {
        while (*s && pos < CAPACITY - 1)
            data[pos++] = *s++;
        data[pos] = '\0';
    }

    void AppendFloat(const char* key, float v)
    {
        char tmp[128];
        snprintf(tmp, sizeof(tmp), "%s=%f\n", key, (double)v);
        AppendStr(tmp);
    }

    void AppendVec2(const char* key, VanVec2 v)
    {
        char tmp[128];
        snprintf(tmp, sizeof(tmp), "%s=%f,%f\n", key, (double)v.x, (double)v.y);
        AppendStr(tmp);
    }

    void AppendVec4(const char* key, VanVec4 v)
    {
        char tmp[160];
        snprintf(tmp, sizeof(tmp), "%s=%f,%f,%f,%f\n", key,
                 (double)v.x, (double)v.y, (double)v.z, (double)v.w);
        AppendStr(tmp);
    }

    void AppendInt(const char* key, int v)
    {
        char tmp[128];
        snprintf(tmp, sizeof(tmp), "%s=%d\n", key, v);
        AppendStr(tmp);
    }

    void AppendBool(const char* key, bool v)
    {
        char tmp[128];
        snprintf(tmp, sizeof(tmp), "%s=%d\n", key, v ? 1 : 0);
        AppendStr(tmp);
    }
};

} // anonymous namespace

// ---------------------------------------------------------------------------
// Public API implementation
// ---------------------------------------------------------------------------

namespace VanGui
{

void LoadTheme(VanThemeID theme)
{
    VAN_ASSERT(theme >= 0 && theme < VanTheme_COUNT);
    switch (theme)
    {
        case VanTheme_Dark:
            StyleColorsDark();
            break;
        case VanTheme_Light:
            StyleColorsLight();
            break;
        case VanTheme_Classic:
            StyleColorsClassic();
            break;
        case VanTheme_Dracula:
            StyleColorsDracula(nullptr);
            break;
        case VanTheme_Nord:
            StyleColorsNord(nullptr);
            break;
        case VanTheme_Monokai:
            StyleColorsMonokai(nullptr);
            break;
        case VanTheme_GruvboxDark:
            StyleColorsGruvboxDark(nullptr);
            break;
        default:
            VAN_ASSERT(0 && "Unknown VanThemeID");
            break;
    }
}

void LoadTheme(const char* name)
{
    VAN_ASSERT(name != nullptr);
    for (int i = 0; i < s_ThemeTableCount; ++i)
    {
        if (strcmp(s_ThemeTable[i].name, name) == 0)
        {
            LoadTheme(s_ThemeTable[i].id);
            return;
        }
    }
    // Unknown name: fall back to dark.
    VAN_ASSERT(0 && "Unknown theme name passed to VanGui::LoadTheme(const char*)");
    LoadTheme(VanTheme_Dark);
}

const char* GetThemeName(VanThemeID theme)
{
    VAN_ASSERT(theme >= 0 && theme < VanTheme_COUNT);
    for (int i = 0; i < s_ThemeTableCount; ++i)
        if (s_ThemeTable[i].id == theme)
            return s_ThemeTable[i].name;
    return "unknown";
}

// ---------------------------------------------------------------------------
// SaveThemeToMemory
// Serializes the current VanGuiStyle to an INI-like text blob.
// The caller must free the returned buffer with VAN_FREE().
// ---------------------------------------------------------------------------

char* SaveThemeToMemory(size_t* out_size)
{
    const VanGuiStyle& style = GetStyle();
    WriteBuffer wb;

    // -- [VanGuiStyle] section --
    wb.AppendStr("[VanGuiStyle]\n");

    wb.AppendFloat("Alpha",                      style.Alpha);
    wb.AppendFloat("DisabledAlpha",              style.DisabledAlpha);
    wb.AppendVec2 ("WindowPadding",              style.WindowPadding);
    wb.AppendFloat("WindowRounding",             style.WindowRounding);
    wb.AppendFloat("WindowBorderSize",           style.WindowBorderSize);
    wb.AppendFloat("WindowBorderHoverPadding",   style.WindowBorderHoverPadding);
    wb.AppendVec2 ("WindowMinSize",              style.WindowMinSize);
    wb.AppendVec2 ("WindowTitleAlign",           style.WindowTitleAlign);
    wb.AppendInt  ("WindowMenuButtonPosition",   (int)style.WindowMenuButtonPosition);
    wb.AppendFloat("ChildRounding",              style.ChildRounding);
    wb.AppendFloat("ChildBorderSize",            style.ChildBorderSize);
    wb.AppendFloat("PopupRounding",              style.PopupRounding);
    wb.AppendFloat("PopupBorderSize",            style.PopupBorderSize);
    wb.AppendVec2 ("FramePadding",               style.FramePadding);
    wb.AppendFloat("FrameRounding",              style.FrameRounding);
    wb.AppendFloat("FrameBorderSize",            style.FrameBorderSize);
    wb.AppendVec2 ("ItemSpacing",                style.ItemSpacing);
    wb.AppendVec2 ("ItemInnerSpacing",           style.ItemInnerSpacing);
    wb.AppendVec2 ("CellPadding",                style.CellPadding);
    wb.AppendVec2 ("TouchExtraPadding",          style.TouchExtraPadding);
    wb.AppendFloat("IndentSpacing",              style.IndentSpacing);
    wb.AppendFloat("ColumnsMinSpacing",          style.ColumnsMinSpacing);
    wb.AppendFloat("ScrollbarSize",              style.ScrollbarSize);
    wb.AppendFloat("ScrollbarRounding",          style.ScrollbarRounding);
    wb.AppendFloat("ScrollbarPadding",           style.ScrollbarPadding);
    wb.AppendFloat("GrabMinSize",                style.GrabMinSize);
    wb.AppendFloat("GrabRounding",               style.GrabRounding);
    wb.AppendFloat("LogSliderDeadzone",          style.LogSliderDeadzone);
    wb.AppendFloat("ImageRounding",              style.ImageRounding);
    wb.AppendFloat("ImageBorderSize",            style.ImageBorderSize);
    wb.AppendFloat("TabRounding",                style.TabRounding);
    wb.AppendFloat("TabBorderSize",              style.TabBorderSize);
    wb.AppendFloat("TabMinWidthBase",            style.TabMinWidthBase);
    wb.AppendFloat("TabMinWidthShrink",          style.TabMinWidthShrink);
    wb.AppendFloat("TabCloseButtonMinWidthSelected",   style.TabCloseButtonMinWidthSelected);
    wb.AppendFloat("TabCloseButtonMinWidthUnselected", style.TabCloseButtonMinWidthUnselected);
    wb.AppendFloat("TabBarBorderSize",           style.TabBarBorderSize);
    wb.AppendFloat("TabBarOverlineSize",         style.TabBarOverlineSize);
    wb.AppendFloat("TableAngledHeadersAngle",    style.TableAngledHeadersAngle);
    wb.AppendVec2 ("TableAngledHeadersTextAlign",style.TableAngledHeadersTextAlign);
    wb.AppendInt  ("TreeLinesFlags",             (int)style.TreeLinesFlags);
    wb.AppendFloat("TreeLinesSize",              style.TreeLinesSize);
    wb.AppendFloat("TreeLinesRounding",          style.TreeLinesRounding);
    wb.AppendFloat("DragDropTargetRounding",     style.DragDropTargetRounding);
    wb.AppendFloat("DragDropTargetBorderSize",   style.DragDropTargetBorderSize);
    wb.AppendFloat("DragDropTargetPadding",      style.DragDropTargetPadding);
    wb.AppendFloat("ColorMarkerSize",            style.ColorMarkerSize);
    wb.AppendInt  ("ColorButtonPosition",        (int)style.ColorButtonPosition);
    wb.AppendVec2 ("ButtonTextAlign",            style.ButtonTextAlign);
    wb.AppendVec2 ("SelectableTextAlign",        style.SelectableTextAlign);
    wb.AppendFloat("InputTextCursorSize",        style.InputTextCursorSize);
    wb.AppendFloat("SeparatorSize",              style.SeparatorSize);
    wb.AppendFloat("SeparatorTextBorderSize",    style.SeparatorTextBorderSize);
    wb.AppendVec2 ("SeparatorTextAlign",         style.SeparatorTextAlign);
    wb.AppendVec2 ("SeparatorTextPadding",       style.SeparatorTextPadding);
    wb.AppendVec2 ("DisplayWindowPadding",       style.DisplayWindowPadding);
    wb.AppendVec2 ("DisplaySafeAreaPadding",     style.DisplaySafeAreaPadding);
    wb.AppendFloat("MouseCursorScale",           style.MouseCursorScale);
    wb.AppendBool ("AntiAliasedLines",           style.AntiAliasedLines);
    wb.AppendBool ("AntiAliasedLinesUseTex",     style.AntiAliasedLinesUseTex);
    wb.AppendBool ("AntiAliasedFill",            style.AntiAliasedFill);
    wb.AppendFloat("CurveTessellationTol",       style.CurveTessellationTol);
    wb.AppendFloat("CircleTessellationMaxError", style.CircleTessellationMaxError);
    wb.AppendFloat("HoverStationaryDelay",       style.HoverStationaryDelay);
    wb.AppendFloat("HoverDelayShort",            style.HoverDelayShort);
    wb.AppendFloat("HoverDelayNormal",           style.HoverDelayNormal);
    wb.AppendInt  ("HoverFlagsForTooltipMouse",  (int)style.HoverFlagsForTooltipMouse);
    wb.AppendInt  ("HoverFlagsForTooltipNav",    (int)style.HoverFlagsForTooltipNav);
    wb.AppendFloat("FontSizeBase",               style.FontSizeBase);
    wb.AppendFloat("FontScaleMain",              style.FontScaleMain);

    // -- [VanGuiColors] section --
    wb.AppendStr("\n[VanGuiColors]\n");
    for (int i = 0; i < s_ColTableCount; ++i)
    {
        char key[64];
        snprintf(key, sizeof(key), "Col_%s", s_ColTable[i].name);
        wb.AppendVec4(key, style.Colors[s_ColTable[i].col]);
    }

    // Allocate a copy on the VanGui heap.
    const size_t len = (size_t)wb.pos;
    char* buf = (char*)VAN_ALLOC(len + 1);
    VAN_ASSERT(buf != nullptr);
    memcpy(buf, wb.data, len + 1);

    if (out_size)
        *out_size = len;

    return buf;
}

// ---------------------------------------------------------------------------
// LoadThemeFromMemory
// Parses the INI-like text and applies the values to the current style.
// Unknown keys are silently ignored.  Returns true on success.
// ---------------------------------------------------------------------------

bool LoadThemeFromMemory(const char* data, size_t data_size)
{
    VAN_ASSERT(data != nullptr);

    VanGuiStyle& style = GetStyle();

    enum Section { Sec_None, Sec_Style, Sec_Colors };
    Section sec = Sec_None;

    const char* p   = data;
    const char* end = data + data_size;

    while (p < end)
    {
        // Find end of line.
        const char* line_start = p;
        while (p < end && *p != '\n' && *p != '\r')
            ++p;
        const char* line_end = p;

        // Advance past line ending(s).
        if (p < end && *p == '\r') ++p;
        if (p < end && *p == '\n') ++p;

        // Skip empty lines and comments.
        if (line_start == line_end)
            continue;
        if (*line_start == ';' || *line_start == '#')
            continue;

        // Section header?
        if (*line_start == '[')
        {
            // Copy line to a null-terminated buffer for safe strcmp.
            const int line_len = (int)(line_end - line_start);
            char section_buf[64];
            const int copy_len = line_len < 63 ? line_len : 63;
            memcpy(section_buf, line_start, (size_t)copy_len);
            section_buf[copy_len] = '\0';

            if (strcmp(section_buf, "[VanGuiStyle]") == 0)
                sec = Sec_Style;
            else if (strcmp(section_buf, "[VanGuiColors]") == 0)
                sec = Sec_Colors;
            else
                sec = Sec_None;
            continue;
        }

        if (sec == Sec_None)
            continue;

        // Find '=' separator.
        const char* eq = line_start;
        while (eq < line_end && *eq != '=')
            ++eq;
        if (eq >= line_end)
            continue; // No '=' found — malformed line.

        // Extract key and value as null-terminated strings.
        const int key_len = (int)(eq - line_start);
        const int val_len = (int)(line_end - eq - 1);
        if (key_len <= 0 || val_len <= 0)
            continue;

        char key[128];
        char val[256];
        const int k = key_len < 127 ? key_len : 127;
        const int v = val_len < 255 ? val_len : 255;
        memcpy(key, line_start,  (size_t)k); key[k] = '\0';
        memcpy(val, eq + 1,      (size_t)v); val[v] = '\0';

        if (sec == Sec_Style)
        {
            // Float fields
            float  fv  = 0.0f;
            float  fx  = 0.0f, fy = 0.0f;
            int    iv  = 0;

#define PARSE_FLOAT(field) \
    if (strcmp(key, #field) == 0) { sscanf_s(val, "%f", &fv); style.field = fv; continue; }
#define PARSE_VEC2(field) \
    if (strcmp(key, #field) == 0) { sscanf_s(val, "%f,%f", &fx, &fy); style.field = VanVec2(fx, fy); continue; }
#define PARSE_INT(field) \
    if (strcmp(key, #field) == 0) { sscanf_s(val, "%d", &iv); style.field = (decltype(style.field))iv; continue; }
#define PARSE_BOOL(field) \
    if (strcmp(key, #field) == 0) { sscanf_s(val, "%d", &iv); style.field = (iv != 0); continue; }

            PARSE_FLOAT(Alpha)
            PARSE_FLOAT(DisabledAlpha)
            PARSE_VEC2 (WindowPadding)
            PARSE_FLOAT(WindowRounding)
            PARSE_FLOAT(WindowBorderSize)
            PARSE_FLOAT(WindowBorderHoverPadding)
            PARSE_VEC2 (WindowMinSize)
            PARSE_VEC2 (WindowTitleAlign)
            PARSE_INT  (WindowMenuButtonPosition)
            PARSE_FLOAT(ChildRounding)
            PARSE_FLOAT(ChildBorderSize)
            PARSE_FLOAT(PopupRounding)
            PARSE_FLOAT(PopupBorderSize)
            PARSE_VEC2 (FramePadding)
            PARSE_FLOAT(FrameRounding)
            PARSE_FLOAT(FrameBorderSize)
            PARSE_VEC2 (ItemSpacing)
            PARSE_VEC2 (ItemInnerSpacing)
            PARSE_VEC2 (CellPadding)
            PARSE_VEC2 (TouchExtraPadding)
            PARSE_FLOAT(IndentSpacing)
            PARSE_FLOAT(ColumnsMinSpacing)
            PARSE_FLOAT(ScrollbarSize)
            PARSE_FLOAT(ScrollbarRounding)
            PARSE_FLOAT(ScrollbarPadding)
            PARSE_FLOAT(GrabMinSize)
            PARSE_FLOAT(GrabRounding)
            PARSE_FLOAT(LogSliderDeadzone)
            PARSE_FLOAT(ImageRounding)
            PARSE_FLOAT(ImageBorderSize)
            PARSE_FLOAT(TabRounding)
            PARSE_FLOAT(TabBorderSize)
            PARSE_FLOAT(TabMinWidthBase)
            PARSE_FLOAT(TabMinWidthShrink)
            PARSE_FLOAT(TabCloseButtonMinWidthSelected)
            PARSE_FLOAT(TabCloseButtonMinWidthUnselected)
            PARSE_FLOAT(TabBarBorderSize)
            PARSE_FLOAT(TabBarOverlineSize)
            PARSE_FLOAT(TableAngledHeadersAngle)
            PARSE_VEC2 (TableAngledHeadersTextAlign)
            PARSE_INT  (TreeLinesFlags)
            PARSE_FLOAT(TreeLinesSize)
            PARSE_FLOAT(TreeLinesRounding)
            PARSE_FLOAT(DragDropTargetRounding)
            PARSE_FLOAT(DragDropTargetBorderSize)
            PARSE_FLOAT(DragDropTargetPadding)
            PARSE_FLOAT(ColorMarkerSize)
            PARSE_INT  (ColorButtonPosition)
            PARSE_VEC2 (ButtonTextAlign)
            PARSE_VEC2 (SelectableTextAlign)
            PARSE_FLOAT(InputTextCursorSize)
            PARSE_FLOAT(SeparatorSize)
            PARSE_FLOAT(SeparatorTextBorderSize)
            PARSE_VEC2 (SeparatorTextAlign)
            PARSE_VEC2 (SeparatorTextPadding)
            PARSE_VEC2 (DisplayWindowPadding)
            PARSE_VEC2 (DisplaySafeAreaPadding)
            PARSE_FLOAT(MouseCursorScale)
            PARSE_BOOL (AntiAliasedLines)
            PARSE_BOOL (AntiAliasedLinesUseTex)
            PARSE_BOOL (AntiAliasedFill)
            PARSE_FLOAT(CurveTessellationTol)
            PARSE_FLOAT(CircleTessellationMaxError)
            PARSE_FLOAT(HoverStationaryDelay)
            PARSE_FLOAT(HoverDelayShort)
            PARSE_FLOAT(HoverDelayNormal)
            PARSE_INT  (HoverFlagsForTooltipMouse)
            PARSE_INT  (HoverFlagsForTooltipNav)
            PARSE_FLOAT(FontSizeBase)
            PARSE_FLOAT(FontScaleMain)

#undef PARSE_FLOAT
#undef PARSE_VEC2
#undef PARSE_INT
#undef PARSE_BOOL
            // Unknown key — ignore.
        }
        else if (sec == Sec_Colors)
        {
            // Key format: "Col_<ColorName>"
            if (strncmp(key, "Col_", 4) != 0)
                continue;
            const char* col_name = key + 4;

            for (int i = 0; i < s_ColTableCount; ++i)
            {
                if (strcmp(s_ColTable[i].name, col_name) == 0)
                {
                    float r = 0.0f, g = 0.0f, b = 0.0f, a = 1.0f;
                    sscanf_s(val, "%f,%f,%f,%f", &r, &g, &b, &a);
                    style.Colors[s_ColTable[i].col] = VanVec4(r, g, b, a);
                    break;
                }
            }
            // Unknown color name — ignore.
        }
    }

    return true;
}

// ---------------------------------------------------------------------------
// File I/O wrappers
// ---------------------------------------------------------------------------

void SaveThemeToFile(const char* filename)
{
    VAN_ASSERT(filename != nullptr);

    size_t size = 0;
    char*  buf  = SaveThemeToMemory(&size);
    if (!buf)
        return;

    FILE* f = nullptr;
#if defined(_MSC_VER)
    fopen_s(&f, filename, "wb");
#else
    f = fopen(filename, "wb");
#endif
    if (f)
    {
        fwrite(buf, 1, size, f);
        fclose(f);
    }

    VAN_FREE(buf);
}

bool LoadThemeFromFile(const char* filename)
{
    VAN_ASSERT(filename != nullptr);

    FILE* f = nullptr;
#if defined(_MSC_VER)
    fopen_s(&f, filename, "rb");
#else
    f = fopen(filename, "rb");
#endif
    if (!f)
        return false;

    fseek(f, 0, SEEK_END);
    const long file_size = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (file_size <= 0)
    {
        fclose(f);
        return false;
    }

    char* buf = (char*)VAN_ALLOC((size_t)file_size + 1);
    if (!buf)
    {
        fclose(f);
        return false;
    }

    const size_t read = fread(buf, 1, (size_t)file_size, f);
    fclose(f);
    buf[read] = '\0';

    const bool ok = LoadThemeFromMemory(buf, read);
    VAN_FREE(buf);
    return ok;
}

} // namespace VanGui
