"""Decode generated GX and compare every submitted corner to native source tables."""
from dataclasses import replace
import itertools
from pathlib import Path
import random
import shutil
import struct
import subprocess
import sys
import tempfile

import pytest

import compile_nds_stage_gx as gx


def test_compiled_corners_patch_coverage_and_stack():
    packet = gx.stage.generate(gx.stage._paths.REPO_ROOT, 'dreamland')
    blob = gx.compile_packet(packet)
    header, runs, patches, words = gx.decode(blob)
    assert header[6] == 0xFF and header[7] == gx.signature(packet)
    static_mask = header[10] | (header[11] << 32)
    attributes = gx.vertex_attributes(packet)
    triangles = 0
    for index, record in enumerate(runs):
        first, count, pf, pc, nt, segment = record[:6]
        if not count:
            continue
        assert segment in gx.SEGMENTS
        run = packet.runs[index]
        assert nt == run.triangle_count
        cross = bool(run.flags & gx.stage.RUN_FLAG_PROJECTED_CROSS_MATRIX)
        scope = patches[pf:pf + pc]
        is_noz = run.submit_class == gx.stage.SUBMIT_PROJECTED_NO_Z
        composed_noz = is_noz and (cross or not static_mask & (1 << run.binding_index))
        assert sum(p[1] == gx.VIEW for p in scope) == (0 if composed_noz else 1)
        assert sum(p[1] in (gx.NOZ, gx.COMPOSED_NOZ) for p in scope) == (nt if is_noz and not cross else 0)
        assert sum(p[1] == gx.CORNER_NOZ for p in scope) == (nt*3 if cross else 0)
        if cross:
            assert [p[3] >> 3 for p in scope if p[1] == gx.CORNER_NOZ] == [t for t in range(nt) for _ in range(3)]
        assert sum(p[1] == gx.PROJECTION for p in scope) == (0 if is_noz else 1)
        assert bool(sum(p[1] == gx.WORLD for p in scope)) == (not composed_noz and not static_mask & (1 << run.binding_index))
        indices = packet.corners[run.first_corner:run.first_corner + nt*3]
        assert record[12] | (record[13] << 32) == sum(1 << b for b in {packet.vertices[i].matrix_binding for i in indices})
        assert sum(p[1] == gx.COLOR for p in scope) == sum(attributes[i][0] is None for i in indices)
        assert sum(p[1] == gx.UV for p in scope) == sum(attributes[i][1] is None for i in indices)
        assert scope[0] == (first + 1, gx.MATERIAL, index, 0)
        assert words[first] == 0x002B2A29
        assert all(first <= p[0] and p[0] + (3 if p[1] == gx.MATERIAL else 1 if p[1] in (gx.COLOR, gx.UV) else 16) <= first + count for p in scope)
        actual, depth = [], 0
        for op, args in gx.commands(words[first:first + count]):
            if op == 0x11:
                depth += 1
            elif op == 0x12:
                depth -= args[0]
                assert depth >= 0
            elif op in (0x20, 0x22):
                baked = attributes[indices[len(actual)]][0 if op == 0x20 else 1]
                assert args[0] == (0 if baked is None else baked)
            elif op == 0x23:
                assert depth == 1
                actual.append(tuple(struct.unpack('<h', struct.pack('<H', n))[0]
                                    for n in (args[0] & 65535, args[0] >> 16, args[1] & 65535)))
        expected = []
        for t in range(nt):
            vertices = [packet.vertices[i] for i in packet.corners[run.first_corner + 3*t:run.first_corner + 3*t + 3]]
            shift = max(gx.stage.stage_vertex_coordinate_shift(v) for v in vertices)
            if run.submit_class == gx.stage.SUBMIT_RAW_CURRENT:
                shift = 0
            elif run.submit_class == gx.stage.SUBMIT_PROJECTED_RANGE_OR_MATRIX:
                shift = max(gx.stage.stage_vertex_coordinate_shift(packet.vertices[i]) for i in
                            packet.corners[run.first_corner:run.first_corner + nt*3])
            for v in vertices:
                vertex_shift = gx.stage.stage_vertex_coordinate_shift(v) if cross and len({v.matrix_binding for v in vertices}) != 1 else shift
                assert all(record[6+a] <= getattr(v, axis) <= record[9+a] for a, axis in enumerate(('x', 'y', 'z')))
                # Independent nearest/away-from-zero arithmetic, not the compiler helper.
                expected.append(tuple(((-1 if n < 0 else 1) * ((abs(n) + (1 << (vertex_shift-1))) >> vertex_shift) if vertex_shift else n) * 16
                                      for n in (v.x, v.y, v.z)))
        assert actual == expected and depth == 0
        triangles += nt
    assert triangles == 202
    damaged = bytearray(blob)
    damaged[-1] ^= 1
    with pytest.raises(ValueError):
        gx.decode(damaged)
    bad = list(packet.runs)
    first = packet.segments[4].first_run  # cross-binding source-Z is not admitted
    bad[first] = replace(bad[first], flags=bad[first].flags | gx.stage.RUN_FLAG_PROJECTED_CROSS_MATRIX)
    with pytest.raises(ValueError):
        gx.compile_packet(replace(packet, runs=tuple(bad)))


