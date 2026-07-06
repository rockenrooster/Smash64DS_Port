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

#define NDS_IFCOMMON_RELOC_SYMBOLS(X) \
    X(NDS_RELOC_ASSET_IF_COMMON_ANNOUNCE, llIFCommonAnnounceCommonLetterTSprite, 0x5bd0u) \
    X(NDS_RELOC_ASSET_IF_COMMON_ANNOUNCE, llIFCommonAnnounceCommonSymbolExclaimSprite, 0x7d98u) \
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
    X(NDS_RELOC_ASSET_INVALID, llIFCommonItemArrowSprite, 0x50u) \
    X(NDS_RELOC_ASSET_INVALID, llIFCommonItemFileID, 0x57u) \
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

#include <reloc_data_ftdata_symbols.h>

void lbRelocInitSetup(LBRelocSetup *setup);
size_t lbRelocGetFileSize(const void *file_id);
void *lbRelocGetExternHeapFile(const void *file_id, void *heap);
void *lbRelocGetForceExternHeapFile(const void *file_id, void *heap);
void *lbRelocGetStatusBufferFile(const void *file_id);
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
