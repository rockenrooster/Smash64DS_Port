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

typedef struct {
    u8 col[3];
    u8 pad1;
    u8 colc[3];
    u8 pad2;
    s8 dir[3];
    u8 pad3;
} Light_t;

typedef union {
    Light_t l;
    long long int force_structure_alignment;
} Light;

typedef struct {
    Light l[2];
} LookAt;

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

/* Vertex, matching the N64 PR/gbi.h ABI exactly (decomp
 * BattleShip-main/decomp/include/PR/gbi.h:1099-1121). The credits glyph-DL
 * builder (scstaffroll.c:2053) and the fighter display path address Vtx
 * directly; gSPVertex below carries a Vtx* in w1. */
typedef struct {
    short ob[3];
    unsigned short flag;
    short tc[2];
    unsigned char cn[4];
} Vtx_t;

typedef struct {
    short ob[3];
    unsigned short flag;
    short tc[2];
    signed char n[3];
    unsigned char a;
} Vtx_tn;

typedef union {
    Vtx_t v;
    Vtx_tn n;
    long long int force_structure_alignment;
} Vtx;

#define G_ZBUFFER         0x00000001u
#define G_SHADE           0x00000004u
#define G_CULL_BACK       0x00002000u
#define G_LIGHTING        0x00020000u
#define G_SHADING_SMOOTH  0x00200000u

/* Color-combiner argument tokens.
 *
 * The SDK does NOT define bare COMBINED/TEXEL0/... values: source spells
 * them only inside gDPSetCombineLERP argument lists and G_CC_* token
 * lists, and the LERP macro pastes each argument onto G_CCMUX_ / G_ACMUX_.
 * A previous revision of this header defined them as numbers (COMBINED 0,
 * TEXEL0 1, ...); that broke the G_CC_* path, because a G_CC_* list
 * rescanned through gDPSetCombineMode expanded its tokens to numbers
 * BEFORE the LERP paste, producing G_CCMUX_3 instead of G_CCMUX_PRIMITIVE.
 * Do not reintroduce numeric aliases here: a literal 0 argument already
 * encodes mux ZERO via G_CCMUX_0 (31) / G_ACMUX_0 (7), which is distinct
 * from COMBINED. */

/* Color-combiner mux inputs, matching the N64 PR/gbi.h G_CCMUX_* values
 * exactly (decomp BattleShip-main/decomp/include/PR/gbi.h:473-493). These
 * are what gDPSetCombineLERP pastes its arguments onto. */
#define G_CCMUX_COMBINED 0
#define G_CCMUX_TEXEL0 1
#define G_CCMUX_TEXEL1 2
#define G_CCMUX_PRIMITIVE 3
#define G_CCMUX_SHADE 4
#define G_CCMUX_ENVIRONMENT 5
#define G_CCMUX_CENTER 6
#define G_CCMUX_SCALE 6
#define G_CCMUX_COMBINED_ALPHA 7
#define G_CCMUX_TEXEL0_ALPHA 8
#define G_CCMUX_TEXEL1_ALPHA 9
#define G_CCMUX_PRIMITIVE_ALPHA 10
#define G_CCMUX_SHADE_ALPHA 11
#define G_CCMUX_ENV_ALPHA 12
#define G_CCMUX_LOD_FRACTION 13
#define G_CCMUX_PRIM_LOD_FRAC 14
#define G_CCMUX_NOISE 7
#define G_CCMUX_K4 7
#define G_CCMUX_K5 15
#define G_CCMUX_1 6
#define G_CCMUX_0 31

/* Alpha-combiner mux inputs, matching PR/gbi.h G_ACMUX_* exactly (:496-505). */
#define G_ACMUX_COMBINED 0
#define G_ACMUX_TEXEL0 1
#define G_ACMUX_TEXEL1 2
#define G_ACMUX_PRIMITIVE 3
#define G_ACMUX_SHADE 4
#define G_ACMUX_ENVIRONMENT 5
#define G_ACMUX_LOD_FRACTION 0
#define G_ACMUX_PRIM_LOD_FRAC 6
#define G_ACMUX_1 6
#define G_ACMUX_0 7

/* Combine-word helpers, matching the SDK GCCc0w0/GCCc1w0/GCCc0w1/GCCc1w1
 * field layout exactly (:3088-3102). Port-prefixed so this header never
 * collides with a co-included SDK header. */
