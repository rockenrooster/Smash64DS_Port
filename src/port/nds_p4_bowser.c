/*
 * P4 Bowser: native ports of the donor's special routines. Source:
 * JSsixtyfour/smashremix 5e04fe7, src/Bowser/BowserSpecial.asm and
 * bowser.asm, with his hooks in jigglypuffkirbyshared.asm and
 * captainshared.asm, read as assembled (scripts/p4/mipsdis.py). Bowser runs
 * Yoshi's status code (fp->fkind == nFTKindYoshi).
 *
 * His Clown Copter is his own special file 2 (BOWSER_CLOWN_COPTER, loaded
 * into gNdsP4BowserSpecial2 by the generator's OWN_SPECIAL_FILES) on the
 * Falcon Flyer's description, which his entry_script reaches through the
 * Falcon Flyer's case of ftCommonAppearSetStatus (NDS_P4_ENTRY_PORT).
 *
 * Player-struct offsets in the donor map to BattleShip fields as:
 *   0x17C/0x180/0x184 temp variables 1-3   motion_vars.flags.flag0/1/2
 *   0x18D & 0x07                           clears is_absorb, absorb_lr,
 *                                          is_goto_attack100 and is_fastfall
 *   0x1B6 B button mask                    input.button_mask_b
 *   0x1BC/0x1BE buttons held / tapped      input.pl.button_hold / button_tap
 *   0xADC/0xADE flame timer and ammo (u16) passive_vars, first two halfwords
 *   0xB18-0xB2C                            status_vars.common.fireflower
 *   0x8E8 + 4n                             joints[n]
 *   attributes + 0x0/0x64/0x33C            size, jumps_max, joint_itemlight_id
 */
#include <nds/nds_p4.h>

#if NDS_P4_BOWSER

#include <ef/effect.h>
#include <gm/gmsound.h>
#include <it/item.h>
#include <sys/audio.h>
#include <wp/weapon.h>

void gcDrawDObjTreeDLLinksForGObj(GObj *gobj);
void mpCommonSetFighterWaitOrLanding(GObj *fighter_gobj);
void gcAddAnimJointAll(GObj *gobj, AObjEvent32 **anim_joints, f32 anim_frame);
void gcAddDObjAnimJoint(DObj *dobj, AObjEvent32 *anim_joint, f32 anim_frame);
void gcPlayAnimAll(GObj *gobj);
void gcSetAnimSpeed(GObj *gobj, f32 anim_speed);
void efManagerCaptainEntryCarProcUpdate(GObj *effect_gobj);
GObj *itFFlowerWeaponFlameMakeWeapon(GObj *fighter_gobj, Vec3f *pos, Vec3f *vel);
extern ITDesc dITFFlowerItemDesc;

#ifndef lbRelocGetFileData
#define lbRelocGetFileData(type, file, offset) \
    ((type)((uintptr_t)(file) + (intptr_t)(offset)))
#endif
#ifndef DObjGetStruct
#define DObjGetStruct(gobj) ((DObj *)((gobj)->obj))
#endif

/* His Clown Copter file (generated, OWN_SPECIAL_FILES). */
extern void *gNdsP4BowserSpecial2;

/* Makers that found a file or a joint missing: counted, never a fault. */
__attribute__((used)) volatile u32 gNdsP4BowserArticleMisses;

/* Remix's status ids (his action array). */
#define BOWSER_STATUS_USP_AIR 0xDF
#define BOWSER_STATUS_NSP 0xE4
#define BOWSER_STATUS_FTHROW 0xE5
#define BOWSER_STATUS_FTHROW_FALL 0xE6
#define BOWSER_STATUS_NSP_AIR 0xE7
#define BOWSER_STATUS_FTHROW_LANDING 0xE8

/* BowserUSP constants, as assembled (each a `lui` upper half). */
#define BOWSER_USP_INITIAL_SPEED 60.0F          /* INITIAL_SPEED 0x4270 */
#define BOWSER_USP_GRAVITY 1.125F               /* GRAVITY 0x3F90 */
#define BOWSER_USP_X_ACCEL 0.03125F             /* X_ACCELERATION 0x3D00 */
#define BOWSER_USP_MAX_X 40.0F                  /* MAX_X_SPEED 0x4220 */
#define BOWSER_USP_GROUND_ACCEL_BITS 0x3D5DCCCDu /* lui 0x3D5D; ori 0xCCCD */
#define BOWSER_USP_GROUND_MAX 44.0F             /* lui 0x4230 */
#define BOWSER_USP_GROUND_MAX_SLOW 10.0F        /* lui 0x4120 */

