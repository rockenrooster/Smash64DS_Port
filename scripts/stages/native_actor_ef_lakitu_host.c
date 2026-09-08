/* Host GX recorder for the actual ef-lakitu executor (inserted by pytest).
 * Texture decoding and camera math have their own owners; this exercises the
 * native submission boundary, the three-stage submit (warm pure-resident
 * preflight, bounded cold upload pass, eviction-proof pure reverify, then
 * rebind-by-name emission), per-epoch tri emission, per-drawable matrix
 * generations, material words, and fail-closed paths with the REAL executor
 * and REAL packet tables spliced at NATIVE_ACTOR_IMPLEMENTATION.
 *
 * The resolve/bind stubs mirror the production semantics this executor
 * depends on: ResolveResidentTexture is a PURE resident lookup (no upload,
 * no eviction, no bind side effects, FALSE before upload on a miss) whose
 * failure increments the reject counter; BindTexture is the live
 * upload-on-miss bind (FALSE with reject telemetry on a forced fail, else
 * the epoch becomes resident and the per-bind eviction mask applies, so an
 * upload can drop a sibling exactly like a VRAM eviction would);
 * BindTextureName is void, elides an already-bound name, and only then
 * closes the batch and switches. */
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
#define NDS_P2_STAGE_CASTLE 1
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
    u32 hardware_texture_bind_count, hardware_texture_ready_count;
    u32 hardware_texture_reject_count;
    u32 hardware_texture_format, hardware_texture_width;
    u32 hardware_texture_height;
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
/* Cache-entry mirror: the fields the executor's commit sequence touches. */
typedef struct { u8 ready, pinned; u32 name, params, last_used_frame; }
    NDSRendererHardwareTextureCacheEntry;
typedef struct {
    NDSRendererHardwareTextureCacheEntry *entry;
    u32 name, params, format, width, height;
} NDSRendererHardwareResolvedTexture;
static u32 sNdsRendererHardwareBoundTextureName = 7;
static u32 sNdsRendererHardwareMatrixMode, sNdsRendererHardwareSubmitted;
static NDSRendererHardwareTextureCacheEntry *sNdsRendererHardwareActiveTextureEntry;
static u32 sNdsRendererHardwareFrameSerial = 9u;
static u32 g_load_count, g_gen_counter = 100;
static NDSRendererMatrix20p12 g_loaded_mvp[8];
static u32 g_loaded_gen[8];
static u32 g_tri_batches, g_verts, g_last_poly_alpha;
static u32 g_fail_resolve_mask; /* bit d: the d-th resident resolve fails */
static u32 g_fail_bind_mask; /* bit d: the d-th live upload bind fails */
static u32 g_evict_on_bind[3]; /* residents cleared after bind d uploads */
static u32 g_resident[3]; /* 1: drawable d's epoch sits in the shared cache */
static u32 g_bind_calls;
static u32 g_resolves_before_binds; /* resolve count latched at first live bind */
static u32 g_binds_seen;
static u32 g_force_alpha_zero;
static u32 g_combine_w0, g_combine_w1, g_combine_calls;
static u32 g_setimage_calls, g_record_calls;
static u32 g_setimage_off[8];
static u32 g_loadtlut_calls, g_last_loadtlut;
static u32 g_resolve_calls, g_name_binds, g_apply_calls, g_last_params;
static u32 g_static_hits;
static NDSRendererHardwareTextureCacheEntry g_entries[3];
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
{ (void)s;(void)a; if (g_setimage_calls<8) g_setimage_off[g_setimage_calls]=b;
  g_setimage_calls++; g_record_calls++; }
static void ndsRendererRecordLoadBlock(NDSRendererStats *s,u32 a,u32 b)
{ (void)s;(void)a;(void)b; g_record_calls++; }
static void ndsRendererRecordLoadTlut(NDSRendererStats *s,u32 a)
{ (void)s; g_loadtlut_calls++; g_last_loadtlut=a; g_record_calls++; }
static void ndsRendererInitTraversalState(NDSRendererTraversalState *s,
    const NDSRendererConfig *c,NDSRendererStats *t,void *v,void *m,u32 n)
{ (void)s;(void)v;(void)m;(void)n; t->geometry_mode=c->initial_geometry_mode; }
static void ndsRendererHardwareEndBatch(void) {}
/* Pure resident lookup: FALSE (never an upload) on a miss or forced fail,
 * and the reject counter is the failure telemetry. Both preflight passes
 * run in drawable order, but attempt 0 may short-circuit, so the drawable
 * index is rebased at the first live bind rather than read off the raw
 * call count. */
