"""What owns the FTR lane, per symbol, in the SHIPPING configuration.

FTR IS A SPAN, NOT A SYMBOL SET.  src/port/reloc_backend_renderer_dl.c:16863
opens it (`owner_start = cpuGetTiming()`, the `bl` at 0x0205eef0 inside
ndsFighterDisplayContractSubmit) and :16878 / :16937 close it, so the bucket is
the dynamic extent of that one function.  A per-PC profiler charges every leaf
to itself and never to the span it ran inside, so the bucket cannot be read off
a symbol table.  This reconstructs it from CALL FLOW.

THE MODEL, stated so it can be attacked:
  N(S)      exact invocations of S in the window = the profiler's instruction
            count at S's entry PC (the profiler is instruction-accurate).
  c(S->T)   exact calls S made to T = the instruction count at each `bl` PC
            inside S that targets T, summed.  For the ROOT only, sites BEFORE
            the span-opening `bl` are excluded -- 6,624 of the root's
            invocations enter, but only 3,200 reach the timer, the rest
            early-return above it and are not in the bucket.
  s(S)      self cycles of S / N(S) -- one invocation, excluding callees.
  I(S)      inclusive cost of one TOP-LEVEL invocation:
            s(S) + sum_T c(S->T)/N(S) * I(T), self-edge included, so a directly
            recursive S gets its whole tree (the 1/(1-r) factor).
  f(S)      invocations of S inside the span, ALL of them including recursive
            re-entries:  f(root) = 3,200, f(T) = sum_S f(S)*c(S->T)/N(S).
  incl(S)   f(S) * (1 - r_S) * I(S), r_S = c(S->S)/N(S).  The (1-r) converts
            f from all invocations to TOP-LEVEL ones, which is what I(S) prices.
            Without it ftDisplayMainDrawDefault reads 476,903 tk/fr -- larger
            than the whole lane -- because it recurses 0.96 times per call.
  self(S)   f(S) * s(S).  Sums to the root's inclusive exactly.

THE ONE ASSUMPTION: an invocation of S costs the same on average whether the
caller is inside the span or outside it -- the same assumption
../2026-08-16_shdt-mechanism/SHDT_MECHANISM.md's --attribute-leaves rests on.
It is NOT assumed for the answer as a whole: the model's total is checked
against the tick-HUD's own FTR bucket, a number this script never reads.

UNITS: 2 profile cycles = 1 project tick.
"""
import bisect
import csv
import sys

D = 'artifacts/performance/2026-08-16_ftr-attribution/'
NM = 'artifacts/performance/2026-08-16_sitr-excursion/c221-nm.txt'
MM = 'artifacts/performance/2026-08-16_sitr-excursion/c221-multimask.csv'
ROOT = 'ndsFighterDisplayContractSubmit'
GATE_PC = 0x0205eef0          # the `bl cpuGetTiming` that opens the span
MASKS = ['whole', 'm80prof', 'm80gate']
NFR = {'whole': 1601, 'm80prof': 80, 'm80gate': 80}
STALLS = ['issue', 'icache_fill', 'dcache_fill', 'write_buffer',
          'bus_contention', 'interlock']
MASK = sys.argv[1] if len(sys.argv) > 1 else 'whole'
FR = float(NFR[MASK])

addr, size = {}, {}
for line in open(NM):
    p = line.split()
    if len(p) == 4 and p[2] in 'tTwW' and int(p[1], 16):
        addr[p[3]] = int(p[0], 16)
        size[p[3]] = int(p[1], 16)

sym = {r['symbol']: {k: int(v) for k, v in r.items() if k != 'symbol'}
       for r in csv.DictReader(open(D + 'c221-symmask.csv'))}
site = {int(r['pc'], 16): {m: int(r['%s_instructions' % m]) for m in MASKS}
        for r in csv.DictReader(open(D + 'c221-sitemask.csv'))}


def cyc(s, m=None):
    return sym.get(s, {}).get('%s_total_cycles' % (m or MASK), 0)


N = {s: site.get(a, {}).get(MASK, 0) for s, a in addr.items()}

edge = {}
for r in csv.DictReader(open(D + 'c221-callsites.csv')):
    pc = int(r['site_pc'], 16)
    if r['caller'] == ROOT and pc < GATE_PC:
        continue                                   # above the timer: not in FTR
    n = site.get(pc, {}).get(MASK, 0)
    if n:
        k = (r['caller'], r['callee'])
        edge[k] = edge.get(k, 0) + n
out, incoming = {}, {}
for (a, b), n in edge.items():
    out.setdefault(a, []).append((b, n))
    incoming[b] = incoming.get(b, 0) + n

# The root's in-span SELF cost, exact: sum the whole-mask cycles of every PC of
# ndsFighterDisplayContractSubmit at or above the gate.  Charging the function
# whole would bill FTR for the 3,424 invocations that early-return above it.
root_lo, root_hi = addr[ROOT], addr[ROOT] + size[ROOT]
root_self_cycles = 0
if MASK == 'whole':
    for r in csv.DictReader(open(MM)):
        pc = int(r['pc'], 16)
        if GATE_PC <= pc < root_hi:
            root_self_cycles += int(r['whole_total_cycles'])
else:                                              # scale by the whole-mask ratio
    tot = cyc(ROOT, 'whole')
    root_self_cycles = cyc(ROOT)