/* His flame breath: Fire Flower shooting with his own ammo. */
#define BOWSER_FLAME_AMMO_MAX 20
#define BOWSER_FLAME_RECHARGE_TICS 30
#define BOWSER_FLAME_RECHARGE 2
#define BOWSER_FLAME_JOINT 7
#define BOWSER_FLAME_X 120.0F                   /* lui 0x42F0 */
#define BOWSER_FLAME_OFFSET_X 100.0F            /* lui 0x42C8, times his facing */
#define BOWSER_FLAME_OFFSET_Y -200.0F           /* lui 0xC348 */
#define BOWSER_FLAME_LAND_CANCEL_FRAME 20.0F    /* lui 0x41A0 */
#define BOWSER_BUTTON_B 0x4000u                 /* Joypad.B */
#define BOWSER_EFFECT_INT 12
#define BOWSER_AMMO_INT 8
#define BOWSER_FLAME_LOOP_INDEX 5
#define BOWSER_RELEASE_LAG 20
#define BOWSER_FIRE_COUNT_MAX 0x10000
#define ITCOMMONDATA_FFLOWER_FLAME_ANGLES 0x360 /* llITCommonDataFFlowerFlameAngles */

static f32 ndsP4BowserBitsToF32(u32 bits)
{
    union { u32 u; f32 f; } v;

    v.u = bits;
    return v.f;
}

/* passive_vars' first two halfwords: [0] the recharge timer, [1] the
 * flames left. */
static u16 *ndsP4BowserFlame(FTStruct *fp)
{
    return (u16 *)(void *)&fp->passive_vars;
}

/* ---- Up special (Whirling Fortress) ---- */

/* BowserUSP.air_initial_ (air_usp): the air status with the status
 * variables zeroed, every jump spent and 60 up. */
void ndsP4BowserUSPAirInitial(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    s32 *vars = (s32 *)(void *)&fp->status_vars;
    s32 i;

    for (i = 0; i < 6; i++)
    {
        vars[i] = 0;
    }
    ftMainSetStatus(fighter_gobj, BOWSER_STATUS_USP_AIR, 0.0F, 1.0F, FTSTATUS_PRESERVE_NONE);
    fp->motion_vars.flags.flag0 = 0;
    fp->motion_vars.flags.flag1 = 0;
    fp->motion_vars.flags.flag2 = 0;
    fp->is_absorb = FALSE;
    fp->absorb_lr = 0;
    fp->is_goto_attack100 = FALSE;
    fp->is_fastfall = FALSE;
    fp->jumps_used = (u8)fp->attr->jumps_max;
    fp->physics.vel_air.y = BOWSER_USP_INITIAL_SPEED;
}

/* BowserUSP.ground_physics_ (0xDE): Donkey Kong's spin drive, slower once
 * the script sets temp variable 2. */
void ndsP4BowserUSPGroundPhysics(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    ftPhysicsApplyClampGroundVelStickRange(fp, 0,
        ndsP4BowserBitsToF32(BOWSER_USP_GROUND_ACCEL_BITS),
        (fp->motion_vars.flags.flag1 == 0) ? BOWSER_USP_GROUND_MAX :
                                             BOWSER_USP_GROUND_MAX_SLOW);
    ftPhysicsSetGroundVelTransferAir(fighter_gobj);
}

/* BowserUSP.air_physics_ (0xDF): light gravity, his own drift. */
void ndsP4BowserUSPAirPhysics(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    FTAttributes *attr = fp->attr;

    ftPhysicsApplyGravityClampTVel(fp, BOWSER_USP_GRAVITY, attr->tvel_base);
    ftPhysicsClampAirVelXStickRange(fp, 8, BOWSER_USP_X_ACCEL, BOWSER_USP_MAX_X);
    ftPhysicsApplyAirVelXFriction(fp, attr);
}

