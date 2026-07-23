//-----------------------------------------------------------------------------
// vangui_node_graph.cpp  --  Node graph / node editor widget for VanGUI
//-----------------------------------------------------------------------------

#include "vangui_node_graph.h"
#include <string.h>   // memset, snprintf
#include <math.h>     // fabsf, sqrtf, fmodf

//=============================================================================
// Internal constants
//=============================================================================

static const int   NG_MAX_NODES      = 128;   // max nodes tracked per frame
static const int   NG_MAX_PINS       = 16;    // max pins per side per node
static const int   NG_MAX_LINKS      = 512;   // max NodeLink() calls per frame
static const float NG_GRID_STEP      = 32.0f;
static const float NG_GRID_DOT_R     = 1.5f;
static const float NG_NODE_ROUNDING  = 6.0f;
static const float NG_TITLE_H        = 22.0f; // title bar height in graph units
static const float NG_PIN_RADIUS     = 5.0f;  // pin circle radius in screen px (pre-zoom)
static const float NG_PIN_BTN_HALF   = 6.0f;  // half-size of pin hit area (pre-zoom px)
static const float NG_NODE_PAD_X     = 10.0f; // horizontal content padding
static const float NG_NODE_PAD_Y     = 6.0f;  // vertical padding between pins
static const float NG_LINK_THICKNESS = 2.0f;
static const float NG_LINK_HIT_DIST  = 8.0f;  // pixels for link hover/delete
static const float NG_SHADOW_OFFSET  = 4.0f;
static const float NG_ZOOM_MIN       = 0.1f;
static const float NG_ZOOM_MAX       = 4.0f;
static const float NG_ZOOM_SPEED     = 0.1f;

//=============================================================================
// Internal helper math
// Safe without VANGUI_DEFINE_MATH_OPERATORS.
//=============================================================================

static inline VanVec2 V2Add  (VanVec2 a, VanVec2 b) { return VanVec2(a.x + b.x, a.y + b.y); }
static inline VanVec2 V2Sub  (VanVec2 a, VanVec2 b) { return VanVec2(a.x - b.x, a.y - b.y); }
static inline VanVec2 V2Scale(VanVec2 a, float s)   { return VanVec2(a.x * s,   a.y * s);   }
static inline float   V2Dot  (VanVec2 a, VanVec2 b) { return a.x * b.x + a.y * b.y; }
static inline float   V2LenSq(VanVec2 a)             { return a.x * a.x + a.y * a.y; }

// Squared distance from point P to line segment AB
static float PointSegDistSq(VanVec2 p, VanVec2 a, VanVec2 b)
{
    VanVec2 ab = V2Sub(b, a);
    VanVec2 ap = V2Sub(p, a);
    float len_sq = V2LenSq(ab);
    if (len_sq < 1e-6f)
        return V2LenSq(ap);
    float t = V2Dot(ap, ab) / len_sq;
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    VanVec2 proj = V2Add(a, V2Scale(ab, t));
    return V2LenSq(V2Sub(p, proj));
}

// Sample a cubic bezier at t in [0,1]
static VanVec2 BezierSample(VanVec2 p1, VanVec2 p2, VanVec2 p3, VanVec2 p4, float t)
{
    float u   = 1.0f - t;
    float uu  = u * u;
    float uuu = uu * u;
    float tt  = t * t;
    float ttt = tt * t;
    return VanVec2(uuu * p1.x + 3.0f * uu * t * p2.x + 3.0f * u * tt * p3.x + ttt * p4.x,
                   uuu * p1.y + 3.0f * uu * t * p2.y + 3.0f * u * tt * p3.y + ttt * p4.y);
}

// Approximate minimum squared distance from point to a cubic bezier (20 segments)
static float BezierPointDistSq(VanVec2 p,
                                VanVec2 p1, VanVec2 p2, VanVec2 p3, VanVec2 p4)
{
    const int N = 20;
    float best = 1e30f;
    VanVec2 prev = p1;
    for (int i = 1; i <= N; i++)
    {
        VanVec2 cur = BezierSample(p1, p2, p3, p4, (float)i / (float)N);
        float d = PointSegDistSq(p, prev, cur);
        if (d < best) best = d;
        prev = cur;
    }
    return best;
}

// Pack a VanVec4 colour (r,g,b,a in [0,1]) into VanU32 (ABGR byte order, matching VanGUI).
static VanU32 Col4ToU32(VanVec4 c)
{
    auto Clamp = [](float v) -> float { return v < 0.f ? 0.f : (v > 1.f ? 1.f : v); };
    return ((VanU32)(Clamp(c.w) * 255.0f + 0.5f) << 24)
         | ((VanU32)(Clamp(c.z) * 255.0f + 0.5f) << 16)
         | ((VanU32)(Clamp(c.y) * 255.0f + 0.5f) <<  8)
         | ((VanU32)(Clamp(c.x) * 255.0f + 0.5f)      );
}

