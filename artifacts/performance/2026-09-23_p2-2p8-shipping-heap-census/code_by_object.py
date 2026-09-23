"""Attribute a linked ARM9 ELF's symbols to the object files that define them.
usage: python code_by_object.py <build_dir> <elf>"""
import collections, pathlib, subprocess, sys
NM = "C:/devkitPro/devkitARM/bin/arm-none-eabi-nm"
bdir, elf = pathlib.Path(sys.argv[1]), sys.argv[2]
owner = {}
dup = set()
for o in sorted(bdir.glob("*.o")):
    out = subprocess.run([NM, "--defined-only", str(o)], capture_output=True, text=True).stdout
    for line in out.splitlines():
        p = line.split()
        if len(p) < 3:
            continue
        kind, name = p[1], p[2]
        if kind.lower() not in "tdbr":
            continue
        key = name
        if key in owner and owner[key] != o.stem:
            dup.add(key)
        owner.setdefault(key, o.stem)
out = subprocess.run([NM, "-S", "--defined-only", elf], capture_output=True, text=True).stdout
by = collections.defaultdict(collections.Counter)
unk = collections.Counter()
for line in out.splitlines():
    p = line.split()
    if len(p) < 4:
        continue
    addr, size, kind, name = int(p[0], 16), int(p[1], 16), p[2], p[3]
    if size >= 0x400000:
        continue
    region = "itcm" if addr < 0x02000000 else ("dtcm" if addr >= 0x02FF0000 else "main")
    k = kind.lower()
    cls = {"t": "text", "d": "data", "b": "bss", "r": "rodata"}.get(k)
    if cls is None:
        continue
    o = owner.get(name)
    if o is None:
        unk[cls] += size
        o = "?"
    by[o][region + "." + cls] += size
tot = collections.Counter()
rows = []
for o, c in by.items():
    main = c["main.text"] + c["main.rodata"] + c["main.data"]
    rows.append((main, c["main.bss"], c["itcm.text"], o))
    tot.update(c)
rows.sort(reverse=True)
print("object main(text+ro+data) main.bss itcm")
for m, b, i, o in rows:
    print("%-58s %9d %8d %6d" % (o, m, b, i))
print("TOTAL", dict(tot))
print("dup names", len(dup))
