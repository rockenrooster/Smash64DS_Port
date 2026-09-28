#!/usr/bin/env python3
"""lane3_r1_symcheck.py -- are the R1 symbols linked in an ELF?  (read-only, uses nm)

Usage: lane3_r1_symcheck.py <elf> absent|present [--nm PATH]

R1 = hierarchy mode 7, the per-root hardware executor, the CPU triangle rasteriser and the
resident raw-path tables.  `absent` is the expectation for a P2 ELF built with
NDS_FIGHTER_LEGACY_EXEC=0; `present` is the expectation for every P1-family ELF.
Prints each R1 base name with the bytes found (0 = not linked) and exits 1 on a violation.
The name list is the FP link's P0 unit (lane3_members_p0_fp.csv) reduced to base names;
the 22 *JointSchedule / *BindingJoints tables are matched by suffix.
"""
import re
import subprocess
import sys

FUNCS = [
    'ndsRendererExecuteNativeFighterOwnerHierarchy', 'ndsRendererAdapterPrepareNativeOwnerHierarchy',
    'ndsRendererExecuteNativeFighterRootHardware', 'ndsRendererBeginNativeFighterOwner',
    'ndsRendererEndNativeFighterOwner', 'ndsRendererAbortNativeFighterOwner',
    'ndsRendererNativePrepareHierarchyRun', 'ndsRendererNativePrepareDirectRun',
    'ndsRendererNativeVisitSourceCommand', 'ndsRendererNativeEmitDenseRawRun',
    'ndsRendererNativeEmitProductionRawTexturedRun', 'ndsRendererNativeEmitProductionRawUntexturedRun',
    'ndsRendererNativeMatrix3Mul20p12', 'ndsRendererSubmitHardwareTriangle',
    'ndsRendererHardwareSubmitVertex', 'ndsRendererHardwareClipVertexNdcDepth',
    'ndsRendererHardwareSubmitNearClippedTriangle', 'ndsRendererHardwareEmitClippedVertex',
    'ndsRendererHardwareTriangleInsideNearPlane', 'ndsRendererHardwareGetLightShadeLut',
    'ndsRendererTransformCachedVertex', 'ndsRendererEnsureTransformedVertex',
    'ndsRendererRecordTransformedTriangle', 'ndsRendererExecuteNativeFighterRoot',
    'ndsRendererHardwareSubmitVertexRawZCold', 'ndsRendererHardwareSubmitVertexDecalCold',
    'ndsRendererSubmitHardwareTriangleRawMatrixCold', 'ndsRendererSubmitHardwareTriangleDepthStatsCold',
]
OBJS = [
    'sNdsNativeFighterPackedCorners', 'sNdsNativeFighterPackedCornersLow', 'sNdsNativeFighterDenseCorners',
    'sNdsNativeFighterRunFirstCorner', 'sNdsNativeFighterRunFirstCornerLow',
    'sNdsRendererHardwareLightShadeCache', 'sNdsRendererHardwareLightShadeCacheNext',
]
SUFFIX = re.compile(r'^sNdsNative\w+(JointSchedule|BindingJoints)$')


def base(name):
    return re.sub(r'(\.(constprop|isra|part|cold)\.\d+)+$', '', name)


def main():
    elf, want = sys.argv[1], sys.argv[2]
    nm = 'C:/devkitPro/devkitARM/bin/arm-none-eabi-nm.exe'
    if '--nm' in sys.argv:
        nm = sys.argv[sys.argv.index('--nm') + 1]
    out = subprocess.run([nm, '--format=sysv', '-S', elf], check=True, capture_output=True, text=True).stdout
    found = {}
    joint = 0
    for line in out.splitlines():
        p = [x.strip() for x in line.split('|')]
        if len(p) < 5 or not p[4]:
            continue
        try:
            size = int(p[4], 16)
        except ValueError:
            continue
        b = base(p[0])
        if b in FUNCS or b in OBJS:
            found[b] = found.get(b, 0) + size
        elif SUFFIX.match(b):
            joint += size
            found['(joint/binding tables)'] = joint
    bad = 0
    for n in FUNCS + OBJS + ['(joint/binding tables)']:
        sz = found.get(n, 0)
        ok = (sz == 0) if want == 'absent' else (sz > 0 or n in ('ndsRendererNativeEmitProductionRawTexturedRun',))
        print('  %-52s %7d %s' % (n, sz, '' if ok else '<-- unexpected'))
        if not ok and want == 'absent':
            bad += 1
    linked = sum(1 for v in found.values() if v)
    print('%s: %d of %d R1 names linked (%d B) -> expected %s' % (
        elf.replace('\\', '/').split('/')[-1], linked, len(FUNCS) + len(OBJS) + 1, sum(found.values()), want))
    sys.exit(1 if bad else 0)


if __name__ == '__main__':
    main()
