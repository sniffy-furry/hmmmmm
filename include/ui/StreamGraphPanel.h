#pragma once
// ─── ui/StreamGraphPanel.h ───────────────────────────────────────────────────
// Phase 8 (VanGUI Overhaul) — visual stream-bundle dependency graph.
//
// Each StreamSection becomes a node; its texture packs and solid lists surface
// as output pins, and texture-pack pins link into a shared "Texture Packs" sink
// node so cross-references are visible at a glance. Built on vangui_node_graph.
// Non-critical viewer — decoupled from the rest of the shell.
// ─────────────────────────────────────────────────────────────────────────────
#include "formats/StreamBundle.h"

#include <vangui.h>
#include <vangui_node_graph.h>

#include <string>
#include <vector>

namespace nfsmw {

class StreamGraphPanel {
public:
    StreamGraphPanel();
    ~StreamGraphPanel();

    StreamGraphPanel(const StreamGraphPanel&)            = delete;
    StreamGraphPanel& operator=(const StreamGraphPanel&) = delete;

    /// Rebuild the graph from parsed stream sections.
    void SetSections(const std::vector<StreamSection>& sections);

    /// Draw the node canvas filling a (w, h) region. Pass (0,0) to autofill.
    void Draw(float w, float h);

    [[nodiscard]] bool HasData() const { return !nodes_.empty(); }

private:
    struct PinDef  { int id; std::string label; bool output; };
    struct NodeDef { int id; std::string title; VanVec2 pos; std::vector<PinDef> pins; };
    struct LinkDef { int fromNode; int fromPin; int toNode; int toPin; };

    VanGui::VanNodeGraphContext* ctx_ = nullptr;
    std::vector<NodeDef>         nodes_;
    std::vector<LinkDef>         links_;
    int                          nextId_ = 1;
};

} // namespace nfsmw