/* ---- Down special (Bowser Bomb) ---- */

/* BowserDSP.air_physics_ (0xE2): Yoshi's bomb fall with the top joint
 * kept round (every axis the z scale), then the animation's own drop. */
void ndsP4BowserDSPAirPhysics(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    DObj *topn = fp->joints[nFTPartsJointTopN];

    topn->scale.vec.f.y = topn->scale.vec.f.z;
    topn->scale.vec.f.x = topn->scale.vec.f.z;
    ftPhysicsApplyAirVelTransNYZ(fighter_gobj);
}

/* ---- Forward throw (Kirby's jumping throw, his statuses) ---- */

/* BowserFThrow.transition_1_: the fall, the victim mortal again. */
static void ndsP4BowserFThrowFallSetStatus(GObj *fighter_gobj)
{
    FTStruct *this_fp = ftGetStruct(fighter_gobj);
    FTStruct *catch_fp = ftGetStruct(this_fp->catch_gobj);

    ftMainSetStatus(fighter_gobj, BOWSER_STATUS_FTHROW_FALL, 0.0F, 1.0F,
                    FTSTATUS_PRESERVE_TEXTUREPART);
    catch_fp->is_ignore_dead = FALSE;
}

/* BowserFThrow.transition_2_: the landing slam. */
static void ndsP4BowserFThrowLandingSetStatus(GObj *fighter_gobj)
{
    mpCommonSetFighterGround(ftGetStruct(fighter_gobj));
    ftMainSetStatus(fighter_gobj, BOWSER_STATUS_FTHROW_LANDING, 0.0F, 1.0F,
                    FTSTATUS_PRESERVE_TEXTUREPART);
}

/* BowserFThrow.main_ (0xE5 update). */
void ndsP4BowserFThrowMain(GObj *fighter_gobj)
{
    ftAnimEndCheckSetStatus(fighter_gobj, ndsP4BowserFThrowFallSetStatus);
}

/* BowserFThrow.collision_ (0xE5/0xE6 map): slam on landing while falling. */
void ndsP4BowserFThrowMap(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if ((mpCommonCheckFighterLanding(fighter_gobj) != FALSE) &&
        (fp->physics.vel_air.y < 0.0F))
    {
        ndsP4BowserFThrowLandingSetStatus(fighter_gobj);
    }
}

/* ---- Neutral special (flame breath) ----
 *
 * The Fire Flower's shooting (ftCommonFireFlowerShootProcAccessory and
 * UpdateAmmoStats) run from the status update, on B, with his own ammo,
 * flame origin (joint 7, 120 forward, then 100 ahead and 200 down in the
 * world) and smoke. The donor picks the first flame's ammo cost from a word
 * of its own stack (`lw t7, 0xB24(sp)`, meant to be the shot count at
 * 0xB24(s0)): a word far up the caller's frames, not zero there, so every
 * flame costs one. */

/* BowserNSP.ground_initial_ / air_initial_: the Fire Flower's shooting
 * variables, then the status and its events. */
static void ndsP4BowserNSPInitial(GObj *fighter_gobj, s32 status_id)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    ftCommonFireFlowerStatusVars *ff = &fp->status_vars.common.fireflower;

    fp->motion_vars.flags.flag0 = 0;
    ff->flame_vel_index = 0;
    ff->ammo_sub = 1;
    ff->effect_make_int = 1;
    ff->ammo_fire_count = 0;
    ff->is_release = FALSE;
    ff->release_lag = 0;
    ftMainSetStatus(fighter_gobj, status_id, 0.0F, 1.0F, FTSTATUS_PRESERVE_NONE);
    ftMainPlayAnimEventsAll(fighter_gobj);
}

void ndsP4BowserNSPGroundInitial(GObj *fighter_gobj)
{
    ndsP4BowserNSPInitial(fighter_gobj, BOWSER_STATUS_NSP);
}

void ndsP4BowserNSPAirInitial(GObj *fighter_gobj)
{
    ndsP4BowserNSPInitial(fighter_gobj, BOWSER_STATUS_NSP_AIR);
}

/* projectile_spawn and hitbox_generation: the Fire Flower's flame at the
 * index's angle, from the item file's angle table, with his offset; the
 * ammo it costs. */
