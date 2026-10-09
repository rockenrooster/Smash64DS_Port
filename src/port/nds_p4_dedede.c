/*
 * P4 Dedede: native ports of the donor's special routines and of the patches
 * Remix makes on his character id. Source: JSsixtyfour/smashremix 5e04fe7,
 * src/Dedede/DededeSpecial.asm and Dedede.asm, jigglypuffkirbyshared.asm
 * (jump_fix_1-5) and Reflect.asm (the custom absorb), read as assembled
 * (scripts/p4/mipsdis.py). Dedede runs Captain's status code
 * (fp->fkind == nFTKindCaptain); his inhale runs Kirby's (ftkirbyspecialn.c,
 * battleship_kirby.c) on his own statuses.
 *
 * Player-struct offsets in the donor map to BattleShip fields as:
 *   0x01C status tics                      status_total_tics
 *   0x02C percent                          percent_damage
 *   0x044 direction                        lr
 *   0x048-0x050 air velocity               physics.vel_air
 *   0x060/0x064 ground velocity            physics.vel_ground.x/.y
 *   0x0CE/0x0D2 collision flags            coll_data.mask_curr/mask_stat
 *   0x148 jumps used                       jumps_used
 *   0x14C kinetic state                    ga
 *   0x17C/0x180/0x184 temp variables 1-3   motion_vars.flags.flag0/1/2
 *   0x18C bit 26                           is_reflect
 *   0x192 bit 7                            is_special_interrupt
 *   0x1B4/0x1B6/0x1B8 button masks         input.button_mask_a/_b/_z
 *   0x1BC/0x1BE buttons held, tapped       input.pl.button_hold/button_tap
 *   0x1C2/0x1C3 stick                      input.pl.stick_range.x/.y
 *   0x840 held fighter                     catch_gobj
 *   0x850 reflect box                      special_coll
 *   0x8E8 + 4n parts                       joints[n]
 *   0x9E0/0x9EC                            proc_physics, proc_damage
 *   0xADC-0xAE4                            passive_vars words 0-2: his
 *                                          minions' pointers (words 0-1,
 *                                          S6) and the down special's
 *                                          charge (word 2)
 *   0xB18-0xB28                            status_vars words 0-4: the down
 *                                          special's charge-or-toss choice
 *                                          (word 0), timer (word 1), held
 *                                          minion (word 2) and toss frames
 *                                          (word 4); the up special's stick
 *                                          (word 2, a float)
 *   attributes + 0x58/0x64                 gravity, jumps_max
 *
 * The up special's landing stars are Yoshi's star weapon on his special
 * file 2 (YoshiShared.asm down_special_struct_fix, S6, below). S6 owns his
 * minions (Waddle Dee, Waddle Doo and Gordo: attach_minion_'s items, the
 * toss's throw flag) and his entry; the charged hand's colour
 * (DEDEDE_CHARGE, gfx_routine_end) is a Remix colour routine the port's
 * table declines (S4); Kirby wearing his hat is S8's; the CPU's recovery
 * and reflect behaviour are S10's.
 */
#include <nds/nds_p4.h>

#if NDS_P4_DEDEDE

#if !NDS_P2_KIRBY
#error "P4 Dedede's inhale runs Kirby's status code (NDS_P2_KIRBY)"
#endif
#if !NDS_P2_YOSHI
#error "P4 Dedede's landing stars run Yoshi's star weapon (NDS_P2_YOSHI)"
#endif

#include <ef/effect.h>
#include <it/item.h>
#include <macros.h>
#include <reloc_data.h>
#include <sys/audio.h>
#include <sys/obj.h>
#include <wp/weapon.h>

#ifndef DObjGetStruct
#define DObjGetStruct(gobj) ((DObj *)((gobj)->obj))
#endif

void ftKirbySpecialNInitStatusVars(GObj *fighter_gobj, sb32 unused);
void ftKirbySpecialNSetCatchParams(FTStruct *fp);
void ftKirbySpecialNGotoSetCatchParams(GObj *fighter_gobj);
void ftKirbySpecialNApplyCaptureDamage(GObj *kirby_gobj, GObj *victim_gobj, s32 damage);
sb32 ftKirbySpecialNTurnCheckGotoTurn(GObj *fighter_gobj, void (*proc_status)(GObj *));
void ftKirbySpecialNTurnSetStatus(GObj *fighter_gobj);
void ftKirbySpecialNEatSetStatusParam(GObj *fighter_gobj, s32 status_id);
void ftCommonThrownKirbyStarSetStatus(GObj *fighter_gobj);
void ftCommonThrownCommonStarUpdatePhysics(GObj *fighter_gobj, f32 decelerate);
s32 ftCommonWalkGetWalkStatus(s8 stick_range_x);
sb32 ftCommonWalkCheckInputSuccess(GObj *fighter_gobj);
sb32 ftCommonWaitCheckInputSuccess(GObj *fighter_gobj);
void ftParamKirbyTryMakeMapStarEffect(GObj *fighter_gobj);
#if NDS_P2_ITEM_CORE
GObj *itStarRodWeaponStarMakeWeapon(GObj *fighter_gobj, Vec3f *pos, ub8 is_smash);
extern void *gITManagerCommonData;
#endif

/* Dedede.Action (his action array; 0xEF on are add_new_action's). */
#define DEDEDE_STATUS_USP_BEGIN 0xE4
#define DEDEDE_STATUS_USP_MOVE 0xE5
#define DEDEDE_STATUS_USP_LAND 0xE6
#define DEDEDE_STATUS_NSP_BEGIN_GROUND 0xE7
#define DEDEDE_STATUS_NSP_LOOP_GROUND 0xE8
#define DEDEDE_STATUS_USP_CANCEL 0xE9
#define DEDEDE_STATUS_NSP_PULL_GROUND 0xEA
#define DEDEDE_STATUS_NSP_SWALLOW_GROUND 0xEB
#define DEDEDE_STATUS_NSP_IDLE_GROUND 0xEC
#define DEDEDE_STATUS_NSP_SPIT_GROUND 0xED
#define DEDEDE_STATUS_NSP_TURN_GROUND 0xEE
#define DEDEDE_STATUS_NSP_END_GROUND 0xEF
#define DEDEDE_STATUS_NSP_BEGIN_AIR 0xF0
#define DEDEDE_STATUS_NSP_LOOP_AIR 0xF1
#define DEDEDE_STATUS_NSP_PULL_AIR 0xF2
#define DEDEDE_STATUS_NSP_SWALLOW_AIR 0xF3
#define DEDEDE_STATUS_NSP_FALL 0xF4
#define DEDEDE_STATUS_NSP_SPIT_AIR 0xF5
#define DEDEDE_STATUS_NSP_TURN_AIR 0xF6
#define DEDEDE_STATUS_NSP_END_AIR 0xF7
#define DEDEDE_STATUS_DSPG_BEGIN 0xF8
#define DEDEDE_STATUS_DSPG_CHARGE 0xF9
#define DEDEDE_STATUS_DSPG_SHOOT 0xFA
#define DEDEDE_STATUS_DSPA_BEGIN 0xFB
#define DEDEDE_STATUS_DSPA_CHARGE 0xFC
#define DEDEDE_STATUS_DSPA_SHOOT 0xFD
#define DEDEDE_STATUS_NSP_WALK_1 0xFF
#define DEDEDE_STATUS_USP_CEILING_BONK 0x102

