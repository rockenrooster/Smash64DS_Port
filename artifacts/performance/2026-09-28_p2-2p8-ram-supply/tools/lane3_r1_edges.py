#!/usr/bin/env python3
"""lane3_r1_edges.py -- what cutting the R1 entry REFERENCES frees, edge by edge.

Usage: lane3_r1_edges.py <graph.pkl> <src.csv> [--csv out.csv] [--list N]

R1 = the four retired fighter modes: hierarchy mode 7, the per-root hardware executor,
the CPU triangle rasteriser and the resident raw-path tables.  Their runtime entries are
called only from ndsFighterMarioFoxDLAllDrawForSlot.  This tool answers two things:

  1. Who references each entry?  (a second referrer would keep the entry linked even
     after DrawForSlot stops calling it.)
  2. Does cutting only the DrawForSlot -> entry EDGES free the same set as deleting the
     entry NODES?  (Equal sets mean the call-site edit alone unlinks R1.)

Also prints the bytes of the freed set by section, and the resident-table share that
needs a second edit (the struct initialisers hold the corner tables alive).
"""
import collections
import csv
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import lane3_refgraph as rg  # noqa: E402

HEAP = {'.main', '.main.rw', '.main.bss', '.text.hot', '.text.hot.draw', '.text.frontend_resident'}
ENTRY = re.compile(r'^(ndsRendererExecuteNativeFighterOwnerHierarchy|ndsRendererAdapterPrepareNativeOwnerHierarchy|'
                   r'ndsRendererAdapterBuildNativeHierarchyInputs|ndsRendererBeginNativeFighterOwner|'
                   r'ndsRendererExecuteNativeFighterRoot|ndsRendererEndNativeFighterOwner|'
                   r'ndsRendererAbortNativeFighterOwner)')
DRAW = re.compile(r'^ndsFighterMarioFoxDLAllDrawForSlot')
TAB = (re.compile(r'^(sNdsNativeFighterHighTables|sNdsNativeFighterLowTables)'),
       re.compile(r'^sNdsNativeFighter(PackedCorners|RunFirstCorner)'))


def load_src(path):
    d = {}
    with open(path, newline='') as f:
        for r in csv.DictReader(f):
            d[(r['name'], int(r['addr']))] = (r['srcfile'], r['line'])
    return d


def tally(L, idxs):
    t = collections.Counter()
    for i in idxs:
        s = L.syms[i]
        t[(s['sec'], s['kind'])] += s['size']
    return t


def summary(L, idxs):
    t = tally(L, idxs)
    heap = sum(v for (sec, k), v in t.items() if sec in HEAP)
    itcm = sum(v for (sec, k), v in t.items() if sec == '.itcm')
    other = sum(v for (sec, k), v in t.items() if sec not in HEAP and sec != '.itcm')
    return heap, itcm, other, t


def main():
    a = sys.argv
    L = rg.Loaded(a[1])
    src = load_src(a[2])
    roots = ['main', 'crt0Startup']
    full = L.reach(roots)
    print('== entries and their referrers')
    for i, s in enumerate(L.syms):
        if ENTRY.search(s['name']):
            refs = sorted(L.syms[c]['name'] for c in L.inn.get(i, ()))
            print('  %-58s %6d B  <- %s' % (s['name'], s['size'], ', '.join(refs) if refs else '(none)'))
    part_node = L.reach(roots, cut=ENTRY, cut_edges=[TAB])
    part_edge = L.reach(roots, cut_edges=[(DRAW, ENTRY), TAB])
    u_node = full - part_node
    u_edge = full - part_edge
    print('== freed set')
    for label, u in (('entry NODES cut', u_node), ('DrawForSlot->entry EDGES cut', u_edge)):
        heap, itcm, other, t = summary(L, u)
        print('  %-30s members %3d  heap %7d  itcm %5d  other %5d  total %7d' % (
            label, len(u), heap, itcm, other, heap + itcm + other))
    print('  sets equal:', u_node == u_edge)
    extra = u_node - u_edge
    if extra:
        print('  only freed when the nodes go (kept by another referrer today):')
        for i in sorted(extra, key=lambda i: -L.syms[i]['size'])[:20]:
            s = L.syms[i]
            print('    %6d %s <- %s' % (s['size'], s['name'], ', '.join(
                sorted(L.syms[c]['name'] for c in L.inn.get(i, ()))[:4])))
    # resident-table share
    u_entries_only = full - L.reach(roots, cut=ENTRY)
    tab_only = u_node - u_entries_only
    print('== freed by the entry edit alone: %d members, %d B; the rest is held alive only by the '
          'table-struct initialisers (needs the assets.c edit):' % (
              len(u_entries_only), sum(L.syms[i]['size'] for i in u_entries_only)))
    for i in sorted(tab_only, key=lambda i: -L.syms[i]['size']):
        s = L.syms[i]
        print('    %6d %s (%s)' % (s['size'], s['name'], s['sec']))
    print('    total %d' % sum(L.syms[i]['size'] for i in tab_only))
    t = tally(L, u_node)
    print('== freed set by (section, kind)')
    for k in sorted(t):
        print('    %-22s %-7s %7d' % (k[0], k[1], t[k]))
    if '--list' in a:
        n = int(a[a.index('--list') + 1])
        for i in sorted(u_node, key=lambda i: -L.syms[i]['size'])[:n]:
            s = L.syms[i]
            sf = src.get((s['name'], s['addr']), ('?', 0))
            print('  %6d %-58s %-9s %s:%s' % (s['size'], s['name'][:58], s['sec'], sf[0], sf[1]))
    if '--csv' in a:
        with open(a[a.index('--csv') + 1], 'w', newline='') as f:
            w = csv.writer(f)
            w.writerow(['name', 'size', 'section', 'kind', 'srcfile', 'line'])
            for i in sorted(u_node, key=lambda i: -L.syms[i]['size']):
                s = L.syms[i]
                sf = src.get((s['name'], s['addr']), ('?', 0))
                w.writerow([s['name'], s['size'], s['sec'], s['kind'], sf[0], sf[1]])


if __name__ == '__main__':
    main()
