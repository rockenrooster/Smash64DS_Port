/*
 * P4 Snake (EXTRA): native ports of the donor's special routines, his four
 * items and his up smash's mortar, and of the patches EXTRA makes on his
 * character id. Source: joaorb64/smashremix-plus-extra 96621af,
 * extra_characters/Snake (SnakeSpecial.asm, C4.asm, Grenade.asm,
 * Nikita.asm, Cypher.asm, AI.asm, main.asm), read as assembled
 * (scripts/p4/mipsdis.py). He runs Captain Falcon's status code
 * (fp->fkind == nFTKindCaptain): the C4 (down special), the grenades
 * (neutral), the Nikita (forward smash), the Cypher (up), a crawl, a dash
 * attack into his crouch, a two-part forward tilt, the mortar up smash and
 * five codec taunts.
 *
 * Player-struct offsets in the donor map to BattleShip fields as:
 *   0x001C "current frame (int)"            status_total_tics
 *   0x0024 action                          status_id
 *   0x0030 / 0x0040 / 0x0044               damage_resist / hitlag_tics / lr
 *   0x004C / 0x0060                        vel_air.y / vel_ground.x
 *   0x0078 position (a pointer)            coll_data.p_translate
 *   0x008C "player x speed"                coll_data.pos_diff.x
 *   0x00B4 / 0x00BC                        coll_data.map_coll center / width
 *   0x00EC / 0x00F4 / 0x0144               coll_data.floor_line_id /
 *                                          floor_flags / ignore_line_id
 *   0x0148 / 0x014C                        jumps_used / ga
 *   0x017C/0x180/0x184 temp variables 1-3  motion_vars.flags.flag0/1/2
 *   0x018D bits 0xF8                       is_absorb, absorb_lr,
 *                                          is_goto_attack100, is_fastfall
 *   0x0191 bit 0x20                        is_damage_resist
 *   0x01BC / 0x01BE / 0x01C0               input.pl.button_hold / tap /
 *                                          release
 *   0x01C2 / 0x01C3                        input.pl.stick_range.x / .y
 *   0x01C6 / 0x01C8 / 0x01C9               input.cp.button_inputs /
 *                                          stick_range.x / .y
 *   0x01D4 / 0x01FC / 0x01CC+0x4C..0x58    computer.p_command / target_gobj /
 *                                          cliff_left_pos, cliff_right_pos
 *   0x0269                                 tap_stick_y
 *   0x084C / 0x08E8 / 0x0928 / 0x092C      item_gobj / joints[0] / [16] / [17]
 *   0x09C8 (+0x24, 0x34-0x40, 0x64, 0xB0)  attr (traction, kneebend_anim_length,
 *                                          jump_vel_x, jump_height_mul / _base,
 *                                          jumps_max, cliffcatch_coll.y)
 *   0x0ADC C4 item struct                  passive_vars word 0
 *   0x0AE0 Nikita item struct              passive_vars word 1
 *   0x0AE4 up special bool                 passive_vars word 2
 *   0x0AE8 grenades out                    passive_vars word 3
 *   0x0B18 - 0x0B24                        status_vars.common.kneebend
 *
 * The items keep BattleShip's N64 item layout (nds_p4_peach.c asserts it):
 * the donor's custom words (0x1C4 its maker, 0x1C8-0x1DC, 0x350-0x35C) are
 * written by offset, its bitfield bytes by field: 0x2CE bit 0x80
 * is_allow_pickup, 0x2CE bits 0x30 times_landed, 0x2CF bit 0x40
 * is_attach_surface, 0x2CF bit 0x20 is_thrown, 0x158 bit 0x10
 * attack_coll.can_rehit_shield, 0x158 bit 0x80 attack_coll.can_setoff.
 * Writes the donor makes into a DObj's rotate.xobj (0x28, meant as the
 * translation's z) and into the weapon's attack_pos (0x158, meant as its
 * reflect and absorb bits) reach fields nothing reads or that the next hit
 * update rewrites, and are not made here.
 */
#include <nds/nds_p4.h>

#if NDS_P4_SNAKE

#include <ef/effect.h>
#include <gr/ground.h>
#include <if/interface.h>
#include <it/item.h>
#include <sc/scene.h>
#include <sys/audio.h>
#include <wp/weapon.h>
#include <math.h>

#ifndef DObjGetStruct
#define DObjGetStruct(gobj) ((DObj *)((gobj)->obj))
#endif
#ifndef DOBJ_FLAG_HIDDEN
#define DOBJ_FLAG_HIDDEN (1 << 1)
#endif

GObj *itManagerMakeItem(GObj *parent_gobj, ITDesc *item_desc, Vec3f *pos, Vec3f *vel, u32 flags);
void mpCommonRunItemCollisionDefault(GObj *item_gobj, Vec3f *pos, MPCollData *coll_data);
void itLinkBombCommonSetHitStatusNormal(GObj *item_gobj);
void itMSBombAttachedUpdateSurface(GObj *item_gobj);
sb32 itLGunWeaponAmmoProcHit(GObj *weapon_gobj);
void gcSetAnimSpeed(GObj *gobj, f32 anim_speed);
/* ftCommonSquatWaitSetStatus, as battleship_ftcommon_normal_moveset.c
 * compiles it. */
void ndsBaseFTCommonSquatWaitSetStatus(GObj *fighter_gobj);
void ftCommonSquatWaitProcInterrupt(GObj *fighter_gobj);
sb32 ftCommonPassCheckInputSuccess(FTStruct *fp);
LBParticle *efManagerDustExpandSmallMakeEffect(Vec3f *pos, f32 f_index);
LBParticle *efManagerSparkleWhiteMultiExplodeMakeEffect(Vec3f *pos);
GObj *efManagerQuakeMakeEffect(s32 magnitude);
LBParticle *efManagerFuraSparkleMakeEffect(Vec3f *pos);
GObj *efManagerItemGetSwirlProcUpdate(Vec3f *pos);
s32 syUtilsRandIntRange(s32 range);
f32 syUtilsArcTan2(f32 y, f32 x);
f32 __sinf(f32);
f32 __cosf(f32);
extern u8 *dFTComputerPlayerInputScripts[];

extern void *gNdsP4SnakeSpecial1;
extern void *gNdsP4SnakeSpecial2;

static f32 ndsP4SnakeBitsToF32(u32 bits)
{
    union { u32 u; f32 f; } v;

    v.u = bits;
    return v.f;
}

/* The donors test a float's word against zero (beqz, bnez): -0.0 counts as
 * set. */
static u32 ndsP4SnakeF32Bits(f32 value)
{
    union { u32 u; f32 f; } v;

    v.f = value;
    return v.u;
}

static s32 *ndsP4SnakePassiveWord(FTStruct *fp, u32 index)
{
    return &((s32 *)(void *)&fp->passive_vars)[index];
}

static s32 *ndsP4SnakeItemWord(ITStruct *ip, u32 n64_offset)
{
    return (s32 *)(void *)((u8 *)ip + n64_offset);
}

#define SNAKE_ITEM_F32(ip_, off_) (*(f32 *)(void *)ndsP4SnakeItemWord((ip_), (off_)))

/* main.asm's Action scope and his added actions, as exported. */
#define SNAKE_STATUS_CROUCHF 0xF2
#define SNAKE_STATUS_CROUCHB 0xF3
#define SNAKE_STATUS_TILTF2 0xF5
#define SNAKE_STATUS_GRENADE_START 0xF6
#define SNAKE_STATUS_GRENADE_WAIT 0xF7
#define SNAKE_STATUS_GRENADE_THROW_N 0xF8
#define SNAKE_STATUS_GRENADE_THROW_EMPTY 0xF9
#define SNAKE_STATUS_GRENADE_THROW_F 0xFA
#define SNAKE_STATUS_GRENADE_THROW_B 0xFB
#define SNAKE_STATUS_GRENADE_WALK_F 0xFC
#define SNAKE_STATUS_GRENADE_WALK_B 0xFD
#define SNAKE_STATUS_GRENADE_JUMPSQUAT 0xFE
#define SNAKE_STATUS_GRENADE_START_AIR 0xFF
#define SNAKE_STATUS_GRENADE_WAIT_AIR 0x100
#define SNAKE_STATUS_GRENADE_THROW_N_AIR 0x101
#define SNAKE_STATUS_GRENADE_THROW_EMPTY_AIR 0x102
#define SNAKE_STATUS_GRENADE_THROW_F_AIR 0x103
#define SNAKE_STATUS_GRENADE_THROW_B_AIR 0x104
#define SNAKE_STATUS_C4_START 0x105
#define SNAKE_STATUS_C4_GROUND 0x106
#define SNAKE_STATUS_C4_WALL 0x107
#define SNAKE_STATUS_C4_ENEMY 0x108
#define SNAKE_STATUS_C4_DETONATE 0x109
#define SNAKE_STATUS_C4_AIR_START 0x10A
#define SNAKE_STATUS_C4_AIR_GROUND 0x10B
#define SNAKE_STATUS_C4_AIR_WALL 0x10C
#define SNAKE_STATUS_C4_AIR_ENEMY 0x10D
#define SNAKE_STATUS_C4_AIR_DETONATE 0x10E
#define SNAKE_STATUS_NIKITA_START 0x10F
#define SNAKE_STATUS_NIKITA_OPERATION 0x110
#define SNAKE_STATUS_NIKITA_END 0x111
#define SNAKE_STATUS_NIKITA_SUCCESS 0x112
#define SNAKE_STATUS_NIKITA_AIR_OPERATION 0x114
#define SNAKE_STATUS_NIKITA_AIR_END 0x115
#define SNAKE_STATUS_NIKITA_AIR_SUCCESS 0x116
#define SNAKE_STATUS_CYPHER_START 0x117
#define SNAKE_STATUS_CYPHER_AIR_START 0x118
#define SNAKE_STATUS_CYPHER_AIR_HANG 0x119
#define SNAKE_STATUS_CYPHER_AIR_CUT 0x11A
#define SNAKE_STATUS_TAUNT_MEILING 0x11D
#define SNAKE_STATUS_TAUNT_OTACON 0x11E
#define SNAKE_STATUS_TAUNT_FALCON 0x11F
#define SNAKE_STATUS_TAUNT_SLIPPY 0x120

#define SNAKE_CONTENT 18u                    /* scripts/p4/contents.json */
#define SNAKE_BUTTON_A 0x8000u               /* Joypad.A */
#define SNAKE_BUTTON_B 0x4000u               /* Joypad.B */
#define SNAKE_BUTTON_Z 0x2000u               /* Joypad.Z */
#define SNAKE_JOINT_HAND 16                  /* 0x0928 */
#define SNAKE_JOINT_ITEM_LIGHT 17            /* 0x092C */
#define SNAKE_FGM_EXPLODE_S 0x00
#define SNAKE_FGM_EXPLODE 0x01
#define SNAKE_FGM_C4_ATTACH 0x2C             /* proximity bomb attach */
#define SNAKE_FGM_PUNCH_S 0x28
#define SNAKE_FGM_KICK_M 0x20                /* FGM.hit.KICK_M */
#define SNAKE_FGM_NONE 0x2B7
#define SNAKE_ATTACK_EVENTS 0x88             /* llLinkMainBombAttackEvents */

_Static_assert(nFTCommonStatusWait == 0x0A, "Action.Idle");
_Static_assert(nGCCommonKindFighter == 0x3E8, "player object type");
_Static_assert((MAP_FLAG_MAIN_MASK == 0x0C21) && (MAP_FLAG_FLOOR == 0x0800), "0x0C21, 0x0800");
_Static_assert(nFTCommonStatusDamageFall == 0x39, "Action.Tumble");
_Static_assert(offsetof(ITStruct, item_vars) == 0x350, "ITStruct is not N64's layout");
_Static_assert(offsetof(ITStruct, attack_coll.attack_pos[1]) == 0x1C4, "ITStruct is not N64's layout");

/* The status change Remix's routines make: frame, speed 1.0, flags 0, then
 * the motion's frame-0 events ("unknown common subroutine" 0x800E0830). */
static void ndsP4SnakeSetStatus(GObj *fighter_gobj, s32 status_id)
{
    ftMainSetStatus(fighter_gobj, status_id, 0.0F, 1.0F, FTSTATUS_PRESERVE_NONE);
    ftMainPlayAnimEventsAll(fighter_gobj);
}

/* An air_to_ground: grounded, the grounded status from the same frame. Its
 * status change's fifth argument is a stack word the donor never writes;
 * the DS preserves nothing. */
static void ndsP4SnakeAirToGround(GObj *fighter_gobj, s32 status_id)
{
    mpCommonSetFighterGround(ftGetStruct(fighter_gobj));
    ftMainSetStatus(fighter_gobj, status_id, fighter_gobj->anim_frame, 1.0F,
                    FTSTATUS_PRESERVE_NONE);
}

/* A spawn point ahead of and above his top joint, scaled by its x scale:
 * the grenade's and the Nikita's (280, 225), the mortar's (50, 200). */
static void ndsP4SnakeSpawnPos(FTStruct *fp, f32 ahead, f32 up, Vec3f *pos)
{
    f32 scale = fp->joints[nFTPartsJointTopN]->scale.vec.f.x;

    pos->x = fp->coll_data.p_translate->x + (ahead * (scale * (f32)fp->lr));
    pos->y = fp->coll_data.p_translate->y + (up * scale);
    pos->z = 0.0F;
}

/* A hand point: joint 16's world position, z 0. */
static void ndsP4SnakeHandPos(FTStruct *fp, Vec3f *pos)
{
    pos->x = pos->y = pos->z = 0.0F;
    gmCollisionGetFighterPartsWorldPosition(fp->joints[SNAKE_JOINT_HAND], pos);
    pos->z = 0.0F;
}

/* Every item's stage_setting_: the maker gets no flags (the caller's
 * 0x80000002 sits in a frame the maker never reads), and the setting ends
 * with the item's map collision from Snake's position. */
static GObj *ndsP4SnakeMakeItem(GObj *fighter_gobj, ITDesc *desc, Vec3f *pos, Vec3f *vel)
{
    if ((desc->p_file == NULL) || (*desc->p_file == NULL))
    {
        return NULL;
    }
    return itManagerMakeItem(fighter_gobj, desc, pos, vel, 0);
}

static void ndsP4SnakeItemCollideFromOwner(GObj *item_gobj, FTStruct *fp)
{
    mpCommonRunItemCollisionDefault(item_gobj, fp->coll_data.p_translate, &fp->coll_data);
}

/* The donors' explosion_hitboxes_: Link's bomb event update
 * (itLinkBombExplodeUpdateAttackEvent past its file load) on the item's
 * own file, at Link's offset 0x88: the hitbox files are copies of the
 * bomb's, events and all. */
static void ndsP4SnakeExplodeUpdateAttackEvent(GObj *item_gobj, ITDesc *desc)
{
    ITStruct *ip = itGetStruct(item_gobj);
    ITAttackEvent *ev = ndsItGetAttackEvent(desc, (const void *)SNAKE_ATTACK_EVENTS);

    if ((ev != NULL) && (ip->multi == ev[ip->event_id].timer))
    {
        ip->attack_coll.angle = ev[ip->event_id].angle;
        ip->attack_coll.damage = ev[ip->event_id].damage;
        ip->attack_coll.size = ev[ip->event_id].size;
        ip->attack_coll.can_rehit_item = TRUE;
        ip->attack_coll.can_hop = FALSE;
        ip->attack_coll.can_reflect = FALSE;
        ip->attack_coll.can_setoff = FALSE;
        ip->attack_coll.element = nGMHitElementFire;
        ip->event_id++;

        if (ip->event_id == 4)
        {
            ip->event_id = 3;
        }
    }
}

/* begin_explosion_'s head (Link's bomb 0x8018656C/0x801864E8): the event
 * count and timer cleared, the throw multiplier 1. */
static void ndsP4SnakeExplodeInit(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    ip->event_id = 0;
    ip->multi = 0;
    ip->attack_coll.throw_mul = 1.0F;
}

/* explosion_'s shared middle: the explosion effect, a quake, the item's
 * DObj hidden (flags 2) and its hit sound 1, its hitbox refreshed. */
static LBParticle *ndsP4SnakeExplodeEffects(GObj *item_gobj)
{
    LBParticle *pc =
        efManagerSparkleWhiteMultiExplodeMakeEffect(&DObjGetStruct(item_gobj)->translate.vec.f);

    efManagerQuakeMakeEffect(1);
    return pc;
}

static void ndsP4SnakeExplodeHide(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    DObjGetStruct(item_gobj)->flags = DOBJ_FLAG_HIDDEN;
    ip->attack_coll.fgm_id = 1;
    itMainRefreshAttackColl(item_gobj);
}

/* detect_collision_ (grenade, Nikita): itMapProcAllCheckCollisionFlag
 * without its landing's edge adjust, sound and spin reset. */
static sb32 ndsP4SnakeItemDetectCollision(MPCollData *coll_data, GObj *item_gobj, u32 coll_flags)
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

/* ---- Crawl (CROUCH, CROUCHF, CROUCHB) ---- */

/* Crawl.interrupt: SquatWait's interrupts (any of them changing his status
 * is the donor's "returned 1": they all set another status), then the
 * stick: forward past 8 crawls forward, back past -8 crawls back, else the
 * crouch (CrouchIdle, his CROUCH), from frame 1 when it changes. */