ROOT_ENTRIES = site[GATE_PC][MASK]                 # invocations reaching the span

selfcyc = dict((s, cyc(s)) for s in addr)
selfcyc[ROOT] = root_self_cycles
# The root's edges were restricted to the post-gate sites, so its per-invocation
# frequencies must divide by the invocations that REACH the gate, not by every
# entry.  Dividing by N[ROOT] halved the whole flow and read 133,909 tk/fr with
# a uniform 48% share -- 3,200/6,624 -- on every symbol in the lane.
Neff = dict(N)
Neff[ROOT] = ROOT_ENTRIES

I = {s: (selfcyc[s] / Neff[s] if Neff.get(s) else 0.0) for s in addr}
for _ in range(400):
    d = 0.0
    for s in addr:
        if not Neff.get(s):
            continue
        v = selfcyc[s] / Neff[s]
        for t, n in out.get(s, ()):
            if Neff.get(t):
                v += (n / Neff[s]) * I[t]
        d = max(d, abs(v - I[s]))
        I[s] = v
    if d < 1e-9:
        break

f = {s: 0.0 for s in addr}
for _ in range(400):
    nf = {s: 0.0 for s in addr}
    nf[ROOT] = float(ROOT_ENTRIES)
    for s in addr:
        if not f[s] or not Neff.get(s):
            continue
        for t, n in out.get(s, ()):
            if Neff.get(t):
                nf[t] += f[s] * (n / Neff[s])
    if max(abs(nf[s] - f[s]) for s in addr) < 1e-9:
        f = nf
        break
    f = nf
r_self = {s: (edge.get((s, s), 0) / Neff[s] if Neff.get(s) else 0.0)
          for s in addr}

total_self = sum(f[s] * (selfcyc[s] / Neff[s]) for s in addr if Neff.get(s))
root_incl = f[ROOT] * (1 - r_self[ROOT]) * I[ROOT]
print('MASK %s   %d frames   root %s' % (MASK, NFR[MASK], ROOT))
print('root invocations/frame  %.2f enter, %.2f reach the timer (the span)'
      % (N[ROOT] / FR, ROOT_ENTRIES / FR))
print('MODEL TOTAL   root inclusive %.0f tk/fr   sum of per-symbol self %.0f'
      % (root_incl / (2 * FR), total_self / (2 * FR)))
print('CHECK vs the instrument: tick-HUD FTR on the c220 basis reads P50 '
      '290,432, band 41-120 290,400')
print('      (../2026-08-16_collision-setup-share/lanes.txt).  This script '
      'never reads that file.')
print('')

rows = []
for s in addr:
    if not Neff.get(s) or f[s] <= 0.0:
        continue
    selftk = f[s] * (selfcyc[s] / Neff[s]) / (2 * FR)
    inctk = f[s] * (1 - r_self[s]) * I[s] / (2 * FR)
    if selftk < 1.0 and inctk < 1.0:
        continue
    c = selfcyc[s]
    st = [(sym.get(s, {}).get('%s_%s' % (MASK, k), 0) / c if c else 0.0)
          for k in STALLS]
    ins = sym.get(s, {}).get('%s_instructions' % MASK, 0)
    rows.append(dict(self_=selftk, incl=inctk, sym=s, cf=f[s] / FR,
                     nf=N[s] / FR, sz=size.get(s, 0), st=st,
                     cpi=(c / ins if ins else 0.0)))

H = '%9s %9s %8s %7s %6s %6s  %-44s %s'
HD = H % ('self t/f', 'incl t/f', 'FTRc/f', 'bytes', 'cyc/in', 'share',
          'symbol', 'iss   ic   dc   wb  bus  ilk')


def show(rs, n):
    print(HD)
    for r in rs[:n]:
        print(H % ('%.0f' % r['self_'], '%.0f' % r['incl'], '%.2f' % r['cf'],
                   '%d' % r['sz'], '%.2f' % r['cpi'],
                   '%.0f%%' % (100.0 * r['cf'] / r['nf']) if r['nf'] else '-',
                   r['sym'][:44],
                   ' '.join('%4.0f%%' % (100 * x) for x in r['st'])))


print('=== FTR BY SELF COST (a leaf charged to itself) ===')
show(sorted(rows, key=lambda r: -r['self_']), 42)
print('')
print('=== FTR BY INCLUSIVE COST (the BUILDERS, leaves rolled up) ===')
show(sorted(rows, key=lambda r: -r['incl']), 26)

print('')
print('=== RECONCILIATION: direct call sites against entry-PC counts ===')
print('Shortfall = calls this model cannot see (register-indirect, veneers).')
print('%-44s %12s %12s %8s' % ('symbol', 'entry count', 'sites sum', 'covered'))
for r in sorted(rows, key=lambda r: -r['self_'])[:18]:
    e, i = N[r['sym']], incoming.get(r['sym'], 0)
    print('%-44s %12d %12d %7s'
          % (r['sym'][:44], e, i, ('%.0f%%' % (100.0 * i / e)) if e else '-'))
tot_in = sum(incoming.get(s, 0) for s in addr if f[s] > 0 and s != ROOT)
tot_en = sum(N[s] for s in addr if f[s] > 0 and s != ROOT)
print('ALL FTR-reached symbols: sites %d vs entries %d = %.1f%% covered'
      % (tot_in, tot_en, 100.0 * tot_in / tot_en))
