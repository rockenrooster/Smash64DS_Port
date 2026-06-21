#ifndef SSB64_NDS_RELOC_DATA_H
#define SSB64_NDS_RELOC_DATA_H

#include <stddef.h>

#include <PR/ultratypes.h>

typedef struct LBFileNode {
    u32 id;
    void *addr;
} LBFileNode;

typedef struct LBRelocSetup {
    uintptr_t table_addr;
    u32 table_files_num;
    void *file_heap;
    size_t file_heap_size;
    LBFileNode *status_buffer;
    s32 status_buffer_size;
    LBFileNode *force_status_buffer;
    s32 force_status_buffer_size;
} LBRelocSetup;

extern uintptr_t lLBRelocTableAddr;
extern u32 llRelocFileCount;
extern uintptr_t llN64LogoFileID;
extern uintptr_t llN64LogoSprite;
extern uintptr_t llIFCommonAnnounceCommonFileID;
extern uintptr_t llMVCommonFileID;
extern uintptr_t llMVOpeningCommonFileID;
extern uintptr_t llMVOpeningRoomTransitionFileID;
extern uintptr_t llMVOpeningRoomScene1FileID;
extern uintptr_t llMVOpeningRoomScene2FileID;
extern uintptr_t llMVOpeningRoomScene3FileID;
extern uintptr_t llMVOpeningRoomScene4FileID;
extern uintptr_t llMVOpeningRunFileID;
extern uintptr_t llMVOpeningYamabukiFileID;
extern uintptr_t llMVOpeningSectorFileID;
extern uintptr_t llMVOpeningRunCrashFileID;
extern uintptr_t llMVOpeningRoomWallpaperFileID;
extern uintptr_t llMVOpeningPortraitsSet1FileID;
extern uintptr_t llMVOpeningPortraitsSet2FileID;
extern uintptr_t llMVCommonRoomBackgroundDObjDesc;
extern uintptr_t llMVCommonRoomDeskDObjDesc;
extern uintptr_t llMVCommonRoomOutsideDisplayList;
extern uintptr_t llMVCommonRoomHazeDisplayList;
extern uintptr_t llMVCommonRoomSunlightDisplayList;
extern uintptr_t llMVCommonRoomPencilsDObjDesc;
extern uintptr_t llMVCommonRoomPencilsAnimJoint;
extern uintptr_t llMVCommonRoomLogoDObjDesc;
extern uintptr_t llMVCommonRoomLogoMObjSub;
extern uintptr_t llMVCommonRoomLogoMatAnimJoint;
extern uintptr_t llMVCommonRoomBossShadowDisplayList;
extern uintptr_t llMVCommonRoomBossShadowAnimJoint;
extern uintptr_t llMVCommonRoomSpotlightDisplayList;
extern uintptr_t llMVCommonRoomSpotlightMObjSub;
extern uintptr_t llMVCommonRoomSpotlightMatAnimJoint;
extern uintptr_t llMVOpeningRoomTransitionOverlayDisplayList;
extern uintptr_t llMVOpeningRoomScene1CamAnimJoint;
extern uintptr_t llMVOpeningRoomScene2CamAnimJoint;
extern uintptr_t llMVOpeningRoomWallpaperSprite;
extern uintptr_t llMVOpeningPortraitsSet1SamusSprite;
extern uintptr_t llMVOpeningPortraitsSet1MarioSprite;
extern uintptr_t llMVOpeningPortraitsSet1FoxSprite;
extern uintptr_t llMVOpeningPortraitsSet1PikachuSprite;
extern uintptr_t llMVOpeningPortraitsSet1CoverSprite;
extern uintptr_t llMVOpeningPortraitsSet2LinkSprite;
extern uintptr_t llMVOpeningPortraitsSet2KirbySprite;
extern uintptr_t llMVOpeningPortraitsSet2DonkeySprite;
extern uintptr_t llMVOpeningPortraitsSet2YoshiSprite;
extern uintptr_t llMVOpeningRunWallpaperSprite;
extern uintptr_t llMVOpeningYamabukiWallpaperSprite;
extern uintptr_t llMVOpeningSectorCockpitSprite;
extern uintptr_t llMNTitleFileID;
extern uintptr_t llMNTitleFireAnimFileID;
extern uintptr_t llMNTitleLogoAnimCutoutSprite;
extern uintptr_t llMNTitleLogoAnimStrikeVSprite;
extern uintptr_t llMNTitleLogoAnimStrikeHSprite;
extern uintptr_t llMNTitleLogoAnimFullSprite;
extern uintptr_t llMNTitleBorderUpperSprite;
extern uintptr_t llMNTitleTMSprite;
extern uintptr_t llMNTitleCutoutSprite;
extern uintptr_t llMNTitleTMUnkSprite;
extern uintptr_t llMNTitleCopyrightSprite;
extern uintptr_t llMNTitlePressStartSprite;
extern uintptr_t llMNTitleSuperSprite;
extern uintptr_t llMNTitleSmashSprite;
extern uintptr_t llMNTitleBrosSprite;
extern uintptr_t llMNTitleFireAnimFrame1Sprite;
extern uintptr_t llMNTitleFireAnimFrame2Sprite;
extern uintptr_t llMNTitleFireAnimFrame3Sprite;
extern uintptr_t llMNTitleFireAnimFrame4Sprite;
extern uintptr_t llMNTitleFireAnimFrame5Sprite;
extern uintptr_t llMNTitleFireAnimFrame6Sprite;
extern uintptr_t llMNTitleFireAnimFrame7Sprite;
extern uintptr_t llMNTitleFireAnimFrame8Sprite;
extern uintptr_t llMNTitleFireAnimFrame9Sprite;
extern uintptr_t llMNTitleFireAnimFrame10Sprite;
extern uintptr_t llMNTitleFireAnimFrame11Sprite;
extern uintptr_t llMNTitleFireAnimFrame12Sprite;
extern uintptr_t llMNTitleFireAnimFrame13Sprite;
extern uintptr_t llMNTitleFireAnimFrame14Sprite;
extern uintptr_t llMNTitleFireAnimFrame15Sprite;
extern uintptr_t llMNTitleFireAnimFrame16Sprite;
extern uintptr_t llMNTitleFireAnimFrame17Sprite;
extern uintptr_t llMNTitleFireAnimFrame18Sprite;
extern uintptr_t llMNTitleFireAnimFrame19Sprite;
extern uintptr_t llMNTitleFireAnimFrame20Sprite;
extern uintptr_t llMNTitleFireAnimFrame21Sprite;
extern uintptr_t llMNTitleFireAnimFrame22Sprite;
extern uintptr_t llMNTitleFireAnimFrame23Sprite;
extern uintptr_t llMNTitleFireAnimFrame24Sprite;
extern uintptr_t llMNTitleFireAnimFrame25Sprite;
extern uintptr_t llMNTitleFireAnimFrame26Sprite;
extern uintptr_t llMNTitleFireAnimFrame27Sprite;
extern uintptr_t llMNTitleFireAnimFrame28Sprite;
extern uintptr_t llMNTitleFireAnimFrame29Sprite;
extern uintptr_t llMNTitleFireAnimFrame30Sprite;
extern uintptr_t llMNCommonFileID;
extern uintptr_t llMNVSModeFileID;
extern uintptr_t llMNCommonOptionTabLeftSprite;
extern uintptr_t llMNCommonOptionTabMiddleSprite;
extern uintptr_t llMNCommonOptionTabRightSprite;
extern uintptr_t llMNCommonFrameSprite;
extern uintptr_t llMNCommonGameModeTextSprite;
extern uintptr_t llMNCommonDigit0Sprite;
extern uintptr_t llMNCommonDigit1Sprite;
extern uintptr_t llMNCommonDigit2Sprite;
extern uintptr_t llMNCommonDigit3Sprite;
extern uintptr_t llMNCommonDigit4Sprite;
extern uintptr_t llMNCommonDigit5Sprite;
extern uintptr_t llMNCommonDigit6Sprite;
extern uintptr_t llMNCommonDigit7Sprite;
extern uintptr_t llMNCommonDigit8Sprite;
extern uintptr_t llMNCommonDigit9Sprite;
extern uintptr_t llMNCommonInfinitySprite;
extern uintptr_t llMNCommonArrowRSprite;
extern uintptr_t llMNCommonArrowLSprite;
extern uintptr_t llMNCommonDecalPaperSprite;
extern uintptr_t llMNCommonSmashLogoSprite;
extern uintptr_t llMNCommonSmashBrosCollageSprite;
extern uintptr_t llMNVSModeVSStartTextSprite;
extern uintptr_t llMNVSModeRulePeriodTextSprite;
extern uintptr_t llMNVSModeTimeTextSprite;
extern uintptr_t llMNVSModeStockTextSprite;
extern uintptr_t llMNVSModeTeamTextSprite;
extern uintptr_t llMNVSModeTimePeriodTextSprite;
extern uintptr_t llMNVSModeMinTextSprite;
extern uintptr_t llMNVSModeStockPeriodTextSprite;
extern uintptr_t llMNVSModeVSOptionsTextSprite;
extern uintptr_t llMNVSModeConsoleIconDarkSprite;
extern uintptr_t llMNVSModeVSTextSprite;
extern uintptr_t llMNTitleLogoDObjDesc;
extern uintptr_t llMNTitleLogoAnimJoint;
extern uintptr_t llMNTitleLabelsDObjDesc;
extern uintptr_t llMNTitleLabelsAnimJoint;
extern uintptr_t llMNTitlePressStartDObjDesc;
extern uintptr_t llMNTitlePressStartAnimJoint;
extern uintptr_t llMNTitleSlashDObjDesc;
extern uintptr_t llMNTitleSlashMObjSub;
extern uintptr_t llMNTitleSlashAnimJoint;
extern uintptr_t llMNTitleSlashMatAnimJoint;
extern uintptr_t llMNTitleFireDObjDesc;
extern uintptr_t llMNTitleFireAnimJoint;
extern uintptr_t lMNTitleParticleScriptBankLo;
extern uintptr_t lMNTitleParticleScriptBankHi;
extern uintptr_t lMNTitleParticleTextureBankLo;
extern uintptr_t lMNTitleParticleTextureBankHi;
extern uintptr_t llIFCommonAnnounceCommonLetterASprite;
extern uintptr_t llIFCommonAnnounceCommonLetterBSprite;
extern uintptr_t llIFCommonAnnounceCommonLetterCSprite;
extern uintptr_t llIFCommonAnnounceCommonLetterDSprite;
extern uintptr_t llIFCommonAnnounceCommonLetterESprite;
extern uintptr_t llIFCommonAnnounceCommonLetterFSprite;
extern uintptr_t llIFCommonAnnounceCommonLetterGSprite;
extern uintptr_t llIFCommonAnnounceCommonLetterHSprite;
extern uintptr_t llIFCommonAnnounceCommonLetterISprite;
extern uintptr_t llIFCommonAnnounceCommonLetterKSprite;
extern uintptr_t llIFCommonAnnounceCommonLetterLSprite;
extern uintptr_t llIFCommonAnnounceCommonLetterMSprite;
extern uintptr_t llIFCommonAnnounceCommonLetterNSprite;
extern uintptr_t llIFCommonAnnounceCommonLetterOSprite;
extern uintptr_t llIFCommonAnnounceCommonLetterPSprite;
extern uintptr_t llIFCommonAnnounceCommonLetterRSprite;
extern uintptr_t llIFCommonAnnounceCommonLetterSSprite;
extern uintptr_t llIFCommonAnnounceCommonLetterUSprite;
extern uintptr_t llIFCommonAnnounceCommonLetterXSprite;
extern uintptr_t llIFCommonAnnounceCommonLetterYSprite;
extern uintptr_t llMVOpeningCommonMarioCamAnimJoint;
extern uintptr_t llMVOpeningCommonDonkeyCamAnimJoint;
extern uintptr_t llMVOpeningCommonSamusCamAnimJoint;
extern uintptr_t llMVOpeningCommonFoxCamAnimJoint;
extern uintptr_t llMVOpeningCommonLinkCamAnimJoint;
extern uintptr_t llMVOpeningCommonYoshiCamAnimJoint;
extern uintptr_t llMVOpeningCommonPikachuCamAnimJoint;
extern uintptr_t llMVOpeningCommonKirbyCamAnimJoint;