void ndsP4SnakeCrawlInterrupt(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    s32 status_id = fp->status_id;
    s32 stick;
    s32 next;

    ftCommonSquatWaitProcInterrupt(fighter_gobj);

    if (fp->status_id != status_id)
    {
        return;
    }
    stick = fp->input.pl.stick_range.x * fp->lr;

    if (stick >= 8)
    {
        next = SNAKE_STATUS_CROUCHF;
    }
    else if (stick < -8)
    {
        next = SNAKE_STATUS_CROUCHB;
    }
    else next = nFTCommonStatusSquatWait;

    if (fp->status_id != next)
    {
        ftMainSetStatus(fighter_gobj, next, 1.0F, 1.0F, FTSTATUS_PRESERVE_NONE);
    }
}

/* Crawl.physics: the stick's ground speed at 0.2 with his traction, the
 * ground-to-air transfer, and the animation at a tenth of his speed (the
 * donor's 0x3DCC0000). */
void ndsP4SnakeCrawlPhysics(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    ftPhysicsSetGroundVelStickRange(fp, ndsP4SnakeBitsToF32(0x3E4CCCCDu), fp->attr->traction);
    ftPhysicsSetGroundVelTransferAir(fighter_gobj);
    gcSetAnimSpeed(fighter_gobj, ABSF(fp->physics.vel_ground.x) * ndsP4SnakeBitsToF32(0x3DCC0000u));
}

/* DashAttack.main: into his crouch at the end. */
void ndsP4SnakeDashAttackUpdate(GObj *fighter_gobj)
{
    ftAnimEndCheckSetStatus(fighter_gobj, ndsBaseFTCommonSquatWaitSetStatus);
}

/* ---- Forward tilt (TILTF, TILTF2) ---- */

/* TiltF.main: A pressed at any point (temp variable 3, cleared on frame 2)
 * past frame 11 goes into the second kick; else wait at the end. */
void ndsP4SnakeTiltFUpdate(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    f32 frame = fighter_gobj->anim_frame;

    if (frame == 2.0F)
    {
        fp->motion_vars.flags.flag2 = 0;
    }
    if (fp->input.pl.button_tap & SNAKE_BUTTON_A)
    {
        fp->motion_vars.flags.flag2 = 1;
    }
    if ((11.0F < frame) && (fp->motion_vars.flags.flag2 != 0))
    {
        ftMainSetStatus(fighter_gobj, SNAKE_STATUS_TILTF2, 0.0F, 1.0F, FTSTATUS_PRESERVE_NONE);
        return;
    }
    ftAnimEndSetWait(fighter_gobj);
}

/* ---- Taunt (TAUNT_COLONEL and the other calls) ---- */

/* Taunt.main: on frame 2 a call, one in eight each: the Colonel (two,
 * staying), Mei Ling (two), Otacon (two), Falcon, Slippy, from the same
 * frame; wait at the end. */
void ndsP4SnakeTauntUpdate(GObj *fighter_gobj)
{
    if (fighter_gobj->anim_frame == 2.0F)
    {
        static const u16 calls[8] = {
            0, 0, SNAKE_STATUS_TAUNT_MEILING, SNAKE_STATUS_TAUNT_MEILING,
            SNAKE_STATUS_TAUNT_OTACON, SNAKE_STATUS_TAUNT_OTACON,
            SNAKE_STATUS_TAUNT_FALCON, SNAKE_STATUS_TAUNT_SLIPPY
        };
        s32 call = calls[syUtilsRandIntRange(8)];

        if (call != 0)
        {
            ftMainSetStatus(fighter_gobj, call, fighter_gobj->anim_frame, 1.0F,
                            FTSTATUS_PRESERVE_NONE);
            ftMainPlayAnimEventsAll(fighter_gobj);
        }
    }
    ftAnimEndSetWait(fighter_gobj);
}

/* ---- Up smash (SMASHU) and its mortar ---- */

#define SNAKE_MORTAR_LIFETIME 256
#define SNAKE_MORTAR_SIZE 140.0F
#define SNAKE_MORTAR_SCALE 0.75F
#define SNAKE_MORTAR_VEL_X 6.0F
#define SNAKE_MORTAR_VEL_Y 120.0F
#define SNAKE_MORTAR_GRAVITY 4.0F
#define SNAKE_MORTAR_TVEL (-120.0F)
#define SNAKE_MORTAR_ECB_Y 120.0F
#define SNAKE_MORTAR_ECB_WIDTH 56.0F
#define SNAKE_MORTAR_SMOKE_Y 50.0F
#define SNAKE_MORTAR_BLAST_SIZE 300.0F
#define SNAKE_MORTAR_BLAST_DAMAGE 14
#define SNAKE_MORTAR_BLAST_ANGLE 65
#define SNAKE_MORTAR_BLAST_BKB 87
#define SNAKE_MORTAR_BLAST_KBS 80
#define SNAKE_MORTAR_BLAST_LIFETIME 5

/* Temp variable 2 (weapon_vars word 1): set once it has burst. */
static s32 *ndsP4SnakeMortarBurst(WPStruct *wp)
{
    return &((s32 *)(void *)&wp->weapon_vars)[1];
}

/* USmash.projectile_update: gravity 4 down to -120, a smoke puff every
 * fourth frame 50 above it along its course, its model turned to its
 * course. */
static sb32 ndsP4SnakeMortarProcUpdate(GObj *weapon_gobj)
{
    WPStruct *wp = wpGetStruct(weapon_gobj);
    DObj *dobj = DObjGetStruct(weapon_gobj);

    if (wpMainDecLifeCheckExpire(wp) != FALSE)
    {
        return TRUE;
    }
    if (wp->physics.vel_air.y < SNAKE_MORTAR_TVEL)
    {
        wp->physics.vel_air.y = SNAKE_MORTAR_TVEL;
    }
    else wp->physics.vel_air.y -= SNAKE_MORTAR_GRAVITY;

    if ((wp->lifetime & 3) == 0)
    {
        f32 theta = -dobj->rotate.vec.f.z;
        f32 c = __cosf(theta);
        f32 s = __sinf(theta);
        Vec3f pos;

        pos.x = dobj->translate.vec.f.x + ((0.0F * c) + (SNAKE_MORTAR_SMOKE_Y * s));
        pos.y = dobj->translate.vec.f.y + ((SNAKE_MORTAR_SMOKE_Y * c) - (0.0F * s));
        pos.z = dobj->translate.vec.f.z;
        efManagerDustExpandSmallMakeEffect(&pos, 0.0F);
    }
    wpMainVelSetModelPitch(weapon_gobj);
    wpMainReflectorRotateWeaponModel(weapon_gobj);

    return FALSE;
}

/* USmash.projectile_collision (hit, shield, hop, and the map's contact):
 * once, it bursts where it is: five more frames, a fresh 300-wide fire
 * hitbox (14 damage, 65 degrees, 87 base, 80 growth) that clashes, still,
 * its model gone, the explosion effect, a quake and the small explosion
 * sound. */
static sb32 ndsP4SnakeMortarBurstHit(GObj *weapon_gobj)
{
    WPStruct *wp = wpGetStruct(weapon_gobj);
    DObj *dobj = DObjGetStruct(weapon_gobj);
    s32 i;

    if (*ndsP4SnakeMortarBurst(wp) == 1)
    {
        return FALSE;
    }
    *ndsP4SnakeMortarBurst(wp) = 1;
    wp->lifetime = SNAKE_MORTAR_BLAST_LIFETIME;
    wp->attack_coll.attack_state = nGMAttackStateNew;

    for (i = 0; i < 4; i++)
    {
        wp->attack_coll.attack_records[i].victim_gobj = NULL;
    }
    wp->attack_coll.size = SNAKE_MORTAR_BLAST_SIZE;
    wp->attack_coll.damage = SNAKE_MORTAR_BLAST_DAMAGE;
    wp->attack_coll.element = nGMHitElementFire;
    wp->attack_coll.angle = SNAKE_MORTAR_BLAST_ANGLE;
    wp->attack_coll.knockback_base = SNAKE_MORTAR_BLAST_BKB;
    wp->attack_coll.knockback_scale = SNAKE_MORTAR_BLAST_KBS;
    wp->attack_coll.can_setoff = TRUE;
    wp->physics.vel_air.x = 0.0F;
    wp->physics.vel_air.y = 0.0F;

    dobj->dl = NULL;
    efManagerSparkleWhiteMultiExplodeMakeEffect(&dobj->translate.vec.f);
    efManagerQuakeMakeEffect(1);
    func_800269C0_275C0(SNAKE_FGM_EXPLODE_S);

    return FALSE;
}

/* USmash.projectile_procmap: any contact bursts it. */
static sb32 ndsP4SnakeMortarProcMap(GObj *weapon_gobj)
{
    if (wpMapTestAllCheckCollEnd(weapon_gobj) != FALSE)
    {
        ndsP4SnakeMortarBurstHit(weapon_gobj);
    }
    return FALSE;
}

/* USmash.projectile_reflector: its course reversed and a fresh lifetime. */
static sb32 ndsP4SnakeMortarProcReflector(GObj *weapon_gobj)
{
    WPStruct *wp = wpGetStruct(weapon_gobj);

    wp->physics.vel_air.x = -wp->physics.vel_air.x;
    wp->physics.vel_air.y = -wp->physics.vel_air.y;
    wp->lifetime = SNAKE_MORTAR_LIFETIME;

    return FALSE;
}

/* USmash.projectile_struct: WPAttributes at 0x160 of his special file 1
 * (MERGED_FILERESOURCE_6_2); set-off and absorb take the Ray Gun shot's
 * hit (its shockwave). Remix's projectile id 0 is Mario's fireball kind;
 * the DS numbers the mortar past the source's kinds. */
static WPDesc sNdsP4SnakeMortarDesc = {
    0x00,
    NDS_P4_WP_KIND_SNAKE_MORTAR,
    &gNdsP4SnakeSpecial1,
    0x160,
    { nGCMatrixKindTra, 0x48, 0 },
    ndsP4SnakeMortarProcUpdate,
    ndsP4SnakeMortarProcMap,
    ndsP4SnakeMortarBurstHit,
    ndsP4SnakeMortarBurstHit,
    ndsP4SnakeMortarBurstHit,
    itLGunWeaponAmmoProcHit,
    ndsP4SnakeMortarProcReflector,
    itLGunWeaponAmmoProcHit
};

/* USmash.main: on his 30th frame the mortar 50 ahead and 200 up, at 0.75
 * scale, 256 frames, a silent 140-wide hitbox of 1 that neither clashes nor
 * knocks back, a 120-tall box 56 wide, 6 along his facing and 120 up; wait
 * at the end. */
void ndsP4SnakeUSmashUpdate(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if ((fp->status_total_tics == 30) && (gNdsP4SnakeSpecial1 != NULL))
    {
        Vec3f pos;
        GObj *weapon_gobj;

        ndsP4SnakeSpawnPos(fp, 50.0F, 200.0F, &pos);
        weapon_gobj = wpManagerMakeWeapon(fighter_gobj, &sNdsP4SnakeMortarDesc, &pos,
                                          WEAPON_FLAG_COLLPROJECT | WEAPON_FLAG_PARENT_FIGHTER);
        if (weapon_gobj != NULL)
        {
            WPStruct *wp = wpGetStruct(weapon_gobj);
            DObj *dobj = DObjGetStruct(weapon_gobj);

            dobj->scale.vec.f.x = SNAKE_MORTAR_SCALE;
            dobj->scale.vec.f.y = SNAKE_MORTAR_SCALE;
            wp->lifetime = SNAKE_MORTAR_LIFETIME;
            wp->attack_coll.size = SNAKE_MORTAR_SIZE;
            wp->attack_coll.damage = 1;
            wp->attack_coll.element = nGMHitElementNormal;
            wp->attack_coll.angle = 361;
            wp->attack_coll.knockback_base = 0;
            wp->attack_coll.knockback_scale = 0;
            wp->attack_coll.fgm_id = SNAKE_FGM_NONE;
            wp->attack_coll.can_setoff = FALSE;
            wp->coll_data.map_coll.top = SNAKE_MORTAR_ECB_Y;
            wp->coll_data.map_coll.bottom = -SNAKE_MORTAR_ECB_Y;
            wp->coll_data.map_coll.width = SNAKE_MORTAR_ECB_WIDTH;
            wp->physics.vel_air.x = SNAKE_MORTAR_VEL_X * (f32)fp->lr;
            wp->physics.vel_air.y = SNAKE_MORTAR_VEL_Y;
            *ndsP4SnakeMortarBurst(wp) = 0;
            wpMainVelSetModelPitch(weapon_gobj);
            wpMainReflectorRotateWeaponModel(weapon_gobj);
        }
    }
    ftAnimEndSetWait(fighter_gobj);
}

/* ---- C4 (down special) ---- */

#define SNAKE_C4_FUSE 1600
#define SNAKE_C4_SCALE_BITS 0x3ECC0000u      /* lui 0x3ECC */
#define SNAKE_C4_ECB 20.0F
#define SNAKE_C4_MAX_SPEED 63.0F
#define SNAKE_C4_GRAVITY 3.6F
#define SNAKE_C4_STICKY_RANGE 50.0F
#define SNAKE_C4_WALL_REACH 50.0F
#define SNAKE_C4_FLASH_FRAMES 30
#define SNAKE_C4_EXPLODE_FRAMES 3
#define SNAKE_C4_EXPLODE_SIZE 424.0F
#define SNAKE_C4_EXPLODE_DAMAGE 17
#define SNAKE_C4_EXPLODE_ANGLE 80
#define SNAKE_C4_EXPLODE_BKB 20
#define SNAKE_C4_EXPLODE_KBS 100
#define SNAKE_C4_DETONATE_FRAME 25.0F

/* C4.asm VARIABLES and its custom words. */
#define SNAKE_C4_STICKY 0x350               /* the FTStruct it sticks to */
#define SNAKE_C4_FRAMES_SET 0x354
#define SNAKE_ITEM_MAKER 0x1C4              /* the FTStruct that made it */
#define SNAKE_ITEM_MAX_SPEED 0x1C8
#define SNAKE_ITEM_ROT_DIR 0x1CC
#define SNAKE_ITEM_REFRESH 0x1D0
#define SNAKE_ITEM_HIT_FLAG 0x1D4

static sb32 ndsP4SnakeC4ProcUpdate(GObj *item_gobj);
static sb32 ndsP4SnakeC4ProcMap(GObj *item_gobj);
static sb32 ndsP4SnakeC4RestingProcMap(GObj *item_gobj);
static sb32 ndsP4SnakeC4ExplodeProcUpdate(GObj *item_gobj);

