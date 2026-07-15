#if NDS_IMPORT_BATTLESHIP_WEAPON_MANAGER

#include <PR/gbi.h>
#include <PR/gu.h>
#include <ft/fighter.h>
#include <gr/ground.h>
#include <it/item.h>
#include <mp/map.h>
#include <nds/nds_startup.h>
#include <reloc_data.h>
#include <sc/scene.h>
#include <string.h>
#include <sys/matrix.h>
#include <sys/objhelper.h>
#include <sys/objman.h>
#include <sys/rdp.h>
#include <sys/taskman.h>
#include <wp/weapon.h>

#ifndef bzero
#define bzero(ptr, size) memset((ptr), 0, (size))
#endif

#ifndef DObjGetStruct
#define DObjGetStruct(gobj) ((DObj *)((gobj)->obj))
#endif

#ifndef syMatrixAdvanceW
#define syMatrixAdvance(mtx, mtx_heap, type) \
    (mtx = (mtx_heap).ptr, \
     (mtx_heap).ptr = (void *)((type *)(mtx_heap).ptr + 1))
#define syMatrixAdvanceW(mtx, mtx_heap) syMatrixAdvance(mtx, mtx_heap, Mtx)
#endif

#ifndef gSPPopMatrix
#define gSPPopMatrix(pkt, mode) ((void)(pkt), (void)(mode))
#endif

#ifndef SSB64_NDS_ALSOUNDEFFECT_STRUCT
#define SSB64_NDS_ALSOUNDEFFECT_STRUCT
struct alSoundEffect {
    void *unk_0x0;
    void *unk_0x4;
    void *unk_0x8;
    void *unk_0xC;
    u16 unk_0x10;
    u16 unk_0x12;
    u16 unk_0x14;
    u16 unk_0x16;
    u16 unk_0x18;
    u16 unk_0x1A;
    u16 unk_0x1C;
    u8 unk_0x1E;
    u8 unk_0x1F;
    u16 unk_0x20;
    u16 unk_0x22;
    u16 unk_0x24;
    u16 sfx_id;
    u16 sfx_max;
    u8 filler_0x2A[0x2F - 0x2A];
    u8 balance;
};
#endif

extern void gcSetupCustomDObjs(GObj *gobj, DObjDesc *dobjdesc, DObj **dobjs,
                               u8 tk1, u8 tk2, u8 tk3);
extern void gcAddMObjAll(GObj *gobj, MObjSub ***p_mobjsubs);
extern void gcAddAnimAll(GObj *gobj, AObjEvent32 **anim_joints,
                         AObjEvent32 ***p_matanim_joints, f32 anim_frame);
extern void gcPlayAnimAll(GObj *gobj);
extern void gcDrawDObjDLHead1(GObj *gobj);
extern void gcDrawDObjDLLinksForGObj(GObj *gobj);
extern void gcDrawDObjTreeDLLinksForGObj(GObj *gobj);
extern void lbCommonDObjScaleXProcDisplay(GObj *gobj);
extern void wpDisplayDObjTreeDLLinks(GObj *weapon_gobj);
extern void func_ovl3_80167618(GObj *weapon_gobj);
extern void wpDisplayDObjDLLinks(GObj *weapon_gobj);
extern void wpDisplayDLHead1(GObj *weapon_gobj);
extern f32 lbCommonNormDist2D(Vec3f *p);
extern f32 lbCommonMag2D(Vec3f *p);
extern Vec3f *lbCommonScale2D(Vec3f *dst, f32 scale);
extern Vec3f *lbCommonReflect2D(Vec3f *dst, Vec3f *p);
extern f32 lbCommonSim2D(Vec3f *a, Vec3f *b);
extern void lbCommonCross3D(Vec3f *a, Vec3f *b, Vec3f *out);
extern f32 ftParamGetStale(s32 player, s32 motion_attack_id,
                           s32 motion_count);
extern u16 ftParamGetMotionCount(void);
extern u16 ftParamGetStatUpdateCount(void);
extern void efManagerSetOffMakeEffect(Vec3f *pos, s32 damage);
extern void guMtxIdentF(float mf[4][4]);
extern void guMtxL2F(float mf[4][4], Mtx *m);
extern void guMtxCatF(float mf[4][4], float nf[4][4], float res[4][4]);
extern void guMtxXFMF(float mf[4][4], float x, float y, float z, float *ox,
                      float *oy, float *oz);
extern u16 gFTManagerMotionCount;
extern u16 gFTManagerStatUpdateCount;
extern Gfx dFTDisplayMainHitCollisionEdgeDL[];
extern Gfx dFTDisplayMainHitCollisionBlendDL[];
extern Gfx dFTDisplayMainHitCollisionCubeDL[];
extern Gfx dFTDisplayMainMapCollisionBottomDL[];
extern Gfx dFTDisplayMainMapCollisionTopDL[];

#define NDS_WPMANAGER_BRIDGE __attribute__((weak))

