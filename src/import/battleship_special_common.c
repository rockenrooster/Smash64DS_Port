#if NDS_IMPORT_BATTLESHIP_MARIO_FIREBALL || \
    NDS_IMPORT_BATTLESHIP_FOX_BLASTER || \
    NDS_IMPORT_BATTLESHIP_MARIO_SPECIAL_HI || \
    NDS_IMPORT_BATTLESHIP_FOX_SPECIAL_HI

#include <ft/fighter.h>

#define NDS_SPECIAL_COMMON_WEAK_STATUS(name)           \
    __attribute__((weak)) void name(GObj *fighter_gobj) \
    {                                                   \
        (void)fighter_gobj;                             \
    }

// ponytail: unowned special setters stay weak until their character TUs land.
NDS_SPECIAL_COMMON_WEAK_STATUS(ftMarioSpecialNSetStatus)
NDS_SPECIAL_COMMON_WEAK_STATUS(ftMarioSpecialAirNSetStatus)
NDS_SPECIAL_COMMON_WEAK_STATUS(ftFoxSpecialNSetStatus)
NDS_SPECIAL_COMMON_WEAK_STATUS(ftFoxSpecialAirNSetStatus)
NDS_SPECIAL_COMMON_WEAK_STATUS(ftKirbyCopyMarioSpecialNSetStatus)
NDS_SPECIAL_COMMON_WEAK_STATUS(ftKirbyCopyFoxSpecialNSetStatus)
NDS_SPECIAL_COMMON_WEAK_STATUS(ftKirbyCopyDonkeySpecialNStartSetStatus)
NDS_SPECIAL_COMMON_WEAK_STATUS(ftKirbyCopySamusSpecialNStartSetStatus)
NDS_SPECIAL_COMMON_WEAK_STATUS(ftKirbyCopyLinkSpecialNSetStatus)
NDS_SPECIAL_COMMON_WEAK_STATUS(ftKirbyCopyYoshiSpecialNSetStatus)
NDS_SPECIAL_COMMON_WEAK_STATUS(ftKirbyCopyCaptainSpecialNSetStatus)
NDS_SPECIAL_COMMON_WEAK_STATUS(ftKirbySpecialNStartSetStatus)
NDS_SPECIAL_COMMON_WEAK_STATUS(ftKirbyCopyPikachuSpecialNSetStatus)
NDS_SPECIAL_COMMON_WEAK_STATUS(ftKirbyCopyPurinSpecialNSetStatus)
NDS_SPECIAL_COMMON_WEAK_STATUS(ftKirbyCopyNessSpecialNSetStatus)
NDS_SPECIAL_COMMON_WEAK_STATUS(ftDonkeySpecialNStartSetStatus)
NDS_SPECIAL_COMMON_WEAK_STATUS(ftSamusSpecialNStartSetStatus)
NDS_SPECIAL_COMMON_WEAK_STATUS(ftLinkSpecialNSetStatus)
NDS_SPECIAL_COMMON_WEAK_STATUS(ftYoshiSpecialNSetStatus)
NDS_SPECIAL_COMMON_WEAK_STATUS(ftCaptainSpecialNSetStatus)
NDS_SPECIAL_COMMON_WEAK_STATUS(ftPikachuSpecialNSetStatus)
NDS_SPECIAL_COMMON_WEAK_STATUS(ftPurinSpecialNSetStatus)
NDS_SPECIAL_COMMON_WEAK_STATUS(ftNessSpecialNSetStatus)
NDS_SPECIAL_COMMON_WEAK_STATUS(ftKirbyCopyMarioSpecialAirNSetStatus)
NDS_SPECIAL_COMMON_WEAK_STATUS(ftKirbyCopyFoxSpecialAirNSetStatus)
NDS_SPECIAL_COMMON_WEAK_STATUS(ftKirbyCopyDonkeySpecialAirNStartSetStatus)
NDS_SPECIAL_COMMON_WEAK_STATUS(ftKirbyCopySamusSpecialAirNStartSetStatus)
NDS_SPECIAL_COMMON_WEAK_STATUS(ftKirbyCopyLinkSpecialAirNSetStatus)
NDS_SPECIAL_COMMON_WEAK_STATUS(ftKirbyCopyYoshiSpecialAirNSetStatus)
NDS_SPECIAL_COMMON_WEAK_STATUS(ftKirbyCopyCaptainSpecialAirNSetStatus)
NDS_SPECIAL_COMMON_WEAK_STATUS(ftKirbySpecialAirNStartSetStatus)
NDS_SPECIAL_COMMON_WEAK_STATUS(ftKirbyCopyPikachuSpecialAirNSetStatus)
NDS_SPECIAL_COMMON_WEAK_STATUS(ftKirbyCopyPurinSpecialAirNSetStatus)
NDS_SPECIAL_COMMON_WEAK_STATUS(ftKirbyCopyNessSpecialAirNSetStatus)
NDS_SPECIAL_COMMON_WEAK_STATUS(ftDonkeySpecialAirNStartSetStatus)
NDS_SPECIAL_COMMON_WEAK_STATUS(ftSamusSpecialAirNStartSetStatus)
NDS_SPECIAL_COMMON_WEAK_STATUS(ftLinkSpecialAirNSetStatus)
NDS_SPECIAL_COMMON_WEAK_STATUS(ftYoshiSpecialAirNSetStatus)
NDS_SPECIAL_COMMON_WEAK_STATUS(ftCaptainSpecialAirNSetStatus)
NDS_SPECIAL_COMMON_WEAK_STATUS(ftPikachuSpecialAirNSetStatus)
NDS_SPECIAL_COMMON_WEAK_STATUS(ftPurinSpecialAirNSetStatus)
NDS_SPECIAL_COMMON_WEAK_STATUS(ftNessSpecialAirNSetStatus)
NDS_SPECIAL_COMMON_WEAK_STATUS(ftMarioSpecialHiSetStatus)
NDS_SPECIAL_COMMON_WEAK_STATUS(ftFoxSpecialHiStartSetStatus)
NDS_SPECIAL_COMMON_WEAK_STATUS(ftDonkeySpecialHiSetStatus)
NDS_SPECIAL_COMMON_WEAK_STATUS(ftSamusSpecialHiSetStatus)
NDS_SPECIAL_COMMON_WEAK_STATUS(ftLinkSpecialHiSetStatus)
NDS_SPECIAL_COMMON_WEAK_STATUS(ftYoshiSpecialHiSetStatus)
NDS_SPECIAL_COMMON_WEAK_STATUS(ftCaptainSpecialHiSetStatus)
NDS_SPECIAL_COMMON_WEAK_STATUS(ftKirbySpecialHiSetStatus)
NDS_SPECIAL_COMMON_WEAK_STATUS(ftPikachuSpecialHiStartSetStatus)
NDS_SPECIAL_COMMON_WEAK_STATUS(ftPurinSpecialHiSetStatus)
NDS_SPECIAL_COMMON_WEAK_STATUS(ftNessSpecialHiStartSetStatus)
NDS_SPECIAL_COMMON_WEAK_STATUS(ftMarioSpecialAirHiSetStatus)
NDS_SPECIAL_COMMON_WEAK_STATUS(ftFoxSpecialAirHiStartSetStatus)
NDS_SPECIAL_COMMON_WEAK_STATUS(ftDonkeySpecialAirHiSetStatus)
NDS_SPECIAL_COMMON_WEAK_STATUS(ftSamusSpecialAirHiSetStatus)
NDS_SPECIAL_COMMON_WEAK_STATUS(ftLinkSpecialAirHiSetStatus)
NDS_SPECIAL_COMMON_WEAK_STATUS(ftYoshiSpecialAirHiSetStatus)
NDS_SPECIAL_COMMON_WEAK_STATUS(ftCaptainSpecialAirHiSetStatus)
NDS_SPECIAL_COMMON_WEAK_STATUS(ftKirbySpecialAirHiSetStatus)
NDS_SPECIAL_COMMON_WEAK_STATUS(ftPikachuSpecialAirHiStartSetStatus)
NDS_SPECIAL_COMMON_WEAK_STATUS(ftPurinSpecialAirHiSetStatus)
NDS_SPECIAL_COMMON_WEAK_STATUS(ftNessSpecialAirHiStartSetStatus)
NDS_SPECIAL_COMMON_WEAK_STATUS(ftMarioSpecialAirLwSetStatus)
NDS_SPECIAL_COMMON_WEAK_STATUS(ftFoxSpecialAirLwStartSetStatus)
NDS_SPECIAL_COMMON_WEAK_STATUS(ftSamusSpecialAirLwSetStatus)
NDS_SPECIAL_COMMON_WEAK_STATUS(ftLinkSpecialAirLwSetStatus)
NDS_SPECIAL_COMMON_WEAK_STATUS(ftYoshiSpecialAirLwStartSetStatus)
NDS_SPECIAL_COMMON_WEAK_STATUS(ftCaptainSpecialAirLwSetStatus)
NDS_SPECIAL_COMMON_WEAK_STATUS(ftKirbySpecialAirLwStartSetStatus)
NDS_SPECIAL_COMMON_WEAK_STATUS(ftPikachuSpecialAirLwStartSetStatus)
NDS_SPECIAL_COMMON_WEAK_STATUS(ftPurinSpecialAirLwSetStatus)
NDS_SPECIAL_COMMON_WEAK_STATUS(ftNessSpecialAirLwStartSetStatus)