/* flashbang_item_states: aerial, resting, explosion. */
static ITStatusDesc sNdsP4SnakeC4StatusDescs[3] = {
    { ndsP4SnakeC4ProcUpdate, ndsP4SnakeC4ProcMap, NULL, NULL, NULL, NULL, NULL, NULL },
    { ndsP4SnakeC4ProcUpdate, ndsP4SnakeC4RestingProcMap, NULL, NULL, NULL, NULL, NULL, NULL },
    { ndsP4SnakeC4ExplodeProcUpdate, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
};

/* item_info_array: ITAttributes at 0x40 of his special file 1
 * (MERGED_FILERESOURCE_6_0). Remix leaves its kind 0, the Box's: the DS
 * decodes item attributes per kind, so it numbers the C4 past the source's
 * kinds (nothing the C4 reaches tests for the Box). */
static ITDesc sNdsP4SnakeC4Desc = {
    NDS_P4_IT_KIND_SNAKE_C4, &gNdsP4SnakeSpecial1, (const void *)0x40,
    { nGCMatrixKindTraRotRpyR, nGCMatrixKindNull, 0x00 }, 0,
    ndsP4SnakeC4ProcUpdate, ndsP4SnakeC4ProcMap, NULL, NULL, NULL, NULL, NULL, NULL
};

/* flashbang_begin_resting_: still, no hitbox or hurtbox, the set count
 * restarted, not landed, grounded, resting, and the attach sound. */
static void ndsP4SnakeC4BeginResting(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    ip->attack_coll.attack_state = nGMAttackStateOff;
    ip->damage_coll.hitstatus = nGMHitStatusNone;
    *ndsP4SnakeItemWord(ip, SNAKE_C4_FRAMES_SET) = 0;
    ip->times_landed = 0;
    ip->physics.vel_air.y = 0.0F;
    ip->physics.vel_air.z = 0.0F;
    itMapSetGround(ip);
    itMainSetStatus(item_gobj, sNdsP4SnakeC4StatusDescs, 1);
    func_800269C0_275C0(SNAKE_FGM_C4_ATTACH);
}

/* attach_floor: resting, turned to the surface it touched (the proximity
 * mine's), mirrored when it faces left. */
static void ndsP4SnakeC4AttachFloor(GObj *item_gobj)
{
    ndsP4SnakeC4BeginResting(item_gobj);
    itMSBombAttachedUpdateSurface(item_gobj);

    if (itGetStruct(item_gobj)->lr == -1)
    {
        DObjGetStruct(item_gobj)->rotate.vec.f.z = -DObjGetStruct(item_gobj)->rotate.vec.f.z;
    }
}

/* flashbang_begin_explosion_: Link's bomb's explosion start on the C4's
 * file, then 424 wide, 17 fire damage at 80 degrees, 20 base and 100
 * growth, silent. */
static void ndsP4SnakeC4BeginExplosion(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    ndsP4SnakeExplodeInit(item_gobj);
    ip->attack_coll.knockback_scale = 70;
    ip->attack_coll.knockback_base = 50;
    ndsP4SnakeExplodeUpdateAttackEvent(item_gobj, &sNdsP4SnakeC4Desc);
    itMainSetStatus(item_gobj, sNdsP4SnakeC4StatusDescs, 2);
    ip->attack_coll.fgm_id = SNAKE_FGM_NONE;
    ip->attack_coll.element = nGMHitElementFire;
    ip->attack_coll.size = SNAKE_C4_EXPLODE_SIZE;
    ip->attack_coll.damage = SNAKE_C4_EXPLODE_DAMAGE;
    ip->attack_coll.angle = SNAKE_C4_EXPLODE_ANGLE;
    ip->attack_coll.knockback_base = SNAKE_C4_EXPLODE_BKB;
    ip->attack_coll.knockback_scale = SNAKE_C4_EXPLODE_KBS;
}

/* flashbang_explosion_: still, no hurtbox, no owner stats, the explosion
 * effect at 1.25, a quake, the explosion. */
static void ndsP4SnakeC4Explode(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    LBParticle *pc;

    ip->physics.vel_air.x = ip->physics.vel_air.y = ip->physics.vel_air.z = 0.0F;
    ip->damage_coll.hitstatus = nGMHitStatusNone;
    itMainClearOwnerStats(item_gobj);
    pc = ndsP4SnakeExplodeEffects(item_gobj);

    if (pc != NULL)
    {
        pc->xf->scale.x = pc->xf->scale.y = pc->xf->scale.z = 1.25F;
    }
    ndsP4SnakeExplodeHide(item_gobj);
    ndsP4SnakeC4BeginExplosion(item_gobj);
    func_800269C0_275C0(SNAKE_FGM_EXPLODE);
}

/* flashbang_main_: the fuse; stuck to a fighter it rides his body's centre
 * and goes when he is KO'd (his maker may set another); in the air its
 * fall to 63, on the ground still; set down (or stuck) it flashes every
 * 8th of its first 30 frames. */
static sb32 ndsP4SnakeC4ProcUpdate(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    DObj *dobj = DObjGetStruct(item_gobj);
    FTStruct *sticky = (FTStruct *)(uintptr_t)*ndsP4SnakeItemWord(ip, SNAKE_C4_STICKY);
    sb32 set_down = FALSE;

    if (ip->lifetime == 0)
    {
        ndsP4SnakeC4Explode(item_gobj);
        *ndsP4SnakeItemWord(ip, SNAKE_ITEM_HIT_FLAG) = FALSE;
        return FALSE;
    }
    ip->lifetime--;

    if (sticky != NULL)
    {
        if (sticky->status_id < nFTCommonStatusWait)
        {
            FTStruct *maker = (FTStruct *)(uintptr_t)*ndsP4SnakeItemWord(ip, SNAKE_ITEM_MAKER);

            *ndsP4SnakePassiveWord(maker, 0) = 0;
            return TRUE;
        }
        ip->physics.vel_air.x = 0.0F;
        ip->physics.vel_air.y = 0.0F;
        dobj->translate.vec.f.x = sticky->coll_data.p_translate->x;
        dobj->translate.vec.f.y = sticky->coll_data.p_translate->y +
                                  sticky->coll_data.map_coll.center;
        (*ndsP4SnakeItemWord(ip, SNAKE_C4_FRAMES_SET))++;
        set_down = TRUE;
    }
    else if (ip->ga == nMPKineticsGround)
    {
        ip->physics.vel_air.x = 0.0F;
        ip->attack_coll.attack_state = nGMAttackStateOff;
        (*ndsP4SnakeItemWord(ip, SNAKE_C4_FRAMES_SET))++;
        set_down = TRUE;
    }
    else
    {
        f32 max_speed = SNAKE_ITEM_F32(ip, SNAKE_ITEM_MAX_SPEED) - 1.0F;
        f32 rot_dir = SNAKE_ITEM_F32(ip, SNAKE_ITEM_ROT_DIR);
        f32 speed;
        f32 rot;

        SNAKE_ITEM_F32(ip, SNAKE_ITEM_MAX_SPEED) =
            (max_speed <= SNAKE_C4_MAX_SPEED) ? SNAKE_C4_MAX_SPEED : max_speed;
        itMainApplyGravityClampTVel(ip, SNAKE_C4_GRAVITY, SNAKE_ITEM_F32(ip, SNAKE_ITEM_MAX_SPEED));

        if (ndsP4SnakeF32Bits(ip->physics.vel_air.x) != 0u)
        {
            rot_dir = ip->physics.vel_air.x / ABSF(ip->physics.vel_air.x);
            SNAKE_ITEM_F32(ip, SNAKE_ITEM_ROT_DIR) = rot_dir;
        }
        speed = sqrtf((ip->physics.vel_air.x * ip->physics.vel_air.x) +
                      (ip->physics.vel_air.y * ip->physics.vel_air.y));
        rot = (ndsP4SnakeF32Bits(speed) != 0u) ?
            (ndsP4SnakeBitsToF32(0x3C900000u) + (ndsP4SnakeBitsToF32(0x3CA3D70Au) * speed)) * rot_dir :
            0.0F;
        dobj->rotate.vec.f.z -= rot;
    }
    if (set_down != FALSE)
    {
        s32 frames = *ndsP4SnakeItemWord(ip, SNAKE_C4_FRAMES_SET);

        if (((u32)frames < SNAKE_C4_FLASH_FRAMES) && ((frames & 7) == 0))
        {
            Vec3f pos = dobj->translate.vec.f;

            efManagerFuraSparkleMakeEffect(&pos);
        }
    }
    *ndsP4SnakeItemWord(ip, SNAKE_ITEM_HIT_FLAG) = FALSE;
    return FALSE;
}

/* flashbang_collision_: unless stuck to a fighter, any contact attaches it
 * (itMapCheckMapProcAll). */
static sb32 ndsP4SnakeC4ProcMap(GObj *item_gobj)
{
    if (*ndsP4SnakeItemWord(itGetStruct(item_gobj), SNAKE_C4_STICKY) == 0)
    {
        itMapCheckMapProcAll(item_gobj, ndsP4SnakeC4AttachFloor);
    }
    return FALSE;
}

/* flashbang_resting_collision_: its generic resting collision call is
 * commented out; nothing. */
static sb32 ndsP4SnakeC4RestingProcMap(GObj *item_gobj)
{
    (void)item_gobj;
    return FALSE;
}

/* flashbang_exploding_main_: the bomb's events, three frames, then gone and
 * a C4 free for him again. */
static sb32 ndsP4SnakeC4ExplodeProcUpdate(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    ndsP4SnakeExplodeUpdateAttackEvent(item_gobj, &sNdsP4SnakeC4Desc);
    ip->multi++;

    if (ip->multi == SNAKE_C4_EXPLODE_FRAMES)
    {
        FTStruct *maker = (FTStruct *)(uintptr_t)*ndsP4SnakeItemWord(ip, SNAKE_ITEM_MAKER);

        *ndsP4SnakePassiveWord(maker, 0) = 0;
        return TRUE;
    }
    return FALSE;
}

/* flashbang_blast_zone_ (proc_dead): a C4 free for him again. The blast
 * zone routines leave v0 as the caller set it, the routine's own address:
 * every one destroys its item. */
static sb32 ndsP4SnakeC4ProcDead(GObj *item_gobj)
{
    FTStruct *maker =
        (FTStruct *)(uintptr_t)*ndsP4SnakeItemWord(itGetStruct(item_gobj), SNAKE_ITEM_MAKER);

    *ndsP4SnakePassiveWord(maker, 0) = 0;
    return TRUE;
}

/* A wall ahead of a point, his ECB width plus 50 along his facing: the left
 * wall facing right, the right wall facing left. */
static sb32 ndsP4SnakeWallAhead(Vec3f *base, f32 width, s32 lr, s32 *line_id, Vec3f *angle)
{
    Vec3f target = *base;

    target.x += (width + SNAKE_C4_WALL_REACH) * (f32)lr;

    return (lr < 0) ?
        mpCollisionCheckRWallLineCollisionSame(base, &target, NULL, line_id, NULL, angle) :
        mpCollisionCheckLWallLineCollisionSame(base, &target, NULL, line_id, NULL, angle);
}

/* flashbang_stage_setting_: the C4 at 0.4 scale (mirrored facing left), a
 * 40-tall box, 1600 frames of fuse, his team and port, his C4 (passive word
 * 0), the fighter it sticks to (his temp variable 1: the attach sound and
 * no model), no hitbox or hurtbox, no damage and no clash, 63 down, and, not
 * stuck, a wall ahead attaches it at once turned to the wall. */
static GObj *ndsP4SnakeMakeC4(GObj *fighter_gobj, Vec3f *pos)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    Vec3f vel = { 0.0F, 0.0F, 0.0F };
    GObj *item_gobj = ndsP4SnakeMakeItem(fighter_gobj, &sNdsP4SnakeC4Desc, pos, &vel);
    ITStruct *ip;
    DObj *dobj;
    f32 scale = ndsP4SnakeBitsToF32(SNAKE_C4_SCALE_BITS);

    if (item_gobj == NULL)
    {
        return NULL;
    }
    ip = itGetStruct(item_gobj);
    dobj = DObjGetStruct(item_gobj);
    gcAddXObjForDObjFixed(dobj, 0x2E, 0);
    gcAddXObjForDObjFixed(dobj->child, 0x2E, 0);

    ip->coll_data.map_coll.center = 0.0F;
    ip->coll_data.map_coll.top = SNAKE_C4_ECB;
    ip->coll_data.map_coll.bottom = -SNAKE_C4_ECB;
    ip->coll_data.map_coll.width = SNAKE_C4_ECB;
    dobj->scale.vec.f.x = dobj->scale.vec.f.y = dobj->scale.vec.f.z = scale;

    if (fp->lr == -1)
    {
        dobj->scale.vec.f.x = -dobj->scale.vec.f.x;
    }
    ip->lr = fp->lr;
    ip->attack_coll.can_rehit_shield = TRUE;
    ip->lifetime = SNAKE_C4_FUSE;
    ip->physics.vel_air.x = ip->physics.vel_air.y = ip->physics.vel_air.z = 0.0F;
    ip->owner_gobj = fighter_gobj;
    ip->team = fp->team;
    ip->player = fp->player;
    *ndsP4SnakePassiveWord(fp, 0) = (s32)(uintptr_t)ip;
    *ndsP4SnakeItemWord(ip, SNAKE_ITEM_MAKER) = (s32)(uintptr_t)fp;
    *ndsP4SnakeItemWord(ip, SNAKE_C4_STICKY) = fp->motion_vars.flags.flag0;
    *ndsP4SnakeItemWord(ip, SNAKE_C4_FRAMES_SET) = 0;

    if (fp->motion_vars.flags.flag0 != 0)
    {
        func_800269C0_275C0(SNAKE_FGM_C4_ATTACH);
        dobj->dl = NULL;
    }
    ip->damage_coll.hitstatus = nGMHitStatusNone;
    ip->attack_coll.attack_state = nGMAttackStateOff;
    ip->attack_coll.damage = 0;
    ip->attack_coll.can_setoff = FALSE;
    SNAKE_ITEM_F32(ip, SNAKE_ITEM_MAX_SPEED) = SNAKE_C4_MAX_SPEED;
    *ndsP4SnakeItemWord(ip, SNAKE_ITEM_ROT_DIR) = 0;
    *ndsP4SnakeItemWord(ip, SNAKE_ITEM_REFRESH) = 0;
    *ndsP4SnakeItemWord(ip, SNAKE_ITEM_HIT_FLAG) = FALSE;
    ip->proc_dead = ndsP4SnakeC4ProcDead;
    ip->coll_data.cliff_id = 0;
    ip->attack_coll.knockback_scale = 0;
    ip->attack_coll.knockback_weight = 0;
    ip->attack_coll.knockback_base = 1;
    ndsP4SnakeItemCollideFromOwner(item_gobj, fp);

    if (*ndsP4SnakeItemWord(ip, SNAKE_C4_STICKY) == 0)
    {
        Vec3f base = dobj->translate.vec.f;
        Vec3f angle;
        s32 line_id;

        base.y += ip->coll_data.map_coll.center;

        if (ndsP4SnakeWallAhead(&base, ip->coll_data.map_coll.width, fp->lr, &line_id, &angle) !=
            FALSE)
        {
            ip->attach_line_id = line_id;
            ip->is_attach_surface = TRUE;
            dobj->rotate.vec.f.z = syUtilsArcTan2(angle.y, angle.x) - ndsP4SnakeBitsToF32(0x3FC90FDBu);

            if (ip->lr == -1)
            {
                dobj->rotate.vec.f.z = -dobj->rotate.vec.f.z;
            }
            ndsP4SnakeC4BeginResting(item_gobj);
        }
    }
    return item_gobj;
}

/* C4.initial (ground and air): place one, or with one out set it off. */
void ndsP4SnakeC4Initial(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    sb32 out = (*ndsP4SnakePassiveWord(fp, 0) != 0) ? TRUE : FALSE;

    if (fp->ga == nMPKineticsGround)
    {
        ndsP4SnakeSetStatus(fighter_gobj, out ? SNAKE_STATUS_C4_DETONATE : SNAKE_STATUS_C4_START);
    }
    else ndsP4SnakeSetStatus(fighter_gobj, out ? SNAKE_STATUS_C4_AIR_DETONATE :
                                                 SNAKE_STATUS_C4_AIR_START);
}

/* C4.Start.goto_next: a wall ahead of his body's centre, and the first
 * other fighter he faces within 50 (times his scale) plus both ECB widths
 * (temp variable 1). Team attack off skips fighters on a team: the donor
 * compares the target's team with s2's, a register it never sets (meant:
 * his own, s0); the DS compares his. Stuck, walled or set down. */
static void ndsP4SnakeC4StartGotoNext(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    Vec3f base = *fp->coll_data.p_translate;
    f32 range = SNAKE_C4_STICKY_RANGE * fp->joints[nFTPartsJointTopN]->scale.vec.f.x;
    FTStruct *sticky = NULL;
    sb32 wall;
    GObj *other;

    base.y += fp->coll_data.map_coll.center;
    wall = ndsP4SnakeWallAhead(&base, fp->coll_data.map_coll.width, fp->lr, NULL, NULL);

    for (other = gGCCommonLinks[nGCCommonLinkIDFighter]; other != NULL; other = other->link_next)
    {
        FTStruct *ofp = ftGetStruct(other);
        f32 dx;
        f32 dy;

        if (other == fighter_gobj)
        {
            continue;
        }
        if ((gSCManagerBattleState->is_team_attack == FALSE) && (ofp->team == fp->team))
        {
            continue;
        }
        dx = ofp->coll_data.p_translate->x - base.x;

        if ((fp->lr < 0) ? !(dx < 0.0F) : !(0.0F <= dx))
        {
            continue;
        }
        dy = (ofp->coll_data.p_translate->y + ofp->coll_data.map_coll.center) - base.y;

        if (sqrtf((dx * dx) + (dy * dy)) <=
            ((range + ofp->coll_data.map_coll.width) + fp->coll_data.map_coll.width))
        {
            sticky = ofp;
            break;
        }
    }
    fp->motion_vars.flags.flag0 = (u32)(uintptr_t)sticky;

    if (fp->ga == nMPKineticsGround)
    {
        ndsP4SnakeSetStatus(fighter_gobj, (sticky != NULL) ? SNAKE_STATUS_C4_ENEMY :
                                          (wall != FALSE) ? SNAKE_STATUS_C4_WALL :
                                                            SNAKE_STATUS_C4_GROUND);
    }
    else ndsP4SnakeSetStatus(fighter_gobj, (sticky != NULL) ? SNAKE_STATUS_C4_AIR_ENEMY :
                                           (wall != FALSE) ? SNAKE_STATUS_C4_AIR_WALL :
                                                             SNAKE_STATUS_C4_AIR_GROUND);
}

/* C4.Start.main (both). */
void ndsP4SnakeC4StartUpdate(GObj *fighter_gobj)
{
    ftAnimEndCheckSetStatus(fighter_gobj, ndsP4SnakeC4StartGotoNext);
}

static void ndsP4SnakeC4StartAirToGround(GObj *fighter_gobj)
{
    ndsP4SnakeAirToGround(fighter_gobj, SNAKE_STATUS_C4_START);
}

/* C4.Start.air_collision (C4AIRSTART's map). */
void ndsP4SnakeC4StartAirMap(GObj *fighter_gobj)
{
    mpCommonProcFighterLanding(fighter_gobj, ndsP4SnakeC4StartAirToGround);
}

/* C4.Ground.main_ground / main_air: the C4 from his hand on frame 13 (9
 * airborne) on the ground, 3 (4) on a wall, 2 on a fighter, with the
 * item-get swirl; wait or fall at the end. */
static void ndsP4SnakeC4Place(GObj *fighter_gobj, f32 frame_ground, f32 frame_wall)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    s32 status_id = fp->status_id;
    f32 frame;

    if ((status_id == SNAKE_STATUS_C4_GROUND) || (status_id == SNAKE_STATUS_C4_AIR_GROUND))
    {
        frame = frame_ground;
    }
    else if ((status_id == SNAKE_STATUS_C4_WALL) || (status_id == SNAKE_STATUS_C4_AIR_WALL))
    {
        frame = frame_wall;
    }
    else frame = 2.0F;

    if (fighter_gobj->anim_frame == frame)
    {
        Vec3f pos;

        ndsP4SnakeHandPos(fp, &pos);
        (void)ndsP4SnakeMakeC4(fighter_gobj, &pos);
        efManagerItemGetSwirlProcUpdate(&pos);
    }
}

