#!/usr/bin/env python3
"""
vlt_dict_resolve.py - MW attribute-hash dictionary ingest / resolver.

The naming wall: NFS:MW's ~747 vault schema-field names (and many collection keys)
are EA::Reflection strings stripped at compile time - they exist in the retail build
only as lookup2/0xABCDEF00 hashes and appear as a string in NO shipped file. They
cannot be brute-forced (>1.2M English combos = ~2 hits). The one file-side way through
is an EXTERNAL DICTIONARY: any list of candidate names (VltEd/Binary/community field
lists, EA symbol dumps, your own guesses). This tool hashes every candidate - with
case/format variants - and matches against the unresolved hashes.

USAGE
  # dry run: report what a wordlist would resolve
  python vlt_dict_resolve.py path/to/wordlist.txt

  # ingest every file in the drop folder
  python vlt_dict_resolve.py --drop

  # actually LAND confirmed names into the encyclopedia data files
  python vlt_dict_resolve.py --drop --apply

  # resolve against a custom target hash list instead of the census
  python vlt_dict_resolve.py words.txt --targets 0x1234abcd,0xdeadbeef

INPUT FORMATS (auto-detected, all mixed freely)
  - plain name per line:            TopSpeedMultiplier
  - "hash = name" (VltEd/editor):   0xEC57E16B = TopSpeedMultiplier
  - "name = value" / CSV / TSV:     TopSpeedMultiplier,Float,1.0
  - VltEd XML-ish / quoted:         <Field name="TopSpeedMultiplier" .../>
  - anything: every [A-Za-z_][A-Za-z0-9_]{2,63} token on each line is tried

Every token is tried as-is AND lower/UPPER/snake_case/UPPER_SNAKE/CamelCase.
Matches are exact 32-bit hash equality (collision prob ~ targets*tokens / 2^32).
"""
import sys, os, re, json, argparse, glob

# ---- engine hash (Bob Jenkins lookup2, seed 0xABCDEF00) -- verified vs speed.exe ----
M = 0xFFFFFFFF
def _mix(a, b, c):
    a=(a-b)&M; a=(a-c)&M; a^=(c>>13)
    b=(b-c)&M; b=(b-a)&M; b^=(a<<8)&M
    c=(c-a)&M; c=(c-b)&M; c^=(b>>13)
    a=(a-b)&M; a=(a-c)&M; a^=(c>>12)
    b=(b-c)&M; b=(b-a)&M; b^=(a<<16)&M
    c=(c-a)&M; c=(c-b)&M; c^=(b>>5)
    a=(a-b)&M; a=(a-c)&M; a^=(c>>3)
    b=(b-c)&M; b=(b-a)&M; b^=(a<<10)&M
    c=(c-a)&M; c=(c-b)&M; c^=(b>>15)
    return a&M, b&M, c&M
def lookup2(key, initval=0xABCDEF00):
    if isinstance(key, str): key = key.encode('latin1')
    a = b = 0x9E3779B9; c = initval & M
    length = len(key); k = 0; ln = length
    while ln >= 12:
        a=(a+(key[k]|key[k+1]<<8|key[k+2]<<16|key[k+3]<<24))&M
        b=(b+(key[k+4]|key[k+5]<<8|key[k+6]<<16|key[k+7]<<24))&M
        c=(c+(key[k+8]|key[k+9]<<8|key[k+10]<<16|key[k+11]<<24))&M
        a,b,c=_mix(a,b,c); k+=12; ln-=12
    c=(c+length)&M
    if ln>=11: c=(c+(key[k+10]<<24))&M
    if ln>=10: c=(c+(key[k+9]<<16))&M
    if ln>=9:  c=(c+(key[k+8]<<8))&M
    if ln>=8:  b=(b+(key[k+7]<<24))&M
    if ln>=7:  b=(b+(key[k+6]<<16))&M
    if ln>=6:  b=(b+(key[k+5]<<8))&M
    if ln>=5:  b=(b+key[k+4])&M
    if ln>=4:  a=(a+(key[k+3]<<24))&M
    if ln>=3:  a=(a+(key[k+2]<<16))&M
    if ln>=2:  a=(a+(key[k+1]<<8))&M
    if ln>=1:  a=(a+key[k])&M
    a,b,c=_mix(a,b,c)
    return c