/* DededeUSP (Super Dedede Jump). */
#define DEDEDE_USP_BEGIN_SPEED 0.5F
#define DEDEDE_USP_MOVE_SPEED_X 0.6875F
#define DEDEDE_USP_MOVE_SPEED_Y 144.0F
#define DEDEDE_USP_GRAVITY 2.0F
#define DEDEDE_USP_GRAVITY_PEAK 0.19921875F     /* 0x3E4C, "0.2" */
#define DEDEDE_USP_GRAVITY_FALLING 2.5F
#define DEDEDE_USP_MAX_FALLING 125.0F
#define DEDEDE_USP_TAP_B 0x40u                  /* B_PRESSED, the taps' high byte */
#define DEDEDE_USP_CANCEL_STICK_Y -39
#define DEDEDE_USP_CEILING_FGM 0x134
#define DEDEDE_PRESERVE_USP_MOVE 0x0001u        /* hit */
/* begin_physics_'s temp variable 3 (the script's). */
#define DEDEDE_USP_LATERAL 0
#define DEDEDE_USP_RISE 2

/* DededeNSP (the inhale). */
#define DEDEDE_NSP_SPIT_DAMAGE 16
#define DEDEDE_NSP_SPIT_VELOCITY 240.0F
#define DEDEDE_NSP_SPIT_DECELERATION 10.0F
#define DEDEDE_NSP_STAR_JOINT 16
#define DEDEDE_NSP_WIND_X 1000.0F
#define DEDEDE_PRESERVE_NSP_LOOP 0x0824u        /* loop sfx, model part, effect */
#define DEDEDE_PRESERVE_NSP_LOOP_SWITCH 0x4825u /* the same, rumble and hit */
#define DEDEDE_PRESERVE_NSP_SPIT 0x00A4u        /* texture part, model part, effect */
/* Dedede.INITIAL_ABSORB_TIME (Kirby's 500 / 2), plus the victim's percent. */
#define DEDEDE_NSP_CAPTURE_WAIT 250

/* DededeDSP (the minion toss). */
#define DEDEDE_DSP_FIRST_PULL 6
#define DEDEDE_DSP_FIRST_STOW 32
#define DEDEDE_DSP_FIRST_CHARGE 54
#define DEDEDE_DSP_SECOND_PULL 60
#define DEDEDE_DSP_SECOND_STOW 87
#define DEDEDE_DSP_SECOND_CHARGE 109
#define DEDEDE_DSP_FINAL_PULL 115
#define DEDEDE_DSP_CHARGE_MAX 2
#define DEDEDE_DSP_CHARGE_END_FRAME 20.0F       /* CHARGE_END 0x41A0 */
#define DEDEDE_DSP_RUMMAGE_TIME 12u             /* MINION_RUMMAGE_TIME */
#define DEDEDE_DSP_COLANIM_CHARGE 0x74          /* GFXRoutine.id.DEDEDE_CHARGE */
#define DEDEDE_DSP_FAKE_ITEM_KIND 0x1C          /* no smoke on the destroy */
#define DEDEDE_PRESERVE_DSP 0x0002u             /* colanim */
#define DEDEDE_PRESERVE_DSP_CHARGE 0x0802u      /* loop sfx, colanim */

static s32 *ndsP4DededeStatusS32(FTStruct *fp, s32 word)
{
    return &((s32 *)(void *)&fp->status_vars)[word];
}

static s32 *ndsP4DededePassiveS32(FTStruct *fp, s32 word)
{
    return &((s32 *)(void *)&fp->passive_vars)[word];
}

/* Status word 2, the down special's held minion (an item GObj, S6). */
static GObj **ndsP4DededeMinion(FTStruct *fp)
{
    return (GObj **)(void *)ndsP4DededeStatusS32(fp, 2);
}

static void ndsP4DededeClearFlags(FTStruct *fp)
{
    fp->motion_vars.flags.flag0 = 0;
    fp->motion_vars.flags.flag1 = 0;
    fp->motion_vars.flags.flag2 = 0;
}

/* ---- Up special (Super Dedede Jump) ---- */

static void ndsP4DededeUSPMoveInitial(GObj *fighter_gobj);
static void ndsP4DededeUSPCancelInitial(GObj *fighter_gobj);

/* DededeUSP.begin_initial_ (ground_usp and air_usp). */
void ndsP4DededeUSPInitial(GObj *fighter_gobj)
{
    FTStruct *fp;

    ftMainSetStatus(fighter_gobj, DEDEDE_STATUS_USP_BEGIN, 0.0F, 1.0F, FTSTATUS_PRESERVE_NONE);
    ftMainPlayAnimEventsAll(fighter_gobj);
    fp = ftGetStruct(fighter_gobj);
    ndsP4DededeClearFlags(fp);
    fp->physics.vel_ground.x = 0.0F;
    fp->physics.vel_ground.y = 0.0F;
}

/* DededeUSP.begin_main_ (0xE4 update). */
void ndsP4DededeUSPBeginMain(GObj *fighter_gobj)
{
    ftAnimEndCheckSetStatus(fighter_gobj, ndsP4DededeUSPMoveInitial);
}

/* DededeUSP.apply_vertical_movement_: the script's temp variable 2 picks
 * the gravity (rising, the peak, falling). */
static void ndsP4DededeUSPApplyVertical(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    f32 mul;

    if (fp->motion_vars.flags.flag1 == 0)
    {
        mul = DEDEDE_USP_GRAVITY;
    }
    else if (fp->motion_vars.flags.flag1 == 1)
    {
        mul = DEDEDE_USP_GRAVITY_PEAK;
    }
    else mul = DEDEDE_USP_GRAVITY_FALLING;

    ftPhysicsApplyGravityClampTVel(fp, fp->attr->gravity * mul, DEDEDE_USP_MAX_FALLING);
    (void)ftPhysicsCheckClampAirVelXDecMax(fp, fp->attr);
}

/* DededeUSP.begin_physics_ (0xE4 physics): the script's temp variable 3
 * holds him (lateral drift only), launches him (no jumps left, the stick's
 * drift and 144 up) or lets him rise. */
void ndsP4DededeUSPBeginPhysics(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    f32 stick_x;

    if (fp->motion_vars.flags.flag2 == DEDEDE_USP_LATERAL)
    {
        /* apply_lateral_movement_ */
        stick_x = (f32)fp->input.pl.stick_range.x;
        *(f32 *)(void *)ndsP4DededeStatusS32(fp, 2) = stick_x;
        fp->physics.vel_air.x = DEDEDE_USP_BEGIN_SPEED * stick_x;
        fp->physics.vel_air.y = 0.0F;
        return;
    }
    if (fp->motion_vars.flags.flag2 != DEDEDE_USP_RISE)
    {
        fp->jumps_used = fp->attr->jumps_max;

        stick_x = (f32)fp->input.pl.stick_range.x;
        *(f32 *)(void *)ndsP4DededeStatusS32(fp, 2) = stick_x;
        fp->physics.vel_air.x = stick_x * DEDEDE_USP_MOVE_SPEED_X;
        fp->physics.vel_air.y = DEDEDE_USP_MOVE_SPEED_Y;
    }
    ndsP4DededeUSPApplyVertical(fighter_gobj);
}

/* DededeUSP.begin_set_aerial: off the floor, airborne without the source's
 * air change. */
static void ndsP4DededeUSPBeginSetAir(GObj *fighter_gobj)
{
    ftGetStruct(fighter_gobj)->ga = nMPKineticsAir;
}

/* DededeUSP.begin_collision_ (0xE4 map). In the air a floor only grounds
 * him (the status runs on); a ledge catches him. */
void ndsP4DededeUSPBeginMap(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    u16 stat;

    if (fp->ga == nMPKineticsGround)
    {
        mpCommonProcFighterOnFloor(fighter_gobj, ndsP4DededeUSPBeginSetAir);
        return;
    }
    if (mpCommonCheckFighterCeilHeavyCliff(fighter_gobj) == FALSE)
    {
        return;
    }
    stat = fp->coll_data.mask_stat;

    if (stat & MAP_FLAG_FLOOR)
    {
        mpCommonSetFighterGround(fp);
    }
    else if (stat & MAP_FLAG_CLIFF_MASK)
    {
        ftCommonCliffCatchSetStatus(fighter_gobj);
    }
}

