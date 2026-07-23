// vangui_widgets_ext.cpp — see vangui_widgets_ext.h. Empty TU unless enabled.
#include "vangui_widgets_ext.h"

#ifdef VANGUI_ENABLE_WIDGETS_EXT

#include "vangui_anim.h"   // Toggle thumb slide (shim snaps when anim off)
#include <cstring>
#include <cstdio>

namespace VanGui {

namespace {
inline VanU32 Accent()    { return GetColorU32(VanGuiCol_ButtonActive); }
inline VanU32 FrameCol()  { return GetColorU32(VanGuiCol_FrameBg); }
inline VanU32 TextCol()   { return GetColorU32(VanGuiCol_Text); }
} // anonymous

bool Toggle(const char* label, bool* v)
{
    const float h   = GetFrameHeight();
    const float w   = h * 1.8f;
    const float rad = h * 0.5f;
    const VanVec2 p = GetCursorScreenPos();

    PushID(label);
    const bool pressed = InvisibleButton("##tgl", VanVec2(w, h));
    PopID();
    bool changed = false;
    if (pressed) { *v = !*v; changed = true; }

    const float target = *v ? 1.0f : 0.0f;
    const float t = Anim::AnimFloat(GetID(label), target, { .Duration = 0.12f, .Easing = Anim::VanEasing_CubicOut });

    VanDrawList* dl = GetWindowDrawList();
    const VanU32 off = GetColorU32(VanGuiCol_FrameBg);
    dl->AddRectFilled(p, VanVec2(p.x + w, p.y + h), off, rad);
    // Fade the accent "on" track in over the off track as t goes 0->1.
    const VanVec4 acc = GetStyleColorVec4(VanGuiCol_ButtonActive);
    dl->AddRectFilled(p, VanVec2(p.x + w, p.y + h),
                      GetColorU32(VanVec4(acc.x, acc.y, acc.z, acc.w * t)), rad);
    const float cx = p.x + rad + t * (w - h);
    dl->AddCircleFilled(VanVec2(cx, p.y + rad), rad - 2.0f, GetColorU32(VanVec4(1,1,1,1)));

    // label to the right
    if (label[0] && label[0] != '#') {
        SameLine();
        const VanVec2 tp = GetCursorScreenPos();
        dl->AddText(VanVec2(tp.x, p.y + (h - GetTextLineHeight()) * 0.5f), TextCol(), label);
        Dummy(VanVec2(CalcTextSize(label).x, h));
    }
    return changed;
}

bool SegmentedControl(const char* id, int* current, const char* const labels[], int count)
{
    if (count <= 0) return false;
    bool changed = false;
    PushID(id);
    PushStyleVar(VanGuiStyleVar_ItemSpacing, VanVec2(0, 0));
    for (int i = 0; i < count; ++i) {
        if (i > 0) SameLine();
        const bool sel = (*current == i);
        if (sel) {
            PushStyleColor(VanGuiCol_Button, Accent());
            PushStyleColor(VanGuiCol_ButtonHovered, Accent());
        }
        PushID(i);
        if (Button(labels[i])) { if (*current != i) { *current = i; changed = true; } }
        PopID();
        if (sel) PopStyleColor(2);
    }
    PopStyleVar();
    PopID();
    return changed;
}

bool Chip(const char* label, bool* p_open)
{
    const VanVec2 pad(8.0f, 3.0f);
    const VanVec2 ts = CalcTextSize(label);
    const float   xsz = p_open ? GetTextLineHeight() : 0.0f;
    const VanVec2 sz(ts.x + pad.x * 2 + xsz, ts.y + pad.y * 2);
    const VanVec2 p  = GetCursorScreenPos();

    PushID(label);
    const bool clicked = InvisibleButton("##chip", sz);
    const bool hov = IsItemHovered();
    VanDrawList* dl = GetWindowDrawList();
    const float rad = sz.y * 0.5f;
    dl->AddRectFilled(p, VanVec2(p.x + sz.x, p.y + sz.y), GetColorU32(hov ? VanGuiCol_FrameBgHovered : VanGuiCol_FrameBg), rad);
    dl->AddText(VanVec2(p.x + pad.x, p.y + pad.y), TextCol(), label);
    bool body = clicked;
    if (p_open) {
        const VanVec2 c(p.x + sz.x - xsz * 0.5f - pad.x * 0.5f, p.y + sz.y * 0.5f);
        const float r = ts.y * 0.30f;
        const bool over_x = hov && IsMouseHoveringRect(VanVec2(c.x - r*1.6f, c.y - r*1.6f), VanVec2(c.x + r*1.6f, c.y + r*1.6f));
        const VanU32 xc = over_x ? GetColorU32(VanGuiCol_Text) : GetColorU32(VanGuiCol_TextDisabled);
        dl->AddLine(VanVec2(c.x - r, c.y - r), VanVec2(c.x + r, c.y + r), xc, 1.5f);
        dl->AddLine(VanVec2(c.x + r, c.y - r), VanVec2(c.x - r, c.y + r), xc, 1.5f);
        if (clicked && over_x) { *p_open = false; body = false; }
    }
    PopID();
    return body;
}

void Badge(const char* text, VanU32 color)
{
    const VanVec2 ts = CalcTextSize(text);
    const VanVec2 pad(6.0f, 1.0f);
    const VanVec2 sz(ts.x + pad.x * 2, ts.y + pad.y * 2);
    const VanVec2 p = GetCursorScreenPos();
    VanDrawList* dl = GetWindowDrawList();
    dl->AddRectFilled(p, VanVec2(p.x + sz.x, p.y + sz.y), color ? color : Accent(), sz.y * 0.5f);
    dl->AddText(VanVec2(p.x + pad.x, p.y + pad.y), GetColorU32(VanVec4(1,1,1,1)), text);
    Dummy(sz);
}

int Breadcrumb(const char* const items[], int count)
{
    int clicked = -1;
    PushID("##crumb");
    for (int i = 0; i < count; ++i) {
        if (i > 0) { SameLine(0, 4); TextDisabled("/"); SameLine(0, 4); }
        const bool last = (i == count - 1);
        PushID(i);
        if (last) { TextUnformatted(items[i]); }
        else {
            const VanVec4 link = GetStyleColorVec4(VanGuiCol_ButtonActive);
            PushStyleColor(VanGuiCol_Button,        VanVec4(0, 0, 0, 0));
            PushStyleColor(VanGuiCol_ButtonHovered, VanVec4(link.x, link.y, link.z, 0.15f));
            PushStyleColor(VanGuiCol_ButtonActive,  VanVec4(link.x, link.y, link.z, 0.25f));
            PushStyleColor(VanGuiCol_Text,          link);
            if (SmallButton(items[i])) clicked = i;
            PopStyleColor(4);
        }
        PopID();
    }
    PopID();
    return clicked;
}

bool Stepper(const char* label, int* v, int step, int v_min, int v_max)
{
    bool changed = false;
    PushID(label);
    const float bw = GetFrameHeight();
    if (Button("-", VanVec2(bw, bw))) { if (*v - step >= v_min) { *v -= step; changed = true; } }
    SameLine(0, 2);
    char buf[32]; std::snprintf(buf, sizeof buf, "%d", *v);
    const float tw = CalcTextSize(buf).x + GetStyle().FramePadding.x * 2 + 16.0f;
    SetNextItemWidth(tw);
    if (DragInt("##val", v, (float)step * 0.1f, v_min, v_max)) changed = true;
    SameLine(0, 2);
    if (Button("+", VanVec2(bw, bw))) { if (*v + step <= v_max) { *v += step; changed = true; } }
    if (label[0] && label[0] != '#') { SameLine(); TextUnformatted(label); }
    PopID();
    return changed;
}

bool StarRating(const char* id, int* rating, int max_stars)
{
    bool changed = false;
    const float s = GetTextLineHeight() + 2.0f;
    VanVec2 p = GetCursorScreenPos();
    VanDrawList* dl = GetWindowDrawList();
    PushID(id);
    for (int i = 0; i < max_stars; ++i) {
        PushID(i);
        const VanVec2 cell = GetCursorScreenPos();
        const bool clk = InvisibleButton("##star", VanVec2(s, s));
        const bool hov = IsItemHovered();
        const bool filled = (i < *rating) || (hov && i <= *rating /*hover preview handled below*/);
        (void)filled;
        const bool on = (i < *rating);
        const VanU32 col = on ? GetColorU32(VanGuiCol_ButtonActive)
                              : (hov ? GetColorU32(VanGuiCol_Text) : GetColorU32(VanGuiCol_TextDisabled));
        // simple 5-point star via filled circle proxy (kept light)
        dl->AddText(VanVec2(cell.x, cell.y), col, on ? "\xE2\x98\x85" : "\xE2\x98\x86"); // ★ / ☆
        if (clk) { *rating = i + 1; changed = true; }
        PopID();
        if (i < max_stars - 1) SameLine(0, 1);
    }
    (void)p;
    PopID();
    return changed;
}

bool SearchBox(const char* id, char* buf, size_t buf_size, const char* hint, float width)
{
    PushID(id);
    if (width > 0) SetNextItemWidth(width);
    else           SetNextItemWidth(-1.0f);
    const bool changed = InputTextWithHint("##search", hint, buf, buf_size);
    // clear 'x' overlay
    if (buf[0]) {
        const VanVec2 mn = GetItemRectMin();
        const VanVec2 mx = GetItemRectMax();
        const float r = (mx.y - mn.y) * 0.18f;
        const VanVec2 c(mx.x - r * 3.0f, (mn.y + mx.y) * 0.5f);
        VanDrawList* dl = GetWindowDrawList();
        const bool over = IsMouseHoveringRect(VanVec2(c.x - r*2, mn.y), VanVec2(mx.x, mx.y));
        const VanU32 xc = over ? GetColorU32(VanGuiCol_Text) : GetColorU32(VanGuiCol_TextDisabled);
        dl->AddLine(VanVec2(c.x - r, c.y - r), VanVec2(c.x + r, c.y + r), xc, 1.5f);
        dl->AddLine(VanVec2(c.x + r, c.y - r), VanVec2(c.x - r, c.y + r), xc, 1.5f);
        if (over && IsMouseClicked(0)) { buf[0] = '\0'; PopID(); return true; }
    }
    PopID();
    return changed;
}

} // namespace VanGui

#endif // VANGUI_ENABLE_WIDGETS_EXT
