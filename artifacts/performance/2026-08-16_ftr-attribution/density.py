"""Fetch density INSIDE the FTR lane -- how much of what FTR pays to fetch runs.

../2026-08-14_hot-footprint/HOT_FOOTPRINT.md sized cold-path out-of-lining
whole-match (ceiling ~81,800 tk/fr, realistic 25,000-40,000) and left it as
"hand work per function ... must be sized against the >=16,000 tk/fr floor
before it is started."  Nothing since has sized it PER FUNCTION, and the
whole-match figure spans lanes whose cost is excursion, where a uniform saving
does not convert.  FTR is FLAT (band41-120 / P50 = 1.00), so a tick removed here
converts 1:1 at rank-80 -- which makes this the lane to size it in.

Method is HOT_FOOTPRINT's, restricted to the FTR flow:
  paid lines   32-byte lines of this function holding >=1 executed PC
  live B       instruction bytes at executed PCs
  pool B       4-byte literal-pool words, resolved from objdump's own
               `@ (addr <sym+off>)` annotation on every pc-relative load.
               Thumb-1 CANNOT materialise a 32-bit constant inline, so pool
               bytes are NOT removable and counting them as cold overstates
               the lever -- the correction HOT_FOOTPRINT section 2 had to make.
  cold B       paid - live - pool: fetched, never executed, not a constant
  ceiling      the fill this function would still pay if live+pool compacted
               into ceil((live+pool)/32) lines, times FTR's share of it.

Perfect compaction is not achievable at basic-block granularity; the ceiling is
an upper bound and is labelled as one.

UNITS: 2 profile cycles = 1 project tick.
"""
import collections
import csv
import re
import sys

D = 'artifacts/performance/2026-08-16_ftr-attribution/'
MM = 'artifacts/performance/2026-08-16_sitr-excursion/c221-multimask.csv'
DIS = sys.argv[1]
MASK = sys.argv[2] if len(sys.argv) > 2 else 'm80gate'
NFR = {'whole': 1601, 'm80prof': 80, 'm80gate': 80}[MASK]
LINE = 32

sys.argv = ['attribute.py', MASK]
import io
import contextlib
ns = {'__name__': '__main__', '__file__': D + 'attribute.py'}
with contextlib.redirect_stdout(io.StringIO()):
    exec(compile(open(D + 'attribute.py').read(), D + 'attribute.py', 'exec'),
         ns)
f, Neff, sym, addr, size = ns['f'], ns['Neff'], ns['sym'], ns['addr'], ns['size']
share = {s: (f[s] / Neff[s] if Neff.get(s) else 0.0) for s in addr}

FUNC = re.compile(r'^([0-9a-f]+) <(.+?)>:$')
INSN = re.compile(r'^\s*([0-9a-f]+):\s+((?:[0-9a-f]{2,8} )+)\s*(\S+)\s*(.*)$')
POOL = re.compile(r'@ \(?([0-9a-f]+) <')

owner, width = {}, {}
pool = set()
cur = None
for raw in open(DIS, errors='ignore'):
    m = FUNC.match(raw.rstrip('\n'))
    if m:
        cur = m.group(2)
        continue
    if cur is None:
        continue
    mi = INSN.match(raw)
    if not mi:
        continue
    pc = int(mi.group(1), 16)
    owner[pc] = cur
    width[pc] = len(mi.group(2).replace(' ', '')) // 2
    if mi.group(3).startswith('ldr') and '[pc' in mi.group(4):
        p = POOL.search(raw)
        if p:
            pool.add(int(p.group(1), 16))

executed = set()
for r in csv.DictReader(open(MM)):
    pc = int(r['pc'], 16)
    if pc in owner:
        executed.add(pc)

paid_lines = collections.defaultdict(set)
live_b = collections.Counter()
for pc in executed:
    fn = owner[pc]
    paid_lines[fn].add(pc & ~(LINE - 1))
    live_b[fn] += width[pc]
