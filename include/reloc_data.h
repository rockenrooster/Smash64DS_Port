#ifndef SSB64_NDS_RELOC_DATA_H
#define SSB64_NDS_RELOC_DATA_H

#include <stddef.h>

#include <PR/ultratypes.h>

#ifndef _LBTYPES_H_
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
#endif

extern uintptr_t lLBRelocTableAddr;
extern u32 llRelocFileCount;
extern uintptr_t llN64LogoFileID;
extern uintptr_t llN64LogoSprite;
extern uintptr_t llIFCommonPlayerFileID;
extern uintptr_t llIFCommonGameStatusFileID;
extern uintptr_t llIFCommonPlayerDamageFileID;
extern uintptr_t llIFCommonTimerFileID;
extern uintptr_t llIFCommonDigitsFileID;
extern uintptr_t llIFCommonBattlePauseFileID;
extern uintptr_t llIFCommonPlayerTagsFileID;
extern uintptr_t llIFCommonAnnounceCommonFileID;
extern uintptr_t llSYKseg1ValidateFileID;
extern uintptr_t llSYKseg1ValidateFunc;
extern uintptr_t llSYKseg1ValidateNBytes;
extern uintptr_t llFTManagerCommonFileID;
extern uintptr_t llFTCommonMovesetFileID;
extern uintptr_t llKirbyMainMotionSpecialNFTKirbyCopy;
extern uintptr_t llEFCommonEffects1FileID;
extern uintptr_t llEFCommonEffects2FileID;
extern uintptr_t llEFCommonEffects3FileID;
extern uintptr_t llMarioMainMotionFileID;
extern uintptr_t llMarioMainFileID;
extern uintptr_t llMarioSpecial1FileID;
extern uintptr_t llMarioModelFileID;
extern uintptr_t llMarioSpecial3FileID;
extern uintptr_t llMarioShieldPoseFileID;
extern uintptr_t llMarioSpecial2FileID;
extern uintptr_t llFoxSpecial3FileID;
extern uintptr_t llFoxMainMotionFileID;
extern uintptr_t llFoxMainFileID;
extern uintptr_t llFoxSpecial1FileID;
extern uintptr_t llFoxModelFileID;
extern uintptr_t llFoxShieldPoseFileID;
extern uintptr_t llFoxSpecial4FileID;
extern uintptr_t llFoxSpecial2FileID;
extern uintptr_t llMarioModelStockSprite;
extern uintptr_t llMarioModelFTEmblemSprite;
extern uintptr_t llFoxModelStockSprite;
extern uintptr_t llFoxModelFTEmblemSprite;
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
extern intptr_t lGRPupupuParticleScriptBankLo;
extern intptr_t lGRPupupuParticleScriptBankHi;
extern intptr_t lGRPupupuParticleTextureBankLo;
extern intptr_t lGRPupupuParticleTextureBankHi;
/* P2-4 Yoster vapor bank markers, decomp gr/grcommon/gryoster.h:9-12.
 * Defined by src/import/battleship_gryoster_ground.c behind
 * NDS_P2_STAGE_YOSTER; address identity only, as with the Pupupu bank. */
extern intptr_t lGRYosterParticleScriptBankLo;
extern intptr_t lGRYosterParticleScriptBankHi;
extern intptr_t lGRYosterParticleTextureBankLo;
extern intptr_t lGRYosterParticleTextureBankHi;
extern uintptr_t llIFCommonAnnounceCommonLetterASprite;
extern uintptr_t llIFCommonAnnounceCommonLetterBSprite;
extern uintptr_t llIFCommonAnnounceCommonLetterCSprite;
extern uintptr_t llIFCommonAnnounceCommonLetterDSprite;
extern uintptr_t llIFCommonAnnounceCommonLetterESprite;
extern uintptr_t llIFCommonAnnounceCommonLetterFSprite;
extern uintptr_t llIFCommonAnnounceCommonLetterGSprite;
extern uintptr_t llIFCommonAnnounceCommonLetterHSprite;
extern uintptr_t llIFCommonAnnounceCommonLetterISprite;
extern uintptr_t llIFCommonAnnounceCommonLetterJSprite;
extern uintptr_t llIFCommonAnnounceCommonLetterKSprite;
extern uintptr_t llIFCommonAnnounceCommonLetterLSprite;
extern uintptr_t llIFCommonAnnounceCommonLetterMSprite;
extern uintptr_t llIFCommonAnnounceCommonLetterNSprite;
extern uintptr_t llIFCommonAnnounceCommonLetterOSprite;
extern uintptr_t llIFCommonAnnounceCommonLetterPSprite;
extern uintptr_t llIFCommonAnnounceCommonLetterQSprite;
extern uintptr_t llIFCommonAnnounceCommonLetterRSprite;
extern uintptr_t llIFCommonAnnounceCommonLetterSSprite;
extern uintptr_t llIFCommonAnnounceCommonLetterUSprite;
extern uintptr_t llIFCommonAnnounceCommonLetterVSprite;
extern uintptr_t llIFCommonAnnounceCommonLetterWSprite;
extern uintptr_t llIFCommonAnnounceCommonLetterXSprite;
extern uintptr_t llIFCommonAnnounceCommonLetterYSprite;
extern uintptr_t llIFCommonAnnounceCommonLetterZSprite;
extern uintptr_t llIFCommonAnnounceCommonSymbolPeriodSprite;

#define NDS_IFCOMMON_RELOC_SYMBOLS(X) \
    X(NDS_RELOC_ASSET_IF_COMMON_ANNOUNCE, llIFCommonAnnounceCommonLetterJSprite, 0x2a90u) \
    X(NDS_RELOC_ASSET_IF_COMMON_ANNOUNCE, llIFCommonAnnounceCommonLetterQSprite, 0x4f10u) \
    X(NDS_RELOC_ASSET_IF_COMMON_ANNOUNCE, llIFCommonAnnounceCommonLetterTSprite, 0x5bd0u) \
    X(NDS_RELOC_ASSET_IF_COMMON_ANNOUNCE, llIFCommonAnnounceCommonLetterVSprite, 0x65d8u) \
    X(NDS_RELOC_ASSET_IF_COMMON_ANNOUNCE, llIFCommonAnnounceCommonLetterWSprite, 0x6c00u) \
    X(NDS_RELOC_ASSET_IF_COMMON_ANNOUNCE, llIFCommonAnnounceCommonLetterZSprite, 0x7ae8u) \
    X(NDS_RELOC_ASSET_IF_COMMON_ANNOUNCE, llIFCommonAnnounceCommonSymbolExclaimSprite, 0x7d98u) \
    X(NDS_RELOC_ASSET_IF_COMMON_ANNOUNCE, llIFCommonAnnounceCommonSymbolPeriodSprite, 0x7e50u) \
    X(NDS_RELOC_ASSET_IF_COMMON_BATTLE_PAUSE, llIFCommonBattlePauseDecalAButtonSprite, 0x958u) \
    X(NDS_RELOC_ASSET_IF_COMMON_BATTLE_PAUSE, llIFCommonBattlePauseDecalArrowsSprite, 0x1538u) \
    X(NDS_RELOC_ASSET_IF_COMMON_BATTLE_PAUSE, llIFCommonBattlePauseDecalBButtonSprite, 0xa88u) \
    X(NDS_RELOC_ASSET_IF_COMMON_BATTLE_PAUSE, llIFCommonBattlePauseDecalControlStickSprite, 0x17a8u) \
    X(NDS_RELOC_ASSET_IF_COMMON_BATTLE_PAUSE, llIFCommonBattlePauseDecalLTriggerSprite, 0x18c8u) \
    X(NDS_RELOC_ASSET_IF_COMMON_BATTLE_PAUSE, llIFCommonBattlePauseDecalPauseSprite, 0x438u) \
    X(NDS_RELOC_ASSET_IF_COMMON_BATTLE_PAUSE, llIFCommonBattlePauseDecalPlusSprite, 0x4d8u) \
    X(NDS_RELOC_ASSET_IF_COMMON_BATTLE_PAUSE, llIFCommonBattlePauseDecalResetSprite, 0x610u) \
    X(NDS_RELOC_ASSET_IF_COMMON_BATTLE_PAUSE, llIFCommonBattlePauseDecalRetrySprite, 0x828u) \
    X(NDS_RELOC_ASSET_IF_COMMON_BATTLE_PAUSE, llIFCommonBattlePauseDecalRTriggerSprite, 0xcf8u) \
    X(NDS_RELOC_ASSET_IF_COMMON_BATTLE_PAUSE, llIFCommonBattlePauseDecalSmashBallSprite, 0x6d8u) \
    X(NDS_RELOC_ASSET_IF_COMMON_BATTLE_PAUSE, llIFCommonBattlePauseDecalZTriggerSprite, 0xbd8u) \
    X(NDS_RELOC_ASSET_IF_COMMON_BATTLE_PAUSE, llIFCommonBattlePausePlayerNum1PSprite, 0x78u) \
    X(NDS_RELOC_ASSET_IF_COMMON_BATTLE_PAUSE, llIFCommonBattlePausePlayerNum2PSprite, 0x138u) \
    X(NDS_RELOC_ASSET_IF_COMMON_BATTLE_PAUSE, llIFCommonBattlePausePlayerNum3PSprite, 0x1f8u) \
    X(NDS_RELOC_ASSET_IF_COMMON_BATTLE_PAUSE, llIFCommonBattlePausePlayerNum4PSprite, 0x2b8u) \
    X(NDS_RELOC_ASSET_IF_COMMON_DIGITS, llIFCommonDigits0Sprite, 0x68u) \
    X(NDS_RELOC_ASSET_IF_COMMON_DIGITS, llIFCommonDigits1Sprite, 0x118u) \
    X(NDS_RELOC_ASSET_IF_COMMON_DIGITS, llIFCommonDigits2Sprite, 0x1c8u) \
    X(NDS_RELOC_ASSET_IF_COMMON_DIGITS, llIFCommonDigits3Sprite, 0x278u) \
    X(NDS_RELOC_ASSET_IF_COMMON_DIGITS, llIFCommonDigits4Sprite, 0x328u) \
    X(NDS_RELOC_ASSET_IF_COMMON_DIGITS, llIFCommonDigits5Sprite, 0x3d8u) \
    X(NDS_RELOC_ASSET_IF_COMMON_DIGITS, llIFCommonDigits6Sprite, 0x488u) \
    X(NDS_RELOC_ASSET_IF_COMMON_DIGITS, llIFCommonDigits7Sprite, 0x538u) \
    X(NDS_RELOC_ASSET_IF_COMMON_DIGITS, llIFCommonDigits8Sprite, 0x5e8u) \
    X(NDS_RELOC_ASSET_IF_COMMON_DIGITS, llIFCommonDigits9Sprite, 0x698u) \
    X(NDS_RELOC_ASSET_IF_COMMON_DIGITS, llIFCommonDigitsDashSprite, 0x710u) \
    X(NDS_RELOC_ASSET_IF_COMMON_DIGITS, llIFCommonDigitsCrossSprite, 0x828u) \
    X(NDS_RELOC_ASSET_IF_COMMON_GAME_STATUS, llIFCommonGameStatusBlueLetterASprite, 0x1de68u) \
    X(NDS_RELOC_ASSET_IF_COMMON_GAME_STATUS, llIFCommonGameStatusBlueLetterESprite, 0x144e0u) \
    X(NDS_RELOC_ASSET_IF_COMMON_GAME_STATUS, llIFCommonGameStatusBlueLetterGSprite, 0x20788u) \
    X(NDS_RELOC_ASSET_IF_COMMON_GAME_STATUS, llIFCommonGameStatusBlueLetterISprite, 0xf740u) \
    X(NDS_RELOC_ASSET_IF_COMMON_GAME_STATUS, llIFCommonGameStatusBlueLetterMSprite, 0x127e0u) \
    X(NDS_RELOC_ASSET_IF_COMMON_GAME_STATUS, llIFCommonGameStatusBlueLetterPSprite, 0x18fe8u) \
    X(NDS_RELOC_ASSET_IF_COMMON_GAME_STATUS, llIFCommonGameStatusBlueLetterSSprite, 0x1b5f8u) \
    X(NDS_RELOC_ASSET_IF_COMMON_GAME_STATUS, llIFCommonGameStatusBlueLetterTSprite, 0xe4a8u) \
    X(NDS_RELOC_ASSET_IF_COMMON_GAME_STATUS, llIFCommonGameStatusBlueLetterUSprite, 0x16eb8u) \
    X(NDS_RELOC_ASSET_IF_COMMON_GAME_STATUS, llIFCommonGameStatusFrameSprite, 0x21760u) \
    X(NDS_RELOC_ASSET_IF_COMMON_GAME_STATUS, llIFCommonGameStatusLampBlueContourSprite, 0x25290u) \
    X(NDS_RELOC_ASSET_IF_COMMON_GAME_STATUS, llIFCommonGameStatusLampBlueDimSprite, 0x21ba8u) \
    X(NDS_RELOC_ASSET_IF_COMMON_GAME_STATUS, llIFCommonGameStatusLampBlueLightSprite, 0x22f18u) \
    X(NDS_RELOC_ASSET_IF_COMMON_GAME_STATUS, llIFCommonGameStatusLampRedContourSprite, 0x23a28u) \
    X(NDS_RELOC_ASSET_IF_COMMON_GAME_STATUS, llIFCommonGameStatusLampRedDimSprite, 0x21950u) \
    X(NDS_RELOC_ASSET_IF_COMMON_GAME_STATUS, llIFCommonGameStatusLampRedLightSprite, 0x22128u) \
    X(NDS_RELOC_ASSET_IF_COMMON_GAME_STATUS, llIFCommonGameStatusLampYellowContourSprite, 0x24620u) \
    X(NDS_RELOC_ASSET_IF_COMMON_GAME_STATUS, llIFCommonGameStatusLampYellowDimSprite, 0x21a10u) \
    X(NDS_RELOC_ASSET_IF_COMMON_GAME_STATUS, llIFCommonGameStatusLampYellowLightSprite, 0x22588u) \
    X(NDS_RELOC_ASSET_IF_COMMON_GAME_STATUS, llIFCommonGameStatusOrangeExclamationMarkSprite, 0xc370u) \
    X(NDS_RELOC_ASSET_IF_COMMON_GAME_STATUS, llIFCommonGameStatusOrangeLetterGSprite, 0x4d78u) \
    X(NDS_RELOC_ASSET_IF_COMMON_GAME_STATUS, llIFCommonGameStatusOrangeLetterOSprite, 0xa730u) \
    X(NDS_RELOC_ASSET_IF_COMMON_GAME_STATUS, llIFCommonGameStatusRodShadowSprite, 0x21878u) \
    X(NDS_RELOC_ASSET_IF_COMMON_GAME_STATUS, llIFCommonGameStatusRodSprite, 0x20990u) \
    X(NDS_RELOC_ASSET_IF_COMMON_ITEM, llIFCommonItemArrowSprite, 0x50u) \
    X(NDS_RELOC_ASSET_IF_COMMON_ITEM, llIFCommonItemFileID, 0x57u) \
    X(NDS_RELOC_ASSET_INVALID, llITCommonDataFileID, 0xfbu) \
    X(NDS_RELOC_ASSET_IF_COMMON_PLAYER, llIFCommonPlayerArrowsAnimJoint, 0x270u) \
    X(NDS_RELOC_ASSET_IF_COMMON_PLAYER, llIFCommonPlayerArrowsDObjDesc, 0x188u) \
    X(NDS_RELOC_ASSET_IF_COMMON_PLAYER_DAMAGE, llIFCommonPlayerDamageDigit0Sprite, 0x148u) \
    X(NDS_RELOC_ASSET_IF_COMMON_PLAYER_DAMAGE, llIFCommonPlayerDamageDigit1Sprite, 0x2d8u) \
    X(NDS_RELOC_ASSET_IF_COMMON_PLAYER_DAMAGE, llIFCommonPlayerDamageDigit2Sprite, 0x500u) \
    X(NDS_RELOC_ASSET_IF_COMMON_PLAYER_DAMAGE, llIFCommonPlayerDamageDigit3Sprite, 0x698u) \
    X(NDS_RELOC_ASSET_IF_COMMON_PLAYER_DAMAGE, llIFCommonPlayerDamageDigit4Sprite, 0x8c0u) \
    X(NDS_RELOC_ASSET_IF_COMMON_PLAYER_DAMAGE, llIFCommonPlayerDamageDigit5Sprite, 0xa58u) \
    X(NDS_RELOC_ASSET_IF_COMMON_PLAYER_DAMAGE, llIFCommonPlayerDamageDigit6Sprite, 0xc80u) \
    X(NDS_RELOC_ASSET_IF_COMMON_PLAYER_DAMAGE, llIFCommonPlayerDamageDigit7Sprite, 0xe18u) \
    X(NDS_RELOC_ASSET_IF_COMMON_PLAYER_DAMAGE, llIFCommonPlayerDamageDigit8Sprite, 0x1040u) \
    X(NDS_RELOC_ASSET_IF_COMMON_PLAYER_DAMAGE, llIFCommonPlayerDamageDigit9Sprite, 0x1270u) \
    X(NDS_RELOC_ASSET_IF_COMMON_PLAYER_DAMAGE, llIFCommonPlayerDamageSymbolHPSprite, 0x15d8u) \
    X(NDS_RELOC_ASSET_IF_COMMON_PLAYER_DAMAGE, llIFCommonPlayerDamageSymbolPercentSprite, 0x1458u) \
    X(NDS_RELOC_ASSET_IF_COMMON_PLAYER, llIFCommonPlayerMagnifyDisplayList, 0x030u) \
    X(NDS_RELOC_ASSET_IF_COMMON_PLAYER, llIFCommonPlayerMagnifyFrameImage, 0x2c8u) \
    X(NDS_RELOC_ASSET_IF_COMMON_PLAYER_TAGS, llIFCommonPlayerTags1PSprite, 0x258u) \
    X(NDS_RELOC_ASSET_IF_COMMON_PLAYER_TAGS, llIFCommonPlayerTags2PSprite, 0x4f8u) \
    X(NDS_RELOC_ASSET_IF_COMMON_PLAYER_TAGS, llIFCommonPlayerTags3PSprite, 0x798u) \
    X(NDS_RELOC_ASSET_IF_COMMON_PLAYER_TAGS, llIFCommonPlayerTags4PSprite, 0xa38u) \
    X(NDS_RELOC_ASSET_IF_COMMON_PLAYER_TAGS, llIFCommonPlayerTagsAllySprite, 0xeb8u) \
    X(NDS_RELOC_ASSET_IF_COMMON_PLAYER_TAGS, llIFCommonPlayerTagsCPSprite, 0xcd8u) \
    X(NDS_RELOC_ASSET_IF_COMMON_TIMER, llIFCommonTimerDigit0Sprite, 0x138u) \
    X(NDS_RELOC_ASSET_IF_COMMON_TIMER, llIFCommonTimerDigit1Sprite, 0x228u) \
    X(NDS_RELOC_ASSET_IF_COMMON_TIMER, llIFCommonTimerDigit2Sprite, 0x3a8u) \
    X(NDS_RELOC_ASSET_IF_COMMON_TIMER, llIFCommonTimerDigit3Sprite, 0x528u) \
    X(NDS_RELOC_ASSET_IF_COMMON_TIMER, llIFCommonTimerDigit4Sprite, 0x6a8u) \
    X(NDS_RELOC_ASSET_IF_COMMON_TIMER, llIFCommonTimerDigit5Sprite, 0x828u) \
    X(NDS_RELOC_ASSET_IF_COMMON_TIMER, llIFCommonTimerDigit6Sprite, 0x9a8u) \
    X(NDS_RELOC_ASSET_IF_COMMON_TIMER, llIFCommonTimerDigit7Sprite, 0xb28u) \
    X(NDS_RELOC_ASSET_IF_COMMON_TIMER, llIFCommonTimerDigit8Sprite, 0xca8u) \
    X(NDS_RELOC_ASSET_IF_COMMON_TIMER, llIFCommonTimerDigit9Sprite, 0xe28u) \
    X(NDS_RELOC_ASSET_IF_COMMON_TIMER, llIFCommonTimerSymbolColonSprite, 0xf08u) \
    X(NDS_RELOC_ASSET_IF_COMMON_TIMER, llIFCommonTimerSymbolCSecSprite, 0x1238u) \
    X(NDS_RELOC_ASSET_IF_COMMON_TIMER, llIFCommonTimerSymbolSecSprite, 0x1140u)

