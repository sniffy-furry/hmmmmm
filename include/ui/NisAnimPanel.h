#pragma once
// ─── ui/NisAnimPanel.h ────────────────────────────────────────────────────────
// Animation Data Browser for NIS/Scene_*_BundleB.bun cutscene bundles.
//
// Shows the real, named skeleton/bone data recovered from the 0x00E34009
// chunk (see formats/NisAnim.h) — e.g. "Bip21" with its full 3ds-Max biped
// rig (Spine 1-3, Clavical, Upperarm/Forearm/Hand/fingers, Thigh/Shin/Foot/
// Toe, plus facial bones: Brow/Cheek/Nose/Jaw/Lip/Eyes).
//
// Scope (deliberate, per project direction — no fake/placeholder data):
// this panel surfaces the real skeleton names and bone lists only. The
// actual keyframe rotation curves live in an unindexed region of the ELF's
// .data section (not addressable via the symbol table's st_value/st_size,
// which point at a 4-byte-per-bone slot, not a keyframe stream) — decoding
// that motion data is a separate, deeper RE task not yet done. The panel
// says so explicitly rather than fabricating a playback timeline.
// ─────────────────────────────────────────────────────────────────────────────
#include "Common.h"
#include "formats/NisAnim.h"
#include "ui/FileDialog.h"
#include "async/TaskQueue.h"
#include <filesystem>
#include <string>
#include <vector>

namespace nfsmw {

class NisAnimPanel {
public:
    /// Load a NIS .bun file from `path`, walking its chunk tree for every
    /// 0x00E34009 chunk. Spawns a background task via `tasks`.
    void Load(const std::filesystem::path& path, TaskQueue& tasks);

    void Close();

    void Draw(float w, float h, FileDialog& fd, TaskQueue& tasks);

    bool IsLoaded() const { return state_ == State::Ready; }

private:
    enum class State { Empty, Loading, Ready, Error };
    State       state_ = State::Empty;
    std::string errorMsg_;
    std::string sourceName_;

    struct Clip {
        size_t      chunkOffset = 0;
        NisAnimClip data;
    };
    std::vector<Clip> clips_;
    int                selectedClip_ = -1;
    char               boneFilter_[128] = "";

    void DrawEmptyState(float w, float h);
    void DrawLoadingState(float w, float h);
    void DrawErrorState(float w, float h);
    void DrawReadyState(float w, float h);
};

} // namespace nfsmw