static s32 ndsRendererHardwareResolveResidentTexture(NDSRendererStats *s,
    const NDSRendererConfig *c,NDSRendererTraversalState *t,
    NDSRendererHardwareResolvedTexture *r)
{
    u32 d;
    (void)c;(void)t;
    g_resolve_calls++;
    if (r == 0) { return 0; }
    memset(r,0,sizeof(*r));
    d = g_resolve_calls - 1u;
    if (g_binds_seen != 0u) { d -= g_resolves_before_binds; }
    if (d >= 3u)
    {
        s->hardware_texture_reject_count++;
        return 0;
    }
    if (((g_fail_resolve_mask & (1u << d)) != 0u) || (g_resident[d] == 0u))
    {
        s->hardware_texture_reject_count++;
        return 0;
    }
    r->entry=&g_entries[d]; r->name=g_entries[d].name;
    r->params=g_entries[d].params|0xa5u;
    r->format=8u+d; r->width=32u+d; r->height=64u+d;
    return 1;
}
/* Live upload bind: FALSE (with reject telemetry) on a forced fail, else
 * the epoch becomes resident and the per-bind eviction mask applies, so an
 * upload can drop a sibling exactly like a VRAM eviction would. Upload
 * passes run in drawable order, so the call index is the drawable index. */
static s32 ndsRendererHardwareBindTexture(NDSRendererStats *s,
    const NDSRendererConfig *c,NDSRendererTraversalState *t)
{
    u32 d;
    u32 bit;
    (void)c;(void)t;
    if (g_binds_seen == 0u)
    {
        g_binds_seen = 1u;
        g_resolves_before_binds = g_resolve_calls;
    }
    d = g_bind_calls;
    g_bind_calls++;
    if (d >= 3u)
    {
        s->hardware_texture_reject_count++;
        return 0;
    }
    if (g_fail_bind_mask & (1u << d))
    {
        s->hardware_texture_reject_count++;
        return 0;
    }
    g_resident[d] = 1u;
    for (bit = 0u; bit < 3u; bit++)
    {
        if (g_evict_on_bind[d] & (1u << bit)) { g_resident[bit] = 0u; }
    }
    if (s != 0)
    {
        s->hardware_texture_bind_count++;
        s->hardware_texture_ready_count++;
    }
    return 1;
}
static void ndsRendererHardwareBindTextureName(NDSRendererStats *s,u32 name)
{
    if (name == 0u) { return; }
    if (sNdsRendererHardwareBoundTextureName != name)
    {
        ndsRendererHardwareEndBatch();
        sNdsRendererHardwareBoundTextureName = name;
        g_name_binds++;
        if (s != 0) { s->hardware_texture_bind_count++; }
    }
}
static void ndsRendererHardwareApplyTextureParams(u32 p)
{ g_apply_calls++; g_last_params=p; }
static void ndsRendererHardwareRecordBattleStaticTextureHit(
    const NDSRendererHardwareTextureCacheEntry *e)
{ (void)e; g_static_hits++; }
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
{ if (g_load_count<8) { g_loaded_mvp[g_load_count]=*m; g_loaded_gen[g_load_count]=g; }
  g_load_count++; }
static void ndsRendererHardwareBeginTriangleBatch(NDSRendererStats *s,
    u32 t,u32 name,u32 p,u32 m,u32 g)
{ (void)s;(void)t;(void)name;(void)m;(void)g;(void)p; g_tri_batches++; }
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

