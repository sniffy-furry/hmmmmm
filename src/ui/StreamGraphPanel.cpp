// ─── ui/StreamGraphPanel.cpp ─────────────────────────────────────────────────
// Phase 8 — stream-bundle dependency graph (see header). Built on
// VanGui::*NodeGraph from vendor/VanGUI/misc/vangui_node_graph.h.
// ─────────────────────────────────────────────────────────────────────────────
#include "ui/StreamGraphPanel.h"

namespace nfsmw {

StreamGraphPanel::StreamGraphPanel()  { ctx_ = VanGui::CreateNodeGraphContext(); }
StreamGraphPanel::~StreamGraphPanel() { if (ctx_) VanGui::DestroyNodeGraphContext(ctx_); }

void StreamGraphPanel::SetSections(const std::vector<StreamSection>& sections) {
    nodes_.clear();
    links_.clear();
    nextId_ = 1;

    // Shared sink node that every section's texture-pack pin links into.
    NodeDef texSink;
    texSink.id    = nextId_++;
    texSink.title = "Texture Packs";
    texSink.pos   = VanVec2(560.0f, 40.0f);
    const int texInPin = nextId_++;
    texSink.pins.push_back({ texInPin, "in", false });

    float y = 40.0f;
    for (const auto& s : sections) {
        NodeDef n;
        n.id    = nextId_++;
        n.title = s.name.empty() ? ("section_" + std::to_string(s.number)) : s.name;
        n.pos   = VanVec2(60.0f, y);
        y += 130.0f;

        n.pins.push_back({ nextId_++, "geom", false });   // input

        if (!s.texturePacks.empty()) {
            const int outPin = nextId_++;
            n.pins.push_back({ outPin,
                               "tex (" + std::to_string(s.texturePacks.size()) + ")",
                               true });
            links_.push_back({ n.id, outPin, texSink.id, texInPin });
        }
        if (!s.solidLists.empty()) {
            n.pins.push_back({ nextId_++,
                               "solids (" + std::to_string(s.solidLists.size()) + ")",
                               true });
        }

        nodes_.push_back(std::move(n));
    }
    nodes_.push_back(std::move(texSink));
}

void StreamGraphPanel::Draw(float w, float h) {
    if (nodes_.empty()) {
        VanGui::TextDisabled("Open a stream bundle to visualise its sections.");
        return;
    }

    if (VanGui::BeginNodeGraph("##streamgraph", ctx_, VanVec2(w, h))) {
        for (auto& n : nodes_) {
            VanGui::BeginNode(n.id, &n.pos);
            VanGui::NodeTitle(n.title.c_str());
            for (auto& p : n.pins)
                VanGui::NodePin(p.id, p.output, p.label.c_str());
            VanGui::EndNode();
        }
        for (const auto& l : links_)
            VanGui::NodeLink(l.fromNode, l.fromPin, l.toNode, l.toPin);

        VanGui::EndNodeGraph();
    }
}

} // namespace nfsmw
