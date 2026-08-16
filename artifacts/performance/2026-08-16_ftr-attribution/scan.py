"""One pass over the 3.87 GB v3-c221 capture, reducing to what FTR needs.

Emits two artifacts, both small enough to keep:

  c221-symmask.csv   per SYMBOL x mask: instructions, cycles and the five stall
                     classes.  Self cost, and the stall split the brief asks for.
  c221-sitemask.csv  per PC x mask: instructions ONLY, and only for PCs that are
                     a function ENTRY or a direct call SITE.  The profiler is
                     instruction-accurate, so an entry PC's instruction count is
                     that function's exact call count and a `bl` PC's is that
                     site's exact call count.  Everything the call-flow model
                     needs, and nothing else.

Masks: whole (1,601), m80prof, m80gate -- see masks.py.
UNITS: 2 profile cycles = 1 project tick.
"""
import bisect
import csv
import sys
import time

D = 'artifacts/performance/2026-08-16_ftr-attribution/'
PROF = 'artifacts/performance/2026-08-16_sitr-excursion/v3-c221/arm9-profile.csv'
NM = 'artifacts/performance/2026-08-16_sitr-excursion/c221-nm.txt'
MASKS = ['whole', 'm80prof', 'm80gate']
COLS = ['instructions', 'total_cycles', 'issue', 'icache_fill', 'dcache_fill',
        'write_buffer', 'bus_contention', 'interlock', 'halt_wait']

sets = {m: set(int(x) for x in open(D + 'regions-%s.txt' % m) if x.strip())
        for m in MASKS[1:]}

syms = []
for line in open(NM):
    p = line.split()
    if len(p) == 4 and p[2] in 'tTwW':
        a, s = int(p[0], 16), int(p[1], 16)
        if s:
            syms.append((a, a + s, p[3]))
syms.sort()
starts = [s[0] for s in syms]
entries = set(s[0] for s in syms)
sites = set(int(r['site_pc'], 16)
            for r in csv.DictReader(open(D + 'c221-callsites.csv')))
want = entries | sites


def owner(pc):
    i = bisect.bisect_right(starts, pc) - 1
    if i >= 0 and pc < syms[i][1]:
        return syms[i][2]
    return '<unattributed>'


NC = len(COLS)
NM_ = len(MASKS)
symacc = {}
siteacc = {}
pcown = {}
t0 = time.time()
n = 0
with open(PROF, 'rb') as fh:
    head = fh.readline().decode().rstrip('\n').split(',')
    idx = [head.index(c) for c in COLS]
    ri, pi = head.index('region'), head.index('pc')
    for line in fh:
        f = line.split(b',')
        n += 1
        reg = int(f[ri])
        pc = int(f[pi], 16)
        o = pcown.get(pc)
        if o is None:
            o = pcown[pc] = owner(pc)
        vals = [int(f[j]) for j in idx]
        which = [0]
        for k, m in enumerate(MASKS[1:], start=1):
            if reg in sets[m]:
                which.append(k)
        rec = symacc.get(o)
        if rec is None:
            rec = symacc[o] = [0] * (NM_ * NC)
        for k in which:
            b = k * NC
            for j, v in enumerate(vals):
                rec[b + j] += v
        if pc in want:
            srec = siteacc.get(pc)
            if srec is None:
                srec = siteacc[pc] = [0] * NM_
            ins = vals[0]
            for k in which:
                srec[k] += ins
        if (n & 0x3fffff) == 0:
            print('  %d rows  %.0fs' % (n, time.time() - t0), file=sys.stderr,
                  flush=True)

print('rows=%d  symbols=%d  tracked_pcs=%d  %.0fs'
      % (n, len(symacc), len(siteacc), time.time() - t0), file=sys.stderr)

w = csv.writer(open(D + 'c221-symmask.csv', 'w', newline=''))
w.writerow(['symbol'] + ['%s_%s' % (m, c) for m in MASKS for c in COLS])
for s, rec in sorted(symacc.items()):
    w.writerow([s] + rec)

w = csv.writer(open(D + 'c221-sitemask.csv', 'w', newline=''))
w.writerow(['pc'] + ['%s_instructions' % m for m in MASKS])
for pc, rec in sorted(siteacc.items()):
    w.writerow(['0x%08x' % pc] + rec)

open(D + 'scan.meta.txt', 'w').write(
    'source=%s\nrows=%d\nsymbols=%d\ntracked_pcs=%d\n'
    'masks: whole=1601 m80prof=%d m80gate=%d\n'
    'alignment=region = frame - 439 (m80gate only; m80prof needs none)\n'
    'cycles_per_tick=2\n'
    % (PROF, n, len(symacc), len(siteacc), len(sets['m80prof']),
       len(sets['m80gate'])))
