# `van.h` — the VanGUI fluent facade

> One include. One namespace. RAII everywhere. `std::expected` for the fallible
> bits. The VanHooks feel, applied to UI.

`van.h` is a header-only ergonomic front door to VanGUI. It does **not**
reimplement any widget — it wraps the existing core and the enhancement pillars
so call sites read like intent. It adds zero per-frame allocations and a single
tiny layout-cursor stack; with nothing called, it compiles away to direct
`VanGui::` calls.

```cpp
#include "van.h"
using namespace van;

if (auto w = window("Settings", { .size = {420, 300}, .p_open = &open })) {
    heading("Display");
    row([&]{
        button("Save", { .primary = true }).on_click(save).tooltip("write to disk");
        button("Cancel").on_click([&]{ open = false; });
    });
    checkbox("V-Sync", cfg.vsync);
    slider("Volume", cfg.volume, 0.f, 100.f, "%.0f%%");
}
```

## Why it exists

The core VanGUI API is immediate-mode C-style: `Begin/End` pairs you must balance
by hand, out-params via pointers, `nodiscard` returns, manual `SameLine`. That is
fast and explicit, but verbose to script with. `van.h` keeps the immediate-mode
model and speed while giving you:

- **RAII scopes** — every `Begin*/End*` and `Push*/Pop*` becomes a movable guard
  that closes itself. No mismatched `End` calls, ever.
- **Fluent widget results** — `button(...).on_click(fn).tooltip("...")`.
- **References, not pointers** — `checkbox("V-Sync", cfg.vsync)`.
- **Designated-initializer options** — `button("Go", { .primary = true })`.
- **Auto-layout** — `row([&]{ ... })` arranges children horizontally with no
  manual `SameLine` (works for `van::` widgets).
- **`std::expected` helpers** and opt-in **toast / dialog** integrations.

## Scopes (RAII)

`if (auto s = van::xxx(...))` — the bool is the open/visible state. The scope
closes on the way out of the block.

| Factory | Wraps | Ends |
|---|---|---|
| `window(title, WindowOpts)` | `Begin/End` | always |
| `child(id, ChildOpts)` | `BeginChild/EndChild` | always |
| `group()` | `BeginGroup/EndGroup` | always |
| `disabled(on)` | `BeginDisabled/EndDisabled` | always |
| `popup(id)` / `modal(title,…)` | `BeginPopup(Modal)/EndPopup` | if open |
| `menu(label)` / `menu_bar()` / `main_menu_bar()` | `BeginMenu*/EndMenu*` | if open |
| `tab_bar(id)` / `tab(label)` | `BeginTabBar/Item` | if open |
| `table(id, cols, TableOpts)` | `BeginTable/EndTable` | if open |
| `tooltip()` / `combo(label,preview)` / `list_box(label)` | … | if open |
| `tree(label)` | `TreeNode/TreePop` | if open |
| `style_color(idx,col)` / `style_var(idx,val)` | `Push/PopStyleColor/Var` | always |
| `id(...)` / `item_width(w)` / `indent()` | `Push/Pop*` | always |

```cpp
{
    auto c = van::style_color(VanGuiCol_Button, Vec4(0.8f,0.2f,0.2f,1));
    van::button("Danger");          // styled
}                                   // color popped automatically
```

## Widgets

All take references for bound values and return a `Response`:

`button` · `small_button` · `checkbox` · `radio` · `selectable` · `menu_item` ·
`slider` (float/int) · `drag` (float/int) · `input` (float/int) · `input_text`
(char buffer or `std::string`) · `color_edit` (`Vec4`/`Color`) · `combo`
(array or `"a\0b\0"`).

`Response` exposes `clicked()/changed()`, `hovered()`, `active()`, and the
chaining methods `on_click(fn)`, `on_change(fn)`, `on_hover(fn)`, `tooltip(text)`.

## Layout

- `row([&]{ ... })` — children flow horizontally (auto `SameLine`).
- `column([&]{ ... })` — children stack vertically.
- `grid(columns, count, [&](int i){ ... })` — fixed-column wrapping grid.
- `separator()`, `spacing()`, `same_line()`, `new_line()`, `dummy(size)`.
- Text: `text`, `text_colored`, `text_disabled`, `heading` (separator label),
  `bullet`.
- Tables: `table_setup`, `table_headers`, `table_row`, `table_col`.

## `std::expected` helpers (when `<expected>` is available)

```cpp
van::on_value(van::GetOpenFileName("*.txt"), [](const std::string& path){ load(path); });
van::on_error(result, [](auto e){ log(e); });
auto path = van::value_or(result, std::string{});
```

## Optional integrations

Light up automatically when you compiled the matching module:

- **notify** (`VANGUI_ENABLE_NOTIFY`): `toast_info/success/warning/error(msg)`,
  and `toast_on_error(expected, "msg")`.
- **dialogs** (`VANGUI_ENABLE_DIALOGS`): `confirm(title)` (+ the module's
  `MessageBox`, `GetOpenFileName`, `GetSaveFileName`).

## Build

`van.h` is header-only and lives at the repo root, already on the include path of
anything linking the `vangui` core. For clarity you can also link the convenience
interface target:

```cmake
target_link_libraries(my_app PRIVATE vangui::van)   # adds the include path
```

See `examples/van_facade_demo.cpp` for a complete settings window built entirely
with the facade.

## Relationship to the rest of the suite

`van.h` is the ergonomic surface; the pillars remain the engine:

- animation/transitions → `vangui_anim` (the facade's styled widgets ride the
  same theme/anim path),
- toasts → `vangui_notify`, dialogs → `vangui_dialogs`,
- declarative theming → `vangui_style_sheet` (`.vss`),
- layout primitives → `vangui_layout` (the facade's `row/column` are a lighter,
  widget-aware convenience over the same idea).

Use the facade for everyday UI; drop to the pillar APIs or raw `VanGui::` calls
any time you need the full surface. They interoperate freely.
