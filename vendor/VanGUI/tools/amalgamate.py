#!/usr/bin/env python3
"""Amalgamate the header-only VanGUI utilities into a single drop-in header.

Produces `van_kit.h`: one include that pulls in the header-only parts of the
enhancement suite (undo, settings, theme generator, reflection, signals). The
COMPILED modules (anim, loading, dialogs, charts, etc.) still need their .cpp in
the build — this kit is for the zero-link header utilities only.

Usage:  python3 tools/amalgamate.py [output_path]
"""
import os, re, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
# Order matters: theme_gen depends on theme_engine.h (kept as a normal include).
HEADERS = [
    "misc/vangui_signals.h",
    "misc/vangui_undo.h",
    "misc/vangui_settings.h",
    "misc/vangui_theme_gen.h",
    "misc/vangui_reflect.h",
]
OUT = sys.argv[1] if len(sys.argv) > 1 else os.path.join(ROOT, "van_kit.h")

sys_includes = set()
bodies = []
local_inc = re.compile(r'^\s*#\s*include\s*"(.*)"')
sys_inc   = re.compile(r'^\s*#\s*include\s*<(.*)>')

# Basenames provided by the kit prologue or amalgamated below -> safe to drop.
provided = {os.path.basename(h) for h in HEADERS}
provided |= {"vangui.h", "vangui_theme_engine.h"}

for h in HEADERS:
    with open(os.path.join(ROOT, h), encoding="utf-8") as f:
        out_lines = []
        for line in f:
            if line.strip() == "#pragma once":
                continue
            m = local_inc.match(line)
            if m:
                base = os.path.basename(m.group(1))
                if base in provided:
                    continue          # provided by the kit; drop
                out_lines.append(line.rstrip("\n"))   # keep other local includes
                continue
            s = sys_inc.match(line)
            if s:
                sys_includes.add(s.group(1))
                continue
            out_lines.append(line.rstrip("\n"))
        bodies.append((h, "\n".join(out_lines)))

with open(OUT, "w", encoding="utf-8") as o:
    o.write("// van_kit.h - amalgamated header-only VanGUI utilities (generated).\n")
    o.write("// Regenerate with: python3 tools/amalgamate.py\n")
    o.write("// NOTE: compiled modules (anim/loading/dialogs/charts/...) still need their .cpp.\n")
    o.write("#pragma once\n")
    o.write('#include "vangui.h"\n')
    o.write('#include "misc/vangui_theme_engine.h"  // for theme_gen token types\n')
    for inc in sorted(sys_includes):
        o.write(f"#include <{inc}>\n")
    o.write("\n")
    for name, body in bodies:
        o.write(f"// ===== {name} " + "=" * (66 - len(name)) + "\n")
        o.write(body)
        o.write("\n\n")
print(f"wrote {OUT} ({sum(len(b) for _,b in bodies)} bytes of body)")
