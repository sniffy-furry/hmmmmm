// vangui_panels.cpp — see vangui_panels.h. Empty TU unless enabled.
#include "vangui_panels.h"

#ifdef VANGUI_ENABLE_PANELS

#include "vangui_anim.h"
#include <cmath>

namespace VanGui {

bool Splitter(const char* id, bool split_vertically, float thickness,
              float* size1, float* size2, float min1, float min2)
{
    PushID(id);
    const VanVec2 avail = GetContentRegionAvail();
    const VanVec2 sz = split_vertically ? VanVec2(thickness, avail.y) : VanVec2(avail.x, thickness);
    (void)InvisibleButton("##splitter", sz);
    const bool hovered = IsItemHovered();
    const bool active  = IsItemActive();
    if (hovered || active)
        SetMouseCursor(split_vertically ? VanGuiMouseCursor_ResizeEW : VanGuiMouseCursor_ResizeNS);

    bool changed = false;
    if (active)
    {
        float d = split_vertically ? GetIO().MouseDelta.x : GetIO().MouseDelta.y;
        if (d != 0.0f)
        {
            if (*size1 + d < min1) d = min1 - *size1;
            if (*size2 - d < min2) d = *size2 - min2;
            if (d != 0.0f) { *size1 += d; *size2 -= d; changed = true; }
        }
    }

    const VanVec2 mn = GetItemRectMin(), mx = GetItemRectMax();
    const VanU32 col = GetColorU32(active ? VanGuiCol_SeparatorActive
                                          : (hovered ? VanGuiCol_SeparatorHovered : VanGuiCol_Separator));
    GetWindowDrawList()->AddRectFilled(mn, mx, col, thickness * 0.4f);
    PopID();
    return changed;
}

// --- accordion --------------------------------------------------------------
namespace { constexpr int kAccMax = 32; static bool s_accPushedIndent[kAccMax]; static int s_accDepth = 0; }

bool AccordionSection(const char* label, bool* open)
{
    PushID(label);
    const float h = GetFrameHeight();
    const VanVec2 p = GetCursorScreenPos();
    const float w = GetContentRegionAvail().x;

    const bool clicked = InvisibleButton("##acc", VanVec2(w, h));
    if (clicked) *open = !*open;
    const bool hov = IsItemHovered();

    const float t = Anim::AnimFloat(GetID("##accT"), *open ? 1.0f : 0.0f,
                                    { .Duration = 0.14f, .Easing = Anim::VanEasing_CubicOut });

    VanDrawList* dl = GetWindowDrawList();
    if (hov) dl->AddRectFilled(p, VanVec2(p.x + w, p.y + h), GetColorU32(VanGuiCol_HeaderHovered), 3.0f);

    // chevron: rotates 0 (right) -> 90deg (down) with t
    const float cy = p.y + h * 0.5f;
    const float cx = p.x + h * 0.5f;
    const float a = t * 1.5707963f;          // 0..90deg
    const float r = h * 0.18f;
    const float ca = std::cos(a), sa = std::sin(a);
    auto rot = [&](float x, float y) { return VanVec2(cx + (x * ca - y * sa), cy + (x * sa + y * ca)); };
    const VanU32 ic = GetColorU32(VanGuiCol_Text);
    dl->AddLine(rot(-r * 0.4f, -r), rot(r * 0.6f, 0), ic, 1.6f);
    dl->AddLine(rot(r * 0.6f, 0), rot(-r * 0.4f, r), ic, 1.6f);

    dl->AddText(VanVec2(p.x + h, p.y + (h - GetTextLineHeight()) * 0.5f), GetColorU32(VanGuiCol_Text), label);

    const bool show = *open;
    if (show) { Indent(h * 0.5f); if (s_accDepth < kAccMax) s_accPushedIndent[s_accDepth++] = true; }
    else      { if (s_accDepth < kAccMax) s_accPushedIndent[s_accDepth++] = false; }
    return show;
}

void AccordionEnd()
{
    if (s_accDepth > 0) { if (s_accPushedIndent[--s_accDepth]) Unindent(GetFrameHeight() * 0.5f); }
    PopID();
}

// --- status bar -------------------------------------------------------------
bool BeginStatusBar()
{
    const VanGuiViewport* vp = GetMainViewport();
    const float h = GetFrameHeight();
    SetNextWindowPos(VanVec2(vp->WorkPos.x, vp->WorkPos.y + vp->WorkSize.y - h));
    SetNextWindowSize(VanVec2(vp->WorkSize.x, h));
    PushStyleVar(VanGuiStyleVar_WindowRounding, 0.0f);
    PushStyleVar(VanGuiStyleVar_WindowBorderSize, 0.0f);
    PushStyleVar(VanGuiStyleVar_WindowPadding, VanVec2(8.0f, 3.0f));
    const VanGuiWindowFlags f = VanGuiWindowFlags_NoTitleBar | VanGuiWindowFlags_NoResize |
                                VanGuiWindowFlags_NoMove | VanGuiWindowFlags_NoScrollbar |
                                VanGuiWindowFlags_NoSavedSettings | VanGuiWindowFlags_NoNav |
                                VanGuiWindowFlags_NoBringToFrontOnFocus;
    const bool open = Begin("##van_statusbar", nullptr, f);
    PopStyleVar(3);
    if (open) AlignTextToFramePadding();
    return open;
}
void EndStatusBar() { End(); }

// --- toolbar ----------------------------------------------------------------
bool BeginToolbar(const char* id, float height)
{
    if (height <= 0.0f) height = GetFrameHeight() + GetStyle().FramePadding.y * 2.0f;
    PushStyleVar(VanGuiStyleVar_ItemSpacing, VanVec2(4.0f, GetStyle().ItemSpacing.y));
    const bool open = BeginChild(id, VanVec2(0, height), VanGuiChildFlags_Borders,
                                 VanGuiWindowFlags_NoScrollbar | VanGuiWindowFlags_NoScrollWithMouse);
    if (open) AlignTextToFramePadding();
    return open;
}
void EndToolbar()
{
    EndChild();
    PopStyleVar();
}
void ToolbarSeparator()
{
    SameLine();
    const VanVec2 p = GetCursorScreenPos();
    const float h = GetFrameHeight();
    GetWindowDrawList()->AddLine(VanVec2(p.x + 3, p.y + 2), VanVec2(p.x + 3, p.y + h - 2),
                                 GetColorU32(VanGuiCol_Separator), 1.0f);
    Dummy(VanVec2(6, h));
    SameLine();
}

} // namespace VanGui

#endif // VANGUI_ENABLE_PANELS
