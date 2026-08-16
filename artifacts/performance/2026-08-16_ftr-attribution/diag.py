"""Diagnostic: is the FTR call graph a DAG, and where does the flow enter?

Written because the first attribution run produced an inclusive figure for
ftDisplayMainDrawDefault (476,903 tk/fr) LARGER than the root's own inclusive
(278,625), which is impossible for a subtree.  Either the graph has a cycle or
the flow enters that symbol by an edge its caller does not own.
"""
import csv
import sys

D = 'artifacts/performance/2026-08-16_ftr-attribution/'
NM = 'artifacts/performance/2026-08-16_sitr-excursion/c221-nm.txt'
ROOT = 'ndsFighterDisplayContractSubmit'
MASK = 'whole'
FR = 1601

addr = {}
for line in open(NM):
    p = line.split()
    if len(p) == 4 and p[2] in 'tTwW' and int(p[1], 16):
        addr[p[3]] = int(p[0], 16)
site = {int(r['pc'], 16): int(r['%s_instructions' % MASK])
        for r in csv.DictReader(open(D + 'c221-sitemask.csv'))}
N = {s: site.get(a, 0) for s, a in addr.items()}
edge, kinds = {}, {}
for r in csv.DictReader(open(D + 'c221-callsites.csv')):
    n = site.get(int(r['site_pc'], 16), 0)
    if n:
        k = (r['caller'], r['callee'])
        edge[k] = edge.get(k, 0) + n
        kinds[k] = r['kind']
out, inn = {}, {}
for (a, b), n in edge.items():
    out.setdefault(a, []).append((b, n))
    inn.setdefault(b, []).append((a, n))

for name in sys.argv[1:] or [ROOT, 'ftDisplayMainDrawDefault',
                             'ndsFighterDisplayContractCountFlags']:
    print('== %s   N=%d (%.2f/fr) ==' % (name, N.get(name, 0),
                                         N.get(name, 0) / FR))
    print('  IN:')
    for a, n in sorted(inn.get(name, []), key=lambda x: -x[1])[:8]:
        print('    %-52s %10d  %.2f per caller-invocation'
              % (a[:52], n, n / N[a] if N.get(a) else float('nan')))
    tot = sum(n for _, n in inn.get(name, []))
    print('    sites total %d vs entry %d (%.0f%%)'
          % (tot, N.get(name, 0),
             100.0 * tot / N[name] if N.get(name) else 0))
    print('  OUT:')
    for b, n in sorted(out.get(name, []), key=lambda x: -x[1])[:10]:
        print('    %-52s %10d  %.2f per invocation  [%s]'
              % (b[:52], n, n / N[name] if N.get(name) else float('nan'),
                 kinds[(name, b)]))
    print('')

# --- cycle detection over the edges the FTR flow actually uses -------------
colour = {}
cycles = []
stack = []


def dfs(u):
    colour[u] = 1
    stack.append(u)
    for v, _n in out.get(u, ()):
        if not N.get(v):
            continue
        if colour.get(v) == 1:
            cycles.append(stack[stack.index(v):] + [v])
        elif colour.get(v, 0) == 0:
            dfs(v)
    stack.pop()
    colour[u] = 2


sys.setrecursionlimit(20000)
dfs(ROOT)
print('reachable from %s: %d symbols' % (ROOT, sum(1 for c in colour if
                                                   colour[c] == 2)))
print('BACK EDGES (cycles) found: %d' % len(cycles))
for c in cycles[:12]:
    print('   ' + ' -> '.join(x[:34] for x in c))
