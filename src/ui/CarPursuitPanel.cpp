// ─── ui/CarPursuitPanel.cpp ───────────────────────────────────────────────────
#include "ui/CarPursuitPanel.h"
#include "ui/CarPanel.h"
#include "formats/CarInfo.h"
#include <vangui.h>

namespace nfsmw {

void CarPursuitPanel::OnCarLoaded(const CarContext& ctx) {
    if (ctx.pursuitReady) {
        edit_     = ctx.pursuitData;
        stubMode_ = false;
    } else {
        // CarPursuitParser::Load() is still stubbed for this car (CARINFO.BIN
        // offsets not yet verified) — default-initialise with heat levels 1-5
        // so the UI has something sensible to show.
        edit_ = CarPursuit{};
        for (int i = 0; i < 5; ++i)
            edit_.heatTable[i].heatLevel = i + 1;
        stubMode_ = true;
    }
    dirty_ = false;
    writeStatus_.clear();
}

void CarPursuitPanel::Draw(CarContext& ctx, TaskQueue& tasks) {
    // Real pursuit values are only shown once they can be read from the game.
    // No placeholder editing, no JSON presets — honest notice instead.
    if (!ctx.pursuitReady) {
        VanGui::TextColored({1.f,0.78f,0.35f,1.f}, "Pursuit data not available");
        VanGui::Spacing();
        VanGui::TextWrapped(
            "Most Wanted's pursuit/heat tuning is not stored per car in the CARS "
            "folder. It lives in the global gameplay vault (GLOBAL/attributes.bin) "
            "under the pursuit/AI classes (AICopManager, AIPursuit, cop and "
            "roadblock settings).");
        VanGui::Spacing();
        VanGui::TextWrapped(
            "Those records are keyed by hashes whose names the game compiles into "
            "speed.exe (the attributes.bin string table carries the class names "
            "but no field-name cross-references). Decoding the values needs that "
            "schema; once available they will appear here read-only.");
        return;
    }

    DrawToolbar(ctx, tasks);
    VanGui::Separator();
    DrawBaseFields();
    VanGui::Spacing();
    DrawHeatTable();
}

void CarPursuitPanel::DrawBaseFields() {
    int bh = (int)edit_.baseHeat;
    if (VanGui::SliderInt("Base Heat Level##bhl", &bh, 1, 5)) {
        edit_.baseHeat = (uint32_t)bh; dirty_ = true;
    }
    if (VanGui::DragFloat("Busted Penalty (mult.)##bp", &edit_.bustedPenalty,
                          0.01f, 0.f, 10.f)) dirty_ = true;
    if (VanGui::DragFloat("Escape Radius (m)##er",      &edit_.escapeRadius,
                          10.f,  0.f, 2000.f)) dirty_ = true;
}

void CarPursuitPanel::DrawHeatTable() {
    VanGui::TextUnformatted("Heat Table");
    VanGui::Separator();

    constexpr VanGuiTableFlags kFlags =
        VanGuiTableFlags_Borders    |
        VanGuiTableFlags_RowBg      |
        VanGuiTableFlags_SizingFixedFit;

    if (!VanGui::BeginTable("##heattbl", 6, kFlags)) return;

    VanGui::TableSetupColumn("Level",       VanGuiTableColumnFlags_WidthFixed, 45.f);
    VanGui::TableSetupColumn("Aggression",  VanGuiTableColumnFlags_WidthFixed, 90.f);
    VanGui::TableSetupColumn("Traffic",     VanGuiTableColumnFlags_WidthFixed, 70.f);
    VanGui::TableSetupColumn("Roadblock",   VanGuiTableColumnFlags_WidthFixed, 75.f);
    VanGui::TableSetupColumn("Spike Strip", VanGuiTableColumnFlags_WidthFixed, 75.f);
    VanGui::TableSetupColumn("Helicopter",  VanGuiTableColumnFlags_WidthFixed, 75.f);
    VanGui::TableHeadersRow();

    for (int i = 0; i < 5; ++i) {
        auto& he = edit_.heatTable[i];
        VanGui::TableNextRow();

        (void)VanGui::TableSetColumnIndex(0);
        VanGui::Text("%d", he.heatLevel);

        (void)VanGui::TableSetColumnIndex(1);
        VanGui::SetNextItemWidth(80.f);
        char lid[16]; std::snprintf(lid, sizeof(lid), "##agg%d", i);
        if (VanGui::DragFloat(lid, &he.aggressionScale, 0.01f, 0.f, 5.f)) dirty_=true;

        (void)VanGui::TableSetColumnIndex(2);
        VanGui::SetNextItemWidth(60.f);
        char tid[16]; std::snprintf(tid, sizeof(tid), "##trd%d", i);
        if (VanGui::DragFloat(tid, &he.trafficDensity,  0.01f, 0.f, 1.f)) dirty_=true;

        auto flagCheck = [&](int col, uint32_t flag, const char* idstr) {
            (void)VanGui::TableSetColumnIndex(col);
            bool v = (he.flagsUnlocked & flag) != 0;
            if (VanGui::Checkbox(idstr, &v)) {
                if (v) he.flagsUnlocked |=  flag;
                else   he.flagsUnlocked &= ~flag;
                dirty_ = true;
            }
        };
        char idRb[16], idSs[16], idHel[16];
        std::snprintf(idRb,  sizeof(idRb),  "##rb%d",  i);
        std::snprintf(idSs,  sizeof(idSs),  "##ss%d",  i);
        std::snprintf(idHel, sizeof(idHel), "##hel%d", i);
        flagCheck(3, HeatFlags::Roadblocks,  idRb);
        flagCheck(4, HeatFlags::SpikeStrips, idSs);
        flagCheck(5, HeatFlags::Helicopters, idHel);
    }

    VanGui::EndTable();
}

void CarPursuitPanel::DrawToolbar(CarContext& ctx, TaskQueue& /*tasks*/) {
    bool canWrite = !stubMode_ && dirty_;

    if (!canWrite) VanGui::BeginDisabled();
    if (VanGui::Button("Save to game##pursuit")) DoSave(ctx);
    if (!canWrite) VanGui::EndDisabled();

    VanGui::SameLine();
    if (VanGui::Button("Revert##pursuit")) { OnCarLoaded(ctx); }

    if (dirty_) { VanGui::SameLine(); VanGui::TextDisabled("*"); }
    if (!writeStatus_.empty()) {
        VanGui::SameLine();
        VanGui::TextWrapped("%s", writeStatus_.c_str());
    }
}

void CarPursuitPanel::DoSave(CarContext& ctx) {
    edit_.path  = ctx.carDir / "CARINFO.BIN";
    edit_.carId = ctx.id;
    backup_.EnsureFileBak(edit_.path);
    auto r = CarPursuitParser::Save(edit_);
    if (r) {
        dirty_       = false;
        writeStatus_ = "Saved.";
    } else {
        writeStatus_ = r.error;
    }
}

} // namespace nfsmw
