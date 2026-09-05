static MPYakumonoDObj sNdsMPCollisionYakumonoDObjs;
static Vec3f sNdsMPCollisionSpeeds[NDS_MP_YAKUMONO_DOBJ_SLOTS];
MPYakumonoDObj *gMPCollisionYakumonoDObjs =
    &sNdsMPCollisionYakumonoDObjs;
Vec3f *gMPCollisionSpeeds = sNdsMPCollisionSpeeds;
s32 gMPCollisionYakumonosNum = NDS_MP_YAKUMONO_DOBJ_SLOTS;
MPAllBounds gMPCollisionBounds;
u32 gMPCollisionBGMCurrent;
u32 gMPCollisionBGMDefault;
uintptr_t lLBRelocTableAddr;
u32 llRelocFileCount;
uintptr_t llN64LogoFileID;
uintptr_t llN64LogoSprite;
uintptr_t llIFCommonPlayerFileID;
uintptr_t llIFCommonGameStatusFileID;
uintptr_t llIFCommonPlayerDamageFileID;
uintptr_t llIFCommonTimerFileID;
uintptr_t llIFCommonDigitsFileID;
uintptr_t llIFCommonBattlePauseFileID;
uintptr_t llIFCommonPlayerTagsFileID;
uintptr_t llIFCommonAnnounceCommonFileID;
uintptr_t llMNVSResultsFileID;
uintptr_t llFTEmblemModelsFileID;
uintptr_t llFTStocksZakoFileID;
uintptr_t llLBTransitionAeroplaneFileID;
uintptr_t llLBTransitionCheckFileID;
uintptr_t llLBTransitionGakubuthiFileID;
uintptr_t llLBTransitionKannonFileID;
uintptr_t llLBTransitionStarFileID;
uintptr_t llLBTransitionSudare1FileID;
uintptr_t llLBTransitionSudare2FileID;
uintptr_t llLBTransitionBlockFileID;
uintptr_t llLBTransitionRotScaleFileID;
uintptr_t llLBTransitionCurtainFileID;
uintptr_t llLBTransitionCameraFileID;
uintptr_t llSYKseg1ValidateFileID;
uintptr_t llSYKseg1ValidateFunc;
uintptr_t llSYKseg1ValidateNBytes;
uintptr_t llFTManagerCommonFileID;
uintptr_t llFTCommonMovesetFileID = 0xc9u;
uintptr_t llEFCommonEffects1FileID;
uintptr_t llEFCommonEffects2FileID;
uintptr_t llEFCommonEffects3FileID;
uintptr_t llMarioMainMotionFileID;
uintptr_t llMarioMainFileID;
uintptr_t llMarioSpecial1FileID;
uintptr_t llMarioModelFileID;
uintptr_t llMarioSpecial3FileID;
uintptr_t llMarioShieldPoseFileID;
uintptr_t llMarioSpecial2FileID;
uintptr_t llFoxSpecial3FileID;
uintptr_t llFoxMainMotionFileID;
uintptr_t llFoxMainFileID;
uintptr_t llFoxSpecial1FileID;
uintptr_t llFoxModelFileID;
uintptr_t llFoxShieldPoseFileID;
uintptr_t llFoxSpecial4FileID;
uintptr_t llFoxSpecial2FileID;
uintptr_t llMarioModelStockSprite = 0x72d0u;
uintptr_t llMarioModelFTEmblemSprite = 0x74c8u;
uintptr_t llFoxModelStockSprite = 0x7c28u;
uintptr_t llFoxModelFTEmblemSprite = 0x7e08u;
uintptr_t llKirbyMainMotionftKirbyAttack100Effect;
uintptr_t llMVCommonFileID;
uintptr_t llMVOpeningCommonFileID;
uintptr_t llMVOpeningRoomTransitionFileID;
uintptr_t llMVOpeningRoomScene1FileID;
uintptr_t llMVOpeningRoomScene2FileID;
uintptr_t llMVOpeningRoomScene3FileID;
uintptr_t llMVOpeningRoomScene4FileID;
uintptr_t llMVOpeningRunFileID;
uintptr_t llMVOpeningYamabukiFileID;
uintptr_t llMVOpeningSectorFileID;
uintptr_t llMVOpeningRunCrashFileID;
uintptr_t llMVOpeningRoomWallpaperFileID;
uintptr_t llMVOpeningPortraitsSet1FileID;
uintptr_t llMVOpeningPortraitsSet2FileID;
uintptr_t llMVCommonRoomBackgroundDObjDesc;
uintptr_t llMVCommonRoomDeskDObjDesc;
uintptr_t llMVCommonRoomOutsideDisplayList;
uintptr_t llMVCommonRoomHazeDisplayList;
uintptr_t llMVCommonRoomSunlightDisplayList;
uintptr_t llMVCommonRoomPencilsDObjDesc;
uintptr_t llMVCommonRoomPencilsAnimJoint;
uintptr_t llMVCommonRoomLogoDObjDesc;
uintptr_t llMVCommonRoomLogoMObjSub;
uintptr_t llMVCommonRoomLogoMatAnimJoint;
uintptr_t llMVCommonRoomBossShadowDisplayList;
uintptr_t llMVCommonRoomBossShadowAnimJoint;
uintptr_t llMVCommonRoomSpotlightDisplayList;
uintptr_t llMVCommonRoomSpotlightMObjSub;
uintptr_t llMVCommonRoomSpotlightMatAnimJoint;
uintptr_t llMVOpeningRoomTransitionOverlayDisplayList;
uintptr_t llMVOpeningRoomScene1CamAnimJoint;
uintptr_t llMVOpeningRoomScene2CamAnimJoint;
uintptr_t llMVOpeningRoomWallpaperSprite;
uintptr_t llMVOpeningPortraitsSet1SamusSprite;
uintptr_t llMVOpeningPortraitsSet1MarioSprite;
uintptr_t llMVOpeningPortraitsSet1FoxSprite;
uintptr_t llMVOpeningPortraitsSet1PikachuSprite;
uintptr_t llMVOpeningPortraitsSet1CoverSprite;
uintptr_t llMVOpeningPortraitsSet2LinkSprite;
uintptr_t llMVOpeningPortraitsSet2KirbySprite;
uintptr_t llMVOpeningPortraitsSet2DonkeySprite;
uintptr_t llMVOpeningPortraitsSet2YoshiSprite;
uintptr_t llMVOpeningRunWallpaperSprite;
uintptr_t llMVOpeningYamabukiWallpaperSprite;
uintptr_t llMVOpeningSectorCockpitSprite;
uintptr_t llMNTitleFileID;
uintptr_t llMNTitleFireAnimFileID;
uintptr_t llMNTitleLogoAnimCutoutSprite;
uintptr_t llMNTitleLogoAnimStrikeVSprite;
uintptr_t llMNTitleLogoAnimStrikeHSprite;
uintptr_t llMNTitleLogoAnimFullSprite;
uintptr_t llMNTitleBorderUpperSprite;
uintptr_t llMNTitleTMSprite;
uintptr_t llMNTitleCutoutSprite;
uintptr_t llMNTitleTMUnkSprite;
uintptr_t llMNTitleCopyrightSprite;
uintptr_t llMNTitlePressStartSprite;
uintptr_t llMNTitleSuperSprite;
uintptr_t llMNTitleSmashSprite;
uintptr_t llMNTitleBrosSprite;
uintptr_t llMNTitleFireAnimFrame1Sprite;
uintptr_t llMNTitleFireAnimFrame2Sprite;
uintptr_t llMNTitleFireAnimFrame3Sprite;
uintptr_t llMNTitleFireAnimFrame4Sprite;
uintptr_t llMNTitleFireAnimFrame5Sprite;
uintptr_t llMNTitleFireAnimFrame6Sprite;
uintptr_t llMNTitleFireAnimFrame7Sprite;
uintptr_t llMNTitleFireAnimFrame8Sprite;
uintptr_t llMNTitleFireAnimFrame9Sprite;
uintptr_t llMNTitleFireAnimFrame10Sprite;
uintptr_t llMNTitleFireAnimFrame11Sprite;
uintptr_t llMNTitleFireAnimFrame12Sprite;
uintptr_t llMNTitleFireAnimFrame13Sprite;
uintptr_t llMNTitleFireAnimFrame14Sprite;
uintptr_t llMNTitleFireAnimFrame15Sprite;
uintptr_t llMNTitleFireAnimFrame16Sprite;
uintptr_t llMNTitleFireAnimFrame17Sprite;
uintptr_t llMNTitleFireAnimFrame18Sprite;
uintptr_t llMNTitleFireAnimFrame19Sprite;
uintptr_t llMNTitleFireAnimFrame20Sprite;
uintptr_t llMNTitleFireAnimFrame21Sprite;
uintptr_t llMNTitleFireAnimFrame22Sprite;
uintptr_t llMNTitleFireAnimFrame23Sprite;
uintptr_t llMNTitleFireAnimFrame24Sprite;
uintptr_t llMNTitleFireAnimFrame25Sprite;
uintptr_t llMNTitleFireAnimFrame26Sprite;
uintptr_t llMNTitleFireAnimFrame27Sprite;
uintptr_t llMNTitleFireAnimFrame28Sprite;
uintptr_t llMNTitleFireAnimFrame29Sprite;
uintptr_t llMNTitleFireAnimFrame30Sprite;
uintptr_t llMNCommonFileID;
uintptr_t llMNVSModeFileID;
uintptr_t llMNCommonOptionTabLeftSprite;
uintptr_t llMNCommonOptionTabMiddleSprite;
uintptr_t llMNCommonOptionTabRightSprite;
uintptr_t llMNCommonFrameSprite;
uintptr_t llMNCommonGameModeTextSprite;
uintptr_t llMNCommonDigit0Sprite;
uintptr_t llMNCommonDigit1Sprite;
uintptr_t llMNCommonDigit2Sprite;
uintptr_t llMNCommonDigit3Sprite;
uintptr_t llMNCommonDigit4Sprite;
uintptr_t llMNCommonDigit5Sprite;
uintptr_t llMNCommonDigit6Sprite;
uintptr_t llMNCommonDigit7Sprite;
uintptr_t llMNCommonDigit8Sprite;
uintptr_t llMNCommonDigit9Sprite;
uintptr_t llMNCommonInfinitySprite;
uintptr_t llMNCommonArrowRSprite;
uintptr_t llMNCommonArrowLSprite;
uintptr_t llMNCommonDecalPaperSprite;
uintptr_t llMNCommonSmashLogoSprite;
uintptr_t llMNCommonSmashBrosCollageSprite;
uintptr_t llMNVSModeVSStartTextSprite;
uintptr_t llMNVSModeRulePeriodTextSprite;
uintptr_t llMNVSModeTimeTextSprite;
uintptr_t llMNVSModeStockTextSprite;
uintptr_t llMNVSModeTeamTextSprite;
uintptr_t llMNVSModeTimePeriodTextSprite;
uintptr_t llMNVSModeMinTextSprite;
uintptr_t llMNVSModeStockPeriodTextSprite;
uintptr_t llMNVSModeVSOptionsTextSprite;
uintptr_t llMNVSModeConsoleIconDarkSprite;
uintptr_t llMNVSModeVSTextSprite;
uintptr_t llMNTitleLogoDObjDesc;
uintptr_t llMNTitleLogoAnimJoint;
uintptr_t llMNTitleLabelsDObjDesc;
uintptr_t llMNTitleLabelsAnimJoint;
uintptr_t llMNTitlePressStartDObjDesc;
uintptr_t llMNTitlePressStartAnimJoint;
uintptr_t llMNTitleSlashDObjDesc;
uintptr_t llMNTitleSlashMObjSub;
uintptr_t llMNTitleSlashAnimJoint;
uintptr_t llMNTitleSlashMatAnimJoint;
uintptr_t llMNTitleFireDObjDesc;
uintptr_t llMNTitleFireAnimJoint;
uintptr_t lMNTitleParticleScriptBankLo;
uintptr_t lMNTitleParticleScriptBankHi;
uintptr_t lMNTitleParticleTextureBankLo;
uintptr_t lMNTitleParticleTextureBankHi;
intptr_t lGRPupupuParticleScriptBankLo;
intptr_t lGRPupupuParticleScriptBankHi;
intptr_t lGRPupupuParticleTextureBankLo;
intptr_t lGRPupupuParticleTextureBankHi;
uintptr_t llIFCommonAnnounceCommonLetterASprite;
uintptr_t llIFCommonAnnounceCommonLetterBSprite;
uintptr_t llIFCommonAnnounceCommonLetterCSprite;
uintptr_t llIFCommonAnnounceCommonLetterDSprite;
uintptr_t llIFCommonAnnounceCommonLetterESprite;
uintptr_t llIFCommonAnnounceCommonLetterFSprite;
uintptr_t llIFCommonAnnounceCommonLetterGSprite;
uintptr_t llIFCommonAnnounceCommonLetterHSprite;
uintptr_t llIFCommonAnnounceCommonLetterISprite;
uintptr_t llIFCommonAnnounceCommonLetterKSprite;
uintptr_t llIFCommonAnnounceCommonLetterLSprite;
uintptr_t llIFCommonAnnounceCommonLetterMSprite;
uintptr_t llIFCommonAnnounceCommonLetterNSprite;
uintptr_t llIFCommonAnnounceCommonLetterOSprite;
uintptr_t llIFCommonAnnounceCommonLetterPSprite;
uintptr_t llIFCommonAnnounceCommonLetterRSprite;
uintptr_t llIFCommonAnnounceCommonLetterSSprite;
uintptr_t llIFCommonAnnounceCommonLetterUSprite;
uintptr_t llIFCommonAnnounceCommonLetterXSprite;
uintptr_t llIFCommonAnnounceCommonLetterYSprite;

