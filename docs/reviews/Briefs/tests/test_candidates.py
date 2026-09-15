#!/usr/bin/env python3
"""Host-only tests of candidate fragments. Not an ARM/ROM or original-asset test."""
from __future__ import annotations
import ast
import json
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
DATA = json.loads((ROOT / 'research/candidate_preimages.json').read_text())
CANDS = {p['id'].split('_', 1)[0]: p for p in DATA['candidates']}


def frag(key, index, which='after'):
    return CANDS[key]['fragments'][index][which]


def compile_run(source: str, defines=(), expected=0):
    compiler = shutil.which('gcc') or shutil.which('clang')
    if not compiler:
        raise unittest.SkipTest('Host C compiler unavailable; compile/run tests not executed')
    with tempfile.TemporaryDirectory() as tmp:
        path = Path(tmp)
        (path / 'test.c').write_text(source)
        exe = path / 'test.exe'
        cmd = [compiler, '-std=c99', '-Wall', '-Wextra', '-Werror', '-Wno-unused-function',
               '-Wno-unused-const-variable', *['-D' + d for d in defines],
               str(path / 'test.c'), '-o', str(exe)]
        proc = subprocess.run(cmd, capture_output=True, text=True)
        if proc.returncode:
            raise AssertionError(proc.stderr)
        proc = subprocess.run([str(exe)], capture_output=True, text=True)
        if proc.returncode != expected:
            raise AssertionError(f'Expected exit {expected}, got {proc.returncode}: {proc.stderr}')


LINK_PREFIX = r'''
#include <stddef.h>
#include <stdint.h>
typedef uint32_t u32;
typedef struct { int marker; } NDSNativeRoot;
typedef struct { int marker; } NDSNativeFighterRuntimeTables;
typedef struct {
    const NDSNativeFighterRuntimeTables *tables;
    u32 root_light_preamble_count;
    const u32 (*root_light_preambles)[2];
} NDSNativeFighterOwnerRuntime;
#define NDS_FTR_COUNT(a) (sizeof(a)/sizeof((a)[0]))
static const NDSNativeFighterRuntimeTables bodyHigh={1}, bodyLow={2};
static const NDSNativeFighterRuntimeTables sNdsNativeLinkBoomerangFighterHighTables={3};
static const NDSNativeFighterRuntimeTables sNdsNativeLinkBoomerangFighterLowTables={4};
static const u32 bodyLights[1][2]={{11,12}};
static const u32 sNdsNativeLinkBoomerangRootLightPreambles[2][2]={{21,22},{23,24}};
static const u32 sNdsNativeLinkSpecialNSourceOwners[3]={0,2,9};
static const NDSNativeFighterOwnerRuntime sNdsNativeLinkSpecialNHighOwner={&bodyHigh,1,bodyLights};
static const NDSNativeFighterOwnerRuntime sNdsNativeLinkSpecialNLowOwner={&bodyLow,1,bodyLights};
static const NDSNativeFighterOwnerRuntime otherOwner={&bodyHigh,1,bodyLights};
'''
LINK_MAIN = r'''
int main(void) {
    u32 count=99;
    const NDSNativeFighterRuntimeTables *t;
    const u32 (*lights)[2];
    t=ndsRendererNativeFighterTablesForResolvedRoot(NULL,&sNdsNativeLinkSpecialNHighOwner,0,1);
#if NDS_P2_LINK && defined(NDS_NATIVE_LINK_ROOT_PROGRAMS_PRESENT)
    if(t!=&sNdsNativeLinkBoomerangFighterHighTables) return 11;
    t=ndsRendererNativeFighterTablesForResolvedRoot(NULL,&sNdsNativeLinkSpecialNLowOwner,0,1);
    if(t!=&sNdsNativeLinkBoomerangFighterLowTables) return 12;
    if(ndsRendererNativeFighterTablesForResolvedRoot(NULL,&sNdsNativeLinkSpecialNHighOwner,0,2)!=NULL) return 13;
    if(ndsRendererNativeFighterTablesForResolvedRoot(NULL,&sNdsNativeLinkSpecialNHighOwner,0,3)!=NULL) return 14;
    lights=ndsRendererNativeFighterLightPreamblesForResolvedRoot(NULL,&sNdsNativeLinkSpecialNHighOwner,0,1,&count);
    if(lights!=sNdsNativeLinkBoomerangRootLightPreambles || count!=2) return 15;
    lights=ndsRendererNativeFighterLightPreamblesForResolvedRoot(NULL,&sNdsNativeLinkSpecialNHighOwner,0,2,&count);
    if(lights!=NULL || count!=0) return 16;
    count=99;
    lights=ndsRendererNativeFighterLightPreamblesForResolvedRoot(NULL,&sNdsNativeLinkSpecialNHighOwner,0,3,&count);
    if(lights!=NULL || count!=0) return 17;
#else
    if(t!=&bodyHigh) return 18;
#endif
    if(ndsRendererNativeFighterTablesForResolvedRoot(NULL,&otherOwner,0,999)!=&bodyHigh) return 19;
    if(ndsRendererNativeFighterTablesForResolvedRoot(NULL,&sNdsNativeLinkSpecialNLowOwner,0,0)!=&bodyLow) return 20;
    if(ndsRendererNativeFighterTablesForResolvedRoot(NULL,NULL,0,0)!=NULL) return 21;
    count=99;
    lights=ndsRendererNativeFighterLightPreamblesForResolvedRoot(NULL,NULL,0,0,&count);
    if(lights!=NULL || count!=0) return 22;
    lights=ndsRendererNativeFighterLightPreamblesForResolvedRoot(NULL,&otherOwner,0,1,&count);
    if(lights!=bodyLights || count!=1) return 23;
    return 0;
}
'''