static NDSRendererMatrix20p12 g_proj, g_cam, g_locals[6], g_mvps[3];
static u8 g_parents[6], g_bindings[6];
static NDSRendererStats g_stats;
static u8 g_packet[16384];
static void reset_log(void)
{
    u32 i;
    g_load_count=0; g_tri_batches=0; g_verts=0;
    g_last_poly_alpha=0; g_fail_resolve_mask=0;
    g_force_alpha_zero=0; g_combine_calls=0; g_setimage_calls=0;
    g_record_calls=0; g_loadtlut_calls=0; g_gen_counter=100;
    g_resolve_calls=0; g_name_binds=0; g_apply_calls=0; g_last_params=0;
    g_static_hits=0; g_bind_calls=0; g_binds_seen=0;
    g_resolves_before_binds=0; g_fail_bind_mask=0;
    g_evict_on_bind[0]=0; g_evict_on_bind[1]=0; g_evict_on_bind[2]=0;
    /* Warm default: every epoch resident, so the legacy tests exercise the
     * no-upload steady state. Cold tests clear g_resident after reset. */
    g_resident[0]=1u; g_resident[1]=1u; g_resident[2]=1u; sNdsRendererHardwareBoundTextureName=7;
    sNdsRendererHardwareSubmitted=0; sNdsRendererHardwareActiveTextureEntry=0;
    memset(&g_stats,0,sizeof(g_stats)); memset(g_loaded_gen,0,sizeof(g_loaded_gen));
    for (i=0;i<3u;i++) {
        g_entries[i].ready=1; g_entries[i].pinned=(i==0u)?1u:0u;
        g_entries[i].name=41u+i; g_entries[i].params=0x1000u*(i+1u);
        g_entries[i].last_used_frame=0u;
    }
}
static void build_hierarchy(void)
{
    static const u8 P[6]={31,0,1,2,1,1}, B[6]={0,1,2,3,4,5};
    u32 i,r,c;
    memcpy(g_parents,P,sizeof(P)); memcpy(g_bindings,B,sizeof(B));
    for (i=0;i<6u;i++) {
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
    h.roots=0; h.config=0; h.joint_count=6u; h.root_count=0u;
    return h;
}
int main(void)
{
    NDSRendererNativeFighterHierarchy h;
    sb32 ok;
    u32 i;
    /* solid: 6 tris, 3 distinct generations, 18 verts, MVP content forwarded */
    reset_log(); build_hierarchy(); h=make_hier();
    ok=ndsRendererSubmitNativeEfLakitu(g_packet,sizeof(g_packet),&h,g_mvps,
        NDS_RENDERER_GEOM_ZBUFFER,&g_stats);
    CHECK("solid", ok && g_stats.hardware_triangle_count==6u);
    CHECK("solid_gen", g_load_count==3u && g_loaded_gen[0]!=g_loaded_gen[1] &&
        g_loaded_gen[1]!=g_loaded_gen[2] && g_loaded_gen[0]!=g_loaded_gen[2]);
    CHECK("solid_verts", g_verts==18u);
    CHECK("solid_mvp", memcmp(&g_loaded_mvp[0],&g_mvps[0],sizeof(g_mvps[0]))==0 &&
        memcmp(&g_loaded_mvp[2],&g_mvps[2],sizeof(g_mvps[2]))==0);
    /* three-stage warm path: 3 pure resident resolves, then 3 name
     * rebinds. No live bind runs on a hit -- no allocation, no I/O. */
    CHECK("solid_preflight", g_resolve_calls==3u && g_name_binds==3u);
    CHECK("warm_no_upload", g_bind_calls==0u);
    CHECK("solid_binds", g_stats.hardware_texture_bind_count==3u &&
        g_stats.hardware_texture_ready_count==3u &&
        g_stats.hardware_texture_reject_count==0u);
    CHECK("solid_commit", g_stats.hardware_texture_format==10u &&
        g_stats.hardware_texture_width==34u &&
        g_stats.hardware_texture_height==66u);
    CHECK("solid_entry", g_entries[0].last_used_frame==
            sNdsRendererHardwareFrameSerial+1u &&
        g_entries[2].last_used_frame==sNdsRendererHardwareFrameSerial+1u &&
        g_entries[2].params==(0x3000u|0xa5u) &&
        sNdsRendererHardwareActiveTextureEntry==&g_entries[2]);
    CHECK("solid_params", g_apply_calls==3u && g_last_params==(0x3000u|0xa5u));
    CHECK("solid_pinned", g_static_hits==1u);
    /* three epochs recorded: palette TLUT x3, images = the three bank offsets */
    CHECK("tlut", g_loadtlut_calls==3u && g_last_loadtlut==0x0503c000u);
    CHECK("images", g_setimage_calls==6u &&
        g_setimage_off[1]==(u32)(uintptr_t)(g_packet+0x3da0u) &&
        g_setimage_off[3]==(u32)(uintptr_t)(g_packet+0x3b98u) &&
        g_setimage_off[5]==(u32)(uintptr_t)(g_packet+0x3710u));
    /* production setup words execute: combine + three palettes */
    CHECK("combine", g_combine_calls==1u &&
        g_combine_w0==0xfc127e24u && g_combine_w1==0xfffff3f9u);
    CHECK("records", g_record_calls>=20u);
    /* live pose change moves the loaded matrix bytes */
    reset_log(); build_hierarchy(); h=make_hier();
    g_locals[0].m[3][0]=12345;
    ok=ndsRendererSubmitNativeEfLakitu(g_packet,sizeof(g_packet),&h,g_mvps,
        NDS_RENDERER_GEOM_ZBUFFER,&g_stats);
    CHECK("pose_live", ok && g_stats.hardware_triangle_count==6u);
    /* production corner/run/uv ranges */
    { int good=1; for (i=0u;i<18u;i++)
        if (sNdsNativeActorEfLakituTriIndices[i]>=12u) { good=0; }
      CHECK("corners_range", good); }
    CHECK("corners_exact", sNdsNativeActorEfLakituTriIndices[0]==3u &&
        sNdsNativeActorEfLakituTriIndices[1]==2u &&
        sNdsNativeActorEfLakituTriIndices[2]==1u &&
        sNdsNativeActorEfLakituTriIndices[3]==0u &&
        sNdsNativeActorEfLakituTriIndices[4]==3u &&
        sNdsNativeActorEfLakituTriIndices[5]==1u &&
        sNdsNativeActorEfLakituTriIndices[6]==7u &&
        sNdsNativeActorEfLakituTriIndices[12]==11u &&
        sNdsNativeActorEfLakituTriIndices[17]==9u);
    CHECK("runs", sNdsNativeActorEfLakituRuns[0]==3u &&
        sNdsNativeActorEfLakituRuns[1]==0u &&
        sNdsNativeActorEfLakituRuns[2]==2u &&
        sNdsNativeActorEfLakituRuns[3]==4u &&
        sNdsNativeActorEfLakituRuns[4]==2u &&
        sNdsNativeActorEfLakituRuns[6]==5u &&
        sNdsNativeActorEfLakituRuns[7]==4u &&
        sNdsNativeActorEfLakituRuns[8]==2u);
    { int j, vok=1; for (j=0;j<12;j++) {
        s16 s=sNdsNativeActorEfLakituVerts[j*5+3];
        s16 t=sNdsNativeActorEfLakituVerts[j*5+4];
        if (s<0||s>4095||t<0||t>4095) { vok=0; } }
      CHECK("uv", vok); }
    CHECK("texepoch", sNdsNativeActorEfLakituTextureEpochs[0]==0x36e8u &&
        sNdsNativeActorEfLakituTextureEpochs[2]==0x3da0u &&
        sNdsNativeActorEfLakituTextureEpochs[3]==192u &&
        sNdsNativeActorEfLakituTextureEpochs[6]==0x3b98u &&
        sNdsNativeActorEfLakituTextureEpochs[10]==0x3710u &&
        sNdsNativeActorEfLakituTextureEpochs[11]==1160u);
    /* hardware resolving alpha to 0 skips every drawable, stays TRUE */
    reset_log(); build_hierarchy(); h=make_hier(); g_force_alpha_zero=1;
    ok=ndsRendererSubmitNativeEfLakitu(g_packet,sizeof(g_packet),&h,g_mvps,
        NDS_RENDERER_GEOM_ZBUFFER,&g_stats);
    CHECK("hwalpha0", ok && g_stats.hardware_triangle_count==0u &&
        g_load_count==0u && g_name_binds==0u && g_resolve_calls==3u);
    /* COLD FIRST DRAW: nothing resident. Attempt 0 misses d0, the bounded
     * upload pass makes all three resident, the pure reverify proves it,
     * and the actor draws fully native -- no generic fallback involved. */
    reset_log(); build_hierarchy(); h=make_hier();
    g_resident[0]=0u; g_resident[1]=0u; g_resident[2]=0u;
    ok=ndsRendererSubmitNativeEfLakitu(g_packet,sizeof(g_packet),&h,g_mvps,
        NDS_RENDERER_GEOM_ZBUFFER,&g_stats);
    CHECK("cold_ok", ok && g_stats.hardware_triangle_count==6u);
    CHECK("cold_verts", g_verts==18u && g_load_count==3u &&
        g_tri_batches==3u);
    CHECK("cold_phases", g_resolve_calls==4u && g_bind_calls==3u &&
        g_name_binds==3u);
    CHECK("cold_telemetry", g_stats.hardware_texture_reject_count==1u &&
        g_stats.hardware_texture_bind_count==6u &&
        g_stats.hardware_texture_ready_count==6u);
    CHECK("cold_commit", g_stats.hardware_texture_format==10u &&
        g_entries[2].last_used_frame==sNdsRendererHardwareFrameSerial+1u &&
        sNdsRendererHardwareActiveTextureEntry==&g_entries[2]);
    /* LIVE-BIND FAILURE at each drawable of a cold draw: FALSE with ZERO
     * GX emission -- no matrix load, no triangle batch, no vertex, no
     * name rebind, no submit flag. */
    for (i=0u;i<3u;i++)
    {
        reset_log(); build_hierarchy(); h=make_hier();
        g_resident[0]=0u; g_resident[1]=0u; g_resident[2]=0u;
        g_fail_bind_mask=1u<<i;
        ok=ndsRendererSubmitNativeEfLakitu(g_packet,sizeof(g_packet),&h,g_mvps,
            NDS_RENDERER_GEOM_ZBUFFER,&g_stats);
        CHECK("bindfail_ok", !ok);
        CHECK("bindfail_zero", g_verts==0u && g_tri_batches==0u &&
            g_load_count==0u && g_name_binds==0u);
        CHECK("bindfail_stats", g_stats.hardware_triangle_count==0u &&
            g_stats.hardware_zbuffer_triangle_count==0u &&
            g_stats.hardware_vertex_count==0u &&
            sNdsRendererHardwareSubmitted==0u);
        CHECK("bindfail_telemetry", g_resolve_calls==1u &&
            g_bind_calls==i+1u &&
            g_stats.hardware_texture_reject_count==2u &&
            g_stats.hardware_texture_ready_count==i);
    }
    /* EVICTION BETWEEN PREBINDS: every upload succeeds, but binding d1
     * drops d0 (then binding d2 drops d1). The pure reverify must catch
     * it: FALSE, zero emission, never a partial actor. Three binds alone
     * prove nothing. */
    reset_log(); build_hierarchy(); h=make_hier();
    g_resident[0]=0u; g_resident[1]=0u; g_resident[2]=0u;
    g_evict_on_bind[1]=1u<<0;
    ok=ndsRendererSubmitNativeEfLakitu(g_packet,sizeof(g_packet),&h,g_mvps,
        NDS_RENDERER_GEOM_ZBUFFER,&g_stats);
    CHECK("evict_ok", !ok);
    CHECK("evict_zero", g_verts==0u && g_tri_batches==0u &&
        g_load_count==0u && g_name_binds==0u &&
        sNdsRendererHardwareSubmitted==0u);
    CHECK("evict_telemetry", g_resolve_calls==2u && g_bind_calls==3u &&
        g_stats.hardware_texture_reject_count==2u);
    reset_log(); build_hierarchy(); h=make_hier();
    g_resident[0]=0u; g_resident[1]=0u; g_resident[2]=0u;
    g_evict_on_bind[2]=1u<<1;
    ok=ndsRendererSubmitNativeEfLakitu(g_packet,sizeof(g_packet),&h,g_mvps,
        NDS_RENDERER_GEOM_ZBUFFER,&g_stats);
    CHECK("evict2_ok", !ok);
    CHECK("evict2_zero", g_verts==0u && g_tri_batches==0u &&
        g_load_count==0u && g_name_binds==0u &&
        sNdsRendererHardwareSubmitted==0u);
    CHECK("evict2_telemetry", g_resolve_calls==3u && g_bind_calls==3u &&
        g_stats.hardware_texture_reject_count==2u);
    /* PARTIAL-EMISSION DEFECT GUARD: a resident resolve failing at drawable
     * 0, 1, or 2 must fail closed with ZERO GX emission -- no matrix load,
     * no triangle batch, no vertex, no name rebind, no submit flag, no
     * entry commit -- so the route's legacy fallback never duplicates
     * geometry and nothing draws partially. The masked drawable fails both
     * the warm attempt and the post-upload reverify, so each failing case
     * costs exactly two resolves, two rejects, and one bounded upload
     * pass that still emits nothing. */
    for (i=0u;i<3u;i++)
    {
        reset_log(); build_hierarchy(); h=make_hier();
        g_fail_resolve_mask=1u<<i;
        ok=ndsRendererSubmitNativeEfLakitu(g_packet,sizeof(g_packet),&h,g_mvps,
            NDS_RENDERER_GEOM_ZBUFFER,&g_stats);
        CHECK("fail_zero_ok", !ok);
        CHECK("fail_zero_verts", g_verts==0u && g_tri_batches==0u &&
            g_load_count==0u && g_name_binds==0u);
        CHECK("fail_zero_stats", g_stats.hardware_triangle_count==0u &&
            g_stats.hardware_zbuffer_triangle_count==0u &&
            g_stats.hardware_vertex_count==0u &&
            sNdsRendererHardwareSubmitted==0u);
        CHECK("fail_zero_telemetry", g_resolve_calls==2u*i+2u &&
            g_bind_calls==3u &&
            g_stats.hardware_texture_reject_count==2u &&
            g_stats.hardware_texture_bind_count==3u);
        CHECK("fail_zero_entry", g_entries[0].last_used_frame==0u &&
            g_entries[1].last_used_frame==0u &&
            g_entries[2].last_used_frame==0u);
    }
    /* fail-closed gates (all reject BEFORE the preflight resolves) */
    reset_log(); build_hierarchy(); h=make_hier(); h.joint_count=5u;
    ok=ndsRendererSubmitNativeEfLakitu(g_packet,sizeof(g_packet),&h,g_mvps,
        NDS_RENDERER_GEOM_ZBUFFER,&g_stats);
    CHECK("reject_joint", !ok && g_resolve_calls==0u);
    reset_log(); build_hierarchy(); h=make_hier(); g_parents[2]=0u;
    ok=ndsRendererSubmitNativeEfLakitu(g_packet,sizeof(g_packet),&h,g_mvps,
        NDS_RENDERER_GEOM_ZBUFFER,&g_stats);
    CHECK("reject_parent", !ok && g_resolve_calls==0u);
    reset_log(); build_hierarchy(); h=make_hier(); g_bindings[3]=0u;
    ok=ndsRendererSubmitNativeEfLakitu(g_packet,sizeof(g_packet),&h,g_mvps,
        NDS_RENDERER_GEOM_ZBUFFER,&g_stats);
    CHECK("reject_binding", !ok && g_resolve_calls==0u);
    reset_log(); build_hierarchy(); h=make_hier(); g_locals[4].m[3][3]=0;
    ok=ndsRendererSubmitNativeEfLakitu(g_packet,sizeof(g_packet),&h,g_mvps,
        NDS_RENDERER_GEOM_ZBUFFER,&g_stats);
    CHECK("reject_affine", !ok && g_resolve_calls==0u);
    reset_log(); build_hierarchy(); h=make_hier();
    ok=ndsRendererSubmitNativeEfLakitu(0,sizeof(g_packet),&h,g_mvps,
        NDS_RENDERER_GEOM_ZBUFFER,&g_stats);
    CHECK("reject_nullpacket", !ok && g_resolve_calls==0u);
    reset_log(); build_hierarchy(); h=make_hier();
    ok=ndsRendererSubmitNativeEfLakitu(g_packet,sizeof(g_packet),&h,0,
        NDS_RENDERER_GEOM_ZBUFFER,&g_stats);
    CHECK("reject_nullmvp", !ok && g_resolve_calls==0u);
    reset_log(); build_hierarchy(); h=make_hier();
    ok=ndsRendererSubmitNativeEfLakitu(g_packet,0x3710u,&h,g_mvps,
        NDS_RENDERER_GEOM_ZBUFFER,&g_stats);
    CHECK("reject_short", !ok && g_resolve_calls==0u);
    reset_log(); build_hierarchy(); h=make_hier();
    ok=ndsRendererSubmitNativeEfLakitu(g_packet,sizeof(g_packet),&h,g_mvps,
        0u,&g_stats);
    CHECK("reject_noz", !ok && g_resolve_calls==0u);
    if (g_failures==0) { printf("LAKITU-HOST-OK\n"); }
    return g_failures ? 1 : 0;
}
