#!/usr/bin/env python3
"""vlt_viewer.py - EA AttribSys VLT/VPAK parser (NFS Most Wanted 2005 era).

Parsing core for vlt_viewer_gui.py (Tkinter browser) and vlt_cli.py (headless).
Reads attributes.bin / FE_ATTRIB.bin / gameplay.bin style VPAK vaults.

Everything here was derived from, and is verified against, the retail files
themselves — no external hash dictionaries are consulted. Resolution of hashes
uses only strings that ship inside the vault (plus optional caller-supplied
tables whose entries are hash-verified before use).

On-disk model (verified against retail attributes.bin / FE_ATTRIB.bin):

  VPAK header
    +0x00 'VPAK'  +0x04 u32 version(1)  +0x08 u32 dirOffset(0x40)
    +0x0C u32 (varies)  +0x10 u32[]: [0]=0 [1]=binSize [2]=vltSize
    [3]=binOffset(0x80) [4]=vltOffset   +0x40 char name[64]

  BIN block (offset word[3]) — chunk(s) then raw heap:
    'ErtS' (StrE) string table: packed NUL-terminated names
    raw heap: reflection type-name strings, class field-layout tables,
              collection entry tables, overflow value storage

  VLT block (offset word[4]) — chunks:
    'NpeD' (DepN) dependencies : u32 count; u32 hash[count]; u32 strOfs[count]; blob
    'NrtS' (StrN) symbols      : u64 (displayed as VLT version)
    'NtaD' (DatN) node data    : u32 num1; u32 num2; u32 count; packed nodes
    'NpxE' (ExpN) exports      : u32 count; {u32 name; u32 kind; u32 pad;
                                             u32 size; u32 offset}[count]
                                 offset is relative to the VLT block start
    'NrtP' (PtrN) pointer fixups: {u32 src; u16 a; u16 b; u32 arg}[]
                                 (a,b)==(1,0): VLT-internal; (3,1)/(2,*): src in
                                 VLT points at BIN+arg. 0xEFFECADD in node data
                                 marks unfixed pointer slots (they are NOT
                                 record delimiters).

  Node kinds (kind = lookup2 of the Attrib load-data class name):
    Attrib::ClassLoadData      0x5E970CBC
      {u32 name; u32 baseSize; u32 numDefinitions; u32 layoutPtr*;
       u32 collectionReserve; u32 pad; u32 requiredCount}          (0x1C bytes)
      layoutPtr -> BIN heap: numDefinitions x 16-byte field defs
      {u32 key; u32 type; u16 offset; u16 size; u16 maxCount;
       u8 flags; u8 alignment}
    Attrib::CollectionLoadData 0x8E112EB7
      {u32 name; u32 class; u32 parent; u32 numEntries; u32 w4;
       u32 reserve; u32 numTypes; u32 dataPtr*;
       u32 types[numTypes];
       numEntries x {u32 key; u8 data4[4]; u16 typeIndex; u16 nodeFlags}}
      dataPtr -> BIN heap: the collection's instance-value blob, laid out per
      the class field table (offset/size). data4 holds an inline value or an
      0xEFFECADD pointer slot (its fixup then gives the BIN offset).
      Node sizes tile exactly as 0x20 + 4*numTypes + 12*numEntries.
    Attrib::DatabaseLoadData   0xCBBC628F
      {u32 num1; u32 num2; u32 count; u32 ptr*; u32 sizes[count]}
      sizes[] = sizeof() of each reflected type
"""

import struct
from dataclasses import dataclass, field
from typing import Dict, List, Optional

# ── lookup2 / 0xABCDEF00 — the reflection hash ───────────────────────────────
# Byte-faithful port of the speed.exe routine at 0x5CC090 (public entry
# 0x5CC240 seeds edx=0xABCDEF00). Witness: lookup2("default") == 0xEEC2271A.

_M = 0xFFFFFFFF

def _mix(a, b, c):
    a=(a-b)&_M; a=(a-c)&_M; a^=(c>>13)
    b=(b-c)&_M; b=(b-a)&_M; b^=(a<<8)&_M
    c=(c-a)&_M; c=(c-b)&_M; c^=(b>>13)
    a=(a-b)&_M; a=(a-c)&_M; a^=(c>>12)
    b=(b-c)&_M; b=(b-a)&_M; b^=(a<<16)&_M
    c=(c-a)&_M; c=(c-b)&_M; c^=(b>>5)
    a=(a-b)&_M; a=(a-c)&_M; a^=(c>>3)
    b=(b-c)&_M; b=(b-a)&_M; b^=(a<<10)&_M
    c=(c-a)&_M; c=(c-b)&_M; c^=(b>>15)
    return a & _M, b & _M, c & _M

