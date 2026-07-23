// vangui_feedback.cpp — see vangui_feedback.h. Empty TU unless enabled.
#include "vangui_feedback.h"

#ifdef VANGUI_ENABLE_FEEDBACK

#include "vangui_anim.h"
#include <cstdio>
#include <cmath>

namespace VanGui {

void AnimatedValue(const char* id, float value, const char* fmt)
{
    const float shown = Anim::AnimFloat(GetID(id), value, { .Duration = 0.5f, .Easing = Anim::VanEasing_CubicOut });
    char buf[64];
    std::snprintf(buf, sizeof buf, fmt, shown);
    TextUnformatted(buf);
}

bool RippleButton(const char* label, VanVec2 size)
{
    const VanGuiID rid = GetID(label);
    const bool clicked = Button(label, size);
    const VanVec2 mn = GetItemRectMin(), mx = GetItemRectMax();
    if (clicked) Anim::Reset(rid);                 // restart the ripple
    const float t = Anim::AnimFloat(rid, clicked ? 0.0f : 1.0f, { .Duration = 0.45f, .Easing = Anim::VanEasing_QuadOut });

    if (t < 1.0f)
    {
        VanDrawList* dl = GetWindowDrawList();
        dl->PushClipRect(mn, mx, true);
        const VanVec2 c((mn.x + mx.x) * 0.5f, (mn.y + mx.y) * 0.5f);
        const float dx = mx.x - mn.x, dy = mx.y - mn.y;
        const float maxr = std::sqrt(dx * dx + dy * dy) * 0.5f;
        const int alpha = (int)((1.0f - t) * 90.0f);
        dl->AddCircleFilled(c, t * maxr, VAN_COL32(255, 255, 255, alpha));
        dl->PopClipRect();
    }
    return clicked;
}

bool ElevatedButton(const char* label, VanVec2 size)
{
    const bool clicked = Button(label, size);
    const VanVec2 mn = GetItemRectMin(), mx = GetItemRectMax();
    const float e = Anim::AnimFloat(GetID(label), IsItemHovered() ? 1.0f : 0.0f,
                                    { .Duration = 0.12f, .Easing = Anim::VanEasing_QuadOut });
    if (e > 0.001f)
    {
        VanDrawList* dl = GetWindowDrawList();
        const float grow = e * 3.0f;
        const VanVec4 acc = GetStyleColorVec4(VanGuiCol_ButtonActive);
        dl->AddRect(VanVec2(mn.x - grow, mn.y - grow), VanVec2(mx.x + grow, mx.y + grow),
                    GetColorU32(VanVec4(acc.x, acc.y, acc.z, e * 0.55f)),
                    GetStyle().FrameRounding + grow, 0, 1.5f);
    }
    return clicked;
}

bool ReorderableList(const char* id, const char* const* labels, int* order, int count)
{
    if (count <= 0) return false;
    bool changed = false;
    const float row_h = GetFrameHeight() + 4.0f;
    const VanVec2 origin = GetCursorScreenPos();
    const float w = GetContentRegionAvail().x;
    VanDrawList* dl = GetWindowDrawList();

    // Identify a dragged slot (if any) via per-row invisible handles.
    static int s_drag = -1;             // active drag slot (one list at a time, IM-style)
    PushID(id);

    for (int slot = 0; slot < count; ++slot)
    {
        PushID(slot);
        const VanVec2 rp(origin.x, origin.y + slot * row_h);
        SetCursorScreenPos(rp);
        (void)InvisibleButton("##row", VanVec2(w, row_h));
        if (IsItemActivated()) s_drag = slot;
        PopID();
    }

    // Resolve drag movement: if the active drag row crossed a neighbour, swap.
    if (s_drag >= 0 && IsMouseDown(0))
    {
        const float my = GetIO().MousePos.y;
        int target = (int)((my - origin.y) / row_h);
        if (target < 0) target = 0;
        if (target > count - 1) target = count - 1;
        if (target != s_drag)
        {
            const int step = (target > s_drag) ? 1 : -1;
            for (int i = s_drag; i != target; i += step)
            {
                const int tmp = order[i]; order[i] = order[i + step]; order[i + step] = tmp;
            }
            s_drag = target;
            changed = true;
        }
    }
    if (!IsMouseDown(0)) s_drag = -1;

    // Draw rows, springing each toward its slot Y (a tiny settle when reordered).
    for (int slot = 0; slot < count; ++slot)
    {
        const int item = order[slot];
        PushID(item + 10000);
        const float targetY = origin.y + slot * row_h;
        const float y = (s_drag == slot) ? GetIO().MousePos.y - row_h * 0.5f
                                         : Anim::SpringFloat(GetID("##rowY"), targetY, { .Stiffness = 220.f, .Damping = 26.f });
        const VanVec2 a(origin.x, y), b(origin.x + w, y + row_h);
        const bool dragging = (s_drag == slot);
        dl->AddRectFilled(a, b, GetColorU32(dragging ? VanGuiCol_FrameBgActive : VanGuiCol_FrameBg), 4.0f);
        dl->AddText(VanVec2(a.x + 8, a.y + (row_h - GetTextLineHeight()) * 0.5f), GetColorU32(VanGuiCol_Text), labels[item]);
        // grip dots
        const VanU32 grip = GetColorU32(VanGuiCol_TextDisabled);
        for (int g = 0; g < 3; ++g)
            dl->AddCircleFilled(VanVec2(b.x - 14, a.y + row_h * 0.5f - 5 + g * 5), 1.3f, grip);
        PopID();
    }

    // Reserve the total layout height.
    SetCursorScreenPos(origin);
    Dummy(VanVec2(w, row_h * count));
    PopID();
    return changed;
}

} // namespace VanGui

#endif // VANGUI_ENABLE_FEEDBACK
