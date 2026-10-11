/*
 * P4 Crash: native ports of the donor's special routines and of the patches
 * Remix makes on his character id. Source: JSsixtyfour/smashremix 5e04fe7,
 * src/Crash/CrashSpecial.asm, Crash.asm and Hitbox.asm, read as assembled
 * (scripts/p4/mipsdis.py): the OS.copy_segment blocks are the original
 * game's code. Crash runs Mario's status code (fp->fkind == nFTKindMario).
 *
 * Player-struct offsets in the donor map to BattleShip fields as:
 *   0x0F4 floor flags                      coll_data.floor_flags
 *   0x0EC/0x144 floor line, ignored line   coll_data.floor_line_id,
 *                                          coll_data.ignore_line_id
 *   0x17C/0x180/0x184 temp variables 1-3   motion_vars.flags.flag0/1/2
 *   0x18D & 0x07                           clears is_absorb, absorb_lr,
 *                                          is_goto_attack100 and is_fastfall
 *   0x1BC buttons held                     input.pl.button_hold
 *   0x1C2 stick x                          input.pl.stick_range.x
 *   0x9F4 shield routine                   proc_shield
 *   0xB18-0xB24                            status_vars, words 0-3: the spin's
 *                                          saved x, y and ground velocities
 *                                          and its "hitlag over"; the dig's
 *                                          timer, effect and move offset
 *   attributes + 0x9C/0xA0                 map_coll.top/center
 *
 * The spin and dig effects (create_spin_gfx_, create_dig_gfx_) and what the
 * donor runs only while the dig effect exists (dig_graphic_update_,
 * dig_update_'s dust, sound and rumble, and the dive landing's two sounds)
 * are below ("The spin and dig effects"). The Pokemon Stadium announcer
 * lines are not in the port.
 */
#include <nds/nds_p4.h>

#if NDS_P4_CRASH

#include <ef/effect.h>
#include <it/item.h>
#include <macros.h>
#include <sys/audio.h>
#include <sys/obj.h>
#include <sys/objdef.h>
#include <sys/objman.h>

s32 ftCommonKneeBendGetInputTypeCommon(FTStruct *fp);
void ftCaptainSpecialHiProcInterrupt(GObj *fighter_gobj);

/* Crash.Action (his action array). */
#define CRASH_STATUS_NSPG 0xDF
#define CRASH_STATUS_NSPA 0xE0
#define CRASH_STATUS_NSPG_BLOCKED 0xE1
#define CRASH_STATUS_NSPA_BLOCKED 0xE2
#define CRASH_STATUS_DSP_BEGIN 0xE5
#define CRASH_STATUS_DSP_WAIT 0xE6
#define CRASH_STATUS_DSP_TURN 0xE7
#define CRASH_STATUS_DSP_END 0xE8
#define CRASH_STATUS_DSP_DIVE 0xE9
#define CRASH_STATUS_DSP_AIR_DIVE 0xEA
#define CRASH_STATUS_USPG 0xEB
#define CRASH_STATUS_USPA 0xEC
#define CRASH_STATUS_USP_LANDING 0xED

/* The transitions' preserve word: hit, colanim, effect and the loop sound
 * ("continue: 3C FGM, attached gfx, gfx routines, hitboxes"). */
#define CRASH_PRESERVE_SPIN 0x0807u
/* The dig's: hit, colanim and effect. */
#define CRASH_PRESERVE_DIG 0x0007u

/* CrashNSP (the spin). */
#define CRASH_NSP_MIN_SPEED 2.0F
#define CRASH_NSP_MAX_SPEED 78.0F
#define CRASH_NSP_END_ACCELERATION 4.5F
#define CRASH_NSP_ACCELERATION 2.0F
#define CRASH_NSP_G_SPEED_MULTIPLIER 0.625F
#define CRASH_NSP_A_SPEED_MULTIPLIER 0.5F
#define CRASH_NSP_GRAVITY 2.625F
#define CRASH_NSP_FALL_SPEED 48.0F
#define CRASH_NSP_JUMP_SPEED 72.0F
#define CRASH_NSP_FLING_SIZE 300.0F
#define CRASH_FGM_FLING 0x5BD
#define CRASH_FGM_EAT 0x5BF

/* CrashUSP (the belly flop). */
#define CRASH_USP_Y_SPEED 56.0F
#define CRASH_USP_FALL_SPEED 120.0F
#define CRASH_USP_GRAVITY 1.5F
#define CRASH_USP_GRAVITY_2 18.0F
#define CRASH_USP_AIR_SPEED 48.0F
#define CRASH_USP_AIR_SPEED_2 20.0F
#define CRASH_USP_AIR_FRICTION 2.0F

/* CrashDSP (the dig). */
#define CRASH_DSP_MIN_SPEED 2.0F
#define CRASH_DSP_MAX_SPEED 40.0F
#define CRASH_DSP_ACCELERATION 8.0F
#define CRASH_DSP_G_SPEED_MULTIPLIER 0.5F
#define CRASH_DSP_END_Y_SPEED 60.0F
#define CRASH_DSP_END_Y_SPEED_EDGE 40.0F
#define CRASH_DSP_DIVE_Y_SPEED -100.0F
#define CRASH_DSP_DIVE_AIR_Y_SPEED 30.0F
#define CRASH_DSP_DIVE_AIR_FRICTION 1.75F
#define CRASH_DSP_DIVE_FALL_SPEED 100.0F
#define CRASH_DSP_DIVE_GRAVITY 0.5F
#define CRASH_DSP_DIVE_GRAVITY_2 7.0F
#define CRASH_DSP_MAX_TIME 180
#define CRASH_BUTTON_B 0x4000u

/* CrashDSP.dig_ecb_patch_: his collision box while digging, and the size
 * the patch restores (hard-coded in the donor). */
#define CRASH_ECB_DEFAULT_UPPER 320.0F
#define CRASH_ECB_DEFAULT_MIDDLE 190.0F
#define CRASH_ECB_DIG_UPPER 150.0F
#define CRASH_ECB_DIG_MIDDLE 80.0F

static f32 ndsP4CrashBitsToF32(u32 bits)
{
    union { u32 u; f32 f; } v;

    v.u = bits;
    return v.f;
}

static void ndsP4CrashClearFastFall(FTStruct *fp)
{
    fp->is_absorb = FALSE;
    fp->absorb_lr = 0;
    fp->is_goto_attack100 = FALSE;
    fp->is_fastfall = FALSE;
}

static f32 *ndsP4CrashStatusF32(FTStruct *fp, s32 word)
{
    return &((f32 *)(void *)&fp->status_vars)[word];
}

static s32 *ndsP4CrashStatusS32(FTStruct *fp, s32 word)
{
    return &((s32 *)(void *)&fp->status_vars)[word];
}

/* ftPhysicsApplyAirVelXFriction with the donor's stack "attributes": only
 * the friction word differs. */