void ndsP4SnakeC4GroundUpdate(GObj *fighter_gobj)
{
    ndsP4SnakeC4Place(fighter_gobj, 13.0F, 3.0F);
    ftAnimEndSetWait(fighter_gobj);
}

void ndsP4SnakeC4GroundAirUpdate(GObj *fighter_gobj)
{
    ndsP4SnakeC4Place(fighter_gobj, 9.0F, 4.0F);
    ftAnimEndSetFall(fighter_gobj);
}

/* C4.Detonate.detonate_check: on frame 25 his C4's fuse to 0 (one already
 * past 0 stays). */
static void ndsP4SnakeC4DetonateCheck(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    ITStruct *c4 = (ITStruct *)(uintptr_t)*ndsP4SnakePassiveWord(fp, 0);

    if ((fighter_gobj->anim_frame == SNAKE_C4_DETONATE_FRAME) && (c4 != NULL) && (c4->lifetime >= 0))
    {
        c4->lifetime = 0;
    }
}

void ndsP4SnakeC4DetonateUpdate(GObj *fighter_gobj)
{
    ndsP4SnakeC4DetonateCheck(fighter_gobj);
    ftAnimEndSetWait(fighter_gobj);
}

void ndsP4SnakeC4DetonateAirUpdate(GObj *fighter_gobj)
{
    ndsP4SnakeC4DetonateCheck(fighter_gobj);
    ftAnimEndSetFall(fighter_gobj);
}

static void ndsP4SnakeC4DetonateAirToGround(GObj *fighter_gobj)
{
    ndsP4SnakeAirToGround(fighter_gobj, SNAKE_STATUS_C4_DETONATE);
}

/* C4.Detonate.air_collision (C4AIRDETONATE's map). */
void ndsP4SnakeC4DetonateAirMap(GObj *fighter_gobj)
{
    mpCommonProcFighterLanding(fighter_gobj, ndsP4SnakeC4DetonateAirToGround);
}

/* ---- Grenades (neutral special) ---- */

#define SNAKE_GRENADE_FUSE 150
#define SNAKE_GRENADE_MAX 2
#define SNAKE_GRENADE_MAX_SPEED 120.0F
#define SNAKE_GRENADE_GRAVITY 2.8F
#define SNAKE_GRENADE_BOUNCE 0.5F
#define SNAKE_GRENADE_SPAWN_FRAME 2.0F
#define SNAKE_GRENADE_EXPLODE_FRAMES 2
#define SNAKE_GRENADE_EXPLODE_DAMAGE 9
/* The release's stat flags, the donor's 38 (its comment's
 * nFTStatusAttackIDItemThrow is another value). */
#define SNAKE_GRENADE_RELEASE_STAT 38

/* Grenade.asm custom words. */
#define SNAKE_GRENADE_PENDING 0x350
#define SNAKE_GRENADE_LAST_OWNER 0x1D8
#define SNAKE_GRENADE_LANDINGS 0x1DC

static sb32 ndsP4SnakeGrenadeUnreleasedProcUpdate(GObj *item_gobj);
static sb32 ndsP4SnakeGrenadeProcUpdate(GObj *item_gobj);
static sb32 ndsP4SnakeGrenadeProcMap(GObj *item_gobj);
static sb32 ndsP4SnakeGrenadeRestingProcMap(GObj *item_gobj);
static sb32 ndsP4SnakeGrenadeExplodeProcUpdate(GObj *item_gobj);
static sb32 ndsP4SnakeGrenadeBounce(GObj *item_gobj);
static sb32 ndsP4SnakeGrenadeProcDamage(GObj *item_gobj);

/* flashbang_item_states: unreleased, resting, explosion, thrown. */
static ITStatusDesc sNdsP4SnakeGrenadeStatusDescs[4] = {
    { ndsP4SnakeGrenadeUnreleasedProcUpdate, NULL, NULL, NULL, NULL, NULL, NULL,
      ndsP4SnakeGrenadeProcDamage },
    { ndsP4SnakeGrenadeProcUpdate, ndsP4SnakeGrenadeRestingProcMap, ndsP4SnakeGrenadeBounce,
      ndsP4SnakeGrenadeBounce, itMainCommonProcHop, NULL, itMainCommonProcReflector,
      ndsP4SnakeGrenadeProcDamage },
    { ndsP4SnakeGrenadeExplodeProcUpdate, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
    { ndsP4SnakeGrenadeProcUpdate, ndsP4SnakeGrenadeProcMap, ndsP4SnakeGrenadeBounce,
      ndsP4SnakeGrenadeBounce, itMainCommonProcHop, NULL, itMainCommonProcReflector,
      ndsP4SnakeGrenadeProcDamage },
};

/* item_info_array: ITAttributes at 0x40 of his special file 2 (the
 * grenade's own file); Remix's custom item 0x4C, past the source's kinds
 * here as there. */
static ITDesc sNdsP4SnakeGrenadeDesc = {
    NDS_P4_IT_KIND_SNAKE_GRENADE, &gNdsP4SnakeSpecial2, (const void *)0x40,
    { nGCMatrixKindTraRotRpyR, nGCMatrixKindNull, 0x00 }, 0,
    ndsP4SnakeGrenadeUnreleasedProcUpdate, NULL, NULL, NULL, NULL, NULL, NULL,
    ndsP4SnakeGrenadeProcDamage
};

static FTStruct *ndsP4SnakeGrenadeMaker(ITStruct *ip)
{
    return (FTStruct *)(uintptr_t)*ndsP4SnakeItemWord(ip, SNAKE_ITEM_MAKER);
}

/* flashbang_begin_main_ (DROP_ITEM): pickable, airborne, no hitbox, its
 * hurtbox on (the Link bomb's), thrown. */
static void ndsP4SnakeGrenadeBeginMain(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    ip->is_allow_pickup = TRUE;
    itMapSetAir(ip);
    ip->attack_coll.attack_state = nGMAttackStateOff;
    itLinkBombCommonSetHitStatusNormal(item_gobj);
    itMainSetStatus(item_gobj, sNdsP4SnakeGrenadeStatusDescs, 3);
}

/* flashbang_begin_resting_: pickable, not landed, still, grounded,
 * resting. */
static void ndsP4SnakeGrenadeBeginResting(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    ip->is_allow_pickup = TRUE;
    ip->times_landed = 0;
    ip->physics.vel_air.y = 0.0F;
    *ndsP4SnakeItemWord(ip, SNAKE_GRENADE_LANDINGS) = 0;
    ip->physics.vel_air.z = 0.0F;
    itMapSetGround(ip);
    itLinkBombCommonSetHitStatusNormal(item_gobj);
    itMainSetStatus(item_gobj, sNdsP4SnakeGrenadeStatusDescs, 1);
}

/* flashbang_begin_explosion_: Link's bomb's explosion start on the
 * grenade's file, then a 300-wide first frame of 9 fire damage that hits
 * its owner too (80 degrees, 70 base, 69 growth, -4 to shields), silent,
 * not pickable. */
static void ndsP4SnakeGrenadeBeginExplosion(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    ndsP4SnakeExplodeInit(item_gobj);
    ndsP4SnakeExplodeUpdateAttackEvent(item_gobj, &sNdsP4SnakeGrenadeDesc);
    itMainSetStatus(item_gobj, sNdsP4SnakeGrenadeStatusDescs, 2);
    ip->attack_coll.fgm_id = SNAKE_FGM_NONE;
    ip->attack_coll.element = nGMHitElementFire;
    ip->attack_coll.size = 300.0F;
    ip->attack_coll.damage = SNAKE_GRENADE_EXPLODE_DAMAGE;
    ip->attack_coll.shield_damage = -4;
    ip->attack_coll.angle = 80;
    ip->attack_coll.knockback_base = 70;
    ip->attack_coll.knockback_scale = 69;
    ip->is_allow_pickup = FALSE;
}

/* flashbang_explosion_: a held grenade leaves its holder's hand first;
 * its owner kept for the second frame, still, no hurtbox, no owner stats,
 * the explosion effect, a quake, the explosion, the small explosion
 * sound. */
static void ndsP4SnakeGrenadeExplode(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    if (ip->is_hold)
    {
        Vec3f vel = { 0.0F, 0.0F, 0.0F };

        itMainSetFighterRelease(item_gobj, &vel, 1.0F, SNAKE_GRENADE_RELEASE_STAT, 0);
    }
    *ndsP4SnakeItemWord(ip, SNAKE_GRENADE_LAST_OWNER) = (s32)(uintptr_t)ip->owner_gobj;
    ip->physics.vel_air.x = ip->physics.vel_air.y = ip->physics.vel_air.z = 0.0F;
    ip->damage_coll.hitstatus = nGMHitStatusNone;
    itMainClearOwnerStats(item_gobj);
    (void)ndsP4SnakeExplodeEffects(item_gobj);
    ndsP4SnakeExplodeHide(item_gobj);
    ndsP4SnakeGrenadeBeginExplosion(item_gobj);
    func_800269C0_275C0(SNAKE_FGM_EXPLODE_S);
}

/* The fuse, shared by the held and the loose grenade: at 0 it goes. */
static sb32 ndsP4SnakeGrenadeFuse(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    if (ip->lifetime == 0)
    {
        ndsP4SnakeGrenadeExplode(item_gobj);
        return TRUE;
    }
    ip->lifetime--;
    return FALSE;
}

/* unreleased_main: in his hand (joint 17) while he stays in a grenade
 * status (one that lands is let go without a bounce); its fuse burns; it
 * keeps no spin. */
static sb32 ndsP4SnakeGrenadeUnreleasedProcUpdate(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    FTStruct *owner = ftGetStruct(ip->owner_gobj);
    s32 status_id = owner->status_id;

    if (!(((status_id >= SNAKE_STATUS_GRENADE_START) &&
           (status_id <= SNAKE_STATUS_GRENADE_JUMPSQUAT) &&
           (status_id != SNAKE_STATUS_GRENADE_THROW_EMPTY)) ||
          ((status_id >= SNAKE_STATUS_GRENADE_START_AIR) &&
           (status_id <= SNAKE_STATUS_GRENADE_THROW_B_AIR) &&
           (status_id != SNAKE_STATUS_GRENADE_THROW_EMPTY_AIR))))
    {
        *ndsP4SnakeItemWord(ip, SNAKE_GRENADE_LANDINGS) = 1;
        ndsP4SnakeGrenadeBeginMain(item_gobj);
    }
    else
    {
        Vec3f pos = { 0.0F, 0.0F, 0.0F };
        DObj *dobj = DObjGetStruct(item_gobj);

        gmCollisionGetFighterPartsWorldPosition(owner->joints[SNAKE_JOINT_ITEM_LIGHT], &pos);
        dobj->translate.vec.f.x = pos.x;
        dobj->translate.vec.f.y = pos.y;

        if (ndsP4SnakeGrenadeFuse(item_gobj) == FALSE)
        {
            dobj->rotate.vec.f.z = 0.0F;
        }
    }
    *ndsP4SnakeItemWord(ip, SNAKE_ITEM_HIT_FLAG) = FALSE;
    return FALSE;
}

/* flashbang_main_: a hit left pending goes off now (its hitlag over); else
 * in the air its fall to 120, on the ground a slide at 0.9375 to a stop
 * below 2 (and no hitbox); its fuse; its spin by its speed; the hitbox
 * refreshed when its timer runs out. */
static sb32 ndsP4SnakeGrenadeProcUpdate(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    DObj *dobj = DObjGetStruct(item_gobj);

    if (*ndsP4SnakeItemWord(ip, SNAKE_GRENADE_PENDING) != 0)
    {
        *ndsP4SnakeItemWord(ip, SNAKE_GRENADE_PENDING) = 0;
        ndsP4SnakeGrenadeExplode(item_gobj);
    }
    else
    {
        if (ip->ga != nMPKineticsGround)
        {
            f32 max_speed = SNAKE_ITEM_F32(ip, SNAKE_ITEM_MAX_SPEED) - 1.0F;

            SNAKE_ITEM_F32(ip, SNAKE_ITEM_MAX_SPEED) =
                (max_speed <= SNAKE_GRENADE_MAX_SPEED) ? SNAKE_GRENADE_MAX_SPEED : max_speed;
            itMainApplyGravityClampTVel(ip, SNAKE_GRENADE_GRAVITY,
                                        SNAKE_ITEM_F32(ip, SNAKE_ITEM_MAX_SPEED));
        }
        else
        {
            ip->physics.vel_air.x *= ndsP4SnakeBitsToF32(0x3F700000u);

            if (ABSF(ip->physics.vel_air.x) < 2.0F)
            {
                ip->physics.vel_air.x = 0.0F;
                ip->attack_coll.attack_state = nGMAttackStateOff;
            }
        }
        if (ndsP4SnakeGrenadeFuse(item_gobj) == FALSE)
        {
            f32 rot_dir = SNAKE_ITEM_F32(ip, SNAKE_ITEM_ROT_DIR);
            f32 speed;
            s32 refresh;

            if (ndsP4SnakeF32Bits(ip->physics.vel_air.x) != 0u)
            {
                rot_dir = ip->physics.vel_air.x / ABSF(ip->physics.vel_air.x);
                SNAKE_ITEM_F32(ip, SNAKE_ITEM_ROT_DIR) = rot_dir;
            }
            speed = sqrtf((ip->physics.vel_air.x * ip->physics.vel_air.x) +
                          (ip->physics.vel_air.y * ip->physics.vel_air.y));
            dobj->rotate.vec.f.z -= (ndsP4SnakeF32Bits(speed) != 0u) ?
                (ndsP4SnakeBitsToF32(0x3C900000u) + (ndsP4SnakeBitsToF32(0x3B449BA6u) * speed)) *
                    rot_dir :
                0.0F;

            refresh = *ndsP4SnakeItemWord(ip, SNAKE_ITEM_REFRESH);

            if (refresh != 0)
            {
                *ndsP4SnakeItemWord(ip, SNAKE_ITEM_REFRESH) = --refresh;

                if (refresh == 0)
                {
                    s32 i;

                    for (i = 0; i < 4; i++)
                    {
                        ip->attack_coll.attack_records[i].victim_gobj = NULL;
                    }
                }
            }
        }
    }
    *ndsP4SnakeItemWord(ip, SNAKE_ITEM_HIT_FLAG) = FALSE;
    return FALSE;
}

/* flashbang_collision_ (thrown): the map with no landing effects, a
 * rebound off any surface at 0.5, and the second floor contact sets it
 * down. */
static sb32 ndsP4SnakeGrenadeProcMap(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    sb32 hit = mpProcessUpdateMain(&ip->coll_data, ndsP4SnakeItemDetectCollision, item_gobj,
                                   MAP_FLAG_MAIN_MASK);

    itMapCheckCollideAllRebound(item_gobj, MAP_FLAG_MAIN_MASK, SNAKE_GRENADE_BOUNCE, NULL);

    if ((hit != FALSE) && (ip->coll_data.mask_stat & MAP_FLAG_FLOOR))
    {
        s32 landings = ++(*ndsP4SnakeItemWord(ip, SNAKE_GRENADE_LANDINGS));

        if (landings >= 2)
        {
            ndsP4SnakeGrenadeBeginResting(item_gobj);
        }
    }
    return FALSE;
}

/* flashbang_resting_collision_: off a ledge it falls (thrown). */
static sb32 ndsP4SnakeGrenadeRestingProcMap(GObj *item_gobj)
{
    itMapCheckLRWallProcNoFloor(item_gobj, ndsP4SnakeGrenadeBeginMain);
    return FALSE;
}

/* flashbang_exploding_main_: from the second frame a 500-wide hitbox that
 * spares the owner it had before (its last owner), the bomb's events, two
 * frames, then gone and one grenade fewer out for its maker. */
static sb32 ndsP4SnakeGrenadeExplodeProcUpdate(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    ip->owner_gobj = (GObj *)(uintptr_t)*ndsP4SnakeItemWord(ip, SNAKE_GRENADE_LAST_OWNER);
    ip->attack_coll.size = 500.0F;
    ndsP4SnakeExplodeUpdateAttackEvent(item_gobj, &sNdsP4SnakeGrenadeDesc);
    ip->multi++;

    if (ip->multi == SNAKE_GRENADE_EXPLODE_FRAMES)
    {
        (*ndsP4SnakePassiveWord(ndsP4SnakeGrenadeMaker(ip), 3))--;
        return TRUE;
    }
    return FALSE;
}

/* flashbang_hurtbox_collision_ (hit, shield): thrown, no hitbox, a bounce
 * at 80 degrees (cos 0x3E31, sin 0x3F7C) away from its course at an eighth
 * of its speed plus 30. */
static sb32 ndsP4SnakeGrenadeBounce(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    f32 speed;
    f32 vel_x;

    ndsP4SnakeGrenadeBeginMain(item_gobj);
    ip->attack_coll.attack_state = nGMAttackStateOff;
    speed = (sqrtf((ip->physics.vel_air.x * ip->physics.vel_air.x) +
                   (ip->physics.vel_air.y * ip->physics.vel_air.y)) *
             ndsP4SnakeBitsToF32(0x3E000000u)) + 30.0F;
    vel_x = speed * ndsP4SnakeBitsToF32(0x3E310000u);

    if (ip->physics.vel_air.x == 0.0F)
    {
        vel_x = 0.0F;
    }
    else if (!(ip->physics.vel_air.x < 0.0F))
    {
        vel_x = -vel_x;
    }
    ip->physics.vel_air.x = vel_x;
    ip->physics.vel_air.y = speed * ndsP4SnakeBitsToF32(0x3F7C0000u);

    return FALSE;
}

/* flashbang_hitbox_collision_ (hurt by a hitbox): the hit's owner and port
 * take it over; another grenade's flying contact bounces it (any item
 * whose attack records name it, in the item list's order); any other hit
 * leaves an explosion pending for the frame after its hitlag, thrown. */
static sb32 ndsP4SnakeGrenadeProcDamage(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    GObj *attacker = NULL;
    GObj *other;

    ip->owner_gobj = ip->damage_gobj;
    ip->player = GMCOMMON_PLAYERS_MAX;

    if ((ip->damage_gobj != NULL) && (ip->damage_gobj->id == nGCCommonKindFighter))
    {
        ip->player = ftGetStruct(ip->damage_gobj)->player;
    }
    for (other = gGCCommonLinks[nGCCommonLinkIDItem]; other != NULL; other = other->link_next)
    {
        ITStruct *oip;
        s32 i;

        if (other == item_gobj)
        {
            continue;
        }
        oip = itGetStruct(other);

        for (i = 0; i < 4; i++)
        {
            if (oip->attack_coll.attack_records[i].victim_gobj == item_gobj)
            {
                break;
            }
        }
        if (i < 4)
        {
            attacker = other;
            break;
        }
    }
    if ((attacker != NULL) && (itGetStruct(attacker)->kind == NDS_P4_IT_KIND_SNAKE_GRENADE) &&
        (itGetStruct(attacker)->attack_coll.element != nGMHitElementFire))
    {
        ndsP4SnakeGrenadeBounce(item_gobj);
        return FALSE;
    }
    itMainSetStatus(item_gobj, sNdsP4SnakeGrenadeStatusDescs, 3);
    *ndsP4SnakeItemWord(ip, SNAKE_GRENADE_PENDING) = 1;

    return FALSE;
}

/* flashbang_blast_zone_ (proc_dead): one grenade fewer out. */
static sb32 ndsP4SnakeGrenadeProcDead(GObj *item_gobj)
{
    (*ndsP4SnakePassiveWord(ndsP4SnakeGrenadeMaker(itGetStruct(item_gobj)), 3))--;
    return TRUE;
}

/* throw_initial_ (THROW_ITEM): hurtbox and hitbox on, 1 damage, thrown. */
static void ndsP4SnakeGrenadeThrown(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    ip->damage_coll.hitstatus = nGMHitStatusNormal;
    ip->attack_coll.attack_state = nGMAttackStateNew;
    ip->attack_coll.damage = 1;
    itMainSetStatus(item_gobj, sNdsP4SnakeGrenadeStatusDescs, 3);
}

/* flashbang_stage_setting_: 150 frames of fuse, hurtbox on, no hitbox, 1
 * damage, the punch sound, its maker's port and count (passive word 3), 1.5
 * its throw speed, no knockback, rehits shields. */
static GObj *ndsP4SnakeMakeGrenade(GObj *fighter_gobj, Vec3f *pos)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    Vec3f vel = { 0.0F, 0.0F, 0.0F };
    GObj *item_gobj = ndsP4SnakeMakeItem(fighter_gobj, &sNdsP4SnakeGrenadeDesc, pos, &vel);
    ITStruct *ip;
    DObj *dobj;

    if (item_gobj == NULL)
    {
        return NULL;
    }
    ip = itGetStruct(item_gobj);
    dobj = DObjGetStruct(item_gobj);
    gcAddXObjForDObjFixed(dobj, 0x2E, 0);
    gcAddXObjForDObjFixed(dobj->child, 0x2E, 0);

    ip->attack_coll.can_rehit_shield = TRUE;
    ip->lifetime = SNAKE_GRENADE_FUSE;
    *ndsP4SnakeItemWord(ip, 0x354) = 4;
    *ndsP4SnakeItemWord(ip, SNAKE_GRENADE_PENDING) = 0;
    ip->physics.vel_air.x = ip->physics.vel_air.y = ip->physics.vel_air.z = 0.0F;
    ip->damage_coll.hitstatus = nGMHitStatusNormal;
    ip->attack_coll.attack_state = nGMAttackStateOff;
    ip->attack_coll.damage = 1;
    SNAKE_ITEM_F32(ip, SNAKE_ITEM_MAX_SPEED) = SNAKE_GRENADE_MAX_SPEED;
    *ndsP4SnakeItemWord(ip, SNAKE_ITEM_ROT_DIR) = 0;
    *ndsP4SnakeItemWord(ip, SNAKE_ITEM_REFRESH) = 0;
    *ndsP4SnakeItemWord(ip, SNAKE_ITEM_HIT_FLAG) = FALSE;
    ip->proc_dead = ndsP4SnakeGrenadeProcDead;
    ip->coll_data.cliff_id = 0;
    ip->owner_gobj = fighter_gobj;
    ip->player = fp->player;
    ip->attack_coll.fgm_id = SNAKE_FGM_PUNCH_S;
    *ndsP4SnakeItemWord(ip, SNAKE_ITEM_MAKER) = (s32)(uintptr_t)fp;
    *ndsP4SnakeItemWord(ip, SNAKE_GRENADE_LANDINGS) = 0;
    (*ndsP4SnakePassiveWord(fp, 3))++;
    *ndsP4SnakeItemWord(ip, SNAKE_GRENADE_LAST_OWNER) = (s32)(uintptr_t)fighter_gobj;
    ip->vel_scale = 1.5F;
    ip->attack_coll.knockback_scale = 0;
    ip->attack_coll.knockback_weight = 0;
    ip->attack_coll.knockback_base = 0;
    ndsP4SnakeItemCollideFromOwner(item_gobj, fp);

    return item_gobj;
}