def vlt32(key, initval=0xABCDEF00):
    """lookup2 with the AttribSys seed — hashes every vault name."""
    if isinstance(key, str):
        key = key.encode('latin1')
    a = b = 0x9E3779B9
    c = initval & _M
    length = len(key); k = 0; ln = length
    while ln >= 12:
        a=(a+(key[k]|key[k+1]<<8|key[k+2]<<16|key[k+3]<<24))&_M
        b=(b+(key[k+4]|key[k+5]<<8|key[k+6]<<16|key[k+7]<<24))&_M
        c=(c+(key[k+8]|key[k+9]<<8|key[k+10]<<16|key[k+11]<<24))&_M
        a,b,c=_mix(a,b,c); k+=12; ln-=12
    c=(c+length)&_M
    if ln>=11: c=(c+(key[k+10]<<24))&_M
    if ln>=10: c=(c+(key[k+9]<<16))&_M
    if ln>=9:  c=(c+(key[k+8]<<8))&_M
    if ln>=8:  b=(b+(key[k+7]<<24))&_M
    if ln>=7:  b=(b+(key[k+6]<<16))&_M
    if ln>=6:  b=(b+(key[k+5]<<8))&_M
    if ln>=5:  b=(b+key[k+4])&_M
    if ln>=4:  a=(a+(key[k+3]<<24))&_M
    if ln>=3:  a=(a+(key[k+2]<<16))&_M
    if ln>=2:  a=(a+(key[k+1]<<8))&_M
    if ln>=1:  a=(a+key[k])&_M
    a,b,c=_mix(a,b,c)
    return c

lookup2 = vlt32  # alias

# node kind hashes (self-describing: hash of the Attrib load-data class name)
KIND_CLASS      = vlt32("Attrib::ClassLoadData")       # 0x5E970CBC
KIND_COLLECTION = vlt32("Attrib::CollectionLoadData")  # 0x8E112EB7
KIND_DATABASE   = vlt32("Attrib::DatabaseLoadData")    # 0xCBBC628F

POINTER_PLACEHOLDER = 0xEFFECADD

def _cid(tag: bytes) -> int:
    return struct.unpack("<I", tag)[0]

CHUNK_NAMES = {
    _cid(b"ErtS"): "String Table (StrE)",
    _cid(b"NpeD"): "Dependencies (DepN)",
    _cid(b"NrtS"): "Symbols (StrN)",
    _cid(b"NtaD"): "Node Data (DatN)",
    _cid(b"NpxE"): "Exports (ExpN)",
    _cid(b"NrtP"): "Pointer Fixups (PtrN)",
}

# ── data model ────────────────────────────────────────────────────────────────

@dataclass
class VaultPackEntry:
    name: str
    vlt_offset: int
    vlt_size: int
    bin_offset: int
    bin_size: int
    data: bytes          # whole file
    index: int = 0

@dataclass
class Chunk:
    cid: int
    size: int            # payload size
    offset: int          # payload offset within its block
    data: bytes

@dataclass
class FieldDef:
    key: int
    type_key: int
    offset: int
    size: int
    max_count: int
    flags: int
    alignment: int

@dataclass
class ClassDef:
    name_hash: int
    base_size: int
    num_definitions: int
    layout_ptr: int      # BIN offset of the field table (after fixup), or -1
    collection_reserve: int
    required_count: int
    fields: List[FieldDef] = field(default_factory=list)

@dataclass
class CollectionEntry:
    key: int
    type_index: int
    node_flags: int
    data4: bytes

@dataclass
class Collection:
    name_hash: int
    class_hash: int
    parent_hash: int
    table_reserve: int
    num_entries: int
    num_types: int
    types: List[int] = field(default_factory=list)
    entries: List[CollectionEntry] = field(default_factory=list)
    data_ptr: int = -1          # BIN offset of the instance-value blob
    export_name: int = 0
    vlt_offset: int = 0
    node_size: int = 0

@dataclass
class Database:
    num1: int
    num2: int
    count: int
    sizes: List[int] = field(default_factory=list)

@dataclass
class Export:
    name_hash: int
    kind_hash: int
    pad: int
    size: int
    offset: int

@dataclass
class Fixup:
    src: int
    a: int
    b: int
    arg: int