# ---- paths ----
HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)                                  # MWTools/
DATA = os.path.join(ROOT, "reference", "MWEncyclopedia", "RE-Data-And-Discoveries", "data")
DROP = os.path.join(HERE, "vlt_dictionaries")                 # where users drop wordlists
CENSUS = os.path.join(DATA, "vault_schema_census.json")
FIELDNAMES = os.path.join(DATA, "vault_field_names.json")

TOKEN = re.compile(r'[A-Za-z_][A-Za-z0-9_]{2,63}')

def variants(w):
    """Every plausible spelling of a token EA might have used."""
    out = {w, w.lower(), w.upper()}
    # CamelCase -> snake and UPPER_SNAKE
    snake = re.sub(r'(?<!^)(?=[A-Z])', '_', w)
    out |= {snake.lower(), snake.upper()}
    # snake_case -> CamelCase and lowerCamel
    if '_' in w:
        parts = [p for p in w.split('_') if p]
        if parts:
            out.add(''.join(p.capitalize() for p in parts))
            out.add(parts[0].lower() + ''.join(p.capitalize() for p in parts[1:]))
    return out

def load_targets(census_only=True, extra=None):
    """Map hash(int) -> type string, for every UNRESOLVED hash we care about."""
    targets = {}
    already = set()
    if os.path.exists(FIELDNAMES):
        for h in json.load(open(FIELDNAMES)).get("resolved_fields", {}):
            already.add(int(h, 16))
    if os.path.exists(CENSUS):
        cen = json.load(open(CENSUS))
        for h, v in cen["fields"].items():
            hi = int(h, 16)
            if not v.get("name") and hi not in already:
                targets[hi] = v.get("type", "?")
    # unresolved collection keys tracked in residual data files (skip already-named)
    for fn in ("ai_rubberband.json", "pursuit_escalation.json", "damage_zones.json"):
        p = os.path.join(DATA, fn)
        if not os.path.exists(p): continue
        blob = open(p).read()
        for m in re.finditer(r'0x([0-9A-Fa-f]{8})', blob):
            hi = int(m.group(1), 16)
            if hi not in already:
                targets.setdefault(hi, "collection?")
    if extra:
        for h in extra:
            targets.setdefault(h, "custom")
    return targets

def parse_hash_name_pairs(text):
    """Explicit '0xHASH = name' lines let us confirm the source's own hash too."""
    pairs = {}
    for line in text.splitlines():
        m = re.match(r'\s*0x([0-9A-Fa-f]{8})\s*[=:,\t]\s*(.+?)\s*$', line)
        if m:
            nm = m.group(2).strip().strip('"\'')
            nm = TOKEN.match(nm).group(0) if TOKEN.match(nm) else nm
            pairs[int(m.group(1), 16)] = nm
    return pairs

def harvest_tokens(text):
    return set(TOKEN.findall(text))

def resolve(files, targets):
    hits = {}          # hash -> name
    provenance = {}    # hash -> (name, source_file, matched_form)
    declared_ok = 0    # source said 0xH=name AND our lookup2 agrees
    declared_bad = []  # source said 0xH=name but our hash disagrees (bad source / wrong seed)
    tokens_tried = set()
    for f in files:
        try: text = open(f, 'r', encoding='utf-8', errors='replace').read()
        except Exception: continue
        base = os.path.basename(f)
        # 1) explicit hash=name pairs: cross-check the source's own claims
        for h, nm in parse_hash_name_pairs(text).items():
            if lookup2(nm) == h:
                declared_ok += 1
                if h in targets and h not in hits:
                    hits[h] = nm; provenance[h] = (nm, base, "declared+verified")
            else:
                declared_bad.append((h, nm, lookup2(nm)))
        # 2) harvest every token, try all variants
        for tok in harvest_tokens(text):
            for v in variants(tok):
                tokens_tried.add(v)
                h = lookup2(v)
                if h in targets and h not in hits:
                    hits[h] = v; provenance[h] = (v, base, "token-variant")
    return hits, provenance, declared_ok, declared_bad, len(tokens_tried)