/* Grenade.initial (ground and air): a held item dropped; a grenade, or the
 * empty throw with two out. */
void ndsP4SnakeGrenadeInitial(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    sb32 can = (*ndsP4SnakePassiveWord(fp, 3) < SNAKE_GRENADE_MAX) ? TRUE : FALSE;

    if (fp->item_gobj != NULL)
    {
        Vec3f vel = { 0.0F, 0.0F, 0.0F };

        itMainSetFighterDrop(fp->item_gobj, &vel, 1.0F);
    }
    if (fp->ga == nMPKineticsGround)
    {
        ndsP4SnakeSetStatus(fighter_gobj, can ? SNAKE_STATUS_GRENADE_START :
                                                SNAKE_STATUS_GRENADE_THROW_EMPTY);
    }
    else ndsP4SnakeSetStatus(fighter_gobj, can ? SNAKE_STATUS_GRENADE_START_AIR :
                                                 SNAKE_STATUS_GRENADE_THROW_EMPTY_AIR);

    fp->motion_vars.flags.flag0 = 0;
}

/* Grenade.Start.goto_next: its ga test reads the word 0x14C past its own
 * address (ftAnimEndCheckSetStatus calls it through a1): an instruction,
 * never 0, so the pull always ends in the aerial wait (a grounded Snake
 * lands back into the wait on his next map pass). */
static void ndsP4SnakeGrenadeStartGotoNext(GObj *fighter_gobj)
{
    ndsP4SnakeSetStatus(fighter_gobj, SNAKE_STATUS_GRENADE_WAIT_AIR);
}

/* Grenade.Start.main (both): on frame 2 the grenade 280 ahead and 225 up
 * (temp variable 1). */
void ndsP4SnakeGrenadeStartUpdate(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (fighter_gobj->anim_frame == SNAKE_GRENADE_SPAWN_FRAME)
    {
        Vec3f pos;

        ndsP4SnakeSpawnPos(fp, 280.0F, 225.0F, &pos);
        fp->motion_vars.flags.flag0 = (u32)(uintptr_t)ndsP4SnakeMakeGrenade(fighter_gobj, &pos);
    }
    ftAnimEndCheckSetStatus(fighter_gobj, ndsP4SnakeGrenadeStartGotoNext);
}

static void ndsP4SnakeGrenadeStartAirToGround(GObj *fighter_gobj)
{
    ndsP4SnakeAirToGround(fighter_gobj, SNAKE_STATUS_GRENADE_START);
}

/* Grenade.Start.air_collision (GRENADESTARTAIR's map). */
void ndsP4SnakeGrenadeStartAirMap(GObj *fighter_gobj)
{
    mpCommonProcFighterLanding(fighter_gobj, ndsP4SnakeGrenadeStartAirToGround);
}

/* Grenade.Wait.main (the waits and walks): grounded, shield or grab; B let
 * go throws, back past -20 back, forward past 20 forward, else neutral. */
void ndsP4SnakeGrenadeWaitUpdate(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    s32 stick;
    s32 status_id;

    if (fp->ga == nMPKineticsGround)
    {
        if ((ftCommonGuardOnCheckInterruptCommon(fighter_gobj) != FALSE) ||
            (ftCommonCatchCheckInterruptCommon(fighter_gobj) != FALSE))
        {
            return;
        }
    }
    if (fp->input.pl.button_hold & SNAKE_BUTTON_B)
    {
        return;
    }
    stick = fp->input.pl.stick_range.x * fp->lr;

    if (fp->ga != nMPKineticsGround)
    {
        status_id = (stick <= -20) ? SNAKE_STATUS_GRENADE_THROW_B_AIR :
                    (stick >= 20)  ? SNAKE_STATUS_GRENADE_THROW_F_AIR :
                                     SNAKE_STATUS_GRENADE_THROW_N_AIR;
    }
    else status_id = (stick <= -20) ? SNAKE_STATUS_GRENADE_THROW_B :
                     (stick >= 20)  ? SNAKE_STATUS_GRENADE_THROW_F :
                                      SNAKE_STATUS_GRENADE_THROW_N;

    ndsP4SnakeSetStatus(fighter_gobj, status_id);
}

static void ndsP4SnakeGrenadeWaitAirToGround(GObj *fighter_gobj)
{
    ndsP4SnakeAirToGround(fighter_gobj, SNAKE_STATUS_GRENADE_WAIT);
}

/* Grenade.Wait.air_collision (GRENADEWAITAIR's map). */
void ndsP4SnakeGrenadeWaitAirMap(GObj *fighter_gobj)
{
    mpCommonProcFighterLanding(fighter_gobj, ndsP4SnakeGrenadeWaitAirToGround);
}

/* Grenade.Throw.main (the throws): on its release frame (neutral 8, 36
 * along and 36 up; forward 10, 100 and 20; back 9, 6 and 64) the grenade
 * leaves his hand with a new hitbox, thrown and pickable; wait or fall at
 * the end. */
void ndsP4SnakeGrenadeThrowUpdate(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    s32 status_id = fp->status_id;
    f32 frame;
    f32 vel_x;
    f32 vel_y;

    if ((status_id == SNAKE_STATUS_GRENADE_THROW_N) ||
        (status_id == SNAKE_STATUS_GRENADE_THROW_N_AIR))
    {
        frame = 8.0F, vel_x = 36.0F, vel_y = 36.0F;
    }
    else if ((status_id == SNAKE_STATUS_GRENADE_THROW_F) ||
             (status_id == SNAKE_STATUS_GRENADE_THROW_F_AIR))
    {
        frame = 10.0F, vel_x = 100.0F, vel_y = 20.0F;
    }
    else frame = 9.0F, vel_x = 6.0F, vel_y = 64.0F;

    if ((fighter_gobj->anim_frame == frame) && (fp->motion_vars.flags.flag0 != 0))
    {
        GObj *item_gobj = (GObj *)(uintptr_t)fp->motion_vars.flags.flag0;
        ITStruct *ip = itGetStruct(item_gobj);

        ip->physics.vel_air.x = (f32)fp->lr * vel_x;
        ip->physics.vel_air.y = vel_y;
        ip->attack_coll.attack_state = nGMAttackStateNew;
        itMainSetStatus(item_gobj, sNdsP4SnakeGrenadeStatusDescs, 3);
        ip->is_thrown = TRUE;
        ip->is_allow_pickup = TRUE;
    }
    if (fp->ga == nMPKineticsGround)
    {
        ftAnimEndSetWait(fighter_gobj);
    }
    else ftAnimEndSetFall(fighter_gobj);
}

/* Grenade.Throw.air_to_ground: each aerial throw lands into its grounded
 * one. */
static void ndsP4SnakeGrenadeThrowAirToGround(GObj *fighter_gobj)
{
    s32 status_id = ftGetStruct(fighter_gobj)->status_id;

    ndsP4SnakeAirToGround(fighter_gobj,
                          (status_id == SNAKE_STATUS_GRENADE_THROW_N_AIR) ? SNAKE_STATUS_GRENADE_THROW_N :
                          (status_id == SNAKE_STATUS_GRENADE_THROW_F_AIR) ? SNAKE_STATUS_GRENADE_THROW_F :
                          (status_id == SNAKE_STATUS_GRENADE_THROW_B_AIR) ? SNAKE_STATUS_GRENADE_THROW_B :
                                                                            SNAKE_STATUS_GRENADE_THROW_EMPTY);
}

/* Grenade.Throw.air_collision (the aerial throws' map). */
void ndsP4SnakeGrenadeThrowAirMap(GObj *fighter_gobj)
{
    mpCommonProcFighterLanding(fighter_gobj, ndsP4SnakeGrenadeThrowAirToGround);
}

/* Grenade.JumpSquat.check_interrupt (Donkey Kong's cargo kneebend): a jump
 * input starts the grenade jumpsquat. */
static sb32 ndsP4SnakeGrenadeJumpSquatCheck(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    s32 input_source = ftCommonKneeBendGetInputTypeCommon(fp);

    if (input_source == FTCOMMON_KNEEBEND_INPUT_TYPE_NONE)
    {
        return FALSE;
    }
    ftMainSetStatus(fighter_gobj, SNAKE_STATUS_GRENADE_JUMPSQUAT, 0.0F, 1.0F,
                    FTSTATUS_PRESERVE_NONE);
    fp->status_vars.common.kneebend.jump_force = fp->input.pl.stick_range.y;
    fp->status_vars.common.kneebend.anim_frame = 0.0F;
    fp->status_vars.common.kneebend.input_source = input_source;
    fp->status_vars.common.kneebend.is_shorthop = FALSE;

    return TRUE;
}

/* Grenade.Walk.plat_drop_initial_: through the platform into the aerial
 * wait. */
static void ndsP4SnakeGrenadePlatDrop(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    mpCommonSetFighterAir(fp);
    ndsP4SnakeSetStatus(fighter_gobj, SNAKE_STATUS_GRENADE_WAIT_AIR);
    ftPhysicsClampAirVelXMax(fp);
    fp->coll_data.ignore_line_id = fp->coll_data.floor_line_id;
    fp->tap_stick_y = FTINPUT_STICKBUFFER_TICS_MAX;
    fp->physics.vel_air.y = 0.0F;
}

/* Grenade.Walk.interrupt (the waits and walks): a jump, a drop through a
 * platform, then the stick: forward past 8 walks forward, back past -8
 * back, else the wait, from frame 1 when it changes. */
void ndsP4SnakeGrenadeWalkInterrupt(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    s32 stick;
    s32 next;

    if (ndsP4SnakeGrenadeJumpSquatCheck(fighter_gobj) != FALSE)
    {
        return;
    }
    if ((fp->coll_data.floor_flags & MAP_VERTEX_COLL_PASS) &&
        (ftCommonPassCheckInputSuccess(fp) != FALSE))
    {
        ndsP4SnakeGrenadePlatDrop(fighter_gobj);
        return;
    }
    stick = fp->input.pl.stick_range.x * fp->lr;

    if (stick >= 8)
    {
        next = SNAKE_STATUS_GRENADE_WALK_F;
    }
    else if (stick < -8)
    {
        next = SNAKE_STATUS_GRENADE_WALK_B;
    }
    else next = SNAKE_STATUS_GRENADE_WAIT;

    if (fp->status_id != next)
    {
        ftMainSetStatus(fighter_gobj, next, 1.0F, 1.0F, FTSTATUS_PRESERVE_NONE);
    }
}

/* Grenade.Walk.physics: the stick's ground speed at 0.3 with his traction,
 * the transfer, the animation at a twentieth of his speed (0x3D4C0000). */
void ndsP4SnakeGrenadeWalkPhysics(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    ftPhysicsSetGroundVelStickRange(fp, ndsP4SnakeBitsToF32(0x3E99999Au), fp->attr->traction);
    ftPhysicsSetGroundVelTransferAir(fighter_gobj);
    gcSetAnimSpeed(fighter_gobj, ABSF(fp->physics.vel_ground.x) * ndsP4SnakeBitsToF32(0x3D4C0000u));
}

/* Grenade.JumpSquat.jump_initial_: Donkey Kong's cargo jump into the aerial
 * wait at speed 1.0 (the donor enters ftDonkeyThrowFJumpSetStatus past its
 * status change). */
static void ndsP4SnakeGrenadeJump(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    FTAttributes *attr = fp->attr;
    s32 vel_x;
    s32 vel_y;

    mpCommonSetFighterAir(fp);
    ftMainSetStatus(fighter_gobj, SNAKE_STATUS_GRENADE_WAIT_AIR, 0.0F, 1.0F, FTSTATUS_PRESERVE_NONE);

    switch (fp->status_vars.common.kneebend.input_source)
    {
    case FTCOMMON_KNEEBEND_INPUT_TYPE_BUTTON:
        ftCommonJumpGetJumpForceButton(fp->input.pl.stick_range.x, &vel_x, &vel_y,
                                       fp->status_vars.common.kneebend.is_shorthop);
        break;

    case FTCOMMON_KNEEBEND_INPUT_TYPE_STICK:
    default:
        vel_x = fp->input.pl.stick_range.x;
        vel_y = fp->status_vars.common.kneebend.jump_force;
        break;
    }
    fp->physics.vel_air.y = (vel_y * attr->jump_height_mul) + attr->jump_height_base;
    fp->physics.vel_air.x = vel_x * attr->jump_vel_x;
    fp->tap_stick_y = FTINPUT_STICKBUFFER_TICS_MAX;
}