static const f32 dNDSWeaponStaleTable[] = { 0.75F, 0.82F, 0.89F, 0.96F };

NDS_WPMANAGER_BRIDGE f32 lbCommonNormDist2D(Vec3f *vec)
{
    f32 magnitude = sqrtf(SQUARE(vec->x) + SQUARE(vec->y));

    if (magnitude == 0.0F)
    {
        return 0.0F;
    }
    vec->x *= 1.0F / magnitude;
    vec->y *= 1.0F / magnitude;
    return magnitude;
}

NDS_WPMANAGER_BRIDGE f32 lbCommonSim2D(Vec3f *a, Vec3f *b)
{
    f32 magnitude_a = sqrtf(SQUARE(a->x) + SQUARE(a->y));
    f32 magnitude_b = sqrtf(SQUARE(b->x) + SQUARE(b->y));

    return (a->x * b->x + a->y * b->y) / (magnitude_b + magnitude_a);
}

NDS_WPMANAGER_BRIDGE void lbCommonDObjScaleXProcDisplay(GObj *gobj)
{
    (void)gobj;
}

NDS_WPMANAGER_BRIDGE f32 ftParamGetStale(s32 player, s32 attack_id,
                                         s32 motion_count)
{
    s32 stale_id;
    s32 start_array_id;
    s32 current_array_id;
    s32 i;

    if ((gSCManagerBattleState == NULL) ||
        (player >= (s32)ARRAY_COUNT(gSCManagerBattleState->players)) ||
        (attack_id == nFTMotionAttackIDNone))
    {
        return 1.0F;
    }
    stale_id = gSCManagerBattleState->players[player].stale_id;
    current_array_id = start_array_id =
        (stale_id != 0) ?
            stale_id - 1 :
            ARRAY_COUNT(gSCManagerBattleState->players[player].stale_info) - 1;

    for (i = 0; i < (s32)ARRAY_COUNT(dNDSWeaponStaleTable); i++)
    {
        if (attack_id ==
            gSCManagerBattleState->players[player]
                .stale_info[current_array_id]
                .attack_id)
        {
            if ((u16)motion_count !=
                gSCManagerBattleState->players[player]
                    .stale_info[current_array_id]
                    .motion_count)
            {
                return dNDSWeaponStaleTable[i];
            }
            if (current_array_id == start_array_id)
            {
                i--;
            }
        }
        current_array_id =
            (current_array_id != 0) ? current_array_id - 1 :
                                      ARRAY_COUNT(gSCManagerBattleState->players[player].stale_info) - 1;
    }
    return 1.0F;
}

NDS_WPMANAGER_BRIDGE u16 ftParamGetMotionCount(void)
{
    u16 motion_count = gFTManagerMotionCount++;

    if (gFTManagerMotionCount == 0)
    {
        gFTManagerMotionCount = 1;
    }
    return motion_count;
}

NDS_WPMANAGER_BRIDGE u16 ftParamGetStatUpdateCount(void)
{
    u16 update_count = gFTManagerStatUpdateCount++;

    if (gFTManagerStatUpdateCount == 0)
    {
        gFTManagerStatUpdateCount = 1;
    }
    return update_count;
}

NDS_WPMANAGER_BRIDGE void mpProcessRunLWallCollisionAdjNew(
    MPCollData *coll_data)
{
    (void)mpProcessCheckTestLWallCollisionAdjNew(coll_data);
}

NDS_WPMANAGER_BRIDGE void mpProcessRunRWallCollisionAdjNew(
    MPCollData *coll_data)
{
    (void)mpProcessCheckTestRWallCollisionAdjNew(coll_data);
}

NDS_WPMANAGER_BRIDGE void mpProcessRunCeilEdgeAdjust(MPCollData *coll_data)
{
    (void)coll_data;
}

void mpCommonRunWeaponCollisionDefault(
    GObj *weapon_gobj, Vec3f *pos, MPCollData *coll_data)
{
    WPStruct *wp = wpGetStruct(weapon_gobj);

    if ((wp == NULL) || (pos == NULL) || (coll_data == NULL))
    {
        return;
    }
    gNdsCollisionRuntimeDiagnostics.default_weapon_calls++;
    mpCommonCopyCollDataStats(&wp->coll_data, pos, coll_data);
    mpCommonRunDefaultCollision(&wp->coll_data, weapon_gobj,
                                MAP_PROC_TYPE_DEFAULT);
    mpCommonResetCollDataStats(&wp->coll_data);
}

#undef NDS_WPMANAGER_BRIDGE

#include "../../decomp/BattleShip-main/decomp/src/wp/wpmanager.c"
#include "../../decomp/BattleShip-main/decomp/src/wp/wpmain.c"
#include "../../decomp/BattleShip-main/decomp/src/wp/wpmap.c"
#include "../../decomp/BattleShip-main/decomp/src/wp/wpprocess.c"
#include "../../decomp/BattleShip-main/decomp/src/wp/wpdisplay.c"

#endif
