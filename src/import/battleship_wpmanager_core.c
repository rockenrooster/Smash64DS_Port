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
/* decomp lb/lbcommon.c:3008-3013, verbatim. This file declared it and nothing
 * defined it, which held until Sector Z became the first caller: its Arwing
 * orients a 3D laser with two cross products (grsector.c:268-269). */
void lbCommonCross3D(Vec3f *a, Vec3f *b, Vec3f *out)
{
    out->x = (a->y * b->z) - (a->z * b->y);
    out->y = (a->z * b->x) - (a->x * b->z);
    out->z = (a->x * b->y) - (a->y * b->x);
}
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

#if !NDS_IMPORT_BATTLESHIP_MPPROCESS_LIVE
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
#endif

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

/* The weapon pool is allocated ONCE per battle from gSYTaskmanGeneralHeap, and
 * on this target its size decides whether Mario's fireballs spawn at all.
 *
 * Measured 2026-08-01, sampled at the moment a fireball was refused:
 * gSYTaskmanGeneralHeap had 14,796 bytes free, sizeof(WPStruct) is 704, and
 * ifCommonSetMaxNumGObj (ifcommon.c:3156) latches gcSetMaxNumGObj to whatever
 * happens to be alive the instant free space drops under 25 KiB. It had --
 * sGCCommonsMaxNum 47 with 47 active -- so gcMakeGObjSPAfter refused four of
 * eleven spawn requests (SpawnFailGObj 4, SpawnFailPool 0).
 *
 * That latch is an N64 low-memory guard, and reproducing it here does not
 * preserve SSB64's behaviour when the DS-specific pool itself consumes the
 * missing headroom. The fidelity census therefore measures live weapon demand
 * and refusals together with the real 25,600-byte heap floor. The temporary
 * ten-entry ceiling costs 4,928 bytes more than the former pool of three;
 * final sizing is measured high-water plus margin from both required rosters.
 * Do not raise the pool again without re-reading the measured low-water. */
#define wpManagerAllocWeapons ndsBaseWpManagerAllocWeapons
#define wpManagerGetNextStructAlloc ndsBaseWpManagerGetNextStructAlloc
#define wpManagerSetPrevStructAlloc ndsBaseWpManagerSetPrevStructAlloc
#define wpManagerMakeWeapon ndsBaseWpManagerMakeWeapon
#include "../../decomp/BattleShip-main/decomp/src/wp/wpmanager.c"
#undef wpManagerMakeWeapon
#undef wpManagerSetPrevStructAlloc
#undef wpManagerGetNextStructAlloc
#undef wpManagerAllocWeapons
#include "../../decomp/BattleShip-main/decomp/src/wp/wpmain.c"
#include "../../decomp/BattleShip-main/decomp/src/wp/wpmap.c"
#include "../../decomp/BattleShip-main/decomp/src/wp/wpprocess.c"
#include "../../decomp/BattleShip-main/decomp/src/wp/wpdisplay.c"

__attribute__((used)) volatile u32 gNdsWeaponPoolRefusalCount;
__attribute__((used)) volatile u32 gNdsWeaponPoolLiveHighWater;
static u32 sNdsWeaponPoolLiveCount;

static void ndsWpManagerRecordLiveAlloc(void)
{
    sNdsWeaponPoolLiveCount++;
    if (sNdsWeaponPoolLiveCount > gNdsWeaponPoolLiveHighWater)
    {
        gNdsWeaponPoolLiveHighWater = sNdsWeaponPoolLiveCount;
    }
}

WPStruct *wpManagerGetNextStructAlloc(void)
{
    WPStruct *wp = ndsBaseWpManagerGetNextStructAlloc();

    if (wp == NULL)
    {
        gNdsWeaponPoolRefusalCount++;
        return NULL;
    }
    ndsWpManagerRecordLiveAlloc();
    return wp;
}

void wpManagerSetPrevStructAlloc(WPStruct *wp)
{
    if (wp == NULL)
    {
        return;
    }
    if (sNdsWeaponPoolLiveCount != 0u)
    {
        sNdsWeaponPoolLiveCount--;
    }
    ndsBaseWpManagerSetPrevStructAlloc(wp);
}

/* WPAttributes seam. The loader blanket-swaps every u32 word, which
 * half-exchanges the WPAttributes s16 run (attack_offsets + map_coll);
 * wpManagerMakeWeapon reads that run at wpmanager.c:196 and :285-288, so the
 * source constructor is transcribed here and normalizes the resident struct
 * immediately after resolving it, before any of those fields are consumed.
 * The port owns the exactly-once normalization record. */
