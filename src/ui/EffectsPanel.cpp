// ─── ui/EffectsPanel.cpp ──────────────────────────────────────────────────────
#include "ui/EffectsPanel.h"
#include "ui/EffectPreview.h"
#include "formats/EmitterCatalog.h"
#include <vangui.h>
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <unordered_set>

namespace nfsmw {

namespace {

std::string Lower(const std::string& s) {
    std::string o = s;
    for (char& c : o) c = (char)std::tolower((unsigned char)c);
    return o;
}

// Infer a preview family from an effect/emitter collection name.
FxFamily FamilyOfName(const std::string& nameIn) {
    const std::string n = Lower(nameIn);
    auto has = [&](const char* s){ return n.find(s) != std::string::npos; };
    if (has("bird"))                                   return FxFamily::Birds;
    if (has("leaf") || has("leaves"))                  return FxFamily::Leaves;
    if (has("fog") || has("haze") || has("mist"))      return FxFamily::Fog;
    if (has("fountain") || has("ripple") || has("water") || has("splash") || has("wtr"))
                                                       return FxFamily::Water;
    if (has("smoke") || has("steam") || has("chimney") || has("stack") || has("smk"))
                                                       return FxFamily::SmokeSteam;
    if (has("dust") || has("mote"))                    return FxFamily::Dust;
    if (has("coplight") || has("light") || has("flare") || has("glow"))
                                                       return FxFamily::Lights;
    if (has("spark") || has("sprk") || has("glass") || has("coin"))
                                                       return FxFamily::Sparks;
    if (has("fire") || has("flame") || has("burn"))    return FxFamily::Fire;
    if (has("nos") || has("backfire") || has("exhaust")|| has("fxcar"))
                                                       return FxFamily::Car;
    if (has("dirt") || has("grass") || has("gravel") || has("fxtd"))
                                                       return FxFamily::Terrain;
    if (has("explo") || has("blast") || has("gaspump"))return FxFamily::Explosion;
    return FxFamily::Other;
}

float BitsToFloat(uint32_t u) { float f; std::memcpy(&f, &u, 4); return f; }
uint32_t FloatToBits(float f)  { uint32_t u; std::memcpy(&u, &f, 4); return u; }

} // namespace

void EffectsPanel::Draw() {
    DrawToolbar();

    if (collections_.empty()) {
        VanGui::Spacing();
        VanGui::TextWrapped(
            "Open your GLOBAL/attributes.bin to browse and edit the game's "
            "particle effects \xe2\x80\x94 birds, smoke, fog, fountains, cop lights, "
            "fire, NOS and more. You can retune colours, gravity, particle count, "
            "life, speed and spread, with a live preview. Effects are presentation-"
            "only, so this is the safest data in the game to change.");
        if (!status_.empty()) { VanGui::Spacing(); VanGui::TextWrapped("%s", status_.c_str()); }
        return;
    }

    const float totalW = VanGui::GetContentRegionAvail().x;
    const float listW  = std::min(320.f, totalW * 0.34f);
    DrawList(listW);
    VanGui::SameLine();
    DrawEditor(totalW - listW - VanGui::GetStyle().ItemSpacing.x);
}

void EffectsPanel::DrawToolbar() {
    if (VanGui::Button("Open Cars Folder\xe2\x80\xa6")) {
        fileDlg_.Show("Select your CARS or GLOBAL folder", FileDialog::Mode::Folder, {},
            [this](const std::filesystem::path& dir) {
                std::error_code ec;
                for (const auto* rel : { "attributes.bin", "GLOBAL/attributes.bin",
                                         "../GLOBAL/attributes.bin" }) {
                    auto cand = dir / rel;
                    if (std::filesystem::exists(cand, ec)) { LoadVault(cand); return; }
                }
                status_ = "No attributes.bin found under that folder.";
            });
    }
    VanGui::SameLine();
    if (VanGui::Button("Select Attribute\xe2\x80\xa6")) {
        fileDlg_.Show("Select an attribute vault (.bin)", FileDialog::Mode::Open,
            { ".bin" }, [this](const std::filesystem::path& p) { LoadVault(p); });
    }
    if (!vaultPath_.empty()) {
        VanGui::SameLine();
        VanGui::TextDisabled("%s", vaultPath_.filename().string().c_str());
    }
    fileDlg_.Draw();

    if (!collections_.empty()) {
        VanGui::SameLine();
        const bool hasEdits = !edits_.empty() || !colEdits_.empty();
        if (!hasEdits) VanGui::BeginDisabled();
        if (VanGui::Button("Save changes")) SaveEdits();
        if (!hasEdits) VanGui::EndDisabled();
    }
    if (!status_.empty()) VanGui::TextDisabled("%s", status_.c_str());
}

void EffectsPanel::LoadVault(const std::filesystem::path& p) {
    edits_.clear(); colEdits_.clear(); collections_.clear(); selected_ = -1;
    auto r = VaultDecoder::Decode(p);
    if (!r) { status_ = "Load failed: " + r.error; return; }
    vaultPath_ = p;
    recs_ = std::move(r.value);
    RebuildCollections();
    char buf[160];
    std::snprintf(buf, sizeof buf, "Loaded %zu effect collections from %s",
                  collections_.size(), p.filename().string().c_str());
    status_ = buf;
}

void EffectsPanel::RebuildCollections() {
    collections_.clear();
    // Field hashes that belong to the Effects-group classes.
    std::unordered_set<uint32_t> fxFields;
    for (const auto& c : VaultSchemaCatalog())
        if (std::string(c.group) == "Effects")
            for (const auto& f : c.fields) fxFields.insert(f.hash);

    std::unordered_map<uint32_t, size_t> idx;
    for (auto& rec : recs_) {
        // Only records that carry at least one emitter/effect field.
        bool isFx = false;
        for (auto& t : rec.triples)
            if (fxFields.count(t.fieldHash)) { isFx = true; break; }
        if (!isFx) continue;

        auto it = idx.find(rec.keyHash);
        if (it == idx.end()) {
            idx[rec.keyHash] = collections_.size();
            FxCollection fc;
            fc.keyHash = rec.keyHash;
            if (!rec.keyName.empty()) fc.name = rec.keyName;
            else { char b[24]; std::snprintf(b, sizeof b, "effect 0x%08X", rec.keyHash); fc.name = b; }
            collections_.push_back(std::move(fc));
            it = idx.find(rec.keyHash);
        }
        FxCollection& fc = collections_[it->second];
        for (auto& t : rec.triples) {
            if (!fxFields.count(t.fieldHash)) continue;
            bool dup = false;
            for (const VaultTriple* ex : fc.fields)
                if (ex->fieldHash == t.fieldHash) { dup = true; break; }
            if (!dup) fc.fields.push_back(&t);
        }
    }
    std::sort(collections_.begin(), collections_.end(),
              [](const FxCollection& a, const FxCollection& b) { return a.name < b.name; });
}

void EffectsPanel::DrawList(float w) {
    (void)VanGui::BeginChild("##fxlist", VanVec2(w, 0), true);
    VanGui::SetNextItemWidth(-1.f);
    (void)VanGui::InputTextWithHint("##fxsearch", "Search effects\xe2\x80\xa6",
                                    search_, sizeof search_);
    VanGui::Separator();

    std::string flt = Lower(search_);
    for (int i = 0; i < (int)collections_.size(); ++i) {
        const auto& fc = collections_[i];
        if (!flt.empty() && Lower(fc.name).find(flt) == std::string::npos) continue;
        VanGui::PushID(i);
        if (VanGui::Selectable(fc.name.c_str(), selected_ == i)) selected_ = i;
        VanGui::PopID();
    }
    VanGui::EndChild();
}

void EffectsPanel::DrawPreviewBox(const std::string& effectName, float h) {
    (void)VanGui::BeginChild("##fxpreview", VanVec2(-1.f, h), true);
    const VanVec2 p0 = VanGui::GetWindowPos();
    const VanVec2 sz = VanGui::GetWindowSize();
    VanDrawList* dl = VanGui::GetWindowDrawList();
    const float cx = p0.x + sz.x * 0.5f;
    const float cy = p0.y + sz.y * 0.62f;   // emit from lower-middle
    const float t  = (float)VanGui::GetTime();
    EmitParticles(dl, cx, cy, FamilyOfName(effectName), effectName, t, 1.4f);
    VanGui::TextDisabled("live preview");
    VanGui::EndChild();
}

void EffectsPanel::DrawEditor(float w) {
    (void)VanGui::BeginChild("##fxeditor", VanVec2(w, 0), false);
    if (selected_ < 0 || selected_ >= (int)collections_.size()) {
        VanGui::TextDisabled("Select an effect to edit its parameters.");
        VanGui::EndChild();
        return;
    }
    const FxCollection& fc = collections_[selected_];
    VanGui::Text("%s", fc.name.c_str());
    VanGui::SameLine();
    VanGui::TextDisabled("(%s)", FxFamilyLabel(FamilyOfName(fc.name)));

    DrawPreviewBox(fc.name, 150.f);
    VanGui::Separator();

    for (const VaultTriple* t : fc.fields) {
        const VaultField* vf = FindVaultField(t->fieldHash);
        const char* type = vf ? vf->type : "Float";
        const std::string& name = t->fieldName;
        VanGui::PushID((int)t->fieldHash);

        if (std::strncmp(name.c_str(), "Color", 5) == 0) {
            // Packed RGBA/ARGB uint32 stored in the value's bytes.
            auto itc = colEdits_.find(t->valueOffset);
            uint32_t c = (itc != colEdits_.end()) ? itc->second : FloatToBits(t->value);
            float col[4] = { ((c >> 16) & 0xFF) / 255.f, ((c >> 8) & 0xFF) / 255.f,
                             (c & 0xFF) / 255.f, ((c >> 24) & 0xFF) / 255.f };
            VanGui::SetNextItemWidth(220.f);
            if (VanGui::ColorEdit4(name.c_str(), col)) {
                uint32_t nc = ((uint32_t)(col[3] * 255.f) << 24) |
                              ((uint32_t)(col[0] * 255.f) << 16) |
                              ((uint32_t)(col[1] * 255.f) << 8)  |
                               (uint32_t)(col[2] * 255.f);
                colEdits_[t->valueOffset] = nc;
            }
            if (itc != colEdits_.end()) { VanGui::SameLine(); VanGui::TextColored(VanVec4(0.98f,0.75f,0.30f,1.f), "*"); }
        } else if (std::strcmp(type, "Int32") == 0 || std::strcmp(type, "UInt32") == 0) {
            auto itc = colEdits_.find(t->valueOffset);
            int iv = (int)(itc != colEdits_.end() ? itc->second : FloatToBits(t->value));
            VanGui::SetNextItemWidth(140.f);
            if (VanGui::InputInt(name.c_str(), &iv)) colEdits_[t->valueOffset] = (uint32_t)iv;
            if (itc != colEdits_.end()) { VanGui::SameLine(); VanGui::TextColored(VanVec4(0.98f,0.75f,0.30f,1.f), "*"); }
        } else {   // Float (and unknown scalar) — the common case
            auto it = edits_.find(t->valueOffset);
            float v = (it != edits_.end()) ? it->second : t->value;
            VanGui::SetNextItemWidth(140.f);
            if (VanGui::DragFloat(name.c_str(), &v, 0.05f)) edits_[t->valueOffset] = v;
            if (it != edits_.end()) { VanGui::SameLine(); VanGui::TextColored(VanVec4(0.98f,0.75f,0.30f,1.f), "*"); }
        }
        VanGui::PopID();
    }
    VanGui::EndChild();
}

void EffectsPanel::SaveEdits() {
    if (vaultPath_.empty() || (edits_.empty() && colEdits_.empty())) return;
    std::vector<VaultDecoder::Edit> batch;
    batch.reserve(edits_.size() + colEdits_.size());
    for (const auto& [off, val] : edits_)    batch.push_back({ off, val });
    for (const auto& [off, bits] : colEdits_) batch.push_back({ off, BitsToFloat(bits) });
    auto r = VaultDecoder::PatchFloats(vaultPath_, batch, backup_);
    if (!r) { status_ = "Save failed: " + r.error; return; }
    char buf[160];
    std::snprintf(buf, sizeof buf, "Saved %d effect value(s) to %s (backup kept)",
                  r.value, vaultPath_.filename().string().c_str());
    status_ = buf;
    // Re-decode so offsets/values reflect the new file.
    LoadVault(vaultPath_);
}

} // namespace nfsmw
