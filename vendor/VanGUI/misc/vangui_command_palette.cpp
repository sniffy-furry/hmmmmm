// dear vangui: command palette widget (Ctrl+K style)
// See vangui_command_palette.h for usage instructions.

#include "vangui.h"

#ifndef VANGUI_DISABLE

#include "vangui_command_palette.h"
#include "vangui_anim.h"   // animated open (Pillar 1)

// ---------------------------------------------------------------------------
// Internal state — all file-scope, inside an anonymous namespace so no
// symbols leak into the global namespace.
// ---------------------------------------------------------------------------
namespace
{
    static const int k_MaxCommands = 512;
    static const float k_PaletteWidth  = 580.0f;
    static const float k_ResultsHeight = 300.0f;

    // Per-entry filter score. Higher is better.
    // 2 = prefix match in Name, 1 = mid-string match in Name or match in Keywords, 0 = no match.
    struct FilteredEntry
    {
        int Index; // index into g_Commands
        int Score;
    };

    struct CommandPaletteState
    {
        bool            Open            = false;
        bool            JustOpened      = false; // true for exactly one frame after Open flips to true
        char            SearchBuf[256]  = {};
        int             SelectedIndex   = 0;     // index into FilteredIndices

        VanGui::VanCommand Commands[k_MaxCommands];
        int             CommandCount    = 0;

        FilteredEntry   Filtered[k_MaxCommands];
        int             FilteredCount   = 0;
    };

    static CommandPaletteState g_State;

} // anonymous namespace

// ---------------------------------------------------------------------------
// Helper: case-insensitive substring search.
// Returns a pointer to the first occurrence of needle inside haystack,
// or nullptr if not found. Does not use any STL or CRT beyond tolower().
// ---------------------------------------------------------------------------
static const char* StrFindCaseInsensitive(const char* haystack, const char* needle)
{
    if (!haystack || !needle || needle[0] == '\0')
        return haystack; // empty needle always matches at position 0

    for (; *haystack != '\0'; ++haystack)
    {
        // Quick check: does the first char match?
        if (((*haystack) | 0x20) != ((*needle) | 0x20))
            continue;

        // Walk both strings in parallel.
        const char* h = haystack;
        const char* n = needle;
        while (*n != '\0' && ((*h) | 0x20) == ((*n) | 0x20))
        {
            ++h;
            ++n;
        }
        if (*n == '\0')
            return haystack; // full match
    }
    return nullptr;
}

static bool StrContainsCaseInsensitive(const char* haystack, const char* needle)
{
    return StrFindCaseInsensitive(haystack, needle) != nullptr;
}

// ---------------------------------------------------------------------------
// Fuzzy subsequence scorer.
// Returns >0 if every char of `needle` appears in `hay` in order (case-
// insensitive), with bonuses for contiguous runs and word-boundary starts —
// so "fop" matches "File: Open". Returns 0 when the subsequence is incomplete.
// (TODO 'filters: fuzzy'.)
// ---------------------------------------------------------------------------
static int FuzzySubsequenceScore(const char* hay, const char* needle)
{
    if (!needle || needle[0] == '\0') return 1;
    if (!hay) return 0;

    const char* n = needle;
    int  score    = 0;
    int  run      = 0;
    bool prevSep  = true;   // start of string counts as a word boundary

    for (const char* h = *hay ? hay : hay; *h; ++h)
    {
        const char hc = (char)(*h | 0x20);
        const char nc = (char)(*n | 0x20);
        if (nc == hc)
        {
            score += 1;
            if (run > 0)  score += 3;   // contiguous match bonus
            if (prevSep)  score += 5;   // word-boundary bonus
            ++run;
            ++n;
            if (*n == '\0') return score + 1;  // whole needle consumed
        }
        else
        {
            run = 0;
        }
        const char c = *h;
        prevSep = (c == ' ' || c == '-' || c == ':' || c == '_' || c == '/' || c == '.');
    }
    return 0;   // needle not fully matched
}