#define NDS_DEFINE_IFCOMMON_RELOC_SYMBOL(asset, name, value) uintptr_t name = value;
NDS_IFCOMMON_RELOC_SYMBOLS(NDS_DEFINE_IFCOMMON_RELOC_SYMBOL)
#undef NDS_DEFINE_IFCOMMON_RELOC_SYMBOL

#define NDS_DEFINE_VS_RESULTS_RELOC_SYMBOL(asset, name, value) uintptr_t name = value;
NDS_VS_RESULTS_RELOC_SYMBOLS(NDS_DEFINE_VS_RESULTS_RELOC_SYMBOL)
#undef NDS_DEFINE_VS_RESULTS_RELOC_SYMBOL

#define NDS_DEFINE_TRANSITION_RELOC_SYMBOL(asset, name, value) uintptr_t name = value;
NDS_TRANSITION_RELOC_SYMBOLS(NDS_DEFINE_TRANSITION_RELOC_SYMBOL)
#undef NDS_DEFINE_TRANSITION_RELOC_SYMBOL

uintptr_t llBonusPicturePlatformFileID = 0xeu;
#define NDS_DEFINE_BONUS_PICTURE_PLATFORM_RELOC_SYMBOL(asset, name, value) uintptr_t name = value;
NDS_BONUS_PICTURE_PLATFORM_RELOC_SYMBOLS(NDS_DEFINE_BONUS_PICTURE_PLATFORM_RELOC_SYMBOL)
#undef NDS_DEFINE_BONUS_PICTURE_PLATFORM_RELOC_SYMBOL

