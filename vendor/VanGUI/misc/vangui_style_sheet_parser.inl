// vangui_style_sheet_parser.inl
// -----------------------------------------------------------------------------
// Pure parser core for the .vss stylesheet format. Contains NO calls into the
// VanGui runtime — it only consumes text and produces a ParsedSheet of resolved
// colors / numbers. This makes it unit-testable in isolation (see the test in
// docs / Test). The applying side lives in vangui_style_sheet.cpp.
//
// All symbols are `static` so this .inl can be included by both the module TU
// and a standalone test TU without ODR conflicts.
// -----------------------------------------------------------------------------

#include "vangui_theme_engine.h"   // VanThemeToken, VanThemeTokenSet
#include <cstring>
#include <cstdlib>
#include <cstdio>

namespace VanGui {
namespace Detail {

struct ParsedSheet
{
    VanThemeTokenSet tokens;
    bool   hasToken[VanThemeToken_COUNT];
    float  frameRounding;   // <0 = unset
    float  windowRounding;  // <0 = unset
    float  framePaddingX;   // <0 = unset
    float  framePaddingY;   // <0 = unset
};

static void ParsedSheetInit(ParsedSheet& s)
{
    for (int i = 0; i < VanThemeToken_COUNT; ++i) s.hasToken[i] = false;
    s.frameRounding = s.windowRounding = s.framePaddingX = s.framePaddingY = -1.0f;
}

// --- value model ------------------------------------------------------------

enum ValKind { Val_None, Val_Color, Val_Number };
struct Value
{
    ValKind kind = Val_None;
    VanVec4 color = VanVec4(0,0,0,1);
    float   number = 0.0f;
    bool    percent = false;
};

struct VarEntry { char name[64]; Value val; };

// --- parser state -----------------------------------------------------------

struct Parser
{
    const char* p;
    const char* end;
    int         line;
    VarEntry    vars[64];
    int         varCount;
    char        err[160];
    bool        failed;

