#pragma once
// ─── ui/EmitterEditor.h ───────────────────────────────────────────────────────
// Reusable emitter/FX-attachment authoring shared by the object and car viewers:
// a catalogue picker, per-attachment placement (position / rotation / scale),
// a live markers + looping-particles overlay, and a JSON sidecar. Manages
// attachments for many "subjects" (objects or car parts) keyed by name, so one
// editor + one sidecar covers a whole bundle or car.
// ─────────────────────────────────────────────────────────────────────────────
#include "formats/EmitterCatalog.h"
#include <glm/glm.hpp>
#include <vangui.h>
#include <cstdint>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

namespace nfsmw {

/// One emitter/FX attached to a subject, with an editable placement (offset from
/// the subject origin, in game/local space), rotation and scale.
struct EmitterAttachment {
    std::string effect;
    std::string emitter;
    float       pos[3]  = { 0.f, 0.f, 0.f };
    float       rot[3]  = { 0.f, 0.f, 0.f };   // euler degrees
    float       scale   = 1.f;
    bool        enabled = true;
};

class EmitterEditor {
public:
    /// Where attachments persist (usually "<file>.fx.json" or "<car>/<id>.fx.json").
    void SetSidecar(const std::filesystem::path& p) { sidecar_ = p; }

    /// Drop all attachments + reset (e.g. when closing a file).
    void Clear();

    /// Read / write the sidecar.
    void Load();
    void Save();

    /// Point the editor at a subject (object/part) by name; `cx,cy,cz` is where a
    /// newly-added emitter is placed by default (usually the subject's centre).
    void SetSubject(const std::string& name, float cx, float cy, float cz);

    bool Empty() const { return byName_.empty(); }
    bool Dirty() const { return dirty_; }

    /// Draw the authoring UI for the active subject (catalogue picker, list,
    /// placement, save/reload, marker toggle).
    void DrawPanel();

    /// Project & draw markers + looping particles for the active subject onto
    /// the image at (imgX,imgY) sized imgW×imgH. `mvp` is proj*view*model.
    void DrawOverlay(VanDrawList* dl, const glm::mat4& mvp,
                     float imgX, float imgY, float imgW, float imgH);

    bool showMarkers = true;

private:
    std::filesystem::path                                          sidecar_;
    std::unordered_map<std::string, std::vector<EmitterAttachment>> byName_;
    std::string activeName_;
    bool        hasActive_  = false;
    float       defCtr_[3]  = { 0.f, 0.f, 0.f };

    int         addFamily_  = 0;
    char        search_[64] = {0};
    bool        dirty_      = false;
    std::string status_;

    std::vector<EmitterAttachment>* Active();
};

} // namespace nfsmw
