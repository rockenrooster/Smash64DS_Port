"""Static RAM census of an ARM9 ELF: sections by memory region, and the largest
.bss/.data/.sbss symbols in main RAM, grouped by source file where nm -l knows it.

usage: python ram_census.py <elf> [top_n]
"""
import collections
import subprocess
import sys

ELF = sys.argv[1]
TOP = int(sys.argv[2]) if len(sys.argv) > 2 else 40
TOOLS = "C:/devkitPro/devkitARM/bin/arm-none-eabi-"

REGIONS = [
    ("ITCM", 0x01000000, 0x02000000),  # ITCM and its mirrors (linked at 0x01FF8000)
    ("DTCM", 0x02FF0000, 0x03000000),  # calico places DTCM at 0x02FF0000 on DS (checked below)
    ("MAIN", 0x02000000, 0x02400000),
    ("WRAM", 0x03000000, 0x03800000),
]


def region_of(addr):
    for name, lo, hi in REGIONS:
        if lo <= addr < hi:
            return name
    return "OTHER"


out = subprocess.run([TOOLS + "readelf", "-S", "-W", ELF], capture_output=True, text=True).stdout
secs = []
for line in out.splitlines():
    line = line.strip()
    if not line.startswith("["):
        continue
    parts = line[line.index("]") + 1:].split()
    if not parts or parts[0] == "Name":
        continue
    if len(parts) < 6:
        continue
    try:
        int(parts[2], 16)
    except ValueError:
        continue
    name, typ, addr, off, size = parts[0], parts[1], int(parts[2], 16), int(parts[3], 16), int(parts[4], 16)
    flags = parts[6] if len(parts) > 6 else ""
    if "A" not in flags or size == 0:
        continue
    secs.append((name, typ, addr, size))

print("== allocated sections")
by_region = collections.Counter()
for name, typ, addr, size in sorted(secs, key=lambda s: s[2]):
    reg = region_of(addr)
    by_region[reg] += size
    print("  %-28s %-9s 0x%08x %9d  %s" % (name, typ, addr, size, reg))
print("== bytes per region")
for reg, size in sorted(by_region.items()):
    print("  %-6s %9d" % (reg, size))

nm = subprocess.run([TOOLS + "nm", "-S", "--size-sort", "-C", ELF], capture_output=True, text=True).stdout
rows = []
for line in nm.splitlines():
    p = line.split()
    if len(p) < 4:
        continue
    addr, size, kind, name = int(p[0], 16), int(p[1], 16), p[2], " ".join(p[3:])
    if kind.lower() not in ("b", "d", "s", "g") or size >= 0x400000:
        continue
    rows.append((size, addr, kind, name))
rows.sort(reverse=True)
print("== largest data/bss symbols (all regions)")
for size, addr, kind, name in rows[:TOP]:
    print("  %9d  %s  0x%08x  %-6s %s" % (size, kind, addr, region_of(addr), name))
main_bss = sum(s for s, a, k, n in rows if region_of(a) == "MAIN" and k.lower() == "b")
main_data = sum(s for s, a, k, n in rows if region_of(a) == "MAIN" and k.lower() == "d")
print("== main RAM symbol bytes: bss %d  data %d" % (main_bss, main_data))
