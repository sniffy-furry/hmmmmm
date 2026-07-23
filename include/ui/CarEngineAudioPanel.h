#pragma once
// ─── ui/CarEngineAudioPanel.h ─────────────────────────────────────────────────
// Thin wrapper around SoundBankPanel for engine GIN audio.
// Sources the GINFile from CarContext::engineGIN (loaded by CarPanel).
// All browse/play/export/replace logic is delegated to SoundBankPanel.
// ─────────────────────────────────────────────────────────────────────────────
#include "Common.h"
#include "ui/SoundBankPanel.h"
#include "ui/FileDialog.h"
#include "async/TaskQueue.h"

namespace nfsmw {

struct CarContext;

class CarEngineAudioPanel {
public:
    void SetFileDialog(FileDialog* fd) { bank_.SetFileDialog(fd); }

    /// Called when a new car is loaded — points the inner SoundBankPanel at
    /// the car's GIN file (re-opens from disk so SoundBankPanel owns the data).
    void OnCarLoaded(const CarContext& ctx, TaskQueue& tasks);

    /// Draw browser (left pane) and preview (right pane) side by side.
    void Draw(const CarContext& ctx, TaskQueue& tasks);

private:
    SoundBankPanel bank_;
    std::string    selectedGIN_;   ///< currently-open GIN filename (from SOUND/ENGINE)

    void OpenSelected(const CarContext& ctx, TaskQueue& tasks);
};

} // namespace nfsmw
