#pragma once
// ─── ui/CarBrowserPanel.h ─────────────────────────────────────────────────────
// Scans CARS/ for three-letter subdirectories and presents a selectable list.
// Drives the shared CarContext via a callback supplied by CarPanel.
// ─────────────────────────────────────────────────────────────────────────────
#include "Common.h"
#include "async/TaskQueue.h"
#include <filesystem>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace nfsmw {

class CarBrowserPanel {
public:
    using OnSelectFn = std::function<void(const std::string& carId)>;

    /// Set the callback invoked when the user picks a car.
    void SetOnSelect(OnSelectFn fn) { onSelect_ = std::move(fn); }

    /// Begin an async scan of `carsRoot`.  Clears any previous list.
    void Scan(const std::filesystem::path& carsRoot, TaskQueue& tasks);

    /// Draw the list into the current VanGui region.
    void Draw();

    bool IsScanning() const { return scanning_; }
    bool HasCars()    const { return !carIds_.empty(); }
    const std::string& SelectedId() const { return selectedId_; }

    /// Optional display-name lookup (car ID -> friendly name) used to render
    /// "ID — Full Name" in the selectable list. Not exhaustive; IDs without
    /// an entry are shown as-is.
    static const std::unordered_map<std::string, std::string> kCarNames;

private:
    std::vector<std::string> carIds_;
    std::string              selectedId_;
    std::string              filter_;
    bool                     scanning_ = false;

    // Pending result from the worker thread
    std::vector<std::string> pendingIds_;
    bool                     pendingReady_ = false;

    OnSelectFn onSelect_;

    void PumpPending();
};

} // namespace nfsmw
