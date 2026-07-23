#!/usr/bin/env python3
"""Walk an EAGL chunk tree (NFSMW .BUN/.BIN) and print the chunk-ID inventory.

Chunk header: u32 id (LE), u32 size (LE), then `size` payload bytes.
Container chunks have bit 31 set (0x80000000) and hold nested chunks.
Leaf chunks may carry 0x11 alignment padding at the start of their payload.
"""
import sys
import struct
from collections import Counter

counts = Counter()


def walk(buf, off, end, depth, max_depth=12):
    while off + 8 <= end:
        cid, size = struct.unpack_from("<II", buf, off)
        if cid == 0 and size == 0:
            off += 8
            continue
        payload = off + 8
        payload_end = payload + size
        if payload_end > end or size < 0:
            # bogus / not a chunk boundary — stop this level
            break
        counts[cid] += 1
        indent = "  " * depth
        is_container = (cid & 0x80000000) != 0
        print(f"{indent}0x{cid:08X}  size={size}  @{off}")
        if is_container and depth < max_depth:
            walk(buf, payload, payload_end, depth + 1, max_depth)
        off = payload_end


def main():
    path = sys.argv[1]
    with open(path, "rb") as f:
        buf = f.read()
    print(f"== {path}  ({len(buf)} bytes) ==")
    walk(buf, 0, len(buf), 0)
    print("\n== chunk-id totals ==")
    for cid, n in sorted(counts.items()):
        flag = " (container)" if cid & 0x80000000 else ""
        print(f"0x{cid:08X}: {n}{flag}")


if __name__ == "__main__":
    main()
