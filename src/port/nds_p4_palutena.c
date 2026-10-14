/*
 * P6 Palutena: the routines of her authored specials (docs/P6/characters/
 * palutena.md, move card P1): Explosive Flame (neutral special), Warp (up
 * special) and Counter / Reflect (down special). She runs Fox's status code,
 * the setup parent of her base Peach (fp->fkind == nFTKindFox), without
 * Peach's Remix behaviour: Peach's overrides (the float, the parasol fall,
 * the turnip) belong to Peach's content and hers name none of them. Her
 * export keeps Peach's special rows, which name Peach's ports, so this file
 * compiles nds_p4_peach.c when Peach's code is not compiled.
 *
 * The statuses are her appended rows (scripts/p6/palutena/spec.json
 * "statuses"; each ground status's air twin follows it), entered from her
 * special starters (the spec's ground_nsp ... air_dsp rows). Their scripts
 * (scripts/p6/palutena/motions/) set the flags read here: Explosive Flame's
 * flag0 at each burst and flag1 at the blast, the counter's flag0 while its
 * window is open.
 *
 * status_vars words: 0 Explosive Flame's distance (units), the warp's angle
 * (as f32 bits) or the counter's hit; 1 the warp's frames left.
 */
#include <nds/nds_p4.h>

#if NDS_P4_CODE_PALUTENA

#if !NDS_P4_CODE_PEACH
#include "nds_p4_peach.c"
#endif

#include <ef/effect.h>
#include <ft/ftcomputer.h>
#include <macros.h>
#include <sys/audio.h>
#include <sys/utils.h>
#if NDS_P2_KIRBY
#include "kirby/nds_p4_kirby.generated.h"
#endif

/* scripts/p6/palutena/spec.json "statuses"; a ground status's air twin is
 * the next id. */
#define PAL_STATUS_FLAME 0xF6
#define PAL_STATUS_WARP_START 0xF8
#define PAL_STATUS_WARP 0xFA
#define PAL_STATUS_WARP_END 0xFC
#define PAL_STATUS_COUNTER 0xFE
#define PAL_STATUS_COUNTER_HIT 0x100
#define PAL_AIR 1

/* After-image, colour animation and hitboxes carried across a ground/air
 * switch (Ike's, Roy's and Marth's word). */
#define PAL_PRESERVE_SWITCH 0x2003u

/* Explosive Flame (card): the stick at the cast picks the distance -- back
 * (against her facing) near, neutral middle, forward far -- at her height.
 * The script's hitboxes sit on TopN; this keeps them at the distance. */
#define PAL_FLAME_NEAR 600.0F
#define PAL_FLAME_MID 1000.0F
#define PAL_FLAME_FAR 1400.0F
#define PAL_FLAME_HEIGHT 320.0F
#define PAL_FLAME_STICK 20
/* In the air she nearly hangs while casting. */
#define PAL_FLAME_AIR_FALL_MAX 12.0F

/* Warp (card): she vanishes at the end of the start, travels 10 frames at
 * 100 units a frame along the stick (eight directions; neutral is up),
 * stops at walls and floors and reappears; from the air she ends in
 * special fall. */
#define PAL_WARP_FRAMES 10
#define PAL_WARP_SPEED 100.0F
#define PAL_WARP_STICK 11
#define PAL_WARP_DRIFT 0.6F

/* The counter's window holds her as Marth's does (0x48000000), and
 * reflects projectiles (Fox's rule) with Ness's reflector kind (no status
 * change on a reflect). */
#define PAL_COUNTER_ARMOUR 131072.0F

static FTSpecialColl sNdsP4PalutenaReflectColl = {
    nFTSpecialCollKindNessReflector, nFTPartsJointCommonStart,
    { 0.0F, 120.0F, 0.0F }, { 330.0F, 330.0F, 330.0F }, 50
};

static s32 *ndsP4PalutenaVar(FTStruct *fp, s32 k)
{
    return &((s32 *)(void *)&fp->status_vars)[k];
}

static f32 *ndsP4PalutenaVarF(FTStruct *fp, s32 k)
{
    return &((f32 *)(void *)&fp->status_vars)[k];
}