#define NDS_DECLARE_IFCOMMON_RELOC_SYMBOL(asset, name, value) extern uintptr_t name;
NDS_IFCOMMON_RELOC_SYMBOLS(NDS_DECLARE_IFCOMMON_RELOC_SYMBOL)
#undef NDS_DECLARE_IFCOMMON_RELOC_SYMBOL

extern uintptr_t llMNVSResultsFileID;
extern uintptr_t llFTEmblemModelsFileID;
extern uintptr_t llFTStocksZakoFileID;

#define NDS_VS_RESULTS_RELOC_SYMBOLS(X) \
    X(NDS_RELOC_ASSET_MN_VS_RESULTS, llMNVSResultsTKOTextSprite, 0x0358u) \
    X(NDS_RELOC_ASSET_MN_VS_RESULTS, llMNVSResultsPlaceTextSprite, 0x0990u) \
    X(NDS_RELOC_ASSET_MN_VS_RESULTS, llMNVSResultsKOsTextSprite, 0x0d38u) \
    X(NDS_RELOC_ASSET_MN_VS_RESULTS, llMNVSResultsPtsTextSprite, 0x10d8u) \
    X(NDS_RELOC_ASSET_MN_VS_RESULTS, llMNVSResults1PArrowSprite, 0x49e8u) \
    X(NDS_RELOC_ASSET_MN_VS_RESULTS, llMNVSResults2PArrowSprite, 0x4b08u) \
    X(NDS_RELOC_ASSET_MN_VS_RESULTS, llMNVSResults3PArrowSprite, 0x4c28u) \
    X(NDS_RELOC_ASSET_MN_VS_RESULTS, llMNVSResults4PArrowSprite, 0x4d48u) \
    X(NDS_RELOC_ASSET_MN_VS_RESULTS, llMNVSResultsWallpaperSprite, 0xd5c8u) \
    X(NDS_RELOC_ASSET_MN_VS_RESULTS, llMNVSResultsWinnerSprite, 0xe2a0u) \
    X(NDS_RELOC_ASSET_FT_EMBLEM_MODELS, llFTEmblemModelsMarioMObjSub, 0x0000u) \
    X(NDS_RELOC_ASSET_FT_EMBLEM_MODELS, llFTEmblemModelsMarioDObjDesc, 0x0990u) \
    X(NDS_RELOC_ASSET_FT_EMBLEM_MODELS, llFTEmblemModelsMarioMatAnimJoint, 0x0a14u) \
    X(NDS_RELOC_ASSET_FT_EMBLEM_MODELS, llFTEmblemModelsDonkeyMObjSub, 0x0b00u) \
    X(NDS_RELOC_ASSET_FT_EMBLEM_MODELS, llFTEmblemModelsDonkeyDObjDesc, 0x1348u) \
    X(NDS_RELOC_ASSET_FT_EMBLEM_MODELS, llFTEmblemModelsDonkeyMatAnimJoint, 0x13ccu) \
    X(NDS_RELOC_ASSET_FT_EMBLEM_MODELS, llFTEmblemModelsMetroidMObjSub, 0x1470u) \
    X(NDS_RELOC_ASSET_FT_EMBLEM_MODELS, llFTEmblemModelsMetroidDObjDesc, 0x1860u) \
    X(NDS_RELOC_ASSET_FT_EMBLEM_MODELS, llFTEmblemModelsMetroidMatAnimJoint, 0x18e4u) \
    X(NDS_RELOC_ASSET_FT_EMBLEM_MODELS, llFTEmblemModelsFoxMObjSub, 0x1940u) \
    X(NDS_RELOC_ASSET_FT_EMBLEM_MODELS, llFTEmblemModelsFoxDObjDesc, 0x21d0u) \
    X(NDS_RELOC_ASSET_FT_EMBLEM_MODELS, llFTEmblemModelsFoxMatAnimJoint, 0x2254u) \
    X(NDS_RELOC_ASSET_FT_EMBLEM_MODELS, llFTEmblemModelsZeldaMObjSub, 0x22b0u) \
    X(NDS_RELOC_ASSET_FT_EMBLEM_MODELS, llFTEmblemModelsZeldaDObjDesc, 0x2520u) \
    X(NDS_RELOC_ASSET_FT_EMBLEM_MODELS, llFTEmblemModelsZeldaMatAnimJoint, 0x25a4u) \
    X(NDS_RELOC_ASSET_FT_EMBLEM_MODELS, llFTEmblemModelsYoshiMObjSub, 0x2690u) \
    X(NDS_RELOC_ASSET_FT_EMBLEM_MODELS, llFTEmblemModelsYoshiDObjDesc, 0x2f10u) \
    X(NDS_RELOC_ASSET_FT_EMBLEM_MODELS, llFTEmblemModelsYoshiMatAnimJoint, 0x2f94u) \
    X(NDS_RELOC_ASSET_FT_EMBLEM_MODELS, llFTEmblemModelsFZeroMObjSub, 0x2ff0u) \
    X(NDS_RELOC_ASSET_FT_EMBLEM_MODELS, llFTEmblemModelsFZeroDObjDesc, 0x3828u) \
    X(NDS_RELOC_ASSET_FT_EMBLEM_MODELS, llFTEmblemModelsFZeroMatAnimJoint, 0x38acu) \
    X(NDS_RELOC_ASSET_FT_EMBLEM_MODELS, llFTEmblemModelsKirbyMObjSub, 0x3900u) \
    X(NDS_RELOC_ASSET_FT_EMBLEM_MODELS, llFTEmblemModelsKirbyDObjDesc, 0x3e68u) \
    X(NDS_RELOC_ASSET_FT_EMBLEM_MODELS, llFTEmblemModelsKirbyMatAnimJoint, 0x3eecu) \
    X(NDS_RELOC_ASSET_FT_EMBLEM_MODELS, llFTEmblemModelsPMonstersMObjSub, 0x3f40u) \
    X(NDS_RELOC_ASSET_FT_EMBLEM_MODELS, llFTEmblemModelsPMonstersDObjDesc, 0x4710u) \
    X(NDS_RELOC_ASSET_FT_EMBLEM_MODELS, llFTEmblemModelsPMonstersMatAnimJoint, 0x4794u) \
    X(NDS_RELOC_ASSET_FT_EMBLEM_MODELS, llFTEmblemModelsMotherMObjSub, 0x4840u) \
    X(NDS_RELOC_ASSET_FT_EMBLEM_MODELS, llFTEmblemModelsMotherDObjDesc, 0x5a00u) \
    X(NDS_RELOC_ASSET_FT_EMBLEM_MODELS, llFTEmblemModelsMotherMatAnimJoint, 0x5a84u)

#define NDS_DECLARE_VS_RESULTS_RELOC_SYMBOL(asset, name, value) extern uintptr_t name;
NDS_VS_RESULTS_RELOC_SYMBOLS(NDS_DECLARE_VS_RESULTS_RELOC_SYMBOL)
#undef NDS_DECLARE_VS_RESULTS_RELOC_SYMBOL

extern uintptr_t llLBTransitionAeroplaneFileID;
extern uintptr_t llLBTransitionCheckFileID;
extern uintptr_t llLBTransitionGakubuthiFileID;
extern uintptr_t llLBTransitionKannonFileID;
extern uintptr_t llLBTransitionStarFileID;
extern uintptr_t llLBTransitionSudare1FileID;
extern uintptr_t llLBTransitionSudare2FileID;
extern uintptr_t llLBTransitionBlockFileID;
extern uintptr_t llLBTransitionRotScaleFileID;
extern uintptr_t llLBTransitionCurtainFileID;
extern uintptr_t llLBTransitionCameraFileID;

#define NDS_TRANSITION_RELOC_SYMBOLS(X) \
    X(NDS_RELOC_ASSET_TRANSITION_AEROPLANE, llLBTransitionAeroplaneDObjDesc, 0xb3f8u) \
    X(NDS_RELOC_ASSET_TRANSITION_AEROPLANE, llLBTransitionAeroplaneAnimJoint, 0xb710u) \
    X(NDS_RELOC_ASSET_TRANSITION_CHECK, llLBTransitionCheckDObjDesc, 0x3e80u) \
    X(NDS_RELOC_ASSET_TRANSITION_CHECK, llLBTransitionCheckAnimJoint, 0x4038u) \
    X(NDS_RELOC_ASSET_TRANSITION_GAKUBUTHI, llLBTransitionGakubuthiDObjDesc, 0x0f98u) \
    X(NDS_RELOC_ASSET_TRANSITION_GAKUBUTHI, llLBTransitionGakubuthiAnimJoint, 0x101cu) \
    X(NDS_RELOC_ASSET_TRANSITION_KANNON, llLBTransitionKannonDObjDesc, 0x1f00u) \
    X(NDS_RELOC_ASSET_TRANSITION_KANNON, llLBTransitionKannonAnimJoint, 0x1fb0u) \
    X(NDS_RELOC_ASSET_TRANSITION_STAR, llLBTransitionStarDObjDesc, 0x2450u) \
    X(NDS_RELOC_ASSET_TRANSITION_STAR, llLBTransitionStarAnimJoint, 0x24d4u) \
    X(NDS_RELOC_ASSET_TRANSITION_SUDARE1, llLBTransitionSudare1DObjDesc, 0x74a8u) \
    X(NDS_RELOC_ASSET_TRANSITION_SUDARE1, llLBTransitionSudare1AnimJoint, 0x7660u) \
    X(NDS_RELOC_ASSET_TRANSITION_SUDARE2, llLBTransitionSudare2DObjDesc, 0x3ea0u) \
    X(NDS_RELOC_ASSET_TRANSITION_SUDARE2, llLBTransitionSudare2AnimJoint, 0x3f50u) \
    X(NDS_RELOC_ASSET_TRANSITION_BLOCK, llLBTransitionBlockDObjDesc, 0x4e18u) \
    X(NDS_RELOC_ASSET_TRANSITION_BLOCK, llLBTransitionBlockAnimJoint, 0x536cu) \
    X(NDS_RELOC_ASSET_TRANSITION_ROTSCALE, llLBTransitionRotScaleDObjDesc, 0x0f98u) \
    X(NDS_RELOC_ASSET_TRANSITION_ROTSCALE, llLBTransitionRotScaleAnimJoint, 0x101cu) \
    X(NDS_RELOC_ASSET_TRANSITION_CURTAIN, llLBTransitionCurtainDObjDesc, 0x7ae0u) \
    X(NDS_RELOC_ASSET_TRANSITION_CURTAIN, llLBTransitionCurtainAnimJoint, 0x7c98u) \
    X(NDS_RELOC_ASSET_TRANSITION_CAMERA, llLBTransitionCameraDObjDesc, 0x3f90u) \
    X(NDS_RELOC_ASSET_TRANSITION_CAMERA, llLBTransitionCameraAnimJoint, 0x4148u)

#define NDS_DECLARE_TRANSITION_RELOC_SYMBOL(asset, name, value) extern uintptr_t name;
NDS_TRANSITION_RELOC_SYMBOLS(NDS_DECLARE_TRANSITION_RELOC_SYMBOL)
#undef NDS_DECLARE_TRANSITION_RELOC_SYMBOL

/* BonusPicturePlatform (reloc file 0xe, reloc_bonus): staged by scripts/menus/stage_reloc_file.py. */
extern uintptr_t llBonusPicturePlatformFileID;

#define NDS_BONUS_PICTURE_PLATFORM_RELOC_SYMBOLS(X) \
    X(NDS_RELOC_ASSET_BONUS_PICTURE_PLATFORM, llBonusPicturePlatformSprite, 0x27388u)

#define NDS_DECLARE_BONUS_PICTURE_PLATFORM_RELOC_SYMBOL(asset, name, value) extern uintptr_t name;
NDS_BONUS_PICTURE_PLATFORM_RELOC_SYMBOLS(NDS_DECLARE_BONUS_PICTURE_PLATFORM_RELOC_SYMBOL)
#undef NDS_DECLARE_BONUS_PICTURE_PLATFORM_RELOC_SYMBOL

/* BonusPicture (reloc file 0xd, reloc_bonus): staged by scripts/menus/stage_reloc_file.py. */
extern uintptr_t llBonusPictureFileID;

#define NDS_BONUS_PICTURE_RELOC_SYMBOLS(X) \
    X(NDS_RELOC_ASSET_BONUS_PICTURE, llBonusPictureTargetSprite, 0xe980u) \
    X(NDS_RELOC_ASSET_BONUS_PICTURE, llBonusPictureRaceSprite, 0x1a658u)

#define NDS_DECLARE_BONUS_PICTURE_RELOC_SYMBOL(asset, name, value) extern uintptr_t name;
NDS_BONUS_PICTURE_RELOC_SYMBOLS(NDS_DECLARE_BONUS_PICTURE_RELOC_SYMBOL)
#undef NDS_DECLARE_BONUS_PICTURE_RELOC_SYMBOL

/* CharacterNames (reloc file 0xc, reloc_misc_named): staged by scripts/menus/stage_reloc_file.py. */
extern uintptr_t llCharacterNamesFileID;

#define NDS_CHARACTER_NAMES_RELOC_SYMBOLS(X) \
    X(NDS_RELOC_ASSET_CHARACTER_NAMES, llCharacterNamesMarioSprite, 0x0138u) \
    X(NDS_RELOC_ASSET_CHARACTER_NAMES, llCharacterNamesFoxSprite, 0x0258u) \
    X(NDS_RELOC_ASSET_CHARACTER_NAMES, llCharacterNamesDonkeySprite, 0x0378u) \
    X(NDS_RELOC_ASSET_CHARACTER_NAMES, llCharacterNamesSamusSprite, 0x04f8u) \
    X(NDS_RELOC_ASSET_CHARACTER_NAMES, llCharacterNamesLuigiSprite, 0x0618u) \
    X(NDS_RELOC_ASSET_CHARACTER_NAMES, llCharacterNamesLinkSprite, 0x0738u) \
    X(NDS_RELOC_ASSET_CHARACTER_NAMES, llCharacterNamesYoshiSprite, 0x0858u) \
    X(NDS_RELOC_ASSET_CHARACTER_NAMES, llCharacterNamesCaptainSprite, 0x0a38u) \
    X(NDS_RELOC_ASSET_CHARACTER_NAMES, llCharacterNamesKirbySprite, 0x0bb8u) \
    X(NDS_RELOC_ASSET_CHARACTER_NAMES, llCharacterNamesPikachuSprite, 0x0d38u) \
    X(NDS_RELOC_ASSET_CHARACTER_NAMES, llCharacterNamesPurinSprite, 0x0f78u) \
    X(NDS_RELOC_ASSET_CHARACTER_NAMES, llCharacterNamesNessSprite, 0x1098u)

#define NDS_DECLARE_CHARACTER_NAMES_RELOC_SYMBOL(asset, name, value) extern uintptr_t name;
NDS_CHARACTER_NAMES_RELOC_SYMBOLS(NDS_DECLARE_CHARACTER_NAMES_RELOC_SYMBOL)
#undef NDS_DECLARE_CHARACTER_NAMES_RELOC_SYMBOL

/* MNPlayersDifficulty (reloc file 0x18, reloc_menus): staged by scripts/menus/stage_reloc_file.py. */
extern uintptr_t llMNPlayersDifficultyFileID;

#define NDS_MN_PLAYERS_DIFFICULTY_RELOC_SYMBOLS(X) \
    X(NDS_RELOC_ASSET_MN_PLAYERS_DIFFICULTY, llMNPlayersDifficultyEasyTextSprite, 0x0098u) \
    X(NDS_RELOC_ASSET_MN_PLAYERS_DIFFICULTY, llMNPlayersDifficultyHardTextSprite, 0x0178u) \
    X(NDS_RELOC_ASSET_MN_PLAYERS_DIFFICULTY, llMNPlayersDifficultyNormalTextSprite, 0x02d8u) \
    X(NDS_RELOC_ASSET_MN_PLAYERS_DIFFICULTY, llMNPlayersDifficultyVeryEasyTextSprite, 0x0438u) \
    X(NDS_RELOC_ASSET_MN_PLAYERS_DIFFICULTY, llMNPlayersDifficultyVeryHardTextSprite, 0x0598u)

#define NDS_DECLARE_MN_PLAYERS_DIFFICULTY_RELOC_SYMBOL(asset, name, value) extern uintptr_t name;
NDS_MN_PLAYERS_DIFFICULTY_RELOC_SYMBOLS(NDS_DECLARE_MN_PLAYERS_DIFFICULTY_RELOC_SYMBOL)
#undef NDS_DECLARE_MN_PLAYERS_DIFFICULTY_RELOC_SYMBOL

/* IFCommonTimer (reloc file 0xa5, reloc_interface): symbols the hand table lacks, staged by scripts/menus/stage_reloc_file.py. */
#define NDS_IF_COMMON_TIMER_EXTRA_RELOC_SYMBOLS(X) \
    X(NDS_RELOC_ASSET_IF_COMMON_TIMER, llIFCommonTimerSymbolCrossSprite, 0x1018u) \
    X(NDS_RELOC_ASSET_IF_COMMON_TIMER, llIFCommonTimerSymbolUnderscoreSprite, 0x1090u)

#define NDS_DECLARE_IF_COMMON_TIMER_EXTRA_RELOC_SYMBOL(asset, name, value) extern uintptr_t name;
NDS_IF_COMMON_TIMER_EXTRA_RELOC_SYMBOLS(NDS_DECLARE_IF_COMMON_TIMER_EXTRA_RELOC_SYMBOL)
#undef NDS_DECLARE_IF_COMMON_TIMER_EXTRA_RELOC_SYMBOL

/* IFCommonDigits (reloc file 0x24, reloc_interface): symbols the hand table lacks, staged by scripts/menus/stage_reloc_file.py. */
#define NDS_IF_COMMON_DIGITS_EXTRA_RELOC_SYMBOLS(X) \
    X(NDS_RELOC_ASSET_IF_COMMON_DIGITS, llIFCommonDigitsColonSprite, 0x08d8u)

#define NDS_DECLARE_IF_COMMON_DIGITS_EXTRA_RELOC_SYMBOL(asset, name, value) extern uintptr_t name;
NDS_IF_COMMON_DIGITS_EXTRA_RELOC_SYMBOLS(NDS_DECLARE_IF_COMMON_DIGITS_EXTRA_RELOC_SYMBOL)
#undef NDS_DECLARE_IF_COMMON_DIGITS_EXTRA_RELOC_SYMBOL

/* SCExplainMain (reloc file 0xfc, reloc_scene): staged by scripts/menus/stage_reloc_file.py. */
extern uintptr_t llSCExplainMainFileID;

#define NDS_SC_EXPLAIN_MAIN_RELOC_SYMBOLS(X) \
    X(NDS_RELOC_ASSET_SC_EXPLAIN_MAIN, llSCExplainMain0KeyEvent, 0x0000u) \
    X(NDS_RELOC_ASSET_SC_EXPLAIN_MAIN, llSCExplainMain1KeyEvent, 0x09d4u) \
    X(NDS_RELOC_ASSET_SC_EXPLAIN_MAIN, llSCExplainMain2KeyEvent, 0x13fcu) \
    X(NDS_RELOC_ASSET_SC_EXPLAIN_MAIN, llSCExplainMain3KeyEvent, 0x1400u) \
    X(NDS_RELOC_ASSET_SC_EXPLAIN_MAIN, llSCExplainMainExplainPhase, 0x1404u)

