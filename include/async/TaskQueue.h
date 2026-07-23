#pragma once
// ─── async/TaskQueue.h ────────────────────────────────────────────────────────
// Progress-reporting task façade. As of the VanGUI overhaul (Phase 2) this is a
// THIN ADAPTER over VanGui's shared thread pool (misc/vangui_thread.h) — the old
// hand-rolled std::thread-per-task pool is gone. Work lambdas run on a VanGUI
// worker; completion callbacks are posted to the main thread via
// VanGui::PostToMainThread (drained each frame inside VanGui::NewFrameExtras()).
//
// The Submit()/PumpAndDraw()/Busy() API is unchanged so the ~80 panel call sites
// keep compiling; PumpAndDraw() now only draws the progress overlay (callback
// dispatch is owned by the VanGUI thread pool).
//
// Usage:
//   taskQueue.Submit("Decompressing foo.lzc", [path](ProgressState& p) {
//       p.fraction = -1.f;        // indeterminate spinner
//       auto buf = ReadFile(path);
//       p.fraction = 0.5f;
//       // ...
//   }, [&]{ /* main-thread callback when done */ });
//
//   // In your render loop:
//   taskQueue.PumpAndDraw();      // draws the progress overlay
//
// ─────────────────────────────────────────────────────────────────────────────
#include "Common.h"
#include <atomic>
#include <functional>
#include <mutex>
#include <string>
#include <vector>
#include <memory>

namespace nfsmw {

// ─── ProgressState ────────────────────────────────────────────────────────────
// Shared between the worker and the UI thread (accessed atomically).
// `fraction` in [0, 1] → determinate bar; negative → spinning indeterminate.
// `label` is set once before Submit() returns; `detail` may be updated by work.
struct ProgressState {
    std::string             label;    ///< Task name shown in the progress overlay
    std::atomic<float>      fraction{ -1.f };  ///< -1 = indeterminate
    // Detail text (UI only reads it; worker may write). Guarded by the TaskQueue
    // mutex on write (workers should call SetDetail rather than assigning directly).
    std::string             detail;
    std::mutex              detailMtx;

    void SetDetail(const std::string& s) {
        std::lock_guard lk(detailMtx);
        detail = s;
    }
    std::string GetDetail() {
        std::lock_guard lk(detailMtx);
        return detail;
    }
};

// ─── TaskQueue ────────────────────────────────────────────────────────────────
class TaskQueue {
public:
    TaskQueue()  = default;
    ~TaskQueue() { CancelAll(); }

    // Non-copyable, non-movable (mutexes and atomics don't like it).
    TaskQueue(const TaskQueue&)            = delete;
    TaskQueue& operator=(const TaskQueue&) = delete;

    /// Submit a task.
    ///   `label`  — human-readable name shown in progress overlay.
    ///   `work`   — called on a worker thread. Receives a ProgressState& to update.
    ///   `onDone` — called on the UI thread the frame after work completes.
    void Submit(std::string label,
                std::function<void(ProgressState&)> work,
                std::function<void()> onDone = {});

    /// Call once per frame from the UI thread. Draws the progress overlay for
    /// the oldest in-flight task. (Completion callbacks are dispatched by the
    /// VanGUI thread pool via PostToMainThread, not here.)
    void PumpAndDraw();

    /// True when at least one task is still in flight.
    bool Busy() const;

    /// Forget tracked tasks (real worker join happens at VanGui::ShutdownThreadPool).
    void CancelAll();

private:
    std::mutex                                    tasksMtx_;
    std::vector<std::shared_ptr<ProgressState>>   active_;  ///< In-flight tasks

    void DrawOverlay(ProgressState& p);
};

} // namespace nfsmw
