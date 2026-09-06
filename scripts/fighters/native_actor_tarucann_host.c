/* Host GX recorder for the actual tarucann executor (inserted by pytest).
 * Texture decoding and camera math have their own owners; this exercises the
 * native submission boundary, source corner order and no-draw failure paths. */
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
typedef uint8_t u8;
typedef uint16_t u16;
typedef int16_t s16;
typedef uint32_t u32;
typedef int32_t s32;
typedef int sb32;
typedef s16 v16;
#define TRUE 1
#define FALSE 0
#define NDS_P2_STAGE_JUNGLE 1
#define NDS_RENDERER_HW_TRIANGLES 1
#define NDS_RENDERER_GEOM_ZBUFFER 1u
#define NDS_RENDERER_HW_WORLD_UNIT_SHIFT 8u
#define NDS_RENDERER_TEXTURE_DATA_O2R_WORD_SWAPPED 1u
#define NDS_RENDERER_HW_MATRIX_MODE_FIGHTER_HIERARCHY 4u
#define GL_PROJECTION 1u
#define GL_MODELVIEW 2u
#define NDS_FIGHTER_PACKET_DMA_WAIT() ((void)0)
typedef struct { s32 m[4][4]; } NDSRendererMatrix20p12;
typedef NDSRendererMatrix20p12 m4x4;
typedef struct { s16 x,y,z,s,t; u8 r,g,b,a; } NDSRendererInputVertex;
typedef struct { u32 uls,ult; } NDSRendererTileState;
typedef struct {
    u32 geometry_mode, blend_color, texture_scale_s, texture_scale_t;
    u32 hardware_triangle_count, hardware_zbuffer_triangle_count;
    u32 hardware_vertex_count;
    NDSRendererTileState texture_tiles[1];
} NDSRendererStats;
typedef struct {
    const NDSRendererMatrix20p12 *projection, *camera_modelview, *joint_locals;
    const u8 *joint_parents, *joint_bindings;
    u32 joint_count;
} NDSRendererNativeFighterHierarchy;
typedef struct {
    const NDSRendererMatrix20p12 *initial_projection, *initial_modelview;
    u32 initial_geometry_mode, texture_data_layout;
    const void *(*resolve_data)(const void *,size_t,void *);
    void *user;
} NDSRendererConfig;
typedef struct { int unused; } NDSRendererTraversalState;
static u32 sNdsRendererHardwareMatrixMode, sNdsRendererHardwareMatrixGeneration;
static u32 sNdsRendererHardwareMatrixLoaded, sNdsRendererHardwareSubmitted;
static u32 sNdsRendererHardwareBoundTextureName = 7;
static u32 texture_ok = 1, vertices_emitted, matrices_emitted, triangles_profiled;
static s16 xyz[6][3], uv[6][2], current_uv[2];
static m4x4 matrices[4];
static u32 mode_words[6], mode_count, images[2], image_count;
static void ndsRendererRecordOtherMode(NDSRendererStats *s,u32 op,u32 a,u32 b)
{ (void)s;(void)op;(void)a; assert(mode_count < 6); mode_words[mode_count++]=b; }
static void ndsRendererRecordSetCombine(NDSRendererStats *s,u32 a,u32 b)
{ (void)s; assert(a==0xfc121824u && b==0xff33ffffu); }
static void ndsRendererRecordSetTile(NDSRendererStats *s,u32 a,u32 b)
{ (void)s;(void)a;(void)b; }
static void ndsRendererRecordSetTileSize(NDSRendererStats *s,u32 a,u32 b)
{ (void)s;(void)a; assert(b==0x000fc0fcu); }
static void ndsRendererRecordTextureState(NDSRendererStats *s,u32 a,u32 b)
{ (void)a; s->texture_scale_s=b>>16; s->texture_scale_t=b&65535; }
static void ndsRendererRecordSetImage(NDSRendererStats *s,u32 a,u32 b)
{ (void)s;(void)a; assert(image_count < 2); images[image_count++]=b; }
static void ndsRendererRecordLoadBlock(NDSRendererStats *s,u32 a,u32 b)
{ (void)s;(void)a; assert(b==0x073ff200u); }
static void ndsRendererRecordLoadTlut(NDSRendererStats *s,u32 b)
{ (void)s; assert(b==0x0503c000u); }
static s32 ndsRendererRoundShiftS32Signed(s32 v,u32 shift) { return v>>shift; }
static void ndsRendererCopyMtx20p12ToM4x4(const NDSRendererMatrix20p12 *s,m4x4 *d)
{ *d=*s; }
static void ndsRendererInitTraversalState(NDSRendererTraversalState *s,
    const NDSRendererConfig *c,NDSRendererStats *t,void *v,void *m,u32 n)
{ (void)s;(void)v;(void)m;(void)n; t->geometry_mode=c->initial_geometry_mode; }
static void ndsRendererHardwareEndBatch(void) {}
static s32 ndsRendererHardwareBindTexture(NDSRendererStats *s,
    const NDSRendererConfig *c,NDSRendererTraversalState *t)
{ (void)s;(void)t; assert(c->texture_data_layout==1); return texture_ok; }
static u32 ndsRendererHardwareColorSource(NDSRendererStats *s) { (void)s;return 0; }
static u32 ndsRendererHardwareUseMaterialColor(NDSRendererStats *s) { (void)s;return 0; }
static u32 ndsRendererHardwareUseVertexColor(NDSRendererStats *s) { (void)s;return 1; }
static u32 ndsRendererHardwareAlpha(NDSRendererStats *s,const NDSRendererInputVertex *v)
{ (void)s; return v->a>>3; }
static u32 ndsRendererActiveTextureTile(NDSRendererStats *s) { (void)s;return 0; }
static s32 ndsRendererHardwareTextureFilterOffset(NDSRendererStats *s) { (void)s;return 8; }
static void ndsRendererHardwareSetMatrixMode(u32 m) { (void)m; }
static void glLoadMatrix4x4(const m4x4 *m)
{ assert(matrices_emitted<4); matrices[matrices_emitted++]=*m; }
static void glMultMatrix4x4(const m4x4 *m) { glLoadMatrix4x4(m); }
static u32 ndsRendererNextMatrixGeneration(void) { return 9; }
static void ndsRendererProfileRecordMatrixLoad(void) {}
static u32 ndsRendererHardwarePolyFmt(NDSRendererStats *s,u32 a) { (void)s;return a; }
static void ndsRendererHardwareBeginTriangleBatch(NDSRendererStats *s,
    u32 t,u32 name,u32 p,u32 m,u32 g)
{ (void)s;(void)p;(void)m;(void)g; assert(t && name==7); }
static s16 ndsRendererHardwareVertexCoord(s16 v,u32 scaled)
{ assert(scaled); return v*16; }
static u16 ndsRendererHardwarePackedVertexColor(NDSRendererStats *s,
    const NDSRendererInputVertex *v,u32 c,s32 m,s32 use,u32 color,s32 valid,u32 mod)
{ (void)s;(void)c;(void)m;(void)color;(void)valid;(void)mod; assert(use);return v->r; }
static void glColor(u16 c) { assert(c==255 || c==221); }
static s16 ndsRendererHardwareTexCoord(s16 v,u32 scale,u32 origin,s32 offset)
{ return (s16)(((s32)v*(s32)scale>>17)-(s32)(origin<<2)+offset); }
static void glTexCoord2t16(s16 s,s16 t) { current_uv[0]=s;current_uv[1]=t; }
static void glVertex3v16(s16 x,s16 y,s16 z)
{ assert(vertices_emitted<6); xyz[vertices_emitted][0]=x;
  xyz[vertices_emitted][1]=y;xyz[vertices_emitted][2]=z;
  memcpy(uv[vertices_emitted++],current_uv,sizeof(current_uv)); }
