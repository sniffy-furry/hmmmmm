#!/bin/sh
# mine_names.sh — first-party name mining driver (no external dictionaries).
#
#   1. builds tools/mwhashminer.c
#   2. emits the unresolved-hash target list from the encyclopedia census
#   3. runs `strings` over EVERY retail file under reference/mw and pipes
#      through the miner
#   4. writes hits (hash, matched string, algorithm, file, offset) to
#      mined.tsv for review — nothing is auto-applied
#
# Usage:  sh tools/mine_names.sh [game_root] [out.tsv]
set -e
ROOT="${1:-reference/mw}"
OUT="${2:-mined.tsv}"
HERE="$(cd "$(dirname "$0")" && pwd)"
BIN=/tmp/mwhashminer
TGT=/tmp/mine_targets.txt

cc -O2 -o "$BIN" "$HERE/mwhashminer.c"

python3 - "$HERE/../reference/MWEncyclopedia/RE-Data-And-Discoveries/data" "$TGT" << 'EOF'
import json, sys, os
data, out = sys.argv[1], sys.argv[2]
targets=set()
cen=json.load(open(os.path.join(data,'vault_schema_census.json')))
for h in cen['fields']: targets.add(int(h,16))
try:
    sch=json.load(open(os.path.join(data,'gameplay_race_schema.json')))
    for f in sch['fields_in_file_order']: targets.add(int(f['hash'],16))
except FileNotFoundError: pass
with open(out,'w') as f:
    for t in sorted(targets): f.write('0x%08x\n'%t)
print('targets:',len(targets))
EOF

: > "$OUT"
find "$ROOT" -type f | while read -r f; do
    strings -a -t x -n 3 "$f" 2>/dev/null | "$BIN" "$TGT" "$f" >> "$OUT"
done
echo "hits: $(wc -l < "$OUT") -> $OUT"
echo "review with: cut -f1,2 $OUT | sort -u"
