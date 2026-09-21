"""Execute the shared native quad's real C with a captured GX boundary."""
from pathlib import Path
import shutil
import subprocess


def test_native_quad_submission_and_bind_failure(tmp_path):
    kernel = Path(__file__).resolve().parents[2] / "src/nds/nds_native_textured_quad.exec.inc"
    source = r'''
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
typedef unsigned u32;
typedef unsigned char u8;
typedef unsigned short u16;
typedef int s32;
typedef unsigned long long u64;
typedef long long s64;
#define NDS_RENDERER_HW_TRIANGLES 1
#define NDS_RENDERER_PROFILE_LEVEL 0
#define NDS_RENDERER_GEOM_ZBUFFER 1u
#define POLY_FORMAT_LIGHT0 16u
#define TRUE 1u
#define FALSE 0u
typedef struct { s32 x,y,z,s,t; } NDSRendererInputVertex;
typedef struct { s32 x; } NDSRendererClipVertex20p12;
typedef struct { s32 m[4][4]; } NDSRendererMatrix20p12;
static u32 ndsR2HwMathSqrt64(u64 v) { u64 r = 0u; while ((r + 1u) * (r + 1u) <= v) r++; return (u32)r; }
static s32 ndsR2HwMathDiv64(s64 a, s32 b) { return (s32)(a / b); }
typedef struct { s32 unused; } NDSRendererTraversalState;
typedef struct { s32 uls,ult; u32 width,height; } NDSRendererTileState;
typedef struct {
    u32 geometry_mode, texture_scale_s, texture_scale_t;
    u32 hardware_triangle_count, hardware_zbuffer_triangle_count, hardware_vertex_count;
    u32 matrix_transform_count, transformed_vertex_count;
    u32 prim_color, env_color;
    NDSRendererTileState texture_tiles[1];
} NDSRendererStats;
typedef struct { NDSRendererMatrix20p12 *initial_projection, *initial_modelview; u32 texture_data_layout; } NDSRendererConfig;
#define NDS_RENDERER_TEXTURE_DATA_O2R_WORD_SWAPPED 1u
u32 sNdsRendererHardwareBoundTextureName, sNdsRendererHardwareMatrixMode;
u32 sNdsRendererHardwareMatrixGeneration, sNdsRendererHardwareSubmitted;
u32 fail_bind, test_alpha=31, raw, projected, foreground, matrix_kind, poly_geom;
typedef s32 (*NDSRendererTextureFillCallback)(u8 *pixels, u32 bytes, void *user_data);
u32 gNdsRendererSceneTextureVramResetCount, graded_builds, graded_binds, fail_graded;
u32 palette_uploads; u16 last_color, build_pal[32], upload_pal[32];
#define GL_TEXTURE_2D 1
#define glColorTableEXT(t,l,n,f,x,pal) (palette_uploads++, memcpy(upload_pal,(pal),sizeof(upload_pal)))
void *sNdsRendererHardwareActiveTextureEntry;
u8 sNdsRendererHardwareTextureScratch[4096], graded_pixels[64];
static s32 ndsRendererHardwarePrepareIFCommonA3I5Atlas(u32 w, u32 h, const u16 *pal,
    NDSRendererTextureFillCallback fill, void *user, u32 *name) {
    if (fail_graded) return FALSE;
    memcpy(build_pal, pal, sizeof(build_pal));
    fill(graded_pixels, w * h, user); graded_builds++; *name = 77u; return TRUE; }
#define ndsRendererHardwareBindTextureState(n) (graded_binds++)
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
#define glColor(x) (last_color=(u16)(x))
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
    /* Guard depth: a unit-scale view 1000 units down -Z moves 150 units (the
     * floor) toward the eye along the view ray and touches nothing else. */
    { NDSRendererMatrix20p12 view = {{{4096,0,0,0},{0,4096,0,0},{0,0,4096,0},{0,0,-1000*4096,4096}}};
      ndsRendererNativeQuadGuardDepth(&view, 10);
      assert(view.m[3][2] == -850*4096 && view.m[3][0] == 0 && view.m[3][1] == 0);
      assert(view.m[0][0] == 4096 && view.m[3][3] == 4096); }
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
    /* Graded IA8: built once per image, reused, rebuilt after a scene texture
     * reset, and a refused build falls back to the generic cache. */
    { static const u8 ia8[16] = {0xdb,0x01,0xff,0x0a};
      NDSRendererStats g={0}; g.texture_tiles[0].width=4; g.texture_tiles[0].height=4;
      /* Thunder: PRIM white over ENV 0xffff66. The palette is the combiner
       * ENV->PRIM ramp and the polygon is white, never the material colour. */
      g.prim_color=0xffffffffu; g.env_color=0xffff66ffu;
      fail_bind=0; test_alpha=31;
      assert(ndsRendererNativeTexturedQuadIa8(v,indices,ia8,&cfg,&g,&state)==31);
      assert(graded_builds==1 && sNdsRendererHardwareBoundTextureName==77u);
      assert(build_pal[0]==(31u|(31u<<5)|(12u<<10)) && build_pal[31]==0x7fffu);
      assert(last_color==0x7fffu && palette_uploads==0u);
      assert(graded_pixels[0]==0xbb && graded_pixels[1]==0x00 && graded_pixels[2]==0xff && graded_pixels[3]==0xa0);
      assert(ndsRendererNativeTexturedQuadIa8(v,indices,ia8,&cfg,&g,&state)==31);
      assert(graded_builds==1 && graded_binds==1 && palette_uploads==0u);
      /* A live material colour change re-uploads 32 entries, not the image. */
      g.env_color=0x000000ffu;
      assert(ndsRendererNativeTexturedQuadIa8(v,indices,ia8,&cfg,&g,&state)==31);
      assert(graded_builds==1 && palette_uploads==1u);
      assert(upload_pal[0]==0u && upload_pal[31]==0x7fffu && upload_pal[16]==(16u|(16u<<5)|(16u<<10)));
      gNdsRendererSceneTextureVramResetCount++;
      assert(ndsRendererNativeTexturedQuadIa8(v,indices,ia8,&cfg,&g,&state)==31);
      assert(graded_builds==2);
      gNdsRendererSceneTextureVramResetCount++; fail_graded=1; fail_bind=1;
      assert(ndsRendererNativeTexturedQuadIa8(v,indices,ia8,&cfg,&g,&state)==-1);
      fail_bind=0;
      assert(ndsRendererNativeTexturedQuadIa8(v,indices,ia8,&cfg,&g,&state)==31); }
    /* The lane. An O2R word-swapped file keeps logical byte i at i ^ 3, so the
     * same four source bytes must land mirrored; and CI4 over a one-bit TLUT
     * becomes a coverage ramp: interior 7/7, opaque edge 5/7, clear fringe 2/7
     * in the neighbour's colour, untouched clear 0. */
    { static const u8 ia8b[16] = {0xdb,0x01,0xff,0x0a};
      /* 8x4 CI4, logical rows: 00011000 / 00111100 / 00011000 / 00000000;
       * physical bytes are each logical word reversed. */
      static const u8 ci4[16] = {0x00,0x10,0x01,0x00, 0x00,0x11,0x11,0x00, 0x00,0x10,0x01,0x00, 0,0,0,0};
      /* TLUT: entry 0 clear black, entry 1 opaque red (0xf801); word swapped. */
      static const u8 tlut[32] = {0x01,0xf8,0x00,0x00};
      NDSRendererConfig swapped = {&m,&m,NDS_RENDERER_TEXTURE_DATA_O2R_WORD_SWAPPED};
      NDSRendererStats g={0}; u32 before;
      gNdsRendererSceneTextureVramResetCount++; fail_graded=0; fail_bind=0; test_alpha=31;
      g.texture_tiles[0].width=4; g.texture_tiles[0].height=4;
      assert(ndsRendererNativeTexturedQuadIa8(v,indices,ia8b,&swapped,&g,&state)==31);
      assert(graded_pixels[3]==0xbb && graded_pixels[2]==0x00 && graded_pixels[1]==0xff && graded_pixels[0]==0xa0);
      g.texture_tiles[0].width=8; before=palette_uploads; last_color=0;
      assert(ndsRendererNativeTexturedQuadCi4(v,indices,ci4,tlut,&swapped,&g,&state)==31);
      assert(build_pal[0]==0u && build_pal[1]==31u && palette_uploads==before);
      assert(last_color==0u); /* CI4 keeps TEXEL0 * SHADE, not the white ramp */
      assert(graded_pixels[8+3]==((7u<<5)|1u) && graded_pixels[8+2]==((5u<<5)|1u)); /* row 1 interior, edge... */
      assert(graded_pixels[8+1]==((2u<<5)|1u) && graded_pixels[8+6]==((2u<<5)|1u)); /* ...and fringe */
      assert(graded_pixels[0]==0u && graded_pixels[3]==((5u<<5)|1u) && graded_pixels[2]==((2u<<5)|1u));
      assert(graded_pixels[24+3]==((2u<<5)|1u) && graded_pixels[24]==0u); }
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
    # The helper is shared (Sing's rings, Samus's Charge Shot) and lives beside
    # the quad kernel; both owners must still call it.
    root = Path(__file__).resolve().parents[2]
    path = root / "src/nds/nds_native_textured_quad.exec.inc"
    marker = "static void __attribute__((unused)) ndsRendererNativeQuadGuardDepth("
    body = marker + path.read_text().split(marker, 1)[1].split(
        "\n/* `image` non-NULL selects the graded copy", 1)[0]
    for owner in ("nds_native_purin_sing.exec.inc", "nds_native_samus_chargeshot.exec.inc"):
        assert "ndsRendererNativeQuadGuardDepth(&front_view," in (root / "src/nds" / owner).read_text()
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
    ndsRendererNativeQuadGuardDepth(&v,300);
    assert(v.m[3][2] == -850*4096); /* Guard's 150-unit floor. */
    v.m[0][0]=v.m[1][1]=8192; v.m[3][2]=-1000*4096;
    ndsRendererNativeQuadGuardDepth(&v,300);
    assert(v.m[3][2] == -700*4096); /* Half the live scaled radius. */
    assert(v.m[0][0] == 8192 && v.m[1][1] == 8192); /* Pose unchanged. */
    v.m[0][0]=v.m[1][1]=4096; v.m[3][0]=600*4096; v.m[3][2]=-800*4096;
    ndsRendererNativeQuadGuardDepth(&v,75);
    assert(v.m[3][0] == 510*4096 && v.m[3][2] == -680*4096);
    v.m[3][0]=v.m[3][2]=0;
    ndsRendererNativeQuadGuardDepth(&v,300);
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