static void ndsP4BowserFlameSpawn(GObj *fighter_gobj, Vec3f *pos, s32 index,
                                  s32 ammo_sub)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    u16 *flame = ndsP4BowserFlame(fp);
    const f32 *angle;
    Vec3f vel;

    if ((dITFFlowerItemDesc.p_file == NULL) || (*dITFFlowerItemDesc.p_file == NULL))
    {
        gNdsP4BowserArticleMisses++;
        return;
    }
    angle = (const f32 *)((uintptr_t)*dITFFlowerItemDesc.p_file +
                          ITCOMMONDATA_FFLOWER_FLAME_ANGLES);
    vel.x = cosf(angle[index]) * ITFFLOWER_AMMO_VEL;
    vel.y = sinf(angle[index]) * ITFFLOWER_AMMO_VEL;
    vel.z = 0.0F;

    pos->x += (f32)fp->lr * BOWSER_FLAME_OFFSET_X;
    pos->y += BOWSER_FLAME_OFFSET_Y;
    itFFlowerWeaponFlameMakeWeapon(fighter_gobj, pos, &vel);

    flame[1] = (u16)(flame[1] - ammo_sub);
}

/* BowserNSP_part2_: a flame when the ammo covers it, then the shot count,
 * the angle step and the attack id once a sweep ends. */
static void ndsP4BowserFlameShoot(FTStruct *fp, s32 ammo_sub)
{
    ftCommonFireFlowerStatusVars *ff = &fp->status_vars.common.fireflower;
    u16 *flame = ndsP4BowserFlame(fp);

    if ((s32)flame[1] >= ammo_sub)
    {
        f32 size_mul = 1.0F / fp->attr->size;
        Vec3f pos;
        s32 index;

        pos.x = 60.0F * size_mul;
        pos.y = 100.0F * size_mul;
        pos.z = BOWSER_FLAME_X * size_mul;

        if (fp->joints[BOWSER_FLAME_JOINT] == NULL)
        {
            gNdsP4BowserArticleMisses++;
        }
        else
        {
            gmCollisionGetFighterPartsWorldPosition(fp->joints[BOWSER_FLAME_JOINT], &pos);

            index = ff->flame_vel_index;
            if (index >= BOWSER_FLAME_LOOP_INDEX)
            {
                index = BOWSER_AMMO_INT - index;
            }
            ndsP4BowserFlameSpawn(fp->fighter_gobj, &pos, index, ammo_sub);
            ftParamMakeRumble(fp, 6, 0);
        }
    }
    ff->ammo_fire_count++;

    if (ff->ammo_fire_count > BOWSER_FIRE_COUNT_MAX)
    {
        ff->ammo_fire_count = BOWSER_FIRE_COUNT_MAX;
    }
    ff->flame_vel_index++;

    if (ff->flame_vel_index >= BOWSER_AMMO_INT)
    {
        ff->flame_vel_index = 0;

        ftParamSetMotionID(fp, nFTMotionAttackIDFireFlowerShoot);
        ftParamSetStatUpdate(fp, fp->stat_flags.halfword);
        ftParamUpdate1PGameAttackStats(fp, 0);
    }
}

