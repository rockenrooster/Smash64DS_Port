/* BattleShip's shared item owner. Link's Bomb was its first live client and
 * gave the file its name; Ness's PK Fire pillar is the second, so the TU is
 * gated on NDS_P2_ITEM_CORE (any fighter with an item article) rather than on
 * one fighter's flag. LinkBomb's own attribute load and status dispatch stay
 * behind NDS_P2_LINK below.
 *
 * Keep gameplay/state ownership common-item shaped, but do not drag the whole
 * P2-5 common-item bank/random-spawn registry into a fighter row. Generic
 * item map/process/display code below is BattleShip source verbatim; manager
 * setup and ITAttributes are DS adaptations around that source contract. */
#if NDS_P2_ITEM_CORE

#include <ef/effect.h>
#include <ft/fighter.h>
#include <gm/gmsound.h>
#include <it/item.h>
#include <mp/map.h>
#include <reloc_data.h>
#include <sc/scene.h>
#include <string.h>
#include <sys/develop.h>
#include <sys/objhelper.h>
#include <sys/objman.h>
#include <sys/taskman.h>
#include <wp/weapon.h>

#ifndef DObjGetStruct
#define DObjGetStruct(gobj) ((DObj *)((gobj)->obj))
#endif

#if NDS_P2_LINK_BOMB_TOUR
extern volatile u32 gNdsLinkBombTourAttrValidCount;
void ndsLinkBombTourProofItemDestroy(GObj *item_gobj);
#endif

/* BattleShip itdisplay.c uses a two-cycle preamble around ColAnim items.  The
 * project-wide gDPSetCycleType remains a no-op because most legacy streams are
 * not consumed by the DS; this TU is different now that the item DObj path
 * captures the proc's own state. Emit the ordinary F3DEX2 SETOTHERMODE_H word
 * locally so the generic item renderer sees the same cycle-type transition. */
#ifndef G_CYC_2CYCLE
#define G_CYC_2CYCLE 0x00100000u
#endif
#ifndef G_RM_PASS
#define G_RM_PASS 0x0c080000u
#endif
#undef gDPSetCycleType
#define gDPSetCycleType(pkt, type) do { \
    Gfx *_nds_it_pkt = (Gfx *)(pkt); \
    if (NDS_GBI_PACKET_IN_MAIN_RAM(_nds_it_pkt)) { \
        _nds_it_pkt->words.w0 = 0xe3000a01u; \
        _nds_it_pkt->words.w1 = (u32)(type); \
    } \
} while (0)

extern void gcSetupCustomDObjsWithMObj(GObj *gobj, DObjDesc *dobjdesc,
                                        MObjSub ***p_mobjsubs, DObj **dobjs,
                                        u8 tk1, u8 tk2, u8 tk3);
extern void gcAddMObjAll(GObj *gobj, MObjSub ***p_mobjsubs);
extern void gcAddAnimAll(GObj *gobj, AObjEvent32 **anim_joints,
                         AObjEvent32 ***p_matanim_joints, f32 anim_frame);
extern void gcPlayAnimAll(GObj *gobj);
extern void gcSetDObjTransformsForGObj(GObj *gobj, DObjDesc *dobjdesc);
extern void gcDrawDObjTreeForGObj(GObj *gobj);
extern void gcDrawDObjTreeDLLinksForGObj(GObj *gobj);
extern f32 lbCommonNormDist2D(Vec3f *p);
extern f32 lbCommonMag2D(Vec3f *p);
extern f32 lbCommonSim2D(Vec3f *a, Vec3f *b);
extern Vec3f *lbCommonScale2D(Vec3f *dst, f32 scale);
extern Vec3f *lbCommonReflect2D(Vec3f *dst, Vec3f *p);
extern u16 ftParamGetMotionCount(void);
extern u16 ftParamGetStatUpdateCount(void);
extern void ftParamSetModelPartDefaultID(GObj *fighter_gobj, s32 joint_id,
                                         s32 modelpart_id);
extern void ftParamTryUpdateItemMusic(void);
extern GObj *efManagerItemGetSwirlProcUpdate(Vec3f *pos);
extern LBParticle *efManagerSparkleWhiteScaleMakeEffect(Vec3f *pos, f32 scale);
extern LBParticle *efManagerDustExpandLargeMakeEffect(Vec3f *pos);
extern void func_800269C0_275C0(u16 sfx_id);
extern sb32 ftMainUpdateColAnim(GMColAnim *colanim, GObj *gobj,
                                sb32 is_muted, sb32 is_effect_skip);
extern sb32 gmCollisionCheckFighterAttackItemDamageCollide(
    FTAttackColl *attack_coll, ITDamageColl *damage_coll, GObj *item_gobj);
extern sb32 gmCollisionCheckWeaponAttackItemAttackCollide(
    WPAttackColl *wp_attack_coll, s32 wp_attack_id,
    ITAttackColl *it_attack_coll, s32 it_attack_id);
extern sb32 gmCollisionCheckItemAttacksCollide(ITAttackColl *attack_coll1,
                                                s32 attack_id1,
                                                ITAttackColl *attack_coll2,
                                                s32 attack_id2);
extern sb32 gmCollisionCheckItemAttackDamageCollide(
    ITAttackColl *attack_coll, s32 attack_id, ITDamageColl *damage_coll,
    GObj *item_gobj);
extern sb32 gmCollisionCheckWeaponAttackItemDamageCollide(
    WPAttackColl *attack_coll, s32 attack_id, ITDamageColl *damage_coll,
    GObj *item_gobj);
extern void gmCollisionGetFighterAttackItemDamagePosition(
    Vec3f *dst, FTAttackColl *attack_coll, ITDamageColl *damage_coll,
    GObj *item_gobj);
