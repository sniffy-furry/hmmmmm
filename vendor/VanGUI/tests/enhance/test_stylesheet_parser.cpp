#define VANGUI_ENABLE_STYLESHEET
#define VANGUI_ENABLE_THEME_ENGINE
#include "vangui.h"
#include "misc/vangui_style_sheet_parser.inl"
#include <cstdio>
#include <cmath>
using namespace VanGui;
using namespace VanGui::Detail;

static int g_fail = 0;
#define CHECK(c,m) do{ if(!(c)){printf("  FAIL: %s\n",m);++g_fail;} else printf("  ok  : %s\n",m);}while(0)
static bool approx(float a,float b){ return fabsf(a-b)<=0.01f; }

int main(){
    printf("[1] valid sheet: vars, $ref, lighten(), rules\n");
    {
        const char* css =
            "/* theme */\n"
            ":root { primary: #4285F4; surface: #14141A; radius: 6px; }\n"
            "Button       { background: $primary; rounding: $radius; }\n"
            "Button:hover { background: lighten($primary, 10%); }\n"
            "Window       { background: $surface; }\n";
        ParsedSheet s; const char* err=nullptr;
        bool ok = ParseStyleSheet(css, strlen(css), s, &err);
        CHECK(ok, "parse succeeds");
        CHECK(err==nullptr, "no error string on success");
        CHECK(s.hasToken[VanThemeToken_Primary], "primary token set");
        CHECK(approx(s.tokens.Colors[VanThemeToken_Primary].x, 0x42/255.0f), "primary.r == 0x42");
        CHECK(approx(s.tokens.Colors[VanThemeToken_Primary].y, 0x85/255.0f), "primary.g == 0x85");
        CHECK(approx(s.tokens.Colors[VanThemeToken_Primary].z, 0xF4/255.0f), "primary.b == 0xF4");
        CHECK(s.hasToken[VanThemeToken_Background], "Window background -> Background token");
        CHECK(approx(s.tokens.Colors[VanThemeToken_Background].x, 0x14/255.0f), "surface.r==0x14");
        CHECK(approx(s.frameRounding, 6.0f), "radius 6px -> frameRounding 6");
        CHECK(s.hasToken[VanThemeToken_Secondary], "hover -> Secondary token set");
        // lighten by 10% adds 0.1 to each channel (clamped)
        CHECK(approx(s.tokens.Colors[VanThemeToken_Secondary].x, 0x42/255.0f + 0.10f), "secondary = lighten(primary,10%)");
    }

    printf("[2] #RGB short hex and #RRGGBBAA\n");
    {
        const char* css = ":root { primary: #f00; overlay: #00000080; }\n";
        ParsedSheet s; const char* err=nullptr;
        CHECK(ParseStyleSheet(css, strlen(css), s, &err), "parse short+alpha hex");
        CHECK(approx(s.tokens.Colors[VanThemeToken_Primary].x, 1.0f) &&
              approx(s.tokens.Colors[VanThemeToken_Primary].y, 0.0f), "#f00 -> red");
        CHECK(approx(s.tokens.Colors[VanThemeToken_Overlay].w, 0x80/255.0f), "overlay alpha 0x80");
    }

    printf("[3] error reporting with line number\n");
    {
        const char* css = ":root { primary: #4285F4; }\nButton  background: $primary; }\n";
        ParsedSheet s; const char* err=nullptr;
        bool ok = ParseStyleSheet(css, strlen(css), s, &err);
        CHECK(!ok, "missing '{' fails");
        CHECK(err && strstr(err, "line 2"), "error mentions line 2");
        printf("       (error = \"%s\")\n", err?err:"(null)");
    }

    printf("[4] unknown variable is an error\n");
    {
        const char* css = "Button { background: $nope; }\n";
        ParsedSheet s; const char* err=nullptr;
        CHECK(!ParseStyleSheet(css, strlen(css), s, &err), "unknown $var fails");
        CHECK(err && strstr(err, "unknown variable"), "error names unknown variable");
    }

    printf("\n%s (%d failure%s)\n", g_fail?"TESTS FAILED":"ALL TESTS PASSED", g_fail, g_fail==1?"":"s");
    return g_fail?1:0;
}