/* BowserNSP.main_ (0xE4/0xE7 update). */
void ndsP4BowserNSPMain(GObj *fighter_gobj)
{
    FTStruct *fp;
    ftCommonFireFlowerStatusVars *ff;
    s32 ammo_sub = 1;

    ftAnimEndCheckSetStatus(fighter_gobj, mpCommonSetFighterWaitOrFall);

    fp = ftGetStruct(fighter_gobj);
    ff = &fp->status_vars.common.fireflower;

    if (!(fp->input.pl.button_hold & fp->input.button_mask_b))
    {
        ff->is_release = TRUE;
    }
    if (ff->release_lag < BOWSER_RELEASE_LAG)
    {
        ff->release_lag++;
    }
    if ((ff->release_lag < BOWSER_RELEASE_LAG) &&
        (fp->input.pl.button_tap & fp->input.button_mask_b))
    {
        ff->release_lag = 0;
    }
    if (fp->motion_vars.flags.flag0 != 0)
    {
        ff->effect_make_int--;

        if (ff->effect_make_int == 0)
        {
            ff->effect_make_int = BOWSER_EFFECT_INT;

            if (ndsP4BowserFlame(fp)[1] == 0)
            {
                /* Out of flames: smoke at his mouth and the empty burn. */
                Vec3f smoke = { 81.0F, 304.0F, 260.0F };

                ftParamMakeEffect(fighter_gobj, nEFKindDustLight,
                                  fp->attr->joint_itemlight_id, &smoke, NULL,
                                  -fp->lr, TRUE, FALSE);
                func_800269C0_275C0(nSYAudioFGMFireFlowerBurn);
            }
            else
            {
                Vec3f smoke = { 0.0F, 0.0F, -180.0F };

                ftParamMakeEffect(fighter_gobj, nEFKindDustLight,
                                  nFTPartsJointTopN, &smoke, NULL, fp->lr,
                                  FALSE, FALSE);
                func_800269C0_275C0(nSYAudioFGMBurnE);
            }
        }
        ff->ammo_sub--;

        if (ff->ammo_sub == 0)
        {
            ff->ammo_sub = BOWSER_AMMO_INT;

            ndsP4BowserFlameShoot(fp, ammo_sub);
        }
        if (fp->motion_vars.flags.flag0 == 1)
        {
            if ((s32)ndsP4BowserFlame(fp)[1] >= ammo_sub)
            {
                Vec3f dust = { 0.0F, 0.0F, -180.0F };

                ftParamMakeEffect(fighter_gobj, nEFKindDustDashSmall,
                                  nFTPartsJointTopN, &dust, NULL, fp->lr,
                                  FALSE, FALSE);
            }
            fp->motion_vars.flags.flag0 = 2;

            gcSetAnimSpeed(fighter_gobj, 0.0F);
        }
    }
    if ((ff->ammo_fire_count >= 5) && (ff->is_release != FALSE) &&
        (ff->release_lag >= BOWSER_RELEASE_LAG))
    {
        fp->motion_vars.flags.flag0 = 0;

        gcSetAnimSpeed(fighter_gobj, 1.0F);
    }
}

/* BowserNSP.air_to_ground_: the ground breath at the same frame and speed. */
static void ndsP4BowserNSPAirToGround(GObj *fighter_gobj)
{
    mpCommonSetFighterGround(ftGetStruct(fighter_gobj));
    ftMainSetStatus(fighter_gobj, BOWSER_STATUS_NSP, fighter_gobj->anim_frame,
                    DObjGetStruct(fighter_gobj)->anim_speed, FTSTATUS_PRESERVE_NONE);
}

/* BowserNSP.air_collision_ (0xE7 map): landing keeps breathing while B is
 * held past frame 20; otherwise an ordinary landing. */
void ndsP4BowserNSPAirMap(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    void (*proc_landing)(GObj *) = ndsP4BowserNSPAirToGround;

    if ((fighter_gobj->anim_frame >= BOWSER_FLAME_LAND_CANCEL_FRAME) &&
        !(fp->input.pl.button_hold & BOWSER_BUTTON_B))
    {
        proc_landing = mpCommonSetFighterWaitOrLanding;
    }
    mpCommonProcFighterLanding(fighter_gobj, proc_landing);
}

/* bowser.asm bowser_nsp_recharge: two flames every 30 frames outside the
 * breath, until 20 (a count the costs leave odd overshoots, as there). */
static void ndsP4BowserAfterProcMap(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    u16 *flame = ndsP4BowserFlame(fp);

    flame[0]++;
    if (flame[0] != BOWSER_FLAME_RECHARGE_TICS)
    {
        return;
    }
    flame[0] = 0;

    if ((fp->status_id == BOWSER_STATUS_NSP) || (fp->status_id == BOWSER_STATUS_NSP_AIR))
    {
        return;
    }
    if (flame[1] == BOWSER_FLAME_AMMO_MAX)
    {
        return;
    }
    flame[1] = (u16)(flame[1] + BOWSER_FLAME_RECHARGE);
}

/* jigglypuffkirbyshared.asm kirby_blast_fix_1: a fall refills his flames. */
static void ndsP4BowserOnDead(GObj *fighter_gobj)
{
    ndsP4BowserFlame(ftGetStruct(fighter_gobj))[1] = BOWSER_FLAME_AMMO_MAX;
}