extern void gmCollisionGetWeaponAttackItemAttackPosition(
    Vec3f *dst, WPAttackColl *wp_attack_coll, s32 wp_attack_id,
    ITAttackColl *it_attack_coll, s32 it_attack_id);
extern void gmCollisionGetWeaponAttackItemDamagePosition(
    Vec3f *dst, WPAttackColl *attack_coll, s32 attack_id,
    ITDamageColl *damage_coll, GObj *item_gobj);
extern void gmCollisionGetItemAttackItemAttackPosition(
    Vec3f *dst, ITAttackColl *attack_coll1, s32 attack_id1,
    ITAttackColl *attack_coll2, s32 attack_id2);
extern void gmCollisionGetItemAttackItemDamagePosition(
    Vec3f *dst, ITAttackColl *attack_coll, s32 attack_id,
    ITDamageColl *damage_coll, GObj *item_gobj);

#if NDS_P2_LINK
/* LinkBomb status dispatch is the first common-item table client. Its three
 * call sites below are already keyed on `ip->kind == nITKindLinkBomb`, so with
 * Link out of the build no item can reach them and the dispatch goes with him.
 */
extern void itLinkBombDroppedSetStatus(GObj *item_gobj);
extern void itLinkBombThrownSetStatus(GObj *item_gobj);
extern void itLinkBombHoldSetStatus(GObj *item_gobj);
#endif

static ITStruct *sNdsItemStructsFree;
static ITAttributes sNdsLinkBombAttributes;
static void *sNdsLinkBombAttributesFile;
s32 gITManagerDisplayMode;

static void ndsItParamLinkResetShieldModelParts(GObj *fighter_gobj);
static void ndsItParamSetHammerParams(GObj *fighter_gobj);

/* BattleShip lbcommon.c:1241-1257. This helper is tiny and item ownership is
 * its first live DS client, so keep the exact tree-detach semantics here until
 * the broader lbcommon owner graduates. */
void lbCommonEjectTreeDObj(DObj *dobj)
{
    DObj *child_dobj = dobj->child;
    DObj *parent_dobj = dobj->parent;

    dobj->child = NULL;
    gcEjectDObj(dobj);

    if (parent_dobj == DOBJ_PARENT_NULL)
    {
        child_dobj->parent_gobj->obj = child_dobj;
        child_dobj->parent_gobj->obj_kind = nGCCommonAppendDObj;
    }
    else
    {
        parent_dobj->child = child_dobj;
    }
    child_dobj->parent = parent_dobj;
}

/* BattleShip mpcommon.c:968-975 verbatim. The fighter/weapon wrappers already
 * live in the DS collision bridge; LinkBomb is the first item caller. */
void mpCommonRunItemCollisionDefault(GObj *item_gobj, Vec3f *pos,
                                     MPCollData *coll_data)
{
    ITStruct *ip = itGetStruct(item_gobj);

    mpCommonCopyCollDataStats(&ip->coll_data, pos, coll_data);
    mpCommonRunDefaultCollision(&ip->coll_data, item_gobj,
                                MAP_PROC_TYPE_DEFAULT);
    mpCommonResetCollDataStats(&ip->coll_data);
}

static s32 ndsItSignExtend(u32 value, u32 width)
{
    u32 shift = 32u - width;
    return (s32)(value << shift) >> shift;
}

/* BattleShip's audited ITAttributes ROM layout (0x48 bytes) is packed using
 * IDO rules that ARM GCC does not reproduce. The reloc loader has already
 * converted each source u32 to host numeric order and fixed pointer words;
 * decode those words once into an ordinary DS structure. */