#define NDS_DECLARE_SC_EXPLAIN_MAIN_RELOC_SYMBOL(asset, name, value) extern uintptr_t name;
NDS_SC_EXPLAIN_MAIN_RELOC_SYMBOLS(NDS_DECLARE_SC_EXPLAIN_MAIN_RELOC_SYMBOL)
#undef NDS_DECLARE_SC_EXPLAIN_MAIN_RELOC_SYMBOL

/* SCExplainGraphics (reloc file 0xc6, reloc_scene): staged by scripts/menus/stage_reloc_file.py. */
extern uintptr_t llSCExplainGraphicsFileID;

#define NDS_SC_EXPLAIN_GRAPHICS_RELOC_SYMBOLS(X) \
    X(NDS_RELOC_ASSET_SC_EXPLAIN_GRAPHICS, llSCExplainGraphicsStickMObjSub, 0x5028u) \
    X(NDS_RELOC_ASSET_SC_EXPLAIN_GRAPHICS, llSCExplainGraphicsStickDObjDesc, 0x5300u) \
    X(NDS_RELOC_ASSET_SC_EXPLAIN_GRAPHICS, llSCExplainGraphicsStickNeutralMatAnimJoint, 0x5390u) \
    X(NDS_RELOC_ASSET_SC_EXPLAIN_GRAPHICS, llSCExplainGraphicsStickHoldUpMatAnimJoint, 0x53c0u) \
    X(NDS_RELOC_ASSET_SC_EXPLAIN_GRAPHICS, llSCExplainGraphicsStickTapUpMatAnimJoint, 0x53f0u) \
    X(NDS_RELOC_ASSET_SC_EXPLAIN_GRAPHICS, llSCExplainGraphicsStickHoldForwardMatAnimJoint, 0x5430u) \
    X(NDS_RELOC_ASSET_SC_EXPLAIN_GRAPHICS, llSCExplainGraphicsStickTapForwardMatAnimJoint, 0x5450u) \
    X(NDS_RELOC_ASSET_SC_EXPLAIN_GRAPHICS, llSCExplainGraphicsTapSparkMObjSub, 0x5a98u) \
    X(NDS_RELOC_ASSET_SC_EXPLAIN_GRAPHICS, llSCExplainGraphicsTapSparkDisplayList, 0x5b68u) \
    X(NDS_RELOC_ASSET_SC_EXPLAIN_GRAPHICS, llSCExplainGraphicsTapSparkMatAnimJoint, 0x5c20u) \
    X(NDS_RELOC_ASSET_SC_EXPLAIN_GRAPHICS, llSCExplainGraphicsSpecialMoveRGBDisplayList, 0x5e40u) \
    X(NDS_RELOC_ASSET_SC_EXPLAIN_GRAPHICS, llSCExplainGraphicsDamage1Sprite, 0x6c58u) \
    X(NDS_RELOC_ASSET_SC_EXPLAIN_GRAPHICS, llSCExplainGraphicsDamage2Sprite, 0x72d8u) \
    X(NDS_RELOC_ASSET_SC_EXPLAIN_GRAPHICS, llSCExplainGraphicsDamage3Sprite, 0x7c38u) \
    X(NDS_RELOC_ASSET_SC_EXPLAIN_GRAPHICS, llSCExplainGraphicsShieldSprite, 0x8218u) \
    X(NDS_RELOC_ASSET_SC_EXPLAIN_GRAPHICS, llSCExplainGraphicsStartFightingSprite, 0x8c78u) \
    X(NDS_RELOC_ASSET_SC_EXPLAIN_GRAPHICS, llSCExplainGraphicsPlayerCountSprite, 0x91a8u) \
    X(NDS_RELOC_ASSET_SC_EXPLAIN_GRAPHICS, llSCExplainGraphicsHereTextSprite, 0x9628u) \
    X(NDS_RELOC_ASSET_SC_EXPLAIN_GRAPHICS, llSCExplainGraphicsBannerSprite, 0x10260u) \
    X(NDS_RELOC_ASSET_SC_EXPLAIN_GRAPHICS, llSCExplainGraphicsTapTheStickSprite, 0x11f60u) \
    X(NDS_RELOC_ASSET_SC_EXPLAIN_GRAPHICS, llSCExplainGraphicsJumpSprite, 0x12b60u) \
    X(NDS_RELOC_ASSET_SC_EXPLAIN_GRAPHICS, llSCExplainGraphicsMidairJumpSprite, 0x13658u) \
    X(NDS_RELOC_ASSET_SC_EXPLAIN_GRAPHICS, llSCExplainGraphicsAttackSprite, 0x139f0u) \
    X(NDS_RELOC_ASSET_SC_EXPLAIN_GRAPHICS, llSCExplainGraphicsPowerAttackSprite, 0x14448u) \
    X(NDS_RELOC_ASSET_SC_EXPLAIN_GRAPHICS, llSCExplainGraphicsSmashAttackSprite, 0x14e30u) \
    X(NDS_RELOC_ASSET_SC_EXPLAIN_GRAPHICS, llSCExplainGraphicsKnockThemOffSprite, 0x15c40u) \
    X(NDS_RELOC_ASSET_SC_EXPLAIN_GRAPHICS, llSCExplainGraphicsBUpGetBackSprite, 0x17fe0u) \
    X(NDS_RELOC_ASSET_SC_EXPLAIN_GRAPHICS, llSCExplainGraphicsSpecialMovesSprite, 0x1a440u) \
    X(NDS_RELOC_ASSET_SC_EXPLAIN_GRAPHICS, llSCExplainGraphicsDodgeSprite, 0x1aa10u) \
    X(NDS_RELOC_ASSET_SC_EXPLAIN_GRAPHICS, llSCExplainGraphicsThrowEnemySprite, 0x1b468u) \
    X(NDS_RELOC_ASSET_SC_EXPLAIN_GRAPHICS, llSCExplainGraphicsGrabItemsSprite, 0x1b950u) \
    X(NDS_RELOC_ASSET_SC_EXPLAIN_GRAPHICS, llSCExplainGraphicsUseItemsSprite, 0x1beb0u) \
    X(NDS_RELOC_ASSET_SC_EXPLAIN_GRAPHICS, llSCExplainGraphicsThrowItemsSprite, 0x1cd20u) \
    X(NDS_RELOC_ASSET_SC_EXPLAIN_GRAPHICS, llSCExplainGraphicsAButtonSprite, 0x1d338u) \
    X(NDS_RELOC_ASSET_SC_EXPLAIN_GRAPHICS, llSCExplainGraphicsBButtonSprite, 0x1d948u) \
    X(NDS_RELOC_ASSET_SC_EXPLAIN_GRAPHICS, llSCExplainGraphicsZButtonSprite, 0x1df58u) \
    X(NDS_RELOC_ASSET_SC_EXPLAIN_GRAPHICS, llSCExplainGraphicsPlusSymbolSprite, 0x1e018u)

#define NDS_DECLARE_SC_EXPLAIN_GRAPHICS_RELOC_SYMBOL(asset, name, value) extern uintptr_t name;
NDS_SC_EXPLAIN_GRAPHICS_RELOC_SYMBOLS(NDS_DECLARE_SC_EXPLAIN_GRAPHICS_RELOC_SYMBOL)
#undef NDS_DECLARE_SC_EXPLAIN_GRAPHICS_RELOC_SYMBOL

/* GRWallpaperTrainingYellow (reloc file 0x1b, reloc_stages): staged by scripts/menus/stage_reloc_file.py. */
extern uintptr_t llGRWallpaperTrainingYellowFileID;

#define NDS_GR_WALLPAPER_TRAINING_YELLOW_RELOC_SYMBOLS(X) \
    X(NDS_RELOC_ASSET_GR_WALLPAPER_TRAINING_YELLOW, llGRWallpaperTrainingYellowSprite, 0x20718u)

#define NDS_DECLARE_GR_WALLPAPER_TRAINING_YELLOW_RELOC_SYMBOL(asset, name, value) extern uintptr_t name;
NDS_GR_WALLPAPER_TRAINING_YELLOW_RELOC_SYMBOLS(NDS_DECLARE_GR_WALLPAPER_TRAINING_YELLOW_RELOC_SYMBOL)
#undef NDS_DECLARE_GR_WALLPAPER_TRAINING_YELLOW_RELOC_SYMBOL

/* GRWallpaperTrainingBlue (reloc file 0x1c, reloc_stages): staged by scripts/menus/stage_reloc_file.py. */
extern uintptr_t llGRWallpaperTrainingBlueFileID;

#define NDS_GR_WALLPAPER_TRAINING_BLUE_RELOC_SYMBOLS(X) \
    X(NDS_RELOC_ASSET_GR_WALLPAPER_TRAINING_BLUE, llGRWallpaperTrainingBlueSprite, 0x20718u)

#define NDS_DECLARE_GR_WALLPAPER_TRAINING_BLUE_RELOC_SYMBOL(asset, name, value) extern uintptr_t name;
NDS_GR_WALLPAPER_TRAINING_BLUE_RELOC_SYMBOLS(NDS_DECLARE_GR_WALLPAPER_TRAINING_BLUE_RELOC_SYMBOL)
#undef NDS_DECLARE_GR_WALLPAPER_TRAINING_BLUE_RELOC_SYMBOL

/* GRWallpaperTrainingBlack (reloc file 0x1a, reloc_stages): staged by scripts/menus/stage_reloc_file.py. */
extern uintptr_t llGRWallpaperTrainingBlackFileID;

#define NDS_GR_WALLPAPER_TRAINING_BLACK_RELOC_SYMBOLS(X) \
    X(NDS_RELOC_ASSET_GR_WALLPAPER_TRAINING_BLACK, llGRWallpaperTrainingBlackSprite, 0x20718u)

#define NDS_DECLARE_GR_WALLPAPER_TRAINING_BLACK_RELOC_SYMBOL(asset, name, value) extern uintptr_t name;
NDS_GR_WALLPAPER_TRAINING_BLACK_RELOC_SYMBOLS(NDS_DECLARE_GR_WALLPAPER_TRAINING_BLACK_RELOC_SYMBOL)
#undef NDS_DECLARE_GR_WALLPAPER_TRAINING_BLACK_RELOC_SYMBOL

/* SC1PTrainingMode (reloc file 0xfe, reloc_scene): staged by scripts/menus/stage_reloc_file.py. */
extern uintptr_t llSC1PTrainingModeFileID;

#define NDS_SC1P_TRAINING_MODE_RELOC_SYMBOLS(X) \
    X(NDS_RELOC_ASSET_SC1P_TRAINING_MODE, llSC1PTrainingModeDisplayLabelPosSpriteArray, 0x0000u) \
    X(NDS_RELOC_ASSET_SC1P_TRAINING_MODE, llSC1PTrainingModeDisplayOptionSpriteArray, 0x0020u) \
    X(NDS_RELOC_ASSET_SC1P_TRAINING_MODE, llSC1PTrainingModeMenuLabelPosSpriteArray, 0x00bcu) \
    X(NDS_RELOC_ASSET_SC1P_TRAINING_MODE, llSC1PTrainingMode0x10CPosSpriteArray, 0x010cu) \
    X(NDS_RELOC_ASSET_SC1P_TRAINING_MODE, llSC1PTrainingModeMenuOptionSpriteArray, 0x013cu) \
    X(NDS_RELOC_ASSET_SC1P_TRAINING_MODE, llSC1PTrainingMode0x1B8PosSpriteArray, 0x01b8u)

#define NDS_DECLARE_SC1P_TRAINING_MODE_RELOC_SYMBOL(asset, name, value) extern uintptr_t name;
NDS_SC1P_TRAINING_MODE_RELOC_SYMBOLS(NDS_DECLARE_SC1P_TRAINING_MODE_RELOC_SYMBOL)
#undef NDS_DECLARE_SC1P_TRAINING_MODE_RELOC_SYMBOL

/* MNSoundTest (reloc file 0xc4, reloc_menus): staged by scripts/menus/stage_reloc_file.py. */
extern uintptr_t llMNSoundTestFileID;

#define NDS_MN_SOUND_TEST_RELOC_SYMBOLS(X) \
    X(NDS_RELOC_ASSET_MN_SOUND_TEST, llMNSoundTestMusicTextSprite, 0x0438u) \
    X(NDS_RELOC_ASSET_MN_SOUND_TEST, llMNSoundTestSoundTextSprite, 0x09c0u) \
    X(NDS_RELOC_ASSET_MN_SOUND_TEST, llMNSoundTestVoiceTextSprite, 0x0e48u) \
    X(NDS_RELOC_ASSET_MN_SOUND_TEST, llMNSoundTestCapsuleRightSprite, 0x1138u) \
    X(NDS_RELOC_ASSET_MN_SOUND_TEST, llMNSoundTestColonExitTextSprite, 0x1208u) \
    X(NDS_RELOC_ASSET_MN_SOUND_TEST, llMNSoundTestColonFadeOutTextSprite, 0x1348u) \
    X(NDS_RELOC_ASSET_MN_SOUND_TEST, llMNSoundTestColonPlayTextSprite, 0x1450u) \
    X(NDS_RELOC_ASSET_MN_SOUND_TEST, llMNSoundTestSoundTestTextSprite, 0x1bb8u) \
    X(NDS_RELOC_ASSET_MN_SOUND_TEST, llMNSoundTestStartButtonSprite, 0x1d50u)

#define NDS_DECLARE_MN_SOUND_TEST_RELOC_SYMBOL(asset, name, value) extern uintptr_t name;
NDS_MN_SOUND_TEST_RELOC_SYMBOLS(NDS_DECLARE_MN_SOUND_TEST_RELOC_SYMBOL)
#undef NDS_DECLARE_MN_SOUND_TEST_RELOC_SYMBOL

/* MNBackupClearHeaderOption (reloc file 0x4e, reloc_menus): staged by scripts/menus/stage_reloc_file.py. */
extern uintptr_t llMNBackupClearHeaderOptionFileID;

#define NDS_MN_BACKUP_CLEAR_HEADER_OPTION_RELOC_SYMBOLS(X) \
    X(NDS_RELOC_ASSET_MN_BACKUP_CLEAR_HEADER_OPTION, llMNBackupClearHeaderOptionSprite, 0x0b40u)

#define NDS_DECLARE_MN_BACKUP_CLEAR_HEADER_OPTION_RELOC_SYMBOL(asset, name, value) extern uintptr_t name;
NDS_MN_BACKUP_CLEAR_HEADER_OPTION_RELOC_SYMBOLS(NDS_DECLARE_MN_BACKUP_CLEAR_HEADER_OPTION_RELOC_SYMBOL)
#undef NDS_DECLARE_MN_BACKUP_CLEAR_HEADER_OPTION_RELOC_SYMBOL

/* MNBackupClear (reloc file 0x4d, reloc_menus): staged by scripts/menus/stage_reloc_file.py. */
extern uintptr_t llMNBackupClearFileID;

#define NDS_MN_BACKUP_CLEAR_RELOC_SYMBOLS(X) \
    X(NDS_RELOC_ASSET_MN_BACKUP_CLEAR, llMNBackupClearHeaderBackupClearSprite, 0x0b60u) \
    X(NDS_RELOC_ASSET_MN_BACKUP_CLEAR, llMNBackupClear0x11e0Sprite, 0x11e0u) \
    X(NDS_RELOC_ASSET_MN_BACKUP_CLEAR, llMNBackupClear0x1770Sprite, 0x1770u) \
    X(NDS_RELOC_ASSET_MN_BACKUP_CLEAR, llMNBackupClear0x1b98Sprite, 0x1b98u) \
    X(NDS_RELOC_ASSET_MN_BACKUP_CLEAR, llMNBackupClear0x2300Sprite, 0x2300u) \
    X(NDS_RELOC_ASSET_MN_BACKUP_CLEAR, llMNBackupClear0x2890Sprite, 0x2890u) \
    X(NDS_RELOC_ASSET_MN_BACKUP_CLEAR, llMNBackupClear0x2d30Sprite, 0x2d30u) \
    X(NDS_RELOC_ASSET_MN_BACKUP_CLEAR, llMNBackupClear0x33b0Sprite, 0x33b0u) \
    X(NDS_RELOC_ASSET_MN_BACKUP_CLEAR, llMNBackupClearOptionNewcomersSprite, 0x3a00u) \
    X(NDS_RELOC_ASSET_MN_BACKUP_CLEAR, llMNBackupClearOption1PHighScoreSprite, 0x4050u) \
    X(NDS_RELOC_ASSET_MN_BACKUP_CLEAR, llMNBackupClearOptionVSRecordSprite, 0x46a0u) \
    X(NDS_RELOC_ASSET_MN_BACKUP_CLEAR, llMNBackupClearOptionSubjectModeSprite, 0x4cf0u) \
    X(NDS_RELOC_ASSET_MN_BACKUP_CLEAR, llMNBackupClearOptionPrizeSprite, 0x5340u) \
    X(NDS_RELOC_ASSET_MN_BACKUP_CLEAR, llMNBackupClearOptionAllDataClearSprite, 0x5990u) \
    X(NDS_RELOC_ASSET_MN_BACKUP_CLEAR, llMNBackupClearOptionCircleSprite, 0x5db8u) \
    X(NDS_RELOC_ASSET_MN_BACKUP_CLEAR, llMNBackupClearIsOkayTextSprite, 0x63c8u) \
    X(NDS_RELOC_ASSET_MN_BACKUP_CLEAR, llMNBackupClearAreYouSureTextSprite, 0x69d8u) \
    X(NDS_RELOC_ASSET_MN_BACKUP_CLEAR, llMNBackupClearOptionBonusStageTimeSprite, 0x7020u) \
    X(NDS_RELOC_ASSET_MN_BACKUP_CLEAR, llMNBackupClearOptionYesHighlightPalette, 0x7500u) \
    X(NDS_RELOC_ASSET_MN_BACKUP_CLEAR, llMNBackupClearOptionYesNotPalette, 0x7528u) \
    X(NDS_RELOC_ASSET_MN_BACKUP_CLEAR, llMNBackupClearOptionConfirmPalette, 0x7550u) \
    X(NDS_RELOC_ASSET_MN_BACKUP_CLEAR, llMNBackupClearOptionYesSprite, 0x7580u) \
    X(NDS_RELOC_ASSET_MN_BACKUP_CLEAR, llMNBackupClearOptionNoHighlightPalette, 0x7a60u) \
    X(NDS_RELOC_ASSET_MN_BACKUP_CLEAR, llMNBackupClearOptionNoNotPalette, 0x7a88u) \
    X(NDS_RELOC_ASSET_MN_BACKUP_CLEAR, llMNBackupClearOptionNoSprite, 0x7ab8u)

#define NDS_DECLARE_MN_BACKUP_CLEAR_RELOC_SYMBOL(asset, name, value) extern uintptr_t name;
NDS_MN_BACKUP_CLEAR_RELOC_SYMBOLS(NDS_DECLARE_MN_BACKUP_CLEAR_RELOC_SYMBOL)
#undef NDS_DECLARE_MN_BACKUP_CLEAR_RELOC_SYMBOL

/* MNOption (reloc file 0x4, reloc_menus): staged by scripts/menus/stage_reloc_file.py. */
extern uintptr_t llMNOptionFileID;