def land(hits, provenance):
    """Write confirmed names into vault_field_names.json + vault_schema_census.json."""
    import struct
    cen = json.load(open(CENSUS))
    vf = json.load(open(FIELDNAMES))
    rf = vf["resolved_fields"]
    attrib = None
    for cand in (os.path.join(ROOT, "reference", "mw", "GLOBAL", "ATTRIBUTES.BIN"),):
        if os.path.exists(cand): attrib = open(cand, 'rb').read()
    def samples(hh):
        if not attrib: return []
        out = []; pp = struct.pack('<I', hh); i = -1
        while len(out) < 6:
            i = attrib.find(pp, i+1)
            if i < 0: break
            t, = struct.unpack_from('<I', attrib, i+8)
            if t & 0xFFFFFF00 == 0x00200000:
                v, = struct.unpack_from('<f', attrib, i+4); out.append(round(v, 4))
        return out
    added = 0
    for h, nm in hits.items():
        hx = "%08x" % h
        if hx in cen["fields"] and not cen["fields"][hx]["name"]:
            cen["fields"][hx]["name"] = nm
        if hx not in rf:
            typ = cen["fields"].get(hx, {}).get("type")
            rf[hx] = {"name": nm, "type": typ, "sample_values": samples(h),
                      "recovered": f"dictionary-ingest ({provenance[h][1]}); lookup2/0xABCDEF00 exact match"}
            added += 1
    cen["resolved"] = sum(1 for v in cen["fields"].values() if v["name"])
    vf["resolved_fields"] = dict(sorted(rf.items()))
    json.dump(cen, open(CENSUS, 'w'), indent=0)
    json.dump(vf, open(FIELDNAMES, 'w'), indent=1)
    return added

def main():
    ap = argparse.ArgumentParser(description="MW vault-hash dictionary resolver")
    ap.add_argument("wordlists", nargs="*", help="wordlist file(s)")
    ap.add_argument("--drop", action="store_true", help="ingest everything in tools/vlt_dictionaries/")
    ap.add_argument("--apply", action="store_true", help="write confirmed names into the encyclopedia data")
    ap.add_argument("--targets", help="comma-separated extra target hashes (0x...)")
    args = ap.parse_args()

    files = list(args.wordlists)
    if args.drop:
        os.makedirs(DROP, exist_ok=True)
        files += [p for p in glob.glob(os.path.join(DROP, "**", "*"), recursive=True)
                  if os.path.isfile(p) and not p.endswith(".md")]
    if not files:
        print("No wordlists given. Drop files in:", DROP)
        print("Then run:  python vlt_dict_resolve.py --drop [--apply]")
        return
    extra = [int(x, 16) for x in args.targets.split(",")] if args.targets else None
    targets = load_targets(extra=extra)
    print(f"[targets] {len(targets)} unresolved hashes loaded")
    print(f"[input]   {len(files)} file(s)")
    hits, prov, dok, dbad, ntok = resolve(files, targets)
    print(f"[hashed]  {ntok} distinct token-variants")
    if dok:  print(f"[verify]  {dok} source 'hash=name' pairs re-hash correctly (good source)")
    if dbad: print(f"[verify]  WARNING {len(dbad)} pairs DISAGREE with lookup2 (wrong hash/seed?): e.g. {dbad[:3]}")
    print(f"[HITS]    {len(hits)} unresolved hashes newly named:\n")
    for h, nm in sorted(hits.items(), key=lambda x: x[1]):
        t, src, how = prov[h]
        print(f"  0x{h:08X}  {nm:28s}  [{how}, {src}]")
    if not hits:
        print("  (none - try a larger/more MW-specific dictionary)")
    if args.apply and hits:
        n = land(hits, prov)
        print(f"\n[LANDED]  {n} names written to vault_field_names.json + vault_schema_census.json")
    elif hits:
        print(f"\n(dry run - re-run with --apply to write these into the encyclopedia)")

if __name__ == "__main__":
    main()
