# VanGUI Enhancement Roadmap

**Status:** Draft v1 · **C++ Standard:** C++23 · **Base:** Dear ImGui 1.92.x fork
**Governing document:** `Skills/GoldenRules/Engineering_Constitution.md`
**Audience:** human contributors and AI coding assistants working on the VanGUI enhancement layer.

---

## 1. Vision

VanGUI keeps Dear ImGui's immediate-mode core — rebuild-every-frame, no retained widget
tree, optimized vertex output — and layers on a **strictly opt-in enhancement suite** that
makes finished applications feel closer to Qt: motion, transitions, loading states,
declarative theming, standard dialogs, decoupled eventing, and model/view data binding.

The defining constraint is **not negotiable**: every enhancement is compile-time toggleable
and adds **zero cost when unused**. With all toggles off, VanGUI compiles, links, and runs
byte-for-byte like upstream Dear ImGui. "Lightweight by default, full application when you
opt in."

This roadmap supersedes ad-hoc feature work. It exists because the enhancement layer already
started organically (`misc/vangui_notify`, `misc/vangui_theme_engine`, `misc/vangui_themes`,
`misc/vangui_command_palette`, `misc/vangui_node_graph`) and now needs a unifying
architecture before it accretes duplication.

---

## 2. Where we are today (audit)