static void ndsP4CrashAirFriction(FTStruct *fp, f32 friction)
{
    if (fp->physics.vel_air.x < 0.0F)
    {
        fp->physics.vel_air.x += friction;
        if (fp->physics.vel_air.x >= 0.0F)
        {
            fp->physics.vel_air.x = 0.0F;
        }
    }
    else
    {
        fp->physics.vel_air.x -= friction;
        if (fp->physics.vel_air.x <= 0.0F)
        {
            fp->physics.vel_air.x = 0.0F;
        }
    }
}

/* The spin's and the dig's shared steering: accelerate the x velocity
 * toward the target by `accel`, a difference under 2 leaving it as it is
 * (the donor's `mov.s f2, f4` meant the opposite and does nothing), capped
 * at `max` with its sign. */
static f32 ndsP4CrashSteer(f32 vel, f32 target, f32 accel, f32 max)
{
    f32 diff = target - vel;

    if (2.0F <= ABS(diff))
    {
        union { f32 f; u32 u; } d;

        d.f = diff;
        vel = ((d.u & 0x80000000u) == 0u) ? (vel + accel) : (vel - accel);
    }
    if (!(ABS(vel) <= max))
    {
        union { f32 f; u32 u; } v, m;

        v.f = vel;
        m.f = max;
        m.u |= v.u & 0x80000000u;
        vel = m.f;
    }
    return vel;
}

/* ---- The spin and dig effects (S6) ----
 *
 * CrashNSP.create_spin_gfx_ is Link's spin attack effect
 * (efManagerLinkSpinAttackMakeEffect, whose tail the donor jumps into) on
 * CRASH_SPIN_GFX, his special file 2; setup_spin_gfx_ marks it attached and
 * gives its two parts their prim and env colours from spin_gfx_table by
 * costume. CrashDSP.create_dig_gfx_ makes the same description on
 * CRASH_DIG_GFX (the file special file 2's word 0xCC0 names), attached by
 * position to his top joint with its facing copied; setup_dig_gfx_ makes
 * its second XObj scalable. Remix's Size.asm render routine and its writes
 * of the size multiplier are its size toggle (1 in the profile). */
extern void *gNdsP4CrashSpecial2;
extern const u32 gNdsP4CrashSpinColors[28];
extern const u32 gNdsP4CrashDigTurnTrack[8];
extern const u32 gNdsP4CrashDigWaitTrack[4];
extern const u32 gNdsP4CrashDigEndTrack[35];
void efManagerHaveStructProcUpdate(GObj *effect_gobj);
void gcDrawDObjTreeDLLinksForGObj(GObj *gobj);
f32 ftKirbySpecialLwGetGroundAxisYaw(FTStruct *fp);
void ftParamProcPauseEffect(GObj *effect_gobj);
void ftParamProcResumeEffect(GObj *fighter_gobj);

#define CRASH_SPIN_COSTUMES 7
#define CRASH_DIG_FILE_WORD 0xCC0
#define CRASH_FGM_DIG 132
#define CRASH_FGM_DIVE_LAND_1 45
#define CRASH_FGM_DIVE_LAND_2 220

static EFDesc sNdsP4CrashSpinDesc = {
    0x4 | EFFECT_FLAG_USERDATA, 15, &gNdsP4CrashSpecial2,
    { 0x50, nGCMatrixKindRotRpyR, 0x00 },
    { nGCMatrixKindTraRotRpyRSca, nGCMatrixKindNull, 0x00 },
    efManagerHaveStructProcUpdate, gcDrawDObjTreeDLLinksForGObj,
    0x708, 0x838, 0x894, 0x8B4
};

static void ndsP4CrashDigGfxProcUpdate(GObj *effect_gobj);

/* dig_gfx_struct; its file head is set per make (create_dig_gfx_). */
static EFDesc sNdsP4CrashDigDesc = {
    0x4 | EFFECT_FLAG_USERDATA, 15, NULL,
    { 0x50, nGCMatrixKindRotRpyR, 0x00 },
    { nGCMatrixKindTraRotRpyRSca, nGCMatrixKindNull, 0x00 },
    ndsP4CrashDigGfxProcUpdate, gcDrawDObjTreeDLLinksForGObj,
    0x230, 0x340, 0x378, 0x394
};

/* The dig effect once he comes out (end_initial_): the donor freezes the
 * attached effect's joint matrix where it is (XObj unk05 = 2) and plays the
 * end track there. The port's attach builder reads the joint every draw, so
 * the effect is remade free at that position: its root's own translation is
 * where kind 0x50 put it, its rotation and joint 1's texture scroll carry
 * over. */
static EFDesc sNdsP4CrashDigEndDesc = {
    0x4 | EFFECT_FLAG_USERDATA, 15, NULL,
    { nGCMatrixKindTraRotRpyRSca, nGCMatrixKindNull, 0x00 },
    { nGCMatrixKindTraRotRpyRSca, nGCMatrixKindNull, 0x00 },
    ndsP4CrashDigGfxProcUpdate, gcDrawDObjTreeDLLinksForGObj,
    0x230, 0x340, 0x378, 0x394
};

static void ndsP4CrashSetColor(SYColorPack *c, u32 rgba)
{
    c->s.r = (u8)(rgba >> 24);
    c->s.g = (u8)(rgba >> 16);
    c->s.b = (u8)(rgba >> 8);
    c->s.a = (u8)rgba;
}

static void ndsP4CrashSetPartColors(DObj *part, const u32 *prim_env)
{
    if ((part != NULL) && (part->mobj != NULL))
    {
        ndsP4CrashSetColor(&part->mobj->sub.primcolor, prim_env[0]);
        ndsP4CrashSetColor(&part->mobj->sub.envcolor, prim_env[1]);
    }
}

/* CrashNSP.create_spin_gfx_ + setup_spin_gfx_. */
static void ndsP4CrashMakeSpinGfx(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    GObj *effect_gobj;
    EFStruct *ep;
    DObj *dobj;
    const u32 *colors;

    effect_gobj = efManagerMakeEffectNoForce(&sNdsP4CrashSpinDesc);
    if (effect_gobj == NULL)
    {
        return;
    }
    ep = efGetStruct(effect_gobj);
    ep->fighter_gobj = fighter_gobj;
    fp->proc_lagstart = ftParamProcPauseEffect;
    fp->proc_lagend = ftParamProcResumeEffect;
    dobj = DObjGetStruct(effect_gobj);
    dobj->user_data.p = fp->joints[nFTPartsJointTopN];
    dobj->rotate.vec.f.y = (fp->lr == +1) ? F_CLC_DTOR32(30.0F) : F_CLC_DTOR32(210.0F);

    fp->is_effect_attach = TRUE;
    colors = &gNdsP4CrashSpinColors[4 * ((fp->costume < CRASH_SPIN_COSTUMES) ? fp->costume : 0)];
    if (dobj->child != NULL)
    {
        ndsP4CrashSetPartColors(dobj->child, &colors[0]);
        ndsP4CrashSetPartColors(dobj->child->child, &colors[2]);
    }
}