uintptr_t llBonusPictureFileID = 0xdu;
#define NDS_DEFINE_BONUS_PICTURE_RELOC_SYMBOL(asset, name, value) uintptr_t name = value;
NDS_BONUS_PICTURE_RELOC_SYMBOLS(NDS_DEFINE_BONUS_PICTURE_RELOC_SYMBOL)
#undef NDS_DEFINE_BONUS_PICTURE_RELOC_SYMBOL

uintptr_t llCharacterNamesFileID = 0xcu;
#define NDS_DEFINE_CHARACTER_NAMES_RELOC_SYMBOL(asset, name, value) uintptr_t name = value;
NDS_CHARACTER_NAMES_RELOC_SYMBOLS(NDS_DEFINE_CHARACTER_NAMES_RELOC_SYMBOL)
#undef NDS_DEFINE_CHARACTER_NAMES_RELOC_SYMBOL

uintptr_t llMNPlayersDifficultyFileID = 0x18u;
#define NDS_DEFINE_MN_PLAYERS_DIFFICULTY_RELOC_SYMBOL(asset, name, value) uintptr_t name = value;
NDS_MN_PLAYERS_DIFFICULTY_RELOC_SYMBOLS(NDS_DEFINE_MN_PLAYERS_DIFFICULTY_RELOC_SYMBOL)
#undef NDS_DEFINE_MN_PLAYERS_DIFFICULTY_RELOC_SYMBOL

#define NDS_DEFINE_IF_COMMON_TIMER_EXTRA_RELOC_SYMBOL(asset, name, value) uintptr_t name = value;
NDS_IF_COMMON_TIMER_EXTRA_RELOC_SYMBOLS(NDS_DEFINE_IF_COMMON_TIMER_EXTRA_RELOC_SYMBOL)
#undef NDS_DEFINE_IF_COMMON_TIMER_EXTRA_RELOC_SYMBOL

#define NDS_DEFINE_IF_COMMON_DIGITS_EXTRA_RELOC_SYMBOL(asset, name, value) uintptr_t name = value;
NDS_IF_COMMON_DIGITS_EXTRA_RELOC_SYMBOLS(NDS_DEFINE_IF_COMMON_DIGITS_EXTRA_RELOC_SYMBOL)
#undef NDS_DEFINE_IF_COMMON_DIGITS_EXTRA_RELOC_SYMBOL

uintptr_t llSCExplainMainFileID = 0xfcu;
#define NDS_DEFINE_SC_EXPLAIN_MAIN_RELOC_SYMBOL(asset, name, value) uintptr_t name = value;
NDS_SC_EXPLAIN_MAIN_RELOC_SYMBOLS(NDS_DEFINE_SC_EXPLAIN_MAIN_RELOC_SYMBOL)
#undef NDS_DEFINE_SC_EXPLAIN_MAIN_RELOC_SYMBOL

uintptr_t llSCExplainGraphicsFileID = 0xc6u;
#define NDS_DEFINE_SC_EXPLAIN_GRAPHICS_RELOC_SYMBOL(asset, name, value) uintptr_t name = value;
NDS_SC_EXPLAIN_GRAPHICS_RELOC_SYMBOLS(NDS_DEFINE_SC_EXPLAIN_GRAPHICS_RELOC_SYMBOL)
#undef NDS_DEFINE_SC_EXPLAIN_GRAPHICS_RELOC_SYMBOL

uintptr_t llGRWallpaperTrainingYellowFileID = 0x1bu;
#define NDS_DEFINE_GR_WALLPAPER_TRAINING_YELLOW_RELOC_SYMBOL(asset, name, value) uintptr_t name = value;
NDS_GR_WALLPAPER_TRAINING_YELLOW_RELOC_SYMBOLS(NDS_DEFINE_GR_WALLPAPER_TRAINING_YELLOW_RELOC_SYMBOL)
#undef NDS_DEFINE_GR_WALLPAPER_TRAINING_YELLOW_RELOC_SYMBOL

uintptr_t llGRWallpaperTrainingBlueFileID = 0x1cu;
#define NDS_DEFINE_GR_WALLPAPER_TRAINING_BLUE_RELOC_SYMBOL(asset, name, value) uintptr_t name = value;
NDS_GR_WALLPAPER_TRAINING_BLUE_RELOC_SYMBOLS(NDS_DEFINE_GR_WALLPAPER_TRAINING_BLUE_RELOC_SYMBOL)
#undef NDS_DEFINE_GR_WALLPAPER_TRAINING_BLUE_RELOC_SYMBOL

uintptr_t llGRWallpaperTrainingBlackFileID = 0x1au;
#define NDS_DEFINE_GR_WALLPAPER_TRAINING_BLACK_RELOC_SYMBOL(asset, name, value) uintptr_t name = value;
NDS_GR_WALLPAPER_TRAINING_BLACK_RELOC_SYMBOLS(NDS_DEFINE_GR_WALLPAPER_TRAINING_BLACK_RELOC_SYMBOL)
#undef NDS_DEFINE_GR_WALLPAPER_TRAINING_BLACK_RELOC_SYMBOL