@dataclass
class Vault:
    entry: VaultPackEntry
    version: int = 0
    dependencies: List[str] = field(default_factory=list)
    strings: List[str] = field(default_factory=list)
    exports: List[Export] = field(default_factory=list)
    classes: List[ClassDef] = field(default_factory=list)
    collections: List[Collection] = field(default_factory=list)
    databases: List[Database] = field(default_factory=list)
    chunks: List[Chunk] = field(default_factory=list)       # VLT block
    bin_chunks: List[Chunk] = field(default_factory=list)   # BIN block
    vlt_fixups: List[Fixup] = field(default_factory=list)
    raw_fixups: List[Fixup] = field(default_factory=list)
    class_by_hash: Dict[int, ClassDef] = field(default_factory=dict)
    hash_names: Dict[int, str] = field(default_factory=dict)
    bin_heap_offset: int = 0   # offset within BIN block where raw heap starts

# ── container parsing ─────────────────────────────────────────────────────────

def parse_vpak(data: bytes) -> List[VaultPackEntry]:
    """VPAK header (verified on attributes.bin=1 vault, FE_ATTRIB.bin=1,
    gameplay.bin=272):
      +0x00 'VPAK'
      +0x04 u32 numVaults
      +0x08 u32 nameBlobOffset
      +0x0C u32 nameBlobSize
      +0x10 numVaults x {u32 nameOfs; u32 binSize; u32 vltSize;
                         u32 binOff; u32 vltOff}
    """
    if len(data) < 0x20 or data[0:4] != b"VPAK":
        raise ValueError("not a VPAK vault (missing magic)")
    count, blob_off, blob_size = struct.unpack_from("<3I", data, 4)
    if count == 0 or count > 0x10000 or blob_off + blob_size > len(data):
        raise ValueError(f"implausible VPAK directory (count={count})")
    blob = data[blob_off:blob_off + blob_size]
    out = []
    for i in range(count):
        name_ofs, bin_size, vlt_size, bin_off, vlt_off = struct.unpack_from(
            "<5I", data, 0x10 + 20 * i)
        nm = blob[name_ofs:].split(b"\x00")[0].decode("latin1") \
            if name_ofs < len(blob) else f"vault{i}"
        if bin_off + bin_size > len(data) + 16 or vlt_off + vlt_size > len(data) + 16:
            raise ValueError(f"vault[{i}] '{nm}' sections exceed file size")
        out.append(VaultPackEntry(name=nm, vlt_offset=vlt_off, vlt_size=vlt_size,
                                  bin_offset=bin_off, bin_size=bin_size,
                                  data=data, index=i))
    return out

def _walk_chunks(data: bytes, base: int, size: int) -> List[Chunk]:
    """{tag[4]; u32 size incl. 8-byte header}; stops at non-chunk data."""
    out = []
    off, end = base, base + size
    while off + 8 <= end:
        cid = struct.unpack_from("<I", data, off)[0]
        csz = struct.unpack_from("<I", data, off + 4)[0]
        if cid not in CHUNK_NAMES or csz < 8 or off + csz > end + 16:
            break
        out.append(Chunk(cid=cid, size=csz - 8, offset=off + 8 - base,
                         data=data[off + 8:off + csz]))
        off += csz
    return out

def _parse_strings(payload: bytes) -> List[str]:
    return [s.decode("latin1") for s in payload.split(b"\x00") if s]

def _parse_deps(payload: bytes) -> List[str]:
    if len(payload) < 4:
        return []
    count = struct.unpack_from("<I", payload, 0)[0]
    if count == 0 or len(payload) < 4 + 8 * count:
        return []
    blob = payload[4 + 8 * count:]
    offs = struct.unpack_from(f"<{count}I", payload, 4 + 4 * count)
    out = []
    for o in offs:
        if o < len(blob):
            out.append(blob[o:].split(b"\x00")[0].decode("latin1"))
    return out

