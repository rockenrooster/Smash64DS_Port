#!/usr/bin/env python3
"""lane3_r1_total.py -- link-level bytes the complete R1 patch frees, step by step.

Usage: lane3_r1_total.py <graph.pkl>          (graph from lane3_relocgraph.py build)

Models the patch as cuts of edges leaving ndsFighterMarioFoxDLAllDrawForSlot and prints the
freed set after each step:
  1. the R1 entries (hierarchy executor, PrepareNativeOwnerHierarchy, Begin/End/Abort, the
     per-root wrapper) plus the corner-table initialiser edges         -> E1-E7 + A1
  2. the DL-draw visit callback pointer, only the per-root call took it -> E5 side effect
  3. the function-static persistent_renderer_vertices                   -> E8
The cut for step 2 and 3 was chosen from the active source of the patched file with the flag
at 0 (lane3_r1_pp.py grep): those identifiers no longer appear there.
"""
import collections
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import lane3_refgraph as rg  # noqa: E402

HEAP = {'.main', '.main.rw', '.main.bss', '.text.hot', '.text.hot.draw', '.text.frontend_resident'}
DRAW = re.compile(r'^ndsFighterMarioFoxDLAllDrawForSlot')
ENTRY = re.compile(r'^(ndsRendererExecuteNativeFighterOwnerHierarchy|ndsRendererAdapterPrepareNativeOwnerHierarchy|'
                   r'ndsRendererBeginNativeFighterOwner|ndsRendererExecuteNativeFighterRoot|'
                   r'ndsRendererEndNativeFighterOwner|ndsRendererAbortNativeFighterOwner)')
CALLBACK = re.compile(r'^ndsFighterMarioFoxVisitDLDrawCommand')
VERTEX_CACHE = re.compile(r'^persistent_renderer_vertices')
TAB = (re.compile(r'^(sNdsNativeFighterHighTables|sNdsNativeFighterLowTables)'),
       re.compile(r'^sNdsNativeFighter(PackedCorners|RunFirstCorner)'))


def main():
    L = rg.Loaded(sys.argv[1])
    roots = ['main', 'crt0Startup']
    full = L.reach(roots)

    def freed(edges):
        return full - L.reach(roots, cut_edges=edges)

    def size(u, secs=None):
        return sum(L.syms[i]['size'] for i in u if secs is None or L.syms[i]['sec'] in secs)

    steps = [
        ('E1-E7 + A1: R1 entries + corner tables', [(DRAW, ENTRY), TAB]),
        ('+ E5 side effect: DL-draw visit callback', [(DRAW, ENTRY), TAB, (DRAW, CALLBACK)]),
        ('+ E8: persistent_renderer_vertices', [(DRAW, ENTRY), TAB, (DRAW, CALLBACK), (DRAW, VERTEX_CACHE)]),
    ]
    prev = set()
    for label, edges in steps:
        u = freed(edges)
        add = u - prev
        print('%-44s total %7d B  (+%d%s)' % (
            label, size(u), size(add),
            ': ' + ', '.join(sorted(L.syms[i]['name'] for i in add))[:100] if prev else ''))
        prev = u
    t = collections.Counter()
    for i in prev:
        s = L.syms[i]
        t[(s['sec'], s['kind'])] += s['size']
    print('by (section, kind):', dict(t))
    print('heap-section bytes %d, other %d' % (size(prev, HEAP), size(prev) - size(prev, HEAP)))


if __name__ == '__main__':
    main()