uintptr_t llSC1PTrainingModeFileID = 0xfeu;
#define NDS_DEFINE_SC1P_TRAINING_MODE_RELOC_SYMBOL(asset, name, value) uintptr_t name = value;
NDS_SC1P_TRAINING_MODE_RELOC_SYMBOLS(NDS_DEFINE_SC1P_TRAINING_MODE_RELOC_SYMBOL)
#undef NDS_DEFINE_SC1P_TRAINING_MODE_RELOC_SYMBOL

uintptr_t llMNSoundTestFileID = 0xc4u;
#define NDS_DEFINE_MN_SOUND_TEST_RELOC_SYMBOL(asset, name, value) uintptr_t name = value;
NDS_MN_SOUND_TEST_RELOC_SYMBOLS(NDS_DEFINE_MN_SOUND_TEST_RELOC_SYMBOL)
#undef NDS_DEFINE_MN_SOUND_TEST_RELOC_SYMBOL

uintptr_t llMNBackupClearHeaderOptionFileID = 0x4eu;
#define NDS_DEFINE_MN_BACKUP_CLEAR_HEADER_OPTION_RELOC_SYMBOL(asset, name, value) uintptr_t name = value;
NDS_MN_BACKUP_CLEAR_HEADER_OPTION_RELOC_SYMBOLS(NDS_DEFINE_MN_BACKUP_CLEAR_HEADER_OPTION_RELOC_SYMBOL)
#undef NDS_DEFINE_MN_BACKUP_CLEAR_HEADER_OPTION_RELOC_SYMBOL

uintptr_t llMNBackupClearFileID = 0x4du;
#define NDS_DEFINE_MN_BACKUP_CLEAR_RELOC_SYMBOL(asset, name, value) uintptr_t name = value;
NDS_MN_BACKUP_CLEAR_RELOC_SYMBOLS(NDS_DEFINE_MN_BACKUP_CLEAR_RELOC_SYMBOL)
#undef NDS_DEFINE_MN_BACKUP_CLEAR_RELOC_SYMBOL

uintptr_t llMNOptionFileID = 0x4u;
#define NDS_DEFINE_MN_OPTION_RELOC_SYMBOL(asset, name, value) uintptr_t name = value;
NDS_MN_OPTION_RELOC_SYMBOLS(NDS_DEFINE_MN_OPTION_RELOC_SYMBOL)
#undef NDS_DEFINE_MN_OPTION_RELOC_SYMBOL

uintptr_t llMNCharactersFileID = 0x10u;
#define NDS_DEFINE_MN_CHARACTERS_RELOC_SYMBOL(asset, name, value) uintptr_t name = value;
NDS_MN_CHARACTERS_RELOC_SYMBOLS(NDS_DEFINE_MN_CHARACTERS_RELOC_SYMBOL)
#undef NDS_DEFINE_MN_CHARACTERS_RELOC_SYMBOL

uintptr_t llMNVSRecordMainFileID = 0x1fu;
#define NDS_DEFINE_MNVS_RECORD_MAIN_RELOC_SYMBOL(asset, name, value) uintptr_t name = value;
NDS_MNVS_RECORD_MAIN_RELOC_SYMBOLS(NDS_DEFINE_MNVS_RECORD_MAIN_RELOC_SYMBOL)
#undef NDS_DEFINE_MNVS_RECORD_MAIN_RELOC_SYMBOL

uintptr_t llMNDataCommonFileID = 0x20u;
#define NDS_DEFINE_MN_DATA_COMMON_RELOC_SYMBOL(asset, name, value) uintptr_t name = value;
NDS_MN_DATA_COMMON_RELOC_SYMBOLS(NDS_DEFINE_MN_DATA_COMMON_RELOC_SYMBOL)
#undef NDS_DEFINE_MN_DATA_COMMON_RELOC_SYMBOL

uintptr_t llMNDataFileID = 0x5u;
#define NDS_DEFINE_MN_DATA_RELOC_SYMBOL(asset, name, value) uintptr_t name = value;
NDS_MN_DATA_RELOC_SYMBOLS(NDS_DEFINE_MN_DATA_RELOC_SYMBOL)
#undef NDS_DEFINE_MN_DATA_RELOC_SYMBOL

uintptr_t llMNCongraYoshiTopFileID = 0xadu;
#define NDS_DEFINE_MN_CONGRA_YOSHI_TOP_RELOC_SYMBOL(asset, name, value) uintptr_t name = value;
NDS_MN_CONGRA_YOSHI_TOP_RELOC_SYMBOLS(NDS_DEFINE_MN_CONGRA_YOSHI_TOP_RELOC_SYMBOL)
#undef NDS_DEFINE_MN_CONGRA_YOSHI_TOP_RELOC_SYMBOL

uintptr_t llMNCongraYoshiBottomFileID = 0xacu;
#define NDS_DEFINE_MN_CONGRA_YOSHI_BOTTOM_RELOC_SYMBOL(asset, name, value) uintptr_t name = value;
NDS_MN_CONGRA_YOSHI_BOTTOM_RELOC_SYMBOLS(NDS_DEFINE_MN_CONGRA_YOSHI_BOTTOM_RELOC_SYMBOL)
#undef NDS_DEFINE_MN_CONGRA_YOSHI_BOTTOM_RELOC_SYMBOL

uintptr_t llMNCongraSamusTopFileID = 0xb1u;
#define NDS_DEFINE_MN_CONGRA_SAMUS_TOP_RELOC_SYMBOL(asset, name, value) uintptr_t name = value;
NDS_MN_CONGRA_SAMUS_TOP_RELOC_SYMBOLS(NDS_DEFINE_MN_CONGRA_SAMUS_TOP_RELOC_SYMBOL)
#undef NDS_DEFINE_MN_CONGRA_SAMUS_TOP_RELOC_SYMBOL

uintptr_t llMNCongraSamusBottomFileID = 0xb0u;
#define NDS_DEFINE_MN_CONGRA_SAMUS_BOTTOM_RELOC_SYMBOL(asset, name, value) uintptr_t name = value;
NDS_MN_CONGRA_SAMUS_BOTTOM_RELOC_SYMBOLS(NDS_DEFINE_MN_CONGRA_SAMUS_BOTTOM_RELOC_SYMBOL)
#undef NDS_DEFINE_MN_CONGRA_SAMUS_BOTTOM_RELOC_SYMBOL

uintptr_t llMNCongraPurinTopFileID = 0xb5u;
#define NDS_DEFINE_MN_CONGRA_PURIN_TOP_RELOC_SYMBOL(asset, name, value) uintptr_t name = value;
NDS_MN_CONGRA_PURIN_TOP_RELOC_SYMBOLS(NDS_DEFINE_MN_CONGRA_PURIN_TOP_RELOC_SYMBOL)
#undef NDS_DEFINE_MN_CONGRA_PURIN_TOP_RELOC_SYMBOL