#define SSB64_NDS_GCC_C0W0(saRGB0, mRGB0, saA0, mA0) \
    (((((u32)(saRGB0)) & 0xfu) << 20) | ((((u32)(mRGB0)) & 0x1fu) << 15) | \
     ((((u32)(saA0)) & 0x7u) << 12) | ((((u32)(mA0)) & 0x7u) << 9))
#define SSB64_NDS_GCC_C1W0(saRGB1, mRGB1) \
    (((((u32)(saRGB1)) & 0xfu) << 5) | (((u32)(mRGB1)) & 0x1fu))
#define SSB64_NDS_GCC_C0W1(sbRGB0, aRGB0, sbA0, aA0) \
    (((((u32)(sbRGB0)) & 0xfu) << 28) | ((((u32)(aRGB0)) & 0x7u) << 15) | \
     ((((u32)(sbA0)) & 0x7u) << 12) | ((((u32)(aA0)) & 0x7u) << 9))
#define SSB64_NDS_GCC_C1W1(sbRGB1, saA1, mA1, aRGB1, sbA1, aA1) \
    (((((u32)(sbRGB1)) & 0xfu) << 24) | ((((u32)(saA1)) & 0x7u) << 21) | \
     ((((u32)(mA1)) & 0x7u) << 18) | ((((u32)(aRGB1)) & 0x7u) << 6) | \
     ((((u32)(sbA1)) & 0x7u) << 3) | (((u32)(aA1)) & 0x7u))

/* F3DEX2 packet opcodes, matching PR/gbi.h (G_VTX/G_TRI1/G_TRI2 0x01/05/06,
 * G_TEXTURE 0xd7, G_SETCOMBINE 0xfc, G_SETFILLCOLOR 0xf7, G_FILLRECT 0xf6,
 * G_SETOTHERMODE_H/L 0xe3/0xe2). The DS renderer decodes exactly these
 * (nds_renderer_preamble.c:1774); anything else falls into its unsupported
 * counter, never an abort. */
#define G_VTX 0x01u
#define G_TRI1 0x05u
#define G_TRI2 0x06u
#define G_TEXTURE 0xd7u
#define G_SETCOMBINE 0xfcu
#define G_SETFILLCOLOR 0xf7u
#define G_FILLRECT 0xf6u
#define G_SETOTHERMODE_H 0xe3u
#define G_SETOTHERMODE_L 0xe2u
#define G_MDSFT_CYCLETYPE 20u

/* GBI switch, matching PR/mbi.h. Guarded: mbi.h is the canonical home and
 * out of scope here, so a future mbi.h definition wins without a clash. */
#ifndef G_ON
#define G_ON 1
#endif

/* RGBA8888 to RGBA5551, matching PR/gbi.h GCONVERT5551_RGBA8888 exactly
 * (:302-304). Used by 1P data tables (mnsoundtest.c:1094). */
#define GCONVERT5551_RGBA8888(rgba8888) \
    (((((u32)(rgba8888)) >> 16) & 0xf800u) | \
     ((((u32)(rgba8888)) >> 13) & 0x7c0u) | \
     ((((u32)(rgba8888)) >> 10) & 0x3eu) | \
     ((((u32)(rgba8888)) >> 7) & 0x1u))

/* Cycle-type words are (n << G_MDSFT_CYCLETYPE); G_CYC_1CYCLE is 0.
 * G_CYC_FILL was 0u here, which aliased 1CYCLE; the credits highlight and
 * the wallpaper FILL lists need the real word. */
#define G_CYC_1CYCLE 0u
#define G_CYC_2CYCLE 0x00100000u
#define G_CYC_COPY 0x00200000u
#define G_CYC_FILL 0x00300000u

/* THE RENDER MODES CARRY THEIR REAL othermode_l WORDS.
 *
 * These were all 0u, which made gDPSetRenderMode unfixable on its own: even
 * with the macro un-stubbed, `mode1 | mode2` would have been zero and the
 * 29-bit rendermode field of othermode_l would have been CLEARED rather than
 * set. Both halves of the defect had to be repaired together.
 *
 * Every value below is the ordinary F3DEX2 encoding, taken from the authority
 * rather than transcribed: they are the preprocessor's own expansion of
 * decomp/BattleShip-main/decomp/include/PR/gbi.h (RM_AA_ZB_XLU_SURF(1) and
 * friends), so they cannot drift from the source the effect procs were
 * written against.
 *
 * What they buy: ndsRendererHardwareAlpha (nds_renderer.c:8295) returns a
 * hardcoded alpha 31 -- fully opaque -- unless othermode_l shows one of
 * ZMODE_XLU (0x800), FORCE_BL (0x4000), CVG_X_ALPHA (0x1000), or alpha
 * compare == threshold. G_RM_AA_ZB_XLU_SURF sets all three of the first
 * group; G_RM_CLD_SURF sets FORCE_BL. G_RM_AA_ZB_OPA_SURF sets none of them,
 * so opaque geometry keeps the fast opaque path exactly as before. */
