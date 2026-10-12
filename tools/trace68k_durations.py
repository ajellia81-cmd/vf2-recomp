#!/usr/bin/env python3
"""The 68000's instruction lengths, per PC, in vf2run's trace and MAME's, without needing the two
runs to stay in step: for each PC, the most common number of cycles to the next instruction in
each trace, and the PCs where those differ (with MAME's disassembly).
   tools/trace68k_durations.py <ours.txt> <mame.txt>"""
import sys, re
from collections import Counter, defaultdict


def durations(path, mame):
    d = defaultdict(Counter)
    dis = {}
    prev = None
    for line in open(path):
        if mame:
            m = re.match(r'\s*(?:(\d+) )?([0-9A-Fa-f]+): (.*)', line)
            if not m or not m.group(1):
                prev = None
                continue
            c, pc = int(m.group(1)), int(m.group(2), 16)
            dis.setdefault(pc, m.group(3).strip())
        else:
            p = line.split()
            c, pc = int(p[0]), int(p[1], 16)
        if prev:
            d[prev[1]][c - prev[0]] += 1
        prev = (c, pc)
    return d, dis


ours, _ = durations(sys.argv[1], False)
mame, dis = durations(sys.argv[2], True)
rows = []
for pc in sorted(set(ours) & set(mame)):
    o, m = ours[pc].most_common(1)[0][0], mame[pc].most_common(1)[0][0]
    if o != m:
        rows.append((pc, o, m, sum(mame[pc].values())))
print(f"{len(set(ours) & set(mame))} PCs in both; {len(rows)} whose usual length differs")
for pc, o, m, n in rows:
    print(f"{pc:06x}  ours {o:3d}  MAME {m:3d}  ({n}x)  {dis.get(pc, '')}   ours {dict(ours[pc].most_common(3))} MAME {dict(mame[pc].most_common(3))}")
