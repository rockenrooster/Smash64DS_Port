"""Execute the production kind-48 branches with shared and per-draw cameras."""
from pathlib import Path
import re
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts/menus'))
from source_test_helpers import function, braced


def test_shared_camera_operands_preserve_draw_results(tmp_path):
    source = (ROOT / 'src/port/renderer_adapter_matrix.c').read_text()
    apply = function(source, 'ndsRendererAdapterApplyMvpRecalc')
    prepare = braced(apply, r'^\s*if \(\(kind == nGCMatrixKind48\)')
    billboard = braced(apply, r'^\s*else if \(kind == nGCMatrixKind48\)')
    billboard = billboard.replace('else if', 'if', 1)
    camera_type = re.search(r'typedef struct NDSRendererAdapterMvpCamera\s*\{.*?\} NDSRendererAdapterMvpCamera;', source, re.S).group()
    code = r'''
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <string.h>
typedef float f32; typedef uint16_t u16; typedef uint32_t u32;
typedef float Mtx44f[4][4]; typedef struct { int32_t m[4][4]; } Mtx;
typedef Mtx NDSRendererMatrix20p12;
typedef struct { f32 x,y,z; } Vec;
typedef struct { struct { Vec eye,at; } vec;
    struct { struct { u16 norm; f32 fovy,aspect,near,far,scale; } persp; } projection;
} CObj;
typedef struct { struct { struct { Vec f; } vec; } scale; f32 parent_scale; } DObj;
#define TRUE 1
#define FALSE 0
#define nGCMatrixKind48 48
#define nGCMatrixKind46 46
#define NDS_RENDERER_ADAPTER_MVP_RECALC_Z_0X46_KIND 70
#define NDS_RENDERER_ADAPTER_EF_GROUND_BILLBOARD_KIND 72
static u32 perspectives, lookats, cats, gNdsRendererAdapterCustom47RejectCount;
static f32 sNdsRendererAdapterMvpRecalcScaleX;
/* Opaque camera builders stand in for the already source-pinned math. Distinct
 * inputs produce distinct rows, so stale frame reuse and operand drift fail. */
static void syMatrixPerspFastF(Mtx44f m,u16 *n,f32 f,f32 a,f32 near,f32 far,f32 scale) {
    perspectives++; for(int r=0;r<4;r++) for(int c=0;c<4;c++) m[r][c]=(f+a+near+far+scale+r*7+c)*0.01f;
}
static void syMatrixScaF(Mtx44f *m,f32 x,f32 y,f32 z) {
    memset(m,0,sizeof(*m)); (*m)[0][0]=x; (*m)[1][1]=y; (*m)[2][2]=z; (*m)[3][3]=1;
}
static void syMatrixLookAtF(Mtx44f *m,f32 x,f32 y,f32 z,f32 ax,f32 ay,f32 az,f32 ux,f32 uy,f32 uz) {
    assert(x==0 && ax==0 && az==0 && ux==0 && uy==1 && uz==0);
    lookats++; for(int r=0;r<4;r++) for(int c=0;c<4;c++) (*m)[r][c]=(y+z+ay+r*3+c)*0.02f;
}
static void guMtxCatF(Mtx44f a,Mtx44f b,Mtx44f out) {
    Mtx44f temp; cats++; for(int r=0;r<4;r++) for(int c=0;c<4;c++) {
        temp[r][c]=0; for(int k=0;k<4;k++) temp[r][c]+=a[r][k]*b[k][c];
    } memcpy(out,temp,sizeof(temp));
}
static int ndsRendererAdapterMvpParentScaleX(DObj *d,f32 *out) { *out=d->parent_scale; return TRUE; }
static void syMatrixF2L(Mtx44f *in,Mtx *out) { for(int r=0;r<4;r++) for(int c=0;c<4;c++) out->m[r][c]=(int32_t)((*in)[r][c]*4096); }
static void ndsRendererAdapterMtxFromN64(Mtx *in,NDSRendererMatrix20p12 *out) { *out=*in; }
''' + camera_type + '\n' + function(source, 'ndsRendererAdapterMvpPerspectiveF') + '\n' + function(source, 'ndsRendererAdapterMvpMod1F') + r'''
static void draw(CObj *cobj,DObj *dobj,NDSRendererAdapterMvpCamera *camera,Mtx *result) {
    u32 kind=48,row,col; f32 recalc_scale_x,recalc_scale_y;
    Mtx44f local_perspective_f,zrot_f,source_orientation_f;
    f32 (*perspective_f)[4]=local_perspective_f;
    Mtx rotation_mtx; NDSRendererMatrix20p12 source_orientation;
    const Mtx *projection_value=0,*modelview_value=0;
    const Mtx **projection_ptr=&projection_value, **modelview_ptr=&modelview_value;
''' + prepare + '\n' + billboard + r'''
    *result=source_orientation;
}
int main(void) {
    for(int frame=0;frame<24;frame++) {
        CObj c={0}; c.vec.eye=(Vec){frame*1.5f,120.0f+frame,500.0f-frame};
        c.vec.at=(Vec){14.0f,30.0f,0.0f};
        if(frame==0) { c.vec.at.x=c.vec.eye.x; c.vec.at.z=c.vec.eye.z; }
        c.projection.persp.fovy=38+frame; c.projection.persp.aspect=1.333333f;
        c.projection.persp.near=10; c.projection.persp.far=2000; c.projection.persp.scale=1;
        NDSRendererAdapterMvpCamera shared={0};
        u32 p0=perspectives,l0=lookats,c0=cats;
        for(int binding=0;binding<7;binding++) {
            DObj d={0}; d.parent_scale=0.5f+binding*0.125f;
            d.scale.vec.f=(Vec){1+binding*0.1f,0.25f+binding*0.3f,1};
            Mtx a,b; draw(&c,&d,0,&a); f32 scale=sNdsRendererAdapterMvpRecalcScaleX;
            draw(&c,&d,&shared,&b);
            assert(memcmp(&a,&b,sizeof(a))==0 && scale==sNdsRendererAdapterMvpRecalcScaleX);
        }
        assert(perspectives-p0==8); /* Seven private builds, one shared build. */
        assert(lookats-l0==(frame ? 8 : 0) && cats-c0==(frame ? 8 : 0));
    }
    assert(gNdsRendererAdapterCustom47RejectCount==0);
}
'''
    path = tmp_path / 'stage_camera.c'
    exe = path.with_suffix('.exe')
    path.write_text(code)
    build = subprocess.run([shutil.which('gcc'), '-std=c11', '-O2', str(path), '-lm', '-o', str(exe)], capture_output=True, text=True)
    assert build.returncode == 0, build.stderr
    run = subprocess.run([str(exe)], capture_output=True, text=True)
    assert run.returncode == 0, run.stderr