#define G_RM_AA_OPA_SURF 0x00442048u
#define G_RM_AA_OPA_SURF2 0x00112048u
#define G_RM_AA_XLU_SURF 0x004041c8u
#define G_RM_AA_XLU_SURF2 0x001041c8u
#define G_RM_AA_ZB_OPA_SURF 0x00442078u
#define G_RM_AA_ZB_OPA_SURF2 0x00112078u
#define G_RM_AA_ZB_XLU_SURF 0x004049d8u
#define G_RM_AA_ZB_XLU_SURF2 0x001049d8u
/* decomp include/PR/gbi.h:773-776 against :718-721: RM_AA_ZB_TEX_EDGE is
 * RM_AA_ZB_OPA_SURF term for term, plus CVG_X_ALPHA (0x1000, :696) and
 * TEX_EDGE -- and TEX_EDGE is 0 in this gbi (:699 says so, noting it used to
 * be 0x8000). So the pair above OR'd with 0x1000, and nothing else. Kabigon
 * draws through it: an alpha-tested cutout surface, which is exactly what the
 * CVG_X_ALPHA bit tells ndsRendererHardwareAlpha to stop treating as opaque. */
#define G_RM_AA_ZB_TEX_EDGE 0x00443078u
#define G_RM_AA_ZB_TEX_EDGE2 0x00113078u
#define G_RM_CLD_SURF 0x00404340u
#define G_RM_CLD_SURF2 0x00104340u
#define G_RM_OPA_SURF 0x0c084000u
#define G_RM_OPA_SURF2 0x03024000u
#define G_RM_XLU_SURF 0x00404240u
#define G_RM_XLU_SURF2 0x00104240u
#define G_RM_NOOP 0u
#define G_RM_NOOP2 0u
/* GBL_c1(G_BL_CLR_IN, G_BL_0, G_BL_CLR_IN, G_BL_1): the 1-cycle pass-through
 * the 1P item/fighter/boss display procs pair with G_CYC_2CYCLE
 * (itdisplay.c:242, ftdisplaymain.c:693, sc1pintro.c:946). */
#define G_RM_PASS 0x0c080000u
#define G_AC_NONE 0u
#define G_AC_THRESHOLD 1u
#define G_TP_PERSP 0u
#define G_TP_NONE 0u
#define G_ZS_PIXEL 0u
/* gDPSetDepthSource discards its argument, so like every other mode bit in this
 * header this carries no value -- it exists so the source compiles. */
#define G_ZS_PRIM 0u
#define G_TX_NOMIRROR 0u
#define G_TX_WRAP 0u
#define G_TX_MIRROR 0x1u
#define G_TX_CLAMP 0x2u
#define G_TX_NOMASK 0u
#define G_TX_NOLOD 0u
#define G_TX_LOADTILE 0u
#define G_TX_RENDERTILE 0u
/* Typical CC cycle-1 token lists, matching PR/gbi.h exactly (:508-521).
 * These are EIGHT-TOKEN lists, not values: gDPSetCombineMode rescans them
 * into gDPSetCombineLERP's sixteen parameters (SDK :3140-3151). */
