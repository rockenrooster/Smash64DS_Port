#ifndef SSB64_NDS_GBI_H
#define SSB64_NDS_GBI_H

#include <stdint.h>

#include <PR/mbi.h>
#include <PR/ultratypes.h>

/* Gfx is an N64 display-list word pair. The imported task/object/lb-common
 * code accesses dl->words.w0 / .w1 directly (taskman.c) and via the gSP and
 * gDP macros, so Gfx must expose a .words union with two u32 halves plus a
 * 64-bit alignment member. On the DS there is no RSP to consume these words,
 * so the gSP and gDP macros are stubbed to no-ops; only the layout needs to
 * match. */
typedef union
{
    struct
    {
        u32 w0;
        u32 w1;
    } words;
    long long int force_structure_alignment;
} Gfx;

typedef struct Lights1 {
    u8 ambient[3];
    u8 diffuse[3];
    s8 direction[3];
} Lights1;

/* Viewport types, matching the N64 PR/gbi.h ABI exactly. The imported
 * rdp/video/object code references Vp (the aligned union used by CObj.viewport
 * and gSYRdpViewport) and accesses its inner Vp_t.vscale/vtrans arrays. */
typedef struct {
    short vscale[4];
    short vtrans[4];
} Vp_t;

typedef union {
    Vp_t vp;
    long long int force_structure_alignment;
} Vp;

/* 4x4 fixed-point matrix (s15.16), matching PR/gbi.h. Used by XObj.mtx and the
 * transform/object matrix pipeline. */
typedef long Mtx_t[4][4];

typedef union {
    Mtx_t m;
    long long int force_structure_alignment;
} Mtx;

#define G_ZBUFFER         0x00000001u
#define G_SHADE           0x00000004u
#define G_CULL_BACK       0x00002000u
#define G_LIGHTING        0x00020000u
#define G_SHADING_SMOOTH  0x00200000u

/* Color-combiner symbolic inputs used by imported scene display callbacks.
 * These only need stable integer values while the DS display-list writer is
 * stubbed; the renderer adapter decodes real packed commands from O2R assets. */
#define COMBINED     0
#define TEXEL0       1
#define TEXEL1       2
#define PRIMITIVE    3
#define SHADE        4
#define ENVIRONMENT  5
#define CENTER       6
#define SCALE        6
#define COMBINED_ALPHA 7
#define TEXEL0_ALPHA 8
#define TEXEL1_ALPHA 9
#define PRIMITIVE_ALPHA 10
#define SHADE_ALPHA 11
#define ENV_ALPHA    12
#define LOD_FRACTION 13
#define PRIM_LOD_FRAC 14
#define NOISE        7
#define K4           7
#define K5           15
#define ONE          6
#define ZERO         31

#define G_CYC_1CYCLE 0u
#define G_RM_AA_XLU_SURF 0u
#define G_RM_AA_XLU_SURF2 0u
#define G_RM_AA_ZB_OPA_SURF 0u
#define G_RM_AA_ZB_OPA_SURF2 0u
#define G_RM_AA_ZB_XLU_SURF 0u
#define G_RM_AA_ZB_XLU_SURF2 0u
#define G_TP_PERSP 0u
#define G_ZS_PIXEL 0u

/* Segment base register indexes / move-word targets used by the task manager's
 * segment setup (gSPSegment). The DS has no segment registers, but the macros
 * must compile. */
#define G_MW_SEGMENT 0x06
#define G_MWO_SEGMENT_F 0x0f

#define gdSPDefLights1(ar, ag, ab, dr, dg, db, x, y, z) \
    { { (ar), (ag), (ab) }, { (dr), (dg), (db) }, { (x), (y), (z) } }

#define NDS_GBI_PACKET_IN_MAIN_RAM(pkt) \
    (((uintptr_t)(pkt) >= 0x02000000u) && ((uintptr_t)(pkt) < 0x02400000u))

#define NDS_GBI_ZERO_PACKET(pkt) do { \
    Gfx *_nds_gbi_pkt = (Gfx *)(pkt); \
    if (NDS_GBI_PACKET_IN_MAIN_RAM(_nds_gbi_pkt)) { \
        _nds_gbi_pkt->words.w0 = 0; \
        _nds_gbi_pkt->words.w1 = 0; \
    } \
} while (0)

