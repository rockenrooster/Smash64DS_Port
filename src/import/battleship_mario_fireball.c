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
void ftMarioSpecialAirNSwitchStatusGround(GObj *fighter_gobj);
void ftMarioSpecialNSwitchStatusAir(GObj *fighter_gobj);

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