static sb32 ndsItDecodeAttributes(void *file, const void *offset_token,
                                  ITAttributes *out)
{
    u8 *raw;
    u32 w10, w14, w18, w1c, w20, w24, w28, w2c, w30;
    u32 w34, w38, w3c, w40, w44;
    void *ptr;

    if ((file == NULL) || (out == NULL))
    {
        return FALSE;
    }
    raw = lbRelocGetFileData(u8*, file, offset_token);
    if (raw == NULL)
    {
        return FALSE;
    }
    memset(out, 0, sizeof(*out));

    memcpy(&ptr, raw + 0x00, sizeof(ptr)); out->data = ptr;
    memcpy(&ptr, raw + 0x04, sizeof(ptr)); out->p_mobjsubs = (MObjSub ***)ptr;
    memcpy(&ptr, raw + 0x08, sizeof(ptr)); out->anim_joints = (AObjEvent32 **)ptr;
    memcpy(&ptr, raw + 0x0c, sizeof(ptr)); out->p_matanim_joints = (AObjEvent32 ***)ptr;

#define NDS_IT_WORD(off, dst) memcpy(&(dst), raw + (off), sizeof(dst))
    NDS_IT_WORD(0x10, w10); NDS_IT_WORD(0x14, w14);
    NDS_IT_WORD(0x18, w18); NDS_IT_WORD(0x1c, w1c);
    NDS_IT_WORD(0x20, w20); NDS_IT_WORD(0x24, w24);
    NDS_IT_WORD(0x28, w28); NDS_IT_WORD(0x2c, w2c);
    NDS_IT_WORD(0x30, w30); NDS_IT_WORD(0x34, w34);
    NDS_IT_WORD(0x38, w38); NDS_IT_WORD(0x3c, w3c);
    NDS_IT_WORD(0x40, w40); NDS_IT_WORD(0x44, w44);
#undef NDS_IT_WORD

    out->is_display_xlu = (w10 >> 31) & 1u;
    out->is_item_dobjs = (w10 >> 30) & 1u;
    out->is_display_colanim = (w10 >> 29) & 1u;
    out->is_give_hitlag = (w10 >> 28) & 1u;
    out->weight = (w10 >> 27) & 1u;
    out->attack_offset0_x = (s16)ndsItSignExtend((w10 >> 11) & 0xffffu, 16);
    out->attack_offset0_y = (s16)ndsItSignExtend((w14 >> 16) & 0xffffu, 16);
    out->attack_offset0_z = (s16)ndsItSignExtend(w14 & 0xffffu, 16);
    out->attack_offset1_x = (s16)ndsItSignExtend((w18 >> 16) & 0xffffu, 16);
    out->attack_offset1_y = (s16)ndsItSignExtend(w18 & 0xffffu, 16);
    out->attack_offset1_z = (s16)ndsItSignExtend((w1c >> 16) & 0xffffu, 16);
    out->damage_coll_offset.x = (s16)ndsItSignExtend(w1c & 0xffffu, 16);
    out->damage_coll_offset.y = (s16)ndsItSignExtend((w20 >> 16) & 0xffffu, 16);
    out->damage_coll_offset.z = (s16)ndsItSignExtend(w20 & 0xffffu, 16);
    out->damage_coll_size.x = (s16)ndsItSignExtend((w24 >> 16) & 0xffffu, 16);
    out->damage_coll_size.y = (s16)ndsItSignExtend(w24 & 0xffffu, 16);
    out->damage_coll_size.z = (s16)ndsItSignExtend((w28 >> 16) & 0xffffu, 16);
    out->map_coll_top = (s16)ndsItSignExtend(w28 & 0xffffu, 16);
    out->map_coll_center = (s16)ndsItSignExtend((w2c >> 16) & 0xffffu, 16);
    out->map_coll_bottom = (s16)ndsItSignExtend(w2c & 0xffffu, 16);
    out->map_coll_width = (s16)ndsItSignExtend((w30 >> 16) & 0xffffu, 16);
    out->size = (u16)(w30 & 0xffffu);
    out->angle = ndsItSignExtend((w34 >> 22) & 0x3ffu, 10);
    out->knockback_scale = (w34 >> 12) & 0x3ffu;
    out->damage = (w34 >> 4) & 0xffu;
    out->element = w34 & 0xfu;
    out->knockback_weight = (w38 >> 22) & 0x3ffu;
    out->shield_damage = ndsItSignExtend((w38 >> 14) & 0xffu, 8);
    out->attack_count = (w38 >> 12) & 0x3u;
    out->can_setoff = (w38 >> 11) & 1u;
    out->hit_sfx = (u16)((w38 >> 1) & 0x3ffu);
    out->priority = (w3c >> 29) & 0x7u;
    out->can_rehit_item = (w3c >> 28) & 1u;
    out->can_rehit_fighter = (w3c >> 27) & 1u;
    out->can_hop = (w3c >> 26) & 1u;
    out->can_reflect = (w3c >> 25) & 1u;
    out->can_shield = (w3c >> 24) & 1u;
    out->knockback_base = (w3c >> 14) & 0x3ffu;
    out->type = (w3c >> 10) & 0xfu;
    out->hitstatus = (w3c >> 6) & 0xfu;
    out->drop_sfx = (u16)((w40 >> 22) & 0x3ffu);
    out->throw_sfx = (u16)((w40 >> 12) & 0x3ffu);
    out->smash_sfx = (u16)((w40 >> 2) & 0x3ffu);
    out->vel_scale = (u16)((w44 >> 23) & 0x1ffu);
    out->spin_speed = (u16)(w44 & 0xffffu);
    return TRUE;
}

static sb32 ndsItValidateLinkBombAttributes(const ITAttributes *attr)
{
    return (attr != NULL) && (attr->data != NULL) &&
           (attr->is_display_xlu == TRUE) &&
           (attr->is_item_dobjs == FALSE) &&
           (attr->is_display_colanim == TRUE) &&
           (attr->is_give_hitlag == TRUE) &&
           (attr->weight == nITWeightLight) &&
           (attr->damage_coll_size.x == 100) &&
           (attr->damage_coll_size.y == 100) &&
           (attr->damage_coll_size.z == 100) &&
           (attr->map_coll_top == 113) &&
           (attr->map_coll_center == 0) &&
           (attr->map_coll_bottom == -113) &&
           (attr->map_coll_width == 113) &&
           (attr->size == 220) && (attr->damage == 2) &&
           (attr->angle == 80) && (attr->knockback_scale == 20) &&
           (attr->knockback_base == 60) && (attr->attack_count == 1) &&
           (attr->type == nITTypeThrow) && (attr->vel_scale == 60) &&
           (attr->can_hop == TRUE) && (attr->can_reflect == TRUE) &&
           (attr->can_shield == TRUE);
}

ITAttackEvent *ndsItGetAttackEvent(const ITDesc *item_desc,
                                   const void *offset_token)
{
    static ITAttackEvent s_link_bomb_events[4];
    static void *s_link_bomb_events_file;
    u8 *raw;
    s32 i;

    if ((item_desc == NULL) || (item_desc->p_file == NULL) ||
        (*item_desc->p_file == NULL))
    {
        return NULL;
    }
    if (item_desc->kind != nITKindLinkBomb)
    {
        return NULL;
    }
    if (s_link_bomb_events_file == *item_desc->p_file)
    {
        return s_link_bomb_events;
    }
    raw = lbRelocGetFileData(u8*, *item_desc->p_file, offset_token);
    if (raw == NULL)
    {
        return NULL;
    }
    for (i = 0; i < 4; i++)
    {
        u32 word0, word1;
        memcpy(&word0, raw + i * 8, sizeof(word0));
        memcpy(&word1, raw + i * 8 + 4, sizeof(word1));
        s_link_bomb_events[i].timer = (u8)((word0 >> 24) & 0xffu);
        s_link_bomb_events[i].angle = ndsItSignExtend((word0 >> 14) & 0x3ffu, 10);
        s_link_bomb_events[i].damage = (word0 >> 6) & 0xffu;
        s_link_bomb_events[i].size = (u16)((word1 >> 16) & 0xffffu);
    }
    if ((s_link_bomb_events[0].timer != 0) ||
        (s_link_bomb_events[1].timer != 2) ||
        (s_link_bomb_events[2].timer != 4) ||
        (s_link_bomb_events[3].timer != 6) ||
        (s_link_bomb_events[0].angle != 361) ||
        (s_link_bomb_events[0].damage != 5) ||
        (s_link_bomb_events[0].size != 300) ||
        (s_link_bomb_events[1].size != 230) ||
        (s_link_bomb_events[2].size != 150) ||
        (s_link_bomb_events[3].size != 0))
    {
        return NULL;
    }
    s_link_bomb_events_file = *item_desc->p_file;
    return s_link_bomb_events;
}

