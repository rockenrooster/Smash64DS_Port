#!/usr/bin/env python3
"""Which object files own the EXECUTED text, and how much of each is dead?

Footprint reduction needs a target, and a function list is not one -- you cannot
apply `-Os`, a split, or a deletion to a function-shaped hole. You apply it to a
translation unit. This maps the linker map's `.main` placements back to object
files and joins them to what the profile says actually ran.

Per object:

  text B        bytes it contributes to .main
  exec B        bytes in functions that executed at least one instruction
  exec%         how much of what it contributes is even reachable in a match
  fetched B     bytes inside 32-byte lines the CPU actually paid to fetch

An object with large `text B` and small `exec%` is carrying cold code through the
link into the same address space the hot code competes for; the fix is
`--gc-sections` reach, a split, or `-Os`, and it costs no behaviour. An object
with high `exec%` and high `fetched B` is genuinely hot and only shrinks by
writing less code.

UNITS: bytes. Cycle figures come from the density census, not here.
"""

from __future__ import annotations

import argparse
import collections
import csv
import re

FUNC = re.compile(r"^([0-9a-f]+) <(.+?)>:$")
INSN = re.compile(r"^\s*([0-9a-f]+):\s+((?:[0-9a-f]{2,8} )+)\s*(\S+)\s*(.*)$")
# ld map: " .text.foo      0x02005338       0x24 build/bar.o"
MAP_ONE = re.compile(r"^\s*\.(?:text|rodata)\S*\s+0x([0-9a-f]+)\s+0x([0-9a-f]+)\s+(\S+)$")
MAP_SPLIT = re.compile(r"^\s*\.(?:text|rodata)\S*$")
MAP_CONT = re.compile(r"^\s+0x([0-9a-f]+)\s+0x([0-9a-f]+)\s+(\S+)$")

LINE = 32
ITCM_LO, ITCM_HI = 0x01FF8000, 0x02000000


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("profile")
    ap.add_argument("--dis", required=True)
    ap.add_argument("--map", required=True)
    ap.add_argument("--top", type=int, default=26)
    args = ap.parse_args()

    # address ranges -> object
    chunks: list[tuple[int, int, str]] = []
    pending = False
    with open(args.map, errors="ignore") as handle:
        for raw in handle:
            line = raw.rstrip("\n")
            m = MAP_ONE.match(line)
            if m:
                pending = False
            elif pending:
                m = MAP_CONT.match(line)
                pending = False
            elif MAP_SPLIT.match(line):
                pending = True
                continue
            else:
                continue
            if not m:
                continue
            addr, size, obj = int(m.group(1), 16), int(m.group(2), 16), m.group(3)
            if size and not (ITCM_LO <= addr < ITCM_HI):
                chunks.append((addr, addr + size, obj))
    chunks.sort()
    starts = [c[0] for c in chunks]

    import bisect

    def obj_of(addr: int):
        i = bisect.bisect_right(starts, addr) - 1
        if i < 0:
            return None
        lo, hi, obj = chunks[i]
        return obj if lo <= addr < hi else None

    owner: dict[int, str] = {}
    width: dict[int, int] = {}
    fn_start: dict[str, int] = {}
    fn_bytes: collections.Counter = collections.Counter()
    cur = None
    with open(args.dis, errors="ignore") as handle:
        for raw in handle:
            m = FUNC.match(raw.rstrip("\n"))
            if m:
                cur = m.group(2)
                fn_start.setdefault(cur, int(m.group(1), 16))
                continue
            if cur is None:
                continue
            mi = INSN.match(raw)
            if not mi:
                continue
            pc = int(mi.group(1), 16)
            owner[pc] = cur
            width[pc] = len(mi.group(2).replace(" ", "")) // 2
            fn_bytes[cur] += width[pc]

    executed_fns: set[str] = set()
    fetched: dict[str, set] = collections.defaultdict(set)
    with open(args.profile, newline="") as handle:
        rows = csv.reader(handle)
        header = next(rows)
        i_pc = {n: i for i, n in enumerate(header)}["pc"]
        for row in rows:
            try:
                pc = int(row[i_pc], 16)
            except ValueError:
                continue
            if ITCM_LO <= pc < ITCM_HI or pc not in owner:
                continue
            executed_fns.add(owner[pc])
            o = obj_of(pc)
            if o:
                fetched[o].add(pc & ~(LINE - 1))

    text_b: collections.Counter = collections.Counter()
    exec_b: collections.Counter = collections.Counter()
    for fn, nb in fn_bytes.items():
        a = fn_start.get(fn)
        if a is None or ITCM_LO <= a < ITCM_HI:
            continue
        o = obj_of(a)
        if not o:
            continue
        text_b[o] += nb
        if fn in executed_fns:
            exec_b[o] += nb

    tot_t = sum(text_b.values())
    tot_e = sum(exec_b.values())
    tot_f = sum(len(v) for v in fetched.values()) * LINE
    print(f"objects placed in main text: {len(text_b):,}")
    print(f"  total text        {tot_t:>10,} B")
    print(f"  in executed fns   {tot_e:>10,} B ({100 * tot_e / tot_t:.1f}%)")
    print(f"  in fetched lines  {tot_f:>10,} B "
          f"({100 * tot_f / tot_t:.1f}% of text)\n")
    print(f"{'text B':>10}{'exec B':>10}{'exec%':>7}{'fetched B':>11}  object")
    print("-" * 96)
    for obj, n in sorted(fetched.items(), key=lambda kv: -len(kv[1]))[:args.top]:
        t = text_b.get(obj, 0)
        e = exec_b.get(obj, 0)
        print(f"{t:>10,}{e:>10,}{(100 * e / t if t else 0):>7.1f}"
              f"{len(n) * LINE:>11,}  {obj[-58:]}")

    print("\n== largest COLD contributors (text placed, little or none executed) ==")
    print(f"{'text B':>10}{'exec%':>7}  object")
    cold = [(t, obj) for obj, t in text_b.items()
            if t > 4096 and (exec_b.get(obj, 0) / t) < 0.25]
    for t, obj in sorted(cold, reverse=True)[:12]:
        print(f"{t:>10,}{100 * exec_b.get(obj, 0) / t:>7.1f}  {obj[-62:]}")


if __name__ == "__main__":
    main()
