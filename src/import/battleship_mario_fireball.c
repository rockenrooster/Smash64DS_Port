#if NDS_IMPORT_BATTLESHIP_MARIO_FIREBALL

#include <common.h>
#include <ef/effect.h>
#include <ft/fighter.h>
#include <gm/gmsound.h>
#include <mp/map.h>
#include <reloc_data.h>
#include <sc/scene.h>
#include <sys/audio.h>
#include <wp/weapon.h>

#ifndef DObjGetStruct
#define DObjGetStruct(gobj) ((DObj *)((gobj)->obj))
#endif

#ifndef FTMARIO_FIREBALL_SPAWN_JOINT
#define FTMARIO_FIREBALL_SPAWN_JOINT 16
#endif

uintptr_t llMarioMainFireballWeaponAttributes;
uintptr_t llMarioSpecial1FireballWeaponAttributes;
uintptr_t llLuigiSpecial1FireballWeaponAttributes;

extern f32 lbCommonMag2D(Vec3f *vec);
extern Vec3f *syVectorRotateAbout3D(Vec3f *dst, Vec3f *dir, f32 angle);
extern u16 gFTManagerMotionCount;
extern u16 gFTManagerStatUpdateCount;
void ftMarioSpecialAirNSwitchStatusGround(GObj *fighter_gobj);
void ftMarioSpecialNSwitchStatusAir(GObj *fighter_gobj);

#define NDS_FIREBALL_BRIDGE __attribute__((weak))

static const f32 dNDSFireballStaleTable[] = { 0.75F, 0.82F, 0.89F, 0.96F };

NDS_FIREBALL_BRIDGE f32 lbCommonNormDist2D(Vec3f *vec)
{
    f32 magnitude = sqrtf(SQUARE(vec->x) + SQUARE(vec->y));

    if (magnitude == 0.0F)
    {
        return 0.0F;
    }
    vec->x *= 1.0F / magnitude;
    vec->y *= 1.0F / magnitude;
    return magnitude;
}

NDS_FIREBALL_BRIDGE f32 lbCommonSim2D(Vec3f *a, Vec3f *b)
{
    f32 magnitude_a = sqrtf(SQUARE(a->x) + SQUARE(a->y));
    f32 magnitude_b = sqrtf(SQUARE(b->x) + SQUARE(b->y));

    return (a->x * b->x + a->y * b->y) / (magnitude_b + magnitude_a);
}

NDS_FIREBALL_BRIDGE void lbCommonDObjScaleXProcDisplay(GObj *gobj)
{
    (void)gobj;
}

NDS_FIREBALL_BRIDGE f32 ftParamGetStale(s32 player, s32 attack_id,
                                        u16 motion_count)
{
    s32 stale_id;
    s32 start_array_id;
    s32 current_array_id;
    s32 i;

    if ((gSCManagerBattleState == NULL) ||
        (player >= (s32)ARRAY_COUNT(gSCManagerBattleState->players)) ||
        (attack_id == nFTMotionAttackIDNone))
    {
        return 1.0F;
    }
    stale_id = gSCManagerBattleState->players[player].stale_id;
    current_array_id = start_array_id =
        (stale_id != 0) ?
            stale_id - 1 :
            ARRAY_COUNT(gSCManagerBattleState->players[player].stale_info) - 1;

    for (i = 0; i < (s32)ARRAY_COUNT(dNDSFireballStaleTable); i++)
    {
        if (attack_id ==
            gSCManagerBattleState->players[player]
                .stale_info[current_array_id]
                .attack_id)
        {
            if (motion_count !=
                gSCManagerBattleState->players[player]
                    .stale_info[current_array_id]
                    .motion_count)
            {
                return dNDSFireballStaleTable[i];
            }
            if (current_array_id == start_array_id)
            {
                i--;
            }
        }
        current_array_id =
            (current_array_id != 0) ?
                current_array_id - 1 :
                ARRAY_COUNT(gSCManagerBattleState->players[player].stale_info) -
                    1;
    }
    return 1.0F;
}

NDS_FIREBALL_BRIDGE u16 ftParamGetMotionCount(void)
{
    u16 motion_count = gFTManagerMotionCount++;

    if (gFTManagerMotionCount == 0)
    {
        gFTManagerMotionCount = 1;
    }
    return motion_count;
}