uintptr_t llMNCongraPurinBottomFileID = 0xb4u;
#define NDS_DEFINE_MN_CONGRA_PURIN_BOTTOM_RELOC_SYMBOL(asset, name, value) uintptr_t name = value;
NDS_MN_CONGRA_PURIN_BOTTOM_RELOC_SYMBOLS(NDS_DEFINE_MN_CONGRA_PURIN_BOTTOM_RELOC_SYMBOL)
#undef NDS_DEFINE_MN_CONGRA_PURIN_BOTTOM_RELOC_SYMBOL

uintptr_t llMNCongraPikachuTopFileID = 0xafu;
#define NDS_DEFINE_MN_CONGRA_PIKACHU_TOP_RELOC_SYMBOL(asset, name, value) uintptr_t name = value;
NDS_MN_CONGRA_PIKACHU_TOP_RELOC_SYMBOLS(NDS_DEFINE_MN_CONGRA_PIKACHU_TOP_RELOC_SYMBOL)
#undef NDS_DEFINE_MN_CONGRA_PIKACHU_TOP_RELOC_SYMBOL

uintptr_t llMNCongraPikachuBottomFileID = 0xaeu;
#define NDS_DEFINE_MN_CONGRA_PIKACHU_BOTTOM_RELOC_SYMBOL(asset, name, value) uintptr_t name = value;
NDS_MN_CONGRA_PIKACHU_BOTTOM_RELOC_SYMBOLS(NDS_DEFINE_MN_CONGRA_PIKACHU_BOTTOM_RELOC_SYMBOL)
#undef NDS_DEFINE_MN_CONGRA_PIKACHU_BOTTOM_RELOC_SYMBOL

uintptr_t llMNCongraNessTopFileID = 0xc1u;
#define NDS_DEFINE_MN_CONGRA_NESS_TOP_RELOC_SYMBOL(asset, name, value) uintptr_t name = value;
NDS_MN_CONGRA_NESS_TOP_RELOC_SYMBOLS(NDS_DEFINE_MN_CONGRA_NESS_TOP_RELOC_SYMBOL)
#undef NDS_DEFINE_MN_CONGRA_NESS_TOP_RELOC_SYMBOL

uintptr_t llMNCongraNessBottomFileID = 0xc0u;
#define NDS_DEFINE_MN_CONGRA_NESS_BOTTOM_RELOC_SYMBOL(asset, name, value) uintptr_t name = value;
NDS_MN_CONGRA_NESS_BOTTOM_RELOC_SYMBOLS(NDS_DEFINE_MN_CONGRA_NESS_BOTTOM_RELOC_SYMBOL)
#undef NDS_DEFINE_MN_CONGRA_NESS_BOTTOM_RELOC_SYMBOL

uintptr_t llMNCongraMarioTopFileID = 0xbbu;
#define NDS_DEFINE_MN_CONGRA_MARIO_TOP_RELOC_SYMBOL(asset, name, value) uintptr_t name = value;
NDS_MN_CONGRA_MARIO_TOP_RELOC_SYMBOLS(NDS_DEFINE_MN_CONGRA_MARIO_TOP_RELOC_SYMBOL)
#undef NDS_DEFINE_MN_CONGRA_MARIO_TOP_RELOC_SYMBOL

uintptr_t llMNCongraMarioBottomFileID = 0xbau;
#define NDS_DEFINE_MN_CONGRA_MARIO_BOTTOM_RELOC_SYMBOL(asset, name, value) uintptr_t name = value;
NDS_MN_CONGRA_MARIO_BOTTOM_RELOC_SYMBOLS(NDS_DEFINE_MN_CONGRA_MARIO_BOTTOM_RELOC_SYMBOL)
#undef NDS_DEFINE_MN_CONGRA_MARIO_BOTTOM_RELOC_SYMBOL

uintptr_t llMNCongraLuigiTopFileID = 0xbdu;
#define NDS_DEFINE_MN_CONGRA_LUIGI_TOP_RELOC_SYMBOL(asset, name, value) uintptr_t name = value;
NDS_MN_CONGRA_LUIGI_TOP_RELOC_SYMBOLS(NDS_DEFINE_MN_CONGRA_LUIGI_TOP_RELOC_SYMBOL)
#undef NDS_DEFINE_MN_CONGRA_LUIGI_TOP_RELOC_SYMBOL

uintptr_t llMNCongraLuigiBottomFileID = 0xbcu;
#define NDS_DEFINE_MN_CONGRA_LUIGI_BOTTOM_RELOC_SYMBOL(asset, name, value) uintptr_t name = value;
NDS_MN_CONGRA_LUIGI_BOTTOM_RELOC_SYMBOLS(NDS_DEFINE_MN_CONGRA_LUIGI_BOTTOM_RELOC_SYMBOL)
#undef NDS_DEFINE_MN_CONGRA_LUIGI_BOTTOM_RELOC_SYMBOL

uintptr_t llMNCongraLinkTopFileID = 0xb3u;
#define NDS_DEFINE_MN_CONGRA_LINK_TOP_RELOC_SYMBOL(asset, name, value) uintptr_t name = value;
NDS_MN_CONGRA_LINK_TOP_RELOC_SYMBOLS(NDS_DEFINE_MN_CONGRA_LINK_TOP_RELOC_SYMBOL)
#undef NDS_DEFINE_MN_CONGRA_LINK_TOP_RELOC_SYMBOL

uintptr_t llMNCongraLinkBottomFileID = 0xb2u;
#define NDS_DEFINE_MN_CONGRA_LINK_BOTTOM_RELOC_SYMBOL(asset, name, value) uintptr_t name = value;
NDS_MN_CONGRA_LINK_BOTTOM_RELOC_SYMBOLS(NDS_DEFINE_MN_CONGRA_LINK_BOTTOM_RELOC_SYMBOL)
#undef NDS_DEFINE_MN_CONGRA_LINK_BOTTOM_RELOC_SYMBOL

uintptr_t llMNCongraKirbyTopFileID = 0xabu;
#define NDS_DEFINE_MN_CONGRA_KIRBY_TOP_RELOC_SYMBOL(asset, name, value) uintptr_t name = value;
NDS_MN_CONGRA_KIRBY_TOP_RELOC_SYMBOLS(NDS_DEFINE_MN_CONGRA_KIRBY_TOP_RELOC_SYMBOL)
#undef NDS_DEFINE_MN_CONGRA_KIRBY_TOP_RELOC_SYMBOL

