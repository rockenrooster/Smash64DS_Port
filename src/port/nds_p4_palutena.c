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
 * flag2 at the twinkle (its value the blast's sound), the counter's flag0
 * while its window is open.
 *
 * status_vars words: 0 Explosive Flame's distance (units), the warp's angle
 * (as f32 bits) or the counter's hit; 1 the warp's frames left, or whether
 * Explosive Flame's fire is made.
 */
#include <nds/nds_p4.h>

#if NDS_P4_CODE_PALUTENA

#if !NDS_P4_CODE_PEACH
#include "nds_p4_peach.c"
#endif

#include <ef/effect.h>
#include <ft/ftcomputer.h>
#include <macros.h>
#include <math.h>
#include <sys/audio.h>
#include <sys/utils.h>
#include <wp/weapon.h>
#if NDS_P2_KIRBY
#include "kirby/nds_p4_kirby.generated.h"
#endif

/* The source's particle effects (efmanager.c) and Quick Attack's angle
 * test (vector.c). */
extern LBParticle *efManagerFlameRandomMakeEffect(Vec3f *pos);
extern LBParticle *efManagerSparkleWhiteMultiMakeEffect(Vec3f *pos);
extern LBParticle *efManagerSparkleWhiteMultiExplodeMakeEffect(Vec3f *pos);
extern LBParticle *efManagerSparkleWhiteScaleMakeEffect(Vec3f *pos, f32 scale);
f32 syVectorAngleDiff3D(Vec3f *a, Vec3f *b);

/* The Fire Flower's flame (it/itcommon/itfflower.c): its weapon attributes
 * in the common item file start Explosive Flame's fire. */
extern void *gITManagerCommonData;
extern uintptr_t llITCommonDataFFlowerFlameWeaponAttributes;

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
 * (against her facing) near, neutral middle, forward far -- at her height,
 * as TopN offsets (times her size). Owner playtest 2026-10-14: the middle
 * 0.75x (1000 -> 750), the far 1.15x (1400 -> 1610; 1.5x in the first round
 * was too far), the near kept below the middle in the old ratio (600 ->
 * 450); the explosion twice the size (its effects stay their size), a white
 * twinkle where it will go off, no green flash ring, no ringing sound.
 *
 * Round 3: the fire is a weapon of its own, made where the twinkle shows
 * (her script's flag2, whose value names the blast's sound), so a hit on her
 * after the twinkle no longer cancels it. From PAL_FLAME_BURN_AT it burns as
 * the Fire Flower's flames do (their hit: fire, angle 0, a fixed weight-3
 * knockback, the small burn sound), every PAL_FLAME_BURN_EVERY tics hitting
 * again, so a fighter caught in it is held there; at PAL_FLAME_BLAST_AT the
 * blast, 1.25x the card's 15% (18.75), for PAL_FLAME_BLAST_TICS. The cast to
 * the blast is 0.85x the first rounds' (twinkle 12 -> 10, blast 34 -> 29).
 * It is hidden: her fire is the effects.
 *
 * Owner playtest 2026-10-14 (p6h): two fires a few tics apart. A ground/air
 * switch mid-cast sets the twin status at the same frame, and the source
 * fast-forwards its script there (ftMainSetStatus ->
 * ftMainPlayAnimEventsForward), setting flag2 again: a cast makes one fire
 * (status_vars word 1), and no fire is made while one of the fighter's own
 * is younger than PAL_FLAME_COOLDOWN tics. Owner p6i: one more double cast,
 * its trigger unknown: no fire is made while one of hers is burning at all
 * (a fire lives PAL_FLAME_BLAST_AT + PAL_FLAME_BLAST_TICS tics; her cast is
 * longer than that, so two casts never overlap). */
#define PAL_FLAME_NEAR 450.0F
#define PAL_FLAME_MID 750.0F
#define PAL_FLAME_FAR 1610.0F
#define PAL_FLAME_HEIGHT 320.0F
#define PAL_FLAME_STICK 20
#define PAL_FLAME_BURN_AT 5
#define PAL_FLAME_BURN_EVERY 5
#define PAL_FLAME_BLAST_AT 19
#define PAL_FLAME_BLAST_TICS 3
#define PAL_FLAME_BURN_DAMAGE 1
#define PAL_FLAME_BURN_SIZE 640.0F
#define PAL_FLAME_BURN_KNOCKBACK_WEIGHT 3
#define PAL_FLAME_BLAST_DAMAGE 19
#define PAL_FLAME_BLAST_SIZE 860.0F
#define PAL_FLAME_BLAST_ANGLE 361
#define PAL_FLAME_BLAST_KNOCKBACK_SCALE 95
#define PAL_FLAME_BLAST_KNOCKBACK_BASE 40
/* In the air she nearly hangs while casting. */
#define PAL_FLAME_AIR_FALL_MAX 12.0F

/* Warp (card; owner playtest 2026-10-14: twice the first 1000 units -- three
 * times was too far -- sliding along walls, white twinkles for the
 * effects): she vanishes at the end of the start, travels 10 frames at 200
 * units a frame along the stick (eight directions; neutral is up) and
 * reappears; from the air she ends in special fall. A wall or ceiling met
 * nearly head-on stops her; one met at a glancing angle she slides along
 * (Pikachu's Quick Attack's rule), so an upward warp beside a wall rides it
 * to the ledge. A CPU recovering travels only as far as its target. */
