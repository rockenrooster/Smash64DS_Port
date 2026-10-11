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
 * file 2 (YoshiShared.asm down_special_struct_fix, S6, below). His minions
 * (Waddle Dee, Waddle Doo and Gordo) are P4 item kinds on his special file 1
 * and the Waddle Doo's beam a P4 weapon kind (S6, below); the charged hand's
 * colour is Remix's DEDEDE_CHARGE routine (p4_colanims.py), which his
 * gfx_routine_end case brings back; Kirby wearing his hat is S8's; the CPU's recovery
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
#if !NDS_P2_ITEM_CORE
#error "P4 Dedede's minions run the common items' code (NDS_P2_ITEM_CORE)"
#endif
#if !NDS_P2_SAMUS
#error "P4 Dedede's minions spawn at Samus's charge shot spot (NDS_P2_SAMUS)"
#endif

#include <stddef.h>

#include <ef/effect.h>
#include <it/item.h>
#include <macros.h>
#include <reloc_data.h>
#include <sc/scene.h>
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

/* ---- His minions (S6): Waddle Dee, Waddle Doo and Gordo ----
 *
 * Remix items/WaddleDee.asm, WaddleDoo.asm and Gordo.asm, read as assembled.
 * attach_minion_ makes one in his hand by the charge (0 a Waddle Dee, 1 a
 * Waddle Doo, else a Gordo) from WADDLE_DEE_INFO, his special file 1, whose
 * item attributes (0x40, 0xB0, 0x120) name the models in WADDLE_DEE_GFX. A
 * held minion follows his hand; the toss sets its throw flag (the item's
 * `multi`), and it flies, lands, walks and attacks on its own until its
 * duration runs out. A Waddle Doo's attack is a beam of four stars on the
 * Star Rod star's attributes, each placed off the one before.
 *
 * The donor keeps its own variables in the item struct by their N64 offsets
 * (0x1C4 owner, 0x1C8 current max speed, 0x1CC rotation direction, 0x1D0
 * hitbox refresh timer, 0x1D4 hitbox collision flag: inside the attack
 * collision's position records, which the DS struct lays out as BattleShip
 * does; 0x224-0x23C are its records' victim words) and walks with the
 * Bob-omb's walk at the Bob-omb's offsets from the attributes' tree, where
 * Remix laid the minion files out to match. Not ported: the Pokemon Stadium
 * announcer, the 1P Dedede Team case of minion_free_ and the donor's copy of
 * his handicap into its own item's `type` byte (no reader; the beam takes
 * the handicap from him directly). */

#define NDS_P4_DEDEDE_ITW(ip_, off_) (*(s32 *)(void *)((u8 *)(ip_) + (off_)))
#define NDS_P4_DEDEDE_ITF(ip_, off_) (*(f32 *)(void *)((u8 *)(ip_) + (off_)))
_Static_assert(offsetof(ITStruct, attack_coll) == 0x10C, "ITStruct is not N64's layout");
_Static_assert(offsetof(ITStruct, damage_queue) == 0x298, "ITStruct is not N64's layout");
_Static_assert(offsetof(ITStruct, item_vars) == 0x350, "ITStruct is not N64's layout");
_Static_assert(offsetof(WPStruct, weapon_vars) == 0x29C, "WPStruct is not N64's layout");

#define MINION_OWNER 0x1C4
#define MINION_MAX_SPEED_CUR 0x1C8
#define MINION_ROTATION_DIR 0x1CC
#define MINION_HIT_REFRESH 0x1D0
#define MINION_HIT_FLAG 0x1D4
#define MINION_ITEM_VARS_1 0x354     /* "unknown value", written 4 */

/* WaddleDee.minion_attributes */
#define MINION_DURATION 480
#define MINION_GRAVITY 2.0F
#define MINION_MAX_SPEED 48.0F
#define MINION_BOUNCE 0.3F
#define MINION_ANGLE 0.872665F
#define MINION_ROTATION 0.003F
#define MINION_GROUND_SPEED 15.0F
#define MINION_LAND_LAG 16
#define MINION_H_JUMP_SPEED 8.0F
#define MINION_V_JUMP_SPEED 40.0F
#define MINION_BEAM_CHARGE_TIME 20
#define MINION_BEAM_INITIAL_ANGLE 0x3F900000u
#define MINION_BEAM_DISTANCE 256.0F
#define MINION_BEAM_ROTATION 0x3E860000u
#define MINION_BEAM_DURATION 10
#define MINION_WADDLE_DEE_WALK_ATTACK_DELAY 4u
#define MINION_WADDLE_DOO_WALK_ATTACK_DELAY 8u
#define MINION_FGM_STEP 0x44B        /* Dedede/sounds/WADDLE_STEP_L */
#define MINION_FGM_JUMP 0x44A        /* Dedede/sounds/WADDLE_JUMP */
#define MINION_FGM_DOO_CHARGE 0xDC
#define MINION_FGM_DOO_SHOOT 0x40
/* Bob-omb walk offsets (reloc_data.us.h: DataStart, WalkRight, WalkLeft). */
#define MINION_WALK_R_DL (0x3310 - 0x33F8)
#define MINION_WALK_L_DL (0x34C0 - 0x33F8)
/* Gordo.gordo_attributes */
#define GORDO_DURATION 220
#define GORDO_GRAVITY 2.0F
#define GORDO_MAX_SPEED 48.0F
#define GORDO_BOUNCE 0.95F
#define GORDO_ANGLE 0.872665F
#define GORDO_ROTATION 0.001F
#define GORDO_DAMAGE 18
#define GORDO_SURFACE_STICK_FGM 0x107
#define GORDO_SURFACE_STICK_DAMAGE 5
/* WaddleDoo.asm's beam */
#define BEAM_STAR_COUNT 4
#define BEAM_STAR_DAMAGE 2
#define BEAM_STAR_ELEMENT 3          /* commented "electric"; 3 is Slash */
#define BEAM_STAR_FGM 0x16           /* FGM.hit.ELECTRIC_S */
#define BEAM_START_UP 10
/* itStarRodWeaponStarMakeWeapon's smash star speed (itstarrod.h). */
#define BEAM_STAR_MAKER_VEL_X 120.0F

extern void *gNdsP4DededeSpecial1;
extern uintptr_t llITCommonDataStarRodWeaponAttributes;
void itBombHeiCommonSetWalkLR(GObj *item_gobj, ub8 lr);
void itBombHeiWalkInitVars(GObj *item_gobj);
void itMSBombAttachedUpdateSurface(GObj *item_gobj);
void mpCommonRunItemCollisionDefault(GObj *item_gobj, Vec3f *pos, MPCollData *coll_data);
void ftSamusSpecialNGetChargeShotPosition(FTStruct *fp, Vec3f *pos);
sb32 itStarRodWeaponStarProcUpdate(GObj *weapon_gobj);
LBParticle *efManagerSparkleWhiteScaleMakeEffect(Vec3f *pos, f32 scale);

static f32 ndsP4DededeMinionBits(u32 bits)
{
    union { u32 u; f32 f; } v;

    v.u = bits;
    return v.f;
}

/* The donor's zero tests on a float are word tests (beqz on the bits). */
static sb32 ndsP4DededeMinionZeroBits(f32 value)
{
    union { f32 f; u32 u; } v;

    v.f = value;
    return (v.u == 0u) ? TRUE : FALSE;
}

/* cvt.w.s under the N64's round-to-nearest-even mode (speeds are >= 0). */
static s32 ndsP4DededeMinionRoundEven(f32 value)
{
    s32 whole = (s32)value;
    f32 frac = value - (f32)whole;

    if ((frac > 0.5F) || ((frac == 0.5F) && ((whole & 1) != 0)))
    {
        whole++;
    }
    return whole;
}

static sb32 ndsP4DededeMinionNullMain(GObj *item_gobj);
static sb32 ndsP4DededeDeeAerialMain(GObj *item_gobj);
static sb32 ndsP4DededeDeeCollision(GObj *item_gobj);
static sb32 ndsP4DededeMinionHurtboxCollision(GObj *item_gobj);
static sb32 ndsP4DededeMinionHitboxCollision(GObj *item_gobj);
static sb32 ndsP4DededeDeeLandingMain(GObj *item_gobj);
static sb32 ndsP4DededeDeeRestingCollision(GObj *item_gobj);
static sb32 ndsP4DededeMinionWalkMain(GObj *item_gobj);
static sb32 ndsP4DededeWaddleGroundCollision(GObj *item_gobj);
static sb32 ndsP4DededeDeeJumpsquatMain(GObj *item_gobj);
static sb32 ndsP4DededeDooAttackMain(GObj *item_gobj);
static sb32 ndsP4DededeMinionBlastZone(GObj *item_gobj);
static sb32 ndsP4DededeGordoNullMain(GObj *item_gobj);
static sb32 ndsP4DededeGordoAerialMain(GObj *item_gobj);
static sb32 ndsP4DededeGordoCollision(GObj *item_gobj);
static sb32 ndsP4DededeGordoHurtboxCollision(GObj *item_gobj);
static sb32 ndsP4DededeGordoHitboxCollision(GObj *item_gobj);
static sb32 ndsP4DededeGordoRestingMain(GObj *item_gobj);
static sb32 ndsP4DededeGordoRestingCollision(GObj *item_gobj);
static sb32 ndsP4DededeGordoRestingHurtboxCollision(GObj *item_gobj);

