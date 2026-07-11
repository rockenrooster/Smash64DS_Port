#include <ft/fighter.h>
#include <if/interface.h>
#include <nds/nds_fighter_display.h>
#include <sys/objman.h>
#include <sys/taskman.h>
#include "../../decomp/BattleShip-main/decomp/src/sys/objdisplay.h"

typedef struct {
    s16 ob[3];
    u16 flag;
    s16 tc[2];
    u8 cn[4];
} Vtx_t;

typedef struct {
    s16 ob[3];
    u16 flag;
    s16 tc[2];
    s8 n[3];
    u8 a;
} Vtx_tn;

typedef union {
    Vtx_t v;
    Vtx_tn n;
    long long int force_structure_alignment;
} Vtx;

#ifndef DObjGetStruct
#define DObjGetStruct(gobj) ((DObj *)((gobj)->obj))
#endif
#ifndef CObjGetStruct
#define CObjGetStruct(gobj) ((CObj *)((gobj)->obj))
#endif

extern GObj *gGMCameraGObj;
extern Mtx44f gGMCameraMatrix;

extern Vec3f gLBCommonScale;
extern s32 gcPrepDObjMatrix(Gfx **dls, DObj *dobj);
extern void gcDrawMObjForDObj(DObj *dobj, Gfx **dls);
extern sb32 gmCameraCheckTargetInBounds(f32 pos_x, f32 pos_y);
extern void func_ovl2_800EB924(CObj *cobj, Mtx44f matrix, Vec3f *pos,
                               f32 *dist_x, f32 *dist_y);

#define LIGHT_1 1
#define LIGHT_2 2
/* Preserve the source GBI encodings before the narrow port header stubs them. */
#undef G_CYC_1CYCLE
#define G_CYC_1CYCLE 0x00000000u
#define G_CYC_2CYCLE 0x00100000u
#define G_ON 1
#define G_TX_NOMIRROR 0
#define G_RM_PASS 0x0c080000u
#define G_RM_FOG_PRIM_A 0xc4000000u
#define G_RM_AA_ZB_TEX_EDGE 0
#define G_RM_AA_ZB_TEX_EDGE2 0
#undef G_RM_AA_ZB_OPA_SURF
#define G_RM_AA_ZB_OPA_SURF 0x00442078u
#undef G_RM_AA_ZB_OPA_SURF2
#define G_RM_AA_ZB_OPA_SURF2 0x00112078u
#undef G_RM_AA_ZB_XLU_SURF2
#define G_RM_AA_ZB_XLU_SURF2 0x001049d8u
#define GBL_c1(...) 0
#define GBL_c2(...) 0
#define AA_EN 0
#define Z_UPD 0
#define IM_RD 0
#define CLR_ON_CVG 0
#define CVG_DST_WRAP 0
#define ZMODE_OPA 0
#define FORCE_BL 0
#define G_BL_CLR_IN 0
#define G_BL_A_IN 0
#define G_BL_CLR_MEM 0
#define G_BL_1MA 0

#define gsDPSetCycleType(...) { 0 }
#define gsSPLightColor(...) { 0 }
#define gsDPSetCombineLERP(...) { 0 }
#define gsSPTexture(...) { 0 }
#define gsDPSetTextureImage(...) { 0 }
#define gsDPSetTile(...) { 0 }
#define gsDPLoadSync(...) { 0 }
#define gsDPLoadBlock(...) { 0 }
#define gsDPSetTileSize(...) { 0 }
#define gsSPVertex(...) { 0 }
#define gsSP1Triangle(...) { 0 }
#define gsSP2Triangles(...) { 0 }
#define gsDPSetEnvColor(...) { 0 }
#undef gsSPClearGeometryMode
#define gsSPClearGeometryMode(...) { 0 }

#ifndef gSPVertex
#define gSPVertex(pkt, vtx, count, first) \
    do { (void)(pkt); (void)(vtx); (void)(count); (void)(first); } while (0)
#endif
#ifndef gSP2Triangles
#define gSP2Triangles(pkt, a, b, c, flag0, d, e, f, flag1) \
    do { (void)(pkt); } while (0)
#endif

#undef gSPDisplayList
#define gSPDisplayList(pkt, dl) \
    ndsFighterDisplayContractSelectDL((const Gfx *)(dl))
#undef gDPSetCycleType
#define gDPSetCycleType(pkt, cycle) \
    ndsFighterDisplayContractSetCycleType((u32)(cycle))
#undef gDPSetRenderMode
#define gDPSetRenderMode(pkt, mode1, mode2) \
    ndsFighterDisplayContractSetRenderMode((u32)(mode1), (u32)(mode2))
#undef gSPSetGeometryMode
#define gSPSetGeometryMode(pkt, mode) \
    ndsFighterDisplayContractSetGeometryMode(0u, (u32)(mode))
#undef gSPClearGeometryMode
#define gSPClearGeometryMode(pkt, mode) \
    ndsFighterDisplayContractSetGeometryMode((u32)(mode), 0u)
#undef gDPSetEnvColor
#define gDPSetEnvColor(pkt, r, g, b, a) \
    ndsFighterDisplayContractSetEnvColor((r), (g), (b), (a))
#undef gDPSetPrimColor
#define gDPSetPrimColor(pkt, m, l, r, g, b, a) \
    ndsFighterDisplayContractSetPrimColor((r), (g), (b), (a))

#ifndef gDPSetFogColor
#define gDPSetFogColor(pkt, r, g, b, a) \
    do { (void)(pkt); (void)(r); (void)(g); (void)(b); (void)(a); } while (0)
#endif
#ifndef gSPPopMatrix
#define gSPPopMatrix(pkt, mode) do { (void)(pkt); (void)(mode); } while (0)
#endif

#define mpCollisionSetLightColorGetAlpha \
    ndsFighterDisplayContractSetStageEnvColor
#define scSubsysFighterDrawLightColorGetAlpha \
    ndsFighterDisplayContractSetStageEnvColor
#define gmCameraCheckTargetInBounds \
    ndsFighterDisplayContractCheckTargetInBounds
#define func_ovl2_800EB924 ndsFighterDisplayContractProjectTarget

#define ftDisplayMainProcDisplay ndsBaseFTDisplayMainProcDisplay
#include "../../decomp/BattleShip-main/decomp/src/ft/ftdisplaymain.c"
#undef ftDisplayMainProcDisplay
#undef mpCollisionSetLightColorGetAlpha
#undef scSubsysFighterDrawLightColorGetAlpha
#undef gmCameraCheckTargetInBounds
#undef func_ovl2_800EB924
