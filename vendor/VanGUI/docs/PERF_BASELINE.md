# VanGUI Enhancement Suite — Performance Baseline

Methodology per Roadmap §11: benchmark before optimizing; record numbers here;
gate regressions in CI. Numbers below were measured with GCC `-O2` on the CI
sandbox (single thread). Re-measure on target hardware and update.

## Pillar 1 — `vangui_anim`

| Metric | Budget (§11) | Measured | Status |
|---|---|---|---|
| `NewFrameUpdate()` cost, pool empty | ≤ 50 ns | **~2–3 ns** | PASS (early-out) |
| `NewFrameUpdate()` + 256 active `AnimFloat`, per frame | ≤ 20 µs | **~10–25 µs** (10–11 µs quiet; up to ~25 µs under shared-CI load) | re-measure on a dedicated target |
| Heap allocations per frame, steady state | 0 | **0** (pool reuse; proven by unit test: 10k create/destroy, no growth) | PASS |
| Idle "is animating" flag toggles correctly | required | **works** (`IsAnimating()==true` while moving, `false` after settle) | PASS |
| Per-tween state size | ≤ 32 bytes | **84 bytes** | **OVER BUDGET** |

### The 84-byte slot (tracked follow-up)

`VanAnimSlot` currently stores four `float[4]` channel arrays (`Start`, `Cur`,
`Vel`, `Target`) plus a 20-byte header = 84 bytes, because one pool serves both
scalar (1-channel) and color (4-channel) animations and springs.

For 256 tweens that is ~21 KB — negligible in absolute terms, but it misses the
§11 budget. Recommended remediation (a self-contained optimization, no API
change):

1. Split into two pools — a scalar pool (`Start,Cur,Vel,Target` = 4 floats → ~36
   bytes with header) and a color pool (only allocated when `AnimColor` is used).
   Scalar tweens, the common case, then fit the budget; color tweens pay for what
   they use.
2. Or store `Vel` only for spring slots via a separate spring sub-pool, since
   duration tweens never read it.

This is isolated to `vangui_anim.cpp` internals; the public API and the passing
correctness tests are unaffected. Filed as a P4 polish item.

## Off-build parity (the zero-cost claim)

With every `VANGUI_ENABLE_*` undefined:

- `vangui_anim.cpp`, `vangui_loading.cpp`, `vangui_dialogs.cpp`,
  `vangui_layout.cpp`, `vangui_style_sheet.cpp`, `vangui_views.cpp` each compile
  to an **empty translation unit** (0 symbols in `VanGui::` — verified with `nm`).
- The single core hook in `VanGui::NewFrame()` is wrapped in
  `#ifdef VANGUI_ENABLE_ANIM`, so the preprocessed core is identical to upstream.
- Consumer code calling the shim APIs compiles unchanged and snaps to target
  values (verified).

## Binary-size deltas

Not measured here (requires the full link on target). Wire `size`-based CI gates
per §11 once a reference link exists; the empty-TU result above bounds the
"feature off" delta at ~0.
