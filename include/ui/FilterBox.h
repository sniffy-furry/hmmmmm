#pragma once
// ── ui/FilterBox.h ────────────────────────────────────────────────────────────
// Lightweight VanGui filter-box widget (audit 4.2).
//
// Eliminates the repeated pattern:
//   char buf[128]; InputTextWithHint; copy to std::string; tolower.
// that appeared 3× in TexturePackPanel::DrawTree, VaultPanel::DrawTree,
// and VaultPanel::DrawStringsTab.
//
// Usage:
//   // As a member:              FilterBox texFilter_;
//   // In Draw():                if (texFilter_.Draw("##id", "Filter textures…"))
//   //                               { /* optional: react to change */ }
//   // In filter loop:           if (!texFilter_.Matches(name)) continue;
// ─────────────────────────────────────────────────────────────────────────────
#include "core/StringUtil.h"
#include <vangui.h>
#include <cstring>
#include <string>

namespace nfsmw {

struct FilterBox {
    char        buf[128] = {};   ///< raw VanGui buffer (NUL-terminated)
    std::string lower;           ///< ToLowerAscii(buf), recomputed on change

    /// Render the InputTextWithHint widget.
    /// @param widgetId   VanGui ID string (e.g. "##filter")
    /// @param hint       Placeholder text shown when the box is empty
    /// @param width      Pass -1.f (default) to stretch full available width
    /// @return true if the text changed this frame
    bool Draw(const char* widgetId, const char* hint, float width = -1.f) {
        if (width != 0.f)
            VanGui::SetNextItemWidth(width);
        if (VanGui::InputTextWithHint(widgetId, hint, buf, sizeof(buf))) {
            lower = ToLowerAscii(std::string(buf));
            return true;
        }
        return false;
    }

    /// Returns true if `name` contains the current filter (case-insensitive).
    /// An empty filter matches everything.
    bool Matches(std::string_view name) const {
        return ContainsCIStr(name, lower);
    }

    bool IsEmpty() const { return buf[0] == '\0'; }

    void Clear() {
        buf[0] = '\0';
        lower.clear();
    }
};

} // namespace nfsmw
