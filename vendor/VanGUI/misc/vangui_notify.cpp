//-----------------------------------------------------------------------------
// vangui_notify.cpp  --  Toast / notification overlay for VanGUI
//-----------------------------------------------------------------------------

#include "vangui_notify.h"
#include "vangui_anim.h"      // Pillar 1 substrate (fade/slide)
#include "vangui_loading.h"   // Detail::ProgressBarPrim (shared progress draw, DRY)

#include <stdarg.h>   // va_list, va_start, va_end, vsnprintf
#include <string.h>   // memset, memmove

//-----------------------------------------------------------------------------
// Tuneable constants
//-----------------------------------------------------------------------------

static const float  NOTIFY_FADE_IN_MS       = 150.0f;   // fade-in duration (ms)
static const float  NOTIFY_FADE_OUT_MS      = 300.0f;   // fade-out duration (ms)
static const float  NOTIFY_DEFAULT_MS       = 3000.0f;  // default lifetime (ms)
static const float  NOTIFY_PADDING_X        = 20.0f;    // edge padding (px)
static const float  NOTIFY_PADDING_Y        = 20.0f;    // edge padding (px)
static const float  NOTIFY_PADDING_BETWEEN  = 6.0f;     // gap between toasts (px)
static const float  NOTIFY_BORDER_WIDTH     = 4.0f;     // left accent stripe width (px)
static const float  NOTIFY_MAX_BG_ALPHA     = 0.88f;    // peak window opacity
static const float  NOTIFY_MIN_WIDTH        = 260.0f;   // minimum toast width (px)
static const float  NOTIFY_PROGRESS_HEIGHT  = 4.0f;     // progress bar thickness (px)
static const int    NOTIFY_HARD_MAX         = 64;       // absolute ceiling on ring size

//-----------------------------------------------------------------------------
// Internal notification record
//-----------------------------------------------------------------------------

namespace
{

struct VanNotification
{
    VanGui::VanNotifyID   ID;
    VanGui::VanNotifyType Type;
    char                  Msg[256];
    float                 CreationTime;   // VanGui::GetTime() at insertion (seconds)
    float                 DurationMs;
    float                 Progress;       // <0 = hidden, 0..1 = progress bar
    bool                  Dismissed;      // set by DismissNotification()
    bool                  Seeded;         // anim slot primed (drives fade-in)

    // Opacity 0..1 for the current frame, driven by the shared animation
    // substrate (vangui_anim) instead of a bespoke per-toast fade timer.
    // The toast is "open" during its hold window and flips closed when it enters
    // the fade-out tail or is dismissed; AnimBool turns that into a smooth fade.
    // When VANGUI_ENABLE_ANIM is off, AnimBool snaps (toasts pop in/out).
    float ComputeFade(float t)
    {
        const VanGuiID aid = (VanGuiID)(0x4E4F5400u ^ (unsigned)ID); // "NOT\0" ^ id
        VanGui::Anim::VanAnimParams ap;
        ap.Duration = NOTIFY_FADE_OUT_MS / 1000.0f;
        ap.Easing   = VanGui::Anim::VanEasing_QuadOut;
        if (!Seeded)
        {
            Seeded = true;
            return VanGui::Anim::AnimBool(aid, false, ap); // prime at 0 -> fades in next frame
        }
        const float elapsed  = (t - CreationTime) * 1000.0f;
        const float hold_end = DurationMs - NOTIFY_FADE_OUT_MS;
        const bool  open     = !Dismissed && elapsed < hold_end;
        return VanGui::Anim::AnimBool(aid, open, ap);
    }

    bool IsExpired(float t) const
    {
        return Dismissed || ((t - CreationTime) * 1000.0f >= DurationMs);
    }
};

//-----------------------------------------------------------------------------
// File-scope context
//-----------------------------------------------------------------------------

static const int NOTIFY_DEFAULT_MAX = 8;

struct VanNotifyContext
{
    VanNotification     Ring[NOTIFY_HARD_MAX];
    int                 Count        = 0;
    int                 MaxCount     = NOTIFY_DEFAULT_MAX;
    int                 NextID       = 1;           // starts at 1; 0 = invalid
    VanGui::VanNotifyPos Pos         = VanGui::VanNotifyPos_BottomRight;

