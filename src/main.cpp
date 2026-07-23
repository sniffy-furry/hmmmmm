// ─── NFSMW Toolkit — entry point ──────────────────────────────────────────────
#include "ui/AppShell.h"
#include <cstdio>

int main(int /*argc*/, char** /*argv*/) {
    nfsmw::AppShell app;

    if (auto r = app.Init(1400, 900); !r) {
        std::fprintf(stderr, "Init failed: %s\n", r.error.c_str());
        return 1;
    }

    app.Run();
    return 0;
}