    void Fail(const char* msg)
    {
        if (!failed)
            std::snprintf(err, sizeof(err), "line %d: %s", line, msg);
        failed = true;
    }
};

static float Clamp01f(float v) { return v < 0 ? 0 : (v > 1 ? 1 : v); }

static int HexNyb(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

// Parse #RGB / #RRGGBB / #RRGGBBAA starting after '#'. Returns false on bad hex.
static bool ParseHexColor(const char* s, int n, VanVec4& out)
{
    auto two = [](int hi, int lo) { return (hi * 16 + lo) / 255.0f; };
    if (n == 3)
    {
        int r = HexNyb(s[0]), g = HexNyb(s[1]), b = HexNyb(s[2]);
        if ((r|g|b) < 0) return false;
        out = VanVec4(two(r,r), two(g,g), two(b,b), 1.0f);
        return true;
    }
    if (n == 6 || n == 8)
    {
        int v[8];
        for (int i = 0; i < n; ++i) { v[i] = HexNyb(s[i]); if (v[i] < 0) return false; }
        out.x = two(v[0], v[1]);
        out.y = two(v[2], v[3]);
        out.z = two(v[4], v[5]);
        out.w = (n == 8) ? two(v[6], v[7]) : 1.0f;
        return true;
    }
    return false;
}

static void SkipWs(Parser& ps)
{
    while (ps.p < ps.end)
    {
        const char c = *ps.p;
        if (c == '\n') { ++ps.line; ++ps.p; }
        else if (c == ' ' || c == '\t' || c == '\r') { ++ps.p; }
        else if (c == '/' && ps.p + 1 < ps.end && ps.p[1] == '*')
        {
            ps.p += 2;
            while (ps.p + 1 < ps.end && !(ps.p[0] == '*' && ps.p[1] == '/'))
            { if (*ps.p == '\n') ++ps.line; ++ps.p; }
            ps.p += 2;
        }
        else break;
    }
}

static bool IsIdentChar(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
           (c >= '0' && c <= '9') || c == '_' || c == '-';
}

// Selectors may additionally contain ':' (e.g. ":root", "Button:hover").
static bool IsSelectorChar(char c) { return IsIdentChar(c) || c == ':'; }

// Read an identifier/selector/property token into buf.
static int ReadIdent(Parser& ps, char* buf, int cap)
{
    int n = 0;
    while (ps.p < ps.end && IsIdentChar(*ps.p) && n < cap - 1)
        buf[n++] = *ps.p++;
    buf[n] = '\0';
    return n;
}

static int ReadSelector(Parser& ps, char* buf, int cap)
{
    int n = 0;
    while (ps.p < ps.end && IsSelectorChar(*ps.p) && n < cap - 1)
        buf[n++] = *ps.p++;
    buf[n] = '\0';
    return n;
}

static Value* FindVar(Parser& ps, const char* name)
{
    for (int i = 0; i < ps.varCount; ++i)
        if (std::strcmp(ps.vars[i].name, name) == 0) return &ps.vars[i].val;
    return nullptr;
}

static void SetVar(Parser& ps, const char* name, const Value& v)
{
    if (Value* e = FindVar(ps, name)) { *e = v; return; }
    if (ps.varCount < 64)
    {
        std::snprintf(ps.vars[ps.varCount].name, sizeof(ps.vars[ps.varCount].name), "%s", name);
        ps.vars[ps.varCount].val = v;
        ++ps.varCount;
    }
}

static VanVec4 ShadeColor(VanVec4 c, float amount, bool lighten)
{
    const float a = lighten ? amount : -amount;
    return VanVec4(Clamp01f(c.x + a), Clamp01f(c.y + a), Clamp01f(c.z + a), c.w);
}

// Parse a single value (after the ':'), up to ';' or '}'.
static Value ParseValue(Parser& ps)
{
    SkipWs(ps);
    Value v;
    if (ps.p >= ps.end) { ps.Fail("unexpected end of value"); return v; }

    if (*ps.p == '#')
    {
        ++ps.p;
        const char* s = ps.p;
        int n = 0;
        while (ps.p < ps.end && HexNyb(*ps.p) >= 0) { ++ps.p; ++n; }
        if (!ParseHexColor(s, n, v.color)) { ps.Fail("invalid hex color"); return v; }
        v.kind = Val_Color;
        return v;
    }
    if (*ps.p == '$')
    {
        ++ps.p;
        char name[64]; ReadIdent(ps, name, sizeof(name));
        Value* ref = FindVar(ps, name);
        if (!ref) { ps.Fail("unknown variable"); return v; }
        return *ref;
    }
    // function: lighten(...) / darken(...)
    if ((ps.end - ps.p) >= 7 &&
        (std::strncmp(ps.p, "lighten", 7) == 0 || std::strncmp(ps.p, "darken", 6) == 0))
    {
        const bool lighten = (*ps.p == 'l');
        ps.p += lighten ? 7 : 6;
        SkipWs(ps);
        if (ps.p >= ps.end || *ps.p != '(') { ps.Fail("expected '(' after function"); return v; }
        ++ps.p;
        Value base = ParseValue(ps);          // recursive (handles $var / #hex)
        SkipWs(ps);
        if (ps.p >= ps.end || *ps.p != ',') { ps.Fail("expected ',' in function"); return v; }
        ++ps.p;
        Value amt = ParseValue(ps);
        SkipWs(ps);
        if (ps.p >= ps.end || *ps.p != ')') { ps.Fail("expected ')' after function"); return v; }
        ++ps.p;
        if (base.kind != Val_Color) { ps.Fail("function expects a color"); return v; }
        const float frac = amt.percent ? amt.number / 100.0f : amt.number;
        v.kind = Val_Color;
        v.color = ShadeColor(base.color, frac, lighten);
        return v;
    }
    // number (optionally with px / %)
    {
        char* endp = nullptr;
        const float num = std::strtof(ps.p, &endp);
        if (endp == ps.p) { ps.Fail("expected a value"); return v; }
        ps.p = endp;
        v.kind = Val_Number;
        v.number = num;
        if (ps.p < ps.end && *ps.p == '%') { v.percent = true; ++ps.p; }
        else if ((ps.end - ps.p) >= 2 && ps.p[0] == 'p' && ps.p[1] == 'x') ps.p += 2;
        return v;
    }
}

// Token-name -> enum, or -1.
static int TokenFromName(const char* n)
{
    struct M { const char* k; int v; };
    static const M map[] = {
        {"background", VanThemeToken_Background}, {"surface", VanThemeToken_Surface},
        {"border", VanThemeToken_Border},         {"primary", VanThemeToken_Primary},
        {"secondary", VanThemeToken_Secondary},   {"text", VanThemeToken_TextPrimary},
        {"text-primary", VanThemeToken_TextPrimary}, {"text-dim", VanThemeToken_TextDim},
        {"error", VanThemeToken_Error},           {"warning", VanThemeToken_Warning},
        {"success", VanThemeToken_Success},       {"info", VanThemeToken_Info},
        {"overlay", VanThemeToken_Overlay},
    };
    for (const M& m : map) if (std::strcmp(m.k, n) == 0) return m.v;
    return -1;
}

// Apply one "prop: value" declaration inside a selector block.
static void ApplyDecl(Parser& ps, ParsedSheet& sheet, const char* selector,
                      const char* prop, const Value& val)
{
    const bool root = (std::strcmp(selector, ":root") == 0);

    if (root)
    {
        // Every :root declaration becomes a variable...
        SetVar(ps, prop, val);
        // ...and, if it names a token, seeds that token.
        const int tk = TokenFromName(prop);
        if (tk >= 0 && val.kind == Val_Color)
        {
            sheet.tokens.Colors[tk] = val.color;
            sheet.hasToken[tk] = true;
        }
        if ((std::strcmp(prop, "radius") == 0 || std::strcmp(prop, "rounding") == 0)
            && val.kind == Val_Number)
            sheet.frameRounding = val.number;
        return;
    }

    // Widget selectors -> tokens / style vars (minimal mapping).
    auto setTok = [&](int tk) {
        if (val.kind == Val_Color) { sheet.tokens.Colors[tk] = val.color; sheet.hasToken[tk] = true; }
    };
    if (std::strcmp(selector, "Button") == 0)
    {
        if (std::strcmp(prop, "background") == 0) setTok(VanThemeToken_Primary);
        else if (std::strcmp(prop, "rounding") == 0 && val.kind == Val_Number) sheet.frameRounding = val.number;
    }
    else if (std::strcmp(selector, "Button:hover") == 0)
    {
        if (std::strcmp(prop, "background") == 0) setTok(VanThemeToken_Secondary);
    }
    else if (std::strcmp(selector, "Window") == 0)
    {
        if (std::strcmp(prop, "background") == 0) setTok(VanThemeToken_Background);
        else if (std::strcmp(prop, "border") == 0) setTok(VanThemeToken_Border);
        else if (std::strcmp(prop, "rounding") == 0 && val.kind == Val_Number) sheet.windowRounding = val.number;
    }
    else if (std::strcmp(selector, "Text") == 0)
    {
        if (std::strcmp(prop, "color") == 0) setTok(VanThemeToken_TextPrimary);
    }
    // Unknown selectors are ignored (forward-compatible), not an error.
}

// Parse the whole sheet. Returns true on success; on failure, *err points into
// the parser's static-lifetime buffer (valid until the next parse).
static bool ParseStyleSheet(const char* text, size_t len, ParsedSheet& out, const char** err)
{
    static char s_err[160];
    Parser ps;
    ps.p = text; ps.end = text + len; ps.line = 1;
    ps.varCount = 0; ps.failed = false; ps.err[0] = '\0';
    ParsedSheetInit(out);

    while (true)
    {
        SkipWs(ps);
        if (ps.p >= ps.end) break;

        char selector[64];
        if (ReadSelector(ps, selector, sizeof(selector)) == 0) { ps.Fail("expected a selector"); break; }
        SkipWs(ps);
        if (ps.p >= ps.end || *ps.p != '{') { ps.Fail("expected '{'"); break; }
        ++ps.p;

        // declarations
        while (true)
        {
            SkipWs(ps);
            if (ps.p < ps.end && *ps.p == '}') { ++ps.p; break; }
            if (ps.p >= ps.end) { ps.Fail("unterminated block"); break; }

            char prop[64];
            if (ReadIdent(ps, prop, sizeof(prop)) == 0) { ps.Fail("expected a property"); break; }
            SkipWs(ps);
            if (ps.p >= ps.end || *ps.p != ':') { ps.Fail("expected ':'"); break; }
            ++ps.p;

            Value val = ParseValue(ps);
            if (ps.failed) break;
            ApplyDecl(ps, out, selector, prop, val);

            SkipWs(ps);
            if (ps.p < ps.end && *ps.p == ';') ++ps.p;
        }
        if (ps.failed) break;
    }

    if (ps.failed)
    {
        std::snprintf(s_err, sizeof(s_err), "%s", ps.err);
        if (err) *err = s_err;
        return false;
    }
    if (err) *err = nullptr;
    return true;
}

} // namespace Detail
} // namespace VanGui