    VanGui::VanNotifyID Insert(const VanNotification& n)
    {
        if (Count < MaxCount)
        {
            Ring[Count++] = n;
        }
        else
        {
            // Displace oldest: shift left, overwrite last slot.
            for (int i = 0; i < MaxCount - 1; ++i)
                Ring[i] = Ring[i + 1];
            Ring[MaxCount - 1] = n;
        }
        return n.ID;
    }

    VanNotification* FindByID(VanGui::VanNotifyID id)
    {
        if (id == VanGui::VanNotifyID_Invalid) return nullptr;
        for (int i = 0; i < Count; ++i)
            if (Ring[i].ID == id) return &Ring[i];
        return nullptr;
    }

    void RemoveExpired(float t)
    {
        int w = 0;
        for (int i = 0; i < Count; ++i)
            if (!Ring[i].IsExpired(t))
                Ring[w++] = Ring[i];
        Count = w;
    }
};

static VanNotifyContext g_Ctx;

//-----------------------------------------------------------------------------
// Helpers
//-----------------------------------------------------------------------------

static VanU32 TypeColor(VanGui::VanNotifyType type, float alpha)
{
    if (alpha < 0.0f) alpha = 0.0f;
    if (alpha > 1.0f) alpha = 1.0f;
    const VanU32 a = static_cast<VanU32>(alpha * 255.0f + 0.5f);
    switch (type)
    {
        case VanGui::VanNotifyType_Info:    return VAN_COL32( 66, 133, 244, a);
        case VanGui::VanNotifyType_Success: return VAN_COL32( 52, 168,  83, a);
        case VanGui::VanNotifyType_Warning: return VAN_COL32(251, 188,   4, a);
        case VanGui::VanNotifyType_Error:   return VAN_COL32(234,  67,  53, a);
        default:                            return VAN_COL32(180, 180, 180, a);
    }
}

static const char* TypePrefix(VanGui::VanNotifyType type)
{
    switch (type)
    {
        case VanGui::VanNotifyType_Info:    return "[INFO] ";
        case VanGui::VanNotifyType_Success: return "[OK]   ";
        case VanGui::VanNotifyType_Warning: return "[WARN] ";
        case VanGui::VanNotifyType_Error:   return "[ERROR]";
        default:                            return "";
    }
}

// Build "##notify_NNN" into buf (buf must be >= 16 bytes).
static void MakeWindowID(char* buf, int idx)
{
    const char* prefix = "##notify_";
    int w = 0;
    for (; prefix[w]; ++w) buf[w] = prefix[w];
    // Append up to 3-digit decimal index.
    if (idx >= 100) buf[w++] = static_cast<char>('0' + idx / 100);
    if (idx >=  10) buf[w++] = static_cast<char>('0' + (idx / 10) % 10);
    buf[w++] = static_cast<char>('0' + idx % 10);
    buf[w]   = '\0';
}

// Compute anchor position and pivot for the chosen corner.
static void AnchorForPos(VanGui::VanNotifyPos pos,
                         const VanGuiViewport* vp,
                         VanVec2& out_anchor, VanVec2& out_pivot)
{
    const float wx = vp->WorkPos.x;
    const float wy = vp->WorkPos.y;
    const float ww = vp->WorkSize.x;
    const float wh = vp->WorkSize.y;

    switch (pos)
    {
        case VanGui::VanNotifyPos_TopLeft:
            out_anchor = VanVec2(wx + NOTIFY_PADDING_X,        wy + NOTIFY_PADDING_Y);
            out_pivot  = VanVec2(0.0f, 0.0f);
            break;
        case VanGui::VanNotifyPos_TopRight:
            out_anchor = VanVec2(wx + ww - NOTIFY_PADDING_X,   wy + NOTIFY_PADDING_Y);
            out_pivot  = VanVec2(1.0f, 0.0f);
            break;
        case VanGui::VanNotifyPos_BottomLeft:
            out_anchor = VanVec2(wx + NOTIFY_PADDING_X,        wy + wh - NOTIFY_PADDING_Y);
            out_pivot  = VanVec2(0.0f, 1.0f);
            break;
        case VanGui::VanNotifyPos_BottomRight:
        default:
            out_anchor = VanVec2(wx + ww - NOTIFY_PADDING_X,   wy + wh - NOTIFY_PADDING_Y);
            out_pivot  = VanVec2(1.0f, 1.0f);
            break;
    }
}

// Returns true if the corner is at the bottom (toasts stack upward).
static bool IsBottomPos(VanGui::VanNotifyPos pos)
{
    return pos == VanGui::VanNotifyPos_BottomRight ||
           pos == VanGui::VanNotifyPos_BottomLeft;
}

} // anonymous namespace