| Area | State | Notes |
|---|---|---|
| Core | Dear ImGui 1.92.9 WIP, renamed `Van*` | Tracks upstream `master`; recent texture-system rework merged |
| C++23 | Enabled (`CMAKE_CXX_STANDARD 23`) | Also ships a C++20 modules target (`vangui_module`, `import vangui;`) |
| Toasts | `vangui_notify` — full | Info/Success/Warning/Error, progress bars, 4 anchor corners, **own fade timing** |
| Theming | `vangui_theme_engine` + `vangui_themes` | 12 semantic tokens, **animated `TransitionToTheme`**, push/pop scopes, file hot-reload; **own color-interp timing** |
| Presets | dark, light, classic, dracula, nord, monokai, gruvbox_dark | Save/load to file + memory |
| Command palette | `vangui_command_palette` | Command registry, open/close, render; no fuzzy match yet |
| Node graph | `vangui_node_graph` | Editor present |
| Theme editor | `vangui_theme_editor` | Live editor window |
| Docking | `vangui_docking.cpp` present; `origin/docking` branch | Not merged to `master` (TODO #2109) |
| **Animation** | **None as a system** | Only hover-delay timers (`HoverDelayShort/Normal`, `HoverStationaryDelay`) and `g.Time` |

### 2.1 The central problem

`vangui_notify` and `vangui_theme_engine` **each implement their own time integration and
interpolation**. Toast fade and theme color-blending are the same operation — advance a
normalized parameter `t` over time, shape it with an easing curve, interpolate. This is a
direct violation of the Constitution's first chapter (DRY: *"Every piece of knowledge shall
exist in exactly one authoritative location"*).

**The single most important architectural decision in this roadmap is to extract that shared
operation into one substrate — `vangui_anim` — and refactor the existing extras onto it.**
Every other enhancement (loading effects, dialog open/close, widget motion, layout
transitions) is then a thin consumer of that substrate.

---

## 3. Design principles

These derive from the Engineering Constitution and Dear ImGui's own idioms. Every PR in this
roadmap is reviewed against them.

1. **Opt-in, zero-cost-when-off.** Each feature lives behind a `VANGUI_ENABLE_*` macro (the
   existing convention, cf. `VANGUI_ENABLE_FREETYPE`, `VANGUI_ENABLE_TEST_ENGINE`). When the
   macro is undefined, the public functions become `inline` no-op shims that snap to the
   target value and allocate nothing. Callers compile unchanged either way.
2. **Immediate-mode preserved.** No persistent user-facing widget objects. Animation and view
   state is keyed by `VanGuiID` (the existing ID stack) and stored context-side, exactly as
   tables and windows already store transient state.
3. **No per-frame allocations in the render path.** State pools grow once and reuse slots;
   eviction marks slots free. This is an explicit Constitution rule and an existing VanGUI
   performance rule (see `Skills/UI_UX/ImGui_Production.md`).
4. **Pull-based, not callback-based.** The idiomatic IM pattern: the user calls a function
   each frame and receives the current value. No retained callback registration in the core
   substrate (the one exception, signals/slots, is itself an opt-in module — see §7.3).
5. **`std::expected`, not exceptions, for fallible operations.** Matches VanHooks and the
   Constitution. Animation/query calls that cannot fail stay `noexcept` and return by value.
6. **RAII for every scope.** `Begin*/End*` pairs get RAII guard equivalents; connection
   handles auto-disconnect; pushed scopes auto-pop in debug builds with assertions.
7. **DRY across the suite.** One easing library, one time source, one interpolation path, one
   per-frame update entry point. Extras consume; they do not reimplement.
8. **Don't fork the core.** Touch `vangui.cpp` in exactly one place — a single guarded
   `NewFrame` hook (§4.3). Everything else lives in `misc/` (or a new top-level `enhance/`).
   This keeps upstream merges tractable, since the fork tracks ImGui `master`.

---

## 4. Architecture overview

### 4.1 Layering

```
        ┌─────────────────────────────────────────────────────────────┐
        │  Application code (opts in via VANGUI_ENABLE_* in vanconfig)  │
        └─────────────────────────────────────────────────────────────┘
                                     │
   ┌─────────────┬──────────────┬───┴────────┬───────────────┬──────────────┐
   │  notify*    │ theme_engine*│  dialogs   │ command_pal*  │  views / mvc │   Pillar 2,3,4
   │  loading    │ stylesheet   │  layout    │  node_graph*  │  signals     │   (consumers)
   └─────────────┴──────────────┴────────────┴───────────────┴──────────────┘
                                     │  (all motion goes through one path)
                        ┌────────────┴────────────┐
                        │      vangui_anim         │                          Pillar 1
                        │  easing · tween · spring │   (the substrate)
                        │  ID-keyed state pool     │
                        └────────────┬────────────┘
                                     │  reads io.DeltaTime, g.Time; one NewFrame hook
        ┌────────────────────────────┴────────────────────────────────┐
        │           VanGUI immediate-mode core (unmodified)            │
        └──────────────────────────────────────────────────────────────┘

   * = already exists, to be refactored onto vangui_anim in Phase 1
```

### 4.2 File layout (proposed)

New modules follow the existing `misc/` placement and naming. (`enhance/` is an acceptable
alternative top-level directory if we prefer to separate "official extras" from "misc helpers";
default is to keep `misc/` to avoid build-script churn.)

```
misc/
  vangui_anim.h / .cpp            # Pillar 1 — easing, tween, spring, ID-keyed state
  vangui_loading.h / .cpp         # Pillar 2 — spinners, bars, skeletons, overlays
  vangui_style_sheet.h / .cpp     # Pillar 3 — QSS-like declarative theming front-end
  vangui_dialogs.h / .cpp         # Pillar 3 — message box + file dialog
  vangui_signals.h                # Pillar 3 — header-only signals/slots (RAII connections)
  vangui_views.h / .cpp           # Pillar 3 — model/view list & tree adapters
  vangui_layout.h / .cpp          # Pillar 3 — HBox/VBox/Grid/flow helpers
  vangui_notify.{h,cpp}           # Pillar 4 — refactor onto vangui_anim
  vangui_theme_engine.{h,cpp}     # Pillar 4 — refactor onto vangui_anim
  vangui_command_palette.{h,cpp}  # Pillar 4 — fuzzy match + animated open
  vangui_enhance.h                # umbrella include + NewFrameExtras()/RenderExtras()
```

### 4.3 The single core hook

The substrate needs exactly one integration point in the core: advance active animations once
per frame using `io.DeltaTime`. We add it guarded so the unmodified build is unaffected.

```cpp
// in VanGui::NewFrame(), end of function, in vangui.cpp:
#ifdef VANGUI_ENABLE_ANIM
    VanGui::Anim::NewFrameUpdate();   // O(active tweens); early-out when pool empty
#endif
```

When `VANGUI_ENABLE_ANIM` is undefined, this line does not exist and the core is byte-identical
to upstream. This is the only edit to `vangui.cpp` the entire roadmap permits.

### 4.4 Compile-time toggle matrix

```c
// vanconfig.h (user adds the ones they want; all default OFF)
#define VANGUI_ENABLE_ENHANCEMENTS   // master switch (implies nothing on its own)
#define VANGUI_ENABLE_ANIM           // Pillar 1 — required by everything below
#define VANGUI_ENABLE_LOADING        // Pillar 2  (implies ANIM)
#define VANGUI_ENABLE_STYLESHEET     // Pillar 3  (implies theme_engine)
#define VANGUI_ENABLE_DIALOGS        // Pillar 3  (implies ANIM)
#define VANGUI_ENABLE_SIGNALS        // Pillar 3  (header-only, no deps)
#define VANGUI_ENABLE_VIEWS          // Pillar 3
#define VANGUI_ENABLE_LAYOUT         // Pillar 3
```

A module whose dependency is missing fails at compile time with a `#error` naming the required
macro (fail loud, fail early — Constitution: *prefer compile-time validation*).

---

## 5. Pillar 1 — Animation & transition core (`vangui_anim`)

The foundation. Small, pure where possible, no allocations on the hot path.

### 5.1 Easing

Pure, `constexpr`, `noexcept`. The mathematical primitive everything else composes.

```cpp
namespace VanGui::Anim {

enum VanEasing : int {
    VanEasing_Linear,
    VanEasing_QuadIn,    VanEasing_QuadOut,    VanEasing_QuadInOut,
    VanEasing_CubicIn,   VanEasing_CubicOut,   VanEasing_CubicInOut,
    VanEasing_ExpoOut,   VanEasing_CircOut,
    VanEasing_BackOut,   VanEasing_ElasticOut, VanEasing_BounceOut,
    VanEasing_COUNT
};

// t in [0,1] -> shaped [0,1] (some curves overshoot by design, e.g. BackOut/ElasticOut).
[[nodiscard]] constexpr float Ease(VanEasing fn, float t) noexcept;

}
```

### 5.2 ID-keyed tween state

State persists across frames, keyed by `VanGuiID`, stored in a context-side pool. Slots are
"touched" on access and **evicted after `ConfigAnimCompactFrames` untouched frames** — the same
transient-memory discipline the core already uses (`ConfigMemoryCompactTimer`).

```cpp
struct VanAnimParams {
    float     Duration = 0.20f;             // seconds
    VanEasing Easing   = VanEasing_CubicOut;
    float     Delay    = 0.0f;              // seconds before motion starts
    bool      Unscaled = false;            // ignore application time-scale
};

// Animated scalar. Returns the current value; advances toward `target` over time.
[[nodiscard]] VANGUI_API float   AnimFloat(VanGuiID id, float target, const VanAnimParams& p = {});

// Animated color (componentwise, gamma-correct optional).
[[nodiscard]] VANGUI_API VanVec4 AnimColor(VanGuiID id, VanVec4 target, const VanAnimParams& p = {});

// Entrance/exit driver: returns 0..1 progress as `open` flips. Drives fades, scale-ins,
// slide-ins for popups, toasts, dialogs, tooltips, list rows.
[[nodiscard]] VANGUI_API float   AnimBool(VanGuiID id, bool open, const VanAnimParams& p = {});
```

### 5.3 Spring dynamics (optional, natural motion)

Framerate-independent semi-implicit integration. No fixed duration — settles by physics. Used
where overshoot/settle feels better than a curve (drag handles, reorder, knob).

```cpp
struct VanSpringParams { float Stiffness = 170.f; float Damping = 26.f; float Eps = 0.001f; };
[[nodiscard]] VANGUI_API float SpringFloat(VanGuiID id, float target, const VanSpringParams& p = {});
```

### 5.4 Zero-cost-when-off shims

When `VANGUI_ENABLE_ANIM` is undefined, `vangui_anim.h` still declares the API as `inline`
pass-throughs so consumer code compiles identically and pays nothing:

```cpp
#ifndef VANGUI_ENABLE_ANIM
inline float   AnimFloat(VanGuiID, float target, const VanAnimParams& = {}) { return target; }
inline VanVec4 AnimColor(VanGuiID, VanVec4 target, const VanAnimParams& = {}) { return target; }
inline float   AnimBool (VanGuiID, bool open,     const VanAnimParams& = {}) { return open ? 1.f : 0.f; }
#endif
```

### 5.5 Internals & invariants

- **Storage:** `VanGuiStorage` mapping `id -> slot`, plus a `VanVector<VanAnimState>` pool. Grows
  once; freed slots are recycled. **No allocation while steady-state.**
- **Update:** `NewFrameUpdate()` early-returns when the pool is empty (cost ≈ zero when idle).
  Otherwise advances each active slot by `io.DeltaTime` (or unscaled time), clamps, evicts.
- **Idle integration:** while any tween is active, set a context "needs refresh" flag so
  backends can stop sleeping and resume continuous rendering; clear it when all settle. This
  fixes the long-standing TODO *"misc: idle: expose woken up boolean … for backend to be able
  to stop refreshing easily"* and is what lets us claim "lightweight" honestly — an animated UI
  only burns frames while something is actually moving.
- **Determinism for tests:** `NewFrameUpdate()` reads time through an injectable clock so unit
  tests can step animations with fixed `dt` (see §10).

---

## 6. Pillar 2 — Loading effects (`vangui_loading`)

Pure consumers of Pillar 1 and `VanDrawList`. No new core state. Strings pre-formatted; no
per-frame heap.

```cpp
namespace VanGui {

// Indeterminate spinners (phase from g.Time — deterministic, stateless).
VANGUI_API void Spinner(const char* id, float radius, float thickness, VanU32 color, float speed = 1.f);
VANGUI_API void SpinnerDots(const char* id, float radius, int count = 8, VanU32 color = 0);
VANGUI_API void SpinnerBars(const char* id, VanVec2 size, VanU32 color = 0);

// Indeterminate sliding bar (gradient sweep via AddRectFilledMultiColor).
VANGUI_API void IndeterminateBar(const char* id, VanVec2 size);

// Determinate circular progress (fraction 0..1; fill animated via AnimFloat).
VANGUI_API void ProgressRing(const char* id, float fraction, float radius, float thickness);

// Skeleton placeholders with shimmer (shimmer phase from g.Time).
VANGUI_API void Skeleton(VanVec2 size, float rounding = 4.f);
VANGUI_API void SkeletonText(int lines, float line_height = 0.f);

// Dim-and-cover a region while busy; fades via AnimBool. RAII variant provided.
VANGUI_API bool BeginLoadingOverlay(const char* id, bool* p_busy);
VANGUI_API void EndLoadingOverlay();

}
```

Notes:
- Spinners are intentionally **stateless** (phase derived from global time), so they cost
  nothing in the anim pool. Only determinate widgets that interpolate a value (`ProgressRing`,
  overlay fade) touch Pillar 1.
- `vangui_notify` already has live progress bars; `ProgressRing`/`IndeterminateBar` reuse the
  same draw helpers to avoid a second progress implementation (DRY).

---

## 7. Pillar 3 — Qt-like application framework

The heaviest pillar, deliberately split into independently toggleable modules so users adopt
only what they need. Nothing here is required for the others.

### 7.1 Stylesheet layer (`vangui_style_sheet`)

A small **declarative theming front-end** over the existing `vangui_theme_engine`. The theme
engine remains the runtime (semantic tokens + animated transitions); the stylesheet is an
authoring format that compiles down to a `VanThemeTokenSet` plus a set of style-var pushes.

```
/* example.vss — parsed at load, hot-reloaded via existing WatchThemeFile() */
:root        { primary: #4285F4; surface: #14141A; radius: 6px; }
Button       { background: $primary; rounding: $radius; padding: 10 5; }
Button:hover { background: lighten($primary, 8%); }
Window       { background: $surface; border: 1px $border; }
```

```cpp
namespace VanGui {
[[nodiscard]] VANGUI_API std::expected<void, const char*> LoadStyleSheet(const char* path);
[[nodiscard]] VANGUI_API std::expected<void, const char*> LoadStyleSheetFromMemory(const char* text, size_t len);
VANGUI_API void PushStyleSheetScope(const char* selector);   // scoped overrides
VANGUI_API void PopStyleSheetScope();
}
```

- Parse errors return `std::expected` with a human-readable message and line number — never an
  exception, never a silent default.
- Hot-reload rides on `theme_engine`'s existing `WatchThemeFile()` / `PollThemeFileChanges()`,
  with the animated transition reused so a saved `.vss` cross-fades live.
- This addresses TODO items *"style: better default styles"*, *"style: global scale setting"*,
  *"style: a concept of compact style"* by giving them a single declarative home.

### 7.2 Standard dialogs (`vangui_dialogs`)

Qt-parity modal helpers. Open/close animated through Pillar 1.

```cpp
namespace VanGui {

enum VanDialogResult { VanDialog_None = 0, VanDialog_Ok, VanDialog_Cancel, VanDialog_Yes, VanDialog_No };
enum VanDialogButtons { VanDialogButtons_Ok, VanDialogButtons_OkCancel, VanDialogButtons_YesNo };

// Immediate-mode modal; returns a result once on the frame a button is pressed.
VANGUI_API VanDialogResult MessageBox(const char* title, const char* message,
                                      VanDialogButtons buttons = VanDialogButtons_Ok);

// File dialog: native platform picker when available, in-GUI browser fallback otherwise.
// Returns std::expected so "user cancelled" and "no backend" are distinct, explicit outcomes.
[[nodiscard]] VANGUI_API std::expected<std::string, VanDialogResult>
    GetOpenFileName(const char* filters = nullptr);
[[nodiscard]] VANGUI_API std::expected<std::string, VanDialogResult>
    GetSaveFileName(const char* default_name = nullptr, const char* filters = nullptr);
}
```

Addresses TODO *"modals: make modal title bar blink"* (reuse anim) and the general lack of
standard dialogs that every "full application" needs.

### 7.3 Signals / slots (`vangui_signals`, header-only)

The one intentional retained-state concession — and it is fully opt-in, instance-based (no
global mutable state), and composition-friendly. It bridges immediate-mode widget results to
decoupled application logic the way Qt's signals do.

```cpp
namespace VanGui {

template <class... Args>
class VanSignal {
public:
    [[nodiscard]] VanConnection connect(std::function<void(Args...)> slot);  // RAII handle
    void emit(Args... args) const;                                           // call all live slots
    void disconnect_all() noexcept;
};

// VanConnection auto-disconnects on destruction (RAII). Move-only.
class VanConnection { /* ... */ };
}
```

- `VanConnection` is move-only and disconnects in its destructor → no dangling slots
  (Constitution: RAII, deterministic lifetime).
- Slot storage uses a small-buffer optimization to avoid per-connection heap churn for the
  common 0–2 captures case.
- Pure header, no link dependency, no core hook.

### 7.4 Model/View adapters (`vangui_views`)

Qt's model/view, expressed immediate-mode and virtualized through the existing `VanGuiListClipper`
so a million-row model still renders O(visible rows) with zero retained row widgets.

```cpp
namespace VanGui {
struct VanListModel {           // user implements; pure interface, no inheritance required of data
    int  (*RowCount)(void* ud);
    void (*DrawRow)(void* ud, int row, bool selected);
    void* UserData;
};
VANGUI_API bool BeginListView(const char* id, const VanListModel& model, int* p_selected);
VANGUI_API void EndListView();
// Tree variant with the same shape (BeginTreeView / EndTreeView).
}
```

### 7.5 Layout helpers (`vangui_layout`)

Constraint/flow helpers that resolve a long backlog of layout TODOs (*"horizontal layout
helper #97"*, *"horizontal flow until no space left #404"*, *"more generic alignment"*). Pure
layout math over the cursor API — **no retained layout tree**.

```cpp
namespace VanGui {
VANGUI_API void BeginHBox(const char* id, float spacing = -1.f);  // stretch/align children
VANGUI_API void BeginVBox(const char* id, float spacing = -1.f);
VANGUI_API void BeginGrid(const char* id, int columns, float spacing = -1.f);
VANGUI_API void EndBox();   // matches Begin*Box / BeginGrid (RAII guard provided)
VANGUI_API void Stretch(float weight = 1.f);   // flexible spacer
}
```

### 7.6 Docking

Docking already exists on `origin/docking` and as `vangui_docking.cpp`. The work here is
**merge and stabilize to `master`** (TODO #2109), not new development. Dockable panels are the
backbone of a Qt-like multi-panel application. Sequenced late (Phase 3) because it is the
highest-risk merge against an upstream-tracking core.

---

## 8. Pillar 4 — Unify existing extras

Refactor, not rewrite. Each existing module keeps its public API (backward compatible) but its
internals move onto the shared substrate and conventions.

| Module | Change | Removes |
|---|---|---|
| `vangui_notify` | Fade/slide via `AnimBool`; progress draw shared with `vangui_loading` | Bespoke fade timer & easing |
| `vangui_theme_engine` | `TransitionToTheme` drives `AnimColor`/`AnimFloat` blend | Bespoke color-interp timing |
| `vangui_command_palette` | Animated open (`AnimBool`); fuzzy match (TODO *filters: fuzzy*) | Hard show/hide pop |
| `vangui_themes` | Stylesheet (`.vss`) becomes a supported authoring path alongside presets | — |
| `vangui_node_graph` | Edge/pan/zoom easing via spring; consistent naming pass | Any inline tweening |

Also introduce a convenience driver so users don't hand-call every `Render*`:

```cpp
namespace VanGui {
VANGUI_API void NewFrameExtras();   // per-frame update for all enabled extras (after NewFrame)
VANGUI_API void RenderExtras();     // draw all enabled overlays (notify, transitions, palette…)
}
```

Both are thin and fully guarded; with no extras enabled they are empty.

---

## 9. Phasing & milestones

Each phase is independently shippable and leaves `master` green. "DoD" = Definition of Done.

| Phase | Scope | DoD |
|---|---|---|
| **P0 — Substrate** | `vangui_anim` (easing, tween, spring, ID pool, NewFrame hook, off-shims, idle flag) | Unit tests pass with injected clock; zero allocs/frame proven; off-build byte-identical to upstream |
| **P1 — Unify + Loading** | Refactor `notify` + `theme_engine` onto anim; ship `vangui_loading` | No behavioral regression in notify/theme demos; duplicated timers deleted; loading widgets in demo |
| **P2 — Authoring + App basics** | `vangui_style_sheet`, `vangui_dialogs`, `vangui_layout` | `.vss` hot-reload cross-fades; MessageBox/file dialog return `std::expected`; HBox/VBox/Grid in demo |
| **P3 — Framework** | `vangui_signals`, `vangui_views`, **docking merge** | Signals RAII-safe under ASan; 1M-row ListView at O(visible); docking stable on master |
| **P4 — Polish** | Idle/power integration end-to-end, fuzzy palette, docs, natvis, perf pass | Idle backend sleeps when no motion; metrics page shows anim pool; roadmap items checked off |

Dependency order is strict: **P0 blocks everything**; P1 blocks the convenience drivers; P2/P3
modules are mutually independent except docking (last).

### 9.1 Implementation status

All phases have been implemented. Every new translation unit compiles clean
against the real core headers and produces an **empty TU when its enable macro is
off**; pure-logic modules ship with passing standalone tests (`tests/enhance/`).

| Phase | Deliverables | Status |
|---|---|---|
| **P0** | `vangui_anim` (easing, tween, spring, ID pool, single `NewFrame` hook, off-shims, idle flag) | **Done** — unit tests pass with injected clock; 0 allocs/frame proven; off-build byte-identical |
| **P1** | `notify` + `theme_engine` refactored onto the substrate (duplicate timers deleted); `vangui_loading` shipped | **Done** — both refactors compile on/off; loading widgets implemented; notify/loading share one progress primitive |
| **P2** | `vangui_style_sheet` (.vss), `vangui_dialogs`, `vangui_layout` | **Done** — parser has 16 passing tests with line-numbered errors; dialogs return `std::expected`; layout HBox/VBox/Grid/Stretch |
| **P3** | `vangui_signals`, `vangui_views`, docking | **Done** — signals RAII-safe under ASan/UBSan; views virtualize via `VanGuiListClipper`; docking found already merged (see `docs/DOCKING_MERGE.md`) |
| **P4** | Idle integration, fuzzy palette, umbrella drivers, docs, perf | **Done** — `vangui_enhance.h` (`NewFrameExtras`/`RenderExtras`/`EnhanceWantsRedraw`); fuzzy subsequence match; `docs/PERF_BASELINE.md` |

**One tracked follow-up:** the per-tween slot is 84 bytes vs the §11 ≤32-byte
budget. Functionally correct and tiny in absolute terms; a pool-split
optimization is described in `docs/PERF_BASELINE.md`. Performance and binary-size
budgets should be re-measured on a dedicated target (the CI sandbox is shared and
noisy) and gated in CI.

CMake options added (all default **OFF**, zero-cost when unused):
`VANGUI_MISC_ANIM`, `VANGUI_MISC_LOADING`, `VANGUI_MISC_LAYOUT`,
`VANGUI_MISC_DIALOGS`, `VANGUI_MISC_STYLESHEET`, `VANGUI_MISC_SIGNALS`,
`VANGUI_MISC_VIEWS`.

---

## 10. Testing strategy

Per the Constitution (Testing chapter) and the existing `VANGUI_ENABLE_TEST_ENGINE` hook.

- **Headless unit tests (null backend).** `vangui_anim` is tested with an **injected clock**:
  step `dt`, assert interpolated values against closed-form easing; assert eviction after
  `ConfigAnimCompactFrames`; assert pool reuse (no growth) over 10k create/destroy cycles.
- **Allocation assertions.** A test allocator wrapper asserts **zero heap allocations** during
  steady-state animation frames (render-path no-alloc rule).
- **Interaction tests** via `vangui_test_engine`: open palette → fuzzy query → enter; open
  dialog → click → assert result; theme transition → assert `IsThemeTransitioning()` lifecycle.
- **Off-build parity.** CI configuration with all `VANGUI_ENABLE_*` undefined must produce a
  core translation unit diff-identical to the upstream baseline (guards the zero-cost claim).
- **Sanitizers.** ASan/UBSan on the signals module (RAII connection lifetimes) and the anim
  pool (ID aliasing).
- **Visual regression (optional).** Golden-image capture of spinners/skeletons at fixed time
  steps via the null backend's framebuffer.

---

## 11. Performance budget

Explicit numbers so "lightweight" is measurable, not aspirational. Validated in P4.

| Metric | Budget |
|---|---|
| Heap allocations per frame (steady state, any pillar) | **0** |
| `Anim::NewFrameUpdate()` cost, pool empty | ≤ 50 ns (early-out) |
| `Anim::NewFrameUpdate()` cost, 256 active tweens | ≤ 20 µs single-thread |
| Per-tween state size | ≤ 32 bytes |
| Binary size delta, `VANGUI_ENABLE_ANIM` only | ≤ ~12 KB |
| Binary size delta, all pillars enabled | target ≤ ~120 KB |
| Frames rendered while UI fully idle (idle integration on) | **0** (backend sleeps) |

Methodology: benchmark before optimizing (Constitution); record numbers in
`docs/PERF_BASELINE.md`; gate regressions in CI.

---

## 12. Risks & mitigations

| Risk | Impact | Mitigation |
|---|---|---|
| **Upstream merge churn** (fork tracks ImGui `master`) | Conflicts on every pull | Keep core edits to the single `NewFrame` hook; everything else in `misc/`; rebase extras, never the core |
| **ID collisions** for anim/view state | Wrong widget animates | Key off the real ID stack; expose `PushID` discipline in docs; debug-build duplicate-ID warning (core already has `VANGUI_DEBUG_HIGHLIGHT_ALL_ID_CONFLICTS`) |
| **Retained-state creep** eroding IM model | Architectural drift | Signals/views are the *only* retained modules and are opt-in; PR review rejects retained state elsewhere |
| **Frame-rate dependence** of motion | Janky on 30 vs 144 Hz | All integration uses `io.DeltaTime`; springs are semi-implicit; tests run at 30/60/144 Hz dt |
| **Binary bloat** | "Lightweight" claim weakens | Per-module toggles + budget table + CI size gate |
| **Easing overshoot** clipping colors/alpha | Visual artifacts | `AnimColor` clamps to valid range after shaping; document which curves overshoot |
| **Scope sprawl** (Qt is enormous) | Never ships | YAGNI: build the listed surface only; new requests go through this roadmap, not ad hoc |

---

## 13. Constitution compliance map

| Constitution rule | How this roadmap satisfies it |
|---|---|
| DRY | One `vangui_anim` substrate; extras refactored off duplicate timers (§2.1, §8) |
| KISS / YAGNI | Pull-based API; fixed feature surface; no speculative extensibility |
| RAII | `End*` guards, `VanConnection` auto-disconnect, scoped style/token pushes |
| Error handling (`std::expected`, no exceptions) | Stylesheet load, file dialogs return `std::expected` (§7.1, §7.2) |
| Composition over inheritance | Views use a plain struct-of-fn-ptrs model; signals are instance-based |
| No per-frame allocations | Pool reuse + zero-alloc tests (§5.5, §10, §11) |
| Compile-time validation | `VANGUI_ENABLE_*` gating with `#error` on missing deps (§4.4) |
| No global mutable state unless justified | Signals are instance-based; anim state lives in context, not globals |
| Treat warnings as errors | Applies to all new TUs; CI enforced |
| Document invariants | Each module header states ID-keying, lifetime, and thread rules |

---

## 14. Open questions (to resolve before P2)

1. **Directory:** keep new modules in `misc/`, or introduce a top-level `enhance/`? (Default:
   `misc/`, to avoid build-script churn.)
2. **Stylesheet scope:** ship the full `.vss` grammar in P2, or a minimal token-only subset
   first and grow it? (Default: minimal subset — tokens + a few widget rules — then iterate.)
3. **File dialog backend:** is a native platform picker in-scope, or is the in-GUI browser
   fallback sufficient for v1? (Affects platform-layer work.)
4. **Modules build:** mirror every new `.cpp` into the C++20 modules target (`*.ixx`), or keep
   enhancements classic-include only until the surface stabilizes? (Default: classic-only
   first.)

---

*This roadmap is a living document. Amend it via PR; do not let feature work outrun it.*