static GObj **ndsP4CrashDigGfx(FTStruct *fp)
{
    return (GObj **)(void *)ndsP4CrashStatusS32(fp, 1);
}

/* CrashDSP.create_dig_gfx_ + setup_dig_gfx_; the effect is kept in the
 * dig's status word 1 (0xB1C). */
static GObj *ndsP4CrashMakeDigGfx(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    GObj *effect_gobj;
    DObj *dobj;
    DObj *top = fp->joints[nFTPartsJointTopN];

    if (gNdsP4CrashSpecial2 == NULL)
    {
        return NULL;
    }
    sNdsP4CrashDigDesc.file_head =
        (void **)(void *)((u8 *)gNdsP4CrashSpecial2 + CRASH_DIG_FILE_WORD);
    effect_gobj = efManagerMakeEffectNoForce(&sNdsP4CrashDigDesc);
    if (effect_gobj == NULL)
    {
        return NULL;
    }
    efGetStruct(effect_gobj)->fighter_gobj = fighter_gobj;
    dobj = DObjGetStruct(effect_gobj);
    dobj->user_data.p = top;
    dobj->rotate.vec.f.y = top->rotate.vec.f.y;

    fp->is_effect_attach = TRUE;
    if (dobj->xobjs[1] != NULL)
    {
        dobj->xobjs[1]->kind = nGCMatrixKindTraRotRpyRSca;
    }
    *ndsP4CrashDigGfx(fp) = effect_gobj;
    return effect_gobj;
}

/* gcAddDObjAnimJoint on the dig effect's joint 1. */
static void ndsP4CrashDigTrack(GObj *effect_gobj, const u32 *track)
{
    DObj *root = (effect_gobj != NULL) ? DObjGetStruct(effect_gobj) : NULL;

    if ((root != NULL) && (root->child != NULL))
    {
        gcAddDObjAnimJoint(root->child, (AObjEvent32 *)(uintptr_t)track, 0.0F);
    }
}

/* CrashDSP.dig_graphic_update_: the effect follows the slope and scrolls
 * its texture with his speed. */
static void ndsP4CrashDigGraphicUpdate(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    f32 slope = ftKirbySpecialLwGetGroundAxisYaw(fp);
    GObj *effect_gobj = *ndsP4CrashDigGfx(fp);
    DObj *root;
    MObj *mobj;

    if (effect_gobj == NULL)
    {
        return;
    }
    root = DObjGetStruct(effect_gobj);
    root->rotate.vec.f.z = slope;
    mobj = (root->child != NULL) ? root->child->mobj : NULL;
    if (mobj != NULL)
    {
        mobj->sub.trav -= ABSF(fp->physics.vel_air.x) * ndsP4CrashBitsToF32(0x3B400000u);
    }
}

/* CrashDSP.dig_update_: while he moves, dust behind him every 16 frames of
 * the dig's timer (from the phase his first move marked) and a sound every
 * 32. */
static void ndsP4CrashDigUpdate(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    s32 timer;
    Vec3f pos;

    if ((*ndsP4CrashDigGfx(fp) == NULL) || (fp->physics.vel_air.x == 0.0F))
    {
        return;
    }
    timer = *ndsP4CrashStatusS32(fp, 0) - *ndsP4CrashStatusS32(fp, 2);
    if ((timer & 0xF) != 0)
    {
        return;
    }
    pos.x = DObjGetStruct(fighter_gobj)->translate.vec.f.x - ((f32)fp->lr * 140.0F);
    pos.y = DObjGetStruct(fighter_gobj)->translate.vec.f.y;
    pos.z = 0.0F;
    efManagerDustLightMakeEffect(&pos, fp->lr, 1.0F);
    if ((timer & 0x1F) == 0)
    {
        /* and Global.rumble_(port, 0, 30): the DS has no rumble. */
        func_800269C0_275C0(CRASH_FGM_DIG);
    }
}

/* CrashDSP.dig_gfx_main_ (0x800FD568 with the donor's end): when its
 * animation ends, a heavy dust 80 below where it stood. */
static void ndsP4CrashDigGfxProcUpdate(GObj *effect_gobj)
{
    EFStruct *ep = efGetStruct(effect_gobj);
    DObj *root;
    Vec3f pos;

    if (ep->is_pause_effect)
    {
        return;
    }
    gcPlayAnimAll(effect_gobj);
    if (effect_gobj->anim_frame > 0.0F)
    {
        return;
    }
    root = DObjGetStruct(effect_gobj);
    pos.x = root->translate.vec.f.x;
    pos.y = root->translate.vec.f.y - 80.0F;
    pos.z = 0.0F;
    efManagerDustHeavyDoubleMakeEffect(&pos, 1, 1.0F);
    if ((ep->fighter_gobj != NULL) &&
        (*ndsP4CrashDigGfx(ftGetStruct(ep->fighter_gobj)) == effect_gobj))
    {
        *ndsP4CrashDigGfx(ftGetStruct(ep->fighter_gobj)) = NULL;
    }
    efManagerSetPrevStructAlloc(ep);
    gcEjectGObj(effect_gobj);
}

/* end_initial_'s part: the effect stays where he leaves the ground and
 * plays the end track (sNdsP4CrashDigEndDesc). */
static void ndsP4CrashDigGfxEnd(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    GObj *old_gobj = *ndsP4CrashDigGfx(fp);
    GObj *effect_gobj;
    DObj *top = fp->joints[nFTPartsJointTopN];
    DObj *old_root;
    DObj *root;

    if (old_gobj == NULL)
    {
        return;
    }
    old_root = DObjGetStruct(old_gobj);
    sNdsP4CrashDigEndDesc.file_head = sNdsP4CrashDigDesc.file_head;
    effect_gobj = efManagerMakeEffectNoForce(&sNdsP4CrashDigEndDesc);
    if (effect_gobj != NULL)
    {
        efGetStruct(effect_gobj)->fighter_gobj = fighter_gobj;
        root = DObjGetStruct(effect_gobj);
        root->translate.vec.f = top->translate.vec.f;
        root->rotate.vec.f = old_root->rotate.vec.f;
        root->rotate.vec.f.y = top->rotate.vec.f.y;
        if ((root->child != NULL) && (root->child->mobj != NULL) &&
            (old_root->child != NULL) && (old_root->child->mobj != NULL))
        {
            root->child->mobj->sub.trau = old_root->child->mobj->sub.trau;
            root->child->mobj->sub.trav = old_root->child->mobj->sub.trav;
        }
        ndsP4CrashDigTrack(effect_gobj, gNdsP4CrashDigEndTrack);
    }
    *ndsP4CrashDigGfx(fp) = effect_gobj;
    efManagerSetPrevStructAlloc(efGetStruct(old_gobj));
    gcEjectGObj(old_gobj);
}

/* ---- Neutral special (the spin) ---- */

