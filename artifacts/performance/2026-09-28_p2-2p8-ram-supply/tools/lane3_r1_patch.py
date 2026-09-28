#!/usr/bin/env python3
"""lane3_r1_patch.py -- write the R1-retirement patches (P2 only) as .patch files.

Usage: lane3_r1_patch.py <repo-root> <out-dir> <scratch-dir>

Reads HEAD blobs with `git show` (read-only), applies the edits below to copies held in
<scratch-dir>, and writes two unified diffs into <out-dir>:
  lane3_r1_p1safe_sources.patch   include/nds/nds_renderer.h, src/port/renderer_adapter_fighter.c,
                                  src/nds/nds_renderer_assets.c
  lane3_r1_p1safe_makefile.patch  Makefile  (kept apart: another agent edits the Makefile)
Nothing in the repository is modified.  Check with, from the repo root:
  git apply --check <patch>
Every anchor is asserted to match exactly once, so a drifted tree fails loudly.
The flag semantics: NDS_FIGHTER_LEGACY_EXEC=1 (default) compiles the tree exactly as before;
=0 (P2 targets) folds the R1 entries away so the linker drops them.
"""
import difflib
import os
import subprocess
import sys


def head(repo, path):
    out = subprocess.run(['git', '-C', repo, 'show', 'HEAD:' + path], check=True, capture_output=True)
    return out.stdout.decode('utf-8').replace('\r\n', '\n')


def once(text, old, new, label):
    n = text.count(old)
    if n != 1:
        raise SystemExit('anchor "%s" matched %d times' % (label, n))
    return text.replace(old, new, 1)


def lines_of(text, lo, hi):
    """1-based inclusive line range as one string (with trailing newline)."""
    ls = text.split('\n')
    return '\n'.join(ls[lo - 1:hi]) + '\n'


# --------------------------------------------------------------------------- header
HDR_ANCHOR = ('#if (NDS_RENDERER_BENCHMARK_MODE < NDS_RENDERER_BENCHMARK_NONE) || \\\n'
              '    (NDS_RENDERER_BENCHMARK_MODE > NDS_RENDERER_BENCHMARK_WARM_NO_UPLOAD)\n'
              '#error "NDS_RENDERER_BENCHMARK_MODE must be 0 through 4"\n'
              '#endif\n')
HDR_NEW = HDR_ANCHOR + '''
/* R1 retirement. NDS_FIGHTER_LEGACY_EXEC=1 keeps four fighter draw modes that
 * only lab tooling can still select: hierarchy mode 7, the per-root hardware
 * executor (modes 5-7) with the CPU triangle rasteriser behind it, and the
 * resident raw-path tables (PackedCorners, DenseCorners, RunFirstCorner and the
 * joint schedules). A build that draws fighters through the production owner
 * (fast-run mode 8 or 9, no oracle, Task 56 primitive streams, HW light) never
 * reaches them, so the P2 targets set 0 and the linker drops the code. Every
 * other target and the host tools keep the default 1 and compile exactly as
 * before. The guard turns a flag combination that would need one of the retired
 * modes into a build error instead of a fighter with no executor. */
#ifndef NDS_FIGHTER_LEGACY_EXEC
#define NDS_FIGHTER_LEGACY_EXEC 1
#endif

#if (NDS_FIGHTER_LEGACY_EXEC != 0) && (NDS_FIGHTER_LEGACY_EXEC != 1)
#error "NDS_FIGHTER_LEGACY_EXEC must be 0 or 1"
#endif

#if !NDS_FIGHTER_LEGACY_EXEC && \\
    (!NDS_RENDERER_HW_TRIANGLES || (NDS_RENDERER_PROFILE_LEVEL != 0) || \\
     !NDS_R2_FIGHTER_NO_ORACLE || !NDS_R2_FIGHTER_HW_LIGHT || \\
     (NDS_TASK56_FIGHTER_PRIMITIVES < 1) || NDS_R2_STRIP_ROUTE || \\
     NDS_RENDERER_SCREEN_SPACE_CENSUS || \\
     (NDS_RENDERER_BENCHMARK_MODE != NDS_RENDERER_BENCHMARK_NONE) || \\
     (NDS_RENDERER_FAST_RUN_DEFAULT < \\
      NDS_RENDERER_FAST_RUN_NATIVE_FIGHTER_OWNER_PRODUCTION))
#error "NDS_FIGHTER_LEGACY_EXEC=0 needs HW triangles, profile 0, NO_ORACLE, HW light, Task 56 streams (no strip route or census) and fast-run mode 8 or 9"
#endif
'''


