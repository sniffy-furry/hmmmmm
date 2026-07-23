// ─── ui/CarPerfPanel.cpp ──────────────────────────────────────────────────────
#include "ui/CarPerfPanel.h"
#include "ui/CarPanel.h"
#include "formats/CarInfo.h"
#include "core/StringHash.h"
#include <vangui.h>
#include <algorithm>
#include <cctype>
#include <cstdio>

namespace nfsmw {

void CarPerfPanel::OnCarLoaded(const CarContext& ctx) {
    edit_        = ctx.perfData;
    dirty_       = false;
    stubMode_    = !ctx.perfReady;
    writeStatus_.clear();
}

void CarPerfPanel::Draw(CarContext& ctx, TaskQueue& tasks) {
    // Performance/handling live in the global tuning vault (GLOBAL/attributes.bin),
    // keyed per car. Load it here and edit the real named fields directly.
    DrawVaultEditor(ctx);

    if (ctx.perfReady) {
        VanGui::Separator();
        DrawToolbar(ctx, tasks);
        VanGui::Separator();
        DrawEngineSection();
        DrawGearboxSection();
        DrawHandlingSection();
        DrawSuspensionSection(ctx);
        DrawTyreSection();
    }
}

// ─── Attribute-vault editor ───────────────────────────────────────────────────

void CarPerfPanel::LoadVaultFile(const std::filesystem::path& p) {
    edits_.clear();
    vaultRecs_.clear();
    recClass_.clear();
    auto r = VaultDecoder::Decode(p);
    if (!r) { vaultStatus_ = "Load failed: " + r.error; return; }
    vaultPath_ = p;
    vaultRecs_ = std::move(r.value);

    // Assign each record to the schema class whose fields it best matches.
    recClass_.resize(vaultRecs_.size(), nullptr);
    for (size_t i = 0; i < vaultRecs_.size(); ++i) {
        const VaultClassSchema* best = nullptr;
        int bestHits = 0;
        for (const auto& cls : VaultSchemaCatalog()) {
            int hits = 0;
            for (const auto& t : vaultRecs_[i].triples)
                if (cls.FindField(t.fieldHash)) ++hits;
            if (hits > bestHits) { bestHits = hits; best = &cls; }
        }
        recClass_[i] = best;
    }
    char buf[160];
    std::snprintf(buf, sizeof buf, "Loaded %zu value records from %s",
                  vaultRecs_.size(), p.filename().string().c_str());
    vaultStatus_ = buf;
}

void CarPerfPanel::SaveVaultEdits() {
    if (edits_.empty() || vaultPath_.empty()) return;
    std::vector<VaultDecoder::Edit> batch;
    batch.reserve(edits_.size());
    for (const auto& [off, val] : edits_) batch.push_back({ off, val });
    auto r = VaultDecoder::PatchFloats(vaultPath_, batch, backup_);
    if (!r) { vaultStatus_ = "Save failed: " + r.error; return; }
    char buf[160];
    std::snprintf(buf, sizeof buf, "Saved %d value(s) to %s (backup kept)",
                  r.value, vaultPath_.filename().string().c_str());
    vaultStatus_ = buf;
    edits_.clear();
    // Re-decode so offsets/values reflect the new file.
    LoadVaultFile(vaultPath_);
}

void CarPerfPanel::DrawVaultEditor(CarContext& ctx) {
    // Auto-probe GLOBAL/attributes.bin next to the CARS folder the first time we
    // see a given car directory, so it "just works" without a manual open.
    if (vaultPath_.empty() && !ctx.carDir.empty() && autoTriedFor_ != ctx.carDir) {
        autoTriedFor_ = ctx.carDir;
        std::error_code ec;
        // CARS/<id>/ -> game root -> GLOBAL/attributes.bin
        const auto root = ctx.carDir.parent_path().parent_path();
        for (const auto* rel : { "GLOBAL/attributes.bin", "GLOBAL/GLOBALB.bun",
                                 "attributes.bin" }) {
            auto cand = root / rel;
            if (std::filesystem::exists(cand, ec) && cand.extension() == ".bin") {
                LoadVaultFile(cand);
                break;
            }
        }
    }

    // ── Toolbar ───────────────────────────────────────────────────────────────
    if (VanGui::Button("Open Cars Folder\xe2\x80\xa6")) {
        fileDlg_.Show("Select your CARS or GLOBAL folder", FileDialog::Mode::Folder, {},
            [this](const std::filesystem::path& dir) {
                std::error_code ec;
                for (const auto* rel : { "attributes.bin", "GLOBAL/attributes.bin",
                                         "../GLOBAL/attributes.bin" }) {
                    auto cand = dir / rel;
                    if (std::filesystem::exists(cand, ec)) { LoadVaultFile(cand); return; }
                }
                vaultStatus_ = "No attributes.bin found under that folder.";
            });
    }
    VanGui::SameLine();
    if (VanGui::Button("Select Attribute\xe2\x80\xa6")) {
        fileDlg_.Show("Select an attribute vault (.bin)", FileDialog::Mode::Open,
            { ".bin" }, [this](const std::filesystem::path& p) { LoadVaultFile(p); });
    }
    if (!vaultPath_.empty()) {
        VanGui::SameLine();
        VanGui::TextDisabled("%s", vaultPath_.filename().string().c_str());
    }
    fileDlg_.Draw();

    if (vaultRecs_.empty()) {
        VanGui::Spacing();
        VanGui::TextWrapped(
            "Open your GLOBAL/attributes.bin (or gameplay.bin) to edit the real "
            "engine, transmission, chassis, tyre, NOS and induction values. Fields "
            "are shown by name; edits are written in place with a .bak backup.");
        if (!vaultStatus_.empty()) { VanGui::Spacing(); VanGui::TextWrapped("%s", vaultStatus_.c_str()); }
        return;
    }

    // ── Search + save ───────────────────────────────────────────────────────
    VanGui::SetNextItemWidth(220.f);
    (void)VanGui::InputTextWithHint("##vaultsearch", "Search field / car key\xe2\x80\xa6",
                                    vaultSearch_, sizeof vaultSearch_);
    VanGui::SameLine();
    const bool hasEdits = !edits_.empty();
    if (!hasEdits) VanGui::BeginDisabled();
    if (VanGui::Button(hasEdits ? "Save changes to attributes.bin" : "No changes"))
        SaveVaultEdits();
    if (!hasEdits) VanGui::EndDisabled();
    if (!vaultStatus_.empty()) VanGui::TextDisabled("%s", vaultStatus_.c_str());
    VanGui::Separator();

    std::string filt = vaultSearch_;
    std::transform(filt.begin(), filt.end(), filt.begin(),
                   [](unsigned char c) { return (char)std::tolower(c); });
    auto matches = [&](const std::string& s) {
        if (filt.empty()) return true;
        std::string l = s;
        std::transform(l.begin(), l.end(), l.begin(),
                       [](unsigned char c) { return (char)std::tolower(c); });
        return l.find(filt) != std::string::npos;
    };

    // ── Records grouped by class ──────────────────────────────────────────────
    (void)VanGui::BeginChild("##vaultlist", VanVec2(0, 0), true);
    for (const auto& cls : VaultSchemaCatalog()) {
        // gather this class's records
        std::vector<size_t> recs;
        for (size_t i = 0; i < vaultRecs_.size(); ++i)
            if (recClass_[i] == &cls) recs.push_back(i);
        if (recs.empty()) continue;

        char hdr[64];
        std::snprintf(hdr, sizeof hdr, "%s  (%s)  [%zu]",
                      cls.className, cls.group, recs.size());
        if (!VanGui::CollapsingHeader(hdr)) continue;

        // Merge records that share a key into one entry, de-duplicating fields.
        // (The vault stores a car's tuning across several marker-delimited
        // records; showing each raw record produced repeated "(default/base)"
        // rows. Grouping by key collapses them into one node per car/key.)
        struct KeyGroup { uint32_t keyHash; std::string label; std::vector<const VaultTriple*> fields; };
        std::vector<KeyGroup> groups;
        std::unordered_map<uint32_t, size_t> gi;
        for (size_t ri : recs) {
            auto& rec = vaultRecs_[ri];
            auto itg = gi.find(rec.keyHash);
            if (itg == gi.end()) {
                gi[rec.keyHash] = groups.size();
                KeyGroup g; g.keyHash = rec.keyHash;
                if (!rec.keyName.empty()) g.label = rec.keyName;
                else { char b[24]; std::snprintf(b, sizeof b, "key 0x%08X", rec.keyHash); g.label = b; }
                groups.push_back(std::move(g));
                itg = gi.find(rec.keyHash);
            }
            KeyGroup& g = groups[itg->second];
            for (auto& t : rec.triples) {
                if (t.fieldName.empty()) continue;
                bool dup = false;
                for (const VaultTriple* ex : g.fields)
                    if (ex->fieldHash == t.fieldHash) { dup = true; break; }
                if (!dup) g.fields.push_back(&t);
            }
        }

        VanGui::Indent();
        for (const KeyGroup& g : groups) {
            if (g.fields.empty()) continue;
            if (!matches(g.label) && !matches(cls.className)) {
                bool any = false;
                for (const VaultTriple* t : g.fields) if (matches(t->fieldName)) { any = true; break; }
                if (!any) continue;
            }
            VanGui::PushID((int)g.keyHash);   // stable + unique (grouped by key)
            if (VanGui::TreeNodeEx("##rec", VanGuiTreeNodeFlags_DefaultOpen, "%s", g.label.c_str())) {
                for (const VaultTriple* t : g.fields) {
                    if (!filt.empty() && !matches(t->fieldName) && !matches(g.label)) continue;
                    auto it = edits_.find(t->valueOffset);
                    float v = (it != edits_.end()) ? it->second : t->value;
                    VanGui::SetNextItemWidth(140.f);
                    VanGui::PushID((int)t->fieldHash);   // stable + unique within the key
                    if (VanGui::DragFloat(t->fieldName.c_str(), &v, 0.01f))
                        edits_[t->valueOffset] = v;
                    if (it != edits_.end()) {
                        VanGui::SameLine();
                        VanGui::TextColored(VanVec4(0.98f,0.75f,0.30f,1.f), "*");
                    }
                    VanGui::PopID();
                }
                VanGui::TreePop();
            }
            VanGui::PopID();
        }
        VanGui::Unindent();
    }
    VanGui::EndChild();
}

// ── Sections ──────────────────────────────────────────────────────────────────

void CarPerfPanel::DrawEngineSection() {
    if (!VanGui::CollapsingHeader("Engine", VanGuiTreeNodeFlags_DefaultOpen)) return;
    VanGui::Indent();
    if (VanGui::DragFloat("Top Speed (mph)##ts",  &edit_.topSpeedMph,    1.f,  0.f,400.f)) dirty_=true;
    if (VanGui::DragFloat("0–60 time (s)##0_60",  &edit_.accel0_60,     0.01f, 0.f, 30.f)) dirty_=true;
    if (VanGui::DragFloat("Torque (Nm)##torq",    &edit_.engineTorque,  10.f,  0.f,2000.f)) dirty_=true;
    if (VanGui::DragFloat("Redline (RPM)##rpm",   &edit_.engineRPM,    100.f,  0.f,12000.f)) dirty_=true;
    if (VanGui::DragFloat("Turbo Boost##turbo",   &edit_.turboBoost,   0.01f, 0.f,  5.f)) dirty_=true;
    VanGui::Unindent();
}

void CarPerfPanel::DrawGearboxSection() {
    if (!VanGui::CollapsingHeader("Gearbox")) return;
    VanGui::Indent();
    int gc = (int)edit_.gearCount;
    if (VanGui::SliderInt("Gear Count##gc", &gc, 0, 8)) {
        edit_.gearCount = (uint32_t)gc; dirty_ = true;
    }
    for (int i = 0; i <= (int)edit_.gearCount && i < 8; ++i) {
        char label[32];
        std::snprintf(label, sizeof(label), i == 0 ? "Reverse##gr0" : "Gear %d##gr%d", i, i);
        if (VanGui::DragFloat(label, &edit_.gearRatios[i], 0.01f, 0.01f, 10.f)) dirty_ = true;
    }
    if (VanGui::DragFloat("Final Drive##fd", &edit_.finalDrive, 0.01f, 0.01f, 10.f)) dirty_=true;
    VanGui::Unindent();
}

void CarPerfPanel::DrawHandlingSection() {
    if (!VanGui::CollapsingHeader("Handling")) return;
    VanGui::Indent();
    if (VanGui::DragFloat("Mass (kg)##mass",          &edit_.mass,           10.f,  100.f,5000.f)) dirty_=true;
    if (VanGui::DragFloat("Front Weight Bias##fwb",   &edit_.frontWeightBias,0.01f,  0.f,   1.f)) dirty_=true;
    if (VanGui::DragFloat("Front Downforce##fdf",     &edit_.frontDownforce, 10.f,    0.f,5000.f)) dirty_=true;
    if (VanGui::DragFloat("Rear Downforce##rdf",      &edit_.rearDownforce,  10.f,    0.f,5000.f)) dirty_=true;
    if (VanGui::DragFloat("Steering Lock (deg)##sl",  &edit_.steeringLock,    1.f,    5.f,  60.f)) dirty_=true;
    if (VanGui::DragFloat("Brake Bias (front)##bb",   &edit_.brakeBias,      0.01f,   0.f,   1.f)) dirty_=true;
    if (VanGui::DragFloat("Brake Strength##bs",       &edit_.brakeStrength,   0.1f,   0.f, 100.f)) dirty_=true;
    VanGui::Unindent();
}

void CarPerfPanel::DrawSuspensionSection(CarContext& /*ctx*/) {
    if (!VanGui::CollapsingHeader("Suspension")) return;
    VanGui::Indent();
    bool changed = false;
    if (VanGui::DragFloat("Front Ride Height (m)##frh", &edit_.frontSuspHeight,0.001f,0.f,0.5f)) changed=true;
    if (VanGui::DragFloat("Rear Ride Height (m)##rrh",  &edit_.rearSuspHeight, 0.001f,0.f,0.5f)) changed=true;
    if (VanGui::DragFloat("Front Track (m)##ftr",        &edit_.frontSuspTrack, 0.001f,0.f,2.f))  changed=true;
    if (VanGui::DragFloat("Rear Track (m)##rtr",         &edit_.rearSuspTrack,  0.001f,0.f,2.f))  changed=true;
    if (changed) {
        dirty_ = true;
        // Poke CarViewerPanel to update wheel corner positions — viewer reads
        // from ctx.perfData which will be updated on Save().
    }
    VanGui::Unindent();
}

void CarPerfPanel::DrawTyreSection() {
    if (!VanGui::CollapsingHeader("Tyres")) return;
    VanGui::Indent();
    if (VanGui::DragFloat("Friction Front##tff", &edit_.tyreFrictionFront, 0.01f, 0.f, 2.f)) dirty_=true;
    if (VanGui::DragFloat("Friction Rear##tfr",  &edit_.tyreFrictionRear,  0.01f, 0.f, 2.f)) dirty_=true;
    if (VanGui::DragFloat("Tyre Width (m)##tw",  &edit_.tyreWidth,         0.001f,0.1f,0.4f)) dirty_=true;
    VanGui::Unindent();
}

// ── Toolbar ───────────────────────────────────────────────────────────────────

void CarPerfPanel::DrawToolbar(CarContext& ctx, TaskQueue& /*tasks*/) {
    bool canWrite = !stubMode_ && dirty_;

    if (!canWrite) VanGui::BeginDisabled();
    if (VanGui::Button("Save to game##perf")) DoSave(ctx);
    if (!canWrite) VanGui::EndDisabled();

    VanGui::SameLine();
    if (VanGui::Button("Revert##perf")) { edit_ = ctx.perfData; dirty_ = false; }

    if (dirty_) { VanGui::SameLine(); VanGui::TextDisabled("*"); }
    if (!writeStatus_.empty()) {
        VanGui::SameLine();
        VanGui::TextWrapped("%s", writeStatus_.c_str());
    }
}

void CarPerfPanel::DoSave(CarContext& ctx) {
    backup_.EnsureFileBak(ctx.carDir / "CARINFO.BIN");
    edit_.path  = ctx.carDir / "CARINFO.BIN";
    edit_.carId = ctx.id;
    auto r = CarInfoParser::Save(edit_);
    if (r) {
        ctx.perfData = edit_;
        dirty_       = false;
        writeStatus_ = "Saved.";
    } else {
        writeStatus_ = r.error;
    }
}

} // namespace nfsmw
