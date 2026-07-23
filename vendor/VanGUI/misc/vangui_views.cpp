// vangui_views.cpp — Pillar 3 model/view adapters. See vangui_views.h.
// Empty TU unless VANGUI_ENABLE_VIEWS is defined.

#include "vangui_views.h"

#ifdef VANGUI_ENABLE_VIEWS

#include <cstdio>

namespace VanGui {

namespace {
// Track whether the matching End* should call EndChild (only when Begin opened one).
struct ViewFrame { bool child; };
constexpr int kMaxViews = 16;
static ViewFrame s_ListStack[kMaxViews]; static int s_ListDepth = 0;
static ViewFrame s_TreeStack[kMaxViews]; static int s_TreeDepth = 0;
}

bool BeginListView(const char* id, const VanListModel& model, int* p_selected, const VanVec2& size)
{
    bool changed = false;
    const bool child = BeginChild(id, size, VanGuiChildFlags_Borders);
    if (s_ListDepth < kMaxViews) s_ListStack[s_ListDepth++] = { child };
    if (!child)
        return false;

    const int count = model.RowCount ? model.RowCount(model.UserData) : 0;
    const float row_h = GetTextLineHeightWithSpacing();

    VanGuiListClipper clipper;
    clipper.Begin(count, row_h);
    while (clipper.Step())
    {
        for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row)
        {
            PushID(row);
            const bool selected = p_selected && (*p_selected == row);

            // Selectable spanning the row provides hit-testing + highlight; the
            // model draws the actual contents on top via DrawRow.
            if (Selectable("##row", selected,
                           VanGuiSelectableFlags_SpanAllColumns | VanGuiSelectableFlags_AllowOverlap))
            {
                if (p_selected && *p_selected != row) { *p_selected = row; changed = true; }
            }
            // Re-anchor the cursor to the row start so DrawRow renders in place.
            SameLine(0.0f, 0.0f);
            if (model.DrawRow) model.DrawRow(model.UserData, row, selected);
            PopID();
        }
    }
    return changed;
}

void EndListView()
{
    if (s_ListDepth <= 0) return;
    const ViewFrame f = s_ListStack[--s_ListDepth];
    // BeginChild must always be paired with EndChild regardless of return value.
    (void)f;
    EndChild();
}

namespace {
bool DrawTreeNode(const VanTreeModel& model, int node, int* p_selected, bool& changed)
{
    const bool leaf     = model.IsLeaf ? model.IsLeaf(model.UserData, node) : false;
    const bool selected = p_selected && (*p_selected == node);

    VanGuiTreeNodeFlags flags = VanGuiTreeNodeFlags_OpenOnArrow |
                                VanGuiTreeNodeFlags_SpanAvailWidth;
    if (selected) flags |= VanGuiTreeNodeFlags_Selected;
    if (leaf)     flags |= VanGuiTreeNodeFlags_Leaf | VanGuiTreeNodeFlags_NoTreePushOnOpen;

    PushID(node);
    const bool open = TreeNodeEx("##node", flags);
    if (IsItemClicked() && p_selected && *p_selected != node) { *p_selected = node; changed = true; }

    // Draw the node's own contents aligned to the node row.
    SameLine();
    if (model.DrawNode) model.DrawNode(model.UserData, node, selected);

    if (open && !leaf)
    {
        const int n = model.ChildCount ? model.ChildCount(model.UserData, node) : 0;
        for (int i = 0; i < n; ++i)
            DrawTreeNode(model, model.ChildAt(model.UserData, node, i), p_selected, changed);
        TreePop();
    }
    PopID();
    return changed;
}
}

bool BeginTreeView(const char* id, const VanTreeModel& model, int* p_selected, const VanVec2& size)
{
    bool changed = false;
    const bool child = BeginChild(id, size, VanGuiChildFlags_Borders);
    if (s_TreeDepth < kMaxViews) s_TreeStack[s_TreeDepth++] = { child };
    if (!child)
        return false;

    const int roots = model.ChildCount ? model.ChildCount(model.UserData, -1) : 0;
    for (int i = 0; i < roots; ++i)
        DrawTreeNode(model, model.ChildAt(model.UserData, -1, i), p_selected, changed);
    return changed;
}

void EndTreeView()
{
    if (s_TreeDepth <= 0) return;
    const ViewFrame f = s_TreeStack[--s_TreeDepth];
    (void)f;
    EndChild();
}

} // namespace VanGui

#endif // VANGUI_ENABLE_VIEWS