pool_b = collections.Counter()
for pc in pool:
    fn = owner.get(pc)
    if fn and (pc & ~(LINE - 1)) in paid_lines[fn]:
        pool_b[fn] += 4


def fill_tk(s):
    """FTR's share of this symbol's instruction-fetch ticks on the mask."""
    return (sym.get(s, {}).get('%s_icache_fill' % MASK, 0) * share.get(s, 0.0)
            / (2.0 * NFR))


rows = []
for s in addr:
    if share.get(s, 0.0) <= 0.0 or s not in paid_lines:
        continue
    lines = len(paid_lines[s])
    paid = lines * LINE
    live, pl = live_b[s], pool_b[s]
    cold = paid - live - pl
    need = -(-(live + pl) // LINE)
    ft = fill_tk(s)
    ceil_tk = ft * (lines - need) / lines if lines else 0.0
    rows.append((ceil_tk, ft, s, lines, paid, live, pl, cold, need,
                 size.get(s, 0)))

rows.sort(reverse=True)
tot_fill = sum(r[1] for r in rows)
tot_ceil = sum(r[0] for r in rows)
tot_paid = sum(r[4] for r in rows)
tot_live = sum(r[5] for r in rows)
tot_pool = sum(r[6] for r in rows)
print('FTR FETCH DENSITY, mask %s (%d frames).  Symbols the FTR flow reaches: %d'
      % (MASK, NFR, len(rows)))
print('paid %d B in %d lines | live %d B (%.1f%%) | pool %d B (%.1f%%) | '
      'cold %d B (%.1f%%)'
      % (tot_paid, tot_paid // LINE, tot_live, 100.0 * tot_live / tot_paid,
         tot_pool, 100.0 * tot_pool / tot_paid,
         tot_paid - tot_live - tot_pool,
         100.0 * (tot_paid - tot_live - tot_pool) / tot_paid))
print('FTR instruction fetch on this mask: %.0f tk/fr' % tot_fill)
print('PERFECT-COMPACTION CEILING (upper bound, not achievable): %.0f tk/fr'
      % tot_ceil)
print('')
H = '%9s %9s %6s %7s %7s %6s %7s %6s  %-42s'
print(H % ('ceil t/f', 'fill t/f', 'lines', 'paid B', 'live B', 'pool B',
           'cold B', 'need', 'symbol'))
for c, ft, s, lines, paid, live, pl, cold, need, sz in rows[:26]:
    print(H % ('%.0f' % c, '%.0f' % ft, '%d' % lines, '%d' % paid, '%d' % live,
               '%d' % pl, '%d' % cold, '%d' % need, s[:42]))

print('')
print('=== ITCM CANDIDATES: the same fetch, bought with ITCM instead ===')
print('A body in ITCM pays no fetch at all.  The bound below is NOT the fill')
print('figure -- an instruction still retires -- it is')
print('    cycles - (instructions + dcache + write_buffer + bus + interlock),')
print('the cost this symbol would shed if fetch were free and nothing else')
print('changed.  ITCM_CENSUS.md puts the recoverable pool at 688 B by eviction')
print('(+54), on top of 220 B free on the instrument today.')
K = ['dcache_fill', 'write_buffer', 'bus_contention', 'interlock']
cand = []
for c, ft, s, lines, paid, live, pl, cold, need, sz in rows:
    d = sym.get(s, {})
    cyc = d.get('%s_total_cycles' % MASK, 0)
    floor = (d.get('%s_instructions' % MASK, 0)
             + sum(d.get('%s_%s' % (MASK, k), 0) for k in K))
    gain = max(0, cyc - floor) * share.get(s, 0.0) / (2.0 * NFR)
    cand.append((gain, ft, s, sz, gain / sz if sz else 0))
cand.sort(reverse=True)
print('%9s %9s %7s %9s  %-46s' % ('fetch-free', 'fill t/f', 'bytes',
                                  'tk/fr/B', 'symbol'))
for g, ft, s, sz, per in cand[:14]:
    print('%9.0f %9.0f %7d %9.2f  %-46s' % (g, ft, sz, per, s[:46]))
