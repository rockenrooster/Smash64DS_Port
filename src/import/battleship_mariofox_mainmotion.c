#include <PR/ultratypes.h>
#include <ft/fighter.h>

/*
 * Bounded motion-command extract for the dash-run attack-event proof.
 *
 * Source of truth:
 *   decomp/BattleShip-main/decomp/src/relocData/202_MarioMainMotion.c
 *   decomp/BattleShip-main/decomp/src/relocData/208_FoxMainMotion.c
 *
 * The full relocData files pull in the whole item/map/audio enum surface. This
 * milestone only needs the selected original hitbox command words scanned by
 * ftMainPlayAnimEventsAll, so keep the extract narrow until the full motion
 * script runtime is imported.
 */

#define NDS_FTMOTION_EVENT_END 0u
#define NDS_FTMOTION_EVENT_ASYNC_WAIT 2u
#define NDS_FTMOTION_EVENT_MAKE_ATTACK_COLL 3u
#define NDS_FTMOTION_EVENT_CLEAR_ATTACK_COLL_ALL 6u
#define NDS_FTMOTION_EVENT_SET_ATTACK_COLL_DAMAGE 8u
#define NDS_FTMOTION_EVENT_SET_ATTACK_COLL_SIZE 9u
#define NDS_FTMOTION_FIELD(value, shift, width) \
    ((((u32)(value)) & ((1u << (width)) - 1u)) << (shift))

#define NDS_FTMOTION_CMD_END() \
    NDS_FTMOTION_FIELD(NDS_FTMOTION_EVENT_END, 26, 6)

#define NDS_FTMOTION_CMD_WAIT_ASYNC(frames) \
    (NDS_FTMOTION_FIELD(NDS_FTMOTION_EVENT_ASYNC_WAIT, 26, 6) | \
     NDS_FTMOTION_FIELD((frames), 0, 26))

#define NDS_FTMOTION_CMD_MAKE_ATTACK_COLL_S1(aid, gid, jid, dmg, reb, elem) \
    (NDS_FTMOTION_FIELD(NDS_FTMOTION_EVENT_MAKE_ATTACK_COLL, 26, 6) | \
     NDS_FTMOTION_FIELD((aid), 23, 3) | \
     NDS_FTMOTION_FIELD((gid), 20, 3) | \
     NDS_FTMOTION_FIELD((jid), 13, 7) | \
     NDS_FTMOTION_FIELD((dmg), 5, 8) | \
     NDS_FTMOTION_FIELD((reb), 4, 1) | \
     NDS_FTMOTION_FIELD((elem), 0, 4))

#define NDS_FTMOTION_CMD_MAKE_ATTACK_COLL_S2(size, ox) \
    (NDS_FTMOTION_FIELD((size), 16, 16) | \
     NDS_FTMOTION_FIELD((ox), 0, 16))

#define NDS_FTMOTION_CMD_MAKE_ATTACK_COLL_S3(oy, oz) \
    (NDS_FTMOTION_FIELD((oy), 16, 16) | \
     NDS_FTMOTION_FIELD((oz), 0, 16))

#define NDS_FTMOTION_CMD_MAKE_ATTACK_COLL_S4(angle, kbs, kbw, ga) \
    (NDS_FTMOTION_FIELD((angle), 22, 10) | \
     NDS_FTMOTION_FIELD((kbs), 12, 10) | \
     NDS_FTMOTION_FIELD((kbw), 2, 10) | \
     NDS_FTMOTION_FIELD((ga), 0, 2))

#define NDS_FTMOTION_CMD_MAKE_ATTACK_COLL_S5(shield, fgm_level, fgm_kind, bkb) \
    (NDS_FTMOTION_FIELD((shield), 24, 8) | \
     NDS_FTMOTION_FIELD((fgm_level), 21, 3) | \
     NDS_FTMOTION_FIELD((fgm_kind), 17, 4) | \
     NDS_FTMOTION_FIELD((bkb), 7, 10))

#define NDS_FTMOTION_CMD_MAKE_ATTACK_COLL(aid, gid, jid, dmg, reb, elem, \
                                          size, ox, oy, oz, angle, kbs, kbw, \
                                          ga, shield, fgm_level, fgm_kind, bkb) \
    NDS_FTMOTION_CMD_MAKE_ATTACK_COLL_S1((aid), (gid), (jid), (dmg), (reb), \
                                         (elem)), \
    NDS_FTMOTION_CMD_MAKE_ATTACK_COLL_S2((size), (ox)), \
    NDS_FTMOTION_CMD_MAKE_ATTACK_COLL_S3((oy), (oz)), \
    NDS_FTMOTION_CMD_MAKE_ATTACK_COLL_S4((angle), (kbs), (kbw), (ga)), \
    NDS_FTMOTION_CMD_MAKE_ATTACK_COLL_S5((shield), (fgm_level), (fgm_kind), \
                                         (bkb))

#define NDS_FTMOTION_CMD_CLEAR_ATTACK_COLL_ALL() \
    NDS_FTMOTION_FIELD(NDS_FTMOTION_EVENT_CLEAR_ATTACK_COLL_ALL, 26, 6)

#define NDS_FTMOTION_CMD_SET_ATTACK_COLL_DAMAGE(aid, dmg) \
    (NDS_FTMOTION_FIELD(NDS_FTMOTION_EVENT_SET_ATTACK_COLL_DAMAGE, 26, 6) | \
     NDS_FTMOTION_FIELD((aid), 23, 3) | \
     NDS_FTMOTION_FIELD((dmg), 15, 8))