static void ndsP4PalutenaSetStatus(GObj *fighter_gobj, s32 status_id)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    ftMainSetStatus(fighter_gobj, status_id, 0.0F, 1.0F, FTSTATUS_PRESERVE_NONE);
    ftMainPlayAnimEventsAll(fighter_gobj);
    fp->motion_vars.flags.flag0 = 0;
    fp->motion_vars.flags.flag1 = 0;
    fp->motion_vars.flags.flag2 = 0;
}

/* A status's ground or air twin, at the same frame. */
static void ndsP4PalutenaToAir(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    mpCommonSetFighterAir(fp);
    ftMainSetStatus(fighter_gobj, fp->status_id + PAL_AIR, fighter_gobj->anim_frame, 1.0F,
                    PAL_PRESERVE_SWITCH);
    ftPhysicsClampAirVelXMax(fp);
}

static void ndsP4PalutenaToGround(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    mpCommonSetFighterGround(fp);
    ftMainSetStatus(fighter_gobj, fp->status_id - PAL_AIR, fighter_gobj->anim_frame, 1.0F,
                    PAL_PRESERVE_SWITCH);
}

void ndsP4PalutenaGroundMap(GObj *fighter_gobj)
{
    mpCommonProcFighterOnEdge(fighter_gobj, ndsP4PalutenaToAir);
}

void ndsP4PalutenaAirMap(GObj *fighter_gobj)
{
    mpCommonProcFighterLanding(fighter_gobj, ndsP4PalutenaToGround);
}

/* The model-space point `forward` ahead of her (facing) and `up` above her
 * feet. */
static void ndsP4PalutenaAhead(GObj *fighter_gobj, f32 forward, f32 up, Vec3f *out)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    *out = DObjGetStruct(fighter_gobj)->translate.vec.f;
    out->x += forward * (f32)fp->lr;
    out->y += up;
}

/* ---- Explosive Flame ---- */

/* The cast's distance: the stick's (back near, neutral middle, forward
 * far); a CPU's, the one nearest its target ahead (card "CPU": the TopN
 * offsets reach that far times the fighter's size). */
static f32 ndsP4PalutenaFlameDistance(FTStruct *fp)
{
    s32 stick = fp->input.pl.stick_range.x * fp->lr;

    if (fp->pkind == nFTPlayerKindCom)
    {
        f32 ahead = (fp->computer.target_pos.x - fp->joints[nFTPartsJointTopN]->translate.vec.f.x) *
                    (f32)fp->lr / fp->attr->size;

        return (ahead < ((PAL_FLAME_NEAR + PAL_FLAME_MID) * 0.5F)) ? PAL_FLAME_NEAR :
               (ahead < ((PAL_FLAME_MID + PAL_FLAME_FAR) * 0.5F)) ? PAL_FLAME_MID : PAL_FLAME_FAR;
    }
    return (stick >= PAL_FLAME_STICK) ? PAL_FLAME_FAR :
           (stick <= -PAL_FLAME_STICK) ? PAL_FLAME_NEAR : PAL_FLAME_MID;
}

static void ndsP4PalutenaFlameStart(GObj *fighter_gobj, s32 status_id)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    f32 distance = ndsP4PalutenaFlameDistance(fp);

    ndsP4PalutenaSetStatus(fighter_gobj, status_id);
    *ndsP4PalutenaVarF(fp, 0) = distance;
}

/* ground_nsp. */
void ndsP4PalutenaFlameGroundInitial(GObj *fighter_gobj)
{
    ndsP4PalutenaFlameStart(fighter_gobj, PAL_STATUS_FLAME);
}

/* air_nsp: the fall slowed. */
void ndsP4PalutenaFlameAirInitial(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (fp->physics.vel_air.y < 0.0F)
    {
        fp->physics.vel_air.y *= 0.25F;
    }
    ndsP4PalutenaFlameStart(fighter_gobj, PAL_STATUS_FLAME + PAL_AIR);
}

/* Every TopN hitbox at the cast's distance; the fire at each burst (flag0)
 * and the blast (flag1). */