class PatchTests(unittest.TestCase):
    def test_patch_apply_and_reverse_on_reconstructed_preimages(self):
        # Filler is deliberate: these are fragment fixtures, NOT the checkout.
        for item in DATA['candidates']:
            with self.subTest(candidate=item['id']), tempfile.TemporaryDirectory() as tmp:
                p = Path(tmp)
                subprocess.run(['git', 'init', '-q', str(p)], check=True)
                bypath = {}
                for f in item['fragments']:
                    bypath.setdefault(f['path'], []).append(f)
                originals = {}
                for name, fragments in bypath.items():
                    lines = []
                    for f in sorted(fragments, key=lambda f: f['line_hint']):
                        needed = f['line_hint'] - 1 - len(lines)
                        self.assertGreaterEqual(needed, 0, f'{name} overlapping fragment hints')
                        lines.extend('// fixture padding\n' for _ in range(needed))
                        lines.extend(f['before'].splitlines(True))
                    lines.extend('// fixture tail\n' for _ in range(10))
                    target=p/name
                    target.parent.mkdir(parents=True, exist_ok=True)
                    # git-apply compares patch context byte-for-byte.  Keep the
                    # reconstructed source in the patch's LF form on Windows;
                    # Path.write_text's platform newline translation otherwise
                    # turns every candidate fixture into a false mismatch.
                    target.write_text(''.join(lines), newline='\n')
                    originals[name]=target.read_bytes()
                patch=str(ROOT/item['path'])
                for args in (['--check','--whitespace=error'], ['--whitespace=error'],
                             ['--reverse','--check','--whitespace=error'], ['--reverse','--whitespace=error']):
                    proc=subprocess.run(['git','-C',str(p),'apply',*args,patch], capture_output=True,text=True)
                    self.assertEqual(proc.returncode,0,proc.stderr)
                for name, expected in originals.items():
                    self.assertEqual((p/name).read_bytes(),expected)

    def test_impact_no_painter_counter_in_replacement(self):
        self.assertNotIn('NextProjectedDepth',frag('R01',3))
        self.assertIn('projected_z[index]',frag('R01',3))
        self.assertIn('ndsRendererHardwareSourceDepthToV16(',frag('R01',1))
        self.assertIn('(s64)out->z * NDS_RENDERER_HW_PROJECTED_VERTEX, out->w',frag('R01',1))

    def test_impact_xy_unchanged(self):
        for axis in ('x','y'):
            expected=f'projected_{axis}[i] = ndsRendererHardwareProjectToV16(\n            (s64)out->{axis} * NDS_RENDERER_HW_PROJECTED_VERTEX, out->w);'
            self.assertIn(expected,frag('R01',1,'before'))
            self.assertIn(expected,frag('R01',1))

    def test_effect_z_is_scoped_to_both_effect_callers(self):
        self.assertEqual(sum(f['after'].count('SubmitEffectDObjTree') for f in CANDS['R02']['fragments']),2)
        for f in CANDS['R02']['fragments']:
            self.assertIn('SubmitEffectDObjTree',f['after'])
            self.assertIn('NDS_RENDERER_GEOM_ZBUFFER',f['after'])
            self.assertNotIn('SubmitWeapon',f['after'])

    def test_reset_release_precedes_all_three_resets(self):
        for f in CANDS['R03']['fragments']:
            self.assertLess(f['after'].index('ReleaseOwnerImagesInRange'),f['after'].index('syMallocReset'))
            self.assertEqual(f['after'].count('syMallocReset'),1)

    def test_link_baseline_reproduces_wrong_foreign_table(self):
        compile_run(LINK_PREFIX+frag('R04',0,'before')+frag('R04',1,'before')+LINK_MAIN,
                    ['NDS_P2_LINK=1','NDS_NATIVE_LINK_ROOT_PROGRAMS_PRESENT=1'],expected=11)

    def test_link_candidate_routes_high_low_and_bounds(self):
        compile_run(LINK_PREFIX+frag('R04',0)+frag('R04',1)+LINK_MAIN,
                    ['NDS_P2_LINK=1','NDS_NATIVE_LINK_ROOT_PROGRAMS_PRESENT=1'])

    def test_link_disabled_build(self):
        compile_run(LINK_PREFIX+frag('R04',0)+frag('R04',1)+LINK_MAIN,['NDS_P2_LINK=0'])

    def test_link_programs_absent_build(self):
        compile_run(LINK_PREFIX+frag('R04',0)+frag('R04',1)+LINK_MAIN,['NDS_P2_LINK=1'])

    def test_data_generator_ast_and_mocked_layout(self):
        source=frag('R05',0).split('def main(',1)[0]
        ast.parse(source)
        class Record:
            def __init__(self,*args,**kwargs): self.args=args; self.kw=kwargs
        scope={'Placement':Record,'SurfaceSpec':Record,'COLLAGE_FULL_BLEED':object(),
               'OPTION_TAB_HI':((130,0,40),(255,0,40)),
               'OPTION_TAB_NOT':((0,0,0),(130,130,170)),'MENU_FIELD':object()}
        exec(compile(source,'DATA_surface_fragment','exec'),scope)
        specs=scope['DATA_SURFACE_SPECS']
        self.assertEqual(len(specs),11)
        self.assertEqual(len({s.args[0] for s in specs}),11)
        self.assertEqual(specs[0].args[0],'DATA')
        positions={'LOCKED_CHARACTERS':(113,57,26),'LOCKED_VS_RECORD':(81,126,27),
                   'UNLOCKED_CHARACTERS':(133,42,26),'UNLOCKED_VS_RECORD':(101,89,27),
                   'UNLOCKED_SOUND_TEST':(69,136,26)}
        for s in specs[1:]:
            key=s.args[0][5:]
            if key.endswith('_HI'): key=key[:-3]
            x,y,dx=positions[key]
            self.assertEqual(s.kw['box'],(x,y,164,29))
            parts=s.args[1]
            self.assertEqual(parts[1].kw['tile'],(128,29))
            self.assertEqual(parts[2].args[2:4],(x+144,y))
            self.assertEqual(parts[3].args[2:4],(x+dx,y+4))

    def test_data_ids_append_after_character_data(self):
        text=frag('R05',1)
        self.assertLess(text.index('CHARACTERS_SURFACE_SPECS'),text.index('DATA_SURFACE_SPECS'))
        self.assertIn('DATA_SURFACE_SPECS',frag('R05',2))

    def test_data_refresh_native_api_and_retry(self):
        # Assemble only exact replacement globals and refresh plus a source-equivalent
        # Last helper. Mock blitter checks C control flow, not hardware pixels.
        import re
        globals_=frag('R05',4)
        refresh=frag('R05',5)
        tokens=sorted(set(re.findall(r'NDS_MN_UI_KIT_SURFACE_\w+',globals_)))
        prefix='''#include <stdint.h>\n#include <stddef.h>\ntypedef uint32_t u32;\ntypedef uint16_t NdsUiKitSurfaceId;\n#define TRUE 1u\n#define FALSE 0u\n#define NDS_MENU_DATA_ROWS 3u\n'''
        prefix+='enum {'+','.join(tokens)+'};\n'
        prefix+='''static u32 attempts,successes,fail_at;\nstatic NdsUiKitSurfaceId recorded[64];\nstatic u32 ndsUiKitBlitSurfaces(const NdsUiKitSurfaceId *s,u32 n) {\n    attempts++; if(n!=1u) return FALSE; if(attempts==fail_at) return FALSE;\n    recorded[successes++]=*s; return TRUE; }\n'''
        helper='static u32 ndsMenuShellDataLast(void) {return sMenuDataHaveSoundTest ? 2u : 1u;}\n'
        main=r'''
static void reset(u32 unlocked) {
    u32 i; sMenuDataHaveSoundTest=unlocked; sMenuDataCursor=0;
    sMenuDataPlateReady=FALSE; attempts=successes=fail_at=0;
    for(i=0;i<3;i++) sMenuDataRowSurface[i]=NDS_MENU_DATA_SURFACE_NONE;
}
int main(void) {
    reset(0); ndsMenuShellDataRefresh();
    if(successes!=3 || recorded[0]!=NDS_MN_UI_KIT_SURFACE_DATA) return 1;
    if(recorded[1]!=NDS_MN_UI_KIT_SURFACE_DATA_LOCKED_CHARACTERS_HI) return 2;
    ndsMenuShellDataRefresh(); if(successes!=3) return 3;
    sMenuDataCursor=1; ndsMenuShellDataRefresh(); if(successes!=5) return 4;
    reset(1); ndsMenuShellDataRefresh(); if(successes!=4) return 5;
    if(recorded[3]!=NDS_MN_UI_KIT_SURFACE_DATA_UNLOCKED_SOUND_TEST) return 6;
    reset(1); fail_at=1; ndsMenuShellDataRefresh();
    if(sMenuDataPlateReady || successes!=0) return 7;
    fail_at=0; ndsMenuShellDataRefresh(); if(successes!=4) return 8;
    reset(1); fail_at=3; ndsMenuShellDataRefresh();
    if(successes!=2 || sMenuDataRowSurface[1]!=NDS_MENU_DATA_SURFACE_NONE) return 9;
    fail_at=0; ndsMenuShellDataRefresh(); if(successes!=4) return 10;
    if(attempts!=5) return 11;
    ndsMenuShellDataRefresh(); if(attempts!=5) return 12;
    reset(0); ndsMenuShellDataRefresh(); if(successes!=3) return 13;
    return 0;
}
'''
        compile_run(prefix+globals_+helper+refresh+main)


if __name__=='__main__':
    unittest.main(verbosity=2)