#undef NDS_SPECIAL_COMMON_WEAK_STATUS

void ftKirbySpecialNSetStatusSelect(GObj *fighter_gobj);
void ftKirbySpecialAirNSetStatusSelect(GObj *fighter_gobj);

#if NDS_P4
/* Remix's special-move starter tables (S3): the source checks run with the
 * content's rows lent to its kind's entries (ndsP4CheckSpecialLent). */
#include <nds/nds_p4.h>
#define ftCommonSpecialNCheckInterruptCommon \
    ndsP4SourceFTCommonSpecialNCheckInterruptCommon
#define ftCommonSpecialAirCheckInterruptCommon \
    ndsP4SourceFTCommonSpecialAirCheckInterruptCommon
sb32 ndsP4SourceFTCommonSpecialNCheckInterruptCommon(GObj *fighter_gobj);
sb32 ndsP4SourceFTCommonSpecialAirCheckInterruptCommon(GObj *fighter_gobj);
#endif
#include "../../decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommonspecialn.c"
#include "../../decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommonspecialair.c"
#if NDS_P4
#undef ftCommonSpecialNCheckInterruptCommon
#undef ftCommonSpecialAirCheckInterruptCommon

sb32 ftCommonSpecialNCheckInterruptCommon(GObj *fighter_gobj)
{
    static NDSP4SpecialStart *const tables[] = { dFTCommonSpecialNStatusList };
    static const u8 slots[] = { NDS_P4_SPECIAL_GROUND_N };

    return ndsP4CheckSpecialLent(fighter_gobj,
                                 ndsP4SourceFTCommonSpecialNCheckInterruptCommon,
                                 tables, slots, ARRAY_COUNT(slots));
}

