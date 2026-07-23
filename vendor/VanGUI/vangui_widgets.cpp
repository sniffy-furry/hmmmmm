// dear vangui, v1.92.9 WIP
// (widgets code)

/*

Index of this file:

// [SECTION] Forward Declarations
// [SECTION] Widgets: Text, etc.
// [SECTION] Widgets: Main (Button, Image, Checkbox, RadioButton, ProgressBar, Bullet, etc.)
// [SECTION] Widgets: Low-level Layout helpers (Spacing, Dummy, NewLine, Separator, etc.)
// [SECTION] Widgets: ComboBox
// [SECTION] Data Type and Data Formatting Helpers
// [SECTION] Widgets: DragScalar, DragFloat, DragInt, etc.
// [SECTION] Widgets: SliderScalar, SliderFloat, SliderInt, etc.
// [SECTION] Widgets: InputScalar, InputFloat, InputInt, etc.
// [SECTION] Widgets: InputText, InputTextMultiline
// [SECTION] Widgets: ColorEdit, ColorPicker, ColorButton, etc.
// [SECTION] Widgets: TreeNode, CollapsingHeader, etc.
// [SECTION] Widgets: Selectable
// [SECTION] Widgets: Typing-Select support
// [SECTION] Widgets: Box-Select support
// [SECTION] Widgets: Multi-Select support
// [SECTION] Widgets: Multi-Select helpers
// [SECTION] Widgets: ListBox
// [SECTION] Widgets: PlotLines, PlotHistogram
// [SECTION] Widgets: Value helpers
// [SECTION] Widgets: MenuItem, BeginMenu, EndMenu, etc.
// [SECTION] Widgets: BeginTabBar, EndTabBar, etc.
// [SECTION] Widgets: BeginTabItem, EndTabItem, etc.
// [SECTION] Widgets: Columns, BeginColumns, EndColumns, etc.

*/

#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#ifndef VANGUI_DEFINE_MATH_OPERATORS
#define VANGUI_DEFINE_MATH_OPERATORS
#endif

#include "vangui.h"
#ifndef VANGUI_DISABLE
#include "vangui_internal.h"

// System includes
#include <stdint.h>     // intptr_t

//-------------------------------------------------------------------------
// Warnings
//-------------------------------------------------------------------------

// Visual Studio warnings
#ifdef _MSC_VER
#pragma warning (disable: 4127)     // condition expression is constant
#pragma warning (disable: 4996)     // 'This function or variable may be unsafe': strcpy, strdup, sprintf, vsnprintf, sscanf, fopen
#if defined(_MSC_VER) && _MSC_VER >= 1922 // MSVC 2019 16.2 or later
#pragma warning (disable: 5054)     // operator '|': deprecated between enumerations of different types
#endif
#pragma warning (disable: 26451)    // [Static Analyzer] Arithmetic overflow : Using operator 'xxx' on a 4 byte value and then casting the result to a 8 byte value. Cast the value to the wider type before calling operator 'xxx' to avoid overflow(io.2).
#pragma warning (disable: 26812)    // [Static Analyzer] The enum type 'xxx' is unscoped. Prefer 'enum class' over 'enum' (Enum.3).
#endif

// Clang/GCC warnings with -Weverything
#if defined(__clang__)
#if __has_warning("-Wunknown-warning-option")
#pragma clang diagnostic ignored "-Wunknown-warning-option"         // warning: unknown warning group 'xxx'                      // not all warnings are known by all Clang versions and they tend to be rename-happy.. so ignoring warnings triggers new warnings on some configuration. Great!
#endif
#pragma clang diagnostic ignored "-Wunknown-pragmas"                // warning: unknown warning group 'xxx'
#pragma clang diagnostic ignored "-Wold-style-cast"                 // warning: use of old-style cast                            // yes, they are more terse.
#pragma clang diagnostic ignored "-Wfloat-equal"                    // warning: comparing floating point with == or != is unsafe // storing and comparing against same constants (typically 0.0f) is ok.
#pragma clang diagnostic ignored "-Wformat"                         // warning: format specifies type 'int' but the argument has type 'unsigned int'
#pragma clang diagnostic ignored "-Wformat-nonliteral"              // warning: format string is not a string literal            // passing non-literal to vsnformat(). yes, user passing incorrect format strings can crash the code.
#pragma clang diagnostic ignored "-Wsign-conversion"                // warning: implicit conversion changes signedness
#pragma clang diagnostic ignored "-Wunused-macros"                  // warning: macro is not used                                // we define snprintf/vsnprintf on Windows so they are available, but not always used.
#pragma clang diagnostic ignored "-Wzero-as-null-pointer-constant"  // warning: zero as null pointer constant                    // some standard header variations use #define nullptr 0
#pragma clang diagnostic ignored "-Wdouble-promotion"               // warning: implicit conversion from 'float' to 'double' when passing argument to function  // using printf() is a misery with this as C++ va_arg ellipsis changes float to double.
#pragma clang diagnostic ignored "-Wenum-enum-conversion"           // warning: bitwise operation between different enumeration types ('XXXFlags_' and 'XXXFlagsPrivate_')
#pragma clang diagnostic ignored "-Wdeprecated-enum-enum-conversion"// warning: bitwise operation between different enumeration types ('XXXFlags_' and 'XXXFlagsPrivate_') is deprecated
#pragma clang diagnostic ignored "-Wimplicit-int-float-conversion"  // warning: implicit conversion from 'xxx' to 'float' may lose precision
#pragma clang diagnostic ignored "-Wunsafe-buffer-usage"            // warning: 'xxx' is an unsafe pointer used for buffer access
#pragma clang diagnostic ignored "-Wnontrivial-memaccess"           // warning: first argument in call to 'memset' is a pointer to non-trivially copyable type
#pragma clang diagnostic ignored "-Wswitch-default"                 // warning: 'switch' missing 'default' label
#elif defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wpragmas"                          // warning: unknown option after '#pragma GCC diagnostic' kind
#pragma GCC diagnostic ignored "-Wfloat-equal"                      // warning: comparing floating-point with '==' or '!=' is unsafe
#pragma GCC diagnostic ignored "-Wformat"                           // warning: format '%p' expects argument of type 'int'/'void*', but argument X has type 'unsigned int'/'VanGuiWindow*'
#pragma GCC diagnostic ignored "-Wformat-nonliteral"                // warning: format not a string literal, format string not checked
#pragma GCC diagnostic ignored "-Wdeprecated-enum-enum-conversion"  // warning: bitwise operation between different enumeration types ('XXXFlags_' and 'XXXFlagsPrivate_') is deprecated
#pragma GCC diagnostic ignored "-Wdouble-promotion"                 // warning: implicit conversion from 'float' to 'double' when passing argument to function
#pragma GCC diagnostic ignored "-Wstrict-overflow"                  // warning: assuming signed overflow does not occur when simplifying division / ..when changing X +- C1 cmp C2 to X cmp C2 -+ C1
#pragma GCC diagnostic ignored "-Wclass-memaccess"                  // [__GNUC__ >= 8] warning: 'memset/memcpy' clearing/writing an object of type 'xxxx' with no trivial copy-assignment; use assignment or value-initialization instead
#pragma GCC diagnostic ignored "-Wcast-qual"                        // warning: cast from type 'const xxxx *' to type 'xxxx *' casts away qualifiers
#pragma GCC diagnostic ignored "-Wconversion"                       // warning: conversion to 'xxxx' from 'xxxx' may change value
#pragma GCC diagnostic ignored "-Wsign-conversion"                  // warning: conversion to 'xxxx' from 'xxxx' may change the sign of the result
#endif

//-------------------------------------------------------------------------
// Data
//-------------------------------------------------------------------------

// Widgets
static const float          DRAGDROP_HOLD_TO_OPEN_TIMER = 0.70f;    // Time for drag-hold to activate items accepting the VanGuiButtonFlags_PressedOnDragDropHold button behavior.
static const float          DRAG_MOUSE_THRESHOLD_FACTOR = 0.50f;    // Multiplier for the default value of io.MouseDragThreshold to make DragFloat/DragInt react faster to mouse drags.

// Those MIN/MAX values are not define because we need to point to them
static const signed char    VAN_S8_MIN  = -128;
static const signed char    VAN_S8_MAX  = 127;
static const unsigned char  VAN_U8_MIN  = 0;
static const unsigned char  VAN_U8_MAX  = 0xFF;
static const signed short   VAN_S16_MIN = -32768;
static const signed short   VAN_S16_MAX = 32767;
static const unsigned short VAN_U16_MIN = 0;
static const unsigned short VAN_U16_MAX = 0xFFFF;
static const VanS32          VAN_S32_MIN = INT_MIN;    // (-2147483647 - 1), (0x80000000);
static const VanS32          VAN_S32_MAX = INT_MAX;    // (2147483647), (0x7FFFFFFF)
static const VanU32          VAN_U32_MIN = 0;
static const VanU32          VAN_U32_MAX = UINT_MAX;   // (0xFFFFFFFF)
#ifdef LLONG_MIN
static const VanS64          VAN_S64_MIN = LLONG_MIN;  // (-9223372036854775807ll - 1ll);
static const VanS64          VAN_S64_MAX = LLONG_MAX;  // (9223372036854775807ll);
#else
static const VanS64          VAN_S64_MIN = -9223372036854775807LL - 1;
static const VanS64          VAN_S64_MAX = 9223372036854775807LL;
#endif
static const VanU64          VAN_U64_MIN = 0;
#ifdef ULLONG_MAX
static const VanU64          VAN_U64_MAX = ULLONG_MAX; // (0xFFFFFFFFFFFFFFFFull);
#else
static const VanU64          VAN_U64_MAX = (2ULL * 9223372036854775807LL + 1);
#endif

//-------------------------------------------------------------------------
// [SECTION] Forward Declarations
//-------------------------------------------------------------------------

// For InputTextEx()
static bool     InputTextFilterCharacter(VanGuiContext* ctx, VanGuiInputTextState* state, unsigned int* p_char, VanGuiInputTextCallback callback, void* user_data, bool input_source_is_clipboard = false);
static VanVec2   InputTextCalcTextSize(VanGuiContext* ctx, const char* text_begin, const char* text_end_display, const char* text_end, const char** out_remaining = nullptr, VanVec2* out_offset = nullptr, VanDrawTextFlags flags = 0);

//-------------------------------------------------------------------------
// [SECTION] Widgets: Text, etc.
//-------------------------------------------------------------------------
// - TextEx() [Internal]
// - TextUnformatted()
// - Text()
// - TextV()
// - TextColored()
// - TextColoredV()
// - TextDisabled()
// - TextDisabledV()
// - TextWrapped()
// - TextWrappedV()
// - LabelText()
// - LabelTextV()
// - BulletText()
// - BulletTextV()
//-------------------------------------------------------------------------

void VanGui::TextEx(const char* text, const char* text_end, VanGuiTextFlags flags)
{
    VanGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return;
    VanGuiContext& g = *GVanGui;

    // Accept null ranges
    if (text == text_end)
        text = text_end = "";

    // Calculate length
    const char* text_begin = text;
    if (text_end == nullptr)
        text_end = text + VanStrlen(text); // FIXME-OPT

    const VanVec2 text_pos(window->DC.CursorPos.x, window->DC.CursorPos.y + window->DC.CurrLineTextBaseOffset);
    const float wrap_pos_x = window->DC.TextWrapPos;
    const bool wrap_enabled = (wrap_pos_x >= 0.0f);
    if (text_end - text <= 2000 || wrap_enabled)
    {
        // Common case
        const float wrap_width = wrap_enabled ? CalcWrapWidthForPos(window->DC.CursorPos, wrap_pos_x) : 0.0f;
        const VanVec2 text_size = CalcTextSize(text_begin, text_end, false, wrap_width);

        VanRect bb(text_pos, text_pos + text_size);
        ItemSize(text_size, 0.0f);
        if (!ItemAdd(bb, 0))
            return;

        // Render (we don't hide text after ## in this end-user function)
        RenderTextWrapped(bb.Min, text_begin, text_end, wrap_width);
    }
    else
    {
        // Long text!
        // Perform manual coarse clipping to optimize for long multi-line text
        // - From this point we will only compute the width of lines that are visible. Optimization only available when word-wrapping is disabled.
        // - We also don't vertically center the text within the line full height, which is unlikely to matter because we are likely the biggest and only item on the line.
        // - We use memchr(), pay attention that well optimized versions of those str/mem functions are much faster than a casually written loop.
        const char* line = text;
        const float line_height = GetTextLineHeight();
        VanVec2 text_size(0, 0);

        // Lines to skip (can't skip when logging text)
        VanVec2 pos = text_pos;
        if (!g.LogEnabled)
        {
            int lines_skippable = static_cast<int>((window->ClipRect.Min.y - text_pos.y) / line_height);
            if (lines_skippable > 0)
            {
                int lines_skipped = 0;
                while (line < text_end && lines_skipped < lines_skippable)
                {
                    const char* line_end = static_cast<const char*>(VanMemchr(line, '\n', text_end - line));
                    if (!line_end)
                        line_end = text_end;
                    if ((flags & VanGuiTextFlags_NoWidthForLargeClippedText) == 0)
                        text_size.x = VanMax(text_size.x, CalcTextSize(line, line_end).x);
                    line = line_end + 1;
                    lines_skipped++;
                }
                pos.y += lines_skipped * line_height;
            }
        }

        // Lines to render
        if (line < text_end)
        {
            VanRect line_rect(pos, pos + VanVec2(FLT_MAX, line_height));
            while (line < text_end)
            {
                if (IsClippedEx(line_rect, 0))
                    break;

                const char* line_end = static_cast<const char*>(VanMemchr(line, '\n', text_end - line));
                if (!line_end)
                    line_end = text_end;
                text_size.x = VanMax(text_size.x, CalcTextSize(line, line_end).x);
                RenderText(pos, line, line_end, false);
                line = line_end + 1;
                line_rect.Min.y += line_height;
                line_rect.Max.y += line_height;
                pos.y += line_height;
            }

            // Count remaining lines
            int lines_skipped = 0;
            while (line < text_end)
            {
                const char* line_end = static_cast<const char*>(VanMemchr(line, '\n', text_end - line));
                if (!line_end)
                    line_end = text_end;
                if ((flags & VanGuiTextFlags_NoWidthForLargeClippedText) == 0)
                    text_size.x = VanMax(text_size.x, CalcTextSize(line, line_end).x);
                line = line_end + 1;
                lines_skipped++;
            }
            pos.y += lines_skipped * line_height;
        }
        text_size.y = (pos - text_pos).y;

        VanRect bb(text_pos, text_pos + text_size);
        ItemSize(text_size, 0.0f);
        ItemAdd(bb, 0);
    }
}

// Note that all functions taking format strings in the API may be passed ("%s", text) or ("%.*s", text_len, text),
// which will automatically bypass the formatter.
void VanGui::TextUnformatted(const char* text, const char* text_end)
{
    TextEx(text, text_end, VanGuiTextFlags_NoWidthForLargeClippedText);
}

void VanGui::Text(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    TextV(fmt, args);
    va_end(args);
}

void VanGui::TextV(const char* fmt, va_list args)
{
    VanGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return;

    const char* text, *text_end;
    VanFormatStringToTempBufferV(&text, &text_end, fmt, args);
    TextEx(text, text_end, VanGuiTextFlags_NoWidthForLargeClippedText);
}

void VanGui::TextColored(const VanVec4& col, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    TextColoredV(col, fmt, args);
    va_end(args);
}

void VanGui::TextColoredV(const VanVec4& col, const char* fmt, va_list args)
{
    PushStyleColor(VanGuiCol_Text, col);
    TextV(fmt, args);
    PopStyleColor();
}

void VanGui::TextDisabled(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    TextDisabledV(fmt, args);
    va_end(args);
}

void VanGui::TextDisabledV(const char* fmt, va_list args)
{
    VanGuiContext& g = *GVanGui;
    PushStyleColor(VanGuiCol_Text, g.Style.Colors[VanGuiCol_TextDisabled]);
    TextV(fmt, args);
    PopStyleColor();
}

void VanGui::TextWrapped(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    TextWrappedV(fmt, args);
    va_end(args);
}

void VanGui::TextWrappedV(const char* fmt, va_list args)
{
    VanGuiContext& g = *GVanGui;
    const bool need_backup = (g.CurrentWindow->DC.TextWrapPos < 0.0f);  // Keep existing wrap position if one is already set
    if (need_backup)
        PushTextWrapPos(0.0f);
    TextV(fmt, args);
    if (need_backup)
        PopTextWrapPos();
}

void VanGui::TextAligned(float align_x, float size_x, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    TextAlignedV(align_x, size_x, fmt, args);
    va_end(args);
}

// align_x: 0.0f = left, 0.5f = center, 1.0f = right.
// size_x : 0.0f = shortcut for GetContentRegionAvail().x
// FIXME-WIP: Works but API is likely to be reworked. This is designed for 1 item on the line. (#7024)
void VanGui::TextAlignedV(float align_x, float size_x, const char* fmt, va_list args)
{
    VanGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return;

    const char* text, *text_end;
    VanFormatStringToTempBufferV(&text, &text_end, fmt, args);
    const VanVec2 text_size = CalcTextSize(text, text_end);
    size_x = CalcItemSize(VanVec2(size_x, 0.0f), 0.0f, text_size.y).x;

    VanVec2 pos(window->DC.CursorPos.x, window->DC.CursorPos.y + window->DC.CurrLineTextBaseOffset);
    VanVec2 pos_max(pos.x + size_x, window->ClipRect.Max.y);
    VanVec2 size(VanMin(size_x, text_size.x), text_size.y);
    window->DC.CursorMaxPos.x = VanMax(window->DC.CursorMaxPos.x, pos.x + text_size.x);
    window->DC.IdealMaxPos.x = VanMax(window->DC.IdealMaxPos.x, pos.x + text_size.x);
    if (align_x > 0.0f && text_size.x < size_x)
        pos.x += VanTrunc((size_x - text_size.x) * align_x);
    RenderTextEllipsis(window->DrawList, pos, pos_max, pos_max.x, text, text_end, &text_size);

    const VanVec2 backup_max_pos = window->DC.CursorMaxPos;
    ItemSize(size);
    ItemAdd(VanRect(pos, pos + size), 0);
    window->DC.CursorMaxPos.x = backup_max_pos.x; // Cancel out extending content size because right-aligned text would otherwise mess it up.

    if (size_x < text_size.x && IsItemHovered(VanGuiHoveredFlags_NoNavOverride | VanGuiHoveredFlags_AllowWhenDisabled | VanGuiHoveredFlags_ForTooltip))
        SetTooltip("%.*s", static_cast<int>(text_end - text), text);
}

void VanGui::LabelText(const char* label, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    LabelTextV(label, fmt, args);
    va_end(args);
}

// Add a label+text combo aligned to other label+value widgets
void VanGui::LabelTextV(const char* label, const char* fmt, va_list args)
{
    VanGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return;

    VanGuiContext& g = *GVanGui;
    const VanGuiStyle& style = g.Style;
    const float w = CalcItemWidth();

    const char* value_text_begin, *value_text_end;
    VanFormatStringToTempBufferV(&value_text_begin, &value_text_end, fmt, args);
    const VanVec2 value_size = CalcTextSize(value_text_begin, value_text_end, false);
    const char* label_end = FindRenderedTextEnd(label);
    const VanVec2 label_size = CalcTextSize(label, label_end, false);

    const VanVec2 pos = window->DC.CursorPos;
    const VanRect value_bb(pos, pos + VanVec2(w, value_size.y + style.FramePadding.y * 2));
    const VanRect total_bb(pos, pos + VanVec2(w + (label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f), VanMax(value_size.y, label_size.y) + style.FramePadding.y * 2));
    ItemSize(total_bb, style.FramePadding.y);
    if (!ItemAdd(total_bb, 0))
        return;

    // Render
    RenderTextClipped(value_bb.Min + style.FramePadding, value_bb.Max, value_text_begin, value_text_end, &value_size, VanVec2(0.0f, 0.0f));
    if (label_size.x > 0.0f)
        RenderText(VanVec2(value_bb.Max.x + style.ItemInnerSpacing.x, value_bb.Min.y + style.FramePadding.y), label, label_end, false);
}

void VanGui::BulletText(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    BulletTextV(fmt, args);
    va_end(args);
}

// Text with a little bullet aligned to the typical tree node.
void VanGui::BulletTextV(const char* fmt, va_list args)
{
    VanGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return;

    VanGuiContext& g = *GVanGui;
    const VanGuiStyle& style = g.Style;

    const char* text_begin, *text_end;
    VanFormatStringToTempBufferV(&text_begin, &text_end, fmt, args);
    const VanVec2 label_size = CalcTextSize(text_begin, text_end, false);
    const VanVec2 total_size = VanVec2(g.FontSize + (label_size.x > 0.0f ? (label_size.x + style.FramePadding.x * 2) : 0.0f), label_size.y);  // Empty text doesn't add padding
    VanVec2 pos = window->DC.CursorPos;
    pos.y += window->DC.CurrLineTextBaseOffset;
    ItemSize(total_size, 0.0f);
    const VanRect bb(pos, pos + total_size);
    if (!ItemAdd(bb, 0))
        return;

    // Render
    VanU32 text_col = GetColorU32(VanGuiCol_Text);
    RenderBullet(window->DrawList, bb.Min + VanVec2(style.FramePadding.x + g.FontSize * 0.5f, g.FontSize * 0.5f), text_col);
    RenderText(bb.Min + VanVec2(g.FontSize + style.FramePadding.x * 2, 0.0f), text_begin, text_end, false);
}

//-------------------------------------------------------------------------
// [SECTION] Widgets: Main
//-------------------------------------------------------------------------
// - ButtonBehavior() [Internal]
// - Button()
// - SmallButton()
// - InvisibleButton()
// - ArrowButton()
// - CloseButton() [Internal]
// - CollapseButton() [Internal]
// - GetWindowScrollbarID() [Internal]
// - GetWindowScrollbarRect() [Internal]
// - Scrollbar() [Internal]
// - ScrollbarEx() [Internal]
// - Image()
// - ImageButton()
// - Checkbox()
// - CheckboxFlagsT() [Internal]
// - CheckboxFlags()
// - RadioButton()
// - ProgressBar()
// - Bullet()
// - Hyperlink()
//-------------------------------------------------------------------------

// The ButtonBehavior() function is key to many interactions and used by many/most widgets.
// Because we handle so many cases (keyboard/gamepad navigation, drag and drop) and many specific behavior (via VanGuiButtonFlags_),
// this code is a little complex.
// By far the most common path is interacting with the Mouse using the default VanGuiButtonFlags_PressedOnClickRelease button behavior.
// See the series of events below and the corresponding state reported by dear vangui:
//------------------------------------------------------------------------------------------------------------------------------------------------
// with PressedOnClickRelease:             return-value  IsItemHovered()  IsItemActive()  IsItemActivated()  IsItemDeactivated()  IsItemClicked()
//   Frame N+0 (mouse is outside bb)        -             -                -               -                  -                    -
//   Frame N+1 (mouse moves inside bb)      -             true             -               -                  -                    -
//   Frame N+2 (mouse button is down)       -             true             true            true               -                    true
//   Frame N+3 (mouse button is down)       -             true             true            -                  -                    -
//   Frame N+4 (mouse moves outside bb)     -             -                true            -                  -                    -
//   Frame N+5 (mouse moves inside bb)      -             true             true            -                  -                    -
//   Frame N+6 (mouse button is released)   true          true             -               -                  true                 -
//   Frame N+7 (mouse button is released)   -             true             -               -                  -                    -
//   Frame N+8 (mouse moves outside bb)     -             -                -               -                  -                    -
//------------------------------------------------------------------------------------------------------------------------------------------------
// with PressedOnClick:                    return-value  IsItemHovered()  IsItemActive()  IsItemActivated()  IsItemDeactivated()  IsItemClicked()
//   Frame N+2 (mouse button is down)       true          true             true            true               -                    true
//   Frame N+3 (mouse button is down)       -             true             true            -                  -                    -
//   Frame N+6 (mouse button is released)   -             true             -               -                  true                 -
//   Frame N+7 (mouse button is released)   -             true             -               -                  -                    -
//------------------------------------------------------------------------------------------------------------------------------------------------
// with PressedOnRelease:                  return-value  IsItemHovered()  IsItemActive()  IsItemActivated()  IsItemDeactivated()  IsItemClicked()
//   Frame N+2 (mouse button is down)       -             true             -               -                  -                    true
//   Frame N+3 (mouse button is down)       -             true             -               -                  -                    -
//   Frame N+6 (mouse button is released)   true          true             -               -                  -                    -
//   Frame N+7 (mouse button is released)   -             true             -               -                  -                    -
//------------------------------------------------------------------------------------------------------------------------------------------------
// with PressedOnDoubleClick:              return-value  IsItemHovered()  IsItemActive()  IsItemActivated()  IsItemDeactivated()  IsItemClicked()
//   Frame N+0 (mouse button is down)       -             true             -               -                  -                    true
//   Frame N+1 (mouse button is down)       -             true             -               -                  -                    -
//   Frame N+2 (mouse button is released)   -             true             -               -                  -                    -
//   Frame N+3 (mouse button is released)   -             true             -               -                  -                    -
//   Frame N+4 (mouse button is down)       true          true             true            true               -                    true
//   Frame N+5 (mouse button is down)       -             true             true            -                  -                    -
//   Frame N+6 (mouse button is released)   -             true             -               -                  true                 -
//   Frame N+7 (mouse button is released)   -             true             -               -                  -                    -
//------------------------------------------------------------------------------------------------------------------------------------------------
// Note that some combinations are supported,
// - PressedOnDragDropHold can generally be associated with any flag.
// - PressedOnDoubleClick can be associated by PressedOnClickRelease/PressedOnRelease, in which case the second release event won't be reported.
//------------------------------------------------------------------------------------------------------------------------------------------------
// The behavior of the return-value changes when VanGuiItemFlags_ButtonRepeat is set:
//                                         Repeat+                  Repeat+           Repeat+             Repeat+
//                                         PressedOnClickRelease    PressedOnClick    PressedOnRelease    PressedOnDoubleClick
//-------------------------------------------------------------------------------------------------------------------------------------------------
//   Frame N+0 (mouse button is down)       -                        true              -                   true
//   ...                                    -                        -                 -                   -
//   Frame N + RepeatDelay                  true                     true              -                   true
//   ...                                    -                        -                 -                   -
//   Frame N + RepeatDelay + RepeatRate*N   true                     true              -                   true
//-------------------------------------------------------------------------------------------------------------------------------------------------

// - FIXME: For refactor we could output flags, incl mouse hovered vs nav keyboard vs nav triggered etc.
//   And better standardize how widgets use 'GetColor32((held && hovered) ? ... : hovered ? ...)' vs 'GetColor32(held ? ... : hovered ? ...);'
//   For mouse feedback we typically prefer the 'held && hovered' test, but for nav feedback not always. Outputting hovered=true on Activation may be misleading.
// - Since v1.91.2 (Sept 2024) we included io.ConfigDebugHighlightIdConflicts feature.
//   One idiom which was previously valid which will now emit a warning is when using multiple overlaid ButtonBehavior()
//   with same ID and different MouseButton (see #8030). You can fix it by:
//       (1) switching to use a single ButtonBehavior() with multiple _MouseButton flags.
//    or (2) surrounding those calls with PushItemFlag(VanGuiItemFlags_AllowDuplicateId, true); ... PopItemFlag()
bool VanGui::ButtonBehavior(const VanRect& bb, VanGuiID id, bool* out_hovered, bool* out_held, VanGuiButtonFlags flags)
{
    VanGuiContext& g = *GVanGui;
    VanGuiWindow* window = GetCurrentWindow();

    // Default behavior inherited from item flags
    // Note that _both_ ButtonFlags and ItemFlags are valid sources, so copy one into the item_flags and only check that.
    VanGuiItemFlags item_flags = (g.LastItemData.ID == id ? g.LastItemData.ItemFlags : g.CurrentItemFlags);
    if (flags & VanGuiButtonFlags_AllowOverlap)
        item_flags |= VanGuiItemFlags_AllowOverlap;
    if (item_flags & VanGuiItemFlags_NoFocus)
        flags |= VanGuiButtonFlags_NoFocus | VanGuiButtonFlags_NoNavFocus;

    // Default only reacts to left mouse button
    if ((flags & VanGuiButtonFlags_MouseButtonMask_) == 0)
        flags |= VanGuiButtonFlags_MouseButtonLeft;

    // Default behavior requires click + release inside bounding box
    if ((flags & VanGuiButtonFlags_PressedOnMask_) == 0)
        flags |= (item_flags & VanGuiItemFlags_ButtonRepeat) ? VanGuiButtonFlags_PressedOnClick : VanGuiButtonFlags_PressedOnDefault_;

    VanGuiWindow* backup_hovered_window = g.HoveredWindow;
    const bool flatten_hovered_children = (flags & VanGuiButtonFlags_FlattenChildren) && g.HoveredWindow && g.HoveredWindow->RootWindow == window->RootWindow;
    if (flatten_hovered_children)
        g.HoveredWindow = window;

#ifdef VANGUI_ENABLE_TEST_ENGINE
    // Alternate registration spot, for when caller didn't use ItemAdd()
    if (g.LastItemData.ID != id)
        VANGUI_TEST_ENGINE_ITEM_ADD(id, bb, nullptr);
#endif

    bool pressed = false;
    bool hovered = ItemHoverable(bb, id, item_flags);

    // Special mode for Drag and Drop used by openables (tree nodes, tabs etc.)
    // where holding the button pressed for a long time while drag a payload item triggers the button.
    if (g.DragDropActive)
    {
        if ((flags & VanGuiButtonFlags_PressedOnDragDropHold) && !(g.DragDropSourceFlags & VanGuiDragDropFlags_SourceNoHoldToOpenOthers) && IsItemHovered(VanGuiHoveredFlags_AllowWhenBlockedByActiveItem))
        {
            hovered = true;
            SetHoveredID(id);
            if (g.HoveredIdTimer - g.IO.DeltaTime <= DRAGDROP_HOLD_TO_OPEN_TIMER && g.HoveredIdTimer >= DRAGDROP_HOLD_TO_OPEN_TIMER)
            {
                pressed = true;
                g.DragDropHoldJustPressedId = id;
                FocusWindow(window);
            }
        }
        if (g.DragDropAcceptIdPrev == id && (g.DragDropAcceptFlagsPrev & VanGuiDragDropFlags_AcceptDrawAsHovered))
            hovered = true;
    }

    if (flatten_hovered_children)
        g.HoveredWindow = backup_hovered_window;

    // Mouse handling
    const VanGuiID test_owner_id = (flags & VanGuiButtonFlags_NoTestKeyOwner) ? VanGuiKeyOwner_Any : id;
    if (hovered)
    {
        VAN_ASSERT(id != 0); // Lazily check inside rare path.

        // Poll mouse buttons
        // - 'mouse_button_clicked' is generally carried into ActiveIdMouseButton when setting ActiveId.
        // - Technically we only need some values in one code path, but since this is gated by hovered test this is fine.
        int mouse_button_clicked = -1;
        int mouse_button_released = -1;
        for (int button = 0; button < 3; button++)
            if (flags & (VanGuiButtonFlags_MouseButtonLeft << button)) // Handle VanGuiButtonFlags_MouseButtonRight and VanGuiButtonFlags_MouseButtonMiddle here.
            {
                if (IsMouseClicked(button, VanGuiInputFlags_None, test_owner_id) && mouse_button_clicked == -1) { mouse_button_clicked = button; }
                if (IsMouseReleased(button, test_owner_id) && mouse_button_released == -1) { mouse_button_released = button; }
            }

        // Process initial action
        const bool mods_ok = !(flags & VanGuiButtonFlags_NoKeyModsAllowed) || (!g.IO.KeyCtrl && !g.IO.KeyShift && !g.IO.KeyAlt);
        if (mods_ok)
        {
            if (mouse_button_clicked != -1 && g.ActiveId != id)
            {
                if (!(flags & VanGuiButtonFlags_NoSetKeyOwner))
                    SetKeyOwner(MouseButtonToKey(mouse_button_clicked), id);
                if (flags & (VanGuiButtonFlags_PressedOnClickRelease | VanGuiButtonFlags_PressedOnClickReleaseAnywhere))
                {
                    SetActiveID(id, window);
                    g.ActiveIdMouseButton = static_cast<VanS8>(mouse_button_clicked);
                    if (!(flags & VanGuiButtonFlags_NoNavFocus))
                    {
                        SetFocusID(id, window);
                        FocusWindow(window);
                    }
                    else if (!(flags & VanGuiButtonFlags_NoFocus))
                    {
                        FocusWindow(window, VanGuiFocusRequestFlags_RestoreFocusedChild); // Still need to focus and bring to front, but try to avoid losing NavId when navigating a child
                    }
                }
                if ((flags & VanGuiButtonFlags_PressedOnClick) || ((flags & VanGuiButtonFlags_PressedOnDoubleClick) && g.IO.MouseClickedCount[mouse_button_clicked] == 2))
                {
                    pressed = true;
                    if (flags & VanGuiButtonFlags_NoHoldingActiveId)
                        ClearActiveID();
                    else
                        SetActiveID(id, window); // Hold on ID
                    g.ActiveIdMouseButton = static_cast<VanS8>(mouse_button_clicked);
                    if (!(flags & VanGuiButtonFlags_NoNavFocus))
                    {
                        SetFocusID(id, window);
                        FocusWindow(window);
                    }
                    else if (!(flags & VanGuiButtonFlags_NoFocus))
                    {
                        FocusWindow(window, VanGuiFocusRequestFlags_RestoreFocusedChild); // Still need to focus and bring to front, but try to avoid losing NavId when navigating a child
                    }
                }
                if (flags & VanGuiButtonFlags_PressedOnRelease)
                {
                    // FIXME: Traditionally VanGuiButtonFlags_PressedOnRelease never took ActiveId. Adding it in 2026-03-20 since VanGuiButtonFlags_NoHoldingActiveId can always be added.
                    // We don't yet perform an explicit ClearActiveID() to reduce scope of change, but this possibility could be investigated.
                    if (!(flags & VanGuiButtonFlags_NoHoldingActiveId))
                        SetActiveID(id, window); // Hold on ID
                    g.ActiveIdMouseButton = static_cast<VanS8>(mouse_button_clicked);
                }
            }
            if (flags & VanGuiButtonFlags_PressedOnRelease)
            {
                if (mouse_button_released != -1)
                {
                    const bool has_repeated_at_least_once = (item_flags & VanGuiItemFlags_ButtonRepeat) && g.IO.MouseDownDurationPrev[mouse_button_released] >= g.IO.KeyRepeatDelay; // Repeat mode trumps on release behavior
                    if (!has_repeated_at_least_once)
                        pressed = true;
                    if (!(flags & VanGuiButtonFlags_NoNavFocus))
                        SetFocusID(id, window); // FIXME: Lack of FocusWindow() call here is inconsistent with other paths. Research why.
                    ClearActiveID();
                }
            }

            // 'Repeat' mode acts when held regardless of _PressedOn flags (see table above).
            // Relies on repeat logic of IsMouseClicked() but we may as well do it ourselves if we end up exposing finer RepeatDelay/RepeatRate settings.
            if (g.ActiveId == id && (item_flags & VanGuiItemFlags_ButtonRepeat))
                if (g.IO.MouseDownDuration[g.ActiveIdMouseButton] > 0.0f && IsMouseClicked(g.ActiveIdMouseButton, VanGuiInputFlags_Repeat, test_owner_id))
                    pressed = true;
        }

        if (pressed && g.IO.ConfigNavCursorVisibleAuto)
            g.NavCursorVisible = false;
    }

    // Keyboard/Gamepad navigation handling
    // We report navigated and navigation-activated items as hovered but we don't set g.HoveredId to not interfere with mouse.
    if ((item_flags & VanGuiItemFlags_Disabled) == 0)
    {
        if (g.NavId == id && g.NavCursorVisible && g.NavHighlightItemUnderNav)
            if (!(flags & VanGuiButtonFlags_NoHoveredOnFocus))
                hovered = true;
        if (g.NavActivateDownId == id)
        {
            bool nav_activated_by_code = (g.NavActivateId == id);
            bool nav_activated_by_inputs = (g.NavActivatePressedId == id);
            if (!nav_activated_by_inputs && (item_flags & VanGuiItemFlags_ButtonRepeat))
            {
                // Avoid pressing multiple keys from triggering excessive amount of repeat events
                const VanGuiKeyData* key1 = GetKeyData(VanGuiKey_Space);
                const VanGuiKeyData* key2 = GetKeyData(VanGuiKey_Enter);
                const VanGuiKeyData* key3 = GetKeyData(VanGuiKey_NavGamepadActivate);
                const float t1 = VanMax(VanMax(key1->DownDuration, key2->DownDuration), key3->DownDuration);
                nav_activated_by_inputs = CalcTypematicRepeatAmount(t1 - g.IO.DeltaTime, t1, g.IO.KeyRepeatDelay, g.IO.KeyRepeatRate) > 0;
            }
            if (nav_activated_by_code || nav_activated_by_inputs)
            {
                // Set active id so it can be queried by user via IsItemActive(), equivalent of holding the mouse button.
                pressed = true;
                SetActiveID(id, window);
                g.ActiveIdSource = g.NavInputSource;
                if (!(flags & VanGuiButtonFlags_NoNavFocus) && !(g.NavActivateFlags & VanGuiActivateFlags_FromShortcut))
                    SetFocusID(id, window);
                if (g.NavActivateFlags & VanGuiActivateFlags_FromShortcut)
                    g.ActiveIdFromShortcut = true;
            }
        }
    }

    // Process while held
    bool held = false;
    if (g.ActiveId == id)
    {
        if (g.ActiveIdSource == VanGuiInputSource_Mouse)
        {
            if (g.ActiveIdIsJustActivated)
                g.ActiveIdClickOffset = g.IO.MousePos - bb.Min;

            const int mouse_button = g.ActiveIdMouseButton;
            if (mouse_button == -1)
            {
                // Fallback for the rare situation were g.ActiveId was set programmatically or from another widget (e.g. #6304).
                ClearActiveID();
            }
            else if (IsMouseDown(mouse_button, test_owner_id))
            {
                held = true;
            }
            else
            {
                bool release_in = hovered && (flags & VanGuiButtonFlags_PressedOnClickRelease) != 0;
                bool release_anywhere = (flags & VanGuiButtonFlags_PressedOnClickReleaseAnywhere) != 0;
                if ((release_in || release_anywhere) && !g.DragDropActive)
                {
                    // Report as pressed when releasing the mouse (this is the most common path)
                    bool is_double_click_release = (flags & VanGuiButtonFlags_PressedOnDoubleClick) && g.IO.MouseReleased[mouse_button] && g.IO.MouseClickedLastCount[mouse_button] == 2;
                    bool is_repeating_already = (item_flags & VanGuiItemFlags_ButtonRepeat) && g.IO.MouseDownDurationPrev[mouse_button] >= g.IO.KeyRepeatDelay; // Repeat mode trumps <on release>
                    bool is_button_avail_or_owned = TestKeyOwner(MouseButtonToKey(mouse_button), test_owner_id);
                    if (!is_double_click_release && !is_repeating_already && is_button_avail_or_owned)
                        pressed = true;
                }
                ClearActiveID();
            }
            if (!(flags & VanGuiButtonFlags_NoNavFocus) && g.IO.ConfigNavCursorVisibleAuto)
                g.NavCursorVisible = false;
        }
        else if (g.ActiveIdSource == VanGuiInputSource_Keyboard || g.ActiveIdSource == VanGuiInputSource_Gamepad)
        {
            // When activated using Nav, we hold on the ActiveID until activation button is released
            if (g.NavActivateDownId == id)
                held = true; // hovered == true not true as we are already likely hovered on direct activation.
            else
                ClearActiveID();
        }
        if (pressed)
            g.ActiveIdHasBeenPressedBefore = true;
    }

    // Activation highlight (this may be a remote activation)
    if (g.NavHighlightActivatedId == id && (item_flags & VanGuiItemFlags_Disabled) == 0)
        hovered = true;

    if (out_hovered) *out_hovered = hovered;
    if (out_held) *out_held = held;

    return pressed;
}

bool VanGui::ButtonEx(const char* label, const VanVec2& size_arg, VanGuiButtonFlags flags)
{
    VanGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    VanGuiContext& g = *GVanGui;
    const VanGuiStyle& style = g.Style;
    const VanGuiID id = window->GetID(label);
    const char* label_end = FindRenderedTextEnd(label);
    const VanVec2 label_size = CalcTextSize(label, label_end, false);

    VanVec2 pos = window->DC.CursorPos;
    if ((flags & VanGuiButtonFlags_AlignTextBaseLine) && style.FramePadding.y < window->DC.CurrLineTextBaseOffset) // Try to vertically align buttons that are smaller/have no padding so that text baseline matches (bit hacky, since it shouldn't be a flag)
        pos.y += window->DC.CurrLineTextBaseOffset - style.FramePadding.y;
    VanVec2 size = CalcItemSize(size_arg, label_size.x + style.FramePadding.x * 2.0f, label_size.y + style.FramePadding.y * 2.0f);

    const VanRect bb(pos, pos + size);
    ItemSize(size, style.FramePadding.y);
    if (!ItemAdd(bb, id))
        return false;

    bool hovered, held;
    bool pressed = ButtonBehavior(bb, id, &hovered, &held, flags);

    // Render
    const VanU32 col = GetColorU32((held && hovered) ? VanGuiCol_ButtonActive : hovered ? VanGuiCol_ButtonHovered : VanGuiCol_Button);
    RenderNavCursor(bb, id);
    RenderFrame(bb.Min, bb.Max, col, true, style.FrameRounding);

    if (g.LogEnabled)
        LogSetNextTextDecoration("[", "]");
    RenderTextClipped(bb.Min + style.FramePadding, bb.Max - style.FramePadding, label, label_end, &label_size, style.ButtonTextAlign, &bb);

    // Automatically close popups
    //if (pressed && !(flags & VanGuiButtonFlags_DontClosePopups) && (window->Flags & VanGuiWindowFlags_Popup))
    //    CloseCurrentPopup();

    VANGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags);
    return pressed;
}

bool VanGui::Button(const char* label, const VanVec2& size_arg)
{
    return ButtonEx(label, size_arg, VanGuiButtonFlags_None);
}

// Small buttons fits within text without additional vertical spacing.
bool VanGui::SmallButton(const char* label)
{
    VanGuiContext& g = *GVanGui;
    float backup_padding_y = g.Style.FramePadding.y;
    g.Style.FramePadding.y = 0.0f;
    bool pressed = ButtonEx(label, VanVec2(0, 0), VanGuiButtonFlags_AlignTextBaseLine);
    g.Style.FramePadding.y = backup_padding_y;
    return pressed;
}

// Tip: use VanGui::PushID()/PopID() to push indices or pointers in the ID stack.
// Then you can keep 'str_id' empty or the same for all your buttons (instead of creating a string based on a non-string id)
bool VanGui::InvisibleButton(const char* str_id, const VanVec2& size_arg, VanGuiButtonFlags flags)
{
    VanGuiContext& g = *GVanGui;
    VanGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    // Ensure zero-size fits to contents
    VanVec2 size = CalcItemSize(VanVec2(size_arg.x != 0.0f ? size_arg.x : -FLT_MIN, size_arg.y != 0.0f ? size_arg.y : -FLT_MIN), 0.0f, 0.0f);

    const VanGuiID id = window->GetID(str_id);
    const VanRect bb(window->DC.CursorPos, window->DC.CursorPos + size);
    ItemSize(size);
    if (!ItemAdd(bb, id, nullptr, (flags & VanGuiButtonFlags_EnableNav) ? VanGuiItemFlags_None : VanGuiItemFlags_NoNav))
        return false;

    bool hovered, held;
    bool pressed = ButtonBehavior(bb, id, &hovered, &held, flags);
    RenderNavCursor(bb, id);

    VANGUI_TEST_ENGINE_ITEM_INFO(id, str_id, g.LastItemData.StatusFlags);
    return pressed;
}

bool VanGui::ArrowButtonEx(const char* str_id, VanGuiDir dir, VanVec2 size, VanGuiButtonFlags flags)
{
    VanGuiContext& g = *GVanGui;
    VanGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    const VanGuiID id = window->GetID(str_id);
    const VanRect bb(window->DC.CursorPos, window->DC.CursorPos + size);
    const float default_size = GetFrameHeight();
    ItemSize(size, (size.y >= default_size) ? g.Style.FramePadding.y : -1.0f);
    if (!ItemAdd(bb, id))
        return false;

    bool hovered, held;
    bool pressed = ButtonBehavior(bb, id, &hovered, &held, flags);

    // Render
    const VanU32 bg_col = GetColorU32((held && hovered) ? VanGuiCol_ButtonActive : hovered ? VanGuiCol_ButtonHovered : VanGuiCol_Button);
    const VanU32 text_col = GetColorU32(VanGuiCol_Text);
    RenderNavCursor(bb, id);
    RenderFrame(bb.Min, bb.Max, bg_col, true, g.Style.FrameRounding);
    RenderArrow(window->DrawList, bb.Min + VanVec2(VanMax(0.0f, (size.x - g.FontSize) * 0.5f), VanMax(0.0f, (size.y - g.FontSize) * 0.5f)), text_col, dir);

    VANGUI_TEST_ENGINE_ITEM_INFO(id, str_id, g.LastItemData.StatusFlags);
    return pressed;
}

bool VanGui::ArrowButton(const char* str_id, VanGuiDir dir)
{
    float sz = GetFrameHeight();
    return ArrowButtonEx(str_id, dir, VanVec2(sz, sz), VanGuiButtonFlags_None);
}

// Button to close a window
bool VanGui::CloseButton(VanGuiID id, const VanVec2& pos)
{
    VanGuiContext& g = *GVanGui;
    VanGuiWindow* window = g.CurrentWindow;

    // Tweak 1: Shrink hit-testing area if button covers an abnormally large proportion of the visible region. That's in order to facilitate moving the window away. (#3825)
    // This may better be applied as a general hit-rect reduction mechanism for all widgets to ensure the area to move window is always accessible?
    const VanRect bb(pos, pos + VanVec2(g.FontSize, g.FontSize));
    VanRect bb_interact = bb;
    const float area_to_visible_ratio = window->OuterRectClipped.GetArea() / bb.GetArea();
    if (area_to_visible_ratio < 1.5f)
        bb_interact.Expand(VanTrunc(bb_interact.GetSize() * -0.25f));

    // Tweak 2: We intentionally allow interaction when clipped so that a mechanical Alt,Right,Activate sequence can always close a window.
    // (this isn't the common behavior of buttons, but it doesn't affect the user because navigation tends to keep items visible in scrolling layer).
    bool is_clipped = !ItemAdd(bb_interact, id);

    bool hovered, held;
    bool pressed = ButtonBehavior(bb_interact, id, &hovered, &held);
    if (is_clipped)
        return pressed;

    // Render
    VanU32 bg_col = GetColorU32(held ? VanGuiCol_ButtonActive : VanGuiCol_ButtonHovered);
    if (hovered)
        window->DrawList->AddRectFilled(bb.Min, bb.Max, bg_col);
    RenderNavCursor(bb, id, VanGuiNavRenderCursorFlags_Compact);
    const VanU32 cross_col = GetColorU32(VanGuiCol_Text);
    const VanVec2 cross_center = bb.GetCenter() - VanVec2(0.5f, 0.5f);
    const float cross_extent = g.FontSize * 0.5f * 0.7071f - 1.0f;
    const float cross_thickness = 1.0f * static_cast<float>(static_cast<int>(g.Style._MainScale)); // FIXME-DPI
    window->DrawList->AddLine(cross_center + VanVec2(+cross_extent, +cross_extent), cross_center + VanVec2(-cross_extent, -cross_extent), cross_col, cross_thickness);
    window->DrawList->AddLine(cross_center + VanVec2(+cross_extent, -cross_extent), cross_center + VanVec2(-cross_extent, +cross_extent), cross_col, cross_thickness);

    return pressed;
}

bool VanGui::CollapseButton(VanGuiID id, const VanVec2& pos)
{
    VanGuiContext& g = *GVanGui;
    VanGuiWindow* window = g.CurrentWindow;

    VanRect bb(pos, pos + VanVec2(g.FontSize, g.FontSize));
    bool is_clipped = !ItemAdd(bb, id);
    bool hovered, held;
    bool pressed = ButtonBehavior(bb, id, &hovered, &held, VanGuiButtonFlags_None);
    if (is_clipped)
        return pressed;

    // Render
    VanU32 bg_col = GetColorU32((held && hovered) ? VanGuiCol_ButtonActive : hovered ? VanGuiCol_ButtonHovered : VanGuiCol_Button);
    VanU32 text_col = GetColorU32(VanGuiCol_Text);
    if (hovered || held)
        window->DrawList->AddRectFilled(bb.Min, bb.Max, bg_col);
    RenderNavCursor(bb, id, VanGuiNavRenderCursorFlags_Compact);
    RenderArrow(window->DrawList, bb.Min, text_col, window->Collapsed ? VanGuiDir_Right : VanGuiDir_Down, 1.0f);

    // Switch to moving the window after mouse is moved beyond the initial drag threshold
    if (IsItemActive() && IsMouseDragging(0))
        StartMouseMovingWindow(window);

    return pressed;
}

VanGuiID VanGui::GetWindowScrollbarID(VanGuiWindow* window, VanGuiAxis axis)
{
    return window->GetID(axis == VanGuiAxis_X ? "#SCROLLX" : "#SCROLLY");
}

// Return scrollbar rectangle, must only be called for corresponding axis if window->ScrollbarX/Y is set.
VanRect VanGui::GetWindowScrollbarRect(VanGuiWindow* window, VanGuiAxis axis)
{
    VanGuiContext& g = *GVanGui;
    const VanRect outer_rect = window->Rect();
    const VanRect inner_rect = window->InnerRect;
    const float scrollbar_size = window->ScrollbarSizes[axis ^ 1]; // (ScrollbarSizes.x = width of Y scrollbar; ScrollbarSizes.y = height of X scrollbar)
    VAN_ASSERT(scrollbar_size >= 0.0f);
    const float border_size = VAN_ROUND(window->WindowBorderSize * 0.5f);
    const float border_top = (window->Flags & VanGuiWindowFlags_MenuBar) ? VAN_ROUND(g.Style.FrameBorderSize * 0.5f) : (window->Flags & VanGuiWindowFlags_NoTitleBar) ? border_size : 0;
    if (axis == VanGuiAxis_X)
        return VanRect(inner_rect.Min.x + border_size, VanMax(outer_rect.Min.y + border_size, outer_rect.Max.y - border_size - scrollbar_size), inner_rect.Max.x - border_size, outer_rect.Max.y - border_size);
    else
        return VanRect(VanMax(outer_rect.Min.x, outer_rect.Max.x - border_size - scrollbar_size), inner_rect.Min.y + border_top, outer_rect.Max.x - border_size, inner_rect.Max.y - border_size);
}

void VanGui::ExtendHitBoxWhenNearViewportEdge(VanGuiWindow* window, VanRect* bb, float threshold, VanGuiAxis axis)
{
    VanRect window_rect = window->RootWindow->Rect();
    VanRect viewport_rect = window->Viewport->GetMainRect();
    if (window_rect.Min[axis] == viewport_rect.Min[axis] && bb->Min[axis] > window_rect.Min[axis] && bb->Min[axis] - threshold <= window_rect.Min[axis])
        bb->Min[axis] = window_rect.Min[axis];
    if (window_rect.Max[axis] == viewport_rect.Max[axis] && bb->Max[axis] < window_rect.Max[axis] && bb->Max[axis] + threshold >= window_rect.Max[axis])
        bb->Max[axis] = window_rect.Max[axis];
}

void VanGui::Scrollbar(VanGuiAxis axis)
{
    VanGuiContext& g = *GVanGui;
    VanGuiWindow* window = g.CurrentWindow;
    const VanGuiID id = GetWindowScrollbarID(window, axis);

    // Calculate scrollbar bounding box
    VanRect bb = GetWindowScrollbarRect(window, axis);
    VanDrawFlags rounding_corners = CalcRoundingFlagsForRectInRect(bb, window->Rect(), g.Style.WindowBorderSize);
    float size_visible = window->InnerRect.Max[axis] - window->InnerRect.Min[axis];
    float size_contents = window->ContentSize[axis] + window->WindowPadding[axis] * 2.0f;
    VanS64 scroll = static_cast<VanS64>(window->Scroll[axis]);
    ScrollbarEx(bb, id, axis, &scroll, static_cast<VanS64>(size_visible), static_cast<VanS64>(size_contents), rounding_corners);
    window->Scroll[axis] = static_cast<float>(scroll);
}

// Vertical/Horizontal scrollbar
// The entire piece of code below is rather confusing because:
// - We handle absolute seeking (when first clicking outside the grab) and relative manipulation (afterward or when clicking inside the grab)
// - We store values as normalized ratio and in a form that allows the window content to change while we are holding on a scrollbar
// - We handle both horizontal and vertical scrollbars, which makes the terminology not ideal.
// Still, the code should probably be made simpler..
bool VanGui::ScrollbarEx(const VanRect& bb_frame, VanGuiID id, VanGuiAxis axis, VanS64* p_scroll_v, VanS64 size_visible_v, VanS64 size_contents_v, VanDrawFlags draw_rounding_flags)
{
    VanGuiContext& g = *GVanGui;
    VanGuiWindow* window = g.CurrentWindow;
    if (window->SkipItems)
        return false;

    const float bb_frame_width = bb_frame.GetWidth();
    const float bb_frame_height = bb_frame.GetHeight();
    if (bb_frame_width <= 0.0f || bb_frame_height <= 0.0f)
        return false;

    // When we are too small, start hiding and disabling the grab (this reduce visual noise on very small window and facilitate using the window resize grab)
    float alpha = 1.0f;
    if ((axis == VanGuiAxis_Y) && bb_frame_height < bb_frame_width)
        alpha = VanSaturate(bb_frame_height / VanMax(bb_frame_width * 2.0f, 1.0f));
    if (alpha <= 0.0f)
        return false;

    const VanGuiStyle& style = g.Style;
    const bool allow_interaction = (alpha >= 1.0f);

    VanRect bb = bb_frame;
    float padding = VAN_TRUNC(VanMin(style.ScrollbarPadding, VanMin(bb_frame_width, bb_frame_height) * 0.5f));
    bb.Expand(-padding);

    // V denote the main, longer axis of the scrollbar (= height for a vertical scrollbar)
    const float scrollbar_size_v = (axis == VanGuiAxis_X) ? bb.GetWidth() : bb.GetHeight();
    if (scrollbar_size_v < 1.0f)
        return false;

    // Calculate the height of our grabbable box. It generally represent the amount visible (vs the total scrollable amount)
    // But we maintain a minimum size in pixel to allow for the user to still aim inside.
    VAN_ASSERT(VanMax(size_contents_v, size_visible_v) > 0.0f); // Adding this assert to check if the VanMax(XXX,1.0f) is still needed. PLEASE CONTACT ME if this triggers.
    const VanS64 win_size_v = VanMax(VanMax(size_contents_v, size_visible_v), static_cast<VanS64>(1));
    const float grab_h_minsize = VanMin(bb.GetSize()[axis], style.GrabMinSize);
    const float grab_h_pixels = static_cast<float>(static_cast<int>(VanClamp(scrollbar_size_v * (static_cast<float>(size_visible_v) / static_cast<float>(win_size_v)), grab_h_minsize, scrollbar_size_v)));
    const float grab_h_norm = grab_h_pixels / scrollbar_size_v;

    // As a special thing, we allow scrollbar near the edge of a screen/viewport to be reachable with mouse at the extreme edge (#9276)
    VanRect bb_hit = bb_frame;
    ExtendHitBoxWhenNearViewportEdge(window, &bb_hit, g.Style.WindowBorderSize, static_cast<VanGuiAxis>(axis ^ 1));

    // Handle input right away. None of the code of Begin() is relying on scrolling position before calling Scrollbar().
    bool held = false;
    bool hovered = false;
    ItemAdd(bb_frame, id, nullptr, VanGuiItemFlags_NoNav);
    ButtonBehavior(bb_hit, id, &hovered, &held, VanGuiButtonFlags_NoNavFocus);

    const VanS64 scroll_max = VanMax(static_cast<VanS64>(1), size_contents_v - size_visible_v);
    float scroll_ratio = VanSaturate(static_cast<float>(*p_scroll_v) / static_cast<float>(scroll_max));
    float grab_v_norm = scroll_ratio * (scrollbar_size_v - grab_h_pixels) / scrollbar_size_v; // Grab position in normalized space
    if (held && allow_interaction && grab_h_norm < 1.0f)
    {
        const float scrollbar_pos_v = bb.Min[axis];
        const float mouse_pos_v = g.IO.MousePos[axis];

        // Click position in scrollbar normalized space (0.0f->1.0f)
        const float clicked_v_norm = VanSaturate((mouse_pos_v - scrollbar_pos_v) / scrollbar_size_v);

        const int held_dir = (clicked_v_norm < grab_v_norm) ? -1 : (clicked_v_norm > grab_v_norm + grab_h_norm) ? +1 : 0;
        if (g.ActiveIdIsJustActivated)
        {
            // On initial click when held_dir == 0 (clicked over grab): calculate the distance between mouse and the center of the grab
            const bool scroll_to_clicked_location = (g.IO.ConfigScrollbarScrollByPage == false || g.IO.KeyShift || held_dir == 0);
            g.ScrollbarSeekMode = scroll_to_clicked_location ? 0 : static_cast<short>(held_dir);
            g.ScrollbarClickDeltaToGrabCenter = (held_dir == 0 && !g.IO.KeyShift) ? clicked_v_norm - grab_v_norm - grab_h_norm * 0.5f : 0.0f;
        }

        // Apply scroll (p_scroll_v will generally point on one member of window->Scroll)
        // It is ok to modify Scroll here because we are being called in Begin() after the calculation of ContentSize and before setting up our starting position
        if (g.ScrollbarSeekMode == 0)
        {
            // Absolute seeking
            const float scroll_v_norm = VanSaturate((clicked_v_norm - g.ScrollbarClickDeltaToGrabCenter - grab_h_norm * 0.5f) / (1.0f - grab_h_norm));
            *p_scroll_v = static_cast<VanS64>(scroll_v_norm * scroll_max);
        }
        else
        {
            // Page by page
            if (IsMouseClicked(VanGuiMouseButton_Left, VanGuiInputFlags_Repeat) && held_dir == g.ScrollbarSeekMode)
            {
                float page_dir = (g.ScrollbarSeekMode > 0.0f) ? +1.0f : -1.0f;
                *p_scroll_v = VanClamp(*p_scroll_v + static_cast<VanS64>(page_dir * size_visible_v), static_cast<VanS64>(0), scroll_max);
            }
        }

        // Update values for rendering
        scroll_ratio = VanSaturate(static_cast<float>(*p_scroll_v) / static_cast<float>(scroll_max));
        grab_v_norm = scroll_ratio * (scrollbar_size_v - grab_h_pixels) / scrollbar_size_v;

        // Update distance to grab now that we have seek'ed and saturated
        //if (seek_absolute)
        //    g.ScrollbarClickDeltaToGrabCenter = clicked_v_norm - grab_v_norm - grab_h_norm * 0.5f;
    }

    // Render
    const VanU32 bg_col = GetColorU32(VanGuiCol_ScrollbarBg);
    const VanU32 grab_col = GetColorU32(held ? VanGuiCol_ScrollbarGrabActive : hovered ? VanGuiCol_ScrollbarGrabHovered : VanGuiCol_ScrollbarGrab, alpha);
    window->DrawList->AddRectFilled(bb_frame.Min, bb_frame.Max, bg_col, window->WindowRounding, draw_rounding_flags);
    VanRect grab_rect;
    if (axis == VanGuiAxis_X)
        grab_rect = VanRect(VanLerp(bb.Min.x, bb.Max.x, grab_v_norm), bb.Min.y, VanLerp(bb.Min.x, bb.Max.x, grab_v_norm) + grab_h_pixels, bb.Max.y);
    else
        grab_rect = VanRect(bb.Min.x, VanLerp(bb.Min.y, bb.Max.y, grab_v_norm), bb.Max.x, VanLerp(bb.Min.y, bb.Max.y, grab_v_norm) + grab_h_pixels);
    window->DrawList->AddRectFilled(grab_rect.Min, grab_rect.Max, grab_col, style.ScrollbarRounding);

    return held;
}

// - Read about VanTextureID/VanTextureRef here: https://github.com/ocornut/vangui/wiki/Image-Loading-and-Displaying-Examples
// - 'uv0' and 'uv1' are texture coordinates. Read about them from the same link above.
void VanGui::ImageWithBg(VanTextureRef tex_ref, const VanVec2& image_size, const VanVec2& uv0, const VanVec2& uv1, const VanVec4& bg_col, const VanVec4& tint_col)
{
    VanGuiContext& g = *GVanGui;
    VanGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return;

    const VanVec2 padding(g.Style.ImageBorderSize, g.Style.ImageBorderSize);
    const VanRect bb(window->DC.CursorPos, window->DC.CursorPos + image_size + padding * 2.0f);
    ItemSize(bb);
    if (!ItemAdd(bb, 0))
        return;

    // Render
    float rounding = g.Style.ImageRounding;
    if (bg_col.w > 0.0f)
        window->DrawList->AddRectFilled(bb.Min + padding, bb.Max - padding, GetColorU32(bg_col), rounding);
    if (rounding > 0.0f)
        window->DrawList->AddImageRounded(tex_ref, bb.Min + padding, bb.Max - padding, uv0, uv1, GetColorU32(tint_col), rounding);
    else
        window->DrawList->AddImage(tex_ref, bb.Min + padding, bb.Max - padding, uv0, uv1, GetColorU32(tint_col));
    if (g.Style.ImageBorderSize > 0.0f)
        window->DrawList->AddRect(bb.Min, bb.Max, GetColorU32(VanGuiCol_Border), rounding, g.Style.ImageBorderSize);
}

void VanGui::Image(VanTextureRef tex_ref, const VanVec2& image_size, const VanVec2& uv0, const VanVec2& uv1)
{
    ImageWithBg(tex_ref, image_size, uv0, uv1);
}

// 1.91.9 (February 2025) removed 'tint_col' and 'border_col' parameters, made border size not depend on color value. (#8131, #8238)
#ifndef VANGUI_DISABLE_OBSOLETE_FUNCTIONS
void VanGui::Image(VanTextureRef tex_ref, const VanVec2& image_size, const VanVec2& uv0, const VanVec2& uv1, const VanVec4& tint_col, const VanVec4& border_col)
{
    VanGuiContext& g = *GVanGui;
    PushStyleVar(VanGuiStyleVar_ImageBorderSize, (border_col.w > 0.0f) ? VanMax(1.0f, g.Style.ImageBorderSize) : 0.0f); // Preserve legacy behavior where border is always visible when border_col's Alpha is >0.0f
    PushStyleColor(VanGuiCol_Border, border_col);
    ImageWithBg(tex_ref, image_size, uv0, uv1, VanVec4(0, 0, 0, 0), tint_col);
    PopStyleColor();
    PopStyleVar();
}
#endif

bool VanGui::ImageButtonEx(VanGuiID id, VanTextureRef tex_ref, const VanVec2& image_size, const VanVec2& uv0, const VanVec2& uv1, const VanVec4& bg_col, const VanVec4& tint_col, VanGuiButtonFlags flags)
{
    VanGuiContext& g = *GVanGui;
    VanGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    const VanVec2 padding = g.Style.FramePadding;
    const VanRect bb(window->DC.CursorPos, window->DC.CursorPos + image_size + padding * 2.0f);
    ItemSize(bb);
    if (!ItemAdd(bb, id))
        return false;

    bool hovered, held;
    bool pressed = ButtonBehavior(bb, id, &hovered, &held, flags);

    // Render
    const VanU32 col = GetColorU32((held && hovered) ? VanGuiCol_ButtonActive : hovered ? VanGuiCol_ButtonHovered : VanGuiCol_Button);
    RenderNavCursor(bb, id);
    RenderFrame(bb.Min, bb.Max, col, true, g.Style.FrameRounding);
    if (bg_col.w > 0.0f)
        window->DrawList->AddRectFilled(bb.Min + padding, bb.Max - padding, GetColorU32(bg_col));
    float image_rounding = VanMax(g.Style.FrameRounding - VanMax(padding.x, padding.y), g.Style.ImageRounding);
    if (image_rounding > 0.0f)
        window->DrawList->AddImageRounded(tex_ref, bb.Min + padding, bb.Max - padding, uv0, uv1, GetColorU32(tint_col), image_rounding);
    else
        window->DrawList->AddImage(tex_ref, bb.Min + padding, bb.Max - padding, uv0, uv1, GetColorU32(tint_col));

    return pressed;
}

// - ImageButton() adds style.FramePadding*2.0f to provided size. This is in order to facilitate fitting an image in a button.
// - ImageButton() draws a background based on regular Button() color + optionally an inner background if specified. (#8165) // FIXME: Maybe that's not the best design?
bool VanGui::ImageButton(const char* str_id, VanTextureRef tex_ref, const VanVec2& image_size, const VanVec2& uv0, const VanVec2& uv1, const VanVec4& bg_col, const VanVec4& tint_col)
{
    VanGuiContext& g = *GVanGui;
    VanGuiWindow* window = g.CurrentWindow;
    if (window->SkipItems)
        return false;

    return ImageButtonEx(window->GetID(str_id), tex_ref, image_size, uv0, uv1, bg_col, tint_col);
}

#ifndef VANGUI_DISABLE_OBSOLETE_FUNCTIONS
// Legacy API obsoleted in 1.89. Two differences with new ImageButton()
// - old ImageButton() used VanTextureID as item id (created issue with multiple buttons with same image, transient texture id values, opaque computation of ID)
// - new ImageButton() requires an explicit 'const char* str_id'
// - old ImageButton() had frame_padding' override argument.
// - new ImageButton() always use style.FramePadding.
/*
bool VanGui::ImageButton(VanTextureID user_texture_id, const VanVec2& size, const VanVec2& uv0, const VanVec2& uv1, int frame_padding, const VanVec4& bg_col, const VanVec4& tint_col)
{
    // Default to using texture ID as ID. User can still push string/integer prefixes.
    PushID((VanTextureID)(intptr_t)user_texture_id);
    if (frame_padding >= 0)
        PushStyleVar(VanGuiStyleVar_FramePadding, VanVec2(static_cast<float>(frame_padding), static_cast<float>(frame_padding)));
    bool ret = ImageButton("", user_texture_id, size, uv0, uv1, bg_col, tint_col);
    if (frame_padding >= 0)
        PopStyleVar();
    PopID();
    return ret;
}
*/
#endif // #ifndef VANGUI_DISABLE_OBSOLETE_FUNCTIONS

bool VanGui::Checkbox(const char* label, bool* v)
{
    VanGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    VanGuiContext& g = *GVanGui;
    const VanGuiStyle& style = g.Style;
    const VanGuiID id = window->GetID(label);
    const char* label_end = FindRenderedTextEnd(label);
    const VanVec2 label_size = CalcTextSize(label, label_end, false);

    const float square_sz = GetFrameHeight();
    const VanVec2 pos = window->DC.CursorPos;
    const VanRect total_bb(pos, pos + VanVec2(square_sz + (label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f), label_size.y + style.FramePadding.y * 2.0f));
    ItemSize(total_bb, style.FramePadding.y);
    const bool is_visible = ItemAdd(total_bb, id);
    const bool is_multi_select = (g.LastItemData.ItemFlags & VanGuiItemFlags_IsMultiSelect) != 0;
    if (!is_visible)
        if (!is_multi_select || !g.BoxSelectState.UnclipMode || !g.BoxSelectState.UnclipRect.Overlaps(total_bb)) // Extra layer of "no logic clip" for box-select support
        {
            VANGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags | VanGuiItemStatusFlags_Checkable | (*v ? VanGuiItemStatusFlags_Checked : 0));
            return false;
        }

    // Range-Selection/Multi-selection support (header)
    bool checked = *v;
    if (is_multi_select)
        MultiSelectItemHeader(id, &checked, nullptr);

    bool hovered, held;
    bool pressed = ButtonBehavior(total_bb, id, &hovered, &held);

    // Range-Selection/Multi-selection support (footer)
    if (is_multi_select)
        MultiSelectItemFooter(id, &checked, &pressed);
    else if (pressed)
        checked = !checked;

    if (*v != checked)
    {
        *v = checked;
        pressed = true; // return value
        MarkItemEdited(id);
    }

    const VanRect check_bb(pos, pos + VanVec2(square_sz, square_sz));
    const bool mixed_value = (g.LastItemData.ItemFlags & VanGuiItemFlags_MixedValue) != 0;
    if (is_visible)
    {
        RenderNavCursor(total_bb, id);
        VanU32 bg_col = GetColorU32((held && hovered) ? VanGuiCol_FrameBgActive : hovered ? VanGuiCol_FrameBgHovered : (mixed_value || checked) ? VanGuiCol_CheckboxSelectedBg : VanGuiCol_FrameBg);
        VanU32 check_col = GetColorU32(VanGuiCol_CheckMark);
        RenderFrame(check_bb.Min, check_bb.Max, bg_col, true, style.FrameRounding);
        if (mixed_value)
        {
            // Undocumented tristate/mixed/indeterminate checkbox (#2644)
            // This may seem awkwardly designed because the aim is to make VanGuiItemFlags_MixedValue supported by all widgets (not just checkbox)
            VanVec2 pad(VanMax(1.0f, VAN_TRUNC(square_sz / 3.6f)), VanMax(1.0f, VAN_TRUNC(square_sz / 3.6f)));
            window->DrawList->AddRectFilled(check_bb.Min + pad, check_bb.Max - pad, check_col, style.FrameRounding);
        }
        else if (*v)
        {
            const float pad = VanMax(1.0f, VAN_TRUNC(square_sz / 6.0f));
            RenderCheckMark(window->DrawList, check_bb.Min + VanVec2(pad, pad), check_col, square_sz - pad * 2.0f);
        }
    }
    const VanVec2 label_pos = VanVec2(check_bb.Max.x + style.ItemInnerSpacing.x, check_bb.Min.y + style.FramePadding.y);
    if (g.LogEnabled)
        LogRenderedText(&label_pos, mixed_value ? "[~]" : *v ? "[x]" : "[ ]");
    if (is_visible && label_size.x > 0.0f)
        RenderText(label_pos, label, label_end, false);

    VANGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags | VanGuiItemStatusFlags_Checkable | (*v ? VanGuiItemStatusFlags_Checked : 0));
    return pressed;
}

template<typename T>
bool VanGui::CheckboxFlagsT(const char* label, T* flags, T flags_value)
{
    bool all_on = (*flags & flags_value) == flags_value;
    bool any_on = (*flags & flags_value) != 0;
    bool pressed;
    if (!all_on && any_on)
    {
        VanGuiContext& g = *GVanGui;
        g.NextItemData.ItemFlags |= VanGuiItemFlags_MixedValue;
        pressed = Checkbox(label, &all_on);
    }
    else
    {
        pressed = Checkbox(label, &all_on);

    }
    if (pressed)
    {
        if (all_on)
            *flags |= flags_value;
        else
            *flags &= ~flags_value;
    }
    return pressed;
}

bool VanGui::CheckboxFlags(const char* label, int* flags, int flags_value)
{
    return CheckboxFlagsT(label, flags, flags_value);
}

bool VanGui::CheckboxFlags(const char* label, unsigned int* flags, unsigned int flags_value)
{
    return CheckboxFlagsT(label, flags, flags_value);
}

bool VanGui::CheckboxFlags(const char* label, VanS64* flags, VanS64 flags_value)
{
    return CheckboxFlagsT(label, flags, flags_value);
}

bool VanGui::CheckboxFlags(const char* label, VanU64* flags, VanU64 flags_value)
{
    return CheckboxFlagsT(label, flags, flags_value);
}

bool VanGui::RadioButton(const char* label, bool active)
{
    VanGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    VanGuiContext& g = *GVanGui;
    const VanGuiStyle& style = g.Style;
    const VanGuiID id = window->GetID(label);
    const char* label_end = FindRenderedTextEnd(label);
    const VanVec2 label_size = CalcTextSize(label, label_end, false);

    const float square_sz = GetFrameHeight();
    const VanVec2 pos = window->DC.CursorPos;
    const VanRect check_bb(pos, pos + VanVec2(square_sz, square_sz));
    const VanRect total_bb(pos, pos + VanVec2(square_sz + (label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f), label_size.y + style.FramePadding.y * 2.0f));
    ItemSize(total_bb, style.FramePadding.y);
    if (!ItemAdd(total_bb, id))
        return false;

    VanVec2 center = check_bb.GetCenter();
    center.x = VAN_ROUND(center.x);
    center.y = VAN_ROUND(center.y);
    const float radius = (square_sz - 1.0f) * 0.5f;

    bool hovered, held;
    bool pressed = ButtonBehavior(total_bb, id, &hovered, &held);
    if (pressed)
        MarkItemEdited(id);

    RenderNavCursor(total_bb, id);
    const int num_segment = window->DrawList->_CalcCircleAutoSegmentCount(radius);
    window->DrawList->AddCircleFilled(center, radius, GetColorU32((held && hovered) ? VanGuiCol_FrameBgActive : hovered ? VanGuiCol_FrameBgHovered : VanGuiCol_FrameBg), num_segment);
    if (active)
    {
        const float pad = VanMax(1.0f, VAN_TRUNC(square_sz / 6.0f));
        window->DrawList->AddCircleFilled(center, radius - pad, GetColorU32(VanGuiCol_CheckMark));
    }

    if (style.FrameBorderSize > 0.0f)
    {
        window->DrawList->AddCircle(center + VanVec2(1, 1), radius, GetColorU32(VanGuiCol_BorderShadow), num_segment, style.FrameBorderSize);
        window->DrawList->AddCircle(center, radius, GetColorU32(VanGuiCol_Border), num_segment, style.FrameBorderSize);
    }

    VanVec2 label_pos = VanVec2(check_bb.Max.x + style.ItemInnerSpacing.x, check_bb.Min.y + style.FramePadding.y);
    if (g.LogEnabled)
        LogRenderedText(&label_pos, active ? "(x)" : "( )");
    if (label_size.x > 0.0f)
        RenderText(label_pos, label, label_end, false);

    VANGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags);
    return pressed;
}

// FIXME: This would work nicely if it was a public template, e.g. 'template<T> RadioButton(const char* label, T* v, T v_button)', but I'm not sure how we would expose it..
bool VanGui::RadioButton(const char* label, int* v, int v_button)
{
    const bool pressed = RadioButton(label, *v == v_button);
    if (pressed)
        *v = v_button;
    return pressed;
}

// size_arg (for each axis) < 0.0f: align to end, 0.0f: auto, > 0.0f: specified size
void VanGui::ProgressBar(float fraction, const VanVec2& size_arg, const char* overlay)
{
    VanGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return;

    VanGuiContext& g = *GVanGui;
    const VanGuiStyle& style = g.Style;

    VanVec2 pos = window->DC.CursorPos;
    VanVec2 size = CalcItemSize(size_arg, CalcItemWidth(), g.FontSize + style.FramePadding.y * 2.0f);
    VanRect bb(pos, pos + size);
    ItemSize(size, style.FramePadding.y);
    if (!ItemAdd(bb, 0))
        return;

    // Fraction < 0.0f will display an indeterminate progress bar animation
    // The value must be animated along with time, so e.g. passing '-1.0f * VanGui::GetTime()' as fraction works.
    const bool is_indeterminate = (fraction < 0.0f);
    if (!is_indeterminate)
        fraction = VanSaturate(fraction);

    // Out of courtesy we accept a NaN fraction without crashing
    float fill_n0 = 0.0f;
    float fill_n1 = (fraction == fraction) ? fraction : 0.0f;

    if (is_indeterminate)
    {
        const float fill_width_n = 0.2f;
        fill_n0 = VanFmod(-fraction, 1.0f) * (1.0f + fill_width_n) - fill_width_n;
        fill_n1 = VanSaturate(fill_n0 + fill_width_n);
        fill_n0 = VanSaturate(fill_n0);
    }

    // Render
    RenderFrame(bb.Min, bb.Max, GetColorU32(VanGuiCol_FrameBg), true, style.FrameRounding);
    bb.Expand(VanVec2(-style.FrameBorderSize, -style.FrameBorderSize));
    float fill_x0 = VanLerp(bb.Min.x, bb.Max.x, fill_n0);
    float fill_x1 = VanLerp(bb.Min.x, bb.Max.x, fill_n1);
    if (fill_x0 < fill_x1)
        RenderRectFilledInRangeH(window->DrawList, bb, GetColorU32(VanGuiCol_PlotHistogram), fill_x0, fill_x1, style.FrameRounding);

    // Default displaying the fraction as percentage string, but user can override it
    // Don't display text for indeterminate bars by default
    char overlay_buf[32];
    if (!is_indeterminate || overlay != nullptr)
    {
        if (!overlay)
        {
            VanFormatString(overlay_buf, VAN_COUNTOF(overlay_buf), "%.0f%%", fraction * 100 + 0.01f);
            overlay = overlay_buf;
        }

        VanVec2 overlay_size = CalcTextSize(overlay, nullptr);
        if (overlay_size.x > 0.0f)
        {
            float text_x = is_indeterminate ? (bb.Min.x + bb.Max.x - overlay_size.x) * 0.5f : fill_x1 + style.ItemSpacing.x;
            RenderTextClipped(VanVec2(VanClamp(text_x, bb.Min.x, bb.Max.x - overlay_size.x - style.ItemInnerSpacing.x), bb.Min.y), bb.Max, overlay, nullptr, &overlay_size, VanVec2(0.0f, 0.5f), &bb);
        }
    }
}

void VanGui::Bullet()
{
    VanGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return;

    VanGuiContext& g = *GVanGui;
    const VanGuiStyle& style = g.Style;
    const float line_height = VanMax(VanMin(window->DC.CurrLineSize.y, g.FontSize + style.FramePadding.y * 2), g.FontSize);
    const VanRect bb(window->DC.CursorPos, window->DC.CursorPos + VanVec2(g.FontSize, line_height));
    ItemSize(bb);
    if (!ItemAdd(bb, 0))
    {
        SameLine(0, style.FramePadding.x * 2);
        return;
    }

    // Render and stay on same line
    VanU32 text_col = GetColorU32(VanGuiCol_Text);
    RenderBullet(window->DrawList, bb.Min + VanVec2(style.FramePadding.x + g.FontSize * 0.5f, line_height * 0.5f), text_col);
    SameLine(0, style.FramePadding.x * 2.0f);
}

// This is provided as a convenience for being an often requested feature.
// FIXME-STYLE: we delayed adding as there is a larger plan to revamp the styling system.
// Because of this we currently don't provide many styling options for this widget
// (e.g. hovered/active colors are automatically inferred from a single color).
bool VanGui::TextLink(const char* label)
{
    VanGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    VanGuiContext& g = *GVanGui;
    const VanGuiID id = window->GetID(label);
    const char* label_end = FindRenderedTextEnd(label);

    VanVec2 pos(window->DC.CursorPos.x, window->DC.CursorPos.y + window->DC.CurrLineTextBaseOffset);
    VanVec2 size = CalcTextSize(label, label_end, false);
    VanRect bb(pos, pos + size);
    ItemSize(size, 0.0f);
    if (!ItemAdd(bb, id))
        return false;

    bool hovered, held;
    bool pressed = ButtonBehavior(bb, id, &hovered, &held);
    RenderNavCursor(bb, id);

    if (hovered)
        SetMouseCursor(VanGuiMouseCursor_Hand);

    VanVec4 text_colf = g.Style.Colors[VanGuiCol_TextLink];
    VanVec4 line_colf = text_colf;
    {
        // FIXME-STYLE: Read comments above. This widget is NOT written in the same style as some earlier widgets,
        // as we are currently experimenting/planning a different styling system.
        float h, s, v;
        ColorConvertRGBtoHSV(text_colf.x, text_colf.y, text_colf.z, h, s, v);
        if (held || hovered)
        {
            v = VanSaturate(v + (held ? 0.4f : 0.3f));
            h = VanFmod(h + 0.02f, 1.0f);
        }
        ColorConvertHSVtoRGB(h, s, v, text_colf.x, text_colf.y, text_colf.z);
        v = VanSaturate(v - 0.20f);
        ColorConvertHSVtoRGB(h, s, v, line_colf.x, line_colf.y, line_colf.z);
    }

    float line_y = bb.Max.y + VanFloor(g.FontBaked->Descent * g.FontBakedScale * 0.20f);
    window->DrawList->AddLineH(bb.Min.x, bb.Max.x, line_y, GetColorU32(line_colf), 1.0f * static_cast<float>(static_cast<int>(g.Style._MainScale))); // FIXME-TEXT: Underline mode // FIXME-DPI

    PushStyleColor(VanGuiCol_Text, GetColorU32(text_colf));
    RenderText(bb.Min, label, label_end, false);
    PopStyleColor();

    VANGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags);
    return pressed;
}

bool VanGui::TextLinkOpenURL(const char* label, const char* url)
{
    VanGuiContext& g = *GVanGui;
    if (url == nullptr)
        url = label;
    bool pressed = TextLink(label);
    if (pressed && g.PlatformIO.Platform_OpenInShellFn != nullptr)
        g.PlatformIO.Platform_OpenInShellFn(&g, url);
    SetItemTooltip(LocalizeGetMsg(VanGuiLocKey_OpenLink_s), url); // It is more reassuring for user to _always_ display URL when we same as label
    if (BeginPopupContextItem())
    {
        if (MenuItem(LocalizeGetMsg(VanGuiLocKey_CopyLink)))
            SetClipboardText(url);
        EndPopup();
    }
    return pressed;
}

//-------------------------------------------------------------------------
// [SECTION] Widgets: Low-level Layout helpers
//-------------------------------------------------------------------------
// - Spacing()
// - Dummy()
// - NewLine()
// - AlignTextToFramePadding()
// - SeparatorEx() [Internal]
// - Separator()
// - SplitterBehavior() [Internal]
// - ShrinkWidths() [Internal]
//-------------------------------------------------------------------------

void VanGui::Spacing()
{
    VanGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return;
    ItemSize(VanVec2(0, 0));
}

void VanGui::Dummy(const VanVec2& size)
{
    VanGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return;

    const VanRect bb(window->DC.CursorPos, window->DC.CursorPos + size);
    ItemSize(size);
    ItemAdd(bb, 0);
}

void VanGui::NewLine()
{
    VanGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return;

    VanGuiContext& g = *GVanGui;
    const VanGuiLayoutType backup_layout_type = window->DC.LayoutType;
    window->DC.LayoutType = VanGuiLayoutType_Vertical;
    window->DC.IsSameLine = false;
    if (window->DC.CurrLineSize.y > 0.0f)     // In the event that we are on a line with items that is smaller that FontSize high, we will preserve its height.
        ItemSize(VanVec2(0, 0));
    else
        ItemSize(VanVec2(0.0f, g.FontSize));
    window->DC.LayoutType = backup_layout_type;
}

void VanGui::AlignTextToFramePadding()
{
    VanGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return;

    VanGuiContext& g = *GVanGui;
    window->DC.CurrLineSize.y = VanMax(window->DC.CurrLineSize.y, g.FontSize + g.Style.FramePadding.y * 2);
    window->DC.CurrLineTextBaseOffset = VanMax(window->DC.CurrLineTextBaseOffset, g.Style.FramePadding.y);
}

// Horizontal/vertical separating line
// FIXME: Surprisingly, this seemingly trivial widget is a victim of many different legacy/tricky layout issues.
// Note how thickness == 1.0f is handled specifically as not moving CursorPos by 'thickness', but other values are.
void VanGui::SeparatorEx(VanGuiSeparatorFlags flags, float thickness)
{
    VanGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return;

    VanGuiContext& g = *GVanGui;
    VAN_ASSERT(VanIsPowerOfTwo(flags & (VanGuiSeparatorFlags_Horizontal | VanGuiSeparatorFlags_Vertical)));   // Check that only 1 option is selected
    VAN_ASSERT(thickness > 0.0f);

    if (flags & VanGuiSeparatorFlags_Vertical)
    {
        // Vertical separator, for menu bars (use current line height).
        float y1 = window->DC.CursorPos.y;
        float y2 = window->DC.CursorPos.y + window->DC.CurrLineSize.y;
        const VanRect bb(VanVec2(window->DC.CursorPos.x, y1), VanVec2(window->DC.CursorPos.x + thickness, y2));
        ItemSize(VanVec2(thickness, 0.0f));
        if (!ItemAdd(bb, 0))
            return;

        // Draw
        window->DrawList->AddRectFilled(bb.Min, bb.Max, GetColorU32(VanGuiCol_Separator));
        if (g.LogEnabled)
            LogText(" |");
    }
    else if (flags & VanGuiSeparatorFlags_Horizontal)
    {
        // Horizontal Separator
        float x1 = window->DC.CursorPos.x;
        float x2 = window->WorkRect.Max.x;

        // Preserve legacy behavior inside Columns()
        // Before Tables API happened, we relied on Separator() to span all columns of a Columns() set.
        // We currently don't need to provide the same feature for tables because tables naturally have border features.
        VanGuiOldColumns* columns = (flags & VanGuiSeparatorFlags_SpanAllColumns) ? window->DC.CurrentColumns : nullptr;
        if (columns)
        {
            x1 = window->Pos.x + window->DC.Indent.x; // Used to be Pos.x before 2023/10/03
            x2 = window->Pos.x + window->Size.x;
            PushColumnsBackground();
        }

        // We don't provide our width to the layout so that it doesn't get feed back into AutoFit
        // FIXME: This prevents ->CursorMaxPos based bounding box evaluation from working (e.g. TableEndCell)

        // Between 1.71 and 1.92.7, we maintained a hack where a 1.0f thin Separator() would not impact layout.
        // This was mostly chosen to allow backward compatibility with user's code assuming zero-height when calculating height for layout (e.g. bottom alignment of a status bar).
        // In order to handle scaling we need to scale separator thickness and it would not makes sense to have a disparity depending on height.
        ////float thickness_for_layout = (thickness == 1.0f) ? 0.0f : thickness; // FIXME: See 1.70/1.71 Separator() change: makes legacy 1-px separator not affect layout yet. Should change.
        const VanRect bb(VanVec2(x1, window->DC.CursorPos.y), VanVec2(x2, window->DC.CursorPos.y + thickness));
        ItemSize(VanVec2(0.0f, thickness));

        if (ItemAdd(bb, 0))
        {
            // Draw
            window->DrawList->AddRectFilled(bb.Min, bb.Max, GetColorU32(VanGuiCol_Separator));
            if (g.LogEnabled)
                LogRenderedText(&bb.Min, "--------------------------------\n");

        }
        if (columns)
        {
            PopColumnsBackground();
            columns->LineMinY = window->DC.CursorPos.y;
        }
    }
}

void VanGui::Separator()
{
    VanGuiContext& g = *GVanGui;
    VanGuiWindow* window = g.CurrentWindow;
    if (window->SkipItems)
        return;

    // Those flags should eventually be configurable by the user
    VanGuiSeparatorFlags flags = (window->DC.LayoutType == VanGuiLayoutType_Horizontal) ? VanGuiSeparatorFlags_Vertical : VanGuiSeparatorFlags_Horizontal;

    // Only applies to legacy Columns() api as they relied on Separator() a lot.
    if (window->DC.CurrentColumns)
        flags |= VanGuiSeparatorFlags_SpanAllColumns;

    SeparatorEx(flags, VanMax(g.Style.SeparatorSize, 1.0f));
}

void VanGui::SeparatorTextEx(VanGuiID id, const char* label, const char* label_end, float extra_w)
{
    VanGuiContext& g = *GVanGui;
    VanGuiWindow* window = g.CurrentWindow;
    VanGuiStyle& style = g.Style;

    const VanVec2 label_size = CalcTextSize(label, label_end, false);
    const VanVec2 pos = window->DC.CursorPos;
    const VanVec2 padding = style.SeparatorTextPadding;

    const float separator_thickness = style.SeparatorTextBorderSize;
    const VanVec2 min_size(label_size.x + extra_w + padding.x * 2.0f, VanMax(label_size.y + padding.y * 2.0f, separator_thickness));
    const VanRect bb(pos, VanVec2(window->WorkRect.Max.x, pos.y + min_size.y));
    const float text_baseline_y = VanTrunc((bb.GetHeight() - label_size.y) * style.SeparatorTextAlign.y + 0.99999f); //VanMax(padding.y, VanTrunc((style.SeparatorTextSize - label_size.y) * 0.5f));
    ItemSize(min_size, text_baseline_y);
    if (!ItemAdd(bb, id))
        return;

    const float sep1_x1 = pos.x;
    const float sep2_x2 = bb.Max.x;
    const float seps_y = VanTrunc((bb.Min.y + bb.Max.y) * 0.5f + 0.99999f);

    const float label_avail_w = VanMax(0.0f, sep2_x2 - sep1_x1 - padding.x * 2.0f);
    const VanVec2 label_pos(pos.x + padding.x + VanMax(0.0f, (label_avail_w - label_size.x - extra_w) * style.SeparatorTextAlign.x), pos.y + text_baseline_y); // FIXME-ALIGN

    // This allows using SameLine() to position something in the 'extra_w'
    window->DC.CursorPosPrevLine.x = label_pos.x + label_size.x;

    const VanU32 separator_col = GetColorU32(VanGuiCol_Separator);
    if (label_size.x > 0.0f)
    {
        const float sep1_x2 = label_pos.x - style.ItemSpacing.x;
        const float sep2_x1 = label_pos.x + label_size.x + extra_w + style.ItemSpacing.x;
        if (sep1_x2 > sep1_x1 && separator_thickness > 0.0f)
            window->DrawList->AddLineH(sep1_x1, sep1_x2, seps_y, separator_col, separator_thickness);
        if (sep2_x2 > sep2_x1 && separator_thickness > 0.0f)
            window->DrawList->AddLineH(sep2_x1, sep2_x2, seps_y, separator_col, separator_thickness);
        if (g.LogEnabled)
            LogSetNextTextDecoration("---", nullptr);
        RenderTextEllipsis(window->DrawList, label_pos, VanVec2(bb.Max.x, bb.Max.y + style.ItemSpacing.y), bb.Max.x, label, label_end, &label_size);
    }
    else
    {
        if (g.LogEnabled)
            LogText("---");
        if (separator_thickness > 0.0f)
            window->DrawList->AddLineH(sep1_x1, sep2_x2, seps_y, separator_col, separator_thickness);
    }
}

void VanGui::SeparatorText(const char* label)
{
    VanGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return;

    // The SeparatorText() vs SeparatorTextEx() distinction is designed to be considerate that we may want:
    // - allow separator-text to be draggable items (would require a stable ID + a noticeable highlight)
    // - this high-level entry point to allow formatting? (which in turns may require ID separate from formatted string)
    // - because of this we probably can't turn 'const char* label' into 'const char* fmt, ...'
    // Otherwise, we can decide that users wanting to drag this would layout a dedicated drag-item,
    // and then we can turn this into a format function.
    SeparatorTextEx(0, label, FindRenderedTextEnd(label), 0.0f);
}

// Using 'hover_visibility_delay' allows us to hide the highlight and mouse cursor for a short time, which can be convenient to reduce visual noise.
bool VanGui::SplitterBehavior(const VanRect& bb, VanGuiID id, VanGuiAxis axis, float* size1, float* size2, float min_size1, float min_size2, float hover_extend, float hover_visibility_delay, VanU32 bg_col)
{
    VanGuiContext& g = *GVanGui;
    VanGuiWindow* window = g.CurrentWindow;

    if (!ItemAdd(bb, id, nullptr, VanGuiItemFlags_NoNav))
        return false;

    // FIXME: AFAIK the only leftover reason for passing VanGuiButtonFlags_AllowOverlap here is
    // to allow caller of SplitterBehavior() to call SetItemAllowOverlap() after the item.
    // Nowadays we would instead want to use SetNextItemAllowOverlap() before the item.
    VanGuiButtonFlags button_flags = VanGuiButtonFlags_FlattenChildren;
#ifndef VANGUI_DISABLE_OBSOLETE_FUNCTIONS
    button_flags |= VanGuiButtonFlags_AllowOverlap;
#endif

    bool hovered, held;
    VanRect bb_interact = bb;
    bb_interact.Expand(axis == VanGuiAxis_Y ? VanVec2(0.0f, hover_extend) : VanVec2(hover_extend, 0.0f));
    ButtonBehavior(bb_interact, id, &hovered, &held, button_flags);
    if (hovered)
        g.LastItemData.StatusFlags |= VanGuiItemStatusFlags_HoveredRect; // for IsItemHovered(), because bb_interact is larger than bb

    if (held || (hovered && g.HoveredIdPreviousFrame == id && g.HoveredIdTimer >= hover_visibility_delay))
        SetMouseCursor(axis == VanGuiAxis_Y ? VanGuiMouseCursor_ResizeNS : VanGuiMouseCursor_ResizeEW);

    VanRect bb_render = bb;
    if (held)
    {
        float mouse_delta = (g.IO.MousePos - g.ActiveIdClickOffset - bb_interact.Min)[axis];

        // Minimum pane size
        float size_1_maximum_delta = VanMax(0.0f, *size1 - min_size1);
        float size_2_maximum_delta = VanMax(0.0f, *size2 - min_size2);
        if (mouse_delta < -size_1_maximum_delta)
            mouse_delta = -size_1_maximum_delta;
        if (mouse_delta > size_2_maximum_delta)
            mouse_delta = size_2_maximum_delta;

        // Apply resize
        if (mouse_delta != 0.0f)
        {
            *size1 = VanMax(*size1 + mouse_delta, min_size1);
            *size2 = VanMax(*size2 - mouse_delta, min_size2);
            bb_render.Translate((axis == VanGuiAxis_X) ? VanVec2(mouse_delta, 0.0f) : VanVec2(0.0f, mouse_delta));
            MarkItemEdited(id);
        }
    }

    // Render at new position
    if (bg_col & VAN_COL32_A_MASK)
        window->DrawList->AddRectFilled(bb_render.Min, bb_render.Max, bg_col, 0.0f);
    const VanU32 col = GetColorU32(held ? VanGuiCol_SeparatorActive : (hovered && g.HoveredIdTimer >= hover_visibility_delay) ? VanGuiCol_SeparatorHovered : VanGuiCol_Separator);
    window->DrawList->AddRectFilled(bb_render.Min, bb_render.Max, col, 0.0f);

    return held;
}

static int VANGUI_CDECL ShrinkWidthItemComparer(const void* lhs, const void* rhs)
{
    const VanGuiShrinkWidthItem* a = static_cast<const VanGuiShrinkWidthItem*>(lhs);
    const VanGuiShrinkWidthItem* b = static_cast<const VanGuiShrinkWidthItem*>(rhs);
    if (int d = static_cast<int>(b->Width - a->Width))
        return d;
    return b->Index - a->Index;
}

// Shrink excess width from a set of item, by removing width from the larger items first.
// Set items Width to -1.0f to disable shrinking this item.
void VanGui::ShrinkWidths(VanGuiShrinkWidthItem* items, int count, float width_excess, float width_min)
{
    if (count == 1)
    {
        if (items[0].Width >= 0.0f)
            items[0].Width = VanMax(items[0].Width - width_excess, width_min);
        return;
    }
    VanQsort(items, static_cast<size_t>(count), sizeof(VanGuiShrinkWidthItem), ShrinkWidthItemComparer); // Sort largest first, smallest last.
    int count_same_width = 1;
    while (width_excess > 0.001f && count_same_width < count)
    {
        while (count_same_width < count && items[0].Width <= items[count_same_width].Width)
            count_same_width++;
        float max_width_to_remove_per_item = (count_same_width < count && items[count_same_width].Width >= 0.0f) ? (items[0].Width - items[count_same_width].Width) : (items[0].Width - 1.0f);
        max_width_to_remove_per_item = VanMin(items[0].Width - width_min, max_width_to_remove_per_item);
        if (max_width_to_remove_per_item <= 0.0f)
            break;
        float base_width_to_remove_per_item = VanMin(width_excess / count_same_width, max_width_to_remove_per_item);
        for (int item_n = 0; item_n < count_same_width; item_n++)
        {
            float width_to_remove_for_this_item = VanMin(base_width_to_remove_per_item, items[item_n].Width - width_min);
            items[item_n].Width -= width_to_remove_for_this_item;
            width_excess -= width_to_remove_for_this_item;
        }
    }

    // Round width and redistribute remainder
    // Ensure that e.g. the right-most tab of a shrunk tab-bar always reaches exactly at the same distance from the right-most edge of the tab bar separator.
    width_excess = 0.0f;
    for (int n = 0; n < count; n++)
    {
        float width_rounded = VanTrunc(items[n].Width);
        width_excess += items[n].Width - width_rounded;
        items[n].Width = width_rounded;
    }
    while (width_excess > 0.0f)
        for (int n = 0; n < count && width_excess > 0.0f; n++)
        {
            float width_to_add = VanMin(items[n].InitialWidth - items[n].Width, 1.0f);
            items[n].Width += width_to_add;
            width_excess -= width_to_add;
        }
}

//-------------------------------------------------------------------------
// [SECTION] Widgets: ComboBox
//-------------------------------------------------------------------------
// - CalcMaxPopupHeightFromItemCount() [Internal]
// - BeginCombo()
// - BeginComboPopup() [Internal]
// - EndCombo()
// - BeginComboPreview() [Internal]
// - EndComboPreview() [Internal]
// - Combo()
//-------------------------------------------------------------------------

static float CalcMaxPopupHeightFromItemCount(int items_count)
{
    VanGuiContext& g = *GVanGui;
    if (items_count <= 0)
        return FLT_MAX;
    return (g.FontSize + g.Style.ItemSpacing.y) * items_count - g.Style.ItemSpacing.y + (g.Style.WindowPadding.y * 2);
}

bool VanGui::BeginCombo(const char* label, const char* preview_value, VanGuiComboFlags flags)
{
    VanGuiContext& g = *GVanGui;
    VanGuiWindow* window = GetCurrentWindow();

    VanGuiNextWindowDataFlags backup_next_window_data_flags = g.NextWindowData.HasFlags;
    g.NextWindowData.ClearFlags(); // We behave like Begin() and need to consume those values
    if (window->SkipItems)
        return false;

    const VanGuiStyle& style = g.Style;
    const VanGuiID id = window->GetID(label);
    VAN_ASSERT((flags & (VanGuiComboFlags_NoArrowButton | VanGuiComboFlags_NoPreview)) != (VanGuiComboFlags_NoArrowButton | VanGuiComboFlags_NoPreview)); // Can't use both flags together
    if (flags & VanGuiComboFlags_WidthFitPreview)
        VAN_ASSERT((flags & (VanGuiComboFlags_NoPreview | static_cast<VanGuiComboFlags>(VanGuiComboFlags_CustomPreview))) == 0);

    const float arrow_size = (flags & VanGuiComboFlags_NoArrowButton) ? 0.0f : GetFrameHeight();
    const char* label_end = FindRenderedTextEnd(label);
    const VanVec2 label_size = CalcTextSize(label, label_end, false);
    const float preview_width = ((flags & VanGuiComboFlags_WidthFitPreview) && (preview_value != nullptr)) ? CalcTextSize(preview_value, nullptr, false).x : 0.0f;
    const float w = (flags & VanGuiComboFlags_NoPreview) ? arrow_size : ((flags & VanGuiComboFlags_WidthFitPreview) ? (arrow_size + preview_width + style.FramePadding.x * 2.0f) : CalcItemWidth());
    const VanRect bb(window->DC.CursorPos, window->DC.CursorPos + VanVec2(w, label_size.y + style.FramePadding.y * 2.0f));
    const VanRect total_bb(bb.Min, bb.Max + VanVec2(label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f, 0.0f));
    ItemSize(total_bb, style.FramePadding.y);
    if (!ItemAdd(total_bb, id, &bb))
        return false;

    // Open on click
    bool hovered, held;
    bool pressed = ButtonBehavior(bb, id, &hovered, &held);
    const VanGuiID popup_id = VanHashStr("##ComboPopup", 0, id);
    bool popup_open = IsPopupOpen(popup_id, VanGuiPopupFlags_None);
    if (pressed && !popup_open)
    {
        OpenPopupEx(popup_id, VanGuiPopupFlags_None);
        popup_open = true;
    }

    // Render shape
    const VanU32 frame_col = GetColorU32(hovered ? VanGuiCol_FrameBgHovered : VanGuiCol_FrameBg);
    const float value_x2 = VanMax(bb.Min.x, bb.Max.x - arrow_size);
    RenderNavCursor(bb, id);
    if (!(flags & VanGuiComboFlags_NoPreview))
        window->DrawList->AddRectFilled(bb.Min, VanVec2(value_x2, bb.Max.y), frame_col, style.FrameRounding, (flags & VanGuiComboFlags_NoArrowButton) ? VanDrawFlags_RoundCornersAll : VanDrawFlags_RoundCornersLeft);
    if (!(flags & VanGuiComboFlags_NoArrowButton))
    {
        VanU32 bg_col = GetColorU32((popup_open || hovered) ? VanGuiCol_ButtonHovered : VanGuiCol_Button);
        VanU32 text_col = GetColorU32(VanGuiCol_Text);
        window->DrawList->AddRectFilled(VanVec2(value_x2, bb.Min.y), bb.Max, bg_col, style.FrameRounding, (w <= arrow_size) ? VanDrawFlags_RoundCornersAll : VanDrawFlags_RoundCornersRight);
        if (value_x2 + arrow_size - style.FramePadding.x <= bb.Max.x)
            RenderArrow(window->DrawList, VanVec2(value_x2 + style.FramePadding.y, bb.Min.y + style.FramePadding.y), text_col, VanGuiDir_Down, 1.0f);
    }
    RenderFrameBorder(bb.Min, bb.Max, style.FrameRounding);

    // Custom preview
    if (flags & VanGuiComboFlags_CustomPreview)
    {
        g.ComboPreviewData.PreviewRect = VanRect(bb.Min.x, bb.Min.y, value_x2, bb.Max.y);
        VAN_ASSERT(preview_value == nullptr || preview_value[0] == 0);
        preview_value = nullptr;
    }

    // Render preview and label
    if (preview_value != nullptr && !(flags & VanGuiComboFlags_NoPreview))
    {
        if (g.LogEnabled)
            LogSetNextTextDecoration("{", "}");
        RenderTextClipped(bb.Min + style.FramePadding, VanVec2(value_x2, bb.Max.y), preview_value, nullptr, nullptr);
    }
    if (label_size.x > 0)
        RenderText(VanVec2(bb.Max.x + style.ItemInnerSpacing.x, bb.Min.y + style.FramePadding.y), label, label_end, false);

    if (!popup_open)
        return false;

    g.NextWindowData.HasFlags = backup_next_window_data_flags;
    return BeginComboPopup(popup_id, bb, flags);
}

bool VanGui::BeginComboPopup(VanGuiID popup_id, const VanRect& bb, VanGuiComboFlags flags)
{
    VanGuiContext& g = *GVanGui;
    if (!IsPopupOpen(popup_id, VanGuiPopupFlags_None))
    {
        g.NextWindowData.ClearFlags();
        return false;
    }

    // Set popup size
    float w = bb.GetWidth();
    if (g.NextWindowData.HasFlags & VanGuiNextWindowDataFlags_HasSizeConstraint)
    {
        g.NextWindowData.SizeConstraintRect.Min.x = VanMax(g.NextWindowData.SizeConstraintRect.Min.x, w);
    }
    else
    {
        if ((flags & VanGuiComboFlags_HeightMask_) == 0)
            flags |= VanGuiComboFlags_HeightRegular;
        VAN_ASSERT(VanIsPowerOfTwo(flags & VanGuiComboFlags_HeightMask_)); // Only one
        int popup_max_height_in_items = -1;
        if (flags & VanGuiComboFlags_HeightRegular)     popup_max_height_in_items = 8;
        else if (flags & VanGuiComboFlags_HeightSmall)  popup_max_height_in_items = 4;
        else if (flags & VanGuiComboFlags_HeightLarge)  popup_max_height_in_items = 20;
        VanVec2 constraint_min(0.0f, 0.0f), constraint_max(FLT_MAX, FLT_MAX);
        if ((g.NextWindowData.HasFlags & VanGuiNextWindowDataFlags_HasSize) == 0 || g.NextWindowData.SizeVal.x <= 0.0f) // Don't apply constraints if user specified a size
            constraint_min.x = w;
        if ((g.NextWindowData.HasFlags & VanGuiNextWindowDataFlags_HasSize) == 0 || g.NextWindowData.SizeVal.y <= 0.0f)
            constraint_max.y = CalcMaxPopupHeightFromItemCount(popup_max_height_in_items);
        SetNextWindowSizeConstraints(constraint_min, constraint_max);
    }

    // This is essentially a specialized version of BeginPopupEx()
    char name[16];
    VanFormatString(name, VAN_COUNTOF(name), "##Combo_%02d", g.BeginComboDepth); // Recycle windows based on depth

    // Set position given a custom constraint (peak into expected window size so we can position it)
    // FIXME: This might be easier to express with an hypothetical SetNextWindowPosConstraints() function?
    // FIXME: This might be moved to Begin() or at least around the same spot where Tooltips and other Popups are calling FindBestWindowPosForPopupEx()?
    if (VanGuiWindow* popup_window = FindWindowByName(name))
        if (popup_window->WasActive)
        {
            // Always override 'AutoPosLastDirection' to not leave a chance for a past value to affect us.
            VanVec2 size_expected = CalcWindowNextAutoFitSize(popup_window);
            popup_window->AutoPosLastDirection = (flags & VanGuiComboFlags_PopupAlignLeft) ? VanGuiDir_Left : VanGuiDir_Down; // Left = "Below, Toward Left", Down = "Below, Toward Right (default)"
            VanRect r_outer = GetPopupAllowedExtentRect(popup_window);
            VanVec2 pos = FindBestWindowPosForPopupEx(bb.GetBL(), size_expected, &popup_window->AutoPosLastDirection, r_outer, bb, VanGuiPopupPositionPolicy_ComboBox);
            SetNextWindowPos(pos);
        }

    // We don't use BeginPopupEx() solely because we have a custom name string, which we could make an argument to BeginPopupEx()
    VanGuiWindowFlags window_flags = VanGuiWindowFlags_AlwaysAutoResize | VanGuiWindowFlags_Popup | VanGuiWindowFlags_NoTitleBar | VanGuiWindowFlags_NoResize | VanGuiWindowFlags_NoSavedSettings | VanGuiWindowFlags_NoMove;
    PushStyleVarX(VanGuiStyleVar_WindowPadding, g.Style.FramePadding.x); // Horizontally align ourselves with the framed text
    bool ret = Begin(name, nullptr, window_flags);
    PopStyleVar();
    if (!ret)
    {
        EndPopup();
        if (!g.IO.ConfigDebugBeginReturnValueOnce && !g.IO.ConfigDebugBeginReturnValueLoop) // Begin may only return false with those debug tools activated.
            VAN_ASSERT(0); // This should never happen as we tested for IsPopupOpen() above
        return false;
    }
    g.BeginComboDepth++;
    return true;
}

void VanGui::EndCombo()
{
    VanGuiContext& g = *GVanGui;
    g.BeginComboDepth--;
    char name[16];
    VanFormatString(name, VAN_COUNTOF(name), "##Combo_%02d", g.BeginComboDepth); // FIXME: Move those to helpers?
    if (strcmp(g.CurrentWindow->Name, name) != 0)
        VAN_ASSERT_USER_ERROR_RET(0, "Calling EndCombo() in wrong window!");
    EndPopup();
}

// Call directly after the BeginCombo/EndCombo block. The preview is designed to only host non-interactive elements
// (Experimental, see GitHub issues: #1658, #4168)
bool VanGui::BeginComboPreview()
{
    VanGuiContext& g = *GVanGui;
    VanGuiWindow* window = g.CurrentWindow;
    VanGuiComboPreviewData* preview_data = &g.ComboPreviewData;

    if (window->SkipItems || !(g.LastItemData.StatusFlags & VanGuiItemStatusFlags_Visible))
        return false;
    VAN_ASSERT(g.LastItemData.Rect.Min.x == preview_data->PreviewRect.Min.x && g.LastItemData.Rect.Min.y == preview_data->PreviewRect.Min.y); // Didn't call after BeginCombo/EndCombo block or forgot to pass VanGuiComboFlags_CustomPreview flag?
    if (!window->ClipRect.Overlaps(preview_data->PreviewRect)) // Narrower test (optional)
        return false;

    // FIXME: This could be contained in a PushWorkRect() api
    preview_data->BackupCursorPos = window->DC.CursorPos;
    preview_data->BackupCursorMaxPos = window->DC.CursorMaxPos;
    preview_data->BackupCursorPosPrevLine = window->DC.CursorPosPrevLine;
    preview_data->BackupPrevLineTextBaseOffset = window->DC.PrevLineTextBaseOffset;
    preview_data->BackupLayout = window->DC.LayoutType;
    window->DC.CursorPos = preview_data->PreviewRect.Min + g.Style.FramePadding;
    window->DC.CursorMaxPos = window->DC.CursorPos;
    window->DC.LayoutType = VanGuiLayoutType_Horizontal;
    window->DC.IsSameLine = false;
    PushClipRect(preview_data->PreviewRect.Min, preview_data->PreviewRect.Max, true);

    return true;
}

void VanGui::EndComboPreview()
{
    VanGuiContext& g = *GVanGui;
    VanGuiWindow* window = g.CurrentWindow;
    VanGuiComboPreviewData* preview_data = &g.ComboPreviewData;

    // FIXME: Using CursorMaxPos approximation instead of correct AABB which we will store in VanDrawCmd in the future
    VanDrawList* draw_list = window->DrawList;
    if (window->DC.CursorMaxPos.x < preview_data->PreviewRect.Max.x && window->DC.CursorMaxPos.y < preview_data->PreviewRect.Max.y)
        if (draw_list->CmdBuffer.Size > 1) // Unlikely case that the PushClipRect() didn't create a command
        {
            draw_list->_CmdHeader.ClipRect = draw_list->CmdBuffer[draw_list->CmdBuffer.Size - 1].ClipRect = draw_list->CmdBuffer[draw_list->CmdBuffer.Size - 2].ClipRect;
            draw_list->_TryMergeDrawCmds();
        }
    PopClipRect();
    window->DC.CursorPos = preview_data->BackupCursorPos;
    window->DC.CursorMaxPos = VanMax(window->DC.CursorMaxPos, preview_data->BackupCursorMaxPos);
    window->DC.CursorPosPrevLine = preview_data->BackupCursorPosPrevLine;
    window->DC.PrevLineTextBaseOffset = preview_data->BackupPrevLineTextBaseOffset;
    window->DC.LayoutType = preview_data->BackupLayout;
    window->DC.IsSameLine = false;
    preview_data->PreviewRect = VanRect();
}

// Getter for the old Combo() API: const char*[]
static const char* Items_ArrayGetter(void* data, int idx)
{
    const char* const* items = static_cast<const char* const*>(data);
    return items[idx];
}

// Getter for the old Combo() API: "item1\0item2\0item3\0"
static const char* Items_SingleStringGetter(void* data, int idx)
{
    const char* items_separated_by_zeros = static_cast<const char*>(data);
    int items_count = 0;
    const char* p = items_separated_by_zeros;
    while (*p)
    {
        if (idx == items_count)
            break;
        p += VanStrlen(p) + 1;
        items_count++;
    }
    return *p ? p : nullptr;
}

// Old API, prefer using BeginCombo() nowadays if you can.
bool VanGui::Combo(const char* label, int* current_item, const char* (*getter)(void* user_data, int idx), void* user_data, int items_count, int popup_max_height_in_items)
{
    VanGuiContext& g = *GVanGui;

    // Call the getter to obtain the preview string which is a parameter to BeginCombo()
    const char* preview_value = nullptr;
    if (*current_item >= 0 && *current_item < items_count)
        preview_value = getter(user_data, *current_item);

    // The old Combo() API exposed "popup_max_height_in_items". The new more general BeginCombo() API doesn't have/need it, but we emulate it here.
    if (popup_max_height_in_items != -1 && !(g.NextWindowData.HasFlags & VanGuiNextWindowDataFlags_HasSizeConstraint))
        SetNextWindowSizeConstraints(VanVec2(0, 0), VanVec2(FLT_MAX, CalcMaxPopupHeightFromItemCount(popup_max_height_in_items)));

    if (!BeginCombo(label, preview_value, VanGuiComboFlags_None))
        return false;

    // Display items
    bool value_changed = false;
    VanGuiListClipper clipper;
    clipper.Begin(items_count);
    clipper.IncludeItemByIndex(*current_item);
    while (clipper.Step())
        for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
        {
            const char* item_text = getter(user_data, i);
            if (item_text == nullptr)
                item_text = "*Unknown item*";

            PushID(i);
            const bool item_selected = (i == *current_item);
            if (Selectable(item_text, item_selected) && *current_item != i)
            {
                value_changed = true;
                *current_item = i;
            }
            if (item_selected)
                SetItemDefaultFocus();
            PopID();
        }

    EndCombo();
    if (value_changed)
        MarkItemEdited(g.LastItemData.ID);

    return value_changed;
}

// Combo box helper allowing to pass an array of strings.
bool VanGui::Combo(const char* label, int* current_item, const char* const items[], int items_count, int height_in_items)
{
    const bool value_changed = Combo(label, current_item, Items_ArrayGetter, static_cast<void*>(const_cast<char**>(items)), items_count, height_in_items);
    return value_changed;
}

// Combo box helper allowing to pass all items in a single string literal holding multiple zero-terminated items "item1\0item2\0"
bool VanGui::Combo(const char* label, int* current_item, const char* items_separated_by_zeros, int height_in_items)
{
    int items_count = 0;
    const char* p = items_separated_by_zeros;       // FIXME-OPT: Avoid computing this, or at least only when combo is open
    while (*p)
    {
        p += VanStrlen(p) + 1;
        items_count++;
    }
    bool value_changed = Combo(label, current_item, Items_SingleStringGetter, static_cast<void*>(const_cast<char*>(items_separated_by_zeros)), items_count, height_in_items);
    return value_changed;
}

//-------------------------------------------------------------------------
// [SECTION] Data Type and Data Formatting Helpers [Internal]
//-------------------------------------------------------------------------
// - DataTypeGetInfo()
// - DataTypeFormatString()
// - DataTypeApplyOp()
// - DataTypeApplyFromText()
// - DataTypeCompare()
// - DataTypeClamp()
// - GetMinimumStepAtDecimalPrecision
// - RoundScalarWithFormat<>()
//-------------------------------------------------------------------------

static const VanU32 GDefaultRgbaColorMarkers[4] =
{
    VAN_COL32(240,20,20,255), VAN_COL32(20,240,20,255), VAN_COL32(20,20,240,255), VAN_COL32(140,140,140,255)
};

static const VanGuiDataTypeInfo GDataTypeInfo[] =
{
    { sizeof(char),             "S8",   "%d",   "%d"    },  // VanGuiDataType_S8
    { sizeof(unsigned char),    "U8",   "%u",   "%u"    },
    { sizeof(short),            "S16",  "%d",   "%d"    },  // VanGuiDataType_S16
    { sizeof(unsigned short),   "U16",  "%u",   "%u"    },
    { sizeof(int),              "S32",  "%d",   "%d"    },  // VanGuiDataType_S32
    { sizeof(unsigned int),     "U32",  "%u",   "%u"    },
#ifdef _MSC_VER
    { sizeof(VanS64),            "S64",  "%I64d","%I64d" },  // VanGuiDataType_S64
    { sizeof(VanU64),            "U64",  "%I64u","%I64u" },
#else
    { sizeof(VanS64),            "S64",  "%lld", "%lld"  },  // VanGuiDataType_S64
    { sizeof(VanU64),            "U64",  "%llu", "%llu"  },
#endif
    { sizeof(float),            "float", "%.3f","%f"    },  // VanGuiDataType_Float (float are promoted to double in va_arg)
    { sizeof(double),           "double","%f",  "%lf"   },  // VanGuiDataType_Double
    { sizeof(bool),             "bool", "%d",   "%d"    },  // VanGuiDataType_Bool
    { 0,                        "char*","%s",   "%s"    },  // VanGuiDataType_String
};
VAN_STATIC_ASSERT(VAN_COUNTOF(GDataTypeInfo) == VanGuiDataType_COUNT);

const VanGuiDataTypeInfo* VanGui::DataTypeGetInfo(VanGuiDataType data_type)
{
    VAN_ASSERT(data_type >= 0 && data_type < VanGuiDataType_COUNT);
    return &GDataTypeInfo[data_type];
}

int VanGui::DataTypeFormatString(char* buf, int buf_size, VanGuiDataType data_type, const void* p_data, const char* format)
{
    // Signedness doesn't matter when pushing integer arguments
    if (data_type == VanGuiDataType_S32 || data_type == VanGuiDataType_U32)
        return VanFormatString(buf, buf_size, format, *static_cast<const VanU32*>(p_data));
    if (data_type == VanGuiDataType_S64 || data_type == VanGuiDataType_U64)
        return VanFormatString(buf, buf_size, format, *static_cast<const VanU64*>(p_data));
    if (data_type == VanGuiDataType_Float)
        return VanFormatString(buf, buf_size, format, *static_cast<const float*>(p_data));
    if (data_type == VanGuiDataType_Double)
        return VanFormatString(buf, buf_size, format, *static_cast<const double*>(p_data));
    if (data_type == VanGuiDataType_S8)
        return VanFormatString(buf, buf_size, format, *static_cast<const VanS8*>(p_data));
    if (data_type == VanGuiDataType_U8)
        return VanFormatString(buf, buf_size, format, *static_cast<const VanU8*>(p_data));
    if (data_type == VanGuiDataType_S16)
        return VanFormatString(buf, buf_size, format, *static_cast<const VanS16*>(p_data));
    if (data_type == VanGuiDataType_U16)
        return VanFormatString(buf, buf_size, format, *static_cast<const VanU16*>(p_data));
    VAN_ASSERT(0);
    return 0;
}

void VanGui::DataTypeApplyOp(VanGuiDataType data_type, int op, void* output, const void* arg1, const void* arg2)
{
    VAN_ASSERT(op == '+' || op == '-');
    switch (data_type)
    {
        case VanGuiDataType_S8:
            if (op == '+') { *reinterpret_cast<VanS8*>(output)  = VanAddClampOverflow(*static_cast<const VanS8*>(arg1),  *static_cast<const VanS8*>(arg2),  VAN_S8_MIN,  VAN_S8_MAX); }
            if (op == '-') { *reinterpret_cast<VanS8*>(output)  = VanSubClampOverflow(*static_cast<const VanS8*>(arg1),  *static_cast<const VanS8*>(arg2),  VAN_S8_MIN,  VAN_S8_MAX); }
            return;
        case VanGuiDataType_U8:
            if (op == '+') { *reinterpret_cast<VanU8*>(output)  = VanAddClampOverflow(*static_cast<const VanU8*>(arg1),  *static_cast<const VanU8*>(arg2),  VAN_U8_MIN,  VAN_U8_MAX); }
            if (op == '-') { *reinterpret_cast<VanU8*>(output)  = VanSubClampOverflow(*static_cast<const VanU8*>(arg1),  *static_cast<const VanU8*>(arg2),  VAN_U8_MIN,  VAN_U8_MAX); }
            return;
        case VanGuiDataType_S16:
            if (op == '+') { *reinterpret_cast<VanS16*>(output) = VanAddClampOverflow(*static_cast<const VanS16*>(arg1), *static_cast<const VanS16*>(arg2), VAN_S16_MIN, VAN_S16_MAX); }
            if (op == '-') { *reinterpret_cast<VanS16*>(output) = VanSubClampOverflow(*static_cast<const VanS16*>(arg1), *static_cast<const VanS16*>(arg2), VAN_S16_MIN, VAN_S16_MAX); }
            return;
        case VanGuiDataType_U16:
            if (op == '+') { *reinterpret_cast<VanU16*>(output) = VanAddClampOverflow(*static_cast<const VanU16*>(arg1), *static_cast<const VanU16*>(arg2), VAN_U16_MIN, VAN_U16_MAX); }
            if (op == '-') { *reinterpret_cast<VanU16*>(output) = VanSubClampOverflow(*static_cast<const VanU16*>(arg1), *static_cast<const VanU16*>(arg2), VAN_U16_MIN, VAN_U16_MAX); }
            return;
        case VanGuiDataType_S32:
            if (op == '+') { *reinterpret_cast<VanS32*>(output) = VanAddClampOverflow(*static_cast<const VanS32*>(arg1), *static_cast<const VanS32*>(arg2), VAN_S32_MIN, VAN_S32_MAX); }
            if (op == '-') { *reinterpret_cast<VanS32*>(output) = VanSubClampOverflow(*static_cast<const VanS32*>(arg1), *static_cast<const VanS32*>(arg2), VAN_S32_MIN, VAN_S32_MAX); }
            return;
        case VanGuiDataType_U32:
            if (op == '+') { *reinterpret_cast<VanU32*>(output) = VanAddClampOverflow(*static_cast<const VanU32*>(arg1), *static_cast<const VanU32*>(arg2), VAN_U32_MIN, VAN_U32_MAX); }
            if (op == '-') { *reinterpret_cast<VanU32*>(output) = VanSubClampOverflow(*static_cast<const VanU32*>(arg1), *static_cast<const VanU32*>(arg2), VAN_U32_MIN, VAN_U32_MAX); }
            return;
        case VanGuiDataType_S64:
            if (op == '+') { *reinterpret_cast<VanS64*>(output) = VanAddClampOverflow(*static_cast<const VanS64*>(arg1), *static_cast<const VanS64*>(arg2), VAN_S64_MIN, VAN_S64_MAX); }
            if (op == '-') { *reinterpret_cast<VanS64*>(output) = VanSubClampOverflow(*static_cast<const VanS64*>(arg1), *static_cast<const VanS64*>(arg2), VAN_S64_MIN, VAN_S64_MAX); }
            return;
        case VanGuiDataType_U64:
            if (op == '+') { *reinterpret_cast<VanU64*>(output) = VanAddClampOverflow(*static_cast<const VanU64*>(arg1), *static_cast<const VanU64*>(arg2), VAN_U64_MIN, VAN_U64_MAX); }
            if (op == '-') { *reinterpret_cast<VanU64*>(output) = VanSubClampOverflow(*static_cast<const VanU64*>(arg1), *static_cast<const VanU64*>(arg2), VAN_U64_MIN, VAN_U64_MAX); }
            return;
        case VanGuiDataType_Float:
            if (op == '+') { *reinterpret_cast<float*>(output) = *static_cast<const float*>(arg1) + *static_cast<const float*>(arg2); }
            if (op == '-') { *reinterpret_cast<float*>(output) = *static_cast<const float*>(arg1) - *static_cast<const float*>(arg2); }
            return;
        case VanGuiDataType_Double:
            if (op == '+') { *reinterpret_cast<double*>(output) = *static_cast<const double*>(arg1) + *static_cast<const double*>(arg2); }
            if (op == '-') { *reinterpret_cast<double*>(output) = *static_cast<const double*>(arg1) - *static_cast<const double*>(arg2); }
            return;
        case VanGuiDataType_COUNT: break;
    }
    VAN_ASSERT(0);
}

// User can input math operators (e.g. +100) to edit a numerical values.
// NB: This is _not_ a full expression evaluator. We should probably add one and replace this dumb mess..
bool VanGui::DataTypeApplyFromText(const char* buf, VanGuiDataType data_type, void* p_data, const char* format, void* p_data_when_empty)
{
    // Copy the value in an opaque buffer so we can compare at the end of the function if it changed at all.
    const VanGuiDataTypeInfo* type_info = DataTypeGetInfo(data_type);
    VanGuiDataTypeStorage data_backup;
    memcpy(&data_backup, p_data, type_info->Size);

    while (VanCharIsBlankA(*buf))
        buf++;
    if (!buf[0])
    {
        if (p_data_when_empty != nullptr)
        {
            memcpy(p_data, p_data_when_empty, type_info->Size);
            return memcmp(&data_backup, p_data, type_info->Size) != 0;
        }
        return false;
    }

    // Sanitize format
    // - For float/double we have to ignore format with precision (e.g. "%.2f") because sscanf doesn't take them in, so force them into %f and %lf
    char format_sanitized[32];
    if (data_type == VanGuiDataType_Float || data_type == VanGuiDataType_Double)
    {
        format = type_info->ScanFmt;
    }
    else
    {
        format = VanParseFormatSanitizeForScanning(format, format_sanitized, VAN_COUNTOF(format_sanitized));
        if (format[0] == '\0')
            format = type_info->ScanFmt; // Format doesn't want us to show the number currently, but we still need to parse the resulting input
    }

    // Small types need a 32-bit buffer to receive the result from scanf()
    int v32 = 0;
    if (sscanf(buf, format, type_info->Size >= 4 ? p_data : &v32) < 1)
        return false;
    if (type_info->Size < 4)
    {
        if (data_type == VanGuiDataType_S8)
            *reinterpret_cast<VanS8*>(p_data) = static_cast<VanS8>(VanClamp(v32, static_cast<int>(VAN_S8_MIN), static_cast<int>(VAN_S8_MAX)));
        else if (data_type == VanGuiDataType_U8)
            *reinterpret_cast<VanU8*>(p_data) = static_cast<VanU8>(VanClamp(v32, static_cast<int>(VAN_U8_MIN), static_cast<int>(VAN_U8_MAX)));
        else if (data_type == VanGuiDataType_S16)
            *reinterpret_cast<VanS16*>(p_data) = static_cast<VanS16>(VanClamp(v32, static_cast<int>(VAN_S16_MIN), static_cast<int>(VAN_S16_MAX)));
        else if (data_type == VanGuiDataType_U16)
            *reinterpret_cast<VanU16*>(p_data) = static_cast<VanU16>(VanClamp(v32, static_cast<int>(VAN_U16_MIN), static_cast<int>(VAN_U16_MAX)));
        else
            VAN_ASSERT(0);
    }

    return memcmp(&data_backup, p_data, type_info->Size) != 0;
}

template<typename T>
static int DataTypeCompareT(const T* lhs, const T* rhs)
{
    if (*lhs < *rhs) return -1;
    if (*lhs > *rhs) return +1;
    return 0;
}

int VanGui::DataTypeCompare(VanGuiDataType data_type, const void* arg_1, const void* arg_2)
{
    switch (data_type)
    {
    case VanGuiDataType_S8:     return DataTypeCompareT<VanS8  >(static_cast<const VanS8*  >(arg_1), static_cast<const VanS8*  >(arg_2));
    case VanGuiDataType_U8:     return DataTypeCompareT<VanU8  >(static_cast<const VanU8*  >(arg_1), static_cast<const VanU8*  >(arg_2));
    case VanGuiDataType_S16:    return DataTypeCompareT<VanS16 >(static_cast<const VanS16* >(arg_1), static_cast<const VanS16* >(arg_2));
    case VanGuiDataType_U16:    return DataTypeCompareT<VanU16 >(static_cast<const VanU16* >(arg_1), static_cast<const VanU16* >(arg_2));
    case VanGuiDataType_S32:    return DataTypeCompareT<VanS32 >(static_cast<const VanS32* >(arg_1), static_cast<const VanS32* >(arg_2));
    case VanGuiDataType_U32:    return DataTypeCompareT<VanU32 >(static_cast<const VanU32* >(arg_1), static_cast<const VanU32* >(arg_2));
    case VanGuiDataType_S64:    return DataTypeCompareT<VanS64 >(static_cast<const VanS64* >(arg_1), static_cast<const VanS64* >(arg_2));
    case VanGuiDataType_U64:    return DataTypeCompareT<VanU64 >(static_cast<const VanU64* >(arg_1), static_cast<const VanU64* >(arg_2));
    case VanGuiDataType_Float:  return DataTypeCompareT<float >(static_cast<const float*  >(arg_1), static_cast<const float*  >(arg_2));
    case VanGuiDataType_Double: return DataTypeCompareT<double>(static_cast<const double* >(arg_1), static_cast<const double* >(arg_2));
    case VanGuiDataType_COUNT:  break;
    }
    VAN_ASSERT(0);
    return 0;
}

template<typename T>
static bool DataTypeClampT(T* v, const T* v_min, const T* v_max)
{
    // Clamp, both sides are optional, return true if modified
    if (v_min && *v < *v_min) { *v = *v_min; return true; }
    if (v_max && *v > *v_max) { *v = *v_max; return true; }
    return false;
}

bool VanGui::DataTypeClamp(VanGuiDataType data_type, void* p_data, const void* p_min, const void* p_max)
{
    switch (data_type)
    {
    case VanGuiDataType_S8:     return DataTypeClampT<VanS8  >(static_cast<VanS8*  >(p_data), static_cast<const VanS8*  >(p_min), static_cast<const VanS8*  >(p_max));
    case VanGuiDataType_U8:     return DataTypeClampT<VanU8  >(static_cast<VanU8*  >(p_data), static_cast<const VanU8*  >(p_min), static_cast<const VanU8*  >(p_max));
    case VanGuiDataType_S16:    return DataTypeClampT<VanS16 >(static_cast<VanS16* >(p_data), static_cast<const VanS16* >(p_min), static_cast<const VanS16* >(p_max));
    case VanGuiDataType_U16:    return DataTypeClampT<VanU16 >(static_cast<VanU16* >(p_data), static_cast<const VanU16* >(p_min), static_cast<const VanU16* >(p_max));
    case VanGuiDataType_S32:    return DataTypeClampT<VanS32 >(static_cast<VanS32* >(p_data), static_cast<const VanS32* >(p_min), static_cast<const VanS32* >(p_max));
    case VanGuiDataType_U32:    return DataTypeClampT<VanU32 >(static_cast<VanU32* >(p_data), static_cast<const VanU32* >(p_min), static_cast<const VanU32* >(p_max));
    case VanGuiDataType_S64:    return DataTypeClampT<VanS64 >(static_cast<VanS64* >(p_data), static_cast<const VanS64* >(p_min), static_cast<const VanS64* >(p_max));
    case VanGuiDataType_U64:    return DataTypeClampT<VanU64 >(static_cast<VanU64* >(p_data), static_cast<const VanU64* >(p_min), static_cast<const VanU64* >(p_max));
    case VanGuiDataType_Float:  return DataTypeClampT<float >(static_cast<float*  >(p_data), static_cast<const float*  >(p_min), static_cast<const float*  >(p_max));
    case VanGuiDataType_Double: return DataTypeClampT<double>(static_cast<double* >(p_data), static_cast<const double* >(p_min), static_cast<const double* >(p_max));
    case VanGuiDataType_COUNT:  break;
    }
    VAN_ASSERT(0);
    return false;
}

bool VanGui::DataTypeIsZero(VanGuiDataType data_type, const void* p_data)
{
    VanGuiContext& g = *GVanGui;
    return DataTypeCompare(data_type, p_data, &g.DataTypeZeroValue) == 0;
}

static float GetMinimumStepAtDecimalPrecision(int decimal_precision)
{
    static const float min_steps[10] = { 1.0f, 0.1f, 0.01f, 0.001f, 0.0001f, 0.00001f, 0.000001f, 0.0000001f, 0.00000001f, 0.000000001f };
    if (decimal_precision < 0)
        return FLT_MIN;
    return (decimal_precision < VAN_COUNTOF(min_steps)) ? min_steps[decimal_precision] : VanPow(10.0f, static_cast<float>(-decimal_precision));
}

template<typename TYPE>
TYPE VanGui::RoundScalarWithFormatT(const char* format, VanGuiDataType data_type, TYPE v)
{
    VAN_UNUSED(data_type);
    VAN_ASSERT(data_type == VanGuiDataType_Float || data_type == VanGuiDataType_Double);
    const char* fmt_start = VanParseFormatFindStart(format);
    if (fmt_start[0] != '%' || fmt_start[1] == '%') // Don't apply if the value is not visible in the format string
        return v;

    // Sanitize format
    char fmt_sanitized[32];
    VanParseFormatSanitizeForPrinting(fmt_start, fmt_sanitized, VAN_COUNTOF(fmt_sanitized));
    fmt_start = fmt_sanitized;

    // Format value with our rounding, and read back
    char v_str[64];
    VanFormatString(v_str, VAN_COUNTOF(v_str), fmt_start, v);
    const char* p = v_str;
    while (*p == ' ')
        p++;
    v = static_cast<TYPE>(VanAtof(p));

    return v;
}

//-------------------------------------------------------------------------
// [SECTION] Widgets: DragScalar, DragFloat, DragInt, etc.
//-------------------------------------------------------------------------
// - DragBehaviorT<>() [Internal]
// - DragBehavior() [Internal]
// - DragScalar()
// - DragScalarN()
// - DragFloat()
// - DragFloat2()
// - DragFloat3()
// - DragFloat4()
// - DragFloatRange2()
// - DragInt()
// - DragInt2()
// - DragInt3()
// - DragInt4()
// - DragIntRange2()
//-------------------------------------------------------------------------

// This is called by DragBehavior() when the widget is active (held by mouse or being manipulated with Nav controls)
template<typename TYPE, typename SIGNEDTYPE, typename FLOATTYPE>
bool VanGui::DragBehaviorT(VanGuiDataType data_type, TYPE* v, float v_speed, const TYPE v_min, const TYPE v_max, const char* format, VanGuiSliderFlags flags)
{
    VanGuiContext& g = *GVanGui;
    const VanGuiAxis axis = (flags & VanGuiSliderFlags_Vertical) ? VanGuiAxis_Y : VanGuiAxis_X;
    const bool is_bounded = (v_min < v_max) || ((v_min == v_max) && (v_min != 0.0f || (flags & VanGuiSliderFlags_ClampZeroRange)));
    const bool is_wrapped = is_bounded && (flags & VanGuiSliderFlags_WrapAround);
    const bool is_logarithmic = (flags & VanGuiSliderFlags_Logarithmic) != 0;
    const bool is_floating_point = (data_type == VanGuiDataType_Float) || (data_type == VanGuiDataType_Double);

    // Default tweak speed
    if (v_speed == 0.0f && is_bounded && (v_max - v_min < FLT_MAX))
        v_speed = static_cast<float>((v_max - v_min) * g.DragSpeedDefaultRatio);

    // Inputs accumulates into g.DragCurrentAccum, which is flushed into the current value as soon as it makes a difference with our precision settings
    float adjust_delta = 0.0f;
    if (g.ActiveIdSource == VanGuiInputSource_Mouse && IsMousePosValid() && IsMouseDragPastThreshold(0, g.IO.MouseDragThreshold * DRAG_MOUSE_THRESHOLD_FACTOR))
    {
        adjust_delta = g.IO.MouseDelta[axis];
        if (g.IO.KeyAlt && !(flags & VanGuiSliderFlags_NoSpeedTweaks))
            adjust_delta *= 1.0f / 100.0f;
        if (g.IO.KeyShift && !(flags & VanGuiSliderFlags_NoSpeedTweaks))
            adjust_delta *= 10.0f;
    }
    else if (g.ActiveIdSource == VanGuiInputSource_Keyboard || g.ActiveIdSource == VanGuiInputSource_Gamepad)
    {
        const int decimal_precision = is_floating_point ? VanParseFormatPrecision(format, 3) : 0;
        const bool tweak_slow = IsKeyDown((g.NavInputSource == VanGuiInputSource_Gamepad) ? VanGuiKey_NavGamepadTweakSlow : VanGuiKey_NavKeyboardTweakSlow);
        const bool tweak_fast = IsKeyDown((g.NavInputSource == VanGuiInputSource_Gamepad) ? VanGuiKey_NavGamepadTweakFast : VanGuiKey_NavKeyboardTweakFast);
        const float tweak_factor = (flags & VanGuiSliderFlags_NoSpeedTweaks) ? 1.0f : tweak_slow ? 1.0f / 10.0f : tweak_fast ? 10.0f : 1.0f;
        adjust_delta = GetNavTweakPressedAmount(axis) * tweak_factor;
        v_speed = VanMax(v_speed, GetMinimumStepAtDecimalPrecision(decimal_precision));
    }
    adjust_delta *= v_speed;

    // For vertical drag we currently assume that Up=higher value (like we do with vertical sliders). This may become a parameter.
    if (axis == VanGuiAxis_Y)
        adjust_delta = -adjust_delta;

    // For logarithmic use our range is effectively 0..1 so scale the delta into that range
    if (is_logarithmic && (v_max - v_min < FLT_MAX) && ((v_max - v_min) > 0.000001f)) // Epsilon to avoid /0
        adjust_delta /= static_cast<float>(v_max - v_min);

    // Clear current value on activation
    // Avoid altering values and clamping when we are _already_ past the limits and heading in the same direction, so e.g. if range is 0..255, current value is 300 and we are pushing to the right side, keep the 300.
    const bool is_just_activated = g.ActiveIdIsJustActivated;
    const bool is_already_past_limits_and_pushing_outward = is_bounded && !is_wrapped && ((*v >= v_max && adjust_delta > 0.0f) || (*v <= v_min && adjust_delta < 0.0f));
    if (is_just_activated || is_already_past_limits_and_pushing_outward)
    {
        g.DragCurrentAccum = 0.0f;
        g.DragCurrentAccumDirty = false;
    }
    else if (adjust_delta != 0.0f)
    {
        g.DragCurrentAccum += adjust_delta;
        g.DragCurrentAccumDirty = true;
    }

    if (!g.DragCurrentAccumDirty)
        return false;

    TYPE v_cur = *v;
    FLOATTYPE v_old_ref_for_accum_remainder = static_cast<FLOATTYPE>(0.0f);

    float logarithmic_zero_epsilon = 0.0f; // Only valid when is_logarithmic is true
    const float zero_deadzone_halfsize = 0.0f; // Drag widgets have no deadzone (as it doesn't make sense)
    if (is_logarithmic)
    {
        // When using logarithmic sliders, we need to clamp to avoid hitting zero, but our choice of clamp value greatly affects slider precision. We attempt to use the specified precision to estimate a good lower bound.
        const int decimal_precision = is_floating_point ? VanParseFormatPrecision(format, 3) : 1;
        logarithmic_zero_epsilon = VanPow(0.1f, static_cast<float>(decimal_precision));

        // Convert to parametric space, apply delta, convert back
        float v_old_parametric = ScaleRatioFromValueT<TYPE, SIGNEDTYPE, FLOATTYPE>(data_type, v_cur, v_min, v_max, logarithmic_zero_epsilon, zero_deadzone_halfsize);
        float v_new_parametric = v_old_parametric + g.DragCurrentAccum;
        v_cur = ScaleValueFromRatioT<TYPE, SIGNEDTYPE, FLOATTYPE>(data_type, v_new_parametric, v_min, v_max, logarithmic_zero_epsilon, zero_deadzone_halfsize);
        v_old_ref_for_accum_remainder = v_old_parametric;
    }
    else
    {
        v_cur += static_cast<SIGNEDTYPE>(g.DragCurrentAccum);
    }

    // Round to user desired precision based on format string
    if (is_floating_point && !(flags & VanGuiSliderFlags_NoRoundToFormat))
        v_cur = RoundScalarWithFormatT<TYPE>(format, data_type, v_cur);

    // Preserve remainder after rounding has been applied. This also allow slow tweaking of values.
    g.DragCurrentAccumDirty = false;
    if (is_logarithmic)
    {
        // Convert to parametric space, apply delta, convert back
        float v_new_parametric = ScaleRatioFromValueT<TYPE, SIGNEDTYPE, FLOATTYPE>(data_type, v_cur, v_min, v_max, logarithmic_zero_epsilon, zero_deadzone_halfsize);
        g.DragCurrentAccum -= static_cast<float>(v_new_parametric - v_old_ref_for_accum_remainder);
    }
    else
    {
        g.DragCurrentAccum -= static_cast<float>(static_cast<SIGNEDTYPE>(v_cur) - static_cast<SIGNEDTYPE>(*v));
    }

    // Lose zero sign for float/double
    if (v_cur == static_cast<TYPE>(-0))
        v_cur = static_cast<TYPE>(0);

    if (*v != v_cur && is_bounded)
    {
        if (is_wrapped)
        {
            // Wrap values
            if (v_cur < v_min)
                v_cur += v_max - v_min + (is_floating_point ? 0 : 1);
            if (v_cur > v_max)
                v_cur -= v_max - v_min + (is_floating_point ? 0 : 1);
        }
        else
        {
            // Clamp values + handle overflow/wrap-around for integer types.
            if (v_cur < v_min || (v_cur > *v && adjust_delta < 0.0f && !is_floating_point))
                v_cur = v_min;
            if (v_cur > v_max || (v_cur < *v && adjust_delta > 0.0f && !is_floating_point))
                v_cur = v_max;
        }
    }

    // Apply result
    if (*v == v_cur)
        return false;
    *v = v_cur;
    return true;
}

bool VanGui::DragBehavior(VanGuiID id, VanGuiDataType data_type, void* p_v, float v_speed, const void* p_min, const void* p_max, const char* format, VanGuiSliderFlags flags)
{
    // Read vangui.cpp "API BREAKING CHANGES" section for 1.78 if you hit this assert.
    VAN_ASSERT((flags == 1 || (flags & VanGuiSliderFlags_InvalidMask_) == 0) && "Invalid VanGuiSliderFlags flags! Has the legacy 'float power' argument been mistakenly cast to flags? Call function with VanGuiSliderFlags_Logarithmic flags instead.");

    VanGuiContext& g = *GVanGui;
    if (g.ActiveId == id)
    {
        // Those are the things we can do easily outside the DragBehaviorT<> template, saves code generation.
        if (g.ActiveIdSource == VanGuiInputSource_Mouse && !g.IO.MouseDown[0])
            ClearActiveID();
        else if ((g.ActiveIdSource == VanGuiInputSource_Keyboard || g.ActiveIdSource == VanGuiInputSource_Gamepad) && g.NavActivatePressedId == id && !g.ActiveIdIsJustActivated)
            ClearActiveID();
    }
    if (g.ActiveId != id)
        return false;
    if ((g.LastItemData.ItemFlags & VanGuiItemFlags_ReadOnly) || (flags & VanGuiSliderFlags_ReadOnly))
        return false;

    switch (data_type)
    {
    case VanGuiDataType_S8:     { VanS32 v32 = static_cast<VanS32>(*reinterpret_cast<VanS8*>(p_v));  bool r = DragBehaviorT<VanS32, VanS32, float>(VanGuiDataType_S32, &v32, v_speed, p_min ? *(const VanS8*) p_min : VAN_S8_MIN,  p_max ? *(const VanS8*)p_max  : VAN_S8_MAX,  format, flags); if (r) *reinterpret_cast<VanS8*>(p_v) = static_cast<VanS8>(v32); return r; }
    case VanGuiDataType_U8:     { VanU32 v32 = static_cast<VanU32>(*reinterpret_cast<VanU8*>(p_v));  bool r = DragBehaviorT<VanU32, VanS32, float>(VanGuiDataType_U32, &v32, v_speed, p_min ? *(const VanU8*) p_min : VAN_U8_MIN,  p_max ? *(const VanU8*)p_max  : VAN_U8_MAX,  format, flags); if (r) *reinterpret_cast<VanU8*>(p_v) = static_cast<VanU8>(v32); return r; }
    case VanGuiDataType_S16:    { VanS32 v32 = static_cast<VanS32>(*reinterpret_cast<VanS16*>(p_v)); bool r = DragBehaviorT<VanS32, VanS32, float>(VanGuiDataType_S32, &v32, v_speed, p_min ? *(const VanS16*)p_min : VAN_S16_MIN, p_max ? *(const VanS16*)p_max : VAN_S16_MAX, format, flags); if (r) *reinterpret_cast<VanS16*>(p_v) = static_cast<VanS16>(v32); return r; }
    case VanGuiDataType_U16:    { VanU32 v32 = static_cast<VanU32>(*reinterpret_cast<VanU16*>(p_v)); bool r = DragBehaviorT<VanU32, VanS32, float>(VanGuiDataType_U32, &v32, v_speed, p_min ? *(const VanU16*)p_min : VAN_U16_MIN, p_max ? *(const VanU16*)p_max : VAN_U16_MAX, format, flags); if (r) *reinterpret_cast<VanU16*>(p_v) = static_cast<VanU16>(v32); return r; }
    case VanGuiDataType_S32:    return DragBehaviorT<VanS32, VanS32, float >(data_type, reinterpret_cast<VanS32*>(p_v),  v_speed, p_min ? *(const VanS32* )p_min : VAN_S32_MIN, p_max ? *(const VanS32* )p_max : VAN_S32_MAX, format, flags);
    case VanGuiDataType_U32:    return DragBehaviorT<VanU32, VanS32, float >(data_type, reinterpret_cast<VanU32*>(p_v),  v_speed, p_min ? *(const VanU32* )p_min : VAN_U32_MIN, p_max ? *(const VanU32* )p_max : VAN_U32_MAX, format, flags);
    case VanGuiDataType_S64:    return DragBehaviorT<VanS64, VanS64, double>(data_type, reinterpret_cast<VanS64*>(p_v),  v_speed, p_min ? *(const VanS64* )p_min : VAN_S64_MIN, p_max ? *(const VanS64* )p_max : VAN_S64_MAX, format, flags);
    case VanGuiDataType_U64:    return DragBehaviorT<VanU64, VanS64, double>(data_type, reinterpret_cast<VanU64*>(p_v),  v_speed, p_min ? *(const VanU64* )p_min : VAN_U64_MIN, p_max ? *(const VanU64* )p_max : VAN_U64_MAX, format, flags);
    case VanGuiDataType_Float:  return DragBehaviorT<float, float, float >(data_type, reinterpret_cast<float*>(p_v),  v_speed, p_min ? *(const float* )p_min : -FLT_MAX,   p_max ? *(const float* )p_max : FLT_MAX,    format, flags);
    case VanGuiDataType_Double: return DragBehaviorT<double,double,double>(data_type, reinterpret_cast<double*>(p_v), v_speed, p_min ? *(const double*)p_min : -DBL_MAX,   p_max ? *(const double*)p_max : DBL_MAX,    format, flags);
    case VanGuiDataType_COUNT:  break;
    }
    VAN_ASSERT(0);
    return false;
}

// Only clamp Ctrl+Click input when VanGuiSliderFlags_ClampOnInput is set (generally via VanGuiSliderFlags_AlwaysClamp)
static bool TempInputIsClampEnabled(VanGuiSliderFlags flags, VanGuiDataType data_type, const void* p_min, const void* p_max)
{
    if ((flags & VanGuiSliderFlags_ClampOnInput) && (p_min != nullptr || p_max != nullptr))
    {
        const int clamp_range_dir = (p_min != nullptr && p_max != nullptr) ? VanGui::DataTypeCompare(data_type, p_min, p_max) : 0; // -1 when *p_min < *p_max, == 0 when *p_min == *p_max
        if (p_min == nullptr || p_max == nullptr || clamp_range_dir < 0)
            return true;
        if (clamp_range_dir == 0)
            return VanGui::DataTypeIsZero(data_type, p_min) ? ((flags & VanGuiSliderFlags_ClampZeroRange) != 0) : true;
    }
    return false;
}

// Note: p_data, p_min and p_max are _pointers_ to a memory address holding the data. For a Drag widget, p_min and p_max are optional.
// Read code of e.g. DragFloat(), DragInt() etc. or examples in 'Demo->Widgets->Data Types' to understand how to use this function directly.
bool VanGui::DragScalar(const char* label, VanGuiDataType data_type, void* p_data, float v_speed, const void* p_min, const void* p_max, const char* format, VanGuiSliderFlags flags)
{
    VanGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    VanGuiContext& g = *GVanGui;
    const VanGuiStyle& style = g.Style;
    const VanGuiID id = window->GetID(label);
    const float w = CalcItemWidth();
    const VanU32 color_marker = (g.NextItemData.HasFlags & VanGuiNextItemDataFlags_HasColorMarker) ? g.NextItemData.ColorMarker : 0;

    const char* label_end = FindRenderedTextEnd(label);
    const VanVec2 label_size = CalcTextSize(label, label_end, false);
    const VanRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + VanVec2(w, label_size.y + style.FramePadding.y * 2.0f));
    const VanRect total_bb(frame_bb.Min, frame_bb.Max + VanVec2(label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f, 0.0f));

    const bool temp_input_allowed = (flags & VanGuiSliderFlags_NoInput) == 0;
    ItemSize(total_bb, style.FramePadding.y);
    if (!ItemAdd(total_bb, id, &frame_bb, temp_input_allowed ? VanGuiItemFlags_Inputable : 0))
        return false;

    // Default format string when passing nullptr
    if (format == nullptr)
        format = DataTypeGetInfo(data_type)->PrintFmt;

    const bool hovered = ItemHoverable(frame_bb, id, g.LastItemData.ItemFlags);
    bool temp_input_is_active = temp_input_allowed && TempInputIsActive(id);
    if (!temp_input_is_active)
    {
        // Tabbing or Ctrl+Click on Drag turns it into an InputText
        const bool clicked = hovered && IsMouseClicked(0, VanGuiInputFlags_None, id);
        const bool double_clicked = (hovered && g.IO.MouseClickedCount[0] == 2 && TestKeyOwner(VanGuiKey_MouseLeft, id));
        const bool make_active = (clicked || double_clicked || g.NavActivateId == id);
        if (make_active && (clicked || double_clicked))
            SetKeyOwner(VanGuiKey_MouseLeft, id);
        if (make_active && temp_input_allowed)
            if ((clicked && g.IO.KeyCtrl) || double_clicked || (g.NavActivateId == id && (g.NavActivateFlags & VanGuiActivateFlags_PreferInput)))
                temp_input_is_active = true;

        // (Optional) simple click (without moving) turns Drag into an InputText
        if (g.IO.ConfigDragClickToInputText && temp_input_allowed && !temp_input_is_active)
            if (g.ActiveId == id && hovered && g.IO.MouseReleased[0] && !IsMouseDragPastThreshold(0, g.IO.MouseDragThreshold * DRAG_MOUSE_THRESHOLD_FACTOR))
            {
                g.NavActivateId = id;
                g.NavActivateFlags = VanGuiActivateFlags_PreferInput;
                temp_input_is_active = true;
            }

        // Store initial value (not used by main lib but available as a convenience but some mods e.g. to revert)
        if (make_active)
            memcpy(&g.ActiveIdValueOnActivation, p_data, DataTypeGetInfo(data_type)->Size);

        if (make_active && !temp_input_is_active)
        {
            SetActiveID(id, window);
            SetFocusID(id, window);
            FocusWindow(window);
            g.ActiveIdUsingNavDirMask = (1 << VanGuiDir_Left) | (1 << VanGuiDir_Right);
        }
    }

    if (temp_input_is_active)
    {
        const bool clamp_enabled = TempInputIsClampEnabled(flags, data_type, p_min, p_max);
        return TempInputScalar(frame_bb, id, label, data_type, p_data, format, clamp_enabled ? p_min : nullptr, clamp_enabled ? p_max : nullptr);
    }

    // Draw frame
    const VanU32 frame_col = GetColorU32(g.ActiveId == id ? VanGuiCol_FrameBgActive : hovered ? VanGuiCol_FrameBgHovered : VanGuiCol_FrameBg);
    RenderNavCursor(frame_bb, id);
    RenderFrame(frame_bb.Min, frame_bb.Max, frame_col, false, style.FrameRounding);
    if (color_marker != 0 && style.ColorMarkerSize > 0.0f)
        RenderColorComponentMarker(frame_bb, GetColorU32(color_marker), style.FrameRounding);
    RenderFrameBorder(frame_bb.Min, frame_bb.Max, g.Style.FrameRounding);

    // Drag behavior
    const bool value_changed = DragBehavior(id, data_type, p_data, v_speed, p_min, p_max, format, flags);
    if (value_changed)
        MarkItemEdited(id);

    // Display value using user-provided display format so user can add prefix/suffix/decorations to the value.
    char value_buf[64];
    const char* value_buf_end = value_buf + DataTypeFormatString(value_buf, VAN_COUNTOF(value_buf), data_type, p_data, format);
    if (g.LogEnabled)
        LogSetNextTextDecoration("{", "}");
    RenderTextClipped(frame_bb.Min, frame_bb.Max, value_buf, value_buf_end, nullptr, VanVec2(0.5f, 0.5f));

    if (label_size.x > 0.0f)
        RenderText(VanVec2(frame_bb.Max.x + style.ItemInnerSpacing.x, frame_bb.Min.y + style.FramePadding.y), label, label_end, false);

    VANGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags | (temp_input_allowed ? VanGuiItemStatusFlags_Inputable : 0));
    return value_changed;
}

bool VanGui::DragScalarN(const char* label, VanGuiDataType data_type, void* p_data, int components, float v_speed, const void* p_min, const void* p_max, const char* format, VanGuiSliderFlags flags)
{
    VanGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    VanGuiContext& g = *GVanGui;
    bool value_changed = false;
    BeginGroup();
    PushID(label);
    PushMultiItemsWidths(components, CalcItemWidth());
    size_t type_size = GDataTypeInfo[data_type].Size;
    for (int i = 0; i < components; i++)
    {
        PushID(i);
        if (i > 0)
            SameLine(0, g.Style.ItemInnerSpacing.x);
        if (flags & VanGuiSliderFlags_ColorMarkers)
            SetNextItemColorMarker(GDefaultRgbaColorMarkers[i]);
        value_changed |= DragScalar("", data_type, p_data, v_speed, p_min, p_max, format, flags);
        PopID();
        PopItemWidth();
        p_data = static_cast<void*>(static_cast<char*>(p_data) + type_size);
    }
    PopID();

    const char* label_end = FindRenderedTextEnd(label);
    if (label != label_end)
    {
        SameLine(0, g.Style.ItemInnerSpacing.x);
        TextEx(label, label_end);
    }

    EndGroup();
    return value_changed;
}

bool VanGui::DragFloat(const char* label, float* v, float v_speed, float v_min, float v_max, const char* format, VanGuiSliderFlags flags)
{
    return DragScalar(label, VanGuiDataType_Float, v, v_speed, &v_min, &v_max, format, flags);
}

bool VanGui::DragFloat2(const char* label, float v[2], float v_speed, float v_min, float v_max, const char* format, VanGuiSliderFlags flags)
{
    return DragScalarN(label, VanGuiDataType_Float, v, 2, v_speed, &v_min, &v_max, format, flags);
}

bool VanGui::DragFloat3(const char* label, float v[3], float v_speed, float v_min, float v_max, const char* format, VanGuiSliderFlags flags)
{
    return DragScalarN(label, VanGuiDataType_Float, v, 3, v_speed, &v_min, &v_max, format, flags);
}

bool VanGui::DragFloat4(const char* label, float v[4], float v_speed, float v_min, float v_max, const char* format, VanGuiSliderFlags flags)
{
    return DragScalarN(label, VanGuiDataType_Float, v, 4, v_speed, &v_min, &v_max, format, flags);
}

// NB: You likely want to specify the VanGuiSliderFlags_AlwaysClamp when using this.
bool VanGui::DragFloatRange2(const char* label, float* v_current_min, float* v_current_max, float v_speed, float v_min, float v_max, const char* format, const char* format_max, VanGuiSliderFlags flags)
{
    VanGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    VanGuiContext& g = *GVanGui;
    PushID(label);
    BeginGroup();
    PushMultiItemsWidths(2, CalcItemWidth());

    float min_min = (v_min >= v_max) ? -FLT_MAX : v_min;
    float min_max = (v_min >= v_max) ? *v_current_max : VanMin(v_max, *v_current_max);
    VanGuiSliderFlags min_flags = flags | ((min_min == min_max) ? VanGuiSliderFlags_ReadOnly : 0);
    bool value_changed = DragScalar("##min", VanGuiDataType_Float, v_current_min, v_speed, &min_min, &min_max, format, min_flags);
    PopItemWidth();
    SameLine(0, g.Style.ItemInnerSpacing.x);

    float max_min = (v_min >= v_max) ? *v_current_min : VanMax(v_min, *v_current_min);
    float max_max = (v_min >= v_max) ? FLT_MAX : v_max;
    VanGuiSliderFlags max_flags = flags | ((max_min == max_max) ? VanGuiSliderFlags_ReadOnly : 0);
    value_changed |= DragScalar("##max", VanGuiDataType_Float, v_current_max, v_speed, &max_min, &max_max, format_max ? format_max : format, max_flags);
    PopItemWidth();
    SameLine(0, g.Style.ItemInnerSpacing.x);

    TextEx(label, FindRenderedTextEnd(label));
    EndGroup();
    PopID();

    return value_changed;
}

// NB: v_speed is float to allow adjusting the drag speed with more precision
bool VanGui::DragInt(const char* label, int* v, float v_speed, int v_min, int v_max, const char* format, VanGuiSliderFlags flags)
{
    return DragScalar(label, VanGuiDataType_S32, v, v_speed, &v_min, &v_max, format, flags);
}

bool VanGui::DragInt2(const char* label, int v[2], float v_speed, int v_min, int v_max, const char* format, VanGuiSliderFlags flags)
{
    return DragScalarN(label, VanGuiDataType_S32, v, 2, v_speed, &v_min, &v_max, format, flags);
}

bool VanGui::DragInt3(const char* label, int v[3], float v_speed, int v_min, int v_max, const char* format, VanGuiSliderFlags flags)
{
    return DragScalarN(label, VanGuiDataType_S32, v, 3, v_speed, &v_min, &v_max, format, flags);
}

bool VanGui::DragInt4(const char* label, int v[4], float v_speed, int v_min, int v_max, const char* format, VanGuiSliderFlags flags)
{
    return DragScalarN(label, VanGuiDataType_S32, v, 4, v_speed, &v_min, &v_max, format, flags);
}

// NB: You likely want to specify the VanGuiSliderFlags_AlwaysClamp when using this.
bool VanGui::DragIntRange2(const char* label, int* v_current_min, int* v_current_max, float v_speed, int v_min, int v_max, const char* format, const char* format_max, VanGuiSliderFlags flags)
{
    VanGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    VanGuiContext& g = *GVanGui;
    PushID(label);
    BeginGroup();
    PushMultiItemsWidths(2, CalcItemWidth());

    int min_min = (v_min >= v_max) ? INT_MIN : v_min;
    int min_max = (v_min >= v_max) ? *v_current_max : VanMin(v_max, *v_current_max);
    VanGuiSliderFlags min_flags = flags | ((min_min == min_max) ? VanGuiSliderFlags_ReadOnly : 0);
    bool value_changed = DragInt("##min", v_current_min, v_speed, min_min, min_max, format, min_flags);
    PopItemWidth();
    SameLine(0, g.Style.ItemInnerSpacing.x);

    int max_min = (v_min >= v_max) ? *v_current_min : VanMax(v_min, *v_current_min);
    int max_max = (v_min >= v_max) ? INT_MAX : v_max;
    VanGuiSliderFlags max_flags = flags | ((max_min == max_max) ? VanGuiSliderFlags_ReadOnly : 0);
    value_changed |= DragInt("##max", v_current_max, v_speed, max_min, max_max, format_max ? format_max : format, max_flags);
    PopItemWidth();
    SameLine(0, g.Style.ItemInnerSpacing.x);

    TextEx(label, FindRenderedTextEnd(label));
    EndGroup();
    PopID();

    return value_changed;
}

//-------------------------------------------------------------------------
// [SECTION] Widgets: SliderScalar, SliderFloat, SliderInt, etc.
//-------------------------------------------------------------------------
// - ScaleRatioFromValueT<> [Internal]
// - ScaleValueFromRatioT<> [Internal]
// - SliderBehaviorT<>() [Internal]
// - SliderBehavior() [Internal]
// - SliderScalar()
// - SliderScalarN()
// - SliderFloat()
// - SliderFloat2()
// - SliderFloat3()
// - SliderFloat4()
// - SliderAngle()
// - SliderInt()
// - SliderInt2()
// - SliderInt3()
// - SliderInt4()
// - VSliderScalar()
// - VSliderFloat()
// - VSliderInt()
//-------------------------------------------------------------------------

// Convert a value v in the output space of a slider into a parametric position on the slider itself (the logical opposite of ScaleValueFromRatioT)
template<typename TYPE, typename SIGNEDTYPE, typename FLOATTYPE>
float VanGui::ScaleRatioFromValueT(VanGuiDataType data_type, TYPE v, TYPE v_min, TYPE v_max, float logarithmic_zero_epsilon, float zero_deadzone_halfsize)
{
    if (v_min == v_max)
        return 0.0f;
    VAN_UNUSED(data_type);

    const TYPE v_clamped = (v_min < v_max) ? VanClamp(v, v_min, v_max) : VanClamp(v, v_max, v_min);
    if (logarithmic_zero_epsilon > 0.0f) // == is_logarithmic from caller
    {
        bool flipped = v_max < v_min;

        if (flipped) // Handle the case where the range is backwards
            VanSwap(v_min, v_max);

        // Fudge min/max to avoid getting close to log(0)
        FLOATTYPE v_min_fudged = (VanAbs(static_cast<FLOATTYPE>(v_min)) < logarithmic_zero_epsilon) ? ((v_min < 0.0f) ? -logarithmic_zero_epsilon : logarithmic_zero_epsilon) : static_cast<FLOATTYPE>(v_min);
        FLOATTYPE v_max_fudged = (VanAbs(static_cast<FLOATTYPE>(v_max)) < logarithmic_zero_epsilon) ? ((v_max < 0.0f) ? -logarithmic_zero_epsilon : logarithmic_zero_epsilon) : static_cast<FLOATTYPE>(v_max);

        // Awkward special cases - we need ranges of the form (-100 .. 0) to convert to (-100 .. -epsilon), not (-100 .. epsilon)
        if ((v_min == 0.0f) && (v_max < 0.0f))
            v_min_fudged = -logarithmic_zero_epsilon;
        else if ((v_max == 0.0f) && (v_min < 0.0f))
            v_max_fudged = -logarithmic_zero_epsilon;

        float result;
        if (v_clamped <= v_min_fudged)
            result = 0.0f; // Workaround for values that are in-range but below our fudge
        else if (v_clamped >= v_max_fudged)
            result = 1.0f; // Workaround for values that are in-range but above our fudge
        else if ((v_min * v_max) < 0.0f) // Range crosses zero, so split into two portions
        {
            float zero_point_center = (-static_cast<float>(v_min)) / (static_cast<float>(v_max) - static_cast<float>(v_min)); // The zero point in parametric space.  There's an argument we should take the logarithmic nature into account when calculating this, but for now this should do (and the most common case of a symmetrical range works fine)
            float zero_point_snap_L = zero_point_center - zero_deadzone_halfsize;
            float zero_point_snap_R = zero_point_center + zero_deadzone_halfsize;
            if (v == 0.0f)
                result = zero_point_center; // Special case for exactly zero
            else if (v < 0.0f)
                result = (1.0f - static_cast<float>(VanLog(-static_cast<FLOATTYPE>(v_clamped) / logarithmic_zero_epsilon) / VanLog(-v_min_fudged / logarithmic_zero_epsilon))) * zero_point_snap_L;
            else
                result = zero_point_snap_R + (static_cast<float>(VanLog(static_cast<FLOATTYPE>(v_clamped) / logarithmic_zero_epsilon) / VanLog(v_max_fudged / logarithmic_zero_epsilon)) * (1.0f - zero_point_snap_R));
        }
        else if ((v_min < 0.0f) || (v_max < 0.0f)) // Entirely negative slider
            result = 1.0f - static_cast<float>(VanLog(-static_cast<FLOATTYPE>(v_clamped) / -v_max_fudged) / VanLog(-v_min_fudged / -v_max_fudged));
        else
            result = static_cast<float>(VanLog(static_cast<FLOATTYPE>(v_clamped) / v_min_fudged) / VanLog(v_max_fudged / v_min_fudged));

        return flipped ? (1.0f - result) : result;
    }
    else
    {
        // Linear slider
        return static_cast<float>(static_cast<FLOATTYPE>(static_cast<SIGNEDTYPE>(v_clamped - v_min)) / static_cast<FLOATTYPE>(static_cast<SIGNEDTYPE>(v_max - v_min)));
    }
}

// Convert a parametric position on a slider into a value v in the output space (the logical opposite of ScaleRatioFromValueT)
template<typename TYPE, typename SIGNEDTYPE, typename FLOATTYPE>
TYPE VanGui::ScaleValueFromRatioT(VanGuiDataType data_type, float t, TYPE v_min, TYPE v_max, float logarithmic_zero_epsilon, float zero_deadzone_halfsize)
{
    // We special-case the extents because otherwise our logarithmic fudging can lead to "mathematically correct"
    // but non-intuitive behaviors like a fully-left slider not actually reaching the minimum value. Also generally simpler.
    if (t <= 0.0f || v_min == v_max)
        return v_min;
    if (t >= 1.0f)
        return v_max;

    TYPE result = static_cast<TYPE>(0);
    if (logarithmic_zero_epsilon > 0.0f) // == is_logarithmic from caller
    {
        // Fudge min/max to avoid getting silly results close to zero
        FLOATTYPE v_min_fudged = (VanAbs(static_cast<FLOATTYPE>(v_min)) < logarithmic_zero_epsilon) ? ((v_min < 0.0f) ? -logarithmic_zero_epsilon : logarithmic_zero_epsilon) : static_cast<FLOATTYPE>(v_min);
        FLOATTYPE v_max_fudged = (VanAbs(static_cast<FLOATTYPE>(v_max)) < logarithmic_zero_epsilon) ? ((v_max < 0.0f) ? -logarithmic_zero_epsilon : logarithmic_zero_epsilon) : static_cast<FLOATTYPE>(v_max);

        const bool flipped = v_max < v_min; // Check if range is "backwards"
        if (flipped)
            VanSwap(v_min_fudged, v_max_fudged);

        // Awkward special case - we need ranges of the form (-100 .. 0) to convert to (-100 .. -epsilon), not (-100 .. epsilon)
        if ((v_max == 0.0f) && (v_min < 0.0f))
            v_max_fudged = -logarithmic_zero_epsilon;

        float t_with_flip = flipped ? (1.0f - t) : t; // t, but flipped if necessary to account for us flipping the range

        if ((v_min * v_max) < 0.0f) // Range crosses zero, so we have to do this in two parts
        {
            float zero_point_center = (-static_cast<float>(VanMin(v_min, v_max))) / VanAbs(static_cast<float>(v_max) - static_cast<float>(v_min)); // The zero point in parametric space
            float zero_point_snap_L = zero_point_center - zero_deadzone_halfsize;
            float zero_point_snap_R = zero_point_center + zero_deadzone_halfsize;
            if (t_with_flip >= zero_point_snap_L && t_with_flip <= zero_point_snap_R)
                result = static_cast<TYPE>(0.0f); // Special case to make getting exactly zero possible (the epsilon prevents it otherwise)
            else if (t_with_flip < zero_point_center)
                result = static_cast<TYPE>(-(logarithmic_zero_epsilon * VanPow(-v_min_fudged / logarithmic_zero_epsilon, static_cast<FLOATTYPE>(1.0f - (t_with_flip / zero_point_snap_L)))));
            else
                result = static_cast<TYPE>(logarithmic_zero_epsilon * VanPow(v_max_fudged / logarithmic_zero_epsilon, static_cast<FLOATTYPE>((t_with_flip - zero_point_snap_R) / (1.0f - zero_point_snap_R))));
        }
        else if ((v_min < 0.0f) || (v_max < 0.0f)) // Entirely negative slider
            result = static_cast<TYPE>(-(-v_max_fudged * VanPow(-v_min_fudged / -v_max_fudged, static_cast<FLOATTYPE>(1.0f - t_with_flip))));
        else
            result = static_cast<TYPE>(v_min_fudged * VanPow(v_max_fudged / v_min_fudged, static_cast<FLOATTYPE>(t_with_flip)));
    }
    else
    {
        // Linear slider
        const bool is_floating_point = (data_type == VanGuiDataType_Float) || (data_type == VanGuiDataType_Double);
        if (is_floating_point)
        {
            result = VanLerp(v_min, v_max, t);
        }
        else if (t < 1.0)
        {
            // - For integer values we want the clicking position to match the grab box so we round above
            //   This code is carefully tuned to work with large values (e.g. high ranges of U64) while preserving this property..
            // - Not doing a *1.0 multiply at the end of a range as it tends to be lossy. While absolute aiming at a large s64/u64
            //   range is going to be imprecise anyway, with this check we at least make the edge values matches expected limits.
            FLOATTYPE v_new_off_f = static_cast<SIGNEDTYPE>(v_max - v_min) * t;
            result = static_cast<TYPE>(static_cast<SIGNEDTYPE>(v_min) + static_cast<SIGNEDTYPE>(v_new_off_f + static_cast<FLOATTYPE>(v_min > v_max ? -0.5 : 0.5)));
        }
    }

    return result;
}

// FIXME: Try to move more of the code into shared SliderBehavior()
template<typename TYPE, typename SIGNEDTYPE, typename FLOATTYPE>
bool VanGui::SliderBehaviorT(const VanRect& bb, VanGuiID id, VanGuiDataType data_type, TYPE* v, const TYPE v_min, const TYPE v_max, const char* format, VanGuiSliderFlags flags, VanRect* out_grab_bb)
{
    VanGuiContext& g = *GVanGui;
    const VanGuiStyle& style = g.Style;

    const VanGuiAxis axis = (flags & VanGuiSliderFlags_Vertical) ? VanGuiAxis_Y : VanGuiAxis_X;
    const bool is_logarithmic = (flags & VanGuiSliderFlags_Logarithmic) != 0;
    const bool is_floating_point = (data_type == VanGuiDataType_Float) || (data_type == VanGuiDataType_Double);
    const float v_range_f = static_cast<float>(v_min < v_max ? v_max - v_min : v_min - v_max); // We don't need high precision for what we do with it.

    // Calculate bounds
    const float grab_padding = 2.0f; // FIXME: Should be part of style.
    const float slider_sz = (bb.Max[axis] - bb.Min[axis]) - grab_padding * 2.0f;
    float grab_sz = style.GrabMinSize;
    if (!is_floating_point && v_range_f >= 0.0f)                         // v_range_f < 0 may happen on integer overflows
        grab_sz = VanMax(slider_sz / (v_range_f + 1), style.GrabMinSize); // For integer sliders: if possible have the grab size represent 1 unit
    grab_sz = VanMin(grab_sz, slider_sz);
    const float slider_usable_sz = slider_sz - grab_sz;
    const float slider_usable_pos_min = bb.Min[axis] + grab_padding + grab_sz * 0.5f;
    const float slider_usable_pos_max = bb.Max[axis] - grab_padding - grab_sz * 0.5f;

    float logarithmic_zero_epsilon = 0.0f; // Only valid when is_logarithmic is true
    float zero_deadzone_halfsize = 0.0f; // Only valid when is_logarithmic is true
    if (is_logarithmic)
    {
        // When using logarithmic sliders, we need to clamp to avoid hitting zero, but our choice of clamp value greatly affects slider precision. We attempt to use the specified precision to estimate a good lower bound.
        const int decimal_precision = is_floating_point ? VanParseFormatPrecision(format, 3) : 1;
        logarithmic_zero_epsilon = VanPow(0.1f, static_cast<float>(decimal_precision));
        zero_deadzone_halfsize = (style.LogSliderDeadzone * 0.5f) / VanMax(slider_usable_sz, 1.0f);
    }

    // Process interacting with the slider
    bool value_changed = false;
    if (g.ActiveId == id)
    {
        bool set_new_value = false;
        float clicked_t = 0.0f;
        if (g.ActiveIdSource == VanGuiInputSource_Mouse)
        {
            if (!g.IO.MouseDown[0])
            {
                ClearActiveID();
            }
            else
            {
                const float mouse_abs_pos = g.IO.MousePos[axis];
                if (g.ActiveIdIsJustActivated)
                {
                    float grab_t = ScaleRatioFromValueT<TYPE, SIGNEDTYPE, FLOATTYPE>(data_type, *v, v_min, v_max, logarithmic_zero_epsilon, zero_deadzone_halfsize);
                    if (axis == VanGuiAxis_Y)
                        grab_t = 1.0f - grab_t;
                    const float grab_pos = VanLerp(slider_usable_pos_min, slider_usable_pos_max, grab_t);
                    const bool clicked_around_grab = (mouse_abs_pos >= grab_pos - grab_sz * 0.5f - 1.0f) && (mouse_abs_pos <= grab_pos + grab_sz * 0.5f + 1.0f); // No harm being extra generous here.
                    g.SliderGrabClickOffset = (clicked_around_grab && is_floating_point) ? mouse_abs_pos - grab_pos : 0.0f;
                }
                if (slider_usable_sz > 0.0f)
                    clicked_t = VanSaturate((mouse_abs_pos - g.SliderGrabClickOffset - slider_usable_pos_min) / slider_usable_sz);
                if (axis == VanGuiAxis_Y)
                    clicked_t = 1.0f - clicked_t;
                set_new_value = true;
            }
        }
        else if (g.ActiveIdSource == VanGuiInputSource_Keyboard || g.ActiveIdSource == VanGuiInputSource_Gamepad)
        {
            if (g.ActiveIdIsJustActivated)
            {
                g.SliderCurrentAccum = 0.0f; // Reset any stored nav delta upon activation
                g.SliderCurrentAccumDirty = false;
            }

            float input_delta = (axis == VanGuiAxis_X) ? GetNavTweakPressedAmount(axis) : -GetNavTweakPressedAmount(axis);
            if (input_delta != 0.0f)
            {
                const bool tweak_slow = IsKeyDown((g.NavInputSource == VanGuiInputSource_Gamepad) ? VanGuiKey_NavGamepadTweakSlow : VanGuiKey_NavKeyboardTweakSlow);
                const bool tweak_fast = IsKeyDown((g.NavInputSource == VanGuiInputSource_Gamepad) ? VanGuiKey_NavGamepadTweakFast : VanGuiKey_NavKeyboardTweakFast);
                const int decimal_precision = is_floating_point ? VanParseFormatPrecision(format, 3) : 0;
                if (decimal_precision > 0)
                {
                    input_delta /= 100.0f; // Keyboard/Gamepad tweak speeds in % of slider bounds
                    if (tweak_slow)
                        input_delta /= 10.0f;
                }
                else
                {
                    if ((v_range_f >= -100.0f && v_range_f <= 100.0f && v_range_f != 0.0f) || tweak_slow)
                        input_delta = ((input_delta < 0.0f) ? -1.0f : +1.0f) / v_range_f; // Keyboard/Gamepad tweak speeds in integer steps
                    else
                        input_delta /= 100.0f;
                }
                if (tweak_fast)
                    input_delta *= 10.0f;

                g.SliderCurrentAccum += input_delta;
                g.SliderCurrentAccumDirty = true;
            }

            float delta = g.SliderCurrentAccum;
            if (g.NavActivatePressedId == id && !g.ActiveIdIsJustActivated)
            {
                ClearActiveID();
            }
            else if (g.SliderCurrentAccumDirty)
            {
                clicked_t = ScaleRatioFromValueT<TYPE, SIGNEDTYPE, FLOATTYPE>(data_type, *v, v_min, v_max, logarithmic_zero_epsilon, zero_deadzone_halfsize);

                if ((clicked_t >= 1.0f && delta > 0.0f) || (clicked_t <= 0.0f && delta < 0.0f)) // This is to avoid applying the saturation when already past the limits
                {
                    set_new_value = false;
                    g.SliderCurrentAccum = 0.0f; // If pushing up against the limits, don't continue to accumulate
                }
                else
                {
                    set_new_value = true;
                    float old_clicked_t = clicked_t;
                    clicked_t = VanSaturate(clicked_t + delta);

                    // Calculate what our "new" clicked_t will be, and thus how far we actually moved the slider, and subtract this from the accumulator
                    TYPE v_new = ScaleValueFromRatioT<TYPE, SIGNEDTYPE, FLOATTYPE>(data_type, clicked_t, v_min, v_max, logarithmic_zero_epsilon, zero_deadzone_halfsize);
                    if (is_floating_point && !(flags & VanGuiSliderFlags_NoRoundToFormat))
                        v_new = RoundScalarWithFormatT<TYPE>(format, data_type, v_new);
                    float new_clicked_t = ScaleRatioFromValueT<TYPE, SIGNEDTYPE, FLOATTYPE>(data_type, v_new, v_min, v_max, logarithmic_zero_epsilon, zero_deadzone_halfsize);

                    if (delta > 0)
                        g.SliderCurrentAccum -= VanMin(new_clicked_t - old_clicked_t, delta);
                    else
                        g.SliderCurrentAccum -= VanMax(new_clicked_t - old_clicked_t, delta);
                }

                g.SliderCurrentAccumDirty = false;
            }
        }

        if (set_new_value)
            if ((g.LastItemData.ItemFlags & VanGuiItemFlags_ReadOnly) || (flags & VanGuiSliderFlags_ReadOnly))
                set_new_value = false;

        if (set_new_value)
        {
            TYPE v_new = ScaleValueFromRatioT<TYPE, SIGNEDTYPE, FLOATTYPE>(data_type, clicked_t, v_min, v_max, logarithmic_zero_epsilon, zero_deadzone_halfsize);

            // Round to user desired precision based on format string
            if (is_floating_point && !(flags & VanGuiSliderFlags_NoRoundToFormat))
                v_new = RoundScalarWithFormatT<TYPE>(format, data_type, v_new);

            // Apply result
            if (*v != v_new)
            {
                *v = v_new;
                value_changed = true;
            }
        }
    }

    if (slider_sz < 1.0f)
    {
        *out_grab_bb = VanRect(bb.Min, bb.Min);
    }
    else
    {
        // Output grab position so it can be displayed by the caller
        float grab_t = ScaleRatioFromValueT<TYPE, SIGNEDTYPE, FLOATTYPE>(data_type, *v, v_min, v_max, logarithmic_zero_epsilon, zero_deadzone_halfsize);
        if (axis == VanGuiAxis_Y)
            grab_t = 1.0f - grab_t;
        const float grab_pos = VanLerp(slider_usable_pos_min, slider_usable_pos_max, grab_t);
        if (axis == VanGuiAxis_X)
            *out_grab_bb = VanRect(grab_pos - grab_sz * 0.5f, bb.Min.y + grab_padding, grab_pos + grab_sz * 0.5f, bb.Max.y - grab_padding);
        else
            *out_grab_bb = VanRect(bb.Min.x + grab_padding, grab_pos - grab_sz * 0.5f, bb.Max.x - grab_padding, grab_pos + grab_sz * 0.5f);
    }

    return value_changed;
}

// For 32-bit and larger types, slider bounds are limited to half the natural type range.
// So e.g. an integer Slider between INT_MAX-10 and INT_MAX will fail, but an integer Slider between INT_MAX/2-10 and INT_MAX/2 will be ok.
// It would be possible to lift that limitation with some work but it doesn't seem to be worth it for sliders.
bool VanGui::SliderBehavior(const VanRect& bb, VanGuiID id, VanGuiDataType data_type, void* p_v, const void* p_min, const void* p_max, const char* format, VanGuiSliderFlags flags, VanRect* out_grab_bb)
{
    // Read vangui.cpp "API BREAKING CHANGES" section for 1.78 if you hit this assert.
    VAN_ASSERT((flags == 1 || (flags & VanGuiSliderFlags_InvalidMask_) == 0) && "Invalid VanGuiSliderFlags flags! Has the legacy 'float power' argument been mistakenly cast to flags? Call function with VanGuiSliderFlags_Logarithmic flags instead.");
    VAN_ASSERT((flags & VanGuiSliderFlags_WrapAround) == 0); // Not supported by SliderXXX(), only by DragXXX()

    switch (data_type)
    {
    case VanGuiDataType_S8:  { VanS32 v32 = static_cast<VanS32>(*reinterpret_cast<VanS8*>(p_v));  bool r = SliderBehaviorT<VanS32, VanS32, float>(bb, id, VanGuiDataType_S32, &v32, *(const VanS8*)p_min,  *(const VanS8*)p_max,  format, flags, out_grab_bb); if (r) *reinterpret_cast<VanS8*>(p_v)  = static_cast<VanS8>(v32);  return r; }
    case VanGuiDataType_U8:  { VanU32 v32 = static_cast<VanU32>(*reinterpret_cast<VanU8*>(p_v));  bool r = SliderBehaviorT<VanU32, VanS32, float>(bb, id, VanGuiDataType_U32, &v32, *(const VanU8*)p_min,  *(const VanU8*)p_max,  format, flags, out_grab_bb); if (r) *reinterpret_cast<VanU8*>(p_v)  = static_cast<VanU8>(v32);  return r; }
    case VanGuiDataType_S16: { VanS32 v32 = static_cast<VanS32>(*reinterpret_cast<VanS16*>(p_v)); bool r = SliderBehaviorT<VanS32, VanS32, float>(bb, id, VanGuiDataType_S32, &v32, *(const VanS16*)p_min, *(const VanS16*)p_max, format, flags, out_grab_bb); if (r) *reinterpret_cast<VanS16*>(p_v) = static_cast<VanS16>(v32); return r; }
    case VanGuiDataType_U16: { VanU32 v32 = static_cast<VanU32>(*reinterpret_cast<VanU16*>(p_v)); bool r = SliderBehaviorT<VanU32, VanS32, float>(bb, id, VanGuiDataType_U32, &v32, *(const VanU16*)p_min, *(const VanU16*)p_max, format, flags, out_grab_bb); if (r) *reinterpret_cast<VanU16*>(p_v) = static_cast<VanU16>(v32); return r; }
    case VanGuiDataType_S32:
        VAN_ASSERT(*(const VanS32*)p_min >= VAN_S32_MIN / 2 && *(const VanS32*)p_max <= VAN_S32_MAX / 2);
        return SliderBehaviorT<VanS32, VanS32, float >(bb, id, data_type, reinterpret_cast<VanS32*>(p_v),  *(const VanS32*)p_min,  *(const VanS32*)p_max,  format, flags, out_grab_bb);
    case VanGuiDataType_U32:
        VAN_ASSERT(*(const VanU32*)p_max <= VAN_U32_MAX / 2);
        return SliderBehaviorT<VanU32, VanS32, float >(bb, id, data_type, reinterpret_cast<VanU32*>(p_v),  *(const VanU32*)p_min,  *(const VanU32*)p_max,  format, flags, out_grab_bb);
    case VanGuiDataType_S64:
        VAN_ASSERT(*(const VanS64*)p_min >= VAN_S64_MIN / 2 && *(const VanS64*)p_max <= VAN_S64_MAX / 2);
        return SliderBehaviorT<VanS64, VanS64, double>(bb, id, data_type, reinterpret_cast<VanS64*>(p_v),  *(const VanS64*)p_min,  *(const VanS64*)p_max,  format, flags, out_grab_bb);
    case VanGuiDataType_U64:
        VAN_ASSERT(*(const VanU64*)p_max <= VAN_U64_MAX / 2);
        return SliderBehaviorT<VanU64, VanS64, double>(bb, id, data_type, reinterpret_cast<VanU64*>(p_v),  *(const VanU64*)p_min,  *(const VanU64*)p_max,  format, flags, out_grab_bb);
    case VanGuiDataType_Float:
        VAN_ASSERT(*(const float*)p_min >= -FLT_MAX / 2.0f && *(const float*)p_max <= FLT_MAX / 2.0f);
        return SliderBehaviorT<float, float, float >(bb, id, data_type, reinterpret_cast<float*>(p_v),  *(const float*)p_min,  *(const float*)p_max,  format, flags, out_grab_bb);
    case VanGuiDataType_Double:
        VAN_ASSERT(*(const double*)p_min >= -DBL_MAX / 2.0f && *(const double*)p_max <= DBL_MAX / 2.0f);
        return SliderBehaviorT<double, double, double>(bb, id, data_type, reinterpret_cast<double*>(p_v), *(const double*)p_min, *(const double*)p_max, format, flags, out_grab_bb);
    case VanGuiDataType_COUNT: break;
    }
    VAN_ASSERT(0);
    return false;
}

// Note: p_data, p_min and p_max are _pointers_ to a memory address holding the data. For a slider, they are all required.
// Read code of e.g. SliderFloat(), SliderInt() etc. or examples in 'Demo->Widgets->Data Types' to understand how to use this function directly.
bool VanGui::SliderScalar(const char* label, VanGuiDataType data_type, void* p_data, const void* p_min, const void* p_max, const char* format, VanGuiSliderFlags flags)
{
    VanGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    VanGuiContext& g = *GVanGui;
    const VanGuiStyle& style = g.Style;
    const VanGuiID id = window->GetID(label);
    const float w = CalcItemWidth();
    const VanU32 color_marker = (g.NextItemData.HasFlags & VanGuiNextItemDataFlags_HasColorMarker) ? g.NextItemData.ColorMarker : 0;

    const char* label_end = FindRenderedTextEnd(label);
    const VanVec2 label_size = CalcTextSize(label, label_end, false);
    const VanRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + VanVec2(w, label_size.y + style.FramePadding.y * 2.0f));
    const VanRect total_bb(frame_bb.Min, frame_bb.Max + VanVec2(label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f, 0.0f));

    const bool temp_input_allowed = (flags & VanGuiSliderFlags_NoInput) == 0;
    ItemSize(total_bb, style.FramePadding.y);
    if (!ItemAdd(total_bb, id, &frame_bb, temp_input_allowed ? VanGuiItemFlags_Inputable : 0))
        return false;

    // Default format string when passing nullptr
    if (format == nullptr)
        format = DataTypeGetInfo(data_type)->PrintFmt;

    const bool hovered = ItemHoverable(frame_bb, id, g.LastItemData.ItemFlags);
    bool temp_input_is_active = temp_input_allowed && TempInputIsActive(id);
    if (!temp_input_is_active)
    {
        // Tabbing or Ctrl+Click on Slider turns it into an input box
        const bool clicked = hovered && IsMouseClicked(0, VanGuiInputFlags_None, id);
        const bool make_active = (clicked || g.NavActivateId == id);
        if (make_active && clicked)
            SetKeyOwner(VanGuiKey_MouseLeft, id);
        if (make_active && temp_input_allowed)
            if ((clicked && g.IO.KeyCtrl) || (g.NavActivateId == id && (g.NavActivateFlags & VanGuiActivateFlags_PreferInput)))
                temp_input_is_active = true;

        // Store initial value (not used by main lib but available as a convenience but some mods e.g. to revert)
        if (make_active)
            memcpy(&g.ActiveIdValueOnActivation, p_data, DataTypeGetInfo(data_type)->Size);

        if (make_active && !temp_input_is_active)
        {
            SetActiveID(id, window);
            SetFocusID(id, window);
            FocusWindow(window);
            g.ActiveIdUsingNavDirMask |= (1 << VanGuiDir_Left) | (1 << VanGuiDir_Right);
        }
    }

    if (temp_input_is_active)
    {
        // Only clamp Ctrl+Click input when VanGuiSliderFlags_ClampOnInput is set (generally via VanGuiSliderFlags_AlwaysClamp)
        const bool clamp_enabled = (flags & VanGuiSliderFlags_ClampOnInput) != 0; // Don't use TempInputIsClampEnabled()
        return TempInputScalar(frame_bb, id, label, data_type, p_data, format, clamp_enabled ? p_min : nullptr, clamp_enabled ? p_max : nullptr);
    }

    // Draw frame
    const VanU32 frame_col = GetColorU32(g.ActiveId == id ? VanGuiCol_FrameBgActive : hovered ? VanGuiCol_FrameBgHovered : VanGuiCol_FrameBg);
    RenderNavCursor(frame_bb, id);
    RenderFrame(frame_bb.Min, frame_bb.Max, frame_col, false, style.FrameRounding);
    if (color_marker != 0 && style.ColorMarkerSize > 0.0f)
        RenderColorComponentMarker(frame_bb, GetColorU32(color_marker), style.FrameRounding);
    RenderFrameBorder(frame_bb.Min, frame_bb.Max, g.Style.FrameRounding);

    // Slider behavior
    VanRect grab_bb;
    const bool value_changed = SliderBehavior(frame_bb, id, data_type, p_data, p_min, p_max, format, flags, &grab_bb);
    if (value_changed)
        MarkItemEdited(id);

    // Render grab
    if (grab_bb.Max.x > grab_bb.Min.x)
        window->DrawList->AddRectFilled(grab_bb.Min, grab_bb.Max, GetColorU32(g.ActiveId == id ? VanGuiCol_SliderGrabActive : VanGuiCol_SliderGrab), style.GrabRounding);

    // Display value using user-provided display format so user can add prefix/suffix/decorations to the value.
    char value_buf[64];
    const char* value_buf_end = value_buf + DataTypeFormatString(value_buf, VAN_COUNTOF(value_buf), data_type, p_data, format);
    if (g.LogEnabled)
        LogSetNextTextDecoration("{", "}");
    RenderTextClipped(frame_bb.Min, frame_bb.Max, value_buf, value_buf_end, nullptr, VanVec2(0.5f, 0.5f));

    if (label_size.x > 0.0f)
        RenderText(VanVec2(frame_bb.Max.x + style.ItemInnerSpacing.x, frame_bb.Min.y + style.FramePadding.y), label, label_end, false);

    VANGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags | (temp_input_allowed ? VanGuiItemStatusFlags_Inputable : 0));
    return value_changed;
}

// Add multiple sliders on 1 line for compact edition of multiple components
bool VanGui::SliderScalarN(const char* label, VanGuiDataType data_type, void* v, int components, const void* v_min, const void* v_max, const char* format, VanGuiSliderFlags flags)
{
    VanGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    VanGuiContext& g = *GVanGui;
    bool value_changed = false;
    BeginGroup();
    PushID(label);
    PushMultiItemsWidths(components, CalcItemWidth());
    size_t type_size = GDataTypeInfo[data_type].Size;
    for (int i = 0; i < components; i++)
    {
        PushID(i);
        if (i > 0)
            SameLine(0, g.Style.ItemInnerSpacing.x);
        if (flags & VanGuiSliderFlags_ColorMarkers)
            SetNextItemColorMarker(GDefaultRgbaColorMarkers[i]);
        value_changed |= SliderScalar("", data_type, v, v_min, v_max, format, flags);
        PopID();
        PopItemWidth();
        v = static_cast<void*>(static_cast<char*>(v) + type_size);
    }
    PopID();

    const char* label_end = FindRenderedTextEnd(label);
    if (label != label_end)
    {
        SameLine(0, g.Style.ItemInnerSpacing.x);
        TextEx(label, label_end);
    }

    EndGroup();
    return value_changed;
}

bool VanGui::SliderFloat(const char* label, float* v, float v_min, float v_max, const char* format, VanGuiSliderFlags flags)
{
    return SliderScalar(label, VanGuiDataType_Float, v, &v_min, &v_max, format, flags);
}

bool VanGui::SliderFloat2(const char* label, float v[2], float v_min, float v_max, const char* format, VanGuiSliderFlags flags)
{
    return SliderScalarN(label, VanGuiDataType_Float, v, 2, &v_min, &v_max, format, flags);
}

bool VanGui::SliderFloat3(const char* label, float v[3], float v_min, float v_max, const char* format, VanGuiSliderFlags flags)
{
    return SliderScalarN(label, VanGuiDataType_Float, v, 3, &v_min, &v_max, format, flags);
}

bool VanGui::SliderFloat4(const char* label, float v[4], float v_min, float v_max, const char* format, VanGuiSliderFlags flags)
{
    return SliderScalarN(label, VanGuiDataType_Float, v, 4, &v_min, &v_max, format, flags);
}

bool VanGui::SliderAngle(const char* label, float* v_rad, float v_degrees_min, float v_degrees_max, const char* format, VanGuiSliderFlags flags)
{
    if (format == nullptr)
        format = "%.0f deg";
    float v_deg = (*v_rad) * 360.0f / (2 * VAN_PI);
    bool value_changed = SliderFloat(label, &v_deg, v_degrees_min, v_degrees_max, format, flags);
    if (value_changed)
        *v_rad = v_deg * (2 * VAN_PI) / 360.0f;
    return value_changed;
}

bool VanGui::SliderInt(const char* label, int* v, int v_min, int v_max, const char* format, VanGuiSliderFlags flags)
{
    return SliderScalar(label, VanGuiDataType_S32, v, &v_min, &v_max, format, flags);
}

bool VanGui::SliderInt2(const char* label, int v[2], int v_min, int v_max, const char* format, VanGuiSliderFlags flags)
{
    return SliderScalarN(label, VanGuiDataType_S32, v, 2, &v_min, &v_max, format, flags);
}

bool VanGui::SliderInt3(const char* label, int v[3], int v_min, int v_max, const char* format, VanGuiSliderFlags flags)
{
    return SliderScalarN(label, VanGuiDataType_S32, v, 3, &v_min, &v_max, format, flags);
}

bool VanGui::SliderInt4(const char* label, int v[4], int v_min, int v_max, const char* format, VanGuiSliderFlags flags)
{
    return SliderScalarN(label, VanGuiDataType_S32, v, 4, &v_min, &v_max, format, flags);
}

bool VanGui::VSliderScalar(const char* label, const VanVec2& size, VanGuiDataType data_type, void* p_data, const void* p_min, const void* p_max, const char* format, VanGuiSliderFlags flags)
{
    VanGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    VanGuiContext& g = *GVanGui;
    const VanGuiStyle& style = g.Style;
    const VanGuiID id = window->GetID(label);

    const char* label_end = FindRenderedTextEnd(label);
    const VanVec2 label_size = CalcTextSize(label, label_end, false);
    const VanRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + size);
    const VanRect bb(frame_bb.Min, frame_bb.Max + VanVec2(label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f, 0.0f));

    ItemSize(bb, style.FramePadding.y);
    if (!ItemAdd(frame_bb, id))
        return false;

    // Default format string when passing nullptr
    if (format == nullptr)
        format = DataTypeGetInfo(data_type)->PrintFmt;

    const bool hovered = ItemHoverable(frame_bb, id, g.LastItemData.ItemFlags);
    const bool clicked = hovered && IsMouseClicked(0, VanGuiInputFlags_None, id);
    if (clicked || g.NavActivateId == id)
    {
        if (clicked)
            SetKeyOwner(VanGuiKey_MouseLeft, id);
        SetActiveID(id, window);
        SetFocusID(id, window);
        FocusWindow(window);
        g.ActiveIdUsingNavDirMask |= (1 << VanGuiDir_Up) | (1 << VanGuiDir_Down);
    }

    // Draw frame
    const VanU32 frame_col = GetColorU32(g.ActiveId == id ? VanGuiCol_FrameBgActive : hovered ? VanGuiCol_FrameBgHovered : VanGuiCol_FrameBg);
    RenderNavCursor(frame_bb, id);
    RenderFrame(frame_bb.Min, frame_bb.Max, frame_col, true, g.Style.FrameRounding);

    // Slider behavior
    VanRect grab_bb;
    const bool value_changed = SliderBehavior(frame_bb, id, data_type, p_data, p_min, p_max, format, flags | VanGuiSliderFlags_Vertical, &grab_bb);
    if (value_changed)
        MarkItemEdited(id);

    // Render grab
    if (grab_bb.Max.y > grab_bb.Min.y)
        window->DrawList->AddRectFilled(grab_bb.Min, grab_bb.Max, GetColorU32(g.ActiveId == id ? VanGuiCol_SliderGrabActive : VanGuiCol_SliderGrab), style.GrabRounding);

    // Display value using user-provided display format so user can add prefix/suffix/decorations to the value.
    // For the vertical slider we allow centered text to overlap the frame padding
    char value_buf[64];
    const char* value_buf_end = value_buf + DataTypeFormatString(value_buf, VAN_COUNTOF(value_buf), data_type, p_data, format);
    RenderTextClipped(VanVec2(frame_bb.Min.x, frame_bb.Min.y + style.FramePadding.y), frame_bb.Max, value_buf, value_buf_end, nullptr, VanVec2(0.5f, 0.0f));
    if (label_size.x > 0.0f)
        RenderText(VanVec2(frame_bb.Max.x + style.ItemInnerSpacing.x, frame_bb.Min.y + style.FramePadding.y), label, label_end, false);

    VANGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags);
    return value_changed;
}

bool VanGui::VSliderFloat(const char* label, const VanVec2& size, float* v, float v_min, float v_max, const char* format, VanGuiSliderFlags flags)
{
    return VSliderScalar(label, size, VanGuiDataType_Float, v, &v_min, &v_max, format, flags);
}

bool VanGui::VSliderInt(const char* label, const VanVec2& size, int* v, int v_min, int v_max, const char* format, VanGuiSliderFlags flags)
{
    return VSliderScalar(label, size, VanGuiDataType_S32, v, &v_min, &v_max, format, flags);
}

//-------------------------------------------------------------------------
// [SECTION] Widgets: InputScalar, InputFloat, InputInt, etc.
//-------------------------------------------------------------------------
// - VanParseFormatFindStart() [Internal]
// - VanParseFormatFindEnd() [Internal]
// - VanParseFormatTrimDecorations() [Internal]
// - VanParseFormatSanitizeForPrinting() [Internal]
// - VanParseFormatSanitizeForScanning() [Internal]
// - VanParseFormatPrecision() [Internal]
// - TempInputText() [Internal]
// - TempInputScalar() [Internal]
// - InputScalar()
// - InputScalarN()
// - InputFloat()
// - InputFloat2()
// - InputFloat3()
// - InputFloat4()
// - InputInt()
// - InputInt2()
// - InputInt3()
// - InputInt4()
// - InputDouble()
//-------------------------------------------------------------------------

// We don't use strchr() because our strings are usually very short and often start with '%'
const char* VanParseFormatFindStart(const char* fmt)
{
    while (char c = fmt[0])
    {
        if (c == '%' && fmt[1] != '%')
            return fmt;
        else if (c == '%')
            fmt++;
        fmt++;
    }
    return fmt;
}

const char* VanParseFormatFindEnd(const char* fmt)
{
    // Printf/scanf types modifiers: I/L/h/j/l/t/w/z. Other uppercase letters qualify as types aka end of the format.
    if (fmt[0] != '%')
        return fmt;
    const unsigned int ignored_uppercase_mask = (1 << ('I'-'A')) | (1 << ('L'-'A'));
    const unsigned int ignored_lowercase_mask = (1 << ('h'-'a')) | (1 << ('j'-'a')) | (1 << ('l'-'a')) | (1 << ('t'-'a')) | (1 << ('w'-'a')) | (1 << ('z'-'a'));
    for (char c; (c = *fmt) != 0; fmt++)
    {
        if (c >= 'A' && c <= 'Z' && ((1 << (c - 'A')) & ignored_uppercase_mask) == 0)
            return fmt + 1;
        if (c >= 'a' && c <= 'z' && ((1 << (c - 'a')) & ignored_lowercase_mask) == 0)
            return fmt + 1;
    }
    return fmt;
}

// Extract the format out of a format string with leading or trailing decorations
//  fmt = "blah blah"  -> return ""
//  fmt = "%.3f"       -> return fmt
//  fmt = "hello %.3f" -> return fmt + 6
//  fmt = "%.3f hello" -> return buf written with "%.3f"
const char* VanParseFormatTrimDecorations(const char* fmt, char* buf, size_t buf_size)
{
    const char* fmt_start = VanParseFormatFindStart(fmt);
    if (fmt_start[0] != '%')
        return "";
    const char* fmt_end = VanParseFormatFindEnd(fmt_start);
    if (fmt_end[0] == 0) // If we only have leading decoration, we don't need to copy the data.
        return fmt_start;
    VanStrncpy(buf, fmt_start, VanMin(static_cast<size_t>(fmt_end - fmt_start) + 1, buf_size));
    return buf;
}

// Sanitize format
// - Zero terminate so extra characters after format (e.g. "%f123") don't confuse atof/atoi
// - stb_sprintf.h supports several new modifiers which format numbers in a way that also makes them incompatible atof/atoi.
void VanParseFormatSanitizeForPrinting(const char* fmt_in, char* fmt_out, size_t fmt_out_size)
{
    const char* fmt_end = VanParseFormatFindEnd(fmt_in);
    VAN_UNUSED(fmt_out_size);
    VAN_ASSERT(static_cast<size_t>(fmt_end - fmt_in + 1) < fmt_out_size); // Format is too long, let us know if this happens to you!
    while (fmt_in < fmt_end)
    {
        char c = *fmt_in++;
        if (c != '\'' && c != '$' && c != '_') // Custom flags provided by stb_sprintf.h. POSIX 2008 also supports '.
            *(fmt_out++) = c;
    }
    *fmt_out = 0; // Zero-terminate
}

// - For scanning we need to remove all width and precision fields and flags "%+3.7f" -> "%f". BUT don't strip types like "%I64d" which includes digits. ! "%07I64d" -> "%I64d"
const char* VanParseFormatSanitizeForScanning(const char* fmt_in, char* fmt_out, size_t fmt_out_size)
{
    const char* fmt_end = VanParseFormatFindEnd(fmt_in);
    const char* fmt_out_begin = fmt_out;
    VAN_UNUSED(fmt_out_size);
    VAN_ASSERT(static_cast<size_t>(fmt_end - fmt_in + 1) < fmt_out_size); // Format is too long, let us know if this happens to you!
    bool has_type = false;
    while (fmt_in < fmt_end)
    {
        char c = *fmt_in++;
        if (!has_type && ((c >= '0' && c <= '9') || c == '.' || c == '+' || c == '#'))
            continue;
        has_type |= ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')); // Stop skipping digits
        if (c != '\'' && c != '$' && c != '_') // Custom flags provided by stb_sprintf.h. POSIX 2008 also supports '.
            *(fmt_out++) = c;
    }
    *fmt_out = 0; // Zero-terminate
    return fmt_out_begin;
}

template<typename TYPE>
static const char* VanAtoi(const char* src, TYPE* output)
{
    int negative = 0;
    if (*src == '-') { negative = 1; src++; }
    if (*src == '+') { src++; }
    TYPE v = 0;
    while (*src >= '0' && *src <= '9')
        v = (v * 10) + (*src++ - '0');
    *output = negative ? -v : v;
    return src;
}

// Parse display precision back from the display format string
// FIXME: This is still used by some navigation code path to infer a minimum tweak step, but we should aim to rework widgets so it isn't needed.
int VanParseFormatPrecision(const char* fmt, int default_precision)
{
    fmt = VanParseFormatFindStart(fmt);
    if (fmt[0] != '%')
        return default_precision;
    fmt++;
    while (*fmt >= '0' && *fmt <= '9')
        fmt++;
    int precision = INT_MAX;
    if (*fmt == '.')
    {
        fmt = VanAtoi<int>(fmt + 1, &precision);
        if (precision < 0 || precision > 99)
            precision = default_precision;
    }
    if (*fmt == 'e' || *fmt == 'E') // Maximum precision with scientific notation
        precision = -1;
    if ((*fmt == 'g' || *fmt == 'G') && precision == INT_MAX)
        precision = -1;
    return (precision == INT_MAX) ? default_precision : precision;
}

// Create text input in place of another active widget (e.g. used when doing a Ctrl+Click on drag/slider widgets)
// - This must be submitted right after the item it is overlaying.
// FIXME: Facilitate using this in variety of other situations.
bool VanGui::TempInputText(const VanRect& bb, VanGuiID id, const char* label, char* buf, size_t buf_size, VanGuiInputTextFlags flags, VanGuiInputTextCallback callback, void* user_data)
{
    // On the first frame, g.TempInputTextId == 0, then on subsequent frames it becomes == id.
    // We clear ActiveID on the first frame to allow the InputText() taking it back.
    VanGuiContext& g = *GVanGui;
    VanGuiWindow* window = g.CurrentWindow;

    const bool init = (g.TempInputId != id);
    if (init)
        ClearActiveID();

    VanVec2 backup_pos = window->DC.CursorPos;
    window->DC.CursorPos = bb.Min;
    g.LastItemData.ItemFlags |= VanGuiItemFlags_AllowDuplicateId; // Using VanGuiInputTextFlags_MergedItem above will skip ItemAdd() so we poke here.
    bool value_changed = InputTextEx(label, nullptr, buf, static_cast<int>(buf_size), bb.GetSize(), flags | VanGuiInputTextFlags_TempInput | VanGuiInputTextFlags_AutoSelectAll, callback, user_data);
    KeepAliveID(id); // Not done because of VanGuiInputTextFlags_TempInput
    if (init)
    {
        // First frame we started displaying the InputText widget, we expect it to take the active id.
        VAN_ASSERT(g.ActiveId == id);
        g.TempInputId = g.ActiveId;
    }
    if (g.ActiveId != id)
        g.TempInputId = 0;
    window->DC.CursorPos = backup_pos;
    return value_changed;
}

// Note that Drag/Slider functions are only forwarding the min/max values clamping values if the VanGuiSliderFlags_AlwaysClamp flag is set!
// This is intended: this way we allow Ctrl+Click manual input to set a value out of bounds, for maximum flexibility.
// However this may not be ideal for all uses, as some user code may break on out of bound values.
bool VanGui::TempInputScalar(const VanRect& bb, VanGuiID id, const char* label, VanGuiDataType data_type, void* p_data, const char* format, const void* p_clamp_min, const void* p_clamp_max)
{
    // FIXME: May need to clarify display behavior if format doesn't contain %.
    // "%d" -> "%d" / "There are %d items" -> "%d" / "items" -> "%d" (fallback). Also see #6405
    VanGuiContext& g = *GVanGui;
    const VanGuiDataTypeInfo* type_info = DataTypeGetInfo(data_type);
    char fmt_buf[32];
    char data_buf[32];
    format = VanParseFormatTrimDecorations(format, fmt_buf, VAN_COUNTOF(fmt_buf));
    if (format[0] == 0)
        format = type_info->PrintFmt;
    DataTypeFormatString(data_buf, VAN_COUNTOF(data_buf), data_type, p_data, format);
    VanStrTrimBlanks(data_buf);

    VanGuiInputTextFlags flags = VanGuiInputTextFlags_AutoSelectAll | static_cast<VanGuiInputTextFlags>(VanGuiInputTextFlags_LocalizeDecimalPoint);
    g.LastItemData.ItemFlags |= VanGuiItemFlags_NoMarkEdited; // Because TempInputText() uses VanGuiInputTextFlags_MergedItem it doesn't submit a new item, so we poke LastItemData.
    if (!TempInputText(bb, id, label, data_buf, VAN_COUNTOF(data_buf), flags))
        return false;

    // Backup old value
    size_t data_type_size = type_info->Size;
    VanGuiDataTypeStorage data_backup;
    memcpy(&data_backup, p_data, data_type_size);

    // Apply new value (or operations) then clamp
    DataTypeApplyFromText(data_buf, data_type, p_data, format, nullptr);
    if (p_clamp_min || p_clamp_max)
    {
        if (p_clamp_min && p_clamp_max && DataTypeCompare(data_type, p_clamp_min, p_clamp_max) > 0)
            VanSwap(p_clamp_min, p_clamp_max);
        DataTypeClamp(data_type, p_data, p_clamp_min, p_clamp_max);
    }

    // Only mark as edited if new value is different
    g.LastItemData.ItemFlags &= ~VanGuiItemFlags_NoMarkEdited;
    bool value_changed = memcmp(&data_backup, p_data, data_type_size) != 0;
    if (value_changed)
        MarkItemEdited(id);
    return value_changed;
}

void VanGui::SetNextItemRefVal(VanGuiDataType data_type, void* p_data)
{
    VanGuiContext& g = *GVanGui;
    g.NextItemData.HasFlags |= VanGuiNextItemDataFlags_HasRefVal;
    memcpy(&g.NextItemData.RefVal, p_data, DataTypeGetInfo(data_type)->Size);
}

// Note: p_data, p_step, p_step_fast are _pointers_ to a memory address holding the data. For an Input widget, p_step and p_step_fast are optional.
// Read code of e.g. InputFloat(), InputInt() etc. or examples in 'Demo->Widgets->Data Types' to understand how to use this function directly.
bool VanGui::InputScalar(const char* label, VanGuiDataType data_type, void* p_data, const void* p_step, const void* p_step_fast, const char* format, VanGuiInputTextFlags flags)
{
    VanGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    VanGuiContext& g = *GVanGui;
    VanGuiStyle& style = g.Style;
    //VAN_ASSERT((flags & VanGuiInputTextFlags_EnterReturnsTrue) == 0); // Not supported by InputScalar(). Please open an issue if you this would be useful to you. Otherwise use IsItemDeactivatedAfterEdit()!

    if (format == nullptr)
        format = DataTypeGetInfo(data_type)->PrintFmt;

    void* p_data_default = (g.NextItemData.HasFlags & VanGuiNextItemDataFlags_HasRefVal) ? &g.NextItemData.RefVal : &g.DataTypeZeroValue;

    char buf[64];
    if ((flags & VanGuiInputTextFlags_DisplayEmptyRefVal) && DataTypeCompare(data_type, p_data, p_data_default) == 0)
        buf[0] = 0;
    else
        DataTypeFormatString(buf, VAN_COUNTOF(buf), data_type, p_data, format);

    // Disable the MarkItemEdited() call in InputText but keep VanGuiItemStatusFlags_Edited.
    // We call MarkItemEdited() ourselves by comparing the actual data rather than the string.
    g.NextItemData.ItemFlags |= VanGuiItemFlags_NoMarkEdited;
    flags |= VanGuiInputTextFlags_AutoSelectAll | static_cast<VanGuiInputTextFlags>(VanGuiInputTextFlags_LocalizeDecimalPoint);

    const bool has_step_buttons = (p_step != nullptr);
    const float button_size = has_step_buttons ? GetFrameHeight() : 0.0f;
    bool ret;
    if (has_step_buttons)
    {
        // With Step Buttons
        BeginGroup(); // The only purpose of the group here is to allow the caller to query item data e.g. IsItemActive()
        PushID(label);
        SetNextItemWidth(VanMax(1.0f, CalcItemWidth() - (button_size + style.ItemInnerSpacing.x) * 2));
        ret = InputText("", buf, VAN_COUNTOF(buf), flags); // PushID(label) + "" gives us the expected ID from outside point of view
        VANGUI_TEST_ENGINE_ITEM_INFO(g.LastItemData.ID, label, g.LastItemData.StatusFlags | VanGuiItemStatusFlags_Inputable);
    }
    else
    {
        // Without Step Buttons
        ret = InputText(label, buf, VAN_COUNTOF(buf), flags);
    }

    // Apply
    bool input_edited = (g.LastItemData.StatusFlags & VanGuiItemStatusFlags_EditedInternal) != 0; // We would be using 'ret' if VanGuiInputTextFlags_EnterReturnsTrue was not involved.
    bool value_changed = input_edited ? DataTypeApplyFromText(buf, data_type, p_data, format, (flags & VanGuiInputTextFlags_ParseEmptyRefVal) ? p_data_default : nullptr) : false;

    // Step buttons
    if (has_step_buttons)
    {
        const VanVec2 backup_frame_padding = style.FramePadding;
        style.FramePadding.x = style.FramePadding.y;
        if (flags & VanGuiInputTextFlags_ReadOnly)
            BeginDisabled();
        PushItemFlag(VanGuiItemFlags_ButtonRepeat, true);
        SameLine(0, style.ItemInnerSpacing.x);
        if (ButtonEx("-", VanVec2(button_size, button_size)))
        {
            DataTypeApplyOp(data_type, '-', p_data, p_data, g.IO.KeyCtrl && p_step_fast ? p_step_fast : p_step);
            value_changed = ret = true;
        }
        SameLine(0, style.ItemInnerSpacing.x);
        if (ButtonEx("+", VanVec2(button_size, button_size)))
        {
            DataTypeApplyOp(data_type, '+', p_data, p_data, g.IO.KeyCtrl && p_step_fast ? p_step_fast : p_step);
            value_changed = ret = true;
        }
        PopItemFlag();
        if (flags & VanGuiInputTextFlags_ReadOnly)
            EndDisabled();

        const char* label_end = FindRenderedTextEnd(label);
        if (label != label_end)
        {
            SameLine(0, style.ItemInnerSpacing.x);
            TextEx(label, label_end);
        }
        style.FramePadding = backup_frame_padding;

        PopID();
        EndGroup();
    }

    g.LastItemData.ItemFlags &= ~VanGuiItemFlags_NoMarkEdited;
    if (value_changed)
        MarkItemEdited(g.LastItemData.ID);

    if (flags & VanGuiInputTextFlags_EnterReturnsTrue)
        return ret;
    return value_changed;
}

bool VanGui::InputScalarN(const char* label, VanGuiDataType data_type, void* p_data, int components, const void* p_step, const void* p_step_fast, const char* format, VanGuiInputTextFlags flags)
{
    VanGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    VanGuiContext& g = *GVanGui;
    bool value_changed = false;
    BeginGroup();
    PushID(label);
    PushMultiItemsWidths(components, CalcItemWidth());
    size_t type_size = GDataTypeInfo[data_type].Size;
    for (int i = 0; i < components; i++)
    {
        PushID(i);
        if (i > 0)
            SameLine(0, g.Style.ItemInnerSpacing.x);
        value_changed |= InputScalar("", data_type, p_data, p_step, p_step_fast, format, flags);
        PopID();
        PopItemWidth();
        p_data = static_cast<void*>(static_cast<char*>(p_data) + type_size);
    }
    PopID();

    const char* label_end = FindRenderedTextEnd(label);
    if (label != label_end)
    {
        SameLine(0.0f, g.Style.ItemInnerSpacing.x);
        TextEx(label, label_end);
    }

    EndGroup();
    return value_changed;
}

bool VanGui::InputFloat(const char* label, float* v, float step, float step_fast, const char* format, VanGuiInputTextFlags flags)
{
    return InputScalar(label, VanGuiDataType_Float, static_cast<void*>(v), static_cast<void*>(step > 0.0f ? &step : nullptr), static_cast<void*>(step_fast > 0.0f ? &step_fast : nullptr), format, flags);
}

bool VanGui::InputFloat2(const char* label, float v[2], const char* format, VanGuiInputTextFlags flags)
{
    return InputScalarN(label, VanGuiDataType_Float, v, 2, nullptr, nullptr, format, flags);
}

bool VanGui::InputFloat3(const char* label, float v[3], const char* format, VanGuiInputTextFlags flags)
{
    return InputScalarN(label, VanGuiDataType_Float, v, 3, nullptr, nullptr, format, flags);
}

bool VanGui::InputFloat4(const char* label, float v[4], const char* format, VanGuiInputTextFlags flags)
{
    return InputScalarN(label, VanGuiDataType_Float, v, 4, nullptr, nullptr, format, flags);
}

bool VanGui::InputInt(const char* label, int* v, int step, int step_fast, VanGuiInputTextFlags flags)
{
    // Hexadecimal input provided as a convenience but the flag name is awkward. Typically you'd use InputText() to parse your own data, if you want to handle prefixes.
    const char* format = (flags & VanGuiInputTextFlags_CharsHexadecimal) ? "%08X" : "%d";
    return InputScalar(label, VanGuiDataType_S32, static_cast<void*>(v), static_cast<void*>(step > 0 ? &step : nullptr), static_cast<void*>(step_fast > 0 ? &step_fast : nullptr), format, flags);
}

bool VanGui::InputInt2(const char* label, int v[2], VanGuiInputTextFlags flags)
{
    return InputScalarN(label, VanGuiDataType_S32, v, 2, nullptr, nullptr, "%d", flags);
}

bool VanGui::InputInt3(const char* label, int v[3], VanGuiInputTextFlags flags)
{
    return InputScalarN(label, VanGuiDataType_S32, v, 3, nullptr, nullptr, "%d", flags);
}

bool VanGui::InputInt4(const char* label, int v[4], VanGuiInputTextFlags flags)
{
    return InputScalarN(label, VanGuiDataType_S32, v, 4, nullptr, nullptr, "%d", flags);
}

bool VanGui::InputDouble(const char* label, double* v, double step, double step_fast, const char* format, VanGuiInputTextFlags flags)
{
    return InputScalar(label, VanGuiDataType_Double, static_cast<void*>(v), static_cast<void*>(step > 0.0 ? &step : nullptr), static_cast<void*>(step_fast > 0.0 ? &step_fast : nullptr), format, flags);
}

//-------------------------------------------------------------------------
// [SECTION] Widgets: InputText, InputTextMultiline, InputTextWithHint
//-------------------------------------------------------------------------
// - vanstb_textedit.h include
// - InputText()
// - InputTextWithHint()
// - InputTextMultiline()
// - InputTextEx() [Internal]
// - DebugNodeInputTextState() [Internal]
//-------------------------------------------------------------------------

namespace VanStb
{
#include "vanstb_textedit.h"
}

// If you want to use InputText() with std::string or any custom dynamic string type, use the wrapper in misc/cpp/vangui_stdlib.h/.cpp!
bool VanGui::InputText(const char* label, char* buf, size_t buf_size, VanGuiInputTextFlags flags, VanGuiInputTextCallback callback, void* user_data)
{
    VAN_ASSERT(!(flags & VanGuiInputTextFlags_Multiline)); // call InputTextMultiline()
    return InputTextEx(label, nullptr, buf, static_cast<int>(buf_size), VanVec2(0, 0), flags, callback, user_data);
}

bool VanGui::InputTextMultiline(const char* label, char* buf, size_t buf_size, const VanVec2& size, VanGuiInputTextFlags flags, VanGuiInputTextCallback callback, void* user_data)
{
    return InputTextEx(label, nullptr, buf, static_cast<int>(buf_size), size, flags | VanGuiInputTextFlags_Multiline, callback, user_data);
}

bool VanGui::InputTextWithHint(const char* label, const char* hint, char* buf, size_t buf_size, VanGuiInputTextFlags flags, VanGuiInputTextCallback callback, void* user_data)
{
    VAN_ASSERT(!(flags & VanGuiInputTextFlags_Multiline)); // call InputTextMultiline() or  InputTextEx() manually if you need multi-line + hint.
    return InputTextEx(label, hint, buf, static_cast<int>(buf_size), VanVec2(0, 0), flags, callback, user_data);
}

static VanVec2 InputTextCalcTextSize(VanGuiContext* ctx, const char* text_begin, const char* text_end_display, const char* text_end, const char** out_remaining, VanVec2* out_offset, VanDrawTextFlags flags)
{
    VanGuiContext& g = *ctx;
    VanGuiInputTextState* obj = &g.InputTextState;
    VAN_ASSERT(text_end_display >= text_begin && text_end_display <= text_end);
    return VanFontCalcTextSizeEx(g.Font, g.FontSize, FLT_MAX, obj->WrapWidth, text_begin, text_end_display, text_end, out_remaining, out_offset, flags);
}

// Wrapper for stb_textedit.h to edit text (our wrapper is for: statically sized buffer, single-line, wchar characters. InputText converts between UTF-8 and wchar)
// With our UTF-8 use of stb_textedit:
// - STB_TEXTEDIT_GETCHAR is nothing more than a a "GETBYTE". It's only used to compare to ascii or to copy blocks of text so we are fine.
// - One exception is the STB_TEXTEDIT_IS_SPACE feature which would expect a full char in order to handle full-width space such as 0x3000 (see VanCharIsBlankW).
// - ...but we don't use that feature.
namespace VanStb
{
static int     STB_TEXTEDIT_STRINGLEN(const VanGuiInputTextState* obj)                             { return obj->TextLen; }
static char    STB_TEXTEDIT_GETCHAR(const VanGuiInputTextState* obj, int idx)                      { VAN_ASSERT(idx >= 0 && idx <= obj->TextLen); return obj->TextSrc[idx]; }
static float   STB_TEXTEDIT_GETWIDTH(VanGuiInputTextState* obj, int line_start_idx, int char_idx)  { unsigned int c; VanTextCharFromUtf8(&c, obj->TextSrc + line_start_idx + char_idx, obj->TextSrc + obj->TextLen); if (static_cast<VanWchar>(c) == '\n') return VANSTB_TEXTEDIT_GETWIDTH_NEWLINE; VanGuiContext& g = *obj->Ctx; return g.FontBaked->GetCharAdvance(static_cast<VanWchar>(c)) * g.FontBakedScale; }
static char    STB_TEXTEDIT_NEWLINE = '\n';
static void    STB_TEXTEDIT_LAYOUTROW(StbTexteditRow* r, VanGuiInputTextState* obj, int line_start_idx)
{
    const char* text = obj->TextSrc;
    const char* text_remaining = nullptr;
    const VanVec2 size = InputTextCalcTextSize(obj->Ctx, text + line_start_idx, text + obj->TextLen, text + obj->TextLen, &text_remaining, nullptr, VanDrawTextFlags_StopOnNewLine | VanDrawTextFlags_WrapKeepBlanks);
    r->x0 = 0.0f;
    r->x1 = size.x;
    r->baseline_y_delta = size.y;
    r->ymin = 0.0f;
    r->ymax = size.y;
    r->num_chars = static_cast<int>(text_remaining - (text + line_start_idx));
}

#define VANSTB_TEXTEDIT_GETNEXTCHARINDEX  VANSTB_TEXTEDIT_GETNEXTCHARINDEX_IMPL
#define VANSTB_TEXTEDIT_GETPREVCHARINDEX  VANSTB_TEXTEDIT_GETPREVCHARINDEX_IMPL

static int VANSTB_TEXTEDIT_GETNEXTCHARINDEX_IMPL(VanGuiInputTextState* obj, int idx)
{
    if (idx >= obj->TextLen)
        return obj->TextLen + 1;
    unsigned int c;
    return idx + VanTextCharFromUtf8(&c, obj->TextSrc + idx, obj->TextSrc + obj->TextLen);
}

static int VANSTB_TEXTEDIT_GETPREVCHARINDEX_IMPL(VanGuiInputTextState* obj, int idx)
{
    if (idx <= 0)
        return -1;
    const char* p = VanTextFindPreviousUtf8Codepoint(obj->TextSrc, obj->TextSrc + idx);
    return static_cast<int>(p - obj->TextSrc);
}

static bool VanCharIsSeparatorW(unsigned int c)
{
    static const unsigned int separator_list[] =
    {
        ',', 0x3001, '.', 0x3002, ';', 0xFF1B, '(', 0xFF08, ')', 0xFF09, '{', 0xFF5B, '}', 0xFF5D,
        '[', 0x300C, ']', 0x300D, '|', 0xFF5C, '!', 0xFF01, '\\', 0xFFE5, '/', 0x30FB, 0xFF0F,
        '\n', '\r',
    };
    for (unsigned int separator : separator_list)
        if (c == separator)
            return true;
    return false;
}

static int is_word_boundary_from_right(VanGuiInputTextState* obj, int idx)
{
    // When VanGuiInputTextFlags_Password is set, we don't want actions such as Ctrl+Arrow to leak the fact that underlying data are blanks or separators.
    if ((obj->Flags & VanGuiInputTextFlags_Password) || idx <= 0)
        return 0;

    const char* curr_p = obj->TextSrc + idx;
    const char* prev_p = VanTextFindPreviousUtf8Codepoint(obj->TextSrc, curr_p);
    unsigned int curr_c; VanTextCharFromUtf8(&curr_c, curr_p, obj->TextSrc + obj->TextLen);
    unsigned int prev_c; VanTextCharFromUtf8(&prev_c, prev_p, obj->TextSrc + obj->TextLen);

    bool prev_white = VanCharIsBlankW(prev_c);
    bool prev_separ = VanCharIsSeparatorW(prev_c);
    bool curr_white = VanCharIsBlankW(curr_c);
    bool curr_separ = VanCharIsSeparatorW(curr_c);
    return ((prev_white || prev_separ) && !(curr_separ || curr_white)) || (curr_separ && !prev_separ);
}
static int is_word_boundary_from_left(VanGuiInputTextState* obj, int idx)
{
    if ((obj->Flags & VanGuiInputTextFlags_Password) || idx <= 0)
        return 0;

    const char* curr_p = obj->TextSrc + idx;
    const char* prev_p = VanTextFindPreviousUtf8Codepoint(obj->TextSrc, curr_p);
    unsigned int prev_c; VanTextCharFromUtf8(&prev_c, curr_p, obj->TextSrc + obj->TextLen);
    unsigned int curr_c; VanTextCharFromUtf8(&curr_c, prev_p, obj->TextSrc + obj->TextLen);

    bool prev_white = VanCharIsBlankW(prev_c);
    bool prev_separ = VanCharIsSeparatorW(prev_c);
    bool curr_white = VanCharIsBlankW(curr_c);
    bool curr_separ = VanCharIsSeparatorW(curr_c);
    return ((prev_white) && !(curr_separ || curr_white)) || (curr_separ && !prev_separ);
}
static int  STB_TEXTEDIT_MOVEWORDLEFT_IMPL(VanGuiInputTextState* obj, int idx)
{
    idx = VANSTB_TEXTEDIT_GETPREVCHARINDEX(obj, idx);
    while (idx >= 0 && !is_word_boundary_from_right(obj, idx))
        idx = VANSTB_TEXTEDIT_GETPREVCHARINDEX(obj, idx);
    return idx < 0 ? 0 : idx;
}
static int  STB_TEXTEDIT_MOVEWORDRIGHT_MAC(VanGuiInputTextState* obj, int idx)
{
    int len = obj->TextLen;
    idx = VANSTB_TEXTEDIT_GETNEXTCHARINDEX(obj, idx);
    while (idx < len && !is_word_boundary_from_left(obj, idx))
        idx = VANSTB_TEXTEDIT_GETNEXTCHARINDEX(obj, idx);
    return idx > len ? len : idx;
}
static int  STB_TEXTEDIT_MOVEWORDRIGHT_WIN(VanGuiInputTextState* obj, int idx)
{
    idx = VANSTB_TEXTEDIT_GETNEXTCHARINDEX(obj, idx);
    int len = obj->TextLen;
    while (idx < len && !is_word_boundary_from_right(obj, idx))
        idx = VANSTB_TEXTEDIT_GETNEXTCHARINDEX(obj, idx);
    return idx > len ? len : idx;
}
static int  STB_TEXTEDIT_MOVEWORDRIGHT_IMPL(VanGuiInputTextState* obj, int idx)  { VanGuiContext& g = *obj->Ctx; if (g.IO.ConfigMacOSXBehaviors) return STB_TEXTEDIT_MOVEWORDRIGHT_MAC(obj, idx); else return STB_TEXTEDIT_MOVEWORDRIGHT_WIN(obj, idx); }
#define STB_TEXTEDIT_MOVEWORDLEFT       STB_TEXTEDIT_MOVEWORDLEFT_IMPL  // They need to be #define for stb_textedit.h
#define STB_TEXTEDIT_MOVEWORDRIGHT      STB_TEXTEDIT_MOVEWORDRIGHT_IMPL

// Reimplementation of stb_textedit_move_line_start()/stb_textedit_move_line_end() which supports word-wrapping.
static int STB_TEXTEDIT_MOVELINESTART_IMPL(VanGuiInputTextState* obj, VanStb::STB_TexteditState* state, int cursor)
{
    if (state->single_line)
        return 0;

    if (obj->WrapWidth > 0.0f)
    {
        VanGuiContext& g = *obj->Ctx;
        const char* p_cursor = obj->TextSrc + cursor;
        const char* p_bol = VanStrbol(p_cursor, obj->TextSrc);
        const char* p = p_bol;
        const char* text_end = obj->TextSrc + obj->TextLen; // End of line would be enough
        while (p >= p_bol)
        {
            const char* p_eol = VanFontCalcWordWrapPositionEx(g.Font, g.FontSize, p, text_end, obj->WrapWidth, VanDrawTextFlags_WrapKeepBlanks);
            if (p == p_cursor) // If we are already on a visible beginning-of-line, return real beginning-of-line (would be same as regular handler below)
                return static_cast<int>(p_bol - obj->TextSrc);
            if (p_eol == p_cursor && obj->TextA[cursor] != '\n' && obj->LastMoveDirectionLR == VanGuiDir_Left)
                return static_cast<int>(p_bol - obj->TextSrc);
            if (p_eol >= p_cursor)
                return static_cast<int>(p - obj->TextSrc);
            p = (*p_eol == '\n') ? p_eol + 1 : p_eol;
        }
    }

    // Regular handler, same as stb_textedit_move_line_start()
    while (cursor > 0)
    {
        int prev_cursor = VANSTB_TEXTEDIT_GETPREVCHARINDEX(obj, cursor);
        if (STB_TEXTEDIT_GETCHAR(obj, prev_cursor) == STB_TEXTEDIT_NEWLINE)
            break;
        cursor = prev_cursor;
    }
    return cursor;
}

static int STB_TEXTEDIT_MOVELINEEND_IMPL(VanGuiInputTextState* obj, VanStb::STB_TexteditState* state, int cursor)
{
    int n = STB_TEXTEDIT_STRINGLEN(obj);
    if (state->single_line)
        return n;

    if (obj->WrapWidth > 0.0f)
    {
        VanGuiContext& g = *obj->Ctx;
        const char* p_cursor = obj->TextSrc + cursor;
        const char* p = VanStrbol(p_cursor, obj->TextSrc);
        const char* text_end = obj->TextSrc + obj->TextLen; // End of line would be enough
        while (p < text_end)
        {
            const char* p_eol = VanFontCalcWordWrapPositionEx(g.Font, g.FontSize, p, text_end, obj->WrapWidth, VanDrawTextFlags_WrapKeepBlanks);
            cursor = static_cast<int>(p_eol - obj->TextSrc);
            if (p_eol == p_cursor && obj->LastMoveDirectionLR != VanGuiDir_Left) // If we are already on a visible end-of-line, switch to regular handle
                break;
            if (p_eol > p_cursor)
                return cursor;
            p = (*p_eol == '\n') ? p_eol + 1 : p_eol;
        }
    }
    // Regular handler, same as stb_textedit_move_line_end()
    while (cursor < n && STB_TEXTEDIT_GETCHAR(obj, cursor) != STB_TEXTEDIT_NEWLINE)
        cursor = VANSTB_TEXTEDIT_GETNEXTCHARINDEX(obj, cursor);
    return cursor;
}

#define STB_TEXTEDIT_MOVELINESTART      STB_TEXTEDIT_MOVELINESTART_IMPL
#define STB_TEXTEDIT_MOVELINEEND        STB_TEXTEDIT_MOVELINEEND_IMPL

static void STB_TEXTEDIT_DELETECHARS(VanGuiInputTextState* obj, int pos, int n)
{
    // Offset remaining text (+ copy zero terminator)
    VAN_ASSERT(obj->TextSrc == obj->TextA.Data);
    char* dst = obj->TextA.Data + pos;
    char* src = obj->TextA.Data + pos + n;
    memmove(dst, src, obj->TextLen - n - pos + 1);
    obj->EditedBefore = obj->EditedThisFrame = true;
    obj->TextLen -= n;
}

static int STB_TEXTEDIT_INSERTCHARS(VanGuiInputTextState* obj, int pos, const char* new_text, int new_text_len)
{
    const bool is_resizable = (obj->Flags & VanGuiInputTextFlags_CallbackResize) != 0;
    const int text_len = obj->TextLen;
    VAN_ASSERT(pos <= text_len);

    // We support partial insertion (with a mod in stb_textedit.h)
    const int avail = obj->BufCapacity - 1 - obj->TextLen;
    if (!is_resizable && new_text_len > avail)
        new_text_len = static_cast<int>(VanTextFindValidUtf8CodepointEnd(new_text, new_text + new_text_len, new_text + avail) - new_text); // Truncate to closest UTF-8 codepoint. Alternative: return 0 to cancel insertion.
    if (new_text_len == 0)
        return 0;

    // Grow internal buffer if needed
    VAN_ASSERT(obj->TextSrc == obj->TextA.Data);
    if (text_len + new_text_len + 1 > obj->TextA.Size && is_resizable)
    {
        obj->TextA.resize(text_len + VanClamp(new_text_len, 32, VanMax(256, new_text_len)) + 1);
        obj->TextSrc = obj->TextA.Data;
    }

    char* text = obj->TextA.Data;
    if (pos != text_len)
        memmove(text + pos + new_text_len, text + pos, static_cast<size_t>(text_len - pos));
    memcpy(text + pos, new_text, static_cast<size_t>(new_text_len));

    obj->EditedBefore = obj->EditedThisFrame = true;
    obj->TextLen += new_text_len;
    obj->TextA[obj->TextLen] = '\0';

    return new_text_len;
}

// We don't use an enum so we can build even with conflicting symbols (if another user of stb_textedit.h leak their STB_TEXTEDIT_K_* symbols)
#define STB_TEXTEDIT_K_LEFT         0x200000 // keyboard input to move cursor left
#define STB_TEXTEDIT_K_RIGHT        0x200001 // keyboard input to move cursor right
#define STB_TEXTEDIT_K_UP           0x200002 // keyboard input to move cursor up
#define STB_TEXTEDIT_K_DOWN         0x200003 // keyboard input to move cursor down
#define STB_TEXTEDIT_K_LINESTART    0x200004 // keyboard input to move cursor to start of line
#define STB_TEXTEDIT_K_LINEEND      0x200005 // keyboard input to move cursor to end of line
#define STB_TEXTEDIT_K_TEXTSTART    0x200006 // keyboard input to move cursor to start of text
#define STB_TEXTEDIT_K_TEXTEND      0x200007 // keyboard input to move cursor to end of text
#define STB_TEXTEDIT_K_DELETE       0x200008 // keyboard input to delete selection or character under cursor
#define STB_TEXTEDIT_K_BACKSPACE    0x200009 // keyboard input to delete selection or character left of cursor
#define STB_TEXTEDIT_K_UNDO         0x20000A // keyboard input to perform undo
#define STB_TEXTEDIT_K_REDO         0x20000B // keyboard input to perform redo
#define STB_TEXTEDIT_K_WORDLEFT     0x20000C // keyboard input to move cursor left one word
#define STB_TEXTEDIT_K_WORDRIGHT    0x20000D // keyboard input to move cursor right one word
#define STB_TEXTEDIT_K_PGUP         0x20000E // keyboard input to move cursor up a page
#define STB_TEXTEDIT_K_PGDOWN       0x20000F // keyboard input to move cursor down a page
#define STB_TEXTEDIT_K_SHIFT        0x400000

#define VANSTB_TEXTEDIT_IMPLEMENTATION
#define VANSTB_TEXTEDIT_memmove memmove
#include "vanstb_textedit.h"

// stb_textedit internally allows for a single undo record to do addition and deletion, but somehow, calling
// the stb_textedit_paste() function creates two separate records, so we perform it manually. (FIXME: Report to nothings/stb?)
static void stb_textedit_replace(VanGuiInputTextState* str, STB_TexteditState* state, const VANSTB_TEXTEDIT_CHARTYPE* text, int text_len)
{
    stb_text_makeundo_replace(str, state, 0, str->TextLen, text_len);
    VanStb::STB_TEXTEDIT_DELETECHARS(str, 0, str->TextLen);
    state->cursor = state->select_start = state->select_end = 0;
    if (text_len <= 0)
        return;
    int text_len_inserted = VanStb::STB_TEXTEDIT_INSERTCHARS(str, 0, text, text_len);
    if (text_len_inserted > 0)
    {
        state->cursor = state->select_start = state->select_end = text_len;
        state->has_preferred_x = 0;
        return;
    }
    VAN_ASSERT(0); // Failed to insert character, normally shouldn't happen because of how we currently use stb_textedit_replace()
}

} // namespace VanStb

// We added an extra indirection where 'Stb' is heap-allocated, in order facilitate the work of bindings generators.
VanGuiInputTextState::VanGuiInputTextState()
{
    memset(static_cast<void*>(this), 0, sizeof(*this));
    Stb = VAN_NEW(VanStbTexteditState);
    memset(Stb, 0, sizeof(*Stb));
}

VanGuiInputTextState::~VanGuiInputTextState()
{
    VAN_DELETE(Stb);
}

void VanGuiInputTextState::OnKeyPressed(int key)
{
    stb_textedit_key(this, Stb, key);
    CursorFollow = true;
    CursorAnimReset();
    const int key_u = (key & ~STB_TEXTEDIT_K_SHIFT);
    if (key_u == STB_TEXTEDIT_K_LEFT || key_u == STB_TEXTEDIT_K_LINESTART || key_u == STB_TEXTEDIT_K_TEXTSTART || key_u == STB_TEXTEDIT_K_BACKSPACE || key_u == STB_TEXTEDIT_K_WORDLEFT)
        LastMoveDirectionLR = VanGuiDir_Left;
    else if (key_u == STB_TEXTEDIT_K_RIGHT || key_u == STB_TEXTEDIT_K_LINEEND || key_u == STB_TEXTEDIT_K_TEXTEND || key_u == STB_TEXTEDIT_K_DELETE || key_u == STB_TEXTEDIT_K_WORDRIGHT)
        LastMoveDirectionLR = VanGuiDir_Right;
}

void VanGuiInputTextState::OnCharPressed(unsigned int c)
{
    // Convert the key to a UTF8 byte sequence.
    // The changes we had to make to stb_textedit_key made it very much UTF-8 specific which is not too great.
    char utf8[5];
    VanTextCharToUtf8(utf8, c);
    stb_textedit_text(this, Stb, utf8, static_cast<int>(VanStrlen(utf8)));
    CursorFollow = true;
    CursorAnimReset();
}

// Those functions are not inlined in vangui_internal.h, allowing us to hide VanStbTexteditState from that header.
void VanGuiInputTextState::CursorAnimReset()                 { CursorAnim = -0.30f; } // After a user-input the cursor stays on for a while without blinking
void VanGuiInputTextState::CursorClamp()                     { Stb->cursor = VanMin(Stb->cursor, TextLen); Stb->select_start = VanMin(Stb->select_start, TextLen); Stb->select_end = VanMin(Stb->select_end, TextLen); }
bool VanGuiInputTextState::HasSelection() const              { return Stb->select_start != Stb->select_end; }
void VanGuiInputTextState::ClearSelection()                  { Stb->select_start = Stb->select_end = Stb->cursor; }
int  VanGuiInputTextState::GetCursorPos() const              { return Stb->cursor; }
int  VanGuiInputTextState::GetSelectionStart() const         { return Stb->select_start; }
int  VanGuiInputTextState::GetSelectionEnd() const           { return Stb->select_end; }
void VanGuiInputTextState::SetSelection(int start, int end)  { Stb->select_start = start; Stb->cursor = Stb->select_end = end; }
float VanGuiInputTextState::GetPreferredOffsetX() const      { return Stb->has_preferred_x ? Stb->preferred_x : -1; }
void VanGuiInputTextState::SelectAll()                       { Stb->select_start = 0; Stb->cursor = Stb->select_end = TextLen; Stb->has_preferred_x = 0; }
void VanGuiInputTextState::ReloadUserBufAndSelectAll()       { WantReloadUserBuf = true; ReloadSelectionStart = 0; ReloadSelectionEnd = INT_MAX; }
void VanGuiInputTextState::ReloadUserBufAndKeepSelection()   { WantReloadUserBuf = true; ReloadSelectionStart = Stb->select_start; ReloadSelectionEnd = Stb->select_end; }
void VanGuiInputTextState::ReloadUserBufAndMoveToEnd()       { WantReloadUserBuf = true; ReloadSelectionStart = ReloadSelectionEnd = INT_MAX; }

VanGuiInputTextCallbackData::VanGuiInputTextCallbackData()
{
    memset(static_cast<void*>(this), 0, sizeof(*this));
}

// Public API to manipulate UTF-8 text from within a callback.
// FIXME: The existence of this rarely exercised code path is a bit of a nuisance.
// Historically they existed because STB_TEXTEDIT_INSERTCHARS() etc. worked on our VanWchar
// buffer, but nowadays they both work on UTF-8 data. Should aim to merge both.
void VanGuiInputTextCallbackData::DeleteChars(int pos, int bytes_count)
{
    VAN_ASSERT(pos + bytes_count <= BufTextLen);
    char* dst = Buf + pos;
    const char* src = Buf + pos + bytes_count;
    memmove(dst, src, BufTextLen - bytes_count - pos + 1);

    if (CursorPos >= pos + bytes_count)
        CursorPos -= bytes_count;
    else if (CursorPos >= pos)
        CursorPos = pos;
    SelectionStart = SelectionEnd = CursorPos;
    BufDirty = true;
    BufTextLen -= bytes_count;
}

void VanGuiInputTextCallbackData::InsertChars(int pos, const char* new_text, const char* new_text_end)
{
    // Accept null ranges
    if (new_text == new_text_end)
        return;

    VanGuiContext& g = *Ctx;
    VanGuiInputTextState* obj = &g.InputTextState;
    VAN_ASSERT(obj->ID != 0 && g.ActiveId == obj->ID);
    const bool is_resizable = (Flags & VanGuiInputTextFlags_CallbackResize) != 0;
    const bool is_readonly = (Flags & VanGuiInputTextFlags_ReadOnly) != 0;
    int new_text_len = new_text_end ? static_cast<int>(new_text_end - new_text) : static_cast<int>(VanStrlen(new_text));

    // We support partial insertion (with a mod in stb_textedit.h)
    const int avail = BufSize - 1 - BufTextLen;
    if (!is_resizable && new_text_len > avail)
        new_text_len = static_cast<int>(VanTextFindValidUtf8CodepointEnd(new_text, new_text + new_text_len, new_text + avail) - new_text); // Truncate to closest UTF-8 codepoint. Alternative: return 0 to cancel insertion.
    if (new_text_len == 0)
        return;

    // Grow internal buffer if needed
    if (new_text_len + BufTextLen + 1 > obj->TextA.Size && is_resizable && !is_readonly)
    {
        VAN_ASSERT(Buf == obj->TextA.Data);
        int new_buf_size = BufTextLen + VanClamp(new_text_len * 4, 32, VanMax(256, new_text_len)) + 1;
        obj->TextA.resize(new_buf_size + 1);
        obj->TextSrc = obj->TextA.Data;
        Buf = obj->TextA.Data;
        BufSize = obj->BufCapacity = new_buf_size;
    }

    if (BufTextLen != pos)
        memmove(Buf + pos + new_text_len, Buf + pos, static_cast<size_t>(BufTextLen - pos));
    memcpy(Buf + pos, new_text, static_cast<size_t>(new_text_len) * sizeof(char));
    Buf[BufTextLen + new_text_len] = '\0';

    BufDirty = true;
    BufTextLen += new_text_len;
    if (CursorPos >= pos)
        CursorPos += new_text_len;
    CursorPos = VanClamp(CursorPos, 0, BufTextLen);
    SelectionStart = SelectionEnd = CursorPos;
}

void VanGui::PushPasswordFont()
{
    VanGuiContext& g = *GVanGui;
    VanFontBaked* backup = &g.InputTextPasswordFontBackupBaked;
    VAN_ASSERT(backup->IndexAdvanceX.Size == 0 && backup->IndexLookup.Size == 0);
    VanFontGlyph* glyph = g.FontBaked->FindGlyph('*');
    g.InputTextPasswordFontBackupFlags = g.Font->Flags;
    backup->FallbackGlyphIndex = g.FontBaked->FallbackGlyphIndex;
    backup->FallbackAdvanceX = g.FontBaked->FallbackAdvanceX;
    backup->IndexLookup.swap(g.FontBaked->IndexLookup);
    backup->IndexAdvanceX.swap(g.FontBaked->IndexAdvanceX);
    g.Font->Flags |= VanFontFlags_NoLoadGlyphs;
    g.FontBaked->FallbackGlyphIndex = g.FontBaked->Glyphs.index_from_ptr(glyph);
    g.FontBaked->FallbackAdvanceX = glyph->AdvanceX;
}

void VanGui::PopPasswordFont()
{
    VanGuiContext& g = *GVanGui;
    VanFontBaked* backup = &g.InputTextPasswordFontBackupBaked;
    g.Font->Flags = g.InputTextPasswordFontBackupFlags;
    g.FontBaked->FallbackGlyphIndex = backup->FallbackGlyphIndex;
    g.FontBaked->FallbackAdvanceX = backup->FallbackAdvanceX;
    g.FontBaked->IndexLookup.swap(backup->IndexLookup);
    g.FontBaked->IndexAdvanceX.swap(backup->IndexAdvanceX);
    VAN_ASSERT(backup->IndexAdvanceX.Size == 0 && backup->IndexLookup.Size == 0);
}

// Return false to discard a character.
static bool InputTextFilterCharacter(VanGuiContext* ctx, VanGuiInputTextState* state, unsigned int* p_char, VanGuiInputTextCallback callback, void* user_data, bool input_source_is_clipboard)
{
    VAN_ASSERT(state != nullptr);
    unsigned int c = *p_char;
    VanGuiInputTextFlags flags = state->Flags;

    // Filter non-printable (NB: isprint is unreliable! see #2467)
    bool apply_named_filters = true;
    if (c < 0x20)
    {
        bool pass = false;
        pass |= (c == '\n') && (flags & VanGuiInputTextFlags_Multiline) != 0;    // Note that an Enter KEY will emit \r and be ignored (we poll for KEY in InputText() code)
        if (c == '\n' && input_source_is_clipboard && (flags & VanGuiInputTextFlags_Multiline) == 0) // In single line mode, replace \n with a space
        {
            c = *p_char = ' ';
            pass = true;
        }
        pass |= (c == '\n') && (flags & VanGuiInputTextFlags_Multiline) != 0;
        pass |= (c == '\t') && (flags & VanGuiInputTextFlags_AllowTabInput) != 0;
        if (!pass)
            return false;
        apply_named_filters = false; // Override named filters below so newline and tabs can still be inserted.
    }

    if (input_source_is_clipboard == false)
    {
        // We ignore Ascii representation of delete (emitted from Backspace on OSX, see #2578, #2817)
        if (c == 127)
            return false;

        // Filter private Unicode range. GLFW on OSX seems to send private characters for special keys like arrow keys (FIXME)
        if (c >= 0xE000 && c <= 0xF8FF)
            return false;
    }

    // Filter Unicode ranges we are not handling in this build
    if (c > VAN_UNICODE_CODEPOINT_MAX)
        return false;

    // Generic named filters
    if (apply_named_filters && (flags & (VanGuiInputTextFlags_CharsDecimal | VanGuiInputTextFlags_CharsHexadecimal | VanGuiInputTextFlags_CharsUppercase | VanGuiInputTextFlags_CharsNoBlank | VanGuiInputTextFlags_CharsScientific | static_cast<VanGuiInputTextFlags>(VanGuiInputTextFlags_LocalizeDecimalPoint))))
    {
        // The libc allows overriding locale, with e.g. 'setlocale(LC_NUMERIC, "de_DE.UTF-8");' which affect the output/input of printf/scanf to use e.g. ',' instead of '.'.
        // The standard mandate that programs starts in the "C" locale where the decimal point is '.'.
        // We don't really intend to provide widespread support for it, but out of empathy for people stuck with using odd API, we support the bare minimum aka overriding the decimal point.
        // Change the default decimal_point with:
        //   VanGui::GetPlatformIO()->Platform_LocaleDecimalPoint = *localeconv()->decimal_point;
        // Users of non-default decimal point (in particular ',') may be affected by word-selection logic (is_word_boundary_from_right/is_word_boundary_from_left) functions.
        VanGuiContext& g = *ctx;
        const unsigned c_decimal_point = static_cast<unsigned int>(g.PlatformIO.Platform_LocaleDecimalPoint);
        if (flags & (VanGuiInputTextFlags_CharsDecimal | VanGuiInputTextFlags_CharsScientific | static_cast<VanGuiInputTextFlags>(VanGuiInputTextFlags_LocalizeDecimalPoint)))
            if (c == '.' || c == ',')
                c = c_decimal_point;

        // Full-width -> half-width conversion for numeric fields: https://en.wikipedia.org/wiki/Halfwidth_and_Fullwidth_Forms_(Unicode_block)
        // While this is mostly convenient, this has the side-effect for uninformed users accidentally inputting full-width characters that they may
        // scratch their head as to why it works in numerical fields vs in generic text fields it would require support in the font.
        if (flags & (VanGuiInputTextFlags_CharsDecimal | VanGuiInputTextFlags_CharsScientific | VanGuiInputTextFlags_CharsHexadecimal))
            if (c >= 0xFF01 && c <= 0xFF5E)
                c = c - 0xFF01 + 0x21;

        // Allow 0-9 . - + * /
        if (flags & VanGuiInputTextFlags_CharsDecimal)
            if (!(c >= '0' && c <= '9') && (c != c_decimal_point) && (c != '-') && (c != '+') && (c != '*') && (c != '/'))
                return false;

        // Allow 0-9 . - + * / e E
        if (flags & VanGuiInputTextFlags_CharsScientific)
            if (!(c >= '0' && c <= '9') && (c != c_decimal_point) && (c != '-') && (c != '+') && (c != '*') && (c != '/') && (c != 'e') && (c != 'E'))
                return false;

        // Allow 0-9 a-F A-F
        if (flags & VanGuiInputTextFlags_CharsHexadecimal)
            if (!(c >= '0' && c <= '9') && !(c >= 'a' && c <= 'f') && !(c >= 'A' && c <= 'F'))
                return false;

        // Turn a-z into A-Z
        if (flags & VanGuiInputTextFlags_CharsUppercase)
            if (c >= 'a' && c <= 'z')
                c += static_cast<unsigned int>('A' - 'a');

        if (flags & VanGuiInputTextFlags_CharsNoBlank)
            if (VanCharIsBlankW(c))
                return false;

        *p_char = c;
    }

    // Custom callback filter
    if (flags & VanGuiInputTextFlags_CallbackCharFilter)
    {
        VanGuiContext& g = *GVanGui;
        VanGuiInputTextCallbackData callback_data;
        callback_data.Ctx = &g;
        callback_data.ID = state->ID;
        callback_data.Flags = flags;
        callback_data.EventFlag = VanGuiInputTextFlags_CallbackCharFilter;
        callback_data.EventChar = static_cast<VanWchar>(c);
        callback_data.EventActivated = (g.ActiveId == state->ID && g.ActiveIdIsJustActivated);
        callback_data.CursorPos = state->Stb->cursor;
        callback_data.SelectionStart = state->Stb->select_start;
        callback_data.SelectionEnd = state->Stb->select_end;
        callback_data.UserData = user_data;
        if (callback(&callback_data) != 0)
            return false;
        *p_char = callback_data.EventChar;
        if (!callback_data.EventChar)
            return false;
    }

    return true;
}

// Find the shortest single replacement we can make to get from old_buf to new_buf
// Note that this doesn't directly alter state->TextA, state->TextLen. They are expected to be made valid separately.
// FIXME: Ideally we should transition toward (1) making InsertChars()/DeleteChars() update undo-stack (2) discourage (and keep reconcile) or obsolete (and remove reconcile) accessing buffer directly.
static void InputTextReconcileUndoState(VanGuiInputTextState* state, const char* old_buf, int old_length, const char* new_buf, int new_length)
{
    const int shorter_length = VanMin(old_length, new_length);
    int first_diff;
    for (first_diff = 0; first_diff < shorter_length; first_diff++)
        if (old_buf[first_diff] != new_buf[first_diff])
            break;
    if (first_diff == old_length && first_diff == new_length)
        return;

    int old_last_diff = old_length   - 1;
    int new_last_diff = new_length - 1;
    for (; old_last_diff >= first_diff && new_last_diff >= first_diff; old_last_diff--, new_last_diff--)
        if (old_buf[old_last_diff] != new_buf[new_last_diff])
            break;

    const int insert_len = new_last_diff - first_diff + 1;
    const int delete_len = old_last_diff - first_diff + 1;
    if (insert_len > 0 || delete_len > 0)
        if (VANSTB_TEXTEDIT_CHARTYPE* p = stb_text_createundo(&state->Stb->undostate, first_diff, delete_len, insert_len))
            for (int i = 0; i < delete_len; i++)
                p[i] = old_buf[first_diff + i];
}

// As InputText() retain textual data and we currently provide a path for user to not retain it (via local variables)
// we need some form of hook to reapply data back to user buffer on deactivation frame. (#4714)
// It would be more desirable that we discourage users from taking advantage of the "user not retaining data" trick,
// but that more likely be attractive when we do have _NoLiveEdit flag available.
void VanGui::InputTextDeactivateHook(VanGuiID id)
{
    VanGuiContext& g = *GVanGui;
    VanGuiInputTextState* state = &g.InputTextState;
    if (id == 0 || state->ID != id)
        return;
    //VANGUI_DEBUG_LOG_ACTIVEID("InputTextDeactivateHook() id = 0x%08X\n", id);
    g.InputTextDeactivatedState.ID = state->ID;
    if (state->Flags & VanGuiInputTextFlags_ReadOnly)
    {
        g.InputTextDeactivatedState.TextA.resize(0); // In theory this data won't be used, but clear to be neat.
    }
    else
    {
        VAN_ASSERT(state->TextA.Data != 0);
        VAN_ASSERT(state->TextA[state->TextLen] == 0);
        g.InputTextDeactivatedState.TextA.resize(state->TextLen + 1);
        memcpy(g.InputTextDeactivatedState.TextA.Data, state->TextA.Data, state->TextLen + 1);
    }
}

static int* VanLowerBound(int* in_begin, int* in_end, int v)
{
    int* in_p = in_begin;
    for (size_t count = static_cast<size_t>(in_end - in_p); count > 0; )
    {
        size_t count2 = count >> 1;
        int* mid = in_p + count2;
        if (*mid < v)
        {
            in_p = ++mid;
            count -= count2 + 1;
        }
        else
        {
            count = count2;
        }
    }
    return in_p;
}

// FIXME-WORDWRAP: Bundle some of this into VanGuiTextIndex and/or extract as a different tool?
// 'max_output_buffer_size' happens to be a meaningful optimization to avoid writing the full line_index when not necessarily needed (e.g. very large buffer, scrolled up, inactive)
static int InputTextLineIndexBuild(VanGuiInputTextFlags flags, VanGuiTextIndex* line_index, const char* buf, const char* buf_end, float wrap_width, int max_output_buffer_size, const char** out_buf_end)
{
    VanGuiContext& g = *GVanGui;
    int size = 0;
    const char* s;
    bool trailing_line_already_counted = false;
    if (flags & VanGuiInputTextFlags_WordWrap)
    {
        for (s = buf; s < buf_end; s = (*s == '\n') ? s + 1 : s)
        {
            if (size++ <= max_output_buffer_size)
                line_index->Offsets.push_back(static_cast<int>(s - buf));
            s = VanFontCalcWordWrapPositionEx(g.Font, g.FontSize, s, buf_end, wrap_width, VanDrawTextFlags_WrapKeepBlanks);
        }
    }
    else if (buf_end != nullptr)
    {
        for (s = buf; s < buf_end; s = s ? s + 1 : buf_end)
        {
            if (size++ <= max_output_buffer_size)
                line_index->Offsets.push_back(static_cast<int>(s - buf));
            s = static_cast<const char*>(VanMemchr(s, '\n', buf_end - s));
        }
    }
    else
    {
        // Inactive path: we don't know buf_end ahead of time.
        const char* s_eol;
        for (s = buf; ; s = s_eol + 1)
        {
            if (size++ <= max_output_buffer_size)
                line_index->Offsets.push_back(static_cast<int>(s - buf));
            if ((s_eol = strchr(s, '\n')) != nullptr)
                continue;
            s += strlen(s);
            trailing_line_already_counted = true;
            break;
        }
    }
    if (out_buf_end != nullptr)
        *out_buf_end = buf_end = s;
    if (size == 0)
    {
        line_index->Offsets.push_back(0);
        size++;
    }
    if (buf_end > buf && buf_end[-1] == '\n' && !trailing_line_already_counted && size++ <= max_output_buffer_size)
        line_index->Offsets.push_back(static_cast<int>(buf_end - buf));
    return size;
}

static VanVec2 InputTextLineIndexGetPosOffset(VanGuiContext& g, VanGuiInputTextState* state, VanGuiTextIndex* line_index, const char* buf, const char* buf_end, int cursor_n)
{
    const char* cursor_ptr = buf + cursor_n;
    int* it_begin = line_index->Offsets.begin();
    int* it_end = line_index->Offsets.end();
    const int* it = VanLowerBound(it_begin, it_end, cursor_n);
    if (it > it_begin)
        if (it == it_end || *it != cursor_n || (state != nullptr && state->WrapWidth > 0.0f && state->LastMoveDirectionLR == VanGuiDir_Right && cursor_ptr[-1] != '\n' && cursor_ptr[-1] != 0))
            it--;

    const int line_no = (it == it_begin) ? 0 : line_index->Offsets.index_from_ptr(it);
    const char* line_start = line_index->get_line_begin(buf, line_no);
    VanVec2 offset;
    offset.x = InputTextCalcTextSize(&g, line_start, cursor_ptr, buf_end, nullptr, nullptr, VanDrawTextFlags_WrapKeepBlanks).x;
    offset.y = (line_no + 1) * g.FontSize;
    return offset;
}

// Edit a string of text
// - buf_size account for the zero-terminator, so a buf_size of 6 can hold "Hello" but not "Hello!".
//   This is so we can easily call InputText() on static arrays using ARRAYSIZE() and to match
//   Note that in std::string world, capacity() would omit 1 byte used by the zero-terminator.
// - When active, hold on a privately held copy of the text (and apply back to 'buf'). So changing 'buf' while the InputText is active has no effect.
// - If you want to use InputText() with std::string or any custom dynamic string type, use the wrapper in misc/cpp/vangui_stdlib.h/.cpp!
bool VanGui::InputTextEx(const char* label, const char* hint, char* buf, int buf_size, const VanVec2& size_arg, VanGuiInputTextFlags flags, VanGuiInputTextCallback callback, void* callback_user_data)
{
    VanGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    VAN_ASSERT(buf != nullptr && buf_size > 0);
    VAN_ASSERT(!((flags & VanGuiInputTextFlags_CallbackHistory) && (flags & VanGuiInputTextFlags_Multiline)));        // Can't use both together (they both use up/down keys)
    VAN_ASSERT(!((flags & VanGuiInputTextFlags_CallbackCompletion) && (flags & VanGuiInputTextFlags_AllowTabInput))); // Can't use both together (they both use tab key)
    VAN_ASSERT(!((flags & VanGuiInputTextFlags_ElideLeft) && (flags & VanGuiInputTextFlags_Multiline)));              // Multiline does not not work with left-trimming
    VAN_ASSERT((flags & VanGuiInputTextFlags_WordWrap) == 0 || (flags & VanGuiInputTextFlags_Password) == 0);         // WordWrap does not work with Password mode.
    VAN_ASSERT((flags & VanGuiInputTextFlags_WordWrap) == 0 || (flags & VanGuiInputTextFlags_Multiline) != 0);        // WordWrap does not work in single-line mode.

    VanGuiContext& g = *GVanGui;
    VanGuiIO& io = g.IO;
    const VanGuiStyle& style = g.Style;

    const bool RENDER_SELECTION_WHEN_INACTIVE = false;
    const bool is_multiline = (flags & VanGuiInputTextFlags_Multiline) != 0;

    if (is_multiline) // Open group before calling GetID() because groups tracks id created within their scope (including the scrollbar)
        BeginGroup();
    const VanGuiID id = window->GetID(label);
    const char* label_end = FindRenderedTextEnd(label);
    const VanVec2 label_size = CalcTextSize(label, label_end, false);
    const VanVec2 frame_size = CalcItemSize(size_arg, CalcItemWidth(), (is_multiline ? g.FontSize * 8.0f : label_size.y) + style.FramePadding.y * 2.0f); // Arbitrary default of 8 lines high for multi-line
    const VanVec2 total_size = VanVec2(frame_size.x + (label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f), frame_size.y);

    const VanRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + frame_size);
    const VanRect total_bb(frame_bb.Min, frame_bb.Min + total_size);

    VanGuiWindow* draw_window = window;
    VanVec2 inner_size = frame_size;
    VanGuiLastItemData item_data_backup;
    if (is_multiline)
    {
        VanVec2 backup_pos = window->DC.CursorPos;
        ItemSize(total_bb, style.FramePadding.y);
        bool no_clip = (g.InputTextDeactivatedState.ID == id) || (g.ActiveId == id) || (id == g.NavActivateId); // Mimic some of ItemAdd() logic + add InputTextDeactivatedState.ID check.
        if (!ItemAdd(total_bb, id, &frame_bb, VanGuiItemFlags_Inputable) && !no_clip)
        {
            EndGroup();
            return false;
        }
        item_data_backup = g.LastItemData;
        window->DC.CursorPos = backup_pos;

        // Prevent NavActivation from explicit Tabbing when our widget accepts Tab inputs: this allows cycling through widgets without stopping.
        if (g.NavActivateId == id && (g.NavActivateFlags & VanGuiActivateFlags_FromTabbing) && !(g.NavActivateFlags & VanGuiActivateFlags_FromFocusApi) && (flags & VanGuiInputTextFlags_AllowTabInput))
            g.NavActivateId = 0;

        // Prevent NavActivate reactivating in BeginChild() when we are already active.
        const VanGuiID backup_activate_id = g.NavActivateId;
        if (g.ActiveId == id) // Prevent reactivation
            g.NavActivateId = 0;

        // We reproduce the contents of BeginChildFrame() in order to provide 'label' so our window internal data are easier to read/debug.
        PushStyleColor(VanGuiCol_ChildBg, style.Colors[VanGuiCol_FrameBg]);
        PushStyleVar(VanGuiStyleVar_ChildRounding, style.FrameRounding);
        PushStyleVar(VanGuiStyleVar_ChildBorderSize, style.FrameBorderSize);
        PushStyleVar(VanGuiStyleVar_WindowPadding, VanVec2(0, 0)); // Ensure no clip rect so mouse hover can reach FramePadding edges
        bool child_visible = BeginChildEx(label, id, frame_bb.GetSize(), VanGuiChildFlags_Borders, VanGuiWindowFlags_NoMove);
        g.NavActivateId = backup_activate_id;
        PopStyleVar(3);
        PopStyleColor();
        if (!child_visible && !no_clip)
        {
            EndChild();
            EndGroup();
            return false;
        }
        draw_window = g.CurrentWindow; // Child window
        draw_window->DC.NavLayersActiveMaskNext |= (1 << draw_window->DC.NavLayerCurrent); // This is to ensure that EndChild() will display a navigation highlight so we can "enter" into it.
        draw_window->DC.CursorPos += style.FramePadding;
        inner_size.x -= draw_window->ScrollbarSizes.x;

        // FIXME: Could this be a VanGuiChildFlags to affect the SetLastItemDataForWindow() call?
        g.LastItemData.ID = id;
        g.LastItemData.ItemFlags = item_data_backup.ItemFlags;
        g.LastItemData.StatusFlags = item_data_backup.StatusFlags;
    }
    else
    {
        // Support for internal VanGuiInputTextFlags_MergedItem flag, which could be redesigned as an ItemFlags if needed (with test performed in ItemAdd)
        ItemSize(total_bb, style.FramePadding.y);
        if (!(flags & VanGuiInputTextFlags_TempInput))
            if (!ItemAdd(total_bb, id, &frame_bb, VanGuiItemFlags_Inputable))
                return false;
    }

    // Ensure mouse cursor is set even after switching to keyboard/gamepad mode. May generalize further? (#6417)
    bool hovered = ItemHoverable(frame_bb, id, g.LastItemData.ItemFlags | VanGuiItemFlags_NoNavDisableMouseHover);
    if (hovered)
        SetMouseCursor(VanGuiMouseCursor_TextInput);
    if (hovered && g.NavHighlightItemUnderNav)
        hovered = false;

    // We are only allowed to access the state if we are already the active widget.
    VanGuiInputTextState* state = GetInputTextState(id);

    if (g.LastItemData.ItemFlags & VanGuiItemFlags_ReadOnly)
        flags |= VanGuiInputTextFlags_ReadOnly;
    const bool is_readonly = (flags & VanGuiInputTextFlags_ReadOnly) != 0;
    const bool is_password = (flags & VanGuiInputTextFlags_Password) != 0;
    const bool is_undoable = (flags & VanGuiInputTextFlags_NoUndoRedo) == 0;
    const bool is_resizable = (flags & VanGuiInputTextFlags_CallbackResize) != 0;
    if (is_resizable)
        VAN_ASSERT(callback != nullptr); // Must provide a callback if you set the VanGuiInputTextFlags_CallbackResize flag!

    // Word-wrapping: enforcing a fixed width not altered by vertical scrollbar makes things easier, notably to track cursor reliably and avoid one-frame glitches.
    // Instead of using VanGuiWindowFlags_AlwaysVerticalScrollbar we account for that space if the scrollbar is not visible.
    const bool is_wordwrap = (flags & VanGuiInputTextFlags_WordWrap) != 0;
    float wrap_width = 0.0f;
    if (is_wordwrap)
        wrap_width = VanMax(1.0f, GetContentRegionAvail().x + (draw_window->ScrollbarY ? 0.0f : -g.Style.ScrollbarSize));

    const bool user_clicked = hovered && io.MouseClicked[0];
    const bool input_requested_by_nav = (g.ActiveId != id) && (g.NavActivateId == id);
    const bool input_requested_by_reactivate = (g.InputTextReactivateId == id); // for io.ConfigInputTextEnterKeepActive
    const bool input_requested_by_user = (user_clicked) || (g.ActiveId == 0 && (flags & VanGuiInputTextFlags_TempInput));
    const VanGuiID scrollbar_id = (is_multiline && state != nullptr) ? GetWindowScrollbarID(draw_window, VanGuiAxis_Y) : 0;
    const bool user_scroll_finish = is_multiline && state != nullptr && g.ActiveId == 0 && g.ActiveIdPreviousFrame == scrollbar_id;
    const bool user_scroll_active = is_multiline && state != nullptr && g.ActiveId == scrollbar_id;
    bool clear_active_id = false;
    bool select_all = false;

    float scroll_y = is_multiline ? draw_window->Scroll.y : FLT_MAX;

    const bool init_reload_from_user_buf = (state != nullptr && state->WantReloadUserBuf);
    const bool init_changed_specs_multiline = (state != nullptr && (state->Stb->single_line != !is_multiline)); // state != nullptr means its our state.
    const bool init_changed_specs_readonly = (state != nullptr && ((state->Flags ^ flags) & VanGuiInputTextFlags_ReadOnly)); // state != nullptr means its our state.
    const bool init_make_active = (input_requested_by_user || input_requested_by_nav || input_requested_by_reactivate || user_scroll_finish);
    if (init_reload_from_user_buf)
    {
        int new_len = static_cast<int>(VanStrlen(buf));
        VAN_ASSERT(new_len + 1 <= buf_size && "Is your input buffer properly zero-terminated?");
        state->WantReloadUserBuf = false;
        InputTextReconcileUndoState(state, state->TextA.Data, state->TextLen, buf, new_len);
        state->TextA.resize(buf_size + 1); // we use +1 to make sure that .Data is always pointing to at least an empty string.
        state->TextLen = new_len;
        memcpy(state->TextA.Data, buf, state->TextLen + 1);
        state->Stb->select_start = state->ReloadSelectionStart;
        state->Stb->cursor = state->Stb->select_end = state->ReloadSelectionEnd; // will be clamped to bounds below
    }
    else if ((init_make_active && g.ActiveId != id) || init_changed_specs_multiline || init_changed_specs_readonly)
    {
        // Access state even if we don't own it yet.
        state = &g.InputTextState;
        state->CursorAnimReset();

        // Backup state of deactivating item so they'll have a chance to do a write to output buffer on the same frame they report IsItemDeactivatedAfterEdit (#4714)
        InputTextDeactivateHook(state->ID);

        // Take a copy of the initial buffer value.
        // From the moment we focused we are normally ignoring the content of 'buf' (unless we are in read-only mode)
        const int buf_len = static_cast<int>(VanStrlen(buf));
        VAN_ASSERT(((buf_len + 1 <= buf_size) || (buf_len == 0 && buf_size == 0)) && "Is your input buffer properly zero-terminated?");
        if (!user_scroll_finish)
        {
            state->TextToRevertTo.resize(buf_len + 1);    // UTF-8. we use +1 to make sure that .Data is always pointing to at least an empty string.
            memcpy(state->TextToRevertTo.Data, buf, buf_len + 1);
        }

        // Preserve cursor position and undo/redo stack if we come back to same widget
        // FIXME: Since we reworked this on 2022/06, may want to differentiate recycle_cursor vs recycle_undostate?
        bool recycle_state = (state->ID == id && !init_changed_specs_multiline);
        if (recycle_state && !init_changed_specs_readonly && (state->TextLen != buf_len || (state->TextA.Data == nullptr || strncmp(state->TextA.Data, buf, buf_len) != 0)))
            recycle_state = false;

        // Start edition
        state->ID = id;
        state->TextLen = buf_len;
        state->EditedBefore = false;
        if (!is_readonly)
        {
            state->TextA.resize(buf_size + 1); // we use +1 to make sure that .Data is always pointing to at least an empty string.
            memcpy(state->TextA.Data, buf, state->TextLen + 1);
        }

        // Find initial scroll position for right alignment
        state->Scroll = VanVec2(0.0f, 0.0f);
        if (flags & VanGuiInputTextFlags_ElideLeft)
            state->Scroll.x += VanMax(0.0f, CalcTextSize(buf).x - frame_size.x + style.FramePadding.x * 2.0f);

        // Recycle existing cursor/selection/undo stack but clamp position
        // Note a single mouse click will override the cursor/position immediately by calling stb_textedit_click handler.
        if (!recycle_state)
            stb_textedit_initialize_state(state->Stb, !is_multiline);

        if (!is_multiline)
        {
            if (flags & VanGuiInputTextFlags_AutoSelectAll)
                select_all = true;
            if (input_requested_by_nav && (!recycle_state || !(g.NavActivateFlags & VanGuiActivateFlags_TryToPreserveState)))
                select_all = true;
            if (user_clicked && io.KeyCtrl)
                select_all = true;
        }

        if (flags & VanGuiInputTextFlags_AlwaysOverwrite)
            state->Stb->insert_mode = 1; // stb field name is indeed incorrect (see #2863)
    }

    const bool is_osx = io.ConfigMacOSXBehaviors;
    if (init_make_active && g.ActiveId != id)
    {
        VAN_ASSERT(state && state->ID == id);
        SetActiveID(id, window);
        SetFocusID(id, window);
        FocusWindow(window);
        if (input_requested_by_nav)
            SetNavCursorVisibleAfterMove();
    }
    if (g.ActiveId == id)
    {
        // Declare some inputs, the other are registered and polled via Shortcut() routing system.
        // FIXME: The reason we don't use Shortcut() is we would need a routing flag to specify multiple mods, or to all mods combination into individual shortcuts.
        const VanGuiKey always_owned_keys[] = { VanGuiKey_LeftArrow, VanGuiKey_RightArrow, VanGuiKey_Delete, VanGuiKey_Backspace, VanGuiKey_Home, VanGuiKey_End };
        for (VanGuiKey key : always_owned_keys)
            SetKeyOwner(key, id);
        if (user_clicked)
            SetKeyOwner(VanGuiKey_MouseLeft, id);
        g.ActiveIdUsingNavDirMask |= (1 << VanGuiDir_Left) | (1 << VanGuiDir_Right);
        if (is_multiline || (flags & VanGuiInputTextFlags_CallbackHistory))
        {
            g.ActiveIdUsingNavDirMask |= (1 << VanGuiDir_Up) | (1 << VanGuiDir_Down);
            SetKeyOwner(VanGuiKey_UpArrow, id);
            SetKeyOwner(VanGuiKey_DownArrow, id);
        }
        if (is_multiline)
        {
            SetKeyOwner(VanGuiKey_PageUp, id);
            SetKeyOwner(VanGuiKey_PageDown, id);
        }
        // FIXME: May be a problem to always steal Alt on OSX, would ideally still allow an uninterrupted Alt down-up to toggle menu
        if (is_osx)
            SetKeyOwner(VanGuiMod_Alt, id);

        // Expose scroll in a manner that is agnostic to us using a child window
        if (is_multiline && state != nullptr)
            state->Scroll.y = draw_window->Scroll.y;

        // Read-only mode always ever read from source buffer. Refresh TextLen when active.
        if (is_readonly && state != nullptr)
            state->TextLen = static_cast<int>(VanStrlen(buf));
        if (state != nullptr)
            state->CursorClamp();
        //if (is_readonly && state != nullptr)
        //    state->TextA.clear(); // Uncomment to facilitate debugging, but we otherwise prefer to keep/amortize th allocation.
    }
    if (state != nullptr)
        state->TextSrc = is_readonly ? buf : state->TextA.Data;

    // We have an edge case if ActiveId was set through another widget (e.g. widget being swapped), clear id immediately (don't wait until the end of the function)
    if (g.ActiveId == id && state == nullptr)
        ClearActiveID();

    // Release focus when we click outside
    if (g.ActiveId == id && io.MouseClicked[0] && !init_make_active) //-V560
        clear_active_id = true;

    // Lock the decision of whether we are going to take the path displaying the cursor or selection
    bool render_cursor = (g.ActiveId == id) || (state && user_scroll_active);
    bool render_selection = state && (state->HasSelection() || select_all) && (RENDER_SELECTION_WHEN_INACTIVE || render_cursor);
    bool value_changed = false;
    bool validated = false;

    // Select the buffer to render.
    const bool buf_display_from_state = (render_cursor || render_selection || g.ActiveId == id) && !is_readonly && state;
    bool is_displaying_hint = (hint != nullptr && (buf_display_from_state ? state->TextA.Data : buf)[0] == 0);

    // Password pushes a temporary font with only a fallback glyph
    if (is_password && !is_displaying_hint)
        PushPasswordFont();

    if (state != nullptr && state->ID == id)
    {
        state->Flags = flags;

        // Word-wrapping: attempt to keep cursor in view while resizing frame/parent (FIXME-WORDWRAP: would be better to preserve same relative offset)
        if (is_wordwrap && state->WrapWidth != wrap_width)
        {
            state->CursorCenterY = true;
            state->WrapWidth = wrap_width;
            render_cursor = true;
        }
    }

    // Process mouse inputs and character inputs
    if (g.ActiveId == id)
    {
        VAN_ASSERT(state != nullptr);
        state->EditedThisFrame = false;
        state->BufCapacity = buf_size;
        state->WrapWidth = wrap_width;

        // Although we are active we don't prevent mouse from hovering other elements unless we are interacting right now with the widget.
        // Down the line we should have a cleaner library-wide concept of Selected vs Active.
        g.ActiveIdAllowOverlap = !io.MouseDown[0];

        // Edit in progress
        const float mouse_x = (io.MousePos.x - frame_bb.Min.x - style.FramePadding.x) + state->Scroll.x;
        const float mouse_y = (is_multiline ? (io.MousePos.y - draw_window->DC.CursorPos.y) : (g.FontSize * 0.5f));

        if (select_all)
        {
            state->SelectAll();
            state->SelectedAllMouseLock = true;
        }
        else if (hovered && io.MouseClickedCount[0] >= 2 && !io.KeyShift)
        {
            stb_textedit_click(state, state->Stb, mouse_x, mouse_y);
            const int multiclick_count = (io.MouseClickedCount[0] - 2);
            if ((multiclick_count % 2) == 0)
            {
                // Double-click: Select word
                // We always use the "Mac" word advance for double-click select vs Ctrl+Right which use the platform dependent variant:
                // FIXME: There are likely many ways to improve this behavior, but there's no "right" behavior (depends on use-case, software, OS)
                const bool is_bol = (state->Stb->cursor == 0) || VanStb::STB_TEXTEDIT_GETCHAR(state, state->Stb->cursor - 1) == '\n';
                if (STB_TEXT_HAS_SELECTION(state->Stb) || !is_bol)
                    state->OnKeyPressed(STB_TEXTEDIT_K_WORDLEFT);
                //state->OnKeyPressed(STB_TEXTEDIT_K_WORDRIGHT | STB_TEXTEDIT_K_SHIFT);
                if (!STB_TEXT_HAS_SELECTION(state->Stb))
                    VanStb::stb_textedit_prep_selection_at_cursor(state->Stb);
                state->Stb->cursor = VanStb::STB_TEXTEDIT_MOVEWORDRIGHT_MAC(state, state->Stb->cursor);
                state->Stb->select_end = state->Stb->cursor;
                VanStb::stb_textedit_clamp(state, state->Stb);
            }
            else
            {
                // Triple-click: Select line
                const bool is_eol = VanStb::STB_TEXTEDIT_GETCHAR(state, state->Stb->cursor) == '\n';
                state->WrapWidth = 0.0f; // Temporarily disable wrapping so we use real line start.
                state->OnKeyPressed(STB_TEXTEDIT_K_LINESTART);
                state->OnKeyPressed(STB_TEXTEDIT_K_LINEEND | STB_TEXTEDIT_K_SHIFT);
                state->OnKeyPressed(STB_TEXTEDIT_K_RIGHT | STB_TEXTEDIT_K_SHIFT);
                state->WrapWidth = wrap_width;
                if (!is_eol && is_multiline)
                {
                    VanSwap(state->Stb->select_start, state->Stb->select_end);
                    state->Stb->cursor = state->Stb->select_end;
                }
                state->CursorFollow = false;
            }
            state->CursorAnimReset();
        }
        else if (io.MouseClicked[0] && !state->SelectedAllMouseLock)
        {
            if (hovered)
            {
                if (io.KeyShift)
                    stb_textedit_drag(state, state->Stb, mouse_x, mouse_y);
                else
                    stb_textedit_click(state, state->Stb, mouse_x, mouse_y);
                state->CursorAnimReset();
            }
        }
        else if (io.MouseDown[0] && !state->SelectedAllMouseLock && (io.MouseDelta.x != 0.0f || io.MouseDelta.y != 0.0f))
        {
            stb_textedit_drag(state, state->Stb, mouse_x, mouse_y);
            state->CursorAnimReset();
            state->CursorFollow = true;
        }
        if (state->SelectedAllMouseLock && !io.MouseDown[0])
            state->SelectedAllMouseLock = false;

        // We expect backends to emit a Tab key but some also emit a Tab character which we ignore (#2467, #1336)
        // (For Tab and Enter: Win32/SFML/Allegro are sending both keys and chars, GLFW and SDL are only sending keys. For Space they all send all threes)
        if ((flags & VanGuiInputTextFlags_AllowTabInput) && !is_readonly)
        {
            if (Shortcut(VanGuiKey_Tab, VanGuiInputFlags_Repeat, id))
            {
                unsigned int c = '\t'; // Insert TAB
                if (InputTextFilterCharacter(&g, state, &c, callback, callback_user_data))
                    state->OnCharPressed(c);
            }
            // FIXME: Implement Shift+Tab
            /*
            if (Shortcut(VanGuiKey_Tab | VanGuiMod_Shift, VanGuiInputFlags_Repeat, id))
            {
            }
            */
        }

        // Process regular text input (before we check for Return because using some IME will effectively send a Return?)
        // We ignore Ctrl inputs, but need to allow Alt+Ctrl as some keyboards (e.g. German) use AltGR (which _is_ Alt+Ctrl) to input certain characters.
        const bool ignore_char_inputs = (io.KeyCtrl && !io.KeyAlt) || (is_osx && io.KeyCtrl);
        if (io.InputQueueCharacters.Size > 0)
        {
            if (!ignore_char_inputs && !is_readonly && !input_requested_by_nav)
                for (int n = 0; n < io.InputQueueCharacters.Size; n++)
                {
                    // Insert character if they pass filtering
                    unsigned int c = static_cast<unsigned int>(io.InputQueueCharacters[n]);
                    if (c == '\t') // Skip Tab, see above.
                        continue;
                    if (InputTextFilterCharacter(&g, state, &c, callback, callback_user_data))
                        state->OnCharPressed(c);
                }

            // Consume characters
            io.InputQueueCharacters.resize(0);
        }
    }

    // Process other shortcuts/key-presses
    bool revert_edit = false;
    if (g.ActiveId == id && !g.ActiveIdIsJustActivated && !clear_active_id)
    {
        VAN_ASSERT(state != nullptr);

        const int row_count_per_page = VanMax(static_cast<int>((inner_size.y - style.FramePadding.y) / g.FontSize), 1);
        state->Stb->row_count_per_page = row_count_per_page;

        const int k_mask = (io.KeyShift ? STB_TEXTEDIT_K_SHIFT : 0);
        const bool is_wordmove_key_down = is_osx ? io.KeyAlt : io.KeyCtrl;                     // OS X style: Text editing cursor movement using Alt instead of Ctrl
        const bool is_startend_key_down = is_osx && io.KeyCtrl && !io.KeySuper && !io.KeyAlt;  // OS X style: Line/Text Start and End using Cmd+Arrows instead of Home/End

        // Using Shortcut() with VanGuiInputFlags_RouteFocused (default policy) to allow routing operations for other code (e.g. calling window trying to use Ctrl+A and Ctrl+B: former would be handled by InputText)
        // Otherwise we could simply assume that we own the keys as we are active.
        const VanGuiInputFlags f_repeat = VanGuiInputFlags_Repeat;
        const bool is_cut   = (Shortcut(VanGuiMod_Ctrl | VanGuiKey_X, f_repeat, id) || Shortcut(VanGuiMod_Shift | VanGuiKey_Delete, f_repeat, id)) && !is_readonly && !is_password && (!is_multiline || state->HasSelection());
        const bool is_copy  = (Shortcut(VanGuiMod_Ctrl | VanGuiKey_C, 0,        id) || Shortcut(VanGuiMod_Ctrl  | VanGuiKey_Insert, 0,        id)) && !is_password && (!is_multiline || state->HasSelection());
        const bool is_paste = (Shortcut(VanGuiMod_Ctrl | VanGuiKey_V, f_repeat, id) || Shortcut(VanGuiMod_Shift | VanGuiKey_Insert, f_repeat, id)) && !is_readonly;
        const bool is_undo  = (Shortcut(VanGuiMod_Ctrl | VanGuiKey_Z, f_repeat, id)) && !is_readonly && is_undoable;
        const bool is_redo =  (Shortcut(VanGuiMod_Ctrl | VanGuiKey_Y, f_repeat, id) || Shortcut(VanGuiMod_Ctrl | VanGuiMod_Shift | VanGuiKey_Z, f_repeat, id)) && !is_readonly && is_undoable;
        const bool is_select_all = Shortcut(VanGuiMod_Ctrl | VanGuiKey_A, 0, id);

        // We allow validate/cancel with Nav source (gamepad) to makes it easier to undo an accidental NavInput press with no keyboard wired, but otherwise it isn't very useful.
        const bool nav_gamepad_active = (io.ConfigFlags & VanGuiConfigFlags_NavEnableGamepad) != 0 && (io.BackendFlags & VanGuiBackendFlags_HasGamepad) != 0;
        const bool is_enter = Shortcut(VanGuiKey_Enter, f_repeat, id) || Shortcut(VanGuiKey_KeypadEnter, f_repeat, id);
        const bool is_ctrl_enter = Shortcut(VanGuiMod_Ctrl | VanGuiKey_Enter, f_repeat, id) || Shortcut(VanGuiMod_Ctrl | VanGuiKey_KeypadEnter, f_repeat, id);
        const bool is_shift_enter = Shortcut(VanGuiMod_Shift | VanGuiKey_Enter, f_repeat, id) || Shortcut(VanGuiMod_Shift | VanGuiKey_KeypadEnter, f_repeat, id);
        const bool is_gamepad_validate = nav_gamepad_active && IsKeyPressed(VanGuiKey_NavGamepadActivate, false);
        const bool is_cancel = Shortcut(VanGuiKey_Escape, f_repeat, id) || (nav_gamepad_active && Shortcut(VanGuiKey_NavGamepadCancel, f_repeat, id));

        // FIXME: Should use more Shortcut() and reduce IsKeyPressed()+SetKeyOwner(), but requires modifiers combination to be taken account of.
        // FIXME-OSX: Missing support for Alt(option)+Right/Left = go to end of line, or next line if already in end of line.
        if (IsKeyPressed(VanGuiKey_LeftArrow))                        { state->OnKeyPressed((is_startend_key_down ? STB_TEXTEDIT_K_LINESTART : is_wordmove_key_down ? STB_TEXTEDIT_K_WORDLEFT : STB_TEXTEDIT_K_LEFT) | k_mask); }
        else if (IsKeyPressed(VanGuiKey_RightArrow))                  { state->OnKeyPressed((is_startend_key_down ? STB_TEXTEDIT_K_LINEEND : is_wordmove_key_down ? STB_TEXTEDIT_K_WORDRIGHT : STB_TEXTEDIT_K_RIGHT) | k_mask); }
        else if (IsKeyPressed(VanGuiKey_UpArrow) && is_multiline)     { if (io.KeyCtrl) SetScrollY(draw_window, VanMax(draw_window->Scroll.y - g.FontSize, 0.0f)); else state->OnKeyPressed((is_startend_key_down ? STB_TEXTEDIT_K_TEXTSTART : STB_TEXTEDIT_K_UP) | k_mask); }
        else if (IsKeyPressed(VanGuiKey_DownArrow) && is_multiline)   { if (io.KeyCtrl) SetScrollY(draw_window, VanMin(draw_window->Scroll.y + g.FontSize, GetScrollMaxY())); else state->OnKeyPressed((is_startend_key_down ? STB_TEXTEDIT_K_TEXTEND : STB_TEXTEDIT_K_DOWN) | k_mask); }
        else if (IsKeyPressed(VanGuiKey_PageUp) && is_multiline)      { state->OnKeyPressed(STB_TEXTEDIT_K_PGUP | k_mask); scroll_y -= row_count_per_page * g.FontSize; }
        else if (IsKeyPressed(VanGuiKey_PageDown) && is_multiline)    { state->OnKeyPressed(STB_TEXTEDIT_K_PGDOWN | k_mask); scroll_y += row_count_per_page * g.FontSize; }
        else if (IsKeyPressed(VanGuiKey_Home))                        { state->OnKeyPressed(io.KeyCtrl ? STB_TEXTEDIT_K_TEXTSTART | k_mask : STB_TEXTEDIT_K_LINESTART | k_mask); }
        else if (IsKeyPressed(VanGuiKey_End))                         { state->OnKeyPressed(io.KeyCtrl ? STB_TEXTEDIT_K_TEXTEND | k_mask : STB_TEXTEDIT_K_LINEEND | k_mask); }
        else if (IsKeyPressed(VanGuiKey_Delete) && !is_readonly && !is_cut)
        {
            if (!state->HasSelection())
            {
                // OSX doesn't seem to have Super+Delete to delete until end-of-line, so we don't emulate that (as opposed to Super+Backspace)
                if (is_wordmove_key_down)
                    state->OnKeyPressed(STB_TEXTEDIT_K_WORDRIGHT | STB_TEXTEDIT_K_SHIFT);
            }
            state->OnKeyPressed(STB_TEXTEDIT_K_DELETE | k_mask);
        }
        else if (IsKeyPressed(VanGuiKey_Backspace) && !is_readonly)
        {
            if (!state->HasSelection())
            {
                if (is_wordmove_key_down)
                    state->OnKeyPressed(STB_TEXTEDIT_K_WORDLEFT | STB_TEXTEDIT_K_SHIFT);
                else if (is_osx && io.KeyCtrl && !io.KeyAlt && !io.KeySuper)
                    state->OnKeyPressed(STB_TEXTEDIT_K_LINESTART | STB_TEXTEDIT_K_SHIFT);
            }
            state->OnKeyPressed(STB_TEXTEDIT_K_BACKSPACE | k_mask);
        }
        else if (is_enter || is_ctrl_enter || is_shift_enter || is_gamepad_validate)
        {
            // Determine if we turn Enter into a \n character
            bool ctrl_enter_for_new_line = (flags & VanGuiInputTextFlags_CtrlEnterForNewLine) != 0;
            bool is_new_line = is_multiline && !is_gamepad_validate && (is_shift_enter || (is_enter && !ctrl_enter_for_new_line) || (is_ctrl_enter && ctrl_enter_for_new_line));
            if (!is_new_line)
            {
                validated = clear_active_id = true;
                if (io.ConfigInputTextEnterKeepActive && !is_multiline)
                {
                    // Queue reactivation, so that e.g. IsItemDeactivatedAfterEdit() will work. (#9001)
                    state->SelectAll(); // No need to scroll
                    g.InputTextReactivateId = id; // Mark for reactivation on next frame
                }
            }
            else if (!is_readonly)
            {
                // Insert new line
                unsigned int c = '\n';
                if (InputTextFilterCharacter(&g, state, &c, callback, callback_user_data))
                    state->OnCharPressed(c);
            }
        }
        else if (is_cancel)
        {
            if (flags & VanGuiInputTextFlags_EscapeClearsAll)
            {
                if (state->TextA.Data[0] != 0)
                {
                    revert_edit = true;
                }
                else
                {
                    render_cursor = render_selection = false;
                    clear_active_id = true;
                }
            }
            else
            {
                clear_active_id = revert_edit = true;
                render_cursor = render_selection = false;
            }
        }
        else if (is_undo || is_redo)
        {
            state->OnKeyPressed(is_undo ? STB_TEXTEDIT_K_UNDO : STB_TEXTEDIT_K_REDO);
            state->ClearSelection();
        }
        else if (is_select_all)
        {
            state->SelectAll();
            state->CursorFollow = true;
        }
        else if (is_cut || is_copy)
        {
            // Cut, Copy
            if (g.PlatformIO.Platform_SetClipboardTextFn != nullptr)
            {
                // SetClipboardText() only takes null terminated strings + state->TextSrc may point to read-only user buffer, so we need to make a copy.
                const int ib = state->HasSelection() ? VanMin(state->Stb->select_start, state->Stb->select_end) : 0;
                const int ie = state->HasSelection() ? VanMax(state->Stb->select_start, state->Stb->select_end) : state->TextLen;
                g.TempBuffer.reserve(ie - ib + 1);
                memcpy(g.TempBuffer.Data, state->TextSrc + ib, ie - ib);
                g.TempBuffer.Data[ie - ib] = 0;
                SetClipboardText(g.TempBuffer.Data);
            }
            if (is_cut)
            {
                if (!state->HasSelection())
                    state->SelectAll();
                state->CursorFollow = true;
                stb_textedit_cut(state, state->Stb);
            }
        }
        else if (is_paste)
        {
            if (const char* clipboard = GetClipboardText())
            {
                // Filter pasted buffer
                const int clipboard_len = static_cast<int>(VanStrlen(clipboard));
                const char* clipboard_end = clipboard + clipboard_len;
                VanVector<char> clipboard_filtered;
                clipboard_filtered.reserve(clipboard_len + 1);
                for (const char* s = clipboard; *s != 0; )
                {
                    unsigned int c;
                    int in_len = VanTextCharFromUtf8(&c, s, clipboard_end);
                    s += in_len;
                    if (!InputTextFilterCharacter(&g, state, &c, callback, callback_user_data, true))
                        continue;
                    char c_utf8[5];
                    VanTextCharToUtf8(c_utf8, c);
                    int out_len = static_cast<int>(VanStrlen(c_utf8));
                    clipboard_filtered.resize(clipboard_filtered.Size + out_len);
                    memcpy(clipboard_filtered.Data + clipboard_filtered.Size - out_len, c_utf8, out_len);
                }
                if (clipboard_filtered.Size > 0) // If everything was filtered, ignore the pasting operation
                {
                    clipboard_filtered.push_back(0);
                    stb_textedit_paste(state, state->Stb, clipboard_filtered.Data, clipboard_filtered.Size - 1);
                    state->CursorFollow = true;
                }
            }
        }

        // Update render selection flag after events have been handled, so selection highlight can be displayed during the same frame.
        render_selection |= state->HasSelection() && (RENDER_SELECTION_WHEN_INACTIVE || render_cursor);
    }

    // Process revert and user callbacks
    const char* apply_new_text = nullptr;
    int apply_new_text_length = 0;
    if (g.ActiveId == id)
    {
        VAN_ASSERT(state != nullptr);
        if (revert_edit && !is_readonly)
        {
            if (flags & VanGuiInputTextFlags_EscapeClearsAll)
            {
                // Clear input
                VAN_ASSERT(state->TextA.Data[0] != 0);
                apply_new_text = "";
                apply_new_text_length = 0;
                value_changed = true;
                char empty_string = 0;
                stb_textedit_replace(state, state->Stb, &empty_string, 0);
            }
            else if (strcmp(state->TextA.Data, state->TextToRevertTo.Data) != 0)
            {
                apply_new_text = state->TextToRevertTo.Data;
                apply_new_text_length = state->TextToRevertTo.Size - 1;

                // Restore initial value. Only return true if restoring to the initial value changes the current buffer contents.
                // Push records into the undo stack so we can Ctrl+Z the revert operation itself
                value_changed = true;
                stb_textedit_replace(state, state->Stb, state->TextToRevertTo.Data, state->TextToRevertTo.Size - 1);
            }
        }

        // User callback
        if ((flags & (VanGuiInputTextFlags_CallbackCompletion | VanGuiInputTextFlags_CallbackHistory | VanGuiInputTextFlags_CallbackEdit | VanGuiInputTextFlags_CallbackAlways)) != 0)
        {
            VAN_ASSERT(callback != nullptr);

            // The reason we specify the usage semantic (Completion/History) is that Completion needs to disable keyboard TABBING at the moment.
            VanGuiInputTextFlags event_flag = 0;
            VanGuiKey event_key = VanGuiKey_None;
            if ((flags & VanGuiInputTextFlags_CallbackCompletion) != 0 && Shortcut(VanGuiKey_Tab, 0, id))
            {
                event_flag = VanGuiInputTextFlags_CallbackCompletion;
                event_key = VanGuiKey_Tab;
            }
            else if ((flags & VanGuiInputTextFlags_CallbackHistory) != 0 && IsKeyPressed(VanGuiKey_UpArrow))
            {
                event_flag = VanGuiInputTextFlags_CallbackHistory;
                event_key = VanGuiKey_UpArrow;
            }
            else if ((flags & VanGuiInputTextFlags_CallbackHistory) != 0 && IsKeyPressed(VanGuiKey_DownArrow))
            {
                event_flag = VanGuiInputTextFlags_CallbackHistory;
                event_key = VanGuiKey_DownArrow;
            }
            else if ((flags & VanGuiInputTextFlags_CallbackEdit) && state->EditedThisFrame)
            {
                event_flag = VanGuiInputTextFlags_CallbackEdit;
            }
            else if (flags & VanGuiInputTextFlags_CallbackAlways)
            {
                event_flag = VanGuiInputTextFlags_CallbackAlways;
            }

            if (event_flag)
            {
                VanGuiInputTextCallbackData callback_data;
                callback_data.Ctx = &g;
                callback_data.ID = id;
                callback_data.Flags = flags;
                callback_data.EventFlag = event_flag;
                callback_data.EventActivated = (g.ActiveId == state->ID && g.ActiveIdIsJustActivated);
                callback_data.UserData = callback_user_data;

                // FIXME-OPT: Undo stack reconcile needs a backup of the data until we rework API, see #7925
                char* callback_buf = is_readonly ? buf : state->TextA.Data;
                VAN_ASSERT(callback_buf == state->TextSrc);
                state->CallbackTextBackup.resize(state->TextLen + 1);
                memcpy(state->CallbackTextBackup.Data, callback_buf, state->TextLen + 1);

                callback_data.EventKey = event_key;
                callback_data.Buf = callback_buf;
                callback_data.BufTextLen = state->TextLen;
                callback_data.BufSize = state->BufCapacity;
                callback_data.BufDirty = false;
                callback_data.CursorPos = state->Stb->cursor;
                callback_data.SelectionStart = state->Stb->select_start;
                callback_data.SelectionEnd = state->Stb->select_end;

                // Call user code
                callback(&callback_data);

                // Read back what user may have modified
                callback_buf = is_readonly ? buf : state->TextA.Data; // Pointer may have been invalidated by a resize callback
                VAN_ASSERT(callback_data.Buf == callback_buf);         // Invalid to modify those fields
                VAN_ASSERT(callback_data.BufSize == state->BufCapacity);
                VAN_ASSERT(callback_data.Flags == flags);
                if (callback_data.BufDirty || callback_data.CursorPos != state->Stb->cursor)
                    state->CursorFollow = true;
                state->Stb->cursor = VanClamp(callback_data.CursorPos, 0, callback_data.BufTextLen);
                state->Stb->select_start = VanClamp(callback_data.SelectionStart, 0, callback_data.BufTextLen);
                state->Stb->select_end = VanClamp(callback_data.SelectionEnd, 0, callback_data.BufTextLen);
                if (callback_data.BufDirty)
                {
                    // Callback may update buffer and thus set buf_dirty even in read-only mode.
                    VAN_ASSERT(callback_data.BufTextLen == static_cast<int>(VanStrlen(callback_data.Buf))); // You need to maintain BufTextLen if you change the text!
                    InputTextReconcileUndoState(state, state->CallbackTextBackup.Data, state->CallbackTextBackup.Size - 1, callback_data.Buf, callback_data.BufTextLen);
                    state->TextLen = callback_data.BufTextLen;  // Assume correct length and valid UTF-8 from user, saves us an extra strlen()
                    state->CursorAnimReset();
                }
            }
        }

        // Will copy result string if modified.
        // FIXME-OPT: Could mark dirty state from the stb_textedit callbacks
        if (!is_readonly && strcmp(state->TextSrc, buf) != 0)
        {
            apply_new_text = state->TextSrc;
            apply_new_text_length = state->TextLen;
            value_changed = true;
        }
    }

    // Handle reapplying final data on deactivation (see InputTextDeactivateHook() for details)
    // This is used when e.g. losing focus or tabbing out into another InputText() which may already be using the temp buffer.
    if (g.InputTextDeactivatedState.ID == id)
    {
        if (g.ActiveId != id && IsItemDeactivatedAfterEdit() && !is_readonly && strcmp(g.InputTextDeactivatedState.TextA.Data, buf) != 0)
        {
            apply_new_text = g.InputTextDeactivatedState.TextA.Data;
            apply_new_text_length = g.InputTextDeactivatedState.TextA.Size - 1;
            value_changed = true;
            //VANGUI_DEBUG_LOG("InputText(): apply Deactivated data for 0x%08X: \"%.*s\".\n", id, apply_new_text_length, apply_new_text);
        }
        g.InputTextDeactivatedState.ID = 0;
    }

    // Write back result to user buffer. This can currently only happen when (g.ActiveId == id) or when just deactivated.
    // - As soon as the InputText() is active, our stored in-widget value gets priority over any underlying modification of the user buffer.
    // - Make sure we always reapply the live buffer back to the input/user buffer before clearing ActiveId, even thought strictly speaking
    //   it was not modified on this frame. This allows the user to use InputText() without maintaining any user-side storage.
    //   (PS: if you use this property together with VanGuiInputTextFlags_CallbackResize, you are at the risk of recreating a temporary
    //    allocated/string object every frame. Which in the grand scheme of scheme is nothing, but isn't dear vangui vibe).
    if (apply_new_text != nullptr)
    {
        VAN_ASSERT(apply_new_text_length >= 0);
        if (is_resizable)
        {
            VanGuiInputTextCallbackData callback_data;
            callback_data.Ctx = &g;
            callback_data.ID = id;
            callback_data.Flags = flags;
            callback_data.EventFlag = VanGuiInputTextFlags_CallbackResize;
            callback_data.EventActivated = (state != nullptr && g.ActiveId == state->ID && g.ActiveIdIsJustActivated);
            callback_data.Buf = buf;
            callback_data.BufTextLen = apply_new_text_length;
            callback_data.BufSize = VanMax(buf_size, apply_new_text_length + 1);
            callback_data.UserData = callback_user_data;
            callback(&callback_data);
            buf = callback_data.Buf;
            buf_size = callback_data.BufSize;
            apply_new_text_length = VanMin(callback_data.BufTextLen, buf_size - 1);
            VAN_ASSERT(apply_new_text_length <= buf_size);
        }
        //VANGUI_DEBUG_PRINT("InputText(\"%s\"): apply_new_text length %d\n", label, apply_new_text_length);

        // If the underlying buffer resize was denied or not carried to the next frame, apply_new_text_length+1 may be >= buf_size.
        VanStrncpy(buf, apply_new_text, VanMin(apply_new_text_length + 1, buf_size));
    }

    // Release active ID at the end of the function (so e.g. pressing Return still does a final application of the value)
    // Otherwise request text input ahead for next frame.
    if (g.ActiveId == id && clear_active_id)
        ClearActiveID();

    // Render frame
    if (!is_multiline)
    {
        RenderNavCursor(frame_bb, id);
        RenderFrame(frame_bb.Min, frame_bb.Max, GetColorU32(VanGuiCol_FrameBg), true, style.FrameRounding);
    }

    VanVec2 draw_pos = is_multiline ? draw_window->DC.CursorPos : frame_bb.Min + style.FramePadding;
    VanRect clip_rect(frame_bb.Min.x, frame_bb.Min.y, frame_bb.Min.x + inner_size.x, frame_bb.Min.y + inner_size.y); // Not using frame_bb.Max because we have adjusted size
    if (is_multiline)
        clip_rect.ClipWith(draw_window->ClipRect);

    // Set upper limit of single-line InputTextEx() at 2 million characters strings. The current pathological worst case is a long line
    // without any carriage return, which would makes VanFont::RenderText() reserve too many vertices and probably crash. Avoid it altogether.
    // Note that we only use this limit on single-line InputText(), so a pathologically large line on a InputTextMultiline() would still crash.
    const int buf_display_max_length = 2 * 1024 * 1024;
    const char* buf_display = buf_display_from_state ? state->TextA.Data : buf; //-V595
    const char* buf_display_end = nullptr; // We have specialized paths below for setting the length

    // Display hint when contents is empty
    // At this point we need to handle the possibility that a callback could have modified the underlying buffer (#8368)
    const bool new_is_displaying_hint = (hint != nullptr && (buf_display_from_state ? state->TextA.Data : buf)[0] == 0);
    if (new_is_displaying_hint != is_displaying_hint)
    {
        if (is_password && !is_displaying_hint)
            PopPasswordFont();
        is_displaying_hint = new_is_displaying_hint;
        if (is_password && !is_displaying_hint)
            PushPasswordFont();
    }
    if (is_displaying_hint)
    {
        buf_display = hint;
        buf_display_end = hint + VanStrlen(hint);
    }
    else
    {
        if (render_cursor || render_selection || g.ActiveId == id)
            buf_display_end = buf_display + state->TextLen; //-V595
        else if (is_multiline && !is_wordwrap)
            buf_display_end = nullptr; // Inactive multi-line: end of buffer will be output by InputTextLineIndexBuild() special strchr() path.
        else
            buf_display_end = buf_display + VanStrlen(buf_display);
    }

    // Calculate visibility
    int line_visible_n0 = 0, line_visible_n1 = 1;
    if (is_multiline)
        CalcClipRectVisibleItemsY(clip_rect, draw_pos, g.FontSize, &line_visible_n0, &line_visible_n1);

    // Build line index for easy data access (makes code below simpler and faster)
    VanGuiTextIndex* line_index = &g.InputTextLineIndex;
    line_index->Offsets.resize(0);
    int line_count = 1;
    if (is_multiline)
    {
        // If scrolling is expected to change build full index.
        // FIXME-OPT: Could append to index when new value of line_visible_n1 becomes bigger, see second call to CalcClipRectVisibleItemsY() below.
        bool will_scroll_y = state && ((state->CursorFollow && render_cursor) || (state->CursorCenterY && (render_cursor || render_selection)));
        line_count = InputTextLineIndexBuild(flags, line_index, buf_display, buf_display_end, wrap_width, will_scroll_y ? INT_MAX : line_visible_n1 + 1, buf_display_end ? nullptr : &buf_display_end);
    }
    line_index->EndOffset = static_cast<int>(buf_display_end - buf_display);
    line_visible_n1 = VanMin(line_visible_n1, line_count);

    // Store text height (we don't need width)
    float text_size_y = line_count * g.FontSize;
    //GetForegroundDrawList()->AddRect(draw_pos + VanVec2(0, line_visible_n0 * g.FontSize), draw_pos + VanVec2(frame_size.x, line_visible_n1 * g.FontSize), VAN_COL32(255, 0, 0, 255));

    // Calculate blinking cursor position
    const VanVec2 cursor_offset = render_cursor && state ? InputTextLineIndexGetPosOffset(g, state, line_index, buf_display, buf_display_end, state->Stb->cursor) : VanVec2(0.0f, 0.0f);
    VanVec2 draw_scroll;

    // Render text. We currently only render selection when the widget is active or while scrolling.
    const VanU32 text_col = GetColorU32(is_displaying_hint ? VanGuiCol_TextDisabled : VanGuiCol_Text);
    if (render_cursor || render_selection)
    {
        // Render text (with cursor and selection)
        // This is going to be messy. We need to:
        // - Display the text (this alone can be more easily clipped)
        // - Handle scrolling, highlight selection, display cursor (those all requires some form of 1d->2d cursor position calculation)
        // - Measure text height (for scrollbar)
        // We are attempting to do most of that in **one main pass** to minimize the computation cost (non-negligible for large amount of text) + 2nd pass for selection rendering (we could merge them by an extra refactoring effort)
        // FIXME: This should occur on buf_display but we'd need to maintain cursor/select_start/select_end for UTF-8.
        VAN_ASSERT(state != nullptr);
        state->LineCount = line_count;

        // Scroll
        float new_scroll_y = scroll_y;
        if (render_cursor && state->CursorFollow)
        {
            // Horizontal scroll in chunks of quarter width
            if (!(flags & VanGuiInputTextFlags_NoHorizontalScroll))
            {
                const float scroll_increment_x = inner_size.x * 0.25f;
                const float visible_width = inner_size.x - style.FramePadding.x;
                if (cursor_offset.x < state->Scroll.x)
                    state->Scroll.x = VAN_TRUNC(VanMax(0.0f, cursor_offset.x - scroll_increment_x));
                else if (cursor_offset.x - visible_width >= state->Scroll.x)
                    state->Scroll.x = VAN_TRUNC(cursor_offset.x - visible_width + scroll_increment_x);
            }
            else
            {
                state->Scroll.x = 0.0f;
            }

            // Vertical scroll
            if (is_multiline)
            {
                // Test if cursor is vertically visible
                if (cursor_offset.y - g.FontSize < scroll_y)
                    new_scroll_y = VanMax(0.0f, cursor_offset.y - g.FontSize);
                else if (cursor_offset.y - (inner_size.y - style.FramePadding.y * 2.0f) >= scroll_y)
                    new_scroll_y = cursor_offset.y - inner_size.y + style.FramePadding.y * 2.0f;
            }
            state->CursorFollow = false;
        }
        if (state->CursorCenterY)
        {
            if (is_multiline)
                new_scroll_y = cursor_offset.y - g.FontSize - (inner_size.y * 0.5f - style.FramePadding.y);
            state->CursorCenterY = false;
            render_cursor = false;
        }
        if (new_scroll_y != scroll_y)
        {
            const float scroll_max_y = VanMax((text_size_y + style.FramePadding.y * 2.0f) - inner_size.y, 0.0f);
            scroll_y = VanClamp(new_scroll_y, 0.0f, scroll_max_y);
            draw_pos.y += (draw_window->Scroll.y - scroll_y);   // Manipulate cursor pos immediately avoid a frame of lag
            draw_window->Scroll.y = scroll_y;
            CalcClipRectVisibleItemsY(clip_rect, draw_pos, g.FontSize, &line_visible_n0, &line_visible_n1);
            line_visible_n1 = VanMin(line_visible_n1, line_count);
        }

        // Draw selection
        draw_scroll.x = state->Scroll.x;
        if (render_selection)
        {
            const VanU32 bg_color = GetColorU32(VanGuiCol_TextSelectedBg, render_cursor ? 1.0f : 0.6f); // FIXME: current code flow mandate that render_cursor is always true here, we are leaving the transparent one for tests.
            const float bg_offy_up = is_multiline ? 0.0f : -1.0f; // FIXME-DPI: those offsets should be part of the style? they don't play so well with multi-line selection.
            const float bg_offy_dn = is_multiline ? 0.0f : 2.0f;
            const float bg_eol_width = VAN_TRUNC(g.FontBaked->GetCharAdvance(static_cast<VanWchar>(' ')) * 0.50f); // So we can see selected empty lines

            const char* text_selected_begin = buf_display + VanMin(state->Stb->select_start, state->Stb->select_end);
            const char* text_selected_end = buf_display + VanMax(state->Stb->select_start, state->Stb->select_end);
            for (int line_n = line_visible_n0; line_n < line_visible_n1; line_n++)
            {
                const char* p = line_index->get_line_begin(buf_display, line_n);
                const char* p_eol = line_index->get_line_end(buf_display, line_n);
                const bool p_eol_is_wrap = (p_eol < buf_display_end && *p_eol != '\n');
                if (p_eol_is_wrap)
                    p_eol++;
                const char* line_selected_begin = (text_selected_begin > p) ? text_selected_begin : p;
                const char* line_selected_end = (text_selected_end < p_eol) ? text_selected_end : p_eol;

                float rect_width = 0.0f;
                if (line_selected_begin < line_selected_end)
                    rect_width += CalcTextSize(line_selected_begin, line_selected_end).x;
                if (text_selected_begin <= p_eol && text_selected_end > p_eol && !p_eol_is_wrap)
                    rect_width += bg_eol_width; // So we can see selected empty lines
                if (rect_width == 0.0f)
                    continue;

                VanRect rect;
                rect.Min.x = draw_pos.x - draw_scroll.x + CalcTextSize(p, line_selected_begin).x;
                rect.Min.y = draw_pos.y - draw_scroll.y + line_n * g.FontSize;
                rect.Max.x = rect.Min.x + rect_width;
                rect.Max.y = rect.Min.y + bg_offy_dn + g.FontSize;
                rect.Min.y += bg_offy_up;
                rect.ClipWith(clip_rect);
                draw_window->DrawList->AddRectFilled(rect.Min, rect.Max, bg_color);
            }
        }
    }

    // Find render position for right alignment (single-line only)
    if (g.ActiveId != id && (flags & VanGuiInputTextFlags_ElideLeft) && !render_cursor && !render_selection)
        draw_pos.x = VanMin(draw_pos.x, frame_bb.Max.x - CalcTextSize(buf_display, nullptr).x - style.FramePadding.x);
    //draw_scroll.x = state->Scroll.x; // Preserve scroll when inactive?

    // Render text
    if ((is_multiline || (buf_display_end - buf_display) < buf_display_max_length) && (text_col & VAN_COL32_A_MASK) && (line_visible_n0 < line_visible_n1))
        g.Font->RenderText(draw_window->DrawList, g.FontSize,
            draw_pos - draw_scroll + VanVec2(0.0f, line_visible_n0 * g.FontSize),
            text_col, clip_rect.AsVec4(),
            line_index->get_line_begin(buf_display, line_visible_n0),
            line_index->get_line_end(buf_display, line_visible_n1 - 1),
            wrap_width, VanDrawTextFlags_WrapKeepBlanks | VanDrawTextFlags_CpuFineClip);

    // Render blinking cursor
    if (render_cursor)
    {
        state->CursorAnim += io.DeltaTime;
        bool cursor_is_visible = (!g.IO.ConfigInputTextCursorBlink) || (state->CursorAnim <= 0.0f) || VanFmod(state->CursorAnim, 1.20f) <= 0.80f;
        VanVec2 cursor_screen_pos = VanTrunc(draw_pos + cursor_offset - draw_scroll);
        VanRect cursor_screen_rect(cursor_screen_pos.x, cursor_screen_pos.y - g.FontSize + 0.5f, cursor_screen_pos.x + 1.0f, cursor_screen_pos.y - 1.5f);
        if (cursor_is_visible && cursor_screen_rect.Overlaps(clip_rect))
            draw_window->DrawList->AddLineV(cursor_screen_rect.Min.x, cursor_screen_rect.Min.y, cursor_screen_rect.Max.y, GetColorU32(VanGuiCol_InputTextCursor), style.InputTextCursorSize);

        // Notify OS of text input position for advanced IME (-1 x offset so that Windows IME can cover our cursor. Bit of an extra nicety.)
        // This is required for some backends (SDL3) to start emitting character/text inputs.
        // As per #6341, make sure we don't set that on the deactivating frame.
        if (!is_readonly && g.ActiveId == id)
        {
            VanGuiPlatformImeData* ime_data = &g.PlatformImeData; // (this is a public struct, passed to io.Platform_SetImeDataFn() handler)
            ime_data->WantVisible = true;
            ime_data->WantTextInput = true;
            ime_data->InputPos = VanVec2(cursor_screen_pos.x - 1.0f, cursor_screen_pos.y - g.FontSize);
            ime_data->InputLineHeight = g.FontSize;
            ime_data->ViewportId = window->Viewport->ID;
        }
    }

    if (is_password && !is_displaying_hint)
        PopPasswordFont();

    if (is_multiline)
    {
        // For focus requests to work on our multiline we need to ensure our child ItemAdd() call specifies the VanGuiItemFlags_Inputable (see #4761, #7870)...
        Dummy(VanVec2(0.0f, text_size_y + style.FramePadding.y));
        g.NextItemData.ItemFlags |= static_cast<VanGuiItemFlags>(VanGuiItemFlags_Inputable) | VanGuiItemFlags_NoTabStop;
        EndChild();
        item_data_backup.StatusFlags |= (g.LastItemData.StatusFlags & VanGuiItemStatusFlags_HoveredWindow);

        // ...and then we need to undo the group overriding last item data, which gets a bit messy as EndGroup() tries to forward scrollbar being active...
        // FIXME: This quite messy/tricky, should attempt to get rid of the child window.
        EndGroup();
        if (g.LastItemData.ID == 0 || g.LastItemData.ID != GetWindowScrollbarID(draw_window, VanGuiAxis_Y))
        {
            g.LastItemData.ID = id;
            g.LastItemData.ItemFlags = item_data_backup.ItemFlags;
            g.LastItemData.StatusFlags = item_data_backup.StatusFlags;
        }
    }
    if (state && is_readonly)
        state->TextSrc = nullptr;

    // Log as text
    if (g.LogEnabled && (!is_password || is_displaying_hint))
    {
        LogSetNextTextDecoration("{", "}");
        LogRenderedText(&draw_pos, buf_display, buf_display_end);
    }

    if (label_size.x > 0)
        RenderText(VanVec2(frame_bb.Max.x + style.ItemInnerSpacing.x, frame_bb.Min.y + style.FramePadding.y), label, label_end, false);

    if (value_changed)
        MarkItemEdited(id);

    VANGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags | VanGuiItemStatusFlags_Inputable);
    if ((flags & VanGuiInputTextFlags_EnterReturnsTrue) != 0)
        return validated;
    else
        return value_changed;
}

void VanGui::DebugNodeInputTextState(VanGuiInputTextState* state)
{
#ifndef VANGUI_DISABLE_DEBUG_TOOLS
    VanGuiContext& g = *GVanGui;
    VanStb::STB_TexteditState* stb_state = state->Stb;
    VanStb::StbUndoState* undo_state = &stb_state->undostate;
    Text("ID: 0x%08X, ActiveID: 0x%08X", state->ID, g.ActiveId);
    DebugLocateItemOnHover(state->ID);
    Text("TextLen: %d, Cursor: %d%s, Selection: %d..%d", state->TextLen, stb_state->cursor,
        (state->Flags & VanGuiInputTextFlags_WordWrap) ? (state->LastMoveDirectionLR == VanGuiDir_Left ? " (L)" : " (R)") : "",
        stb_state->select_start, stb_state->select_end);
    Text("BufCapacity: %d, LineCount: %d", state->BufCapacity, state->LineCount);
    Text("(Internal Buffer: TextA Size: %d, Capacity: %d)", state->TextA.Size, state->TextA.Capacity);
    Text("has_preferred_x: %d (%.2f)", stb_state->has_preferred_x, stb_state->preferred_x);
    Text("undo_point: %d, redo_point: %d, undo_char_point: %d, redo_char_point: %d", undo_state->undo_point, undo_state->redo_point, undo_state->undo_char_point, undo_state->redo_char_point);
    if (BeginChild("undopoints", VanVec2(0.0f, GetTextLineHeight() * 10), VanGuiChildFlags_Borders | VanGuiChildFlags_ResizeY)) // Visualize undo state
    {
        PushStyleVar(VanGuiStyleVar_ItemSpacing, VanVec2(0, 0));
        for (int n = 0; n < VANSTB_TEXTEDIT_UNDOSTATECOUNT; n++)
        {
            VanStb::StbUndoRecord* undo_rec = &undo_state->undo_rec[n];
            const char undo_rec_type = (n < undo_state->undo_point) ? 'u' : (n >= undo_state->redo_point) ? 'r' : ' ';
            if (undo_rec_type == ' ')
                BeginDisabled();
            const int buf_preview_len = (undo_rec_type != ' ' && undo_rec->char_storage != -1) ? undo_rec->insert_length : 0;
            const char* buf_preview_str = undo_state->undo_char + undo_rec->char_storage;
            Text("%c [%02d] where %03d, insert %03d, delete %03d, char_storage %03d \"%.*s\"",
                undo_rec_type, n, undo_rec->where, undo_rec->insert_length, undo_rec->delete_length, undo_rec->char_storage, buf_preview_len, buf_preview_str);
            if (undo_rec_type == ' ')
                EndDisabled();
        }
        PopStyleVar();
    }
    EndChild();
#else
    VAN_UNUSED(state);
#endif
}

//-------------------------------------------------------------------------
// [SECTION] Widgets: ColorEdit, ColorPicker, ColorButton, etc.
//-------------------------------------------------------------------------
// - ColorEdit3()
// - ColorEdit4()
// - ColorPicker3()
// - RenderColorRectWithAlphaCheckerboard() [Internal]
// - ColorPicker4()
// - ColorButton()
// - SetColorEditOptions()
// - ColorTooltip() [Internal]
// - ColorEditOptionsPopup() [Internal]
// - ColorPickerOptionsPopup() [Internal]
//-------------------------------------------------------------------------

bool VanGui::ColorEdit3(const char* label, float col[3], VanGuiColorEditFlags flags)
{
    return ColorEdit4(label, col, flags | VanGuiColorEditFlags_NoAlpha);
}

static void ColorEditRestoreH(const float* col, float* H)
{
    VanGuiContext& g = *GVanGui;
    VAN_ASSERT(g.ColorEditCurrentID != 0);
    if (g.ColorEditSavedID != g.ColorEditCurrentID || g.ColorEditSavedColor != VanGui::ColorConvertFloat4ToU32(VanVec4(col[0], col[1], col[2], 0)))
        return;
    *H = g.ColorEditSavedHue;
}

// ColorEdit supports RGB and HSV inputs. In case of RGB input resulting color may have undefined hue and/or saturation.
// Since widget displays both RGB and HSV values we must preserve hue and saturation to prevent these values resetting.
static void ColorEditRestoreHS(const float* col, float* H, float* S, float* V)
{
    VanGuiContext& g = *GVanGui;
    VAN_ASSERT(g.ColorEditCurrentID != 0);
    if (g.ColorEditSavedID != g.ColorEditCurrentID || g.ColorEditSavedColor != VanGui::ColorConvertFloat4ToU32(VanVec4(col[0], col[1], col[2], 0)))
        return;

    // When S == 0, H is undefined.
    // When H == 1 it wraps around to 0.
    if (*S == 0.0f || (*H == 0.0f && g.ColorEditSavedHue == 1))
        *H = g.ColorEditSavedHue;

    // When V == 0, S is undefined.
    if (*V == 0.0f)
        *S = g.ColorEditSavedSat;
}

// Edit colors components (each component in 0.0f..1.0f range).
// See enum VanGuiColorEditFlags_ for available options. e.g. Only access 3 floats if VanGuiColorEditFlags_NoAlpha flag is set.
// With typical options: Left-click on color square to open color picker. Right-click to open option menu. Ctrl+Click over input fields to edit them and TAB to go to next item.
bool VanGui::ColorEdit4(const char* label, float col[4], VanGuiColorEditFlags flags)
{
    VanGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    VanGuiContext& g = *GVanGui;
    const VanGuiStyle& style = g.Style;
    const float square_sz = GetFrameHeight();
    const char* label_display_end = FindRenderedTextEnd(label);
    float w_full = CalcItemWidth();
    g.NextItemData.ClearFlags();

    BeginGroup();
    PushID(label);
    const bool set_current_color_edit_id = (g.ColorEditCurrentID == 0);
    if (set_current_color_edit_id)
        g.ColorEditCurrentID = window->IDStack.back();

    // If we're not showing any slider there's no point in doing any HSV conversions
    const VanGuiColorEditFlags flags_untouched = flags;
    if (flags & VanGuiColorEditFlags_NoInputs)
        flags = (flags & (~VanGuiColorEditFlags_DisplayMask_)) | VanGuiColorEditFlags_DisplayRGB | VanGuiColorEditFlags_NoOptions;

    // Context menu: display and modify options (before defaults are applied)
    if (!(flags & VanGuiColorEditFlags_NoOptions))
        ColorEditOptionsPopup(col, flags);

    // Read stored options
    if (!(flags & VanGuiColorEditFlags_DisplayMask_))
        flags |= (g.ColorEditOptions & VanGuiColorEditFlags_DisplayMask_);
    if (!(flags & VanGuiColorEditFlags_DataTypeMask_))
        flags |= (g.ColorEditOptions & VanGuiColorEditFlags_DataTypeMask_);
    if (!(flags & VanGuiColorEditFlags_PickerMask_))
        flags |= (g.ColorEditOptions & VanGuiColorEditFlags_PickerMask_);
    if (!(flags & VanGuiColorEditFlags_InputMask_))
        flags |= (g.ColorEditOptions & VanGuiColorEditFlags_InputMask_);
    flags |= (g.ColorEditOptions & ~(VanGuiColorEditFlags_DisplayMask_ | VanGuiColorEditFlags_DataTypeMask_ | VanGuiColorEditFlags_PickerMask_ | VanGuiColorEditFlags_InputMask_));
    VAN_ASSERT(VanIsPowerOfTwo(flags & VanGuiColorEditFlags_DisplayMask_)); // Check that only 1 is selected
    VAN_ASSERT(VanIsPowerOfTwo(flags & VanGuiColorEditFlags_InputMask_));   // Check that only 1 is selected

    const bool alpha = (flags & VanGuiColorEditFlags_NoAlpha) == 0;
    const bool hdr = (flags & VanGuiColorEditFlags_HDR) != 0;
    const int components = alpha ? 4 : 3;
    const float w_button = (flags & VanGuiColorEditFlags_NoSmallPreview) ? 0.0f : (square_sz + style.ItemInnerSpacing.x);
    const float w_inputs = VanMax(w_full - w_button, 1.0f);
    w_full = w_inputs + w_button;

    // Convert to the formats we need
    float f[4] = { col[0], col[1], col[2], alpha ? col[3] : 1.0f };
    if ((flags & VanGuiColorEditFlags_InputHSV) && (flags & VanGuiColorEditFlags_DisplayRGB))
        ColorConvertHSVtoRGB(f[0], f[1], f[2], f[0], f[1], f[2]);
    else if ((flags & VanGuiColorEditFlags_InputRGB) && (flags & VanGuiColorEditFlags_DisplayHSV))
    {
        // Hue is lost when converting from grayscale rgb (saturation=0). Restore it.
        ColorConvertRGBtoHSV(f[0], f[1], f[2], f[0], f[1], f[2]);
        ColorEditRestoreHS(col, &f[0], &f[1], &f[2]);
    }
    int i[4] = { VAN_F32_TO_INT8_UNBOUND(f[0]), VAN_F32_TO_INT8_UNBOUND(f[1]), VAN_F32_TO_INT8_UNBOUND(f[2]), VAN_F32_TO_INT8_UNBOUND(f[3]) };

    bool value_changed = false;
    bool value_changed_as_float = false;

    const VanVec2 pos = window->DC.CursorPos;
    const float inputs_offset_x = (style.ColorButtonPosition == VanGuiDir_Left) ? w_button : 0.0f;
    window->DC.CursorPos.x = pos.x + inputs_offset_x;

    if ((flags & (VanGuiColorEditFlags_DisplayRGB | VanGuiColorEditFlags_DisplayHSV)) != 0 && (flags & VanGuiColorEditFlags_NoInputs) == 0)
    {
        // RGB/HSV 0..255 Sliders
        const float w_items = w_inputs - style.ItemInnerSpacing.x * (components - 1);
        const float w_per_component = VAN_TRUNC(w_items / components);
        const bool draw_color_marker = (flags & (VanGuiColorEditFlags_DisplayHSV | VanGuiColorEditFlags_NoColorMarkers)) == 0;

        const bool hide_prefix = draw_color_marker || (w_per_component <= CalcTextSize((flags & VanGuiColorEditFlags_Float) ? "M:0.000" : "M:000").x);
        static const char* ids[4] = { "##X", "##Y", "##Z", "##W" };
        static const char* fmt_table_int[3][4] =
        {
            {   "%3d",   "%3d",   "%3d",   "%3d" }, // Short display
            { "R:%3d", "G:%3d", "B:%3d", "A:%3d" }, // Long display for RGBA
            { "H:%3d", "S:%3d", "V:%3d", "A:%3d" }  // Long display for HSVA
        };
        static const char* fmt_table_float[3][4] =
        {
            {   "%0.3f",   "%0.3f",   "%0.3f",   "%0.3f" }, // Short display
            { "R:%0.3f", "G:%0.3f", "B:%0.3f", "A:%0.3f" }, // Long display for RGBA
            { "H:%0.3f", "S:%0.3f", "V:%0.3f", "A:%0.3f" }  // Long display for HSVA
        };
        const int fmt_idx = hide_prefix ? 0 : (flags & VanGuiColorEditFlags_DisplayHSV) ? 2 : 1;
        const VanGuiSliderFlags drag_flags = draw_color_marker ? VanGuiSliderFlags_ColorMarkers : VanGuiSliderFlags_None;

        float prev_split = 0.0f;
        for (int n = 0; n < components; n++)
        {
            if (n > 0)
                SameLine(0, style.ItemInnerSpacing.x);
            float next_split = VAN_TRUNC(w_items * (n + 1) / components);
            SetNextItemWidth(VanMax(next_split - prev_split, 1.0f));
            prev_split = next_split;
            if (draw_color_marker)
                SetNextItemColorMarker(GDefaultRgbaColorMarkers[n]);

            // FIXME: When VanGuiColorEditFlags_HDR flag is passed HS values snap in weird ways when SV values go below 0.
            if (flags & VanGuiColorEditFlags_Float)
            {
                value_changed |= DragFloat(ids[n], &f[n], 1.0f / 255.0f, 0.0f, hdr ? 0.0f : 1.0f, fmt_table_float[fmt_idx][n], drag_flags);
                value_changed_as_float |= value_changed;
            }
            else
            {
                value_changed |= DragInt(ids[n], &i[n], 1.0f, 0, hdr ? 0 : 255, fmt_table_int[fmt_idx][n], drag_flags);
            }
            if (!(flags & VanGuiColorEditFlags_NoOptions))
                OpenPopupOnItemClick("context", VanGuiPopupFlags_MouseButtonRight);
        }
    }
    else if ((flags & VanGuiColorEditFlags_DisplayHex) != 0 && (flags & VanGuiColorEditFlags_NoInputs) == 0)
    {
        // RGB Hexadecimal Input
        char buf[64];
        if (alpha)
            VanFormatString(buf, VAN_COUNTOF(buf), "#%02X%02X%02X%02X", VanClamp(i[0], 0, 255), VanClamp(i[1], 0, 255), VanClamp(i[2], 0, 255), VanClamp(i[3], 0, 255));
        else
            VanFormatString(buf, VAN_COUNTOF(buf), "#%02X%02X%02X", VanClamp(i[0], 0, 255), VanClamp(i[1], 0, 255), VanClamp(i[2], 0, 255));
        SetNextItemWidth(w_inputs);
        if (InputText("##Text", buf, VAN_COUNTOF(buf), VanGuiInputTextFlags_CharsUppercase))
        {
            value_changed = true;
            char* p = buf;
            while (*p == '#' || VanCharIsBlankA(*p))
                p++;
            i[0] = i[1] = i[2] = 0;
            i[3] = 0xFF; // alpha default to 255 is not parsed by scanf (e.g. inputting #FFFFFF omitting alpha)
            int r;
            if (alpha)
                r = sscanf(p, "%02X%02X%02X%02X", (unsigned int*)&i[0], (unsigned int*)&i[1], (unsigned int*)&i[2], (unsigned int*)&i[3]); // Treat at unsigned (%X is unsigned)
            else
                r = sscanf(p, "%02X%02X%02X", (unsigned int*)&i[0], (unsigned int*)&i[1], (unsigned int*)&i[2]);
            VAN_UNUSED(r); // Fixes C6031: Return value ignored: 'sscanf'.
        }
        if (!(flags & VanGuiColorEditFlags_NoOptions))
            OpenPopupOnItemClick("context", VanGuiPopupFlags_MouseButtonRight);
    }

    VanGuiWindow* picker_active_window = nullptr;
    if (!(flags & VanGuiColorEditFlags_NoSmallPreview))
    {
        const float button_offset_x = ((flags & VanGuiColorEditFlags_NoInputs) || (style.ColorButtonPosition == VanGuiDir_Left)) ? 0.0f : w_inputs + style.ItemInnerSpacing.x;
        window->DC.CursorPos = VanVec2(pos.x + button_offset_x, pos.y);

        const VanVec4 col_v4(col[0], col[1], col[2], alpha ? col[3] : 1.0f);
        if (ColorButton("##ColorButton", col_v4, flags))
        {
            if (!(flags & VanGuiColorEditFlags_NoPicker))
            {
                // Store current color and open a picker
                g.ColorPickerRef = col_v4;
                OpenPopup("picker");
                SetNextWindowPos(g.LastItemData.Rect.GetBL() + VanVec2(0.0f, style.ItemSpacing.y));
            }
        }
        if (!(flags & VanGuiColorEditFlags_NoOptions))
            OpenPopupOnItemClick("context", VanGuiPopupFlags_MouseButtonRight);

        if (BeginPopup("picker"))
        {
            if (g.CurrentWindow->BeginCount == 1)
            {
                picker_active_window = g.CurrentWindow;
                if (label != label_display_end)
                {
                    TextEx(label, label_display_end);
                    Spacing();
                }
                VanGuiColorEditFlags picker_flags_to_forward = VanGuiColorEditFlags_DataTypeMask_ | VanGuiColorEditFlags_PickerMask_ | VanGuiColorEditFlags_InputMask_ | VanGuiColorEditFlags_HDR | VanGuiColorEditFlags_NoAlpha | VanGuiColorEditFlags_AlphaBar;
                VanGuiColorEditFlags picker_flags = (flags_untouched & picker_flags_to_forward) | VanGuiColorEditFlags_DisplayMask_ | VanGuiColorEditFlags_NoLabel | VanGuiColorEditFlags_AlphaPreviewHalf;
                SetNextItemWidth(square_sz * 12.0f); // Use 256 + bar sizes?
                value_changed |= ColorPicker4("##picker", col, picker_flags, &g.ColorPickerRef.x);
            }
            EndPopup();
        }
    }

    if (label != label_display_end && !(flags & VanGuiColorEditFlags_NoLabel))
    {
        // Position not necessarily next to last submitted button (e.g. if style.ColorButtonPosition == VanGuiDir_Left),
        // but we need to use SameLine() to setup baseline correctly. Might want to refactor SameLine() to simplify this.
        SameLine(0.0f, style.ItemInnerSpacing.x);
        window->DC.CursorPos.x = pos.x + ((flags & VanGuiColorEditFlags_NoInputs) ? w_button : w_full + style.ItemInnerSpacing.x);
        TextEx(label, label_display_end);
    }

    // Convert back
    if (value_changed && picker_active_window == nullptr)
    {
        if (!value_changed_as_float)
            for (int n = 0; n < 4; n++)
                f[n] = i[n] / 255.0f;
        if ((flags & VanGuiColorEditFlags_DisplayHSV) && (flags & VanGuiColorEditFlags_InputRGB))
        {
            g.ColorEditSavedHue = f[0];
            g.ColorEditSavedSat = f[1];
            ColorConvertHSVtoRGB(f[0], f[1], f[2], f[0], f[1], f[2]);
            g.ColorEditSavedID = g.ColorEditCurrentID;
            g.ColorEditSavedColor = ColorConvertFloat4ToU32(VanVec4(f[0], f[1], f[2], 0));
        }
        if ((flags & VanGuiColorEditFlags_DisplayRGB) && (flags & VanGuiColorEditFlags_InputHSV))
            ColorConvertRGBtoHSV(f[0], f[1], f[2], f[0], f[1], f[2]);

        col[0] = f[0];
        col[1] = f[1];
        col[2] = f[2];
        if (alpha)
            col[3] = f[3];
    }

    if (set_current_color_edit_id)
        g.ColorEditCurrentID = 0;
    PopID();
    EndGroup();

    // Drag and Drop Target
    // NB: The flag test is merely an optional micro-optimization, BeginDragDropTarget() does the same test.
    if ((g.LastItemData.StatusFlags & VanGuiItemStatusFlags_HoveredRect) && !(g.LastItemData.ItemFlags & VanGuiItemFlags_ReadOnly) && !(flags & VanGuiColorEditFlags_NoDragDrop) && BeginDragDropTarget())
    {
        bool accepted_drag_drop = false;
        if (const VanGuiPayload* payload = AcceptDragDropPayload(VANGUI_PAYLOAD_TYPE_COLOR_3F))
        {
            memcpy(reinterpret_cast<float*>(col), payload->Data, sizeof(float) * 3); // Preserve alpha if any //-V512 //-V1086
            value_changed = accepted_drag_drop = true;
        }
        if (const VanGuiPayload* payload = AcceptDragDropPayload(VANGUI_PAYLOAD_TYPE_COLOR_4F))
        {
            memcpy(reinterpret_cast<float*>(col), payload->Data, sizeof(float) * components);
            value_changed = accepted_drag_drop = true;
        }

        // Drag-drop payloads are always RGB
        if (accepted_drag_drop && (flags & VanGuiColorEditFlags_InputHSV))
            ColorConvertRGBtoHSV(col[0], col[1], col[2], col[0], col[1], col[2]);
        EndDragDropTarget();
    }

    // When picker is being actively used, use its active id so IsItemActive() will function on ColorEdit4().
    if (picker_active_window && g.ActiveId != 0 && g.ActiveIdWindow == picker_active_window)
        g.LastItemData.ID = g.ActiveId;

    if (value_changed && g.LastItemData.ID != 0) // In case of ID collision, the second EndGroup() won't catch g.ActiveId
        MarkItemEdited(g.LastItemData.ID);

    return value_changed;
}

bool VanGui::ColorPicker3(const char* label, float col[3], VanGuiColorEditFlags flags)
{
    float col4[4] = { col[0], col[1], col[2], 1.0f };
    if (!ColorPicker4(label, col4, flags | VanGuiColorEditFlags_NoAlpha))
        return false;
    col[0] = col4[0]; col[1] = col4[1]; col[2] = col4[2];
    return true;
}

// Helper for ColorPicker4()
static void RenderArrowsForVerticalBar(VanDrawList* draw_list, VanVec2 pos, VanVec2 half_sz, float bar_w, float alpha)
{
    VanU32 alpha8 = VAN_F32_TO_INT8_SAT(alpha);
    VanGui::RenderArrowPointingAt(draw_list, VanVec2(pos.x + half_sz.x + 1,         pos.y), VanVec2(half_sz.x + 2, half_sz.y + 1), VanGuiDir_Right, VAN_COL32(0,0,0,alpha8));
    VanGui::RenderArrowPointingAt(draw_list, VanVec2(pos.x + half_sz.x,             pos.y), half_sz,                              VanGuiDir_Right, VAN_COL32(255,255,255,alpha8));
    VanGui::RenderArrowPointingAt(draw_list, VanVec2(pos.x + bar_w - half_sz.x - 1, pos.y), VanVec2(half_sz.x + 2, half_sz.y + 1), VanGuiDir_Left,  VAN_COL32(0,0,0,alpha8));
    VanGui::RenderArrowPointingAt(draw_list, VanVec2(pos.x + bar_w - half_sz.x,     pos.y), half_sz,                              VanGuiDir_Left,  VAN_COL32(255,255,255,alpha8));
}

// Note: ColorPicker4() only accesses 3 floats if VanGuiColorEditFlags_NoAlpha flag is set.
// (In C++ the 'float col[4]' notation for a function argument is equivalent to 'float* col', we only specify a size to facilitate understanding of the code.)
// FIXME: we adjust the big color square height based on item width, which may cause a flickering feedback loop (if automatic height makes a vertical scrollbar appears, affecting automatic width..)
// FIXME: this is trying to be aware of style.Alpha but not fully correct. Also, the color wheel will have overlapping glitches with (style.Alpha < 1.0)
bool VanGui::ColorPicker4(const char* label, float col[4], VanGuiColorEditFlags flags, const float* ref_col)
{
    VanGuiContext& g = *GVanGui;
    VanGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    VanDrawList* draw_list = window->DrawList;
    VanGuiStyle& style = g.Style;
    VanGuiIO& io = g.IO;

    const float width = CalcItemWidth();
    const bool is_readonly = ((g.NextItemData.ItemFlags | g.CurrentItemFlags) & VanGuiItemFlags_ReadOnly) != 0;
    g.NextItemData.ClearFlags();

    PushID(label);
    const bool set_current_color_edit_id = (g.ColorEditCurrentID == 0);
    if (set_current_color_edit_id)
        g.ColorEditCurrentID = window->IDStack.back();
    BeginGroup();

    if (!(flags & VanGuiColorEditFlags_NoSidePreview))
        flags |= VanGuiColorEditFlags_NoSmallPreview;

    // Context menu: display and store options.
    if (!(flags & VanGuiColorEditFlags_NoOptions))
        ColorPickerOptionsPopup(col, flags);

    // Read stored options
    if (!(flags & VanGuiColorEditFlags_PickerMask_))
        flags |= ((g.ColorEditOptions & VanGuiColorEditFlags_PickerMask_) ? g.ColorEditOptions : VanGuiColorEditFlags_DefaultOptions_) & VanGuiColorEditFlags_PickerMask_;
    if (!(flags & VanGuiColorEditFlags_InputMask_))
        flags |= ((g.ColorEditOptions & VanGuiColorEditFlags_InputMask_) ? g.ColorEditOptions : VanGuiColorEditFlags_DefaultOptions_) & VanGuiColorEditFlags_InputMask_;
    VAN_ASSERT(VanIsPowerOfTwo(flags & VanGuiColorEditFlags_PickerMask_)); // Check that only 1 is selected
    VAN_ASSERT(VanIsPowerOfTwo(flags & VanGuiColorEditFlags_InputMask_));  // Check that only 1 is selected
    if (!(flags & VanGuiColorEditFlags_NoOptions))
        flags |= (g.ColorEditOptions & VanGuiColorEditFlags_AlphaBar);

    // Setup
    int components = (flags & VanGuiColorEditFlags_NoAlpha) ? 3 : 4;
    bool alpha_bar = (flags & VanGuiColorEditFlags_AlphaBar) && !(flags & VanGuiColorEditFlags_NoAlpha);
    VanVec2 picker_pos = window->DC.CursorPos;
    float square_sz = GetFrameHeight();
    float bars_width = square_sz; // Arbitrary smallish width of Hue/Alpha picking bars
    float sv_picker_size = VanMax(bars_width * 1, width - (alpha_bar ? 2 : 1) * (bars_width + style.ItemInnerSpacing.x)); // Saturation/Value picking box
    float bar0_pos_x = picker_pos.x + sv_picker_size + style.ItemInnerSpacing.x;
    float bar1_pos_x = bar0_pos_x + bars_width + style.ItemInnerSpacing.x;
    float bars_triangles_half_sz = VAN_TRUNC(bars_width * 0.20f);

    float backup_initial_col[4];
    memcpy(backup_initial_col, col, components * sizeof(float));

    float wheel_thickness = sv_picker_size * 0.08f;
    float wheel_r_outer = sv_picker_size * 0.50f;
    float wheel_r_inner = wheel_r_outer - wheel_thickness;
    VanVec2 wheel_center(picker_pos.x + (sv_picker_size + bars_width)*0.5f, picker_pos.y + sv_picker_size * 0.5f);

    // Note: the triangle is displayed rotated with triangle_pa pointing to Hue, but most coordinates stays unrotated for logic.
    float triangle_r = wheel_r_inner - static_cast<int>(sv_picker_size * 0.027f);
    VanVec2 triangle_pa = VanVec2(triangle_r, 0.0f); // Hue point.
    VanVec2 triangle_pb = VanVec2(triangle_r * -0.5f, triangle_r * -0.866025f); // Black point.
    VanVec2 triangle_pc = VanVec2(triangle_r * -0.5f, triangle_r * +0.866025f); // White point.

    float H = col[0], S = col[1], V = col[2];
    float R = col[0], G = col[1], B = col[2];
    if (flags & VanGuiColorEditFlags_InputRGB)
    {
        // Hue is lost when converting from grayscale rgb (saturation=0). Restore it.
        ColorConvertRGBtoHSV(R, G, B, H, S, V);
        ColorEditRestoreHS(col, &H, &S, &V);
    }
    else if (flags & VanGuiColorEditFlags_InputHSV)
    {
        ColorConvertHSVtoRGB(H, S, V, R, G, B);
    }

    bool value_changed = false, value_changed_h = false, value_changed_sv = false;

    PushItemFlag(VanGuiItemFlags_NoNav, true);
    if (flags & VanGuiColorEditFlags_PickerHueWheel)
    {
        // Hue wheel + SV triangle logic
        (void)(InvisibleButton("hsv", VanVec2(sv_picker_size + style.ItemInnerSpacing.x + bars_width, sv_picker_size)));
        if (IsItemActive() && !is_readonly)
        {
            VanVec2 initial_off = g.IO.MouseClickedPos[0] - wheel_center;
            VanVec2 current_off = g.IO.MousePos - wheel_center;
            float initial_dist2 = VanLengthSqr(initial_off);
            if (initial_dist2 >= (wheel_r_inner - 1) * (wheel_r_inner - 1) && initial_dist2 <= (wheel_r_outer + 1) * (wheel_r_outer + 1))
            {
                // Interactive with Hue wheel
                H = VanAtan2(current_off.y, current_off.x) / VAN_PI * 0.5f;
                if (H < 0.0f)
                    H += 1.0f;
                value_changed = value_changed_h = true;
            }
            float cos_hue_angle = VanCos(-H * 2.0f * VAN_PI);
            float sin_hue_angle = VanSin(-H * 2.0f * VAN_PI);
            if (VanTriangleContainsPoint(triangle_pa, triangle_pb, triangle_pc, VanRotate(initial_off, cos_hue_angle, sin_hue_angle)))
            {
                // Interacting with SV triangle
                VanVec2 current_off_unrotated = VanRotate(current_off, cos_hue_angle, sin_hue_angle);
                if (!VanTriangleContainsPoint(triangle_pa, triangle_pb, triangle_pc, current_off_unrotated))
                    current_off_unrotated = VanTriangleClosestPoint(triangle_pa, triangle_pb, triangle_pc, current_off_unrotated);
                float uu, vv, ww;
                VanTriangleBarycentricCoords(triangle_pa, triangle_pb, triangle_pc, current_off_unrotated, uu, vv, ww);
                V = VanClamp(1.0f - vv, 0.0001f, 1.0f);
                S = VanClamp(uu / V, 0.0001f, 1.0f);
                value_changed = value_changed_sv = true;
            }
        }
        if (!(flags & VanGuiColorEditFlags_NoOptions))
            OpenPopupOnItemClick("context", VanGuiPopupFlags_MouseButtonRight);
    }
    else if (flags & VanGuiColorEditFlags_PickerHueBar)
    {
        // SV rectangle logic
        (void)(InvisibleButton("sv", VanVec2(sv_picker_size, sv_picker_size)));
        if (IsItemActive() && !is_readonly)
        {
            S = VanSaturate((io.MousePos.x - picker_pos.x) / (sv_picker_size - 1));
            V = 1.0f - VanSaturate((io.MousePos.y - picker_pos.y) / (sv_picker_size - 1));
            ColorEditRestoreH(col, &H); // Greatly reduces hue jitter and reset to 0 when hue == 255 and color is rapidly modified using SV square.
            value_changed = value_changed_sv = true;
        }
        if (!(flags & VanGuiColorEditFlags_NoOptions))
            OpenPopupOnItemClick("context", VanGuiPopupFlags_MouseButtonRight);

        // Hue bar logic
        SetCursorScreenPos(VanVec2(bar0_pos_x, picker_pos.y));
        (void)(InvisibleButton("hue", VanVec2(bars_width, sv_picker_size)));
        if (IsItemActive() && !is_readonly)
        {
            H = VanSaturate((io.MousePos.y - picker_pos.y) / (sv_picker_size - 1));
            value_changed = value_changed_h = true;
        }
    }

    // Alpha bar logic
    if (alpha_bar)
    {
        SetCursorScreenPos(VanVec2(bar1_pos_x, picker_pos.y));
        (void)(InvisibleButton("alpha", VanVec2(bars_width, sv_picker_size)));
        if (IsItemActive())
        {
            col[3] = 1.0f - VanSaturate((io.MousePos.y - picker_pos.y) / (sv_picker_size - 1));
            value_changed = true;
        }
    }
    PopItemFlag(); // VanGuiItemFlags_NoNav

    if (!(flags & VanGuiColorEditFlags_NoSidePreview))
    {
        SameLine(0, style.ItemInnerSpacing.x);
        BeginGroup();
    }

    if (!(flags & VanGuiColorEditFlags_NoLabel))
    {
        const char* label_display_end = FindRenderedTextEnd(label);
        if (label != label_display_end)
        {
            if ((flags & VanGuiColorEditFlags_NoSidePreview))
                SameLine(0, style.ItemInnerSpacing.x);
            TextEx(label, label_display_end);
        }
    }

    if (!(flags & VanGuiColorEditFlags_NoSidePreview))
    {
        PushItemFlag(VanGuiItemFlags_NoNavDefaultFocus, true);
        VanVec4 col_v4(col[0], col[1], col[2], (flags & VanGuiColorEditFlags_NoAlpha) ? 1.0f : col[3]);
        if ((flags & VanGuiColorEditFlags_NoLabel))
            Text("Current");

        VanGuiColorEditFlags sub_flags_to_forward = VanGuiColorEditFlags_InputMask_ | VanGuiColorEditFlags_HDR | VanGuiColorEditFlags_AlphaMask_ | VanGuiColorEditFlags_NoTooltip;
        (void)(ColorButton("##current", col_v4, (flags & sub_flags_to_forward), VanVec2(square_sz * 3, square_sz * 2)));
        if (ref_col != nullptr)
        {
            Text("Original");
            VanVec4 ref_col_v4(ref_col[0], ref_col[1], ref_col[2], (flags & VanGuiColorEditFlags_NoAlpha) ? 1.0f : ref_col[3]);
            if (ColorButton("##original", ref_col_v4, (flags & sub_flags_to_forward), VanVec2(square_sz * 3, square_sz * 2)))
            {
                memcpy(col, ref_col, components * sizeof(float));
                value_changed = true;
            }
        }
        PopItemFlag();
        EndGroup();
    }

    // Convert back color to RGB
    if (value_changed_h || value_changed_sv)
    {
        if (flags & VanGuiColorEditFlags_InputRGB)
        {
            ColorConvertHSVtoRGB(H, S, V, col[0], col[1], col[2]);
            g.ColorEditSavedHue = H;
            g.ColorEditSavedSat = S;
            g.ColorEditSavedID = g.ColorEditCurrentID;
            g.ColorEditSavedColor = ColorConvertFloat4ToU32(VanVec4(col[0], col[1], col[2], 0));
        }
        else if (flags & VanGuiColorEditFlags_InputHSV)
        {
            col[0] = H;
            col[1] = S;
            col[2] = V;
        }
    }

    // R,G,B and H,S,V slider color editor
    bool value_changed_fix_hue_wrap = false;
    if ((flags & VanGuiColorEditFlags_NoInputs) == 0)
    {
        PushItemWidth((alpha_bar ? bar1_pos_x : bar0_pos_x) + bars_width - picker_pos.x);
        VanGuiColorEditFlags sub_flags_to_forward = VanGuiColorEditFlags_DataTypeMask_ | VanGuiColorEditFlags_InputMask_ | VanGuiColorEditFlags_HDR | VanGuiColorEditFlags_AlphaMask_ | VanGuiColorEditFlags_NoOptions | VanGuiColorEditFlags_NoTooltip | VanGuiColorEditFlags_NoSmallPreview;
        VanGuiColorEditFlags sub_flags = (flags & sub_flags_to_forward) | VanGuiColorEditFlags_NoPicker;
        if (flags & VanGuiColorEditFlags_DisplayRGB || (flags & VanGuiColorEditFlags_DisplayMask_) == 0)
            if (ColorEdit4("##rgb", col, sub_flags | VanGuiColorEditFlags_DisplayRGB))
            {
                // FIXME: Hackily differentiating using the DragInt (ActiveId != 0 && !ActiveIdAllowOverlap) vs. using the InputText or DropTarget.
                // For the later we don't want to run the hue-wrap canceling code. If you are well versed in HSV picker please provide your input! (See #2050)
                value_changed_fix_hue_wrap = (g.ActiveId != 0 && !g.ActiveIdAllowOverlap);
                value_changed = true;
            }
        if (flags & VanGuiColorEditFlags_DisplayHSV || (flags & VanGuiColorEditFlags_DisplayMask_) == 0)
            value_changed |= ColorEdit4("##hsv", col, sub_flags | VanGuiColorEditFlags_DisplayHSV);
        if (flags & VanGuiColorEditFlags_DisplayHex || (flags & VanGuiColorEditFlags_DisplayMask_) == 0)
            value_changed |= ColorEdit4("##hex", col, sub_flags | VanGuiColorEditFlags_DisplayHex);
        PopItemWidth();
    }

    // Try to cancel hue wrap (after ColorEdit4 call), if any
    if (value_changed_fix_hue_wrap && (flags & VanGuiColorEditFlags_InputRGB))
    {
        float new_H, new_S, new_V;
        ColorConvertRGBtoHSV(col[0], col[1], col[2], new_H, new_S, new_V);
        if (new_H <= 0 && H > 0)
        {
            if (new_V <= 0 && V != new_V)
                ColorConvertHSVtoRGB(H, S, new_V <= 0 ? V * 0.5f : new_V, col[0], col[1], col[2]);
            else if (new_S <= 0)
                ColorConvertHSVtoRGB(H, new_S <= 0 ? S * 0.5f : new_S, new_V, col[0], col[1], col[2]);
        }
    }

    if (value_changed)
    {
        if (flags & VanGuiColorEditFlags_InputRGB)
        {
            R = col[0];
            G = col[1];
            B = col[2];
            ColorConvertRGBtoHSV(R, G, B, H, S, V);
            ColorEditRestoreHS(col, &H, &S, &V);   // Fix local Hue as display below will use it immediately.
        }
        else if (flags & VanGuiColorEditFlags_InputHSV)
        {
            H = col[0];
            S = col[1];
            V = col[2];
            ColorConvertHSVtoRGB(H, S, V, R, G, B);
        }
    }

    const int style_alpha8 = VAN_F32_TO_INT8_SAT(style.Alpha);
    const VanU32 col_black = VAN_COL32(0,0,0,style_alpha8);
    const VanU32 col_white = VAN_COL32(255,255,255,style_alpha8);
    const VanU32 col_midgrey = VAN_COL32(128,128,128,style_alpha8);
    const VanU32 col_hues[6 + 1] = { VAN_COL32(255,0,0,style_alpha8), VAN_COL32(255,255,0,style_alpha8), VAN_COL32(0,255,0,style_alpha8), VAN_COL32(0,255,255,style_alpha8), VAN_COL32(0,0,255,style_alpha8), VAN_COL32(255,0,255,style_alpha8), VAN_COL32(255,0,0,style_alpha8) };

    VanVec4 hue_color_f(1, 1, 1, style.Alpha); ColorConvertHSVtoRGB(H, 1, 1, hue_color_f.x, hue_color_f.y, hue_color_f.z);
    VanU32 hue_color32 = ColorConvertFloat4ToU32(hue_color_f);
    VanU32 user_col32_striped_of_alpha = ColorConvertFloat4ToU32(VanVec4(R, G, B, style.Alpha)); // Important: this is still including the main rendering/style alpha!!

    VanVec2 sv_cursor_pos;

    if (flags & VanGuiColorEditFlags_PickerHueWheel)
    {
        // Render Hue Wheel
        const float aeps = 0.5f / wheel_r_outer; // Half a pixel arc length in radians (2pi cancels out).
        const int segment_per_arc = VanMax(4, static_cast<int>(wheel_r_outer) / 12);
        for (int n = 0; n < 6; n++)
        {
            const float a0 = (n)     /6.0f * 2.0f * VAN_PI - aeps;
            const float a1 = (n+1.0f)/6.0f * 2.0f * VAN_PI + aeps;
            const int vert_start_idx = draw_list->VtxBuffer.Size;
            draw_list->PathArcTo(wheel_center, (wheel_r_inner + wheel_r_outer)*0.5f, a0, a1, segment_per_arc);
            draw_list->PathStroke(col_white, wheel_thickness);
            const int vert_end_idx = draw_list->VtxBuffer.Size;

            // Paint colors over existing vertices
            VanVec2 gradient_p0(wheel_center.x + VanCos(a0) * wheel_r_inner, wheel_center.y + VanSin(a0) * wheel_r_inner);
            VanVec2 gradient_p1(wheel_center.x + VanCos(a1) * wheel_r_inner, wheel_center.y + VanSin(a1) * wheel_r_inner);
            ShadeVertsLinearColorGradientKeepAlpha(draw_list, vert_start_idx, vert_end_idx, gradient_p0, gradient_p1, col_hues[n], col_hues[n + 1]);
        }

        // Render Cursor + preview on Hue Wheel
        float cos_hue_angle = VanCos(H * 2.0f * VAN_PI);
        float sin_hue_angle = VanSin(H * 2.0f * VAN_PI);
        VanVec2 hue_cursor_pos(wheel_center.x + cos_hue_angle * (wheel_r_inner + wheel_r_outer) * 0.5f, wheel_center.y + sin_hue_angle * (wheel_r_inner + wheel_r_outer) * 0.5f);
        float hue_cursor_rad = value_changed_h ? wheel_thickness * 0.65f : wheel_thickness * 0.55f;
        int hue_cursor_segments = draw_list->_CalcCircleAutoSegmentCount(hue_cursor_rad); // Lock segment count so the +1 one matches others.
        draw_list->AddCircleFilled(hue_cursor_pos, hue_cursor_rad, hue_color32, hue_cursor_segments);
        draw_list->AddCircle(hue_cursor_pos, hue_cursor_rad + 1, col_midgrey, hue_cursor_segments);
        draw_list->AddCircle(hue_cursor_pos, hue_cursor_rad, col_white, hue_cursor_segments);

        // Render SV triangle (rotated according to hue)
        VanVec2 tra = wheel_center + VanRotate(triangle_pa, cos_hue_angle, sin_hue_angle);
        VanVec2 trb = wheel_center + VanRotate(triangle_pb, cos_hue_angle, sin_hue_angle);
        VanVec2 trc = wheel_center + VanRotate(triangle_pc, cos_hue_angle, sin_hue_angle);
        VanVec2 uv_white = GetFontTexUvWhitePixel();
        draw_list->PrimReserve(3, 3);
        draw_list->PrimVtx(tra, uv_white, hue_color32);
        draw_list->PrimVtx(trb, uv_white, col_black);
        draw_list->PrimVtx(trc, uv_white, col_white);
        draw_list->AddTriangle(tra, trb, trc, col_midgrey, 1.5f);
        sv_cursor_pos = VanLerp(VanLerp(trc, tra, VanSaturate(S)), trb, VanSaturate(1 - V));
    }
    else if (flags & VanGuiColorEditFlags_PickerHueBar)
    {
        // Render SV Square
        draw_list->AddRectFilledMultiColor(picker_pos, picker_pos + VanVec2(sv_picker_size, sv_picker_size), col_white, hue_color32, hue_color32, col_white);
        draw_list->AddRectFilledMultiColor(picker_pos, picker_pos + VanVec2(sv_picker_size, sv_picker_size), 0, 0, col_black, col_black);
        RenderFrameBorder(picker_pos, picker_pos + VanVec2(sv_picker_size, sv_picker_size), 0.0f);
        sv_cursor_pos.x = VanClamp(VAN_ROUND(picker_pos.x + VanSaturate(S)     * sv_picker_size), picker_pos.x + 2, picker_pos.x + sv_picker_size - 2); // Sneakily prevent the circle to stick out too much
        sv_cursor_pos.y = VanClamp(VAN_ROUND(picker_pos.y + VanSaturate(1 - V) * sv_picker_size), picker_pos.y + 2, picker_pos.y + sv_picker_size - 2);

        // Render Hue Bar
        for (int i = 0; i < 6; ++i)
            draw_list->AddRectFilledMultiColor(VanVec2(bar0_pos_x, picker_pos.y + i * (sv_picker_size / 6)), VanVec2(bar0_pos_x + bars_width, picker_pos.y + (i + 1) * (sv_picker_size / 6)), col_hues[i], col_hues[i], col_hues[i + 1], col_hues[i + 1]);
        float bar0_line_y = VAN_ROUND(picker_pos.y + H * sv_picker_size);
        RenderFrameBorder(VanVec2(bar0_pos_x, picker_pos.y), VanVec2(bar0_pos_x + bars_width, picker_pos.y + sv_picker_size), 0.0f);
        RenderArrowsForVerticalBar(draw_list, VanVec2(bar0_pos_x - 1, bar0_line_y), VanVec2(bars_triangles_half_sz + 1, bars_triangles_half_sz), bars_width + 2.0f, style.Alpha);
    }

    // Render cursor/preview circle (clamp S/V within 0..1 range because floating points colors may lead HSV values to be out of range)
    float sv_cursor_rad = value_changed_sv ? wheel_thickness * 0.55f : wheel_thickness * 0.40f;
    int sv_cursor_segments = draw_list->_CalcCircleAutoSegmentCount(sv_cursor_rad); // Lock segment count so the +1 one matches others.
    draw_list->AddCircleFilled(sv_cursor_pos, sv_cursor_rad, user_col32_striped_of_alpha, sv_cursor_segments);
    draw_list->AddCircle(sv_cursor_pos, sv_cursor_rad + 1, col_midgrey, sv_cursor_segments);
    draw_list->AddCircle(sv_cursor_pos, sv_cursor_rad, col_white, sv_cursor_segments);

    // Render alpha bar
    if (alpha_bar)
    {
        float alpha = VanSaturate(col[3]);
        VanRect bar1_bb(bar1_pos_x, picker_pos.y, bar1_pos_x + bars_width, picker_pos.y + sv_picker_size);
        RenderColorRectWithAlphaCheckerboard(draw_list, bar1_bb.Min, bar1_bb.Max, 0, bar1_bb.GetWidth() / 2.0f, VanVec2(0.0f, 0.0f));
        draw_list->AddRectFilledMultiColor(bar1_bb.Min, bar1_bb.Max, user_col32_striped_of_alpha, user_col32_striped_of_alpha, user_col32_striped_of_alpha & ~VAN_COL32_A_MASK, user_col32_striped_of_alpha & ~VAN_COL32_A_MASK);
        float bar1_line_y = VAN_ROUND(picker_pos.y + (1.0f - alpha) * sv_picker_size);
        RenderFrameBorder(bar1_bb.Min, bar1_bb.Max, 0.0f);
        RenderArrowsForVerticalBar(draw_list, VanVec2(bar1_pos_x - 1, bar1_line_y), VanVec2(bars_triangles_half_sz + 1, bars_triangles_half_sz), bars_width + 2.0f, style.Alpha);
    }

    EndGroup();

    if (value_changed && memcmp(backup_initial_col, col, components * sizeof(float)) == 0)
        value_changed = false;
    if (value_changed && g.LastItemData.ID != 0) // In case of ID collision, the second EndGroup() won't catch g.ActiveId
        MarkItemEdited(g.LastItemData.ID);

    if (set_current_color_edit_id)
        g.ColorEditCurrentID = 0;
    PopID();

    return value_changed;
}

// A little color square. Return true when clicked.
// FIXME: May want to display/ignore the alpha component in the color display? Yet show it in the tooltip.
// 'desc_id' is not called 'label' because we don't display it next to the button, but only in the tooltip.
// Note that 'col' may be encoded in HSV if VanGuiColorEditFlags_InputHSV is set.
bool VanGui::ColorButton(const char* desc_id, const VanVec4& col, VanGuiColorEditFlags flags, const VanVec2& size_arg)
{
    VanGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    VanGuiContext& g = *GVanGui;
    const VanGuiID id = window->GetID(desc_id);
    const float default_size = GetFrameHeight();
    const VanVec2 size(size_arg.x == 0.0f ? default_size : size_arg.x, size_arg.y == 0.0f ? default_size : size_arg.y);
    const VanRect bb(window->DC.CursorPos, window->DC.CursorPos + size);
    ItemSize(bb, (size.y >= default_size) ? g.Style.FramePadding.y : 0.0f);
    if (!ItemAdd(bb, id))
        return false;

    bool hovered, held;
    bool pressed = ButtonBehavior(bb, id, &hovered, &held);

    if (flags & (VanGuiColorEditFlags_NoAlpha | VanGuiColorEditFlags_AlphaOpaque))
        flags &= ~(VanGuiColorEditFlags_AlphaNoBg | VanGuiColorEditFlags_AlphaPreviewHalf);

    VanVec4 col_rgb = col;
    if (flags & VanGuiColorEditFlags_InputHSV)
        ColorConvertHSVtoRGB(col_rgb.x, col_rgb.y, col_rgb.z, col_rgb.x, col_rgb.y, col_rgb.z);

    VanVec4 col_rgb_without_alpha(col_rgb.x, col_rgb.y, col_rgb.z, 1.0f);
    float grid_step = VanMin(size.x, size.y) / 2.99f;
    float rounding = VanMin(g.Style.FrameRounding, grid_step * 0.5f);
    VanRect bb_inner = bb;
    float off = 0.0f;
    if ((flags & VanGuiColorEditFlags_NoBorder) == 0)
    {
        off = -0.75f; // The border (using Col_FrameBg) tends to look off when color is near-opaque and rounding is enabled. This offset seemed like a good middle ground to reduce those artifacts.
        bb_inner.Expand(off);
    }
    if ((flags & VanGuiColorEditFlags_AlphaPreviewHalf) && col_rgb.w < 1.0f)
    {
        float mid_x = VAN_ROUND((bb_inner.Min.x + bb_inner.Max.x) * 0.5f);
        if ((flags & VanGuiColorEditFlags_AlphaNoBg) == 0)
            RenderColorRectWithAlphaCheckerboard(window->DrawList, VanVec2(bb_inner.Min.x + grid_step, bb_inner.Min.y), bb_inner.Max, GetColorU32(col_rgb), grid_step, VanVec2(-grid_step + off, off), rounding, VanDrawFlags_RoundCornersRight);
        else
            window->DrawList->AddRectFilled(VanVec2(bb_inner.Min.x + grid_step, bb_inner.Min.y), bb_inner.Max, GetColorU32(col_rgb), rounding, VanDrawFlags_RoundCornersRight);
        window->DrawList->AddRectFilled(bb_inner.Min, VanVec2(mid_x, bb_inner.Max.y), GetColorU32(col_rgb_without_alpha), rounding, VanDrawFlags_RoundCornersLeft);
    }
    else
    {
        // Because GetColorU32() multiplies by the global style Alpha and we don't want to display a checkerboard if the source code had no alpha
        VanVec4 col_source = (flags & VanGuiColorEditFlags_AlphaOpaque) ? col_rgb_without_alpha : col_rgb;
        if (col_source.w < 1.0f && (flags & VanGuiColorEditFlags_AlphaNoBg) == 0)
            RenderColorRectWithAlphaCheckerboard(window->DrawList, bb_inner.Min, bb_inner.Max, GetColorU32(col_source), grid_step, VanVec2(off, off), rounding);
        else
            window->DrawList->AddRectFilled(bb_inner.Min, bb_inner.Max, GetColorU32(col_source), rounding);
    }
    RenderNavCursor(bb, id);
    if ((flags & VanGuiColorEditFlags_NoBorder) == 0)
    {
        if (g.Style.FrameBorderSize > 0.0f)
            RenderFrameBorder(bb.Min, bb.Max, rounding);
        else
            window->DrawList->AddRect(bb.Min, bb.Max, GetColorU32(VanGuiCol_FrameBg), rounding, 1.0f * static_cast<float>(static_cast<int>(g.Style._MainScale))); // Color buttons are often in need of some sort of border // FIXME-DPI
    }

    // Drag and Drop Source
    // NB: The ActiveId test is merely an optional micro-optimization, BeginDragDropSource() does the same test.
    if (g.ActiveId == id && !(flags & VanGuiColorEditFlags_NoDragDrop) && BeginDragDropSource())
    {
        if (flags & VanGuiColorEditFlags_NoAlpha)
            (void)(SetDragDropPayload(VANGUI_PAYLOAD_TYPE_COLOR_3F, &col_rgb, sizeof(float) * 3, VanGuiCond_Once));
        else
            (void)(SetDragDropPayload(VANGUI_PAYLOAD_TYPE_COLOR_4F, &col_rgb, sizeof(float) * 4, VanGuiCond_Once));
        (void)(ColorButton(desc_id, col, flags));
        SameLine();
        TextEx("Color");
        EndDragDropSource();
    }

    // Tooltip
    if (!(flags & VanGuiColorEditFlags_NoTooltip) && hovered && IsItemHovered(VanGuiHoveredFlags_ForTooltip))
        ColorTooltip(desc_id, &col.x, flags & (VanGuiColorEditFlags_InputMask_ | VanGuiColorEditFlags_AlphaMask_));

    return pressed;
}

// Initialize/override default color options
// FIXME: Could be moved to a simple IO field.
void VanGui::SetColorEditOptions(VanGuiColorEditFlags flags)
{
    VanGuiContext& g = *GVanGui;
    if ((flags & VanGuiColorEditFlags_DisplayMask_) == 0)
        flags |= VanGuiColorEditFlags_DefaultOptions_ & VanGuiColorEditFlags_DisplayMask_;
    if ((flags & VanGuiColorEditFlags_DataTypeMask_) == 0)
        flags |= VanGuiColorEditFlags_DefaultOptions_ & VanGuiColorEditFlags_DataTypeMask_;
    if ((flags & VanGuiColorEditFlags_PickerMask_) == 0)
        flags |= VanGuiColorEditFlags_DefaultOptions_ & VanGuiColorEditFlags_PickerMask_;
    if ((flags & VanGuiColorEditFlags_InputMask_) == 0)
        flags |= VanGuiColorEditFlags_DefaultOptions_ & VanGuiColorEditFlags_InputMask_;
    VAN_ASSERT(VanIsPowerOfTwo(flags & VanGuiColorEditFlags_DisplayMask_));    // Check only 1 option is selected
    VAN_ASSERT(VanIsPowerOfTwo(flags & VanGuiColorEditFlags_DataTypeMask_));   // Check only 1 option is selected
    VAN_ASSERT(VanIsPowerOfTwo(flags & VanGuiColorEditFlags_PickerMask_));     // Check only 1 option is selected
    VAN_ASSERT(VanIsPowerOfTwo(flags & VanGuiColorEditFlags_InputMask_));      // Check only 1 option is selected
    g.ColorEditOptions = flags;
}

// Note: only access 3 floats if VanGuiColorEditFlags_NoAlpha flag is set.
void VanGui::ColorTooltip(const char* text, const float* col, VanGuiColorEditFlags flags)
{
    VanGuiContext& g = *GVanGui;

    if (!BeginTooltipEx(VanGuiTooltipFlags_OverridePrevious, VanGuiWindowFlags_None))
        return;
    const char* text_end = text ? FindRenderedTextEnd(text, nullptr) : text;
    if (text_end > text)
    {
        TextEx(text, text_end);
        Separator();
    }

    VanVec2 sz(g.FontSize * 3 + g.Style.FramePadding.y * 2, g.FontSize * 3 + g.Style.FramePadding.y * 2);
    VanVec4 cf(col[0], col[1], col[2], (flags & VanGuiColorEditFlags_NoAlpha) ? 1.0f : col[3]);
    int cr = VAN_F32_TO_INT8_SAT(col[0]), cg = VAN_F32_TO_INT8_SAT(col[1]), cb = VAN_F32_TO_INT8_SAT(col[2]), ca = (flags & VanGuiColorEditFlags_NoAlpha) ? 255 : VAN_F32_TO_INT8_SAT(col[3]);
    VanGuiColorEditFlags flags_to_forward = VanGuiColorEditFlags_InputMask_ | VanGuiColorEditFlags_AlphaMask_;
    (void)(ColorButton("##preview", cf, (flags & flags_to_forward) | VanGuiColorEditFlags_NoTooltip, sz));
    SameLine();
    if ((flags & VanGuiColorEditFlags_InputRGB) || !(flags & VanGuiColorEditFlags_InputMask_))
    {
        if (flags & VanGuiColorEditFlags_NoAlpha)
            Text("#%02X%02X%02X\nR: %d, G: %d, B: %d\n(%.3f, %.3f, %.3f)", cr, cg, cb, cr, cg, cb, col[0], col[1], col[2]);
        else
            Text("#%02X%02X%02X%02X\nR:%d, G:%d, B:%d, A:%d\n(%.3f, %.3f, %.3f, %.3f)", cr, cg, cb, ca, cr, cg, cb, ca, col[0], col[1], col[2], col[3]);
    }
    else if (flags & VanGuiColorEditFlags_InputHSV)
    {
        if (flags & VanGuiColorEditFlags_NoAlpha)
            Text("H: %.3f, S: %.3f, V: %.3f", col[0], col[1], col[2]);
        else
            Text("H: %.3f, S: %.3f, V: %.3f, A: %.3f", col[0], col[1], col[2], col[3]);
    }
    EndTooltip();
}

void VanGui::ColorEditOptionsPopup(const float* col, VanGuiColorEditFlags flags)
{
    bool allow_opt_inputs = !(flags & VanGuiColorEditFlags_DisplayMask_);
    bool allow_opt_datatype = !(flags & VanGuiColorEditFlags_DataTypeMask_);
    if ((!allow_opt_inputs && !allow_opt_datatype) || !BeginPopup("context"))
        return;

    VanGuiContext& g = *GVanGui;
    PushItemFlag(VanGuiItemFlags_NoMarkEdited, true);
    VanGuiColorEditFlags opts = g.ColorEditOptions;
    if (allow_opt_inputs)
    {
        if (RadioButton("RGB", (opts & VanGuiColorEditFlags_DisplayRGB) != 0)) opts = (opts & ~VanGuiColorEditFlags_DisplayMask_) | VanGuiColorEditFlags_DisplayRGB;
        if (RadioButton("HSV", (opts & VanGuiColorEditFlags_DisplayHSV) != 0)) opts = (opts & ~VanGuiColorEditFlags_DisplayMask_) | VanGuiColorEditFlags_DisplayHSV;
        if (RadioButton("Hex", (opts & VanGuiColorEditFlags_DisplayHex) != 0)) opts = (opts & ~VanGuiColorEditFlags_DisplayMask_) | VanGuiColorEditFlags_DisplayHex;
    }
    if (allow_opt_datatype)
    {
        if (allow_opt_inputs) Separator();
        if (RadioButton("0..255",     (opts & VanGuiColorEditFlags_Uint8) != 0)) opts = (opts & ~VanGuiColorEditFlags_DataTypeMask_) | VanGuiColorEditFlags_Uint8;
        if (RadioButton("0.00..1.00", (opts & VanGuiColorEditFlags_Float) != 0)) opts = (opts & ~VanGuiColorEditFlags_DataTypeMask_) | VanGuiColorEditFlags_Float;
    }

    if (allow_opt_inputs || allow_opt_datatype)
        Separator();
    if (Button("Copy as..", VanVec2(-1, 0)))
        OpenPopup("Copy");
    if (BeginPopup("Copy"))
    {
        int cr = VAN_F32_TO_INT8_SAT(col[0]), cg = VAN_F32_TO_INT8_SAT(col[1]), cb = VAN_F32_TO_INT8_SAT(col[2]), ca = (flags & VanGuiColorEditFlags_NoAlpha) ? 255 : VAN_F32_TO_INT8_SAT(col[3]);
        char buf[64];
        VanFormatString(buf, VAN_COUNTOF(buf), "(%.3ff, %.3ff, %.3ff, %.3ff)", col[0], col[1], col[2], (flags & VanGuiColorEditFlags_NoAlpha) ? 1.0f : col[3]);
        if (Selectable(buf))
            SetClipboardText(buf);
        VanFormatString(buf, VAN_COUNTOF(buf), "(%d,%d,%d,%d)", cr, cg, cb, ca);
        if (Selectable(buf))
            SetClipboardText(buf);
        VanFormatString(buf, VAN_COUNTOF(buf), "#%02X%02X%02X", cr, cg, cb);
        if (Selectable(buf))
            SetClipboardText(buf);
        if (!(flags & VanGuiColorEditFlags_NoAlpha))
        {
            VanFormatString(buf, VAN_COUNTOF(buf), "#%02X%02X%02X%02X", cr, cg, cb, ca);
            if (Selectable(buf))
                SetClipboardText(buf);
        }
        EndPopup();
    }

    g.ColorEditOptions = opts;
    PopItemFlag();
    EndPopup();
}

void VanGui::ColorPickerOptionsPopup(const float* ref_col, VanGuiColorEditFlags flags)
{
    bool allow_opt_picker = !(flags & VanGuiColorEditFlags_PickerMask_);
    bool allow_opt_alpha_bar = !(flags & VanGuiColorEditFlags_NoAlpha) && !(flags & VanGuiColorEditFlags_AlphaBar);
    if ((!allow_opt_picker && !allow_opt_alpha_bar) || !BeginPopup("context"))
        return;

    VanGuiContext& g = *GVanGui;
    PushItemFlag(VanGuiItemFlags_NoMarkEdited, true);
    if (allow_opt_picker)
    {
        VanVec2 picker_size(g.FontSize * 8, VanMax(g.FontSize * 8 - (GetFrameHeight() + g.Style.ItemInnerSpacing.x), 1.0f)); // FIXME: Picker size copied from main picker function
        PushItemWidth(picker_size.x);
        for (int picker_type = 0; picker_type < 2; picker_type++)
        {
            // Draw small/thumbnail version of each picker type (over an invisible button for selection)
            if (picker_type > 0) Separator();
            PushID(picker_type);
            VanGuiColorEditFlags picker_flags = VanGuiColorEditFlags_NoInputs | VanGuiColorEditFlags_NoOptions | VanGuiColorEditFlags_NoLabel | VanGuiColorEditFlags_NoSidePreview | (flags & VanGuiColorEditFlags_NoAlpha);
            if (picker_type == 0) picker_flags |= VanGuiColorEditFlags_PickerHueBar;
            if (picker_type == 1) picker_flags |= VanGuiColorEditFlags_PickerHueWheel;
            VanVec2 backup_pos = GetCursorScreenPos();
            if (Selectable("##selectable", false, 0, picker_size)) // By default, Selectable() is closing popup
                g.ColorEditOptions = (g.ColorEditOptions & ~VanGuiColorEditFlags_PickerMask_) | (picker_flags & VanGuiColorEditFlags_PickerMask_);
            SetCursorScreenPos(backup_pos);
            VanVec4 previewing_ref_col;
            memcpy(&previewing_ref_col, ref_col, sizeof(float) * ((picker_flags & VanGuiColorEditFlags_NoAlpha) ? 3 : 4));
            (void)(ColorPicker4("##previewing_picker", &previewing_ref_col.x, picker_flags));
            PopID();
        }
        PopItemWidth();
    }
    if (allow_opt_alpha_bar)
    {
        if (allow_opt_picker) Separator();
        (void)(CheckboxFlags("Alpha Bar", &g.ColorEditOptions, VanGuiColorEditFlags_AlphaBar));
    }
    PopItemFlag();
    EndPopup();
}

//-------------------------------------------------------------------------
// [SECTION] Widgets: TreeNode, CollapsingHeader, etc.
//-------------------------------------------------------------------------
// - TreeNode()
// - TreeNodeV()
// - TreeNodeEx()
// - TreeNodeExV()
// - TreeNodeStoreStackData() [Internal]
// - TreeNodeBehavior() [Internal]
// - TreePush()
// - TreePop()
// - GetTreeNodeToLabelSpacing()
// - SetNextItemOpen()
// - CollapsingHeader()
//-------------------------------------------------------------------------

bool VanGui::TreeNode(const char* str_id, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    bool is_open = TreeNodeExV(str_id, 0, fmt, args);
    va_end(args);
    return is_open;
}

bool VanGui::TreeNode(const void* ptr_id, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    bool is_open = TreeNodeExV(ptr_id, 0, fmt, args);
    va_end(args);
    return is_open;
}

bool VanGui::TreeNode(const char* label)
{
    VanGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;
    VanGuiID id = window->GetID(label);
    return TreeNodeBehavior(id, VanGuiTreeNodeFlags_None, label, nullptr);
}

bool VanGui::TreeNodeV(const char* str_id, const char* fmt, va_list args)
{
    return TreeNodeExV(str_id, 0, fmt, args);
}

bool VanGui::TreeNodeV(const void* ptr_id, const char* fmt, va_list args)
{
    return TreeNodeExV(ptr_id, 0, fmt, args);
}

bool VanGui::TreeNodeEx(const char* label, VanGuiTreeNodeFlags flags)
{
    VanGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;
    VanGuiID id = window->GetID(label);
    return TreeNodeBehavior(id, flags, label, nullptr);
}

bool VanGui::TreeNodeEx(const char* str_id, VanGuiTreeNodeFlags flags, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    bool is_open = TreeNodeExV(str_id, flags, fmt, args);
    va_end(args);
    return is_open;
}

bool VanGui::TreeNodeEx(const void* ptr_id, VanGuiTreeNodeFlags flags, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    bool is_open = TreeNodeExV(ptr_id, flags, fmt, args);
    va_end(args);
    return is_open;
}

bool VanGui::TreeNodeExV(const char* str_id, VanGuiTreeNodeFlags flags, const char* fmt, va_list args)
{
    VanGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    VanGuiID id = window->GetID(str_id);
    const char* label, *label_end;
    VanFormatStringToTempBufferV(&label, &label_end, fmt, args);
    return TreeNodeBehavior(id, flags, label, label_end);
}

bool VanGui::TreeNodeExV(const void* ptr_id, VanGuiTreeNodeFlags flags, const char* fmt, va_list args)
{
    VanGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    VanGuiID id = window->GetID(ptr_id);
    const char* label, *label_end;
    VanFormatStringToTempBufferV(&label, &label_end, fmt, args);
    return TreeNodeBehavior(id, flags, label, label_end);
}

// The reason those two functions are not yet in public API is because I would like to design a more feature-full and generic API for this.
// They are otherwise function (cc: #3823, #9251, #7553, #6754, #5423, #2958, #2079, #1947, #1131, #722)
bool VanGui::TreeNodeGetOpen(VanGuiID storage_id)
{
    VanGuiContext& g = *GVanGui;
    VanGuiStorage* storage = g.CurrentWindow->DC.StateStorage;
    return storage->GetInt(storage_id, 0) != 0;
}

void VanGui::TreeNodeSetOpen(VanGuiID storage_id, bool is_open)
{
    VanGuiContext& g = *GVanGui;
    VanGuiStorage* storage = g.CurrentWindow->DC.StateStorage;
    storage->SetInt(storage_id, is_open ? 1 : 0);
}

bool VanGui::TreeNodeUpdateNextOpen(VanGuiID storage_id, VanGuiTreeNodeFlags flags)
{
    // Leaf node always open a new tree/id scope. If you never use it, add VanGuiTreeNodeFlags_NoTreePushOnOpen.
    if (flags & VanGuiTreeNodeFlags_Leaf)
        return true;

    // We only write to the tree storage if the user clicks, or explicitly use the SetNextItemOpen function
    VanGuiContext& g = *GVanGui;
    VanGuiWindow* window = g.CurrentWindow;
    VanGuiStorage* storage = window->DC.StateStorage;

    bool is_open;
    if (g.NextItemData.HasFlags & VanGuiNextItemDataFlags_HasOpen)
    {
        if (g.NextItemData.OpenCond & VanGuiCond_Always)
        {
            is_open = g.NextItemData.OpenVal;
            TreeNodeSetOpen(storage_id, is_open);
        }
        else
        {
            // We treat VanGuiCond_Once and VanGuiCond_FirstUseEver the same because tree node state are not saved persistently.
            const int stored_value = storage->GetInt(storage_id, -1);
            if (stored_value == -1)
            {
                is_open = g.NextItemData.OpenVal;
                TreeNodeSetOpen(storage_id, is_open);
            }
            else
            {
                is_open = stored_value != 0;
            }
        }
    }
    else
    {
        is_open = storage->GetInt(storage_id, (flags & VanGuiTreeNodeFlags_DefaultOpen) ? 1 : 0) != 0;
    }

    // When logging is enabled, we automatically expand tree nodes (but *NOT* collapsing headers.. seems like sensible behavior).
    // NB- If we are above max depth we still allow manually opened nodes to be logged.
    if (g.LogEnabled && !(flags & VanGuiTreeNodeFlags_NoAutoOpenOnLog) && (window->DC.TreeDepth - g.LogDepthRef) < g.LogDepthToExpand)
        is_open = true;

    return is_open;
}

// Store VanGuiTreeNodeStackData for just submitted node.
// Currently only supports 32 level deep and we are fine with (1 << Depth) overflowing into a zero, easy to increase.
static void TreeNodeStoreStackData(VanGuiTreeNodeFlags flags, float x1)
{
    VanGuiContext& g = *GVanGui;
    VanGuiWindow* window = g.CurrentWindow;

    g.TreeNodeStack.resize(g.TreeNodeStack.Size + 1);
    VanGuiTreeNodeStackData* tree_node_data = &g.TreeNodeStack.Data[g.TreeNodeStack.Size - 1];
    tree_node_data->ID = g.LastItemData.ID;
    tree_node_data->TreeFlags = flags;
    tree_node_data->ItemFlags = g.LastItemData.ItemFlags;
    tree_node_data->NavRect = g.LastItemData.NavRect;

    // Initially I tried to latch value for GetColorU32(VanGuiCol_TreeLines) but it's not a good trade-off for very large trees.
    const bool draw_lines = (flags & (VanGuiTreeNodeFlags_DrawLinesFull | VanGuiTreeNodeFlags_DrawLinesToNodes)) != 0;
    tree_node_data->DrawLinesX1 = draw_lines ? (x1 + g.FontSize * 0.5f + g.Style.FramePadding.x) : +FLT_MAX;
    tree_node_data->DrawLinesTableColumn = (draw_lines && g.CurrentTable) ? static_cast<VanGuiTableColumnIdx>(g.CurrentTable->CurrentColumn) : -1;
    tree_node_data->DrawLinesToNodesY2 = -FLT_MAX;
    window->DC.TreeHasStackDataDepthMask |= (1 << window->DC.TreeDepth);
    if (flags & VanGuiTreeNodeFlags_DrawLinesToNodes)
        window->DC.TreeRecordsClippedNodesY2Mask |= (1 << window->DC.TreeDepth);
}

bool VanGui::TreeNodeBehavior(VanGuiID id, VanGuiTreeNodeFlags flags, const char* label, const char* label_end)
{
    VanGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    VanGuiContext& g = *GVanGui;
    const VanGuiStyle& style = g.Style;

    // When not framed, we vertically increase height up to typical framed widget height
    const bool display_frame = (flags & VanGuiTreeNodeFlags_Framed) != 0;
    const bool use_frame_padding = (display_frame || (flags & VanGuiTreeNodeFlags_FramePadding));
    const VanVec2 padding = use_frame_padding ? style.FramePadding : VanVec2(style.FramePadding.x, VanMin(window->DC.CurrLineTextBaseOffset, style.FramePadding.y));

    if (!label_end)
        label_end = FindRenderedTextEnd(label);
    const VanVec2 label_size = CalcTextSize(label, label_end, false);

    const float text_offset_x = g.FontSize + (display_frame ? padding.x * 3 : padding.x * 2);   // Collapsing arrow width + Spacing
    const float text_offset_y = use_frame_padding ? VanMax(style.FramePadding.y, window->DC.CurrLineTextBaseOffset) : window->DC.CurrLineTextBaseOffset; // Latch before ItemSize changes it
    const float text_width = g.FontSize + label_size.x + padding.x * 2;                         // Include collapsing arrow

    const float frame_height = label_size.y + padding.y * 2;
    const bool span_all_columns = (flags & VanGuiTreeNodeFlags_SpanAllColumns) != 0 && (g.CurrentTable != nullptr);
    const bool span_all_columns_label = (flags & VanGuiTreeNodeFlags_LabelSpanAllColumns) != 0 && (g.CurrentTable != nullptr);
    VanRect frame_bb;
    frame_bb.Min.x = span_all_columns ? window->ParentWorkRect.Min.x : (flags & VanGuiTreeNodeFlags_SpanFullWidth) ? window->WorkRect.Min.x : window->DC.CursorPos.x;
    frame_bb.Min.y = window->DC.CursorPos.y + (text_offset_y - padding.y);
    frame_bb.Max.x = span_all_columns ? window->ParentWorkRect.Max.x : (flags & VanGuiTreeNodeFlags_SpanLabelWidth) ? window->DC.CursorPos.x + text_width + padding.x : window->WorkRect.Max.x;
    frame_bb.Max.y = window->DC.CursorPos.y + (text_offset_y - padding.y) + frame_height;
    if (display_frame)
    {
        const float outer_extend = VAN_TRUNC(window->WindowPadding.x * 0.5f); // Framed header expand a little outside of current limits
        frame_bb.Min.x -= outer_extend;
        frame_bb.Max.x += outer_extend;
    }

    VanVec2 text_pos(window->DC.CursorPos.x + text_offset_x, window->DC.CursorPos.y + text_offset_y);
    ItemSize(VanVec2(text_width, frame_height), padding.y);

    // For regular tree nodes, we arbitrary allow to click past 2 worth of ItemSpacing
    VanRect interact_bb = frame_bb;
    if ((flags & (VanGuiTreeNodeFlags_Framed | VanGuiTreeNodeFlags_SpanAvailWidth | VanGuiTreeNodeFlags_SpanFullWidth | VanGuiTreeNodeFlags_SpanLabelWidth | VanGuiTreeNodeFlags_SpanAllColumns)) == 0)
        interact_bb.Max.x = frame_bb.Min.x + text_width + (label_size.x > 0.0f ? style.ItemSpacing.x * 2.0f : 0.0f);

    // Compute open and multi-select states before ItemAdd() as it clear NextItem data.
    VanGuiID storage_id = (g.NextItemData.HasFlags & VanGuiNextItemDataFlags_HasStorageID) ? g.NextItemData.StorageId : id;
    bool is_open = TreeNodeUpdateNextOpen(storage_id, flags);

    bool is_visible;
    if (span_all_columns || span_all_columns_label)
    {
        // Modify ClipRect for the ItemAdd(), faster than doing a PushColumnsBackground/PushTableBackgroundChannel for every Selectable..
        const float backup_clip_rect_min_x = window->ClipRect.Min.x;
        const float backup_clip_rect_max_x = window->ClipRect.Max.x;
        window->ClipRect.Min.x = window->ParentWorkRect.Min.x;
        window->ClipRect.Max.x = window->ParentWorkRect.Max.x;
        is_visible = ItemAdd(interact_bb, id);
        window->ClipRect.Min.x = backup_clip_rect_min_x;
        window->ClipRect.Max.x = backup_clip_rect_max_x;
    }
    else
    {
        is_visible = ItemAdd(interact_bb, id);
    }
    g.LastItemData.StatusFlags |= VanGuiItemStatusFlags_HasDisplayRect;
    g.LastItemData.DisplayRect = frame_bb;

    // If a NavLeft request is happening and VanGuiTreeNodeFlags_NavLeftJumpsToParent enabled:
    // Store data for the current depth to allow returning to this node from any child item.
    // For this purpose we essentially compare if g.NavIdIsAlive went from 0 to 1 between TreeNode() and TreePop().
    // It will become tempting to enable VanGuiTreeNodeFlags_NavLeftJumpsToParent by default or move it to VanGuiStyle.
    bool store_tree_node_stack_data = false;
    if ((flags & VanGuiTreeNodeFlags_DrawLinesMask_) == 0)
        flags |= g.Style.TreeLinesFlags;
    const bool draw_tree_lines = (flags & (VanGuiTreeNodeFlags_DrawLinesFull | VanGuiTreeNodeFlags_DrawLinesToNodes)) && (frame_bb.Min.y < window->ClipRect.Max.y) && (g.Style.TreeLinesSize > 0.0f);
    if (!(flags & VanGuiTreeNodeFlags_NoTreePushOnOpen))
    {
        store_tree_node_stack_data = draw_tree_lines;
        if ((flags & VanGuiTreeNodeFlags_NavLeftJumpsToParent) && !g.NavIdIsAlive)
            if (g.NavMoveDir == VanGuiDir_Left && g.NavWindow == window && NavMoveRequestButNoResultYet())
                store_tree_node_stack_data = true;
    }

    const bool is_leaf = (flags & VanGuiTreeNodeFlags_Leaf) != 0;
    if (!is_visible)
    {
        if ((flags & VanGuiTreeNodeFlags_DrawLinesToNodes) && (window->DC.TreeRecordsClippedNodesY2Mask & (1 << (window->DC.TreeDepth - 1))))
        {
            VanGuiTreeNodeStackData* parent_data = &g.TreeNodeStack.Data[g.TreeNodeStack.Size - 1];
            parent_data->DrawLinesToNodesY2 = VanMax(parent_data->DrawLinesToNodesY2, window->DC.CursorPos.y); // Don't need to aim to mid Y position as we are clipped anyway.
            if (frame_bb.Min.y >= window->ClipRect.Max.y)
                window->DC.TreeRecordsClippedNodesY2Mask &= ~(1 << (window->DC.TreeDepth - 1)); // Done
        }
        if (is_open && store_tree_node_stack_data)
            TreeNodeStoreStackData(flags, text_pos.x - text_offset_x); // Call before TreePushOverrideID()
        if (is_open && !(flags & VanGuiTreeNodeFlags_NoTreePushOnOpen))
            TreePushOverrideID(id);
        VANGUI_TEST_ENGINE_ITEM_INFO(g.LastItemData.ID, label, g.LastItemData.StatusFlags | (is_leaf ? 0 : VanGuiItemStatusFlags_Openable) | (is_open ? VanGuiItemStatusFlags_Opened : 0));
        return is_open;
    }

    if (span_all_columns || span_all_columns_label)
    {
        TablePushBackgroundChannel();
        g.LastItemData.StatusFlags |= VanGuiItemStatusFlags_HasClipRect;
        g.LastItemData.ClipRect = window->ClipRect;
    }

    VanGuiButtonFlags button_flags = VanGuiTreeNodeFlags_None;
    if ((flags & VanGuiTreeNodeFlags_AllowOverlap) || (g.LastItemData.ItemFlags & VanGuiItemFlags_AllowOverlap))
        button_flags |= VanGuiButtonFlags_AllowOverlap;
    if (!is_leaf)
        button_flags |= VanGuiButtonFlags_PressedOnDragDropHold;

    // We allow clicking on the arrow section with keyboard modifiers held, in order to easily
    // allow browsing a tree while preserving selection with code implementing multi-selection patterns.
    // When clicking on the rest of the tree node we always disallow keyboard modifiers.
    const float arrow_hit_x1 = (text_pos.x - text_offset_x) - style.TouchExtraPadding.x;
    const float arrow_hit_x2 = (text_pos.x - text_offset_x) + (g.FontSize + padding.x * 2.0f) + style.TouchExtraPadding.x;
    const bool is_mouse_x_over_arrow = (g.IO.MousePos.x >= arrow_hit_x1 && g.IO.MousePos.x < arrow_hit_x2);

    const bool is_multi_select = (g.LastItemData.ItemFlags & VanGuiItemFlags_IsMultiSelect) != 0;
    if (is_multi_select) // We absolutely need to distinguish open vs select so _OpenOnArrow comes by default
        flags |= (flags & VanGuiTreeNodeFlags_OpenOnMask_) == 0 ? VanGuiTreeNodeFlags_OpenOnArrow | VanGuiTreeNodeFlags_OpenOnDoubleClick : VanGuiTreeNodeFlags_OpenOnArrow;

    // Open behaviors can be altered with the _OpenOnArrow and _OnOnDoubleClick flags.
    // Some alteration have subtle effects (e.g. toggle on MouseUp vs MouseDown events) due to requirements for multi-selection and drag and drop support.
    // - Single-click on label = Toggle on MouseUp (default, when _OpenOnArrow=0)
    // - Single-click on arrow = Toggle on MouseDown (when _OpenOnArrow=0)
    // - Single-click on arrow = Toggle on MouseDown (when _OpenOnArrow=1)
    // - Double-click on label = Toggle on MouseDoubleClick (when _OpenOnDoubleClick=1)
    // - Double-click on arrow = Toggle on MouseDoubleClick (when _OpenOnDoubleClick=1 and _OpenOnArrow=0)
    // It is rather standard that arrow click react on Down rather than Up.
    // We set VanGuiButtonFlags_PressedOnClickRelease on OpenOnDoubleClick because we want the item to be active on the initial MouseDown in order for drag and drop to work.
    if (is_mouse_x_over_arrow)
        button_flags |= VanGuiButtonFlags_PressedOnClick;
    else if (flags & VanGuiTreeNodeFlags_OpenOnDoubleClick)
        button_flags |= VanGuiButtonFlags_PressedOnClickRelease | VanGuiButtonFlags_PressedOnDoubleClick;
    else
        button_flags |= VanGuiButtonFlags_PressedOnClickRelease;
    if (flags & VanGuiTreeNodeFlags_NoNavFocus)
        button_flags |= VanGuiButtonFlags_NoNavFocus;

    bool selected = (flags & VanGuiTreeNodeFlags_Selected) != 0;
    const bool was_selected = selected;

    // Multi-selection support (header)
    if (is_multi_select)
    {
        // Handle multi-select + alter button flags for it
        MultiSelectItemHeader(id, &selected, &button_flags);
        if (is_mouse_x_over_arrow)
            button_flags = (button_flags | VanGuiButtonFlags_PressedOnClick) & ~VanGuiButtonFlags_PressedOnClickRelease;
    }
    else
    {
        if (window != g.HoveredWindow || !is_mouse_x_over_arrow)
            button_flags |= VanGuiButtonFlags_NoKeyModsAllowed;
    }

    bool hovered, held;
    bool pressed = ButtonBehavior(interact_bb, id, &hovered, &held, button_flags);
    bool toggled = false;
    if (!is_leaf)
    {
        if (pressed && g.DragDropHoldJustPressedId != id)
        {
            if ((flags & VanGuiTreeNodeFlags_OpenOnMask_) == 0 || (g.NavActivateId == id && !is_multi_select))
                toggled = true; // Single click
            if (flags & VanGuiTreeNodeFlags_OpenOnArrow)
                toggled |= is_mouse_x_over_arrow && !g.NavHighlightItemUnderNav; // Lightweight equivalent of IsMouseHoveringRect() since ButtonBehavior() already did the job
            if ((flags & VanGuiTreeNodeFlags_OpenOnDoubleClick) && g.IO.MouseClickedCount[0] == 2)
                toggled = true; // Double click
        }
        else if (pressed && g.DragDropHoldJustPressedId == id)
        {
            VAN_ASSERT(button_flags & VanGuiButtonFlags_PressedOnDragDropHold);
            if (!is_open) // When using Drag and Drop "hold to open" we keep the node highlighted after opening, but never close it again.
                toggled = true;
            else
                pressed = false; // Cancel press so it doesn't trigger selection.
        }

        if (g.NavId == id && g.NavMoveDir == VanGuiDir_Left && is_open)
        {
            toggled = true;
            NavClearPreferredPosForAxis(VanGuiAxis_X);
            NavMoveRequestCancel();
        }
        if (g.NavId == id && g.NavMoveDir == VanGuiDir_Right && !is_open) // If there's something upcoming on the line we may want to give it the priority?
        {
            toggled = true;
            NavClearPreferredPosForAxis(VanGuiAxis_X);
            NavMoveRequestCancel();
        }

        if (toggled)
        {
            is_open = !is_open;
            window->DC.StateStorage->SetInt(storage_id, is_open);
            g.LastItemData.StatusFlags |= VanGuiItemStatusFlags_ToggledOpen;
        }
    }

    // Multi-selection support (footer)
    if (is_multi_select)
    {
        bool pressed_copy = pressed && !toggled;
        MultiSelectItemFooter(id, &selected, &pressed_copy);
        if (pressed)
            SetNavID(id, window->DC.NavLayerCurrent, g.CurrentFocusScopeId, interact_bb);
    }

    if (selected != was_selected)
        g.LastItemData.StatusFlags |= VanGuiItemStatusFlags_ToggledSelection;

    // Render
    {
        const VanU32 text_col = GetColorU32(VanGuiCol_Text);
        VanGuiNavRenderCursorFlags nav_render_cursor_flags = VanGuiNavRenderCursorFlags_Compact;
        if (is_multi_select)
            nav_render_cursor_flags |= VanGuiNavRenderCursorFlags_AlwaysDraw; // Always show the nav rectangle
        if (display_frame)
        {
            // Framed type
            const VanU32 bg_col = GetColorU32((held && hovered) ? VanGuiCol_HeaderActive : hovered ? VanGuiCol_HeaderHovered : VanGuiCol_Header);
            RenderFrame(frame_bb.Min, frame_bb.Max, bg_col, true, style.FrameRounding);
            RenderNavCursor(frame_bb, id, nav_render_cursor_flags);
            if (span_all_columns && !span_all_columns_label)
                TablePopBackgroundChannel();
            if (flags & VanGuiTreeNodeFlags_Bullet)
                RenderBullet(window->DrawList, VanVec2(text_pos.x - text_offset_x * 0.60f, text_pos.y + g.FontSize * 0.5f), text_col);
            else if (!is_leaf)
                RenderArrow(window->DrawList, VanVec2(text_pos.x - text_offset_x + padding.x, text_pos.y), text_col, is_open ? ((flags & VanGuiTreeNodeFlags_UpsideDownArrow) ? VanGuiDir_Up : VanGuiDir_Down) : VanGuiDir_Right, 1.0f);
            else // Leaf without bullet, left-adjusted text
                text_pos.x -= text_offset_x - padding.x;
            if (flags & VanGuiTreeNodeFlags_ClipLabelForTrailingButton)
                frame_bb.Max.x -= g.FontSize + style.FramePadding.x;
            if (g.LogEnabled)
                LogSetNextTextDecoration("###", "###");
        }
        else
        {
            // Unframed typed for tree nodes
            if (hovered || selected)
            {
                const VanU32 bg_col = GetColorU32((held && hovered) ? VanGuiCol_HeaderActive : hovered ? VanGuiCol_HeaderHovered : VanGuiCol_Header);
                RenderFrame(frame_bb.Min, frame_bb.Max, bg_col, false);
            }
            RenderNavCursor(frame_bb, id, nav_render_cursor_flags);
            if (span_all_columns && !span_all_columns_label)
                TablePopBackgroundChannel();
            if (flags & VanGuiTreeNodeFlags_Bullet)
                RenderBullet(window->DrawList, VanVec2(text_pos.x - text_offset_x * 0.5f, text_pos.y + g.FontSize * 0.5f), text_col);
            else if (!is_leaf)
                RenderArrow(window->DrawList, VanVec2(text_pos.x - text_offset_x + padding.x, text_pos.y + g.FontSize * 0.15f), text_col, is_open ? ((flags & VanGuiTreeNodeFlags_UpsideDownArrow) ? VanGuiDir_Up : VanGuiDir_Down) : VanGuiDir_Right, 0.70f);
            if (g.LogEnabled)
                LogSetNextTextDecoration(">", nullptr);
        }

        if (draw_tree_lines)
            TreeNodeDrawLineToChildNode(VanVec2(text_pos.x - text_offset_x + padding.x, text_pos.y + g.FontSize * 0.5f));

        // Label
        if (display_frame)
            RenderTextClipped(text_pos, frame_bb.Max, label, label_end, &label_size);
        else
            RenderText(text_pos, label, label_end, false);

        if (span_all_columns_label)
            TablePopBackgroundChannel();
    }

    if (is_open && store_tree_node_stack_data)
        TreeNodeStoreStackData(flags, text_pos.x - text_offset_x); // Call before TreePushOverrideID()
    if (is_open && !(flags & VanGuiTreeNodeFlags_NoTreePushOnOpen))
        TreePushOverrideID(id); // Could use TreePush(label) but this avoid computing twice

    VANGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags | (is_leaf ? 0 : VanGuiItemStatusFlags_Openable) | (is_open ? VanGuiItemStatusFlags_Opened : 0));
    return is_open;
}

// Draw horizontal line from our parent node
// This is only called for visible child nodes so we are not too fussy anymore about performances
void VanGui::TreeNodeDrawLineToChildNode(const VanVec2& target_pos)
{
    VanGuiContext& g = *GVanGui;
    VanGuiWindow* window = g.CurrentWindow;
    if (window->DC.TreeDepth == 0 || (window->DC.TreeHasStackDataDepthMask & (1 << (window->DC.TreeDepth - 1))) == 0)
        return;

    VanGuiTreeNodeStackData* parent_data = &g.TreeNodeStack.Data[g.TreeNodeStack.Size - 1];
    float x1 = VanTrunc(parent_data->DrawLinesX1);
    float x2 = VanTrunc(target_pos.x - g.Style.ItemInnerSpacing.x);
    float y = VanTrunc(target_pos.y);
    float rounding = (g.Style.TreeLinesRounding > 0.0f) ? VanMin(x2 - x1, g.Style.TreeLinesRounding) : 0.0f;
    parent_data->DrawLinesToNodesY2 = VanMax(parent_data->DrawLinesToNodesY2, y - rounding);
    if (x1 >= x2)
        return;
    if (rounding > 0.0f)
    {
        x1 += 0.5f + rounding;
        window->DrawList->PathArcToFast(VanVec2(x1, y - rounding), rounding, 6, 3);
        if (x1 < x2)
            window->DrawList->PathLineTo(VanVec2(x2, y));
        window->DrawList->PathStroke(GetColorU32(VanGuiCol_TreeLines), g.Style.TreeLinesSize);
    }
    else
    {
        window->DrawList->AddLineH(x1, x2, y, GetColorU32(VanGuiCol_TreeLines), g.Style.TreeLinesSize);
    }
}

// Draw vertical line of the hierarchy
void VanGui::TreeNodeDrawLineToTreePop(const VanGuiTreeNodeStackData* data)
{
    VanGuiContext& g = *GVanGui;
    VanGuiWindow* window = g.CurrentWindow;
    float y1 = VanMax(data->NavRect.Max.y, window->ClipRect.Min.y);
    float y2 = data->DrawLinesToNodesY2;
    if (data->TreeFlags & VanGuiTreeNodeFlags_DrawLinesFull)
    {
        float y2_full = window->DC.CursorPos.y;
        if (g.CurrentTable)
            y2_full = VanMax(g.CurrentTable->RowPosY2, y2_full);
        y2_full = VanTrunc(y2_full - g.Style.ItemSpacing.y - g.FontSize * 0.5f);
        if (y2 + (g.Style.ItemSpacing.y + g.Style.TreeLinesRounding) < y2_full) // FIXME: threshold to use ToNodes Y2 instead of Full Y2 when close by ItemSpacing.y
            y2 = y2_full;
    }
    y2 = VanMin(y2, window->ClipRect.Max.y);
    if (y2 <= y1)
        return;
    float x = VanTrunc(data->DrawLinesX1);
    if (data->DrawLinesTableColumn != -1)
        TablePushColumnChannel(data->DrawLinesTableColumn);
    window->DrawList->AddLineV(x, y1, y2, GetColorU32(VanGuiCol_TreeLines), g.Style.TreeLinesSize);
    if (data->DrawLinesTableColumn != -1)
        TablePopColumnChannel();
}

void VanGui::TreePush(const char* str_id)
{
    VanGuiWindow* window = GetCurrentWindow();
    Indent();
    window->DC.TreeDepth++;
    PushID(str_id);
}

void VanGui::TreePush(const void* ptr_id)
{
    VanGuiWindow* window = GetCurrentWindow();
    Indent();
    window->DC.TreeDepth++;
    PushID(ptr_id);
}

void VanGui::TreePushOverrideID(VanGuiID id)
{
    VanGuiContext& g = *GVanGui;
    VanGuiWindow* window = g.CurrentWindow;
    Indent();
    window->DC.TreeDepth++;
    PushOverrideID(id);
}

void VanGui::TreePop()
{
    VanGuiContext& g = *GVanGui;
    VanGuiWindow* window = g.CurrentWindow;
    Unindent();

    window->DC.TreeDepth--;
    VanU32 tree_depth_mask = (1 << window->DC.TreeDepth);

    if (window->DC.TreeHasStackDataDepthMask & tree_depth_mask)
    {
        const VanGuiTreeNodeStackData* data = &g.TreeNodeStack.Data[g.TreeNodeStack.Size - 1];
        VAN_ASSERT(data->ID == window->IDStack.back());

        // Handle Left arrow to move to parent tree node (when VanGuiTreeNodeFlags_NavLeftJumpsToParent is enabled)
        if (data->TreeFlags & VanGuiTreeNodeFlags_NavLeftJumpsToParent)
            if (g.NavIdIsAlive && g.NavMoveDir == VanGuiDir_Left && g.NavWindow == window && NavMoveRequestButNoResultYet())
                NavMoveRequestResolveWithPastTreeNode(&g.NavMoveResultLocal, data);

        // Draw hierarchy lines
        if (data->DrawLinesX1 != +FLT_MAX && window->DC.CursorPos.y >= window->ClipRect.Min.y)
            TreeNodeDrawLineToTreePop(data);

        g.TreeNodeStack.pop_back();
        window->DC.TreeHasStackDataDepthMask &= ~tree_depth_mask;
        window->DC.TreeRecordsClippedNodesY2Mask &= ~tree_depth_mask;
    }

    VAN_ASSERT(window->IDStack.Size > 1); // There should always be 1 element in the IDStack (pushed during window creation). If this triggers you called TreePop/PopID too much.
    PopID();
}

// Horizontal distance preceding label when using TreeNode() or Bullet()
float VanGui::GetTreeNodeToLabelSpacing()
{
    VanGuiContext& g = *GVanGui;
    return g.FontSize + (g.Style.FramePadding.x * 2.0f);
}

// Set next TreeNode/CollapsingHeader open state.
void VanGui::SetNextItemOpen(bool is_open, VanGuiCond cond)
{
    VanGuiContext& g = *GVanGui;
    if (g.CurrentWindow->SkipItems)
        return;
    g.NextItemData.HasFlags |= VanGuiNextItemDataFlags_HasOpen;
    g.NextItemData.OpenVal = is_open;
    g.NextItemData.OpenCond = static_cast<VanU8>(cond ? cond : VanGuiCond_Always);
}

// Set next TreeNode/CollapsingHeader storage id.
void VanGui::SetNextItemStorageID(VanGuiID storage_id)
{
    VanGuiContext& g = *GVanGui;
    if (g.CurrentWindow->SkipItems)
        return;
    g.NextItemData.HasFlags |= VanGuiNextItemDataFlags_HasStorageID;
    g.NextItemData.StorageId = storage_id;
}

// CollapsingHeader returns true when opened but do not indent nor push into the ID stack (because of the VanGuiTreeNodeFlags_NoTreePushOnOpen flag).
// This is basically the same as calling TreeNodeEx(label, VanGuiTreeNodeFlags_CollapsingHeader). You can remove the _NoTreePushOnOpen flag if you want behavior closer to normal TreeNode().
bool VanGui::CollapsingHeader(const char* label, VanGuiTreeNodeFlags flags)
{
    VanGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;
    VanGuiID id = window->GetID(label);
    return TreeNodeBehavior(id, flags | VanGuiTreeNodeFlags_CollapsingHeader, label);
}

// p_visible == nullptr                        : regular collapsing header
// p_visible != nullptr && *p_visible == true  : show a small close button on the corner of the header, clicking the button will set *p_visible = false
// p_visible != nullptr && *p_visible == false : do not show the header at all
// Do not mistake this with the Open state of the header itself, which you can adjust with SetNextItemOpen() or VanGuiTreeNodeFlags_DefaultOpen.
bool VanGui::CollapsingHeader(const char* label, bool* p_visible, VanGuiTreeNodeFlags flags)
{
    VanGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    if (p_visible && !*p_visible)
        return false;

    VanGuiID id = window->GetID(label);
    flags |= VanGuiTreeNodeFlags_CollapsingHeader;
    if (p_visible)
        flags |= VanGuiTreeNodeFlags_AllowOverlap | static_cast<VanGuiTreeNodeFlags>(VanGuiTreeNodeFlags_ClipLabelForTrailingButton);
    bool is_open = TreeNodeBehavior(id, flags, label);
    if (p_visible != nullptr)
    {
        // Create a small overlapping close button
        // FIXME: We can evolve this into user accessible helpers to add extra buttons on title bars, headers, etc.
        // FIXME: CloseButton can overlap into text, need find a way to clip the text somehow.
        VanGuiContext& g = *GVanGui;
        VanGuiLastItemData last_item_backup = g.LastItemData;
        float button_size = g.FontSize;
        float button_x = VanMax(g.LastItemData.Rect.Min.x, g.LastItemData.Rect.Max.x - g.Style.FramePadding.x - button_size);
        float button_y = g.LastItemData.Rect.Min.y + g.Style.FramePadding.y;
        VanGuiID close_button_id = GetIDWithSeed("#CLOSE", nullptr, id);
        if (CloseButton(close_button_id, VanVec2(button_x, button_y)))
            *p_visible = false;
        g.LastItemData = last_item_backup;
    }

    return is_open;
}

//-------------------------------------------------------------------------
// [SECTION] Widgets: Selectable
//-------------------------------------------------------------------------
// - Selectable()
//-------------------------------------------------------------------------

// Tip: pass a non-visible label (e.g. "##hello") then you can use the space to draw other text or image.
// But you need to make sure the ID is unique, e.g. enclose calls in PushID/PopID or use ##unique_id.
// With this scheme, VanGuiSelectableFlags_SpanAllColumns and VanGuiSelectableFlags_AllowOverlap are also frequently used flags.
// FIXME: Selectable() with (size.x == 0.0f) and (SelectableTextAlign.x > 0.0f) followed by SameLine() is currently not supported.
bool VanGui::Selectable(const char* label, bool selected, VanGuiSelectableFlags flags, const VanVec2& size_arg)
{
    VanGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    VanGuiContext& g = *GVanGui;
    const VanGuiStyle& style = g.Style;

    // Submit label or explicit size to ItemSize(), whereas ItemAdd() will submit a larger/spanning rectangle.
    VanGuiID id = window->GetID(label);
    const char* label_end = FindRenderedTextEnd(label);
    VanVec2 label_size = CalcTextSize(label, label_end, false);
    VanVec2 size(size_arg.x != 0.0f ? size_arg.x : label_size.x, size_arg.y != 0.0f ? size_arg.y : label_size.y);
    VanVec2 pos = window->DC.CursorPos;
    pos.y += window->DC.CurrLineTextBaseOffset;
    ItemSize(size, 0.0f);

    // Fill horizontal space
    // We don't support (size < 0.0f) in Selectable() because the ItemSpacing extension would make explicitly right-aligned sizes not visibly match other widgets.
    const bool span_all_columns = (flags & VanGuiSelectableFlags_SpanAllColumns) != 0;
    const float min_x = span_all_columns ? window->ParentWorkRect.Min.x : pos.x;
    const float max_x = span_all_columns ? window->ParentWorkRect.Max.x : window->WorkRect.Max.x;
    if (size_arg.x == 0.0f || (flags & VanGuiSelectableFlags_SpanAvailWidth))
        size.x = VanMax(label_size.x, max_x - min_x);

    // Selectables are meant to be tightly packed together with no click-gap, so we extend their box to cover spacing between selectable.
    // FIXME: Not part of layout so not included in clipper calculation, but ItemSize currently doesn't allow offsetting CursorPos.
    VanRect bb(min_x, pos.y, min_x + size.x, pos.y + size.y);
    if ((flags & VanGuiSelectableFlags_NoPadWithHalfSpacing) == 0)
    {
        const float spacing_x = span_all_columns ? 0.0f : style.ItemSpacing.x;
        const float spacing_y = style.ItemSpacing.y;
        const float spacing_L = VAN_TRUNC(spacing_x * 0.50f);
        const float spacing_U = VAN_TRUNC(spacing_y * 0.50f);
        bb.Min.x -= spacing_L;
        bb.Min.y -= spacing_U;
        bb.Max.x += (spacing_x - spacing_L);
        bb.Max.y += (spacing_y - spacing_U);
    }
    //if (g.IO.KeyCtrl) { GetForegroundDrawList()->AddRect(bb.Min, bb.Max, VAN_COL32(0, 255, 0, 255)); }

    const bool disabled_item = (flags & VanGuiSelectableFlags_Disabled) != 0;
    const VanGuiItemFlags extra_item_flags = disabled_item ? static_cast<VanGuiItemFlags>(VanGuiItemFlags_Disabled) : VanGuiItemFlags_None;
    bool is_visible;
    if (span_all_columns)
    {
        // Modify ClipRect for the ItemAdd(), faster than doing a PushColumnsBackground/PushTableBackgroundChannel for every Selectable..
        const float backup_clip_rect_min_x = window->ClipRect.Min.x;
        const float backup_clip_rect_max_x = window->ClipRect.Max.x;
        window->ClipRect.Min.x = window->ParentWorkRect.Min.x;
        window->ClipRect.Max.x = window->ParentWorkRect.Max.x;
        is_visible = ItemAdd(bb, id, nullptr, extra_item_flags);
        window->ClipRect.Min.x = backup_clip_rect_min_x;
        window->ClipRect.Max.x = backup_clip_rect_max_x;
    }
    else
    {
        is_visible = ItemAdd(bb, id, nullptr, extra_item_flags);
    }

    const bool is_multi_select = (g.LastItemData.ItemFlags & VanGuiItemFlags_IsMultiSelect) != 0;
    if (!is_visible)
        if (!is_multi_select || !g.BoxSelectState.UnclipMode || !g.BoxSelectState.UnclipRect.Overlaps(bb)) // Extra layer of "no logic clip" for box-select support (would be more overhead to add to ItemAdd)
            return false;

    const bool disabled_global = (g.CurrentItemFlags & VanGuiItemFlags_Disabled) != 0;
    if (disabled_item && !disabled_global) // Only testing this as an optimization
        BeginDisabled();

    // FIXME: We can standardize the behavior of those two, we could also keep the fast path of override ClipRect + full push on render only,
    // which would be advantageous since most selectable are not selected.
    if (span_all_columns)
    {
        if (g.CurrentTable)
            TablePushBackgroundChannel();
        else if (window->DC.CurrentColumns)
            PushColumnsBackground();
        g.LastItemData.StatusFlags |= VanGuiItemStatusFlags_HasClipRect;
        g.LastItemData.ClipRect = window->ClipRect;
    }

    // We use NoHoldingActiveID on menus so user can click and _hold_ on a menu then drag to browse child entries
    VanGuiButtonFlags button_flags = 0;
    if (flags & VanGuiSelectableFlags_NoHoldingActiveID) { button_flags |= VanGuiButtonFlags_NoHoldingActiveId; }
    if (flags & VanGuiSelectableFlags_NoSetKeyOwner)     { button_flags |= VanGuiButtonFlags_NoSetKeyOwner; }
    if (flags & VanGuiSelectableFlags_SelectOnClick)     { button_flags |= VanGuiButtonFlags_PressedOnClick; }
    if (flags & VanGuiSelectableFlags_SelectOnRelease)   { button_flags |= VanGuiButtonFlags_PressedOnRelease; }
    if (flags & VanGuiSelectableFlags_AllowDoubleClick)  { button_flags |= VanGuiButtonFlags_PressedOnClickRelease | VanGuiButtonFlags_PressedOnDoubleClick; }
    if ((flags & VanGuiSelectableFlags_AllowOverlap) || (g.LastItemData.ItemFlags & VanGuiItemFlags_AllowOverlap)) { button_flags |= VanGuiButtonFlags_AllowOverlap; }

    // Multi-selection support (header)
    const bool was_selected = selected;
    if (is_multi_select)
    {
        // Handle multi-select + alter button flags for it
        MultiSelectItemHeader(id, &selected, &button_flags);
    }

    bool hovered, held;
    bool pressed = ButtonBehavior(bb, id, &hovered, &held, button_flags);
    bool auto_selected = false;

    // Multi-selection support (footer)
    if (is_multi_select)
    {
        MultiSelectItemFooter(id, &selected, &pressed);
    }
    else
    {
        // Auto-select when moved into
        // - This will be more fully fleshed in the range-select branch
        // - This is not exposed as it won't nicely work with some user side handling of shift/control
        // - We cannot do 'if (g.NavJustMovedToId != id) { selected = false; pressed = was_selected; }' for two reasons
        //   - (1) it would require focus scope to be set, need exposing PushFocusScope() or equivalent (e.g. BeginSelection() calling PushFocusScope())
        //   - (2) usage will fail with clipped items
        //   The multi-select API aim to fix those issues, e.g. may be replaced with a BeginSelection() API.
        if ((flags & VanGuiSelectableFlags_SelectOnNav) && g.NavJustMovedToId != 0 && g.NavJustMovedToFocusScopeId == g.CurrentFocusScopeId)
            if (g.NavJustMovedToId == id && (g.NavJustMovedToKeyMods & VanGuiMod_Ctrl) == 0)
                selected = pressed = auto_selected = true;
    }

    // Update NavId when clicking or when Hovering (this doesn't happen on most widgets), so navigation can be resumed with keyboard/gamepad
    if (pressed || (hovered && (flags & VanGuiSelectableFlags_SetNavIdOnHover)))
    {
        if (!g.NavHighlightItemUnderNav && g.NavWindow == window && g.NavLayer == window->DC.NavLayerCurrent)
        {
            SetNavID(id, window->DC.NavLayerCurrent, g.CurrentFocusScopeId, WindowRectAbsToRel(window, bb)); // (bb == NavRect)
            if (g.IO.ConfigNavCursorVisibleAuto)
                g.NavCursorVisible = false;
        }
    }
    if (pressed)
        MarkItemEdited(id);

    if (selected != was_selected)
        g.LastItemData.StatusFlags |= VanGuiItemStatusFlags_ToggledSelection;

    // Render
    if (is_visible)
    {
        const bool highlighted = hovered || (flags & VanGuiSelectableFlags_Highlight);
        if (highlighted || selected)
        {
            // Between 1.91.0 and 1.91.4 we made selected Selectable use an arbitrary lerp between _Header and _HeaderHovered. Removed that now. (#8106)
            VanU32 col = GetColorU32((held && highlighted) ? VanGuiCol_HeaderActive : highlighted ? VanGuiCol_HeaderHovered : VanGuiCol_Header);
            RenderFrame(bb.Min, bb.Max, col, false, 0.0f);
        }
        if (g.NavId == id)
        {
            VanGuiNavRenderCursorFlags nav_render_cursor_flags = VanGuiNavRenderCursorFlags_Compact | VanGuiNavRenderCursorFlags_NoRounding;
            if (is_multi_select)
                nav_render_cursor_flags |= VanGuiNavRenderCursorFlags_AlwaysDraw; // Always show the nav rectangle
            RenderNavCursor(bb, id, nav_render_cursor_flags);
        }
    }

    if (span_all_columns)
    {
        if (g.CurrentTable)
            TablePopBackgroundChannel();
        else if (window->DC.CurrentColumns)
            PopColumnsBackground();
    }

    // Text stays at the submission position. Alignment/clipping extents ignore SpanAllColumns.
    if (is_visible)
        RenderTextClipped(pos, VanVec2(VanMin(pos.x + size.x, window->WorkRect.Max.x), pos.y + size.y), label, label_end, &label_size, style.SelectableTextAlign, &bb);

#ifdef VANGUI_DEBUG_BOXSELECT
    if (g.BoxSelectState.UnclipMode) { GetForegroundDrawList()->AddText(pos, VAN_COL32(255,255,0,200), label, label_end); }
#endif

    // Automatically close popups
    if (pressed && !auto_selected && (window->Flags & VanGuiWindowFlags_Popup) && !(flags & VanGuiSelectableFlags_NoAutoClosePopups) && (g.LastItemData.ItemFlags & VanGuiItemFlags_AutoClosePopups))
        CloseCurrentPopup();

    if (disabled_item && !disabled_global)
        EndDisabled();

    // Selectable() always returns a pressed state!
    // Users of BeginMultiSelect()/EndMultiSelect() scope: you may call VanGui::IsItemToggledSelection() to retrieve
    // selection toggle, only useful if you need that state updated (e.g. for rendering purpose) before reaching EndMultiSelect().
    VANGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags);
    return pressed; //-V1020
}

bool VanGui::Selectable(const char* label, bool* p_selected, VanGuiSelectableFlags flags, const VanVec2& size_arg)
{
    if (Selectable(label, *p_selected, flags, size_arg))
    {
        *p_selected = !*p_selected;
        return true;
    }
    return false;
}


//-------------------------------------------------------------------------
// [SECTION] Widgets: Typing-Select support
//-------------------------------------------------------------------------

// [Experimental] Currently not exposed in public API.
// Consume character inputs and return search request, if any.
// This would typically only be called on the focused window or location you want to grab inputs for, e.g.
//   if (VanGui::IsWindowFocused(...))
//       if (VanGuiTypingSelectRequest* req = VanGui::GetTypingSelectRequest())
//           focus_idx = VanGui::TypingSelectFindMatch(req, my_items.size(), [](void*, int n) { return my_items[n]->Name; }, &my_items, -1);
// However the code is written in a way where calling it from multiple locations is safe (e.g. to obtain buffer).
VanGuiTypingSelectRequest* VanGui::GetTypingSelectRequest(VanGuiTypingSelectFlags flags)
{
    VanGuiContext& g = *GVanGui;
    VanGuiTypingSelectState* data = &g.TypingSelectState;
    VanGuiTypingSelectRequest* out_request = &data->Request;

    // Clear buffer
    const float TYPING_SELECT_RESET_TIMER = 1.80f;          // FIXME: Potentially move to IO config.
    const int TYPING_SELECT_SINGLE_CHAR_COUNT_FOR_LOCK = 4; // Lock single char matching when repeating same char 4 times
    if (data->SearchBuffer[0] != 0)
    {
        bool clear_buffer = false;
        clear_buffer |= (g.NavFocusScopeId != data->FocusScope);
        clear_buffer |= (data->LastRequestTime + TYPING_SELECT_RESET_TIMER < g.Time);
        clear_buffer |= g.NavAnyRequest;
        clear_buffer |= g.ActiveId != 0 && g.NavActivateId == 0; // Allow temporary SPACE activation to not interfere
        clear_buffer |= IsKeyPressed(VanGuiKey_Escape) || IsKeyPressed(VanGuiKey_Enter);
        clear_buffer |= IsKeyPressed(VanGuiKey_Backspace) && (flags & VanGuiTypingSelectFlags_AllowBackspace) == 0;
        //if (clear_buffer) { VANGUI_DEBUG_LOG("GetTypingSelectRequest(): Clear SearchBuffer.\n"); }
        if (clear_buffer)
            data->Clear();
    }

    // Append to buffer
    const int buffer_max_len = VAN_COUNTOF(data->SearchBuffer) - 1;
    int buffer_len = static_cast<int>(VanStrlen(data->SearchBuffer));
    bool select_request = false;
    for (VanWchar w : g.IO.InputQueueCharacters)
    {
        const int w_len = VanTextCountUtf8BytesFromStr(&w, &w + 1);
        if (w < 32 || (buffer_len == 0 && VanCharIsBlankW(w)) || (buffer_len + w_len > buffer_max_len)) // Ignore leading blanks
            continue;
        char w_buf[5];
        VanTextCharToUtf8(w_buf, static_cast<unsigned int>(w));
        if (data->SingleCharModeLock && w_len == out_request->SingleCharSize && memcmp(w_buf, data->SearchBuffer, w_len) == 0)
        {
            select_request = true; // Same character: don't need to append to buffer.
            continue;
        }
        if (data->SingleCharModeLock)
        {
            data->Clear(); // Different character: clear
            buffer_len = 0;
        }
        memcpy(data->SearchBuffer + buffer_len, w_buf, w_len + 1); // Append
        buffer_len += w_len;
        select_request = true;
    }
    g.IO.InputQueueCharacters.resize(0);

    // Handle backspace
    if ((flags & VanGuiTypingSelectFlags_AllowBackspace) && IsKeyPressed(VanGuiKey_Backspace, VanGuiInputFlags_Repeat))
    {
        char* p = const_cast<char*>(VanTextFindPreviousUtf8Codepoint(data->SearchBuffer, data->SearchBuffer + buffer_len));
        *p = 0;
        buffer_len = static_cast<int>(p - data->SearchBuffer);
    }

    // Return request if any
    if (buffer_len == 0)
        return nullptr;
    if (select_request)
    {
        data->FocusScope = g.NavFocusScopeId;
        data->LastRequestFrame = g.FrameCount;
        data->LastRequestTime = static_cast<float>(g.Time);
    }
    out_request->Flags = flags;
    out_request->SearchBufferLen = buffer_len;
    out_request->SearchBuffer = data->SearchBuffer;
    out_request->SelectRequest = (data->LastRequestFrame == g.FrameCount);
    out_request->SingleCharMode = false;
    out_request->SingleCharSize = 0;

    // Calculate if buffer contains the same character repeated.
    // - This can be used to implement a special search mode on first character.
    // - Performed on UTF-8 codepoint for correctness.
    // - SingleCharMode is always set for first input character, because it usually leads to a "next".
    if (flags & VanGuiTypingSelectFlags_AllowSingleCharMode)
    {
        const char* buf_begin = out_request->SearchBuffer;
        const char* buf_end = out_request->SearchBuffer + out_request->SearchBufferLen;
        const int c0_len = VanTextCountUtf8BytesFromChar(buf_begin, buf_end);
        const char* p = buf_begin + c0_len;
        for (; p < buf_end; p += c0_len)
            if (memcmp(buf_begin, p, static_cast<size_t>(c0_len)) != 0)
                break;
        const int single_char_count = (p == buf_end) ? (out_request->SearchBufferLen / c0_len) : 0;
        out_request->SingleCharMode = (single_char_count > 0 || data->SingleCharModeLock);
        out_request->SingleCharSize = static_cast<VanS8>(c0_len);
        data->SingleCharModeLock |= (single_char_count >= TYPING_SELECT_SINGLE_CHAR_COUNT_FOR_LOCK); // From now on we stop search matching to lock to single char mode.
    }

    return out_request;
}

static int VanStrimatchlen(const char* s1, const char* s1_end, const char* s2)
{
    int match_len = 0;
    while (s1 < s1_end && VanToUpper(*s1++) == VanToUpper(*s2++))
        match_len++;
    return match_len;
}

// Default handler for finding a result for typing-select. You may implement your own.
// You might want to display a tooltip to visualize the current request SearchBuffer
// When SingleCharMode is set:
// - it is better to NOT display a tooltip of other on-screen display indicator.
// - the index of the currently focused item is required.
//   if your SetNextItemSelectionUserData() values are indices, you can obtain it from VanGuiMultiSelectIO::NavIdItem, otherwise from g.NavLastValidSelectionUserData.
int VanGui::TypingSelectFindMatch(VanGuiTypingSelectRequest* req, int items_count, const char* (*get_item_name_func)(void*, int), void* user_data, int nav_item_idx)
{
    if (req == nullptr || req->SelectRequest == false) // Support nullptr parameter so both calls can be done from same spot.
        return -1;
    int idx = -1;
    if (req->SingleCharMode && (req->Flags & VanGuiTypingSelectFlags_AllowSingleCharMode))
        idx = TypingSelectFindNextSingleCharMatch(req, items_count, get_item_name_func, user_data, nav_item_idx);
    else
        idx = TypingSelectFindBestLeadingMatch(req, items_count, get_item_name_func, user_data);
    if (idx != -1)
        SetNavCursorVisibleAfterMove();
    return idx;
}

// Special handling when a single character is repeated: perform search on a single letter and goes to next.
int VanGui::TypingSelectFindNextSingleCharMatch(VanGuiTypingSelectRequest* req, int items_count, const char* (*get_item_name_func)(void*, int), void* user_data, int nav_item_idx)
{
    // FIXME: Assume selection user data is index. Would be extremely practical.
    //if (nav_item_idx == -1)
    //    nav_item_idx = (int)g.NavLastValidSelectionUserData;

    int first_match_idx = -1;
    bool return_next_match = false;
    for (int idx = 0; idx < items_count; idx++)
    {
        const char* item_name = get_item_name_func(user_data, idx);
        if (VanStrimatchlen(req->SearchBuffer, req->SearchBuffer + req->SingleCharSize, item_name) < req->SingleCharSize)
            continue;
        if (return_next_match)                           // Return next matching item after current item.
            return idx;
        if (first_match_idx == -1 && nav_item_idx == -1) // Return first match immediately if we don't have a nav_item_idx value.
            return idx;
        if (first_match_idx == -1)                       // Record first match for wrapping.
            first_match_idx = idx;
        if (nav_item_idx == idx)                         // Record that we encountering nav_item so we can return next match.
            return_next_match = true;
    }
    return first_match_idx; // First result
}

int VanGui::TypingSelectFindBestLeadingMatch(VanGuiTypingSelectRequest* req, int items_count, const char* (*get_item_name_func)(void*, int), void* user_data)
{
    int longest_match_idx = -1;
    int longest_match_len = 0;
    for (int idx = 0; idx < items_count; idx++)
    {
        const char* item_name = get_item_name_func(user_data, idx);
        const int match_len = VanStrimatchlen(req->SearchBuffer, req->SearchBuffer + req->SearchBufferLen, item_name);
        if (match_len <= longest_match_len)
            continue;
        longest_match_idx = idx;
        longest_match_len = match_len;
        if (match_len == req->SearchBufferLen)
            break;
    }
    return longest_match_idx;
}

void VanGui::DebugNodeTypingSelectState(VanGuiTypingSelectState* data)
{
#ifndef VANGUI_DISABLE_DEBUG_TOOLS
    Text("SearchBuffer = \"%s\"", data->SearchBuffer);
    Text("SingleCharMode = %d, Size = %d, Lock = %d", data->Request.SingleCharMode, data->Request.SingleCharSize, data->SingleCharModeLock);
    Text("LastRequest = time: %.2f, frame: %d", data->LastRequestTime, data->LastRequestFrame);
#else
    VAN_UNUSED(data);
#endif
}

//-------------------------------------------------------------------------
// [SECTION] Widgets: Box-Select support
// This has been extracted away from Multi-Select logic in the hope that it could eventually be used elsewhere, but hasn't been yet.
//-------------------------------------------------------------------------
// Extra logic in MultiSelectItemFooter() and VanGuiListClipper::Step()
//-------------------------------------------------------------------------
// - BoxSelectPreStartDrag() [Internal]
// - BoxSelectActivateDrag() [Internal]
// - BoxSelectDeactivateDrag() [Internal]
// - BoxSelectScrollWithMouseDrag() [Internal]
// - BeginBoxSelect() [Internal]
// - EndBoxSelect() [Internal]
//-------------------------------------------------------------------------

// Call on the initial click.
static void BoxSelectPreStartDrag(VanGuiID id, VanGuiSelectionUserData clicked_item)
{
    VanGuiContext& g = *GVanGui;
    VanGuiBoxSelectState* bs = &g.BoxSelectState;
    bs->ID = id;
    bs->IsStarting = true; // Consider starting box-select.
    bs->IsStartedFromVoid = (clicked_item == VanGuiSelectionUserData_Invalid);
    bs->IsStartedSetNavIdOnce = bs->IsStartedFromVoid;
    bs->KeyMods = g.IO.KeyMods;
    bs->StartPosRel = bs->EndPosRel = VanGui::WindowPosAbsToRel(g.CurrentWindow, g.IO.MousePos);
    bs->ScrollAccum = VanVec2(0.0f, 0.0f);
}

static void BoxSelectActivateDrag(VanGuiBoxSelectState* bs, VanGuiWindow* window)
{
    VanGuiContext& g = *GVanGui;
    VANGUI_DEBUG_LOG_SELECTION("[selection] BeginBoxSelect() 0X%08X: Activate\n", bs->ID);
    bs->IsActive = true;
    bs->Window = window;
    bs->IsStarting = false;
    VanGui::SetActiveID(bs->ID, window);
    VanGui::SetActiveIdUsingAllKeyboardKeys();
    if (bs->IsStartedFromVoid && (bs->KeyMods & (VanGuiMod_Ctrl | VanGuiMod_Shift)) == 0)
        bs->RequestClear = true;
}

static void BoxSelectDeactivateDrag(VanGuiBoxSelectState* bs)
{
    VanGuiContext& g = *GVanGui;
    bs->IsActive = bs->IsStarting = false;
    if (g.ActiveId == bs->ID)
    {
        VANGUI_DEBUG_LOG_SELECTION("[selection] BeginBoxSelect() 0X%08X: Deactivate\n", bs->ID);
        VanGui::ClearActiveID();
    }
    bs->ID = 0;
}

static void BoxSelectScrollWithMouseDrag(VanGuiBoxSelectState* bs, VanGuiWindow* window, const VanRect& inner_r)
{
    VanGuiContext& g = *GVanGui;
    VAN_ASSERT(bs->Window == window);
    for (int n = 0; n < 2; n++) // each axis
    {
        const float mouse_pos = g.IO.MousePos[n];
        const float dist = (mouse_pos > inner_r.Max[n]) ? mouse_pos - inner_r.Max[n] : (mouse_pos < inner_r.Min[n]) ? mouse_pos - inner_r.Min[n] : 0.0f;
        const float scroll_curr = window->Scroll[n];
        if (dist == 0.0f || (dist < 0.0f && scroll_curr < 0.0f) || (dist > 0.0f && scroll_curr >= window->ScrollMax[n]))
            continue;

        const float speed_multiplier = VanLinearRemapClamp(g.FontSize, g.FontSize * 5.0f, 1.0f, 4.0f, VanAbs(dist)); // x1 to x4 depending on distance
        const float scroll_step = g.FontSize * 35.0f * speed_multiplier * VanSign(dist) * g.IO.DeltaTime;
        bs->ScrollAccum[n] += scroll_step;

        // Accumulate into a stored value so we can handle high-framerate
        const float scroll_step_i = VanFloor(bs->ScrollAccum[n]);
        if (scroll_step_i == 0.0f)
            continue;
        if (n == 0)
            VanGui::SetScrollX(window, scroll_curr + scroll_step_i);
        else
            VanGui::SetScrollY(window, scroll_curr + scroll_step_i);
        bs->ScrollAccum[n] -= scroll_step_i;
    }
}

bool VanGui::BeginBoxSelect(const VanRect& scope_rect, VanGuiWindow* window, VanGuiID box_select_id, VanGuiMultiSelectFlags ms_flags)
{
    VanGuiContext& g = *GVanGui;
    VanGuiBoxSelectState* bs = &g.BoxSelectState;
    KeepAliveID(box_select_id);
    if (bs->ID != box_select_id)
        return false;

    // IsStarting is set by MultiSelectItemFooter() when considering a possible box-select. We validate it here and lock geometry.
    bs->UnclipMode = false;
    bs->RequestClear = false;
    if (bs->IsStarting && IsMouseDragPastThreshold(0))
        BoxSelectActivateDrag(bs, window);
    else if ((bs->IsStarting || bs->IsActive) && g.IO.MouseDown[0] == false)
        BoxSelectDeactivateDrag(bs);
    if (!bs->IsActive)
        return false;

    // Current frame absolute prev/current rectangles are used to toggle selection.
    // They are derived from positions relative to scrolling space, so "previous" rectangle is reprojected for current frame coordinates.
    VanVec2 start_pos_abs = WindowPosRelToAbs(window, bs->StartPosRel);
    VanVec2 prev_end_pos_abs = WindowPosRelToAbs(window, bs->EndPosRel); // Clamped already
    VanVec2 curr_end_pos_abs = g.IO.MousePos;
    if (ms_flags & VanGuiMultiSelectFlags_ScopeWindow) // Box-select scrolling only happens with ScopeWindow
        curr_end_pos_abs = VanClamp(curr_end_pos_abs, scope_rect.Min, scope_rect.Max);
    bs->BoxSelectRectPrev.Min = VanMin(start_pos_abs, prev_end_pos_abs);
    bs->BoxSelectRectPrev.Max = VanMax(start_pos_abs, prev_end_pos_abs);
    bs->BoxSelectRectCurr.Min = VanMin(start_pos_abs, curr_end_pos_abs);
    bs->BoxSelectRectCurr.Max = VanMax(start_pos_abs, curr_end_pos_abs);
    //VANGUI_DEBUG_LOG("StartPosRel (%.2f,%.2f) EndPosRel (%.2f,%.2f) -> (%.2f,%.2f)\n", bs->StartPosRel.x, bs->StartPosRel.y, bs->EndPosRel.x, bs->EndPosRel.y, WindowPosAbsToRel(window, g.IO.MousePos).x, WindowPosAbsToRel(window, g.IO.MousePos).y);

    // Box-select 2D mode detects change of the rectangle.
    // Storing unclip rects which will be tested by widgets supporting box-select. Always update rectangles when active (even if we don't use them).
    // To facilitate understanding this: enable VANGUI_DEBUG_BOXSELECT and visualize all geometry.
    if (ms_flags & (VanGuiMultiSelectFlags_BoxSelect1d | VanGuiMultiSelectFlags_BoxSelect2d))
    {
        // For both sides, compute the area differing between Prev and Curr rectangles.
        bs->UnclipRects[0] = bs->UnclipRects[1] = VanRect(+FLT_MAX, +FLT_MAX, -FLT_MAX, -FLT_MAX);
        for (int side = 0; side < 2; side++)
        {
            VanVec2 d_min = (side == 0) ? VanMin(bs->BoxSelectRectCurr.Min, bs->BoxSelectRectPrev.Min) : VanMin(bs->BoxSelectRectCurr.Max, bs->BoxSelectRectPrev.Max);
            VanVec2 d_max = (side == 0) ? VanMax(bs->BoxSelectRectCurr.Min, bs->BoxSelectRectPrev.Min) : VanMax(bs->BoxSelectRectCurr.Max, bs->BoxSelectRectPrev.Max);
            if (d_min.x != d_max.x)
            {
                bs->UnclipRects[0].AddX(d_min.x);
                bs->UnclipRects[0].AddX(d_max.x);
            }
            if (d_min.y != d_max.y)
            {
                bs->UnclipRects[1].AddY(d_min.y);
                bs->UnclipRects[1].AddY(d_max.y);
            }
        }

        VanRect box_select_intersection = bs->BoxSelectRectPrev;
        box_select_intersection.Add(bs->BoxSelectRectCurr);
        if (ms_flags & VanGuiMultiSelectFlags_BoxSelect2d)
            if (bs->BoxSelectRectPrev.Min.x != bs->BoxSelectRectCurr.Min.x || bs->BoxSelectRectPrev.Max.x != bs->BoxSelectRectCurr.Max.x)
            {
                bs->UnclipRects[0].AddY(box_select_intersection.Min.y);
                bs->UnclipRects[0].AddY(box_select_intersection.Max.y);
            }
        if (ms_flags & (VanGuiMultiSelectFlags_BoxSelect1d | VanGuiMultiSelectFlags_BoxSelect2d))
            if (bs->BoxSelectRectPrev.Min.y != bs->BoxSelectRectCurr.Min.y || bs->BoxSelectRectPrev.Max.y != bs->BoxSelectRectCurr.Max.y)
            {
                bs->UnclipRects[1].AddX(box_select_intersection.Min.x);
                bs->UnclipRects[1].AddX(box_select_intersection.Max.x);
            }

        // Merge both rectangles into one.
        // FIXME-OPT: When UnclipRect.Area() is much larger than the sum of UnclipRects[0]/[1] Areas, widgets should
        // ideally first use UnclipRect as a first coarse cull layer + the individual ones as a second validation.
        bs->UnclipRect = bs->UnclipRects[0];
        bs->UnclipRect.Add(bs->UnclipRects[1]);
        if (!bs->UnclipRect.IsInverted() && (!window->ClipRect.Contains(bs->UnclipRect.Min) || !window->ClipRect.Contains(bs->UnclipRect.Max))) // !! Don't use Contains(VanRect)
            bs->UnclipMode = true;
        if (bs->UnclipMode && g.CurrentTable != nullptr)
            TableApplyExternalUnclipRect(g.CurrentTable, bs->UnclipRect); // No need submitting both
    }

#ifdef VANGUI_DEBUG_BOXSELECT
    //GetForegroundDrawList()->AddRect(scope_rect.Min, scope_rect.Max, VAN_COL32(0, 255, 0, 200), 0.0f, 0, 4.0f);
    //GetForegroundDrawList()->AddRect(bs->BoxSelectRectPrev.Min, bs->BoxSelectRectPrev.Max, VAN_COL32(255,0,0,200), 0.0f, 0, 3.0f);
    //GetForegroundDrawList()->AddRect(bs->BoxSelectRectCurr.Min, bs->BoxSelectRectCurr.Max, VAN_COL32(0,255,0,200), 0.0f, 0, 1.0f);
    if (ms_flags & (VanGuiMultiSelectFlags_BoxSelect1d | VanGuiMultiSelectFlags_BoxSelect2d))
    {
        for (VanRect& unclip_r : bs->UnclipRects)
            if (!unclip_r.IsInverted())
                GetForegroundDrawList()->AddRect(unclip_r.Min, unclip_r.Max, bs->UnclipMode ? VAN_COL32(255, 255, 0, 200) : VAN_COL32(255, 0, 0, 200), 0.0f, 0, 4.0f);
        GetForegroundDrawList()->AddRect(bs->UnclipRect.Min, bs->UnclipRect.Max, bs->UnclipMode ? VAN_COL32(255, 255, 0, 200) : VAN_COL32(255, 0, 0, 200), 0.0f, 0, 2.0f);
    }
#endif
    return true;
}

void VanGui::EndBoxSelect(const VanRect& scope_rect, VanGuiMultiSelectFlags ms_flags)
{
    VanGuiContext& g = *GVanGui;
    VanGuiWindow* window = g.CurrentWindow;
    VanGuiBoxSelectState* bs = &g.BoxSelectState;
    VAN_ASSERT(bs->IsActive);
    bs->UnclipMode = false;

    // Render selection rectangle
    bs->EndPosRel = WindowPosAbsToRel(window, VanClamp(g.IO.MousePos, scope_rect.Min, scope_rect.Max)); // Clamp stored position according to current scrolling view
    VanRect box_select_r = bs->BoxSelectRectCurr;
    box_select_r.ClipWith(scope_rect);
    VanGuiWindow* draw_window = FindFrontMostVisibleChildWindow(window);
    draw_window->DrawList->AddRectFilled(box_select_r.Min, box_select_r.Max, GetColorU32(VanGuiCol_SeparatorHovered, 0.30f)); // FIXME-MULTISELECT: Styling
    draw_window->DrawList->AddRect(box_select_r.Min, box_select_r.Max, GetColorU32(VanGuiCol_NavCursor)); // FIXME-MULTISELECT FIXME-DPI: Styling

    // Scroll
    const bool enable_scroll = (ms_flags & VanGuiMultiSelectFlags_ScopeWindow) && (ms_flags & VanGuiMultiSelectFlags_BoxSelectNoScroll) == 0;
    if (enable_scroll)
    {
        VanRect scroll_r = scope_rect;
        scroll_r.Expand(-g.FontSize);
        //GetForegroundDrawList()->AddRect(scroll_r.Min, scroll_r.Max, VAN_COL32(0, 255, 0, 255));
        if (!scroll_r.Contains(g.IO.MousePos))
            BoxSelectScrollWithMouseDrag(bs, window, scroll_r);
    }
}

//-------------------------------------------------------------------------
// [SECTION] Widgets: Multi-Select support
//-------------------------------------------------------------------------
// - DebugLogMultiSelectRequests() [Internal]
// - CalcScopeRect() [Internal]
// - BeginMultiSelect()
// - EndMultiSelect()
// - SetNextItemSelectionUserData()
// - MultiSelectItemHeader() [Internal]
// - MultiSelectItemFooter() [Internal]
// - DebugNodeMultiSelectState() [Internal]
//-------------------------------------------------------------------------

static void DebugLogMultiSelectRequests(const char* function, const VanGuiMultiSelectIO* io)
{
    VanGuiContext& g = *GVanGui;
    VAN_UNUSED(function);
    for (const VanGuiSelectionRequest& req : io->Requests)
    {
        if (req.Type == VanGuiSelectionRequestType_SetAll)    VANGUI_DEBUG_LOG_SELECTION("[selection] %s: Request: SetAll %d (= %s)\n", function, req.Selected, req.Selected ? "SelectAll" : "Clear");
        if (req.Type == VanGuiSelectionRequestType_SetRange)  VANGUI_DEBUG_LOG_SELECTION("[selection] %s: Request: SetRange %" VAN_PRId64 "..%" VAN_PRId64 " (0x%" VAN_PRIX64 "..0x%" VAN_PRIX64 ") = %d (dir %d)\n", function, req.RangeFirstItem, req.RangeLastItem, req.RangeFirstItem, req.RangeLastItem, req.Selected, req.RangeDirection);
    }
}

static VanRect CalcScopeRect(VanGuiMultiSelectTempData* ms, VanGuiWindow* window)
{
    if (ms->Flags & VanGuiMultiSelectFlags_ScopeRect)
    {
        // Warning: this depends on CursorMaxPos so it means to be called by EndMultiSelect() only
        // This probably doesn't work inside a table as there are ample ambiguities related to exact time of calling BeginMultiSelect()/EndMultiSelect().
        return VanRect(ms->ScopeRectMin, VanMax(window->DC.CursorMaxPos, ms->ScopeRectMin));
    }
    else
    {
        //// When a table, pull HostClipRect, which allows us to predict ClipRect before first row/layout is performed. (#7970)
        VanRect scope_rect = window->InnerClipRect;
        //if (g.CurrentTable != nullptr)
        //    scope_rect = g.CurrentTable->HostClipRect;

        // Add inner table decoration (#7821) // FIXME: Why not baking in InnerClipRect?
        scope_rect.Min = VanMin(scope_rect.Min + VanVec2(window->DecoInnerSizeX1, window->DecoInnerSizeY1), scope_rect.Max);
        return scope_rect;
    }
}

// Return VanGuiMultiSelectIO structure.
// Lifetime: don't hold on VanGuiMultiSelectIO* pointers over multiple frames or past any subsequent call to BeginMultiSelect() or EndMultiSelect().
// Passing 'selection_size' and 'items_count' parameters is currently optional.
// - 'selection_size' is useful to disable some shortcut routing: e.g. VanGuiMultiSelectFlags_ClearOnEscape won't claim Escape key when selection_size 0,
//    allowing a first press to clear selection THEN the second press to leave child window and return to parent.
// - 'items_count' is stored in VanGuiMultiSelectIO which makes it a convenient way to pass the information to your ApplyRequest() handler (but you may pass it differently).
// - If they are costly for you to compute (e.g. external intrusive selection without maintaining size), you may avoid them and pass -1.
//   - If you can easily tell if your selection is empty or not, you may pass 0/1, or you may enable VanGuiMultiSelectFlags_ClearOnEscape flag dynamically.
VanGuiMultiSelectIO* VanGui::BeginMultiSelect(VanGuiMultiSelectFlags flags, int selection_size, int items_count)
{
    VanGuiContext& g = *GVanGui;
    VanGuiWindow* window = g.CurrentWindow;

    if (++g.MultiSelectTempDataStacked > g.MultiSelectTempData.Size)
        g.MultiSelectTempData.resize(g.MultiSelectTempDataStacked, VanGuiMultiSelectTempData());
    VanGuiMultiSelectTempData* ms = &g.MultiSelectTempData[g.MultiSelectTempDataStacked - 1];
    VAN_STATIC_ASSERT(offsetof(VanGuiMultiSelectTempData, IO) == 0); // Clear() relies on that.
    g.CurrentMultiSelect = ms;
    if ((flags & (VanGuiMultiSelectFlags_ScopeWindow | VanGuiMultiSelectFlags_ScopeRect)) == 0)
        flags |= VanGuiMultiSelectFlags_ScopeWindow;
    if (flags & VanGuiMultiSelectFlags_SingleSelect)
        flags &= ~(VanGuiMultiSelectFlags_BoxSelect2d | VanGuiMultiSelectFlags_BoxSelect1d);
    if (flags & VanGuiMultiSelectFlags_BoxSelect2d)
        flags &= ~VanGuiMultiSelectFlags_BoxSelect1d;

    // FIXME: Workaround to the fact we override CursorMaxPos, meaning size measurement are lost. (#8250)
    // They should perhaps be stacked properly?
    if (VanGuiTable* table = g.CurrentTable)
    {
        if (!table->IsLayoutLocked)
            TableUpdateLayout(table);
        else if (table->CurrentColumn != -1)
            TableEndCell(table); // This is currently safe to call multiple time. If that properly is lost we can extract the "save measurement" part of it.
    }

    // FIXME: BeginFocusScope()
    const VanGuiID id = window->IDStack.back();
    ms->Clear();
    ms->FocusScopeId = id;
    ms->Flags = flags;
    ms->BackupCursorMaxPos = window->DC.CursorMaxPos;
    ms->ScopeRectMin = window->DC.CursorPos;
    if (flags & VanGuiMultiSelectFlags_ScopeRect)
        window->DC.CursorMaxPos = ms->ScopeRectMin; // CalcScopeRect() for VanGuiMultiSelectFlags_ScopeRect will measure in EndMultiSelect().
    PushFocusScope(ms->FocusScopeId);
    ms->IsFocused = IsInNavFocusRoute(g.CurrentFocusScopeId);
    if (flags & VanGuiMultiSelectFlags_ScopeWindow) // Mark parent child window as navigable into, with highlight. Assume user will always submit interactive items.
        window->DC.NavLayersActiveMask |= 1 << VanGuiNavLayer_Main;

    // Use copy of keyboard mods at the time of the request, otherwise we would requires mods to be held for an extra frame.
    ms->KeyMods = g.NavJustMovedToId ? (g.NavJustMovedToIsTabbing ? 0 : g.NavJustMovedToKeyMods) : g.IO.KeyMods;
    if (flags & VanGuiMultiSelectFlags_NoRangeSelect)
        ms->KeyMods &= ~VanGuiMod_Shift;

    // Bind storage
    VanGuiMultiSelectState* storage = g.MultiSelectStorage.GetOrAddByKey(id);
    storage->ID = id;
    storage->LastFrameActive = g.FrameCount;
    storage->LastSelectionSize = selection_size;
    storage->Window = window;
    ms->Storage = storage;

    // Output to user
    ms->IO.Requests.resize(0);
    ms->IO.RangeSrcItem = storage->RangeSrcItem;
    ms->IO.NavIdItem = storage->NavIdItem;
    ms->IO.NavIdSelected = (storage->NavIdSelected == 1) ? true : false;
    ms->IO.ItemsCount = items_count;

    // Clear when using Navigation to move within the scope
    // (we compare FocusScopeId so it possible to use multiple selections inside a same window)
    bool request_clear = false;
    bool request_select_all = false;
    if (g.NavJustMovedToId != 0 && g.NavJustMovedToFocusScopeId == ms->FocusScopeId && g.NavJustMovedToHasSelectionData)
    {
        if (ms->KeyMods & VanGuiMod_Shift)
            ms->IsKeyboardSetRange = true;
        if (ms->IsKeyboardSetRange)
            VAN_ASSERT(storage->RangeSrcItem != VanGuiSelectionUserData_Invalid); // Not ready -> could clear?
        if ((ms->KeyMods & (VanGuiMod_Ctrl | VanGuiMod_Shift)) == 0 && (flags & (VanGuiMultiSelectFlags_NoAutoClear | VanGuiMultiSelectFlags_NoAutoSelect)) == 0)
            request_clear = true;
    }
    else if (g.NavJustMovedFromFocusScopeId == ms->FocusScopeId)
    {
        // Also clear on leaving scope (may be optional?)
        if ((ms->KeyMods & (VanGuiMod_Ctrl | VanGuiMod_Shift)) == 0 && (flags & (VanGuiMultiSelectFlags_NoAutoClear | VanGuiMultiSelectFlags_NoAutoSelect)) == 0)
            request_clear = true;
    }

    // Box-select handling: update active state.
    VanGuiBoxSelectState* bs = &g.BoxSelectState;
    if (flags & (VanGuiMultiSelectFlags_BoxSelect1d | VanGuiMultiSelectFlags_BoxSelect2d))
    {
        ms->BoxSelectId = GetID("##BoxSelect");
        if (BeginBoxSelect(CalcScopeRect(ms, window), window, ms->BoxSelectId, flags))
            request_clear |= bs->RequestClear;
    }

    if (ms->IsFocused)
    {
        // Shortcut: Clear selection (Escape)
        // - Only claim shortcut if selection is not empty, allowing further presses on Escape to e.g. leave current child window.
        // - Box select also handle Escape and needs to pass an id to bypass ActiveIdUsingAllKeyboardKeys lock.
        if (flags & VanGuiMultiSelectFlags_ClearOnEscape)
        {
            if (selection_size != 0 || bs->IsActive)
                if (Shortcut(VanGuiKey_Escape, VanGuiInputFlags_None, bs->IsActive ? bs->ID : 0))
                {
                    request_clear = true;
                    if (bs->IsActive)
                        BoxSelectDeactivateDrag(bs);
                }
        }

        // Shortcut: Select all (Ctrl+A)
        if (!(flags & VanGuiMultiSelectFlags_SingleSelect) && !(flags & VanGuiMultiSelectFlags_NoSelectAll))
            if (Shortcut(VanGuiMod_Ctrl | VanGuiKey_A))
                request_select_all = true;
    }

    if (request_clear || request_select_all)
    {
        MultiSelectAddSetAll(ms, request_select_all);
        if (!request_select_all)
            storage->LastSelectionSize = 0;
    }
    ms->LoopRequestSetAll = request_select_all ? 1 : request_clear ? 0 : -1;
    //ms->PrevSubmittedItem = VanGuiSelectionUserData_Invalid;

    if (g.DebugLogFlags & VanGuiDebugLogFlags_EventSelection)
        DebugLogMultiSelectRequests("BeginMultiSelect", &ms->IO);

    return &ms->IO;
}

// Return updated VanGuiMultiSelectIO structure.
// Lifetime: don't hold on VanGuiMultiSelectIO* pointers over multiple frames or past any subsequent call to BeginMultiSelect() or EndMultiSelect().
VanGuiMultiSelectIO* VanGui::EndMultiSelect()
{
    VanGuiContext& g = *GVanGui;
    VanGuiMultiSelectTempData* ms = g.CurrentMultiSelect;
    VanGuiMultiSelectState* storage = ms->Storage;
    VanGuiWindow* window = g.CurrentWindow;
    VAN_ASSERT_USER_ERROR(ms->FocusScopeId == g.CurrentFocusScopeId, "EndMultiSelect() FocusScope mismatch!");
    VAN_ASSERT(g.CurrentMultiSelect != nullptr && storage->Window == g.CurrentWindow);
    VAN_ASSERT(g.MultiSelectTempDataStacked > 0 && &g.MultiSelectTempData[g.MultiSelectTempDataStacked - 1] == g.CurrentMultiSelect);

    VanRect scope_rect = CalcScopeRect(ms, window);
    if (ms->IsFocused)
    {
        // We currently don't allow user code to modify RangeSrcItem by writing to BeginIO's version, but that would be an easy change here.
        if (ms->IO.RangeSrcReset || (ms->RangeSrcPassedBy == false && ms->IO.RangeSrcItem != VanGuiSelectionUserData_Invalid)) // Can't read storage->RangeSrcItem here -> we want the state at beginning of the scope (see tests for easy failure)
        {
            VANGUI_DEBUG_LOG_SELECTION("[selection] EndMultiSelect: Reset RangeSrcItem.\n"); // Will set be to NavId.
            storage->RangeSrcItem = VanGuiSelectionUserData_Invalid;
        }
        if (ms->NavIdPassedBy == false && storage->NavIdItem != VanGuiSelectionUserData_Invalid)
        {
            VANGUI_DEBUG_LOG_SELECTION("[selection] EndMultiSelect: Reset NavIdItem.\n");
            storage->NavIdItem = VanGuiSelectionUserData_Invalid;
            storage->NavIdSelected = -1;
        }

        if ((ms->Flags & (VanGuiMultiSelectFlags_BoxSelect1d | VanGuiMultiSelectFlags_BoxSelect2d)) && GetBoxSelectState(ms->BoxSelectId))
            EndBoxSelect(scope_rect, ms->Flags);
    }

    if (ms->IsEndIO == false)
        ms->IO.Requests.resize(0);

    // Clear selection when clicking void?
    // We specifically test for IsMouseDragPastThreshold(0) == false to allow box-selection!
    // The InnerRect test is necessary for non-child/decorated windows.
    bool scope_hovered = window->InnerRect.Contains(g.IO.MousePos) && IsWindowHovered(VanGuiHoveredFlags_ChildWindows);
    if (scope_hovered && (ms->Flags & VanGuiMultiSelectFlags_ScopeRect))
        scope_hovered &= scope_rect.Contains(g.IO.MousePos);
    if (scope_hovered && g.HoveredId == 0 && g.ActiveId == 0)
    {
        if (ms->Flags & (VanGuiMultiSelectFlags_BoxSelect1d | VanGuiMultiSelectFlags_BoxSelect2d))
        {
            if (!g.BoxSelectState.IsActive && !g.BoxSelectState.IsStarting && g.IO.MouseClickedCount[0] == 1)
            {
                BoxSelectPreStartDrag(ms->BoxSelectId, VanGuiSelectionUserData_Invalid);
                FocusWindow(window, VanGuiFocusRequestFlags_UnlessBelowModal);
                SetHoveredID(ms->BoxSelectId);
                if (ms->Flags & VanGuiMultiSelectFlags_ScopeRect)
                    SetNavID(0, VanGuiNavLayer_Main, ms->FocusScopeId, VanRect(g.IO.MousePos, g.IO.MousePos)); // Automatically switch FocusScope for initial click from void to box-select.
            }
        }

        if (ms->Flags & VanGuiMultiSelectFlags_ClearOnClickVoid)
            if (IsMouseReleased(0) && IsMouseDragPastThreshold(0) == false && g.IO.KeyMods == VanGuiMod_None)
                MultiSelectAddSetAll(ms, false);
    }

    // Courtesy nav wrapping helper flag
    if (ms->Flags & VanGuiMultiSelectFlags_NavWrapX)
    {
        VAN_ASSERT(ms->Flags & VanGuiMultiSelectFlags_ScopeWindow); // Only supported at window scope
        NavMoveRequestTryWrapping(GetCurrentWindow(), VanGuiNavMoveFlags_WrapX);
    }

    // Unwind
    if (VanGuiTable* table = g.CurrentTable)
        if (table->IsInsideRow)
            TableEndRow(table);
    window->DC.CursorMaxPos = VanMax(ms->BackupCursorMaxPos, window->DC.CursorMaxPos);
    PopFocusScope();

    if (g.DebugLogFlags & VanGuiDebugLogFlags_EventSelection)
        DebugLogMultiSelectRequests("EndMultiSelect", &ms->IO);

    ms->FocusScopeId = 0;
    ms->Flags = VanGuiMultiSelectFlags_None;
    g.CurrentMultiSelect = (--g.MultiSelectTempDataStacked > 0) ? &g.MultiSelectTempData[g.MultiSelectTempDataStacked - 1] : nullptr;

    return &ms->IO;
}

void VanGui::SetNextItemSelectionUserData(VanGuiSelectionUserData selection_user_data)
{
    // Note that flags will be cleared by ItemAdd(), so it's only useful for Navigation code!
    // This designed so widgets can also cheaply set this before calling ItemAdd(), so we are not tied to MultiSelect api.
    VanGuiContext& g = *GVanGui;
    g.NextItemData.SelectionUserData = selection_user_data;
    g.NextItemData.FocusScopeId = g.CurrentFocusScopeId;

    if (VanGuiMultiSelectTempData* ms = g.CurrentMultiSelect)
    {
        // Auto updating RangeSrcPassedBy for cases were clipper is not used (done before ItemAdd() clipping)
        g.NextItemData.ItemFlags |= VanGuiItemFlags_HasSelectionUserData | VanGuiItemFlags_IsMultiSelect;
        if (ms->IO.RangeSrcItem == selection_user_data)
            ms->RangeSrcPassedBy = true;
        //ms->PrevSubmittedItem = ms->CurrSubmittedItem; // Can't rely on previous g.NextItemData.SelectionUserData because NextItemData is not restored on nested multi-select.
        //ms->CurrSubmittedItem = selection_user_data;
    }
    else
    {
        g.NextItemData.ItemFlags |= VanGuiItemFlags_HasSelectionUserData;
    }
}

// In charge of:
// - Applying SetAll for submitted items.
// - Applying SetRange for submitted items and record end points.
// - Altering button behavior flags to facilitate use with drag and drop.
void VanGui::MultiSelectItemHeader(VanGuiID id, bool* p_selected, VanGuiButtonFlags* p_button_flags)
{
    VanGuiContext& g = *GVanGui;
    VanGuiMultiSelectTempData* ms = g.CurrentMultiSelect;

    bool selected = *p_selected;
    if (ms->IsFocused)
    {
        VanGuiMultiSelectState* storage = ms->Storage;
        VanGuiSelectionUserData item_data = g.NextItemData.SelectionUserData;
        VAN_ASSERT(g.NextItemData.FocusScopeId == g.CurrentFocusScopeId && "Forgot to call SetNextItemSelectionUserData() prior to item, required in BeginMultiSelect()/EndMultiSelect() scope");

        // Apply SetAll (Clear/SelectAll) requests requested by BeginMultiSelect().
        // This is only useful if the user hasn't processed them already, and this only works if the user isn't using the clipper.
        // If you are using a clipper you need to process the SetAll request after calling BeginMultiSelect()
        if (ms->LoopRequestSetAll != -1)
            selected = (ms->LoopRequestSetAll == 1);

        // When using Shift+Nav: because it can incur scrolling we cannot afford a frame of lag with the selection highlight (otherwise scrolling would happen before selection)
        // For this to work, we need someone to set 'RangeSrcPassedBy = true' at some point (either clipper either SetNextItemSelectionUserData() function)
        if (ms->IsKeyboardSetRange)
        {
            VAN_ASSERT(id != 0 && (ms->KeyMods & VanGuiMod_Shift) != 0);
            const bool is_range_dst = (ms->RangeDstPassedBy == false) && g.NavJustMovedToId == id;     // Assume that g.NavJustMovedToId is not clipped.
            if (is_range_dst)
                ms->RangeDstPassedBy = true;
            if (is_range_dst && storage->RangeSrcItem == VanGuiSelectionUserData_Invalid) // If we don't have RangeSrc, assign RangeSrc = RangeDst
            {
                storage->RangeSrcItem = item_data;
                storage->RangeSelected = selected ? 1 : 0;
            }
            const bool is_range_src = storage->RangeSrcItem == item_data;
            if (is_range_src || is_range_dst || ms->RangeSrcPassedBy != ms->RangeDstPassedBy)
            {
                // Apply range-select value to visible items
                VAN_ASSERT(storage->RangeSrcItem != VanGuiSelectionUserData_Invalid && storage->RangeSelected != -1);
                selected = (storage->RangeSelected != 0);
            }
            else if ((ms->KeyMods & VanGuiMod_Ctrl) == 0 && (ms->Flags & VanGuiMultiSelectFlags_NoAutoClear) == 0)
            {
                // Clear other items
                selected = false;
            }
        }
        *p_selected = selected;
    }

    // Alter button behavior flags
    // To handle drag and drop of multiple items we need to avoid clearing selection on click.
    // Enabling this test makes actions using Ctrl+Shift delay their effect on MouseUp which is annoying, but it allows drag and drop of multiple items.
    if (p_button_flags != nullptr)
    {
        VanGuiButtonFlags button_flags = *p_button_flags;
        button_flags |= VanGuiButtonFlags_NoHoveredOnFocus;
        button_flags &= ~(VanGuiButtonFlags_PressedOnClick | VanGuiButtonFlags_PressedOnClickRelease);
        if (ms->Flags & VanGuiMultiSelectFlags_SelectOnClickAlways)
            button_flags |= VanGuiButtonFlags_PressedOnClick;
        else if (ms->Flags & VanGuiMultiSelectFlags_SelectOnClickRelease)
            button_flags |= VanGuiButtonFlags_PressedOnClickRelease;
        else // VanGuiMultiSelectFlags_SelectOnAuto
            button_flags |= (!selected || (g.ActiveId == id && g.ActiveIdHasBeenPressedBefore)) ? VanGuiButtonFlags_PressedOnClick : VanGuiButtonFlags_PressedOnClickRelease;
        *p_button_flags = button_flags;
    }
}

// In charge of:
// - Auto-select on navigation.
// - Box-select toggle handling.
// - Right-click handling.
// - Altering selection based on Ctrl/Shift modifiers, both for keyboard and mouse.
// - Record current selection state for RangeSrc
// This is all rather complex, best to run and refer to "widgets_multiselect_xxx" tests in vangui_test_suite.
void VanGui::MultiSelectItemFooter(VanGuiID id, bool* p_selected, bool* p_pressed)
{
    VanGuiContext& g = *GVanGui;
    VanGuiWindow* window = g.CurrentWindow;

    bool selected = *p_selected;
    bool pressed = *p_pressed;
    VanGuiMultiSelectTempData* ms = g.CurrentMultiSelect;
    VanGuiMultiSelectState* storage = ms->Storage;
    if (pressed)
        ms->IsFocused = true;

    bool hovered = false;
    if (g.LastItemData.StatusFlags & VanGuiItemStatusFlags_HoveredRect)
        hovered = IsItemHovered(VanGuiHoveredFlags_AllowWhenBlockedByPopup);
    if (!ms->IsFocused && !hovered)
        return;

    VanGuiSelectionUserData item_data = g.NextItemData.SelectionUserData;

    VanGuiMultiSelectFlags flags = ms->Flags;
    const bool is_singleselect = (flags & VanGuiMultiSelectFlags_SingleSelect) != 0;
    bool is_ctrl = (ms->KeyMods & VanGuiMod_Ctrl) != 0;
    bool is_shift = (ms->KeyMods & VanGuiMod_Shift) != 0;

    bool apply_to_range_src = false;

    if (g.NavId == id && storage->RangeSrcItem == VanGuiSelectionUserData_Invalid)
        apply_to_range_src = true;
    if (ms->IsEndIO == false)
    {
        ms->IO.Requests.resize(0);
        ms->IsEndIO = true;
    }

    // Auto-select as you navigate a list
    if (g.NavJustMovedToId == id)
    {
        if ((flags & VanGuiMultiSelectFlags_NoAutoSelect) == 0)
        {
            if (is_ctrl && is_shift)
                pressed = true;
            else if (!is_ctrl)
                selected = pressed = true;
        }
        else
        {
            // With NoAutoSelect, using Shift+keyboard performs a write/copy
            if (is_shift)
                pressed = true;
            else if (!is_ctrl)
                apply_to_range_src = true; // Since if (pressed) {} main block is not running we update this
        }
    }

    if (apply_to_range_src)
    {
        storage->RangeSrcItem = item_data;
        storage->RangeSelected = selected; // Will be updated at the end of this function anyway.
    }

    // Box-select toggle handling
    if (ms->BoxSelectId != 0)
        if (VanGuiBoxSelectState* bs = GetBoxSelectState(ms->BoxSelectId))
        {
            VanRect item_rect = g.LastItemData.Rect;
            if (!window->DC.NavIsScrollPushableX) // FIXME: Rename to be more generic.
                if (VanGuiTable* table = g.CurrentTable)
                    if (table->CurrentColumn != -1)
                    {
                        // FIXME: We cannot solely use current ClipRect as it includes HostClipRect.
                        // However we account for ClipRect being larger than current column (e.g. when using SpanAllColumns)
                        // A more generic version would be nice, but window->WorkRect.Min/Max exclude CellPadding. (#7994, #9383)
                        VanGuiTableColumn* column = &table->Columns[table->CurrentColumn];
                        float clip_min_x = (g.LastItemData.ItemFlags & VanGuiItemStatusFlags_HasClipRect) ? g.LastItemData.ClipRect.Min.x : window->ClipRect.Min.x;
                        float clip_max_x = (g.LastItemData.ItemFlags & VanGuiItemStatusFlags_HasClipRect) ? g.LastItemData.ClipRect.Max.x : window->ClipRect.Max.x;
                        if (clip_min_x != clip_max_x) // When zero sized we expect that bounds have been clamped and thus are unreliable
                        {
                            item_rect.Min.x = VanMax(item_rect.Min.x, VanMin(column->MinX, clip_min_x));
                            item_rect.Max.x = VanMin(item_rect.Max.x, VanMax(column->MaxX, clip_max_x));
                        }
                        else
                        {
                            item_rect.Min.x = VanMax(item_rect.Min.x, column->MinX);
                            item_rect.Max.x = VanMin(item_rect.Max.x, column->MaxX);
                        }
                        //GetForegroundDrawList()->AddRect(item_rect.Min, item_rect.Max, VAN_COL32(255, 0, 255, 255));
                    }
            const bool rect_overlap_curr = bs->BoxSelectRectCurr.Overlaps(item_rect);
            const bool rect_overlap_prev = bs->BoxSelectRectPrev.Overlaps(item_rect);
            if ((rect_overlap_curr && !rect_overlap_prev && !selected) || (rect_overlap_prev && !rect_overlap_curr))
            {
                if (storage->LastSelectionSize <= 0 && bs->IsStartedSetNavIdOnce)
                {
                    pressed = true; // First item act as a pressed: code below will emit selection request and set NavId (whatever we emit here will be overridden anyway)
                    bs->IsStartedSetNavIdOnce = false;
                }
                else
                {
                    selected = !selected;
                    MultiSelectAddSetRange(ms, selected, +1, item_data, item_data);
#ifdef VANGUI_DEBUG_BOXSELECT
                    GetForegroundDrawList()->AddRectFilled(g.LastItemData.Rect.Min, g.LastItemData.Rect.Max, selected ? VAN_COL32(0, 255, 0, 200) : VAN_COL32(255, 0, 0, 200));
#endif
                }
                storage->LastSelectionSize = VanMax(storage->LastSelectionSize + 1, 1);
            }
        }

    // Right-click handling.
    // FIXME-MULTISELECT: Maybe should be moved to Selectable()? Also see #5816, #8200, #9015
    if (hovered && IsMouseClicked(1) && (flags & (VanGuiMultiSelectFlags_NoAutoSelect | VanGuiMultiSelectFlags_NoSelectOnRightClick)) == 0)
    {
        if (g.ActiveId != 0 && g.ActiveId != id)
            ClearActiveID();
        SetFocusID(id, window);
        if (!pressed && !selected)
        {
            pressed = true;
            is_ctrl = is_shift = false;
        }
    }

    // Unlike Space, Enter doesn't alter selection (but can still return a press) unless current item is not selected.
    // The later, "unless current item is not select", may become optional? It seems like a better default if Enter doesn't necessarily open something
    // (unlike e.g. Windows explorer). For use case where Enter always open something, we might decide to make this optional?
    const bool enter_pressed = pressed && (g.NavActivateId == id) && (g.NavActivateFlags & VanGuiActivateFlags_PreferInput);

    // Alter selection
    if (pressed && (!enter_pressed || !selected))
    {
        // Box-select
        VanGuiInputSource input_source = (g.NavJustMovedToId == id || g.NavActivateId == id) ? g.NavInputSource : VanGuiInputSource_Mouse;
        if (flags & (VanGuiMultiSelectFlags_BoxSelect1d | VanGuiMultiSelectFlags_BoxSelect2d))
            if (!g.BoxSelectState.IsActive && !g.BoxSelectState.IsStarting && input_source == VanGuiInputSource_Mouse && g.IO.MouseClickedCount[0] == 1)
                BoxSelectPreStartDrag(ms->BoxSelectId, item_data);

        //----------------------------------------------------------------------------------------
        // ACTION                      | Begin  | Pressed/Activated  | End
        //----------------------------------------------------------------------------------------
        // Keys Navigated:             | Clear  | Src=item, Sel=1               SetRange 1
        // Keys Navigated: Ctrl        | n/a    | n/a
        // Keys Navigated:      Shift  | n/a    | Dst=item, Sel=1,   => Clear + SetRange 1
        // Keys Navigated: Ctrl+Shift  | n/a    | Dst=item, Sel=Src  => Clear + SetRange Src-Dst
        // Keys Activated:             | n/a    | Src=item, Sel=1    => Clear + SetRange 1
        // Keys Activated: Ctrl        | n/a    | Src=item, Sel=!Sel =>         SetSange 1
        // Keys Activated:      Shift  | n/a    | Dst=item, Sel=1    => Clear + SetSange 1
        //----------------------------------------------------------------------------------------
        // Mouse Pressed:              | n/a    | Src=item, Sel=1,   => Clear + SetRange 1
        // Mouse Pressed:  Ctrl        | n/a    | Src=item, Sel=!Sel =>         SetRange 1
        // Mouse Pressed:       Shift  | n/a    | Dst=item, Sel=1,   => Clear + SetRange 1
        // Mouse Pressed:  Ctrl+Shift  | n/a    | Dst=item, Sel=!Sel =>         SetRange Src-Dst
        //----------------------------------------------------------------------------------------

        if ((flags & VanGuiMultiSelectFlags_NoAutoClear) == 0)
        {
            bool request_clear = false;
            if (is_singleselect)
                request_clear = true;
            else if ((input_source == VanGuiInputSource_Mouse || g.NavActivateId == id) && !is_ctrl)
                request_clear = (flags & VanGuiMultiSelectFlags_NoAutoClearOnReselect) ? !selected : true;
            else if ((input_source == VanGuiInputSource_Keyboard || input_source == VanGuiInputSource_Gamepad) && is_shift && !is_ctrl)
                request_clear = true; // With is_shift==false the RequestClear was done in BeginIO, not necessary to do again.
            if (request_clear)
                MultiSelectAddSetAll(ms, false);
        }

        int range_direction;
        bool range_selected;
        if (is_shift && !is_singleselect)
        {
            //VAN_ASSERT(storage->HasRangeSrc && storage->HasRangeValue);
            if (storage->RangeSrcItem == VanGuiSelectionUserData_Invalid)
                storage->RangeSrcItem = item_data;
            if ((flags & VanGuiMultiSelectFlags_NoAutoSelect) == 0)
            {
                // Shift+Arrow always select
                // Ctrl+Shift+Arrow copy source selection state (already stored by BeginMultiSelect() in storage->RangeSelected)
                range_selected = (is_ctrl && storage->RangeSelected != -1) ? (storage->RangeSelected != 0) : true;
            }
            else
            {
                // Shift+Arrow copy source selection state
                // Shift+Click always copy from target selection state
                if (ms->IsKeyboardSetRange)
                    range_selected = (storage->RangeSelected != -1) ? (storage->RangeSelected != 0) : true;
                else
                    range_selected = !selected;
            }
            range_direction = ms->RangeSrcPassedBy ? +1 : -1;
        }
        else
        {
            // Ctrl inverts selection, otherwise always select
            if ((flags & VanGuiMultiSelectFlags_NoAutoSelect) == 0)
                selected = is_ctrl ? !selected : true;
            else
                selected = !selected;
            storage->RangeSrcItem = item_data;
            range_selected = selected;
            range_direction = +1;
        }
        MultiSelectAddSetRange(ms, range_selected, range_direction, storage->RangeSrcItem, item_data);
    }

    // Update/store the selection state of the Source item (used by Ctrl+Shift, when Source is unselected we perform a range unselect)
    if (storage->RangeSrcItem == item_data)
        storage->RangeSelected = selected ? 1 : 0;

    // Update/store the selection state of focused item
    if (g.NavId == id)
    {
        storage->NavIdItem = item_data;
        storage->NavIdSelected = selected ? 1 : 0;
    }
    if (storage->NavIdItem == item_data)
        ms->NavIdPassedBy = true;

    *p_selected = selected;
    *p_pressed = pressed;
}

void VanGui::MultiSelectAddSetAll(VanGuiMultiSelectTempData* ms, bool selected)
{
    VanGuiSelectionRequest req = { VanGuiSelectionRequestType_SetAll, selected, 0, VanGuiSelectionUserData_Invalid, VanGuiSelectionUserData_Invalid };
    ms->IO.Requests.resize(0);      // Can always clear previous requests
    ms->IO.Requests.push_back(req); // Add new request
}

void VanGui::MultiSelectAddSetRange(VanGuiMultiSelectTempData* ms, bool selected, int range_dir, VanGuiSelectionUserData first_item, VanGuiSelectionUserData last_item)
{
    // Merge contiguous spans into same request (unless NoRangeSelect is set which guarantees single-item ranges)
    // FIXME-OPT: Disabled on 2026/04/09 as this would break with any form of coarse clipping that we don't know about (e.g. TableNextColumn() return value).
    // The low-hanging fruit would be to know that VanGuiSelectionUserData are sequential indices, in which case we can trivially compare PrevSubmittedItem + RangeDir == FirstItem.
    // User can always perform this merge if required.
#if 0
    if (ms->IO.Requests.Size > 0 && first_item == last_item && (ms->Flags & VanGuiMultiSelectFlags_NoRangeSelect) == 0)
    {
        VanGuiSelectionRequest* prev = &ms->IO.Requests.Data[ms->IO.Requests.Size - 1];
        if (prev->Type == VanGuiSelectionRequestType_SetRange && prev->RangeLastItem == ms->PrevSubmittedItem && prev->Selected == selected)
        {
            prev->RangeLastItem = last_item;
            return;
        }
    }
#endif

    VanGuiSelectionRequest req = { VanGuiSelectionRequestType_SetRange, selected, static_cast<VanS8>(range_dir), (range_dir > 0) ? first_item : last_item, (range_dir > 0) ? last_item : first_item };
    ms->IO.Requests.push_back(req); // Add new request
}

void VanGui::DebugNodeMultiSelectState(VanGuiMultiSelectState* storage)
{
#ifndef VANGUI_DISABLE_DEBUG_TOOLS
    const bool is_active = (storage->LastFrameActive >= GetFrameCount() - 2); // Note that fully clipped early out scrolling tables will appear as inactive here.
    if (!is_active) { PushStyleColor(VanGuiCol_Text, GetStyleColorVec4(VanGuiCol_TextDisabled)); }
    bool open = TreeNode(reinterpret_cast<void*>(static_cast<intptr_t>(storage->ID)), "MultiSelect 0x%08X in '%s'%s", storage->ID, storage->Window ? storage->Window->Name : "N/A", is_active ? "" : " *Inactive*");
    if (!is_active) { PopStyleColor(); }
    if (!open)
        return;
    Text("RangeSrcItem = %" VAN_PRId64 " (0x%" VAN_PRIX64 "), RangeSelected = %d", storage->RangeSrcItem, storage->RangeSrcItem, storage->RangeSelected);
    Text("NavIdItem = %" VAN_PRId64 " (0x%" VAN_PRIX64 "), NavIdSelected = %d", storage->NavIdItem, storage->NavIdItem, storage->NavIdSelected);
    Text("LastSelectionSize = %d", storage->LastSelectionSize); // Provided by user
    TreePop();
#else
    VAN_UNUSED(storage);
#endif
}

//-------------------------------------------------------------------------
// [SECTION] Widgets: Multi-Select helpers
//-------------------------------------------------------------------------
// - VanGuiSelectionBasicStorage
// - VanGuiSelectionExternalStorage
//-------------------------------------------------------------------------

VanGuiSelectionBasicStorage::VanGuiSelectionBasicStorage()
{
    Size = 0;
    PreserveOrder = false;
    UserData = nullptr;
    AdapterIndexToStorageId = [](VanGuiSelectionBasicStorage*, int idx) { return static_cast<VanGuiID>(idx); };
    _SelectionOrder = 1; // Always >0
}

void VanGuiSelectionBasicStorage::Clear()
{
    Size = 0;
    _SelectionOrder = 1; // Always >0
    _Storage.Data.resize(0);
}

void VanGuiSelectionBasicStorage::Swap(VanGuiSelectionBasicStorage& r)
{
    VanSwap(Size, r.Size);
    VanSwap(_SelectionOrder, r._SelectionOrder);
    _Storage.Data.swap(r._Storage.Data);
}

bool VanGuiSelectionBasicStorage::Contains(VanGuiID id) const
{
    return _Storage.GetInt(id, 0) != 0;
}

static int VANGUI_CDECL PairComparerByValueInt(const void* lhs, const void* rhs)
{
    int lhs_v = static_cast<const VanGuiStoragePair*>(lhs)->val_i;
    int rhs_v = static_cast<const VanGuiStoragePair*>(rhs)->val_i;
    return (lhs_v > rhs_v ? +1 : lhs_v < rhs_v ? -1 : 0);
}

// GetNextSelectedItem() is an abstraction allowing us to change our underlying actual storage system without impacting user.
// (e.g. store unselected vs compact down, compact down on demand, use raw VanVector<VanGuiID> instead of VanGuiStorage...)
bool VanGuiSelectionBasicStorage::GetNextSelectedItem(void** opaque_it, VanGuiID* out_id)
{
    VanGuiStoragePair* it = static_cast<VanGuiStoragePair*>(*opaque_it);
    VanGuiStoragePair* it_end = _Storage.Data.Data + _Storage.Data.Size;
    if (PreserveOrder && it == nullptr && it_end != nullptr)
        VanQsort(_Storage.Data.Data, static_cast<size_t>(_Storage.Data.Size), sizeof(VanGuiStoragePair), PairComparerByValueInt); // ~VanGuiStorage::BuildSortByValueInt()
    if (it == nullptr)
        it = _Storage.Data.Data;
    VAN_ASSERT(it >= _Storage.Data.Data && it <= it_end);
    if (it != it_end)
        while (it->val_i == 0 && it < it_end)
            it++;
    const bool has_more = (it != it_end);
    *opaque_it = has_more ? static_cast<void**>(static_cast<void*>(it + 1)) : static_cast<void**>(static_cast<void*>(it));
    *out_id = has_more ? it->key : 0;
    if (PreserveOrder && !has_more)
        _Storage.BuildSortByKey();
    return has_more;
}

void VanGuiSelectionBasicStorage::SetItemSelected(VanGuiID id, bool selected)
{
    int* p_int = _Storage.GetIntRef(id, 0);
    if (selected && *p_int == 0) { *p_int = _SelectionOrder++; Size++; }
    else if (!selected && *p_int != 0) { *p_int = 0; Size--; }
}

// Optimized for batch edits (with same value of 'selected')
static void VanGuiSelectionBasicStorage_BatchSetItemSelected(VanGuiSelectionBasicStorage* selection, VanGuiID id, bool selected, int size_before_amends, int selection_order)
{
    VanGuiStorage* storage = &selection->_Storage;
    VanGuiStoragePair* it = VanLowerBound(storage->Data.Data, storage->Data.Data + size_before_amends, id);
    const bool is_contained = (it != storage->Data.Data + size_before_amends) && (it->key == id);
    if (selected == (is_contained && it->val_i != 0))
        return;
    if (selected && !is_contained)
        storage->Data.push_back(VanGuiStoragePair(id, selection_order)); // Push unsorted at end of vector, will be sorted in SelectionMultiAmendsFinish()
    else if (is_contained)
        it->val_i = selected ? selection_order : 0; // Modify in-place.
    selection->Size += selected ? +1 : -1;
}

static void VanGuiSelectionBasicStorage_BatchFinish(VanGuiSelectionBasicStorage* selection, bool selected, int size_before_amends)
{
    VanGuiStorage* storage = &selection->_Storage;
    if (selected && selection->Size != size_before_amends)
        storage->BuildSortByKey(); // When done selecting: sort everything
}

// Apply requests coming from BeginMultiSelect() and EndMultiSelect().
// - Enable 'Demo->Tools->Debug Log->Selection' to see selection requests as they happen.
// - Honoring SetRange requests requires that you can iterate/interpolate between RangeFirstItem and RangeLastItem.
//   - In this demo we often submit indices to SetNextItemSelectionUserData() + store the same indices in persistent selection.
//   - Your code may do differently. If you store pointers or objects ID in VanGuiSelectionUserData you may need to perform
//     a lookup in order to have some way to iterate/interpolate between two items.
// - A full-featured application is likely to allow search/filtering which is likely to lead to using indices
//   and constructing a view index <> object id/ptr data structure anyway.
// WHEN YOUR APPLICATION SETTLES ON A CHOICE, YOU WILL PROBABLY PREFER TO GET RID OF THIS UNNECESSARY 'VanGuiSelectionBasicStorage' INDIRECTION LOGIC.
// Notice that with the simplest adapter (using indices everywhere), all functions return their parameters.
// The most simple implementation (using indices everywhere) would look like:
//   for (VanGuiSelectionRequest& req : ms_io->Requests)
//   {
//      if (req.Type == VanGuiSelectionRequestType_SetAll)    { Clear(); if (req.Selected) { for (int n = 0; n < items_count; n++) { SetItemSelected(n, true); } }
//      if (req.Type == VanGuiSelectionRequestType_SetRange)  { for (int n = (int)ms_io->RangeFirstItem; n <= (int)ms_io->RangeLastItem; n++) { SetItemSelected(n, ms_io->Selected); } }
//   }
void VanGuiSelectionBasicStorage::ApplyRequests(VanGuiMultiSelectIO* ms_io)
{
    // For convenience we obtain ItemsCount as passed to BeginMultiSelect(), which is optional.
    // It makes sense when using VanGuiSelectionBasicStorage to simply pass your items count to BeginMultiSelect().
    // Other scheme may handle SetAll differently.
    VAN_ASSERT(ms_io->ItemsCount != -1 && "Missing value for items_count in BeginMultiSelect() call!");
    VAN_ASSERT(AdapterIndexToStorageId != nullptr);

    // This is optimized/specialized to cope with very large selections (e.g. 100k+ items)
    // - A simpler version could call SetItemSelected() directly instead of VanGuiSelectionBasicStorage_BatchSetItemSelected() + VanGuiSelectionBasicStorage_BatchFinish().
    // - Optimized select can append unsorted, then sort in a second pass. Optimized unselect can clear in-place then compact in a second pass.
    // - A more optimal version wouldn't even use VanGuiStorage but directly a VanVector<VanGuiID> to reduce bandwidth, but this is a reasonable trade off to reuse code.
    // - There are many ways this could be better optimized. The worse case scenario being: using BoxSelect2d in a grid, box-select scrolling down while wiggling
    //   left and right: it affects coarse clipping + can emit multiple SetRange with 1 item each.
    // FIXME-OPT: For each block of consecutive SetRange request:
    // - add all requests to a sorted list, store ID, selected, offset in VanGuiStorage.
    // - rewrite sorted storage a single time.
    for (VanGuiSelectionRequest& req : ms_io->Requests)
    {
        if (req.Type == VanGuiSelectionRequestType_SetAll)
        {
            Clear();
            if (req.Selected)
            {
                _Storage.Data.reserve(ms_io->ItemsCount);
                const int size_before_amends = _Storage.Data.Size;
                for (int idx = 0; idx < ms_io->ItemsCount; idx++, _SelectionOrder++)
                    VanGuiSelectionBasicStorage_BatchSetItemSelected(this, GetStorageIdFromIndex(idx), req.Selected, size_before_amends, _SelectionOrder);
                VanGuiSelectionBasicStorage_BatchFinish(this, req.Selected, size_before_amends);
            }
        }
        else if (req.Type == VanGuiSelectionRequestType_SetRange)
        {
            const int selection_changes = static_cast<int>(req.RangeLastItem) - static_cast<int>(req.RangeFirstItem) + 1;
            //VanGuiContext& g = *GVanGui; VANGUI_DEBUG_LOG_SELECTION("Req %d/%d: set %d to %d\n", ms_io->Requests.index_from_ptr(&req), ms_io->Requests.Size, selection_changes, req.Selected);
            if (selection_changes == 1 || (selection_changes < Size / 100))
            {
                // Multiple sorted insertion + copy likely to be faster.
                // Technically we could do a single copy with a little more work (sort sequential SetRange requests)
                for (int idx = static_cast<int>(req.RangeFirstItem); idx <= static_cast<int>(req.RangeLastItem); idx++)
                    SetItemSelected(GetStorageIdFromIndex(idx), req.Selected);
            }
            else
            {
                // Append insertion + single sort likely be faster.
                // Use req.RangeDirection to set order field so that Shift+Clicking from 1 to 5 is different than Shift+Clicking from 5 to 1
                const int size_before_amends = _Storage.Data.Size;
                int selection_order = _SelectionOrder + ((req.RangeDirection < 0) ? selection_changes - 1 : 0);
                for (int idx = static_cast<int>(req.RangeFirstItem); idx <= static_cast<int>(req.RangeLastItem); idx++, selection_order += req.RangeDirection)
                    VanGuiSelectionBasicStorage_BatchSetItemSelected(this, GetStorageIdFromIndex(idx), req.Selected, size_before_amends, selection_order);
                if (req.Selected)
                    _SelectionOrder += selection_changes;
                VanGuiSelectionBasicStorage_BatchFinish(this, req.Selected, size_before_amends);
            }
        }
    }
}

//-------------------------------------------------------------------------

VanGuiSelectionExternalStorage::VanGuiSelectionExternalStorage()
{
    UserData = nullptr;
    AdapterSetItemSelected = nullptr;
}

// Apply requests coming from BeginMultiSelect() and EndMultiSelect().
// We also pull 'ms_io->ItemsCount' as passed for BeginMultiSelect() for consistency with VanGuiSelectionBasicStorage
// This makes no assumption about underlying storage.
void VanGuiSelectionExternalStorage::ApplyRequests(VanGuiMultiSelectIO* ms_io)
{
    VAN_ASSERT(AdapterSetItemSelected);
    for (VanGuiSelectionRequest& req : ms_io->Requests)
    {
        if (req.Type == VanGuiSelectionRequestType_SetAll)
            for (int idx = 0; idx < ms_io->ItemsCount; idx++)
                AdapterSetItemSelected(this, idx, req.Selected);
        if (req.Type == VanGuiSelectionRequestType_SetRange)
            for (int idx = static_cast<int>(req.RangeFirstItem); idx <= static_cast<int>(req.RangeLastItem); idx++)
                AdapterSetItemSelected(this, idx, req.Selected);
    }
}

//-------------------------------------------------------------------------
// [SECTION] Widgets: ListBox
//-------------------------------------------------------------------------
// - BeginListBox()
// - EndListBox()
// - ListBox()
//-------------------------------------------------------------------------

// This is essentially a thin wrapper to using BeginChild/EndChild with the VanGuiChildFlags_FrameStyle flag for stylistic changes + displaying a label.
// This handle some subtleties with capturing info from the label.
// If you don't need a label you can pretty much directly use VanGui::BeginChild() with VanGuiChildFlags_FrameStyle.
// Tip: To have a list filling the entire window width, use size.x = -FLT_MIN and pass an non-visible label e.g. "##empty"
// Tip: If your vertical size is calculated from an item count (e.g. 10 * item_height) consider adding a fractional part to facilitate seeing scrolling boundaries (e.g. 10.5f * item_height).
bool VanGui::BeginListBox(const char* label, const VanVec2& size_arg)
{
    VanGuiContext& g = *GVanGui;
    VanGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    const VanGuiStyle& style = g.Style;
    const VanGuiID id = GetID(label);
    const char* label_end = FindRenderedTextEnd(label);
    const VanVec2 label_size = CalcTextSize(label, label_end, false);

    // Size default to hold ~7.25 items.
    // Fractional number of items helps seeing that we can scroll down/up without looking at scrollbar.
    VanVec2 size = VanTrunc(CalcItemSize(size_arg, CalcItemWidth(), GetTextLineHeightWithSpacing() * 7.25f + style.FramePadding.y * 2.0f));
    VanVec2 frame_size = VanVec2(size.x, VanMax(size.y, label_size.y));
    VanRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + frame_size);
    VanRect bb(frame_bb.Min, frame_bb.Max + VanVec2(label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f, 0.0f));
    g.NextItemData.ClearFlags();

    if (!IsRectVisible(bb.Min, bb.Max))
    {
        ItemSize(bb.GetSize(), style.FramePadding.y);
        ItemAdd(bb, 0, &frame_bb);
        g.NextWindowData.ClearFlags(); // We behave like Begin() and need to consume those values
        return false;
    }

    // FIXME-OPT: We could omit the BeginGroup() if label_size.x == 0.0f but would need to omit the EndGroup() as well.
    BeginGroup();
    if (label_size.x > 0.0f)
    {
        VanVec2 label_pos = VanVec2(frame_bb.Max.x + style.ItemInnerSpacing.x, frame_bb.Min.y + style.FramePadding.y);
        RenderText(label_pos, label, label_end, false);
        window->DC.CursorMaxPos = VanMax(window->DC.CursorMaxPos, label_pos + label_size);
        AlignTextToFramePadding();
    }

    (void)(BeginChild(id, frame_bb.GetSize(), VanGuiChildFlags_FrameStyle));
    return true;
}

void VanGui::EndListBox()
{
    VanGuiContext& g = *GVanGui;
    VanGuiWindow* window = g.CurrentWindow;
    VAN_ASSERT((window->Flags & VanGuiWindowFlags_ChildWindow) && "Mismatched BeginListBox/EndListBox calls. Did you test the return value of BeginListBox?");
    VAN_UNUSED(window);

    EndChild();
    EndGroup(); // This is only required to be able to do IsItemXXX query on the whole ListBox including label
}

bool VanGui::ListBox(const char* label, int* current_item, const char* const items[], int items_count, int height_items)
{
    const bool value_changed = ListBox(label, current_item, Items_ArrayGetter, static_cast<void*>(const_cast<char**>(items)), items_count, height_items);
    return value_changed;
}

// This is merely a helper around BeginListBox(), EndListBox().
// Considering using those directly to submit custom data or store selection differently.
bool VanGui::ListBox(const char* label, int* current_item, const char* (*getter)(void* user_data, int idx), void* user_data, int items_count, int height_in_items)
{
    VanGuiContext& g = *GVanGui;

    // Calculate size from "height_in_items"
    if (height_in_items < 0)
        height_in_items = VanMin(items_count, 7);
    float height_in_items_f = height_in_items + 0.25f;
    VanVec2 size(0.0f, VanTrunc(GetTextLineHeightWithSpacing() * height_in_items_f + g.Style.FramePadding.y * 2.0f));

    if (!BeginListBox(label, size))
        return false;

    // Assume all items have even height (= 1 line of text). If you need items of different height,
    // you can create a custom version of ListBox() in your code without using the clipper.
    bool value_changed = false;
    VanGuiListClipper clipper;
    clipper.Begin(items_count, GetTextLineHeightWithSpacing()); // We know exactly our line height here so we pass it as a minor optimization, but generally you don't need to.
    clipper.IncludeItemByIndex(*current_item);
    while (clipper.Step())
        for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
        {
            const char* item_text = getter(user_data, i);
            if (item_text == nullptr)
                item_text = "*Unknown item*";

            PushID(i);
            const bool item_selected = (i == *current_item);
            if (Selectable(item_text, item_selected))
            {
                *current_item = i;
                value_changed = true;
            }
            if (item_selected)
                SetItemDefaultFocus();
            PopID();
        }
    EndListBox();

    if (value_changed)
        MarkItemEdited(g.LastItemData.ID);

    return value_changed;
}

//-------------------------------------------------------------------------
// [SECTION] Widgets: PlotLines, PlotHistogram
//-------------------------------------------------------------------------
// - PlotEx() [Internal]
// - PlotLines()
// - PlotHistogram()
//-------------------------------------------------------------------------
// Plot/Graph widgets are not very good.
// Consider using VanPlot (https://github.com/epezent/implot) which is much better!
//-------------------------------------------------------------------------

int VanGui::PlotEx(VanGuiPlotType plot_type, const char* label, float (*values_getter)(void* data, int idx), void* data, int values_count, int values_offset, const char* overlay_text, float scale_min, float scale_max, const VanVec2& size_arg)
{
    VanGuiContext& g = *GVanGui;
    VanGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return -1;

    const VanGuiStyle& style = g.Style;
    const VanGuiID id = window->GetID(label);

    const char* label_end = FindRenderedTextEnd(label);
    const VanVec2 label_size = CalcTextSize(label, label_end, false);
    const VanVec2 frame_size = CalcItemSize(size_arg, CalcItemWidth(), label_size.y + style.FramePadding.y * 2.0f);

    const VanRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + frame_size);
    const VanRect inner_bb(frame_bb.Min + style.FramePadding, frame_bb.Max - style.FramePadding);
    const VanRect total_bb(frame_bb.Min, frame_bb.Max + VanVec2(label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f, 0));
    ItemSize(total_bb, style.FramePadding.y);
    if (!ItemAdd(total_bb, id, &frame_bb, VanGuiItemFlags_NoNav))
        return -1;
    bool hovered;
    ButtonBehavior(frame_bb, id, &hovered, nullptr);

    // Determine scale from values if not specified
    if (scale_min == FLT_MAX || scale_max == FLT_MAX)
    {
        float v_min = FLT_MAX;
        float v_max = -FLT_MAX;
        for (int i = 0; i < values_count; i++)
        {
            const float v = values_getter(data, i);
            if (v != v) // Ignore NaN values
                continue;
            v_min = VanMin(v_min, v);
            v_max = VanMax(v_max, v);
        }
        if (scale_min == FLT_MAX)
            scale_min = v_min;
        if (scale_max == FLT_MAX)
            scale_max = v_max;
    }

    RenderFrame(frame_bb.Min, frame_bb.Max, GetColorU32(VanGuiCol_FrameBg), true, style.FrameRounding);

    const int values_count_min = (plot_type == VanGuiPlotType_Lines) ? 2 : 1;
    int idx_hovered = -1;
    if (values_count >= values_count_min)
    {
        int res_w = VanMin(static_cast<int>(frame_size.x), values_count) + ((plot_type == VanGuiPlotType_Lines) ? -1 : 0);
        int item_count = values_count + ((plot_type == VanGuiPlotType_Lines) ? -1 : 0);

        // Tooltip on hover
        if (hovered && inner_bb.Contains(g.IO.MousePos))
        {
            const float t = VanClamp((g.IO.MousePos.x - inner_bb.Min.x) / (inner_bb.Max.x - inner_bb.Min.x), 0.0f, 0.9999f);
            const int v_idx = static_cast<int>(t * item_count);
            VAN_ASSERT(v_idx >= 0 && v_idx < values_count);

            const float v0 = values_getter(data, (v_idx + values_offset) % values_count);
            const float v1 = values_getter(data, (v_idx + 1 + values_offset) % values_count);
            if (plot_type == VanGuiPlotType_Lines)
                SetTooltip("%d: %8.4g\n%d: %8.4g", v_idx, v0, v_idx + 1, v1);
            else if (plot_type == VanGuiPlotType_Histogram)
                SetTooltip("%d: %8.4g", v_idx, v0);
            idx_hovered = v_idx;
        }

        const float t_step = 1.0f / static_cast<float>(res_w);
        const float inv_scale = (scale_min == scale_max) ? 0.0f : (1.0f / (scale_max - scale_min));

        float v0 = values_getter(data, (0 + values_offset) % values_count);
        float t0 = 0.0f;
        VanVec2 tp0 = VanVec2( t0, 1.0f - VanSaturate((v0 - scale_min) * inv_scale) );                       // Point in the normalized space of our target rectangle
        float histogram_zero_line_t = (scale_min * scale_max < 0.0f) ? (1 + scale_min * inv_scale) : (scale_min < 0.0f ? 0.0f : 1.0f);   // Where does the zero line stands

        const VanU32 col_base = GetColorU32((plot_type == VanGuiPlotType_Lines) ? VanGuiCol_PlotLines : VanGuiCol_PlotHistogram);
        const VanU32 col_hovered = GetColorU32((plot_type == VanGuiPlotType_Lines) ? VanGuiCol_PlotLinesHovered : VanGuiCol_PlotHistogramHovered);

        for (int n = 0; n < res_w; n++)
        {
            const float t1 = t0 + t_step;
            const int v1_idx = static_cast<int>(t0 * item_count + 0.5f);
            VAN_ASSERT(v1_idx >= 0 && v1_idx < values_count);
            const float v1 = values_getter(data, (v1_idx + values_offset + 1) % values_count);
            const VanVec2 tp1 = VanVec2( t1, 1.0f - VanSaturate((v1 - scale_min) * inv_scale) );

            // NB: Draw calls are merged together by the DrawList system. Still, we should render our batch are lower level to save a bit of CPU.
            VanVec2 pos0 = VanLerp(inner_bb.Min, inner_bb.Max, tp0);
            VanVec2 pos1 = VanLerp(inner_bb.Min, inner_bb.Max, (plot_type == VanGuiPlotType_Lines) ? tp1 : VanVec2(tp1.x, histogram_zero_line_t));
            if (plot_type == VanGuiPlotType_Lines)
            {
                window->DrawList->AddLine(pos0, pos1, idx_hovered == v1_idx ? col_hovered : col_base);
            }
            else if (plot_type == VanGuiPlotType_Histogram)
            {
                if (pos1.x >= pos0.x + 2.0f)
                    pos1.x -= 1.0f;
                window->DrawList->AddRectFilled(pos0, pos1, idx_hovered == v1_idx ? col_hovered : col_base);
            }

            t0 = t1;
            tp0 = tp1;
        }
    }

    // Text overlay
    if (overlay_text)
        RenderTextClipped(VanVec2(frame_bb.Min.x, frame_bb.Min.y + style.FramePadding.y), frame_bb.Max, overlay_text, nullptr, nullptr, VanVec2(0.5f, 0.0f));

    if (label_size.x > 0.0f)
        RenderText(VanVec2(frame_bb.Max.x + style.ItemInnerSpacing.x, inner_bb.Min.y), label, label_end, false);

    // Return hovered index or -1 if none are hovered.
    // This is currently not exposed in the public API because we need a larger redesign of the whole thing, but in the short-term we are making it available in PlotEx().
    VANGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags);
    return idx_hovered;
}

struct VanGuiPlotArrayGetterData
{
    const float* Values;
    int Stride;

    VanGuiPlotArrayGetterData(const float* values, int stride) { Values = values; Stride = stride; }
};

static float Plot_ArrayGetter(void* data, int idx)
{
    VanGuiPlotArrayGetterData* plot_data = static_cast<VanGuiPlotArrayGetterData*>(data);
    const float v = *reinterpret_cast<const float*>(reinterpret_cast<const unsigned char*>(plot_data->Values) + static_cast<size_t>(idx) * plot_data->Stride);
    return v;
}

void VanGui::PlotLines(const char* label, const float* values, int values_count, int values_offset, const char* overlay_text, float scale_min, float scale_max, VanVec2 graph_size, int stride)
{
    VanGuiPlotArrayGetterData data(values, stride);
    PlotEx(VanGuiPlotType_Lines, label, &Plot_ArrayGetter, static_cast<void*>(&data), values_count, values_offset, overlay_text, scale_min, scale_max, graph_size);
}

void VanGui::PlotLines(const char* label, float (*values_getter)(void* data, int idx), void* data, int values_count, int values_offset, const char* overlay_text, float scale_min, float scale_max, VanVec2 graph_size)
{
    PlotEx(VanGuiPlotType_Lines, label, values_getter, data, values_count, values_offset, overlay_text, scale_min, scale_max, graph_size);
}

// Plot Histogram (the data provided _is_ histogram data. it doesn't compute the histogram of your data)
void VanGui::PlotHistogram(const char* label, const float* values, int values_count, int values_offset, const char* overlay_text, float scale_min, float scale_max, VanVec2 graph_size, int stride)
{
    VanGuiPlotArrayGetterData data(values, stride);
    PlotEx(VanGuiPlotType_Histogram, label, &Plot_ArrayGetter, static_cast<void*>(&data), values_count, values_offset, overlay_text, scale_min, scale_max, graph_size);
}

void VanGui::PlotHistogram(const char* label, float (*values_getter)(void* data, int idx), void* data, int values_count, int values_offset, const char* overlay_text, float scale_min, float scale_max, VanVec2 graph_size)
{
    PlotEx(VanGuiPlotType_Histogram, label, values_getter, data, values_count, values_offset, overlay_text, scale_min, scale_max, graph_size);
}

//-------------------------------------------------------------------------
// [SECTION] Widgets: Value helpers
// Those is not very useful, legacy API.
//-------------------------------------------------------------------------
// - Value()
//-------------------------------------------------------------------------

void VanGui::Value(const char* prefix, bool b)
{
    Text("%s: %s", prefix, (b ? "true" : "false"));
}

void VanGui::Value(const char* prefix, int v)
{
    Text("%s: %d", prefix, v);
}

void VanGui::Value(const char* prefix, unsigned int v)
{
    Text("%s: %d", prefix, v);
}

void VanGui::Value(const char* prefix, float v, const char* float_format)
{
    if (float_format)
    {
        char fmt[64];
        VanFormatString(fmt, VAN_COUNTOF(fmt), "%%s: %s", float_format);
        Text(fmt, prefix, v);
    }
    else
    {
        Text("%s: %.3f", prefix, v);
    }
}

//-------------------------------------------------------------------------
// [SECTION] MenuItem, BeginMenu, EndMenu, etc.
//-------------------------------------------------------------------------
// - VanGuiMenuColumns [Internal]
// - BeginMenuBar()
// - EndMenuBar()
// - BeginMainMenuBar()
// - EndMainMenuBar()
// - BeginMenu()
// - EndMenu()
// - MenuItemEx() [Internal]
// - MenuItem()
//-------------------------------------------------------------------------

// Helpers for internal use
void VanGuiMenuColumns::Update(float spacing, bool window_reappearing)
{
    if (window_reappearing)
        memset(Widths, 0, sizeof(Widths));
    Spacing = static_cast<VanU16>(spacing);
    CalcNextTotalWidth(true);
    memset(Widths, 0, sizeof(Widths));
    TotalWidth = NextTotalWidth;
    NextTotalWidth = 0;
}

void VanGuiMenuColumns::CalcNextTotalWidth(bool update_offsets)
{
    VanU16 offset = 0;
    bool want_spacing = false;
    for (int i = 0; i < VAN_COUNTOF(Widths); i++)
    {
        VanU16 width = Widths[i];
        if (want_spacing && width > 0)
            offset += Spacing;
        want_spacing |= (width > 0);
        if (update_offsets)
        {
            if (i == 1) { OffsetLabel = offset; }
            if (i == 2) { OffsetShortcut = offset; }
            if (i == 3) { OffsetMark = offset; }
        }
        offset += width;
    }
    NextTotalWidth = offset;
}

float VanGuiMenuColumns::DeclColumns(float w_icon, float w_label, float w_shortcut, float w_mark)
{
    Widths[0] = VanMax(Widths[0], static_cast<VanU16>(w_icon));
    Widths[1] = VanMax(Widths[1], static_cast<VanU16>(w_label));
    Widths[2] = VanMax(Widths[2], static_cast<VanU16>(w_shortcut));
    Widths[3] = VanMax(Widths[3], static_cast<VanU16>(w_mark));
    CalcNextTotalWidth(false);
    return static_cast<float>(VanMax(TotalWidth, NextTotalWidth));
}

// FIXME: Provided a rectangle perhaps e.g. a BeginMenuBarEx() could be used anywhere..
// Currently the main responsibility of this function being to setup clip-rect + horizontal layout + menu navigation layer.
// Ideally we also want this to be responsible for claiming space out of the main window scrolling rectangle, in which case VanGuiWindowFlags_MenuBar will become unnecessary.
// Then later the same system could be used for multiple menu-bars, scrollbars, side-bars.
bool VanGui::BeginMenuBar()
{
    VanGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;
    if (!(window->Flags & VanGuiWindowFlags_MenuBar))
        return false;

    VAN_ASSERT(!window->DC.MenuBarAppending);
    BeginGroup(); // Backup position on layer 0 // FIXME: Misleading to use a group for that backup/restore
    PushID("##MenuBar");

    // We don't clip with current window clipping rectangle as it is already set to the area below. However we clip with window full rect.
    // We remove 1 worth of rounding to Max.x to that text in long menus and small windows don't tend to display over the lower-right rounded area, which looks particularly glitchy.
    const float border_top = VanMax(window->WindowBorderSize * 0.5f - window->TitleBarHeight, 0.0f);
    VanRect bar_rect = window->MenuBarRect();
    VanRect clip_rect(VAN_ROUND(bar_rect.Min.x + window->WindowBorderSize * 0.5f), VAN_ROUND(bar_rect.Min.y + border_top), VAN_ROUND(VanMax(bar_rect.Min.x, bar_rect.Max.x - VanMax(window->WindowRounding, window->WindowBorderSize * 0.5f))), VAN_ROUND(bar_rect.Max.y));
    clip_rect.ClipWith(window->OuterRectClipped);
    PushClipRect(clip_rect.Min, clip_rect.Max, false);

    // We overwrite CursorMaxPos because BeginGroup sets it to CursorPos (essentially the .EmitItem hack in EndMenuBar() would need something analogous here, maybe a BeginGroupEx() with flags).
    window->DC.CursorPos = window->DC.CursorMaxPos = VanVec2(bar_rect.Min.x + window->DC.MenuBarOffset.x, bar_rect.Min.y + window->DC.MenuBarOffset.y);
    window->DC.LayoutType = VanGuiLayoutType_Horizontal;
    window->DC.IsSameLine = false;
    window->DC.NavLayerCurrent = VanGuiNavLayer_Menu;
    window->DC.MenuBarAppending = true;
    AlignTextToFramePadding();
    return true;
}

void VanGui::EndMenuBar()
{
    VanGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return;
    VanGuiContext& g = *GVanGui;

    VAN_MSVC_WARNING_SUPPRESS(6011); // Static Analysis false positive "warning C6011: Dereferencing nullptr pointer 'window'"
    VAN_ASSERT(window->Flags & VanGuiWindowFlags_MenuBar);
    VAN_ASSERT(window->DC.MenuBarAppending);

    // Nav: When a move request within one of our child menu failed, capture the request to navigate among our siblings.
    if (NavMoveRequestButNoResultYet() && (g.NavMoveDir == VanGuiDir_Left || g.NavMoveDir == VanGuiDir_Right) && (g.NavWindow->Flags & VanGuiWindowFlags_ChildMenu))
    {
        // Try to find out if the request is for one of our child menu
        VanGuiWindow* nav_earliest_child = g.NavWindow;
        while (nav_earliest_child->ParentWindow && (nav_earliest_child->ParentWindow->Flags & VanGuiWindowFlags_ChildMenu))
            nav_earliest_child = nav_earliest_child->ParentWindow;
        if (nav_earliest_child->ParentWindow == window && nav_earliest_child->DC.ParentLayoutType == VanGuiLayoutType_Horizontal && (g.NavMoveFlags & VanGuiNavMoveFlags_Forwarded) == 0)
        {
            // To do so we claim focus back, restore NavId and then process the movement request for yet another frame.
            // This involve a one-frame delay which isn't very problematic in this situation. We could remove it by scoring in advance for multiple window (probably not worth bothering)
            const VanGuiNavLayer layer = VanGuiNavLayer_Menu;
            VAN_ASSERT(window->DC.NavLayersActiveMaskNext & (1 << layer)); // Sanity check (FIXME: Seems unnecessary)
            FocusWindow(window);
            SetNavID(window->NavLastIds[layer], layer, 0, window->NavRectRel[layer]);
            // FIXME-NAV: How to deal with this when not using g.IO.ConfigNavCursorVisibleAuto?
            if (g.NavCursorVisible)
            {
                g.NavCursorVisible = false; // Hide nav cursor for the current frame so we don't see the intermediary selection. Will be set again
                g.NavCursorHideFrames = 2;
            }
            g.NavHighlightItemUnderNav = g.NavMousePosDirty = true;
            NavMoveRequestForward(g.NavMoveDir, g.NavMoveClipDir, g.NavMoveFlags, g.NavMoveScrollFlags); // Repeat
        }
    }
    else
    {
        NavMoveRequestTryWrapping(window, VanGuiNavMoveFlags_WrapX);
    }

    PopClipRect();
    PopID();
    VAN_MSVC_WARNING_SUPPRESS(6011); // Static Analysis false positive "warning C6011: Dereferencing nullptr pointer 'window'"
    window->DC.MenuBarOffset.x = window->DC.CursorPos.x - window->Pos.x; // Save horizontal position so next append can reuse it. This is kinda equivalent to a per-layer CursorPos.

    // FIXME: Extremely confusing, cleanup by (a) working on WorkRect stack system (b) not using a Group confusingly here.
    VanGuiGroupData& group_data = g.GroupStack.back();
    group_data.EmitItem = false;
    VanVec2 restore_cursor_max_pos = group_data.BackupCursorMaxPos;
    window->DC.IdealMaxPos.x = VanMax(window->DC.IdealMaxPos.x, window->DC.CursorMaxPos.x - window->Scroll.x); // Convert ideal extents for scrolling layer equivalent.
    EndGroup(); // Restore position on layer 0 // FIXME: Misleading to use a group for that backup/restore
    window->DC.LayoutType = VanGuiLayoutType_Vertical;
    window->DC.IsSameLine = false;
    window->DC.NavLayerCurrent = VanGuiNavLayer_Main;
    window->DC.MenuBarAppending = false;
    window->DC.CursorMaxPos = restore_cursor_max_pos;
}

// Important: calling order matters!
// FIXME: Somehow overlapping with docking tech.
// FIXME: The "rect-cut" aspect of this could be formalized into a lower-level helper (rect-cut: https://halt.software/dead-simple-layouts)
bool VanGui::BeginViewportSideBar(const char* name, VanGuiViewport* viewport_p, VanGuiDir dir, float axis_size, VanGuiWindowFlags window_flags)
{
    VAN_ASSERT(dir != VanGuiDir_None);

    VanGuiWindow* bar_window = FindWindowByName(name);
    if (bar_window == nullptr || bar_window->BeginCount == 0)
    {
        // Calculate and set window size/position
        VanGuiViewportP* viewport = static_cast<VanGuiViewportP*>(viewport_p ? viewport_p : GetMainViewport());
        VanRect avail_rect = viewport->GetWorkRect();
        VanGuiAxis axis = (dir == VanGuiDir_Up || dir == VanGuiDir_Down) ? VanGuiAxis_Y : VanGuiAxis_X;
        VanVec2 pos = avail_rect.Min;
        if (dir == VanGuiDir_Right || dir == VanGuiDir_Down)
            pos[axis] = avail_rect.Max[axis] - axis_size;
        VanVec2 size = avail_rect.GetSize();
        size[axis] = axis_size;
        SetNextWindowPos(pos);
        SetNextWindowSize(size);

        // Report our size into work area (for next frame) using actual window size
        if (dir == VanGuiDir_Up || dir == VanGuiDir_Left)
            viewport->BuildWorkInsetMin[axis] += axis_size;
        else if (dir == VanGuiDir_Down || dir == VanGuiDir_Right)
            viewport->BuildWorkInsetMax[axis] += axis_size;
    }

    window_flags |= VanGuiWindowFlags_NoTitleBar | VanGuiWindowFlags_NoResize | VanGuiWindowFlags_NoMove;
    PushStyleVar(VanGuiStyleVar_WindowRounding, 0.0f);
    PushStyleVar(VanGuiStyleVar_WindowMinSize, VanVec2(0, 0)); // Lift normal size constraint
    bool is_open = Begin(name, nullptr, window_flags);
    PopStyleVar(2);

    return is_open;
}

bool VanGui::BeginMainMenuBar()
{
    VanGuiContext& g = *GVanGui;
    VanGuiViewportP* viewport = static_cast<VanGuiViewportP*>(GetMainViewport());

    // For the main menu bar, which cannot be moved, we honor g.Style.DisplaySafeAreaPadding to ensure text can be visible on a TV set.
    // FIXME: This could be generalized as an opt-in way to clamp window->DC.CursorStartPos to avoid SafeArea?
    // FIXME: Consider removing support for safe area down the line... it's messy. Nowadays consoles have support for TV calibration in OS settings.
    g.NextWindowData.MenuBarOffsetMinVal = VanVec2(g.Style.DisplaySafeAreaPadding.x, VanMax(g.Style.DisplaySafeAreaPadding.y - g.Style.FramePadding.y, 0.0f));
    VanGuiWindowFlags window_flags = VanGuiWindowFlags_NoScrollbar | VanGuiWindowFlags_NoSavedSettings | VanGuiWindowFlags_MenuBar;
    float height = GetFrameHeight();
    bool is_open = BeginViewportSideBar("##MainMenuBar", viewport, VanGuiDir_Up, height, window_flags);
    g.NextWindowData.MenuBarOffsetMinVal = VanVec2(0.0f, 0.0f);
    if (!is_open)
    {
        End();
        return false;
    }

    // Temporarily disable _NoSavedSettings, in the off-chance that tables or child windows submitted within the menu-bar may want to use settings. (#8356)
    g.CurrentWindow->Flags &= ~VanGuiWindowFlags_NoSavedSettings;
    (void)(BeginMenuBar());
    return is_open;
}

void VanGui::EndMainMenuBar()
{
    VanGuiContext& g = *GVanGui;
    VAN_ASSERT_USER_ERROR_RET(g.CurrentWindow->DC.MenuBarAppending, "Calling EndMainMenuBar() not from a menu-bar!"); // Not technically testing that it is the main menu bar

    EndMenuBar();
    g.CurrentWindow->Flags |= VanGuiWindowFlags_NoSavedSettings; // Restore _NoSavedSettings (#8356)

    // When the user has left the menu layer (typically: closed menus through activation of an item), we restore focus to the previous window
    // FIXME: With this strategy we won't be able to restore a nullptr focus.
    if (g.CurrentWindow == g.NavWindow && g.NavLayer == VanGuiNavLayer_Main && !g.NavAnyRequest && g.ActiveId == 0)
        FocusTopMostWindowUnderOne(g.NavWindow, nullptr, nullptr, VanGuiFocusRequestFlags_UnlessBelowModal | VanGuiFocusRequestFlags_RestoreFocusedChild);

    End();
}

static bool IsRootOfOpenMenuSet()
{
    VanGuiContext& g = *GVanGui;
    VanGuiWindow* window = g.CurrentWindow;
    if ((g.OpenPopupStack.Size <= g.BeginPopupStack.Size) || (window->Flags & VanGuiWindowFlags_ChildMenu))
        return false;

    // Initially we used 'upper_popup->OpenParentId == window->IDStack.back()' to differentiate multiple menu sets from each others
    // (e.g. inside menu bar vs loose menu items) based on parent ID.
    // This would however prevent the use of e.g. PushID() user code submitting menus.
    // Previously this worked between popup and a first child menu because the first child menu always had the _ChildWindow flag,
    // making hovering on parent popup possible while first child menu was focused - but this was generally a bug with other side effects.
    // Instead we don't treat Popup specifically (in order to consistently support menu features in them), maybe the first child menu of a Popup
    // doesn't have the _ChildWindow flag, and we rely on this IsRootOfOpenMenuSet() check to allow hovering between root window/popup and first child menu.
    // In the end, lack of ID check made it so we could no longer differentiate between separate menu sets. To compensate for that, we at least check parent window nav layer.
    // This fixes the most common case of menu opening on hover when moving between window content and menu bar. Multiple different menu sets in same nav layer would still
    // open on hover, but that should be a lesser problem, because if such menus are close in proximity in window content then it won't feel weird and if they are far apart
    // it likely won't be a problem anyone runs into.
    const VanGuiPopupData* upper_popup = &g.OpenPopupStack[g.BeginPopupStack.Size];
    if (window->DC.NavLayerCurrent != upper_popup->ParentNavLayer)
        return false;
    return upper_popup->Window && (upper_popup->Window->Flags & VanGuiWindowFlags_ChildMenu) && VanGui::IsWindowChildOf(upper_popup->Window, window, true);
}

bool VanGui::BeginMenuEx(const char* label, const char* icon, bool enabled)
{
    VanGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    VanGuiContext& g = *GVanGui;
    const VanGuiStyle& style = g.Style;
    const VanGuiID id = window->GetID(label);
    bool menu_is_open = IsPopupOpen(id, VanGuiPopupFlags_None);

    // Sub-menus are ChildWindow so that mouse can be hovering across them (otherwise top-most popup menu would steal focus and not allow hovering on parent menu)
    // The first menu in a hierarchy isn't so hovering doesn't get across (otherwise e.g. resizing borders with VanGuiButtonFlags_FlattenChildren would react), but top-most BeginMenu() will bypass that limitation.
    VanGuiWindowFlags window_flags = VanGuiWindowFlags_ChildMenu | VanGuiWindowFlags_AlwaysAutoResize | VanGuiWindowFlags_NoMove | VanGuiWindowFlags_NoTitleBar | VanGuiWindowFlags_NoSavedSettings | VanGuiWindowFlags_NoNavFocus;
    if (window->Flags & VanGuiWindowFlags_ChildMenu)
        window_flags |= VanGuiWindowFlags_ChildWindow;

    // If a menu with same the ID was already submitted, we will append to it, matching the behavior of Begin().
    // We are relying on a O(N) search - so O(N log N) over the frame - which seems like the most efficient for the expected small amount of BeginMenu() calls per frame.
    // If somehow this is ever becoming a problem we can switch to use e.g. VanGuiStorage mapping key to last frame used.
    if (g.MenusIdSubmittedThisFrame.contains(id))
    {
        if (menu_is_open)
            menu_is_open = BeginPopupMenuEx(id, label, window_flags); // menu_is_open can be 'false' when the popup is completely clipped (e.g. zero size display)
        else
            g.NextWindowData.ClearFlags();          // we behave like Begin() and need to consume those values
        return menu_is_open;
    }

    // Tag menu as used. Next time BeginMenu() with same ID is called it will append to existing menu
    g.MenusIdSubmittedThisFrame.push_back(id);

    const char* label_end = FindRenderedTextEnd(label);
    VanVec2 label_size = CalcTextSize(label, label_end, false);

    // Odd hack to allow hovering across menus of a same menu-set (otherwise we wouldn't be able to hover parent without always being a Child window)
    // This is only done for items for the menu set and not the full parent window.
    const bool menuset_is_open = IsRootOfOpenMenuSet();
    if (menuset_is_open)
        PushItemFlag(VanGuiItemFlags_NoWindowHoverableCheck, true);

    // The reference position stored in popup_pos will be used by Begin() to find a suitable position for the child menu,
    // However the final position is going to be different! It is chosen by FindBestWindowPosForPopup().
    // e.g. Menus tend to overlap each other horizontally to amplify relative Z-ordering.
    VanVec2 popup_pos;
    VanVec2 pos = window->DC.CursorPos;
    PushID(label);
    if (!enabled)
        BeginDisabled();

    bool pressed;

    const VanGuiSelectableFlags selectable_flags = VanGuiSelectableFlags_NoAutoClosePopups | static_cast<VanGuiSelectableFlags>(VanGuiSelectableFlags_SelectOnClick);
    VanGuiMenuColumns* offsets = &window->DC.MenuColumns;
    if (window->DC.LayoutType == VanGuiLayoutType_Horizontal)
    {
        // Menu inside a horizontal menu bar
        // Selectable extend their highlight by half ItemSpacing in each direction.
        // For ChildMenu, the popup position will be overwritten by the call to FindBestWindowPosForPopup() in Begin()
        window->DC.CursorPos.x += VAN_TRUNC(style.ItemSpacing.x * 0.5f);
        PushStyleVarX(VanGuiStyleVar_ItemSpacing, style.ItemSpacing.x * 2.0f);
        VanVec2 text_pos(window->DC.CursorPos.x + offsets->OffsetLabel, pos.y + window->DC.CurrLineTextBaseOffset);
        pressed = Selectable("", menu_is_open, selectable_flags, label_size);
        LogSetNextTextDecoration("[", "]");
        RenderText(text_pos, label, label_end, false);
        PopStyleVar();
        window->DC.CursorPos.x += VAN_TRUNC(style.ItemSpacing.x * (-1.0f + 0.5f)); // -1 spacing to compensate the spacing added when Selectable() did a SameLine(). It would also work to call SameLine() ourselves after the PopStyleVar().
        popup_pos = VanVec2(pos.x - 1.0f - VAN_TRUNC(style.ItemSpacing.x * 0.5f), text_pos.y - style.FramePadding.y + window->MenuBarHeight);
    }
    else
    {
        // Menu inside a regular/vertical menu
        // (In a typical menu window where all items are BeginMenu() or MenuItem() calls, extra_w will always be 0.0f.
        //  Only when they are other items sticking out we're going to add spacing, yet only register minimum width into the layout system.)
        float icon_w = (icon && icon[0]) ? CalcTextSize(icon, nullptr).x : 0.0f;
        float checkmark_w = VAN_TRUNC(g.FontSize * 1.20f);
        float min_w = offsets->DeclColumns(icon_w, label_size.x, 0.0f, checkmark_w); // Feedback to next frame
        float extra_w = VanMax(0.0f, GetContentRegionAvail().x - min_w);
        VanVec2 text_pos(window->DC.CursorPos.x, pos.y + window->DC.CurrLineTextBaseOffset);
        pressed = Selectable("", menu_is_open, selectable_flags | VanGuiSelectableFlags_SpanAvailWidth, VanVec2(min_w, label_size.y));
        LogSetNextTextDecoration("", ">");
        RenderText(VanVec2(text_pos.x + offsets->OffsetLabel, text_pos.y), label, label_end, false);
        if (icon_w > 0.0f)
            RenderText(VanVec2(text_pos.x + offsets->OffsetIcon, text_pos.y), icon);
        RenderArrow(window->DrawList, VanVec2(text_pos.x + offsets->OffsetMark + extra_w + g.FontSize * 0.30f, text_pos.y), GetColorU32(VanGuiCol_Text), VanGuiDir_Right);
        popup_pos = VanVec2(pos.x, text_pos.y - style.WindowPadding.y);
    }
    if (!enabled)
        EndDisabled();

    // Once dragged, release ActiveId + key ownership. This is to allow the idiom of mouse down a menu, dragging elsewhere, up on some other MenuItem(). (#8233, #9394)
    // Could move logic into lower-level VanGuiButtonFlags_AutoReleaseActiveId + VanGuiButtonFlags_AutoReleaseKeyOwner? Easier once we get rid of the Selectable() middle-man here.
    if (g.ActiveId == id && g.HoveredId != id && g.ActiveIdSource == VanGuiInputSource_Mouse && IsMouseDragging(0))
    {
        ClearActiveID();
        SetKeyOwner(VanGuiKey_MouseLeft, VanGuiKeyOwner_NoOwner);
    }

    const bool hovered = (g.HoveredId == id) && enabled && !g.NavHighlightItemUnderNav;
    if (menuset_is_open)
        PopItemFlag();

    bool want_open = false;
    bool want_open_nav_init = false;
    bool want_close = false;
    if (window->DC.LayoutType == VanGuiLayoutType_Vertical) // (window->Flags & (VanGuiWindowFlags_Popup|VanGuiWindowFlags_ChildMenu))
    {
        // Close menu when not hovering it anymore unless we are moving roughly in the direction of the menu
        // Implement http://bjk5.com/post/44698559168/breaking-down-amazons-mega-dropdown to avoid using timers, so menus feels more reactive.
        bool moving_toward_child_menu = false;
        VanGuiPopupData* child_popup = (g.BeginPopupStack.Size < g.OpenPopupStack.Size) ? &g.OpenPopupStack[g.BeginPopupStack.Size] : nullptr; // Popup candidate (testing below)
        VanGuiWindow* child_menu_window = (child_popup && child_popup->Window && child_popup->Window->ParentWindow == window) ? child_popup->Window : nullptr;
        if (g.HoveredWindow == window && child_menu_window != nullptr)
        {
            const float ref_unit = g.FontSize; // FIXME-DPI
            const float child_dir = (window->Pos.x < child_menu_window->Pos.x) ? 1.0f : -1.0f;
            const VanRect next_window_rect = child_menu_window->Rect();
            VanVec2 ta = (g.IO.MousePos - g.IO.MouseDelta);
            VanVec2 tb = (child_dir > 0.0f) ? next_window_rect.GetTL() : next_window_rect.GetTR();
            VanVec2 tc = (child_dir > 0.0f) ? next_window_rect.GetBL() : next_window_rect.GetBR();
            const float pad_farmost_h = VanClamp(VanFabs(ta.x - tb.x) * 0.30f, ref_unit * 0.5f, ref_unit * 2.5f); // Add a bit of extra slack.
            ta.x += child_dir * -0.5f;
            tb.x += child_dir * ref_unit;
            tc.x += child_dir * ref_unit;
            tb.y = ta.y + VanMax((tb.y - pad_farmost_h) - ta.y, -ref_unit * 8.0f); // Triangle has maximum height to limit the slope and the bias toward large sub-menus
            tc.y = ta.y + VanMin((tc.y + pad_farmost_h) - ta.y, +ref_unit * 8.0f);
            moving_toward_child_menu = VanTriangleContainsPoint(ta, tb, tc, g.IO.MousePos);
            //GetForegroundDrawList()->AddTriangleFilled(ta, tb, tc, moving_toward_child_menu ? VAN_COL32(0,128,0,128) : VAN_COL32(128,0,0,128)); // [DEBUG]
        }

        // The 'HovereWindow == window' check creates an inconsistency (e.g. moving away from menu slowly tends to hit same window, whereas moving away fast does not)
        // But we also need to not close the top-menu menu when moving over void. Perhaps we should extend the triangle check to a larger polygon.
        // (Remember to test this on BeginPopup("A")->BeginMenu("B") sequence which behaves slightly differently as B isn't a Child of A and hovering isn't shared.)
        if (menu_is_open && !hovered && g.HoveredWindow == window && !moving_toward_child_menu && !g.NavHighlightItemUnderNav && g.ActiveId == 0)
            want_close = true;

        // Open
        // (note: at this point 'hovered' actually includes the NavDisableMouseHover == false test)
        if (!menu_is_open && pressed) // Click/activate to open
            want_open = true;
        else if (!menu_is_open && hovered && !moving_toward_child_menu) // Hover to open
            want_open = true;
        else if (!menu_is_open && hovered && g.HoveredIdTimer >= 0.30f && g.MouseStationaryTimer >= 0.30f) // Hover to open (timer fallback)
            want_open = true;
        if (g.NavId == id && g.NavMoveDir == VanGuiDir_Right) // Nav-Right to open
        {
            want_open = want_open_nav_init = true;
            NavMoveRequestCancel();
            SetNavCursorVisibleAfterMove();
        }
    }
    else
    {
        // Menu bar
        if (menu_is_open && pressed && menuset_is_open) // Click an open menu again to close it
        {
            want_close = true;
            want_open = menu_is_open = false;
        }
        else if (pressed || (hovered && menuset_is_open && !menu_is_open)) // First click to open, then hover to open others
        {
            want_open = true;
        }
        else if (g.NavId == id && g.NavMoveDir == VanGuiDir_Down) // Nav-Down to open
        {
            want_open = true;
            NavMoveRequestCancel();
        }
    }

    if (!enabled) // explicitly close if an open menu becomes disabled, facilitate users code a lot in pattern such as 'if (BeginMenu("options", has_object)) { ..use object.. }'
        want_close = true;
    if (want_close && IsPopupOpen(id, VanGuiPopupFlags_None))
        ClosePopupToLevel(g.BeginPopupStack.Size, true);

    VANGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags | VanGuiItemStatusFlags_Openable | (menu_is_open ? VanGuiItemStatusFlags_Opened : 0));
    PopID();

    if (g.ActiveId == id && want_open)
        g.ActiveIdNoClearOnFocusLoss = true;

    if (want_open && !menu_is_open && g.OpenPopupStack.Size > g.BeginPopupStack.Size)
    {
        // Don't reopen/recycle same menu level in the same frame if it is a different menu ID, first close the other menu and yield for a frame.
        OpenPopup(label);
    }
    else if (want_open)
    {
        menu_is_open = true;
        OpenPopup(label, VanGuiPopupFlags_NoReopen);// | (want_open_nav_init ? VanGuiPopupFlags_NoReopenAlwaysNavInit : 0));
    }

    if (menu_is_open)
    {
        VanGuiLastItemData last_item_in_parent = g.LastItemData;
        SetNextWindowPos(popup_pos, VanGuiCond_Always);                  // Note: misleading: the value will serve as reference for FindBestWindowPosForPopup(), not actual pos.
        PushStyleVar(VanGuiStyleVar_ChildRounding, style.PopupRounding); // First level will use _PopupRounding, subsequent will use _ChildRounding
        menu_is_open = BeginPopupMenuEx(id, label, window_flags); // menu_is_open may be 'false' when the popup is completely clipped (e.g. zero size display)
        PopStyleVar();
        if (menu_is_open)
        {
            // Implement what VanGuiPopupFlags_NoReopenAlwaysNavInit would do:
            // Perform an init request in the case the popup was already open (via a previous mouse hover)
            if (want_open && want_open_nav_init && !g.NavInitRequest)
            {
                FocusWindow(g.CurrentWindow, VanGuiFocusRequestFlags_UnlessBelowModal);
                NavInitWindow(g.CurrentWindow, false);
            }

            // Restore LastItemData so IsItemXXXX functions can work after BeginMenu()/EndMenu()
            // (This fixes using IsItemClicked() and IsItemHovered(), but IsItemHovered() also relies on its support for VanGuiItemFlags_NoWindowHoverableCheck)
            g.LastItemData = last_item_in_parent;
            if (g.HoveredWindow == window)
                g.LastItemData.StatusFlags |= VanGuiItemStatusFlags_HoveredWindow;
        }
    }
    else
    {
        g.NextWindowData.ClearFlags(); // We behave like Begin() and need to consume those values
    }

    return menu_is_open;
}

bool VanGui::BeginMenu(const char* label, bool enabled)
{
    return BeginMenuEx(label, nullptr, enabled);
}

void VanGui::EndMenu()
{
    // Nav: When a left move request our menu failed, close ourselves.
    VanGuiContext& g = *GVanGui;
    VanGuiWindow* window = g.CurrentWindow;
    VAN_ASSERT_USER_ERROR_RET((window->Flags & (VanGuiWindowFlags_Popup | VanGuiWindowFlags_ChildMenu)) == (VanGuiWindowFlags_Popup | VanGuiWindowFlags_ChildMenu), "Calling EndMenu() in wrong window!");

    VanGuiWindow* parent_window = window->ParentWindow;  // Should always be != nullptr is we passed assert.
    if (window->BeginCount == window->BeginCountPreviousFrame)
        if (g.NavMoveDir == VanGuiDir_Left && NavMoveRequestButNoResultYet())
            if (g.NavWindow && (g.NavWindow->RootWindowForNav == window) && parent_window->DC.LayoutType == VanGuiLayoutType_Vertical)
            {
                ClosePopupToLevel(g.BeginPopupStack.Size - 1, true);
                NavMoveRequestCancel();
            }

    EndPopup();
}

bool VanGui::MenuItemEx(const char* label, const char* icon, const char* shortcut, bool selected, bool enabled)
{
    VanGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    VanGuiContext& g = *GVanGui;
    VanGuiStyle& style = g.Style;
    VanVec2 pos = window->DC.CursorPos;
    const char* label_end = FindRenderedTextEnd(label);
    VanVec2 label_size = CalcTextSize(label, label_end, false);

    // See BeginMenuEx() for comments about this.
    const bool menuset_is_open = IsRootOfOpenMenuSet();
    if (menuset_is_open)
        PushItemFlag(VanGuiItemFlags_NoWindowHoverableCheck, true);

    // We've been using the equivalent of VanGuiSelectableFlags_SetNavIdOnHover on all Selectable() since early Nav system days (commit 43ee5d73),
    // but I am unsure whether this should be kept at all. For now moved it to be an opt-in feature used by menus only.
    bool pressed;
    PushID(label);
    if (!enabled)
        BeginDisabled();

    // We use VanGuiSelectableFlags_NoSetKeyOwner to allow down on one menu item, move, up on another.
    const VanGuiSelectableFlags selectable_flags = static_cast<VanGuiSelectableFlags>(VanGuiSelectableFlags_SelectOnRelease) | static_cast<VanGuiSelectableFlags>(VanGuiSelectableFlags_SetNavIdOnHover);
    VanGuiMenuColumns* offsets = &window->DC.MenuColumns;
    if (window->DC.LayoutType == VanGuiLayoutType_Horizontal)
    {
        // Mimic the exact layout spacing of BeginMenu() to allow MenuItem() inside a menu bar, which is a little misleading but may be useful
        // Note that in this situation: we don't render the shortcut, we render a highlight instead of the selected tick mark.
        window->DC.CursorPos.x += VAN_TRUNC(style.ItemSpacing.x * 0.5f);
        VanVec2 text_pos(window->DC.CursorPos.x + offsets->OffsetLabel, window->DC.CursorPos.y + window->DC.CurrLineTextBaseOffset);
        PushStyleVarX(VanGuiStyleVar_ItemSpacing, style.ItemSpacing.x * 2.0f);
        pressed = Selectable("", selected, selectable_flags, VanVec2(label_size.x, 0.0f));
        PopStyleVar();
        if (g.LastItemData.StatusFlags & VanGuiItemStatusFlags_Visible)
            RenderText(text_pos, label, label_end, false);
        window->DC.CursorPos.x += VAN_TRUNC(style.ItemSpacing.x * (-1.0f + 0.5f)); // -1 spacing to compensate the spacing added when Selectable() did a SameLine(). It would also work to call SameLine() ourselves after the PopStyleVar().
    }
    else
    {
        // Menu item inside a vertical menu
        // (In a typical menu window where all items are BeginMenu() or MenuItem() calls, extra_w will always be 0.0f.
        //  Only when they are other items sticking out we're going to add spacing, yet only register minimum width into the layout system.)
        float icon_w = (icon && icon[0]) ? CalcTextSize(icon, nullptr).x : 0.0f;
        float shortcut_w = (shortcut && shortcut[0]) ? CalcTextSize(shortcut, nullptr).x : 0.0f;
        float checkmark_w = VAN_TRUNC(g.FontSize * 1.20f);
        float min_w = offsets->DeclColumns(icon_w, label_size.x, shortcut_w, checkmark_w); // Feedback for next frame
        float stretch_w = VanMax(0.0f, GetContentRegionAvail().x - min_w);
        VanVec2 text_pos(pos.x, pos.y + window->DC.CurrLineTextBaseOffset);
        pressed = Selectable("", false, selectable_flags | VanGuiSelectableFlags_SpanAvailWidth, VanVec2(min_w, label_size.y));
        if (g.LastItemData.StatusFlags & VanGuiItemStatusFlags_Visible)
        {
            RenderText(text_pos + VanVec2(offsets->OffsetLabel, 0.0f), label, label_end, false);
            if (icon_w > 0.0f)
                RenderText(text_pos + VanVec2(offsets->OffsetIcon, 0.0f), icon);
            if (shortcut_w > 0.0f)
            {
                PushStyleColor(VanGuiCol_Text, style.Colors[VanGuiCol_TextDisabled]);
                LogSetNextTextDecoration("(", ")");
                RenderText(text_pos + VanVec2(offsets->OffsetShortcut + stretch_w, 0.0f), shortcut, nullptr, false);
                PopStyleColor();
            }
            if (selected)
                RenderCheckMark(window->DrawList, text_pos + VanVec2(offsets->OffsetMark + stretch_w + g.FontSize * 0.40f, g.FontSize * 0.134f * 0.5f), GetColorU32(VanGuiCol_Text), g.FontSize * 0.866f);
        }
    }

    // Once dragged, release ActiveId + key ownership. This is to allow the idiom of mouse down a menu, dragging elsewhere, up on some other MenuItem(). (#8233, #9394)
    // Could move logic into lower-level VanGuiButtonFlags_AutoReleaseActiveId + VanGuiButtonFlags_AutoReleaseKeyOwner? Easier once we get rid of the Selectable() middle-man here.
    const VanGuiID id = g.LastItemData.ID;
    if (g.ActiveId == id && g.HoveredId != id && g.ActiveIdSource == VanGuiInputSource_Mouse && IsMouseDragging(0))
    {
        ClearActiveID();
        SetKeyOwner(VanGuiKey_MouseLeft, VanGuiKeyOwner_NoOwner);
    }


    VANGUI_TEST_ENGINE_ITEM_INFO(g.LastItemData.ID, label, g.LastItemData.StatusFlags | VanGuiItemStatusFlags_Checkable | (selected ? VanGuiItemStatusFlags_Checked : 0));
    if (!enabled)
        EndDisabled();
    PopID();
    if (menuset_is_open)
        PopItemFlag();

    return pressed;
}

bool VanGui::MenuItem(const char* label, const char* shortcut, bool selected, bool enabled)
{
    return MenuItemEx(label, nullptr, shortcut, selected, enabled);
}

bool VanGui::MenuItem(const char* label, const char* shortcut, bool* p_selected, bool enabled)
{
    if (MenuItemEx(label, nullptr, shortcut, p_selected ? *p_selected : false, enabled))
    {
        if (p_selected)
            *p_selected = !*p_selected;
        return true;
    }
    return false;
}

//-------------------------------------------------------------------------
// [SECTION] Widgets: BeginTabBar, EndTabBar, etc.
//-------------------------------------------------------------------------
// - BeginTabBar()
// - BeginTabBarEx() [Internal]
// - EndTabBar()
// - TabBarLayout() [Internal]
// - TabBarCalcTabID() [Internal]
// - TabBarCalcMaxTabWidth() [Internal]
// - TabBarFindTabById() [Internal]
// - TabBarFindTabByOrder() [Internal]
// - TabBarGetCurrentTab() [Internal]
// - TabBarGetTabName() [Internal]
// - TabBarRemoveTab() [Internal]
// - TabBarCloseTab() [Internal]
// - TabBarScrollClamp() [Internal]
// - TabBarScrollToTab() [Internal]
// - TabBarQueueFocus() [Internal]
// - TabBarQueueReorder() [Internal]
// - TabBarProcessReorderFromMousePos() [Internal]
// - TabBarProcessReorder() [Internal]
// - TabBarScrollingButtons() [Internal]
// - TabBarTabListPopupButton() [Internal]
//-------------------------------------------------------------------------

struct VanGuiTabBarSection
{
    int                 TabCount;               // Number of tabs in this section.
    float               Width;                  // Sum of width of tabs in this section (after shrinking down)
    float               WidthAfterShrinkMinWidth;
    float               Spacing;                // Horizontal spacing at the end of the section.

    VanGuiTabBarSection() { memset(static_cast<void*>(this), 0, sizeof(*this)); }
};

namespace VanGui
{
    static void             TabBarLayout(VanGuiTabBar* tab_bar);
    static VanU32            TabBarCalcTabID(VanGuiTabBar* tab_bar, const char* label, VanGuiWindow* docked_window);
    static float            TabBarCalcMaxTabWidth();
    static float            TabBarScrollClamp(VanGuiTabBar* tab_bar, float scrolling);
    static void             TabBarScrollToTab(VanGuiTabBar* tab_bar, VanGuiID tab_id, VanGuiTabBarSection* sections);
    static VanGuiTabItem*    TabBarScrollingButtons(VanGuiTabBar* tab_bar);
    static VanGuiTabItem*    TabBarTabListPopupButton(VanGuiTabBar* tab_bar);
}

VanGuiTabBar::VanGuiTabBar()
{
    memset(static_cast<void*>(this), 0, sizeof(*this));
    CurrFrameVisible = PrevFrameVisible = -1;
    LastTabItemIdx = -1;
}

static inline int TabItemGetSectionIdx(const VanGuiTabItem* tab)
{
    return (tab->Flags & VanGuiTabItemFlags_Leading) ? 0 : (tab->Flags & VanGuiTabItemFlags_Trailing) ? 2 : 1;
}

static int VANGUI_CDECL TabItemComparerBySection(const void* lhs, const void* rhs)
{
    const VanGuiTabItem* a = static_cast<const VanGuiTabItem*>(lhs);
    const VanGuiTabItem* b = static_cast<const VanGuiTabItem*>(rhs);
    const int a_section = TabItemGetSectionIdx(a);
    const int b_section = TabItemGetSectionIdx(b);
    if (a_section != b_section)
        return a_section - b_section;
    return static_cast<int>(a->IndexDuringLayout - b->IndexDuringLayout);
}

static int VANGUI_CDECL TabItemComparerByBeginOrder(const void* lhs, const void* rhs)
{
    const VanGuiTabItem* a = static_cast<const VanGuiTabItem*>(lhs);
    const VanGuiTabItem* b = static_cast<const VanGuiTabItem*>(rhs);
    return static_cast<int>(a->BeginOrder - b->BeginOrder);
}

static VanGuiTabBar* GetTabBarFromTabBarRef(const VanGuiPtrOrIndex& ref)
{
    VanGuiContext& g = *GVanGui;
    return ref.Ptr ? static_cast<VanGuiTabBar*>(ref.Ptr) : g.TabBars.GetByIndex(ref.Index);
}

static VanGuiPtrOrIndex GetTabBarRefFromTabBar(VanGuiTabBar* tab_bar)
{
    VanGuiContext& g = *GVanGui;
    if (g.TabBars.Contains(tab_bar))
        return VanGuiPtrOrIndex(g.TabBars.GetIndex(tab_bar));
    return VanGuiPtrOrIndex(tab_bar);
}

VanGuiTabBar* VanGui::TabBarFindByID(VanGuiID id)
{
    VanGuiContext& g = *GVanGui;
    return g.TabBars.GetByKey(id);
}

// Remove TabBar data (currently only used by TestEngine)
void    VanGui::TabBarRemove(VanGuiTabBar* tab_bar)
{
    VanGuiContext& g = *GVanGui;
    g.TabBars.Remove(tab_bar->ID, tab_bar);
}

bool    VanGui::BeginTabBar(const char* str_id, VanGuiTabBarFlags flags)
{
    VanGuiContext& g = *GVanGui;
    VanGuiWindow* window = g.CurrentWindow;
    if (window->SkipItems)
        return false;

    VanGuiID id = window->GetID(str_id);
    VanGuiTabBar* tab_bar = g.TabBars.GetOrAddByKey(id);
    VanRect tab_bar_bb = VanRect(window->DC.CursorPos.x, window->DC.CursorPos.y, window->WorkRect.Max.x, window->DC.CursorPos.y + g.FontSize + g.Style.FramePadding.y * 2);
    tab_bar->ID = id;
    tab_bar->SeparatorMinX = tab_bar_bb.Min.x - VAN_TRUNC(window->WindowPadding.x * 0.5f);
    tab_bar->SeparatorMaxX = tab_bar_bb.Max.x + VAN_TRUNC(window->WindowPadding.x * 0.5f);
    //if (g.NavWindow && IsWindowChildOf(g.NavWindow, window, false, false))
    flags |= VanGuiTabBarFlags_IsFocused;
    return BeginTabBarEx(tab_bar, tab_bar_bb, flags);
}

bool    VanGui::BeginTabBarEx(VanGuiTabBar* tab_bar, const VanRect& tab_bar_bb, VanGuiTabBarFlags flags)
{
    VanGuiContext& g = *GVanGui;
    VanGuiWindow* window = g.CurrentWindow;
    if (window->SkipItems)
        return false;

    VAN_ASSERT(tab_bar->ID != 0);
    if ((flags & VanGuiTabBarFlags_DockNode) == 0)
        PushOverrideID(tab_bar->ID);

    // Add to stack
    g.CurrentTabBarStack.push_back(GetTabBarRefFromTabBar(tab_bar));
    g.CurrentTabBar = tab_bar;
    tab_bar->Window = window;

    // Append with multiple BeginTabBar()/EndTabBar() pairs.
    tab_bar->BackupCursorPos = window->DC.CursorPos;
    if (tab_bar->CurrFrameVisible == g.FrameCount)
    {
        window->DC.CursorPos = VanVec2(tab_bar->BarRect.Min.x, tab_bar->BarRect.Max.y + tab_bar->ItemSpacingY);
        tab_bar->BeginCount++;
        return true;
    }

    // Ensure correct ordering when toggling VanGuiTabBarFlags_Reorderable flag, or when a new tab was added while being not reorderable
    if ((flags & VanGuiTabBarFlags_Reorderable) != (tab_bar->Flags & VanGuiTabBarFlags_Reorderable) || (tab_bar->TabsAddedNew && !(flags & VanGuiTabBarFlags_Reorderable)))
        VanQsort(tab_bar->Tabs.Data, tab_bar->Tabs.Size, sizeof(VanGuiTabItem), TabItemComparerByBeginOrder);
    tab_bar->TabsAddedNew = false;

    // Flags
    if ((flags & VanGuiTabBarFlags_FittingPolicyMask_) == 0)
        flags |= VanGuiTabBarFlags_FittingPolicyDefault_;

    tab_bar->Flags = flags;
    tab_bar->BarRect = tab_bar_bb;
    tab_bar->WantLayout = true; // Layout will be done on the first call to ItemTab()
    tab_bar->PrevFrameVisible = tab_bar->CurrFrameVisible;
    tab_bar->CurrFrameVisible = g.FrameCount;
    tab_bar->PrevTabsContentsHeight = tab_bar->CurrTabsContentsHeight;
    tab_bar->CurrTabsContentsHeight = 0.0f;
    tab_bar->ItemSpacingY = g.Style.ItemSpacing.y;
    tab_bar->FramePadding = g.Style.FramePadding;
    tab_bar->TabsActiveCount = 0;
    tab_bar->LastTabItemIdx = -1;
    tab_bar->BeginCount = 1;

    // Set cursor pos in a way which only be used in the off-chance the user erroneously submits item before BeginTabItem(): items will overlap
    window->DC.CursorPos = VanVec2(tab_bar->BarRect.Min.x, tab_bar->BarRect.Max.y + tab_bar->ItemSpacingY);

    // Draw separator
    // (it would be misleading to draw this in EndTabBar() suggesting that it may be drawn over tabs, as tab bar are appendable)
    const VanU32 col = GetColorU32((flags & VanGuiTabBarFlags_IsFocused) ? VanGuiCol_TabSelected : VanGuiCol_TabDimmedSelected);
    if (g.Style.TabBarBorderSize > 0.0f)
    {
        const float y = tab_bar->BarRect.Max.y;
        window->DrawList->AddRectFilled(VanVec2(tab_bar->SeparatorMinX, y - g.Style.TabBarBorderSize), VanVec2(tab_bar->SeparatorMaxX, y), col);
    }
    return true;
}

void    VanGui::EndTabBar()
{
    VanGuiContext& g = *GVanGui;
    VanGuiWindow* window = g.CurrentWindow;
    if (window->SkipItems)
        return;

    VanGuiTabBar* tab_bar = g.CurrentTabBar;
    VAN_ASSERT_USER_ERROR_RET(tab_bar != nullptr, "Mismatched BeginTabBar()/EndTabBar()!");

    // Fallback in case no TabItem have been submitted
    if (tab_bar->WantLayout)
        TabBarLayout(tab_bar);

    // Restore the last visible height if no tab is visible, this reduce vertical flicker/movement when a tabs gets removed without calling SetTabItemClosed().
    const bool tab_bar_appearing = (tab_bar->PrevFrameVisible + 1 < g.FrameCount);
    if (tab_bar->VisibleTabWasSubmitted || tab_bar->VisibleTabId == 0 || tab_bar_appearing)
    {
        tab_bar->CurrTabsContentsHeight = VanMax(window->DC.CursorPos.y - tab_bar->BarRect.Max.y, tab_bar->CurrTabsContentsHeight);
        window->DC.CursorPos.y = tab_bar->BarRect.Max.y + tab_bar->CurrTabsContentsHeight;
    }
    else
    {
        window->DC.CursorPos.y = tab_bar->BarRect.Max.y + tab_bar->PrevTabsContentsHeight;
    }
    if (tab_bar->BeginCount > 1)
        window->DC.CursorPos = tab_bar->BackupCursorPos;

    tab_bar->LastTabItemIdx = -1;
    if ((tab_bar->Flags & VanGuiTabBarFlags_DockNode) == 0)
        PopID();

    g.CurrentTabBarStack.pop_back();
    g.CurrentTabBar = g.CurrentTabBarStack.empty() ? nullptr : GetTabBarFromTabBarRef(g.CurrentTabBarStack.back());
}

// Scrolling happens only in the central section (leading/trailing sections are not scrolling)
static float TabBarCalcScrollableWidth(VanGuiTabBar* tab_bar, VanGuiTabBarSection* sections)
{
    return tab_bar->BarRect.GetWidth() - sections[0].Width - sections[2].Width - sections[1].Spacing;
}

// This is called only once a frame before by the first call to ItemTab()
// The reason we're not calling it in BeginTabBar() is to leave a chance to the user to call the SetTabItemClosed() functions.
static void VanGui::TabBarLayout(VanGuiTabBar* tab_bar)
{
    VanGuiContext& g = *GVanGui;
    tab_bar->WantLayout = false;

    // Track selected tab when resizing our parent down
    const bool scroll_to_selected_tab = (tab_bar->BarRectPrevWidth > tab_bar->BarRect.GetWidth());
    tab_bar->BarRectPrevWidth = tab_bar->BarRect.GetWidth();

    // Garbage collect by compacting list
    // Detect if we need to sort out tab list (e.g. in rare case where a tab changed section)
    int tab_dst_n = 0;
    bool need_sort_by_section = false;
    VanGuiTabBarSection sections[3]; // Layout sections: Leading, Central, Trailing
    for (int tab_src_n = 0; tab_src_n < tab_bar->Tabs.Size; tab_src_n++)
    {
        VanGuiTabItem* tab = &tab_bar->Tabs[tab_src_n];
        if (tab->LastFrameVisible < tab_bar->PrevFrameVisible || tab->WantClose)
        {
            // Remove tab
            if (tab_bar->VisibleTabId == tab->ID) { tab_bar->VisibleTabId = 0; }
            if (tab_bar->SelectedTabId == tab->ID) { tab_bar->SelectedTabId = 0; }
            if (tab_bar->NextSelectedTabId == tab->ID) { tab_bar->NextSelectedTabId = 0; }
            continue;
        }
        if (tab_dst_n != tab_src_n)
            tab_bar->Tabs[tab_dst_n] = tab_bar->Tabs[tab_src_n];

        tab = &tab_bar->Tabs[tab_dst_n];
        tab->IndexDuringLayout = static_cast<VanS16>(tab_dst_n);

        // We will need sorting if tabs have changed section (e.g. moved from one of Leading/Central/Trailing to another)
        int curr_tab_section_n = TabItemGetSectionIdx(tab);
        if (tab_dst_n > 0)
        {
            VanGuiTabItem* prev_tab = &tab_bar->Tabs[tab_dst_n - 1];
            int prev_tab_section_n = TabItemGetSectionIdx(prev_tab);
            if (curr_tab_section_n == 0 && prev_tab_section_n != 0)
                need_sort_by_section = true;
            if (prev_tab_section_n == 2 && curr_tab_section_n != 2)
                need_sort_by_section = true;
        }

        sections[curr_tab_section_n].TabCount++;
        tab_dst_n++;
    }
    if (tab_bar->Tabs.Size != tab_dst_n)
        tab_bar->Tabs.resize(tab_dst_n);

    if (need_sort_by_section)
        VanQsort(tab_bar->Tabs.Data, tab_bar->Tabs.Size, sizeof(VanGuiTabItem), TabItemComparerBySection);

    // Calculate spacing between sections
    const float tab_spacing = g.Style.ItemInnerSpacing.x;
    sections[0].Spacing = sections[0].TabCount > 0 && (sections[1].TabCount + sections[2].TabCount) > 0 ? tab_spacing : 0.0f;
    sections[1].Spacing = sections[1].TabCount > 0 && sections[2].TabCount > 0 ? tab_spacing : 0.0f;

    // Setup next selected tab
    VanGuiID scroll_to_tab_id = 0;
    if (tab_bar->NextScrollToTabId)
    {
        scroll_to_tab_id = tab_bar->NextScrollToTabId;
        tab_bar->NextScrollToTabId = 0;
    }
    if (tab_bar->NextSelectedTabId)
    {
        tab_bar->SelectedTabId = tab_bar->NextSelectedTabId;
        tab_bar->NextSelectedTabId = 0;
        scroll_to_tab_id = tab_bar->SelectedTabId;
    }

    // Process order change request (we could probably process it when requested but it's just saner to do it in a single spot).
    if (tab_bar->ReorderRequestTabId != 0)
    {
        if (TabBarProcessReorder(tab_bar))
            if (tab_bar->ReorderRequestTabId == tab_bar->SelectedTabId)
                scroll_to_tab_id = tab_bar->ReorderRequestTabId;
        tab_bar->ReorderRequestTabId = 0;
    }

    // Tab List Popup (will alter tab_bar->BarRect and therefore the available width!)
    const bool tab_list_popup_button = (tab_bar->Flags & VanGuiTabBarFlags_TabListPopupButton) != 0;
    if (tab_list_popup_button)
        if (VanGuiTabItem* tab_to_select = TabBarTabListPopupButton(tab_bar)) // NB: Will alter BarRect.Min.x!
            scroll_to_tab_id = tab_bar->SelectedTabId = tab_to_select->ID;

    // Leading/Trailing tabs will be shrink only if central one aren't visible anymore, so layout the shrink data as: leading, trailing, central
    // (whereas our tabs are stored as: leading, central, trailing)
    int shrink_buffer_indexes[3] = { 0, sections[0].TabCount + sections[2].TabCount, sections[0].TabCount };
    g.ShrinkWidthBuffer.resize(tab_bar->Tabs.Size);

    // Minimum shrink width
    const float shrink_min_width = (tab_bar->Flags & VanGuiTabBarFlags_FittingPolicyMixed) ? g.Style.TabMinWidthShrink : 1.0f;

    // Compute ideal tabs widths + store them into shrink buffer
    VanGuiTabItem* most_recently_selected_tab = nullptr;
    int curr_section_n = -1;
    bool found_selected_tab_id = false;
    for (int tab_n = 0; tab_n < tab_bar->Tabs.Size; tab_n++)
    {
        VanGuiTabItem* tab = &tab_bar->Tabs[tab_n];
        VAN_ASSERT(tab->LastFrameVisible >= tab_bar->PrevFrameVisible);

        if ((most_recently_selected_tab == nullptr || most_recently_selected_tab->LastFrameSelected < tab->LastFrameSelected) && !(tab->Flags & VanGuiTabItemFlags_Button))
            most_recently_selected_tab = tab;
        if (tab->ID == tab_bar->SelectedTabId)
            found_selected_tab_id = true;
        if (scroll_to_tab_id == 0 && g.NavJustMovedToId == tab->ID)
            scroll_to_tab_id = tab->ID;

        // Refresh tab width immediately, otherwise changes of style e.g. style.FramePadding.x would noticeably lag in the tab bar.
        // Additionally, when using TabBarAddTab() to manipulate tab bar order we occasionally insert new tabs that don't have a width yet,
        // and we cannot wait for the next BeginTabItem() call. We cannot compute this width within TabBarAddTab() because font size depends on the active window.
        const char* tab_name = TabBarGetTabName(tab_bar, tab);
        const bool has_close_button_or_unsaved_marker = (tab->Flags & VanGuiTabItemFlags_NoCloseButton) == 0 || (tab->Flags & VanGuiTabItemFlags_UnsavedDocument);
        tab->ContentWidth = (tab->RequestedWidth >= 0.0f) ? tab->RequestedWidth : TabItemCalcSize(tab_name, has_close_button_or_unsaved_marker).x;
        if ((tab->Flags & VanGuiTabItemFlags_Button) == 0)
            tab->ContentWidth = VanMax(tab->ContentWidth, g.Style.TabMinWidthBase);

        int section_n = TabItemGetSectionIdx(tab);
        VanGuiTabBarSection* section = &sections[section_n];
        section->Width += tab->ContentWidth + (section_n == curr_section_n ? tab_spacing : 0.0f);
        section->WidthAfterShrinkMinWidth += VanMin(tab->ContentWidth, shrink_min_width) + (section_n == curr_section_n ? tab_spacing : 0.0f);
        curr_section_n = section_n;

        // Store data so we can build an array sorted by width if we need to shrink tabs down
        VAN_MSVC_WARNING_SUPPRESS(6385);
        VanGuiShrinkWidthItem* shrink_width_item = &g.ShrinkWidthBuffer[shrink_buffer_indexes[section_n]++];
        shrink_width_item->Index = tab_n;
        shrink_width_item->Width = shrink_width_item->InitialWidth = tab->ContentWidth;
        tab->Width = VanMax(tab->ContentWidth, 1.0f);
    }

    // Compute total ideal width (used for e.g. auto-resizing a window)
    float width_all_tabs_after_min_width_shrink = 0.0f;
    tab_bar->WidthAllTabsIdeal = 0.0f;
    for (int section_n = 0; section_n < 3; section_n++)
    {
        tab_bar->WidthAllTabsIdeal += sections[section_n].Width + sections[section_n].Spacing;
        width_all_tabs_after_min_width_shrink += sections[section_n].WidthAfterShrinkMinWidth + sections[section_n].Spacing;
    }

    // Horizontal scrolling buttons
    // Important: note that TabBarScrollButtons() will alter BarRect.Max.x.
    const bool can_scroll = (tab_bar->Flags & VanGuiTabBarFlags_FittingPolicyScroll) || (tab_bar->Flags & VanGuiTabBarFlags_FittingPolicyMixed);
    const float width_all_tabs_to_use_for_scroll = (tab_bar->Flags & VanGuiTabBarFlags_FittingPolicyScroll) ? tab_bar->WidthAllTabs : width_all_tabs_after_min_width_shrink;
    tab_bar->ScrollButtonEnabled = ((width_all_tabs_to_use_for_scroll > tab_bar->BarRect.GetWidth() && tab_bar->Tabs.Size > 1) && !(tab_bar->Flags & VanGuiTabBarFlags_NoTabListScrollingButtons) && can_scroll);
    if (tab_bar->ScrollButtonEnabled)
        if (VanGuiTabItem* scroll_and_select_tab = TabBarScrollingButtons(tab_bar))
        {
            scroll_to_tab_id = scroll_and_select_tab->ID;
            if ((scroll_and_select_tab->Flags & VanGuiTabItemFlags_Button) == 0)
                tab_bar->SelectedTabId = scroll_to_tab_id;
        }
    if (scroll_to_tab_id == 0 && scroll_to_selected_tab)
        scroll_to_tab_id = tab_bar->SelectedTabId;

    // Shrink widths if full tabs don't fit in their allocated space
    float section_0_w = sections[0].Width + sections[0].Spacing;
    float section_1_w = sections[1].Width + sections[1].Spacing;
    float section_2_w = sections[2].Width + sections[2].Spacing;
    bool central_section_is_visible = (section_0_w + section_2_w) < tab_bar->BarRect.GetWidth();
    float width_excess;
    if (central_section_is_visible)
        width_excess = VanMax(section_1_w - (tab_bar->BarRect.GetWidth() - section_0_w - section_2_w), 0.0f); // Excess used to shrink central section
    else
        width_excess = (section_0_w + section_2_w) - tab_bar->BarRect.GetWidth(); // Excess used to shrink leading/trailing section

    // With VanGuiTabBarFlags_FittingPolicyScroll policy, we will only shrink leading/trailing if the central section is not visible anymore
    const bool can_shrink = (tab_bar->Flags & VanGuiTabBarFlags_FittingPolicyShrink) || (tab_bar->Flags & VanGuiTabBarFlags_FittingPolicyMixed);
    if (width_excess >= 1.0f && (can_shrink || !central_section_is_visible))
    {
        int shrink_data_count = (central_section_is_visible ? sections[1].TabCount : sections[0].TabCount + sections[2].TabCount);
        int shrink_data_offset = (central_section_is_visible ? sections[0].TabCount + sections[2].TabCount : 0);
        ShrinkWidths(g.ShrinkWidthBuffer.Data + shrink_data_offset, shrink_data_count, width_excess, shrink_min_width);

        // Apply shrunk values into tabs and sections
        for (int tab_n = shrink_data_offset; tab_n < shrink_data_offset + shrink_data_count; tab_n++)
        {
            VanGuiTabItem* tab = &tab_bar->Tabs[g.ShrinkWidthBuffer[tab_n].Index];
            float shrinked_width = VAN_TRUNC(g.ShrinkWidthBuffer[tab_n].Width);
            if (shrinked_width < 0.0f)
                continue;

            shrinked_width = VanMax(1.0f, shrinked_width);
            int section_n = TabItemGetSectionIdx(tab);
            sections[section_n].Width -= (tab->Width - shrinked_width);
            tab->Width = shrinked_width;
        }
    }

    // Layout all active tabs
    int section_tab_index = 0;
    float tab_offset = 0.0f;
    tab_bar->WidthAllTabs = 0.0f;
    for (int section_n = 0; section_n < 3; section_n++)
    {
        VanGuiTabBarSection* section = &sections[section_n];
        if (section_n == 2)
            tab_offset = VanMin(VanMax(0.0f, tab_bar->BarRect.GetWidth() - section->Width), tab_offset);

        for (int tab_n = 0; tab_n < section->TabCount; tab_n++)
        {
            VanGuiTabItem* tab = &tab_bar->Tabs[section_tab_index + tab_n];
            tab->Offset = tab_offset;
            tab->NameOffset = -1;
            tab_offset += tab->Width + (tab_n < section->TabCount - 1 ? g.Style.ItemInnerSpacing.x : 0.0f);
        }
        tab_bar->WidthAllTabs += VanMax(section->Width + section->Spacing, 0.0f);
        tab_offset += section->Spacing;
        section_tab_index += section->TabCount;
    }

    // Clear name buffers
    tab_bar->TabsNames.Buf.resize(0);

    // If we have lost the selected tab, select the next most recently active one
    const bool tab_bar_appearing = (tab_bar->PrevFrameVisible + 1 < g.FrameCount);
    if (found_selected_tab_id == false && !tab_bar_appearing)
        tab_bar->SelectedTabId = 0;
    if (tab_bar->SelectedTabId == 0 && tab_bar->NextSelectedTabId == 0 && most_recently_selected_tab != nullptr)
        scroll_to_tab_id = tab_bar->SelectedTabId = most_recently_selected_tab->ID;

    // Lock in visible tab
    tab_bar->VisibleTabId = tab_bar->SelectedTabId;
    tab_bar->VisibleTabWasSubmitted = false;

    // Apply request requests
    if (scroll_to_tab_id != 0)
        TabBarScrollToTab(tab_bar, scroll_to_tab_id, sections);
    else if (tab_bar->ScrollButtonEnabled && IsMouseHoveringRect(tab_bar->BarRect.Min, tab_bar->BarRect.Max, true) && IsWindowContentHoverable(g.CurrentWindow))
    {
        const float wheel = g.IO.MouseWheelRequestAxisSwap ? g.IO.MouseWheel : g.IO.MouseWheelH;
        const VanGuiKey wheel_key = g.IO.MouseWheelRequestAxisSwap ? VanGuiKey_MouseWheelY : VanGuiKey_MouseWheelX;
        if (TestKeyOwner(wheel_key, tab_bar->ID) && wheel != 0.0f)
        {
            const float scroll_step = wheel * TabBarCalcScrollableWidth(tab_bar, sections) / 3.0f;
            tab_bar->ScrollingTargetDistToVisibility = 0.0f;
            tab_bar->ScrollingTarget = TabBarScrollClamp(tab_bar, tab_bar->ScrollingTarget - scroll_step);
        }
        SetKeyOwner(wheel_key, tab_bar->ID);
    }

    // Update scrolling
    tab_bar->ScrollingAnim = TabBarScrollClamp(tab_bar, tab_bar->ScrollingAnim);
    tab_bar->ScrollingTarget = TabBarScrollClamp(tab_bar, tab_bar->ScrollingTarget);
    if (tab_bar->ScrollingAnim != tab_bar->ScrollingTarget)
    {
        // Scrolling speed adjust itself so we can always reach our target in 1/3 seconds.
        // Teleport if we are aiming far off the visible line
        tab_bar->ScrollingSpeed = VanMax(tab_bar->ScrollingSpeed, 70.0f * g.FontSize);
        tab_bar->ScrollingSpeed = VanMax(tab_bar->ScrollingSpeed, VanFabs(tab_bar->ScrollingTarget - tab_bar->ScrollingAnim) / 0.3f);
        const bool teleport = (tab_bar->PrevFrameVisible + 1 < g.FrameCount) || (tab_bar->ScrollingTargetDistToVisibility > 10.0f * g.FontSize);
        tab_bar->ScrollingAnim = teleport ? tab_bar->ScrollingTarget : VanLinearSweep(tab_bar->ScrollingAnim, tab_bar->ScrollingTarget, g.IO.DeltaTime * tab_bar->ScrollingSpeed);
    }
    else
    {
        tab_bar->ScrollingSpeed = 0.0f;
    }
    tab_bar->ScrollingRectMinX = tab_bar->BarRect.Min.x + sections[0].Width + sections[0].Spacing;
    tab_bar->ScrollingRectMaxX = tab_bar->BarRect.Max.x - sections[2].Width - sections[1].Spacing;

    // Actual layout in host window (we don't do it in BeginTabBar() so as not to waste an extra frame)
    VanGuiWindow* window = g.CurrentWindow;
    window->DC.CursorPos = tab_bar->BarRect.Min;
    ItemSize(VanVec2(tab_bar->WidthAllTabs, tab_bar->BarRect.GetHeight()), tab_bar->FramePadding.y);
    window->DC.IdealMaxPos.x = VanMax(window->DC.IdealMaxPos.x, tab_bar->BarRect.Min.x + tab_bar->WidthAllTabsIdeal);
}

// Dockable windows uses Name/ID in the global namespace. Non-dockable items use the ID stack.
static VanU32   VanGui::TabBarCalcTabID(VanGuiTabBar* tab_bar, const char* label, VanGuiWindow* docked_window)
{
    VAN_ASSERT(docked_window == nullptr); // master branch only
    VAN_UNUSED(docked_window);
    if (tab_bar->Flags & VanGuiTabBarFlags_DockNode)
    {
        VanGuiID id = VanHashStr(label);
        KeepAliveID(id);
        return id;
    }
    else
    {
        VanGuiWindow* window = GVanGui->CurrentWindow;
        return window->GetID(label);
    }
}

static float VanGui::TabBarCalcMaxTabWidth()
{
    VanGuiContext& g = *GVanGui;
    return g.FontSize * 20.0f;
}

VanGuiTabItem* VanGui::TabBarFindTabByID(VanGuiTabBar* tab_bar, VanGuiID tab_id)
{
    if (tab_id != 0)
        for (int n = 0; n < tab_bar->Tabs.Size; n++)
            if (tab_bar->Tabs[n].ID == tab_id)
                return &tab_bar->Tabs[n];
    return nullptr;
}

// Order = visible order, not submission order! (which is tab->BeginOrder)
VanGuiTabItem* VanGui::TabBarFindTabByOrder(VanGuiTabBar* tab_bar, int order)
{
    if (order < 0 || order >= tab_bar->Tabs.Size)
        return nullptr;
    return &tab_bar->Tabs[order];
}

VanGuiTabItem* VanGui::TabBarGetCurrentTab(VanGuiTabBar* tab_bar)
{
    if (tab_bar->LastTabItemIdx < 0 || tab_bar->LastTabItemIdx >= tab_bar->Tabs.Size)
        return nullptr;
    return &tab_bar->Tabs[tab_bar->LastTabItemIdx];
}

const char* VanGui::TabBarGetTabName(VanGuiTabBar* tab_bar, VanGuiTabItem* tab)
{
    if (tab->NameOffset == -1)
        return "N/A";
    VAN_ASSERT(tab->NameOffset < tab_bar->TabsNames.Buf.Size);
    return tab_bar->TabsNames.Buf.Data + tab->NameOffset;
}

// The *TabId fields are already set by the docking system _before_ the actual TabItem was created, so we clear them regardless.
void VanGui::TabBarRemoveTab(VanGuiTabBar* tab_bar, VanGuiID tab_id)
{
    if (VanGuiTabItem* tab = TabBarFindTabByID(tab_bar, tab_id))
        tab_bar->Tabs.erase(tab);
    if (tab_bar->VisibleTabId == tab_id)      { tab_bar->VisibleTabId = 0; }
    if (tab_bar->SelectedTabId == tab_id)     { tab_bar->SelectedTabId = 0; }
    if (tab_bar->NextSelectedTabId == tab_id) { tab_bar->NextSelectedTabId = 0; }
}

// Called on manual closure attempt
void VanGui::TabBarCloseTab(VanGuiTabBar* tab_bar, VanGuiTabItem* tab)
{
    if (tab->Flags & VanGuiTabItemFlags_Button)
        return; // A button appended with TabItemButton().

    if ((tab->Flags & (VanGuiTabItemFlags_UnsavedDocument | VanGuiTabItemFlags_NoAssumedClosure)) == 0)
    {
        // This will remove a frame of lag for selecting another tab on closure.
        // However we don't run it in the case where the 'Unsaved' flag is set, so user gets a chance to fully undo the closure
        tab->WantClose = true;
        if (tab_bar->VisibleTabId == tab->ID)
        {
            tab->LastFrameVisible = -1;
            tab_bar->SelectedTabId = tab_bar->NextSelectedTabId = 0;
        }
    }
    else
    {
        // Actually select before expecting closure attempt (on an UnsavedDocument tab user is expect to e.g. show a popup)
        if (tab_bar->VisibleTabId != tab->ID)
            TabBarQueueFocus(tab_bar, tab);
    }
}

static float VanGui::TabBarScrollClamp(VanGuiTabBar* tab_bar, float scrolling)
{
    scrolling = VanMin(scrolling, tab_bar->WidthAllTabs - tab_bar->BarRect.GetWidth());
    return VanMax(scrolling, 0.0f);
}

// Note: we may scroll to tab that are not selected! e.g. using keyboard arrow keys
static void VanGui::TabBarScrollToTab(VanGuiTabBar* tab_bar, VanGuiID tab_id, VanGuiTabBarSection* sections)
{
    VanGuiTabItem* tab = TabBarFindTabByID(tab_bar, tab_id);
    if (tab == nullptr)
        return;
    if (tab->Flags & VanGuiTabItemFlags_SectionMask_)
        return;

    VanGuiContext& g = *GVanGui;
    float margin = g.FontSize * 1.0f; // When to scroll to make Tab N+1 visible always make a bit of N visible to suggest more scrolling area (since we don't have a scrollbar)
    int order = TabBarGetTabOrder(tab_bar, tab);

    // Scrolling happens only in the central section (leading/trailing sections are not scrolling)
    float scrollable_width = TabBarCalcScrollableWidth(tab_bar, sections);

    // We make all tabs positions all relative Sections[0].Width to make code simpler
    float tab_x1 = tab->Offset - sections[0].Width + (order > sections[0].TabCount - 1 ? -margin : 0.0f);
    float tab_x2 = tab->Offset - sections[0].Width + tab->Width + (order + 1 < tab_bar->Tabs.Size - sections[2].TabCount ? margin : 1.0f);
    tab_bar->ScrollingTargetDistToVisibility = 0.0f;
    if (tab_bar->ScrollingTarget > tab_x1 || (tab_x2 - tab_x1 >= scrollable_width))
    {
        // Scroll to the left
        tab_bar->ScrollingTargetDistToVisibility = VanMax(tab_bar->ScrollingAnim - tab_x2, 0.0f);
        tab_bar->ScrollingTarget = tab_x1;
    }
    else if (tab_bar->ScrollingTarget < tab_x2 - scrollable_width)
    {
        // Scroll to the right
        tab_bar->ScrollingTargetDistToVisibility = VanMax((tab_x1 - scrollable_width) - tab_bar->ScrollingAnim, 0.0f);
        tab_bar->ScrollingTarget = tab_x2 - scrollable_width;
    }
}

void VanGui::TabBarQueueFocus(VanGuiTabBar* tab_bar, VanGuiTabItem* tab)
{
    tab_bar->NextSelectedTabId = tab->ID;
}

void VanGui::TabBarQueueFocus(VanGuiTabBar* tab_bar, const char* tab_name)
{
    VAN_ASSERT((tab_bar->Flags & VanGuiTabBarFlags_DockNode) == 0); // Only supported for manual/explicit tab bars
    VanGuiID tab_id = TabBarCalcTabID(tab_bar, tab_name, nullptr);
    tab_bar->NextSelectedTabId = tab_id;
}

void VanGui::TabBarQueueReorder(VanGuiTabBar* tab_bar, VanGuiTabItem* tab, int offset)
{
    VAN_ASSERT(offset != 0);
    VAN_ASSERT(tab_bar->ReorderRequestTabId == 0);
    tab_bar->ReorderRequestTabId = tab->ID;
    tab_bar->ReorderRequestOffset = static_cast<VanS16>(offset);
}

void VanGui::TabBarQueueReorderFromMousePos(VanGuiTabBar* tab_bar, VanGuiTabItem* src_tab, VanVec2 mouse_pos)
{
    VanGuiContext& g = *GVanGui;
    VAN_ASSERT(tab_bar->ReorderRequestTabId == 0);
    if ((tab_bar->Flags & VanGuiTabBarFlags_Reorderable) == 0)
        return;

    const float tab_spacing = g.Style.ItemInnerSpacing.x;
    const bool is_central_section = (src_tab->Flags & VanGuiTabItemFlags_SectionMask_) == 0;
    const float bar_offset = tab_bar->BarRect.Min.x - (is_central_section ? tab_bar->ScrollingTarget : 0);

    // Count number of contiguous tabs we are crossing over
    const int dir = (bar_offset + src_tab->Offset) > mouse_pos.x ? -1 : +1;
    const int src_idx = tab_bar->Tabs.index_from_ptr(src_tab);
    int dst_idx = src_idx;
    for (int i = src_idx; i >= 0 && i < tab_bar->Tabs.Size; i += dir)
    {
        // Reordered tabs must share the same section
        const VanGuiTabItem* dst_tab = &tab_bar->Tabs[i];
        if (dst_tab->Flags & VanGuiTabItemFlags_NoReorder)
            break;
        if ((dst_tab->Flags & VanGuiTabItemFlags_SectionMask_) != (src_tab->Flags & VanGuiTabItemFlags_SectionMask_))
            break;
        dst_idx = i;

        // Include spacing after tab, so when mouse cursor is between tabs we would not continue checking further tabs that are not hovered.
        const float x1 = bar_offset + dst_tab->Offset - tab_spacing;
        const float x2 = bar_offset + dst_tab->Offset + dst_tab->Width + tab_spacing;
        //GetForegroundDrawList()->AddRect(VanVec2(x1, tab_bar->BarRect.Min.y), VanVec2(x2, tab_bar->BarRect.Max.y), VAN_COL32(255, 0, 0, 255));
        if ((dir < 0 && mouse_pos.x > x1) || (dir > 0 && mouse_pos.x < x2))
            break;
    }

    if (dst_idx != src_idx)
        TabBarQueueReorder(tab_bar, src_tab, dst_idx - src_idx);
}

bool VanGui::TabBarProcessReorder(VanGuiTabBar* tab_bar)
{
    VanGuiTabItem* tab1 = TabBarFindTabByID(tab_bar, tab_bar->ReorderRequestTabId);
    if (tab1 == nullptr || (tab1->Flags & VanGuiTabItemFlags_NoReorder))
        return false;

    //VAN_ASSERT(tab_bar->Flags & VanGuiTabBarFlags_Reorderable); // <- this may happen when using debug tools
    int tab2_order = TabBarGetTabOrder(tab_bar, tab1) + tab_bar->ReorderRequestOffset;
    if (tab2_order < 0 || tab2_order >= tab_bar->Tabs.Size)
        return false;

    // Reordered tabs must share the same section
    // (Note: TabBarQueueReorderFromMousePos() also has a similar test but since we allow direct calls to TabBarQueueReorder() we do it here too)
    VanGuiTabItem* tab2 = &tab_bar->Tabs[tab2_order];
    if (tab2->Flags & VanGuiTabItemFlags_NoReorder)
        return false;
    if ((tab1->Flags & VanGuiTabItemFlags_SectionMask_) != (tab2->Flags & VanGuiTabItemFlags_SectionMask_))
        return false;

    VanGuiTabItem item_tmp = *tab1;
    VanGuiTabItem* src_tab = (tab_bar->ReorderRequestOffset > 0) ? tab1 + 1 : tab2;
    VanGuiTabItem* dst_tab = (tab_bar->ReorderRequestOffset > 0) ? tab1 : tab2 + 1;
    const int move_count = (tab_bar->ReorderRequestOffset > 0) ? tab_bar->ReorderRequestOffset : -tab_bar->ReorderRequestOffset;
    memmove(dst_tab, src_tab, move_count * sizeof(VanGuiTabItem));
    *tab2 = item_tmp;

    if (tab_bar->Flags & VanGuiTabBarFlags_SaveSettings)
        MarkIniSettingsDirty();
    return true;
}

static VanGuiTabItem* VanGui::TabBarScrollingButtons(VanGuiTabBar* tab_bar)
{
    VanGuiContext& g = *GVanGui;
    VanGuiWindow* window = g.CurrentWindow;

    const VanVec2 arrow_button_size(g.FontSize - 2.0f, g.FontSize + g.Style.FramePadding.y * 2.0f);
    const float scrolling_buttons_width = arrow_button_size.x * 2.0f;

    const VanVec2 backup_cursor_pos = window->DC.CursorPos;
    //window->DrawList->AddRect(VanVec2(tab_bar->BarRect.Max.x - scrolling_buttons_width, tab_bar->BarRect.Min.y), VanVec2(tab_bar->BarRect.Max.x, tab_bar->BarRect.Max.y), VAN_COL32(255,0,0,255));

    int select_dir = 0;
    VanVec4 arrow_col = g.Style.Colors[VanGuiCol_Text];
    arrow_col.w *= 0.5f;

    PushStyleColor(VanGuiCol_Text, arrow_col);
    PushStyleColor(VanGuiCol_Button, VanVec4(0, 0, 0, 0));
    PushItemFlag(VanGuiItemFlags_ButtonRepeat | VanGuiItemFlags_NoNav, true);
    const float backup_repeat_delay = g.IO.KeyRepeatDelay;
    const float backup_repeat_rate = g.IO.KeyRepeatRate;
    g.IO.KeyRepeatDelay = 0.250f;
    g.IO.KeyRepeatRate = 0.200f;
    float x = VanMax(tab_bar->BarRect.Min.x, tab_bar->BarRect.Max.x - scrolling_buttons_width);
    window->DC.CursorPos = VanVec2(x, tab_bar->BarRect.Min.y);
    if (ArrowButtonEx("##<", VanGuiDir_Left, arrow_button_size, VanGuiButtonFlags_PressedOnClick))
        select_dir = -1;
    window->DC.CursorPos = VanVec2(x + arrow_button_size.x, tab_bar->BarRect.Min.y);
    if (ArrowButtonEx("##>", VanGuiDir_Right, arrow_button_size, VanGuiButtonFlags_PressedOnClick))
        select_dir = +1;
    PopItemFlag();
    PopStyleColor(2);
    g.IO.KeyRepeatRate = backup_repeat_rate;
    g.IO.KeyRepeatDelay = backup_repeat_delay;

    VanGuiTabItem* tab_to_scroll_to = nullptr;
    if (select_dir != 0)
        if (VanGuiTabItem* tab_item = TabBarFindTabByID(tab_bar, tab_bar->SelectedTabId))
        {
            int selected_order = TabBarGetTabOrder(tab_bar, tab_item);
            int target_order = selected_order + select_dir;

            // Skip tab item buttons until another tab item is found or end is reached
            while (tab_to_scroll_to == nullptr)
            {
                // If we are at the end of the list, still scroll to make our tab visible
                tab_to_scroll_to = &tab_bar->Tabs[(target_order >= 0 && target_order < tab_bar->Tabs.Size) ? target_order : selected_order];

                // Cross through buttons
                // (even if first/last item is a button, return it so we can update the scroll)
                if (tab_to_scroll_to->Flags & VanGuiTabItemFlags_Button)
                {
                    target_order += select_dir;
                    selected_order += select_dir;
                    tab_to_scroll_to = (target_order < 0 || target_order >= tab_bar->Tabs.Size) ? tab_to_scroll_to : nullptr;
                }
            }
        }
    window->DC.CursorPos = backup_cursor_pos;
    tab_bar->BarRect.Max.x -= scrolling_buttons_width + 1.0f;

    return tab_to_scroll_to;
}

static VanGuiTabItem* VanGui::TabBarTabListPopupButton(VanGuiTabBar* tab_bar)
{
    VanGuiContext& g = *GVanGui;
    VanGuiWindow* window = g.CurrentWindow;

    // We use g.Style.FramePadding.y to match the square ArrowButton size
    const float tab_list_popup_button_width = g.FontSize + g.Style.FramePadding.y;
    const VanVec2 backup_cursor_pos = window->DC.CursorPos;
    window->DC.CursorPos = VanVec2(tab_bar->BarRect.Min.x - g.Style.FramePadding.y, tab_bar->BarRect.Min.y);
    tab_bar->BarRect.Min.x += tab_list_popup_button_width;

    VanVec4 arrow_col = g.Style.Colors[VanGuiCol_Text];
    arrow_col.w *= 0.5f;
    PushStyleColor(VanGuiCol_Text, arrow_col);
    PushStyleColor(VanGuiCol_Button, VanVec4(0, 0, 0, 0));
    bool open = BeginCombo("##v", nullptr, VanGuiComboFlags_NoPreview | VanGuiComboFlags_HeightLargest);
    PopStyleColor(2);

    VanGuiTabItem* tab_to_select = nullptr;
    if (open)
    {
        for (int tab_n = 0; tab_n < tab_bar->Tabs.Size; tab_n++)
        {
            VanGuiTabItem* tab = &tab_bar->Tabs[tab_n];
            if (tab->Flags & VanGuiTabItemFlags_Button)
                continue;

            const char* tab_name = TabBarGetTabName(tab_bar, tab);
            if (Selectable(tab_name, tab_bar->SelectedTabId == tab->ID))
                tab_to_select = tab;
        }
        EndCombo();
    }

    window->DC.CursorPos = backup_cursor_pos;
    return tab_to_select;
}

//-------------------------------------------------------------------------
// [SECTION] Widgets: BeginTabItem, EndTabItem, etc.
//-------------------------------------------------------------------------
// - BeginTabItem()
// - EndTabItem()
// - TabItemButton()
// - TabItemEx() [Internal]
// - SetTabItemClosed()
// - TabItemCalcSize() [Internal]
// - TabItemBackground() [Internal]
// - TabItemLabelAndCloseButton() [Internal]
//-------------------------------------------------------------------------

bool    VanGui::BeginTabItem(const char* label, bool* p_open, VanGuiTabItemFlags flags)
{
    VanGuiContext& g = *GVanGui;
    VanGuiWindow* window = g.CurrentWindow;
    if (window->SkipItems)
        return false;

    VanGuiTabBar* tab_bar = g.CurrentTabBar;
    VAN_ASSERT_USER_ERROR_RETV(tab_bar != nullptr, false, "Needs to be called between BeginTabBar() and EndTabBar()!");
    VAN_ASSERT((flags & VanGuiTabItemFlags_Button) == 0); // BeginTabItem() Can't be used with button flags, use TabItemButton() instead!

    bool ret = TabItemEx(tab_bar, label, p_open, flags, nullptr);
    if (ret && !(flags & VanGuiTabItemFlags_NoPushId))
    {
        VanGuiTabItem* tab = &tab_bar->Tabs[tab_bar->LastTabItemIdx];
        PushOverrideID(tab->ID); // We already hashed 'label' so push into the ID stack directly instead of doing another hash through PushID(label)
    }
    return ret;
}

void    VanGui::EndTabItem()
{
    VanGuiContext& g = *GVanGui;
    VanGuiWindow* window = g.CurrentWindow;
    if (window->SkipItems)
        return;

    VanGuiTabBar* tab_bar = g.CurrentTabBar;
    VAN_ASSERT_USER_ERROR_RET(tab_bar != nullptr, "Needs to be called between BeginTabBar() and EndTabBar()!");
    VAN_ASSERT(tab_bar->LastTabItemIdx >= 0);
    VanGuiTabItem* tab = &tab_bar->Tabs[tab_bar->LastTabItemIdx];
    if (!(tab->Flags & VanGuiTabItemFlags_NoPushId))
        PopID();
}

bool    VanGui::TabItemButton(const char* label, VanGuiTabItemFlags flags)
{
    VanGuiContext& g = *GVanGui;
    VanGuiWindow* window = g.CurrentWindow;
    if (window->SkipItems)
        return false;

    VanGuiTabBar* tab_bar = g.CurrentTabBar;
    VAN_ASSERT_USER_ERROR_RETV(tab_bar != nullptr, false, "Needs to be called between BeginTabBar() and EndTabBar()!");
    return TabItemEx(tab_bar, label, nullptr, flags | VanGuiTabItemFlags_Button | VanGuiTabItemFlags_NoReorder, nullptr);
}

void    VanGui::TabItemSpacing(const char* str_id, VanGuiTabItemFlags flags, float width)
{
    VanGuiContext& g = *GVanGui;
    VanGuiWindow* window = g.CurrentWindow;
    if (window->SkipItems)
        return;

    VanGuiTabBar* tab_bar = g.CurrentTabBar;
    VAN_ASSERT_USER_ERROR_RET(tab_bar != nullptr, "Needs to be called between BeginTabBar() and EndTabBar()!");
    SetNextItemWidth(width);
    TabItemEx(tab_bar, str_id, nullptr, flags | VanGuiTabItemFlags_Button | VanGuiTabItemFlags_NoReorder | VanGuiTabItemFlags_Invisible, nullptr);
}

bool    VanGui::TabItemEx(VanGuiTabBar* tab_bar, const char* label, bool* p_open, VanGuiTabItemFlags flags, VanGuiWindow* docked_window)
{
    // Layout whole tab bar if not already done
    VanGuiContext& g = *GVanGui;
    if (tab_bar->WantLayout)
    {
        VanGuiNextItemData backup_next_item_data = g.NextItemData;
        TabBarLayout(tab_bar);
        g.NextItemData = backup_next_item_data;
    }
    VanGuiWindow* window = g.CurrentWindow;
    if (window->SkipItems)
        return false;

    const VanGuiStyle& style = g.Style;
    const VanGuiID id = TabBarCalcTabID(tab_bar, label, docked_window);

    // If the user called us with *p_open == false, we early out and don't render.
    // We make a call to ItemAdd() so that attempts to use a contextual popup menu with an implicit ID won't use an older ID.
    VANGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags);
    if (p_open && !*p_open)
    {
        ItemAdd(VanRect(), id, nullptr, VanGuiItemFlags_NoNav);
        return false;
    }

    VAN_ASSERT(!p_open || !(flags & VanGuiTabItemFlags_Button));
    VAN_ASSERT((flags & (VanGuiTabItemFlags_Leading | VanGuiTabItemFlags_Trailing)) != (VanGuiTabItemFlags_Leading | VanGuiTabItemFlags_Trailing)); // Can't use both Leading and Trailing

    // Store into VanGuiTabItemFlags_NoCloseButton, also honor VanGuiTabItemFlags_NoCloseButton passed by user (although not documented)
    if (flags & VanGuiTabItemFlags_NoCloseButton)
        p_open = nullptr;
    else if (p_open == nullptr)
        flags |= VanGuiTabItemFlags_NoCloseButton;

    // Acquire tab data
    VanGuiTabItem* tab = TabBarFindTabByID(tab_bar, id);
    bool tab_is_new = false;
    if (tab == nullptr)
    {
        tab_bar->Tabs.push_back(VanGuiTabItem());
        tab = &tab_bar->Tabs.back();
        tab->ID = id;
        tab_bar->TabsAddedNew = tab_is_new = true;
    }
    tab_bar->LastTabItemIdx = static_cast<VanS16>(tab_bar->Tabs.index_from_ptr(tab));

    // Calculate tab contents size
    VanVec2 size = TabItemCalcSize(label, (p_open != nullptr) || (flags & VanGuiTabItemFlags_UnsavedDocument));
    tab->RequestedWidth = -1.0f;
    if (g.NextItemData.HasFlags & VanGuiNextItemDataFlags_HasWidth)
        size.x = tab->RequestedWidth = g.NextItemData.Width;
    if (tab_is_new)
        tab->Width = VanMax(1.0f, size.x);
    tab->ContentWidth = size.x;
    tab->BeginOrder = tab_bar->TabsActiveCount++;

    const bool tab_bar_appearing = (tab_bar->PrevFrameVisible + 1 < g.FrameCount);
    const bool tab_bar_focused = (tab_bar->Flags & VanGuiTabBarFlags_IsFocused) != 0;
    const bool tab_appearing = (tab->LastFrameVisible + 1 < g.FrameCount);
    const bool tab_just_unsaved = (flags & VanGuiTabItemFlags_UnsavedDocument) && !(tab->Flags & VanGuiTabItemFlags_UnsavedDocument);
    const bool is_tab_button = (flags & VanGuiTabItemFlags_Button) != 0;
    tab->LastFrameVisible = g.FrameCount;
    tab->Flags = flags;

    // Append name _WITH_ the zero-terminator
    if (docked_window != nullptr)
    {
        VAN_ASSERT(docked_window == nullptr); // master branch only
    }
    else
    {
        tab->NameOffset = static_cast<VanS32>(tab_bar->TabsNames.size());
        tab_bar->TabsNames.append(label, label + VanStrlen(label) + 1);
    }

    // Update selected tab
    if (!is_tab_button)
    {
        if (tab_appearing && (tab_bar->Flags & VanGuiTabBarFlags_AutoSelectNewTabs) && tab_bar->NextSelectedTabId == 0)
            if (!tab_bar_appearing || tab_bar->SelectedTabId == 0)
                TabBarQueueFocus(tab_bar, tab); // New tabs gets activated
        if ((flags & VanGuiTabItemFlags_SetSelected) && (tab_bar->SelectedTabId != id)) // _SetSelected can only be passed on explicit tab bar
            TabBarQueueFocus(tab_bar, tab);
    }

    // Lock visibility
    // (Note: tab_contents_visible != tab_selected... because Ctrl+Tab operations may preview some tabs without selecting them!)
    bool tab_contents_visible = (tab_bar->VisibleTabId == id);
    if (tab_contents_visible)
        tab_bar->VisibleTabWasSubmitted = true;

    // On the very first frame of a tab bar we let first tab contents be visible to minimize appearing glitches
    if (!tab_contents_visible && tab_bar->SelectedTabId == 0 && tab_bar_appearing)
        if (tab_bar->Tabs.Size == 1 && !(tab_bar->Flags & VanGuiTabBarFlags_AutoSelectNewTabs))
            tab_contents_visible = true;

    // Note that tab_is_new is not necessarily the same as tab_appearing! When a tab bar stops being submitted
    // and then gets submitted again, the tabs will have 'tab_appearing=true' but 'tab_is_new=false'.
    if (tab_appearing && (!tab_bar_appearing || tab_is_new))
    {
        ItemAdd(VanRect(), id, nullptr, VanGuiItemFlags_NoNav);
        if (is_tab_button)
            return false;
        return tab_contents_visible;
    }

    if (tab_bar->SelectedTabId == id)
        tab->LastFrameSelected = g.FrameCount;

    // Backup current layout position
    const VanVec2 backup_main_cursor_pos = window->DC.CursorPos;

    // Layout
    const bool is_central_section = (tab->Flags & VanGuiTabItemFlags_SectionMask_) == 0;
    size.x = tab->Width;
    if (is_central_section)
        window->DC.CursorPos = tab_bar->BarRect.Min + VanVec2(VAN_TRUNC(tab->Offset - tab_bar->ScrollingAnim), 0.0f);
    else
        window->DC.CursorPos = tab_bar->BarRect.Min + VanVec2(tab->Offset, 0.0f);
    VanVec2 pos = window->DC.CursorPos;
    VanRect bb(pos, pos + size);

    // We don't have CPU clipping primitives to clip the CloseButton (until it becomes a texture), so need to add an extra draw call (temporary in the case of vertical animation)
    const bool want_clip_rect = is_central_section && (bb.Min.x < tab_bar->ScrollingRectMinX || bb.Max.x > tab_bar->ScrollingRectMaxX);
    if (want_clip_rect)
        PushClipRect(VanVec2(VanClamp(bb.Min.x, tab_bar->ScrollingRectMinX, tab_bar->ScrollingRectMaxX), bb.Min.y - 1), VanVec2(tab_bar->ScrollingRectMaxX, bb.Max.y), true);

    VanVec2 backup_cursor_max_pos = window->DC.CursorMaxPos;
    ItemSize(bb.GetSize(), style.FramePadding.y);
    window->DC.CursorMaxPos = backup_cursor_max_pos;

    if (!ItemAdd(bb, id))
    {
        if (want_clip_rect)
            PopClipRect();
        window->DC.CursorPos = backup_main_cursor_pos;
        return tab_contents_visible;
    }

    // Click to Select a tab
    // Allow the close button to overlap
    VanGuiButtonFlags button_flags = (static_cast<VanGuiButtonFlags>(is_tab_button ? VanGuiButtonFlags_PressedOnClickRelease : VanGuiButtonFlags_PressedOnClick) | VanGuiButtonFlags_AllowOverlap);
    if (g.DragDropActive)
        button_flags |= VanGuiButtonFlags_PressedOnDragDropHold;
    bool hovered, held, pressed;
    if (flags & VanGuiTabItemFlags_Invisible)
        hovered = held = pressed = false;
    else
        pressed = ButtonBehavior(bb, id, &hovered, &held, button_flags);
    if (pressed && !is_tab_button)
        TabBarQueueFocus(tab_bar, tab);

    // Drag and drop: re-order tabs
    if (held && !tab_appearing && IsMouseDragging(0))
    {
        if (!g.DragDropActive && (tab_bar->Flags & VanGuiTabBarFlags_Reorderable))
        {
            // While moving a tab it will jump on the other side of the mouse, so we also test for MouseDelta.x
            if (g.IO.MouseDelta.x < 0.0f && g.IO.MousePos.x < bb.Min.x)
            {
                TabBarQueueReorderFromMousePos(tab_bar, tab, g.IO.MousePos);
            }
            else if (g.IO.MouseDelta.x > 0.0f && g.IO.MousePos.x > bb.Max.x)
            {
                TabBarQueueReorderFromMousePos(tab_bar, tab, g.IO.MousePos);
            }
        }
    }

#if 0
    if (hovered && g.HoveredIdNotActiveTimer > TOOLTIP_DELAY && bb.GetWidth() < tab->ContentWidth)
    {
        // Enlarge tab display when hovering
        bb.Max.x = bb.Min.x + VAN_TRUNC(VanLerp(bb.GetWidth(), tab->ContentWidth, VanSaturate((g.HoveredIdNotActiveTimer - 0.40f) * 6.0f)));
        display_draw_list = GetForegroundDrawList(window);
        TabItemBackground(display_draw_list, bb, flags, GetColorU32(VanGuiCol_TitleBgActive));
    }
#endif

    // Render tab shape
    const bool is_visible = (g.LastItemData.StatusFlags & VanGuiItemStatusFlags_Visible) && !(flags & VanGuiTabItemFlags_Invisible);
    if (is_visible)
    {
        VanDrawList* display_draw_list = window->DrawList;
        const VanU32 tab_col = GetColorU32((held || hovered) ? VanGuiCol_TabHovered : tab_contents_visible ? (tab_bar_focused ? VanGuiCol_TabSelected : VanGuiCol_TabDimmedSelected) : (tab_bar_focused ? VanGuiCol_Tab : VanGuiCol_TabDimmed));
        TabItemBackground(display_draw_list, bb, flags, tab_col);
        if (tab_contents_visible && (tab_bar->Flags & VanGuiTabBarFlags_DrawSelectedOverline) && style.TabBarOverlineSize > 0.0f)
        {
            // Might be moved to TabItemBackground() ?
            VanVec2 tl = bb.GetTL() + VanVec2(0, 1.0f * g.CurrentDpiScale);
            VanVec2 tr = bb.GetTR() + VanVec2(0, 1.0f * g.CurrentDpiScale);
            VanU32 overline_col = GetColorU32(tab_bar_focused ? VanGuiCol_TabSelectedOverline : VanGuiCol_TabDimmedSelectedOverline);
            if (style.TabRounding > 0.0f)
            {
                float rounding = style.TabRounding;
                display_draw_list->PathArcToFast(tl + VanVec2(+rounding, +rounding), rounding, 7, 9);
                display_draw_list->PathArcToFast(tr + VanVec2(-rounding, +rounding), rounding, 9, 11);
                display_draw_list->PathStroke(overline_col, style.TabBarOverlineSize);
            }
            else
            {
                display_draw_list->AddLine(tl - VanVec2(0.5f, 0.5f), tr - VanVec2(0.5f, 0.5f), overline_col, style.TabBarOverlineSize);
            }
        }
        RenderNavCursor(bb, id);

        // Select with right mouse button. This is so the common idiom for context menu automatically highlight the current widget.
        const bool hovered_unblocked = IsItemHovered(VanGuiHoveredFlags_AllowWhenBlockedByPopup);
        if (tab_bar->SelectedTabId != tab->ID && hovered_unblocked && (IsMouseClicked(1) || IsMouseReleased(1)) && !is_tab_button)
            TabBarQueueFocus(tab_bar, tab);

        if (tab_bar->Flags & VanGuiTabBarFlags_NoCloseWithMiddleMouseButton)
            flags |= VanGuiTabItemFlags_NoCloseWithMiddleMouseButton;

        // Render tab label, process close button
        const VanGuiID close_button_id = p_open ? GetIDWithSeed("#CLOSE", nullptr, id) : 0;
        bool just_closed;
        bool text_clipped;
        TabItemLabelAndCloseButton(display_draw_list, bb, tab_just_unsaved ? (flags & ~VanGuiTabItemFlags_UnsavedDocument) : flags, tab_bar->FramePadding, label, id, close_button_id, tab_contents_visible, &just_closed, &text_clipped);
        if (just_closed && p_open != nullptr)
        {
            *p_open = false;
            TabBarCloseTab(tab_bar, tab);
        }

        // Tooltip
        // (Won't work over the close button because ItemOverlap systems messes up with HoveredIdTimer-> seems ok)
        // (We test IsItemHovered() to discard e.g. when another item is active or drag and drop over the tab bar, which g.HoveredId ignores)
        // FIXME: This is a mess.
        // FIXME: We may want disabled tab to still display the tooltip?
        if (text_clipped && g.HoveredId == id && !held)
            if (!(tab_bar->Flags & VanGuiTabBarFlags_NoTooltip) && !(tab->Flags & VanGuiTabItemFlags_NoTooltip))
                SetItemTooltip("%.*s", static_cast<int>(FindRenderedTextEnd(label) - label), label);
    }

    // Restore main window position so user can draw there
    if (want_clip_rect)
        PopClipRect();
    window->DC.CursorPos = backup_main_cursor_pos;

    VAN_ASSERT(!is_tab_button || !(tab_bar->SelectedTabId == tab->ID && is_tab_button)); // TabItemButton should not be selected
    if (is_tab_button)
        return pressed;
    return tab_contents_visible;
}

// [Public] This is call is 100% optional but it allows to remove some one-frame glitches when a tab has been unexpectedly removed.
// To use it to need to call the function SetTabItemClosed() between BeginTabBar() and EndTabBar().
// Tabs closed by the close button will automatically be flagged to avoid this issue.
void    VanGui::SetTabItemClosed(const char* label)
{
    VanGuiContext& g = *GVanGui;
    bool is_within_manual_tab_bar = g.CurrentTabBar && !(g.CurrentTabBar->Flags & VanGuiTabBarFlags_DockNode);
    if (is_within_manual_tab_bar)
    {
        VanGuiTabBar* tab_bar = g.CurrentTabBar;
        VanGuiID tab_id = TabBarCalcTabID(tab_bar, label, nullptr);
        if (VanGuiTabItem* tab = TabBarFindTabByID(tab_bar, tab_id))
            tab->WantClose = true; // Will be processed by next call to TabBarLayout()
    }
}

VanVec2 VanGui::TabItemCalcSize(const char* label, bool has_close_button_or_unsaved_marker)
{
    VanGuiContext& g = *GVanGui;
    VanVec2 label_size = CalcTextSize(label, nullptr, true);
    VanVec2 size = VanVec2(label_size.x + g.Style.FramePadding.x, label_size.y + g.Style.FramePadding.y * 2.0f);
    if (has_close_button_or_unsaved_marker)
        size.x += g.Style.FramePadding.x + (g.Style.ItemInnerSpacing.x + g.FontSize); // We use Y intentionally to fit the close button circle.
    else
        size.x += g.Style.FramePadding.x + 1.0f;
    return VanVec2(VanMin(size.x, TabBarCalcMaxTabWidth()), size.y);
}

VanVec2 VanGui::TabItemCalcSize(VanGuiWindow*)
{
    VAN_ASSERT(0); // This function exists to facilitate merge with 'docking' branch.
    return VanVec2(0.0f, 0.0f);
}

void VanGui::TabItemBackground(VanDrawList* draw_list, const VanRect& bb, VanGuiTabItemFlags flags, VanU32 col)
{
    // While rendering tabs, we trim 1 pixel off the top of our bounding box so they can fit within a regular frame height while looking "detached" from it.
    VanGuiContext& g = *GVanGui;
    const float width = bb.GetWidth();
    VAN_UNUSED(flags);
    VAN_ASSERT(width > 0.0f);
    const float rounding = VanMax(0.0f, VanMin((flags & VanGuiTabItemFlags_Button) ? g.Style.FrameRounding : g.Style.TabRounding, width * 0.5f - 1.0f));
    const float y1 = bb.Min.y + 1.0f;
    const float y2 = bb.Max.y - g.Style.TabBarBorderSize;
    draw_list->PathLineTo(VanVec2(bb.Min.x, y2));
    draw_list->PathArcToFast(VanVec2(bb.Min.x + rounding, y1 + rounding), rounding, 6, 9);
    draw_list->PathArcToFast(VanVec2(bb.Max.x - rounding, y1 + rounding), rounding, 9, 12);
    draw_list->PathLineTo(VanVec2(bb.Max.x, y2));
    draw_list->PathFillConvex(col);
    if (g.Style.TabBorderSize > 0.0f)
    {
        draw_list->PathLineTo(VanVec2(bb.Min.x + 0.5f, y2));
        draw_list->PathArcToFast(VanVec2(bb.Min.x + rounding + 0.5f, y1 + rounding + 0.5f), rounding, 6, 9);
        draw_list->PathArcToFast(VanVec2(bb.Max.x - rounding - 0.5f, y1 + rounding + 0.5f), rounding, 9, 12);
        draw_list->PathLineTo(VanVec2(bb.Max.x - 0.5f, y2));
        draw_list->PathStroke(GetColorU32(VanGuiCol_Border), g.Style.TabBorderSize);
    }
}

// Render text label (with custom clipping) + Unsaved Document marker + Close Button logic
// We tend to lock style.FramePadding for a given tab-bar, hence the 'frame_padding' parameter.
void VanGui::TabItemLabelAndCloseButton(VanDrawList* draw_list, const VanRect& bb, VanGuiTabItemFlags flags, VanVec2 frame_padding, const char* label, VanGuiID tab_id, VanGuiID close_button_id, bool is_contents_visible, bool* out_just_closed, bool* out_text_clipped)
{
    VanGuiContext& g = *GVanGui;
    const char* label_end = FindRenderedTextEnd(label);
    VanVec2 label_size = CalcTextSize(label, label_end, false);

    if (out_just_closed)
        *out_just_closed = false;
    if (out_text_clipped)
        *out_text_clipped = false;

    if (bb.GetWidth() <= 1.0f)
        return;

    // In Style V2 we'll have full override of all colors per state (e.g. focused, selected)
    // But right now if you want to alter text color of tabs this is what you need to do.
#if 0
    const float backup_alpha = g.Style.Alpha;
    if (!is_contents_visible)
        g.Style.Alpha *= 0.7f;
#endif

    // Render text label (with clipping + alpha gradient) + unsaved marker
    VanRect text_ellipsis_clip_bb(bb.Min.x + frame_padding.x, bb.Min.y + frame_padding.y, bb.Max.x - frame_padding.x, bb.Max.y);

    // Return clipped state ignoring the close button
    if (out_text_clipped)
    {
        *out_text_clipped = (text_ellipsis_clip_bb.Min.x + label_size.x) > text_ellipsis_clip_bb.Max.x;
        //draw_list->AddCircle(text_ellipsis_clip_bb.Min, 3.0f, *out_text_clipped ? VAN_COL32(255, 0, 0, 255) : VAN_COL32(0, 255, 0, 255));
    }

    const float button_sz = g.FontSize;
    const VanVec2 button_pos(VanMax(bb.Min.x, bb.Max.x - frame_padding.x - button_sz), bb.Min.y + frame_padding.y);

    // Close Button & Unsaved Marker
    // We are relying on a subtle and confusing distinction between 'hovered' and 'g.HoveredId' which happens because we are using VanGuiButtonFlags_AllowOverlapMode + SetItemAllowOverlap()
    //  'hovered' will be true when hovering the Tab but NOT when hovering the close button
    //  'g.HoveredId==id' will be true when hovering the Tab including when hovering the close button
    //  'g.ActiveId==close_button_id' will be true when we are holding on the close button, in which case both hovered booleans are false
    bool close_button_pressed = false;
    bool close_button_visible = false;
    bool is_hovered = g.HoveredId == tab_id || g.HoveredId == close_button_id || g.ActiveId == tab_id || g.ActiveId == close_button_id; // Any interaction account for this too.

    if (close_button_id != 0)
    {
        if (is_contents_visible)
            close_button_visible = (g.Style.TabCloseButtonMinWidthSelected < 0.0f) ? true : (is_hovered && bb.GetWidth() >= VanMax(button_sz, g.Style.TabCloseButtonMinWidthSelected));
        else
            close_button_visible = (g.Style.TabCloseButtonMinWidthUnselected < 0.0f) ? true : (is_hovered && bb.GetWidth() >= VanMax(button_sz, g.Style.TabCloseButtonMinWidthUnselected));
    }

    // When tabs/document is unsaved, the unsaved marker takes priority over the close button.
    const bool unsaved_marker_visible = (flags & VanGuiTabItemFlags_UnsavedDocument) != 0 && (button_pos.x + button_sz <= bb.Max.x) && (!close_button_visible || !is_hovered);
    if (unsaved_marker_visible)
    {
        VanVec2 bullet_pos = button_pos + VanVec2(button_sz, button_sz) * 0.5f;
        RenderBullet(draw_list, bullet_pos, GetColorU32(VanGuiCol_UnsavedMarker));
    }
    else if (close_button_visible)
    {
        VanGuiLastItemData last_item_backup = g.LastItemData;
        if (CloseButton(close_button_id, button_pos))
            close_button_pressed = true;
        g.LastItemData = last_item_backup;

        // Close with middle mouse button
        if (is_hovered && !(flags & VanGuiTabItemFlags_NoCloseWithMiddleMouseButton) && IsMouseClicked(2))
            close_button_pressed = true;
    }

    // This is all rather complicated
    // (the main idea is that because the close button only appears on hover, we don't want it to alter the ellipsis position)
    // FIXME: if FramePadding is noticeably large, ellipsis_max_x will be wrong here (e.g. #3497), maybe for consistency that parameter of RenderTextEllipsis() shouldn't exist..
    float ellipsis_max_x = text_ellipsis_clip_bb.Max.x;
    if (close_button_visible || unsaved_marker_visible)
    {
        const bool visible_without_hover = unsaved_marker_visible || (is_contents_visible ? g.Style.TabCloseButtonMinWidthSelected : g.Style.TabCloseButtonMinWidthUnselected) < 0.0f;
        if (visible_without_hover)
        {
            text_ellipsis_clip_bb.Max.x -= button_sz * 0.90f;
            ellipsis_max_x -= button_sz * 0.90f;
        }
        else
        {
            text_ellipsis_clip_bb.Max.x -= button_sz * 1.00f;
        }
    }
    LogSetNextTextDecoration("/", "\\");
    RenderTextEllipsis(draw_list, text_ellipsis_clip_bb.Min, text_ellipsis_clip_bb.Max, ellipsis_max_x, label, label_end, &label_size);

#if 0
    if (!is_contents_visible)
        g.Style.Alpha = backup_alpha;
#endif

    if (out_just_closed)
        *out_just_closed = close_button_pressed;
}


#endif // #ifndef VANGUI_DISABLE