#define G_CC_PRIMITIVE 0, 0, 0, PRIMITIVE, 0, 0, 0, PRIMITIVE
#define G_CC_SHADE 0, 0, 0, SHADE, 0, 0, 0, SHADE
#define G_CC_MODULATEI TEXEL0, 0, SHADE, 0, 0, 0, 0, SHADE
#define G_CC_DECALRGBA 0, 0, 0, TEXEL0, 0, 0, 0, TEXEL0
#define G_CC_BLENDPEDECALA PRIMITIVE, ENVIRONMENT, TEXEL0, ENVIRONMENT, 0, 0, 0, TEXEL0
#define G_CC_MODULATEI_PRIM TEXEL0, 0, PRIMITIVE, 0, 0, 0, 0, PRIMITIVE
#define G_CC_MODULATEIA_PRIM TEXEL0, 0, PRIMITIVE, 0, TEXEL0, 0, PRIMITIVE, 0
#define G_AC_DITHER 3u
#define G_CD_MAGICSQ 0u
#define G_AD_PATTERN 0u
#define G_TT_NONE 0u
#define G_TT_RGBA16 0u
#define G_MTX_MODELVIEW 0x00u
#define G_MTX_PROJECTION 0x01u
#define G_MTX_MUL 0x00u
#define G_MTX_LOAD 0x02u
#define G_MTX_NOPUSH 0x00u
#define G_MTX_PUSH 0x04u
#define G_SC_NON_INTERLACE 0u

#define GPACK_FILL16(w) (((w) << 16) | ((w) << 0))

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

/* Static (initializer) DL macros produce source-exact F3DEX2 word pairs. */
#define gsSPSetGeometryMode(mode)       { 0 }
#define gsSPClearGeometryMode(mode)     { 0 }
#define gsSPSetLights1(light)           { 0 }
#define gsDPPipeSync()                  { 0 }
#define gsDPSetRenderMode(mode1, mode2) { 0 }
#define gsDPSetAlphaCompare(type)       { 0 }
#define gsDPSetBlendColor(r, g, b, a)   { 0 }
#define gsDPSetPrimColor(m, l, r, g, b, a) { 0 }
#define gsDPSetCombineMode(mode1, mode2) { 0 }
#define gsSPEndDisplayList()            { 0 }
/* G_SETOTHERMODE_H, sft=G_MDSFT_CYCLETYPE(20), len=2:
 * w0 = 0xe3 | ((32-20-2)<<8) | (2-1). Carries the 1P wallpaper FILL list
 * (grwallpaper.c) and the credits text-box list (scstaffroll.c:479). */
#define gsDPSetCycleType(type)          { 0xe3000a01u, (u32)(type) }
/* G_SETFILLCOLOR word pair. */
#define gsDPSetFillColor(d)             { 0xf7000000u, (u32)(d) }
/* G_FILLRECT word pair: w0 carries lrx/lry, w1 carries ulx/uly. */
#define gsDPFillRectangle(ulx, uly, lrx, lry) \
    { (0xf6u << 24) | ((((u32)(lrx)) & 0x3ffu) << 14) | ((((u32)(lry)) & 0x3ffu) << 2), \
      ((((u32)(ulx)) & 0x3ffu) << 14) | ((((u32)(uly)) & 0x3ffu) << 2) }

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

#define gSPMatrix(pkt, mtx, params) do { \
    NDS_GBI_ZERO_PACKET(pkt); \
    (void)(mtx); (void)(params); \
} while (0)

#define gSPViewport(pkt, vp) do { \
    NDS_GBI_ZERO_PACKET(pkt); \
    (void)(vp); \
} while (0)

#define gSPLookAtX(pkt, lookat) do { \
    NDS_GBI_ZERO_PACKET(pkt); \
    (void)(lookat); \
} while (0)

#define gSPLookAtY(pkt, lookat) do { \
    NDS_GBI_ZERO_PACKET(pkt); \
    (void)(lookat); \
} while (0)

#define gSPPerspNormalize(pkt, norm) do { \
    NDS_GBI_ZERO_PACKET(pkt); \
    (void)(norm); \
} while (0)

#define gSPSetGeometryMode(pkt, mode) do { \
    NDS_GBI_ZERO_PACKET(pkt); \
    (void)(mode); \
} while (0)

#define gSPClearGeometryMode(pkt, mode) do { \
    NDS_GBI_ZERO_PACKET(pkt); \
    (void)(mode); \
} while (0)

/* F3DEX2 triangle index packing (SDK __gsSP1Triangle_w1f with F3DEX indices:
 * each index doubled, flag rotating the first vertex). The renderer decodes
 * exactly this doubling (ndsGBIDecodePackedTriIndices divides by two). */