def test_live_near_bounds_are_conservative():
    sys.path.insert(0, str(gx.stage._paths.REPO_ROOT / 'scripts/menus'))
    from source_test_helpers import function
    source = (gx.stage._paths.REPO_ROOT / 'src/nds/nds_stage_gx.exec.inc').read_text()
    code = r'''
#include <stdint.h>
#include <limits.h>
#include <stdio.h>
#include <nds/nds_stage_gx.h>
typedef int32_t s32; typedef int64_t s64; typedef uint32_t u32; typedef int sb32;
typedef struct { s32 m[4][4]; } NDSRendererMatrix20p12;
static s32 ndsRendererClampS64ToS32(s64 n) {
    return n < INT32_MIN ? INT32_MIN : n > INT32_MAX ? INT32_MAX : (s32)n;
}
''' + function(source, 'ndsStageGxInsideNear') + r'''
int main(void) {
    NDSStageGxRun r = {0}; NDSRendererMatrix20p12 m;
    while (scanf("%hd", &r.minimum[0]) == 1) {
        for (int i=1; i<3; i++) if (scanf("%hd", &r.minimum[i]) != 1) return 2;
        for (int i=0; i<3; i++) if (scanf("%hd", &r.maximum[i]) != 1) return 2;
        for (int i=0; i<16; i++) if (scanf("%d", &m.m[i/4][i%4]) != 1) return 2;
        printf("%d\n", ndsStageGxInsideNear(&r, &m));
    }
}
'''
    rng = random.Random(6404)
    cases, inside = [], []
    clamp = lambda n: max(-(1 << 31), min((1 << 31) - 1, n))
    for i in range(256):
        lo = [rng.randint(-32768, 30000) for _ in range(3)]
        hi = [rng.randint(n, 32767) for n in lo]
        m = [0] * 16
        limit = 8 if i < 64 else 0x7FFFFFFF
        for axis in range(3):
            m[axis*4+2] = rng.randint(-limit, limit)
            m[axis*4+3] = rng.randint(-limit, limit)
        m[14] = 1000000 if i < 64 else rng.randint(-0x80000000, 0x7FFFFFFF)
        m[15] = 1000000 if i < 64 else rng.randint(-0x80000000, 0x7FFFFFFF)
        if i == 64:  # exactly on the eye plane
            m[3] = m[7] = m[11] = m[15] = 0
        points = itertools.product(*zip(lo, hi))
        inside.append(all((w := clamp(sum(m[a*4+3]*v[a] for a in range(3)) + m[15])) > 0 and
                          clamp(sum(m[a*4+2]*v[a] for a in range(3)) + m[14]) + w >= 0
                          for v in points))
        cases.append(' '.join(map(str, lo + hi + m)))
    with tempfile.TemporaryDirectory() as directory:
        path = Path(directory) / 'near.c'
        binary = path.with_suffix('.exe')
        path.write_text(code)
        built = subprocess.run([shutil.which('gcc'), '-std=c11', '-O2', '-I',
                                str(gx.stage._paths.REPO_ROOT / 'include'), str(path), '-o', str(binary)],
                               capture_output=True, text=True)
        assert built.returncode == 0, built.stderr
        run = subprocess.run([str(binary)], input='\n'.join(cases), capture_output=True, text=True)
        assert run.returncode == 0, run.stderr
        actual = list(map(int, run.stdout.split()))
    assert len(actual) == len(cases) and sum(actual) >= 64
    assert actual[64] == 0
    assert all(not admit or expected for admit, expected in zip(actual, inside))


