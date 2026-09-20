"""Execute the shared native quad's real C with a captured GX boundary."""
from pathlib import Path
import shutil
import subprocess


def test_native_quad_submission_and_bind_failure(tmp_path):
    kernel = Path(__file__).resolve().parents[2] / "src/nds/nds_native_textured_quad.exec.inc"
    source = r'''
#include <assert.h>
#include <stddef.h>
typedef unsigned u32;
typedef unsigned short u16;
typedef int s32;
#define NDS_RENDERER_HW_TRIANGLES 1
#define NDS_RENDERER_PROFILE_LEVEL 0
#define NDS_RENDERER_GEOM_ZBUFFER 1u
#define POLY_FORMAT_LIGHT0 16u
#define TRUE 1u
#define FALSE 0u
typedef struct { s32 x,y,z,s,t; } NDSRendererInputVertex;
typedef struct { s32 x; } NDSRendererClipVertex20p12;
typedef struct { s32 unused; } NDSRendererMatrix20p12;
typedef struct { s32 unused; } NDSRendererTraversalState;
typedef struct { s32 uls,ult; } NDSRendererTileState;
typedef struct {
    u32 geometry_mode, texture_scale_s, texture_scale_t;
    u32 hardware_triangle_count, hardware_zbuffer_triangle_count, hardware_vertex_count;
    u32 matrix_transform_count, transformed_vertex_count;
    NDSRendererTileState texture_tiles[1];
} NDSRendererStats;
typedef struct { NDSRendererMatrix20p12 *initial_projection, *initial_modelview; } NDSRendererConfig;
u32 sNdsRendererHardwareBoundTextureName, sNdsRendererHardwareMatrixMode;
u32 sNdsRendererHardwareMatrixGeneration, sNdsRendererHardwareSubmitted;
u32 fail_bind, test_alpha=31, raw, projected, foreground, matrix_kind, poly_geom;
s32 test_depth=100, xs[6], zs[6];
#define NDS_FIGHTER_PACKET_DMA_WAIT() ((void)0)
#define ndsRendererHardwareEndBatch() ((void)0)
#define ndsRendererHardwareBindTexture(a,b,c) (!fail_bind)
#define ndsRendererHardwareAlpha(a,b) test_alpha
#define ndsRendererHardwareColorSource(a) 0u
#define ndsRendererHardwareUseMaterialColor(a) 0u
#define ndsRendererHardwareUseVertexColor(a) 0u
#define ndsRendererActiveTextureTile(a) 0u
#define ndsRendererHardwareTextureFilterOffset(a) 0
#define ndsRendererMtxMul20p12(a,b,c) ((void)0)
#define ndsRendererLoadHardwareMatrices(a,b) (matrix_kind=1)
#define ndsRendererNextMatrixGeneration() 1u
#define ndsRendererLoadHardwareSplitMatrices(a,b,c) (matrix_kind=2)
#define ndsRendererHardwarePolyFmt(s,a) (poly_geom=(s)->geometry_mode)
#define ndsRendererHardwareBeginTriangleBatch(a,b,c,d,e,f) ((void)(d))
#define ndsRendererHardwareNextProjectedDepth() (test_depth++)
#define ndsRendererHardwarePackedVertexColor(a,b,c,d,e,f,g,h) 0u
#define glColor(x) ((void)(x))
#define ndsRendererHardwareTexCoord(a,b,c,d) (a)
#define glTexCoord2t16(x,y) ((void)(x),(void)(y))
#define ndsRendererTransformVertex20p12(m,v,c) ((c)->x=(v)->x*2)
#define ndsRendererHardwareClipVertex(c,z) (xs[projected]=(c)->x,zs[projected++]=(z))
#define ndsRendererHardwareVertexCoord(v,b) (v)
#define glVertex3v16(x,y,z) (xs[raw++]=(x))
#define ndsRendererProfileRecordHardwareTriangle() ((void)0)
#define ndsRendererHardwareEnterProjectedForeground() (foreground++)
'''
    source += '\n#include "' + kernel.as_posix() + '"\n'
    source += r'''
int main(void) {
    NDSRendererInputVertex v[4] = {{1},{2},{3},{4}};
    const u16 indices[6] = {3,2,1,0,3,1};
    NDSRendererMatrix20p12 m = {0}; NDSRendererConfig cfg = {&m,&m};
    NDSRendererTraversalState state = {0};
    {
        NDSRendererStats stats = {0}; stats.geometry_mode=5;
        raw=projected=foreground=0; test_depth=100;
        assert(ndsRendererNativeTexturedQuad(v,indices,&cfg,&stats,&state)==31);
        assert(stats.hardware_triangle_count==2 && stats.hardware_vertex_count==6);
        assert(stats.hardware_zbuffer_triangle_count==2u);
        assert(stats.geometry_mode==5 && matrix_kind==2u);
        assert(poly_geom==5u && raw==6u);
        for (u32 i=0;i<6;++i) {
            assert(xs[i]==(s32)(indices[i]+1));
        }
        assert(foreground==0 && stats.matrix_transform_count==0);
    }
    NDSRendererStats stats={0}; test_alpha=0; raw=projected=0;
    assert(ndsRendererNativeTexturedQuad(v,indices,&cfg,&stats,&state)==0);
    assert(raw==0 && projected==0 && stats.hardware_triangle_count==0);
    fail_bind=1;
    assert(ndsRendererNativeTexturedQuad(v,indices,&cfg,&stats,&state)==-1);
    return 0;
}
'''
    c_file = tmp_path / "quad.c"
    executable = tmp_path / "quad.exe"
    c_file.write_text(source)
    cc = shutil.which("gcc") or shutil.which("clang")
    assert cc, "Host C compiler required"
    subprocess.run([cc, "-std=c11", str(c_file), "-o", str(executable)], check=True, capture_output=True)
    result = subprocess.run([str(executable)], capture_output=True, text=True)
    assert result.returncode == 0, result.stderr