uintptr_t llMNCongraKirbyBottomFileID = 0xaau;
#define NDS_DEFINE_MN_CONGRA_KIRBY_BOTTOM_RELOC_SYMBOL(asset, name, value) uintptr_t name = value;
NDS_MN_CONGRA_KIRBY_BOTTOM_RELOC_SYMBOLS(NDS_DEFINE_MN_CONGRA_KIRBY_BOTTOM_RELOC_SYMBOL)
#undef NDS_DEFINE_MN_CONGRA_KIRBY_BOTTOM_RELOC_SYMBOL

uintptr_t llMNCongraFoxTopFileID = 0xbfu;
#define NDS_DEFINE_MN_CONGRA_FOX_TOP_RELOC_SYMBOL(asset, name, value) uintptr_t name = value;
NDS_MN_CONGRA_FOX_TOP_RELOC_SYMBOLS(NDS_DEFINE_MN_CONGRA_FOX_TOP_RELOC_SYMBOL)
#undef NDS_DEFINE_MN_CONGRA_FOX_TOP_RELOC_SYMBOL

uintptr_t llMNCongraFoxBottomFileID = 0xbeu;
#define NDS_DEFINE_MN_CONGRA_FOX_BOTTOM_RELOC_SYMBOL(asset, name, value) uintptr_t name = value;
NDS_MN_CONGRA_FOX_BOTTOM_RELOC_SYMBOLS(NDS_DEFINE_MN_CONGRA_FOX_BOTTOM_RELOC_SYMBOL)
#undef NDS_DEFINE_MN_CONGRA_FOX_BOTTOM_RELOC_SYMBOL

uintptr_t llMNCongraDonkeyTopFileID = 0xb9u;
#define NDS_DEFINE_MN_CONGRA_DONKEY_TOP_RELOC_SYMBOL(asset, name, value) uintptr_t name = value;
NDS_MN_CONGRA_DONKEY_TOP_RELOC_SYMBOLS(NDS_DEFINE_MN_CONGRA_DONKEY_TOP_RELOC_SYMBOL)
#undef NDS_DEFINE_MN_CONGRA_DONKEY_TOP_RELOC_SYMBOL

uintptr_t llMNCongraDonkeyBottomFileID = 0xb8u;
#define NDS_DEFINE_MN_CONGRA_DONKEY_BOTTOM_RELOC_SYMBOL(asset, name, value) uintptr_t name = value;
NDS_MN_CONGRA_DONKEY_BOTTOM_RELOC_SYMBOLS(NDS_DEFINE_MN_CONGRA_DONKEY_BOTTOM_RELOC_SYMBOL)
#undef NDS_DEFINE_MN_CONGRA_DONKEY_BOTTOM_RELOC_SYMBOL

uintptr_t llMNCongraCaptainTopFileID = 0xb7u;
#define NDS_DEFINE_MN_CONGRA_CAPTAIN_TOP_RELOC_SYMBOL(asset, name, value) uintptr_t name = value;
NDS_MN_CONGRA_CAPTAIN_TOP_RELOC_SYMBOLS(NDS_DEFINE_MN_CONGRA_CAPTAIN_TOP_RELOC_SYMBOL)
#undef NDS_DEFINE_MN_CONGRA_CAPTAIN_TOP_RELOC_SYMBOL

uintptr_t llMNCongraCaptainBottomFileID = 0xb6u;
#define NDS_DEFINE_MN_CONGRA_CAPTAIN_BOTTOM_RELOC_SYMBOL(asset, name, value) uintptr_t name = value;
NDS_MN_CONGRA_CAPTAIN_BOTTOM_RELOC_SYMBOLS(NDS_DEFINE_MN_CONGRA_CAPTAIN_BOTTOM_RELOC_SYMBOL)
#undef NDS_DEFINE_MN_CONGRA_CAPTAIN_BOTTOM_RELOC_SYMBOL

uintptr_t llMVEndingFileID = 0x4cu;
#define NDS_DEFINE_MV_ENDING_RELOC_SYMBOL(asset, name, value) uintptr_t name = value;
NDS_MV_ENDING_RELOC_SYMBOLS(NDS_DEFINE_MV_ENDING_RELOC_SYMBOL)
#undef NDS_DEFINE_MV_ENDING_RELOC_SYMBOL

uintptr_t llSCStaffrollFileID = 0xc3u;
#define NDS_DEFINE_SC_STAFFROLL_RELOC_SYMBOL(asset, name, value) uintptr_t name = value;
NDS_SC_STAFFROLL_RELOC_SYMBOLS(NDS_DEFINE_SC_STAFFROLL_RELOC_SYMBOL)
#undef NDS_DEFINE_SC_STAFFROLL_RELOC_SYMBOL

uintptr_t llMNMessageFileID = 0x9u;
#define NDS_DEFINE_MN_MESSAGE_RELOC_SYMBOL(asset, name, value) uintptr_t name = value;
NDS_MN_MESSAGE_RELOC_SYMBOLS(NDS_DEFINE_MN_MESSAGE_RELOC_SYMBOL)
#undef NDS_DEFINE_MN_MESSAGE_RELOC_SYMBOL

uintptr_t llMNPlayers1PModeFileID = 0x17u;
#define NDS_DEFINE_MN_PLAYERS1P_MODE_RELOC_SYMBOL(asset, name, value) uintptr_t name = value;
NDS_MN_PLAYERS1P_MODE_RELOC_SYMBOLS(NDS_DEFINE_MN_PLAYERS1P_MODE_RELOC_SYMBOL)
#undef NDS_DEFINE_MN_PLAYERS1P_MODE_RELOC_SYMBOL

uintptr_t llMN1PContinueFileID = 0x4fu;
#define NDS_DEFINE_MN1P_CONTINUE_RELOC_SYMBOL(asset, name, value) uintptr_t name = value;
NDS_MN1P_CONTINUE_RELOC_SYMBOLS(NDS_DEFINE_MN1P_CONTINUE_RELOC_SYMBOL)
#undef NDS_DEFINE_MN1P_CONTINUE_RELOC_SYMBOL

uintptr_t llMN1PFileID = 0x2u;
#define NDS_DEFINE_MN1P_RELOC_SYMBOL(asset, name, value) uintptr_t name = value;
NDS_MN1P_RELOC_SYMBOLS(NDS_DEFINE_MN1P_RELOC_SYMBOL)
#undef NDS_DEFINE_MN1P_RELOC_SYMBOL

uintptr_t llSC1PIntroFileID = 0xbu;
#define NDS_DEFINE_SC1P_INTRO_RELOC_SYMBOL(asset, name, value) uintptr_t name = value;
NDS_SC1P_INTRO_RELOC_SYMBOLS(NDS_DEFINE_SC1P_INTRO_RELOC_SYMBOL)
#undef NDS_DEFINE_SC1P_INTRO_RELOC_SYMBOL

