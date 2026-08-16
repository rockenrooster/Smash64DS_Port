"""The FTR lane rolled into groups, and its stall classes totalled.

Runs the same call-flow model as attribute.py (see its docstring for the model
and its one assumption) and then answers the two questions a ranking cannot:

  1. Which STALL CLASS owns the lane?  A lane that is mostly instruction fetch
     has a placement lever; one that is mostly bus contention has a "submit less"
     lever and that is a fidelity decision, not an engineering one.
  2. What does each PHASE of the fighter draw cost?  The tree is
     Submit -> {capture pass, camera, playback}, and the playback splits again
     into the per-joint matrix build and the native production emit.

ITCM CANDIDATES are ranked at the end by icache_fill ticks per byte, because
ITCM is the scarce resource: ../2026-08-16_itcm-census/ITCM_CENSUS.md puts the
recoverable pool at 688 B by eviction (+54), on top of 220 B free on the
instrument.
"""
import csv
import subprocess
import sys

D = 'artifacts/performance/2026-08-16_ftr-attribution/'
MASK = sys.argv[1] if len(sys.argv) > 1 else 'm80gate'
NFR = {'whole': 1601, 'm80prof': 80, 'm80gate': 80}[MASK]
STALLS = ['issue', 'icache_fill', 'dcache_fill', 'write_buffer',
          'bus_contention', 'interlock']

# Re-use attribute.py's model verbatim rather than re-implementing it.
sys.argv = ['attribute.py', MASK]
ns = {'__name__': '__main__', '__file__': D + 'attribute.py'}
import io
import contextlib
buf = io.StringIO()
with contextlib.redirect_stdout(buf):
    exec(compile(open(D + 'attribute.py').read(), D + 'attribute.py', 'exec'),
         ns)
f, Neff, selfcyc, sym, size, addr = (ns['f'], ns['Neff'], ns['selfcyc'],
                                     ns['sym'], ns['size'], ns['addr'])
I, r_self, out = ns['I'], ns['r_self'], ns['out']
FR = float(NFR)


def tk_self(s):
    return f[s] * (selfcyc[s] / Neff[s]) / (2 * FR) if Neff.get(s) else 0.0


def tk_incl(s):
    return f[s] * (1 - r_self[s]) * I[s] / (2 * FR) if Neff.get(s) else 0.0


def stall(s, k):
    """Ticks of stall class k inside FTR: the symbol's stall share of its own
    cycles, times the share of it FTR owns."""
    c = selfcyc[s]
    if not c:
        return 0.0
    return tk_self(s) * (sym.get(s, {}).get('%s_%s' % (MASK, k), 0) / c)


live = [s for s in addr if Neff.get(s) and f[s] > 0.0]
tot = sum(tk_self(s) for s in live)
print('FTR LANE, mask %s, %d frames.   TOTAL %.0f tk/fr' % (MASK, NFR, tot))
print('  the instrument\'s own figure for this lane: FTR band 41-120 = 290,400,'
      ' P50 290,432')
print('')
print('=== STALL CLASSES OF THE WHOLE LANE ===')
print('"issue" is the profiler\'s RESIDUAL class and can read slightly negative'
      ' on a\nsymbol that retires under one cycle per instruction; it is not a'
      ' stall you can delete.')
ins = sum(sym.get(s, {}).get('%s_instructions' % MASK, 0) *
          (f[s] / Neff[s]) for s in live)
print('  %-16s %10s %7s' % ('class', 'tk/fr', 'share'))
print('  %-16s %10.0f %6.1f%%' % ('instructions', ins / (2 * FR),
                                  100.0 * ins / (2 * FR) / tot))
for k in STALLS:
    v = sum(stall(s, k) for s in live)
    print('  %-16s %10.0f %6.1f%%' % (k, v, 100.0 * v / tot))

