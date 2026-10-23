"""RAM ledger of a build (P8 R0): from its link map, the bytes each output
section, object and family holds in main RAM and the TCMs, and the image,
battle-resident and menu-resident totals; with --diff, the change from an
older build.

usage: python ram_ledger.py MAP [--diff OLD_MAP] [--top N] [--section NAME]

MAP is a build's link map (builds/<name>/.map). Totals:
  image            the static main-RAM sections (.crt0, .text.hot*, .main,
                   .main.rw, .main.bss and the small init tables)
  VS battle        the image: a VS battle lends the whole front-end overlay
  1P battle        the image plus the overlay's 1P head (the part before
                   __nds_frontend_1p_end), which a 1P battle keeps
  menus            the image plus the whole front-end overlay
The TCMs are listed apart: they are not main RAM.

Families group input sections by the symbol each one holds (the build uses
-ffunction-sections and -fdata-sections), by name: see FAMILIES.
--section NAME lists one output section's largest symbols.
"""
import re
import sys
from collections import defaultdict
from pathlib import Path

MAIN_IMAGE = (".crt0", ".text.hot", ".text.hot.draw", ".main", ".main.rw",
              ".main.bss", ".init_array", ".fini_array", ".tinfo", ".eh_frame")
TCM = (".itcm", ".dtcm", ".dtcm.bss", ".vectors")
OVERLAY = ".ovl.frontend"

# (family, regex on the symbol or input-section name), first match wins.
FAMILIES = [
    ("diagnostics", r"(?i)diag|census|probe|witness|proof|freeze|tickhud|tick_hud|gNdsLab|Lab[A-Z]"),
    ("audio", r"(?i)audio|fgm|bgm|syAudio|alSyn|n_al|sound"),
    ("fighter", r"(?i)fighter|^ft|\bft[A-Z]|Ftr|FtPose|ftanim|figatree|battlepack|kirby|mario|fox|samus|link|yoshi|pikachu|purin|ness|captain|donkey|luigi|boss|lean|P4[A-Z]|nds_p4|_p4_"),
    ("stage", r"(?i)stage|^gr|\bgr[A-Z]|wallpaper|mpCollision|^mp[A-Z]|yakumono|whispy|inishie|jungle|sector|yamabuki|castle|zebes|hyrule|pupupu|yoster"),
    ("item", r"(?i)^it[A-Z]|\bit[A-Z]|item"),
    ("entry/effect", r"(?i)^ef[A-Z]|\bef[A-Z]|effect|entry|particle|lbParticle|halo|wave"),
    ("menu/2d", r"(?i)^mn[A-Z]|\bmn[A-Z]|menu|css|ui_?kit|UiKit|title|results|sobj|sprite|ifCommon|ifcommon|^mv[A-Z]|opening|congra|staffroll|sc1p|1p"),
    ("renderer", r"(?i)renderer|native|gx|texture|matrix|^gc[A-Z]|draw"),
    ("net", r"(?i)net|lobby|lockstep|radio"),
    ("system", r"(?i)^sy[A-Z]|taskman|objman|malloc|coroutine|thread|scene|reloc|nitro|fat|fs[A-Z_]|^os[A-Z]|libc|_r$|printf|str|mem"),
]
FAMILY_RX = [(f, re.compile(rx)) for f, rx in FAMILIES]


def family(name):
    sym = re.sub(r"^\.(text|rodata|data|bss|sbss|sdata|itcm|dtcm)(\.\w+)*?\.", "", name)
    for f, rx in FAMILY_RX:
        if rx.search(sym):
            return f
    return "common"


def parse(path):
    """Output sections {name: (addr, size)} and input sections
    [(output, name, size, object)] from a GNU ld map, plus symbols seen."""
    out_secs = {}
    inputs = []
    symbols = {}
    current = None
    pending = None
    for line in Path(path).read_text(errors="replace").splitlines():
        m = re.match(r"^(\.[\w.]+)\s+0x([0-9a-f]+)\s+0x([0-9a-f]+)", line)
        if m:
            current = m.group(1)
            out_secs[current] = (int(m.group(2), 16), int(m.group(3), 16))
            pending = None
            continue
        m = re.match(r"^(\.[\w.]+)\s*$", line)
        if m:
            current = m.group(1)
            pending = None
            continue
        if current is None:
            continue
        m = re.match(r"^ (\.[\w.$]+)\s+0x([0-9a-f]+)\s+0x([0-9a-f]+)\s+(\S+)", line)
        if m:
            inputs.append((current, m.group(1), int(m.group(3), 16), Path(m.group(4)).name))
            pending = None
            continue
        m = re.match(r"^ (\.[\w.$]+)\s*$", line)
        if m:
            pending = m.group(1)
            continue
        m = re.match(r"^\s+0x([0-9a-f]+)\s+0x([0-9a-f]+)\s+(\S+)", line)
        if m and pending:
            inputs.append((current, pending, int(m.group(2), 16), Path(m.group(3)).name))
            pending = None
            continue
        m = re.match(r"^\s+0x([0-9a-f]+)\s+([A-Za-z_]\w*)\s*(=.*)?$", line)
        if m:
            symbols[m.group(2)] = int(m.group(1), 16)
    return out_secs, inputs, symbols


