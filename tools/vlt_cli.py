#!/usr/bin/env python3
"""vlt_cli.py - headless front-end for the vlt_viewer.py VPAK parser.

Same parsing core as vlt_viewer_gui.py (Tkinter), no GUI. Reads EA AttribSys
VPAK vaults (attributes.bin / FE_ATTRIB.bin / gameplay.bin / *.bun) and dumps
everything the GUI can show, plus a machine-readable schema census that the
dictionary resolver (vlt_dict_resolve.py) consumes.

Usage
  python vlt_cli.py FILE [FILE...] [section flags] [options]

Section flags (default = --summary):
  --summary            one line per vault (counts)
  --chunks             VLT + BIN chunk table
  --classes            class field-layout tables (offset/size/type/flags)
  --collections        collections with entries
  --databases          Attrib::DatabaseLoadData type-size tables
  --exports            raw export directory
  --strings            in-vault string table (with lookup2 hashes)
  --all                every section above

Census / data options:
  --census OUT.json    write the {fieldHash:{type,name}} schema census
                       (merges over every FILE; matches vault_schema_census.json)
  --dict FILE|DIR      hash-verified name table(s): 'name' or '0xHASH = name'
                       lines; only entries whose lookup2 matches are trusted
  --vault N            restrict to vault index N (multi-vault VPAKs)
  --json               emit the selected sections as JSON instead of text
  --hash STRING        just print lookup2(STRING) and exit
"""
import argparse
import glob
import json
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import vlt_viewer as V


# ── hash-verified external dictionary ────────────────────────────────────────
def load_dict(paths):
    """Return {hash:int -> name} from wordlists/hash=name files. Every entry is
    hash-verified: a candidate is trusted only if lookup2(name) reproduces it.
    Bare-name lines are hashed and indexed; '0xHASH = name' lines are accepted
    only when the declared hash agrees with our own lookup2 (bad seeds rejected)."""
    files = []
    for p in paths or []:
        if os.path.isdir(p):
            files += sorted(glob.glob(os.path.join(p, "*")))
        else:
            files.append(p)
    table = {}
    for f in files:
        if not os.path.isfile(f):
            continue
        try:
            text = open(f, "r", encoding="utf-8", errors="replace").read()
        except OSError:
            continue
        for line in text.splitlines():
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            name = line
            if line[:2].lower() == "0x" and any(c in line for c in "=:,\t"):
                # explicit hash=name — cross-check the source's own claim
                for sep in ("=", ":", ",", "\t"):
                    if sep in line:
                        _h, name = line.split(sep, 1)
                        name = name.strip().strip("\"'")
                        break
            # keep only the leading identifier token
            tok = ""
            for ch in name:
                if ch.isalnum() or ch == "_":
                    tok += ch
                else:
                    break
            if len(tok) < 3:
                continue
            for cand in {tok, tok.lower(), tok.upper()}:
                table.setdefault(V.vlt32(cand), cand)
    return table


def _resolve(v, h, extra):
    return V.name(v, h, extra)


# ── section printers ─────────────────────────────────────────────────────────
def dump_text(v, sections, extra):
    e = v.entry
    print(f"\n{'='*72}\nVAULT[{e.index}] '{e.name}'  "
          f"vlt@0x{e.vlt_offset:X}(0x{e.vlt_size:X})  "
          f"bin@0x{e.bin_offset:X}(0x{e.bin_size:X})")
    print(f"  strings={len(v.strings)} exports={len(v.exports)} "
          f"classes={len(v.classes)} collections={len(v.collections)} "
          f"databases={len(v.databases)} fixups(vlt/raw)="
          f"{len(v.vlt_fixups)}/{len(v.raw_fixups)}")
    if "chunks" in sections:
        print()
        V.print_chunks(v)
    if "exports" in sections:
        print(f"\nExports: {len(v.exports)}")
        for ex in v.exports:
            print(f"  {_resolve(v, ex.name_hash, extra):<32} "
                  f"kind={_resolve(v, ex.kind_hash, extra):<28} "
                  f"@0x{ex.offset:06X} size=0x{ex.size:X}")
    if "classes" in sections:
        print()
        _print_classes(v, extra)
    if "collections" in sections:
        print()
        _print_collections(v, extra)
    if "databases" in sections:
        print()
        V.print_databases(v)
    if "strings" in sections:
        print(f"\nStrings: {len(v.strings)}")
        for s in v.strings:
            print(f"  0x{V.vlt32(s):08X}  {s}")


