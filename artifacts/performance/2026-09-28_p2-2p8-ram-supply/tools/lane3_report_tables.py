#!/usr/bin/env python3
"""lane3_report_tables.py -- every number in lane3-renderer-retirement.md, both ELFs.

Usage: lane3_report_tables.py <scratch-dir> <out-dir>
  <scratch-dir> holds  fp.graph.pkl fp.src.csv gate.graph.pkl gate.src.csv
  (built by lane3_refgraph.py build / lane3_srcmap.py from the two saved ELFs).
Writes into <out-dir>: lane3_tables.json, lane3_members_<unit>_<elf>.csv
and prints the family / unit table in the layout the report uses.

Units (sets of symbols that ONLY the named entry symbols keep alive; static upper bound):
  DIAG      ndsResetStartupDiagnostics (+ its word tables, Reset helpers, orphaned diagnostics BSS)
  OLD_ALL   ndsFighterMarioFoxDLAllDrawForSlot + reset-hook residue + dead canonical tables
  P0        the config-dead part of OLD_ALL: hierarchy mode (FastRunMode 7) + per-root executor
  P1        OLD_ALL - P0: the fallback-only part (plan / validate / production / packet record+replay)
Families (name predicates over every non-overlay symbol) are listed in FAMILIES below.
"""
import collections
import csv
import json
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import lane3_refgraph as rg  # noqa: E402

HEAP_SECS = {'.main', '.main.rw', '.main.bss', '.text.hot', '.text.hot.draw', '.text.frontend_resident'}

HOOKS = [
    (r'^ndsRendererNativeForgetFighterRunUvTables', r'^sNdsNativeFighterRunUvInputs$'),
    (r'^(ndsRelocPrepareSceneCache|ndsFighterIntroTransientReset|ndsFighterDisplayContractSubmit)', r'^sNdsFighterDrawPlan$'),
    (r'^ndsFighterRendererInvalidateMaterialCachesForSlot', r'^sNdsRendererAdapterNativeOwnerMaterialKeys$'),
    (r'^(ndsRendererFighterPacketInvalidateSlot|ndsRendererFighterPacketRelease|ndsRendererNativeApplyMaterial)', r'^sNdsFighterPacketRecorder$'),
    (r'^(ndsRendererFighterPacketInvalidateSlot|ndsRendererFighterPacketRelease|ndsFtrLeanMaterialize)', r'^sNdsFighterPackets$'),
    (r'^(sNdsNativeFighterHighTables|sNdsNativeFighterLowTables)', r'^sNdsNativeFighter(PackedCorners|RunFirstCorner)'),
]
HIER = r'^(ndsRendererExecuteNativeFighterOwnerHierarchy|ndsRendererAdapterPrepareNativeOwnerHierarchy|ndsRendererAdapterBuildNativeHierarchyInputs)'
PERROOT = r'^(ndsRendererBeginNativeFighterOwner|ndsRendererExecuteNativeFighterRoot|ndsRendererEndNativeFighterOwner|ndsRendererAbortNativeFighterOwner)'

FAMILIES = [
    # (id, title, name-regex, optional source-file regex)
    ('PKT', 'ndsFighterPacket* / ndsRendererFighterPacket* / sNdsFighterPacket* / gNdsFighterPacket*', r'FighterPacket', None),
    ('LEANPKT', 'ndsFtrLeanPacket* (the lean path itself)', r'^ndsFtrLeanPacket', None),
    ('PROD', '*Production* (native fighter production)', r'Production', None),
    ('OWNIMG', 'owner-image bind/ensure/hat-image + image slots', r'(OwnerImage|BindKirbyHatImage|NativeKirbyHat(Working|Active|Match))', None),
    ('OWNMAT', 'native-owner materials/workspace/validation/keys (renderer_adapter_matrix)', r'^sNdsRendererAdapterNativeOwner', None),
    ('TEXSCR', 'texture scratch / refresh / key pools', r'^sNdsRendererHardware(TextureScratch|TextureRefreshLarge|TextureRefreshSmall|TextureRefreshQueue|TextureIdentPool|TextureCache|TextureLookup|Ci4IndexCache|Texel01Ci4ClassTable|Texel01Ci4PairLut|StaticKeyPointers)|^sNdsRendererStaticTexturePalette', None),
    ('TASK36', '*Task36* remnants', r'Task36|task36|TASK36', None),
    ('STGSUB', 'ndsRendererAdapterSubmitStageDL (dispatch hub)', r'^ndsRendererAdapterSubmitStageDL$', None),
    ('STGEMIT', 'ndsRendererNativeStageEmit* (CPU stage vertex emission)', r'^ndsRendererNativeStageEmit', None),
    ('STGALL', 'all native-stage symbols (NativeStage*, ndsStageGx*, PrepareNativeStageOwner ...)', r'NativeStage|ndsStageGx|PrepareNativeStageOwner|CommitNativeStageSegment|StageOwner', None),
    ('MARIOFOX', 'ndsFighterMarioFox* (proof-era names)', r'^ndsFighterMarioFox', None),
    ('HALO', 'rebirth halo packets + submit', r'RebirthHalo', None),
    ('WHISPY', 'whispy packet + submit', r'Whispy', None),
    ('FRAMEBUF', 'gSYFramebufferSets', r'^gSYFramebufferSets$', None),
    ('LEAN', 'the lean path (ndsFtrLean*, sNdsFtrLean*, nds_ftr_lean_kernel, renderer_fighter_lean)', r'^(ndsFtrLean|sNdsFtrLean|gNdsFtrLean)', None),
    ('DIAGFILES', 'diagnostics_*.c globals (all)', None, r'diagnostics_'),
    ('ORACLE', 'oracle/shadow/witness/census/probe/proof named symbols', r'Oracle|oracle|Shadow|shadow|Witness|Census|census|Probe|probe|Proof|proof', None),
]


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
        sec = s['sec']
        kind = s['kind']
        key = 'heap_' + kind if sec in HEAP_SECS else ('itcm_text' if sec == '.itcm' else ('dtcm' if sec.startswith('.dtcm') else 'ovl_' + kind if sec.startswith('.ovl') else 'other_' + kind))
        t[key] += s['size']
    return t


