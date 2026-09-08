/* Host GX recorder for the actual ef-bronto executor (inserted by pytest).
 * Texture decoding and camera math have their own owners; this exercises the
 * native submission boundary, live sprite-frame selection, per-submit matrix
 * generations, material words, and fail-closed paths with the REAL executor
 * and REAL packet tables spliced at NATIVE_ACTOR_IMPLEMENTATION. */
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
typedef uint8_t u8;
typedef uint16_t u16;
typedef int16_t s16;
typedef uint32_t u32;
typedef int32_t s32;
typedef int sb32;
typedef uint16_t v16;
#define TRUE 1
#define FALSE 0
#define NDS_RENDERER_HW_TRIANGLES 1
#define NDS_RENDERER_GEOM_ZBUFFER 1u
#define NDS_RENDERER_TEXTURE_DATA_O2R_WORD_SWAPPED 1u
#define NDS_FIGHTER_PACKET_DMA_WAIT() ((void)0)
typedef struct { s32 m[4][4]; } NDSRendererMatrix20p12;
typedef struct { s16 x,y,z,s,t; u8 r,g,b,a; } NDSRendererInputVertex;
typedef struct { u32 uls,ult; } NDSRendererTileState;
typedef struct {
    u32 geometry_mode, prim_color, env_color, blend_color;
    u32 texture_scale_s, texture_scale_t;
    u32 hardware_triangle_count, hardware_zbuffer_triangle_count;
    u32 hardware_vertex_count;
    NDSRendererTileState texture_tiles[8];
} NDSRendererStats;
typedef struct {
    const NDSRendererMatrix20p12 *projection, *camera_modelview, *joint_locals;
    const u8 *joint_parents, *joint_bindings;
    const void *roots, *config;
    u32 joint_count, root_count;
} NDSRendererNativeFighterHierarchy;
typedef struct {
    const NDSRendererMatrix20p12 *initial_projection, *initial_modelview;
    u32 initial_geometry_mode, texture_data_layout;
} NDSRendererConfig;
typedef struct { int unused; } NDSRendererTraversalState;
static u32 sNdsRendererHardwareBoundTextureName = 7;
static u32 sNdsRendererHardwareMatrixMode, sNdsRendererHardwareSubmitted;
static u32 g_load_count, g_gen_counter = 100;
static NDSRendererMatrix20p12 g_loaded_mvp;
static u32 g_loaded_gen;
static u32 g_tri_batches, g_verts, g_bind_calls, g_last_poly_alpha;
static u32 g_fail_bind, g_force_alpha_zero;
static u32 g_combine_w0, g_combine_w1, g_combine_calls;
static u32 g_setimage_calls, g_record_calls;
static u32 g_setimage_w0[4], g_setimage_off[4];
static u32 g_loadtlut_calls, g_last_loadtlut;
static int g_failures;
#define CHECK(name, cond) do { if (!(cond)) { \
    printf("FAIL %s line %d\n", name, __LINE__); g_failures++; } } while (0)
static void ndsRendererRecordOtherMode(NDSRendererStats *s,u32 a,u32 b,u32 c)
{ (void)s;(void)a;(void)b;(void)c; g_record_calls++; }
static void ndsRendererRecordSetCombine(NDSRendererStats *s,u32 a,u32 b)
{ (void)s; g_combine_w0=a; g_combine_w1=b; g_combine_calls++; g_record_calls++; }
static void ndsRendererRecordSetTile(NDSRendererStats *s,u32 a,u32 b)
{ (void)s;(void)a;(void)b; g_record_calls++; }
static void ndsRendererRecordTextureState(NDSRendererStats *s,u32 a,u32 b)
{ (void)s;(void)a;(void)b; g_record_calls++; }
static void ndsRendererRecordSetTileSize(NDSRendererStats *s,u32 a,u32 b)
{ (void)s;(void)a;(void)b; g_record_calls++; }
static void ndsRendererRecordSetImage(NDSRendererStats *s,u32 a,u32 b)
{ (void)s; if (g_setimage_calls<4) {
      g_setimage_w0[g_setimage_calls]=a; g_setimage_off[g_setimage_calls]=b; }
  g_setimage_calls++; g_record_calls++; }