#define PAL_WARP_FRAMES 10
#define PAL_WARP_FRAMES_MIN 3
#define PAL_WARP_SPEED 200.0F
#define PAL_WARP_STICK 11
#define PAL_WARP_DRIFT 0.6F
#define PAL_WARP_HALT_ANGLE F_CST_DTOR32(150.0F)
#define PAL_TWINKLE_SCALE 2.0F

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

/* The white twinkle (the smash charge's) `forward` ahead of her at her
 * centre's height. */
static void ndsP4PalutenaTwinkle(GObj *fighter_gobj, f32 forward, f32 scale)
{
    Vec3f pos;

    ndsP4PalutenaAhead(fighter_gobj, forward, PAL_FLAME_HEIGHT, &pos);
    efManagerSparkleWhiteScaleMakeEffect(&pos, scale);
}

/* ---- Explosive Flame's fire ---- */

typedef struct NDSP4PalutenaFlameVars
{
    s32 tic;    /* since the twinkle */
    u32 fgm;    /* the blast's sound, 0 for none */
} NDSP4PalutenaFlameVars;

_Static_assert(sizeof(NDSP4PalutenaFlameVars) <= sizeof(((WPStruct *)0)->weapon_vars),
               "the fire's variables outgrow the weapon variables");

static NDSP4PalutenaFlameVars *ndsP4PalutenaFlameVars(WPStruct *wp)
{
    return (NDSP4PalutenaFlameVars *)(void *)&wp->weapon_vars;
}

/* The fire's hit from this tic on, a fresh one (every fighter may be hit
 * again). */
static void ndsP4PalutenaFlameHit(WPStruct *wp, s32 damage, f32 size, s32 angle, s32 knockback_scale,
                                  s32 knockback_weight, s32 knockback_base, u16 fgm)
{
    wp->attack_coll.damage = damage;
    wp->attack_coll.size = size * 0.5F;
    wp->attack_coll.angle = angle;
    wp->attack_coll.knockback_scale = knockback_scale;
    wp->attack_coll.knockback_weight = knockback_weight;
    wp->attack_coll.knockback_base = knockback_base;
    wp->attack_coll.element = nGMHitElementFire;
    wp->attack_coll.fgm_id = fgm;
    wp->attack_coll.attack_state = nGMAttackStateNew;
    wpMainClearAttackRecord(wp);
}

/* The burn's hits and flames, then the blast; gone after it. */
static sb32 ndsP4PalutenaFlameProcUpdate(GObj *weapon_gobj)
{
    WPStruct *wp = wpGetStruct(weapon_gobj);
    NDSP4PalutenaFlameVars *v = ndsP4PalutenaFlameVars(wp);
    Vec3f *pos = &DObjGetStruct(weapon_gobj)->translate.vec.f;
    s32 t = v->tic++;

    if (t == PAL_FLAME_BLAST_AT)
    {
        ndsP4PalutenaFlameHit(wp, PAL_FLAME_BLAST_DAMAGE, PAL_FLAME_BLAST_SIZE, PAL_FLAME_BLAST_ANGLE,
                              PAL_FLAME_BLAST_KNOCKBACK_SCALE, 0, PAL_FLAME_BLAST_KNOCKBACK_BASE,
                              nSYAudioFGMBurnL);
        if (v->fgm != 0)
        {
            func_800269C0_275C0(v->fgm);
        }
        efManagerSparkleWhiteMultiExplodeMakeEffect(pos);
    }
    else if ((t >= PAL_FLAME_BURN_AT) && (t < PAL_FLAME_BLAST_AT) &&
             (((t - PAL_FLAME_BURN_AT) % PAL_FLAME_BURN_EVERY) == 0))
    {
        ndsP4PalutenaFlameHit(wp, PAL_FLAME_BURN_DAMAGE, PAL_FLAME_BURN_SIZE, 0, 100,
                              PAL_FLAME_BURN_KNOCKBACK_WEIGHT, 0, nSYAudioFGMBurnS);
        efManagerFlameRandomMakeEffect(pos);
        efManagerSparkleWhiteMultiMakeEffect(pos);
    }
    return (t >= (PAL_FLAME_BLAST_AT + PAL_FLAME_BLAST_TICS - 1)) ? TRUE : FALSE;
}

/* The Fire Flower flame's kind and attributes; no map collision (it stays
 * where it was made) and nothing it meets ends it. */
static WPDesc sNdsP4PalutenaFlameDesc = {
    0x00, NDS_P4_WP_KIND_PALUTENA_FLAME, &gITManagerCommonData,
    (intptr_t)&llITCommonDataFFlowerFlameWeaponAttributes,
    { nGCMatrixKindTraRotRpyRSca, nGCMatrixKindNull, 0 },
    ndsP4PalutenaFlameProcUpdate, NULL, NULL, NULL, NULL, NULL, NULL, NULL
};