def edit_header(t):
    return once(t, HDR_ANCHOR, HDR_NEW, 'nds_renderer.h benchmark validation')


# --------------------------------------------------------------------------- fighter adapter
def edit_fighter(t):
    # E1a: native_owner_started exists only with the per-root path
    t = once(t, '    sb32 native_owner_started = FALSE;\n',
             '#if NDS_FIGHTER_LEGACY_EXEC\n    sb32 native_owner_started = FALSE;\n#endif\n', 'E1a started decl')
    # E1b: hierarchy mode becomes a constant
    t = once(t, '    sb32 native_owner_hierarchy_mode;\n',
             '#if NDS_FIGHTER_LEGACY_EXEC\n'
             '    sb32 native_owner_hierarchy_mode;\n'
             '#else\n'
             '    /* R1 retired: hierarchy mode 7 is not compiled, every test of it folds. */\n'
             '    const sb32 native_owner_hierarchy_mode = FALSE;\n'
             '#endif\n', 'E1b hierarchy decl')
    # E2: which fast-run modes admit a native owner
    old = ('    native_owner_enabled =\n'
           '        (((gNdsRendererFastRunMode ==\n'
           '           NDS_RENDERER_FAST_RUN_NATIVE_MARIO) && (owner_slot == 0u)) ||\n'
           '         ((gNdsRendererFastRunMode ==\n'
           '           NDS_RENDERER_FAST_RUN_NATIVE_FOX) && (owner_slot == 1u)) ||\n'
           '          (gNdsRendererFastRunMode ==\n'
           '           NDS_RENDERER_FAST_RUN_NATIVE_FIGHTERS) ||\n'
           '          (gNdsRendererFastRunMode ==\n'
           '           NDS_RENDERER_FAST_RUN_NATIVE_FIGHTER_OWNER_PRODUCTION) ||\n'
           '           (gNdsRendererFastRunMode ==\n'
           '            NDS_RENDERER_FAST_RUN_NATIVE_COMPLETE_STAGE)) ? TRUE :\n'
           '                                                                     FALSE;\n')
    new = ('#if NDS_FIGHTER_LEGACY_EXEC\n' + old +
           '#else\n'
           '    /* R1 retired: only the production modes admit a native owner. A poked\n'
           '     * mode 5-7 is not native and takes the fail-closed reject below. */\n'
           '    native_owner_enabled =\n'
           '        ((gNdsRendererFastRunMode ==\n'
           '          NDS_RENDERER_FAST_RUN_NATIVE_FIGHTER_OWNER_PRODUCTION) ||\n'
           '         (gNdsRendererFastRunMode ==\n'
           '          NDS_RENDERER_FAST_RUN_NATIVE_COMPLETE_STAGE)) ? TRUE : FALSE;\n'
           '#endif\n')
    t = once(t, old, new, 'E2 native_owner_enabled')
    # E3: hierarchy-mode assignment
    old = ('    native_owner_hierarchy_mode =\n'
           '        (gNdsRendererFastRunMode ==\n'
           '         NDS_RENDERER_FAST_RUN_NATIVE_FIGHTERS) ? TRUE : FALSE;\n')
    t = once(t, old, '#if NDS_FIGHTER_LEGACY_EXEC\n' + old + '#endif\n', 'E3 hierarchy assign')
    # E4: per-root Begin block
    old = ('    if ((native_owner_enabled != FALSE) &&\n'
           '        (native_owner_production_attempted == FALSE))\n'
           '    {\n'
           '        if (ndsRendererBeginNativeFighterOwner(\n')
    i = t.index(old)
    j = t.index('#endif\n', t.index('nNDSTickHudNativeOwnerFallbackBegin', i))   # the TICK_HUD #endif
    k = t.index('    }\n#endif\n', j)                                              # end of Begin block + outer #endif
    blk_end = k + len('    }\n')
    t = t[:i] + '#if NDS_FIGHTER_LEGACY_EXEC\n' + t[i:blk_end] + '#endif\n' + t[blk_end:]
    # E5: per-root branch of the draw loop
    old_start = '        else if (native_owner_started != FALSE)\n        {\n            if ((native_root_enabled == FALSE) ||\n'
    i = t.index(old_start)
    old_tail = ('                native_owner_started = FALSE;\n'
                '                native_owner_failed = TRUE;\n'
                '            }\n'
                '        }\n')
    j = t.index(old_tail, i) + len(old_tail)
    t = t[:i] + '#if NDS_FIGHTER_LEGACY_EXEC\n' + t[i:j] + '#endif\n' + t[j:]
    # E6: End block
    old = ('    if (native_owner_started != FALSE)\n'
           '    {\n'
           '        if (ndsRendererEndNativeFighterOwner(\n')
    i = t.index(old)
    old_tail = '        native_owner_started = FALSE;\n    }\n'
    j = t.index(old_tail, i) + len(old_tail)
    t = t[:i] + '#if NDS_FIGHTER_LEGACY_EXEC\n' + t[i:j] + '#endif\n' + t[j:]
    # E7: the two locals only the per-root call read (keeps -Wunused-but-set-variable quiet)
    old = ('#if NDS_RENDERER_PROFILE_LEVEL < 2\n'
           '        if (native_root_enabled != FALSE)\n'
           '        {\n'
           '            native_materials =\n'
           '                sNdsRendererAdapterNativeOwnerMaterials[\n'
           '                    sNdsRendererAdapterNativeOwnerMaterialRows[i]];\n'
           '            native_material_count = native_owner_material_counts[i];\n'
           '        }\n'
           '#endif\n')
    new = ('#if NDS_RENDERER_PROFILE_LEVEL < 2\n'
           '#if NDS_FIGHTER_LEGACY_EXEC\n'
           '        if (native_root_enabled != FALSE)\n'
           '        {\n'
           '            native_materials =\n'
           '                sNdsRendererAdapterNativeOwnerMaterials[\n'
           '                    sNdsRendererAdapterNativeOwnerMaterialRows[i]];\n'
           '            native_material_count = native_owner_material_counts[i];\n'
           '        }\n'
           '#else\n'
           '        /* R1 retired: the per-root executor that read these is not compiled. */\n'
           '        (void)native_materials;\n'
           '        (void)native_material_count;\n'
           '#endif\n'
           '#endif\n')
    t = once(t, old, new, 'E7 material locals')
    # E8: the static vertex cache only Begin / the per-root call / End used
    t = once(t, '    static NDSRendererVertexCache persistent_renderer_vertices;\n',
             '#if NDS_FIGHTER_LEGACY_EXEC\n'
             '    static NDSRendererVertexCache persistent_renderer_vertices;\n'
             '#endif\n', 'E8a vertex cache decl')
    t = once(t, '    ndsRendererInitVertexCache(&persistent_renderer_vertices);\n',
             '#if NDS_FIGHTER_LEGACY_EXEC\n'
             '    ndsRendererInitVertexCache(&persistent_renderer_vertices);\n'
             '#endif\n', 'E8b vertex cache init')
    return t


