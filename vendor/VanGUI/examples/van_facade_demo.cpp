// van_facade_demo.cpp
// A self-contained demonstration of the van:: fluent facade. Call VanFacadeDemo()
// once per frame from inside your VanGUI frame (between NewFrame() and Render()).
// Build by adding van.h's directory to your include path; no extra link deps
// beyond the VanGUI core (and vangui_notify if you want the toast lines).

#include "van.h"

namespace {
struct DemoState {
    bool   open      = true;
    bool   vsync     = true;
    bool   fullscreen= false;
    float  volume    = 70.0f;
    int    quality   = 2;
    int    theme     = 0;
    van::Vec4 accent { 0.26f, 0.59f, 0.98f, 1.0f };
    char   profile[64] = "default";
};
DemoState g;
}

void VanFacadeDemo()
{
    using namespace van;

    // A menu bar built from RAII scopes — no manual End* calls.
    if (auto bar = main_menu_bar()) {
        if (auto m = menu("File")) {
            menu_item("New",  { .shortcut = "Ctrl+N" });
            menu_item("Open", { .shortcut = "Ctrl+O" });
            separator();
            menu_item("Quit", { .shortcut = "Ctrl+Q" }).on_click([]{ g.open = false; });
        }
        if (auto m = menu("View")) {
            menu_item("Fullscreen", g.fullscreen);
        }
    }

    if (!g.open) return;

    if (auto w = window("VanGUI \xE2\x80\x94 Fluent Demo", { .size = {460, 420}, .p_open = &g.open })) {

        heading("Display");
        checkbox("V-Sync", g.vsync);
        same_line();
        checkbox("Fullscreen", g.fullscreen);

        slider("Volume", g.volume, 0.0f, 100.0f, "%.0f%%").tooltip("Master output volume");

        const char* qualities[] = { "Low", "Medium", "High", "Ultra" };
        combo("Quality", g.quality, qualities, 4);

        const char* themes = "Dark\0Light\0Nord\0Dracula\0";
        combo("Theme", g.theme, themes);

        color_edit("Accent", g.accent);
        input_text("Profile", g.profile, sizeof(g.profile));

        heading("Actions");
        // A button row that auto-arranges horizontally — no SameLine needed.
        row([&]{
            button("Apply", { .primary = true })
                .on_click([]{
#                   ifdef VANGUI_ENABLE_NOTIFY
                    toast_success("Settings applied");
#                   endif
                })
                .tooltip("Save and apply");
            button("Reset").on_click([]{ g = DemoState{}; });
            button("Cancel").on_click([]{ g.open = false; });
        });

        // Disabled section reacts to a live value, scoped via RAII.
        if (auto d = disabled(!g.vsync)) {
            text_disabled("Frame pacing options (enable V-Sync to edit)");
        }

        if (auto t = tree("Advanced")) {
            bullet("Everything here is plain van:: calls");
            bullet("Scopes close themselves (RAII)");
            grid(4, 8, [](int i){ button("cell"); (void)i; });
        }

        // A two-column table, also a RAII scope.
        if (auto tb = table("##stats", 2)) {
            table_row(); table_col(); text("Resolution");
            table_col(); text("1920 x 1080");
            table_row(); table_col(); text("Renderer");
            table_col(); text("VanGUI");
        }
    }
}
