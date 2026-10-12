#!/usr/bin/env python3
"""Compares vf2run's frames and dumps with MAME's (tools/mame_snap.sh), frame by frame.
   tools/compare.py <dir> <frame> [frame...]   (dir holds fNNNNN* from vf2run and mame_fNNNNN* from MAME)"""
import sys, os
from PIL import Image

def diff_bytes(a, b, unit=4):
    n = min(len(a), len(b))
    bad = [i for i in range(0, n, unit) if a[i:i+unit] != b[i:i+unit]]
    return bad

d = sys.argv[1]
for f in map(int, sys.argv[2:]):
    ours, mame = f"{d}/f{f:05d}", f"{d}/mame_f{f:05d}"
    line = [f"frame {f}:"]
    if os.path.exists(ours + ".png") and os.path.exists(mame + ".png"):
        a = Image.open(ours + ".png").convert("RGB"); b = Image.open(mame + ".png").convert("RGB")
        if a.size != b.size: line.append(f"picture size {a.size} vs {b.size}")
        else:
            pa, pb = a.tobytes(), b.tobytes()
            bad = sum(1 for i in range(0, len(pa), 3) if pa[i:i+3] != pb[i:i+3])
            line.append(f"picture {'identical' if bad == 0 else f'{bad} pixels differ'}")
    for what, base in (("workram", 0x500000), ("tileram", 0x1000000), ("palram", 0x1800000), ("colorxlat", 0x1810000), ("backup", 0x1d00000), ("eeprom", 0), ("bufferram", 0x900000)):
        pa, pb = ours + f"_{what}.bin", mame + f"_{what}.bin"
        if os.path.exists(pa) and os.path.exists(pb):
            a, b = open(pa, "rb").read(), open(pb, "rb").read()
            bad = diff_bytes(a, b)
            if not bad: line.append(f"{what} identical")
            else: line.append(f"{what} {len(bad)} words differ (first {base + bad[0]:08x})")
    if os.path.exists(ours + "_regs.txt") and os.path.exists(mame + "_regs.txt"):
        ra = open(ours + "_regs.txt").read().split(); rb = open(mame + "_regs.txt").read().split()
        dif = [f"{ra[i]} {ra[i+1]}/{rb[i+1]}" for i in range(0, min(len(ra), len(rb)), 2) if ra[i+1] != rb[i+1]]
        line.append("regs identical" if not dif else "regs differ: " + ", ".join(dif[:6]))
    print("  ".join(line))