static void ndsP4CrashNSPShieldHit(GObj *fighter_gobj);

static void ndsP4CrashNSPInitial(GObj *fighter_gobj, s32 status_id, f32 x_scale)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    ftMainSetStatus(fighter_gobj, status_id, 0.0F, 1.0F, FTSTATUS_PRESERVE_NONE);
    ftMainPlayAnimEventsAll(fighter_gobj);
    ndsP4CrashMakeSpinGfx(fighter_gobj);
    fp->proc_shield = ndsP4CrashNSPShieldHit;
    fp->physics.vel_air.x *= x_scale;
    fp->motion_vars.flags.flag0 = 0;
    fp->motion_vars.flags.flag1 = 0;
    fp->motion_vars.flags.flag2 = 0;
}

/* CrashNSP.ground_initial_ (ground_nsp). */
void ndsP4CrashNSPGroundInitial(GObj *fighter_gobj)
{
    ndsP4CrashNSPInitial(fighter_gobj, CRASH_STATUS_NSPG, 1.5F);
}

/* CrashNSP.air_initial_ (air_nsp). */
void ndsP4CrashNSPAirInitial(GObj *fighter_gobj)
{
    ndsP4CrashNSPInitial(fighter_gobj, CRASH_STATUS_NSPA, 1.25F);
    ndsP4CrashClearFastFall(ftGetStruct(fighter_gobj));
}

/* Any active hitbox's record against a shield
 * (Character.get_hitbox_collision_flags_ & 0x40). */
static sb32 ndsP4CrashShieldContact(FTStruct *fp)
{
    s32 i;
    s32 j;

    for (i = 0; i < FTATTACKCOLL_NUM_MAX; i++)
    {
        if (fp->attack_colls[i].attack_state == nGMAttackStateOff)
        {
            continue;
        }
        for (j = 0; j < GMATTACKREC_NUM_MAX; j++)
        {
            if (fp->attack_colls[i].attack_records[j].victim_flags.is_interact_shield)
            {
                return TRUE;
            }
        }
    }
    return FALSE;
}

/* CrashNSP.shield_hit_routine_ (his shield routine; it runs for clangs
 * too, so it tests for a shield): the spin stops dead and keeps its
 * velocities for after the "hitlag" the blocked animation plays. */
static void ndsP4CrashNSPShieldHit(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (ndsP4CrashShieldContact(fp) == FALSE)
    {
        return;
    }
    fp->motion_vars.flags.flag1 = 0;
    *ndsP4CrashStatusF32(fp, 0) = fp->physics.vel_air.x * 0.25F;
    *ndsP4CrashStatusF32(fp, 1) = fp->physics.vel_air.y * 0.5F;
    *ndsP4CrashStatusF32(fp, 2) = fp->physics.vel_ground.x * 0.5F;
    *ndsP4CrashStatusS32(fp, 3) = FALSE;
    fp->physics.vel_air.x = 0.0F;
    fp->physics.vel_ground.x = 0.0F;
    fp->physics.vel_air.y = 0.0F;
    if (fp->ga == nMPKineticsGround)
    {
        ftMainSetStatus(fighter_gobj, CRASH_STATUS_NSPG_BLOCKED, 0.0F, 1.0F,
                        FTSTATUS_PRESERVE_NONE);
    }
    else
    {
        ndsP4CrashClearFastFall(fp);
        ftMainSetStatus(fighter_gobj, CRASH_STATUS_NSPA_BLOCKED, 0.0F, 1.0F,
                        FTSTATUS_PRESERVE_NONE);
    }
    ftMainPlayAnimEventsAll(fighter_gobj);

    /* CrashNSP.shield_hitlag_patch_ (ftMainProcParams): a blocked spin
     * takes no hitlag from the push unless it rebounds. The push is the
     * damage the source charges after this routine and clears after it. */
    if (fp->attack_rebound == 0.0F)
    {
        fp->attack_shield_push = 0;
    }
}

/* CrashNSP.spin_detect_item_: the first light item that can be picked up
 * within 300 of him across and from his feet to 600 above (its previous
 * position, as the donor reads it). */
static GObj *ndsP4CrashSpinDetectItem(GObj *fighter_gobj)
{
#if NDS_P2_ITEM_CORE
    DObj *topn = DObjGetStruct(fighter_gobj);
    f32 px = topn->translate.vec.f.x;
    f32 py = topn->translate.vec.f.y + CRASH_NSP_FLING_SIZE;
    GObj *item_gobj;

    for (item_gobj = gGCCommonLinks[nGCCommonLinkIDItem]; item_gobj != NULL;
         item_gobj = item_gobj->link_next)
    {
        ITStruct *ip = itGetStruct(item_gobj);

        if ((ip->is_allow_pickup == FALSE) || (ip->weight == 0))
        {
            continue;
        }
        if (!(ABS(px - ip->coll_data.pos_prev.x) <= CRASH_NSP_FLING_SIZE))
        {
            continue;
        }
        if (!(ABS(py - ip->coll_data.pos_prev.y) <= CRASH_NSP_FLING_SIZE))
        {
            continue;
        }
        return item_gobj;
    }
#else
    (void)fighter_gobj;
#endif
    return NULL;
}

#if NDS_P2_ITEM_CORE
/* CrashNSP.flung_item_state_table: no routines; it flies on its velocity
 * until it leaves the stage. */
static ITStatusDesc sNdsP4CrashFlungItemStatus[1];
#endif

/* CrashNSP's item check (ground_main_, air_main_): an item near the spin
 * flies away at 300 and can never be picked up. */
static void ndsP4CrashNSPItemCheck(GObj *fighter_gobj)
{
#if NDS_P2_ITEM_CORE
    GObj *item_gobj = ndsP4CrashSpinDetectItem(fighter_gobj);
    ITStruct *ip;

    if (item_gobj == NULL)
    {
        return;
    }
    ip = itGetStruct(item_gobj);
    ip->physics.vel_air.x =
        (DObjGetStruct(fighter_gobj)->translate.vec.f.x <= DObjGetStruct(item_gobj)->translate.vec.f.x)
            ? CRASH_NSP_FLING_SIZE : -CRASH_NSP_FLING_SIZE;
    /* CrashNSP.item_fling_. */
    ip->is_allow_pickup = FALSE;
    itMapSetAir(ip);
    itMainSetStatus(item_gobj, sNdsP4CrashFlungItemStatus, 0);
    func_800269C0_275C0(CRASH_FGM_FLING);
#else
    (void)fighter_gobj;
#endif
}

static void ndsP4CrashNSPGroundToAir(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    mpCommonSetFighterAir(fp);
    ftMainSetStatus(fighter_gobj, CRASH_STATUS_NSPA, fighter_gobj->anim_frame, 1.0F,
                    CRASH_PRESERVE_SPIN);
    fp->proc_shield = ndsP4CrashNSPShieldHit;
}