def fmt(t):
    heap = sum(v for k, v in t.items() if k.startswith('heap_'))
    return {'text': t['heap_text'], 'rodata': t['heap_rodata'], 'data': t['heap_data'], 'bss': t['heap_bss'],
            'heap_total': heap, 'itcm': t['itcm_text'], 'dtcm': t['dtcm'], 'overlay': sum(v for k, v in t.items() if k.startswith('ovl_'))}


def write_members(L, src, idxs, path):
    with open(path, 'w', newline='') as f:
        w = csv.writer(f)
        w.writerow(['name', 'size', 'section', 'kind', 'srcfile', 'line'])
        for i in sorted(idxs, key=lambda i: -L.syms[i]['size']):
            s = L.syms[i]
            sf = src.get((s['name'], s['addr']), ('?', 0))
            w.writerow([s['name'], s['size'], s['sec'], s['kind'], sf[0], sf[1]])


def main():
    scratch, outdir = sys.argv[1], sys.argv[2]
    result = {}
    for elf in ('fp', 'gate'):
        L = rg.Loaded(os.path.join(scratch, elf + '.graph.pkl'))
        src = load_src(os.path.join(scratch, elf + '.src.csv'))
        roots = ['main', 'crt0Startup']
        full = L.reach(roots)
        edges = [(re.compile(a), re.compile(b)) for a, b in HOOKS]

        def unit(cut, edge_list=None):
            part = L.reach(roots, cut=re.compile(cut), cut_edges=edge_list)
            return full - part

        u_diag = unit(r'^ndsResetStartupDiagnostics$')
        u_old = unit(r'^ndsFighterMarioFoxDLAllDrawForSlot', edges)
                # P0 must also carry the tables only the P0 code reads; recompute against the same edge cuts
        tab_edge = [e for e in edges if e[1].pattern.startswith('^sNdsNativeFighter(PackedCorners')]
        u_p0 = full - L.reach(roots, cut=re.compile('(' + HIER + ')|(' + PERROOT + ')'), cut_edges=tab_edge)
        u_p0 &= u_old
        u_p1 = u_old - u_p0
        units = {'DIAG': u_diag, 'OLD_ALL': u_old, 'P0': u_p0, 'P1': u_p1}
        res = {'units': {}, 'families': {}}
        for name, s in units.items():
            res['units'][name] = dict(fmt(tally(L, s)), members=len(s))
            write_members(L, src, s, os.path.join(outdir, 'lane3_members_%s_%s.csv' % (name.lower(), elf)))
        for fid, title, nrx, frx in FAMILIES:
            nre = re.compile(nrx) if nrx else None
            fre = re.compile(frx) if frx else None
            mem = set()
            for i, s in enumerate(L.syms):
                if s['sec'].startswith('.ovl') and fid not in ('ORACLE',):
                    continue
                sf = src.get((s['name'], s['addr']), ('?', 0))[0]
                if nre is not None and not nre.search(s['name']):
                    continue
                if fre is not None and not fre.search(sf):
                    continue
                mem.add(i)
            inside = mem & u_old
            outside = mem - u_old
            res['families'][fid] = {'title': title, 'all': dict(fmt(tally(L, mem)), members=len(mem)),
                                    'in_old_unit': dict(fmt(tally(L, inside)), members=len(inside)),
                                    'outside_old_unit': dict(fmt(tally(L, outside)), members=len(outside))}
        result[elf] = res
    with open(os.path.join(outdir, 'lane3_tables.json'), 'w') as f:
        json.dump(result, f, indent=1)
    for elf in ('fp', 'gate'):
        print('#### ELF', elf)
        print('%-9s %8s %8s %8s %8s | %9s | %6s %6s' % ('unit', 'text', 'rodata', 'data', 'bss', 'heap_tot', 'itcm', 'members'))
        for name, r in result[elf]['units'].items():
            print('%-9s %8d %8d %8d %8d | %9d | %6d %6d' % (name, r['text'], r['rodata'], r['data'], r['bss'], r['heap_total'], r['itcm'], r['members']))
        print('%-9s %8s %8s %8s %8s | %9s | %s' % ('family', 'text', 'rodata', 'data', 'bss', 'heap_tot', 'in-old-unit heap / outside heap'))
        for fid, r in result[elf]['families'].items():
            a = r['all']
            print('%-9s %8d %8d %8d %8d | %9d | %d / %d' % (fid, a['text'], a['rodata'], a['data'], a['bss'], a['heap_total'], r['in_old_unit']['heap_total'], r['outside_old_unit']['heap_total']))


if __name__ == '__main__':
    main()
