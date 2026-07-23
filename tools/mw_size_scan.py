#!/usr/bin/env python3
"""mw_size_scan.py - object-size recovery scanner for retail v1.3 speed.exe.

Recovers per-class object sizes by scanning each registered class's factory
init_fn (and, one level deep, the create-functions it calls) for allocation
sites: `push <size>` as the last push before a `call` to a known allocator.

This is the FIXED (2026-07-03) version of the scanner used for Discovery 10:
  * does NOT stop at the first `ret` (early-out paths hid later alloc sites -
    that bug is what originally hid e.g. the CDAction init_fn interiors);
  * follows direct `call` targets ONE level deep (the init_fns are often
    factory *wrappers* whose `operator new` lives in a create-fn they call,
    e.g. World_UpdateBody's init_fn -> 0x784ec0 -> push 0x70);
  * knows the engine's pool allocator `0x5d29d0` (`push <size>; mov ecx,<pool>;
    call 0x5d29d0`) in addition to plain `operator new`;
  * allocator set is CALIBRATED, not assumed: pass 1 finds, for every class
    with an already-verified size, which call target follows `push <known
    size>`; only call targets that validate this way are trusted in pass 2;
  * refuses to guess: init_fns with multiple alloc sites (the director-action
    registration loops allocate shared managers/helpers, not the class) are
    reported as ambiguous, never auto-assigned.

Verified result on retail v1.3 (md5 c0516b485065fabdd69579816b5df763):
reproduces every known size and recovers SoundAI = 600 (init_fn 0x7207c0 ->
operator new(0x258) -> ctor 0x7200c0). The remaining 10 unknowns are
confirmed NOT statically recoverable (5 named systems constructed in place;
5 director actions instantiated through the reflection factory - ctor
0x486000 for CDActionIce has no direct callers, the size lives in a
runtime-populated descriptor).

Usage:
  python3 mw_size_scan.py <speed.exe> <class_evidence.json>
"""
import json
import struct
import sys
from collections import defaultdict

try:
    import capstone
except ImportError:
    sys.exit("needs capstone: pip install capstone")

IMAGE_BASE = 0x400000
INIT_FN_SCAN = 0x800   # bytes of the init_fn body to decode (no early ret stop)
CALLEE_SCAN = 0x400    # bytes of each one-level callee to decode
MAX_SANE_SIZE = 0x4000  # pushes above this are not object sizes


def load_pe(path):
    data = open(path, "rb").read()
    pe_off = struct.unpack_from("<I", data, 0x3C)[0]
    nsec = struct.unpack_from("<H", data, pe_off + 6)[0]
    opt_size = struct.unpack_from("<H", data, pe_off + 20)[0]
    sec_off = pe_off + 24 + opt_size
    secs = []
    for i in range(nsec):
        o = sec_off + i * 40
        name = data[o:o + 8].rstrip(b"\0").decode()
        vsize, va, rsize, roff = struct.unpack_from("<IIII", data, o + 8)
        secs.append((name, IMAGE_BASE + va, max(vsize, rsize), roff))
    return data, secs


def make_mapper(data, secs):
    def va2off(va):
        for _, sva, size, roff in secs:
            if sva <= va < sva + size:
                return roff + (va - sva)
        return None
    text = next(s for s in secs if s[0] == ".text")
    text_lo, text_hi = text[1], text[1] + text[2]
    return va2off, (text_lo, text_hi)


def decode(md, data, va2off, va, length):
    off = va2off(va)
    if off is None:
        return []
    return list(md.disasm(data[off:off + length], va))


def alloc_sites(insns, allocators=None):
    """Yield (call_va, call_target, pushed_imm) where the most recent push
    before a direct call is `push imm`. If `allocators` given, filter to it."""
    pending = None  # (imm, insn_va)
    out = []
    for ins in insns:
        if ins.mnemonic == "push":
            op = ins.operands[0]
            pending = (op.imm, ins.address) if op.type == capstone.x86.X86_OP_IMM else None
        elif ins.mnemonic == "call":
            op = ins.operands[0]
            if op.type == capstone.x86.X86_OP_IMM and pending is not None:
                imm = pending[0]
                if 0 < imm <= MAX_SANE_SIZE and (allocators is None or op.imm in allocators):
                    out.append((ins.address, op.imm, imm))
            pending = None
    return out


def call_targets(insns, text_range):
    lo, hi = text_range
    seen, out = set(), []
    for ins in insns:
        if ins.mnemonic == "call":
            op = ins.operands[0]
            if op.type == capstone.x86.X86_OP_IMM and lo <= op.imm < hi and op.imm not in seen:
                seen.add(op.imm)
                out.append(op.imm)
    return out