def test_attribute_decisions_match_renderer():
    sys.path.insert(0, str(gx.stage._paths.REPO_ROOT / 'scripts/menus'))
    from source_test_helpers import function
    source = (gx.stage._paths.REPO_ROOT / 'src/nds/nds_renderer_textures_effects.c').read_text()
    names = ('ndsRendererCombineUsesColor', 'ndsRendererCombineOutputUsesColor',
             'ndsRendererCombineSecondOutputUsesColor', 'ndsRendererHardwareUseSecondCycle',
             'ndsRendererHardwareSecondCyclePassesCombined', 'ndsRendererHardwareOutputUsesColor',
             'ndsRendererHardwareUsesLitPrimitiveModulate', 'ndsRendererHardwareLitShadeCombine',
             'ndsRendererHardwareUseMaterialColor', 'ndsRendererHardwareUseVertexColor',
             'ndsRendererCombineUsesAlpha', 'ndsRendererCombineSecondOutputUsesAlpha',
             'ndsRendererHardwareOutputUsesAlpha')
    code = r'''
#include <stdint.h>
#include <stdio.h>
typedef uint32_t u32; typedef int s32;
typedef struct { u32 texture_combine_w0, texture_combine_w1, othermode_h,
    geometry_mode, env_color, texture_combine_count; } NDSRendererStats;
#define TRUE 1
#define FALSE 0
#define NDS_RENDERER_CYCLETYPE_MASK (3u << 20)
#define NDS_RENDERER_CYC_2CYCLE (1u << 20)
#define NDS_RENDERER_GEOM_LIGHTING 0x20000u
#define NDS_RENDERER_CCMUX_COMBINED 0u
#define NDS_RENDERER_CCMUX_PRIMITIVE 3u
#define NDS_RENDERER_CCMUX_SHADE 4u
#define NDS_RENDERER_CCMUX_ENVIRONMENT 5u
#define NDS_RENDERER_CCMUX_ZERO_AB 15u
#define NDS_RENDERER_CCMUX_ZERO_D 7u
#define NDS_RENDERER_ACMUX_COMBINED 0u
''' + '\n'.join(function(source, name) for name in names) + r'''
int main(void) {
    NDSRendererStats s;
    while (scanf("%u %u %u %u %u", &s.texture_combine_w0, &s.texture_combine_w1,
                 &s.othermode_h, &s.geometry_mode, &s.env_color) == 5) {
        s.texture_combine_count = (s.texture_combine_w0 || s.texture_combine_w1) ? 1 : 0;
        int color = ndsRendererHardwareUseMaterialColor(&s) ? -1 : ndsRendererHardwareUseVertexColor(&s);
        int texture = ndsRendererCombineUsesColor(s.texture_combine_w0, s.texture_combine_w1, 1) ||
                      ndsRendererHardwareOutputUsesAlpha(&s, 1);
        printf("%d %d\n", color, texture);
    }
}
'''
    packet = gx.stage.generate(gx.stage._paths.REPO_ROOT, 'dreamland')
    policies = list(packet.policies)
    rng = random.Random(6405)
    policies += [replace(policies[0], combine_w0=rng.getrandbits(32), combine_w1=rng.getrandbits(32),
                         othermode_h=rng.getrandbits(32), geometry_mode=rng.getrandbits(32)) for _ in range(256)]
    rows = [(p, env) for p in policies for env in (0, 0xFFFFFFFF)]
    inputs = '\n'.join(f'{p.combine_w0} {p.combine_w1} {p.othermode_h} {p.geometry_mode} {env}' for p, env in rows)
    with tempfile.TemporaryDirectory() as directory:
        path = Path(directory) / 'attributes.c'
        binary = path.with_suffix('.exe')
        path.write_text(code)
        built = subprocess.run([shutil.which('gcc'), '-std=c11', '-O2', str(path), '-o', str(binary)],
                               capture_output=True, text=True)
        assert built.returncode == 0, built.stderr
        run = subprocess.run([str(binary)], input=inputs, capture_output=True, text=True)
        assert run.returncode == 0, run.stderr
    actual = [tuple(map(int, line.split())) for line in run.stdout.splitlines()]
    assert len(actual) == len(rows)
    rgba = 0x4FA2CBFF
    packed = ((rgba >> 27) & 31) | (((rgba >> 19) & 31) << 5) | (((rgba >> 11) & 31) << 10)
    for (policy, _), (color_mode, texture) in zip(rows, actual):
        baked = gx.vertex_colour(policy, rgba)
        if baked is not None:
            assert color_mode != -1
            assert baked == (packed if color_mode else 0x7FFF)
        assert gx.texture_used(policy) == bool(texture)