# --------------------------------------------------------------------------- resident tables
def edit_assets(t):
    old_hi = ('    sNdsNativeFighterPackedCorners, NDS_FTR_COUNT(sNdsNativeFighterPackedCorners),\n'
              '    sNdsNativeFighterRunFirstCorner,\n'
              '    NDS_FTR_COUNT(sNdsNativeFighterRunFirstCorner),\n')
    new_hi = ('#if NDS_FIGHTER_LEGACY_EXEC\n' + old_hi +
              '#else\n'
              '    /* R1 retired: no raw-path reader is compiled, so these stay unlinked. */\n'
              '    NULL, 0u, NULL, 0u,\n'
              '#endif\n')
    t = once(t, old_hi, new_hi, 'assets High corners')
    old_lo = ('    sNdsNativeFighterPackedCornersLow,\n'
              '    NDS_FTR_COUNT(sNdsNativeFighterPackedCornersLow),\n'
              '    sNdsNativeFighterRunFirstCornerLow,\n'
              '    NDS_FTR_COUNT(sNdsNativeFighterRunFirstCornerLow),\n')
    new_lo = ('#if NDS_FIGHTER_LEGACY_EXEC\n' + old_lo +
              '#else\n'
              '    NULL, 0u, NULL, 0u,\n'
              '#endif\n')
    return once(t, old_lo, new_lo, 'assets Low corners')