def test_sing_uses_live_guard_camera_bias(tmp_path):
    path = Path(__file__).resolve().parents[2] / "src/nds/nds_native_purin_sing.exec.inc"
    body = "static void ndsNativePurinSingGuardDepth" + path.read_text().split(
        "static void ndsNativePurinSingGuardDepth", 1)[1].split(
        "\nsb32 ndsRendererSubmitNativePurinSing", 1)[0]
    source = r'''
#include <assert.h>
#include <stdint.h>
#include <math.h>
typedef int32_t s32; typedef int64_t s64;
typedef uint32_t u32; typedef uint64_t u64;
typedef struct { s32 m[4][4]; } NDSRendererMatrix20p12;
u32 ndsR2HwMathSqrt64(u64 v) { return (u32)sqrt((double)v); }
s64 ndsR2HwMathDiv64(s64 v, s32 d) { return v/d; }
''' + body + r'''
int main(void) {
    NDSRendererMatrix20p12 v = {{{4096,0,0,0},{0,4096,0,0},{0,0,4096,0},{0,0,-1000*4096,4096}}};
    ndsNativePurinSingGuardDepth(&v,300);
    assert(v.m[3][2] == -850*4096); /* Guard's 150-unit floor. */
    v.m[0][0]=v.m[1][1]=8192; v.m[3][2]=-1000*4096;
    ndsNativePurinSingGuardDepth(&v,300);
    assert(v.m[3][2] == -700*4096); /* Half the live scaled radius. */
    assert(v.m[0][0] == 8192 && v.m[1][1] == 8192); /* Pose unchanged. */
    v.m[0][0]=v.m[1][1]=4096; v.m[3][0]=600*4096; v.m[3][2]=-800*4096;
    ndsNativePurinSingGuardDepth(&v,75);
    assert(v.m[3][0] == 510*4096 && v.m[3][2] == -680*4096);
    v.m[3][0]=v.m[3][2]=0;
    ndsNativePurinSingGuardDepth(&v,300);
    assert(v.m[3][0] == 0 && v.m[3][2] == 0);
    return 0;
}
'''
    c_file, executable = tmp_path / "bias.c", tmp_path / "bias.exe"
    c_file.write_text(source)
    cc = shutil.which("gcc") or shutil.which("clang")
    assert cc, "Host C compiler required"
    subprocess.run([cc, "-std=c11", str(c_file), "-lm", "-o", str(executable)], check=True, capture_output=True)
    result = subprocess.run([str(executable)], capture_output=True, text=True)
    assert result.returncode == 0, result.stderr