/* Grenade.JumpSquat.main (Donkey Kong's cargo kneebend update). */
void ndsP4SnakeGrenadeJumpSquatUpdate(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    fp->status_vars.common.kneebend.anim_frame++;

    if ((fp->status_vars.common.kneebend.input_source == FTCOMMON_KNEEBEND_INPUT_TYPE_BUTTON) &&
        (fp->status_vars.common.kneebend.anim_frame <= FTCOMMON_KNEEBEND_SHORTHOP_FRAMES) &&
        (fp->input.pl.button_release & (R_CBUTTONS | L_CBUTTONS | D_CBUTTONS | U_CBUTTONS)))
    {
        fp->status_vars.common.kneebend.is_shorthop = TRUE;
    }
    if (fp->attr->kneebend_anim_length <= fp->status_vars.common.kneebend.anim_frame)
    {
        ndsP4SnakeGrenadeJump(fighter_gobj);
    }
}

/* ---- Nikita (forward smash) ---- */

#define SNAKE_NIKITA_FUSE 500
#define SNAKE_NIKITA_OUT_OF_GAS 250
#define SNAKE_NIKITA_SPAWN_FRAME 41.0F
#define SNAKE_NIKITA_ECB 50.0F
#define SNAKE_NIKITA_SPEED 30.0F              /* INITIAL_SPEED 0x41F0 */
#define SNAKE_NIKITA_ACCEL 4.0F
#define SNAKE_NIKITA_MAX_SPEED 70.0F
#define SNAKE_NIKITA_TURN_BITS 0x3DA0D97Cu    /* 4.5 degrees */
#define SNAKE_NIKITA_DECEL 0.75F
#define SNAKE_NIKITA_GRAVITY 1.5F
#define SNAKE_NIKITA_TVEL 60.0F
#define SNAKE_NIKITA_DAMAGE_LIMIT 25
#define SNAKE_NIKITA_PI_BITS 0x40490FDBu
#define SNAKE_NIKITA_2PI_BITS 0x40C90FDBu

/* Nikita.asm VARIABLES. */
#define SNAKE_NIKITA_VAR_SPEED 0x350
#define SNAKE_NIKITA_VAR_ANGLE 0x354
#define SNAKE_NIKITA_VAR_HIT 0x358
#define SNAKE_NIKITA_VAR_DISABLED 0x35C

static sb32 ndsP4SnakeNikitaProcUpdate(GObj *item_gobj);
static sb32 ndsP4SnakeNikitaProcMap(GObj *item_gobj);
static sb32 ndsP4SnakeNikitaProcHit(GObj *item_gobj);
static sb32 ndsP4SnakeNikitaProcReflector(GObj *item_gobj);
static sb32 ndsP4SnakeNikitaProcDamage(GObj *item_gobj);
static sb32 ndsP4SnakeNikitaExplodeProcUpdate(GObj *item_gobj);

/* item_states: flying, disabled (unused), explosion. */
static ITStatusDesc sNdsP4SnakeNikitaStatusDescs[3] = {
    { ndsP4SnakeNikitaProcUpdate, ndsP4SnakeNikitaProcMap, ndsP4SnakeNikitaProcHit,
      ndsP4SnakeNikitaProcHit, ndsP4SnakeNikitaProcHit, NULL, ndsP4SnakeNikitaProcReflector,
      ndsP4SnakeNikitaProcDamage },
    { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
    { ndsP4SnakeNikitaExplodeProcUpdate, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
};

/* item_info_array: ITAttributes at 0xF0 of his special file 1
 * (MERGED_FILERESOURCE_6_1); kind as the C4's. */
static ITDesc sNdsP4SnakeNikitaDesc = {
    NDS_P4_IT_KIND_SNAKE_NIKITA, &gNdsP4SnakeSpecial1, (const void *)0xF0,
    { nGCMatrixKindTraRotRpyR, nGCMatrixKindNull, 0x00 }, 0,
    ndsP4SnakeNikitaProcUpdate, ndsP4SnakeNikitaProcMap, ndsP4SnakeNikitaProcHit,
    ndsP4SnakeNikitaProcHit, ndsP4SnakeNikitaProcHit, NULL, ndsP4SnakeNikitaProcReflector,
    ndsP4SnakeNikitaProcDamage
};

static FTStruct *ndsP4SnakeNikitaMaker(ITStruct *ip)
{
    return (FTStruct *)(uintptr_t)*ndsP4SnakeItemWord(ip, SNAKE_ITEM_MAKER);
}

/* Its maker's Nikita (passive word 1) cleared when it is this one. */
static void ndsP4SnakeNikitaRelease(ITStruct *ip)
{
    FTStruct *maker = ndsP4SnakeNikitaMaker(ip);

    if (*ndsP4SnakePassiveWord(maker, 1) == (s32)(uintptr_t)ip)
    {
        *ndsP4SnakePassiveWord(maker, 1) = 0;
    }
}

/* begin_main_: airborne, flying. */
static void ndsP4SnakeNikitaBeginMain(GObj *item_gobj)
{
    itMapSetAir(itGetStruct(item_gobj));
    itMainSetStatus(item_gobj, sNdsP4SnakeNikitaStatusDescs, 0);
}

/* begin_explosion_: Link's bomb's explosion start on the Nikita's file,
 * then fire: a 324-wide 14 at 50 degrees, 75 base and 70 growth, or out of
 * gas a 225-wide 7 at 50, 50 and 110; silent. */
static void ndsP4SnakeNikitaBeginExplosion(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    ndsP4SnakeExplodeInit(item_gobj);
    ip->attack_coll.knockback_scale = 70;
    ip->attack_coll.knockback_base = 50;
    ndsP4SnakeExplodeUpdateAttackEvent(item_gobj, &sNdsP4SnakeNikitaDesc);
    itMainSetStatus(item_gobj, sNdsP4SnakeNikitaStatusDescs, 2);
    ip->attack_coll.fgm_id = SNAKE_FGM_NONE;
    ip->attack_coll.element = nGMHitElementFire;

    if (*ndsP4SnakeItemWord(ip, SNAKE_NIKITA_VAR_DISABLED) == 0)
    {
        ip->attack_coll.size = 324.0F;
        ip->attack_coll.damage = 14;
        ip->attack_coll.angle = 50;
        ip->attack_coll.knockback_base = 75;
        ip->attack_coll.knockback_scale = 70;
    }
    else
    {
        ip->attack_coll.size = 225.0F;
        ip->attack_coll.damage = 7;
        ip->attack_coll.angle = 50;
        ip->attack_coll.knockback_base = 50;
        ip->attack_coll.knockback_scale = 110;
    }
}

/* explosion_: still, no hurtbox (its owner kept), the explosion effect, a
 * quake, the explosion, the small explosion sound. */
static void ndsP4SnakeNikitaExplode(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    ip->physics.vel_air.x = ip->physics.vel_air.y = ip->physics.vel_air.z = 0.0F;
    ip->damage_coll.hitstatus = nGMHitStatusNone;
    (void)ndsP4SnakeExplodeEffects(item_gobj);
    ndsP4SnakeExplodeHide(item_gobj);
    ndsP4SnakeNikitaBeginExplosion(item_gobj);
    func_800269C0_275C0(SNAKE_FGM_EXPLODE_S);
}

/* main_'s _control: steered while its maker, Snake, operates it (passive
 * word 1 naming it): the stick past 10 turns its course toward the stick's
 * angle by up to 4.5 degrees a frame (an exact reversal turns upward by its
 * release facing). Returns TRUE while turning. */
static sb32 ndsP4SnakeNikitaSteer(ITStruct *ip)
{
    FTStruct *owner = ftGetStruct(ip->owner_gobj);
    f32 pi = ndsP4SnakeBitsToF32(SNAKE_NIKITA_PI_BITS);
    f32 two_pi = ndsP4SnakeBitsToF32(SNAKE_NIKITA_2PI_BITS);
    f32 turn = ndsP4SnakeBitsToF32(SNAKE_NIKITA_TURN_BITS);
    f32 current = SNAKE_ITEM_F32(ip, SNAKE_NIKITA_VAR_ANGLE);
    f32 stick_x;
    f32 stick_y;
    f32 target;
    f32 diff;
    f32 next;

    if ((owner->nds_p4_content != SNAKE_CONTENT) ||
        ((owner->status_id != SNAKE_STATUS_NIKITA_OPERATION) &&
         (owner->status_id != SNAKE_STATUS_NIKITA_AIR_OPERATION)) ||
        (*ndsP4SnakePassiveWord(owner, 1) != (s32)(uintptr_t)ip))
    {
        return FALSE;
    }
    stick_x = owner->input.pl.stick_range.x;
    stick_y = owner->input.pl.stick_range.y;

    if (!(10.0F <= sqrtf((stick_y * stick_y) + (stick_x * stick_x))))
    {
        next = current;
    }
    else
    {
        target = syUtilsArcTan2(stick_y, stick_x);
        diff = target - current;

        if (!(-pi < diff))
        {
            diff += two_pi;
        }
        if (!(diff <= pi))
        {
            diff -= two_pi;
        }
        if (!(ndsP4SnakeBitsToF32(0x3C800000u) < ABSF(diff - pi)) && (ip->lr < 0))
        {
            diff -= two_pi;
        }
        if (!(turn < ABSF(diff)))
        {
            next = target;
        }
        else next = current + ((0.0F < diff) ? turn : -turn);

        if (!(-pi < next))
        {
            next += two_pi;
        }
    }
    SNAKE_ITEM_F32(ip, SNAKE_NIKITA_VAR_ANGLE) = next;

    return (next != current) ? TRUE : FALSE;
}

/* main_: 25 damage taken or its fuse out sets it off; below 250 frames it
 * is out of gas; its hitbox comes back with the refresh timer; steered, it
 * slows to 30 while turning or after a hit, else gains 4 a frame to 70, on
 * its course; out of gas it slows by 0.75 and falls at 1.5 to 60; smoke
 * behind it every 4th frame (16th out of gas). */
static sb32 ndsP4SnakeNikitaProcUpdate(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    DObj *dobj = DObjGetStruct(item_gobj);
    sb32 turning = FALSE;
    s32 refresh;

    if ((ip->percent_damage >= SNAKE_NIKITA_DAMAGE_LIMIT) || (ip->lifetime == 0))
    {
        ndsP4SnakeNikitaExplode(item_gobj);
        *ndsP4SnakeItemWord(ip, SNAKE_ITEM_HIT_FLAG) = FALSE;
        return FALSE;
    }
    ip->lifetime--;

    if (ip->lifetime < SNAKE_NIKITA_OUT_OF_GAS)
    {
        *ndsP4SnakeItemWord(ip, SNAKE_NIKITA_VAR_DISABLED) = 1;
    }
    refresh = *ndsP4SnakeItemWord(ip, SNAKE_ITEM_REFRESH);

    if (refresh != 0)
    {
        *ndsP4SnakeItemWord(ip, SNAKE_ITEM_REFRESH) = --refresh;

        if (refresh == 0)
        {
            s32 i;

            for (i = 0; i < 4; i++)
            {
                ip->attack_coll.attack_records[i].victim_gobj = NULL;
            }
            ip->attack_coll.attack_state = nGMAttackStateNew;
            ip->attack_coll.damage = 1;
        }
    }
    if (*ndsP4SnakeItemWord(ip, SNAKE_NIKITA_VAR_DISABLED) == 0)
    {
        turning = ndsP4SnakeNikitaSteer(ip);

        if ((ip->attack_coll.attack_state == nGMAttackStateOff) || (turning != FALSE))
        {
            SNAKE_ITEM_F32(ip, SNAKE_NIKITA_VAR_SPEED) = SNAKE_NIKITA_SPEED;
        }
        else
        {
            f32 speed = SNAKE_ITEM_F32(ip, SNAKE_NIKITA_VAR_SPEED) + SNAKE_NIKITA_ACCEL;

            SNAKE_ITEM_F32(ip, SNAKE_NIKITA_VAR_SPEED) =
                (speed <= SNAKE_NIKITA_MAX_SPEED) ? speed : SNAKE_NIKITA_MAX_SPEED;
        }
    }
    dobj->rotate.vec.f.z = SNAKE_ITEM_F32(ip, SNAKE_NIKITA_VAR_ANGLE) +
                           ndsP4SnakeBitsToF32(SNAKE_NIKITA_PI_BITS);

    if (*ndsP4SnakeItemWord(ip, SNAKE_NIKITA_VAR_DISABLED) == 0)
    {
        f32 angle = SNAKE_ITEM_F32(ip, SNAKE_NIKITA_VAR_ANGLE);
        f32 speed = SNAKE_ITEM_F32(ip, SNAKE_NIKITA_VAR_SPEED);

        ip->physics.vel_air.x = speed * __cosf(angle);
        ip->physics.vel_air.y = speed * __sinf(angle);
    }
    else
    {
        f32 vel_x = ip->physics.vel_air.x;
        f32 vel_y;

        if (ABSF(vel_x) <= SNAKE_NIKITA_DECEL)
        {
            ip->physics.vel_air.x = 0.0F;
        }
        else ip->physics.vel_air.x = (0.0F < vel_x) ? (vel_x - SNAKE_NIKITA_DECEL) :
                                                      (vel_x + SNAKE_NIKITA_DECEL);

        vel_y = ip->physics.vel_air.y - SNAKE_NIKITA_GRAVITY;

        if (-SNAKE_NIKITA_TVEL <= vel_y)
        {
            ip->physics.vel_air.y = vel_y;
        }
    }
    ip->physics.vel_air.z = 0.0F;

    if ((ip->lifetime & ((*ndsP4SnakeItemWord(ip, SNAKE_NIKITA_VAR_DISABLED) != 0) ? 0xF : 0x3)) ==
        0)
    {
        f32 theta = -dobj->rotate.vec.f.z;
        f32 c = __cosf(theta);
        f32 s = __sinf(theta);
        Vec3f pos;

        pos.x = dobj->translate.vec.f.x + ((50.0F * c) + (0.0F * s));
        pos.y = dobj->translate.vec.f.y + ((0.0F * c) - (50.0F * s));
        pos.z = dobj->translate.vec.f.z;
        efManagerDustExpandSmallMakeEffect(&pos, 0.0F);
    }
    *ndsP4SnakeItemWord(ip, SNAKE_ITEM_HIT_FLAG) = FALSE;
    return FALSE;
}

/* collision_: any surface (no landing effects) sets it off. */
static sb32 ndsP4SnakeNikitaProcMap(GObj *item_gobj)
{
    if (mpProcessUpdateMain(&itGetStruct(item_gobj)->coll_data, ndsP4SnakeItemDetectCollision,
                            item_gobj, MAP_FLAG_MAIN_MASK) != FALSE)
    {
        ndsP4SnakeNikitaExplode(item_gobj);
    }
    return FALSE;
}

/* hurtbox_collision_ (hit, shield, hop): flying again with its fuse out
 * (it goes next frame), marking a hit that dealt damage. */
static sb32 ndsP4SnakeNikitaProcHit(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    ndsP4SnakeNikitaBeginMain(item_gobj);
    ip->lifetime = 0;

    if (ip->hit_normal_damage != 0)
    {
        *ndsP4SnakeItemWord(ip, SNAKE_NIKITA_VAR_HIT) = 1;
    }
    return FALSE;
}

/* hitbox_collision_ (hurt by a hitbox): flying, no hitbox for 60 frames, at
 * 30, turned by a quarter of the hit's angle (the Sakurai angle as 43) plus
 * up to a half for 25 damage. */
static sb32 ndsP4SnakeNikitaProcDamage(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    s32 angle_i = ip->damage_angle;
    s32 damage;
    f32 turn;

    ndsP4SnakeNikitaBeginMain(item_gobj);
    *ndsP4SnakeItemWord(ip, SNAKE_ITEM_REFRESH) = 60;
    ip->attack_coll.attack_state = nGMAttackStateOff;
    *ndsP4SnakeItemWord(ip, SNAKE_ITEM_HIT_FLAG) = TRUE;
    ip->attack_coll.damage = 0;
    ip->hit_normal_damage = 0;
    SNAKE_ITEM_F32(ip, SNAKE_NIKITA_VAR_SPEED) = SNAKE_NIKITA_SPEED;

    if (angle_i == 361)
    {
        angle_i = 43;
    }
    turn = ftCommonDamageGetKnockbackAngle(angle_i, ip->ga, ip->damage_knockback) * 0.25F;
    damage = ip->damage_queue;

    if (damage != 0)
    {
        if (damage >= 25)
        {
            damage = 25;
        }
        turn += ((f32)damage / 25.0F) * 0.5F;
    }
    SNAKE_ITEM_F32(ip, SNAKE_NIKITA_VAR_ANGLE) += turn;

    return FALSE;
}

/* reflect_: turned around, and Snake loses control of it. */
static sb32 ndsP4SnakeNikitaProcReflector(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    SNAKE_ITEM_F32(ip, SNAKE_NIKITA_VAR_ANGLE) += ndsP4SnakeBitsToF32(SNAKE_NIKITA_PI_BITS);
    ndsP4SnakeNikitaRelease(ip);

    return FALSE;
}

/* exploding_main_: the bomb's events for one frame, then gone, Snake's
 * control cleared when it was his. */
static sb32 ndsP4SnakeNikitaExplodeProcUpdate(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);

    ndsP4SnakeExplodeUpdateAttackEvent(item_gobj, &sNdsP4SnakeNikitaDesc);
    ip->multi++;

    if (ip->multi == 1)
    {
        ndsP4SnakeNikitaRelease(ip);
        return TRUE;
    }
    return FALSE;
}

/* blast_zone_ (proc_dead). */
static sb32 ndsP4SnakeNikitaProcDead(GObj *item_gobj)
{
    ndsP4SnakeNikitaRelease(itGetStruct(item_gobj));
    return TRUE;
}

/* stage_setting_: a 100-tall box, 500 frames of fuse, his team and port,
 * Snake's Nikita (passive word 1), on his facing (180 degrees facing left),
 * still, hurtbox and a silent 1-damage hitbox on, no clash, no knockback. */
static GObj *ndsP4SnakeMakeNikita(GObj *fighter_gobj, Vec3f *pos)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    Vec3f vel = { 0.0F, 0.0F, 0.0F };
    GObj *item_gobj = ndsP4SnakeMakeItem(fighter_gobj, &sNdsP4SnakeNikitaDesc, pos, &vel);
    ITStruct *ip;

    if (item_gobj == NULL)
    {
        return NULL;
    }
    ip = itGetStruct(item_gobj);
    ip->coll_data.map_coll.center = 0.0F;
    ip->coll_data.map_coll.top = SNAKE_NIKITA_ECB;
    ip->coll_data.map_coll.bottom = -SNAKE_NIKITA_ECB;
    ip->coll_data.map_coll.width = SNAKE_NIKITA_ECB;
    ip->attack_coll.can_rehit_shield = TRUE;
    ip->lifetime = SNAKE_NIKITA_FUSE;
    ip->owner_gobj = fighter_gobj;
    ip->team = fp->team;
    ip->player = fp->player;
    *ndsP4SnakePassiveWord(fp, 1) = (s32)(uintptr_t)ip;
    *ndsP4SnakeItemWord(ip, SNAKE_ITEM_MAKER) = (s32)(uintptr_t)fp;
    *ndsP4SnakeItemWord(ip, SNAKE_NIKITA_VAR_ANGLE) = 0;
    *ndsP4SnakeItemWord(ip, SNAKE_NIKITA_VAR_HIT) = 0;
    *ndsP4SnakeItemWord(ip, SNAKE_NIKITA_VAR_DISABLED) = 0;

    if (fp->lr == -1)
    {
        SNAKE_ITEM_F32(ip, SNAKE_NIKITA_VAR_ANGLE) = ndsP4SnakeBitsToF32(SNAKE_NIKITA_PI_BITS);
    }
    ip->lr = fp->lr;
    *ndsP4SnakeItemWord(ip, SNAKE_NIKITA_VAR_SPEED) = 0;
    ip->damage_coll.hitstatus = nGMHitStatusNormal;
    ip->attack_coll.attack_state = nGMAttackStateNew;
    ip->attack_coll.damage = 1;
    ip->attack_coll.fgm_id = SNAKE_FGM_NONE;
    ip->attack_coll.can_setoff = FALSE;
    SNAKE_ITEM_F32(ip, SNAKE_ITEM_MAX_SPEED) = 63.0F;
    *ndsP4SnakeItemWord(ip, SNAKE_ITEM_ROT_DIR) = 0;
    *ndsP4SnakeItemWord(ip, SNAKE_ITEM_REFRESH) = 0;
    *ndsP4SnakeItemWord(ip, SNAKE_ITEM_HIT_FLAG) = FALSE;
    ip->proc_dead = ndsP4SnakeNikitaProcDead;
    ip->coll_data.cliff_id = 0;
    ip->attack_coll.knockback_scale = 0;
    ip->attack_coll.knockback_weight = 0;
    ip->attack_coll.knockback_base = 1;
    ndsP4SnakeItemCollideFromOwner(item_gobj, fp);

    return item_gobj;
}

