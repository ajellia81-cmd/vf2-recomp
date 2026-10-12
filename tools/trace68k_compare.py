#!/usr/bin/env python3
"""Compares the 68000's instruction traces: vf2run --68k-trace ("cycles pc") with MAME's
(tools/mame_trace68k.sh: "cycles pc: disassembly").
   tools/trace68k_compare.py <ours.txt> <mame.txt> [max reports]
Reports the first place the PCs part, and the instructions whose length (cycles to the next
one) differs, by opcode, with how often."""
import sys, re
from collections import Counter, defaultdict


def load(path, mame):
    cyc, pcs, dis = [], [], []
    for line in open(path):
        if mame:
            m = re.match(r'\s*(?:(\d+) )?([0-9A-Fa-f]+): (.*)', line)
            if not m:
                continue
            cyc.append(int(m.group(1)) if m.group(1) else None)
            pcs.append(int(m.group(2), 16))
            dis.append(m.group(3).strip())
        else:
            p = line.split()
            if len(p) < 2:
                continue
            cyc.append(int(p[0])); pcs.append(int(p[1], 16)); dis.append('')
    return cyc, pcs, dis


oc, op, _ = load(sys.argv[1], False)
mc, mp, md = load(sys.argv[2], True)
limit = int(sys.argv[3]) if len(sys.argv) > 3 else 20
n = min(len(op), len(mp))
part = next((i for i in range(n) if op[i] != mp[i]), None)
print(f"ours {len(op)} instructions, MAME's {len(mp)}; PCs {'the same throughout' if part is None else f'part at #{part}'}")
end = part if part is not None else n
diffs = Counter()
examples = {}
shift = defaultdict(int)
for i in range(1, end - 1):
    if mc[i] is None or mc[i + 1] is None:
        continue
    lo, lm = oc[i + 1] - oc[i], mc[i + 1] - mc[i]
    if lo != lm:
        key = md[i].split()[0] if md[i] else '?'
        k = (key, lo, lm)
        diffs[k] += 1
        if k not in examples:
            examples[k] = (i, mp[i], md[i])
for (key, lo, lm), cnt in diffs.most_common(limit):
    i, pc, d = examples[(key, lo, lm)]
    print(f"{cnt:7d}x  {key:10s} ours {lo:3d} MAME {lm:3d}   first #{i} {pc:06x}: {d}")
if part is not None:
    print("where they part:")
    for i in range(max(0, part - 6), min(n, part + 3)):
        print(f"  #{i}: ours {oc[i]} {op[i]:06x}   MAME {mc[i]} {mp[i]:06x}: {md[i]}")
first = next((i for i in range(1, end) if mc[i] is not None and oc[i] - mc[i] != oc[1] - mc[1]), None)
print(f"offset ours-MAME at #1: {oc[1] - mc[1]}; first change of offset at #{first}" if first else f"offset ours-MAME constant {oc[1] - mc[1]}")