static void ndsP4CrashNSPAirToGround(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    mpCommonSetFighterGround(fp);
    ftMainSetStatus(fighter_gobj, CRASH_STATUS_NSPG, fighter_gobj->anim_frame, 1.0F,
                    CRASH_PRESERVE_SPIN);
    fp->proc_shield = ndsP4CrashNSPShieldHit;
}

/* CrashNSP.ground_main_ (0xDF update): the end of the animation waits;
 * otherwise a jump input (before the script's temp variable 2) jumps out
 * of the spin at 72 with the double dust. The item check runs unless he
 * jumped. */
void ndsP4CrashNSPGroundMain(GObj *fighter_gobj)
{
    FTStruct *fp;

    if (fighter_gobj->anim_frame <= 0.0F)
    {
        mpCommonSetFighterWaitOrFall(fighter_gobj);
    }
    else
    {
        fp = ftGetStruct(fighter_gobj);
        if ((fp->motion_vars.flags.flag1 == 0) &&
            (ftCommonKneeBendGetInputTypeCommon(fp) != 0))
        {
            /* CrashNSP.ground_to_air_jump_. */
            ndsP4CrashNSPGroundToAir(fighter_gobj);
            fp->physics.vel_air.y = CRASH_NSP_JUMP_SPEED;
            efManagerDustHeavyDoubleMakeEffect(fp->coll_data.p_translate, 1, 1.0F);
            return;
        }
    }
    ndsP4CrashNSPItemCheck(fighter_gobj);
}

/* CrashNSP.air_main_ (0xE0 update). */
void ndsP4CrashNSPAirMain(GObj *fighter_gobj)
{
    if (fighter_gobj->anim_frame <= 0.0F)
    {
        mpCommonSetFighterWaitOrFall(fighter_gobj);
    }
    ndsP4CrashNSPItemCheck(fighter_gobj);
}

/* CrashNSP.blocked_main_ (0xE1/0xE2 update): when the script's temp
 * variable 2 ends the blocked "hitlag", the saved velocities return. */
void ndsP4CrashNSPBlockedMain(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (fp->motion_vars.flags.flag1 != 0)
    {
        *ndsP4CrashStatusS32(fp, 3) = TRUE;
        fp->physics.vel_air.x = *ndsP4CrashStatusF32(fp, 0);
        fp->physics.vel_air.y = *ndsP4CrashStatusF32(fp, 1);
        fp->physics.vel_ground.x = *ndsP4CrashStatusF32(fp, 2);
        fp->motion_vars.flags.flag1 = 0;
    }
    if (fighter_gobj->anim_frame <= 0.0F)
    {
        mpCommonSetFighterWaitOrFall(fighter_gobj);
    }
}

/* CrashNSP.ground_physics_ (0xDF physics): the stick steers the spin
 * (0.625 a stick unit, accelerating by 2); a neutral stick slows it at
 * 1.25 and stops it under 2; temp variable 2 brakes it at 4.5. */
void ndsP4CrashNSPGroundPhysics(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    f32 accel = CRASH_NSP_ACCELERATION;
    f32 target = 0.0F;

    if (fp->motion_vars.flags.flag1 != 0)
    {
        accel = CRASH_NSP_END_ACCELERATION;
    }
    else if (ABS(fp->input.pl.stick_range.x) < 11)
    {
        accel = 1.25F;
        if (ABS(fp->physics.vel_air.x) <= CRASH_NSP_MIN_SPEED)
        {
            fp->physics.vel_air.x = 0.0F;
            return;
        }
    }
    else target = (f32)fp->input.pl.stick_range.x * CRASH_NSP_G_SPEED_MULTIPLIER;

    fp->physics.vel_air.x = ndsP4CrashSteer(fp->physics.vel_air.x, target, accel,
                                            CRASH_NSP_MAX_SPEED);
    fp->physics.vel_ground.x = fp->physics.vel_air.x * (f32)fp->lr;
}

/* CrashNSP.air_physics_ (0xE0 physics): the same steering at 0.5 a stick
 * unit, then his spin gravity. */
void ndsP4CrashNSPAirPhysics(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (ABS(fp->input.pl.stick_range.x) < 11)
    {
        if (ABS(fp->physics.vel_air.x) <= CRASH_NSP_MIN_SPEED)
        {
            fp->physics.vel_air.x = 0.0F;
        }
        else fp->physics.vel_air.x = ndsP4CrashSteer(fp->physics.vel_air.x, 0.0F,
                                                     CRASH_NSP_ACCELERATION, CRASH_NSP_MAX_SPEED);
    }
    else fp->physics.vel_air.x = ndsP4CrashSteer(
        fp->physics.vel_air.x,
        (f32)fp->input.pl.stick_range.x * CRASH_NSP_A_SPEED_MULTIPLIER,
        CRASH_NSP_ACCELERATION, CRASH_NSP_MAX_SPEED);

    ftPhysicsApplyGravityClampTVel(fp, CRASH_NSP_GRAVITY, CRASH_NSP_FALL_SPEED);
}

/* CrashNSP.blocked_air_physics_ (0xE2 physics): nothing until the blocked
 * "hitlag" is over. */
void ndsP4CrashNSPBlockedAirPhysics(GObj *fighter_gobj)
{
    if (*ndsP4CrashStatusS32(ftGetStruct(fighter_gobj), 3) != FALSE)
    {
        ftPhysicsApplyAirVelFriction(fighter_gobj);
    }
}

/* CrashNSP.ground_collision_ / air_collision_. */
void ndsP4CrashNSPGroundMap(GObj *fighter_gobj)
{
    mpCommonProcFighterOnFloor(fighter_gobj, ndsP4CrashNSPGroundToAir);
}

void ndsP4CrashNSPAirMap(GObj *fighter_gobj)
{
    mpCommonProcFighterLanding(fighter_gobj, ndsP4CrashNSPAirToGround);
}

static void ndsP4CrashNSPBlockedGroundToAir(GObj *fighter_gobj)
{
    mpCommonSetFighterAir(ftGetStruct(fighter_gobj));
    ftMainSetStatus(fighter_gobj, CRASH_STATUS_NSPA_BLOCKED, fighter_gobj->anim_frame, 1.0F,
                    CRASH_PRESERVE_SPIN);
}

static void ndsP4CrashNSPBlockedAirToGround(GObj *fighter_gobj)
{
    mpCommonSetFighterGround(ftGetStruct(fighter_gobj));
    ftMainSetStatus(fighter_gobj, CRASH_STATUS_NSPG_BLOCKED, fighter_gobj->anim_frame, 1.0F,
                    CRASH_PRESERVE_SPIN);
}

/* CrashNSP.blocked_ground_collision_ / blocked_air_collision_. */
void ndsP4CrashNSPBlockedGroundMap(GObj *fighter_gobj)
{
    mpCommonProcFighterOnFloor(fighter_gobj, ndsP4CrashNSPBlockedGroundToAir);
}

void ndsP4CrashNSPBlockedAirMap(GObj *fighter_gobj)
{
    mpCommonProcFighterLanding(fighter_gobj, ndsP4CrashNSPBlockedAirToGround);
}