/* DededeUSP.move_initial_ (the begin's end of motion). */
static void ndsP4DededeUSPMoveInitial(GObj *fighter_gobj)
{
    ftMainSetStatus(fighter_gobj, DEDEDE_STATUS_USP_MOVE, 0.0F, 1.0F, DEDEDE_PRESERVE_USP_MOVE);
    ftMainPlayAnimEventsAll(fighter_gobj);
}

/* DededeUSP.move_cancel_ (0xE5 interrupt): from the apex down, a B tap or
 * the stick down past 39 drops him early. */
void ndsP4DededeUSPMoveInterrupt(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (!(fp->physics.vel_air.y <= 0.0F))
    {
        return;
    }
    if ((((fp->input.pl.button_tap >> 8) & DEDEDE_USP_TAP_B) != 0) ||
        (fp->input.pl.stick_range.y < DEDEDE_USP_CANCEL_STICK_Y))
    {
        ftMainSetStatus(fighter_gobj, DEDEDE_STATUS_USP_CANCEL, 0.0F, 1.0F, FTSTATUS_PRESERVE_NONE);
    }
}

/* DededeUSP.move_physics_ (0xE5 physics). */
void ndsP4DededeUSPMovePhysics(GObj *fighter_gobj)
{
    ndsP4DededeUSPApplyVertical(fighter_gobj);
}

/* DededeUSP.move_collision_ (0xE5 map): a floor lands him, a ledge catches
 * him, a ceiling bonks him. Read as assembled the ceiling test after a ledge
 * catch reads a temporary the catch's flash effect leaves; a Remix Dedede
 * keeps the ledge, so it reads zero there. */
void ndsP4DededeUSPMoveMap(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    u16 stat;

    if (mpCommonCheckFighterCeilHeavyCliff(fighter_gobj) == FALSE)
    {
        return;
    }
    stat = fp->coll_data.mask_stat;

    if (stat & MAP_FLAG_FLOOR)
    {
        mpCommonSetFighterGround(fp);
        ftMainSetStatus(fighter_gobj, DEDEDE_STATUS_USP_LAND, 0.0F, 1.0F, FTSTATUS_PRESERVE_NONE);
        ftGetStruct(fighter_gobj)->physics.vel_ground.x = 0.0F;
        return;
    }
    if (stat & MAP_FLAG_CLIFF_MASK)
    {
        ftCommonCliffCatchSetStatus(fighter_gobj);
        return;
    }
    if (stat & MAP_FLAG_CEIL)
    {
        ftMainSetStatus(fighter_gobj, DEDEDE_STATUS_USP_CEILING_BONK, 0.0F, 1.0F, FTSTATUS_PRESERVE_NONE);
        func_800269C0_275C0(DEDEDE_USP_CEILING_FGM);
    }
}

/* ---- The landing's stars (YoshiShared.asm downspecial_struct_dedede) ----
 *
 * Yoshi's star weapon with its description's file pointer swapped for
 * Dedede's special file 2 (Dedede file 7, generated as
 * gNdsP4DededeSpecial2), which carries the star's WPAttributes at Yoshi's
 * offset; its graphic is the item file's shared star quad, which the DS
 * renderer admits under any owner. */
#define DEDEDE_STAR_LIFETIME 16                 /* WPYOSHISTAR_LIFETIME */
#define DEDEDE_STAR_ANGLE 0.5235988F            /* WPYOSHISTAR_ANGLE, 30 degrees */
#define DEDEDE_STAR_VEL 30.0F                   /* WPYOSHISTAR_VEL */
#define DEDEDE_STAR_OFF_X 300.0F                /* WPYOSHISTAR_OFF_X */
#define DEDEDE_STAR_OFF_Y 20.0F                 /* WPYOSHISTAR_OFF_Y */
#define DEDEDE_STAR_ATTRIBUTES 0x40             /* llYoshiMainStarWeaponAttributes */

extern void *gNdsP4DededeSpecial2;

/* Makers that found his star file missing: counted, never a fault. */
__attribute__((used)) volatile u32 gNdsP4DededeArticleMisses;

f32 __sinf(f32);
f32 __cosf(f32);
sb32 wpYoshiStarProcUpdate(GObj *weapon_gobj);
sb32 wpYoshiStarProcMap(GObj *weapon_gobj);
sb32 wpYoshiStarProcHit(GObj *weapon_gobj);
sb32 wpYoshiStarProcShield(GObj *weapon_gobj);
sb32 wpYoshiStarProcHop(GObj *weapon_gobj);
sb32 wpYoshiStarProcReflector(GObj *weapon_gobj);

static WPDesc sNdsP4DededeStarWeaponDesc = {
    0x00,
    nWPKindYoshiStar,
    &gNdsP4DededeSpecial2,
    DEDEDE_STAR_ATTRIBUTES,
    { nGCMatrixKindTraRotRpyRSca, nGCMatrixKindNull, 0 },
    wpYoshiStarProcUpdate,
    wpYoshiStarProcMap,
    wpYoshiStarProcHit,
    wpYoshiStarProcShield,
    wpYoshiStarProcHop,
    wpYoshiStarProcHit,
    wpYoshiStarProcReflector,
    wpYoshiStarProcShield
};

/* wpYoshiStarMakeWeapon on that description: 20 up and 300 out on `lr`'s
 * side, flying at 30 up 30 degrees. */
static void ndsP4DededeStarMakeWeapon(GObj *fighter_gobj, Vec3f *pos, s32 lr)
{
    GObj *weapon_gobj;
    WPStruct *wp;
    Vec3f offset = *pos;

    offset.y += DEDEDE_STAR_OFF_Y;

    if (lr == +1)
    {
        offset.x += DEDEDE_STAR_OFF_X;
    }
    else offset.x -= DEDEDE_STAR_OFF_X;

    weapon_gobj = wpManagerMakeWeapon(fighter_gobj, &sNdsP4DededeStarWeaponDesc, &offset,
                                      WEAPON_FLAG_COLLPROJECT | WEAPON_FLAG_PARENT_FIGHTER);
    if (weapon_gobj == NULL)
    {
        return;
    }
    wp = wpGetStruct(weapon_gobj);
    wp->lr = lr;
    wp->lifetime = DEDEDE_STAR_LIFETIME;
    wp->physics.vel_air.x = __cosf(DEDEDE_STAR_ANGLE) * (DEDEDE_STAR_VEL * wp->lr);
    wp->physics.vel_air.y = __sinf(DEDEDE_STAR_ANGLE) * DEDEDE_STAR_VEL;
}

/* DededeUSP.landing_main_'s wpYoshiStarMakeStars: one star each way from
 * his top joint. */
static void ndsP4DededeUSPLandingStars(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    Vec3f pos;

    if (gNdsP4DededeSpecial2 == NULL)
    {
        gNdsP4DededeArticleMisses++;
        return;
    }
    pos.x = pos.y = pos.z = 0.0F;
    gmCollisionGetFighterPartsWorldPosition(fp->joints[nFTPartsJointTopN], &pos);

    ndsP4DededeStarMakeWeapon(fighter_gobj, &pos, fp->lr);
    ndsP4DededeStarMakeWeapon(fighter_gobj, &pos, -fp->lr);
}

/* DededeUSP.landing_main_ (0xE6 update): the script's temp variable 1
 * makes the stars; the end of motion stands him up. */
