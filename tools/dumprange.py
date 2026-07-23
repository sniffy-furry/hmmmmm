#!/usr/bin/env python3
"""Hex-dump a byte range of a file. Usage: dumprange.py <file> <start> <len>"""
import sys, struct

path = sys.argv[1]
start = int(sys.argv[2])
length = int(sys.argv[3])
with open(path, "rb") as f:
    f.seek(start)
    data = f.read(length)
for i in range(0, len(data), 16):
    row = data[i:i+16]
    hexs = " ".join(f"{b:02X}" for b in row)
    floats = ""
    if len(row) >= 4:
        fs = []
        for j in range(0, len(row) - 3, 4):
            fs.append("%.3f" % struct.unpack_from("<f", row, j)[0])
        floats = "  " + " ".join(fs)
    print(f"{start+i:8d}: {hexs:<48}{floats}")
