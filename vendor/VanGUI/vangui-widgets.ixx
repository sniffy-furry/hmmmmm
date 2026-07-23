// VanGUI Widgets Module Partition — thin interface
// Implementation lives in vangui-widgets-impl.cpp (partition implementation unit).
// Intentionally empty global module fragment — prevents MSVC C2572.
module;
// intentionally empty global module fragment

export module vangui:widgets;

//=============================================================================
// EXPORTED PUBLIC API — widget and UI functions visible to importers
//=============================================================================

export namespace VanGui {

    // -------------------------------------------------------------------------
    // Widgets: Text
    // -------------------------------------------------------------------------
    using VanGui::TextUnformatted;
    using VanGui::Text;
    using VanGui::TextV;
    using VanGui::TextColored;
    using VanGui::TextColoredV;
    using VanGui::TextDisabled;
    using VanGui::TextDisabledV;
    using VanGui::TextWrapped;
    using VanGui::TextWrappedV;
    using VanGui::LabelText;
    using VanGui::LabelTextV;
    using VanGui::BulletText;
    using VanGui::BulletTextV;
    using VanGui::SeparatorText;

    // -------------------------------------------------------------------------
    // Widgets: Main (Buttons, Checkbox, RadioButton, ProgressBar, Links)
    // -------------------------------------------------------------------------
    using VanGui::Button;
    using VanGui::SmallButton;
    using VanGui::InvisibleButton;
    using VanGui::ArrowButton;
    using VanGui::Checkbox;
    using VanGui::CheckboxFlags;
    using VanGui::RadioButton;
    using VanGui::ProgressBar;
    using VanGui::Bullet;
    using VanGui::TextLink;
    using VanGui::TextLinkOpenURL;

    // -------------------------------------------------------------------------
    // Widgets: Images
    // -------------------------------------------------------------------------
    using VanGui::Image;
    using VanGui::ImageWithBg;
    using VanGui::ImageButton;

    // -------------------------------------------------------------------------
    // Widgets: Combo Box (Dropdown)
    // -------------------------------------------------------------------------
    using VanGui::BeginCombo;
    using VanGui::EndCombo;
    using VanGui::Combo;

    // -------------------------------------------------------------------------
    // Widgets: Drag Sliders
    // -------------------------------------------------------------------------
    using VanGui::DragFloat;
    using VanGui::DragFloat2;
    using VanGui::DragFloat3;
    using VanGui::DragFloat4;
    using VanGui::DragFloatRange2;
    using VanGui::DragInt;
    using VanGui::DragInt2;
    using VanGui::DragInt3;
    using VanGui::DragInt4;
    using VanGui::DragIntRange2;
    using VanGui::DragScalar;
    using VanGui::DragScalarN;

    // -------------------------------------------------------------------------
    // Widgets: Regular Sliders
    // -------------------------------------------------------------------------
    using VanGui::SliderFloat;
    using VanGui::SliderFloat2;
    using VanGui::SliderFloat3;
    using VanGui::SliderFloat4;
    using VanGui::SliderAngle;
    using VanGui::SliderInt;
    using VanGui::SliderInt2;
    using VanGui::SliderInt3;
    using VanGui::SliderInt4;
    using VanGui::SliderScalar;
    using VanGui::SliderScalarN;
    using VanGui::VSliderFloat;
    using VanGui::VSliderInt;
    using VanGui::VSliderScalar;

    // -------------------------------------------------------------------------
    // Widgets: Input with Keyboard
    // -------------------------------------------------------------------------
    using VanGui::InputText;
    using VanGui::InputTextMultiline;
    using VanGui::InputTextWithHint;
    using VanGui::InputFloat;
    using VanGui::InputFloat2;
    using VanGui::InputFloat3;
    using VanGui::InputFloat4;
    using VanGui::InputInt;
    using VanGui::InputInt2;
    using VanGui::InputInt3;
    using VanGui::InputInt4;
    using VanGui::InputDouble;
    using VanGui::InputScalar;
    using VanGui::InputScalarN;