def scan_class(md, data, va2off, text_range, init_va):
    """Return list of (where, call_target, size) alloc-site candidates for one
    init_fn: its own body plus each direct callee, one level deep."""
    body = decode(md, data, va2off, init_va, INIT_FN_SCAN)
    sites = [("init_fn", va, tgt, sz) for va, tgt, sz in alloc_sites(body)]
    for callee in call_targets(body, text_range):
        cbody = decode(md, data, va2off, callee, CALLEE_SCAN)
        sites += [(f"callee 0x{callee:x}", va, tgt, sz)
                  for va, tgt, sz in alloc_sites(cbody)]
    return sites


def main():
    exe = sys.argv[1] if len(sys.argv) > 1 else "reference/mw/speed.exe"
    evid = sys.argv[2] if len(sys.argv) > 2 else \
        "reference/MWEncyclopedia/RE-Data-And-Discoveries/data/class_evidence.json"
    data, secs = load_pe(exe)
    va2off, text_range = make_mapper(data, secs)
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    md.detail = True
    classes = json.load(open(evid))

    # ---- pass 1: calibrate the allocator set on classes with known sizes ----
    allocators = defaultdict(int)
    per_class = {}
    for c in classes:
        if not c.get("init_fn_va"):
            continue
        init_va = int(c["init_fn_va"], 16)
        per_class[c["name"]] = sites = scan_class(md, data, va2off, text_range, init_va)
        known = c.get("object_size")
        if known:
            for _, _, tgt, sz in sites:
                if sz == known:
                    allocators[tgt] += 1
    # A real allocator has a non-trivial body. Reject getter/leaf STUBS that
    # merely `mov eax, imm32; ret` (or `ret N`) and ignore their pushed argument:
    # e.g. 0x6269B0 returns a fixed global 0x9205E0 regardless of the `push 80`
    # a caller emits, so a size attributed to it is not an operator-new size.
    # Trusting it silently promoted PVehicle=80 on false evidence.
    def is_stub_allocator(va):
        insns = decode(md, data, va2off, va, 16)
        for ins in insns:
            if ins.mnemonic == "mov" and ins.operands and \
               ins.operands[0].type == capstone.x86.X86_OP_REG and \
               ins.reg_name(ins.operands[0].reg) == "eax" and \
               len(ins.operands) == 2 and ins.operands[1].type == capstone.x86.X86_OP_IMM:
                nxt = insns[insns.index(ins) + 1] if insns.index(ins) + 1 < len(insns) else None
                if nxt and nxt.mnemonic.startswith("ret"):
                    return True   # `mov eax, imm32; ret` — a constant getter, not an allocator
            break  # only inspect the first instruction's shape
        return False

    trusted = {t for t, n in allocators.items() if n >= 2 and not is_stub_allocator(t)}
    rejected = {t for t, n in allocators.items() if n >= 2 and is_stub_allocator(t)}
    for t in sorted(rejected):
        print(f"REJECTED stub 'allocator' 0x{t:x} ({allocators[t]} hits) — "
              f"`mov eax,imm; ret` getter, ignores its arg; sizes via it are unverified")
    print("calibrated allocators (>=2 known-size hits, stubs excluded):")
    for t in sorted(trusted):
        print(f"  0x{t:x}  ({allocators[t]} hits)")

    # ---- pass 2: verify knowns + recover unknowns using trusted allocators ----
    ok = miss = 0
    print("\n-- known sizes, reproduced? --")
    for c in classes:
        known = c.get("object_size")
        if not known or c["name"] not in per_class:
            continue
        sizes = {sz for _, _, tgt, sz in per_class[c["name"]] if tgt in trusted}
        if known in sizes:
            ok += 1
        else:
            miss += 1
            print(f"  MISS {c['name']}: known {known}, scan saw {sorted(sizes)}")
    print(f"  reproduced {ok}, missed {miss}")

    print("\n-- unknowns --")
    for c in classes:
        if c.get("object_size") or c["name"] not in per_class:
            continue
        hits = [(w, sz) for w, _, tgt, sz in per_class[c["name"]] if tgt in trusted]
        uniq = sorted({sz for _, sz in hits})
        if len(uniq) == 1:
            print(f"  RECOVERED {c['name']} = {uniq[0]} bytes ({hits[0][0]})")
        elif uniq:
            print(f"  AMBIGUOUS {c['name']}: allocs {uniq} - multi-object "
                  f"init_fn (managers/helpers), NOT auto-assigned")
        else:
            print(f"  NO-ALLOC  {c['name']}: no trusted alloc site in init_fn "
                  f"or one-level callees (runtime/reflection construction)")
    no_init = [c["name"] for c in classes if not c.get("init_fn_va")]
    if no_init:
        print(f"\n  no init_fn (named singletons, built in place): {', '.join(sorted(set(no_init)))}")


if __name__ == "__main__":
    main()