void ndsP4PalutenaFlameMain(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    f32 distance = *ndsP4PalutenaVarF(fp, 0);
    Vec3f pos;
    s32 i;

    for (i = 0; i < ARRAY_COUNT(fp->attack_colls); i++)
    {
        if ((fp->attack_colls[i].attack_state != nGMAttackStateOff) &&
            (fp->attack_colls[i].joint_id == nFTPartsJointTopN))
        {
            fp->attack_colls[i].offset.z = distance;
        }
    }
    if (fp->motion_vars.flags.flag0 != 0)
    {
        fp->motion_vars.flags.flag0 = 0;
        ndsP4PalutenaAhead(fighter_gobj, distance, PAL_FLAME_HEIGHT, &pos);
        efManagerFlameRandomMakeEffect(&pos);
        efManagerSparkleWhiteMultiMakeEffect(&pos);
    }
    if (fp->motion_vars.flags.flag1 != 0)
    {
        fp->motion_vars.flags.flag1 = 0;
        ndsP4PalutenaAhead(fighter_gobj, distance, PAL_FLAME_HEIGHT, &pos);
        efManagerSparkleWhiteMultiExplodeMakeEffect(&pos);
        efManagerFlashLargeMakeEffect(&pos);
    }
    if (fighter_gobj->anim_frame <= 0.0F)
    {
        mpCommonSetFighterWaitOrFall(fighter_gobj);
    }
}

/* The air cast: a slow fall, the stick drifting her. */
void ndsP4PalutenaFlameAirPhysics(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    ftPhysicsApplyAirVelDrift(fighter_gobj);
    if (fp->physics.vel_air.y < -PAL_FLAME_AIR_FALL_MAX)
    {
        fp->physics.vel_air.y = -PAL_FLAME_AIR_FALL_MAX;
    }
}

/* ---- Warp ---- */

/* ground_usp. */
void ndsP4PalutenaWarpGroundInitial(GObj *fighter_gobj)
{
    ndsP4PalutenaSetStatus(fighter_gobj, PAL_STATUS_WARP_START);
}

/* air_usp: she stops. */
void ndsP4PalutenaWarpAirInitial(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    fp->physics.vel_air.x *= 0.25F;
    fp->physics.vel_air.y = 0.0F;
    ndsP4PalutenaSetStatus(fighter_gobj, PAL_STATUS_WARP_START + PAL_AIR);
}

static void ndsP4PalutenaVanish(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    Vec3f pos;

    fp->is_invisible = TRUE;
    fp->is_jostle_ignore = TRUE;
    ftParamSetHitStatusAll(fighter_gobj, nGMHitStatusIntangible);
    ndsP4PalutenaAhead(fighter_gobj, 0.0F, PAL_FLAME_HEIGHT, &pos);
    efManagerSparkleWhiteMultiMakeEffect(&pos);
}

/* The start's end: the stick (in eight directions, neutral up) gives the
 * travel's angle; on the ground a level or downward stick travels along the
 * ground, anything else lifts her. A CPU recovering aims at its recovery
 * target (ftComputer's, the ledge it makes for: card "CPU"). */
static void ndsP4PalutenaWarpGo(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    f32 x = (f32)(fp->input.pl.stick_range.x * fp->lr);
    f32 y = (f32)fp->input.pl.stick_range.y;
    f32 angle;
    s32 status_id;

    if ((fp->pkind == nFTPlayerKindCom) && (fp->computer.objective == nFTComputerObjectiveRecover))
    {
        x = (fp->computer.target_pos.x - fp->joints[nFTPartsJointTopN]->translate.vec.f.x) * (f32)fp->lr;
        y = fp->computer.target_pos.y - fp->joints[nFTPartsJointTopN]->translate.vec.f.y;
    }
    if (((x * x) + (y * y)) < (f32)(PAL_WARP_STICK * PAL_WARP_STICK))
    {
        angle = F_CST_DTOR32(90.0F);
    }
    else
    {
        /* To the nearest of eight directions. */
        angle = syUtilsArcTan2(y, x);
        angle = F_CST_DTOR32(45.0F) * (f32)(s32)((angle / F_CST_DTOR32(45.0F)) +
                                                 ((angle < 0.0F) ? -0.5F : 0.5F));
    }
    if ((fp->ga == nMPKineticsGround) && (angle <= 0.0F) && (angle > F_CST_DTOR32(-90.0F)))
    {
        status_id = PAL_STATUS_WARP;
        angle = 0.0F;
    }
    else if ((fp->ga == nMPKineticsGround) && (angle <= F_CST_DTOR32(-90.0F)))
    {
        status_id = PAL_STATUS_WARP;
        angle = F_CST_DTOR32(180.0F);
    }
    else
    {
        if (fp->ga == nMPKineticsGround)
        {
            mpCommonSetFighterAir(fp);
        }
        status_id = PAL_STATUS_WARP + PAL_AIR;
    }
    ftMainSetStatus(fighter_gobj, status_id, 0.0F, 1.0F, FTSTATUS_PRESERVE_NONE);
    fp = ftGetStruct(fighter_gobj);
    *ndsP4PalutenaVarF(fp, 0) = angle;
    *ndsP4PalutenaVar(fp, 1) = PAL_WARP_FRAMES;
    ndsP4PalutenaVanish(fighter_gobj);
}

