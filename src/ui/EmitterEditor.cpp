// ─── ui/EmitterEditor.cpp ─────────────────────────────────────────────────────
#include "ui/EmitterEditor.h"
#include "ui/EffectPreview.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <nlohmann/json.hpp>

namespace nfsmw {

std::vector<EmitterAttachment>* EmitterEditor::Active() {
    if (!hasActive_) return nullptr;
    return &byName_[activeName_];
}

void EmitterEditor::Clear() {
    byName_.clear();
    hasActive_ = false;
    activeName_.clear();
    status_.clear();
}

void EmitterEditor::SetSubject(const std::string& name, float cx, float cy, float cz) {
    activeName_ = name;
    hasActive_  = !name.empty();
    defCtr_[0] = cx; defCtr_[1] = cy; defCtr_[2] = cz;
}

void EmitterEditor::DrawPanel() {
    VanGui::Separator();
    VanGui::Text("Emitters / FX");
    VanGui::SameLine();
    VanGui::TextDisabled("(attach smoke, lights, birds, fog\xe2\x80\xa6)");

    std::vector<EmitterAttachment>* list = Active();
    if (!list) { VanGui::TextDisabled("    select a part first."); return; }

    // ── Add from the catalogue ────────────────────────────────────────────────
    const auto& fams = EmitterFamilies();
    if (addFamily_ < 0 || addFamily_ >= (int)fams.size()) addFamily_ = 0;
    VanGui::SetNextItemWidth(150.f);
    if (VanGui::BeginCombo("##fxfam", FxFamilyLabel(fams[addFamily_]))) {
        for (int i = 0; i < (int)fams.size(); ++i)
            if (VanGui::Selectable(FxFamilyLabel(fams[i]), i == addFamily_)) addFamily_ = i;
        VanGui::EndCombo();
    }
    VanGui::SameLine();
    VanGui::SetNextItemWidth(140.f);
    (void)VanGui::InputTextWithHint("##fxsearch", "search\xe2\x80\xa6", search_, sizeof search_);

    std::string flt = search_;
    std::transform(flt.begin(), flt.end(), flt.begin(),
                   [](unsigned char c){ return (char)std::tolower(c); });
    auto match = [&](const char* s) {
        if (flt.empty()) return true;
        std::string l = s;
        std::transform(l.begin(), l.end(), l.begin(),
                       [](unsigned char c){ return (char)std::tolower(c); });
        return l.find(flt) != std::string::npos;
    };

    if (VanGui::BeginCombo("##fxadd", "Add emitter\xe2\x80\xa6")) {
        for (const EmitterDef* e : EmittersInFamily(fams[addFamily_])) {
            if (!match(e->effect) && !match(e->desc)) continue;
            char lbl[160];
            std::snprintf(lbl, sizeof lbl, "%s  \xe2\x80\x94  %s", e->effect, e->desc);
            if (VanGui::Selectable(lbl)) {
                EmitterAttachment a;
                a.effect  = e->effect;
                a.emitter = e->emitter ? e->emitter : "";
                a.pos[0] = defCtr_[0]; a.pos[1] = defCtr_[1]; a.pos[2] = defCtr_[2];
                list->push_back(std::move(a));
                dirty_ = true;
            }
        }
        VanGui::EndCombo();
    }

    // ── Current attachments ───────────────────────────────────────────────────
    if (list->empty()) {
        VanGui::TextDisabled("    no emitters attached yet.");
    } else {
        int removeIdx = -1;
        for (int i = 0; i < (int)list->size(); ++i) {
            EmitterAttachment& a = (*list)[i];
            VanGui::PushID(i);
            bool en = a.enabled;
            if (VanGui::Checkbox("##en", &en)) { a.enabled = en; dirty_ = true; }
            VanGui::SameLine();
            if (VanGui::TreeNodeEx("##fx", VanGuiTreeNodeFlags_DefaultOpen, "%s", a.effect.c_str())) {
                VanGui::SetNextItemWidth(220.f);
                if (VanGui::DragFloat3("position", a.pos, 0.05f)) dirty_ = true;
                VanGui::SetNextItemWidth(220.f);
                if (VanGui::DragFloat3("rotation\xc2\xb0", a.rot, 0.5f)) dirty_ = true;
                VanGui::SetNextItemWidth(120.f);
                if (VanGui::DragFloat("scale", &a.scale, 0.01f, 0.01f, 100.f)) dirty_ = true;
                if (!a.emitter.empty()) VanGui::TextDisabled("emitter: %s", a.emitter.c_str());
                if (VanGui::SmallButton("Remove")) removeIdx = i;
                VanGui::TreePop();
            }
            VanGui::PopID();
        }
        if (removeIdx >= 0) { list->erase(list->begin() + removeIdx); dirty_ = true; }
    }

    VanGui::Spacing();
    const bool canSave = dirty_ && !sidecar_.empty();
    if (!canSave) VanGui::BeginDisabled();
    if (VanGui::Button("Save emitters")) Save();
    if (!canSave) VanGui::EndDisabled();
    VanGui::SameLine();
    if (VanGui::Button("Reload")) Load();
    VanGui::SameLine();
    (void)VanGui::Checkbox("Show markers", &showMarkers);
    if (!status_.empty()) VanGui::TextDisabled("%s", status_.c_str());
}

void EmitterEditor::DrawOverlay(VanDrawList* dl, const glm::mat4& mvp,
                                float imgX, float imgY, float imgW, float imgH) {
    if (!showMarkers || !hasActive_) return;
    auto it = byName_.find(activeName_);
    if (it == byName_.end()) return;
    const float tNow = (float)VanGui::GetTime();

    // Project a world point to screen; returns false if behind the camera.
    auto project = [&](const glm::vec3& wp, float& ox, float& oy) -> bool {
        glm::vec4 c = mvp * glm::vec4(wp, 1.0f);
        if (c.w <= 0.0001f) return false;
        ox = imgX + (c.x / c.w * 0.5f + 0.5f) * imgW;
        oy = imgY + (1.0f - (c.y / c.w * 0.5f + 0.5f)) * imgH;
        return true;
    };

    for (const auto& a : it->second) {
        const glm::vec3 P(a.pos[0], a.pos[1], a.pos[2]);
        float sx, sy;
        if (!project(P, sx, sy)) continue;

        if (a.enabled) {
            // Pixels-per-world-unit at this depth → particles size to the view
            // (true scale, zoom-responsive) rather than a fixed pixel blob.
            float ax = 0.f, ay = 0.f, bx = 0.f, by = 0.f, pxu = 60.f;
            if (project(P + glm::vec3(1, 0, 0), ax, ay) &&
                project(P + glm::vec3(0, 1, 0), bx, by)) {
                const float dX = std::sqrt((ax - sx) * (ax - sx) + (ay - sy) * (ay - sy));
                const float dY = std::sqrt((bx - sx) * (bx - sx) + (by - sy) * (by - sy));
                pxu = 0.5f * (dX + dY);
            }
            const float user = std::min(std::max(a.scale, 0.05f), 20.f);
            const float sc   = std::min(std::max(pxu * 0.006f * user, 0.30f), 6.0f);
            const EmitterDef* ed = FindEmitter(a.effect);
            EmitParticles(dl, sx, sy, ed ? ed->family : FxFamily::Other, a.effect, tNow, sc);
        }

        const auto col = a.enabled ? VAN_COL32(120, 210, 255, 255)
                                   : VAN_COL32(150, 150, 150, 180);
        dl->AddCircleFilled(VanVec2(sx, sy), 4.0f, col);
        dl->AddCircle(VanVec2(sx, sy), 7.0f, col, 0, 1.5f);
        dl->AddText(VanVec2(sx + 8.0f, sy - 7.0f), col, a.effect.c_str());
    }
}

void EmitterEditor::Save() {
    if (sidecar_.empty()) return;
    nlohmann::json j;
    j["subjects"] = nlohmann::json::object();
    for (const auto& [name, atts] : byName_) {
        if (atts.empty()) continue;
        nlohmann::json arr = nlohmann::json::array();
        for (const auto& a : atts)
            arr.push_back({ {"effect", a.effect}, {"emitter", a.emitter},
                            {"pos", { a.pos[0], a.pos[1], a.pos[2] }},
                            {"rot", { a.rot[0], a.rot[1], a.rot[2] }},
                            {"scale", a.scale}, {"enabled", a.enabled} });
        j["subjects"][name] = std::move(arr);
    }
    std::ofstream f(sidecar_, std::ios::trunc);
    if (!f) { status_ = "Could not write sidecar."; return; }
    f << j.dump(2);
    dirty_ = false;
    status_ = "Saved emitters.";
}

void EmitterEditor::Load() {
    byName_.clear();
    dirty_ = false;
    if (sidecar_.empty()) return;
    std::ifstream f(sidecar_);
    if (!f) return;

    nlohmann::json j;
    try { f >> j; } catch (...) { status_ = "Emitter sidecar is not valid JSON."; return; }
    if (!j.contains("subjects")) return;

    int n = 0;
    for (auto& [name, arr] : j["subjects"].items()) {
        auto& dst = byName_[name];
        for (auto& e : arr) {
            EmitterAttachment a;
            a.effect  = e.value("effect", std::string());
            a.emitter = e.value("emitter", std::string());
            a.scale   = e.value("scale", 1.f);
            a.enabled = e.value("enabled", true);
            if (e.contains("pos") && e["pos"].size() == 3)
                for (int k = 0; k < 3; ++k) a.pos[k] = e["pos"][k].get<float>();
            if (e.contains("rot") && e["rot"].size() == 3)
                for (int k = 0; k < 3; ++k) a.rot[k] = e["rot"][k].get<float>();
            dst.push_back(std::move(a));
            ++n;
        }
    }
    status_ = "Loaded " + std::to_string(n) + " emitter(s).";
}

} // namespace nfsmw