def _print_classes(v, extra):
    print(f"Classes: {len(v.classes)}")
    for cd in v.classes:
        print(f"  class {_resolve(v, cd.name_hash, extra)} "
              f"(0x{cd.name_hash:08X}) baseSize=0x{cd.base_size:X} "
              f"defs={cd.num_definitions} reserve={cd.collection_reserve} "
              f"required={cd.required_count}")
        for f in cd.fields:
            print(f"    {_resolve(v, f.key, extra):<30} "
                  f"{_resolve(v, f.type_key, extra):<26} "
                  f"off=0x{f.offset:03X} size=0x{f.size:03X} n={f.max_count} "
                  f"flags={V.fmt_flags(f.flags)} align={f.alignment}")


def _print_collections(v, extra):
    print(f"Collections: {len(v.collections)}")
    for coll in v.collections:
        print(f"  coll {_resolve(v, coll.name_hash, extra)} "
              f"class={_resolve(v, coll.class_hash, extra)} "
              f"parent={_resolve(v, coll.parent_hash, extra)} "
              f"entries={coll.num_entries} "
              f"types={[_resolve(v, t, extra) for t in coll.types]}")
        for ent in coll.entries:
            print(f"    {_resolve(v, ent.key, extra):<30} ti={ent.type_index:<3} "
                  f"nf={V.fmt_nodeflags(ent.node_flags)} data={ent.data4.hex(' ')}")


# ── JSON view ────────────────────────────────────────────────────────────────
def vault_to_dict(v, sections, extra):
    e = v.entry
    out = {
        "index": e.index, "name": e.name,
        "vlt_offset": e.vlt_offset, "vlt_size": e.vlt_size,
        "bin_offset": e.bin_offset, "bin_size": e.bin_size,
        "counts": {"strings": len(v.strings), "exports": len(v.exports),
                   "classes": len(v.classes), "collections": len(v.collections),
                   "databases": len(v.databases)},
    }
    if "chunks" in sections:
        out["chunks"] = {
            "vlt": [{"tag": V.CHUNK_NAMES.get(c.cid, "?"), "offset": c.offset,
                     "size": c.size} for c in v.chunks],
            "bin": [{"tag": V.CHUNK_NAMES.get(c.cid, "?"), "offset": c.offset,
                     "size": c.size} for c in v.bin_chunks],
        }
    if "classes" in sections:
        out["classes"] = [{
            "name": _resolve(v, cd.name_hash, extra), "hash": f"0x{cd.name_hash:08X}",
            "base_size": cd.base_size, "num_definitions": cd.num_definitions,
            "fields": [{"key": _resolve(v, f.key, extra), "key_hash": f"0x{f.key:08X}",
                        "type": _resolve(v, f.type_key, extra), "offset": f.offset,
                        "size": f.size, "max_count": f.max_count,
                        "flags": f.flags, "align": f.alignment} for f in cd.fields],
        } for cd in v.classes]
    if "collections" in sections:
        out["collections"] = [{
            "name": _resolve(v, coll.name_hash, extra),
            "class": _resolve(v, coll.class_hash, extra),
            "parent": _resolve(v, coll.parent_hash, extra),
            "entries": coll.num_entries,
            "types": [_resolve(v, t, extra) for t in coll.types],
        } for coll in v.collections]
    if "databases" in sections:
        out["databases"] = [{"num1": d.num1, "num2": d.num2, "count": d.count,
                             "sizes": d.sizes} for d in v.databases]
    if "strings" in sections:
        out["strings"] = [{"hash": f"0x{V.vlt32(s):08X}", "s": s} for s in v.strings]
    if "exports" in sections:
        out["exports"] = [{"name": _resolve(v, ex.name_hash, extra),
                           "kind": _resolve(v, ex.kind_hash, extra),
                           "offset": ex.offset, "size": ex.size} for ex in v.exports]
    return out