    // -------------------------------------------------------------------------
    // Widgets: Color Editor/Picker
    // -------------------------------------------------------------------------
    using VanGui::ColorEdit3;
    using VanGui::ColorEdit4;
    using VanGui::ColorPicker3;
    using VanGui::ColorPicker4;
    using VanGui::ColorButton;
    using VanGui::SetColorEditOptions;

    // -------------------------------------------------------------------------
    // Widgets: Trees
    // -------------------------------------------------------------------------
    using VanGui::TreeNode;
    using VanGui::TreeNodeV;
    using VanGui::TreeNodeEx;
    using VanGui::TreeNodeExV;
    using VanGui::TreePush;
    using VanGui::TreePop;
    using VanGui::GetTreeNodeToLabelSpacing;
    using VanGui::CollapsingHeader;
    using VanGui::SetNextItemOpen;
    using VanGui::SetNextItemStorageID;
    using VanGui::TreeNodeGetOpen;

    // -------------------------------------------------------------------------
    // Widgets: Selectables
    // -------------------------------------------------------------------------
    using VanGui::Selectable;

    // -------------------------------------------------------------------------
    // Widgets: Multi-Selection
    // -------------------------------------------------------------------------
    using VanGui::BeginMultiSelect;
    using VanGui::EndMultiSelect;
    using VanGui::SetNextItemSelectionUserData;
    using VanGui::IsItemToggledSelection;

    // -------------------------------------------------------------------------
    // Widgets: List Boxes
    // -------------------------------------------------------------------------
    using VanGui::BeginListBox;
    using VanGui::EndListBox;
    using VanGui::ListBox;

    // -------------------------------------------------------------------------
    // Widgets: Data Plotting
    // -------------------------------------------------------------------------
    using VanGui::PlotLines;
    using VanGui::PlotHistogram;

    // -------------------------------------------------------------------------
    // Widgets: Value() Helpers
    // -------------------------------------------------------------------------
    using VanGui::Value;

    // -------------------------------------------------------------------------
    // Widgets: Menus
    // -------------------------------------------------------------------------
    using VanGui::BeginMenuBar;
    using VanGui::EndMenuBar;
    using VanGui::BeginMainMenuBar;
    using VanGui::EndMainMenuBar;
    using VanGui::BeginMenu;
    using VanGui::EndMenu;
    using VanGui::MenuItem;

    // -------------------------------------------------------------------------
    // Tooltips
    // -------------------------------------------------------------------------
    using VanGui::BeginTooltip;
    using VanGui::EndTooltip;
    using VanGui::SetTooltip;
    using VanGui::SetTooltipV;
    using VanGui::BeginItemTooltip;
    using VanGui::SetItemTooltip;
    using VanGui::SetItemTooltipV;

    // -------------------------------------------------------------------------
    // Popups & Modals
    // -------------------------------------------------------------------------
    using VanGui::BeginPopup;
    using VanGui::BeginPopupModal;
    using VanGui::EndPopup;
    using VanGui::OpenPopup;
    using VanGui::OpenPopupOnItemClick;
    using VanGui::CloseCurrentPopup;
    using VanGui::BeginPopupContextItem;
    using VanGui::BeginPopupContextWindow;
    using VanGui::BeginPopupContextVoid;
    using VanGui::IsPopupOpen;

    // -------------------------------------------------------------------------
    // Tables
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

    // -------------------------------------------------------------------------
    // Legacy Columns API
    // -------------------------------------------------------------------------
    using VanGui::Columns;
    using VanGui::NextColumn;
    using VanGui::GetColumnIndex;
    using VanGui::GetColumnWidth;
    using VanGui::SetColumnWidth;
    using VanGui::GetColumnOffset;
    using VanGui::SetColumnOffset;
    using VanGui::GetColumnsCount;

