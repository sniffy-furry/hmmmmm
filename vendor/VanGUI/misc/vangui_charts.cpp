// vangui_charts.cpp — see vangui_charts.h. Empty TU unless enabled.
#include "vangui_charts.h"

#ifdef VANGUI_ENABLE_CHARTS

#include <cmath>
#include <cstdio>

namespace VanGui {

namespace {
void Range(const float* v, int n, float& mn, float& mx)
{
    mn = 1e30f; mx = -1e30f;
    for (int i = 0; i < n; ++i) { if (v[i] < mn) mn = v[i]; if (v[i] > mx) mx = v[i]; }
    if (mn > mx) { mn = 0; mx = 1; }
    if (mx - mn < 1e-6f) { mx = mn + 1.0f; }
}
inline VanU32 Resolve(VanU32 c) { return c ? c : GetColorU32(VanGuiCol_PlotLines); }
inline void Reserve(VanVec2 size, VanVec2& mn, VanVec2& mx)
{
    mn = GetCursorScreenPos();
    if (size.x <= 0) size.x = GetContentRegionAvail().x;
    if (size.y <= 0) size.y = GetTextLineHeight() * 2.0f;
    mx = VanVec2(mn.x + size.x, mn.y + size.y);
    Dummy(size);
}
} // anonymous

void Sparkline(const char* id, const float* values, int count, VanVec2 size, VanU32 color)
{
    (void)id;
    if (count < 2) { VanVec2 a,b; Reserve(size,a,b); return; }
    VanVec2 mn, mx; Reserve(size, mn, mx);
    float lo, hi; Range(values, count, lo, hi);
    VanDrawList* dl = GetWindowDrawList();
    const float w = mx.x - mn.x, h = mx.y - mn.y;
    dl->PathClear();
    for (int i = 0; i < count; ++i) {
        const float x = mn.x + w * (float)i / (float)(count - 1);
        const float y = mx.y - h * (values[i] - lo) / (hi - lo);
        dl->PathLineTo(VanVec2(x, y));
    }
    dl->PathStroke(Resolve(color), 0, 1.5f);
}

void BarChart(const char* id, const float* values, int count, VanVec2 size, VanU32 color)
{
    (void)id;
    if (count < 1) return;
    VanVec2 mn, mx; Reserve(size, mn, mx);
    float lo, hi; Range(values, count, lo, hi);
    if (lo > 0) lo = 0;   // bars grow from zero baseline
    VanDrawList* dl = GetWindowDrawList();
    const float w = mx.x - mn.x, h = mx.y - mn.y;
    const float bw = w / (float)count;
    const VanU32 col = color ? color : GetColorU32(VanGuiCol_PlotHistogram);
    for (int i = 0; i < count; ++i) {
        const float x0 = mn.x + bw * i + 1.0f;
        const float x1 = mn.x + bw * (i + 1) - 1.0f;
        const float y  = mx.y - h * (values[i] - lo) / (hi - lo);
        dl->AddRectFilled(VanVec2(x0, y), VanVec2(x1, mx.y), col, 1.0f);
    }
}

void LineChart(const char* id, const float* values, int count, VanVec2 size,
               float v_min, float v_max, VanU32 color)
{
    (void)id;
    if (count < 2) return;
    VanVec2 mn, mx; Reserve(size, mn, mx);
    float lo = v_min, hi = v_max;
    if (hi <= lo) Range(values, count, lo, hi);
    VanDrawList* dl = GetWindowDrawList();
    dl->AddRect(mn, mx, GetColorU32(VanGuiCol_Border), 2.0f);
    const float w = mx.x - mn.x, h = mx.y - mn.y;
    dl->PathClear();
    for (int i = 0; i < count; ++i) {
        const float x = mn.x + w * (float)i / (float)(count - 1);
        float f = (values[i] - lo) / (hi - lo); if (f < 0) f = 0; if (f > 1) f = 1;
        dl->PathLineTo(VanVec2(x, mx.y - h * f));
    }
    dl->PathStroke(Resolve(color), 0, 1.5f);
}

void Gauge(const char* id, float fraction, float radius, const char* center_label, VanU32 color)
{
    (void)id;
    if (fraction < 0) fraction = 0;
    if (fraction > 1) fraction = 1;
    VanVec2 mn, mx; Reserve(VanVec2(radius * 2.0f, radius * 2.0f), mn, mx);
    VanDrawList* dl = GetWindowDrawList();
    const VanVec2 c(mn.x + radius, mn.y + radius);
    const float a0 = 2.3561945f;             // 135deg
    const float sweep = 4.712389f;           // 270deg
    const float thick = radius * 0.16f;
    dl->PathArcTo(c, radius - thick, a0, a0 + sweep, 48);
    dl->PathStroke(GetColorU32(VanGuiCol_FrameBg), 0, thick);
    dl->PathArcTo(c, radius - thick, a0, a0 + sweep * fraction, 48);
    dl->PathStroke(color ? color : GetColorU32(VanGuiCol_PlotHistogram), 0, thick);
    if (center_label) {
        const VanVec2 ts = CalcTextSize(center_label);
        dl->AddText(VanVec2(c.x - ts.x * 0.5f, c.y - ts.y * 0.5f), GetColorU32(VanGuiCol_Text), center_label);
    } else {
        char buf[16]; std::snprintf(buf, sizeof buf, "%d%%", (int)(fraction * 100 + 0.5f));
        const VanVec2 ts = CalcTextSize(buf);
        dl->AddText(VanVec2(c.x - ts.x * 0.5f, c.y - ts.y * 0.5f), GetColorU32(VanGuiCol_Text), buf);
    }
}

void HeatStrip(const char* id, const float* values, int count, VanVec2 size)
{
    (void)id;
    if (count < 1) return;
    VanVec2 mn, mx; Reserve(size, mn, mx);
    float lo, hi; Range(values, count, lo, hi);
    VanDrawList* dl = GetWindowDrawList();
    const float w = (mx.x - mn.x) / (float)count;
    for (int i = 0; i < count; ++i) {
        float f = (values[i] - lo) / (hi - lo); if (f < 0) f = 0; if (f > 1) f = 1;
        // cool (blue) -> warm (red) through green/yellow
        const float r = f;
        const float g = 1.0f - std::fabs(f - 0.5f) * 2.0f;
        const float b = 1.0f - f;
        const VanU32 col = GetColorU32(VanVec4(r, g, b, 1.0f));
        dl->AddRectFilled(VanVec2(mn.x + w * i, mn.y), VanVec2(mn.x + w * (i + 1), mx.y), col);
    }
}

} // namespace VanGui

#endif // VANGUI_ENABLE_CHARTS
