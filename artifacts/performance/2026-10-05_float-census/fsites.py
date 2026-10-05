"""Map the lab float census's per-site table to functions and source files.

usage: fsites.py <gdb log> <elf> [frames]
The log holds `print/x gNdsTask9FloatSiteLr`, `print/u gNdsTask9FloatSiteCount`
and `print/u gNdsTask9FloatCostTicks` / `gNdsTask9FloatCostCalls` lines.
"""
import bisect
import collections
import re
import subprocess
import sys

log, elf = sys.argv[1], sys.argv[2]
frames = int(sys.argv[3]) if len(sys.argv) > 3 else 1
BIN = 'C:/devkitPro/devkitARM/bin/arm-none-eabi-'

text = open(log, errors='replace').read()


def array(name):
    m = re.search(r'\$\d+ = \{([^}]*)\}\s*\n(?=[^\n]*$|)', text) if False else None
    for block in re.finditer(r'ARRAY ' + name + r'\n\$\d+ = \{([^}]*)\}', text):
        vals = []
        for tok in block.group(1).split(','):
            tok = tok.strip()
            rep = re.match(r'(\S+) <repeats (\d+) times>', tok)
            if rep:
                vals += [int(rep.group(1), 0)] * int(rep.group(2))
            elif tok:
                vals.append(int(tok, 0))
        return vals
    raise SystemExit('missing ' + name)


lrs = array('gNdsTask9FloatSiteLr')
counts = array('gNdsTask9FloatSiteCount')
ticks = array('gNdsTask9FloatCostTicks')
calls = array('gNdsTask9FloatCostCalls')
ROUTINES = ['fadd', 'fsub', 'frsub', 'fmul', 'fdiv', 'fcmpeq', 'fcmplt', 'fcmple',
            'fcmpge', 'fcmpgt', 'fcmpun', 'f2iz', 'f2uiz', 'i2f', 'ui2f', 'l2f',
            'ul2f', 'f2d', 'd2f', 'dadd', 'dsub', 'drsub', 'dmul', 'ddiv',
            'dcmpeq', 'dcmplt', 'dcmple', 'dcmpge', 'dcmpgt', 'dcmpun', 'd2iz',
            'i2d', 'ui2d', 'l2d', 'ul2d']
cost = {r: (ticks[i] / calls[i] if calls[i] else 0.0) for i, r in enumerate(ROUTINES)}

nm = subprocess.run([BIN + 'nm', '-S', '--defined-only', elf], capture_output=True,
                    text=True).stdout
syms = []
for line in nm.splitlines():
    p = line.split()
    if len(p) == 4 and p[2] in 'tTwW':
        syms.append((int(p[0], 16) & ~1, int(p[1], 16), p[3]))
syms.sort()
addrs = [s[0] for s in syms]


def func(pc):
    i = bisect.bisect_right(addrs, pc) - 1
    if i >= 0 and pc < syms[i][0] + max(syms[i][1], 4):
        return syms[i][2]
    return '?'


# The routine each site calls: the bl just before the return address.
dis = subprocess.run([BIN + 'objdump', '-d', '--no-show-raw-insn', elf],
                     capture_output=True, text=True).stdout
target = {}
for line in dis.splitlines():
    m = re.match(r'\s*([0-9a-f]+):\s+(?:bl|blx)\s+[0-9a-f]+ <__wrap___aeabi_(\w+?)(\+0x[0-9a-f]+)?>', line)
    if m:
        target[int(m.group(1), 16)] = m.group(2)

per_func = collections.defaultdict(lambda: collections.Counter())
per_func_cyc = collections.Counter()
total_calls = 0
total_cyc = 0.0
site_rows = []
for lr, n in zip(lrs, counts):
    if lr == 0 or n == 0:
        continue
    pc = (lr & ~1) - 4
    routine = target.get(pc) or target.get(pc + 2) or '?'
    f = func(pc)
    c = cost.get(routine, 40.0) * n
    per_func[f][routine] += n
    per_func_cyc[f] += c
    total_calls += n
    total_cyc += c
    site_rows.append((c, n, f, routine, pc))

print(f'sites {len(site_rows)}  calls {total_calls:,}  est cycles {total_cyc:,.0f}  '
      f'per frame {total_calls / frames:,.0f} calls, {total_cyc / frames:,.0f} cycles')
print('routine cost (ticks/call): ' + ' '.join(f'{r}={cost[r]:.0f}' for r in ROUTINES if cost[r]))
files = {}
names = [f for f, _ in per_func_cyc.most_common(120)]
a2l = subprocess.run([BIN + 'addr2line', '-e', elf] +
                     [hex(next(s[0] for s in syms if s[2] == n)) for n in names if n != '?'],
                     capture_output=True, text=True).stdout.splitlines()
for n, loc in zip([n for n in names if n != '?'], a2l):
    files[n] = re.sub(r'.*Smash64DS_Port/', '', loc.replace('\\', '/')).split(':')[0]
cum = 0.0
print(f'{"cyc/frame":>10} {"cum%":>5} {"calls/f":>8}  function  [file]  routines')
for f, c in per_func_cyc.most_common(120):
    cum += c
    mix = ' '.join(f'{r}:{k // max(frames, 1)}' for r, k in per_func[f].most_common(4))
    print(f'{c / frames:>10,.0f} {100 * cum / total_cyc:>5.1f} '
          f'{sum(per_func[f].values()) / frames:>8,.0f}  {f}  [{files.get(f, "?")}]  {mix}')
