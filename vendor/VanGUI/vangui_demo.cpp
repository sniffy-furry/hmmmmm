// dear vangui, v1.92.9 WIP
// (demo code)

// Help:
// - Read FAQ at http://dearvangui.com/faq
// - Call and read VanGui::ShowDemoWindow() in vangui_demo.cpp. All applications in examples/ are doing that.
// - Need help integrating VanGUI in your codebase?
//   - Read Getting Started https://github.com/ocornut/vangui/wiki/Getting-Started
//   - Read 'Programmer guide' in vangui.cpp for notes on how to setup VanGUI in your codebase.
// Read top of vangui.cpp and vangui.h for many details, documentation, comments, links.
// Get the latest version at https://github.com/ocornut/vangui

// How to easily locate code?
// - Use Tools->Item Picker to debug break in code by clicking any widgets: https://github.com/ocornut/vangui/wiki/Debug-Tools
// - Browse pthom's online vangui_explorer: web version the demo w/ source code browser: https://pthom.github.io/vangui_explorer
// - Find a visible string and search for it in the code!

//---------------------------------------------------
// PLEASE DO NOT REMOVE THIS FILE FROM YOUR PROJECT!
//---------------------------------------------------
// Message to the person tempted to delete this file when integrating VanGUI into their codebase:
// Think again! It is the most useful reference code that you and other coders will want to refer to and call.
// Have the VanGui::ShowDemoWindow() function wired in an always-available debug menu of your game/app!
// Also include Metrics! ItemPicker! DebugLog! and other debug features.
// Removing this file from your project is hindering access to documentation for everyone in your team,
// likely leading you to poorer usage of the library.
// Everything in this file will be stripped out by the linker if you don't call VanGui::ShowDemoWindow().
// If you want to link core VanGUI in your shipped builds but want a thorough guarantee that the demo will not be
// linked, you can setup your vanconfig.h with #define VANGUI_DISABLE_DEMO_WINDOWS and those functions will be empty.
// In another situation, whenever you have VanGUI available you probably want this to be available for reference.
// Thank you,
// -Your beloved friend, vangui_demo.cpp (which you won't delete)

//--------------------------------------------
// ABOUT THE MEANING OF THE 'static' KEYWORD:
//--------------------------------------------
// In this demo code, we frequently use 'static' variables inside functions.
// A static variable persists across calls. It is essentially a global variable but declared inside the scope of the function.
// Think of "static int n = 0;" as "global int n = 0;" !
// We do this IN THE DEMO because we want:
// - to gather code and data in the same place.
// - to make the demo source code faster to read, faster to change, smaller in size.
// - it is also a convenient way of storing simple UI related information as long as your function
//   doesn't need to be reentrant or used in multiple threads.
// This might be a pattern you will want to use in your code, but most of the data you would be working
// with in a complex codebase is likely going to be stored outside your functions.

//-----------------------------------------
// ABOUT THE CODING STYLE OF OUR DEMO CODE
//-----------------------------------------
// The Demo code in this file is designed to be easy to copy-and-paste into your application!
// Because of this:
// - We never omit the VanGui:: prefix when calling functions, even though most code here is in the same namespace.
// - We try to declare static variables in the local scope, as close as possible to the code using them.
// - We never use any of the helpers/facilities used internally by VanGUI, unless available in the public API.
// - We never use maths operators on VanVec2/VanVec4. For our other sources files we use them, and they are provided
//   by vangui.h using the VANGUI_DEFINE_MATH_OPERATORS define. For your own sources file they are optional
//   and require you either enable those, either provide your own via VAN_VEC2_CLASS_EXTRA in vanconfig.h.
//   Because we can't assume anything about your support of maths operators, we cannot use them in vangui_demo.cpp.

// Navigating this file:
// - In Visual Studio: Ctrl+Comma ("Edit.GoToAll") can follow symbols inside comments, whereas Ctrl+F12 ("Edit.GoToImplementation") cannot.
// - In Visual Studio w/ Visual Assist installed: Alt+G ("VAssistX.GoToImplementation") can also follow symbols inside comments.
// - In VS Code, CLion, etc.: Ctrl+Click can follow symbols inside comments.
// - You can search/grep for all sections listed in the index to find the section.

/*

Index of this file:

// [SECTION] Forward Declarations
// [SECTION] Helpers
// [SECTION] Demo Window / ShowDemoWindow()
// [SECTION] DemoWindowMenuBar()
// [SECTION] Helpers: ExampleTreeNode, ExampleMemberInfo (for use by Property Editor & Multi-Select demos)
// [SECTION] Helpers: ExampleImageViewer
// [SECTION] DemoWindowWidgetsBasic()
// [SECTION] DemoWindowWidgetsBullets()
// [SECTION] DemoWindowWidgetsCollapsingHeaders()
// [SECTION] DemoWindowWidgetsComboBoxes()
// [SECTION] DemoWindowWidgetsColorAndPickers()
// [SECTION] DemoWindowWidgetsDataTypes()
// [SECTION] DemoWindowWidgetsDisableBlocks()
// [SECTION] DemoWindowWidgetsDragAndDrop()
// [SECTION] DemoWindowWidgetsDragsAndSliders()
// [SECTION] DemoWindowWidgetsFonts()
// [SECTION] DemoWindowWidgetsImages()
// [SECTION] DemoWindowWidgetsListBoxes()
// [SECTION] DemoWindowWidgetsMultiComponents()
// [SECTION] DemoWindowWidgetsPlotting()
// [SECTION] DemoWindowWidgetsProgressBars()
// [SECTION] DemoWindowWidgetsQueryingStatuses()
// [SECTION] DemoWindowWidgetsSelectables()
// [SECTION] DemoWindowWidgetsSelectionAndMultiSelect()
// [SECTION] DemoWindowWidgetsTabs()
// [SECTION] DemoWindowWidgetsText()
// [SECTION] DemoWindowWidgetsTextFilter()
// [SECTION] DemoWindowWidgetsTextInput()
// [SECTION] DemoWindowWidgetsTooltips()
// [SECTION] DemoWindowWidgetsTreeNodes()
// [SECTION] DemoWindowWidgetsVerticalSliders()
// [SECTION] DemoWindowWidgets()
// [SECTION] DemoWindowLayout()
// [SECTION] DemoWindowPopups()
// [SECTION] DemoWindowTables()
// [SECTION] DemoWindowInputs()
// [SECTION] About Window / ShowAboutWindow()
// [SECTION] Style Editor / ShowStyleEditor()
// [SECTION] User Guide / ShowUserGuide()
// [SECTION] Example App: Main Menu Bar / ShowExampleAppMainMenuBar()
// [SECTION] Example App: Debug Console / ShowExampleAppConsole()
// [SECTION] Example App: Image Viewer / ShowExampleAppImageViewer()
// [SECTION] Example App: Debug Log / ShowExampleAppLog()
// [SECTION] Example App: Simple Layout / ShowExampleAppLayout()
// [SECTION] Example App: Property Editor / ShowExampleAppPropertyEditor()
// [SECTION] Example App: Long Text / ShowExampleAppLongText()
// [SECTION] Example App: Auto Resize / ShowExampleAppAutoResize()
// [SECTION] Example App: Constrained Resize / ShowExampleAppConstrainedResize()
// [SECTION] Example App: Simple overlay / ShowExampleAppSimpleOverlay()
// [SECTION] Example App: Fullscreen window / ShowExampleAppFullscreen()
// [SECTION] Example App: Manipulating window titles / ShowExampleAppWindowTitles()
// [SECTION] Example App: Custom Rendering using VanDrawList API / ShowExampleAppCustomRendering()
// [SECTION] Example App: Documents Handling / ShowExampleAppDocuments()
// [SECTION] Example App: Assets Browser / ShowExampleAppAssetsBrowser()

*/

#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "vangui.h"
#ifndef VANGUI_DISABLE

// System includes
#include <ctype.h>          // toupper
#include <climits>         // INT_MIN, INT_MAX
#include <cmath>           // sqrtf, powf, cosf, sinf, floorf, ceilf
#include <cstdio>          // vsnprintf, sscanf, printf
#include <cstdlib>         // nullptr, malloc, free, strtol, strtod
#include <stdint.h>         // intptr_t
#if !defined(_MSC_VER) || _MSC_VER >= 1800
#include <inttypes.h>       // PRId64/PRIu64, not avail in some MinGW headers.
#endif
#ifdef __EMSCRIPTEN__
#include <emscripten/version.h>     // __EMSCRIPTEN_MAJOR__ etc.
#endif

// Visual Studio warnings
#ifdef _MSC_VER
#pragma warning (disable: 4127)     // condition expression is constant
#pragma warning (disable: 4996)     // 'This function or variable may be unsafe': strcpy, strdup, vsnprintf, sscanf, fopen
#pragma warning (disable: 26451)    // [Static Analyzer] Arithmetic overflow : Using operator 'xxx' on a 4 byte value and then casting the result to an 8 byte value. Cast the value to the wider type before calling operator 'xxx' to avoid overflow(io.2).
#endif

// Clang/GCC warnings with -Weverything
#if defined(__clang__)
#if __has_warning("-Wunknown-warning-option")
#pragma clang diagnostic ignored "-Wunknown-warning-option"         // warning: unknown warning group 'xxx'                     // not all warnings are known by all Clang versions and they tend to be rename-happy.. so ignoring warnings triggers new warnings on some configuration. Great!
#endif
#pragma clang diagnostic ignored "-Wunknown-pragmas"                // warning: unknown warning group 'xxx'
#pragma clang diagnostic ignored "-Wold-style-cast"                 // warning: use of old-style cast                           // yes, they are more terse.
#pragma clang diagnostic ignored "-Wdeprecated-declarations"        // warning: 'xx' is deprecated: The POSIX name for this..   // for strdup used in demo code (so user can copy & paste the code)
#pragma clang diagnostic ignored "-Wint-to-void-pointer-cast"       // warning: cast to 'void *' from smaller integer type
#pragma clang diagnostic ignored "-Wformat"                         // warning: format specifies type 'int' but the argument has type 'unsigned int'
#pragma clang diagnostic ignored "-Wformat-security"                // warning: format string is not a string literal
#pragma clang diagnostic ignored "-Wexit-time-destructors"          // warning: declaration requires an exit-time destructor    // exit-time destruction order is undefined. if MemFree() leads to users code that has been disabled before exit it might cause problems. VanGui coding style welcomes static/globals.
#pragma clang diagnostic ignored "-Wunused-macros"                  // warning: macro is not used                               // we define snprintf/vsnprintf on Windows so they are available, but not always used.
#pragma clang diagnostic ignored "-Wzero-as-null-pointer-constant"  // warning: zero as null pointer constant                   // some standard header variations use #define nullptr 0
#pragma clang diagnostic ignored "-Wdouble-promotion"               // warning: implicit conversion from 'float' to 'double' when passing argument to function  // using printf() is a misery with this as C++ va_arg ellipsis changes float to double.
#pragma clang diagnostic ignored "-Wreserved-id-macro"              // warning: macro name is a reserved identifier
#pragma clang diagnostic ignored "-Wimplicit-int-float-conversion"  // warning: implicit conversion from 'xxx' to 'float' may lose precision
#pragma clang diagnostic ignored "-Wunsafe-buffer-usage"            // warning: 'xxx' is an unsafe pointer used for buffer access
#pragma clang diagnostic ignored "-Wswitch-default"                 // warning: 'switch' missing 'default' label
#elif defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wpragmas"                          // warning: unknown option after '#pragma GCC diagnostic' kind
#pragma GCC diagnostic ignored "-Wfloat-equal"                      // warning: comparing floating-point with '==' or '!=' is unsafe
#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"              // warning: cast to pointer from integer of different size
#pragma GCC diagnostic ignored "-Wformat"                           // warning: format '%p' expects argument of type 'int'/'void*', but argument X has type 'unsigned int'/'VanGuiWindow*'
#pragma GCC diagnostic ignored "-Wformat-security"                  // warning: format string is not a string literal (potentially insecure)
#pragma GCC diagnostic ignored "-Wdouble-promotion"                 // warning: implicit conversion from 'float' to 'double' when passing argument to function
#pragma GCC diagnostic ignored "-Wconversion"                       // warning: conversion to 'xxxx' from 'xxxx' may alter its value
#pragma GCC diagnostic ignored "-Wmisleading-indentation"           // [__GNUC__ >= 6] warning: this 'if' clause does not guard this statement      // GCC 6.0+ only. See #883 on GitHub.
#pragma GCC diagnostic ignored "-Wstrict-overflow"                  // warning: assuming signed overflow does not occur when simplifying division / ..when changing X +- C1 cmp C2 to X cmp C2 -+ C1
#pragma GCC diagnostic ignored "-Wcast-qual"                        // warning: cast from type 'const xxxx *' to type 'xxxx *' casts away qualifiers
#endif

// Play it nice with Windows users (Update: May 2018, Notepad now supports Unix-style carriage returns!)
#ifdef _WIN32
#define VAN_NEWLINE  "\r\n"
#else
#define VAN_NEWLINE  "\n"
#endif

// Helpers
#if defined(_MSC_VER) && !defined(snprintf)
#define snprintf    _snprintf
#endif
#if defined(_MSC_VER) && !defined(vsnprintf)
#define vsnprintf   _vsnprintf
#endif

// Format specifiers for 64-bit values (hasn't been decently standardized before VS2013)
#if !defined(PRId64) && defined(_MSC_VER)
#define PRId64 "I64d"
#define PRIu64 "I64u"
#elif !defined(PRId64)
#define PRId64 "lld"
#define PRIu64 "llu"
#endif

// Helpers macros
// We normally try to not use many helpers in vangui_demo.cpp in order to make code easier to copy and paste,
// but making an exception here as those are largely simplifying code...
// In other vangui sources we can use nicer internal functions from vangui_internal.h (VanMin/VanMax) but not in the demo.
#define VAN_MIN(A, B)            (((A) < (B)) ? (A) : (B))
#define VAN_MAX(A, B)            (((A) >= (B)) ? (A) : (B))
#define VAN_CLAMP(V, MN, MX)     ((V) < (MN) ? (MN) : (V) > (MX) ? (MX) : (V))

// Enforce cdecl calling convention for functions called by the standard library,
// in case compilation settings changed the default to e.g. __vectorcall
#ifndef VANGUI_CDECL
#ifdef _MSC_VER
#define VANGUI_CDECL __cdecl
#else
#define VANGUI_CDECL
#endif
#endif

//-----------------------------------------------------------------------------
// [SECTION] Forward Declarations
//-----------------------------------------------------------------------------

#if !defined(VANGUI_DISABLE_DEMO_WINDOWS)

// Forward Declarations
struct VanGuiDemoWindowData;
static void ShowExampleAppMainMenuBar();
static void ShowExampleAppAssetsBrowser(bool* p_open);
static void ShowExampleAppConsole(bool* p_open);
static void ShowExampleAppCustomRendering(bool* p_open);
static void ShowExampleAppDocuments(bool* p_open);
static void ShowExampleAppImageViewer(bool* p_open);
static void ShowExampleAppLog(bool* p_open);
static void ShowExampleAppLayout(bool* p_open);
static void ShowExampleAppPropertyEditor(bool* p_open, VanGuiDemoWindowData* demo_data);
static void ShowExampleAppSimpleOverlay(bool* p_open);
static void ShowExampleAppAutoResize(bool* p_open);
static void ShowExampleAppConstrainedResize(bool* p_open);
static void ShowExampleAppFullscreen(bool* p_open);
static void ShowExampleAppLongText(bool* p_open);
static void ShowExampleAppWindowTitles(bool* p_open);
static void ShowExampleMenuFile();

// We split the contents of the big ShowDemoWindow() function into smaller functions
// (because the link time of very large functions tends to grow non-linearly)
static void DemoWindowMenuBar(VanGuiDemoWindowData* demo_data);
static void DemoWindowWidgets(VanGuiDemoWindowData* demo_data);
static void DemoWindowLayout();
static void DemoWindowPopups();
static void DemoWindowTables();
static void DemoWindowColumns();
static void DemoWindowInputs();

// Helper tree functions used by Property Editor & Multi-Select demos
struct ExampleTreeNode;
static ExampleTreeNode* ExampleTree_CreateNode(const char* name, int uid, ExampleTreeNode* parent);
static void             ExampleTree_DestroyNode(ExampleTreeNode* node);

//-----------------------------------------------------------------------------
// [SECTION] Helpers
//-----------------------------------------------------------------------------

// Helper to display a little (?) mark which shows a tooltip when hovered.
// In your own code you may want to display an actual icon if you are using a merged icon fonts (see docs/FONTS.md)
static void HelpMarker(const char* desc)
{
    VanGui::TextDisabled("(?)");
    if (VanGui::BeginItemTooltip())
    {
        VanGui::PushTextWrapPos(VanGui::GetFontSize() * 35.0f);
        VanGui::TextUnformatted(desc);
        VanGui::PopTextWrapPos();
        VanGui::EndTooltip();
    }
}

// Helper to wire demo markers located in code to an interactive browser (e.g. https://pthom.github.io/vangui_explorer)
#if VANGUI_VERSION_NUM >= 19263
namespace VanGui { extern VANGUI_API void DemoMarker(const char* file, int line, const char* section); }
#define VANGUI_DEMO_MARKER(section)  do { VanGui::DemoMarker("vangui_demo.cpp", __LINE__, section); } while (0)
#endif

// Sneakily forward declare functions which aren't worth putting in public API yet
namespace VanGui
{
    VANGUI_API void ShowFontAtlas(VanFontAtlas* atlas);
    VANGUI_API void TreeNodeSetOpen(VanGuiID storage_id, bool is_open);
}

//-----------------------------------------------------------------------------
// [SECTION] Demo Window / ShowDemoWindow()
//-----------------------------------------------------------------------------

// Data to be shared across different functions of the demo.
struct VanGuiDemoWindowData
{
    // Examples Apps (accessible from the "Examples" menu)
    bool ShowMainMenuBar = false;
    bool ShowAppAssetsBrowser = false;
    bool ShowAppConsole = false;
    bool ShowAppCustomRendering = false;
    bool ShowAppDocuments = false;
    bool ShowAppImageViewer = false;
    bool ShowAppLog = false;
    bool ShowAppLayout = false;
    bool ShowAppPropertyEditor = false;
    bool ShowAppSimpleOverlay = false;
    bool ShowAppAutoResize = false;
    bool ShowAppConstrainedResize = false;
    bool ShowAppFullscreen = false;
    bool ShowAppLongText = false;
    bool ShowAppWindowTitles = false;

    // VanGUI Tools (accessible from the "Tools" menu)
    bool ShowMetrics = false;
    bool ShowDebugLog = false;
    bool ShowIDStackTool = false;
    bool ShowStyleEditor = false;
    bool ShowAbout = false;

    // Other data
    bool DisableSections = false;
    ExampleTreeNode* DemoTree = nullptr;

    ~VanGuiDemoWindowData() { if (DemoTree) ExampleTree_DestroyNode(DemoTree); }
};

// Demonstrate most VanGUI features (this is big function!)
// You may execute this function to experiment with the UI and understand what it does.
// You may then search for keywords in the code when you are interested by a specific feature.
void VanGui::ShowDemoWindow(bool* p_open)
{
    // Exceptionally add an extra assert here for people confused about initial VanGUI setup
    // Most functions would normally just assert/crash if the context is missing.
    VAN_ASSERT(VanGui::GetCurrentContext() != nullptr && "Missing VanGUI context. Refer to examples app!");

    // Verify ABI compatibility between caller code and compiled version of VanGUI. This helps detects some build issues.
    VANGUI_CHECKVERSION();

    // Stored data
    static VanGuiDemoWindowData demo_data;

    // Examples Apps (accessible from the "Examples" menu)
    if (demo_data.ShowMainMenuBar)          { ShowExampleAppMainMenuBar(); }
    if (demo_data.ShowAppDocuments)         { ShowExampleAppDocuments(&demo_data.ShowAppDocuments); }
    if (demo_data.ShowAppAssetsBrowser)     { ShowExampleAppAssetsBrowser(&demo_data.ShowAppAssetsBrowser); }
    if (demo_data.ShowAppConsole)           { ShowExampleAppConsole(&demo_data.ShowAppConsole); }
    if (demo_data.ShowAppCustomRendering)   { ShowExampleAppCustomRendering(&demo_data.ShowAppCustomRendering); }
    if (demo_data.ShowAppImageViewer)       { ShowExampleAppImageViewer(&demo_data.ShowAppImageViewer); }
    if (demo_data.ShowAppLog)               { ShowExampleAppLog(&demo_data.ShowAppLog); }
    if (demo_data.ShowAppLayout)            { ShowExampleAppLayout(&demo_data.ShowAppLayout); }
    if (demo_data.ShowAppPropertyEditor)    { ShowExampleAppPropertyEditor(&demo_data.ShowAppPropertyEditor, &demo_data); }
    if (demo_data.ShowAppSimpleOverlay)     { ShowExampleAppSimpleOverlay(&demo_data.ShowAppSimpleOverlay); }
    if (demo_data.ShowAppAutoResize)        { ShowExampleAppAutoResize(&demo_data.ShowAppAutoResize); }
    if (demo_data.ShowAppConstrainedResize) { ShowExampleAppConstrainedResize(&demo_data.ShowAppConstrainedResize); }
    if (demo_data.ShowAppFullscreen)        { ShowExampleAppFullscreen(&demo_data.ShowAppFullscreen); }
    if (demo_data.ShowAppLongText)          { ShowExampleAppLongText(&demo_data.ShowAppLongText); }
    if (demo_data.ShowAppWindowTitles)      { ShowExampleAppWindowTitles(&demo_data.ShowAppWindowTitles); }

    // VanGUI Tools (accessible from the "Tools" menu)
    if (demo_data.ShowMetrics)              { VanGui::ShowMetricsWindow(&demo_data.ShowMetrics); }
    if (demo_data.ShowDebugLog)             { VanGui::ShowDebugLogWindow(&demo_data.ShowDebugLog); }
    if (demo_data.ShowIDStackTool)          { VanGui::ShowIDStackToolWindow(&demo_data.ShowIDStackTool); }
    if (demo_data.ShowAbout)                { VanGui::ShowAboutWindow(&demo_data.ShowAbout); }
    if (demo_data.ShowStyleEditor)
    {
        VanGui::Begin("VanGUI Style Editor", &demo_data.ShowStyleEditor);
        VanGui::ShowStyleEditor();
        VanGui::End();
    }

    // Demonstrate the various window flags. Typically you would just use the default!
    static bool no_titlebar = false;
    static bool no_scrollbar = false;
    static bool no_menu = false;
    static bool no_move = false;
    static bool no_resize = false;
    static bool no_collapse = false;
    static bool no_close = false;
    static bool no_nav = false;
    static bool no_background = false;
    static bool no_bring_to_front = false;
    static bool unsaved_document = false;

    VanGuiWindowFlags window_flags = 0;
    if (no_titlebar)        window_flags |= VanGuiWindowFlags_NoTitleBar;
    if (no_scrollbar)       window_flags |= VanGuiWindowFlags_NoScrollbar;
    if (!no_menu)           window_flags |= VanGuiWindowFlags_MenuBar;
    if (no_move)            window_flags |= VanGuiWindowFlags_NoMove;
    if (no_resize)          window_flags |= VanGuiWindowFlags_NoResize;
    if (no_collapse)        window_flags |= VanGuiWindowFlags_NoCollapse;
    if (no_nav)             window_flags |= VanGuiWindowFlags_NoNav;
    if (no_background)      window_flags |= VanGuiWindowFlags_NoBackground;
    if (no_bring_to_front)  window_flags |= VanGuiWindowFlags_NoBringToFrontOnFocus;
    if (unsaved_document)   window_flags |= VanGuiWindowFlags_UnsavedDocument;
    if (no_close)           p_open = nullptr; // Don't pass our bool* to Begin

    // We specify a default position/size in case there's no data in the .ini file.
    // We only do it to make the demo applications a little more welcoming, but typically this isn't required.
    const VanGuiViewport* main_viewport = VanGui::GetMainViewport();
    VanGui::SetNextWindowPos(VanVec2(main_viewport->WorkPos.x + 650, main_viewport->WorkPos.y + 20), VanGuiCond_FirstUseEver);
    VanGui::SetNextWindowSize(VanVec2(550, 680), VanGuiCond_FirstUseEver);

    // Main body of the Demo window starts here.
    if (!VanGui::Begin("VanGUI Demo", p_open, window_flags))
    {
        // Early out if the window is collapsed, as an optimization.
        VanGui::End();
        return;
    }

    // Most framed widgets share a common width settings. Remaining width is used for the label.
    // The width of the frame may be changed with PushItemWidth() or SetNextItemWidth().
    // - Positive value for absolute size, negative value for right-alignment.
    // - The default value is about GetWindowWidth() * 0.65f.
    // - See 'Demo->Layout->Widgets Width' for details.
    // Here we change the frame width based on how much width we want to give to the label.
    const float label_width_base = VanGui::GetFontSize() * 12;               // Some amount of width for label, based on font size.
    const float label_width_max = VanGui::GetContentRegionAvail().x * 0.40f; // ...but always leave some room for framed widgets.
    const float label_width = VAN_MIN(label_width_base, label_width_max);
    VanGui::PushItemWidth(-label_width);                                     // Right-align: framed items will leave 'label_width' available for the label.
    //VanGui::PushItemWidth(VanGui::GetContentRegionAvail().x * 0.40f);       // e.g. Use 40% width for framed widgets, leaving 60% width for labels.
    //VanGui::PushItemWidth(-VanGui::GetContentRegionAvail().x * 0.40f);      // e.g. Use 40% width for labels, leaving 60% width for framed widgets.
    //VanGui::PushItemWidth(VanGui::GetFontSize() * -12);                     // e.g. Use XXX width for labels, leaving the rest for framed widgets.

    // Menu Bar
    DemoWindowMenuBar(&demo_data);

    VanGui::Text("dear vangui says hello! (%s) (%d)", VANGUI_VERSION, VANGUI_VERSION_NUM);
    VanGui::Spacing();

    if (VanGui::CollapsingHeader("Help"))
    {
        VANGUI_DEMO_MARKER("Help");
        VanGui::SeparatorText("ABOUT THIS DEMO:");
        VanGui::BulletText("Sections below are demonstrating many aspects of the library.");
        VanGui::BulletText("The \"Examples\" menu above leads to more demo contents.");
        VanGui::BulletText("The \"Tools\" menu above gives access to: About Box, Style Editor,\n"
                          "and Metrics/Debugger (general purpose VanGUI debugging tool).");
        VanGui::BulletText("Web demo (w/ source code browser): ");
        VanGui::SameLine(0, 0);
        VanGui::TextLinkOpenURL("https://pthom.github.io/vangui_explorer");

        VanGui::SeparatorText("PROGRAMMER GUIDE:");
        VanGui::BulletText("See the ShowDemoWindow() code in vangui_demo.cpp. <- you are here!");
        VanGui::BulletText("See comments in vangui.cpp.");
        VanGui::BulletText("See example applications in the examples/ folder.");
        VanGui::BulletText("Read the FAQ at ");
        VanGui::SameLine(0, 0);
        VanGui::TextLinkOpenURL("https://www.dearvangui.com/faq/");
        VanGui::BulletText("Set 'io.ConfigFlags |= NavEnableKeyboard' for keyboard controls.");
        VanGui::BulletText("Set 'io.ConfigFlags |= NavEnableGamepad' for gamepad controls.");

        VanGui::SeparatorText("USER GUIDE:");
        VanGui::ShowUserGuide();
    }

    if (VanGui::CollapsingHeader("Configuration"))
    {
        VanGuiIO& io = VanGui::GetIO();

        if (VanGui::TreeNode("Configuration##2"))
        {
            VANGUI_DEMO_MARKER("Configuration");
            VanGui::SeparatorText("General");
            VanGui::CheckboxFlags("io.ConfigFlags: NavEnableKeyboard",    &io.ConfigFlags, VanGuiConfigFlags_NavEnableKeyboard);
            VanGui::SameLine(); HelpMarker("Enable keyboard controls.");
            VanGui::CheckboxFlags("io.ConfigFlags: NavEnableGamepad",     &io.ConfigFlags, VanGuiConfigFlags_NavEnableGamepad);
            VanGui::SameLine(); HelpMarker("Enable gamepad controls. Require backend to set io.BackendFlags |= VanGuiBackendFlags_HasGamepad.\n\nRead instructions in vangui.cpp for details.");
            VanGui::CheckboxFlags("io.ConfigFlags: NoMouse",              &io.ConfigFlags, VanGuiConfigFlags_NoMouse);
            VanGui::SameLine(); HelpMarker("Instruct dear vangui to disable mouse inputs and interactions.");

            // The "NoMouse" option can get us stuck with a disabled mouse! Let's provide an alternative way to fix it:
            if (io.ConfigFlags & VanGuiConfigFlags_NoMouse)
            {
                if (fmodf(static_cast<float>(VanGui::GetTime()), 0.40f) < 0.20f)
                {
                    VanGui::SameLine();
                    VanGui::Text("<<PRESS SPACE TO DISABLE>>");
                }
                // Prevent both being checked
                if (VanGui::IsKeyPressed(VanGuiKey_Space) || (io.ConfigFlags & VanGuiConfigFlags_NoKeyboard))
                    io.ConfigFlags &= ~VanGuiConfigFlags_NoMouse;
            }

            VanGui::CheckboxFlags("io.ConfigFlags: NoMouseCursorChange",  &io.ConfigFlags, VanGuiConfigFlags_NoMouseCursorChange);
            VanGui::SameLine(); HelpMarker("Instruct backend to not alter mouse cursor shape and visibility.");
            VanGui::CheckboxFlags("io.ConfigFlags: NoKeyboard", &io.ConfigFlags, VanGuiConfigFlags_NoKeyboard);
            VanGui::SameLine(); HelpMarker("Instruct dear vangui to disable keyboard inputs and interactions.");

            VanGui::Checkbox("io.ConfigInputTrickleEventQueue", &io.ConfigInputTrickleEventQueue);
            VanGui::SameLine(); HelpMarker("Enable input queue trickling: some types of events submitted during the same frame (e.g. button down + up) will be spread over multiple frames, improving interactions with low framerates.");
            VanGui::Checkbox("io.MouseDrawCursor", &io.MouseDrawCursor);
            VanGui::SameLine(); HelpMarker("Instruct VanGUI to render a mouse cursor itself. Note that a mouse cursor rendered via your application GPU rendering path will feel more laggy than hardware cursor, but will be more in sync with your other visuals.\n\nSome desktop applications may use both kinds of cursors (e.g. enable software cursor only when resizing/dragging something).");

            VanGui::SeparatorText("Keyboard/Gamepad Navigation");
            VanGui::Checkbox("io.ConfigNavSwapGamepadButtons", &io.ConfigNavSwapGamepadButtons);
            VanGui::Checkbox("io.ConfigNavMoveSetMousePos", &io.ConfigNavMoveSetMousePos);
            VanGui::SameLine(); HelpMarker("Directional/tabbing navigation teleports the mouse cursor. May be useful on TV/console systems where moving a virtual mouse is difficult");
            VanGui::Checkbox("io.ConfigNavCaptureKeyboard", &io.ConfigNavCaptureKeyboard);
            VanGui::Checkbox("io.ConfigNavEscapeClearFocusItem", &io.ConfigNavEscapeClearFocusItem);
            VanGui::SameLine(); HelpMarker("Pressing Escape clears focused item.");
            VanGui::Checkbox("io.ConfigNavEscapeClearFocusWindow", &io.ConfigNavEscapeClearFocusWindow);
            VanGui::SameLine(); HelpMarker("Pressing Escape clears focused window.");
            VanGui::Checkbox("io.ConfigNavCursorVisibleAuto", &io.ConfigNavCursorVisibleAuto);
            VanGui::SameLine(); HelpMarker("Using directional navigation key makes the cursor visible. Mouse click hides the cursor.");
            VanGui::Checkbox("io.ConfigNavCursorVisibleAlways", &io.ConfigNavCursorVisibleAlways);
            VanGui::SameLine(); HelpMarker("Navigation cursor is always visible.");

            VanGui::SeparatorText("Windows");
            VanGui::Checkbox("io.ConfigWindowsResizeFromEdges", &io.ConfigWindowsResizeFromEdges);
            VanGui::SameLine(); HelpMarker("Enable resizing of windows from their edges and from the lower-left corner.\nThis requires VanGuiBackendFlags_HasMouseCursors for better mouse cursor feedback.");
            VanGui::Checkbox("io.ConfigWindowsMoveFromTitleBarOnly", &io.ConfigWindowsMoveFromTitleBarOnly);
            VanGui::Checkbox("io.ConfigWindowsCopyContentsWithCtrlC", &io.ConfigWindowsCopyContentsWithCtrlC); // [EXPERIMENTAL]
            VanGui::SameLine(); HelpMarker("*EXPERIMENTAL* Ctrl+C copy the contents of focused window into the clipboard.\n\nExperimental because:\n- (1) has known issues with nested Begin/End pairs.\n- (2) text output quality varies.\n- (3) text output is in submission order rather than spatial order.");
            VanGui::Checkbox("io.ConfigScrollbarScrollByPage", &io.ConfigScrollbarScrollByPage);
            VanGui::SameLine(); HelpMarker("Enable scrolling page by page when clicking outside the scrollbar grab.\nWhen disabled, always scroll to clicked location.\nWhen enabled, Shift+Click scrolls to clicked location.");

            VanGui::SeparatorText("Widgets");
            VanGui::Checkbox("io.ConfigInputTextCursorBlink", &io.ConfigInputTextCursorBlink);
            VanGui::SameLine(); HelpMarker("Enable blinking cursor (optional as some users consider it to be distracting).");
            VanGui::Checkbox("io.ConfigInputTextEnterKeepActive", &io.ConfigInputTextEnterKeepActive);
            VanGui::SameLine(); HelpMarker("Pressing Enter will reactivate item and select all text (single-line only).");
            VanGui::Checkbox("io.ConfigDragClickToInputText", &io.ConfigDragClickToInputText);
            VanGui::SameLine(); HelpMarker("Enable turning DragXXX widgets into text input with a simple mouse click-release (without moving).");
            VanGui::Checkbox("io.ConfigMacOSXBehaviors", &io.ConfigMacOSXBehaviors);
            VanGui::SameLine(); HelpMarker("Swap Cmd<>Ctrl keys, enable various MacOS style behaviors.");
            VanGui::Text("Also see Style->Rendering for rendering options.");

            // Also read: https://github.com/ocornut/vangui/wiki/Error-Handling
            VanGui::SeparatorText("Error Handling");

            VanGui::Checkbox("io.ConfigErrorRecovery", &io.ConfigErrorRecovery);
            VanGui::SameLine(); HelpMarker(
                "Options to configure how we handle recoverable errors.\n"
                "- Error recovery is not perfect nor guaranteed! It is a feature to ease development.\n"
                "- You not are not supposed to rely on it in the course of a normal application run.\n"
                "- Possible usage: facilitate recovery from errors triggered from a scripting language or after specific exceptions handlers.\n"
                "- Always ensure that on programmers seat you have at minimum Asserts or Tooltips enabled when making direct vangui API call! "
                "Otherwise it would severely hinder your ability to catch and correct mistakes!");
            VanGui::Checkbox("io.ConfigErrorRecoveryEnableAssert", &io.ConfigErrorRecoveryEnableAssert);
            VanGui::Checkbox("io.ConfigErrorRecoveryEnableDebugLog", &io.ConfigErrorRecoveryEnableDebugLog);
            VanGui::Checkbox("io.ConfigErrorRecoveryEnableTooltip", &io.ConfigErrorRecoveryEnableTooltip);
            if (!io.ConfigErrorRecoveryEnableAssert && !io.ConfigErrorRecoveryEnableDebugLog && !io.ConfigErrorRecoveryEnableTooltip)
                io.ConfigErrorRecoveryEnableAssert = io.ConfigErrorRecoveryEnableDebugLog = io.ConfigErrorRecoveryEnableTooltip = true;

            // Also read: https://github.com/ocornut/vangui/wiki/Debug-Tools
            VanGui::SeparatorText("Debug");
            VanGui::Checkbox("io.ConfigDebugIsDebuggerPresent", &io.ConfigDebugIsDebuggerPresent);
            VanGui::SameLine(); HelpMarker("Enable various tools calling VAN_DEBUG_BREAK().\n\nRequires a debugger being attached, otherwise VAN_DEBUG_BREAK() options will appear to crash your application.");
            VanGui::Checkbox("io.ConfigDebugHighlightIdConflicts", &io.ConfigDebugHighlightIdConflicts);
            VanGui::SameLine(); HelpMarker("Highlight and show an error message when multiple items have conflicting identifiers.");
            VanGui::BeginDisabled();
            VanGui::Checkbox("io.ConfigDebugBeginReturnValueOnce", &io.ConfigDebugBeginReturnValueOnce);
            VanGui::EndDisabled();
            VanGui::SameLine(); HelpMarker("First calls to Begin()/BeginChild() will return false.\n\nTHIS OPTION IS DISABLED because it needs to be set at application boot-time to make sense. Showing the disabled option is a way to make this feature easier to discover.");
            VanGui::Checkbox("io.ConfigDebugBeginReturnValueLoop", &io.ConfigDebugBeginReturnValueLoop);
            VanGui::SameLine(); HelpMarker("Some calls to Begin()/BeginChild() will return false.\n\nWill cycle through window depths then repeat. Windows should be flickering while running.");
            VanGui::Checkbox("io.ConfigDebugIgnoreFocusLoss", &io.ConfigDebugIgnoreFocusLoss);
            VanGui::SameLine(); HelpMarker("Option to deactivate io.AddFocusEvent(false) handling. May facilitate interactions with a debugger when focus loss leads to clearing inputs data.");
            VanGui::Checkbox("io.ConfigDebugIniSettings", &io.ConfigDebugIniSettings);
            VanGui::SameLine(); HelpMarker("Option to save .ini data with extra comments (particularly helpful for Docking, but makes saving slower).");

            VanGui::TreePop();
            VanGui::Spacing();
        }

        if (VanGui::TreeNode("Backend Flags"))
        {
            VANGUI_DEMO_MARKER("Configuration/Backend Flags");
            HelpMarker(
                "Those flags are set by the backends (vangui_impl_xxx files) to specify their capabilities.\n"
                "Here we expose them as read-only fields to avoid breaking interactions with your backend.");

            // FIXME: Maybe we need a BeginReadonly() equivalent to keep label bright?
            VanGui::BeginDisabled();
            VanGui::CheckboxFlags("io.BackendFlags: HasGamepad",           &io.BackendFlags, VanGuiBackendFlags_HasGamepad);
            VanGui::CheckboxFlags("io.BackendFlags: HasMouseCursors",      &io.BackendFlags, VanGuiBackendFlags_HasMouseCursors);
            VanGui::CheckboxFlags("io.BackendFlags: HasSetMousePos",       &io.BackendFlags, VanGuiBackendFlags_HasSetMousePos);
            VanGui::CheckboxFlags("io.BackendFlags: RendererHasVtxOffset", &io.BackendFlags, VanGuiBackendFlags_RendererHasVtxOffset);
            VanGui::CheckboxFlags("io.BackendFlags: RendererHasTextures",  &io.BackendFlags, VanGuiBackendFlags_RendererHasTextures);
            VanGui::EndDisabled();

            VanGui::TreePop();
            VanGui::Spacing();
        }

        if (VanGui::TreeNode("Style, Fonts"))
        {
            VANGUI_DEMO_MARKER("Configuration/Style, Fonts");
            VanGui::Checkbox("Style Editor", &demo_data.ShowStyleEditor);
            VanGui::SameLine();
            HelpMarker("The same contents can be accessed in 'Tools->Style Editor' or by calling the ShowStyleEditor() function.");
            VanGui::TreePop();
            VanGui::Spacing();
        }

        if (VanGui::TreeNode("Capture/Logging"))
        {
            VANGUI_DEMO_MARKER("Configuration/Capture, Logging");
            HelpMarker(
                "The logging API redirects all text output so you can easily capture the content of "
                "a window or a block. Tree nodes can be automatically expanded.\n"
                "Try opening any of the contents below in this window and then click one of the \"Log To\" button.");
            VanGui::LogButtons();

            HelpMarker("You can also call VanGui::LogText() to output directly to the log without a visual output.");
            if (VanGui::Button("Copy \"Hello, world!\" to clipboard"))
            {
                VanGui::LogToClipboard();
                VanGui::LogText("Hello, world!");
                VanGui::LogFinish();
            }
            VanGui::TreePop();
        }
    }

    if (VanGui::CollapsingHeader("Window options"))
    {
        VANGUI_DEMO_MARKER("Window options");
        if (VanGui::BeginTable("split", 3))
        {
            VanGui::TableNextColumn(); VanGui::Checkbox("No titlebar", &no_titlebar);
            VanGui::TableNextColumn(); VanGui::Checkbox("No scrollbar", &no_scrollbar);
            VanGui::TableNextColumn(); VanGui::Checkbox("No menu", &no_menu);
            VanGui::TableNextColumn(); VanGui::Checkbox("No move", &no_move);
            VanGui::TableNextColumn(); VanGui::Checkbox("No resize", &no_resize);
            VanGui::TableNextColumn(); VanGui::Checkbox("No collapse", &no_collapse);
            VanGui::TableNextColumn(); VanGui::Checkbox("No close", &no_close);
            VanGui::TableNextColumn(); VanGui::Checkbox("No nav", &no_nav);
            VanGui::TableNextColumn(); VanGui::Checkbox("No background", &no_background);
            VanGui::TableNextColumn(); VanGui::Checkbox("No bring to front", &no_bring_to_front);
            VanGui::TableNextColumn(); VanGui::Checkbox("Unsaved document", &unsaved_document);
            VanGui::EndTable();
        }
    }

    // All demo contents
    DemoWindowWidgets(&demo_data);
    DemoWindowLayout();
    DemoWindowPopups();
    DemoWindowTables();
    DemoWindowInputs();

    // End of ShowDemoWindow()
    VanGui::PopItemWidth();
    VanGui::End();
}

//-----------------------------------------------------------------------------
// [SECTION] DemoWindowMenuBar()
//-----------------------------------------------------------------------------

static void DemoWindowMenuBar(VanGuiDemoWindowData* demo_data)
{
    if (VanGui::BeginMenuBar())
    {
        if (VanGui::BeginMenu("Menu"))
        {
            VANGUI_DEMO_MARKER("Menu/File");
            ShowExampleMenuFile();
            VanGui::EndMenu();
        }
        if (VanGui::BeginMenu("Examples"))
        {
            VANGUI_DEMO_MARKER("Menu/Examples");
            VanGui::MenuItem("Main menu bar", nullptr, &demo_data->ShowMainMenuBar);

            VanGui::SeparatorText("Mini apps");
            VanGui::MenuItem("Assets Browser", nullptr, &demo_data->ShowAppAssetsBrowser);
            VanGui::MenuItem("Console", nullptr, &demo_data->ShowAppConsole);
            VanGui::MenuItem("Custom rendering", nullptr, &demo_data->ShowAppCustomRendering);
            VanGui::MenuItem("Documents", nullptr, &demo_data->ShowAppDocuments);
            VanGui::MenuItem("Image Viewer", nullptr, &demo_data->ShowAppImageViewer);
            VanGui::MenuItem("Log", nullptr, &demo_data->ShowAppLog);
            VanGui::MenuItem("Property editor", nullptr, &demo_data->ShowAppPropertyEditor);
            VanGui::MenuItem("Simple layout", nullptr, &demo_data->ShowAppLayout);
            VanGui::MenuItem("Simple overlay", nullptr, &demo_data->ShowAppSimpleOverlay);

            VanGui::SeparatorText("Concepts");
            VanGui::MenuItem("Auto-resizing window", nullptr, &demo_data->ShowAppAutoResize);
            VanGui::MenuItem("Constrained-resizing window", nullptr, &demo_data->ShowAppConstrainedResize);
            VanGui::MenuItem("Fullscreen window", nullptr, &demo_data->ShowAppFullscreen);
            VanGui::MenuItem("Long text display", nullptr, &demo_data->ShowAppLongText);
            VanGui::MenuItem("Manipulating window titles", nullptr, &demo_data->ShowAppWindowTitles);

            VanGui::EndMenu();
        }
        //if (VanGui::MenuItem("MenuItem")) {} // You can also use MenuItem() inside a menu bar!
        if (VanGui::BeginMenu("Tools"))
        {
            VANGUI_DEMO_MARKER("Menu/Tools");
            VanGuiIO& io = VanGui::GetIO();
#ifndef VANGUI_DISABLE_DEBUG_TOOLS
            const bool has_debug_tools = true;
#else
            const bool has_debug_tools = false;
#endif
            VanGui::MenuItem("Metrics/Debugger", nullptr, &demo_data->ShowMetrics, has_debug_tools);
            if (VanGui::BeginMenu("Debug Options"))
            {
                VanGui::BeginDisabled(!has_debug_tools);
                VanGui::Checkbox("Highlight ID Conflicts", &io.ConfigDebugHighlightIdConflicts);
                VanGui::EndDisabled();
                VanGui::Checkbox("Assert on error recovery", &io.ConfigErrorRecoveryEnableAssert);
                VanGui::TextDisabled("(see Demo->Configuration for more)");
                VanGui::EndMenu();
            }
            VanGui::MenuItem("Debug Log", nullptr, &demo_data->ShowDebugLog, has_debug_tools);
            VanGui::MenuItem("ID Stack Tool", nullptr, &demo_data->ShowIDStackTool, has_debug_tools);
            bool is_debugger_present = io.ConfigDebugIsDebuggerPresent;
            if (VanGui::MenuItem("Item Picker", nullptr, false, has_debug_tools))// && is_debugger_present))
                VanGui::DebugStartItemPicker();
            if (!is_debugger_present)
                VanGui::SetItemTooltip("Requires io.ConfigDebugIsDebuggerPresent=true to be set.\n\nWe otherwise disable some extra features to avoid casual users crashing the application.");
            VanGui::MenuItem("Style Editor", nullptr, &demo_data->ShowStyleEditor);
            VanGui::MenuItem("About VanGUI", nullptr, &demo_data->ShowAbout);

            VanGui::EndMenu();
        }
        VanGui::EndMenuBar();
    }
}

//-----------------------------------------------------------------------------
// [SECTION] Helpers: ExampleTreeNode, ExampleMemberInfo (for use by Property Editor & Multi-Select demos)
//-----------------------------------------------------------------------------

// Simple representation for a tree
// (this is designed to be simple to understand for our demos, not to be fancy or efficient etc.)
struct ExampleTreeNode
{
    // Tree structure
    char                        Name[28] = "";
    int                         UID = 0;
    ExampleTreeNode*            Parent = nullptr;
    VanVector<ExampleTreeNode*>  Childs;
    int                         IndexInParent = 0;  // Maintaining this allows us to implement linear traversal more easily

    // Leaf Data
    bool                        HasData = false;    // All leaves have data
    bool                        DataMyBool = true;
    int                         DataMyInt = 128;
    VanVec2                      DataMyVec2 = VanVec2(0.0f, 3.141592f);
};

// Simple representation of struct metadata/serialization data.
// (this is a minimal version of what a typical advanced application may provide)
struct ExampleMemberInfo
{
    const char*     Name;       // Member name
    VanGuiDataType   DataType;   // Member type
    int             DataCount;  // Member count (1 when scalar)
    int             Offset;     // Offset inside parent structure
};

// Metadata description of ExampleTreeNode struct.
static const ExampleMemberInfo ExampleTreeNodeMemberInfos[]
{
    { "MyName",     VanGuiDataType_String,  1, offsetof(ExampleTreeNode, Name) },
    { "MyBool",     VanGuiDataType_Bool,    1, offsetof(ExampleTreeNode, DataMyBool) },
    { "MyInt",      VanGuiDataType_S32,     1, offsetof(ExampleTreeNode, DataMyInt) },
    { "MyVec2",     VanGuiDataType_Float,   2, offsetof(ExampleTreeNode, DataMyVec2) },
};

static ExampleTreeNode* ExampleTree_CreateNode(const char* name, int uid, ExampleTreeNode* parent)
{
    ExampleTreeNode* node = VAN_NEW(ExampleTreeNode);
    snprintf(node->Name, VAN_COUNTOF(node->Name), "%s", name);
    node->UID = uid;
    node->Parent = parent;
    node->IndexInParent = parent ? parent->Childs.Size : 0;
    if (parent)
        parent->Childs.push_back(node);
    return node;
}

static void ExampleTree_DestroyNode(ExampleTreeNode* node)
{
    for (ExampleTreeNode* child_node : node->Childs)
        ExampleTree_DestroyNode(child_node);
    VAN_DELETE(node);
}

// Create example tree data
// (warning: this can allocates MANY MANY more times than other code in all of VanGUI + demo combined)
// (a real application managing one million nodes would likely store its tree data differently)
static ExampleTreeNode* ExampleTree_CreateDemoTree()
{
    //     20 root nodes ->    211 total nodes,   ~261 allocs.
    //   1000 root nodes ->   ~11K total nodes,   ~14K allocs.
    //  10000 root nodes ->  ~123K total nodes,  ~154K allocs.
    // 100000 root nodes -> ~1338K total nodes, ~1666K allocs.
    const int ROOT_ITEMS_COUNT = 20;

    static const char* category_names[] = { "Apple", "Banana", "Cherry", "Kiwi", "Mango", "Orange", "Pear", "Pineapple", "Strawberry", "Watermelon" };
    const int category_count = VAN_COUNTOF(category_names);
    const size_t NAME_MAX_LEN = sizeof(ExampleTreeNode::Name);
    char name_buf[NAME_MAX_LEN];
    int uid = 0;
    ExampleTreeNode* node_L0 = ExampleTree_CreateNode("<ROOT>", ++uid, nullptr);
    for (int idx_L0 = 0; idx_L0 < ROOT_ITEMS_COUNT; idx_L0++)
    {
        snprintf(name_buf, VAN_COUNTOF(name_buf), "%s %d", category_names[idx_L0 / (ROOT_ITEMS_COUNT / category_count)], idx_L0 % (ROOT_ITEMS_COUNT / category_count));
        ExampleTreeNode* node_L1 = ExampleTree_CreateNode(name_buf, ++uid, node_L0);
        const int number_of_childs = static_cast<int>(strlen(node_L1->Name));
        for (int idx_L1 = 0; idx_L1 < number_of_childs; idx_L1++)
        {
            snprintf(name_buf, VAN_COUNTOF(name_buf), "Child %d", idx_L1);
            ExampleTreeNode* node_L2 = ExampleTree_CreateNode(name_buf, ++uid, node_L1);
            node_L2->HasData = true;
            if (idx_L1 == 0)
            {
                snprintf(name_buf, VAN_COUNTOF(name_buf), "Sub-child %d", 0);
                ExampleTreeNode* node_L3 = ExampleTree_CreateNode(name_buf, ++uid, node_L2);
                node_L3->HasData = true;
            }
        }
    }
    return node_L0;
}

//-----------------------------------------------------------------------------
// [SECTION] Helpers: ExampleImageViewer
//-----------------------------------------------------------------------------

struct ExampleImageViewerData
{
    VanU32   ImageBgColor = VAN_COL32(100, 100, 100, 255);
    VanU32   GridColor = VAN_COL32(255, 255, 255, 100);
    bool    GridEnabled = true;
    bool    ViewReset = true;
    VanVec2  ViewOffset; // in image space
    float   Zoom = 10.0f;
    float   ZoomMin = 1.0f;
    float   ZoomMax = 10000.0f;
};

static void ExampleImageViewer_DrawOptions(ExampleImageViewerData* data)
{
    VanGui::SetNextItemShortcut(VanGuiKey_G, VanGuiInputFlags_Tooltip); // | VanGuiInputFlags_RouteGlobal
    VanGui::Checkbox("Grid", &data->GridEnabled);
    VanGui::SameLine();
    VanGui::SetNextItemWidth(VanGui::GetFontSize() * 10.0f);
    float zoom_100 = data->Zoom * 100.0f;
    if (VanGui::DragFloat("##Zoom", &zoom_100, 5.0f, data->ZoomMin * 100.0f, data->ZoomMax * 100.0f, "%.0f%%", VanGuiSliderFlags_AlwaysClamp))
        data->Zoom = zoom_100 / 100.0f;
}

static void ExampleImageViewer_DrawCanvas(ExampleImageViewerData* data, VanVec2 canvas_size, VanTextureRef image_tex_ref, int image_w, int image_h)
{
    VanGuiIO& io = VanGui::GetIO();
    VanGuiPlatformIO& platform_io = VanGui::GetPlatformIO();
    VanDrawList* draw_list = VanGui::GetWindowDrawList();
    VAN_ASSERT(canvas_size.x >= 0.0f && canvas_size.y >= 0.0f);

    // Layout canvas
    VanGui::InvisibleButton("##Canvas", canvas_size);
    VanVec2 canvas_min = VanGui::GetItemRectMin();
    VanVec2 canvas_max = VanGui::GetItemRectMax();

    if (data->ViewReset)
        data->ViewOffset = VanVec2((canvas_size.x * 0.5f / data->Zoom) - 0.5f, (canvas_size.y * 0.5f / data->Zoom) - 0.5f); // Add half a pixel padding
    data->ViewReset = false;

    // Handle inputs
    if (VanGui::SetItemKeyOwner(VanGuiKey_MouseWheelY))
        if (io.MouseWheel != 0.0f)
            data->Zoom = VAN_CLAMP(data->Zoom * (1.0f + io.MouseWheel * 0.10f), data->ZoomMin, data->ZoomMax);
    float zoom = data->Zoom; // (float)(int)ViewZoom;
    if (VanGui::IsItemActive() && VanGui::IsMouseDragging(0))
    {
        data->ViewOffset.x -= io.MouseDelta.x / zoom;
        data->ViewOffset.y -= io.MouseDelta.y / zoom;
    }

    // Display image
    VanVec2 image_min, image_max;
    image_min.x = static_cast<float>(static_cast<int>((canvas_min.x - (data->ViewOffset.x * zoom)) + (canvas_size.x * 0.5f)));
    image_min.y = static_cast<float>(static_cast<int>((canvas_min.y - (data->ViewOffset.y * zoom)) + (canvas_size.y * 0.5f)));
    image_max.x = static_cast<float>(static_cast<int>(image_min.x + image_w * zoom));
    image_max.y = static_cast<float>(static_cast<int>(image_min.y + image_h * zoom));
    draw_list->AddRect(VanVec2(canvas_min.x - 1.0f, canvas_min.y - 1.0f), VanVec2(canvas_max.x + 1.0f, canvas_max.y + 1.0f), VAN_COL32(255, 255, 255, 255));
    draw_list->PushClipRect(canvas_min, canvas_max, true);
    draw_list->AddRectFilled(image_min, image_max, data->ImageBgColor);
    if (platform_io.DrawCallback_SetSamplerNearest != nullptr)
        draw_list->AddCallback(platform_io.DrawCallback_SetSamplerNearest);
    draw_list->AddImage(image_tex_ref, image_min, image_max);
    if (platform_io.DrawCallback_SetSamplerLinear != nullptr)
        draw_list->AddCallback(VanGui::GetPlatformIO().DrawCallback_SetSamplerLinear);

    // Display grid lines for visible pixels
    if (data->GridEnabled && zoom > 6.0f)
    {
        const float step = static_cast<float>(zoom);
        for (int px = static_cast<int>((canvas_min.x - image_min.x) / step); px <= static_cast<int>((canvas_max.x - image_min.x) / step); px++)
            draw_list->AddLineV(image_min.x + px * step, canvas_min.y, canvas_max.y, data->GridColor, 1.0f);
        for (int py = static_cast<int>((canvas_min.y - image_min.y) / step); py <= static_cast<int>((canvas_max.y - image_min.y) / step); py++)
            draw_list->AddLineH(canvas_min.x, canvas_max.x, image_min.y + py * step, data->GridColor, 1.0f);
    }
    draw_list->PopClipRect();
}

//-----------------------------------------------------------------------------
// [SECTION] DemoWindowWidgetsBasic()
//-----------------------------------------------------------------------------

static void DemoWindowWidgetsBasic()
{
    if (VanGui::TreeNode("Basic"))
    {
        VANGUI_DEMO_MARKER("Widgets/Basic");
        VanGui::SeparatorText("General");

        VANGUI_DEMO_MARKER("Widgets/Basic/Button");
        static int clicked = 0;
        if (VanGui::Button("Button"))
            clicked++;
        if (clicked & 1)
        {
            VanGui::SameLine();
            VanGui::Text("Thanks for clicking me!");
        }

        VANGUI_DEMO_MARKER("Widgets/Basic/Checkbox");
        static bool check = true;
        VanGui::Checkbox("checkbox", &check);

        VANGUI_DEMO_MARKER("Widgets/Basic/RadioButton");
        static int e = 0;
        VanGui::RadioButton("radio a", &e, 0); VanGui::SameLine();
        VanGui::RadioButton("radio b", &e, 1); VanGui::SameLine();
        VanGui::RadioButton("radio c", &e, 2);

        VanGui::AlignTextToFramePadding();
        VanGui::TextLinkOpenURL("Hyperlink", "https://github.com/ocornut/vangui/wiki/Error-Handling");

        // Color buttons, demonstrate using PushID() to add unique identifier in the ID stack, and changing style.
        VANGUI_DEMO_MARKER("Widgets/Basic/Buttons (Colored)");
        for (int i = 0; i < 7; i++)
        {
            if (i > 0)
                VanGui::SameLine();
            VanGui::PushID(i);
            VanGui::PushStyleColor(VanGuiCol_Button, static_cast<VanVec4>(VanColor::HSV(i / 7.0f, 0.6f, 0.6f)));
            VanGui::PushStyleColor(VanGuiCol_ButtonHovered, static_cast<VanVec4>(VanColor::HSV(i / 7.0f, 0.7f, 0.7f)));
            VanGui::PushStyleColor(VanGuiCol_ButtonActive, static_cast<VanVec4>(VanColor::HSV(i / 7.0f, 0.8f, 0.8f)));
            VanGui::Button("Click");
            VanGui::PopStyleColor(3);
            VanGui::PopID();
        }

        // Use AlignTextToFramePadding() to align text baseline to the baseline of framed widgets elements
        // (otherwise a Text+SameLine+Button sequence will have the text a little too high by default!)
        // See 'Demo->Layout->Text Baseline Alignment' for details.
        VanGui::AlignTextToFramePadding();
        VanGui::Text("Hold to repeat:");
        VanGui::SameLine();

        // Arrow buttons with Repeater
        VANGUI_DEMO_MARKER("Widgets/Basic/Buttons (Repeating)");
        static int counter = 0;
        float spacing = VanGui::GetStyle().ItemInnerSpacing.x;
        VanGui::PushItemFlag(VanGuiItemFlags_ButtonRepeat, true);
        if (VanGui::ArrowButton("##left", VanGuiDir_Left)) { counter--; }
        VanGui::SameLine(0.0f, spacing);
        if (VanGui::ArrowButton("##right", VanGuiDir_Right)) { counter++; }
        VanGui::PopItemFlag();
        VanGui::SameLine();
        VanGui::Text("%d", counter);

        VanGui::Button("Tooltip");
        VanGui::SetItemTooltip("I am a tooltip");

        VanGui::LabelText("label", "Value");

        VanGui::SeparatorText("Inputs");

        {
            // If you want to use InputText() with std::string or any custom dynamic string type:
            // - For std::string: use the wrapper in misc/cpp/vangui_stdlib.h/.cpp
            // - Otherwise, see the 'VanGUI Demo->Widgets->Text Input->Resize Callback' for using VanGuiInputTextFlags_CallbackResize.
            VANGUI_DEMO_MARKER("Widgets/Basic/InputText");
            static char str0[128] = "Hello, world!";
            VanGui::InputText("input text", str0, VAN_COUNTOF(str0));
            VanGui::SameLine(); HelpMarker(
                "USER:\n"
                "Hold Shift or use mouse to select text.\n"
                "Ctrl+Left/Right to word jump.\n"
                "Ctrl+A or Double-Click to select all.\n"
                "Ctrl+X,Ctrl+C,Ctrl+V for clipboard.\n"
                "Ctrl+Z to undo, Ctrl+Y/Ctrl+Shift+Z to redo.\n"
                "Escape to revert.\n\n"
                "PROGRAMMER:\n"
                "You can use the VanGuiInputTextFlags_CallbackResize facility if you need to wire InputText() "
                "to a dynamic string type. See misc/cpp/vangui_stdlib.h for an example (this is not demonstrated "
                "in vangui_demo.cpp).");

            static char str1[128] = "";
            VanGui::InputTextWithHint("input text (w/ hint)", "enter text here", str1, VAN_COUNTOF(str1));

            VANGUI_DEMO_MARKER("Widgets/Basic/InputInt, InputFloat");
            static int i0 = 123;
            VanGui::InputInt("input int", &i0);

            static float f0 = 0.001f;
            VanGui::InputFloat("input float", &f0, 0.01f, 1.0f, "%.3f");

            static double d0 = 999999.00000001;
            VanGui::InputDouble("input double", &d0, 0.01f, 1.0f, "%.8f");

            static float f1 = 1.e10f;
            VanGui::InputFloat("input scientific", &f1, 0.0f, 0.0f, "%e");
            VanGui::SameLine(); HelpMarker(
                "You can input value using the scientific notation,\n"
                "  e.g. \"1e+8\" becomes \"100000000\".");

            static float vec4a[4] = { 0.10f, 0.20f, 0.30f, 0.44f };
            VanGui::InputFloat3("input float3", vec4a);
        }

        VanGui::SeparatorText("Drags");

        {
            VANGUI_DEMO_MARKER("Widgets/Basic/DragInt, DragFloat");
            static int i1 = 50, i2 = 42, i3 = 128;
            VanGui::DragInt("drag int", &i1, 1);
            VanGui::SameLine(); HelpMarker(
                "Click and drag to edit value.\n"
                "Hold Shift/Alt for faster/slower edit.\n"
                "Double-Click or Ctrl+Click to input value.");
            VanGui::DragInt("drag int 0..100", &i2, 1, 0, 100, "%d%%", VanGuiSliderFlags_AlwaysClamp);
            VanGui::DragInt("drag int wrap 100..200", &i3, 1, 100, 200, "%d", VanGuiSliderFlags_WrapAround);

            static float f1 = 1.00f, f2 = 0.0067f;
            VanGui::DragFloat("drag float", &f1, 0.005f);
            VanGui::DragFloat("drag small float", &f2, 0.0001f, 0.0f, 0.0f, "%.06f ns");
            //VanGui::DragFloat("drag wrap -1..1", &f3, 0.005f, -1.0f, 1.0f, nullptr, VanGuiSliderFlags_WrapAround);
        }

        VanGui::SeparatorText("Sliders");

        {
            VANGUI_DEMO_MARKER("Widgets/Basic/SliderInt, SliderFloat");
            static int i1 = 0;
            VanGui::SliderInt("slider int", &i1, -1, 3);
            VanGui::SameLine(); HelpMarker("Ctrl+Click to input value.");

            static float f1 = 0.123f, f2 = 0.0f;
            VanGui::SliderFloat("slider float", &f1, 0.0f, 1.0f, "ratio = %.3f");
            VanGui::SliderFloat("slider float (log)", &f2, -10.0f, 10.0f, "%.4f", VanGuiSliderFlags_Logarithmic);

            VANGUI_DEMO_MARKER("Widgets/Basic/SliderAngle");
            static float angle = 0.0f;
            VanGui::SliderAngle("slider angle", &angle);

            // Using the format string to display a name instead of an integer.
            // Here we completely omit '%d' from the format string, so it'll only display a name.
            // This technique can also be used with DragInt().
            VANGUI_DEMO_MARKER("Widgets/Basic/Slider (enum)");
            enum Element { Element_Fire, Element_Earth, Element_Air, Element_Water, Element_COUNT };
            static int elem = Element_Fire;
            const char* elems_names[Element_COUNT] = { "Fire", "Earth", "Air", "Water" };
            const char* elem_name = (elem >= 0 && elem < Element_COUNT) ? elems_names[elem] : "Unknown";
            VanGui::SliderInt("slider enum", &elem, 0, Element_COUNT - 1, elem_name); // Use VanGuiSliderFlags_NoInput flag to disable Ctrl+Click here.
            VanGui::SameLine(); HelpMarker("Using the format string parameter to display a name instead of the underlying integer.");
        }

        VanGui::SeparatorText("Selectors/Pickers");

        {
            VANGUI_DEMO_MARKER("Widgets/Basic/ColorEdit3, ColorEdit4");
            static float col1[3] = { 1.0f, 0.0f, 0.2f };
            static float col2[4] = { 0.4f, 0.7f, 0.0f, 0.5f };
            VanGui::ColorEdit3("color 1", col1);
            VanGui::SameLine(); HelpMarker(
                "Click on the color square to open a color picker.\n"
                "Click and hold to use drag and drop.\n"
                "Right-Click on the color square to show options.\n"
                "Ctrl+Click on individual component to input value.\n");

            VanGui::ColorEdit4("color 2", col2);
        }

        {
            // Using the _simplified_ one-liner Combo() api here
            // See "Combo" section for examples of how to use the more flexible BeginCombo()/EndCombo() api.
            VANGUI_DEMO_MARKER("Widgets/Basic/Combo");
            const char* items[] = { "AAAA", "BBBB", "CCCC", "DDDD", "EEEE", "FFFF", "GGGG", "HHHH", "IIIIIII", "JJJJ", "KKKKKKK" };
            static int item_current = 0;
            VanGui::Combo("combo", &item_current, items, VAN_COUNTOF(items));
            VanGui::SameLine(); HelpMarker(
                "Using the simplified one-liner Combo API here.\n"
                "Refer to the \"Combo\" section below for an explanation of how to use the more flexible and general BeginCombo/EndCombo API.");
        }

        {
            // Using the _simplified_ one-liner ListBox() api here
            // See "List boxes" section for examples of how to use the more flexible BeginListBox()/EndListBox() api.
            VANGUI_DEMO_MARKER("Widgets/Basic/ListBox");
            const char* items[] = { "Apple", "Banana", "Cherry", "Kiwi", "Mango", "Orange", "Pineapple", "Strawberry", "Watermelon" };
            static int item_current = 1;
            VanGui::ListBox("listbox", &item_current, items, VAN_COUNTOF(items), 4);
            VanGui::SameLine(); HelpMarker(
                "Using the simplified one-liner ListBox API here.\n"
                "Refer to the \"List boxes\" section below for an explanation of how to use the more flexible and general BeginListBox/EndListBox API.");
        }

        // Testing VanGuiOnceUponAFrame helper.
        //static VanGuiOnceUponAFrame once;
        //for (int i = 0; i < 5; i++)
        //    if (once)
        //        VanGui::Text("This will be displayed only once.");

        VanGui::TreePop();
    }
}

//-----------------------------------------------------------------------------
// [SECTION] DemoWindowWidgetsBullets()
//-----------------------------------------------------------------------------

static void DemoWindowWidgetsBullets()
{
    if (VanGui::TreeNode("Bullets"))
    {
        VANGUI_DEMO_MARKER("Widgets/Bullets");
        VanGui::BulletText("Bullet point 1");
        VanGui::BulletText("Bullet point 2\nOn multiple lines");
        if (VanGui::TreeNode("Tree node"))
        {
            VanGui::BulletText("Another bullet point");
            VanGui::TreePop();
        }
        VanGui::Bullet(); VanGui::Text("Bullet point 3 (two calls)");
        VanGui::Bullet(); VanGui::SmallButton("Button");
        VanGui::TreePop();
    }
}

//-----------------------------------------------------------------------------
// [SECTION] DemoWindowWidgetsCollapsingHeaders()
//-----------------------------------------------------------------------------

static void DemoWindowWidgetsCollapsingHeaders()
{
    if (VanGui::TreeNode("Collapsing Headers"))
    {
        VANGUI_DEMO_MARKER("Widgets/Collapsing Headers");
        static bool closable_group = true;
        VanGui::Checkbox("Show 2nd header", &closable_group);
        if (VanGui::CollapsingHeader("Header", VanGuiTreeNodeFlags_None))
        {
            VanGui::Text("IsItemHovered: %d", VanGui::IsItemHovered());
            for (int i = 0; i < 5; i++)
                VanGui::Text("Some content %d", i);
        }
        if (VanGui::CollapsingHeader("Header with a close button", &closable_group))
        {
            VanGui::Text("IsItemHovered: %d", VanGui::IsItemHovered());
            for (int i = 0; i < 5; i++)
                VanGui::Text("More content %d", i);
        }
        /*
        if (VanGui::CollapsingHeader("Header with a bullet", VanGuiTreeNodeFlags_Bullet))
            VanGui::Text("IsItemHovered: %d", VanGui::IsItemHovered());
        */
        VanGui::TreePop();
    }
}

//-----------------------------------------------------------------------------
// [SECTION] DemoWindowWidgetsColorAndPickers()
//-----------------------------------------------------------------------------

static void DemoWindowWidgetsColorAndPickers()
{
    if (VanGui::TreeNode("Color/Picker Widgets"))
    {
        VANGUI_DEMO_MARKER("Widgets/Color");
        static VanVec4 color = VanVec4(114.0f / 255.0f, 144.0f / 255.0f, 154.0f / 255.0f, 200.0f / 255.0f);
        static VanGuiColorEditFlags base_flags = VanGuiColorEditFlags_None;

        VanGui::SeparatorText("Options");
        VanGui::CheckboxFlags("VanGuiColorEditFlags_NoAlpha", &base_flags, VanGuiColorEditFlags_NoAlpha);
        VanGui::CheckboxFlags("VanGuiColorEditFlags_AlphaOpaque", &base_flags, VanGuiColorEditFlags_AlphaOpaque);
        VanGui::CheckboxFlags("VanGuiColorEditFlags_AlphaNoBg", &base_flags, VanGuiColorEditFlags_AlphaNoBg);
        VanGui::CheckboxFlags("VanGuiColorEditFlags_AlphaPreviewHalf", &base_flags, VanGuiColorEditFlags_AlphaPreviewHalf);
        VanGui::CheckboxFlags("VanGuiColorEditFlags_NoOptions", &base_flags, VanGuiColorEditFlags_NoOptions); VanGui::SameLine(); HelpMarker("Right-click on the individual color widget to show options.");
        VanGui::CheckboxFlags("VanGuiColorEditFlags_NoDragDrop", &base_flags, VanGuiColorEditFlags_NoDragDrop);
        VanGui::CheckboxFlags("VanGuiColorEditFlags_NoColorMarkers", &base_flags, VanGuiColorEditFlags_NoColorMarkers);
        VanGui::CheckboxFlags("VanGuiColorEditFlags_HDR", &base_flags, VanGuiColorEditFlags_HDR); VanGui::SameLine(); HelpMarker("Currently all this does is to lift the 0..1 limits on dragging widgets.");

        VANGUI_DEMO_MARKER("Widgets/Color/ColorEdit");
        VanGui::SeparatorText("Inline color editor");
        VanGui::Text("Color widget:");
        VanGui::SameLine(); HelpMarker(
            "Click on the color square to open a color picker.\n"
            "Ctrl+Click on individual component to input value.\n");
        VanGui::ColorEdit3("MyColor##1", reinterpret_cast<float*>(&color), base_flags);

        VANGUI_DEMO_MARKER("Widgets/Color/ColorEdit (HSV, with Alpha)");
        VanGui::Text("Color widget HSV with Alpha:");
        VanGui::ColorEdit4("MyColor##2", reinterpret_cast<float*>(&color), VanGuiColorEditFlags_DisplayHSV | base_flags);

        VANGUI_DEMO_MARKER("Widgets/Color/ColorEdit (float display)");
        VanGui::Text("Color widget with Float Display:");
        VanGui::ColorEdit4("MyColor##2f", reinterpret_cast<float*>(&color), VanGuiColorEditFlags_Float | base_flags);

        VANGUI_DEMO_MARKER("Widgets/Color/ColorButton (with Picker)");
        VanGui::Text("Color button with Picker:");
        VanGui::SameLine(); HelpMarker(
            "With the VanGuiColorEditFlags_NoInputs flag you can hide all the slider/text inputs.\n"
            "With the VanGuiColorEditFlags_NoLabel flag you can pass a non-empty label which will only "
            "be used for the tooltip and picker popup.");
        VanGui::ColorEdit4("MyColor##3", reinterpret_cast<float*>(&color), VanGuiColorEditFlags_NoInputs | VanGuiColorEditFlags_NoLabel | base_flags);

        VANGUI_DEMO_MARKER("Widgets/Color/ColorButton (with custom Picker popup)");
        VanGui::Text("Color button with Custom Picker Popup:");

        // Generate a default palette. The palette will persist and can be edited.
        static bool saved_palette_init = true;
        static VanVec4 saved_palette[32] = {};
        if (saved_palette_init)
        {
            for (int n = 0; n < VAN_COUNTOF(saved_palette); n++)
            {
                VanGui::ColorConvertHSVtoRGB(n / 31.0f, 0.8f, 0.8f,
                    saved_palette[n].x, saved_palette[n].y, saved_palette[n].z);
                saved_palette[n].w = 1.0f; // Alpha
            }
            saved_palette_init = false;
        }

        static VanVec4 backup_color;
        bool open_popup = VanGui::ColorButton("MyColor##3b", color, base_flags);
        VanGui::SameLine(0, VanGui::GetStyle().ItemInnerSpacing.x);
        open_popup |= VanGui::Button("Palette");
        if (open_popup)
        {
            VanGui::OpenPopup("mypicker");
            backup_color = color;
        }
        if (VanGui::BeginPopup("mypicker"))
        {
            VanGui::Text("MY CUSTOM COLOR PICKER WITH AN AMAZING PALETTE!");
            VanGui::Separator();
            VanGui::ColorPicker4("##picker", reinterpret_cast<float*>(&color), base_flags | VanGuiColorEditFlags_NoSidePreview | VanGuiColorEditFlags_NoSmallPreview);
            VanGui::SameLine();

            VanGui::BeginGroup(); // Lock X position
            VanGui::Text("Current");
            VanGui::ColorButton("##current", color, VanGuiColorEditFlags_NoPicker | VanGuiColorEditFlags_AlphaPreviewHalf, VanVec2(60, 40));
            VanGui::Text("Previous");
            if (VanGui::ColorButton("##previous", backup_color, VanGuiColorEditFlags_NoPicker | VanGuiColorEditFlags_AlphaPreviewHalf, VanVec2(60, 40)))
                color = backup_color;
            VanGui::Separator();
            VanGui::Text("Palette");
            for (int n = 0; n < VAN_COUNTOF(saved_palette); n++)
            {
                VanGui::PushID(n);
                if ((n % 8) != 0)
                    VanGui::SameLine(0.0f, VanGui::GetStyle().ItemSpacing.y);

                VanGuiColorEditFlags palette_button_flags = VanGuiColorEditFlags_NoAlpha | VanGuiColorEditFlags_NoPicker | VanGuiColorEditFlags_NoTooltip;
                if (VanGui::ColorButton("##palette", saved_palette[n], palette_button_flags, VanVec2(20, 20)))
                    color = VanVec4(saved_palette[n].x, saved_palette[n].y, saved_palette[n].z, color.w); // Preserve alpha!

                // Allow user to drop colors into each palette entry. Note that ColorButton() is already a
                // drag source by default, unless specifying the VanGuiColorEditFlags_NoDragDrop flag.
                if (VanGui::BeginDragDropTarget())
                {
                    if (const VanGuiPayload* payload = VanGui::AcceptDragDropPayload(VANGUI_PAYLOAD_TYPE_COLOR_3F))
                        memcpy(static_cast<float*>(static_cast<void*>(&saved_palette[n])), payload->Data, sizeof(float) * 3);
                    if (const VanGuiPayload* payload = VanGui::AcceptDragDropPayload(VANGUI_PAYLOAD_TYPE_COLOR_4F))
                        memcpy(static_cast<float*>(static_cast<void*>(&saved_palette[n])), payload->Data, sizeof(float) * 4);
                    VanGui::EndDragDropTarget();
                }

                VanGui::PopID();
            }
            VanGui::EndGroup();
            VanGui::EndPopup();
        }

        VANGUI_DEMO_MARKER("Widgets/Color/ColorButton (simple)");
        VanGui::Text("Color button only:");
        static bool no_border = false;
        VanGui::Checkbox("VanGuiColorEditFlags_NoBorder", &no_border);
        VanGui::ColorButton("MyColor##3c", *(VanVec4*)&color, base_flags | (no_border ? VanGuiColorEditFlags_NoBorder : 0), VanVec2(80, 80));

        VANGUI_DEMO_MARKER("Widgets/Color/ColorPicker");
        VanGui::SeparatorText("Color picker");

        static bool ref_color = false;
        static VanVec4 ref_color_v(1.0f, 0.0f, 1.0f, 0.5f);
        static int picker_mode = 0;
        static int display_mode = 0;
        static VanGuiColorEditFlags color_picker_flags = VanGuiColorEditFlags_AlphaBar;

        VanGui::PushID("Color picker");
        VanGui::CheckboxFlags("VanGuiColorEditFlags_NoAlpha", &color_picker_flags, VanGuiColorEditFlags_NoAlpha);
        VanGui::CheckboxFlags("VanGuiColorEditFlags_AlphaBar", &color_picker_flags, VanGuiColorEditFlags_AlphaBar);
        VanGui::CheckboxFlags("VanGuiColorEditFlags_NoSidePreview", &color_picker_flags, VanGuiColorEditFlags_NoSidePreview);
        if (color_picker_flags & VanGuiColorEditFlags_NoSidePreview)
        {
            VanGui::SameLine();
            VanGui::Checkbox("With Ref Color", &ref_color);
            if (ref_color)
            {
                VanGui::SameLine();
                VanGui::ColorEdit4("##RefColor", &ref_color_v.x, VanGuiColorEditFlags_NoInputs | base_flags);
            }
        }

        VanGui::Combo("Picker Mode", &picker_mode, "Auto/Current\0VanGuiColorEditFlags_PickerHueBar\0VanGuiColorEditFlags_PickerHueWheel\0");
        VanGui::SameLine(); HelpMarker("When not specified explicitly, user can right-click the picker to change mode.");

        VanGui::Combo("Display Mode", &display_mode, "Auto/Current\0VanGuiColorEditFlags_NoInputs\0VanGuiColorEditFlags_DisplayRGB\0VanGuiColorEditFlags_DisplayHSV\0VanGuiColorEditFlags_DisplayHex\0");
        VanGui::SameLine(); HelpMarker(
            "ColorEdit defaults to displaying RGB inputs if you don't specify a display mode, "
            "but the user can change it with a right-click on those inputs.\n\nColorPicker defaults to displaying RGB+HSV+Hex "
            "if you don't specify a display mode.\n\nYou can change the defaults using SetColorEditOptions().");

        VanGuiColorEditFlags flags = base_flags | color_picker_flags;
        if (picker_mode == 1)  flags |= VanGuiColorEditFlags_PickerHueBar;
        if (picker_mode == 2)  flags |= VanGuiColorEditFlags_PickerHueWheel;
        if (display_mode == 1) flags |= VanGuiColorEditFlags_NoInputs;       // Disable all RGB/HSV/Hex displays
        if (display_mode == 2) flags |= VanGuiColorEditFlags_DisplayRGB;     // Override display mode
        if (display_mode == 3) flags |= VanGuiColorEditFlags_DisplayHSV;
        if (display_mode == 4) flags |= VanGuiColorEditFlags_DisplayHex;
        VanGui::ColorPicker4("MyColor##4", (float*)&color, flags, ref_color ? &ref_color_v.x : nullptr);

        VanGui::Text("Set defaults in code:");
        VanGui::SameLine(); HelpMarker(
            "SetColorEditOptions() is designed to allow you to set boot-time default.\n"
            "We don't have Push/Pop functions because you can force options on a per-widget basis if needed, "
            "and the user can change non-forced ones with the options menu.\nWe don't have a getter to avoid "
            "encouraging you to persistently save values that aren't forward-compatible.");
        if (VanGui::Button("Default: Uint8 + HSV + Hue Bar"))
            VanGui::SetColorEditOptions(VanGuiColorEditFlags_Uint8 | VanGuiColorEditFlags_DisplayHSV | VanGuiColorEditFlags_PickerHueBar);
        if (VanGui::Button("Default: Float + HDR + Hue Wheel"))
            VanGui::SetColorEditOptions(VanGuiColorEditFlags_Float | VanGuiColorEditFlags_HDR | VanGuiColorEditFlags_PickerHueWheel);

        // Always display a small version of both types of pickers
        // (that's in order to make it more visible in the demo to people who are skimming quickly through it)
        VanGui::Text("Both types:");
        float w = (VanGui::GetContentRegionAvail().x - VanGui::GetStyle().ItemSpacing.y) * 0.40f;
        VanGui::SetNextItemWidth(w);
        VanGui::ColorPicker3("##MyColor##5", reinterpret_cast<float*>(&color), VanGuiColorEditFlags_PickerHueBar | VanGuiColorEditFlags_NoSidePreview | VanGuiColorEditFlags_NoInputs | VanGuiColorEditFlags_NoAlpha);
        VanGui::SameLine();
        VanGui::SetNextItemWidth(w);
        VanGui::ColorPicker3("##MyColor##6", reinterpret_cast<float*>(&color), VanGuiColorEditFlags_PickerHueWheel | VanGuiColorEditFlags_NoSidePreview | VanGuiColorEditFlags_NoInputs | VanGuiColorEditFlags_NoAlpha);
        VanGui::PopID();

        // HSV encoded support (to avoid RGB<>HSV round trips and singularities when S==0 or V==0)
        static VanVec4 color_hsv(0.23f, 1.0f, 1.0f, 1.0f); // Stored as HSV!
        VanGui::Spacing();
        VanGui::Text("HSV encoded colors");
        VanGui::SameLine(); HelpMarker(
            "By default, colors are given to ColorEdit and ColorPicker in RGB, but VanGuiColorEditFlags_InputHSV "
            "allows you to store colors as HSV and pass them to ColorEdit and ColorPicker as HSV. This comes with the "
            "added benefit that you can manipulate hue values with the picker even when saturation or value are zero.");
        VanGui::Text("Color widget with InputHSV:");
        VanGui::ColorEdit4("HSV shown as RGB##1", reinterpret_cast<float*>(&color_hsv), VanGuiColorEditFlags_DisplayRGB | VanGuiColorEditFlags_InputHSV | VanGuiColorEditFlags_Float);
        VanGui::ColorEdit4("HSV shown as HSV##1", reinterpret_cast<float*>(&color_hsv), VanGuiColorEditFlags_DisplayHSV | VanGuiColorEditFlags_InputHSV | VanGuiColorEditFlags_Float);
        VanGui::DragFloat4("Raw HSV values", reinterpret_cast<float*>(&color_hsv), 0.01f, 0.0f, 1.0f);

        VanGui::TreePop();
    }
}

//-----------------------------------------------------------------------------
// [SECTION] DemoWindowWidgetsComboBoxes()
//-----------------------------------------------------------------------------

static void DemoWindowWidgetsComboBoxes()
{
    if (VanGui::TreeNode("Combo"))
    {
        VANGUI_DEMO_MARKER("Widgets/Combo");
        // Combo Boxes are also called "Dropdown" in other systems
        // Expose flags as checkbox for the demo
        static VanGuiComboFlags flags = 0;
        VanGui::CheckboxFlags("VanGuiComboFlags_PopupAlignLeft", &flags, VanGuiComboFlags_PopupAlignLeft);
        VanGui::SameLine(); HelpMarker("Only makes a difference if the popup is larger than the combo");
        if (VanGui::CheckboxFlags("VanGuiComboFlags_NoArrowButton", &flags, VanGuiComboFlags_NoArrowButton))
            flags &= ~VanGuiComboFlags_NoPreview;     // Clear incompatible flags
        if (VanGui::CheckboxFlags("VanGuiComboFlags_NoPreview", &flags, VanGuiComboFlags_NoPreview))
            flags &= ~(VanGuiComboFlags_NoArrowButton | VanGuiComboFlags_WidthFitPreview); // Clear incompatible flags
        if (VanGui::CheckboxFlags("VanGuiComboFlags_WidthFitPreview", &flags, VanGuiComboFlags_WidthFitPreview))
            flags &= ~VanGuiComboFlags_NoPreview;

        // Override default popup height
        if (VanGui::CheckboxFlags("VanGuiComboFlags_HeightSmall", &flags, VanGuiComboFlags_HeightSmall))
            flags &= ~(VanGuiComboFlags_HeightMask_ & ~VanGuiComboFlags_HeightSmall);
        if (VanGui::CheckboxFlags("VanGuiComboFlags_HeightRegular", &flags, VanGuiComboFlags_HeightRegular))
            flags &= ~(VanGuiComboFlags_HeightMask_ & ~VanGuiComboFlags_HeightRegular);
        if (VanGui::CheckboxFlags("VanGuiComboFlags_HeightLargest", &flags, VanGuiComboFlags_HeightLargest))
            flags &= ~(VanGuiComboFlags_HeightMask_ & ~VanGuiComboFlags_HeightLargest);

        // Using the generic BeginCombo() API, you have full control over how to display the combo contents.
        // (your selection data could be an index, a pointer to the object, an id for the object, a flag intrusively
        // stored in the object itself, etc.)
        const char* items[] = { "AAAA", "BBBB", "CCCC", "DDDD", "EEEE", "FFFF", "GGGG", "HHHH", "IIII", "JJJJ", "KKKK", "LLLLLLL", "MMMM", "OOOOOOO" };
        static int item_selected_idx = 0; // Here we store our selection data as an index.

        // Pass in the preview value visible before opening the combo (it could technically be different contents or not pulled from items[])
        const char* combo_preview_value = items[item_selected_idx];
        if (VanGui::BeginCombo("combo 1", combo_preview_value, flags))
        {
            for (int n = 0; n < VAN_COUNTOF(items); n++)
            {
                const bool is_selected = (item_selected_idx == n);
                if (VanGui::Selectable(items[n], is_selected))
                    item_selected_idx = n;

                // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                if (is_selected)
                    VanGui::SetItemDefaultFocus();
            }
            VanGui::EndCombo();
        }

        // Show case embedding a filter using a simple trick: displaying the filter inside combo contents.
        // See https://github.com/ocornut/vangui/issues/718 for advanced/esoteric alternatives.
        if (VanGui::BeginCombo("combo 2 (w/ filter)", combo_preview_value, flags))
        {
            static VanGuiTextFilter filter;
            if (VanGui::IsWindowAppearing())
            {
                VanGui::SetKeyboardFocusHere();
                filter.Clear();
            }
            VanGui::SetNextItemShortcut(VanGuiMod_Ctrl | VanGuiKey_F);
            filter.Draw("##Filter", -FLT_MIN);

            for (int n = 0; n < VAN_COUNTOF(items); n++)
            {
                const bool is_selected = (item_selected_idx == n);
                if (filter.PassFilter(items[n]))
                    if (VanGui::Selectable(items[n], is_selected))
                        item_selected_idx = n;
            }
            VanGui::EndCombo();
        }

        VanGui::Spacing();
        VanGui::SeparatorText("One-liner variants");
        HelpMarker("The Combo() function is not greatly useful apart from cases were you want to embed all options in a single strings.\nFlags above don't apply to this section.");

        // Simplified one-liner Combo() API, using values packed in a single constant string
        // This is a convenience for when the selection set is small and known at compile-time.
        static int item_current_2 = 0;
        VanGui::Combo("combo 3 (one-liner)", &item_current_2, "aaaa\0bbbb\0cccc\0dddd\0eeee\0\0");

        // Simplified one-liner Combo() using an array of const char*
        // This is not very useful (may obsolete): prefer using BeginCombo()/EndCombo() for full control.
        static int item_current_3 = -1; // If the selection isn't within 0..count, Combo won't display a preview
        VanGui::Combo("combo 4 (array)", &item_current_3, items, VAN_COUNTOF(items));

        // Simplified one-liner Combo() using an accessor function
        static int item_current_4 = 0;
        VanGui::Combo("combo 5 (function)", &item_current_4, [](void* data, int n) { return ((const char**)data)[n]; }, items, VAN_COUNTOF(items));

        VanGui::TreePop();
    }
}

//-----------------------------------------------------------------------------
// [SECTION] DemoWindowWidgetsDataTypes()
//-----------------------------------------------------------------------------

static void DemoWindowWidgetsDataTypes()
{
    if (VanGui::TreeNode("Data Types"))
    {
        VANGUI_DEMO_MARKER("Widgets/Data Types");
        // DragScalar/InputScalar/SliderScalar functions allow various data types
        // - signed/unsigned
        // - 8/16/32/64-bits
        // - integer/float/double
        // To avoid polluting the public API with all possible combinations, we use the VanGuiDataType enum
        // to pass the type, and passing all arguments by pointer.
        // This is the reason the test code below creates local variables to hold "zero" "one" etc. for each type.
        // In practice, if you frequently use a given type that is not covered by the normal API entry points,
        // you can wrap it yourself inside a 1 line function which can take typed argument as value instead of void*,
        // and then pass their address to the generic function. For example:
        //   bool MySliderU64(const char *label, u64* value, u64 min = 0, u64 max = 0, const char* format = "%lld")
        //   {
        //      return SliderScalar(label, VanGuiDataType_U64, value, &min, &max, format);
        //   }

        // Setup limits (as helper variables so we can take their address, as explained above)
        // Note: SliderScalar() functions have a maximum usable range of half the natural type maximum, hence the /2.
        #ifndef LLONG_MIN
        VanS64 LLONG_MIN = -9223372036854775807LL - 1;
        VanS64 LLONG_MAX = 9223372036854775807LL;
        VanU64 ULLONG_MAX = (2ULL * 9223372036854775807LL + 1);
        #endif
        const char    s8_zero  = 0,   s8_one  = 1,   s8_fifty  = 50, s8_min  = -128,        s8_max = 127;
        const VanU8    u8_zero  = 0,   u8_one  = 1,   u8_fifty  = 50, u8_min  = 0,           u8_max = 255;
        const short   s16_zero = 0,   s16_one = 1,   s16_fifty = 50, s16_min = -32768,      s16_max = 32767;
        const VanU16   u16_zero = 0,   u16_one = 1,   u16_fifty = 50, u16_min = 0,           u16_max = 65535;
        const VanS32   s32_zero = 0,   s32_one = 1,   s32_fifty = 50, s32_min = INT_MIN/2,   s32_max = INT_MAX/2,    s32_hi_a = INT_MAX/2 - 100,    s32_hi_b = INT_MAX/2;
        const VanU32   u32_zero = 0,   u32_one = 1,   u32_fifty = 50, u32_min = 0,           u32_max = UINT_MAX/2,   u32_hi_a = UINT_MAX/2 - 100,   u32_hi_b = UINT_MAX/2;
        const VanS64   s64_zero = 0,   s64_one = 1,   s64_fifty = 50, s64_min = LLONG_MIN/2, s64_max = LLONG_MAX/2,  s64_hi_a = LLONG_MAX/2 - 100,  s64_hi_b = LLONG_MAX/2;
        const VanU64   u64_zero = 0,   u64_one = 1,   u64_fifty = 50, u64_min = 0,           u64_max = ULLONG_MAX/2, u64_hi_a = ULLONG_MAX/2 - 100, u64_hi_b = ULLONG_MAX/2;
        const float   f32_zero = 0.f, f32_one = 1.f, f32_lo_a = -10000000000.0f, f32_hi_a = +10000000000.0f;
        const double  f64_zero = 0.,  f64_one = 1.,  f64_lo_a = -1000000000000000.0, f64_hi_a = +1000000000000000.0;

        // State
        static char   s8_v  = 127;
        static VanU8   u8_v  = 255;
        static short  s16_v = 32767;
        static VanU16  u16_v = 65535;
        static VanS32  s32_v = -1;
        static VanU32  u32_v = static_cast<VanU32>(-1);
        static VanS64  s64_v = -1;
        static VanU64  u64_v = static_cast<VanU64>(-1);
        static float  f32_v = 0.123f;
        static double f64_v = 90000.01234567890123456789;

        const float drag_speed = 0.2f;
        static bool drag_clamp = false;
        VANGUI_DEMO_MARKER("Widgets/Data Types/Drags");
        VanGui::SeparatorText("Drags");
        VanGui::Checkbox("Clamp integers to 0..50", &drag_clamp);
        VanGui::SameLine(); HelpMarker(
            "As with every widget in dear vangui, we never modify values unless there is a user interaction.\n"
            "You can override the clamping limits by using Ctrl+Click to input a value.");
        VanGui::DragScalar("drag s8",        VanGuiDataType_S8,     &s8_v,  drag_speed, drag_clamp ? &s8_zero  : nullptr, drag_clamp ? &s8_fifty  : nullptr);
        VanGui::DragScalar("drag u8",        VanGuiDataType_U8,     &u8_v,  drag_speed, drag_clamp ? &u8_zero  : nullptr, drag_clamp ? &u8_fifty  : nullptr, "%u ms");
        VanGui::DragScalar("drag s16",       VanGuiDataType_S16,    &s16_v, drag_speed, drag_clamp ? &s16_zero : nullptr, drag_clamp ? &s16_fifty : nullptr);
        VanGui::DragScalar("drag u16",       VanGuiDataType_U16,    &u16_v, drag_speed, drag_clamp ? &u16_zero : nullptr, drag_clamp ? &u16_fifty : nullptr, "%u ms");
        VanGui::DragScalar("drag s32",       VanGuiDataType_S32,    &s32_v, drag_speed, drag_clamp ? &s32_zero : nullptr, drag_clamp ? &s32_fifty : nullptr);
        VanGui::DragScalar("drag s32 hex",   VanGuiDataType_S32,    &s32_v, drag_speed, drag_clamp ? &s32_zero : nullptr, drag_clamp ? &s32_fifty : nullptr, "0x%08X");
        VanGui::DragScalar("drag u32",       VanGuiDataType_U32,    &u32_v, drag_speed, drag_clamp ? &u32_zero : nullptr, drag_clamp ? &u32_fifty : nullptr, "%u ms");
        VanGui::DragScalar("drag s64",       VanGuiDataType_S64,    &s64_v, drag_speed, drag_clamp ? &s64_zero : nullptr, drag_clamp ? &s64_fifty : nullptr);
        VanGui::DragScalar("drag u64",       VanGuiDataType_U64,    &u64_v, drag_speed, drag_clamp ? &u64_zero : nullptr, drag_clamp ? &u64_fifty : nullptr);
        VanGui::DragScalar("drag float",     VanGuiDataType_Float,  &f32_v, 0.005f,  &f32_zero, &f32_one, "%f");
        VanGui::DragScalar("drag float log", VanGuiDataType_Float,  &f32_v, 0.005f,  &f32_zero, &f32_one, "%f", VanGuiSliderFlags_Logarithmic);
        VanGui::DragScalar("drag double",    VanGuiDataType_Double, &f64_v, 0.0005f, &f64_zero, nullptr,     "%.10f grams");
        VanGui::DragScalar("drag double log",VanGuiDataType_Double, &f64_v, 0.0005f, &f64_zero, &f64_one, "0 < %.10f < 1", VanGuiSliderFlags_Logarithmic);

        VANGUI_DEMO_MARKER("Widgets/Data Types/Sliders");
        VanGui::SeparatorText("Sliders");
        VanGui::SliderScalar("slider s8 full",       VanGuiDataType_S8,     &s8_v,  &s8_min,   &s8_max,   "%d");
        VanGui::SliderScalar("slider u8 full",       VanGuiDataType_U8,     &u8_v,  &u8_min,   &u8_max,   "%u");
        VanGui::SliderScalar("slider s16 full",      VanGuiDataType_S16,    &s16_v, &s16_min,  &s16_max,  "%d");
        VanGui::SliderScalar("slider u16 full",      VanGuiDataType_U16,    &u16_v, &u16_min,  &u16_max,  "%u");
        VanGui::SliderScalar("slider s32 low",       VanGuiDataType_S32,    &s32_v, &s32_zero, &s32_fifty,"%d");
        VanGui::SliderScalar("slider s32 high",      VanGuiDataType_S32,    &s32_v, &s32_hi_a, &s32_hi_b, "%d");
        VanGui::SliderScalar("slider s32 full",      VanGuiDataType_S32,    &s32_v, &s32_min,  &s32_max,  "%d");
        VanGui::SliderScalar("slider s32 hex",       VanGuiDataType_S32,    &s32_v, &s32_zero, &s32_fifty, "0x%04X");
        VanGui::SliderScalar("slider u32 low",       VanGuiDataType_U32,    &u32_v, &u32_zero, &u32_fifty,"%u");
        VanGui::SliderScalar("slider u32 high",      VanGuiDataType_U32,    &u32_v, &u32_hi_a, &u32_hi_b, "%u");
        VanGui::SliderScalar("slider u32 full",      VanGuiDataType_U32,    &u32_v, &u32_min,  &u32_max,  "%u");
        VanGui::SliderScalar("slider s64 low",       VanGuiDataType_S64,    &s64_v, &s64_zero, &s64_fifty,"%" PRId64);
        VanGui::SliderScalar("slider s64 high",      VanGuiDataType_S64,    &s64_v, &s64_hi_a, &s64_hi_b, "%" PRId64);
        VanGui::SliderScalar("slider s64 full",      VanGuiDataType_S64,    &s64_v, &s64_min,  &s64_max,  "%" PRId64);
        VanGui::SliderScalar("slider u64 low",       VanGuiDataType_U64,    &u64_v, &u64_zero, &u64_fifty,"%" PRIu64 " ms");
        VanGui::SliderScalar("slider u64 high",      VanGuiDataType_U64,    &u64_v, &u64_hi_a, &u64_hi_b, "%" PRIu64 " ms");
        VanGui::SliderScalar("slider u64 full",      VanGuiDataType_U64,    &u64_v, &u64_min,  &u64_max,  "%" PRIu64 " ms");
        VanGui::SliderScalar("slider float low",     VanGuiDataType_Float,  &f32_v, &f32_zero, &f32_one);
        VanGui::SliderScalar("slider float low log", VanGuiDataType_Float,  &f32_v, &f32_zero, &f32_one,  "%.10f", VanGuiSliderFlags_Logarithmic);
        VanGui::SliderScalar("slider float high",    VanGuiDataType_Float,  &f32_v, &f32_lo_a, &f32_hi_a, "%e");
        VanGui::SliderScalar("slider double low",    VanGuiDataType_Double, &f64_v, &f64_zero, &f64_one,  "%.10f grams");
        VanGui::SliderScalar("slider double low log",VanGuiDataType_Double, &f64_v, &f64_zero, &f64_one,  "%.10f", VanGuiSliderFlags_Logarithmic);
        VanGui::SliderScalar("slider double high",   VanGuiDataType_Double, &f64_v, &f64_lo_a, &f64_hi_a, "%e grams");

        VanGui::SeparatorText("Sliders (reverse)");
        VanGui::SliderScalar("slider s8 reverse",    VanGuiDataType_S8,   &s8_v,  &s8_max,    &s8_min,   "%d");
        VanGui::SliderScalar("slider u8 reverse",    VanGuiDataType_U8,   &u8_v,  &u8_max,    &u8_min,   "%u");
        VanGui::SliderScalar("slider s32 reverse",   VanGuiDataType_S32,  &s32_v, &s32_fifty, &s32_zero, "%d");
        VanGui::SliderScalar("slider u32 reverse",   VanGuiDataType_U32,  &u32_v, &u32_fifty, &u32_zero, "%u");
        VanGui::SliderScalar("slider s64 reverse",   VanGuiDataType_S64,  &s64_v, &s64_fifty, &s64_zero, "%" PRId64);
        VanGui::SliderScalar("slider u64 reverse",   VanGuiDataType_U64,  &u64_v, &u64_fifty, &u64_zero, "%" PRIu64 " ms");

        VANGUI_DEMO_MARKER("Widgets/Data Types/Inputs");
        static bool inputs_step = true;
        static VanGuiInputTextFlags flags = VanGuiInputTextFlags_None;
        VanGui::SeparatorText("Inputs");
        VanGui::Checkbox("Show step buttons", &inputs_step);
        VanGui::CheckboxFlags("VanGuiInputTextFlags_ReadOnly", &flags, VanGuiInputTextFlags_ReadOnly);
        VanGui::CheckboxFlags("VanGuiInputTextFlags_ParseEmptyRefVal", &flags, VanGuiInputTextFlags_ParseEmptyRefVal);
        VanGui::CheckboxFlags("VanGuiInputTextFlags_DisplayEmptyRefVal", &flags, VanGuiInputTextFlags_DisplayEmptyRefVal);
        VanGui::InputScalar("input s8",      VanGuiDataType_S8,     &s8_v,  inputs_step ? &s8_one  : nullptr, nullptr, "%d", flags);
        VanGui::InputScalar("input u8",      VanGuiDataType_U8,     &u8_v,  inputs_step ? &u8_one  : nullptr, nullptr, "%u", flags);
        VanGui::InputScalar("input s16",     VanGuiDataType_S16,    &s16_v, inputs_step ? &s16_one : nullptr, nullptr, "%d", flags);
        VanGui::InputScalar("input u16",     VanGuiDataType_U16,    &u16_v, inputs_step ? &u16_one : nullptr, nullptr, "%u", flags);
        VanGui::InputScalar("input s32",     VanGuiDataType_S32,    &s32_v, inputs_step ? &s32_one : nullptr, nullptr, "%d", flags);
        VanGui::InputScalar("input s32 hex", VanGuiDataType_S32,    &s32_v, inputs_step ? &s32_one : nullptr, nullptr, "%04X", flags);
        VanGui::InputScalar("input u32",     VanGuiDataType_U32,    &u32_v, inputs_step ? &u32_one : nullptr, nullptr, "%u", flags);
        VanGui::InputScalar("input u32 hex", VanGuiDataType_U32,    &u32_v, inputs_step ? &u32_one : nullptr, nullptr, "%08X", flags);
        VanGui::InputScalar("input s64",     VanGuiDataType_S64,    &s64_v, inputs_step ? &s64_one : nullptr, nullptr, nullptr, flags);
        VanGui::InputScalar("input u64",     VanGuiDataType_U64,    &u64_v, inputs_step ? &u64_one : nullptr, nullptr, nullptr, flags);
        VanGui::InputScalar("input float",   VanGuiDataType_Float,  &f32_v, inputs_step ? &f32_one : nullptr, nullptr, nullptr, flags);
        VanGui::InputScalar("input double",  VanGuiDataType_Double, &f64_v, inputs_step ? &f64_one : nullptr, nullptr, nullptr, flags);

        VanGui::TreePop();
    }
}

//-----------------------------------------------------------------------------
// [SECTION] DemoWindowWidgetsDisableBlocks()
//-----------------------------------------------------------------------------

static void DemoWindowWidgetsDisableBlocks(VanGuiDemoWindowData* demo_data)
{
    if (VanGui::TreeNode("Disable Blocks"))
    {
        VANGUI_DEMO_MARKER("Widgets/Disable Blocks");
        VanGui::Checkbox("Disable entire section above", &demo_data->DisableSections);
        VanGui::SameLine(); HelpMarker("Demonstrate using BeginDisabled()/EndDisabled() across other sections.");
        VanGui::TreePop();
    }
}

//-----------------------------------------------------------------------------
// [SECTION] DemoWindowWidgetsDragAndDrop()
//-----------------------------------------------------------------------------

static void DemoWindowWidgetsDragAndDrop()
{
    if (VanGui::TreeNode("Drag and Drop"))
    {
        VANGUI_DEMO_MARKER("Widgets/Drag and drop");
        if (VanGui::TreeNode("Drag and drop in standard widgets"))
        {
            VANGUI_DEMO_MARKER("Widgets/Drag and drop/Standard widgets");
            // ColorEdit widgets automatically act as drag source and drag target.
            // They are using standardized payload strings VANGUI_PAYLOAD_TYPE_COLOR_3F and VANGUI_PAYLOAD_TYPE_COLOR_4F
            // to allow your own widgets to use colors in their drag and drop interaction.
            // Also see 'Demo->Widgets->Color/Picker Widgets->Palette' demo.
            HelpMarker("You can drag from the color squares.");
            static float col1[3] = { 1.0f, 0.0f, 0.2f };
            static float col2[4] = { 0.4f, 0.7f, 0.0f, 0.5f };
            VanGui::ColorEdit3("color 1", col1);
            VanGui::ColorEdit4("color 2", col2);
            VanGui::TreePop();
        }

        if (VanGui::TreeNode("Drag and drop to copy/swap items"))
        {
            VANGUI_DEMO_MARKER("Widgets/Drag and drop/Copy-swap items");
            enum Mode
            {
                Mode_Copy,
                Mode_Move,
                Mode_Swap
            };
            static int mode = 0;
            if (VanGui::RadioButton("Copy", mode == Mode_Copy)) { mode = Mode_Copy; } VanGui::SameLine();
            if (VanGui::RadioButton("Move", mode == Mode_Move)) { mode = Mode_Move; } VanGui::SameLine();
            if (VanGui::RadioButton("Swap", mode == Mode_Swap)) { mode = Mode_Swap; }
            static const char* names[9] =
            {
                "Bobby", "Beatrice", "Betty",
                "Brianna", "Barry", "Bernard",
                "Bibi", "Blaine", "Bryn"
            };
            for (int n = 0; n < VAN_COUNTOF(names); n++)
            {
                VanGui::PushID(n);
                if ((n % 3) != 0)
                    VanGui::SameLine();
                VanGui::Button(names[n], VanVec2(60, 60));

                // Our buttons are both drag sources and drag targets here!
                if (VanGui::BeginDragDropSource(VanGuiDragDropFlags_None))
                {
                    // Set payload to carry the index of our item (could be anything)
                    VanGui::SetDragDropPayload("DND_DEMO_CELL", &n, sizeof(int));

                    // Display preview (could be anything, e.g. when dragging an image we could decide to display
                    // the filename and a small preview of the image, etc.)
                    if (mode == Mode_Copy) { VanGui::Text("Copy %s", names[n]); }
                    if (mode == Mode_Move) { VanGui::Text("Move %s", names[n]); }
                    if (mode == Mode_Swap) { VanGui::Text("Swap %s", names[n]); }
                    VanGui::EndDragDropSource();
                }
                if (VanGui::BeginDragDropTarget())
                {
                    if (const VanGuiPayload* payload = VanGui::AcceptDragDropPayload("DND_DEMO_CELL"))
                    {
                        VAN_ASSERT(payload->DataSize == sizeof(int));
                        int payload_n = *static_cast<const int*>(payload->Data);
                        if (mode == Mode_Copy)
                        {
                            names[n] = names[payload_n];
                        }
                        if (mode == Mode_Move)
                        {
                            names[n] = names[payload_n];
                            names[payload_n] = "";
                        }
                        if (mode == Mode_Swap)
                        {
                            const char* tmp = names[n];
                            names[n] = names[payload_n];
                            names[payload_n] = tmp;
                        }
                    }
                    VanGui::EndDragDropTarget();
                }
                VanGui::PopID();
            }
            VanGui::TreePop();
        }

        if (VanGui::TreeNode("Drag to reorder items (simple)"))
        {
            VANGUI_DEMO_MARKER("Widgets/Drag and Drop/Drag to reorder items (simple)");
            // FIXME: there is temporary (usually single-frame) ID Conflict during reordering as a same item may be submitting twice.
            // This code was always slightly faulty but in a way which was not easily noticeable.
            // Until we fix this, enable VanGuiItemFlags_AllowDuplicateId to disable detecting the issue.
            VanGui::PushItemFlag(VanGuiItemFlags_AllowDuplicateId, true);

            // Simple reordering
            HelpMarker(
                "We don't use the drag and drop api at all here! "
                "Instead we query when the item is held but not hovered, and order items accordingly.");
            static const char* item_names[] = { "Item One", "Item Two", "Item Three", "Item Four", "Item Five" };
            for (int n = 0; n < VAN_COUNTOF(item_names); n++)
            {
                const char* item = item_names[n];
                VanGui::Selectable(item);

                if (VanGui::IsItemActive() && !VanGui::IsItemHovered())
                {
                    int n_next = n + (VanGui::GetMouseDragDelta(0).y < 0.f ? -1 : 1);
                    if (n_next >= 0 && n_next < VAN_COUNTOF(item_names))
                    {
                        item_names[n] = item_names[n_next];
                        item_names[n_next] = item;
                        VanGui::ResetMouseDragDelta();
                    }
                }
            }

            VanGui::PopItemFlag();
            VanGui::TreePop();
        }

        if (VanGui::TreeNode("Tooltip at target location"))
        {
            VANGUI_DEMO_MARKER("Widgets/Drag and Drop/Tooltip at target location");
            for (int n = 0; n < 2; n++)
            {
                // Drop targets
                VanGui::Button(n ? "drop here##1" : "drop here##0");
                if (VanGui::BeginDragDropTarget())
                {
                    VanGuiDragDropFlags drop_target_flags = VanGuiDragDropFlags_AcceptBeforeDelivery | VanGuiDragDropFlags_AcceptNoPreviewTooltip;
                    if (const VanGuiPayload* payload = VanGui::AcceptDragDropPayload(VANGUI_PAYLOAD_TYPE_COLOR_4F, drop_target_flags))
                    {
                        VAN_UNUSED(payload);
                        VanGui::SetMouseCursor(VanGuiMouseCursor_NotAllowed);
                        VanGui::SetTooltip("Cannot drop here!");
                    }
                    VanGui::EndDragDropTarget();
                }

                // Drop source
                static VanVec4 col4 = { 1.0f, 0.0f, 0.2f, 1.0f };
                if (n == 0)
                    VanGui::ColorButton("drag me", col4);

            }
            VanGui::TreePop();
        }

        VanGui::TreePop();
    }
}

//-----------------------------------------------------------------------------
// [SECTION] DemoWindowWidgetsDragsAndSliders()
//-----------------------------------------------------------------------------

static void DemoWindowWidgetsDragsAndSliders()
{
    if (VanGui::TreeNode("Drag/Slider Flags"))
    {
        VANGUI_DEMO_MARKER("Widgets/Drag and Slider Flags");
        // Demonstrate using advanced flags for DragXXX and SliderXXX functions. Note that the flags are the same!
        static VanGuiSliderFlags flags = VanGuiSliderFlags_None;
        VanGui::CheckboxFlags("VanGuiSliderFlags_AlwaysClamp", &flags, VanGuiSliderFlags_AlwaysClamp);
        VanGui::CheckboxFlags("VanGuiSliderFlags_ClampOnInput", &flags, VanGuiSliderFlags_ClampOnInput);
        VanGui::SameLine(); HelpMarker("Clamp value to min/max bounds when input manually with Ctrl+Click. By default Ctrl+Click allows going out of bounds.");
        VanGui::CheckboxFlags("VanGuiSliderFlags_ClampZeroRange", &flags, VanGuiSliderFlags_ClampZeroRange);
        VanGui::SameLine(); HelpMarker("Clamp even if min==max==0.0f. Otherwise DragXXX functions don't clamp.");
        VanGui::CheckboxFlags("VanGuiSliderFlags_Logarithmic", &flags, VanGuiSliderFlags_Logarithmic);
        VanGui::SameLine(); HelpMarker("Enable logarithmic editing (more precision for small values).");
        VanGui::CheckboxFlags("VanGuiSliderFlags_NoRoundToFormat", &flags, VanGuiSliderFlags_NoRoundToFormat);
        VanGui::SameLine(); HelpMarker("Disable rounding underlying value to match precision of the format string (e.g. %.3f values are rounded to those 3 digits).");
        VanGui::CheckboxFlags("VanGuiSliderFlags_NoInput", &flags, VanGuiSliderFlags_NoInput);
        VanGui::SameLine(); HelpMarker("Disable Ctrl+Click or Enter key allowing to input text directly into the widget.");
        VanGui::CheckboxFlags("VanGuiSliderFlags_NoSpeedTweaks", &flags, VanGuiSliderFlags_NoSpeedTweaks);
        VanGui::SameLine(); HelpMarker("Disable keyboard modifiers altering tweak speed. Useful if you want to alter tweak speed yourself based on your own logic.");
        VanGui::CheckboxFlags("VanGuiSliderFlags_WrapAround", &flags, VanGuiSliderFlags_WrapAround);
        VanGui::SameLine(); HelpMarker("Enable wrapping around from max to min and from min to max (only supported by DragXXX() functions)");
        VanGui::CheckboxFlags("VanGuiSliderFlags_ColorMarkers", &flags, VanGuiSliderFlags_ColorMarkers);

        // Drags
        static float drag_f = 0.5f;
        static float drag_f4[4];
        static int drag_i = 50;
        VanGui::Text("Underlying float value: %f", drag_f);
        VanGui::DragFloat("DragFloat (0 -> 1)", &drag_f, 0.005f, 0.0f, 1.0f, "%.3f", flags);
        VanGui::DragFloat("DragFloat (0 -> +inf)", &drag_f, 0.005f, 0.0f, FLT_MAX, "%.3f", flags);
        VanGui::DragFloat("DragFloat (-inf -> 1)", &drag_f, 0.005f, -FLT_MAX, 1.0f, "%.3f", flags);
        VanGui::DragFloat("DragFloat (-inf -> +inf)", &drag_f, 0.005f, -FLT_MAX, +FLT_MAX, "%.3f", flags);
        //VanGui::DragFloat("DragFloat (0 -> 0)", &drag_f, 0.005f, 0.0f, 0.0f, "%.3f", flags);           // To test ClampZeroRange
        //VanGui::DragFloat("DragFloat (100 -> 100)", &drag_f, 0.005f, 100.0f, 100.0f, "%.3f", flags);
        VanGui::DragInt("DragInt (0 -> 100)", &drag_i, 0.5f, 0, 100, "%d", flags);
        VanGui::DragFloat4("DragFloat4 (0 -> 1)", drag_f4, 0.005f, 0.0f, 1.0f, "%.3f", flags); // Multi-component item, mostly here to document the effect of VanGuiSliderFlags_ColorMarkers.

        // Sliders
        static float slider_f = 0.5f;
        static float slider_f4[4];
        static int slider_i = 50;
        const VanGuiSliderFlags flags_for_sliders = (flags & ~VanGuiSliderFlags_WrapAround);
        VanGui::Text("Underlying float value: %f", slider_f);
        VanGui::SliderFloat("SliderFloat (0 -> 1)", &slider_f, 0.0f, 1.0f, "%.3f", flags_for_sliders);
        VanGui::SliderInt("SliderInt (0 -> 100)", &slider_i, 0, 100, "%d", flags_for_sliders);
        VanGui::SliderFloat4("SliderFloat4 (0 -> 1)", slider_f4, 0.0f, 1.0f, "%.3f", flags); // Multi-component item, mostly here to document the effect of VanGuiSliderFlags_ColorMarkers.

        VanGui::TreePop();
    }
}

//-----------------------------------------------------------------------------
// [SECTION] DemoWindowWidgetsFonts()
//-----------------------------------------------------------------------------

static void DemoWindowWidgetsFonts()
{
    if (VanGui::TreeNode("Fonts"))
    {
        VANGUI_DEMO_MARKER("Widgets/Fonts");
        VanFontAtlas* atlas = VanGui::GetIO().Fonts;
        VanGui::ShowFontAtlas(atlas);
        // FIXME-NEWATLAS: Provide a demo to add/create a procedural font?
        VanGui::TreePop();
    }
}

//-----------------------------------------------------------------------------
// [SECTION] DemoWindowWidgetsImages()
//-----------------------------------------------------------------------------

static void DemoWindowWidgetsImages()
{
    if (VanGui::TreeNode("Images"))
    {
        VANGUI_DEMO_MARKER("Widgets/Images");
        VanGuiIO& io = VanGui::GetIO();
        VanGui::TextWrapped(
            "Below we are displaying the font texture (which is the only texture we have access to in this demo). "
            "Use the 'VanTextureID' type as storage to pass pointers or identifier to your own texture data. "
            "Hover the texture for a zoomed view!");

        // Below we are displaying the font texture because it is the only texture we have access to inside the demo!
        // Read description about VanTextureID/VanTextureRef and FAQ for details about texture identifiers.
        // If you use one of the default vangui_impl_XXXX.cpp rendering backend, they all have comments at the top
        // of their respective source file to specify what they are using as texture identifier, for example:
        // - The vangui_impl_dx11.cpp renderer expect a 'ID3D11ShaderResourceView*' pointer.
        // - The vangui_impl_opengl3.cpp renderer expect a GLuint OpenGL texture identifier, etc.
        // So with the DirectX11 backend, you call VanGui::Image() with a 'ID3D11ShaderResourceView*' cast to VanTextureID.
        // - If you decided that VanTextureID = MyEngineTexture*, then you can pass your MyEngineTexture* pointers
        //   to VanGui::Image(), and gather width/height through your own functions, etc.
        // - You can use ShowMetricsWindow() to inspect the draw data that are being passed to your renderer,
        //   it will help you debug issues if you are confused about it.
        // - Consider using the lower-level VanDrawList::AddImage() API, via VanGui::GetWindowDrawList()->AddImage().
        // - Read https://github.com/ocornut/vangui/blob/master/docs/FAQ.md
        // - Read https://github.com/ocornut/vangui/wiki/Image-Loading-and-Displaying-Examples

        // Grab the current texture identifier used by the font atlas.
        VanFontAtlas* atlas = io.Fonts;
        VanTextureRef my_tex_id = atlas->TexRef;
        float my_tex_w = static_cast<float>(atlas->TexData->Width); // Regular user code should never have to care about TexData-> fields, but since we want to display the entire texture here, we pull Width/Height from it.
        float my_tex_h = static_cast<float>(atlas->TexData->Height);
        VanGui::Text("%.0fx%.0f", my_tex_w, my_tex_h);

        // Basic drawing
        VanGui::SeparatorText("Image()/ImageWithBg() function");
        VanVec2 uv_min = VanVec2(0.0f, 0.0f); // Top-left
        VanVec2 uv_max = VanVec2(1.0f, 1.0f); // Lower-right
        VanGui::PushStyleVar(VanGuiStyleVar_ImageBorderSize, VAN_MAX(1.0f, VanGui::GetStyle().ImageBorderSize));
        VanGui::ImageWithBg(my_tex_id, VanVec2(my_tex_w, my_tex_h), uv_min, uv_max, VanVec4(0.0f, 0.0f, 0.0f, 1.0f));
        VanGui::PopStyleVar();

        // Fancy widget
        VanGui::SeparatorText("Interactive Image Viewer");
        static ExampleImageViewerData image_viewer;
        VanVec2 canvas_size(VanGui::GetContentRegionAvail().x, my_tex_h * 2.0f);
        ExampleImageViewer_DrawOptions(&image_viewer);
        ExampleImageViewer_DrawCanvas(&image_viewer, canvas_size, my_tex_id, static_cast<int>(my_tex_w), static_cast<int>(my_tex_h));

        VANGUI_DEMO_MARKER("Widgets/Images/Textured buttons");
        VanGui::SeparatorText("Textured Buttons");
        VanGui::TextWrapped("And now some textured buttons..");
        static int pressed_count = 0;
        for (int i = 0; i < 8; i++)
        {
            // UV coordinates are often (0.0f, 0.0f) and (1.0f, 1.0f) to display an entire textures.
            // Here are trying to display only a 32x32 pixels area of the texture, hence the UV computation.
            // Read about UV coordinates here: https://github.com/ocornut/vangui/wiki/Image-Loading-and-Displaying-Examples
            VanGui::PushID(i);
            if (i > 0)
                VanGui::PushStyleVar(VanGuiStyleVar_FramePadding, VanVec2(i - 1.0f, i - 1.0f));
            VanVec2 size = VanVec2(32.0f, 32.0f);                         // Size of the image we want to make visible
            VanVec2 uv0 = VanVec2(0.0f, 0.0f);                            // UV coordinates for lower-left
            VanVec2 uv1 = VanVec2(32.0f / my_tex_w, 32.0f / my_tex_h);    // UV coordinates for (32,32) in our texture
            VanVec4 bg_col = VanVec4(0.0f, 0.0f, 0.0f, 1.0f);             // Black background
            VanVec4 tint_col = VanVec4(1.0f, 1.0f, 1.0f, 1.0f);           // No tint
            if (VanGui::ImageButton("", my_tex_id, size, uv0, uv1, bg_col, tint_col))
                pressed_count += 1;
            if (i > 0)
                VanGui::PopStyleVar();
            VanGui::PopID();
            VanGui::SameLine();
        }
        VanGui::NewLine();
        VanGui::Text("Pressed %d times.", pressed_count);
        VanGui::TreePop();
    }
}

//-----------------------------------------------------------------------------
// [SECTION] DemoWindowWidgetsListBoxes()
//-----------------------------------------------------------------------------

static void DemoWindowWidgetsListBoxes()
{
    if (VanGui::TreeNode("List Boxes"))
    {
        VANGUI_DEMO_MARKER("Widgets/List Boxes");
        // BeginListBox() is essentially a thin wrapper to using BeginChild()/EndChild()
        // using the VanGuiChildFlags_FrameStyle flag for stylistic changes + displaying a label.
        // You may be tempted to simply use BeginChild() directly. However note that BeginChild() requires EndChild()
        // to always be called (inconsistent with BeginListBox()/EndListBox()).

        // Using the generic BeginListBox() API, you have full control over how to display the combo contents.
        // (your selection data could be an index, a pointer to the object, an id for the object, a flag intrusively
        // stored in the object itself, etc.)
        const char* items[] = { "AAAA", "BBBB", "CCCC", "DDDD", "EEEE", "FFFF", "GGGG", "HHHH", "IIII", "JJJJ", "KKKK", "LLLLLLL", "MMMM", "OOOOOOO" };
        static int item_selected_idx = 0; // Here we store our selected data as an index.

        static bool item_highlight = false;
        int item_highlighted_idx = -1; // Here we store our highlighted data as an index.
        VanGui::Checkbox("Highlight hovered item in second listbox", &item_highlight);

        if (VanGui::BeginListBox("listbox 1"))
        {
            for (int n = 0; n < VAN_COUNTOF(items); n++)
            {
                const bool is_selected = (item_selected_idx == n);
                if (VanGui::Selectable(items[n], is_selected))
                    item_selected_idx = n;

                if (item_highlight && VanGui::IsItemHovered())
                    item_highlighted_idx = n;

                // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                if (is_selected)
                    VanGui::SetItemDefaultFocus();
            }
            VanGui::EndListBox();
        }
        VanGui::SameLine(); HelpMarker("Here we are sharing selection state between both boxes.");

        // Custom size: use all width, 5 items tall
        VanGui::Text("Full-width:");
        if (VanGui::BeginListBox("##listbox 2", VanVec2(-FLT_MIN, 5 * VanGui::GetTextLineHeightWithSpacing())))
        {
            for (int n = 0; n < VAN_COUNTOF(items); n++)
            {
                bool is_selected = (item_selected_idx == n);
                VanGuiSelectableFlags flags = (item_highlighted_idx == n) ? VanGuiSelectableFlags_Highlight : 0;
                if (VanGui::Selectable(items[n], is_selected, flags))
                    item_selected_idx = n;

                // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                if (is_selected)
                    VanGui::SetItemDefaultFocus();
            }
            VanGui::EndListBox();
        }

        VanGui::TreePop();
    }
}

//-----------------------------------------------------------------------------
// [SECTION] DemoWindowWidgetsMultiComponents()
//-----------------------------------------------------------------------------

static void DemoWindowWidgetsMultiComponents()
{
    if (VanGui::TreeNode("Multi-component Widgets"))
    {
        VANGUI_DEMO_MARKER("Widgets/Multi-component Widgets");
        static float vec4f[4] = { 0.10f, 0.20f, 0.30f, 0.44f };
        static int vec4i[4] = { 1, 5, 100, 255 };

        static VanGuiSliderFlags flags = 0;
        VanGui::CheckboxFlags("VanGuiSliderFlags_ColorMarkers", &flags, VanGuiSliderFlags_ColorMarkers); // Only passing this to Drag/Sliders

        VanGui::SeparatorText("2-wide");
        VanGui::InputFloat2("input float2", vec4f);
        VanGui::InputInt2("input int2", vec4i);
        VanGui::DragFloat2("drag float2", vec4f, 0.01f, 0.0f, 1.0f, nullptr, flags);
        VanGui::DragInt2("drag int2", vec4i, 1, 0, 255, nullptr, flags);
        VanGui::SliderFloat2("slider float2", vec4f, 0.0f, 1.0f, nullptr, flags);
        VanGui::SliderInt2("slider int2", vec4i, 0, 255, nullptr, flags);

        VanGui::SeparatorText("3-wide");
        VanGui::InputFloat3("input float3", vec4f);
        VanGui::InputInt3("input int3", vec4i);
        VanGui::DragFloat3("drag float3", vec4f, 0.01f, 0.0f, 1.0f, nullptr, flags);
        VanGui::DragInt3("drag int3", vec4i, 1, 0, 255, nullptr, flags);
        VanGui::SliderFloat3("slider float3", vec4f, 0.0f, 1.0f, nullptr, flags);
        VanGui::SliderInt3("slider int3", vec4i, 0, 255, nullptr, flags);

        VanGui::SeparatorText("4-wide");
        VanGui::InputFloat4("input float4", vec4f);
        VanGui::InputInt4("input int4", vec4i);
        VanGui::DragFloat4("drag float4", vec4f, 0.01f, 0.0f, 1.0f, nullptr, flags);
        VanGui::DragInt4("drag int4", vec4i, 1, 0, 255, nullptr, flags);
        VanGui::SliderFloat4("slider float4", vec4f, 0.0f, 1.0f, nullptr, flags);
        VanGui::SliderInt4("slider int4", vec4i, 0, 255, nullptr, flags);

        VanGui::SeparatorText("Ranges");
        static float begin = 10, end = 90;
        static int begin_i = 100, end_i = 1000;
        VanGui::DragFloatRange2("range float", &begin, &end, 0.25f, 0.0f, 100.0f, "Min: %.1f %%", "Max: %.1f %%", VanGuiSliderFlags_AlwaysClamp);
        VanGui::DragIntRange2("range int", &begin_i, &end_i, 5, 0, 1000, "Min: %d units", "Max: %d units");
        VanGui::DragIntRange2("range int (no bounds)", &begin_i, &end_i, 5, 0, 0, "Min: %d units", "Max: %d units");

        VanGui::TreePop();
    }
}

//-----------------------------------------------------------------------------
// [SECTION] DemoWindowWidgetsPlotting()
//-----------------------------------------------------------------------------

static void DemoWindowWidgetsPlotting()
{
    // Plot/Graph widgets are not very good.
// Consider using a third-party library such as VanPlot: https://github.com/epezent/implot
// (see others https://github.com/ocornut/vangui/wiki/Useful-Extensions)
    if (VanGui::TreeNode("Plotting"))
    {
        VANGUI_DEMO_MARKER("Widgets/Plotting");
        VanGui::Text("Need better plotting and graphing? Consider using VanPlot:");
        VanGui::TextLinkOpenURL("https://github.com/epezent/implot");
        VanGui::Separator();

        static bool animate = true;
        VanGui::Checkbox("Animate", &animate);

        // Plot as lines and plot as histogram
        static float arr[] = { 0.6f, 0.1f, 1.0f, 0.5f, 0.92f, 0.1f, 0.2f };
        VanGui::PlotLines("Frame Times", arr, VAN_COUNTOF(arr));
        VanGui::PlotHistogram("Histogram", arr, VAN_COUNTOF(arr), 0, nullptr, 0.0f, 1.0f, VanVec2(0, 80.0f));
        //VanGui::SameLine(); HelpMarker("Consider using VanPlot instead!");

        // Fill an array of contiguous float values to plot
        // Tip: If your float aren't contiguous but part of a structure, you can pass a pointer to your first float
        // and the sizeof() of your structure in the "stride" parameter.
        static float values[90] = {};
        static int values_offset = 0;
        static double refresh_time = 0.0;
        if (!animate || refresh_time == 0.0)
            refresh_time = VanGui::GetTime();
        while (refresh_time < VanGui::GetTime()) // Create data at fixed 60 Hz rate for the demo
        {
            static float phase = 0.0f;
            values[values_offset] = cosf(phase);
            values_offset = (values_offset + 1) % VAN_COUNTOF(values);
            phase += 0.10f * values_offset;
            refresh_time += 1.0f / 60.0f;
        }

        // Plots can display overlay texts
        // (in this example, we will display an average value)
        {
            float average = 0.0f;
            for (int n = 0; n < VAN_COUNTOF(values); n++)
                average += values[n];
            average /= static_cast<float>(VAN_COUNTOF(values));
            char overlay[32];
            snprintf(overlay, sizeof(overlay), "avg %f", average);
            VanGui::PlotLines("Lines", values, VAN_COUNTOF(values), values_offset, overlay, -1.0f, 1.0f, VanVec2(0, 80.0f));
        }

        // Use functions to generate output
        // FIXME: This is actually VERY awkward because current plot API only pass in indices.
        // We probably want an API passing floats and user provide sample rate/count.
        struct Funcs
        {
            static float Sin(void*, int i) { return sinf(i * 0.1f); }
            static float Saw(void*, int i) { return (i & 1) ? 1.0f : -1.0f; }
        };
        static int func_type = 0, display_count = 70;
        VanGui::SeparatorText("Functions");
        VanGui::SetNextItemWidth(VanGui::GetFontSize() * 8);
        VanGui::Combo("func", &func_type, "Sin\0Saw\0");
        VanGui::SameLine();
        VanGui::SliderInt("Sample count", &display_count, 1, 400);
        float (*func)(void*, int) = (func_type == 0) ? Funcs::Sin : Funcs::Saw;
        VanGui::PlotLines("Lines##2", func, nullptr, display_count, 0, nullptr, -1.0f, 1.0f, VanVec2(0, 80));
        VanGui::PlotHistogram("Histogram##2", func, nullptr, display_count, 0, nullptr, -1.0f, 1.0f, VanVec2(0, 80));

        VanGui::TreePop();
    }
}

//-----------------------------------------------------------------------------
// [SECTION] DemoWindowWidgetsProgressBars()
//-----------------------------------------------------------------------------

static void DemoWindowWidgetsProgressBars()
{
    if (VanGui::TreeNode("Progress Bars"))
    {
        VANGUI_DEMO_MARKER("Widgets/Progress Bars");
        // Animate a simple progress bar
        static float progress_accum = 0.0f, progress_dir = 1.0f;
        progress_accum += progress_dir * 0.4f * VanGui::GetIO().DeltaTime;
        if (progress_accum >= +1.1f) { progress_accum = +1.1f; progress_dir *= -1.0f; }
        if (progress_accum <= -0.1f) { progress_accum = -0.1f; progress_dir *= -1.0f; }

        const float progress = VAN_CLAMP(progress_accum, 0.0f, 1.0f);

        // Typically we would use VanVec2(-1.0f,0.0f) or VanVec2(-FLT_MIN,0.0f) to use all available width,
        // or VanVec2(width,0.0f) for a specified width. VanVec2(0.0f,0.0f) uses ItemWidth.
        VanGui::ProgressBar(progress, VanVec2(0.0f, 0.0f));
        VanGui::SameLine(0.0f, VanGui::GetStyle().ItemInnerSpacing.x);
        VanGui::Text("Progress Bar");

        char buf[32];
        snprintf(buf, sizeof(buf), "%d/%d", static_cast<int>(progress * 1753), 1753);
        VanGui::ProgressBar(progress, VanVec2(0.f, 0.f), buf);

        // Pass an animated negative value, e.g. -1.0f * (float)VanGui::GetTime() is the recommended value.
        // Adjust the factor if you want to adjust the animation speed.
        VanGui::ProgressBar(-1.0f * static_cast<float>(VanGui::GetTime()), VanVec2(0.0f, 0.0f), "Searching..");
        VanGui::SameLine(0.0f, VanGui::GetStyle().ItemInnerSpacing.x);
        VanGui::Text("Indeterminate");

        VanGui::TreePop();
    }
}

//-----------------------------------------------------------------------------
// [SECTION] DemoWindowWidgetsQueryingStatuses()
//-----------------------------------------------------------------------------

static void DemoWindowWidgetsQueryingStatuses()
{
    if (VanGui::TreeNode("Querying Item Status (Edited/Active/Hovered etc.)"))
    {
        VANGUI_DEMO_MARKER("Widgets/Querying Item Status (Edited,Active,Hovered etc.)");
        // Select an item type
        const char* item_names[] =
        {
            "Text", "Button", "Button (w/ repeat)", "Checkbox", "SliderFloat", "InputText", "InputTextMultiline", "InputFloat",
            "InputFloat3", "ColorEdit4", "Selectable", "MenuItem", "TreeNode", "TreeNode (w/ double-click)", "Combo", "ListBox"
        };
        static int item_type = 4;
        static bool item_disabled = false;
        VanGui::Combo("Item Type", &item_type, item_names, VAN_COUNTOF(item_names), VAN_COUNTOF(item_names));
        VanGui::SameLine();
        HelpMarker("Testing how various types of items are interacting with the IsItemXXX functions. Note that the bool return value of most VanGui function is generally equivalent to calling VanGui::IsItemHovered().");
        VanGui::Checkbox("Item Disabled", &item_disabled);

        // Submit selected items so we can query their status in the code following it.
        bool ret = false;
        static bool b = false;
        static float col4f[4] = { 1.0f, 0.5, 0.0f, 1.0f };
        static char str[16] = {};
        if (item_disabled)
            VanGui::BeginDisabled(true);
        if (item_type == 0) { VanGui::Text("ITEM: Text"); }                                              // Testing text items with no identifier/interaction
        if (item_type == 1) { ret = VanGui::Button("ITEM: Button"); }                                    // Testing button
        if (item_type == 2) { VanGui::PushItemFlag(VanGuiItemFlags_ButtonRepeat, true); ret = VanGui::Button("ITEM: Button"); VanGui::PopItemFlag(); } // Testing button (with repeater)
        if (item_type == 3) { ret = VanGui::Checkbox("ITEM: Checkbox", &b); }                            // Testing checkbox
        if (item_type == 4) { ret = VanGui::SliderFloat("ITEM: SliderFloat", &col4f[0], 0.0f, 1.0f); }   // Testing basic item
        if (item_type == 5) { ret = VanGui::InputText("ITEM: InputText", &str[0], VAN_COUNTOF(str)); }  // Testing input text (which handles tabbing)
        if (item_type == 6) { ret = VanGui::InputTextMultiline("ITEM: InputTextMultiline", &str[0], VAN_COUNTOF(str)); } // Testing input text (which uses a child window)
        if (item_type == 7) { ret = VanGui::InputFloat("ITEM: InputFloat", col4f, 1.0f); }               // Testing +/- buttons on scalar input
        if (item_type == 8) { ret = VanGui::InputFloat3("ITEM: InputFloat3", col4f); }                   // Testing multi-component items (IsItemXXX flags are reported merged)
        if (item_type == 9) { ret = VanGui::ColorEdit4("ITEM: ColorEdit4", col4f); }                     // Testing multi-component items (IsItemXXX flags are reported merged)
        if (item_type == 10) { ret = VanGui::Selectable("ITEM: Selectable"); }                            // Testing selectable item
        if (item_type == 11) { ret = VanGui::MenuItem("ITEM: MenuItem"); }                                // Testing menu item (they use VanGuiButtonFlags_PressedOnRelease button policy)
        if (item_type == 12) { ret = VanGui::TreeNode("ITEM: TreeNode"); if (ret) VanGui::TreePop(); }     // Testing tree node
        if (item_type == 13) { ret = VanGui::TreeNodeEx("ITEM: TreeNode w/ VanGuiTreeNodeFlags_OpenOnDoubleClick", VanGuiTreeNodeFlags_OpenOnDoubleClick | VanGuiTreeNodeFlags_NoTreePushOnOpen); } // Testing tree node with VanGuiButtonFlags_PressedOnDoubleClick button policy.
        if (item_type == 14) { const char* items[] = { "Apple", "Banana", "Cherry", "Kiwi" }; static int current = 1; ret = VanGui::Combo("ITEM: Combo", &current, items, VAN_COUNTOF(items)); }
        if (item_type == 15) { const char* items[] = { "Apple", "Banana", "Cherry", "Kiwi" }; static int current = 1; ret = VanGui::ListBox("ITEM: ListBox", &current, items, VAN_COUNTOF(items), VAN_COUNTOF(items)); }

        bool hovered_delay_none = VanGui::IsItemHovered();
        bool hovered_delay_stationary = VanGui::IsItemHovered(VanGuiHoveredFlags_Stationary);
        bool hovered_delay_short = VanGui::IsItemHovered(VanGuiHoveredFlags_DelayShort);
        bool hovered_delay_normal = VanGui::IsItemHovered(VanGuiHoveredFlags_DelayNormal);
        bool hovered_delay_tooltip = VanGui::IsItemHovered(VanGuiHoveredFlags_ForTooltip); // = Normal + Stationary

        // Display the values of IsItemHovered() and other common item state functions.
        // Note that the VanGuiHoveredFlags_XXX flags can be combined.
        // Because BulletText is an item itself and that would affect the output of IsItemXXX functions,
        // we query every state in a single call to avoid storing them and to simplify the code.
        VanGui::BulletText(
            "Return value = %d\n"
            "IsItemFocused() = %d\n"
            "IsItemHovered() = %d\n"
            "IsItemHovered(_AllowWhenBlockedByPopup) = %d\n"
            "IsItemHovered(_AllowWhenBlockedByActiveItem) = %d\n"
            "IsItemHovered(_AllowWhenOverlappedByItem) = %d\n"
            "IsItemHovered(_AllowWhenOverlappedByWindow) = %d\n"
            "IsItemHovered(_AllowWhenDisabled) = %d\n"
            "IsItemHovered(_RectOnly) = %d\n"
            "IsItemActive() = %d\n"
            "IsItemEdited() = %d\n"
            "IsItemActivated() = %d\n"
            "IsItemDeactivated() = %d\n"
            "IsItemDeactivatedAfterEdit() = %d\n"
            "IsItemVisible() = %d\n"
            "IsItemClicked() = %d\n"
            "IsItemToggledOpen() = %d\n"
            "GetItemRectMin() = (%.1f, %.1f)\n"
            "GetItemRectMax() = (%.1f, %.1f)\n"
            "GetItemRectSize() = (%.1f, %.1f)",
            ret,
            VanGui::IsItemFocused(),
            VanGui::IsItemHovered(),
            VanGui::IsItemHovered(VanGuiHoveredFlags_AllowWhenBlockedByPopup),
            VanGui::IsItemHovered(VanGuiHoveredFlags_AllowWhenBlockedByActiveItem),
            VanGui::IsItemHovered(VanGuiHoveredFlags_AllowWhenOverlappedByItem),
            VanGui::IsItemHovered(VanGuiHoveredFlags_AllowWhenOverlappedByWindow),
            VanGui::IsItemHovered(VanGuiHoveredFlags_AllowWhenDisabled),
            VanGui::IsItemHovered(VanGuiHoveredFlags_RectOnly),
            VanGui::IsItemActive(),
            VanGui::IsItemEdited(),
            VanGui::IsItemActivated(),
            VanGui::IsItemDeactivated(),
            VanGui::IsItemDeactivatedAfterEdit(),
            VanGui::IsItemVisible(),
            VanGui::IsItemClicked(),
            VanGui::IsItemToggledOpen(),
            VanGui::GetItemRectMin().x, VanGui::GetItemRectMin().y,
            VanGui::GetItemRectMax().x, VanGui::GetItemRectMax().y,
            VanGui::GetItemRectSize().x, VanGui::GetItemRectSize().y
        );
        VanGui::BulletText(
            "with Hovering Delay or Stationary test:\n"
            "IsItemHovered() = %d\n"
            "IsItemHovered(_Stationary) = %d\n"
            "IsItemHovered(_DelayShort) = %d\n"
            "IsItemHovered(_DelayNormal) = %d\n"
            "IsItemHovered(_Tooltip) = %d",
            hovered_delay_none, hovered_delay_stationary, hovered_delay_short, hovered_delay_normal, hovered_delay_tooltip);

        if (item_disabled)
            VanGui::EndDisabled();

        char buf[1] = "";
        VanGui::InputText("unused", buf, VAN_COUNTOF(buf), VanGuiInputTextFlags_ReadOnly);
        VanGui::SameLine();
        HelpMarker("This widget is only here to be able to tab-out of the widgets above and see e.g. Deactivated() status.");

        VanGui::TreePop();
    }

    if (VanGui::TreeNode("Querying Window Status (Focused/Hovered etc.)"))
    {
        VANGUI_DEMO_MARKER("Widgets/Querying Window Status (Focused,Hovered etc.)");
        static bool embed_all_inside_a_child_window = false;
        VanGui::Checkbox("Embed everything inside a child window for testing _RootWindow flag.", &embed_all_inside_a_child_window);
        if (embed_all_inside_a_child_window)
            VanGui::BeginChild("outer_child", VanVec2(0, VanGui::GetFontSize() * 20.0f), VanGuiChildFlags_Borders);

        // Testing IsWindowFocused() function with its various flags.
        VanGui::BulletText(
            "IsWindowFocused() = %d\n"
            "IsWindowFocused(_ChildWindows) = %d\n"
            "IsWindowFocused(_ChildWindows|_NoPopupHierarchy) = %d\n"
            "IsWindowFocused(_ChildWindows|_RootWindow) = %d\n"
            "IsWindowFocused(_ChildWindows|_RootWindow|_NoPopupHierarchy) = %d\n"
            "IsWindowFocused(_RootWindow) = %d\n"
            "IsWindowFocused(_RootWindow|_NoPopupHierarchy) = %d\n"
            "IsWindowFocused(_AnyWindow) = %d\n",
            VanGui::IsWindowFocused(),
            VanGui::IsWindowFocused(VanGuiFocusedFlags_ChildWindows),
            VanGui::IsWindowFocused(VanGuiFocusedFlags_ChildWindows | VanGuiFocusedFlags_NoPopupHierarchy),
            VanGui::IsWindowFocused(VanGuiFocusedFlags_ChildWindows | VanGuiFocusedFlags_RootWindow),
            VanGui::IsWindowFocused(VanGuiFocusedFlags_ChildWindows | VanGuiFocusedFlags_RootWindow | VanGuiFocusedFlags_NoPopupHierarchy),
            VanGui::IsWindowFocused(VanGuiFocusedFlags_RootWindow),
            VanGui::IsWindowFocused(VanGuiFocusedFlags_RootWindow | VanGuiFocusedFlags_NoPopupHierarchy),
            VanGui::IsWindowFocused(VanGuiFocusedFlags_AnyWindow));

        // Testing IsWindowHovered() function with its various flags.
        VanGui::BulletText(
            "IsWindowHovered() = %d\n"
            "IsWindowHovered(_AllowWhenBlockedByPopup) = %d\n"
            "IsWindowHovered(_AllowWhenBlockedByActiveItem) = %d\n"
            "IsWindowHovered(_ChildWindows) = %d\n"
            "IsWindowHovered(_ChildWindows|_NoPopupHierarchy) = %d\n"
            "IsWindowHovered(_ChildWindows|_RootWindow) = %d\n"
            "IsWindowHovered(_ChildWindows|_RootWindow|_NoPopupHierarchy) = %d\n"
            "IsWindowHovered(_RootWindow) = %d\n"
            "IsWindowHovered(_RootWindow|_NoPopupHierarchy) = %d\n"
            "IsWindowHovered(_ChildWindows|_AllowWhenBlockedByPopup) = %d\n"
            "IsWindowHovered(_AnyWindow) = %d\n"
            "IsWindowHovered(_Stationary) = %d\n",
            VanGui::IsWindowHovered(),
            VanGui::IsWindowHovered(VanGuiHoveredFlags_AllowWhenBlockedByPopup),
            VanGui::IsWindowHovered(VanGuiHoveredFlags_AllowWhenBlockedByActiveItem),
            VanGui::IsWindowHovered(VanGuiHoveredFlags_ChildWindows),
            VanGui::IsWindowHovered(VanGuiHoveredFlags_ChildWindows | VanGuiHoveredFlags_NoPopupHierarchy),
            VanGui::IsWindowHovered(VanGuiHoveredFlags_ChildWindows | VanGuiHoveredFlags_RootWindow),
            VanGui::IsWindowHovered(VanGuiHoveredFlags_ChildWindows | VanGuiHoveredFlags_RootWindow | VanGuiHoveredFlags_NoPopupHierarchy),
            VanGui::IsWindowHovered(VanGuiHoveredFlags_RootWindow),
            VanGui::IsWindowHovered(VanGuiHoveredFlags_RootWindow | VanGuiHoveredFlags_NoPopupHierarchy),
            VanGui::IsWindowHovered(VanGuiHoveredFlags_ChildWindows | VanGuiHoveredFlags_AllowWhenBlockedByPopup),
            VanGui::IsWindowHovered(VanGuiHoveredFlags_AnyWindow),
            VanGui::IsWindowHovered(VanGuiHoveredFlags_Stationary));

        VanGui::BeginChild("child", VanVec2(0, 50), VanGuiChildFlags_Borders);
        VanGui::Text("This is another child window for testing the _ChildWindows flag.");
        VanGui::EndChild();
        if (embed_all_inside_a_child_window)
            VanGui::EndChild();

        // Calling IsItemHovered() after begin returns the hovered status of the title bar.
        // This is useful in particular if you want to create a context menu associated to the title bar of a window.
        static bool test_window = false;
        VanGui::Checkbox("Hovered/Active tests after Begin() for title bar testing", &test_window);
        if (test_window)
        {
            VanGui::Begin("Title bar Hovered/Active tests", &test_window);
            if (VanGui::BeginPopupContextItem()) // <-- This is using IsItemHovered()
            {
                if (VanGui::MenuItem("Close")) { test_window = false; }
                VanGui::EndPopup();
            }
            VanGui::Text(
                "IsItemHovered() after begin = %d (== is title bar hovered)\n"
                "IsItemActive() after begin = %d (== is window being clicked/moved)\n",
                VanGui::IsItemHovered(), VanGui::IsItemActive());
            VanGui::End();
        }

        VanGui::TreePop();
    }
}

//-----------------------------------------------------------------------------
// [SECTION] DemoWindowWidgetsSelectables()
//-----------------------------------------------------------------------------

static void DemoWindowWidgetsSelectables()
{
    //VanGui::SetNextItemOpen(true, VanGuiCond_Once);
    if (VanGui::TreeNode("Selectables"))
    {
        VANGUI_DEMO_MARKER("Widgets/Selectables");
        // Selectable() has 2 overloads:
        // - The one taking "bool selected" as a read-only selection information.
        //   When Selectable() has been clicked it returns true and you can alter selection state accordingly.
        // - The one taking "bool* p_selected" as a read-write selection information (convenient in some cases)
        // The earlier is more flexible, as in real application your selection may be stored in many different ways
        // and not necessarily inside a bool value (e.g. in flags within objects, as an external list, etc).
        VANGUI_DEMO_MARKER("Widgets/Selectables/Basic");
        if (VanGui::TreeNode("Basic"))
        {
            static bool selection[5] = { false, true, false, false };
            VanGui::Selectable("1. I am selectable", &selection[0]);
            VanGui::Selectable("2. I am selectable", &selection[1]);
            VanGui::Selectable("3. I am selectable", &selection[2]);
            if (VanGui::Selectable("4. I am double clickable", selection[3], VanGuiSelectableFlags_AllowDoubleClick))
                if (VanGui::IsMouseDoubleClicked(0))
                    selection[3] = !selection[3];
            VanGui::TreePop();
        }

        VANGUI_DEMO_MARKER("Widgets/Selectables/Rendering more items on the same line");
        if (VanGui::TreeNode("Multiple items on the same line"))
        {
            VANGUI_DEMO_MARKER("Widgets/Selectables/Multiple items on the same line");
            // - Using SetNextItemAllowOverlap()
            // - Using the Selectable() override that takes "bool* p_selected" parameter, the bool value is toggled automatically.
            {
                static bool selected[3] = {};
                VanGui::SetNextItemAllowOverlap(); VanGui::Selectable("main.c", &selected[0]); VanGui::SameLine(); VanGui::SmallButton("Link 1");
                VanGui::SetNextItemAllowOverlap(); VanGui::Selectable("hello.cpp", &selected[1]); VanGui::SameLine(); VanGui::SmallButton("Link 2");
                VanGui::SetNextItemAllowOverlap(); VanGui::Selectable("hello.h", &selected[2]); VanGui::SameLine(); VanGui::SmallButton("Link 3");
            }

            // (2)
            // - Using VanGuiSelectableFlags_AllowOverlap is a shortcut for calling SetNextItemAllowOverlap()
            // - No visible label, display contents inside the selectable bounds.
            // - We don't maintain actual selection in this example to keep things simple.
            VanGui::Spacing();
            {
                static bool checked[5] = {};
                static int selected_n = 0;
                const float color_marker_w = VanGui::CalcTextSize("x").x;
                for (int n = 0; n < 5; n++)
                {
                    VanGui::PushID(n);
                    VanGui::AlignTextToFramePadding();
                    if (VanGui::Selectable("##selectable", selected_n == n, VanGuiSelectableFlags_AllowOverlap))
                        selected_n = n;
                    VanGui::SameLine(0, 0);
                    VanGui::Checkbox("##check", &checked[n]);
                    VanGui::SameLine();
                    VanVec4 color((n & 1) ? 1.0f : 0.2f, (n & 2) ? 1.0f : 0.2f, 0.2f, 1.0f);
                    VanGui::ColorButton("##color", color, VanGuiColorEditFlags_NoTooltip, VanVec2(color_marker_w, 0));
                    VanGui::SameLine();
                    VanGui::Text("Some label");
                    VanGui::PopID();
                }
            }

            VanGui::TreePop();
        }

        if (VanGui::TreeNode("In Tables"))
        {
            VANGUI_DEMO_MARKER("Widgets/Selectables/In Tables");
            static bool selected[10] = {};

            if (VanGui::BeginTable("split1", 3, VanGuiTableFlags_Resizable | VanGuiTableFlags_NoSavedSettings | VanGuiTableFlags_Borders))
            {
                for (int i = 0; i < 10; i++)
                {
                    char label[32];
                    snprintf(label, sizeof(label), "Item %d", i);
                    VanGui::TableNextColumn();
                    VanGui::Selectable(label, &selected[i]); // FIXME-TABLE: Selection overlap
                }
                VanGui::EndTable();
            }
            VanGui::Spacing();
            if (VanGui::BeginTable("split2", 3, VanGuiTableFlags_Resizable | VanGuiTableFlags_NoSavedSettings | VanGuiTableFlags_Borders))
            {
                for (int i = 0; i < 10; i++)
                {
                    char label[32];
                    snprintf(label, sizeof(label), "Item %d", i);
                    VanGui::TableNextRow();
                    VanGui::TableNextColumn();
                    VanGui::Selectable(label, &selected[i], VanGuiSelectableFlags_SpanAllColumns);
                    VanGui::TableNextColumn();
                    VanGui::Text("Some other contents");
                    VanGui::TableNextColumn();
                    VanGui::Text("123456");
                }
                VanGui::EndTable();
            }
            VanGui::TreePop();
        }

        if (VanGui::TreeNode("Grid"))
        {
            VANGUI_DEMO_MARKER("Widgets/Selectables/Grid");
            static char selected[4][4] = { { 1, 0, 0, 0 }, { 0, 1, 0, 0 }, { 0, 0, 1, 0 }, { 0, 0, 0, 1 } };

            // Add in a bit of silly fun...
            const float time = static_cast<float>(VanGui::GetTime());
            const bool winning_state = memchr(selected, 0, sizeof(selected)) == nullptr; // If all cells are selected...
            if (winning_state)
                VanGui::PushStyleVar(VanGuiStyleVar_SelectableTextAlign, VanVec2(0.5f + 0.5f * cosf(time * 2.0f), 0.5f + 0.5f * sinf(time * 3.0f)));

            const float size = VanGui::CalcTextSize("Sailor").x;
            for (int y = 0; y < 4; y++)
                for (int x = 0; x < 4; x++)
                {
                    if (x > 0)
                        VanGui::SameLine();
                    VanGui::PushID(y * 4 + x);
                    if (VanGui::Selectable("Sailor", selected[y][x] != 0, 0, VanVec2(size, size)))
                    {
                        // Toggle clicked cell + toggle neighbors
                        selected[y][x] ^= 1;
                        if (x > 0) { selected[y][x - 1] ^= 1; }
                        if (x < 3) { selected[y][x + 1] ^= 1; }
                        if (y > 0) { selected[y - 1][x] ^= 1; }
                        if (y < 3) { selected[y + 1][x] ^= 1; }
                    }
                    VanGui::PopID();
                }

            if (winning_state)
                VanGui::PopStyleVar();
            VanGui::TreePop();
        }
        if (VanGui::TreeNode("Alignment"))
        {
            VANGUI_DEMO_MARKER("Widgets/Selectables/Alignment");
            HelpMarker(
                "By default, Selectables uses style.SelectableTextAlign but it can be overridden on a per-item "
                "basis using PushStyleVar(). You'll probably want to always keep your default situation to "
                "left-align otherwise it becomes difficult to layout multiple items on a same line");

            static bool selected[3 * 3] = { true, false, true, false, true, false, true, false, true };
            const float size = VanGui::CalcTextSize("(1.0,1.0)").x;
            for (int y = 0; y < 3; y++)
            {
                for (int x = 0; x < 3; x++)
                {
                    VanVec2 alignment = VanVec2(static_cast<float>(x) / 2.0f, static_cast<float>(y) / 2.0f);
                    char name[32];
                    snprintf(name, sizeof(name), "(%.1f,%.1f)", alignment.x, alignment.y);
                    if (x > 0) VanGui::SameLine();
                    VanGui::PushStyleVar(VanGuiStyleVar_SelectableTextAlign, alignment);
                    VanGui::Selectable(name, &selected[3 * y + x], VanGuiSelectableFlags_None, VanVec2(size, size));
                    VanGui::PopStyleVar();
                }
            }
            VanGui::TreePop();
        }
        VanGui::TreePop();
    }
}

//-----------------------------------------------------------------------------
// [SECTION] DemoWindowWidgetsSelectionAndMultiSelect()
//-----------------------------------------------------------------------------
// Multi-selection demos
// Also read: https://github.com/ocornut/vangui/wiki/Multi-Select
//-----------------------------------------------------------------------------

static const char* ExampleNames[] =
{
    "Artichoke", "Arugula", "Asparagus", "Avocado", "Bamboo Shoots", "Bean Sprouts", "Beans", "Beet", "Belgian Endive", "Bell Pepper",
    "Bitter Gourd", "Bok Choy", "Broccoli", "Brussels Sprouts", "Burdock Root", "Cabbage", "Calabash", "Capers", "Carrot", "Cassava",
    "Cauliflower", "Celery", "Celery Root", "Celcuce", "Chayote", "Chinese Broccoli", "Corn", "Cucumber"
};

// Extra functions to add deletion support to VanGuiSelectionBasicStorage
struct ExampleSelectionWithDeletion : VanGuiSelectionBasicStorage
{
    // Find which item should be Focused after deletion.
    // Call _before_ item submission. Return an index in the before-deletion item list, your item loop should call SetKeyboardFocusHere() on it.
    // The subsequent ApplyDeletionPostLoop() code will use it to apply Selection.
    // - We cannot provide this logic in core VanGUI because we don't have access to selection data.
    // - We don't actually manipulate the VanVector<> here, only in ApplyDeletionPostLoop(), but using similar API for consistency and flexibility.
    // - Important: Deletion only works if the underlying VanGuiID for your items are stable: aka not depend on their index, but on e.g. item id/ptr.
    // FIXME-MULTISELECT: Doesn't take account of the possibility focus target will be moved during deletion. Need refocus or scroll offset.
    int ApplyDeletionPreLoop(VanGuiMultiSelectIO* ms_io, int items_count)
    {
        if (Size == 0)
            return -1;

        // If focused item is not selected...
        const int focused_idx = static_cast<int>(ms_io->NavIdItem);  // Index of currently focused item
        if (ms_io->NavIdSelected == false)  // This is merely a shortcut, == Contains(adapter->IndexToStorage(items, focused_idx))
        {
            ms_io->RangeSrcReset = true;    // Request to recover RangeSrc from NavId next frame. Would be ok to reset even when NavIdSelected==true, but it would take an extra frame to recover RangeSrc when deleting a selected item.
            return focused_idx;             // Request to focus same item after deletion.
        }

        // If focused item is selected: land on first unselected item after focused item.
        for (int idx = focused_idx + 1; idx < items_count; idx++)
            if (!Contains(GetStorageIdFromIndex(idx)))
                return idx;

        // If focused item is selected: otherwise return last unselected item before focused item.
        for (int idx = VAN_MIN(focused_idx, items_count) - 1; idx >= 0; idx--)
            if (!Contains(GetStorageIdFromIndex(idx)))
                return idx;

        return -1;
    }

    // Rewrite item list (delete items) + update selection.
    // - Call after EndMultiSelect()
    // - We cannot provide this logic in core VanGUI because we don't have access to your items, nor to selection data.
    template<typename ITEM_TYPE>
    void ApplyDeletionPostLoop(VanGuiMultiSelectIO* ms_io, VanVector<ITEM_TYPE>& items, int item_curr_idx_to_select)
    {
        // Rewrite item list (delete items) + convert old selection index (before deletion) to new selection index (after selection).
        // If NavId was not part of selection, we will stay on same item.
        VanVector<ITEM_TYPE> new_items;
        new_items.reserve(items.Size - Size);
        int item_next_idx_to_select = -1;
        for (int idx = 0; idx < items.Size; idx++)
        {
            if (!Contains(GetStorageIdFromIndex(idx)))
                new_items.push_back(items[idx]);
            if (item_curr_idx_to_select == idx)
                item_next_idx_to_select = new_items.Size - 1;
        }
        items.swap(new_items);

        // Update selection
        Clear();
        if (item_next_idx_to_select != -1 && ms_io->NavIdSelected)
            SetItemSelected(GetStorageIdFromIndex(item_next_idx_to_select), true);
    }
};

// Example: Implement dual list box storage and interface
struct ExampleDualListBox
{
    VanVector<VanGuiID>           Items[2];               // ID is index into ExampleName[]
    VanGuiSelectionBasicStorage  Selections[2];          // Store ExampleItemId into selection
    bool                        OptKeepSorted = true;

    void MoveAll(int src, int dst)
    {
        VAN_ASSERT((src == 0 && dst == 1) || (src == 1 && dst == 0));
        for (VanGuiID item_id : Items[src])
            Items[dst].push_back(item_id);
        Items[src].clear();
        SortItems(dst);
        Selections[src].Swap(Selections[dst]);
        Selections[src].Clear();
    }
    void MoveSelected(int src, int dst)
    {
        for (int src_n = 0; src_n < Items[src].Size; src_n++)
        {
            VanGuiID item_id = Items[src][src_n];
            if (!Selections[src].Contains(item_id))
                continue;
            Items[src].erase(&Items[src][src_n]); // FIXME-OPT: Could be implemented more optimally (rebuild src items and swap)
            Items[dst].push_back(item_id);
            src_n--;
        }
        if (OptKeepSorted)
            SortItems(dst);
        Selections[src].Swap(Selections[dst]);
        Selections[src].Clear();
    }
    void ApplySelectionRequests(VanGuiMultiSelectIO* ms_io, int side)
    {
        // In this example we store item id in selection (instead of item index)
        Selections[side].UserData = Items[side].Data;
        Selections[side].AdapterIndexToStorageId = [](VanGuiSelectionBasicStorage* self, int idx) { VanGuiID* items = (VanGuiID*)self->UserData; return items[idx]; };
        Selections[side].ApplyRequests(ms_io);
    }
    static int VANGUI_CDECL CompareItemsByValue(const void* lhs, const void* rhs)
    {
        const int* a = static_cast<const int*>(lhs);
        const int* b = static_cast<const int*>(rhs);
        return *a - *b;
    }
    void SortItems(int n)
    {
        qsort(Items[n].Data, static_cast<size_t>(Items[n].Size), sizeof(Items[n][0]), CompareItemsByValue);
    }
    void Show()
    {
        //if (VanGui::Checkbox("Sorted", &OptKeepSorted) && OptKeepSorted) { SortItems(0); SortItems(1); }
        if (VanGui::BeginTable("split", 3, VanGuiTableFlags_None))
        {
            VanGui::TableSetupColumn("", VanGuiTableColumnFlags_WidthStretch);    // Left side
            VanGui::TableSetupColumn("", VanGuiTableColumnFlags_WidthFixed);      // Buttons
            VanGui::TableSetupColumn("", VanGuiTableColumnFlags_WidthStretch);    // Right side
            VanGui::TableNextRow();

            int request_move_selected = -1;
            int request_move_all = -1;
            float child_height_0 = 0.0f;
            for (int side = 0; side < 2; side++)
            {
                // FIXME-MULTISELECT: Dual List Box: Add context menus
                // FIXME-NAV: Using VanGuiWindowFlags_NavFlattened exhibit many issues.
                VanVector<VanGuiID>& items = Items[side];
                VanGuiSelectionBasicStorage& selection = Selections[side];

                VanGui::TableSetColumnIndex((side == 0) ? 0 : 2);
                VanGui::Text("%s (%d)", (side == 0) ? "Available" : "Basket", items.Size);

                // Submit scrolling range to avoid glitches on moving/deletion
                const float items_height = VanGui::GetTextLineHeightWithSpacing();
                VanGui::SetNextWindowContentSize(VanVec2(0.0f, items.Size * items_height));

                bool child_visible;
                if (side == 0)
                {
                    // Left child is resizable
                    VanGui::SetNextWindowSizeConstraints(VanVec2(0.0f, VanGui::GetFrameHeightWithSpacing() * 4), VanVec2(FLT_MAX, FLT_MAX));
                    child_visible = VanGui::BeginChild("0", VanVec2(-FLT_MIN, VanGui::GetFontSize() * 20), VanGuiChildFlags_FrameStyle | VanGuiChildFlags_ResizeY);
                    child_height_0 = VanGui::GetWindowSize().y;
                }
                else
                {
                    // Right child use same height as left one
                    child_visible = VanGui::BeginChild("1", VanVec2(-FLT_MIN, child_height_0), VanGuiChildFlags_FrameStyle);
                }
                if (child_visible)
                {
                    VanGuiMultiSelectFlags flags = VanGuiMultiSelectFlags_BoxSelect1d;
                    VanGuiMultiSelectIO* ms_io = VanGui::BeginMultiSelect(flags, selection.Size, items.Size);
                    ApplySelectionRequests(ms_io, side);

                    for (int item_n = 0; item_n < items.Size; item_n++)
                    {
                        VanGuiID item_id = items[item_n];
                        bool item_is_selected = selection.Contains(item_id);
                        VanGui::SetNextItemSelectionUserData(item_n);
                        VanGui::Selectable(ExampleNames[item_id], item_is_selected, VanGuiSelectableFlags_AllowDoubleClick);
                        if (VanGui::IsItemFocused())
                        {
                            // FIXME-MULTISELECT: Dual List Box: Transfer focus
                            if (VanGui::IsKeyPressed(VanGuiKey_Enter) || VanGui::IsKeyPressed(VanGuiKey_KeypadEnter))
                                request_move_selected = side;
                            if (VanGui::IsMouseDoubleClicked(0)) // FIXME-MULTISELECT: Double-click on multi-selection?
                                request_move_selected = side;
                        }
                    }

                    ms_io = VanGui::EndMultiSelect();
                    ApplySelectionRequests(ms_io, side);
                }
                VanGui::EndChild();
            }

            // Buttons columns
            VanGui::TableSetColumnIndex(1);
            VanGui::NewLine();
            //VanVec2 button_sz = { VanGui::CalcTextSize(">>").x + VanGui::GetStyle().FramePadding.x * 2.0f, VanGui::GetFrameHeight() + padding.y * 2.0f };
            VanVec2 button_sz = { VanGui::GetFrameHeight(), VanGui::GetFrameHeight() };

            // (Using BeginDisabled()/EndDisabled() works but feels distracting given how it is currently visualized)
            if (VanGui::Button(">>", button_sz))
                request_move_all = 0;
            if (VanGui::Button(">", button_sz))
                request_move_selected = 0;
            if (VanGui::Button("<", button_sz))
                request_move_selected = 1;
            if (VanGui::Button("<<", button_sz))
                request_move_all = 1;

            // Process requests
            if (request_move_all != -1)
                MoveAll(request_move_all, request_move_all ^ 1);
            if (request_move_selected != -1)
                MoveSelected(request_move_selected, request_move_selected ^ 1);

            // FIXME-MULTISELECT: Support action from outside
            /*
            if (OptKeepSorted == false)
            {
                VanGui::NewLine();
                if (VanGui::ArrowButton("MoveUp", VanGuiDir_Up)) {}
                if (VanGui::ArrowButton("MoveDown", VanGuiDir_Down)) {}
            }
            */

            VanGui::EndTable();
        }
    }
};

static void DemoWindowWidgetsSelectionAndMultiSelect(VanGuiDemoWindowData* demo_data)
{
    if (VanGui::TreeNode("Selection State & Multi-Select"))
    {
        VANGUI_DEMO_MARKER("Widgets/Selection State & Multi-Select");
        HelpMarker("Selections can be built using Selectable(), TreeNode() or other widgets. Selection state is owned by application code/data.");

        VanGui::BulletText("Wiki page:");
        VanGui::SameLine();
        VanGui::TextLinkOpenURL("vangui/wiki/Multi-Select", "https://github.com/ocornut/vangui/wiki/Multi-Select");

        // Without any fancy API: manage single-selection yourself.
        if (VanGui::TreeNode("Single-Select"))
        {
            VANGUI_DEMO_MARKER("Widgets/Selection State/Single-Select");
            static int selected = -1;
            for (int n = 0; n < 5; n++)
            {
                char buf[32];
                snprintf(buf, sizeof(buf), "Object %d", n);
                if (VanGui::Selectable(buf, selected == n))
                    selected = n;
            }
            VanGui::TreePop();
        }

        // Demonstrate implementation a most-basic form of multi-selection manually
        // This doesn't support the Shift modifier which requires BeginMultiSelect()!
        if (VanGui::TreeNode("Multi-Select (manual/simplified, without BeginMultiSelect)"))
        {
            VANGUI_DEMO_MARKER("Widgets/Selection State/Multi-Select (manual/simplified, without BeginMultiSelect)");
            HelpMarker("Hold Ctrl and Click to select multiple items.");
            static bool selection[5] = { false, false, false, false, false };
            for (int n = 0; n < 5; n++)
            {
                char buf[32];
                snprintf(buf, sizeof(buf), "Object %d", n);
                if (VanGui::Selectable(buf, selection[n]))
                {
                    if (!VanGui::GetIO().KeyCtrl) // Clear selection when Ctrl is not held
                        memset(selection, 0, sizeof(selection));
                    selection[n] ^= 1; // Toggle current item
                }
            }
            VanGui::TreePop();
        }

        // Demonstrate handling proper multi-selection using the BeginMultiSelect/EndMultiSelect API.
        // Shift+Click w/ Ctrl and other standard features are supported.
        // We use the VanGuiSelectionBasicStorage helper which you may freely reimplement.
        if (VanGui::TreeNode("Multi-Select"))
        {
            VANGUI_DEMO_MARKER("Widgets/Selection State/Multi-Select");
            VanGui::Text("Supported features:");
            VanGui::BulletText("Keyboard navigation (arrows, page up/down, home/end, space).");
            VanGui::BulletText("Ctrl modifier to preserve and toggle selection.");
            VanGui::BulletText("Shift modifier for range selection.");
            VanGui::BulletText("Ctrl+A to select all.");
            VanGui::BulletText("Escape to clear selection.");
            VanGui::BulletText("Click and drag to box-select.");
            VanGui::Text("Tip: Use 'Demo->Tools->Debug Log->Selection' to see selection requests as they happen.");

            // Use default selection.Adapter: Pass index to SetNextItemSelectionUserData(), store index in Selection
            const int ITEMS_COUNT = 50;
            static VanGuiSelectionBasicStorage selection;
            VanGui::Text("Selection: %d/%d", selection.Size, ITEMS_COUNT);

            // The BeginChild() has no purpose for selection logic, other that offering a scrolling region.
            if (VanGui::BeginChild("##Basket", VanVec2(-FLT_MIN, VanGui::GetFontSize() * 20), VanGuiChildFlags_FrameStyle | VanGuiChildFlags_ResizeY))
            {
                VanGuiMultiSelectFlags flags = VanGuiMultiSelectFlags_ClearOnEscape | VanGuiMultiSelectFlags_BoxSelect1d;
                VanGuiMultiSelectIO* ms_io = VanGui::BeginMultiSelect(flags, selection.Size, ITEMS_COUNT);
                selection.ApplyRequests(ms_io);

                for (int n = 0; n < ITEMS_COUNT; n++)
                {
                    char label[64];
                    snprintf(label, sizeof(label), "Object %05d: %s", n, ExampleNames[n % VAN_COUNTOF(ExampleNames)]);
                    bool item_is_selected = selection.Contains(static_cast<VanGuiID>(n));
                    VanGui::SetNextItemSelectionUserData(n);
                    VanGui::Selectable(label, item_is_selected);
                }

                ms_io = VanGui::EndMultiSelect();
                selection.ApplyRequests(ms_io);
            }
            VanGui::EndChild();
            VanGui::TreePop();
        }

        // Demonstrate using the clipper with BeginMultiSelect()/EndMultiSelect()
        if (VanGui::TreeNode("Multi-Select (with clipper)"))
        {
            VANGUI_DEMO_MARKER("Widgets/Selection State/Multi-Select (with clipper)");
            // Use default selection.Adapter: Pass index to SetNextItemSelectionUserData(), store index in Selection
            static VanGuiSelectionBasicStorage selection;

            VanGui::Text("Added features:");
            VanGui::BulletText("Using VanGuiListClipper.");

            const int ITEMS_COUNT = 10000;
            VanGui::Text("Selection: %d/%d", selection.Size, ITEMS_COUNT);
            if (VanGui::BeginChild("##Basket", VanVec2(-FLT_MIN, VanGui::GetFontSize() * 20), VanGuiChildFlags_FrameStyle | VanGuiChildFlags_ResizeY))
            {
                VanGuiMultiSelectFlags flags = VanGuiMultiSelectFlags_ClearOnEscape | VanGuiMultiSelectFlags_BoxSelect1d;
                VanGuiMultiSelectIO* ms_io = VanGui::BeginMultiSelect(flags, selection.Size, ITEMS_COUNT);
                selection.ApplyRequests(ms_io);

                VanGuiListClipper clipper;
                clipper.Begin(ITEMS_COUNT);
                if (ms_io->RangeSrcItem != -1)
                    clipper.IncludeItemByIndex(static_cast<int>(ms_io->RangeSrcItem)); // Ensure RangeSrc item is not clipped.
                while (clipper.Step())
                {
                    for (int n = clipper.DisplayStart; n < clipper.DisplayEnd; n++)
                    {
                        char label[64];
                        snprintf(label, sizeof(label), "Object %05d: %s", n, ExampleNames[n % VAN_COUNTOF(ExampleNames)]);
                        bool item_is_selected = selection.Contains(static_cast<VanGuiID>(n));
                        VanGui::SetNextItemSelectionUserData(n);
                        VanGui::Selectable(label, item_is_selected);
                    }
                }

                ms_io = VanGui::EndMultiSelect();
                selection.ApplyRequests(ms_io);
            }
            VanGui::EndChild();
            VanGui::TreePop();
        }

        // Demonstrate dynamic item list + deletion support using the BeginMultiSelect/EndMultiSelect API.
        // In order to support Deletion without any glitches you need to:
        // - (1) If items are submitted in their own scrolling area, submit contents size SetNextWindowContentSize() ahead of time to prevent one-frame readjustment of scrolling.
        // - (2) Items needs to have persistent ID Stack identifier = ID needs to not depends on their index. PushID(index) = KO. PushID(item_id) = OK. This is in order to focus items reliably after a selection.
        // - (3) BeginXXXX process
        // - (4) Focus process
        // - (5) EndXXXX process
        if (VanGui::TreeNode("Multi-Select (with deletion)"))
        {
            VANGUI_DEMO_MARKER("Widgets/Selection State/Multi-Select (with deletion)");
            // Storing items data separately from selection data.
            // (you may decide to store selection data inside your item (aka intrusive storage) if you don't need multiple views over same items)
            // Use a custom selection.Adapter: store item identifier in Selection (instead of index)
            static VanVector<VanGuiID> items;
            static ExampleSelectionWithDeletion selection;
            selection.UserData = static_cast<void*>(&items);
            selection.AdapterIndexToStorageId = [](VanGuiSelectionBasicStorage* self, int idx) { VanVector<VanGuiID>* p_items = static_cast<VanVector<VanGuiID>*>(self->UserData); return (*p_items)[idx]; }; // Index -> ID

            VanGui::Text("Added features:");
            VanGui::BulletText("Dynamic list with Delete key support.");
            VanGui::Text("Selection size: %d/%d", selection.Size, items.Size);

            // Initialize default list with 50 items + button to add/remove items.
            static VanGuiID items_next_id = 0;
            if (items_next_id == 0)
                for (VanGuiID n = 0; n < 50; n++)
                    items.push_back(items_next_id++);
            if (VanGui::SmallButton("Add 20 items"))     { for (int n = 0; n < 20; n++) { items.push_back(items_next_id++); } }
            VanGui::SameLine();
            if (VanGui::SmallButton("Remove 20 items"))  { for (int n = VAN_MIN(20, items.Size); n > 0; n--) { selection.SetItemSelected(items.back(), false); items.pop_back(); } }

            // (1) Extra to support deletion: Submit scrolling range to avoid glitches on deletion
            const float items_height = VanGui::GetTextLineHeightWithSpacing();
            VanGui::SetNextWindowContentSize(VanVec2(0.0f, items.Size * items_height));

            if (VanGui::BeginChild("##Basket", VanVec2(-FLT_MIN, VanGui::GetFontSize() * 20), VanGuiChildFlags_FrameStyle | VanGuiChildFlags_ResizeY))
            {
                VanGuiMultiSelectFlags flags = VanGuiMultiSelectFlags_ClearOnEscape | VanGuiMultiSelectFlags_BoxSelect1d;
                VanGuiMultiSelectIO* ms_io = VanGui::BeginMultiSelect(flags, selection.Size, items.Size);
                selection.ApplyRequests(ms_io);

                const bool want_delete = VanGui::Shortcut(VanGuiKey_Delete, VanGuiInputFlags_Repeat) && (selection.Size > 0);
                const int item_curr_idx_to_focus = want_delete ? selection.ApplyDeletionPreLoop(ms_io, items.Size) : -1;

                for (int n = 0; n < items.Size; n++)
                {
                    const VanGuiID item_id = items[n];
                    char label[64];
                    snprintf(label, sizeof(label), "Object %05u: %s", item_id, ExampleNames[item_id % VAN_COUNTOF(ExampleNames)]);

                    bool item_is_selected = selection.Contains(item_id);
                    VanGui::SetNextItemSelectionUserData(n);
                    VanGui::Selectable(label, item_is_selected);
                    if (item_curr_idx_to_focus == n)
                        VanGui::SetKeyboardFocusHere(-1);
                }

                // Apply multi-select requests
                ms_io = VanGui::EndMultiSelect();
                selection.ApplyRequests(ms_io);
                if (want_delete)
                    selection.ApplyDeletionPostLoop(ms_io, items, item_curr_idx_to_focus);
            }
            VanGui::EndChild();
            VanGui::TreePop();
        }

        // Implement a Dual List Box (#6648)
        if (VanGui::TreeNode("Multi-Select (dual list box)"))
        {
            VANGUI_DEMO_MARKER("Widgets/Selection State/Multi-Select (dual list box)");
            // Init default state
            static ExampleDualListBox dlb;
            if (dlb.Items[0].Size == 0 && dlb.Items[1].Size == 0)
                for (int item_id = 0; item_id < VAN_COUNTOF(ExampleNames); item_id++)
                    dlb.Items[0].push_back(static_cast<VanGuiID>(item_id));

            // Show
            dlb.Show();

            VanGui::TreePop();
        }

        // Demonstrate using the clipper with BeginMultiSelect()/EndMultiSelect()
        if (VanGui::TreeNode("Multi-Select (in a table)"))
        {
            VANGUI_DEMO_MARKER("Widgets/Selection State/Multi-Select (in a table)");
            static VanGuiSelectionBasicStorage selection;

            const int ITEMS_COUNT = 10000;
            VanGui::Text("Selection: %d/%d", selection.Size, ITEMS_COUNT);
            if (VanGui::BeginTable("##Basket", 2, VanGuiTableFlags_ScrollY | VanGuiTableFlags_RowBg | VanGuiTableFlags_BordersOuter, VanVec2(0.0f, VanGui::GetFontSize() * 20)))
            {
                VanGui::TableSetupColumn("Object");
                VanGui::TableSetupColumn("Action");
                VanGui::TableSetupScrollFreeze(0, 1);
                VanGui::TableHeadersRow();

                VanGuiMultiSelectFlags flags = VanGuiMultiSelectFlags_ClearOnEscape | VanGuiMultiSelectFlags_BoxSelect1d;
                VanGuiMultiSelectIO* ms_io = VanGui::BeginMultiSelect(flags, selection.Size, ITEMS_COUNT);
                selection.ApplyRequests(ms_io);

                VanGuiListClipper clipper;
                clipper.Begin(ITEMS_COUNT);
                if (ms_io->RangeSrcItem != -1)
                    clipper.IncludeItemByIndex(static_cast<int>(ms_io->RangeSrcItem)); // Ensure RangeSrc item is not clipped.
                while (clipper.Step())
                {
                    for (int n = clipper.DisplayStart; n < clipper.DisplayEnd; n++)
                    {
                        VanGui::TableNextRow();
                        VanGui::TableNextColumn();
                        VanGui::PushID(n);
                        char label[64];
                        snprintf(label, sizeof(label), "Object %05d: %s", n, ExampleNames[n % VAN_COUNTOF(ExampleNames)]);
                        bool item_is_selected = selection.Contains(static_cast<VanGuiID>(n));
                        VanGui::SetNextItemSelectionUserData(n);
                        VanGui::Selectable(label, item_is_selected, VanGuiSelectableFlags_SpanAllColumns | VanGuiSelectableFlags_AllowOverlap);
                        VanGui::TableNextColumn();
                        VanGui::SmallButton("hello");
                        VanGui::PopID();
                    }
                }

                ms_io = VanGui::EndMultiSelect();
                selection.ApplyRequests(ms_io);
                VanGui::EndTable();
            }
            VanGui::TreePop();
        }

        if (VanGui::TreeNode("Multi-Select (checkboxes)"))
        {
            VANGUI_DEMO_MARKER("Widgets/Selection State/Multi-Select (checkboxes)");
            VanGui::Text("In a list of checkboxes (not selectable):");
            VanGui::BulletText("Using _NoAutoSelect + _NoAutoClear flags.");
            VanGui::BulletText("Shift+Click to check multiple boxes.");
            VanGui::BulletText("Shift+Keyboard to copy current value to other boxes.");

            // If you have an array of checkboxes, you may want to use NoAutoSelect + NoAutoClear and the VanGuiSelectionExternalStorage helper.
            static bool items[20] = {};
            static VanGuiMultiSelectFlags flags = VanGuiMultiSelectFlags_NoAutoSelect | VanGuiMultiSelectFlags_NoAutoClear | VanGuiMultiSelectFlags_ClearOnEscape;
            VanGui::CheckboxFlags("VanGuiMultiSelectFlags_NoAutoSelect", &flags, VanGuiMultiSelectFlags_NoAutoSelect);
            VanGui::CheckboxFlags("VanGuiMultiSelectFlags_NoAutoClear", &flags, VanGuiMultiSelectFlags_NoAutoClear);
            VanGui::CheckboxFlags("VanGuiMultiSelectFlags_BoxSelect2d", &flags, VanGuiMultiSelectFlags_BoxSelect2d); // Cannot use VanGuiMultiSelectFlags_BoxSelect1d as checkboxes are varying width.

            if (VanGui::BeginChild("##Basket", VanVec2(-FLT_MIN, VanGui::GetFontSize() * 20), VanGuiChildFlags_Borders | VanGuiChildFlags_ResizeY))
            {
                VanGuiMultiSelectIO* ms_io = VanGui::BeginMultiSelect(flags, -1, VAN_COUNTOF(items));
                VanGuiSelectionExternalStorage storage_wrapper;
                storage_wrapper.UserData = static_cast<void*>(items);
                storage_wrapper.AdapterSetItemSelected = [](VanGuiSelectionExternalStorage* self, int n, bool selected) { bool* array = static_cast<bool*>(self->UserData); array[n] = selected; };
                storage_wrapper.ApplyRequests(ms_io);
                for (int n = 0; n < 20; n++)
                {
                    char label[32];
                    snprintf(label, sizeof(label), "Item %d", n);
                    VanGui::SetNextItemSelectionUserData(n);
                    VanGui::Checkbox(label, &items[n]);
                }
                ms_io = VanGui::EndMultiSelect();
                storage_wrapper.ApplyRequests(ms_io);
            }
            VanGui::EndChild();

            VanGui::TreePop();
        }

        // Demonstrate individual selection scopes in same window
        if (VanGui::TreeNode("Multi-Select (multiple scopes)"))
        {
            VANGUI_DEMO_MARKER("Widgets/Selection State/Multi-Select (multiple scopes)");
            // Use default select: Pass index to SetNextItemSelectionUserData(), store index in Selection
            const int SCOPES_COUNT = 3;
            const int ITEMS_COUNT = 8; // Per scope
            static VanGuiSelectionBasicStorage selections_data[SCOPES_COUNT];

            // Use VanGuiMultiSelectFlags_ScopeRect to not affect other selections in same window.
            static VanGuiMultiSelectFlags flags = VanGuiMultiSelectFlags_ScopeRect | VanGuiMultiSelectFlags_ClearOnEscape;// | VanGuiMultiSelectFlags_ClearOnClickVoid;
            if (VanGui::CheckboxFlags("VanGuiMultiSelectFlags_ScopeWindow", &flags, VanGuiMultiSelectFlags_ScopeWindow) && (flags & VanGuiMultiSelectFlags_ScopeWindow))
                flags &= ~VanGuiMultiSelectFlags_ScopeRect;
            if (VanGui::CheckboxFlags("VanGuiMultiSelectFlags_ScopeRect", &flags, VanGuiMultiSelectFlags_ScopeRect) && (flags & VanGuiMultiSelectFlags_ScopeRect))
                flags &= ~VanGuiMultiSelectFlags_ScopeWindow;
            VanGui::CheckboxFlags("VanGuiMultiSelectFlags_ClearOnClickVoid", &flags, VanGuiMultiSelectFlags_ClearOnClickVoid);
            VanGui::CheckboxFlags("VanGuiMultiSelectFlags_BoxSelect1d", &flags, VanGuiMultiSelectFlags_BoxSelect1d);

            for (int selection_scope_n = 0; selection_scope_n < SCOPES_COUNT; selection_scope_n++)
            {
                VanGui::PushID(selection_scope_n);
                VanGuiSelectionBasicStorage* selection = &selections_data[selection_scope_n];
                VanGuiMultiSelectIO* ms_io = VanGui::BeginMultiSelect(flags, selection->Size, ITEMS_COUNT);
                selection->ApplyRequests(ms_io);

                VanGui::SeparatorText("Selection scope");
                VanGui::Text("Selection size: %d/%d", selection->Size, ITEMS_COUNT);

                for (int n = 0; n < ITEMS_COUNT; n++)
                {
                    char label[64];
                    snprintf(label, sizeof(label), "Object %05d: %s", n, ExampleNames[n % VAN_COUNTOF(ExampleNames)]);
                    bool item_is_selected = selection->Contains(static_cast<VanGuiID>(n));
                    VanGui::SetNextItemSelectionUserData(n);
                    VanGui::Selectable(label, item_is_selected);
                }

                // Apply multi-select requests
                ms_io = VanGui::EndMultiSelect();
                selection->ApplyRequests(ms_io);
                VanGui::PopID();
            }
            VanGui::TreePop();
        }

        // See ShowExampleAppAssetsBrowser()
        if (VanGui::TreeNode("Multi-Select (tiled assets browser)"))
        {
            VanGui::Checkbox("Assets Browser", &demo_data->ShowAppAssetsBrowser);
            VanGui::Text("(also access from 'Examples->Assets Browser' in menu)");
            VanGui::TreePop();
        }

        // Demonstrate supporting multiple-selection in a tree.
        // - We don't use linear indices for selection user data, but our ExampleTreeNode* pointer directly!
        //   This showcase how SetNextItemSelectionUserData() never assume indices!
        // - The difficulty here is to "interpolate" from RangeSrcItem to RangeDstItem in the SetAll/SetRange request.
        //   We want this interpolation to match what the user sees: in visible order, skipping closed nodes.
        //   This is implemented by our TreeGetNextNodeInVisibleOrder() user-space helper.
        // - Important: In a real codebase aiming to implement full-featured selectable tree with custom filtering, you
        //   are more likely to build an array mapping sequential indices to visible tree nodes, since your
        //   filtering/search + clipping process will benefit from it. Having this will make this interpolation much easier.
        // - Consider this a prototype: we are working toward simplifying some of it.
        if (VanGui::TreeNode("Multi-Select (trees)"))
        {
            VANGUI_DEMO_MARKER("Widgets/Selection State/Multi-Select (trees)");
            HelpMarker(
                "This is rather advanced and experimental. If you are getting started with multi-select, "
                "please don't start by looking at how to use it for a tree!\n\n"
                "Future versions will try to simplify and formalize some of this.");

            struct ExampleTreeFuncs
            {
                static void DrawNode(ExampleTreeNode* node, VanGuiSelectionBasicStorage* selection)
                {
                    VanGuiTreeNodeFlags tree_node_flags = VanGuiTreeNodeFlags_SpanAvailWidth | VanGuiTreeNodeFlags_OpenOnArrow | VanGuiTreeNodeFlags_OpenOnDoubleClick;
                    tree_node_flags |= VanGuiTreeNodeFlags_NavLeftJumpsToParent; // Enable pressing left to jump to parent
                    if (node->Childs.Size == 0)
                        tree_node_flags |= VanGuiTreeNodeFlags_Bullet | VanGuiTreeNodeFlags_Leaf;
                    if (selection->Contains(static_cast<VanGuiID>(node->UID)))
                        tree_node_flags |= VanGuiTreeNodeFlags_Selected;

                    // Using SetNextItemStorageID() to specify storage id, so we can easily peek into
                    // the storage holding open/close stage, using our TreeNodeGetOpen/TreeNodeSetOpen() functions.
                    VanGui::SetNextItemSelectionUserData(static_cast<VanGuiSelectionUserData>(reinterpret_cast<intptr_t>(node)));
                    VanGui::SetNextItemStorageID(static_cast<VanGuiID>(node->UID));
                    if (VanGui::TreeNodeEx(node->Name, tree_node_flags))
                    {
                        for (ExampleTreeNode* child : node->Childs)
                            DrawNode(child, selection);
                        VanGui::TreePop();
                    }
                    else if (VanGui::IsItemToggledOpen())
                    {
                        TreeCloseAndUnselectChildNodes(node, selection);
                    }
                }

                // When closing a node: 1) close and unselect all child nodes, 2) select parent if any child was selected.
                // FIXME: This is currently handled by user logic but I'm hoping to eventually provide tree node
                // features to do this automatically, e.g. a VanGuiTreeNodeFlags_AutoCloseChildNodes etc.
                static int TreeCloseAndUnselectChildNodes(ExampleTreeNode* node, VanGuiSelectionBasicStorage* selection, int depth = 0)
                {
                    // Recursive close (the test for depth == 0 is because we call this on a node that was just closed!)
                    int unselected_count = selection->Contains(static_cast<VanGuiID>(node->UID)) ? 1 : 0;
                    if (depth == 0 || VanGui::TreeNodeGetOpen(static_cast<VanGuiID>(node->UID)))
                    {
                        for (ExampleTreeNode* child : node->Childs)
                            unselected_count += TreeCloseAndUnselectChildNodes(child, selection, depth + 1);
                        VanGui::TreeNodeSetOpen(static_cast<VanGuiID>(node->UID), false);
                    }

                    // Select root node if any of its child was selected, otherwise unselect
                    selection->SetItemSelected(static_cast<VanGuiID>(node->UID), (depth == 0 && unselected_count > 0));
                    return unselected_count;
                }

                // Apply multi-selection requests
                static void ApplySelectionRequests(VanGuiMultiSelectIO* ms_io, ExampleTreeNode* tree, VanGuiSelectionBasicStorage* selection)
                {
                    for (VanGuiSelectionRequest& req : ms_io->Requests)
                    {
                        if (req.Type == VanGuiSelectionRequestType_SetAll)
                        {
                            if (req.Selected)
                                TreeSetAllInOpenNodes(tree, selection, req.Selected);
                            else
                                selection->Clear();
                        }
                        else if (req.Type == VanGuiSelectionRequestType_SetRange)
                        {
                            ExampleTreeNode* first_node = reinterpret_cast<ExampleTreeNode*>(static_cast<intptr_t>(req.RangeFirstItem));
                            ExampleTreeNode* last_node = reinterpret_cast<ExampleTreeNode*>(static_cast<intptr_t>(req.RangeLastItem));
                            for (ExampleTreeNode* node = first_node; node != nullptr; node = TreeGetNextNodeInVisibleOrder(node, last_node))
                                selection->SetItemSelected(static_cast<VanGuiID>(node->UID), req.Selected);
                        }
                    }
                }

                static void TreeSetAllInOpenNodes(ExampleTreeNode* node, VanGuiSelectionBasicStorage* selection, bool selected)
                {
                    if (node->Parent != nullptr) // Root node isn't visible nor selectable in our scheme
                        selection->SetItemSelected(static_cast<VanGuiID>(node->UID), selected);
                    if (node->Parent == nullptr || VanGui::TreeNodeGetOpen(static_cast<VanGuiID>(node->UID)))
                        for (ExampleTreeNode* child : node->Childs)
                            TreeSetAllInOpenNodes(child, selection, selected);
                }

                // Interpolate in *user-visible order* AND only *over opened nodes*.
                // If you have a sequential mapping tables (e.g. generated after a filter/search pass) this would be simpler.
                // Here the tricks are that:
                // - we store/maintain ExampleTreeNode::IndexInParent which allows implementing a linear iterator easily, without searches, without recursion.
                //   this could be replaced by a search in parent, aka 'int index_in_parent = curr_node->Parent->Childs.find_index(curr_node)'
                //   which would only be called when crossing from child to a parent, aka not too much.
                // - we call SetNextItemStorageID() before our TreeNode() calls with an ID which doesn't relate to UI stack,
                //   making it easier to call TreeNodeGetOpen()/TreeNodeSetOpen() from any location.
                static ExampleTreeNode* TreeGetNextNodeInVisibleOrder(ExampleTreeNode* curr_node, ExampleTreeNode* last_node)
                {
                    // Reached last node
                    if (curr_node == last_node)
                        return nullptr;

                    // Recurse into childs. Query storage to tell if the node is open.
                    if (curr_node->Childs.Size > 0 && VanGui::TreeNodeGetOpen(static_cast<VanGuiID>(curr_node->UID)))
                        return curr_node->Childs[0];

                    // Next sibling, then into our own parent
                    while (curr_node->Parent != nullptr)
                    {
                        if (curr_node->IndexInParent + 1 < curr_node->Parent->Childs.Size)
                            return curr_node->Parent->Childs[curr_node->IndexInParent + 1];
                        curr_node = curr_node->Parent;
                    }
                    return nullptr;
                }

            }; // ExampleTreeFuncs

            static VanGuiSelectionBasicStorage selection;
            if (demo_data->DemoTree == nullptr)
                demo_data->DemoTree = ExampleTree_CreateDemoTree(); // Create tree once
            VanGui::Text("Selection size: %d", selection.Size);

            if (VanGui::BeginChild("##Tree", VanVec2(-FLT_MIN, VanGui::GetFontSize() * 20), VanGuiChildFlags_FrameStyle | VanGuiChildFlags_ResizeY))
            {
                ExampleTreeNode* tree = demo_data->DemoTree;
                VanGuiMultiSelectFlags ms_flags = VanGuiMultiSelectFlags_ClearOnEscape | VanGuiMultiSelectFlags_BoxSelect2d;
                VanGuiMultiSelectIO* ms_io = VanGui::BeginMultiSelect(ms_flags, selection.Size, -1);
                ExampleTreeFuncs::ApplySelectionRequests(ms_io, tree, &selection);
                for (ExampleTreeNode* node : tree->Childs)
                    ExampleTreeFuncs::DrawNode(node, &selection);
                ms_io = VanGui::EndMultiSelect();
                ExampleTreeFuncs::ApplySelectionRequests(ms_io, tree, &selection);
            }
            VanGui::EndChild();

            VanGui::TreePop();
        }

        // Advanced demonstration of BeginMultiSelect()
        // - Showcase clipping.
        // - Showcase deletion.
        // - Showcase basic drag and drop.
        // - Showcase TreeNode variant (note that tree node don't expand in the demo: supporting expanding tree nodes + clipping a separate thing).
        // - Showcase using inside a table.
        //VanGui::SetNextItemOpen(true, VanGuiCond_Once);
        if (VanGui::TreeNode("Multi-Select (advanced)"))
        {
            VANGUI_DEMO_MARKER("Widgets/Selection State/Multi-Select (advanced)");
            // Options
            enum WidgetType { WidgetType_Selectable, WidgetType_TreeNode };
            static bool use_clipper = true;
            static bool use_deletion = true;
            static bool use_drag_drop = true;
            static bool show_in_table = false;
            static bool show_color_button = true;
            static VanGuiMultiSelectFlags flags = VanGuiMultiSelectFlags_ClearOnEscape | VanGuiMultiSelectFlags_BoxSelect1d;
            static WidgetType widget_type = WidgetType_Selectable;

            if (VanGui::TreeNode("Options"))
            {
                if (VanGui::RadioButton("Selectables", widget_type == WidgetType_Selectable)) { widget_type = WidgetType_Selectable; }
                VanGui::SameLine();
                if (VanGui::RadioButton("Tree nodes", widget_type == WidgetType_TreeNode)) { widget_type = WidgetType_TreeNode; }
                VanGui::SameLine();
                HelpMarker("TreeNode() is technically supported but... using this correctly is more complicated (you need some sort of linear/random access to your tree, which is suited to advanced trees setups already implementing filters and clipper. We will work toward simplifying and demoing this.\n\nFor now the tree demo is actually a little bit meaningless because it is an empty tree with only root nodes.");
                VanGui::Checkbox("Enable clipper", &use_clipper);
                VanGui::Checkbox("Enable deletion", &use_deletion);
                VanGui::Checkbox("Enable drag & drop", &use_drag_drop);
                VanGui::Checkbox("Show in a table", &show_in_table);
                VanGui::Checkbox("Show color button", &show_color_button);
                VanGui::CheckboxFlags("VanGuiMultiSelectFlags_SingleSelect", &flags, VanGuiMultiSelectFlags_SingleSelect);
                VanGui::CheckboxFlags("VanGuiMultiSelectFlags_NoSelectAll", &flags, VanGuiMultiSelectFlags_NoSelectAll);
                VanGui::CheckboxFlags("VanGuiMultiSelectFlags_NoRangeSelect", &flags, VanGuiMultiSelectFlags_NoRangeSelect);
                VanGui::CheckboxFlags("VanGuiMultiSelectFlags_NoAutoSelect", &flags, VanGuiMultiSelectFlags_NoAutoSelect);
                VanGui::CheckboxFlags("VanGuiMultiSelectFlags_NoAutoClear", &flags, VanGuiMultiSelectFlags_NoAutoClear);
                VanGui::CheckboxFlags("VanGuiMultiSelectFlags_NoAutoClearOnReselect", &flags, VanGuiMultiSelectFlags_NoAutoClearOnReselect);
                VanGui::CheckboxFlags("VanGuiMultiSelectFlags_NoSelectOnRightClick", &flags, VanGuiMultiSelectFlags_NoSelectOnRightClick);
                VanGui::CheckboxFlags("VanGuiMultiSelectFlags_BoxSelect1d", &flags, VanGuiMultiSelectFlags_BoxSelect1d);
                VanGui::CheckboxFlags("VanGuiMultiSelectFlags_BoxSelect2d", &flags, VanGuiMultiSelectFlags_BoxSelect2d);
                VanGui::CheckboxFlags("VanGuiMultiSelectFlags_BoxSelectNoScroll", &flags, VanGuiMultiSelectFlags_BoxSelectNoScroll);
                VanGui::CheckboxFlags("VanGuiMultiSelectFlags_ClearOnEscape", &flags, VanGuiMultiSelectFlags_ClearOnEscape);
                VanGui::CheckboxFlags("VanGuiMultiSelectFlags_ClearOnClickVoid", &flags, VanGuiMultiSelectFlags_ClearOnClickVoid);
                if (VanGui::CheckboxFlags("VanGuiMultiSelectFlags_ScopeWindow", &flags, VanGuiMultiSelectFlags_ScopeWindow) && (flags & VanGuiMultiSelectFlags_ScopeWindow))
                    flags &= ~VanGuiMultiSelectFlags_ScopeRect;
                if (VanGui::CheckboxFlags("VanGuiMultiSelectFlags_ScopeRect", &flags, VanGuiMultiSelectFlags_ScopeRect) && (flags & VanGuiMultiSelectFlags_ScopeRect))
                    flags &= ~VanGuiMultiSelectFlags_ScopeWindow;
                if (VanGui::CheckboxFlags("VanGuiMultiSelectFlags_SelectOnAuto", &flags, VanGuiMultiSelectFlags_SelectOnAuto))
                    flags &= ~(VanGuiMultiSelectFlags_SelectOnMask_ ^ VanGuiMultiSelectFlags_SelectOnAuto);
                VanGui::SameLine(); HelpMarker("Apply selection on mouse down when clicking on unselected item, on mouse up when clicking on selected item. (Default)");
                if (VanGui::CheckboxFlags("VanGuiMultiSelectFlags_SelectOnClickAlways", &flags, VanGuiMultiSelectFlags_SelectOnClickAlways))
                    flags &= ~(VanGuiMultiSelectFlags_SelectOnMask_ ^ VanGuiMultiSelectFlags_SelectOnClickAlways);
                VanGui::SameLine(); HelpMarker("Prevents Drag and Drop from being used on multi-selection, but allows e.g. BoxSelect to always reselect even when clicking inside an existing selection. (Excel style behavior)");
                if (VanGui::CheckboxFlags("VanGuiMultiSelectFlags_SelectOnClickRelease", &flags, VanGuiMultiSelectFlags_SelectOnClickRelease))
                    flags &= ~(VanGuiMultiSelectFlags_SelectOnMask_ ^ VanGuiMultiSelectFlags_SelectOnClickRelease);
                VanGui::SameLine(); HelpMarker("Allow dragging an unselected item without altering selection.");
                VanGui::TreePop();
            }

            // Initialize default list with 1000 items.
            // Use default selection.Adapter: Pass index to SetNextItemSelectionUserData(), store index in Selection
            static VanVector<int> items;
            static int items_next_id = 0;
            if (items_next_id == 0) { for (int n = 0; n < 1000; n++) { items.push_back(items_next_id++); } }
            static ExampleSelectionWithDeletion selection;
            static bool request_deletion_from_menu = false; // Queue deletion triggered from context menu

            VanGui::Text("Selection size: %d/%d", selection.Size, items.Size);

            const float items_height = (widget_type == WidgetType_TreeNode) ? VanGui::GetTextLineHeight() : VanGui::GetTextLineHeightWithSpacing();
            VanGui::SetNextWindowContentSize(VanVec2(0.0f, items.Size * items_height));
            if (VanGui::BeginChild("##Basket", VanVec2(-FLT_MIN, VanGui::GetFontSize() * 20), VanGuiChildFlags_FrameStyle | VanGuiChildFlags_ResizeY))
            {
                VanVec2 color_button_sz(VanGui::GetFontSize(), VanGui::GetFontSize());
                if (widget_type == WidgetType_TreeNode)
                    VanGui::PushStyleVarY(VanGuiStyleVar_ItemSpacing, 0.0f);

                VanGuiMultiSelectIO* ms_io = VanGui::BeginMultiSelect(flags, selection.Size, items.Size);
                selection.ApplyRequests(ms_io);

                const bool want_delete = (VanGui::Shortcut(VanGuiKey_Delete, VanGuiInputFlags_Repeat) && (selection.Size > 0)) || request_deletion_from_menu;
                const int item_curr_idx_to_focus = want_delete ? selection.ApplyDeletionPreLoop(ms_io, items.Size) : -1;
                request_deletion_from_menu = false;

                if (show_in_table)
                {
                    if (widget_type == WidgetType_TreeNode)
                        VanGui::PushStyleVar(VanGuiStyleVar_CellPadding, VanVec2(0.0f, 0.0f));
                    VanGui::BeginTable("##Split", 2, VanGuiTableFlags_Resizable | VanGuiTableFlags_NoSavedSettings | VanGuiTableFlags_NoPadOuterX);
                    VanGui::TableSetupColumn("", VanGuiTableColumnFlags_WidthStretch, 0.70f);
                    VanGui::TableSetupColumn("", VanGuiTableColumnFlags_WidthStretch, 0.30f);
                    //VanGui::PushStyleVarY(VanGuiStyleVar_ItemSpacing, 0.0f);
                }

                VanGuiListClipper clipper;
                if (use_clipper)
                {
                    clipper.Begin(items.Size);
                    if (item_curr_idx_to_focus != -1)
                        clipper.IncludeItemByIndex(item_curr_idx_to_focus); // Ensure focused item is not clipped.
                    if (ms_io->RangeSrcItem != -1)
                        clipper.IncludeItemByIndex(static_cast<int>(ms_io->RangeSrcItem)); // Ensure RangeSrc item is not clipped.
                }

                while (!use_clipper || clipper.Step())
                {
                    const int item_begin = use_clipper ? clipper.DisplayStart : 0;
                    const int item_end = use_clipper ? clipper.DisplayEnd : items.Size;
                    for (int n = item_begin; n < item_end; n++)
                    {
                        if (show_in_table)
                            VanGui::TableNextColumn();

                        const int item_id = items[n];
                        const char* item_category = ExampleNames[item_id % VAN_COUNTOF(ExampleNames)];
                        char label[64];
                        snprintf(label, sizeof(label), "Object %05d: %s", item_id, item_category);

                        // IMPORTANT: for deletion refocus to work we need object ID to be stable,
                        // aka not depend on their index in the list. Here we use our persistent item_id
                        // instead of index to build a unique ID that will persist.
                        // (If we used PushID(index) instead, focus wouldn't be restored correctly after deletion).
                        VanGui::PushID(item_id);

                        // Emit a color button, to test that Shift+LeftArrow landing on an item that is not part
                        // of the selection scope doesn't erroneously alter our selection.
                        if (show_color_button)
                        {
                            VanU32 dummy_col = (VanU32)((unsigned int)n * 0xC250B74B) | VAN_COL32_A_MASK;
                            VanGui::ColorButton("##", VanColor(dummy_col), VanGuiColorEditFlags_NoTooltip, color_button_sz);
                            VanGui::SameLine();
                        }

                        // Submit item
                        bool item_is_selected = selection.Contains(static_cast<VanGuiID>(n));
                        bool item_is_open = false;
                        VanGui::SetNextItemSelectionUserData(n);
                        if (widget_type == WidgetType_Selectable)
                        {
                            VanGui::Selectable(label, item_is_selected, VanGuiSelectableFlags_None);
                        }
                        else if (widget_type == WidgetType_TreeNode)
                        {
                            VanGuiTreeNodeFlags tree_node_flags = VanGuiTreeNodeFlags_SpanAvailWidth | VanGuiTreeNodeFlags_OpenOnArrow | VanGuiTreeNodeFlags_OpenOnDoubleClick;
                            if (item_is_selected)
                                tree_node_flags |= VanGuiTreeNodeFlags_Selected;
                            item_is_open = VanGui::TreeNodeEx(label, tree_node_flags);
                        }

                        // Focus (for after deletion)
                        if (item_curr_idx_to_focus == n)
                            VanGui::SetKeyboardFocusHere(-1);

                        // Drag and Drop
                        if (use_drag_drop && VanGui::BeginDragDropSource())
                        {
                            // Create payload with full selection OR single unselected item.
                            // (the later is only possible when using VanGuiMultiSelectFlags_SelectOnClickRelease)
                            if (VanGui::GetDragDropPayload() == nullptr)
                            {
                                VanVector<int> payload_items;
                                void* it = nullptr;
                                VanGuiID id = 0;
                                if (!item_is_selected)
                                    payload_items.push_back(item_id);
                                else
                                    while (selection.GetNextSelectedItem(&it, &id))
                                        payload_items.push_back(static_cast<int>(id));
                                VanGui::SetDragDropPayload("MULTISELECT_DEMO_ITEMS", payload_items.Data, static_cast<size_t>(payload_items.size_in_bytes()));
                            }

                            // Display payload content in tooltip
                            const VanGuiPayload* payload = VanGui::GetDragDropPayload();
                            const int* payload_items = static_cast<const int*>(payload->Data);
                            const int payload_count = static_cast<int>(payload->DataSize) / static_cast<int>(sizeof(int));
                            if (payload_count == 1)
                                VanGui::Text("Object %05d: %s", payload_items[0], ExampleNames[payload_items[0] % VAN_COUNTOF(ExampleNames)]);
                            else
                                VanGui::Text("Dragging %d objects", payload_count);

                            VanGui::EndDragDropSource();
                        }

                        if (widget_type == WidgetType_TreeNode && item_is_open)
                            VanGui::TreePop();

                        // Right-click: context menu
                        if (VanGui::BeginPopupContextItem())
                        {
                            VanGui::BeginDisabled(!use_deletion || selection.Size == 0);
                            snprintf(label, sizeof(label), "Delete %d item(s)###DeleteSelected", selection.Size);
                            if (VanGui::Selectable(label))
                                request_deletion_from_menu = true;
                            VanGui::EndDisabled();
                            VanGui::Selectable("Close");
                            VanGui::EndPopup();
                        }

                        // Demo content within a table
                        if (show_in_table)
                        {
                            VanGui::TableNextColumn();
                            VanGui::SetNextItemWidth(-FLT_MIN);
                            VanGui::PushStyleVar(VanGuiStyleVar_FramePadding, VanVec2(0, 0));
                            VanGui::InputText("##NoLabel", const_cast<char*>(item_category), strlen(item_category), VanGuiInputTextFlags_ReadOnly);
                            VanGui::PopStyleVar();
                        }

                        VanGui::PopID();
                    }
                    if (!use_clipper)
                        break;
                }

                if (show_in_table)
                {
                    VanGui::EndTable();
                    if (widget_type == WidgetType_TreeNode)
                        VanGui::PopStyleVar();
                }

                // Apply multi-select requests
                ms_io = VanGui::EndMultiSelect();
                selection.ApplyRequests(ms_io);
                if (want_delete)
                    selection.ApplyDeletionPostLoop(ms_io, items, item_curr_idx_to_focus);

                if (widget_type == WidgetType_TreeNode)
                    VanGui::PopStyleVar();
            }
            VanGui::EndChild();
            VanGui::TreePop();
        }
        VanGui::TreePop();
    }
}

//-----------------------------------------------------------------------------
// [SECTION] DemoWindowWidgetsTabs()
//-----------------------------------------------------------------------------

static void EditTabBarFittingPolicyFlags(VanGuiTabBarFlags* p_flags)
{
    if ((*p_flags & VanGuiTabBarFlags_FittingPolicyMask_) == 0)
        *p_flags |= VanGuiTabBarFlags_FittingPolicyDefault_;
    if (VanGui::CheckboxFlags("VanGuiTabBarFlags_FittingPolicyMixed", p_flags, VanGuiTabBarFlags_FittingPolicyMixed))
        *p_flags &= ~(VanGuiTabBarFlags_FittingPolicyMask_ ^ VanGuiTabBarFlags_FittingPolicyMixed);
    if (VanGui::CheckboxFlags("VanGuiTabBarFlags_FittingPolicyShrink", p_flags, VanGuiTabBarFlags_FittingPolicyShrink))
        *p_flags &= ~(VanGuiTabBarFlags_FittingPolicyMask_ ^ VanGuiTabBarFlags_FittingPolicyShrink);
    if (VanGui::CheckboxFlags("VanGuiTabBarFlags_FittingPolicyScroll", p_flags, VanGuiTabBarFlags_FittingPolicyScroll))
        *p_flags &= ~(VanGuiTabBarFlags_FittingPolicyMask_ ^ VanGuiTabBarFlags_FittingPolicyScroll);
}

static void DemoWindowWidgetsTabs()
{
    if (VanGui::TreeNode("Tabs"))
    {
        VANGUI_DEMO_MARKER("Widgets/Tabs");
        if (VanGui::TreeNode("Basic"))
        {
            VANGUI_DEMO_MARKER("Widgets/Tabs/Basic");
            VanGuiTabBarFlags tab_bar_flags = VanGuiTabBarFlags_None;
            if (VanGui::BeginTabBar("MyTabBar", tab_bar_flags))
            {
                if (VanGui::BeginTabItem("Avocado"))
                {
                    VanGui::Text("This is the Avocado tab!\nblah blah blah blah blah");
                    VanGui::EndTabItem();
                }
                if (VanGui::BeginTabItem("Broccoli"))
                {
                    VanGui::Text("This is the Broccoli tab!\nblah blah blah blah blah");
                    VanGui::EndTabItem();
                }
                if (VanGui::BeginTabItem("Cucumber"))
                {
                    VanGui::Text("This is the Cucumber tab!\nblah blah blah blah blah");
                    VanGui::EndTabItem();
                }
                VanGui::EndTabBar();
            }
            VanGui::Separator();
            VanGui::TreePop();
        }

        if (VanGui::TreeNode("Advanced & Close Button"))
        {
            VANGUI_DEMO_MARKER("Widgets/Tabs/Advanced & Close Button");
            // Expose a couple of the available flags. In most cases you may just call BeginTabBar() with no flags (0).
            static VanGuiTabBarFlags tab_bar_flags = VanGuiTabBarFlags_Reorderable;
            VanGui::CheckboxFlags("VanGuiTabBarFlags_Reorderable", &tab_bar_flags, VanGuiTabBarFlags_Reorderable);
            VanGui::CheckboxFlags("VanGuiTabBarFlags_AutoSelectNewTabs", &tab_bar_flags, VanGuiTabBarFlags_AutoSelectNewTabs);
            VanGui::CheckboxFlags("VanGuiTabBarFlags_TabListPopupButton", &tab_bar_flags, VanGuiTabBarFlags_TabListPopupButton);
            VanGui::CheckboxFlags("VanGuiTabBarFlags_NoCloseWithMiddleMouseButton", &tab_bar_flags, VanGuiTabBarFlags_NoCloseWithMiddleMouseButton);
            VanGui::CheckboxFlags("VanGuiTabBarFlags_DrawSelectedOverline", &tab_bar_flags, VanGuiTabBarFlags_DrawSelectedOverline);
            EditTabBarFittingPolicyFlags(&tab_bar_flags);

            // Tab Bar
            VanGui::AlignTextToFramePadding();
            VanGui::Text("Opened:");
            const char* names[4] = { "Artichoke", "Beetroot", "Celery", "Daikon" };
            static bool opened[4] = { true, true, true, true }; // Persistent user state
            for (int n = 0; n < VAN_COUNTOF(opened); n++)
            {
                VanGui::SameLine();
                VanGui::Checkbox(names[n], &opened[n]);
            }

            // Passing a bool* to BeginTabItem() is similar to passing one to Begin():
            // the underlying bool will be set to false when the tab is closed.
            if (VanGui::BeginTabBar("MyTabBar", tab_bar_flags))
            {
                for (int n = 0; n < VAN_COUNTOF(opened); n++)
                    if (opened[n] && VanGui::BeginTabItem(names[n], &opened[n], VanGuiTabItemFlags_None))
                    {
                        VanGui::Text("This is the %s tab!", names[n]);
                        if (n & 1)
                            VanGui::Text("I am an odd tab.");
                        VanGui::EndTabItem();
                    }
                VanGui::EndTabBar();
            }
            VanGui::Separator();
            VanGui::TreePop();
        }

        if (VanGui::TreeNode("TabItemButton & Leading/Trailing flags"))
        {
            VANGUI_DEMO_MARKER("Widgets/Tabs/TabItemButton & Leading-Trailing flags");
            static VanVector<int> active_tabs;
            static int next_tab_id = 0;
            if (next_tab_id == 0) // Initialize with some default tabs
                for (int i = 0; i < 3; i++)
                    active_tabs.push_back(next_tab_id++);

            // TabItemButton() and Leading/Trailing flags are distinct features which we will demo together.
            // (It is possible to submit regular tabs with Leading/Trailing flags, or TabItemButton tabs without Leading/Trailing flags...
            // but they tend to make more sense together)
            static bool show_leading_button = true;
            static bool show_trailing_button = true;
            VanGui::Checkbox("Show Leading TabItemButton()", &show_leading_button);
            VanGui::Checkbox("Show Trailing TabItemButton()", &show_trailing_button);

            // Expose some other flags which are useful to showcase how they interact with Leading/Trailing tabs
            static VanGuiTabBarFlags tab_bar_flags = VanGuiTabBarFlags_AutoSelectNewTabs | VanGuiTabBarFlags_Reorderable | VanGuiTabBarFlags_FittingPolicyShrink;
            EditTabBarFittingPolicyFlags(&tab_bar_flags);

            if (VanGui::BeginTabBar("MyTabBar", tab_bar_flags))
            {
                // Demo a Leading TabItemButton(): click the "?" button to open a menu
                if (show_leading_button)
                    if (VanGui::TabItemButton("?", VanGuiTabItemFlags_Leading | VanGuiTabItemFlags_NoTooltip))
                        VanGui::OpenPopup("MyHelpMenu");
                if (VanGui::BeginPopup("MyHelpMenu"))
                {
                    VanGui::Selectable("Hello!");
                    VanGui::EndPopup();
                }

                // Demo Trailing Tabs: click the "+" button to add a new tab.
                // (In your app you may want to use a font icon instead of the "+")
                // We submit it before the regular tabs, but thanks to the VanGuiTabItemFlags_Trailing flag it will always appear at the end.
                if (show_trailing_button)
                    if (VanGui::TabItemButton("+", VanGuiTabItemFlags_Trailing | VanGuiTabItemFlags_NoTooltip))
                        active_tabs.push_back(next_tab_id++); // Add new tab

                // Submit our regular tabs
                for (int n = 0; n < active_tabs.Size; )
                {
                    bool open = true;
                    char name[16];
                    snprintf(name, VAN_COUNTOF(name), "%04d", active_tabs[n]);
                    if (VanGui::BeginTabItem(name, &open, VanGuiTabItemFlags_None))
                    {
                        VanGui::Text("This is the %s tab!", name);
                        VanGui::EndTabItem();
                    }

                    if (!open)
                        active_tabs.erase(active_tabs.Data + n);
                    else
                        n++;
                }

                VanGui::EndTabBar();
            }
            VanGui::Separator();
            VanGui::TreePop();
        }
        VanGui::TreePop();
    }
}

//-----------------------------------------------------------------------------
// [SECTION] DemoWindowWidgetsText()
//-----------------------------------------------------------------------------

static void DemoWindowWidgetsText()
{
    if (VanGui::TreeNode("Text"))
    {
        VANGUI_DEMO_MARKER("Widgets/Text");
        if (VanGui::TreeNode("Colorful Text"))
        {
            VANGUI_DEMO_MARKER("Widgets/Text/Colored Text");
            // Using shortcut. You can use PushStyleColor()/PopStyleColor() for more flexibility.
            VanGui::TextColored(VanVec4(1.0f, 0.0f, 1.0f, 1.0f), "Pink");
            VanGui::TextColored(VanVec4(1.0f, 1.0f, 0.0f, 1.0f), "Yellow");
            VanGui::TextDisabled("Disabled");
            VanGui::SameLine(); HelpMarker("The TextDisabled color is stored in VanGuiStyle.");
            VanGui::TreePop();
        }

        if (VanGui::TreeNode("Font Size"))
        {
            VANGUI_DEMO_MARKER("Widgets/Text/Font Size");
            VanGuiStyle& style = VanGui::GetStyle();
            const float global_scale = style.FontScaleMain * style.FontScaleDpi;
            VanGui::Text("style.FontScaleMain = %0.2f", style.FontScaleMain);
            VanGui::Text("style.FontScaleDpi = %0.2f", style.FontScaleDpi);
            VanGui::Text("global_scale = ~%0.2f", global_scale); // This is not technically accurate as internal scales may apply, but conceptually let's pretend it is.
            VanGui::Text("FontSize = %0.2f", VanGui::GetFontSize());

            VanGui::SeparatorText("");
            static float custom_size = 16.0f;
            VanGui::SliderFloat("custom_size", &custom_size, 10.0f, 100.0f, "%.0f");
            VanGui::Text("VanGui::PushFont(nullptr, custom_size);");
            VanGui::PushFont(nullptr, custom_size);
            VanGui::Text("FontSize = %.2f (== %.2f * global_scale)", VanGui::GetFontSize(), custom_size);
            VanGui::PopFont();

            VanGui::SeparatorText("");
            static float custom_scale = 1.0f;
            VanGui::SliderFloat("custom_scale", &custom_scale, 0.5f, 4.0f, "%.2f");
            VanGui::Text("VanGui::PushFont(nullptr, style.FontSizeBase * custom_scale);");
            VanGui::PushFont(nullptr, style.FontSizeBase * custom_scale);
            VanGui::Text("FontSize = %.2f (== style.FontSizeBase * %.2f * global_scale)", VanGui::GetFontSize(), custom_scale);
            VanGui::PopFont();

            VanGui::SeparatorText("");
            for (float scaling = 0.5f; scaling <= 4.0f; scaling += 0.5f)
            {
                VanGui::PushFont(nullptr, style.FontSizeBase * scaling);
                VanGui::Text("FontSize = %.2f (== style.FontSizeBase * %.2f * global_scale)", VanGui::GetFontSize(), scaling);
                VanGui::PopFont();
            }

            VanGui::TreePop();
        }

        if (VanGui::TreeNode("Word Wrapping"))
        {
            VANGUI_DEMO_MARKER("Widgets/Text/Word Wrapping");
            // Using shortcut. You can use PushTextWrapPos()/PopTextWrapPos() for more flexibility.
            VanGui::TextWrapped(
                "This text should automatically wrap on the edge of the window. The current implementation "
                "for text wrapping follows simple rules suitable for English and possibly other languages.");
            VanGui::Spacing();

            static float wrap_width = 200.0f;
            VanGui::SliderFloat("Wrap width", &wrap_width, -20, 600, "%.0f");

            VanDrawList* draw_list = VanGui::GetWindowDrawList();
            for (int n = 0; n < 2; n++)
            {
                VanGui::Text("Test paragraph %d:", n);
                VanVec2 pos = VanGui::GetCursorScreenPos();
                VanVec2 marker_min = VanVec2(pos.x + wrap_width, pos.y);
                VanVec2 marker_max = VanVec2(pos.x + wrap_width + 10, pos.y + VanGui::GetTextLineHeight());
                VanGui::PushTextWrapPos(VanGui::GetCursorPos().x + wrap_width);
                if (n == 0)
                    VanGui::Text("The lazy dog is a good dog. This paragraph should fit within %.0f pixels. Testing a 1 character word. The quick brown fox jumps over the lazy dog.", wrap_width);
                else
                    VanGui::Text("aaaaaaaa bbbbbbbb, c cccccccc,dddddddd. d eeeeeeee   ffffffff. gggggggg!hhhhhhhh");

                // Draw actual text bounding box, following by marker of our expected limit (should not overlap!)
                draw_list->AddRect(VanGui::GetItemRectMin(), VanGui::GetItemRectMax(), VAN_COL32(255, 255, 0, 255));
                draw_list->AddRectFilled(marker_min, marker_max, VAN_COL32(255, 0, 255, 255));
                VanGui::PopTextWrapPos();
            }

            VanGui::TreePop();
        }

        if (VanGui::TreeNode("UTF-8 Text"))
        {
            VANGUI_DEMO_MARKER("Widgets/Text/UTF-8 Text");
            // UTF-8 test with Japanese characters
            // (Needs a suitable font? Try "Google Noto" or "Arial Unicode". See docs/FONTS.md for details.)
            // - From C++11 you can use the u8"my text" syntax to encode literal strings as UTF-8
            // - For earlier compiler, you may be able to encode your sources as UTF-8 (e.g. in Visual Studio, you
            //   can save your source files as 'UTF-8 without signature').
            // - FOR THIS DEMO FILE ONLY, BECAUSE WE WANT TO SUPPORT OLD COMPILERS, WE ARE *NOT* INCLUDING RAW UTF-8
            //   CHARACTERS IN THIS SOURCE FILE. Instead we are encoding a few strings with hexadecimal constants.
            //   Don't do this in your application! Please use u8"text in any language" in your application!
            // Note that characters values are preserved even by InputText() if the font cannot be displayed,
            // so you can safely copy & paste garbled characters into another application.
            VanGui::TextWrapped(
                "CJK text will only appear if the font was loaded with the appropriate CJK character ranges. "
                "Call io.Fonts->AddFontFromFileTTF() manually to load extra character ranges. "
                "Read docs/FONTS.md for details.");
            VanGui::Text("Hiragana: \xe3\x81\x8b\xe3\x81\x8d\xe3\x81\x8f\xe3\x81\x91\xe3\x81\x93 (kakikukeko)");
            VanGui::Text("Kanjis: \xe6\x97\xa5\xe6\x9c\xac\xe8\xaa\x9e (nihongo)");
            static char buf[32] = "\xe6\x97\xa5\xe6\x9c\xac\xe8\xaa\x9e";
            //static char buf[32] = u8"NIHONGO"; // <- this is how you would write it with C++11, using real kanjis
            VanGui::InputText("UTF-8 input", buf, VAN_COUNTOF(buf));
            VanGui::TreePop();
        }
        VanGui::TreePop();
    }
}

//-----------------------------------------------------------------------------
// [SECTION] DemoWindowWidgetsTextFilter()
//-----------------------------------------------------------------------------

static void DemoWindowWidgetsTextFilter()
{
    if (VanGui::TreeNode("Text Filter"))
    {
        VANGUI_DEMO_MARKER("Widgets/Text Filter");
        // Helper class to easy setup a text filter.
        // You may want to implement a more feature-full filtering scheme in your own application.
        HelpMarker("Not a widget per-se, but VanGuiTextFilter is a helper to perform simple filtering on text strings.");
        static VanGuiTextFilter filter;
        VanGui::Text("Filter usage:\n"
            "  \"\"         display all lines\n"
            "  \"xxx\"      display lines containing \"xxx\"\n"
            "  \"xxx,yyy\"  display lines containing \"xxx\" or \"yyy\"\n"
            "  \"-xxx\"     hide lines containing \"xxx\"");
        filter.Draw();
        const char* lines[] = { "aaa1.c", "bbb1.c", "ccc1.c", "aaa2.cpp", "bbb2.cpp", "ccc2.cpp", "abc.h", "hello, world" };
        for (int i = 0; i < VAN_COUNTOF(lines); i++)
            if (filter.PassFilter(lines[i]))
                VanGui::BulletText("%s", lines[i]);
        VanGui::TreePop();
    }
}

//-----------------------------------------------------------------------------
// [SECTION] DemoWindowWidgetsTextInput()
//-----------------------------------------------------------------------------

static void DemoWindowWidgetsTextInput()
{
    // To wire InputText() with std::string or any other custom string type,
    // see the "Text Input > Resize Callback" section of this demo, and the misc/cpp/vangui_stdlib.h file.
    if (VanGui::TreeNode("Text Input"))
    {
        VANGUI_DEMO_MARKER("Widgets/Text Input");
        if (VanGui::TreeNode("Multi-line Text Input"))
        {
            VANGUI_DEMO_MARKER("Widgets/Text Input/Multi-line Text Input");
            // WE ARE USING A FIXED-SIZE BUFFER FOR SIMPLICITY HERE.
            // If you want to use InputText() with std::string or any custom dynamic string type:
            // - For std::string: use the wrapper in misc/cpp/vangui_stdlib.h/.cpp
            // - Otherwise, see the 'VanGUI Demo->Widgets->Text Input->Resize Callback' for using VanGuiInputTextFlags_CallbackResize.
            static char text[1024 * 16] =
                "/*\n"
                " The Pentium F00F bug, shorthand for F0 0F C7 C8,\n"
                " the hexadecimal encoding of one offending instruction,\n"
                " more formally, the invalid operand with locked CMPXCHG8B\n"
                " instruction bug, is a design flaw in the majority of\n"
                " Intel Pentium, Pentium MMX, and Pentium OverDrive\n"
                " processors (all in the P5 microarchitecture).\n"
                "*/\n\n"
                "label:\n"
                "\tlock cmpxchg8b eax\n";

            static VanGuiInputTextFlags flags = VanGuiInputTextFlags_AllowTabInput;
            HelpMarker("You can use the VanGuiInputTextFlags_CallbackResize facility if you need to wire InputTextMultiline() to a dynamic string type. See misc/cpp/vangui_stdlib.h for an example. (This is not demonstrated in vangui_demo.cpp because we don't want to include <string> in here)");
            VanGui::CheckboxFlags("VanGuiInputTextFlags_ReadOnly", &flags, VanGuiInputTextFlags_ReadOnly);
            VanGui::CheckboxFlags("VanGuiInputTextFlags_WordWrap", &flags, VanGuiInputTextFlags_WordWrap);
            VanGui::SameLine(); HelpMarker("Feature is currently in Beta. Please read comments in vangui.h");
            VanGui::CheckboxFlags("VanGuiInputTextFlags_AllowTabInput", &flags, VanGuiInputTextFlags_AllowTabInput);
            VanGui::SameLine(); HelpMarker("When _AllowTabInput is set, passing through the widget with Tabbing doesn't automatically activate it, in order to also cycling through subsequent widgets.");
            VanGui::CheckboxFlags("VanGuiInputTextFlags_CtrlEnterForNewLine", &flags, VanGuiInputTextFlags_CtrlEnterForNewLine);
            VanGui::InputTextMultiline("##source", text, VAN_COUNTOF(text), VanVec2(-FLT_MIN, VanGui::GetTextLineHeight() * 16), flags);
            VanGui::TreePop();
        }

        if (VanGui::TreeNode("Filtered Text Input"))
        {
            VANGUI_DEMO_MARKER("Widgets/Text Input/Filtered Text Input");
            struct TextFilters
            {
                // Modify character input by altering 'data->Eventchar' (VanGuiInputTextFlags_CallbackCharFilter callback)
                static int FilterCasingSwap(VanGuiInputTextCallbackData* data)
                {
                    if (data->EventChar >= 'a' && data->EventChar <= 'z') { data->EventChar -= 'a' - 'A'; } // Lowercase becomes uppercase
                    else if (data->EventChar >= 'A' && data->EventChar <= 'Z') { data->EventChar += 'a' - 'A'; } // Uppercase becomes lowercase
                    return 0;
                }

                // Return 0 (pass) if the character is 'i' or 'm' or 'g' or 'u' or 'i', otherwise return 1 (filter out)
                static int FilterVanGuiLetters(VanGuiInputTextCallbackData* data)
                {
                    if (data->EventChar < 256 && strchr("vangui", (char)data->EventChar))
                        return 0;
                    return 1;
                }
            };

            static char buf1[32] = ""; VanGui::InputText("default", buf1, VAN_COUNTOF(buf1));
            static char buf2[32] = ""; VanGui::InputText("decimal", buf2, VAN_COUNTOF(buf2), VanGuiInputTextFlags_CharsDecimal);
            static char buf3[32] = ""; VanGui::InputText("hexadecimal", buf3, VAN_COUNTOF(buf3), VanGuiInputTextFlags_CharsHexadecimal | VanGuiInputTextFlags_CharsUppercase);
            static char buf4[32] = ""; VanGui::InputText("uppercase", buf4, VAN_COUNTOF(buf4), VanGuiInputTextFlags_CharsUppercase);
            static char buf5[32] = ""; VanGui::InputText("no blank", buf5, VAN_COUNTOF(buf5), VanGuiInputTextFlags_CharsNoBlank);
            static char buf6[32] = ""; VanGui::InputText("casing swap", buf6, VAN_COUNTOF(buf6), VanGuiInputTextFlags_CallbackCharFilter, TextFilters::FilterCasingSwap); // Use CharFilter callback to replace characters.
            static char buf7[32] = ""; VanGui::InputText("\"vangui\"", buf7, VAN_COUNTOF(buf7), VanGuiInputTextFlags_CallbackCharFilter, TextFilters::FilterVanGuiLetters); // Use CharFilter callback to disable some characters.
            VanGui::TreePop();
        }

        if (VanGui::TreeNode("Password Input"))
        {
            VANGUI_DEMO_MARKER("Widgets/Text Input/Password input");
            static char password[64] = "password123";
            VanGui::InputText("password", password, VAN_COUNTOF(password), VanGuiInputTextFlags_Password);
            VanGui::SameLine(); HelpMarker("Display all characters as '*'.\nDisable clipboard cut and copy.\nDisable logging.\n");
            VanGui::InputTextWithHint("password (w/ hint)", "<password>", password, VAN_COUNTOF(password), VanGuiInputTextFlags_Password);
            VanGui::InputText("password (clear)", password, VAN_COUNTOF(password));
            VanGui::TreePop();
        }

        if (VanGui::TreeNode("Completion, History, Edit Callbacks"))
        {
            VANGUI_DEMO_MARKER("Widgets/Text Input/Completion, History, Edit Callbacks");
            struct Funcs
            {
                static int MyCallback(VanGuiInputTextCallbackData* data)
                {
                    if (data->EventFlag == VanGuiInputTextFlags_CallbackCompletion)
                    {
                        data->InsertChars(data->CursorPos, "..");
                    }
                    else if (data->EventFlag == VanGuiInputTextFlags_CallbackHistory)
                    {
                        if (data->EventKey == VanGuiKey_UpArrow)
                        {
                            data->DeleteChars(0, data->BufTextLen);
                            data->InsertChars(0, "Pressed Up!");
                            data->SelectAll();
                        }
                        else if (data->EventKey == VanGuiKey_DownArrow)
                        {
                            data->DeleteChars(0, data->BufTextLen);
                            data->InsertChars(0, "Pressed Down!");
                            data->SelectAll();
                        }
                    }
                    else if (data->EventFlag == VanGuiInputTextFlags_CallbackEdit)
                    {
                        // Toggle casing of first character
                        char c = data->Buf[0];
                        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) data->Buf[0] ^= 32;
                        data->BufDirty = true;

                        // Increment a counter
                        int* p_int = (int*)data->UserData;
                        *p_int = *p_int + 1;
                    }
                    return 0;
                }
            };
            static char buf1[64];
            VanGui::InputText("Completion", buf1, VAN_COUNTOF(buf1), VanGuiInputTextFlags_CallbackCompletion, Funcs::MyCallback);
            VanGui::SameLine(); HelpMarker(
                "Here we append \"..\" each time Tab is pressed. "
                "See 'Examples>Console' for a more meaningful demonstration of using this callback.");

            static char buf2[64];
            VanGui::InputText("History", buf2, VAN_COUNTOF(buf2), VanGuiInputTextFlags_CallbackHistory, Funcs::MyCallback);
            VanGui::SameLine(); HelpMarker(
                "Here we replace and select text each time Up/Down are pressed. "
                "See 'Examples>Console' for a more meaningful demonstration of using this callback.");

            static char buf3[64];
            static int edit_count = 0;
            VanGui::InputText("Edit", buf3, VAN_COUNTOF(buf3), VanGuiInputTextFlags_CallbackEdit, Funcs::MyCallback, static_cast<void*>(&edit_count));
            VanGui::SameLine(); HelpMarker(
                "Here we toggle the casing of the first character on every edit + count edits.");
            VanGui::SameLine(); VanGui::Text("(%d)", edit_count);

            VanGui::TreePop();
        }

        if (VanGui::TreeNode("Resize Callback"))
        {
            VANGUI_DEMO_MARKER("Widgets/Text Input/Resize Callback");
            // To wire InputText() with std::string or any other custom string type,
            // you can use the VanGuiInputTextFlags_CallbackResize flag + create a custom VanGui::InputText() wrapper
            // using your preferred type. See misc/cpp/vangui_stdlib.h for an implementation of this using std::string.
            HelpMarker(
                "Using VanGuiInputTextFlags_CallbackResize to wire your custom string type to InputText().\n\n"
                "See misc/cpp/vangui_stdlib.h for an implementation of this for std::string.");
            struct Funcs
            {
                static int MyResizeCallback(VanGuiInputTextCallbackData* data)
                {
                    if (data->EventFlag == VanGuiInputTextFlags_CallbackResize)
                    {
                        VanVector<char>* my_str = (VanVector<char>*)data->UserData;
                        VAN_ASSERT(my_str->begin() == data->Buf);
                        my_str->resize(data->BufSize); // NB: On resizing calls, generally data->BufSize == data->BufTextLen + 1
                        data->Buf = my_str->begin();
                    }
                    return 0;
                }

                // Note: Because VanGui:: is a namespace you would typically add your own function into the namespace.
                // For example, you code may declare a function 'VanGui::InputText(const char* label, MyString* my_str)'
                static bool MyInputTextMultiline(const char* label, VanVector<char>* my_str, const VanVec2& size = VanVec2(0, 0), VanGuiInputTextFlags flags = 0)
                {
                    VAN_ASSERT((flags & VanGuiInputTextFlags_CallbackResize) == 0);
                    return VanGui::InputTextMultiline(label, my_str->begin(), static_cast<size_t>(my_str->size()), size, flags | VanGuiInputTextFlags_CallbackResize, Funcs::MyResizeCallback, static_cast<void*>(my_str));
                }
            };

            // For this demo we are using VanVector as a string container.
            // Note that because we need to store a terminating zero character, our size/capacity are 1 more
            // than usually reported by a typical string class.
            static VanGuiInputTextFlags flags = VanGuiInputTextFlags_None;
            VanGui::CheckboxFlags("VanGuiInputTextFlags_WordWrap", &flags, VanGuiInputTextFlags_WordWrap);

            static VanVector<char> my_str;
            if (my_str.empty())
                my_str.push_back(0);
            Funcs::MyInputTextMultiline("##MyStr", &my_str, VanVec2(-FLT_MIN, VanGui::GetTextLineHeight() * 16), flags);
            VanGui::Text("Data: %p\nSize: %d\nCapacity: %d", static_cast<void*>(my_str.begin()), my_str.size(), my_str.capacity());
            VanGui::TreePop();
        }

        if (VanGui::TreeNode("Eliding, Alignment"))
        {
            VANGUI_DEMO_MARKER("Widgets/Text Input/Eliding, Alignment");
            static char buf1[128] = "/path/to/some/folder/with/long/filename.cpp";
            static VanGuiInputTextFlags flags = VanGuiInputTextFlags_ElideLeft;
            VanGui::CheckboxFlags("VanGuiInputTextFlags_ElideLeft", &flags, VanGuiInputTextFlags_ElideLeft);
            VanGui::InputText("Path", buf1, VAN_COUNTOF(buf1), flags);
            VanGui::TreePop();
        }

        if (VanGui::TreeNode("Miscellaneous"))
        {
            VANGUI_DEMO_MARKER("Widgets/Text Input/Miscellaneous");
            static char buf1[16];
            static VanGuiInputTextFlags flags = VanGuiInputTextFlags_EscapeClearsAll;
            VanGui::CheckboxFlags("VanGuiInputTextFlags_EscapeClearsAll", &flags, VanGuiInputTextFlags_EscapeClearsAll);
            VanGui::CheckboxFlags("VanGuiInputTextFlags_ReadOnly", &flags, VanGuiInputTextFlags_ReadOnly);
            VanGui::CheckboxFlags("VanGuiInputTextFlags_NoUndoRedo", &flags, VanGuiInputTextFlags_NoUndoRedo);
            VanGui::InputText("Hello", buf1, VAN_COUNTOF(buf1), flags);
            VanGui::TreePop();
        }

        VanGui::TreePop();
    }

}

//-----------------------------------------------------------------------------
// [SECTION] DemoWindowWidgetsTooltips()
//-----------------------------------------------------------------------------

static void DemoWindowWidgetsTooltips()
{
    if (VanGui::TreeNode("Tooltips"))
    {
        VANGUI_DEMO_MARKER("Widgets/Tooltips");
        // Tooltips are windows following the mouse. They do not take focus away.
        VanGui::SeparatorText("General");

        // Typical use cases:
        // - Short-form (text only):      SetItemTooltip("Hello");
        // - Short-form (any contents):   if (BeginItemTooltip()) { Text("Hello"); EndTooltip(); }

        // - Full-form (text only):       if (IsItemHovered(...)) { SetTooltip("Hello"); }
        // - Full-form (any contents):    if (IsItemHovered(...) && BeginTooltip()) { Text("Hello"); EndTooltip(); }

        HelpMarker(
            "Tooltip are typically created by using a IsItemHovered() + SetTooltip() sequence.\n\n"
            "We provide a helper SetItemTooltip() function to perform the two with standards flags.");

        VanVec2 sz = VanVec2(-FLT_MIN, 0.0f);

        VanGui::Button("Basic", sz);
        VanGui::SetItemTooltip("I am a tooltip");

        VanGui::Button("Fancy", sz);
        if (VanGui::BeginItemTooltip())
        {
            VanGui::Text("I am a fancy tooltip");
            static float arr[] = { 0.6f, 0.1f, 1.0f, 0.5f, 0.92f, 0.1f, 0.2f };
            VanGui::PlotLines("Curve", arr, VAN_COUNTOF(arr));
            VanGui::Text("Sin(time) = %f", sinf(static_cast<float>(VanGui::GetTime())));
            VanGui::EndTooltip();
        }

        VanGui::SeparatorText("Always On");

        // Showcase NOT relying on a IsItemHovered() to emit a tooltip.
        // Here the tooltip is always emitted when 'always_on == true'.
        static int always_on = 0;
        VanGui::RadioButton("Off", &always_on, 0);
        VanGui::SameLine();
        VanGui::RadioButton("Always On (Simple)", &always_on, 1);
        VanGui::SameLine();
        VanGui::RadioButton("Always On (Advanced)", &always_on, 2);
        if (always_on == 1)
            VanGui::SetTooltip("I am following you around.");
        else if (always_on == 2 && VanGui::BeginTooltip())
        {
            VanGui::ProgressBar(sinf(static_cast<float>(VanGui::GetTime())) * 0.5f + 0.5f, VanVec2(VanGui::GetFontSize() * 25, 0.0f));
            VanGui::EndTooltip();
        }

        VanGui::SeparatorText("Custom");

        HelpMarker(
            "Passing VanGuiHoveredFlags_ForTooltip to IsItemHovered() is the preferred way to standardize "
            "tooltip activation details across your application. You may however decide to use custom "
            "flags for a specific tooltip instance.");

        // The following examples are passed for documentation purpose but may not be useful to most users.
        // Passing VanGuiHoveredFlags_ForTooltip to IsItemHovered() will pull VanGuiHoveredFlags flags values from
        // 'style.HoverFlagsForTooltipMouse' or 'style.HoverFlagsForTooltipNav' depending on whether mouse or keyboard/gamepad is being used.
        // With default settings, VanGuiHoveredFlags_ForTooltip is equivalent to VanGuiHoveredFlags_DelayShort + VanGuiHoveredFlags_Stationary.
        VanGui::Button("Manual", sz);
        if (VanGui::IsItemHovered(VanGuiHoveredFlags_ForTooltip))
            VanGui::SetTooltip("I am a manually emitted tooltip.");

        VanGui::Button("DelayNone", sz);
        if (VanGui::IsItemHovered(VanGuiHoveredFlags_DelayNone))
            VanGui::SetTooltip("I am a tooltip with no delay.");

        VanGui::Button("DelayShort", sz);
        if (VanGui::IsItemHovered(VanGuiHoveredFlags_DelayShort | VanGuiHoveredFlags_NoSharedDelay))
            VanGui::SetTooltip("I am a tooltip with a short delay (%0.2f sec).", VanGui::GetStyle().HoverDelayShort);

        VanGui::Button("DelayLong", sz);
        if (VanGui::IsItemHovered(VanGuiHoveredFlags_DelayNormal | VanGuiHoveredFlags_NoSharedDelay))
            VanGui::SetTooltip("I am a tooltip with a long delay (%0.2f sec).", VanGui::GetStyle().HoverDelayNormal);

        VanGui::Button("Stationary", sz);
        if (VanGui::IsItemHovered(VanGuiHoveredFlags_Stationary))
            VanGui::SetTooltip("I am a tooltip requiring mouse to be stationary before activating.");

        // Using VanGuiHoveredFlags_ForTooltip will pull flags from 'style.HoverFlagsForTooltipMouse' or 'style.HoverFlagsForTooltipNav',
        // which default value include the VanGuiHoveredFlags_AllowWhenDisabled flag.
        VanGui::BeginDisabled();
        VanGui::Button("Disabled item", sz);
        if (VanGui::IsItemHovered(VanGuiHoveredFlags_ForTooltip))
            VanGui::SetTooltip("I am a a tooltip for a disabled item.");
        VanGui::EndDisabled();

        VanGui::TreePop();
    }
}

//-----------------------------------------------------------------------------
// [SECTION] DemoWindowWidgetsTreeNodes()
//-----------------------------------------------------------------------------

static void DemoWindowWidgetsTreeNodes()
{
    if (VanGui::TreeNode("Tree Nodes"))
    {
        VANGUI_DEMO_MARKER("Widgets/Tree Nodes");
        // See see "Examples -> Property Editor" (ShowExampleAppPropertyEditor() function) for a fancier, data-driven tree.
        if (VanGui::TreeNode("Basic Trees"))
        {
            VANGUI_DEMO_MARKER("Widgets/Tree Nodes/Basic Trees");
            for (int i = 0; i < 5; i++)
            {
                // Use SetNextItemOpen() so set the default state of a node to be open. We could
                // also use TreeNodeEx() with the VanGuiTreeNodeFlags_DefaultOpen flag to achieve the same thing!
                if (i == 0)
                    VanGui::SetNextItemOpen(true, VanGuiCond_Once);

                // Here we use PushID() to generate a unique base ID, and then the "" used as TreeNode id won't conflict.
                // An alternative to using 'PushID() + TreeNode("", ...)' to generate a unique ID is to use 'TreeNode((void*)(intptr_t)i, ...)',
                // aka generate a dummy pointer-sized value to be hashed. The demo below uses that technique. Both are fine.
                VanGui::PushID(i);
                if (VanGui::TreeNode("", "Child %d", i))
                {
                    VanGui::Text("blah blah");
                    VanGui::SameLine();
                    if (VanGui::SmallButton("button")) {}
                    VanGui::TreePop();
                }
                VanGui::PopID();
            }
            VanGui::TreePop();
        }

        if (VanGui::TreeNode("Hierarchy Lines"))
        {
            VANGUI_DEMO_MARKER("Widgets/Tree Nodes/Hierarchy Lines");
            static VanGuiTreeNodeFlags base_flags = VanGuiTreeNodeFlags_DrawLinesFull | VanGuiTreeNodeFlags_DefaultOpen;
            HelpMarker("Default option for DrawLinesXXX is stored in style.TreeLinesFlags");
            VanGui::CheckboxFlags("VanGuiTreeNodeFlags_DrawLinesNone", &base_flags, VanGuiTreeNodeFlags_DrawLinesNone);
            VanGui::CheckboxFlags("VanGuiTreeNodeFlags_DrawLinesFull", &base_flags, VanGuiTreeNodeFlags_DrawLinesFull);
            VanGui::CheckboxFlags("VanGuiTreeNodeFlags_DrawLinesToNodes", &base_flags, VanGuiTreeNodeFlags_DrawLinesToNodes);

            if (VanGui::TreeNodeEx("Parent", base_flags))
            {
                if (VanGui::TreeNodeEx("Child 1", base_flags))
                {
                    VanGui::Button("Button for Child 1");
                    VanGui::TreePop();
                }
                if (VanGui::TreeNodeEx("Child 2", base_flags))
                {
                    VanGui::Button("Button for Child 2");
                    VanGui::TreePop();
                }
                VanGui::Text("Remaining contents");
                VanGui::Text("Remaining contents");
                VanGui::TreePop();
            }

            VanGui::TreePop();
        }

        if (VanGui::TreeNode("Clipping Large Trees"))
        {
            VANGUI_DEMO_MARKER("Widgets/Tree Nodes/Clipping Large Trees");
            VanGui::TextWrapped(
                "- Using VanGuiListClipper with trees is a less easy than on arrays or grids.\n"
                "- Refer to 'Demo->Examples->Property Editor' for an example of how to do that.\n"
                "- Discuss in #3823");
            VanGui::TreePop();
        }

        if (VanGui::TreeNode("Selectable Nodes"))
        {
            VANGUI_DEMO_MARKER("Widgets/Tree Nodes/Selectable Nodes");
            HelpMarker(
                "Manually implemented selectable nodes.\n"
                "Click to select, Ctrl+Click to toggle, click on arrows or double-click to open.\n\n"
                "You may also use the multi-select API (see 'Demo->Widgets->Selection State & Multi-Select') for more advanced multi-selection features.");

            // Hold in 'selection_mask' a simple representation of what may be user-side selection state.
            // - You may retain selection state inside or outside your objects in whatever format you see fit.
            //   You may use VanGuiSelectionBasicStorage which is conceptually close to a set<> of identifiers.
            // - We record which node was clicked and then apply selection at the end of the loop.
            // - This is a manual and simplified reimplementation of multi-selection, which the full
            //   BeginMultiSelect() API implements better, but which is not trivial to wire for trees.
            static int selection_mask = 0x00;
            int node_clicked_idx = -1;
            for (int node_n = 0; node_n < 6; node_n++)
            {
                // Disable the default "open on single-click behavior" + set Selected flag according to our selection.
                // To alter selection we use if 'IsItemClicked() && !IsItemToggledOpen()', so clicking on an arrow doesn't alter selection.
                // In a BeginMultiSelect()/EndMultiSelect() we could use IsItemToggledSelection() but here we reimplement and use our own logic.
                VanGuiTreeNodeFlags flags = VanGuiTreeNodeFlags_OpenOnArrow | VanGuiTreeNodeFlags_OpenOnDoubleClick | VanGuiTreeNodeFlags_SpanAvailWidth;
                if (selection_mask & (1 << node_n))
                    flags |= VanGuiTreeNodeFlags_Selected;

                bool is_open = VanGui::TreeNodeEx(reinterpret_cast<void*>(static_cast<intptr_t>(node_n)), flags, "Selectable Node %d", node_n);
                if (VanGui::IsItemClicked() && !VanGui::IsItemToggledOpen())
                    node_clicked_idx = node_n;
                if (is_open)
                {
                    VanGui::BulletText("<Node contents here>");
                    VanGui::TreePop();
                }
            }
            if (node_clicked_idx != -1)
            {
                // Update selection state (process outside of tree loop to avoid visual inconsistencies during the clicking frame)
                if (VanGui::GetIO().KeyCtrl)
                    selection_mask ^= (1 << node_clicked_idx);          // Ctrl+Click to toggle
                else //if (!(selection_mask & (1 << node_clicked_idx))) // Depending on selection behavior you want, may want to preserve selection when clicking on item that is part of the selection
                    selection_mask = (1 << node_clicked_idx);           // Click to single-select
            }
            VanGui::TreePop();
        }

        if (VanGui::TreeNode("Advanced"))
        {
            VANGUI_DEMO_MARKER("Widgets/Tree Nodes/Advanced");
            static VanGuiTreeNodeFlags base_flags = VanGuiTreeNodeFlags_OpenOnArrow | VanGuiTreeNodeFlags_OpenOnDoubleClick | VanGuiTreeNodeFlags_SpanAvailWidth;
            static bool align_label_with_current_x_position = false;
            static bool use_drag_and_drop = false;
            VanGui::CheckboxFlags("VanGuiTreeNodeFlags_OpenOnArrow", &base_flags, VanGuiTreeNodeFlags_OpenOnArrow);
            VanGui::CheckboxFlags("VanGuiTreeNodeFlags_OpenOnDoubleClick", &base_flags, VanGuiTreeNodeFlags_OpenOnDoubleClick);
            VanGui::CheckboxFlags("VanGuiTreeNodeFlags_SpanAvailWidth", &base_flags, VanGuiTreeNodeFlags_SpanAvailWidth); VanGui::SameLine(); HelpMarker("Extend hit area to all available width instead of allowing more items to be laid out after the node.");
            VanGui::CheckboxFlags("VanGuiTreeNodeFlags_SpanFullWidth", &base_flags, VanGuiTreeNodeFlags_SpanFullWidth);
            VanGui::CheckboxFlags("VanGuiTreeNodeFlags_SpanLabelWidth", &base_flags, VanGuiTreeNodeFlags_SpanLabelWidth); VanGui::SameLine(); HelpMarker("Reduce hit area to the text label and a bit of margin.");
            VanGui::CheckboxFlags("VanGuiTreeNodeFlags_SpanAllColumns", &base_flags, VanGuiTreeNodeFlags_SpanAllColumns); VanGui::SameLine(); HelpMarker("For use in Tables only.");
            VanGui::CheckboxFlags("VanGuiTreeNodeFlags_AllowOverlap", &base_flags, VanGuiTreeNodeFlags_AllowOverlap);
            VanGui::CheckboxFlags("VanGuiTreeNodeFlags_Framed", &base_flags, VanGuiTreeNodeFlags_Framed); VanGui::SameLine(); HelpMarker("Draw frame with background (e.g. for CollapsingHeader)");
            VanGui::CheckboxFlags("VanGuiTreeNodeFlags_FramePadding", &base_flags, VanGuiTreeNodeFlags_FramePadding);
            VanGui::CheckboxFlags("VanGuiTreeNodeFlags_NavLeftJumpsToParent", &base_flags, VanGuiTreeNodeFlags_NavLeftJumpsToParent);

            HelpMarker("Default option for DrawLinesXXX is stored in style.TreeLinesFlags");
            VanGui::CheckboxFlags("VanGuiTreeNodeFlags_DrawLinesNone", &base_flags, VanGuiTreeNodeFlags_DrawLinesNone);
            VanGui::CheckboxFlags("VanGuiTreeNodeFlags_DrawLinesFull", &base_flags, VanGuiTreeNodeFlags_DrawLinesFull);
            VanGui::CheckboxFlags("VanGuiTreeNodeFlags_DrawLinesToNodes", &base_flags, VanGuiTreeNodeFlags_DrawLinesToNodes);

            VanGui::Checkbox("Align label with current X position", &align_label_with_current_x_position);
            VanGui::Checkbox("Make Tree Nodes as drag & drop sources", &use_drag_and_drop);
            if (align_label_with_current_x_position)
                VanGui::Unindent(VanGui::GetTreeNodeToLabelSpacing());

            for (int node_n = 0; node_n < 6; node_n++)
            {
                VanGuiTreeNodeFlags node_flags = base_flags;
                if (node_n < 3)
                {
                    // Items 0..2 are Tree Node
                    bool is_open = VanGui::TreeNodeEx(reinterpret_cast<void*>(static_cast<intptr_t>(node_n)), node_flags, "Selectable Node %d", node_n);
                    if (use_drag_and_drop && VanGui::BeginDragDropSource())
                    {
                        VanGui::SetDragDropPayload("MY_TREENODE_PAYLOAD_TYPE", nullptr, 0);
                        VanGui::Text("This is a drag and drop source");
                        VanGui::EndDragDropSource();
                    }
                    if (node_n == 2 && (base_flags & VanGuiTreeNodeFlags_SpanLabelWidth))
                    {
                        // Item 2 has an additional inline button to help demonstrate SpanLabelWidth.
                        VanGui::SameLine();
                        if (VanGui::SmallButton("button")) {}
                    }
                    if (is_open)
                    {
                        VanGui::BulletText("Blah blah\nBlah Blah");
                        VanGui::SameLine();
                        VanGui::SmallButton("Button");
                        VanGui::TreePop();
                    }
                }
                else
                {
                    // Items 3..5 are Tree Leaves
                    // The only reason we use TreeNode at all is to allow selection of the leaf. Otherwise we can
                    // use BulletText() or advance the cursor by GetTreeNodeToLabelSpacing() and call Text().
                    node_flags |= VanGuiTreeNodeFlags_Leaf | VanGuiTreeNodeFlags_NoTreePushOnOpen; // VanGuiTreeNodeFlags_Bullet
                    VanGui::TreeNodeEx(reinterpret_cast<void*>(static_cast<intptr_t>(node_n)), node_flags, "Selectable Leaf %d", node_n);
                    if (use_drag_and_drop && VanGui::BeginDragDropSource())
                    {
                        VanGui::SetDragDropPayload("MY_TREENODE_PAYLOAD_TYPE", nullptr, 0);
                        VanGui::Text("This is a drag and drop source");
                        VanGui::EndDragDropSource();
                    }
                }
            }
            if (align_label_with_current_x_position)
                VanGui::Indent(VanGui::GetTreeNodeToLabelSpacing());
            VanGui::TreePop();
        }
        VanGui::TreePop();
    }
}

//-----------------------------------------------------------------------------
// [SECTION] DemoWindowWidgetsVerticalSliders()
//-----------------------------------------------------------------------------

static void DemoWindowWidgetsVerticalSliders()
{
    if (VanGui::TreeNode("Vertical Sliders"))
    {
        VANGUI_DEMO_MARKER("Widgets/Vertical Sliders");
        const float spacing = 4;
        VanGui::PushStyleVar(VanGuiStyleVar_ItemSpacing, VanVec2(spacing, spacing));

        static int int_value = 0;
        VanGui::VSliderInt("##int", VanVec2(18, 160), &int_value, 0, 5);
        VanGui::SameLine();

        static float values[7] = { 0.0f, 0.60f, 0.35f, 0.9f, 0.70f, 0.20f, 0.0f };
        VanGui::PushID("set1");
        for (int i = 0; i < 7; i++)
        {
            if (i > 0) VanGui::SameLine();
            VanGui::PushID(i);
            VanGui::PushStyleColor(VanGuiCol_FrameBg, static_cast<VanVec4>(VanColor::HSV(i / 7.0f, 0.5f, 0.5f)));
            VanGui::PushStyleColor(VanGuiCol_FrameBgHovered, static_cast<VanVec4>(VanColor::HSV(i / 7.0f, 0.6f, 0.5f)));
            VanGui::PushStyleColor(VanGuiCol_FrameBgActive, static_cast<VanVec4>(VanColor::HSV(i / 7.0f, 0.7f, 0.5f)));
            VanGui::PushStyleColor(VanGuiCol_SliderGrab, static_cast<VanVec4>(VanColor::HSV(i / 7.0f, 0.9f, 0.9f)));
            VanGui::VSliderFloat("##v", VanVec2(18, 160), &values[i], 0.0f, 1.0f, "");
            if (VanGui::IsItemActive() || VanGui::IsItemHovered())
                VanGui::SetTooltip("%.3f", values[i]);
            VanGui::PopStyleColor(4);
            VanGui::PopID();
        }
        VanGui::PopID();

        VanGui::SameLine();
        VanGui::PushID("set2");
        static float values2[4] = { 0.20f, 0.80f, 0.40f, 0.25f };
        const int rows = 3;
        const VanVec2 small_slider_size(18, static_cast<float>(static_cast<int>((160.0f - (rows - 1) * spacing) / rows)));
        for (int nx = 0; nx < 4; nx++)
        {
            if (nx > 0) VanGui::SameLine();
            VanGui::BeginGroup();
            for (int ny = 0; ny < rows; ny++)
            {
                VanGui::PushID(nx * rows + ny);
                VanGui::VSliderFloat("##v", small_slider_size, &values2[nx], 0.0f, 1.0f, "");
                if (VanGui::IsItemActive() || VanGui::IsItemHovered())
                    VanGui::SetTooltip("%.3f", values2[nx]);
                VanGui::PopID();
            }
            VanGui::EndGroup();
        }
        VanGui::PopID();

        VanGui::SameLine();
        VanGui::PushID("set3");
        for (int i = 0; i < 4; i++)
        {
            if (i > 0) VanGui::SameLine();
            VanGui::PushID(i);
            VanGui::PushStyleVar(VanGuiStyleVar_GrabMinSize, 40);
            VanGui::VSliderFloat("##v", VanVec2(40, 160), &values[i], 0.0f, 1.0f, "%.2f\nsec");
            VanGui::PopStyleVar();
            VanGui::PopID();
        }
        VanGui::PopID();
        VanGui::PopStyleVar();
        VanGui::TreePop();
    }
}

//-----------------------------------------------------------------------------
// [SECTION] DemoWindowWidgets()
//-----------------------------------------------------------------------------

static void DemoWindowWidgets(VanGuiDemoWindowData* demo_data)
{
    //VanGui::SetNextItemOpen(true, VanGuiCond_Once);
    if (!VanGui::CollapsingHeader("Widgets"))
        return;
    // VANGUI_DEMO_MARKER("Widgets");

    const bool disable_all = demo_data->DisableSections; // The Checkbox for that is inside the "Disabled" section at the bottom
    if (disable_all)
        VanGui::BeginDisabled();

    DemoWindowWidgetsBasic();
    DemoWindowWidgetsBullets();
    DemoWindowWidgetsCollapsingHeaders();
    DemoWindowWidgetsComboBoxes();
    DemoWindowWidgetsColorAndPickers();
    DemoWindowWidgetsDataTypes();

    if (disable_all)
        VanGui::EndDisabled();
    DemoWindowWidgetsDisableBlocks(demo_data);
    if (disable_all)
        VanGui::BeginDisabled();

    DemoWindowWidgetsDragAndDrop();
    DemoWindowWidgetsDragsAndSliders();
    DemoWindowWidgetsFonts();
    DemoWindowWidgetsImages();
    DemoWindowWidgetsListBoxes();
    DemoWindowWidgetsMultiComponents();
    DemoWindowWidgetsPlotting();
    DemoWindowWidgetsProgressBars();
    DemoWindowWidgetsQueryingStatuses();
    DemoWindowWidgetsSelectables();
    DemoWindowWidgetsSelectionAndMultiSelect(demo_data);
    DemoWindowWidgetsTabs();
    DemoWindowWidgetsText();
    DemoWindowWidgetsTextFilter();
    DemoWindowWidgetsTextInput();
    DemoWindowWidgetsTooltips();
    DemoWindowWidgetsTreeNodes();
    DemoWindowWidgetsVerticalSliders();

    if (disable_all)
        VanGui::EndDisabled();
}

//-----------------------------------------------------------------------------
// [SECTION] DemoWindowLayout()
//-----------------------------------------------------------------------------

static void DemoWindowLayout()
{
    if (!VanGui::CollapsingHeader("Layout & Scrolling"))
        return;

    if (VanGui::TreeNode("Child windows"))
    {
        VANGUI_DEMO_MARKER("Layout/Child windows");
        VanGui::SeparatorText("Child windows");

        HelpMarker("Use child windows to begin into a self-contained independent scrolling/clipping regions within a host window.");
        static bool disable_mouse_wheel = false;
        static bool disable_menu = false;
        VanGui::Checkbox("Disable Mouse Wheel", &disable_mouse_wheel);
        VanGui::Checkbox("Disable Menu", &disable_menu);

        // Child 1: no border, enable horizontal scrollbar
        {
            VanGuiWindowFlags window_flags = VanGuiWindowFlags_HorizontalScrollbar;
            if (disable_mouse_wheel)
                window_flags |= VanGuiWindowFlags_NoScrollWithMouse;
            VanGui::BeginChild("ChildL", VanVec2(VanGui::GetContentRegionAvail().x * 0.5f, 260), VanGuiChildFlags_None, window_flags);
            for (int i = 0; i < 100; i++)
                VanGui::Text("%04d: scrollable region", i);
            VanGui::EndChild();
        }

        VanGui::SameLine();

        // Child 2: rounded border
        {
            VanGuiWindowFlags window_flags = VanGuiWindowFlags_None;
            if (disable_mouse_wheel)
                window_flags |= VanGuiWindowFlags_NoScrollWithMouse;
            if (!disable_menu)
                window_flags |= VanGuiWindowFlags_MenuBar;
            VanGui::PushStyleVar(VanGuiStyleVar_ChildRounding, 5.0f);
            VanGui::BeginChild("ChildR", VanVec2(0, 260), VanGuiChildFlags_Borders, window_flags);
            if (!disable_menu && VanGui::BeginMenuBar())
            {
                if (VanGui::BeginMenu("Menu"))
                {
                    ShowExampleMenuFile();
                    VanGui::EndMenu();
                }
                VanGui::EndMenuBar();
            }
            if (VanGui::BeginTable("split", 2, VanGuiTableFlags_Resizable | VanGuiTableFlags_NoSavedSettings))
            {
                for (int i = 0; i < 100; i++)
                {
                    char buf[32];
                    snprintf(buf, sizeof(buf), "%03d", i);
                    VanGui::TableNextColumn();
                    VanGui::Button(buf, VanVec2(-FLT_MIN, 0.0f));
                }
                VanGui::EndTable();
            }
            VanGui::EndChild();
            VanGui::PopStyleVar();
        }

        // Child 3: manual-resize
        VanGui::SeparatorText("Manual-resize");
        {
            HelpMarker("Drag bottom border to resize. Double-click bottom border to auto-fit to vertical contents.");
            //if (VanGui::Button("Set Height to 200"))
            //    VanGui::SetNextWindowSize(VanVec2(-FLT_MIN, 200.0f));

            VanGui::PushStyleColor(VanGuiCol_ChildBg, VanGui::GetStyleColorVec4(VanGuiCol_FrameBg));
            if (VanGui::BeginChild("ResizableChild", VanVec2(-FLT_MIN, VanGui::GetTextLineHeightWithSpacing() * 8), VanGuiChildFlags_Borders | VanGuiChildFlags_ResizeY))
                for (int n = 0; n < 10; n++)
                    VanGui::Text("Line %04d", n);
            VanGui::PopStyleColor();
            VanGui::EndChild();
        }

        // Child 4: auto-resizing height with a limit
        VanGui::SeparatorText("Auto-resize with constraints");
        {
            static int draw_lines = 3;
            static int max_height_in_lines = 10;
            VanGui::SetNextItemWidth(VanGui::GetFontSize() * 8);
            VanGui::DragInt("Lines Count", &draw_lines, 0.2f);
            VanGui::SetNextItemWidth(VanGui::GetFontSize() * 8);
            VanGui::DragInt("Max Height (in Lines)", &max_height_in_lines, 0.2f);

            VanGui::SetNextWindowSizeConstraints(VanVec2(0.0f, VanGui::GetTextLineHeightWithSpacing() * 1), VanVec2(FLT_MAX, VanGui::GetTextLineHeightWithSpacing() * max_height_in_lines));
            if (VanGui::BeginChild("ConstrainedChild", VanVec2(-FLT_MIN, 0.0f), VanGuiChildFlags_Borders | VanGuiChildFlags_AutoResizeY))
                for (int n = 0; n < draw_lines; n++)
                    VanGui::Text("Line %04d", n);
            VanGui::EndChild();
        }

        VanGui::SeparatorText("Misc/Advanced");

        // Demonstrate a few extra things
        // - Changing VanGuiCol_ChildBg (which is transparent black in default styles)
        // - Using SetCursorPos() to position child window (the child window is an item from the POV of parent window)
        //   You can also call SetNextWindowPos() to position the child window. The parent window will effectively
        //   layout from this position.
        // - Using VanGui::GetItemRectMin/Max() to query the "item" state (because the child window is an item from
        //   the POV of the parent window). See 'Demo->Querying Status (Edited/Active/Hovered etc.)' for details.
        {
            static int offset_x = 0;
            static bool override_bg_color = true;
            static VanGuiChildFlags child_flags = VanGuiChildFlags_Borders | VanGuiChildFlags_ResizeX | VanGuiChildFlags_ResizeY;
            VanGui::SetNextItemWidth(VanGui::GetFontSize() * 8);
            VanGui::DragInt("Offset X", &offset_x, 1.0f, -1000, 1000);
            VanGui::Checkbox("Override ChildBg color", &override_bg_color);
            VanGui::CheckboxFlags("VanGuiChildFlags_Borders", &child_flags, VanGuiChildFlags_Borders);
            VanGui::CheckboxFlags("VanGuiChildFlags_AlwaysUseWindowPadding", &child_flags, VanGuiChildFlags_AlwaysUseWindowPadding);
            VanGui::CheckboxFlags("VanGuiChildFlags_ResizeX", &child_flags, VanGuiChildFlags_ResizeX);
            VanGui::CheckboxFlags("VanGuiChildFlags_ResizeY", &child_flags, VanGuiChildFlags_ResizeY);
            VanGui::CheckboxFlags("VanGuiChildFlags_FrameStyle", &child_flags, VanGuiChildFlags_FrameStyle);
            VanGui::SameLine(); HelpMarker("Style the child window like a framed item: use FrameBg, FrameRounding, FrameBorderSize, FramePadding instead of ChildBg, ChildRounding, ChildBorderSize, WindowPadding.");
            if (child_flags & VanGuiChildFlags_FrameStyle)
                override_bg_color = false;

            VanGui::SetCursorPosX(VanGui::GetCursorPosX() + static_cast<float>(offset_x));
            if (override_bg_color)
                VanGui::PushStyleColor(VanGuiCol_ChildBg, VAN_COL32(255, 0, 0, 100));
            VanGui::BeginChild("Red", VanVec2(200, 100), child_flags, VanGuiWindowFlags_None);
            if (override_bg_color)
                VanGui::PopStyleColor();

            for (int n = 0; n < 50; n++)
                VanGui::Text("Some test %d", n);
            VanGui::EndChild();
            bool child_is_hovered = VanGui::IsItemHovered();
            VanVec2 child_rect_min = VanGui::GetItemRectMin();
            VanVec2 child_rect_max = VanGui::GetItemRectMax();
            VanGui::Text("Hovered: %d", child_is_hovered);
            VanGui::Text("Rect of child window is: (%.0f,%.0f) (%.0f,%.0f)", child_rect_min.x, child_rect_min.y, child_rect_max.x, child_rect_max.y);
        }

        VanGui::TreePop();
    }

    if (VanGui::TreeNode("Widgets Width"))
    {
        VANGUI_DEMO_MARKER("Layout/Widgets Width");
        static float f = 0.0f;
        static bool show_indented_items = true;
        VanGui::Checkbox("Show indented items", &show_indented_items);

        // Use SetNextItemWidth() to set the width of a single upcoming item.
        // Use PushItemWidth()/PopItemWidth() to set the width of a group of items.
        // In real code use you'll probably want to choose width values that are proportional to your font size
        // e.g. Using '20.0f * GetFontSize()' as width instead of '200.0f', etc.

        VanGui::Text("SetNextItemWidth/PushItemWidth(100)");
        VanGui::SameLine(); HelpMarker("Fixed width.");
        VanGui::PushItemWidth(100);
        VanGui::DragFloat("float##1b", &f);
        if (show_indented_items)
        {
            VanGui::Indent();
            VanGui::DragFloat("float (indented)##1b", &f);
            VanGui::Unindent();
        }
        VanGui::PopItemWidth();

        VanGui::Text("SetNextItemWidth/PushItemWidth(-100)");
        VanGui::SameLine(); HelpMarker("Align to right edge minus 100");
        VanGui::PushItemWidth(-100);
        VanGui::DragFloat("float##2a", &f);
        if (show_indented_items)
        {
            VanGui::Indent();
            VanGui::DragFloat("float (indented)##2b", &f);
            VanGui::Unindent();
        }
        VanGui::PopItemWidth();

        VanGui::Text("SetNextItemWidth/PushItemWidth(GetContentRegionAvail().x * 0.5f)");
        VanGui::SameLine(); HelpMarker("Half of available width.\n(~ right-cursor_pos)\n(works within a column set)");
        VanGui::PushItemWidth(VanGui::GetContentRegionAvail().x * 0.5f);
        VanGui::DragFloat("float##3a", &f);
        if (show_indented_items)
        {
            VanGui::Indent();
            VanGui::DragFloat("float (indented)##3b", &f);
            VanGui::Unindent();
        }
        VanGui::PopItemWidth();

        VanGui::Text("SetNextItemWidth/PushItemWidth(-GetContentRegionAvail().x * 0.5f)");
        VanGui::SameLine(); HelpMarker("Align to right edge minus half");
        VanGui::PushItemWidth(-VanGui::GetContentRegionAvail().x * 0.5f);
        VanGui::DragFloat("float##4a", &f);
        if (show_indented_items)
        {
            VanGui::Indent();
            VanGui::DragFloat("float (indented)##4b", &f);
            VanGui::Unindent();
        }
        VanGui::PopItemWidth();

        VanGui::Text("SetNextItemWidth/PushItemWidth(-Min(GetContentRegionAvail().x * 0.40f, GetFontSize() * 12))");
        VanGui::PushItemWidth(-VAN_MIN(VanGui::GetFontSize() * 12, VanGui::GetContentRegionAvail().x * 0.40f));
        VanGui::DragFloat("float##5a", &f);
        if (show_indented_items)
        {
            VanGui::Indent();
            VanGui::DragFloat("float (indented)##5b", &f);
            VanGui::Unindent();
        }
        VanGui::PopItemWidth();

        // Demonstrate using PushItemWidth to surround three items.
        // Calling SetNextItemWidth() before each of them would have the same effect.
        VanGui::Text("SetNextItemWidth/PushItemWidth(-FLT_MIN)");
        VanGui::SameLine(); HelpMarker("Align to right edge");
        VanGui::PushItemWidth(-FLT_MIN);
        VanGui::DragFloat("##float6a", &f);
        if (show_indented_items)
        {
            VanGui::Indent();
            VanGui::DragFloat("float (indented)##6b", &f);
            VanGui::Unindent();
        }
        VanGui::PopItemWidth();

        VanGui::TreePop();
    }

    if (VanGui::TreeNode("Basic Horizontal Layout"))
    {
        VANGUI_DEMO_MARKER("Layout/Basic Horizontal Layout");
        VanGui::TextWrapped("(Use VanGui::SameLine() to keep adding items to the right of the preceding item)");

        // Text
        VANGUI_DEMO_MARKER("Layout/Basic Horizontal Layout/SameLine");
        VanGui::Text("Two items: Hello"); VanGui::SameLine();
        VanGui::TextColored(VanVec4(1, 1, 0, 1), "Sailor");

        // Adjust spacing
        VanGui::Text("More spacing: Hello"); VanGui::SameLine(0, 20);
        VanGui::TextColored(VanVec4(1, 1, 0, 1), "Sailor");

        // Button
        VanGui::AlignTextToFramePadding();
        VanGui::Text("Normal buttons"); VanGui::SameLine();
        VanGui::Button("Banana"); VanGui::SameLine();
        VanGui::Button("Apple"); VanGui::SameLine();
        VanGui::Button("Corniflower");

        // Button
        VanGui::Text("Small buttons"); VanGui::SameLine();
        VanGui::SmallButton("Like this one"); VanGui::SameLine();
        VanGui::Text("can fit within a text block.");

        // Aligned to arbitrary position. Easy/cheap column.
        VANGUI_DEMO_MARKER("Layout/Basic Horizontal Layout/SameLine (with offset)");
        VanGui::Text("Aligned");
        VanGui::SameLine(150); VanGui::Text("x=150");
        VanGui::SameLine(300); VanGui::Text("x=300");
        VanGui::Text("Aligned");
        VanGui::SameLine(150); VanGui::SmallButton("x=150");
        VanGui::SameLine(300); VanGui::SmallButton("x=300");

        // Checkbox
        VANGUI_DEMO_MARKER("Layout/Basic Horizontal Layout/SameLine (more)");
        static bool c1 = false, c2 = false, c3 = false, c4 = false;
        VanGui::Checkbox("My", &c1); VanGui::SameLine();
        VanGui::Checkbox("Tailor", &c2); VanGui::SameLine();
        VanGui::Checkbox("Is", &c3); VanGui::SameLine();
        VanGui::Checkbox("Rich", &c4);

        // Various
        static float f0 = 1.0f, f1 = 2.0f, f2 = 3.0f;
        VanGui::PushItemWidth(VanGui::CalcTextSize("AAAAAAA").x);
        const char* items[] = { "AAAA", "BBBB", "CCCC", "DDDD" };
        static int item = -1;
        VanGui::Combo("Combo", &item, items, VAN_COUNTOF(items)); VanGui::SameLine();
        VanGui::SliderFloat("X", &f0, 0.0f, 5.0f); VanGui::SameLine();
        VanGui::SliderFloat("Y", &f1, 0.0f, 5.0f); VanGui::SameLine();
        VanGui::SliderFloat("Z", &f2, 0.0f, 5.0f);

        VanGui::Text("Lists:");
        static int selection[4] = { 0, 1, 2, 3 };
        for (int i = 0; i < 4; i++)
        {
            if (i > 0) VanGui::SameLine();
            VanGui::PushID(i);
            VanGui::ListBox("", &selection[i], items, VAN_COUNTOF(items));
            VanGui::PopID();
            //VanGui::SetItemTooltip("ListBox %d hovered", i);
        }
        VanGui::PopItemWidth();

        // Dummy
        VANGUI_DEMO_MARKER("Layout/Basic Horizontal Layout/Dummy");
        VanVec2 button_sz(40, 40);
        VanGui::Button("A", button_sz); VanGui::SameLine();
        VanGui::Dummy(button_sz); VanGui::SameLine();
        VanGui::Button("B", button_sz);

        // Manually wrapping
        // (we should eventually provide this as an automatic layout feature, but for now you can do it manually)
        VANGUI_DEMO_MARKER("Layout/Basic Horizontal Layout/Manual wrapping");
        VanGui::Text("Manual wrapping:");
        VanGuiStyle& style = VanGui::GetStyle();
        int buttons_count = 20;
        float window_visible_x2 = VanGui::GetCursorScreenPos().x + VanGui::GetContentRegionAvail().x;
        for (int n = 0; n < buttons_count; n++)
        {
            VanGui::PushID(n);
            VanGui::Button("Box", button_sz);
            float last_button_x2 = VanGui::GetItemRectMax().x;
            float next_button_x2 = last_button_x2 + style.ItemSpacing.x + button_sz.x; // Expected position if next button was on same line
            if (n + 1 < buttons_count && next_button_x2 < window_visible_x2)
                VanGui::SameLine();
            VanGui::PopID();
        }

        VanGui::TreePop();
    }

    if (VanGui::TreeNode("Groups"))
    {
        VANGUI_DEMO_MARKER("Layout/Groups");
        HelpMarker(
            "BeginGroup() basically locks the horizontal position for new line. "
            "EndGroup() bundles the whole group so that you can use \"item\" functions such as "
            "IsItemHovered()/IsItemActive() or SameLine() etc. on the whole group.");
        VanGui::BeginGroup();
        {
            VanGui::BeginGroup();
            VanGui::Button("AAA");
            VanGui::SameLine();
            VanGui::Button("BBB");
            VanGui::SameLine();
            VanGui::BeginGroup();
            VanGui::Button("CCC");
            VanGui::Button("DDD");
            VanGui::EndGroup();
            VanGui::SameLine();
            VanGui::Button("EEE");
            VanGui::EndGroup();
            VanGui::SetItemTooltip("First group hovered");
        }
        // Capture the group size and create widgets using the same size
        VanVec2 size = VanGui::GetItemRectSize();
        const float values[5] = { 0.5f, 0.20f, 0.80f, 0.60f, 0.25f };
        VanGui::PlotHistogram("##values", values, VAN_COUNTOF(values), 0, nullptr, 0.0f, 1.0f, size);

        VanGui::Button("ACTION", VanVec2((size.x - VanGui::GetStyle().ItemSpacing.x) * 0.5f, size.y));
        VanGui::SameLine();
        VanGui::Button("REACTION", VanVec2((size.x - VanGui::GetStyle().ItemSpacing.x) * 0.5f, size.y));
        VanGui::EndGroup();
        VanGui::SameLine();

        VanGui::Button("LEVERAGE\nBUZZWORD", size);
        VanGui::SameLine();

        if (VanGui::BeginListBox("List", size))
        {
            VanGui::Selectable("Selected", true);
            VanGui::Selectable("Not Selected", false);
            VanGui::EndListBox();
        }

        VanGui::TreePop();
    }

    if (VanGui::TreeNode("Text Baseline Alignment"))
    {
        VANGUI_DEMO_MARKER("Layout/Text Baseline Alignment");
        {
            VanGui::BulletText("Text baseline:");
            VanGui::SameLine(); HelpMarker(
                "This is testing the vertical alignment that gets applied on text to keep it aligned with widgets. "
                "Lines only composed of text or \"small\" widgets use less vertical space than lines with framed widgets.");
            VanGui::Indent();

            VanGui::Text("KO Blahblah"); VanGui::SameLine();
            VanGui::Button("Some framed item"); VanGui::SameLine();
            HelpMarker("Baseline of button will look misaligned with text..");

            // If your line starts with text, call AlignTextToFramePadding() to align text to upcoming widgets.
            // (because we don't know what's coming after the Text() statement, we need to move the text baseline
            // down by FramePadding.y ahead of time)
            VanGui::AlignTextToFramePadding();
            VanGui::Text("OK Blahblah"); VanGui::SameLine();
            VanGui::Button("Some framed item##2"); VanGui::SameLine();
            HelpMarker("We call AlignTextToFramePadding() to vertically align the text baseline by +FramePadding.y");

            // SmallButton() uses the same vertical padding as Text
            VanGui::Button("TEST##1"); VanGui::SameLine();
            VanGui::Text("TEST"); VanGui::SameLine();
            VanGui::SmallButton("TEST##2");

            // If your line starts with text, call AlignTextToFramePadding() to align text to upcoming widgets.
            VanGui::AlignTextToFramePadding();
            VanGui::Text("Text aligned to framed item"); VanGui::SameLine();
            VanGui::Button("Item##1"); VanGui::SameLine();
            VanGui::Text("Item"); VanGui::SameLine();
            VanGui::SmallButton("Item##2"); VanGui::SameLine();
            VanGui::Button("Item##3");

            VanGui::Unindent();
        }

        VanGui::Spacing();

        {
            VanGui::BulletText("Multi-line text:");
            VanGui::Indent();
            VanGui::Text("One\nTwo\nThree"); VanGui::SameLine();
            VanGui::Text("Hello\nWorld"); VanGui::SameLine();
            VanGui::Text("Banana");

            VanGui::Text("Banana"); VanGui::SameLine();
            VanGui::Text("Hello\nWorld"); VanGui::SameLine();
            VanGui::Text("One\nTwo\nThree");

            VanGui::Button("HOP##1"); VanGui::SameLine();
            VanGui::Text("Banana"); VanGui::SameLine();
            VanGui::Text("Hello\nWorld"); VanGui::SameLine();
            VanGui::Text("Banana");

            VanGui::Button("HOP##2"); VanGui::SameLine();
            VanGui::Text("Hello\nWorld"); VanGui::SameLine();
            VanGui::Text("Banana");
            VanGui::Unindent();
        }

        VanGui::Spacing();

        {
            VanGui::BulletText("Misc items:");
            VanGui::Indent();

            // SmallButton() sets FramePadding to zero. Text baseline is aligned to match baseline of previous Button.
            VanGui::Button("80x80", VanVec2(80, 80));
            VanGui::SameLine();
            VanGui::Button("50x50", VanVec2(50, 50));
            VanGui::SameLine();
            VanGui::Button("Button()");
            VanGui::SameLine();
            VanGui::SmallButton("SmallButton()");

            // Tree
            // (here the node appears after a button and has odd intent, so we use VanGuiTreeNodeFlags_DrawLinesNone to disable hierarchy outline)
            const float spacing = VanGui::GetStyle().ItemInnerSpacing.x;
            VanGui::Button("Button##1"); // Will make line higher
            VanGui::SameLine(0.0f, spacing);
            if (VanGui::TreeNodeEx("Node##1", VanGuiTreeNodeFlags_DrawLinesNone))
            {
                // Placeholder tree data
                for (int i = 0; i < 6; i++)
                    VanGui::BulletText("Item %d..", i);
                VanGui::TreePop();
            }

            const float padding = static_cast<float>(static_cast<int>(VanGui::GetFontSize() * 1.20f)); // Large padding
            VanGui::PushStyleVarY(VanGuiStyleVar_FramePadding, padding);
            VanGui::Button("Button##2");
            VanGui::PopStyleVar();
            VanGui::SameLine(0.0f, spacing);
            if (VanGui::TreeNodeEx("Node##2", VanGuiTreeNodeFlags_DrawLinesNone))
                VanGui::TreePop();

            // Vertically align text node a bit lower so it'll be vertically centered with upcoming widget.
            // Otherwise you can use SmallButton() (smaller fit).
            VanGui::AlignTextToFramePadding();

            // Common mistake to avoid: if we want to SameLine after TreeNode we need to do it before we add
            // other contents "inside" the node.
            bool node_open = VanGui::TreeNode("Node##3");
            VanGui::SameLine(0.0f, spacing); VanGui::Button("Button##3");
            if (node_open)
            {
                // Placeholder tree data
                for (int i = 0; i < 6; i++)
                    VanGui::BulletText("Item %d..", i);
                VanGui::TreePop();
            }

            // Bullet
            VanGui::Button("Button##4");
            VanGui::SameLine(0.0f, spacing);
            VanGui::BulletText("Bullet text");

            VanGui::AlignTextToFramePadding();
            VanGui::BulletText("Node");
            VanGui::SameLine(0.0f, spacing); VanGui::Button("Button##5");
            VanGui::Unindent();
        }

        VanGui::TreePop();
    }

    if (VanGui::TreeNode("Scrolling"))
    {
        VANGUI_DEMO_MARKER("Layout/Scrolling/Vertical");
        // Vertical scroll functions
        HelpMarker("Use SetScrollHereY() or SetScrollFromPosY() to scroll to a given vertical position.");

        static int track_item = 50;
        static bool enable_track = true;
        static bool enable_extra_decorations = false;
        static float scroll_to_off_px = 0.0f;
        static float scroll_to_pos_px = 200.0f;

        VanGui::Checkbox("Decoration", &enable_extra_decorations);

        VanGui::PushItemWidth(VanGui::GetFontSize() * 10);
        enable_track |= VanGui::DragInt("##item", &track_item, 0.25f, 0, 99, "Item = %d");
        VanGui::SameLine();
        VanGui::Checkbox("Track", &enable_track);

        bool scroll_to_off = VanGui::DragFloat("##off", &scroll_to_off_px, 1.00f, 0, FLT_MAX, "+%.0f px");
        VanGui::SameLine();
        scroll_to_off |= VanGui::Button("Scroll Offset");

        bool scroll_to_pos = VanGui::DragFloat("##pos", &scroll_to_pos_px, 1.00f, -10, FLT_MAX, "X/Y = %.0f px");
        VanGui::SameLine();
        scroll_to_pos |= VanGui::Button("Scroll To Pos");
        VanGui::PopItemWidth();

        if (scroll_to_off || scroll_to_pos)
            enable_track = false;

        VanGuiStyle& style = VanGui::GetStyle();
        float child_w = (VanGui::GetContentRegionAvail().x - 4 * style.ItemSpacing.x) / 5;
        if (child_w < 1.0f)
            child_w = 1.0f;
        VanGui::PushID("##VerticalScrolling");
        for (int i = 0; i < 5; i++)
        {
            if (i > 0) VanGui::SameLine();
            VanGui::BeginGroup();
            const char* names[] = { "Top", "25%", "Center", "75%", "Bottom" };
            VanGui::TextUnformatted(names[i]);

            const VanGuiWindowFlags child_flags = enable_extra_decorations ? VanGuiWindowFlags_MenuBar : 0;
            const VanGuiID child_id = VanGui::GetID(reinterpret_cast<void*>(static_cast<intptr_t>(i)));
            const bool child_is_visible = VanGui::BeginChild(child_id, VanVec2(child_w, 200.0f), VanGuiChildFlags_Borders, child_flags);
            if (VanGui::BeginMenuBar())
            {
                VanGui::TextUnformatted("abc");
                VanGui::EndMenuBar();
            }
            if (scroll_to_off)
                VanGui::SetScrollY(scroll_to_off_px);
            if (scroll_to_pos)
                VanGui::SetScrollFromPosY(VanGui::GetCursorStartPos().y + scroll_to_pos_px, i * 0.25f);
            if (child_is_visible) // Avoid calling SetScrollHereY when running with culled items
            {
                for (int item = 0; item < 100; item++)
                {
                    if (enable_track && item == track_item)
                    {
                        VanGui::TextColored(VanVec4(1, 1, 0, 1), "Item %d", item);
                        VanGui::SetScrollHereY(i * 0.25f); // 0.0f:top, 0.5f:center, 1.0f:bottom
                    }
                    else
                    {
                        VanGui::Text("Item %d", item);
                    }
                }
            }
            float scroll_y = VanGui::GetScrollY();
            float scroll_max_y = VanGui::GetScrollMaxY();
            VanGui::EndChild();
            VanGui::Text("%.0f/%.0f", scroll_y, scroll_max_y);
            VanGui::EndGroup();
        }
        VanGui::PopID();

        // Horizontal scroll functions
        VANGUI_DEMO_MARKER("Layout/Scrolling/Horizontal");
        VanGui::Spacing();
        HelpMarker(
            "Use SetScrollHereX() or SetScrollFromPosX() to scroll to a given horizontal position.\n\n"
            "Because the clipping rectangle of most window hides half worth of WindowPadding on the "
            "left/right, using SetScrollFromPosX(+1) will usually result in clipped text whereas the "
            "equivalent SetScrollFromPosY(+1) wouldn't.");
        VanGui::PushID("##HorizontalScrolling");
        for (int i = 0; i < 5; i++)
        {
            float child_height = VanGui::GetTextLineHeight() + style.ScrollbarSize + style.WindowPadding.y * 2.0f;
            VanGuiWindowFlags child_flags = VanGuiWindowFlags_HorizontalScrollbar | (enable_extra_decorations ? VanGuiWindowFlags_AlwaysVerticalScrollbar : 0);
            VanGuiID child_id = VanGui::GetID(reinterpret_cast<void*>(static_cast<intptr_t>(i)));
            bool child_is_visible = VanGui::BeginChild(child_id, VanVec2(-100, child_height), VanGuiChildFlags_Borders, child_flags);
            if (scroll_to_off)
                VanGui::SetScrollX(scroll_to_off_px);
            if (scroll_to_pos)
                VanGui::SetScrollFromPosX(VanGui::GetCursorStartPos().x + scroll_to_pos_px, i * 0.25f);
            if (child_is_visible) // Avoid calling SetScrollHereY when running with culled items
            {
                for (int item = 0; item < 100; item++)
                {
                    if (item > 0)
                        VanGui::SameLine();
                    if (enable_track && item == track_item)
                    {
                        VanGui::TextColored(VanVec4(1, 1, 0, 1), "Item %d", item);
                        VanGui::SetScrollHereX(i * 0.25f); // 0.0f:left, 0.5f:center, 1.0f:right
                    }
                    else
                    {
                        VanGui::Text("Item %d", item);
                    }
                }
            }
            float scroll_x = VanGui::GetScrollX();
            float scroll_max_x = VanGui::GetScrollMaxX();
            VanGui::EndChild();
            VanGui::SameLine();
            const char* names[] = { "Left", "25%", "Center", "75%", "Right" };
            VanGui::Text("%s\n%.0f/%.0f", names[i], scroll_x, scroll_max_x);
            VanGui::Spacing();
        }
        VanGui::PopID();

        // Miscellaneous Horizontal Scrolling Demo
        VANGUI_DEMO_MARKER("Layout/Scrolling/Horizontal (more)");
        HelpMarker(
            "Horizontal scrolling for a window is enabled via the VanGuiWindowFlags_HorizontalScrollbar flag.\n\n"
            "You may want to also explicitly specify content width by using SetNextWindowContentWidth() before Begin().");
        static int lines = 7;
        VanGui::SliderInt("Lines", &lines, 1, 15);
        VanGui::PushStyleVar(VanGuiStyleVar_FrameRounding, 3.0f);
        VanGui::PushStyleVar(VanGuiStyleVar_FramePadding, VanVec2(2.0f, 1.0f));
        VanVec2 scrolling_child_size = VanVec2(0, VanGui::GetFrameHeightWithSpacing() * 7 + 30);
        VanGui::BeginChild("scrolling", scrolling_child_size, VanGuiChildFlags_Borders, VanGuiWindowFlags_HorizontalScrollbar);
        for (int line = 0; line < lines; line++)
        {
            // Display random stuff. For the sake of this trivial demo we are using basic Button() + SameLine()
            // If you want to create your own time line for a real application you may be better off manipulating
            // the cursor position yourself, aka using SetCursorPos/SetCursorScreenPos to position the widgets
            // yourself. You may also want to use the lower-level VanDrawList API.
            int num_buttons = 10 + ((line & 1) ? line * 9 : line * 3);
            for (int n = 0; n < num_buttons; n++)
            {
                if (n > 0) VanGui::SameLine();
                VanGui::PushID(n + line * 1000);
                char num_buf[16];
                snprintf(num_buf, sizeof(num_buf), "%d", n);
                const char* label = (!(n % 15)) ? "FizzBuzz" : (!(n % 3)) ? "Fizz" : (!(n % 5)) ? "Buzz" : num_buf;
                float hue = n * 0.05f;
                VanGui::PushStyleColor(VanGuiCol_Button, static_cast<VanVec4>(VanColor::HSV(hue, 0.6f, 0.6f)));
                VanGui::PushStyleColor(VanGuiCol_ButtonHovered, static_cast<VanVec4>(VanColor::HSV(hue, 0.7f, 0.7f)));
                VanGui::PushStyleColor(VanGuiCol_ButtonActive, static_cast<VanVec4>(VanColor::HSV(hue, 0.8f, 0.8f)));
                VanGui::Button(label, VanVec2(40.0f + sinf(static_cast<float>(line + n)) * 20.0f, 0.0f));
                VanGui::PopStyleColor(3);
                VanGui::PopID();
            }
        }
        float scroll_x = VanGui::GetScrollX();
        float scroll_max_x = VanGui::GetScrollMaxX();
        VanGui::EndChild();
        VanGui::PopStyleVar(2);
        float scroll_x_delta = 0.0f;
        VanGui::SmallButton("<<");
        if (VanGui::IsItemActive())
            scroll_x_delta = -VanGui::GetIO().DeltaTime * 1000.0f;
        VanGui::SameLine();
        VanGui::Text("Scroll from code"); VanGui::SameLine();
        VanGui::SmallButton(">>");
        if (VanGui::IsItemActive())
            scroll_x_delta = +VanGui::GetIO().DeltaTime * 1000.0f;
        VanGui::SameLine();
        VanGui::Text("%.0f/%.0f", scroll_x, scroll_max_x);
        if (scroll_x_delta != 0.0f)
        {
            // Demonstrate a trick: you can use Begin to set yourself in the context of another window
            // (here we are already out of your child window)
            VanGui::BeginChild("scrolling");
            VanGui::SetScrollX(VanGui::GetScrollX() + scroll_x_delta);
            VanGui::EndChild();
        }
        VanGui::Spacing();

        static bool show_horizontal_contents_size_demo_window = false;
        VanGui::Checkbox("Show Horizontal contents size demo window", &show_horizontal_contents_size_demo_window);

        if (show_horizontal_contents_size_demo_window)
        {
            static bool show_h_scrollbar = true;
            static bool show_button = true;
            static bool show_tree_nodes = true;
            static bool show_text_wrapped = false;
            static bool show_columns = true;
            static bool show_tab_bar = true;
            static bool show_child = false;
            static bool explicit_content_size = false;
            static float contents_size_x = 300.0f;
            if (explicit_content_size)
                VanGui::SetNextWindowContentSize(VanVec2(contents_size_x, 0.0f));
            VanGui::Begin("Horizontal contents size demo window", &show_horizontal_contents_size_demo_window, show_h_scrollbar ? VanGuiWindowFlags_HorizontalScrollbar : 0);
            VANGUI_DEMO_MARKER("Layout/Scrolling/Horizontal contents size demo window");
            VanGui::PushStyleVar(VanGuiStyleVar_ItemSpacing, VanVec2(2, 0));
            VanGui::PushStyleVar(VanGuiStyleVar_FramePadding, VanVec2(2, 0));
            HelpMarker(
                "Test how different widgets react and impact the work rectangle growing when horizontal scrolling is enabled.\n\n"
                "Use 'Metrics->Tools->Show windows rectangles' to visualize rectangles.");
            VanGui::Checkbox("H-scrollbar", &show_h_scrollbar);
            VanGui::Checkbox("Button", &show_button);            // Will grow contents size (unless explicitly overwritten)
            VanGui::Checkbox("Tree nodes", &show_tree_nodes);    // Will grow contents size and display highlight over full width
            VanGui::Checkbox("Text wrapped", &show_text_wrapped);// Will grow and use contents size
            VanGui::Checkbox("Columns", &show_columns);          // Will use contents size
            VanGui::Checkbox("Tab bar", &show_tab_bar);          // Will use contents size
            VanGui::Checkbox("Child", &show_child);              // Will grow and use contents size
            VanGui::Checkbox("Explicit content size", &explicit_content_size);
            VanGui::Text("Scroll %.1f/%.1f %.1f/%.1f", VanGui::GetScrollX(), VanGui::GetScrollMaxX(), VanGui::GetScrollY(), VanGui::GetScrollMaxY());
            if (explicit_content_size)
            {
                VanGui::SameLine();
                VanGui::SetNextItemWidth(VanGui::CalcTextSize("123456").x);
                VanGui::DragFloat("##csx", &contents_size_x);
                VanVec2 p = VanGui::GetCursorScreenPos();
                VanGui::GetWindowDrawList()->AddRectFilled(p, VanVec2(p.x + 10, p.y + 10), VAN_COL32_WHITE);
                VanGui::GetWindowDrawList()->AddRectFilled(VanVec2(p.x + contents_size_x - 10, p.y), VanVec2(p.x + contents_size_x, p.y + 10), VAN_COL32_WHITE);
                VanGui::Dummy(VanVec2(0, 10));
            }
            VanGui::PopStyleVar(2);
            VanGui::Separator();
            if (show_button)
            {
                VanGui::Button("this is a 300-wide button", VanVec2(300, 0));
            }
            if (show_tree_nodes)
            {
                bool open = true;
                if (VanGui::TreeNode("this is a tree node"))
                {
                    if (VanGui::TreeNode("another one of those tree node..."))
                    {
                        VanGui::Text("Some tree contents");
                        VanGui::TreePop();
                    }
                    VanGui::TreePop();
                }
                VanGui::CollapsingHeader("CollapsingHeader", &open);
            }
            if (show_text_wrapped)
            {
                VanGui::TextWrapped("This text should automatically wrap on the edge of the work rectangle.");
            }
            if (show_columns)
            {
                VanGui::Text("Tables:");
                if (VanGui::BeginTable("table", 4, VanGuiTableFlags_Borders))
                {
                    for (int n = 0; n < 4; n++)
                    {
                        VanGui::TableNextColumn();
                        VanGui::Text("Width %.2f", VanGui::GetContentRegionAvail().x);
                    }
                    VanGui::EndTable();
                }
                VanGui::Text("Columns:");
                VanGui::Columns(4);
                for (int n = 0; n < 4; n++)
                {
                    VanGui::Text("Width %.2f", VanGui::GetColumnWidth());
                    VanGui::NextColumn();
                }
                VanGui::Columns(1);
            }
            if (show_tab_bar && VanGui::BeginTabBar("Hello"))
            {
                if (VanGui::BeginTabItem("OneOneOne")) { VanGui::EndTabItem(); }
                if (VanGui::BeginTabItem("TwoTwoTwo")) { VanGui::EndTabItem(); }
                if (VanGui::BeginTabItem("ThreeThreeThree")) { VanGui::EndTabItem(); }
                if (VanGui::BeginTabItem("FourFourFour")) { VanGui::EndTabItem(); }
                VanGui::EndTabBar();
            }
            if (show_child)
            {
                VanGui::BeginChild("child", VanVec2(0, 0), VanGuiChildFlags_Borders);
                VanGui::EndChild();
            }
            VanGui::End();
        }

        VanGui::TreePop();
    }

    if (VanGui::TreeNode("Text Clipping"))
    {
        VANGUI_DEMO_MARKER("Layout/Text Clipping");
        static VanVec2 size(100.0f, 100.0f);
        static VanVec2 offset(30.0f, 30.0f);
        VanGui::DragFloat2("size", reinterpret_cast<float*>(&size), 0.5f, 1.0f, 200.0f, "%.0f");
        VanGui::TextWrapped("(Click and drag to scroll)");

        HelpMarker(
            "(Left) Using VanGui::PushClipRect():\n"
            "Will alter VanGui hit-testing logic + VanDrawList rendering.\n"
            "(use this if you want your clipping rectangle to affect interactions)\n\n"
            "(Center) Using VanDrawList::PushClipRect():\n"
            "Will alter VanDrawList rendering only.\n"
            "(use this as a shortcut if you are only using VanDrawList calls)\n\n"
            "(Right) Using VanDrawList::AddText() with a fine ClipRect:\n"
            "Will alter only this specific VanDrawList::AddText() rendering.\n"
            "This is often used internally to avoid altering the clipping rectangle and minimize draw calls.");

        for (int n = 0; n < 3; n++)
        {
            if (n > 0)
                VanGui::SameLine();

            VanGui::PushID(n);
            VanGui::InvisibleButton("##canvas", size);
            if (VanGui::IsItemActive() && VanGui::IsMouseDragging(VanGuiMouseButton_Left))
            {
                offset.x += VanGui::GetIO().MouseDelta.x;
                offset.y += VanGui::GetIO().MouseDelta.y;
            }
            VanGui::PopID();
            if (!VanGui::IsItemVisible()) // Skip rendering as VanDrawList elements are not clipped.
                continue;

            const VanVec2 p0 = VanGui::GetItemRectMin();
            const VanVec2 p1 = VanGui::GetItemRectMax();
            const char* text_str = "Line 1 hello\nLine 2 clip me!";
            const VanVec2 text_pos = VanVec2(p0.x + offset.x, p0.y + offset.y);
            VanDrawList* draw_list = VanGui::GetWindowDrawList();
            switch (n)
            {
            case 0:
                VanGui::PushClipRect(p0, p1, true);
                draw_list->AddRectFilled(p0, p1, VAN_COL32(90, 90, 120, 255));
                draw_list->AddText(text_pos, VAN_COL32_WHITE, text_str);
                VanGui::PopClipRect();
                break;
            case 1:
                draw_list->PushClipRect(p0, p1, true);
                draw_list->AddRectFilled(p0, p1, VAN_COL32(90, 90, 120, 255));
                draw_list->AddText(text_pos, VAN_COL32_WHITE, text_str);
                draw_list->PopClipRect();
                break;
            case 2:
                VanVec4 clip_rect(p0.x, p0.y, p1.x, p1.y); // AddText() takes a VanVec4* here so let's convert.
                draw_list->AddRectFilled(p0, p1, VAN_COL32(90, 90, 120, 255));
                draw_list->AddText(VanGui::GetFont(), VanGui::GetFontSize(), text_pos, VAN_COL32_WHITE, text_str, nullptr, 0.0f, &clip_rect);
                break;
            }
        }

        VanGui::TreePop();
    }

    if (VanGui::TreeNode("Overlap Mode"))
    {
        VANGUI_DEMO_MARKER("Layout/Overlap Mode");
        static bool enable_allow_overlap = true;

        HelpMarker(
            "Hit-testing is by default performed in item submission order, which generally is perceived as 'back-to-front'.\n\n"
            "By using SetNextItemAllowOverlap() you can notify that an item may be overlapped by another. "
            "Doing so alters the hovering logic: items using AllowOverlap mode requires an extra frame to accept hovered state.");
        VanGui::Checkbox("Enable AllowOverlap", &enable_allow_overlap);

        VanVec2 button1_pos = VanGui::GetCursorScreenPos();
        VanVec2 button2_pos = VanVec2(button1_pos.x + 50.0f, button1_pos.y + 50.0f);
        if (enable_allow_overlap)
            VanGui::SetNextItemAllowOverlap();
        VanGui::Button("Button 1", VanVec2(80, 80));
        VanGui::SetCursorScreenPos(button2_pos);
        VanGui::Button("Button 2", VanVec2(80, 80));

        // This is typically used with width-spanning items.
        // (note that Selectable() has a dedicated flag VanGuiSelectableFlags_AllowOverlap, which is a shortcut
        // for using SetNextItemAllowOverlap(). For demo purpose we use SetNextItemAllowOverlap() here.)
        if (enable_allow_overlap)
            VanGui::SetNextItemAllowOverlap();
        VanGui::Selectable("Some Selectable", false);
        VanGui::SameLine();
        VanGui::SmallButton("++");

        VanGui::TreePop();
    }
}

//-----------------------------------------------------------------------------
// [SECTION] DemoWindowPopups()
//-----------------------------------------------------------------------------

static void DemoWindowPopups()
{
    if (!VanGui::CollapsingHeader("Popups & Modal windows"))
        return;

    // The properties of popups windows are:
    // - They block normal mouse hovering detection outside them. (*)
    // - Unless modal, they can be closed by clicking anywhere outside them, or by pressing ESCAPE.
    // - Their visibility state (~bool) is held internally by VanGUI instead of being held by the programmer as
    //   we are used to with regular Begin() calls. User can manipulate the visibility state by calling OpenPopup().
    // (*) One can use IsItemHovered(VanGuiHoveredFlags_AllowWhenBlockedByPopup) to bypass it and detect hovering even
    //     when normally blocked by a popup.
    // Those three properties are connected. The library needs to hold their visibility state BECAUSE it can close
    // popups at any time.

    // Typical use for regular windows:
    //   bool my_tool_is_active = false; if (VanGui::Button("Open")) my_tool_is_active = true; [...] if (my_tool_is_active) Begin("My Tool", &my_tool_is_active) { [...] } End();
    // Typical use for popups:
    //   if (VanGui::Button("Open")) VanGui::OpenPopup("MyPopup"); if (VanGui::BeginPopup("MyPopup")) { [...] EndPopup(); }

    // With popups we have to go through a library call (here OpenPopup) to manipulate the visibility state.
    // This may be a bit confusing at first but it should quickly make sense. Follow on the examples below.

    if (VanGui::TreeNode("Popups"))
    {
        VANGUI_DEMO_MARKER("Popups/Popups");
        VanGui::TextWrapped(
            "When a popup is active, it inhibits interacting with windows that are behind the popup. "
            "Clicking outside the popup closes it.");

        static int selected_fish = -1;
        const char* names[] = { "Bream", "Haddock", "Mackerel", "Pollock", "Tilefish" };
        static bool toggles[] = { true, false, false, false, false };

        // Simple selection popup (if you want to show the current selection inside the Button itself,
        // you may want to build a string using the "###" operator to preserve a constant ID with a variable label)
        if (VanGui::Button("Select.."))
            VanGui::OpenPopup("my_select_popup");
        VanGui::SameLine();
        VanGui::TextUnformatted(selected_fish == -1 ? "<None>" : names[selected_fish]);
        if (VanGui::BeginPopup("my_select_popup"))
        {
            VanGui::SeparatorText("Aquarium");
            for (int i = 0; i < VAN_COUNTOF(names); i++)
                if (VanGui::Selectable(names[i]))
                    selected_fish = i;
            VanGui::EndPopup();
        }

        // Showing a menu with toggles
        if (VanGui::Button("Toggle.."))
            VanGui::OpenPopup("my_toggle_popup");
        if (VanGui::BeginPopup("my_toggle_popup"))
        {
            for (int i = 0; i < VAN_COUNTOF(names); i++)
                VanGui::MenuItem(names[i], "", &toggles[i]);
            if (VanGui::BeginMenu("Sub-menu"))
            {
                VanGui::MenuItem("Click me");
                VanGui::EndMenu();
            }

            VanGui::Separator();
            VanGui::Text("Tooltip here");
            VanGui::SetItemTooltip("I am a tooltip over a popup");

            if (VanGui::Button("Stacked Popup"))
                VanGui::OpenPopup("another popup");
            if (VanGui::BeginPopup("another popup"))
            {
                for (int i = 0; i < VAN_COUNTOF(names); i++)
                    VanGui::MenuItem(names[i], "", &toggles[i]);
                if (VanGui::BeginMenu("Sub-menu"))
                {
                    VanGui::MenuItem("Click me");
                    if (VanGui::Button("Stacked Popup"))
                        VanGui::OpenPopup("another popup");
                    if (VanGui::BeginPopup("another popup"))
                    {
                        VanGui::Text("I am the last one here.");
                        VanGui::EndPopup();
                    }
                    VanGui::EndMenu();
                }
                VanGui::EndPopup();
            }
            VanGui::EndPopup();
        }

        // Call the more complete ShowExampleMenuFile which we use in various places of this demo
        if (VanGui::Button("With a menu.."))
            VanGui::OpenPopup("my_file_popup");
        if (VanGui::BeginPopup("my_file_popup", VanGuiWindowFlags_MenuBar))
        {
            if (VanGui::BeginMenuBar())
            {
                if (VanGui::BeginMenu("File"))
                {
                    ShowExampleMenuFile();
                    VanGui::EndMenu();
                }
                if (VanGui::BeginMenu("Edit"))
                {
                    VanGui::MenuItem("Dummy");
                    VanGui::EndMenu();
                }
                VanGui::EndMenuBar();
            }
            VanGui::Text("Hello from popup!");
            VanGui::Button("This is a dummy button..");
            VanGui::EndPopup();
        }

        VanGui::TreePop();
    }

    if (VanGui::TreeNode("Context menus"))
    {
        VANGUI_DEMO_MARKER("Popups/Context menus");
        HelpMarker("\"Context\" functions are simple helpers to associate a Popup to a given Item or Window identifier.");

        // BeginPopupContextItem() is a helper to provide common/simple popup behavior of essentially doing:
        //     if (id == 0)
        //         id = GetItemID(); // Use last item id
        //     if (IsItemHovered() && IsMouseReleased(VanGuiMouseButton_Right))
        //         OpenPopup(id);
        //     return BeginPopup(id);
        // For advanced uses you may want to replicate and customize this code.
        // See more details in BeginPopupContextItem().

        // Example 1
        // When used after an item that has an ID (e.g. Button), we can skip providing an ID to BeginPopupContextItem(),
        // and BeginPopupContextItem() will use the last item ID as the popup ID.
        {
            const char* names[5] = { "Label1", "Label2", "Label3", "Label4", "Label5" };
            static int selected = -1;
            for (int n = 0; n < 5; n++)
            {
                if (VanGui::Selectable(names[n], selected == n))
                    selected = n;
                if (VanGui::BeginPopupContextItem()) // <-- use last item id as popup id
                {
                    selected = n;
                    VanGui::Text("This is a popup for \"%s\"!", names[n]);
                    if (VanGui::Button("Close"))
                        VanGui::CloseCurrentPopup();
                    VanGui::EndPopup();
                }
                VanGui::SetItemTooltip("Right-click to open popup");
            }
        }

        // Example 2
        // Popup on a Text() element which doesn't have an identifier: we need to provide an identifier to BeginPopupContextItem().
        // Using an explicit identifier is also convenient if you want to activate the popups from different locations.
        {
            HelpMarker("Text() elements don't have stable identifiers so we need to provide one.");
            static float value = 0.5f;
            VanGui::Text("Value = %.3f <-- (1) right-click this text", value);
            if (VanGui::BeginPopupContextItem("my popup"))
            {
                if (VanGui::Selectable("Set to zero")) value = 0.0f;
                if (VanGui::Selectable("Set to PI")) value = 3.1415f;
                VanGui::SetNextItemWidth(-FLT_MIN);
                VanGui::DragFloat("##Value", &value, 0.1f, 0.0f, 0.0f);
                VanGui::EndPopup();
            }

            // We can also use OpenPopupOnItemClick() to toggle the visibility of a given popup.
            // Here we make it that right-clicking this other text element opens the same popup as above.
            // The popup itself will be submitted by the code above.
            VanGui::Text("(2) Or right-click this text");
            VanGui::OpenPopupOnItemClick("my popup", VanGuiPopupFlags_MouseButtonRight);

            // Back to square one: manually open the same popup.
            if (VanGui::Button("(3) Or click this button"))
                VanGui::OpenPopup("my popup");
        }

        // Example 3
        // When using BeginPopupContextItem() with an implicit identifier (nullptr == use last item ID),
        // we need to make sure your item identifier is stable.
        // In this example we showcase altering the item label while preserving its identifier, using the ### operator (see FAQ).
        {
            HelpMarker("Showcase using a popup ID linked to item ID, with the item having a changing label + stable ID using the ### operator.");
            static char name[32] = "Label1";
            char buf[64];
            snprintf(buf, sizeof(buf), "Button: %s###Button", name); // ### operator override ID ignoring the preceding label
            VanGui::Button(buf);
            if (VanGui::BeginPopupContextItem())
            {
                VanGui::Text("Edit name:");
                VanGui::InputText("##edit", name, VAN_COUNTOF(name));
                if (VanGui::Button("Close"))
                    VanGui::CloseCurrentPopup();
                VanGui::EndPopup();
            }
            VanGui::SameLine(); VanGui::Text("(<-- right-click here)");
        }

        VanGui::TreePop();
    }

    if (VanGui::TreeNode("Modals"))
    {
        VANGUI_DEMO_MARKER("Popups/Modals");
        VanGui::TextWrapped("Modal windows are like popups but the user cannot close them by clicking outside.");

        if (VanGui::Button("Delete.."))
            VanGui::OpenPopup("Delete?");

        // Always center this window when appearing
        VanVec2 center = VanGui::GetMainViewport()->GetCenter();
        VanGui::SetNextWindowPos(center, VanGuiCond_Appearing, VanVec2(0.5f, 0.5f));

        if (VanGui::BeginPopupModal("Delete?", nullptr, VanGuiWindowFlags_AlwaysAutoResize))
        {
            VanGui::Text("All those beautiful files will be deleted.\nThis operation cannot be undone!");
            VanGui::Separator();

            //static int unused_i = 0;
            //VanGui::Combo("Combo", &unused_i, "Delete\0Delete harder\0");

            static bool dont_ask_me_next_time = false;
            VanGui::PushStyleVar(VanGuiStyleVar_FramePadding, VanVec2(0, 0));
            VanGui::Checkbox("Don't ask me next time", &dont_ask_me_next_time);
            VanGui::PopStyleVar();

            if (VanGui::Button("OK", VanVec2(120, 0))) { VanGui::CloseCurrentPopup(); }
            VanGui::SetItemDefaultFocus();
            VanGui::SameLine();
            if (VanGui::Button("Cancel", VanVec2(120, 0))) { VanGui::CloseCurrentPopup(); }
            VanGui::EndPopup();
        }

        if (VanGui::Button("Stacked modals.."))
            VanGui::OpenPopup("Stacked 1");
        if (VanGui::BeginPopupModal("Stacked 1", nullptr, VanGuiWindowFlags_MenuBar))
        {
            if (VanGui::BeginMenuBar())
            {
                if (VanGui::BeginMenu("File"))
                {
                    if (VanGui::MenuItem("Some menu item")) {}
                    VanGui::EndMenu();
                }
                VanGui::EndMenuBar();
            }
            VanGui::Text("Hello from Stacked The First\nUsing style.Colors[VanGuiCol_ModalWindowDimBg] behind it.");

            // Testing behavior of widgets stacking their own regular popups over the modal.
            static int item = 1;
            static float color[4] = { 0.4f, 0.7f, 0.0f, 0.5f };
            VanGui::Combo("Combo", &item, "aaaa\0bbbb\0cccc\0dddd\0eeee\0\0");
            VanGui::ColorEdit4("Color", color);

            if (VanGui::Button("Add another modal.."))
                VanGui::OpenPopup("Stacked 2");

            // Also demonstrate passing a bool* to BeginPopupModal(), this will create a regular close button which
            // will close the popup. Note that the visibility state of popups is owned by vangui, so the input value
            // of the bool actually doesn't matter here.
            bool unused_open = true;
            if (VanGui::BeginPopupModal("Stacked 2", &unused_open))
            {
                VanGui::Text("Hello from Stacked The Second!");
                VanGui::ColorEdit4("Color", color); // Allow opening another nested popup
                if (VanGui::Button("Close"))
                    VanGui::CloseCurrentPopup();
                VanGui::EndPopup();
            }

            if (VanGui::Button("Close"))
                VanGui::CloseCurrentPopup();
            VanGui::EndPopup();
        }

        VanGui::TreePop();
    }

    if (VanGui::TreeNode("Menus inside a regular window"))
    {
        VANGUI_DEMO_MARKER("Popups/Menus inside a regular window");
        VanGui::TextWrapped("Below we are testing adding menu items to a regular window. It's rather unusual but should work!");
        VanGui::Separator();

        VanGui::MenuItem("Menu item", "Ctrl+M");
        if (VanGui::BeginMenu("Menu inside a regular window"))
        {
            ShowExampleMenuFile();
            VanGui::EndMenu();
        }
        VanGui::Separator();
        VanGui::TreePop();
    }
}

// Dummy data structure that we use for the Table demo.
// (pre-C++11 doesn't allow us to instantiate VanVector<MyItem> template if this structure is defined inside the demo function)
namespace
{
// We are passing our own identifier to TableSetupColumn() to facilitate identifying columns in the sorting code.
// This identifier will be passed down into VanGuiTableSortSpec::ColumnUserID.
// But it is possible to omit the user id parameter of TableSetupColumn() and just use the column index instead! (VanGuiTableSortSpec::ColumnIndex)
// If you don't use sorting, you will generally never care about giving column an ID!
enum MyItemColumnID
{
    MyItemColumnID_ID,
    MyItemColumnID_Name,
    MyItemColumnID_Action,
    MyItemColumnID_Quantity,
    MyItemColumnID_Description
};

struct MyItem
{
    int         ID;
    const char* Name;
    int         Quantity;

    // We have a problem which is affecting _only this demo_ and should not affect your code:
    // As we don't rely on std:: or other third-party library to compile dear vangui, we only have reliable access to qsort(),
    // however qsort doesn't allow passing user data to comparing function.
    // As a workaround, we are storing the sort specs in a static/global for the comparing function to access.
    // In your own use case you would probably pass the sort specs to your sorting/comparing functions directly and not use a global.
    // We could technically call VanGui::TableGetSortSpecs() in CompareWithSortSpecs(), but considering that this function is called
    // very often by the sorting algorithm it would be a little wasteful.
    static const VanGuiTableSortSpecs* s_current_sort_specs;

    static void SortWithSortSpecs(VanGuiTableSortSpecs* sort_specs, MyItem* items, int items_count)
    {
        s_current_sort_specs = sort_specs; // Store in variable accessible by the sort function.
        if (items_count > 1)
            qsort(items, static_cast<size_t>(items_count), sizeof(items[0]), MyItem::CompareWithSortSpecs);
        s_current_sort_specs = nullptr;
    }

    // Compare function to be used by qsort()
    static int VANGUI_CDECL CompareWithSortSpecs(const void* lhs, const void* rhs)
    {
        const MyItem* a = (const MyItem*)lhs;
        const MyItem* b = (const MyItem*)rhs;
        for (int n = 0; n < s_current_sort_specs->SpecsCount; n++)
        {
            // Here we identify columns using the ColumnUserID value that we ourselves passed to TableSetupColumn()
            // We could also choose to identify columns based on their index (sort_spec->ColumnIndex), which is simpler!
            const VanGuiTableColumnSortSpecs* sort_spec = &s_current_sort_specs->Specs[n];
            int delta = 0;
            switch (sort_spec->ColumnUserID)
            {
            case MyItemColumnID_ID:             delta = (a->ID - b->ID);                break;
            case MyItemColumnID_Name:           delta = (strcmp(a->Name, b->Name));     break;
            case MyItemColumnID_Quantity:       delta = (a->Quantity - b->Quantity);    break;
            case MyItemColumnID_Description:    delta = (strcmp(a->Name, b->Name));     break;
            default: VAN_ASSERT(0); break;
            }
            if (delta > 0)
                return (sort_spec->SortDirection == VanGuiSortDirection_Ascending) ? +1 : -1;
            if (delta < 0)
                return (sort_spec->SortDirection == VanGuiSortDirection_Ascending) ? -1 : +1;
        }

        // qsort() is instable so always return a way to differentiate items.
        // Your own compare function may want to avoid fallback on implicit sort specs.
        // e.g. a Name compare if it wasn't already part of the sort specs.
        return a->ID - b->ID;
    }
};
const VanGuiTableSortSpecs* MyItem::s_current_sort_specs = nullptr;
}

// Make the UI compact because there are so many fields
static void PushStyleCompact()
{
    VanGuiStyle& style = VanGui::GetStyle();
    VanGui::PushStyleVarY(VanGuiStyleVar_FramePadding, static_cast<float>(static_cast<int>(style.FramePadding.y * 0.60f)));
    VanGui::PushStyleVarY(VanGuiStyleVar_ItemSpacing, static_cast<float>(static_cast<int>(style.ItemSpacing.y * 0.60f)));
}

static void PopStyleCompact()
{
    VanGui::PopStyleVar(2);
}

// Show a combo box with a choice of sizing policies
static void EditTableSizingFlags(VanGuiTableFlags* p_flags)
{
    struct EnumDesc { VanGuiTableFlags Value; const char* Name; const char* Tooltip; };
    static const EnumDesc policies[] =
    {
        { VanGuiTableFlags_None,               "Default",                            "Use default sizing policy:\n- VanGuiTableFlags_SizingFixedFit if ScrollX is on or if host window has VanGuiWindowFlags_AlwaysAutoResize.\n- VanGuiTableFlags_SizingStretchSame otherwise." },
        { VanGuiTableFlags_SizingFixedFit,     "VanGuiTableFlags_SizingFixedFit",     "Columns default to _WidthFixed (if resizable) or _WidthAuto (if not resizable), matching contents width." },
        { VanGuiTableFlags_SizingFixedSame,    "VanGuiTableFlags_SizingFixedSame",    "Columns are all the same width, matching the maximum contents width.\nImplicitly disable VanGuiTableFlags_Resizable and enable VanGuiTableFlags_NoKeepColumnsVisible." },
        { VanGuiTableFlags_SizingStretchProp,  "VanGuiTableFlags_SizingStretchProp",  "Columns default to _WidthStretch with weights proportional to their widths." },
        { VanGuiTableFlags_SizingStretchSame,  "VanGuiTableFlags_SizingStretchSame",  "Columns default to _WidthStretch with same weights." }
    };
    int idx;
    for (idx = 0; idx < VAN_COUNTOF(policies); idx++)
        if (policies[idx].Value == (*p_flags & VanGuiTableFlags_SizingMask_))
            break;
    const char* preview_text = (idx < VAN_COUNTOF(policies)) ? policies[idx].Name + (idx > 0 ? strlen("VanGuiTableFlags") : 0) : "";
    if (VanGui::BeginCombo("Sizing Policy", preview_text))
    {
        for (int n = 0; n < VAN_COUNTOF(policies); n++)
            if (VanGui::Selectable(policies[n].Name, idx == n))
                *p_flags = (*p_flags & ~VanGuiTableFlags_SizingMask_) | policies[n].Value;
        VanGui::EndCombo();
    }
    VanGui::SameLine();
    VanGui::TextDisabled("(?)");
    if (VanGui::BeginItemTooltip())
    {
        VanGui::PushTextWrapPos(VanGui::GetFontSize() * 50.0f);
        for (int m = 0; m < VAN_COUNTOF(policies); m++)
        {
            VanGui::Separator();
            VanGui::Text("%s:", policies[m].Name);
            VanGui::Separator();
            VanGui::SetCursorPosX(VanGui::GetCursorPosX() + VanGui::GetStyle().IndentSpacing * 0.5f);
            VanGui::TextUnformatted(policies[m].Tooltip);
        }
        VanGui::PopTextWrapPos();
        VanGui::EndTooltip();
    }
}

static void EditTableColumnsFlags(VanGuiTableColumnFlags* p_flags)
{
    VanGui::CheckboxFlags("_Disabled", p_flags, VanGuiTableColumnFlags_Disabled); VanGui::SameLine(); HelpMarker("Master disable flag (also hide from context menu)");
    VanGui::CheckboxFlags("_DefaultHide", p_flags, VanGuiTableColumnFlags_DefaultHide);
    VanGui::CheckboxFlags("_DefaultSort", p_flags, VanGuiTableColumnFlags_DefaultSort);
    if (VanGui::CheckboxFlags("_WidthStretch", p_flags, VanGuiTableColumnFlags_WidthStretch))
        *p_flags &= ~(VanGuiTableColumnFlags_WidthMask_ ^ VanGuiTableColumnFlags_WidthStretch);
    if (VanGui::CheckboxFlags("_WidthFixed", p_flags, VanGuiTableColumnFlags_WidthFixed))
        *p_flags &= ~(VanGuiTableColumnFlags_WidthMask_ ^ VanGuiTableColumnFlags_WidthFixed);
    VanGui::CheckboxFlags("_NoResize", p_flags, VanGuiTableColumnFlags_NoResize);
    VanGui::CheckboxFlags("_NoReorder", p_flags, VanGuiTableColumnFlags_NoReorder);
    VanGui::CheckboxFlags("_NoHide", p_flags, VanGuiTableColumnFlags_NoHide);
    VanGui::CheckboxFlags("_NoClip", p_flags, VanGuiTableColumnFlags_NoClip);
    VanGui::CheckboxFlags("_NoSort", p_flags, VanGuiTableColumnFlags_NoSort);
    VanGui::CheckboxFlags("_NoSortAscending", p_flags, VanGuiTableColumnFlags_NoSortAscending);
    VanGui::CheckboxFlags("_NoSortDescending", p_flags, VanGuiTableColumnFlags_NoSortDescending);
    VanGui::CheckboxFlags("_NoHeaderLabel", p_flags, VanGuiTableColumnFlags_NoHeaderLabel);
    VanGui::CheckboxFlags("_NoHeaderWidth", p_flags, VanGuiTableColumnFlags_NoHeaderWidth);
    VanGui::CheckboxFlags("_PreferSortAscending", p_flags, VanGuiTableColumnFlags_PreferSortAscending);
    VanGui::CheckboxFlags("_PreferSortDescending", p_flags, VanGuiTableColumnFlags_PreferSortDescending);
    VanGui::CheckboxFlags("_IndentEnable", p_flags, VanGuiTableColumnFlags_IndentEnable); VanGui::SameLine(); HelpMarker("Default for column 0");
    VanGui::CheckboxFlags("_IndentDisable", p_flags, VanGuiTableColumnFlags_IndentDisable); VanGui::SameLine(); HelpMarker("Default for column >0");
    VanGui::CheckboxFlags("_AngledHeader", p_flags, VanGuiTableColumnFlags_AngledHeader);
}

static void ShowTableColumnsStatusFlags(VanGuiTableColumnFlags flags)
{
    VanGui::CheckboxFlags("_IsEnabled", &flags, VanGuiTableColumnFlags_IsEnabled);
    VanGui::CheckboxFlags("_IsVisible", &flags, VanGuiTableColumnFlags_IsVisible);
    VanGui::CheckboxFlags("_IsSorted", &flags, VanGuiTableColumnFlags_IsSorted);
    VanGui::CheckboxFlags("_IsHovered", &flags, VanGuiTableColumnFlags_IsHovered);
}

//-----------------------------------------------------------------------------
// [SECTION] DemoWindowTables()
//-----------------------------------------------------------------------------

static void DemoWindowTables()
{
    //VanGui::SetNextItemOpen(true, VanGuiCond_Once);
    if (!VanGui::CollapsingHeader("Tables & Columns"))
        return;

    // Using those as a base value to create width/height that are factor of the size of our font
    const float TEXT_BASE_WIDTH = VanGui::CalcTextSize("A").x;
    const float TEXT_BASE_HEIGHT = VanGui::GetTextLineHeightWithSpacing();

    VanGui::PushID("Tables");

    int open_action = -1;
    if (VanGui::Button("Expand all"))
        open_action = 1;
    VanGui::SameLine();
    if (VanGui::Button("Collapse all"))
        open_action = 0;
    VanGui::SameLine();

    // Options
    static bool disable_indent = false;
    VanGui::Checkbox("Disable tree indentation", &disable_indent);
    VanGui::SameLine();
    HelpMarker("Disable the indenting of tree nodes so demo tables can use the full window width.");
    VanGui::Separator();
    if (disable_indent)
        VanGui::PushStyleVar(VanGuiStyleVar_IndentSpacing, 0.0f);

    // About Styling of tables
    // Most settings are configured on a per-table basis via the flags passed to BeginTable() and TableSetupColumns APIs.
    // There are however a few settings that a shared and part of the VanGuiStyle structure:
    //   style.CellPadding                          // Padding within each cell
    //   style.Colors[VanGuiCol_TableHeaderBg]       // Table header background
    //   style.Colors[VanGuiCol_TableBorderStrong]   // Table outer and header borders
    //   style.Colors[VanGuiCol_TableBorderLight]    // Table inner borders
    //   style.Colors[VanGuiCol_TableRowBg]          // Table row background when VanGuiTableFlags_RowBg is enabled (even rows)
    //   style.Colors[VanGuiCol_TableRowBgAlt]       // Table row background when VanGuiTableFlags_RowBg is enabled (odds rows)

    // Demos
    if (open_action != -1)
        VanGui::SetNextItemOpen(open_action != 0);
    if (VanGui::TreeNode("Basic"))
    {
        VANGUI_DEMO_MARKER("Tables/Basic");
        // Here we will showcase three different ways to output a table.
        // They are very simple variations of a same thing!

        // [Method 1] Using TableNextRow() to create a new row, and TableSetColumnIndex() to select the column.
        // In many situations, this is the most flexible and easy to use pattern.
        HelpMarker("Using TableNextRow() + calling TableSetColumnIndex() _before_ each cell, in a loop.");
        if (VanGui::BeginTable("table1", 3))
        {
            for (int row = 0; row < 4; row++)
            {
                VanGui::TableNextRow();
                for (int column = 0; column < 3; column++)
                {
                    VanGui::TableSetColumnIndex(column);
                    VanGui::Text("Row %d Column %d", row, column);
                }
            }
            VanGui::EndTable();
        }

        // [Method 2] Using TableNextColumn() called multiple times, instead of using a for loop + TableSetColumnIndex().
        // This is generally more convenient when you have code manually submitting the contents of each column.
        HelpMarker("Using TableNextRow() + calling TableNextColumn() _before_ each cell, manually.");
        if (VanGui::BeginTable("table2", 3))
        {
            for (int row = 0; row < 4; row++)
            {
                VanGui::TableNextRow();
                VanGui::TableNextColumn();
                VanGui::Text("Row %d", row);
                VanGui::TableNextColumn();
                VanGui::Text("Some contents");
                VanGui::TableNextColumn();
                VanGui::Text("123.456");
            }
            VanGui::EndTable();
        }

        // [Method 3] We call TableNextColumn() _before_ each cell. We never call TableNextRow(),
        // as TableNextColumn() will automatically wrap around and create new rows as needed.
        // This is generally more convenient when your cells all contains the same type of data.
        HelpMarker(
            "Only using TableNextColumn(), which tends to be convenient for tables where every cell contains "
            "the same type of contents.\n This is also more similar to the old NextColumn() function of the "
            "Columns API, and provided to facilitate the Columns->Tables API transition.");
        if (VanGui::BeginTable("table3", 3))
        {
            for (int item = 0; item < 14; item++)
            {
                VanGui::TableNextColumn();
                VanGui::Text("Item %d", item);
            }
            VanGui::EndTable();
        }

        VanGui::TreePop();
    }

    if (open_action != -1)
        VanGui::SetNextItemOpen(open_action != 0);
    if (VanGui::TreeNode("Borders, background"))
    {
        VANGUI_DEMO_MARKER("Tables/Borders, background");
        // Expose a few Borders related flags interactively
        enum ContentsType { CT_Text, CT_FillButton };
        static VanGuiTableFlags flags = VanGuiTableFlags_Borders | VanGuiTableFlags_RowBg;
        static bool display_headers = false;
        static int contents_type = CT_Text;

        PushStyleCompact();
        VanGui::CheckboxFlags("VanGuiTableFlags_RowBg", &flags, VanGuiTableFlags_RowBg);
        VanGui::CheckboxFlags("VanGuiTableFlags_Borders", &flags, VanGuiTableFlags_Borders);
        VanGui::SameLine(); HelpMarker("VanGuiTableFlags_Borders\n = VanGuiTableFlags_BordersInnerV\n | VanGuiTableFlags_BordersOuterV\n | VanGuiTableFlags_BordersInnerH\n | VanGuiTableFlags_BordersOuterH");
        VanGui::Indent();

        VanGui::CheckboxFlags("VanGuiTableFlags_BordersH", &flags, VanGuiTableFlags_BordersH);
        VanGui::Indent();
        VanGui::CheckboxFlags("VanGuiTableFlags_BordersOuterH", &flags, VanGuiTableFlags_BordersOuterH);
        VanGui::CheckboxFlags("VanGuiTableFlags_BordersInnerH", &flags, VanGuiTableFlags_BordersInnerH);
        VanGui::Unindent();

        VanGui::CheckboxFlags("VanGuiTableFlags_BordersV", &flags, VanGuiTableFlags_BordersV);
        VanGui::Indent();
        VanGui::CheckboxFlags("VanGuiTableFlags_BordersOuterV", &flags, VanGuiTableFlags_BordersOuterV);
        VanGui::CheckboxFlags("VanGuiTableFlags_BordersInnerV", &flags, VanGuiTableFlags_BordersInnerV);
        VanGui::Unindent();

        VanGui::CheckboxFlags("VanGuiTableFlags_BordersOuter", &flags, VanGuiTableFlags_BordersOuter);
        VanGui::CheckboxFlags("VanGuiTableFlags_BordersInner", &flags, VanGuiTableFlags_BordersInner);
        VanGui::Unindent();

        VanGui::AlignTextToFramePadding(); VanGui::Text("Cell contents:");
        VanGui::SameLine(); VanGui::RadioButton("Text", &contents_type, CT_Text);
        VanGui::SameLine(); VanGui::RadioButton("FillButton", &contents_type, CT_FillButton);
        VanGui::Checkbox("Display headers", &display_headers);
        VanGui::CheckboxFlags("VanGuiTableFlags_NoBordersInBody", &flags, VanGuiTableFlags_NoBordersInBody); VanGui::SameLine(); HelpMarker("Disable vertical borders in columns Body (borders will always appear in Headers)");
        PopStyleCompact();

        if (VanGui::BeginTable("table1", 3, flags))
        {
            // Display headers so we can inspect their interaction with borders
            // (Headers are not the main purpose of this section of the demo, so we are not elaborating on them now. See other sections for details)
            if (display_headers)
            {
                VanGui::TableSetupColumn("One");
                VanGui::TableSetupColumn("Two");
                VanGui::TableSetupColumn("Three");
                VanGui::TableHeadersRow();
            }

            for (int row = 0; row < 5; row++)
            {
                VanGui::TableNextRow();
                for (int column = 0; column < 3; column++)
                {
                    VanGui::TableSetColumnIndex(column);
                    char buf[32];
                    snprintf(buf, sizeof(buf), "Hello %d,%d", column, row);
                    if (contents_type == CT_Text)
                        VanGui::TextUnformatted(buf);
                    else if (contents_type == CT_FillButton)
                        VanGui::Button(buf, VanVec2(-FLT_MIN, 0.0f));
                }
            }
            VanGui::EndTable();
        }
        VanGui::TreePop();
    }

    if (open_action != -1)
        VanGui::SetNextItemOpen(open_action != 0);
    if (VanGui::TreeNode("Resizable, stretch"))
    {
        VANGUI_DEMO_MARKER("Tables/Resizable, stretch");
        // By default, if we don't enable ScrollX the sizing policy for each column is "Stretch"
        // All columns maintain a sizing weight, and they will occupy all available width.
        static VanGuiTableFlags flags = VanGuiTableFlags_SizingStretchSame | VanGuiTableFlags_Resizable | VanGuiTableFlags_BordersOuter | VanGuiTableFlags_BordersV | VanGuiTableFlags_ContextMenuInBody;
        PushStyleCompact();
        VanGui::CheckboxFlags("VanGuiTableFlags_Resizable", &flags, VanGuiTableFlags_Resizable);
        VanGui::CheckboxFlags("VanGuiTableFlags_BordersV", &flags, VanGuiTableFlags_BordersV);
        VanGui::SameLine(); HelpMarker(
            "Using the _Resizable flag automatically enables the _BordersInnerV flag as well, "
            "this is why the resize borders are still showing when unchecking this.");
        PopStyleCompact();

        if (VanGui::BeginTable("table1", 3, flags))
        {
            for (int row = 0; row < 5; row++)
            {
                VanGui::TableNextRow();
                for (int column = 0; column < 3; column++)
                {
                    VanGui::TableSetColumnIndex(column);
                    VanGui::Text("Hello %d,%d", column, row);
                }
            }
            VanGui::EndTable();
        }
        VanGui::TreePop();
    }

    if (open_action != -1)
        VanGui::SetNextItemOpen(open_action != 0);
    if (VanGui::TreeNode("Resizable, fixed"))
    {
        VANGUI_DEMO_MARKER("Tables/Resizable, fixed");
        // Here we use VanGuiTableFlags_SizingFixedFit (even though _ScrollX is not set)
        // So columns will adopt the "Fixed" policy and will maintain a fixed width regardless of the whole available width (unless table is small)
        // If there is not enough available width to fit all columns, they will however be resized down.
        // FIXME-TABLE: Providing a stretch-on-init would make sense especially for tables which don't have saved settings
        HelpMarker(
            "Using _Resizable + _SizingFixedFit flags.\n"
            "Fixed-width columns generally makes more sense if you want to use horizontal scrolling.\n\n"
            "Double-click a column border to auto-fit the column to its contents.");
        PushStyleCompact();
        static VanGuiTableFlags flags = VanGuiTableFlags_SizingFixedFit | VanGuiTableFlags_Resizable | VanGuiTableFlags_BordersOuter | VanGuiTableFlags_BordersV | VanGuiTableFlags_ContextMenuInBody;
        VanGui::CheckboxFlags("VanGuiTableFlags_NoHostExtendX", &flags, VanGuiTableFlags_NoHostExtendX);
        PopStyleCompact();

        if (VanGui::BeginTable("table1", 3, flags))
        {
            for (int row = 0; row < 5; row++)
            {
                VanGui::TableNextRow();
                for (int column = 0; column < 3; column++)
                {
                    VanGui::TableSetColumnIndex(column);
                    VanGui::Text("Hello %d,%d", column, row);
                }
            }
            VanGui::EndTable();
        }
        VanGui::TreePop();
    }

    if (open_action != -1)
        VanGui::SetNextItemOpen(open_action != 0);
    if (VanGui::TreeNode("Resizable, mixed"))
    {
        VANGUI_DEMO_MARKER("Tables/Resizable, mixed");
        HelpMarker(
            "Using TableSetupColumn() to alter resizing policy on a per-column basis.\n\n"
            "When combining Fixed and Stretch columns, generally you only want one, maybe two trailing columns to use _WidthStretch.");
        static VanGuiTableFlags flags = VanGuiTableFlags_SizingFixedFit | VanGuiTableFlags_RowBg | VanGuiTableFlags_Borders | VanGuiTableFlags_Resizable | VanGuiTableFlags_Reorderable | VanGuiTableFlags_Hideable;

        if (VanGui::BeginTable("table1", 3, flags))
        {
            VanGui::TableSetupColumn("AAA", VanGuiTableColumnFlags_WidthFixed);
            VanGui::TableSetupColumn("BBB", VanGuiTableColumnFlags_WidthFixed);
            VanGui::TableSetupColumn("CCC", VanGuiTableColumnFlags_WidthStretch);
            VanGui::TableHeadersRow();
            for (int row = 0; row < 5; row++)
            {
                VanGui::TableNextRow();
                for (int column = 0; column < 3; column++)
                {
                    VanGui::TableSetColumnIndex(column);
                    VanGui::Text("%s %d,%d", (column == 2) ? "Stretch" : "Fixed", column, row);
                }
            }
            VanGui::EndTable();
        }
        if (VanGui::BeginTable("table2", 6, flags))
        {
            VanGui::TableSetupColumn("AAA", VanGuiTableColumnFlags_WidthFixed);
            VanGui::TableSetupColumn("BBB", VanGuiTableColumnFlags_WidthFixed);
            VanGui::TableSetupColumn("CCC", VanGuiTableColumnFlags_WidthFixed | VanGuiTableColumnFlags_DefaultHide);
            VanGui::TableSetupColumn("DDD", VanGuiTableColumnFlags_WidthStretch);
            VanGui::TableSetupColumn("EEE", VanGuiTableColumnFlags_WidthStretch);
            VanGui::TableSetupColumn("FFF", VanGuiTableColumnFlags_WidthStretch | VanGuiTableColumnFlags_DefaultHide);
            VanGui::TableHeadersRow();
            for (int row = 0; row < 5; row++)
            {
                VanGui::TableNextRow();
                for (int column = 0; column < 6; column++)
                {
                    VanGui::TableSetColumnIndex(column);
                    VanGui::Text("%s %d,%d", (column >= 3) ? "Stretch" : "Fixed", column, row);
                }
            }
            VanGui::EndTable();
        }
        VanGui::TreePop();
    }

    if (open_action != -1)
        VanGui::SetNextItemOpen(open_action != 0);
    if (VanGui::TreeNode("Reorderable, hideable, with headers"))
    {
        VANGUI_DEMO_MARKER("Tables/Reorderable, hideable, with headers");
        HelpMarker(
            "Click and drag column headers to reorder columns.\n\n"
            "Right-click on a header to open a context menu.");
        static VanGuiTableFlags flags = VanGuiTableFlags_Resizable | VanGuiTableFlags_Reorderable | VanGuiTableFlags_Hideable | VanGuiTableFlags_BordersOuter | VanGuiTableFlags_BordersV;
        PushStyleCompact();
        VanGui::CheckboxFlags("VanGuiTableFlags_Resizable", &flags, VanGuiTableFlags_Resizable);
        VanGui::CheckboxFlags("VanGuiTableFlags_Reorderable", &flags, VanGuiTableFlags_Reorderable);
        VanGui::CheckboxFlags("VanGuiTableFlags_Hideable", &flags, VanGuiTableFlags_Hideable);
        VanGui::CheckboxFlags("VanGuiTableFlags_NoBordersInBody", &flags, VanGuiTableFlags_NoBordersInBody);
        VanGui::CheckboxFlags("VanGuiTableFlags_NoBordersInBodyUntilResize", &flags, VanGuiTableFlags_NoBordersInBodyUntilResize); VanGui::SameLine(); HelpMarker("Disable vertical borders in columns Body until hovered for resize (borders will always appear in Headers)");
        VanGui::CheckboxFlags("VanGuiTableFlags_HighlightHoveredColumn", &flags, VanGuiTableFlags_HighlightHoveredColumn);
        PopStyleCompact();

        if (VanGui::BeginTable("table1", 3, flags))
        {
            // Submit columns name with TableSetupColumn() and call TableHeadersRow() to create a row with a header in each column.
            // (Later we will show how TableSetupColumn() has other uses, optional flags, sizing weight etc.)
            VanGui::TableSetupColumn("One");
            VanGui::TableSetupColumn("Two");
            VanGui::TableSetupColumn("Three");
            VanGui::TableHeadersRow();
            for (int row = 0; row < 6; row++)
            {
                VanGui::TableNextRow();
                for (int column = 0; column < 3; column++)
                {
                    VanGui::TableSetColumnIndex(column);
                    VanGui::Text("Hello %d,%d", column, row);
                }
            }
            VanGui::EndTable();
        }

        // Use outer_size.x == 0.0f instead of default to make the table as tight as possible
        // (only valid when no scrolling and no stretch column)
        if (VanGui::BeginTable("table2", 3, flags | VanGuiTableFlags_SizingFixedFit, VanVec2(0.0f, 0.0f)))
        {
            VanGui::TableSetupColumn("One");
            VanGui::TableSetupColumn("Two");
            VanGui::TableSetupColumn("Three");
            VanGui::TableHeadersRow();
            for (int row = 0; row < 6; row++)
            {
                VanGui::TableNextRow();
                for (int column = 0; column < 3; column++)
                {
                    VanGui::TableSetColumnIndex(column);
                    VanGui::Text("Fixed %d,%d", column, row);
                }
            }
            VanGui::EndTable();
        }
        VanGui::TreePop();
    }

    if (open_action != -1)
        VanGui::SetNextItemOpen(open_action != 0);
    if (VanGui::TreeNode("Padding"))
    {
        VANGUI_DEMO_MARKER("Tables/Padding");
        // First example: showcase use of padding flags and effect of BorderOuterV/BorderInnerV on X padding.
        // We don't expose BorderOuterH/BorderInnerH here because they have no effect on X padding.
        HelpMarker(
            "We often want outer padding activated when any using features which makes the edges of a column visible:\n"
            "e.g.:\n"
            "- BorderOuterV\n"
            "- any form of row selection\n"
            "Because of this, activating BorderOuterV sets the default to PadOuterX. "
            "Using PadOuterX or NoPadOuterX you can override the default.\n\n"
            "Actual padding values are using style.CellPadding.\n\n"
            "In this demo we don't show horizontal borders to emphasize how they don't affect default horizontal padding.");

        static VanGuiTableFlags flags1 = VanGuiTableFlags_BordersV;
        PushStyleCompact();
        VanGui::CheckboxFlags("VanGuiTableFlags_PadOuterX", &flags1, VanGuiTableFlags_PadOuterX);
        VanGui::SameLine(); HelpMarker("Enable outer-most padding (default if VanGuiTableFlags_BordersOuterV is set)");
        VanGui::CheckboxFlags("VanGuiTableFlags_NoPadOuterX", &flags1, VanGuiTableFlags_NoPadOuterX);
        VanGui::SameLine(); HelpMarker("Disable outer-most padding (default if VanGuiTableFlags_BordersOuterV is not set)");
        VanGui::CheckboxFlags("VanGuiTableFlags_NoPadInnerX", &flags1, VanGuiTableFlags_NoPadInnerX);
        VanGui::SameLine(); HelpMarker("Disable inner padding between columns (double inner padding if BordersOuterV is on, single inner padding if BordersOuterV is off)");
        VanGui::CheckboxFlags("VanGuiTableFlags_BordersOuterV", &flags1, VanGuiTableFlags_BordersOuterV);
        VanGui::CheckboxFlags("VanGuiTableFlags_BordersInnerV", &flags1, VanGuiTableFlags_BordersInnerV);
        static bool show_headers = false;
        VanGui::Checkbox("show_headers", &show_headers);
        PopStyleCompact();

        if (VanGui::BeginTable("table_padding", 3, flags1))
        {
            if (show_headers)
            {
                VanGui::TableSetupColumn("One");
                VanGui::TableSetupColumn("Two");
                VanGui::TableSetupColumn("Three");
                VanGui::TableHeadersRow();
            }

            for (int row = 0; row < 5; row++)
            {
                VanGui::TableNextRow();
                for (int column = 0; column < 3; column++)
                {
                    VanGui::TableSetColumnIndex(column);
                    if (row == 0)
                    {
                        VanGui::Text("Avail %.2f", VanGui::GetContentRegionAvail().x);
                    }
                    else
                    {
                        char buf[32];
                        snprintf(buf, sizeof(buf), "Hello %d,%d", column, row);
                        VanGui::Button(buf, VanVec2(-FLT_MIN, 0.0f));
                    }
                    //if (VanGui::TableGetColumnFlags() & VanGuiTableColumnFlags_IsHovered)
                    //    VanGui::TableSetBgColor(VanGuiTableBgTarget_CellBg, VAN_COL32(0, 100, 0, 255));
                }
            }
            VanGui::EndTable();
        }

        // Second example: set style.CellPadding to (0.0) or a custom value.
        // FIXME-TABLE: Vertical border effectively not displayed the same way as horizontal one...
        HelpMarker("Setting style.CellPadding to (0,0) or a custom value.");
        static VanGuiTableFlags flags2 = VanGuiTableFlags_Borders | VanGuiTableFlags_RowBg;
        static VanVec2 cell_padding(0.0f, 0.0f);
        static bool show_widget_frame_bg = true;

        PushStyleCompact();
        VanGui::CheckboxFlags("VanGuiTableFlags_Borders", &flags2, VanGuiTableFlags_Borders);
        VanGui::CheckboxFlags("VanGuiTableFlags_BordersH", &flags2, VanGuiTableFlags_BordersH);
        VanGui::CheckboxFlags("VanGuiTableFlags_BordersV", &flags2, VanGuiTableFlags_BordersV);
        VanGui::CheckboxFlags("VanGuiTableFlags_BordersInner", &flags2, VanGuiTableFlags_BordersInner);
        VanGui::CheckboxFlags("VanGuiTableFlags_BordersOuter", &flags2, VanGuiTableFlags_BordersOuter);
        VanGui::CheckboxFlags("VanGuiTableFlags_RowBg", &flags2, VanGuiTableFlags_RowBg);
        VanGui::CheckboxFlags("VanGuiTableFlags_Resizable", &flags2, VanGuiTableFlags_Resizable);
        VanGui::Checkbox("show_widget_frame_bg", &show_widget_frame_bg);
        VanGui::SliderFloat2("CellPadding", &cell_padding.x, 0.0f, 10.0f, "%.0f");
        PopStyleCompact();

        VanGui::PushStyleVar(VanGuiStyleVar_CellPadding, cell_padding);
        if (VanGui::BeginTable("table_padding_2", 3, flags2))
        {
            static char text_bufs[3 * 5][16]; // Mini text storage for 3x5 cells
            static bool init = true;
            if (!show_widget_frame_bg)
                VanGui::PushStyleColor(VanGuiCol_FrameBg, 0);
            for (int cell = 0; cell < 3 * 5; cell++)
            {
                VanGui::TableNextColumn();
                if (init)
                    strcpy(text_bufs[cell], "edit me");
                VanGui::SetNextItemWidth(-FLT_MIN);
                VanGui::PushID(cell);
                VanGui::InputText("##cell", text_bufs[cell], VAN_COUNTOF(text_bufs[cell]));
                VanGui::PopID();
            }
            if (!show_widget_frame_bg)
                VanGui::PopStyleColor();
            init = false;
            VanGui::EndTable();
        }
        VanGui::PopStyleVar();

        VanGui::TreePop();
    }

    if (open_action != -1)
        VanGui::SetNextItemOpen(open_action != 0);
    if (VanGui::TreeNode("Sizing policies"))
    {
        VANGUI_DEMO_MARKER("Tables/Explicit widths");
        static VanGuiTableFlags flags1 = VanGuiTableFlags_BordersV | VanGuiTableFlags_BordersOuterH | VanGuiTableFlags_RowBg | VanGuiTableFlags_ContextMenuInBody;
        PushStyleCompact();
        VanGui::CheckboxFlags("VanGuiTableFlags_Resizable", &flags1, VanGuiTableFlags_Resizable);
        VanGui::CheckboxFlags("VanGuiTableFlags_NoHostExtendX", &flags1, VanGuiTableFlags_NoHostExtendX);
        PopStyleCompact();

        static VanGuiTableFlags sizing_policy_flags[4] = { VanGuiTableFlags_SizingFixedFit, VanGuiTableFlags_SizingFixedSame, VanGuiTableFlags_SizingStretchProp, VanGuiTableFlags_SizingStretchSame };
        for (int table_n = 0; table_n < 4; table_n++)
        {
            VanGui::PushID(table_n);
            VanGui::SetNextItemWidth(TEXT_BASE_WIDTH * 30);
            EditTableSizingFlags(&sizing_policy_flags[table_n]);

            // To make it easier to understand the different sizing policy,
            // For each policy: we display one table where the columns have equal contents width,
            // and one where the columns have different contents width.
            if (VanGui::BeginTable("table1", 3, sizing_policy_flags[table_n] | flags1))
            {
                for (int row = 0; row < 3; row++)
                {
                    VanGui::TableNextRow();
                    VanGui::TableNextColumn(); VanGui::Text("Oh dear");
                    VanGui::TableNextColumn(); VanGui::Text("Oh dear");
                    VanGui::TableNextColumn(); VanGui::Text("Oh dear");
                }
                VanGui::EndTable();
            }
            if (VanGui::BeginTable("table2", 3, sizing_policy_flags[table_n] | flags1))
            {
                for (int row = 0; row < 3; row++)
                {
                    VanGui::TableNextRow();
                    VanGui::TableNextColumn(); VanGui::Text("AAAA");
                    VanGui::TableNextColumn(); VanGui::Text("BBBBBBBB");
                    VanGui::TableNextColumn(); VanGui::Text("CCCCCCCCCCCC");
                }
                VanGui::EndTable();
            }
            VanGui::PopID();
        }

        VanGui::Spacing();
        VanGui::TextUnformatted("Advanced");
        VanGui::SameLine();
        HelpMarker(
            "This section allows you to interact and see the effect of various sizing policies "
            "depending on whether Scroll is enabled and the contents of your columns.");

        enum ContentsType { CT_ShowWidth, CT_ShortText, CT_LongText, CT_Button, CT_FillButton, CT_InputText };
        static VanGuiTableFlags flags = VanGuiTableFlags_ScrollY | VanGuiTableFlags_Borders | VanGuiTableFlags_RowBg | VanGuiTableFlags_Resizable;
        static int contents_type = CT_ShowWidth;
        static int column_count = 3;

        PushStyleCompact();
        VanGui::PushID("Advanced");
        VanGui::PushItemWidth(TEXT_BASE_WIDTH * 30);
        EditTableSizingFlags(&flags);
        VanGui::Combo("Contents", &contents_type, "Show width\0Short Text\0Long Text\0Button\0Fill Button\0InputText\0");
        if (contents_type == CT_FillButton)
        {
            VanGui::SameLine();
            HelpMarker(
                "Be mindful that using right-alignment (e.g. size.x = -FLT_MIN) creates a feedback loop "
                "where contents width can feed into auto-column width can feed into contents width.");
        }
        VanGui::DragInt("Columns", &column_count, 0.1f, 1, 64, "%d", VanGuiSliderFlags_AlwaysClamp);
        VanGui::CheckboxFlags("VanGuiTableFlags_Resizable", &flags, VanGuiTableFlags_Resizable);
        VanGui::CheckboxFlags("VanGuiTableFlags_PreciseWidths", &flags, VanGuiTableFlags_PreciseWidths);
        VanGui::SameLine(); HelpMarker("Disable distributing remainder width to stretched columns (width allocation on a 100-wide table with 3 columns: Without this flag: 33,33,34. With this flag: 33,33,33). With larger number of columns, resizing will appear to be less smooth.");
        VanGui::CheckboxFlags("VanGuiTableFlags_ScrollX", &flags, VanGuiTableFlags_ScrollX);
        VanGui::CheckboxFlags("VanGuiTableFlags_ScrollY", &flags, VanGuiTableFlags_ScrollY);
        VanGui::CheckboxFlags("VanGuiTableFlags_NoClip", &flags, VanGuiTableFlags_NoClip);
        VanGui::PopItemWidth();
        VanGui::PopID();
        PopStyleCompact();

        if (VanGui::BeginTable("table2", column_count, flags, VanVec2(0.0f, TEXT_BASE_HEIGHT * 7)))
        {
            for (int cell = 0; cell < 10 * column_count; cell++)
            {
                VanGui::TableNextColumn();
                int column = VanGui::TableGetColumnIndex();
                int row = VanGui::TableGetRowIndex();

                VanGui::PushID(cell);
                char label[32];
                static char text_buf[32] = "";
                snprintf(label, sizeof(label), "Hello %d,%d", column, row);
                switch (contents_type)
                {
                case CT_ShortText:  VanGui::TextUnformatted(label); break;
                case CT_LongText:   VanGui::Text("Some %s text %d,%d\nOver two lines..", column == 0 ? "long" : "longeeer", column, row); break;
                case CT_ShowWidth:  VanGui::Text("W: %.1f", VanGui::GetContentRegionAvail().x); break;
                case CT_Button:     VanGui::Button(label); break;
                case CT_FillButton: VanGui::Button(label, VanVec2(-FLT_MIN, 0.0f)); break;
                case CT_InputText:  VanGui::SetNextItemWidth(-FLT_MIN); VanGui::InputText("##", text_buf, VAN_COUNTOF(text_buf)); break;
                }
                VanGui::PopID();
            }
            VanGui::EndTable();
        }
        VanGui::TreePop();
    }

    if (open_action != -1)
        VanGui::SetNextItemOpen(open_action != 0);
    if (VanGui::TreeNode("Vertical scrolling, with clipping"))
    {
        VANGUI_DEMO_MARKER("Tables/Vertical scrolling, with clipping");
        HelpMarker(
            "Here we activate ScrollY, which will create a child window container to allow hosting scrollable contents.\n\n"
            "We also demonstrate using VanGuiListClipper to virtualize the submission of many items.");
        static VanGuiTableFlags flags = VanGuiTableFlags_ScrollY | VanGuiTableFlags_RowBg | VanGuiTableFlags_BordersOuter | VanGuiTableFlags_BordersV | VanGuiTableFlags_Resizable | VanGuiTableFlags_Reorderable | VanGuiTableFlags_Hideable;

        PushStyleCompact();
        VanGui::CheckboxFlags("VanGuiTableFlags_ScrollY", &flags, VanGuiTableFlags_ScrollY);
        PopStyleCompact();

        // When using ScrollX or ScrollY we need to specify a size for our table container!
        // Otherwise by default the table will fit all available space, like a BeginChild() call.
        VanVec2 outer_size = VanVec2(0.0f, TEXT_BASE_HEIGHT * 8);
        if (VanGui::BeginTable("table_scrolly", 3, flags, outer_size))
        {
            VanGui::TableSetupScrollFreeze(0, 1); // Make top row always visible
            VanGui::TableSetupColumn("One", VanGuiTableColumnFlags_None);
            VanGui::TableSetupColumn("Two", VanGuiTableColumnFlags_None);
            VanGui::TableSetupColumn("Three", VanGuiTableColumnFlags_None);
            VanGui::TableHeadersRow();

            // Demonstrate using clipper for large vertical lists
            VanGuiListClipper clipper;
            clipper.Begin(1000);
            while (clipper.Step())
            {
                for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++)
                {
                    VanGui::TableNextRow();
                    for (int column = 0; column < 3; column++)
                    {
                        VanGui::TableSetColumnIndex(column);
                        VanGui::Text("Hello %d,%d", column, row);
                    }
                }
            }
            VanGui::EndTable();
        }
        VanGui::TreePop();
    }

    if (open_action != -1)
        VanGui::SetNextItemOpen(open_action != 0);
    if (VanGui::TreeNode("Horizontal scrolling"))
    {
        VANGUI_DEMO_MARKER("Tables/Horizontal scrolling");
        HelpMarker(
            "When ScrollX is enabled, the default sizing policy becomes VanGuiTableFlags_SizingFixedFit, "
            "as automatically stretching columns doesn't make much sense with horizontal scrolling.\n\n"
            "Also note that as of the current version, you will almost always want to enable ScrollY along with ScrollX, "
            "because the container window won't automatically extend vertically to fix contents "
            "(this may be improved in future versions).");
        static VanGuiTableFlags flags = VanGuiTableFlags_ScrollX | VanGuiTableFlags_ScrollY | VanGuiTableFlags_RowBg | VanGuiTableFlags_BordersOuter | VanGuiTableFlags_BordersV | VanGuiTableFlags_Resizable | VanGuiTableFlags_Reorderable | VanGuiTableFlags_Hideable;
        static int freeze_cols = 1;
        static int freeze_rows = 1;

        PushStyleCompact();
        VanGui::CheckboxFlags("VanGuiTableFlags_Resizable", &flags, VanGuiTableFlags_Resizable);
        VanGui::CheckboxFlags("VanGuiTableFlags_ScrollX", &flags, VanGuiTableFlags_ScrollX);
        VanGui::CheckboxFlags("VanGuiTableFlags_ScrollY", &flags, VanGuiTableFlags_ScrollY);
        VanGui::SetNextItemWidth(VanGui::GetFrameHeight());
        VanGui::DragInt("freeze_cols", &freeze_cols, 0.2f, 0, 9, nullptr, VanGuiSliderFlags_NoInput);
        VanGui::SetNextItemWidth(VanGui::GetFrameHeight());
        VanGui::DragInt("freeze_rows", &freeze_rows, 0.2f, 0, 9, nullptr, VanGuiSliderFlags_NoInput);
        PopStyleCompact();

        // When using ScrollX or ScrollY we need to specify a size for our table container!
        // Otherwise by default the table will fit all available space, like a BeginChild() call.
        VanVec2 outer_size = VanVec2(0.0f, TEXT_BASE_HEIGHT * 8);
        if (VanGui::BeginTable("table_scrollx", 7, flags, outer_size))
        {
            VanGui::TableSetupScrollFreeze(freeze_cols, freeze_rows);
            VanGui::TableSetupColumn("Line #", VanGuiTableColumnFlags_NoHide); // Make the first column not hideable to match our use of TableSetupScrollFreeze()
            VanGui::TableSetupColumn("One");
            VanGui::TableSetupColumn("Two");
            VanGui::TableSetupColumn("Three");
            VanGui::TableSetupColumn("Four");
            VanGui::TableSetupColumn("Five");
            VanGui::TableSetupColumn("Six");
            VanGui::TableHeadersRow();
            for (int row = 0; row < 20; row++)
            {
                VanGui::TableNextRow();
                for (int column = 0; column < 7; column++)
                {
                    // Both TableNextColumn() and TableSetColumnIndex() return true when a column is visible or performing width measurement.
                    // Because here we know that:
                    // - A) all our columns are contributing the same to row height
                    // - B) column 0 is always visible,
                    // We only always submit this one column and can skip others.
                    // More advanced per-column clipping behaviors may benefit from polling the status flags via TableGetColumnFlags().
                    if (!VanGui::TableSetColumnIndex(column) && column > 0)
                        continue;
                    if (column == 0)
                        VanGui::Text("Line %d", row);
                    else
                        VanGui::Text("Hello world %d,%d", column, row);
                }
            }
            VanGui::EndTable();
        }

        VanGui::Spacing();
        VanGui::TextUnformatted("Stretch + ScrollX");
        VanGui::SameLine();
        HelpMarker(
            "Showcase using Stretch columns + ScrollX together: "
            "this is rather unusual and only makes sense when specifying an 'inner_width' for the table!\n"
            "Without an explicit value, inner_width is == outer_size.x and therefore using Stretch columns "
            "along with ScrollX doesn't make sense.");
        static VanGuiTableFlags flags2 = VanGuiTableFlags_SizingStretchSame | VanGuiTableFlags_ScrollX | VanGuiTableFlags_ScrollY | VanGuiTableFlags_BordersOuter | VanGuiTableFlags_RowBg | VanGuiTableFlags_ContextMenuInBody;
        static float inner_width = 1000.0f;
        PushStyleCompact();
        VanGui::PushID("flags3");
        VanGui::PushItemWidth(TEXT_BASE_WIDTH * 30);
        VanGui::CheckboxFlags("VanGuiTableFlags_ScrollX", &flags2, VanGuiTableFlags_ScrollX);
        VanGui::DragFloat("inner_width", &inner_width, 1.0f, 0.0f, FLT_MAX, "%.1f");
        VanGui::PopItemWidth();
        VanGui::PopID();
        PopStyleCompact();
        if (VanGui::BeginTable("table2", 7, flags2, outer_size, inner_width))
        {
            for (int cell = 0; cell < 20 * 7; cell++)
            {
                VanGui::TableNextColumn();
                VanGui::Text("Hello world %d,%d", VanGui::TableGetColumnIndex(), VanGui::TableGetRowIndex());
            }
            VanGui::EndTable();
        }
        VanGui::TreePop();
    }

    if (open_action != -1)
        VanGui::SetNextItemOpen(open_action != 0);
    if (VanGui::TreeNode("Columns flags"))
    {
        VANGUI_DEMO_MARKER("Tables/Columns flags");
        // Create a first table just to show all the options/flags we want to make visible in our example!
        const int column_count = 3;
        const char* column_names[column_count] = { "One", "Two", "Three" };
        static VanGuiTableColumnFlags column_flags[column_count] = { VanGuiTableColumnFlags_DefaultSort, VanGuiTableColumnFlags_None, VanGuiTableColumnFlags_DefaultHide };
        static VanGuiTableColumnFlags column_flags_out[column_count] = { 0, 0, 0 }; // Output from TableGetColumnFlags()

        if (VanGui::BeginTable("table_columns_flags_checkboxes", column_count, VanGuiTableFlags_None))
        {
            PushStyleCompact();
            for (int column = 0; column < column_count; column++)
            {
                VanGui::TableNextColumn();
                VanGui::PushID(column);
                VanGui::AlignTextToFramePadding(); // FIXME-TABLE: Workaround for wrong text baseline propagation across columns
                VanGui::Text("'%s'", column_names[column]);
                VanGui::Spacing();
                VanGui::Text("Input flags:");
                EditTableColumnsFlags(&column_flags[column]);
                VanGui::Spacing();
                VanGui::Text("Output flags:");
                VanGui::BeginDisabled();
                ShowTableColumnsStatusFlags(column_flags_out[column]);
                VanGui::EndDisabled();
                VanGui::PopID();
            }
            PopStyleCompact();
            VanGui::EndTable();
        }

        // Create the real table we care about for the example!
        // We use a scrolling table to be able to showcase the difference between the _IsEnabled and _IsVisible flags above,
        // otherwise in a non-scrolling table columns are always visible (unless using VanGuiTableFlags_NoKeepColumnsVisible
        // + resizing the parent window down).
        const VanGuiTableFlags flags
            = VanGuiTableFlags_SizingFixedFit | VanGuiTableFlags_ScrollX | VanGuiTableFlags_ScrollY
            | VanGuiTableFlags_RowBg | VanGuiTableFlags_BordersOuter | VanGuiTableFlags_BordersV
            | VanGuiTableFlags_Resizable | VanGuiTableFlags_Reorderable | VanGuiTableFlags_Hideable | VanGuiTableFlags_Sortable;
        VanVec2 outer_size = VanVec2(0.0f, TEXT_BASE_HEIGHT * 9);
        if (VanGui::BeginTable("table_columns_flags", column_count, flags, outer_size))
        {
            bool has_angled_header = false;
            for (int column = 0; column < column_count; column++)
            {
                has_angled_header |= (column_flags[column] & VanGuiTableColumnFlags_AngledHeader) != 0;
                VanGui::TableSetupColumn(column_names[column], column_flags[column]);
            }
            if (has_angled_header)
                VanGui::TableAngledHeadersRow();
            VanGui::TableHeadersRow();
            for (int column = 0; column < column_count; column++)
                column_flags_out[column] = VanGui::TableGetColumnFlags(column);
            float indent_step = static_cast<float>(static_cast<int>(TEXT_BASE_WIDTH) / 2);
            for (int row = 0; row < 8; row++)
            {
                // Add some indentation to demonstrate usage of per-column IndentEnable/IndentDisable flags.
                VanGui::Indent(indent_step);
                VanGui::TableNextRow();
                for (int column = 0; column < column_count; column++)
                {
                    VanGui::TableSetColumnIndex(column);
                    VanGui::Text("%s %s", (column == 0) ? "Indented" : "Hello", VanGui::TableGetColumnName(column));
                }
            }
            VanGui::Unindent(indent_step * 8.0f);

            VanGui::EndTable();
        }
        VanGui::TreePop();
    }

    if (open_action != -1)
        VanGui::SetNextItemOpen(open_action != 0);
    if (VanGui::TreeNode("Columns widths"))
    {
        VANGUI_DEMO_MARKER("Tables/Columns widths");
        HelpMarker("Using TableSetupColumn() to setup default width.");

        static VanGuiTableFlags flags1 = VanGuiTableFlags_Borders | VanGuiTableFlags_NoBordersInBodyUntilResize;
        PushStyleCompact();
        VanGui::CheckboxFlags("VanGuiTableFlags_Resizable", &flags1, VanGuiTableFlags_Resizable);
        VanGui::CheckboxFlags("VanGuiTableFlags_NoBordersInBodyUntilResize", &flags1, VanGuiTableFlags_NoBordersInBodyUntilResize);
        PopStyleCompact();
        if (VanGui::BeginTable("table1", 3, flags1))
        {
            // We could also set VanGuiTableFlags_SizingFixedFit on the table and all columns will default to VanGuiTableColumnFlags_WidthFixed.
            VanGui::TableSetupColumn("one", VanGuiTableColumnFlags_WidthFixed, 100.0f); // Default to 100.0f
            VanGui::TableSetupColumn("two", VanGuiTableColumnFlags_WidthFixed, 200.0f); // Default to 200.0f
            VanGui::TableSetupColumn("three", VanGuiTableColumnFlags_WidthFixed);       // Default to auto
            VanGui::TableHeadersRow();
            for (int row = 0; row < 4; row++)
            {
                VanGui::TableNextRow();
                for (int column = 0; column < 3; column++)
                {
                    VanGui::TableSetColumnIndex(column);
                    if (row == 0)
                        VanGui::Text("(w: %5.1f)", VanGui::GetContentRegionAvail().x);
                    else
                        VanGui::Text("Hello %d,%d", column, row);
                }
            }
            VanGui::EndTable();
        }

        HelpMarker(
            "Using TableSetupColumn() to setup explicit width.\n\nUnless _NoKeepColumnsVisible is set, "
            "fixed columns with set width may still be shrunk down if there's not enough space in the host.");

        static VanGuiTableFlags flags2 = VanGuiTableFlags_None;
        PushStyleCompact();
        VanGui::CheckboxFlags("VanGuiTableFlags_NoKeepColumnsVisible", &flags2, VanGuiTableFlags_NoKeepColumnsVisible);
        VanGui::CheckboxFlags("VanGuiTableFlags_BordersInnerV", &flags2, VanGuiTableFlags_BordersInnerV);
        VanGui::CheckboxFlags("VanGuiTableFlags_BordersOuterV", &flags2, VanGuiTableFlags_BordersOuterV);
        PopStyleCompact();
        if (VanGui::BeginTable("table2", 4, flags2))
        {
            // We could also set VanGuiTableFlags_SizingFixedFit on the table and then all columns
            // will default to VanGuiTableColumnFlags_WidthFixed.
            VanGui::TableSetupColumn("", VanGuiTableColumnFlags_WidthFixed, 100.0f);
            VanGui::TableSetupColumn("", VanGuiTableColumnFlags_WidthFixed, TEXT_BASE_WIDTH * 15.0f);
            VanGui::TableSetupColumn("", VanGuiTableColumnFlags_WidthFixed, TEXT_BASE_WIDTH * 30.0f);
            VanGui::TableSetupColumn("", VanGuiTableColumnFlags_WidthFixed, TEXT_BASE_WIDTH * 15.0f);
            for (int row = 0; row < 5; row++)
            {
                VanGui::TableNextRow();
                for (int column = 0; column < 4; column++)
                {
                    VanGui::TableSetColumnIndex(column);
                    if (row == 0)
                        VanGui::Text("(w: %5.1f)", VanGui::GetContentRegionAvail().x);
                    else
                        VanGui::Text("Hello %d,%d", column, row);
                }
            }
            VanGui::EndTable();
        }
        VanGui::TreePop();
    }

    if (open_action != -1)
        VanGui::SetNextItemOpen(open_action != 0);
    if (VanGui::TreeNode("Nested tables"))
    {
        VANGUI_DEMO_MARKER("Tables/Nested tables");
        HelpMarker("This demonstrates embedding a table into another table cell.");

        if (VanGui::BeginTable("table_nested1", 2, VanGuiTableFlags_Borders | VanGuiTableFlags_Resizable | VanGuiTableFlags_Reorderable | VanGuiTableFlags_Hideable))
        {
            VanGui::TableSetupColumn("A0");
            VanGui::TableSetupColumn("A1");
            VanGui::TableHeadersRow();

            VanGui::TableNextColumn();
            VanGui::Text("A0 Row 0");
            {
                float rows_height = (TEXT_BASE_HEIGHT * 2.0f) + (VanGui::GetStyle().CellPadding.y * 2.0f);
                if (VanGui::BeginTable("table_nested2", 2, VanGuiTableFlags_Borders | VanGuiTableFlags_Resizable | VanGuiTableFlags_Reorderable | VanGuiTableFlags_Hideable))
                {
                    VanGui::TableSetupColumn("B0");
                    VanGui::TableSetupColumn("B1");
                    VanGui::TableHeadersRow();

                    VanGui::TableNextRow(VanGuiTableRowFlags_None, rows_height);
                    VanGui::TableNextColumn();
                    VanGui::Text("B0 Row 0");
                    VanGui::TableNextColumn();
                    VanGui::Text("B1 Row 0");
                    VanGui::TableNextRow(VanGuiTableRowFlags_None, rows_height);
                    VanGui::TableNextColumn();
                    VanGui::Text("B0 Row 1");
                    VanGui::TableNextColumn();
                    VanGui::Text("B1 Row 1");

                    VanGui::EndTable();
                }
            }
            VanGui::TableNextColumn(); VanGui::Text("A1 Row 0");
            VanGui::TableNextColumn(); VanGui::Text("A0 Row 1");
            VanGui::TableNextColumn(); VanGui::Text("A1 Row 1");
            VanGui::EndTable();
        }
        VanGui::TreePop();
    }

    if (open_action != -1)
        VanGui::SetNextItemOpen(open_action != 0);
    if (VanGui::TreeNode("Row height"))
    {
        VANGUI_DEMO_MARKER("Tables/Row height");
        HelpMarker(
            "You can pass a 'min_row_height' to TableNextRow().\n\nRows are padded with 'style.CellPadding.y' on top and bottom, "
            "so effectively the minimum row height will always be >= 'style.CellPadding.y * 2.0f'.\n\n"
            "We cannot honor a _maximum_ row height as that would require a unique clipping rectangle per row.");
        if (VanGui::BeginTable("table_row_height", 1, VanGuiTableFlags_Borders))
        {
            for (int row = 0; row < 8; row++)
            {
                float min_row_height = static_cast<float>(static_cast<int>(TEXT_BASE_HEIGHT * 0.30f * row + VanGui::GetStyle().CellPadding.y * 2.0f));
                VanGui::TableNextRow(VanGuiTableRowFlags_None, min_row_height);
                VanGui::TableNextColumn();
                VanGui::Text("min_row_height = %.2f", min_row_height);
            }
            VanGui::EndTable();
        }

        HelpMarker(
            "Showcase using SameLine(0,0) to share Current Line Height between cells.\n\n"
            "Please note that Tables Row Height is not the same thing as Current Line Height, "
            "as a table cell may contains multiple lines.");
        if (VanGui::BeginTable("table_share_lineheight", 2, VanGuiTableFlags_Borders))
        {
            VanGui::TableNextRow();
            VanGui::TableNextColumn();
            VanGui::ColorButton("##1", VanVec4(0.13f, 0.26f, 0.40f, 1.0f), VanGuiColorEditFlags_None, VanVec2(40, 40));
            VanGui::TableNextColumn();
            VanGui::Text("Line 1");
            VanGui::Text("Line 2");

            VanGui::TableNextRow();
            VanGui::TableNextColumn();
            VanGui::ColorButton("##2", VanVec4(0.13f, 0.26f, 0.40f, 1.0f), VanGuiColorEditFlags_None, VanVec2(40, 40));
            VanGui::TableNextColumn();
            VanGui::SameLine(0.0f, 0.0f); // Reuse line height from previous column
            VanGui::Text("Line 1, with SameLine(0,0)");
            VanGui::Text("Line 2");

            VanGui::EndTable();
        }

        HelpMarker("Showcase altering CellPadding.y between rows. Note that CellPadding.x is locked for the entire table.");
        if (VanGui::BeginTable("table_changing_cellpadding_y", 1, VanGuiTableFlags_Borders))
        {
            VanGuiStyle& style = VanGui::GetStyle();
            for (int row = 0; row < 8; row++)
            {
                if ((row % 3) == 2)
                    VanGui::PushStyleVarY(VanGuiStyleVar_CellPadding, 20.0f);
                VanGui::TableNextRow(VanGuiTableRowFlags_None);
                VanGui::TableNextColumn();
                VanGui::Text("CellPadding.y = %.2f", style.CellPadding.y);
                if ((row % 3) == 2)
                    VanGui::PopStyleVar();
            }
            VanGui::EndTable();
        }

        VanGui::TreePop();
    }

    if (open_action != -1)
        VanGui::SetNextItemOpen(open_action != 0);
    if (VanGui::TreeNode("Outer size"))
    {
        VANGUI_DEMO_MARKER("Tables/Outer size");
        // Showcasing use of VanGuiTableFlags_NoHostExtendX and VanGuiTableFlags_NoHostExtendY
        // Important to that note how the two flags have slightly different behaviors!
        VanGui::Text("Using NoHostExtendX and NoHostExtendY:");
        PushStyleCompact();
        static VanGuiTableFlags flags = VanGuiTableFlags_Borders | VanGuiTableFlags_Resizable | VanGuiTableFlags_ContextMenuInBody | VanGuiTableFlags_RowBg | VanGuiTableFlags_SizingFixedFit | VanGuiTableFlags_NoHostExtendX;
        VanGui::CheckboxFlags("VanGuiTableFlags_NoHostExtendX", &flags, VanGuiTableFlags_NoHostExtendX);
        VanGui::SameLine(); HelpMarker("Make outer width auto-fit to columns, overriding outer_size.x value.\n\nOnly available when ScrollX/ScrollY are disabled and Stretch columns are not used.");
        VanGui::CheckboxFlags("VanGuiTableFlags_NoHostExtendY", &flags, VanGuiTableFlags_NoHostExtendY);
        VanGui::SameLine(); HelpMarker("Make outer height stop exactly at outer_size.y (prevent auto-extending table past the limit).\n\nOnly available when ScrollX/ScrollY are disabled. Data below the limit will be clipped and not visible.");
        PopStyleCompact();

        VanVec2 outer_size = VanVec2(0.0f, TEXT_BASE_HEIGHT * 5.5f);
        if (VanGui::BeginTable("table1", 3, flags, outer_size))
        {
            for (int row = 0; row < 10; row++)
            {
                VanGui::TableNextRow();
                for (int column = 0; column < 3; column++)
                {
                    VanGui::TableNextColumn();
                    VanGui::Text("Cell %d,%d", column, row);
                }
            }
            VanGui::EndTable();
        }
        VanGui::SameLine();
        VanGui::Text("Hello!");

        VanGui::Spacing();

        VanGui::Text("Using explicit size:");
        if (VanGui::BeginTable("table2", 3, VanGuiTableFlags_Borders | VanGuiTableFlags_RowBg, VanVec2(TEXT_BASE_WIDTH * 30, 0.0f)))
        {
            for (int row = 0; row < 5; row++)
            {
                VanGui::TableNextRow();
                for (int column = 0; column < 3; column++)
                {
                    VanGui::TableNextColumn();
                    VanGui::Text("Cell %d,%d", column, row);
                }
            }
            VanGui::EndTable();
        }
        VanGui::SameLine();
        if (VanGui::BeginTable("table3", 3, VanGuiTableFlags_Borders | VanGuiTableFlags_RowBg, VanVec2(TEXT_BASE_WIDTH * 30, 0.0f)))
        {
            const float rows_height = TEXT_BASE_HEIGHT * 1.5f + VanGui::GetStyle().CellPadding.y * 2.0f;
            for (int row = 0; row < 3; row++)
            {
                VanGui::TableNextRow(0, rows_height);
                for (int column = 0; column < 3; column++)
                {
                    VanGui::TableNextColumn();
                    VanGui::Text("Cell %d,%d", column, row);
                }
            }
            VanGui::EndTable();
        }

        VanGui::TreePop();
    }

    if (open_action != -1)
        VanGui::SetNextItemOpen(open_action != 0);
    if (VanGui::TreeNode("Background color"))
    {
        VANGUI_DEMO_MARKER("Tables/Background color");
        static VanGuiTableFlags flags = VanGuiTableFlags_RowBg;
        static int row_bg_type = 1;
        static int row_bg_target = 1;
        static int cell_bg_type = 1;

        PushStyleCompact();
        VanGui::CheckboxFlags("VanGuiTableFlags_Borders", &flags, VanGuiTableFlags_Borders);
        VanGui::CheckboxFlags("VanGuiTableFlags_RowBg", &flags, VanGuiTableFlags_RowBg);
        VanGui::SameLine(); HelpMarker("VanGuiTableFlags_RowBg automatically sets RowBg0 to alternative colors pulled from the Style.");
        VanGui::Combo("row bg type", (int*)&row_bg_type, "None\0Red\0Gradient\0");
        VanGui::Combo("row bg target", (int*)&row_bg_target, "RowBg0\0RowBg1\0"); VanGui::SameLine(); HelpMarker("Target RowBg0 to override the alternating odd/even colors,\nTarget RowBg1 to blend with them.");
        VanGui::Combo("cell bg type", (int*)&cell_bg_type, "None\0Blue\0"); VanGui::SameLine(); HelpMarker("We are colorizing cells to B1->C2 here.");
        VAN_ASSERT(row_bg_type >= 0 && row_bg_type <= 2);
        VAN_ASSERT(row_bg_target >= 0 && row_bg_target <= 1);
        VAN_ASSERT(cell_bg_type >= 0 && cell_bg_type <= 1);
        PopStyleCompact();

        if (VanGui::BeginTable("table1", 5, flags))
        {
            for (int row = 0; row < 6; row++)
            {
                VanGui::TableNextRow();

                // Demonstrate setting a row background color with 'VanGui::TableSetBgColor(VanGuiTableBgTarget_RowBgX, ...)'
                // We use a transparent color so we can see the one behind in case our target is RowBg1 and RowBg0 was already targeted by the VanGuiTableFlags_RowBg flag.
                if (row_bg_type != 0)
                {
                    VanU32 row_bg_color = VanGui::GetColorU32(row_bg_type == 1 ? VanVec4(0.7f, 0.3f, 0.3f, 0.65f) : VanVec4(0.2f + row * 0.1f, 0.2f, 0.2f, 0.65f)); // Flat or Gradient?
                    VanGui::TableSetBgColor(VanGuiTableBgTarget_RowBg0 + row_bg_target, row_bg_color);
                }

                // Fill cells
                for (int column = 0; column < 5; column++)
                {
                    VanGui::TableSetColumnIndex(column);
                    VanGui::Text("%c%c", 'A' + row, '0' + column);

                    // Change background of Cells B1->C2
                    // Demonstrate setting a cell background color with 'VanGui::TableSetBgColor(VanGuiTableBgTarget_CellBg, ...)'
                    // (the CellBg color will be blended over the RowBg and ColumnBg colors)
                    // We can also pass a column number as a third parameter to TableSetBgColor() and do this outside the column loop.
                    if (row >= 1 && row <= 2 && column >= 1 && column <= 2 && cell_bg_type == 1)
                    {
                        VanU32 cell_bg_color = VanGui::GetColorU32(VanVec4(0.3f, 0.3f, 0.7f, 0.65f));
                        VanGui::TableSetBgColor(VanGuiTableBgTarget_CellBg, cell_bg_color);
                    }
                }
            }
            VanGui::EndTable();
        }
        VanGui::TreePop();
    }

    if (open_action != -1)
        VanGui::SetNextItemOpen(open_action != 0);
    if (VanGui::TreeNode("Tree view"))
    {
        VANGUI_DEMO_MARKER("Tables/Tree view");
        static VanGuiTableFlags table_flags = VanGuiTableFlags_BordersV | VanGuiTableFlags_BordersOuterH | VanGuiTableFlags_Resizable | VanGuiTableFlags_RowBg | VanGuiTableFlags_NoBordersInBody;

        static VanGuiTreeNodeFlags tree_node_flags_base = VanGuiTreeNodeFlags_SpanAllColumns | VanGuiTreeNodeFlags_DefaultOpen | VanGuiTreeNodeFlags_DrawLinesFull;
        VanGui::CheckboxFlags("VanGuiTreeNodeFlags_SpanFullWidth",  &tree_node_flags_base, VanGuiTreeNodeFlags_SpanFullWidth);
        VanGui::CheckboxFlags("VanGuiTreeNodeFlags_SpanLabelWidth",  &tree_node_flags_base, VanGuiTreeNodeFlags_SpanLabelWidth);
        VanGui::CheckboxFlags("VanGuiTreeNodeFlags_SpanAllColumns", &tree_node_flags_base, VanGuiTreeNodeFlags_SpanAllColumns);
        VanGui::CheckboxFlags("VanGuiTreeNodeFlags_LabelSpanAllColumns", &tree_node_flags_base, VanGuiTreeNodeFlags_LabelSpanAllColumns);
        VanGui::SameLine(); HelpMarker("Useful if you know that you aren't displaying contents in other columns");

        HelpMarker("See \"Columns flags\" section to configure how indentation is applied to individual columns.");
        if (VanGui::BeginTable("3ways", 3, table_flags))
        {
            // The first column will use the default _WidthStretch when ScrollX is Off and _WidthFixed when ScrollX is On
            VanGui::TableSetupColumn("Name", VanGuiTableColumnFlags_NoHide);
            VanGui::TableSetupColumn("Size", VanGuiTableColumnFlags_WidthFixed, TEXT_BASE_WIDTH * 12.0f);
            VanGui::TableSetupColumn("Type", VanGuiTableColumnFlags_WidthFixed, TEXT_BASE_WIDTH * 18.0f);
            VanGui::TableHeadersRow();

            // Simple storage to output a dummy file-system.
            struct MyTreeNode
            {
                const char*     Name;
                const char*     Type;
                int             Size;
                int             ChildIdx;
                int             ChildCount;
                static void DisplayNode(const MyTreeNode* node, const MyTreeNode* all_nodes)
                {
                    VanGui::TableNextRow();
                    VanGui::TableNextColumn();
                    const bool is_folder = (node->ChildCount > 0);

                    VanGuiTreeNodeFlags node_flags = tree_node_flags_base;
                    if (node != &all_nodes[0])
                        node_flags &= ~VanGuiTreeNodeFlags_LabelSpanAllColumns; // Only demonstrate this on the root node.

                    if (is_folder)
                    {
                        bool open = VanGui::TreeNodeEx(node->Name, node_flags);
                        if ((node_flags & VanGuiTreeNodeFlags_LabelSpanAllColumns) == 0)
                        {
                            VanGui::TableNextColumn();
                            VanGui::TextDisabled("--");
                            VanGui::TableNextColumn();
                            VanGui::TextUnformatted(node->Type);
                        }
                        if (open)
                        {
                            for (int child_n = 0; child_n < node->ChildCount; child_n++)
                                DisplayNode(&all_nodes[node->ChildIdx + child_n], all_nodes);
                            VanGui::TreePop();
                        }
                    }
                    else
                    {
                        VanGui::TreeNodeEx(node->Name, node_flags | VanGuiTreeNodeFlags_Leaf | VanGuiTreeNodeFlags_Bullet | VanGuiTreeNodeFlags_NoTreePushOnOpen);
                        VanGui::TableNextColumn();
                        VanGui::Text("%d", node->Size);
                        VanGui::TableNextColumn();
                        VanGui::TextUnformatted(node->Type);
                    }
                }
            };
            static const MyTreeNode nodes[] =
            {
                { "Root with Long Name",          "Folder",       -1,       1, 3    }, // 0
                { "Music",                        "Folder",       -1,       4, 2    }, // 1
                { "Textures",                     "Folder",       -1,       6, 3    }, // 2
                { "desktop.ini",                  "System file",  1024,    -1,-1    }, // 3
                { "File1_a.wav",                  "Audio file",   123000,  -1,-1    }, // 4
                { "File1_b.wav",                  "Audio file",   456000,  -1,-1    }, // 5
                { "Image001.png",                 "Image file",   203128,  -1,-1    }, // 6
                { "Copy of Image001.png",         "Image file",   203256,  -1,-1    }, // 7
                { "Copy of Image001 (Final2).png","Image file",   203512,  -1,-1    }, // 8
            };

            MyTreeNode::DisplayNode(&nodes[0], nodes);

            VanGui::EndTable();
        }
        VanGui::TreePop();
    }

    if (open_action != -1)
        VanGui::SetNextItemOpen(open_action != 0);
    if (VanGui::TreeNode("Item width"))
    {
        VANGUI_DEMO_MARKER("Tables/Item width");
        HelpMarker(
            "Showcase using PushItemWidth() and how it is preserved on a per-column basis.\n\n"
            "Note that on auto-resizing non-resizable fixed columns, querying the content width for "
            "e.g. right-alignment doesn't make sense.");
        if (VanGui::BeginTable("table_item_width", 3, VanGuiTableFlags_Borders))
        {
            VanGui::TableSetupColumn("small");
            VanGui::TableSetupColumn("half");
            VanGui::TableSetupColumn("right-align");
            VanGui::TableHeadersRow();

            for (int row = 0; row < 3; row++)
            {
                VanGui::TableNextRow();
                if (row == 0)
                {
                    // Setup ItemWidth once (instead of setting up every time, which is also possible but less efficient)
                    VanGui::TableSetColumnIndex(0);
                    VanGui::PushItemWidth(TEXT_BASE_WIDTH * 3.0f); // Small
                    VanGui::TableSetColumnIndex(1);
                    VanGui::PushItemWidth(-VanGui::GetContentRegionAvail().x * 0.5f);
                    VanGui::TableSetColumnIndex(2);
                    VanGui::PushItemWidth(-FLT_MIN); // Right-aligned
                }

                // Draw our contents
                static float dummy_f = 0.0f;
                VanGui::PushID(row);
                VanGui::TableSetColumnIndex(0);
                VanGui::SliderFloat("float0", &dummy_f, 0.0f, 1.0f);
                VanGui::TableSetColumnIndex(1);
                VanGui::SliderFloat("float1", &dummy_f, 0.0f, 1.0f);
                VanGui::TableSetColumnIndex(2);
                VanGui::SliderFloat("##float2", &dummy_f, 0.0f, 1.0f); // No visible label since right-aligned
                VanGui::PopID();
            }
            VanGui::EndTable();
        }
        VanGui::TreePop();
    }

    // Demonstrate using TableHeader() calls instead of TableHeadersRow()
    if (open_action != -1)
        VanGui::SetNextItemOpen(open_action != 0);
    if (VanGui::TreeNode("Custom headers"))
    {
        VANGUI_DEMO_MARKER("Tables/Custom headers");
        const int COLUMNS_COUNT = 3;
        if (VanGui::BeginTable("table_custom_headers", COLUMNS_COUNT, VanGuiTableFlags_Borders | VanGuiTableFlags_Reorderable | VanGuiTableFlags_Hideable))
        {
            VanGui::TableSetupColumn("Apricot");
            VanGui::TableSetupColumn("Banana");
            VanGui::TableSetupColumn("Cherry");

            // Dummy entire-column selection storage
            // FIXME: It would be nice to actually demonstrate full-featured selection using those checkbox.
            static bool column_selected[3] = {};

            // Instead of calling TableHeadersRow() we'll submit custom headers ourselves.
            // (A different approach is also possible:
            //    - Specify VanGuiTableColumnFlags_NoHeaderLabel in some TableSetupColumn() call.
            //    - Call TableHeadersRow() normally. This will submit TableHeader() with no name.
            //    - Then call TableSetColumnIndex() to position yourself in the column and submit your stuff e.g. Checkbox().)
            VanGui::TableNextRow(VanGuiTableRowFlags_Headers);
            for (int column = 0; column < COLUMNS_COUNT; column++)
            {
                VanGui::TableSetColumnIndex(column);
                const char* column_name = VanGui::TableGetColumnName(column); // Retrieve name passed to TableSetupColumn()
                VanGui::PushID(column);
                VanGui::PushStyleVar(VanGuiStyleVar_FramePadding, VanVec2(0, 0));
                VanGui::Checkbox("##checkall", &column_selected[column]);
                VanGui::PopStyleVar();
                VanGui::SameLine(0.0f, VanGui::GetStyle().ItemInnerSpacing.x);
                VanGui::TableHeader(column_name);
                VanGui::PopID();
            }

            // Submit table contents
            for (int row = 0; row < 5; row++)
            {
                VanGui::TableNextRow();
                for (int column = 0; column < 3; column++)
                {
                    char buf[32];
                    snprintf(buf, sizeof(buf), "Cell %d,%d", column, row);
                    VanGui::TableSetColumnIndex(column);
                    VanGui::Selectable(buf, column_selected[column]);
                }
            }
            VanGui::EndTable();
        }
        VanGui::TreePop();
    }

    // Demonstrate using VanGuiTableColumnFlags_AngledHeader flag to create angled headers
    if (open_action != -1)
        VanGui::SetNextItemOpen(open_action != 0);
    if (VanGui::TreeNode("Angled headers"))
    {
        VANGUI_DEMO_MARKER("Tables/Angled headers");
        const char* column_names[] = { "Track", "cabasa", "ride", "smash", "tom-hi", "tom-mid", "tom-low", "hihat-o", "hihat-c", "snare-s", "snare-c", "clap", "rim", "kick" };
        const int columns_count = VAN_COUNTOF(column_names);
        const int rows_count = 12;

        static VanGuiTableFlags table_flags = VanGuiTableFlags_SizingFixedFit | VanGuiTableFlags_ScrollX | VanGuiTableFlags_ScrollY | VanGuiTableFlags_BordersOuter | VanGuiTableFlags_BordersInnerH | VanGuiTableFlags_Hideable | VanGuiTableFlags_Resizable | VanGuiTableFlags_Reorderable | VanGuiTableFlags_HighlightHoveredColumn;
        static VanGuiTableColumnFlags column_flags = VanGuiTableColumnFlags_AngledHeader | VanGuiTableColumnFlags_WidthFixed;
        static bool bools[columns_count * rows_count] = {}; // Dummy storage selection storage
        static int frozen_cols = 1;
        static int frozen_rows = 2;
        VanGui::CheckboxFlags("_ScrollX", &table_flags, VanGuiTableFlags_ScrollX);
        VanGui::CheckboxFlags("_ScrollY", &table_flags, VanGuiTableFlags_ScrollY);
        VanGui::CheckboxFlags("_Resizable", &table_flags, VanGuiTableFlags_Resizable);
        VanGui::CheckboxFlags("_Sortable", &table_flags, VanGuiTableFlags_Sortable);
        VanGui::CheckboxFlags("_NoBordersInBody", &table_flags, VanGuiTableFlags_NoBordersInBody);
        VanGui::CheckboxFlags("_HighlightHoveredColumn", &table_flags, VanGuiTableFlags_HighlightHoveredColumn);
        VanGui::SetNextItemWidth(VanGui::GetFontSize() * 8);
        VanGui::SliderInt("Frozen columns", &frozen_cols, 0, 2);
        VanGui::SetNextItemWidth(VanGui::GetFontSize() * 8);
        VanGui::SliderInt("Frozen rows", &frozen_rows, 0, 2);
        VanGui::CheckboxFlags("Disable header contributing to column width", &column_flags, VanGuiTableColumnFlags_NoHeaderWidth);

        if (VanGui::TreeNode("Style settings"))
        {
            VanGui::SameLine();
            HelpMarker("Giving access to some VanGuiStyle value in this demo for convenience.");
            VanGui::SetNextItemWidth(VanGui::GetFontSize() * 8);
            VanGui::SliderAngle("style.TableAngledHeadersAngle", &VanGui::GetStyle().TableAngledHeadersAngle, -50.0f, +50.0f);
            VanGui::SetNextItemWidth(VanGui::GetFontSize() * 8);
            VanGui::SliderFloat2("style.TableAngledHeadersTextAlign", reinterpret_cast<float*>(&VanGui::GetStyle().TableAngledHeadersTextAlign), 0.0f, 1.0f, "%.2f");
            VanGui::TreePop();
        }

        if (VanGui::BeginTable("table_angled_headers", columns_count, table_flags, VanVec2(0.0f, TEXT_BASE_HEIGHT * 12)))
        {
            VanGui::TableSetupColumn(column_names[0], VanGuiTableColumnFlags_NoHide | VanGuiTableColumnFlags_NoReorder);
            for (int n = 1; n < columns_count; n++)
                VanGui::TableSetupColumn(column_names[n], column_flags);
            VanGui::TableSetupScrollFreeze(frozen_cols, frozen_rows);

            VanGui::TableAngledHeadersRow(); // Draw angled headers for all columns with the VanGuiTableColumnFlags_AngledHeader flag.
            VanGui::TableHeadersRow();       // Draw remaining headers and allow access to context-menu and other functions.
            for (int row = 0; row < rows_count; row++)
            {
                VanGui::PushID(row);
                VanGui::TableNextRow();
                VanGui::TableSetColumnIndex(0);
                VanGui::AlignTextToFramePadding();
                VanGui::Text("Track %d", row);
                for (int column = 1; column < columns_count; column++)
                    if (VanGui::TableSetColumnIndex(column))
                    {
                        VanGui::PushID(column);
                        VanGui::Checkbox("", &bools[row * columns_count + column]);
                        VanGui::PopID();
                    }
                VanGui::PopID();
            }
            VanGui::EndTable();
        }
        VanGui::TreePop();
    }

    // Demonstrate creating custom context menus inside columns,
    // while playing it nice with context menus provided by TableHeadersRow()/TableHeader()
    if (open_action != -1)
        VanGui::SetNextItemOpen(open_action != 0);
    if (VanGui::TreeNode("Context menus"))
    {
        VANGUI_DEMO_MARKER("Tables/Context menus");
        HelpMarker(
            "By default, right-clicking over a TableHeadersRow()/TableHeader() line will open the default context-menu.\n"
            "Using VanGuiTableFlags_ContextMenuInBody we also allow right-clicking over columns body.");
        static VanGuiTableFlags flags1 = VanGuiTableFlags_Resizable | VanGuiTableFlags_Reorderable | VanGuiTableFlags_Hideable | VanGuiTableFlags_Borders | VanGuiTableFlags_ContextMenuInBody;

        PushStyleCompact();
        VanGui::CheckboxFlags("VanGuiTableFlags_ContextMenuInBody", &flags1, VanGuiTableFlags_ContextMenuInBody);
        PopStyleCompact();

        // Context Menus: first example
        // [1.1] Right-click on the TableHeadersRow() line to open the default table context menu.
        // [1.2] Right-click in columns also open the default table context menu (if VanGuiTableFlags_ContextMenuInBody is set)
        const int COLUMNS_COUNT = 3;
        if (VanGui::BeginTable("table_context_menu", COLUMNS_COUNT, flags1))
        {
            VanGui::TableSetupColumn("One");
            VanGui::TableSetupColumn("Two");
            VanGui::TableSetupColumn("Three");

            // [1.1]] Right-click on the TableHeadersRow() line to open the default table context menu.
            VanGui::TableHeadersRow();

            // Submit dummy contents
            for (int row = 0; row < 4; row++)
            {
                VanGui::TableNextRow();
                for (int column = 0; column < COLUMNS_COUNT; column++)
                {
                    VanGui::TableSetColumnIndex(column);
                    VanGui::Text("Cell %d,%d", column, row);
                }
            }
            VanGui::EndTable();
        }

        // Context Menus: second example
        // [2.1] Right-click on the TableHeadersRow() line to open the default table context menu.
        // [2.2] Right-click on the ".." to open a custom popup
        // [2.3] Right-click in columns to open another custom popup
        HelpMarker(
            "Demonstrate mixing table context menu (over header), item context button (over button) "
            "and custom per-column context menu (over column body).");
        VanGuiTableFlags flags2 = VanGuiTableFlags_Resizable | VanGuiTableFlags_SizingFixedFit | VanGuiTableFlags_Reorderable | VanGuiTableFlags_Hideable | VanGuiTableFlags_Borders;
        if (VanGui::BeginTable("table_context_menu_2", COLUMNS_COUNT, flags2))
        {
            VanGui::TableSetupColumn("One");
            VanGui::TableSetupColumn("Two");
            VanGui::TableSetupColumn("Three");

            // [2.1] Right-click on the TableHeadersRow() line to open the default table context menu.
            VanGui::TableHeadersRow();
            for (int row = 0; row < 4; row++)
            {
                VanGui::TableNextRow();
                for (int column = 0; column < COLUMNS_COUNT; column++)
                {
                    // Submit dummy contents
                    VanGui::TableSetColumnIndex(column);
                    VanGui::Text("Cell %d,%d", column, row);
                    VanGui::SameLine();

                    // [2.2] Right-click on the ".." to open a custom popup
                    VanGui::PushID(row * COLUMNS_COUNT + column);
                    VanGui::SmallButton("..");
                    if (VanGui::BeginPopupContextItem())
                    {
                        VanGui::Text("This is the popup for Button(\"..\") in Cell %d,%d", column, row);
                        if (VanGui::Button("Close"))
                            VanGui::CloseCurrentPopup();
                        VanGui::EndPopup();
                    }
                    VanGui::PopID();
                }
            }

            // [2.3] Right-click anywhere in columns to open another custom popup
            // (instead of testing for !IsAnyItemHovered() we could also call OpenPopup() with VanGuiPopupFlags_NoOpenOverExistingPopup
            // to manage popup priority as the popups triggers, here "are we hovering a column" are overlapping)
            int hovered_column = -1;
            for (int column = 0; column < COLUMNS_COUNT + 1; column++)
            {
                VanGui::PushID(column);
                if (VanGui::TableGetColumnFlags(column) & VanGuiTableColumnFlags_IsHovered)
                    hovered_column = column;
                if (hovered_column == column && !VanGui::IsAnyItemHovered() && VanGui::IsMouseReleased(1))
                    VanGui::OpenPopup("MyPopup");
                if (VanGui::BeginPopup("MyPopup"))
                {
                    if (column == COLUMNS_COUNT)
                        VanGui::Text("This is a custom popup for unused space after the last column.");
                    else
                        VanGui::Text("This is a custom popup for Column %d", column);
                    if (VanGui::Button("Close"))
                        VanGui::CloseCurrentPopup();
                    VanGui::EndPopup();
                }
                VanGui::PopID();
            }

            VanGui::EndTable();
            VanGui::Text("Hovered column: %d", hovered_column);
        }
        VanGui::TreePop();
    }

    // Demonstrate creating multiple tables with the same ID
    if (open_action != -1)
        VanGui::SetNextItemOpen(open_action != 0);
    if (VanGui::TreeNode("Synced instances"))
    {
        VANGUI_DEMO_MARKER("Tables/Synced instances");
        HelpMarker("Multiple tables with the same identifier will share their settings, width, visibility, order etc.");

        static VanGuiTableFlags flags = VanGuiTableFlags_Resizable | VanGuiTableFlags_Reorderable | VanGuiTableFlags_Hideable | VanGuiTableFlags_Borders | VanGuiTableFlags_SizingFixedFit | VanGuiTableFlags_NoSavedSettings;
        VanGui::CheckboxFlags("VanGuiTableFlags_Resizable", &flags, VanGuiTableFlags_Resizable);
        VanGui::CheckboxFlags("VanGuiTableFlags_ScrollY", &flags, VanGuiTableFlags_ScrollY);
        VanGui::CheckboxFlags("VanGuiTableFlags_SizingFixedFit", &flags, VanGuiTableFlags_SizingFixedFit);
        VanGui::CheckboxFlags("VanGuiTableFlags_HighlightHoveredColumn", &flags, VanGuiTableFlags_HighlightHoveredColumn);
        for (int n = 0; n < 3; n++)
        {
            char buf[32];
            snprintf(buf, sizeof(buf), "Synced Table %d", n);
            bool open = VanGui::CollapsingHeader(buf, VanGuiTreeNodeFlags_DefaultOpen);
            if (open && VanGui::BeginTable("Table", 3, flags, VanVec2(0.0f, VanGui::GetTextLineHeightWithSpacing() * 5)))
            {
                VanGui::TableSetupColumn("One");
                VanGui::TableSetupColumn("Two");
                VanGui::TableSetupColumn("Three");
                VanGui::TableHeadersRow();
                const int cell_count = (n == 1) ? 27 : 9; // Make second table have a scrollbar to verify that additional decoration is not affecting column positions.
                for (int cell = 0; cell < cell_count; cell++)
                {
                    VanGui::TableNextColumn();
                    VanGui::Text("this cell %d", cell);
                }
                VanGui::EndTable();
            }
        }
        VanGui::TreePop();
    }

    // Demonstrate using Sorting facilities
    // This is a simplified version of the "Advanced" example, where we mostly focus on the code necessary to handle sorting.
    // Note that the "Advanced" example also showcase manually triggering a sort (e.g. if item quantities have been modified)
    static const char* template_items_names[] =
    {
        "Banana", "Apple", "Cherry", "Watermelon", "Grapefruit", "Strawberry", "Mango",
        "Kiwi", "Orange", "Pineapple", "Blueberry", "Plum", "Coconut", "Pear", "Apricot"
    };
    if (open_action != -1)
        VanGui::SetNextItemOpen(open_action != 0);
    if (VanGui::TreeNode("Sorting"))
    {
        VANGUI_DEMO_MARKER("Tables/Sorting");
        // Create item list
        static VanVector<MyItem> items;
        if (items.Size == 0)
        {
            items.resize(50, MyItem());
            for (int n = 0; n < items.Size; n++)
            {
                const int template_n = n % VAN_COUNTOF(template_items_names);
                MyItem& item = items[n];
                item.ID = n;
                item.Name = template_items_names[template_n];
                item.Quantity = (n * n - n) % 20; // Assign default quantities
            }
        }

        // Options
        static VanGuiTableFlags flags =
            VanGuiTableFlags_Resizable | VanGuiTableFlags_Reorderable | VanGuiTableFlags_Hideable | VanGuiTableFlags_Sortable | VanGuiTableFlags_SortMulti
            | VanGuiTableFlags_RowBg | VanGuiTableFlags_BordersOuter | VanGuiTableFlags_BordersV | VanGuiTableFlags_NoBordersInBody
            | VanGuiTableFlags_ScrollY;
        PushStyleCompact();
        VanGui::CheckboxFlags("VanGuiTableFlags_SortMulti", &flags, VanGuiTableFlags_SortMulti);
        VanGui::SameLine(); HelpMarker("When sorting is enabled: hold shift when clicking headers to sort on multiple column. TableGetSortSpecs() may return specs where (SpecsCount > 1).");
        VanGui::CheckboxFlags("VanGuiTableFlags_SortTristate", &flags, VanGuiTableFlags_SortTristate);
        VanGui::SameLine(); HelpMarker("When sorting is enabled: allow no sorting, disable default sorting. TableGetSortSpecs() may return specs where (SpecsCount == 0).");
        PopStyleCompact();

        if (VanGui::BeginTable("table_sorting", 4, flags, VanVec2(0.0f, TEXT_BASE_HEIGHT * 15), 0.0f))
        {
            // Declare columns
            // We use the "user_id" parameter of TableSetupColumn() to specify a user id that will be stored in the sort specifications.
            // This is so our sort function can identify a column given our own identifier. We could also identify them based on their index!
            // Demonstrate using a mixture of flags among available sort-related flags:
            // - VanGuiTableColumnFlags_DefaultSort
            // - VanGuiTableColumnFlags_NoSort / VanGuiTableColumnFlags_NoSortAscending / VanGuiTableColumnFlags_NoSortDescending
            // - VanGuiTableColumnFlags_PreferSortAscending / VanGuiTableColumnFlags_PreferSortDescending
            VanGui::TableSetupColumn("ID",       VanGuiTableColumnFlags_DefaultSort          | VanGuiTableColumnFlags_WidthFixed,   0.0f, MyItemColumnID_ID);
            VanGui::TableSetupColumn("Name",                                                  VanGuiTableColumnFlags_WidthFixed,   0.0f, MyItemColumnID_Name);
            VanGui::TableSetupColumn("Action",   VanGuiTableColumnFlags_NoSort               | VanGuiTableColumnFlags_WidthFixed,   0.0f, MyItemColumnID_Action);
            VanGui::TableSetupColumn("Quantity", VanGuiTableColumnFlags_PreferSortDescending | VanGuiTableColumnFlags_WidthStretch, 0.0f, MyItemColumnID_Quantity);
            VanGui::TableSetupScrollFreeze(0, 1); // Make row always visible
            VanGui::TableHeadersRow();

            // Sort our data if sort specs have been changed!
            if (VanGuiTableSortSpecs* sort_specs = VanGui::TableGetSortSpecs())
                if (sort_specs->SpecsDirty)
                {
                    MyItem::SortWithSortSpecs(sort_specs, items.Data, items.Size);
                    sort_specs->SpecsDirty = false;
                }

            // Demonstrate using clipper for large vertical lists
            VanGuiListClipper clipper;
            clipper.Begin(items.Size);
            while (clipper.Step())
                for (int row_n = clipper.DisplayStart; row_n < clipper.DisplayEnd; row_n++)
                {
                    // Display a data item
                    MyItem* item = &items[row_n];
                    VanGui::PushID(item->ID);
                    VanGui::TableNextRow();
                    VanGui::TableNextColumn();
                    VanGui::Text("%04d", item->ID);
                    VanGui::TableNextColumn();
                    VanGui::TextUnformatted(item->Name);
                    VanGui::TableNextColumn();
                    VanGui::SmallButton("None");
                    VanGui::TableNextColumn();
                    VanGui::Text("%d", item->Quantity);
                    VanGui::PopID();
                }
            VanGui::EndTable();
        }
        VanGui::TreePop();
    }

    // In this example we'll expose most table flags and settings.
    // For specific flags and settings refer to the corresponding section for more detailed explanation.
    // This section is mostly useful to experiment with combining certain flags or settings with each others.
    //VanGui::SetNextItemOpen(true, VanGuiCond_Once); // [DEBUG]
    if (open_action != -1)
        VanGui::SetNextItemOpen(open_action != 0);
    if (VanGui::TreeNode("Advanced"))
    {
        VANGUI_DEMO_MARKER("Tables/Advanced");
        static VanGuiTableFlags flags =
            VanGuiTableFlags_Resizable | VanGuiTableFlags_Reorderable | VanGuiTableFlags_Hideable
            | VanGuiTableFlags_Sortable | VanGuiTableFlags_SortMulti
            | VanGuiTableFlags_RowBg | VanGuiTableFlags_Borders | VanGuiTableFlags_NoBordersInBody
            | VanGuiTableFlags_ScrollX | VanGuiTableFlags_ScrollY
            | VanGuiTableFlags_SizingFixedFit;
        static VanGuiTableColumnFlags columns_base_flags = VanGuiTableColumnFlags_None;

        enum ContentsType { CT_Text, CT_Button, CT_SmallButton, CT_FillButton, CT_Selectable, CT_SelectableSpanRow };
        static int contents_type = CT_SelectableSpanRow;
        const char* contents_type_names[] = { "Text", "Button", "SmallButton", "FillButton", "Selectable", "Selectable (span row)" };
        static int freeze_cols = 1;
        static int freeze_rows = 1;
        static int items_count = VAN_COUNTOF(template_items_names) * 2;
        static VanVec2 outer_size_value = VanVec2(0.0f, TEXT_BASE_HEIGHT * 12);
        static float row_min_height = 0.0f; // Auto
        static float inner_width_with_scroll = 0.0f; // Auto-extend
        static bool outer_size_enabled = true;
        static bool show_headers = true;
        static bool show_wrapped_text = false;
        //static VanGuiTextFilter filter;
        //VanGui::SetNextItemOpen(true, VanGuiCond_Once); // FIXME-TABLE: Enabling this results in initial clipped first pass on table which tend to affect column sizing
        if (VanGui::TreeNode("Options"))
        {
            // Make the UI compact because there are so many fields
            PushStyleCompact();
            VanGui::PushItemWidth(TEXT_BASE_WIDTH * 28.0f);

            if (VanGui::TreeNodeEx("Features:", VanGuiTreeNodeFlags_DefaultOpen))
            {
                VanGui::CheckboxFlags("VanGuiTableFlags_Resizable", &flags, VanGuiTableFlags_Resizable);
                VanGui::CheckboxFlags("VanGuiTableFlags_Reorderable", &flags, VanGuiTableFlags_Reorderable);
                VanGui::CheckboxFlags("VanGuiTableFlags_Hideable", &flags, VanGuiTableFlags_Hideable);
                VanGui::CheckboxFlags("VanGuiTableFlags_Sortable", &flags, VanGuiTableFlags_Sortable);
                VanGui::CheckboxFlags("VanGuiTableFlags_NoSavedSettings", &flags, VanGuiTableFlags_NoSavedSettings);
                VanGui::CheckboxFlags("VanGuiTableFlags_ContextMenuInBody", &flags, VanGuiTableFlags_ContextMenuInBody);
                VanGui::TreePop();
            }

            if (VanGui::TreeNodeEx("Decorations:", VanGuiTreeNodeFlags_DefaultOpen))
            {
                VanGui::CheckboxFlags("VanGuiTableFlags_RowBg", &flags, VanGuiTableFlags_RowBg);
                VanGui::CheckboxFlags("VanGuiTableFlags_BordersV", &flags, VanGuiTableFlags_BordersV);
                VanGui::CheckboxFlags("VanGuiTableFlags_BordersOuterV", &flags, VanGuiTableFlags_BordersOuterV);
                VanGui::CheckboxFlags("VanGuiTableFlags_BordersInnerV", &flags, VanGuiTableFlags_BordersInnerV);
                VanGui::CheckboxFlags("VanGuiTableFlags_BordersH", &flags, VanGuiTableFlags_BordersH);
                VanGui::CheckboxFlags("VanGuiTableFlags_BordersOuterH", &flags, VanGuiTableFlags_BordersOuterH);
                VanGui::CheckboxFlags("VanGuiTableFlags_BordersInnerH", &flags, VanGuiTableFlags_BordersInnerH);
                VanGui::CheckboxFlags("VanGuiTableFlags_NoBordersInBody", &flags, VanGuiTableFlags_NoBordersInBody); VanGui::SameLine(); HelpMarker("Disable vertical borders in columns Body (borders will always appear in Headers)");
                VanGui::CheckboxFlags("VanGuiTableFlags_NoBordersInBodyUntilResize", &flags, VanGuiTableFlags_NoBordersInBodyUntilResize); VanGui::SameLine(); HelpMarker("Disable vertical borders in columns Body until hovered for resize (borders will always appear in Headers)");
                VanGui::TreePop();
            }

            if (VanGui::TreeNodeEx("Sizing:", VanGuiTreeNodeFlags_DefaultOpen))
            {
                EditTableSizingFlags(&flags);
                VanGui::SameLine(); HelpMarker("In the Advanced demo we override the policy of each column so those table-wide settings have less effect that typical.");
                VanGui::CheckboxFlags("VanGuiTableFlags_NoHostExtendX", &flags, VanGuiTableFlags_NoHostExtendX);
                VanGui::SameLine(); HelpMarker("Make outer width auto-fit to columns, overriding outer_size.x value.\n\nOnly available when ScrollX/ScrollY are disabled and Stretch columns are not used.");
                VanGui::CheckboxFlags("VanGuiTableFlags_NoHostExtendY", &flags, VanGuiTableFlags_NoHostExtendY);
                VanGui::SameLine(); HelpMarker("Make outer height stop exactly at outer_size.y (prevent auto-extending table past the limit).\n\nOnly available when ScrollX/ScrollY are disabled. Data below the limit will be clipped and not visible.");
                VanGui::CheckboxFlags("VanGuiTableFlags_NoKeepColumnsVisible", &flags, VanGuiTableFlags_NoKeepColumnsVisible);
                VanGui::SameLine(); HelpMarker("Only available if ScrollX is disabled.");
                VanGui::CheckboxFlags("VanGuiTableFlags_PreciseWidths", &flags, VanGuiTableFlags_PreciseWidths);
                VanGui::SameLine(); HelpMarker("Disable distributing remainder width to stretched columns (width allocation on a 100-wide table with 3 columns: Without this flag: 33,33,34. With this flag: 33,33,33). With larger number of columns, resizing will appear to be less smooth.");
                VanGui::CheckboxFlags("VanGuiTableFlags_NoClip", &flags, VanGuiTableFlags_NoClip);
                VanGui::SameLine(); HelpMarker("Disable clipping rectangle for every individual columns (reduce draw command count, items will be able to overflow into other columns). Generally incompatible with ScrollFreeze options.");
                VanGui::TreePop();
            }

            if (VanGui::TreeNodeEx("Padding:", VanGuiTreeNodeFlags_DefaultOpen))
            {
                VanGui::CheckboxFlags("VanGuiTableFlags_PadOuterX", &flags, VanGuiTableFlags_PadOuterX);
                VanGui::CheckboxFlags("VanGuiTableFlags_NoPadOuterX", &flags, VanGuiTableFlags_NoPadOuterX);
                VanGui::CheckboxFlags("VanGuiTableFlags_NoPadInnerX", &flags, VanGuiTableFlags_NoPadInnerX);
                VanGui::TreePop();
            }

            if (VanGui::TreeNodeEx("Scrolling:", VanGuiTreeNodeFlags_DefaultOpen))
            {
                VanGui::CheckboxFlags("VanGuiTableFlags_ScrollX", &flags, VanGuiTableFlags_ScrollX);
                VanGui::SameLine();
                VanGui::SetNextItemWidth(VanGui::GetFrameHeight());
                VanGui::DragInt("freeze_cols", &freeze_cols, 0.2f, 0, 9, nullptr, VanGuiSliderFlags_NoInput);
                VanGui::CheckboxFlags("VanGuiTableFlags_ScrollY", &flags, VanGuiTableFlags_ScrollY);
                VanGui::SameLine();
                VanGui::SetNextItemWidth(VanGui::GetFrameHeight());
                VanGui::DragInt("freeze_rows", &freeze_rows, 0.2f, 0, 9, nullptr, VanGuiSliderFlags_NoInput);
                VanGui::TreePop();
            }

            if (VanGui::TreeNodeEx("Sorting:", VanGuiTreeNodeFlags_DefaultOpen))
            {
                VanGui::CheckboxFlags("VanGuiTableFlags_SortMulti", &flags, VanGuiTableFlags_SortMulti);
                VanGui::SameLine(); HelpMarker("When sorting is enabled: hold shift when clicking headers to sort on multiple column. TableGetSortSpecs() may return specs where (SpecsCount > 1).");
                VanGui::CheckboxFlags("VanGuiTableFlags_SortTristate", &flags, VanGuiTableFlags_SortTristate);
                VanGui::SameLine(); HelpMarker("When sorting is enabled: allow no sorting, disable default sorting. TableGetSortSpecs() may return specs where (SpecsCount == 0).");
                VanGui::TreePop();
            }

            if (VanGui::TreeNodeEx("Headers:", VanGuiTreeNodeFlags_DefaultOpen))
            {
                VanGui::Checkbox("show_headers", &show_headers);
                VanGui::CheckboxFlags("VanGuiTableFlags_HighlightHoveredColumn", &flags, VanGuiTableFlags_HighlightHoveredColumn);
                VanGui::CheckboxFlags("VanGuiTableColumnFlags_AngledHeader", &columns_base_flags, VanGuiTableColumnFlags_AngledHeader);
                VanGui::SameLine(); HelpMarker("Enable AngledHeader on all columns. Best enabled on selected narrow columns (see \"Angled headers\" section of the demo).");
                VanGui::TreePop();
            }

            if (VanGui::TreeNodeEx("Other:", VanGuiTreeNodeFlags_DefaultOpen))
            {
                VanGui::Checkbox("show_wrapped_text", &show_wrapped_text);

                VanGui::DragFloat2("##OuterSize", &outer_size_value.x);
                VanGui::SameLine(0.0f, VanGui::GetStyle().ItemInnerSpacing.x);
                VanGui::Checkbox("outer_size", &outer_size_enabled);
                VanGui::SameLine();
                HelpMarker("If scrolling is disabled (ScrollX and ScrollY not set):\n"
                    "- The table is output directly in the parent window.\n"
                    "- OuterSize.x < 0.0f will right-align the table.\n"
                    "- OuterSize.x = 0.0f will narrow fit the table unless there are any Stretch columns.\n"
                    "- OuterSize.y then becomes the minimum size for the table, which will extend vertically if there are more rows (unless NoHostExtendY is set).");

                // From a user point of view we will tend to use 'inner_width' differently depending on whether our table is embedding scrolling.
                // To facilitate toying with this demo we will actually pass 0.0f to the BeginTable() when ScrollX is disabled.
                VanGui::DragFloat("inner_width (when ScrollX active)", &inner_width_with_scroll, 1.0f, 0.0f, FLT_MAX);

                VanGui::DragFloat("row_min_height", &row_min_height, 1.0f, 0.0f, FLT_MAX);
                VanGui::SameLine(); HelpMarker("Specify height of the Selectable item.");

                VanGui::DragInt("items_count", &items_count, 0.1f, 0, 9999);
                VanGui::Combo("items_type (first column)", &contents_type, contents_type_names, VAN_COUNTOF(contents_type_names));
                //filter.Draw("filter");
                VanGui::TreePop();
            }

            VanGui::PopItemWidth();
            PopStyleCompact();
            VanGui::Spacing();
            VanGui::TreePop();
        }

        // Update item list if we changed the number of items
        static VanVector<MyItem> items;
        static VanVector<int> selection;
        static bool items_need_sort = false;
        if (items.Size != items_count)
        {
            items.resize(items_count, MyItem());
            for (int n = 0; n < items_count; n++)
            {
                const int template_n = n % VAN_COUNTOF(template_items_names);
                MyItem& item = items[n];
                item.ID = n;
                item.Name = template_items_names[template_n];
                item.Quantity = (template_n == 3) ? 10 : (template_n == 4) ? 20 : 0; // Assign default quantities
            }
        }

        const VanDrawList* parent_draw_list = VanGui::GetWindowDrawList();
        const int parent_draw_list_draw_cmd_count = parent_draw_list->CmdBuffer.Size;
        VanVec2 table_scroll_cur, table_scroll_max; // For debug display
        const VanDrawList* table_draw_list = nullptr;  // "

        // Submit table
        const float inner_width_to_use = (flags & VanGuiTableFlags_ScrollX) ? inner_width_with_scroll : 0.0f;
        if (VanGui::BeginTable("table_advanced", 6, flags, outer_size_enabled ? outer_size_value : VanVec2(0, 0), inner_width_to_use))
        {
            // Declare columns
            // We use the "user_id" parameter of TableSetupColumn() to specify a user id that will be stored in the sort specifications.
            // This is so our sort function can identify a column given our own identifier. We could also identify them based on their index!
            VanGui::TableSetupColumn("ID",           columns_base_flags | VanGuiTableColumnFlags_DefaultSort | VanGuiTableColumnFlags_WidthFixed | VanGuiTableColumnFlags_NoHide, 0.0f, MyItemColumnID_ID);
            VanGui::TableSetupColumn("Name",         columns_base_flags | VanGuiTableColumnFlags_WidthFixed, 0.0f, MyItemColumnID_Name);
            VanGui::TableSetupColumn("Action",       columns_base_flags | VanGuiTableColumnFlags_NoSort | VanGuiTableColumnFlags_WidthFixed, 0.0f, MyItemColumnID_Action);
            VanGui::TableSetupColumn("Quantity",     columns_base_flags | VanGuiTableColumnFlags_PreferSortDescending, 0.0f, MyItemColumnID_Quantity);
            VanGui::TableSetupColumn("Description",  columns_base_flags | ((flags & VanGuiTableFlags_NoHostExtendX) ? 0 : VanGuiTableColumnFlags_WidthStretch), 0.0f, MyItemColumnID_Description);
            VanGui::TableSetupColumn("Hidden",       columns_base_flags |  VanGuiTableColumnFlags_DefaultHide | VanGuiTableColumnFlags_NoSort);
            VanGui::TableSetupScrollFreeze(freeze_cols, freeze_rows);

            // Sort our data if sort specs have been changed!
            VanGuiTableSortSpecs* sort_specs = VanGui::TableGetSortSpecs();
            if (sort_specs && sort_specs->SpecsDirty)
                items_need_sort = true;
            if (sort_specs && items_need_sort && items.Size > 1)
            {
                MyItem::SortWithSortSpecs(sort_specs, items.Data, items.Size);
                sort_specs->SpecsDirty = false;
            }
            items_need_sort = false;

            // Take note of whether we are currently sorting based on the Quantity field,
            // we will use this to trigger sorting when we know the data of this column has been modified.
            const bool sorts_specs_using_quantity = (VanGui::TableGetColumnFlags(3) & VanGuiTableColumnFlags_IsSorted) != 0;

            // Show headers
            if (show_headers && (columns_base_flags & VanGuiTableColumnFlags_AngledHeader) != 0)
                VanGui::TableAngledHeadersRow();
            if (show_headers)
                VanGui::TableHeadersRow();

            // Show data
            // FIXME-TABLE FIXME-NAV: How we can get decent up/down even though we have the buttons here?
#if 1
            // Demonstrate using clipper for large vertical lists
            VanGuiListClipper clipper;
            clipper.Begin(items.Size);
            while (clipper.Step())
            {
                for (int row_n = clipper.DisplayStart; row_n < clipper.DisplayEnd; row_n++)
#else
            // Without clipper
            {
                for (int row_n = 0; row_n < items.Size; row_n++)
#endif
                {
                    MyItem* item = &items[row_n];
                    //if (!filter.PassFilter(item->Name))
                    //    continue;

                    const bool item_is_selected = selection.contains(item->ID);
                    VanGui::PushID(item->ID);
                    VanGui::TableNextRow(VanGuiTableRowFlags_None, row_min_height);

                    // For the demo purpose we can select among different type of items submitted in the first column
                    VanGui::TableSetColumnIndex(0);
                    char label[32];
                    snprintf(label, sizeof(label), "%04d", item->ID);
                    if (contents_type == CT_Text)
                        VanGui::TextUnformatted(label);
                    else if (contents_type == CT_Button)
                        VanGui::Button(label);
                    else if (contents_type == CT_SmallButton)
                        VanGui::SmallButton(label);
                    else if (contents_type == CT_FillButton)
                        VanGui::Button(label, VanVec2(-FLT_MIN, 0.0f));
                    else if (contents_type == CT_Selectable || contents_type == CT_SelectableSpanRow)
                    {
                        VanGuiSelectableFlags selectable_flags = (contents_type == CT_SelectableSpanRow) ? VanGuiSelectableFlags_SpanAllColumns | VanGuiSelectableFlags_AllowOverlap : VanGuiSelectableFlags_None;
                        if (VanGui::Selectable(label, item_is_selected, selectable_flags, VanVec2(0, row_min_height)))
                        {
                            if (VanGui::GetIO().KeyCtrl)
                            {
                                if (item_is_selected)
                                    selection.find_erase_unsorted(item->ID);
                                else
                                    selection.push_back(item->ID);
                            }
                            else
                            {
                                selection.clear();
                                selection.push_back(item->ID);
                            }
                        }
                    }

                    if (VanGui::TableSetColumnIndex(1))
                        VanGui::TextUnformatted(item->Name);

                    // Here we demonstrate marking our data set as needing to be sorted again if we modified a quantity,
                    // and we are currently sorting on the column showing the Quantity.
                    // To avoid triggering a sort while holding the button, we only trigger it when the button has been released.
                    // You will probably need some extra logic if you want to automatically sort when a specific entry changes.
                    if (VanGui::TableSetColumnIndex(2))
                    {
                        if (VanGui::SmallButton("Chop")) { item->Quantity += 1; }
                        if (sorts_specs_using_quantity && VanGui::IsItemDeactivated()) { items_need_sort = true; }
                        VanGui::SameLine();
                        if (VanGui::SmallButton("Eat")) { item->Quantity -= 1; }
                        if (sorts_specs_using_quantity && VanGui::IsItemDeactivated()) { items_need_sort = true; }
                    }

                    if (VanGui::TableSetColumnIndex(3))
                        VanGui::Text("%d", item->Quantity);

                    VanGui::TableSetColumnIndex(4);
                    if (show_wrapped_text)
                        VanGui::TextWrapped("Lorem ipsum dolor sit amet");
                    else
                        VanGui::Text("Lorem ipsum dolor sit amet");

                    if (VanGui::TableSetColumnIndex(5))
                        VanGui::Text("1234");

                    VanGui::PopID();
                }
            }

            // Store some info to display debug details below
            table_scroll_cur = VanVec2(VanGui::GetScrollX(), VanGui::GetScrollY());
            table_scroll_max = VanVec2(VanGui::GetScrollMaxX(), VanGui::GetScrollMaxY());
            table_draw_list = VanGui::GetWindowDrawList();
            VanGui::EndTable();
        }
        static bool show_debug_details = false;
        VanGui::Checkbox("Debug details", &show_debug_details);
        if (show_debug_details && table_draw_list)
        {
            VanGui::SameLine(0.0f, 0.0f);
            const int table_draw_list_draw_cmd_count = table_draw_list->CmdBuffer.Size;
            if (table_draw_list == parent_draw_list)
                VanGui::Text(": DrawCmd: +%d (in same window)",
                    table_draw_list_draw_cmd_count - parent_draw_list_draw_cmd_count);
            else
                VanGui::Text(": DrawCmd: +%d (in child window), Scroll: (%.f/%.f) (%.f/%.f)",
                    table_draw_list_draw_cmd_count - 1, table_scroll_cur.x, table_scroll_max.x, table_scroll_cur.y, table_scroll_max.y);
        }
        VanGui::TreePop();
    }

    VanGui::PopID();

    DemoWindowColumns();

    if (disable_indent)
        VanGui::PopStyleVar();
}

// Demonstrate old/legacy Columns API!
// [2020: Columns are under-featured and not maintained. Prefer using the more flexible and powerful BeginTable() API!]
static void DemoWindowColumns()
{
    bool open = VanGui::TreeNode("Legacy Columns API");
    VanGui::SameLine();
    HelpMarker("Columns() is an old API! Prefer using the more flexible and powerful BeginTable() API!");
    if (!open)
        return;

    // Basic columns
    if (VanGui::TreeNode("Basic"))
    {
        VANGUI_DEMO_MARKER("Columns (legacy API)/Basic");
        VanGui::Text("Without border:");
        VanGui::Columns(3, "mycolumns3", false);  // 3-ways, no border
        VanGui::Separator();
        for (int n = 0; n < 14; n++)
        {
            char label[32];
            snprintf(label, sizeof(label), "Item %d", n);
            if (VanGui::Selectable(label)) {}
            //if (VanGui::Button(label, VanVec2(-FLT_MIN,0.0f))) {}
            VanGui::NextColumn();
        }
        VanGui::Columns(1);
        VanGui::Separator();

        VanGui::Text("With border:");
        VanGui::Columns(4, "mycolumns"); // 4-ways, with border
        VanGui::Separator();
        VanGui::Text("ID"); VanGui::NextColumn();
        VanGui::Text("Name"); VanGui::NextColumn();
        VanGui::Text("Path"); VanGui::NextColumn();
        VanGui::Text("Hovered"); VanGui::NextColumn();
        VanGui::Separator();
        const char* names[3] = { "One", "Two", "Three" };
        const char* paths[3] = { "/path/one", "/path/two", "/path/three" };
        static int selected = -1;
        for (int i = 0; i < 3; i++)
        {
            char label[32];
            snprintf(label, sizeof(label), "%04d", i);
            if (VanGui::Selectable(label, selected == i, VanGuiSelectableFlags_SpanAllColumns))
                selected = i;
            bool hovered = VanGui::IsItemHovered();
            VanGui::NextColumn();
            VanGui::Text(names[i]); VanGui::NextColumn();
            VanGui::Text(paths[i]); VanGui::NextColumn();
            VanGui::Text("%d", hovered); VanGui::NextColumn();
        }
        VanGui::Columns(1);
        VanGui::Separator();
        VanGui::TreePop();
    }

    if (VanGui::TreeNode("Borders"))
    {
        VANGUI_DEMO_MARKER("Columns (legacy API)/Borders");
        // NB: Future columns API should allow automatic horizontal borders.
        static bool h_borders = true;
        static bool v_borders = true;
        static int columns_count = 4;
        const int lines_count = 3;
        VanGui::SetNextItemWidth(VanGui::GetFontSize() * 8);
        VanGui::DragInt("##columns_count", &columns_count, 0.1f, 2, 10, "%d columns");
        if (columns_count < 2)
            columns_count = 2;
        VanGui::SameLine();
        VanGui::Checkbox("horizontal", &h_borders);
        VanGui::SameLine();
        VanGui::Checkbox("vertical", &v_borders);
        VanGui::Columns(columns_count, nullptr, v_borders);
        for (int i = 0; i < columns_count * lines_count; i++)
        {
            if (h_borders && VanGui::GetColumnIndex() == 0)
                VanGui::Separator();
            VanGui::PushID(i);
            VanGui::Text("%c%c%c", 'a' + i, 'a' + i, 'a' + i);
            VanGui::Text("Width %.2f", VanGui::GetColumnWidth());
            VanGui::Text("Avail %.2f", VanGui::GetContentRegionAvail().x);
            VanGui::Text("Offset %.2f", VanGui::GetColumnOffset());
            VanGui::Text("Long text that is likely to clip");
            VanGui::Button("Button", VanVec2(-FLT_MIN, 0.0f));
            VanGui::PopID();
            VanGui::NextColumn();
        }
        VanGui::Columns(1);
        if (h_borders)
            VanGui::Separator();
        VanGui::TreePop();
    }

    // Create multiple items in a same cell before switching to next column
    if (VanGui::TreeNode("Mixed items"))
    {
        VANGUI_DEMO_MARKER("Columns (legacy API)/Mixed items");
        VanGui::Columns(3, "mixed");
        VanGui::Separator();

        VanGui::Text("Hello");
        VanGui::Button("Banana");
        VanGui::NextColumn();

        VanGui::Text("VanGui");
        VanGui::Button("Apple");
        static float foo = 1.0f;
        VanGui::InputFloat("red", &foo, 0.05f, 0, "%.3f");
        VanGui::Text("An extra line here.");
        VanGui::NextColumn();

        VanGui::Text("Sailor");
        VanGui::Button("Corniflower");
        static float bar = 1.0f;
        VanGui::InputFloat("blue", &bar, 0.05f, 0, "%.3f");
        VanGui::NextColumn();

        if (VanGui::CollapsingHeader("Category A")) { VanGui::Text("Blah blah blah"); } VanGui::NextColumn();
        if (VanGui::CollapsingHeader("Category B")) { VanGui::Text("Blah blah blah"); } VanGui::NextColumn();
        if (VanGui::CollapsingHeader("Category C")) { VanGui::Text("Blah blah blah"); } VanGui::NextColumn();
        VanGui::Columns(1);
        VanGui::Separator();
        VanGui::TreePop();
    }

    // Word wrapping
    if (VanGui::TreeNode("Word-wrapping"))
    {
        VANGUI_DEMO_MARKER("Columns (legacy API)/Word-wrapping");
        VanGui::Columns(2, "word-wrapping");
        VanGui::Separator();
        VanGui::TextWrapped("The quick brown fox jumps over the lazy dog.");
        VanGui::TextWrapped("Hello Left");
        VanGui::NextColumn();
        VanGui::TextWrapped("The quick brown fox jumps over the lazy dog.");
        VanGui::TextWrapped("Hello Right");
        VanGui::Columns(1);
        VanGui::Separator();
        VanGui::TreePop();
    }

    if (VanGui::TreeNode("Horizontal Scrolling"))
    {
        VANGUI_DEMO_MARKER("Columns (legacy API)/Horizontal Scrolling");
        VanGui::SetNextWindowContentSize(VanVec2(1500.0f, 0.0f));
        VanVec2 child_size = VanVec2(0, VanGui::GetFontSize() * 20.0f);
        VanGui::BeginChild("##ScrollingRegion", child_size, VanGuiChildFlags_None, VanGuiWindowFlags_HorizontalScrollbar);
        VanGui::Columns(10);

        // Also demonstrate using clipper for large vertical lists
        int ITEMS_COUNT = 2000;
        VanGuiListClipper clipper;
        clipper.Begin(ITEMS_COUNT);
        while (clipper.Step())
        {
            for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
                for (int j = 0; j < 10; j++)
                {
                    VanGui::Text("Line %d Column %d...", i, j);
                    VanGui::NextColumn();
                }
        }
        VanGui::Columns(1);
        VanGui::EndChild();
        VanGui::TreePop();
    }

    if (VanGui::TreeNode("Tree"))
    {
        VANGUI_DEMO_MARKER("Columns (legacy API)/Tree");
        VanGui::Columns(2, "tree", true);
        for (int x = 0; x < 3; x++)
        {
            bool open1 = VanGui::TreeNode(reinterpret_cast<void*>(static_cast<intptr_t>(x)), "Node%d", x);
            VanGui::NextColumn();
            VanGui::Text("Node contents");
            VanGui::NextColumn();
            if (open1)
            {
                for (int y = 0; y < 3; y++)
                {
                    bool open2 = VanGui::TreeNode(reinterpret_cast<void*>(static_cast<intptr_t>(y)), "Node%d.%d", x, y);
                    VanGui::NextColumn();
                    VanGui::Text("Node contents");
                    if (open2)
                    {
                        VanGui::Text("Even more contents");
                        if (VanGui::TreeNode("Tree in column"))
                        {
                            VanGui::Text("The quick brown fox jumps over the lazy dog");
                            VanGui::TreePop();
                        }
                    }
                    VanGui::NextColumn();
                    if (open2)
                        VanGui::TreePop();
                }
                VanGui::TreePop();
            }
        }
        VanGui::Columns(1);
        VanGui::TreePop();
    }

    VanGui::TreePop();
}

//-----------------------------------------------------------------------------
// [SECTION] DemoWindowInputs()
//-----------------------------------------------------------------------------

static void DemoWindowInputs()
{
    if (VanGui::CollapsingHeader("Inputs & Focus"))
    {
        VanGuiIO& io = VanGui::GetIO();

        // Display inputs submitted to VanGuiIO
        VanGui::SetNextItemOpen(true, VanGuiCond_Once);
        bool inputs_opened = VanGui::TreeNode("Inputs");
        VanGui::SameLine();
        HelpMarker(
            "This is a simplified view. See more detailed input state:\n"
            "- in 'Tools->Metrics/Debugger->Inputs'.\n"
            "- in 'Tools->Debug Log->IO'.");
        if (inputs_opened)
        {
            VANGUI_DEMO_MARKER("Inputs & Focus/Inputs");
            if (VanGui::IsMousePosValid())
                VanGui::Text("Mouse pos: (%g, %g)", io.MousePos.x, io.MousePos.y);
            else
                VanGui::Text("Mouse pos: <INVALID>");
            VanGui::Text("Mouse delta: (%g, %g)", io.MouseDelta.x, io.MouseDelta.y);
            VanGui::Text("Mouse down:");
            for (int i = 0; i < VAN_COUNTOF(io.MouseDown); i++) if (VanGui::IsMouseDown(i)) { VanGui::SameLine(); VanGui::Text("b%d (%.02f secs)", i, io.MouseDownDuration[i]); }
            VanGui::Text("Mouse wheel: %.1f", io.MouseWheel);
            VanGui::Text("Mouse clicked count:");
            for (int i = 0; i < VAN_COUNTOF(io.MouseDown); i++) if (io.MouseClickedCount[i] > 0) { VanGui::SameLine(); VanGui::Text("b%d: %d", i, io.MouseClickedCount[i]); }

            // We iterate both legacy native range and named VanGuiKey ranges. This is a little unusual/odd but this allows
            // displaying the data for old/new backends.
            // User code should never have to go through such hoops!
            // You can generally iterate between VanGuiKey_NamedKey_BEGIN and VanGuiKey_NamedKey_END.
            struct funcs { static bool IsLegacyNativeDupe(VanGuiKey) { return false; } };
            VanGuiKey start_key = VanGuiKey_NamedKey_BEGIN;
            VanGui::Text("Keys down:");         for (VanGuiKey key = start_key; key < VanGuiKey_NamedKey_END; key = (VanGuiKey)(key + 1)) { if (funcs::IsLegacyNativeDupe(key) || !VanGui::IsKeyDown(key)) continue; VanGui::SameLine(); VanGui::Text((key < VanGuiKey_NamedKey_BEGIN) ? "\"%s\"" : "\"%s\" %d", VanGui::GetKeyName(key), key); }
            VanGui::Text("Keys mods: %s%s%s%s", io.KeyCtrl ? "CTRL " : "", io.KeyShift ? "SHIFT " : "", io.KeyAlt ? "ALT " : "", io.KeySuper ? "SUPER " : "");
            VanGui::Text("Chars queue:");       for (int i = 0; i < io.InputQueueCharacters.Size; i++) { VanWchar c = io.InputQueueCharacters[i]; VanGui::SameLine();  VanGui::Text("\'%c\' (0x%04X)", (c > ' ' && c <= 255) ? (char)c : '?', c); } // FIXME: We should convert 'c' to UTF-8 here but the functions are not public.

            VanGui::TreePop();
        }

        // Display VanGuiIO output flags
        VanGui::SetNextItemOpen(true, VanGuiCond_Once);
        bool outputs_opened = VanGui::TreeNode("Outputs");
        VanGui::SameLine();
        HelpMarker(
            "The value of io.WantCaptureMouse and io.WantCaptureKeyboard are normally set by VanGUI "
            "to instruct your application of how to route inputs. Typically, when a value is true, it means "
            "VanGUI wants the corresponding inputs and we expect the underlying application to ignore them.\n\n"
            "The most typical case is: when hovering a window, VanGUI set io.WantCaptureMouse to true, "
            "and underlying application should ignore mouse inputs (in practice there are many and more subtle "
            "rules leading to how those flags are set).");
        if (outputs_opened)
        {
            VANGUI_DEMO_MARKER("Inputs & Focus/Outputs");
            VanGui::Text("io.WantCaptureMouse: %d", io.WantCaptureMouse);
            VanGui::Text("io.WantCaptureMouseUnlessPopupClose: %d", io.WantCaptureMouseUnlessPopupClose);
            VanGui::Text("io.WantCaptureKeyboard: %d", io.WantCaptureKeyboard);
            VanGui::Text("io.WantTextInput: %d", io.WantTextInput);
            VanGui::Text("io.WantSetMousePos: %d", io.WantSetMousePos);
            VanGui::Text("io.NavActive: %d, io.NavVisible: %d", io.NavActive, io.NavVisible);

            VANGUI_DEMO_MARKER("Inputs & Focus/Outputs/WantCapture override");
            if (VanGui::TreeNode("WantCapture override"))
            {
                HelpMarker(
                    "Hovering the colored canvas will override io.WantCaptureXXX fields.\n"
                    "Notice how normally (when set to none), the value of io.WantCaptureKeyboard would be false when hovering "
                    "and true when clicking.");
                static int capture_override_mouse = -1;
                static int capture_override_keyboard = -1;
                const char* capture_override_desc[] = { "None", "Set to false", "Set to true" };
                VanGui::SetNextItemWidth(VanGui::GetFontSize() * 15);
                VanGui::SliderInt("SetNextFrameWantCaptureMouse() on hover", &capture_override_mouse, -1, +1, capture_override_desc[capture_override_mouse + 1], VanGuiSliderFlags_AlwaysClamp);
                VanGui::SetNextItemWidth(VanGui::GetFontSize() * 15);
                VanGui::SliderInt("SetNextFrameWantCaptureKeyboard() on hover", &capture_override_keyboard, -1, +1, capture_override_desc[capture_override_keyboard + 1], VanGuiSliderFlags_AlwaysClamp);

                VanGui::ColorButton("##panel", VanVec4(0.7f, 0.1f, 0.7f, 1.0f), VanGuiColorEditFlags_NoTooltip | VanGuiColorEditFlags_NoDragDrop, VanVec2(128.0f, 96.0f)); // Dummy item
                if (VanGui::IsItemHovered() && capture_override_mouse != -1)
                    VanGui::SetNextFrameWantCaptureMouse(capture_override_mouse == 1);
                if (VanGui::IsItemHovered() && capture_override_keyboard != -1)
                    VanGui::SetNextFrameWantCaptureKeyboard(capture_override_keyboard == 1);

                VanGui::TreePop();
            }
            VanGui::TreePop();
        }

        // Demonstrate using Shortcut() and Routing Policies.
        // The general flow is:
        // - Code interested in a chord (e.g. "Ctrl+A") declares their intent.
        // - Multiple locations may be interested in same chord! Routing helps find a winner.
        // - Every frame, we resolve all claims and assign one owner if the modifiers are matching.
        // - The lower-level function is 'bool SetShortcutRouting()', returns true when caller got the route.
        // - Most of the times, SetShortcutRouting() is not called directly. User mostly calls Shortcut() with routing flags.
        // - If you call Shortcut() WITHOUT any routing option, it uses VanGuiInputFlags_RouteFocused.
        // TL;DR: Most uses will simply be:
        // - Shortcut(VanGuiMod_Ctrl | VanGuiKey_A); // Use VanGuiInputFlags_RouteFocused policy.
        if (VanGui::TreeNode("Shortcuts"))
        {
            VANGUI_DEMO_MARKER("Inputs & Focus/Shortcuts");
            static VanGuiInputFlags route_options = VanGuiInputFlags_Repeat;
            static VanGuiInputFlags route_type = VanGuiInputFlags_RouteFocused;
            VanGui::CheckboxFlags("VanGuiInputFlags_Repeat", &route_options, VanGuiInputFlags_Repeat);
            VanGui::RadioButton("VanGuiInputFlags_RouteActive", &route_type, VanGuiInputFlags_RouteActive);
            VanGui::RadioButton("VanGuiInputFlags_RouteFocused (default)", &route_type, VanGuiInputFlags_RouteFocused);
            VanGui::Indent();
            VanGui::BeginDisabled(route_type != VanGuiInputFlags_RouteFocused);
            VanGui::CheckboxFlags("VanGuiInputFlags_RouteOverActive##0", &route_options, VanGuiInputFlags_RouteOverActive);
            VanGui::EndDisabled();
            VanGui::Unindent();
            VanGui::RadioButton("VanGuiInputFlags_RouteGlobal", &route_type, VanGuiInputFlags_RouteGlobal);
            VanGui::Indent();
            VanGui::BeginDisabled(route_type != VanGuiInputFlags_RouteGlobal);
            VanGui::CheckboxFlags("VanGuiInputFlags_RouteOverFocused", &route_options, VanGuiInputFlags_RouteOverFocused);
            VanGui::CheckboxFlags("VanGuiInputFlags_RouteOverActive", &route_options, VanGuiInputFlags_RouteOverActive);
            VanGui::CheckboxFlags("VanGuiInputFlags_RouteUnlessBgFocused", &route_options, VanGuiInputFlags_RouteUnlessBgFocused);
            VanGui::EndDisabled();
            VanGui::Unindent();
            VanGui::RadioButton("VanGuiInputFlags_RouteAlways", &route_type, VanGuiInputFlags_RouteAlways);
            VanGuiInputFlags flags = route_type | route_options; // Merged flags
            if (route_type != VanGuiInputFlags_RouteGlobal)
                flags &= ~(VanGuiInputFlags_RouteOverFocused | VanGuiInputFlags_RouteOverActive | VanGuiInputFlags_RouteUnlessBgFocused);

            VanGui::SeparatorText("Using SetNextItemShortcut()");
            VanGui::Text("Ctrl+S");
            VanGui::SetNextItemShortcut(VanGuiMod_Ctrl | VanGuiKey_S, flags | VanGuiInputFlags_Tooltip);
            VanGui::Button("Save");
            VanGui::Text("Alt+F");
            VanGui::SetNextItemShortcut(VanGuiMod_Alt | VanGuiKey_F, flags | VanGuiInputFlags_Tooltip);
            static float f = 0.5f;
            VanGui::SliderFloat("Factor", &f, 0.0f, 1.0f);

            VanGui::SeparatorText("Using Shortcut()");
            const float line_height = VanGui::GetTextLineHeightWithSpacing();
            const VanGuiKeyChord key_chord = VanGuiMod_Ctrl | VanGuiKey_A;

            VanGui::Text("Ctrl+A");
            VanGui::Text("IsWindowFocused: %d, Shortcut: %s", VanGui::IsWindowFocused(), VanGui::Shortcut(key_chord, flags) ? "PRESSED" : "...");

            VanGui::PushStyleColor(VanGuiCol_ChildBg, VanVec4(1.0f, 0.0f, 1.0f, 0.1f));

            VanGui::BeginChild("WindowA", VanVec2(-FLT_MIN, line_height * 14), true);
            VanGui::Text("Press Ctrl+A and see who receives it!");
            VanGui::Separator();

            // 1: Window polling for Ctrl+A
            VanGui::Text("(in WindowA)");
            VanGui::Text("IsWindowFocused: %d, Shortcut: %s", VanGui::IsWindowFocused(), VanGui::Shortcut(key_chord, flags) ? "PRESSED" : "...");

            // 2: InputText also polling for Ctrl+A: it always uses _RouteFocused internally (gets priority when active)
            // (Commented because the owner-aware version of Shortcut() is still in vangui_internal.h)
            //char str[16] = "Press Ctrl+A";
            //VanGui::Spacing();
            //VanGui::InputText("InputTextB", str, VAN_COUNTOF(str), VanGuiInputTextFlags_ReadOnly);
            //VanGuiID item_id = VanGui::GetItemID();
            //VanGui::SameLine(); HelpMarker("Internal widgets always use _RouteFocused");
            //VanGui::Text("IsWindowFocused: %d, Shortcut: %s", VanGui::IsWindowFocused(), VanGui::Shortcut(key_chord, flags, item_id) ? "PRESSED" : "...");

            // 3: Dummy child is not claiming the route: focusing them shouldn't steal route away from WindowA
            VanGui::BeginChild("ChildD", VanVec2(-FLT_MIN, line_height * 4), true);
            VanGui::Text("(in ChildD: not using same Shortcut)");
            VanGui::Text("IsWindowFocused: %d", VanGui::IsWindowFocused());
            VanGui::EndChild();

            // 4: Child window polling for Ctrl+A. It is deeper than WindowA and gets priority when focused.
            VanGui::BeginChild("ChildE", VanVec2(-FLT_MIN, line_height * 4), true);
            VanGui::Text("(in ChildE: using same Shortcut)");
            VanGui::Text("IsWindowFocused: %d, Shortcut: %s", VanGui::IsWindowFocused(), VanGui::Shortcut(key_chord, flags) ? "PRESSED" : "...");
            VanGui::EndChild();

            // 5: In a popup
            if (VanGui::Button("Open Popup"))
                VanGui::OpenPopup("PopupF");
            if (VanGui::BeginPopup("PopupF"))
            {
                VanGui::Text("(in PopupF)");
                VanGui::Text("IsWindowFocused: %d, Shortcut: %s", VanGui::IsWindowFocused(), VanGui::Shortcut(key_chord, flags) ? "PRESSED" : "...");
                // (Commented because the owner-aware version of Shortcut() is still in vangui_internal.h)
                //VanGui::InputText("InputTextG", str, VAN_COUNTOF(str), VanGuiInputTextFlags_ReadOnly);
                //VanGui::Text("IsWindowFocused: %d, Shortcut: %s", VanGui::IsWindowFocused(), VanGui::Shortcut(key_chord, flags, VanGui::GetItemID()) ? "PRESSED" : "...");
                VanGui::EndPopup();
            }
            VanGui::EndChild();
            VanGui::PopStyleColor();

            VanGui::TreePop();
        }

        // Display mouse cursors
        if (VanGui::TreeNode("Mouse Cursors"))
        {
            VANGUI_DEMO_MARKER("Inputs & Focus/Mouse Cursors");
            const char* mouse_cursors_names[] = { "Arrow", "TextInput", "ResizeAll", "ResizeNS", "ResizeEW", "ResizeNESW", "ResizeNWSE", "Hand", "Wait", "Progress", "NotAllowed" };
            VAN_ASSERT(VAN_COUNTOF(mouse_cursors_names) == VanGuiMouseCursor_COUNT);

            VanGuiMouseCursor current = VanGui::GetMouseCursor();
            const char* cursor_name = (current >= VanGuiMouseCursor_Arrow) && (current < VanGuiMouseCursor_COUNT) ? mouse_cursors_names[current] : "N/A";
            VanGui::Text("Current mouse cursor = %d: %s", current, cursor_name);
            VanGui::BeginDisabled(true);
            VanGui::CheckboxFlags("io.BackendFlags: HasMouseCursors", &io.BackendFlags, VanGuiBackendFlags_HasMouseCursors);
            VanGui::EndDisabled();

            VanGui::Text("Hover to see mouse cursors:");
            VanGui::SameLine(); HelpMarker(
                "Your application can render a different mouse cursor based on what VanGui::GetMouseCursor() returns. "
                "If software cursor rendering (io.MouseDrawCursor) is set VanGui will draw the right cursor for you, "
                "otherwise your backend needs to handle it.");
            for (int i = 0; i < VanGuiMouseCursor_COUNT; i++)
            {
                char label[32];
                snprintf(label, sizeof(label), "Mouse cursor %d: %s", i, mouse_cursors_names[i]);
                VanGui::Bullet(); VanGui::Selectable(label, false);
                if (VanGui::IsItemHovered())
                    VanGui::SetMouseCursor(i);
            }
            VanGui::TreePop();
        }

        if (VanGui::TreeNode("Tabbing"))
        {
            VANGUI_DEMO_MARKER("Inputs & Focus/Tabbing");
            VanGui::Text("Use Tab/Shift+Tab to cycle through keyboard editable fields.");
            static char buf[32] = "hello";
            VanGui::InputText("1", buf, VAN_COUNTOF(buf));
            VanGui::InputText("2", buf, VAN_COUNTOF(buf));
            VanGui::InputText("3", buf, VAN_COUNTOF(buf));
            VanGui::PushItemFlag(VanGuiItemFlags_NoTabStop, true);
            VanGui::InputText("4 (tab skip)", buf, VAN_COUNTOF(buf));
            VanGui::SameLine(); HelpMarker("Item won't be cycled through when using TAB or Shift+Tab.");
            VanGui::PopItemFlag();
            VanGui::InputText("5", buf, VAN_COUNTOF(buf));
            VanGui::TreePop();
        }

        if (VanGui::TreeNode("Focus from code"))
        {
            VANGUI_DEMO_MARKER("Inputs & Focus/Focus from code");
            bool focus_1 = VanGui::Button("Focus on 1"); VanGui::SameLine();
            bool focus_2 = VanGui::Button("Focus on 2"); VanGui::SameLine();
            bool focus_3 = VanGui::Button("Focus on 3");
            int has_focus = 0;
            static char buf[128] = "click on a button to set focus";

            if (focus_1) VanGui::SetKeyboardFocusHere();
            VanGui::InputText("1", buf, VAN_COUNTOF(buf));
            if (VanGui::IsItemActive()) has_focus = 1;

            if (focus_2) VanGui::SetKeyboardFocusHere();
            VanGui::InputText("2", buf, VAN_COUNTOF(buf));
            if (VanGui::IsItemActive()) has_focus = 2;

            VanGui::PushItemFlag(VanGuiItemFlags_NoTabStop, true);
            if (focus_3) VanGui::SetKeyboardFocusHere();
            VanGui::InputText("3 (tab skip)", buf, VAN_COUNTOF(buf));
            if (VanGui::IsItemActive()) has_focus = 3;
            VanGui::SameLine(); HelpMarker("Item won't be cycled through when using TAB or Shift+Tab.");
            VanGui::PopItemFlag();

            if (has_focus)
                VanGui::Text("Item with focus: %d", has_focus);
            else
                VanGui::Text("Item with focus: <none>");

            // Use >= 0 parameter to SetKeyboardFocusHere() to focus an upcoming item
            static float f3[3] = { 0.0f, 0.0f, 0.0f };
            int focus_ahead = -1;
            if (VanGui::Button("Focus on X")) { focus_ahead = 0; } VanGui::SameLine();
            if (VanGui::Button("Focus on Y")) { focus_ahead = 1; } VanGui::SameLine();
            if (VanGui::Button("Focus on Z")) { focus_ahead = 2; }
            if (focus_ahead != -1) VanGui::SetKeyboardFocusHere(focus_ahead);
            VanGui::SliderFloat3("Float3", &f3[0], 0.0f, 1.0f);

            VanGui::TextWrapped("NB: Cursor & selection are preserved when refocusing last used item in code.");
            VanGui::TreePop();
        }

        if (VanGui::TreeNode("Dragging"))
        {
            VANGUI_DEMO_MARKER("Inputs & Focus/Dragging");
            VanGui::TextWrapped("You can use VanGui::GetMouseDragDelta(0) to query for the dragged amount on any widget.");
            for (int button = 0; button < 3; button++)
            {
                VanGui::Text("IsMouseDragging(%d):", button);
                VanGui::Text("  w/ default threshold: %d,", VanGui::IsMouseDragging(button));
                VanGui::Text("  w/ zero threshold: %d,", VanGui::IsMouseDragging(button, 0.0f));
                VanGui::Text("  w/ large threshold: %d,", VanGui::IsMouseDragging(button, 20.0f));
            }

            VanGui::Button("Drag Me");
            if (VanGui::IsItemActive())
                VanGui::GetForegroundDrawList()->AddLine(io.MouseClickedPos[0], io.MousePos, VanGui::GetColorU32(VanGuiCol_Button), 4.0f); // Draw a line between the button and the mouse cursor

            // Drag operations gets "unlocked" when the mouse has moved past a certain threshold
            // (the default threshold is stored in io.MouseDragThreshold). You can request a lower or higher
            // threshold using the second parameter of IsMouseDragging() and GetMouseDragDelta().
            VanVec2 value_raw = VanGui::GetMouseDragDelta(0, 0.0f);
            VanVec2 value_with_lock_threshold = VanGui::GetMouseDragDelta(0);
            VanVec2 mouse_delta = io.MouseDelta;
            VanGui::Text("GetMouseDragDelta(0):");
            VanGui::Text("  w/ default threshold: (%.1f, %.1f)", value_with_lock_threshold.x, value_with_lock_threshold.y);
            VanGui::Text("  w/ zero threshold: (%.1f, %.1f)", value_raw.x, value_raw.y);
            VanGui::Text("io.MouseDelta: (%.1f, %.1f)", mouse_delta.x, mouse_delta.y);
            VanGui::TreePop();
        }
    }
}

//-----------------------------------------------------------------------------
// [SECTION] About Window / ShowAboutWindow()
// Access from VanGUI Demo -> Tools -> About
//-----------------------------------------------------------------------------

void VanGui::ShowAboutWindow(bool* p_open)
{
    if (!VanGui::Begin("About VanGUI", p_open, VanGuiWindowFlags_AlwaysAutoResize))
    {
        VanGui::End();
        return;
    }
    VANGUI_DEMO_MARKER("Tools/About VanGUI");
    VanGui::Text("VanGUI %s (%d)", VANGUI_VERSION, VANGUI_VERSION_NUM);

    VanGui::TextLinkOpenURL("Homepage", "https://github.com/ocornut/vangui");
    VanGui::SameLine();
    VanGui::TextLinkOpenURL("FAQ", "https://github.com/ocornut/vangui/blob/master/docs/FAQ.md");
    VanGui::SameLine();
    VanGui::TextLinkOpenURL("Wiki", "https://github.com/ocornut/vangui/wiki");
    VanGui::SameLine();
    VanGui::TextLinkOpenURL("Extensions", "https://github.com/ocornut/vangui/wiki/Useful-Extensions");
    VanGui::SameLine();
    VanGui::TextLinkOpenURL("Releases", "https://github.com/ocornut/vangui/releases");
    VanGui::SameLine();
    VanGui::TextLinkOpenURL("Funding", "https://github.com/ocornut/vangui/wiki/Funding");

    VanGui::Separator();
    VanGui::Text("(c) 2014-2026 Omar Cornut");
    VanGui::Text("Developed by Omar Cornut and all VanGUI contributors.");
    VanGui::Text("VanGUI is licensed under the MIT License, see LICENSE for more information.");
    VanGui::Text("If your company uses this, please consider funding the project.");

    static bool show_config_info = false;
    VanGui::Checkbox("Config/Build Information", &show_config_info);
    if (show_config_info)
    {
        VanGuiIO& io = VanGui::GetIO();
        VanGuiStyle& style = VanGui::GetStyle();

        bool copy_to_clipboard = VanGui::Button("Copy to clipboard");
        VanVec2 child_size = VanVec2(0, VanGui::GetTextLineHeightWithSpacing() * 18);
        VanGui::BeginChild(VanGui::GetID("cfg_infos"), child_size, VanGuiChildFlags_FrameStyle);
        if (copy_to_clipboard)
        {
            VanGui::LogToClipboard();
            VanGui::LogText("// (Copy from the next line. Keep the ``` markers for formatting.)\n");
            VanGui::LogText("```cpp\n"); // Back quotes will make text appears without formatting when pasting on GitHub
        }

        VanGui::Text("VanGUI %s (%d)", VANGUI_VERSION, VANGUI_VERSION_NUM);
        VanGui::Separator();
        VanGui::Text("sizeof(size_t): %d, sizeof(VanDrawIdx): %d, sizeof(VanDrawVert): %d", static_cast<int>(sizeof(size_t)), static_cast<int>(sizeof(VanDrawIdx)), static_cast<int>(sizeof(VanDrawVert)));
        VanGui::Text("define: __cplusplus=%d", static_cast<int>(__cplusplus));
#ifdef VANGUI_ENABLE_TEST_ENGINE
        VanGui::Text("define: VANGUI_ENABLE_TEST_ENGINE");
#endif
#ifdef VANGUI_DISABLE_OBSOLETE_FUNCTIONS
        VanGui::Text("define: VANGUI_DISABLE_OBSOLETE_FUNCTIONS");
#endif
#ifdef VANGUI_DISABLE_WIN32_DEFAULT_CLIPBOARD_FUNCTIONS
        VanGui::Text("define: VANGUI_DISABLE_WIN32_DEFAULT_CLIPBOARD_FUNCTIONS");
#endif
#ifdef VANGUI_DISABLE_WIN32_DEFAULT_IME_FUNCTIONS
        VanGui::Text("define: VANGUI_DISABLE_WIN32_DEFAULT_IME_FUNCTIONS");
#endif
#ifdef VANGUI_DISABLE_WIN32_FUNCTIONS
        VanGui::Text("define: VANGUI_DISABLE_WIN32_FUNCTIONS");
#endif
#ifdef VANGUI_DISABLE_DEFAULT_SHELL_FUNCTIONS
        VanGui::Text("define: VANGUI_DISABLE_DEFAULT_SHELL_FUNCTIONS");
#endif
#ifdef VANGUI_DISABLE_DEFAULT_FORMAT_FUNCTIONS
        VanGui::Text("define: VANGUI_DISABLE_DEFAULT_FORMAT_FUNCTIONS");
#endif
#ifdef VANGUI_DISABLE_DEFAULT_MATH_FUNCTIONS
        VanGui::Text("define: VANGUI_DISABLE_DEFAULT_MATH_FUNCTIONS");
#endif
#ifdef VANGUI_DISABLE_DEFAULT_FILE_FUNCTIONS
        VanGui::Text("define: VANGUI_DISABLE_DEFAULT_FILE_FUNCTIONS");
#endif
#ifdef VANGUI_DISABLE_FILE_FUNCTIONS
        VanGui::Text("define: VANGUI_DISABLE_FILE_FUNCTIONS");
#endif
#ifdef VANGUI_DISABLE_DEFAULT_ALLOCATORS
        VanGui::Text("define: VANGUI_DISABLE_DEFAULT_ALLOCATORS");
#endif
#ifdef VANGUI_USE_BGRA_PACKED_COLOR
        VanGui::Text("define: VANGUI_USE_BGRA_PACKED_COLOR");
#endif
#ifdef _WIN32
        VanGui::Text("define: _WIN32");
#endif
#ifdef _WIN64
        VanGui::Text("define: _WIN64");
#endif
#ifdef __linux__
        VanGui::Text("define: __linux__");
#endif
#ifdef __APPLE__
        VanGui::Text("define: __APPLE__");
#endif
#ifdef _MSC_VER
        VanGui::Text("define: _MSC_VER=%d", _MSC_VER);
#endif
#ifdef _MSVC_LANG
        VanGui::Text("define: _MSVC_LANG=%d", static_cast<int>(_MSVC_LANG));
#endif
#ifdef __MINGW32__
        VanGui::Text("define: __MINGW32__");
#endif
#ifdef __MINGW64__
        VanGui::Text("define: __MINGW64__");
#endif
#ifdef __GNUC__
        VanGui::Text("define: __GNUC__=%d", static_cast<int>(__GNUC__));
#endif
#ifdef __clang_version__
        VanGui::Text("define: __clang_version__=%s", __clang_version__);
#endif
#ifdef __EMSCRIPTEN__
        VanGui::Text("define: __EMSCRIPTEN__");
#ifdef __EMSCRIPTEN_MAJOR__
        VanGui::Text("Emscripten: %d.%d.%d", __EMSCRIPTEN_MAJOR__, __EMSCRIPTEN_MINOR__, __EMSCRIPTEN_TINY__);
#else
        VanGui::Text("Emscripten: %d.%d.%d", __EMSCRIPTEN_major__, __EMSCRIPTEN_minor__, __EMSCRIPTEN_tiny__);
#endif
#endif
#ifdef NDEBUG
        VanGui::Text("define: NDEBUG");
#endif

        // Heuristic to detect no-op VAN_ASSERT() macros
        // - This is designed so people opening bug reports would convey and notice that they have disabled asserts for VanGUI code.
        // - 16 is > strlen("((void)(_EXPR))") which we suggested in our vanconfig.h template as a possible way to disable.
        int assert_runs_expression = 0;
        VAN_ASSERT(++assert_runs_expression);
        int assert_expand_len = static_cast<int>(strlen(VAN_STRINGIFY((VAN_ASSERT(true)))));
        bool assert_maybe_disabled = (!assert_runs_expression || assert_expand_len <= 16);
        VanGui::Text("VAN_ASSERT: runs expression: %s. expand size: %s%s",
            assert_runs_expression ? "OK" : "KO", (assert_expand_len > 16) ? "OK" : "KO", assert_maybe_disabled ? " (MAYBE DISABLED?!)" : "");
        if (assert_maybe_disabled)
        {
            VanGui::SameLine();
            HelpMarker("VAN_ASSERT() calls assert() by default. Compiling with NDEBUG will usually strip out assert() to nothing, which is NOT recommended because we use asserts to notify of programmer mistakes!");
        }

        VanGui::Separator();
        VanGui::Text("io.BackendPlatformName: %s", io.BackendPlatformName ? io.BackendPlatformName : "nullptr");
        VanGui::Text("io.BackendRendererName: %s", io.BackendRendererName ? io.BackendRendererName : "nullptr");
        VanGui::Text("io.ConfigFlags: 0x%08X", io.ConfigFlags);
        if (io.ConfigFlags & VanGuiConfigFlags_NavEnableKeyboard)        VanGui::Text(" NavEnableKeyboard");
        if (io.ConfigFlags & VanGuiConfigFlags_NavEnableGamepad)         VanGui::Text(" NavEnableGamepad");
        if (io.ConfigFlags & VanGuiConfigFlags_NoMouse)                  VanGui::Text(" NoMouse");
        if (io.ConfigFlags & VanGuiConfigFlags_NoMouseCursorChange)      VanGui::Text(" NoMouseCursorChange");
        if (io.ConfigFlags & VanGuiConfigFlags_NoKeyboard)               VanGui::Text(" NoKeyboard");
        if (io.MouseDrawCursor)                                         VanGui::Text("io.MouseDrawCursor");
        if (io.ConfigMacOSXBehaviors)                                   VanGui::Text("io.ConfigMacOSXBehaviors");
        if (io.ConfigNavMoveSetMousePos)                                VanGui::Text("io.ConfigNavMoveSetMousePos");
        if (io.ConfigNavCaptureKeyboard)                                VanGui::Text("io.ConfigNavCaptureKeyboard");
        if (io.ConfigInputTextCursorBlink)                              VanGui::Text("io.ConfigInputTextCursorBlink");
        if (io.ConfigWindowsResizeFromEdges)                            VanGui::Text("io.ConfigWindowsResizeFromEdges");
        if (io.ConfigWindowsMoveFromTitleBarOnly)                       VanGui::Text("io.ConfigWindowsMoveFromTitleBarOnly");
        if (io.ConfigMemoryCompactTimer >= 0.0f)                        VanGui::Text("io.ConfigMemoryCompactTimer = %.1f", io.ConfigMemoryCompactTimer);
        VanGui::Text("io.BackendFlags: 0x%08X", io.BackendFlags);
        if (io.BackendFlags & VanGuiBackendFlags_HasGamepad)             VanGui::Text(" HasGamepad");
        if (io.BackendFlags & VanGuiBackendFlags_HasMouseCursors)        VanGui::Text(" HasMouseCursors");
        if (io.BackendFlags & VanGuiBackendFlags_HasSetMousePos)         VanGui::Text(" HasSetMousePos");
        if (io.BackendFlags & VanGuiBackendFlags_RendererHasVtxOffset)   VanGui::Text(" RendererHasVtxOffset");
        if (io.BackendFlags & VanGuiBackendFlags_RendererHasTextures)    VanGui::Text(" RendererHasTextures");
        VanGui::Separator();
        VanGui::Text("io.Fonts: %d fonts, Flags: 0x%08X, TexSize: %d,%d", io.Fonts->Fonts.Size, io.Fonts->Flags, io.Fonts->TexData->Width, io.Fonts->TexData->Height);
        VanGui::Text("io.Fonts->FontLoaderName: %s", io.Fonts->FontLoaderName ? io.Fonts->FontLoaderName : "nullptr");
        VanGui::Text("io.DisplaySize: %.2f,%.2f", io.DisplaySize.x, io.DisplaySize.y);
        VanGui::Text("io.DisplayFramebufferScale: %.2f,%.2f", io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
        VanGui::Separator();
        VanGui::Text("style.WindowPadding: %.2f,%.2f", style.WindowPadding.x, style.WindowPadding.y);
        VanGui::Text("style.WindowBorderSize: %.2f", style.WindowBorderSize);
        VanGui::Text("style.FramePadding: %.2f,%.2f", style.FramePadding.x, style.FramePadding.y);
        VanGui::Text("style.FrameRounding: %.2f", style.FrameRounding);
        VanGui::Text("style.FrameBorderSize: %.2f", style.FrameBorderSize);
        VanGui::Text("style.ItemSpacing: %.2f,%.2f", style.ItemSpacing.x, style.ItemSpacing.y);
        VanGui::Text("style.ItemInnerSpacing: %.2f,%.2f", style.ItemInnerSpacing.x, style.ItemInnerSpacing.y);

        if (copy_to_clipboard)
        {
            VanGui::LogText("\n```\n");
            VanGui::LogFinish();
        }
        VanGui::EndChild();
    }
    VanGui::End();
}

//-----------------------------------------------------------------------------
// [SECTION] Style Editor / ShowStyleEditor()
//-----------------------------------------------------------------------------
// - ShowStyleSelector()
// - ShowStyleEditor()
//-----------------------------------------------------------------------------

// Demo helper function to select among default colors. See ShowStyleEditor() for more advanced options.
bool VanGui::ShowStyleSelector(const char* label)
{
    // FIXME: This is a bit tricky to get right as style are functions, they don't register a name nor the fact that one is active.
    // So we keep track of last active one among our limited selection.
    static int style_idx = -1;
    const char* style_names[] = { "Dark", "Light", "Classic" };
    bool ret = false;
    if (VanGui::BeginCombo(label, (style_idx >= 0 && style_idx < VAN_COUNTOF(style_names)) ? style_names[style_idx] : ""))
    {
        for (int n = 0; n < VAN_COUNTOF(style_names); n++)
        {
            if (VanGui::Selectable(style_names[n], style_idx == n, VanGuiSelectableFlags_SelectOnNav))
            {
                style_idx = n;
                ret = true;
                switch (style_idx)
                {
                case 0: VanGui::StyleColorsDark(); break;
                case 1: VanGui::StyleColorsLight(); break;
                case 2: VanGui::StyleColorsClassic(); break;
                }
            }
            else if (style_idx == n)
                VanGui::SetItemDefaultFocus();
        }
        VanGui::EndCombo();
    }
    return ret;
}

static const char* GetTreeLinesFlagsName(VanGuiTreeNodeFlags flags)
{
    if (flags == VanGuiTreeNodeFlags_DrawLinesNone) return "DrawLinesNone";
    if (flags == VanGuiTreeNodeFlags_DrawLinesFull) return "DrawLinesFull";
    if (flags == VanGuiTreeNodeFlags_DrawLinesToNodes) return "DrawLinesToNodes";
    return "";
}

// We omit the VanGui:: prefix in this function, as we don't expect user to be copy and pasting this code.
void VanGui::ShowStyleEditor(VanGuiStyle* ref)
{
    VANGUI_DEMO_MARKER("Tools/Style Editor");
    // You can pass in a reference VanGuiStyle structure to compare to, revert to and save to
    // (without a reference style pointer, we will use one compared locally as a reference)
    VanGuiStyle& style = GetStyle();
    static VanGuiStyle ref_saved_style;

    // Default to using internal storage as reference
    static bool init = true;
    if (init && ref == nullptr)
        ref_saved_style = style;
    init = false;
    if (ref == nullptr)
        ref = &ref_saved_style;

    // The logic behind dynamically changing 'max_border_size' is to not encourage people to increase border size too much: it'll likely reveal lots of subtle rendering artifacts and this isn't a priority right now.
    // Note that _MainScale is currently internal PLEASE DO NOT USE IN YOUR CODE.
    const float default_border_size = static_cast<float>(static_cast<int>(style._MainScale));
    const float max_border_size = VAN_MAX(default_border_size, 2.0f);

    PushItemWidth(GetWindowWidth() * 0.50f);

    {
        // General
        SeparatorText("General");
        if ((GetIO().BackendFlags & VanGuiBackendFlags_RendererHasTextures) == 0)
        {
            BulletText("Warning: Font scaling will NOT be smooth, because\nVanGuiBackendFlags_RendererHasTextures is not set!");
            BulletText("For instructions, see:");
            SameLine();
            TextLinkOpenURL("docs/BACKENDS.md", "https://github.com/ocornut/vangui/blob/master/docs/BACKENDS.md");
        }

        if (ShowStyleSelector("Colors##Selector"))
            ref_saved_style = style;
        ShowFontSelector("Fonts##Selector");
        if (DragFloat("FontSizeBase", &style.FontSizeBase, 0.20f, 5.0f, 100.0f, "%.0f"))
            style._NextFrameFontSizeBase = style.FontSizeBase; // FIXME: Temporary hack until we finish remaining work.
        SameLine(0.0f, 0.0f); Text(" (out %.2f)", GetFontSize());
        DragFloat("FontScaleMain", &style.FontScaleMain, 0.02f, 0.5f, 4.0f);
        //BeginDisabled(GetIO().ConfigDpiScaleFonts);
        DragFloat("FontScaleDpi", &style.FontScaleDpi, 0.02f, 0.5f, 4.0f);
        //SetItemTooltip("When io.ConfigDpiScaleFonts is set, this value is automatically overwritten.");
        //EndDisabled();

        // Simplified Settings (expose floating-pointer border sizes as boolean representing 0.0f or 1.0f)
        if (SliderFloat("FrameRounding", &style.FrameRounding, 0.0f, 12.0f, "%.0f"))
            style.GrabRounding = style.FrameRounding; // Make GrabRounding always the same value as FrameRounding
        { bool border = (style.WindowBorderSize > 0.0f); if (Checkbox("WindowBorder", &border)) { style.WindowBorderSize = border ? default_border_size : 0.0f; } }
        SameLine();
        { bool border = (style.FrameBorderSize > 0.0f);  if (Checkbox("FrameBorder", &border)) { style.FrameBorderSize = border ? default_border_size : 0.0f; } }
        SameLine();
        { bool border = (style.PopupBorderSize > 0.0f);  if (Checkbox("PopupBorder", &border)) { style.PopupBorderSize = border ? default_border_size : 0.0f; } }
    }

    // Save/Revert button
    if (Button("Save Ref"))
        *ref = ref_saved_style = style;
    SameLine();
    if (Button("Revert Ref"))
        style = *ref;
    SameLine();
    HelpMarker(
        "Save/Revert in local non-persistent storage. Default Colors definition are not affected. "
        "Use \"Export\" below to save them somewhere.");

    SeparatorText("Details");
    if (BeginTabBar("##tabs", VanGuiTabBarFlags_None))
    {
        if (BeginTabItem("Sizes"))
        {
            SeparatorText("Main");
            SliderFloat2("WindowPadding", reinterpret_cast<float*>(&style.WindowPadding), 0.0f, 20.0f, "%.0f");
            SliderFloat2("FramePadding", reinterpret_cast<float*>(&style.FramePadding), 0.0f, 20.0f, "%.0f");
            SliderFloat2("ItemSpacing", reinterpret_cast<float*>(&style.ItemSpacing), 0.0f, 20.0f, "%.0f");
            SliderFloat2("ItemInnerSpacing", reinterpret_cast<float*>(&style.ItemInnerSpacing), 0.0f, 20.0f, "%.0f");
            SliderFloat2("TouchExtraPadding", reinterpret_cast<float*>(&style.TouchExtraPadding), 0.0f, 10.0f, "%.0f");
            SliderFloat("IndentSpacing", &style.IndentSpacing, 0.0f, 30.0f, "%.0f");
            SliderFloat("GrabMinSize", &style.GrabMinSize, 1.0f, 20.0f, "%.0f");

            SeparatorText("Borders");
            SliderFloat("WindowBorderSize", &style.WindowBorderSize, 0.0f, max_border_size, "%.0f");
            SliderFloat("ChildBorderSize", &style.ChildBorderSize, 0.0f, max_border_size, "%.0f");
            SliderFloat("PopupBorderSize", &style.PopupBorderSize, 0.0f, max_border_size, "%.0f");
            SliderFloat("FrameBorderSize", &style.FrameBorderSize, 0.0f, max_border_size, "%.0f");

            SeparatorText("Rounding");
            SliderFloat("WindowRounding", &style.WindowRounding, 0.0f, 12.0f, "%.0f");
            SliderFloat("ChildRounding", &style.ChildRounding, 0.0f, 12.0f, "%.0f");
            SliderFloat("FrameRounding", &style.FrameRounding, 0.0f, 12.0f, "%.0f");
            SliderFloat("PopupRounding", &style.PopupRounding, 0.0f, 12.0f, "%.0f");
            SliderFloat("GrabRounding", &style.GrabRounding, 0.0f, 12.0f, "%.0f");

            SeparatorText("Scrollbar");
            SliderFloat("ScrollbarSize", &style.ScrollbarSize, 1.0f, 20.0f, "%.0f");
            SliderFloat("ScrollbarRounding", &style.ScrollbarRounding, 0.0f, 12.0f, "%.0f");
            SliderFloat("ScrollbarPadding", &style.ScrollbarPadding, 0.0f, 10.0f, "%.0f");

            SeparatorText("Tabs");
            SliderFloat("TabBorderSize", &style.TabBorderSize, 0.0f, max_border_size, "%.0f");
            SliderFloat("TabBarBorderSize", &style.TabBarBorderSize, 0.0f, max_border_size, "%.0f");
            SliderFloat("TabBarOverlineSize", &style.TabBarOverlineSize, 0.0f, VAN_MAX(3.0f, max_border_size), "%.0f");
            SameLine(); HelpMarker("Overline is only drawn over the selected tab when VanGuiTabBarFlags_DrawSelectedOverline is set.");
            DragFloat("TabMinWidthBase", &style.TabMinWidthBase, 0.5f, 1.0f, 500.0f, "%.0f");
            DragFloat("TabMinWidthShrink", &style.TabMinWidthShrink, 0.5f, 1.0f, 500.0f, "%0.f");
            DragFloat("TabCloseButtonMinWidthSelected", &style.TabCloseButtonMinWidthSelected, 0.5f, -1.0f, 100.0f, (style.TabCloseButtonMinWidthSelected < 0.0f) ? "%.0f (Always)" : "%.0f");
            DragFloat("TabCloseButtonMinWidthUnselected", &style.TabCloseButtonMinWidthUnselected, 0.5f, -1.0f, 100.0f, (style.TabCloseButtonMinWidthUnselected < 0.0f) ? "%.0f (Always)" : "%.0f");
            SliderFloat("TabRounding", &style.TabRounding, 0.0f, 12.0f, "%.0f");

            SeparatorText("Tables");
            SliderFloat2("CellPadding", reinterpret_cast<float*>(&style.CellPadding), 0.0f, 20.0f, "%.0f");
            SliderAngle("TableAngledHeadersAngle", &style.TableAngledHeadersAngle, -50.0f, +50.0f);
            SliderFloat2("TableAngledHeadersTextAlign", reinterpret_cast<float*>(&style.TableAngledHeadersTextAlign), 0.0f, 1.0f, "%.2f");

            SeparatorText("Trees");
            bool combo_open = BeginCombo("TreeLinesFlags", GetTreeLinesFlagsName(style.TreeLinesFlags));
            SameLine();
            HelpMarker("[Experimental] Tree lines may not work in all situations (e.g. using a clipper) and may incurs slight traversal overhead.\n\nVanGuiTreeNodeFlags_DrawLinesFull is faster than VanGuiTreeNodeFlags_DrawLinesToNode.");
            if (combo_open)
            {
                const VanGuiTreeNodeFlags options[] = { VanGuiTreeNodeFlags_DrawLinesNone, VanGuiTreeNodeFlags_DrawLinesFull, VanGuiTreeNodeFlags_DrawLinesToNodes };
                for (VanGuiTreeNodeFlags option : options)
                    if (Selectable(GetTreeLinesFlagsName(option), style.TreeLinesFlags == option))
                        style.TreeLinesFlags = option;
                EndCombo();
            }
            SliderFloat("TreeLinesSize", &style.TreeLinesSize, 0.0f, max_border_size, "%.0f");
            SliderFloat("TreeLinesRounding", &style.TreeLinesRounding, 0.0f, 12.0f, "%.0f");

            SeparatorText("Windows");
            SliderFloat2("WindowTitleAlign", reinterpret_cast<float*>(&style.WindowTitleAlign), 0.0f, 1.0f, "%.2f");
            SliderFloat("WindowBorderHoverPadding", &style.WindowBorderHoverPadding, 1.0f, 20.0f, "%.0f");
            int window_menu_button_position = style.WindowMenuButtonPosition + 1;
            if (Combo("WindowMenuButtonPosition", reinterpret_cast<int*>(&window_menu_button_position), "None\0Left\0Right\0"))
                style.WindowMenuButtonPosition = static_cast<VanGuiDir>(window_menu_button_position - 1);

            SeparatorText("Widgets");
            SliderFloat("ColorMarkerSize", &style.ColorMarkerSize, 0.0f, 8.0f, "%.0f");
            Combo("ColorButtonPosition", reinterpret_cast<int*>(&style.ColorButtonPosition), "Left\0Right\0");
            SliderFloat2("ButtonTextAlign", reinterpret_cast<float*>(&style.ButtonTextAlign), 0.0f, 1.0f, "%.2f");
            SameLine(); HelpMarker("Alignment applies when a button is larger than its text content.");
            SliderFloat2("SelectableTextAlign", reinterpret_cast<float*>(&style.SelectableTextAlign), 0.0f, 1.0f, "%.2f");
            SameLine(); HelpMarker("Alignment applies when a selectable is larger than its text content.");
            SliderFloat("SeparatorSize", &style.SeparatorSize, 0.0f, 10.0f, "%.0f");
            SliderFloat("SeparatorTextBorderSize", &style.SeparatorTextBorderSize, 0.0f, 10.0f, "%.0f");
            SliderFloat2("SeparatorTextAlign", reinterpret_cast<float*>(&style.SeparatorTextAlign), 0.0f, 1.0f, "%.2f");
            SliderFloat2("SeparatorTextPadding", reinterpret_cast<float*>(&style.SeparatorTextPadding), 0.0f, 40.0f, "%.0f");
            SliderFloat("LogSliderDeadzone", &style.LogSliderDeadzone, 0.0f, 12.0f, "%.0f");
            SliderFloat("ImageRounding", &style.ImageRounding, 0.0f, 12.0f, "%.0f");
            SliderFloat("ImageBorderSize", &style.ImageBorderSize, 0.0f, max_border_size, "%.0f");

            SeparatorText("Tooltips");
            for (int n = 0; n < 2; n++)
                if (TreeNodeEx(n == 0 ? "HoverFlagsForTooltipMouse" : "HoverFlagsForTooltipNav"))
                {
                    VanGuiHoveredFlags* p = (n == 0) ? &style.HoverFlagsForTooltipMouse : &style.HoverFlagsForTooltipNav;
                    CheckboxFlags("VanGuiHoveredFlags_DelayNone", p, VanGuiHoveredFlags_DelayNone);
                    CheckboxFlags("VanGuiHoveredFlags_DelayShort", p, VanGuiHoveredFlags_DelayShort);
                    CheckboxFlags("VanGuiHoveredFlags_DelayNormal", p, VanGuiHoveredFlags_DelayNormal);
                    CheckboxFlags("VanGuiHoveredFlags_Stationary", p, VanGuiHoveredFlags_Stationary);
                    CheckboxFlags("VanGuiHoveredFlags_NoSharedDelay", p, VanGuiHoveredFlags_NoSharedDelay);
                    TreePop();
                }

            SeparatorText("Misc");
            SliderFloat2("DisplayWindowPadding", reinterpret_cast<float*>(&style.DisplayWindowPadding), 0.0f, 30.0f, "%.0f"); SameLine(); HelpMarker("Apply to regular windows: amount which we enforce to keep visible when moving near edges of your screen.");
            SliderFloat2("DisplaySafeAreaPadding", reinterpret_cast<float*>(&style.DisplaySafeAreaPadding), 0.0f, 30.0f, "%.0f"); SameLine(); HelpMarker("Apply to every windows, menus, popups, tooltips: amount where we avoid displaying contents. Adjust if you cannot see the edges of your screen (e.g. on a TV where scaling has not been configured).");

            EndTabItem();
        }

        if (BeginTabItem("Colors"))
        {
            static int output_dest = 0;
            static bool output_only_modified = true;
            if (Button("Export"))
            {
                if (output_dest == 0)
                    LogToClipboard();
                else
                    LogToTTY();
                LogText("VanVec4* colors = GetStyle().Colors;" VAN_NEWLINE);
                for (int i = 0; i < VanGuiCol_COUNT; i++)
                {
                    const VanVec4& col = style.Colors[i];
                    const char* name = GetStyleColorName(i);
                    if (!output_only_modified || memcmp(&col, &ref->Colors[i], sizeof(VanVec4)) != 0)
                        LogText("colors[VanGuiCol_%s]%*s= VanVec4(%.2ff, %.2ff, %.2ff, %.2ff);" VAN_NEWLINE,
                            name, 23 - static_cast<int>(strlen(name)), "", col.x, col.y, col.z, col.w);
                }
                LogFinish();
            }
            SameLine(); SetNextItemWidth(GetFontSize() * 10); Combo("##output_type", &output_dest, "To Clipboard\0To TTY\0");
            SameLine(); Checkbox("Only Modified Colors", &output_only_modified);

            static VanGuiTextFilter filter;
            filter.Draw("Filter colors", GetFontSize() * 16);

            static VanGuiColorEditFlags alpha_flags = 0;
            if (RadioButton("Opaque", alpha_flags == VanGuiColorEditFlags_AlphaOpaque))       { alpha_flags = VanGuiColorEditFlags_AlphaOpaque; } SameLine();
            if (RadioButton("Alpha",  alpha_flags == VanGuiColorEditFlags_None))              { alpha_flags = VanGuiColorEditFlags_None; } SameLine();
            if (RadioButton("Both",   alpha_flags == VanGuiColorEditFlags_AlphaPreviewHalf))  { alpha_flags = VanGuiColorEditFlags_AlphaPreviewHalf; } SameLine();
            HelpMarker(
                "In the color list:\n"
                "Left-click on color square to open color picker,\n"
                "Right-click to open edit options menu.");

            SetNextWindowSizeConstraints(VanVec2(0.0f, GetTextLineHeightWithSpacing() * 10), VanVec2(FLT_MAX, FLT_MAX));
            BeginChild("##colors", VanVec2(0, 0), VanGuiChildFlags_Borders | VanGuiChildFlags_NavFlattened, VanGuiWindowFlags_AlwaysVerticalScrollbar | VanGuiWindowFlags_AlwaysHorizontalScrollbar);
            PushItemWidth(GetFontSize() * -12);
            for (int i = 0; i < VanGuiCol_COUNT; i++)
            {
                const char* name = GetStyleColorName(i);
                if (!filter.PassFilter(name))
                    continue;
                PushID(i);
#ifndef VANGUI_DISABLE_DEBUG_TOOLS
                if (Button("?"))
                    DebugFlashStyleColor((VanGuiCol)i);
                SetItemTooltip("Flash given color to identify places where it is used.");
                SameLine();
#endif
                ColorEdit4("##color", reinterpret_cast<float*>(&style.Colors[i]), VanGuiColorEditFlags_AlphaBar | alpha_flags);
                if (memcmp(&style.Colors[i], &ref->Colors[i], sizeof(VanVec4)) != 0)
                {
                    // Tips: in a real user application, you may want to merge and use an icon font into the main font,
                    // so instead of "Save"/"Revert" you'd use icons!
                    // Read the FAQ and docs/FONTS.md about using icon fonts. It's really easy and super convenient!
                    SameLine(0.0f, style.ItemInnerSpacing.x); if (Button("Save")) { ref->Colors[i] = style.Colors[i]; }
                    SameLine(0.0f, style.ItemInnerSpacing.x); if (Button("Revert")) { style.Colors[i] = ref->Colors[i]; }
                }
                SameLine(0.0f, style.ItemInnerSpacing.x);
                TextUnformatted(name);
                PopID();
            }
            PopItemWidth();
            EndChild();

            EndTabItem();
        }

        if (BeginTabItem("Fonts"))
        {
            VanGuiIO& io = GetIO();
            VanFontAtlas* atlas = io.Fonts;
            ShowFontAtlas(atlas);

            // Post-baking font scaling. Note that this is NOT the nice way of scaling fonts, read below.
            // (we enforce hard clamping manually as by default DragFloat/SliderFloat allows Ctrl+Click text to get out of bounds).
            /*
            SeparatorText("Legacy Scaling");
            const float MIN_SCALE = 0.3f;
            const float MAX_SCALE = 2.0f;
            HelpMarker(
                "Those are old settings provided for convenience.\n"
                "However, the _correct_ way of scaling your UI is currently to reload your font at the designed size, "
                "rebuild the font atlas, and call style.ScaleAllSizes() on a reference VanGuiStyle structure.\n"
                "Using those settings here will give you poor quality results.");
            PushItemWidth(GetFontSize() * 8);
            DragFloat("global scale", &io.FontGlobalScale, 0.005f, MIN_SCALE, MAX_SCALE, "%.2f", VanGuiSliderFlags_AlwaysClamp); // Scale everything
            //static float window_scale = 1.0f;
            //if (DragFloat("window scale", &window_scale, 0.005f, MIN_SCALE, MAX_SCALE, "%.2f", VanGuiSliderFlags_AlwaysClamp)) // Scale only this window
            //    SetWindowFontScale(window_scale);
            PopItemWidth();
            */

            EndTabItem();
        }

        if (BeginTabItem("Rendering"))
        {
            Checkbox("Anti-aliased lines", &style.AntiAliasedLines);
            SameLine();
            HelpMarker("When disabling anti-aliasing lines, you'll probably want to disable borders in your style as well.");

            Checkbox("Anti-aliased lines use texture", &style.AntiAliasedLinesUseTex);
            SameLine();
            HelpMarker("Faster lines using texture data. Require backend to render with bilinear filtering (not point/nearest filtering).");

            Checkbox("Anti-aliased fill", &style.AntiAliasedFill);
            PushItemWidth(GetFontSize() * 8);
            DragFloat("Curve Tessellation Tolerance", &style.CurveTessellationTol, 0.02f, 0.10f, 10.0f, "%.2f");
            if (style.CurveTessellationTol < 0.10f) style.CurveTessellationTol = 0.10f;

            // When editing the "Circle Segment Max Error" value, draw a preview of its effect on auto-tessellated circles.
            DragFloat("Circle Tessellation Max Error", &style.CircleTessellationMaxError , 0.005f, 0.10f, 5.0f, "%.2f", VanGuiSliderFlags_AlwaysClamp);
            const bool show_samples = IsItemActive();
            if (show_samples)
                SetNextWindowPos(GetCursorScreenPos());
            if (show_samples && BeginTooltip())
            {
                TextUnformatted("(R = radius, N = approx number of segments)");
                Spacing();
                VanDrawList* draw_list = GetWindowDrawList();
                const float min_widget_width = CalcTextSize("R: MMM\nN: MMM").x;
                for (int n = 0; n < 8; n++)
                {
                    const float RAD_MIN = 5.0f;
                    const float RAD_MAX = 70.0f;
                    const float rad = RAD_MIN + (RAD_MAX - RAD_MIN) * static_cast<float>(n) / (8.0f - 1.0f);

                    BeginGroup();

                    // N is not always exact here due to how PathArcTo() function work internally
                    Text("R: %.f\nN: %d", rad, draw_list->_CalcCircleAutoSegmentCount(rad));

                    const float canvas_width = VAN_MAX(min_widget_width, rad * 2.0f);
                    const float offset_x     = floorf(canvas_width * 0.5f);
                    const float offset_y     = floorf(RAD_MAX);

                    const VanVec2 p1 = GetCursorScreenPos();
                    draw_list->AddCircle(VanVec2(p1.x + offset_x, p1.y + offset_y), rad, GetColorU32(VanGuiCol_Text));
                    Dummy(VanVec2(canvas_width, RAD_MAX * 2));

                    /*
                    const VanVec2 p2 = GetCursorScreenPos();
                    draw_list->AddCircleFilled(VanVec2(p2.x + offset_x, p2.y + offset_y), rad, GetColorU32(VanGuiCol_Text));
                    Dummy(VanVec2(canvas_width, RAD_MAX * 2));
                    */

                    EndGroup();
                    SameLine();
                }
                EndTooltip();
            }
            SameLine();
            HelpMarker("When drawing circle primitives with \"num_segments == 0\" tessellation will be calculated automatically.");

            DragFloat("Global Alpha", &style.Alpha, 0.005f, 0.20f, 1.0f, "%.2f"); // Not exposing zero here so user doesn't "lose" the UI (zero alpha clips all widgets). But application code could have a toggle to switch between zero and non-zero.
            DragFloat("Disabled Alpha", &style.DisabledAlpha, 0.005f, 0.0f, 1.0f, "%.2f"); SameLine(); HelpMarker("Additional alpha multiplier for disabled items (multiply over current value of Alpha).");
            PopItemWidth();

            EndTabItem();
        }

        EndTabBar();
    }
    PopItemWidth();
}

//-----------------------------------------------------------------------------
// [SECTION] User Guide / ShowUserGuide()
//-----------------------------------------------------------------------------

// We omit the VanGui:: prefix in this function, as we don't expect user to be copy and pasting this code.
void VanGui::ShowUserGuide()
{
    VanGuiIO& io = GetIO();
    BulletText("Double-click on title bar to collapse window.");
    BulletText(
        "Click and drag on lower corner or border to resize window.\n"
        "(double-click to auto fit window to its contents)");
    BulletText("Ctrl+Click on a slider or drag box to input value as text.");
    BulletText("Tab/Shift+Tab to cycle through keyboard editable fields.");
    BulletText("Ctrl+Tab/Ctrl+Shift+Tab to focus windows.");
    if (io.FontAllowUserScaling)
        BulletText("Ctrl+Mouse Wheel to zoom window contents.");
    BulletText("While inputting text:\n");
    Indent();
    BulletText("Ctrl+Left/Right to word jump.");
    BulletText("Ctrl+A or double-click to select all.");
    BulletText("Ctrl+X/C/V to use clipboard cut/copy/paste.");
    BulletText("Ctrl+Z to undo, Ctrl+Y/Ctrl+Shift+Z to redo.");
    BulletText("Escape to revert.");
    Unindent();
    BulletText("With Keyboard controls enabled:");
    Indent();
    BulletText("Arrow keys or Home/End/PageUp/PageDown to navigate.");
    BulletText("Space to activate a widget.");
    BulletText("Return to input text into a widget.");
    BulletText("Escape to deactivate a widget, close popup,\nexit a child window or the menu layer, clear focus.");
    BulletText("Alt to jump to the menu layer of a window.");
    BulletText("Menu or Shift+F10 to open a context menu.");
    Unindent();
    BulletText("With Gamepad controls enabled:");
    Indent();
    BulletText("D-Pad: Navigate / Tweak / Resize (in Windowing mode).");
    BulletText("%s Face button: Activate / Open / Toggle. Hold: activate with text input.", io.ConfigNavSwapGamepadButtons ? "East" : "South");
    BulletText("%s Face button: Cancel / Close / Exit.", io.ConfigNavSwapGamepadButtons ? "South" : "East");
    BulletText("West Face button: Toggle Menu. Hold for Windowing mode (Focus/Move/Resize windows).");
    BulletText("North Face button: Open Context Menu.");
    BulletText("L1/R1: Tweak Slower/Faster, Focus Previous/Next (in Windowing Mode).");
    Unindent();
}

//-----------------------------------------------------------------------------
// [SECTION] Example App: Main Menu Bar / ShowExampleAppMainMenuBar()
//-----------------------------------------------------------------------------
// - ShowExampleAppMainMenuBar()
// - ShowExampleMenuFile()
//-----------------------------------------------------------------------------

// Demonstrate creating a "main" fullscreen menu bar and populating it.
// Note the difference between BeginMainMenuBar() and BeginMenuBar():
// - BeginMenuBar() = menu-bar inside current window (which needs the VanGuiWindowFlags_MenuBar flag!)
// - BeginMainMenuBar() = helper to create menu-bar-sized window at the top of the main viewport + call BeginMenuBar() into it.
static void ShowExampleAppMainMenuBar()
{
    if (VanGui::BeginMainMenuBar())
    {
        if (VanGui::BeginMenu("File"))
        {
            VANGUI_DEMO_MARKER("Menu/File");
            ShowExampleMenuFile();
            VanGui::EndMenu();
        }
        if (VanGui::BeginMenu("Edit"))
        {
            VANGUI_DEMO_MARKER("Menu/Edit");
            if (VanGui::MenuItem("Undo", "Ctrl+Z")) {}
            if (VanGui::MenuItem("Redo", "Ctrl+Y", false, false)) {} // Disabled item
            VanGui::Separator();
            if (VanGui::MenuItem("Cut", "Ctrl+X")) {}
            if (VanGui::MenuItem("Copy", "Ctrl+C")) {}
            if (VanGui::MenuItem("Paste", "Ctrl+V")) {}
            VanGui::EndMenu();
        }
        VanGui::EndMainMenuBar();
    }
}

// Note that shortcuts are currently provided for display only
// (future version will add explicit flags to BeginMenu() to request processing shortcuts)
static void ShowExampleMenuFile()
{
    VANGUI_DEMO_MARKER("Examples/Menu");
    VanGui::MenuItem("(demo menu)", nullptr, false, false);
    if (VanGui::MenuItem("New")) {}
    if (VanGui::MenuItem("Open", "Ctrl+O")) {}
    if (VanGui::BeginMenu("Open Recent"))
    {
        VanGui::MenuItem("fish_hat.c");
        VanGui::MenuItem("fish_hat.inl");
        VanGui::MenuItem("fish_hat.h");
        if (VanGui::BeginMenu("More.."))
        {
            VanGui::MenuItem("Hello");
            VanGui::MenuItem("Sailor");
            if (VanGui::BeginMenu("Recurse.."))
            {
                ShowExampleMenuFile();
                VanGui::EndMenu();
            }
            VanGui::EndMenu();
        }
        VanGui::EndMenu();
    }
    if (VanGui::MenuItem("Save", "Ctrl+S")) {}
    if (VanGui::MenuItem("Save As..")) {}

    VanGui::Separator();
    if (VanGui::BeginMenu("Options"))
    {
        VANGUI_DEMO_MARKER("Examples/Menu/Options");
        static bool enabled = true;
        VanGui::MenuItem("Enabled", "", &enabled);
        VanGui::BeginChild("child", VanVec2(0, VanGui::GetTextLineHeightWithSpacing() * 5.0f), VanGuiChildFlags_Borders);
        for (int i = 0; i < 10; i++)
            VanGui::Text("Scrolling Text %d", i);
        VanGui::EndChild();
        static float f = 0.5f;
        static int n = 0;
        VanGui::SliderFloat("Value", &f, 0.0f, 1.0f);
        VanGui::InputFloat("Input", &f, 0.1f);
        VanGui::Combo("Combo", &n, "Yes\0No\0Maybe\0\0");
        VanGui::EndMenu();
    }

    if (VanGui::BeginMenu("Colors"))
    {
        VANGUI_DEMO_MARKER("Examples/Menu/Colors");
        float sz = VanGui::GetTextLineHeight();
        for (int i = 0; i < VanGuiCol_COUNT; i++)
        {
            const char* name = VanGui::GetStyleColorName((VanGuiCol)i);
            VanVec2 p = VanGui::GetCursorScreenPos();
            VanGui::GetWindowDrawList()->AddRectFilled(p, VanVec2(p.x + sz, p.y + sz), VanGui::GetColorU32((VanGuiCol)i));
            VanGui::Dummy(VanVec2(sz, sz));
            VanGui::SameLine();
            VanGui::MenuItem(name);
        }
        VanGui::EndMenu();
    }

    // Here we demonstrate appending again to the "Options" menu (which we already created above)
    // Of course in this demo it is a little bit silly that this function calls BeginMenu("Options") twice.
    // In a real code-base using it would make senses to use this feature from very different code locations.
    if (VanGui::BeginMenu("Options")) // <-- Append!
    {
        VANGUI_DEMO_MARKER("Examples/Menu/Append to an existing menu");
        static bool b = true;
        VanGui::Checkbox("SomeOption", &b);
        VanGui::EndMenu();
    }

    if (VanGui::BeginMenu("Disabled", false)) // Disabled
    {
        VAN_ASSERT(0);
    }
    if (VanGui::MenuItem("Checked", nullptr, true)) {}
    VanGui::Separator();
    if (VanGui::MenuItem("Quit", "Alt+F4")) {}
}

//-----------------------------------------------------------------------------
// [SECTION] Example App: Debug Console / ShowExampleAppConsole()
//-----------------------------------------------------------------------------

// Demonstrate creating a simple console window, with scrolling, filtering, completion and history.
// For the console example, we are using a more C++ like approach of declaring a class to hold both data and functions.
struct ExampleAppConsole
{
    char                  InputBuf[256];
    VanVector<char*>       Items;
    VanVector<const char*> Commands;
    VanVector<char*>       History;
    int                   HistoryPos;    // -1: new line, 0..History.Size-1 browsing history.
    VanGuiTextFilter       Filter;
    bool                  AutoScroll;
    bool                  ScrollToBottom;

    ExampleAppConsole()
    {
        ClearLog();
        memset(InputBuf, 0, sizeof(InputBuf));
        HistoryPos = -1;

        // "CLASSIFY" is here to provide the test case where "C"+[tab] completes to "CL" and display multiple matches.
        Commands.push_back("HELP");
        Commands.push_back("HISTORY");
        Commands.push_back("CLEAR");
        Commands.push_back("CLASSIFY");
        AutoScroll = true;
        ScrollToBottom = false;
        AddLog("Welcome to VanGUI!");
    }
    ~ExampleAppConsole()
    {
        ClearLog();
        for (int i = 0; i < History.Size; i++)
            VanGui::MemFree(History[i]);
    }

    // Portable helpers
    static int   Stricmp(const char* s1, const char* s2)         { int d; while ((d = toupper(*s2) - toupper(*s1)) == 0 && *s1) { s1++; s2++; } return d; }
    static int   Strnicmp(const char* s1, const char* s2, int n) { int d = 0; while (n > 0 && (d = toupper(*s2) - toupper(*s1)) == 0 && *s1) { s1++; s2++; n--; } return d; }
    static char* Strdup(const char* s)                           { VAN_ASSERT(s); size_t len = strlen(s) + 1; void* buf = VanGui::MemAlloc(len); VAN_ASSERT(buf); return (char*)memcpy(buf, (const void*)s, len); }
    static void  Strtrim(char* s)                                { char* str_end = s + strlen(s); while (str_end > s && str_end[-1] == ' ') str_end--; *str_end = 0; }

    void    ClearLog()
    {
        for (int i = 0; i < Items.Size; i++)
            VanGui::MemFree(Items[i]);
        Items.clear();
    }

    void    AddLog(const char* fmt, ...) VAN_FMTARGS(2)
    {
        // FIXME-OPT
        char buf[1024];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buf, VAN_COUNTOF(buf), fmt, args);
        buf[VAN_COUNTOF(buf)-1] = 0;
        va_end(args);
        Items.push_back(Strdup(buf));
    }

    void    Draw(const char* title, bool* p_open)
    {
        VanGui::SetNextWindowSize(VanVec2(520, 600), VanGuiCond_FirstUseEver);
        if (!VanGui::Begin(title, p_open))
        {
            VanGui::End();
            return;
        }
        VANGUI_DEMO_MARKER("Examples/Console");

        // As a specific feature guaranteed by the library, after calling Begin() the last Item represent the title bar.
        // So e.g. IsItemHovered() will return true when hovering the title bar.
        // Here we create a context menu only available from the title bar.
        if (VanGui::BeginPopupContextItem())
        {
            if (VanGui::MenuItem("Close Console"))
                *p_open = false;
            VanGui::EndPopup();
        }

        VanGui::TextWrapped(
            "This example implements a console with basic coloring, completion (TAB key) and history (Up/Down keys). A more elaborate "
            "implementation may want to store entries along with extra data such as timestamp, emitter, etc.");
        VanGui::TextWrapped("Enter 'HELP' for help.");

        // TODO: display items starting from the bottom

        if (VanGui::SmallButton("Add Debug Text"))  { AddLog("%d some text", Items.Size); AddLog("some more text"); AddLog("display very important message here!"); }
        VanGui::SameLine();
        if (VanGui::SmallButton("Add Debug Error")) { AddLog("[error] something went wrong"); }
        VanGui::SameLine();
        if (VanGui::SmallButton("Clear"))           { ClearLog(); }
        VanGui::SameLine();
        bool copy_to_clipboard = VanGui::SmallButton("Copy");
        //static float t = 0.0f; if (VanGui::GetTime() - t > 0.02f) { t = VanGui::GetTime(); AddLog("Spam %f", t); }

        VanGui::Separator();

        // Options menu
        if (VanGui::BeginPopup("Options"))
        {
            VanGui::Checkbox("Auto-scroll", &AutoScroll);
            VanGui::EndPopup();
        }

        // Options, Filter
        VanGui::SetNextItemShortcut(VanGuiMod_Ctrl | VanGuiKey_O, VanGuiInputFlags_Tooltip);
        if (VanGui::Button("Options"))
            VanGui::OpenPopup("Options");
        VanGui::SameLine();
        Filter.Draw("Filter (\"incl,-excl\") (\"error\")", 180);
        VanGui::Separator();

        // Reserve enough left-over height for 1 separator + 1 input text
        VanGuiStyle& style = VanGui::GetStyle();
        const float footer_height_to_reserve = style.SeparatorSize + style.ItemSpacing.y + VanGui::GetFrameHeightWithSpacing();
        if (VanGui::BeginChild("ScrollingRegion", VanVec2(0, -footer_height_to_reserve), VanGuiChildFlags_NavFlattened, VanGuiWindowFlags_HorizontalScrollbar))
        {
            if (VanGui::BeginPopupContextWindow())
            {
                if (VanGui::Selectable("Clear")) ClearLog();
                VanGui::EndPopup();
            }

            // Display every line as a separate entry so we can change their color or add custom widgets.
            // If you only want raw text you can use VanGui::TextUnformatted(log.begin(), log.end());
            // NB- if you have thousands of entries this approach may be too inefficient and may require user-side clipping
            // to only process visible items. The clipper will automatically measure the height of your first item and then
            // "seek" to display only items in the visible area.
            // To use the clipper we can replace your standard loop:
            //      for (int i = 0; i < Items.Size; i++)
            //   With:
            //      VanGuiListClipper clipper;
            //      clipper.Begin(Items.Size);
            //      while (clipper.Step())
            //         for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
            // - That your items are evenly spaced (same height)
            // - That you have cheap random access to your elements (you can access them given their index,
            //   without processing all the ones before)
            // You cannot this code as-is if a filter is active because it breaks the 'cheap random-access' property.
            // We would need random-access on the post-filtered list.
            // A typical application wanting coarse clipping and filtering may want to pre-compute an array of indices
            // or offsets of items that passed the filtering test, recomputing this array when user changes the filter,
            // and appending newly elements as they are inserted. This is left as a task to the user until we can manage
            // to improve this example code!
            // If your items are of variable height:
            // - Split them into same height items would be simpler and facilitate random-seeking into your list.
            // - Consider using manual call to IsRectVisible() and skipping extraneous decoration from your items.
            VanGui::PushStyleVar(VanGuiStyleVar_ItemSpacing, VanVec2(4, 1)); // Tighten spacing
            if (copy_to_clipboard)
                VanGui::LogToClipboard();
            for (const char* item : Items)
            {
                if (!Filter.PassFilter(item))
                    continue;

                // Normally you would store more information in your item than just a string.
                // (e.g. make Items[] an array of structure, store color/type etc.)
                VanVec4 color;
                bool has_color = false;
                if (strstr(item, "[error]")) { color = VanVec4(1.0f, 0.4f, 0.4f, 1.0f); has_color = true; }
                else if (strncmp(item, "# ", 2) == 0) { color = VanVec4(1.0f, 0.8f, 0.6f, 1.0f); has_color = true; }
                if (has_color)
                    VanGui::PushStyleColor(VanGuiCol_Text, color);
                VanGui::TextUnformatted(item);
                if (has_color)
                    VanGui::PopStyleColor();
            }
            if (copy_to_clipboard)
                VanGui::LogFinish();

            // Keep up at the bottom of the scroll region if we were already at the bottom at the beginning of the frame.
            // Using a scrollbar or mouse-wheel will take away from the bottom edge.
            if (ScrollToBottom || (AutoScroll && VanGui::GetScrollY() >= VanGui::GetScrollMaxY()))
                VanGui::SetScrollHereY(1.0f);
            ScrollToBottom = false;

            VanGui::PopStyleVar();
        }
        VanGui::EndChild();
        VanGui::Separator();

        // Command-line
        bool reclaim_focus = false;
        VanGuiInputTextFlags input_text_flags = VanGuiInputTextFlags_EnterReturnsTrue | VanGuiInputTextFlags_EscapeClearsAll | VanGuiInputTextFlags_CallbackCompletion | VanGuiInputTextFlags_CallbackHistory;
        if (VanGui::InputText("Input", InputBuf, VAN_COUNTOF(InputBuf), input_text_flags, &TextEditCallbackStub, static_cast<void*>(this)))
        {
            char* s = InputBuf;
            Strtrim(s);
            if (s[0])
                ExecCommand(s);
            strcpy(s, "");
            reclaim_focus = true;
        }

        // Auto-focus on window apparition
        VanGui::SetItemDefaultFocus();
        if (reclaim_focus)
            VanGui::SetKeyboardFocusHere(-1); // Auto focus previous widget

        VanGui::End();
    }

    void    ExecCommand(const char* command_line)
    {
        AddLog("# %s\n", command_line);

        // Insert into history. First find match and delete it so it can be pushed to the back.
        // This isn't trying to be smart or optimal.
        HistoryPos = -1;
        for (int i = History.Size - 1; i >= 0; i--)
            if (Stricmp(History[i], command_line) == 0)
            {
                VanGui::MemFree(History[i]);
                History.erase(History.begin() + i);
                break;
            }
        History.push_back(Strdup(command_line));

        // Process command
        if (Stricmp(command_line, "CLEAR") == 0)
        {
            ClearLog();
        }
        else if (Stricmp(command_line, "HELP") == 0)
        {
            AddLog("Commands:");
            for (int i = 0; i < Commands.Size; i++)
                AddLog("- %s", Commands[i]);
        }
        else if (Stricmp(command_line, "HISTORY") == 0)
        {
            int first = History.Size - 10;
            for (int i = first > 0 ? first : 0; i < History.Size; i++)
                AddLog("%3d: %s\n", i, History[i]);
        }
        else
        {
            AddLog("Unknown command: '%s'\n", command_line);
        }

        // On command input, we scroll to bottom even if AutoScroll==false
        ScrollToBottom = true;
    }

    // In C++11 you'd be better off using lambdas for this sort of forwarding callbacks
    static int TextEditCallbackStub(VanGuiInputTextCallbackData* data)
    {
        ExampleAppConsole* console = (ExampleAppConsole*)data->UserData;
        return console->TextEditCallback(data);
    }

    int     TextEditCallback(VanGuiInputTextCallbackData* data)
    {
        //AddLog("cursor: %d, selection: %d-%d", data->CursorPos, data->SelectionStart, data->SelectionEnd);
        switch (data->EventFlag)
        {
        case VanGuiInputTextFlags_CallbackCompletion:
            {
                // Example of TEXT COMPLETION

                // Locate beginning of current word
                const char* word_end = data->Buf + data->CursorPos;
                const char* word_start = word_end;
                while (word_start > data->Buf)
                {
                    const char c = word_start[-1];
                    if (c == ' ' || c == '\t' || c == ',' || c == ';')
                        break;
                    word_start--;
                }

                // Build a list of candidates
                VanVector<const char*> candidates;
                for (int i = 0; i < Commands.Size; i++)
                    if (Strnicmp(Commands[i], word_start, static_cast<int>(word_end - word_start)) == 0)
                        candidates.push_back(Commands[i]);

                if (candidates.Size == 0)
                {
                    // No match
                    AddLog("No match for \"%.*s\"!\n", static_cast<int>(word_end - word_start), word_start);
                }
                else if (candidates.Size == 1)
                {
                    // Single match. Delete the beginning of the word and replace it entirely so we've got nice casing.
                    data->DeleteChars(static_cast<int>(word_start - data->Buf), static_cast<int>(word_end - word_start));
                    data->InsertChars(data->CursorPos, candidates[0]);
                    data->InsertChars(data->CursorPos, " ");
                }
                else
                {
                    // Multiple matches. Complete as much as we can..
                    // So inputting "C"+Tab will complete to "CL" then display "CLEAR" and "CLASSIFY" as matches.
                    int match_len = static_cast<int>(word_end - word_start);
                    for (;;)
                    {
                        int c = 0;
                        bool all_candidates_matches = true;
                        for (int i = 0; i < candidates.Size && all_candidates_matches; i++)
                            if (i == 0)
                                c = toupper(candidates[i][match_len]);
                            else if (c == 0 || c != toupper(candidates[i][match_len]))
                                all_candidates_matches = false;
                        if (!all_candidates_matches)
                            break;
                        match_len++;
                    }

                    if (match_len > 0)
                    {
                        data->DeleteChars(static_cast<int>(word_start - data->Buf), static_cast<int>(word_end - word_start));
                        data->InsertChars(data->CursorPos, candidates[0], candidates[0] + match_len);
                    }

                    // List matches
                    AddLog("Possible matches:\n");
                    for (int i = 0; i < candidates.Size; i++)
                        AddLog("- %s\n", candidates[i]);
                }

                break;
            }
        case VanGuiInputTextFlags_CallbackHistory:
            {
                // Example of HISTORY
                const int prev_history_pos = HistoryPos;
                if (data->EventKey == VanGuiKey_UpArrow)
                {
                    if (HistoryPos == -1)
                        HistoryPos = History.Size - 1;
                    else if (HistoryPos > 0)
                        HistoryPos--;
                }
                else if (data->EventKey == VanGuiKey_DownArrow)
                {
                    if (HistoryPos != -1)
                        if (++HistoryPos >= History.Size)
                            HistoryPos = -1;
                }

                // A better implementation would preserve the data on the current input line along with cursor position.
                if (prev_history_pos != HistoryPos)
                {
                    const char* history_str = (HistoryPos >= 0) ? History[HistoryPos] : "";
                    data->DeleteChars(0, data->BufTextLen);
                    data->InsertChars(0, history_str);
                }
            }
        }
        return 0;
    }
};

static void ShowExampleAppConsole(bool* p_open)
{
    static ExampleAppConsole console;
    console.Draw("Example: Console", p_open);
}

//-----------------------------------------------------------------------------
// [SECTION] Example App: Image Viewer / ShowExampleAppImageViewer()
//-----------------------------------------------------------------------------

static void ShowExampleAppImageViewer(bool* p_open)
{
    VanFontAtlas* atlas = VanGui::GetIO().Fonts;
    VanTextureRef tex_ref = atlas->TexRef; // We don't have access to other textures in this demo!
    int tex_w = atlas->TexData->Width;
    int tex_h = atlas->TexData->Height;
    if (VanGui::Begin("Example: Image Viewer", p_open))
    {
        static ExampleImageViewerData image_viewer;
        ExampleImageViewer_DrawOptions(&image_viewer);
        VanVec2 canvas_size = VanGui::GetContentRegionAvail();
        VanVec2 canvas_min_size = VanGui::IsWindowAppearing() ? VanVec2(3.0f * tex_w, 4.0f * tex_h) : VanVec2(1.0f, 1.0f);
        canvas_size = VanVec2(VAN_MAX(canvas_size.x, canvas_min_size.x), VAN_MAX(canvas_size.y, canvas_min_size.y));
        ExampleImageViewer_DrawCanvas(&image_viewer, canvas_size, tex_ref, tex_w, tex_h);
    }
    VanGui::End();
}

//-----------------------------------------------------------------------------
// [SECTION] Example App: Debug Log / ShowExampleAppLog()
//-----------------------------------------------------------------------------

// Usage:
//  static ExampleAppLog my_log;
//  my_log.AddLog("Hello %d world\n", 123);
//  my_log.Draw("title");
struct ExampleAppLog
{
    VanGuiTextBuffer     Buf;
    VanGuiTextFilter     Filter;
    VanVector<int>       LineOffsets; // Index to lines offset. We maintain this with AddLog() calls.
    bool                AutoScroll;  // Keep scrolling if already at the bottom.

    ExampleAppLog()
    {
        AutoScroll = true;
        Clear();
    }

    void    Clear()
    {
        Buf.clear();
        LineOffsets.clear();
        LineOffsets.push_back(0);
    }

    void    AddLog(const char* fmt, ...) VAN_FMTARGS(2)
    {
        int old_size = Buf.size();
        va_list args;
        va_start(args, fmt);
        Buf.appendfv(fmt, args);
        va_end(args);
        for (int new_size = Buf.size(); old_size < new_size; old_size++)
            if (Buf[old_size] == '\n')
                LineOffsets.push_back(old_size + 1);
    }

    void    Draw(const char* title, bool* p_open = nullptr)
    {
        if (!VanGui::Begin(title, p_open))
        {
            VanGui::End();
            return;
        }

        // Options menu
        if (VanGui::BeginPopup("Options"))
        {
            VanGui::Checkbox("Auto-scroll", &AutoScroll);
            VanGui::EndPopup();
        }

        // Main window
        if (VanGui::Button("Options"))
            VanGui::OpenPopup("Options");
        VanGui::SameLine();
        bool clear = VanGui::Button("Clear");
        VanGui::SameLine();
        bool copy = VanGui::Button("Copy");
        VanGui::SameLine();
        Filter.Draw("Filter", -100.0f);

        VanGui::Separator();

        if (VanGui::BeginChild("scrolling", VanVec2(0, 0), VanGuiChildFlags_None, VanGuiWindowFlags_HorizontalScrollbar))
        {
            if (clear)
                Clear();
            if (copy)
                VanGui::LogToClipboard();

            VanGui::PushStyleVar(VanGuiStyleVar_ItemSpacing, VanVec2(0, 0));
            const char* buf = Buf.begin();
            const char* buf_end = Buf.end();
            if (Filter.IsActive())
            {
                // In this example we don't use the clipper when Filter is enabled.
                // This is because we don't have random access to the result of our filter.
                // A real application processing logs with ten of thousands of entries may want to store the result of
                // search/filter.. especially if the filtering function is not trivial (e.g. reg-exp).
                for (int line_no = 0; line_no < LineOffsets.Size; line_no++)
                {
                    const char* line_start = buf + LineOffsets[line_no];
                    const char* line_end = (line_no + 1 < LineOffsets.Size) ? (buf + LineOffsets[line_no + 1] - 1) : buf_end;
                    if (Filter.PassFilter(line_start, line_end))
                        VanGui::TextUnformatted(line_start, line_end);
                }
            }
            else
            {
                // The simplest and easy way to display the entire buffer:
                //   VanGui::TextUnformatted(buf_begin, buf_end);
                // And it'll just work. TextUnformatted() has specialization for large blob of text and will fast-forward
                // to skip non-visible lines. Here we instead demonstrate using the clipper to only process lines that are
                // within the visible area.
                // If you have tens of thousands of items and their processing cost is non-negligible, coarse clipping them
                // on your side is recommended. Using VanGuiListClipper requires
                // - A) random access into your data
                // - B) items all being the  same height,
                // both of which we can handle since we have an array pointing to the beginning of each line of text.
                // When using the filter (in the block of code above) we don't have random access into the data to display
                // anymore, which is why we don't use the clipper. Storing or skimming through the search result would make
                // it possible (and would be recommended if you want to search through tens of thousands of entries).
                VanGuiListClipper clipper;
                clipper.Begin(LineOffsets.Size);
                while (clipper.Step())
                {
                    for (int line_no = clipper.DisplayStart; line_no < clipper.DisplayEnd; line_no++)
                    {
                        const char* line_start = buf + LineOffsets[line_no];
                        const char* line_end = (line_no + 1 < LineOffsets.Size) ? (buf + LineOffsets[line_no + 1] - 1) : buf_end;
                        VanGui::TextUnformatted(line_start, line_end);
                    }
                }
                clipper.End();
            }
            VanGui::PopStyleVar();

            // Keep up at the bottom of the scroll region if we were already at the bottom at the beginning of the frame.
            // Using a scrollbar or mouse-wheel will take away from the bottom edge.
            if (AutoScroll && VanGui::GetScrollY() >= VanGui::GetScrollMaxY())
                VanGui::SetScrollHereY(1.0f);
        }
        VanGui::EndChild();
        VanGui::End();
    }
};

// Demonstrate creating a simple log window with basic filtering.
static void ShowExampleAppLog(bool* p_open)
{
    static ExampleAppLog log;

    // For the demo: add a debug button _BEFORE_ the normal log window contents
    // We take advantage of a rarely used feature: multiple calls to Begin()/End() are appending to the _same_ window.
    // Most of the contents of the window will be added by the log.Draw() call.
    VanGui::SetNextWindowSize(VanVec2(500, 400), VanGuiCond_FirstUseEver);
    VanGui::Begin("Example: Log", p_open);
    VANGUI_DEMO_MARKER("Examples/Log");
    if (VanGui::SmallButton("[Debug] Add 5 entries"))
    {
        static int counter = 0;
        const char* categories[3] = { "info", "warn", "error" };
        const char* words[] = { "Bumfuzzled", "Cattywampus", "Snickersnee", "Abibliophobia", "Absquatulate", "Nincompoop", "Pauciloquent" };
        for (int n = 0; n < 5; n++)
        {
            const char* category = categories[counter % VAN_COUNTOF(categories)];
            const char* word = words[counter % VAN_COUNTOF(words)];
            log.AddLog("[%05d] [%s] Hello, current time is %.1f, here's a word: '%s'\n",
                VanGui::GetFrameCount(), category, VanGui::GetTime(), word);
            counter++;
        }
    }
    VanGui::End();

    // Actually call in the regular Log helper (which will Begin() into the same window as we just did)
    log.Draw("Example: Log", p_open);
}

//-----------------------------------------------------------------------------
// [SECTION] Example App: Simple Layout / ShowExampleAppLayout()
//-----------------------------------------------------------------------------

// Demonstrate create a window with multiple child windows.
static void ShowExampleAppLayout(bool* p_open)
{
    VanGui::SetNextWindowSize(VanVec2(500, 440), VanGuiCond_FirstUseEver);
    if (VanGui::Begin("Example: Simple layout", p_open, VanGuiWindowFlags_MenuBar))
    {
        VANGUI_DEMO_MARKER("Examples/Simple layout");
        if (VanGui::BeginMenuBar())
        {
            if (VanGui::BeginMenu("File"))
            {
                if (VanGui::MenuItem("Close", "Ctrl+W")) { *p_open = false; }
                VanGui::EndMenu();
            }
            VanGui::EndMenuBar();
        }

        // Left
        static int selected = 0;
        {
            VanGui::BeginChild("left pane", VanVec2(150, 0), VanGuiChildFlags_Borders | VanGuiChildFlags_ResizeX);
            for (int i = 0; i < 100; i++)
            {
                char label[128];
                snprintf(label, sizeof(label), "MyObject %d", i);
                if (VanGui::Selectable(label, selected == i, VanGuiSelectableFlags_SelectOnNav))
                    selected = i;
            }
            VanGui::EndChild();
        }
        VanGui::SameLine();

        // Right
        {
            VanGui::BeginGroup();
            VanGui::BeginChild("item view", VanVec2(0, -VanGui::GetFrameHeightWithSpacing())); // Leave room for 1 line below us
            VanGui::Text("MyObject: %d", selected);
            VanGui::Separator();
            if (VanGui::BeginTabBar("##Tabs", VanGuiTabBarFlags_None))
            {
                if (VanGui::BeginTabItem("Description"))
                {
                    VanGui::TextWrapped("Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. ");
                    VanGui::EndTabItem();
                }
                if (VanGui::BeginTabItem("Details"))
                {
                    VanGui::Text("ID: 0123456789");
                    VanGui::EndTabItem();
                }
                VanGui::EndTabBar();
            }
            VanGui::EndChild();
            if (VanGui::Button("Revert")) {}
            VanGui::SameLine();
            if (VanGui::Button("Save")) {}
            VanGui::EndGroup();
        }
    }
    VanGui::End();
}

//-----------------------------------------------------------------------------
// [SECTION] Example App: Property Editor / ShowExampleAppPropertyEditor()
//-----------------------------------------------------------------------------
// Some of the interactions are a bit lack-luster:
// - We would want pressing validating or leaving the filter to somehow restore focus.
// - We may want more advanced filtering (child nodes) and clipper support: both will need extra work.
// - We would want to customize some keyboard interactions to easily keyboard navigate between the tree and the properties.
//-----------------------------------------------------------------------------

struct ExampleAppPropertyEditor
{
    VanGuiTextFilter     Filter;
    ExampleTreeNode*    SelectedNode = nullptr;
    bool                UseClipper = false;

    void Draw(ExampleTreeNode* root_node)
    {
        VANGUI_DEMO_MARKER("Examples/Property editor");

        // Left side: draw tree
        // - Currently using a table to benefit from RowBg feature
        // - Our tree node are all of equal height, facilitating the use of a clipper.
        if (VanGui::BeginChild("##tree", VanVec2(300, 0), VanGuiChildFlags_ResizeX | VanGuiChildFlags_Borders | VanGuiChildFlags_NavFlattened))
        {
            VanGui::PushItemFlag(VanGuiItemFlags_NoNavDefaultFocus, true);
            VanGui::Checkbox("Use Clipper", &UseClipper);
            VanGui::SameLine();
            VanGui::Text("(%d root nodes)", root_node->Childs.Size);
            VanGui::SetNextItemWidth(-FLT_MIN);
            VanGui::SetNextItemShortcut(VanGuiMod_Ctrl | VanGuiKey_F, VanGuiInputFlags_Tooltip);
            if (VanGui::InputTextWithHint("##Filter", "incl,-excl", Filter.InputBuf, VAN_COUNTOF(Filter.InputBuf), VanGuiInputTextFlags_EscapeClearsAll))
                Filter.Build();
            VanGui::PopItemFlag();

            if (VanGui::BeginTable("##list", 1, VanGuiTableFlags_RowBg))
            {
                if (UseClipper)
                    DrawClippedTree(root_node);
                else
                    DrawTree(root_node);
                VanGui::EndTable();
            }
        }
        VanGui::EndChild();

        // Right side: draw properties
        VanGui::SameLine();

        VanGui::BeginGroup(); // Lock X position
        if (ExampleTreeNode* node = SelectedNode)
        {
            VanGui::Text("%s", node->Name);
            VanGui::TextDisabled("UID: 0x%08X", node->UID);
            VanGui::Separator();
            if (VanGui::BeginTable("##properties", 2, VanGuiTableFlags_Resizable | VanGuiTableFlags_ScrollY))
            {
                // Push object ID after we entered the table, so table is shared for all objects
                VanGui::PushID(node->UID);
                VanGui::TableSetupColumn("", VanGuiTableColumnFlags_WidthFixed);
                VanGui::TableSetupColumn("", VanGuiTableColumnFlags_WidthStretch, 2.0f); // Default twice larger
                if (node->HasData)
                {
                    // In a typical application, the structure description would be derived from a data-driven system.
                    // - We try to mimic this with our ExampleMemberInfo structure and the ExampleTreeNodeMemberInfos[] array.
                    // - Limits and some details are hard-coded to simplify the demo.
                    for (const ExampleMemberInfo& field_desc : ExampleTreeNodeMemberInfos)
                    {
                        VanGui::TableNextRow();
                        VanGui::PushID(field_desc.Name);
                        VanGui::TableNextColumn();
                        VanGui::AlignTextToFramePadding();
                        VanGui::TextUnformatted(field_desc.Name);
                        VanGui::TableNextColumn();
                        void* field_ptr = static_cast<void*>(reinterpret_cast<unsigned char*>(node) + field_desc.Offset);
                        switch (field_desc.DataType)
                        {
                        case VanGuiDataType_Bool:
                        {
                            VAN_ASSERT(field_desc.DataCount == 1);
                            VanGui::Checkbox("##Editor", static_cast<bool*>(field_ptr));
                            break;
                        }
                        case VanGuiDataType_S32:
                        {
                            int v_min = INT_MIN, v_max = INT_MAX;
                            VanGui::SetNextItemWidth(-FLT_MIN);
                            VanGui::DragScalarN("##Editor", field_desc.DataType, field_ptr, field_desc.DataCount, 1.0f, &v_min, &v_max);
                            break;
                        }
                        case VanGuiDataType_Float:
                        {
                            float v_min = 0.0f, v_max = 1.0f;
                            VanGui::SetNextItemWidth(-FLT_MIN);
                            VanGui::SliderScalarN("##Editor", field_desc.DataType, field_ptr, field_desc.DataCount, &v_min, &v_max);
                            break;
                        }
                        case VanGuiDataType_String:
                        {
                            VanGui::InputText("##Editor", reinterpret_cast<char*>(field_ptr), 28);
                            break;
                        }
                        }
                        VanGui::PopID();
                    }
                }
                VanGui::PopID();
                VanGui::EndTable();
            }
        }
        VanGui::EndGroup();
    }

    // Custom search filter
    // - Here we apply on root node only.
    // - This does a case insensitive stristr which is pretty heavy. In a real large-scale app you would likely store a filtered list which in turns would be trivial to linearize.
    inline bool IsNodePassingFilter(ExampleTreeNode* node)
    {
        return node->Parent->Parent != nullptr || Filter.PassFilter(node->Name);
    }

    // Basic version, recursive. This is how you would generally draw a tree.
    // - Simple but going to be noticeably costly if you have a large amount of nodes as DrawTreeNode() is called for all of them.
    // - On my desktop PC (2020), for 10K nodes in an optimized build this takes ~1.2 ms
    // - Unlike arrays or grids which are very easy to clip, trees are currently more difficult to clip.
    void DrawTree(ExampleTreeNode* node)
    {
        for (ExampleTreeNode* child : node->Childs)
            if (IsNodePassingFilter(child) && DrawTreeNode(child))
            {
                DrawTree(child);
                VanGui::TreePop();
            }
    }

    // More advanced version. Use a alternative clipping technique: fast-forwarding through non-visible chunks.
    // - On my desktop PC (2020), for 10K nodes in an optimized build this takes ~0.1 ms
    //   (in ExampleTree_CreateDemoTree(), change 'int ROOT_ITEMS_COUNT = 10000' to try with this amount of root nodes).
    // - 1. Use clipper with indeterminate count (items_count = INT_MAX): we need to call SeekCursorForItem() at the end once we know the count.
    // - 2. Use SetNextItemStorageID() to specify ID used for open/close storage, making it easy to call TreeNodeGetOpen() on any arbitrary node.
    // - 3. Linearize tree during traversal: our tree data structure makes it easy to access sibling and parents.
    // - Unlike clipping for a regular array or grid which may be done using random access limited to visible areas,
    //   this technique requires traversing most accessible nodes. This could be made more optimal with extra work,
    //   but this is a decent simplicity<>speed trade-off.
    // See https://github.com/ocornut/vangui/issues/3823 for discussions about this.
    void DrawClippedTree(ExampleTreeNode* root_node)
    {
        ExampleTreeNode* node = root_node->Childs[0]; // First node
        VanGuiListClipper clipper;
        clipper.Begin(INT_MAX);
        while (clipper.Step())
            while (clipper.UserIndex < clipper.DisplayEnd && node != nullptr)
                node = DrawClippedTreeNodeAndAdvanceToNext(&clipper, node);

        // Keep going to count nodes and submit final count so we have a reliable scrollbar.
        // - One could consider caching this value and only refreshing it occasionally e.g. window is focused and an action occurs.
        // - Incorrect but cheap approximation would be to use 'clipper_current_idx = VAN_MAX(clipper_current_idx, root_node->Childs.Size)' instead.
        // - If either of those is implemented, the general cost will approach zero when scrolling is at the top of the tree.
        while (node != nullptr)
            node = DrawClippedTreeNodeAndAdvanceToNext(&clipper, node);
        //clipper.UserIndex = VAN_MAX(clipper.UserIndex, root_node->Childs.Size); // <-- Cheap approximation instead of while() loop above.
        clipper.SeekCursorForItem(clipper.UserIndex);
    }

    ExampleTreeNode* DrawClippedTreeNodeAndAdvanceToNext(VanGuiListClipper* clipper, ExampleTreeNode* node)
    {
        if (IsNodePassingFilter(node))
        {
            // Draw node if within visible range
            bool is_open = false;
            if (clipper->UserIndex >= clipper->DisplayStart && clipper->UserIndex < clipper->DisplayEnd)
            {
                is_open = DrawTreeNode(node);
            }
            else
            {
                is_open = (node->Childs.Size > 0 && VanGui::TreeNodeGetOpen(static_cast<VanGuiID>(node->UID)));
                if (is_open)
                    VanGui::TreePush(node->Name);
            }
            clipper->UserIndex++;

            // Next node: recurse into childs
            if (is_open)
                return node->Childs[0];
        }

        // Next node: next sibling, otherwise move back to parent
        while (node != nullptr)
        {
            if (node->IndexInParent + 1 < node->Parent->Childs.Size)
                return node->Parent->Childs[node->IndexInParent + 1];
            node = node->Parent;
            if (node->Parent == nullptr)
                break;
            VanGui::TreePop();
        }
        return nullptr;
    }

    // To support node with same name we incorporate node->UID into the item ID.
    // (this would more naturally be done using PushID(node->UID) + TreeNodeEx(node->Name, tree_flags),
    //   but it would require in DrawClippedTreeNodeAndAdvanceToNext() to add PushID() before TreePush(), and PopID() after TreePop(),
    //   so instead we use TreeNodeEx(node->UID, tree_flags, "%s", node->Name) here)
    bool DrawTreeNode(ExampleTreeNode* node)
    {
        VanGui::TableNextRow();
        VanGui::TableNextColumn();
        VanGuiTreeNodeFlags tree_flags = VanGuiTreeNodeFlags_None;
        tree_flags |= VanGuiTreeNodeFlags_OpenOnArrow | VanGuiTreeNodeFlags_OpenOnDoubleClick; // Standard opening mode as we are likely to want to add selection afterwards
        tree_flags |= VanGuiTreeNodeFlags_NavLeftJumpsToParent;  // Left arrow support
        tree_flags |= VanGuiTreeNodeFlags_SpanFullWidth;         // Span full width for easier mouse reach
        tree_flags |= VanGuiTreeNodeFlags_DrawLinesToNodes;      // Always draw hierarchy outlines
        if (node == SelectedNode)
            tree_flags |= VanGuiTreeNodeFlags_Selected;          // Draw selection highlight
        if (node->Childs.Size == 0)
            tree_flags |= VanGuiTreeNodeFlags_Leaf | VanGuiTreeNodeFlags_Bullet | VanGuiTreeNodeFlags_NoTreePushOnOpen; // Use _NoTreePushOnOpen + set is_open=false to avoid unnecessarily push/pop on leaves.
        if (node->DataMyBool == false)
            VanGui::PushStyleColor(VanGuiCol_Text, VanGui::GetStyle().Colors[VanGuiCol_TextDisabled]);
        VanGui::SetNextItemStorageID(static_cast<VanGuiID>(node->UID));        // Use node->UID as storage id
        bool is_open = VanGui::TreeNodeEx(reinterpret_cast<void*>(static_cast<intptr_t>(node->UID)), tree_flags, "%s", node->Name);
        if (node->Childs.Size == 0)
            is_open = false;
        if (node->DataMyBool == false)
            VanGui::PopStyleColor();
        if (VanGui::IsItemFocused())
            SelectedNode = node;
        return is_open;
    }
};

// Demonstrate creating a simple property editor.
static void ShowExampleAppPropertyEditor(bool* p_open, VanGuiDemoWindowData* demo_data)
{
    VanGui::SetNextWindowSize(VanVec2(430, 450), VanGuiCond_FirstUseEver);
    if (!VanGui::Begin("Example: Property editor", p_open))
    {
        VanGui::End();
        return;
    }

    VANGUI_DEMO_MARKER("Examples/Property Editor");
    static ExampleAppPropertyEditor property_editor;
    if (demo_data->DemoTree == nullptr)
        demo_data->DemoTree = ExampleTree_CreateDemoTree();
    property_editor.Draw(demo_data->DemoTree);

    VanGui::End();
}

//-----------------------------------------------------------------------------
// [SECTION] Example App: Long Text / ShowExampleAppLongText()
//-----------------------------------------------------------------------------

// Demonstrate/test rendering huge amount of text, and the incidence of clipping.
static void ShowExampleAppLongText(bool* p_open)
{
    VanGui::SetNextWindowSize(VanVec2(520, 600), VanGuiCond_FirstUseEver);
    if (!VanGui::Begin("Example: Long text display", p_open))
    {
        VanGui::End();
        return;
    }
    VANGUI_DEMO_MARKER("Examples/Long text display");

    static int test_type = 0;
    static VanGuiTextBuffer log;
    static int lines = 0;
    VanGui::Text("Printing unusually long amount of text.");
    VanGui::Combo("Test type", &test_type,
        "Single call to TextUnformatted()\0"
        "Multiple calls to Text(), clipped\0"
        "Multiple calls to Text(), not clipped (slow)\0");
    VanGui::Text("Buffer contents: %d lines, %d bytes", lines, log.size());
    if (VanGui::Button("Clear")) { log.clear(); lines = 0; }
    VanGui::SameLine();
    if (VanGui::Button("Add 1000 lines"))
    {
        for (int i = 0; i < 1000; i++)
            log.appendf("%i The quick brown fox jumps over the lazy dog\n", lines + i);
        lines += 1000;
    }
    VanGui::BeginChild("Log");
    switch (test_type)
    {
    case 0:
        // Single call to TextUnformatted() with a big buffer
        VanGui::TextUnformatted(log.begin(), log.end());
        break;
    case 1:
        {
            // Multiple calls to Text(), manually coarsely clipped - demonstrate how to use the VanGuiListClipper helper.
            VanGui::PushStyleVar(VanGuiStyleVar_ItemSpacing, VanVec2(0, 0));
            VanGuiListClipper clipper;
            clipper.Begin(lines);
            while (clipper.Step())
                for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
                    VanGui::Text("%i The quick brown fox jumps over the lazy dog", i);
            VanGui::PopStyleVar();
            break;
        }
    case 2:
        // Multiple calls to Text(), not clipped (slow)
        VanGui::PushStyleVar(VanGuiStyleVar_ItemSpacing, VanVec2(0, 0));
        for (int i = 0; i < lines; i++)
            VanGui::Text("%i The quick brown fox jumps over the lazy dog", i);
        VanGui::PopStyleVar();
        break;
    }
    VanGui::EndChild();
    VanGui::End();
}

//-----------------------------------------------------------------------------
// [SECTION] Example App: Auto Resize / ShowExampleAppAutoResize()
//-----------------------------------------------------------------------------

// Demonstrate creating a window which gets auto-resized according to its content.
static void ShowExampleAppAutoResize(bool* p_open)
{
    if (!VanGui::Begin("Example: Auto-resizing window", p_open, VanGuiWindowFlags_AlwaysAutoResize))
    {
        VanGui::End();
        return;
    }
    VANGUI_DEMO_MARKER("Examples/Auto-resizing window");

    static int lines = 10;
    VanGui::TextUnformatted(
        "Window will resize every-frame to the size of its content.\n"
        "Note that you probably don't want to query the window size to\n"
        "output your content because that would create a feedback loop.");
    VanGui::SliderInt("Number of lines", &lines, 1, 20);
    for (int i = 0; i < lines; i++)
        VanGui::Text("%*sThis is line %d", i * 4, "", i); // Pad with space to extend size horizontally
    VanGui::End();
}

//-----------------------------------------------------------------------------
// [SECTION] Example App: Constrained Resize / ShowExampleAppConstrainedResize()
//-----------------------------------------------------------------------------

// Demonstrate creating a window with custom resize constraints.
// Note that size constraints currently don't work on a docked window (when in 'docking' branch)
static void ShowExampleAppConstrainedResize(bool* p_open)
{
    struct CustomConstraints
    {
        // Helper functions to demonstrate programmatic constraints
        // FIXME: This doesn't take account of decoration size (e.g. title bar), library should make this easier.
        // FIXME: None of the three demos works consistently when resizing from borders.
        static void AspectRatio(VanGuiSizeCallbackData* data)
        {
            float aspect_ratio = *static_cast<float*>(data->UserData);
            data->DesiredSize.y = static_cast<float>(static_cast<int>(data->DesiredSize.x / aspect_ratio));
        }
        static void Square(VanGuiSizeCallbackData* data)
        {
            data->DesiredSize.x = data->DesiredSize.y = VAN_MAX(data->DesiredSize.x, data->DesiredSize.y);
        }
        static void Step(VanGuiSizeCallbackData* data)
        {
            float step = *static_cast<float*>(data->UserData);
            data->DesiredSize = VanVec2(static_cast<float>(static_cast<int>(data->DesiredSize.x / step + 0.5f)) * step, static_cast<float>(static_cast<int>(data->DesiredSize.y / step + 0.5f)) * step);
        }
    };

    const char* test_desc[] =
    {
        "Between 100x100 and 500x500",
        "At least 100x100",
        "Resize vertical + lock current width",
        "Resize horizontal + lock current height",
        "Width Between 400 and 500",
        "Height at least 400",
        "Custom: Aspect Ratio 16:9",
        "Custom: Always Square",
        "Custom: Fixed Steps (100)",
    };

    // Options
    static bool auto_resize = false;
    static bool window_padding = true;
    static int type = 6; // Aspect Ratio
    static int display_lines = 10;

    // Submit constraint
    float aspect_ratio = 16.0f / 9.0f;
    float fixed_step = 100.0f;
    if (type == 0) VanGui::SetNextWindowSizeConstraints(VanVec2(100, 100), VanVec2(500, 500));         // Between 100x100 and 500x500
    if (type == 1) VanGui::SetNextWindowSizeConstraints(VanVec2(100, 100), VanVec2(FLT_MAX, FLT_MAX)); // Width > 100, Height > 100
    if (type == 2) VanGui::SetNextWindowSizeConstraints(VanVec2(-1, 0),    VanVec2(-1, FLT_MAX));      // Resize vertical + lock current width
    if (type == 3) VanGui::SetNextWindowSizeConstraints(VanVec2(0, -1),    VanVec2(FLT_MAX, -1));      // Resize horizontal + lock current height
    if (type == 4) VanGui::SetNextWindowSizeConstraints(VanVec2(400, -1),  VanVec2(500, -1));          // Width Between and 400 and 500
    if (type == 5) VanGui::SetNextWindowSizeConstraints(VanVec2(-1, 400),  VanVec2(-1, FLT_MAX));      // Height at least 400
    if (type == 6) VanGui::SetNextWindowSizeConstraints(VanVec2(0, 0),     VanVec2(FLT_MAX, FLT_MAX), CustomConstraints::AspectRatio, static_cast<void*>(&aspect_ratio));   // Aspect ratio
    if (type == 7) VanGui::SetNextWindowSizeConstraints(VanVec2(0, 0),     VanVec2(FLT_MAX, FLT_MAX), CustomConstraints::Square);                              // Always Square
    if (type == 8) VanGui::SetNextWindowSizeConstraints(VanVec2(0, 0),     VanVec2(FLT_MAX, FLT_MAX), CustomConstraints::Step, static_cast<void*>(&fixed_step));            // Fixed Step

    // Submit window
    if (!window_padding)
        VanGui::PushStyleVar(VanGuiStyleVar_WindowPadding, VanVec2(0.0f, 0.0f));
    const VanGuiWindowFlags window_flags = auto_resize ? VanGuiWindowFlags_AlwaysAutoResize : 0;
    const bool window_open = VanGui::Begin("Example: Constrained Resize", p_open, window_flags);
    if (!window_padding)
        VanGui::PopStyleVar();
    VANGUI_DEMO_MARKER("Examples/Constrained Resizing window");
    if (window_open)
    {
        if (VanGui::GetIO().KeyShift)
        {
            // Display a dummy viewport (in your real app you would likely use ImageButton() to display a texture)
            VanVec2 avail_size = VanGui::GetContentRegionAvail();
            VanVec2 pos = VanGui::GetCursorScreenPos();
            VanGui::ColorButton("viewport", VanVec4(0.5f, 0.2f, 0.5f, 1.0f), VanGuiColorEditFlags_NoTooltip | VanGuiColorEditFlags_NoDragDrop, avail_size);
            VanGui::SetCursorScreenPos(VanVec2(pos.x + 10, pos.y + 10));
            VanGui::Text("%.2f x %.2f", avail_size.x, avail_size.y);
        }
        else
        {
            VanGui::Text("(Hold Shift to display a dummy viewport)");
            if (VanGui::Button("Set 200x200")) { VanGui::SetWindowSize(VanVec2(200, 200)); } VanGui::SameLine();
            if (VanGui::Button("Set 500x500")) { VanGui::SetWindowSize(VanVec2(500, 500)); } VanGui::SameLine();
            if (VanGui::Button("Set 800x200")) { VanGui::SetWindowSize(VanVec2(800, 200)); }
            VanGui::SetNextItemWidth(VanGui::GetFontSize() * 20);
            VanGui::Combo("Constraint", &type, test_desc, VAN_COUNTOF(test_desc));
            VanGui::SetNextItemWidth(VanGui::GetFontSize() * 20);
            VanGui::DragInt("Lines", &display_lines, 0.2f, 1, 100);
            VanGui::Checkbox("Auto-resize", &auto_resize);
            VanGui::Checkbox("Window padding", &window_padding);
            for (int i = 0; i < display_lines; i++)
                VanGui::Text("%*sHello, sailor! Making this line long enough for the example.", i * 4, "");
        }
    }
    VanGui::End();
}

//-----------------------------------------------------------------------------
// [SECTION] Example App: Simple overlay / ShowExampleAppSimpleOverlay()
//-----------------------------------------------------------------------------

// Demonstrate creating a simple static window with no decoration
// + a context-menu to choose which corner of the screen to use.
static void ShowExampleAppSimpleOverlay(bool* p_open)
{
    static int location = 0;
    VanGuiIO& io = VanGui::GetIO();
    VanGuiWindowFlags window_flags = VanGuiWindowFlags_NoDecoration | VanGuiWindowFlags_AlwaysAutoResize | VanGuiWindowFlags_NoSavedSettings | VanGuiWindowFlags_NoFocusOnAppearing | VanGuiWindowFlags_NoNav;
    if (location >= 0)
    {
        const float PAD = 10.0f;
        const VanGuiViewport* viewport = VanGui::GetMainViewport();
        VanVec2 work_pos = viewport->WorkPos; // Use work area to avoid menu-bar/task-bar, if any!
        VanVec2 work_size = viewport->WorkSize;
        VanVec2 window_pos, window_pos_pivot;
        window_pos.x = (location & 1) ? (work_pos.x + work_size.x - PAD) : (work_pos.x + PAD);
        window_pos.y = (location & 2) ? (work_pos.y + work_size.y - PAD) : (work_pos.y + PAD);
        window_pos_pivot.x = (location & 1) ? 1.0f : 0.0f;
        window_pos_pivot.y = (location & 2) ? 1.0f : 0.0f;
        VanGui::SetNextWindowPos(window_pos, VanGuiCond_Always, window_pos_pivot);
        window_flags |= VanGuiWindowFlags_NoMove;
    }
    else if (location == -2)
    {
        // Center window
        VanGui::SetNextWindowPos(VanGui::GetMainViewport()->GetCenter(), VanGuiCond_Always, VanVec2(0.5f, 0.5f));
        window_flags |= VanGuiWindowFlags_NoMove;
    }
    VanGui::SetNextWindowBgAlpha(0.35f); // Transparent background
    if (VanGui::Begin("Example: Simple overlay", p_open, window_flags))
    {
        VANGUI_DEMO_MARKER("Examples/Simple overlay"); // Scroll up to the beginning of this function to see overlay flags
        VanGui::Text("Simple overlay\n" "(right-click to change position)");
        VanGui::Separator();
        if (VanGui::IsMousePosValid())
            VanGui::Text("Mouse Position: (%.1f,%.1f)", io.MousePos.x, io.MousePos.y);
        else
            VanGui::Text("Mouse Position: <invalid>");
        if (VanGui::BeginPopupContextWindow())
        {
            if (VanGui::MenuItem("Custom",       nullptr, location == -1)) location = -1;
            if (VanGui::MenuItem("Center",       nullptr, location == -2)) location = -2;
            if (VanGui::MenuItem("Top-left",     nullptr, location == 0)) location = 0;
            if (VanGui::MenuItem("Top-right",    nullptr, location == 1)) location = 1;
            if (VanGui::MenuItem("Bottom-left",  nullptr, location == 2)) location = 2;
            if (VanGui::MenuItem("Bottom-right", nullptr, location == 3)) location = 3;
            if (p_open && VanGui::MenuItem("Close")) *p_open = false;
            VanGui::EndPopup();
        }
    }
    VanGui::End();
}

//-----------------------------------------------------------------------------
// [SECTION] Example App: Fullscreen window / ShowExampleAppFullscreen()
//-----------------------------------------------------------------------------

// Demonstrate creating a window covering the entire screen/viewport
static void ShowExampleAppFullscreen(bool* p_open)
{
    static bool use_work_area = true;
    static VanGuiWindowFlags flags = VanGuiWindowFlags_NoDecoration | VanGuiWindowFlags_NoMove | VanGuiWindowFlags_NoSavedSettings;

    // We demonstrate using the full viewport area or the work area (without menu-bars, task-bars etc.)
    // Based on your use case you may want one or the other.
    const VanGuiViewport* viewport = VanGui::GetMainViewport();
    VanGui::SetNextWindowPos(use_work_area ? viewport->WorkPos : viewport->Pos);
    VanGui::SetNextWindowSize(use_work_area ? viewport->WorkSize : viewport->Size);

    if (VanGui::Begin("Example: Fullscreen window", p_open, flags))
    {
        VANGUI_DEMO_MARKER("Examples/Fullscreen window");
        VanGui::Checkbox("Use work area instead of main area", &use_work_area);
        VanGui::SameLine();
        HelpMarker("Main Area = entire viewport,\nWork Area = entire viewport minus sections used by the main menu bars, task bars etc.\n\nEnable the main-menu bar in Examples menu to see the difference.");

        VanGui::CheckboxFlags("VanGuiWindowFlags_NoBackground", &flags, VanGuiWindowFlags_NoBackground);
        VanGui::CheckboxFlags("VanGuiWindowFlags_NoDecoration", &flags, VanGuiWindowFlags_NoDecoration);
        VanGui::Indent();
        VanGui::CheckboxFlags("VanGuiWindowFlags_NoTitleBar", &flags, VanGuiWindowFlags_NoTitleBar);
        VanGui::CheckboxFlags("VanGuiWindowFlags_NoCollapse", &flags, VanGuiWindowFlags_NoCollapse);
        VanGui::CheckboxFlags("VanGuiWindowFlags_NoScrollbar", &flags, VanGuiWindowFlags_NoScrollbar);
        VanGui::Unindent();

        if (p_open && VanGui::Button("Close this window"))
            *p_open = false;
    }
    VanGui::End();
}

//-----------------------------------------------------------------------------
// [SECTION] Example App: Manipulating Window Titles / ShowExampleAppWindowTitles()
//-----------------------------------------------------------------------------

// Demonstrate the use of "##" and "###" in identifiers to manipulate ID generation.
// This applies to all regular items as well.
// Read FAQ section "How can I have multiple widgets with the same label?" for details.
static void ShowExampleAppWindowTitles(bool*)
{
    const VanGuiViewport* viewport = VanGui::GetMainViewport();
    const VanVec2 base_pos = viewport->Pos;

    // By default, Windows are uniquely identified by their title.
    // You can use the "##" and "###" markers to manipulate the display/ID.

    // Using "##" to display same title but have unique identifier.
    VanGui::SetNextWindowPos(VanVec2(base_pos.x + 100, base_pos.y + 100), VanGuiCond_FirstUseEver);
    VanGui::Begin("Same title as another window##1");
    VANGUI_DEMO_MARKER("Examples/Manipulating window titles##1");
    VanGui::Text("This is window 1.\nMy title is the same as window 2, but my identifier is unique.");
    VanGui::End();

    VanGui::SetNextWindowPos(VanVec2(base_pos.x + 100, base_pos.y + 200), VanGuiCond_FirstUseEver);
    VanGui::Begin("Same title as another window##2");
    VANGUI_DEMO_MARKER("Examples/Manipulating window titles##2");
    VanGui::Text("This is window 2.\nMy title is the same as window 1, but my identifier is unique.");
    VanGui::End();

    // Using "###" to display a changing title but keep a static identifier "AnimatedTitle"
    char buf[128];
    snprintf(buf, sizeof(buf), "Animated title %c %d###AnimatedTitle", "|/-\\"[static_cast<int>(VanGui::GetTime() / 0.25f) & 3], VanGui::GetFrameCount());
    VanGui::SetNextWindowPos(VanVec2(base_pos.x + 100, base_pos.y + 300), VanGuiCond_FirstUseEver);
    VanGui::Begin(buf);
    VANGUI_DEMO_MARKER("Examples/Manipulating window titles##3");
    VanGui::Text("This window has a changing title.");
    VanGui::End();
}

//-----------------------------------------------------------------------------
// [SECTION] Example App: Custom Rendering using VanDrawList API / ShowExampleAppCustomRendering()
//-----------------------------------------------------------------------------

// Add a |_| looking shape
static void PathConcaveShape(VanDrawList* draw_list, float x, float y, float sz)
{
    const VanVec2 pos_norms[] = { { 0.0f, 0.0f }, { 0.3f, 0.0f }, { 0.3f, 0.7f }, { 0.7f, 0.7f }, { 0.7f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };
    for (const VanVec2& p : pos_norms)
        draw_list->PathLineTo(VanVec2(x + 0.5f + static_cast<int>(sz * p.x), y + 0.5f + static_cast<int>(sz * p.y)));
}

// Demonstrate using the low-level VanDrawList to draw custom shapes.
static void ShowExampleAppCustomRendering(bool* p_open)
{
    if (!VanGui::Begin("Example: Custom rendering", p_open))
    {
        VanGui::End();
        return;
    }
    VANGUI_DEMO_MARKER("Examples/Custom rendering");

    // Tip: If you do a lot of custom rendering, you probably want to use your own geometrical types and benefit of
    // overloaded operators, etc. Define VAN_VEC2_CLASS_EXTRA in vanconfig.h to create implicit conversions between your
    // types and VanVec2/VanVec4. VanGUI defines overloaded operators but they are internal to vangui.cpp and not
    // exposed outside (to avoid messing with your types) In this example we are not using the maths operators!

    if (VanGui::BeginTabBar("##TabBar"))
    {
        if (VanGui::BeginTabItem("Primitives"))
        {
            VANGUI_DEMO_MARKER("Examples/Custom rendering/Primitives");
            VanGui::PushItemWidth(-VanGui::GetFontSize() * 15);
            VanDrawList* draw_list = VanGui::GetWindowDrawList();

            // Draw gradients
            // (note that those are currently exacerbating our sRGB/Linear issues)
            // Calling VanGui::GetColorU32() multiplies the given colors by the current Style Alpha, but you may pass the VAN_COL32() directly as well..
            VanGui::Text("Gradients");
            VanVec2 gradient_size = VanVec2(VanGui::CalcItemWidth(), VanGui::GetFrameHeight());
            {
                VanVec2 p0 = VanGui::GetCursorScreenPos();
                VanVec2 p1 = VanVec2(p0.x + gradient_size.x, p0.y + gradient_size.y);
                VanU32 col_a = VanGui::GetColorU32(VAN_COL32(0, 0, 0, 255));
                VanU32 col_b = VanGui::GetColorU32(VAN_COL32(255, 255, 255, 255));
                draw_list->AddRectFilledMultiColor(p0, p1, col_a, col_b, col_b, col_a);
                VanGui::InvisibleButton("##gradient1", gradient_size);
            }
            {
                VanVec2 p0 = VanGui::GetCursorScreenPos();
                VanVec2 p1 = VanVec2(p0.x + gradient_size.x, p0.y + gradient_size.y);
                VanU32 col_a = VanGui::GetColorU32(VAN_COL32(0, 255, 0, 255));
                VanU32 col_b = VanGui::GetColorU32(VAN_COL32(255, 0, 0, 255));
                draw_list->AddRectFilledMultiColor(p0, p1, col_a, col_b, col_b, col_a);
                VanGui::InvisibleButton("##gradient2", gradient_size);
            }

            // Draw a bunch of primitives
            VanGui::Text("All primitives");
            static float sz = 36.0f;
            static float thickness = 3.0f;
            static int ngon_sides = 6;
            static bool circle_segments_override = false;
            static int circle_segments_override_v = 12;
            static bool curve_segments_override = false;
            static int curve_segments_override_v = 8;
            static VanVec4 colf = VanVec4(1.0f, 1.0f, 0.4f, 1.0f);
            VanGui::DragFloat("Size", &sz, 0.2f, 2.0f, 100.0f, "%.0f");
            VanGui::DragFloat("Thickness", &thickness, 0.05f, 1.0f, 8.0f, "%.02f");
            VanGui::SliderInt("N-gon sides", &ngon_sides, 3, 12);
            VanGui::Checkbox("##circlesegmentoverride", &circle_segments_override);
            VanGui::SameLine(0.0f, VanGui::GetStyle().ItemInnerSpacing.x);
            circle_segments_override |= VanGui::SliderInt("Circle segments override", &circle_segments_override_v, 3, 40);
            VanGui::Checkbox("##curvessegmentoverride", &curve_segments_override);
            VanGui::SameLine(0.0f, VanGui::GetStyle().ItemInnerSpacing.x);
            curve_segments_override |= VanGui::SliderInt("Curves segments override", &curve_segments_override_v, 3, 40);
            VanGui::ColorEdit4("Color", &colf.x);

            const VanVec2 p = VanGui::GetCursorScreenPos();
            const VanU32 col = VanColor(colf);
            const float spacing = 10.0f;
            const VanDrawFlags corners_tl_br = VanDrawFlags_RoundCornersTopLeft | VanDrawFlags_RoundCornersBottomRight;
            const float rounding = sz / 5.0f;
            const int circle_segments = circle_segments_override ? circle_segments_override_v : 0;
            const int curve_segments = curve_segments_override ? curve_segments_override_v : 0;
            const VanVec2 cp3[3] = { VanVec2(0.0f, sz * 0.6f), VanVec2(sz * 0.5f, -sz * 0.4f), VanVec2(sz, sz) }; // Control points for curves
            const VanVec2 cp4[4] = { VanVec2(0.0f, 0.0f), VanVec2(sz * 1.3f, sz * 0.3f), VanVec2(sz - sz * 1.3f, sz - sz * 0.3f), VanVec2(sz, sz) };

            float x = p.x + 4.0f;
            float y = p.y + 4.0f;
            for (int n = 0; n < 2; n++)
            {
                // First line uses a thickness of 1.0f, second line uses the configurable thickness
                float th = (n == 0) ? 1.0f : thickness;
                draw_list->AddNgon(VanVec2(x + sz*0.5f, y + sz*0.5f), sz*0.5f, col, ngon_sides, th);                 x += sz + spacing;  // N-gon
                draw_list->AddCircle(VanVec2(x + sz*0.5f, y + sz*0.5f), sz*0.5f, col, circle_segments, th);          x += sz + spacing;  // Circle
                draw_list->AddEllipse(VanVec2(x + sz*0.5f, y + sz*0.5f), VanVec2(sz*0.5f, sz*0.3f), col, -0.3f, circle_segments, th); x += sz + spacing;  // Ellipse
                draw_list->AddRect(VanVec2(x, y), VanVec2(x + sz, y + sz), col, 0.0f, th);                            x += sz + spacing;  // Square
                draw_list->AddRect(VanVec2(x, y), VanVec2(x + sz, y + sz), col, rounding, th);                        x += sz + spacing;  // Square with all rounded corners
                draw_list->AddRect(VanVec2(x, y), VanVec2(x + sz, y + sz), col, rounding, th, corners_tl_br);         x += sz + spacing;  // Square with two rounded corners
                draw_list->AddTriangle(VanVec2(x+sz*0.5f,y), VanVec2(x+sz, y+sz-0.5f), VanVec2(x, y+sz-0.5f), col, th);x += sz + spacing;  // Triangle
                //draw_list->AddTriangle(VanVec2(x+sz*0.2f,y), VanVec2(x, y+sz-0.5f), VanVec2(x+sz*0.4f, y+sz-0.5f), col, th);x+= sz*0.4f + spacing; // Thin triangle
                PathConcaveShape(draw_list, x, y, sz); draw_list->PathStroke(col, th, VanDrawFlags_Closed);          x += sz + spacing;  // Concave Shape
                //draw_list->AddPolyline(concave_shape, VAN_COUNTOF(concave_shape), col, VanDrawFlags_Closed, th);
                draw_list->AddLineH(x, x + sz, y, col, th);                                                         x += sz + spacing;  // Horizontal line (note: drawing a filled rectangle will be faster!)
                draw_list->AddLineV(x, y, y + sz, col, th);                                                         x += spacing;       // Vertical line (note: drawing a filled rectangle will be faster!)
                draw_list->AddLine(VanVec2(x, y), VanVec2(x + sz, y + sz), col, th);                                  x += sz + spacing;  // Diagonal line

                // Path
                draw_list->PathArcTo(VanVec2(x + sz*0.5f, y + sz*0.5f), sz*0.5f, 3.141592f, 3.141592f * -0.5f);
                draw_list->PathStroke(col, th);
                x += sz + spacing;

                // Quadratic Bezier Curve (3 control points)
                draw_list->AddBezierQuadratic(VanVec2(x + cp3[0].x, y + cp3[0].y), VanVec2(x + cp3[1].x, y + cp3[1].y), VanVec2(x + cp3[2].x, y + cp3[2].y), col, th, curve_segments);
                x += sz + spacing;

                // Cubic Bezier Curve (4 control points)
                draw_list->AddBezierCubic(VanVec2(x + cp4[0].x, y + cp4[0].y), VanVec2(x + cp4[1].x, y + cp4[1].y), VanVec2(x + cp4[2].x, y + cp4[2].y), VanVec2(x + cp4[3].x, y + cp4[3].y), col, th, curve_segments);

                x = p.x + 4;
                y += sz + spacing;
            }

            // Filled shapes
            draw_list->AddNgonFilled(VanVec2(x + sz * 0.5f, y + sz * 0.5f), sz * 0.5f, col, ngon_sides);             x += sz + spacing;  // N-gon
            draw_list->AddCircleFilled(VanVec2(x + sz * 0.5f, y + sz * 0.5f), sz * 0.5f, col, circle_segments);      x += sz + spacing;  // Circle
            draw_list->AddEllipseFilled(VanVec2(x + sz * 0.5f, y + sz * 0.5f), VanVec2(sz * 0.5f, sz * 0.3f), col, -0.3f, circle_segments); x += sz + spacing;// Ellipse
            draw_list->AddRectFilled(VanVec2(x, y), VanVec2(x + sz, y + sz), col);                                    x += sz + spacing;  // Square
            draw_list->AddRectFilled(VanVec2(x, y), VanVec2(x + sz, y + sz), col, 10.0f);                             x += sz + spacing;  // Square with all rounded corners
            draw_list->AddRectFilled(VanVec2(x, y), VanVec2(x + sz, y + sz), col, 10.0f, corners_tl_br);              x += sz + spacing;  // Square with two rounded corners
            draw_list->AddTriangleFilled(VanVec2(x+sz*0.5f,y), VanVec2(x+sz, y+sz-0.5f), VanVec2(x, y+sz-0.5f), col);  x += sz + spacing;  // Triangle
            //draw_list->AddTriangleFilled(VanVec2(x+sz*0.2f,y), VanVec2(x, y+sz-0.5f), VanVec2(x+sz*0.4f, y+sz-0.5f), col); x += sz*0.4f + spacing; // Thin triangle
            PathConcaveShape(draw_list, x, y, sz); draw_list->PathFillConcave(col);                                 x += sz + spacing;  // Concave shape
            draw_list->AddRectFilled(VanVec2(x, y), VanVec2(x + sz, y + thickness), col);                             x += sz + spacing;  // Horizontal line (faster than AddLine, but only handle integer thickness)
            draw_list->AddRectFilled(VanVec2(x, y), VanVec2(x + thickness, y + sz), col);                             x += spacing * 2.0f;// Vertical line (faster than AddLine, but only handle integer thickness)
            draw_list->AddRectFilled(VanVec2(x, y), VanVec2(x + 1, y + 1), col);                                      x += sz;            // Pixel (faster than AddLine)

            // Path
            draw_list->PathArcTo(VanVec2(x + sz * 0.5f, y + sz * 0.5f), sz * 0.5f, 3.141592f * -0.5f, 3.141592f);
            draw_list->PathFillConvex(col);
            x += sz + spacing;

            // Quadratic Bezier Curve (3 control points)
            draw_list->PathLineTo(VanVec2(x + cp3[0].x, y + cp3[0].y));
            draw_list->PathBezierQuadraticCurveTo(VanVec2(x + cp3[1].x, y + cp3[1].y), VanVec2(x + cp3[2].x, y + cp3[2].y), curve_segments);
            draw_list->PathFillConvex(col);
            x += sz + spacing;

            draw_list->AddRectFilledMultiColor(VanVec2(x, y), VanVec2(x + sz, y + sz), VAN_COL32(0, 0, 0, 255), VAN_COL32(255, 0, 0, 255), VAN_COL32(255, 255, 0, 255), VAN_COL32(0, 255, 0, 255));
            x += sz + spacing;

            VanGui::Dummy(VanVec2((sz + spacing) * 13.2f, (sz + spacing) * 3.0f));
            VanGui::PopItemWidth();
            VanGui::EndTabItem();
        }

        if (VanGui::BeginTabItem("Canvas"))
        {
            VANGUI_DEMO_MARKER("Examples/Custom rendering/Canvas");
            static VanVector<VanVec2> points;
            static VanVec2 scrolling(0.0f, 0.0f);
            static bool opt_enable_grid = true;
            static bool opt_enable_context_menu = true;
            static bool adding_line = false;

            VanGui::Checkbox("Enable grid", &opt_enable_grid);
            VanGui::Checkbox("Enable context menu", &opt_enable_context_menu);
            VanGui::Text("Mouse Left: drag to add lines,\nMouse Right: drag to scroll, click for context menu.");

            // Typically you would use a BeginChild()/EndChild() pair to benefit from a clipping region + own scrolling.
            // Here we demonstrate that this can be replaced by simple offsetting + custom drawing + PushClipRect/PopClipRect() calls.
            // To use a child window instead we could use, e.g:
            //      VanGui::PushStyleVar(VanGuiStyleVar_WindowPadding, VanVec2(0, 0));      // Disable padding
            //      VanGui::PushStyleColor(VanGuiCol_ChildBg, VAN_COL32(50, 50, 50, 255));  // Set a background color
            //      VanGui::BeginChild("canvas", VanVec2(0.0f, 0.0f), VanGuiChildFlags_Borders, VanGuiWindowFlags_NoMove);
            //      VanGui::PopStyleColor();
            //      VanGui::PopStyleVar();
            //      [...]
            //      VanGui::EndChild();

            // Using InvisibleButton() as a convenience 1) it will advance the layout cursor and 2) allows us to use IsItemHovered()/IsItemActive()
            VanVec2 canvas_p0 = VanGui::GetCursorScreenPos();      // VanDrawList API uses screen coordinates!
            VanVec2 canvas_sz = VanGui::GetContentRegionAvail();   // Resize canvas to what's available
            if (canvas_sz.x < 50.0f) canvas_sz.x = 50.0f;
            if (canvas_sz.y < 50.0f) canvas_sz.y = 50.0f;
            VanVec2 canvas_p1 = VanVec2(canvas_p0.x + canvas_sz.x, canvas_p0.y + canvas_sz.y);

            // Draw border and background color
            VanGuiIO& io = VanGui::GetIO();
            VanDrawList* draw_list = VanGui::GetWindowDrawList();
            draw_list->AddRectFilled(canvas_p0, canvas_p1, VAN_COL32(50, 50, 50, 255));
            draw_list->AddRect(canvas_p0, canvas_p1, VAN_COL32(255, 255, 255, 255));

            // This will catch our interactions
            VanGui::InvisibleButton("canvas", canvas_sz, VanGuiButtonFlags_MouseButtonLeft | VanGuiButtonFlags_MouseButtonRight);
            const bool is_hovered = VanGui::IsItemHovered(); // Hovered
            const bool is_active = VanGui::IsItemActive();   // Held
            const VanVec2 origin(canvas_p0.x + scrolling.x, canvas_p0.y + scrolling.y); // Lock scrolled origin
            const VanVec2 mouse_pos_in_canvas(io.MousePos.x - origin.x, io.MousePos.y - origin.y);

            // Add first and second point
            if (is_hovered && !adding_line && VanGui::IsMouseClicked(VanGuiMouseButton_Left))
            {
                points.push_back(mouse_pos_in_canvas);
                points.push_back(mouse_pos_in_canvas);
                adding_line = true;
            }
            if (adding_line)
            {
                points.back() = mouse_pos_in_canvas;
                if (!VanGui::IsMouseDown(VanGuiMouseButton_Left))
                    adding_line = false;
            }

            // Pan (we use a zero mouse threshold when there's no context menu)
            // You may decide to make that threshold dynamic based on whether the mouse is hovering something etc.
            const float mouse_threshold_for_pan = opt_enable_context_menu ? -1.0f : 0.0f;
            if (is_active && VanGui::IsMouseDragging(VanGuiMouseButton_Right, mouse_threshold_for_pan))
            {
                scrolling.x += io.MouseDelta.x;
                scrolling.y += io.MouseDelta.y;
            }

            // Context menu (under default mouse threshold)
            VanVec2 drag_delta = VanGui::GetMouseDragDelta(VanGuiMouseButton_Right);
            if (opt_enable_context_menu && drag_delta.x == 0.0f && drag_delta.y == 0.0f)
                VanGui::OpenPopupOnItemClick("context", VanGuiPopupFlags_MouseButtonRight);
            if (VanGui::BeginPopup("context"))
            {
                if (adding_line)
                    points.resize(points.size() - 2);
                adding_line = false;
                if (VanGui::MenuItem("Remove one", nullptr, false, points.Size > 0)) { points.resize(points.size() - 2); }
                if (VanGui::MenuItem("Remove all", nullptr, false, points.Size > 0)) { points.clear(); }
                VanGui::EndPopup();
            }

            // Draw grid + all lines in the canvas
            draw_list->PushClipRect(canvas_p0, canvas_p1, true);
            if (opt_enable_grid)
            {
                const float GRID_STEP = 64.0f;
                for (float x = fmodf(scrolling.x, GRID_STEP); x < canvas_sz.x; x += GRID_STEP)
                    draw_list->AddLineV(canvas_p0.x + x, canvas_p0.y, canvas_p1.y, VAN_COL32(200, 200, 200, 40));
                for (float y = fmodf(scrolling.y, GRID_STEP); y < canvas_sz.y; y += GRID_STEP)
                    draw_list->AddLineH(canvas_p0.x, canvas_p1.x, canvas_p0.y + y, VAN_COL32(200, 200, 200, 40));
            }
            for (int n = 0; n < points.Size; n += 2)
                draw_list->AddLine(VanVec2(origin.x + points[n].x, origin.y + points[n].y), VanVec2(origin.x + points[n + 1].x, origin.y + points[n + 1].y), VAN_COL32(255, 255, 0, 255), 2.0f);
            draw_list->PopClipRect();

            VanGui::EndTabItem();
        }

        if (VanGui::BeginTabItem("BG/FG draw lists"))
        {
            VANGUI_DEMO_MARKER("Examples/Custom rendering/BG & FG draw lists");
            static bool draw_bg = true;
            static bool draw_fg = true;
            VanGui::Checkbox("Draw in Background draw list", &draw_bg);
            VanGui::SameLine(); HelpMarker("The Background draw list will be rendered below every VanGUI windows.");
            VanGui::Checkbox("Draw in Foreground draw list", &draw_fg);
            VanGui::SameLine(); HelpMarker("The Foreground draw list will be rendered over every VanGUI windows.");
            VanVec2 window_pos = VanGui::GetWindowPos();
            VanVec2 window_size = VanGui::GetWindowSize();
            VanVec2 window_center = VanVec2(window_pos.x + window_size.x * 0.5f, window_pos.y + window_size.y * 0.5f);
            if (draw_bg)
                VanGui::GetBackgroundDrawList()->AddCircle(window_center, window_size.x * 0.6f, VAN_COL32(255, 0, 0, 200), 0, 10 + 4);
            if (draw_fg)
                VanGui::GetForegroundDrawList()->AddCircle(window_center, window_size.y * 0.6f, VAN_COL32(0, 255, 0, 200), 0, 10);
            VanGui::EndTabItem();
        }

        // Demonstrate out-of-order rendering via channels splitting
        // We use functions in VanDrawList as each draw list contains a convenience splitter,
        // but you can also instantiate your own VanDrawListSplitter if you need to nest them.
        if (VanGui::BeginTabItem("Draw Channels"))
        {
            VANGUI_DEMO_MARKER("Examples/Custom rendering/Draw Channels");
            VanDrawList* draw_list = VanGui::GetWindowDrawList();
            {
                VanGui::Text("Blue shape is drawn first: appears in back");
                VanGui::Text("Red shape is drawn after: appears in front");
                VanVec2 p0 = VanGui::GetCursorScreenPos();
                draw_list->AddRectFilled(VanVec2(p0.x, p0.y), VanVec2(p0.x + 50, p0.y + 50), VAN_COL32(0, 0, 255, 255)); // Blue
                draw_list->AddRectFilled(VanVec2(p0.x + 25, p0.y + 25), VanVec2(p0.x + 75, p0.y + 75), VAN_COL32(255, 0, 0, 255)); // Red
                VanGui::Dummy(VanVec2(75, 75));
            }
            VanGui::Separator();
            {
                VanGui::Text("Blue shape is drawn first, into channel 1: appears in front");
                VanGui::Text("Red shape is drawn after, into channel 0: appears in back");
                VanVec2 p1 = VanGui::GetCursorScreenPos();

                // Create 2 channels and draw a Blue shape THEN a Red shape.
                // You can create any number of channels. Tables API use 1 channel per column in order to better batch draw calls.
                draw_list->ChannelsSplit(2);
                draw_list->ChannelsSetCurrent(1);
                draw_list->AddRectFilled(VanVec2(p1.x, p1.y), VanVec2(p1.x + 50, p1.y + 50), VAN_COL32(0, 0, 255, 255)); // Blue
                draw_list->ChannelsSetCurrent(0);
                draw_list->AddRectFilled(VanVec2(p1.x + 25, p1.y + 25), VanVec2(p1.x + 75, p1.y + 75), VAN_COL32(255, 0, 0, 255)); // Red

                // Flatten/reorder channels. Red shape is in channel 0 and it appears below the Blue shape in channel 1.
                // This works by copying draw indices only (vertices are not copied).
                draw_list->ChannelsMerge();
                VanGui::Dummy(VanVec2(75, 75));
                VanGui::Text("After reordering, contents of channel 0 appears below channel 1.");
            }
            VanGui::EndTabItem();
        }

        VanGui::EndTabBar();
    }

    VanGui::End();
}

//-----------------------------------------------------------------------------
// [SECTION] Example App: Documents Handling / ShowExampleAppDocuments()
//-----------------------------------------------------------------------------

// Simplified structure to mimic a Document model
struct MyDocument
{
    char        Name[32];   // Document title
    int         UID;        // Unique ID (necessary as we can change title)
    bool        Open;       // Set when open (we keep an array of all available documents to simplify demo code!)
    bool        OpenPrev;   // Copy of Open from last update.
    bool        Dirty;      // Set when the document has been modified
    VanVec4      Color;      // An arbitrary variable associated to the document

    MyDocument(int uid, const char* name, bool open = true, const VanVec4& color = VanVec4(1.0f, 1.0f, 1.0f, 1.0f))
    {
        UID = uid;
        snprintf(Name, sizeof(Name), "%s", name);
        Open = OpenPrev = open;
        Dirty = false;
        Color = color;
    }
    void DoOpen()       { Open = true; }
    void DoForceClose() { Open = false; Dirty = false; }
    void DoSave()       { Dirty = false; }
};

struct ExampleAppDocuments
{
    VanVector<MyDocument>    Documents;
    VanVector<MyDocument*>   CloseQueue;
    MyDocument*             RenamingDoc = nullptr;
    bool                    RenamingStarted = false;

    ExampleAppDocuments()
    {
        Documents.push_back(MyDocument(0, "Lettuce",             true,  VanVec4(0.4f, 0.8f, 0.4f, 1.0f)));
        Documents.push_back(MyDocument(1, "Eggplant",            true,  VanVec4(0.8f, 0.5f, 1.0f, 1.0f)));
        Documents.push_back(MyDocument(2, "Carrot",              true,  VanVec4(1.0f, 0.8f, 0.5f, 1.0f)));
        Documents.push_back(MyDocument(3, "Tomato",              false, VanVec4(1.0f, 0.3f, 0.4f, 1.0f)));
        Documents.push_back(MyDocument(4, "A Rather Long Title", false, VanVec4(0.4f, 0.8f, 0.8f, 1.0f)));
        Documents.push_back(MyDocument(5, "Some Document",       false, VanVec4(0.8f, 0.8f, 1.0f, 1.0f)));
    }

    // As we allow to change document name, we append a never-changing document ID so tabs are stable
    void GetTabName(MyDocument* doc, char* out_buf, size_t out_buf_size)
    {
        snprintf(out_buf, out_buf_size, "%s###doc%d", doc->Name, doc->UID);
    }

    // Display placeholder contents for the Document
    void DisplayDocContents(MyDocument* doc)
    {
        VanGui::PushID(doc);
        VanGui::Text("Document \"%s\"", doc->Name);
        VanGui::PushStyleColor(VanGuiCol_Text, doc->Color);
        VanGui::TextWrapped("Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.");
        VanGui::PopStyleColor();

        VanGui::SetNextItemShortcut(VanGuiMod_Ctrl | VanGuiKey_R, VanGuiInputFlags_Tooltip);
        if (VanGui::Button("Rename.."))
        {
            RenamingDoc = doc;
            RenamingStarted = true;
        }
        VanGui::SameLine();

        VanGui::SetNextItemShortcut(VanGuiMod_Ctrl | VanGuiKey_M, VanGuiInputFlags_Tooltip);
        if (VanGui::Button("Modify"))
            doc->Dirty = true;

        VanGui::SameLine();
        VanGui::SetNextItemShortcut(VanGuiMod_Ctrl | VanGuiKey_S, VanGuiInputFlags_Tooltip);
        if (VanGui::Button("Save"))
            doc->DoSave();

        VanGui::SameLine();
        VanGui::SetNextItemShortcut(VanGuiMod_Ctrl | VanGuiKey_W, VanGuiInputFlags_Tooltip);
        if (VanGui::Button("Close"))
            CloseQueue.push_back(doc);
        VanGui::ColorEdit3("color", &doc->Color.x);  // Useful to test drag and drop and hold-dragged-to-open-tab behavior.
        VanGui::PopID();
    }

    // Display context menu for the Document
    void DisplayDocContextMenu(MyDocument* doc)
    {
        if (!VanGui::BeginPopupContextItem())
            return;

        char buf[256];
        snprintf(buf, sizeof(buf), "Save %s", doc->Name);
        if (VanGui::MenuItem(buf, "Ctrl+S", false, doc->Open))
            doc->DoSave();
        if (VanGui::MenuItem("Rename...", "Ctrl+R", false, doc->Open))
            RenamingDoc = doc;
        if (VanGui::MenuItem("Close", "Ctrl+W", false, doc->Open))
            CloseQueue.push_back(doc);
        VanGui::EndPopup();
    }

    // [Optional] Notify the system of Tabs/Windows closure that happened outside the regular tab interface.
    // If a tab has been closed programmatically (aka closed from another source such as the Checkbox() in the demo,
    // as opposed to clicking on the regular tab closing button) and stops being submitted, it will take a frame for
    // the tab bar to notice its absence. During this frame there will be a gap in the tab bar, and if the tab that has
    // disappeared was the selected one, the tab bar will report no selected tab during the frame. This will effectively
    // give the impression of a flicker for one frame.
    // We call SetTabItemClosed() to manually notify the Tab Bar or Docking system of removed tabs to avoid this glitch.
    // Note that this completely optional, and only affect tab bars with the VanGuiTabBarFlags_Reorderable flag.
    void NotifyOfDocumentsClosedElsewhere()
    {
        for (MyDocument& doc : Documents)
        {
            if (!doc.Open && doc.OpenPrev)
                VanGui::SetTabItemClosed(doc.Name);
            doc.OpenPrev = doc.Open;
        }
    }
};

void ShowExampleAppDocuments(bool* p_open)
{
    static ExampleAppDocuments app;

    // Options
    static bool opt_reorderable = true;
    static VanGuiTabBarFlags opt_fitting_flags = VanGuiTabBarFlags_FittingPolicyDefault_;

    bool window_contents_visible = VanGui::Begin("Example: Documents", p_open, VanGuiWindowFlags_MenuBar);
    if (!window_contents_visible)
    {
        VanGui::End();
        return;
    }
    VANGUI_DEMO_MARKER("Examples/Documents");

    // Menu
    if (VanGui::BeginMenuBar())
    {
        if (VanGui::BeginMenu("File"))
        {
            int open_count = 0;
            for (MyDocument& doc : app.Documents)
                open_count += doc.Open ? 1 : 0;

            if (VanGui::BeginMenu("Open", open_count < app.Documents.Size))
            {
                for (MyDocument& doc : app.Documents)
                    if (!doc.Open && VanGui::MenuItem(doc.Name))
                        doc.DoOpen();
                VanGui::EndMenu();
            }
            if (VanGui::MenuItem("Close All Documents", nullptr, false, open_count > 0))
                for (MyDocument& doc : app.Documents)
                    app.CloseQueue.push_back(&doc);
            if (VanGui::MenuItem("Exit") && p_open)
                *p_open = false;
            VanGui::EndMenu();
        }
        VanGui::EndMenuBar();
    }

    // [Debug] List documents with one checkbox for each
    for (int doc_n = 0; doc_n < app.Documents.Size; doc_n++)
    {
        MyDocument& doc = app.Documents[doc_n];
        if (doc_n > 0)
            VanGui::SameLine();
        VanGui::PushID(&doc);
        if (VanGui::Checkbox(doc.Name, &doc.Open))
            if (!doc.Open)
                doc.DoForceClose();
        VanGui::PopID();
    }

    VanGui::Separator();

    // About the VanGuiWindowFlags_UnsavedDocument / VanGuiTabItemFlags_UnsavedDocument flags.
    // They have multiple effects:
    // - Display a dot next to the title.
    // - Tab is selected when clicking the X close button.
    // - Closure is not assumed (will wait for user to stop submitting the tab).
    //   Otherwise closure is assumed when pressing the X, so if you keep submitting the tab may reappear at end of tab bar.
    //   We need to assume closure by default otherwise waiting for "lack of submission" on the next frame would leave an empty
    //   hole for one-frame, both in the tab-bar and in tab-contents when closing a tab/window.
    //   The rarely used SetTabItemClosed() function is a way to notify of programmatic closure to avoid the one-frame hole.

    // Submit Tab Bar and Tabs
    {
        VanGuiTabBarFlags tab_bar_flags = (opt_fitting_flags) | (opt_reorderable ? VanGuiTabBarFlags_Reorderable : 0);
        tab_bar_flags |= VanGuiTabBarFlags_DrawSelectedOverline;
        if (VanGui::BeginTabBar("##tabs", tab_bar_flags))
        {
            if (opt_reorderable)
                app.NotifyOfDocumentsClosedElsewhere();

            // [DEBUG] Stress tests
            //if ((VanGui::GetFrameCount() % 30) == 0) docs[1].Open ^= 1;            // [DEBUG] Automatically show/hide a tab. Test various interactions e.g. dragging with this on.
            //if (VanGui::GetIO().KeyCtrl) VanGui::SetTabItemSelected(docs[1].Name);  // [DEBUG] Test SetTabItemSelected(), probably not very useful as-is anyway..

            // Submit Tabs
            for (MyDocument& doc : app.Documents)
            {
                if (!doc.Open)
                    continue;

                // As we allow to change document name, we append a never-changing document id so tabs are stable
                char doc_name_buf[64];
                app.GetTabName(&doc, doc_name_buf, sizeof(doc_name_buf));
                VanGuiTabItemFlags tab_flags = (doc.Dirty ? VanGuiTabItemFlags_UnsavedDocument : 0);
                bool visible = VanGui::BeginTabItem(doc_name_buf, &doc.Open, tab_flags);

                // Cancel attempt to close when unsaved add to save queue so we can display a popup.
                if (!doc.Open && doc.Dirty)
                {
                    doc.Open = true;
                    app.CloseQueue.push_back(&doc);
                }

                app.DisplayDocContextMenu(&doc);
                if (visible)
                {
                    app.DisplayDocContents(&doc);
                    VanGui::EndTabItem();
                }
            }

            VanGui::EndTabBar();
        }
    }

    // Display renaming UI
    if (app.RenamingDoc != nullptr)
    {
        if (app.RenamingStarted)
            VanGui::OpenPopup("Rename");
        if (VanGui::BeginPopup("Rename"))
        {
            VanGui::SetNextItemWidth(VanGui::GetFontSize() * 30);
            if (VanGui::InputText("###Name", app.RenamingDoc->Name, VAN_COUNTOF(app.RenamingDoc->Name), VanGuiInputTextFlags_EnterReturnsTrue))
            {
                VanGui::CloseCurrentPopup();
                app.RenamingDoc = nullptr;
            }
            if (app.RenamingStarted)
                VanGui::SetKeyboardFocusHere(-1);
            VanGui::EndPopup();
        }
        else
        {
            app.RenamingDoc = nullptr;
        }
        app.RenamingStarted = false;
    }

    // Display closing confirmation UI
    if (!app.CloseQueue.empty())
    {
        int close_queue_unsaved_documents = 0;
        for (int n = 0; n < app.CloseQueue.Size; n++)
            if (app.CloseQueue[n]->Dirty)
                close_queue_unsaved_documents++;

        if (close_queue_unsaved_documents == 0)
        {
            // Close documents when all are unsaved
            for (int n = 0; n < app.CloseQueue.Size; n++)
                app.CloseQueue[n]->DoForceClose();
            app.CloseQueue.clear();
        }
        else
        {
            if (!VanGui::IsPopupOpen("Save?"))
                VanGui::OpenPopup("Save?");
            if (VanGui::BeginPopupModal("Save?", nullptr, VanGuiWindowFlags_AlwaysAutoResize))
            {
                VanGui::Text("Save change to the following items?");
                float item_height = VanGui::GetTextLineHeightWithSpacing();
                if (VanGui::BeginChild(VanGui::GetID("frame"), VanVec2(-FLT_MIN, 6.25f * item_height), VanGuiChildFlags_FrameStyle))
                    for (MyDocument* doc : app.CloseQueue)
                        if (doc->Dirty)
                            VanGui::Text("%s", doc->Name);
                VanGui::EndChild();

                VanVec2 button_size(VanGui::GetFontSize() * 7.0f, 0.0f);
                if (VanGui::Button("Yes", button_size))
                {
                    for (MyDocument* doc : app.CloseQueue)
                    {
                        if (doc->Dirty)
                            doc->DoSave();
                        doc->DoForceClose();
                    }
                    app.CloseQueue.clear();
                    VanGui::CloseCurrentPopup();
                }
                VanGui::SameLine();
                if (VanGui::Button("No", button_size))
                {
                    for (MyDocument* doc : app.CloseQueue)
                        doc->DoForceClose();
                    app.CloseQueue.clear();
                    VanGui::CloseCurrentPopup();
                }
                VanGui::SameLine();
                if (VanGui::Button("Cancel", button_size))
                {
                    app.CloseQueue.clear();
                    VanGui::CloseCurrentPopup();
                }
                VanGui::EndPopup();
            }
        }
    }

    VanGui::End();
}

//-----------------------------------------------------------------------------
// [SECTION] Example App: Assets Browser / ShowExampleAppAssetsBrowser()
//-----------------------------------------------------------------------------

//#include "vangui_internal.h" // NavMoveRequestTryWrapping()

struct ExampleAsset
{
    VanGuiID ID;
    int     Type;

    ExampleAsset(VanGuiID id, int type) { ID = id; Type = type; }

    static const VanGuiTableSortSpecs* s_current_sort_specs;

    static void SortWithSortSpecs(VanGuiTableSortSpecs* sort_specs, ExampleAsset* items, int items_count)
    {
        s_current_sort_specs = sort_specs; // Store in variable accessible by the sort function.
        if (items_count > 1)
            qsort(items, static_cast<size_t>(items_count), sizeof(items[0]), ExampleAsset::CompareWithSortSpecs);
        s_current_sort_specs = nullptr;
    }

    // Compare function to be used by qsort()
    static int VANGUI_CDECL CompareWithSortSpecs(const void* lhs, const void* rhs)
    {
        const ExampleAsset* a = static_cast<const ExampleAsset*>(lhs);
        const ExampleAsset* b = static_cast<const ExampleAsset*>(rhs);
        for (int n = 0; n < s_current_sort_specs->SpecsCount; n++)
        {
            const VanGuiTableColumnSortSpecs* sort_spec = &s_current_sort_specs->Specs[n];
            int delta = 0;
            if (sort_spec->ColumnIndex == 0)
                delta = (static_cast<int>(a->ID) - static_cast<int>(b->ID));
            else if (sort_spec->ColumnIndex == 1)
                delta = (a->Type - b->Type);
            if (delta > 0)
                return (sort_spec->SortDirection == VanGuiSortDirection_Ascending) ? +1 : -1;
            if (delta < 0)
                return (sort_spec->SortDirection == VanGuiSortDirection_Ascending) ? -1 : +1;
        }
        return static_cast<int>(a->ID) - static_cast<int>(b->ID);
    }
};
const VanGuiTableSortSpecs* ExampleAsset::s_current_sort_specs = nullptr;

struct ExampleAssetsBrowser
{
    // Options
    bool            ShowTypeOverlay = true;
    bool            AllowSorting = true;
    bool            AllowBoxSelect = true;                  // Will set VanGuiMultiSelectFlags_BoxSelect2d
    bool            AllowBoxSelectInsideSelection = false;  // Will set VanGuiMultiSelectFlags_SelectOnClickAlways
    bool            AllowDragUnselected = false;            // Will set VanGuiMultiSelectFlags_SelectOnClickRelease
    float           IconSize = 0;
    int             IconSpacing = 10;
    int             IconHitSpacing = 4;                     // Increase hit-spacing if you want to make it possible to clear or box-select from gaps. Some spacing is required to able to amend with Shift+box-select. Value is small in Explorer.
    bool            StretchSpacing = true;
    bool            UseScrollX = false;                     // Debug: submit twice the number of items per line (overflow horizontally to exercise ScrollX + box-select)

    // State
    VanVector<ExampleAsset> Items;               // Our items
    ExampleSelectionWithDeletion Selection;     // Our selection (VanGuiSelectionBasicStorage + helper funcs to handle deletion)
    VanGuiID         NextItemId = 0;             // Unique identifier when creating new items
    bool            RequestDelete = false;      // Deferred deletion request
    bool            RequestSort = false;        // Deferred sort request
    float           ZoomWheelAccum = 0.0f;      // Mouse wheel accumulator to handle smooth wheels better

    // Calculated sizes for layout, output of UpdateLayoutSizes(). Could be locals but our code is simpler this way.
    VanVec2          LayoutItemSize;
    VanVec2          LayoutItemStep;             // == LayoutItemSize + LayoutItemSpacing
    float           LayoutItemSpacing = 0.0f;
    float           LayoutSelectableSpacing = 0.0f;
    float           LayoutOuterPadding = 0.0f;
    int             LayoutColumnCount = 0;
    int             LayoutLineCount = 0;

    // Functions
    ExampleAssetsBrowser()
    {
        AddItems(10000);
    }
    void AddItems(int count)
    {
        if (Items.Size == 0)
            NextItemId = 0;
        Items.reserve(Items.Size + count);
        for (int n = 0; n < count; n++, NextItemId++)
            Items.push_back(ExampleAsset(NextItemId, (NextItemId % 20) < 15 ? 0 : (NextItemId % 20) < 18 ? 1 : 2));
        RequestSort = true;
    }
    void ClearItems()
    {
        Items.clear();
        Selection.Clear();
    }

    // Logic would be written in the main code BeginChild() and outputting to local variables.
    // We extracted it into a function so we can call it easily from multiple places.
    void UpdateLayoutSizes(float avail_width)
    {
        // Layout: when not stretching: allow extending into right-most spacing.
        LayoutItemSpacing = static_cast<float>(IconSpacing);
        if (StretchSpacing == false)
            avail_width += floorf(LayoutItemSpacing * 0.5f);

        // Layout: calculate number of icon per line and number of lines
        LayoutItemSize = VanVec2(floorf(IconSize), floorf(IconSize));
        LayoutColumnCount = VAN_MAX(static_cast<int>(avail_width / (LayoutItemSize.x + LayoutItemSpacing)), 1);

        // Layout: when stretching: allocate remaining space to more spacing. Round before division, so item_spacing may be non-integer.
        if (StretchSpacing && LayoutColumnCount > 1)
            LayoutItemSpacing = floorf(avail_width - LayoutItemSize.x * LayoutColumnCount) / LayoutColumnCount;

        if (UseScrollX)
            LayoutColumnCount *= 2;
        LayoutLineCount = (Items.Size + LayoutColumnCount - 1) / LayoutColumnCount;

        LayoutItemStep = VanVec2(LayoutItemSize.x + LayoutItemSpacing, LayoutItemSize.y + LayoutItemSpacing);
        LayoutSelectableSpacing = VAN_MAX(floorf(LayoutItemSpacing) - IconHitSpacing, 0.0f);
        LayoutOuterPadding = floorf(LayoutItemSpacing * 0.5f);
    }

    void Draw(const char* title, bool* p_open)
    {
        if (IconSize <= 0.0f)
            IconSize = VanGui::CalcTextSize("99999").x;

        VanGui::SetNextWindowSize(VanVec2(IconSize * 25, IconSize * 15), VanGuiCond_FirstUseEver);
        if (!VanGui::Begin(title, p_open, VanGuiWindowFlags_MenuBar))
        {
            VanGui::End();
            return;
        }
        VANGUI_DEMO_MARKER("Examples/Assets Browser");

        // Menu bar
        if (VanGui::BeginMenuBar())
        {
            if (VanGui::BeginMenu("File"))
            {
                if (VanGui::MenuItem("Add 10000 items"))
                    AddItems(10000);
                if (VanGui::MenuItem("Clear items"))
                    ClearItems();
                VanGui::Separator();
                if (VanGui::MenuItem("Close", nullptr, false, p_open != nullptr))
                    *p_open = false;
                VanGui::EndMenu();
            }
            if (VanGui::BeginMenu("Edit"))
            {
                if (VanGui::MenuItem("Delete", "Del", false, Selection.Size > 0))
                    RequestDelete = true;
                VanGui::EndMenu();
            }
            if (VanGui::BeginMenu("Options"))
            {
                VanGui::PushItemWidth(VanGui::GetFontSize() * 10);

                VanGui::SeparatorText("Contents");
                VanGui::Checkbox("Show Type Overlay", &ShowTypeOverlay);
                VanGui::Checkbox("Allow Sorting", &AllowSorting);

                VanGui::SeparatorText("Selection Behavior");
                VanGui::Checkbox("Allow box-selection", &AllowBoxSelect);
                if (VanGui::Checkbox("Allow box-selection from selected items", &AllowBoxSelectInsideSelection) && AllowBoxSelectInsideSelection)
                    AllowDragUnselected = false;
                if (VanGui::Checkbox("Allow dragging unselected item", &AllowDragUnselected) && AllowDragUnselected)
                    AllowBoxSelectInsideSelection = false;

                VanGui::SeparatorText("Layout");
                VanGui::SliderFloat("Icon Size", &IconSize, 16.0f, 128.0f, "%.0f");
                VanGui::SameLine(); HelpMarker("Use Ctrl+Wheel to zoom");
                VanGui::SliderInt("Icon Spacing", &IconSpacing, 0, 32);
                VanGui::SliderInt("Icon Hit Spacing", &IconHitSpacing, 0, 32);
                VanGui::Checkbox("Stretch Spacing", &StretchSpacing);
                VanGui::Checkbox("Use ScrollX", &UseScrollX);
                VanGui::PopItemWidth();
                VanGui::EndMenu();
            }
            VanGui::EndMenuBar();
        }

        // Show a table with ONLY one header row to showcase the idea/possibility of using this to provide a sorting UI
        if (AllowSorting)
        {
            VanGui::PushStyleVar(VanGuiStyleVar_ItemSpacing, VanVec2(0, 0));
            VanGuiTableFlags table_flags_for_sort_specs = VanGuiTableFlags_Sortable | VanGuiTableFlags_SortMulti | VanGuiTableFlags_SizingFixedFit | VanGuiTableFlags_Borders;
            if (VanGui::BeginTable("for_sort_specs_only", 2, table_flags_for_sort_specs, VanVec2(0.0f, VanGui::GetFrameHeight())))
            {
                VanGui::TableSetupColumn("Index");
                VanGui::TableSetupColumn("Type");
                VanGui::TableHeadersRow();
                if (VanGuiTableSortSpecs* sort_specs = VanGui::TableGetSortSpecs())
                    if (sort_specs->SpecsDirty || RequestSort)
                    {
                        ExampleAsset::SortWithSortSpecs(sort_specs, Items.Data, Items.Size);
                        sort_specs->SpecsDirty = RequestSort = false;
                    }
                VanGui::EndTable();
            }
            VanGui::PopStyleVar();
        }

        VanGuiIO& io = VanGui::GetIO();
        VanGui::SetNextWindowContentSize(VanVec2(0.0f, LayoutOuterPadding + LayoutLineCount * (LayoutItemSize.y + LayoutItemSpacing)));
        if (VanGui::BeginChild("Assets", VanVec2(0.0f, -VanGui::GetTextLineHeightWithSpacing()), VanGuiChildFlags_Borders, VanGuiWindowFlags_NoMove | VanGuiWindowFlags_HorizontalScrollbar))
        {
            VanDrawList* draw_list = VanGui::GetWindowDrawList();

            const float avail_width = VanGui::GetContentRegionAvail().x;
            UpdateLayoutSizes(avail_width);

            // Calculate and store start position.
            VanVec2 start_pos = VanGui::GetCursorScreenPos();
            start_pos = VanVec2(start_pos.x + LayoutOuterPadding, start_pos.y + LayoutOuterPadding);
            VanGui::SetCursorScreenPos(start_pos);

            // Multi-select
            VanGuiMultiSelectFlags ms_flags = VanGuiMultiSelectFlags_ClearOnEscape | VanGuiMultiSelectFlags_ClearOnClickVoid;

            // - Enable box-select (in 2D mode, so that changing box-select rectangle X1/X2 boundaries will affect clipped items)
            if (AllowBoxSelect)
                ms_flags |= VanGuiMultiSelectFlags_BoxSelect2d;

            // - Selection mode
            if (AllowDragUnselected)
                ms_flags |= VanGuiMultiSelectFlags_SelectOnClickRelease; // Rarely used: Allows dragging an unselected item without selecting it(rarely used)
            else if (AllowBoxSelectInsideSelection)
                ms_flags |= VanGuiMultiSelectFlags_SelectOnClickAlways; // Rarely used: Prevents Drag and Drop from being used on multiple-selection, but allows e.g. BoxSelect to always reselect even when clicking inside an existing selection.

            // - Enable keyboard wrapping on X axis
            // (FIXME-MULTISELECT: We haven't designed/exposed a general nav wrapping api yet, so this flag is provided as a courtesy to avoid doing:
            //    VanGui::NavMoveRequestTryWrapping(VanGui::GetCurrentWindow(), VanGuiNavMoveFlags_WrapX);
            // When we finish implementing a more general API for this, we will obsolete this flag in favor of the new system)
            ms_flags |= VanGuiMultiSelectFlags_NavWrapX;

            VanGuiMultiSelectIO* ms_io = VanGui::BeginMultiSelect(ms_flags, Selection.Size, Items.Size);

            // Use custom selection adapter: store ID in selection (recommended)
            Selection.UserData = this;
            Selection.AdapterIndexToStorageId = [](VanGuiSelectionBasicStorage* self_, int idx) { ExampleAssetsBrowser* self = (ExampleAssetsBrowser*)self_->UserData; return self->Items[idx].ID; };
            Selection.ApplyRequests(ms_io);

            const bool want_delete = (VanGui::Shortcut(VanGuiKey_Delete, VanGuiInputFlags_Repeat) && (Selection.Size > 0)) || RequestDelete;
            const int item_curr_idx_to_focus = want_delete ? Selection.ApplyDeletionPreLoop(ms_io, Items.Size) : -1;
            RequestDelete = false;

            // Push LayoutSelectableSpacing (which is LayoutItemSpacing minus hit-spacing, if we decide to have hit gaps between items)
            // Altering style ItemSpacing may seem unnecessary as we position every items using SetCursorScreenPos()...
            // But it is necessary for two reasons:
            // - Selectables uses it by default to visually fill the space between two items.
            // - The vertical spacing would be measured by Clipper to calculate line height if we didn't provide it explicitly (here we do).
            VanGui::PushStyleVar(VanGuiStyleVar_ItemSpacing, VanVec2(LayoutSelectableSpacing, LayoutSelectableSpacing));

            // Rendering parameters
            const VanU32 icon_type_overlay_colors[3] = { 0, VAN_COL32(200, 70, 70, 255), VAN_COL32(70, 170, 70, 255) };
            const VanU32 icon_bg_color = VanGui::GetColorU32(VAN_COL32(35, 35, 35, 220));
            const VanVec2 icon_type_overlay_size = VanVec2(4.0f, 4.0f);
            const bool display_label = (LayoutItemSize.x >= VanGui::CalcTextSize("999").x);

            const int column_count = LayoutColumnCount;
            VanGuiListClipper clipper;
            clipper.Begin(LayoutLineCount, LayoutItemStep.y);
            if (item_curr_idx_to_focus != -1)
                clipper.IncludeItemByIndex(item_curr_idx_to_focus / column_count); // Ensure focused item line is not clipped.
            if (ms_io->RangeSrcItem != -1)
                clipper.IncludeItemByIndex(static_cast<int>(ms_io->RangeSrcItem) / column_count); // Ensure RangeSrc item line is not clipped.
            while (clipper.Step())
            {
                for (int line_idx = clipper.DisplayStart; line_idx < clipper.DisplayEnd; line_idx++)
                {
                    const int item_min_idx_for_current_line = line_idx * column_count;
                    const int item_max_idx_for_current_line = VAN_MIN((line_idx + 1) * column_count, Items.Size);
                    for (int item_idx = item_min_idx_for_current_line; item_idx < item_max_idx_for_current_line; ++item_idx)
                    {
                        ExampleAsset* item_data = &Items[item_idx];
                        VanGui::PushID(static_cast<int>(item_data->ID));

                        // Position item
                        VanVec2 pos = VanVec2(start_pos.x + (item_idx % column_count) * LayoutItemStep.x, start_pos.y + line_idx * LayoutItemStep.y);
                        VanGui::SetCursorScreenPos(pos);

                        VanGui::SetNextItemSelectionUserData(item_idx);
                        bool item_is_selected = Selection.Contains((VanGuiID)item_data->ID);
                        bool item_is_visible = VanGui::IsRectVisible(LayoutItemSize);
                        VanGui::Selectable("", item_is_selected, VanGuiSelectableFlags_None, LayoutItemSize);

                        // Update our selection state immediately (without waiting for EndMultiSelect() requests)
                        // because we use this to alter the color of our text/icon.
                        if (VanGui::IsItemToggledSelection())
                            item_is_selected = !item_is_selected;

                        // Focus (for after deletion)
                        if (item_curr_idx_to_focus == item_idx)
                            VanGui::SetKeyboardFocusHere(-1);

                        // Drag and drop
                        if (VanGui::BeginDragDropSource())
                        {
                            // Create payload with full selection OR single unselected item.
                            // (the later is only possible when using VanGuiMultiSelectFlags_SelectOnClickRelease)
                            if (VanGui::GetDragDropPayload() == nullptr)
                            {
                                VanVector<VanGuiID> payload_items;
                                void* it = nullptr;
                                VanGuiID id = 0;
                                if (!item_is_selected)
                                    payload_items.push_back(item_data->ID);
                                else
                                    while (Selection.GetNextSelectedItem(&it, &id))
                                        payload_items.push_back(id);
                                VanGui::SetDragDropPayload("ASSETS_BROWSER_ITEMS", payload_items.Data, (size_t)payload_items.size_in_bytes());
                            }

                            // Display payload content in tooltip, by extracting it from the payload data
                            // (we could read from selection, but it is more correct and reusable to read from payload)
                            const VanGuiPayload* payload = VanGui::GetDragDropPayload();
                            const int payload_count = static_cast<int>(payload->DataSize) / static_cast<int>(sizeof(VanGuiID));
                            VanGui::Text("%d assets", payload_count);

                            VanGui::EndDragDropSource();
                        }

                        // Render icon (a real app would likely display an image/thumbnail here)
                        // Because we use VanGuiMultiSelectFlags_BoxSelect2d, clipping vertical may occasionally be larger, so we coarse-clip our rendering as well.
                        if (item_is_visible)
                        {
                            VanVec2 box_min(pos.x - 1, pos.y - 1);
                            VanVec2 box_max(box_min.x + LayoutItemSize.x + 2, box_min.y + LayoutItemSize.y + 2); // Dubious
                            draw_list->AddRectFilled(box_min, box_max, icon_bg_color); // Background color
                            if (ShowTypeOverlay && item_data->Type != 0)
                            {
                                VanU32 type_col = icon_type_overlay_colors[item_data->Type % VAN_COUNTOF(icon_type_overlay_colors)];
                                draw_list->AddRectFilled(VanVec2(box_max.x - 2 - icon_type_overlay_size.x, box_min.y + 2), VanVec2(box_max.x - 2, box_min.y + 2 + icon_type_overlay_size.y), type_col);
                            }
                            if (display_label)
                            {
                                VanU32 label_col = VanGui::GetColorU32(item_is_selected ? VanGuiCol_Text : VanGuiCol_TextDisabled);
                                char label[32];
                                snprintf(label, sizeof(label), "%d", item_data->ID);
                                draw_list->AddText(VanVec2(box_min.x, box_max.y - VanGui::GetFontSize()), label_col, label);
                            }
                        }

                        VanGui::PopID();
                    }
                }
            }
            clipper.End();
            if (Items.Size == 0)
                VanGui::Dummy(VanVec2(0, 0));
            VanGui::PopStyleVar(); // VanGuiStyleVar_ItemSpacing

            // Context menu
            if (VanGui::BeginPopupContextWindow())
            {
                VanGui::Text("Selection: %d items", Selection.Size);
                VanGui::Separator();
                if (VanGui::MenuItem("Delete", "Del", false, Selection.Size > 0))
                    RequestDelete = true;
                VanGui::EndPopup();
            }

            ms_io = VanGui::EndMultiSelect();
            Selection.ApplyRequests(ms_io);
            if (want_delete)
                Selection.ApplyDeletionPostLoop(ms_io, Items, item_curr_idx_to_focus);

            // Zooming with Ctrl+Wheel
            if (VanGui::IsWindowAppearing())
                ZoomWheelAccum = 0.0f;
            if (VanGui::IsWindowHovered() && io.MouseWheel != 0.0f && VanGui::IsKeyDown(VanGuiMod_Ctrl) && VanGui::IsAnyItemActive() == false)
            {
                ZoomWheelAccum += io.MouseWheel;
                if (fabsf(ZoomWheelAccum) >= 1.0f)
                {
                    // Calculate hovered item index from mouse location
                    // FIXME: Locking aiming on 'hovered_item_idx' (with a cool-down timer) would ensure zoom keeps on it.
                    const float hovered_item_nx = (io.MousePos.x - start_pos.x + LayoutItemSpacing * 0.5f) / LayoutItemStep.x;
                    const float hovered_item_ny = (io.MousePos.y - start_pos.y + LayoutItemSpacing * 0.5f) / LayoutItemStep.y;
                    const int hovered_item_idx = (static_cast<int>(hovered_item_ny) * LayoutColumnCount) + static_cast<int>(hovered_item_nx);
                    //VanGui::SetTooltip("%f,%f -> item %d", hovered_item_nx, hovered_item_ny, hovered_item_idx); // Move those 4 lines in block above for easy debugging

                    // Zoom
                    IconSize *= powf(1.1f, static_cast<float>(static_cast<int>(ZoomWheelAccum)));
                    IconSize = VAN_CLAMP(IconSize, 16.0f, 128.0f);
                    ZoomWheelAccum -= static_cast<int>(ZoomWheelAccum);
                    UpdateLayoutSizes(avail_width);

                    // Manipulate scroll to that we will land at the same Y location of currently hovered item.
                    // - Calculate next frame position of item under mouse
                    // - Set new scroll position to be used in next VanGui::BeginChild() call.
                    float hovered_item_rel_pos_y = (static_cast<float>(hovered_item_idx / LayoutColumnCount) + fmodf(hovered_item_ny, 1.0f)) * LayoutItemStep.y;
                    hovered_item_rel_pos_y += VanGui::GetStyle().WindowPadding.y;
                    float mouse_local_y = io.MousePos.y - VanGui::GetWindowPos().y;
                    VanGui::SetScrollY(hovered_item_rel_pos_y - mouse_local_y);
                }
            }
        }
        VanGui::EndChild();

        VanGui::Text("Selected: %d/%d items", Selection.Size, Items.Size);
        VanGui::End();
    }
};

void ShowExampleAppAssetsBrowser(bool* p_open)
{
    VANGUI_DEMO_MARKER("Examples/Assets Browser");
    static ExampleAssetsBrowser assets_browser;
    assets_browser.Draw("Example: Assets Browser", p_open);
}

// End of Demo code
#else

void VanGui::ShowAboutWindow(bool*) {}
void VanGui::ShowDemoWindow(bool*) {}
void VanGui::ShowUserGuide() {}
void VanGui::ShowStyleEditor(VanGuiStyle*) {}
bool VanGui::ShowStyleSelector(const char*) { return false; }

#endif // #ifndef VANGUI_DISABLE_DEMO_WINDOWS

#endif // #ifndef VANGUI_DISABLE