/* WaddleDee.item_state_table: held, aerial, landing, walking, jump squat,
 * jump. Every shared minion routine changes state through this table, a
 * Waddle Doo's too (its own differs only in states 4 and 5). */
static ITStatusDesc sNdsP4DededeDeeStatusDescs[6] = {
    { ndsP4DededeMinionNullMain, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
    { ndsP4DededeDeeAerialMain, ndsP4DededeDeeCollision, ndsP4DededeMinionHurtboxCollision,
      ndsP4DededeMinionHurtboxCollision, itMainCommonProcHop, NULL, itMainCommonProcReflector,
      ndsP4DededeMinionHitboxCollision },
    { ndsP4DededeDeeLandingMain, ndsP4DededeDeeRestingCollision, ndsP4DededeMinionHurtboxCollision,
      ndsP4DededeMinionHurtboxCollision, itMainCommonProcHop, NULL, itMainCommonProcReflector,
      ndsP4DededeMinionHitboxCollision },
    { ndsP4DededeMinionWalkMain, ndsP4DededeWaddleGroundCollision, NULL, NULL, NULL, NULL, NULL,
      ndsP4DededeMinionHitboxCollision },
    { ndsP4DededeDeeJumpsquatMain, ndsP4DededeWaddleGroundCollision, NULL, NULL, NULL, NULL, NULL,
      ndsP4DededeMinionHitboxCollision },
    { ndsP4DededeDeeJumpsquatMain, ndsP4DededeDeeCollision, ndsP4DededeMinionHurtboxCollision,
      ndsP4DededeMinionHurtboxCollision, NULL, NULL, NULL, ndsP4DededeMinionHitboxCollision },
};

/* WaddleDoo.item_state_table: states 4 and 5 are its beam. */
static ITStatusDesc sNdsP4DededeDooStatusDescs[6] = {
    { ndsP4DededeMinionNullMain, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
    { ndsP4DededeDeeAerialMain, ndsP4DededeDeeCollision, ndsP4DededeMinionHurtboxCollision,
      ndsP4DededeMinionHurtboxCollision, itMainCommonProcHop, NULL, itMainCommonProcReflector,
      ndsP4DededeMinionHitboxCollision },
    { ndsP4DededeDeeLandingMain, ndsP4DededeDeeRestingCollision, ndsP4DededeMinionHurtboxCollision,
      ndsP4DededeMinionHurtboxCollision, itMainCommonProcHop, NULL, itMainCommonProcReflector,
      ndsP4DededeMinionHitboxCollision },
    { ndsP4DededeMinionWalkMain, ndsP4DededeWaddleGroundCollision, NULL, NULL, NULL, NULL, NULL,
      ndsP4DededeMinionHitboxCollision },
    { ndsP4DededeDooAttackMain, ndsP4DededeWaddleGroundCollision, NULL, NULL, NULL, NULL, NULL,
      ndsP4DededeMinionHitboxCollision },
    { ndsP4DededeDooAttackMain, ndsP4DededeWaddleGroundCollision, NULL, NULL, NULL, NULL, NULL,
      ndsP4DededeMinionHitboxCollision },
};

/* Gordo.item_state_table: held, aerial, resting (stuck). */
static ITStatusDesc sNdsP4DededeGordoStatusDescs[3] = {
    { ndsP4DededeGordoNullMain, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
    { ndsP4DededeGordoAerialMain, ndsP4DededeGordoCollision, ndsP4DededeGordoHurtboxCollision,
      ndsP4DededeGordoHurtboxCollision, itMainCommonProcHop, NULL, itMainCommonProcReflector,
      ndsP4DededeGordoHitboxCollision },
    { ndsP4DededeGordoRestingMain, ndsP4DededeGordoRestingCollision,
      ndsP4DededeGordoRestingHurtboxCollision, NULL, NULL, NULL, NULL, NULL },
};

/* The three item_info_arrays: his special file 1, transforms 0x1B. */
static ITDesc sNdsP4DededeDeeDesc = {
    NDS_P4_IT_KIND_WADDLE_DEE, &gNdsP4DededeSpecial1, (const void *)0x40,
    { nGCMatrixKindTraRotRpyR, nGCMatrixKindNull, 0x00 }, 0,
    ndsP4DededeMinionNullMain, NULL, NULL, NULL, NULL, NULL, NULL, NULL
};
static ITDesc sNdsP4DededeDooDesc = {
    NDS_P4_IT_KIND_WADDLE_DOO, &gNdsP4DededeSpecial1, (const void *)0xB0,
    { nGCMatrixKindTraRotRpyR, nGCMatrixKindNull, 0x00 }, 0,
    ndsP4DededeMinionNullMain, NULL, NULL, NULL, NULL, NULL, NULL, NULL
};
static ITDesc sNdsP4DededeGordoDesc = {
    NDS_P4_IT_KIND_GORDO, &gNdsP4DededeSpecial1, (const void *)0x120,
    { nGCMatrixKindTraRotRpyR, nGCMatrixKindNull, 0x00 }, 0,
    ndsP4DededeGordoNullMain, NULL, NULL, NULL, NULL, NULL, NULL, NULL
};

static void ndsP4DededeMinionImage(GObj *item_gobj, u16 image)
{
    MObj *mobj = DObjGetStruct(item_gobj)->mobj;

    if (mobj != NULL)
    {
        mobj->texture_id_curr = image;
    }
}

/* The image change the donor makes with its MatAnim pointers cleared. */
static void ndsP4DededeMinionImageStill(GObj *item_gobj, u16 image)
{
    MObj *mobj = DObjGetStruct(item_gobj)->mobj;

    if (mobj != NULL)
    {
        mobj->texture_id_curr = image;
        mobj->aobj = NULL;
        mobj->matanim_joint.event32 = NULL;
    }
}

static void ndsP4DededeMinionClearVictims(ITStruct *ip)
{
    NDS_P4_DEDEDE_ITW(ip, 0x224) = 0;
    NDS_P4_DEDEDE_ITW(ip, 0x22C) = 0;
    NDS_P4_DEDEDE_ITW(ip, 0x234) = 0;
    NDS_P4_DEDEDE_ITW(ip, 0x23C) = 0;
}

/* WaddleDee.ground_move_initial (the Bob-omb's walk turn, 0x80177104, at
 * the minion's own speed). */
static void ndsP4DededeGroundMoveInitial(GObj *item_gobj, sb32 right)
{
    ITStruct *ip = itGetStruct(item_gobj);
    DObj *dobj = DObjGetStruct(item_gobj);
    u8 *data = (u8 *)ip->attr->data;

    if (right != FALSE)
    {
        ip->lr = +1;
        ip->physics.vel_air.x = MINION_GROUND_SPEED;
        dobj->dl = (Gfx *)(void *)(data + MINION_WALK_R_DL);
    }
    else
    {
        ip->lr = -1;
        ip->physics.vel_air.x = -MINION_GROUND_SPEED;
        dobj->dl = (Gfx *)(void *)(data + MINION_WALK_L_DL);
    }
}

/* WaddleDee.minion_free_: a minion leaving takes itself out of his two
 * slots (slot 1 the oldest) and his hand, if he is still in the match. */
static void ndsP4DededeMinionFree(GObj *item_gobj)
{
    GObj *owner = (GObj *)(uintptr_t)NDS_P4_DEDEDE_ITW(itGetStruct(item_gobj), MINION_OWNER);
    GObj *fighter_gobj;
    FTStruct *fp;
    s32 *slot1;
    s32 *slot2;

    for (fighter_gobj = gGCCommonLinks[nGCCommonLinkIDFighter]; fighter_gobj != NULL;
         fighter_gobj = fighter_gobj->link_next)
    {
        if (fighter_gobj == owner)
        {
            break;
        }
    }
    if (fighter_gobj == NULL)
    {
        return;
    }
    fp = ftGetStruct(fighter_gobj);
    if (ndsP4Content(fp) != NDS_P4_ID_DEDEDE)
    {
        return;
    }
    if (*ndsP4DededeMinion(fp) == item_gobj)
    {
        *ndsP4DededeMinion(fp) = NULL;
    }
    slot1 = ndsP4DededePassiveS32(fp, 0);
    slot2 = ndsP4DededePassiveS32(fp, 1);
    if (*slot1 == (s32)(uintptr_t)item_gobj)
    {
        *slot1 = *slot2;
        *slot2 = 0;
    }
    else if (*slot2 == (s32)(uintptr_t)item_gobj)
    {
        *slot2 = 0;
    }
}

/* WaddleDee.minion_blast_zone_ (also the Gordo's): minion_free_'s result is
 * the minion's struct, never zero, so the item is destroyed. */
static sb32 ndsP4DededeMinionBlastZone(GObj *item_gobj)
{
    ndsP4DededeMinionFree(item_gobj);
    return TRUE;
}

/* WaddleDee.remove_minion_check_: a thrown minion takes a free slot; with
 * both full the oldest is given one frame left. */
static void ndsP4DededeMinionRemoveCheck(GObj *item_gobj)
{
    FTStruct *fp = ftGetStruct(itGetStruct(item_gobj)->owner_gobj);
    s32 *slot1 = ndsP4DededePassiveS32(fp, 0);
    s32 *slot2 = ndsP4DededePassiveS32(fp, 1);

    if (*slot1 == 0)
    {
        *slot1 = (s32)(uintptr_t)item_gobj;
        return;
    }
    if (*slot2 == 0)
    {
        *slot2 = (s32)(uintptr_t)item_gobj;
        return;
    }
    itGetStruct((GObj *)(uintptr_t)*slot1)->lifetime = 1;
    *slot1 = *slot2;
    *slot2 = (s32)(uintptr_t)item_gobj;
}

/* The ownership both stage_setting_s give: his player, team and object,
 * the last in the donor's owner word too; no hurtbox or hitbox. */
static void ndsP4DededeMinionOwn(ITStruct *ip, GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    ip->owner_gobj = fighter_gobj;
    ip->player = fp->player;
    ip->team = fp->team;
    NDS_P4_DEDEDE_ITW(ip, MINION_OWNER) = (s32)(uintptr_t)fighter_gobj;
    ip->damage_coll.hitstatus = nGMHitStatusNone;
    ip->attack_coll.attack_state = nGMAttackStateOff;
    NDS_P4_DEDEDE_ITF(ip, MINION_MAX_SPEED_CUR) = MINION_MAX_SPEED;   /* = GORDO_MAX_SPEED */
    NDS_P4_DEDEDE_ITW(ip, MINION_ROTATION_DIR) = 0;
    NDS_P4_DEDEDE_ITW(ip, MINION_HIT_REFRESH) = 0;
    NDS_P4_DEDEDE_ITW(ip, MINION_HIT_FLAG) = 0;
}

/* WaddleDee.stage_setting_ (Waddle Dee and Waddle Doo): the held image, no
 * clang, walking his way. */
static GObj *ndsP4DededeMinionMake(GObj *fighter_gobj, ITDesc *desc, Vec3f *pos, Vec3f *vel)
{
    GObj *item_gobj = itManagerMakeItem(fighter_gobj, desc, pos, vel, 0);
    ITStruct *ip;
    DObj *dobj;

    if (item_gobj == NULL)
    {
        return NULL;
    }
    ip = itGetStruct(item_gobj);
    dobj = DObjGetStruct(item_gobj);
    ndsP4DededeMinionImage(item_gobj, 1);
    dobj->translate.vec.f.z = 0.0F;
    ip->lifetime = 0;
    ip->multi = 0;
    gcAddXObjForDObjFixed(dobj, 0x2E, 0);
    ndsP4DededeMinionOwn(ip, fighter_gobj);
    ip->proc_dead = ndsP4DededeMinionBlastZone;
    ip->attack_coll.can_setoff = FALSE;
    ndsP4DededeGroundMoveInitial(item_gobj, (ftGetStruct(fighter_gobj)->lr == 1) ? TRUE : FALSE);
    return item_gobj;
}

/* Gordo.stage_setting_: its thrown damage, rehitting shields. */
static GObj *ndsP4DededeGordoMake(GObj *fighter_gobj, Vec3f *pos, Vec3f *vel)
{
    GObj *item_gobj = itManagerMakeItem(fighter_gobj, &sNdsP4DededeGordoDesc, pos, vel, 0);
    ITStruct *ip;
    DObj *dobj;

    if (item_gobj == NULL)
    {
        return NULL;
    }
    ip = itGetStruct(item_gobj);
    dobj = DObjGetStruct(item_gobj);
    dobj->translate.vec.f.z = 0.0F;
    ip->multi = 0;
    gcAddXObjForDObjFixed(dobj, 0x2E, 0);
    ip->attack_coll.can_rehit_shield = TRUE;
    ip->lifetime = 0;
    NDS_P4_DEDEDE_ITW(ip, MINION_ITEM_VARS_1) = 4;
    ndsP4DededeMinionOwn(ip, fighter_gobj);
    ip->attack_coll.damage = GORDO_DAMAGE;
    ip->proc_dead = ndsP4DededeMinionBlastZone;
    return item_gobj;
}

/* DededeDSP.attach_minion_ (the item part): at his hand (Samus's charge
 * shot spot), 65 up as its velocity, the minion by the charge. */
static GObj *ndsP4DededeMinionAttach(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    s32 charge = *ndsP4DededePassiveS32(fp, 2);
    Vec3f pos;
    Vec3f vel;

    ftSamusSpecialNGetChargeShotPosition(fp, &pos);
    vel.x = 0.0F;
    vel.y = 65.0F;
    vel.z = 0.0F;
    if (charge == 0)
    {
        return ndsP4DededeMinionMake(fighter_gobj, &sNdsP4DededeDeeDesc, &pos, &vel);
    }
    if (charge == 1)
    {
        return ndsP4DededeMinionMake(fighter_gobj, &sNdsP4DededeDooDesc, &pos, &vel);
    }
    return ndsP4DededeGordoMake(fighter_gobj, &pos, &vel);
}

/* WaddleDee.minion_begin_main_: to the aerial state. */
static void ndsP4DededeMinionBeginMain(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    ip->is_allow_pickup = FALSE;
    ip->attack_coll.attack_state = nGMAttackStateNew;
    itMapSetAir(ip);
    ip->multi = 0;
    ip->damage_coll.hitstatus = nGMHitStatusNormal;
    itMainSetStatus(item_gobj, sNdsP4DededeDeeStatusDescs, 1);
}

static void ndsP4DededeGordoBeginThrow(GObj *item_gobj);

/* WaddleDee.minion_throw_initial_ / Gordo.gordo_throw_initial_: 200 below
 * the hand, at ANGLE less the stick's push (x 1/128), MAX_SPEED; a minion
 * gets its hurtbox and may hit its owner. */
static void ndsP4DededeMinionThrowInitial(GObj *item_gobj, GObj *fighter_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    FTStruct *fp = ftGetStruct(fighter_gobj);
    DObj *dobj = DObjGetStruct(item_gobj);
    sb32 gordo = (ip->kind == NDS_P4_IT_KIND_GORDO) ? TRUE : FALSE;
    f32 speed = (gordo != FALSE) ? GORDO_MAX_SPEED : MINION_MAX_SPEED;
    f32 lr = (f32)fp->lr;
    f32 angle;

    dobj->translate.vec.f.y += -200.0F;
    dobj->translate.vec.f.z = 0.0F;
    if (gordo == FALSE)
    {
        ndsP4DededeMinionImage(item_gobj, 1);
    }
    ip->attack_coll.can_rehit_shield = TRUE;
    ip->lifetime = (gordo != FALSE) ? GORDO_DURATION : MINION_DURATION;
    NDS_P4_DEDEDE_ITW(ip, MINION_ITEM_VARS_1) = 4;
    angle = ((gordo != FALSE) ? GORDO_ANGLE : MINION_ANGLE) +
            (ndsP4DededeMinionBits(0xBC000000u) * ((f32)fp->input.pl.stick_range.x * lr));
    ip->lr = fp->lr;
    ip->physics.vel_air.x = (speed * cosf(angle)) * lr;
    ip->physics.vel_air.y = speed * sinf(angle);
    ip->physics.vel_air.z = 0.0F;
    if (gordo == FALSE)
    {
        ip->damage_coll.hitstatus = nGMHitStatusNormal;
    }
    ip->attack_coll.attack_state = nGMAttackStateNew;
    if (gordo == FALSE)
    {
        ip->is_damage_all = TRUE;
        ndsP4DededeMinionBeginMain(item_gobj);
    }
    else ndsP4DededeGordoBeginThrow(item_gobj);

    mpCommonRunItemCollisionDefault(item_gobj, fp->coll_data.p_translate, &fp->coll_data);
}

/* WaddleDee.minion_null_main_ / Gordo.gordo_null_main_ (held): the throw
 * flag throws it, 0xFFFF puts it away; else it follows his hand (joint 10,
 * 144 out; a Gordo 128 and flat in z) and a minion swaps its image every
 * fourth frame (the donor reads the current and next TEXID as one word). */
static sb32 ndsP4DededeMinionHeld(GObj *item_gobj, f32 reach)
{
    ITStruct *ip = itGetStruct(item_gobj);
    DObj *dobj = DObjGetStruct(item_gobj);
    sb32 gordo = (ip->kind == NDS_P4_IT_KIND_GORDO) ? TRUE : FALSE;
    s16 flag = (s16)ip->multi;

    if (flag != 0)
    {
        if (flag == -1)
        {
            ip->kind = DEDEDE_DSP_FAKE_ITEM_KIND;
            return TRUE;
        }
        ndsP4DededeMinionThrowInitial(item_gobj, ip->owner_gobj);
        ndsP4DededeMinionRemoveCheck(item_gobj);
        ip->lifetime++;
        return FALSE;
    }
    if ((gordo == FALSE) && ((ip->lifetime & 3) == 0) && (dobj->mobj != NULL))
    {
        MObj *mobj = dobj->mobj;

        mobj->texture_id_curr =
            ((mobj->texture_id_curr == 0) && (mobj->texture_id_next == 0)) ? 1 : 0;
    }
    dobj->translate.vec.f.x = reach;
    dobj->translate.vec.f.y = 0.0F;
    dobj->translate.vec.f.z = 0.0F;
    gmCollisionGetFighterPartsWorldPosition(ftGetStruct(ip->owner_gobj)->joints[10],
                                            &dobj->translate.vec.f);
    if (gordo != FALSE)
    {
        dobj->translate.vec.f.z = 0.0F;
    }
    dobj->rotate.vec.f.z = 0.0F;
    NDS_P4_DEDEDE_ITW(ip, MINION_ROTATION_DIR) = 0;
    ip->lifetime++;
    return FALSE;
}

static sb32 ndsP4DededeMinionNullMain(GObj *item_gobj)
{
    return ndsP4DededeMinionHeld(item_gobj, 144.0F);
}

static sb32 ndsP4DededeGordoNullMain(GObj *item_gobj)
{
    return ndsP4DededeMinionHeld(item_gobj, 128.0F);
}

/* The flying spin: base 0x3C900000 plus ROTATION times the speed, turning
 * the way it travels (kept while its x speed is zero). */
static f32 ndsP4DededeMinionSpin(ITStruct *ip, f32 rotation, f32 *speed_out)
{
    f32 *dir = &NDS_P4_DEDEDE_ITF(ip, MINION_ROTATION_DIR);
    f32 speed;

    if (ndsP4DededeMinionZeroBits(ip->physics.vel_air.x) == FALSE)
    {
        *dir = ip->physics.vel_air.x / ABSF(ip->physics.vel_air.x);
    }
    speed = sqrtf((ip->physics.vel_air.x * ip->physics.vel_air.x) +
                  (ip->physics.vel_air.y * ip->physics.vel_air.y));
    *speed_out = speed;
    return (ndsP4DededeMinionBits(0x3C900000u) + (rotation * speed)) * *dir;
}

/* Airborne: gravity to a top speed that eases down to MAX_SPEED by 1 a
 * frame; grounded: a slide (x 0.875) that stops, and stops hitting, under
 * 2. */
static void ndsP4DededeMinionFlightSpeed(ITStruct *ip, f32 gravity, f32 max_speed)
{
    f32 *cur = &NDS_P4_DEDEDE_ITF(ip, MINION_MAX_SPEED_CUR);

    if (ip->ga != nMPKineticsGround)
    {
        *cur -= 1.0F;
        if (*cur <= max_speed)
        {
            *cur = max_speed;
        }
        itMainApplyGravityClampTVel(ip, gravity, *cur);
    }
    else
    {
        ip->physics.vel_air.x *= ndsP4DededeMinionBits(0x3F700000u);
        if (ABSF(ip->physics.vel_air.x) < 2.0F)
        {
            ip->physics.vel_air.x = 0.0F;
            ip->attack_coll.attack_state = nGMAttackStateOff;
        }
    }
}

/* The hitbox refresh timer: at its end the attack records clear. */
static void ndsP4DededeMinionRefreshTick(ITStruct *ip)
{
    s32 *timer = &NDS_P4_DEDEDE_ITW(ip, MINION_HIT_REFRESH);

    if (*timer != 0)
    {
        *timer -= 1;
        if (*timer == 0)
        {
            ndsP4DededeMinionClearVictims(ip);
        }
    }
}

/* Its direction from its x speed, kept at zero speed. */
static void ndsP4DededeMinionFaceTravel(ITStruct *ip)
{
    if (ndsP4DededeMinionZeroBits(ip->physics.vel_air.x) == FALSE)
    {
        ip->lr = (ip->physics.vel_air.x < 0.0F) ? -1 : +1;
    }
}

static void ndsP4DededeMinionLandingInitial(GObj *item_gobj);

/* WaddleDee.waddle_dee_aerial_main_. */
static sb32 ndsP4DededeDeeAerialMain(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    f32 speed;
    f32 spin;
    sb32 ret = FALSE;

    ndsP4DededeMinionFaceTravel(ip);
    ndsP4DededeMinionFlightSpeed(ip, MINION_GRAVITY, MINION_MAX_SPEED);
    if (ip->lifetime == 0)
    {
        ndsP4DededeMinionFree(item_gobj);
        ret = TRUE;
        goto end;
    }
    ip->lifetime--;
    spin = ndsP4DededeMinionSpin(ip, MINION_ROTATION, &speed);
    if (ip->ga == nMPKineticsGround)
    {
        ndsP4DededeMinionLandingInitial(item_gobj);
        goto end;
    }
    DObjGetStruct(item_gobj)->rotate.vec.f.z -= spin;
    ndsP4DededeMinionRefreshTick(ip);
end:
    NDS_P4_DEDEDE_ITW(ip, MINION_HIT_FLAG) = 0;
    return ret;
}

static void ndsP4DededeMinionGroundedWalkInitial(GObj *item_gobj);

/* WaddleDee.waddle_dee_landing_main_: the slide, then after LAND_LAG frames
 * the walk. */
static sb32 ndsP4DededeDeeLandingMain(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    sb32 ret = FALSE;
    s16 timer;

    ndsP4DededeMinionFaceTravel(ip);
    ip->physics.vel_air.x *= ndsP4DededeMinionBits(0x3F700000u);
    if (ABSF(ip->physics.vel_air.x) < 2.0F)
    {
        ip->physics.vel_air.x = 0.0F;
        ip->attack_coll.attack_state = nGMAttackStateOff;
    }
    if (ip->lifetime == 0)
    {
        ndsP4DededeMinionFree(item_gobj);
        ret = TRUE;
        goto end;
    }
    ip->lifetime--;
    timer = (s16)(ip->multi + 1);
    ip->multi = (u16)timer;
    if (timer == MINION_LAND_LAG)
    {
        ndsP4DededeMinionGroundedWalkInitial(item_gobj);
    }
end:
    NDS_P4_DEDEDE_ITW(ip, MINION_HIT_FLAG) = 0;
    return ret;
}

/* WaddleDee.minion_detect_collision_: the item's all-surface test
 * (itMapProcAllCheckCollisionFlag) without the landing's edge snap, sound,
 * spin reset and collision end. */
static sb32 ndsP4DededeMinionDetectCollision(MPCollData *coll_data, GObj *item_gobj, u32 coll_flags)
{
    (void)item_gobj;

    if (mpProcessCheckTestLWallCollisionAdjNew(coll_data) != FALSE)
    {
        mpProcessRunLWallCollisionAdjNew(coll_data);
    }
    if (mpProcessCheckTestRWallCollisionAdjNew(coll_data) != FALSE)
    {
        mpProcessRunRWallCollisionAdjNew(coll_data);
    }
    if (mpProcessCheckTestCeilCollisionAdjNew(coll_data) != FALSE)
    {
        mpProcessRunCeilCollisionAdjNew(coll_data);

        if (coll_data->mask_stat & MAP_FLAG_CEIL)
        {
            mpProcessRunCeilEdgeAdjust(coll_data);
        }
    }
    if (mpProcessRunFloorCollisionAdjNewNULL(coll_data) != FALSE)
    {
        mpProcessSetLandingFloor(coll_data);
    }
    return (coll_data->mask_curr & coll_flags) ? TRUE : FALSE;
}

/* WaddleDee.waddle_dee_collision_: bounces off walls and ceilings at
 * BOUNCE; a floor lands it, hitting. */
static sb32 ndsP4DededeDeeCollision(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    sb32 hit = mpProcessUpdateMain(&ip->coll_data, ndsP4DededeMinionDetectCollision, item_gobj,
                                   0x0C21);

    itMapCheckCollideAllRebound(item_gobj, 0x0421, MINION_BOUNCE, NULL);
    if ((hit != FALSE) && (ip->coll_data.mask_stat & MAP_FLAG_FLOOR))
    {
        ndsP4DededeMinionLandingInitial(item_gobj);
        ip->attack_coll.attack_state = nGMAttackStateNew;
    }
    return FALSE;
}

/* WaddleDee.waddle_dee_resting_collision_: off a ledge, aerial again. */
static sb32 ndsP4DededeDeeRestingCollision(GObj *item_gobj)
{
    itMapCheckLRWallProcNoFloor(item_gobj, ndsP4DededeMinionBeginMain);
    return FALSE;
}

/* WaddleDee.minion_landing_initial_: the step sound, upright, the landing
 * image. */
static void ndsP4DededeMinionLandingInitial(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    func_800269C0_275C0(MINION_FGM_STEP);
    DObjGetStruct(item_gobj)->rotate.vec.f.z = 0.0F;
    ndsP4DededeMinionImage(item_gobj, 1);
    ip->times_landed = 0;
    ip->multi = 0;
    ip->physics.vel_air.y = 0.0F;
    ip->physics.vel_air.z = 0.0F;
    itMapSetGround(ip);
    ip->damage_coll.hitstatus = nGMHitStatusNormal;
    itMainSetStatus(item_gobj, sNdsP4DededeDeeStatusDescs, 2);
}

static void ndsP4DededeMinionWalkInitial(GObj *item_gobj);

/* WaddleDee.minion_grounded_walk_initial_: still, owned by him again. */
static void ndsP4DededeMinionGroundedWalkInitial(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    ip->physics.vel_air.x = 0.0F;
    ip->physics.vel_air.y = 0.0F;
    ip->physics.vel_air.z = 0.0F;
    ip->owner_gobj = (GObj *)(uintptr_t)NDS_P4_DEDEDE_ITW(ip, MINION_OWNER);
    ndsP4DededeMinionWalkInitial(item_gobj);
}

/* WaddleDee.minion_walk_intitial: the Bob-omb's walk set-up (its walk
 * MatAnim at the Bob-omb's offset; Remix's remove_bob_bomb_walk_fgm_ keeps
 * its fuse sound to the Bob-omb), keeping the owner it clears, hurtbox on,
 * hitbox off, then the walk at the minion's speed in its facing. */
static void ndsP4DededeMinionWalkInitial(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    GObj *owner = ip->owner_gobj;

    ip->damage_coll.hitstatus = nGMHitStatusNormal;
    ip->physics.vel_air.x = (f32)ip->lr * MINION_GROUND_SPEED;
    ip->event_id = 0;
    ip->multi = 0;
    ip->attack_coll.throw_mul = 1.0F;
    itBombHeiWalkInitVars(item_gobj);
    ip->attack_coll.attack_state = nGMAttackStateOff;
    ip->owner_gobj = owner;
    itMainSetStatus(item_gobj, sNdsP4DededeDeeStatusDescs, 3);
    ndsP4DededeGroundMoveInitial(item_gobj, (ip->lr == 1) ? TRUE : FALSE);
}

/* WaddleDee.waddle_ground_to_air_transition: off a ledge while walking. */
static void ndsP4DededeWaddleGroundToAir(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    ndsP4DededeMinionImageStill(item_gobj, 0);
    ip->ga = nMPKineticsAir;
    ip->attack_coll.attack_state = nGMAttackStateNew;
    ip->damage_coll.hitstatus = nGMHitStatusNormal;
    itMainSetStatus(item_gobj, sNdsP4DededeDeeStatusDescs, 1);
}

/* WaddleDee.waddle_ground_collision: the Bob-omb's walk map
 * (itBombHeiWalkProcMap) with its own fall; the walls turn it at the
 * Bob-omb's speed. */
static sb32 ndsP4DededeWaddleGroundCollision(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    itMapCheckLRWallProcNoFloor(item_gobj, ndsP4DededeWaddleGroundToAir);
    if (ip->coll_data.mask_curr & MAP_FLAG_LWALL)
    {
        itBombHeiCommonSetWalkLR(item_gobj, 0);
    }
    if (ip->coll_data.mask_curr & MAP_FLAG_RWALL)
    {
        itBombHeiCommonSetWalkLR(item_gobj, 1);
    }
    return FALSE;
}

/* WaddleDee.check_target_: a target ahead within range (a cone: the height
 * range grows by half the distance ahead), nearer ahead than the one so
 * far. */
static sb32 ndsP4DededeMinionCheckTarget(ITStruct *ip, GObj *target, GObj *best, f32 *best_dx,
                                         sb32 edge)
{
    Vec3f *self = &DObjGetStruct(ip->item_gobj)->translate.vec.f;
    Vec3f *other = &DObjGetStruct(target)->translate.vec.f;
    sb32 doo = (ip->kind == NDS_P4_IT_KIND_WADDLE_DOO) ? TRUE : FALSE;
    f32 dx = (other->x - self->x) * (f32)ip->lr;
    f32 range_x = (edge != FALSE) ? 1280.0F : (doo != FALSE) ? 1096.0F : 600.0F;
    f32 range_y = (edge != FALSE) ? 1280.0F : (doo != FALSE) ? 600.0F : 700.0F;

    if (!(dx <= range_x) || !(0.0F <= dx))
    {
        return FALSE;
    }
    if ((best != NULL) && !(dx <= *best_dx))
    {
        return FALSE;
    }
    if (!(ABSF(other->y - self->y) <= (range_y + (0.5F * dx))))
    {
        return FALSE;
    }
    *best_dx = dx;
    return TRUE;
}

/* WaddleDee.check_for_targets_: fighters first (not its owner, not one of
 * the owner's team without team attack, not KO'd), then items with a
 * hurtbox its owner does not own. The donor reads the owner's team as its
 * struct's byte 0xC, the high byte of an item's or weapon's kind (0) when
 * one of those last hit it; it skips the fighters with no owner. */
static GObj *ndsP4DededeMinionFindTarget(ITStruct *ip, sb32 edge)
{
    GObj *owner = ip->owner_gobj;
    GObj *best = NULL;
    f32 best_dx = 0.0F;
    GObj *gobj;

    if (owner != NULL)
    {
        u8 owner_team = (owner->id == nGCCommonKindFighter) ? ftGetStruct(owner)->team : 0;

        for (gobj = gGCCommonLinks[nGCCommonLinkIDFighter]; gobj != NULL; gobj = gobj->link_next)
        {
            FTStruct *tfp;

            if (gobj == owner)
            {
                continue;
            }
            tfp = ftGetStruct(gobj);
            if ((gSCManagerBattleState->is_team_battle != FALSE) &&
                (gSCManagerBattleState->is_team_attack == FALSE) && (tfp->team == owner_team))
            {
                continue;
            }
            if ((u32)tfp->status_id < 7u)
            {
                continue;
            }
            if (ndsP4DededeMinionCheckTarget(ip, gobj, best, &best_dx, edge) != FALSE)
            {
                best = gobj;
            }
        }
        if (best != NULL)
        {
            return best;
        }
    }
    for (gobj = gGCCommonLinks[nGCCommonLinkIDItem]; gobj != NULL; gobj = gobj->link_next)
    {
        ITStruct *tip = itGetStruct(gobj);

        if ((tip == ip) || ((tip->damage_coll.hitstatus & 1) == 0) || (tip->owner_gobj == owner))
        {
            continue;
        }
        if (ndsP4DededeMinionCheckTarget(ip, gobj, best, &best_dx, edge) != FALSE)
        {
            best = gobj;
        }
    }
    return best;
}

/* WaddleDee.waddle_edge_check_: at a ledge it turns unless a target is
 * ahead. */
static void ndsP4DededeWaddleEdgeCheck(GObj *item_gobj, sb32 right)
{
    if (ndsP4DededeMinionFindTarget(itGetStruct(item_gobj), TRUE) == NULL)
    {
        ndsP4DededeGroundMoveInitial(item_gobj, right);
    }
}

static void ndsP4DededeDeeAttackInitial(GObj *item_gobj);
static void ndsP4DededeDooAttackInitial(GObj *item_gobj);

/* WaddleDee.minion_walk_main (on the Bob-omb's walk, 0x801776A0): the
 * ledge turn, a target check every 16 frames once it has walked its delay,
 * its duration. */
static sb32 ndsP4DededeMinionWalkMain(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    ITAttributes *attr = ip->attr;
    DObj *dobj = DObjGetStruct(item_gobj);
    sb32 ret = FALSE;
    u16 next;
    Vec3f edge;

    if (mpCollisionCheckExistLineID(ip->coll_data.floor_line_id) != FALSE)
    {
        if (ip->lr == -1)
        {
            mpCollisionGetFloorEdgeL(ip->coll_data.floor_line_id, &edge);

            if ((dobj->translate.vec.f.x - (f32)attr->map_coll_width) <= edge.x)
            {
                ndsP4DededeWaddleEdgeCheck(item_gobj, TRUE);
            }
        }
        else
        {
            mpCollisionGetFloorEdgeR(ip->coll_data.floor_line_id, &edge);

            if (edge.x <= (dobj->translate.vec.f.x + (f32)attr->map_coll_width))
            {
                ndsP4DededeWaddleEdgeCheck(item_gobj, FALSE);
            }
        }
    }
    next = (u16)(ip->multi + 1);
    if ((ip->multi & 0xF) == 0)
    {
        u32 delay = (ip->kind == NDS_P4_IT_KIND_WADDLE_DEE) ?
            MINION_WADDLE_DEE_WALK_ATTACK_DELAY : MINION_WADDLE_DOO_WALK_ATTACK_DELAY;

        if (((u32)next >= delay) && (ndsP4DededeMinionFindTarget(ip, FALSE) != NULL))
        {
            if (ip->kind == NDS_P4_IT_KIND_WADDLE_DEE)
            {
                ndsP4DededeDeeAttackInitial(item_gobj);
            }
            else ndsP4DededeDooAttackInitial(item_gobj);

            ip->physics.vel_air.z = 0.0F;
            ip->physics.vel_air.y = 0.0F;
            ip->physics.vel_air.x = 0.0F;
            next = (u16)(ip->multi + 1);
        }
    }
    if (ip->lifetime == 0)
    {
        ndsP4DededeMinionFree(item_gobj);
        ret = TRUE;
    }
    else ip->lifetime--;

    ip->multi = next;
    return ret;
}

/* WaddleDee.waddle_dee_attack_initial: the jump squat. */
static void ndsP4DededeDeeAttackInitial(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    DObjGetStruct(item_gobj)->rotate.vec.f.z = 0.0F;
    ndsP4DededeMinionImageStill(item_gobj, 5);
    ip->attack_coll.attack_state = nGMAttackStateOff;
    ip->times_landed = 0;
    ip->multi = 0;
    itMapSetGround(ip);
    ip->damage_coll.hitstatus = nGMHitStatusNormal;
    itMainSetStatus(item_gobj, sNdsP4DededeDeeStatusDescs, 4);
}

/* WaddleDee.waddle_dee_jumpsquat_main (states 4 and 5): LAND_LAG frames of
 * squat, the jump (8 ahead, 40 up, hitting), then back to the landing. */
static sb32 ndsP4DededeDeeJumpsquatMain(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    sb32 ret = FALSE;

    if (ip->ga != nMPKineticsGround)
    {
        f32 *cur = &NDS_P4_DEDEDE_ITF(ip, MINION_MAX_SPEED_CUR);

        *cur -= 1.0F;
        if (*cur <= MINION_MAX_SPEED)
        {
            *cur = MINION_MAX_SPEED;
        }
        itMainApplyGravityClampTVel(ip, MINION_GRAVITY, *cur);
    }
    else
    {
        s16 timer = (s16)(ip->multi + 1);

        ip->multi = (u16)timer;
        if (timer > MINION_LAND_LAG)
        {
            ndsP4DededeMinionLandingInitial(item_gobj);
            goto end;
        }
        if (timer == MINION_LAND_LAG)
        {
            func_800269C0_275C0(MINION_FGM_JUMP);
            ndsP4DededeMinionImage(item_gobj, 6);
            ip->attack_coll.attack_state = nGMAttackStateNew;
            ip->physics.vel_air.x = MINION_H_JUMP_SPEED * (f32)ip->lr;
            ip->physics.vel_air.y = MINION_V_JUMP_SPEED;
            ip->damage_coll.hitstatus = nGMHitStatusNormal;
            ip->ga = nMPKineticsAir;
            itMainSetStatus(item_gobj, sNdsP4DededeDeeStatusDescs, 5);
            goto end;
        }
    }
    if (ip->lifetime == 0)
    {
        ndsP4DededeMinionFree(item_gobj);
        ret = TRUE;
        goto end;
    }
    ip->lifetime--;
    ndsP4DededeMinionRefreshTick(ip);
end:
    NDS_P4_DEDEDE_ITW(ip, MINION_HIT_FLAG) = 0;
    return ret;
}

/* The duration a contact costs: 1.5 x its speed (at least 12 left). */
static void ndsP4DededeMinionContactCost(ITStruct *ip, f32 speed)
{
    s32 whole = ndsP4DededeMinionRoundEven(speed);
    s32 left = ip->lifetime - (whole + (s32)((u32)whole >> 1));

    ip->lifetime = (left < 12) ? 12 : left;
}

/* WaddleDee.minion_hurtbox_collision_ (its hitbox met a hurtbox or a
 * shield): aerial with its hitbox off, popped up at half its speed plus 5
 * unless it traded hits. */
static sb32 ndsP4DededeMinionHurtboxCollision(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    f32 speed;

    ndsP4DededeMinionBeginMain(item_gobj);
    ip->attack_coll.attack_state = nGMAttackStateOff;
    speed = sqrtf((ip->physics.vel_air.x * ip->physics.vel_air.x) +
                  (ip->physics.vel_air.y * ip->physics.vel_air.y));
    if (NDS_P4_DEDEDE_ITW(ip, MINION_HIT_FLAG) == 0)
    {
        ip->physics.vel_air.x = 0.0F;
        ip->physics.vel_air.y = (speed * 0.5F) + 5.0F;
    }
    NDS_P4_DEDEDE_ITW(ip, MINION_HIT_FLAG) = 0;
    NDS_P4_DEDEDE_ITW(ip, MINION_HIT_REFRESH) = 0;
    ndsP4DededeMinionContactCost(ip, speed);
    return FALSE;
}

/* A hit's launch: (damage x 4) + 10 along the hit's angle, away from the
 * attacker, who now owns it. The donor's angle call takes a stale register
 * for the ground flag (its state table's address), never the air value. */
static void ndsP4DededeMinionLaunch(ITStruct *ip)
{
    GObj *attacker = ip->damage_gobj;
    f32 knockback = ((f32)ip->damage_queue * 4.0F) + 10.0F;
    f32 angle;

    ip->owner_gobj = attacker;
    ip->player = GMCOMMON_PLAYERS_MAX;
    if ((attacker != NULL) && (attacker->id == nGCCommonKindFighter))
    {
        ip->player = ftGetStruct(attacker)->player;
    }
    NDS_P4_DEDEDE_ITF(ip, MINION_MAX_SPEED_CUR) = knockback;
    angle = ftCommonDamageGetKnockbackAngle(ip->damage_angle, nMPKineticsGround, knockback);
    ip->physics.vel_air.x = (f32)(-ip->damage_lr) * (knockback * cosf(angle));
    ip->physics.vel_air.y = knockback * sinf(angle);
    NDS_P4_DEDEDE_ITW(ip, MINION_HIT_REFRESH) = 16;
    NDS_P4_DEDEDE_ITW(ip, MINION_HIT_FLAG) = TRUE;
}

/* WaddleDee.minion_hitbox_collision_ (an attack hit it): launched with the
 * thrown image, its duration shortened by 8 x the damage (at least 20). */
static sb32 ndsP4DededeMinionHitboxCollision(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    s32 left;

    ndsP4DededeMinionBeginMain(item_gobj);
    ndsP4DededeMinionImageStill(item_gobj, 0);
    ip->ga = nMPKineticsAir;
    ndsP4DededeMinionLaunch(ip);
    left = ip->lifetime - (ip->damage_queue * 8);
    ip->lifetime = (left < 20) ? 20 : left;
    return FALSE;
}

/* ---- The Waddle Doo's beam ---- */

static sb32 ndsP4DededeBeamMain(GObj *weapon_gobj);

/* WaddleDoo.beam_info_array: the Star Rod star's attributes, its own kind
 * and main, no collision routines (the stars pass through what they hit). */
static WPDesc sNdsP4DededeBeamDesc = {
    0x00, NDS_P4_WP_KIND_WADDLEDOO_BEAM, &gITManagerCommonData,
    (intptr_t)&llITCommonDataStarRodWeaponAttributes,
    { nGCMatrixKindTraRotRpyRSca, nGCMatrixKindNull, 0 },
    ndsP4DededeBeamMain, NULL, NULL, NULL, NULL, NULL, NULL, NULL
};

/* A star's variables at the donor's offsets from the weapon variables
 * (0x29C the Star Rod star's own lifetime); the donor keeps the start-up
 * timer in proc_dead's low half (0x29A), here beside them. */
typedef struct NDSP4DededeBeamVars
{
    s32 lifetime;       /* 0x29C: the Star Rod star's lifetime */
    GObj *parent;       /* 0x2A0: the Waddle Doo, or the star before */
    f32 angle;          /* 0x2A4 */
    s16 shown;          /* 0x2A8: the last frame's draw flag */
    s16 start_up;
} NDSP4DededeBeamVars;

_Static_assert(sizeof(NDSP4DededeBeamVars) <= sizeof(((WPStruct *)0)->weapon_vars),
               "the beam's variables outgrow the weapon variables");

static NDSP4DededeBeamVars *ndsP4DededeBeamVars(WPStruct *wp)
{
    return (NDSP4DededeBeamVars *)(void *)&wp->weapon_vars;
}

/* WaddleDoo.star_stage_setting (create_beam_star enters the Star Rod's
 * smash star maker with the beam's description): one star at the
 * coordinates, then the beam's own state. Each call starts its stack copy
 * of the duration, start-up and angle from the attributes, so every star
 * gets 10, 10 and the initial angle. The x offset ignores the angle's
 * cosine (the donor scales the facing by the distance alone); its clang
 * write lands on the hitbox position, rewritten after, so the attributes'
 * clang stays. */
static GObj *ndsP4DededeBeamStar(GObj *doo_gobj, Vec3f *coords, GObj *parent)
{
    ITStruct *ip = itGetStruct(doo_gobj);
    GObj *dedede = (GObj *)(uintptr_t)NDS_P4_DEDEDE_ITW(ip, MINION_OWNER);
    GObj *weapon_gobj = wpManagerMakeWeapon(ip->owner_gobj, &sNdsP4DededeBeamDesc, coords,
                                            (WEAPON_FLAG_COLLPROJECT | WEAPON_FLAG_PARENT_FIGHTER));
    Vec3f at = *coords;
    WPStruct *wp;
    NDSP4DededeBeamVars *v;
    DObj *dobj;

    if (weapon_gobj == NULL)
    {
        return NULL;
    }
    wp = wpGetStruct(weapon_gobj);
    dobj = DObjGetStruct(weapon_gobj);
    wp->physics.vel_air.x = BEAM_STAR_MAKER_VEL_X * wp->lr;
    gcAddXObjForDObjFixed(dobj, 0x2E, 0);
    dobj->translate.vec.f = at;
    dobj->translate.vec.f.z = 0.0F;

    weapon_gobj->flags = GOBJ_FLAG_HIDDEN;
    wp->physics.vel_air.x = 0.0F;
    wp->handicap = ftGetStruct(dedede)->handicap;
    wp->owner_gobj = ip->owner_gobj;
    v = ndsP4DededeBeamVars(wp);
    v->parent = parent;
    v->shown = 1;
    v->lifetime = MINION_BEAM_DURATION;
    v->start_up = BEAM_START_UP;
    v->angle = ndsP4DededeMinionBits(MINION_BEAM_INITIAL_ANGLE);
    wp->lifetime = 0xFF;
    wp->attack_coll.damage = BEAM_STAR_DAMAGE;
    wp->attack_coll.element = BEAM_STAR_ELEMENT;
    wp->attack_coll.fgm_id = BEAM_STAR_FGM;
    wp->attack_coll.attack_state = nGMAttackStateOff;
    wp->lr = ip->lr;
    dobj->translate.vec.f.x = ((f32)wp->lr * MINION_BEAM_DISTANCE) + at.x;
    wp->attack_coll.attack_pos[0].pos_curr.x = dobj->translate.vec.f.x;
    dobj->translate.vec.f.y = at.y + (sinf(v->angle) * MINION_BEAM_DISTANCE);
    wp->attack_coll.attack_pos[0].pos_curr.y = dobj->translate.vec.f.y;
    return weapon_gobj;
}

/* The donor's test that a star's parent is still there: its struct has a
 * kind, and it has a DObj (an ejected GObj's is cleared). */
static sb32 ndsP4DededeBeamParentAlive(GObj *parent)
{
    const u8 *user = (const u8 *)parent->user_data.p;

    return ((*(const s32 *)(const void *)(user + 0xC) != 0) &&
            (DObjGetStruct(parent) != NULL)) ? TRUE : FALSE;
}

/* WaddleDoo.beam_main: the start-up (a sparkle on its first frame), then
 * the Star Rod star's update; shown and hitting two frames in four, and on
 * even lifetimes it turns BEAM_ROTATION about its parent, the distance
 * ahead of it, and may hit again. */
static sb32 ndsP4DededeBeamMain(GObj *weapon_gobj)
{
    WPStruct *wp = wpGetStruct(weapon_gobj);
    NDSP4DededeBeamVars *v = ndsP4DededeBeamVars(wp);
    DObj *dobj = DObjGetStruct(weapon_gobj);
    GObj *parent;

    if (v->start_up != 0)
    {
        if (v->start_up == BEAM_START_UP)
        {
            efManagerSparkleWhiteScaleMakeEffect(&dobj->translate.vec.f, 1.0F);
        }
        v->start_up--;
        return FALSE;
    }
    if (itStarRodWeaponStarProcUpdate(weapon_gobj) != FALSE)
    {
        return TRUE;
    }
    if ((s32)weapon_gobj->flags != (s32)v->shown)
    {
        v->shown = (s16)weapon_gobj->flags;
    }
    else
    {
        weapon_gobj->flags = (weapon_gobj->flags == GOBJ_FLAG_HIDDEN) ? 0 : GOBJ_FLAG_HIDDEN;
        wp->attack_coll.attack_state = (wp->attack_coll.attack_state != nGMAttackStateInterpolate) ?
            nGMAttackStateInterpolate : nGMAttackStateOff;
    }
    if ((v->lifetime & 1) != 0)
    {
        return FALSE;
    }
    v->angle -= ndsP4DededeMinionBits(MINION_BEAM_ROTATION);
    parent = v->parent;
    if ((parent == NULL) || (parent->user_data.p == NULL))
    {
        return FALSE;
    }
    if (ndsP4DededeBeamParentAlive(parent) == FALSE)
    {
        v->parent = NULL;
        return FALSE;
    }
    {
        Vec3f *to = &DObjGetStruct(parent)->translate.vec.f;

        dobj->translate.vec.f.x = ((f32)wp->lr * MINION_BEAM_DISTANCE) + to->x;
        dobj->translate.vec.f.y = to->y + (sinf(v->angle) * MINION_BEAM_DISTANCE);
    }
    wp->attack_coll.attack_records[0].victim_gobj = NULL;
    return FALSE;
}

/* WaddleDoo.waddle_doo_attack_initial: the squat image, the charge sound
 * and BEAM_CHARGE_TIME frames of flashing. */
static void ndsP4DededeDooAttackInitial(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    DObjGetStruct(item_gobj)->rotate.vec.f.z = 0.0F;
    ndsP4DededeMinionImageStill(item_gobj, 5);
    ip->attack_coll.attack_state = nGMAttackStateOff;
    ip->times_landed = 0;
    ip->multi = 0;
    itMapSetGround(ip);
    ip->damage_coll.hitstatus = nGMHitStatusNormal;
    func_800269C0_275C0(MINION_FGM_DOO_CHARGE);
    itMainCheckSetColAnimID(item_gobj, 7, MINION_BEAM_CHARGE_TIME);
    itMainSetStatus(item_gobj, sNdsP4DededeDooStatusDescs, 4);
}

/* WaddleDoo.waddle_doo_attack_main: the charge; a star a frame for
 * BEAM_STAR_COUNT frames (the first at the Doo, without its hurtbox, each
 * next off the last, which the item's first variable word keeps); 40
 * frames of beam; the walk. */
static sb32 ndsP4DededeDooAttackMain(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    GObj **last = (GObj **)(void *)&ip->item_vars;
    s16 timer = (s16)(ip->multi + 1);
    s32 first = MINION_BEAM_CHARGE_TIME - BEAM_STAR_COUNT + 1;
    sb32 ret = FALSE;

    ip->multi = (u16)timer;
    if (timer > MINION_BEAM_CHARGE_TIME)
    {
        if (timer >= (MINION_BEAM_CHARGE_TIME + 40))
        {
            ndsP4DededeMinionWalkInitial(item_gobj);
            goto end;
        }
    }
    else if (timer >= first)
    {
        Vec3f *coords;
        GObj *parent;

        if (timer == first)
        {
            ip->damage_coll.hitstatus = nGMHitStatusNone;
            func_800269C0_275C0(MINION_FGM_DOO_SHOOT);
            coords = &DObjGetStruct(item_gobj)->translate.vec.f;
            ndsP4DededeMinionImage(item_gobj, 6);
            parent = item_gobj;
        }
        else
        {
            if (*last == NULL)
            {
                goto duration;
            }
            coords = &DObjGetStruct(*last)->translate.vec.f;
            parent = *last;
        }
        *last = ndsP4DededeBeamStar(item_gobj, coords, parent);
        itMainCheckSetColAnimID(item_gobj, 0x2A, 0x14);
    }
duration:
    if (ip->lifetime == 0)
    {
        ndsP4DededeMinionFree(item_gobj);
        ret = TRUE;
        goto end;
    }
    ip->lifetime--;
end:
    NDS_P4_DEDEDE_ITW(ip, MINION_HIT_FLAG) = 0;
    return ret;
}

/* ---- Gordo ---- */

/* Gordo.gordo_begin_throw_: aerial and hitting, its hurtbox off. */
static void ndsP4DededeGordoBeginThrow(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    ip->attack_coll.attack_state = nGMAttackStateNew;
    itMapSetAir(ip);
    ip->multi = 0;
    ip->damage_coll.hitstatus = nGMHitStatusNormal;
    itMainSetStatus(item_gobj, sNdsP4DededeGordoStatusDescs, 1);
    ip->damage_coll.hitstatus = nGMHitStatusNone;
}

/* Gordo.gordo_aerial_main_: grounded it is gone (without leaving his
 * slots, as the donor); its records clear at the refresh timer's end and
 * at 35 speed or more. */
static sb32 ndsP4DededeGordoAerialMain(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    s32 *timer = &NDS_P4_DEDEDE_ITW(ip, MINION_HIT_REFRESH);
    f32 speed;
    f32 spin;
    sb32 ret = FALSE;

    ndsP4DededeMinionFlightSpeed(ip, GORDO_GRAVITY, GORDO_MAX_SPEED);
    if (ip->lifetime == 0)
    {
        ndsP4DededeMinionFree(item_gobj);
        ret = TRUE;
        goto end;
    }
    ip->lifetime--;
    spin = ndsP4DededeMinionSpin(ip, GORDO_ROTATION, &speed);
    if (ip->ga == nMPKineticsGround)
    {
        ret = TRUE;
        goto end;
    }
    DObjGetStruct(item_gobj)->rotate.vec.f.z -= spin;
    if (*timer != 0)
    {
        *timer -= 1;
        if (*timer != 0)
        {
            goto end;
        }
        ndsP4DededeMinionClearVictims(ip);
    }
    if (35.0F <= speed)
    {
        ndsP4DededeMinionClearVictims(ip);
    }
end:
    NDS_P4_DEDEDE_ITW(ip, MINION_HIT_FLAG) = 0;
    return ret;
}

/* Gordo.stick_to_surface: the Motion Sensor Bomb's surface stick (a 30
 * box) keeping its spin, then the stuck damage and sound. The donor's
 * rumble for its player has no DS counterpart. */
static void ndsP4DededeGordoStick(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    DObj *dobj = DObjGetStruct(item_gobj);
    f32 rotate_z;

    ip->coll_data.map_coll.top = 30.0F;
    ip->coll_data.map_coll.width = 30.0F;
    ip->physics.vel_air.z = 0.0F;
    ip->physics.vel_air.y = 0.0F;
    ip->physics.vel_air.x = 0.0F;
    ip->coll_data.map_coll.center = 0.0F;
    ip->coll_data.map_coll.bottom = -30.0F;
    rotate_z = dobj->rotate.vec.f.z;
    itMSBombAttachedUpdateSurface(item_gobj);
    dobj->rotate.vec.f.z = rotate_z;
    ip->is_attach_surface = TRUE;
    ip->attack_coll.damage = GORDO_SURFACE_STICK_DAMAGE;
    func_800269C0_275C0(GORDO_SURFACE_STICK_FGM);
}

/* Gordo.begin_resting_: stuck. */
static void ndsP4DededeGordoBeginResting(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    ip->damage_coll.hitstatus = nGMHitStatusNone;
    ip->times_landed = 0;
    ip->physics.vel_air.y = 0.0F;
    ip->physics.vel_air.z = 0.0F;
    ndsP4DededeGordoStick(item_gobj);
    ip->damage_coll.hitstatus = nGMHitStatusNormal;
    itMainSetStatus(item_gobj, sNdsP4DededeGordoStatusDescs, 2);
}

/* Gordo.gordo_collision_: the item's full surface test (its spin put back
 * after a touch), a bounce at BOUNCE; a wall or ceiling, or a floor met
 * under 32 down, sticks it. */
static sb32 ndsP4DededeGordoCollision(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    DObj *dobj = DObjGetStruct(item_gobj);
    f32 rotate_z = dobj->rotate.vec.f.z;
    sb32 hit = itMapTestAllCollisionFlag(item_gobj, 0x0C21);

    if (hit != FALSE)
    {
        dobj->rotate.vec.f.z = rotate_z;
    }
    itMapCheckCollideAllRebound(item_gobj, 0x0C21, GORDO_BOUNCE, NULL);
    if ((hit != FALSE) && (!(ip->coll_data.mask_stat & MAP_FLAG_FLOOR) ||
                           (ABSF(ip->physics.vel_air.y) < 32.0F)))
    {
        ndsP4DededeGordoBeginResting(item_gobj);
    }
    return FALSE;
}

/* Gordo.gordo_resting_main_: still until its duration ends. */
static sb32 ndsP4DededeGordoRestingMain(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    sb32 ret = FALSE;

    ip->physics.vel_air.x = 0.0F;
    if (ip->lifetime == 0)
    {
        ndsP4DededeMinionFree(item_gobj);
        ret = TRUE;
        goto end;
    }
    ip->lifetime--;
    ndsP4DededeMinionRefreshTick(ip);
end:
    NDS_P4_DEDEDE_ITW(ip, MINION_HIT_FLAG) = 0;
    return ret;
}

/* Gordo.resting_collision_: none (the donor's call is commented out). */
static sb32 ndsP4DededeGordoRestingCollision(GObj *item_gobj)
{
    (void)item_gobj;
    return FALSE;
}

/* Gordo.gordo_hurtbox_collision_: the minions' pop-up, refreshing in 16. */
static sb32 ndsP4DededeGordoHurtboxCollision(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    f32 speed = sqrtf((ip->physics.vel_air.x * ip->physics.vel_air.x) +
                      (ip->physics.vel_air.y * ip->physics.vel_air.y));

    if (NDS_P4_DEDEDE_ITW(ip, MINION_HIT_FLAG) == 0)
    {
        ip->physics.vel_air.x = 0.0F;
        ip->physics.vel_air.y = (speed * 0.5F) + 5.0F;
    }
    NDS_P4_DEDEDE_ITW(ip, MINION_HIT_FLAG) = 0;
    NDS_P4_DEDEDE_ITW(ip, MINION_HIT_REFRESH) = 16;
    ndsP4DededeMinionContactCost(ip, speed);
    return FALSE;
}

/* Gordo.resting_hurtbox_collision_. */
static sb32 ndsP4DededeGordoRestingHurtboxCollision(GObj *item_gobj)
{
    NDS_P4_DEDEDE_ITW(itGetStruct(item_gobj), MINION_HIT_REFRESH) = 16;
    return FALSE;
}

/* Gordo.gordo_hitbox_collision_: launched like a minion, facing away from
 * the hit, its duration shortened by 2 x the damage (at least 20). */
static sb32 ndsP4DededeGordoHitboxCollision(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    s32 left;

    ndsP4DededeGordoBeginThrow(item_gobj);
    ndsP4DededeMinionLaunch(ip);
    ip->lr = -ip->damage_lr;
    left = ip->lifetime - (ip->damage_queue * 2);
    ip->lifetime = (left < 20) ? 20 : left;
    return FALSE;
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
 * in his hand (held in status word 2; NULL when the item pool is full). */
static void ndsP4DededeDSPAttachMinion(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    fp->proc_damage = ndsP4DededeDSPOnDamage;
    *ndsP4DededeMinion(fp) = ndsP4DededeMinionAttach(fighter_gobj);
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

/* The held minion's throw flag (the item's `multi`, 0x33E): its held
 * routine throws it next frame. */
static void ndsP4DededeDSPThrowMinion(GObj *minion_gobj)
{
    itGetStruct(minion_gobj)->multi = 1;
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

/* Dedede.charge_gfx_routine_ (gfx_routine_end): the full charge's colour
 * comes back when another colour animation ends. */
static void ndsP4DededeColAnimEnd(GObj *fighter_gobj)
{
    if (*ndsP4DededePassiveS32(ftGetStruct(fighter_gobj), 2) == DEDEDE_DSP_CHARGE_MAX)
    {
        (void)ftParamCheckSetFighterColAnimID(fighter_gobj, DEDEDE_DSP_COLANIM_CHARGE, 0);
    }
}

/* Dedede.asm entry_script 0x8013DCF4 is Kirby's case of
 * ftCommonAppearSetStatus: the warp star on Kirby's special file 2, which
 * his Main externs, so the file is resident and VS's file setup
 * (ftManagerSetupFilesPlayablesAll) names it for Kirby's maker. */
GObj *efManagerKirbyEntryStarMakeEffect(Vec3f *pos, s32 lr);

static void ndsP4DededeEntryCase(FTStruct *fp)
{
    efManagerKirbyEntryStarMakeEffect(&fp->entry_pos, fp->status_vars.common.entry.lr);
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
    .entry_case = ndsP4DededeEntryCase,
    .colanim_end = ndsP4DededeColAnimEnd,
};

#endif /* NDS_P4_DEDEDE */
