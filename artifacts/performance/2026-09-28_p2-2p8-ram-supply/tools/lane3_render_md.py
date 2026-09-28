#!/usr/bin/env python3
"""lane3_render_md.py -- Markdown rows for the report from lane3_tables.json.

Usage: lane3_render_md.py <lane3_tables.json>
Prints (1) the unit table and (2) the family table, FP and GATE side by side, each cell
'text / rodata / data / bss' in bytes plus the heap total (main-RAM sections only; ITCM and
DTCM shown separately because they do not shrink the battle heap).
"""
import json
import sys


def cell(x):
    return '%s / %s / %s / %s' % tuple('{:,}'.format(x[k]) for k in ('text', 'rodata', 'data', 'bss'))


def tot(x):
    return '{:,}'.format(x['heap_total'])


def main():
    d = json.load(open(sys.argv[1]))
    print('| unit | members | FP text / rodata / data / bss | FP heap B | GATE text / rodata / data / bss | GATE heap B |')
    print('|---|---:|---|---:|---|---:|')
    for u in ('P0', 'P1', 'OLD_ALL', 'DIAG'):
        a, b = d['fp']['units'][u], d['gate']['units'][u]
        print('| %s | %d / %d | %s | %s | %s | %s |' % (u, a['members'], b['members'], cell(a), tot(a), cell(b), tot(b)))
    print()
    rows = [
        ('PKT', 'in_old_unit', 'ndsFighterPacket* recorder side (in old unit)'),
        ('PKT', 'outside_old_unit', 'ndsFighterPacket* shared with lean'),
        ('PROD', 'in_old_unit', '*Production* (in old unit)'),
        ('PROD', 'outside_old_unit', '*Production* shared with lean'),
        ('OWNIMG', 'all', 'owner-image bind/ensure/hat image'),
        ('OWNMAT', 'in_old_unit', 'native-owner material state (old only)'),
        ('OWNMAT', 'outside_old_unit', 'native-owner materials/workspace (lean uses)'),
        ('TEXSCR', 'all', 'texture scratch / refresh / key pools'),
        ('TASK36', 'all', '*Task36* remnants'),
        ('STGSUB', 'all', 'ndsRendererAdapterSubmitStageDL'),
        ('STGEMIT', 'all', 'ndsRendererNativeStageEmit*'),
        ('STGALL', 'all', 'all native-stage symbols'),
        ('MARIOFOX', 'in_old_unit', 'ndsFighterMarioFox* (in old unit)'),
        ('MARIOFOX', 'outside_old_unit', 'ndsFighterMarioFox* (rest)'),
        ('HALO', 'all', 'rebirth halo packets + submit'),
        ('WHISPY', 'all', 'whispy packet + submit'),
        ('FRAMEBUF', 'all', 'gSYFramebufferSets'),
        ('LEAN', 'all', 'lean path (context)'),
        ('DIAGFILES', 'all', 'diagnostics_*.c globals'),
        ('ORACLE', 'all', 'oracle/shadow/witness/census/probe/proof names'),
    ]
    print('| family | FP text / rodata / data / bss | FP heap B | GATE text / rodata / data / bss | GATE heap B | FP ITCM / DTCM |')
    print('|---|---|---:|---|---:|---|')
    for fid, part, title in rows:
        a = d['fp']['families'][fid][part]
        b = d['gate']['families'][fid][part]
        print('| %s | %s | %s | %s | %s | %s / %s |' % (title, cell(a), tot(a), cell(b), tot(b), '{:,}'.format(a['itcm']), '{:,}'.format(a['dtcm'])))


if __name__ == '__main__':
    main()
