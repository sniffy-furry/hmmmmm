// VanGUI Draw Module Partition — thin interface
// Implementation lives in vangui-draw-impl.cpp (partition implementation unit).
// This file has an intentionally empty global module fragment so its IFC carries
// no vangui.h declarations — prevents MSVC C2572 when vangui.ixx imports all partitions.
module;
// intentionally empty global module fragment

export module vangui:draw;

//=============================================================================
// EXPORTED PUBLIC API — draw/font types and functions visible to importers
//=============================================================================
export namespace VanGui {

    // -------------------------------------------------------------------------
    // Draw index / callback types
    // -------------------------------------------------------------------------
    using ::VanDrawIdx;
    using ::VanDrawCallback;

    // -------------------------------------------------------------------------
    // Draw flags
    // -------------------------------------------------------------------------
    using ::VanDrawFlags;
    using ::VanDrawFlags_;
    using ::VanDrawListFlags;
    using ::VanDrawListFlags_;

    // -------------------------------------------------------------------------
    // Core draw structures
    // -------------------------------------------------------------------------
    using ::VanDrawCmd;
    using ::VanDrawCmdHeader;
    using ::VanDrawVert;
    using ::VanDrawChannel;
    using ::VanDrawListSplitter;
    using ::VanDrawListSharedData;
    using ::VanDrawList;
    using ::VanDrawData;

    // -------------------------------------------------------------------------
    // Texture API
    // -------------------------------------------------------------------------
    using ::VanTextureData;

    // -------------------------------------------------------------------------
    // Font structures
    // -------------------------------------------------------------------------
    using ::VanFontConfig;
    using ::VanFontGlyph;
    using ::VanFontGlyphRangesBuilder;
    using ::VanFontAtlasRect;
    using ::VanFontAtlasBuilder;
    using ::VanFontAtlas;
    using ::VanFontBaked;
    using ::VanFontLoader;
    using ::VanFont;

    // -------------------------------------------------------------------------
    // Draw-related query functions
    // -------------------------------------------------------------------------
    using ::VanGui::GetDrawData;
    using ::VanGui::GetWindowDrawList;
    using ::VanGui::GetBackgroundDrawList;
    using ::VanGui::GetForegroundDrawList;
    using ::VanGui::GetDrawListSharedData;

} // export namespace VanGui
