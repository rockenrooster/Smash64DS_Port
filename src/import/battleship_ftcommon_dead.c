#include <ef/effect.h>
#include <ft/fighter.h>
#include <gm/gmsound.h>
#include <if/interface.h>
#include <it/item.h>
#include <sc/scene.h>
#include <nds/nds_p4_contents.h>
#if NDS_P4
#include <nds/nds_p4.h>
#endif

#ifndef DObjGetStruct
#define DObjGetStruct(gobj) ((DObj *)((gobj)->obj))
#endif

#ifndef CObjGetStruct
#define CObjGetStruct(gobj) ((CObj *)((gobj)->obj))
#endif

extern GObj *gGMCameraGObj;
typedef struct alSoundEffect alSoundEffect;

alSoundEffect *func_800269C0_275C0(u16 sfx_id);
void ifCommonBattleEndAddSoundQueueID(u16 sfx_id);
void ifCommonPlayerDamageStartBreakAnim(FTStruct *fp);
void ifCommonPlayerStockMakeStockSnap(FTStruct *fp);
void ifCommonPlayerScoreMakeEffect(FTStruct *fp, s32 score);
void ifCommonBattleUpdateScoreStocks(FTStruct *fp);
void ifCommonAnnounceEndMessage(void);
void sc1PGameSetPlayerDefeatStats(s32 player, s32 team_order);
void sc1PGameSpawnEnemyTeamNext(GObj *fighter_gobj);
void ftCommonSleepSetStatus(GObj *fighter_gobj);
void ftCommonRebirthDownSetStatus(GObj *fighter_gobj);
void ftManagerDestroyFighterWeapons(GObj *fighter_gobj);
void itMainDestroyItem(GObj *item_gobj);
GObj *efManagerQuakeMakeEffect(s32 quake_id);
GObj *efManagerDeadExplodeMakeEffect(Vec3f *pos, s32 player, u32 kind);
LBParticle *efManagerSparkleWhiteDeadMakeEffect(Vec3f *pos, f32 scale);
void ifScreenFlashSetColAnimID(s32 colanim_id, s32 colanim_duration);
void ftParamStopVoiceRunProcDamage(GObj *fighter_gobj);
void ftParamTryUpdateItemMusic(void);

#define ftCommonDeadCheckInterruptCommon \
    ndsBaseFTCommonDeadCheckInterruptCommon

sb32 ndsBaseFTCommonDeadCheckInterruptCommon(GObj *fighter_gobj);

#if NDS_P4
/* P4: Remix's refills on a fall (jigglypuffkirbyshared.asm kirby_blast_fix_1
 * gives Bowser his 20 flames back) run after the source's weapon cleanup, the
 * file's one call. */
#define ftManagerDestroyFighterWeapons(gobj_) \
    (ftManagerDestroyFighterWeapons(gobj_), ndsP4OnDead(gobj_))
#endif
#include "../../decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommondead.c"
#if NDS_P4
#undef ftManagerDestroyFighterWeapons
#endif

#undef ftCommonDeadCheckInterruptCommon

#include <nds/nds_fcmp.h>
#include <nds/nds_fixed_convert.h>
#include <string.h>

u16 syUtilsRandUShort(void);

/* ftcommondead.c:ftCommonDeadCheckInterruptCommon without soft float (owner
 * 2026-10-05: fixed point only), exact. Every fighter asks it every tick, and
 * each test promoted an s16 map bound to float (`__aeabi_i2f`) and compared
 * (`__aeabi_fcmp*`): ~32 conversions and 32 compares a frame. The bounds'
 * float order keys (nds_fcmp.h) are built once per ground (integer bit
 * construction, exact for |v| < 2^24), the position's two keys once a call,
 * and every test is one integer compare -- exact for every non-NaN pair. The
 * clamp writes `bound +- 500.0F` are exact integers in float, so their images
 * are built the same way. `syUtilsRandFloat() < (1.0F / 6.0F)` is
 * `syUtilsRandUShort() <= 10922`: the same generator step, u / 65536 <
 * 0.16666667163 exactly when u <= 10922. The 1P team bounds and the VS ones
 * are tested in the same order with the same outcomes, so one chain serves
 * both on a base index. Same branches, same order, same writes: the replay
 * digest is unchanged. Compact on purpose: it is ITCM-resident. */
enum
{
    NDS_DEAD_TOP,
    NDS_DEAD_BOTTOM,
    NDS_DEAD_RIGHT,
    NDS_DEAD_LEFT,
    NDS_DEAD_TEAM_TOP,
    NDS_DEAD_TEAM_BOTTOM,
    NDS_DEAD_TEAM_RIGHT,
    NDS_DEAD_TEAM_LEFT,
    NDS_DEAD_BOUNDS
};