// Pack individual byte components (0-255) into VanU32 ABGR
static inline VanU32 RGBA(VanU32 r, VanU32 g, VanU32 b, VanU32 a)
{
    return (a << 24) | (b << 16) | (g << 8) | r;
}

//=============================================================================
// Internal structures
//=============================================================================

// Per-node state; one slot per unique node id encountered this frame
struct NgNodeState
{
    int     Id;
    VanVec2 Pos;                     // graph-space top-left (updated by drag)
    VanVec2 Size;                    // graph-space size (written in EndNode)
    VanVec2 PinPos[2][NG_MAX_PINS];  // screen-space pin positions [side][pin_id]
    int     PinCount[2];             // pins submitted this frame per side
    bool    Active;                  // slot in use this frame
    VanVec2* PosPtr;                 // pointer caller passed to BeginNode (for write-back)
};

// A recorded NodeLink() call; rendered in bulk during EndNodeGraph
struct NgLinkRecord
{
    int     FromNode, FromPin;
    int     ToNode,   ToPin;
    VanVec4 Color;
};

//=============================================================================
// VanNodeGraphContext definition
//=============================================================================

struct VanGui::VanNodeGraphContext
{
    // ---- Persistent across frames ----
    VanVec2 Scrolling;        // canvas pan offset (graph-space units)
    float   Zoom;             // zoom multiplier

    // ---- Per-frame node table ----
    NgNodeState Nodes[NG_MAX_NODES];
    int         NodeCount;

    // ---- Per-frame link table (filled by NodeLink()) ----
    NgLinkRecord Links[NG_MAX_LINKS];
    int          LinkCount;

    // ---- Interaction ----
    int  HoveredNode;         // -1 = none
    int  SelectedNode;        // -1 = none

    // Link dragging
    bool DraggingLink;
    int  DraggingFromNode;    // source node id
    int  DraggingFromPin;     // source pin id
    bool DraggingFromOutput;  // true = started on an output pin

    // ---- Output events (valid after EndNodeGraph, until next BeginNodeGraph) ----
    bool LinkCreated;
    int  LC_FromNode, LC_FromPin, LC_ToNode, LC_ToPin;

    bool LinkDeleted;
    int  LD_FromNode, LD_FromPin, LD_ToNode, LD_ToPin;

    // ---- BeginNode / EndNode working state ----
    int     CurNodeIdx;        // index into Nodes[] while a node is open (-1 otherwise)
    float   CurMaxLabelW;      // widest label seen so far in this node (for sizing)

    // ---- Canvas rendering state (set in BeginNodeGraph) ----
    VanVec2      CanvasOrigin;
    VanVec2      CanvasSize;
    VanDrawList* DrawList;

    VanNodeGraphContext()
    {
        memset(this, 0, sizeof(*this));
        Zoom              = 1.0f;
        HoveredNode       = -1;
        SelectedNode      = -1;
        DraggingFromNode  = -1;
        DraggingFromPin   = -1;
        CurNodeIdx        = -1;
    }
};

//=============================================================================
// Module-level globals
//=============================================================================

// The currently open graph (set by BeginNodeGraph, cleared by EndNodeGraph)
static VanGui::VanNodeGraphContext* s_CurrentCtx = nullptr;

// Holds the most-recently-ended context so the free query functions work
// without requiring the caller to pass a context pointer.
static VanGui::VanNodeGraphContext* s_LastCtx = nullptr;

//=============================================================================
// Coordinate helpers
//=============================================================================

static VanVec2 GraphToScreen(const VanGui::VanNodeGraphContext* ctx, VanVec2 gp)
{
    // screen = canvas_origin + (graph_pos + scrolling) * zoom
    return V2Add(ctx->CanvasOrigin, V2Scale(V2Add(gp, ctx->Scrolling), ctx->Zoom));
}

static VanVec2 ScreenToGraph(const VanGui::VanNodeGraphContext* ctx, VanVec2 sp)
{
    return V2Sub(V2Scale(V2Sub(sp, ctx->CanvasOrigin), 1.0f / ctx->Zoom), ctx->Scrolling);
}

//=============================================================================
// Node slot management
//=============================================================================

