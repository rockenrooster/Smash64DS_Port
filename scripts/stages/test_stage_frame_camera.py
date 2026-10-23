"""Execute the production kind-48 branch with shared and per-draw cameras.

The branch builds the billboard rows in fixed point: the camera's Mod1 (the
yaw-free eye/target relation, ndsRendererAdapterMvpMod1Q) and the rows from
it, the perspective and the binding's recalc scales
(ndsRendererAdapterMvpKind48RowsQ). A frame's shared camera builds its Mod1
once and, as the memo's owner, the rows once per distinct pair of scales;
every draw must still get exactly the rows a private draw builds.
"""
from pathlib import Path
import re
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts/menus'))
from source_test_helpers import function, braced  # noqa: E402
from source_test_helpers import source as read_split  # noqa: E402


def _host(fn):
    """The ARM-state attribute means nothing to a host compiler."""
    return fn.replace('target("arm")', '').replace('__attribute__((noinline, ))',
                                                   '__attribute__((noinline))')


def test_shared_camera_operands_preserve_draw_results(tmp_path):
    source = read_split(ROOT / 'src/port/renderer_adapter_matrix.c')
    apply = function(source, 'ndsRendererAdapterApplyMvpRecalc')
    billboard = braced(apply, r'^\s*else if \(kind == nGCMatrixKind48\)')
    billboard = billboard.replace('else if', 'if', 1)
    mod1_type = re.search(r'typedef struct NDSRendererAdapterMvpMod1Q\s*\{.*?\} '
                          r'NDSRendererAdapterMvpMod1Q;', source, re.S).group()
    camera_type = re.search(r'typedef struct NDSRendererAdapterMvpCamera\s*\{.*?\} '
                            r'NDSRendererAdapterMvpCamera;', source, re.S).group()
    mod1 = _host(function(source, 'ndsRendererAdapterMvpMod1Q')).replace(
        'ndsRendererAdapterMvpMod1Q(CObj', 'mod1_real(CObj', 1)
    rows = _host(function(source, 'ndsRendererAdapterMvpKind48RowsQ')).replace(
        'ndsRendererAdapterMvpKind48RowsQ(const', 'rows_real(const', 1)
    code = r'''
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <string.h>
typedef float f32; typedef uint16_t u16; typedef uint32_t u32; typedef int32_t s32;
typedef int64_t s64; typedef uint64_t u64; typedef s32 sb32;
typedef float Mtx44f[4][4];
typedef struct { s32 m[4][4]; } NDSRendererMatrix20p12;
typedef struct { f32 x,y,z; } Vec;
typedef struct { struct { Vec eye,at; } vec; } CObj;
typedef struct { struct { struct { Vec f; } vec; } scale; f32 parent_scale; } DObj;
#define TRUE 1
#define FALSE 0
#define nGCMatrixKind48 48
#define NDS_R2_COLLISION_F32_OVERFLOW INT32_MIN
static u32 gNdsRendererAdapterCustom47RejectCount, mod1_calls, rows_calls;
static f32 sNdsRendererAdapterMvpRecalcScaleX;
/* nds_fixed_convert.h's contract: round half away from zero, INT32_MIN
 * past the range; the hardware divider truncates, the root floors. */
static s32 ndsF32ToFixed(f32 value, unsigned bits) {
    double x = (double)value * (double)(1u << bits);
    if (!(x > -2147483648.0 && x < 2147483648.0)) return INT32_MIN;
    return (s32)(x < 0 ? x - 0.5 : x + 0.5);
}
static u64 ndsR2HwMathSqrt64Fast(u64 v) {
    u64 r = (u64)sqrtl((long double)v);
    while (r * r > v) r--;
    while ((r + 1) * (r + 1) <= v) r++;
    return r;
}
static s64 ndsR2HwMathDivideFast(s64 num, s64 den) { return num / den; }
static u32 ndsFloatBits(f32 value) { u32 u; memcpy(&u, &value, 4); return u; }
static sb32 ndsRendererAdapterMvpParentScaleX(DObj *d, f32 *out) {
    *out = d->parent_scale; return TRUE;
}
''' + mod1_type + '\n' + camera_type + r'''
#define NDS_MVP_MEMO_48 4u
static u32 sNdsMvpMemoOn;
static u32 sNdsMvpMemo48Count;
static u32 sNdsMvpMemo46Count;
static u32 sNdsMvpMemo48Key[NDS_MVP_MEMO_48][2];
static NDSRendererMatrix20p12 sNdsMvpMemo48Rows[NDS_MVP_MEMO_48];
static const NDSRendererAdapterMvpCamera *sNdsMvpMemoOwner;
''' + function(source, 'ndsRendererAdapterMvpMemoReset') + '\n' + \
        function(source, 'ndsRendererAdapterMvpMemoFor') + '\n' + \
        function(source, 'ndsMvpMulQ30') + '\n' + function(source, 'ndsMvpScaleQ16') + \
        '\n' + mod1 + '\n' + rows + r'''
/* The production calls, counted. */
static void ndsRendererAdapterMvpMod1Q(CObj *c, NDSRendererAdapterMvpMod1Q *out) {
    mod1_calls++; mod1_real(c, out);
}
static void ndsRendererAdapterMvpKind48RowsQ(const NDSRendererAdapterMvpMod1Q *m,
        const NDSRendererMatrix20p12 *p, f32 sx, f32 sy, NDSRendererMatrix20p12 *out) {
    rows_calls++; rows_real(m, p, sx, sy, out);
}
static void draw(CObj *cobj, DObj *dobj, NDSRendererAdapterMvpCamera *camera,
                 const NDSRendererMatrix20p12 *perspective, NDSRendererMatrix20p12 *result) {
    u32 kind = 48;
    f32 recalc_scale_x, recalc_scale_y;
    NDSRendererMatrix20p12 source_orientation;
    const NDSRendererMatrix20p12 *projection_value = 0, *modelview_value = 0;
    const NDSRendererMatrix20p12 **projection_ptr = &projection_value;
    const NDSRendererMatrix20p12 **modelview_ptr = &modelview_value;
''' + billboard + r'''
    (void)projection_ptr; (void)modelview_ptr; (void)recalc_scale_y;
    *result = source_orientation;
}
int main(void) {
    for (int frame = 0; frame < 24; frame++) {
        CObj c = {0};
        NDSRendererMatrix20p12 persp = {0};
        c.vec.eye = (Vec){frame * 1.5f, 120.0f + frame, 500.0f - frame};
        c.vec.at = (Vec){14.0f, 30.0f, 0.0f};
        if (frame == 0) { c.vec.at.x = c.vec.eye.x; c.vec.at.z = c.vec.eye.z; } /* no yaw */
        persp.m[0][0] = 6000 + frame * 17; persp.m[1][1] = 8000 + frame * 13;
        persp.m[2][2] = -4100 - frame; persp.m[2][3] = -40960 + frame * 64;
        persp.m[3][2] = -4096;
        NDSRendererAdapterMvpCamera shared = {0}, other = {0};
        shared.perspective = &persp; other.perspective = &persp;
        ndsRendererAdapterMvpMemoReset(&shared);
        u32 m0 = mod1_calls, r0 = rows_calls;
        for (int binding = 0; binding < 7; binding++) {
            DObj d = {0};
            int k = binding % 4;   /* bindings 4-6 repeat 0-2's scales */
            d.parent_scale = 0.5f + k * 0.125f;
            d.scale.vec.f = (Vec){1 + k * 0.1f, 0.25f + k * 0.3f, 1};
            NDSRendererMatrix20p12 a, b, o;
            draw(&c, &d, 0, &persp, &a);
            f32 scale = sNdsRendererAdapterMvpRecalcScaleX;
            draw(&c, &d, &shared, &persp, &b);
            assert(memcmp(&a, &b, sizeof(a)) == 0 && scale == sNdsRendererAdapterMvpRecalcScaleX);
            /* A camera that is not the memo's owner builds its own rows. */
            draw(&c, &d, &other, &persp, &o);
            assert(memcmp(&a, &o, sizeof(a)) == 0);
        }
        /* Seven private Mod1s, one a camera; seven private rows, four shared
         * (distinct scales) and seven for the camera the memo does not own. */
        assert(mod1_calls - m0 == 7 + 1 + 1);
        assert(rows_calls - r0 == 7 + 4 + 7);
    }
    assert(gNdsRendererAdapterCustom47RejectCount == 0);
    return 0;
}
'''
    path = tmp_path / 'stage_camera.c'
    exe = path.with_suffix('.exe')
    path.write_text(code)
    build = subprocess.run([shutil.which('gcc'), '-std=c11', '-O2', str(path), '-lm',
                            '-o', str(exe)], capture_output=True, text=True)
    assert build.returncode == 0, build.stderr
    run = subprocess.run([str(exe)], capture_output=True, text=True)
    assert run.returncode == 0, run.stderr