/* ---- Up special (the belly flop) ---- */

static void ndsP4CrashUSPInitial(GObj *fighter_gobj, s32 status_id)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    ftMainSetStatus(fighter_gobj, status_id, 0.0F, 1.0F, FTSTATUS_PRESERVE_NONE);
    ftMainPlayAnimEventsAll(fighter_gobj);
    fp->motion_vars.flags.flag0 = 0;
    fp->motion_vars.flags.flag1 = 0;
    fp->motion_vars.flags.flag2 = 0;
}

/* CrashUSP.ground_initial_ (ground_usp). */
void ndsP4CrashUSPGroundInitial(GObj *fighter_gobj)
{
    ndsP4CrashUSPInitial(fighter_gobj, CRASH_STATUS_USPG);
    ftGetStruct(fighter_gobj)->physics.vel_air.y = 0.0F;
}

/* CrashUSP.air_initial_ (air_usp): the fall nearly stopped (0x3D480000,
 * the source's "0.05"). */
void ndsP4CrashUSPAirInitial(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    ndsP4CrashUSPInitial(fighter_gobj, CRASH_STATUS_USPA);
    ndsP4CrashClearFastFall(fp);
    fp->physics.vel_air.y *= ndsP4CrashBitsToF32(0x3D480000u);
}

/* CrashUSP.landing_initial_. */
static void ndsP4CrashUSPLandingInitial(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    mpCommonSetFighterGround(fp);
    ftMainSetStatus(fighter_gobj, CRASH_STATUS_USP_LANDING, 0.0F, 1.0F, FTSTATUS_PRESERVE_NONE);
    ftMainPlayAnimEventsAll(fighter_gobj);
    fp->physics.vel_ground.x = 0.0F;
}

/* CrashUSP.main_ (0xEB/0xEC update): the script's temp variable 1 launches
 * him, airborne, his jumps spent. */
void ndsP4CrashUSPMain(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (fp->motion_vars.flags.flag0 == 0)
    {
        return;
    }
    fp->motion_vars.flags.flag0 = 0;
    fp->physics.vel_air.y = CRASH_USP_Y_SPEED;
    mpCommonSetFighterAir(fp);
    fp->jumps_used = (u8)fp->attr->jumps_max;
}

/* CrashUSP.change_direction_ (0xEB/0xEC interrupt): while the script holds
 * temp variable 3, one chance to turn (Captain's, armed by temp variable 2,
 * both cleared after). */
void ndsP4CrashUSPChangeDirection(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (fp->motion_vars.flags.flag2 == 0)
    {
        return;
    }
    fp->motion_vars.flags.flag1 = 1;
    ftCaptainSpecialHiProcInterrupt(fighter_gobj);
    fp->motion_vars.flags.flag1 = 0;
    fp->motion_vars.flags.flag2 = 0;
}

/* CrashUSP.physics_ (0xEB/0xEC physics): his gravity (18 once the script's
 * temp variable 2 drops him), his drift (slower then) and air friction 2;
 * the drop skips the drift whenever the speed clamp holds. */
void ndsP4CrashUSPPhysics(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    FTAttributes *attr = fp->attr;
    sb32 drop = (fp->motion_vars.flags.flag1 != 0) ? TRUE : FALSE;

    ftPhysicsApplyGravityClampTVel(fp, (drop != FALSE) ? CRASH_USP_GRAVITY_2 : CRASH_USP_GRAVITY,
                                   CRASH_USP_FALL_SPEED);
    if ((drop != FALSE) && (ftPhysicsCheckClampAirVelXDecMax(fp, attr) != FALSE))
    {
        return;
    }
    /* CrashUSP.air_control_. */
    if (drop != FALSE)
    {
        ftPhysicsClampAirVelXStickRange(fp, 8, ndsP4CrashBitsToF32(0x3D4D0000u),
                                        CRASH_USP_AIR_SPEED_2);
    }
    else ftPhysicsClampAirVelXStickRange(fp, 8, ndsP4CrashBitsToF32(0x3DA40000u),
                                         CRASH_USP_AIR_SPEED);
    /* CrashUSP.air_friction_. */
    ndsP4CrashAirFriction(fp, CRASH_USP_AIR_FRICTION);
}

/* CrashUSP.collision_ (0xEB/0xEC map): a ledge or a landing of his own. */
void ndsP4CrashUSPMap(GObj *fighter_gobj)
{
    if (ftGetStruct(fighter_gobj)->ga == nMPKineticsGround)
    {
        mpCommonSetFighterFallOnEdgeBreak(fighter_gobj);
    }
    else mpCommonProcFighterCliff(fighter_gobj, ndsP4CrashUSPLandingInitial);
}

/* ---- Down special (the dig) ---- */

static void ndsP4CrashDSPSetStatus(GObj *fighter_gobj, s32 status_id, f32 frame_begin)
{
    ftMainSetStatus(fighter_gobj, status_id, frame_begin, 1.0F, CRASH_PRESERVE_DIG);
    ftMainPlayAnimEventsAll(fighter_gobj);
}

static void ndsP4CrashDSPClearFlags(FTStruct *fp)
{
    fp->motion_vars.flags.flag0 = 0;
    fp->motion_vars.flags.flag1 = 0;
    fp->motion_vars.flags.flag2 = 0;
}

/* CrashDSP.begin_initial_. */
static void ndsP4CrashDSPBeginInitial(GObj *fighter_gobj)
{
    ndsP4CrashDSPSetStatus(fighter_gobj, CRASH_STATUS_DSP_BEGIN, 0.0F);
    (void)ndsP4CrashMakeDigGfx(fighter_gobj);
    ndsP4CrashDSPClearFlags(ftGetStruct(fighter_gobj));
}

/* CrashDSP.dive_initial_. */
static void ndsP4CrashDSPDiveInitial(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    ndsP4CrashDSPSetStatus(fighter_gobj, CRASH_STATUS_DSP_DIVE, 0.0F);
    ndsP4CrashDSPClearFlags(fp);
    fp->physics.vel_air.x = 0.0F;
}

/* CrashDSP.initial_ (ground_dsp): a floor he can drop through dives
 * instead of digging. */
void ndsP4CrashDSPInitial(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    u32 pass = fp->coll_data.floor_flags & 0x4000u;

    *ndsP4CrashStatusS32(fp, 1) = 0;
    *ndsP4CrashStatusS32(fp, 2) = -1;
    if (pass != 0u)
    {
        ndsP4CrashDSPDiveInitial(fighter_gobj);
    }
    else ndsP4CrashDSPBeginInitial(fighter_gobj);
}

/* CrashDSP.dive_air_initial_ (air_dsp). */
void ndsP4CrashDSPDiveAirInitial(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    ndsP4CrashDSPSetStatus(fighter_gobj, CRASH_STATUS_DSP_AIR_DIVE, 0.0F);
    ndsP4CrashDSPClearFlags(fp);
    fp->physics.vel_air.x = 0.0F;
    fp->physics.vel_air.y = CRASH_DSP_DIVE_AIR_Y_SPEED;
}