static NgNodeState* FindOrAddNode(VanGui::VanNodeGraphContext* ctx, int id)
{
    for (int i = 0; i < NG_MAX_NODES; i++)
        if (ctx->Nodes[i].Active && ctx->Nodes[i].Id == id)
            return &ctx->Nodes[i];
    // Claim a free slot
    for (int i = 0; i < NG_MAX_NODES; i++)
    {
        if (!ctx->Nodes[i].Active)
        {
            memset(&ctx->Nodes[i], 0, sizeof(NgNodeState));
            ctx->Nodes[i].Active = true;
            ctx->Nodes[i].Id     = id;
            ctx->NodeCount++;
            return &ctx->Nodes[i];
        }
    }
    VAN_ASSERT(false && "VanNodeGraph: exceeded NG_MAX_NODES (128)");
    return &ctx->Nodes[0]; // unreachable in correct usage
}

static NgNodeState* FindNode(VanGui::VanNodeGraphContext* ctx, int id)
{
    for (int i = 0; i < NG_MAX_NODES; i++)
        if (ctx->Nodes[i].Active && ctx->Nodes[i].Id == id)
            return &ctx->Nodes[i];
    return nullptr;
}

//=============================================================================
// Implementation
//=============================================================================

namespace VanGui
{

//-----------------------------------------------------------------------------
VanNodeGraphContext* CreateNodeGraphContext()
{
    return VAN_NEW(VanNodeGraphContext);
}

//-----------------------------------------------------------------------------
void DestroyNodeGraphContext(VanNodeGraphContext* ctx)
{
    VAN_ASSERT(ctx != nullptr);
    if (s_CurrentCtx == ctx) s_CurrentCtx = nullptr;
    if (s_LastCtx    == ctx) s_LastCtx    = nullptr;
    VAN_DELETE(ctx);
}

//-----------------------------------------------------------------------------
bool BeginNodeGraph(const char* str_id, VanNodeGraphContext* ctx,
                    const VanVec2& size)
{
    VAN_ASSERT(ctx != nullptr);
    VAN_ASSERT(s_CurrentCtx == nullptr &&
               "BeginNodeGraph called without a matching EndNodeGraph");

    // ---- Reset per-frame tables (keep persistent state) ----
    ctx->NodeCount   = 0;
    ctx->LinkCount   = 0;
    ctx->LinkCreated = false;
    ctx->LinkDeleted = false;
    ctx->CurNodeIdx  = -1;
    ctx->HoveredNode = -1;
    // Mark all node slots as inactive so stale ones are reclaimed
    for (int i = 0; i < NG_MAX_NODES; i++)
        ctx->Nodes[i].Active = false;

    // ---- Determine canvas size ----
    VanVec2 canvas_size = size;
    {
        VanVec2 avail = VanGui::GetContentRegionAvail();
        if (canvas_size.x <= 0.0f) canvas_size.x = avail.x;
        if (canvas_size.y <= 0.0f) canvas_size.y = avail.y;
        if (canvas_size.x < 1.0f)  canvas_size.x = 1.0f;
        if (canvas_size.y < 1.0f)  canvas_size.y = 1.0f;
    }

    // ---- Open child window ----
    VanGuiWindowFlags wflags = VanGuiWindowFlags_NoScrollbar
                             | VanGuiWindowFlags_NoMove
                             | VanGuiWindowFlags_NoScrollWithMouse;
    bool visible = VanGui::BeginChild(str_id, canvas_size,
                                      VanGuiChildFlags_None, wflags);
    if (!visible)
    {
        VanGui::EndChild();
        return false;
    }

    s_CurrentCtx         = ctx;
    ctx->CanvasOrigin    = VanGui::GetCursorScreenPos();
    ctx->CanvasSize      = canvas_size;
    ctx->DrawList        = VanGui::GetWindowDrawList();

    // ---- Mouse wheel zoom (keeps graph-point under cursor fixed) ----
    if (VanGui::IsWindowHovered(VanGuiHoveredFlags_ChildWindows |
                                VanGuiHoveredFlags_AllowWhenBlockedByActiveItem))
    {
        float wheel = VanGui::GetIO().MouseWheel;
        if (wheel != 0.0f)
        {
            VanVec2 m_screen = VanGui::GetMousePos();
            VanVec2 m_graph  = ScreenToGraph(ctx, m_screen);

            float nz = ctx->Zoom * (1.0f + wheel * NG_ZOOM_SPEED);
            if (nz < NG_ZOOM_MIN) nz = NG_ZOOM_MIN;
            if (nz > NG_ZOOM_MAX) nz = NG_ZOOM_MAX;
            ctx->Zoom = nz;

            // Recalculate scrolling so the same graph point stays under cursor:
            // screen = origin + (graph + scroll) * zoom
            // => scroll = (screen - origin) / zoom - graph
            VanVec2 local   = V2Sub(m_screen, ctx->CanvasOrigin);
            ctx->Scrolling  = V2Sub(V2Scale(local, 1.0f / ctx->Zoom), m_graph);
        }
    }

    // ---- Draw dot-grid background ----
    {
        VanDrawList* dl = ctx->DrawList;
        VanU32 dot_col  = RGBA(80, 80, 80, 200);

        dl->PushClipRect(ctx->CanvasOrigin,
                         V2Add(ctx->CanvasOrigin, ctx->CanvasSize), true);

        float step = NG_GRID_STEP * ctx->Zoom;

        // Find the first dot position at or just before the canvas origin
        float ox = ctx->CanvasOrigin.x + fmodf(ctx->Scrolling.x * ctx->Zoom, step);
        float oy = ctx->CanvasOrigin.y + fmodf(ctx->Scrolling.y * ctx->Zoom, step);
        if (ox > ctx->CanvasOrigin.x) ox -= step;
        if (oy > ctx->CanvasOrigin.y) oy -= step;

        for (float x = ox; x < ctx->CanvasOrigin.x + ctx->CanvasSize.x + step; x += step)
            for (float y = oy; y < ctx->CanvasOrigin.y + ctx->CanvasSize.y + step; y += step)
                dl->AddCircleFilled(VanVec2(x, y), NG_GRID_DOT_R, dot_col);

        dl->PopClipRect();
    }

    // ---- Background hit area: panning + deselection ----
    // Placed behind nodes. Nodes use SetNextItemAllowOverlap to sit on top.
    {
        VanGui::SetCursorScreenPos(ctx->CanvasOrigin);
        (void)VanGui::InvisibleButton("##canvas_bg", ctx->CanvasSize);
        bool bg_hovered = VanGui::IsItemHovered();
        bool bg_active  = VanGui::IsItemActive();

        // Pan via middle mouse or Alt+LMB
        bool panning = (bg_active && VanGui::IsMouseDragging(2))   // middle
                    || (bg_active && VanGui::IsMouseDragging(0)
                        && VanGui::GetIO().KeyAlt);                  // Alt+LMB
        if (panning)
        {
            VanVec2 delta = VanGui::GetIO().MouseDelta;
            ctx->Scrolling.x += delta.x / ctx->Zoom;
            ctx->Scrolling.y += delta.y / ctx->Zoom;
        }

        // Click on empty background: deselect + cancel link drag
        if (bg_hovered && VanGui::IsMouseClicked(0) && !VanGui::GetIO().KeyAlt)
        {
            ctx->SelectedNode    = -1;
            ctx->DraggingLink    = false;
            ctx->DraggingFromNode = -1;
            ctx->DraggingFromPin  = -1;
        }
    }

    VanGui::SetCursorScreenPos(ctx->CanvasOrigin);
    return true;
}

//-----------------------------------------------------------------------------
void EndNodeGraph()
{
    VAN_ASSERT(s_CurrentCtx != nullptr &&
               "EndNodeGraph called without BeginNodeGraph");
    VanNodeGraphContext* ctx = s_CurrentCtx;
    VAN_ASSERT(ctx->CurNodeIdx == -1 &&
               "EndNodeGraph called while a BeginNode is still open");

    VanDrawList* dl       = ctx->DrawList;
    VanVec2      mouse_pos = VanGui::GetMousePos();
    bool         del_key  = VanGui::IsKeyPressed(VanGuiKey_Delete, false);

    dl->PushClipRect(ctx->CanvasOrigin,
                     V2Add(ctx->CanvasOrigin, ctx->CanvasSize), true);

    // ---- Draw recorded links ----
    for (int li = 0; li < ctx->LinkCount; li++)
    {
        const NgLinkRecord& lr = ctx->Links[li];

        NgNodeState* fn = FindNode(ctx, lr.FromNode);
        NgNodeState* tn = FindNode(ctx, lr.ToNode);
        if (!fn || !tn) continue;
        if (lr.FromPin < 0 || lr.FromPin >= NG_MAX_PINS) continue;
        if (lr.ToPin   < 0 || lr.ToPin   >= NG_MAX_PINS) continue;

        // Output pin -> Input pin bezier
        VanVec2 p1 = fn->PinPos[1][lr.FromPin]; // from-node output side
        VanVec2 p4 = tn->PinPos[0][lr.ToPin];   // to-node   input  side

        float dx     = p4.x - p1.x;
        float offset = fabsf(dx) * 0.5f;
        if (offset < 50.0f) offset = 50.0f;
        VanVec2 p2 = VanVec2(p1.x + offset, p1.y);
        VanVec2 p3 = VanVec2(p4.x - offset, p4.y);

        VanU32 col = Col4ToU32(lr.Color);

        // Compute distance from mouse to this bezier for hover / delete
        float dist_sq = BezierPointDistSq(mouse_pos, p1, p2, p3, p4);
        float hit_sq  = NG_LINK_HIT_DIST * NG_LINK_HIT_DIST;

        if (del_key && dist_sq < hit_sq)
        {
            // Record deletion event (first hit wins)
            if (!ctx->LinkDeleted)
            {
                ctx->LinkDeleted  = true;
                ctx->LD_FromNode  = lr.FromNode;
                ctx->LD_FromPin   = lr.FromPin;
                ctx->LD_ToNode    = lr.ToNode;
                ctx->LD_ToPin     = lr.ToPin;
            }
            col = RGBA(255, 60, 60, 220); // red flash before caller removes it
        }
        else if (!del_key && dist_sq < hit_sq)
        {
            // Hover highlight: brighten colour
            VanVec4 hc = lr.Color;
            auto Bright = [](float v) -> float {
                v *= 1.4f; return v > 1.0f ? 1.0f : v;
            };
            col = Col4ToU32(VanVec4(Bright(hc.x), Bright(hc.y),
                                    Bright(hc.z), hc.w));
        }

        dl->AddBezierCubic(p1, p2, p3, p4, col, NG_LINK_THICKNESS);
    }

    // ---- Draw in-progress link drag ----
    if (ctx->DraggingLink && ctx->DraggingFromNode >= 0)
    {
        NgNodeState* src = FindNode(ctx, ctx->DraggingFromNode);
        if (src)
        {
            int    side = ctx->DraggingFromOutput ? 1 : 0;
            VanVec2 p1  = src->PinPos[side][ctx->DraggingFromPin];
            VanVec2 p4  = mouse_pos;

            float offset = fabsf(p4.x - p1.x) * 0.5f;
            if (offset < 50.0f) offset = 50.0f;
            VanVec2 p2, p3;
            if (ctx->DraggingFromOutput)
            {
                p2 = VanVec2(p1.x + offset, p1.y);
                p3 = VanVec2(p4.x - offset, p4.y);
            }
            else
            {
                // Dragging from an input: curve goes left-to-right naturally
                p2 = VanVec2(p1.x - offset, p1.y);
                p3 = VanVec2(p4.x + offset, p4.y);
            }
            dl->AddBezierCubic(p1, p2, p3, p4,
                               RGBA(200, 200, 200, 200), NG_LINK_THICKNESS);
        }

        // Cancel drag on mouse release without landing on a compatible pin
        if (!VanGui::IsMouseDown(0))
        {
            ctx->DraggingLink     = false;
            ctx->DraggingFromNode = -1;
            ctx->DraggingFromPin  = -1;
        }
    }

    dl->PopClipRect();

    VanGui::EndChild();

    s_LastCtx    = ctx;
    s_CurrentCtx = nullptr;
}

//-----------------------------------------------------------------------------
void BeginNode(int id, VanVec2* inout_pos)
{
    VAN_ASSERT(s_CurrentCtx != nullptr &&
               "BeginNode called outside BeginNodeGraph/EndNodeGraph");
    VAN_ASSERT(inout_pos != nullptr);
    VanNodeGraphContext* ctx = s_CurrentCtx;
    VAN_ASSERT(ctx->CurNodeIdx == -1 &&
               "BeginNode called while another node is still open");

    NgNodeState* ns   = FindOrAddNode(ctx, id);
    ns->Pos           = *inout_pos;        // initialise from caller
    ns->PosPtr        = inout_pos;         // store for write-back in EndNode
    ns->PinCount[0]   = 0;
    ns->PinCount[1]   = 0;
    ctx->CurNodeIdx   = (int)(ns - ctx->Nodes);
    ctx->CurMaxLabelW = 80.0f;             // minimum content width (graph units)
}

//-----------------------------------------------------------------------------
void EndNode()
{
    VAN_ASSERT(s_CurrentCtx != nullptr);
    VanNodeGraphContext* ctx = s_CurrentCtx;
    VAN_ASSERT(ctx->CurNodeIdx >= 0 && "EndNode called without BeginNode");

    NgNodeState* ns  = &ctx->Nodes[ctx->CurNodeIdx];
    VanDrawList* dl  = ctx->DrawList;
    float        zoom = ctx->Zoom;

    // ---- Compute node screen dimensions ----
    // Node width: widest label + padding + pin circles on both sides
    // Width is expressed in screen pixels.
    float pin_circle_w = (NG_PIN_RADIUS * 2.0f + 8.0f) * zoom; // per side
    float content_w    = ctx->CurMaxLabelW + pin_circle_w * 2.0f
                       + NG_NODE_PAD_X * 2.0f;

    int total_pins = ns->PinCount[0] > ns->PinCount[1]
                   ? ns->PinCount[0] : ns->PinCount[1];
    float line_h_screen = (VanGui::GetTextLineHeight() + NG_NODE_PAD_Y);
    float content_h     = (float)total_pins * line_h_screen + NG_NODE_PAD_Y;

    float title_h_screen = NG_TITLE_H * zoom;
    float node_w = content_w;
    float node_h = title_h_screen + content_h;

    // Store graph-space size (used if caller needs it; not strictly required)
    ns->Size = VanVec2(node_w / zoom, node_h / zoom);

    VanVec2 tl = GraphToScreen(ctx, ns->Pos);
    VanVec2 br = V2Add(tl, VanVec2(node_w, node_h));

    // ---- Fix-up output pin X positions now that we know the node width ----
    // NodePin() used an estimated width when placing output pins. Correct it.
    for (int i = 0; i < ns->PinCount[1]; i++)
        ns->PinPos[1][i].x = br.x;   // right edge

    // ---- Shadow ----
    dl->AddRectFilled(V2Add(tl, VanVec2(NG_SHADOW_OFFSET, NG_SHADOW_OFFSET)),
                      V2Add(br, VanVec2(NG_SHADOW_OFFSET, NG_SHADOW_OFFSET)),
                      RGBA(0, 0, 0, 60), NG_NODE_ROUNDING);

    // ---- Selection outline (drawn behind the node body so it shows as a glow) ----
    if (ctx->SelectedNode == ns->Id)
    {
        dl->AddRectFilled(V2Sub(tl, VanVec2(3.0f, 3.0f)),
                          V2Add(br, VanVec2(3.0f, 3.0f)),
                          RGBA(255, 180, 0, 200), NG_NODE_ROUNDING + 3.0f);
    }

    // ---- Node body ----
    dl->AddRectFilled(tl, br, RGBA(40, 40, 40, 230), NG_NODE_ROUNDING);

    // ---- Title bar ----
    VanVec2 title_br = VanVec2(br.x, tl.y + title_h_screen);
    // Top-rounded rect for the title
    dl->AddRectFilled(tl, title_br, RGBA(60, 60, 80, 230), NG_NODE_ROUNDING);
    // Fill the bottom portion of the title bar flat (no rounding) so it
    // blends flush into the node body below.
    if (title_h_screen > NG_NODE_ROUNDING)
    {
        dl->AddRectFilled(
            VanVec2(tl.x,  tl.y + NG_NODE_ROUNDING),
            title_br,
            RGBA(60, 60, 80, 230), 0.0f);
    }

    // ---- Border ----
    {
        bool hovered = (ctx->HoveredNode == ns->Id);
        VanU32 border_col = hovered ? RGBA(140, 140, 160, 220)
                                    : RGBA(80,  80,  100, 160);
        float r = NG_NODE_ROUNDING;
        // Four edges (skipping corners for simplicity; the filled rects provide visual rounding)
        dl->AddLine(VanVec2(tl.x + r, tl.y), VanVec2(br.x - r, tl.y), border_col);
        dl->AddLine(VanVec2(br.x, tl.y + r), VanVec2(br.x, br.y - r), border_col);
        dl->AddLine(VanVec2(br.x - r, br.y), VanVec2(tl.x + r, br.y), border_col);
        dl->AddLine(VanVec2(tl.x, br.y - r), VanVec2(tl.x, tl.y + r), border_col);
    }

    // ---- Title bar drag button (select + drag node) ----
    VanGui::SetCursorScreenPos(tl);
    VanGui::SetNextItemAllowOverlap(); // pins inside the node will overlap
    char drag_id[48];
    snprintf(drag_id, sizeof(drag_id), "##nd_%d", ns->Id);
    (void)VanGui::InvisibleButton(drag_id, VanVec2(node_w, title_h_screen));

    bool title_hov = VanGui::IsItemHovered();
    bool title_act = VanGui::IsItemActive();

    if (title_hov)
        ctx->HoveredNode = ns->Id;

    if (title_hov && VanGui::IsMouseClicked(0))
        ctx->SelectedNode = ns->Id;

    // Drag to move (only when not dragging a link)
    if (title_act && VanGui::IsMouseDragging(0) && !ctx->DraggingLink)
    {
        VanVec2 delta = VanGui::GetIO().MouseDelta;
        ns->Pos.x += delta.x / zoom;
        ns->Pos.y += delta.y / zoom;
    }

    // Write updated position back to caller's variable
    if (ns->PosPtr)
        *ns->PosPtr = ns->Pos;

    ctx->CurNodeIdx = -1;
}

//-----------------------------------------------------------------------------
void NodeTitle(const char* title)
{
    VAN_ASSERT(s_CurrentCtx != nullptr);
    VanNodeGraphContext* ctx = s_CurrentCtx;
    VAN_ASSERT(ctx->CurNodeIdx >= 0 && "NodeTitle called outside BeginNode/EndNode");

    NgNodeState* ns = &ctx->Nodes[ctx->CurNodeIdx];

    float title_h_screen = NG_TITLE_H * ctx->Zoom;
    float text_h         = VanGui::GetTextLineHeight();
    VanVec2 screen_tl    = GraphToScreen(ctx, ns->Pos);
    VanVec2 text_pos     = VanVec2(screen_tl.x + NG_NODE_PAD_X,
                                   screen_tl.y + (title_h_screen - text_h) * 0.5f);

    ctx->DrawList->AddText(text_pos, RGBA(230, 230, 230, 255), title);

    // Update max label width so the node auto-sizes to fit the title
    float tw = VanGui::CalcTextSize(title).x;
    if (tw > ctx->CurMaxLabelW) ctx->CurMaxLabelW = tw;
}

//-----------------------------------------------------------------------------
bool NodePin(int pin_id, bool is_output, const char* label, VanVec4 color)
{
    VAN_ASSERT(s_CurrentCtx != nullptr);
    VanNodeGraphContext* ctx = s_CurrentCtx;
    VAN_ASSERT(ctx->CurNodeIdx >= 0 && "NodePin called outside BeginNode/EndNode");

    NgNodeState* ns   = &ctx->Nodes[ctx->CurNodeIdx];
    VanDrawList* dl   = ctx->DrawList;
    float        zoom = ctx->Zoom;
    int          side = is_output ? 1 : 0;
    int          idx  = ns->PinCount[side];

    VAN_ASSERT(idx < NG_MAX_PINS && "Exceeded NG_MAX_PINS (16) on one side of a node");

    // ---- Compute pin screen position ----
    VanVec2 node_tl      = GraphToScreen(ctx, ns->Pos);
    float   title_h_s    = NG_TITLE_H * zoom;
    float   line_h_s     = VanGui::GetTextLineHeight() + NG_NODE_PAD_Y;
    float   pin_y        = node_tl.y + title_h_s + NG_NODE_PAD_Y
                         + (float)idx * line_h_s + line_h_s * 0.5f;

    // For outputs: we use a temporary X that will be corrected in EndNode once
    // we know the full node width. For inputs: X is the left edge.
    // We store both in PinPos; EndNode patches output X.
    float pin_x_input  = node_tl.x;
    // Rough output X estimate (will be overwritten in EndNode)
    float pin_area_w   = (NG_PIN_RADIUS * 2.0f + 8.0f) * zoom;
    float est_content_w = ctx->CurMaxLabelW + pin_area_w * 2.0f
                        + NG_NODE_PAD_X * 2.0f;
    float pin_x_output = node_tl.x + est_content_w;

    float   pin_x        = is_output ? pin_x_output : pin_x_input;
    VanVec2 pin_screen   = VanVec2(pin_x, pin_y);

    // Store for link rendering (output X will be patched later)
    ns->PinPos[side][idx] = pin_screen;
    ns->PinCount[side]++;

    // ---- Draw pin circle ----
    bool is_src = ctx->DraggingLink &&
                  ctx->DraggingFromNode   == ns->Id &&
                  ctx->DraggingFromPin    == pin_id  &&
                  ctx->DraggingFromOutput == is_output;

    VanU32 pin_col = is_src ? RGBA(255, 255, 100, 255) : Col4ToU32(color);
    dl->AddCircleFilled(pin_screen, NG_PIN_RADIUS * zoom, pin_col);

    // ---- Draw label ----
    float text_h   = VanGui::GetTextLineHeight();
    float label_y  = pin_y - text_h * 0.5f;
    float label_x;
    if (!is_output)
        label_x = pin_x + NG_PIN_RADIUS * zoom + 4.0f;
    else
    {
        float lw = VanGui::CalcTextSize(label).x;
        label_x  = pin_x - NG_PIN_RADIUS * zoom - 4.0f - lw;
    }
    dl->AddText(VanVec2(label_x, label_y), RGBA(210, 210, 210, 255), label);

    float lw = VanGui::CalcTextSize(label).x;
    if (lw > ctx->CurMaxLabelW) ctx->CurMaxLabelW = lw;

    // ---- Invisible button for interaction ----
    float btn_half = NG_PIN_BTN_HALF * zoom;
    VanGui::SetCursorScreenPos(VanVec2(pin_screen.x - btn_half,
                                       pin_screen.y - btn_half));
    VanGui::SetNextItemAllowOverlap();
    char pin_id_str[64];
    snprintf(pin_id_str, sizeof(pin_id_str), "##pin_%d_%d_%d", ns->Id, pin_id, side);
    (void)VanGui::InvisibleButton(pin_id_str, VanVec2(btn_half * 2.0f, btn_half * 2.0f));

    // AllowWhenBlockedByActiveItem so we can detect release over a pin while
    // another item (the source pin's button) is active.
    bool pin_hov     = VanGui::IsItemHovered(VanGuiHoveredFlags_AllowWhenBlockedByActiveItem);
    bool pin_clicked = VanGui::IsItemClicked(0);

    bool result = false;

    if (pin_clicked && !ctx->DraggingLink)
    {
        // Start a new link drag
        ctx->DraggingLink       = true;
        ctx->DraggingFromNode   = ns->Id;
        ctx->DraggingFromPin    = pin_id;
        ctx->DraggingFromOutput = is_output;
    }
    else if (ctx->DraggingLink && pin_hov && !VanGui::IsMouseDown(0))
    {
        // Mouse released over this pin -- attempt to complete the link
        bool src_is_out = ctx->DraggingFromOutput;

        // Valid: one side is output, one is input; different nodes
        bool compatible = (src_is_out != is_output)
                       && (ctx->DraggingFromNode != ns->Id);

        if (compatible)
        {
            if (src_is_out)
            {
                ctx->LC_FromNode = ctx->DraggingFromNode;
                ctx->LC_FromPin  = ctx->DraggingFromPin;
                ctx->LC_ToNode   = ns->Id;
                ctx->LC_ToPin    = pin_id;
            }
            else
            {
                // Dragged from an input; normalise to output->input
                ctx->LC_FromNode = ns->Id;
                ctx->LC_FromPin  = pin_id;
                ctx->LC_ToNode   = ctx->DraggingFromNode;
                ctx->LC_ToPin    = ctx->DraggingFromPin;
            }
            ctx->LinkCreated = true;
            result           = true;
        }

        // End drag regardless of whether connection was made
        ctx->DraggingLink     = false;
        ctx->DraggingFromNode = -1;
        ctx->DraggingFromPin  = -1;
    }

    return result;
}

//-----------------------------------------------------------------------------
void NodeLink(int from_node_id, int from_pin_id,
              int to_node_id,   int to_pin_id,
              VanVec4 color)
{
    VAN_ASSERT(s_CurrentCtx != nullptr &&
               "NodeLink called outside BeginNodeGraph/EndNodeGraph");
    VanNodeGraphContext* ctx = s_CurrentCtx;

    if (ctx->LinkCount >= NG_MAX_LINKS)
    {
        VAN_ASSERT(false && "VanNodeGraph: exceeded NG_MAX_LINKS (512)");
        return;
    }

    NgLinkRecord& lr = ctx->Links[ctx->LinkCount++];
    lr.FromNode = from_node_id;
    lr.FromPin  = from_pin_id;
    lr.ToNode   = to_node_id;
    lr.ToPin    = to_pin_id;
    lr.Color    = color;
}

//-----------------------------------------------------------------------------
// Query functions (operate on s_LastCtx, the most-recently-ended graph)
//-----------------------------------------------------------------------------

bool IsLinkCreated(int* from_node_id, int* from_pin_id,
                   int* to_node_id,   int* to_pin_id)
{
    if (!s_LastCtx || !s_LastCtx->LinkCreated)
        return false;

    if (from_node_id) *from_node_id = s_LastCtx->LC_FromNode;
    if (from_pin_id)  *from_pin_id  = s_LastCtx->LC_FromPin;
    if (to_node_id)   *to_node_id   = s_LastCtx->LC_ToNode;
    if (to_pin_id)    *to_pin_id    = s_LastCtx->LC_ToPin;
    return true;
}

bool IsLinkDeleted(int* from_node_id, int* from_pin_id,
                   int* to_node_id,   int* to_pin_id)
{
    if (!s_LastCtx || !s_LastCtx->LinkDeleted)
        return false;

    if (from_node_id) *from_node_id = s_LastCtx->LD_FromNode;
    if (from_pin_id)  *from_pin_id  = s_LastCtx->LD_FromPin;
    if (to_node_id)   *to_node_id   = s_LastCtx->LD_ToNode;
    if (to_pin_id)    *to_pin_id    = s_LastCtx->LD_ToPin;
    return true;
}

bool IsNodeSelected(int node_id)
{
    return s_LastCtx && (s_LastCtx->SelectedNode == node_id);
}

} // namespace VanGui
