#include "async/TaskQueue.h"
#include "core/Logger.h"

#include <vangui.h>
#include <vangui_thread.h>   // VanGui::Async / PostToMainThread (shared pool)

#include <algorithm>
#include <cmath>

namespace nfsmw {

// ─── Submit ──────────────────────────────────────────────────────────────────
// Dispatches `work` to the shared VanGUI thread pool. The completion lambda is
// posted to the main thread by VanGui::Async (drained inside NewFrameExtras()),
// so `onDone` always runs on the UI thread — safe to call VanGui from it.
void TaskQueue::Submit(std::string label,
                       std::function<void(ProgressState&)> work,
                       std::function<void()> onDone) {
    auto progress     = std::make_shared<ProgressState>();
    progress->label   = std::move(label);

    {
        std::lock_guard lk(tasksMtx_);
        active_.push_back(progress);
    }
    LOG_DEBUG("TaskQueue: submitted '{}'", progress->label);

    VanGui::Async(
        // ── worker thread: do the work, never touch VanGui ──
        [work = std::move(work), progress]() {
            try {
                work(*progress);
            } catch (const std::exception& ex) {
                LOG_ERROR("TaskQueue: task '{}' threw: {}", progress->label, ex.what());
            } catch (...) {
                LOG_ERROR("TaskQueue: task '{}' threw unknown exception", progress->label);
            }
            progress->fraction.store(1.0f);
        },
        // ── main thread: untrack + fire onDone ──
        [this, progress, onDone = std::move(onDone)]() {
            {
                std::lock_guard lk(tasksMtx_);
                active_.erase(std::remove(active_.begin(), active_.end(), progress),
                              active_.end());
            }
            if (onDone) { try { onDone(); } catch (...) {} }
        });
}

// ─── PumpAndDraw ─────────────────────────────────────────────────────────────
void TaskQueue::PumpAndDraw() {
    // Completion callbacks are dispatched by the VanGUI thread pool; here we only
    // draw the overlay for the oldest in-flight task.
    std::shared_ptr<ProgressState> active;
    {
        std::lock_guard lk(tasksMtx_);
        if (!active_.empty()) active = active_.front();
    }
    if (active && active->fraction.load() < 1.0f) DrawOverlay(*active);
}

// ─── Busy ────────────────────────────────────────────────────────────────────
bool TaskQueue::Busy() const {
    std::lock_guard lk(const_cast<std::mutex&>(tasksMtx_));
    return !active_.empty();
}

// ─── CancelAll ───────────────────────────────────────────────────────────────
// Workers are owned by the shared pool; they are joined at
// VanGui::ShutdownThreadPool(). Here we just drop our tracking entries.
void TaskQueue::CancelAll() {
    std::lock_guard lk(tasksMtx_);
    active_.clear();
}

// ─── DrawOverlay ─────────────────────────────────────────────────────────────
// Bottom-centre card: animated spinning ring + label + progress bar.
// Accent colour is read from VanGuiCol_SliderGrab, which ApplyTheme() always
// sets to the current theme accent — so the overlay stays theme-aware without
// any coupling between TaskQueue and AppShell.
void TaskQueue::DrawOverlay(ProgressState& p) {
    VanGuiViewport* vp       = VanGui::GetMainViewport();
    const float    overlayW = vp->WorkSize.x * 0.46f;
    const float    pad      = 10.0f;
    const float    spinR    = 8.0f;   // spinner ring radius
    const float    spinD    = spinR * 2.0f;
    const float    barH     = 8.0f;
    const float    textH    = VanGui::GetTextLineHeight();
    const float    rowH     = std::max(spinD, textH);

    const std::string det = p.GetDetail();
    const float detH = det.empty() ? 0.0f : (textH + 4.0f);
    const float winH = pad + rowH + 6.0f + barH + detH + pad;

    const VanVec2 pos(
        vp->WorkPos.x + (vp->WorkSize.x - overlayW) * 0.5f,
        vp->WorkPos.y + vp->WorkSize.y - winH - 38.0f);

    VanGui::SetNextWindowPos(pos, VanGuiCond_Always);
    VanGui::SetNextWindowSize(VanVec2(overlayW, winH), VanGuiCond_Always);
    VanGui::SetNextWindowBgAlpha(0.92f);

    constexpr VanGuiWindowFlags kFlags =
        VanGuiWindowFlags_NoDecoration | VanGuiWindowFlags_NoInputs  |
        VanGuiWindowFlags_NoMove       | VanGuiWindowFlags_NoSavedSettings |
        VanGuiWindowFlags_NoDocking    | VanGuiWindowFlags_NoBringToFrontOnFocus;

    VanGui::PushStyleVar(VanGuiStyleVar_WindowPadding,  VanVec2(pad, pad));
    VanGui::PushStyleVar(VanGuiStyleVar_WindowRounding, 8.0f);

    if (VanGui::Begin("##taskprogress", nullptr, kFlags)) {
        VanDrawList* dl   = VanGui::GetWindowDrawList();
        const float frac = p.fraction.load();
        const float t    = static_cast<float>(VanGui::GetTime());

        // ── Theme accent (via SliderGrab, always set to the current accent) ──
        const VanVec4 acV = VanGui::GetStyleColorVec4(VanGuiCol_SliderGrab);
        const VanU32  acC = VanGui::ColorConvertFloat4ToU32(acV);

        const float innerW = overlayW - pad * 2.0f;

        // ── Row 1: spinning ring + label (drawn with DrawList for precise layout) ──
        const VanVec2 rowOrigin = VanGui::GetCursorScreenPos();
        const VanVec2 sc(rowOrigin.x + spinR, rowOrigin.y + rowH * 0.5f);

        // Dim track circle
        dl->AddCircle(sc, spinR, VAN_COL32(128, 128, 128, 60), 32, 2.0f);

        // Animated arc (225° sweep that rotates with time)
        constexpr float kPi = 3.14159265358979f;
        const float angle   = std::fmod(t * 2.2f, 1.0f) * 2.0f * kPi;
        const float arcSpan = kPi * 1.25f;
        constexpr int kSegs = 18;
        for (int i = 0; i < kSegs; ++i) {
            const float a0  = angle + static_cast<float>(i)     / kSegs * arcSpan;
            const float a1  = angle + static_cast<float>(i + 1) / kSegs * arcSpan;
            const float alp = static_cast<float>(i + 1) / kSegs;
            const VanU32 col = VanGui::ColorConvertFloat4ToU32(
                VanVec4(acV.x, acV.y, acV.z, acV.w * alp * 0.95f));
            dl->AddLine(VanVec2(sc.x + spinR * cosf(a0), sc.y + spinR * sinf(a0)),
                        VanVec2(sc.x + spinR * cosf(a1), sc.y + spinR * sinf(a1)),
                        col, 2.5f);
        }

        // Label text vertically centred alongside the ring
        const VanVec2 labelPos(rowOrigin.x + spinD + 8.0f,
                              rowOrigin.y + (rowH - textH) * 0.5f);
        dl->AddText(labelPos,
                    VanGui::GetColorU32(VanGuiCol_Text),
                    p.label.c_str());

        VanGui::Dummy(VanVec2(innerW, rowH));
        VanGui::Spacing();

        // ── Row 2: progress bar ──────────────────────────────────────────────
        const VanVec2 bbMin = VanGui::GetCursorScreenPos();
        const VanVec2 bbMax = VanVec2(bbMin.x + innerW, bbMin.y + barH);
        const float  bw    = innerW;

        // Track
        dl->AddRectFilled(bbMin, bbMax, VAN_COL32(80, 80, 80, 100), 4.0f);

        if (frac < 0.f) {
            // Indeterminate — moving sweep with wrap-around
            const float phase = std::fmod(t * 1.6f, 1.0f);
            const float seg   = 0.38f;
            const float start = phase;
            const float end   = start + seg;
            if (end > 1.f) {
                dl->AddRectFilled(VanVec2(bbMin.x + start * bw, bbMin.y), bbMax, acC, 4.0f);
                dl->AddRectFilled(bbMin, VanVec2(bbMin.x + (end - 1.f) * bw, bbMax.y), acC, 4.0f);
            } else {
                dl->AddRectFilled(VanVec2(bbMin.x + start * bw, bbMin.y),
                                  VanVec2(bbMin.x + end   * bw, bbMax.y), acC, 4.0f);
            }
        } else {
            const float clamped = std::clamp(frac, 0.f, 1.f);
            dl->AddRectFilled(bbMin, VanVec2(bbMin.x + clamped * bw, bbMax.y), acC, 4.0f);
        }
        VanGui::Dummy(VanVec2(innerW, barH));

        // ── Row 3: optional detail text ──────────────────────────────────────
        if (!det.empty()) {
            VanGui::Spacing();
            VanGui::TextDisabled("%s", det.c_str());
        }
    }
    VanGui::End();
    VanGui::PopStyleVar(2);
}

} // namespace nfsmw
