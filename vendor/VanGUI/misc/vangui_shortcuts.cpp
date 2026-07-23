// vangui_shortcuts.cpp — see vangui_shortcuts.h. Empty TU unless enabled.
#include "vangui_shortcuts.h"

#ifdef VANGUI_ENABLE_SHORTCUTS

#include <cstdio>
#include <cstring>

#ifdef VANGUI_ENABLE_COMMAND_PALETTE
#include "vangui_command_palette.h"
#endif

namespace VanGui {

namespace {
struct Shortcut { VanGuiKeyChord chord; const char* name; void (*action)(void*); void* user; };
constexpr int kMax = 256;
static Shortcut s_items[kMax];
static int      s_count = 0;
}

void RegisterShortcut(VanGuiKeyChord chord, const char* name, void (*action)(void*), void* user)
{
    if (s_count >= kMax || !action) return;
    s_items[s_count++] = { chord, name ? name : "", action, user };
#ifdef VANGUI_ENABLE_COMMAND_PALETTE
    // Mirror into the command palette so it is discoverable + runnable there.
    VanCommand cmd{};
    cmd.Name = name ? name : "";
    cmd.Keywords = nullptr;
    cmd.Action = action;
    cmd.UserData = user;
    RegisterCommand(cmd);
#endif
}

void ClearShortcuts() { s_count = 0; }

void DispatchShortcuts()
{
    for (int i = 0; i < s_count; ++i)
        if (s_items[i].chord != 0 && IsKeyChordPressed(s_items[i].chord))
            s_items[i].action(s_items[i].user);
}

void GetShortcutLabel(VanGuiKeyChord chord, char* buf, size_t buf_size)
{
    if (!buf || buf_size == 0) return;
    buf[0] = '\0';
    auto append = [&](const char* s) {
        const size_t used = std::strlen(buf);
        if (used > 0)
            snprintf(buf + used, buf_size - used, "+%s", s);
        else
            snprintf(buf, buf_size, "%s", s);
    };
    if (chord & VanGuiMod_Ctrl)  append("Ctrl");
    if (chord & VanGuiMod_Shift) append("Shift");
    if (chord & VanGuiMod_Alt)   append("Alt");
    if (chord & VanGuiMod_Super) append("Super");
    const VanGuiKey key = (VanGuiKey)(chord & ~VanGuiMod_Mask_);
    const char* kn = GetKeyName(key);
    if (kn && kn[0]) append(kn);
}

} // namespace VanGui

#endif // VANGUI_ENABLE_SHORTCUTS
