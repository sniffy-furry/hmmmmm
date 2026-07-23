// dear vangui, v1.92.9 WIP
// (internal structures/api)

// You may use this file to debug, understand or extend VanGUI features but we don't provide any guarantee of forward compatibility.

/*

Index of this file:

// [SECTION] Header mess
// [SECTION] Forward declarations
// [SECTION] Context pointer
// [SECTION] STB libraries includes
// [SECTION] Macros
// [SECTION] Generic helpers
// [SECTION] VanDrawList support
// [SECTION] Style support
// [SECTION] Data types support
// [SECTION] Widgets support: flags, enums, data structures
// [SECTION] Popup support
// [SECTION] Inputs support
// [SECTION] Clipper support
// [SECTION] Navigation support
// [SECTION] Typing-select support
// [SECTION] Columns support
// [SECTION] Box-select support
// [SECTION] Multi-select support
// [SECTION] Docking support
// [SECTION] Viewport support
// [SECTION] Settings support
// [SECTION] Localization support
// [SECTION] Error handling, State recovery support
// [SECTION] Metrics, Debug tools
// [SECTION] Generic context hooks
// [SECTION] VanGuiContext (main vangui context)
// [SECTION] VanGuiWindowTempData, VanGuiWindow
// [SECTION] Tab bar, Tab item support
// [SECTION] Table support
// [SECTION] VanGui internal API
// [SECTION] VanFontLoader
// [SECTION] VanFontAtlas internal API
// [SECTION] Test Engine specific hooks (vangui_test_engine)

*/

#pragma once
#ifndef VANGUI_DISABLE

//-----------------------------------------------------------------------------
// [SECTION] Header mess
//-----------------------------------------------------------------------------

#ifndef VANGUI_VERSION
#include "vangui.h"
#endif

#include <algorithm>   // std::min, std::max, std::clamp, std::swap
#include <bit>         // std::has_single_bit, std::popcount
#include <climits>     // INT_MIN, INT_MAX
#include <cmath>       // std::sqrt, std::fabs, std::fmod, std::pow, std::floor, std::ceil, std::cos, std::sin
#include <cstdio>      // FILE*, sscanf
#include <cstdlib>     // malloc, free, qsort, atoi, atof
#include <numbers>     // std::numbers::pi_v

// Enable SSE intrinsics if available
#if (defined __SSE__ || defined __x86_64__ || defined _M_X64 || (defined(_M_IX86_FP) && (_M_IX86_FP >= 1))) && !defined(VANGUI_DISABLE_SSE) && !defined(_M_ARM64) && !defined(_M_ARM64EC)
#define VANGUI_ENABLE_SSE
#include <immintrin.h>
#if (defined __AVX__ || defined __SSE4_2__)
#define VANGUI_ENABLE_SSE4_2
#include <nmmintrin.h>
#endif
#endif
// Emscripten has partial SSE 4.2 support where _mm_crc32_u32 is not available. See https://emscripten.org/docs/porting/simd.html#id11 and #8213
#if defined(VANGUI_ENABLE_SSE4_2) && !defined(VANGUI_USE_LEGACY_CRC32_ADLER) && !defined(__EMSCRIPTEN__)
#define VANGUI_ENABLE_SSE4_2_CRC
#endif

// Visual Studio warnings
#ifdef _MSC_VER
#pragma warning (push)
#pragma warning (disable: 4251)     // class 'xxx' needs to have dll-interface to be used by clients of struct 'xxx' // when VANGUI_API is set to__declspec(dllexport)
#pragma warning (disable: 26495)    // [Static Analyzer] Variable 'XXX' is uninitialized. Always initialize a member variable (type.6).
#pragma warning (disable: 26812)    // [Static Analyzer] The enum type 'xxx' is unscoped. Prefer 'enum class' over 'enum' (Enum.3).
#if defined(_MSC_VER) && _MSC_VER >= 1922 // MSVC 2019 16.2 or later
#pragma warning (disable: 5054)     // operator '|': deprecated between enumerations of different types
#endif
#endif

// Clang/GCC warnings with -Weverything
#if defined(__clang__)
#pragma clang diagnostic push
#if __has_warning("-Wunknown-warning-option")
#pragma clang diagnostic ignored "-Wunknown-warning-option"         // warning: unknown warning group 'xxx'
#endif
#pragma clang diagnostic ignored "-Wunknown-pragmas"                // warning: unknown warning group 'xxx'
#pragma clang diagnostic ignored "-Wfloat-equal"                    // warning: comparing floating point with == or != is unsafe // storing and comparing against same constants ok, for VanFloor()
#pragma clang diagnostic ignored "-Wold-style-cast"                 // warning: use of old-style cast
#pragma clang diagnostic ignored "-Wzero-as-null-pointer-constant"  // warning: zero as null pointer constant
#pragma clang diagnostic ignored "-Wdouble-promotion"               // warning: implicit conversion from 'float' to 'double' when passing argument to function
#pragma clang diagnostic ignored "-Wimplicit-int-float-conversion"  // warning: implicit conversion from 'xxx' to 'float' may lose precision
#pragma clang diagnostic ignored "-Wmissing-noreturn"               // warning: function 'xxx' could be declared with attribute 'noreturn'
#pragma clang diagnostic ignored "-Wdeprecated-enum-enum-conversion"// warning: bitwise operation between different enumeration types ('XXXFlags_' and 'XXXFlagsPrivate_') is deprecated
#pragma clang diagnostic ignored "-Wunsafe-buffer-usage"            // warning: 'xxx' is an unsafe pointer used for buffer access
#pragma clang diagnostic ignored "-Wnontrivial-memaccess"           // warning: first argument in call to 'memset' is a pointer to non-trivially copyable type
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpragmas"                          // warning: unknown option after '#pragma GCC diagnostic' kind
#pragma GCC diagnostic ignored "-Wfloat-equal"                      // warning: comparing floating-point with '==' or '!=' is unsafe
#pragma GCC diagnostic ignored "-Wclass-memaccess"                  // [__GNUC__ >= 8] warning: 'memset/memcpy' clearing/writing an object of type 'xxxx' with no trivial copy-assignment; use assignment or value-initialization instead
#pragma GCC diagnostic ignored "-Wdeprecated-enum-enum-conversion"  // warning: bitwise operation between different enumeration types ('XXXFlags_' and 'XXXFlagsPrivate_') is deprecated
#pragma GCC diagnostic ignored "-Wsign-conversion"                  // warning: conversion to 'xxxx' from 'xxxx' may change the sign of the result
#endif

// In 1.89.4, we moved the implementation of "courtesy maths operators" from vangui_internal.h in vangui.h
// As they are frequently requested, we do not want to encourage to many people using vangui_internal.h
#if defined(VANGUI_DEFINE_MATH_OPERATORS) && !defined(VANGUI_DEFINE_MATH_OPERATORS_IMPLEMENTED)
#error Please '#define VANGUI_DEFINE_MATH_OPERATORS' _BEFORE_ including vangui.h!
#endif

// Legacy defines
#ifdef VANGUI_DISABLE_FORMAT_STRING_FUNCTIONS            // Renamed in 1.74
#error Use VANGUI_DISABLE_DEFAULT_FORMAT_FUNCTIONS
#endif
#ifdef VANGUI_DISABLE_MATH_FUNCTIONS                     // Renamed in 1.74
#error Use VANGUI_DISABLE_DEFAULT_MATH_FUNCTIONS
#endif

// Enable stb_truetype by default unless FreeType is enabled.
// You can compile with both by defining both VANGUI_ENABLE_FREETYPE and VANGUI_ENABLE_STB_TRUETYPE together.
#ifndef VANGUI_ENABLE_FREETYPE
#define VANGUI_ENABLE_STB_TRUETYPE
#endif

//-----------------------------------------------------------------------------
// [SECTION] Forward declarations
//-----------------------------------------------------------------------------

// Utilities
// (other types which are not forwarded declared are: VanBitArray<>, VanSpan<>, VanSpanAllocator<>, VanStableVector<>, VanPool<>, VanChunkStream<>)
struct VanBitVector;                 // Store 1-bit per value
struct VanRect;                      // An axis-aligned rectangle (2 points)
struct VanGuiTextIndex;              // Maintain a line index for a text buffer.

// VanDrawList/VanFontAtlas
struct VanDrawDataBuilder;           // Helper to build a VanDrawData instance
struct VanDrawListSharedData;        // Data shared between all VanDrawList instances
struct VanFontAtlasBuilder;          // Internal storage for incrementally packing and building a VanFontAtlas
struct VanFontAtlasPostProcessData;  // Data available to potential texture post-processing functions
struct VanFontAtlasRectEntry;        // Packed rectangle lookup entry

// VanGui
struct VanGuiBoxSelectState;         // Box-selection state (currently used by multi-selection, could potentially be used by others)
struct VanGuiColorMod;               // Stacked color modifier, backup of modified data so we can restore it
struct VanGuiContext;                // Main VanGUI context
struct VanGuiDockContext;            // Docking system context
struct VanGuiDockNode;               // Docking system node (hold a list of Windows OR two child dock nodes)
struct VanGuiDockRequest;            // Docking system pending request (target a specific node/window)
struct VanGuiDockNodeSettings;       // Storage for a dock node in .ini file (we preserve those even if the associated dock node isn't active during the session)
struct VanGuiPlatformMonitor;        // User-facing structure for a connected monitor/display
struct VanGuiContextHook;            // Hook for extensions like VanGuiTestEngine
struct VanGuiDataTypeInfo;           // Type information associated to a VanGuiDataType enum
struct VanGuiDeactivatedItemData;    // Data for IsItemDeactivated()/IsItemDeactivatedAfterEdit() function.
struct VanGuiErrorRecoveryState;     // Storage of stack sizes for error handling and recovery
struct VanGuiGroupData;              // Stacked storage data for BeginGroup()/EndGroup()
struct VanGuiInputTextState;         // Internal state of the currently focused/edited text input box
struct VanGuiInputTextDeactivateData;// Short term storage to backup text of a deactivating InputText() while another is stealing active id
struct VanGuiLastItemData;           // Status storage for last submitted items
struct VanGuiLocEntry;               // A localization entry.
struct VanGuiMenuColumns;            // Simple column measurement, currently used for MenuItem() only
struct VanGuiMultiSelectState;       // Multi-selection persistent state (for focused selection).
struct VanGuiMultiSelectTempData;    // Multi-selection temporary state (while traversing).
struct VanGuiNavItemData;            // Result of a keyboard/gamepad directional navigation move query result
struct VanGuiMetricsConfig;          // Storage for ShowMetricsWindow() and DebugNodeXXX() functions
struct VanGuiNextWindowData;         // Storage for SetNextWindow** functions
struct VanGuiNextItemData;           // Storage for SetNextItem** functions
struct VanGuiOldColumnData;          // Storage data for a single column for legacy Columns() api
struct VanGuiOldColumns;             // Storage data for a columns set for legacy Columns() api
struct VanGuiPopupData;              // Storage for current popup stack
struct VanGuiSettingsHandler;        // Storage for one type registered in the .ini file
struct VanGuiStyleMod;               // Stacked style modifier, backup of modified data so we can restore it
struct VanGuiStyleVarInfo;           // Style variable information (e.g. to access style variables from an enum)
struct VanGuiTabBar;                 // Storage for a tab bar
struct VanGuiTabItem;                // Storage for a tab item (within a tab bar)
struct VanGuiTable;                  // Storage for a table
struct VanGuiTableHeaderData;        // Storage for TableAngledHeadersRow()
struct VanGuiTableColumn;            // Storage for one column of a table
struct VanGuiTableInstanceData;      // Storage for one instance of a same table
struct VanGuiTableTempData;          // Temporary storage for one table (one per table in the stack), shared between tables.
struct VanGuiTableSettings;          // Storage for a table .ini settings
struct VanGuiTableColumnsSettings;   // Storage for a column .ini settings
struct VanGuiTreeNodeStackData;      // Temporary storage for TreeNode().
struct VanGuiTypingSelectState;      // Storage for GetTypingSelectRequest()
struct VanGuiTypingSelectRequest;    // Storage for GetTypingSelectRequest() (aimed to be public)
struct VanGuiWindow;                 // Storage for one window
struct VanGuiWindowTempData;         // Temporary storage for one window (that's the data which in theory we could ditch at the end of the frame, in practice we currently keep it for each window)
struct VanGuiWindowSettings;         // Storage for a window .ini settings (we keep one of those even if the actual window wasn't instanced during this session)

// Enumerations
// Use your programming IDE "Go to definition" facility on the names of the center columns to find the actual flags/enum lists.
enum VanGuiLocKey : int;                 // -> enum VanGuiLocKey              // Enum: a localization entry for translation.
using VanGuiLayoutType = int;            // -> enum VanGuiLayoutType_         // Enum: Horizontal or vertical

// Flags
using VanDrawTextFlags          = int;   // -> enum VanDrawTextFlags_         // Flags: for VanTextCalcWordWrapPositionEx()
using VanGuiActivateFlags       = int;   // -> enum VanGuiActivateFlags_      // Flags: for navigation/focus function (will be for ActivateItem() later)
using VanGuiDebugLogFlags       = int;   // -> enum VanGuiDebugLogFlags_      // Flags: for ShowDebugLogWindow(), g.DebugLogFlags
using VanGuiFocusRequestFlags   = int;   // -> enum VanGuiFocusRequestFlags_  // Flags: for FocusWindow()
using VanGuiItemStatusFlags     = int;   // -> enum VanGuiItemStatusFlags_    // Flags: for g.LastItemData.StatusFlags
using VanGuiOldColumnFlags      = int;   // -> enum VanGuiOldColumnFlags_     // Flags: for BeginColumns()
using VanGuiLogFlags            = int;   // -> enum VanGuiLogFlags_           // Flags: for LogBegin() text capturing function
using VanGuiNavRenderCursorFlags = int;  // -> enum VanGuiNavRenderCursorFlags_// Flags: for RenderNavCursor()
using VanGuiNavMoveFlags        = int;   // -> enum VanGuiNavMoveFlags_       // Flags: for navigation requests
using VanGuiNextItemDataFlags   = int;   // -> enum VanGuiNextItemDataFlags_  // Flags: for SetNextItemXXX() functions
using VanGuiNextWindowDataFlags = int;   // -> enum VanGuiNextWindowDataFlags_// Flags: for SetNextWindowXXX() functions
using VanGuiScrollFlags         = int;   // -> enum VanGuiScrollFlags_        // Flags: for ScrollToItem() and navigation requests
using VanGuiSeparatorFlags      = int;   // -> enum VanGuiSeparatorFlags_     // Flags: for SeparatorEx()
using VanGuiTextFlags           = int;   // -> enum VanGuiTextFlags_          // Flags: for TextEx()
using VanGuiTooltipFlags        = int;   // -> enum VanGuiTooltipFlags_       // Flags: for BeginTooltipEx()
using VanGuiTypingSelectFlags   = int;   // -> enum VanGuiTypingSelectFlags_  // Flags: for GetTypingSelectRequest()
using VanGuiWindowBgClickFlags  = int;   // -> enum VanGuiWindowBgClickFlags_ // Flags: for overriding behavior of clicking on window background/void.
using VanGuiWindowRefreshFlags  = int;   // -> enum VanGuiWindowRefreshFlags_ // Flags: for SetNextWindowRefreshPolicy()

// Table column indexing
using VanGuiTableColumnIdx      = VanS16;
using VanGuiTableDrawChannelIdx = VanU16;

//-----------------------------------------------------------------------------
// [SECTION] Context pointer
// See implementation of this variable in vangui.cpp for comments and details.
//-----------------------------------------------------------------------------

#ifndef GVanGui
extern VANGUI_API VanGuiContext* GVanGui;  // Current implicit context pointer
#endif

//-----------------------------------------------------------------------------
// [SECTION] Macros
//-----------------------------------------------------------------------------

// Debug Printing Into TTY
// (since VANGUI_VERSION_NUM >= 18729: VANGUI_DEBUG_LOG was reworked into VANGUI_DEBUG_PRINTF (and removed framecount from it). If you were using a #define VANGUI_DEBUG_LOG please rename)
#ifndef VANGUI_DEBUG_PRINTF
#ifndef VANGUI_DISABLE_DEFAULT_FORMAT_FUNCTIONS
#define VANGUI_DEBUG_PRINTF(_FMT,...)    printf(_FMT, __VA_ARGS__)
#else
#define VANGUI_DEBUG_PRINTF(_FMT,...)    ((void)0)
#endif
#endif

// Debug Logging for ShowDebugLogWindow(). This is designed for relatively rare events so please don't spam.
#define VANGUI_DEBUG_LOG_ERROR(...)      do { if (g.DebugLogFlags & VanGuiDebugLogFlags_EventError)       VANGUI_DEBUG_LOG(__VA_ARGS__); else g.DebugLogSkippedErrors++; } while (0)
#define VANGUI_DEBUG_LOG_ACTIVEID(...)   do { if (g.DebugLogFlags & VanGuiDebugLogFlags_EventActiveId)    VANGUI_DEBUG_LOG(__VA_ARGS__); } while (0)
#define VANGUI_DEBUG_LOG_FOCUS(...)      do { if (g.DebugLogFlags & VanGuiDebugLogFlags_EventFocus)       VANGUI_DEBUG_LOG(__VA_ARGS__); } while (0)
#define VANGUI_DEBUG_LOG_POPUP(...)      do { if (g.DebugLogFlags & VanGuiDebugLogFlags_EventPopup)       VANGUI_DEBUG_LOG(__VA_ARGS__); } while (0)
#define VANGUI_DEBUG_LOG_NAV(...)        do { if (g.DebugLogFlags & VanGuiDebugLogFlags_EventNav)         VANGUI_DEBUG_LOG(__VA_ARGS__); } while (0)
#define VANGUI_DEBUG_LOG_SELECTION(...)  do { if (g.DebugLogFlags & VanGuiDebugLogFlags_EventSelection)   VANGUI_DEBUG_LOG(__VA_ARGS__); } while (0)
#define VANGUI_DEBUG_LOG_CLIPPER(...)    do { if (g.DebugLogFlags & VanGuiDebugLogFlags_EventClipper)     VANGUI_DEBUG_LOG(__VA_ARGS__); } while (0)
#define VANGUI_DEBUG_LOG_IO(...)         do { if (g.DebugLogFlags & VanGuiDebugLogFlags_EventIO)          VANGUI_DEBUG_LOG(__VA_ARGS__); } while (0)
#define VANGUI_DEBUG_LOG_FONT(...)       do { VanGuiContext* g2 = GVanGui; if (g2 && g2->DebugLogFlags & VanGuiDebugLogFlags_EventFont) VANGUI_DEBUG_LOG(__VA_ARGS__); } while (0) // Called from VanFontAtlas function which may operate without a context.
#define VANGUI_DEBUG_LOG_INPUTROUTING(...) do{if (g.DebugLogFlags & VanGuiDebugLogFlags_EventInputRouting)VANGUI_DEBUG_LOG(__VA_ARGS__); } while (0)

// Debug options (also see ones on top of vangui.cpp)
//#define VANGUI_DEBUG_BOXSELECT

// Static Asserts
#define VAN_STATIC_ASSERT(_COND)         static_assert(_COND, "")

// "Paranoid" Debug Asserts are meant to only be enabled during specific debugging/work, otherwise would slow down the code too much.
// We currently don't have many of those so the effect is currently negligible, but onward intent to add more aggressive ones in the code.
//#define VANGUI_DEBUG_PARANOID
#ifdef VANGUI_DEBUG_PARANOID
#define VAN_ASSERT_PARANOID(_EXPR)       VAN_ASSERT(_EXPR)
#else
#define VAN_ASSERT_PARANOID(_EXPR)
#endif

// Misc Macros
inline constexpr float VAN_PI = std::numbers::pi_v<float>;
#ifdef _WIN32
#define VAN_NEWLINE                      "\r\n"   // Play it nice with Windows users (Update: since 2018-05, Notepad finally appears to support Unix-style carriage returns!)
#else
#define VAN_NEWLINE                      "\n"
#endif
#ifndef VAN_TABSIZE                      // Until we move this to runtime and/or add proper tab support, at least allow users to compile-time override
#define VAN_TABSIZE                      (4)
#endif
#define VAN_MEMALIGN(_OFF,_ALIGN)        (((_OFF) + ((_ALIGN) - 1)) & ~((_ALIGN) - 1))           // Memory align e.g. VAN_ALIGN(0,4)=0, VAN_ALIGN(1,4)=4, VAN_ALIGN(4,4)=4, VAN_ALIGN(5,4)=8
#define VAN_F32_TO_INT8_UNBOUND(_VAL)    ((int)((_VAL) * 255.0f + ((_VAL)>=0 ? 0.5f : -0.5f)))   // Unsaturated, for display purpose
#define VAN_F32_TO_INT8_SAT(_VAL)        ((int)(VanSaturate(_VAL) * 255.0f + 0.5f))               // Saturated, always output 0..255
#define VAN_TRUNC(_VAL)                  (static_cast<float>(static_cast<int>(_VAL)))             // Positive values only! VanTrunc() is not inlined in MSVC debug builds
#define VAN_ROUND(_VAL)                  (static_cast<float>(static_cast<int>((_VAL) + 0.5f)))   // Positive values only!
//#define VAN_FLOOR VAN_TRUNC             // [OBSOLETE] Renamed in 1.90.0 (Sept 2023)

// Hint for branch prediction (C++23, always available)
#define VAN_LIKELY   [[likely]]
#define VAN_UNLIKELY [[unlikely]]

// Enforce cdecl calling convention for functions called by the standard library, in case compilation settings changed the default to e.g. __vectorcall
#ifdef _MSC_VER
#define VANGUI_CDECL __cdecl
#else
#define VANGUI_CDECL
#endif

// Warnings
#if defined(_MSC_VER) && !defined(__clang__)
#define VAN_MSVC_WARNING_SUPPRESS(XXXX)  __pragma(warning(suppress: XXXX))
#else
#define VAN_MSVC_WARNING_SUPPRESS(XXXX)
#endif

// Debug Tools
// Use 'Metrics/Debugger->Tools->Item Picker' to break into the call-stack of a specific item.
// This will call VAN_DEBUG_BREAK() which you may redefine yourself. See https://github.com/scottt/debugbreak for more reference.
#ifndef VAN_DEBUG_BREAK
#if defined (_MSC_VER)
#define VAN_DEBUG_BREAK()    __debugbreak()
#elif defined(__clang__)
#define VAN_DEBUG_BREAK()    __builtin_debugtrap()
#elif defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
#define VAN_DEBUG_BREAK()    __asm__ volatile("int3;nop")
#elif defined(__GNUC__) && defined(__thumb__)
#define VAN_DEBUG_BREAK()    __asm__ volatile(".inst 0xde01")
#elif defined(__GNUC__) && defined(__arm__) && !defined(__thumb__)
#define VAN_DEBUG_BREAK()    __asm__ volatile(".inst 0xe7f001f0")
#else
#define VAN_DEBUG_BREAK()    VAN_ASSERT(0)    // It is expected that you define VAN_DEBUG_BREAK() into something that will break nicely in a debugger!
#endif
#endif // #ifndef VAN_DEBUG_BREAK

// Format specifiers, printing 64-bit hasn't been decently standardized...
// In a real application you should be using PRId64 and PRIu64 from <inttypes.h> (non-windows) and on Windows define them yourself.
#if defined(_MSC_VER) && !defined(__clang__)
#define VAN_PRId64   "I64d"
#define VAN_PRIu64   "I64u"
#define VAN_PRIX64   "I64X"
#else
#define VAN_PRId64   "lld"
#define VAN_PRIu64   "llu"
#define VAN_PRIX64   "llX"
#endif

//-----------------------------------------------------------------------------
// [SECTION] Generic helpers
// Note that the VanXXX helpers functions are lower-level than VanGui functions.
// VanGui functions or the VanGui context are never called/used from other VanXXX functions.
//-----------------------------------------------------------------------------
// - Helpers: Hashing
// - Helpers: Sorting
// - Helpers: Bit manipulation
// - Helpers: String
// - Helpers: Formatting
// - Helpers: UTF-8 <> wchar conversions
// - Helpers: VanVec2/VanVec4 operators
// - Helpers: Maths
// - Helpers: Geometry
// - Helper: VanVec1
// - Helper: VanVec2ih
// - Helper: VanRect
// - Helper: VanBitArray
// - Helper: VanBitVector
// - Helper: VanSpan<>, VanSpanAllocator<>
// - Helper: VanStableVector<>
// - Helper: VanPool<>
// - Helper: VanChunkStream<>
// - Helper: VanGuiTextIndex
// - Helper: VanGuiStorage
//-----------------------------------------------------------------------------

// Helpers: Hashing
VANGUI_API VanGuiID       VanHashData(const void* data, size_t data_size, VanGuiID seed = 0);
VANGUI_API VanGuiID       VanHashStr(const char* data, size_t data_size = 0, VanGuiID seed = 0);
VANGUI_API const char*   VanHashSkipUncontributingPrefix(const char* label);

// Helpers: Sorting
#ifndef VanQsort
inline void             VanQsort(void* base, size_t count, size_t size_of_element, int(VANGUI_CDECL *compare_func)(void const*, void const*)) { if (count > 1) qsort(base, count, size_of_element, compare_func); }
#endif

// Helpers: Color Blending
VANGUI_API VanU32         VanAlphaBlendColors(VanU32 col_a, VanU32 col_b);

// Helpers: Bit manipulation
[[nodiscard]] inline bool         VanIsPowerOfTwo(int v) noexcept          { return v > 0 && std::has_single_bit(static_cast<unsigned>(v)); }
[[nodiscard]] inline bool         VanIsPowerOfTwo(VanU64 v) noexcept       { return v != 0 && std::has_single_bit(v); }
[[nodiscard]] inline int          VanUpperPowerOfTwo(int v) noexcept        { v--; v |= v >> 1; v |= v >> 2; v |= v >> 4; v |= v >> 8; v |= v >> 16; v++; return v; }
[[nodiscard]] inline unsigned int VanCountSetBits(unsigned int v) noexcept  { return static_cast<unsigned int>(std::popcount(v)); }

// Helpers: String
#define VanStrlen strlen
#define VanMemchr memchr
VANGUI_API int           VanStricmp(const char* str1, const char* str2);                      // Case insensitive compare.
VANGUI_API int           VanStrnicmp(const char* str1, const char* str2, size_t count);       // Case insensitive compare to a certain count.
VANGUI_API void          VanStrncpy(char* dst, const char* src, size_t count);                // Copy to a certain count and always zero terminate (strncpy doesn't).
VANGUI_API char*         VanStrdup(const char* str);                                          // Duplicate a string.
VANGUI_API void*         VanMemdup(const void* src, size_t size);                             // Duplicate a chunk of memory.
VANGUI_API char*         VanStrdupcpy(char* dst, size_t* p_dst_size, const char* str);        // Copy in provided buffer, recreate buffer if needed.
VANGUI_API const char*   VanStrchrRange(const char* str_begin, const char* str_end, char c);  // Find first occurrence of 'c' in string range.
VANGUI_API const char*   VanStreolRange(const char* str, const char* str_end);                // End end-of-line
VANGUI_API const char*   VanStristr(const char* haystack, const char* haystack_end, const char* needle, const char* needle_end);  // Find a substring in a string range.
VANGUI_API void          VanStrTrimBlanks(char* str);                                         // Remove leading and trailing blanks from a buffer.
VANGUI_API const char*   VanStrSkipBlank(const char* str);                                    // Find first non-blank character.
VANGUI_API int           VanStrlenW(const VanWchar* str);                                      // Computer string length (VanWchar string)
VANGUI_API const char*   VanStrbol(const char* buf_mid_line, const char* buf_begin);          // Find beginning-of-line
VAN_MSVC_RUNTIME_CHECKS_OFF
inline char             VanToUpper(char c)               { return (c >= 'a' && c <= 'z') ? c &= ~32 : c; }
inline bool             VanCharIsBlankA(char c)          { return c == ' ' || c == '\t'; }
inline bool             VanCharIsBlankW(unsigned int c)  { return c == ' ' || c == '\t' || c == 0x3000; }
inline bool             VanCharIsXdigitA(char c)         { return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f'); }
VAN_MSVC_RUNTIME_CHECKS_RESTORE

// Helpers: Formatting
VANGUI_API int           VanFormatString(char* buf, size_t buf_size, const char* fmt, ...) VAN_FMTARGS(3);
VANGUI_API int           VanFormatStringV(char* buf, size_t buf_size, const char* fmt, va_list args) VAN_FMTLIST(3);
VANGUI_API void          VanFormatStringToTempBuffer(const char** out_buf, const char** out_buf_end, const char* fmt, ...) VAN_FMTARGS(3);
VANGUI_API void          VanFormatStringToTempBufferV(const char** out_buf, const char** out_buf_end, const char* fmt, va_list args) VAN_FMTLIST(3);
VANGUI_API const char*   VanParseFormatFindStart(const char* format);
VANGUI_API const char*   VanParseFormatFindEnd(const char* format);
VANGUI_API const char*   VanParseFormatTrimDecorations(const char* format, char* buf, size_t buf_size);
VANGUI_API void          VanParseFormatSanitizeForPrinting(const char* fmt_in, char* fmt_out, size_t fmt_out_size);
VANGUI_API const char*   VanParseFormatSanitizeForScanning(const char* fmt_in, char* fmt_out, size_t fmt_out_size);
VANGUI_API int           VanParseFormatPrecision(const char* format, int default_value);

// Helpers: UTF-8 <> wchar conversions
VANGUI_API int           VanTextCharToUtf8(char out_buf[5], unsigned int c);                                                      // return output UTF-8 bytes count
VANGUI_API int           VanTextStrToUtf8(char* out_buf, int out_buf_size, const VanWchar* in_text, const VanWchar* in_text_end);   // return output UTF-8 bytes count
VANGUI_API int           VanTextCharFromUtf8(unsigned int* out_char, const char* in_text, const char* in_text_end);               // read one character. return input UTF-8 bytes count
VANGUI_API int           VanTextStrFromUtf8(VanWchar* out_buf, int out_buf_size, const char* in_text, const char* in_text_end, const char** in_remaining = nullptr);   // return input UTF-8 bytes count
VANGUI_API int           VanTextCountCharsFromUtf8(const char* in_text, const char* in_text_end);                                 // return number of UTF-8 code-points (NOT bytes count)
VANGUI_API int           VanTextCountUtf8BytesFromChar(const char* in_text, const char* in_text_end);                             // return number of bytes to express one char in UTF-8
VANGUI_API int           VanTextCountUtf8BytesFromStr(const VanWchar* in_text, const VanWchar* in_text_end);                        // return number of bytes to express string in UTF-8
VANGUI_API const char*   VanTextFindPreviousUtf8Codepoint(const char* in_text_start, const char* in_p);                           // return previous UTF-8 code-point.
VANGUI_API const char*   VanTextFindValidUtf8CodepointEnd(const char* in_text_start, const char* in_text_end, const char* in_p);  // return previous UTF-8 code-point if 'in_p' is not the end of a valid one.
VANGUI_API int           VanTextCountLines(const char* in_text, const char* in_text_end);                                         // return number of lines taken by text. trailing carriage return doesn't count as an extra line.

// Helpers: High-level text functions (DO NOT USE!!! THIS IS A MINIMAL SUBSET OF LARGER UPCOMING CHANGES)
enum VanDrawTextFlags_
{
    VanDrawTextFlags_None                = 0,
    VanDrawTextFlags_CpuFineClip         = 1 << 0,    // Must be == 1/true for legacy with 'bool cpu_fine_clip' arg to RenderText()
    VanDrawTextFlags_WrapKeepBlanks      = 1 << 1,
    VanDrawTextFlags_StopOnNewLine       = 1 << 2,
};
VANGUI_API VanVec2        VanFontCalcTextSizeEx(VanFont* font, float size, float max_width, float wrap_width, const char* text_begin, const char* text_end_display, const char* text_end, const char** out_remaining, VanVec2* out_offset, VanDrawTextFlags flags);
VANGUI_API const char*   VanFontCalcWordWrapPositionEx(VanFont* font, float size, const char* text, const char* text_end, float wrap_width, VanDrawTextFlags flags = 0);
VANGUI_API const char*   VanTextCalcWordWrapNextLineStart(const char* text, const char* text_end, VanDrawTextFlags flags = 0); // trim trailing space and find beginning of next line

// Character classification for word-wrapping logic
enum VanWcharClass
{
    VanWcharClass_Blank, VanWcharClass_Punct, VanWcharClass_Other
};
VANGUI_API void          VanTextInitClassifiers();
VANGUI_API void          VanTextClassifierClear(VanU32* bits, unsigned int codepoint_min, unsigned int codepoint_end, VanWcharClass char_class);
VANGUI_API void          VanTextClassifierSetCharClass(VanU32* bits, unsigned int codepoint_min, unsigned int codepoint_end, VanWcharClass char_class, unsigned int c);
VANGUI_API void          VanTextClassifierSetCharClassFromStr(VanU32* bits, unsigned int codepoint_min, unsigned int codepoint_end, VanWcharClass char_class, const char* s);

// Helpers: File System
#ifdef VANGUI_DISABLE_FILE_FUNCTIONS
#define VANGUI_DISABLE_DEFAULT_FILE_FUNCTIONS
using VanFileHandle = void*;
inline VanFileHandle         VanFileOpen(const char*, const char*)                    { return nullptr; }
inline bool                 VanFileClose(VanFileHandle)                               { return false; }
inline VanU64                VanFileGetSize(VanFileHandle)                             { return (VanU64)-1; }
inline VanU64                VanFileRead(void*, VanU64, VanU64, VanFileHandle)           { return 0; }
inline VanU64                VanFileWrite(const void*, VanU64, VanU64, VanFileHandle)    { return 0; }
#endif
#ifndef VANGUI_DISABLE_DEFAULT_FILE_FUNCTIONS
using VanFileHandle = FILE*;
VANGUI_API VanFileHandle      VanFileOpen(const char* filename, const char* mode);
VANGUI_API bool              VanFileClose(VanFileHandle file);
VANGUI_API VanU64             VanFileGetSize(VanFileHandle file);
VANGUI_API VanU64             VanFileRead(void* data, VanU64 size, VanU64 count, VanFileHandle file);
VANGUI_API VanU64             VanFileWrite(const void* data, VanU64 size, VanU64 count, VanFileHandle file);
#else
#define VANGUI_DISABLE_TTY_FUNCTIONS // Can't use stdout, fflush if we are not using default file functions
#endif
VANGUI_API void*             VanFileLoadToMemory(const char* filename, const char* mode, size_t* out_file_size = nullptr, int padding_bytes = 0);

// Helpers: Maths
VAN_MSVC_RUNTIME_CHECKS_OFF
// - Wrapper for standard libs functions. (Note that vangui_demo.cpp does _not_ use them to keep the code easy to copy)
#ifndef VANGUI_DISABLE_DEFAULT_MATH_FUNCTIONS
#define VanFabs(X)           std::fabs(X)
#define VanSqrt(X)           std::sqrt(X)
#define VanFmod(X, Y)        std::fmod((X), (Y))
#define VanCos(X)            std::cos(X)
#define VanSin(X)            std::sin(X)
#define VanAcos(X)           std::acos(X)
#define VanAtan2(Y, X)       std::atan2((Y), (X))
// VanAtof: safe wrapper around strtod. Returns 0.0 if input is null/empty, otherwise
// converts using strtod which sets errno and returns +-HUGE_VAL on overflow rather than
// silently returning 0 as std::atof would on error.
[[nodiscard]] static inline double VanAtof(const char* s) noexcept
{
    if (!s || !*s) return 0.0;
    char* end = nullptr;
    double v = std::strtod(s, &end);
    return (end != s) ? v : 0.0;
}
#define VanCeil(X)           std::ceil(X)
[[nodiscard]] inline float  VanPow(float x, float y) noexcept   { return std::pow(x, y); }   // DragBehaviorT/SliderBehaviorT uses VanPow with either float/double and need the precision
[[nodiscard]] inline double VanPow(double x, double y) noexcept { return std::pow(x, y); }
[[nodiscard]] inline float  VanLog(float x) noexcept            { return std::log(x); }      // DragBehaviorT/SliderBehaviorT uses VanLog with either float/double and need the precision
[[nodiscard]] inline double VanLog(double x) noexcept           { return std::log(x); }
[[nodiscard]] inline int    VanAbs(int x) noexcept              { return std::abs(x); }
[[nodiscard]] inline float  VanAbs(float x) noexcept            { return std::fabs(x); }
[[nodiscard]] inline double VanAbs(double x) noexcept           { return std::fabs(x); }
[[nodiscard]] inline float  VanSign(float x) noexcept           { return (x < 0.0f) ? -1.0f : (x > 0.0f) ? 1.0f : 0.0f; }
[[nodiscard]] inline double VanSign(double x) noexcept          { return (x < 0.0) ? -1.0 : (x > 0.0) ? 1.0 : 0.0; }
#ifdef VANGUI_ENABLE_SSE
[[nodiscard]] inline float  VanRsqrt(float x) noexcept          { return _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(x))); }
#else
[[nodiscard]] inline float  VanRsqrt(float x) noexcept          { return 1.0f / std::sqrt(x); }
#endif
[[nodiscard]] inline double VanRsqrt(double x) noexcept         { return 1.0 / std::sqrt(x); }
#endif
// - VanMin/VanMax/VanClamp/VanLerp/VanSwap are used by widgets which support variety of types: signed/unsigned int/long long float/double
template<typename T> [[nodiscard]] constexpr T    VanMin(T lhs, T rhs) noexcept                          { return std::min(lhs, rhs); }
template<typename T> [[nodiscard]] constexpr T    VanMax(T lhs, T rhs) noexcept                          { return std::max(lhs, rhs); }
template<typename T> [[nodiscard]] constexpr T    VanClamp(T v, T mn, T mx) noexcept                     { return std::clamp(v, mn, mx); }
template<typename T> [[nodiscard]] constexpr T    VanLerp(double a, double b, float t) noexcept          { return static_cast<T>(a + (b - a) * static_cast<double>(t)); }
template<typename T> [[nodiscard]] constexpr T    VanLerp(T a, T b, float t) noexcept                    { return static_cast<T>(static_cast<float>(a) + static_cast<float>(b - a) * t); }
template<typename T> constexpr void               VanSwap(T& a, T& b) noexcept                           { std::swap(a, b); }
template<typename T> [[nodiscard]] constexpr T    VanAddClampOverflow(T a, T b, T mn, T mx) noexcept     { if (b < 0 && (a < mn - b)) return mn; if (b > 0 && (a > mx - b)) return mx; return a + b; }
template<typename T> [[nodiscard]] constexpr T    VanSubClampOverflow(T a, T b, T mn, T mx) noexcept     { if (b > 0 && (a < mn + b)) return mn; if (b < 0 && (a > mx + b)) return mx; return a - b; }
// - Misc maths helpers
[[nodiscard]] inline VanVec2 VanMin(const VanVec2& lhs, const VanVec2& rhs) noexcept                { return VanVec2(std::min(lhs.x, rhs.x), std::min(lhs.y, rhs.y)); }
[[nodiscard]] inline VanVec2 VanMax(const VanVec2& lhs, const VanVec2& rhs) noexcept                { return VanVec2(std::max(lhs.x, rhs.x), std::max(lhs.y, rhs.y)); }
[[nodiscard]] inline VanVec2 VanClamp(const VanVec2& v, const VanVec2& mn, const VanVec2& mx) noexcept { return VanVec2(std::clamp(v.x, mn.x, mx.x), std::clamp(v.y, mn.y, mx.y)); }
[[nodiscard]] inline VanVec2 VanLerp(const VanVec2& a, const VanVec2& b, float t) noexcept          { return VanVec2(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t); }
[[nodiscard]] inline VanVec2 VanLerp(const VanVec2& a, const VanVec2& b, const VanVec2& t) noexcept { return VanVec2(a.x + (b.x - a.x) * t.x, a.y + (b.y - a.y) * t.y); }
[[nodiscard]] inline VanVec4 VanLerp(const VanVec4& a, const VanVec4& b, float t) noexcept          { return VanVec4(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t, a.w + (b.w - a.w) * t); }
[[nodiscard]] inline float   VanSaturate(float f) noexcept                                           { return std::clamp(f, 0.0f, 1.0f); }
[[nodiscard]] inline float   VanLengthSqr(const VanVec2& lhs) noexcept       { return (lhs.x * lhs.x) + (lhs.y * lhs.y); }
[[nodiscard]] inline float   VanLengthSqr(const VanVec4& lhs) noexcept       { return (lhs.x * lhs.x) + (lhs.y * lhs.y) + (lhs.z * lhs.z) + (lhs.w * lhs.w); }
[[nodiscard]] inline float   VanInvLength(const VanVec2& lhs, float fail_value) noexcept { float d = (lhs.x * lhs.x) + (lhs.y * lhs.y); if (d > 0.0f) return VanRsqrt(d); return fail_value; }
[[nodiscard]] inline float   VanTrunc(float f) noexcept                      { return std::trunc(f); }
[[nodiscard]] inline VanVec2  VanTrunc(const VanVec2& v) noexcept             { return VanVec2(std::trunc(v.x), std::trunc(v.y)); }
[[nodiscard]] inline float   VanFloor(float f) noexcept                      { return std::floor(f); }
[[nodiscard]] inline VanVec2  VanFloor(const VanVec2& v) noexcept             { return VanVec2(std::floor(v.x), std::floor(v.y)); }
[[nodiscard]] inline float   VanTrunc64(float f) noexcept                    { return static_cast<float>(static_cast<VanS64>(f)); }
[[nodiscard]] inline float   VanRound64(float f) noexcept                    { return static_cast<float>(static_cast<VanS64>(f + 0.5f)); } // Positive values only.
[[nodiscard]] inline int     VanModPositive(int a, int b) noexcept            { return (a + b) % b; }
[[nodiscard]] inline float   VanDot(const VanVec2& a, const VanVec2& b) noexcept { return a.x * b.x + a.y * b.y; }
[[nodiscard]] inline VanVec2  VanRotate(const VanVec2& v, float cos_a, float sin_a) noexcept { return VanVec2(v.x * cos_a - v.y * sin_a, v.x * sin_a + v.y * cos_a); }
[[nodiscard]] inline float   VanLinearSweep(float current, float target, float speed) noexcept { if (current < target) return VanMin(current + speed, target); if (current > target) return VanMax(current - speed, target); return current; }
[[nodiscard]] inline float   VanLinearRemapClamp(float s0, float s1, float d0, float d1, float x) noexcept { return VanSaturate((x - s0) / (s1 - s0)) * (d1 - d0) + d0; }
[[nodiscard]] inline VanVec2  VanMul(const VanVec2& lhs, const VanVec2& rhs) noexcept { return VanVec2(lhs.x * rhs.x, lhs.y * rhs.y); }
[[nodiscard]] inline bool    VanIsFloatAboveGuaranteedIntegerPrecision(float f) noexcept { return f <= -16777216 || f >= 16777216; }
[[nodiscard]] inline float   VanExponentialMovingAverage(float avg, float sample, int n) noexcept { avg -= avg / static_cast<float>(n); avg += sample / static_cast<float>(n); return avg; }
VAN_MSVC_RUNTIME_CHECKS_RESTORE

// Helpers: Geometry
VANGUI_API VanVec2     VanBezierCubicCalc(const VanVec2& p1, const VanVec2& p2, const VanVec2& p3, const VanVec2& p4, float t);
VANGUI_API VanVec2     VanBezierCubicClosestPoint(const VanVec2& p1, const VanVec2& p2, const VanVec2& p3, const VanVec2& p4, const VanVec2& p, int num_segments);       // For curves with explicit number of segments
VANGUI_API VanVec2     VanBezierCubicClosestPointCasteljau(const VanVec2& p1, const VanVec2& p2, const VanVec2& p3, const VanVec2& p4, const VanVec2& p, float tess_tol);// For auto-tessellated curves you can use tess_tol = style.CurveTessellationTol
VANGUI_API VanVec2     VanBezierQuadraticCalc(const VanVec2& p1, const VanVec2& p2, const VanVec2& p3, float t);
VANGUI_API VanVec2     VanLineClosestPoint(const VanVec2& a, const VanVec2& b, const VanVec2& p);
VANGUI_API bool       VanTriangleContainsPoint(const VanVec2& a, const VanVec2& b, const VanVec2& c, const VanVec2& p);
VANGUI_API VanVec2     VanTriangleClosestPoint(const VanVec2& a, const VanVec2& b, const VanVec2& c, const VanVec2& p);
VANGUI_API void       VanTriangleBarycentricCoords(const VanVec2& a, const VanVec2& b, const VanVec2& c, const VanVec2& p, float& out_u, float& out_v, float& out_w);
[[nodiscard]] inline float VanTriangleArea(const VanVec2& a, const VanVec2& b, const VanVec2& c) noexcept      { return VanFabs((a.x * (b.y - c.y)) + (b.x * (c.y - a.y)) + (c.x * (a.y - b.y))) * 0.5f; }
[[nodiscard]] inline bool  VanTriangleIsClockwise(const VanVec2& a, const VanVec2& b, const VanVec2& c) noexcept { return ((b.x - a.x) * (c.y - b.y)) - ((c.x - b.x) * (b.y - a.y)) > 0.0f; }

// Helper: VanVec1 (1D vector)
// (this odd construct is used to facilitate the transition between 1D and 2D, and the maintenance of some branches/patches)
VAN_MSVC_RUNTIME_CHECKS_OFF
struct VanVec1
{
    float   x;
    constexpr VanVec1()         : x(0.0f) { }
    constexpr VanVec1(float _x) : x(_x) { }
};

// Helper: VanVec2i (2D vector, integer)
struct VanVec2i
{
    int         x, y;
    constexpr VanVec2i()                             : x(0), y(0) {}
    constexpr VanVec2i(int _x, int _y)               : x(_x), y(_y) {}
};

// Helper: VanVec2ih (2D vector, half-size integer, for long-term packed storage)
struct VanVec2ih
{
    short   x, y;
    constexpr VanVec2ih()                           : x(0), y(0) {}
    constexpr VanVec2ih(short _x, short _y)         : x(_x), y(_y) {}
    constexpr explicit VanVec2ih(const VanVec2& rhs) : x(static_cast<short>(rhs.x)), y(static_cast<short>(rhs.y)) {}
};

// Helper: VanRect (2D axis aligned bounding-box)
// NB: we can't rely on VanVec2 math operators being available here!
struct VANGUI_API VanRect
{
    VanVec2      Min;    // Upper-left
    VanVec2      Max;    // Lower-right

    constexpr VanRect()                                        : Min(0.0f, 0.0f), Max(0.0f, 0.0f)  {}
    constexpr VanRect(const VanVec2& min, const VanVec2& max)    : Min(min), Max(max)                {}
    constexpr VanRect(const VanVec4& v)                         : Min(v.x, v.y), Max(v.z, v.w)      {}
    constexpr VanRect(float x1, float y1, float x2, float y2)  : Min(x1, y1), Max(x2, y2)          {}

    [[nodiscard]] VanVec2  GetCenter() const noexcept                   { return VanVec2((Min.x + Max.x) * 0.5f, (Min.y + Max.y) * 0.5f); }
    [[nodiscard]] VanVec2  GetSize() const noexcept                     { return VanVec2(Max.x - Min.x, Max.y - Min.y); }
    [[nodiscard]] float   GetWidth() const noexcept                    { return Max.x - Min.x; }
    [[nodiscard]] float   GetHeight() const noexcept                   { return Max.y - Min.y; }
    [[nodiscard]] float   GetArea() const noexcept                     { return (Max.x - Min.x) * (Max.y - Min.y); }
    [[nodiscard]] VanVec2  GetTL() const noexcept                       { return Min; }                   // Top-left
    [[nodiscard]] VanVec2  GetTR() const noexcept                       { return VanVec2(Max.x, Min.y); }  // Top-right
    [[nodiscard]] VanVec2  GetBL() const noexcept                       { return VanVec2(Min.x, Max.y); }  // Bottom-left
    [[nodiscard]] VanVec2  GetBR() const noexcept                       { return Max; }                   // Bottom-right
    [[nodiscard]] bool    Contains(const VanVec2& p) const noexcept     { return p.x     >= Min.x && p.y     >= Min.y && p.x     <  Max.x && p.y     <  Max.y; }
    [[nodiscard]] bool    Contains(const VanRect& r) const noexcept     { return r.Min.x >= Min.x && r.Min.y >= Min.y && r.Max.x <= Max.x && r.Max.y <= Max.y; }
    [[nodiscard]] bool    ContainsWithPad(const VanVec2& p, const VanVec2& pad) const noexcept { return p.x >= Min.x - pad.x && p.y >= Min.y - pad.y && p.x < Max.x + pad.x && p.y < Max.y + pad.y; }
    [[nodiscard]] bool    Overlaps(const VanRect& r) const noexcept     { return r.Min.y <  Max.y && r.Max.y >  Min.y && r.Min.x <  Max.x && r.Max.x >  Min.x; }
    void        Add(const VanVec2& p)                { if (Min.x > p.x)     Min.x = p.x;     if (Min.y > p.y)     Min.y = p.y;     if (Max.x < p.x)     Max.x = p.x;     if (Max.y < p.y)     Max.y = p.y; }
    void        Add(const VanRect& r)                { if (Min.x > r.Min.x) Min.x = r.Min.x; if (Min.y > r.Min.y) Min.y = r.Min.y; if (Max.x < r.Max.x) Max.x = r.Max.x; if (Max.y < r.Max.y) Max.y = r.Max.y; }
    void        AddX(float x)                       { if (Min.x > x)       Min.x = x;       if (Max.x < x)       Max.x = x; }
    void        AddY(float y)                       { if (Min.y > y)       Min.y = y;       if (Max.y < y)       Max.y = y; }
    void        Expand(const float amount)          { Min.x -= amount;   Min.y -= amount;   Max.x += amount;   Max.y += amount; }
    void        Expand(const VanVec2& amount)        { Min.x -= amount.x; Min.y -= amount.y; Max.x += amount.x; Max.y += amount.y; }
    void        Translate(const VanVec2& d)          { Min.x += d.x; Min.y += d.y; Max.x += d.x; Max.y += d.y; }
    void        TranslateX(float dx)                { Min.x += dx; Max.x += dx; }
    void        TranslateY(float dy)                { Min.y += dy; Max.y += dy; }
    void        ClipWith(const VanRect& r)           { Min = VanMax(Min, r.Min); Max = VanMin(Max, r.Max); }                   // Simple version, may lead to an inverted rectangle, which is fine for Contains/Overlaps test but not for display.
    void        ClipWithFull(const VanRect& r)       { Min = VanClamp(Min, r.Min, r.Max); Max = VanClamp(Max, r.Min, r.Max); } // Full version, ensure both points are fully clipped.
    [[nodiscard]] bool    IsInverted() const noexcept                  { return Min.x > Max.x || Min.y > Max.y; }
    [[nodiscard]] VanVec4  ToVec4() const noexcept                      { return VanVec4(Min.x, Min.y, Max.x, Max.y); }
    [[nodiscard]] const VanVec4& AsVec4() const noexcept                { return *(const VanVec4*)&Min.x; }
};

// Helper: VanBitArray
#define         VAN_BITARRAY_TESTBIT(_ARRAY, _N)                 ((_ARRAY[(_N) >> 5] & ((VanU32)1 << ((_N) & 31))) != 0) // Macro version of VanBitArrayTestBit(): ensure args have side-effect or are costly!
#define         VAN_BITARRAY_CLEARBIT(_ARRAY, _N)                ((_ARRAY[(_N) >> 5] &= ~((VanU32)1 << ((_N) & 31))))    // Macro version of VanBitArrayClearBit(): ensure args have side-effect or are costly!
[[nodiscard]] inline size_t VanBitArrayGetStorageSizeInBytes(int bitcount) noexcept { return static_cast<size_t>((bitcount + 31) >> 5) << 2; }
inline void     VanBitArrayClearAllBits(VanU32* arr, int bitcount) noexcept { memset(arr, 0, VanBitArrayGetStorageSizeInBytes(bitcount)); }
[[nodiscard]] inline bool VanBitArrayTestBit(const VanU32* arr, int n) noexcept  { VanU32 mask = (VanU32)1 << (n & 31); return (arr[n >> 5] & mask) != 0; }
inline void     VanBitArrayClearBit(VanU32* arr, int n) noexcept           { VanU32 mask = (VanU32)1 << (n & 31); arr[n >> 5] &= ~mask; }
inline void     VanBitArraySetBit(VanU32* arr, int n) noexcept             { VanU32 mask = (VanU32)1 << (n & 31); arr[n >> 5] |= mask; }
inline void     VanBitArraySetBitRange(VanU32* arr, int n, int n2) // Works on range [n..n2)
{
    n2--;
    while (n <= n2)
    {
        int a_mod = (n & 31);
        int b_mod = (n2 > (n | 31) ? 31 : (n2 & 31)) + 1;
        VanU32 mask = static_cast<VanU32>(((VanU64)1 << b_mod) - 1) & ~static_cast<VanU32>(((VanU64)1 << a_mod) - 1);
        arr[n >> 5] |= mask;
        n = (n + 32) & ~31;
    }
}

using VanBitArrayPtr = VanU32*; // Name for use in structs

// Helper: VanBitArray class (wrapper over VanBitArray functions)
// Store 1-bit per value.
template<int BITCOUNT, int OFFSET = 0>
struct VanBitArray
{
    VanU32           Data[(BITCOUNT + 31) >> 5];
    VanBitArray()                                              { ClearAllBits(); }
    void            ClearAllBits() noexcept                  { memset(Data, 0, sizeof(Data)); }
    void            SetAllBits() noexcept                    { memset(Data, 255, sizeof(Data)); }
    [[nodiscard]] bool TestBit(int n) const noexcept         { n += OFFSET; VAN_ASSERT(n >= 0 && n < BITCOUNT); return VAN_BITARRAY_TESTBIT(Data, n); }
    void            SetBit(int n) noexcept                   { n += OFFSET; VAN_ASSERT(n >= 0 && n < BITCOUNT); VanBitArraySetBit(Data, n); }
    void            ClearBit(int n) noexcept                 { n += OFFSET; VAN_ASSERT(n >= 0 && n < BITCOUNT); VanBitArrayClearBit(Data, n); }
    void            SetBitRange(int n, int n2) noexcept      { n += OFFSET; n2 += OFFSET; VAN_ASSERT(n >= 0 && n < BITCOUNT && n2 > n && n2 <= BITCOUNT); VanBitArraySetBitRange(Data, n, n2); } // Works on range [n..n2)
    [[nodiscard]] bool operator[](int n) const noexcept      { n += OFFSET; VAN_ASSERT(n >= 0 && n < BITCOUNT); return VAN_BITARRAY_TESTBIT(Data, n); }
};

// Helper: VanBitVector
// Store 1-bit per value.
struct VANGUI_API VanBitVector
{
    VanVector<VanU32> Storage;
    void            Create(int sz)                        { Storage.resize((sz + 31) >> 5); memset(Storage.Data, 0, static_cast<size_t>(Storage.Size) * sizeof(Storage.Data[0])); }
    void            Clear()                               { Storage.clear(); }
    [[nodiscard]] bool TestBit(int n) const noexcept      { VAN_ASSERT(n < (Storage.Size << 5)); return VAN_BITARRAY_TESTBIT(Storage.Data, n); }
    void            SetBit(int n) noexcept                { VAN_ASSERT(n < (Storage.Size << 5)); VanBitArraySetBit(Storage.Data, n); }
    void            ClearBit(int n) noexcept              { VAN_ASSERT(n < (Storage.Size << 5)); VanBitArrayClearBit(Storage.Data, n); }
};
VAN_MSVC_RUNTIME_CHECKS_RESTORE

// Helper: VanSpan<>
// Pointing to a span of data we don't own.
template<typename T>
struct VanSpan
{
    T*                  Data;
    T*                  DataEnd;

    // Constructors, destructor
    inline VanSpan() noexcept                                { Data = DataEnd = nullptr; }
    inline VanSpan(T* data, int size) noexcept               { Data = data; DataEnd = data + size; }
    inline VanSpan(T* data, T* data_end) noexcept            { Data = data; DataEnd = data_end; }

    inline void         set(T* data, int size) noexcept      { Data = data; DataEnd = data + size; }
    inline void         set(T* data, T* data_end) noexcept   { Data = data; DataEnd = data_end; }
    [[nodiscard]] inline int    size() const noexcept        { return static_cast<int>(static_cast<ptrdiff_t>(DataEnd - Data)); }
    [[nodiscard]] inline size_t size_in_bytes() const noexcept { return static_cast<size_t>(DataEnd - Data) * sizeof(T); }
    inline T&           operator[](int i) noexcept           { T* p = Data + i; VAN_ASSERT(p >= Data && p < DataEnd); return *p; }
    inline const T&     operator[](int i) const noexcept     { const T* p = Data + i; VAN_ASSERT(p >= Data && p < DataEnd); return *p; }

    [[nodiscard]] inline T*       begin() noexcept           { return Data; }
    [[nodiscard]] inline const T* begin() const noexcept     { return Data; }
    [[nodiscard]] inline T*       end() noexcept             { return DataEnd; }
    [[nodiscard]] inline const T* end() const noexcept       { return DataEnd; }

    // Utilities
    [[nodiscard]] inline int index_from_ptr(const T* it) const noexcept { VAN_ASSERT(it >= Data && it < DataEnd); const ptrdiff_t off = it - Data; return static_cast<int>(off); }
};

// Helper: VanSpanAllocator<>
// Facilitate storing multiple chunks into a single large block (the "arena")
// - Usage: call Reserve() N times, allocate GetArenaSizeInBytes() worth, pass it to SetArenaBasePtr(), call GetSpan() N times to retrieve the aligned ranges.
template<int CHUNKS>
struct VanSpanAllocator
{
    char*   BasePtr;
    int     CurrOff;
    int     CurrIdx;
    int     Offsets[CHUNKS];
    int     Sizes[CHUNKS];

    VanSpanAllocator() noexcept                                { memset((void*)this, 0, sizeof(*this)); } // POD-safe zero-init
    inline void  Reserve(int n, size_t sz, int a=4) noexcept  { VAN_ASSERT(n == CurrIdx && n < CHUNKS); CurrOff = VAN_MEMALIGN(CurrOff, a); Offsets[n] = CurrOff; Sizes[n] = static_cast<int>(sz); CurrIdx++; CurrOff += static_cast<int>(sz); }
    [[nodiscard]] inline int  GetArenaSizeInBytes() const noexcept { return CurrOff; }
    inline void  SetArenaBasePtr(void* base_ptr) noexcept     { BasePtr = static_cast<char*>(base_ptr); }
    [[nodiscard]] inline void* GetSpanPtrBegin(int n) noexcept { VAN_ASSERT(n >= 0 && n < CHUNKS && CurrIdx == CHUNKS); return static_cast<void*>(BasePtr + Offsets[n]); }
    [[nodiscard]] inline void* GetSpanPtrEnd(int n) noexcept   { VAN_ASSERT(n >= 0 && n < CHUNKS && CurrIdx == CHUNKS); return static_cast<void*>(BasePtr + Offsets[n] + Sizes[n]); }
    template<typename T>
    inline void  GetSpan(int n, VanSpan<T>* span) noexcept     { span->set(static_cast<T*>(GetSpanPtrBegin(n)), static_cast<T*>(GetSpanPtrEnd(n))); }
};

// Helper: VanStableVector<>
// Allocating chunks of BLOCKSIZE items. Objects pointers are never invalidated when growing, only by clear().
// Important: does not destruct anything!
// Implemented only the minimum set of functions we need for it.
template<typename T, int BLOCKSIZE>
struct VanStableVector
{
    int                 Size = 0;
    int                 Capacity = 0;
    VanVector<T*>        Blocks;

    // Functions
    inline ~VanStableVector()                        { for (T* block : Blocks) VAN_FREE(block); }

    inline void         clear()                     { Size = Capacity = 0; Blocks.clear_delete(); }
    inline void         resize(int new_size)        { if (new_size > Capacity) reserve(new_size); Size = new_size; }
    inline void         reserve(int new_cap)
    {
        new_cap = VAN_MEMALIGN(new_cap, BLOCKSIZE);
        int old_count = Capacity / BLOCKSIZE;
        int new_count = new_cap / BLOCKSIZE;
        if (new_count <= old_count)
            return;
        Blocks.resize(new_count);
        for (int n = old_count; n < new_count; n++)
            Blocks[n] = static_cast<T*>(VAN_ALLOC(sizeof(T) * BLOCKSIZE));
        Capacity = new_cap;
    }
    inline T&           operator[](int i)           { VAN_ASSERT(i >= 0 && i < Size); return Blocks[i / BLOCKSIZE][i % BLOCKSIZE]; }
    inline const T&     operator[](int i) const     { VAN_ASSERT(i >= 0 && i < Size); return Blocks[i / BLOCKSIZE][i % BLOCKSIZE]; }
    inline T*           push_back(const T& v)       { int i = Size; VAN_ASSERT(i >= 0); if (Size == Capacity) reserve(Capacity + BLOCKSIZE); void* ptr = &Blocks[i / BLOCKSIZE][i % BLOCKSIZE]; memcpy(ptr, &v, sizeof(v)); Size++; return static_cast<T*>(ptr); }
};

// Helper: VanPool<>
// Basic keyed storage for contiguous instances, slow/amortized insertion, O(1) indexable, O(Log N) queries by ID over a dense/hot buffer,
// Honor constructor/destructor. Add/remove invalidate all pointers. Indexes have the same lifetime as the associated object.
using VanPoolIdx = int;
template<typename T>
struct VanPool
{
    VanVector<T>     Buf;        // Contiguous data
    VanGuiStorage    Map;        // ID->Index
    VanPoolIdx       FreeIdx;    // Next free idx to use
    VanPoolIdx       AliveCount; // Number of active/alive items (for display purpose)

    VanPool()    { FreeIdx = AliveCount = 0; }
    ~VanPool()   { Clear(); }
    T*          GetByKey(VanGuiID key)               { int idx = Map.GetInt(key, -1); return (idx != -1) ? &Buf[idx] : nullptr; }
    T*          GetByIndex(VanPoolIdx n)             { return &Buf[n]; }
    VanPoolIdx   GetIndex(const T* p) const          { VAN_ASSERT(p >= Buf.Data && p < Buf.Data + Buf.Size); return static_cast<VanPoolIdx>(p - Buf.Data); }
    T*          GetOrAddByKey(VanGuiID key)          { int* p_idx = Map.GetIntRef(key, -1); if (*p_idx != -1) return &Buf[*p_idx]; *p_idx = FreeIdx; return Add(); }
    bool        Contains(const T* p) const          { return (p >= Buf.Data && p < Buf.Data + Buf.Size); }
    void        Clear()                             { for (int n = 0; n < Map.Data.Size; n++) { int idx = Map.Data[n].val_i; if (idx != -1) Buf[idx].~T(); } Map.Clear(); Buf.clear(); FreeIdx = AliveCount = 0; }
    T*          Add()                               { int idx = FreeIdx; if (idx == Buf.Size) { Buf.resize(Buf.Size + 1); FreeIdx++; } else { FreeIdx = *(int*)&Buf[idx]; } VAN_PLACEMENT_NEW(&Buf[idx]) T(); AliveCount++; return &Buf[idx]; }
    void        Remove(VanGuiID key, const T* p)     { Remove(key, GetIndex(p)); }
    void        Remove(VanGuiID key, VanPoolIdx idx)  { Buf[idx].~T(); *(int*)&Buf[idx] = FreeIdx; FreeIdx = idx; Map.SetInt(key, -1); AliveCount--; }
    void        Reserve(int capacity)               { Buf.reserve(capacity); Map.Data.reserve(capacity); }

    // To iterate a VanPool: for (int n = 0; n < pool.GetMapSize(); n++) if (T* t = pool.TryGetMapData(n)) { ... }
    // Can be avoided if you know .Remove() has never been called on the pool, or AliveCount == GetMapSize()
    int         GetAliveCount() const               { return AliveCount; }      // Number of active/alive items in the pool (for display purpose)
    int         GetBufSize() const                  { return Buf.Size; }
    int         GetMapSize() const                  { return Map.Data.Size; }   // It is the map we need iterate to find valid items, since we don't have "alive" storage anywhere
    T*          TryGetMapData(VanPoolIdx n)          { int idx = Map.Data[n].val_i; if (idx == -1) return nullptr; return GetByIndex(idx); }
};

// Helper: VanChunkStream<>
// Build and iterate a contiguous stream of variable-sized structures.
// This is used by Settings to store persistent data while reducing allocation count.
// We store the chunk size first, and align the final size on 4 bytes boundaries.
// The tedious/zealous amount of casting is to avoid -Wcast-align warnings.
template<typename T>
struct VanChunkStream
{
    VanVector<char>  Buf;

    void    clear()                     { Buf.clear(); }
    bool    empty() const               { return Buf.Size == 0; }
    int     size() const                { return Buf.Size; }
    T*      alloc_chunk(size_t sz)      { size_t HDR_SZ = 4; sz = VAN_MEMALIGN(HDR_SZ + sz, 4u); int off = Buf.Size; Buf.resize(off + static_cast<int>(sz)); (*reinterpret_cast<int*>(static_cast<void*>(Buf.Data + off))) = static_cast<int>(sz); return static_cast<T*>(static_cast<void*>(Buf.Data + off + static_cast<int>(HDR_SZ))); }
    T*      begin()                     { size_t HDR_SZ = 4; if (!Buf.Data) return nullptr; return static_cast<T*>(static_cast<void*>(Buf.Data + HDR_SZ)); }
    T*      next_chunk(T* p)            { size_t HDR_SZ = 4; VAN_ASSERT(p >= begin() && p < end()); p = static_cast<T*>(static_cast<void*>(static_cast<char*>(static_cast<void*>(p)) + chunk_size(p))); if (p == static_cast<T*>(static_cast<void*>(static_cast<char*>(static_cast<void*>(end())) + HDR_SZ))) return static_cast<T*>(nullptr); VAN_ASSERT(p < end()); return p; }
    int     chunk_size(const T* p)      { return (reinterpret_cast<const int*>(p))[-1]; }
    T*      end()                       { return static_cast<T*>(static_cast<void*>(Buf.Data + Buf.Size)); }
    int     offset_from_ptr(const T* p) { VAN_ASSERT(p >= begin() && p < end()); const ptrdiff_t off = reinterpret_cast<const char*>(p) - Buf.Data; return static_cast<int>(off); }
    T*      ptr_from_offset(int off)    { VAN_ASSERT(off >= 4 && off < Buf.Size); return static_cast<T*>(static_cast<void*>(Buf.Data + off)); }
    void    swap(VanChunkStream<T>& rhs) { rhs.Buf.swap(Buf); }
};

// Helper: VanGuiTextIndex
// Maintain a line index for a text buffer. This is a strong candidate to be moved into the public API.
struct VanGuiTextIndex
{
    VanVector<int>   Offsets;
    int             EndOffset = 0;                          // Because we don't own text buffer we need to maintain EndOffset (may bake in LineOffsets?)

    void            clear()                                 { Offsets.clear(); EndOffset = 0; }
    int             size()                                  { return Offsets.Size; }
    const char*     get_line_begin(const char* base, int n) { return base + (Offsets.Size != 0 ? Offsets[n] : 0); }
    const char*     get_line_end(const char* base, int n)   { return base + (n + 1 < Offsets.Size ? (Offsets[n + 1] - 1) : EndOffset); }
    void            append(const char* base, int old_size, int new_size);
};

// Helper: VanGuiStorage
VANGUI_API VanGuiStoragePair* VanLowerBound(VanGuiStoragePair* in_begin, VanGuiStoragePair* in_end, VanGuiID key);

//-----------------------------------------------------------------------------
// [SECTION] VanDrawList support
//-----------------------------------------------------------------------------

// VanDrawList: Helper function to calculate a circle's segment count given its radius and a "maximum error" value.
// Estimation of number of circle segment based on error is derived using method described in https://stackoverflow.com/a/2244088/15194693
// Number of segments (N) is calculated using equation:
//   N = ceil ( pi / acos(1 - error / r) )     where r > 0, error <= r
// Our equation is significantly simpler that one in the post thanks for choosing segment that is
// perpendicular to X axis. Follow steps in the article from this starting condition and you will
// will get this result.
//
// Rendering circles with an odd number of segments, while mathematically correct will produce
// asymmetrical results on the raster grid. Therefore we're rounding N to next even number (7->8, 8->8, 9->10 etc.)
#define VAN_ROUNDUP_TO_EVEN(_V)                                  ((((_V) + 1) / 2) * 2)
#define VAN_DRAWLIST_CIRCLE_AUTO_SEGMENT_MIN                     4
#define VAN_DRAWLIST_CIRCLE_AUTO_SEGMENT_MAX                     512
#define VAN_DRAWLIST_CIRCLE_AUTO_SEGMENT_CALC(_RAD,_MAXERROR)    VanClamp(VAN_ROUNDUP_TO_EVEN((int)VanCeil(VAN_PI / VanAcos(1 - VanMin((_MAXERROR), (_RAD)) / (_RAD)))), VAN_DRAWLIST_CIRCLE_AUTO_SEGMENT_MIN, VAN_DRAWLIST_CIRCLE_AUTO_SEGMENT_MAX)

// Raw equation from VAN_DRAWLIST_CIRCLE_AUTO_SEGMENT_CALC rewritten for 'r' and 'error'.
#define VAN_DRAWLIST_CIRCLE_AUTO_SEGMENT_CALC_R(_N,_MAXERROR)    ((_MAXERROR) / (1 - VanCos(VAN_PI / VanMax((float)(_N), VAN_PI))))
#define VAN_DRAWLIST_CIRCLE_AUTO_SEGMENT_CALC_ERROR(_N,_RAD)     ((1 - VanCos(VAN_PI / VanMax((float)(_N), VAN_PI))) / (_RAD))

// VanDrawList: Lookup table size for adaptive arc drawing, cover full circle.
#ifndef VAN_DRAWLIST_ARCFAST_TABLE_SIZE
#define VAN_DRAWLIST_ARCFAST_TABLE_SIZE                          48 // Number of samples in lookup table.
#endif
#define VAN_DRAWLIST_ARCFAST_SAMPLE_MAX                          VAN_DRAWLIST_ARCFAST_TABLE_SIZE // Sample index _PathArcToFastEx() for 360 angle.

// Data shared between all VanDrawList instances
// Conceptually this could have been called e.g. VanDrawListSharedContext
// Typically one VanGui context would create and maintain one of this.
// You may want to create your own instance of you try to VanDrawList completely without VanGui. In that case, watch out for future changes to this structure.
struct VANGUI_API VanDrawListSharedData
{
    VanVec2          TexUvWhitePixel;            // UV of white pixel in the atlas (== FontAtlas->TexUvWhitePixel)
    const VanVec4*   TexUvLines;                 // UV of anti-aliased lines in the atlas (== FontAtlas->TexUvLines)
    VanFontAtlas*    FontAtlas;                  // Current font atlas
    VanFont*         Font;                       // Current font (used for simplified AddText overload)
    float           FontSize;                   // Current font size (used for for simplified AddText overload)
    float           FontScale;                  // Current font scale (== FontSize / Font->FontSize)
    float           CurveTessellationTol;       // Tessellation tolerance when using PathBezierCurveTo()
    float           CircleSegmentMaxError;      // Number of circle segments to use per pixel of radius for AddCircle() etc
    float           InitialFringeScale;         // Initial scale to apply to AA fringe
    VanDrawListFlags InitialFlags;               // Initial flags at the beginning of the frame (it is possible to alter flags on a per-drawlist basis afterwards)
    VanVec4          ClipRectFullscreen;         // Value for PushClipRectFullscreen()
    VanVector<VanVec2> TempBuffer;                // Temporary write buffer
    VanVector<VanDrawList*> DrawLists;            // All draw lists associated to this VanDrawListSharedData
    VanGuiContext*   Context;                    // [OPTIONAL] Link to VanGUI context. 99% of VanDrawList/VanFontAtlas can function without an VanGui context, but this facilitate handling one legacy edge case.

    // Lookup tables
    VanVec2          ArcFastVtx[VAN_DRAWLIST_ARCFAST_TABLE_SIZE]; // Sample points on the quarter of the circle.
    float           ArcFastRadiusCutoff;                        // Cutoff radius after which arc drawing will fallback to slower PathArcTo()
    VanU8            CircleSegmentCounts[64];    // Precomputed segment count for given radius before we calculate it dynamically (to avoid calculation overhead)

    VanDrawListSharedData();
    ~VanDrawListSharedData();
    void SetCircleTessellationMaxError(float max_error);
};

struct VanDrawDataBuilder
{
    VanVector<VanDrawList*>*  Layers[2];      // Pointers to global layers for: regular, tooltip. LayersP[0] is owned by DrawData.
    VanVector<VanDrawList*>   LayerData1;

    VanDrawDataBuilder()                     { memset((void*)this, 0, sizeof(*this)); } // VanVector<> members are zero-init-safe (Data=nullptr, Size=0, Capacity=0)
};

struct VanFontStackData
{
    VanFont*     Font;
    float       FontSizeBeforeScaling;      // ~~ style.FontSizeBase
    float       FontSizeAfterScaling;       // ~~ g.FontSize
};

//-----------------------------------------------------------------------------
// [SECTION] Style support
//-----------------------------------------------------------------------------

struct VanGuiStyleVarInfo
{
    VanU32           Count : 8;      // 1+
    VanGuiDataType   DataType : 8;
    VanU32           Offset : 16;    // Offset in parent structure
    void* GetVarPtr(void* parent) const { return static_cast<void*>(static_cast<unsigned char*>(parent) + Offset); }
};

// Stacked color modifier, backup of modified data so we can restore it
struct VanGuiColorMod
{
    VanGuiCol        Col;
    VanVec4          BackupValue;
};

// Stacked style modifier, backup of modified data so we can restore it. Data type inferred from the variable.
struct VanGuiStyleMod
{
    VanGuiStyleVar   VarIdx;
    union           { int BackupInt[2]; float BackupFloat[2]; };
    VanGuiStyleMod(VanGuiStyleVar idx, int v)     { VarIdx = idx; BackupInt[0] = v; }
    VanGuiStyleMod(VanGuiStyleVar idx, float v)   { VarIdx = idx; BackupFloat[0] = v; }
    VanGuiStyleMod(VanGuiStyleVar idx, VanVec2 v)  { VarIdx = idx; BackupFloat[0] = v.x; BackupFloat[1] = v.y; }
};

//-----------------------------------------------------------------------------
// [SECTION] Data types support
//-----------------------------------------------------------------------------

struct VanGuiDataTypeStorage
{
    VanU8        Data[8];        // Opaque storage to fit any data up to VanGuiDataType_COUNT
};

// Type information associated to one VanGuiDataType. Retrieve with DataTypeGetInfo().
struct VanGuiDataTypeInfo
{
    size_t      Size;           // Size in bytes
    const char* Name;           // Short descriptive name for the type, for debugging
    const char* PrintFmt;       // Default printf format for the type
    const char* ScanFmt;        // Default scanf format for the type
};

// Extend VanGuiDataType_
enum VanGuiDataTypePrivate_
{
    VanGuiDataType_Pointer = VanGuiDataType_COUNT,
    VanGuiDataType_ID,
};

//-----------------------------------------------------------------------------
// [SECTION] Widgets support: flags, enums, data structures
//-----------------------------------------------------------------------------

// Extend VanGuiItemFlags
// - input: PushItemFlag() manipulates g.CurrentItemFlags, g.NextItemData.ItemFlags, ItemAdd() calls may add extra flags too.
// - output: stored in g.LastItemData.ItemFlags
enum VanGuiItemFlagsPrivate_
{
    // Controlled by user
    VanGuiItemFlags_ReadOnly                 = 1 << 11, // false     // [ALPHA] Allow hovering interactions but underlying value is not changed.
    VanGuiItemFlags_MixedValue               = 1 << 12, // false     // [BETA] Represent a mixed/indeterminate value, generally multi-selection where values differ. Currently only supported by Checkbox() (later should support all sorts of widgets)
    VanGuiItemFlags_NoWindowHoverableCheck   = 1 << 13, // false     // Disable hoverable check in ItemHoverable()
    VanGuiItemFlags_AllowOverlap             = 1 << 14, // false     // Allow being overlapped by another widget. Not-hovered to Hovered transition deferred by a frame.
    VanGuiItemFlags_NoNavDisableMouseHover   = 1 << 15, // false     // Nav keyboard/gamepad mode doesn't disable hover highlight (behave as if NavHighlightItemUnderNav==false).
    VanGuiItemFlags_NoMarkEdited             = 1 << 16, // false     // Skip calling MarkItemEdited()
    VanGuiItemFlags_NoFocus                  = 1 << 17, // false     // [EXPERIMENTAL: Not very well specced] Clicking doesn't take focus. Automatically sets VanGuiButtonFlags_NoFocus + VanGuiButtonFlags_NoNavFocus in ButtonBehavior().

    // Controlled by widget code
    VanGuiItemFlags_Inputable                = 1 << 20, // false     // [WIP] Auto-activate input mode when tab focused. Currently only used and supported by a few items before it becomes a generic feature.
    VanGuiItemFlags_HasSelectionUserData     = 1 << 21, // false     // Set by SetNextItemSelectionUserData()
    VanGuiItemFlags_IsMultiSelect            = 1 << 22, // false     // Set by SetNextItemSelectionUserData()

    VanGuiItemFlags_Default_                 = VanGuiItemFlags_AutoClosePopups,    // Please don't change, use PushItemFlag() instead.

    // Obsolete
    //VanGuiItemFlags_SelectableDontClosePopup = !VanGuiItemFlags_AutoClosePopups, // Can't have a redirect as we inverted the behavior
};

// Status flags for an already submitted item
// - output: stored in g.LastItemData.StatusFlags
enum VanGuiItemStatusFlags_
{
    VanGuiItemStatusFlags_None               = 0,
    VanGuiItemStatusFlags_HoveredRect        = 1 << 0,   // Mouse position is within item rectangle (does NOT mean that the window is in correct z-order and can be hovered!, this is only one part of the most-common IsItemHovered test)
    VanGuiItemStatusFlags_HasDisplayRect     = 1 << 1,   // g.LastItemData.DisplayRect is valid
    VanGuiItemStatusFlags_Edited             = 1 << 2,   // Value exposed by item was edited in the current frame (should match the bool return value of most widgets)
    VanGuiItemStatusFlags_ToggledSelection   = 1 << 3,   // Set when Selectable(), TreeNode() reports toggling a selection. We can't report "Selected", only state changes, in order to easily handle clipping with less issues.
    VanGuiItemStatusFlags_ToggledOpen        = 1 << 4,   // Set when TreeNode() reports toggling their open state.
    VanGuiItemStatusFlags_HasDeactivated     = 1 << 5,   // Set if the widget/group is able to provide data for the VanGuiItemStatusFlags_Deactivated flag.
    VanGuiItemStatusFlags_Deactivated        = 1 << 6,   // Only valid if VanGuiItemStatusFlags_HasDeactivated is set.
    VanGuiItemStatusFlags_HoveredWindow      = 1 << 7,   // Override the HoveredWindow test to allow cross-window hover testing.
    VanGuiItemStatusFlags_Visible            = 1 << 8,   // [WIP] Set when item is overlapping the current clipping rectangle (Used internally. Please don't use yet: API/system will change as we refactor Itemadd()).
    VanGuiItemStatusFlags_HasClipRect        = 1 << 9,   // g.LastItemData.ClipRect is valid.
    VanGuiItemStatusFlags_HasShortcut        = 1 << 10,  // g.LastItemData.Shortcut valid. Set by SetNextItemShortcut() -> ItemAdd().
    //VanGuiItemStatusFlags_FocusedByTabbing = 1 << 8,   // Removed IN 1.90.1 (Dec 2023). The trigger is part of g.NavActivateId. See commit 54c1bdeceb.
    VanGuiItemStatusFlags_EditedInternal     = 1 << 11,  // Similar to VanGuiItemStatusFlags_Edited but bypassing VanGuiItemFlags_NoMarkEdited.

    // Additional status + semantic for VanGuiTestEngine
#ifdef VANGUI_ENABLE_TEST_ENGINE
    VanGuiItemStatusFlags_Openable           = 1 << 20,  // Item is an openable (e.g. TreeNode)
    VanGuiItemStatusFlags_Opened             = 1 << 21,  // Opened status
    VanGuiItemStatusFlags_Checkable          = 1 << 22,  // Item is a checkable (e.g. CheckBox, MenuItem)
    VanGuiItemStatusFlags_Checked            = 1 << 23,  // Checked status
    VanGuiItemStatusFlags_Inputable          = 1 << 24,  // Item is a text-inputable (e.g. InputText, SliderXXX, DragXXX)
#endif
};

// Extend VanGuiHoveredFlags_
enum VanGuiHoveredFlagsPrivate_
{
    VanGuiHoveredFlags_DelayMask_                    = VanGuiHoveredFlags_DelayNone | VanGuiHoveredFlags_DelayShort | VanGuiHoveredFlags_DelayNormal | VanGuiHoveredFlags_NoSharedDelay,
    VanGuiHoveredFlags_AllowedMaskForIsWindowHovered = VanGuiHoveredFlags_ChildWindows | VanGuiHoveredFlags_RootWindow | VanGuiHoveredFlags_AnyWindow | VanGuiHoveredFlags_NoPopupHierarchy | VanGuiHoveredFlags_AllowWhenBlockedByPopup | VanGuiHoveredFlags_AllowWhenBlockedByActiveItem | VanGuiHoveredFlags_ForTooltip | VanGuiHoveredFlags_Stationary,
    VanGuiHoveredFlags_AllowedMaskForIsItemHovered   = VanGuiHoveredFlags_AllowWhenBlockedByPopup | VanGuiHoveredFlags_AllowWhenBlockedByActiveItem | VanGuiHoveredFlags_AllowWhenOverlapped | VanGuiHoveredFlags_AllowWhenDisabled | VanGuiHoveredFlags_NoNavOverride | VanGuiHoveredFlags_ForTooltip | VanGuiHoveredFlags_Stationary | VanGuiHoveredFlags_DelayMask_,
};

// Extend VanGuiInputTextFlags_
enum VanGuiInputTextFlagsPrivate_
{
    // [Internal]
    VanGuiInputTextFlags_Multiline           = 1 << 26,  // For internal use by InputTextMultiline()
    VanGuiInputTextFlags_TempInput           = 1 << 27,  // For internal use by TempInputText(), will skip calling ItemAdd(). Require bounding-box to strictly match.
    VanGuiInputTextFlags_LocalizeDecimalPoint= 1 << 28,  // For internal use by InputScalar() and TempInputScalar()
};

// Extend VanGuiButtonFlags_
enum VanGuiButtonFlagsPrivate_
{
    VanGuiButtonFlags_PressedOnClick         = 1 << 4,   // return true on click (mouse down event)
    VanGuiButtonFlags_PressedOnClickRelease  = 1 << 5,   // [Default] return true on click + release on same item <-- this is what the majority of Button are using
    VanGuiButtonFlags_PressedOnClickReleaseAnywhere = 1 << 6, // return true on click + release even if the release event is not done while hovering the item
    VanGuiButtonFlags_PressedOnRelease       = 1 << 7,   // return true on release (default requires click+release). Prior to 2026/03/20 this implied VanGuiButtonFlags_NoHoldingActiveId but they are separate now.
    VanGuiButtonFlags_PressedOnDoubleClick   = 1 << 8,   // return true on double-click (default requires click+release)
    VanGuiButtonFlags_PressedOnDragDropHold  = 1 << 9,   // return true when held into while we are drag and dropping another item (used by e.g. tree nodes, collapsing headers)
    //VanGuiButtonFlags_Repeat               = 1 << 10,  // hold to repeat -> use VanGuiItemFlags_ButtonRepeat instead.
    VanGuiButtonFlags_FlattenChildren        = 1 << 11,  // allow interactions even if a child window is overlapping
    //VanGuiButtonFlags_DontClosePopups      = 1 << 13,  // disable automatically closing parent popup on press
    //VanGuiButtonFlags_Disabled             = 1 << 14,  // disable interactions -> use BeginDisabled() or VanGuiItemFlags_Disabled
    VanGuiButtonFlags_AlignTextBaseLine      = 1 << 15,  // vertically align button to match text baseline - ButtonEx() only // FIXME: Should be removed and handled by SmallButton(), not possible currently because of DC.CursorPosPrevLine
    VanGuiButtonFlags_NoKeyModsAllowed       = 1 << 16,  // disable mouse interaction if a key modifier is held
    VanGuiButtonFlags_NoHoldingActiveId      = 1 << 17,  // don't set ActiveId while holding the mouse (VanGuiButtonFlags_PressedOnClick only)
    VanGuiButtonFlags_NoNavFocus             = 1 << 18,  // don't override navigation focus when activated (FIXME: this is essentially used every time an item uses VanGuiItemFlags_NoNav, but because legacy specs don't requires LastItemData to be set ButtonBehavior(), we can't poll g.LastItemData.ItemFlags)
    VanGuiButtonFlags_NoHoveredOnFocus       = 1 << 19,  // don't report as hovered when nav focus is on this item
    VanGuiButtonFlags_NoSetKeyOwner          = 1 << 20,  // don't set key/input owner on the initial click (note: mouse buttons are keys! often, the key in question will be VanGuiKey_MouseLeft!)
    VanGuiButtonFlags_NoTestKeyOwner         = 1 << 21,  // don't test key/input owner when polling the key (note: mouse buttons are keys! often, the key in question will be VanGuiKey_MouseLeft!)
    VanGuiButtonFlags_NoFocus                = 1 << 22,  // [EXPERIMENTAL: Not very well specced]. Don't focus parent window when clicking.
    VanGuiButtonFlags_PressedOnMask_         = VanGuiButtonFlags_PressedOnClick | VanGuiButtonFlags_PressedOnClickRelease | VanGuiButtonFlags_PressedOnClickReleaseAnywhere | VanGuiButtonFlags_PressedOnRelease | VanGuiButtonFlags_PressedOnDoubleClick | VanGuiButtonFlags_PressedOnDragDropHold,
    VanGuiButtonFlags_PressedOnDefault_      = VanGuiButtonFlags_PressedOnClickRelease,
    //VanGuiButtonFlags_NoKeyModifiers       = VanGuiButtonFlags_NoKeyModsAllowed, // Renamed in 1.91.4
};

// Extend VanGuiComboFlags_
enum VanGuiComboFlagsPrivate_
{
    VanGuiComboFlags_CustomPreview           = 1 << 20,  // enable BeginComboPreview()
};

// Extend VanGuiSliderFlags_
enum VanGuiSliderFlagsPrivate_
{
    VanGuiSliderFlags_Vertical               = 1 << 20,  // Should this slider be orientated vertically?
    VanGuiSliderFlags_ReadOnly               = 1 << 21,  // Consider using g.NextItemData.ItemFlags |= VanGuiItemFlags_ReadOnly instead.
};

// Extend VanGuiSelectableFlags_
enum VanGuiSelectableFlagsPrivate_
{
    // NB: need to be in sync with last value of VanGuiSelectableFlags_
    VanGuiSelectableFlags_NoHoldingActiveID      = 1 << 20,
    VanGuiSelectableFlags_SelectOnClick          = 1 << 22,  // Override button behavior to react on Click (default is Click+Release)
    VanGuiSelectableFlags_SelectOnRelease        = 1 << 23,  // Override button behavior to react on Release (default is Click+Release)
    VanGuiSelectableFlags_SpanAvailWidth         = 1 << 24,  // Span all avail width even if we declared less for layout purpose. FIXME: We may be able to remove this (added in 6251d379, 2bcafc86 for menus)
    VanGuiSelectableFlags_SetNavIdOnHover        = 1 << 25,  // Set Nav/Focus ID on mouse hover (used by MenuItem)
    VanGuiSelectableFlags_NoPadWithHalfSpacing   = 1 << 26,  // Disable padding each side with ItemSpacing * 0.5f
    VanGuiSelectableFlags_NoSetKeyOwner          = 1 << 27,  // Don't set key/input owner on the initial click (note: mouse buttons are keys! often, the key in question will be VanGuiKey_MouseLeft!)
};

// Extend VanGuiTreeNodeFlags_
enum VanGuiTreeNodeFlagsPrivate_
{
    VanGuiTreeNodeFlags_NoNavFocus                 = 1 << 27,// Don't claim nav focus when interacting with this item (#8551)
    VanGuiTreeNodeFlags_ClipLabelForTrailingButton = 1 << 28,// FIXME-WIP: Hard-coded for CollapsingHeader()
    VanGuiTreeNodeFlags_UpsideDownArrow            = 1 << 29,// FIXME-WIP: Turn Down arrow into an Up arrow, for reversed trees (#6517)
    VanGuiTreeNodeFlags_OpenOnMask_                = VanGuiTreeNodeFlags_OpenOnDoubleClick | VanGuiTreeNodeFlags_OpenOnArrow,
    VanGuiTreeNodeFlags_DrawLinesMask_             = VanGuiTreeNodeFlags_DrawLinesNone | VanGuiTreeNodeFlags_DrawLinesFull | VanGuiTreeNodeFlags_DrawLinesToNodes,
};

enum VanGuiSeparatorFlags_
{
    VanGuiSeparatorFlags_None                    = 0,
    VanGuiSeparatorFlags_Horizontal              = 1 << 0,   // Axis default to current layout type, so generally Horizontal unless e.g. in a menu bar
    VanGuiSeparatorFlags_Vertical                = 1 << 1,
    VanGuiSeparatorFlags_SpanAllColumns          = 1 << 2,   // Make separator cover all columns of a legacy Columns() set.
};

// Flags for FocusWindow(). This is not called VanGuiFocusFlags to avoid confusion with public-facing VanGuiFocusedFlags.
// FIXME: Once we finishing replacing more uses of GetTopMostPopupModal()+IsWindowWithinBeginStackOf()
// and FindBlockingModal() with this, we may want to change the flag to be opt-out instead of opt-in.
enum VanGuiFocusRequestFlags_
{
    VanGuiFocusRequestFlags_None                 = 0,
    VanGuiFocusRequestFlags_RestoreFocusedChild  = 1 << 0,   // Find last focused child (if any) and focus it instead.
    VanGuiFocusRequestFlags_UnlessBelowModal     = 1 << 1,   // Do not set focus if the window is below a modal.
};

enum VanGuiTextFlags_
{
    VanGuiTextFlags_None                         = 0,
    VanGuiTextFlags_NoWidthForLargeClippedText   = 1 << 0,
};

enum VanGuiTooltipFlags_
{
    VanGuiTooltipFlags_None                      = 0,
    VanGuiTooltipFlags_OverridePrevious          = 1 << 1,   // Clear/ignore previously submitted tooltip (defaults to append)
};

// FIXME: this is in development, not exposed/functional as a generic feature yet.
// Horizontal/Vertical enums are fixed to 0/1 so they may be used to index VanVec2
enum VanGuiLayoutType_
{
    VanGuiLayoutType_Horizontal = 0,
    VanGuiLayoutType_Vertical = 1
};

// Flags for LogBegin() text capturing function
enum VanGuiLogFlags_
{
    VanGuiLogFlags_None = 0,

    VanGuiLogFlags_OutputTTY         = 1 << 0,
    VanGuiLogFlags_OutputFile        = 1 << 1,
    VanGuiLogFlags_OutputBuffer      = 1 << 2,
    VanGuiLogFlags_OutputClipboard   = 1 << 3,
    VanGuiLogFlags_OutputMask_       = VanGuiLogFlags_OutputTTY | VanGuiLogFlags_OutputFile | VanGuiLogFlags_OutputBuffer | VanGuiLogFlags_OutputClipboard,
};

// X/Y enums are fixed to 0/1 so they may be used to index VanVec2
enum VanGuiAxis
{
    VanGuiAxis_None = -1,
    VanGuiAxis_X = 0,
    VanGuiAxis_Y = 1
};

enum VanGuiPlotType
{
    VanGuiPlotType_Lines,
    VanGuiPlotType_Histogram,
};

// Storage data for BeginComboPreview()/EndComboPreview()
struct VANGUI_API VanGuiComboPreviewData
{
    VanRect          PreviewRect;
    VanVec2          BackupCursorPos;
    VanVec2          BackupCursorMaxPos;
    VanVec2          BackupCursorPosPrevLine;
    float           BackupPrevLineTextBaseOffset;
    VanGuiLayoutType BackupLayout;

    VanGuiComboPreviewData() { memset((void*)this, 0, sizeof(*this)); } // POD-safe zero-init
};

// Stacked storage data for BeginGroup()/EndGroup()
struct VANGUI_API VanGuiGroupData
{
    VanGuiID     WindowID;
    VanVec2      BackupCursorPos;
    VanVec2      BackupCursorMaxPos;
    VanVec2      BackupCursorPosPrevLine;
    VanVec1      BackupIndent;
    VanVec1      BackupGroupOffset;
    VanVec2      BackupCurrLineSize;
    float       BackupCurrLineTextBaseOffset;
    VanGuiID     BackupActiveIdIsAlive;
    bool        BackupActiveIdHasBeenEditedThisFrame;
    bool        BackupDeactivatedIdIsAlive;
    bool        BackupHoveredIdIsAlive;
    bool        BackupIsSameLine;
    bool        EmitItem;
};

// Simple column measurement, currently used for MenuItem() only.. This is very short-sighted/throw-away code and NOT a generic helper.
struct VANGUI_API VanGuiMenuColumns
{
    VanU32       TotalWidth;
    VanU32       NextTotalWidth;
    VanU16       Spacing;
    VanU16       OffsetIcon;         // Always zero for now
    VanU16       OffsetLabel;        // Offsets are locked in Update()
    VanU16       OffsetShortcut;
    VanU16       OffsetMark;
    VanU16       Widths[4];          // Width of:   Icon, Label, Shortcut, Mark  (accumulators for current frame)

    VanGuiMenuColumns() { memset((void*)this, 0, sizeof(*this)); } // POD-safe zero-init
    void        Update(float spacing, bool window_reappearing);
    float       DeclColumns(float w_icon, float w_label, float w_shortcut, float w_mark);
    void        CalcNextTotalWidth(bool update_offsets);
};

// Internal temporary state for deactivating InputText() instances.
// Store as part of VanGuiDeactivatedItemData?
struct VANGUI_API VanGuiInputTextDeactivatedState
{
    VanGuiID            ID;              // widget id owning the text state (which just got deactivated)
    VanVector<char>     TextA;           // text buffer

    VanGuiInputTextDeactivatedState()    { memset((void*)this, 0, sizeof(*this)); } // VanVector<char> member is zero-init-safe (Data=nullptr, Size=0, Capacity=0)
    void    ClearFreeMemory()           { ID = 0; TextA.clear(); }
};

// Forward declare vanstb_textedit.h structure + make its main configuration define accessible
#undef VANSTB_TEXTEDIT_STRING
#undef VANSTB_TEXTEDIT_CHARTYPE
#define VANSTB_TEXTEDIT_STRING             VanGuiInputTextState
#define VANSTB_TEXTEDIT_CHARTYPE           char
#define VANSTB_TEXTEDIT_GETWIDTH_NEWLINE   (-1.0f)
#define VANSTB_TEXTEDIT_UNDOSTATECOUNT     99
#define VANSTB_TEXTEDIT_UNDOCHARCOUNT      999
namespace VanStb { struct STB_TexteditState; }
using VanStbTexteditState = VanStb::STB_TexteditState;

// Internal state of the currently focused/edited text input box
// For a given item ID, access with VanGui::GetInputTextState()
struct VANGUI_API VanGuiInputTextState
{
    VanGuiContext*           Ctx;                    // parent UI context (needs to be set explicitly by parent).
    VanStbTexteditState*     Stb;                    // State for stb_textedit.h
    VanGuiInputTextFlags     Flags;                  // copy of InputText() flags. may be used to check if e.g. VanGuiInputTextFlags_Password is set.
    VanGuiID                 ID;                     // widget id owning the text state
    int                     TextLen;                // UTF-8 length of the string in TextA (in bytes)
    const char*             TextSrc;                // == TextA.Data unless read-only, in which case == buf passed to InputText(). For _ReadOnly fields, pointer will be null outside the InputText() call.
    VanVector<char>          TextA;                  // main UTF8 buffer. TextA.Size is a buffer size! Should always be >= buf_size passed by user (and of course >= CurLenA + 1).
    VanVector<char>          TextToRevertTo;         // value to revert to when pressing Escape = backup of end-user buffer at the time of focus (in UTF-8, unaltered)
    VanVector<char>          CallbackTextBackup;     // temporary storage for callback to support automatic reconcile of undo-stack
    int                     BufCapacity;            // end-user buffer capacity (include zero terminator)
    VanVec2                  Scroll;                 // horizontal offset (managed manually) + vertical scrolling (pulled from child window's own Scroll.y)
    int                     LineCount;              // last line count (solely for debugging)
    float                   WrapWidth;              // word-wrapping width
    float                   CursorAnim;             // timer for cursor blink, reset on every user action so the cursor reappears immediately
    bool                    CursorFollow;           // set when we want scrolling to follow the current cursor position (not always!)
    bool                    CursorCenterY;          // set when we want scrolling to be centered over the cursor position (while resizing a word-wrapping field)
    bool                    SelectedAllMouseLock;   // after a double-click to select all, we ignore further mouse drags to update selection
    bool                    EditedBefore;           // edited since activated
    bool                    EditedThisFrame;        // edited this frame
    bool                    WantReloadUserBuf;      // force a reload of user buf so it may be modified externally. may be automatic in future version.
    VanS8                    LastMoveDirectionLR;    // VanGuiDir_Left or VanGuiDir_Right. track last movement direction so when cursor cross over a word-wrapping boundaries we can display it on either line depending on last move.s
    int                     ReloadSelectionStart;
    int                     ReloadSelectionEnd;

    VanGuiInputTextState();
    ~VanGuiInputTextState();
    void        ClearText()                 { TextLen = 0; TextA[0] = 0; CursorClamp(); }
    void        ClearFreeMemory()           { TextA.clear(); TextToRevertTo.clear(); }
    void        OnKeyPressed(int key);      // Cannot be inline because we call in code in stb_textedit.h implementation
    void        OnCharPressed(unsigned int c);
    float       GetPreferredOffsetX() const;
    const char* GetText()                   { return TextA.Data ? TextA.Data : ""; }

    // Cursor & Selection
    void        CursorAnimReset();
    void        CursorClamp();
    bool        HasSelection() const;
    void        ClearSelection();
    int         GetCursorPos() const;
    int         GetSelectionStart() const;
    int         GetSelectionEnd() const;
    void        SetSelection(int start, int end);
    void        SelectAll();

    // Reload user buf (WIP #2890)
    // If you modify underlying user-passed const char* while active you need to call this (InputText V2 may lift this)
    //   strcpy(my_buf, "hello");
    //   if (VanGuiInputTextState* state = VanGui::GetInputTextState(id)) // id may be VanGui::GetItemID() is last item
    //       state->ReloadUserBufAndSelectAll();
    void        ReloadUserBufAndSelectAll();
    void        ReloadUserBufAndKeepSelection();
    void        ReloadUserBufAndMoveToEnd();
};

enum VanGuiWindowRefreshFlags_
{
    VanGuiWindowRefreshFlags_None                = 0,
    VanGuiWindowRefreshFlags_TryToAvoidRefresh   = 1 << 0,   // [EXPERIMENTAL] Try to keep existing contents, USER MUST NOT HONOR BEGIN() RETURNING FALSE AND NOT APPEND.
    VanGuiWindowRefreshFlags_RefreshOnHover      = 1 << 1,   // [EXPERIMENTAL] Always refresh on hover
    VanGuiWindowRefreshFlags_RefreshOnFocus      = 1 << 2,   // [EXPERIMENTAL] Always refresh on focus
    // Refresh policy/frequency, Load Balancing etc.
};

enum VanGuiWindowBgClickFlags_
{
    VanGuiWindowBgClickFlags_None                = 0,
    VanGuiWindowBgClickFlags_Move                = 1 << 0,   // Click on bg/void + drag to move window. Cleared by default when using io.ConfigWindowsMoveFromTitleBarOnly.
};

enum VanGuiNextWindowDataFlags_
{
    VanGuiNextWindowDataFlags_None               = 0,
    VanGuiNextWindowDataFlags_HasPos             = 1 << 0,
    VanGuiNextWindowDataFlags_HasSize            = 1 << 1,
    VanGuiNextWindowDataFlags_HasContentSize     = 1 << 2,
    VanGuiNextWindowDataFlags_HasCollapsed       = 1 << 3,
    VanGuiNextWindowDataFlags_HasSizeConstraint  = 1 << 4,
    VanGuiNextWindowDataFlags_HasFocus           = 1 << 5,
    VanGuiNextWindowDataFlags_HasBgAlpha         = 1 << 6,
    VanGuiNextWindowDataFlags_HasScroll          = 1 << 7,
    VanGuiNextWindowDataFlags_HasWindowFlags     = 1 << 8,
    VanGuiNextWindowDataFlags_HasChildFlags      = 1 << 9,
    VanGuiNextWindowDataFlags_HasDock           = 1 << 10,
    VanGuiNextWindowDataFlags_HasWindowClass    = 1 << 11,
    VanGuiNextWindowDataFlags_HasRefreshPolicy   = 1 << 12,
    VanGuiNextWindowDataFlags_HasViewport        = 1 << 13,
};

// Storage for SetNexWindow** functions
struct VanGuiNextWindowData
{
    VanGuiNextWindowDataFlags    HasFlags;

    // Members below are NOT cleared. Always rely on HasFlags.
    VanGuiCond                   PosCond;
    VanGuiCond                   SizeCond;
    VanGuiCond                   CollapsedCond;
    VanVec2                      PosVal;
    VanVec2                      PosPivotVal;
    VanVec2                      SizeVal;
    VanVec2                      ContentSizeVal;
    VanVec2                      ScrollVal;
    VanGuiWindowFlags            WindowFlags;            // Only honored by BeginTable()
    VanGuiChildFlags             ChildFlags;
    bool                        CollapsedVal;
    VanRect                      SizeConstraintRect;
    VanGuiSizeCallback           SizeCallback;
    void*                       SizeCallbackUserData;
    float                       BgAlphaVal;             // Override background alpha
    VanVec2                      MenuBarOffsetMinVal;    // (Always on) This is not exposed publicly, so we don't clear it and it doesn't have a corresponding flag (could we? for consistency?)
    VanGuiWindowRefreshFlags     RefreshFlagsVal;

    // Docking
    VanGuiCond          DockCond            = 0;
    VanGuiID            DockId              = 0;
    VanGuiWindowClass   WindowClass         = {};

    // Viewport
    VanGuiID            ViewportId          = 0;
    VanGuiCond          ViewportCond        = 0;

    VanGuiNextWindowData()       { memset((void*)this, 0, sizeof(*this)); } // POD-safe zero-init
    inline void ClearFlags()    { HasFlags = VanGuiNextWindowDataFlags_None; }
};

enum VanGuiNextItemDataFlags_
{
    VanGuiNextItemDataFlags_None             = 0,
    VanGuiNextItemDataFlags_HasWidth         = 1 << 0,
    VanGuiNextItemDataFlags_HasOpen          = 1 << 1,
    VanGuiNextItemDataFlags_HasShortcut      = 1 << 2,
    VanGuiNextItemDataFlags_HasRefVal        = 1 << 3,
    VanGuiNextItemDataFlags_HasStorageID     = 1 << 4,
    VanGuiNextItemDataFlags_HasColorMarker   = 1 << 5,
};

struct VanGuiNextItemData
{
    VanGuiNextItemDataFlags      HasFlags;           // Called HasFlags instead of Flags to avoid mistaking this
    VanGuiItemFlags              ItemFlags;          // Currently only tested/used for VanGuiItemFlags_AllowOverlap and VanGuiItemFlags_HasSelectionUserData.

    // Members below are NOT cleared by ItemAdd() meaning they are still valid during e.g. NavProcessItem(). Always rely on HasFlags.
    VanGuiID                     FocusScopeId;       // Set by SetNextItemSelectionUserData()
    VanGuiSelectionUserData      SelectionUserData;  // Set by SetNextItemSelectionUserData() (note that nullptr/0 is a valid value, we use -1 == VanGuiSelectionUserData_Invalid to mark invalid values)
    float                       Width;              // Set by SetNextItemWidth()
    VanGuiKeyChord               Shortcut;           // Set by SetNextItemShortcut()
    VanGuiInputFlags             ShortcutFlags;      // Set by SetNextItemShortcut()
    bool                        OpenVal;            // Set by SetNextItemOpen()
    VanU8                        OpenCond;           // Set by SetNextItemOpen()
    VanGuiDataTypeStorage        RefVal;             // Not exposed yet, for VanGuiInputTextFlags_ParseEmptyAsRefVal
    VanGuiID                     StorageId;          // Set by SetNextItemStorageID()
    VanU32                       ColorMarker;        // Set by SetNextItemColorMarker(). Not exposed yet, supported by DragScalar,SliderScalar and for VanGuiSliderFlags_ColorMarkers.

    VanGuiNextItemData()         { memset((void*)this, 0, sizeof(*this)); SelectionUserData = -1; } // POD-safe zero-init
    inline void ClearFlags()    { HasFlags = VanGuiNextItemDataFlags_None; ItemFlags = VanGuiItemFlags_None; } // Also cleared manually by ItemAdd()!
};

// Status storage for the last submitted item
struct VanGuiLastItemData
{
    VanGuiID                 ID;
    VanGuiItemFlags          ItemFlags;          // See VanGuiItemFlags_ (called 'InFlags' before v1.91.4).
    VanGuiItemStatusFlags    StatusFlags;        // See VanGuiItemStatusFlags_
    VanRect                  Rect;               // Full rectangle
    VanRect                  NavRect;            // Navigation scoring rectangle (not displayed)
    // Rarely used fields are not explicitly cleared, only valid when the corresponding VanGuiItemStatusFlags are set.
    VanRect                  DisplayRect;        // Display rectangle. ONLY VALID IF (StatusFlags & VanGuiItemStatusFlags_HasDisplayRect) is set.
    VanRect                  ClipRect;           // Clip rectangle at the time of submitting item. ONLY VALID IF (StatusFlags & VanGuiItemStatusFlags_HasClipRect) is set..
    VanGuiKeyChord           Shortcut;           // Shortcut at the time of submitting item. ONLY VALID IF (StatusFlags & VanGuiItemStatusFlags_HasShortcut) is set..

    VanGuiLastItemData()     { memset((void*)this, 0, sizeof(*this)); } // POD-safe zero-init
};

// Store data emitted by TreeNode() for usage by TreePop()
// - To implement VanGuiTreeNodeFlags_NavLeftJumpsToParent: store the minimum amount of data
//   which we can't infer in TreePop(), to perform the equivalent of NavApplyItemToResult().
//   Only stored when the node is a potential candidate for landing on a Left arrow jump.
struct VanGuiTreeNodeStackData
{
    VanGuiID                 ID;
    VanGuiTreeNodeFlags      TreeFlags;
    VanGuiItemFlags          ItemFlags;      // Used for nav landing
    VanRect                  NavRect;        // Used for nav landing
    float                   DrawLinesX1;
    float                   DrawLinesToNodesY2;
    VanGuiTableColumnIdx     DrawLinesTableColumn;
};

// sizeof() = 20
struct VANGUI_API VanGuiErrorRecoveryState
{
    short   SizeOfWindowStack;
    short   SizeOfIDStack;
    short   SizeOfTreeStack;
    short   SizeOfColorStack;
    short   SizeOfStyleVarStack;
    short   SizeOfFontStack;
    short   SizeOfFocusScopeStack;
    short   SizeOfGroupStack;
    short   SizeOfItemFlagsStack;
    short   SizeOfBeginPopupStack;
    short   SizeOfDisabledStack;

    VanGuiErrorRecoveryState() { memset((void*)this, 0, sizeof(*this)); } // POD-safe zero-init
};

// Data saved for each window pushed into the stack
struct VanGuiWindowStackData
{
    VanGuiWindow*            Window;
    VanGuiLastItemData       ParentLastItemDataBackup;
    VanGuiErrorRecoveryState StackSizesInBegin;          // Store size of various stacks for asserting
    bool                    DisabledOverrideReenable;   // Non-child window override disabled flag
    float                   DisabledOverrideReenableAlphaBackup;
};

struct VanGuiShrinkWidthItem
{
    int         Index;
    float       Width;
    float       InitialWidth;
};

struct VanGuiPtrOrIndex
{
    void*       Ptr;            // Either field can be set, not both. e.g. Dock node tab bars are loose while BeginTabBar() ones are in a pool.
    int         Index;          // Usually index in a main pool.

    VanGuiPtrOrIndex(void* ptr)  { Ptr = ptr; Index = -1; }
    VanGuiPtrOrIndex(int index)  { Ptr = nullptr; Index = index; }
};

// Data used by IsItemDeactivated()/IsItemDeactivatedAfterEdit() functions
// Also see VanGuiInputTextDeactivatedState which is an extension for this for InputText()
struct VanGuiDeactivatedItemData
{
    VanGuiID     ID;
    int         ElapseFrame;
    bool        HasBeenEditedBefore;
    bool        IsAlive;
};

//-----------------------------------------------------------------------------
// [SECTION] Popup support
//-----------------------------------------------------------------------------

enum VanGuiPopupPositionPolicy
{
    VanGuiPopupPositionPolicy_Default,
    VanGuiPopupPositionPolicy_ComboBox,
    VanGuiPopupPositionPolicy_Tooltip,
};

// Storage for popup stacks (g.OpenPopupStack and g.BeginPopupStack)
struct VanGuiPopupData
{
    VanGuiID             PopupId;        // Set on OpenPopup()
    VanGuiWindow*        Window;         // Resolved on BeginPopup() - may stay unresolved if user never calls OpenPopup()
    VanGuiWindow*        RestoreNavWindow;// Set on OpenPopup(), a NavWindow that will be restored on popup close
    int                 ParentNavLayer; // Resolved on BeginPopup(). Actually a VanGuiNavLayer type (declared down below), initialized to -1 which is not part of an enum, but serves well-enough as "not any of layers" value
    int                 OpenFrameCount; // Set on OpenPopup()
    VanGuiID             OpenParentId;   // Set on OpenPopup(), we need this to differentiate multiple menu sets from each others (e.g. inside menu bar vs loose menu items)
    VanVec2              OpenPopupPos;   // Set on OpenPopup(), preferred popup position (typically == OpenMousePos when using mouse)
    VanVec2              OpenMousePos;   // Set on OpenPopup(), copy of mouse position at the time of opening popup

    VanGuiPopupData()    { memset((void*)this, 0, sizeof(*this)); ParentNavLayer = OpenFrameCount = -1; } // POD-safe zero-init
};

//-----------------------------------------------------------------------------
// [SECTION] Inputs support
//-----------------------------------------------------------------------------

// Bit array for named keys
using VanBitArrayForNamedKeys = VanBitArray<VanGuiKey_NamedKey_COUNT, -VanGuiKey_NamedKey_BEGIN>;

// [Internal] Key ranges
#define VanGuiKey_LegacyNativeKey_BEGIN  0
#define VanGuiKey_LegacyNativeKey_END    512
#define VanGuiKey_Keyboard_BEGIN         (VanGuiKey_NamedKey_BEGIN)
#define VanGuiKey_Keyboard_END           (VanGuiKey_GamepadStart)
#define VanGuiKey_Gamepad_BEGIN          (VanGuiKey_GamepadStart)
#define VanGuiKey_Gamepad_END            (VanGuiKey_GamepadRStickDown + 1)
#define VanGuiKey_Mouse_BEGIN            (VanGuiKey_MouseLeft)
#define VanGuiKey_Mouse_END              (VanGuiKey_MouseWheelY + 1)
#define VanGuiKey_Aliases_BEGIN          (VanGuiKey_Mouse_BEGIN)
#define VanGuiKey_Aliases_END            (VanGuiKey_Mouse_END)

// [Internal] Named shortcuts for Navigation
#define VanGuiKey_NavKeyboardTweakSlow   VanGuiMod_Ctrl
#define VanGuiKey_NavKeyboardTweakFast   VanGuiMod_Shift
#define VanGuiKey_NavGamepadTweakSlow    VanGuiKey_GamepadL1
#define VanGuiKey_NavGamepadTweakFast    VanGuiKey_GamepadR1
#define VanGuiKey_NavGamepadActivate     (g.IO.ConfigNavSwapGamepadButtons ? VanGuiKey_GamepadFaceRight : VanGuiKey_GamepadFaceDown)
#define VanGuiKey_NavGamepadCancel       (g.IO.ConfigNavSwapGamepadButtons ? VanGuiKey_GamepadFaceDown : VanGuiKey_GamepadFaceRight)
#define VanGuiKey_NavGamepadMenu         VanGuiKey_GamepadFaceLeft    // Toggle menu layer. Hold to enable Windowing.
#define VanGuiKey_NavGamepadContextMenu  VanGuiKey_GamepadFaceUp      // Open context menu (same as Shift+F10)

enum VanGuiInputEventType
{
    VanGuiInputEventType_None = 0,
    VanGuiInputEventType_MousePos,
    VanGuiInputEventType_MouseWheel,
    VanGuiInputEventType_MouseButton,
    VanGuiInputEventType_Key,
    VanGuiInputEventType_Text,
    VanGuiInputEventType_Focus,
    VanGuiInputEventType_COUNT
};

enum VanGuiInputSource : int
{
    VanGuiInputSource_None = 0,
    VanGuiInputSource_Mouse,         // Note: may be Mouse or TouchScreen or Pen. See io.MouseSource to distinguish them.
    VanGuiInputSource_Keyboard,
    VanGuiInputSource_Gamepad,
    VanGuiInputSource_COUNT
};

// FIXME: Structures in the union below need to be declared as anonymous unions appears to be an extension?
// Using VanVec2() would fail on Clang 'union member 'MousePos' has a non-trivial default constructor'
struct VanGuiInputEventMousePos      { float PosX, PosY; VanGuiMouseSource MouseSource; };
struct VanGuiInputEventMouseWheel    { float WheelX, WheelY; VanGuiMouseSource MouseSource; };
struct VanGuiInputEventMouseButton   { int Button; bool Down; VanGuiMouseSource MouseSource; };
struct VanGuiInputEventKey           { VanGuiKey Key; bool Down; float AnalogValue; };
struct VanGuiInputEventText          { unsigned int Char; };
struct VanGuiInputEventAppFocused    { bool Focused; };

struct VanGuiInputEvent
{
    VanGuiInputEventType             Type;
    VanGuiInputSource                Source;
    VanU32                           EventId;        // Unique, sequential increasing integer to identify an event (if you need to correlate them to other data).
    union
    {
        VanGuiInputEventMousePos     MousePos;       // if Type == VanGuiInputEventType_MousePos
        VanGuiInputEventMouseWheel   MouseWheel;     // if Type == VanGuiInputEventType_MouseWheel
        VanGuiInputEventMouseButton  MouseButton;    // if Type == VanGuiInputEventType_MouseButton
        VanGuiInputEventKey          Key;            // if Type == VanGuiInputEventType_Key
        VanGuiInputEventText         Text;           // if Type == VanGuiInputEventType_Text
        VanGuiInputEventAppFocused   AppFocused;     // if Type == VanGuiInputEventType_Focus
    };
    bool                            AddedByTestEngine;

    VanGuiInputEvent() { memset((void*)this, 0, sizeof(*this)); } // POD-safe zero-init
};

// Input function taking an 'VanGuiID owner_id' argument defaults to (VanGuiKeyOwner_Any == 0) aka don't test ownership, which matches legacy behavior.
#define VanGuiKeyOwner_Any           ((VanGuiID)0)    // Accept key that have an owner, UNLESS a call to SetKeyOwner() explicitly used VanGuiInputFlags_LockThisFrame or VanGuiInputFlags_LockUntilRelease.
#define VanGuiKeyOwner_NoOwner       ((VanGuiID)-1)   // Require key to have no owner.
//#define VanGuiKeyOwner_None VanGuiKeyOwner_NoOwner  // We previously called this 'VanGuiKeyOwner_None' but it was inconsistent with our pattern that _None values == 0 and quite dangerous. Also using _NoOwner makes the IsKeyPressed() calls more explicit.

using VanGuiKeyRoutingIndex = VanS16;

// Routing table entry (sizeof() == 16 bytes)
struct VanGuiKeyRoutingData
{
    VanGuiKeyRoutingIndex            NextEntryIndex;
    VanU16                           Mods;               // Technically we'd only need 4-bits but for simplify we store VanGuiMod_ values which need 16-bits.
    VanU16                           RoutingCurrScore;   // [DEBUG] For debug display
    VanU16                           RoutingNextScore;   // Lower is better (0: perfect score)
    VanGuiID                         RoutingCurr;
    VanGuiID                         RoutingNext;

    VanGuiKeyRoutingData()           { NextEntryIndex = -1; Mods = 0; RoutingCurrScore = RoutingNextScore = 0; RoutingCurr = RoutingNext = VanGuiKeyOwner_NoOwner; }
};

// Routing table: maintain a desired owner for each possible key-chord (key + mods), and setup owner in NewFrame() when mods are matching.
// Stored in main context (1 instance)
struct VanGuiKeyRoutingTable
{
    VanGuiKeyRoutingIndex            Index[VanGuiKey_NamedKey_COUNT]; // Index of first entry in Entries[]
    VanVector<VanGuiKeyRoutingData>   Entries;
    VanVector<VanGuiKeyRoutingData>   EntriesNext;                    // Double-buffer to avoid reallocation (could use a shared buffer)

    VanGuiKeyRoutingTable()          { Clear(); }
    void Clear()                    { for (int n = 0; n < VAN_COUNTOF(Index); n++) Index[n] = -1; Entries.clear(); EntriesNext.clear(); }
};

// This extends VanGuiKeyData but only for named keys (legacy keys don't support the new features)
// Stored in main context (1 per named key). In the future it might be merged into VanGuiKeyData.
struct VanGuiKeyOwnerData
{
    VanGuiID     OwnerCurr;
    VanGuiID     OwnerNext;
    bool        LockThisFrame;      // Reading this key requires explicit owner id (until end of frame). Set by VanGuiInputFlags_LockThisFrame.
    bool        LockUntilRelease;   // Reading this key requires explicit owner id (until key is released). Set by VanGuiInputFlags_LockUntilRelease. When this is true LockThisFrame is always true as well.

    VanGuiKeyOwnerData()             { OwnerCurr = OwnerNext = VanGuiKeyOwner_NoOwner; LockThisFrame = LockUntilRelease = false; }
};

// Extend VanGuiInputFlags_
// Flags for extended versions of IsKeyPressed(), IsMouseClicked(), Shortcut(), SetKeyOwner(), SetItemKeyOwner()
// Don't mistake with VanGuiInputTextFlags! (which is for VanGui::InputText() function)
enum VanGuiInputFlagsPrivate_
{
    // Flags for IsKeyPressed(), IsKeyChordPressed(), IsMouseClicked(), Shortcut()
    // - Repeat mode: Repeat rate selection
    VanGuiInputFlags_RepeatRateDefault           = 1 << 1,   // Repeat rate: Regular (default)
    VanGuiInputFlags_RepeatRateNavMove           = 1 << 2,   // Repeat rate: Fast
    VanGuiInputFlags_RepeatRateNavTweak          = 1 << 3,   // Repeat rate: Faster
    // - Repeat mode: Specify when repeating key pressed can be interrupted.
    // - In theory VanGuiInputFlags_RepeatUntilOtherKeyPress may be a desirable default, but it would break too many behavior so everything is opt-in.
    VanGuiInputFlags_RepeatUntilRelease          = 1 << 4,   // Stop repeating when released (default for all functions except Shortcut). This only exists to allow overriding Shortcut() default behavior.
    VanGuiInputFlags_RepeatUntilKeyModsChange    = 1 << 5,   // Stop repeating when released OR if keyboard mods are changed (default for Shortcut)
    VanGuiInputFlags_RepeatUntilKeyModsChangeFromNone = 1 << 6,  // Stop repeating when released OR if keyboard mods are leaving the None state. Allows going from Mod+Key to Key by releasing Mod.
    VanGuiInputFlags_RepeatUntilOtherKeyPress    = 1 << 7,   // Stop repeating when released OR if any other keyboard key is pressed during the repeat

    // Flags for SetKeyOwner(), SetItemKeyOwner()
    // - Locking key away from non-input aware code. Locking is useful to make input-owner-aware code steal keys from non-input-owner-aware code. If all code is input-owner-aware locking would never be necessary.
    VanGuiInputFlags_LockThisFrame               = 1 << 20,  // Further accesses to key data will require EXPLICIT owner ID (VanGuiKeyOwner_Any/0 will NOT accepted for polling). Cleared at end of frame.
    VanGuiInputFlags_LockUntilRelease            = 1 << 21,  // Further accesses to key data will require EXPLICIT owner ID (VanGuiKeyOwner_Any/0 will NOT accepted for polling). Cleared when the key is released or at end of each frame if key is released.

    // - Condition for SetItemKeyOwner()
    VanGuiInputFlags_CondHovered                 = 1 << 22,  // Only set if item is hovered (default to both)
    VanGuiInputFlags_CondActive                  = 1 << 23,  // Only set if item is active (default to both)
    VanGuiInputFlags_CondDefault_                = VanGuiInputFlags_CondHovered | VanGuiInputFlags_CondActive,

    // [Internal] Mask of which function support which flags
    VanGuiInputFlags_RepeatRateMask_             = VanGuiInputFlags_RepeatRateDefault | VanGuiInputFlags_RepeatRateNavMove | VanGuiInputFlags_RepeatRateNavTweak,
    VanGuiInputFlags_RepeatUntilMask_            = VanGuiInputFlags_RepeatUntilRelease | VanGuiInputFlags_RepeatUntilKeyModsChange | VanGuiInputFlags_RepeatUntilKeyModsChangeFromNone | VanGuiInputFlags_RepeatUntilOtherKeyPress,
    VanGuiInputFlags_RepeatMask_                 = VanGuiInputFlags_Repeat | VanGuiInputFlags_RepeatRateMask_ | VanGuiInputFlags_RepeatUntilMask_,
    VanGuiInputFlags_CondMask_                   = VanGuiInputFlags_CondHovered | VanGuiInputFlags_CondActive,
    VanGuiInputFlags_RouteTypeMask_              = VanGuiInputFlags_RouteActive | VanGuiInputFlags_RouteFocused | VanGuiInputFlags_RouteGlobal | VanGuiInputFlags_RouteAlways,
    VanGuiInputFlags_RouteOptionsMask_           = VanGuiInputFlags_RouteOverFocused | VanGuiInputFlags_RouteOverActive | VanGuiInputFlags_RouteUnlessBgFocused | VanGuiInputFlags_RouteFromRootWindow,
    VanGuiInputFlags_SupportedByIsKeyPressed     = VanGuiInputFlags_RepeatMask_,
    VanGuiInputFlags_SupportedByIsMouseClicked   = VanGuiInputFlags_Repeat,
    VanGuiInputFlags_SupportedByShortcut         = VanGuiInputFlags_RepeatMask_ | VanGuiInputFlags_RouteTypeMask_ | VanGuiInputFlags_RouteOptionsMask_,
    VanGuiInputFlags_SupportedBySetNextItemShortcut = VanGuiInputFlags_RepeatMask_ | VanGuiInputFlags_RouteTypeMask_ | VanGuiInputFlags_RouteOptionsMask_ | VanGuiInputFlags_Tooltip,
    VanGuiInputFlags_SupportedBySetKeyOwner      = VanGuiInputFlags_LockThisFrame | VanGuiInputFlags_LockUntilRelease,
    VanGuiInputFlags_SupportedBySetItemKeyOwner  = VanGuiInputFlags_SupportedBySetKeyOwner | VanGuiInputFlags_CondMask_,
};

//-----------------------------------------------------------------------------
// [SECTION] Clipper support
//-----------------------------------------------------------------------------

// Note that Max is exclusive, so perhaps should be using a Begin/End convention.
struct VanGuiListClipperRange
{
    int     Min;
    int     Max;
    bool    PosToIndexConvert;      // Begin/End are absolute position (will be converted to indices later)
    VanS8    PosToIndexOffsetMin;    // Add to Min after converting to indices
    VanS8    PosToIndexOffsetMax;    // Add to Min after converting to indices

    static VanGuiListClipperRange    FromIndices(int min, int max)                               { VanGuiListClipperRange r = { min, max, false, 0, 0 }; return r; }
    static VanGuiListClipperRange    FromPositions(float y1, float y2, int off_min, int off_max) { VanGuiListClipperRange r = { static_cast<int>(y1), static_cast<int>(y2), true, static_cast<VanS8>(off_min), static_cast<VanS8>(off_max) }; return r; }
};

// Temporary clipper data, buffers shared/reused between instances
struct VanGuiListClipperData
{
    VanGuiListClipper*               ListClipper;
    float                           LossynessOffset;
    int                             StepNo;
    int                             ItemsFrozen;
    VanVector<VanGuiListClipperRange> Ranges;

    VanGuiListClipperData()          { memset((void*)this, 0, sizeof(*this)); } // VanVector<> members are zero-init-safe (Data=nullptr, Size=0, Capacity=0)
    void                            Reset(VanGuiListClipper* clipper) { ListClipper = clipper; StepNo = ItemsFrozen = 0; Ranges.resize(0); }
};

//-----------------------------------------------------------------------------
// [SECTION] Navigation support
//-----------------------------------------------------------------------------

enum VanGuiActivateFlags_
{
    VanGuiActivateFlags_None                 = 0,
    VanGuiActivateFlags_PreferInput          = 1 << 0,       // Favor activation that requires keyboard text input (e.g. for Slider/Drag). Default for Enter key.
    VanGuiActivateFlags_PreferTweak          = 1 << 1,       // Favor activation for tweaking with arrows or gamepad (e.g. for Slider/Drag). Default for Space key and if keyboard is not used.
    VanGuiActivateFlags_TryToPreserveState   = 1 << 2,       // Request widget to preserve state if it can (e.g. InputText will try to preserve cursor/selection)
    VanGuiActivateFlags_FromTabbing          = 1 << 3,       // Activation requested by a tabbing request (VanGuiNavMoveFlags_IsTabbing)
    VanGuiActivateFlags_FromShortcut         = 1 << 4,       // Activation requested by an item shortcut via SetNextItemShortcut() function.
    VanGuiActivateFlags_FromFocusApi         = 1 << 5,       // Activation requested by an api request (VanGuiNavMoveFlags_FocusApi)
};

// Early work-in-progress API for ScrollToItem()
// FIXME: Missing flags to request making both edges visible when possible.
enum VanGuiScrollFlags_
{
    VanGuiScrollFlags_None                   = 0,
    VanGuiScrollFlags_KeepVisibleEdgeX       = 1 << 0,       // If item is not visible: scroll as little as possible on X axis to bring item back into view [default for X axis]
    VanGuiScrollFlags_KeepVisibleEdgeY       = 1 << 1,       // If item is not visible: scroll as little as possible on Y axis to bring item back into view [default for Y axis for windows that are already visible]
    VanGuiScrollFlags_KeepVisibleCenterX     = 1 << 2,       // If item is not visible: scroll to make the item centered on X axis [rarely used]
    VanGuiScrollFlags_KeepVisibleCenterY     = 1 << 3,       // If item is not visible: scroll to make the item centered on Y axis
    VanGuiScrollFlags_AlwaysCenterX          = 1 << 4,       // Always center the result item on X axis [rarely used]
    VanGuiScrollFlags_AlwaysCenterY          = 1 << 5,       // Always center the result item on Y axis [default for Y axis for appearing window)
    VanGuiScrollFlags_NoScrollParent         = 1 << 6,       // Disable forwarding scrolling to parent window if required to keep item/rect visible (only scroll window the function was applied to).
    VanGuiScrollFlags_MaskX_                 = VanGuiScrollFlags_KeepVisibleEdgeX | VanGuiScrollFlags_KeepVisibleCenterX | VanGuiScrollFlags_AlwaysCenterX,
    VanGuiScrollFlags_MaskY_                 = VanGuiScrollFlags_KeepVisibleEdgeY | VanGuiScrollFlags_KeepVisibleCenterY | VanGuiScrollFlags_AlwaysCenterY,
};

enum VanGuiNavRenderCursorFlags_
{
    VanGuiNavRenderCursorFlags_None          = 0,
    VanGuiNavRenderCursorFlags_Compact       = 1 << 1,       // Compact highlight, no padding/distance from focused item
    VanGuiNavRenderCursorFlags_AlwaysDraw    = 1 << 2,       // Draw rectangular highlight if (g.NavId == id) even when g.NavCursorVisible == false, aka even when using the mouse.
    VanGuiNavRenderCursorFlags_NoRounding    = 1 << 3,
#ifndef VANGUI_DISABLE_OBSOLETE_FUNCTIONS
    VanGuiNavHighlightFlags_None             = VanGuiNavRenderCursorFlags_None,       // Renamed in 1.91.4
    VanGuiNavHighlightFlags_Compact          = VanGuiNavRenderCursorFlags_Compact,    // Renamed in 1.91.4
    VanGuiNavHighlightFlags_AlwaysDraw       = VanGuiNavRenderCursorFlags_AlwaysDraw, // Renamed in 1.91.4
    VanGuiNavHighlightFlags_NoRounding       = VanGuiNavRenderCursorFlags_NoRounding, // Renamed in 1.91.4
    //VanGuiNavHighlightFlags_TypeThin       = VanGuiNavRenderCursorFlags_Compact,    // Renamed in 1.90.2
#endif
};

enum VanGuiNavMoveFlags_
{
    VanGuiNavMoveFlags_None                  = 0,
    VanGuiNavMoveFlags_LoopX                 = 1 << 0,   // On failed request, restart from opposite side
    VanGuiNavMoveFlags_LoopY                 = 1 << 1,
    VanGuiNavMoveFlags_WrapX                 = 1 << 2,   // On failed request, request from opposite side one line down (when NavDir==right) or one line up (when NavDir==left)
    VanGuiNavMoveFlags_WrapY                 = 1 << 3,   // This is not super useful but provided for completeness
    VanGuiNavMoveFlags_WrapMask_             = VanGuiNavMoveFlags_LoopX | VanGuiNavMoveFlags_LoopY | VanGuiNavMoveFlags_WrapX | VanGuiNavMoveFlags_WrapY,
    VanGuiNavMoveFlags_AllowCurrentNavId     = 1 << 4,   // Allow scoring and considering the current NavId as a move target candidate. This is used when the move source is offset (e.g. pressing PageDown actually needs to send a Up move request, if we are pressing PageDown from the bottom-most item we need to stay in place)
    VanGuiNavMoveFlags_AlsoScoreVisibleSet   = 1 << 5,   // Store alternate result in NavMoveResultLocalVisible that only comprise elements that are already fully visible (used by PageUp/PageDown)
    VanGuiNavMoveFlags_ScrollToEdgeY         = 1 << 6,   // Force scrolling to min/max (used by Home/End) // FIXME-NAV: Aim to remove or reword as VanGuiScrollFlags
    VanGuiNavMoveFlags_Forwarded             = 1 << 7,
    VanGuiNavMoveFlags_DebugNoResult         = 1 << 8,   // Dummy scoring for debug purpose, don't apply result
    VanGuiNavMoveFlags_FocusApi              = 1 << 9,   // Requests from focus API can land/focus/activate items even if they are marked with _NoTabStop (see NavProcessItemForTabbingRequest() for details)
    VanGuiNavMoveFlags_IsTabbing             = 1 << 10,  // == Focus + Activate if item is Inputable + DontChangeNavHighlight
    VanGuiNavMoveFlags_IsPageMove            = 1 << 11,  // Identify a PageDown/PageUp request.
    VanGuiNavMoveFlags_Activate              = 1 << 12,  // Activate/select target item.
    VanGuiNavMoveFlags_NoSelect              = 1 << 13,  // Don't trigger selection by not setting g.NavJustMovedTo
    VanGuiNavMoveFlags_NoSetNavCursorVisible = 1 << 14,  // Do not alter the nav cursor visible state
    VanGuiNavMoveFlags_NoClearActiveId       = 1 << 15,  // (Experimental) Do not clear active id when applying move result
};

enum VanGuiNavLayer
{
    VanGuiNavLayer_Main  = 0,    // Main scrolling layer
    VanGuiNavLayer_Menu  = 1,    // Menu layer (access with Alt)
    VanGuiNavLayer_COUNT
};

// Storage for navigation query/results
struct VanGuiNavItemData
{
    VanGuiWindow*        Window;         // Init,Move    // Best candidate window (result->ItemWindow->RootWindowForNav == request->Window)
    VanGuiID             ID;             // Init,Move    // Best candidate item ID
    VanGuiID             FocusScopeId;   // Init,Move    // Best candidate focus scope ID
    VanRect              RectRel;        // Init,Move    // Best candidate bounding box in window relative space
    VanGuiItemFlags      ItemFlags;      // ????,Move    // Best candidate item flags
    float               DistBox;        //      Move    // Best candidate box distance to current NavId
    float               DistCenter;     //      Move    // Best candidate center distance to current NavId
    float               DistAxial;      //      Move    // Best candidate axial distance to current NavId
    VanGuiSelectionUserData SelectionUserData;//I+Mov    // Best candidate SetNextItemSelectionUserData() value. Valid if (ItemFlags & VanGuiItemFlags_HasSelectionUserData)

    VanGuiNavItemData()  { Clear(); }
    void Clear()        { Window = nullptr; ID = FocusScopeId = 0; ItemFlags = 0; SelectionUserData = -1; DistBox = DistCenter = DistAxial = FLT_MAX; }
};

// Storage for PushFocusScope(), g.FocusScopeStack[], g.NavFocusRoute[]
struct VanGuiFocusScopeData
{
    VanGuiID             ID;
    VanGuiID             WindowID;
};

//-----------------------------------------------------------------------------
// [SECTION] Typing-select support
//-----------------------------------------------------------------------------

// Flags for GetTypingSelectRequest()
enum VanGuiTypingSelectFlags_
{
    VanGuiTypingSelectFlags_None                 = 0,
    VanGuiTypingSelectFlags_AllowBackspace       = 1 << 0,   // Backspace to delete character inputs. If using: ensure GetTypingSelectRequest() is not called more than once per frame (filter by e.g. focus state)
    VanGuiTypingSelectFlags_AllowSingleCharMode  = 1 << 1,   // Allow "single char" search mode which is activated when pressing the same character multiple times.
};

// Returned by GetTypingSelectRequest(), designed to eventually be public.
struct VANGUI_API VanGuiTypingSelectRequest
{
    VanGuiTypingSelectFlags  Flags;              // Flags passed to GetTypingSelectRequest()
    int                     SearchBufferLen;
    const char*             SearchBuffer;       // Search buffer contents (use full string. unless SingleCharMode is set, in which case use SingleCharSize).
    bool                    SelectRequest;      // Set when buffer was modified this frame, requesting a selection.
    bool                    SingleCharMode;     // Notify when buffer contains same character repeated, to implement special mode. In this situation it preferred to not display any on-screen search indication.
    VanS8                    SingleCharSize;     // Length in bytes of first letter codepoint (1 for ascii, 2-4 for UTF-8). If (SearchBufferLen==RepeatCharSize) only 1 letter has been input.
};

// Storage for GetTypingSelectRequest()
struct VANGUI_API VanGuiTypingSelectState
{
    VanGuiTypingSelectRequest Request;           // User-facing data
    char            SearchBuffer[64];           // Search buffer: no need to make dynamic as this search is very transient.
    VanGuiID         FocusScope;
    int             LastRequestFrame = 0;
    float           LastRequestTime = 0.0f;
    bool            SingleCharModeLock = false; // After a certain single char repeat count we lock into SingleCharMode. Two benefits: 1) buffer never fill, 2) we can provide an immediate SingleChar mode without timer elapsing.

    VanGuiTypingSelectState() { memset((void*)this, 0, sizeof(*this)); } // POD-safe zero-init
    void            Clear()  { SearchBuffer[0] = 0; SingleCharModeLock = false; } // We preserve remaining data for easier debugging
};

//-----------------------------------------------------------------------------
// [SECTION] Columns support
//-----------------------------------------------------------------------------

// Flags for internal's BeginColumns(). This is an obsolete API. Prefer using BeginTable() nowadays!
enum VanGuiOldColumnFlags_
{
    VanGuiOldColumnFlags_None                    = 0,
    VanGuiOldColumnFlags_NoBorder                = 1 << 0,   // Disable column dividers
    VanGuiOldColumnFlags_NoResize                = 1 << 1,   // Disable resizing columns when clicking on the dividers
    VanGuiOldColumnFlags_NoPreserveWidths        = 1 << 2,   // Disable column width preservation when adjusting columns
    VanGuiOldColumnFlags_NoForceWithinWindow     = 1 << 3,   // Disable forcing columns to fit within window
    VanGuiOldColumnFlags_GrowParentContentsSize  = 1 << 4,   // Restore pre-1.51 behavior of extending the parent window contents size but _without affecting the columns width at all_. Will eventually remove.

    // Obsolete names (will be removed)
#ifndef VANGUI_DISABLE_OBSOLETE_FUNCTIONS
    //VanGuiColumnsFlags_None                    = VanGuiOldColumnFlags_None,
    //VanGuiColumnsFlags_NoBorder                = VanGuiOldColumnFlags_NoBorder,
    //VanGuiColumnsFlags_NoResize                = VanGuiOldColumnFlags_NoResize,
    //VanGuiColumnsFlags_NoPreserveWidths        = VanGuiOldColumnFlags_NoPreserveWidths,
    //VanGuiColumnsFlags_NoForceWithinWindow     = VanGuiOldColumnFlags_NoForceWithinWindow,
    //VanGuiColumnsFlags_GrowParentContentsSize  = VanGuiOldColumnFlags_GrowParentContentsSize,
#endif
};

struct VanGuiOldColumnData
{
    float               OffsetNorm;             // Column start offset, normalized 0.0 (far left) -> 1.0 (far right)
    float               OffsetNormBeforeResize;
    VanGuiOldColumnFlags Flags;                  // Not exposed
    VanRect              ClipRect;

    VanGuiOldColumnData() { memset((void*)this, 0, sizeof(*this)); } // POD-safe zero-init
};

struct VanGuiOldColumns
{
    VanGuiID             ID;
    VanGuiOldColumnFlags Flags;
    bool                IsFirstFrame;
    bool                IsBeingResized;
    int                 Current;
    int                 Count;
    float               OffMinX, OffMaxX;       // Offsets from HostWorkRect.Min.x
    float               LineMinY, LineMaxY;
    float               HostCursorPosY;         // Backup of CursorPos at the time of BeginColumns()
    float               HostCursorMaxPosX;      // Backup of CursorMaxPos at the time of BeginColumns()
    VanRect              HostInitialClipRect;    // Backup of ClipRect at the time of BeginColumns()
    VanRect              HostBackupClipRect;     // Backup of ClipRect during PushColumnsBackground()/PopColumnsBackground()
    VanRect              HostBackupParentWorkRect;//Backup of WorkRect at the time of BeginColumns()
    VanVector<VanGuiOldColumnData> Columns;
    VanDrawListSplitter  Splitter;

    VanGuiOldColumns()   { memset((void*)this, 0, sizeof(*this)); } // VanVector<>/VanDrawListSplitter members are zero-init-safe (Data=nullptr, Size=0, Capacity=0)
};

//-----------------------------------------------------------------------------
// [SECTION] Box-select support
//-----------------------------------------------------------------------------

struct VanGuiBoxSelectState
{
    // Active box-selection data (persistent, 1 active at a time)
    VanGuiID                 ID;
    bool                    IsActive;
    bool                    IsStarting;
    bool                    IsStartedFromVoid;  // Starting click was not from an item.
    bool                    IsStartedSetNavIdOnce;
    bool                    RequestClear;
    VanGuiKeyChord           KeyMods : 16;       // Latched key-mods for box-select logic.
    VanVec2                  StartPosRel;        // Start position in window-contents relative space (to support scrolling)
    VanVec2                  EndPosRel;          // End position in window-contents relative space
    VanVec2                  ScrollAccum;        // Scrolling accumulator (to behave at high-frame spaces)
    VanGuiWindow*            Window;

    // Temporary/Transient data
    bool                    UnclipMode;         // (Temp/Transient, here in hot area). Set/cleared by the BeginMultiSelect()/EndMultiSelect() owning active box-select.
    VanRect                  UnclipRect;         // Rectangle where ItemAdd() clipping may be temporarily disabled. Need support by multi-select supporting widgets.
    VanRect                  UnclipRects[2];     // Per-axis versions.
    VanRect                  BoxSelectRectPrev;  // Selection rectangle in absolute coordinates (derived every frame from BoxSelectStartPosRel and MousePos)
    VanRect                  BoxSelectRectCurr;

    VanGuiBoxSelectState()   { memset((void*)this, 0, sizeof(*this)); } // POD-safe zero-init
};

//-----------------------------------------------------------------------------
// [SECTION] Multi-select support
//-----------------------------------------------------------------------------

// We always assume that -1 is an invalid value (which works for indices and pointers)
#define VanGuiSelectionUserData_Invalid        ((VanGuiSelectionUserData)-1)

// Temporary storage for multi-select
struct VANGUI_API VanGuiMultiSelectTempData
{
    VanGuiMultiSelectIO      IO;                 // MUST BE FIRST FIELD. Requests are set and returned by BeginMultiSelect()/EndMultiSelect() + written to by user during the loop.
    VanGuiMultiSelectState*  Storage;
    VanGuiID                 FocusScopeId;       // Copied from g.CurrentFocusScopeId (unless another selection scope was pushed manually)
    VanGuiMultiSelectFlags   Flags;
    VanVec2                  ScopeRectMin;
    VanVec2                  BackupCursorMaxPos;
    //VanGuiSelectionUserData CurrSubmittedItem; // Copy of last submitted item data, used to merge output ranges.
    //VanGuiSelectionUserData PrevSubmittedItem; // Copy of previous submitted item data, used to merge output ranges.
    VanGuiID                 BoxSelectId;
    VanGuiKeyChord           KeyMods;
    VanS8                    LoopRequestSetAll;  // -1: no operation, 0: clear all, 1: select all.
    bool                    IsEndIO;            // Set when switching IO from BeginMultiSelect() to EndMultiSelect() state.
    bool                    IsFocused;          // Set if currently focusing the selection scope (any item of the selection). May be used if you have custom shortcut associated to selection.
    bool                    IsKeyboardSetRange; // Set by BeginMultiSelect() when using Shift+Navigation. Because scrolling may be affected we can't afford a frame of lag with Shift+Navigation.
    bool                    NavIdPassedBy;
    bool                    RangeSrcPassedBy;   // Set by the item that matches RangeSrcItem.
    bool                    RangeDstPassedBy;   // Set by the item that matches NavJustMovedToId when IsSetRange is set.

    VanGuiMultiSelectTempData()  { Clear(); }
    void Clear()            { size_t io_sz = sizeof(IO); ClearIO(); memset((void*)(&IO + 1), 0, sizeof(*this) - io_sz); } // Zero-clear except IO as we preserve IO.Requests[] buffer allocation.
    void ClearIO()          { IO.Requests.resize(0); IO.RangeSrcItem = IO.NavIdItem = VanGuiSelectionUserData_Invalid; IO.NavIdSelected = IO.RangeSrcReset = false; }
};

// Persistent storage for multi-select (as long as selection is alive)
struct VANGUI_API VanGuiMultiSelectState
{
    VanGuiWindow*            Window;
    VanGuiID                 ID;
    int                     LastFrameActive;    // Last used frame-count, for GC.
    int                     LastSelectionSize;  // Set by BeginMultiSelect() based on optional info provided by user. May be -1 if unknown.
    VanS8                    RangeSelected;      // -1 (don't have) or true/false
    VanS8                    NavIdSelected;      // -1 (don't have) or true/false
    VanGuiSelectionUserData  RangeSrcItem;       //
    VanGuiSelectionUserData  NavIdItem;          // SetNextItemSelectionUserData() value for NavId (if part of submitted items)

    VanGuiMultiSelectState() { Window = nullptr; ID = 0; LastFrameActive = LastSelectionSize = 0; RangeSelected = NavIdSelected = -1; RangeSrcItem = NavIdItem = VanGuiSelectionUserData_Invalid; }
};

//-----------------------------------------------------------------------------
// [SECTION] Docking support
//-----------------------------------------------------------------------------

// Docking
using VanGuiDockNodeFlags = int; // forward (defined in vangui.h)

enum VanGuiDockNodeState : int
{
    VanGuiDockNodeState_Unknown,
    VanGuiDockNodeState_HostWindowHiddenBecauseSingleWindow,
    VanGuiDockNodeState_HostWindowHiddenBecauseWindowsAreResizing,
    VanGuiDockNodeState_HostWindowVisible,
};

enum VanGuiDockRequestType : int
{
    VanGuiDockRequestType_None = 0,
    VanGuiDockRequestType_Dock,
    VanGuiDockRequestType_Undock,
    VanGuiDockRequestType_Split,    // Split a node and take one side
};

struct VanGuiDockRequest
{
    VanGuiDockRequestType    Type                    = VanGuiDockRequestType_None;
    VanGuiWindow*            DockTargetWindow        = nullptr; // Destination/Target window to dock into (may be a loose window or a DockNode's host window)
    VanGuiDockNode*          DockTargetNode          = nullptr; // Destination/Target Node to dock into
    VanGuiWindow*            DockPayload             = nullptr; // Source/Payload window to dock (may be a loose window or a DockNode's host window)
    VanGuiDir                DockSplitDir            = VanGuiDir_None;
    float                    DockSplitRatio           = 0.5f;
    bool                     DockSplitOuter           = false;
    VanGuiWindow*            UndockTargetWindow      = nullptr;
    VanGuiDockNode*          UndockTargetNode        = nullptr;
};

struct VanGuiDockNodeSettings
{
    VanGuiID                 ID                      = 0;
    VanGuiID                 ParentNodeId            = 0;
    VanGuiID                 ParentWindowId          = 0;
    VanGuiID                 SelectedTabId           = 0;
    VanS8                    SplitAxis               = static_cast<VanS8>(VanGuiAxis_None);
    VanS8                    Depth                   = 0;
    VanGuiDockNodeFlags      Flags                   = 0;           // NB: We save individual flags one by one in ascii format (VanGuiDockNodeFlags_SavedFlagsMask_)
    VanVec2ih                Pos;
    VanVec2ih                Size;
    VanVec2ih                SizeRef;
};

// sizeof() = 156~192
struct VanGuiDockNode
{
    VanGuiID                 ID                      = 0;
    VanGuiDockNodeFlags      SharedFlags             = 0;           // (Write) Flags shared by all windows using this DockNode
    VanGuiDockNodeFlags      LocalFlags              = 0;           // (Write) Flags specific to this DockNode
    VanGuiDockNodeFlags      LocalFlagsInWindows     = 0;           // (Write) Flags specific to this DockNode, applied from windows
    VanGuiDockNodeFlags      MergedFlags             = 0;           // (Read) Effective flags (== SharedFlags | LocalFlagsInWindows | LocalFlags)
    VanGuiDockNodeState      State                   = VanGuiDockNodeState_Unknown;
    VanGuiDockNode*          ParentNode              = nullptr;
    VanGuiDockNode*          ChildNodes[2]           = { nullptr, nullptr };  // [0]=NW,  [1]=SE child nodes
    VanVector<VanGuiWindow*> Windows;                               // Note: unordered list! Iterate tab bar to preserve order.
    VanGuiTabBar*            TabBar                  = nullptr;
    VanVec2                  Pos;                                   // Current position
    VanVec2                  Size;                                  // Current size
    VanVec2                  SizeRef;                               // [Split node only] Last explicitly written-to size (overridden when using a splitter affecting the node), used to calculate Size.
    VanGuiAxis               SplitAxis               = VanGuiAxis_None; // [Split node only] Split axis (X or Y)
    VanGuiWindowClass        WindowClass;                           // [Root node only]
    VanU32                   LastBgColor             = 0;

    VanGuiWindow*            HostWindow              = nullptr;
    VanGuiWindow*            VisibleWindow           = nullptr;     // Generally point to window which is ID is == SelectedTabID, but when CTRL+Tabbing this can be a different window.
    VanGuiDockNode*          CentralNode             = nullptr;     // [Root node only] Pointer to central node.
    VanGuiDockNode*          OnlyNodeWithWindows     = nullptr;     // [Root node only] Set when there is a single visible node within the hierarchy.
    int                      CountNodeWithWindows    = 0;           // [Root node only]
    int                      LastFrameAlive          = -1;          // Last frame number the node was updated or kept alive explicitly with DockSpace() + VanGuiDockNodeFlags_KeepAliveOnly
    int                      LastFrameActive         = -1;          // Last frame number the node was updated.
    int                      LastFrameFocused        = -1;          // Last frame number the node was focused.
    VanGuiID                 LastFocusedNodeId       = 0;           // [Root node only] Which of our child docking node (any ancestor) was last focused.
    VanGuiID                 SelectedTabId           = 0;           // [Leaf node only] Which window is selected.
    VanGuiID                 WantCloseTabId          = 0;           // [Leaf node only] Set when closing a specific tab.
    VanGuiID                 RefViewportId           = 0;           // Reference viewport ID from visible window when HostWindow == nullptr.

    // Flags
    bool                     IsDockSpace             : 1 = false;   // IsExplicitChild
    bool                     IsDocumentNode          : 1 = false;
    bool                     IsLeafNode              : 1 = false;   // IsSplitNode
    bool                     IsCentralNode           : 1 = false;
    bool                     IsHiddenTabBar          : 1 = false;   // Set when the user left-clicked on the dockspace
    bool                     WantHiddenTabBarUpdate  : 1 = false;
    bool                     WantHiddenTabBarToggle  : 1 = false;
    bool                     MarkedForPosSizeWrite   : 1 = false;

    [[nodiscard]] bool IsRootNode() const noexcept      { return ParentNode == nullptr; }
    [[nodiscard]] bool IsDockSpace2() const noexcept    { return IsDockSpace; }
    [[nodiscard]] bool IsFloatingNode() const noexcept  { return ParentNode == nullptr && !IsDockSpace; }
    [[nodiscard]] bool IsEmpty() const noexcept         { return ChildNodes[0] == nullptr && Windows.Size == 0; }
    [[nodiscard]] VanGuiDockNodeFlags GetMergedFlags() const noexcept { return SharedFlags | LocalFlags | LocalFlagsInWindows; }
    [[nodiscard]] VanRect Rect() const noexcept         { return VanRect(Pos.x, Pos.y, Pos.x + Size.x, Pos.y + Size.y); }
    void SetLocalFlags(VanGuiDockNodeFlags flags) noexcept { LocalFlags = flags; UpdateMergedFlags(); }
    void UpdateMergedFlags() noexcept { MergedFlags = SharedFlags | LocalFlags | LocalFlagsInWindows; }
};

struct VanGuiDockContext
{
    VanGuiStorage                    Nodes;               // Map ID -> VanGuiDockNode*: IDs are derived via a hash of the resource
    VanVector<VanGuiDockRequest>     Requests;
    VanVector<VanGuiDockNodeSettings> NodesSettings;
    bool                             WantFullRebuild     = false;
    VanGuiID                         NodeIdNext          = 0;   // Counter for generating unique dock node IDs
};

//-----------------------------------------------------------------------------
// [SECTION] Viewport support
//-----------------------------------------------------------------------------

// VanGuiPlatformMonitor is defined in vangui.h (public API). Forward-declared here for internal use.
// Do not redefine: including both vangui.h + vangui_internal.h in the same TU would cause C2011.
// struct VanGuiPlatformMonitor; // (already fully defined via vangui.h)

// VanGuiViewportP: internal viewport (extends public VanGuiViewport)
// sizeof() currently ~272 bytes (add to VanGuiViewport) + window/frame data
struct VanGuiViewportP : public VanGuiViewport
{
    VanGuiWindow*   Window                  = nullptr;  // Set when the viewport is owned by a window (and OnlyWindow == null means we're the "main" viewport). MainViewport always has window == nullptr.
    int             Idx                     = 0;
    int             LastFrameActive         = -1;       // Last frame number this viewport was activated by a window
    int             LastFocusedStampCount   = 0;        // Last stamp number from when a window hosted by this viewport was focused (by comparing this value between two viewport we have an implicit viewport z-order we use as fallback)
    VanGuiID        LastNameHash            = 0;
    VanVec2         LastPos;
    float           Alpha                   = 1.0f;     // Window opacity (when dragging dockable windows/viewports we make them transparent)
    float           LastAlpha               = 1.0f;
    bool            LastFocusedHadNavWindow = false;    // Was the last window to be focused part of a nav enabled hierarchy?
    short           PlatformMonitor         = -1;
    int             BgFgDrawListsLastFrame[2]      = { -1, -1 };    // Last frame number the background (0) and foreground (1) draw lists were used
    float           BgFgDrawListsLastTimeActive[2] = { -1.0f, -1.0f }; // Last g.Time the background (0) and foreground (1) draw lists were actively used (for GC threshold)
    VanDrawList*    BgFgDrawLists[2]               = { nullptr, nullptr }; // Convenience background (0) and foreground (1) draw lists. We use them to draw software mouser cursor when io.MouseDrawCursor is set and to draw most debug overlays.
    VanDrawData     DrawDataP;
    VanDrawDataBuilder DrawDataBuilder;                 // Temporary data while building final VanDrawData
    VanVec2         WorkInsetMin;                       // Work Area: Inset from Pos to top-left of Work Area. GENERALLY (0,0) or (0,+main_menu_bar_height). Work Area is Full Area but without menu-bars/status-bars.
    VanVec2         WorkInsetMax;                       // Work Area: Inset from Pos+Size to bottom-right of Work Area. GENERALLY (0,0) or (0,-status_bar_height).
    VanVec2         BuildWorkInsetMin;                  // Work Area: Inset being built/updated during current frame
    VanVec2         BuildWorkInsetMax;
    VanVec2         LastPlatformPos     = VanVec2(FLT_MAX, FLT_MAX);
    VanVec2         LastPlatformSize    = VanVec2(FLT_MAX, FLT_MAX);

    [[nodiscard]] VanRect GetMainRect() const noexcept { return VanRect(Pos.x, Pos.y, Pos.x + Size.x, Pos.y + Size.y); }
    [[nodiscard]] VanRect GetWorkRect() const noexcept { return VanRect(WorkPos.x, WorkPos.y, WorkPos.x + WorkSize.x, WorkPos.y + WorkSize.y); }
    void UpdateWorkRect() noexcept { WorkPos = VanVec2(Pos.x + WorkInsetMin.x, Pos.y + WorkInsetMin.y); WorkSize = VanVec2(VanMax(0.0f, Size.x - WorkInsetMin.x + WorkInsetMax.x), VanMax(0.0f, Size.y - WorkInsetMin.y + WorkInsetMax.y)); }
};

//-----------------------------------------------------------------------------
// [SECTION] Settings support
//-----------------------------------------------------------------------------

// Windows data saved in vangui.ini file
// Because we never destroy or rename VanGuiWindowSettings, we can store the names in a separate buffer easily.
// (this is designed to be stored in a VanChunkStream buffer, with the variable-length Name following our structure)
struct VanGuiWindowSettings
{
    VanGuiID     ID;
    VanVec2ih    Pos;
    VanVec2ih    Size;
    bool        Collapsed;
    bool        IsChild;
    bool        WantApply;      // Set when loaded from .ini data (to enable merging/loading .ini data into an already running context)
    bool        WantDelete;     // Set to invalidate/delete the settings entry

    VanGuiWindowSettings()       { memset((void*)this, 0, sizeof(*this)); } // POD-safe zero-init
    char* GetName()             { return reinterpret_cast<char*>(this + 1); }
};

struct VanGuiSettingsHandler
{
    const char* TypeName;       // Short description stored in .ini file. Disallowed characters: '[' ']'
    VanGuiID     TypeHash;       // == VanHashStr(TypeName)
    void        (*ClearAllFn)(VanGuiContext* ctx, VanGuiSettingsHandler* handler);                                // Clear all settings data
    void        (*ReadInitFn)(VanGuiContext* ctx, VanGuiSettingsHandler* handler);                                // Read: Called before reading (in registration order)
    void*       (*ReadOpenFn)(VanGuiContext* ctx, VanGuiSettingsHandler* handler, const char* name);              // Read: Called when entering into a new ini entry e.g. "[Window][Name]"
    void        (*ReadLineFn)(VanGuiContext* ctx, VanGuiSettingsHandler* handler, void* entry, const char* line); // Read: Called for every line of text within an ini entry
    void        (*ApplyAllFn)(VanGuiContext* ctx, VanGuiSettingsHandler* handler);                                // Read: Called after reading (in registration order)
    void        (*WriteAllFn)(VanGuiContext* ctx, VanGuiSettingsHandler* handler, VanGuiTextBuffer* out_buf);      // Write: Output every entries into 'out_buf'
    void*       UserData;

    VanGuiSettingsHandler() { memset((void*)this, 0, sizeof(*this)); } // POD-safe zero-init (all members are pointers/primitives)
};

//-----------------------------------------------------------------------------
// [SECTION] Localization support
//-----------------------------------------------------------------------------

// This is experimental and not officially supported, it'll probably fall short of features, if/when it does we may backtrack.
enum VanGuiLocKey : int
{
    VanGuiLocKey_VersionStr,
    VanGuiLocKey_TableSizeOne,
    VanGuiLocKey_TableSizeAllFit,
    VanGuiLocKey_TableSizeAllDefault,
    VanGuiLocKey_TableResetOrder,
    VanGuiLocKey_WindowingMainMenuBar,
    VanGuiLocKey_WindowingPopup,
    VanGuiLocKey_WindowingUntitled,
    VanGuiLocKey_OpenLink_s,
    VanGuiLocKey_CopyLink,
    VanGuiLocKey_COUNT
};

struct VanGuiLocEntry
{
    VanGuiLocKey     Key;
    const char*     Text;
};

//-----------------------------------------------------------------------------
// [SECTION] Error handling, State recovery support
//-----------------------------------------------------------------------------

// Macros used by Recoverable Error handling
// - Only dispatch error if _EXPR: evaluate as assert (similar to an assert macro).
// - The message will always be a string literal, in order to increase likelihood of being display by an assert handler.
// - See 'Demo->Configuration->Error Handling' and VanGuiIO definitions for details on error handling.
// - Read https://github.com/ocornut/vangui/wiki/Error-Handling for details on error handling.
#ifndef VAN_ASSERT_USER_ERROR
#define VAN_ASSERT_USER_ERROR(_EXPR,_MSG)            do { if (!(_EXPR)) { if (VanGui::ErrorLog(_MSG)) { VAN_ASSERT((_EXPR) && _MSG); } } } while (0)               // Recoverable User Error
#define VAN_ASSERT_USER_ERROR_RET(_EXPR,_MSG)        do { if (!(_EXPR)) { if (VanGui::ErrorLog(_MSG)) { VAN_ASSERT((_EXPR) && _MSG); } return; } } while (0)       // Recoverable User Error
#define VAN_ASSERT_USER_ERROR_RETV(_EXPR,_RETV,_MSG) do { if (!(_EXPR)) { if (VanGui::ErrorLog(_MSG)) { VAN_ASSERT((_EXPR) && _MSG); } return _RETV; } } while (0) // Recoverable User Error
#endif

// The error callback is currently not public, as it is expected that only advanced users will rely on it.
using VanGuiErrorCallback = void (*)(VanGuiContext* ctx, void* user_data, const char* msg); // Function signature for g.ErrorCallback

//-----------------------------------------------------------------------------
// [SECTION] Metrics, Debug Tools
//-----------------------------------------------------------------------------

// See VANGUI_DEBUG_LOG() and VANGUI_DEBUG_LOG_XXX() macros.
enum VanGuiDebugLogFlags_
{
    // Event types
    VanGuiDebugLogFlags_None                 = 0,
    VanGuiDebugLogFlags_EventError           = 1 << 0,   // Error submitted by VAN_ASSERT_USER_ERROR()
    VanGuiDebugLogFlags_EventActiveId        = 1 << 1,
    VanGuiDebugLogFlags_EventFocus           = 1 << 2,
    VanGuiDebugLogFlags_EventPopup           = 1 << 3,
    VanGuiDebugLogFlags_EventNav             = 1 << 4,
    VanGuiDebugLogFlags_EventClipper         = 1 << 5,
    VanGuiDebugLogFlags_EventSelection       = 1 << 6,
    VanGuiDebugLogFlags_EventIO              = 1 << 7,
    VanGuiDebugLogFlags_EventFont            = 1 << 8,
    VanGuiDebugLogFlags_EventInputRouting    = 1 << 9,
    VanGuiDebugLogFlags_EventDocking         = 1 << 10,  // Unused in this branch
    VanGuiDebugLogFlags_EventViewport        = 1 << 11,  // Unused in this branch

    VanGuiDebugLogFlags_EventMask_           = VanGuiDebugLogFlags_EventError | VanGuiDebugLogFlags_EventActiveId | VanGuiDebugLogFlags_EventFocus | VanGuiDebugLogFlags_EventPopup | VanGuiDebugLogFlags_EventNav | VanGuiDebugLogFlags_EventClipper | VanGuiDebugLogFlags_EventSelection | VanGuiDebugLogFlags_EventIO | VanGuiDebugLogFlags_EventFont | VanGuiDebugLogFlags_EventInputRouting | VanGuiDebugLogFlags_EventDocking | VanGuiDebugLogFlags_EventViewport,
    VanGuiDebugLogFlags_OutputToTTY          = 1 << 20,  // Also send output to TTY
    VanGuiDebugLogFlags_OutputToDebugger     = 1 << 21,  // Also send output to Debugger Console [Windows only]
    VanGuiDebugLogFlags_OutputToTestEngine   = 1 << 22,  // Also send output to VanGUI Test Engine
};

struct VanGuiDebugAllocEntry
{
    int         FrameCount;
    VanS16       AllocCount;
    VanS16       FreeCount;
};

struct VanGuiDebugAllocInfo
{
    int         TotalAllocCount;            // Number of call to MemAlloc().
    int         TotalFreeCount;
    VanS16       LastEntriesIdx;             // Current index in buffer
    VanGuiDebugAllocEntry LastEntriesBuf[6]; // Track last 6 frames that had allocations

    VanGuiDebugAllocInfo() { memset((void*)this, 0, sizeof(*this)); } // POD-safe zero-init
};

struct VanGuiMetricsConfig
{
    bool        ShowDebugLog = false;
    bool        ShowIDStackTool = false;
    bool        ShowWindowsRects = false;
    bool        ShowWindowsBeginOrder = false;
    bool        ShowTablesRects = false;
    bool        ShowDrawCmdMesh = true;
    bool        ShowDrawCmdBoundingBoxes = true;
    bool        ShowTextEncodingViewer = false;
    bool        ShowTextureUsedRect = false;
    int         ShowWindowsRectsType = -1;
    int         ShowTablesRectsType = -1;
    int         HighlightMonitorIdx = -1;
    VanGuiID     HighlightViewportID = 0;
    bool        ShowFontPreview = true;
};

struct VanGuiStackLevelInfo
{
    VanGuiID                 ID;
    VanS8                    QueryFrameCount;            // >= 1: Sub-query in progress
    bool                    QuerySuccess;               // Sub-query obtained result from DebugHookIdInfo()
    VanS8                    DataType;                   // VanGuiDataType
    int                     DescOffset;                 // -1 or offset into parent's ResultsPathsBuf

    VanGuiStackLevelInfo()   { memset((void*)this, 0, sizeof(*this)); DataType = -1; DescOffset = -1; } // POD-safe zero-init
};

struct VanGuiDebugItemPathQuery
{
    VanGuiID                 MainID;                     // ID to query details for.
    bool                    Active;                     // Used to disambiguate the case when ID == 0 and e.g. some code calls PushOverrideID(0).
    bool                    Complete;                   // All sub-queries are finished (some may have failed).
    VanS8                    Step;                       // -1: query stack + init Results, >= 0: filling individual stack level.
    VanVector<VanGuiStackLevelInfo> Results;
    VanGuiTextBuffer         ResultsDescBuf;
    VanGuiTextBuffer         ResultPathBuf;

    VanGuiDebugItemPathQuery() { memset((void*)this, 0, sizeof(*this)); } // VanVector<>/VanGuiTextBuffer members are zero-init-safe (Data=nullptr, Size=0, Capacity=0)
};

// State for ID Stack tool queries
struct VanGuiIDStackTool
{
    bool                    OptHexEncodeNonAsciiChars;
    bool                    OptCopyToClipboardOnCtrlC;
    int                     LastActiveFrame;
    float                   CopyToClipboardLastTime;

    VanGuiIDStackTool()      { memset((void*)this, 0, sizeof(*this)); LastActiveFrame = -1; OptHexEncodeNonAsciiChars = true; CopyToClipboardLastTime = -FLT_MAX; } // POD-safe zero-init
};

//-----------------------------------------------------------------------------
// [SECTION] Generic context hooks
//-----------------------------------------------------------------------------

using VanGuiContextHookCallback = void (*)(VanGuiContext* ctx, VanGuiContextHook* hook);
enum VanGuiContextHookType { VanGuiContextHookType_NewFramePre, VanGuiContextHookType_NewFramePost, VanGuiContextHookType_EndFramePre, VanGuiContextHookType_EndFramePost, VanGuiContextHookType_RenderPre, VanGuiContextHookType_RenderPost, VanGuiContextHookType_Shutdown, VanGuiContextHookType_PendingRemoval_ };

struct VanGuiContextHook
{
    VanGuiID                     HookId;     // A unique ID assigned by AddContextHook()
    VanGuiContextHookType        Type;
    VanGuiID                     Owner;
    VanGuiContextHookCallback    Callback;
    void*                       UserData;

    VanGuiContextHook()          { memset((void*)this, 0, sizeof(*this)); } // POD-safe zero-init (all members are primitives/pointers)
};

using VanGuiDemoMarkerCallback = void (*)(const char* file, int line, const char* section);

//-----------------------------------------------------------------------------
// [SECTION] VanGuiContext (main VanGUI context)
//-----------------------------------------------------------------------------

struct VanGuiContext
{
    bool                    Initialized;
    bool                    WithinFrameScope;                   // Set by NewFrame(), cleared by EndFrame()
    bool                    WithinFrameScopeWithImplicitWindow; // Set by NewFrame(), cleared by EndFrame() when the implicit debug window has been pushed
    bool                    TestEngineHookItems;                // Will call test engine hooks: VanGuiTestEngineHook_ItemAdd(), VanGuiTestEngineHook_ItemInfo(), VanGuiTestEngineHook_Log()
    int                     FrameCount;
    int                     FrameCountEnded;
    int                     FrameCountRendered;
    int                     FrameCountPlatformEnded;    // Set by UpdatePlatformWindows() to ensure it is called after Render()/EndFrame()
    double                  Time;
    char                    ContextName[16];                    // Storage for a context name (to facilitate debugging multi-context setups)
    VanGuiIO                 IO;
    VanGuiPlatformIO         PlatformIO;
    VanGuiStyle              Style;
    VanVector<VanFontAtlas*>  FontAtlases;                        // List of font atlases used by the context (generally only contains g.IO.Fonts aka the main font atlas)
    VanFont*                 Font;                               // Currently bound font. (== FontStack.back().Font)
    VanFontBaked*            FontBaked;                          // Currently bound font at currently bound size. (== Font->GetFontBaked(FontSize))
    float                   FontSize;                           // Currently bound font size == line height (== FontSizeBase + externals scales applied in the UpdateCurrentFontSize() function).
    float                   FontSizeBase;                       // Font size before scaling == style.FontSizeBase == value passed to PushFont() when specified.
    float                   FontBakedScale;                     // == FontBaked->Size / FontSize. Scale factor over baked size. Rarely used nowadays, very often == 1.0f.
    float                   FontRasterizerDensity;              // Current font density. Used by all calls to GetFontBaked().
    VanDrawListSharedData    DrawListSharedData;
    VanGuiID                 WithinEndChildID;                   // Set within EndChild()
    VanGuiID                 WithinEndPopupID;                   // Set within EndPopup()
    void*                   TestEngine;                         // Test engine user data

    // Inputs
    VanVector<VanGuiInputEvent> InputEventsQueue;                 // Input events which will be trickled/written into IO structure.
    VanVector<VanGuiInputEvent> InputEventsTrail;                 // Past input events processed in NewFrame(). This is to allow domain-specific application to access e.g mouse/pen trail.
    VanGuiMouseSource        InputEventsNextMouseSource;
    VanU32                   InputEventsNextEventId;

    // Windows state
    VanVector<VanGuiWindow*>  Windows;                            // Windows, sorted in display order, back to front
    VanVector<VanGuiWindow*>  WindowsFocusOrder;                  // Root windows, sorted in focus order, back to front.
    VanVector<VanGuiWindow*>  WindowsTempSortBuffer;              // Temporary buffer used in EndFrame() to reorder windows so parents are kept before their child
    VanVector<VanGuiWindowStackData> CurrentWindowStack;
    VanGuiStorage            WindowsById;                        // Map window's VanGuiID to VanGuiWindow*
    int                     WindowsActiveCount;                 // Number of unique windows submitted by frame
    float                   WindowsBorderHoverPadding;          // Padding around resizable windows for which hovering on counts as hovering the window == VanMax(style.TouchExtraPadding, style.WindowBorderHoverPadding). This isn't so multi-dpi friendly.
    VanGuiID                 DebugBreakInWindow;                 // Set to break in Begin() call.
    VanGuiWindow*            CurrentWindow;                      // Window being drawn into
    VanGuiWindow*            HoveredWindow;                      // Window the mouse is hovering. Will typically catch mouse inputs.
    VanGuiWindow*            HoveredWindowUnderMovingWindow;     // Hovered window ignoring MovingWindow. Only set if MovingWindow is set.
    VanGuiWindow*            HoveredWindowBeforeClear;           // Window the mouse is hovering. Filled even with _NoMouse. This is currently useful for multi-context compositors.
    VanGuiWindow*            MovingWindow;                       // Track the window we clicked on (in order to preserve focus). The actual window that is moved is generally MovingWindow->RootWindow.
    VanGuiWindow*            WheelingWindow;                     // Track the window we started mouse-wheeling on. Until a timer elapse or mouse has moved, generally keep scrolling the same window even if during the course of scrolling the mouse ends up hovering a child window.
    VanVec2                  WheelingWindowRefMousePos;
    int                     WheelingWindowStartFrame;           // This may be set one frame before WheelingWindow is != nullptr
    int                     WheelingWindowScrolledFrame;
    float                   WheelingWindowReleaseTimer;
    VanVec2                  WheelingWindowWheelRemainder;
    VanVec2                  WheelingAxisAvg;

    // Item/widgets state and tracking information
    VanGuiID                 DebugDrawIdConflictsId;             // Set when we detect multiple items with the same identifier
    VanGuiID                 DebugHookIdInfoId;                  // Will call core hooks: DebugHookIdInfo() from GetID functions, used by ID Stack Tool [next HoveredId/ActiveId to not pull in an extra cache-line]
    VanGuiID                 HoveredId;                          // Hovered widget, filled during the frame
    VanGuiID                 HoveredIdPreviousFrame;
    int                     HoveredIdPreviousFrameItemCount;    // Count numbers of items using the same ID as last frame's hovered id
    float                   HoveredIdTimer;                     // Measure contiguous hovering time
    float                   HoveredIdNotActiveTimer;            // Measure contiguous hovering time where the item has not been active
    bool                    HoveredIdAllowOverlap;
    bool                    HoveredIdIsDisabled;                // At least one widget passed the rect test, but has been discarded by disabled flag or popup inhibit. May be true even if HoveredId == 0.
    bool                    ItemUnclipByLog;                    // Disable ItemAdd() clipping, essentially a memory-locality friendly copy of LogEnabled
    VanGuiID                 ActiveId;                           // Active widget
    VanGuiID                 ActiveIdIsAlive;                    // Active widget has been seen this frame (we can't use a bool as the ActiveId may change within the frame)
    float                   ActiveIdTimer;
    bool                    ActiveIdIsJustActivated;            // Set at the time of activation for one frame
    bool                    ActiveIdAllowOverlap;               // Active widget allows another widget to steal active id (generally for overlapping widgets, but not always)
    bool                    ActiveIdNoClearOnFocusLoss;         // Disable losing active id if the active id window gets unfocused.
    bool                    ActiveIdHasBeenPressedBefore;       // Track whether the active id led to a press (this is to allow changing between PressOnClick and PressOnRelease without pressing twice). Used by range_select branch.
    bool                    ActiveIdHasBeenEditedBefore;        // Was the value associated to the widget Edited over the course of the Active state.
    bool                    ActiveIdHasBeenEditedThisFrame;
    bool                    ActiveIdFromShortcut;
    VanS8                    ActiveIdMouseButton;
    VanGuiID                 ActiveIdDisabledId;                 // When clicking a disabled item we set ActiveId=window->MoveId to avoid interference with widget code. Actual item ID is stored here.
    VanVec2                  ActiveIdClickOffset;                // Clicked offset from upper-left corner, if applicable (currently only set by ButtonBehavior)
    VanGuiInputSource        ActiveIdSource;                     // Activating source: VanGuiInputSource_Mouse OR VanGuiInputSource_Keyboard OR VanGuiInputSource_Gamepad
    VanGuiWindow*            ActiveIdWindow;
    VanGuiID                 ActiveIdPreviousFrame;
    VanGuiDeactivatedItemData DeactivatedItemData;
    VanGuiDataTypeStorage    ActiveIdValueOnActivation;          // Backup of initial value at the time of activation. ONLY SET BY SPECIFIC WIDGETS: DragXXX and SliderXXX.
    VanGuiID                 LastActiveId;                       // Store the last non-zero ActiveId, useful for animation.
    float                   LastActiveIdTimer;                  // Store the last non-zero ActiveId timer since the beginning of activation, useful for animation.

    // Key/Input Ownership + Shortcut Routing system
    // - The idea is that instead of "eating" a given key, we can link to an owner.
    // - Input query can then read input by specifying VanGuiKeyOwner_Any (== 0), VanGuiKeyOwner_NoOwner (== -1) or a custom ID.
    // - Routing is requested ahead of time for a given chord (Key + Mods) and granted in NewFrame().
    double                  LastKeyModsChangeTime;              // Record the last time key mods changed (affect repeat delay when using shortcut logic)
    double                  LastKeyModsChangeFromNoneTime;      // Record the last time key mods changed away from being 0 (affect repeat delay when using shortcut logic)
    double                  LastKeyboardKeyPressTime;           // Record the last time a keyboard key (ignore mouse/gamepad ones) was pressed.
    VanBitArrayForNamedKeys  KeysMayBeCharInput;                 // Lookup to tell if a key can emit char input, see IsKeyChordPotentiallyCharInput(). sizeof() = 20 bytes
    VanGuiKeyOwnerData       KeysOwnerData[VanGuiKey_NamedKey_COUNT];
    VanGuiKeyRoutingTable    KeysRoutingTable;
    VanU32                   ActiveIdUsingNavDirMask;            // Active widget will want to read those nav move requests (e.g. can activate a button and move away from it)
    bool                    ActiveIdUsingAllKeyboardKeys;       // Active widget will want to read all keyboard keys inputs. (this is a shortcut for not taking ownership of 100+ keys, frequently used by drag operations)
    VanGuiKeyChord           DebugBreakInShortcutRouting;        // Set to break in SetShortcutRouting()/Shortcut() calls.
    //VanU32                 ActiveIdUsingNavInputMask;          // [OBSOLETE] Since (VANGUI_VERSION_NUM >= 18804) : 'g.ActiveIdUsingNavInputMask |= (1 << VanGuiNavInput_Cancel);' becomes --> 'SetKeyOwner(VanGuiKey_Escape, g.ActiveId) and/or SetKeyOwner(VanGuiKey_NavGamepadCancel, g.ActiveId);'

    // Next window/item data
    VanGuiID                 CurrentFocusScopeId;                // Value for currently appending items == g.FocusScopeStack.back(). Not to be mistaken with g.NavFocusScopeId.
    VanGuiItemFlags          CurrentItemFlags;                   // Value for currently appending items == g.ItemFlagsStack.back()
    VanGuiID                 DebugLocateId;                      // Storage for DebugLocateItemOnHover() feature: this is read by ItemAdd() so we keep it in a hot/cached location
    VanGuiNextItemData       NextItemData;                       // Storage for SetNextItem** functions
    VanGuiLastItemData       LastItemData;                       // Storage for last submitted item (setup by ItemAdd)
    VanGuiNextWindowData     NextWindowData;                     // Storage for SetNextWindow** functions
    bool                    DebugShowGroupRects;
    bool                    GcCompactAll;                       // Request full GC

    // Shared stacks
    VanGuiCol                        DebugFlashStyleColorIdx;    // (Keep close to ColorStack to share cache line)
    VanVector<VanGuiColorMod>         ColorStack;                 // Stack for PushStyleColor()/PopStyleColor() - inherited by Begin()
    VanVector<VanGuiStyleMod>         StyleVarStack;              // Stack for PushStyleVar()/PopStyleVar() - inherited by Begin()
    VanVector<VanFontStackData>       FontStack;                  // Stack for PushFont()/PopFont() - inherited by Begin()
    VanVector<VanGuiFocusScopeData>   FocusScopeStack;            // Stack for PushFocusScope()/PopFocusScope() - inherited by BeginChild(), pushed into by Begin()
    VanVector<VanGuiItemFlags>        ItemFlagsStack;             // Stack for PushItemFlag()/PopItemFlag() - inherited by Begin()
    VanVector<VanGuiGroupData>        GroupStack;                 // Stack for BeginGroup()/EndGroup() - not inherited by Begin()
    VanVector<VanGuiPopupData>        OpenPopupStack;             // Which popups are open (persistent)
    VanVector<VanGuiPopupData>        BeginPopupStack;            // Which level of BeginPopup() we are in (reset every frame)
    VanVector<VanGuiTreeNodeStackData>TreeNodeStack;              // Stack for TreeNode()

    // Docking
    VanGuiDockContext        DockContext;
    bool                     DockNodeWindowMenuHandler = false;  // Default handler for the _NodeWindowMenuButton.

    // Viewports
    VanVector<VanGuiViewportP*> Viewports;                       // Active viewports (always 1+, and generally 1 unless multi-viewports are enabled). Each viewports hold their copy of ImDrawData.
    float                    CurrentDpiScale             = 1.0f; // == CurrentViewport->DpiScale
    VanGuiViewportP*         CurrentViewport             = nullptr; // We track changes of viewport (happening in Begin) so we can call Platform_OnChangedViewport()
    VanGuiViewportP*         MouseViewport               = nullptr;
    VanGuiViewportP*         MouseLastHoveredViewport    = nullptr; // Last known viewport that was hovered by mouse (even if we are not hovering any viewport any longer) + honoring the _NoInputs flag.
    VanGuiID                 PlatformLastFocusedViewportId = 0;
    VanGuiPlatformMonitor    FallbackMonitor;                    // Virtual monitor used as fallback if backend doesn't provide monitor information.
    VanRect                  PlatformMonitorsFullWorkRect;       // Bounding box of all platform monitors
    int                      ViewportCreatedCount        = 0;    // Unique sequential creation counter (mostly for testing/debugging)
    int                      PlatformWindowsCreatedCount = 0;    // Unique sequential creation counter (mostly for testing/debugging)
    int                      ViewportFocusedStampCount   = 0;    // Every time the front-most window changes, we stamp its viewport with an incrementing counter

    // Keyboard/Gamepad Navigation
    bool                    NavCursorVisible;                   // Nav focus cursor/rectangle is visible? We hide it after a mouse click. We show it after a nav move.
    bool                    NavHighlightItemUnderNav;           // Disable mouse hovering highlight. Highlight navigation focused item instead of mouse hovered item.
    //bool                  NavDisableHighlight;                // Old name for !g.NavCursorVisible before 1.91.4 (2024/10/18). OPPOSITE VALUE (g.NavDisableHighlight == !g.NavCursorVisible)
    //bool                  NavDisableMouseHover;               // Old name for g.NavHighlightItemUnderNav before 1.91.1 (2024/10/18) this was called When user starts using keyboard/gamepad, we hide mouse hovering highlight until mouse is touched again.
    bool                    NavMousePosDirty;                   // When set we will update mouse position if io.ConfigNavMoveSetMousePos is set (not enabled by default)
    bool                    NavIdIsAlive;                       // Nav widget has been seen this frame ~~ NavRectRel is valid
    VanGuiID                 NavId;                              // Focused item for navigation
    VanGuiWindow*            NavWindow;                          // Focused window for navigation. Could be called 'FocusedWindow'
    VanGuiID                 NavFocusScopeId;                    // Focused focus scope (e.g. selection code often wants to "clear other items" when landing on an item of the same scope)
    VanGuiNavLayer           NavLayer;                           // Focused layer (main scrolling layer, or menu/title bar layer)
    VanGuiItemFlags          NavIdItemFlags;
    VanGuiID                 NavActivateId;                      // ~~ (g.ActiveId == 0) && (IsKeyPressed(VanGuiKey_Space) || IsKeyDown(VanGuiKey_Enter) || IsKeyPressed(VanGuiKey_NavGamepadActivate)) ? NavId : 0, also set when calling ActivateItemByID()
    VanGuiID                 NavActivateDownId;                  // ~~ IsKeyDown(VanGuiKey_Space) || IsKeyDown(VanGuiKey_Enter) || IsKeyDown(VanGuiKey_NavGamepadActivate) ? NavId : 0
    VanGuiID                 NavActivatePressedId;               // ~~ IsKeyPressed(VanGuiKey_Space) || IsKeyPressed(VanGuiKey_Enter) || IsKeyPressed(VanGuiKey_NavGamepadActivate) ? NavId : 0 (no repeat)
    VanGuiActivateFlags      NavActivateFlags;
    VanVector<VanGuiFocusScopeData> NavFocusRoute;                // Reversed copy focus scope stack for NavId (should contains NavFocusScopeId). This essentially follow the window->ParentWindowForFocusRoute chain.
    VanGuiID                 NavHighlightActivatedId;
    float                   NavHighlightActivatedTimer;
    VanGuiID                 NavOpenContextMenuItemId;
    VanGuiID                 NavOpenContextMenuWindowId;
    VanGuiID                 NavNextActivateId;                  // Set by ActivateItemByID(), queued until next frame.
    VanGuiActivateFlags      NavNextActivateFlags;
    VanGuiInputSource        NavInputSource;                     // Keyboard or Gamepad mode? THIS CAN ONLY BE VanGuiInputSource_Keyboard or VanGuiInputSource_Gamepad
    VanGuiSelectionUserData  NavLastValidSelectionUserData;      // Last valid data passed to SetNextItemSelectionUser(), or -1. For current window. Not reset when focusing an item that doesn't have selection data.
    VanS8                    NavCursorHideFrames;
    //VanGuiID               NavActivateInputId;                 // Removed in 1.89.4 (July 2023). This is now part of g.NavActivateId and sets g.NavActivateFlags |= VanGuiActivateFlags_PreferInput. See commit c9a53aa74, issue #5606.

    // Navigation: Init & Move Requests
    bool                    NavAnyRequest;                      // ~~ NavMoveRequest || NavInitRequest this is to perform early out in ItemAdd()
    bool                    NavInitRequest;                     // Init request for appearing window to select first item
    bool                    NavInitRequestFromMove;
    VanGuiNavItemData        NavInitResult;                      // Init request result (first item of the window, or one for which SetItemDefaultFocus() was called)
    bool                    NavMoveSubmitted;                   // Move request submitted, will process result on next NewFrame()
    bool                    NavMoveScoringItems;                // Move request submitted, still scoring incoming items
    bool                    NavMoveForwardToNextFrame;
    VanGuiNavMoveFlags       NavMoveFlags;
    VanGuiScrollFlags        NavMoveScrollFlags;
    VanGuiKeyChord           NavMoveKeyMods;
    VanGuiDir                NavMoveDir;                         // Direction of the move request (left/right/up/down)
    VanGuiDir                NavMoveDirForDebug;
    VanGuiDir                NavMoveClipDir;                     // FIXME-NAV: Describe the purpose of this better. Might want to rename?
    VanRect                  NavScoringRect;                     // Rectangle used for scoring, in screen space. Based of window->NavRectRel[], modified for directional navigation scoring.
    VanRect                  NavScoringNoClipRect;               // Some nav operations (such as PageUp/PageDown) enforce a region which clipper will attempt to always keep submitted. Unset/invalid if inverted.
    int                     NavScoringDebugCount;               // Metrics for debugging
    int                     NavTabbingDir;                      // Generally -1 or +1, 0 when tabbing without a nav id
    int                     NavTabbingCounter;                  // >0 when counting items for tabbing
    VanGuiNavItemData        NavMoveResultLocal;                 // Best move request candidate within NavWindow
    VanGuiNavItemData        NavMoveResultLocalVisible;          // Best move request candidate within NavWindow that are mostly visible (when using VanGuiNavMoveFlags_AlsoScoreVisibleSet flag)
    VanGuiNavItemData        NavMoveResultOther;                 // Best move request candidate within NavWindow's flattened hierarchy (when using VanGuiWindowFlags_NavFlattened flag)
    VanGuiNavItemData        NavTabbingResultFirst;              // First tabbing request candidate within NavWindow and flattened hierarchy

    // Navigation: record of last move request
    VanGuiID                 NavJustMovedFromFocusScopeId;       // Just navigated from this focus scope id (result of a successfully MoveRequest).
    VanGuiID                 NavJustMovedToId;                   // Just navigated to this id (result of a successfully MoveRequest).
    VanGuiID                 NavJustMovedToFocusScopeId;         // Just navigated to this focus scope id (result of a successfully MoveRequest).
    VanGuiKeyChord           NavJustMovedToKeyMods;
    bool                    NavJustMovedToIsTabbing;            // Copy of VanGuiNavMoveFlags_IsTabbing. Maybe we should store whole flags.
    bool                    NavJustMovedToHasSelectionData;     // Copy of move result's ItemFlags & VanGuiItemFlags_HasSelectionUserData). Maybe we should just store VanGuiNavItemData.

    // Navigation: extra config options (will be made public eventually)
    // - Tabbing (Tab, Shift+Tab) and Windowing (Ctrl+Tab, Ctrl+Shift+Tab) are enabled REGARDLESS of VanGuiConfigFlags_NavEnableKeyboard being set.
    // - Ctrl+Tab is reconfigurable because it is the only shortcut that may be polled when no window are focused. It also doesn't work e.g. Web platforms.
    bool                    ConfigNavEnableTabbing;             // = true. Enable tabbing (Tab, Shift+Tab). PLEASE LET ME KNOW IF YOU USE THIS.
    bool                    ConfigNavWindowingWithGamepad;      // = true. Enable Ctrl+Tab by holding VanGuiKey_GamepadFaceLeft (== VanGuiKey_NavGamepadMenu). When false, the button may still be used to toggle Menu layer.
    VanGuiKeyChord           ConfigNavWindowingKeyNext;          // = VanGuiMod_Ctrl | VanGuiKey_Tab (or VanGuiMod_Super | VanGuiKey_Tab on OS X). Set to 0 to disable. For reconfiguration (see #4828)
    VanGuiKeyChord           ConfigNavWindowingKeyPrev;          // = VanGuiMod_Ctrl | VanGuiMod_Shift | VanGuiKey_Tab (or VanGuiMod_Super | VanGuiMod_Shift | VanGuiKey_Tab on OS X)

    // Navigation: Windowing (Ctrl+Tab for list, or Menu button + keys or directional pads to move/resize)
    VanGuiWindow*            NavWindowingTarget;                 // Target window when doing Ctrl+Tab (or Pad Menu + FocusPrev/Next), this window is temporarily displayed top-most!
    VanGuiWindow*            NavWindowingTargetAnim;             // Record of last valid NavWindowingTarget until DimBgRatio and NavWindowingHighlightAlpha becomes 0.0f, so the fade-out can stay on it.
    VanGuiWindow*            NavWindowingListWindow;             // Internal window actually listing the Ctrl+Tab contents
    float                   NavWindowingTimer;
    float                   NavWindowingHighlightAlpha;
    VanGuiInputSource        NavWindowingInputSource;
    bool                    NavWindowingToggleLayer;            // Set while Alt or GamepadMenu is held, may be cleared by other operations, and processed when releasing the key.
    VanGuiKey                NavWindowingToggleKey;              // Keyboard/gamepad key used when toggling to menu layer.
    VanVec2                  NavWindowingAccumDeltaPos;
    VanVec2                  NavWindowingAccumDeltaSize;

    // Render
    float                   DimBgRatio;                         // 0.0..1.0 animation when fading in a dimming background (for modal window and Ctrl+Tab list)

    // Drag and Drop
    bool                    DragDropActive;
    bool                    DragDropWithinSource;               // Set when within a BeginDragDropXXX/EndDragDropXXX block for a drag source.
    bool                    DragDropWithinTarget;               // Set when within a BeginDragDropXXX/EndDragDropXXX block for a drag target.
    VanGuiDragDropFlags      DragDropSourceFlags;
    int                     DragDropSourceFrameCount;
    int                     DragDropMouseButton;
    VanGuiPayload            DragDropPayload;
    VanRect                  DragDropTargetRect;                 // Store rectangle of current target candidate (we favor small targets when overlapping)
    VanRect                  DragDropTargetClipRect;             // Store ClipRect at the time of item's drawing
    VanGuiID                 DragDropTargetId;
    VanGuiID                 DragDropTargetFullViewport;
    VanGuiDragDropFlags      DragDropAcceptFlagsCurr;
    VanGuiDragDropFlags      DragDropAcceptFlagsPrev;
    float                   DragDropAcceptIdCurrRectSurface;    // Target item surface (we resolve overlapping targets by prioritizing the smaller surface)
    VanGuiID                 DragDropAcceptIdCurr;               // Target item id (set at the time of accepting the payload)
    VanGuiID                 DragDropAcceptIdPrev;               // Target item id from previous frame (we need to store this to allow for overlapping drag and drop targets)
    int                     DragDropAcceptFrameCount;           // Last time a target expressed a desire to accept the source
    VanGuiID                 DragDropHoldJustPressedId;          // Set when holding a payload just made ButtonBehavior() return a press.
    VanVector<unsigned char> DragDropPayloadBufHeap;             // We don't expose the VanVector<> directly, VanGuiPayload only holds pointer+size
    unsigned char           DragDropPayloadBufLocal[16];        // Local buffer for small payloads

    // Clipper
    int                             ClipperTempDataStacked;
    VanVector<VanGuiListClipperData>  ClipperTempData;

    // Tables
    VanGuiTable*                     CurrentTable;
    VanGuiID                         DebugBreakInTable;          // Set to break in BeginTable() call.
    int                             TablesTempDataStacked;      // Temporary table data size (because we leave previous instances undestructed, we generally don't use TablesTempData.Size)
    VanVector<VanGuiTableTempData>    TablesTempData;             // Temporary table data (buffers reused/shared across instances, support nesting)
    VanPool<VanGuiTable>              Tables;                     // Persistent table data
    VanVector<float>                 TablesLastTimeActive;       // Last used timestamp of each tables (SOA, for efficient GC)
    VanVector<VanDrawChannel>         DrawChannelsTempMergeBuffer;

    // Tab bars
    VanGuiTabBar*                    CurrentTabBar;
    VanPool<VanGuiTabBar>             TabBars;
    VanVector<VanGuiPtrOrIndex>       CurrentTabBarStack;
    VanVector<VanGuiShrinkWidthItem>  ShrinkWidthBuffer;

    // Multi-Select state
    VanGuiBoxSelectState             BoxSelectState;
    VanGuiMultiSelectTempData*       CurrentMultiSelect;
    int                             MultiSelectTempDataStacked; // Temporary multi-select data size (because we leave previous instances undestructed, we generally don't use MultiSelectTempData.Size)
    VanVector<VanGuiMultiSelectTempData> MultiSelectTempData;
    VanPool<VanGuiMultiSelectState>   MultiSelectStorage;

    // Hover Delay system
    VanGuiID                 HoverItemDelayId;
    VanGuiID                 HoverItemDelayIdPreviousFrame;
    float                   HoverItemDelayTimer;                // Currently used by IsItemHovered()
    float                   HoverItemDelayClearTimer;           // Currently used by IsItemHovered(): grace time before g.TooltipHoverTimer gets cleared.
    VanGuiID                 HoverItemUnlockedStationaryId;      // Mouse has once been stationary on this item. Only reset after departing the item.
    VanGuiID                 HoverWindowUnlockedStationaryId;    // Mouse has once been stationary on this window. Only reset after departing the window.

    // Mouse state
    VanGuiMouseCursor        MouseCursor;
    float                   MouseStationaryTimer;               // Time the mouse has been stationary (with some loose heuristic)
    VanVec2                  MouseLastValidPos;

    // Widget state
    VanGuiInputTextState     InputTextState;
    VanGuiTextIndex          InputTextLineIndex;                 // Temporary storage
    VanGuiInputTextDeactivatedState InputTextDeactivatedState;
    VanFontBaked             InputTextPasswordFontBackupBaked;
    VanFontFlags             InputTextPasswordFontBackupFlags;
    VanGuiID                 InputTextReactivateId;              // ID of InputText to reactivate on next frame (for io.ConfigInputTextEnterKeepActive behavior)
    VanGuiID                 TempInputId;                        // Temporary text input when using Ctrl+Click on a slider, etc.
    VanGuiDataTypeStorage    DataTypeZeroValue;                  // 0 for all data types
    int                     BeginMenuDepth;
    int                     BeginComboDepth;
    VanGuiColorEditFlags     ColorEditOptions;                   // Store user options for color edit widgets
    VanGuiID                 ColorEditCurrentID;                 // Set temporarily while inside of the parent-most ColorEdit4/ColorPicker4 (because they call each others).
    VanGuiID                 ColorEditSavedID;                   // ID we are saving/restoring HS for
    float                   ColorEditSavedHue;                  // Backup of last Hue associated to LastColor, so we can restore Hue in lossy RGB<>HSV round trips
    float                   ColorEditSavedSat;                  // Backup of last Saturation associated to LastColor, so we can restore Saturation in lossy RGB<>HSV round trips
    VanU32                   ColorEditSavedColor;                // RGB value with alpha set to 0.
    VanVec4                  ColorPickerRef;                     // Initial/reference color at the time of opening the color picker.
    VanGuiComboPreviewData   ComboPreviewData;
    VanRect                  WindowResizeBorderExpectedRect;     // Expected border rect, switch to relative edit if moving
    bool                    WindowResizeRelativeMode;
    short                   ScrollbarSeekMode;                  // 0: scroll to clicked location, -1/+1: prev/next page.
    float                   ScrollbarClickDeltaToGrabCenter;    // When scrolling to mouse location: distance between mouse and center of grab box, normalized in parent space.
    float                   SliderGrabClickOffset;
    float                   SliderCurrentAccum;                 // Accumulated slider delta when using navigation controls.
    bool                    SliderCurrentAccumDirty;            // Has the accumulated slider delta changed since last time we tried to apply it?
    bool                    DragCurrentAccumDirty;
    float                   DragCurrentAccum;                   // Accumulator for dragging modification. Always high-precision, not rounded by end-user precision settings
    float                   DragSpeedDefaultRatio;              // If speed == 0.0f, uses (max-min) * DragSpeedDefaultRatio
    float                   DisabledAlphaBackup;                // Backup for style.Alpha for BeginDisabled()
    short                   DisabledStackSize;
    short                   TooltipOverrideCount;
    VanGuiWindow*            TooltipPreviousWindow;              // Window of last tooltip submitted during the frame
    VanVector<char>          ClipboardHandlerData;               // If no custom clipboard handler is defined
    VanVector<VanGuiID>       MenusIdSubmittedThisFrame;          // A list of menu IDs that were rendered at least once
    VanGuiTypingSelectState  TypingSelectState;                  // State for GetTypingSelectRequest()

    // Platform support
    VanGuiPlatformImeData    PlatformImeData;                    // Data updated by current frame. Will be applied at end of the frame. For some backends, this is required to have WantVisible=true in order to receive text message.
    VanGuiPlatformImeData    PlatformImeDataPrev;                // Previous frame data. When changed we call the platform_io.Platform_SetImeDataFn() handler.

    // Extensions
    // FIXME: We could provide an API to register one slot in an array held in VanGuiContext?
    VanVector<VanTextureData*> UserTextures;                      // List of textures created/managed by user or third-party extension. Automatically appended into platform_io.Textures[].

    // Settings
    bool                    SettingsLoaded;
    float                   SettingsDirtyTimer;                 // Save .ini Settings to memory when time reaches zero
    VanGuiTextBuffer         SettingsIniData;                    // In memory .ini settings
    VanVector<VanGuiSettingsHandler>      SettingsHandlers;       // List of .ini settings handlers
    VanChunkStream<VanGuiWindowSettings>  SettingsWindows;        // VanGuiWindow .ini settings entries
    VanChunkStream<VanGuiTableSettings>   SettingsTables;         // VanGuiTable .ini settings entries

    // Hooks
    VanVector<VanGuiContextHook>          Hooks;                  // Hooks for extensions (e.g. test engine)
    VanGuiID                             HookIdNext;             // Next available HookId
    VanGuiDemoMarkerCallback             DemoMarkerCallback;

    // Localization
    const char*             LocalizationTable[VanGuiLocKey_COUNT];

    // Capture/Logging
    bool                    LogEnabled;                         // Currently capturing
    bool                    LogLineFirstItem;
    VanGuiLogFlags           LogFlags;                           // Capture flags/type
    VanGuiWindow*            LogWindow;
    VanFileHandle            LogFile;                            // If != nullptr log to stdout/ file
    VanGuiTextBuffer         LogBuffer;                          // Accumulation buffer when log to clipboard. This is pointer so our GVanGui static constructor doesn't call heap allocators.
    const char*             LogNextPrefix;                      // See comment in LogSetNextTextDecoration(): doesn't copy underlying data, use carefully!
    const char*             LogNextSuffix;
    float                   LogLinePosY;
    int                     LogDepthRef;
    int                     LogDepthToExpand;
    int                     LogDepthToExpandDefault;            // Default/stored value for LogDepthMaxExpand if not specified in the LogXXX function call.

    // Error Handling
    VanGuiErrorCallback      ErrorCallback;                      // = nullptr. May be exposed in public API eventually.
    void*                   ErrorCallbackUserData;              // = nullptr
    VanVec2                  ErrorTooltipLockedPos;
    bool                    ErrorFirst;
    int                     ErrorCountCurrentFrame;             // [Internal] Number of errors submitted this frame.
    VanGuiErrorRecoveryState StackSizesInNewFrame;               // [Internal]
    VanGuiErrorRecoveryState*StackSizesInBeginForCurrentWindow;  // [Internal]

    // Debug Tools
    // (some of the highly frequently used data are interleaved in other structures above: DebugBreakXXX fields, DebugHookIdInfo, DebugLocateId etc.)
    int                     DebugDrawIdConflictsCount;          // Locked count (preserved when holding Ctrl)
    VanGuiDebugLogFlags      DebugLogFlags;
    VanGuiTextBuffer         DebugLogBuf;
    VanGuiTextIndex          DebugLogIndex;
    int                     DebugLogSkippedErrors;
    VanGuiDebugLogFlags      DebugLogAutoDisableFlags;
    VanU8                    DebugLogAutoDisableFrames;
    VanU8                    DebugLocateFrames;                  // For DebugLocateItemOnHover(). This is used together with DebugLocateId which is in a hot/cached spot above.
    bool                    DebugBreakInLocateId;               // Debug break in ItemAdd() call for g.DebugLocateId.
    VanGuiKeyChord           DebugBreakKeyChord;                 // = VanGuiKey_Pause
    VanS8                    DebugBeginReturnValueCullDepth;     // Cycle between 0..9 then wrap around.
    bool                    DebugItemPickerActive;              // Item picker is active (started with DebugStartItemPicker())
    VanU8                    DebugItemPickerMouseButton;
    VanGuiID                 DebugItemPickerBreakId;             // Will call VAN_DEBUG_BREAK() when encountering this ID
    float                   DebugFlashStyleColorTime;
    VanVec4                  DebugFlashStyleColorBackup;
    VanGuiMetricsConfig      DebugMetricsConfig;
    VanGuiDebugItemPathQuery DebugItemPathQuery;
    VanGuiIDStackTool        DebugIDStackTool;
    VanGuiDebugAllocInfo     DebugAllocInfo;
#if defined(VANGUI_DEBUG_HIGHLIGHT_ALL_ID_CONFLICTS) && !defined(VANGUI_DISABLE_DEBUG_TOOLS)
    VanGuiStorage            DebugDrawIdConflictsAliveCount;
    VanGuiStorage            DebugDrawIdConflictsHighlightSet;
#endif

    // Misc
    float                   FramerateSecPerFrame[60];           // Calculate estimate of framerate for user over the last 60 frames..
    int                     FramerateSecPerFrameIdx;
    int                     FramerateSecPerFrameCount;
    float                   FramerateSecPerFrameAccum;
    int                     WantCaptureMouseNextFrame;          // Explicit capture override via SetNextFrameWantCaptureMouse()/SetNextFrameWantCaptureKeyboard(). Default to -1.
    int                     WantCaptureKeyboardNextFrame;       // "
    int                     WantTextInputNextFrame;             // Copied in EndFrame() from g.PlatformImeData.WantTextInput. Needs to be set for some backends (SDL3) to emit character inputs.
    VanVector<char>          TempBuffer;                         // Temporary text buffer
    char                    TempKeychordName[64];

    VanGuiContext(VanFontAtlas* shared_font_atlas);
    ~VanGuiContext();
};

//-----------------------------------------------------------------------------
// [SECTION] VanGuiWindowTempData, VanGuiWindow
//-----------------------------------------------------------------------------

#define VANGUI_WINDOW_HARD_MIN_SIZE 4.0f

// Transient per-window data, reset at the beginning of the frame. This used to be called VanGuiDrawContext, hence the DC variable name in VanGuiWindow.
// (That's theory, in practice the delimitation between VanGuiWindow and VanGuiWindowTempData is quite tenuous and could be reconsidered..)
// (This doesn't need a constructor because we zero-clear it as part of VanGuiWindow and all frame-temporary data are setup on Begin)
struct VANGUI_API VanGuiWindowTempData
{
    // Layout
    VanVec2                  CursorPos;              // Current emitting position, in absolute coordinates.
    VanVec2                  CursorPosPrevLine;
    VanVec2                  CursorStartPos;         // Initial position after Begin(), generally ~ window position + WindowPadding.
    VanVec2                  CursorMaxPos;           // Used to implicitly calculate ContentSize at the beginning of next frame, for scrolling range and auto-resize. Always growing during the frame.
    VanVec2                  IdealMaxPos;            // Used to implicitly calculate ContentSizeIdeal at the beginning of next frame, for auto-resize only. Always growing during the frame.
    VanVec2                  CurrLineSize;
    VanVec2                  PrevLineSize;
    float                   CurrLineTextBaseOffset; // Baseline offset (0.0f by default on a new line, generally == style.FramePadding.y when a framed item has been added).
    float                   PrevLineTextBaseOffset;
    bool                    IsSameLine;
    bool                    IsSetPos;
    VanVec1                  Indent;                 // Indentation / start position from left of window (increased by TreePush/TreePop, etc.)
    VanVec1                  ColumnsOffset;          // Offset to the current column (if ColumnsCurrent > 0). FIXME: This and the above should be a stack to allow use cases like Tree->Column->Tree. Need revamp columns API.
    VanVec1                  GroupOffset;
    VanVec2                  CursorStartPosLossyness;// Record the loss of precision of CursorStartPos due to really large scrolling amount. This is used by clipper to compensate and fix the most common use case of large scroll area.

    // Keyboard/Gamepad navigation
    VanGuiNavLayer           NavLayerCurrent;        // Current layer, 0..31 (we currently only use 0..1)
    short                   NavLayersActiveMask;    // Which layers have been written to (result from previous frame)
    short                   NavLayersActiveMaskNext;// Which layers have been written to (accumulator for current frame)
    bool                    NavIsScrollPushableX;   // Set when current work location may be scrolled horizontally when moving left / right. This is generally always true UNLESS within a column.
    bool                    NavHideHighlightOneFrame;
    bool                    NavWindowHasScrollY;    // Set per window when scrolling can be used (== ScrollMax.y > 0.0f)

    // Miscellaneous
    bool                    MenuBarAppending;       // FIXME: Remove this
    VanVec2                  MenuBarOffset;          // MenuBarOffset.x is sort of equivalent of a per-layer CursorPos.x, saved/restored as we switch to the menu bar. The only situation when MenuBarOffset.y is > 0 if when (SafeAreaPadding.y > FramePadding.y), often used on TVs.
    VanGuiMenuColumns        MenuColumns;            // Simplified columns storage for menu items measurement
    int                     TreeDepth;              // Current tree depth.
    VanU32                   TreeHasStackDataDepthMask;      // Store whether given depth has VanGuiTreeNodeStackData data. Could be turned into a VanU64 if necessary.
    VanU32                   TreeRecordsClippedNodesY2Mask;  // Store whether we should keep recording Y2. Cleared when passing clip max. Equivalent TreeHasStackDataDepthMask value should always be set.
    VanVector<VanGuiWindow*>  ChildWindows;
    VanGuiStorage*           StateStorage;           // Current persistent per-window storage (store e.g. tree node open/close state)
    VanGuiOldColumns*        CurrentColumns;         // Current columns set
    int                     CurrentTableIdx;        // Current table index (into g.Tables)
    VanGuiLayoutType         LayoutType;
    VanGuiLayoutType         ParentLayoutType;       // Layout type of parent window at the time of Begin()
    VanU32                   ModalDimBgColor;
    VanGuiItemStatusFlags    WindowItemStatusFlags;
    VanGuiItemStatusFlags    ChildItemStatusFlags;

    // Local parameters stacks
    // We store the current settings outside of the vectors to increase memory locality (reduce cache misses). The vectors are rarely modified. Also it allows us to not heap allocate for short-lived windows which are not using those settings.
    float                   ItemWidth;              // Current item width (>0.0: width in pixels, <0.0: align xx pixels to the right of window).
    float                   ItemWidthDefault;
    float                   TextWrapPos;            // Current text wrap pos.
    VanVector<float>         ItemWidthStack;         // Store item widths to restore (attention: .back() is not == ItemWidth)
    VanVector<float>         TextWrapPosStack;       // Store text wrap pos to restore (attention: .back() is not == TextWrapPos)
};

// Storage for one window
struct VANGUI_API VanGuiWindow
{
    VanGuiContext*           Ctx;                                // Parent UI context (needs to be set explicitly by parent).
    char*                   Name;                               // Window name, owned by the window.
    VanGuiID                 ID;                                 // == VanHashStr(Name)
    VanGuiWindowFlags        Flags;                              // See enum VanGuiWindowFlags_
    VanGuiChildFlags         ChildFlags;                         // Set when window is a child window. See enum VanGuiChildFlags_
    VanGuiViewportP*         Viewport;                           // Always set in Begin(). Inactive windows may have a nullptr value here if their viewport was discarded.
    VanVec2                  Pos;                                // Position (always rounded-up to nearest pixel)
    VanVec2                  Size;                               // Current size (==SizeFull or collapsed title bar size)
    VanVec2                  SizeFull;                           // Size when non collapsed
    VanVec2                  ContentSize;                        // Size of contents/scrollable client area (calculated from the extents reach of the cursor) from previous frame. Does not include window decoration or window padding.
    VanVec2                  ContentSizeIdeal;
    VanVec2                  ContentSizeExplicit;                // Size of contents/scrollable client area explicitly request by the user via SetNextWindowContentSize().
    VanVec2                  WindowPadding;                      // Window padding at the time of Begin().
    float                   WindowRounding;                     // Window rounding at the time of Begin(). May be clamped lower to avoid rendering artifacts with title bar, menu bar etc.
    float                   WindowBorderSize;                   // Window border size at the time of Begin().
    float                   TitleBarHeight, MenuBarHeight;      // Note that those used to be function before 2024/05/28. If you have old code calling TitleBarHeight() you can change it to TitleBarHeight.
    float                   DecoOuterSizeX1, DecoOuterSizeY1;   // Left/Up offsets. Sum of non-scrolling outer decorations (X1 generally == 0.0f. Y1 generally = TitleBarHeight + MenuBarHeight). Locked during Begin().
    float                   DecoOuterSizeX2, DecoOuterSizeY2;   // Right/Down offsets (X2 generally == ScrollbarSize.x, Y2 == ScrollbarSizes.y).
    float                   DecoInnerSizeX1, DecoInnerSizeY1;   // Applied AFTER/OVER InnerRect. Specialized for Tables as they use specialized form of clipping and frozen rows/columns are inside InnerRect (and not part of regular decoration sizes).
    int                     NameBufLen;                         // Size of buffer storing Name. May be larger than strlen(Name)!
    VanGuiID                 MoveId;                             // == window->GetID("#MOVE")
    VanGuiID                 ChildId;                            // ID of corresponding item in parent window (for navigation to return from child window to parent window)
    VanGuiID                 PopupId;                            // ID in the popup stack when this window is used as a popup/menu (because we use generic Name/ID for recycling)
    VanVec2                  Scroll;
    VanVec2                  ScrollMax;
    VanVec2                  ScrollTarget;                       // target scroll position. stored as cursor position with scrolling canceled out, so the highest point is always 0.0f. (FLT_MAX for no change)
    VanVec2                  ScrollTargetCenterRatio;            // 0.0f = scroll so that target position is at top, 0.5f = scroll so that target position is centered
    VanVec2                  ScrollTargetEdgeSnapDist;           // 0.0f = no snapping, >0.0f snapping threshold
    VanVec2                  ScrollbarSizes;                     // Size taken by each scrollbars on their smaller axis. Pay attention! ScrollbarSizes.x == width of the vertical scrollbar, ScrollbarSizes.y = height of the horizontal scrollbar.
    bool                    ScrollbarX, ScrollbarY;             // Are scrollbars visible?
    bool                    ScrollbarXStabilizeEnabled;         // Was ScrollbarX previously auto-stabilized?
    VanU8                    ScrollbarXStabilizeToggledHistory;  // Used to stabilize scrollbar visibility in case of feedback loops
    bool                    Active;                             // Set to true on Begin(), unless Collapsed
    bool                    WasActive;
    bool                    WantClose;                          // Set by the platform backend (viewport close button) to request the window be closed
    bool                    WriteAccessed;                      // Set to true when any widget access the current window
    bool                    Collapsed;                          // Set when collapsing window to become only title-bar
    bool                    WantCollapseToggle;
    bool                    SkipItems;                          // Set when items can safely be all clipped (e.g. window not visible or collapsed)
    bool                    SkipRefresh;                        // [EXPERIMENTAL] Reuse previous frame drawn contents, Begin() returns false.
    bool                    Appearing;                          // Set during the frame where the window is appearing (or re-appearing)
    bool                    Hidden;                             // Do not display (== HiddenFrames*** > 0)
    bool                    IsFallbackWindow;                   // Set on the "Debug##Default" window.
    bool                    IsExplicitChild;                    // Set when passed _ChildWindow, left to false by BeginDocked()
    bool                    HasCloseButton;                     // Set when the window has a close button (p_open != nullptr)
    signed char             ResizeBorderHovered;                // Current border being hovered for resize (-1: none, otherwise 0-3)
    signed char             ResizeBorderHeld;                   // Current border being held for resize (-1: none, otherwise 0-3)
    short                   BeginCount;                         // Number of Begin() during the current frame (generally 0 or 1, 1+ if appending via multiple Begin/End pairs)
    short                   BeginCountPreviousFrame;            // Number of Begin() during the previous frame
    short                   BeginOrderWithinParent;             // Begin() order within immediate parent window, if we are a child window. Otherwise 0.
    short                   BeginOrderWithinContext;            // Begin() order within entire vangui context. This is mostly used for debugging submission order related issues.
    short                   FocusOrder;                         // Order within WindowsFocusOrder[], altered when windows are focused.
    VanGuiDir                AutoPosLastDirection;
    VanS8                    AutoFitFramesX, AutoFitFramesY;
    bool                    AutoFitOnlyGrows;
    VanS8                    HiddenFramesCanSkipItems;           // Hide the window for N frames
    VanS8                    HiddenFramesCannotSkipItems;        // Hide the window for N frames while allowing items to be submitted so we can measure their size
    VanS8                    HiddenFramesForRenderOnly;          // Hide the window until frame N at Render() time only
    VanS8                    DisableInputsFrames;                // Disable window interactions for N frames
    VanGuiWindowBgClickFlags BgClickFlags : 8;                   // Configure behavior of click+dragging on window bg/void or over items. Default sets by io.ConfigWindowsMoveFromTitleBarOnly. If you use this please report in #3379.
    VanGuiCond               SetWindowPosAllowFlags : 8;         // store acceptable condition flags for SetNextWindowPos() use.
    VanGuiCond               SetWindowSizeAllowFlags : 8;        // store acceptable condition flags for SetNextWindowSize() use.
    VanGuiCond               SetWindowCollapsedAllowFlags : 8;   // store acceptable condition flags for SetNextWindowCollapsed() use.
    VanGuiCond               SetWindowDockAllowFlags : 8;        // store acceptable condition flags for SetNextWindowDockID() use.
    VanVec2                  SetWindowPosVal;                    // store window position when using a non-zero Pivot (position set needs to be processed when we know the window size)
    VanVec2                  SetWindowPosPivot;                  // store window pivot for positioning. VanVec2(0, 0) when positioning from top-left corner; VanVec2(0.5f, 0.5f) for centering; VanVec2(1, 1) for bottom right.

    VanVector<VanGuiID>       IDStack;                            // ID stack. ID are hashes seeded with the value at the top of the stack. (In theory this should be in the TempData structure)
    VanGuiWindowTempData     DC;                                 // Temporary per-window data, reset at the beginning of the frame. This used to be called VanGuiDrawContext, hence the "DC" variable name.

    // The best way to understand what those rectangles are is to use the 'Metrics->Tools->Show Windows Rectangles' viewer.
    // The main 'OuterRect', omitted as a field, is window->Rect().
    VanRect                  OuterRectClipped;                   // == Window->Rect() just after setup in Begin(). == window->Rect() for root window.
    VanRect                  InnerRect;                          // Inner rectangle (omit title bar, menu bar, scroll bar)
    VanRect                  InnerClipRect;                      // == InnerRect shrunk by WindowPadding*0.5f on each side, clipped within viewport or parent clip rect.
    VanRect                  WorkRect;                           // Initially covers the whole scrolling region. Reduced by containers e.g columns/tables when active. Shrunk by WindowPadding*1.0f on each side. This is meant to replace ContentRegionRect over time (from 1.71+ onward).
    VanRect                  ParentWorkRect;                     // Backup of WorkRect before entering a container such as columns/tables. Used by e.g. SpanAllColumns functions to easily access. Stacked containers are responsible for maintaining this. // FIXME-WORKRECT: Could be a stack?
    VanRect                  ClipRect;                           // Current clipping/scissoring rectangle, evolve as we are using PushClipRect(), etc. == DrawList->clip_rect_stack.back().
    VanRect                  ContentRegionRect;                  // FIXME: This is currently confusing/misleading. It is essentially WorkRect but not handling of scrolling. We currently rely on it as right/bottom aligned sizing operation need some size to rely on.
    VanVec2ih                HitTestHoleSize;                    // Define an optional rectangular hole where mouse will pass-through the window.
    VanVec2ih                HitTestHoleOffset;

    int                     LastFrameActive;                    // Last frame number the window was Active.
    float                   LastTimeActive;                     // Last timestamp the window was Active (using float as we don't need high precision there)
    VanGuiStorage            StateStorage;
    VanVector<VanGuiOldColumns> ColumnsStorage;
    float                   FontWindowScale;                    // User scale multiplier per-window, via SetWindowFontScale()
    float                   FontWindowScaleParents;
    float                   FontRefSize;                        // This is a copy of window->CalcFontSize() at the time of Begin(), trying to phase out CalcFontSize() especially as it may be called on non-current window.
    int                     SettingsOffset;                     // Offset into SettingsWindows[] (offsets are always valid as we only grow the array from the back)

    VanDrawList*             DrawList;                           // == &DrawListInst (for backward compatibility reason with code using vangui_internal.h we keep this a pointer)
    VanDrawList              DrawListInst;
    VanGuiWindow*            ParentWindow;                       // If we are a child _or_ popup _or_ docked window, this is pointing to our parent. Otherwise nullptr.
    VanGuiWindow*            ParentWindowInBeginStack;
    VanGuiWindow*            RootWindow;                         // Point to ourself or first ancestor that is not a child window. Doesn't cross through popups/dock nodes.
    VanGuiWindow*            RootWindowPopupTree;                // Point to ourself or first ancestor that is not a child window. Cross through popups parent<>child.
    VanGuiWindow*            RootWindowForTitleBarHighlight;     // Point to ourself or first ancestor which will display TitleBgActive color when this window is active.
    VanGuiWindow*            RootWindowForNav;                   // Point to ourself or first ancestor which doesn't have the NavFlattened flag.
    VanGuiWindow*            ParentWindowForFocusRoute;          // Set to manual link a window to its logical parent so that Shortcut() chain are honored (e.g. Tool linked to Document)

    VanGuiWindow*            NavLastChildNavWindow;              // When going to the menu bar, we remember the child window we came from. (This could probably be made implicit if we kept g.Windows sorted by last focused including child window.)
    VanGuiID                 NavLastIds[VanGuiNavLayer_COUNT];    // Last known NavId for this window, per layer (0/1)
    VanRect                  NavRectRel[VanGuiNavLayer_COUNT];    // Reference rectangle, in window relative space
    VanVec2                  NavPreferredScoringPosRel[VanGuiNavLayer_COUNT]; // Preferred X/Y position updated when moving on a given axis, reset to FLT_MAX.
    VanGuiID                 NavRootFocusScopeId;                // Focus Scope ID at the time of Begin()

    int                     MemoryDrawListIdxCapacity;          // Backup of last idx/vtx count, so when waking up the window we can preallocate and avoid iterative alloc/copy
    int                     MemoryDrawListVtxCapacity;
    bool                    MemoryCompacted;                    // Set when window extraneous data have been garbage collected

    // Docking
    VanGuiDockNode*          DockNode                = nullptr; // Which node are we docked into. Important: Prefer testing DockIsActive in many cases as this will still be set when the dock node is hidden.
    VanGuiDockNode*          DockNodeAsHost          = nullptr; // Which node are we owning (for parent windows)
    VanGuiID                 DockId                  = 0;       // Backup of last valid DockNode->ID, so single window remember their dock node id even when they are not bound any more
    VanGuiWindowClass        WindowClass;                       // Set by SetNextWindowClass() — controls docking compatibility and viewport routing for this window.
    VanGuiCond               DockTabItemStatusFlags  = 0;       // Store item status flags (e.g. HoveredRect)
    VanRect                  DockTabItemRect;                   // Store item rect
    short                    DockOrder               = -1;      // Order of the last time the window was visible within its DockNode. This is used to reorder windows that are reappearing on the same frame. Same value between windows that were active and windows that were none are possible.
    bool                     DockIsActive            : 1 = false; // When docking artifacts are actually visible. When this is set, DockNode is guaranteed to be != nullptr. ~~ (DockNode != nullptr) && (DockNode->Windows.Size > 1).
    bool                     DockNodeIsVisible       : 1 = false;
    bool                     DockTabIsVisible        : 1 = false; // Is our window visible this frame? ~~ is the corresponding tab selected?
    bool                     DockTabWantClose        : 1 = false;

public:
    VanGuiWindow(VanGuiContext* context, const char* name);
    ~VanGuiWindow();

    VanGuiID     GetID(const char* str, const char* str_end = nullptr);
    VanGuiID     GetID(const void* ptr);
    VanGuiID     GetID(int n);
    VanGuiID     GetIDFromPos(const VanVec2& p_abs);
    VanGuiID     GetIDFromRectangle(const VanRect& r_abs);

    // We don't use g.FontSize because the window may be != g.CurrentWindow.
    VanRect      Rect() const            { return VanRect(Pos.x, Pos.y, Pos.x + Size.x, Pos.y + Size.y); }
    VanRect      TitleBarRect() const    { return VanRect(Pos, VanVec2(Pos.x + SizeFull.x, Pos.y + TitleBarHeight)); }
    VanRect      MenuBarRect() const     { float y1 = Pos.y + TitleBarHeight; return VanRect(Pos.x, y1, Pos.x + SizeFull.x, y1 + MenuBarHeight); }

    // [OBSOLETE] VanGuiWindow::CalcFontSize() was removed in 1.92.0 because error-prone/misleading. You can use window->FontRefSize for a copy of g.FontSize at the time of the last Begin() call for this window.
    //float     CalcFontSize() const    { VanGuiContext& g = *Ctx; return g.FontSizeBase * FontWindowScale * FontWindowScaleParents;
};

//-----------------------------------------------------------------------------
// [SECTION] Tab bar, Tab item support
//-----------------------------------------------------------------------------

// Extend VanGuiTabBarFlags_
enum VanGuiTabBarFlagsPrivate_
{
    VanGuiTabBarFlags_DockNode                   = 1 << 20,  // Part of a dock node [we don't use this in the master branch but it facilitate branch syncing to keep this around]
    VanGuiTabBarFlags_IsFocused                  = 1 << 21,
    VanGuiTabBarFlags_SaveSettings               = 1 << 22,  // FIXME: Settings are handled by the docking system, this only request the tab bar to mark settings dirty when reordering tabs
};

// Extend VanGuiTabItemFlags_
enum VanGuiTabItemFlagsPrivate_
{
    VanGuiTabItemFlags_SectionMask_              = VanGuiTabItemFlags_Leading | VanGuiTabItemFlags_Trailing,
    VanGuiTabItemFlags_NoCloseButton             = 1 << 20,  // Track whether p_open was set or not (we'll need this info on the next frame to recompute ContentWidth during layout)
    VanGuiTabItemFlags_Button                    = 1 << 21,  // Used by TabItemButton, change the tab item behavior to mimic a button
    VanGuiTabItemFlags_Invisible                 = 1 << 22,  // To reserve space e.g. with VanGuiTabItemFlags_Leading
    //VanGuiTabItemFlags_Unsorted                = 1 << 23,  // [Docking] Trailing tabs with the _Unsorted flag will be sorted based on the DockOrder of their Window.
};

// Storage for one active tab item (sizeof() 40 bytes)
struct VanGuiTabItem
{
    VanGuiID             ID;
    VanGuiTabItemFlags   Flags;
    int                 LastFrameVisible;
    int                 LastFrameSelected;      // This allows us to infer an ordered list of the last activated tabs with little maintenance
    float               Offset;                 // Position relative to beginning of tab bar
    float               Width;                  // Width currently displayed
    float               ContentWidth;           // Width of label + padding, stored during BeginTabItem() call (misnamed as "Content" would normally imply width of label only)
    float               RequestedWidth;         // Width optionally requested by caller, -1.0f is unused
    VanS32               NameOffset;             // When Window==nullptr, offset to name within parent VanGuiTabBar::TabsNames
    VanS16               BeginOrder;             // BeginTabItem() order, used to re-order tabs after toggling VanGuiTabBarFlags_Reorderable
    VanS16               IndexDuringLayout;      // Index only used during TabBarLayout(). Tabs gets reordered so 'Tabs[n].IndexDuringLayout == n' but may mismatch during additions.
    bool                WantClose;              // Marked as closed by SetTabItemClosed()

    VanGuiTabItem()      { memset((void*)this, 0, sizeof(*this)); LastFrameVisible = LastFrameSelected = -1; RequestedWidth = -1.0f; NameOffset = -1; BeginOrder = IndexDuringLayout = -1; } // POD-safe zero-init
};

// Storage for a tab bar (sizeof() 160 bytes)
struct VANGUI_API VanGuiTabBar
{
    VanGuiWindow*        Window;
    VanVector<VanGuiTabItem> Tabs;
    VanGuiTabBarFlags    Flags;
    VanGuiID             ID;                     // Zero for tab-bars used by docking
    VanGuiID             SelectedTabId;          // Selected tab/window
    VanGuiID             NextSelectedTabId;      // Next selected tab/window. Will also trigger a scrolling animation
    VanGuiID             NextScrollToTabId;
    VanGuiID             VisibleTabId;           // Can occasionally be != SelectedTabId (e.g. when previewing contents for Ctrl+Tab preview)
    int                 CurrFrameVisible;
    int                 PrevFrameVisible;
    VanRect              BarRect;
    float               BarRectPrevWidth;       // Backup of previous width. When width change we enforce keep horizontal scroll on focused tab.
    float               CurrTabsContentsHeight;
    float               PrevTabsContentsHeight; // Record the height of contents submitted below the tab bar
    float               WidthAllTabs;           // Actual width of all tabs (locked during layout)
    float               WidthAllTabsIdeal;      // Ideal width if all tabs were visible and not clipped
    float               ScrollingAnim;
    float               ScrollingTarget;
    float               ScrollingTargetDistToVisibility;
    float               ScrollingSpeed;
    float               ScrollingRectMinX;
    float               ScrollingRectMaxX;
    float               SeparatorMinX;
    float               SeparatorMaxX;
    VanGuiID             ReorderRequestTabId;
    VanS16               ReorderRequestOffset;
    VanS8                BeginCount;
    bool                WantLayout;
    bool                VisibleTabWasSubmitted;
    bool                TabsAddedNew;           // Set to true when a new tab item or button has been added to the tab bar during last frame
    bool                ScrollButtonEnabled;
    VanS16               TabsActiveCount;        // Number of tabs submitted this frame.
    VanS16               LastTabItemIdx;         // Index of last BeginTabItem() tab for use by EndTabItem()
    float               ItemSpacingY;
    VanVec2              FramePadding;           // style.FramePadding locked at the time of BeginTabBar()
    VanVec2              BackupCursorPos;
    VanGuiTextBuffer     TabsNames;              // For non-docking tab bar we re-append names in a contiguous buffer.

    VanGuiTabBar();
};

//-----------------------------------------------------------------------------
// [SECTION] Table support
//-----------------------------------------------------------------------------

#define VAN_COL32_DISABLE                VAN_COL32(0,0,0,1)   // Special sentinel code which cannot be used as a regular color.
#define VANGUI_TABLE_MAX_COLUMNS         512                 // Arbitrary "safety" maximum, may be lifted in the future if needed. Must fit in VanGuiTableColumnIdx/VanGuiTableDrawChannelIdx.

// [Internal] sizeof() ~ 112
// We use the terminology "Enabled" to refer to a column that is not Hidden by user/api.
// We use the terminology "Clipped" to refer to a column that is out of sight because of scrolling/clipping.
// This is in contrast with some user-facing api such as IsItemVisible() / IsRectVisible() which use "Visible" to mean "not clipped".
struct VanGuiTableColumn
{
    VanGuiTableColumnFlags   Flags;                          // Flags after some patching (not directly same as provided by user). See VanGuiTableColumnFlags_
    float                   WidthGiven;                     // Final/actual width visible == (MaxX - MinX), locked in TableUpdateLayout(). May be > WidthRequest to honor minimum width, may be < WidthRequest to honor shrinking columns down in tight space.
    float                   MinX;                           // Absolute positions
    float                   MaxX;
    float                   WidthRequest;                   // Master width absolute value when !(Flags & _WidthStretch). When Stretch this is derived every frame from StretchWeight in TableUpdateLayout()
    float                   WidthAuto;                      // Automatic width
    float                   WidthMax;                       // Maximum width (FIXME: overwritten by each instance)
    float                   StretchWeight;                  // Master width weight when (Flags & _WidthStretch). Often around ~1.0f initially.
    float                   InitStretchWeightOrWidth;       // Value passed to TableSetupColumn(). For Width it is a content width (_without padding_).
    VanRect                  ClipRect;                       // Clipping rectangle for the column
    VanGuiID                 UserID;                         // Optional, value passed to TableSetupColumn()
    float                   WorkMinX;                       // Contents region min ~(MinX + CellPaddingX + CellSpacingX1) == cursor start position when entering column
    float                   WorkMaxX;                       // Contents region max ~(MaxX - CellPaddingX - CellSpacingX2)
    float                   ItemWidth;                      // Current item width for the column, preserved across rows
    float                   ContentMaxXFrozen;              // Contents maximum position for frozen rows (apart from headers), from which we can infer content width.
    float                   ContentMaxXUnfrozen;
    float                   ContentMaxXHeadersUsed;         // Contents maximum position for headers rows (regardless of freezing). TableHeader() automatically softclip itself + report ideal desired size, to avoid creating extraneous draw calls
    float                   ContentMaxXHeadersIdeal;
    VanS16                   NameOffset;                     // Offset into parent ColumnsNames[]
    VanGuiTableColumnIdx     DisplayOrder;                   // Index within Table's IndexToDisplayOrder[] (column may be reordered by users)
    VanGuiTableColumnIdx     IndexWithinEnabledSet;          // Index within enabled/visible set (<= IndexToDisplayOrder)
    VanGuiTableColumnIdx     PrevEnabledColumn;              // Index of prev enabled/visible column within Columns[], -1 if first enabled/visible column
    VanGuiTableColumnIdx     NextEnabledColumn;              // Index of next enabled/visible column within Columns[], -1 if last enabled/visible column
    VanGuiTableColumnIdx     SortOrder;                      // Index of this column within sort specs, -1 if not sorting on this column, 0 for single-sort, may be >0 on multi-sort
    VanGuiTableDrawChannelIdx DrawChannelCurrent;            // Index within DrawSplitter.Channels[]
    VanGuiTableDrawChannelIdx DrawChannelFrozen;             // Draw channels for frozen rows (often headers)
    VanGuiTableDrawChannelIdx DrawChannelUnfrozen;           // Draw channels for unfrozen rows
    bool                    IsEnabled;                      // IsUserEnabled && (Flags & VanGuiTableColumnFlags_Disabled) == 0
    bool                    IsUserEnabled;                  // Is the column not marked Hidden by the user? (unrelated to being off view, e.g. clipped by scrolling).
    bool                    IsUserEnabledNextFrame;
    bool                    IsVisibleX;                     // Is actually in view (e.g. overlapping the host window clipping rectangle, not scrolled).
    bool                    IsVisibleY;
    bool                    IsRequestOutput;                // Return value for TableSetColumnIndex() / TableNextColumn(): whether we request user to output contents or not.
    bool                    IsSkipItems;                    // Do we want item submissions to this column to be completely ignored (no layout will happen).
    bool                    IsPreserveWidthAuto;
    VanS8                    NavLayerCurrent;                // VanGuiNavLayer in 1 byte
    VanU8                    AutoFitQueue;                   // Queue of 8 values for the next 8 frames to request auto-fit
    VanU8                    CannotSkipItemsQueue;           // Queue of 8 values for the next 8 frames to disable Clipped/SkipItem
    VanU8                    SortDirection : 2;              // VanGuiSortDirection_Ascending or VanGuiSortDirection_Descending
    VanU8                    SortDirectionsAvailCount : 2;   // Number of available sort directions (0 to 3)
    VanU8                    SortDirectionsAvailMask : 4;    // Mask of available sort directions (1-bit each)
    VanU8                    SortDirectionsAvailList;        // Ordered list of available sort directions (2-bits each, total 8-bits)

    VanGuiTableColumn()
    {
        memset((void*)this, 0, sizeof(*this));
        StretchWeight = WidthRequest = -1.0f;
        NameOffset = -1;
        DisplayOrder = IndexWithinEnabledSet = -1;
        PrevEnabledColumn = NextEnabledColumn = -1;
        SortOrder = -1;
        SortDirection = VanGuiSortDirection_None;
        DrawChannelCurrent = DrawChannelFrozen = DrawChannelUnfrozen = (VanU8)-1;
    }
};

// Transient cell data stored per row.
// sizeof() ~ 6 bytes
struct VanGuiTableCellData
{
    VanU32                       BgColor;    // Actual color
    VanGuiTableColumnIdx         Column;     // Column number
};

// Parameters for TableAngledHeadersRowEx()
// This may end up being refactored for more general purpose.
// sizeof() ~ 12 bytes
struct VanGuiTableHeaderData
{
    VanGuiTableColumnIdx         Index;      // Column index
    VanU32                       TextColor;
    VanU32                       BgColor0;
    VanU32                       BgColor1;
};

// Per-instance data that needs preserving across frames (seemingly most others do not need to be preserved aside from debug needs. Does that means they could be moved to VanGuiTableTempData?)
// sizeof() ~ 24 bytes
struct VanGuiTableInstanceData
{
    VanGuiID                     TableInstanceID;
    float                       LastOuterHeight;            // Outer height from last frame
    float                       LastTopHeadersRowHeight;    // Height of first consecutive header rows from last frame (FIXME: this is used assuming consecutive headers are in same frozen set)
    float                       LastFrozenHeight;           // Height of frozen section from last frame
    int                         HoveredRowLast;             // Index of row which was hovered last frame.
    int                         HoveredRowNext;             // Index of row hovered this frame, set after encountering it.

    VanGuiTableInstanceData()    { TableInstanceID = 0; LastOuterHeight = LastTopHeadersRowHeight = LastFrozenHeight = 0.0f; HoveredRowLast = HoveredRowNext = -1; }
};

// sizeof() ~ 592 bytes + heap allocs described in TableBeginInitMemory()
struct VANGUI_API VanGuiTable
{
    VanGuiID                     ID;
    VanGuiTableFlags             Flags;
    void*                       RawData;                    // Single allocation to hold Columns[], DisplayOrderToIndex[], and RowCellData[]
    VanGuiTableTempData*         TempData;                   // Transient data while table is active. Point within g.CurrentTableStack[]
    VanSpan<VanGuiTableColumn>    Columns;                    // Point within RawData[]
    VanSpan<VanGuiTableColumnIdx> DisplayOrderToIndex;        // Point within RawData[]. Store display order of columns (when not reordered, the values are 0...Count-1)
    VanSpan<VanGuiTableCellData>  RowCellData;                // Point within RawData[]. Store cells background requests for current row.
    VanBitArrayPtr               EnabledMaskByDisplayOrder;  // Column DisplayOrder -> IsEnabled map
    VanBitArrayPtr               EnabledMaskByIndex;         // Column Index -> IsEnabled map (== not hidden by user/api) in a format adequate for iterating column without touching cold data
    VanBitArrayPtr               VisibleMaskByIndex;         // Column Index -> IsVisibleX|IsVisibleY map (== not hidden by user/api && not hidden by scrolling/cliprect)
    VanGuiTableFlags             SettingsLoadedFlags;        // Which data were loaded from the .ini file (e.g. when order is not altered we won't save order)
    int                         SettingsOffset;             // Offset in g.SettingsTables
    int                         LastFrameActive;
    int                         ColumnsCount;               // Number of columns declared in BeginTable()
    int                         CurrentRow;
    int                         CurrentColumn;
    VanS16                       InstanceCurrent;            // Count of BeginTable() calls with same ID in the same frame (generally 0). This is a little bit similar to BeginCount for a window, but multiple tables with the same ID are multiple tables, they are just synced.
    VanS16                       InstanceInteracted;         // Mark which instance (generally 0) of the same ID is being interacted with
    float                       RowPosY1;
    float                       RowPosY2;
    float                       RowMinHeight;               // Height submitted to TableNextRow()
    float                       RowCellPaddingY;            // Top and bottom padding. Reloaded during row change.
    float                       RowTextBaseline;
    float                       RowIndentOffsetX;
    VanGuiTableRowFlags          RowFlags : 16;              // Current row flags, see VanGuiTableRowFlags_
    VanGuiTableRowFlags          LastRowFlags : 16;
    int                         RowBgColorCounter;          // Counter for alternating background colors (can be fast-forwarded by e.g clipper), not same as CurrentRow because header rows typically don't increase this.
    VanU32                       RowBgColor[2];              // Background color override for current row.
    VanU32                       BorderColorStrong;
    VanU32                       BorderColorLight;
    float                       BorderX1;
    float                       BorderX2;
    float                       HostIndentX;
    float                       MinColumnWidth;
    float                       OuterPaddingX;
    float                       CellPaddingX;               // Padding from each borders. Locked in BeginTable()/Layout.
    float                       CellSpacingX1;              // Spacing between non-bordered cells. Locked in BeginTable()/Layout.
    float                       CellSpacingX2;
    float                       InnerWidth;                 // User value passed to BeginTable(), see comments at the top of BeginTable() for details.
    float                       ColumnsGivenWidth;          // Sum of current column width
    float                       ColumnsAutoFitWidth;        // Sum of ideal column width in order nothing to be clipped, used for auto-fitting and content width submission in outer window
    float                       ColumnsStretchSumWeights;   // Sum of weight of all enabled stretching columns
    float                       ResizedColumnNextWidth;
    float                       ResizeLockMinContentsX2;    // Lock minimum contents width while resizing down in order to not create feedback loops. But we allow growing the table.
    float                       RefScale;                   // Reference scale to be able to rescale columns on font/dpi changes.
    float                       AngledHeadersHeight;        // Set by TableAngledHeadersRow(), used in TableUpdateLayout()
    float                       AngledHeadersSlope;         // Set by TableAngledHeadersRow(), used in TableUpdateLayout()
    VanRect                      OuterRect;                  // Note: for non-scrolling table, OuterRect.Max.y is often FLT_MAX until EndTable(), unless a height has been specified in BeginTable().
    VanRect                      InnerRect;                  // InnerRect but without decoration. As with OuterRect, for non-scrolling tables, InnerRect.Max.y is "
    VanRect                      WorkRect;
    VanRect                      InnerClipRect;
    VanRect                      BgClipRect;                 // We use this to cpu-clip cell background color fill, evolve during the frame as we cross frozen rows boundaries
    VanRect                      Bg0ClipRectForDrawCmd;      // Actual VanDrawCmd clip rect for BG0/1 channel. This tends to be == OuterWindow->ClipRect at BeginTable() because output in BG0/BG1 is cpu-clipped
    VanRect                      Bg2ClipRectForDrawCmd;      // Actual VanDrawCmd clip rect for BG2 channel. This tends to be a correct, tight-fit, because output to BG2 are done by widgets relying on regular ClipRect.
    VanRect                      HostClipRect;               // This is used to check if we can eventually merge our columns draw calls into the current draw call of the current window.
    VanRect                      HostBackupInnerClipRect;    // Backup of InnerWindow->ClipRect during PushTableBackground()/PopTableBackground()
    VanGuiWindow*                OuterWindow;                // Parent window for the table
    VanGuiWindow*                InnerWindow;                // Window holding the table data (== OuterWindow or a child window)
    VanGuiTextBuffer             ColumnsNames;               // Contiguous buffer holding columns names
    VanDrawListSplitter*         DrawSplitter;               // Shortcut to TempData->DrawSplitter while in table. Isolate draw commands per columns to avoid switching clip rect constantly
    VanGuiTableInstanceData      InstanceDataFirst;
    VanVector<VanGuiTableInstanceData>    InstanceDataExtra;  // FIXME-OPT: Using a small-vector pattern would be good.
    VanGuiTableColumnSortSpecs   SortSpecsSingle;
    VanVector<VanGuiTableColumnSortSpecs> SortSpecsMulti;     // FIXME-OPT: Using a small-vector pattern would be good.
    VanGuiTableSortSpecs         SortSpecs;                  // Public facing sorts specs, this is what we return in TableGetSortSpecs()
    VanGuiTableColumnIdx         SortSpecsCount;
    VanGuiTableColumnIdx         ColumnsEnabledCount;        // Number of enabled columns (<= ColumnsCount)
    VanGuiTableColumnIdx         ColumnsEnabledFixedCount;   // Number of enabled columns using fixed width (<= ColumnsCount)
    VanGuiTableColumnIdx         DeclColumnsCount;           // Count calls to TableSetupColumn()
    VanGuiTableColumnIdx         AngledHeadersCount;         // Count columns with angled headers
    VanGuiTableColumnIdx         HoveredColumnBody;          // Index of column whose visible region is being hovered. Important: == ColumnsCount when hovering empty region after the right-most column!
    VanGuiTableColumnIdx         HoveredColumnBorder;        // Index of column whose right-border is being hovered (for resizing).
    VanGuiTableColumnIdx         HighlightColumnHeader;      // Index of column which should be highlighted.
    VanGuiTableColumnIdx         AutoFitSingleColumn;        // Index of single column requesting auto-fit.
    VanGuiTableColumnIdx         ResizedColumn;              // Index of column being resized. Reset when InstanceCurrent==0.
    VanGuiTableColumnIdx         LastResizedColumn;          // Index of column being resized from previous frame.
    VanGuiTableColumnIdx         HeldHeaderColumn;           // Index of column header being held.
    VanGuiTableColumnIdx         LastHeldHeaderColumn;       // Index of column header being held from previous frame.
    VanGuiTableColumnIdx         ReorderColumn;              // Index of column being reordered. (not cleared)
    VanGuiTableColumnIdx         ReorderColumnDstOrder;      // Requested display order of column being reordered.
    VanGuiTableColumnIdx         LeftMostEnabledColumn;      // Index of left-most non-hidden column.
    VanGuiTableColumnIdx         RightMostEnabledColumn;     // Index of right-most non-hidden column.
    VanGuiTableColumnIdx         LeftMostStretchedColumn;    // Index of left-most stretched column.
    VanGuiTableColumnIdx         RightMostStretchedColumn;   // Index of right-most stretched column.
    VanGuiTableColumnIdx         ContextPopupColumn;         // Column right-clicked on, of -1 if opening context menu from a neutral/empty spot
    VanGuiTableColumnIdx         FreezeRowsRequest;          // Requested frozen rows count
    VanGuiTableColumnIdx         FreezeRowsCount;            // Actual frozen row count (== FreezeRowsRequest, or == 0 when no scrolling offset)
    VanGuiTableColumnIdx         FreezeColumnsRequest;       // Requested frozen columns count
    VanGuiTableColumnIdx         FreezeColumnsCount;         // Actual frozen columns count (== FreezeColumnsRequest, or == 0 when no scrolling offset)
    VanGuiTableColumnIdx         RowCellDataCurrent;         // Index of current RowCellData[] entry in current row
    VanGuiTableDrawChannelIdx    DummyDrawChannel;           // Redirect non-visible columns here.
    VanGuiTableDrawChannelIdx    Bg2DrawChannelCurrent;      // For Selectable() and other widgets drawing across columns after the freezing line. Index within DrawSplitter.Channels[]
    VanGuiTableDrawChannelIdx    Bg2DrawChannelUnfrozen;
    VanS8                        NavLayer;                   // VanGuiNavLayer at the time of BeginTable().
    bool                        IsLayoutLocked;             // Set by TableUpdateLayout() which is called when beginning the first row.
    bool                        IsInsideRow;                // Set when inside TableBeginRow()/TableEndRow().
    bool                        IsInitializing;
    bool                        IsSortSpecsDirty;
    bool                        IsUsingHeaders;             // Set when the first row had the VanGuiTableRowFlags_Headers flag.
    bool                        IsContextPopupOpen;         // Set when default context menu is open (also see: ContextPopupColumn, InstanceInteracted).
    bool                        DisableDefaultContextMenu;  // Disable default context menu. You may submit your own using TableBeginContextMenuPopup()/EndPopup()
    bool                        IsSettingsRequestLoad;
    bool                        IsSettingsDirty;            // Set when table settings have changed and needs to be reported into VanGuiTableSettings data.
    bool                        IsDefaultDisplayOrder;      // Set when display order is unchanged from default (DisplayOrder contains 0...Count-1)
    bool                        IsResetAllRequest;
    bool                        IsResetDisplayOrderRequest;
    bool                        IsUnfrozenRows;             // Set when we got past the frozen row.
    bool                        IsDefaultSizingPolicy;      // Set if user didn't explicitly set a sizing policy in BeginTable()
    bool                        IsActiveIdAliveBeforeTable;
    bool                        IsActiveIdInTable;
    bool                        HasScrollbarYCurr;          // Whether ANY instance of this table had a vertical scrollbar during the current frame.
    bool                        HasScrollbarYPrev;          // Whether ANY instance of this table had a vertical scrollbar during the previous.
    bool                        MemoryCompacted;
    bool                        HostSkipItems;              // Backup of InnerWindow->SkipItem at the end of BeginTable(), because we will overwrite InnerWindow->SkipItem on a per-column basis

    VanGuiTable()                { memset((void*)this, 0, sizeof(*this)); LastFrameActive = -1; } // VanVector<>/VanGuiTextBuffer members are zero-init-safe (Data=nullptr, Size=0, Capacity=0)
    ~VanGuiTable()               { VAN_FREE(RawData); }
};

// Transient data that are only needed between BeginTable() and EndTable(), those buffers are shared (1 per level of stacked table).
// - Accessing those requires chasing an extra pointer so for very frequently used data we leave them in the main table structure.
// - We also leave out of this structure data that tend to be particularly useful for debugging/metrics.
// FIXME-TABLE: more transient data could be stored in a stacked VanGuiTableTempData: e.g. SortSpecs.
// sizeof() ~ 136 bytes.
struct VANGUI_API VanGuiTableTempData
{
    VanGuiID                     WindowID;                   // Shortcut to g.Tables[TableIndex]->OuterWindow->ID.
    int                         TableIndex;                 // Index in g.Tables.Buf[] pool
    float                       LastTimeActive;             // Last timestamp this structure was used
    float                       AngledHeadersExtraWidth;    // Used in EndTable()
    VanVector<VanGuiTableHeaderData> AngledHeadersRequests;   // Used in TableAngledHeadersRow()

    VanVec2                      UserOuterSize;              // outer_size.x passed to BeginTable()
    VanDrawListSplitter          DrawSplitter;

    VanRect                      HostBackupWorkRect;         // Backup of InnerWindow->WorkRect at the end of BeginTable()
    VanRect                      HostBackupParentWorkRect;   // Backup of InnerWindow->ParentWorkRect at the end of BeginTable()
    VanVec2                      HostBackupPrevLineSize;     // Backup of InnerWindow->DC.PrevLineSize at the end of BeginTable()
    VanVec2                      HostBackupCurrLineSize;     // Backup of InnerWindow->DC.CurrLineSize at the end of BeginTable()
    VanVec2                      HostBackupCursorMaxPos;     // Backup of InnerWindow->DC.CursorMaxPos at the end of BeginTable()
    VanVec1                      HostBackupColumnsOffset;    // Backup of OuterWindow->DC.ColumnsOffset at the end of BeginTable()
    float                       HostBackupItemWidth;        // Backup of OuterWindow->DC.ItemWidth at the end of BeginTable()
    int                         HostBackupItemWidthStackSize;//Backup of OuterWindow->DC.ItemWidthStack.Size at the end of BeginTable()

    VanGuiTableTempData()        { memset((void*)this, 0, sizeof(*this)); LastTimeActive = -1.0f; } // VanVector<>/VanDrawListSplitter members are zero-init-safe (Data=nullptr, Size=0, Capacity=0)
};

// sizeof() ~ 16
struct VanGuiTableColumnSettings
{
    float                   WidthOrWeight;
    VanGuiID                 UserID;
    VanGuiTableColumnIdx     Index;
    VanGuiTableColumnIdx     DisplayOrder;
    VanGuiTableColumnIdx     SortOrder;
    VanU8                    SortDirection : 2;
    VanS8                    IsEnabled : 2; // "Visible" in ini file
    VanU8                    IsStretch : 1;

    VanGuiTableColumnSettings()
    {
        WidthOrWeight = 0.0f;
        UserID = 0;
        Index = -1;
        DisplayOrder = SortOrder = -1;
        SortDirection = VanGuiSortDirection_None;
        IsEnabled = -1;
        IsStretch = 0;
    }
};

// This is designed to be stored in a single VanChunkStream (1 header followed by N VanGuiTableColumnSettings, etc.)
struct VanGuiTableSettings
{
    VanGuiID                     ID;                     // Set to 0 to invalidate/delete the setting
    VanGuiTableFlags             SaveFlags;              // Indicate data we want to save using the Resizable/Reorderable/Sortable/Hideable flags (could be using its own flags..)
    float                       RefScale;               // Reference scale to be able to rescale columns on font/dpi changes.
    VanGuiTableColumnIdx         ColumnsCount;
    VanGuiTableColumnIdx         ColumnsCountMax;        // Maximum number of columns this settings instance can store, we can recycle a settings instance with lower number of columns but not higher
    bool                        WantApply;              // Set when loaded from .ini data (to enable merging/loading .ini data into an already running context)

    VanGuiTableSettings()        { memset((void*)this, 0, sizeof(*this)); } // POD-safe zero-init
    VanGuiTableColumnSettings*   GetColumnSettings()     { return reinterpret_cast<VanGuiTableColumnSettings*>(this + 1); }
};

//-----------------------------------------------------------------------------
// [SECTION] VanGui internal API
// No guarantee of forward compatibility here!
//-----------------------------------------------------------------------------

namespace VanGui
{
    // Windows
    // We should always have a CurrentWindow in the stack (there is an implicit "Debug" window)
    // If this ever crashes because g.CurrentWindow is nullptr, it means that either:
    // - VanGui::NewFrame() has never been called, which is illegal.
    // - You are calling VanGui functions after VanGui::EndFrame()/VanGui::Render() and before the next VanGui::NewFrame(), which is also illegal.
    VANGUI_API VanGuiIO&         GetIO(VanGuiContext* ctx);
    VANGUI_API VanGuiPlatformIO& GetPlatformIO(VanGuiContext* ctx);
    inline    float         GetScale()                  { VanGuiContext& g = *GVanGui; return g.Style._MainScale; } // FIXME-DPI: I don't want to formalize this just yet. Because reasons. Please don't use.
    inline    VanGuiWindow*  GetCurrentWindowRead()      { VanGuiContext& g = *GVanGui; return g.CurrentWindow; }
    inline    VanGuiWindow*  GetCurrentWindow()          { VanGuiContext& g = *GVanGui; g.CurrentWindow->WriteAccessed = true; return g.CurrentWindow; }
    VANGUI_API VanGuiWindow*  FindWindowByID(VanGuiID id);
    VANGUI_API VanGuiWindow*  FindWindowByName(const char* name);
    VANGUI_API void          UpdateWindowParentAndRootLinks(VanGuiWindow* window, VanGuiWindowFlags flags, VanGuiWindow* parent_window);
    VANGUI_API void          UpdateWindowSkipRefresh(VanGuiWindow* window);
    VANGUI_API VanVec2        CalcWindowNextAutoFitSize(VanGuiWindow* window);
    VANGUI_API bool          IsWindowChildOf(VanGuiWindow* window, VanGuiWindow* potential_parent, bool popup_hierarchy);
    VANGUI_API bool          IsWindowInBeginStack(VanGuiWindow* window);
    VANGUI_API bool          IsWindowWithinBeginStackOf(VanGuiWindow* window, VanGuiWindow* potential_parent);
    VANGUI_API bool          IsWindowAbove(VanGuiWindow* potential_above, VanGuiWindow* potential_below);
    VANGUI_API bool          IsWindowNavFocusable(VanGuiWindow* window);
    VANGUI_API void          SetWindowPos(VanGuiWindow* window, const VanVec2& pos, VanGuiCond cond = 0);
    VANGUI_API void          SetWindowSize(VanGuiWindow* window, const VanVec2& size, VanGuiCond cond = 0);
    VANGUI_API void          SetWindowCollapsed(VanGuiWindow* window, bool collapsed, VanGuiCond cond = 0);
    VANGUI_API void          SetWindowHitTestHole(VanGuiWindow* window, const VanVec2& pos, const VanVec2& size);
    VANGUI_API void          SetWindowHiddenAndSkipItemsForCurrentFrame(VanGuiWindow* window);
    inline void             SetWindowParentWindowForFocusRoute(VanGuiWindow* window, VanGuiWindow* parent_window) { window->ParentWindowForFocusRoute = parent_window; }
    inline VanRect           WindowRectAbsToRel(VanGuiWindow* window, const VanRect& r) { VanVec2 off = window->DC.CursorStartPos; return VanRect(r.Min.x - off.x, r.Min.y - off.y, r.Max.x - off.x, r.Max.y - off.y); }
    inline VanRect           WindowRectRelToAbs(VanGuiWindow* window, const VanRect& r) { VanVec2 off = window->DC.CursorStartPos; return VanRect(r.Min.x + off.x, r.Min.y + off.y, r.Max.x + off.x, r.Max.y + off.y); }
    inline VanVec2           WindowPosAbsToRel(VanGuiWindow* window, const VanVec2& p)  { VanVec2 off = window->DC.CursorStartPos; return VanVec2(p.x - off.x, p.y - off.y); }
    inline VanVec2           WindowPosRelToAbs(VanGuiWindow* window, const VanVec2& p)  { VanVec2 off = window->DC.CursorStartPos; return VanVec2(p.x + off.x, p.y + off.y); }

    // Windows: Display Order and Focus Order
    VANGUI_API void          FocusWindow(VanGuiWindow* window, VanGuiFocusRequestFlags flags = 0);
    VANGUI_API void          FocusTopMostWindowUnderOne(VanGuiWindow* under_this_window, VanGuiWindow* ignore_window, VanGuiViewport* filter_viewport, VanGuiFocusRequestFlags flags);
    VANGUI_API void          BringWindowToFocusFront(VanGuiWindow* window);
    VANGUI_API void          BringWindowToDisplayFront(VanGuiWindow* window);
    VANGUI_API void          BringWindowToDisplayBack(VanGuiWindow* window);
    VANGUI_API void          BringWindowToDisplayBehind(VanGuiWindow* window, VanGuiWindow* above_window);
    VANGUI_API int           FindWindowDisplayIndex(VanGuiWindow* window);
    VANGUI_API VanGuiWindow*  FindBottomMostVisibleWindowWithinBeginStack(VanGuiWindow* window);

    // Windows: Idle, Refresh Policies [EXPERIMENTAL]
    VANGUI_API void          SetNextWindowRefreshPolicy(VanGuiWindowRefreshFlags flags);

    // Fonts, drawing
    VANGUI_API void          RegisterUserTexture(VanTextureData* tex); // Register external texture. EXPERIMENTAL.
    VANGUI_API void          UnregisterUserTexture(VanTextureData* tex);
    VANGUI_API void          RegisterFontAtlas(VanFontAtlas* atlas);
    VANGUI_API void          UnregisterFontAtlas(VanFontAtlas* atlas);
    VANGUI_API void          SetCurrentFont(VanFont* font, float font_size_before_scaling, float font_size_after_scaling);
    VANGUI_API void          UpdateCurrentFontSize(float restore_font_size_after_scaling);
    VANGUI_API void          SetFontRasterizerDensity(float rasterizer_density);
    inline float            GetFontRasterizerDensity() { return GVanGui->FontRasterizerDensity; }
    inline float            GetRoundedFontSize(float size) { return VAN_ROUND(size); }
    VANGUI_API VanFont*       GetDefaultFont();
    VANGUI_API void          PushPasswordFont();
    VANGUI_API void          PopPasswordFont();
    inline VanDrawList*      GetForegroundDrawList(VanGuiWindow* window) { VAN_UNUSED(window); return GetForegroundDrawList(); } // This seemingly unnecessary wrapper simplifies compatibility between the 'master' and 'docking' branches.
    VANGUI_API VanDrawList*   GetBackgroundDrawList(VanGuiViewport* viewport);                     // get background draw list for the given viewport. this draw list will be the first rendering one. Useful to quickly draw shapes/text behind dear vangui contents.
    VANGUI_API VanDrawList*   GetForegroundDrawList(VanGuiViewport* viewport);                     // get foreground draw list for the given viewport. this draw list will be the last rendered one. Useful to quickly draw shapes/text over dear vangui contents.
    VANGUI_API void          AddDrawListToDrawDataEx(VanDrawData* draw_data, VanVector<VanDrawList*>* out_list, VanDrawList* draw_list);

    // Init
    VANGUI_API void          Initialize();
    VANGUI_API void          Shutdown();    // Since 1.60 this is a _private_ function. You can call DestroyContext() to destroy the context created by CreateContext().

    // Context name & generic context hooks
    VANGUI_API void          SetContextName(VanGuiContext* ctx, const char* name);
    VANGUI_API VanGuiID       AddContextHook(VanGuiContext* ctx, const VanGuiContextHook* hook);
    VANGUI_API void          RemoveContextHook(VanGuiContext* ctx, VanGuiID hook_to_remove);
    VANGUI_API void          CallContextHooks(VanGuiContext* ctx, VanGuiContextHookType type);

    // NewFrame
    VANGUI_API void          UpdateInputEvents(bool trickle_fast_inputs);
    VANGUI_API void          UpdateHoveredWindowAndCaptureFlags(const VanVec2& mouse_pos);
    VANGUI_API void          FindHoveredWindowEx(const VanVec2& pos, bool find_first_and_in_any_viewport, VanGuiWindow** out_hovered_window, VanGuiWindow** out_hovered_window_under_moving_window);
    VANGUI_API void          StartMouseMovingWindow(VanGuiWindow* window);
    VANGUI_API void          StopMouseMovingWindow();
    VANGUI_API void          UpdateMouseMovingWindowNewFrame();
    VANGUI_API void          UpdateMouseMovingWindowEndFrame();

    // Viewports
    VANGUI_API VanGuiViewport*   GetWindowViewport(); // Defined in vangui_docking.cpp / vangui-docking.ixx
    VANGUI_API void          ScaleWindowsInViewport(VanGuiViewportP* viewport, float scale);
    VANGUI_API void          SetWindowViewport(VanGuiWindow* window, VanGuiViewportP* viewport);

    // Settings
    VANGUI_API void                  MarkIniSettingsDirty();
    VANGUI_API void                  MarkIniSettingsDirty(VanGuiWindow* window);
    VANGUI_API void                  ClearIniSettings();
    VANGUI_API void                  AddSettingsHandler(const VanGuiSettingsHandler* handler);
    VANGUI_API void                  RemoveSettingsHandler(const char* type_name);
    VANGUI_API VanGuiSettingsHandler* FindSettingsHandler(const char* type_name);

    // Settings - Windows
    VANGUI_API VanGuiWindowSettings*  CreateNewWindowSettings(const char* name);
    VANGUI_API VanGuiWindowSettings*  FindWindowSettingsByID(VanGuiID id);
    VANGUI_API VanGuiWindowSettings*  FindWindowSettingsByWindow(VanGuiWindow* window);
    VANGUI_API void                  ClearWindowSettings(const char* name);

    // Localization
    VANGUI_API void          LocalizeRegisterEntries(const VanGuiLocEntry* entries, int count);
    inline const char*      LocalizeGetMsg(VanGuiLocKey key) { VanGuiContext& g = *GVanGui; const char* msg = g.LocalizationTable[key]; return msg ? msg : "*Missing Text*"; }

    // Scrolling
    VANGUI_API void          SetScrollX(VanGuiWindow* window, float scroll_x);
    VANGUI_API void          SetScrollY(VanGuiWindow* window, float scroll_y);
    VANGUI_API void          SetScrollFromPosX(VanGuiWindow* window, float local_x, float center_x_ratio);
    VANGUI_API void          SetScrollFromPosY(VanGuiWindow* window, float local_y, float center_y_ratio);

    // Early work-in-progress API (ScrollToItem() will become public)
    VANGUI_API void          ScrollToItem(VanGuiScrollFlags flags = 0);
    VANGUI_API void          ScrollToRect(VanGuiWindow* window, const VanRect& rect, VanGuiScrollFlags flags = 0);
    VANGUI_API VanVec2        ScrollToRectEx(VanGuiWindow* window, const VanRect& rect, VanGuiScrollFlags flags = 0);
//#ifndef VANGUI_DISABLE_OBSOLETE_FUNCTIONS
    inline void             ScrollToBringRectIntoView(VanGuiWindow* window, const VanRect& rect) { ScrollToRect(window, rect, VanGuiScrollFlags_KeepVisibleEdgeY); }
//#endif

    // Basic Accessors
    inline VanGuiItemStatusFlags GetItemStatusFlags() { VanGuiContext& g = *GVanGui; return g.LastItemData.StatusFlags; }
    inline VanGuiID          GetActiveID()   { VanGuiContext& g = *GVanGui; return g.ActiveId; }
    inline VanGuiID          GetFocusID()    { VanGuiContext& g = *GVanGui; return g.NavId; }
    VANGUI_API void          SetActiveID(VanGuiID id, VanGuiWindow* window);
    VANGUI_API void          SetFocusID(VanGuiID id, VanGuiWindow* window);
    VANGUI_API void          ClearActiveID();
    VANGUI_API VanGuiID       GetHoveredID();
    VANGUI_API void          SetHoveredID(VanGuiID id);
    VANGUI_API void          KeepAliveID(VanGuiID id);
    VANGUI_API void          MarkItemEdited(VanGuiID id);     // Mark data associated to given item as "edited", used by IsItemDeactivatedAfterEdit() function.
    VANGUI_API void          PushOverrideID(VanGuiID id);     // Push given value as-is at the top of the ID stack (whereas PushID combines old and new hashes)
    VANGUI_API VanGuiID       GetIDWithSeed(const char* str_id_begin, const char* str_id_end, VanGuiID seed);
    VANGUI_API VanGuiID       GetIDWithSeed(int n, VanGuiID seed);

    // Basic Helpers for widget code
    VANGUI_API void          ItemSize(const VanVec2& size, float text_baseline_y = -1.0f);
    inline void             ItemSize(const VanRect& bb, float text_baseline_y = -1.0f) { ItemSize(bb.GetSize(), text_baseline_y); } // FIXME: This is a misleading API since we expect CursorPos to be bb.Min.
    VANGUI_API bool          ItemAdd(const VanRect& bb, VanGuiID id, const VanRect* nav_bb = nullptr, VanGuiItemFlags extra_flags = 0);
    VANGUI_API bool          ItemHoverable(const VanRect& bb, VanGuiID id, VanGuiItemFlags item_flags);
    VANGUI_API bool          IsWindowContentHoverable(VanGuiWindow* window, VanGuiHoveredFlags flags = 0);
    VANGUI_API bool          IsClippedEx(const VanRect& bb, VanGuiID id);
    VANGUI_API void          SetLastItemData(VanGuiID item_id, VanGuiItemFlags item_flags, VanGuiItemStatusFlags status_flags, const VanRect& item_rect);
    VANGUI_API VanVec2        CalcItemSize(VanVec2 size, float default_w, float default_h);
    VANGUI_API float         CalcWrapWidthForPos(const VanVec2& pos, float wrap_pos_x);
    VANGUI_API void          PushMultiItemsWidths(int components, float width_full);
    VANGUI_API void          ShrinkWidths(VanGuiShrinkWidthItem* items, int count, float width_excess, float width_min);
    VANGUI_API void          CalcClipRectVisibleItemsY(const VanRect& clip_rect, const VanVec2& pos, float items_height, int* out_visible_start, int* out_visible_end);

    // Parameter stacks (shared)
    VANGUI_API const VanGuiStyleVarInfo* GetStyleVarInfo(VanGuiStyleVar idx);
    VANGUI_API void          BeginDisabledOverrideReenable();
    VANGUI_API void          EndDisabledOverrideReenable();

    // Logging/Capture
    VANGUI_API void          LogBegin(VanGuiLogFlags flags, int auto_open_depth);         // -> BeginCapture() when we design v2 api, for now stay under the radar by using the old name.
    VANGUI_API void          LogToBuffer(int auto_open_depth = -1);                      // Start logging/capturing to internal buffer
    VANGUI_API void          LogRenderedText(const VanVec2* ref_pos, const char* text, const char* text_end = nullptr);
    VANGUI_API void          LogSetNextTextDecoration(const char* prefix, const char* suffix);

    // Childs
    VANGUI_API bool          BeginChildEx(const char* name, VanGuiID id, const VanVec2& size_arg, VanGuiChildFlags child_flags, VanGuiWindowFlags window_flags);
    VANGUI_API VanGuiWindow*  FindFrontMostVisibleChildWindow(VanGuiWindow* window);

    // Popups, Modals
    VANGUI_API bool          BeginPopupEx(VanGuiID id, VanGuiWindowFlags extra_window_flags);
    VANGUI_API bool          BeginPopupMenuEx(VanGuiID id, const char* label, VanGuiWindowFlags extra_window_flags);
    VANGUI_API void          OpenPopupEx(VanGuiID id, VanGuiPopupFlags popup_flags = VanGuiPopupFlags_None);
    VANGUI_API void          ClosePopupToLevel(int remaining, bool restore_focus_to_window_under_popup);
    VANGUI_API void          ClosePopupsOverWindow(VanGuiWindow* ref_window, bool restore_focus_to_window_under_popup);
    VANGUI_API void          ClosePopupsExceptModals();
    VANGUI_API bool          IsPopupOpen(VanGuiID id, VanGuiPopupFlags popup_flags);
    VANGUI_API VanRect        GetPopupAllowedExtentRect(VanGuiWindow* window);
    VANGUI_API VanGuiWindow*  GetTopMostPopupModal();
    VANGUI_API VanGuiWindow*  GetTopMostAndVisiblePopupModal();
    VANGUI_API VanGuiWindow*  FindBlockingModal(VanGuiWindow* window);
    VANGUI_API VanVec2        FindBestWindowPosForPopup(VanGuiWindow* window);
    VANGUI_API VanVec2        FindBestWindowPosForPopupEx(const VanVec2& ref_pos, const VanVec2& size, VanGuiDir* last_dir, const VanRect& r_outer, const VanRect& r_avoid, VanGuiPopupPositionPolicy policy);
    VANGUI_API VanGuiMouseButton GetMouseButtonFromPopupFlags(VanGuiPopupFlags flags);
    VANGUI_API bool          IsPopupOpenRequestForItem(VanGuiPopupFlags flags, VanGuiID id);
    VANGUI_API bool          IsPopupOpenRequestForWindow(VanGuiPopupFlags flags);

    // Tooltips
    VANGUI_API bool          BeginTooltipEx(VanGuiTooltipFlags tooltip_flags, VanGuiWindowFlags extra_window_flags);
    VANGUI_API bool          BeginTooltipHidden();

    // Menus
    VANGUI_API bool          BeginViewportSideBar(const char* name, VanGuiViewport* viewport, VanGuiDir dir, float size, VanGuiWindowFlags window_flags);
    VANGUI_API bool          BeginMenuEx(const char* label, const char* icon, bool enabled = true);
    VANGUI_API bool          MenuItemEx(const char* label, const char* icon, const char* shortcut = nullptr, bool selected = false, bool enabled = true);

    // Combos
    VANGUI_API bool          BeginComboPopup(VanGuiID popup_id, const VanRect& bb, VanGuiComboFlags flags);
    VANGUI_API bool          BeginComboPreview();
    VANGUI_API void          EndComboPreview();

    // Keyboard/Gamepad Navigation
    VANGUI_API void          NavInitWindow(VanGuiWindow* window, bool force_reinit);
    VANGUI_API void          NavInitRequestApplyResult();
    VANGUI_API bool          NavMoveRequestButNoResultYet();
    VANGUI_API void          NavMoveRequestSubmit(VanGuiDir move_dir, VanGuiDir clip_dir, VanGuiNavMoveFlags move_flags, VanGuiScrollFlags scroll_flags);
    VANGUI_API void          NavMoveRequestForward(VanGuiDir move_dir, VanGuiDir clip_dir, VanGuiNavMoveFlags move_flags, VanGuiScrollFlags scroll_flags);
    VANGUI_API void          NavMoveRequestResolveWithLastItem(VanGuiNavItemData* result);
    VANGUI_API void          NavMoveRequestResolveWithPastTreeNode(VanGuiNavItemData* result, const VanGuiTreeNodeStackData* tree_node_data);
    VANGUI_API void          NavMoveRequestCancel();
    VANGUI_API void          NavMoveRequestApplyResult();
    VANGUI_API void          NavMoveRequestTryWrapping(VanGuiWindow* window, VanGuiNavMoveFlags move_flags);
    VANGUI_API void          NavHighlightActivated(VanGuiID id);
    VANGUI_API void          NavClearPreferredPosForAxis(VanGuiAxis axis);
    VANGUI_API void          SetNavCursorVisibleAfterMove();
    VANGUI_API void          NavUpdateCurrentWindowIsScrollPushableX();
    VANGUI_API void          SetNavWindow(VanGuiWindow* window);
    VANGUI_API void          SetNavID(VanGuiID id, VanGuiNavLayer nav_layer, VanGuiID focus_scope_id, const VanRect& rect_rel);
    VANGUI_API void          SetNavFocusScope(VanGuiID focus_scope_id);

    // Focus/Activation
    // This should be part of a larger set of API: FocusItem(offset = -1), FocusItemByID(id), ActivateItem(offset = -1), ActivateItemByID(id) etc. which are
    // much harder to design and implement than expected. I have a couple of private branches on this matter but it's not simple. For now implementing the easy ones.
    VANGUI_API void          FocusItem();                    // Focus last item (no selection/activation).
    VANGUI_API void          ActivateItemByID(VanGuiID id);   // Activate an item by ID (button, checkbox, tree node etc.). Activation is queued and processed on the next frame when the item is encountered again. Was called 'ActivateItem()' before 1.89.7.

    // Inputs
    // FIXME: Eventually we should aim to move e.g. IsActiveIdUsingKey() into IsKeyXXX functions.
    inline bool             IsNamedKey(VanGuiKey key)                    { return key >= VanGuiKey_NamedKey_BEGIN && key < VanGuiKey_NamedKey_END; }
    inline bool             IsNamedKeyOrMod(VanGuiKey key)               { return (key >= VanGuiKey_NamedKey_BEGIN && key < VanGuiKey_NamedKey_END) || key == VanGuiMod_Ctrl || key == VanGuiMod_Shift || key == VanGuiMod_Alt || key == VanGuiMod_Super; }
    inline bool             IsLegacyKey(VanGuiKey key)                   { return key >= VanGuiKey_LegacyNativeKey_BEGIN && key < VanGuiKey_LegacyNativeKey_END; }
    inline bool             IsKeyboardKey(VanGuiKey key)                 { return key >= VanGuiKey_Keyboard_BEGIN && key < VanGuiKey_Keyboard_END; }
    inline bool             IsGamepadKey(VanGuiKey key)                  { return key >= VanGuiKey_Gamepad_BEGIN && key < VanGuiKey_Gamepad_END; }
    inline bool             IsMouseKey(VanGuiKey key)                    { return key >= VanGuiKey_Mouse_BEGIN && key < VanGuiKey_Mouse_END; }
    inline bool             IsAliasKey(VanGuiKey key)                    { return key >= VanGuiKey_Aliases_BEGIN && key < VanGuiKey_Aliases_END; }
    inline bool             IsLRModKey(VanGuiKey key)                    { return key >= VanGuiKey_LeftCtrl && key <= VanGuiKey_RightSuper; }
    VanGuiKeyChord           FixupKeyChord(VanGuiKeyChord key_chord);
    inline VanGuiKey         ConvertSingleModFlagToKey(VanGuiKey key)
    {
        if (key == VanGuiMod_Ctrl) return VanGuiKey_ReservedForModCtrl;
        if (key == VanGuiMod_Shift) return VanGuiKey_ReservedForModShift;
        if (key == VanGuiMod_Alt) return VanGuiKey_ReservedForModAlt;
        if (key == VanGuiMod_Super) return VanGuiKey_ReservedForModSuper;
        return key;
    }

    VANGUI_API VanGuiKeyData* GetKeyData(VanGuiContext* ctx, VanGuiKey key);
    inline VanGuiKeyData*    GetKeyData(VanGuiKey key)                                    { VanGuiContext& g = *GVanGui; return GetKeyData(&g, key); }
    VANGUI_API const char*   GetKeyChordName(VanGuiKeyChord key_chord);
    inline VanGuiKey         MouseButtonToKey(VanGuiMouseButton button)                   { VAN_ASSERT(button >= 0 && button < VanGuiMouseButton_COUNT); return static_cast<VanGuiKey>(VanGuiKey_MouseLeft + button); }
    VANGUI_API bool          IsMouseDragPastThreshold(VanGuiMouseButton button, float lock_threshold = -1.0f);
    VANGUI_API VanVec2        GetKeyMagnitude2d(VanGuiKey key_left, VanGuiKey key_right, VanGuiKey key_up, VanGuiKey key_down);
    VANGUI_API float         GetNavTweakPressedAmount(VanGuiAxis axis);
    VANGUI_API int           CalcTypematicRepeatAmount(float t0, float t1, float repeat_delay, float repeat_rate);
    VANGUI_API void          GetTypematicRepeatRate(VanGuiInputFlags flags, float* repeat_delay, float* repeat_rate);
    VANGUI_API void          TeleportMousePos(const VanVec2& pos);
    VANGUI_API void          SetActiveIdUsingAllKeyboardKeys();
    inline bool             IsActiveIdUsingNavDir(VanGuiDir dir)                         { VanGuiContext& g = *GVanGui; return (g.ActiveIdUsingNavDirMask & (1 << dir)) != 0; }

    // [EXPERIMENTAL] Low-Level: Key/Input Ownership
    // - The idea is that instead of "eating" a given input, we can link to an owner id.
    // - Ownership is most often claimed as a result of reacting to a press/down event (but occasionally may be claimed ahead).
    // - Input queries can then read input by specifying VanGuiKeyOwner_Any (== 0), VanGuiKeyOwner_NoOwner (== -1) or a custom ID.
    // - Legacy input queries (without specifying an owner or _Any or _None) are equivalent to using VanGuiKeyOwner_Any (== 0).
    // - Input ownership is automatically released on the frame after a key is released. Therefore:
    //   - for ownership registration happening as a result of a down/press event, the SetKeyOwner() call may be done once (common case).
    //   - for ownership registration happening ahead of a down/press event, the SetKeyOwner() call needs to be made every frame (happens if e.g. claiming ownership on hover).
    // - SetItemKeyOwner() is a shortcut for common simple case. A custom widget will probably want to call SetKeyOwner() multiple times directly based on its interaction state.
    // - This is marked experimental because not all widgets are fully honoring the Set/Test idioms. We will need to move forward step by step.
    //   Please open a GitHub Issue to submit your usage scenario or if there's a use case you need solved.
    VANGUI_API VanGuiID       GetKeyOwner(VanGuiKey key);
    VANGUI_API void          SetKeyOwner(VanGuiKey key, VanGuiID owner_id, VanGuiInputFlags flags = 0);
    VANGUI_API void          SetKeyOwnersForKeyChord(VanGuiKeyChord key, VanGuiID owner_id, VanGuiInputFlags flags = 0);
    VANGUI_API bool          SetItemKeyOwner(VanGuiKey key, VanGuiInputFlags flags);
    VANGUI_API bool          TestKeyOwner(VanGuiKey key, VanGuiID owner_id);               // Test that key is either not owned, either owned by 'owner_id'
    inline VanGuiKeyOwnerData* GetKeyOwnerData(VanGuiContext* ctx, VanGuiKey key)          { if (key & VanGuiMod_Mask_) key = ConvertSingleModFlagToKey(key); VAN_ASSERT(IsNamedKey(key)); return &ctx->KeysOwnerData[key - VanGuiKey_NamedKey_BEGIN]; }

    // [EXPERIMENTAL] High-Level: Input Access functions w/ support for Key/Input Ownership
    // - Important: legacy IsKeyPressed(VanGuiKey, bool repeat=true) _DEFAULTS_ to repeat, new IsKeyPressed() requires _EXPLICIT_ VanGuiInputFlags_Repeat flag.
    // - Expected to be later promoted to public API, the prototypes are designed to replace existing ones (since owner_id can default to Any == 0)
    // - Specifying a value for 'VanGuiID owner' will test that EITHER the key is NOT owned (UNLESS locked), EITHER the key is owned by 'owner'.
    //   Legacy functions use VanGuiKeyOwner_Any meaning that they typically ignore ownership, unless a call to SetKeyOwner() explicitly used VanGuiInputFlags_LockThisFrame or VanGuiInputFlags_LockUntilRelease.
    // - Binding generators may want to ignore those for now, or suffix them with Ex() until we decide if this gets moved into public API.
    VANGUI_API bool          IsKeyDown(VanGuiKey key, VanGuiID owner_id);
    VANGUI_API bool          IsKeyPressed(VanGuiKey key, VanGuiInputFlags flags, VanGuiID owner_id = 0);    // Important: when transitioning from old to new IsKeyPressed(): old API has "bool repeat = true", so would default to repeat. New API requires explicit VanGuiInputFlags_Repeat.
    VANGUI_API bool          IsKeyReleased(VanGuiKey key, VanGuiID owner_id);
    VANGUI_API bool          IsKeyChordPressed(VanGuiKeyChord key_chord, VanGuiInputFlags flags, VanGuiID owner_id = 0);
    VANGUI_API bool          IsMouseDown(VanGuiMouseButton button, VanGuiID owner_id);
    VANGUI_API bool          IsMouseClicked(VanGuiMouseButton button, VanGuiInputFlags flags, VanGuiID owner_id = 0);
    VANGUI_API bool          IsMouseReleased(VanGuiMouseButton button, VanGuiID owner_id);
    VANGUI_API bool          IsMouseDoubleClicked(VanGuiMouseButton button, VanGuiID owner_id);

    // Shortcut Testing & Routing
    // - Set Shortcut() and SetNextItemShortcut() in vangui.h
    // - When a policy (except for VanGuiInputFlags_RouteAlways *) is set, Shortcut() will register itself with SetShortcutRouting(),
    //   allowing the system to decide where to route the input among other route-aware calls.
    //   (* using VanGuiInputFlags_RouteAlways is roughly equivalent to calling IsKeyChordPressed(key) and bypassing route registration and check)
    // - When using one of the routing option:
    //   - The default route is VanGuiInputFlags_RouteFocused (accept inputs if window is in focus stack. Deep-most focused window takes inputs. ActiveId takes inputs over deep-most focused window.)
    //   - Routes are requested given a chord (key + modifiers) and a routing policy.
    //   - Routes are resolved during NewFrame(): if keyboard modifiers are matching current ones: SetKeyOwner() is called + route is granted for the frame.
    //   - Each route may be granted to a single owner. When multiple requests are made we have policies to select the winning route (e.g. deep most window).
    //   - Multiple read sites may use the same owner id can all access the granted route.
    //   - When owner_id is 0 we use the current Focus Scope ID as a owner ID in order to identify our location.
    // - You can chain two unrelated windows in the focus stack using SetWindowParentWindowForFocusRoute()
    //   e.g. if you have a tool window associated to a document, and you want document shortcuts to run when the tool is focused.
    VANGUI_API bool          Shortcut(VanGuiKeyChord key_chord, VanGuiInputFlags flags, VanGuiID owner_id);
    VANGUI_API bool          SetShortcutRouting(VanGuiKeyChord key_chord, VanGuiInputFlags flags, VanGuiID owner_id); // owner_id needs to be explicit and cannot be 0
    VANGUI_API bool          TestShortcutRouting(VanGuiKeyChord key_chord, VanGuiID owner_id);
    VANGUI_API VanGuiKeyRoutingData* GetShortcutRoutingData(VanGuiKeyChord key_chord);

    // [EXPERIMENTAL] Focus Scope
    // This is generally used to identify a unique input location (for e.g. a selection set)
    // There is one per window (automatically set in Begin), but:
    // - Selection patterns generally need to react (e.g. clear a selection) when landing on one item of the set.
    //   So in order to identify a set multiple lists in same window may each need a focus scope.
    //   If you imagine an hypothetical BeginSelectionGroup()/EndSelectionGroup() api, it would likely call PushFocusScope()/EndFocusScope()
    // - Shortcut routing also use focus scope as a default location identifier if an owner is not provided.
    // We don't use the ID Stack for this as it is common to want them separate.
    VANGUI_API void          PushFocusScope(VanGuiID id);
    VANGUI_API void          PopFocusScope();
    VANGUI_API bool          IsInNavFocusRoute(VanGuiID focus_scope_id);
    inline VanGuiID          GetCurrentFocusScope() { VanGuiContext& g = *GVanGui; return g.CurrentFocusScopeId; }   // Focus scope we are outputting into, set by PushFocusScope()

    // Drag and Drop
    VANGUI_API bool          IsDragDropActive();
    VANGUI_API bool          BeginDragDropTargetCustom(const VanRect& bb, VanGuiID id);
    VANGUI_API bool          BeginDragDropTargetViewport(VanGuiViewport* viewport, const VanRect* p_bb = nullptr);
    VANGUI_API void          ClearDragDrop();
    VANGUI_API bool          IsDragDropPayloadBeingAccepted();
    VANGUI_API void          RenderDragDropTargetRectForItem(const VanRect& bb);
    VANGUI_API void          RenderDragDropTargetRectEx(VanDrawList* draw_list, const VanRect& bb, float rounding);

    // Typing-Select API
    // (provide Windows Explorer style "select items by typing partial name" + "cycle through items by typing same letter" feature)
    // (this is currently not documented nor used by main library, but should work. See "widgets_typingselect" in vangui_test_suite for usage code. Please let us know if you use this!)
    VANGUI_API VanGuiTypingSelectRequest* GetTypingSelectRequest(VanGuiTypingSelectFlags flags = VanGuiTypingSelectFlags_None);
    VANGUI_API int           TypingSelectFindMatch(VanGuiTypingSelectRequest* req, int items_count, const char* (*get_item_name_func)(void*, int), void* user_data, int nav_item_idx);
    VANGUI_API int           TypingSelectFindNextSingleCharMatch(VanGuiTypingSelectRequest* req, int items_count, const char* (*get_item_name_func)(void*, int), void* user_data, int nav_item_idx);
    VANGUI_API int           TypingSelectFindBestLeadingMatch(VanGuiTypingSelectRequest* req, int items_count, const char* (*get_item_name_func)(void*, int), void* user_data);

    // Box-Select API
    VANGUI_API bool          BeginBoxSelect(const VanRect& scope_rect, VanGuiWindow* window, VanGuiID box_select_id, VanGuiMultiSelectFlags ms_flags);
    VANGUI_API void          EndBoxSelect(const VanRect& scope_rect, VanGuiMultiSelectFlags ms_flags);

    // Multi-Select API
    VANGUI_API void          MultiSelectItemHeader(VanGuiID id, bool* p_selected, VanGuiButtonFlags* p_button_flags);
    VANGUI_API void          MultiSelectItemFooter(VanGuiID id, bool* p_selected, bool* p_pressed);
    VANGUI_API void          MultiSelectAddSetAll(VanGuiMultiSelectTempData* ms, bool selected);
    VANGUI_API void          MultiSelectAddSetRange(VanGuiMultiSelectTempData* ms, bool selected, int range_dir, VanGuiSelectionUserData first_item, VanGuiSelectionUserData last_item);
    inline VanGuiBoxSelectState*     GetBoxSelectState(VanGuiID id)   { VanGuiContext& g = *GVanGui; return (id != 0 && g.BoxSelectState.ID == id && g.BoxSelectState.IsActive) ? &g.BoxSelectState : nullptr; }
    inline VanGuiMultiSelectState*   GetMultiSelectState(VanGuiID id) { VanGuiContext& g = *GVanGui; return g.MultiSelectStorage.GetByKey(id); }

    // Internal Columns API (this is not exposed because we will encourage transitioning to the Tables API)
    VANGUI_API void          SetWindowClipRectBeforeSetChannel(VanGuiWindow* window, const VanRect& clip_rect);
    VANGUI_API void          BeginColumns(const char* str_id, int count, VanGuiOldColumnFlags flags = 0); // setup number of columns. use an identifier to distinguish multiple column sets. close with EndColumns().
    VANGUI_API void          EndColumns();                                                               // close columns
    VANGUI_API void          PushColumnClipRect(int column_index);
    VANGUI_API void          PushColumnsBackground();
    VANGUI_API void          PopColumnsBackground();
    VANGUI_API VanGuiID       GetColumnsID(const char* str_id, int count);
    VANGUI_API VanGuiOldColumns* FindOrCreateColumns(VanGuiWindow* window, VanGuiID id);
    VANGUI_API float         GetColumnOffsetFromNorm(const VanGuiOldColumns* columns, float offset_norm);
    VANGUI_API float         GetColumnNormFromOffset(const VanGuiOldColumns* columns, float offset);

    // Tables: Candidates for public API
    VANGUI_API void          TableOpenContextMenu(int column_n = -1);
    VANGUI_API void          TableSetColumnWidth(int column_n, float width);
    VANGUI_API void          TableSetColumnSortDirection(int column_n, VanGuiSortDirection sort_direction, bool append_to_sort_specs);
    VANGUI_API int           TableGetHoveredRow();       // Retrieve *PREVIOUS FRAME* hovered row. This difference with TableGetHoveredColumn() is the reason why this is not public yet.
    VANGUI_API float         TableGetHeaderRowHeight();
    VANGUI_API float         TableGetHeaderAngledMaxLabelWidth();
    VANGUI_API void          TablePushBackgroundChannel();
    VANGUI_API void          TablePopBackgroundChannel();
    VANGUI_API void          TablePushColumnChannel(int column_n);
    VANGUI_API void          TablePopColumnChannel();
    VANGUI_API void          TableAngledHeadersRowEx(VanGuiID row_id, float angle, float max_label_width, const VanGuiTableHeaderData* data, int data_count);

    // Tables: Internals
    inline    VanGuiTable*   GetCurrentTable() { VanGuiContext& g = *GVanGui; return g.CurrentTable; }
    VANGUI_API VanGuiTable*   TableFindByID(VanGuiID id);
    VANGUI_API bool          BeginTableEx(const char* name, VanGuiID id, int columns_count, VanGuiTableFlags flags = 0, const VanVec2& outer_size = VanVec2(0, 0), float inner_width = 0.0f);
    VANGUI_API void          TableBeginInitMemory(VanGuiTable* table, int columns_count);
    VANGUI_API void          TableBeginApplyRequests(VanGuiTable* table);
    VANGUI_API void          TableSetupDrawChannels(VanGuiTable* table);
    VANGUI_API void          TableUpdateLayout(VanGuiTable* table);
    VANGUI_API void          TableUpdateBorders(VanGuiTable* table);
    VANGUI_API void          TableUpdateColumnsWeightFromWidth(VanGuiTable* table);
    VANGUI_API void          TableApplyExternalUnclipRect(VanGuiTable* table, VanRect& rect);
    VANGUI_API void          TableDrawBorders(VanGuiTable* table);
    VANGUI_API void          TableDrawDefaultContextMenu(VanGuiTable* table, VanGuiTableFlags flags_for_section_to_display);
    VANGUI_API bool          TableBeginContextMenuPopup(VanGuiTable* table);
    VANGUI_API void          TableMergeDrawChannels(VanGuiTable* table);
    inline VanGuiTableInstanceData*  TableGetInstanceData(VanGuiTable* table, int instance_no) { if (instance_no == 0) return &table->InstanceDataFirst; return &table->InstanceDataExtra[instance_no - 1]; }
    inline VanGuiID                  TableGetInstanceID(VanGuiTable* table, int instance_no)   { return TableGetInstanceData(table, instance_no)->TableInstanceID; }
    VANGUI_API void          TableFixDisplayOrder(VanGuiTable* table);
    VANGUI_API void          TableSortSpecsSanitize(VanGuiTable* table);
    VANGUI_API void          TableSortSpecsBuild(VanGuiTable* table);
    VANGUI_API VanGuiSortDirection TableGetColumnNextSortDirection(VanGuiTableColumn* column);
    VANGUI_API void          TableFixColumnSortDirection(VanGuiTable* table, VanGuiTableColumn* column);
    VANGUI_API float         TableGetColumnWidthAuto(VanGuiTable* table, VanGuiTableColumn* column);
    VANGUI_API void          TableBeginRow(VanGuiTable* table);
    VANGUI_API void          TableEndRow(VanGuiTable* table);
    VANGUI_API void          TableBeginCell(VanGuiTable* table, int column_n);
    VANGUI_API void          TableEndCell(VanGuiTable* table);
    VANGUI_API VanRect        TableGetCellBgRect(const VanGuiTable* table, int column_n);
    VANGUI_API const char*   TableGetColumnName(const VanGuiTable* table, int column_n);
    VANGUI_API VanGuiID       TableGetColumnResizeID(VanGuiTable* table, int column_n, int instance_no = 0);
    VANGUI_API float         TableCalcMaxColumnWidth(const VanGuiTable* table, int column_n);
    VANGUI_API void          TableSetColumnWidthAutoSingle(VanGuiTable* table, int column_n);
    VANGUI_API void          TableSetColumnWidthAutoAll(VanGuiTable* table);
    VANGUI_API void          TableSetColumnDisplayOrder(VanGuiTable* table, int column_n, int dst_order);
    VANGUI_API void          TableQueueSetColumnDisplayOrder(VanGuiTable* table, int column_n, int dst_order);
    VANGUI_API void          TableRemove(VanGuiTable* table);
    VANGUI_API void          TableGcCompactTransientBuffers(VanGuiTable* table);
    VANGUI_API void          TableGcCompactTransientBuffers(VanGuiTableTempData* table);
    VANGUI_API void          TableGcCompactSettings();

    // Tables: Settings
    VANGUI_API void                  TableLoadSettings(VanGuiTable* table);
    VANGUI_API void                  TableSaveSettings(VanGuiTable* table);
    VANGUI_API void                  TableResetSettings(VanGuiTable* table);
    VANGUI_API VanGuiTableSettings*   TableGetBoundSettings(VanGuiTable* table);
    VANGUI_API void                  TableSettingsAddSettingsHandler();
    VANGUI_API VanGuiTableSettings*   TableSettingsCreate(VanGuiID id, int columns_count);
    VANGUI_API VanGuiTableSettings*   TableSettingsFindByID(VanGuiID id);

    // Tab Bars
    inline    VanGuiTabBar*  GetCurrentTabBar() { VanGuiContext& g = *GVanGui; return g.CurrentTabBar; }
    VANGUI_API VanGuiTabBar*  TabBarFindByID(VanGuiID id);
    VANGUI_API void          TabBarRemove(VanGuiTabBar* tab_bar);
    VANGUI_API bool          BeginTabBarEx(VanGuiTabBar* tab_bar, const VanRect& bb, VanGuiTabBarFlags flags);
    VANGUI_API VanGuiTabItem* TabBarFindTabByID(VanGuiTabBar* tab_bar, VanGuiID tab_id);
    VANGUI_API VanGuiTabItem* TabBarFindTabByOrder(VanGuiTabBar* tab_bar, int order);
    VANGUI_API VanGuiTabItem* TabBarGetCurrentTab(VanGuiTabBar* tab_bar);
    inline int              TabBarGetTabOrder(VanGuiTabBar* tab_bar, VanGuiTabItem* tab) { return tab_bar->Tabs.index_from_ptr(tab); }
    VANGUI_API const char*   TabBarGetTabName(VanGuiTabBar* tab_bar, VanGuiTabItem* tab);
    VANGUI_API void          TabBarRemoveTab(VanGuiTabBar* tab_bar, VanGuiID tab_id);
    VANGUI_API void          TabBarCloseTab(VanGuiTabBar* tab_bar, VanGuiTabItem* tab);
    VANGUI_API void          TabBarQueueFocus(VanGuiTabBar* tab_bar, VanGuiTabItem* tab);
    VANGUI_API void          TabBarQueueFocus(VanGuiTabBar* tab_bar, const char* tab_name);
    VANGUI_API void          TabBarQueueReorder(VanGuiTabBar* tab_bar, VanGuiTabItem* tab, int offset);
    VANGUI_API void          TabBarQueueReorderFromMousePos(VanGuiTabBar* tab_bar, VanGuiTabItem* tab, VanVec2 mouse_pos);
    VANGUI_API bool          TabBarProcessReorder(VanGuiTabBar* tab_bar);
    VANGUI_API bool          TabItemEx(VanGuiTabBar* tab_bar, const char* label, bool* p_open, VanGuiTabItemFlags flags, VanGuiWindow* docked_window);
    VANGUI_API void          TabItemSpacing(const char* str_id, VanGuiTabItemFlags flags, float width);
    VANGUI_API VanVec2        TabItemCalcSize(const char* label, bool has_close_button_or_unsaved_marker);
    VANGUI_API VanVec2        TabItemCalcSize(VanGuiWindow* window);
    VANGUI_API void          TabItemBackground(VanDrawList* draw_list, const VanRect& bb, VanGuiTabItemFlags flags, VanU32 col);
    VANGUI_API void          TabItemLabelAndCloseButton(VanDrawList* draw_list, const VanRect& bb, VanGuiTabItemFlags flags, VanVec2 frame_padding, const char* label, VanGuiID tab_id, VanGuiID close_button_id, bool is_contents_visible, bool* out_just_closed, bool* out_text_clipped);

    // Docking
    VANGUI_API void             DockContextInitialize(VanGuiContext* ctx);
    VANGUI_API void             DockContextNewFrameUpdateDocking(VanGuiContext* ctx);
    VANGUI_API VanGuiDockNode*  DockContextFindNodeByID(VanGuiContext* ctx, VanGuiID id);
    VANGUI_API void             DockContextQueueDock(VanGuiContext* ctx, VanGuiWindow* target_window, VanGuiDockNode* target_node, VanGuiWindow* payload, VanGuiDir split_dir, float split_ratio, bool split_outer);
    VANGUI_API void             DockContextQueueUndockWindow(VanGuiContext* ctx, VanGuiWindow* window);
    VANGUI_API void             DockContextQueueUndockNode(VanGuiContext* ctx, VanGuiDockNode* node);
    VANGUI_API void             DockContextRenderWindowOverlay(VanGuiWindow* payload_window);

    // Render helpers
    // AVOID USING OUTSIDE OF VANGUI.CPP! NOT FOR PUBLIC CONSUMPTION. THOSE FUNCTIONS ARE A MESS. THEIR SIGNATURE AND BEHAVIOR WILL CHANGE, THEY NEED TO BE REFACTORED INTO SOMETHING DECENT.
    // NB: All position are in absolute pixels coordinates (we are never using window coordinates internally)
    VANGUI_API void          RenderText(VanVec2 pos, const char* text, const char* text_end = nullptr, bool hide_text_after_hash = true);
    VANGUI_API void          RenderTextWrapped(VanVec2 pos, const char* text, const char* text_end, float wrap_width);
    VANGUI_API void          RenderTextClipped(const VanVec2& pos_min, const VanVec2& pos_max, const char* text, const char* text_end, const VanVec2* text_size_if_known, const VanVec2& align = VanVec2(0, 0), const VanRect* clip_rect = nullptr);
    VANGUI_API void          RenderTextClippedEx(VanDrawList* draw_list, const VanVec2& pos_min, const VanVec2& pos_max, const char* text, const char* text_end, const VanVec2* text_size_if_known, const VanVec2& align = VanVec2(0, 0), const VanRect* clip_rect = nullptr);
    VANGUI_API void          RenderTextEllipsis(VanDrawList* draw_list, const VanVec2& pos_min, const VanVec2& pos_max, float ellipsis_max_x, const char* text, const char* text_end, const VanVec2* text_size_if_known);
    VANGUI_API void          RenderFrame(VanVec2 p_min, VanVec2 p_max, VanU32 fill_col, bool borders = true, float rounding = 0.0f);
    VANGUI_API void          RenderFrameBorder(VanVec2 p_min, VanVec2 p_max, float rounding = 0.0f);
    VANGUI_API void          RenderColorComponentMarker(const VanRect& bb, VanU32 col, float rounding);
    VANGUI_API void          RenderColorRectWithAlphaCheckerboard(VanDrawList* draw_list, VanVec2 p_min, VanVec2 p_max, VanU32 fill_col, float grid_step, VanVec2 grid_off, float rounding = 0.0f, VanDrawFlags flags = 0);
    VANGUI_API void          RenderNavCursor(const VanRect& bb, VanGuiID id, VanGuiNavRenderCursorFlags flags = VanGuiNavRenderCursorFlags_None); // Navigation highlight
#ifndef VANGUI_DISABLE_OBSOLETE_FUNCTIONS
    inline    void          RenderNavHighlight(const VanRect& bb, VanGuiID id, VanGuiNavRenderCursorFlags flags = VanGuiNavRenderCursorFlags_None) { RenderNavCursor(bb, id, flags); } // Renamed in 1.91.4
#endif
    VANGUI_API const char*   FindRenderedTextEnd(const char* text, const char* text_end = nullptr); // Find the optional ## from which we stop displaying text.
    VANGUI_API void          RenderMouseCursor(VanVec2 pos, float scale, VanGuiMouseCursor mouse_cursor, VanU32 col_fill, VanU32 col_border, VanU32 col_shadow);

    // Render helpers (those functions don't access any VanGui state!)
    VANGUI_API void          RenderArrow(VanDrawList* draw_list, VanVec2 pos, VanU32 col, VanGuiDir dir, float scale = 1.0f);
    VANGUI_API void          RenderBullet(VanDrawList* draw_list, VanVec2 pos, VanU32 col);
    VANGUI_API void          RenderCheckMark(VanDrawList* draw_list, VanVec2 pos, VanU32 col, float sz);
    VANGUI_API void          RenderArrowPointingAt(VanDrawList* draw_list, VanVec2 pos, VanVec2 half_sz, VanGuiDir direction, VanU32 col);
    VANGUI_API void          RenderRectFilledInRangeH(VanDrawList* draw_list, const VanRect& rect, VanU32 col, float fill_x0, float fill_x1, float rounding);
    VANGUI_API void          RenderRectFilledWithHole(VanDrawList* draw_list, const VanRect& outer, const VanRect& inner, VanU32 col, float rounding);
    VANGUI_API VanDrawFlags   CalcRoundingFlagsForRectInRect(const VanRect& r_in, const VanRect& r_outer, float threshold);

    // Widgets: Text
    VANGUI_API void          TextEx(const char* text, const char* text_end = nullptr, VanGuiTextFlags flags = 0);
    VANGUI_API void          TextAligned(float align_x, float size_x, const char* fmt, ...);               // FIXME-WIP: Works but API is likely to be reworked. This is designed for 1 item on the line. (#7024)
    VANGUI_API void          TextAlignedV(float align_x, float size_x, const char* fmt, va_list args);

    // Widgets
    VANGUI_API bool          ButtonEx(const char* label, const VanVec2& size_arg = VanVec2(0, 0), VanGuiButtonFlags flags = 0);
    VANGUI_API bool          ArrowButtonEx(const char* str_id, VanGuiDir dir, VanVec2 size_arg, VanGuiButtonFlags flags = 0);
    VANGUI_API bool          ImageButtonEx(VanGuiID id, VanTextureRef tex_ref, const VanVec2& image_size, const VanVec2& uv0, const VanVec2& uv1, const VanVec4& bg_col, const VanVec4& tint_col, VanGuiButtonFlags flags = 0);
    VANGUI_API void          SeparatorEx(VanGuiSeparatorFlags flags, float thickness = 1.0f);
    VANGUI_API void          SeparatorTextEx(VanGuiID id, const char* label, const char* label_end, float extra_width);
    VANGUI_API bool          CheckboxFlags(const char* label, VanS64* flags, VanS64 flags_value);
    VANGUI_API bool          CheckboxFlags(const char* label, VanU64* flags, VanU64 flags_value);

    // Widgets: Window Decorations
    VANGUI_API bool          CloseButton(VanGuiID id, const VanVec2& pos);
    VANGUI_API bool          CollapseButton(VanGuiID id, const VanVec2& pos);
    VANGUI_API void          Scrollbar(VanGuiAxis axis);
    VANGUI_API bool          ScrollbarEx(const VanRect& bb, VanGuiID id, VanGuiAxis axis, VanS64* p_scroll_v, VanS64 avail_v, VanS64 contents_v, VanDrawFlags draw_rounding_flags = 0);
    VANGUI_API VanRect        GetWindowScrollbarRect(VanGuiWindow* window, VanGuiAxis axis);
    VANGUI_API VanGuiID       GetWindowScrollbarID(VanGuiWindow* window, VanGuiAxis axis);
    VANGUI_API VanGuiID       GetWindowResizeCornerID(VanGuiWindow* window, int n); // 0..3: corners
    VANGUI_API VanGuiID       GetWindowResizeBorderID(VanGuiWindow* window, VanGuiDir dir);
    VANGUI_API void          ExtendHitBoxWhenNearViewportEdge(VanGuiWindow* window, VanRect* bb, float threshold, VanGuiAxis axis);

    // Widgets low-level behaviors
    VANGUI_API bool          ButtonBehavior(const VanRect& bb, VanGuiID id, bool* out_hovered, bool* out_held, VanGuiButtonFlags flags = 0);
    VANGUI_API bool          DragBehavior(VanGuiID id, VanGuiDataType data_type, void* p_v, float v_speed, const void* p_min, const void* p_max, const char* format, VanGuiSliderFlags flags);
    VANGUI_API bool          SliderBehavior(const VanRect& bb, VanGuiID id, VanGuiDataType data_type, void* p_v, const void* p_min, const void* p_max, const char* format, VanGuiSliderFlags flags, VanRect* out_grab_bb);
    VANGUI_API bool          SplitterBehavior(const VanRect& bb, VanGuiID id, VanGuiAxis axis, float* size1, float* size2, float min_size1, float min_size2, float hover_extend = 0.0f, float hover_visibility_delay = 0.0f, VanU32 bg_col = 0);

    // Widgets: Tree Nodes
    VANGUI_API bool          TreeNodeBehavior(VanGuiID id, VanGuiTreeNodeFlags flags, const char* label, const char* label_end = nullptr);
    VANGUI_API void          TreeNodeDrawLineToChildNode(const VanVec2& target_pos);
    VANGUI_API void          TreeNodeDrawLineToTreePop(const VanGuiTreeNodeStackData* data);
    VANGUI_API void          TreePushOverrideID(VanGuiID id);
    VANGUI_API void          TreeNodeSetOpen(VanGuiID storage_id, bool open);
    VANGUI_API bool          TreeNodeUpdateNextOpen(VanGuiID storage_id, VanGuiTreeNodeFlags flags);   // Return open state. Consume previous SetNextItemOpen() data, if any. May return true when logging.

    // Template functions are instantiated in vangui_widgets.cpp for a finite number of types.
    // To use them externally (for custom widget) you may need an "extern template" statement in your code in order to link to existing instances and silence Clang warnings (see #2036).
    // e.g. " extern template VANGUI_API float RoundScalarWithFormatT<float, float>(const char* format, VanGuiDataType data_type, float v); "
    template<typename T, typename SIGNED_T, typename FLOAT_T>   VANGUI_API float ScaleRatioFromValueT(VanGuiDataType data_type, T v, T v_min, T v_max, float logarithmic_zero_epsilon, float zero_deadzone_size);
    template<typename T, typename SIGNED_T, typename FLOAT_T>   VANGUI_API T     ScaleValueFromRatioT(VanGuiDataType data_type, float t, T v_min, T v_max, float logarithmic_zero_epsilon, float zero_deadzone_size);
    template<typename T, typename SIGNED_T, typename FLOAT_T>   VANGUI_API bool  DragBehaviorT(VanGuiDataType data_type, T* v, float v_speed, T v_min, T v_max, const char* format, VanGuiSliderFlags flags);
    template<typename T, typename SIGNED_T, typename FLOAT_T>   VANGUI_API bool  SliderBehaviorT(const VanRect& bb, VanGuiID id, VanGuiDataType data_type, T* v, T v_min, T v_max, const char* format, VanGuiSliderFlags flags, VanRect* out_grab_bb);
    template<typename T>                                        VANGUI_API T     RoundScalarWithFormatT(const char* format, VanGuiDataType data_type, T v);
    template<typename T>                                        VANGUI_API bool  CheckboxFlagsT(const char* label, T* flags, T flags_value);

    // Data type helpers
    VANGUI_API const VanGuiDataTypeInfo*  DataTypeGetInfo(VanGuiDataType data_type);
    VANGUI_API int           DataTypeFormatString(char* buf, int buf_size, VanGuiDataType data_type, const void* p_data, const char* format);
    VANGUI_API void          DataTypeApplyOp(VanGuiDataType data_type, int op, void* output, const void* arg_1, const void* arg_2);
    VANGUI_API bool          DataTypeApplyFromText(const char* buf, VanGuiDataType data_type, void* p_data, const char* format, void* p_data_when_empty = nullptr);
    VANGUI_API int           DataTypeCompare(VanGuiDataType data_type, const void* arg_1, const void* arg_2);
    VANGUI_API bool          DataTypeClamp(VanGuiDataType data_type, void* p_data, const void* p_min, const void* p_max);
    VANGUI_API bool          DataTypeIsZero(VanGuiDataType data_type, const void* p_data);

    // InputText
    VANGUI_API bool          InputTextEx(const char* label, const char* hint, char* buf, int buf_size, const VanVec2& size_arg, VanGuiInputTextFlags flags, VanGuiInputTextCallback callback = nullptr, void* user_data = nullptr);
    VANGUI_API void          InputTextDeactivateHook(VanGuiID id);
    VANGUI_API bool          TempInputText(const VanRect& bb, VanGuiID id, const char* label, char* buf, size_t buf_size, VanGuiInputTextFlags flags = 0, VanGuiInputTextCallback callback = nullptr, void* user_data = nullptr);
    VANGUI_API bool          TempInputScalar(const VanRect& bb, VanGuiID id, const char* label, VanGuiDataType data_type, void* p_data, const char* format, const void* p_clamp_min = nullptr, const void* p_clamp_max = nullptr);
    inline bool             TempInputIsActive(VanGuiID id)       { VanGuiContext& g = *GVanGui; return g.ActiveId == id && g.TempInputId == id; }
    inline VanGuiInputTextState* GetInputTextState(VanGuiID id)   { VanGuiContext& g = *GVanGui; return (id != 0 && g.InputTextState.ID == id) ? &g.InputTextState : nullptr; } // Get input text state if active
    VANGUI_API void          SetNextItemRefVal(VanGuiDataType data_type, void* p_data);
    inline bool             IsItemActiveAsInputText() { VanGuiContext& g = *GVanGui; return g.ActiveId != 0 && g.ActiveId == g.LastItemData.ID && g.InputTextState.ID == g.LastItemData.ID; } // This may be useful to apply workaround that a based on distinguish whenever an item is active as a text input field.

    // Color
    VANGUI_API void          ColorTooltip(const char* text, const float* col, VanGuiColorEditFlags flags);
    VANGUI_API void          ColorEditOptionsPopup(const float* col, VanGuiColorEditFlags flags);
    VANGUI_API void          ColorPickerOptionsPopup(const float* ref_col, VanGuiColorEditFlags flags);
    inline void             SetNextItemColorMarker(VanU32 col) { VanGuiContext& g = *GVanGui; g.NextItemData.HasFlags |= VanGuiNextItemDataFlags_HasColorMarker; g.NextItemData.ColorMarker = col; }

    // Plot
    VANGUI_API int           PlotEx(VanGuiPlotType plot_type, const char* label, float (*values_getter)(void* data, int idx), void* data, int values_count, int values_offset, const char* overlay_text, float scale_min, float scale_max, const VanVec2& size_arg);

    // Shade functions (write over already created vertices)
    VANGUI_API void          ShadeVertsLinearColorGradientKeepAlpha(VanDrawList* draw_list, int vert_start_idx, int vert_end_idx, VanVec2 gradient_p0, VanVec2 gradient_p1, VanU32 col0, VanU32 col1);
    VANGUI_API void          ShadeVertsLinearUV(VanDrawList* draw_list, int vert_start_idx, int vert_end_idx, const VanVec2& a, const VanVec2& b, const VanVec2& uv_a, const VanVec2& uv_b, bool clamp);
    VANGUI_API void          ShadeVertsTransformPos(VanDrawList* draw_list, int vert_start_idx, int vert_end_idx, const VanVec2& pivot_in, float cos_a, float sin_a, const VanVec2& pivot_out);

    // Garbage collection
    VANGUI_API void          GcCompactTransientMiscBuffers();
    VANGUI_API void          GcCompactTransientWindowBuffers(VanGuiWindow* window);
    VANGUI_API void          GcAwakeTransientWindowBuffers(VanGuiWindow* window);

    // Error handling, State Recovery
    VANGUI_API bool          ErrorLog(const char* msg);
    VANGUI_API void          ErrorRecoveryStoreState(VanGuiErrorRecoveryState* state_out);
    VANGUI_API void          ErrorRecoveryTryToRecoverState(const VanGuiErrorRecoveryState* state_in);
    VANGUI_API void          ErrorRecoveryTryToRecoverWindowState(const VanGuiErrorRecoveryState* state_in);
    VANGUI_API void          ErrorCheckUsingSetCursorPosToExtendParentBoundaries();
    VANGUI_API void          ErrorCheckEndFrameFinalizeErrorTooltip();
    VANGUI_API bool          BeginErrorTooltip();
    VANGUI_API void          EndErrorTooltip();

    // Demo Doc Marker for e.g. vangui_explorer
    VANGUI_API void          DemoMarker(const char* file, int line, const char* section);

    // Debug Tools
    VANGUI_API void          DebugAllocHook(VanGuiDebugAllocInfo* info, int frame_count, void* ptr, size_t size); // size >= 0 : alloc, size = -1 : free
    VANGUI_API void          DebugDrawCursorPos(VanU32 col = VAN_COL32(255, 0, 0, 255));
    VANGUI_API void          DebugDrawLineExtents(VanU32 col = VAN_COL32(255, 0, 0, 255));
    VANGUI_API void          DebugDrawItemRect(VanU32 col = VAN_COL32(255, 0, 0, 255));
    VANGUI_API void          DebugTextUnformattedWithLocateItem(const char* line_begin, const char* line_end);
    VANGUI_API void          DebugLocateItem(VanGuiID target_id);                     // Call sparingly: only 1 at the same time!
    VANGUI_API void          DebugLocateItemOnHover(VanGuiID target_id);              // Only call on reaction to a mouse Hover: because only 1 at the same time!
    VANGUI_API void          DebugLocateItemResolveWithLastItem();
    VANGUI_API void          DebugBreakClearData();
    VANGUI_API bool          DebugBreakButton(const char* label, const char* description_of_location);
    VANGUI_API void          DebugBreakButtonTooltip(bool keyboard_only, const char* description_of_location);
    VANGUI_API void          ShowFontAtlas(VanFontAtlas* atlas);
    VANGUI_API VanU64         DebugTextureIDToU64(VanTextureID tex_id);
    VANGUI_API void          DebugHookIdInfo(VanGuiID id, VanGuiDataType data_type, const void* data_id, const void* data_id_end);
    VANGUI_API void          DebugNodeColumns(VanGuiOldColumns* columns);
    VANGUI_API void          DebugNodeDrawList(VanGuiWindow* window, VanGuiViewportP* viewport, const VanDrawList* draw_list, const char* label);
    VANGUI_API void          DebugNodeDrawCmdShowMeshAndBoundingBox(VanDrawList* out_draw_list, const VanDrawList* draw_list, const VanDrawCmd* draw_cmd, bool show_mesh, bool show_aabb);
    VANGUI_API void          DebugNodeFont(VanFont* font);
    VANGUI_API void          DebugNodeFontGlyphsForSrcMask(VanFont* font, VanFontBaked* baked, int src_mask);
    VANGUI_API void          DebugNodeFontGlyph(VanFont* font, const VanFontGlyph* glyph);
    VANGUI_API void          DebugNodeTexture(VanTextureData* tex, int int_id, const VanFontAtlasRect* highlight_rect = nullptr); // ID used to facilitate persisting the "current" texture.
    VANGUI_API void          DebugNodeStorage(VanGuiStorage* storage, const char* label);
    VANGUI_API void          DebugNodeTabBar(VanGuiTabBar* tab_bar, const char* label);
    VANGUI_API void          DebugNodeTable(VanGuiTable* table);
    VANGUI_API void          DebugNodeTableSettings(VanGuiTableSettings* settings);
    VANGUI_API void          DebugNodeInputTextState(VanGuiInputTextState* state);
    VANGUI_API void          DebugNodeTypingSelectState(VanGuiTypingSelectState* state);
    VANGUI_API void          DebugNodeMultiSelectState(VanGuiMultiSelectState* state);
    VANGUI_API void          DebugNodeWindow(VanGuiWindow* window, const char* label);
    VANGUI_API void          DebugNodeWindowSettings(VanGuiWindowSettings* settings);
    VANGUI_API void          DebugNodeWindowsList(VanVector<VanGuiWindow*>* windows, const char* label);
    VANGUI_API void          DebugNodeWindowsListByBeginStackParent(VanGuiWindow** windows, int windows_size, VanGuiWindow* parent_in_begin_stack);
    VANGUI_API void          DebugNodeViewport(VanGuiViewportP* viewport);
    VANGUI_API void          DebugRenderKeyboardPreview(VanDrawList* draw_list);
    VANGUI_API void          DebugRenderViewportThumbnail(VanDrawList* draw_list, VanGuiViewportP* viewport, const VanRect& bb);

    // Obsolete functions
#ifndef VANGUI_DISABLE_OBSOLETE_FUNCTIONS
    //inline void   SetItemUsingMouseWheel()                                            { SetItemKeyOwner(VanGuiKey_MouseWheelY); }      // Changed in 1.89
    //inline bool   TreeNodeBehaviorIsOpen(VanGuiID id, VanGuiTreeNodeFlags flags = 0)    { return TreeNodeUpdateNextOpen(id, flags); }   // Renamed in 1.89
    //inline bool   IsKeyPressedMap(VanGuiKey key, bool repeat = true)                   { VAN_ASSERT(IsNamedKey(key)); return IsKeyPressed(key, repeat); } // Removed in 1.87: Mapping from named key is always identity!

    // Refactored focus/nav/tabbing system in 1.82 and 1.84. If you have old/custom copy-and-pasted widgets which used FocusableItemRegister():
    //  (Old) VANGUI_VERSION_NUM  < 18209: using 'ItemAdd(....)'                              and 'bool tab_focused = FocusableItemRegister(...)'
    //  (Old) VANGUI_VERSION_NUM >= 18209: using 'ItemAdd(..., VanGuiItemAddFlags_Focusable)'  and 'bool tab_focused = (g.LastItemData.StatusFlags & VanGuiItemStatusFlags_Focused) != 0'
    //  (New) VANGUI_VERSION_NUM >= 18413: using 'ItemAdd(..., VanGuiItemFlags_Inputable)'     and 'bool tab_focused = (g.NavActivateId == id && (g.NavActivateFlags & VanGuiActivateFlags_PreferInput))'
    //inline bool   FocusableItemRegister(VanGuiWindow* window, VanGuiID id)              // -> pass VanGuiItemAddFlags_Inputable flag to ItemAdd()
    //inline void   FocusableItemUnregister(VanGuiWindow* window)                        // -> unnecessary: TempInputText() uses VanGuiInputTextFlags_MergedItem
#endif

} // namespace VanGui


//-----------------------------------------------------------------------------
// [SECTION] VanFontLoader
//-----------------------------------------------------------------------------

// Hooks and storage for a given font backend.
// This structure is likely to evolve as we add support for incremental atlas updates.
// Conceptually this could be public, but API is still going to be evolve.
struct VanFontLoader
{
    const char*     Name;
    bool            (*LoaderInit)(VanFontAtlas* atlas);
    void            (*LoaderShutdown)(VanFontAtlas* atlas);
    bool            (*FontSrcInit)(VanFontAtlas* atlas, VanFontConfig* src);
    void            (*FontSrcDestroy)(VanFontAtlas* atlas, VanFontConfig* src);
    bool            (*FontSrcContainsGlyph)(VanFontAtlas* atlas, VanFontConfig* src, VanWchar codepoint);
    bool            (*FontBakedInit)(VanFontAtlas* atlas, VanFontConfig* src, VanFontBaked* baked, void* loader_data_for_baked_src);
    void            (*FontBakedDestroy)(VanFontAtlas* atlas, VanFontConfig* src, VanFontBaked* baked, void* loader_data_for_baked_src);
    bool            (*FontBakedLoadGlyph)(VanFontAtlas* atlas, VanFontConfig* src, VanFontBaked* baked, void* loader_data_for_baked_src, VanWchar codepoint, VanFontGlyph* out_glyph, float* out_advance_x);

    // Size of backend data, Per Baked * Per Source. Buffers are managed by core to avoid excessive allocations.
    // FIXME: At this point the two other types of buffers may be managed by core to be consistent?
    size_t          FontBakedSrcLoaderDataSize;

    VanFontLoader()  { memset((void*)this, 0, sizeof(*this)); } // POD-safe zero-init (all members are function pointers/primitives)
};

#ifdef VANGUI_ENABLE_STB_TRUETYPE
VANGUI_API const VanFontLoader* VanFontAtlasGetFontLoaderForStbTruetype();
#endif
#ifndef VANGUI_DISABLE_OBSOLETE_FUNCTIONS
using VanFontBuilderIO = VanFontLoader; // [renamed/changed in 1.92.0] The types are not actually compatible but we provide this as a compile-time error report helper.
#endif

//-----------------------------------------------------------------------------
// [SECTION] VanFontAtlas internal API
//-----------------------------------------------------------------------------

#define VANGUI_FONT_SIZE_MAX                                     (512.0f)
#define VANGUI_FONT_SIZE_THRESHOLD_FOR_LOADADVANCEXONLYMODE      (128.0f)

// Helpers: VanTextureRef ==/!= operators provided as convenience
// (note that _TexID and _TexData are never set simultaneously)
inline bool operator==(const VanTextureRef& lhs, const VanTextureRef& rhs)    { return lhs._TexID == rhs._TexID && lhs._TexData == rhs._TexData; }
inline bool operator!=(const VanTextureRef& lhs, const VanTextureRef& rhs)    { return lhs._TexID != rhs._TexID || lhs._TexData != rhs._TexData; }

// Refer to VanFontAtlasPackGetRect() to better understand how this works.
#define VanFontAtlasRectId_IndexMask_        (0x0007FFFF)    // 20-bits signed: index to access builder->RectsIndex[].
#define VanFontAtlasRectId_GenerationMask_   (0x3FF00000)    // 10-bits: entry generation, so each ID is unique and get can safely detected old identifiers.
#define VanFontAtlasRectId_GenerationShift_  (20)
inline int               VanFontAtlasRectId_GetIndex(VanFontAtlasRectId id)       { return (id & VanFontAtlasRectId_IndexMask_); }
inline unsigned int      VanFontAtlasRectId_GetGeneration(VanFontAtlasRectId id)  { return static_cast<unsigned int>(id & VanFontAtlasRectId_GenerationMask_) >> VanFontAtlasRectId_GenerationShift_; }
inline VanFontAtlasRectId VanFontAtlasRectId_Make(int index_idx, int gen_idx)     { VAN_ASSERT(index_idx >= 0 && index_idx <= VanFontAtlasRectId_IndexMask_ && gen_idx <= (VanFontAtlasRectId_GenerationMask_ >> VanFontAtlasRectId_GenerationShift_)); return static_cast<VanFontAtlasRectId>(index_idx | (gen_idx << VanFontAtlasRectId_GenerationShift_)); }

// Packed rectangle lookup entry (we need an indirection to allow removing/reordering rectangles)
// User are returned VanFontAtlasRectId values which are meant to be persistent.
// We handle this with an indirection. While Rects[] may be in theory shuffled, compacted etc., RectsIndex[] cannot it is keyed by VanFontAtlasRectId.
// RectsIndex[] is used both as an index into Rects[] and an index into itself. This is basically a free-list. See VanFontAtlasBuildAllocRectIndexEntry() code.
// Having this also makes it easier to e.g. sort rectangles during repack.
struct VanFontAtlasRectEntry
{
    int                 TargetIndex : 20;   // When Used: VanFontAtlasRectId -> into Rects[]. When unused: index to next unused RectsIndex[] slot to consume free-list.
    unsigned int        Generation : 10;    // Increased each time the entry is reused for a new rectangle.
    unsigned int        IsUsed : 1;
};

// Data available to potential texture post-processing functions
struct VanFontAtlasPostProcessData
{
    VanFontAtlas*        FontAtlas;
    VanFont*             Font;
    VanFontConfig*       FontSrc;
    VanFontBaked*        FontBaked;
    VanFontGlyph*        Glyph;

    // Pixel data
    void*               Pixels;
    VanTextureFormat     Format;
    int                 Pitch;
    int                 Width;
    int                 Height;
};

// We avoid dragging vanstb_rectpack.h into public header (partly because binding generators are having issues with it)
#ifdef VANGUI_STB_NAMESPACE
namespace VANGUI_STB_NAMESPACE { struct stbrp_node; }
using stbrp_node_im = VANGUI_STB_NAMESPACE::stbrp_node;
#else
struct stbrp_node;
using stbrp_node_im = stbrp_node;
#endif
struct stbrp_context_opaque { char data[80]; };

// Internal storage for incrementally packing and building a VanFontAtlas
struct VanFontAtlasBuilder
{
    stbrp_context_opaque        PackContext;            // Actually 'stbrp_context' but we don't want to define this in the header file.
    VanVector<stbrp_node_im>     PackNodes;
    VanVector<VanTextureRect>     Rects;
    VanVector<VanFontAtlasRectEntry> RectsIndex;          // VanFontAtlasRectId -> index into Rects[]
    VanVector<unsigned char>     TempBuffer;             // Misc scratch buffer
    int                         RectsIndexFreeListStart;// First unused entry
    int                         RectsPackedCount;       // Number of packed rectangles.
    int                         RectsPackedSurface;     // Number of packed pixels. Used when compacting to heuristically find the ideal texture size.
    int                         RectsDiscardedCount;
    int                         RectsDiscardedSurface;
    int                         FrameCount;             // Current frame count
    VanVec2i                     MaxRectSize;            // Largest rectangle to pack (de-facto used as a "minimum texture size")
    VanVec2i                     MaxRectBounds;          // Bottom-right most used pixels
    bool                        LockDisableResize;      // Disable resizing texture
    bool                        PreloadedAllGlyphsRanges; // Set when missing VanGuiBackendFlags_RendererHasTextures features forces atlas to preload everything.

    // Cache of all VanFontBaked
    VanStableVector<VanFontBaked,32> BakedPool;
    VanGuiStorage                BakedMap;               // BakedId --> VanFontBaked*
    int                         BakedDiscardedCount;

    // Custom rectangle identifiers
    VanFontAtlasRectId           PackIdMouseCursors;     // White pixel + mouse cursors. Also happen to be fallback in case of packing failure.
    VanFontAtlasRectId           PackIdLinesTexData;

    VanFontAtlasBuilder()        { memset((void*)this, 0, sizeof(*this)); FrameCount = -1; RectsIndexFreeListStart = -1; PackIdMouseCursors = PackIdLinesTexData = -1; } // VanVector<>/VanStableVector<>/VanGuiStorage members are zero-init-safe (Data=nullptr, Size=0, Capacity=0)
};

VANGUI_API void              VanFontAtlasBuildInit(VanFontAtlas* atlas);
VANGUI_API void              VanFontAtlasBuildDestroy(VanFontAtlas* atlas);
VANGUI_API void              VanFontAtlasBuildMain(VanFontAtlas* atlas);
VANGUI_API void              VanFontAtlasBuildSetupFontLoader(VanFontAtlas* atlas, const VanFontLoader* font_loader);
VANGUI_API void              VanFontAtlasBuildNotifySetFont(VanFontAtlas* atlas, VanFont* old_font, VanFont* new_font);
VANGUI_API void              VanFontAtlasBuildUpdatePointers(VanFontAtlas* atlas);
VANGUI_API void              VanFontAtlasBuildRenderBitmapFromString(VanFontAtlas* atlas, int x, int y, int w, int h, const char* in_str, char in_marker_char);
VANGUI_API void              VanFontAtlasBuildClear(VanFontAtlas* atlas); // Clear output and custom rects

VANGUI_API VanTextureData*    VanFontAtlasTextureAdd(VanFontAtlas* atlas, int w, int h);
VANGUI_API void              VanFontAtlasTextureMakeSpace(VanFontAtlas* atlas);
VANGUI_API void              VanFontAtlasTextureRepack(VanFontAtlas* atlas, int w, int h);
VANGUI_API void              VanFontAtlasTextureGrow(VanFontAtlas* atlas, int old_w = -1, int old_h = -1);
VANGUI_API void              VanFontAtlasTextureCompact(VanFontAtlas* atlas);
VANGUI_API VanVec2i           VanFontAtlasTextureGetSizeEstimate(VanFontAtlas* atlas);

VANGUI_API void              VanFontAtlasBuildSetupFontSpecialGlyphs(VanFontAtlas* atlas, VanFont* font, VanFontConfig* src);
VANGUI_API void              VanFontAtlasBuildLegacyPreloadAllGlyphRanges(VanFontAtlas* atlas); // Legacy
VANGUI_API void              VanFontAtlasBuildGetOversampleFactors(VanFontConfig* src, VanFontBaked* baked, int* out_oversample_h, int* out_oversample_v);
VANGUI_API void              VanFontAtlasBuildDiscardBakes(VanFontAtlas* atlas, int unused_frames);

VANGUI_API bool              VanFontAtlasFontSourceInit(VanFontAtlas* atlas, VanFontConfig* src);
VANGUI_API void              VanFontAtlasFontSourceAddToFont(VanFontAtlas* atlas, VanFont* font, VanFontConfig* src);
VANGUI_API void              VanFontAtlasFontDestroySourceData(VanFontAtlas* atlas, VanFontConfig* src);
VANGUI_API bool              VanFontAtlasFontInitOutput(VanFontAtlas* atlas, VanFont* font); // Using FontDestroyOutput/FontInitOutput sequence useful notably if font loader params have changed
VANGUI_API void              VanFontAtlasFontDestroyOutput(VanFontAtlas* atlas, VanFont* font);
VANGUI_API void              VanFontAtlasFontRebuildOutput(VanFontAtlas* atlas, VanFont* font);
VANGUI_API void              VanFontAtlasFontDiscardBakes(VanFontAtlas* atlas, VanFont* font, int unused_frames);

VANGUI_API VanGuiID           VanFontAtlasBakedGetId(VanGuiID font_id, float baked_size, float rasterizer_density);
VANGUI_API VanFontBaked*      VanFontAtlasBakedGetOrAdd(VanFontAtlas* atlas, VanFont* font, float font_size, float font_rasterizer_density);
VANGUI_API VanFontBaked*      VanFontAtlasBakedGetClosestMatch(VanFontAtlas* atlas, VanFont* font, float font_size, float font_rasterizer_density);
VANGUI_API VanFontBaked*      VanFontAtlasBakedAdd(VanFontAtlas* atlas, VanFont* font, float font_size, float font_rasterizer_density, VanGuiID baked_id);
VANGUI_API void              VanFontAtlasBakedDiscard(VanFontAtlas* atlas, VanFont* font, VanFontBaked* baked);
VANGUI_API VanFontGlyph*      VanFontAtlasBakedAddFontGlyph(VanFontAtlas* atlas, VanFontBaked* baked, VanFontConfig* src, const VanFontGlyph* in_glyph);
VANGUI_API void              VanFontAtlasBakedAddFontGlyphAdvancedX(VanFontAtlas* atlas, VanFontBaked* baked, VanFontConfig* src, VanWchar codepoint, float advance_x);
VANGUI_API void              VanFontAtlasBakedDiscardFontGlyph(VanFontAtlas* atlas, VanFont* font, VanFontBaked* baked, VanFontGlyph* glyph);
VANGUI_API void              VanFontAtlasBakedSetFontGlyphBitmap(VanFontAtlas* atlas, VanFontBaked* baked, VanFontConfig* src, VanFontGlyph* glyph, VanTextureRect* r, const unsigned char* src_pixels, VanTextureFormat src_fmt, int src_pitch);

VANGUI_API void              VanFontAtlasPackInit(VanFontAtlas* atlas);
VANGUI_API VanFontAtlasRectId VanFontAtlasPackAddRect(VanFontAtlas* atlas, int w, int h, VanFontAtlasRectEntry* overwrite_entry = nullptr);
VANGUI_API VanTextureRect*    VanFontAtlasPackGetRect(VanFontAtlas* atlas, VanFontAtlasRectId id);
VANGUI_API VanTextureRect*    VanFontAtlasPackGetRectSafe(VanFontAtlas* atlas, VanFontAtlasRectId id);
VANGUI_API void              VanFontAtlasPackDiscardRect(VanFontAtlas* atlas, VanFontAtlasRectId id);

VANGUI_API void              VanFontAtlasUpdateNewFrame(VanFontAtlas* atlas, int frame_count, bool renderer_has_textures);
VANGUI_API void              VanFontAtlasAddDrawListSharedData(VanFontAtlas* atlas, VanDrawListSharedData* data);
VANGUI_API void              VanFontAtlasRemoveDrawListSharedData(VanFontAtlas* atlas, VanDrawListSharedData* data);
VANGUI_API void              VanFontAtlasUpdateDrawListsTextures(VanFontAtlas* atlas, VanTextureRef old_tex, VanTextureRef new_tex);
VANGUI_API void              VanFontAtlasUpdateDrawListsSharedData(VanFontAtlas* atlas);

VANGUI_API void              VanFontAtlasTextureBlockConvert(const unsigned char* src_pixels, VanTextureFormat src_fmt, int src_pitch, unsigned char* dst_pixels, VanTextureFormat dst_fmt, int dst_pitch, int w, int h);
VANGUI_API void              VanFontAtlasTextureBlockPostProcess(VanFontAtlasPostProcessData* data);
VANGUI_API void              VanFontAtlasTextureBlockPostProcessMultiply(VanFontAtlasPostProcessData* data, float multiply_factor);
VANGUI_API void              VanFontAtlasTextureBlockFill(VanTextureData* dst_tex, int dst_x, int dst_y, int w, int h, VanU32 col);
VANGUI_API void              VanFontAtlasTextureBlockCopy(VanTextureData* src_tex, int src_x, int src_y, VanTextureData* dst_tex, int dst_x, int dst_y, int w, int h);
VANGUI_API void              VanFontAtlasTextureBlockQueueUpload(VanFontAtlas* atlas, VanTextureData* tex, int x, int y, int w, int h);

VANGUI_API bool              VanTextureDataUpdateNewFrame(VanTextureData* tex);
VANGUI_API void              VanTextureDataQueueUpload(VanTextureData* tex, int x, int y, int w, int h);
VANGUI_API int               VanTextureDataGetFormatBytesPerPixel(VanTextureFormat format);
VANGUI_API const char*       VanTextureDataGetStatusName(VanTextureStatus status);
VANGUI_API const char*       VanTextureDataGetFormatName(VanTextureFormat format);

#ifndef VANGUI_DISABLE_DEBUG_TOOLS
VANGUI_API void              VanFontAtlasDebugLogTextureRequests(VanFontAtlas* atlas);
#endif

VANGUI_API bool      VanFontAtlasGetMouseCursorTexData(VanFontAtlas* atlas, VanGuiMouseCursor cursor_type, VanVec2* out_offset, VanVec2* out_size, VanVec2 out_uv_border[2], VanVec2 out_uv_fill[2]);

//-----------------------------------------------------------------------------
// [SECTION] Test Engine specific hooks (vangui_test_engine)
//-----------------------------------------------------------------------------

#ifdef VANGUI_ENABLE_TEST_ENGINE
extern void         VanGuiTestEngineHook_ItemAdd(VanGuiContext* ctx, VanGuiID id, const VanRect& bb, const VanGuiLastItemData* item_data);           // item_data may be nullptr
extern void         VanGuiTestEngineHook_ItemInfo(VanGuiContext* ctx, VanGuiID id, const char* label, VanGuiItemStatusFlags flags);
extern void         VanGuiTestEngineHook_Log(VanGuiContext* ctx, const char* fmt, ...);
extern const char*  VanGuiTestEngine_FindItemDebugLabel(VanGuiContext* ctx, VanGuiID id);

// In VANGUI_VERSION_NUM >= 18934: changed VANGUI_TEST_ENGINE_ITEM_ADD(bb,id) to VANGUI_TEST_ENGINE_ITEM_ADD(id,bb,item_data);
#define VANGUI_TEST_ENGINE_ITEM_ADD(_ID,_BB,_ITEM_DATA)      if (g.TestEngineHookItems) VanGuiTestEngineHook_ItemAdd(&g, _ID, _BB, _ITEM_DATA)    // Register item bounding box
#define VANGUI_TEST_ENGINE_ITEM_INFO(_ID,_LABEL,_FLAGS)      if (g.TestEngineHookItems) VanGuiTestEngineHook_ItemInfo(&g, _ID, _LABEL, _FLAGS)    // Register item label and status flags (optional)
#define VANGUI_TEST_ENGINE_LOG(_FMT,...)                     VanGuiTestEngineHook_Log(&g, _FMT, __VA_ARGS__)                                      // Custom log entry from user land into test log
#else
#define VANGUI_TEST_ENGINE_ITEM_ADD(_ID,_BB,_ITEM_DATA)      ((void)0)
#define VANGUI_TEST_ENGINE_ITEM_INFO(_ID,_LABEL,_FLAGS)      ((void)g)
#endif

//-----------------------------------------------------------------------------

#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#ifdef _MSC_VER
#pragma warning (pop)
#endif

#endif // #ifndef VANGUI_DISABLE