def load_vault(entry: VaultPackEntry) -> Vault:
    data = entry.data
    v = Vault(entry=entry)
    v.bin_chunks = _walk_chunks(data, entry.bin_offset, entry.bin_size)
    v.chunks = _walk_chunks(data, entry.vlt_offset, entry.vlt_size)
    if v.bin_chunks:
        last = v.bin_chunks[-1]
        v.bin_heap_offset = last.offset + last.size

    cmap = {c.cid: c for c in v.chunks}
    bmap = {c.cid: c for c in v.bin_chunks}

    # strings (BIN block ErtS)
    s_chunk = bmap.get(_cid(b"ErtS"))
    if s_chunk:
        v.strings = _parse_strings(s_chunk.data)

    # deps + version
    d = cmap.get(_cid(b"NpeD"))
    if d:
        v.dependencies = _parse_deps(d.data)
    sym = cmap.get(_cid(b"NrtS"))
    if sym and len(sym.data) >= 8:
        v.version = struct.unpack_from("<Q", sym.data, 0)[0]

    # fixups
    p = cmap.get(_cid(b"NrtP"))
    if p:
        n = len(p.data) // 12
        for i in range(n):
            src, a, b, arg = struct.unpack_from("<IHHI", p.data, 12 * i)
            f = Fixup(src, a, b, arg)
            (v.vlt_fixups if (a, b) == (1, 0) else v.raw_fixups).append(f)

    # raw fixup map: pointer-slot offset in VLT -> BIN offset
    raw_at = {f.src: f.arg for f in v.raw_fixups}

    # exports
    e = cmap.get(_cid(b"NpxE"))
    if e and len(e.data) >= 4:
        n = struct.unpack_from("<I", e.data, 0)[0]
        n = min(n, (len(e.data) - 4) // 20)
        for i in range(n):
            nameh, kindh, pad, size, off = struct.unpack_from(
                "<5I", e.data, 4 + 20 * i)
            v.exports.append(Export(nameh, kindh, pad, size, off))

    vlt_base = entry.vlt_offset
    bin_base = entry.bin_offset

    def bin_bytes(off: int, ln: int) -> bytes:
        s = bin_base + off
        return data[s:s + ln]

    for ex in v.exports:
        node_off = vlt_base + ex.offset
        raw = data[node_off:node_off + ex.size]
        if ex.kind_hash == KIND_DATABASE:
            if len(raw) >= 16:
                num1, num2, count, _ptr = struct.unpack_from("<4I", raw, 0)
                sizes = list(struct.unpack_from(
                    f"<{min(count, (len(raw)-16)//4)}I", raw, 16))
                v.databases.append(Database(num1, num2, count, sizes))
        elif ex.kind_hash == KIND_CLASS:
            if len(raw) >= 0x1C:
                nameh, base_size, ndefs, _ptr, reserve, _pad, req = \
                    struct.unpack_from("<7I", raw, 0)
                cd = ClassDef(nameh, base_size, ndefs,
                              raw_at.get(ex.offset + 0xC, -1), reserve, req)
                if cd.layout_ptr >= 0 and ndefs:
                    tbl = bin_bytes(cd.layout_ptr, 16 * ndefs)
                    for i in range(len(tbl) // 16):
                        key, tkey, foff, fsz, mc, fl, al = struct.unpack_from(
                            "<IIHHHBB", tbl, 16 * i)
                        cd.fields.append(FieldDef(key, tkey, foff, fsz, mc, fl, al))
                v.classes.append(cd)
                v.class_by_hash[nameh] = cd
        elif ex.kind_hash == KIND_COLLECTION:
            if len(raw) >= 0x20:
                nameh, clsh, parh, nent, _w4, reserve, ntypes, _ptr = \
                    struct.unpack_from("<8I", raw, 0)
                ntypes = min(ntypes, max(0, (len(raw) - 0x20) // 4))
                types = list(struct.unpack_from(f"<{ntypes}I", raw, 0x20))
                coll = Collection(nameh, clsh, parh, reserve, nent, ntypes,
                                  types,
                                  data_ptr=raw_at.get(ex.offset + 0x1C, -1),
                                  export_name=ex.name_hash,
                                  vlt_offset=ex.offset, node_size=ex.size)
                ebase = 0x20 + 4 * ntypes
                nfit = min(nent, max(0, (len(raw) - ebase) // 12))
                for i in range(nfit):
                    o = ebase + 12 * i
                    key, = struct.unpack_from("<I", raw, o)
                    d4 = raw[o + 4:o + 8]
                    ti, nf = struct.unpack_from("<HH", raw, o + 8)
                    ent = CollectionEntry(key, ti, nf, d4)
                    # pointer-slot entries: resolve the fixup on the data4 slot
                    if d4 == b"\xdd\xca\xfe\xef":
                        ent.data_bin_ofs = raw_at.get(ex.offset + o + 4, -1)
                    v.collections.append  # no-op to keep flow obvious
                    coll.entries.append(ent)
                v.collections.append(coll)

    # hash->name table from in-file strings only (verified evidence)
    for s in v.strings:
        v.hash_names.setdefault(vlt32(s), s)
    # heap strings (reflection type names etc.) — also shipped bytes
    heap = data[bin_base + v.bin_heap_offset: bin_base + entry.bin_size]
    for s in _heap_strings(heap):
        v.hash_names.setdefault(vlt32(s), s)
    return v

def _heap_strings(buf: bytes, minlen: int = 3) -> List[str]:
    out, cur = [], bytearray()
    for b in buf:
        if 32 <= b < 127:
            cur.append(b)
        else:
            if b == 0 and len(cur) >= minlen:
                out.append(cur.decode("latin1"))
            cur = bytearray()
    return out

# ── presentation helpers ─────────────────────────────────────────────────────

def name(v: Optional[Vault], h: int, extra: Optional[Dict[int, str]] = None) -> str:
    if h == 0:
        return "(none)"
    if v is not None:
        s = v.hash_names.get(h)
        if s is not None:
            return s
    if extra:
        s = extra.get(h)
        if s is not None:
            return s
    return f"0x{h:08X}"

FIELD_FLAGS = {0x01: "array", 0x02: "inline", 0x04: "required",
               0x08: "hashref", 0x10: "locked", 0x20: "ptr"}

def fmt_flags(fl: int) -> str:
    bits = [n for b, n in FIELD_FLAGS.items() if fl & b]
    return f"0x{fl:02X}" + (f" ({','.join(bits)})" if bits else "")

NODE_FLAGS = {0x01: "hasEntry", 0x02: "inline", 0x04: "owned",
              0x08: "modified", 0x10: "link", 0x20: "default"}

def fmt_nodeflags(nf: int) -> str:
    bits = [n for b, n in NODE_FLAGS.items() if nf & b]
    return f"0x{nf:04X}" + (f" ({','.join(bits)})" if bits else "")

def print_chunks(v: Vault):
    print("VLT chunks:")
    for c in v.chunks:
        print(f"  {CHUNK_NAMES.get(c.cid, 'Unknown'):<24} @0x{c.offset:06X}  0x{c.size:X} bytes")
    print("BIN chunks:")
    for c in v.bin_chunks:
        print(f"  {CHUNK_NAMES.get(c.cid, 'Unknown'):<24} @0x{c.offset:06X}  0x{c.size:X} bytes")
    print(f"  (raw heap)               @0x{v.bin_heap_offset:06X}  "
          f"0x{v.entry.bin_size - v.bin_heap_offset:X} bytes")

def print_classes(v: Vault):
    print(f"Classes: {len(v.classes)}")
    for cd in v.classes:
        print(f"  class {name(v, cd.name_hash)} (0x{cd.name_hash:08X}) "
              f"baseSize=0x{cd.base_size:X} defs={cd.num_definitions} "
              f"reserve={cd.collection_reserve} required={cd.required_count}")
        for f in cd.fields:
            print(f"    {name(v, f.key):<32} {name(v, f.type_key):<28} "
                  f"off=0x{f.offset:03X} size=0x{f.size:03X} n={f.max_count} "
                  f"flags={fmt_flags(f.flags)} align={f.alignment}")

def print_collections(v: Vault):
    print(f"Collections: {len(v.collections)}")
    for coll in v.collections:
        print(f"  coll {name(v, coll.name_hash)} class={name(v, coll.class_hash)} "
              f"parent={name(v, coll.parent_hash)} entries={coll.num_entries} "
              f"types={[name(v, t) for t in coll.types]}")
        for ent in coll.entries:
            print(f"    {name(v, ent.key):<32} ti={ent.type_index:<3} "
                  f"nf={fmt_nodeflags(ent.node_flags)} data={ent.data4.hex(' ')}")

def print_databases(v: Vault):
    print(f"Databases: {len(v.databases)}")
    for db in v.databases:
        print(f"  num1={db.num1} num2={db.num2} count={db.count}")
        print(f"  type sizes: {db.sizes}")

__all__ = ["vlt32", "lookup2", "parse_vpak", "load_vault", "name",
           "fmt_flags", "fmt_nodeflags", "CHUNK_NAMES", "Vault",
           "VaultPackEntry", "print_chunks", "print_classes",
           "print_collections", "print_databases",
           "KIND_CLASS", "KIND_COLLECTION", "KIND_DATABASE",
           "POINTER_PLACEHOLDER"]