def totals(out_secs, symbols):
    image = sum(out_secs.get(s, (0, 0))[1] for s in MAIN_IMAGE)
    ovl_addr, ovl_size = out_secs.get(OVERLAY, (0, 0))
    head = symbols.get("__nds_frontend_1p_end")
    head_bytes = (head - ovl_addr) if head and ovl_addr <= head <= ovl_addr + ovl_size else None
    t = {"image": image, "VS battle": image, "menus": image + ovl_size}
    t["1P battle"] = image + head_bytes if head_bytes is not None else None
    for s in TCM:
        if s in out_secs:
            t[s] = out_secs[s][1]
    t[OVERLAY] = ovl_size
    return t


def fmt(n):
    return "-" if n is None else f"{n:,}"


def main():
    args = sys.argv[1:]
    top = 25
    if "--top" in args:
        k = args.index("--top")
        top = int(args[k + 1])
        del args[k:k + 2]
    section = None
    if "--section" in args:
        k = args.index("--section")
        section = args[k + 1]
        del args[k:k + 2]
    old = None
    if "--diff" in args:
        k = args.index("--diff")
        old = args[k + 1]
        del args[k:k + 2]
    out_secs, inputs, symbols = parse(args[0])
    t = totals(out_secs, symbols)
    print(f"RAM ledger: {args[0]}")
    if old:
        o_secs, o_inputs, o_symbols = parse(old)
        ot = totals(o_secs, o_symbols)
        print(f"  against {old}")
        for k in t:
            a, b = ot.get(k), t[k]
            d = (b - a) if (a is not None and b is not None) else None
            print(f"  {k:16s} {fmt(a):>12s} -> {fmt(b):>12s}  {'' if d is None else f'{d:+,}'}")
        def keyed(rows):
            agg = defaultdict(int)
            for o, n, s, obj in rows:
                if o in MAIN_IMAGE or o == OVERLAY or o in TCM:
                    agg[(o, n)] += s
            return agg
        a, b = keyed(o_inputs), keyed(inputs)
        changes = sorted(((b.get(k, 0) - a.get(k, 0), k) for k in set(a) | set(b)
                          if b.get(k, 0) != a.get(k, 0)), key=lambda x: -abs(x[0]))
        print(f"\nlargest changes ({len(changes)} input sections changed):")
        for d, (o, n) in changes[:top]:
            print(f"  {d:+9,d}  {o:14s} {n}")
        return
    for k, v in t.items():
        print(f"  {k:16s} {fmt(v):>12s}")
    print("\noutput sections in main RAM and the TCMs:")
    for s in MAIN_IMAGE + (OVERLAY,) + TCM:
        if s in out_secs:
            print(f"  {s:16s} {out_secs[s][1]:>12,}")
    by_obj = defaultdict(int)
    by_fam = defaultdict(lambda: defaultdict(int))
    for o, n, s, obj in inputs:
        if o in MAIN_IMAGE:
            by_obj[obj] += s
            kind = "bss" if ".bss" in o or n.startswith((".bss", ".sbss")) else \
                   "data" if o == ".main.rw" else "text/rodata"
            by_fam[family(n)][kind] += s
    print(f"\nimage by object (top {top}):")
    for obj, s in sorted(by_obj.items(), key=lambda x: -x[1])[:top]:
        print(f"  {s:>10,}  {obj}")
    print("\nimage by family (text/rodata, data, bss):")
    rows = sorted(by_fam.items(), key=lambda x: -sum(x[1].values()))
    for f, kinds in rows:
        total = sum(kinds.values())
        print(f"  {f:14s} {total:>10,}  ({kinds['text/rodata']:,} / {kinds['data']:,} / {kinds['bss']:,})")
    if section:
        rows = sorted(((s, n, obj) for o, n, s, obj in inputs if o == section), reverse=True)
        print(f"\n{section}: largest input sections (top {top}):")
        for s, n, obj in rows[:top]:
            print(f"  {s:>10,}  {n}  [{obj}]")


if __name__ == "__main__":
    main()