void ndsP4DededeUSPLandingMain(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (fp->motion_vars.flags.flag0 != 0)
    {
        fp->motion_vars.flags.flag0 = 0;
        ndsP4DededeUSPLandingStars(fighter_gobj);
    }
    ftAnimEndCheckSetStatus(fighter_gobj, ftCommonWaitSetStatus);
}

/* DededeUSP.cancel_initial_. */
static void ndsP4DededeUSPCancelInitial(GObj *fighter_gobj)
{
    ftMainSetStatus(fighter_gobj, DEDEDE_STATUS_USP_CANCEL, 0.0F, 1.0F, FTSTATUS_PRESERVE_NONE);
}

/* DededeUSP.cancel_main_ (0xE9 update): special fall at the end of motion. */
void ndsP4DededeUSPCancelMain(GObj *fighter_gobj)
{
    if (fighter_gobj->anim_frame <= 0.0F)
    {
        ftCommonFallSpecialSetStatus(fighter_gobj, 1.0F, FALSE, TRUE, FALSE, 1.0F, FALSE);
    }
}

/* DededeUSP.cancel_collision_ (0xE9 map): a floor lands him with the
 * landing-lag flag cleared; a ledge catches him. */
void ndsP4DededeUSPCancelMap(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    u16 stat;

    if (mpCommonCheckFighterCeilHeavyCliff(fighter_gobj) == FALSE)
    {
        return;
    }
    stat = fp->coll_data.mask_stat;

    if (stat & MAP_FLAG_FLOOR)
    {
        mpCommonSetFighterGround(fp);
        *ndsP4DededeStatusS32(fp, 0) = 0;
        ftMainSetStatus(fighter_gobj, nFTCommonStatusLandingFallSpecial, 0.0F, 1.0F,
                        FTSTATUS_PRESERVE_NONE);
    }
    else if (stat & MAP_FLAG_CLIFF_MASK)
    {
        ftCommonCliffCatchSetStatus(fighter_gobj);
    }
}

/* DededeUSP.ceiling_bonk_main_ (0x102 update). */
void ndsP4DededeUSPCeilingBonkMain(GObj *fighter_gobj)
{
    ftAnimEndCheckSetStatus(fighter_gobj, ndsP4DededeUSPCancelInitial);
}

/* DededeUSP.ceiling_subroutine_: the bonk again, his rise stopped. */
static void ndsP4DededeUSPCeilingBonk(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    ftMainSetStatus(fighter_gobj, DEDEDE_STATUS_USP_CEILING_BONK, 0.0F, 1.0F, FTSTATUS_PRESERVE_NONE);
    ftMainPlayAnimEventsAll(fighter_gobj);
    fp->physics.vel_air.z = 0.0F;
    fp->physics.vel_air.y = 0.0F;
}

/* DededeUSP.ceiling_bonk_collision_ (0x102 map), after the source's ceiling
 * map: a ledge catches him, a floor lands him, a heavy ceiling bonks him
 * again. */
void ndsP4DededeUSPCeilingBonkMap(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    u16 stat;

    if (mpCommonCheckFighterCeilHeavyCliff(fighter_gobj) == FALSE)
    {
        return;
    }
    stat = fp->coll_data.mask_stat;

    if (stat & MAP_FLAG_CLIFF_MASK)
    {
        ftCommonCliffCatchSetStatus(fighter_gobj);
    }
    else if (stat & MAP_FLAG_FLOOR)
    {
        mpCommonSetFighterWaitOrLanding(fighter_gobj);
    }
    else if (fp->coll_data.mask_curr & MAP_FLAG_CEILHEAVY)
    {
        ndsP4DededeUSPCeilingBonk(fighter_gobj);
    }
}

/* ---- Neutral special (the inhale) ---- */

static void ndsP4DededeNSPGroundLoopInitial(GObj *fighter_gobj);
static void ndsP4DededeNSPAirLoopInitial(GObj *fighter_gobj);

/* DededeNSP.absorb_struct: Remix's custom reflect kind, index 2
 * (custom_reflect_table: absorb_initial_), on the top joint. */
static FTSpecialColl sNdsP4DededeAbsorbColl = {
    NDS_P4_SPECIAL_COLL_CUSTOM_KIND(2), 0,
    { 0.0F, 318.0F, 250.0F }, { 480.0F, 330.0F, 330.0F },
    0x18000000
};

/* DededeNSP.absorb_setup_: the loop absorbs. */
static void ndsP4DededeAbsorbSetup(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    fp->is_reflect = TRUE;
    fp->special_coll = &sNdsP4DededeAbsorbColl;
}

/* DededeNSP.ground_begin_initial_ / air_begin_initial_: Kirby's start with
 * the flags and the ground velocity cleared after its events. */
static void ndsP4DededeNSPBeginInitial(GObj *fighter_gobj, s32 status_id)
{
    FTStruct *fp;

    ftMainSetStatus(fighter_gobj, status_id, 0.0F, 1.0F, FTSTATUS_PRESERVE_NONE);
    ftKirbySpecialNInitStatusVars(fighter_gobj, FALSE);
    ftKirbySpecialNGotoSetCatchParams(fighter_gobj);
    ftMainPlayAnimEventsAll(fighter_gobj);
    fp = ftGetStruct(fighter_gobj);
    ndsP4DededeClearFlags(fp);
    fp->physics.vel_ground.x = 0.0F;
    fp->physics.vel_ground.y = 0.0F;
}

/* DededeNSP.ground_begin_initial_ (ground_nsp). */
void ndsP4DededeNSPGroundInitial(GObj *fighter_gobj)
{
    ndsP4DededeNSPBeginInitial(fighter_gobj, DEDEDE_STATUS_NSP_BEGIN_GROUND);
}

/* DededeNSP.air_begin_initial_ (air_nsp). */
void ndsP4DededeNSPAirInitial(GObj *fighter_gobj)
{
    ndsP4DededeNSPBeginInitial(fighter_gobj, DEDEDE_STATUS_NSP_BEGIN_AIR);
}

/* DededeNSP.ground_begin_main_ (0xE7 update). */
void ndsP4DededeNSPGroundBeginMain(GObj *fighter_gobj)
{
    ftAnimEndCheckSetStatus(fighter_gobj, ndsP4DededeNSPGroundLoopInitial);
}

/* DededeNSP.air_begin_main_ (0xF0 update). */
void ndsP4DededeNSPAirBeginMain(GObj *fighter_gobj)
{
    ftAnimEndCheckSetStatus(fighter_gobj, ndsP4DededeNSPAirLoopInitial);
}

/* DededeNSP.ground_loop_initial_ / air_loop_initial_: Kirby's loop, then
 * the absorb before its events. */
static void ndsP4DededeNSPLoopInitial(GObj *fighter_gobj, s32 status_id)
{
    ftMainSetStatus(fighter_gobj, status_id, 0.0F, 1.0F, DEDEDE_PRESERVE_NSP_LOOP);
    ftKirbySpecialNGotoSetCatchParams(fighter_gobj);
    ndsP4DededeAbsorbSetup(fighter_gobj);
    ftMainPlayAnimEventsAll(fighter_gobj);
}

static void ndsP4DededeNSPGroundLoopInitial(GObj *fighter_gobj)
{
    ndsP4DededeNSPLoopInitial(fighter_gobj, DEDEDE_STATUS_NSP_LOOP_GROUND);
}

static void ndsP4DededeNSPAirLoopInitial(GObj *fighter_gobj)
{
    ndsP4DededeNSPLoopInitial(fighter_gobj, DEDEDE_STATUS_NSP_LOOP_AIR);
}