#define SSB64_NDS_GBI_TRI_W1F(v0, v1, v2, flag) \
    (((flag) == 0) ? \
        (((((u32)(v0)) * 2u & 0xffu) << 16) | ((((u32)(v1)) * 2u & 0xffu) << 8) | \
         (((u32)(v2)) * 2u & 0xffu)) : \
     ((flag) == 1) ? \
        (((((u32)(v1)) * 2u & 0xffu) << 16) | ((((u32)(v2)) * 2u & 0xffu) << 8) | \
         (((u32)(v0)) * 2u & 0xffu)) : \
        (((((u32)(v2)) * 2u & 0xffu) << 16) | ((((u32)(v0)) * 2u & 0xffu) << 8) | \
         (((u32)(v1)) * 2u & 0xffu)))

/* F3DEX2 G_VTX: w0 carries count and first+count, w1 carries the Vtx
 * address (SDK :1802-1808). Credits glyph DLs load 4 verts at 0
 * (scstaffroll.c:2098). pkt is evaluated once. */
#define gSPVertex(pkt, v, n, v0) do { \
    Gfx *_nds_gbi_pkt = (Gfx *)(pkt); \
    if (NDS_GBI_PACKET_IN_MAIN_RAM(_nds_gbi_pkt)) { \
        _nds_gbi_pkt->words.w0 = (0x01u << 24) | \
            ((((u32)(n)) & 0xffu) << 12) | \
            ((((u32)(v0) + (u32)(n)) & 0x7fu) << 1); \
        _nds_gbi_pkt->words.w1 = (u32)(uintptr_t)(v); \
    } \
} while (0)

/* F3DEX2 G_TRI2 pair (SDK :2170-2177): w0 carries the first triangle,
 * w1 the second. Credits quads use (3,2,1,0, 0,3,1,0) (:2099). */
#define gSP2Triangles(pkt, v00, v01, v02, flag0, v10, v11, v12, flag1) do { \
    Gfx *_nds_gbi_pkt = (Gfx *)(pkt); \
    if (NDS_GBI_PACKET_IN_MAIN_RAM(_nds_gbi_pkt)) { \
        _nds_gbi_pkt->words.w0 = (0x06u << 24) | \
            SSB64_NDS_GBI_TRI_W1F(v00, v01, v02, flag0); \
        _nds_gbi_pkt->words.w1 = SSB64_NDS_GBI_TRI_W1F(v10, v11, v12, flag1); \
    } \
} while (0)

/* F3DEX2 G_TEXTURE (SDK :2761-2770; BOWTIE_VAL is 0): the credits display
 * procs enable texturing with (0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON)
 * (:1503, :1517). Note the F3DEX2 on-shift is 1, not 0. */
#define gSPTexture(pkt, s, t, level, tile, on) do { \
    Gfx *_nds_gbi_pkt = (Gfx *)(pkt); \
    if (NDS_GBI_PACKET_IN_MAIN_RAM(_nds_gbi_pkt)) { \
        _nds_gbi_pkt->words.w0 = (0xd7u << 24) | \
            ((((u32)(level)) & 0x7u) << 11) | ((((u32)(tile)) & 0x7u) << 8) | \
            ((((u32)(on)) & 0x7fu) << 1); \
        _nds_gbi_pkt->words.w1 = ((((u32)(s)) & 0xffffu) << 16) | \
            (((u32)(t)) & 0xffffu); \
    } \
} while (0)

#define gDPFullSync(pkt) do { \
    NDS_GBI_ZERO_PACKET(pkt); \
} while (0)

#define gDPPipeSync(pkt) do { \
    NDS_GBI_ZERO_PACKET(pkt); \
} while (0)

#define gDPLoadSync(pkt) do { \
    NDS_GBI_ZERO_PACKET(pkt); \
} while (0)

#define gDPFillRectangle(pkt, ulx, uly, lrx, lry) do { \
    Gfx *_nds_gbi_pkt = (Gfx *)(pkt); \
    if (NDS_GBI_PACKET_IN_MAIN_RAM(_nds_gbi_pkt)) { \
        _nds_gbi_pkt->words.w0 = (0xf6u << 24) | \
            ((((u32)(lrx)) & 0x3ffu) << 14) | ((((u32)(lry)) & 0x3ffu) << 2); \
        _nds_gbi_pkt->words.w1 = ((((u32)(ulx)) & 0x3ffu) << 14) | \
            ((((u32)(uly)) & 0x3ffu) << 2); \
    } \
} while (0)

#define gDPSetScissor(pkt, mode, ulx, uly, lrx, lry) do { \
    NDS_GBI_ZERO_PACKET(pkt); \
    (void)(mode); (void)(ulx); (void)(uly); (void)(lrx); (void)(lry); \
} while (0)