/* Nikita.Start.goto_next: operated, grounded or aerial. */
static void ndsP4SnakeNikitaStartGotoNext(GObj *fighter_gobj)
{
    ndsP4SnakeSetStatus(fighter_gobj, (ftGetStruct(fighter_gobj)->ga == nMPKineticsGround) ?
                                          SNAKE_STATUS_NIKITA_OPERATION :
                                          SNAKE_STATUS_NIKITA_AIR_OPERATION);
}

/* Nikita.Start.main (both): on frame 41 the Nikita 280 ahead and 225 up. */
void ndsP4SnakeNikitaStartUpdate(GObj *fighter_gobj)
{
    if (fighter_gobj->anim_frame == SNAKE_NIKITA_SPAWN_FRAME)
    {
        Vec3f pos;

        ndsP4SnakeSpawnPos(ftGetStruct(fighter_gobj), 280.0F, 225.0F, &pos);
        (void)ndsP4SnakeMakeNikita(fighter_gobj, &pos);
    }
    ftAnimEndCheckSetStatus(fighter_gobj, ndsP4SnakeNikitaStartGotoNext);
}

static void ndsP4SnakeNikitaStartAirToGround(GObj *fighter_gobj)
{
    ndsP4SnakeAirToGround(fighter_gobj, SNAKE_STATUS_NIKITA_START);
}

void ndsP4SnakeNikitaStartAirMap(GObj *fighter_gobj)
{
    mpCommonProcFighterLanding(fighter_gobj, ndsP4SnakeNikitaStartAirToGround);
}

/* Nikita.Operation.main: no Nikita ends it; Z cuts its engine (out of gas,
 * its hitbox renewed at 1 damage); a hit it dealt is a success. */
void ndsP4SnakeNikitaOperationUpdate(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    ITStruct *ip = (ITStruct *)(uintptr_t)*ndsP4SnakePassiveWord(fp, 1);
    sb32 ground = (fp->ga == nMPKineticsGround) ? TRUE : FALSE;

    if (ip != NULL)
    {
        if (fp->input.pl.button_hold & SNAKE_BUTTON_Z)
        {
            *ndsP4SnakeItemWord(ip, SNAKE_NIKITA_VAR_DISABLED) = 1;
            ip->attack_coll.attack_state = nGMAttackStateNew;
            ip->attack_coll.damage = 1;
            itMainRefreshAttackColl(ip->item_gobj);
        }
        else if (*ndsP4SnakeItemWord(ip, SNAKE_NIKITA_VAR_HIT) != 0)
        {
            *ndsP4SnakePassiveWord(fp, 1) = 0;
            ndsP4SnakeSetStatus(fighter_gobj, ground ? SNAKE_STATUS_NIKITA_SUCCESS :
                                                       SNAKE_STATUS_NIKITA_AIR_SUCCESS);
            return;
        }
        else return;
    }
    *ndsP4SnakePassiveWord(fp, 1) = 0;
    ndsP4SnakeSetStatus(fighter_gobj, ground ? SNAKE_STATUS_NIKITA_END : SNAKE_STATUS_NIKITA_AIR_END);
}

static void ndsP4SnakeNikitaOperationAirToGround(GObj *fighter_gobj)
{
    ndsP4SnakeAirToGround(fighter_gobj, SNAKE_STATUS_NIKITA_OPERATION);
}

void ndsP4SnakeNikitaOperationAirMap(GObj *fighter_gobj)
{
    mpCommonProcFighterLanding(fighter_gobj, ndsP4SnakeNikitaOperationAirToGround);
}

static void ndsP4SnakeNikitaEndAirToGround(GObj *fighter_gobj)
{
    ndsP4SnakeAirToGround(fighter_gobj, SNAKE_STATUS_NIKITA_END);
}

void ndsP4SnakeNikitaEndAirMap(GObj *fighter_gobj)
{
    mpCommonProcFighterLanding(fighter_gobj, ndsP4SnakeNikitaEndAirToGround);
}

static void ndsP4SnakeNikitaSuccessAirToGround(GObj *fighter_gobj)
{
    ndsP4SnakeAirToGround(fighter_gobj, SNAKE_STATUS_NIKITA_SUCCESS);
}

void ndsP4SnakeNikitaSuccessAirMap(GObj *fighter_gobj)
{
    mpCommonProcFighterLanding(fighter_gobj, ndsP4SnakeNikitaSuccessAirToGround);
}

/* ---- Cypher (up special) ---- */

#define SNAKE_CYPHER_FUSE 500
#define SNAKE_CYPHER_SPAWN_FRAME 4.0F
#define SNAKE_CYPHER_ECB 50.0F
#define SNAKE_CYPHER_SCALE 1.5F
#define SNAKE_CYPHER_HURT_SIZE 150.0F
#define SNAKE_CYPHER_DAMAGE_LIMIT 13
#define SNAKE_CYPHER_RISE 1.0F
#define SNAKE_CYPHER_RISE_MAX 40.0F
#define SNAKE_CYPHER_RESIST 7                 /* DAMAGE_RESIST_THRESHOLD */
#define SNAKE_CYPHER_HANG_RISE 6.0F
#define SNAKE_CYPHER_HANG_RISE_MAX 40.0F
#define SNAKE_CYPHER_CANCEL_TICS 20
#define SNAKE_CYPHER_RELEASED 0x350

static sb32 ndsP4SnakeCypherProcUpdate(GObj *item_gobj);
static sb32 ndsP4SnakeCypherProcHit(GObj *item_gobj);
static sb32 ndsP4SnakeCypherProcDamage(GObj *item_gobj);

/* item_info_array: ITAttributes at 0x1E0 of his special file 1
 * (MERGED_FILERESOURCE_6_3); kind as the C4's. */
static ITDesc sNdsP4SnakeCypherDesc = {
    NDS_P4_IT_KIND_SNAKE_CYPHER, &gNdsP4SnakeSpecial1, (const void *)0x1E0,
    { nGCMatrixKindTraRotRpyR, nGCMatrixKindNull, 0x00 }, 0,
    ndsP4SnakeCypherProcUpdate, NULL, ndsP4SnakeCypherProcHit, ndsP4SnakeCypherProcHit,
    ndsP4SnakeCypherProcHit, NULL, NULL, ndsP4SnakeCypherProcDamage
};

/* cypher_main_: held, it rides 80 ahead of and 150 above his hand at his
 * speed while he is Snake in a Cypher status, else it lets go with its
 * hitbox and rises at 10; loose it rises 1 more a frame to 40. 13 damage
 * taken (with the explosion effect) or its fuse out ends it, and his
 * Cypher (temp variable 1) when it was this one. */
static sb32 ndsP4SnakeCypherProcUpdate(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    sb32 done = FALSE;

    if (*ndsP4SnakeItemWord(ip, SNAKE_CYPHER_RELEASED) != 0)
    {
        f32 vel_y = ip->physics.vel_air.y + SNAKE_CYPHER_RISE;

        ip->physics.vel_air.y = (vel_y <= SNAKE_CYPHER_RISE_MAX) ? vel_y : SNAKE_CYPHER_RISE_MAX;
    }
    else
    {
        FTStruct *owner = ftGetStruct(ip->owner_gobj);
        s32 status_id = owner->status_id;

        if ((owner->nds_p4_content != SNAKE_CONTENT) ||
            ((status_id != SNAKE_STATUS_CYPHER_START) &&
             (status_id != SNAKE_STATUS_CYPHER_AIR_START) &&
             (status_id != SNAKE_STATUS_CYPHER_AIR_HANG)))
        {
            *ndsP4SnakeItemWord(ip, SNAKE_CYPHER_RELEASED) = 1;
            ip->attack_coll.attack_state = nGMAttackStateNew;
            ip->physics.vel_air.y = 10.0F;
        }
        else
        {
            Vec3f pos = { 0.0F, 0.0F, 0.0F };
            DObj *dobj = DObjGetStruct(item_gobj);

            gmCollisionGetFighterPartsWorldPosition(owner->joints[SNAKE_JOINT_HAND], &pos);
            dobj->translate.vec.f.x = pos.x + (80.0F * (f32)owner->lr);
            dobj->translate.vec.f.y = pos.y + 150.0F;
            ip->physics.vel_air.x = owner->coll_data.pos_diff.x;
        }
    }
    if (ip->percent_damage >= SNAKE_CYPHER_DAMAGE_LIMIT)
    {
        efManagerSparkleWhiteMultiExplodeMakeEffect(&DObjGetStruct(item_gobj)->translate.vec.f);
        done = TRUE;
    }
    else if (ip->lifetime == 0)
    {
        done = TRUE;
    }
    else ip->lifetime--;

    if (done != FALSE)
    {
        FTStruct *owner = ftGetStruct(ip->owner_gobj);

        if (owner->motion_vars.flags.flag0 == (u32)(uintptr_t)item_gobj)
        {
            owner->motion_vars.flags.flag0 = 0;
        }
        return TRUE;
    }
    return FALSE;
}

/* cypher_hurtbox_collision_ (hit, shield, hop): gone (his temp variable 1
 * left naming it, as the donor leaves it). */
static sb32 ndsP4SnakeCypherProcHit(GObj *item_gobj)
{
    (void)item_gobj;
    return TRUE;
}

/* cypher_hitbox_collision_: hit, it takes no hitlag. */
static sb32 ndsP4SnakeCypherProcDamage(GObj *item_gobj)
{
    itGetStruct(item_gobj)->damage_lag = 0;
    return FALSE;
}

/* cypher_blast_zone_ (proc_dead): gone, nothing else. */
static sb32 ndsP4SnakeCypherProcDead(GObj *item_gobj)
{
    (void)item_gobj;
    return TRUE;
}

/* cypher_stage_setting_: a 100-tall box, 500 frames, his team and port,
 * held, its model at 1.5 turned 180 degrees (mirrored facing left), its
 * hurtbox on at 150 a side and its hitbox off: 6 damage at 42 degrees, 63
 * base and 17 growth, the medium kick sound, rehits shields. */
static GObj *ndsP4SnakeMakeCypher(GObj *fighter_gobj, Vec3f *pos)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    Vec3f vel = { 0.0F, 0.0F, 0.0F };
    GObj *item_gobj = ndsP4SnakeMakeItem(fighter_gobj, &sNdsP4SnakeCypherDesc, pos, &vel);
    ITStruct *ip;
    DObj *dobj;

    if (item_gobj == NULL)
    {
        return NULL;
    }
    ip = itGetStruct(item_gobj);
    dobj = DObjGetStruct(item_gobj);
    gcAddXObjForDObjFixed(dobj, 0x2E, 0);
    gcAddXObjForDObjFixed(dobj->child, 0x2E, 0);

    ip->coll_data.map_coll.center = 0.0F;
    ip->coll_data.map_coll.top = SNAKE_CYPHER_ECB;
    ip->coll_data.map_coll.bottom = -SNAKE_CYPHER_ECB;
    ip->coll_data.map_coll.width = SNAKE_CYPHER_ECB;
    ip->attack_coll.can_rehit_shield = TRUE;
    ip->lifetime = SNAKE_CYPHER_FUSE;
    *ndsP4SnakeItemWord(ip, 0x354) = 4;
    ip->owner_gobj = fighter_gobj;
    ip->team = fp->team;
    ip->player = fp->player;
    *ndsP4SnakeItemWord(ip, SNAKE_ITEM_MAKER) = (s32)(uintptr_t)fp;
    *ndsP4SnakeItemWord(ip, SNAKE_CYPHER_RELEASED) = 0;
    ip->attack_coll.attack_state = nGMAttackStateOff;
    dobj->rotate.vec.f.z = ndsP4SnakeBitsToF32(SNAKE_NIKITA_PI_BITS);
    dobj->scale.vec.f.x = dobj->scale.vec.f.y = dobj->scale.vec.f.z = SNAKE_CYPHER_SCALE;

    if (fp->lr == -1)
    {
        dobj->scale.vec.f.x = -dobj->scale.vec.f.x;
    }
    ip->lr = fp->lr;
    ip->damage_coll.hitstatus = nGMHitStatusNormal;
    ip->attack_coll.damage = 6;
    ip->attack_coll.angle = 42;
    ip->attack_coll.knockback_base = 63;
    ip->attack_coll.knockback_scale = 17;
    ip->attack_coll.fgm_id = SNAKE_FGM_KICK_M;
    ip->damage_coll.size.x = ip->damage_coll.size.y = ip->damage_coll.size.z = SNAKE_CYPHER_HURT_SIZE;
    SNAKE_ITEM_F32(ip, SNAKE_ITEM_MAX_SPEED) = 63.0F;
    *ndsP4SnakeItemWord(ip, SNAKE_ITEM_ROT_DIR) = 0;
    *ndsP4SnakeItemWord(ip, SNAKE_ITEM_REFRESH) = 0;
    ip->proc_dead = ndsP4SnakeCypherProcDead;
    ip->coll_data.cliff_id = 0;
    ndsP4SnakeItemCollideFromOwner(item_gobj, fp);

    return item_gobj;
}

/* Cypher.initial (ground and air): once per airtime (passive word 2), the
 * Cypher start, his fast fall and three other flags of that byte cleared,
 * his aerial jumps spent, no Cypher yet (temp variable 1). */