def test_material_words_and_submission_boundaries():
    sys.path.insert(0, str(gx.stage._paths.REPO_ROOT / 'scripts/menus'))
    from source_test_helpers import function
    source = (gx.stage._paths.REPO_ROOT / 'src/nds/nds_stage_gx.exec.inc').read_text()
    code = r'''
#include <stdint.h>
#include <assert.h>
#include <stddef.h>
#include <nds/nds_stage_gx.h>
typedef uint32_t u32; typedef int sb32;
#define FALSE 0
#define TRUE 1
#define NDS_RENDERER_TEXTURE_PARAM_MUTABLE_MASK 0xc00f0000u
#define NDS_RENDERER_HW_SUBMIT_PROJECTED_NO_Z 3u
#define NDS_RENDERER_NATIVE_STAGE_STATIC_OWNER_COUNT 4u
#define POLY_CULL_NONE 0xc0u
#define POLY_CULL_BACK 0x80u
#define NDS_RENDERER_GX_STATE_ALL 7u
#define GL_TEXTURE_2D 1
#define GL_ALPHA_TEST 2
#define GL_FOG 4
#define DMA_FIFO 0x80000000u
typedef struct { void **data; u32 cur_size; } DynamicArray;
typedef struct { u32 texFormat; int palIndex; } gl_texture_data;
typedef struct { uint16_t addr; } gl_palette_data;
static struct { DynamicArray texturePtrs, palettePtrs; } glGlobalData;
typedef struct { u32 poly_fmt, textured, texture_name, texture_params, alpha_test, alpha_ref; } NDSNativeStagePreparedRun;
static u32 storage[100], *sNdsStageGxWords = storage;
static u32 sNdsStageGxFirstWord, sNdsStageGxWordCount, sNdsStageGxAlpha;
static u32 sNdsFighterPacketDmaPending, gNdsP2StageProgDmas;
static u32 sNdsRendererHardwareBoundTextureName, sNdsRendererHardwareMatrixLoaded, sNdsRendererHardwareTriangleBatchOpen;
static void *sNdsRendererHardwareActiveTextureEntry, *sNdsR2GxLastProjection;
static u32 regs[3], GFX_FIFO, spans[8][2], waits, alphas;
#define DMA_SRC(i) regs[0]
#define DMA_DEST(i) regs[1]
#define DMA_CR(i) regs[2]
#define NDS_FIGHTER_PACKET_DMA_WAIT() do { if (sNdsFighterPacketDmaPending) waits++; sNdsFighterPacketDmaPending=0; } while (0)
static void DC_FlushRange(const u32 *p, u32 bytes) {
    spans[gNdsP2StageProgDmas][0] = (u32)(p-storage);
    spans[gNdsP2StageProgDmas][1] = bytes/4;
}
static void ndsRendererHardwareInvalidateGXState(u32 mask) { assert(mask==7); }
static void ndsRendererNativeStageTask36EndSegment(void) { assert(!sNdsFighterPacketDmaPending); }
static void ndsRendererHardwareEndBatch(void) { assert(!sNdsFighterPacketDmaPending); }
static void glEnable(u32 mode) { assert(!sNdsFighterPacketDmaPending); }
static void glDisable(u32 mode) { assert(!sNdsFighterPacketDmaPending); }
static void glAlphaFunc(u32 ref) { assert(!sNdsFighterPacketDmaPending); alphas++; }
''' + '\n'.join(function(source, name) for name in ('ndsStageGxMaterial', 'ndsStageGxFlush', 'ndsStageGxAppend')) + r'''
int main(void) {
    gl_texture_data texture = {0x12345678u,1}; gl_palette_data palette = {0x321};
    void *textures[] = {0,&texture,0}, *palettes[] = {0,&palette,0};
    glGlobalData.texturePtrs = (DynamicArray){textures,3};
    glGlobalData.palettePtrs = (DynamicArray){palettes,3};
    NDSNativeStagePreparedRun p = {0xc0,1,1,0xc00a0000u,1,3}; u32 w[3];
    assert(ndsStageGxMaterial(w,&p,3,0) && w[0]==0x80 && w[1]==0xd23a5678 && w[2]==0x321);
    assert(ndsStageGxMaterial(w,&p,3,4) && w[0]==0xc0); /* Actors stay two-sided. */
    assert(ndsStageGxMaterial(w,&p,0,0) && w[0]==0xc0); /* Source Z keeps its cull state. */
    texture.palIndex=2; assert(!ndsStageGxMaterial(w,&p,3,0));
    texture.palIndex=-1; assert(!ndsStageGxMaterial(w,&p,3,0));
    texture.palIndex=0; assert(ndsStageGxMaterial(w,&p,3,0) && w[2]==0);
    p.texture_name=3; assert(!ndsStageGxMaterial(w,&p,3,0));
    p.texture_name=2; assert(!ndsStageGxMaterial(w,&p,3,0));
    p.textured=0; assert(ndsStageGxMaterial(w,&p,3,0) && w[1]==0 && w[2]==0);
    NDSStageGxRun a = {.first_word=0,.word_count=10};
    NDSStageGxRun b = {.first_word=10,.word_count=20};
    ndsStageGxAppend(&a,&p); ndsStageGxAppend(&b,&p);
    assert(gNdsP2StageProgDmas==0 && sNdsStageGxWordCount==30);
    a.first_word=40; /* A hidden run leaves a gap: do not submit its words. */
    ndsStageGxAppend(&a,&p);
    assert(gNdsP2StageProgDmas==1 && spans[0][0]==0 && spans[0][1]==30 && waits==1);
    a.first_word=50; p.alpha_ref=7; ndsStageGxAppend(&a,&p);
    assert(gNdsP2StageProgDmas==2 && spans[1][0]==40 && spans[1][1]==10 && waits==2);
    ndsStageGxFlush(); /* Segment/cold-clip boundary must drain the queued span. */
    assert(gNdsP2StageProgDmas==3 && spans[2][0]==50 && spans[2][1]==10);
    assert(sNdsFighterPacketDmaPending && !sNdsStageGxWordCount && alphas==3);
    ndsStageGxFlush(); assert(gNdsP2StageProgDmas==3);
}
'''
    with tempfile.TemporaryDirectory() as directory:
        path = Path(directory) / 'stage_submit.c'
        binary = path.with_suffix('.exe')
        path.write_text(code)
        built = subprocess.run([shutil.which('gcc'), '-std=c11', '-O2',
                                '-I', str(gx.stage._paths.REPO_ROOT / 'include'), str(path), '-o', str(binary)],
                               capture_output=True, text=True)
        assert built.returncode == 0, built.stderr
        run = subprocess.run([str(binary)], capture_output=True, text=True)
        assert run.returncode == 0, run.stderr


