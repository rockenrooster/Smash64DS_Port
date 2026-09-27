"""Split D-cache fills into stack-relative (sp-based / pop) and other, by function.

usage: stack_fills.py ELF DMISS_CSV FRAMES
"""
import bisect
import collections
import csv
import re
import subprocess
import sys

elf, dm, frames = sys.argv[1], sys.argv[2], int(sys.argv[3])
OBJDUMP = 'C:/devkitPro/devkitARM/bin/arm-none-eabi-objdump'
NM = 'C:/devkitPro/devkitARM/bin/arm-none-eabi-nm'

rows = []
pcs = set()
for r in csv.DictReader(open(dm)):
    pc = int(r['pc'], 16)
    rows.append((pc, int(r['line'], 16), int(r['misses'])))
    pcs.add(pc)

funcs = []
for l in subprocess.run([NM, '-S', '-n', elf], capture_output=True, text=True).stdout.splitlines():
    p = l.split()
    if len(p) == 4 and p[2] in 'tTwW':
        funcs.append((int(p[0], 16), int(p[1], 16), p[3]))
funcs.sort()
fa = [f[0] for f in funcs]


def fn(pc):
    i = bisect.bisect_right(fa, pc) - 1
    return funcs[i][2] if i >= 0 else '?'


# disassemble whole ELF once; map address -> instruction text
insn = {}
out = subprocess.run([OBJDUMP, '-d', '--no-show-raw-insn', elf], capture_output=True, text=True).stdout
for line in out.splitlines():
    m = re.match(r'^\s*([0-9a-f]+):\s+(.*)$', line)
    if m:
        a = int(m.group(1), 16)
        if a in pcs:
            insn[a] = m.group(2)


def is_stack(txt):
    if txt is None:
        return None
    t = txt.split(';')[0]
    if re.match(r'^(pop|ldmia\s+sp|ldmfd\s+sp|ldm\s+sp)', t):
        return True
    if re.search(r'\[sp(,|\])', t):
        return True
    return False


tot = collections.Counter()
per = collections.defaultdict(collections.Counter)
unknown = 0
for pc, ln, m in rows:
    s = is_stack(insn.get(pc))
    if s is None:
        unknown += m
        continue
    k = 'stack' if s else 'other'
    tot[k] += m
    per[k][fn(pc)] += m
print('fills/frame: stack %.0f other %.0f unknown-pc %.0f' % (tot['stack'] / frames, tot['other'] / frames, unknown / frames))
print('--- stack fills by function')
for f, m in per['stack'].most_common(40):
    print('%7.1f %s' % (m / frames, f))

# region view: stack vs other per 512 B block over a range given as argv[4]-argv[5]
if len(sys.argv) > 5:
    lo, hi = int(sys.argv[4], 16), int(sys.argv[5], 16)
    blk = collections.defaultdict(collections.Counter)
    for pc, ln, m in rows:
        if lo <= ln < hi:
            s = is_stack(insn.get(pc))
            blk[ln >> 9]['stack' if s else 'other'] += m
    for b in sorted(blk):
        print('%s stack %.1f other %.1f' % (hex(b << 9), blk[b]['stack'] / frames, blk[b]['other'] / frames))
