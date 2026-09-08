/* Host GX recorder for the actual yoster-cloud executor (inserted by pytest).
 * Texture decoding and camera math have their own owners; this exercises the
 * native submission boundary, per-alpha tri gating, per-drawable matrix
 * generations, material words, and fail-closed paths with the REAL executor
 * and REAL packet tables spliced at NATIVE_ACTOR_IMPLEMENTATION. */
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
typedef uint8_t u8;
typedef uint16_t u16;
typedef int16_t s16;
typedef uint32_t u32;
typedef int32_t s32;
typedef int sb32;
typedef uint16_t v16;
#define TRUE 1
#define FALSE 0
#define NDS_P2_STAGE_YOSTER 1
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
static NDSRendererMatrix20p12 g_loaded_mvp[8];
static u32 g_loaded_gen[8];
static u32 g_tri_batches, g_verts, g_bind_calls, g_last_poly_alpha;
static u32 g_last_prim_color, g_fail_bind, g_force_alpha_zero;
static u32 g_combine_w0, g_combine_w1, g_combine_calls;
static u32 g_setimage_calls, g_setimage_off, g_record_calls;
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
{ (void)s;(void)a; g_setimage_calls++; g_setimage_off=b; g_record_calls++; }
static void ndsRendererRecordLoadBlock(NDSRendererStats *s,u32 a,u32 b)
{ (void)s;(void)a;(void)b; g_record_calls++; }
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
{ g_last_prim_color=s->prim_color; return 0x12345678u; }
static u32 ndsRendererHardwareAlpha(NDSRendererStats *s,
    const NDSRendererInputVertex *v)
{ (void)v; if (g_force_alpha_zero) { return 0; } return s->prim_color & 0xffu; }
static u32 ndsRendererNextMatrixGeneration(void) { return ++g_gen_counter; }
static void ndsRendererLoadHardwareRawComposedMatrix(
    const NDSRendererMatrix20p12 *m,u32 g)
{ if (g_load_count<8) { g_loaded_mvp[g_load_count]=*m; g_loaded_gen[g_load_count]=g; }
  g_load_count++; }
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