//-----------------------------------------------------------------------------
// Public API
//-----------------------------------------------------------------------------

namespace VanGui
{

void SetNotificationsPos(VanNotifyPos pos)
{
    g_Ctx.Pos = pos;
}

void SetNotificationsMaxCount(int max_count)
{
    if (max_count < 1)              max_count = 1;
    if (max_count > NOTIFY_HARD_MAX) max_count = NOTIFY_HARD_MAX;
    g_Ctx.MaxCount = max_count;
    // Trim any excess entries.
    if (g_Ctx.Count > g_Ctx.MaxCount)
        g_Ctx.Count = g_Ctx.MaxCount;
}

void ClearNotifications()
{
    g_Ctx.Count = 0;
}

void DismissNotification(VanNotifyID id)
{
    if (VanNotification* n = g_Ctx.FindByID(id))
        n->Dismissed = true;
}

void SetNotificationProgress(VanNotifyID id, float progress)
{
    if (VanNotification* n = g_Ctx.FindByID(id))
        n->Progress = progress;
}

VanNotifyID InsertNotificationV(VanNotifyType type, float duration_ms,
                                 const char* fmt, va_list args)
{
    VAN_ASSERT(fmt != nullptr);

    VanNotification n;
    memset(&n, 0, sizeof(n));
    n.ID           = g_Ctx.NextID++;
    if (g_Ctx.NextID == VanNotifyID_Invalid) g_Ctx.NextID = 1; // skip 0
    n.Type         = type;
    n.CreationTime = static_cast<float>(GetTime());
    n.DurationMs   = (duration_ms > 0.0f) ? duration_ms : NOTIFY_DEFAULT_MS;
    n.Progress     = -1.0f;
    n.Dismissed    = false;

    vsnprintf(n.Msg, sizeof(n.Msg), fmt, args);
    n.Msg[sizeof(n.Msg) - 1] = '\0';

    return g_Ctx.Insert(n);
}

VanNotifyID InsertNotification(VanNotifyType type, float duration_ms,
                                const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    VanNotifyID id = InsertNotificationV(type, duration_ms, fmt, args);
    va_end(args);
    return id;
}

//-----------------------------------------------------------------------------
// Render
//-----------------------------------------------------------------------------

void RenderNotifications()
{
    const float t = static_cast<float>(GetTime());
    g_Ctx.RemoveExpired(t);
    if (g_Ctx.Count == 0)
        return;

    const VanGuiViewport* vp = GetMainViewport();
    VAN_ASSERT(vp != nullptr);

    VanVec2 base_anchor, pivot;
    AnchorForPos(g_Ctx.Pos, vp, base_anchor, pivot);
    const bool bottom = IsBottomPos(g_Ctx.Pos);

    // Stack direction: bottom-anchored → offset upward (negative y per toast);
    //                  top-anchored    → offset downward (positive y per toast).
    float y_offset = 0.0f;

    // Render from top of stack toward anchor so newest is closest to the edge.
    // For bottom: iterate 0..Count-1 (index 0 = oldest = furthest from edge).
    // For top:    same iteration order, direction handled by sign of y_offset.
    for (int i = 0; i < g_Ctx.Count; ++i)
    {
        VanNotification& notif = g_Ctx.Ring[i];
        const float fade = notif.ComputeFade(t);
        if (fade <= 0.0f)
            continue;

        // Compute this toast's anchor Y with accumulated offset.
        VanVec2 anchor = base_anchor;
        if (bottom)
            anchor.y -= y_offset;
        else
            anchor.y += y_offset;

        SetNextWindowPos(anchor, VanGuiCond_Always, pivot);
        SetNextWindowBgAlpha(fade * NOTIFY_MAX_BG_ALPHA);
        SetNextWindowSizeConstraints(VanVec2(NOTIFY_MIN_WIDTH, 0.0f),
                                     VanVec2(400.0f, FLT_MAX));

        PushStyleVar(VanGuiStyleVar_WindowRounding,   6.0f);
        PushStyleVar(VanGuiStyleVar_WindowBorderSize, 1.0f);
        // Left padding wide enough for the accent stripe; right padding for the X button.
        PushStyleVar(VanGuiStyleVar_WindowPadding,
                     VanVec2(NOTIFY_BORDER_WIDTH + 10.0f, 8.0f));

        char wid[16];
        MakeWindowID(wid, i);

        // Interactive window so the close button is clickable.
        const VanGuiWindowFlags flags =
            VanGuiWindowFlags_NoDecoration    |
            VanGuiWindowFlags_NoMove          |
            VanGuiWindowFlags_NoSavedSettings |
            VanGuiWindowFlags_NoBringToFrontOnFocus |
            VanGuiWindowFlags_NoBringToDisplayOnFocus |
            VanGuiWindowFlags_NoNav           |
            VanGuiWindowFlags_AlwaysAutoResize;

        bool visible = Begin(wid, nullptr, flags);
        PopStyleVar(3);

        if (visible)
        {
            VanDrawList* dl       = GetWindowDrawList();
            const VanVec2 wp     = GetWindowPos();
            const VanVec2 ws     = GetWindowSize();
            const VanU32 acc_col = TypeColor(notif.Type, fade);

            // Accent stripe (left border).
            dl->AddRectFilled(
                VanVec2(wp.x, wp.y),
                VanVec2(wp.x + NOTIFY_BORDER_WIDTH, wp.y + ws.y),
                acc_col, 6.0f,
                VanDrawFlags_RoundCornersLeft);

            // --- Close button (top-right corner, drawn via draw list) ----------
            const float btn_sz  = 14.0f;
            const float btn_pad = 5.0f;
            const VanVec2 btn_min(wp.x + ws.x - btn_sz - btn_pad,
                                  wp.y + btn_pad);
            const VanVec2 btn_max(btn_min.x + btn_sz,
                                  btn_min.y + btn_sz);

            SetCursorScreenPos(btn_min);
            PushID(i * 1000 + 1);
            (void)InvisibleButton("##close", VanVec2(btn_sz, btn_sz));
            const bool btn_hovered = IsItemHovered();
            const bool btn_clicked = IsItemClicked();
            PopID();

            if (btn_hovered || btn_clicked)
                dl->AddRectFilled(btn_min, btn_max,
                    VAN_COL32(255, 255, 255, static_cast<int>(40 * fade)), 3.0f);

            // Draw X glyph.
            const float pad = 3.5f;
            const VanU32 x_col = VAN_COL32(200, 200, 200,
                                            static_cast<int>(200 * fade));
            dl->AddLine(VanVec2(btn_min.x + pad, btn_min.y + pad),
                        VanVec2(btn_max.x - pad, btn_max.y - pad), x_col, 1.5f);
            dl->AddLine(VanVec2(btn_max.x - pad, btn_min.y + pad),
                        VanVec2(btn_min.x + pad, btn_max.y - pad), x_col, 1.5f);

            if (btn_clicked)
                notif.Dismissed = true;

            // --- Type prefix in accent color ----------------------------------
            SetCursorScreenPos(VanVec2(wp.x + NOTIFY_BORDER_WIDTH + 10.0f,
                                       wp.y + 8.0f));
            const char* prefix = TypePrefix(notif.Type);
            if (prefix[0] != '\0')
            {
                PushStyleColor(VanGuiCol_Text, TypeColor(notif.Type, fade));
                TextUnformatted(prefix);
                PopStyleColor();
                SameLine(0.0f, 6.0f);
            }

            // Reserve space for the close button so text doesn't overlap it.
            const float text_max_x = wp.x + ws.x - btn_sz - btn_pad * 2.0f - 4.0f;
            PushTextWrapPos(text_max_x);
            PushStyleColor(VanGuiCol_Text,
                VAN_COL32(230, 230, 230, static_cast<int>(230 * fade)));
            TextUnformatted(notif.Msg);
            PopStyleColor();
            PopTextWrapPos();

            // --- Progress bar -------------------------------------------------
            if (notif.Progress >= 0.0f)
            {
                const float progress = notif.Progress < 1.0f ? notif.Progress : 1.0f;
                Spacing();

                // Draw progress bar manually to control color.
                const VanVec2 cursor   = GetCursorScreenPos();
                const float bar_w      = GetContentRegionAvail().x;
                const VanVec2 bar_min  = cursor;
                const VanVec2 bar_max  = VanVec2(cursor.x + bar_w,
                                                  cursor.y + NOTIFY_PROGRESS_HEIGHT);
                const VanVec2 fill_max = VanVec2(cursor.x + bar_w * progress,
                                                  bar_max.y);

                // Shared progress primitive (same code path as vangui_loading).
                (void)fill_max;
                VanGui::Detail::ProgressBarPrim(
                    dl, bar_min, bar_max, progress,
                    VAN_COL32(255, 255, 255, static_cast<int>(30 * fade)),
                    acc_col, 2.0f);

                Dummy(VanVec2(bar_w, NOTIFY_PROGRESS_HEIGHT));
                Spacing();
            }

            y_offset += ws.y + NOTIFY_PADDING_BETWEEN;
        }
        End();
    }
}

//-----------------------------------------------------------------------------
// Convenience wrappers
//-----------------------------------------------------------------------------

#define VH_NOTIFY_WRAPPER(FnName, Type, DurationMs)                          \
    VanNotifyID FnName(const char* fmt, ...)                                 \
    {                                                                        \
        va_list args;                                                        \
        va_start(args, fmt);                                                 \
        VanNotifyID id = InsertNotificationV(Type, DurationMs, fmt, args);  \
        va_end(args);                                                        \
        return id;                                                           \
    }

#define VH_NOTIFY_FOR_WRAPPER(FnName, Type)                                  \
    VanNotifyID FnName(float ms, const char* fmt, ...)                       \
    {                                                                        \
        va_list args;                                                        \
        va_start(args, fmt);                                                 \
        VanNotifyID id = InsertNotificationV(Type, ms, fmt, args);           \
        va_end(args);                                                        \
        return id;                                                           \
    }

#define VH_NOTIFY_V_WRAPPER(FnName, Type)                                    \
    VanNotifyID FnName(const char* fmt, va_list args)                        \
    {                                                                        \
        return InsertNotificationV(Type, NOTIFY_DEFAULT_MS, fmt, args);      \
    }

VH_NOTIFY_WRAPPER   (NotifyInfo,        VanNotifyType_Info,    NOTIFY_DEFAULT_MS)
VH_NOTIFY_WRAPPER   (NotifySuccess,     VanNotifyType_Success, NOTIFY_DEFAULT_MS)
VH_NOTIFY_WRAPPER   (NotifyWarning,     VanNotifyType_Warning, NOTIFY_DEFAULT_MS)
VH_NOTIFY_WRAPPER   (NotifyError,       VanNotifyType_Error,   NOTIFY_DEFAULT_MS)

VH_NOTIFY_FOR_WRAPPER(NotifyInfoFor,    VanNotifyType_Info)
VH_NOTIFY_FOR_WRAPPER(NotifySuccessFor, VanNotifyType_Success)
VH_NOTIFY_FOR_WRAPPER(NotifyWarningFor, VanNotifyType_Warning)
VH_NOTIFY_FOR_WRAPPER(NotifyErrorFor,   VanNotifyType_Error)

VH_NOTIFY_V_WRAPPER (NotifyInfoV,       VanNotifyType_Info)
VH_NOTIFY_V_WRAPPER (NotifySuccessV,    VanNotifyType_Success)
VH_NOTIFY_V_WRAPPER (NotifyWarningV,    VanNotifyType_Warning)
VH_NOTIFY_V_WRAPPER (NotifyErrorV,      VanNotifyType_Error)

#undef VH_NOTIFY_WRAPPER
#undef VH_NOTIFY_FOR_WRAPPER
#undef VH_NOTIFY_V_WRAPPER

} // namespace VanGui
