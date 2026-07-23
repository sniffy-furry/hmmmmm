// ─── ui/CarEngineAudioPanel.cpp ───────────────────────────────────────────────
// Thin wrapper around SoundBankPanel that sources its GIN data from
// CarContext::engineGIN (loaded by CarPanel::LoadCar).
//
// All browse / play / export / replace logic is delegated to the embedded
// SoundBankPanel.  This panel's own responsibilities are:
//   • On car load  — call bank_.OpenGIN() with the engine GIN path so
//                    SoundBankPanel fetches and owns a fresh decoded copy.
//   • On Draw()    — render browser (left) + preview (right) side-by-side
//                    inside the Engine Audio tab's content region, with a
//                    small header showing the GIN filename and clip count.
// ─────────────────────────────────────────────────────────────────────────────
#include "ui/CarEngineAudioPanel.h"
#include "ui/CarPanel.h"       // CarContext
#include "core/Logger.h"
#include <vangui.h>

namespace nfsmw {

// ─────────────────────────────────────────────────────────────────────────────
// OnCarLoaded
//
// Called by CarPanel::LoadCar (on the main thread, after the async load
// completes) when ctx.audioReady is true.  We re-open the GIN from disk via
// SoundBankPanel::OpenGIN so that the inner panel owns its own decoded data
// and can drive async re-import without touching CarContext.
// ─────────────────────────────────────────────────────────────────────────────
void CarEngineAudioPanel::OnCarLoaded(const CarContext& ctx, TaskQueue& tasks)
{
    // The engine GIN now comes from <MW>/SOUND/ENGINE/, chosen by CarPanel's
    // fuzzy match (ctx.engineGINName). It may be empty if nothing matched — the
    // Draw() dropdown still lets the user pick one manually.
    selectedGIN_ = ctx.engineGINName;
    OpenSelected(ctx, tasks);
}

void CarEngineAudioPanel::OpenSelected(const CarContext& ctx, TaskQueue& tasks)
{
    if (selectedGIN_.empty()) return;
    const auto ginPath = ctx.engineDir / selectedGIN_;
    LOG_INFO("CarEngineAudioPanel: opening GIN '{}'", ginPath.string());
    bank_.OpenGIN(ginPath, tasks);
}

// ─────────────────────────────────────────────────────────────────────────────
// Draw
//
// Renders into the current VanGui content region (the Engine Audio tab body).
//
// Layout:
//   ┌──────────────────────────────────────────────────────────────────────┐
//   │ [header] GE_M3G.gin  •  12 clips                                     │
//   ├─────────────────────────────────┬────────────────────────────────────┤
//   │  Browser (left half)            │  Preview / transport (right half)  │
//   └─────────────────────────────────┴────────────────────────────────────┘
//
// If no car is loaded (ctx.audioReady == false) we show a placeholder.
// ─────────────────────────────────────────────────────────────────────────────
void CarEngineAudioPanel::Draw(const CarContext& ctx, TaskQueue& tasks)
{
    if (!ctx.loaded) {
        VanGui::TextDisabled("No car loaded.");
        VanGui::TextDisabled("Select a car from the browser.");
        return;
    }

    if (ctx.engineGINs.empty()) {
        VanGui::TextDisabled("No engine sounds found.");
        VanGui::TextWrapped(
            "Expected GIN files in %s. Make sure your MW SOUND/ENGINE folder is "
            "present next to CARS.", ctx.engineDir.string().c_str());
        return;
    }

    // ── Engine-sound picker (dropdown) ──────────────────────────────────────
    // Names in SOUND/ENGINE don't map 1:1 to car ids, so let the user override
    // the auto-picked match. Base sounds first; "_DCL" entries are the
    // companion deceleration banks.
    VanGui::TextUnformatted("Engine Sound");
    VanGui::SameLine();
    VanGui::SetNextItemWidth(320.f);
    const char* preview = selectedGIN_.empty() ? "(choose a GIN...)"
                                               : selectedGIN_.c_str();
    if (VanGui::BeginCombo("##enginegin", preview)) {
        for (const auto& g : ctx.engineGINs) {
            bool sel = (g == selectedGIN_);
            if (VanGui::Selectable(g.c_str(), sel) && g != selectedGIN_) {
                selectedGIN_ = g;
                OpenSelected(ctx, tasks);
            }
            if (sel) VanGui::SetItemDefaultFocus();
        }
        VanGui::EndCombo();
    }

    if (selectedGIN_.empty() || !bank_.IsLoaded()) {
        VanGui::Separator();
        VanGui::TextDisabled("%s", selectedGIN_.empty()
            ? "Pick an engine sound above to preview and edit it."
            : "Loading...");
        return;
    }

    VanGui::Separator();

    // ── Split: browser left, preview right ──────────────────────────────────
    const float totalWidth  = VanGui::GetContentRegionAvail().x;
    const float totalHeight = VanGui::GetContentRegionAvail().y;
    const float browserW    = totalWidth * 0.45f;
    const float previewW    = totalWidth - browserW - VanGui::GetStyle().ItemSpacing.x;

    // Left pane — entry list
    (void)VanGui::BeginChild("##engine_browser", VanVec2(browserW, totalHeight),
                      /*border=*/true);
    bank_.DrawBrowser();
    VanGui::EndChild();

    VanGui::SameLine();

    // Right pane — waveform + transport + export/replace
    (void)VanGui::BeginChild("##engine_preview", VanVec2(previewW, totalHeight),
                      /*border=*/true);
    bank_.DrawPreview(tasks);
    VanGui::EndChild();
}

} // namespace nfsmw