/* Whether one of the fighter's own fires is burning. */
static sb32 ndsP4PalutenaFlameCooling(GObj *fighter_gobj)
{
    GObj *weapon_gobj;

    for (weapon_gobj = gGCCommonLinks[nGCCommonLinkIDWeapon]; weapon_gobj != NULL;
         weapon_gobj = weapon_gobj->link_next)
    {
        WPStruct *wp = wpGetStruct(weapon_gobj);

        if ((wp != NULL) && (wp->kind == NDS_P4_WP_KIND_PALUTENA_FLAME) &&
            (wp->owner_gobj == fighter_gobj))
        {
            return TRUE;
        }
    }
    return FALSE;
}

/* The twinkle `distance` ahead at the flame's height (TopN offsets: times
 * her size), and the fire there, its blast sounding `fgm`; none while the
 * fighter's last is cooling. */
static void ndsP4PalutenaFlameMakeWeapon(GObj *fighter_gobj, f32 distance, u32 fgm)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    GObj *weapon_gobj;
    WPStruct *wp;
    Vec3f pos;

    if (ndsP4PalutenaFlameCooling(fighter_gobj))
    {
        return;
    }
    ndsP4PalutenaAhead(fighter_gobj, distance * fp->attr->size, PAL_FLAME_HEIGHT * fp->attr->size, &pos);
    efManagerSparkleWhiteScaleMakeEffect(&pos, 1.0F);
    weapon_gobj = wpManagerMakeWeapon(fighter_gobj, &sNdsP4PalutenaFlameDesc, &pos, WEAPON_FLAG_PARENT_FIGHTER);
    if (weapon_gobj == NULL)
    {
        return;
    }
    weapon_gobj->flags = GOBJ_FLAG_HIDDEN;
    wp = wpGetStruct(weapon_gobj);
    wp->attack_coll.attack_state = nGMAttackStateOff;
    wp->attack_coll.shield_damage = 0;
    wp->attack_coll.can_setoff = FALSE;
    wp->attack_coll.can_hop = FALSE;
    wp->attack_coll.can_reflect = FALSE;
    wp->attack_coll.can_absorb = FALSE;
    wp->lifetime = PAL_FLAME_BLAST_AT + PAL_FLAME_BLAST_TICS;
    ndsP4PalutenaFlameVars(wp)->tic = 0;
    ndsP4PalutenaFlameVars(wp)->fgm = fgm;
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
    *ndsP4PalutenaVar(fp, 1) = 0;
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

/* The fire at the cast's distance when the twinkle shows (flag2, its value
 * the blast's sound), once a cast: a ground/air switch's replay of the
 * script sets flag2 again. */
