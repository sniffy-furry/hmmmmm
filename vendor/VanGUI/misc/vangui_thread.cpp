// vangui_thread.cpp
// VanGUI Enhancement Suite — thread pool + main-thread dispatch.
// Empty TU unless VANGUI_ENABLE_THREAD is defined.

#include "vangui_thread.h"

#ifdef VANGUI_ENABLE_THREAD

#include <thread>
#include <deque>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <cassert>
#include <vector>

namespace VanGui {

// ---------------------------------------------------------------------------
// Thread pool
// ---------------------------------------------------------------------------

struct ThreadPoolState
{
    std::vector<std::thread>          workers;
    std::deque<std::function<void()>> tasks;
    std::mutex                        mutex;
    std::condition_variable           cv;
    std::atomic<int>                  pending{0}; // queued + in-flight tasks
    bool                              stop = false;
};

static ThreadPoolState* s_Pool = nullptr;

// ---------------------------------------------------------------------------
// Main-thread dispatch queue (double-buffered for lock-minimizing drain)
// ---------------------------------------------------------------------------

static std::mutex                          s_MainMutex;
static std::vector<std::function<void()>>  s_MainQueue;  // written by any thread
static std::vector<std::function<void()>>  s_DrainBuf;   // main-thread only scratch

// ---------------------------------------------------------------------------
// Internal: submit raw work to the pool (used by public Async and VanFuture).
// ---------------------------------------------------------------------------

static void SubmitToPool(std::function<void()> work)
{
    assert(s_Pool && "Call VanGui::InitThreadPool() before submitting async work.");
    s_Pool->pending.fetch_add(1, std::memory_order_relaxed);
    {
        std::unique_lock lock(s_Pool->mutex);
        s_Pool->tasks.push_back(std::move(work));
    }
    s_Pool->cv.notify_one();
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void InitThreadPool(unsigned int n_threads)
{
    assert(!s_Pool && "InitThreadPool() called twice.");

    if (n_threads == 0)
        n_threads = std::thread::hardware_concurrency();
    if (n_threads == 0)
        n_threads = 2; // safe floor on unusual platforms

    s_Pool = new ThreadPoolState();
    s_Pool->workers.reserve(n_threads);

    for (unsigned int i = 0; i < n_threads; ++i)
    {
        s_Pool->workers.emplace_back([state = s_Pool] {
            for (;;)
            {
                std::function<void()> task;
                {
                    std::unique_lock lock(state->mutex);
                    state->cv.wait(lock, [state] {
                        return state->stop || !state->tasks.empty();
                    });
                    if (state->stop && state->tasks.empty()) return;
                    task = std::move(state->tasks.front());
                    state->tasks.pop_front();
                }
                task();
                state->pending.fetch_sub(1, std::memory_order_relaxed);
            }
        });
    }
}

void ShutdownThreadPool()
{
    if (!s_Pool) return;

    {
        std::unique_lock lock(s_Pool->mutex);
        s_Pool->stop = true;
    }
    s_Pool->cv.notify_all();
    for (auto& t : s_Pool->workers) t.join();

    delete s_Pool;
    s_Pool = nullptr;

    // Drain any remaining main-thread callbacks posted by workers.
    DrainMainThreadQueue();
}

void PostToMainThread(std::function<void()> fn)
{
    std::lock_guard lock(s_MainMutex);
    s_MainQueue.push_back(std::move(fn));
}

void Async(std::function<void()> work)
{
    SubmitToPool(std::move(work));
}

void DrainMainThreadQueue()
{
    // Swap under lock so PostToMainThread can keep posting while we drain.
    {
        std::lock_guard lock(s_MainMutex);
        if (s_MainQueue.empty()) return;
        std::swap(s_MainQueue, s_DrainBuf);
    }
    // Call each pending callback on the main thread — VanGui calls are safe here.
    for (auto& fn : s_DrainBuf) fn();
    s_DrainBuf.clear();
}

bool IsThreadPoolRunning() noexcept { return s_Pool != nullptr; }

int ThreadPoolSize() noexcept
{
    return s_Pool ? static_cast<int>(s_Pool->workers.size()) : 0;
}

int PendingWorkerTasks() noexcept
{
    return s_Pool ? s_Pool->pending.load(std::memory_order_relaxed) : 0;
}

int PendingMainThreadCallbacks() noexcept
{
    std::lock_guard lock(s_MainMutex);
    return static_cast<int>(s_MainQueue.size());
}

} // namespace VanGui

#endif // VANGUI_ENABLE_THREAD