// ---------------------------------------------------------------------------
// Filter rebuild
// ---------------------------------------------------------------------------
static void RebuildFilter()
{
    g_State.FilteredCount  = 0;
    g_State.SelectedIndex  = 0;

    const char* needle = g_State.SearchBuf;
    const bool  empty  = (needle[0] == '\0');

    for (int i = 0; i < g_State.CommandCount; ++i)
    {
        const VanGui::VanCommand& cmd = g_State.Commands[i];

        int score = 0;

        if (empty)
        {
            score = 1; // show everything when no search text
        }
        else
        {
            // Contiguous substring matches rank highest (exact intent), then
            // fuzzy subsequence matches fill in (e.g. "fop" -> "File: Open").
            const char* match_pos = StrFindCaseInsensitive(cmd.Name, needle);
            if (match_pos != nullptr)
            {
                bool is_prefix = (match_pos == cmd.Name);
                if (!is_prefix && match_pos > cmd.Name)
                    is_prefix = (*(match_pos - 1) == ' ');
                score = is_prefix ? 1000 : 600;
            }
            if (score < 600 && cmd.Keywords != nullptr &&
                StrContainsCaseInsensitive(cmd.Keywords, needle))
                score = (score > 500) ? score : 500;

            // Fuzzy fallback / refinement on name and keywords.
            int fz = FuzzySubsequenceScore(cmd.Name, needle);
            if (cmd.Keywords)
            {
                const int fk = FuzzySubsequenceScore(cmd.Keywords, needle);
                if (fk > fz) fz = fk;
            }
            if (fz > score) score = fz;
        }

        if (score > 0)
        {
            VAN_ASSERT(g_State.FilteredCount < k_MaxCommands);
            g_State.Filtered[g_State.FilteredCount].Index = i;
            g_State.Filtered[g_State.FilteredCount].Score = score;
            ++g_State.FilteredCount;
        }
    }

    // Sort by score descending (insertion sort — small N, no STL).
    for (int i = 1; i < g_State.FilteredCount; ++i)
    {
        FilteredEntry key = g_State.Filtered[i];
        int j = i - 1;
        while (j >= 0 && g_State.Filtered[j].Score < key.Score)
        {
            g_State.Filtered[j + 1] = g_State.Filtered[j];
            --j;
        }
        g_State.Filtered[j + 1] = key;
    }
}