GObj *wpManagerMakeWeapon(GObj *parent_gobj, WPDesc *wp_desc, Vec3f *spawn_pos,
                          u32 flags)
{
    GObj *weapon_gobj;
    void (*proc_display)(GObj*);
    WPAttributes *attr;
    WPStruct *wp;
    WPStruct *owner_wp;
    ITStruct *ip;
    FTStruct *fp;
    s32 unused[7];

    wp = wpManagerGetNextStructAlloc();

    if (wp == NULL)
    {
        return NULL;
    }
    weapon_gobj = gcMakeGObjSPAfter(nGCCommonKindWeapon, NULL,
                                    nGCCommonLinkIDWeapon,
                                    GOBJ_PRIORITY_DEFAULT);

    if (weapon_gobj == NULL)
    {
        wpManagerSetPrevStructAlloc(wp);
        return NULL;
    }
    attr = lbRelocGetFileData(WPAttributes*, *wp_desc->p_weapon,
                              wp_desc->o_attributes);
    ndsRelocEnsureWeaponAttributesNormalized(attr);
    weapon_gobj->user_data.p = wp;
    wp->weapon_gobj = weapon_gobj;
    wp->kind = wp_desc->kind;

    switch (flags & WEAPON_MASK_PARENT)
    {
    case WEAPON_FLAG_PARENT_FIGHTER:
        fp = ftGetStruct(parent_gobj);
        wp->owner_gobj = parent_gobj;
        wp->team = fp->team;
        wp->player = fp->player;
        wp->handicap = fp->handicap;
        wp->player_num = fp->player_num;
        wp->lr = fp->lr;
        wp->display_mode = fp->display_mode;
        wp->attack_coll.stale = ftParamGetStale(fp->player,
                                                fp->motion_attack_id,
                                                fp->motion_count);
        wp->attack_coll.motion_attack_id = fp->motion_attack_id;
        wp->attack_coll.motion_count = fp->motion_count;
        wp->attack_coll.stat_flags = fp->stat_flags;
        wp->attack_coll.stat_count = fp->stat_count;
        break;

    case WEAPON_FLAG_PARENT_WEAPON:
        owner_wp = wpGetStruct(parent_gobj);
        wp->owner_gobj = owner_wp->owner_gobj;
        wp->team = owner_wp->team;
        wp->player = owner_wp->player;
        wp->handicap = owner_wp->handicap;
        wp->player_num = owner_wp->player_num;
        wp->lr = owner_wp->lr;
        wp->display_mode = owner_wp->display_mode;
        wp->attack_coll.stale = owner_wp->attack_coll.stale;
        wp->attack_coll.motion_attack_id = owner_wp->attack_coll.motion_attack_id;
        wp->attack_coll.motion_count = owner_wp->attack_coll.motion_count;
        wp->attack_coll.stat_flags = owner_wp->attack_coll.stat_flags;
        wp->attack_coll.stat_count = owner_wp->attack_coll.stat_count;
        break;

    case WEAPON_FLAG_PARENT_ITEM:
        ip = itGetStruct(parent_gobj);
        wp->owner_gobj = ip->owner_gobj;
        wp->team = ip->team;
        wp->player = ip->player;
        wp->handicap = ip->handicap;
        wp->player_num = ip->player_num;
        wp->lr = ip->lr;
        wp->display_mode = ip->display_mode;
        wp->attack_coll.stale = ip->attack_coll.stale;
        wp->attack_coll.motion_attack_id = ip->attack_coll.motion_attack_id;
        wp->attack_coll.motion_count = ip->attack_coll.motion_count;
        wp->attack_coll.stat_flags = ip->attack_coll.stat_flags;
        wp->attack_coll.stat_count = ip->attack_coll.stat_count;
        break;

    default:
    case WEAPON_FLAG_PARENT_GROUND:
        wp->owner_gobj = NULL;
        wp->team = WEAPON_TEAM_DEFAULT;
        wp->player = WEAPON_PORT_DEFAULT;
        wp->handicap = WEAPON_HANDICAP_DEFAULT;
        wp->player_num = 0;
        wp->lr = +1;
        wp->display_mode = sWPManagerDisplayMode;
        wp->attack_coll.motion_attack_id = nFTMotionAttackIDNone;
        wp->attack_coll.stale = WEAPON_STALE_DEFAULT;
        wp->attack_coll.motion_count = ftParamGetMotionCount();
        wp->attack_coll.stat_flags.attack_id = nFTStatusAttackIDNone;
        wp->attack_coll.stat_flags.is_smash_attack = 0;
        wp->attack_coll.stat_flags.ga = 0;
        wp->attack_coll.stat_flags.is_projectile = 0;
        wp->attack_coll.stat_count = ftParamGetStatUpdateCount();
        break;
    }
    wp->attack_coll.attack_state = nGMAttackStateNew;
    wp->physics.vel_air.x = wp->physics.vel_air.y = wp->physics.vel_air.z = 0.0F;
    wp->physics.vel_ground = 0.0F;
    wp->attack_coll.damage = attr->damage;
    wp->attack_coll.element = attr->element;
    wp->attack_coll.offsets[0].x = attr->attack_offsets[0].x;
    wp->attack_coll.offsets[0].y = attr->attack_offsets[0].y;
    wp->attack_coll.offsets[0].z = attr->attack_offsets[0].z;
    wp->attack_coll.offsets[1].x = attr->attack_offsets[1].x;
    wp->attack_coll.offsets[1].y = attr->attack_offsets[1].y;
    wp->attack_coll.offsets[1].z = attr->attack_offsets[1].z;
    wp->attack_coll.size = attr->size * 0.5F;
    wp->attack_coll.angle = attr->angle;
    wp->attack_coll.knockback_scale = attr->knockback_scale;
    wp->attack_coll.knockback_weight = attr->knockback_weight;
    wp->attack_coll.knockback_base = attr->knockback_base;
    wp->attack_coll.can_setoff = attr->can_setoff;
    wp->attack_coll.shield_damage = attr->shield_damage;
    wp->attack_coll.fgm_id = attr->sfx;
    wp->attack_coll.priority = attr->priority;
    wp->attack_coll.can_rehit_item = attr->can_rehit_item;
    wp->attack_coll.can_rehit_fighter = attr->can_rehit_fighter;
    wp->attack_coll.can_rehit_shield = FALSE;
    wp->attack_coll.can_hop = attr->can_hop;
    wp->attack_coll.can_reflect = attr->can_reflect;
    wp->attack_coll.can_absorb = attr->can_absorb;
    wp->attack_coll.can_not_heal = FALSE;
    wp->attack_coll.can_shield = attr->can_shield;
    wp->attack_coll.attack_count = attr->attack_count;
    wp->attack_coll.interact_mask = GMHITCOLLISION_FLAG_ALL;
    wpMainClearAttackRecord(wp);
    wp->hit_normal_damage = 0;
    wp->hit_refresh_damage = 0;
    wp->hit_attack_damage = 0;
    wp->hit_shield_damage = 0;
    wp->reflect_gobj = NULL;
    wp->absorb_gobj = NULL;
    wp->is_hitlag_victim = FALSE;
    wp->is_hitlag_weapon = FALSE;
    wp->is_camera_follow = FALSE;
    wp->group_id = 0;
    wp->is_static_damage = FALSE;
    wp->p_sfx = NULL;
    wp->sfx_id = 0;
    wp->shield_collide_angle = 0.0F;
    wp->shield_collide_dir.x = 0.0F;
    wp->shield_collide_dir.y = 0.0F;
    wp->shield_collide_dir.z = 0.0F;

    if (wp_desc->flags & WEAPON_FLAG_DOBJDESC)
    {
        gcSetupCustomDObjs(weapon_gobj, attr->data, NULL,
                           wp_desc->transform_types.tk1,
                           wp_desc->transform_types.tk2,
                           wp_desc->transform_types.tk3);
        proc_display = (wp_desc->flags & WEAPON_FLAG_DOBJLINKS) ?
            wpDisplayDObjTreeDLLinks : func_ovl3_80167618;
    }
    else
    {
        lbCommonInitDObj3Transforms(
            gcAddDObjForGObj(weapon_gobj, attr->data),
            wp_desc->transform_types.tk1,
            wp_desc->transform_types.tk2,
            wp_desc->transform_types.tk3);
        proc_display = (wp_desc->flags & WEAPON_FLAG_DOBJLINKS) ?
            wpDisplayDObjDLLinks : wpDisplayDLHead1;
    }
    gcAddGObjDisplay(weapon_gobj, proc_display, 14, GOBJ_PRIORITY_DEFAULT, ~0);

    if (attr->p_mobjsubs != NULL)
    {
        gcAddMObjAll(weapon_gobj, attr->p_mobjsubs);
    }
    if ((attr->anim_joints != NULL) || (attr->p_matanim_joints != NULL))
    {
        gcAddAnimAll(weapon_gobj, attr->anim_joints,
                     attr->p_matanim_joints, 0.0F);
    }
    wp->coll_data.p_translate = &DObjGetStruct(weapon_gobj)->translate.vec.f;
    wp->coll_data.p_lr = &wp->lr;
    wp->coll_data.map_coll.top = attr->map_coll_top;
    wp->coll_data.map_coll.center = attr->map_coll_center;
    wp->coll_data.map_coll.bottom = attr->map_coll_bottom;
    wp->coll_data.map_coll.width = attr->map_coll_width;
    wp->coll_data.p_map_coll = &wp->coll_data.map_coll;
    wp->coll_data.ignore_line_id = -1;
    wp->coll_data.floor_line_id = -1;
    wp->coll_data.ceil_line_id = -1;
    wp->coll_data.lwall_line_id = -1;
    wp->coll_data.rwall_line_id = -1;
    wp->coll_data.update_tic = gMPCollisionUpdateTic;
    wp->coll_data.mask_curr = 0;
    wp->coll_data.vel_push.x = 0.0F;
    wp->coll_data.vel_push.y = 0.0F;
    wp->coll_data.vel_push.z = 0.0F;
    gcAddGObjProcess(weapon_gobj, wpProcessProcWeaponMain,
                     nGCProcessKindFunc, 3);
    gcAddGObjProcess(weapon_gobj, wpProcessProcSearchHitWeapon,
                     nGCProcessKindFunc, 1);
    gcAddGObjProcess(weapon_gobj, wpProcessProcHitCollisions,
                     nGCProcessKindFunc, 0);
    wp->proc_update = wp_desc->proc_update;
    wp->proc_map = wp_desc->proc_map;
    wp->proc_hit = wp_desc->proc_hit;
    wp->proc_shield = wp_desc->proc_shield;
    wp->proc_hop = wp_desc->proc_hop;
    wp->proc_setoff = wp_desc->proc_setoff;
    wp->proc_reflector = wp_desc->proc_reflector;
    wp->proc_absorb = wp_desc->proc_absorb;
    wp->proc_dead = NULL;
    wp->coll_data.pos_prev = DObjGetStruct(weapon_gobj)->translate.vec.f =
        *spawn_pos;

    if (flags & WEAPON_FLAG_COLLPROJECT)
    {
        switch (flags & WEAPON_MASK_PARENT)
        {
        default:
        case WEAPON_FLAG_PARENT_GROUND:
            break;

        case WEAPON_FLAG_PARENT_FIGHTER:
            mpCommonRunWeaponCollisionDefault(
                weapon_gobj, ftGetStruct(parent_gobj)->coll_data.p_translate,
                &ftGetStruct(parent_gobj)->coll_data);
            break;

        case WEAPON_FLAG_PARENT_WEAPON:
            mpCommonRunWeaponCollisionDefault(
                weapon_gobj, wpGetStruct(parent_gobj)->coll_data.p_translate,
                &wpGetStruct(parent_gobj)->coll_data);
            break;

        case WEAPON_FLAG_PARENT_ITEM:
            mpCommonRunWeaponCollisionDefault(
                weapon_gobj, itGetStruct(parent_gobj)->coll_data.p_translate,
                &itGetStruct(parent_gobj)->coll_data);
            break;
        }
    }
    wp->ga = nMPKineticsAir;
    wpProcessUpdateHitPositions(weapon_gobj);

    return weapon_gobj;
}

void wpManagerAllocWeapons(void)
{
    WPStruct *wp;
    s32 i;

    sWPManagerStructsAllocFree = wp =
        syTaskmanMalloc(sizeof(WPStruct) * NDS_R2_WEAPON_POOL, 0x8);

    for (i = 0; i < (NDS_R2_WEAPON_POOL - 1); i++)
    {
        wp[i].next = &wp[i + 1];
    }
    if (wp != NULL)
    {
        wp[i].next = NULL;
    }
    sWPManagerGroupID = 1;
    sWPManagerDisplayMode = nDBDisplayModeMaster;
    gNdsWeaponPoolEntries = NDS_R2_WEAPON_POOL;
    gNdsWeaponStructBytes = (u32)sizeof(WPStruct);
    gNdsWeaponPoolRefusalCount = 0;
    gNdsWeaponPoolLiveHighWater = 0;
    sNdsWeaponPoolLiveCount = 0;
}

#endif