static void ndsRendererRecordLoadBlock(NDSRendererStats *s,u32 a,u32 b)
{ (void)s;(void)a;(void)b; g_record_calls++; }
static void ndsRendererRecordLoadTlut(NDSRendererStats *s,u32 a)
{ (void)s; g_loadtlut_calls++; g_last_loadtlut=a; g_record_calls++; }
static void ndsRendererInitTraversalState(NDSRendererTraversalState *s,
    const NDSRendererConfig *c,NDSRendererStats *t,void *v,void *m,u32 n)
{ (void)s;(void)v;(void)m;(void)n; t->geometry_mode=c->initial_geometry_mode; }
static void ndsRendererHardwareEndBatch(void) {}
static s32 ndsRendererHardwareBindTexture(NDSRendererStats *s,
    const NDSRendererConfig *c,NDSRendererTraversalState *t)
{ (void)s;(void)c;(void)t; g_bind_calls++; return g_fail_bind ? 0 : 1; }
static u32 ndsRendererHardwareUseMaterialColor(NDSRendererStats *s)
{ (void)s; return 1; }
static u32 ndsRendererHardwareUseVertexColor(NDSRendererStats *s)
{ (void)s; return 1; }
static u32 ndsRendererActiveTextureTile(NDSRendererStats *s) { (void)s; return 0; }
static s32 ndsRendererHardwareTextureFilterOffset(NDSRendererStats *s)
{ (void)s; return 0; }
static u32 ndsRendererHardwareColorSource(NDSRendererStats *s)
{ (void)s; return 0x12345678u; }
static u32 ndsRendererHardwareAlpha(NDSRendererStats *s,
    const NDSRendererInputVertex *v)
{ (void)v; if (g_force_alpha_zero) { return 0; } return 255u; }
static u32 ndsRendererNextMatrixGeneration(void) { return ++g_gen_counter; }
static void ndsRendererLoadHardwareRawComposedMatrix(
    const NDSRendererMatrix20p12 *m,u32 g)
{ g_loaded_mvp=*m; g_loaded_gen=g; g_load_count++; }
static void ndsRendererHardwareBeginTriangleBatch(NDSRendererStats *s,
    u32 t,u32 name,u32 p,u32 m,u32 g)
{ (void)s;(void)t;(void)name;(void)m;(void)g; g_tri_batches++; }
static u32 ndsRendererHardwarePolyFmt(NDSRendererStats *s,u32 a)
{ (void)s; g_last_poly_alpha=a; return 0xdead0000u|(a&0xffu); }
static v16 ndsRendererHardwareVertexCoord(s16 v,s32 c) { (void)c; return (v16)(v+100); }
static u32 ndsRendererHardwarePackedVertexColor(NDSRendererStats *s,
    const NDSRendererInputVertex *v,u32 c,u32 m,u32 u,u32 a,u32 b,u32 d)
{ (void)s;(void)v;(void)c;(void)m;(void)u;(void)a;(void)b;(void)d; return 1; }
static s32 ndsRendererHardwareTexCoord(s16 v,u32 sc,u32 o,s32 off)
{ (void)sc;(void)o;(void)off; return (s32)v; }
static void ndsRendererProfileRecordHardwareTriangle(void) {}
static void glColor(u32 c) { (void)c; }
static void glTexCoord2t16(s32 a,s32 b) { (void)a;(void)b; }
static void glVertex3v16(v16 x,v16 y,v16 z) { (void)x;(void)y;(void)z; g_verts++; }

/* NATIVE_ACTOR_IMPLEMENTATION */