static NDSRendererMatrix20p12 g_proj, g_cam, g_locals[7], g_mvps[3];
static u8 g_parents[7], g_bindings[7];
static NDSRendererStats g_stats;
static u8 g_packet[2048];
static void reset_log(void)
{
    g_load_count=0; g_tri_batches=0; g_verts=0; g_bind_calls=0;
    g_last_poly_alpha=0; g_last_prim_color=0; g_fail_bind=0;
    g_force_alpha_zero=0; g_combine_calls=0; g_setimage_calls=0;
    g_record_calls=0; g_gen_counter=100;
    memset(&g_stats,0,sizeof(g_stats)); memset(g_loaded_gen,0,sizeof(g_loaded_gen));
}
static void build_hierarchy(void)
{
    static const u8 P[7]={31,0,0,0,1,2,3}, B[7]={0,1,2,3,4,5,6};
    u32 i,r,c;
    memcpy(g_parents,P,sizeof(P)); memcpy(g_bindings,B,sizeof(B));
    for (i=0;i<7u;i++) {
        for (r=0;r<4u;r++) for (c=0;c<4u;c++)
            g_locals[i].m[r][c]=(r==c)?4096:0;
        g_locals[i].m[3][3]=4096;
    }
    for (i=0;i<3u;i++)
        for (r=0;r<4u;r++) for (c=0;c<4u;c++)
            g_mvps[i].m[r][c]=(s32)(1000*(i+1)+17*r+c);
}
static NDSRendererNativeFighterHierarchy make_hier(void)
{
    NDSRendererNativeFighterHierarchy h;
    h.projection=&g_proj; h.camera_modelview=&g_cam; h.joint_locals=g_locals;
    h.joint_parents=g_parents; h.joint_bindings=g_bindings;
    h.roots=0; h.config=0; h.joint_count=7u; h.root_count=0u;
    return h;
}
int main(void)
{
    NDSRendererNativeFighterHierarchy h;
    sb32 ok;
    u32 i;
    /* solid: 6 tris, 3 distinct generations, 18 verts, MVP content forwarded */
    reset_log(); build_hierarchy(); h=make_hier();
    ok=ndsRendererSubmitNativeYosterCloud(g_packet,sizeof(g_packet),&h,g_mvps,
        255u,255u,255u,NDS_RENDERER_GEOM_ZBUFFER,&g_stats);
    CHECK("solid", ok && g_stats.hardware_triangle_count==6u);
    CHECK("solid_gen", g_load_count==3u && g_loaded_gen[0]!=g_loaded_gen[1] &&
        g_loaded_gen[1]!=g_loaded_gen[2] && g_loaded_gen[0]!=g_loaded_gen[2]);
    CHECK("solid_verts", g_verts==18u);
    CHECK("solid_mvp", memcmp(&g_loaded_mvp[0],&g_mvps[0],sizeof(g_mvps[0]))==0 &&
        memcmp(&g_loaded_mvp[2],&g_mvps[2],sizeof(g_mvps[2]))==0);
    CHECK("solid_bind", g_bind_calls==1u);
    /* per-alpha gating: drawable 1 evaporated, drawable 2 half */
    reset_log(); build_hierarchy(); h=make_hier();
    ok=ndsRendererSubmitNativeYosterCloud(g_packet,sizeof(g_packet),&h,g_mvps,
        255u,0u,128u,NDS_RENDERER_GEOM_ZBUFFER,&g_stats);
    CHECK("partial", ok && g_stats.hardware_triangle_count==4u);
    CHECK("partial_gen", g_load_count==2u && g_loaded_gen[0]!=g_loaded_gen[1]);
    CHECK("partial_alpha", g_last_poly_alpha==128u);
    /* fully evaporated: ordinary state, TRUE, nothing emitted */
    reset_log(); build_hierarchy(); h=make_hier();
    ok=ndsRendererSubmitNativeYosterCloud(g_packet,sizeof(g_packet),&h,g_mvps,
        0u,0u,0u,NDS_RENDERER_GEOM_ZBUFFER,&g_stats);
    CHECK("evap", ok && g_stats.hardware_triangle_count==0u &&
        g_load_count==0u && g_bind_calls==0u);
    /* live material alpha reaches the poly format through prim_color */
    reset_log(); build_hierarchy(); h=make_hier();
    ok=ndsRendererSubmitNativeYosterCloud(g_packet,sizeof(g_packet),&h,g_mvps,
        0u,0u,77u,NDS_RENDERER_GEOM_ZBUFFER,&g_stats);
    CHECK("material", ok && g_stats.hardware_triangle_count==2u &&
        g_last_poly_alpha==77u && (g_last_prim_color&0xffu)==77u);
    /* hardware resolving alpha to 0 skips the drawable, stays TRUE */
    reset_log(); build_hierarchy(); h=make_hier(); g_force_alpha_zero=1;
    ok=ndsRendererSubmitNativeYosterCloud(g_packet,sizeof(g_packet),&h,g_mvps,
        255u,255u,255u,NDS_RENDERER_GEOM_ZBUFFER,&g_stats);
    CHECK("hwalpha0", ok && g_stats.hardware_triangle_count==0u && g_load_count==0u);
    /* production setup words execute: combine/env/blend/image epoch */
    reset_log(); build_hierarchy(); h=make_hier();
    ok=ndsRendererSubmitNativeYosterCloud(g_packet,sizeof(g_packet),&h,g_mvps,
        255u,255u,255u,NDS_RENDERER_GEOM_ZBUFFER,&g_stats);
    CHECK("combine", g_combine_calls==1u &&
        g_combine_w0==0xfc309604u && g_combine_w1==0x5ffefff8u);
    CHECK("records", g_record_calls>=10u);
    CHECK("envblend", g_stats.env_color==0x0000ffffu &&
        g_stats.blend_color==0x00000008u);
    CHECK("image", g_setimage_calls==1u &&
        g_setimage_off==(u32)(uintptr_t)(g_packet+0x2b8u));
    /* production corner/run/uv ranges */
    { int good=1; for (i=0u;i<6u;i++)
        if (sNdsNativeActorYosterCloudTriIndices[i]>=4u) { good=0; }
      CHECK("corners_range", good); }
    CHECK("corners_exact", sNdsNativeActorYosterCloudTriIndices[0]==2u &&
        sNdsNativeActorYosterCloudTriIndices[1]==1u &&
        sNdsNativeActorYosterCloudTriIndices[2]==0u &&
        sNdsNativeActorYosterCloudTriIndices[3]==0u &&
        sNdsNativeActorYosterCloudTriIndices[4]==3u &&
        sNdsNativeActorYosterCloudTriIndices[5]==2u);
    CHECK("runs", sNdsNativeActorYosterCloudRuns[0]==4u &&
        sNdsNativeActorYosterCloudRuns[1]==0u &&
        sNdsNativeActorYosterCloudRuns[2]==2u &&
        sNdsNativeActorYosterCloudRuns[3]==5u &&
        sNdsNativeActorYosterCloudRuns[6]==6u);
    { int j, vok=1; for (j=0;j<4;j++) {
        s16 s=sNdsNativeActorYosterCloudVerts[j*5+3];
        s16 t=sNdsNativeActorYosterCloudVerts[j*5+4];
        if (s<0||s>4095||t<0||t>4095) { vok=0; } }
      CHECK("uv", vok); }
    CHECK("texepoch", sNdsNativeActorYosterCloudTextureEpoch[0]==0x2b8u &&
        sNdsNativeActorYosterCloudTextureEpoch[1]==512u &&
        sNdsNativeActorYosterCloudTextureEpoch[3]==4u &&
        sNdsNativeActorYosterCloudTextureEpoch[4]==0u);
    /* fail-closed gates */
    reset_log(); build_hierarchy(); h=make_hier();
    ok=ndsRendererSubmitNativeYosterCloud(g_packet,sizeof(g_packet),&h,g_mvps,
        256u,255u,255u,NDS_RENDERER_GEOM_ZBUFFER,&g_stats);
    CHECK("reject_alpha", !ok);
    reset_log(); build_hierarchy(); h=make_hier(); h.joint_count=6u;
    ok=ndsRendererSubmitNativeYosterCloud(g_packet,sizeof(g_packet),&h,g_mvps,
        255u,255u,255u,NDS_RENDERER_GEOM_ZBUFFER,&g_stats);
    CHECK("reject_joint", !ok);
    reset_log(); build_hierarchy(); h=make_hier(); g_parents[6]=9u;
    ok=ndsRendererSubmitNativeYosterCloud(g_packet,sizeof(g_packet),&h,g_mvps,
        255u,255u,255u,NDS_RENDERER_GEOM_ZBUFFER,&g_stats);
    CHECK("reject_parent", !ok);
    reset_log(); build_hierarchy(); h=make_hier(); g_bindings[2]=7u;
    ok=ndsRendererSubmitNativeYosterCloud(g_packet,sizeof(g_packet),&h,g_mvps,
        255u,255u,255u,NDS_RENDERER_GEOM_ZBUFFER,&g_stats);
    CHECK("reject_binding", !ok);
    reset_log(); build_hierarchy(); h=make_hier(); g_locals[3].m[3][3]=0;
    ok=ndsRendererSubmitNativeYosterCloud(g_packet,sizeof(g_packet),&h,g_mvps,
        255u,255u,255u,NDS_RENDERER_GEOM_ZBUFFER,&g_stats);
    CHECK("reject_affine", !ok);
    reset_log(); build_hierarchy(); h=make_hier();
    ok=ndsRendererSubmitNativeYosterCloud(0,sizeof(g_packet),&h,g_mvps,
        255u,255u,255u,NDS_RENDERER_GEOM_ZBUFFER,&g_stats);
    CHECK("reject_nullpacket", !ok);
    reset_log(); build_hierarchy(); h=make_hier();
    ok=ndsRendererSubmitNativeYosterCloud(g_packet,sizeof(g_packet),&h,0,
        255u,255u,255u,NDS_RENDERER_GEOM_ZBUFFER,&g_stats);
    CHECK("reject_nullmvp", !ok);
    reset_log(); build_hierarchy(); h=make_hier();
    ok=ndsRendererSubmitNativeYosterCloud(g_packet,100u,&h,g_mvps,
        255u,255u,255u,NDS_RENDERER_GEOM_ZBUFFER,&g_stats);
    CHECK("reject_short", !ok);
    reset_log(); build_hierarchy(); h=make_hier();
    ok=ndsRendererSubmitNativeYosterCloud(g_packet,sizeof(g_packet),&h,g_mvps,
        255u,255u,255u,0u,&g_stats);
    CHECK("reject_noz", !ok);
    reset_log(); build_hierarchy(); h=make_hier(); g_fail_bind=1;
    ok=ndsRendererSubmitNativeYosterCloud(g_packet,sizeof(g_packet),&h,g_mvps,
        255u,255u,255u,NDS_RENDERER_GEOM_ZBUFFER,&g_stats);
    CHECK("reject_bind", !ok);
    /* kind48 camera-facing mirror: formula lines cite production sources,
     * the pytest half asserts those cited tokens still exist. */
    {
        #define LOOKAT(mf,ex,ey,ez,ax,ay,az,ux,uy,uz) do { \
            float lx=(ax)-(ex),ly=(ay)-(ey),lz=(az)-(ez); \
            float len=-1.0F/sqrtf(lx*lx+ly*ly+lz*lz); \
            float rx,ry,rz,ux2,uy2,uz2; \
            lx*=len; ly*=len; lz*=len; \
            rx=(uy)*lz-(uz)*ly; ry=(uz)*lx-(ux)*lz; rz=(ux)*ly-(uy)*lx; \
            len=1.0F/sqrtf(rx*rx+ry*ry+rz*rz); \
            rx*=len; ry*=len; rz*=len; \
            ux2=ly*rz-lz*ry; uy2=lz*rx-lx*rz; uz2=lx*ry-ly*rx; \
            len=1.0F/sqrtf(ux2*ux2+uy2*uy2+uz2*uz2); \
            ux2*=len; uy2*=len; uz2*=len; \
            (mf)[0][0]=rx; (mf)[1][0]=ry; (mf)[2][0]=rz; (mf)[3][0]=0; \
            (mf)[0][1]=ux2; (mf)[1][1]=uy2; (mf)[2][1]=uz2; (mf)[3][1]=0; \
            (mf)[0][2]=lx; (mf)[1][2]=ly; (mf)[2][2]=lz; (mf)[3][2]=0; \
            (mf)[0][3]=0; (mf)[1][3]=0; (mf)[2][3]=0; (mf)[3][3]=1; } while (0)
        float zA[4][4], zB[4][4];
        float ey=800.0F, ay=600.0F;
        float dx=500.0F-1000.0F, dz=2000.0F-0.0F;
        float ez=sqrtf(dx*dx+dz*dz);
        float c=cosf(0.7F), s=sinf(0.7F);
        /* camera B: same eye_z/eye_y/at_y, yawed by 0.7 rad. Rotating the
         * (dx,dz) offset preserves its length, so eye_z is identical and
         * the Mod1 inputs coincide: yaw removed, vertical relation kept. */
        float dxB=dx*c-dz*s, dzB=dx*s+dz*c, ezB;
        int r2, c2, same=1, okrow=1;
        float o[4][4];
        LOOKAT(zA,0.0F,ey,ez,0.0F,ay,0.0F,0.0F,1.0F,0.0F);
        ezB=sqrtf(dxB*dxB+dzB*dzB);
        LOOKAT(zB,0.0F,ey,ezB,0.0F,ay,0.0F,0.0F,1.0F,0.0F);
        for (r2=0;r2<4;r2++) for (c2=0;c2<4;c2++)
            if (fabsf(zA[r2][c2]-zB[r2][c2])>1e-4F) { same=0; }
        CHECK("k48_yaw", fabsf(ez-ezB)<1e-3F && same);
        memcpy(o,zA,sizeof(o));
        for (r2=0;r2<3;r2++) { float sc=(r2==1)?1.0F:3.0F;
            for (c2=0;c2<4;c2++) { o[r2][c2]=zA[r2][c2]*sc; } }
        for (c2=0;c2<4;c2++) {
            if (fabsf(o[0][c2]-zA[0][c2]*3.0F)>1e-3F) { okrow=0; }
            if (fabsf(o[1][c2]-zA[1][c2]*1.0F)>1e-3F) { okrow=0; }
            if (fabsf(o[2][c2]-zA[2][c2]*3.0F)>1e-3F) { okrow=0; } }
        CHECK("k48_rowscale", okrow);
        CHECK("k48_face", zA[2][2]>0.5F);
    }
    if (g_failures==0) { printf("CLOUD-HOST-OK\n"); }
    return g_failures ? 1 : 0;
}