    // -------------------------------------------------------------------------
    // Tab Bars, Tabs
    // -------------------------------------------------------------------------
    using VanGui::BeginTabBar;
    using VanGui::EndTabBar;
    using VanGui::BeginTabItem;
    using VanGui::EndTabItem;
    using VanGui::TabItemButton;
    using VanGui::SetTabItemClosed;

    // -------------------------------------------------------------------------
    // Drag and Drop
    // -------------------------------------------------------------------------
    using VanGui::BeginDragDropSource;
    using VanGui::SetDragDropPayload;
    using VanGui::EndDragDropSource;
    using VanGui::BeginDragDropTarget;
    using VanGui::AcceptDragDropPayload;
    using VanGui::EndDragDropTarget;
    using VanGui::GetDragDropPayload;

    // -------------------------------------------------------------------------
    // Disabling
    // -------------------------------------------------------------------------
    using VanGui::BeginDisabled;
    using VanGui::EndDisabled;

    // -------------------------------------------------------------------------
    // Item/Widget Utilities and Query Functions
    // -------------------------------------------------------------------------
    using VanGui::IsItemHovered;
    using VanGui::IsItemActive;
    using VanGui::IsItemFocused;
    using VanGui::IsItemClicked;
    using VanGui::IsItemVisible;
    using VanGui::IsItemEdited;
    using VanGui::IsItemActivated;
    using VanGui::IsItemDeactivated;
    using VanGui::IsItemDeactivatedAfterEdit;
    using VanGui::IsItemToggledOpen;
    using VanGui::IsAnyItemHovered;
    using VanGui::IsAnyItemActive;
    using VanGui::IsAnyItemFocused;
    using VanGui::GetItemID;
    using VanGui::GetItemRectMin;
    using VanGui::GetItemRectMax;
    using VanGui::GetItemRectSize;
    using VanGui::GetItemFlags;
    using VanGui::SetItemDefaultFocus;
    using VanGui::SetKeyboardFocusHere;
    using VanGui::SetNextItemAllowOverlap;
    using VanGui::SetNextItemWidth;
    using VanGui::SetNextItemShortcut;
    using VanGui::SetItemKeyOwner;
    using VanGui::SetNextItemSelectionUserData;

    // -------------------------------------------------------------------------
    // Flag / Enum Types
    // -------------------------------------------------------------------------

    // Window & Child
    using ::VanGuiWindowFlags_;
    using ::VanGuiChildFlags_;

    // Item
    using ::VanGuiItemFlags_;

    // Input
    using ::VanGuiInputTextFlags_;
    using ::VanGuiInputFlags_;

    // Trees / Collapsing
    using ::VanGuiTreeNodeFlags_;

    // Popups
    using ::VanGuiPopupFlags_;

    // Selectables
    using ::VanGuiSelectableFlags_;

    // Combo
    using ::VanGuiComboFlags_;

    // Tab bars / items
    using ::VanGuiTabBarFlags_;
    using ::VanGuiTabItemFlags_;

    // Focus / Hover
    using ::VanGuiFocusedFlags_;
    using ::VanGuiHoveredFlags_;

    // Drag and Drop
    using ::VanGuiDragDropFlags_;

    // Buttons
    using ::VanGuiButtonFlags_;

    // Color
    using ::VanGuiColorEditFlags_;

    // Sliders / Drag
    using ::VanGuiSliderFlags_;

    // Tables
    using ::VanGuiTableFlags_;
    using ::VanGuiTableColumnFlags_;
    using ::VanGuiTableRowFlags_;
    using ::VanGuiTableBgTarget_;

    // Multi-Select
    using ::VanGuiMultiSelectFlags_;

    // Data types & directions
    using ::VanGuiDataType_;
    using ::VanGuiDir;
    using ::VanGuiSortDirection;

} // export namespace VanGui