static ITStruct *itManagerGetNextStructAlloc(void)
{
    ITStruct *ip = sNdsItemStructsFree;
    if (ip != NULL)
    {
        sNdsItemStructsFree = ip->next;
    }
    return ip;
}

void itManagerSetPrevStructAlloc(ITStruct *ip)
{
    if (ip != NULL)
    {
        ip->next = sNdsItemStructsFree;
        sNdsItemStructsFree = ip;
    }
}

void itManagerInitItems(void)
{
    ITStruct *pool;
    s32 i;

    pool = syTaskmanMalloc(sizeof(ITStruct) * ITEM_ALLOC_MAX, 8);
    sNdsItemStructsFree = pool;
    if (pool != NULL)
    {
        memset(pool, 0, sizeof(ITStruct) * ITEM_ALLOC_MAX);
        for (i = 0; i < ITEM_ALLOC_MAX - 1; i++)
        {
            pool[i].next = &pool[i + 1];
        }
        pool[ITEM_ALLOC_MAX - 1].next = NULL;
    }
    sNdsLinkBombAttributesFile = NULL;
    memset(&sNdsLinkBombAttributes, 0, sizeof(sNdsLinkBombAttributes));
    gITManagerDisplayMode = nDBDisplayModeMaster;
}

static sb32 itDisplayCheckItemVisible(ITStruct *ip)
{
    FTStruct *fp;
    if ((ip == NULL) || (ip->owner_gobj == NULL) || !(ip->is_hold))
    {
        return TRUE;
    }
    fp = ftGetStruct(ip->owner_gobj);
    return (fp->is_item_show && !fp->is_invisible) ? TRUE : FALSE;
}

/* Debug collision rendering is not a production display mode on DS. Preserve
 * the source visibility and OPA/XLU tree choice. ColAnim is different: its
 * pre-model EnvColor/render-mode stream is visible gameplay (LinkBomb's final
 * 96-tic yellow/red fuse), so emit the exact source pre/post state around the
 * DObj draw for the DS item renderer to capture. */
void itDisplayOPAProcDisplay(GObj *item_gobj)
{
    if (itDisplayCheckItemVisible(itGetStruct(item_gobj)))
    {
        gcDrawDObjTreeForGObj(item_gobj);
    }
}

void itDisplayXLUProcDisplay(GObj *item_gobj)
{
    if (itDisplayCheckItemVisible(itGetStruct(item_gobj)))
    {
        gcDrawDObjTreeDLLinksForGObj(item_gobj);
    }
}

void itDisplayColAnimOPAProcDisplay(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    if (itDisplayCheckItemVisible(ip))
    {
        gDPPipeSync(gSYTaskmanDLHeads[0]++);
        gDPSetCycleType(gSYTaskmanDLHeads[0]++, G_CYC_2CYCLE);
        gDPSetRenderMode(gSYTaskmanDLHeads[0]++, G_RM_PASS,
                         G_RM_AA_ZB_OPA_SURF2);
        if (ip->colanim.is_use_color1)
        {
            gDPSetEnvColor(gSYTaskmanDLHeads[0]++, ip->colanim.color1.r,
                           ip->colanim.color1.g, ip->colanim.color1.b,
                           ip->colanim.color1.a);
        }
        else
        {
            gDPSetEnvColor(gSYTaskmanDLHeads[0]++, 0, 0, 0, 0);
        }
        gcDrawDObjTreeForGObj(item_gobj);
        gDPPipeSync(gSYTaskmanDLHeads[0]++);
        gDPSetCycleType(gSYTaskmanDLHeads[0]++, G_CYC_1CYCLE);
        gDPSetRenderMode(gSYTaskmanDLHeads[0]++, G_RM_AA_ZB_OPA_SURF,
                         G_RM_AA_ZB_OPA_SURF2);
    }
}