#define NDS_MN_OPTION_RELOC_SYMBOLS(X) \
    X(NDS_RELOC_ASSET_MN_OPTION, llMNOptionSoundTextJapSprite, 0x03d8u) \
    X(NDS_RELOC_ASSET_MN_OPTION, llMNOptionStereoTextJapSprite, 0x06a8u) \
    X(NDS_RELOC_ASSET_MN_OPTION, llMNOptionMonoTextJapSprite, 0x0978u) \
    X(NDS_RELOC_ASSET_MN_OPTION, llMNOptionFlashingTextJapSprite, 0x0ff0u) \
    X(NDS_RELOC_ASSET_MN_OPTION, llMNOptionScreenAdjustTextJapSprite, 0x1580u) \
    X(NDS_RELOC_ASSET_MN_OPTION, llMNOptionBackupClearTextJapSprite, 0x1cf0u) \
    X(NDS_RELOC_ASSET_MN_OPTION, llMNOptionWarningTextSprite, 0x21b8u) \
    X(NDS_RELOC_ASSET_MN_OPTION, llMNOptionPhotosensitivityWarningTextJapSprite, 0x68b0u) \
    X(NDS_RELOC_ASSET_MN_OPTION, llMNOptionWarningIconSprite, 0x6fd8u) \
    X(NDS_RELOC_ASSET_MN_OPTION, llMNOptionStereoTextSprite, 0x71f8u) \
    X(NDS_RELOC_ASSET_MN_OPTION, llMNOptionMonoTextSprite, 0x73a8u) \
    X(NDS_RELOC_ASSET_MN_OPTION, llMNOptionSoundTextSprite, 0x7628u) \
    X(NDS_RELOC_ASSET_MN_OPTION, llMNOptionFlashingTextSprite, 0x7aa8u) \
    X(NDS_RELOC_ASSET_MN_OPTION, llMNOptionScreenAdjustTextSprite, 0x8138u) \
    X(NDS_RELOC_ASSET_MN_OPTION, llMNOptionBackupClearTextSprite, 0x8780u) \
    X(NDS_RELOC_ASSET_MN_OPTION, llMNOptionOptionTextSprite, 0x9288u) \
    X(NDS_RELOC_ASSET_MN_OPTION, llMNOptionSettingsIconDarkSprite, 0xb958u)

#define NDS_DECLARE_MN_OPTION_RELOC_SYMBOL(asset, name, value) extern uintptr_t name;
NDS_MN_OPTION_RELOC_SYMBOLS(NDS_DECLARE_MN_OPTION_RELOC_SYMBOL)
#undef NDS_DECLARE_MN_OPTION_RELOC_SYMBOL

/* MNCharacters (reloc file 0x10, reloc_menus): staged by scripts/menus/stage_reloc_file.py. */
extern uintptr_t llMNCharactersFileID;

#define NDS_MN_CHARACTERS_RELOC_SYMBOLS(X) \
    X(NDS_RELOC_ASSET_MN_CHARACTERS, llMNCharactersLabelSprite, 0x0630u) \
    X(NDS_RELOC_ASSET_MN_CHARACTERS, llMNCharactersNameTagDefaultSprite, 0x1230u) \
    X(NDS_RELOC_ASSET_MN_CHARACTERS, llMNCharactersNameTagTallSprite, 0x28f0u) \
    X(NDS_RELOC_ASSET_MN_CHARACTERS, llMNCharactersMarioNameSprite, 0x2f98u) \
    X(NDS_RELOC_ASSET_MN_CHARACTERS, llMNCharactersFoxNameSprite, 0x33a0u) \
    X(NDS_RELOC_ASSET_MN_CHARACTERS, llMNCharactersDonkeyNameSprite, 0x4290u) \
    X(NDS_RELOC_ASSET_MN_CHARACTERS, llMNCharactersSamusNameSprite, 0x4910u) \
    X(NDS_RELOC_ASSET_MN_CHARACTERS, llMNCharactersLuigiNameSprite, 0x4f78u) \
    X(NDS_RELOC_ASSET_MN_CHARACTERS, llMNCharactersLinkNameSprite, 0x5398u) \
    X(NDS_RELOC_ASSET_MN_CHARACTERS, llMNCharactersYoshiNameSprite, 0x58f8u) \
    X(NDS_RELOC_ASSET_MN_CHARACTERS, llMNCharactersCaptainNameSprite, 0x6828u) \
    X(NDS_RELOC_ASSET_MN_CHARACTERS, llMNCharactersKirbyNameSprite, 0x6e48u) \
    X(NDS_RELOC_ASSET_MN_CHARACTERS, llMNCharactersPikachuNameSprite, 0x7628u) \
    X(NDS_RELOC_ASSET_MN_CHARACTERS, llMNCharactersPurinNameSprite, 0x82e0u) \
    X(NDS_RELOC_ASSET_MN_CHARACTERS, llMNCharactersNessNameSprite, 0x8828u) \
    X(NDS_RELOC_ASSET_MN_CHARACTERS, llMNCharactersMarioStorySprite, 0xaca8u) \
    X(NDS_RELOC_ASSET_MN_CHARACTERS, llMNCharactersFoxStorySprite, 0xd128u) \
    X(NDS_RELOC_ASSET_MN_CHARACTERS, llMNCharactersDonkeyStorySprite, 0xf5a8u) \
    X(NDS_RELOC_ASSET_MN_CHARACTERS, llMNCharactersSamusStorySprite, 0x11a28u) \
    X(NDS_RELOC_ASSET_MN_CHARACTERS, llMNCharactersLuigiStorySprite, 0x13ea8u) \
    X(NDS_RELOC_ASSET_MN_CHARACTERS, llMNCharactersLinkStorySprite, 0x16328u) \
    X(NDS_RELOC_ASSET_MN_CHARACTERS, llMNCharactersYoshiStorySprite, 0x187a8u) \
    X(NDS_RELOC_ASSET_MN_CHARACTERS, llMNCharactersCaptainStorySprite, 0x1ac28u) \
    X(NDS_RELOC_ASSET_MN_CHARACTERS, llMNCharactersKirbyStorySprite, 0x1d0a8u) \
    X(NDS_RELOC_ASSET_MN_CHARACTERS, llMNCharactersPikachuStorySprite, 0x1f528u) \
    X(NDS_RELOC_ASSET_MN_CHARACTERS, llMNCharactersPurinStorySprite, 0x219a8u) \
    X(NDS_RELOC_ASSET_MN_CHARACTERS, llMNCharactersNessStorySprite, 0x23e28u) \
    X(NDS_RELOC_ASSET_MN_CHARACTERS, llMNCharactersWorksWallpaperSprite, 0x25058u) \
    X(NDS_RELOC_ASSET_MN_CHARACTERS, llMNCharactersMarioWorksSprite, 0x25ab8u) \
    X(NDS_RELOC_ASSET_MN_CHARACTERS, llMNCharactersFoxWorksSprite, 0x26518u) \
    X(NDS_RELOC_ASSET_MN_CHARACTERS, llMNCharactersDonkeyWorksSprite, 0x26f78u) \
    X(NDS_RELOC_ASSET_MN_CHARACTERS, llMNCharactersSamusWorksSprite, 0x279d8u) \
    X(NDS_RELOC_ASSET_MN_CHARACTERS, llMNCharactersLuigiWorksSprite, 0x28438u) \
    X(NDS_RELOC_ASSET_MN_CHARACTERS, llMNCharactersLinkWorksSprite, 0x28e98u) \
    X(NDS_RELOC_ASSET_MN_CHARACTERS, llMNCharactersYoshiWorksSprite, 0x298f8u) \
    X(NDS_RELOC_ASSET_MN_CHARACTERS, llMNCharactersCaptainWorksSprite, 0x2a358u) \
    X(NDS_RELOC_ASSET_MN_CHARACTERS, llMNCharactersKirbyWorksSprite, 0x2adb8u) \
    X(NDS_RELOC_ASSET_MN_CHARACTERS, llMNCharactersPikachuWorksSprite, 0x2b818u) \
    X(NDS_RELOC_ASSET_MN_CHARACTERS, llMNCharactersPurinWorksSprite, 0x2c278u) \
    X(NDS_RELOC_ASSET_MN_CHARACTERS, llMNCharactersNessWorksSprite, 0x2ccd8u) \
    X(NDS_RELOC_ASSET_MN_CHARACTERS, llMNCharactersMotionSpecialHiInputSprite, 0x2cda8u) \
    X(NDS_RELOC_ASSET_MN_CHARACTERS, llMNCharactersMotionSpecialNInputSprite, 0x2ce78u) \
    X(NDS_RELOC_ASSET_MN_CHARACTERS, llMNCharactersMotionSpecialLwInputSprite, 0x2cf48u) \
    X(NDS_RELOC_ASSET_MN_CHARACTERS, llMNCharactersMarioSpecialHiNameSprite, 0x2d088u) \
    X(NDS_RELOC_ASSET_MN_CHARACTERS, llMNCharactersFoxSpecialHiNameSprite, 0x2d1c8u) \
    X(NDS_RELOC_ASSET_MN_CHARACTERS, llMNCharactersDonkeySpecialHiNameSprite, 0x2d308u) \
    X(NDS_RELOC_ASSET_MN_CHARACTERS, llMNCharactersSamusSpecialHiNameSprite, 0x2d448u) \
    X(NDS_RELOC_ASSET_MN_CHARACTERS, llMNCharactersLinkSpecialHiNameSprite, 0x2d588u) \
    X(NDS_RELOC_ASSET_MN_CHARACTERS, llMNCharactersYoshiSpecialHiNameSprite, 0x2d6c8u) \
    X(NDS_RELOC_ASSET_MN_CHARACTERS, llMNCharactersCaptainSpecialHiNameSprite, 0x2d808u) \
    X(NDS_RELOC_ASSET_MN_CHARACTERS, llMNCharactersKirbySpecialHiNameSprite, 0x2d948u) \
    X(NDS_RELOC_ASSET_MN_CHARACTERS, llMNCharactersPikachuSpecialHiNameSprite, 0x2da88u) \
    X(NDS_RELOC_ASSET_MN_CHARACTERS, llMNCharactersPurinSpecialHiNameSprite, 0x2dbc8u) \
    X(NDS_RELOC_ASSET_MN_CHARACTERS, llMNCharactersNessSpecialHiNameSprite, 0x2dd08u) \
    X(NDS_RELOC_ASSET_MN_CHARACTERS, llMNCharactersMarioSpecialNNameSprite, 0x2de48u) \
    X(NDS_RELOC_ASSET_MN_CHARACTERS, llMNCharactersFoxSpecialNNameSprite, 0x2df88u) \
    X(NDS_RELOC_ASSET_MN_CHARACTERS, llMNCharactersDonkeySpecialNNameSprite, 0x2e0c8u) \
    X(NDS_RELOC_ASSET_MN_CHARACTERS, llMNCharactersSamusSpecialNNameSprite, 0x2e208u) \
    X(NDS_RELOC_ASSET_MN_CHARACTERS, llMNCharactersLinkSpecialNNameSprite, 0x2e348u) \
    X(NDS_RELOC_ASSET_MN_CHARACTERS, llMNCharactersYoshiSpecialNNameSprite, 0x2e488u) \
    X(NDS_RELOC_ASSET_MN_CHARACTERS, llMNCharactersCaptainSpecialNNameSprite, 0x2e5c8u) \
    X(NDS_RELOC_ASSET_MN_CHARACTERS, llMNCharactersKirbySpecialNNameSprite, 0x2e740u) \
    X(NDS_RELOC_ASSET_MN_CHARACTERS, llMNCharactersPikachuSpecialNNameSprite, 0x2e888u) \
    X(NDS_RELOC_ASSET_MN_CHARACTERS, llMNCharactersPurinSpecialNNameSprite, 0x2e9c8u) \
    X(NDS_RELOC_ASSET_MN_CHARACTERS, llMNCharactersNessSpecialNNameSprite, 0x2eb08u) \
    X(NDS_RELOC_ASSET_MN_CHARACTERS, llMNCharactersMarioSpecialLwNameSprite, 0x2ec48u) \
    X(NDS_RELOC_ASSET_MN_CHARACTERS, llMNCharactersFoxSpecialLwNameSprite, 0x2ed88u) \
    X(NDS_RELOC_ASSET_MN_CHARACTERS, llMNCharactersDonkeySpecialLwNameSprite, 0x2eec8u) \
    X(NDS_RELOC_ASSET_MN_CHARACTERS, llMNCharactersSamusSpecialLwNameSprite, 0x2f008u) \
    X(NDS_RELOC_ASSET_MN_CHARACTERS, llMNCharactersLuigiSpecialLwNameSprite, 0x2f148u) \
    X(NDS_RELOC_ASSET_MN_CHARACTERS, llMNCharactersLinkSpecialLwNameSprite, 0x2f288u) \
    X(NDS_RELOC_ASSET_MN_CHARACTERS, llMNCharactersYoshiSpecialLwNameSprite, 0x2f3c8u) \
    X(NDS_RELOC_ASSET_MN_CHARACTERS, llMNCharactersCaptainSpecialLwNameSprite, 0x2f508u) \
    X(NDS_RELOC_ASSET_MN_CHARACTERS, llMNCharactersKirbySpecialLwNameSprite, 0x2f648u) \
    X(NDS_RELOC_ASSET_MN_CHARACTERS, llMNCharactersPikachuSpecialLwNameSprite, 0x2f788u) \
    X(NDS_RELOC_ASSET_MN_CHARACTERS, llMNCharactersPurinSpecialLwNameSprite, 0x2f8c8u) \
    X(NDS_RELOC_ASSET_MN_CHARACTERS, llMNCharactersNessSpecialLwNameSprite, 0x2fa08u) \
    X(NDS_RELOC_ASSET_MN_CHARACTERS, llMNCharactersStoryWallpaperSprite, 0x30888u)

#define NDS_DECLARE_MN_CHARACTERS_RELOC_SYMBOL(asset, name, value) extern uintptr_t name;
NDS_MN_CHARACTERS_RELOC_SYMBOLS(NDS_DECLARE_MN_CHARACTERS_RELOC_SYMBOL)
#undef NDS_DECLARE_MN_CHARACTERS_RELOC_SYMBOL

/* MNVSRecordMain (reloc file 0x1f, reloc_menus): staged by scripts/menus/stage_reloc_file.py. */
extern uintptr_t llMNVSRecordMainFileID;

#define NDS_MNVS_RECORD_MAIN_RELOC_SYMBOLS(X) \
    X(NDS_RELOC_ASSET_MNVS_RECORD_MAIN, llMNVSRecordMainQuestionSprite, 0x0070u) \
    X(NDS_RELOC_ASSET_MNVS_RECORD_MAIN, llMNVSRecordMainUnknownSprite, 0x0168u) \
    X(NDS_RELOC_ASSET_MNVS_RECORD_MAIN, llMNVSRecordMainLabelTotalSprite, 0x0258u) \
    X(NDS_RELOC_ASSET_MNVS_RECORD_MAIN, llMNVSRecordMainDigit0Sprite, 0x02f0u) \
    X(NDS_RELOC_ASSET_MNVS_RECORD_MAIN, llMNVSRecordMainDigit1Sprite, 0x0390u) \
    X(NDS_RELOC_ASSET_MNVS_RECORD_MAIN, llMNVSRecordMainDigit2Sprite, 0x0430u) \
    X(NDS_RELOC_ASSET_MNVS_RECORD_MAIN, llMNVSRecordMainDigit3Sprite, 0x04d0u) \
    X(NDS_RELOC_ASSET_MNVS_RECORD_MAIN, llMNVSRecordMainDigit4Sprite, 0x0570u) \
    X(NDS_RELOC_ASSET_MNVS_RECORD_MAIN, llMNVSRecordMainDigit5Sprite, 0x0610u) \
    X(NDS_RELOC_ASSET_MNVS_RECORD_MAIN, llMNVSRecordMainDigit6Sprite, 0x06b0u) \
    X(NDS_RELOC_ASSET_MNVS_RECORD_MAIN, llMNVSRecordMainDigit7Sprite, 0x0750u) \
    X(NDS_RELOC_ASSET_MNVS_RECORD_MAIN, llMNVSRecordMainDigit8Sprite, 0x07f0u) \
    X(NDS_RELOC_ASSET_MNVS_RECORD_MAIN, llMNVSRecordMainDigit9Sprite, 0x0890u) \
    X(NDS_RELOC_ASSET_MNVS_RECORD_MAIN, llMNVSRecordMainSymbolPointSprite, 0x0910u) \
    X(NDS_RELOC_ASSET_MNVS_RECORD_MAIN, llMNVSRecordMainLabelWinPercentSprite, 0x0a08u) \
    X(NDS_RELOC_ASSET_MNVS_RECORD_MAIN, llMNVSRecordMainLabelKOsSprite, 0x0af8u) \
    X(NDS_RELOC_ASSET_MNVS_RECORD_MAIN, llMNVSRecordMainLabelTKOSprite, 0x0be8u) \
    X(NDS_RELOC_ASSET_MNVS_RECORD_MAIN, llMNVSRecordMainLabelSDPercentSprite, 0x0cd8u) \
    X(NDS_RELOC_ASSET_MNVS_RECORD_MAIN, llMNVSRecordMainLabelTimeSprite, 0x0e10u) \
    X(NDS_RELOC_ASSET_MNVS_RECORD_MAIN, llMNVSRecordMainLabelUsePercentSprite, 0x0f08u) \
    X(NDS_RELOC_ASSET_MNVS_RECORD_MAIN, llMNVSRecordMainLabelAvgSprite, 0x1008u) \
    X(NDS_RELOC_ASSET_MNVS_RECORD_MAIN, llMNVSRecordMainLabelKOdSprite, 0x1140u) \
    X(NDS_RELOC_ASSET_MNVS_RECORD_MAIN, llMNVSRecordMainSymbolSlashSprite, 0x11d0u) \
    X(NDS_RELOC_ASSET_MNVS_RECORD_MAIN, llMNVSRecordMainUnknown1Sprite, 0x1318u) \
    X(NDS_RELOC_ASSET_MNVS_RECORD_MAIN, llMNVSRecordMainUnknown2Sprite, 0x1458u) \
    X(NDS_RELOC_ASSET_MNVS_RECORD_MAIN, llMNVSRecordMainBattleScoreSprite, 0x15d0u) \
    X(NDS_RELOC_ASSET_MNVS_RECORD_MAIN, llMNVSRecordMainDownArrowsSprite, 0x1668u) \
    X(NDS_RELOC_ASSET_MNVS_RECORD_MAIN, llMNVSRecordMainSideArrowsSprite, 0x17a8u) \
    X(NDS_RELOC_ASSET_MNVS_RECORD_MAIN, llMNVSRecordMainMarioIconBWSprite, 0x1918u) \
    X(NDS_RELOC_ASSET_MNVS_RECORD_MAIN, llMNVSRecordMainFoxIconBWSprite, 0x1a98u) \
    X(NDS_RELOC_ASSET_MNVS_RECORD_MAIN, llMNVSRecordMainDonkeyIconBWSprite, 0x1ca8u) \
    X(NDS_RELOC_ASSET_MNVS_RECORD_MAIN, llMNVSRecordMainSamusIconBWSprite, 0x1e88u) \
    X(NDS_RELOC_ASSET_MNVS_RECORD_MAIN, llMNVSRecordMainLuigiIconBWSprite, 0x2008u) \
    X(NDS_RELOC_ASSET_MNVS_RECORD_MAIN, llMNVSRecordMainYoshiIconBWSprite, 0x2178u) \
    X(NDS_RELOC_ASSET_MNVS_RECORD_MAIN, llMNVSRecordMainLinkIconBWSprite, 0x2370u) \
    X(NDS_RELOC_ASSET_MNVS_RECORD_MAIN, llMNVSRecordMainCaptainIconBWSprite, 0x2540u) \
    X(NDS_RELOC_ASSET_MNVS_RECORD_MAIN, llMNVSRecordMainNessIconBWSprite, 0x2698u) \
    X(NDS_RELOC_ASSET_MNVS_RECORD_MAIN, llMNVSRecordMainPurinIconBWSprite, 0x27c8u) \
    X(NDS_RELOC_ASSET_MNVS_RECORD_MAIN, llMNVSRecordMainKirbyIconBWSprite, 0x2930u) \
    X(NDS_RELOC_ASSET_MNVS_RECORD_MAIN, llMNVSRecordMainPikachuIconBWSprite, 0x2b30u) \
    X(NDS_RELOC_ASSET_MNVS_RECORD_MAIN, llMNVSRecordMainMarioIconColorSprite, 0x2d18u) \
    X(NDS_RELOC_ASSET_MNVS_RECORD_MAIN, llMNVSRecordMainFoxIconColorSprite, 0x2ef8u) \
    X(NDS_RELOC_ASSET_MNVS_RECORD_MAIN, llMNVSRecordMainDonkeyIconColorSprite, 0x3198u) \
    X(NDS_RELOC_ASSET_MNVS_RECORD_MAIN, llMNVSRecordMainSamusIconColorSprite, 0x3438u) \
    X(NDS_RELOC_ASSET_MNVS_RECORD_MAIN, llMNVSRecordMainLuigiIconColorSprite, 0x3618u) \
    X(NDS_RELOC_ASSET_MNVS_RECORD_MAIN, llMNVSRecordMainYoshiIconColorSprite, 0x37f8u) \
    X(NDS_RELOC_ASSET_MNVS_RECORD_MAIN, llMNVSRecordMainLinkIconColorSprite, 0x3a38u) \
    X(NDS_RELOC_ASSET_MNVS_RECORD_MAIN, llMNVSRecordMainCaptainIconColorSprite, 0x3cd8u) \
    X(NDS_RELOC_ASSET_MNVS_RECORD_MAIN, llMNVSRecordMainNessIconColorSprite, 0x3eb8u) \
    X(NDS_RELOC_ASSET_MNVS_RECORD_MAIN, llMNVSRecordMainPurinIconColorSprite, 0x4098u) \
    X(NDS_RELOC_ASSET_MNVS_RECORD_MAIN, llMNVSRecordMainKirbyIconColorSprite, 0x4308u) \
    X(NDS_RELOC_ASSET_MNVS_RECORD_MAIN, llMNVSRecordMainPikachuIconColorSprite, 0x45a8u) \
    X(NDS_RELOC_ASSET_MNVS_RECORD_MAIN, llMNVSRecordMainPortraitWallpaperSprite, 0x4d30u) \
    X(NDS_RELOC_ASSET_MNVS_RECORD_MAIN, llMNVSRecordMainLabelSprite, 0x5428u) \
    X(NDS_RELOC_ASSET_MNVS_RECORD_MAIN, llMNVSRecordMainSymbolColonSprite, 0x54c0u)

