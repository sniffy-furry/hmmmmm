# Docking — merge & stabilization assessment (Roadmap P3 / TODO #2109)

**Status: structurally merged into `master`; remaining work is stabilization, not a branch merge.**

## What the audit found

Contrary to the roadmap's "not merged to master (TODO #2109)" note, docking is
already integrated into the core build of `master`:

- `vangui_docking.cpp` (47 KB) implements the full dock system inside
  `namespace VanGui` — `DockContext*` node management, splitting/merging, window
  docking/undocking, rendering/hit-testing, and the public entry points
  `DockSpace()` and `DockSpaceOverViewport()`.
- It is compiled directly into the core `vangui` static library
  (`CMakeLists.txt`, in `add_library(vangui STATIC … vangui_docking.cpp)`).
- The public API is declared in `vangui.h` (`DockSpace`, `DockSpaceOverViewport`,
  `VanGuiDockNodeFlags`, the `VanGuiConfigFlags_DockingEnable` opt-in flag).
- The C++20 module target exposes it through the `vangui-docking.ixx` partition,
  with the implementation in `vangui-docking-impl.cpp`.

In other words, the docking *code* is on `master` and builds as part of the
core. The work item is therefore **stabilization**, not "merge the branch."

## On `origin/docking`

`git rev-list --left-right --count master...origin/docking` reports `7 / 1789`.
The 1789 commits on the `origin/docking` side are the upstream Dear ImGui
*docking line* (the long-lived upstream branch the fork tracks separately). This
is **not** a feature branch to merge wholesale into `master`:

- Wholesale merging would drag 1789 upstream commits across the renamed `Van*`
  core and almost certainly conflict massively.
- The fork's policy (Roadmap §12, "rebase extras, never the core; keep core edits
  minimal") means upstream docking fixes should be **cherry-picked** into
  `vangui_docking.cpp` as needed, the same way other upstream changes are folded
  into the renamed core — not merged as a branch.

## Remaining stabilization work (cannot be completed in a headless sandbox)

These require the full toolchain + a windowing backend and a running app, so they
are tracked here for the maintainer rather than performed in this pass:

1. **Build the docking config.** Configure with a backend (e.g. GLFW+OpenGL3) and
   `io.ConfigFlags |= VanGuiConfigFlags_DockingEnable`; confirm `vangui_docking.cpp`
   compiles clean against the current `master` core (the texture-system rework
   noted in the roadmap audit is the most likely source of drift).
2. **Runtime smoke tests:** dock a window into a `DockSpace`, split a node,
   undock by drag, re-dock, and close — verify no asserts in
   `DockContextProcess*`.
3. **Persistence:** save/load `.ini`, confirm dock layout round-trips
   (`DockSettingsHandler`).
4. **Multi-viewport interaction** (if viewports are enabled): dock across
   platform windows.
5. **Module parity:** confirm `vangui-docking-impl.cpp` (module build) and
   `vangui_docking.cpp` (classic build) stay in lockstep — they are two entry
   points to the same implementation and must not diverge.

## Recommendation

Close TODO #2109's "merge" framing: docking is merged. Re-file the residual as a
**docking stabilization** checklist (items 1–5 above), gated behind the existing
`VanGuiConfigFlags_DockingEnable` opt-in so the default build is unaffected. Keep
folding upstream docking fixes by cherry-pick, never by merging `origin/docking`.