void ndsP4PalutenaWarpStartMain(GObj *fighter_gobj)
{
    ftAnimEndCheckSetStatus(fighter_gobj, ndsP4PalutenaWarpGo);
}

/* The start in the air: she hangs. */
void ndsP4PalutenaWarpStartAirPhysics(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    fp->physics.vel_air.y = 0.0F;
    ftPhysicsApplyAirVelXFriction(fp, fp->attr);
}

static void ndsP4PalutenaWarpAppear(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    s32 status_id = (fp->ga == nMPKineticsGround) ? PAL_STATUS_WARP_END :
                                                    (PAL_STATUS_WARP_END + PAL_AIR);
    Vec3f pos;

    if (fp->ga == nMPKineticsGround)
    {
        fp->physics.vel_ground.x = 0.0F;
    }
    else
    {
        fp->physics.vel_air.x *= 0.1F;
        fp->physics.vel_air.y = 0.0F;
    }
    ndsP4PalutenaSetStatus(fighter_gobj, status_id);
    ndsP4PalutenaAhead(fighter_gobj, 0.0F, PAL_FLAME_HEIGHT, &pos);
    efManagerSparkleWhiteMultiMakeEffect(&pos);
}

/* The travel: invisible and intangible until its frames run out. */
void ndsP4PalutenaWarpMain(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    fp->is_invisible = TRUE;
    fp->is_jostle_ignore = TRUE;
    if (--*ndsP4PalutenaVar(fp, 1) <= 0)
    {
        ndsP4PalutenaWarpAppear(fighter_gobj);
    }
}

void ndsP4PalutenaWarpPhysics(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    f32 angle = *ndsP4PalutenaVarF(fp, 0);
    f32 x = PAL_WARP_SPEED * __cosf(angle);

    if (fp->ga == nMPKineticsGround)
    {
        fp->physics.vel_ground.x = x;
    }
    else
    {
        fp->physics.vel_air.x = x * (f32)fp->lr;
        fp->physics.vel_air.y = PAL_WARP_SPEED * __sinf(angle);
    }
}

/* Along the ground she goes on past an edge in the air. */
static void ndsP4PalutenaWarpOffEdge(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    mpCommonSetFighterAir(fp);
    ftMainSetStatus(fighter_gobj, PAL_STATUS_WARP + PAL_AIR, fighter_gobj->anim_frame, 1.0F,
                    FTSTATUS_PRESERVE_NONE);
    fp = ftGetStruct(fighter_gobj);
    fp->is_invisible = TRUE;
    fp->is_jostle_ignore = TRUE;
    ftParamSetHitStatusAll(fighter_gobj, nGMHitStatusIntangible);
}

void ndsP4PalutenaWarpGroundMap(GObj *fighter_gobj)
{
    mpCommonProcFighterOnFloor(fighter_gobj, ndsP4PalutenaWarpOffEdge);
}

static void ndsP4PalutenaWarpLand(GObj *fighter_gobj)
{
    mpCommonSetFighterGround(ftGetStruct(fighter_gobj));
    ndsP4PalutenaWarpAppear(fighter_gobj);
}

/* In the air: a ledge catches her, a floor ends the travel; a wall or a
 * ceiling stops it where it is. */
void ndsP4PalutenaWarpAirMap(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (mpCommonProcFighterCliff(fighter_gobj, ndsP4PalutenaWarpLand) != FALSE)
    {
        return;
    }
    if ((fp->coll_data.mask_curr & (MAP_FLAG_LWALL | MAP_FLAG_RWALL | MAP_FLAG_CEIL)) != 0u)
    {
        ndsP4PalutenaWarpAppear(fighter_gobj);
    }
}

/* The air reappearance ends in special fall. */
void ndsP4PalutenaWarpEndAirMain(GObj *fighter_gobj)
{
    if (fighter_gobj->anim_frame <= 0.0F)
    {
        ftCommonFallSpecialSetStatus(fighter_gobj, PAL_WARP_DRIFT, TRUE, TRUE, TRUE, 1.0F, FALSE);
    }
}