uintptr_t llSC1PChallengerFileID = 0xau;
#define NDS_DEFINE_SC1P_CHALLENGER_RELOC_SYMBOL(asset, name, value) uintptr_t name = value;
NDS_SC1P_CHALLENGER_RELOC_SYMBOLS(NDS_DEFINE_SC1P_CHALLENGER_RELOC_SYMBOL)
#undef NDS_DEFINE_SC1P_CHALLENGER_RELOC_SYMBOL

uintptr_t llSC1PStageClear3FileID = 0x97u;
#define NDS_DEFINE_SC1P_STAGE_CLEAR3_RELOC_SYMBOL(asset, name, value) uintptr_t name = value;
NDS_SC1P_STAGE_CLEAR3_RELOC_SYMBOLS(NDS_DEFINE_SC1P_STAGE_CLEAR3_RELOC_SYMBOL)
#undef NDS_DEFINE_SC1P_STAGE_CLEAR3_RELOC_SYMBOL

uintptr_t llSC1PStageClear2FileID = 0x51u;
#define NDS_DEFINE_SC1P_STAGE_CLEAR2_RELOC_SYMBOL(asset, name, value) uintptr_t name = value;
NDS_SC1P_STAGE_CLEAR2_RELOC_SYMBOLS(NDS_DEFINE_SC1P_STAGE_CLEAR2_RELOC_SYMBOL)
#undef NDS_DEFINE_SC1P_STAGE_CLEAR2_RELOC_SYMBOL

uintptr_t llSC1PStageClear1FileID = 0x50u;
#define NDS_DEFINE_SC1P_STAGE_CLEAR1_RELOC_SYMBOL(asset, name, value) uintptr_t name = value;
NDS_SC1P_STAGE_CLEAR1_RELOC_SYMBOLS(NDS_DEFINE_SC1P_STAGE_CLEAR1_RELOC_SYMBOL)
#undef NDS_DEFINE_SC1P_STAGE_CLEAR1_RELOC_SYMBOL

uintptr_t llMVOpeningCommonMarioCamAnimJoint;
uintptr_t llMVOpeningCommonDonkeyCamAnimJoint;
uintptr_t llMVOpeningCommonSamusCamAnimJoint;
uintptr_t llMVOpeningCommonFoxCamAnimJoint;
uintptr_t llMVOpeningCommonLinkCamAnimJoint;
uintptr_t llMVOpeningCommonYoshiCamAnimJoint;
uintptr_t llMVOpeningCommonPikachuCamAnimJoint;
uintptr_t llMVOpeningCommonKirbyCamAnimJoint;

#define NDS_DEFINE_MENU_RELOC_SYMBOL(name, value) uintptr_t name = value;
NDS_MENU_RELOC_SYMBOLS(NDS_DEFINE_MENU_RELOC_SYMBOL)
#undef NDS_DEFINE_MENU_RELOC_SYMBOL

extern s32 sMNStartupSkipAllowWait;
extern sb32 sMNStartupIsProceedOpening;
extern void mnStartupActorFuncRun(GObj *gobj);
extern void mnStartupFuncLights(Gfx **dls);
extern void mnTitleFuncUpdate(void);
extern void lbCommonDrawSObjAttr(GObj *gobj);
extern SObj *lbCommonMakeSObjForGObj(GObj *gobj, Sprite *sprite);
extern void lbCommonDrawSprite(GObj *camera_gobj);

/* Original BattleShip taskman startup scene counters. mnStartupFuncStart
 * increments these through the real gcMake and gcAdd and lbCommon paths; the
 * diagnostic snapshot below reads the resulting real object state. */
extern u32 sGCCommonsActiveNum;
extern u32 sGCCamerasActiveNum;
extern u32 sGCSpritesActiveNum;
extern u32 sGCMatrixesActiveNum;
extern u32 sGCAnimsActiveNum;
extern u32 sGCMaterialsActive;
extern u32 sGCDrawsActiveNum;
extern s32 sMVOpeningRoomTotalTimeTics;
extern s32 sMVOpeningPortraitsTotalTimeTics;
extern s32 sMVOpeningMarioTotalTimeTics;
extern GObj *sMVOpeningRoomMainCameraGObj;
extern GObj *sMVOpeningRoomFighterCameraGObj;
extern GObj *sMVOpeningRoomPencilsGObj;
extern void mvOpeningRoomCommonProcUpdate(GObj *gobj);
extern void mvOpeningRoomCloseUpOverlayProcDisplay(GObj *gobj);
extern void mvOpeningRoomLogoWallpaperProcDisplay(GObj *gobj);
extern void mvOpeningRoomWallpaperProcDisplay(GObj *gobj);
extern void gcDrawDObjTreeForGObj(GObj *gobj);
extern void gcDrawDObjTreeDLLinksForGObj(GObj *gobj);
extern void gcDrawDObjDLLinksForGObj(GObj *gobj);
extern void gcDrawDObjDLHead1(GObj *gobj);
extern void gcPlayAnimAll(GObj *gobj);
extern void gcPlayCamAnim(GObj *gobj);
extern void func_80017EC0(GObj *gobj);

/* Original taskman scene-setup reflection. syTaskmanLoadScene populates
 * sSYTaskmanDefaultFunction and the DL/heap/RDP state; the snapshot reads
 * those to prove the real setup ran. SYTaskFunction is a file-local typedef in
 * the imported sys/taskman.c; mirror its layout here so this DS seam can take
 * the same pointer type the original syTaskmanLoadScene passes to
 * syTaskmanRunTask. */
struct SYTaskFunction;
struct SYTaskFunction
{
    u16 flags;
    void (*scene_update)(void);
    void (*task_update)(struct SYTaskFunction*);
    void (*scene_draw)(void);
    void (*task_draw)(struct SYTaskFunction*);
};
extern struct SYTaskFunction sSYTaskmanDefaultFunction;

/* Original taskman general heap (defined in the imported sys/taskman.c). The DS
 * seam reads its usage to prove the real startup allocation path ran. */
extern SYMallocRegion gSYTaskmanGeneralHeap;
extern s32 sSYTaskmanStatus;
extern s32 sSYTaskmanFramebufferID;
extern u32 D_800454BC;
extern s32 D_80046638[2];
extern OSMesgQueue sSYTaskmanContextMesgQueue;
extern OSMesgQueue sSYTaskmanResetMesgQueue;
extern OSMesgQueue sSYTaskmanGameTicMesgQueue;
extern void func_80005BFC(void);