/* G_SETOTHERMODE_H cycle-type write (see gsDPSetCycleType for the header).
 * w1 is the G_CYC_* word. pkt is evaluated once into the cursor; the
 * main-RAM guard keeps arena-bounds accounting identical to the stubs. */
#define gDPSetCycleType(pkt, cycle) do { \
    Gfx *_nds_gbi_pkt = (Gfx *)(pkt); \
    if (NDS_GBI_PACKET_IN_MAIN_RAM(_nds_gbi_pkt)) { \
        _nds_gbi_pkt->words.w0 = 0xe3000a01u; \
        _nds_gbi_pkt->words.w1 = (u32)(cycle); \
    } \
} while (0)

/* THESE TWO CARRY THEIR WORDS; every other gDP macro here still zeroes.
 *
 * A source proc_display expresses an effect's colour by emitting exactly these
 * two commands into gSYTaskmanDLHeads[] immediately before drawing the model --
 * efManagerShieldProcDisplay writes the per-player dEFManagerShieldColors entry
 * and alpha 0xC0 that way (efmanager.c:80101024). Zeroing them threw that away
 * at the macro, so every source effect inherited whatever prim/env the previous
 * list left in the renderer's persistent RDP state, which is why the shield
 * bubble drew in the stage's dark green rather than translucent white.
 *
 * The words are the ordinary F3DEX2 encodings, so the renderer's existing
 * NDS_RENDERER_OP_SETPRIMCOLOR / _SETENVCOLOR cases consume them unchanged --
 * no new state, no per-effect code. A zeroed packet decodes as opcode 0x00 and
 * was simply ignored, so nothing that previously worked can start failing;
 * a consumer that never reads the span still sees the same packet count and
 * the same buffer accounting. */
#define gDPSetPrimColor(pkt, m, l, r, g, b, a) do { \
    Gfx *_nds_gbi_pkt = (Gfx *)(pkt); \
    if (NDS_GBI_PACKET_IN_MAIN_RAM(_nds_gbi_pkt)) { \
        _nds_gbi_pkt->words.w0 = (0xfau << 24) | \
            (((u32)(m) & 0xffu) << 8) | ((u32)(l) & 0xffu); \
        _nds_gbi_pkt->words.w1 = (((u32)(r) & 0xffu) << 24) | \
            (((u32)(g) & 0xffu) << 16) | (((u32)(b) & 0xffu) << 8) | \
            ((u32)(a) & 0xffu); \
    } \
} while (0)

#define gDPSetFillColor(pkt, color) do { \
    Gfx *_nds_gbi_pkt = (Gfx *)(pkt); \
    if (NDS_GBI_PACKET_IN_MAIN_RAM(_nds_gbi_pkt)) { \
        _nds_gbi_pkt->words.w0 = (0xf7u << 24); \
        _nds_gbi_pkt->words.w1 = (u32)(color); \
    } \
} while (0)

#define gDPSetBlendColor(pkt, r, g, b, a) do { \
    NDS_GBI_ZERO_PACKET(pkt); \
    (void)(r); (void)(g); (void)(b); (void)(a); \
} while (0)

#define gDPSetEnvColor(pkt, r, g, b, a) do { \
    Gfx *_nds_gbi_pkt = (Gfx *)(pkt); \
    if (NDS_GBI_PACKET_IN_MAIN_RAM(_nds_gbi_pkt)) { \
        _nds_gbi_pkt->words.w0 = (0xfbu << 24); \
        _nds_gbi_pkt->words.w1 = (((u32)(r) & 0xffu) << 24) | \
            (((u32)(g) & 0xffu) << 16) | (((u32)(b) & 0xffu) << 8) | \
            ((u32)(a) & 0xffu); \
    } \
} while (0)

/* G_SETCOMBINE word pair, matching the SDK gDPSetCombineLERP expansion
 * exactly (decomp :3104-3124): each argument is pasted onto G_CCMUX_ /
 * G_ACMUX_, never interpreted numerically, so literal 0 encodes mux ZERO
 * (G_CCMUX_0 = 31, G_ACMUX_0 = 7). pkt is evaluated once. */