NDS_FIREBALL_BRIDGE u16 ftParamGetStatUpdateCount(void)
{
    u16 update_count = gFTManagerStatUpdateCount++;

    if (gFTManagerStatUpdateCount == 0)
    {
        gFTManagerStatUpdateCount = 1;
    }
    return update_count;
}

NDS_FIREBALL_BRIDGE void mpProcessRunLWallCollisionAdjNew(
    MPCollData *coll_data)
{
    (void)mpProcessCheckTestLWallCollisionAdjNew(coll_data);
}

NDS_FIREBALL_BRIDGE void mpProcessRunRWallCollisionAdjNew(
    MPCollData *coll_data)
{
    (void)mpProcessCheckTestRWallCollisionAdjNew(coll_data);
}

NDS_FIREBALL_BRIDGE void mpProcessRunCeilEdgeAdjust(MPCollData *coll_data)
{
    (void)coll_data;
}

NDS_FIREBALL_BRIDGE sb32 mpProcessRunFloorCollisionAdjNewNULL(
    MPCollData *coll_data)
{
    (void)coll_data;
    return FALSE;
}

NDS_FIREBALL_BRIDGE void mpCommonRunWeaponCollisionDefault(
    GObj *weapon_gobj, Vec3f *pos, MPCollData *coll_data)
{
    WPStruct *wp = wpGetStruct(weapon_gobj);

    if ((wp == NULL) || (pos == NULL) || (coll_data == NULL))
    {
        return;
    }
    wp->coll_data.pos_prev = *pos;
    wp->coll_data.p_map_coll = &coll_data->map_coll;
    wp->coll_data.update_tic = coll_data->update_tic;
}

#include "../../decomp/BattleShip-main/decomp/src/wp/wpmario/wpmariofireball.c"
#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftmario/ftmariospecialn.c"

#define NDS_FIREBALL_WEAK_STATUS(name)                 \
    __attribute__((weak)) void name(GObj *fighter_gobj) \
    {                                                   \
        (void)fighter_gobj;                             \
    }

