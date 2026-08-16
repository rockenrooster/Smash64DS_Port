"""Every direct call site in the linked c221 image, from the disassembly.

    arm-none-eabi-objdump -d --no-show-raw-insn \
        builds/build-c221-sitrprof/smash64ds-battle-playable-tickhud-hwtri.elf

Emits one row per `bl`/`blx` instruction: the site's PC, the function it sits
in, and the function it calls.  The profiler's per-(region, pc) INSTRUCTION
count at a `bl` PC is exactly the number of calls that site made on that frame
-- the same fact ../2026-08-16_shdt-mechanism used for --attribute-leaves, and
the same one MEMORY.md's "entry PC gives exact call counts" records.

Tail calls matter here and are captured too: an unconditional `b` whose target
is OUTSIDE the containing symbol is a call, not a branch, and dropping them
would silently orphan whole subtrees.  They are marked so the reconciliation
can report them separately.
"""
import bisect
import csv
import re
import sys

DIS = sys.argv[1]
OUT = 'artifacts/performance/2026-08-16_ftr-attribution/c221-callsites.csv'
NM = 'artifacts/performance/2026-08-16_sitr-excursion/c221-nm.txt'

syms = []
for line in open(NM):
    p = line.split()
    if len(p) == 4 and p[2] in 'tTwW':
        a, s = int(p[0], 16), int(p[1], 16)
        if s:
            syms.append((a, a + s, p[3]))
syms.sort()
starts = [s[0] for s in syms]


def owner(pc):
    i = bisect.bisect_right(starts, pc) - 1
    if i >= 0 and pc < syms[i][1]:
        return syms[i][2], syms[i][0], syms[i][1]
    return '<unattributed>', 0, 0


HDR = re.compile(r'^([0-9a-f]{8}) <(.+)>:$')
INS = re.compile(r'^\s*([0-9a-f]+):\t(\S+)(?:\s+(.*))?$')
TGT = re.compile(r'^([0-9a-f]+) <([^>]+)>')

rows = []
cur = None
for line in open(DIS):
    m = HDR.match(line)
    if m:
        cur = m.group(2)
        continue
    m = INS.match(line)
    if not m:
        continue
    pc, mn, ops = int(m.group(1), 16), m.group(2), (m.group(3) or '')
    base = mn.split('.')[0]
    if base not in ('bl', 'blx', 'b'):
        continue
    t = TGT.match(ops.strip())
    if not t:
        continue                      # register-indirect: no static target
    tgt = int(t.group(1), 16)
    sname, slo, shi = owner(pc)
    tname, tlo, _ = owner(tgt)
    if base == 'b':
        if slo <= tgt < shi or tlo == 0:
            continue                  # intra-function branch, or unknown target
        kind = 'tail'
    else:
        kind = 'call'
    if tgt != tlo:
        continue                      # branch into a function body, not an entry
    rows.append((('0x%08x' % pc), sname, tname, kind))

w = csv.writer(open(OUT, 'w', newline=''))
w.writerow(['site_pc', 'caller', 'callee', 'kind'])
w.writerows(rows)
n_call = sum(1 for r in rows if r[3] == 'call')
print('call sites %d (bl/blx %d, tail-b %d), distinct callees %d, callers %d'
      % (len(rows), n_call, len(rows) - n_call,
         len(set(r[2] for r in rows)), len(set(r[1] for r in rows))))