#define gDPSetCombineLERP(pkt, a0, b0, c0, d0, Aa0, Ab0, Ac0, Ad0, \
                           a1, b1, c1, d1, Aa1, Ab1, Ac1, Ad1) do { \
    Gfx *_nds_gbi_pkt = (Gfx *)(pkt); \
    if (NDS_GBI_PACKET_IN_MAIN_RAM(_nds_gbi_pkt)) { \
        _nds_gbi_pkt->words.w0 = (0xfcu << 24) | \
            ((SSB64_NDS_GCC_C0W0(G_CCMUX_##a0, G_CCMUX_##c0, \
                                 G_ACMUX_##Aa0, G_ACMUX_##Ac0) | \
              SSB64_NDS_GCC_C1W0(G_CCMUX_##a1, G_CCMUX_##c1)) & 0xffffffu); \
        _nds_gbi_pkt->words.w1 = (u32)(SSB64_NDS_GCC_C0W1(G_CCMUX_##b0, \
                                                         G_CCMUX_##d0, \
                                                         G_ACMUX_##Ab0, \
                                                         G_ACMUX_##Ad0) | \
                                       SSB64_NDS_GCC_C1W1(G_CCMUX_##b1, \
                                                         G_ACMUX_##Aa1, \
                                                         G_ACMUX_##Ac1, \
                                                         G_CCMUX_##d1, \
                                                         G_ACMUX_##Ab1, \
                                                         G_ACMUX_##Ad1)); \
    } \
} while (0)

/* Two G_CC_* token lists rescan into the sixteen LERP parameters, exactly
 * like the SDK (decomp :3150). */
#define gDPSetCombineMode(pkt, mode1, mode2) \
    gDPSetCombineLERP(pkt, mode1, mode2)

/* THESE TWO ALSO CARRY THEIR WORDS, for the same reason as the colour pair
 * above and with the same consumer discipline.
 *
 * Both are G_SETOTHERMODE_L (0xe2) writes with a fixed shift/length header,
 * so w0 is a constant per macro:
 *   gDPSetRenderMode   -> sft=G_MDSFT_RENDERMODE(3), len=29
 *                         w0 = 0xe2 | ((32-3-29)<<8) | (29-1) = 0xe200001c
 *   gDPSetAlphaCompare -> sft=G_MDSFT_ALPHACOMPARE(0), len=2
 *                         w0 = 0xe2 | ((32-0-2)<<8) | (2-1)   = 0xe2001e01
 * ndsRendererRecordOtherMode (nds_renderer.c:5839) already decodes exactly
 * this header, so no renderer change is needed to read them.
 *
 * SSB64 does not set effect translucency per effect: efDisplayXLUProcDisplay
 * and efDisplayCLDProcDisplay (efdisplay.c:15, :5) are standalone bracket
 * GObjs on links 15 and 18 at display orders 0 and 3 whose entire bodies are
 * these macros, and they write the whole effect LAYER's blend state.
 * efManagerShieldProcDisplay emits only prim/env and depends on that bracket
 * entirely, so zeroing here made the shield, the rebirth halo, the blast
 * pillar, and the impact wave all draw at polygon alpha 31.
 *
 * Nothing in the port executes gSYTaskmanDLHeads[] as a display list, so the
 * only reader of these words is the effect-scoped scan in
 * reloc_backend_renderer_dl.c (ndsRendererAdapterMarkDisplayProcHeads /
 * ...CaptureDisplayProcColors), which applies them solely on the effect
 * submit path. Stage, fighter, interface, menu, and movie draws are
 * unaffected by construction, not by luck. */
#define gDPSetRenderMode(pkt, mode1, mode2) do { \
    Gfx *_nds_gbi_pkt = (Gfx *)(pkt); \
    if (NDS_GBI_PACKET_IN_MAIN_RAM(_nds_gbi_pkt)) { \
        _nds_gbi_pkt->words.w0 = 0xe200001cu; \
        _nds_gbi_pkt->words.w1 = (u32)(mode1) | (u32)(mode2); \
    } \
} while (0)

#define gDPSetAlphaCompare(pkt, type) do { \
    Gfx *_nds_gbi_pkt = (Gfx *)(pkt); \
    if (NDS_GBI_PACKET_IN_MAIN_RAM(_nds_gbi_pkt)) { \
        _nds_gbi_pkt->words.w0 = 0xe2001e01u; \
        _nds_gbi_pkt->words.w1 = (u32)(type); \
    } \
} while (0)

#define gDPSetTexturePersp(pkt, type) do { \
    NDS_GBI_ZERO_PACKET(pkt); \
    (void)(type); \
} while (0)

#define gDPSetDepthSource(pkt, src) do { \
    NDS_GBI_ZERO_PACKET(pkt); \
    (void)(src); \
} while (0)

#define gDPSetColorImage(pkt, fmt, siz, width, image) do { \
    NDS_GBI_ZERO_PACKET(pkt); \
    (void)(fmt); (void)(siz); (void)(width); (void)(image); \
} while (0)

#define gDPSetTextureImage(pkt, fmt, siz, width, image) do { \
    NDS_GBI_ZERO_PACKET(pkt); \
    (void)(fmt); (void)(siz); (void)(width); (void)(image); \
} while (0)

#define gDPSetTile(pkt, fmt, siz, line, tmem, tile, palette, cm_t, mask_t, \
                   shift_t, cm_s, mask_s, shift_s) do { \
    NDS_GBI_ZERO_PACKET(pkt); \
    (void)(fmt); (void)(siz); (void)(line); (void)(tmem); (void)(tile); \
    (void)(palette); (void)(cm_t); (void)(mask_t); (void)(shift_t); \
    (void)(cm_s); (void)(mask_s); (void)(shift_s); \
} while (0)

