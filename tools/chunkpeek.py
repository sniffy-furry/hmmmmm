#!/usr/bin/env python3
"""Find the first instance of a container chunk ID and hex-preview its children.

Usage: chunkpeek.py <file> <hexid>   e.g. chunkpeek.py STREAML2RA.BUN 80034100
"""
import sys
import struct


def find_first(buf, off, end, target, depth=0):
    while off + 8 <= end:
        cid, size = struct.unpack_from("<II", buf, off)
        if cid == 0 and size == 0:
            off += 8
            continue
        payload = off + 8
        payload_end = payload + size
        if payload_end > end:
            break
        if cid == target:
            return off, size
        if cid & 0x80000000:
            r = find_first(buf, payload, payload_end, target, depth + 1)
            if r:
                return r
        off = payload_end
    return None


def preview(buf, off, end, depth, target, maxbytes=96):
    while off + 8 <= end:
        cid, size = struct.unpack_from("<II", buf, off)
        if cid == 0 and size == 0:
            off += 8
            continue
        payload = off + 8
        payload_end = payload + size
        if payload_end > end:
            break
        ind = "  " * depth
        data = buf[payload:payload + min(size, maxbytes)]
        hexs = " ".join(f"{b:02X}" for b in data)
        # interpret first floats / u32s
        floats = []
        u32s = []
        for i in range(0, min(size, 32), 4):
            if payload + i + 4 <= end:
                u32s.append(struct.unpack_from("<I", buf, payload + i)[0])
                floats.append(struct.unpack_from("<f", buf, payload + i)[0])
        print(f"{ind}0x{cid:08X} size={size}")
        print(f"{ind}  hex: {hexs}")
        print(f"{ind}  u32: {[hex(x) for x in u32s]}")
        print(f"{ind}  f32: {['%.3f' % x for x in floats]}")
        if cid & 0x80000000:
            preview(buf, payload, payload_end, depth + 1, target, maxbytes)
        off = payload_end


def main():
    path = sys.argv[1]
    target = int(sys.argv[2], 16)
    with open(path, "rb") as f:
        buf = f.read()
    r = find_first(buf, 0, len(buf), target)
    if not r:
        print(f"0x{target:08X} not found")
        return
    off, size = r
    print(f"first 0x{target:08X} @ {off}, size {size}\n")
    preview(buf, off + 8, off + 8 + size, 0, target)


if __name__ == "__main__":
    main()