static s16 sNdsDeadBoundsKey[NDS_DEAD_BOUNDS];
static u32 sNdsDeadBoundsOrder[NDS_DEAD_BOUNDS];
static u8 sNdsDeadBoundsValid;

static void __attribute__((noinline, cold)) ndsDeadBoundsRebuild(const s16 *key)
{
    u32 i;

    for (i = 0u; i < NDS_DEAD_BOUNDS; i++)
    {
        sNdsDeadBoundsKey[i] = key[i];
        sNdsDeadBoundsOrder[i] = ndsFcmpKey(ndsFixedToF32(key[i], 0u));
    }
    sNdsDeadBoundsValid = 1u;
}

static const u32 *ndsDeadBounds(void)
{
    const MPGroundData *gd = gMPCollisionGroundData;
    s16 key[NDS_DEAD_BOUNDS];

    key[NDS_DEAD_TOP] = gd->map_bound_top;
    key[NDS_DEAD_BOTTOM] = gd->map_bound_bottom;
    key[NDS_DEAD_RIGHT] = gd->map_bound_right;
    key[NDS_DEAD_LEFT] = gd->map_bound_left;
    key[NDS_DEAD_TEAM_TOP] = gd->map_bound_team_top;
    key[NDS_DEAD_TEAM_BOTTOM] = gd->map_bound_team_bottom;
    key[NDS_DEAD_TEAM_RIGHT] = gd->map_bound_team_right;
    key[NDS_DEAD_TEAM_LEFT] = gd->map_bound_team_left;
    if ((sNdsDeadBoundsValid == 0u) ||
        (memcmp(key, sNdsDeadBoundsKey, sizeof(key)) != 0))
    {
        ndsDeadBoundsRebuild(key);
    }
    return sNdsDeadBoundsOrder;
}

static void ndsDeadStopAir(FTStruct *fp)
{
    fp->physics.vel_air.x = 0.0F;
    fp->physics.vel_air.y = 0.0F;
    fp->physics.vel_air.z = 0.0F;
}

sb32 ftCommonDeadCheckInterruptCommon(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    Vec3f *pos = &fp->joints[nFTPartsJointTopN]->translate.vec.f;
    const u32 *bound;
    u32 kx;
    u32 ky;

    if ((fp->fkind == nFTKindBoss) || fp->is_ignore_dead)
    {
        return FALSE;
    }
    bound = ndsDeadBounds();
    kx = ndsFcmpKey(pos->x);
    ky = ndsFcmpKey(pos->y);

    if (fp->is_limit_map_bounds)
    {
        const MPGroundData *gd = gMPCollisionGroundData;

        if (ky < bound[NDS_DEAD_BOTTOM])
        {
            pos->y = ndsFixedToF32(gd->map_bound_bottom + 500, 0u);
            ndsDeadStopAir(fp);
        }
        else if (ky > bound[NDS_DEAD_TOP])
        {
            pos->y = ndsFixedToF32(gd->map_bound_top - 500, 0u);
            ndsDeadStopAir(fp);
        }
        if (kx > bound[NDS_DEAD_RIGHT])
        {
            pos->x = ndsFixedToF32(gd->map_bound_right - 500, 0u);
            ndsDeadStopAir(fp);
        }
        else if (kx < bound[NDS_DEAD_LEFT])
        {
            pos->x = ndsFixedToF32(gd->map_bound_left + 500, 0u);
            ndsDeadStopAir(fp);
        }
        return FALSE;
    }
    if (fp->is_ghost)
    {
        return FALSE;
    }
    if ((gSCManagerBattleState->game_type == nSCBattleGameType1PGame) &&
        (gSCManagerBattleState->players[fp->player].is_spgame_enemy != FALSE))
    {
        bound += NDS_DEAD_TEAM_TOP;
    }
    if (ky < bound[NDS_DEAD_BOTTOM])
    {
        ftCommonDeadDownSetStatus(fighter_gobj);
    }
    else if (kx > bound[NDS_DEAD_RIGHT])
    {
        ftCommonDeadRightSetStatus(fighter_gobj);
    }
    else if (kx < bound[NDS_DEAD_LEFT])
    {
        ftCommonDeadLeftSetStatus(fighter_gobj);
    }
    else if (ky > bound[NDS_DEAD_TOP])
    {
        if (syUtilsRandUShort() <= 10922u)
        {
            ftCommonDeadUpFallSetStatus(fighter_gobj);
        }
        else ftCommonDeadUpStarSetStatus(fighter_gobj);
    }
    else return FALSE;

    return TRUE;
}
