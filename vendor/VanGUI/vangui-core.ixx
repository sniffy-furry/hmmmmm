// VanGUI Core Module Partition — context, math types, window API
module;

// vangui.h provided by vangui_module_pch.h (PCH force-included before this file)

export module vangui:core;

// ---------------------------------------------------------------------------
// Version constants — forwarded as constexpr values from the macros in vangui.h
// ---------------------------------------------------------------------------
export constexpr const char* VANGUI_VERSION_STR = VANGUI_VERSION;
export constexpr int         VANGUI_VERSION_INT  = VANGUI_VERSION_NUM;

export namespace VanGui {

    // -----------------------------------------------------------------------
    // Math types
    // -----------------------------------------------------------------------
    using ::VanVec2;
    using ::VanVec4;

    // -----------------------------------------------------------------------
    // Scalar / character / callback typedefs (needed by the types below)
    // -----------------------------------------------------------------------
    using ::VanGuiID;
    using ::VanS8;
    using ::VanU8;
    using ::VanS16;
    using ::VanU16;
    using ::VanS32;
    using ::VanU32;
    using ::VanS64;
    using ::VanU64;
    using ::VanWchar;
    using ::VanWchar16;
    using ::VanWchar32;
    using ::VanGuiSelectionUserData;
    using ::VanGuiInputTextCallback;
    using ::VanGuiSizeCallback;
    using ::VanGuiMemAllocFunc;
    using ::VanGuiMemFreeFunc;

    // -----------------------------------------------------------------------
    // Texture types (referenced by VanGuiIO / draw API)
    // -----------------------------------------------------------------------
    using ::VanTextureID;
    using ::VanTextureRef;

    // -----------------------------------------------------------------------
    // Core enum / flag typedefs
    // -----------------------------------------------------------------------

    // Enums (strongly-typed)
    using ::VanGuiDir;
    using ::VanGuiKey;
    using ::VanGuiMouseSource;
    using ::VanGuiSortDirection;

    // Enum aliases (int-backed)
    using ::VanGuiCol;
    using ::VanGuiCond;
    using ::VanGuiDataType;
    using ::VanGuiMouseButton;
    using ::VanGuiMouseCursor;
    using ::VanGuiStyleVar;
    using ::VanGuiTableBgTarget;

    // Flag types
    using ::VanGuiWindowFlags;
    using ::VanGuiChildFlags;
    using ::VanGuiInputTextFlags;
    using ::VanGuiBackendFlags;
    using ::VanGuiButtonFlags;
    using ::VanGuiColorEditFlags;
    using ::VanGuiConfigFlags;
    using ::VanGuiComboFlags;
    using ::VanGuiDragDropFlags;
    using ::VanGuiFocusedFlags;
    using ::VanGuiHoveredFlags;
    using ::VanGuiInputFlags;
    using ::VanGuiItemFlags;
    using ::VanGuiKeyChord;
    using ::VanGuiListClipperFlags;
    using ::VanGuiPopupFlags;
    using ::VanGuiMultiSelectFlags;
    using ::VanGuiSelectableFlags;
    using ::VanGuiSliderFlags;
    using ::VanGuiTabBarFlags;
    using ::VanGuiTabItemFlags;
    using ::VanGuiTableFlags;
    using ::VanGuiTableColumnFlags;
    using ::VanGuiTableRowFlags;
    using ::VanGuiTreeNodeFlags;
    using ::VanGuiViewportFlags;
    using ::VanDrawFlags;
    using ::VanDrawListFlags;

    // -----------------------------------------------------------------------
    // Forward-declared / opaque context & style types
    // -----------------------------------------------------------------------
    using ::VanGuiContext;
    using ::VanGuiIO;
    using ::VanGuiStyle;
    using ::VanGuiPlatformIO;
    using ::VanGuiViewport;
    using ::VanGuiKeyData;
    using ::VanGuiListClipper;
    using ::VanGuiMultiSelectIO;
    using ::VanGuiPayload;
    using ::VanGuiSelectionBasicStorage;
    using ::VanGuiSelectionExternalStorage;
    using ::VanGuiSelectionRequest;
    using ::VanGuiSizeCallbackData;
    using ::VanGuiInputTextCallbackData;
    using ::VanGuiStorage;
    using ::VanGuiStoragePair;
    using ::VanGuiTableSortSpecs;
    using ::VanGuiTableColumnSortSpecs;
    using ::VanGuiTextBuffer;
    using ::VanGuiTextFilter;
    using ::VanGuiOnceUponAFrame;

    // Draw layer types used by the core API
    using ::VanDrawList;
    using ::VanDrawData;
    using ::VanDrawCmd;
    using ::VanDrawVert;
    using ::VanDrawChannel;
    using ::VanDrawListSplitter;
    using ::VanFontAtlas;
    using ::VanFont;
    using ::VanColor;

    // -----------------------------------------------------------------------
    // Context creation & access
    // -----------------------------------------------------------------------
    using VanGui::CreateContext;
    using VanGui::DestroyContext;
    using VanGui::GetCurrentContext;
    using VanGui::SetCurrentContext;

    // -----------------------------------------------------------------------
    // Main frame / IO accessors
    // -----------------------------------------------------------------------
    using VanGui::GetIO;
    using VanGui::GetPlatformIO;
    using VanGui::GetStyle;
    using VanGui::NewFrame;
    using VanGui::EndFrame;
    using VanGui::Render;
    using VanGui::GetDrawData;

    // -----------------------------------------------------------------------
    // Window API
    // -----------------------------------------------------------------------
    using VanGui::Begin;
    using VanGui::End;
    using VanGui::BeginChild;
    using VanGui::EndChild;

    // -----------------------------------------------------------------------
    // Version query
    // -----------------------------------------------------------------------
    using VanGui::GetVersion;

} // export namespace VanGui