#define NDS_MENU_RELOC_SYMBOLS(X) \
    X(llFTEmblemSpritesDonkeySprite, 0xc78) \
    X(llFTEmblemSpritesFZeroSprite, 0x32b8) \
    X(llFTEmblemSpritesFileID, 0x14) \
    X(llFTEmblemSpritesFoxSprite, 0x1938) \
    X(llFTEmblemSpritesKirbySprite, 0x1f98) \
    X(llFTEmblemSpritesMarioSprite, 0x618) \
    X(llFTEmblemSpritesMetroidSprite, 0x12d8) \
    X(llFTEmblemSpritesMotherSprite, 0x3f78) \
    X(llFTEmblemSpritesPMonstersSprite, 0x3918) \
    X(llFTEmblemSpritesYoshiSprite, 0x2c58) \
    X(llFTEmblemSpritesZeldaSprite, 0x25f8) \
    X(llGRCastleMapFileID, 0x103) \
    X(llGRCastleMapMapHeader, 0x14) \
    X(llGRHyruleMapFileID, 0x109) \
    X(llGRHyruleMapMapHeader, 0x14) \
    X(llGRInishieMapFileID, 0x104) \
    X(llGRInishieMapMapHeader, 0x14) \
    X(llGRJungleMapFileID, 0x105) \
    X(llGRJungleMapMapHeader, 0x14) \
    X(llGRPupupuMapFileID, 0xff) \
    X(llGRPupupuMapMapHeader, 0x14) \
    X(llGRSectorMapFileID, 0x106) \
    X(llGRSectorMapMapHeader, 0x14) \
    X(llGRWallpaperTrainingBlackFileID, 0x1a) \
    X(llGRWallpaperTrainingBlueFileID, 0x1c) \
    X(llGRWallpaperTrainingBlueSprite, 0x20718) \
    X(llGRWallpaperTrainingYellowFileID, 0x1b) \
    X(llGRYamabukiMapFileID, 0x108) \
    X(llGRYamabukiMapMapHeader, 0x14) \
    X(llGRYosterMapFileID, 0x107) \
    X(llGRYosterMapMapHeader, 0x14) \
    X(llGRZebesMapFileID, 0x101) \
    X(llGRZebesMapMapHeader, 0x14) \
    X(llMNCommonColonSprite, 0xdcf0) \
    X(llMNCommonFontsFileID, 0x21) \
    X(llMNCommonFontsLetterASprite, 0x40) \
    X(llMNCommonFontsLetterBSprite, 0xd0) \
    X(llMNCommonFontsLetterCSprite, 0x160) \
    X(llMNCommonFontsLetterDSprite, 0x1f0) \
    X(llMNCommonFontsLetterESprite, 0x280) \
    X(llMNCommonFontsLetterFSprite, 0x310) \
    X(llMNCommonFontsLetterGSprite, 0x3a0) \
    X(llMNCommonFontsLetterHSprite, 0x430) \
    X(llMNCommonFontsLetterISprite, 0x4c0) \
    X(llMNCommonFontsLetterJSprite, 0x550) \
    X(llMNCommonFontsLetterKSprite, 0x5e0) \
    X(llMNCommonFontsLetterLSprite, 0x670) \
    X(llMNCommonFontsLetterMSprite, 0x700) \
    X(llMNCommonFontsLetterNSprite, 0x790) \
    X(llMNCommonFontsLetterOSprite, 0x820) \
    X(llMNCommonFontsLetterPSprite, 0x8b0) \
    X(llMNCommonFontsLetterQSprite, 0x940) \
    X(llMNCommonFontsLetterRSprite, 0x9d0) \
    X(llMNCommonFontsLetterSSprite, 0xa60) \
    X(llMNCommonFontsLetterTSprite, 0xaf0) \
    X(llMNCommonFontsLetterUSprite, 0xb80) \
    X(llMNCommonFontsLetterVSprite, 0xc10) \
    X(llMNCommonFontsLetterWSprite, 0xca0) \
    X(llMNCommonFontsLetterXSprite, 0xd30) \
    X(llMNCommonFontsLetterYSprite, 0xdc0) \
    X(llMNCommonFontsLetterZSprite, 0xe50) \
    X(llMNCommonFontsSymbolApostropheSprite, 0xed0) \
    X(llMNCommonFontsSymbolPercentSprite, 0xf60) \
    X(llMNCommonFontsSymbolPeriodSprite, 0xfd0) \
    X(llMNMapsCongoJungleSprite, 0x6948) \
    X(llMNMapsCongoJungleTextSprite, 0x678) \
    X(llMNMapsCursorSprite, 0x1ab8) \
    X(llMNMapsDreamLandSprite, 0xbc88) \
    X(llMNMapsDreamLandTextSprite, 0x1418) \
    X(llMNMapsFileID, 0x1e) \
    X(llMNMapsHyruleCastleSprite, 0x8508) \
    X(llMNMapsHyruleCastleTextSprite, 0xb10) \
    X(llMNMapsMushroomKingdomSprite, 0xaea8) \
    X(llMNMapsMushroomKingdomTextSprite, 0x11d8) \
    X(llMNMapsPeachsCastleSprite, 0x4d88) \
    X(llMNMapsPeachsCastleTextSprite, 0x1f8) \
    X(llMNMapsPlanetZebesSprite, 0x7728) \
    X(llMNMapsPlanetZebesTextSprite, 0x8b8) \
    X(llMNMapsPlateLeftSprite, 0x3fa8) \
    X(llMNMapsPlateMiddleSprite, 0x3d68) \
    X(llMNMapsPlateRightSprite, 0x3c68) \
    X(llMNMapsQuestionMarkSprite, 0x1dd8) \
    X(llMNMapsRandomBigSprite, 0xde30) \
    X(llMNMapsRandomSmallSprite, 0xcb10) \
    X(llMNMapsSaffronCitySprite, 0xa0c8) \
    X(llMNMapsSaffronCityTextSprite, 0xf98) \
    X(llMNMapsSectorZSprite, 0x5b68) \
    X(llMNMapsSectorZTextSprite, 0x438) \
    X(llMNMapsStageSelectTextSprite, 0x26a0) \
    X(llMNMapsTilesSprite, 0xc728) \
    X(llMNMapsWoodenCircleSprite, 0x3840) \
    X(llMNMapsYoshisIslandSprite, 0x92e8) \
    X(llMNMapsYoshisIslandTextSprite, 0xd58) \
    X(llMNPlayersCommon0DarkSprite, 0x5388) \
    X(llMNPlayersCommon1DarkSprite, 0x5440) \
    X(llMNPlayersCommon1PPuckSprite, 0x9048) \
    X(llMNPlayersCommon1PTextGradientSprite, 0x8268) \
    X(llMNPlayersCommon1PTextSprite, 0x878) \
    X(llMNPlayersCommon2DarkSprite, 0x5558) \
    X(llMNPlayersCommon2PPuckSprite, 0x9b28) \
    X(llMNPlayersCommon2PTextGradientSprite, 0x8368) \
    X(llMNPlayersCommon2PTextSprite, 0xa58) \
    X(llMNPlayersCommon3DarkSprite, 0x5668) \
    X(llMNPlayersCommon3PPuckSprite, 0xa608) \
    X(llMNPlayersCommon3PTextGradientSprite, 0x8468) \
    X(llMNPlayersCommon3PTextSprite, 0xc38) \
    X(llMNPlayersCommon4DarkSprite, 0x5778) \
    X(llMNPlayersCommon4PPuckSprite, 0xb0e8) \
    X(llMNPlayersCommon4PTextGradientSprite, 0x8568) \
    X(llMNPlayersCommon4PTextSprite, 0xe18) \
    X(llMNPlayersCommon5DarkSprite, 0x5888) \
    X(llMNPlayersCommon6DarkSprite, 0x5998) \
    X(llMNPlayersCommon7DarkSprite, 0x5aa8) \
    X(llMNPlayersCommon8DarkSprite, 0x5bb8) \
    X(llMNPlayersCommon9DarkSprite, 0x5cc8) \
    X(llMNPlayersCommonArrowLSprite, 0xece8) \
    X(llMNPlayersCommonArrowRSprite, 0xedc8) \
    X(llMNPlayersCommonBackButtonSprite, 0x115c8) \
    X(llMNPlayersCommonBlueLabelSprite, 0xec08) \
    X(llMNPlayersCommonButtonTextSprite, 0x1428) \
    X(llMNPlayersCommonCPLabelSprite, 0x63c8) \
    X(llMNPlayersCommonCPLevelTextSprite, 0x1218) \
    X(llMNPlayersCommonCPPuckSprite, 0xbbc8) \
    X(llMNPlayersCommonCPTextSprite, 0xff8) \
    X(llMNPlayersCommonCaptainFalconTextSprite, 0x3998) \
    X(llMNPlayersCommonCursorHandGrabSprite, 0x76e8) \
    X(llMNPlayersCommonCursorHandHoverSprite, 0x8168) \
    X(llMNPlayersCommonCursorHandPointSprite, 0x6f88) \
    X(llMNPlayersCommonDKTextSprite, 0x1ff8) \
    X(llMNPlayersCommonFileID, 0x11) \
    X(llMNPlayersCommonFoxTextSprite, 0x25b8) \
    X(llMNPlayersCommonGateCom1PLUT, 0x11378) \
    X(llMNPlayersCommonGateCom2PLUT, 0x113a0) \
    X(llMNPlayersCommonGateCom3PLUT, 0x113f0) \
    X(llMNPlayersCommonGateCom4PLUT, 0x113c8) \
    X(llMNPlayersCommonGateMan1PLUT, 0x103f8) \
    X(llMNPlayersCommonGateMan2PLUT, 0x10420) \
    X(llMNPlayersCommonGateMan3PLUT, 0x10470) \
    X(llMNPlayersCommonGateMan4PLUT, 0x10448) \
    X(llMNPlayersCommonGreenLabelSprite, 0xe7e8) \
    X(llMNPlayersCommonHandicapTextSprite, 0x1108) \
    X(llMNPlayersCommonHmnLabelSprite, 0x6048) \
    X(llMNPlayersCommonInfinityDarkSprite, 0x3ef0) \
    X(llMNPlayersCommonJigglypuffTextSprite, 0x3db8) \
    X(llMNPlayersCommonKirbyTextSprite, 0x28e8) \
    X(llMNPlayersCommonLinkTextSprite, 0x2ba0) \
    X(llMNPlayersCommonLuigiTextSprite, 0x1b18) \
    X(llMNPlayersCommonMarioTextSprite, 0x1838) \
    X(llMNPlayersCommonNALabelSprite, 0x6748) \
    X(llMNPlayersCommonNessTextSprite, 0x35b0) \
    X(llMNPlayersCommonPikachuTextSprite, 0x32f8) \
    X(llMNPlayersCommonPressTextSprite, 0x14d8) \
    X(llMNPlayersCommonPushTextSprite, 0x12c8) \
    X(llMNPlayersCommonReadyBannerSprite, 0xf530) \
    X(llMNPlayersCommonReadyToFightTextSprite, 0xf448) \
    X(llMNPlayersCommonRedCardSprite, 0x104b0) \
    X(llMNPlayersCommonRedLabelSprite, 0xe3c8) \
    X(llMNPlayersCommonSamusTextSprite, 0x2358) \
    X(llMNPlayersCommonSmashLogoCardLeftSprite, 0xcdb0) \
    X(llMNPlayersCommonSmashLogoCardRightSprite, 0xdfa0) \
    X(llMNPlayersCommonStartTextSprite, 0x1378) \
    X(llMNPlayersCommonStockSelectorSprite, 0x5270) \
    X(llMNPlayersCommonTimeSelectorSprite, 0x48b0) \
    X(llMNPlayersCommonYoshiTextSprite, 0x2ed8) \
    X(llMNPlayersGameModesFileID, 0x12) \
    X(llMNPlayersGameModesFreeForAllTextSprite, 0x280) \
    X(llMNPlayersGameModesTeamBattleTextSprite, 0x4e0) \
    X(llMNPlayersPortraitsCaptainShadowSprite, 0x1e2e8) \
    X(llMNPlayersPortraitsCaptainSprite, 0x19e48) \
    X(llMNPlayersPortraitsCrossSprite, 0x2b8) \
    X(llMNPlayersPortraitsDonkeySprite, 0x8bc8) \
    X(llMNPlayersPortraitsFileID, 0x13) \
    X(llMNPlayersPortraitsFoxSprite, 0xd068) \
    X(llMNPlayersPortraitsKirbySprite, 0xf2b8) \
    X(llMNPlayersPortraitsLinkSprite, 0x11508) \
    X(llMNPlayersPortraitsLuigiShadowSprite, 0x20538) \
    X(llMNPlayersPortraitsLuigiSprite, 0x6978) \
    X(llMNPlayersPortraitsMarioSprite, 0x4728) \
    X(llMNPlayersPortraitsNessShadowSprite, 0x22788) \
    X(llMNPlayersPortraitsNessSprite, 0x17bf8) \
    X(llMNPlayersPortraitsPikachuSprite, 0x159a8) \
    X(llMNPlayersPortraitsPortraitFireBgSprite, 0x24d0) \
    X(llMNPlayersPortraitsPortraitQuestionMarkSprite, 0xf68) \
    X(llMNPlayersPortraitsPurinShadowSprite, 0x249d8) \
    X(llMNPlayersPortraitsPurinSprite, 0x1c098) \
    X(llMNPlayersPortraitsSamusSprite, 0xae18) \
    X(llMNPlayersPortraitsWhiteSquareSprite, 0x6f0) \
    X(llMNPlayersPortraitsYoshiSprite, 0x13758) \
    X(llMNPlayersSpotlightDObjDesc, 0x568) \
    X(llMNPlayersSpotlightFileID, 0x16) \
    X(llMNPlayersSpotlightMObjSub, 0x408) \
    X(llMNSelectCommonFileID, 0x15) \
    X(llMNSelectCommonStoneBackgroundSprite, 0x440)