#define NDS_DECLARE_MNVS_RECORD_MAIN_RELOC_SYMBOL(asset, name, value) extern uintptr_t name;
NDS_MNVS_RECORD_MAIN_RELOC_SYMBOLS(NDS_DECLARE_MNVS_RECORD_MAIN_RELOC_SYMBOL)
#undef NDS_DECLARE_MNVS_RECORD_MAIN_RELOC_SYMBOL

/* MNDataCommon (reloc file 0x20, reloc_menus): staged by scripts/menus/stage_reloc_file.py. */
extern uintptr_t llMNDataCommonFileID;

#define NDS_MN_DATA_COMMON_RELOC_SYMBOLS(X) \
    X(NDS_RELOC_ASSET_MN_DATA_COMMON, llMNDataCommonDataHeaderSprite, 0x0b40u) \
    X(NDS_RELOC_ASSET_MN_DATA_COMMON, llMNDataCommonArrowLSprite, 0x0be0u) \
    X(NDS_RELOC_ASSET_MN_DATA_COMMON, llMNDataCommonArrowRSprite, 0x0c80u)

#define NDS_DECLARE_MN_DATA_COMMON_RELOC_SYMBOL(asset, name, value) extern uintptr_t name;
NDS_MN_DATA_COMMON_RELOC_SYMBOLS(NDS_DECLARE_MN_DATA_COMMON_RELOC_SYMBOL)
#undef NDS_DECLARE_MN_DATA_COMMON_RELOC_SYMBOL

/* MNData (reloc file 0x5, reloc_menus): staged by scripts/menus/stage_reloc_file.py. */
extern uintptr_t llMNDataFileID;

#define NDS_MN_DATA_RELOC_SYMBOLS(X) \
    X(NDS_RELOC_ASSET_MN_DATA, llMNDataCharactersTextJapSprite, 0x06a8u) \
    X(NDS_RELOC_ASSET_MN_DATA, llMNDataVSRecordTextJapSprite, 0x0ac8u) \
    X(NDS_RELOC_ASSET_MN_DATA, llMNDataSoundTestTextJapSprite, 0x10c8u) \
    X(NDS_RELOC_ASSET_MN_DATA, llMNDataCharactersTextSprite, 0x14e0u) \
    X(NDS_RELOC_ASSET_MN_DATA, llMNDataVSRecordTextSprite, 0x1900u) \
    X(NDS_RELOC_ASSET_MN_DATA, llMNDataSoundTestTextSprite, 0x1d20u) \
    X(NDS_RELOC_ASSET_MN_DATA, llMNDataDataTextSprite, 0x23a8u) \
    X(NDS_RELOC_ASSET_MN_DATA, llMNDataDataIconDarkSprite, 0x4a78u)

#define NDS_DECLARE_MN_DATA_RELOC_SYMBOL(asset, name, value) extern uintptr_t name;
NDS_MN_DATA_RELOC_SYMBOLS(NDS_DECLARE_MN_DATA_RELOC_SYMBOL)
#undef NDS_DECLARE_MN_DATA_RELOC_SYMBOL

/* MNCongraYoshiTop (reloc file 0xad, reloc_menus): staged by scripts/menus/stage_reloc_file.py. */
extern uintptr_t llMNCongraYoshiTopFileID;

#define NDS_MN_CONGRA_YOSHI_TOP_RELOC_SYMBOLS(X) \
    X(NDS_RELOC_ASSET_MN_CONGRA_YOSHI_TOP, llMNCongraYoshiTopSprite, 0x20718u)

#define NDS_DECLARE_MN_CONGRA_YOSHI_TOP_RELOC_SYMBOL(asset, name, value) extern uintptr_t name;
NDS_MN_CONGRA_YOSHI_TOP_RELOC_SYMBOLS(NDS_DECLARE_MN_CONGRA_YOSHI_TOP_RELOC_SYMBOL)
#undef NDS_DECLARE_MN_CONGRA_YOSHI_TOP_RELOC_SYMBOL

/* MNCongraYoshiBottom (reloc file 0xac, reloc_menus): staged by scripts/menus/stage_reloc_file.py. */
extern uintptr_t llMNCongraYoshiBottomFileID;

#define NDS_MN_CONGRA_YOSHI_BOTTOM_RELOC_SYMBOLS(X) \
    X(NDS_RELOC_ASSET_MN_CONGRA_YOSHI_BOTTOM, llMNCongraYoshiBottomSprite, 0x20718u)

#define NDS_DECLARE_MN_CONGRA_YOSHI_BOTTOM_RELOC_SYMBOL(asset, name, value) extern uintptr_t name;
NDS_MN_CONGRA_YOSHI_BOTTOM_RELOC_SYMBOLS(NDS_DECLARE_MN_CONGRA_YOSHI_BOTTOM_RELOC_SYMBOL)
#undef NDS_DECLARE_MN_CONGRA_YOSHI_BOTTOM_RELOC_SYMBOL

/* MNCongraSamusTop (reloc file 0xb1, reloc_menus): staged by scripts/menus/stage_reloc_file.py. */
extern uintptr_t llMNCongraSamusTopFileID;

#define NDS_MN_CONGRA_SAMUS_TOP_RELOC_SYMBOLS(X) \
    X(NDS_RELOC_ASSET_MN_CONGRA_SAMUS_TOP, llMNCongraSamusTopSprite, 0x20718u)

#define NDS_DECLARE_MN_CONGRA_SAMUS_TOP_RELOC_SYMBOL(asset, name, value) extern uintptr_t name;
NDS_MN_CONGRA_SAMUS_TOP_RELOC_SYMBOLS(NDS_DECLARE_MN_CONGRA_SAMUS_TOP_RELOC_SYMBOL)
#undef NDS_DECLARE_MN_CONGRA_SAMUS_TOP_RELOC_SYMBOL

/* MNCongraSamusBottom (reloc file 0xb0, reloc_menus): staged by scripts/menus/stage_reloc_file.py. */
extern uintptr_t llMNCongraSamusBottomFileID;

#define NDS_MN_CONGRA_SAMUS_BOTTOM_RELOC_SYMBOLS(X) \
    X(NDS_RELOC_ASSET_MN_CONGRA_SAMUS_BOTTOM, llMNCongraSamusBottomSprite, 0x20718u)

#define NDS_DECLARE_MN_CONGRA_SAMUS_BOTTOM_RELOC_SYMBOL(asset, name, value) extern uintptr_t name;
NDS_MN_CONGRA_SAMUS_BOTTOM_RELOC_SYMBOLS(NDS_DECLARE_MN_CONGRA_SAMUS_BOTTOM_RELOC_SYMBOL)
#undef NDS_DECLARE_MN_CONGRA_SAMUS_BOTTOM_RELOC_SYMBOL

/* MNCongraPurinTop (reloc file 0xb5, reloc_menus): staged by scripts/menus/stage_reloc_file.py. */
extern uintptr_t llMNCongraPurinTopFileID;

#define NDS_MN_CONGRA_PURIN_TOP_RELOC_SYMBOLS(X) \
    X(NDS_RELOC_ASSET_MN_CONGRA_PURIN_TOP, llMNCongraPurinTopSprite, 0x20718u)

#define NDS_DECLARE_MN_CONGRA_PURIN_TOP_RELOC_SYMBOL(asset, name, value) extern uintptr_t name;
NDS_MN_CONGRA_PURIN_TOP_RELOC_SYMBOLS(NDS_DECLARE_MN_CONGRA_PURIN_TOP_RELOC_SYMBOL)
#undef NDS_DECLARE_MN_CONGRA_PURIN_TOP_RELOC_SYMBOL

/* MNCongraPurinBottom (reloc file 0xb4, reloc_menus): staged by scripts/menus/stage_reloc_file.py. */
extern uintptr_t llMNCongraPurinBottomFileID;

#define NDS_MN_CONGRA_PURIN_BOTTOM_RELOC_SYMBOLS(X) \
    X(NDS_RELOC_ASSET_MN_CONGRA_PURIN_BOTTOM, llMNCongraPurinBottomSprite, 0x20718u)

#define NDS_DECLARE_MN_CONGRA_PURIN_BOTTOM_RELOC_SYMBOL(asset, name, value) extern uintptr_t name;
NDS_MN_CONGRA_PURIN_BOTTOM_RELOC_SYMBOLS(NDS_DECLARE_MN_CONGRA_PURIN_BOTTOM_RELOC_SYMBOL)
#undef NDS_DECLARE_MN_CONGRA_PURIN_BOTTOM_RELOC_SYMBOL

/* MNCongraPikachuTop (reloc file 0xaf, reloc_menus): staged by scripts/menus/stage_reloc_file.py. */
extern uintptr_t llMNCongraPikachuTopFileID;

#define NDS_MN_CONGRA_PIKACHU_TOP_RELOC_SYMBOLS(X) \
    X(NDS_RELOC_ASSET_MN_CONGRA_PIKACHU_TOP, llMNCongraPikachuTopSprite, 0x20718u)

#define NDS_DECLARE_MN_CONGRA_PIKACHU_TOP_RELOC_SYMBOL(asset, name, value) extern uintptr_t name;
NDS_MN_CONGRA_PIKACHU_TOP_RELOC_SYMBOLS(NDS_DECLARE_MN_CONGRA_PIKACHU_TOP_RELOC_SYMBOL)
#undef NDS_DECLARE_MN_CONGRA_PIKACHU_TOP_RELOC_SYMBOL

/* MNCongraPikachuBottom (reloc file 0xae, reloc_menus): staged by scripts/menus/stage_reloc_file.py. */
extern uintptr_t llMNCongraPikachuBottomFileID;

#define NDS_MN_CONGRA_PIKACHU_BOTTOM_RELOC_SYMBOLS(X) \
    X(NDS_RELOC_ASSET_MN_CONGRA_PIKACHU_BOTTOM, llMNCongraPikachuBottomSprite, 0x20718u)

#define NDS_DECLARE_MN_CONGRA_PIKACHU_BOTTOM_RELOC_SYMBOL(asset, name, value) extern uintptr_t name;
NDS_MN_CONGRA_PIKACHU_BOTTOM_RELOC_SYMBOLS(NDS_DECLARE_MN_CONGRA_PIKACHU_BOTTOM_RELOC_SYMBOL)
#undef NDS_DECLARE_MN_CONGRA_PIKACHU_BOTTOM_RELOC_SYMBOL

/* MNCongraNessTop (reloc file 0xc1, reloc_menus): staged by scripts/menus/stage_reloc_file.py. */
extern uintptr_t llMNCongraNessTopFileID;

#define NDS_MN_CONGRA_NESS_TOP_RELOC_SYMBOLS(X) \
    X(NDS_RELOC_ASSET_MN_CONGRA_NESS_TOP, llMNCongraNessTopSprite, 0x20718u)

#define NDS_DECLARE_MN_CONGRA_NESS_TOP_RELOC_SYMBOL(asset, name, value) extern uintptr_t name;
NDS_MN_CONGRA_NESS_TOP_RELOC_SYMBOLS(NDS_DECLARE_MN_CONGRA_NESS_TOP_RELOC_SYMBOL)
#undef NDS_DECLARE_MN_CONGRA_NESS_TOP_RELOC_SYMBOL

/* MNCongraNessBottom (reloc file 0xc0, reloc_menus): staged by scripts/menus/stage_reloc_file.py. */
extern uintptr_t llMNCongraNessBottomFileID;

#define NDS_MN_CONGRA_NESS_BOTTOM_RELOC_SYMBOLS(X) \
    X(NDS_RELOC_ASSET_MN_CONGRA_NESS_BOTTOM, llMNCongraNessBottomSprite, 0x20718u)

#define NDS_DECLARE_MN_CONGRA_NESS_BOTTOM_RELOC_SYMBOL(asset, name, value) extern uintptr_t name;
NDS_MN_CONGRA_NESS_BOTTOM_RELOC_SYMBOLS(NDS_DECLARE_MN_CONGRA_NESS_BOTTOM_RELOC_SYMBOL)
#undef NDS_DECLARE_MN_CONGRA_NESS_BOTTOM_RELOC_SYMBOL

/* MNCongraMarioTop (reloc file 0xbb, reloc_menus): staged by scripts/menus/stage_reloc_file.py. */
extern uintptr_t llMNCongraMarioTopFileID;

#define NDS_MN_CONGRA_MARIO_TOP_RELOC_SYMBOLS(X) \
    X(NDS_RELOC_ASSET_MN_CONGRA_MARIO_TOP, llMNCongraMarioTopSprite, 0x20718u)

#define NDS_DECLARE_MN_CONGRA_MARIO_TOP_RELOC_SYMBOL(asset, name, value) extern uintptr_t name;
NDS_MN_CONGRA_MARIO_TOP_RELOC_SYMBOLS(NDS_DECLARE_MN_CONGRA_MARIO_TOP_RELOC_SYMBOL)
#undef NDS_DECLARE_MN_CONGRA_MARIO_TOP_RELOC_SYMBOL

/* MNCongraMarioBottom (reloc file 0xba, reloc_menus): staged by scripts/menus/stage_reloc_file.py. */
extern uintptr_t llMNCongraMarioBottomFileID;

#define NDS_MN_CONGRA_MARIO_BOTTOM_RELOC_SYMBOLS(X) \
    X(NDS_RELOC_ASSET_MN_CONGRA_MARIO_BOTTOM, llMNCongraMarioBottomSprite, 0x20718u)

#define NDS_DECLARE_MN_CONGRA_MARIO_BOTTOM_RELOC_SYMBOL(asset, name, value) extern uintptr_t name;
NDS_MN_CONGRA_MARIO_BOTTOM_RELOC_SYMBOLS(NDS_DECLARE_MN_CONGRA_MARIO_BOTTOM_RELOC_SYMBOL)
#undef NDS_DECLARE_MN_CONGRA_MARIO_BOTTOM_RELOC_SYMBOL

/* MNCongraLuigiTop (reloc file 0xbd, reloc_menus): staged by scripts/menus/stage_reloc_file.py. */
extern uintptr_t llMNCongraLuigiTopFileID;

#define NDS_MN_CONGRA_LUIGI_TOP_RELOC_SYMBOLS(X) \
    X(NDS_RELOC_ASSET_MN_CONGRA_LUIGI_TOP, llMNCongraLuigiTopSprite, 0x20718u)

#define NDS_DECLARE_MN_CONGRA_LUIGI_TOP_RELOC_SYMBOL(asset, name, value) extern uintptr_t name;
NDS_MN_CONGRA_LUIGI_TOP_RELOC_SYMBOLS(NDS_DECLARE_MN_CONGRA_LUIGI_TOP_RELOC_SYMBOL)
#undef NDS_DECLARE_MN_CONGRA_LUIGI_TOP_RELOC_SYMBOL

/* MNCongraLuigiBottom (reloc file 0xbc, reloc_menus): staged by scripts/menus/stage_reloc_file.py. */
extern uintptr_t llMNCongraLuigiBottomFileID;

#define NDS_MN_CONGRA_LUIGI_BOTTOM_RELOC_SYMBOLS(X) \
    X(NDS_RELOC_ASSET_MN_CONGRA_LUIGI_BOTTOM, llMNCongraLuigiBottomSprite, 0x20718u)

#define NDS_DECLARE_MN_CONGRA_LUIGI_BOTTOM_RELOC_SYMBOL(asset, name, value) extern uintptr_t name;
NDS_MN_CONGRA_LUIGI_BOTTOM_RELOC_SYMBOLS(NDS_DECLARE_MN_CONGRA_LUIGI_BOTTOM_RELOC_SYMBOL)
#undef NDS_DECLARE_MN_CONGRA_LUIGI_BOTTOM_RELOC_SYMBOL

/* MNCongraLinkTop (reloc file 0xb3, reloc_menus): staged by scripts/menus/stage_reloc_file.py. */
extern uintptr_t llMNCongraLinkTopFileID;

#define NDS_MN_CONGRA_LINK_TOP_RELOC_SYMBOLS(X) \
    X(NDS_RELOC_ASSET_MN_CONGRA_LINK_TOP, llMNCongraLinkTopSprite, 0x20718u)

#define NDS_DECLARE_MN_CONGRA_LINK_TOP_RELOC_SYMBOL(asset, name, value) extern uintptr_t name;
NDS_MN_CONGRA_LINK_TOP_RELOC_SYMBOLS(NDS_DECLARE_MN_CONGRA_LINK_TOP_RELOC_SYMBOL)
#undef NDS_DECLARE_MN_CONGRA_LINK_TOP_RELOC_SYMBOL

/* MNCongraLinkBottom (reloc file 0xb2, reloc_menus): staged by scripts/menus/stage_reloc_file.py. */
extern uintptr_t llMNCongraLinkBottomFileID;

#define NDS_MN_CONGRA_LINK_BOTTOM_RELOC_SYMBOLS(X) \
    X(NDS_RELOC_ASSET_MN_CONGRA_LINK_BOTTOM, llMNCongraLinkBottomSprite, 0x20718u)

#define NDS_DECLARE_MN_CONGRA_LINK_BOTTOM_RELOC_SYMBOL(asset, name, value) extern uintptr_t name;
NDS_MN_CONGRA_LINK_BOTTOM_RELOC_SYMBOLS(NDS_DECLARE_MN_CONGRA_LINK_BOTTOM_RELOC_SYMBOL)
#undef NDS_DECLARE_MN_CONGRA_LINK_BOTTOM_RELOC_SYMBOL

/* MNCongraKirbyTop (reloc file 0xab, reloc_menus): staged by scripts/menus/stage_reloc_file.py. */
extern uintptr_t llMNCongraKirbyTopFileID;

#define NDS_MN_CONGRA_KIRBY_TOP_RELOC_SYMBOLS(X) \
    X(NDS_RELOC_ASSET_MN_CONGRA_KIRBY_TOP, llMNCongraKirbyTopSprite, 0x20718u)

#define NDS_DECLARE_MN_CONGRA_KIRBY_TOP_RELOC_SYMBOL(asset, name, value) extern uintptr_t name;
NDS_MN_CONGRA_KIRBY_TOP_RELOC_SYMBOLS(NDS_DECLARE_MN_CONGRA_KIRBY_TOP_RELOC_SYMBOL)
#undef NDS_DECLARE_MN_CONGRA_KIRBY_TOP_RELOC_SYMBOL