static void ndsP4PalutenaWarpEndLand(GObj *fighter_gobj)
{
    ftCommonLandingFallSpecialSetStatus(fighter_gobj, FALSE, 1.0F);
}

void ndsP4PalutenaWarpEndAirMap(GObj *fighter_gobj)
{
    mpCommonProcFighterLanding(fighter_gobj, ndsP4PalutenaWarpEndLand);
}

/* ---- Counter / Reflect ---- */

static s32 *ndsP4PalutenaCounterHit(FTStruct *fp)
{
    return ndsP4PalutenaVar(fp, 0);
}

static void ndsP4PalutenaCounterStart(GObj *fighter_gobj, s32 status_id)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    ndsP4PalutenaSetStatus(fighter_gobj, status_id);
    *ndsP4PalutenaCounterHit(fp) = 0;
    fp->special_coll = &sNdsP4PalutenaReflectColl;
}

/* ground_dsp. */
void ndsP4PalutenaCounterGroundInitial(GObj *fighter_gobj)
{
    ndsP4PalutenaCounterStart(fighter_gobj, PAL_STATUS_COUNTER);
}

/* air_dsp: the fall stopped. */
void ndsP4PalutenaCounterAirInitial(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    fp->physics.vel_air.y = 0.0F;
    ndsP4PalutenaCounterStart(fighter_gobj, PAL_STATUS_COUNTER + PAL_AIR);
}

/* Counter update (Marth's): armour and the reflector while the script
 * holds flag0; a counted hit turns her to face it and strikes back. */
void ndsP4PalutenaCounterMain(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    sb32 open = (fp->motion_vars.flags.flag0 != 0) ? TRUE : FALSE;

    fp->knockback_resist_status = (open != FALSE) ? PAL_COUNTER_ARMOUR : 0.0F;
    fp->is_reflect = open;
    if (*ndsP4PalutenaCounterHit(fp) != 0)
    {
        s32 status_id = (fp->ga == nMPKineticsGround) ? PAL_STATUS_COUNTER_HIT :
                                                        (PAL_STATUS_COUNTER_HIT + PAL_AIR);

        fp->knockback_resist_status = 0.0F;
        fp->is_reflect = FALSE;
        fp->lr = fp->damage_lr;
        fp->joints[nFTPartsJointTopN]->rotate.vec.f.y = F_CST_DTOR32(90.0F) * (f32)fp->lr;
        ndsP4PalutenaSetStatus(fighter_gobj, status_id);
        return;
    }
    if (fighter_gobj->anim_frame <= 0.0F)
    {
        fp->knockback_resist_status = 0.0F;
        fp->is_reflect = FALSE;
        mpCommonSetFighterWaitOrFall(fighter_gobj);
    }
}

/* The air counter: a slow fall. */
void ndsP4PalutenaCounterAirPhysics(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    ftPhysicsApplyAirVelDrift(fighter_gobj);
    if (fp->physics.vel_air.y < -PAL_FLAME_AIR_FALL_MAX)
    {
        fp->physics.vel_air.y = -PAL_FLAME_AIR_FALL_MAX;
    }
}

/* ftParamUpdateDamage's head: a hit landing while the counter's window is
 * open deals nothing and is counted (Marth's detection patch). */
static s32 ndsP4PalutenaUpdateDamage(FTStruct *fp, s32 damage)
{
    if (((fp->status_id == PAL_STATUS_COUNTER) || (fp->status_id == (PAL_STATUS_COUNTER + PAL_AIR))) &&
        (fp->motion_vars.flags.flag0 != 0))
    {
        *ndsP4PalutenaCounterHit(fp) = 1;
        return 0;
    }
    return damage;
}

const NDSP4Overrides gNdsP4PalutenaOverrides = {
    .update_damage = ndsP4PalutenaUpdateDamage,
};