// ---------------------------------------------------------------------------
// Render a single command name with the matched substring highlighted.
// Splits the name into [before][match][after] and uses TextColored for the
// match span. Falls back to plain Text() when there is no active search.
// ---------------------------------------------------------------------------
static void RenderCommandName(const char* name, const char* needle,
                               const VanVec4& highlight_color)
{
    if (needle == nullptr || needle[0] == '\0')
    {
        VanGui::TextUnformatted(name);
        return;
    }

    const char* match_start = StrFindCaseInsensitive(name, needle);
    if (match_start == nullptr)
    {
        VanGui::TextUnformatted(name);
        return;
    }

    // Count needle length (we need it to find the end of the matched span).
    int needle_len = 0;
    for (const char* p = needle; *p != '\0'; ++p)
        ++needle_len;

    const char* match_end = match_start + needle_len;

    // Render prefix (before match).
    if (match_start > name)
    {
        VanGui::TextUnformatted(name, match_start);
        VanGui::SameLine(0.0f, 0.0f);
    }

    // Render matched span.
    VanGui::TextColored(highlight_color, "%.*s",
                        static_cast<int>(match_end - match_start), match_start);

    // Render suffix (after match).
    if (*match_end != '\0')
    {
        VanGui::SameLine(0.0f, 0.0f);
        VanGui::TextUnformatted(match_end);
    }
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void VanGui::RegisterCommand(const VanCommand& cmd)
{
    VAN_ASSERT(g_State.CommandCount < k_MaxCommands);
    g_State.Commands[g_State.CommandCount++] = cmd;
    // If palette is open, keep the filtered list fresh.
    if (g_State.Open)
        RebuildFilter();
}

void VanGui::ClearCommands()
{
    g_State.CommandCount  = 0;
    g_State.FilteredCount = 0;
    g_State.SelectedIndex = 0;
}

void VanGui::OpenCommandPalette()
{
    if (!g_State.Open)
    {
        VanGui::Anim::Reset((VanGuiID)0x504C4554u); // "PLET" — restart open fade
        g_State.Open       = true;
        g_State.JustOpened = true;
    }
}

void VanGui::CloseCommandPalette()
{
    g_State.Open       = false;
    g_State.JustOpened = false;
}

bool VanGui::IsCommandPaletteOpen()
{
    return g_State.Open;
}

// ---------------------------------------------------------------------------
// Main render function — call once per frame after VanGui::NewFrame().
// ---------------------------------------------------------------------------
void VanGui::RenderCommandPalette()
{
    // ------------------------------------------------------------------
    // 1. Toggle on Ctrl+K (but not while text input has the character).
    //    We use IsKeyChordPressed so the Ctrl modifier is checked atomically.
    // ------------------------------------------------------------------
    if (VanGui::IsKeyChordPressed(VanGuiMod_Ctrl | VanGuiKey_K))
    {
        if (g_State.Open)
            VanGui::CloseCommandPalette();
        else
            VanGui::OpenCommandPalette();
    }

    if (!g_State.Open)
        return;

    // ------------------------------------------------------------------
    // 2. First-frame initialisation: clear search buffer, rebuild filter,
    //    and signal the InputText to claim focus.
    // ------------------------------------------------------------------
    if (g_State.JustOpened)
    {
        g_State.SearchBuf[0]  = '\0';
        g_State.SelectedIndex = 0;
        RebuildFilter();
        // JustOpened stays true — consumed in the InputText block below,
        // then cleared.
    }

    // ------------------------------------------------------------------
    // 3. Center the popup window on the main viewport.
    // ------------------------------------------------------------------
    VanGuiViewport* viewport = VanGui::GetMainViewport();
    VanVec2 center = viewport->GetCenter();
    VanGui::SetNextWindowPos(center, VanGuiCond_Always, VanVec2(0.5f, 0.3f));
    VanGui::SetNextWindowSize(VanVec2(k_PaletteWidth, 0.0f), VanGuiCond_Always);
    VanGui::SetNextWindowSizeConstraints(
        VanVec2(k_PaletteWidth, 0.0f),
        VanVec2(k_PaletteWidth, k_ResultsHeight + 120.0f));

    VanGuiWindowFlags palette_flags =
        VanGuiWindowFlags_NoTitleBar  |
        VanGuiWindowFlags_NoResize    |
        VanGuiWindowFlags_NoMove      |
        VanGuiWindowFlags_NoScrollbar |
        VanGuiWindowFlags_NoSavedSettings;

    // Use OpenPopup + BeginPopup so the palette closes when the user clicks
    // outside it (standard popup dismissal behaviour).
    static const char* k_PopupId = "##CommandPalette";

    // We call OpenPopup every frame while g_State.Open is true.  This is safe
    // because OpenPopup is idempotent when the popup is already open.
    VanGui::OpenPopup(k_PopupId);

    if (!VanGui::BeginPopup(k_PopupId, palette_flags))
    {
        // Popup was dismissed externally (e.g. click outside).
        g_State.Open       = false;
        g_State.JustOpened = false;
        return;
    }

    // Animated open: fade the palette in via the shared substrate. On the
    // first frame (JustOpened) we seed the tween at 0, then it eases to 1.
    {
        const VanGuiID k_PaletteAnimId = (VanGuiID)0x504C4554u; // "PLET"
        const float pal_alpha = VanGui::Anim::AnimBool(
            k_PaletteAnimId, !g_State.JustOpened,
            { .Duration = 0.12f, .Easing = VanGui::Anim::VanEasing_QuadOut });
        VanGui::PushStyleVar(VanGuiStyleVar_Alpha, pal_alpha);
    }

    // ------------------------------------------------------------------
    // 4. Search / filter input.
    // ------------------------------------------------------------------
    VanGui::PushStyleVar(VanGuiStyleVar_FramePadding, VanVec2(8.0f, 6.0f));
    VanGui::SetNextItemAllowOverlap();

    if (g_State.JustOpened)
        VanGui::SetKeyboardFocusHere();

    bool search_changed = VanGui::InputText(
        "##search",
        g_State.SearchBuf,
        sizeof(g_State.SearchBuf),
        VanGuiInputTextFlags_AutoSelectAll);

    VanGui::PopStyleVar();

    if (g_State.JustOpened)
    {
        g_State.JustOpened = false; // consume the flag
    }

    if (search_changed)
    {
        g_State.SelectedIndex = 0;
        RebuildFilter();
    }

    // ------------------------------------------------------------------
    // 5. Keyboard navigation (must happen before rendering the list so the
    //    scroll-into-view logic fires in the same frame).
    // ------------------------------------------------------------------
    bool execute_selected = false;

    if (VanGui::IsKeyPressed(VanGuiKey_DownArrow, true))
    {
        if (g_State.FilteredCount > 0)
            g_State.SelectedIndex = (g_State.SelectedIndex + 1) % g_State.FilteredCount;
    }
    if (VanGui::IsKeyPressed(VanGuiKey_UpArrow, true))
    {
        if (g_State.FilteredCount > 0)
        {
            g_State.SelectedIndex = g_State.SelectedIndex - 1;
            if (g_State.SelectedIndex < 0)
                g_State.SelectedIndex = g_State.FilteredCount - 1;
        }
    }
    if (VanGui::IsKeyPressed(VanGuiKey_Enter, false))
    {
        execute_selected = true;
    }
    if (VanGui::IsKeyPressed(VanGuiKey_Escape, false))
    {
        VanGui::CloseCurrentPopup();
        g_State.Open = false;
        VanGui::PopStyleVar();   // animated-open alpha
        VanGui::EndPopup();
        return;
    }

    // ------------------------------------------------------------------
    // 6. Separator between search bar and results.
    // ------------------------------------------------------------------
    VanGui::Separator();

    // ------------------------------------------------------------------
    // 7. Scrollable results list.
    // ------------------------------------------------------------------
    const VanVec4 k_HighlightColor(1.0f, 0.85f, 0.3f, 1.0f); // warm yellow
    const VanVec4 k_GrayColor(0.55f, 0.55f, 0.55f, 1.0f);

    int command_to_execute = -1; // index into g_State.Commands

    (void)VanGui::BeginChild("##results", VanVec2(0.0f, k_ResultsHeight), false,
                        VanGuiWindowFlags_None);

    for (int fi = 0; fi < g_State.FilteredCount; ++fi)
    {
        const int cmd_index  = g_State.Filtered[fi].Index;
        const bool is_selected = (fi == g_State.SelectedIndex);

        // Give each row a unique ID so VanGui can distinguish them even
        // though they all use the anonymous label "##s".
        VanGui::PushID(fi);

        // Selectable spans full width; suppress its built-in label rendering
        // so we can do our own highlighted text on the same line.
        VanGui::PushStyleVar(VanGuiStyleVar_SelectableTextAlign, VanVec2(0.0f, 0.5f));
        bool clicked = VanGui::Selectable("##s", is_selected,
                                           VanGuiSelectableFlags_None,
                                           VanVec2(0.0f, 0.0f));
        VanGui::PopStyleVar();

        // Render command name as an overlay on the selectable.
        VanGui::SameLine();
        RenderCommandName(g_State.Commands[cmd_index].Name,
                           g_State.SearchBuf, k_HighlightColor);

        if (is_selected)
        {
            // Make the keyboard-selected item visible.
            if (VanGui::IsKeyPressed(VanGuiKey_DownArrow, true) ||
                VanGui::IsKeyPressed(VanGuiKey_UpArrow,   true))
            {
                VanGui::SetScrollHereY(0.5f);
            }
            VanGui::SetItemDefaultFocus();
        }

        if (clicked)
        {
            g_State.SelectedIndex = fi;
            command_to_execute    = cmd_index;
        }

        VanGui::PopID();
    }

    VanGui::EndChild();

    // ------------------------------------------------------------------
    // 8. Footer: command count.
    // ------------------------------------------------------------------
    VanGui::Separator();
    VanGui::TextColored(k_GrayColor, "%d command%s",
                         g_State.FilteredCount,
                         g_State.FilteredCount == 1 ? "" : "s");

    VanGui::PopStyleVar();   // animated-open alpha
    VanGui::EndPopup();

    // ------------------------------------------------------------------
    // 9. Execute — do this after EndPopup() so that any UI the action
    //    triggers starts in a clean state.
    // ------------------------------------------------------------------
    if (execute_selected && g_State.FilteredCount > 0)
        command_to_execute = g_State.Filtered[g_State.SelectedIndex].Index;

    if (command_to_execute >= 0)
    {
        VanGui::CloseCommandPalette();
        const VanCommand& cmd = g_State.Commands[command_to_execute];
        if (cmd.Action)
            cmd.Action(cmd.UserData);
    }
}

#endif // #ifndef VANGUI_DISABLE