# --------------------------------------------------------------------------- Makefile
def edit_makefile(t):
    anchor = 'NDS_RENDERER_FAST_RUN_DEFAULT ?= $(if $(filter smash64ds-battle-playable-coarse-hwtri,$(TARGET)),8,0)\n'
    new = anchor + (
        '# R1 retirement, P2 targets only. 0 drops fighter hierarchy mode 7, the per-root hardware\n'
        '# executor, the CPU triangle rasteriser and the resident raw-path tables; the P1 family and\n'
        '# every other name keep 1 and build exactly as before. nds_renderer.h refuses 0 outside the\n'
        '# production fighter configuration (profile 0, mode 8/9, NO_ORACLE, HW light, Task 56 streams).\n'
        'NDS_FIGHTER_LEGACY_EXEC ?= $(if $(filter smash64ds smash64ds-p2-shell-hwtri '
        'smash64ds-p2-shell-freeplay-hwtri smash64ds-p2-shell-loop-hwtri '
        'smash64ds-p2-1b-scene-walk-hwtri smash64ds-p2-fourcpu-tickhud-hwtri,$(TARGET)),0,1)\n'
        'ifneq ($(filter $(NDS_FIGHTER_LEGACY_EXEC),0 1),)\n'
        'else\n'
        '$(error NDS_FIGHTER_LEGACY_EXEC must be 0 or 1)\n'
        'endif\n')
    t = once(t, anchor, new, 'Makefile default')
    anchor = "\t\techo '#define NDS_R2_FIGHTER_NO_ORACLE $(NDS_R2_FIGHTER_NO_ORACLE)'; \\\n"
    t = once(t, anchor, anchor + "\t\techo '#define NDS_FIGHTER_LEGACY_EXEC $(NDS_FIGHTER_LEGACY_EXEC)'; \\\n",
             'Makefile config echo')
    anchor = "\t@printf '%s\\n' 'BENCH_MAKE_FAST_RUN_DEFAULT=$(NDS_RENDERER_FAST_RUN_DEFAULT)'\n"
    t = once(t, anchor, anchor + "\t@printf '%s\\n' 'BENCH_MAKE_FIGHTER_LEGACY_EXEC=$(NDS_FIGHTER_LEGACY_EXEC)'\n",
             'Makefile benchmark flags')
    return t


def write_diff(repo, scratch, out_path, files):
    chunks = []
    for path, fn in files:
        old = head(repo, path)
        new = fn(old)
        dst = os.path.join(scratch, path.replace('/', '__'))
        with open(dst, 'w', newline='\n') as f:
            f.write(new)
        d = list(difflib.unified_diff(old.split('\n'), new.split('\n'),
                                      'a/' + path, 'b/' + path, lineterm='', n=3))
        chunks.append('\n'.join(d) + '\n')
        print('%-40s +%d lines' % (path, sum(1 for l in d if l.startswith('+') and not l.startswith('+++'))))
    with open(out_path, 'w', newline='\n') as f:
        f.write(''.join(chunks))


def main():
    repo, out, scratch = sys.argv[1], sys.argv[2], sys.argv[3]
    os.makedirs(scratch, exist_ok=True)
    write_diff(repo, scratch, os.path.join(out, 'lane3_r1_p1safe_sources.patch'), [
        ('include/nds/nds_renderer.h', edit_header),
        ('src/port/renderer_adapter_fighter.c', edit_fighter),
        ('src/nds/nds_renderer_assets.c', edit_assets),
    ])
    write_diff(repo, scratch, os.path.join(out, 'lane3_r1_p1safe_makefile.patch'), [
        ('Makefile', edit_makefile),
    ])


if __name__ == '__main__':
    main()