void ndsP4PalutenaFlameMain(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    s32 *made = ndsP4PalutenaVar(fp, 1);

    if (fp->motion_vars.flags.flag2 != 0)
    {
        if (*made == 0)
        {
            *made = 1;
            ndsP4PalutenaFlameMakeWeapon(fighter_gobj, *ndsP4PalutenaVarF(fp, 0), fp->motion_vars.flags.flag2);
        }
        fp->motion_vars.flags.flag2 = 0;
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

    fp->is_invisible = TRUE;
    fp->is_jostle_ignore = TRUE;
    ftParamSetHitStatusAll(fighter_gobj, nGMHitStatusIntangible);
    ndsP4PalutenaTwinkle(fighter_gobj, 0.0F, PAL_TWINKLE_SCALE);
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
    s32 frames = PAL_WARP_FRAMES;

    if ((fp->pkind == nFTPlayerKindCom) && (fp->computer.objective == nFTComputerObjectiveRecover))
    {
        x = (fp->computer.target_pos.x - fp->joints[nFTPartsJointTopN]->translate.vec.f.x) * (f32)fp->lr;
        y = fp->computer.target_pos.y - fp->joints[nFTPartsJointTopN]->translate.vec.f.y;
        frames = (s32)(sqrtf((x * x) + (y * y)) / PAL_WARP_SPEED) + 1;
        frames = (frames < PAL_WARP_FRAMES_MIN) ? PAL_WARP_FRAMES_MIN :
                 (frames > PAL_WARP_FRAMES) ? PAL_WARP_FRAMES : frames;
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
    *ndsP4PalutenaVar(fp, 1) = frames;
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
    ndsP4PalutenaTwinkle(fighter_gobj, 0.0F, PAL_TWINKLE_SCALE);
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
        /* The ground speed moves her only once it is carried along the
         * floor into her air velocity, as every ground physics proc does
         * (owner playtest 2026-10-14: a sideways warp on the ground did not
         * move). */
        fp->physics.vel_ground.x = x;
        ftPhysicsSetGroundVelTransferAir(fighter_gobj);
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
 * ceiling met within 30 degrees of head-on stops it where it is, and one met
 * at a glancing angle she slides along (ftPikachuSpecialAirHiProcMap's test:
 * the collision pushes her out and the travel's speed along the surface
 * goes on). */
void ndsP4PalutenaWarpAirMap(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    u32 mask;

    if (mpCommonProcFighterCliff(fighter_gobj, ndsP4PalutenaWarpLand) != FALSE)
    {
        return;
    }
    mask = fp->coll_data.mask_curr;
    if (((mask & MAP_FLAG_CEIL) &&
         (syVectorAngleDiff3D(&fp->coll_data.ceil_angle, &fp->physics.vel_air) > PAL_WARP_HALT_ANGLE)) ||
        ((mask & MAP_FLAG_LWALL) &&
         (syVectorAngleDiff3D(&fp->coll_data.lwall_angle, &fp->physics.vel_air) > PAL_WARP_HALT_ANGLE)) ||
        ((mask & MAP_FLAG_RWALL) &&
         (syVectorAngleDiff3D(&fp->coll_data.rwall_angle, &fp->physics.vel_air) > PAL_WARP_HALT_ANGLE)))
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

/* ---- Entry (her gate; card "Entry") ----
 *
 * Her entry_script row is her own (scripts/p6/palutena/spec.json kind_tables
 * "entry_script"): this case makes her gate, the DObjDesc tree at 0 of her
 * special file 2 (scripts/p6/p6_gate.py: the root at her entry position, a
 * node toward the camera, the two doors on their hinges), drawn natively as
 * an entry effect (scripts/p4/p4_articles.py's "gate" root). Its update parts
 * the doors from tic PAL_GATE_OPEN_AT over PAL_GATE_OPEN_TICS, each turning
 * PAL_GATE_OPEN_DEG on its hinge toward the camera, then shrinks the gate
 * away from PAL_GATE_SHRINK_AT and ejects it at PAL_GATE_END, inside her
 * entry clip (scripts/p6/palutena/motions/entry.ms times her sparkles to
 * the parting). The tic count is the effect's (effect_vars.common.size). */
#define PAL_GATE_OPEN_AT 10
#define PAL_GATE_OPEN_TICS 24
#define PAL_GATE_OPEN_DEG 80.0F
#define PAL_GATE_SHRINK_AT 84
#define PAL_GATE_END 100

extern void *gNdsP4PalutenaSpecial2;
void gcDrawDObjTreeForGObj(GObj *gobj);
static void ndsP4PalutenaGateProcUpdate(GObj *effect_gobj);

static EFDesc sNdsP4PalutenaGateDesc = {
    0x4 | EFFECT_FLAG_USERDATA, 10, &gNdsP4PalutenaSpecial2,
    { nGCMatrixKindTraRotRpyRSca, nGCMatrixKindNull, 0x00 },
    { nGCMatrixKindTraRotRpyRSca, nGCMatrixKindNull, 0x00 },
    ndsP4PalutenaGateProcUpdate, gcDrawDObjTreeForGObj,
    0x0, 0x0, 0x0, 0x0
};

static void ndsP4PalutenaGateProcUpdate(GObj *effect_gobj)
{
    EFStruct *ep = efGetStruct(effect_gobj);
    DObj *front = DObjGetStruct(effect_gobj)->child;
    DObj *left = front->child;
    DObj *right = left->sib_next;
    s32 tic = ep->effect_vars.common.size;
    f32 open = 1.0F;
    f32 scale = 1.0F;

    if (ep->is_pause_effect)
    {
        return;
    }
    if (tic >= PAL_GATE_END)
    {
        efManagerSetPrevStructAlloc(ep);
        gcEjectGObj(effect_gobj);
        return;
    }
    if (tic < PAL_GATE_OPEN_AT)
    {
        open = 0.0F;
    }
    else if (tic < (PAL_GATE_OPEN_AT + PAL_GATE_OPEN_TICS))
    {
        /* Eased out: fast as the doors part, slowing to rest. */
        f32 rest = (f32)(PAL_GATE_OPEN_AT + PAL_GATE_OPEN_TICS - tic) / (f32)PAL_GATE_OPEN_TICS;

        open = 1.0F - (rest * rest);
    }
    if (tic >= PAL_GATE_SHRINK_AT)
    {
        scale = (f32)(PAL_GATE_END - tic) / (f32)(PAL_GATE_END - PAL_GATE_SHRINK_AT);
    }
    left->rotate.vec.f.y = F_CST_DTOR32(PAL_GATE_OPEN_DEG) * open;
    right->rotate.vec.f.y = -F_CST_DTOR32(PAL_GATE_OPEN_DEG) * open;
    front->scale.vec.f.x = scale;
    front->scale.vec.f.y = scale;
    front->scale.vec.f.z = scale;
    ep->effect_vars.common.size = tic + 1;
}

/* Her up smash's light pillar (owner playtest 2026-10-14, round 2: ONE
 * column in front of her, exactly vertical, as Ultimate's Heavenly Light;
 * the first build drew it as model-part cards on her hips row, which her
 * animation turned behind her and doubled): an effect of the second tree in
 * her special file 2 (scripts/p6/p6_gate.py PILLAR_TREE: a root and its
 * cards facing the camera), made by the up smash's
 * flag0 (motions/usmash.ms) PAL_PILLAR_AHEAD units ahead of her on her
 * floor, world-aligned; its cards scaled over its life (owner p6j, below;
 * its hitboxes are the script's). The tic is in effect_vars.common.size, as
 * the gate's.
 *
 * Round 3: the hitboxes the column; 2,500 tall (2x her first, owner p6f
 * notes; 1.5x in round 3). The script's attacks 0
 * and 1 are its lower and upper halves: every frame each moves to the other
 * end of its half (PAL_PILLAR_COLUMN, TopN y; the script makes each at the
 * first), and a hitbox's test after its first frame sweeps from where it was
 * to where it is (gmCollisionTestRectangle's interpolated test), so each
 * covers its whole half, 400 wide.
 *
 * Owner p6h notes: 1.25x farther from her (260 -> 325; the script's
 * hitboxes with it), 1.5x the knockback, and its whoosh no longer stored
 * (ftMainPlayHitSFX stops a stored sound when a hit lands). Owner p6i: as
 * long as the 3DS's, its hitboxes up as long.
 *
 * Owner p6j ("redo the blue pillar column VFX since we have a good animation
 * to follow for it now"): the 3DS's Heavenly Light in the owner's half-speed
 * Strong_Up_A clip (a frame a tic; her clip maps its frames 11-23 onto hers
 * 0-12, the staff thrust up at the hit and flag0, frame 12, and its frames
 * 23-56 onto hers 12-42; the tic below is the 3DS's frame less about 24).
 * The tree's cards (p6_gate.py PILLAR_CARDS): the glow
 * (translucent), the core (white down its middle to cyan), the flare on the
 * floor at its foot and the dust either way along the floor (translucent).
 * The column rises from the floor with a round head over PAL_PILLAR_RISE
 * tics (its height sNdsP4PalutenaPillarRise), the flare at its foot fading by
 * PAL_PILLAR_FLARE_END; from PAL_PILLAR_DUST_AT the dust rolls out either
 * way, at its full reach by PAL_PILLAR_DUST_FULL, and settles (flattens) by
 * PAL_PILLAR_DUST_END (the 3DS's frames 26 to 38); the column stands, its
 * glow shimmering, and thins from PAL_PILLAR_THIN_AT to PAL_PILLAR_END (the
 * 3DS's frames 50 to 62; its hitboxes, the script's, are up to tic 26). */
#define PAL_PILLAR_AHEAD 325.0F
#define PAL_PILLAR_TOWARD_CAMERA 120.0F
#define PAL_PILLAR_RISE 3
#define PAL_PILLAR_FLARE_END 9
#define PAL_PILLAR_DUST_AT 2
#define PAL_PILLAR_DUST_FULL 7
#define PAL_PILLAR_DUST_END 13
#define PAL_PILLAR_THIN_AT 24
#define PAL_PILLAR_END 36

static const f32 sNdsP4PalutenaPillarRise[PAL_PILLAR_RISE] = { 0.18F, 0.45F, 1.0F };

static const f32 sNdsP4PalutenaPillarColumn[2][2] = {
    { 200.0F, 1250.0F },    /* attack 0: the floor to the middle */
    { 1250.0F, 2300.0F },   /* attack 1: the middle to the top */
};

static void ndsP4PalutenaPillarProcUpdate(GObj *effect_gobj);

static EFDesc sNdsP4PalutenaPillarDesc = {
    0x4 | EFFECT_FLAG_USERDATA, 10, &gNdsP4PalutenaSpecial2,
    { nGCMatrixKindTraRotRpyRSca, nGCMatrixKindNull, 0x00 },
    { nGCMatrixKindTraRotRpyRSca, nGCMatrixKindNull, 0x00 },
    ndsP4PalutenaPillarProcUpdate, gcDrawDObjTreeForGObj,
    0xDC, 0x0, 0x0, 0x0
};

static void ndsP4PalutenaPillarProcUpdate(GObj *effect_gobj)
{
    EFStruct *ep = efGetStruct(effect_gobj);
    DObj *glow = DObjGetStruct(effect_gobj)->child;
    DObj *core = glow->sib_next;
    DObj *flare = core->sib_next;
    DObj *dust_l = flare->sib_next;
    DObj *dust_r = dust_l->sib_next;
    s32 tic = ep->effect_vars.common.size;
    f32 width;
    f32 height;
    f32 flare_size;
    f32 reach;
    f32 rise;

    if (ep->is_pause_effect)
    {
        return;
    }
    if (tic >= PAL_PILLAR_END)
    {
        efManagerSetPrevStructAlloc(ep);
        gcEjectGObj(effect_gobj);
        return;
    }
    if (tic < PAL_PILLAR_RISE)
    {
        width = 0.7F + (0.1F * (f32)tic);
        height = sNdsP4PalutenaPillarRise[tic];
    }
    else if (tic < PAL_PILLAR_THIN_AT)
    {
        width = 1.0F;
        height = 1.0F;
    }
    else
    {
        width = (f32)(PAL_PILLAR_END - tic) / (f32)(PAL_PILLAR_END - PAL_PILLAR_THIN_AT);
        height = 1.0F;
    }
    glow->scale.vec.f.x = ((tic >= PAL_PILLAR_RISE) && (tic < PAL_PILLAR_THIN_AT) && ((tic & 1) != 0)) ?
        0.94F : width;
    glow->scale.vec.f.y = height;
    core->scale.vec.f.x = (tic < PAL_PILLAR_THIN_AT) ? width : (0.15F + (0.85F * width));
    core->scale.vec.f.y = height;
    /* The flare at its foot: brightest as it rises, gone by
     * PAL_PILLAR_FLARE_END. */
    if (tic < PAL_PILLAR_RISE)
    {
        flare_size = 0.6F + (0.4F * (f32)tic / (f32)PAL_PILLAR_RISE);
    }
    else if (tic < PAL_PILLAR_FLARE_END)
    {
        flare_size = (f32)(PAL_PILLAR_FLARE_END - tic) / (f32)(PAL_PILLAR_FLARE_END - PAL_PILLAR_RISE);
    }
    else
    {
        flare_size = 0.0F;
    }
    flare->scale.vec.f.x = flare_size;
    flare->scale.vec.f.y = flare_size;
    /* The dust: rolling out either way along the floor and rising, then
     * still spreading as it settles. */
    if (tic < PAL_PILLAR_DUST_AT)
    {
        reach = 0.0F;
        rise = 0.0F;
    }
    else if (tic <= PAL_PILLAR_DUST_FULL)
    {
        f32 p = (f32)(tic - PAL_PILLAR_DUST_AT) / (f32)(PAL_PILLAR_DUST_FULL - PAL_PILLAR_DUST_AT);

        reach = 0.25F + (0.85F * p);
        rise = 0.45F + (0.55F * p);
    }
    else if (tic < PAL_PILLAR_DUST_END)
    {
        reach = 1.1F + (0.035F * (f32)(tic - PAL_PILLAR_DUST_FULL));
        rise = (f32)(PAL_PILLAR_DUST_END - tic) / (f32)(PAL_PILLAR_DUST_END - PAL_PILLAR_DUST_FULL);
    }
    else
    {
        reach = 0.0F;
        rise = 0.0F;
    }
    dust_l->scale.vec.f.x = reach;
    dust_l->scale.vec.f.y = rise;
    dust_r->scale.vec.f.x = reach;
    dust_r->scale.vec.f.y = rise;
    ep->effect_vars.common.size = tic + 1;
}

/* Her forward and down smash's wings (owner p6i follow-up, 2026-10-14: "they
 * should articulate and stretch and be mostly symmetrical ... polished and
 * like they belong in the DS version"; then "your wings look disconnected in
 * parts"): an effect of the third tree in her special file 2
 * (scripts/p6/p6_wings.py: the kit's godwing, a wing a side of four bones;
 * each wing one continuous mesh whose quads between two bones reach back to
 * the earlier bone's vertices, so it bends without gaps; the right wing the
 * left's mirror), drawn natively (scripts/p4/p4_articles.py "wings"). It is
 * made on the smash's first tic and attached to her (fp->is_effect_attach),
 * so her next status removes it (ftMainSetStatus) and hitlag holds it with
 * her.
 *
 * Every tic, after her map proc, its nodes are posed from the table the
 * file's word at PAL_WINGS_TABLE_SLOT names, at her animation frame
 * (fighter anim_frame): the root takes her TopN's transform, the torso node
 * her clip's torso row relative to TopN, the pair node its turn and size,
 * each left bone (the pair's children, the chain composed offline) its
 * translation, turn and x and y scale; the right wing the left's values
 * mirrored (x translation, y and z turns negated). Outside the table's
 * frames the root is hidden. Angles are s16 binary angles, scales s16 over
 * 4096. */
#define PAL_WINGS_TREE 0x210                /* p6_gate.py WINGS_TREE */
#define PAL_WINGS_TABLE_SLOT 0x420          /* p6_gate.py WINGS_TABLE_SLOT */
#define PAL_WINGS_MAGIC 0x50574E48u         /* "PWNH" */
#define PAL_WINGS_BONES 4
#define PAL_WINGS_NODES (3 + (2 * PAL_WINGS_BONES))
#define PAL_WINGS_VALUES (6 + 4 + (2 * 8 * PAL_WINGS_BONES))   /* both wings (p6_wings.py ROW_VALUES) */
#define PAL_WINGS_CLIP_FSMASH 0             /* p6_wings.py CLIPS */
#define PAL_WINGS_CLIP_DSMASH 1
#define PAL_WINGS_ANGLE (F_CST_DTOR32(360.0F) / 65536.0F)
#define PAL_WINGS_SCALE (1.0F / 4096.0F)

extern GObj *gGCCommonLinks[];

typedef struct NDSP4PalWingsClip
{
    u32 first;                  /* the clip frames the wings show */
    u32 last;
    u32 rows;                   /* from the table's start */
} NDSP4PalWingsClip;

typedef struct NDSP4PalWingsTable
{
    u32 magic;
    u32 values;                 /* s16 values a row */
    u32 clips;
    NDSP4PalWingsClip clip[2];
} NDSP4PalWingsTable;

static s32 ndsP4PalutenaWingsClip(s32 status_id)
{
    if ((status_id >= nFTCommonStatusAttackS4Hi) && (status_id <= nFTCommonStatusAttackS4Lw))
    {
        return PAL_WINGS_CLIP_FSMASH;
    }
    return (status_id == nFTCommonStatusAttackLw4) ? PAL_WINGS_CLIP_DSMASH : -1;
}

/* Her status changing removes it; this is the guard should one keep it. */
static void ndsP4PalutenaWingsProcUpdate(GObj *effect_gobj)
{
    EFStruct *ep = efGetStruct(effect_gobj);

    if ((ep->fighter_gobj == NULL) ||
        (ndsP4PalutenaWingsClip(ftGetStruct(ep->fighter_gobj)->status_id) < 0))
    {
        efManagerSetPrevStructAlloc(ep);
        gcEjectGObj(effect_gobj);
    }
}

static EFDesc sNdsP4PalutenaWingsDesc = {
    0x4 | EFFECT_FLAG_USERDATA, 10, &gNdsP4PalutenaSpecial2,
    { nGCMatrixKindTraRotRpyRSca, nGCMatrixKindNull, 0x00 },
    { nGCMatrixKindTraRotRpyRSca, nGCMatrixKindNull, 0x00 },
    ndsP4PalutenaWingsProcUpdate, gcDrawDObjTreeForGObj,
    PAL_WINGS_TREE, 0x0, 0x0, 0x0
};

static GObj *ndsP4PalutenaWingsFind(GObj *fighter_gobj)
{
    GObj *effect_gobj;

    for (effect_gobj = gGCCommonLinks[nGCCommonLinkIDEffect]; effect_gobj != NULL;
         effect_gobj = effect_gobj->link_next)
    {
        EFStruct *ep = efGetStruct(effect_gobj);

        if ((ep != NULL) && (ep->fighter_gobj == fighter_gobj) &&
            (ep->proc_update == ndsP4PalutenaWingsProcUpdate))
        {
            return effect_gobj;
        }
    }
    return NULL;
}

static void ndsP4PalutenaWingsSetTurn(DObj *dobj, const s16 *turn, f32 mirror)
{
    dobj->rotate.vec.f.x = (f32)turn[0] * PAL_WINGS_ANGLE;
    dobj->rotate.vec.f.y = (f32)turn[1] * PAL_WINGS_ANGLE * mirror;
    dobj->rotate.vec.f.z = (f32)turn[2] * PAL_WINGS_ANGLE * mirror;
}

static void ndsP4PalutenaWingsPose(GObj *effect_gobj, GObj *fighter_gobj, s32 clip)
{
    const NDSP4PalWingsTable *table;
    const NDSP4PalWingsClip *rows;
    DObj *root = DObjGetStruct(effect_gobj);
    DObj *topn = DObjGetStruct(fighter_gobj);
    DObj *nodes[PAL_WINGS_NODES];
    const s16 *row;
    f32 size;
    s32 frame;
    s32 side;
    s32 k;

    table = *(const NDSP4PalWingsTable **)((const u8 *)gNdsP4PalutenaSpecial2 + PAL_WINGS_TABLE_SLOT);
    frame = (s32)(fighter_gobj->anim_frame + 0.5F);
    rows = &table->clip[clip];

    if ((table->magic != PAL_WINGS_MAGIC) || (table->values != PAL_WINGS_VALUES) ||
        (frame < (s32)rows->first) || (frame > (s32)rows->last))
    {
        root->flags |= DOBJ_FLAG_HIDDEN;
        return;
    }
    /* The tree (p6_wings.py write): root, torso, pair, then the pair's
     * children, the left wing's four bones and the right wing's. */
    nodes[0] = root;
    nodes[1] = root->child;
    nodes[2] = nodes[1]->child;
    nodes[3] = nodes[2]->child;
    for (k = 4; k < PAL_WINGS_NODES; k++)
    {
        nodes[k] = nodes[k - 1]->sib_next;
    }
    root->flags &= ~DOBJ_FLAG_HIDDEN;
    row = (const s16 *)((const u8 *)table + rows->rows) + ((frame - (s32)rows->first) * PAL_WINGS_VALUES);

    root->translate.vec.f = topn->translate.vec.f;
    root->rotate.vec.f = topn->rotate.vec.f;
    root->scale.vec.f = topn->scale.vec.f;

    nodes[1]->translate.vec.f.x = row[0];
    nodes[1]->translate.vec.f.y = row[1];
    nodes[1]->translate.vec.f.z = row[2];
    ndsP4PalutenaWingsSetTurn(nodes[1], &row[3], 1.0F);

    ndsP4PalutenaWingsSetTurn(nodes[2], &row[6], 1.0F);
    size = (f32)row[9] * PAL_WINGS_SCALE;
    nodes[2]->scale.vec.f.x = nodes[2]->scale.vec.f.y = nodes[2]->scale.vec.f.z = size;

    for (k = 0; k < PAL_WINGS_BONES; k++)
    {
        for (side = 0; side < 2; side++)
        {
            /* Each wing its own values (Ultimate's godwing animates them
             * apart), the right wing's stored mirrored as the left's. */
            const s16 *bone = &row[10 + (8 * ((side * PAL_WINGS_BONES) + k))];
            DObj *b = nodes[3 + (side * PAL_WINGS_BONES) + k];
            f32 mirror = (side == 0) ? 1.0F : -1.0F;

            b->translate.vec.f.x = (f32)bone[0] * mirror;
            b->translate.vec.f.y = bone[1];
            b->translate.vec.f.z = bone[2];
            ndsP4PalutenaWingsSetTurn(b, &bone[3], mirror);
            b->scale.vec.f.x = (f32)bone[6] * PAL_WINGS_SCALE;
            b->scale.vec.f.y = (f32)bone[7] * PAL_WINGS_SCALE;
            b->scale.vec.f.z = 1.0F;
        }
    }
}

/* In a wing smash: the wings made on its first tic, posed every tic. */
static void ndsP4PalutenaWings(GObj *fighter_gobj, FTStruct *fp, s32 clip)
{
    GObj *effect_gobj = ndsP4PalutenaWingsFind(fighter_gobj);

    if (effect_gobj == NULL)
    {
        Vec3f pos = DObjGetStruct(fighter_gobj)->translate.vec.f;

        effect_gobj = ndsP4EntryMakeEffect(&sNdsP4PalutenaWingsDesc, &pos);
        if (effect_gobj == NULL)
        {
            return;
        }
        efGetStruct(effect_gobj)->fighter_gobj = fighter_gobj;
        fp->is_effect_attach = TRUE;
    }
    ndsP4PalutenaWingsPose(effect_gobj, fighter_gobj, clip);
}

/* ftMainProcPhysicsMap's per-frame hook: in the forward and down smash, the
 * wings; in the up smash, the pillar's columns sweep, and its flag0 makes
 * the pillar once. */
static void ndsP4PalutenaAfterProcMap(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    GObj *effect_gobj;
    Vec3f pos;
    s32 clip = ndsP4PalutenaWingsClip(fp->status_id);
    s32 i;

    if (clip >= 0)
    {
        ndsP4PalutenaWings(fighter_gobj, fp, clip);
        return;
    }
    if (fp->status_id != nFTCommonStatusAttackHi4)
    {
        return;
    }
    for (i = 0; i < ARRAY_COUNT(sNdsP4PalutenaPillarColumn); i++)
    {
        FTAttackColl *attack_coll = &fp->attack_colls[i];

        if (attack_coll->attack_state != nGMAttackStateOff)
        {
            attack_coll->offset.y = sNdsP4PalutenaPillarColumn[i]
                [(attack_coll->offset.y == sNdsP4PalutenaPillarColumn[i][0]) ? 1 : 0];
        }
    }
    if (fp->motion_vars.flags.flag0 == 0)
    {
        return;
    }
    fp->motion_vars.flags.flag0 = 0;
    pos = DObjGetStruct(fighter_gobj)->translate.vec.f;
    pos.x += PAL_PILLAR_AHEAD * (f32)fp->lr;
    pos.z += PAL_PILLAR_TOWARD_CAMERA;
    effect_gobj = ndsP4EntryMakeEffect(&sNdsP4PalutenaPillarDesc, &pos);
    if (effect_gobj != NULL)
    {
        efGetStruct(effect_gobj)->effect_vars.common.size = 0;
    }
}

static void ndsP4PalutenaEntryCase(FTStruct *fp)
{
    GObj *effect_gobj = ndsP4EntryMakeEffect(&sNdsP4PalutenaGateDesc, &fp->entry_pos);

    if (effect_gobj != NULL)
    {
        efGetStruct(effect_gobj)->effect_vars.common.size = 0;
    }
}

const NDSP4Overrides gNdsP4PalutenaOverrides = {
    .update_damage = ndsP4PalutenaUpdateDamage,
    .entry_case = ndsP4PalutenaEntryCase,
    .after_proc_map = ndsP4PalutenaAfterProcMap,
    /* Ultimate's motions; its hits collide in the stage's plane. */
    .planar_hits = 1u,
};

#if NDS_P2_KIRBY
/* ---- Kirby's copy (A11; card "Kirby") ----
 *
 * Explosive Flame on Kirby's own PK Fire motions and scripts
 * (scripts/p4/p4_kirby.py CUSTOM_POWERS; 228_KirbyMainMotion.c
 * PKFireGround/Air: the cast's PSI sound, his call and psionic flash). At
 * frame 12 (PK Fire's own flag is at 20; 0.85x of his first rounds' blast at
 * 36, as hers) her fire starts at the distance the stick picked at the cast,
 * at his height: the twinkle, then the weapon her cast makes, with her blast
 * sound (p4_kirby.py "cues"). Kirby's script has no hitboxes.
 *
 * status_vars words: 0 the distance (as f32 bits), 1 whether the fire is
 * made. */
#define PAL_KIRBY_FIRE_AT 12.0F
/* Kirby's PK Fire switches between ground and air keeping his face, hurt
 * state, effects and colour (FTKIRBY_COPYNESS_SPECIALN_STATUS_FLAGS). */
#define PAL_KIRBY_PRESERVE_SWITCH (FTSTATUS_PRESERVE_TEXTUREPART | FTSTATUS_PRESERVE_HITSTATUS | \
                                   FTSTATUS_PRESERVE_EFFECT | FTSTATUS_PRESERVE_COLANIM)

static void ndsP4PalutenaKirbyNSPStart(GObj *fighter_gobj, s32 status_id)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    f32 distance = ndsP4PalutenaFlameDistance(fp);

    ndsP4PalutenaSetStatus(fighter_gobj, status_id);
    *ndsP4PalutenaVarF(fp, 0) = distance;
    *ndsP4PalutenaVar(fp, 1) = 0;
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
    s32 *made = ndsP4PalutenaVar(fp, 1);

    if ((*made == 0) && (fighter_gobj->anim_frame >= PAL_KIRBY_FIRE_AT))
    {
        *made = 1;
        ndsP4PalutenaFlameMakeWeapon(fighter_gobj, *ndsP4PalutenaVarF(fp, 0), NDS_P4_KIRBY_PALUTENA_FGM_BLAST);
    }
    fp->motion_vars.flags.flag0 = 0;
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