/* DededeNSP.inhale_loop_ground_to_air_initial_: Kirby's switch, absorbing. */
static void ndsP4DededeNSPLoopGroundToAir(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    mpCommonSetFighterAir(fp);
    ftMainSetStatus(fighter_gobj, DEDEDE_STATUS_NSP_LOOP_AIR, fighter_gobj->anim_frame, 1.0F,
                    DEDEDE_PRESERVE_NSP_LOOP_SWITCH);
    ftKirbySpecialNSetCatchParams(fp);
    ndsP4DededeAbsorbSetup(fighter_gobj);
}

/* DededeNSP.inhale_loop_air_to_ground_initial_. */
static void ndsP4DededeNSPLoopAirToGround(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    mpCommonSetFighterGround(fp);
    ftMainSetStatus(fighter_gobj, DEDEDE_STATUS_NSP_LOOP_GROUND, fighter_gobj->anim_frame, 1.0F,
                    DEDEDE_PRESERVE_NSP_LOOP_SWITCH);
    ftKirbySpecialNSetCatchParams(fp);
    ndsP4DededeAbsorbSetup(fighter_gobj);
}

/* DededeNSP.inhale_loop_ground_to_air_check_ (0xE8 map). */
void ndsP4DededeNSPLoopGroundMap(GObj *fighter_gobj)
{
    mpCommonProcFighterOnFloor(fighter_gobj, ndsP4DededeNSPLoopGroundToAir);
}

/* DededeNSP.inhale_loop_air_to_ground_check_ (0xF1 map). */
void ndsP4DededeNSPLoopAirMap(GObj *fighter_gobj)
{
    mpCommonProcFighterLanding(fighter_gobj, ndsP4DededeNSPLoopAirToGround);
}

/* DededeNSP.check_for_spit_: an A or B tap spits; a held fighter takes 16
 * first. */
static sb32 ndsP4DededeNSPCheckSpit(GObj *fighter_gobj, void (*proc_status)(GObj *))
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (((fp->input.pl.button_tap & fp->input.button_mask_a) == 0) &&
        ((fp->input.pl.button_tap & fp->input.button_mask_b) == 0))
    {
        return FALSE;
    }
    if (fp->catch_gobj != NULL)
    {
        ftKirbySpecialNApplyCaptureDamage(fighter_gobj, fp->catch_gobj, DEDEDE_NSP_SPIT_DAMAGE);
    }
    proc_status(fighter_gobj);

    return TRUE;
}

/* DededeNSP.ground_spit_transition_ / air_spit_transition_: Kirby's throw. */
static void ndsP4DededeNSPSpitSetStatus(GObj *fighter_gobj, s32 status_id)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    ftMainSetStatus(fighter_gobj, status_id, 0.0F, 1.0F, DEDEDE_PRESERVE_NSP_SPIT);
    ftParamSetCaptureImmuneMask(fp, FTCATCHKIND_MASK_ALL);
    ftMainPlayAnimEventsAll(fighter_gobj);
}

static void ndsP4DededeNSPGroundSpit(GObj *fighter_gobj)
{
    ndsP4DededeNSPSpitSetStatus(fighter_gobj, DEDEDE_STATUS_NSP_SPIT_GROUND);
}

static void ndsP4DededeNSPAirSpit(GObj *fighter_gobj)
{
    ndsP4DededeNSPSpitSetStatus(fighter_gobj, DEDEDE_STATUS_NSP_SPIT_AIR);
}

/* DededeNSP.ground_walk_initial_2_: the walk for the stick's reach. Read as
 * assembled its "same walk" test reads the speed table's neighbour words
 * (its own code), never a walk status, so it always changes status. */
static void ndsP4DededeNSPWalkSetStatus(GObj *fighter_gobj, f32 frame_begin)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    s32 status_id = ftCommonWalkGetWalkStatus(fp->input.pl.stick_range.x) +
                    (DEDEDE_STATUS_NSP_WALK_1 - nFTCommonStatusWalkSlow);

    ftMainSetStatus(fighter_gobj, status_id, frame_begin, 1.0F, FTSTATUS_PRESERVE_NONE);
    ftMainPlayAnimEventsAll(fighter_gobj);
}

/* DededeNSP.ground_walk_to_idle_initial_. */
static void ndsP4DededeNSPWalkToIdle(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (fp->ga == nMPKineticsAir)
    {
        mpCommonSetFighterGround(fp);
    }
    ftMainSetStatus(fighter_gobj, DEDEDE_STATUS_NSP_IDLE_GROUND, 0.0F, 1.0F, FTSTATUS_PRESERVE_NONE);
}

/* DededeNSP.ground_idle_interrupt_ (0xEC interrupt): spit, Kirby's turn
 * (0xEE through his status change), or walk. */
void ndsP4DededeNSPIdleInterrupt(GObj *fighter_gobj)
{
    if (ndsP4DededeNSPCheckSpit(fighter_gobj, ndsP4DededeNSPGroundSpit) != FALSE)
    {
        return;
    }
    if (ftKirbySpecialNTurnCheckGotoTurn(fighter_gobj, ftKirbySpecialNTurnSetStatus) != FALSE)
    {
        return;
    }
    /* ground_walk_transition_check_ */
    if (ftCommonWalkCheckInputSuccess(fighter_gobj) != FALSE)
    {
        ndsP4DededeNSPWalkSetStatus(fighter_gobj, 0.0F);
    }
}

/* DededeNSP.ground_walk_interrupt_ (0xFF-0x101 interrupt): spit, stop, or
 * change walk from the current frame. */
void ndsP4DededeNSPWalkInterrupt(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    s32 stick_x;

    if (ndsP4DededeNSPCheckSpit(fighter_gobj, ndsP4DededeNSPGroundSpit) != FALSE)
    {
        return;
    }
    if (ftCommonWaitCheckInputSuccess(fighter_gobj) != FALSE)
    {
        ndsP4DededeNSPWalkToIdle(fighter_gobj);
        return;
    }
    stick_x = ABS(fp->input.pl.stick_range.x);

    if ((ftCommonWalkGetWalkStatus((s8)stick_x) + (DEDEDE_STATUS_NSP_WALK_1 - nFTCommonStatusWalkSlow)) ==
        fp->status_id)
    {
        return;
    }
    ndsP4DededeNSPWalkSetStatus(fighter_gobj, fighter_gobj->anim_frame);
}

/* DededeNSP.ground_walk_fall_initial_: off the floor into the inhale fall,
 * its motion held at frame 0 (speed 0, as assembled). */
static void ndsP4DededeNSPWalkFall(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (fp->ga == nMPKineticsGround)
    {
        mpCommonSetFighterAir(fp);
    }
    ftMainSetStatus(fighter_gobj, DEDEDE_STATUS_NSP_FALL, 0.0F, 0.0F, FTSTATUS_PRESERVE_FASTFALL);
    ftPhysicsClampAirVelXMax(fp);
}

/* DededeNSP.ground_walk_collision_ (0xFF-0x101 map). */
void ndsP4DededeNSPWalkMap(GObj *fighter_gobj)
{
    mpCommonProcFighterOnFloor(fighter_gobj, ndsP4DededeNSPWalkFall);
}

/* DededeNSP.air_fall_interrupt_ (0xF4 interrupt). */
void ndsP4DededeNSPFallInterrupt(GObj *fighter_gobj)
{
    (void)ndsP4DededeNSPCheckSpit(fighter_gobj, ndsP4DededeNSPAirSpit);
}

/* DededeNSP.spat_deceleration_: the spat fighter's physics. */
static void ndsP4DededeNSPSpatPhysics(GObj *fighter_gobj)
{
    ftCommonThrownCommonStarUpdatePhysics(fighter_gobj, DEDEDE_NSP_SPIT_DECELERATION);
}

/* With nothing held (an absorbed weapon or item) he spits a Star Rod star.
 * Read as assembled its smash flag is the low byte of a register the part
 * query leaves, an aligned pointer: never TRUE, never zero, so the star
 * keeps the tilt attributes and flies at the smash speed for the smash
 * lifetime. */