# ── schema census (matches reference vault_schema_census.json) ───────────────
_TYPE_ALIAS = {
    "EA::Reflection::Float": "Float", "EA::Reflection::Int32": "Int32",
    "EA::Reflection::UInt32": "UInt32", "EA::Reflection::Bool": "Bool",
    "EA::Reflection::Int16": "Int16", "EA::Reflection::UInt16": "UInt16",
    "EA::Reflection::Int8": "Int8", "EA::Reflection::UInt8": "UInt8",
    "Attrib::StringKey": "StringKey", "Attrib::Gen::Collection": "GCollectionKey",
}


def _type_label(v, h, extra):
    s = V.name(v, h, extra)
    return _TYPE_ALIAS.get(s, s)


def build_census(vaults, extra):
    """Distinct field descriptors across all vaults: hash -> {type, name}.
    name is the hash-verified resolution (in-vault string or trusted dict) or
    None when the source string was stripped at compile time."""
    fields = {}
    for v in vaults:
        for cd in v.classes:
            for f in cd.fields:
                key = f"{f.key:08x}"
                nm = v.hash_names.get(f.key) or (extra.get(f.key) if extra else None)
                rec = fields.setdefault(key, {"type": _type_label(v, f.type_key, extra),
                                              "name": None})
                if nm and not rec["name"]:
                    rec["name"] = nm
    resolved = sum(1 for r in fields.values() if r["name"])
    return {"total": len(fields), "resolved": resolved,
            "fields": dict(sorted(fields.items()))}


# ── main ─────────────────────────────────────────────────────────────────────
def main(argv=None):
    ap = argparse.ArgumentParser(description="headless VPAK vault dumper")
    ap.add_argument("files", nargs="*")
    ap.add_argument("--summary", action="store_true")
    ap.add_argument("--chunks", action="store_true")
    ap.add_argument("--classes", action="store_true")
    ap.add_argument("--collections", action="store_true")
    ap.add_argument("--databases", action="store_true")
    ap.add_argument("--exports", action="store_true")
    ap.add_argument("--strings", action="store_true")
    ap.add_argument("--all", action="store_true")
    ap.add_argument("--census", metavar="OUT.json")
    ap.add_argument("--dict", action="append", metavar="FILE|DIR")
    ap.add_argument("--vault", type=int, default=None)
    ap.add_argument("--json", action="store_true")
    ap.add_argument("--hash", metavar="STRING")
    args = ap.parse_args(argv)

    if args.hash is not None:
        print(f"0x{V.vlt32(args.hash):08X}  {args.hash}")
        return 0
    if not args.files:
        ap.error("no input files")

    sections = []
    for s in ("chunks", "classes", "collections", "databases", "exports", "strings"):
        if getattr(args, s) or args.all:
            sections.append(s)

    extra = load_dict(args.dict)
    if extra:
        print(f"# dictionary: {len(extra)} hash-verified names loaded",
              file=sys.stderr)

    all_vaults = []
    json_out = []
    for path in args.files:
        try:
            data = open(path, "rb").read()
            entries = V.parse_vpak(data)
        except Exception as ex:  # noqa: BLE001
            print(f"!! {path}: {ex}", file=sys.stderr)
            continue
        if args.vault is not None:
            entries = [e for e in entries if e.index == args.vault]
        vaults = [V.load_vault(e) for e in entries]
        all_vaults += vaults
        if args.json:
            json_out.append({"file": os.path.basename(path),
                             "vaults": [vault_to_dict(v, sections or ["summary"], extra)
                                        for v in vaults]})
        else:
            print(f"\n#### {path}  ({len(vaults)} vault"
                  f"{'s' if len(vaults) != 1 else ''})")
            for v in vaults:
                dump_text(v, sections, extra)

    if args.json and not args.census:
        print(json.dumps(json_out, indent=1))

    if args.census:
        cen = build_census(all_vaults, extra)
        cen_full = {
            "note": "Schema-field census regenerated by vlt_cli.py from the live "
                    "VPAK vault(s) via vlt_viewer.py. 16-byte {fieldHash,typeHash} "
                    "descriptor rows, runs>=3. name=null => source string stripped "
                    "at compile time (recover via external hash-verified dictionary).",
            "sources": [os.path.basename(p) for p in args.files],
            **cen,
        }
        with open(args.census, "w", encoding="utf-8") as fh:
            json.dump(cen_full, fh, indent=1)
        print(f"# census: {cen['total']} distinct fields, "
              f"{cen['resolved']} named -> {args.census}", file=sys.stderr)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