GROUPS = [
    ('CAPTURE pass  (walk the DObj tree, record the draw event list)',
     ['ndsBaseFTDisplayMainProcDisplay', 'ftDisplayMainDrawAll',
      'ftDisplayMainDrawDefault', 'ftDisplayMainDecideFogDraw',
      'gcPrepDObjMatrix', 'gcDrawMObjForDObj', 'gcParseMObjMatAnimJoint',
      'ndsFighterDisplayContractSelectDL', 'ndsFighterDisplayContractCountFlags',
      'ndsBaseGcPlayMObjMatAnim', 'ftDisplayMainDrawShadow']),
    ('PER-JOINT MATRIX build (local -> XObj -> affine compose)',
     ['ndsRendererAdapterBuildDObjLocalMatrix',
      'ndsRendererAdapterBuildDObjXObjMatrix',
      'ndsRendererAdapterBuildFighterTraRotRpyDirect20p12',
      'ndsRendererAdapterBuildFighterTraRotRpyExact',
      'ndsRendererMtxMulAffine20p12', 'ndsRendererMtxLoadN64ToDS20p12',
      'syMatrixTraRotRpyRSca', 'syMatrixTraRotRpyR',
      'ndsRendererLoadHardwareSplitMatrices', 'ndsRendererMtxMul20p12']),
    ('NATIVE PRODUCTION emit (build and push the GX command stream)',
     ['ndsRendererNativeEmitProductionPrimitiveGroups',
      'ndsRendererNativePrepareProductionRun',
      'ndsRendererNativeApplyStateDelta',
      'ndsRendererNativeShadeProductionActions.constprop.0.isra.0',
      'ndsRendererNativeShadeProductionActions.constprop.0',
      'ndsRendererNativeApplyProductionPreamble',
      'ndsRendererNativeApplyRootLightPreamble.isra.0',
      'ndsRendererNativeEmitProductionCrossRun.constprop.0',
      'ndsRendererHardwareEndBatch.part.0',
      'ndsRendererApplyMatrixMoveWordCommand.part.0',
      'ndsRendererRecordSetTile', 'ndsRendererR2WriteLightVector']),
    ('MATERIAL and TEXTURE',
     ['ndsRendererNativeApplyMaterial.part.0',
      'ndsRendererAdapterMaterialAnimHash', 'ndsRendererSyncTextureTile',
      'ndsRendererR2RunTextureMemoApply.constprop.0',
      'ndsRendererHardwareBindTextureName', 'glBindTexture',
      'ndsRendererHardwareBindNoTexture.constprop.0',
      'ndsRendererCaptureTextureLoad.part.0', 'ndsRendererR2MaterialColor15',
      'ndsRelocFindLoadedFileContaining']),
    ('CAMERA (per-fighter look-at and perspective)',
     ['gmCameraLookAtFuncMatrix', 'ndsR2CameraLookAtReflect20p12',
      'ndsR2CameraPerspFast20p12', 'ndsR2CamDiv64',
      'ndsRendererAdapterGetFrameCameraMatrices.constprop.0',
      'ndsRendererAdapterBuildCameraMatrices.constprop.0']),
    ('MEMORY movers', ['memcpy', 'memset', 'memmove', 'armCopyMem32']),
    ('SOFT FLOAT leaves',
     ['__aeabi_fadd', '__aeabi_fmul', '__mulsf3', '__divsf3', '__aeabi_fdiv',
      '__aeabi_fcmpeq', '__aeabi_fcmple', '__aeabi_fcmpge', '__aeabi_fcmplt',
      '__aeabi_fcmpgt', '__aeabi_f2iz', '__aeabi_ui2f', '__floatsisf',
      '__aeabi_l2f', '__aeabi_i2f', '__subsf3', '__addsf3', '__aeabi_frsub']),
]
print('')
print('=== THE LANE BY PHASE (self ticks, each symbol counted once) ===')
seen = set()
G = '%-62s %10s %8s'
print(G % ('group', 'tk/fr', 'share'))
for name, members in GROUPS:
    v = 0.0
    for m in members:
        if m in addr and m not in seen and f.get(m, 0) > 0:
            seen.add(m)
            v += tk_self(m)
    print(G % (name, '%.0f' % v, '%.1f%%' % (100.0 * v / tot)))
rest = sum(tk_self(s) for s in live if s not in seen)
print(G % ('everything else in the span', '%.0f' % rest,
           '%.1f%%' % (100.0 * rest / tot)))
print(G % ('TOTAL', '%.0f' % tot, '100.0%'))
print('')
print('  (of which the two top-level branches, by INCLUSIVE cost:')
for s in ('ndsBaseFTDisplayMainProcDisplay', 'gmCameraLookAtFuncMatrix',
          'ndsFighterMarioFoxDLAllDrawForSlot.constprop.0',
          'ndsRendererExecuteNativeFighterOwnerProduction'):
    print('     %-56s %8.0f tk/fr' % (s, tk_incl(s)))
print('   the first three are siblings under the root; the fourth is inside the'
      ' third)')

print('')
print('=== INSTRUCTION-FETCH OWNERS: what a placement lever could reach ===')
print('%9s %9s %7s %9s  %-46s' % ('icache', 'self t/f', 'bytes', 'ic/byte',
                                  'symbol'))
ic = sorted(((stall(s, 'icache_fill'), s) for s in live), reverse=True)
tot_ic = sum(v for v, _ in ic)
for v, s in ic[:18]:
    b = size.get(s, 0)
    print('%9.0f %9.0f %7d %9.2f  %-46s'
          % (v, tk_self(s), b, (v / b) if b else 0, s[:46]))
print('%9.0f  <= FTR instruction fetch, total' % tot_ic)