static void ndsP4DededeNSPSpitStar(GObj *fighter_gobj, Vec3f *pos)
{
#if NDS_P2_ITEM_CORE
    void **attributes;

    if (gITManagerCommonData == NULL)
    {
        return;
    }
    attributes = lbRelocGetFileData(void **, gITManagerCommonData,
                                    &llITCommonDataStarRodWeaponAttributes);
    if ((attributes == NULL) || (*attributes == NULL))
    {
        return;
    }
    (void)itStarRodWeaponStarMakeWeapon(fighter_gobj, pos, 2);
#else
    (void)fighter_gobj;
    (void)pos;
#endif
}

/* DededeNSP.spit_shoot_check_: at the script's temp variable 3 the held
 * fighter leaves as Kirby's star, faster (240) and slower to stop (10);
 * a content's star deals Remix's kirby_inhale_struct damage. */
static void ndsP4DededeNSPSpitShoot(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    FTStruct *victim_fp;
    GObj *victim_gobj;
    Vec3f pos;
    s32 star_damage;
    s32 i;

    if (fp->motion_vars.flags.flag2 == 0)
    {
        return;
    }
    fp->motion_vars.flags.flag2 = 0;
    victim_gobj = fp->catch_gobj;

    if (victim_gobj == NULL)
    {
        pos.x = pos.y = pos.z = 0.0F;
        gmCollisionGetFighterPartsWorldPosition(fp->joints[DEDEDE_NSP_STAR_JOINT], &pos);
        ndsP4DededeNSPSpitStar(fighter_gobj, &pos);
        return;
    }
    victim_fp = ftGetStruct(victim_gobj);

    ftCommonThrownKirbyStarSetStatus(victim_gobj);

    star_damage = ndsP4KirbyStarDamage(victim_fp);

    if (star_damage >= 0)
    {
        for (i = 0; i < ARRAY_COUNT(victim_fp->attack_colls); i++)
        {
            if (victim_fp->attack_colls[i].attack_state == nGMAttackStateNew)
            {
                victim_fp->attack_colls[i].damage = star_damage;
            }
        }
    }
    ftParamSetThrowParams(victim_fp, fighter_gobj);

    victim_fp->proc_physics = ndsP4DededeNSPSpatPhysics;
    victim_fp->physics.vel_air.z = 0.0F;
    victim_fp->physics.vel_air.y = 0.0F;
    victim_fp->physics.vel_air.x = (f32)(-victim_fp->lr) * DEDEDE_NSP_SPIT_VELOCITY;
}

/* DededeNSP.ground_spit_main_ (0xED update): the end of motion takes him to
 * the fall on the ground as well (Remix's routine ends both with it). */
void ndsP4DededeNSPGroundSpitMain(GObj *fighter_gobj)
{
    ndsP4DededeNSPSpitShoot(fighter_gobj);
    ftAnimEndSetFall(fighter_gobj);
}

/* DededeNSP.air_spit_main_ (0xF5 update). */
void ndsP4DededeNSPAirSpitMain(GObj *fighter_gobj)
{
    ndsP4DededeNSPSpitShoot(fighter_gobj);
    ftAnimEndSetFall(fighter_gobj);
}

/* DededeNSP.absorb_initial_ (custom_reflect_table, through the reflect
 * switch): a weapon or item swallowed, straight to the spit. */
static void ndsP4DededeOnCustomReflect(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    ftKirbySpecialNEatSetStatusParam(fighter_gobj, (fp->ga != nMPKineticsGround) ?
                                     DEDEDE_STATUS_NSP_SPIT_AIR : DEDEDE_STATUS_NSP_SPIT_GROUND);
}

/* Remix's DededeNSP hooks on Kirby's status changes (25 patches in
 * ftkirbyspecialn.c), each for its own Kirby status; the loops, which his
 * own routines start, take his loops. */
static s32 ndsP4DededeKirbyInhaleStatus(s32 status_id)
{
    switch (status_id)
    {
    case nFTKirbyStatusSpecialNStart:
        return DEDEDE_STATUS_NSP_BEGIN_GROUND;

    case nFTKirbyStatusSpecialNLoop:
        return DEDEDE_STATUS_NSP_LOOP_GROUND;

    case nFTKirbyStatusSpecialNEnd:
        return DEDEDE_STATUS_NSP_END_GROUND;

    case nFTKirbyStatusSpecialNCatch:
        return DEDEDE_STATUS_NSP_PULL_GROUND;

    case nFTKirbyStatusSpecialNEat:
        return DEDEDE_STATUS_NSP_SWALLOW_GROUND;

    case nFTKirbyStatusSpecialNThrow:
        return DEDEDE_STATUS_NSP_SPIT_GROUND;

    case nFTKirbyStatusSpecialNWait:
        return DEDEDE_STATUS_NSP_IDLE_GROUND;

    case nFTKirbyStatusSpecialNTurn:
        return DEDEDE_STATUS_NSP_TURN_GROUND;

    case nFTKirbyStatusSpecialAirNStart:
        return DEDEDE_STATUS_NSP_BEGIN_AIR;

    case nFTKirbyStatusSpecialAirNLoop:
        return DEDEDE_STATUS_NSP_LOOP_AIR;

    case nFTKirbyStatusSpecialAirNEnd:
        return DEDEDE_STATUS_NSP_END_AIR;

    case nFTKirbyStatusSpecialAirNCatch:
        return DEDEDE_STATUS_NSP_PULL_AIR;

    case nFTKirbyStatusSpecialAirNEat:
        return DEDEDE_STATUS_NSP_SWALLOW_AIR;

    case nFTKirbyStatusSpecialAirNThrow:
        return DEDEDE_STATUS_NSP_SPIT_AIR;

    case nFTKirbyStatusSpecialAirNWait:
        return DEDEDE_STATUS_NSP_FALL;

    case nFTKirbyStatusSpecialAirNTurn:
        return DEDEDE_STATUS_NSP_TURN_AIR;

    default:
        return status_id;
    }
}

/* Dedede.custom_initial_absorbed_timer_: held half as long as Kirby holds,
 * plus the victim's percent. */
static s32 ndsP4DededeKirbyCaptureWait(FTStruct *victim_fp, s32 breakout_wait)
{
    (void)breakout_wait;

    return DEDEDE_NSP_CAPTURE_WAIT + victim_fp->percent_damage;
}

/* ---- Down special (the minion toss) ---- */

static void ndsP4DededeDSPGroundChargeInitial(GObj *fighter_gobj);
static void ndsP4DededeDSPAirChargeInitial(GObj *fighter_gobj);
static void ndsP4DededeDSPGroundShootInitial(GObj *fighter_gobj);
static void ndsP4DededeDSPAirShootInitial(GObj *fighter_gobj);

/* DededeDSP.destroy_attached_minion_. */
static void ndsP4DededeDSPDestroyMinion(FTStruct *fp)
{
    GObj *minion_gobj = *ndsP4DededeMinion(fp);

    *ndsP4DededeMinion(fp) = NULL;

    if (minion_gobj != NULL)
    {
        itGetStruct(minion_gobj)->kind = DEDEDE_DSP_FAKE_ITEM_KIND;
        itMainDestroyItem(minion_gobj);
    }
}

/* DededeDSP.dedede_on_hit_subroutine_ (the toss statuses' proc_damage):
 * a hit drops the charge and the minion. */
static void ndsP4DededeDSPOnDamage(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    *ndsP4DededePassiveS32(fp, 2) = 0;
    ndsP4DededeDSPDestroyMinion(fp);
}

