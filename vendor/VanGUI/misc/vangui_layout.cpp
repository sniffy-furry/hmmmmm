// vangui_layout.cpp — Pillar 3 layout helpers. See vangui_layout.h.
// Empty TU unless VANGUI_ENABLE_LAYOUT is defined.

#include "vangui_layout.h"

#ifdef VANGUI_ENABLE_LAYOUT

namespace VanGui {

namespace {

enum BoxKind { Box_V, Box_H, Box_Grid };

struct BoxState
{
    BoxKind kind;
    float   spacing;     // resolved spacing along the major axis
    int     columns;     // grid only
    int     cell;        // grid: cells emitted so far
    bool    pushedVar;   // did we push an ItemSpacing style var?
};

constexpr int kMaxDepth = 32;
static BoxState s_Stack[kMaxDepth];
static int      s_Depth = 0;

inline BoxState* Top() { return s_Depth > 0 ? &s_Stack[s_Depth - 1] : nullptr; }

void PushBox(BoxKind kind, float spacing, int columns)
{
    VAN_ASSERT(s_Depth < kMaxDepth && "VanGui layout: box nesting too deep");
    BoxState& b = s_Stack[s_Depth++];
    b.kind     = kind;
    b.columns  = columns;
    b.cell     = 0;
    b.pushedVar = false;

    const VanVec2 cur = GetStyle().ItemSpacing;
    if (spacing >= 0.0f)
    {
        VanVec2 sp = cur;
        if (kind == Box_V) sp.y = spacing;
        else               sp.x = spacing;   // H and Grid use x as major gap
        PushStyleVar(VanGuiStyleVar_ItemSpacing, sp);
        b.pushedVar = true;
        b.spacing   = spacing;
    }
    else
    {
        b.spacing = (kind == Box_V) ? cur.y : cur.x;
    }
    BeginGroup();
}

} // anonymous namespace

void BeginVBox(const char* id, float spacing) { PushID(id); PushBox(Box_V,    spacing, 0); }
void BeginHBox(const char* id, float spacing) { PushID(id); PushBox(Box_H,    spacing, 0); }
void BeginGrid(const char* id, int columns, float spacing)
{
    if (columns < 1) columns = 1;
    PushID(id);
    PushBox(Box_Grid, spacing, columns);
}

void EndBox()
{
    BoxState* b = Top();
    VAN_ASSERT(b && "VanGui::EndBox without a matching Begin*Box");
    if (!b) return;
    EndGroup();
    if (b->pushedVar) PopStyleVar();
    PopID();
    --s_Depth;
}

void NextCell()
{
    BoxState* b = Top();
    if (!b) return;
    if (b->kind == Box_Grid)
    {
        b->cell++;
        if (b->cell % b->columns != 0)
            SameLine(0.0f, b->spacing);   // stay on the row
        // else: fall through to a new row (default vertical advance)
    }
    else if (b->kind == Box_H)
    {
        SameLine(0.0f, b->spacing);
    }
}

void Stretch(float weight)
{
    BoxState* b = Top();
    if (!b) { return; }
    if (weight < 0.0f) weight = 0.0f;

    if (b->kind == Box_V)
    {
        // Vertical flexible spacer: consume remaining column height.
        const float avail = GetContentRegionAvail().y;
        Dummy(VanVec2(0.0f, avail > 0.0f ? avail * weight : 0.0f));
    }
    else
    {
        // Horizontal flexible spacer: keep on the line, consume remaining width.
        SameLine(0.0f, b->spacing);
        const float avail = GetContentRegionAvail().x;
        Dummy(VanVec2(avail > 0.0f ? avail * weight : 0.0f, 0.0f));
        SameLine(0.0f, b->spacing);
    }
}

} // namespace VanGui

#endif // VANGUI_ENABLE_LAYOUT