#define gDPLoadBlock(pkt, tile, uls, ult, lrs, dxt) do { \
    NDS_GBI_ZERO_PACKET(pkt); \
    (void)(tile); (void)(uls); (void)(ult); (void)(lrs); (void)(dxt); \
} while (0)

#define gSPTextureRectangle(pkt, ulx, uly, lrx, lry, tile, s, t, dsdx, dtdy) do { \
    NDS_GBI_ZERO_PACKET(pkt); \
    (void)(ulx); (void)(uly); (void)(lrx); (void)(lry); (void)(tile); \
    (void)(s); (void)(t); (void)(dsdx); (void)(dtdy); \
} while (0)

#define gDPSetTileSize(pkt, tile, uls, ult, lrs, lrt) do { \
    NDS_GBI_ZERO_PACKET(pkt); \
    (void)(tile); (void)(uls); (void)(ult); (void)(lrs); (void)(lrt); \
} while (0)

#define gDPSetColorDither(pkt, mode) do { \
    NDS_GBI_ZERO_PACKET(pkt); \
    (void)(mode); \
} while (0)

#define gDPSetAlphaDither(pkt, mode) do { \
    NDS_GBI_ZERO_PACKET(pkt); \
    (void)(mode); \
} while (0)

#define gDPSetTextureLUT(pkt, type) do { \
    NDS_GBI_ZERO_PACKET(pkt); \
    (void)(type); \
} while (0)

#define gDPLoadTLUT_pal256(pkt, dram) do { \
    NDS_GBI_ZERO_PACKET(pkt); \
    (void)(dram); \
} while (0)

#define gDPSetPrimDepth(pkt, z, dz) do { \
    NDS_GBI_ZERO_PACKET(pkt); \
    (void)(z); (void)(dz); \
} while (0)

#define gDPLoadTextureBlock(pkt, timg, fmt, siz, width, height, pal, \
                            cms, cmt, masks, maskt, shifts, shiftt) do { \
    NDS_GBI_ZERO_PACKET(pkt); \
    (void)(timg); (void)(fmt); (void)(siz); (void)(width); (void)(height); \
    (void)(pal); (void)(cms); (void)(cmt); (void)(masks); (void)(maskt); \
    (void)(shifts); (void)(shiftt); \
} while (0)

#define gDPLoadTextureBlock_4b(pkt, timg, fmt, width, height, pal, \
                               cms, cmt, masks, maskt, shifts, shiftt) do { \
    NDS_GBI_ZERO_PACKET(pkt); \
    (void)(timg); (void)(fmt); (void)(width); (void)(height); (void)(pal); \
    (void)(cms); (void)(cmt); (void)(masks); (void)(maskt); \
    (void)(shifts); (void)(shiftt); \
} while (0)

#define gSPScisTextureRectangle(pkt, ulx, uly, lrx, lry, tile, s, t, \
                                dsdx, dtdy) do { \
    NDS_GBI_ZERO_PACKET(pkt); \
    (void)(ulx); (void)(uly); (void)(lrx); (void)(lry); (void)(tile); \
    (void)(s); (void)(t); (void)(dsdx); (void)(dtdy); \
} while (0)

#endif