/* MNCongraKirbyBottom (reloc file 0xaa, reloc_menus): staged by scripts/menus/stage_reloc_file.py. */
extern uintptr_t llMNCongraKirbyBottomFileID;

#define NDS_MN_CONGRA_KIRBY_BOTTOM_RELOC_SYMBOLS(X) \
    X(NDS_RELOC_ASSET_MN_CONGRA_KIRBY_BOTTOM, llMNCongraKirbyBottomSprite, 0x20718u)

#define NDS_DECLARE_MN_CONGRA_KIRBY_BOTTOM_RELOC_SYMBOL(asset, name, value) extern uintptr_t name;
NDS_MN_CONGRA_KIRBY_BOTTOM_RELOC_SYMBOLS(NDS_DECLARE_MN_CONGRA_KIRBY_BOTTOM_RELOC_SYMBOL)
#undef NDS_DECLARE_MN_CONGRA_KIRBY_BOTTOM_RELOC_SYMBOL

/* MNCongraFoxTop (reloc file 0xbf, reloc_menus): staged by scripts/menus/stage_reloc_file.py. */
extern uintptr_t llMNCongraFoxTopFileID;

#define NDS_MN_CONGRA_FOX_TOP_RELOC_SYMBOLS(X) \
    X(NDS_RELOC_ASSET_MN_CONGRA_FOX_TOP, llMNCongraFoxTopSprite, 0x20718u)

#define NDS_DECLARE_MN_CONGRA_FOX_TOP_RELOC_SYMBOL(asset, name, value) extern uintptr_t name;
NDS_MN_CONGRA_FOX_TOP_RELOC_SYMBOLS(NDS_DECLARE_MN_CONGRA_FOX_TOP_RELOC_SYMBOL)
#undef NDS_DECLARE_MN_CONGRA_FOX_TOP_RELOC_SYMBOL

/* MNCongraFoxBottom (reloc file 0xbe, reloc_menus): staged by scripts/menus/stage_reloc_file.py. */
extern uintptr_t llMNCongraFoxBottomFileID;

#define NDS_MN_CONGRA_FOX_BOTTOM_RELOC_SYMBOLS(X) \
    X(NDS_RELOC_ASSET_MN_CONGRA_FOX_BOTTOM, llMNCongraFoxBottomSprite, 0x20718u)

#define NDS_DECLARE_MN_CONGRA_FOX_BOTTOM_RELOC_SYMBOL(asset, name, value) extern uintptr_t name;
NDS_MN_CONGRA_FOX_BOTTOM_RELOC_SYMBOLS(NDS_DECLARE_MN_CONGRA_FOX_BOTTOM_RELOC_SYMBOL)
#undef NDS_DECLARE_MN_CONGRA_FOX_BOTTOM_RELOC_SYMBOL

/* MNCongraDonkeyTop (reloc file 0xb9, reloc_menus): staged by scripts/menus/stage_reloc_file.py. */
extern uintptr_t llMNCongraDonkeyTopFileID;

#define NDS_MN_CONGRA_DONKEY_TOP_RELOC_SYMBOLS(X) \
    X(NDS_RELOC_ASSET_MN_CONGRA_DONKEY_TOP, llMNCongraDonkeyTopSprite, 0x20718u)

#define NDS_DECLARE_MN_CONGRA_DONKEY_TOP_RELOC_SYMBOL(asset, name, value) extern uintptr_t name;
NDS_MN_CONGRA_DONKEY_TOP_RELOC_SYMBOLS(NDS_DECLARE_MN_CONGRA_DONKEY_TOP_RELOC_SYMBOL)
#undef NDS_DECLARE_MN_CONGRA_DONKEY_TOP_RELOC_SYMBOL

/* MNCongraDonkeyBottom (reloc file 0xb8, reloc_menus): staged by scripts/menus/stage_reloc_file.py. */
extern uintptr_t llMNCongraDonkeyBottomFileID;

#define NDS_MN_CONGRA_DONKEY_BOTTOM_RELOC_SYMBOLS(X) \
    X(NDS_RELOC_ASSET_MN_CONGRA_DONKEY_BOTTOM, llMNCongraDonkeyBottomSprite, 0x20718u)

#define NDS_DECLARE_MN_CONGRA_DONKEY_BOTTOM_RELOC_SYMBOL(asset, name, value) extern uintptr_t name;
NDS_MN_CONGRA_DONKEY_BOTTOM_RELOC_SYMBOLS(NDS_DECLARE_MN_CONGRA_DONKEY_BOTTOM_RELOC_SYMBOL)
#undef NDS_DECLARE_MN_CONGRA_DONKEY_BOTTOM_RELOC_SYMBOL

/* MNCongraCaptainTop (reloc file 0xb7, reloc_menus): staged by scripts/menus/stage_reloc_file.py. */
extern uintptr_t llMNCongraCaptainTopFileID;

#define NDS_MN_CONGRA_CAPTAIN_TOP_RELOC_SYMBOLS(X) \
    X(NDS_RELOC_ASSET_MN_CONGRA_CAPTAIN_TOP, llMNCongraCaptainTopSprite, 0x20718u)

#define NDS_DECLARE_MN_CONGRA_CAPTAIN_TOP_RELOC_SYMBOL(asset, name, value) extern uintptr_t name;
NDS_MN_CONGRA_CAPTAIN_TOP_RELOC_SYMBOLS(NDS_DECLARE_MN_CONGRA_CAPTAIN_TOP_RELOC_SYMBOL)
#undef NDS_DECLARE_MN_CONGRA_CAPTAIN_TOP_RELOC_SYMBOL

/* MNCongraCaptainBottom (reloc file 0xb6, reloc_menus): staged by scripts/menus/stage_reloc_file.py. */
extern uintptr_t llMNCongraCaptainBottomFileID;

#define NDS_MN_CONGRA_CAPTAIN_BOTTOM_RELOC_SYMBOLS(X) \
    X(NDS_RELOC_ASSET_MN_CONGRA_CAPTAIN_BOTTOM, llMNCongraCaptainBottomSprite, 0x20718u)

#define NDS_DECLARE_MN_CONGRA_CAPTAIN_BOTTOM_RELOC_SYMBOL(asset, name, value) extern uintptr_t name;
NDS_MN_CONGRA_CAPTAIN_BOTTOM_RELOC_SYMBOLS(NDS_DECLARE_MN_CONGRA_CAPTAIN_BOTTOM_RELOC_SYMBOL)
#undef NDS_DECLARE_MN_CONGRA_CAPTAIN_BOTTOM_RELOC_SYMBOL

/* MVEnding (reloc file 0x4c, reloc_movies): staged by scripts/menus/stage_reloc_file.py. */
extern uintptr_t llMVEndingFileID;

#define NDS_MV_ENDING_RELOC_SYMBOLS(X) \
    X(NDS_RELOC_ASSET_MV_ENDING, llMVEndingOperatorCamAnimJoint, 0x0000u)

#define NDS_DECLARE_MV_ENDING_RELOC_SYMBOL(asset, name, value) extern uintptr_t name;
NDS_MV_ENDING_RELOC_SYMBOLS(NDS_DECLARE_MV_ENDING_RELOC_SYMBOL)
#undef NDS_DECLARE_MV_ENDING_RELOC_SYMBOL

/* SCStaffroll (reloc file 0xc3, reloc_scene): staged by scripts/menus/stage_reloc_file.py. */
extern uintptr_t llSCStaffrollFileID;

#define NDS_SC_STAFFROLL_RELOC_SYMBOLS(X) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollNameAndJobAUpperImage, 0x0008u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollNameAndJobALowerImage, 0x0178u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollNameAndJobBUpperImage, 0x0218u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollNameAndJobBLowerImage, 0x02d8u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollNameAndJobCUpperImage, 0x0398u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollNameAndJobCLowerImage, 0x0458u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollNameAndJobDUpperImage, 0x04f8u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollNameAndJobDLowerImage, 0x0668u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollNameAndJobEUpperImage, 0x0728u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollNameAndJobELowerImage, 0x07e8u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollNameAndJobFUpperImage, 0x0888u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollNameAndJobFLowerImage, 0x0948u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollNameAndJobGUpperImage, 0x0a08u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollNameAndJobGLowerImage, 0x0b78u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollNameAndJobHUpperImage, 0x0c38u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollNameAndJobHLowerImage, 0x0da8u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollNameAndJobIUpperImage, 0x0e68u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollNameAndJobILowerImage, 0x0f28u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollNameAndJobJUpperImage, 0x0fe8u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollNameAndJobJLowerImage, 0x10a8u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollNameAndJobKUpperImage, 0x1188u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollNameAndJobKLowerImage, 0x12f8u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollNameAndJobLUpperImage, 0x13b8u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollNameAndJobLLowerImage, 0x1478u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollNameAndJobMUpperImage, 0x1538u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollNameAndJobMLowerImage, 0x16a8u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollNameAndJobNUpperImage, 0x17d8u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollNameAndJobNLowerImage, 0x1948u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollNameAndJobOUpperImage, 0x19e8u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollNameAndJobOLowerImage, 0x1b58u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollNameAndJobPUpperImage, 0x1c88u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollNameAndJobPLowerImage, 0x1d48u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollNameAndJobQUpperImage, 0x1e08u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollNameAndJobQLowerImage, 0x1f78u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollNameAndJobRUpperImage, 0x2038u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollNameAndJobRLowerImage, 0x20f8u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollNameAndJobSUpperImage, 0x2198u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollNameAndJobSLowerImage, 0x2258u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollNameAndJobTUpperImage, 0x22f8u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollNameAndJobTLowerImage, 0x23b8u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollNameAndJobUUpperImage, 0x2478u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollNameAndJobULowerImage, 0x2538u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollNameAndJobVUpperImage, 0x25d8u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollNameAndJobVLowerImage, 0x2748u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollNameAndJobWUpperImage, 0x27e8u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollNameAndJobWLowerImage, 0x2958u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollNameAndJobXUpperImage, 0x2a88u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollNameAndJobXLowerImage, 0x2bf8u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollNameAndJobYUpperImage, 0x2c98u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollNameAndJobYLowerImage, 0x2d58u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollNameAndJobZUpperImage, 0x2e18u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollNameAndJobZLowerImage, 0x2f88u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollNameAndJobCommaImage, 0x3018u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollNameAndJobPeriodImage, 0x3078u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollNameAndJobApostropheImage, 0x30b8u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollNameAndJob4Image, 0x3118u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollTextBoxAUpperSprite, 0x3258u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollTextBoxALowerSprite, 0x3310u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollTextBoxBUpperSprite, 0x33e8u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollTextBoxBLowerSprite, 0x34b0u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollTextBoxCUpperSprite, 0x3588u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollTextBoxCLowerSprite, 0x3640u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollTextBoxDUpperSprite, 0x3718u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollTextBoxDLowerSprite, 0x37e0u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollTextBoxEUpperSprite, 0x38b8u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollTextBoxELowerSprite, 0x3970u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollTextBoxFUpperSprite, 0x3a48u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollTextBoxFLowerSprite, 0x3b10u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollTextBoxGUpperSprite, 0x3be8u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollTextBoxGLowerSprite, 0x3ca8u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollTextBoxHUpperSprite, 0x3d78u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollTextBoxHLowerSprite, 0x3e40u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollTextBoxIUpperSprite, 0x3f18u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollTextBoxILowerSprite, 0x3fe0u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollTextBoxJUpperSprite, 0x40b8u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollTextBoxJLowerSprite, 0x4188u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollTextBoxKUpperSprite, 0x4258u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollTextBoxKLowerSprite, 0x4320u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollTextBoxLUpperSprite, 0x43f8u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollTextBoxLLowerSprite, 0x44c0u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollTextBoxMUpperSprite, 0x4598u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollTextBoxMLowerSprite, 0x4650u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollTextBoxNUpperSprite, 0x4728u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollTextBoxNLowerSprite, 0x47e0u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollTextBoxOUpperSprite, 0x48b8u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollTextBoxOLowerSprite, 0x4970u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollTextBoxPUpperSprite, 0x4a48u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollTextBoxPLowerSprite, 0x4b08u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollTextBoxQUpperSprite, 0x4bd8u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollTextBoxQLowerSprite, 0x4c98u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollTextBoxRUpperSprite, 0x4d68u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollTextBoxRLowerSprite, 0x4e20u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollTextBoxSUpperSprite, 0x4ef8u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollTextBoxSLowerSprite, 0x4fb0u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollTextBoxTUpperSprite, 0x5088u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollTextBoxTLowerSprite, 0x5150u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollTextBoxUUpperSprite, 0x5228u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollTextBoxULowerSprite, 0x52e0u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollTextBoxVUpperSprite, 0x53b8u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollTextBoxVLowerSprite, 0x5470u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollTextBoxWUpperSprite, 0x5548u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollTextBoxWLowerSprite, 0x5600u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollTextBoxXUpperSprite, 0x56d8u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollTextBoxXLowerSprite, 0x5790u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollTextBoxYUpperSprite, 0x5868u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollTextBoxYLowerSprite, 0x5928u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollTextBoxZUpperSprite, 0x59f8u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollTextBoxZLowerSprite, 0x5ab0u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollTextBoxColonSprite, 0x5b70u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollTextBoxCommaSprite, 0x5c00u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollTextBoxPeriodSprite, 0x5c90u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollTextBoxDashSprite, 0x5d18u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollTextBox1Sprite, 0x5de8u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollTextBox2Sprite, 0x5eb8u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollTextBox3Sprite, 0x5f88u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollTextBox4Sprite, 0x6058u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollTextBox5Sprite, 0x6128u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollTextBox6Sprite, 0x61f8u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollTextBox7Sprite, 0x62c8u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollTextBox8Sprite, 0x6398u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollTextBox9Sprite, 0x6468u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollTextBox0Sprite, 0x6538u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollTextBoxQuoteSprite, 0x65c0u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollTextBoxAmpersSprite, 0x6698u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollTextBoxSlashSprite, 0x6758u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollTextBoxApostropheSprite, 0x67e0u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollTextBoxQuestionSprite, 0x68b8u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollTextBoxBracketOpenSprite, 0x6988u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollTextBoxBracketCloseSprite, 0x6a58u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollTextBoxEAccentSprite, 0x6b20u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollCrosshairSprite, 0x6d58u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollTextBoxBracketLeftSprite, 0x6f98u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollTextBoxBracketRightSprite, 0x71d8u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollInterpolation, 0x7304u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollAnimJoint, 0x7338u) \
    X(NDS_RELOC_ASSET_SC_STAFFROLL, llSCStaffrollDObjDesc, 0x78c0u)

#define NDS_DECLARE_SC_STAFFROLL_RELOC_SYMBOL(asset, name, value) extern uintptr_t name;
NDS_SC_STAFFROLL_RELOC_SYMBOLS(NDS_DECLARE_SC_STAFFROLL_RELOC_SYMBOL)
#undef NDS_DECLARE_SC_STAFFROLL_RELOC_SYMBOL

/* MNMessage (reloc file 0x9, reloc_menus): staged by scripts/menus/stage_reloc_file.py. */
extern uintptr_t llMNMessageFileID;

#define NDS_MN_MESSAGE_RELOC_SYMBOLS(X) \
    X(NDS_RELOC_ASSET_MN_MESSAGE, llMNMessageUnlockLuigiSprite, 0x09e0u) \
    X(NDS_RELOC_ASSET_MN_MESSAGE, llMNMessageUnlockNessSprite, 0x1148u) \
    X(NDS_RELOC_ASSET_MN_MESSAGE, llMNMessageUnlockCaptainSprite, 0x1f50u) \
    X(NDS_RELOC_ASSET_MN_MESSAGE, llMNMessageUnlockPurinSprite, 0x2e58u) \
    X(NDS_RELOC_ASSET_MN_MESSAGE, llMNMessageUnlockInishieSprite, 0x3458u) \
    X(NDS_RELOC_ASSET_MN_MESSAGE, llMNMessageUnlockSoundTestSprite, 0x4180u) \
    X(NDS_RELOC_ASSET_MN_MESSAGE, llMNMessageUnlockItemSwitchSprite, 0x4eb0u) \
    X(NDS_RELOC_ASSET_MN_MESSAGE, llMNMessageDecalExclaimSprite, 0x5300u)

#define NDS_DECLARE_MN_MESSAGE_RELOC_SYMBOL(asset, name, value) extern uintptr_t name;
NDS_MN_MESSAGE_RELOC_SYMBOLS(NDS_DECLARE_MN_MESSAGE_RELOC_SYMBOL)
#undef NDS_DECLARE_MN_MESSAGE_RELOC_SYMBOL

/* MNPlayers1PMode (reloc file 0x17, reloc_menus): staged by scripts/menus/stage_reloc_file.py. */
extern uintptr_t llMNPlayers1PModeFileID;

#define NDS_MN_PLAYERS1P_MODE_RELOC_SYMBOLS(X) \
    X(NDS_RELOC_ASSET_MN_PLAYERS1P_MODE, llMNPlayers1PMode1PlayerGameTextSprite, 0x0228u) \
    X(NDS_RELOC_ASSET_MN_PLAYERS1P_MODE, llMNPlayers1PModeClosingParenthesisSprite, 0x02c8u) \
    X(NDS_RELOC_ASSET_MN_PLAYERS1P_MODE, llMNPlayers1PModeOpeningParenthesisSprite, 0x0368u) \
    X(NDS_RELOC_ASSET_MN_PLAYERS1P_MODE, llMNPlayers1PModeLevelColonTextSprite, 0x0488u) \
    X(NDS_RELOC_ASSET_MN_PLAYERS1P_MODE, llMNPlayers1PModeStockColonTextSprite, 0x05a8u) \
    X(NDS_RELOC_ASSET_MN_PLAYERS1P_MODE, llMNPlayers1PModeOptionOutlineSprite, 0x1208u) \
    X(NDS_RELOC_ASSET_MN_PLAYERS1P_MODE, llMNPlayers1PModeBestTimeTextSprite, 0x12e0u) \
    X(NDS_RELOC_ASSET_MN_PLAYERS1P_MODE, llMNPlayers1PModeTotalBestTimeTextSprite, 0x1410u) \
    X(NDS_RELOC_ASSET_MN_PLAYERS1P_MODE, llMNPlayers1PModeTargetsTextSprite, 0x1658u) \
    X(NDS_RELOC_ASSET_MN_PLAYERS1P_MODE, llMNPlayers1PModePlatformsTextSprite, 0x1898u) \
    X(NDS_RELOC_ASSET_MN_PLAYERS1P_MODE, llMNPlayers1PModeSmashLogoSprite, 0x1950u) \
    X(NDS_RELOC_ASSET_MN_PLAYERS1P_MODE, llMNPlayers1PModeOptionTextSprite, 0x1ec8u) \
    X(NDS_RELOC_ASSET_MN_PLAYERS1P_MODE, llMNPlayers1PModeSecSprite, 0x1f48u) \
    X(NDS_RELOC_ASSET_MN_PLAYERS1P_MODE, llMNPlayers1PModeCSecSprite, 0x1fc8u) \
    X(NDS_RELOC_ASSET_MN_PLAYERS1P_MODE, llMNPlayers1PModeGateCPLUT, 0x3238u) \
    X(NDS_RELOC_ASSET_MN_PLAYERS1P_MODE, llMNPlayers1PModeRedCardSprite, 0x32a8u)

#define NDS_DECLARE_MN_PLAYERS1P_MODE_RELOC_SYMBOL(asset, name, value) extern uintptr_t name;
NDS_MN_PLAYERS1P_MODE_RELOC_SYMBOLS(NDS_DECLARE_MN_PLAYERS1P_MODE_RELOC_SYMBOL)
#undef NDS_DECLARE_MN_PLAYERS1P_MODE_RELOC_SYMBOL

