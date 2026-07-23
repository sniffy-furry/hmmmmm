// vangui_anim.cpp
// -----------------------------------------------------------------------------
// VanGUI Enhancement Suite — Pillar 1 implementation.
// See vangui_anim.h for the design contract.
//
// The ENTIRE translation unit is empty unless VANGUI_ENABLE_ANIM is defined, so
// it is safe to add this file to the build unconditionally — it costs nothing
// when the feature is off. (Engineering Constitution: opt-in, zero-cost.)
// -----------------------------------------------------------------------------

#include "vangui_anim.h"

#ifdef VANGUI_ENABLE_ANIM

#include <cmath>     // powf, sinf, sqrtf, fabsf
#include <cstring>   // memset

namespace VanGui {
namespace Anim {

// ---------------------------------------------------------------------------
// Easing — compile-time validation of the polynomial core
// ---------------------------------------------------------------------------
static_assert(EaseLinear(0.0f)  == 0.0f && EaseLinear(1.0f)  == 1.0f, "Linear must map endpoints");
static_assert(EaseQuadIn(0.0f)  == 0.0f && EaseQuadIn(1.0f)  == 1.0f, "QuadIn must map endpoints");
static_assert(EaseQuadOut(0.0f) == 0.0f && EaseQuadOut(1.0f) == 1.0f, "QuadOut must map endpoints");
static_assert(EaseCubicIn(0.0f) == 0.0f && EaseCubicIn(1.0f) == 1.0f, "CubicIn must map endpoints");
static_assert(EaseCubicOut(0.0f)== 0.0f && EaseCubicOut(1.0f)== 1.0f, "CubicOut must map endpoints");

float Ease(VanEasing fn, float t) noexcept
{
    if (t <= 0.0f) return 0.0f;
    // Clamp the "in/out" ends except for curves that deliberately overshoot.
    const bool overshoots = (fn == VanEasing_BackOut || fn == VanEasing_ElasticOut);
    if (t >= 1.0f && !overshoots) return 1.0f;

    switch (fn)
    {
    case VanEasing_Linear:     return t;
    case VanEasing_QuadIn:     return EaseQuadIn(t);
    case VanEasing_QuadOut:    return EaseQuadOut(t);
    case VanEasing_QuadInOut:  return (t < 0.5f) ? (2.0f * t * t)
                                                 : (1.0f - powf(-2.0f * t + 2.0f, 2.0f) * 0.5f);
    case VanEasing_CubicIn:    return EaseCubicIn(t);
    case VanEasing_CubicOut:   return EaseCubicOut(t);
    case VanEasing_CubicInOut: return (t < 0.5f) ? (4.0f * t * t * t)
                                                 : (1.0f - powf(-2.0f * t + 2.0f, 3.0f) * 0.5f);
    case VanEasing_ExpoOut:    return 1.0f - powf(2.0f, -10.0f * t);
    case VanEasing_CircOut:    { const float u = t - 1.0f; return sqrtf(1.0f - u * u); }
    case VanEasing_BackOut:    { const float c1 = 1.70158f, c3 = c1 + 1.0f; const float u = t - 1.0f;
                                 return 1.0f + c3 * u * u * u + c1 * u * u; }
    case VanEasing_ElasticOut: { if (t >= 1.0f) return 1.0f;
                                 const float c4 = (2.0f * 3.14159265358979f) / 3.0f;
                                 return powf(2.0f, -10.0f * t) * sinf((t * 10.0f - 0.75f) * c4) + 1.0f; }
    case VanEasing_BounceOut:  { const float n1 = 7.5625f, d1 = 2.75f; float u = t;
                                 if (u < 1.0f / d1)        { return n1 * u * u; }
                                 else if (u < 2.0f / d1)   { u -= 1.5f  / d1; return n1 * u * u + 0.75f; }
                                 else if (u < 2.5f / d1)   { u -= 2.25f / d1; return n1 * u * u + 0.9375f; }
                                 else                      { u -= 2.625f/ d1; return n1 * u * u + 0.984375f; } }
    default:                   return t;
    }
}

// ---------------------------------------------------------------------------
// State pool
// ---------------------------------------------------------------------------
// One slot per animated id. POD only (VanVector copies with memcpy). Stores
// solely the *dynamic* state; all tunables (easing, duration, stiffness…) are
// supplied by the call site every frame, so they are never stored here.

struct VanAnimSlot
{
    VanGuiID Key;          // owning id (needed to clear the map on eviction)
    int      Channels;     // 0 = free slot, 1 = scalar, 4 = color
    int      LastFrame;    // last frame this slot was advanced
    float    Delay;        // remaining delay (seconds)
    float    Elapsed;      // time into the current tween segment (seconds)
    float    Start[4];     // value at segment start (tween)
    float    Cur[4];       // current value (what callers read back)
    float    Vel;          // velocity (spring; scalar only)
    float    Target[4];    // last observed target
};

struct VanAnimContext
{
    VanGuiContext*         Owner = nullptr;   // VanGui context these animations belong to
    VanGuiStorage          IdToSlot;          // VanGuiID -> (slot index + 1); 0 means none
    VanVector<VanAnimSlot> Pool;
    VanVector<int>         FreeList;
    int   FrameCount        = 0;
    int   EvictFrames       = 60;
    float DtOverride        = -1.0f;          // < 0 => use io.DeltaTime
    bool  AnyActiveThisFrame = false;
    bool  NeedsRefresh       = false;
};

// Single persistent state object. This is the one piece of file-scope state in
// the substrate; it is justified exactly as VanGui's own context is — immediate
// mode requires frame-to-frame state to live somewhere, and the public API is
// keyed by id. It is rebound to the active VanGui context in EnsureCtx().
static VanAnimContext g_Anim;

// --- small helpers ---------------------------------------------------------

static inline float Clamp01(float v) noexcept { return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v); }
static inline void  MarkActive() noexcept     { g_Anim.AnyActiveThisFrame = true; }

static void EnsureCtx()
{
    VanGuiContext* cur = VanGui::GetCurrentContext();
    if (cur == g_Anim.Owner)
        return;
    // Context changed (created/destroyed/switched): drop all state.
    g_Anim.Pool.clear();
    g_Anim.FreeList.clear();
    g_Anim.IdToSlot.Clear();
    g_Anim.Owner             = cur;
    g_Anim.FrameCount        = 0;
    g_Anim.AnyActiveThisFrame = false;
    g_Anim.NeedsRefresh      = false;
}

static float FrameDt()
{
    if (g_Anim.DtOverride >= 0.0f)
        return g_Anim.DtOverride;
    float dt = VanGui::GetIO().DeltaTime;
    return (dt < 0.0f) ? 0.0f : dt;
}

static VanAnimSlot* FindSlot(VanGuiID id)
{
    const int idx = g_Anim.IdToSlot.GetInt(id, 0);
    return idx ? &g_Anim.Pool[idx - 1] : nullptr;
}

// Allocate (or recycle) a slot and seed it with `init` values. Allocation only
// happens the first time an id is seen — steady-state animation never allocates.
static VanAnimSlot* NewSlot(VanGuiID id, int channels, const float* init)
{
    int slot;
    if (g_Anim.FreeList.size() > 0)
    {
        slot = g_Anim.FreeList.back();
        g_Anim.FreeList.pop_back();
    }
    else
    {
        VanAnimSlot blank;
        memset(&blank, 0, sizeof(blank));
        g_Anim.Pool.push_back(blank);
        slot = g_Anim.Pool.size() - 1;
    }
    VanAnimSlot& s = g_Anim.Pool[slot];
    memset(&s, 0, sizeof(s));
    s.Key       = id;
    s.Channels  = channels;
    s.LastFrame = g_Anim.FrameCount;
    for (int c = 0; c < channels; ++c)
        s.Start[c] = s.Cur[c] = s.Target[c] = init[c];
    g_Anim.IdToSlot.SetInt(id, slot + 1);
    return &s;
}

// Re-seed an existing slot when an id is reused with a different channel count.
static void ReseedSlot(VanAnimSlot& s, int channels, const float* init)
{
    s.Channels  = channels;
    s.Delay     = 0.0f;
    s.Elapsed   = 0.0f;
    s.LastFrame = g_Anim.FrameCount;
    s.Vel = 0.0f;
    for (int c = 0; c < 4; ++c)
    {
        const float v = (c < channels) ? init[c] : 0.0f;
        s.Start[c] = s.Cur[c] = s.Target[c] = v;
    }
}

// Advance a duration+easing tween by one frame (idempotent within a frame).
static void AdvanceTween(VanAnimSlot& s, const float* target, const VanAnimParams& p)
{
    if (s.LastFrame == g_Anim.FrameCount)   // already advanced this frame
        return;
    s.LastFrame = g_Anim.FrameCount;
    float dt = FrameDt();

    // New target -> begin a fresh segment from the current value.
    bool changed = false;
    for (int c = 0; c < s.Channels; ++c)
        if (target[c] != s.Target[c]) { changed = true; break; }
    if (changed)
    {
        for (int c = 0; c < s.Channels; ++c) { s.Start[c] = s.Cur[c]; s.Target[c] = target[c]; }
        s.Elapsed = 0.0f;
        s.Delay   = p.Delay;
    }

    if (s.Delay > 0.0f)
    {
        s.Delay -= dt;
        if (s.Delay > 0.0f) { MarkActive(); return; }
        dt = -s.Delay;       // spend the leftover into this frame's motion
        s.Delay = 0.0f;
    }

    const float dur = (p.Duration > 0.0f) ? p.Duration : 0.0001f;
    s.Elapsed += dt;
    float t = s.Elapsed / dur;
    const bool done = (t >= 1.0f);
    if (done) t = 1.0f;

    const float k = Ease(p.Easing, t);
    for (int c = 0; c < s.Channels; ++c)
        s.Cur[c] = s.Start[c] + (s.Target[c] - s.Start[c]) * k;

    if (done)
        for (int c = 0; c < s.Channels; ++c) s.Cur[c] = s.Target[c];
    else
        MarkActive();
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

float AnimFloat(VanGuiID id, float target, const VanAnimParams& p)
{
    EnsureCtx();
    VanAnimSlot* s = FindSlot(id);
    if (!s)                    { s = NewSlot(id, 1, &target); return s->Cur[0]; }
    if (s->Channels != 1)      { ReseedSlot(*s, 1, &target); return s->Cur[0]; }
    AdvanceTween(*s, &target, p);
    return s->Cur[0];
}

VanVec4 AnimColor(VanGuiID id, VanVec4 target, const VanAnimParams& p)
{
    EnsureCtx();
    const float tg[4] = { target.x, target.y, target.z, target.w };
    VanAnimSlot* s = FindSlot(id);
    if (!s)               { NewSlot(id, 4, tg);    return target; }
    if (s->Channels != 4) { ReseedSlot(*s, 4, tg); return target; }
    AdvanceTween(*s, tg, p);
    return VanVec4(Clamp01(s->Cur[0]), Clamp01(s->Cur[1]), Clamp01(s->Cur[2]), Clamp01(s->Cur[3]));
}

float AnimBool(VanGuiID id, bool open, const VanAnimParams& p)
{
    const float target = open ? 1.0f : 0.0f;
    EnsureCtx();
    VanAnimSlot* s = FindSlot(id);
    if (!s)               { NewSlot(id, 1, &target);    return target; }
    if (s->Channels != 1) { ReseedSlot(*s, 1, &target); return target; }
    AdvanceTween(*s, &target, p);
    return Clamp01(s->Cur[0]);
}

float SpringFloat(VanGuiID id, float target, const VanSpringParams& p)
{
    EnsureCtx();
    VanAnimSlot* s = FindSlot(id);
    if (!s)               { NewSlot(id, 1, &target);    return target; }
    if (s->Channels != 1) { ReseedSlot(*s, 1, &target); return target; }
    if (s->LastFrame == g_Anim.FrameCount)
        return s->Cur[0];
    s->LastFrame = g_Anim.FrameCount;
    s->Target[0] = target;

    float dt = FrameDt();
    if (dt > 0.05f) dt = 0.05f;   // guard against blow-up after a long stall

    // Semi-implicit (symplectic) Euler — stable and framerate independent.
    float x = s->Cur[0];
    float v = s->Vel;
    const float force = -p.Stiffness * (x - target) - p.Damping * v;
    v += force * dt;
    x += v * dt;
    s->Vel = v;
    s->Cur[0] = x;

    if (fabsf(x - target) < p.Eps && fabsf(v) < p.Eps)
    {
        s->Cur[0] = target;
        s->Vel = 0.0f;
    }
    else
    {
        MarkActive();
    }
    return s->Cur[0];
}

void NewFrameUpdate()
{
    EnsureCtx();
    // Publish the motion seen during the frame that just ended, then reset.
    g_Anim.NeedsRefresh       = g_Anim.AnyActiveThisFrame;
    g_Anim.AnyActiveThisFrame = false;
    g_Anim.FrameCount++;

    // Evict slots untouched for EvictFrames frames (no allocation, O(pool)).
    const int cutoff = g_Anim.FrameCount - g_Anim.EvictFrames;
    for (int i = 0; i < g_Anim.Pool.size(); ++i)
    {
        VanAnimSlot& s = g_Anim.Pool[i];
        if (s.Channels == 0)            // already free
            continue;
        if (s.LastFrame < cutoff)
        {
            g_Anim.IdToSlot.SetInt(s.Key, 0);
            s.Channels = 0;
            g_Anim.FreeList.push_back(i);
        }
    }
}

bool IsAnimating()
{
    return g_Anim.AnyActiveThisFrame || g_Anim.NeedsRefresh;
}

void Reset(VanGuiID id)
{
    EnsureCtx();
    const int idx = g_Anim.IdToSlot.GetInt(id, 0);
    if (!idx)
        return;
    g_Anim.Pool[idx - 1].Channels = 0;
    g_Anim.IdToSlot.SetInt(id, 0);
    g_Anim.FreeList.push_back(idx - 1);
}

void Shutdown()
{
    g_Anim.Pool.clear();
    g_Anim.FreeList.clear();
    g_Anim.IdToSlot.Clear();
    g_Anim.Owner              = nullptr;
    g_Anim.FrameCount         = 0;
    g_Anim.AnyActiveThisFrame = false;
    g_Anim.NeedsRefresh       = false;
}

void SetEvictionFrames(int frames)
{
    g_Anim.EvictFrames = (frames > 1) ? frames : 1;
}

void SetDeltaTimeOverrideForTesting(float dt)
{
    g_Anim.DtOverride = dt;
}

int PoolActiveCount()
{
    EnsureCtx();
    int n = 0;
    for (int i = 0; i < g_Anim.Pool.size(); ++i)
        if (g_Anim.Pool[i].Channels != 0) ++n;
    return n;
}

int PoolCapacity()
{
    EnsureCtx();
    return g_Anim.Pool.size();
}

} // namespace Anim
} // namespace VanGui

#endif // VANGUI_ENABLE_ANIM