static NDSRendererMatrix20p12 g_proj, g_cam, g_locals[3], g_mvp;
static u8 g_parents[3], g_bindings[3];
static NDSRendererStats g_stats;
static u8 g_packet[16384];
static void reset_log(void)
{
    g_load_count=0; g_tri_batches=0; g_verts=0; g_bind_calls=0;
    g_last_poly_alpha=0; g_fail_bind=0;
    g_force_alpha_zero=0; g_combine_calls=0; g_setimage_calls=0;
    g_record_calls=0; g_loadtlut_calls=0; g_gen_counter=100;
    memset(&g_stats,0,sizeof(g_stats)); g_loaded_gen=0;
}
static void build_hierarchy(void)
{
    static const u8 P[3]={31,0,1}, B[3]={0,1,2};
    u32 i,r,c;
    memcpy(g_parents,P,sizeof(P)); memcpy(g_bindings,B,sizeof(B));
    for (i=0;i<3u;i++) {
        for (r=0;r<4u;r++) for (c=0;c<4u;c++)
            g_locals[i].m[r][c]=(r==c)?4096:0;
        g_locals[i].m[3][3]=4096;
    }
    for (r=0;r<4u;r++) for (c=0;c<4u;c++)
        g_mvp.m[r][c]=(s32)(777+17*r+c);
}
static NDSRendererNativeFighterHierarchy make_hier(void)
{
    NDSRendererNativeFighterHierarchy h;
    h.projection=&g_proj; h.camera_modelview=&g_cam; h.joint_locals=g_locals;
    h.joint_parents=g_parents; h.joint_bindings=g_bindings;
    h.roots=0; h.config=0; h.joint_count=3u; h.root_count=0u;
    return h;
}
int main(void)
{
    NDSRendererNativeFighterHierarchy h;
    sb32 ok;
    u32 i;
    /* frame 0: 2 tris, 6 verts, MVP forwarded, hook image = frame-0 offset */
    reset_log(); build_hierarchy(); h=make_hier();
    ok=ndsRendererSubmitNativeEfBronto(g_packet,sizeof(g_packet),&h,&g_mvp,0u,
        NDS_RENDERER_GEOM_ZBUFFER,&g_stats);
    CHECK("solid", ok && g_stats.hardware_triangle_count==2u);
    CHECK("solid_gen", g_load_count==1u);
    CHECK("solid_verts", g_verts==6u);
    CHECK("solid_mvp", memcmp(&g_loaded_mvp,&g_mvp,sizeof(g_mvp))==0);
    CHECK("solid_bind", g_bind_calls==1u);
    CHECK("hook0", g_setimage_calls==2u &&
        g_setimage_w0[0]==0xfd100000u &&
        g_setimage_off[0]==(u32)(uintptr_t)(g_packet+0x2d38u) &&
        g_setimage_w0[1]==0xfd500000u &&
        g_setimage_off[1]==(u32)(uintptr_t)(g_packet+0x306cu));
    /* live frame change selects a different image, same tri census */
    reset_log(); build_hierarchy(); h=make_hier();
    ok=ndsRendererSubmitNativeEfBronto(g_packet,sizeof(g_packet),&h,&g_mvp,2u,
        NDS_RENDERER_GEOM_ZBUFFER,&g_stats);
    CHECK("frame2", ok && g_stats.hardware_triangle_count==2u);
    CHECK("hook2", g_setimage_calls==2u &&
        g_setimage_off[1]==(u32)(uintptr_t)(g_packet+0x2d5cu));
    reset_log(); build_hierarchy(); h=make_hier();
    ok=ndsRendererSubmitNativeEfBronto(g_packet,sizeof(g_packet),&h,&g_mvp,1u,
        NDS_RENDERER_GEOM_ZBUFFER,&g_stats);
    CHECK("hook1", ok &&
        g_setimage_off[1]==(u32)(uintptr_t)(g_packet+0x2ee4u));
    /* production setup words execute: combine + palette epoch */
    CHECK("combine", g_combine_calls==1u &&
        g_combine_w0==0xfc127e24u && g_combine_w1==0xfffff3f9u);
    CHECK("tlut", g_loadtlut_calls==1u && g_last_loadtlut==0x0503c000u);
    CHECK("records", g_record_calls>=10u);
    /* live pose change keeps census (matrix path is live, not baked) */
    reset_log(); build_hierarchy(); h=make_hier();
    g_locals[1].m[3][1]=9999;
    ok=ndsRendererSubmitNativeEfBronto(g_packet,sizeof(g_packet),&h,&g_mvp,0u,
        NDS_RENDERER_GEOM_ZBUFFER,&g_stats);
    CHECK("pose_live", ok && g_stats.hardware_triangle_count==2u);
    /* production corner/run/uv ranges */
    { int good=1; for (i=0u;i<6u;i++)
        if (sNdsNativeActorEfBrontoTriIndices[i]>=4u) { good=0; }
      CHECK("corners_range", good); }
    CHECK("corners_exact", sNdsNativeActorEfBrontoTriIndices[0]==3u &&
        sNdsNativeActorEfBrontoTriIndices[1]==2u &&
        sNdsNativeActorEfBrontoTriIndices[2]==1u &&
        sNdsNativeActorEfBrontoTriIndices[3]==0u &&
        sNdsNativeActorEfBrontoTriIndices[4]==3u &&
        sNdsNativeActorEfBrontoTriIndices[5]==1u);
    CHECK("runs", sNdsNativeActorEfBrontoRuns[0]==2u &&
        sNdsNativeActorEfBrontoRuns[1]==0u &&
        sNdsNativeActorEfBrontoRuns[2]==2u);
    { int j, vok=1; for (j=0;j<4;j++) {
        s16 s=sNdsNativeActorEfBrontoVerts[j*5+3];
        s16 t=sNdsNativeActorEfBrontoVerts[j*5+4];
        if (s<0||s>4095||t<0||t>4095) { vok=0; } }
      CHECK("uv", vok); }
    CHECK("texepoch", sNdsNativeActorEfBrontoTextureEpoch[0]==0x2d38u &&
        sNdsNativeActorEfBrontoTextureEpoch[1]==32u &&
        sNdsNativeActorEfBrontoTextureEpoch[2]==0x306cu &&
        sNdsNativeActorEfBrontoTextureEpoch[3]==388u &&
        sNdsNativeActorEfBrontoTextureEpoch[6]==0x2d5cu &&
        sNdsNativeActorEfBrontoTextureEpoch[7]==392u);
    /* hardware resolving alpha to 0 skips the drawable, stays TRUE */
    reset_log(); build_hierarchy(); h=make_hier(); g_force_alpha_zero=1;
    ok=ndsRendererSubmitNativeEfBronto(g_packet,sizeof(g_packet),&h,&g_mvp,0u,
        NDS_RENDERER_GEOM_ZBUFFER,&g_stats);
    CHECK("hwalpha0", ok && g_stats.hardware_triangle_count==0u && g_load_count==0u);
    /* fail-closed gates: corrupt frame is topology corruption, not a frame */
    reset_log(); build_hierarchy(); h=make_hier();
    ok=ndsRendererSubmitNativeEfBronto(g_packet,sizeof(g_packet),&h,&g_mvp,3u,
        NDS_RENDERER_GEOM_ZBUFFER,&g_stats);
    CHECK("reject_frame3", !ok);
    reset_log(); build_hierarchy(); h=make_hier();
    ok=ndsRendererSubmitNativeEfBronto(g_packet,sizeof(g_packet),&h,&g_mvp,99u,
        NDS_RENDERER_GEOM_ZBUFFER,&g_stats);
    CHECK("reject_frame99", !ok);
    reset_log(); build_hierarchy(); h=make_hier(); h.joint_count=2u;
    ok=ndsRendererSubmitNativeEfBronto(g_packet,sizeof(g_packet),&h,&g_mvp,0u,
        NDS_RENDERER_GEOM_ZBUFFER,&g_stats);
    CHECK("reject_joint", !ok);
    reset_log(); build_hierarchy(); h=make_hier(); g_parents[1]=1u;
    ok=ndsRendererSubmitNativeEfBronto(g_packet,sizeof(g_packet),&h,&g_mvp,0u,
        NDS_RENDERER_GEOM_ZBUFFER,&g_stats);
    CHECK("reject_parent", !ok);
    reset_log(); build_hierarchy(); h=make_hier(); g_bindings[0]=1u;
    ok=ndsRendererSubmitNativeEfBronto(g_packet,sizeof(g_packet),&h,&g_mvp,0u,
        NDS_RENDERER_GEOM_ZBUFFER,&g_stats);
    CHECK("reject_binding", !ok);
    reset_log(); build_hierarchy(); h=make_hier(); g_locals[0].m[3][3]=0;
    ok=ndsRendererSubmitNativeEfBronto(g_packet,sizeof(g_packet),&h,&g_mvp,0u,
        NDS_RENDERER_GEOM_ZBUFFER,&g_stats);
    CHECK("reject_affine", !ok);
    reset_log(); build_hierarchy(); h=make_hier();
    ok=ndsRendererSubmitNativeEfBronto(0,sizeof(g_packet),&h,&g_mvp,0u,
        NDS_RENDERER_GEOM_ZBUFFER,&g_stats);
    CHECK("reject_nullpacket", !ok);
    reset_log(); build_hierarchy(); h=make_hier();
    ok=ndsRendererSubmitNativeEfBronto(g_packet,sizeof(g_packet),&h,0,0u,
        NDS_RENDERER_GEOM_ZBUFFER,&g_stats);
    CHECK("reject_nullmvp", !ok);
    reset_log(); build_hierarchy(); h=make_hier();
    ok=ndsRendererSubmitNativeEfBronto(g_packet,0x2d5cu,&h,&g_mvp,0u,
        NDS_RENDERER_GEOM_ZBUFFER,&g_stats);
    CHECK("reject_short", !ok);
    reset_log(); build_hierarchy(); h=make_hier();
    ok=ndsRendererSubmitNativeEfBronto(g_packet,sizeof(g_packet),&h,&g_mvp,0u,
        0u,&g_stats);
    CHECK("reject_noz", !ok);
    reset_log(); build_hierarchy(); h=make_hier(); g_fail_bind=1;
    ok=ndsRendererSubmitNativeEfBronto(g_packet,sizeof(g_packet),&h,&g_mvp,0u,
        NDS_RENDERER_GEOM_ZBUFFER,&g_stats);
    CHECK("reject_bind", !ok);
    if (g_failures==0) { printf("BRONTO-HOST-OK\n"); }
    return g_failures ? 1 : 0;
}