void ndsP4SnakeCypherInitial(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (*ndsP4SnakePassiveWord(fp, 2) != 0)
    {
        return;
    }
    *ndsP4SnakePassiveWord(fp, 2) = TRUE;
    ndsP4SnakeSetStatus(fighter_gobj, (fp->ga != nMPKineticsGround) ? SNAKE_STATUS_CYPHER_AIR_START :
                                                                      SNAKE_STATUS_CYPHER_START);
    fp->is_absorb = FALSE;
    fp->absorb_lr = 0;
    fp->is_goto_attack100 = FALSE;
    fp->is_fastfall = FALSE;
    fp->jumps_used = fp->attr->jumps_max;
    fp->motion_vars.flags.flag0 = 0;
}

/* Cypher.Start.goto_next: airborne, the hang, his damage resistance primed
 * at 7. */
static void ndsP4SnakeCypherStartGotoNext(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (fp->ga == nMPKineticsGround)
    {
        mpCommonSetFighterAir(fp);
    }
    ftMainSetStatus(fighter_gobj, SNAKE_STATUS_CYPHER_AIR_HANG, 0.0F, 1.0F, FTSTATUS_PRESERVE_NONE);
    ftGetStruct(fighter_gobj)->damage_resist = SNAKE_CYPHER_RESIST;
    ftMainPlayAnimEventsAll(fighter_gobj);
}

/* Cypher.Start.main (both): on frame 4 the Cypher at his hand (temp
 * variable 1). */
void ndsP4SnakeCypherStartUpdate(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (fighter_gobj->anim_frame == SNAKE_CYPHER_SPAWN_FRAME)
    {
        Vec3f pos;

        ndsP4SnakeHandPos(fp, &pos);
        fp->motion_vars.flags.flag0 = (u32)(uintptr_t)ndsP4SnakeMakeCypher(fighter_gobj, &pos);
    }
    ftAnimEndCheckSetStatus(fighter_gobj, ndsP4SnakeCypherStartGotoNext);
}

static void ndsP4SnakeCypherStartAirToGround(GObj *fighter_gobj)
{
    ndsP4SnakeAirToGround(fighter_gobj, SNAKE_STATUS_CYPHER_START);
}

void ndsP4SnakeCypherStartAirMap(GObj *fighter_gobj)
{
    mpCommonProcFighterLanding(fighter_gobj, ndsP4SnakeCypherStartAirToGround);
}

/* Cypher.Hang.goto_next: the cut. */
static void ndsP4SnakeCypherHangGotoNext(GObj *fighter_gobj)
{
    ndsP4SnakeSetStatus(fighter_gobj, SNAKE_STATUS_CYPHER_AIR_CUT);
}

/* Cypher.Hang.main: what his resistance absorbed since last frame (7 less
 * what is left) is taken as damage with its hitlag; his resistance renewed
 * at 7 (non-cumulative armour), the up special spent, his jumps spent, a
 * rise of 6 a frame to 40; the end, or a Cypher gone (stopping his rise),
 * cuts it. */
void ndsP4SnakeCypherHangUpdate(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);

    if (fp->damage_resist < SNAKE_CYPHER_RESIST)
    {
        s32 absorbed = SNAKE_CYPHER_RESIST - fp->damage_resist;

        ftParamUpdateDamage(fp, absorbed);
        fp->hitlag_tics = ftParamGetHitLag(absorbed, fp->status_id, 1.0F);
    }
    fp->is_damage_resist = TRUE;
    fp->damage_resist = SNAKE_CYPHER_RESIST;
    *ndsP4SnakePassiveWord(fp, 2) = TRUE;
    fp->jumps_used = fp->attr->jumps_max;
    fp->physics.vel_air.y += SNAKE_CYPHER_HANG_RISE;

    if (SNAKE_CYPHER_HANG_RISE_MAX <= fp->physics.vel_air.y)
    {
        fp->physics.vel_air.y = SNAKE_CYPHER_HANG_RISE_MAX;
    }
    ftAnimEndCheckSetStatus(fighter_gobj, ndsP4SnakeCypherHangGotoNext);

    if (fp->motion_vars.flags.flag0 == 0)
    {
        fp->physics.vel_air.y = 0.0F;
        ndsP4SnakeCypherHangGotoNext(fighter_gobj);
    }
}

/* Cypher.Hang.interrupt: from his 20th frame the aerial interrupts (any of
 * them changing his status is the donor's "returned 1"), else the stick
 * down past -53 cuts it. */
void ndsP4SnakeCypherHangInterrupt(GObj *fighter_gobj)
{
    FTStruct *fp = ftGetStruct(fighter_gobj);
    s32 status_id = fp->status_id;

    if ((s32)fp->status_total_tics < SNAKE_CYPHER_CANCEL_TICS)
    {
        return;
    }
    ftCommonJumpProcInterrupt(fighter_gobj);

    if (fp->status_id != status_id)
    {
        return;
    }
    if (fp->input.pl.stick_range.y < -53)
    {
        ftMainSetStatus(fighter_gobj, SNAKE_STATUS_CYPHER_AIR_CUT, 0.0F, 1.0F, FTSTATUS_PRESERVE_NONE);
    }
}

/* ---- Script hooks ---- */

/* initial_script_ and grounded_script_ (spawn, rebirth, every landing):
 * the up special back (passive word 2), then the default case. */
static void ndsP4SnakeOnInit(FTStruct *fp)
{
    *ndsP4SnakePassiveWord(fp, 2) = 0;
}

static void ndsP4SnakeOnLanding(FTStruct *fp, sb32 cliff)
{
    (void)cliff;
    *ndsP4SnakePassiveWord(fp, 2) = 0;
}

/* Cypher.Refresh (on_hit): hit, the up special comes back. */
static void ndsP4SnakeOnDamage(FTStruct *fp)
{
    *ndsP4SnakePassiveWord(fp, 2) = 0;
}

/* Remix Item.asm's tables for his grenade, the one item of his anyone may
 * pick up: DROP_ITEM is flashbang_begin_main_, THROW_ITEM throw_initial_,
 * PICKUP_ITEM_INIT none (a held grenade keeps its status). */
static sb32 ndsP4SnakeItem(GObj *item_gobj, s32 which)
{
    if (itGetStruct(item_gobj)->kind != NDS_P4_IT_KIND_SNAKE_GRENADE)
    {
        return FALSE;
    }
    switch (which)
    {
    case NDS_P4_ITEM_DROPPED:
        ndsP4SnakeGrenadeBeginMain(item_gobj);
        break;
    case NDS_P4_ITEM_THROWN:
        ndsP4SnakeGrenadeThrown(item_gobj);
        break;
    default:
        break;
    }
    return TRUE;
}

/* ---- CPU (AI.asm) ---- */

/* AI.ROUTINE / ATTACK_TABLE inputs the routines issue. */
#define SNAKE_COMPUTER_NSPA 0x09
#define SNAKE_COMPUTER_DSPG 0x1B
#define SNAKE_COMPUTER_GRAB 0x1C
#define SNAKE_COMPUTER_STICK_DOWN 0x27

/* cvt.w.s in the FPU's default mode: to nearest, ties to even. */
static s32 ndsP4SnakeRoundNearest(f32 v)
{
    s32 i = (s32)v;
    f32 rem = v - (f32)i;

    if ((rem > 0.5F) || ((rem == 0.5F) && (i & 1)))
    {
        i++;
    }
    else if ((rem < -0.5F) || ((rem == -0.5F) && (i & 1)))
    {
        i--;
    }
    return i;
}

/* recovery_logic: hanging on the Cypher, rising, past the nearer ledge's
 * height at his catch height (the attributes' catch box y, read for his
 * reach too), short of the ledge he holds down to drop it. */
static void ndsP4SnakeComputerRecover(FTStruct *fp)
{
    FTComputer *com = &fp->computer;
    f32 x;
    f32 y;
    f32 ledge_x;
    f32 ledge_y;
    f32 catch_y;
    f32 reach_x;
    f32 vel_y;

    if (fp->status_id != SNAKE_STATUS_CYPHER_AIR_HANG)
    {
        return;
    }
    x = fp->coll_data.p_translate->x;
    y = fp->coll_data.p_translate->y;

    if (ABSF(com->cliff_left_pos.x - x) <= ABSF(com->cliff_right_pos.x - x))
    {
        ledge_x = com->cliff_left_pos.x;
        ledge_y = com->cliff_left_pos.y;
    }
    else
    {
        ledge_x = com->cliff_right_pos.x;
        ledge_y = com->cliff_right_pos.y;
    }
    catch_y = y + fp->attr->cliffcatch_coll.y;
    reach_x = x + ((f32)fp->lr * fp->attr->cliffcatch_coll.y);
    vel_y = fp->physics.vel_air.y;

    if (vel_y <= 0.0F)
    {
        return;
    }
    catch_y += vel_y;

    if (catch_y <= ledge_y)
    {
        return;
    }
    if (((ledge_x - x) < 0.0F) ? !(ledge_x <= reach_x) : !(reach_x <= ledge_x))
    {
        ndsP4ComputerSetCommandImmediate(fp, SNAKE_COMPUTER_STICK_DOWN);
    }
}

/* The target fighter's position, its body's centre when `centre`. */
static sb32 ndsP4SnakeComputerTarget(FTStruct *fp, sb32 centre, f32 *x, f32 *y)
{
    GObj *target = fp->computer.target_gobj;
    FTStruct *tfp;

    if (target == NULL)
    {
        return FALSE;
    }
    tfp = ftGetStruct(target);
    *x = tfp->coll_data.p_translate->x;
    *y = tfp->coll_data.p_translate->y + ((centre != FALSE) ? tfp->coll_data.map_coll.center : 0.0F);

    return TRUE;
}

/* cpu_post_process's nikita_control: over the stage with a Nikita out and a
 * target, no command, the stick 80 toward the target's centre from the
 * Nikita. */
static void ndsP4SnakeComputerNikita(FTStruct *fp)
{
    ITStruct *ip = (ITStruct *)(uintptr_t)*ndsP4SnakePassiveWord(fp, 1);
    DObj *dobj;
    f32 tx;
    f32 ty;
    f32 dx;
    f32 dy;
    f32 d2;

    if ((fp->coll_data.floor_line_id == -1) || (ip == NULL) || (fp->computer.target_gobj == NULL))
    {
        return;
    }
    ndsP4ComputerSetCommandImmediate(fp, nNDSP4ComputerInputNull);
    dobj = DObjGetStruct(ip->item_gobj);
    (void)ndsP4SnakeComputerTarget(fp, TRUE, &tx, &ty);
    dx = tx - dobj->translate.vec.f.x;
    dy = ty - dobj->translate.vec.f.y;
    d2 = (dx * dx) + (dy * dy);

    if (d2 == 0.0F)
    {
        fp->input.cp.stick_range.x = 0;
        fp->input.cp.stick_range.y = 0;
    }
    else
    {
        f32 k = 80.0F / sqrtf(d2);

        fp->input.cp.stick_range.x = ndsP4SnakeRoundNearest(dx * k);
        fp->input.cp.stick_range.y = ndsP4SnakeRoundNearest(dy * k);
    }
}

/* cpu_post_process's grenade_control: off the stage it throws; else one in
 * four frames it keeps B held (one in four of those a grab, the shield's
 * drop); else the stick by the target's distance (as assembled: beyond 500
 * back, within it forward) and B let go, no command. */
static void ndsP4SnakeComputerGrenade(FTStruct *fp)
{
    if (fp->coll_data.floor_line_id != -1)
    {
        f32 tx;
        f32 ty;
        f32 dx;
        f32 dy;
        s32 stick;

        if (syUtilsRandIntRange(4) == 0)
        {
            fp->input.cp.button_inputs |= SNAKE_BUTTON_B;
            ndsP4ComputerSetCommandImmediate(fp, nNDSP4ComputerInputNull);

            if (syUtilsRandIntRange(4) == 0)
            {
                ndsP4ComputerSetCommandImmediate(fp, SNAKE_COMPUTER_GRAB);
            }
            return;
        }
        if (ndsP4SnakeComputerTarget(fp, FALSE, &tx, &ty) == FALSE)
        {
            return;
        }
        dx = tx - fp->coll_data.p_translate->x;
        dy = ty - fp->coll_data.p_translate->y;
        stick = (sqrtf((dx * dx) + (dy * dy)) <= 500.0F) ? 80 : -80;
        fp->input.cp.stick_range.x = stick * fp->lr;
    }
    fp->input.cp.button_inputs = 0;
    ndsP4ComputerSetCommandImmediate(fp, nNDSP4ComputerInputNull);
}

/* cpu_post_process's grenade_out_of_tumble: on tumble's first frame, one in
 * four, a target within 1000 gets a grenade (the aerial special). */
static void ndsP4SnakeComputerTumble(FTStruct *fp)
{
    f32 tx;
    f32 ty;
    f32 dx;
    f32 dy;

    if ((fp->status_total_tics != 1) || (syUtilsRandIntRange(4) != 0) ||
        (ndsP4SnakeComputerTarget(fp, FALSE, &tx, &ty) == FALSE))
    {
        return;
    }
    dx = tx - fp->coll_data.p_translate->x;
    dy = ty - fp->coll_data.p_translate->y;

    if (sqrtf((dx * dx) + (dy * dy)) <= 1000.0F)
    {
        ndsP4ComputerSetCommandImmediate(fp, SNAKE_COMPUTER_NSPA);
    }
}

/* cpu_post_process's dsp_check: with a C4 out, a down special already
 * running gets no command; else over the stage a target's centre within
 * 800 of the C4 sets it off. */
static void ndsP4SnakeComputerC4(FTStruct *fp)
{
    ITStruct *ip = (ITStruct *)(uintptr_t)*ndsP4SnakePassiveWord(fp, 0);
    DObj *dobj;
    f32 tx;
    f32 ty;
    f32 dx;
    f32 dy;

    if (ip == NULL)
    {
        return;
    }
    if (fp->computer.p_command == dFTComputerPlayerInputScripts[SNAKE_COMPUTER_DSPG])
    {
        ndsP4ComputerSetCommandImmediate(fp, nNDSP4ComputerInputNull);
        return;
    }
    if ((fp->coll_data.floor_line_id == -1) ||
        (ndsP4SnakeComputerTarget(fp, TRUE, &tx, &ty) == FALSE))
    {
        return;
    }
    dobj = DObjGetStruct(ip->item_gobj);
    dx = tx - dobj->translate.vec.f.x;
    dy = ty - dobj->translate.vec.f.y;

    if (sqrtf((dx * dx) + (dy * dy)) <= 800.0F)
    {
        ndsP4ComputerSetCommandImmediate(fp, SNAKE_COMPUTER_DSPG);
    }
}

/* cpu_post_process. */
static void ndsP4SnakeComputerPostProcess(FTStruct *fp)
{
    s32 status_id = fp->status_id;

    if ((status_id == SNAKE_STATUS_NIKITA_OPERATION) ||
        (status_id == SNAKE_STATUS_NIKITA_AIR_OPERATION))
    {
        ndsP4SnakeComputerNikita(fp);
    }
    else if ((status_id == SNAKE_STATUS_GRENADE_START) || (status_id == SNAKE_STATUS_GRENADE_WAIT) ||
             (status_id == SNAKE_STATUS_GRENADE_START_AIR) ||
             (status_id == SNAKE_STATUS_GRENADE_WAIT_AIR) ||
             (status_id == SNAKE_STATUS_GRENADE_WALK_F) || (status_id == SNAKE_STATUS_GRENADE_WALK_B))
    {
        ndsP4SnakeComputerGrenade(fp);
    }
    else if (status_id == nFTCommonStatusDamageFall)
    {
        ndsP4SnakeComputerTumble(fp);
    }
    else ndsP4SnakeComputerC4(fp);
}

const NDSP4Computer gNdsP4SnakeComputer = {
    .recover = ndsP4SnakeComputerRecover,
    .post_process = ndsP4SnakeComputerPostProcess,
};

/* Character.action_replace_map. */
static const u16 sNdsP4SnakeStatusReplace[] = {
    nFTCommonStatusAttack12, 0xEF,
    nFTCommonStatusAttackDash, 0xF0,
    nFTCommonStatusSquatWait, 0xF1,
    nFTCommonStatusAttackS3, 0xF4,
    nFTCommonStatusAttackHi4, 0x11B,
    nFTCommonStatusAppeal, 0x11C,
    nFTCommonStatusAttackS4, SNAKE_STATUS_NIKITA_START,
    0xFFFF
};
_Static_assert((nFTCommonStatusAttack12 == 0xBF) && (nFTCommonStatusAttackDash == 0xC0) &&
               (nFTCommonStatusSquatWait == 0x1D) && (nFTCommonStatusAttackS3 == 0xC3) &&
               (nFTCommonStatusAttackHi4 == 0xCF) && (nFTCommonStatusAppeal == 0xBD) &&
               (nFTCommonStatusAttackS4 == 0xCC), "Snake action_replace_map");

/* custom_capture_dk_interrupt (capture_action_fix_): his victims cannot
 * mash out of ThrownDK, here Shouldered. */
static sb32 ndsP4SnakeShoulderedInterrupt(GObj *victim_gobj)
{
    (void)victim_gobj;
    return TRUE;
}

const NDSP4Overrides gNdsP4SnakeOverrides = {
    .on_init = ndsP4SnakeOnInit,
    .on_landing = ndsP4SnakeOnLanding,
    .on_damage = ndsP4SnakeOnDamage,
    .item = ndsP4SnakeItem,
    .shouldered_interrupt = ndsP4SnakeShoulderedInterrupt,
    .status_replace = sNdsP4SnakeStatusReplace,
};

#endif /* NDS_P4_SNAKE */
