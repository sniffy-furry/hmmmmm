// VanGUI Tables Module Partition — thin interface
// Implementation lives in vangui-tables-impl.cpp (partition implementation unit).
// Intentionally empty global module fragment — prevents MSVC C2572.
module;
// intentionally empty global module fragment

export module vangui:tables;

//=============================================================================
// EXPORTED PUBLIC API — Tables API visible to importers
//=============================================================================
export namespace VanGui {

    // -------------------------------------------------------------------------
    // Flags / enum aliases
    // -------------------------------------------------------------------------
    using ::VanGuiTableFlags;
    using ::VanGuiTableColumnFlags;
    using ::VanGuiTableRowFlags;
    using ::VanGuiTableBgTarget;

    // Enum values — VanGuiTableFlags_
    using ::VanGuiTableFlags_None;
    using ::VanGuiTableFlags_Resizable;
    using ::VanGuiTableFlags_Reorderable;
    using ::VanGuiTableFlags_Hideable;
    using ::VanGuiTableFlags_Sortable;
    using ::VanGuiTableFlags_NoSavedSettings;
    using ::VanGuiTableFlags_ContextMenuInBody;
    using ::VanGuiTableFlags_RowBg;
    using ::VanGuiTableFlags_BordersInnerH;
    using ::VanGuiTableFlags_BordersOuterH;
    using ::VanGuiTableFlags_BordersInnerV;
    using ::VanGuiTableFlags_BordersOuterV;
    using ::VanGuiTableFlags_BordersH;
    using ::VanGuiTableFlags_BordersV;
    using ::VanGuiTableFlags_BordersInner;
    using ::VanGuiTableFlags_BordersOuter;
    using ::VanGuiTableFlags_Borders;
    using ::VanGuiTableFlags_NoBordersInBody;
    using ::VanGuiTableFlags_NoBordersInBodyUntilResize;
    using ::VanGuiTableFlags_SizingFixedFit;
    using ::VanGuiTableFlags_SizingFixedSame;
    using ::VanGuiTableFlags_SizingStretchProp;
    using ::VanGuiTableFlags_SizingStretchSame;
    using ::VanGuiTableFlags_NoHostExtendX;
    using ::VanGuiTableFlags_NoHostExtendY;
    using ::VanGuiTableFlags_NoKeepColumnsVisible;
    using ::VanGuiTableFlags_PreciseWidths;
    using ::VanGuiTableFlags_NoClip;
    using ::VanGuiTableFlags_PadOuterX;
    using ::VanGuiTableFlags_NoPadOuterX;
    using ::VanGuiTableFlags_NoPadInnerX;
    using ::VanGuiTableFlags_ScrollX;
    using ::VanGuiTableFlags_ScrollY;
    using ::VanGuiTableFlags_SortMulti;
    using ::VanGuiTableFlags_SortTristate;
    using ::VanGuiTableFlags_HighlightHoveredColumn;
    using ::VanGuiTableFlags_SizingMask_;

    // Enum values — VanGuiTableColumnFlags_
    using ::VanGuiTableColumnFlags_None;
    using ::VanGuiTableColumnFlags_Disabled;
    using ::VanGuiTableColumnFlags_DefaultHide;
    using ::VanGuiTableColumnFlags_DefaultSort;
    using ::VanGuiTableColumnFlags_WidthStretch;
    using ::VanGuiTableColumnFlags_WidthFixed;
    using ::VanGuiTableColumnFlags_NoResize;
    using ::VanGuiTableColumnFlags_NoReorder;
    using ::VanGuiTableColumnFlags_NoHide;
    using ::VanGuiTableColumnFlags_NoClip;
    using ::VanGuiTableColumnFlags_NoSort;
    using ::VanGuiTableColumnFlags_NoSortAscending;
    using ::VanGuiTableColumnFlags_NoSortDescending;
    using ::VanGuiTableColumnFlags_NoHeaderLabel;
    using ::VanGuiTableColumnFlags_NoHeaderWidth;
    using ::VanGuiTableColumnFlags_PreferSortAscending;
    using ::VanGuiTableColumnFlags_PreferSortDescending;
    using ::VanGuiTableColumnFlags_IndentEnable;
    using ::VanGuiTableColumnFlags_IndentDisable;
    using ::VanGuiTableColumnFlags_AngledHeader;
    using ::VanGuiTableColumnFlags_IsEnabled;
    using ::VanGuiTableColumnFlags_IsVisible;
    using ::VanGuiTableColumnFlags_IsSorted;
    using ::VanGuiTableColumnFlags_IsHovered;
    using ::VanGuiTableColumnFlags_WidthMask_;
    using ::VanGuiTableColumnFlags_IndentMask_;
    using ::VanGuiTableColumnFlags_StatusMask_;
    using ::VanGuiTableColumnFlags_NoDirectResize_;

    // Enum values — VanGuiTableRowFlags_
    using ::VanGuiTableRowFlags_None;
    using ::VanGuiTableRowFlags_Headers;

    // Enum values — VanGuiTableBgTarget_
    using ::VanGuiTableBgTarget_None;
    using ::VanGuiTableBgTarget_RowBg0;
    using ::VanGuiTableBgTarget_RowBg1;
    using ::VanGuiTableBgTarget_CellBg;

    // Sort direction
    using ::VanGuiSortDirection;
    using ::VanGuiSortDirection_None;
    using ::VanGuiSortDirection_Ascending;
    using ::VanGuiSortDirection_Descending;

    // Structs
    using ::VanGuiTableSortSpecs;
    using ::VanGuiTableColumnSortSpecs;

    // -------------------------------------------------------------------------
    // Tables API
    // -------------------------------------------------------------------------
    using VanGui::BeginTable;
    using VanGui::EndTable;
    using VanGui::TableNextRow;
    using VanGui::TableNextColumn;
    using VanGui::TableSetColumnIndex;
    using VanGui::TableSetupColumn;
    using VanGui::TableSetupScrollFreeze;
    using VanGui::TableHeader;
    using VanGui::TableHeadersRow;
    using VanGui::TableAngledHeadersRow;
    using VanGui::TableGetSortSpecs;
    using VanGui::TableGetColumnCount;
    using VanGui::TableGetColumnIndex;
    using VanGui::TableGetRowIndex;
    using VanGui::TableGetColumnName;
    using VanGui::TableGetColumnFlags;
    using VanGui::TableSetColumnEnabled;
    using VanGui::TableGetHoveredColumn;
    using VanGui::TableSetBgColor;

} // export namespace VanGui
