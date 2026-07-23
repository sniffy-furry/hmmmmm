// vangui_vui.cpp — see vangui_vui.h. Empty TU unless enabled.
#include "vangui_vui.h"

#ifdef VANGUI_ENABLE_VUI

#include "vangui_vui_parser.inl"   // Detail::VuiNode, Detail::ParseVui
#include <cstdio>
#include <sys/stat.h>

static std::FILE* open_file(const char* path, const char* mode)
{
#if defined(_MSC_VER)
    std::FILE* f = nullptr;
    fopen_s(&f, path, mode);
    return f;
#else
    return std::fopen(path, mode);
#endif
}

namespace VanGui {

struct VanVuiDoc::Impl { Detail::VuiNode root; bool ok = false; };

VanVuiDoc::VanVuiDoc() : impl(std::make_unique<Impl>()) {}
VanVuiDoc::~VanVuiDoc() = default;
VanVuiDoc::VanVuiDoc(VanVuiDoc&&) noexcept = default;
VanVuiDoc& VanVuiDoc::operator=(VanVuiDoc&&) noexcept = default;
bool VanVuiDoc::loaded() const { return impl && impl->ok; }

bool LoadVuiFromMemory(const char* text, size_t len, VanVuiDoc& out, const char** err)
{
    if (!out.impl) out.impl = std::make_unique<VanVuiDoc::Impl>();
    out.impl->ok = Detail::ParseVui(text, len, out.impl->root, err);
    return out.impl->ok;
}

bool LoadVui(const char* path, VanVuiDoc& out, const char** err)
{
    std::FILE* f = open_file(path, "rb");
    if (!f) { if (err) *err = "could not open .vui file"; return false; }
    std::fseek(f, 0, SEEK_END); long n = std::ftell(f); std::fseek(f, 0, SEEK_SET);
    if (n < 0) { std::fclose(f); if (err) *err = "read error"; return false; }
    std::string buf((size_t)n, '\0');
    size_t rd = std::fread(buf.data(), 1, (size_t)n, f); std::fclose(f);
    buf.resize(rd);
    return LoadVuiFromMemory(buf.c_str(), buf.size(), out, err);
}

// --- rendering --------------------------------------------------------------
namespace {

void RenderNode(const Detail::VuiNode& n, VanVuiContext& ctx);

void RenderChildren(const Detail::VuiNode& n, VanVuiContext& ctx, bool horizontal)
{
    for (size_t i = 0; i < n.kids.size(); ++i)
    {
        if (horizontal && i > 0) SameLine();
        RenderNode(n.kids[i], ctx);
    }
}

const char* Lbl(const Detail::VuiNode& n) { return n.label.empty() ? n.type.c_str() : n.label.c_str(); }

void RenderNode(const Detail::VuiNode& n, VanVuiContext& ctx)
{
    const std::string& t = n.type;

    if (t == "Window")
    {
        if (Begin(Lbl(n))) RenderChildren(n, ctx, false);
        End();
    }
    else if (t == "Child")
    {
        const float h = n.attrf("height", 0.0f);
        if (BeginChild(n.id.empty() ? Lbl(n) : n.id.c_str(), VanVec2(0, h), VanGuiChildFlags_Borders))
            RenderChildren(n, ctx, false);
        EndChild();
    }
    else if (t == "Row")    { BeginGroup(); RenderChildren(n, ctx, true);  EndGroup(); }
    else if (t == "Column" || t == "Group") { BeginGroup(); RenderChildren(n, ctx, false); EndGroup(); }
    else if (t == "Heading") SeparatorText(Lbl(n));
    else if (t == "Text")    TextUnformatted(Lbl(n));
    else if (t == "Separator") Separator();
    else if (t == "Spacing")   Spacing();
    else if (t == "Button")
    {
        PushID(n.id.empty() ? Lbl(n) : n.id.c_str());
        if (Button(Lbl(n))) { if (!n.id.empty()) ctx.clicked_.push_back(n.id); }
        PopID();
    }
    else if (t == "Checkbox")
    {
        if (bool* p = (bool*)ctx.find(n.attr("bind"), VanVuiContext::Kind::B))
            (void)Checkbox(Lbl(n), p);
        else { bool tmp = false; (void)Checkbox(Lbl(n), &tmp); }
    }
    else if (t == "Slider")
    {
        const float lo = n.attrf("min", 0.0f), hi = n.attrf("max", 1.0f);
        if (float* p = (float*)ctx.find(n.attr("bind"), VanVuiContext::Kind::F))
            (void)SliderFloat(Lbl(n), p, lo, hi);
        else if (int* pi = (int*)ctx.find(n.attr("bind"), VanVuiContext::Kind::I))
            (void)SliderInt(Lbl(n), pi, (int)lo, (int)hi);
        else { float tmp = lo; (void)SliderFloat(Lbl(n), &tmp, lo, hi); }
    }
    else
    {
        // Unknown node: render its children so unsupported wrappers degrade gracefully.
        RenderChildren(n, ctx, false);
    }
}

} // anonymous

void RenderVui(VanVuiDoc& doc, VanVuiContext& ctx)
{
    if (!doc.impl || !doc.impl->ok) return;
    ctx.clicked_.clear();
    for (auto& top : doc.impl->root.kids)
        RenderNode(top, ctx);
}

// --- hot reload -------------------------------------------------------------
namespace {
struct Watch { std::string path; long long mtime; VanVuiDoc* doc; bool active; };
static Watch s_watch{ "", 0, nullptr, false };
long long MTime(const char* p) { struct stat st; return (stat(p, &st) == 0) ? (long long)st.st_mtime : 0; }
}

void WatchVui(const char* path, VanVuiDoc& doc)
{
    s_watch = { path ? path : "", path ? MTime(path) : 0, &doc, path != nullptr };
}
void PollVuiChanges()
{
    if (!s_watch.active || !s_watch.doc) return;
    const long long m = MTime(s_watch.path.c_str());
    if (m != 0 && m != s_watch.mtime) { s_watch.mtime = m; (void)LoadVui(s_watch.path.c_str(), *s_watch.doc, nullptr); }
}

} // namespace VanGui

#endif // VANGUI_ENABLE_VUI
