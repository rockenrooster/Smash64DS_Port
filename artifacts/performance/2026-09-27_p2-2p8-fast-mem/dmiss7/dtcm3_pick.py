"""Pick small static objects for DTCM by D-fills per byte; resolve their input
sections from the build's object files.

usage: dtcm3_pick.py ELF DMISS FRAMES BUILD_DIR BUDGET
"""
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
OBJDUMP = 'C:/devkitPro/devkitARM/bin/arm-none-eabi-objdump'
objs = []
for l in subprocess.run([NM, '-S', '-n', elf], capture_output=True, text=True).stdout.splitlines():
    p = l.split()
    if len(p) == 4 and p[2] in 'bBdD' and int(p[1], 16) > 0:
        a = int(p[0], 16)
        if 0x02000000 <= a < 0x02400000:
            objs.append((a, int(p[1], 16), p[3], p[2]))
objs.sort()
oa = [o[0] for o in objs]
fills = collections.Counter()
for r in csv.DictReader(open(dm)):
    line = int(r['line'], 16)
    n = int(r['misses'])
    i = bisect.bisect_right(oa, line + 31) - 1
    best = None
    while i >= 0 and objs[i][0] + 0x10000 > line:
        o = objs[i]
        if o[0] < line + 32 and o[0] + o[1] > line:
            ov = min(o[0] + o[1], line + 32) - max(o[0], line)
            if best is None or ov > best[0]:
                best = (ov, i)
        if o[0] < line - 0x4000:
            break
        i -= 1
    if best:
        fills[best[1]] += n
rows = []
for k, v in fills.items():
    a, s, n, t = objs[k]
    if s <= 512:
        rows.append((v / frames / max(s, 4), v / frames, s, n, t))
rows.sort(reverse=True)

# input sections from object files
want = {r[3] for r in rows[:200]}
secmap = collections.defaultdict(set)
ofiles = glob.glob(os.path.join(bdir, '*.o'))
for i in range(0, len(ofiles), 150):
    out = subprocess.run([OBJDUMP, '-t'] + ofiles[i:i + 150], capture_output=True, text=True).stdout
    cur = None
    for line in out.splitlines():
        m = re.match(r'^(\S+\.o):\s+file format', line)
        if m:
            cur = os.path.basename(m.group(1))
            continue
        m = re.match(r'^[0-9a-f]+\s+[lgw! ]+\s*[dDoOFfIi ]*\s*(\S+)\s+[0-9a-f]+\s+(\S+)$', line)
        if m and m.group(2) in want and m.group(1) not in ('*UND*',):
            secmap[m.group(2)].add((m.group(1), cur))

taken = 0
total = 0.0
picked = []
for dens, f, s, n, t in rows:
    if dens < 0.5:
        break
    secs = secmap.get(n)
    if not secs or len(secs) != 1:
        print('skip', n, secs)
        continue
    sec, ofile = next(iter(secs))
    if not (sec.startswith('.bss.') or sec.startswith('.data.') or sec.startswith('.sbss')):
        print('skip-sec', n, sec)
        continue
    if taken + s > budget:
        continue
    taken += s
    total += f
    picked.append((sec, n, s, f, ofile))
for sec, n, s, f, o in picked:
    print('%-60s %4d %6.1f %s' % (sec, s, f, o))
print('bytes', taken, 'fills/frame', round(total, 1))
