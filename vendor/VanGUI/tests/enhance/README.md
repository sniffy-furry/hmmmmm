# Enhancement-suite verification tests

Standalone tests for the opt-in enhancement layer (`misc/vangui_*`). They do not
need a windowing backend: pure-logic modules run directly, and the animation
substrate links against a handful of stub symbols.

Run from the repository root:

```
bash tests/enhance/run.sh
```

| Test | Covers |
|---|---|
| `../../\_anim_test.cpp` | Pillar 1 substrate: easing, tween convergence, AnimBool range, spring settle, slot eviction/reuse, idle flag |
| `test_signals.cpp` | Pillar 3 signals: emit, RAII disconnect, outlive-signal safety, `release()`, `disconnect_all` (built under ASan/UBSan) |
| `test_stylesheet_parser.cpp` | Pillar 3 `.vss` parser: vars, `$ref`, `lighten()/darken()`, hex forms, token mapping, line-numbered errors |
| `test_fuzzy.cpp` | Pillar 4 command-palette fuzzy subsequence scorer ranking |
| `bench_anim.cpp` | Pillar 1 performance: empty-pool early-out, 256-tween frame cost, idle flag |

Modules that depend on the VanGUI draw/widget API (`vangui_loading`,
`vangui_dialogs`, `vangui_layout`, `vangui_views`, `vangui_style_sheet` apply
path) are verified by compiling each translation unit clean against the real
headers (`g++ -c`), since exercising them at runtime needs a live backend.