#define NDS_DECLARE_MENU_RELOC_SYMBOL(name, value) extern uintptr_t name;
NDS_MENU_RELOC_SYMBOLS(NDS_DECLARE_MENU_RELOC_SYMBOL)
#undef NDS_DECLARE_MENU_RELOC_SYMBOL

void lbRelocInitSetup(LBRelocSetup *setup);
size_t lbRelocGetFileSize(const void *file_id);
void *lbRelocGetExternHeapFile(const void *file_id, void *heap);
void *lbRelocGetForceExternHeapFile(const void *file_id, void *heap);
size_t lbRelocGetAllocSize(u32 *ids, u32 len);
size_t lbRelocLoadFilesExtern(u32 *ids, u32 len, void **files, void *heap);
void *ndsRelocGetFileData(void *file, const void *symbol);

#define lbRelocGetFileData(type, file, symbol) \
    ((type)ndsRelocGetFileData((file), (symbol)))

#ifndef ARRAY_COUNT
#define ARRAY_COUNT(array) (sizeof(array) / sizeof((array)[0]))
#endif

#define lbRelocLoadFilesListed(file_ids, out_ptrs) \
    lbRelocLoadFilesExtern( \
        (file_ids), \
        ARRAY_COUNT(file_ids), \
        (out_ptrs), \
        syTaskmanMalloc(lbRelocGetAllocSize((file_ids), ARRAY_COUNT(file_ids)), 0x10))

#endif