/* ---- Entry (the Clown Copter) ----
 *
 * captainshared.asm entry_anim_struct_BOWSER and clown_car_animation1/2:
 * the Falcon Flyer's maker on his file, its craft at 0x1E80, the flight at
 * 0x248C and the two child tracks at 0x24C0 and 0x2530. */
static EFDesc sNdsP4BowserEntryCopterEffectDesc = {
    0x4 | EFFECT_FLAG_USERDATA,                 /* 0x060A0000 */
    10,
    &gNdsP4BowserSpecial2,
    { nGCMatrixKindTraRotRpyRSca, nGCMatrixKindNull, 0x00 },
    { nGCMatrixKindTraRotRpyRSca, nGCMatrixKindNull, 0x00 },
    efManagerCaptainEntryCarProcUpdate,
    gcDrawDObjTreeDLLinksForGObj,
    0x1E80, 0x0000, 0x0000, 0x0000
};

static GObj *ndsP4BowserEntryCopterMakeEffect(Vec3f *pos, s32 lr)
{
    DObj *node_dobj;
    GObj *effect_gobj;
    DObj *dobj;
    s32 i;

    if (gNdsP4BowserSpecial2 == NULL)
    {
        gNdsP4BowserArticleMisses++;
        return NULL;
    }
    effect_gobj = efManagerMakeEffectNoForce(&sNdsP4BowserEntryCopterEffectDesc);

    if (effect_gobj == NULL)
    {
        return NULL;
    }
    dobj = DObjGetStruct(effect_gobj);

    gcAddAnimJointAll(effect_gobj,
        lbRelocGetFileData(AObjEvent32**, gNdsP4BowserSpecial2, 0x248C), 0.0F);

    node_dobj = (dobj->child != NULL) && (dobj->child->child != NULL) ?
        dobj->child->child->child : NULL;

    for (i = nFTPartsJointCommonStart; (i > 0) && (node_dobj != NULL); i--)
    {
        gcAddXObjForDObjFixed(node_dobj, nGCMatrixKindRecalcRotRpyRSca, 0);
        gcAddDObjAnimJoint(node_dobj,
            lbRelocGetFileData(AObjEvent32*, gNdsP4BowserSpecial2, 0x24C0), 0.0F);

        node_dobj = node_dobj->sib_next;
        if (node_dobj == NULL)
        {
            break;
        }
        gcAddDObjAnimJoint(node_dobj,
            lbRelocGetFileData(AObjEvent32*, gNdsP4BowserSpecial2, 0x2530), 0.0F);

        node_dobj = node_dobj->sib_next;
    }
    if (i > 0)
    {
        gNdsP4BowserArticleMisses++;
    }
    gcPlayAnimAll(effect_gobj);

    dobj->translate.vec.f = *pos;

    if (lr == -1)
    {
        dobj->rotate.vec.f.y = F_CLC_DTOR32(180.0F);
    }
    if (DObjGetStruct(effect_gobj)->rotate.vec.f.y == F_CLC_DTOR32(0.0F))
    {
        efManagerSortZNeg(dobj->child);
    }
    else efManagerSortZPos(dobj->child);

    return effect_gobj;
}

/* The Falcon Flyer's case of ftCommonAppearSetStatus, which his
 * entry_script names: arriving from the far side flips him, and the craft
 * is his. */
static void ndsP4BowserEntryCase(FTStruct *fp)
{
    if (fp->status_vars.common.entry.lr == -1)
    {
        fp->status_vars.common.entry.is_rotate = TRUE;
    }
    ndsP4BowserEntryCopterMakeEffect(&fp->entry_pos, fp->status_vars.common.entry.lr);
}

const NDSP4Overrides gNdsP4BowserOverrides = {
    .entry_case = ndsP4BowserEntryCase,
    .throw_f_kirby_status = BOWSER_STATUS_FTHROW,
    .after_proc_map = ndsP4BowserAfterProcMap,
    .on_dead = ndsP4BowserOnDead,
    .no_yoshi_lw_stars = TRUE,
};

#endif /* NDS_P4_BOWSER */
