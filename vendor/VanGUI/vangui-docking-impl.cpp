// dear vangui, v1.92.9 WIP
// (docking and multi-viewport system)
// True C++23 module interface partition — vangui:docking
// Merged from vangui-docking.ixx + vangui_docking.cpp

// Regular C++ TU — symbols get standard external linkage (no ::<!vangui> mangling).
// The thin interface vangui-docking.ixx exports these names; the linker resolves them here.

#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#ifndef VANGUI_DEFINE_MATH_OPERATORS
#define VANGUI_DEFINE_MATH_OPERATORS
#endif

#include "vangui.h"
#ifndef VANGUI_DISABLE
#include "vangui_internal.h"
#endif

//=============================================================================
// IMPLEMENTATION — function bodies (formerly vangui_docking.cpp)
#ifndef VANGUI_DISABLE
//=============================================================================

namespace VanGui {

// Forward declarations (internal helpers referenced before their definition)
static void DockNodeUpdate(VanGuiDockNode* node);
static void DockNodeUpdateTabBar(VanGuiDockNode* node, VanGuiWindow* host_window);
static void DockNodeUpdateVisibleFlag(VanGuiDockNode* node);
static void DockNodeCalcDropRectsAndTestMousePos(const VanRect& parent, VanGuiDir dir, VanRect& out_r, bool outer, VanVec2* test_mouse_pos);

//-----------------------------------------------------------------------------
// [SECTION] Docking: Internal helpers
//-----------------------------------------------------------------------------

VanGuiDockNode* DockContextFindNodeByID(VanGuiContext* ctx, VanGuiID id)
{
    return static_cast<VanGuiDockNode*>(ctx->DockContext.Nodes.GetVoidPtr(id));
}

static VanGuiDockNode* DockContextAddNode(VanGuiContext* ctx, VanGuiID id)
{
    VanGuiContext& g = *ctx;
    if (id == 0)
    {
        do
        {
            id = VanHashStr("##DockNode", 10, ++g.DockContext.NodeIdNext);
        }
        while (DockContextFindNodeByID(ctx, id) != nullptr);
    }
    VanGuiDockNode* node = VAN_NEW(VanGuiDockNode)(id);
    ctx->DockContext.Nodes.SetVoidPtr(id, node);
    return node;
}

static void DockContextRemoveNode(VanGuiContext* ctx, VanGuiDockNode* node, bool merge_sibling_into_parent_node)
{
    if (VanGuiDockNode* parent = node->ParentNode)
    {
        VanGuiDockNode* sibling = (parent->ChildNodes[0] == node) ? parent->ChildNodes[1] : parent->ChildNodes[0];
        if (merge_sibling_into_parent_node && sibling != nullptr)
        {
            // Absorb sibling's last-focused identity into parent before the node is removed
            parent->ChildNodes[0] = parent->ChildNodes[1] = nullptr;
            parent->LastFocusedNodeId = sibling->LastFocusedNodeId;
        }
        else
        {
            if (parent->ChildNodes[0] == node) parent->ChildNodes[0] = nullptr;
            else if (parent->ChildNodes[1] == node) parent->ChildNodes[1] = nullptr;
        }
    }
    ctx->DockContext.Nodes.SetVoidPtr(node->ID, nullptr);
    VAN_DELETE(node);
}

//-----------------------------------------------------------------------------
// [SECTION] Docking: Node splitting
//-----------------------------------------------------------------------------

[[nodiscard]] static VanGuiDockNode* DockNodeGetRootNode(VanGuiDockNode* node) noexcept
{
    while (node->ParentNode != nullptr)
        node = node->ParentNode;
    return node;
}

[[nodiscard]] static bool DockNodeIsDropAllowed(VanGuiWindow* host, VanGuiWindow* payload) noexcept
{
    if (host == payload)
        return false;
    return true;
}

// Calculate the drop rectangles for docking onto a window or node.
// dir == VanGuiDir_None produces a centered "tab" target; directional values produce edge strips.
static void DockNodeCalcDropRectsAndTestMousePos(
    const VanRect& r,
    VanGuiDir      dir,
    VanRect&       out_rect,
    bool           outer_docking,
    VanVec2*       test_mouse_pos)
{
    const float desired_ratio        = (dir == VanGuiDir_None) ? 0.90f : 0.25f;
    const float size_for_direction_min = 30.0f;
    const float size_for_direction_max = VanFloor(VanMin(r.GetWidth(), r.GetHeight()) * desired_ratio);
    const float size_for_direction     = VanMax(size_for_direction_min, size_for_direction_max);

    switch (dir)
    {
    case VanGuiDir_None:
        out_rect = VanRect(
            r.GetCenter() - VanVec2(size_for_direction_max, size_for_direction_max) * 0.5f,
            r.GetCenter() + VanVec2(size_for_direction_max, size_for_direction_max) * 0.5f);
        break;
    case VanGuiDir_Up:
        out_rect = VanRect(
            r.Min.x,
            r.Min.y - (outer_docking ? size_for_direction : 0.0f),
            r.Max.x,
            r.Min.y + (outer_docking ? 0.0f : size_for_direction));
        break;
    case VanGuiDir_Down:
        out_rect = VanRect(
            r.Min.x,
            r.Max.y - (outer_docking ? 0.0f : size_for_direction),
            r.Max.x,
            r.Max.y + (outer_docking ? size_for_direction : 0.0f));
        break;
    case VanGuiDir_Left:
        out_rect = VanRect(
            r.Min.x - (outer_docking ? size_for_direction : 0.0f),
            r.Min.y,
            r.Min.x + (outer_docking ? 0.0f : size_for_direction),
            r.Max.y);
        break;
    case VanGuiDir_Right:
        out_rect = VanRect(
            r.Max.x - (outer_docking ? 0.0f : size_for_direction),
            r.Min.y,
            r.Max.x + (outer_docking ? size_for_direction : 0.0f),
            r.Max.y);
        break;
    default:
        break;
    }

    if (test_mouse_pos != nullptr)
        *test_mouse_pos = out_rect.GetCenter();
}

//-----------------------------------------------------------------------------
// [SECTION] Docking: Drop-zone preview overlay
//-----------------------------------------------------------------------------

// Draw semi-transparent rectangle previewing where the dragged window will land,
// plus directional arrow overlays for each valid drop zone on the target node/window.
// Called once per frame from Render() when the user is dragging a window.
static void DockNodePreviewDockRender(VanGuiWindow* host_window, VanGuiDockNode* host_node, VanGuiWindow* payload_window)
{
    VanGuiContext& g = *GVanGui;
    VAN_ASSERT(g.CurrentWindow == nullptr); // Must be called outside of any Begin/End pair

    if (payload_window == nullptr || host_window == nullptr)
        return;

    // Don't render if docking is not enabled or no window is moving
    if (!(g.IO.ConfigFlags & VanGuiConfigFlags_DockingEnable))
        return;

    VanDrawList* draw_list = GetForegroundDrawList(host_window->Viewport);

    // Collect candidate drop directions: center tab + 4 cardinal edges
    static const VanGuiDir dirs[] = { VanGuiDir_None, VanGuiDir_Left, VanGuiDir_Right, VanGuiDir_Up, VanGuiDir_Down };
    const VanRect node_rect = (host_node != nullptr)
        ? VanRect(host_node->Pos, host_node->Pos + host_node->Size)
        : VanRect(host_window->Pos, host_window->Pos + host_window->SizeFull);

    const VanVec2 mouse_pos = g.IO.MousePos;

    // Colors
    const VanU32 col_preview  = GetColorU32(VanGuiCol_DockingPreview, 0.70f);
    const VanU32 col_empty    = GetColorU32(VanGuiCol_DockingEmptyBg, 0.70f);
    const VanU32 col_outline  = GetColorU32(VanGuiCol_SeparatorHovered);
    const float  rounding     = g.Style.WindowRounding;

    VanGuiDir hovered_dir = VanGuiDir_None;
    bool      center_hovered = false;

    // First pass: find which direction the mouse is hovering
    for (VanGuiDir dir : dirs)
    {
        VanRect drop_rect;
        DockNodeCalcDropRectsAndTestMousePos(node_rect, dir, drop_rect, /*outer=*/false, nullptr);
        if (drop_rect.Contains(mouse_pos))
        {
            if (dir == VanGuiDir_None)
                center_hovered = true;
            else
                hovered_dir = dir;
            break;
        }
    }

    // Second pass: draw each zone, highlighted if hovered
    for (VanGuiDir dir : dirs)
    {
        VanRect drop_rect;
        DockNodeCalcDropRectsAndTestMousePos(node_rect, dir, drop_rect, /*outer=*/false, nullptr);

        const bool is_hovered = (dir == VanGuiDir_None) ? center_hovered : (dir == hovered_dir);

        // Semi-transparent fill
        draw_list->AddRectFilled(drop_rect.Min, drop_rect.Max, is_hovered ? col_preview : col_empty, rounding);
        draw_list->AddRect      (drop_rect.Min, drop_rect.Max, col_outline, rounding, 0, 2.0f);

        // Draw an arrow for cardinal directions
        if (dir != VanGuiDir_None)
        {
            const VanVec2 center = drop_rect.GetCenter();
            const float   arrow  = VanMin(drop_rect.GetWidth(), drop_rect.GetHeight()) * 0.4f;
            RenderArrow(draw_list, center - VanVec2(arrow * 0.5f, arrow * 0.5f), col_outline, dir, arrow);
        }
    }

    // If a direction is hovered, draw the landing-zone preview rectangle
    if (center_hovered || hovered_dir != VanGuiDir_None)
    {
        VanRect preview_rect;
        if (center_hovered)
        {
            // Tab into existing node — use the full node rect
            preview_rect = node_rect;
        }
        else
        {
            // Split — highlight the half the payload would occupy
            VanRect r0, r1;
            const bool split_x = (hovered_dir == VanGuiDir_Left || hovered_dir == VanGuiDir_Right);
            const float ratio  = 0.5f;
            if (split_x)
            {
                const float w = node_rect.GetWidth() * ratio;
                r0 = VanRect(node_rect.Min, VanVec2(node_rect.Min.x + w, node_rect.Max.y));
                r1 = VanRect(VanVec2(node_rect.Min.x + w, node_rect.Min.y), node_rect.Max);
                preview_rect = (hovered_dir == VanGuiDir_Left) ? r0 : r1;
            }
            else
            {
                const float h = node_rect.GetHeight() * ratio;
                r0 = VanRect(node_rect.Min, VanVec2(node_rect.Max.x, node_rect.Min.y + h));
                r1 = VanRect(VanVec2(node_rect.Min.x, node_rect.Min.y + h), node_rect.Max);
                preview_rect = (hovered_dir == VanGuiDir_Up) ? r0 : r1;
            }
        }
        draw_list->AddRectFilled(preview_rect.Min, preview_rect.Max, col_preview, rounding);
        draw_list->AddRect      (preview_rect.Min, preview_rect.Max, col_outline, rounding, 0, 2.0f);
    }
}

// Called from Render() for the moving window — finds a hovered dock target and renders overlay.
void DockContextRenderWindowOverlay(VanGuiWindow* payload_window)
{
    VanGuiContext& g = *GVanGui;

    if (payload_window == nullptr)
        return;

    // Walk all windows from back to front, looking for a candidate host
    for (int i = g.Windows.Size - 1; i >= 0; i--)
    {
        VanGuiWindow* host = g.Windows[i];
        if (host == payload_window)
            continue;
        if (host->Flags & VanGuiWindowFlags_NoMouseInputs)
            continue;
        if (!host->WasActive)
            continue;

        VanRect host_rect(host->Pos, host->Pos + host->SizeFull);
        if (!host_rect.Contains(g.IO.MousePos))
            continue;

        // Found a potential host
        VanGuiDockNode* host_node = host->DockNode;
        DockNodePreviewDockRender(host, host_node, payload_window);
        break;
    }
}

//-----------------------------------------------------------------------------
// [SECTION] Docking: Public API
//-----------------------------------------------------------------------------

VanGuiID DockSpace(VanGuiID dockspace_id, const VanVec2& size, VanGuiDockNodeFlags flags, const VanGuiWindowClass* window_class)
{
    VanGuiContext& g = *GVanGui;
    VanGuiWindow* window = g.CurrentWindow;

    if (!(g.IO.ConfigFlags & VanGuiConfigFlags_DockingEnable))
        return dockspace_id;

    // Retrieve or create the dock node for this dockspace_id
    VanGuiDockNode* node = DockContextFindNodeByID(&g, dockspace_id);
    if (node == nullptr)
    {
        node = DockContextAddNode(&g, dockspace_id);
        node->LocalFlags |= VanGuiDockNodeFlags_None;
    }

    // Apply window class when it changes
    if (window_class != nullptr && window_class->ClassId != node->WindowClass.ClassId)
        node->WindowClass = *window_class;

    node->SharedFlags    = flags;
    node->IsDockSpace    = true;
    node->LastFrameAlive = g.FrameCount;

    // Resolve size: negative components mean "content_avail + value" (i.e. subtract from available)
    const VanVec2 content_avail = GetContentRegionAvail();
    VanVec2 actual_size = VanVec2(
        size.x <= 0.0f ? content_avail.x + size.x : size.x,
        size.y <= 0.0f ? content_avail.y + size.y : size.y);
    actual_size = VanMax(actual_size, VanVec2(4.0f, 4.0f));

    node->Pos     = window->DC.CursorPos;
    node->Size    = actual_size;
    node->SizeRef = actual_size;

    // Advance cursor so the host window accounts for the dockspace area
    ItemSize(actual_size);

    DockNodeUpdate(node);

    return dockspace_id;
}

VanGuiID DockSpaceOverViewport(VanGuiID dockspace_id, const VanGuiViewport* viewport, VanGuiDockNodeFlags flags, const VanGuiWindowClass* window_class)
{
    if (viewport == nullptr)
        viewport = GetMainViewport();

    SetNextWindowPos(viewport->WorkPos);
    SetNextWindowSize(viewport->WorkSize);

    // Build host-window flags: fullscreen, no decorations, not user-movable/resizable
    VanGuiWindowFlags host_window_flags = 0;
    host_window_flags |= VanGuiWindowFlags_NoTitleBar
                       | VanGuiWindowFlags_NoCollapse
                       | VanGuiWindowFlags_NoResize
                       | VanGuiWindowFlags_NoMove
                       | VanGuiWindowFlags_NoBringToFrontOnFocus
                       | VanGuiWindowFlags_NoNavFocus
                       | VanGuiWindowFlags_NoDocking
                       | VanGuiWindowFlags_NoBringToDisplayOnFocus;
    if (flags & VanGuiDockNodeFlags_PassthruCentralNode)
        host_window_flags |= VanGuiWindowFlags_NoBackground;

    char label[32];
    snprintf(label, sizeof(label), "DockSpaceViewport_%08X", viewport->ID);

    PushStyleVar(VanGuiStyleVar_WindowRounding,    0.0f);
    PushStyleVar(VanGuiStyleVar_WindowBorderSize,  0.0f);
    PushStyleVar(VanGuiStyleVar_WindowPadding,     VanVec2(0.0f, 0.0f));
    SetNextWindowViewport(viewport->ID);
    Begin(label, nullptr, host_window_flags);
    PopStyleVar(3);

    if (dockspace_id == 0)
        dockspace_id = GetID("DockSpace");

    DockSpace(dockspace_id, VanVec2(0.0f, 0.0f), flags | VanGuiDockNodeFlags_KeepAliveOnly, window_class);
    End();

    return dockspace_id;
}

void SetNextWindowDockID(VanGuiID dock_id, VanGuiCond cond)
{
    VanGuiContext& g = *GVanGui;
    g.NextWindowData.HasFlags |= VanGuiNextWindowDataFlags_HasDock;
    g.NextWindowData.DockCond = (cond != 0) ? cond : VanGuiCond_Always;
    g.NextWindowData.DockId = dock_id;
}

void SetNextWindowClass(const VanGuiWindowClass* window_class)
{
    VanGuiContext& g = *GVanGui;
    VAN_ASSERT(window_class != nullptr);
    g.NextWindowData.HasFlags |= VanGuiNextWindowDataFlags_HasWindowClass;
    g.NextWindowData.WindowClass = *window_class;
}

[[nodiscard]] VanGuiID GetWindowDockID()
{
    VanGuiContext& g = *GVanGui;
    return g.CurrentWindow->DockId;
}

[[nodiscard]] bool IsWindowDocked()
{
    VanGuiContext& g = *GVanGui;
    return g.CurrentWindow->DockIsActive;
}

void SetNextWindowViewport(VanGuiID viewport_id)
{
    VanGuiContext& g = *GVanGui;
    g.NextWindowData.HasFlags |= VanGuiNextWindowDataFlags_HasViewport;
    g.NextWindowData.ViewportId = viewport_id;
    g.NextWindowData.ViewportCond = VanGuiCond_Always;
}

//-----------------------------------------------------------------------------
// [SECTION] Viewports: Public API
//-----------------------------------------------------------------------------

[[nodiscard]] VanGuiViewport* GetMainViewport()
{
    VanGuiContext& g = *GVanGui;
    return g.Viewports[0];
}

[[nodiscard]] VanGuiViewport* GetWindowViewport()
{
    VanGuiContext& g = *GVanGui;
    VAN_ASSERT(g.CurrentWindow != nullptr && g.CurrentWindow->Viewport != nullptr);
    return g.CurrentWindow->Viewport;
}

[[nodiscard]] VanGuiViewport* FindViewportByID(VanGuiID id)
{
    VanGuiContext& g = *GVanGui;
    for (VanGuiViewportP* viewport : g.Viewports)
        if (viewport->ID == id)
            return viewport;
    return nullptr;
}

[[nodiscard]] VanGuiViewport* FindViewportByPlatformHandle(void* platform_handle)
{
    VanGuiContext& g = *GVanGui;
    for (VanGuiViewportP* viewport : g.Viewports)
        if (viewport->PlatformHandle == platform_handle)
            return viewport;
    return nullptr;
}

//-----------------------------------------------------------------------------
// [SECTION] Viewports: Platform update loop
//-----------------------------------------------------------------------------

void UpdatePlatformWindows()
{
    VanGuiContext& g = *GVanGui;
    VAN_ASSERT(g.FrameCountEnded == g.FrameCount && "Forgot to call Render() or EndFrame() before UpdatePlatformWindows()?");
    VanGuiPlatformIO& platform_io = g.PlatformIO;
    g.FrameCountPlatformEnded = g.FrameCount;

    if (!(g.IO.ConfigFlags & VanGuiConfigFlags_ViewportsEnable))
        return;

    // Create/resize/destroy platform windows to match each active viewport.
    for (VanGuiViewportP* viewport : g.Viewports)
    {
        // Skip the main viewport — the host application manages it
        if (viewport->Flags & VanGuiViewportFlags_OwnedByApp)
            continue;

        const bool is_minimized = (viewport->Flags & VanGuiViewportFlags_IsMinimized) != 0;

        if (!viewport->PlatformWindowCreated)
        {
            // Create platform window
            if (platform_io.Platform_CreateWindow)
                platform_io.Platform_CreateWindow(viewport);
            if (platform_io.Renderer_CreateWindow)
                platform_io.Renderer_CreateWindow(viewport);
            viewport->LastNameHash = 0;
            viewport->LastPlatformPos = viewport->LastPlatformSize = VanVec2(FLT_MAX, FLT_MAX);
            viewport->PlatformWindowCreated = true;
        }
        else
        {
            // Apply any requested resize/move
            if (viewport->PlatformRequestClose && viewport != g.Viewports[0])
            {
                if (viewport->Window)
                    viewport->Window->WantClose = true;
                viewport->PlatformRequestClose = false;
            }

            if (!is_minimized && platform_io.Platform_GetWindowPos)
            {
                const VanVec2 platform_pos = platform_io.Platform_GetWindowPos(viewport);
                if (VanFabs(platform_pos.x - viewport->Pos.x) > 0.5f || VanFabs(platform_pos.y - viewport->Pos.y) > 0.5f)
                {
                    viewport->Pos = platform_pos;
                    viewport->PlatformRequestMove = false;
                }
            }
            if (!is_minimized && platform_io.Platform_GetWindowSize)
            {
                const VanVec2 platform_size = platform_io.Platform_GetWindowSize(viewport);
                if (VanFabs(platform_size.x - viewport->Size.x) > 0.5f || VanFabs(platform_size.y - viewport->Size.y) > 0.5f)
                {
                    if (platform_io.Renderer_SetWindowSize)
                        platform_io.Renderer_SetWindowSize(viewport, platform_size);
                    viewport->Size = platform_size;
                    viewport->PlatformRequestResize = false;
                }
            }
        }

        // Update title
        if (viewport->Window)
        {
            const VanGuiID title_hash = VanHashStr(viewport->Window->Name);
            if (viewport->LastNameHash != title_hash)
            {
                platform_io.Platform_SetWindowTitle(viewport, viewport->Window->Name);
                viewport->LastNameHash = title_hash;
            }
        }

        // Update alpha
        if (platform_io.Platform_SetWindowAlpha && viewport->Alpha != viewport->LastAlpha)
        {
            platform_io.Platform_SetWindowAlpha(viewport, viewport->Alpha);
            viewport->LastAlpha = viewport->Alpha;
        }

        // Show window (first time)
        if (!is_minimized && viewport->PlatformWindowCreated && !viewport->PlatformUserData)
        {
            if (platform_io.Platform_ShowWindow)
                platform_io.Platform_ShowWindow(viewport);
        }

        // Platform callback
        if (platform_io.Platform_UpdateWindow)
            platform_io.Platform_UpdateWindow(viewport);
    }
}

void RenderPlatformWindowsDefault(void* platform_render_arg, void* renderer_render_arg)
{
    VanGuiContext& g = *GVanGui;
    if (!(g.IO.ConfigFlags & VanGuiConfigFlags_ViewportsEnable))
        return;

    VanGuiPlatformIO& platform_io = g.PlatformIO;
    for (VanGuiViewportP* viewport : g.Viewports)
    {
        if (viewport->Flags & VanGuiViewportFlags_IsMinimized)
            continue;
        if (platform_io.Platform_RenderWindow)
            platform_io.Platform_RenderWindow(viewport, platform_render_arg);
        if (platform_io.Renderer_RenderWindow)
            platform_io.Renderer_RenderWindow(viewport, renderer_render_arg);
        if (platform_io.Platform_SwapBuffers)
            platform_io.Platform_SwapBuffers(viewport, platform_render_arg);
        if (platform_io.Renderer_SwapBuffers)
            platform_io.Renderer_SwapBuffers(viewport, renderer_render_arg);
    }
}

void DestroyPlatformWindows()
{
    VanGuiContext& g = *GVanGui;
    VanGuiPlatformIO& platform_io = g.PlatformIO;
    for (VanGuiViewportP* viewport : g.Viewports)
    {
        if (viewport->PlatformWindowCreated)
        {
            if (platform_io.Renderer_DestroyWindow)
                platform_io.Renderer_DestroyWindow(viewport);
            if (platform_io.Platform_DestroyWindow)
                platform_io.Platform_DestroyWindow(viewport);
            viewport->PlatformWindowCreated = false;
        }
        VAN_ASSERT(viewport->RendererUserData == nullptr && viewport->PlatformUserData == nullptr);
    }
}

//-----------------------------------------------------------------------------
// [SECTION] Docking: Tab bar management
//-----------------------------------------------------------------------------

static void DockNodeAddTabBar(VanGuiDockNode* node)
{
    VAN_ASSERT(node->IsLeafNode && node->TabBar == nullptr);
    node->TabBar = VAN_NEW(VanGuiTabBar)();
    node->TabBar->ID = node->ID;
}

static void DockNodeRemoveTabBar(VanGuiDockNode* node)
{
    if (node->TabBar == nullptr)
        return;
    VAN_DELETE(node->TabBar);
    node->TabBar = nullptr;
}

static void DockNodeUpdateVisibleFlag(VanGuiDockNode* node)
{
    if (node->IsLeafNode)
    {
        if (node->Windows.Size == 0)
        {
            node->VisibleWindow = nullptr;
            return;
        }
        VanGuiWindow* found = nullptr;
        for (int n = 0; n < node->Windows.Size; n++)
        {
            if (node->Windows[n]->ID == node->SelectedTabId)
            {
                found = node->Windows[n];
                break;
            }
        }
        if (found != nullptr)
        {
            node->VisibleWindow = found;
        }
        else
        {
            node->VisibleWindow = node->Windows[0];
            node->SelectedTabId = node->Windows[0]->ID;
        }
    }
    else
    {
        node->VisibleWindow = nullptr;
        for (int i = 0; i < 2; i++)
        {
            if (node->ChildNodes[i] != nullptr && node->ChildNodes[i]->VisibleWindow != nullptr)
                node->VisibleWindow = node->ChildNodes[i]->VisibleWindow;
        }
    }
}

static void DockNodeCalcTabBarLayout(const VanGuiDockNode* node, VanRect* out_title_rect, VanRect* out_tab_bar_rect, VanVec2* out_window_menu_button_pos)
{
    VanGuiContext& g = *GVanGui;
    VanGuiStyle& style = g.Style;
    *out_title_rect   = VanRect(node->Pos, node->Pos + VanVec2(node->Size.x, g.FontSize + style.FramePadding.y * 2.0f));
    *out_tab_bar_rect = VanRect(out_title_rect->Min + VanVec2(0.0f, style.FramePadding.y), out_title_rect->Max);
    if (out_window_menu_button_pos != nullptr)
        *out_window_menu_button_pos = out_title_rect->Min;
}

static void DockNodeUpdateTabBar(VanGuiDockNode* node, VanGuiWindow* host_window)
{
    VanGuiContext& g = *GVanGui;

    // Create tab bar if needed (and not suppressed)
    if (node->TabBar == nullptr && !node->IsHiddenTabBar)
        DockNodeAddTabBar(node);

    // Hidden tab bar or single-window shortcut — no tab bar UI required
    const bool single_window = (node->Windows.Size == 1);
    if (node->IsHiddenTabBar || single_window)
    {
        if (node->Windows.Size > 0)
            node->VisibleWindow = node->Windows[0];
        node->LastFrameActive = g.FrameCount;
        return;
    }

    // Ensure tab bar exists for multi-window nodes
    if (node->TabBar == nullptr)
        DockNodeAddTabBar(node);

    VanGuiTabBar* tab_bar = node->TabBar;

    // Calculate tab bar layout geometry
    VanRect title_bar_rect, tab_bar_rect;
    VanVec2 window_menu_button_pos;
    DockNodeCalcTabBarLayout(node, &title_bar_rect, &tab_bar_rect, &window_menu_button_pos);

    // Tab bar flags
    VanGuiTabBarFlags tab_bar_flags =
        VanGuiTabBarFlags_Reorderable |
        VanGuiTabBarFlags_AutoSelectNewTabs |
        VanGuiTabBarFlags_NoTabListScrollingButtons;

    // Propagate the node's desired selected tab into the tab bar
    if (node->SelectedTabId != tab_bar->SelectedTabId)
        tab_bar->NextSelectedTabId = node->SelectedTabId;

    // Draw title-bar background behind the tabs
    if (host_window != nullptr)
        host_window->DrawList->AddRectFilled(title_bar_rect.Min, title_bar_rect.Max, GetColorU32(VanGuiCol_TitleBg));

    // Begin the tab bar
    BeginTabBarEx(tab_bar, tab_bar_rect, tab_bar_flags);

    // Submit / update a tab entry for every window docked in this node
    for (int window_n = 0; window_n < node->Windows.Size; window_n++)
    {
        VanGuiWindow* window = node->Windows[window_n];

        VanGuiTabItem* tab = TabBarFindTabByID(tab_bar, window->ID);
        if (tab == nullptr)
        {
            // First time we see this window — register a new tab
            VanGuiTabItem new_tab = {};
            new_tab.ID = window->ID;
            new_tab.LastFrameVisible = g.FrameCount;
            tab_bar->Tabs.push_back(new_tab);
        }
        else
        {
            tab->LastFrameVisible = g.FrameCount;
        }
    }

    // Remove stale tabs whose windows are no longer docked in this node
    for (int tab_n = tab_bar->Tabs.Size - 1; tab_n >= 0; tab_n--)
    {
        bool found = false;
        for (int w = 0; w < node->Windows.Size; w++)
        {
            if (node->Windows[w]->ID == tab_bar->Tabs[tab_n].ID)
            {
                found = true;
                break;
            }
        }
        if (!found)
            tab_bar->Tabs.erase(&tab_bar->Tabs[tab_n]);
    }

    // Sync selection back to the node
    if (tab_bar->SelectedTabId != 0)
        node->SelectedTabId = tab_bar->SelectedTabId;

    // Update which window should be visible
    DockNodeUpdateVisibleFlag(node);

    node->LastFrameActive = g.FrameCount;
}

static void DockNodeUpdate(VanGuiDockNode* node)
{
    VanGuiContext& g = *GVanGui;

    // Skip nodes that were not kept alive this frame
    if (node->LastFrameAlive < g.FrameCount)
        return;

    // Recurse into child nodes first (depth-first)
    if (node->ChildNodes[0] != nullptr) DockNodeUpdate(node->ChildNodes[0]);
    if (node->ChildNodes[1] != nullptr) DockNodeUpdate(node->ChildNodes[1]);

    // Leaf nodes host the actual windows and their tab bar
    if (node->IsLeafNode)
    {
        DockNodeUpdateTabBar(node, node->HostWindow);
        DockNodeUpdateVisibleFlag(node);
    }

    node->LastFrameActive = g.FrameCount;
}

//-----------------------------------------------------------------------------
// [SECTION] Docking: Settings handlers — forward declarations
//-----------------------------------------------------------------------------

static void  DockSettingsHandler_ClearAll(VanGuiContext* ctx, VanGuiSettingsHandler*);
static void* DockSettingsHandler_ReadOpen(VanGuiContext* ctx, VanGuiSettingsHandler*, const char* name);
static void  DockSettingsHandler_ReadLine(VanGuiContext* ctx, VanGuiSettingsHandler*, void* entry, const char* line);
static void  DockSettingsHandler_ApplyAll(VanGuiContext* ctx, VanGuiSettingsHandler*);
static void  DockSettingsHandler_WriteAll(VanGuiContext* ctx, VanGuiSettingsHandler*, VanGuiTextBuffer* buf);

//-----------------------------------------------------------------------------
// [SECTION] Docking: Node add/remove window, tree split
//-----------------------------------------------------------------------------

static void DockNodeAddWindow(VanGuiDockNode* node, VanGuiWindow* window, bool add_to_tab_bar)
{
    VAN_ASSERT(node->IsLeafNode);

    // Guard: don't add the same window twice
    for (int i = 0; i < node->Windows.Size; i++)
        if (node->Windows[i] == window)
            return;

    node->Windows.push_back(window);
    window->DockNode     = node;
    window->DockIsActive = true;
    window->DockId       = node->ID;

    // Select first window automatically
    if (node->SelectedTabId == 0)
        node->SelectedTabId = window->ID;

    // Update timestamps
    node->LastFrameAlive  = GVanGui->FrameCount;
    node->LastFrameActive = GVanGui->FrameCount;
    node->IsLeafNode      = true;

    // Resolve visible window from selected tab
    node->VisibleWindow = nullptr;
    for (int i = 0; i < node->Windows.Size; i++)
        if (node->Windows[i]->ID == node->SelectedTabId)
        {
            node->VisibleWindow = node->Windows[i];
            break;
        }
    if (node->VisibleWindow == nullptr && node->Windows.Size > 0)
        node->VisibleWindow = node->Windows[0];

    (void)add_to_tab_bar; // tab-bar integration handled at a higher level
}

static void DockNodeRemoveWindow(VanGuiDockNode* node, VanGuiWindow* window, VanGuiID save_dock_id)
{
    VAN_ASSERT(window->DockNode == node);

    // Remove the window from the list
    for (int i = 0; i < node->Windows.Size; i++)
    {
        if (node->Windows[i] == window)
        {
            node->Windows.erase(node->Windows.Data + i);
            break;
        }
    }

    window->DockNode     = nullptr;
    window->DockIsActive = false;
    window->DockId       = save_dock_id;

    if (node->Windows.Size == 0 && !node->IsDockSpace)
    {
        DockContextRemoveNode(GVanGui, node, true);
        return;
    }

    // Pick a new selected tab if the removed window was selected
    if (node->SelectedTabId == window->ID)
        node->SelectedTabId = (node->Windows.Size > 0) ? node->Windows[0]->ID : 0;

    // Update visible window
    node->VisibleWindow = nullptr;
    for (int i = 0; i < node->Windows.Size; i++)
        if (node->Windows[i]->ID == node->SelectedTabId)
        {
            node->VisibleWindow = node->Windows[i];
            break;
        }
    if (node->VisibleWindow == nullptr && node->Windows.Size > 0)
        node->VisibleWindow = node->Windows[0];
}

// Helper: compute split rectangles from a parent rect split along dir.
static void DockNodeCalcSplitRects(VanVec2& pos_old, VanVec2& size_old,
                                    VanVec2& pos_new, VanVec2& size_new,
                                    VanGuiDir dir, VanVec2 size_new_desired)
{
    VanGuiContext& g = *GVanGui;
    const float splitter_sz = g.Style.ItemInnerSpacing.x;

    if (dir == VanGuiDir_Left || dir == VanGuiDir_Right)
    {
        // Horizontal split
        const float w_new = VanClamp(size_new_desired.x, 1.0f, size_old.x - splitter_sz - 1.0f);
        if (dir == VanGuiDir_Left)
        {
            pos_new  = pos_old;
            size_new = VanVec2(w_new, size_old.y);
            pos_old  = VanVec2(pos_old.x + w_new + splitter_sz, pos_old.y);
            size_old = VanVec2(size_old.x - w_new - splitter_sz, size_old.y);
        }
        else // Right
        {
            pos_new  = VanVec2(pos_old.x + size_old.x - w_new, pos_old.y);
            size_new = VanVec2(w_new, size_old.y);
            size_old = VanVec2(size_old.x - w_new - splitter_sz, size_old.y);
        }
    }
    else
    {
        // Vertical split
        const float h_new = VanClamp(size_new_desired.y, 1.0f, size_old.y - splitter_sz - 1.0f);
        if (dir == VanGuiDir_Up)
        {
            pos_new  = pos_old;
            size_new = VanVec2(size_old.x, h_new);
            pos_old  = VanVec2(pos_old.x, pos_old.y + h_new + splitter_sz);
            size_old = VanVec2(size_old.x, size_old.y - h_new - splitter_sz);
        }
        else // Down
        {
            pos_new  = VanVec2(pos_old.x, pos_old.y + size_old.y - h_new);
            size_new = VanVec2(size_old.x, h_new);
            size_old = VanVec2(size_old.x, size_old.y - h_new - splitter_sz);
        }
    }
}

// Split parent_node along split_axis.  Returns the child that did NOT inherit
// the existing windows (i.e. the empty slot ready for a new window).
static VanGuiDockNode* DockNodeTreeSplit(VanGuiContext* ctx, VanGuiDockNode* parent_node,
                                          VanGuiAxis split_axis, int split_inheritor_child_idx,
                                          float split_ratio, VanGuiDockNode* new_node)
{
    VAN_ASSERT(split_axis != VanGuiAxis_None);
    VAN_ASSERT(split_inheritor_child_idx == 0 || split_inheritor_child_idx == 1);

    // Allocate the two child nodes (re-use new_node for one side when provided)
    VanGuiDockNode* child_0 = (new_node != nullptr && split_inheritor_child_idx != 0)
                                  ? new_node : DockContextAddNode(ctx, 0);
    VanGuiDockNode* child_1 = (new_node != nullptr && split_inheritor_child_idx != 1)
                                  ? new_node : DockContextAddNode(ctx, 0);

    child_0->ParentNode = parent_node;
    child_1->ParentNode = parent_node;

    // The "inheritor" child takes over all existing windows
    VanGuiDockNode* child_inheritor = (split_inheritor_child_idx == 0) ? child_0 : child_1;
    VanGuiDockNode* child_empty     = (split_inheritor_child_idx == 0) ? child_1 : child_0;

    // Move windows from parent into the inheritor child
    for (int i = 0; i < parent_node->Windows.Size; i++)
    {
        VanGuiWindow* w = parent_node->Windows[i];
        w->DockNode = child_inheritor;
        child_inheritor->Windows.push_back(w);
    }
    child_inheritor->SelectedTabId   = parent_node->SelectedTabId;
    child_inheritor->VisibleWindow   = parent_node->VisibleWindow;
    child_inheritor->IsLeafNode      = true;
    child_inheritor->LastFrameAlive  = parent_node->LastFrameAlive;
    child_inheritor->LastFrameActive = parent_node->LastFrameActive;

    parent_node->Windows.clear();
    parent_node->SelectedTabId  = 0;
    parent_node->VisibleWindow  = nullptr;
    parent_node->IsLeafNode     = false;
    parent_node->ChildNodes[0]  = child_0;
    parent_node->ChildNodes[1]  = child_1;
    parent_node->SplitAxis      = split_axis;

    // Size the two children from split_ratio
    const VanVec2 parent_size = parent_node->Size;
    const VanVec2 parent_pos  = parent_node->Pos;

    VanVec2 pos_old  = parent_pos;
    VanVec2 size_old = parent_size;
    VanVec2 pos_new, size_new;

    // Map split_axis + split_inheritor_child_idx to a direction for the empty (new) side.
    // child_0 = NW, child_1 = SE; child_empty goes to the "new" side.
    VanGuiDir split_dir;
    if (split_axis == VanGuiAxis_X)
        split_dir = (split_inheritor_child_idx == 0) ? VanGuiDir_Right : VanGuiDir_Left;
    else
        split_dir = (split_inheritor_child_idx == 0) ? VanGuiDir_Down : VanGuiDir_Up;

    VanVec2 size_new_desired;
    if (split_axis == VanGuiAxis_X)
        size_new_desired = VanVec2(VanFloor(parent_size.x * (1.0f - split_ratio)), parent_size.y);
    else
        size_new_desired = VanVec2(parent_size.x, VanFloor(parent_size.y * (1.0f - split_ratio)));

    DockNodeCalcSplitRects(pos_old, size_old, pos_new, size_new, split_dir, size_new_desired);

    // child_0 = NW side, child_1 = SE side
    child_0->Pos     = (split_inheritor_child_idx == 0) ? pos_old  : pos_new;
    child_0->Size    = (split_inheritor_child_idx == 0) ? size_old : size_new;
    child_0->SizeRef = child_0->Size;
    child_1->Pos     = (split_inheritor_child_idx == 0) ? pos_new  : pos_old;
    child_1->Size    = (split_inheritor_child_idx == 0) ? size_new : size_old;
    child_1->SizeRef = child_1->Size;

    return child_empty;
}

//-----------------------------------------------------------------------------
// [SECTION] Docking: Context queue/process requests
//-----------------------------------------------------------------------------

static void DockContextProcessUndockWindow(VanGuiContext* ctx, VanGuiWindow* window)
{
    if (window->DockNode != nullptr)
        DockNodeRemoveWindow(window->DockNode, window, 0);
    window->DockId = 0;
    (void)ctx;
}

static void DockContextProcessUndockNode(VanGuiContext* ctx, VanGuiDockNode* node)
{
    VAN_ASSERT(node->IsLeafNode);

    if (node->IsRootNode())
    {
        // Floating root: undock every window individually
        for (int i = node->Windows.Size - 1; i >= 0; i--)
            DockContextQueueUndockWindow(ctx, node->Windows[i]);
    }
    else
    {
        // Non-root leaf: detach by creating a new floating root and migrating windows
        VanGuiDockNode* new_root = DockContextAddNode(ctx, 0);
        new_root->Pos     = node->Pos;
        new_root->Size    = node->Size;
        new_root->SizeRef = node->SizeRef;

        for (int i = 0; i < node->Windows.Size; i++)
        {
            VanGuiWindow* w = node->Windows[i];
            w->DockNode = new_root;
            new_root->Windows.push_back(w);
        }
        new_root->SelectedTabId   = node->SelectedTabId;
        new_root->VisibleWindow   = node->VisibleWindow;
        new_root->IsLeafNode      = true;
        new_root->LastFrameAlive  = node->LastFrameAlive;
        new_root->LastFrameActive = node->LastFrameActive;

        node->Windows.clear();
        node->SelectedTabId = 0;
        node->VisibleWindow = nullptr;

        DockContextRemoveNode(ctx, node, true);
    }
}

static void DockContextProcessDockRequest(VanGuiContext* ctx, VanGuiDockRequest* req)
{
    if (req->Type == VanGuiDockRequestType_Undock)
    {
        if (req->UndockTargetWindow != nullptr)
            DockContextProcessUndockWindow(ctx, req->UndockTargetWindow);
        if (req->UndockTargetNode != nullptr)
            DockContextProcessUndockNode(ctx, req->UndockTargetNode);
    }
    else if (req->Type == VanGuiDockRequestType_Dock)
    {
        VanGuiWindow* payload = req->DockPayload;
        if (req->DockSplitDir != VanGuiDir_None)
        {
            // Split the target node and dock into the new empty slot
            const VanGuiAxis split_axis = (req->DockSplitDir == VanGuiDir_Left ||
                                           req->DockSplitDir == VanGuiDir_Right)
                                              ? VanGuiAxis_X : VanGuiAxis_Y;
            // inheritor keeps the existing content; new empty slot gets the payload
            const int inheritor_idx = (req->DockSplitDir == VanGuiDir_Left ||
                                       req->DockSplitDir == VanGuiDir_Up) ? 1 : 0;
            VanGuiDockNode* new_node = DockNodeTreeSplit(ctx, req->DockTargetNode,
                                                         split_axis, inheritor_idx,
                                                         req->DockSplitRatio, nullptr);
            DockNodeAddWindow(new_node, payload, true);
        }
        else
        {
            DockNodeAddWindow(req->DockTargetNode, payload, true);
        }
    }
}

void DockContextQueueDock(VanGuiContext* ctx, VanGuiWindow* target_window,
                           VanGuiDockNode* target_node, VanGuiWindow* payload,
                           VanGuiDir split_dir, float split_ratio, bool split_outer)
{
    VanGuiDockRequest req;
    req.Type             = VanGuiDockRequestType_Dock;
    req.DockTargetWindow = target_window;
    req.DockTargetNode   = target_node;
    req.DockPayload      = payload;
    req.DockSplitDir     = split_dir;
    req.DockSplitRatio   = split_ratio;
    req.DockSplitOuter   = split_outer;
    ctx->DockContext.Requests.push_back(req);
}

void DockContextQueueUndockWindow(VanGuiContext* ctx, VanGuiWindow* window)
{
    VanGuiDockRequest req;
    req.Type               = VanGuiDockRequestType_Undock;
    req.UndockTargetWindow = window;
    ctx->DockContext.Requests.push_back(req);
}

void DockContextQueueUndockNode(VanGuiContext* ctx, VanGuiDockNode* node)
{
    VanGuiDockRequest req;
    req.Type             = VanGuiDockRequestType_Undock;
    req.UndockTargetNode = node;
    ctx->DockContext.Requests.push_back(req);
}

void DockContextNewFrameUpdateDocking(VanGuiContext* ctx)
{
    if (!(ctx->IO.ConfigFlags & VanGuiConfigFlags_DockingEnable))
    {
        ctx->DockContext.Requests.clear();
        return;
    }

    // Process all queued dock/undock requests
    for (int i = 0; i < ctx->DockContext.Requests.Size; i++)
        DockContextProcessDockRequest(ctx, &ctx->DockContext.Requests[i]);
    ctx->DockContext.Requests.clear();

    // Garbage-collect stale floating leaf nodes with no windows
    for (int i = 0; i < ctx->DockContext.Nodes.Data.Size; i++)
    {
        VanGuiDockNode* node = static_cast<VanGuiDockNode*>(ctx->DockContext.Nodes.Data[i].val_p);
        if (node == nullptr)
            continue;
        if (node->LastFrameAlive < ctx->FrameCount - 1
            && node->IsFloatingNode()
            && node->Windows.Size == 0
            && node->ChildNodes[0] == nullptr)
        {
            DockContextRemoveNode(ctx, node, false);
        }
    }
}

//-----------------------------------------------------------------------------
// [SECTION] Docking: Context initialization
//-----------------------------------------------------------------------------

void DockContextInitialize(VanGuiContext* ctx)
{
    VanGuiSettingsHandler ini_handler;
    ini_handler.TypeName   = "DockNode";
    ini_handler.TypeHash   = VanHashStr("DockNode");
    ini_handler.ClearAllFn = DockSettingsHandler_ClearAll;
    ini_handler.ReadOpenFn = DockSettingsHandler_ReadOpen;
    ini_handler.ReadLineFn = DockSettingsHandler_ReadLine;
    ini_handler.ApplyAllFn = DockSettingsHandler_ApplyAll;
    ini_handler.WriteAllFn = DockSettingsHandler_WriteAll;
    AddSettingsHandler(&ini_handler);
}

//-----------------------------------------------------------------------------
// [SECTION] Docking: Settings handlers
//-----------------------------------------------------------------------------

static void DockSettingsHandler_ClearAll(VanGuiContext* ctx, VanGuiSettingsHandler*)
{
    ctx->DockContext.NodesSettings.clear();
}

static void* DockSettingsHandler_ReadOpen(VanGuiContext* ctx, VanGuiSettingsHandler*, const char* name)
{
    VanGuiDockNodeSettings node_settings = {};
    // Name is the node ID written as hex, e.g. "0x12345678"
    node_settings.ID = static_cast<VanGuiID>(strtoul(name, nullptr, 16));
    ctx->DockContext.NodesSettings.push_back(node_settings);
    return &ctx->DockContext.NodesSettings.back();
}

static void DockSettingsHandler_ReadLine(VanGuiContext* /*ctx*/, VanGuiSettingsHandler*,
                                          void* entry, const char* line)
{
    VanGuiDockNodeSettings* ns = static_cast<VanGuiDockNodeSettings*>(entry);

    unsigned int u = 0;
    int x = 0, y = 0;
    char axis_ch = 0;

    if (sscanf(line, "Parent=0x%08X", &u) == 1)
    {
        ns->ParentNodeId = static_cast<VanGuiID>(u);
    }
    else if (sscanf(line, "Size=%i,%i", &x, &y) == 2)
    {
        ns->Size = VanVec2ih(static_cast<short>(x), static_cast<short>(y));
    }
    else if (sscanf(line, "SizeRef=%i,%i", &x, &y) == 2)
    {
        ns->SizeRef = VanVec2ih(static_cast<short>(x), static_cast<short>(y));
    }
    else if (sscanf(line, "Split=%c", &axis_ch) == 1)
    {
        if (axis_ch == 'X')      ns->SplitAxis = static_cast<VanS8>(VanGuiAxis_X);
        else if (axis_ch == 'Y') ns->SplitAxis = static_cast<VanS8>(VanGuiAxis_Y);
        else                     ns->SplitAxis = static_cast<VanS8>(VanGuiAxis_None);
    }
    else if (sscanf(line, "Selected=0x%08X", &u) == 1)
    {
        ns->SelectedTabId = static_cast<VanGuiID>(u);
    }
    else if (sscanf(line, "Flags=0x%08X", &u) == 1)
    {
        ns->Flags = static_cast<VanGuiDockNodeFlags>(u);
    }
}

static void DockSettingsHandler_ApplyAll(VanGuiContext* ctx, VanGuiSettingsHandler*)
{
    // First pass: create all nodes
    for (int i = 0; i < ctx->DockContext.NodesSettings.Size; i++)
    {
        VanGuiDockNodeSettings& ns = ctx->DockContext.NodesSettings[i];
        VanGuiDockNode* node = DockContextAddNode(ctx, ns.ID);
        node->SharedFlags   = ns.Flags;
        node->SplitAxis     = static_cast<VanGuiAxis>(ns.SplitAxis);
        node->SelectedTabId = ns.SelectedTabId;
        node->Size          = VanVec2(static_cast<float>(ns.Size.x), static_cast<float>(ns.Size.y));
        node->SizeRef       = VanVec2(static_cast<float>(ns.SizeRef.x), static_cast<float>(ns.SizeRef.y));
        node->IsLeafNode    = (ns.SplitAxis == static_cast<VanS8>(VanGuiAxis_None));
    }

    // Second pass: reconnect parent/child links
    for (int i = 0; i < ctx->DockContext.NodesSettings.Size; i++)
    {
        VanGuiDockNodeSettings& ns = ctx->DockContext.NodesSettings[i];
        if (ns.ParentNodeId == 0)
            continue;
        VanGuiDockNode* node   = DockContextFindNodeByID(ctx, ns.ID);
        VanGuiDockNode* parent = DockContextFindNodeByID(ctx, ns.ParentNodeId);
        if (node == nullptr || parent == nullptr)
            continue;
        node->ParentNode = parent;
        if (parent->ChildNodes[0] == nullptr)
            parent->ChildNodes[0] = node;
        else
            parent->ChildNodes[1] = node;
    }
}

static void DockSettingsHandler_WriteAll(VanGuiContext* ctx, VanGuiSettingsHandler*, VanGuiTextBuffer* buf)
{
    for (int i = 0; i < ctx->DockContext.Nodes.Data.Size; i++)
    {
        VanGuiDockNode* node = static_cast<VanGuiDockNode*>(ctx->DockContext.Nodes.Data[i].val_p);
        if (node == nullptr)
            continue;

        buf->appendf("[DockNode][0x%08X]\n", node->ID);

        if (node->ParentNode != nullptr)
            buf->appendf("Parent=0x%08X\n", node->ParentNode->ID);

        if (node->SplitAxis != VanGuiAxis_None)
            buf->appendf("Split=%c\n", (node->SplitAxis == VanGuiAxis_X) ? 'X' : 'Y');

        if (node->Size.x != 0.0f || node->Size.y != 0.0f)
            buf->appendf("Size=%d,%d\n",
                static_cast<int>(node->Size.x), static_cast<int>(node->Size.y));

        if (node->SizeRef.x != 0.0f || node->SizeRef.y != 0.0f)
            buf->appendf("SizeRef=%d,%d\n",
                static_cast<int>(node->SizeRef.x), static_cast<int>(node->SizeRef.y));

        if (node->SelectedTabId != 0)
            buf->appendf("Selected=0x%08X\n", node->SelectedTabId);

        if (node->SharedFlags != 0)
            buf->appendf("Flags=0x%08X\n", static_cast<unsigned int>(node->SharedFlags));

        buf->appendf("\n");
    }
}

} // namespace VanGui

#endif // VANGUI_DISABLE