/* Static (initializer) DL macros produce zero display-list words. */
#define gsSPSetGeometryMode(mode) { 0 }
#define gsSPSetLights1(light)     { 0 }
#define gsSPEndDisplayList()      { 0 }

/* Runtime DL macros operate on a Gfx* cursor that the caller advances (e.g.
 * gSPDisplayList(dls[0]++, dl)). The DS has no RSP, so these stubs only zero
 * the current word pair; they are reached only inside the parked task loop or
 * via display callbacks, never during the startup setup this increment proves. */
#define gSPDisplayList(pkt, dl) do { \
    NDS_GBI_ZERO_PACKET(pkt); \
    (void)(dl); \
} while (0)

#define gSPSegment(pkt, segment, base) do { \
    NDS_GBI_ZERO_PACKET(pkt); \
    (void)(segment); (void)(base); \
} while (0)

#define gSPEndDisplayList(pkt) do { \
    NDS_GBI_ZERO_PACKET(pkt); \
} while (0)

/* RSP ucode load. The real macro emits a load-ucode display-list command; the
 * DS has no RSP, so it is stubbed to a zeroed DL word. Reached only inside the
 * parked task loop, never during the startup setup this increment proves. */
#define gSPLoadUcodeL(pkt, ucode) do { \
    NDS_GBI_ZERO_PACKET(pkt); \
} while (0)

/* Additional DL macros reached only inside the parked task loop. Stubbed to
 * zero the current DL word pair. */
#define gSPBranchList(pkt, dl) do { \
    NDS_GBI_ZERO_PACKET(pkt); \
    (void)(dl); \
} while (0)

#define gSPSetGeometryMode(pkt, mode) do { \
    NDS_GBI_ZERO_PACKET(pkt); \
    (void)(mode); \
} while (0)

#define gDPFullSync(pkt) do { \
    NDS_GBI_ZERO_PACKET(pkt); \
} while (0)

#define gDPPipeSync(pkt) do { \
    NDS_GBI_ZERO_PACKET(pkt); \
} while (0)

#define gDPFillRectangle(pkt, ulx, uly, lrx, lry) do { \
    NDS_GBI_ZERO_PACKET(pkt); \
    (void)(ulx); (void)(uly); (void)(lrx); (void)(lry); \
} while (0)

#define gDPSetCycleType(pkt, cycle) do { \
    NDS_GBI_ZERO_PACKET(pkt); \
    (void)(cycle); \
} while (0)

#define gDPSetPrimColor(pkt, m, l, r, g, b, a) do { \
    NDS_GBI_ZERO_PACKET(pkt); \
    (void)(m); (void)(l); (void)(r); (void)(g); (void)(b); (void)(a); \
} while (0)

#define gDPSetCombineLERP(pkt, a0, b0, c0, d0, Aa0, Ab0, Ac0, Ad0, \
                          a1, b1, c1, d1, Aa1, Ab1, Ac1, Ad1) do { \
    NDS_GBI_ZERO_PACKET(pkt); \
    (void)(a0); (void)(b0); (void)(c0); (void)(d0); \
    (void)(Aa0); (void)(Ab0); (void)(Ac0); (void)(Ad0); \
    (void)(a1); (void)(b1); (void)(c1); (void)(d1); \
    (void)(Aa1); (void)(Ab1); (void)(Ac1); (void)(Ad1); \
} while (0)

#define gDPSetRenderMode(pkt, mode1, mode2) do { \
    NDS_GBI_ZERO_PACKET(pkt); \
    (void)(mode1); (void)(mode2); \
} while (0)

#define gDPSetTexturePersp(pkt, type) do { \
    NDS_GBI_ZERO_PACKET(pkt); \
    (void)(type); \
} while (0)

#define gDPSetDepthSource(pkt, src) do { \
    NDS_GBI_ZERO_PACKET(pkt); \
    (void)(src); \
} while (0)

#endif