void itDisplayColAnimXLUProcDisplay(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    if (itDisplayCheckItemVisible(ip))
    {
        gDPPipeSync(gSYTaskmanDLHeads[0]++);
        gDPSetCycleType(gSYTaskmanDLHeads[0]++, G_CYC_2CYCLE);
        gDPSetRenderMode(gSYTaskmanDLHeads[0]++, G_RM_PASS,
                         G_RM_AA_ZB_OPA_SURF2);
        gDPPipeSync(gSYTaskmanDLHeads[1]++);
        gDPSetCycleType(gSYTaskmanDLHeads[1]++, G_CYC_2CYCLE);
        gDPSetRenderMode(gSYTaskmanDLHeads[1]++, G_RM_PASS,
                         G_RM_AA_ZB_XLU_SURF2);
        if (ip->colanim.is_use_color1)
        {
            gDPSetEnvColor(gSYTaskmanDLHeads[0]++, ip->colanim.color1.r,
                           ip->colanim.color1.g, ip->colanim.color1.b,
                           ip->colanim.color1.a);
            gDPSetEnvColor(gSYTaskmanDLHeads[1]++, ip->colanim.color1.r,
                           ip->colanim.color1.g, ip->colanim.color1.b,
                           ip->colanim.color1.a);
        }
        else
        {
            gDPSetEnvColor(gSYTaskmanDLHeads[0]++, 0, 0, 0, 0);
            gDPSetEnvColor(gSYTaskmanDLHeads[1]++, 0, 0, 0, 0);
        }
        gcDrawDObjTreeDLLinksForGObj(item_gobj);
        gDPPipeSync(gSYTaskmanDLHeads[0]++);
        gDPSetCycleType(gSYTaskmanDLHeads[0]++, G_CYC_1CYCLE);
        gDPSetRenderMode(gSYTaskmanDLHeads[0]++, G_RM_AA_ZB_OPA_SURF,
                         G_RM_AA_ZB_OPA_SURF2);
        gDPPipeSync(gSYTaskmanDLHeads[1]++);
        gDPSetCycleType(gSYTaskmanDLHeads[1]++, G_CYC_1CYCLE);
        gDPSetRenderMode(gSYTaskmanDLHeads[1]++, G_RM_AA_ZB_XLU_SURF,
                         G_RM_AA_ZB_XLU_SURF2);
    }
}

GObj *itManagerMakeItem(GObj *parent_gobj, ITDesc *item_desc, Vec3f *pos,
                        Vec3f *vel, u32 flags)
{
    ITStruct *ip;
    GObj *item_gobj;
    ITAttributes *attr;
    void (*proc_display)(GObj *);

    if ((item_desc == NULL) || (item_desc->kind != nITKindLinkBomb) ||
        (item_desc->p_file == NULL) || (*item_desc->p_file == NULL) ||
        (pos == NULL) || (vel == NULL))
    {
        return NULL;
    }
    if (sNdsLinkBombAttributesFile != *item_desc->p_file)
    {
        if (!ndsItDecodeAttributes(*item_desc->p_file, item_desc->o_attributes,
                                   &sNdsLinkBombAttributes) ||
            !ndsItValidateLinkBombAttributes(&sNdsLinkBombAttributes))
        {
            return NULL;
        }
        sNdsLinkBombAttributesFile = *item_desc->p_file;
#if NDS_P2_LINK_BOMB_TOUR
        gNdsLinkBombTourAttrValidCount++;
#endif
    }
    attr = &sNdsLinkBombAttributes;
    ip = itManagerGetNextStructAlloc();
    if (ip == NULL)
    {
        return NULL;
    }
    memset(ip, 0, sizeof(*ip));
    item_gobj = gcMakeGObjSPAfter(nGCCommonKindItem, NULL, nGCCommonLinkIDItem,
                                  GOBJ_PRIORITY_DEFAULT);
    if (item_gobj == NULL)
    {
        itManagerSetPrevStructAlloc(ip);
        return NULL;
    }
    proc_display = attr->is_display_colanim ?
        (attr->is_display_xlu ? itDisplayColAnimXLUProcDisplay :
                                itDisplayColAnimOPAProcDisplay) :
        (attr->is_display_xlu ? itDisplayXLUProcDisplay : itDisplayOPAProcDisplay);
    gcAddGObjDisplay(item_gobj, proc_display, 11, GOBJ_PRIORITY_DEFAULT, ~0u);
    item_gobj->user_data.p = ip;
    ip->item_gobj = item_gobj;
    ip->kind = item_desc->kind;
    ip->type = attr->type;
    ip->physics.vel_air = *vel;
    ip->attr = attr;
    itMainSetSpinVelLR(item_gobj);
    itMainResetPlayerVars(item_gobj);
    ip->pickup_wait = ITEM_PICKUP_WAIT_DEFAULT;
    ip->weight = attr->weight;
    ip->is_hitlag_victim = attr->is_give_hitlag;
    ip->drop_sfx = attr->drop_sfx;
    ip->throw_sfx = attr->throw_sfx;
    ip->smash_sfx = attr->smash_sfx;
    ip->vel_scale = F_PCT_TO_DEC(attr->vel_scale);
    /* itmanager.c:285 explicitly clears the initial LR-derived spin after the
     * generic manager has established facing. Individual item statuses own any
     * visible spin from here. LinkBomb starts held, but keep the shared owner
     * source-exact for later P2-5 clients too. */
    ip->spin_step = 0.0F;
    ip->attack_coll.attack_state = item_desc->attack_state;
    ip->attack_coll.damage = attr->damage;
    ip->attack_coll.throw_mul = ITEM_THROW_DEFAULT;
    ip->attack_coll.stale = 1.0F;
    ip->attack_coll.element = attr->element;
    ip->attack_coll.offsets[0].x = attr->attack_offset0_x;
    ip->attack_coll.offsets[0].y = attr->attack_offset0_y;
    ip->attack_coll.offsets[0].z = attr->attack_offset0_z;
    ip->attack_coll.offsets[1].x = attr->attack_offset1_x;
    ip->attack_coll.offsets[1].y = attr->attack_offset1_y;
    ip->attack_coll.offsets[1].z = attr->attack_offset1_z;
    ip->attack_coll.size = attr->size * 0.5F;
    ip->attack_coll.angle = attr->angle;
    ip->attack_coll.knockback_scale = attr->knockback_scale;
    ip->attack_coll.knockback_weight = attr->knockback_weight;
    ip->attack_coll.knockback_base = attr->knockback_base;
    ip->attack_coll.can_setoff = attr->can_setoff;
    ip->attack_coll.shield_damage = attr->shield_damage;
    ip->attack_coll.fgm_id = attr->hit_sfx;
    ip->attack_coll.priority = attr->priority;
    ip->attack_coll.can_rehit_item = attr->can_rehit_item;
    ip->attack_coll.can_rehit_fighter = attr->can_rehit_fighter;
    ip->attack_coll.can_hop = attr->can_hop;
    ip->attack_coll.can_reflect = attr->can_reflect;
    ip->attack_coll.can_shield = attr->can_shield;
    ip->attack_coll.attack_count = attr->attack_count;
    ip->attack_coll.interact_mask = GMHITCOLLISION_FLAG_ALL;
    ip->attack_coll.motion_attack_id = nFTMotionAttackIDNone;
    ip->attack_coll.motion_count = ftParamGetMotionCount();
    ip->attack_coll.stat_flags.attack_id = nFTStatusAttackIDNull;
    ip->attack_coll.stat_count = ftParamGetStatUpdateCount();
    itMainClearAttackRecord(ip);
    ip->damage_coll.hitstatus = attr->hitstatus;
    ip->damage_coll.offset.x = attr->damage_coll_offset.x;
    ip->damage_coll.offset.y = attr->damage_coll_offset.y;
    ip->damage_coll.offset.z = attr->damage_coll_offset.z;
    ip->damage_coll.size.x = attr->damage_coll_size.x * 0.5F;
    ip->damage_coll.size.y = attr->damage_coll_size.y * 0.5F;
    ip->damage_coll.size.z = attr->damage_coll_size.z * 0.5F;
    ip->damage_coll.interact_mask = GMHITCOLLISION_FLAG_ALL;

    if (attr->data != NULL)
    {
        gcSetupCustomDObjsWithMObj(item_gobj, (DObjDesc *)attr->data,
                                   attr->p_mobjsubs, NULL,
                                   item_desc->transform_types.tk1,
                                   item_desc->transform_types.tk2,
                                   item_desc->transform_types.tk3);
        if ((attr->anim_joints != NULL) || (attr->p_matanim_joints != NULL))
        {
            gcAddAnimAll(item_gobj, attr->anim_joints,
                         attr->p_matanim_joints, 0.0F);
            gcPlayAnimAll(item_gobj);
        }
        lbCommonEjectTreeDObj(DObjGetStruct(item_gobj));
    }
    else
    {
        gcAddDObjForGObj(item_gobj, NULL);
    }
    ip->coll_data.p_translate = &DObjGetStruct(item_gobj)->translate.vec.f;
    ip->coll_data.p_lr = &ip->lr;
    ip->coll_data.map_coll.top = attr->map_coll_top;
    ip->coll_data.map_coll.center = attr->map_coll_center;
    ip->coll_data.map_coll.bottom = attr->map_coll_bottom;
    ip->coll_data.map_coll.width = attr->map_coll_width;
    ip->coll_data.p_map_coll = &ip->coll_data.map_coll;
    ip->coll_data.ignore_line_id = -1;
    ip->coll_data.update_tic = gMPCollisionUpdateTic;
    gcAddGObjProcess(item_gobj, itProcessProcItemMain, nGCProcessKindFunc, 3);
    gcAddGObjProcess(item_gobj, itProcessProcSearchHitAll, nGCProcessKindFunc, 1);
    gcAddGObjProcess(item_gobj, itProcessProcHitCollisions, nGCProcessKindFunc, 0);
    ip->proc_update = item_desc->proc_update;
    ip->proc_map = item_desc->proc_map;
    ip->proc_hit = item_desc->proc_hit;
    ip->proc_shield = item_desc->proc_shield;
    ip->proc_hop = item_desc->proc_hop;
    ip->proc_setoff = item_desc->proc_setoff;
    ip->proc_reflector = item_desc->proc_reflector;
    ip->proc_damage = item_desc->proc_damage;
    ip->coll_data.pos_prev = DObjGetStruct(item_gobj)->translate.vec.f = *pos;
    if ((flags & ITEM_FLAG_COLLPROJECT) &&
        ((flags & ITEM_MASK_PARENT) == ITEM_FLAG_PARENT_FIGHTER))
    {
        mpCommonRunItemCollisionDefault(item_gobj,
            ftGetStruct(parent_gobj)->coll_data.p_translate,
            &ftGetStruct(parent_gobj)->coll_data);
    }
    ip->ga = nMPKineticsAir;
    itProcessUpdateAttackPositions(item_gobj);
    itMainClearColAnim(item_gobj);
    return item_gobj;
}

