"""Execute the cannon's projection upload and check source clip coordinates."""
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/menus"))
from source_test_helpers import function


def test_projection_preserves_clip_coordinates_in_ds_world_units(tmp_path):
    body = function((ROOT / "src/nds/nds_renderer_native_owners.c").read_text(),
                    "ndsRendererSubmitNativeTaruCann")
    begin = body.index("ndsRendererHardwareSetMatrixMode(GL_PROJECTION);")
    end = body.index("glLoadMatrix4x4(&hardware);", begin) + len("glLoadMatrix4x4(&hardware);")
    upload = body[begin:end]
    source = r'''
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <assert.h>
typedef unsigned u32;
typedef struct { int32_t m[4][4]; } NDSRendererMatrix20p12;
typedef struct { int32_t m[16]; } m4x4;
typedef struct { const NDSRendererMatrix20p12 *projection; } Hierarchy;
#define GL_PROJECTION 0
#define NDS_RENDERER_HW_WORLD_UNIT_SHIFT 8
static m4x4 observed;
static void ndsRendererHardwareSetMatrixMode(int mode) { (void)mode; }
static void ndsRendererCopyMtx20p12ToM4x4(const NDSRendererMatrix20p12 *in, m4x4 *out)
{ memcpy(out, in, sizeof(*out)); }
static int32_t ndsRendererRoundShiftS32Signed(int32_t value, u32 bits)
{
    int64_t magnitude = value < 0 ? -(int64_t)value : value;
    int32_t rounded = (int32_t)((magnitude + (1LL << (bits - 1))) >> bits);
    return value < 0 ? -rounded : rounded;
}
static void glLoadMatrix4x4(const m4x4 *matrix) { observed = *matrix; }
static void upload(const Hierarchy *hierarchy)
{
    m4x4 hardware;
    u32 i;
''' + upload + r'''
}
static void clip(const int32_t *matrix, const double *v, double *out)
{
    for (int col = 0; col < 4; ++col) {
        out[col] = 0;
        for (int row = 0; row < 4; ++row)
            out[col] += v[row] * matrix[row * 4 + col] / 4096.0;
    }
}
int main(void)
{
    NDSRendererMatrix20p12 projection = {{{0}}};
    projection.m[0][0] = 8000;
    projection.m[1][1] = 10000;
    projection.m[2][2] = -4149;
    projection.m[2][3] = -4096;
    projection.m[3][2] = -2110687;
    Hierarchy hierarchy = { &projection };
    for (int mode = 0; mode < 2; ++mode) {
        if (mode) {
            projection.m[2][3] = 0;
            projection.m[3][3] = 4096;
            projection.m[3][0] = 1024;
            projection.m[3][1] = -2048;
        }
        upload(&hierarchy);
        assert(memcmp(observed.m, projection.m, 12 * sizeof(int32_t)) == 0);
        for (int depth = 1024; depth <= 16384; depth *= 2) {
            double source_v[] = {318, -318, -depth, 1};
            double ds_v[] = {318.0/256, -318.0/256, -depth/256.0, 1};
            double expected[4], actual[4];
            clip(&projection.m[0][0], source_v, expected);
            clip(observed.m, ds_v, actual);
            for (int col = 0; col < 4; ++col)
                assert(fabs(actual[col] - expected[col]/256) <= 0.000123);
            if (!mode) {
                assert(actual[2] >= -actual[3] && actual[2] <= actual[3]);
                assert(fabs(actual[2]/actual[3] - expected[2]/expected[3]) < 0.0001);
            }
        }
    }
    return 0;
}
'''
    compiler = shutil.which("clang") or shutil.which("gcc")
    assert compiler, "Host C compiler required"
    c_file = tmp_path / "projection.c"
    exe = tmp_path / "projection.exe"
    c_file.write_text(source)
    built = subprocess.run([compiler, "-std=c11", str(c_file), "-o", str(exe)],
                           capture_output=True, text=True)
    assert built.returncode == 0, built.stderr[-4000:]
    ran = subprocess.run([str(exe)], capture_output=True, text=True)
    assert ran.returncode == 0, ran.stderr[-4000:]