#if NDS_P2_KIRBY
/* ---- Kirby's copy (A11; card "Kirby") ----
 *
 * Explosive Flame on Kirby's own PK Fire motions and scripts
 * (scripts/p4/p4_kirby.py CUSTOM_POWERS; 228_KirbyMainMotion.c
 * PKFireGround/Air: the cast's PSI sound, his call and psionic flash, and at
 * frame 20 the flag Ness's copy fires PK Fire on). At that flag her
 * explosion starts at the distance the stick picked at the cast, at her
 * height, timed as her script times it (scripts/p6/palutena/motions/
 * flame.ms): three 1% bursts two tics each, five apart, and the 15% blast
 * six tics after the third, three tics long. Kirby's script has no
 * hitboxes: these are made here as its MakeAttackColl would make hers, with
 * her sounds (p4_kirby.py "cues").
 *
 * status_vars words: 0 the distance (as f32 bits), 1 the tics since the
 * flag (-1 before it). */
#define PAL_KIRBY_BURST_EVERY 5
#define PAL_KIRBY_BURSTS 3
#define PAL_KIRBY_BURST_TICS 2
#define PAL_KIRBY_BLAST_AT 16
#define PAL_KIRBY_BLAST_TICS 3
/* Kirby's PK Fire switches between ground and air keeping his face, hurt
 * state, effects and colour (FTKIRBY_COPYNESS_SPECIALN_STATUS_FLAGS), and
 * here the explosion's hitbox too. */
#define PAL_KIRBY_PRESERVE_SWITCH (FTSTATUS_PRESERVE_TEXTUREPART | FTSTATUS_PRESERVE_HITSTATUS | \
                                   FTSTATUS_PRESERVE_EFFECT | FTSTATUS_PRESERVE_COLANIM | \
                                   FTSTATUS_PRESERVE_HIT)

/* Her script's MakeAttackColl on attack 0 (motions/flame.ms): fire on TopN
 * at the distance and her height, hitting ground and air. */
static void ndsP4PalutenaKirbyHit(FTStruct *fp, f32 distance, s32 damage, f32 size, s32 angle,
                                  s32 knockback_scale, s32 knockback_weight, s32 knockback_base,
                                  u32 fgm_level)
{
    FTAttackColl *attack_coll = &fp->attack_colls[0];

    attack_coll->group_id = 0;
    attack_coll->attack_state = nGMAttackStateNew;
    fp->is_attack_active = TRUE;
    ftParamClearAttackRecordID(fp, 0);
    attack_coll->joint_id = nFTPartsJointTopN;
    attack_coll->joint = fp->joints[nFTPartsJointTopN];
    attack_coll->damage = damage;
    attack_coll->can_rebound = FALSE;
    attack_coll->element = nGMHitElementFire;
    attack_coll->size = size * 0.5F;
    attack_coll->offset.x = 0.0F;
    attack_coll->offset.y = PAL_FLAME_HEIGHT;
    attack_coll->offset.z = distance;
    attack_coll->angle = angle;
    attack_coll->knockback_scale = knockback_scale;
    attack_coll->knockback_weight = knockback_weight;
    attack_coll->is_hit_air = TRUE;
    attack_coll->is_hit_ground = TRUE;
    attack_coll->shield_damage = 0;
    attack_coll->fgm_level = fgm_level;
    attack_coll->fgm_kind = 1;  /* fire */
    attack_coll->knockback_base = knockback_base;
    attack_coll->is_scale_pos = FALSE;
    attack_coll->motion_attack_id = fp->motion_attack_id;
    attack_coll->motion_count = fp->motion_count;
    attack_coll->damage = ftParamGetStaledDamage(fp->player, attack_coll->damage,
                                                 attack_coll->motion_attack_id,
                                                 attack_coll->motion_count);
}

static void ndsP4PalutenaKirbyNSPStart(GObj *fighter_gobj, s32 status_id)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    f32 distance = ndsP4PalutenaFlameDistance(fp);

    ndsP4PalutenaSetStatus(fighter_gobj, status_id);
    *ndsP4PalutenaVarF(fp, 0) = distance;
    *ndsP4PalutenaVar(fp, 1) = -1;
}

void ndsP4PalutenaKirbyNSPGroundInitial(GObj *fighter_gobj)
{
    ndsP4PalutenaKirbyNSPStart(fighter_gobj, NDS_P4_KIRBY_STATUS_PALUTENA_NSP_GROUND);
}

/* The fall slowed, as hers. */
void ndsP4PalutenaKirbyNSPAirInitial(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (fp->physics.vel_air.y < 0.0F)
    {
        fp->physics.vel_air.y *= 0.25F;
    }
    ndsP4PalutenaKirbyNSPStart(fighter_gobj, NDS_P4_KIRBY_STATUS_PALUTENA_NSP_AIR);
}