def test_cross_binding_preserves_source_depth_mode():
    sys.path.insert(0, str(gx.stage._paths.REPO_ROOT / 'scripts/menus'))
    from source_test_helpers import function, braced
    source = (gx.stage._paths.REPO_ROOT / 'src/nds/nds_renderer_native_owners.c').read_text()
    loop = braced(function(source, 'ndsRendererCommitNativeStageSegment'), r'^\s*for \(triangle_offset = 0u;')
    code = r'''
#include <stdint.h>
#include <assert.h>
typedef uint32_t u32; typedef int16_t s16;
typedef struct { u32 flags, submit_class, triangle_count, first_corner; } Run;
typedef struct { int dummy; } NDSNativeStageDenseVertex;
typedef struct { int dummy; } NDSNativeStagePreparedDense;
#define NDS_RENDERER_HW_SUBMIT_PROJECTED_NO_Z 3u
#define NDS_NATIVE_STAGE_RUN_FLAG_PROJECTED_CROSS_MATRIX 1u
static u32 sNdsNativeStageCorners[3] = {0,1,2};
static NDSNativeStageDenseVertex sNdsNativeStageVertices[3];
static NDSNativeStagePreparedDense sNdsNativeStagePreparedDense[3];
static int noz, cross, foreground, vertices, depths;
static s16 ndsRendererHardwareNextProjectedDepth(void) { depths++; return 123; }
static void ndsRendererHardwareEnterProjectedForeground(void) { foreground++; }
static int ndsRendererNativeStageEmitCrossMatrixTriangle(const Run *r, const void *p, u32 t) { cross++; return 1; }
static int ndsRendererNativeStageEmitNoZTriangle(const Run *r, const void *p, u32 t, s16 z) { assert(z==123); noz++; return 1; }
static void ndsRendererNativeStageEmitVertex(const void *d, const void *p, const void *r, u32 c) { vertices++; }
static void check(u32 flags, u32 submit, int want_noz, int want_cross, int want_foreground, int want_vertices) {
    Run value = {flags,submit,1,0}; const Run *run = &value;
    const void *prepared_run = 0; u32 triangle_offset, emitted_triangles = 0;
    noz = cross = foreground = vertices = depths = 0;
''' + loop + r'''
    assert(noz==want_noz && cross==want_cross && foreground==want_foreground && vertices==want_vertices);
    assert(depths==want_noz && emitted_triangles==1);
}
int main(void) {
    check(1,3,1,0,0,0); /* Dream Land: cross binding still has no source Z. */
    check(1,6,0,1,1,0); /* Inishie strings keep source-Z crossing. */
    check(1,0,0,1,1,0);
    check(0,3,1,0,0,0);
    check(0,0,0,0,1,3);
}
'''
    with tempfile.TemporaryDirectory() as directory:
        path = Path(directory) / 'cross_depth.c'
        binary = path.with_suffix('.exe')
        path.write_text(code)
        built = subprocess.run([shutil.which('gcc'), '-std=c11', '-O2', str(path), '-o', str(binary)], capture_output=True, text=True)
        assert built.returncode == 0, built.stderr
        run = subprocess.run([str(binary)], capture_output=True, text=True)
        assert run.returncode == 0, run.stderr