/* MN1PContinue (reloc file 0x4f, reloc_menus): staged by scripts/menus/stage_reloc_file.py. */
extern uintptr_t llMN1PContinueFileID;

#define NDS_MN1P_CONTINUE_RELOC_SYMBOLS(X) \
    X(NDS_RELOC_ASSET_MN1P_CONTINUE, llMN1PContinueContinueTextSprite, 0x18f0u) \
    X(NDS_RELOC_ASSET_MN1P_CONTINUE, llMN1PContinueYesTextSprite, 0x1e08u) \
    X(NDS_RELOC_ASSET_MN1P_CONTINUE, llMN1PContinueNoTextSprite, 0x2318u) \
    X(NDS_RELOC_ASSET_MN1P_CONTINUE, llMN1PContinueCursorSprite, 0x2df8u) \
    X(NDS_RELOC_ASSET_MN1P_CONTINUE, llMN1PContinueRoomSprite, 0x1e3d8u) \
    X(NDS_RELOC_ASSET_MN1P_CONTINUE, llMN1PContinueSpotlightSprite, 0x21900u) \
    X(NDS_RELOC_ASSET_MN1P_CONTINUE, llMN1PContinueShadowSprite, 0x224f8u)

#define NDS_DECLARE_MN1P_CONTINUE_RELOC_SYMBOL(asset, name, value) extern uintptr_t name;
NDS_MN1P_CONTINUE_RELOC_SYMBOLS(NDS_DECLARE_MN1P_CONTINUE_RELOC_SYMBOL)
#undef NDS_DECLARE_MN1P_CONTINUE_RELOC_SYMBOL

/* MN1P (reloc file 0x2, reloc_menus): staged by scripts/menus/stage_reloc_file.py. */
extern uintptr_t llMN1PFileID;

#define NDS_MN1P_RELOC_SYMBOLS(X) \
    X(NDS_RELOC_ASSET_MN1P, llMN1P1PGameTextJapSprite, 0x04c8u) \
    X(NDS_RELOC_ASSET_MN1P, llMN1POptionTabSprite, 0x1108u) \
    X(NDS_RELOC_ASSET_MN1P, llMN1PTrainingModeTextJapSprite, 0x1618u) \
    X(NDS_RELOC_ASSET_MN1P, llMN1PBonus1PracticeTextJapSprite, 0x1df8u) \
    X(NDS_RELOC_ASSET_MN1P, llMN1PBonus2PracticeTextJapSprite, 0x25d8u) \
    X(NDS_RELOC_ASSET_MN1P, llMN1P1PGameTextSprite, 0x2a28u) \
    X(NDS_RELOC_ASSET_MN1P, llMN1PControllerIconDarkSprite, 0x50f8u) \
    X(NDS_RELOC_ASSET_MN1P, llMN1P1PTextSprite, 0x5338u) \
    X(NDS_RELOC_ASSET_MN1P, llMN1PTrainingModeTextSprite, 0x5ac8u) \
    X(NDS_RELOC_ASSET_MN1P, llMN1PBonus1PracticeTextSprite, 0x5f28u) \
    X(NDS_RELOC_ASSET_MN1P, llMN1PBonus2PracticeTextSprite, 0x6388u)

#define NDS_DECLARE_MN1P_RELOC_SYMBOL(asset, name, value) extern uintptr_t name;
NDS_MN1P_RELOC_SYMBOLS(NDS_DECLARE_MN1P_RELOC_SYMBOL)
#undef NDS_DECLARE_MN1P_RELOC_SYMBOL

/* SC1PIntro (reloc file 0xb, reloc_scene): staged by scripts/menus/stage_reloc_file.py. */
extern uintptr_t llSC1PIntroFileID;

#define NDS_SC1P_INTRO_RELOC_SYMBOLS(X) \
    X(NDS_RELOC_ASSET_SC1P_INTRO, llSC1PIntroVSDecalSprite, 0x1f10u) \
    X(NDS_RELOC_ASSET_SC1P_INTRO, llSC1PIntroNumber1Sprite, 0x2018u) \
    X(NDS_RELOC_ASSET_SC1P_INTRO, llSC1PIntroNumber2Sprite, 0x2118u) \
    X(NDS_RELOC_ASSET_SC1P_INTRO, llSC1PIntroNumber3Sprite, 0x2218u) \
    X(NDS_RELOC_ASSET_SC1P_INTRO, llSC1PIntroNumber4Sprite, 0x2318u) \
    X(NDS_RELOC_ASSET_SC1P_INTRO, llSC1PIntroNumber5Sprite, 0x2418u) \
    X(NDS_RELOC_ASSET_SC1P_INTRO, llSC1PIntroNumber6Sprite, 0x2518u) \
    X(NDS_RELOC_ASSET_SC1P_INTRO, llSC1PIntroNumber7Sprite, 0x2618u) \
    X(NDS_RELOC_ASSET_SC1P_INTRO, llSC1PIntroNumber8Sprite, 0x2718u) \
    X(NDS_RELOC_ASSET_SC1P_INTRO, llSC1PIntroNumber9Sprite, 0x2818u) \
    X(NDS_RELOC_ASSET_SC1P_INTRO, llSC1PIntroNumber10Sprite, 0x29b8u) \
    X(NDS_RELOC_ASSET_SC1P_INTRO, llSC1PIntroCrossSprite, 0x2b58u) \
    X(NDS_RELOC_ASSET_SC1P_INTRO, llSC1PIntroStageTextSprite, 0x2e38u) \
    X(NDS_RELOC_ASSET_SC1P_INTRO, llSC1PIntroBonusTextSprite, 0x30f8u) \
    X(NDS_RELOC_ASSET_SC1P_INTRO, llSC1PIntroFinalTextSprite, 0x3320u) \
    X(NDS_RELOC_ASSET_SC1P_INTRO, llSC1PIntroBreakTheTargetsTextSprite, 0x3b08u) \
    X(NDS_RELOC_ASSET_SC1P_INTRO, llSC1PIntroBoardThePlatformsTextSprite, 0x4388u) \
    X(NDS_RELOC_ASSET_SC1P_INTRO, llSC1PIntroRaceToTheFinishTextSprite, 0x4ac8u) \
    X(NDS_RELOC_ASSET_SC1P_INTRO, llSC1PIntro0x5028Sprite, 0x5028u) \
    X(NDS_RELOC_ASSET_SC1P_INTRO, llSC1PIntroDashSprite, 0x50e8u) \
    X(NDS_RELOC_ASSET_SC1P_INTRO, llSC1PIntroMetalMarioTextSprite, 0x5328u) \
    X(NDS_RELOC_ASSET_SC1P_INTRO, llSC1PIntroMasterHandTextSprite, 0x5568u) \
    X(NDS_RELOC_ASSET_SC1P_INTRO, llSC1PIntroGiantDKTextSprite, 0x5748u) \
    X(NDS_RELOC_ASSET_SC1P_INTRO, llSC1PIntroFoxMcCloudTextSprite, 0x5988u) \
    X(NDS_RELOC_ASSET_SC1P_INTRO, llSC1PIntroKirbyTeamVS8TextSprite, 0x5c88u) \
    X(NDS_RELOC_ASSET_SC1P_INTRO, llSC1PIntroMarioBrosTextSprite, 0x5ec8u) \
    X(NDS_RELOC_ASSET_SC1P_INTRO, llSC1PIntroFightingPolygonTeamVS30TextSprite, 0x63f8u) \
    X(NDS_RELOC_ASSET_SC1P_INTRO, llSC1PIntroSamusAranTextSprite, 0x6638u) \
    X(NDS_RELOC_ASSET_SC1P_INTRO, llSC1PIntroYoshiTeamVS18TextSprite, 0x6938u) \
    X(NDS_RELOC_ASSET_SC1P_INTRO, llSC1PIntroVSTextSprite, 0x69f8u) \
    X(NDS_RELOC_ASSET_SC1P_INTRO, llSC1PIntroAllyTextSprite, 0x6b18u) \
    X(NDS_RELOC_ASSET_SC1P_INTRO, llSC1PIntroAllyText2Sprite, 0x6c38u) \
    X(NDS_RELOC_ASSET_SC1P_INTRO, llSC1PIntroFighterMarioCamAnimJoint, 0x6c80u) \
    X(NDS_RELOC_ASSET_SC1P_INTRO, llSC1PIntroFighterFoxCamAnimJoint, 0x6cb0u) \
    X(NDS_RELOC_ASSET_SC1P_INTRO, llSC1PIntroFighterDonkeyCamAnimJoint, 0x6ce0u) \
    X(NDS_RELOC_ASSET_SC1P_INTRO, llSC1PIntroFighterSamusCamAnimJoint, 0x6d10u) \
    X(NDS_RELOC_ASSET_SC1P_INTRO, llSC1PIntroFighterLuigiCamAnimJoint, 0x6d40u) \
    X(NDS_RELOC_ASSET_SC1P_INTRO, llSC1PIntroFighterLinkCamAnimJoint, 0x6d70u) \
    X(NDS_RELOC_ASSET_SC1P_INTRO, llSC1PIntroFighterYoshiCamAnimJoint, 0x6da0u) \
    X(NDS_RELOC_ASSET_SC1P_INTRO, llSC1PIntroFighterCaptainCamAnimJoint, 0x6dd0u) \
    X(NDS_RELOC_ASSET_SC1P_INTRO, llSC1PIntroFighterKirbyCamAnimJoint, 0x6e00u) \
    X(NDS_RELOC_ASSET_SC1P_INTRO, llSC1PIntroFighterPikachuCamAnimJoint, 0x6e30u) \
    X(NDS_RELOC_ASSET_SC1P_INTRO, llSC1PIntroFighterPurinCamAnimJoint, 0x6e60u) \
    X(NDS_RELOC_ASSET_SC1P_INTRO, llSC1PIntroFighterNessCamAnimJoint, 0x6e90u) \
    X(NDS_RELOC_ASSET_SC1P_INTRO, llSC1PIntroStageKirbyCamAnimJoint, 0x6ec0u) \
    X(NDS_RELOC_ASSET_SC1P_INTRO, llSC1PIntroStageYoshiCamAnimJoint, 0x6ef0u) \
    X(NDS_RELOC_ASSET_SC1P_INTRO, llSC1PIntroStageBossCamAnimJoint, 0x6f20u) \
    X(NDS_RELOC_ASSET_SC1P_INTRO, llSC1PIntroStageSamusCamAnimJoint, 0x6f50u) \
    X(NDS_RELOC_ASSET_SC1P_INTRO, llSC1PIntroStageFoxCamAnimJoint, 0x6f80u) \
    X(NDS_RELOC_ASSET_SC1P_INTRO, llSC1PIntroStagePikachuCamAnimJoint, 0x6fb0u) \
    X(NDS_RELOC_ASSET_SC1P_INTRO, llSC1PIntroStageLinkCamAnimJoint, 0x6fe0u) \
    X(NDS_RELOC_ASSET_SC1P_INTRO, llSC1PIntroStageDonkeyCamAnimJoint, 0x7010u) \
    X(NDS_RELOC_ASSET_SC1P_INTRO, llSC1PIntroStageMarioCamAnimJoint, 0x7040u) \
    X(NDS_RELOC_ASSET_SC1P_INTRO, llSC1PIntroStageMMarioCamAnimJoint, 0x7070u) \
    X(NDS_RELOC_ASSET_SC1P_INTRO, llSC1PIntroStageZakoCamAnimJoint, 0x70a0u) \
    X(NDS_RELOC_ASSET_SC1P_INTRO, llSC1PIntroLinkMarkerSprite, 0x71d0u) \
    X(NDS_RELOC_ASSET_SC1P_INTRO, llSC1PIntroYoshiMarkerSprite, 0x7320u) \
    X(NDS_RELOC_ASSET_SC1P_INTRO, llSC1PIntroFoxMarkerSprite, 0x7470u) \
    X(NDS_RELOC_ASSET_SC1P_INTRO, llSC1PIntroMarioBrosMarkerSprite, 0x75c0u) \
    X(NDS_RELOC_ASSET_SC1P_INTRO, llSC1PIntroPikachuMarkerSprite, 0x7710u) \
    X(NDS_RELOC_ASSET_SC1P_INTRO, llSC1PIntroDKMarkerSprite, 0x7860u) \
    X(NDS_RELOC_ASSET_SC1P_INTRO, llSC1PIntroKirbyMarkerSprite, 0x79b0u) \
    X(NDS_RELOC_ASSET_SC1P_INTRO, llSC1PIntroSamusMarkerSprite, 0x7b00u) \
    X(NDS_RELOC_ASSET_SC1P_INTRO, llSC1PIntroMarioMarkerSprite, 0x7c50u) \
    X(NDS_RELOC_ASSET_SC1P_INTRO, llSC1PIntroExclamationMarkSprite, 0x7d60u) \
    X(NDS_RELOC_ASSET_SC1P_INTRO, llSC1PIntroBossMarkerSprite, 0x7e70u) \
    X(NDS_RELOC_ASSET_SC1P_INTRO, llSC1PIntroBonusMarkerSprite, 0x7f40u) \
    X(NDS_RELOC_ASSET_SC1P_INTRO, llSC1PIntroBannerTopSprite, 0xc898u) \
    X(NDS_RELOC_ASSET_SC1P_INTRO, llSC1PIntroBannerBottomSprite, 0xed00u) \
    X(NDS_RELOC_ASSET_SC1P_INTRO, llSC1PIntroSkySprite, 0x14bf0u)

#define NDS_DECLARE_SC1P_INTRO_RELOC_SYMBOL(asset, name, value) extern uintptr_t name;
NDS_SC1P_INTRO_RELOC_SYMBOLS(NDS_DECLARE_SC1P_INTRO_RELOC_SYMBOL)
#undef NDS_DECLARE_SC1P_INTRO_RELOC_SYMBOL

/* SC1PChallenger (reloc file 0xa, reloc_scene): staged by scripts/menus/stage_reloc_file.py. */
extern uintptr_t llSC1PChallengerFileID;

#define NDS_SC1P_CHALLENGER_RELOC_SYMBOLS(X) \
    X(NDS_RELOC_ASSET_SC1P_CHALLENGER, llSC1PChallengerChallengerTextSprite, 0x01f8u) \
    X(NDS_RELOC_ASSET_SC1P_CHALLENGER, llSC1PChallengerApproachingTextSprite, 0x0488u) \
    X(NDS_RELOC_ASSET_SC1P_CHALLENGER, llSC1PChallengerWarningTextSprite, 0x0968u) \
    X(NDS_RELOC_ASSET_SC1P_CHALLENGER, llSC1PChallengerDecalExclaimSprite, 0x0db0u)

#define NDS_DECLARE_SC1P_CHALLENGER_RELOC_SYMBOL(asset, name, value) extern uintptr_t name;
NDS_SC1P_CHALLENGER_RELOC_SYMBOLS(NDS_DECLARE_SC1P_CHALLENGER_RELOC_SYMBOL)
#undef NDS_DECLARE_SC1P_CHALLENGER_RELOC_SYMBOL

/* SC1PStageClear3 (reloc file 0x97, reloc_scene): staged by scripts/menus/stage_reloc_file.py. */
extern uintptr_t llSC1PStageClear3FileID;

#define NDS_SC1P_STAGE_CLEAR3_RELOC_SYMBOLS(X) \
    X(NDS_RELOC_ASSET_SC1P_STAGE_CLEAR3, llSC1PStageClear3PlatformSprite, 0x00c0u) \
    X(NDS_RELOC_ASSET_SC1P_STAGE_CLEAR3, llSC1PStageClear3TargetSprite, 0x01d0u)

#define NDS_DECLARE_SC1P_STAGE_CLEAR3_RELOC_SYMBOL(asset, name, value) extern uintptr_t name;
NDS_SC1P_STAGE_CLEAR3_RELOC_SYMBOLS(NDS_DECLARE_SC1P_STAGE_CLEAR3_RELOC_SYMBOL)
#undef NDS_DECLARE_SC1P_STAGE_CLEAR3_RELOC_SYMBOL

/* SC1PStageClear2 (reloc file 0x51, reloc_scene): staged by scripts/menus/stage_reloc_file.py. */
extern uintptr_t llSC1PStageClear2FileID;

#define NDS_SC1P_STAGE_CLEAR2_RELOC_SYMBOLS(X) \
    X(NDS_RELOC_ASSET_SC1P_STAGE_CLEAR2, llSC1PStageClear2ScoreTextSprite, 0x0408u)

#define NDS_DECLARE_SC1P_STAGE_CLEAR2_RELOC_SYMBOL(asset, name, value) extern uintptr_t name;
NDS_SC1P_STAGE_CLEAR2_RELOC_SYMBOLS(NDS_DECLARE_SC1P_STAGE_CLEAR2_RELOC_SYMBOL)
#undef NDS_DECLARE_SC1P_STAGE_CLEAR2_RELOC_SYMBOL

/* SC1PStageClear1 (reloc file 0x50, reloc_scene): staged by scripts/menus/stage_reloc_file.py. */
extern uintptr_t llSC1PStageClear1FileID;