static void ndsRendererProfileRecordHardwareTriangle(void) { triangles_profiled++; }

/* NATIVE_ACTOR_IMPLEMENTATION */

static void reset(void)
{ vertices_emitted=matrices_emitted=triangles_profiled=mode_count=image_count=0; }
int main(void)
{
    u8 asset[3296]={0}, parents[2]={31,0}, bindings[2]={0,1};
    NDSRendererMatrix20p12 identity={ .m={{4096,0,0,0},{0,4096,0,0},
                                       {0,0,4096,0},{0,0,0,4096}} };
    NDSRendererMatrix20p12 local[2]={identity,identity};
    NDSRendererNativeFighterHierarchy h={&identity,&identity,local,parents,bindings,2};
    NDSRendererStats stats={0};
    local[0].m[3][0]=4096;
    assert(ndsRendererSubmitNativeTaruCann(asset,sizeof(asset),&h,1,&stats));
    assert(vertices_emitted==6 && matrices_emitted==4 && triangles_profiled==2);
    assert(stats.hardware_triangle_count==2 && stats.hardware_vertex_count==6);
    assert(sNdsRendererHardwareSubmitted && matrices[2].m[3][0]==16);
    assert(xyz[0][0]==5088 && xyz[1][0]==-5088 && xyz[2][1]==-5088);
    assert(xyz[3][0]==5088 && xyz[4][1]==5088 && xyz[5][1]==-5088);
    assert(uv[0][0]>uv[1][0] && uv[0][1]>uv[2][1]);
    assert(mode_count==6 && mode_words[0]==0x8000 && mode_words[1]==1);
    assert(mode_words[2]==0x553078 && mode_words[3]==0 && mode_words[4]==0);
    assert(mode_words[5]==0x552078 && stats.geometry_mode==0x20001);
    assert(images[0]==(u32)(uintptr_t)(asset+8));
    assert(images[1]==(u32)(uintptr_t)(asset+48));
    reset(); local[0].m[3][0]=8192;
    assert(ndsRendererSubmitNativeTaruCann(asset,sizeof(asset),&h,1,&stats));
    assert(matrices[2].m[3][0]==32); /* live pose, not a cached root */
    reset(); texture_ok=0;
    assert(!ndsRendererSubmitNativeTaruCann(asset,sizeof(asset),&h,1,&stats));
    assert(vertices_emitted==0 && matrices_emitted==0);
    reset(); texture_ok=1; parents[1]=31;
    assert(!ndsRendererSubmitNativeTaruCann(asset,sizeof(asset),&h,1,&stats));
    assert(vertices_emitted==0 && matrices_emitted==0 && mode_count==0);
    parents[1]=0; local[1].m[0][3]=1;
    assert(!ndsRendererSubmitNativeTaruCann(asset,sizeof(asset),&h,1,&stats));
    local[1].m[0][3]=0;
    assert(!ndsRendererSubmitNativeTaruCann(asset,32,&h,1,&stats));
    { NDSNativeTaruCannAssetRange r={asset,sizeof(asset)};
      assert(ndsNativeTaruCannResolveData(asset+48,2048,&r)==asset+48);
      assert(!ndsNativeTaruCannResolveData(asset+3290,16,&r)); }
    return 0;
}