/* DededeDSP.attach_minion_: a Waddle Dee, Waddle Doo or Gordo by the charge,
 * in his hand (S6: none yet). */
static void ndsP4DededeDSPAttachMinion(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    fp->proc_damage = ndsP4DededeDSPOnDamage;
    *ndsP4DededeMinion(fp) = NULL;
}

/* DededeDSP.begin_initial_: a full charge goes straight to the toss. */
static void ndsP4DededeDSPBeginInitial(GObj *fighter_gobj, s32 status_id)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    ftMainSetStatus(fighter_gobj, status_id, 0.0F, 1.0F, FTSTATUS_PRESERVE_NONE);
    ftMainPlayAnimEventsAll(fighter_gobj);

    /* dedede_on_hit_subroutine_establishment_ */
    fp->proc_damage = ndsP4DededeDSPOnDamage;
    *ndsP4DededeMinion(fp) = NULL;
    fp->motion_vars.flags.flag0 = 0;

    *ndsP4DededeStatusS32(fp, 0) = (*ndsP4DededePassiveS32(fp, 2) == DEDEDE_DSP_CHARGE_MAX) ? 1 : 0;
}

/* DededeDSP.ground_begin_initial_ (ground_dsp). */
void ndsP4DededeDSPGroundInitial(GObj *fighter_gobj)
{
    ndsP4DededeDSPBeginInitial(fighter_gobj, DEDEDE_STATUS_DSPG_BEGIN);
}

/* DededeDSP.air_begin_initial_ (air_dsp). */
void ndsP4DededeDSPAirInitial(GObj *fighter_gobj)
{
    ndsP4DededeDSPBeginInitial(fighter_gobj, DEDEDE_STATUS_DSPA_BEGIN);
}

/* DededeDSP.begin_main_ (0xF8/0xFB update): charge or toss at the end. */
void ndsP4DededeDSPBeginMain(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    s32 shoot;

    if (!(fighter_gobj->anim_frame <= 0.0F))
    {
        return;
    }
    shoot = *ndsP4DededeStatusS32(fp, 0);

    if (fp->ga != nMPKineticsGround)
    {
        if (shoot != 0)
        {
            ndsP4DededeDSPAirShootInitial(fighter_gobj);
        }
        else ndsP4DededeDSPAirChargeInitial(fighter_gobj);
    }
    else if (shoot != 0)
    {
        ndsP4DededeDSPGroundShootInitial(fighter_gobj);
    }
    else ndsP4DededeDSPGroundChargeInitial(fighter_gobj);
}

/* DededeDSP.air_begin_transition_. */
static void ndsP4DededeDSPBeginGroundToAir(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    mpCommonSetFighterAir(fp);
    ftPhysicsClampAirVelXMax(fp);
    ftMainSetStatus(fighter_gobj, DEDEDE_STATUS_DSPA_BEGIN, fighter_gobj->anim_frame, 1.0F,
                    DEDEDE_PRESERVE_DSP);
    fp->proc_damage = ndsP4DededeDSPOnDamage;
}

/* DededeDSP.ground_begin_transition_. */
static void ndsP4DededeDSPBeginAirToGround(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    mpCommonSetFighterGround(fp);
    ftMainSetStatus(fighter_gobj, DEDEDE_STATUS_DSPG_BEGIN, fighter_gobj->anim_frame, 1.0F,
                    DEDEDE_PRESERVE_DSP);
    fp->proc_damage = ndsP4DededeDSPOnDamage;
}

/* DededeDSP.ground_begin_collision_ (0xF8 map). */
void ndsP4DededeDSPBeginGroundMap(GObj *fighter_gobj)
{
    mpCommonProcFighterOnEdge(fighter_gobj, ndsP4DededeDSPBeginGroundToAir);
}

/* DededeDSP.air_begin_collision_ (0xFB map). */
void ndsP4DededeDSPBeginAirMap(GObj *fighter_gobj)
{
    mpCommonProcFighterLanding(fighter_gobj, ndsP4DededeDSPBeginAirToGround);
}

/* DededeDSP.ground_charge_initial_ / air_charge_initial_. */
static void ndsP4DededeDSPChargeInitial(GObj *fighter_gobj, s32 status_id)
{
    *ndsP4DededeStatusS32(ftGetStruct(fighter_gobj), 1) = 0;
    ftMainSetStatus(fighter_gobj, status_id, 0.0F, 1.0F, DEDEDE_PRESERVE_DSP);
    ftGetStruct(fighter_gobj)->proc_damage = ndsP4DededeDSPOnDamage;
}

static void ndsP4DededeDSPGroundChargeInitial(GObj *fighter_gobj)
{
    ndsP4DededeDSPChargeInitial(fighter_gobj, DEDEDE_STATUS_DSPG_CHARGE);
}

static void ndsP4DededeDSPAirChargeInitial(GObj *fighter_gobj)
{
    ndsP4DededeDSPChargeInitial(fighter_gobj, DEDEDE_STATUS_DSPA_CHARGE);
}

/* DededeDSP.charge_main_ (0xF9/0xFC update): a minion comes out and goes
 * back twice, each time adding a charge, then the third stays out; fully
 * charged he stops past frame 20. */
void ndsP4DededeDSPChargeMain(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    s32 *timer = ndsP4DededeStatusS32(fp, 1);
    s32 *charge = ndsP4DededePassiveS32(fp, 2);

    *timer += 1;

    switch (*timer)
    {
    case DEDEDE_DSP_FIRST_PULL:
    case DEDEDE_DSP_SECOND_PULL:
    case DEDEDE_DSP_FINAL_PULL:
        ndsP4DededeDSPAttachMinion(fighter_gobj);
        return;

    case DEDEDE_DSP_FIRST_STOW:
    case DEDEDE_DSP_SECOND_STOW:
        ndsP4DededeDSPDestroyMinion(fp);
        return;

    case DEDEDE_DSP_FIRST_CHARGE:
    case DEDEDE_DSP_SECOND_CHARGE:
        *charge += 1;

        if (*charge == DEDEDE_DSP_CHARGE_MAX)
        {
            (void)ftParamCheckSetFighterColAnimID(fighter_gobj, DEDEDE_DSP_COLANIM_CHARGE, 0);
        }
        return;
    }
    if (*charge != DEDEDE_DSP_CHARGE_MAX)
    {
        return;
    }
    if (fighter_gobj->anim_frame < DEDEDE_DSP_CHARGE_END_FRAME)
    {
        return;
    }
    mpCommonSetFighterWaitOrFall(fighter_gobj);
    ndsP4DededeDSPDestroyMinion(fp);
}

/* DededeDSP.ground_charge_interrupt_ / air_charge_interrupt_: past the
 * rummage time, letting go of B tosses; Z puts the minion away. The aerial
 * one times the status, not the charge. */
static void ndsP4DededeDSPChargeInterrupt(GObj *fighter_gobj, u32 tics,
                                          void (*proc_shoot)(GObj *),
                                          void (*proc_cancel)(GObj *))
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if ((tics >= DEDEDE_DSP_RUMMAGE_TIME) && ((fp->input.pl.button_hold & B_BUTTON) == 0))
    {
        proc_shoot(fighter_gobj);
        return;
    }
    if ((fp->input.button_mask_z & fp->input.pl.button_tap) != 0)
    {
        ndsP4DededeDSPDestroyMinion(fp);
        proc_cancel(fighter_gobj);
    }
}

/* DededeDSP.ground_charge_interrupt_ (0xF9 interrupt). */
void ndsP4DededeDSPGroundChargeInterrupt(GObj *fighter_gobj)
{
    ndsP4DededeDSPChargeInterrupt(fighter_gobj,
                                  (u32)*ndsP4DededeStatusS32(ftGetStruct(fighter_gobj), 1),
                                  ndsP4DededeDSPGroundShootInitial, ftCommonWaitSetStatus);
}

