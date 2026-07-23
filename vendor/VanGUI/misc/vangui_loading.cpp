// vangui_loading.cpp
// -----------------------------------------------------------------------------
// VanGUI Enhancement Suite — Pillar 2 implementation. See vangui_loading.h.
//
// Entire TU is empty unless VANGUI_ENABLE_LOADING is defined, so it is safe to
// add to the build unconditionally (opt-in, zero-cost).
// -----------------------------------------------------------------------------

#include "vangui_loading.h"

#ifdef VANGUI_ENABLE_LOADING

#include "vangui_anim.h"
#include <math.h>     // sinf, cosf, fmodf

namespace VanGui {

namespace {

constexpr float k_TwoPi = 6.28318530718f;

inline VanU32 ResolveColor(VanU32 color)
{
    return color ? color : GetColorU32(VanGuiCol_Text);
}

// Reserve `size` of layout space at the current cursor and return the screen-space
// rect that was reserved. Mirrors how core widgets lay themselves out.
inline void ReserveRect(VanVec2 size, VanVec2& out_min, VanVec2& out_max)
{
    out_min = GetCursorScreenPos();
    out_max = VanVec2(out_min.x + size.x, out_min.y + size.y);
    Dummy(size);
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// Spinners — stateless: animation phase derives from global time.
// ---------------------------------------------------------------------------

void Spinner(const char* id, float radius, float thickness, VanU32 color, float speed)
{
    (void)id;
    const VanU32 col = ResolveColor(color);
    VanVec2 mn, mx;
    ReserveRect(VanVec2(radius * 2.0f, radius * 2.0f), mn, mx);

    VanDrawList* dl = GetWindowDrawList();
    const VanVec2 c(mn.x + radius, mn.y + radius);
    const float   t   = (float)GetTime() * speed;
    const int     seg = 30;
    const float   start = fmodf(t * 2.0f, k_TwoPi);
    const float   arc   = k_TwoPi * 0.75f;   // 270-degree sweep

    dl->PathClear();
    for (int i = 0; i <= seg; ++i)
    {
        const float a = start + (arc * (float)i / (float)seg);
        dl->PathLineTo(VanVec2(c.x + cosf(a) * radius, c.y + sinf(a) * radius));
    }
    dl->PathStroke(col, 0, thickness);
}

void SpinnerDots(const char* id, float radius, int count, VanU32 color)
{
    (void)id;
    if (count < 1) count = 1;
    const VanU32 base = ResolveColor(color);
    VanVec2 mn, mx;
    ReserveRect(VanVec2(radius * 2.0f, radius * 2.0f), mn, mx);

    VanDrawList* dl = GetWindowDrawList();
    const VanVec2 c(mn.x + radius, mn.y + radius);
    const float   t        = (float)GetTime();
    const float   dot_r    = radius * 0.18f;
    const float   orbit_r  = radius - dot_r;
    const float   head     = fmodf(t * 1.4f, 1.0f);   // 0..1 trailing brightness

    for (int i = 0; i < count; ++i)
    {
        const float frac = (float)i / (float)count;
        const float a    = frac * k_TwoPi - k_TwoPi * 0.25f;
        // Brightness trails the rotating head.
        float d = head - frac;
        if (d < 0.0f) d += 1.0f;
        const float alpha = 0.25f + 0.75f * (1.0f - d);
        const VanU32 col = GetColorU32(base, alpha);
        dl->AddCircleFilled(VanVec2(c.x + cosf(a) * orbit_r, c.y + sinf(a) * orbit_r), dot_r, col);
    }
}

void SpinnerBars(const char* id, VanVec2 size, VanU32 color)
{
    (void)id;
    if (size.x <= 0.0f) size.x = GetTextLineHeight() * 2.0f;
    if (size.y <= 0.0f) size.y = GetTextLineHeight();
    const VanU32 base = ResolveColor(color);
    VanVec2 mn, mx;
    ReserveRect(size, mn, mx);

    VanDrawList* dl = GetWindowDrawList();
    const int   bars  = 5;
    const float gap   = size.x * 0.06f;
    const float bw    = (size.x - gap * (bars - 1)) / bars;
    const float t     = (float)GetTime();
    for (int i = 0; i < bars; ++i)
    {
        const float phase = t * 4.0f - (float)i * 0.5f;
        const float s     = 0.4f + 0.6f * (0.5f + 0.5f * sinf(phase));
        const float h     = size.y * s;
        const float x0    = mn.x + i * (bw + gap);
        const float y0    = mn.y + (size.y - h) * 0.5f;
        dl->AddRectFilled(VanVec2(x0, y0), VanVec2(x0 + bw, y0 + h), base, bw * 0.3f);
    }
}

void IndeterminateBar(const char* id, VanVec2 size, VanU32 color)
{
    (void)id;
    if (size.x <= 0.0f) size.x = GetContentRegionAvail().x;
    if (size.y <= 0.0f) size.y = 4.0f;
    const VanU32 accent = ResolveColor(color);
    VanVec2 mn, mx;
    ReserveRect(size, mn, mx);

    VanDrawList* dl = GetWindowDrawList();
    dl->AddRectFilled(mn, mx, GetColorU32(VanGuiCol_FrameBg), size.y * 0.5f);

    // A sweeping segment, ~35% of the track width, bouncing left to right.
    const float t      = fmodf((float)GetTime() * 0.8f, 1.0f);
    const float seg_w  = size.x * 0.35f;
    const float travel = size.x + seg_w;
    const float head   = -seg_w + travel * t;
    const float x0     = mn.x + (head < 0.0f ? 0.0f : head);
    const float x1     = mn.x + (head + seg_w > size.x ? size.x : head + seg_w);
    if (x1 > x0)
    {
        const VanU32 edge = GetColorU32(accent, 0.0f);
        dl->AddRectFilledMultiColor(VanVec2(x0, mn.y), VanVec2(x1, mx.y), edge, accent, accent, edge);
    }
}

// ---------------------------------------------------------------------------
// Determinate circular progress — smoothed through the substrate.
// ---------------------------------------------------------------------------

void ProgressRing(const char* id, float fraction, float radius, float thickness, VanU32 color)
{
    if (fraction < 0.0f) fraction = 0.0f;
    if (fraction > 1.0f) fraction = 1.0f;
    const VanU32 accent = ResolveColor(color);
    VanVec2 mn, mx;
    ReserveRect(VanVec2(radius * 2.0f, radius * 2.0f), mn, mx);

    // Smooth the displayed fill so jumps in `fraction` animate instead of snapping.
    const float shown = Anim::AnimFloat(GetID(id), fraction, { .Duration = 0.25f, .Easing = Anim::VanEasing_CubicOut });

    VanDrawList* dl = GetWindowDrawList();
    const VanVec2 c(mn.x + radius, mn.y + radius);
    const float a0 = -k_TwoPi * 0.25f;             // 12 o'clock
    const float a1 = a0 + k_TwoPi * shown;

    dl->PathArcTo(c, radius - thickness * 0.5f, a0, a0 + k_TwoPi, 48);
    dl->PathStroke(GetColorU32(VanGuiCol_FrameBg), 0, thickness);
    if (shown > 0.0001f)
    {
        dl->PathArcTo(c, radius - thickness * 0.5f, a0, a1, 48);
        dl->PathStroke(accent, 0, thickness);
    }
}

// ---------------------------------------------------------------------------
// Skeletons — stateless shimmer from global time.
// ---------------------------------------------------------------------------

namespace {
void DrawSkeleton(VanDrawList* dl, VanVec2 mn, VanVec2 mx, float rounding)
{
    const VanU32 base = GetColorU32(VanGuiCol_FrameBg);
    dl->AddRectFilled(mn, mx, base, rounding);

    // Moving highlight band.
    const float w     = mx.x - mn.x;
    const float t     = fmodf((float)GetTime() * 0.9f, 1.0f);
    const float band  = w * 0.3f;
    const float hx    = mn.x - band + (w + band) * t;
    const float x0    = hx < mn.x ? mn.x : hx;
    const float x1    = hx + band > mx.x ? mx.x : hx + band;
    if (x1 > x0)
    {
        const VanU32 hl   = GetColorU32(VanGuiCol_Text, 0.10f);
        const VanU32 edge = GetColorU32(VanGuiCol_Text, 0.0f);
        dl->AddRectFilledMultiColor(VanVec2(x0, mn.y), VanVec2(x1, mx.y), edge, hl, hl, edge);
    }
}
} // anonymous namespace

void Skeleton(VanVec2 size, float rounding)
{
    if (size.x <= 0.0f) size.x = GetContentRegionAvail().x;
    if (size.y <= 0.0f) size.y = GetTextLineHeight();
    VanVec2 mn, mx;
    ReserveRect(size, mn, mx);
    DrawSkeleton(GetWindowDrawList(), mn, mx, rounding);
}

void SkeletonText(int lines, float line_height)
{
    if (lines < 1) lines = 1;
    if (line_height <= 0.0f) line_height = GetTextLineHeight();
    const float spacing = GetStyle().ItemSpacing.y;
    const float avail   = GetContentRegionAvail().x;
    for (int i = 0; i < lines; ++i)
    {
        // Last line is shorter, like real wrapped text.
        const float w = (i == lines - 1) ? avail * 0.6f : avail;
        Skeleton(VanVec2(w, line_height), line_height * 0.35f);
        if (i != lines - 1)
            Dummy(VanVec2(0.0f, spacing));
    }
}

// ---------------------------------------------------------------------------
// Loading overlay — dim-and-cover the current window while busy.
// ---------------------------------------------------------------------------

bool BeginLoadingOverlay(const char* id, bool busy)
{
    const float a = Anim::AnimBool(GetID(id), busy, { .Duration = 0.18f, .Easing = Anim::VanEasing_QuadOut });
    if (a <= 0.001f)
        return false;

    VanDrawList* dl = GetWindowDrawList();
    const VanVec2 wp = GetWindowPos();
    const VanVec2 ws = GetWindowSize();
    dl->AddRectFilled(wp, VanVec2(wp.x + ws.x, wp.y + ws.y), GetColorU32(VanGuiCol_WindowBg, a * 0.6f));

    // Centered spinner driven by the same draw list.
    const float r = 16.0f;
    const VanVec2 c(wp.x + ws.x * 0.5f, wp.y + ws.y * 0.5f);
    const float t   = (float)GetTime() * 1.0f;
    const float st  = fmodf(t * 2.0f, k_TwoPi);
    const int   seg = 30;
    dl->PathClear();
    for (int i = 0; i <= seg; ++i)
    {
        const float ang = st + (k_TwoPi * 0.75f * (float)i / (float)seg);
        dl->PathLineTo(VanVec2(c.x + cosf(ang) * r, c.y + sinf(ang) * r));
    }
    dl->PathStroke(GetColorU32(VanGuiCol_Text, a), 0, 3.0f);
    return true;
}

void EndLoadingOverlay()
{
    // Nothing to pop — overlay draws directly to the window draw list. Present
    // for API symmetry and the RAII guard.
}

} // namespace VanGui

#endif // VANGUI_ENABLE_LOADING