// ponytail: unowned neutral-special setters stay weak until their TUs land.
NDS_FIREBALL_WEAK_STATUS(ftKirbyCopyMarioSpecialNSetStatus)
NDS_FIREBALL_WEAK_STATUS(ftKirbyCopyFoxSpecialNSetStatus)
NDS_FIREBALL_WEAK_STATUS(ftKirbyCopyDonkeySpecialNStartSetStatus)
NDS_FIREBALL_WEAK_STATUS(ftKirbyCopySamusSpecialNStartSetStatus)
NDS_FIREBALL_WEAK_STATUS(ftKirbyCopyLinkSpecialNSetStatus)
NDS_FIREBALL_WEAK_STATUS(ftKirbyCopyYoshiSpecialNSetStatus)
NDS_FIREBALL_WEAK_STATUS(ftKirbyCopyCaptainSpecialNSetStatus)
NDS_FIREBALL_WEAK_STATUS(ftKirbySpecialNStartSetStatus)
NDS_FIREBALL_WEAK_STATUS(ftKirbyCopyPikachuSpecialNSetStatus)
NDS_FIREBALL_WEAK_STATUS(ftKirbyCopyPurinSpecialNSetStatus)
NDS_FIREBALL_WEAK_STATUS(ftKirbyCopyNessSpecialNSetStatus)
NDS_FIREBALL_WEAK_STATUS(ftFoxSpecialNSetStatus)
NDS_FIREBALL_WEAK_STATUS(ftDonkeySpecialNStartSetStatus)
NDS_FIREBALL_WEAK_STATUS(ftSamusSpecialNStartSetStatus)
NDS_FIREBALL_WEAK_STATUS(ftLinkSpecialNSetStatus)
NDS_FIREBALL_WEAK_STATUS(ftYoshiSpecialNSetStatus)
NDS_FIREBALL_WEAK_STATUS(ftCaptainSpecialNSetStatus)
NDS_FIREBALL_WEAK_STATUS(ftPikachuSpecialNSetStatus)
NDS_FIREBALL_WEAK_STATUS(ftPurinSpecialNSetStatus)
NDS_FIREBALL_WEAK_STATUS(ftNessSpecialNSetStatus)
NDS_FIREBALL_WEAK_STATUS(ftKirbyCopyMarioSpecialAirNSetStatus)
NDS_FIREBALL_WEAK_STATUS(ftKirbyCopyFoxSpecialAirNSetStatus)
NDS_FIREBALL_WEAK_STATUS(ftKirbyCopyDonkeySpecialAirNStartSetStatus)
NDS_FIREBALL_WEAK_STATUS(ftKirbyCopySamusSpecialAirNStartSetStatus)
NDS_FIREBALL_WEAK_STATUS(ftKirbyCopyLinkSpecialAirNSetStatus)
NDS_FIREBALL_WEAK_STATUS(ftKirbyCopyYoshiSpecialAirNSetStatus)
NDS_FIREBALL_WEAK_STATUS(ftKirbyCopyCaptainSpecialAirNSetStatus)
NDS_FIREBALL_WEAK_STATUS(ftKirbySpecialAirNStartSetStatus)
NDS_FIREBALL_WEAK_STATUS(ftKirbyCopyPikachuSpecialAirNSetStatus)
NDS_FIREBALL_WEAK_STATUS(ftKirbyCopyPurinSpecialAirNSetStatus)
NDS_FIREBALL_WEAK_STATUS(ftKirbyCopyNessSpecialAirNSetStatus)
NDS_FIREBALL_WEAK_STATUS(ftFoxSpecialAirNSetStatus)
NDS_FIREBALL_WEAK_STATUS(ftDonkeySpecialAirNStartSetStatus)
NDS_FIREBALL_WEAK_STATUS(ftSamusSpecialAirNStartSetStatus)
NDS_FIREBALL_WEAK_STATUS(ftLinkSpecialAirNSetStatus)
NDS_FIREBALL_WEAK_STATUS(ftYoshiSpecialAirNSetStatus)
NDS_FIREBALL_WEAK_STATUS(ftCaptainSpecialAirNSetStatus)
NDS_FIREBALL_WEAK_STATUS(ftPikachuSpecialAirNSetStatus)
NDS_FIREBALL_WEAK_STATUS(ftPurinSpecialAirNSetStatus)
NDS_FIREBALL_WEAK_STATUS(ftNessSpecialAirNSetStatus)
NDS_FIREBALL_WEAK_STATUS(ftMarioSpecialAirHiSetStatus)
NDS_FIREBALL_WEAK_STATUS(ftFoxSpecialAirHiStartSetStatus)
NDS_FIREBALL_WEAK_STATUS(ftDonkeySpecialAirHiSetStatus)
NDS_FIREBALL_WEAK_STATUS(ftSamusSpecialAirHiSetStatus)
NDS_FIREBALL_WEAK_STATUS(ftLinkSpecialAirHiSetStatus)
NDS_FIREBALL_WEAK_STATUS(ftYoshiSpecialAirHiSetStatus)
NDS_FIREBALL_WEAK_STATUS(ftCaptainSpecialAirHiSetStatus)
NDS_FIREBALL_WEAK_STATUS(ftKirbySpecialAirHiSetStatus)
NDS_FIREBALL_WEAK_STATUS(ftPikachuSpecialAirHiStartSetStatus)
NDS_FIREBALL_WEAK_STATUS(ftPurinSpecialAirHiSetStatus)
NDS_FIREBALL_WEAK_STATUS(ftNessSpecialAirHiStartSetStatus)
NDS_FIREBALL_WEAK_STATUS(ftMarioSpecialAirLwSetStatus)
NDS_FIREBALL_WEAK_STATUS(ftFoxSpecialAirLwStartSetStatus)
NDS_FIREBALL_WEAK_STATUS(ftSamusSpecialAirLwSetStatus)
NDS_FIREBALL_WEAK_STATUS(ftLinkSpecialAirLwSetStatus)
NDS_FIREBALL_WEAK_STATUS(ftYoshiSpecialAirLwStartSetStatus)
NDS_FIREBALL_WEAK_STATUS(ftCaptainSpecialAirLwSetStatus)
NDS_FIREBALL_WEAK_STATUS(ftKirbySpecialAirLwStartSetStatus)
NDS_FIREBALL_WEAK_STATUS(ftPikachuSpecialAirLwStartSetStatus)
NDS_FIREBALL_WEAK_STATUS(ftPurinSpecialAirLwSetStatus)
NDS_FIREBALL_WEAK_STATUS(ftNessSpecialAirLwStartSetStatus)

#undef NDS_FIREBALL_WEAK_STATUS

void ftKirbySpecialNSetStatusSelect(GObj *fighter_gobj);
void ftKirbySpecialAirNSetStatusSelect(GObj *fighter_gobj);

#include "../../decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommonspecialn.c"
#include "../../decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommonspecialair.c"

#endif