#define NDS_SC1P_STAGE_CLEAR1_RELOC_SYMBOLS(X) \
    X(NDS_RELOC_ASSET_SC1P_STAGE_CLEAR1, llSC1PStageClear1StageTextSprite, 0x09d8u) \
    X(NDS_RELOC_ASSET_SC1P_STAGE_CLEAR1, llSC1PStageClear1GameTextSprite, 0x1338u) \
    X(NDS_RELOC_ASSET_SC1P_STAGE_CLEAR1, llSC1PStageClear1ClearTextSprite, 0x1d58u) \
    X(NDS_RELOC_ASSET_SC1P_STAGE_CLEAR1, llSC1PStageClear1SpecialBonusTextSprite, 0x2060u) \
    X(NDS_RELOC_ASSET_SC1P_STAGE_CLEAR1, llSC1PStageClear1ColonTextSprite, 0x2120u) \
    X(NDS_RELOC_ASSET_SC1P_STAGE_CLEAR1, llSC1PStageClear1TimerTextSprite, 0x25e8u) \
    X(NDS_RELOC_ASSET_SC1P_STAGE_CLEAR1, llSC1PStageClear1DamageTextSprite, 0x2b48u) \
    X(NDS_RELOC_ASSET_SC1P_STAGE_CLEAR1, llSC1PStageClear10x3028Sprite, 0x3028u) \
    X(NDS_RELOC_ASSET_SC1P_STAGE_CLEAR1, llSC1PStageClear10x30f8Sprite, 0x30f8u) \
    X(NDS_RELOC_ASSET_SC1P_STAGE_CLEAR1, llSC1PStageClear10x31c8Sprite, 0x31c8u) \
    X(NDS_RELOC_ASSET_SC1P_STAGE_CLEAR1, llSC1PStageClear1BonusBorderSprite, 0xa4b8u) \
    X(NDS_RELOC_ASSET_SC1P_STAGE_CLEAR1, llSC1PStageClear1ResultTextSprite, 0xaf98u) \
    X(NDS_RELOC_ASSET_SC1P_STAGE_CLEAR1, llSC1PStageClear1TargetTextSprite, 0xb4f8u) \
    X(NDS_RELOC_ASSET_SC1P_STAGE_CLEAR1, llSC1PStageClear1BonusPageArrowSprite, 0xb6a8u) \
    X(NDS_RELOC_ASSET_SC1P_STAGE_CLEAR1, llSC1PStageClear1TimerDamageDigit0Sprite, 0xb808u) \
    X(NDS_RELOC_ASSET_SC1P_STAGE_CLEAR1, llSC1PStageClear1TimerDamageDigit1Sprite, 0xb968u) \
    X(NDS_RELOC_ASSET_SC1P_STAGE_CLEAR1, llSC1PStageClear1TimerDamageDigit2Sprite, 0xbac8u) \
    X(NDS_RELOC_ASSET_SC1P_STAGE_CLEAR1, llSC1PStageClear1TimerDamageDigit3Sprite, 0xbc28u) \
    X(NDS_RELOC_ASSET_SC1P_STAGE_CLEAR1, llSC1PStageClear1TimerDamageDigit4Sprite, 0xbd88u) \
    X(NDS_RELOC_ASSET_SC1P_STAGE_CLEAR1, llSC1PStageClear1TimerDamageDigit5Sprite, 0xbee8u) \
    X(NDS_RELOC_ASSET_SC1P_STAGE_CLEAR1, llSC1PStageClear1TimerDamageDigit6Sprite, 0xc048u) \
    X(NDS_RELOC_ASSET_SC1P_STAGE_CLEAR1, llSC1PStageClear1TimerDamageDigit7Sprite, 0xc1a8u) \
    X(NDS_RELOC_ASSET_SC1P_STAGE_CLEAR1, llSC1PStageClear1TimerDamageDigit8Sprite, 0xc308u) \
    X(NDS_RELOC_ASSET_SC1P_STAGE_CLEAR1, llSC1PStageClear1TimerDamageDigit9Sprite, 0xc468u) \
    X(NDS_RELOC_ASSET_SC1P_STAGE_CLEAR1, llSC1PStageClear1TextShadowSprite, 0xd1c8u) \
    X(NDS_RELOC_ASSET_SC1P_STAGE_CLEAR1, llSC1PStageClear1BonusTextSprite, 0xd340u) \
    X(NDS_RELOC_ASSET_SC1P_STAGE_CLEAR1, llSC1PStageClear1CheapShotTextSprite, 0xd528u) \
    X(NDS_RELOC_ASSET_SC1P_STAGE_CLEAR1, llSC1PStageClear1StarFinishTextSprite, 0xd708u) \
    X(NDS_RELOC_ASSET_SC1P_STAGE_CLEAR1, llSC1PStageClear1NoItemTextSprite, 0xd8e8u) \
    X(NDS_RELOC_ASSET_SC1P_STAGE_CLEAR1, llSC1PStageClear1ShieldBreakerTextSprite, 0xdac8u) \
    X(NDS_RELOC_ASSET_SC1P_STAGE_CLEAR1, llSC1PStageClear1JudoWarriorTextSprite, 0xdca8u) \
    X(NDS_RELOC_ASSET_SC1P_STAGE_CLEAR1, llSC1PStageClear1HawkTextSprite, 0xde88u) \
    X(NDS_RELOC_ASSET_SC1P_STAGE_CLEAR1, llSC1PStageClear1ShooterTextSprite, 0xe068u) \
    X(NDS_RELOC_ASSET_SC1P_STAGE_CLEAR1, llSC1PStageClear1HeavyDamageTextSprite, 0xe248u) \
    X(NDS_RELOC_ASSET_SC1P_STAGE_CLEAR1, llSC1PStageClear1AllVariationsTextSprite, 0xe428u) \
    X(NDS_RELOC_ASSET_SC1P_STAGE_CLEAR1, llSC1PStageClear1ItemStrikeTextSprite, 0xe608u) \
    X(NDS_RELOC_ASSET_SC1P_STAGE_CLEAR1, llSC1PStageClear1DoubleKOTextSprite, 0xe7e8u) \
    X(NDS_RELOC_ASSET_SC1P_STAGE_CLEAR1, llSC1PStageClear1TricksterTextSprite, 0xe9c8u) \
    X(NDS_RELOC_ASSET_SC1P_STAGE_CLEAR1, llSC1PStageClear1GiantImpactTextSprite, 0xeba8u) \
    X(NDS_RELOC_ASSET_SC1P_STAGE_CLEAR1, llSC1PStageClear1SpeedsterTextSprite, 0xed88u) \
    X(NDS_RELOC_ASSET_SC1P_STAGE_CLEAR1, llSC1PStageClear1ItemThrowTextSprite, 0xef68u) \
    X(NDS_RELOC_ASSET_SC1P_STAGE_CLEAR1, llSC1PStageClear1TripleKOTextSprite, 0xf148u) \
    X(NDS_RELOC_ASSET_SC1P_STAGE_CLEAR1, llSC1PStageClear1LastChanceTextSprite, 0xf328u) \
    X(NDS_RELOC_ASSET_SC1P_STAGE_CLEAR1, llSC1PStageClear1PacifistTextSprite, 0xf508u) \
    X(NDS_RELOC_ASSET_SC1P_STAGE_CLEAR1, llSC1PStageClear1PerfectTextSprite, 0xf6e8u) \
    X(NDS_RELOC_ASSET_SC1P_STAGE_CLEAR1, llSC1PStageClear1NoMissTextSprite, 0xf8c8u) \
    X(NDS_RELOC_ASSET_SC1P_STAGE_CLEAR1, llSC1PStageClear1NoDamageTextSprite, 0xfaa8u) \
    X(NDS_RELOC_ASSET_SC1P_STAGE_CLEAR1, llSC1PStageClear1FullPowerTextSprite, 0xfc88u) \
    X(NDS_RELOC_ASSET_SC1P_STAGE_CLEAR1, llSC1PStageClear1MewCatcherTextSprite, 0xfe68u) \
    X(NDS_RELOC_ASSET_SC1P_STAGE_CLEAR1, llSC1PStageClear1StarClearTextSprite, 0x10048u) \
    X(NDS_RELOC_ASSET_SC1P_STAGE_CLEAR1, llSC1PStageClear1VegetarianTextSprite, 0x10228u) \
    X(NDS_RELOC_ASSET_SC1P_STAGE_CLEAR1, llSC1PStageClear1HeartThrobTextSprite, 0x10408u) \
    X(NDS_RELOC_ASSET_SC1P_STAGE_CLEAR1, llSC1PStageClear1ThrowDownTextSprite, 0x105e8u) \
    X(NDS_RELOC_ASSET_SC1P_STAGE_CLEAR1, llSC1PStageClear1SmashManiaTextSprite, 0x107c8u) \
    X(NDS_RELOC_ASSET_SC1P_STAGE_CLEAR1, llSC1PStageClear1SmashlessTextSprite, 0x109a8u) \
    X(NDS_RELOC_ASSET_SC1P_STAGE_CLEAR1, llSC1PStageClear1SpecialMoveTextSprite, 0x10b88u) \
    X(NDS_RELOC_ASSET_SC1P_STAGE_CLEAR1, llSC1PStageClear1SingleMoveTextSprite, 0x10d68u) \
    X(NDS_RELOC_ASSET_SC1P_STAGE_CLEAR1, llSC1PStageClear1PokemonFinishTextSprite, 0x10f48u) \
    X(NDS_RELOC_ASSET_SC1P_STAGE_CLEAR1, llSC1PStageClear1BoobyTrapTextSprite, 0x11128u) \
    X(NDS_RELOC_ASSET_SC1P_STAGE_CLEAR1, llSC1PStageClear1FighterStanceTextSprite, 0x11308u) \
    X(NDS_RELOC_ASSET_SC1P_STAGE_CLEAR1, llSC1PStageClear1MysticTextSprite, 0x114e8u) \
    X(NDS_RELOC_ASSET_SC1P_STAGE_CLEAR1, llSC1PStageClear1CometMysticTextSprite, 0x116c8u) \
    X(NDS_RELOC_ASSET_SC1P_STAGE_CLEAR1, llSC1PStageClear1AcidClearTextSprite, 0x118a8u) \
    X(NDS_RELOC_ASSET_SC1P_STAGE_CLEAR1, llSC1PStageClear1BumperClearTextSprite, 0x11a88u) \
    X(NDS_RELOC_ASSET_SC1P_STAGE_CLEAR1, llSC1PStageClear1TornadoClearTextSprite, 0x11c68u) \
    X(NDS_RELOC_ASSET_SC1P_STAGE_CLEAR1, llSC1PStageClear1ArwingClearTextSprite, 0x11e48u) \
    X(NDS_RELOC_ASSET_SC1P_STAGE_CLEAR1, llSC1PStageClear1CounterAttackTextSprite, 0x12028u) \
    X(NDS_RELOC_ASSET_SC1P_STAGE_CLEAR1, llSC1PStageClear1MeteorSmashTextSprite, 0x12208u) \
    X(NDS_RELOC_ASSET_SC1P_STAGE_CLEAR1, llSC1PStageClear1AerialTextSprite, 0x123e8u) \
    X(NDS_RELOC_ASSET_SC1P_STAGE_CLEAR1, llSC1PStageClear1LastSecondTextSprite, 0x125c8u) \
    X(NDS_RELOC_ASSET_SC1P_STAGE_CLEAR1, llSC1PStageClear1Lucky3TextSprite, 0x127a8u) \
    X(NDS_RELOC_ASSET_SC1P_STAGE_CLEAR1, llSC1PStageClear1JackpotTextSprite, 0x12988u) \
    X(NDS_RELOC_ASSET_SC1P_STAGE_CLEAR1, llSC1PStageClear1YoshiRainbowTextSprite, 0x12b68u) \
    X(NDS_RELOC_ASSET_SC1P_STAGE_CLEAR1, llSC1PStageClear1KirbyRanksTextSprite, 0x12d48u) \
    X(NDS_RELOC_ASSET_SC1P_STAGE_CLEAR1, llSC1PStageClear1BrosCalamityTextSprite, 0x12f28u) \
    X(NDS_RELOC_ASSET_SC1P_STAGE_CLEAR1, llSC1PStageClear1DKDefenderTextSprite, 0x13108u) \
    X(NDS_RELOC_ASSET_SC1P_STAGE_CLEAR1, llSC1PStageClear1DKPerfectTextSprite, 0x132e8u) \
    X(NDS_RELOC_ASSET_SC1P_STAGE_CLEAR1, llSC1PStageClear1GoodFriendTextSprite, 0x134c8u) \
    X(NDS_RELOC_ASSET_SC1P_STAGE_CLEAR1, llSC1PStageClear1TrueFriendTextSprite, 0x136a8u) \
    X(NDS_RELOC_ASSET_SC1P_STAGE_CLEAR1, llSC1PStageClear1NoMissClearTextSprite, 0x13888u) \
    X(NDS_RELOC_ASSET_SC1P_STAGE_CLEAR1, llSC1PStageClear1NoDamageClearTextSprite, 0x13a68u) \
    X(NDS_RELOC_ASSET_SC1P_STAGE_CLEAR1, llSC1PStageClear1SpeedKingTextSprite, 0x13c48u) \
    X(NDS_RELOC_ASSET_SC1P_STAGE_CLEAR1, llSC1PStageClear1SpeedDemonTextSprite, 0x13e28u) \
    X(NDS_RELOC_ASSET_SC1P_STAGE_CLEAR1, llSC1PStageClear1VeryEasyClearTextSprite, 0x14008u) \
    X(NDS_RELOC_ASSET_SC1P_STAGE_CLEAR1, llSC1PStageClear1EasyClearTextSprite, 0x141e8u) \
    X(NDS_RELOC_ASSET_SC1P_STAGE_CLEAR1, llSC1PStageClear1NormalClearTextSprite, 0x143c8u) \
    X(NDS_RELOC_ASSET_SC1P_STAGE_CLEAR1, llSC1PStageClear1HardClearTextSprite, 0x145a8u) \
    X(NDS_RELOC_ASSET_SC1P_STAGE_CLEAR1, llSC1PStageClear1VeryHardClearTextSprite, 0x14788u)

#define NDS_DECLARE_SC1P_STAGE_CLEAR1_RELOC_SYMBOL(asset, name, value) extern uintptr_t name;
NDS_SC1P_STAGE_CLEAR1_RELOC_SYMBOLS(NDS_DECLARE_SC1P_STAGE_CLEAR1_RELOC_SYMBOL)
#undef NDS_DECLARE_SC1P_STAGE_CLEAR1_RELOC_SYMBOL

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
    X(ll_113_FileID, 0x71) \
    X(llGRCastleMapFileID, 0x103) \
    X(llGRCastleMapMapHeader, 0x14) \
    X(llGRHyruleMapFileID, 0x109) \
    X(llGRHyruleMapMapHeader, 0x14) \
    X(llGRHyruleMapTwisterThrowHitDesc, 0xbc) \
    X(llGRInishieMapFileID, 0x104) \
    X(llGRInishieMapMapHeader, 0x14) \
    X(llGRInishieMapScaleDObjDesc, 0x380) \
    X(llGRInishieMapMapHead, 0x5f0) \
    X(llGRInishieMapScaleRetractAnimJoint, 0x734) \
    X(llGRJungleMapFileID, 0x105) \
    X(llGRJungleMapMapHeader, 0x14) \
    X(llGRPupupuMapFileID, 0xff) \
    X(llGRPupupuMapMapHeader, 0x14) \
    X(llGRPupupuMapWhispyEyesTransformKindsMObjSub, 0x0f00) \
    X(llGRPupupuMapMapHead, 0x10f0) \
    X(llGRPupupuMapWhispyEyesTransformKindsDObjDesc, 0x10f0) \
    X(llGRPupupuMapWhispyMouthTransformKindsMObjSub, 0x13b0) \
    X(llGRPupupuMapWhispyMouthTransformKindsDObjDesc, 0x1770) \
    X(llGRPupupuMapFlowersBackTransformKindsDObjDesc, 0x2a80) \
    X(llGRPupupuMapFlowersFrontTransformKindsDObjDesc, 0x31f8) \
    X(llGRPupupuMapWhispyEyesLeftTurnAnimJoint, 0x11a0) \
    X(llGRPupupuMapWhispyEyesLeftTurnMatAnimJoint, 0x11e0) \
    X(llGRPupupuMapWhispyEyesLeftBlinkAnimJoint, 0x12b0) \
    X(llGRPupupuMapWhispyEyesRightTurnAnimJoint, 0x1220) \
    X(llGRPupupuMapWhispyEyesRightTurnMatAnimJoint, 0x1270) \
    X(llGRPupupuMapWhispyEyesRightBlinkAnimJoint, 0x1330) \
    X(llGRPupupuMapWhispyMouthLeftStretchAnimJoint, 0x18b0) \
    X(llGRPupupuMapWhispyMouthLeftStretchMatAnimJoint, 0x1a00) \
    X(llGRPupupuMapWhispyMouthLeftTurnAnimJoint, 0x1be0) \
    X(llGRPupupuMapWhispyMouthLeftTurnMatAnimJoint, 0x1ce0) \
    X(llGRPupupuMapWhispyMouthLeftOpenAnimJoint, 0x1e80) \
    X(llGRPupupuMapWhispyMouthLeftOpenMatAnimJoint, 0x20b0) \
    X(llGRPupupuMapWhispyMouthLeftCloseAnimJoint, 0x2100) \
    X(llGRPupupuMapWhispyMouthLeftCloseMatAnimJoint, 0x22a0) \
    X(llGRPupupuMapWhispyMouthRightStretchAnimJoint, 0x1a40) \
    X(llGRPupupuMapWhispyMouthRightStretchMatAnimJoint, 0x1ba0) \
    X(llGRPupupuMapWhispyMouthRightTurnAnimJoint, 0x1d30) \
    X(llGRPupupuMapWhispyMouthRightTurnMatAnimJoint, 0x1e30) \
    X(llGRPupupuMapWhispyMouthRightOpenAnimJoint, 0x22f0) \
    X(llGRPupupuMapWhispyMouthRightOpenMatAnimJoint, 0x2540) \
    X(llGRPupupuMapWhispyMouthRightCloseAnimJoint, 0x2590) \
    X(llGRPupupuMapWhispyMouthRightCloseMatAnimJoint, 0x2740) \
    X(llGRPupupuMapWhispyMouthLeftOpenTexture, 0x2be0) \
    X(llGRPupupuMapWhispyMouthLeftBlowTexture, 0x2c30) \
    X(llGRPupupuMapWhispyMouthLeftCloseTexture, 0x2c80) \
    X(llGRPupupuMapWhispyMouthRightOpenTexture, 0x2cd0) \
    X(llGRPupupuMapWhispyMouthRightBlowTexture, 0x2d20) \
    X(llGRPupupuMapWhispyMouthRightCloseTexture, 0x2d70) \
    X(llGRPupupuMapWhispyEyesLeft0Texture, 0x33e0) \
    X(llGRPupupuMapWhispyEyesLeft1Texture, 0x3450) \
    X(llGRPupupuMapWhispyEyesLeft2Texture, 0x34b0) \
    X(llGRPupupuMapWhispyEyesRight0Texture, 0x3510) \
    X(llGRPupupuMapWhispyEyesRight1Texture, 0x35c0) \
    X(llGRPupupuMapWhispyEyesRight2Texture, 0x3660) \
    X(llStageCastleFileID, 0x5f) \
    X(llStageCastleSprite, 0x26c88) \
    X(llStageDreamLandFileID, 0x58) \
    X(llStageDreamLandSprite, 0x26c88) \
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
    X(llGRYosterMapMapHead, 0x100) \
    X(llGRYosterMap_1E0_AnimJoint, 0x1e0) \
    X(llGRYosterMap_4B8_MObjSub, 0x4b8) \
    X(llGRYosterMapCloudDisplayList, 0x580) \
    X(llGRYosterMapCloudSolidMatAnimJoint, 0x670) \
    X(llGRYosterMapCloudEvaporateMatAnimJoint, 0x690) \
    X(llStageYoshiFileID, 0x5d) \
    X(llStageYoshiSprite, 0x26c88) \
    X(llStageJungleFileID, 0x5c) \
    X(llStageJungleSprite, 0x26c88) \
    X(llStageZebesFileID, 0x59) \
    X(llStageZebesSprite, 0x26c88) \
    X(llStagePokemonFileID, 0x5e) \
    X(llStagePokemonSprite, 0x26c88) \
    /* P2-4s7. Mushroom Kingdom's wallpaper container is relocData file 91,\
     * which decomp's symbol table names only as ll_91_FileID (0x5b) -- it has\
     * no ll<Name>Sprite entry. The offset is the same 0x26c88 every stage\
     * wallpaper uses, read from the map header's own reference to\
     * dStageInishieBackground_0x26c88 (260_GRInishieMap.c:51). The name below\
     * is port-local and follows that data symbol. */\
    X(llStageInishieBackgroundFileID, 0x5b) \
    X(llStageInishieBackgroundSprite, 0x26c88) \
    X(llStageSectorFileID, 0x63) \
    X(llStageSectorSprite, 0x26c88) \
    X(llGRZebesMapFileID, 0x101) \
    X(llGRZebesMapMapHeader, 0x14) \
    X(llMNCommonColonSprite, 0xdcf0) \
    X(llMNCommonDigit0Sprite, 0xd310) \
    X(llMNCommonDigit1Sprite, 0xd3e0) \
    X(llMNCommonDigit2Sprite, 0xd4b0) \
    X(llMNCommonDigit3Sprite, 0xd580) \
    X(llMNCommonDigit4Sprite, 0xd650) \
    X(llMNCommonDigit5Sprite, 0xd720) \
    X(llMNCommonDigit6Sprite, 0xd7f0) \
    X(llMNCommonDigit7Sprite, 0xd8c0) \
    X(llMNCommonDigit8Sprite, 0xd990) \
    X(llMNCommonDigit9Sprite, 0xda60) \
    X(llMNCommonInfinitySprite, 0xdc48) \
    X(llMNCommonSmashBrosCollageSprite, 0x18000) \
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

#include <reloc_data_ftdata_symbols.h>

void lbRelocInitSetup(LBRelocSetup *setup);
size_t lbRelocGetFileSize(const void *file_id);
void *lbRelocGetExternHeapFile(const void *file_id, void *heap);
void *lbRelocGetForceExternHeapFile(const void *file_id, void *heap);
void *lbRelocGetStatusBufferFile(const void *file_id);
size_t lbRelocGetAllocSize(u32 *ids, u32 len);
size_t lbRelocLoadFilesExtern(u32 *ids, u32 len, void **files, void *heap);
void *ndsRelocGetFileData(void *file, const void *symbol);
/* Span of an already-loaded reloc file, 0 if not resident. NOT
 * lbRelocGetFileSize, which answers sizeof(Sprite) for a resident file -- see
 * the definition in src/port/reloc_backend_assets.c. */
size_t ndsRelocGetLoadedFileSize(const void *file_id);

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
