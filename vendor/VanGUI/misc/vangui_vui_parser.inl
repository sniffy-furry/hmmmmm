// vangui_vui_parser.inl
// Pure parser for the .vui declarative UI format. No VanGUI runtime calls, so it
// is unit-testable in isolation. All symbols are `static`.
//
// Grammar (whitespace-insensitive; // and /* */ comments):
//   Node      := Type ['#' id] ['"' label '"'] {key '=' value} ['{' Node* '}']
//   value     := bareword | number | '"' string '"'
// Example:
//   Window "Settings" {
//     Heading "Display"
//     Row { Button#save "Save"  Button "Cancel" }
//     Checkbox "V-Sync" bind=vsync
//     Slider "Volume" bind=volume min=0 max=100
//   }

#include <string>
#include <vector>
#include <cstring>
#include <cstdio>

namespace VanGui {
namespace Detail {

struct VuiNode
{
    std::string type, id, label;
    std::vector<std::pair<std::string, std::string>> attrs;
    std::vector<VuiNode> kids;

    const char* attr(const char* k, const char* def = "") const
    {
        for (auto& a : attrs) if (a.first == k) return a.second.c_str();
        return def;
    }
    bool has(const char* k) const
    {
        for (auto& a : attrs) if (a.first == k) return true;
        return false;
    }
    float attrf(const char* k, float def = 0.0f) const { return has(k) ? (float)std::atof(attr(k)) : def; }
    int   attri(const char* k, int def = 0) const       { return has(k) ? std::atoi(attr(k)) : def; }
};

namespace vui {

struct P { const char* p; const char* end; int line; char err[160]; bool failed; };

inline void Fail(P& s, const char* m) { if (!s.failed) std::snprintf(s.err, sizeof s.err, "line %d: %s", s.line, m); s.failed = true; }

inline void Skip(P& s)
{
    while (s.p < s.end)
    {
        const char c = *s.p;
        if (c == '\n') { ++s.line; ++s.p; }
        else if (c == ' ' || c == '\t' || c == '\r') ++s.p;
        else if (c == '/' && s.p + 1 < s.end && s.p[1] == '/') { while (s.p < s.end && *s.p != '\n') ++s.p; }
        else if (c == '/' && s.p + 1 < s.end && s.p[1] == '*') { s.p += 2; while (s.p + 1 < s.end && !(s.p[0]=='*'&&s.p[1]=='/')) { if (*s.p=='\n') ++s.line; ++s.p; } s.p += 2; }
        else break;
    }
}

inline bool IsIdent(char c) { return (c>='A'&&c<='Z')||(c>='a'&&c<='z')||(c>='0'&&c<='9')||c=='_'||c=='-'||c=='.'; }

inline std::string ReadIdent(P& s)
{
    std::string out;
    while (s.p < s.end && IsIdent(*s.p)) out.push_back(*s.p++);
    return out;
}

inline std::string ReadString(P& s)
{
    std::string out;
    ++s.p; // opening quote
    while (s.p < s.end && *s.p != '"') { if (*s.p == '\n') ++s.line; out.push_back(*s.p++); }
    if (s.p < s.end) ++s.p; // closing quote
    return out;
}

inline std::string ReadValue(P& s)
{
    Skip(s);
    if (s.p < s.end && *s.p == '"') return ReadString(s);
    std::string out;
    while (s.p < s.end && (IsIdent(*s.p) || *s.p=='%' || *s.p=='#')) out.push_back(*s.p++);
    return out;
}

inline bool ParseNode(P& s, VuiNode& node);

inline void ParseBlock(P& s, VuiNode& parent)
{
    ++s.p; // '{'
    while (true)
    {
        Skip(s);
        if (s.p >= s.end) { Fail(s, "unterminated '{'"); return; }
        if (*s.p == '}') { ++s.p; return; }
        VuiNode child;
        if (!ParseNode(s, child)) return;
        parent.kids.push_back(std::move(child));
        if (s.failed) return;
    }
}

inline bool ParseNode(P& s, VuiNode& node)
{
    Skip(s);
    node.type = ReadIdent(s);
    if (node.type.empty()) { Fail(s, "expected a node type"); return false; }

    if (s.p < s.end && *s.p == '#') { ++s.p; node.id = ReadIdent(s); }

    Skip(s);
    if (s.p < s.end && *s.p == '"') node.label = ReadString(s);

    // attributes: key=value pairs (lookahead to avoid eating the next sibling)
    while (true)
    {
        Skip(s);
        if (s.p >= s.end || *s.p == '{' || *s.p == '}') break;
        const char* save = s.p; const int saveLine = s.line;
        std::string key = ReadIdent(s);
        Skip(s);
        if (!key.empty() && s.p < s.end && *s.p == '=')
        {
            ++s.p;
            std::string val = ReadValue(s);
            node.attrs.push_back({ key, val });
        }
        else { s.p = save; s.line = saveLine; break; }   // it's the next node, not an attr
    }

    Skip(s);
    if (s.p < s.end && *s.p == '{') ParseBlock(s, node);
    return !s.failed;
}

} // namespace vui

// Parse a whole document; root.kids holds the top-level nodes.
static bool ParseVui(const char* text, size_t len, VuiNode& root, const char** err)
{
    static char s_err[160];
    vui::P s{ text, text + len, 1, "", false };
    root = VuiNode{};
    while (true)
    {
        vui::Skip(s);
        if (s.p >= s.end) break;
        VuiNode n;
        if (!vui::ParseNode(s, n)) break;
        root.kids.push_back(std::move(n));
        if (s.failed) break;
    }
    if (s.failed) { std::snprintf(s_err, sizeof s_err, "%s", s.err); if (err) *err = s_err; return false; }
    if (err) *err = nullptr;
    return true;
}

} // namespace Detail
} // namespace VanGui