sb32 ftCommonSpecialAirCheckInterruptCommon(GObj *fighter_gobj)
{
    static NDSP4SpecialStart *const tables[] = {
        dFTCommonSpecialAirNStatusList, dFTCommonSpecialAirHiStatusList,
        dFTCommonSpecialAirLwStatusList
    };
    static const u8 slots[] = {
        NDS_P4_SPECIAL_AIR_N, NDS_P4_SPECIAL_AIR_HI, NDS_P4_SPECIAL_AIR_LW
    };

    return ndsP4CheckSpecialLent(fighter_gobj,
                                 ndsP4SourceFTCommonSpecialAirCheckInterruptCommon,
                                 tables, slots, ARRAY_COUNT(slots));
}
#endif

#if NDS_IMPORT_BATTLESHIP_MARIO_SPECIAL_HI || \
    NDS_IMPORT_BATTLESHIP_FOX_SPECIAL_HI
#if NDS_P4
#define ftCommonSpecialHiCheckInterruptCommon \
    ndsP4SourceFTCommonSpecialHiCheckInterruptCommon
sb32 ndsP4SourceFTCommonSpecialHiCheckInterruptCommon(GObj *fighter_gobj);
#else
#define ftCommonSpecialHiCheckInterruptCommon \
    ndsBaseFTCommonSpecialHiCheckInterruptCommon
#endif
sb32 ndsBaseFTCommonSpecialHiCheckInterruptCommon(GObj *fighter_gobj);
#include "../../decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommonspecialhi.c"
#undef ftCommonSpecialHiCheckInterruptCommon
#if NDS_P4
sb32 ndsBaseFTCommonSpecialHiCheckInterruptCommon(GObj *fighter_gobj)
{
    static NDSP4SpecialStart *const tables[] = { dFTCommonSpecialHiStatusList };
    static const u8 slots[] = { NDS_P4_SPECIAL_GROUND_HI };

    return ndsP4CheckSpecialLent(fighter_gobj,
                                 ndsP4SourceFTCommonSpecialHiCheckInterruptCommon,
                                 tables, slots, ARRAY_COUNT(slots));
}
#endif
#endif

#endif