/* CrashDSP.wait_initial_ / turn_intial_. */
static void ndsP4CrashDSPWaitInitial(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    GObj *effect_gobj;

    ndsP4CrashDSPSetStatus(fighter_gobj, CRASH_STATUS_DSP_WAIT, 0.0F);
    effect_gobj = *ndsP4CrashDigGfx(fp);
    if (effect_gobj != NULL)
    {
        DObjGetStruct(effect_gobj)->rotate.vec.f.y =
            fp->joints[nFTPartsJointTopN]->rotate.vec.f.y;
        ndsP4CrashDigTrack(effect_gobj, gNdsP4CrashDigWaitTrack);
    }
}

static void ndsP4CrashDSPTurnInitial(GObj *fighter_gobj)
{
    ndsP4CrashDSPSetStatus(fighter_gobj, CRASH_STATUS_DSP_TURN, 0.0F);
    ndsP4CrashDigTrack(*ndsP4CrashDigGfx(ftGetStruct(fighter_gobj)),
                       gNdsP4CrashDigTurnTrack);
}

/* CrashDSP.end_initial_: out of the ground, 60 up (40 when he slid off an
 * edge). */
static void ndsP4CrashDSPEndInitial(GObj *fighter_gobj, sb32 slide_off)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    ndsP4CrashDSPSetStatus(fighter_gobj, CRASH_STATUS_DSP_END, 0.0F);
    ndsP4CrashDigGfxEnd(fighter_gobj);
    fp->physics.vel_air.y = (slide_off != FALSE) ? CRASH_DSP_END_Y_SPEED_EDGE
                                                 : CRASH_DSP_END_Y_SPEED;
    mpCommonSetFighterAir(fp);
}

/* CrashDSP.slide_off_end_initial_. */
static void ndsP4CrashDSPSlideOffEndInitial(GObj *fighter_gobj)
{
    ndsP4CrashDSPEndInitial(fighter_gobj, TRUE);
}

/* CrashDSP.begin_initial_from_dive_: a dive landing digs in at frame 20. */
static void ndsP4CrashDSPBeginInitialFromDive(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    GObj *effect_gobj;

    mpCommonSetFighterGround(fp);
    ndsP4CrashDSPSetStatus(fighter_gobj, CRASH_STATUS_DSP_BEGIN, 20.0F);
    effect_gobj = ndsP4CrashMakeDigGfx(fighter_gobj);
    if (effect_gobj != NULL)
    {
        DObj *root = DObjGetStruct(effect_gobj);

        if (root->child != NULL)
        {
            root->child->anim_wait = 3.0F;
        }
        func_800269C0_275C0(CRASH_FGM_DIVE_LAND_1);
        func_800269C0_275C0(CRASH_FGM_DIVE_LAND_2);
    }
    ndsP4CrashDSPClearFlags(fp);
}

/* CrashDSP.begin_main_ (0xE5 update): the dig's timer starts at 180. */
void ndsP4CrashDSPBeginMain(GObj *fighter_gobj)
{
    ndsP4CrashDigGraphicUpdate(fighter_gobj);
    if (fighter_gobj->anim_frame <= 0.0F)
    {
        *ndsP4CrashStatusS32(ftGetStruct(fighter_gobj), 0) = CRASH_DSP_MAX_TIME;
        ndsP4CrashDSPWaitInitial(fighter_gobj);
    }
}

/* The dig's timer and B: TRUE when it ended the dig. */
static sb32 ndsP4CrashDSPCheckEnd(GObj *fighter_gobj, FTStruct *fp)
{
    s32 *timer = ndsP4CrashStatusS32(fp, 0);

    *timer -= 1;
    if ((*timer == 0) || ((fp->input.pl.button_hold & CRASH_BUTTON_B) == 0))
    {
        ndsP4CrashDSPEndInitial(fighter_gobj, FALSE);
        return TRUE;
    }
    return FALSE;
}

/* CrashDSP.wait_main_ (0xE6 update): ends when the timer runs out or B is
 * let go; the stick against his facing turns him. */
void ndsP4CrashDSPWaitMain(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    s32 stick_x;

    ndsP4CrashDigGraphicUpdate(fighter_gobj);
    ndsP4CrashDigUpdate(fighter_gobj);
    if (ndsP4CrashDSPCheckEnd(fighter_gobj, fp) != FALSE)
    {
        return;
    }
    stick_x = fp->input.pl.stick_range.x;
    if (ABS(stick_x) < 11)
    {
        return;
    }
    if (((u32)stick_x & 0x80000000u) != ((u32)fp->lr & 0x80000000u))
    {
        ndsP4CrashDSPTurnInitial(fighter_gobj);
    }
}

/* CrashDSP.turn_main_ (0xE7 update): the script's temp variable 2 flips
 * his facing; the turn ends back in the wait. */
void ndsP4CrashDSPTurnMain(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    ndsP4CrashDigGraphicUpdate(fighter_gobj);
    ndsP4CrashDigUpdate(fighter_gobj);
    if (fp->motion_vars.flags.flag1 != 0)
    {
        fp->lr = -fp->lr;
        fp->motion_vars.flags.flag1 = 0;
    }
    if (ndsP4CrashDSPCheckEnd(fighter_gobj, fp) != FALSE)
    {
        return;
    }
    if (fighter_gobj->anim_frame <= 0.0F)
    {
        ndsP4CrashDSPWaitInitial(fighter_gobj);
    }
}

/* CrashDSP.dive_main_ (0xE9/0xEA update): the script's temp variable 2
 * drops him through the platform at 100. */
void ndsP4CrashDSPDiveMain(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (fp->motion_vars.flags.flag1 == 0)
    {
        return;
    }
    fp->coll_data.ignore_line_id = fp->coll_data.floor_line_id;
    fp->physics.vel_air.y = CRASH_DSP_DIVE_Y_SPEED;
    mpCommonSetFighterAir(fp);
}

/* CrashDSP.physics_ (0xE6/0xE7 physics): the dig's steering (0.5 a stick
 * unit, 8, capped at 40); starting to move marks the timer's phase for the
 * dust (status_vars word 2). */
void ndsP4CrashDSPPhysics(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    f32 target = 0.0F;
    s32 *offset = ndsP4CrashStatusS32(fp, 2);

    if (ABS(fp->input.pl.stick_range.x) < 11)
    {
        if (ABS(fp->physics.vel_air.x) <= CRASH_DSP_MIN_SPEED)
        {
            *offset = -1;
            fp->physics.vel_air.x = 0.0F;
            return;
        }
    }
    else target = (f32)fp->input.pl.stick_range.x * CRASH_DSP_G_SPEED_MULTIPLIER;

    if (*offset < 0)
    {
        *offset = *ndsP4CrashStatusS32(fp, 0) & 0x1F;
    }
    fp->physics.vel_air.x = ndsP4CrashSteer(fp->physics.vel_air.x, target,
                                            CRASH_DSP_ACCELERATION, CRASH_DSP_MAX_SPEED);
    fp->physics.vel_ground.x = fp->physics.vel_air.x * (f32)fp->lr;
}