void itMainSetCommonSpin(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    ip->spin_step = (ip->attr->spin_speed != 0) ?
        F_PCT_TO_DEC(ip->attr->spin_speed) * ITEM_SPIN_SPEED_COMMON : 0.0F;
    if (ip->lr == -1) ip->spin_step = -ip->spin_step;
}

void itMainSetThrownSpin(GObj *item_gobj, Vec3f *vel, sb32 is_smash_throw)
{
    ITStruct *ip = itGetStruct(item_gobj);
    ip->spin_step = is_smash_throw ? ITEM_SPIN_SPEED_SMASH_THROW :
                                     ITEM_SPIN_SPEED_NORMAL_THROW;
    if (vel->x < 0.0F) ip->spin_step = -ip->spin_step;
    ip->spin_step = (ip->attr->spin_speed != 0) ?
        F_PCT_TO_DEC(ip->attr->spin_speed) * ip->spin_step : 0.0F;
}

void itMainSetSpinVelLR(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    ip->lr = (ip->physics.vel_air.x >= 0.0F) ? +1 : -1;
    itMainSetCommonSpin(item_gobj);
}

void itMainApplyGravityClampTVel(ITStruct *ip, f32 gravity, f32 terminal_velocity)
{
    ip->physics.vel_air.y -= gravity;
    if (lbCommonMag2D(&ip->physics.vel_air) > terminal_velocity)
    {
        lbCommonNormDist2D(&ip->physics.vel_air);
        lbCommonScale2D(&ip->physics.vel_air, terminal_velocity);
    }
}