void ndsP4PalutenaKirbyNSPUpdate(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    f32 distance = *ndsP4PalutenaVarF(fp, 0);
    s32 *tic = ndsP4PalutenaVar(fp, 1);
    Vec3f pos;

    if ((fp->motion_vars.flags.flag0 != 0) && (*tic < 0))
    {
        *tic = 0;
    }
    fp->motion_vars.flags.flag0 = 0;
    if (*tic >= 0)
    {
        s32 t = *tic;

        if ((t < (PAL_KIRBY_BURST_EVERY * PAL_KIRBY_BURSTS)) && ((t % PAL_KIRBY_BURST_EVERY) == 0))
        {
            if (t == 0)
            {
                func_800269C0_275C0(NDS_P4_KIRBY_PALUTENA_FGM_BURST);
            }
            ndsP4PalutenaKirbyHit(fp, distance, 1, 320.0F, 90, 0, 30, 0, 0);
            ndsP4PalutenaAhead(fighter_gobj, distance, PAL_FLAME_HEIGHT, &pos);
            efManagerFlameRandomMakeEffect(&pos);
            efManagerSparkleWhiteMultiMakeEffect(&pos);
        }
        else if (t == PAL_KIRBY_BLAST_AT)
        {
            func_800269C0_275C0(NDS_P4_KIRBY_PALUTENA_FGM_BLAST);
            ndsP4PalutenaKirbyHit(fp, distance, 15, 430.0F, 361, 95, 0, 40, 2);
            ndsP4PalutenaAhead(fighter_gobj, distance, PAL_FLAME_HEIGHT, &pos);
            efManagerSparkleWhiteMultiExplodeMakeEffect(&pos);
            efManagerFlashLargeMakeEffect(&pos);
        }
        else if (((t < (PAL_KIRBY_BURST_EVERY * PAL_KIRBY_BURSTS)) &&
                  ((t % PAL_KIRBY_BURST_EVERY) == PAL_KIRBY_BURST_TICS)) ||
                 (t == (PAL_KIRBY_BLAST_AT + PAL_KIRBY_BLAST_TICS)))
        {
            ftParamClearAttackCollAll(fighter_gobj);
        }
        if (t <= (PAL_KIRBY_BLAST_AT + PAL_KIRBY_BLAST_TICS))
        {
            *tic = t + 1;
        }
    }
    if (fp->ga == nMPKineticsGround)
    {
        ftAnimEndSetWait(fighter_gobj);
    }
    else ftAnimEndSetFall(fighter_gobj);
}

/* The twin at the same frame, muted (Kirby's script must not replay its
 * cast's sounds). */
static void ndsP4PalutenaKirbyNSPSwitch(GObj *fighter_gobj, s32 status_id)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    ub32 is_muted = fp->is_muted;

    fp->is_muted = TRUE;
    ftMainSetStatus(fighter_gobj, status_id, fighter_gobj->anim_frame, 1.0F, PAL_KIRBY_PRESERVE_SWITCH);
    fp->is_muted = is_muted;
}

static void ndsP4PalutenaKirbyNSPToAir(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    mpCommonSetFighterAir(fp);
    ndsP4PalutenaKirbyNSPSwitch(fighter_gobj, NDS_P4_KIRBY_STATUS_PALUTENA_NSP_AIR);
    ftPhysicsClampAirVelXMax(fp);
}

static void ndsP4PalutenaKirbyNSPToGround(GObj *fighter_gobj)
{
    mpCommonSetFighterGround(ftGetStruct(fighter_gobj));
    ndsP4PalutenaKirbyNSPSwitch(fighter_gobj, NDS_P4_KIRBY_STATUS_PALUTENA_NSP_GROUND);
}

void ndsP4PalutenaKirbyNSPGroundMap(GObj *fighter_gobj)
{
    mpCommonProcFighterOnFloor(fighter_gobj, ndsP4PalutenaKirbyNSPToAir);
}

void ndsP4PalutenaKirbyNSPAirMap(GObj *fighter_gobj)
{
    mpCommonProcFighterLanding(fighter_gobj, ndsP4PalutenaKirbyNSPToGround);
}
#endif /* NDS_P2_KIRBY */

#endif /* NDS_P4_CODE_PALUTENA */