/* CrashDSP.dive_air_physics_ (0xEA physics; 0xE9's once airborne): his
 * dive gravity (7 once the script's temp variable 2 drops him), the
 * default drift and air friction 1.75. */
void ndsP4CrashDSPDiveAirPhysics(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    FTAttributes *attr = fp->attr;

    ftPhysicsApplyGravityClampTVel(fp,
                                   (fp->motion_vars.flags.flag1 != 0) ? CRASH_DSP_DIVE_GRAVITY_2
                                                                       : CRASH_DSP_DIVE_GRAVITY,
                                   CRASH_DSP_DIVE_FALL_SPEED);
    if (ftPhysicsCheckClampAirVelXDecMax(fp, attr) == FALSE)
    {
        ftPhysicsClampAirVelXStickDefault(fp, attr);
        ndsP4CrashAirFriction(fp, CRASH_DSP_DIVE_AIR_FRICTION);
    }
}

/* CrashDSP.dive_physics_ (0xE9 physics). */
void ndsP4CrashDSPDivePhysics(GObj *fighter_gobj)
{
    if (ftGetStruct(fighter_gobj)->ga != nMPKineticsGround)
    {
        ndsP4CrashDSPDiveAirPhysics(fighter_gobj);
    }
}

/* CrashDSP.collision_ (0xE6/0xE7 map): an edge ends the dig. */
void ndsP4CrashDSPMap(GObj *fighter_gobj)
{
    mpCommonProcFighterOnFloor(fighter_gobj, ndsP4CrashDSPSlideOffEndInitial);
}

/* CrashDSP.dive_collision_ (0xE9/0xEA map). */
void ndsP4CrashDSPDiveMap(GObj *fighter_gobj)
{
    if (ftGetStruct(fighter_gobj)->ga == nMPKineticsGround)
    {
        mpCommonSetFighterFallOnEdgeBreak(fighter_gobj);
    }
    else mpCommonProcFighterLanding(fighter_gobj, ndsP4CrashDSPBeginInitialFromDive);
}

/* ---- Patches on his character id ---- */

/* CrashDSP.dig_ecb_patch_ (ftMainSetStatus' head): his attributes'
 * collision box shrinks while he digs and returns to the hard-coded size on
 * every other status. Every Crash shares the attributes, as in Remix. */
void ndsP4CrashOnStatus(GObj *fighter_gobj, s32 status_id)
{
    FTAttributes *attr = ftGetStruct(fighter_gobj)->attr;

    if ((status_id == CRASH_STATUS_DSP_WAIT) || (status_id == CRASH_STATUS_DSP_TURN))
    {
        attr->map_coll.top = CRASH_ECB_DIG_UPPER;
        attr->map_coll.center = CRASH_ECB_DIG_MIDDLE;
    }
    else
    {
        attr->map_coll.top = CRASH_ECB_DEFAULT_UPPER;
        attr->map_coll.center = CRASH_ECB_DEFAULT_MIDDLE;
    }
}

/* Size.asm adjust_top_joint_and_ecb_ (ftMainProcPhysicsMap's head, for
 * everyone): the fighter's collision box is its attributes' every frame
 * (the size multiplier is 1 in VS). The source copies it once, at
 * creation; only Crash's ever changes. */
static void ndsP4CrashBeforePhysicsMap(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    fp->coll_data.map_coll = fp->attr->map_coll;
    fp->coll_data.cliffcatch_coll = fp->attr->cliffcatch_coll;
}

/* Hitbox.asm force_grab_immunity_ (ftMainSearchFighterCatch): digging, he
 * cannot be grabbed by his first hurtbox. The patch reads the character id
 * through the hurtbox pointer the source's loop walks, so only the first
 * hurtbox finds his. */
static sb32 ndsP4CrashGrabbable(FTStruct *victim_fp, s32 damage_coll_id)
{
    if ((damage_coll_id == 0) &&
        ((victim_fp->status_id == CRASH_STATUS_DSP_WAIT) ||
         (victim_fp->status_id == CRASH_STATUS_DSP_TURN)))
    {
        return FALSE;
    }
    return TRUE;
}

/* Crash.asm dair_bounce_ (ftCommonAttackAirLwProcHit, in place of Link's
 * case): his down air bounces him 50 up and restarts at frame 35. */
static void ndsP4CrashAttackAirLwHit(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    fp->is_fastfall = FALSE;
    fp->physics.vel_air.y = 50.0F;
    ftMainSetStatus(fighter_gobj, nFTCommonStatusAttackAirLw, 35.0F, 1.0F,
                    FTSTATUS_PRESERVE_NONE);
}

/* Crash.asm crash_eat_sfx (the Maxim Tomato's heal). */
static void ndsP4CrashOnEatTomato(FTStruct *fp)
{
    (void)fp;
    func_800269C0_275C0(CRASH_FGM_EAT);
}

/* ---- Entry ----
 *
 * Crash.asm entry_script 0x8013DCBC is Samus's point case of
 * ftCommonAppearSetStatus; samusshared.asm get_entry_anim_gfx_struct gives
 * it crash_entry_anim_gfx_struct on CRASH_ENTRY, his special file 3: the
 * point's procs and transforms, DObjDesc 0x2310 and AnimJoint 0x27E8, drawn
 * through DObjDLLinks. */
extern void *gNdsP4CrashSpecial3;
void efManagerHaveStructProcUpdate(GObj *effect_gobj);
void gcDrawDObjTreeDLLinksForGObj(GObj *gobj);

static EFDesc sNdsP4CrashEntryDesc = {
    0x4, 10, &gNdsP4CrashSpecial3,
    { nGCMatrixKindTraRotRpyRSca, nGCMatrixKindNull, 0x00 },
    { nGCMatrixKindTraRotRpyRSca, nGCMatrixKindNull, 0x00 },
    efManagerHaveStructProcUpdate, gcDrawDObjTreeDLLinksForGObj,
    0x2310, 0x0, 0x27E8, 0x0
};

static void ndsP4CrashEntryCase(FTStruct *fp)
{
    ndsP4EntryMakeEffect(&sNdsP4CrashEntryDesc, &fp->entry_pos);
}

const NDSP4Overrides gNdsP4CrashOverrides = {
    .entry_case = ndsP4CrashEntryCase,
    .before_physics_map = ndsP4CrashBeforePhysicsMap,
    .grabbable = ndsP4CrashGrabbable,
    .attack_air_lw_hit = ndsP4CrashAttackAirLwHit,
    .on_eat_tomato = ndsP4CrashOnEatTomato,
};

#endif /* NDS_P4_CRASH */