#define NDS_FTMOTION_CMD_SET_ATTACK_COLL_SIZE(aid, size) \
    (NDS_FTMOTION_FIELD(NDS_FTMOTION_EVENT_SET_ATTACK_COLL_SIZE, 26, 6) | \
     NDS_FTMOTION_FIELD((aid), 23, 3) | \
     NDS_FTMOTION_FIELD((size), 7, 16))

/* 202_MarioMainMotion.c:986-988 */
static const u32 dNDSMarioMainMotionJab1[] = {
    NDS_FTMOTION_CMD_WAIT_ASYNC(2),
    NDS_FTMOTION_CMD_MAKE_ATTACK_COLL(0, 0, 10, 2, 1, 0, 160, 0, 0, 0,
                                      361, 50, 0, 3, 0, 0, 0, 8),
    NDS_FTMOTION_CMD_MAKE_ATTACK_COLL(1, 0, 9, 2, 1, 0, 160, 0, 0, 0,
                                      361, 50, 0, 3, 0, 0, 0, 8),
    NDS_FTMOTION_CMD_END()
};

/* 202_MarioMainMotion.c:998-1004, US branch */
static const u32 dNDSMarioMainMotionJab2[] = {
    NDS_FTMOTION_CMD_WAIT_ASYNC(3),
    NDS_FTMOTION_CMD_MAKE_ATTACK_COLL(0, 0, 16, 2, 1, 0, 180, 16, 0, 0,
                                      70, 50, 0, 3, 0, 0, 0, 8),
    NDS_FTMOTION_CMD_MAKE_ATTACK_COLL(1, 0, 15, 2, 1, 0, 180, 0, 0, 0,
                                      70, 50, 0, 3, 0, 0, 0, 8),
    NDS_FTMOTION_CMD_END()
};

/* 202_MarioMainMotion.c:1015-1028 */
static const u32 dNDSMarioMainMotionJab3[] = {
    NDS_FTMOTION_CMD_WAIT_ASYNC(3),
    NDS_FTMOTION_CMD_MAKE_ATTACK_COLL(0, 0, 25, 4, 0, 0, 150, 0, 0, 0,
                                      361, 100, 0, 3, 0, 1, 1, 10),
    NDS_FTMOTION_CMD_MAKE_ATTACK_COLL(1, 0, 27, 4, 0, 0, 280, 0, 0, 0,
                                      361, 100, 0, 3, 0, 1, 1, 10),
    NDS_FTMOTION_CMD_SET_ATTACK_COLL_DAMAGE(0, 4),
    NDS_FTMOTION_CMD_SET_ATTACK_COLL_DAMAGE(1, 4),
    NDS_FTMOTION_CMD_SET_ATTACK_COLL_SIZE(0, 180),
    NDS_FTMOTION_CMD_SET_ATTACK_COLL_SIZE(1, 280),
    NDS_FTMOTION_CMD_CLEAR_ATTACK_COLL_ALL(),
    NDS_FTMOTION_CMD_END()
};

/* 208_FoxMainMotion.c:1019-1022 */
static const u32 dNDSFoxMainMotionJab1[] = {
    NDS_FTMOTION_CMD_WAIT_ASYNC(3),
    NDS_FTMOTION_CMD_MAKE_ATTACK_COLL(0, 0, 8, 4, 1, 0, 200, 140, 0, 0,
                                      70, 100, 0, 3, 0, 0, 0, 0),
    NDS_FTMOTION_CMD_MAKE_ATTACK_COLL(1, 0, 8, 4, 1, 0, 200, 0, 0, 0,
                                      70, 100, 0, 3, 0, 0, 0, 0),
    NDS_FTMOTION_CMD_END()
};

/* 208_FoxMainMotion.c:1032-1035 */
static const u32 dNDSFoxMainMotionJab2[] = {
    NDS_FTMOTION_CMD_WAIT_ASYNC(4),
    NDS_FTMOTION_CMD_MAKE_ATTACK_COLL(0, 0, 14, 4, 1, 0, 200, 140, 0, 0,
                                      70, 100, 0, 3, 0, 0, 0, 0),
    NDS_FTMOTION_CMD_MAKE_ATTACK_COLL(1, 0, 14, 4, 1, 0, 200, 0, 0, 0,
                                      70, 100, 0, 3, 0, 0, 0, 0),
    NDS_FTMOTION_CMD_END()
};

/* 208_FoxMainMotion.c:1045-1046 */
static const u32 dNDSFoxMainMotionJabLoopStart[] = {
    NDS_FTMOTION_CMD_END()
};

const u32 *ndsBattleShipMarioFoxMainMotionScript(s32 fkind, s32 motion_id)
{
    if (fkind == nFTKindMario)
    {
        switch (motion_id)
        {
        case nFTCommonMotionAttack11:
            return dNDSMarioMainMotionJab1;
        case nFTCommonMotionAttack12:
            return dNDSMarioMainMotionJab2;
        case nFTMarioMotionAttack13:
            return dNDSMarioMainMotionJab3;
        default:
            return NULL;
        }
    }
    if (fkind == nFTKindFox)
    {
        switch (motion_id)
        {
        case nFTCommonMotionAttack11:
            return dNDSFoxMainMotionJab1;
        case nFTCommonMotionAttack12:
            return dNDSFoxMainMotionJab2;
        case nFTFoxMotionAttack100Start:
            return dNDSFoxMainMotionJabLoopStart;
        default:
            return NULL;
        }
    }
    return NULL;
}
