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

void lbRelocInitSetup(LBRelocSetup *setup);
size_t lbRelocGetFileSize(const void *file_id);
void *lbRelocGetExternHeapFile(const void *file_id, void *heap);
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