void itMainResetPlayerVars(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    ip->owner_gobj = NULL;
    ip->team = ITEM_TEAM_DEFAULT;
    ip->player = ITEM_PORT_DEFAULT;
    ip->handicap = ITEM_HANDICAP_DEFAULT;
    ip->player_num = 0;
    ip->attack_coll.throw_mul = ITEM_THROW_DEFAULT;
    ip->display_mode = gITManagerDisplayMode;
}

void itMainClearAttackRecord(ITStruct *ip)
{
    s32 i;
    for (i = 0; i < (s32)ARRAY_COUNT(ip->attack_coll.attack_records); i++)
    {
        GMAttackRecord *record = &ip->attack_coll.attack_records[i];
        record->victim_gobj = NULL;
        record->victim_flags.is_interact_hurt = FALSE;
        record->victim_flags.is_interact_shield = FALSE;
        record->victim_flags.is_interact_reflect = FALSE;
        record->victim_flags.timer_rehit = 0;
        record->victim_flags.group_id = 7;
    }
}

void itMainRefreshAttackColl(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    itMainClearAttackRecord(ip);
    ip->attack_coll.attack_state = nGMAttackStateNew;
    itProcessUpdateAttackPositions(item_gobj);
}

void itMainClearOwnerStats(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    ip->is_damage_all = TRUE;
    ip->owner_gobj = NULL;
    ip->team = ITEM_TEAM_DEFAULT;
}

s32 itMainGetDamageOutput(ITStruct *ip)
{
    s32 damage;
    if (ip->is_thrown)
    {
        f32 mag = syVectorMag3D(&ip->physics.vel_air) * 0.1F;
        damage = (ip->attack_coll.damage + mag) * ip->attack_coll.throw_mul;
    }
    else damage = ip->attack_coll.damage;
    return (damage * ip->attack_coll.stale) + 0.999F;
}

void itMainDestroyItem(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
#if NDS_P2_LINK_BOMB_TOUR
    ndsLinkBombTourProofItemDestroy(item_gobj);
#endif
    if ((ip->is_hold) && (ip->owner_gobj != NULL))
    {
        ftGetStruct(ip->owner_gobj)->item_gobj = NULL;
        ndsItParamSetHammerParams(ip->owner_gobj);
    }
    else
    {
        efManagerDustExpandLargeMakeEffect(
            &DObjGetStruct(item_gobj)->translate.vec.f);
    }
    if (ip->arrow_gobj != NULL) gcEjectGObj(ip->arrow_gobj);
    itManagerSetPrevStructAlloc(ip);
    gcEjectGObj(item_gobj);
}

static void ndsItParamLinkResetShieldModelParts(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    if ((fp->fkind == nFTKindLink) || (fp->fkind == nFTKindNLink))
    {
        ftParamSetModelPartDefaultID(fighter_gobj, 21, 0);
        ftParamSetModelPartDefaultID(fighter_gobj, 19, -1);
    }
}

static void ndsItParamSetHammerParams(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    if ((fp->fkind == nFTKindLink) || (fp->fkind == nFTKindNLink))
    {
        ftParamSetModelPartDefaultID(fighter_gobj, 21, -1);
        ftParamSetModelPartDefaultID(fighter_gobj, 19, 0);
    }
    ftParamTryUpdateItemMusic();
}

void itMainSetFighterRelease(GObj *item_gobj, Vec3f *vel, f32 throw_mul,
                             u16 stat_flags, u16 stat_count)
{
    ITStruct *ip = itGetStruct(item_gobj);
    GObj *fighter_gobj = ip->owner_gobj;
    FTStruct *fp = ftGetStruct(fighter_gobj);
    Vec3f pos = { 0.0F, 0.0F, 0.0F };
    s32 joint_id;
    lbCommonEjectTreeDObj(DObjGetStruct(item_gobj));
    joint_id = (ip->weight == nITWeightHeavy) ? fp->attr->joint_itemheavy_id :
                                                fp->attr->joint_itemlight_id;
    gmCollisionGetFighterPartsWorldPosition(fp->joints[joint_id], &pos);
    DObjGetStruct(item_gobj)->translate.vec.f.x = pos.x;
    DObjGetStruct(item_gobj)->translate.vec.f.y = pos.y;
    DObjGetStruct(item_gobj)->translate.vec.f.z = 0.0F;
    mpCommonRunItemCollisionDefault(item_gobj, fp->coll_data.p_translate,
                                    &fp->coll_data);
    fp->item_gobj = NULL;
    ip->is_hold = FALSE;
    ip->physics.vel_air = *vel;
    syVectorScale3D(&ip->physics.vel_air, ip->vel_scale);
    ip->times_thrown++;
    ip->is_thrown = TRUE;
    ip->attack_coll.throw_mul = throw_mul;
    ip->attack_coll.stat_flags = *(GMStatFlags *)&stat_flags;
    ip->attack_coll.stat_count = stat_count;
    ndsItParamSetHammerParams(fighter_gobj);
    itMainRefreshAttackColl(item_gobj);
}

void itMainSetFighterDrop(GObj *item_gobj, Vec3f *vel, f32 throw_mul)
{
    ITStruct *ip = itGetStruct(item_gobj);
    FTStruct *fp = ftGetStruct(ip->owner_gobj);
#if NDS_P2_LINK
    if (ip->kind == nITKindLinkBomb) itLinkBombDroppedSetStatus(item_gobj);
#endif
    itMainSetFighterRelease(item_gobj, vel, throw_mul,
                            nFTStatusAttackIDItemThrow, fp->stat_count);
    func_800269C0_275C0(ip->drop_sfx);
}

