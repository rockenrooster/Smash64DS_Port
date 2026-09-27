"""Attribute D-cache line fills (melonDS <base>.dmiss.csv) to data objects.

Usage: dmiss_report.py DMISS_CSV ELF FRAMES FT_BASE [PARTS_BASE] [PTYPE]
"""
import bisect
import collections
import csv
import re
import subprocess
import sys

dmiss, elf, frames = sys.argv[1], sys.argv[2], int(sys.argv[3])
ft_base = int(sys.argv[4], 0) if len(sys.argv) > 4 else 0
parts_base = int(sys.argv[5], 0) if len(sys.argv) > 5 else 0
ptype = sys.argv[6] if len(sys.argv) > 6 else None
FT_SIZE = 3012
NM = 'C:/devkitPro/devkitARM/bin/arm-none-eabi-nm'

objs = []
funcs = []
for line in subprocess.run([NM, '-S', '-n', elf], capture_output=True, text=True).stdout.split('\n'):
    p = line.split()
    if len(p) == 4:
        a, s, t, n = int(p[0], 16), int(p[1], 16), p[2], p[3]
        if t in 'bBdDrRvV' and s > 0:
            objs.append((a, s, n))
        elif t in 'tTwW':
            funcs.append((a, s, n))
objs.sort()
funcs.sort()
oa = [o[0] for o in objs]
fa = [f[0] for f in funcs]


def obj_at(addr):
    i = bisect.bisect_right(oa, addr) - 1
    if i >= 0 and addr < objs[i][0] + objs[i][1]:
        return objs[i][2], addr - objs[i][0]
    return None, 0


def func_at(pc):
    i = bisect.bisect_right(fa, pc) - 1
    if i >= 0 and pc < funcs[i][0] + max(funcs[i][1], 2):
        return funcs[i][2]
    return '?%08x' % pc


fields = []
if ptype:
    for line in open(ptype, encoding='utf-8', errors='replace'):
        m = re.match(r'/\*\s+(\d+)\s+(?::\s*\d+\s+)?\|\s+(\d+)\s+\*/    (?! )(.*?);', line)
        if m:
            nm = re.findall(r'(\w+)(?:\[[^\]]*\])*\s*$', m.group(3).split(':')[0].strip())
            if nm:
                fields.append((int(m.group(1)), int(m.group(2)), nm[0]))
fields.sort()
fo = [f[0] for f in fields]


def field_at(off):
    i = bisect.bisect_right(fo, off) - 1
    return fields[i][2] if i >= 0 else '?'


by_class = collections.Counter()
by_obj = collections.Counter()
by_func = collections.Counter()
by_func_class = collections.defaultdict(collections.Counter)
ft_line = collections.Counter()
ft_line_funcs = collections.defaultdict(collections.Counter)
total = 0
for r in csv.DictReader(open(dmiss)):
    pc = int(r['pc'], 16)
    line = int(r['line'], 16)
    n = int(r['misses'])
    total += n
    fn = func_at(pc)
    by_func[fn] += n
    if ft_base and ft_base <= line < ft_base + 4 * FT_SIZE + 32:
        off = (line - ft_base) % FT_SIZE
        cls = 'FTStruct'
        ft_line[off // 32] += n
        ft_line_funcs[off // 32][fn] += n
    elif parts_base and parts_base <= line < parts_base + 4 * 64 * 0x100:
        cls = 'FTParts?'
    else:
        name, _ = obj_at(line)
        if name:
            cls = 'static'
            by_obj[name] += n
        elif 0x02000000 <= line < 0x02400000:
            cls = 'heap/other-main'
        else:
            cls = 'other %02x' % (line >> 24)
    by_class[cls] += n
    by_func_class[fn][cls] += n

print('frames %d, line fills/frame %.0f' % (frames, total / frames))
for c, n in by_class.most_common():
    print('  %-16s %8.0f /fr  %5.1f%%' % (c, n / frames, 100.0 * n / total))
print('--- top functions by line fills/frame (class split)')
for fn, n in by_func.most_common(40):
    top = ', '.join('%s %.0f' % (c, v / frames) for c, v in by_func_class[fn].most_common(3))
    print('%8.0f  %-48s %s' % (n / frames, fn[:48], top))
print('--- top static objects')
for o, n in by_obj.most_common(40):
    print('%8.0f  %s' % (n / frames, o))
if ft_base:
    print('--- FTStruct lines (offset/32) by fills/frame (4 structs summed)')
    tot_ft = sum(ft_line.values())
    acc = 0
    for ln, n in ft_line.most_common(60):
        acc += n
        fs = sorted({field_at(o) for o in range(ln * 32, ln * 32 + 32)})
        fns = ', '.join('%s %.0f' % (f[:24], v / frames) for f, v in ft_line_funcs[ln].most_common(2))
        print('%3d %6.0f cum%5.1f%%  %-40s | %s' % (ln, n / frames, 100.0 * acc / tot_ft, ','.join(fs)[:40], fns))
    print('FTStruct lines touched: %d of %d' % (len(ft_line), (FT_SIZE + 31) // 32))