#if NDS_IMPORT_BATTLESHIP_FTMANAGER
#if NDS_R2_BATTLEPACK_KEEP_CACHE
/* See the NDS_R2_BATTLEPACK_KEEP_CACHE block in the Makefile.
 *
 * 0x17a000 (1,548,288) = 0x150000 + 172,032, and the size is MEASURED, not
 * derived from the boot ladder. The first attempt asked for 0x18f000 (+258,048,
 * enough for the blob plus the full 262,144 B cache) on the strength of
 * check-boot-headroom's 319,840 B of "proven headroom". The libnds heap refused
 * it: gNdsTaskmanArenaChosenSize came back 1,564,672 with
 * gNdsTaskmanArenaAllocFailCount 17 -- the step-down loop below gave up 69,632
 * bytes -- while the 550,080 B animation reservation inside it still SUCCEEDED
 * (ReserveFailCount 0). The result booted, passed every allocator guard, and
 * left the general heap with 6,076 free against the mandated 32,768 floor, so
 * the battle scene never started: soak verdict NEVER-STARTED, zero presented
 * battle frames, and the 2,400 s gate run before it never reached ring stop 0.
 *
 * PROVEN GRANTABLE CEILING: 1,564,672. This asks 16,384 under it so the loop
 * does not step at all, and gNdsTaskmanArenaAllocFailCount 0 is the check on
 * that rather than this comment. The headroom ladder cannot see any of this --
 * it meters the STATIC image against a boot threshold, not the heap a runtime
 * calloc can actually be given.
 *
 * PRICED ON THE STRESS BATTERY 2026-08-15, not projected
 * (artifacts/performance/2026-08-15_battlepack-arena-price/ARENA_PRICE.md).
 * 660 s, 12 battle-scene entries, 7 completed matches, 7 START restarts, 4
 * Sudden Deaths, verdict NO-FREEZE: ChosenSize 1,548,288 with AllocFail 0,
 * ReserveFail 0, Rejects 0, SyMallocOverflow 0, general-heap low-water 52,400
 * against the mandated 32,768 floor and the 25,600 ifCommonSetMaxNumGObj latch,
 * sGCCommonsMaxNum still -1. The low-water is FLAT across the chain -- the
 * single-match figure is 52,864 -- because syTaskmanStartTask rewinds the
 * general heap on every scene entry, so Sudden Death's extra per-player
 * figatree heaps do not accumulate across matches.
 *
 * AND THE COUNTERINTUITIVE PART, because "+172,032 of arena" reads like extra
 * room and is not: the growth REPAYS the pack's own reservation. 1,548,288 less
 * the 451,776 the animation arena reserves leaves taskman 1,096,512, against the
 * shipping arm's 1,376,256 - 262,144 = 1,114,112. This arm gives taskman 17,600
 * bytes LESS, which is why its low-water sits below the shipping arm's rather
 * than above it. Size the next change against the RESIDUE, not against the
 * constant on this line.
 *
 * P2-3r13 RAISED IT 0x17a000 -> 0x1a7000 (+184,320), AND THE BYTES WERE BOUGHT
 * BEFORE THEY WERE SPENT. The scene file store -- 185,696 B of ARM9 .bss for
 * the title / opening-action / Peach's-Castle file destinations -- moved into
 * the scene arena (see ndsRelocSceneFileBuffer in reloc_backend_assets.c), so
 * the libnds heap this calloc draws from grew by more than this constant did
 * and the reserve left after the arena is unchanged. That is the invariant to
 * preserve on any future raise: NEVER raise this without returning at least as
 * much static image first, because the step-down loop below cannot tell the
 * difference between "the target was ambitious" and "the heap is exhausted" --
 * it just hands back a smaller arena and only gNdsTaskmanArenaChosenSize and
 * gNdsTaskmanArenaAllocFailCount ever say so.
 *
 * The PROVEN GRANTABLE CEILING above (1,564,672) was measured against the old
 * static image and does not carry across this change; the current ceiling is
 * whatever ChosenSize reports on the build in hand. It is per-BUILD, not
 * per-project: the four-distinct-kind roster's larger ARM9 binary already cost
 * it 36,864 (AllocFail 9) before any of this. */
#define NDS_TASKMAN_ARENA_SIZE 0x1a7000u
#else
#define NDS_TASKMAN_ARENA_SIZE 0x150000u
#endif
#else
#define NDS_TASKMAN_ARENA_SIZE (1024u * 1024u)
#endif
#define NDS_STARTUP_BOUNDED_UPDATES 55u
#define NDS_STARTUP_LOGO_DRAW_UPDATE 17u
#define NDS_STARTUP_PRESENT_INTERVAL 10u
#define NDS_OPENING_ROOM_PRE_ASSET_TICK 279u
#define NDS_OPENING_ROOM_FIRST_EVENT_RUN_TICK 280u
#define NDS_OPENING_ROOM_TICK380_RUN_TICK 380u
#define NDS_OPENING_ROOM_TICK450_RUN_TICK 450u
#define NDS_OPENING_ROOM_TICK500_RUN_TICK 500u
#define NDS_OPENING_ROOM_TICK560_RUN_TICK 560u
#define NDS_OPENING_ROOM_HANDOFF_TICK (22u * 60u)
#define NDS_OPENING_PORTRAITS_HANDOFF_TICK 150u
#define NDS_OPENING_MARIO_HANDOFF_TICK 60u
#define NDS_OPENING_MOVIE_DRAW_INTERVAL 30u
static void *sNdsTaskmanArenaAlloc;
static u8 *sNdsTaskmanArenaBytes;
volatile u32 gNdsTaskmanArenaChosenSize;
volatile u32 gNdsTaskmanArenaAllocFailCount;

#define NDS_OPENING_ROOM_PENCILS_DOBJ_ENTRIES 4u
#define NDS_OPENING_ROOM_PENCILS_RENDER_DOBJS 3u
#define NDS_OPENING_ROOM_PENCILS_ANIM_JOINTS 3u
#define NDS_AOBJ_EVENT32_OPCODE(value) (((value) >> 25) & 0x7fu)

#define NDS_OPENING_ROOM_PENCILS_CREATE_GOBJ_READY (1u << 0)
#define NDS_OPENING_ROOM_PENCILS_CREATE_DOBJ_READY (1u << 1)
#define NDS_OPENING_ROOM_PENCILS_CREATE_XOBJ_READY (1u << 2)
#define NDS_OPENING_ROOM_PENCILS_CREATE_PROCESS_READY (1u << 3)
#define NDS_OPENING_ROOM_PENCILS_CREATE_DISPLAY_READY (1u << 4)
#define NDS_OPENING_ROOM_PENCILS_CREATE_ANIM_ROOT_READY (1u << 5)
#define NDS_OPENING_ROOM_PENCILS_CREATE_READY_MASK \
    (NDS_OPENING_ROOM_PENCILS_CREATE_GOBJ_READY | \
     NDS_OPENING_ROOM_PENCILS_CREATE_DOBJ_READY | \
     NDS_OPENING_ROOM_PENCILS_CREATE_XOBJ_READY | \
     NDS_OPENING_ROOM_PENCILS_CREATE_PROCESS_READY | \
     NDS_OPENING_ROOM_PENCILS_CREATE_DISPLAY_READY | \
     NDS_OPENING_ROOM_PENCILS_CREATE_ANIM_ROOT_READY)
