"""Pick hot static objects for DTCM from a fill census and find their input
sections. Usage: dtcm_pick.py ELF DMISS FRAMES BUILD_DIR BUDGET"""
import bisect
import collections
import csv
import glob
import os
import re
import subprocess
import sys

elf, dm, frames, bdir, budget = sys.argv[1], sys.argv[2], int(sys.argv[3]), sys.argv[4], int(sys.argv[5])
NM = 'C:/devkitPro/devkitARM/bin/arm-none-eabi-nm'
OD = 'C:/devkitPro/devkitARM/bin/arm-none-eabi-objdump'
objs = []
for l in subprocess.run([NM, '-S', '-n', elf], capture_output=True, text=True).stdout.split('\n'):
    p = l.split()
    if len(p) == 4 and p[2] in 'bBdD' and int(p[1], 16) > 0:
        objs.append((int(p[0], 16), int(p[1], 16), p[3], p[2]))
objs.sort()
oa = [o[0] for o in objs]
c = collections.Counter()
for r in csv.DictReader(open(dm)):
    line = int(r['line'], 16)
    i = bisect.bisect_right(oa, line + 31) - 1
    best = None
    while i >= 0 and objs[i][0] > line - 0x4000:
        o = objs[i]
        if o[0] + o[1] > line and o[0] < line + 32:
            ov = min(o[0] + o[1], line + 32) - max(o[0], line)
            if best is None or ov > best[0]:
                best = (ov, i)
        i -= 1
    if best:
        c[best[1]] += int(r['misses'])
rows = []
for k, v in c.items():
    a, s, n, t = objs[k]
    if 0x02ff0000 <= a < 0x02ff4000:
        continue
    if s > 400:
        continue
    f = v / frames
    rows.append((f / max(s, 32) * 1024, f, s, n, t))
rows.sort(reverse=True)
# symbol -> defining object file and section
defs = {}
for o in glob.glob(os.path.join(bdir, '*.o')):
    out = subprocess.run([OD, '-t', o], capture_output=True, text=True).stdout
    for l in out.split('\n'):
        m = re.match(r'[0-9a-f]+\s+([lg])\s+O\s+(\S+)\s+[0-9a-f]+\s+(\S+)$', l)
        if m:
            defs.setdefault(m.group(3), []).append((os.path.basename(o), m.group(2), m.group(1)))
used = 0
tot = 0
picked = []
for score, f, s, n, t in rows:
    if f < 5.0:
        continue
    d = defs.get(n, [])
    secs = sorted({x[1] for x in d})
    if len(d) != 1 or not (secs[0].startswith('.bss.') or secs[0].startswith('.data.')):
        print('skip %-44s defs=%s' % (n, d[:3]))
        continue
    need = (s + 3) & ~3
    if used + need > budget:
        continue
    used += need
    tot += f
    picked.append((n, secs[0], s, f))
for n, sec, s, f in picked:
    print('%6.1f %4d %-12s %s' % (f, s, sec.split('.')[1], n))
print('picked %d objects, %d bytes, %.0f fills/frame' % (len(picked), used, tot))
open(os.path.join(os.path.dirname(os.path.abspath(__file__)), 'dtcm_pick.txt'), 'w').write(
    '\n'.join('%s %s' % (sec, n) for n, sec, s, f in picked))
