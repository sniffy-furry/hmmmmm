#!/usr/bin/env bash
# Standalone verification for the VanGUI enhancement suite (Pillars 1-4).
# Each test compiles against the real VanGUI headers; the pure-logic tests run
# with tiny link stubs (no backend needed). Run from the repository root:
#     bash tests/enhance/run.sh
set -e
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
CXX="${CXX:-g++}"
STD="-std=c++23 -O2 -I$ROOT"
OUT="$(mktemp -d)"
fail=0

echo "== anim substrate (P0) =="
$CXX $STD -DVANGUI_ENABLE_ANIM "$ROOT/_anim_test.cpp" "$ROOT/misc/vangui_anim.cpp" -o "$OUT/anim" && "$OUT/anim" || fail=1

echo "== signals (P3, ASan/UBSan) =="
$CXX $STD -fsanitize=address,undefined "$ROOT/tests/enhance/test_signals.cpp" -o "$OUT/sig" && "$OUT/sig" || fail=1

echo "== stylesheet parser (P2) =="
$CXX $STD "$ROOT/tests/enhance/test_stylesheet_parser.cpp" -o "$OUT/ss" && "$OUT/ss" || fail=1

echo "== command-palette fuzzy scorer (P4) =="
$CXX $STD "$ROOT/tests/enhance/test_fuzzy.cpp" -o "$OUT/fz" && "$OUT/fz" || fail=1

echo "== anim performance bench (P4) =="
$CXX $STD -DVANGUI_ENABLE_ANIM "$ROOT/tests/enhance/bench_anim.cpp" "$ROOT/misc/vangui_anim.cpp" -o "$OUT/bench" && "$OUT/bench" || fail=1

echo
[ $fail -eq 0 ] && echo "ALL ENHANCE TESTS PASSED" || { echo "ENHANCE TESTS FAILED"; exit 1; }