void itMainSetFighterThrow(GObj *item_gobj, Vec3f *vel, f32 throw_mul,
                           sb32 is_smash_throw)
{
    ITStruct *ip = itGetStruct(item_gobj);
    FTStruct *fp = ftGetStruct(ip->owner_gobj);
    if (ip->weight == nITWeightLight)
    {
        if (is_smash_throw != FALSE)
        {
            ftParamMakeRumble(fp, 6, 0);
        }
    }
    else
    {
        ftParamMakeRumble(fp, (is_smash_throw != FALSE) ? 9 : 6, 0);
    }
#if NDS_P2_LINK
    if (ip->kind == nITKindLinkBomb) itLinkBombThrownSetStatus(item_gobj);
#endif
    itMainSetFighterRelease(item_gobj, vel, throw_mul,
                            fp->stat_flags.halfword, fp->stat_count);
    efManagerSparkleWhiteScaleMakeEffect(&DObjGetStruct(item_gobj)->translate.vec.f,
                                         1.0F);
    func_800269C0_275C0(is_smash_throw ? ip->smash_sfx : ip->throw_sfx);
    itMainSetThrownSpin(item_gobj, vel, is_smash_throw);
}

void itMainSetFighterHold(GObj *item_gobj, GObj *fighter_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    FTStruct *fp = ftGetStruct(fighter_gobj);
    DObj *joint;
    Vec3f pos = { 0.0F, 0.0F, 0.0F };
    s32 joint_id;
    fp->item_gobj = item_gobj;
    ip->owner_gobj = fighter_gobj;
    ip->is_allow_pickup = FALSE;
    ip->is_hold = TRUE;
    ip->team = fp->team;
    ip->player = fp->player;
    ip->handicap = fp->handicap;
    ip->player_num = fp->player_num;
    ip->physics.vel_air.x = ip->physics.vel_air.y = ip->physics.vel_air.z = 0.0F;
    ip->display_mode = fp->display_mode;
    itMapSetAir(ip);
    joint = gcAddDObjForGObj(item_gobj, NULL);
    joint->sib_prev->sib_next = NULL;
    joint->sib_prev = NULL;
    joint->child = DObjGetStruct(item_gobj);
    DObjGetStruct(item_gobj)->parent = joint;
    item_gobj->obj = joint;
    gcAddXObjForDObjFixed(joint, 0x52, 0);
    joint_id = (ip->weight == nITWeightHeavy) ? fp->attr->joint_itemheavy_id :
                                                fp->attr->joint_itemlight_id;
    joint->user_data.p = fp->joints[joint_id];
    gmCollisionGetFighterPartsWorldPosition(fp->joints[joint_id], &pos);
    efManagerItemGetSwirlProcUpdate(&pos);
    gcSetDObjTransformsForGObj(item_gobj, (DObjDesc *)ip->attr->data);
#if NDS_P2_LINK
    if (ip->kind == nITKindLinkBomb) itLinkBombHoldSetStatus(item_gobj);
#endif
    ndsItParamLinkResetShieldModelParts(fighter_gobj);
    if (ip->weight == nITWeightLight)
    {
        func_800269C0_275C0(nSYAudioFGMItemGet);
    }
    ftParamMakeRumble(fp, 6, 0);
    ip->pickup_wait = ITEM_PICKUP_WAIT_DEFAULT;
}

void itMainSetStatus(GObj *item_gobj, ITStatusDesc *status_desc, s32 status_id)
{
    ITStruct *ip = itGetStruct(item_gobj);
    ip->proc_update = status_desc[status_id].proc_update;
    ip->proc_map = status_desc[status_id].proc_map;
    ip->proc_hit = status_desc[status_id].proc_hit;
    ip->proc_shield = status_desc[status_id].proc_shield;
    ip->proc_hop = status_desc[status_id].proc_hop;
    ip->proc_setoff = status_desc[status_id].proc_setoff;
    ip->proc_reflector = status_desc[status_id].proc_reflector;
    ip->proc_damage = status_desc[status_id].proc_damage;
    ip->is_thrown = FALSE;
    ip->attack_coll.stat_flags.attack_id = nFTStatusAttackIDNull;
    ip->attack_coll.stat_flags.is_smash_attack = 0;
    ip->attack_coll.stat_flags.ga = 0;
    ip->attack_coll.stat_flags.is_projectile = 0;
    ip->attack_coll.stat_count = ftParamGetStatUpdateCount();
}

sb32 itMainCheckSetColAnimID(GObj *item_gobj, s32 colanim_id, s32 duration)
{
    return ftParamCheckSetColAnimID(&itGetStruct(item_gobj)->colanim,
                                    colanim_id, duration);
}

void itMainClearColAnim(GObj *item_gobj)
{
    ftParamResetColAnim(&itGetStruct(item_gobj)->colanim);
}

void itMainVelSetRebound(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    ip->physics.vel_air.x *= -0.06F;
    ip->physics.vel_air.y = (ip->physics.vel_air.y * -0.3F) + 25.0F;
}

sb32 itMainCommonProcHop(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    syVectorRotateAbout3D(&ip->physics.vel_air, &ip->shield_collide_dir,
                          ip->shield_collide_angle * 2.0F);
    itMainSetSpinVelLR(item_gobj);
    return FALSE;
}

sb32 itMainCommonProcReflector(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    FTStruct *fp = ftGetStruct(ip->owner_gobj);
    if ((ip->physics.vel_air.x * fp->lr) < 0.0F)
    {
        ip->physics.vel_air.x = -ip->physics.vel_air.x;
    }
    return FALSE;
}

/* Generic source-owned runtime. These contain no per-item registry tables. */
#include "../../decomp/BattleShip-main/decomp/src/it/itmap.c"
#include "../../decomp/BattleShip-main/decomp/src/it/itprocess.c"
#include "../../decomp/BattleShip-main/decomp/src/it/itvisuals.c"

#endif /* NDS_P2_ITEM_CORE */