/* DededeDSP.air_charge_interrupt_ (0xFC interrupt). */
void ndsP4DededeDSPAirChargeInterrupt(GObj *fighter_gobj)
{
    ndsP4DededeDSPChargeInterrupt(fighter_gobj, (u32)ftGetStruct(fighter_gobj)->status_total_tics,
                                  ndsP4DededeDSPAirShootInitial, ftCommonFallSetStatus);
}

/* DededeDSP.air_charge_transition_. */
static void ndsP4DededeDSPChargeGroundToAir(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    mpCommonSetFighterAir(fp);
    ftPhysicsClampAirVelXMax(fp);
    ftMainSetStatus(fighter_gobj, DEDEDE_STATUS_DSPA_CHARGE, fighter_gobj->anim_frame, 1.0F,
                    DEDEDE_PRESERVE_DSP_CHARGE);
    fp->proc_damage = ndsP4DededeDSPOnDamage;
}

/* DededeDSP.ground_charge_transition_. */
static void ndsP4DededeDSPChargeAirToGround(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    mpCommonSetFighterGround(fp);
    ftMainSetStatus(fighter_gobj, DEDEDE_STATUS_DSPG_CHARGE, fighter_gobj->anim_frame, 1.0F,
                    DEDEDE_PRESERVE_DSP_CHARGE);
    fp->proc_damage = ndsP4DededeDSPOnDamage;
}

/* DededeDSP.ground_charge_collision_ (0xF9 map). */
void ndsP4DededeDSPChargeGroundMap(GObj *fighter_gobj)
{
    mpCommonProcFighterOnEdge(fighter_gobj, ndsP4DededeDSPChargeGroundToAir);
}

/* DededeDSP.air_charge_collision_ (0xFC map). */
void ndsP4DededeDSPChargeAirMap(GObj *fighter_gobj)
{
    mpCommonProcFighterLanding(fighter_gobj, ndsP4DededeDSPChargeAirToGround);
}

/* DededeDSP.ground_shoot_initial_ / air_shoot_initial_: the toss, a minion
 * in hand first if none is. */
static void ndsP4DededeDSPShootInitial(GObj *fighter_gobj, s32 status_id)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    *ndsP4DededeStatusS32(fp, 4) = 0;
    ftMainSetStatus(fighter_gobj, status_id, 0.0F, 1.0F, FTSTATUS_PRESERVE_NONE);
    fp->is_special_interrupt = TRUE;

    if (*ndsP4DededeMinion(fp) == NULL)
    {
        ndsP4DededeDSPAttachMinion(fighter_gobj);
    }
    ftMainPlayAnimEventsAll(fighter_gobj);
    fp->proc_damage = ndsP4DededeDSPOnDamage;
}

static void ndsP4DededeDSPGroundShootInitial(GObj *fighter_gobj)
{
    ndsP4DededeDSPShootInitial(fighter_gobj, DEDEDE_STATUS_DSPG_SHOOT);
}

static void ndsP4DededeDSPAirShootInitial(GObj *fighter_gobj)
{
    ndsP4DededeDSPShootInitial(fighter_gobj, DEDEDE_STATUS_DSPA_SHOOT);
}

/* The held minion's throw flag (item + 0x33E, S6). */
static void ndsP4DededeDSPThrowMinion(GObj *minion_gobj)
{
    (void)minion_gobj;
}

/* DededeDSP.shoot_main_ (0xFA/0xFD update): at the script's temp variable 1
 * the charge empties and the held minion is thrown; the end of motion
 * stands or drops him. Its hand-position query feeds nothing. */
void ndsP4DededeDSPShootMain(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    GObj *minion_gobj;

    *ndsP4DededeStatusS32(fp, 4) += 1;

    if (fp->motion_vars.flags.flag0 != 0)
    {
        fp->motion_vars.flags.flag0 = 0;
        *ndsP4DededePassiveS32(fp, 2) = 0;
        minion_gobj = *ndsP4DededeMinion(fp);

        if (minion_gobj != NULL)
        {
            *ndsP4DededeMinion(fp) = NULL;
            ndsP4DededeDSPThrowMinion(minion_gobj);
        }
    }
    if (fighter_gobj->anim_frame <= 0.0F)
    {
        mpCommonSetFighterWaitOrFall(fighter_gobj);
    }
}

/* DededeDSP.ground_to_air: the aerial toss, three statuses on. */
static void ndsP4DededeDSPShootGroundToAir(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    mpCommonSetFighterAir(fp);
    ftMainSetStatus(fighter_gobj, fp->status_id + 3, fighter_gobj->anim_frame, 1.0F, DEDEDE_PRESERVE_DSP);
    ftPhysicsClampAirVelXMax(fp);
    fp->proc_damage = ndsP4DededeDSPOnDamage;
}

/* DededeDSP.air_to_ground. */
static void ndsP4DededeDSPShootAirToGround(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    mpCommonSetFighterGround(fp);
    ftMainSetStatus(fighter_gobj, fp->status_id - 3, fighter_gobj->anim_frame, 1.0F, DEDEDE_PRESERVE_DSP);
    fp->proc_damage = ndsP4DededeDSPOnDamage;
}

/* DededeDSP.ground_shoot_collision_ (0xFA map). */
void ndsP4DededeDSPShootGroundMap(GObj *fighter_gobj)
{
    mpCommonProcFighterOnEdge(fighter_gobj, ndsP4DededeDSPShootGroundToAir);
}

/* DededeDSP.air_shoot_collision_ (0xFD map). */
void ndsP4DededeDSPShootAirMap(GObj *fighter_gobj)
{
    mpCommonProcFighterLanding(fighter_gobj, ndsP4DededeDSPShootAirToGround);
}

/* ---- Patches on his id ---- */

/* Dedede.initial_script_ (spawn and rebirth): no minions, no charge. */
static void ndsP4DededeOnInit(FTStruct *fp)
{
    *ndsP4DededePassiveS32(fp, 0) = 0;
    *ndsP4DededePassiveS32(fp, 1) = 0;
    *ndsP4DededePassiveS32(fp, 2) = 0;
}

/* kirby_cpu_inhale_4 (jigglypuffkirbyshared.asm): Kirby's stars where he
 * meets a wall, ceiling or floor, in ftMainProcPhysicsMap after the map
 * proc. Here after the slope proc, which turns his feet and not his
 * position; with no map proc the contact masks were not read this frame,
 * so no stars, as in the source. */
static void ndsP4DededeAfterProcMap(GObj *fighter_gobj)
{
    if (ftGetStruct(fighter_gobj)->proc_map != NULL)
    {
        ftParamKirbyTryMakeMapStarEffect(fighter_gobj);
    }
}

/* Dedede.jump_multiplier_table, jumps 3-6 (jump_fix_2): the aerial jumps'
 * heights through Kirby's multi-jump branches (jump_fix_1, 3, 4 and 5). */
static const f32 sNdsP4DededeJumpVelocities[4] = { 68.0F, 58.0F, 52.0F, 0.0F };

const NDSP4Overrides gNdsP4DededeOverrides = {
    .after_proc_map = ndsP4DededeAfterProcMap,
    .multi_jump_vel = sNdsP4DededeJumpVelocities,
    .on_init = ndsP4DededeOnInit,
    .kirby_inhale_status = ndsP4DededeKirbyInhaleStatus,
    .kirby_capture_wait = ndsP4DededeKirbyCaptureWait,
    .kirby_inhale_wind_x = DEDEDE_NSP_WIND_X,
    .on_custom_reflect = ndsP4DededeOnCustomReflect,
};

#endif /* NDS_P4_DEDEDE */
