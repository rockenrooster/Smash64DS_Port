void syAudioStopBGMAll(void)
{
}

void syAudioPlayBGM(s32 player, s32 bgm_id)
{
    (void)player;
    (void)bgm_id;
}

void func_800266A0_272A0(void)
{
}

void *func_800269C0_275C0(u16 fgm_id)
{
    (void)fgm_id;
    return NULL;
}

/* scmanager.c excludes these renderer/task wrappers for the NDS target. Keep
 * their original behavior here so imported scenes retain the same entry ABI. */
void scManagerFuncUpdate(SYTaskmanSetup *setup)
{
    syTaskmanStartTask(setup);
}

void scManagerFuncDraw(void)
{
    gcDrawAll();
}

void syAudioSetSettingsUpdated(void)
{
}

sb32 syAudioGetSettingsUpdated(void)
{
    return FALSE;
}

void syAudioSetFXType(u8 type)
{
    (void)type;
}

sb32 syAudioGetRestarting(void)
{
    return FALSE;
}

void ftManagerSetupFileSize(void)
{
}

void ftManagerAllocFighter(u32 data_flags, s32 allocs_num)
{
    (void)data_flags;
    (void)allocs_num;
    if (gFTManagerFigatreeHeapSize == 0)
    {
        gFTManagerFigatreeHeapSize = 0x1000u;
    }
}

void ftManagerSetupFilesAllKind(s32 fkind)
{
    (void)fkind;
}

void *ftManagerAllocFigatreeHeapKind(s32 fkind)
{
    (void)fkind;
    return syTaskmanMalloc(gFTManagerFigatreeHeapSize, 0x10);
}

GObj *ftManagerMakeFighter(FTDesc *desc)
{
    (void)desc;
    return NULL;
}

void ftPublicMakeActor(void)
{
}

void ftParamInitGame(void)
{
}

void ftParamInitPlayerBattleStats(s32 player, GObj *fighter_gobj)
{
    (void)player;
    (void)fighter_gobj;
}

void ftParamSetKey(GObj *fighter_gobj, FTKeyEvent *script)
{
    (void)fighter_gobj;
    (void)script;
}

s32 ftParamGetCostumeCommonID(s32 fkind, s32 color)
{
    (void)fkind;
    return color;
}

s32 ftParamGetCostumeTeamID(s32 fkind, s32 color)
{
    (void)fkind;
    return color;
}

void scSubsysFighterSetStatus(GObj *fighter_gobj, s32 status_id)
{
    (void)fighter_gobj;
    (void)status_id;
}

void efParticleInitAll(void)
{
}

s32 efParticleGetLoadBankID(void *script_lo, void *script_hi,
                            void *texture_lo, void *texture_hi)
{
    (void)script_lo;
    (void)script_hi;
    (void)texture_lo;
    (void)texture_hi;
    return 0;
}

LBGenerator *lbParticleMakeGenerator(s32 bank_id, s32 generator_id)
{
    (void)bank_id;
    (void)generator_id;
    return NULL;
}

void lbParticleDrawTextures(GObj *gobj)
{
    (void)gobj;
}

void mpCollisionInitGroundData(void)
{
}

void gmCameraSetViewportDimensions(s32 ulx, s32 uly, s32 lrx, s32 lry)
{
    (void)ulx;
    (void)uly;
    (void)lrx;
    (void)lry;
}

GObj *gmCameraMakeWallpaperCamera(void)
{
    return NULL;
}

GObj *gmCameraMakeMovieCamera(void (*func_camera)(GObj *))
{
    (void)func_camera;
    return gcMakeCameraGObj(nGCCommonKindSceneCamera,
                            NULL,
                            16,
                            GOBJ_PRIORITY_DEFAULT,
                            func_80017EC0,
                            10,
                            ~0,
                            ~0,
                            FALSE,
                            nGCProcessKindFunc,
                            NULL,
                            1,
                            FALSE);
}

void grWallpaperMakeDecideKind(void)
{
}

void grCommonSetupInitAll(void)
{
}

s32 mpCollisionGetMapObjCountKind(s32 kind)
{
    (void)kind;
    return 1;
}

void mpCollisionGetMapObjIDsKind(s32 kind, s32 *ids)
{
    (void)kind;
    if (ids != NULL)
    {
        ids[0] = 0;
    }
}

void mpCollisionGetMapObjPositionID(s32 id, Vec3f *pos)
{
    (void)id;
    if (pos != NULL)
    {
        pos->x = 0.0F;
        pos->y = 0.0F;
        pos->z = 0.0F;
    }
}

void gmRumbleMakeActor(void)
{
}

void wpManagerAllocWeapons(void)
{
}

void itManagerInitItems(void)
{
}

void efManagerInitEffects(void)
{
}

void lbBackupIsSramValid(void)
{
}

void lbBackupApplyOptions(void)
{
}

void lbBackupWrite(void)
{
}

void syDebugSetFuncPrint(void (*function)(void))
{
    (void)function;
}

void syDebugStartRmonThread5Hang(void)
{
}

void scManagerFuncPrint(void)
{
}

void syRdpSetViewport(Vp *viewport, f32 ulx, f32 uly, f32 lrx, f32 lry)
{
    f32 h;
    f32 v;

    if (viewport == NULL)
    {
        return;
    }

    h = (ulx + lrx) / 2.0F;
    v = (uly + lry) / 2.0F;

    viewport->vp.vscale[0] = (s16)(((s32)((lrx - h) * 4.0F)) & 0xFFFF);
    viewport->vp.vscale[1] = (s16)(((s32)((lry - v) * 4.0F)) & 0xFFFF);
    viewport->vp.vtrans[0] = (s16)(((s32)(h * 4.0F)) & 0xFFFF);
    viewport->vp.vtrans[1] = (s16)(((s32)(v * 4.0F)) & 0xFFFF);
    viewport->vp.vscale[2] = (s16)(0x03FF / 2);
    viewport->vp.vtrans[2] = (s16)(0x03FF / 2);
}

void syRdpSetFuncLights(void (*func_lights)(Gfx **))
{
    (void)func_lights;
}

void syRdpResetSettings(Gfx **dl)
{
    (void)dl;
}

/* Relocation backend.
 *
 * BattleShip call sites pass file IDs as the address of a generated
 * ll...FileID symbol. The generated relocation header is not present in this
 * checkout, so the DS port owns a narrow manifest for the imported slices.
 * The current Opening Room step mirrors the sm64-nds NitroFS pattern: the
 * eight real BattleShip_o2r resources are staged into the ROM, copied into the
 * original task heap, blanket u32 byte-swapped, internally pointer-relocated,
 * and resolved through selected symbol-offset probes. The startup logo Sprite
 * and the current MVCommon logo/spotlight MObjSub slice have narrow
 * mixed-width normalizers; general mixed-width struct fixups, external
 * dependencies, and renderer-safe texture/display-list fixups remain deferred
 * and are reported separately. */
#define NDS_RELOC_OPENING_ROOM_FILE_COUNT 8u
#define NDS_RELOC_OPENING_ROOM_FILE_MASK 0xffu
#define NDS_RELOC_LOADED_FILE_CAPACITY 11u

#define NDS_RELOC_ASSET_INVALID 0xffffffffu
#define NDS_RELOC_ASSET_MN_COMMON 0u
#define NDS_RELOC_ASSET_N64_LOGO 194u
#define NDS_RELOC_ASSET_IF_COMMON_ANNOUNCE 37u
#define NDS_RELOC_ASSET_MV_COMMON 52u
#define NDS_RELOC_ASSET_OPENING_COMMON 65u
#define NDS_RELOC_ASSET_OPENING_ROOM_TRANSITION 63u
#define NDS_RELOC_ASSET_OPENING_ROOM_SCENE1 56u
#define NDS_RELOC_ASSET_OPENING_ROOM_SCENE2 57u
#define NDS_RELOC_ASSET_OPENING_ROOM_SCENE3 58u
#define NDS_RELOC_ASSET_OPENING_ROOM_SCENE4 59u
#define NDS_RELOC_ASSET_OPENING_RUN 55u
#define NDS_RELOC_ASSET_OPENING_YAMABUKI 71u
#define NDS_RELOC_ASSET_OPENING_SECTOR 73u
#define NDS_RELOC_ASSET_OPENING_RUN_CRASH 75u
#define NDS_RELOC_ASSET_OPENING_ROOM_WALLPAPER 90u
#define NDS_RELOC_ASSET_OPENING_PORTRAITS_SET1 53u
#define NDS_RELOC_ASSET_OPENING_PORTRAITS_SET2 54u
#define NDS_RELOC_ASSET_MN_TITLE 167u
#define NDS_RELOC_ASSET_MN_TITLE_FIRE_ANIM 168u
#define NDS_RELOC_ASSET_MN_VS_MODE 6u

#define NDS_RELOC_SYMBOL_MVCOMMON_BACKGROUND_MOBJ 0x042f8u
#define NDS_RELOC_SYMBOL_MVCOMMON_BACKGROUND_DOBJ 0x07e98u
#define NDS_RELOC_SYMBOL_MVCOMMON_DESK_DOBJ 0x08df8u
#define NDS_RELOC_SYMBOL_MVCOMMON_OUTSIDE_DL 0x24200u
#define NDS_RELOC_SYMBOL_MVCOMMON_HAZE_DL 0x098f8u
#define NDS_RELOC_SYMBOL_MVCOMMON_SUNLIGHT_DL 0x24708u
#define NDS_RELOC_SYMBOL_MVCOMMON_PENCILS_DOBJ 0x0aeb8u
#define NDS_RELOC_SYMBOL_MVCOMMON_PENCILS_ANIM 0x0af70u
#define NDS_RELOC_SYMBOL_MVCOMMON_LOGO_MOBJ 0x1bc60u
#define NDS_RELOC_SYMBOL_MVCOMMON_LOGO_DOBJ 0x1c4a8u
#define NDS_RELOC_SYMBOL_MVCOMMON_LOGO_MATANIM 0x1c52cu
#define NDS_RELOC_SYMBOL_MVCOMMON_CLOSEUP_AIR_MOBJ 0x1dca0u
#define NDS_RELOC_SYMBOL_MVCOMMON_CLOSEUP_GROUND_MOBJ 0x1f0f8u
#define NDS_RELOC_SYMBOL_MVCOMMON_DESK_GROUND_MOBJ 0x20480u
#define NDS_RELOC_SYMBOL_MVCOMMON_BOSS_SHADOW_DL 0x1f790u
#define NDS_RELOC_SYMBOL_MVCOMMON_BOSS_SHADOW_ANIM 0x1f924u
#define NDS_RELOC_SYMBOL_MVCOMMON_SPOTLIGHT_MOBJ 0x22c90u
#define NDS_RELOC_SYMBOL_MVCOMMON_SPOTLIGHT_DL 0x22e18u
#define NDS_RELOC_SYMBOL_MVCOMMON_SPOTLIGHT_MATANIM 0x22f10u
#define NDS_RELOC_SYMBOL_OPENING_ROOM_TRANSITION_OVERLAY_DL 0x05a0u
#define NDS_RELOC_SYMBOL_OPENING_ROOM_SCENE1_CAM_ANIM 0x0000u
#define NDS_RELOC_SYMBOL_OPENING_ROOM_SCENE2_CAM_ANIM 0x0000u
#define NDS_RELOC_SYMBOL_OPENING_ROOM_WALLPAPER_SPRITE 0x26c88u
#define NDS_RELOC_SYMBOL_N64_LOGO_SPRITE 0x73c0u
#define NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_A 0x05e0u
#define NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_B 0x09a8u
#define NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_C 0x0d80u
#define NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_D 0x1268u
#define NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_E 0x1628u
#define NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_F 0x1a00u
#define NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_G 0x1f08u
#define NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_H 0x2408u
#define NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_I 0x26b8u
#define NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_K 0x2f98u
#define NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_L 0x3358u
#define NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_M 0x3980u
#define NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_N 0x3e88u
#define NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_O 0x44b0u
#define NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_P 0x4890u
#define NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_R 0x5418u
#define NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_S 0x57f0u
#define NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_U 0x60d8u
#define NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_X 0x7108u
#define NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_Y 0x7608u
#define NDS_RELOC_SYMBOL_OPENING_COMMON_MARIO_CAM_ANIM 0x0000u
#define NDS_RELOC_SYMBOL_OPENING_COMMON_DONKEY_CAM_ANIM 0x0030u
#define NDS_RELOC_SYMBOL_OPENING_COMMON_SAMUS_CAM_ANIM 0x0060u
#define NDS_RELOC_SYMBOL_OPENING_COMMON_FOX_CAM_ANIM 0x0090u
#define NDS_RELOC_SYMBOL_OPENING_COMMON_LINK_CAM_ANIM 0x00c0u
#define NDS_RELOC_SYMBOL_OPENING_COMMON_YOSHI_CAM_ANIM 0x00f0u
#define NDS_RELOC_SYMBOL_OPENING_COMMON_PIKACHU_CAM_ANIM 0x0120u
#define NDS_RELOC_SYMBOL_OPENING_COMMON_KIRBY_CAM_ANIM 0x0150u
#define NDS_RELOC_SYMBOL_OPENING_PORTRAITS_SET1_SAMUS 0x09960u
#define NDS_RELOC_SYMBOL_OPENING_PORTRAITS_SET1_MARIO 0x13310u
#define NDS_RELOC_SYMBOL_OPENING_PORTRAITS_SET1_FOX 0x1ccc0u
#define NDS_RELOC_SYMBOL_OPENING_PORTRAITS_SET1_PIKACHU 0x26670u
#define NDS_RELOC_SYMBOL_OPENING_PORTRAITS_SET1_COVER 0x2b2d0u
#define NDS_RELOC_SYMBOL_OPENING_PORTRAITS_SET2_LINK 0x09960u
#define NDS_RELOC_SYMBOL_OPENING_PORTRAITS_SET2_KIRBY 0x13310u
#define NDS_RELOC_SYMBOL_OPENING_PORTRAITS_SET2_DONKEY 0x1ccc0u
#define NDS_RELOC_SYMBOL_OPENING_PORTRAITS_SET2_YOSHI 0x26670u
#define NDS_RELOC_SYMBOL_OPENING_RUN_WALLPAPER 0x058a0u
#define NDS_RELOC_SYMBOL_OPENING_YAMABUKI_WALLPAPER 0x3ee58u
#define NDS_RELOC_SYMBOL_OPENING_SECTOR_COCKPIT 0x3cc90u
#define NDS_RELOC_SYMBOL_TITLE_LOGO_FULL 0x0bbb0u
#define NDS_RELOC_SYMBOL_TITLE_BORDER_UPPER 0x0c208u
#define NDS_RELOC_SYMBOL_TITLE_TM 0x0f398u
#define NDS_RELOC_SYMBOL_TITLE_CUTOUT 0x11988u
#define NDS_RELOC_SYMBOL_TITLE_TM_UNK 0x11aa8u
#define NDS_RELOC_SYMBOL_TITLE_COPYRIGHT 0x15320u
#define NDS_RELOC_SYMBOL_TITLE_PRESS_START 0x15a48u
#define NDS_RELOC_SYMBOL_TITLE_SUPER 0x16728u
#define NDS_RELOC_SYMBOL_TITLE_SMASH 0x245c8u
#define NDS_RELOC_SYMBOL_TITLE_BROS 0x25188u
#define NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME1 0x01018u
#define NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME2 0x02078u
#define NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME3 0x030d8u
#define NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME4 0x04138u
#define NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME5 0x05198u
#define NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME6 0x061f8u
#define NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME7 0x07258u
#define NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME8 0x082b8u
#define NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME9 0x09318u
#define NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME10 0x0a378u
#define NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME11 0x0b3d8u
#define NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME12 0x0c438u
#define NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME13 0x0d498u
#define NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME14 0x0e4f8u
#define NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME15 0x0f558u
#define NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME16 0x105b8u
#define NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME17 0x11618u
#define NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME18 0x12678u
#define NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME19 0x136d8u
#define NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME20 0x14738u
#define NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME21 0x15798u
#define NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME22 0x167f8u
#define NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME23 0x17858u
#define NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME24 0x188b8u
#define NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME25 0x19918u
#define NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME26 0x1a978u
#define NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME27 0x1b9d8u
#define NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME28 0x1ca38u
#define NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME29 0x1da98u
#define NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME30 0x1eaf8u
#define NDS_RELOC_SYMBOL_MNCOMMON_OPTION_TAB_LEFT 0x01e8u
#define NDS_RELOC_SYMBOL_MNCOMMON_OPTION_TAB_MIDDLE 0x0330u
#define NDS_RELOC_SYMBOL_MNCOMMON_OPTION_TAB_RIGHT 0x0568u
#define NDS_RELOC_SYMBOL_MNCOMMON_FRAME 0x1420u
#define NDS_RELOC_SYMBOL_MNCOMMON_DECAL_PAPER 0x2a30u
#define NDS_RELOC_SYMBOL_MNCOMMON_SMASH_LOGO 0x31f8u
#define NDS_RELOC_SYMBOL_MNCOMMON_GAME_MODE_TEXT 0xd240u
#define NDS_RELOC_SYMBOL_MNCOMMON_DIGIT0 0xd310u
#define NDS_RELOC_SYMBOL_MNCOMMON_DIGIT1 0xd3e0u
#define NDS_RELOC_SYMBOL_MNCOMMON_DIGIT2 0xd4b0u
#define NDS_RELOC_SYMBOL_MNCOMMON_DIGIT3 0xd580u
#define NDS_RELOC_SYMBOL_MNCOMMON_DIGIT4 0xd650u
#define NDS_RELOC_SYMBOL_MNCOMMON_DIGIT5 0xd720u
#define NDS_RELOC_SYMBOL_MNCOMMON_DIGIT6 0xd7f0u
#define NDS_RELOC_SYMBOL_MNCOMMON_DIGIT7 0xd8c0u
#define NDS_RELOC_SYMBOL_MNCOMMON_DIGIT8 0xd990u
#define NDS_RELOC_SYMBOL_MNCOMMON_DIGIT9 0xda60u
#define NDS_RELOC_SYMBOL_MNCOMMON_INFINITY 0xdc48u
#define NDS_RELOC_SYMBOL_MNCOMMON_ARROW_R 0xdd90u
#define NDS_RELOC_SYMBOL_MNCOMMON_ARROW_L 0xde30u
#define NDS_RELOC_SYMBOL_MNCOMMON_SMASH_BROS_COLLAGE 0x18000u
#define NDS_RELOC_SYMBOL_MNVSMODE_VS_START_TEXT 0x24c8u
#define NDS_RELOC_SYMBOL_MNVSMODE_RULE_PERIOD_TEXT 0x2748u
#define NDS_RELOC_SYMBOL_MNVSMODE_TIME_TEXT 0x28e0u
#define NDS_RELOC_SYMBOL_MNVSMODE_STOCK_TEXT 0x2a80u
#define NDS_RELOC_SYMBOL_MNVSMODE_TEAM_TEXT 0x2c20u
#define NDS_RELOC_SYMBOL_MNVSMODE_TIME_PERIOD_TEXT 0x2ec8u
#define NDS_RELOC_SYMBOL_MNVSMODE_MIN_TEXT 0x2fc8u
#define NDS_RELOC_SYMBOL_MNVSMODE_STOCK_PERIOD_TEXT 0x3248u
#define NDS_RELOC_SYMBOL_MNVSMODE_VS_OPTIONS_TEXT 0x3828u
#define NDS_RELOC_SYMBOL_MNVSMODE_CONSOLE_ICON_DARK 0x5eb0u
#define NDS_RELOC_SYMBOL_MNVSMODE_VS_TEXT 0x6118u

#define NDS_OPENING_PORTRAITS_CARD_WIDTH 300u
#define NDS_OPENING_PORTRAITS_CARD_HEIGHT 55u
#define NDS_OPENING_PORTRAITS_COVER_WIDTH 656u
#define NDS_OPENING_PORTRAITS_COVER_HEIGHT 55u
#define NDS_IF_ANNOUNCE_LETTER_MAX_WIDTH 96u
#define NDS_IF_ANNOUNCE_LETTER_MAX_HEIGHT 64u
#define NDS_TITLE_MAX_WIDTH 320u
#define NDS_TITLE_MAX_HEIGHT 240u
#define NDS_TITLE_FILE_BUFFER_SIZE 180000u
#define NDS_OPENING_ACTION_PREVIEW_MAX_WIDTH 320u
#define NDS_OPENING_ACTION_PREVIEW_MAX_HEIGHT 264u
#define NDS_OPENING_ACTION_PREVIEW_SCREEN_WIDTH 320u
#define NDS_OPENING_ACTION_PREVIEW_SCREEN_HEIGHT 240u
#define NDS_OPENING_ACTION_PREVIEW_FILE_BUFFER_SIZE 270000u
#define NDS_OPENING_ACTION_PREVIEW_CACHE_COUNT 3u
#define NDS_OPENING_ACTION_PREVIEW_FRAME_HOLD 36u

#define NDS_RELOC_G_IM_FMT_MAX 4u

#define NDS_OPENING_ROOM_FIRST_EVENT_PENCILS_DOBJ_READY (1u << 0)
#define NDS_OPENING_ROOM_FIRST_EVENT_PENCILS_ANIM_READY (1u << 1)
#define NDS_OPENING_ROOM_FIRST_EVENT_READY_MASK \
    (NDS_OPENING_ROOM_FIRST_EVENT_PENCILS_DOBJ_READY | \
     NDS_OPENING_ROOM_FIRST_EVENT_PENCILS_ANIM_READY)

#define NDS_OPENING_ROOM_FIRST_EVENT_DATA_DOBJ_IDS_READY (1u << 0)
#define NDS_OPENING_ROOM_FIRST_EVENT_DATA_DOBJ_DLS_READY (1u << 1)
#define NDS_OPENING_ROOM_FIRST_EVENT_DATA_ANIM_TABLE_READY (1u << 2)
#define NDS_OPENING_ROOM_FIRST_EVENT_DATA_ANIM_OPCODE_READY (1u << 3)
#define NDS_OPENING_ROOM_FIRST_EVENT_DATA_READY_MASK \
    (NDS_OPENING_ROOM_FIRST_EVENT_DATA_DOBJ_IDS_READY | \
     NDS_OPENING_ROOM_FIRST_EVENT_DATA_DOBJ_DLS_READY | \
     NDS_OPENING_ROOM_FIRST_EVENT_DATA_ANIM_TABLE_READY | \
     NDS_OPENING_ROOM_FIRST_EVENT_DATA_ANIM_OPCODE_READY)

#define NDS_RELOC_ALIGN(value) (((value) + 0xFu) & ~0xFu)

typedef struct NDSRelocLoadedFile {
    u32 asset_id;
    u32 bit;
    void *data;
    u32 data_size;
    u16 reloc_intern_offset;
    u16 reloc_extern_offset;
} NDSRelocLoadedFile;

typedef struct NDSRelocSymbolProbe {
    const void *marker;
    u32 asset_id;
    u32 offset;
    u32 bit;
} NDSRelocSymbolProbe;

typedef struct NDSTitleSpriteDesc {
    const void *symbol;
    u32 offset;
    s16 center_x;
    s16 center_y;
    u16 width;
    u16 height;
    u8 bmfmt;
    u8 bmsiz;
} NDSTitleSpriteDesc;

typedef struct NDSRelocKnownSymbol {
    const void *symbol;
    u32 offset;
} NDSRelocKnownSymbol;

typedef struct NDSOpeningActionPreviewDesc {
    u32 scene_kind;
    u32 asset_id;
    const void *symbol;
    u32 offset;
    u16 width;
    u16 height;
    u16 bitmap_count;
    u8 bmfmt;
    u8 bmsiz;
    s16 x;
    s16 y;
} NDSOpeningActionPreviewDesc;

typedef struct NDSOpeningActionPreviewCache {
    u32 asset_id;
    u32 offset;
    u32 ready;
    u16 width;
    u16 height;
    u8 bmfmt;
    u8 bmsiz;
    u32 pixel_count;
    u16 pixels[NDS_OPENING_ACTION_PREVIEW_SCREEN_WIDTH *
               NDS_OPENING_ACTION_PREVIEW_SCREEN_HEIGHT];
} NDSOpeningActionPreviewCache;

static NDSRelocLoadedFile sNdsRelocLoadedFiles[NDS_RELOC_LOADED_FILE_CAPACITY];
static u32 sNdsRelocLoadedFileCount;

static u8 sNdsTitleFileBuffer[NDS_TITLE_FILE_BUFFER_SIZE];
static u8 sNdsOpeningActionPreviewFileBuffer[
    NDS_OPENING_ACTION_PREVIEW_FILE_BUFFER_SIZE];
static NDSOpeningActionPreviewCache sNdsOpeningActionPreviewCaches[
    NDS_OPENING_ACTION_PREVIEW_CACHE_COUNT];

static const NDSOpeningActionPreviewDesc sNdsOpeningActionPreviewDescs[] = {
    {
        nSCKindOpeningRun, NDS_RELOC_ASSET_OPENING_RUN,
        &llMVOpeningRunWallpaperSprite,
        NDS_RELOC_SYMBOL_OPENING_RUN_WALLPAPER,
        160, 120, 11, G_IM_FMT_CI, G_IM_SIZ_8b, 48, 40
    },
    {
        nSCKindOpeningCliff, NDS_RELOC_ASSET_OPENING_SECTOR,
        &llMVOpeningSectorCockpitSprite,
        NDS_RELOC_SYMBOL_OPENING_SECTOR_COCKPIT,
        320, 240, 48, G_IM_FMT_RGBA, G_IM_SIZ_16b, 0, 0
    },
    {
        nSCKindOpeningYamabuki, NDS_RELOC_ASSET_OPENING_YAMABUKI,
        &llMVOpeningYamabukiWallpaperSprite,
        NDS_RELOC_SYMBOL_OPENING_YAMABUKI_WALLPAPER,
        320, 264, 53, G_IM_FMT_RGBA, G_IM_SIZ_16b, 0, 0
    },
    {
        nSCKindOpeningJungle, NDS_RELOC_ASSET_OPENING_RUN,
        &llMVOpeningRunWallpaperSprite,
        NDS_RELOC_SYMBOL_OPENING_RUN_WALLPAPER,
        160, 120, 11, G_IM_FMT_CI, G_IM_SIZ_8b, 48, 40
    },
    {
        nSCKindOpeningYoster, NDS_RELOC_ASSET_OPENING_YAMABUKI,
        &llMVOpeningYamabukiWallpaperSprite,
        NDS_RELOC_SYMBOL_OPENING_YAMABUKI_WALLPAPER,
        320, 264, 53, G_IM_FMT_RGBA, G_IM_SIZ_16b, 0, 0
    },
    {
        nSCKindOpeningSector, NDS_RELOC_ASSET_OPENING_SECTOR,
        &llMVOpeningSectorCockpitSprite,
        NDS_RELOC_SYMBOL_OPENING_SECTOR_COCKPIT,
        320, 240, 48, G_IM_FMT_RGBA, G_IM_SIZ_16b, 0, 0
    },
    {
        nSCKindOpeningStandoff, NDS_RELOC_ASSET_OPENING_RUN,
        &llMVOpeningRunWallpaperSprite,
        NDS_RELOC_SYMBOL_OPENING_RUN_WALLPAPER,
        160, 120, 11, G_IM_FMT_CI, G_IM_SIZ_8b, 48, 40
    },
    {
        nSCKindOpeningClash, NDS_RELOC_ASSET_OPENING_SECTOR,
        &llMVOpeningSectorCockpitSprite,
        NDS_RELOC_SYMBOL_OPENING_SECTOR_COCKPIT,
        320, 240, 48, G_IM_FMT_RGBA, G_IM_SIZ_16b, 0, 0
    },
    {
        nSCKindOpeningNewcomers, NDS_RELOC_ASSET_OPENING_YAMABUKI,
        &llMVOpeningYamabukiWallpaperSprite,
        NDS_RELOC_SYMBOL_OPENING_YAMABUKI_WALLPAPER,
        320, 264, 53, G_IM_FMT_RGBA, G_IM_SIZ_16b, 0, 0
    },
};

static const NDSRelocSymbolProbe sNdsRelocSymbolProbes[] = {
    {
        &llMVCommonRoomBackgroundDObjDesc,
        NDS_RELOC_ASSET_MV_COMMON,
        NDS_RELOC_SYMBOL_MVCOMMON_BACKGROUND_DOBJ,
        (1u << 0)
    },
    {
        &llMVCommonRoomDeskDObjDesc,
        NDS_RELOC_ASSET_MV_COMMON,
        NDS_RELOC_SYMBOL_MVCOMMON_DESK_DOBJ,
        (1u << 18)
    },
    {
        &llMVCommonRoomOutsideDisplayList,
        NDS_RELOC_ASSET_MV_COMMON,
        NDS_RELOC_SYMBOL_MVCOMMON_OUTSIDE_DL,
        (1u << 16)
    },
    {
        &llMVCommonRoomHazeDisplayList,
        NDS_RELOC_ASSET_MV_COMMON,
        NDS_RELOC_SYMBOL_MVCOMMON_HAZE_DL,
        (1u << 17)
    },
    {
        &llMVCommonRoomSunlightDisplayList,
        NDS_RELOC_ASSET_MV_COMMON,
        NDS_RELOC_SYMBOL_MVCOMMON_SUNLIGHT_DL,
        (1u << 1)
    },
    {
        &llMVOpeningRoomTransitionOverlayDisplayList,
        NDS_RELOC_ASSET_OPENING_ROOM_TRANSITION,
        NDS_RELOC_SYMBOL_OPENING_ROOM_TRANSITION_OVERLAY_DL,
        (1u << 2)
    },
    {
        &llMVOpeningRoomScene1CamAnimJoint,
        NDS_RELOC_ASSET_OPENING_ROOM_SCENE1,
        NDS_RELOC_SYMBOL_OPENING_ROOM_SCENE1_CAM_ANIM,
        (1u << 3)
    },
    {
        &llMVOpeningRoomScene2CamAnimJoint,
        NDS_RELOC_ASSET_OPENING_ROOM_SCENE2,
        NDS_RELOC_SYMBOL_OPENING_ROOM_SCENE2_CAM_ANIM,
        (1u << 15)
    },
    {
        &llMVCommonRoomPencilsDObjDesc,
        NDS_RELOC_ASSET_MV_COMMON,
        NDS_RELOC_SYMBOL_MVCOMMON_PENCILS_DOBJ,
        (1u << 5)
    },
    {
        &llMVCommonRoomPencilsAnimJoint,
        NDS_RELOC_ASSET_MV_COMMON,
        NDS_RELOC_SYMBOL_MVCOMMON_PENCILS_ANIM,
        (1u << 6)
    },
    {
        &llMVCommonRoomLogoDObjDesc,
        NDS_RELOC_ASSET_MV_COMMON,
        NDS_RELOC_SYMBOL_MVCOMMON_LOGO_DOBJ,
        (1u << 9)
    },
    {
        &llMVCommonRoomLogoMObjSub,
        NDS_RELOC_ASSET_MV_COMMON,
        NDS_RELOC_SYMBOL_MVCOMMON_LOGO_MOBJ,
        (1u << 10)
    },
    {
        &llMVCommonRoomLogoMatAnimJoint,
        NDS_RELOC_ASSET_MV_COMMON,
        NDS_RELOC_SYMBOL_MVCOMMON_LOGO_MATANIM,
        (1u << 11)
    },
    {
        &llMVCommonRoomBossShadowDisplayList,
        NDS_RELOC_ASSET_MV_COMMON,
        NDS_RELOC_SYMBOL_MVCOMMON_BOSS_SHADOW_DL,
        (1u << 7)
    },
    {
        &llMVCommonRoomBossShadowAnimJoint,
        NDS_RELOC_ASSET_MV_COMMON,
        NDS_RELOC_SYMBOL_MVCOMMON_BOSS_SHADOW_ANIM,
        (1u << 8)
    },
    {
        &llMVCommonRoomSpotlightDisplayList,
        NDS_RELOC_ASSET_MV_COMMON,
        NDS_RELOC_SYMBOL_MVCOMMON_SPOTLIGHT_DL,
        (1u << 12)
    },
    {
        &llMVCommonRoomSpotlightMObjSub,
        NDS_RELOC_ASSET_MV_COMMON,
        NDS_RELOC_SYMBOL_MVCOMMON_SPOTLIGHT_MOBJ,
        (1u << 13)
    },
    {
        &llMVCommonRoomSpotlightMatAnimJoint,
        NDS_RELOC_ASSET_MV_COMMON,
        NDS_RELOC_SYMBOL_MVCOMMON_SPOTLIGHT_MATANIM,
        (1u << 14)
    },
    {
        &llMVOpeningRoomWallpaperSprite,
        NDS_RELOC_ASSET_OPENING_ROOM_WALLPAPER,
        NDS_RELOC_SYMBOL_OPENING_ROOM_WALLPAPER_SPRITE,
        (1u << 4)
    },
};

static const NDSTitleSpriteDesc sNdsTitleSpriteDescs[] = {
    {
        &llMNTitleCutoutSprite,
        NDS_RELOC_SYMBOL_TITLE_CUTOUT,
        157, 94,
        208, 90,
        G_IM_FMT_I, G_IM_SIZ_4b
    },
    {
        &llMNTitleSmashSprite,
        NDS_RELOC_SYMBOL_TITLE_SMASH,
        161, 88,
        172, 62,
        G_IM_FMT_RGBA, G_IM_SIZ_32b
    },
    {
        &llMNTitleSuperSprite,
        NDS_RELOC_SYMBOL_TITLE_SUPER,
        55, 96,
        64, 50,
        G_IM_FMT_IA, G_IM_SIZ_8b
    },
    {
        &llMNTitleBrosSprite,
        NDS_RELOC_SYMBOL_TITLE_BROS,
        268, 96,
        56, 52,
        G_IM_FMT_IA, G_IM_SIZ_8b
    },
    {
        &llMNTitleTMUnkSprite,
        NDS_RELOC_SYMBOL_TITLE_TM_UNK,
        270, 132,
        32, 12,
        G_IM_FMT_I, G_IM_SIZ_4b
    },
    {
        &llMNTitleCopyrightSprite,
        NDS_RELOC_SYMBOL_TITLE_COPYRIGHT,
        160, 208,
        300, 44,
        G_IM_FMT_IA, G_IM_SIZ_8b
    },
    {
        &llMNTitleBorderUpperSprite,
        NDS_RELOC_SYMBOL_TITLE_BORDER_UPPER,
        160, 15,
        300, 10,
        G_IM_FMT_I, G_IM_SIZ_4b
    },
    {
        &llMNTitlePressStartSprite,
        NDS_RELOC_SYMBOL_TITLE_PRESS_START,
        162, 177,
        96, 18,
        G_IM_FMT_IA, G_IM_SIZ_8b
    },
    {
        &llMNTitleLogoAnimFullSprite,
        NDS_RELOC_SYMBOL_TITLE_LOGO_FULL,
        260, 60,
        128, 124,
        G_IM_FMT_I, G_IM_SIZ_4b
    },
    {
        &llMNTitleTMSprite,
        NDS_RELOC_SYMBOL_TITLE_TM,
        277, 157,
        32, 12,
        G_IM_FMT_I, G_IM_SIZ_4b
    }
};

static const NDSRelocKnownSymbol sNdsTitleFireAnimFrameSymbols[] = {
    { &llMNTitleFireAnimFrame1Sprite, NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME1 },
    { &llMNTitleFireAnimFrame2Sprite, NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME2 },
    { &llMNTitleFireAnimFrame3Sprite, NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME3 },
    { &llMNTitleFireAnimFrame4Sprite, NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME4 },
    { &llMNTitleFireAnimFrame5Sprite, NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME5 },
    { &llMNTitleFireAnimFrame6Sprite, NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME6 },
    { &llMNTitleFireAnimFrame7Sprite, NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME7 },
    { &llMNTitleFireAnimFrame8Sprite, NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME8 },
    { &llMNTitleFireAnimFrame9Sprite, NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME9 },
    { &llMNTitleFireAnimFrame10Sprite, NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME10 },
    { &llMNTitleFireAnimFrame11Sprite, NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME11 },
    { &llMNTitleFireAnimFrame12Sprite, NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME12 },
    { &llMNTitleFireAnimFrame13Sprite, NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME13 },
    { &llMNTitleFireAnimFrame14Sprite, NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME14 },
    { &llMNTitleFireAnimFrame15Sprite, NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME15 },
    { &llMNTitleFireAnimFrame16Sprite, NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME16 },
    { &llMNTitleFireAnimFrame17Sprite, NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME17 },
    { &llMNTitleFireAnimFrame18Sprite, NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME18 },
    { &llMNTitleFireAnimFrame19Sprite, NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME19 },
    { &llMNTitleFireAnimFrame20Sprite, NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME20 },
    { &llMNTitleFireAnimFrame21Sprite, NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME21 },
    { &llMNTitleFireAnimFrame22Sprite, NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME22 },
    { &llMNTitleFireAnimFrame23Sprite, NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME23 },
    { &llMNTitleFireAnimFrame24Sprite, NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME24 },
    { &llMNTitleFireAnimFrame25Sprite, NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME25 },
    { &llMNTitleFireAnimFrame26Sprite, NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME26 },
    { &llMNTitleFireAnimFrame27Sprite, NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME27 },
    { &llMNTitleFireAnimFrame28Sprite, NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME28 },
    { &llMNTitleFireAnimFrame29Sprite, NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME29 },
    { &llMNTitleFireAnimFrame30Sprite, NDS_RELOC_SYMBOL_TITLE_FIRE_FRAME30 },
};

static const NDSRelocKnownSymbol sNdsMNCommonSymbols[] = {
    { &llMNCommonOptionTabLeftSprite, NDS_RELOC_SYMBOL_MNCOMMON_OPTION_TAB_LEFT },
    { &llMNCommonOptionTabMiddleSprite, NDS_RELOC_SYMBOL_MNCOMMON_OPTION_TAB_MIDDLE },
    { &llMNCommonOptionTabRightSprite, NDS_RELOC_SYMBOL_MNCOMMON_OPTION_TAB_RIGHT },
    { &llMNCommonFrameSprite, NDS_RELOC_SYMBOL_MNCOMMON_FRAME },
    { &llMNCommonGameModeTextSprite, NDS_RELOC_SYMBOL_MNCOMMON_GAME_MODE_TEXT },
    { &llMNCommonDigit0Sprite, NDS_RELOC_SYMBOL_MNCOMMON_DIGIT0 },
    { &llMNCommonDigit1Sprite, NDS_RELOC_SYMBOL_MNCOMMON_DIGIT1 },
    { &llMNCommonDigit2Sprite, NDS_RELOC_SYMBOL_MNCOMMON_DIGIT2 },
    { &llMNCommonDigit3Sprite, NDS_RELOC_SYMBOL_MNCOMMON_DIGIT3 },
    { &llMNCommonDigit4Sprite, NDS_RELOC_SYMBOL_MNCOMMON_DIGIT4 },
    { &llMNCommonDigit5Sprite, NDS_RELOC_SYMBOL_MNCOMMON_DIGIT5 },
    { &llMNCommonDigit6Sprite, NDS_RELOC_SYMBOL_MNCOMMON_DIGIT6 },
    { &llMNCommonDigit7Sprite, NDS_RELOC_SYMBOL_MNCOMMON_DIGIT7 },
    { &llMNCommonDigit8Sprite, NDS_RELOC_SYMBOL_MNCOMMON_DIGIT8 },
    { &llMNCommonDigit9Sprite, NDS_RELOC_SYMBOL_MNCOMMON_DIGIT9 },
    { &llMNCommonInfinitySprite, NDS_RELOC_SYMBOL_MNCOMMON_INFINITY },
    { &llMNCommonArrowRSprite, NDS_RELOC_SYMBOL_MNCOMMON_ARROW_R },
    { &llMNCommonArrowLSprite, NDS_RELOC_SYMBOL_MNCOMMON_ARROW_L },
    { &llMNCommonDecalPaperSprite, NDS_RELOC_SYMBOL_MNCOMMON_DECAL_PAPER },
    { &llMNCommonSmashLogoSprite, NDS_RELOC_SYMBOL_MNCOMMON_SMASH_LOGO },
    { &llMNCommonSmashBrosCollageSprite, NDS_RELOC_SYMBOL_MNCOMMON_SMASH_BROS_COLLAGE },
};

static const NDSRelocKnownSymbol sNdsMNVSModeSymbols[] = {
    { &llMNVSModeVSStartTextSprite, NDS_RELOC_SYMBOL_MNVSMODE_VS_START_TEXT },
    { &llMNVSModeRulePeriodTextSprite, NDS_RELOC_SYMBOL_MNVSMODE_RULE_PERIOD_TEXT },
    { &llMNVSModeTimeTextSprite, NDS_RELOC_SYMBOL_MNVSMODE_TIME_TEXT },
    { &llMNVSModeStockTextSprite, NDS_RELOC_SYMBOL_MNVSMODE_STOCK_TEXT },
    { &llMNVSModeTeamTextSprite, NDS_RELOC_SYMBOL_MNVSMODE_TEAM_TEXT },
    { &llMNVSModeTimePeriodTextSprite, NDS_RELOC_SYMBOL_MNVSMODE_TIME_PERIOD_TEXT },
    { &llMNVSModeMinTextSprite, NDS_RELOC_SYMBOL_MNVSMODE_MIN_TEXT },
    { &llMNVSModeStockPeriodTextSprite, NDS_RELOC_SYMBOL_MNVSMODE_STOCK_PERIOD_TEXT },
    { &llMNVSModeVSOptionsTextSprite, NDS_RELOC_SYMBOL_MNVSMODE_VS_OPTIONS_TEXT },
    { &llMNVSModeConsoleIconDarkSprite, NDS_RELOC_SYMBOL_MNVSMODE_CONSOLE_ICON_DARK },
    { &llMNVSModeVSTextSprite, NDS_RELOC_SYMBOL_MNVSMODE_VS_TEXT },
};

static u32 ndsRelocFileID(const void *file_id)
{
    return (u32)(uintptr_t)file_id;
}

static u32 ndsRelocReadBe32(const void *addr)
{
    const u8 *bytes = addr;

    return ((u32)bytes[0] << 24) |
           ((u32)bytes[1] << 16) |
           ((u32)bytes[2] << 8) |
           (u32)bytes[3];
}

static u32 ndsRelocReadNative32(const void *addr)
{
    u32 value;

    memcpy(&value, addr, sizeof(value));
    return value;
}

static void ndsRelocWriteNative32(void *addr, u32 value)
{
    memcpy(addr, &value, sizeof(value));
}

static void ndsRelocWriteNativePointer(void *addr, void *target)
{
    ndsRelocWriteNative32(addr, (u32)(uintptr_t)target);
}

static u32 ndsRelocAssetIDForToken(u32 token)
{
    if (token == ndsRelocFileID(&llN64LogoFileID)) return NDS_RELOC_ASSET_N64_LOGO;
    if (token == ndsRelocFileID(&llIFCommonAnnounceCommonFileID)) return NDS_RELOC_ASSET_IF_COMMON_ANNOUNCE;
    if (token == ndsRelocFileID(&llMVCommonFileID)) return NDS_RELOC_ASSET_MV_COMMON;
    if (token == ndsRelocFileID(&llMVOpeningCommonFileID)) return NDS_RELOC_ASSET_OPENING_COMMON;
    if (token == ndsRelocFileID(&llMVOpeningRoomTransitionFileID)) return NDS_RELOC_ASSET_OPENING_ROOM_TRANSITION;
    if (token == ndsRelocFileID(&llMVOpeningRoomScene1FileID)) return NDS_RELOC_ASSET_OPENING_ROOM_SCENE1;
    if (token == ndsRelocFileID(&llMVOpeningRoomScene2FileID)) return NDS_RELOC_ASSET_OPENING_ROOM_SCENE2;
    if (token == ndsRelocFileID(&llMVOpeningRoomScene3FileID)) return NDS_RELOC_ASSET_OPENING_ROOM_SCENE3;
    if (token == ndsRelocFileID(&llMVOpeningRoomScene4FileID)) return NDS_RELOC_ASSET_OPENING_ROOM_SCENE4;
    if (token == ndsRelocFileID(&llMVOpeningRunFileID)) return NDS_RELOC_ASSET_OPENING_RUN;
    if (token == ndsRelocFileID(&llMVOpeningYamabukiFileID)) return NDS_RELOC_ASSET_OPENING_YAMABUKI;
    if (token == ndsRelocFileID(&llMVOpeningSectorFileID)) return NDS_RELOC_ASSET_OPENING_SECTOR;
    if (token == ndsRelocFileID(&llMVOpeningRunCrashFileID)) return NDS_RELOC_ASSET_OPENING_RUN_CRASH;
    if (token == ndsRelocFileID(&llMVOpeningRoomWallpaperFileID)) return NDS_RELOC_ASSET_OPENING_ROOM_WALLPAPER;
    if (token == ndsRelocFileID(&llMVOpeningPortraitsSet1FileID)) return NDS_RELOC_ASSET_OPENING_PORTRAITS_SET1;
    if (token == ndsRelocFileID(&llMVOpeningPortraitsSet2FileID)) return NDS_RELOC_ASSET_OPENING_PORTRAITS_SET2;
    if (token == ndsRelocFileID(&llMNTitleFileID)) return NDS_RELOC_ASSET_MN_TITLE;
    if (token == ndsRelocFileID(&llMNTitleFireAnimFileID)) return NDS_RELOC_ASSET_MN_TITLE_FIRE_ANIM;
    if (token == ndsRelocFileID(&llMNCommonFileID)) return NDS_RELOC_ASSET_MN_COMMON;
    if (token == ndsRelocFileID(&llMNVSModeFileID)) return NDS_RELOC_ASSET_MN_VS_MODE;
    return NDS_RELOC_ASSET_INVALID;
}

static u32 ndsRelocOpeningRoomBitForAsset(u32 file_id)
{
    if (file_id == NDS_RELOC_ASSET_MV_COMMON) return (1u << 0);
    if (file_id == NDS_RELOC_ASSET_OPENING_ROOM_TRANSITION) return (1u << 1);
    if (file_id == NDS_RELOC_ASSET_OPENING_ROOM_SCENE1) return (1u << 2);
    if (file_id == NDS_RELOC_ASSET_OPENING_ROOM_SCENE2) return (1u << 3);
    if (file_id == NDS_RELOC_ASSET_OPENING_ROOM_SCENE3) return (1u << 4);
    if (file_id == NDS_RELOC_ASSET_OPENING_ROOM_SCENE4) return (1u << 5);
    if (file_id == NDS_RELOC_ASSET_OPENING_RUN_CRASH) return (1u << 6);
    if (file_id == NDS_RELOC_ASSET_OPENING_ROOM_WALLPAPER) return (1u << 7);
    return 0;
}

static s32 ndsRelocIsOpeningRoomAsset(u32 asset_id)
{
    return (ndsRelocOpeningRoomBitForAsset(asset_id) != 0u) ? TRUE : FALSE;
}

static void ndsRelocResetLoadedFiles(void)
{
    memset(sNdsRelocLoadedFiles, 0, sizeof(sNdsRelocLoadedFiles));
    sNdsRelocLoadedFileCount = 0;
}

static NDSRelocLoadedFile *ndsRelocFindLoadedFileByAsset(u32 asset_id)
{
    u32 i;

    for (i = 0; i < sNdsRelocLoadedFileCount; i++)
    {
        if (sNdsRelocLoadedFiles[i].asset_id == asset_id)
        {
            return &sNdsRelocLoadedFiles[i];
        }
    }
    return NULL;
}

static NDSRelocLoadedFile *ndsRelocFindLoadedFileByData(void *file)
{
    u32 i;

    for (i = 0; i < sNdsRelocLoadedFileCount; i++)
    {
        if (sNdsRelocLoadedFiles[i].data == file)
        {
            return &sNdsRelocLoadedFiles[i];
        }
    }
    return NULL;
}

static s32 ndsRelocRangeInLoadedFile(const NDSRelocLoadedFile *loaded,
                                      uintptr_t offset, size_t size)
{
    if ((loaded == NULL) || (loaded->data == NULL))
    {
        return FALSE;
    }
    if ((offset > loaded->data_size) ||
        (size > (size_t)(loaded->data_size - offset)))
    {
        return FALSE;
    }
    return TRUE;
}

static s32 ndsRelocPointerRangeInLoadedFile(const NDSRelocLoadedFile *loaded,
                                             const void *ptr, size_t size)
{
    uintptr_t base;
    uintptr_t addr;

    if ((loaded == NULL) || (loaded->data == NULL) || (ptr == NULL))
    {
        return FALSE;
    }

    base = (uintptr_t)loaded->data;
    addr = (uintptr_t)ptr;
    if ((addr < base) || (addr > (base + loaded->data_size)))
    {
        return FALSE;
    }
    return ndsRelocRangeInLoadedFile(loaded, addr - base, size);
}

static NDSRelocLoadedFile *ndsRelocFindLoadedFileContaining(const void *ptr,
                                                             size_t size)
{
    u32 i;

    for (i = 0; i < sNdsRelocLoadedFileCount; i++)
    {
        if (ndsRelocPointerRangeInLoadedFile(&sNdsRelocLoadedFiles[i],
                                             ptr,
                                             size) != FALSE)
        {
            return &sNdsRelocLoadedFiles[i];
        }
    }
    return NULL;
}

static NDSRelocLoadedFile *ndsRelocRegisterLoadedFile(u32 asset_id, u32 bit,
                                                       void *data,
                                                       const NDSRelocAssetHeader *header)
{
    NDSRelocLoadedFile *loaded;

    loaded = ndsRelocFindLoadedFileByAsset(asset_id);
    if (loaded == NULL)
    {
        if (sNdsRelocLoadedFileCount >= NDS_RELOC_LOADED_FILE_CAPACITY)
        {
            gNdsOpeningRoomRelocPointerFixupFailCount++;
            return NULL;
        }
        loaded = &sNdsRelocLoadedFiles[sNdsRelocLoadedFileCount++];
    }

    loaded->asset_id = asset_id;
    loaded->bit = bit;
    loaded->data = data;
    loaded->data_size = header->data_size;
    loaded->reloc_intern_offset = header->reloc_intern_offset;
    loaded->reloc_extern_offset = header->reloc_extern_offset;

    return loaded;
}

static s32 ndsRelocApplyWordByteSwap(NDSRelocLoadedFile *loaded)
{
    u32 words;
    u32 i;

    if ((loaded == NULL) || (loaded->data == NULL))
    {
        gNdsOpeningRoomRelocWordSwapFailCount++;
        return FALSE;
    }

    words = loaded->data_size / sizeof(u32);
    for (i = 0; i < words; i++)
    {
        void *word = (u8 *)loaded->data + (i * sizeof(u32));

        ndsRelocWriteNative32(word, ndsRelocReadBe32(word));
    }

    if (loaded->asset_id == NDS_RELOC_ASSET_N64_LOGO)
    {
        gNdsStartupLogoRelocWordSwapCount += words;
    }
    else if (ndsRelocIsOpeningRoomAsset(loaded->asset_id) != FALSE)
    {
        gNdsOpeningRoomRelocWordSwapCount += words;
    }
    return TRUE;
}

static s32 ndsRelocApplyInternalPointerFixups(NDSRelocLoadedFile *loaded)
{
    u16 reloc_intern;
    u32 guard;
    u32 fixed_count = 0;

    if ((loaded == NULL) || (loaded->data == NULL))
    {
        gNdsOpeningRoomRelocPointerFixupFailCount++;
        return FALSE;
    }
    if (loaded->reloc_extern_offset != 0xffffu)
    {
        /* The current Opening Room resources do not have external deps. Keep
         * this explicit so the next subsystem adds dependency loading instead
         * of silently treating external pointers as internal ones. */
        gNdsOpeningRoomRelocPointerFixupFailCount++;
        return FALSE;
    }

    reloc_intern = loaded->reloc_intern_offset;
    guard = (loaded->data_size / sizeof(u32)) + 1u;

    while (reloc_intern != 0xffffu)
    {
        uintptr_t slot_offset = (uintptr_t)reloc_intern * sizeof(u32);
        u32 reloc_word;
        u16 next_reloc;
        u16 target_words;
        uintptr_t target_offset;
        void *slot;
        void *target;

        if ((guard == 0) || ((slot_offset + sizeof(u32)) > loaded->data_size))
        {
            gNdsOpeningRoomRelocPointerFixupFailCount++;
            return FALSE;
        }
        guard--;

        slot = (u8 *)loaded->data + slot_offset;
        reloc_word = ndsRelocReadNative32(slot);
        next_reloc = (u16)(reloc_word >> 16);
        target_words = (u16)(reloc_word & 0xffffu);
        target_offset = (uintptr_t)target_words * sizeof(u32);

        if (target_offset >= loaded->data_size)
        {
            gNdsOpeningRoomRelocPointerFixupFailCount++;
            return FALSE;
        }

        target = (u8 *)loaded->data + target_offset;
        ndsRelocWriteNativePointer(slot, target);

        fixed_count++;
        reloc_intern = next_reloc;
    }

    if (loaded->asset_id == NDS_RELOC_ASSET_N64_LOGO)
    {
        gNdsStartupLogoRelocPointerFixupCount += fixed_count;
    }
    else if (ndsRelocIsOpeningRoomAsset(loaded->asset_id) != FALSE)
    {
        gNdsOpeningRoomRelocPointerFixupCount += fixed_count;
    }
    return TRUE;
}

static void ndsRelocSwapS16Pair(s16 *a, s16 *b)
{
    s16 tmp = *a;

    *a = *b;
    *b = tmp;
}

static void ndsRelocSwapSpriteAttrZDepth(Sprite *sprite)
{
    u16 attr = sprite->attr;

    sprite->attr = (u16)sprite->zdepth;
    sprite->zdepth = (s16)attr;
}

static void ndsRelocReverseSpriteColorBytes(Sprite *sprite)
{
    u8 red = sprite->red;
    u8 green = sprite->green;
    u8 blue = sprite->blue;
    u8 alpha = sprite->alpha;

    sprite->red = alpha;
    sprite->green = blue;
    sprite->blue = green;
    sprite->alpha = red;
}

static void ndsRelocNormalizeSpriteHeaderFields(Sprite *sprite, u8 bmfmt,
                                                u8 bmsiz)
{
    ndsRelocSwapS16Pair(&sprite->x, &sprite->y);
    ndsRelocSwapS16Pair(&sprite->width, &sprite->height);
    ndsRelocSwapS16Pair(&sprite->expx, &sprite->expy);
    ndsRelocSwapSpriteAttrZDepth(sprite);
    ndsRelocReverseSpriteColorBytes(sprite);
    ndsRelocSwapS16Pair(&sprite->startTLUT, &sprite->nTLUT);
    ndsRelocSwapS16Pair(&sprite->istart, &sprite->istep);
    ndsRelocSwapS16Pair(&sprite->nbitmaps, &sprite->ndisplist);
    ndsRelocSwapS16Pair(&sprite->bmheight, &sprite->bmHreal);

    /* The blanket u32 endian pass shifts these 8-bit format fields into the
     * padding before the bitmap pointer. Keep correction tied to known Sprite
     * manifests rather than guessing across every relocated resource. */
    sprite->bmfmt = bmfmt;
    sprite->bmsiz = bmsiz;
}

static s32 ndsRelocNormalizeSpriteBitmapTable(NDSRelocLoadedFile *loaded,
                                               Sprite *sprite,
                                               u32 bitmap_count)
{
    Bitmap *bitmap;
    u32 i;

    bitmap = sprite->bitmap;
    if (ndsRelocPointerRangeInLoadedFile(
            loaded, bitmap, sizeof(Bitmap) * bitmap_count) == FALSE)
    {
        return FALSE;
    }

    for (i = 0; i < bitmap_count; i++)
    {
        ndsRelocSwapS16Pair(&bitmap[i].width, &bitmap[i].width_img);
        ndsRelocSwapS16Pair(&bitmap[i].s, &bitmap[i].t);
        ndsRelocSwapS16Pair(&bitmap[i].actualHeight, &bitmap[i].LUToffset);
    }
    return TRUE;
}

static void ndsRelocNormalizeN64LogoSprite(NDSRelocLoadedFile *loaded)
{
    Sprite *sprite;

    if ((loaded == NULL) ||
        (loaded->asset_id != NDS_RELOC_ASSET_N64_LOGO) ||
        (ndsRelocRangeInLoadedFile(loaded, NDS_RELOC_SYMBOL_N64_LOGO_SPRITE,
                                   sizeof(Sprite)) == FALSE))
    {
        return;
    }

    sprite = (Sprite *)((u8 *)loaded->data +
                        NDS_RELOC_SYMBOL_N64_LOGO_SPRITE);
    if ((sprite->width == 128) &&
        (sprite->height == 108) &&
        (sprite->nbitmaps == 8) &&
        (sprite->bmfmt == G_IM_FMT_RGBA) &&
        (sprite->bmsiz == G_IM_SIZ_16b))
    {
        return;
    }

    ndsRelocNormalizeSpriteHeaderFields(sprite, G_IM_FMT_RGBA, G_IM_SIZ_16b);
    (void)ndsRelocNormalizeSpriteBitmapTable(loaded, sprite, 8u);
}

static void ndsRelocNormalizeOpeningPortraitsSprite(
    NDSRelocLoadedFile *loaded,
    u32 offset,
    u32 expected_width,
    u32 expected_height,
    u8 bmfmt,
    u8 bmsiz)
{
    Sprite *sprite;
    u32 bitmap_count;

    if ((loaded == NULL) ||
        (ndsRelocRangeInLoadedFile(loaded, offset, sizeof(Sprite)) == FALSE))
    {
        gNdsOpeningPortraitsSpriteNormalizeFailCount++;
        return;
    }

    sprite = (Sprite *)((u8 *)loaded->data + offset);
    if (((u32)(u16)sprite->width == expected_width) &&
        ((u32)(u16)sprite->height == expected_height) &&
        (sprite->bmfmt == bmfmt) &&
        (sprite->bmsiz == bmsiz))
    {
        gNdsOpeningPortraitsSpriteNormalizeCount++;
        return;
    }

    ndsRelocNormalizeSpriteHeaderFields(sprite, bmfmt, bmsiz);
    bitmap_count = (u32)(u16)sprite->nbitmaps;
    if (((u32)(u16)sprite->width != expected_width) ||
        ((u32)(u16)sprite->height != expected_height) ||
        (bitmap_count == 0) ||
        (bitmap_count > 128u) ||
        (ndsRelocNormalizeSpriteBitmapTable(loaded, sprite,
                                            bitmap_count) == FALSE))
    {
        gNdsOpeningPortraitsSpriteNormalizeFailCount++;
        return;
    }

    gNdsOpeningPortraitsSpriteNormalizeCount++;
}

static void ndsRelocNormalizeOpeningPortraitsSprites(NDSRelocLoadedFile *loaded)
{
    if (loaded == NULL)
    {
        return;
    }

    if (loaded->asset_id == NDS_RELOC_ASSET_OPENING_PORTRAITS_SET1)
    {
        ndsRelocNormalizeOpeningPortraitsSprite(
            loaded, NDS_RELOC_SYMBOL_OPENING_PORTRAITS_SET1_SAMUS,
            NDS_OPENING_PORTRAITS_CARD_WIDTH,
            NDS_OPENING_PORTRAITS_CARD_HEIGHT, G_IM_FMT_RGBA, G_IM_SIZ_16b);
        ndsRelocNormalizeOpeningPortraitsSprite(
            loaded, NDS_RELOC_SYMBOL_OPENING_PORTRAITS_SET1_MARIO,
            NDS_OPENING_PORTRAITS_CARD_WIDTH,
            NDS_OPENING_PORTRAITS_CARD_HEIGHT, G_IM_FMT_RGBA, G_IM_SIZ_16b);
        ndsRelocNormalizeOpeningPortraitsSprite(
            loaded, NDS_RELOC_SYMBOL_OPENING_PORTRAITS_SET1_FOX,
            NDS_OPENING_PORTRAITS_CARD_WIDTH,
            NDS_OPENING_PORTRAITS_CARD_HEIGHT, G_IM_FMT_RGBA, G_IM_SIZ_16b);
        ndsRelocNormalizeOpeningPortraitsSprite(
            loaded, NDS_RELOC_SYMBOL_OPENING_PORTRAITS_SET1_PIKACHU,
            NDS_OPENING_PORTRAITS_CARD_WIDTH,
            NDS_OPENING_PORTRAITS_CARD_HEIGHT, G_IM_FMT_RGBA, G_IM_SIZ_16b);
        ndsRelocNormalizeOpeningPortraitsSprite(
            loaded, NDS_RELOC_SYMBOL_OPENING_PORTRAITS_SET1_COVER,
            NDS_OPENING_PORTRAITS_COVER_WIDTH,
            NDS_OPENING_PORTRAITS_COVER_HEIGHT, G_IM_FMT_I, G_IM_SIZ_4b);
    }
    else if (loaded->asset_id == NDS_RELOC_ASSET_OPENING_PORTRAITS_SET2)
    {
        ndsRelocNormalizeOpeningPortraitsSprite(
            loaded, NDS_RELOC_SYMBOL_OPENING_PORTRAITS_SET2_LINK,
            NDS_OPENING_PORTRAITS_CARD_WIDTH,
            NDS_OPENING_PORTRAITS_CARD_HEIGHT, G_IM_FMT_RGBA, G_IM_SIZ_16b);
        ndsRelocNormalizeOpeningPortraitsSprite(
            loaded, NDS_RELOC_SYMBOL_OPENING_PORTRAITS_SET2_KIRBY,
            NDS_OPENING_PORTRAITS_CARD_WIDTH,
            NDS_OPENING_PORTRAITS_CARD_HEIGHT, G_IM_FMT_RGBA, G_IM_SIZ_16b);
        ndsRelocNormalizeOpeningPortraitsSprite(
            loaded, NDS_RELOC_SYMBOL_OPENING_PORTRAITS_SET2_DONKEY,
            NDS_OPENING_PORTRAITS_CARD_WIDTH,
            NDS_OPENING_PORTRAITS_CARD_HEIGHT, G_IM_FMT_RGBA, G_IM_SIZ_16b);
        ndsRelocNormalizeOpeningPortraitsSprite(
            loaded, NDS_RELOC_SYMBOL_OPENING_PORTRAITS_SET2_YOSHI,
            NDS_OPENING_PORTRAITS_CARD_WIDTH,
            NDS_OPENING_PORTRAITS_CARD_HEIGHT, G_IM_FMT_RGBA, G_IM_SIZ_16b);
    }

    if (gNdsOpeningPortraitsSpriteNormalizeFailCount == 0)
    {
        gNdsOpeningPortraitsRelocResult = NDS_OPENING_PORTRAITS_RELOC_PASS;
    }
}

static void ndsRelocNormalizeIFAnnounceSprite(NDSRelocLoadedFile *loaded,
                                              u32 offset)
{
    Sprite *sprite;
    u32 bitmap_count;

    if ((loaded == NULL) ||
        (loaded->asset_id != NDS_RELOC_ASSET_IF_COMMON_ANNOUNCE) ||
        (ndsRelocRangeInLoadedFile(loaded, offset, sizeof(Sprite)) == FALSE))
    {
        gNdsOpeningMarioSpriteNormalizeFailCount++;
        return;
    }

    sprite = (Sprite *)((u8 *)loaded->data + offset);
    if ((sprite->bmfmt == G_IM_FMT_IA) &&
        (sprite->bmsiz == G_IM_SIZ_8b) &&
        ((u32)(u16)sprite->width <= NDS_IF_ANNOUNCE_LETTER_MAX_WIDTH) &&
        ((u32)(u16)sprite->height <= NDS_IF_ANNOUNCE_LETTER_MAX_HEIGHT) &&
        ((u32)(u16)sprite->nbitmaps != 0) &&
        ((u32)(u16)sprite->nbitmaps <= 16u))
    {
        gNdsOpeningMarioSpriteNormalizeCount++;
        return;
    }

    ndsRelocNormalizeSpriteHeaderFields(sprite, G_IM_FMT_IA, G_IM_SIZ_8b);
    bitmap_count = (u32)(u16)sprite->nbitmaps;
    if (((u32)(u16)sprite->width == 0) ||
        ((u32)(u16)sprite->height == 0) ||
        ((u32)(u16)sprite->width > NDS_IF_ANNOUNCE_LETTER_MAX_WIDTH) ||
        ((u32)(u16)sprite->height > NDS_IF_ANNOUNCE_LETTER_MAX_HEIGHT) ||
        (bitmap_count == 0) ||
        (bitmap_count > 16u) ||
        (ndsRelocNormalizeSpriteBitmapTable(loaded, sprite,
                                            bitmap_count) == FALSE))
    {
        gNdsOpeningMarioSpriteNormalizeFailCount++;
        return;
    }

    gNdsOpeningMarioSpriteNormalizeCount++;
}

static void ndsRelocNormalizeIFAnnounceMarioSprites(NDSRelocLoadedFile *loaded)
{
    if ((loaded == NULL) ||
        (loaded->asset_id != NDS_RELOC_ASSET_IF_COMMON_ANNOUNCE))
    {
        return;
    }

    ndsRelocNormalizeIFAnnounceSprite(
        loaded, NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_A);
    ndsRelocNormalizeIFAnnounceSprite(
        loaded, NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_B);
    ndsRelocNormalizeIFAnnounceSprite(
        loaded, NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_C);
    ndsRelocNormalizeIFAnnounceSprite(
        loaded, NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_D);
    ndsRelocNormalizeIFAnnounceSprite(
        loaded, NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_E);
    ndsRelocNormalizeIFAnnounceSprite(
        loaded, NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_F);
    ndsRelocNormalizeIFAnnounceSprite(
        loaded, NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_G);
    ndsRelocNormalizeIFAnnounceSprite(
        loaded, NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_H);
    ndsRelocNormalizeIFAnnounceSprite(
        loaded, NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_I);
    ndsRelocNormalizeIFAnnounceSprite(
        loaded, NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_K);
    ndsRelocNormalizeIFAnnounceSprite(
        loaded, NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_L);
    ndsRelocNormalizeIFAnnounceSprite(
        loaded, NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_M);
    ndsRelocNormalizeIFAnnounceSprite(
        loaded, NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_N);
    ndsRelocNormalizeIFAnnounceSprite(
        loaded, NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_O);
    ndsRelocNormalizeIFAnnounceSprite(
        loaded, NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_P);
    ndsRelocNormalizeIFAnnounceSprite(
        loaded, NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_R);
    ndsRelocNormalizeIFAnnounceSprite(
        loaded, NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_S);
    ndsRelocNormalizeIFAnnounceSprite(
        loaded, NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_U);
    ndsRelocNormalizeIFAnnounceSprite(
        loaded, NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_X);
    ndsRelocNormalizeIFAnnounceSprite(
        loaded, NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_Y);
}

static s32 ndsRelocNormalizeTitleSprite(NDSRelocLoadedFile *loaded,
                                        const NDSTitleSpriteDesc *desc)
{
    Sprite *sprite;
    u32 bitmap_count;

    if ((loaded == NULL) ||
        (loaded->asset_id != NDS_RELOC_ASSET_MN_TITLE) ||
        (desc == NULL) ||
        (ndsRelocRangeInLoadedFile(loaded, desc->offset,
                                   sizeof(Sprite)) == FALSE))
    {
        gNdsTitleSpriteNormalizeFailCount++;
        return FALSE;
    }

    sprite = (Sprite *)((u8 *)loaded->data + desc->offset);
    if (((u32)(u16)sprite->width == desc->width) &&
        ((u32)(u16)sprite->height == desc->height) &&
        (sprite->bmfmt == desc->bmfmt) &&
        (sprite->bmsiz == desc->bmsiz))
    {
        gNdsTitleSpriteNormalizeCount++;
        return TRUE;
    }

    ndsRelocNormalizeSpriteHeaderFields(sprite, desc->bmfmt, desc->bmsiz);
    bitmap_count = (u32)(u16)sprite->nbitmaps;
    if (((u32)(u16)sprite->width != desc->width) ||
        ((u32)(u16)sprite->height != desc->height) ||
        ((u32)(u16)sprite->width == 0) ||
        ((u32)(u16)sprite->height == 0) ||
        ((u32)(u16)sprite->width > NDS_TITLE_MAX_WIDTH) ||
        ((u32)(u16)sprite->height > NDS_TITLE_MAX_HEIGHT) ||
        (bitmap_count == 0) ||
        (bitmap_count > 128u) ||
        (ndsRelocNormalizeSpriteBitmapTable(loaded, sprite,
                                            bitmap_count) == FALSE))
    {
        gNdsTitleSpriteNormalizeFailCount++;
        return FALSE;
    }

    gNdsTitleSpriteNormalizeCount++;
    return TRUE;
}

static void ndsRelocNormalizeTitleSprites(NDSRelocLoadedFile *loaded)
{
    u32 i;

    if ((loaded == NULL) ||
        (loaded->asset_id != NDS_RELOC_ASSET_MN_TITLE))
    {
        return;
    }

    for (i = 0; i < ARRAY_COUNT(sNdsTitleSpriteDescs); i++)
    {
        (void)ndsRelocNormalizeTitleSprite(loaded,
                                           &sNdsTitleSpriteDescs[i]);
    }

    if ((gNdsTitleSpriteNormalizeFailCount == 0) &&
        (gNdsTitleSpriteNormalizeCount >= ARRAY_COUNT(sNdsTitleSpriteDescs)))
    {
        gNdsTitleRelocResult = NDS_TITLE_RELOC_PASS;
    }
}

static s32 ndsRelocNormalizeTitleFireSprite(NDSRelocLoadedFile *loaded,
                                            u32 offset)
{
    Sprite *sprite;
    u32 bitmap_count;
    Bitmap *bitmap;

    if ((loaded == NULL) ||
        (loaded->asset_id != NDS_RELOC_ASSET_MN_TITLE_FIRE_ANIM) ||
        (ndsRelocRangeInLoadedFile(loaded, offset, sizeof(Sprite)) == FALSE))
    {
        gNdsTitleFireSpriteNormalizeFailCount++;
        return FALSE;
    }

    sprite = (Sprite *)((u8 *)loaded->data + offset);
    if (((u32)(u16)sprite->width == 32u) &&
        ((u32)(u16)sprite->height == 32u) &&
        ((u32)(u16)sprite->nbitmaps == 1u) &&
        (sprite->bmfmt == G_IM_FMT_RGBA) &&
        (sprite->bmsiz == G_IM_SIZ_32b))
    {
        gNdsTitleFireSpriteNormalizeCount++;
        return TRUE;
    }

    ndsRelocNormalizeSpriteHeaderFields(sprite, G_IM_FMT_RGBA, G_IM_SIZ_32b);
    bitmap_count = (u32)(u16)sprite->nbitmaps;
    if (((u32)(u16)sprite->width != 32u) ||
        ((u32)(u16)sprite->height != 32u) ||
        (bitmap_count != 1u) ||
        (ndsRelocNormalizeSpriteBitmapTable(loaded, sprite,
                                            bitmap_count) == FALSE))
    {
        gNdsTitleFireSpriteNormalizeFailCount++;
        return FALSE;
    }

    bitmap = sprite->bitmap;
    if ((bitmap == NULL) ||
        ((u32)(u16)bitmap->width != 32u) ||
        ((u32)(u16)bitmap->width_img != 32u) ||
        ((u32)(u16)bitmap->actualHeight != 32u))
    {
        gNdsTitleFireSpriteNormalizeFailCount++;
        return FALSE;
    }

    gNdsTitleFireSpriteNormalizeCount++;
    return TRUE;
}

static void ndsRelocNormalizeTitleFireSprites(NDSRelocLoadedFile *loaded)
{
    u32 i;

    if ((loaded == NULL) ||
        (loaded->asset_id != NDS_RELOC_ASSET_MN_TITLE_FIRE_ANIM))
    {
        return;
    }

    for (i = 0; i < ARRAY_COUNT(sNdsTitleFireAnimFrameSymbols); i++)
    {
        (void)ndsRelocNormalizeTitleFireSprite(
            loaded,
            sNdsTitleFireAnimFrameSymbols[i].offset);
    }
}

static s32 ndsRelocNormalizeOpeningActionPreviewSprite(
    NDSRelocLoadedFile *loaded,
    const NDSOpeningActionPreviewDesc *desc)
{
    Sprite *sprite;

    if ((loaded == NULL) || (desc == NULL) ||
        (loaded->asset_id != desc->asset_id) ||
        (ndsRelocRangeInLoadedFile(loaded, desc->offset,
                                   sizeof(Sprite)) == FALSE))
    {
        gNdsOpeningMovieActionPreviewSpriteNormalizeFailCount++;
        return FALSE;
    }

    sprite = (Sprite *)((u8 *)loaded->data + desc->offset);
    if (((u32)(u16)sprite->width == desc->width) &&
        ((u32)(u16)sprite->height == desc->height) &&
        ((u32)(u16)sprite->nbitmaps == desc->bitmap_count) &&
        (sprite->bmfmt == desc->bmfmt) &&
        (sprite->bmsiz == desc->bmsiz))
    {
        gNdsOpeningMovieActionPreviewSpriteNormalizeCount++;
        return TRUE;
    }

    ndsRelocNormalizeSpriteHeaderFields(sprite, desc->bmfmt, desc->bmsiz);
    if (((u32)(u16)sprite->width != desc->width) ||
        ((u32)(u16)sprite->height != desc->height) ||
        ((u32)(u16)sprite->nbitmaps != desc->bitmap_count) ||
        (desc->width == 0) || (desc->height == 0) ||
        (desc->width > NDS_OPENING_ACTION_PREVIEW_MAX_WIDTH) ||
        (desc->height > NDS_OPENING_ACTION_PREVIEW_MAX_HEIGHT) ||
        (desc->bitmap_count == 0) ||
        (desc->bitmap_count > 128u) ||
        (ndsRelocNormalizeSpriteBitmapTable(
            loaded, sprite, desc->bitmap_count) == FALSE))
    {
        gNdsOpeningMovieActionPreviewSpriteNormalizeFailCount++;
        return FALSE;
    }

    gNdsOpeningMovieActionPreviewSpriteNormalizeCount++;
    return TRUE;
}

static void ndsRelocReverseColorPackBytes(SYColorPack *color)
{
    u8 red;
    u8 green;
    u8 blue;
    u8 alpha;

    if (color == NULL)
    {
        return;
    }

    red = color->s.r;
    green = color->s.g;
    blue = color->s.b;
    alpha = color->s.a;
    color->s.r = alpha;
    color->s.g = blue;
    color->s.b = green;
    color->s.a = red;
}

static u32 ndsRelocEffectiveMObjSubFlags(const MObjSub *mobjsub)
{
    u32 flags;

    if (mobjsub == NULL)
    {
        return 0;
    }

    flags = mobjsub->flags;
    if (flags == MOBJ_FLAG_NONE)
    {
        return MOBJ_FLAG_TEXTURE | 0x20u | MOBJ_FLAG_ALPHA;
    }
    return flags;
}

static s32 ndsRelocMObjSubFlagsKnown(u32 flags)
{
    return (flags &
            ~(MOBJ_FLAG_ALPHA | MOBJ_FLAG_SPLIT | MOBJ_FLAG_PALETTE |
              MOBJ_FLAG_FRAC | MOBJ_FLAG_TEXTURE | MOBJ_FLAG_PRIMCOLOR |
              MOBJ_FLAG_ENVCOLOR | MOBJ_FLAG_BLENDCOLOR |
              MOBJ_FLAG_LIGHT1 | MOBJ_FLAG_LIGHT2 | 0x8u | 0x20u |
              0x40u)) == 0;
}

static s32 ndsRelocMVCommonMObjSubFlagsLookNative(u32 flags)
{
    return (flags == MOBJ_FLAG_NONE) ||
           (flags == MOBJ_FLAG_PRIMCOLOR) ||
           (flags == (MOBJ_FLAG_PRIMCOLOR | MOBJ_FLAG_LIGHT1));
}

static void ndsRelocRecordMObjSubNormalize(
    const NDSRelocLoadedFile *loaded, const MObjSub *mobjsub)
{
    u32 flags;
    u32 effective_flags;

    if (mobjsub == NULL)
    {
        gNdsOpeningRoomRelocMObjSubNormalizeFailCount++;
        return;
    }

    flags = mobjsub->flags;
    effective_flags = ndsRelocEffectiveMObjSubFlags(mobjsub);
    if (gNdsOpeningRoomRelocMObjSubNormalizeCount == 0)
    {
        gNdsOpeningRoomRelocMObjSubFirstFlags = flags;
    }
    gNdsOpeningRoomRelocMObjSubNormalizeCount++;
    gNdsOpeningRoomRelocMObjSubSourceResult =
        NDS_OPENING_ROOM_RELOC_MOBJ_SOURCE_PASS;

    if (flags == MOBJ_FLAG_NONE)
    {
        gNdsOpeningRoomRelocMObjSubZeroFlagCount++;
    }
    if ((flags & MOBJ_FLAG_PRIMCOLOR) != 0)
    {
        gNdsOpeningRoomRelocMObjSubPrimColorCount++;
    }
    if ((flags & (MOBJ_FLAG_LIGHT1 | MOBJ_FLAG_LIGHT2)) != 0)
    {
        gNdsOpeningRoomRelocMObjSubLightCount++;
    }
    if ((effective_flags & MOBJ_FLAG_TEXTURE) != 0)
    {
        gNdsOpeningRoomRelocMObjSubTextureFlagCount++;
        if (gNdsOpeningRoomRelocMObjSubFirstTextureOffset == 0xffffffffu)
        {
            if ((loaded != NULL) &&
                (ndsRelocPointerRangeInLoadedFile(
                     loaded, mobjsub, sizeof(*mobjsub)) != FALSE))
            {
                gNdsOpeningRoomRelocMObjSubFirstTextureOffset =
                    (u32)((const u8 *)mobjsub - (const u8 *)loaded->data);
            }
            gNdsOpeningRoomRelocMObjSubFirstTextureFlags = flags;
        }
    }

    if (ndsRelocMObjSubFlagsKnown(flags) == FALSE)
    {
        gNdsOpeningRoomRelocMObjSubNormalizeFailCount++;
    }
}

static s32 ndsRelocMObjSubMixedFieldsLookNative(const MObjSub *mobjsub)
{
    if (mobjsub == NULL)
    {
        return FALSE;
    }

    return (ndsRelocMVCommonMObjSubFlagsLookNative(mobjsub->flags) != FALSE) &&
           (mobjsub->fmt <= NDS_RELOC_G_IM_FMT_MAX) &&
           (mobjsub->siz <= G_IM_SIZ_32b) &&
           (mobjsub->block_fmt <= NDS_RELOC_G_IM_FMT_MAX) &&
           (mobjsub->block_siz <= G_IM_SIZ_32b);
}

static void ndsRelocNormalizeMObjSubMixedFields(NDSRelocLoadedFile *loaded,
                                                MObjSub *mobjsub)
{
    u16 old_pad00;
    u8 old_fmt;
    u8 old_siz;
    u16 old_flags;
    u8 old_block_fmt;
    u8 old_block_siz;
    u8 old_prim_l;
    u8 old_prim_m;
    u8 old_prim_pad0;
    u8 old_prim_pad1;

    if (mobjsub == NULL)
    {
        gNdsOpeningRoomRelocMObjSubNormalizeFailCount++;
        return;
    }

    if (ndsRelocMObjSubMixedFieldsLookNative(mobjsub) != FALSE)
    {
        ndsRelocRecordMObjSubNormalize(loaded, mobjsub);
        return;
    }

    old_pad00 = mobjsub->pad00;
    old_fmt = mobjsub->fmt;
    old_siz = mobjsub->siz;
    old_flags = mobjsub->flags;
    old_block_fmt = mobjsub->block_fmt;
    old_block_siz = mobjsub->block_siz;
    old_prim_l = mobjsub->prim_l;
    old_prim_m = mobjsub->prim_m;
    old_prim_pad0 = mobjsub->prim_pad[0];
    old_prim_pad1 = mobjsub->prim_pad[1];

    mobjsub->pad00 = ((u16)old_siz << 8) | old_fmt;
    mobjsub->fmt = (u8)(old_pad00 >> 8);
    mobjsub->siz = (u8)(old_pad00 & 0xffu);
    ndsRelocSwapS16Pair((s16 *)&mobjsub->unk08, (s16 *)&mobjsub->unk0A);
    ndsRelocSwapS16Pair((s16 *)&mobjsub->unk0C, (s16 *)&mobjsub->unk0E);

    mobjsub->flags = ((u16)old_block_siz << 8) | old_block_fmt;
    mobjsub->block_fmt = (u8)(old_flags >> 8);
    mobjsub->block_siz = (u8)(old_flags & 0xffu);
    ndsRelocSwapS16Pair((s16 *)&mobjsub->block_dxt,
                        (s16 *)&mobjsub->unk36);
    ndsRelocSwapS16Pair((s16 *)&mobjsub->unk38, (s16 *)&mobjsub->unk3A);

    mobjsub->prim_l = old_prim_pad1;
    mobjsub->prim_m = old_prim_pad0;
    mobjsub->prim_pad[0] = old_prim_m;
    mobjsub->prim_pad[1] = old_prim_l;
    ndsRelocReverseColorPackBytes(&mobjsub->primcolor);
    ndsRelocReverseColorPackBytes(&mobjsub->envcolor);
    ndsRelocReverseColorPackBytes(&mobjsub->blendcolor);
    ndsRelocReverseColorPackBytes(&mobjsub->light1color);
    ndsRelocReverseColorPackBytes(&mobjsub->light2color);

    ndsRelocRecordMObjSubNormalize(loaded, mobjsub);
}

static void ndsRelocNormalizeMObjSubTable(NDSRelocLoadedFile *loaded,
                                          u32 offset,
                                          u32 head_count)
{
    MObjSub ***p_mobjsubs;
    u32 head_index;

    if ((loaded == NULL) ||
        (loaded->asset_id != NDS_RELOC_ASSET_MV_COMMON) ||
        (ndsRelocRangeInLoadedFile(loaded, offset, sizeof(*p_mobjsubs)) ==
         FALSE))
    {
        gNdsOpeningRoomRelocMObjSubNormalizeFailCount++;
        return;
    }

    p_mobjsubs = (MObjSub ***)((u8 *)loaded->data + offset);
    for (head_index = 0; head_index < head_count; head_index++)
    {
        MObjSub **mobjsubs;
        u32 list_index;

        if (ndsRelocPointerRangeInLoadedFile(
                loaded, &p_mobjsubs[head_index], sizeof(*p_mobjsubs)) ==
            FALSE)
        {
            gNdsOpeningRoomRelocMObjSubNormalizeFailCount++;
            return;
        }

        mobjsubs = p_mobjsubs[head_index];
        if (mobjsubs == NULL)
        {
            continue;
        }
        if (ndsRelocPointerRangeInLoadedFile(
                loaded, mobjsubs, sizeof(*mobjsubs)) == FALSE)
        {
            gNdsOpeningRoomRelocMObjSubNormalizeFailCount++;
            return;
        }

        for (list_index = 0; list_index < 8u; list_index++)
        {
            MObjSub *mobjsub;

            if (ndsRelocPointerRangeInLoadedFile(
                    loaded, &mobjsubs[list_index], sizeof(*mobjsubs)) ==
                FALSE)
            {
                gNdsOpeningRoomRelocMObjSubNormalizeFailCount++;
                return;
            }

            mobjsub = mobjsubs[list_index];
            if (mobjsub == NULL)
            {
                break;
            }
            if (ndsRelocPointerRangeInLoadedFile(
                    loaded, mobjsub, sizeof(*mobjsub)) == FALSE)
            {
                gNdsOpeningRoomRelocMObjSubNormalizeFailCount++;
                return;
            }
            ndsRelocNormalizeMObjSubMixedFields(loaded, mobjsub);
        }
    }
}

static void ndsRelocNormalizeMVCommonMObjSubs(NDSRelocLoadedFile *loaded)
{
    if ((loaded == NULL) ||
        (loaded->asset_id != NDS_RELOC_ASSET_MV_COMMON))
    {
        return;
    }

    ndsRelocNormalizeMObjSubTable(
        loaded, NDS_RELOC_SYMBOL_MVCOMMON_BACKGROUND_MOBJ, 52u);
    ndsRelocNormalizeMObjSubTable(
        loaded, NDS_RELOC_SYMBOL_MVCOMMON_LOGO_MOBJ, 2u);
    ndsRelocNormalizeMObjSubTable(
        loaded, NDS_RELOC_SYMBOL_MVCOMMON_CLOSEUP_AIR_MOBJ, 4u);
    ndsRelocNormalizeMObjSubTable(
        loaded, NDS_RELOC_SYMBOL_MVCOMMON_CLOSEUP_GROUND_MOBJ, 2u);
    ndsRelocNormalizeMObjSubTable(
        loaded, NDS_RELOC_SYMBOL_MVCOMMON_DESK_GROUND_MOBJ, 8u);
    ndsRelocNormalizeMObjSubTable(loaded,
                                  NDS_RELOC_SYMBOL_MVCOMMON_SPOTLIGHT_MOBJ,
                                  2u);
}

static s32 ndsRelocResolveSymbolOffset(NDSRelocLoadedFile *loaded,
                                        const void *symbol, u32 *out_offset)
{
    uintptr_t raw_symbol = (uintptr_t)symbol;
    u32 i;

    if ((loaded == NULL) || (out_offset == NULL))
    {
        return FALSE;
    }

    if (symbol == &llN64LogoSprite)
    {
        if (loaded->asset_id != NDS_RELOC_ASSET_N64_LOGO)
        {
            return FALSE;
        }
        *out_offset = NDS_RELOC_SYMBOL_N64_LOGO_SPRITE;
        return TRUE;
    }
    if (loaded->asset_id == NDS_RELOC_ASSET_IF_COMMON_ANNOUNCE)
    {
        if (symbol == &llIFCommonAnnounceCommonLetterASprite)
        {
            *out_offset = NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_A;
            return TRUE;
        }
        if (symbol == &llIFCommonAnnounceCommonLetterBSprite)
        {
            *out_offset = NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_B;
            return TRUE;
        }
        if (symbol == &llIFCommonAnnounceCommonLetterCSprite)
        {
            *out_offset = NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_C;
            return TRUE;
        }
        if (symbol == &llIFCommonAnnounceCommonLetterDSprite)
        {
            *out_offset = NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_D;
            return TRUE;
        }
        if (symbol == &llIFCommonAnnounceCommonLetterESprite)
        {
            *out_offset = NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_E;
            return TRUE;
        }
        if (symbol == &llIFCommonAnnounceCommonLetterFSprite)
        {
            *out_offset = NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_F;
            return TRUE;
        }
        if (symbol == &llIFCommonAnnounceCommonLetterGSprite)
        {
            *out_offset = NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_G;
            return TRUE;
        }
        if (symbol == &llIFCommonAnnounceCommonLetterHSprite)
        {
            *out_offset = NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_H;
            return TRUE;
        }
        if (symbol == &llIFCommonAnnounceCommonLetterISprite)
        {
            *out_offset = NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_I;
            return TRUE;
        }
        if (symbol == &llIFCommonAnnounceCommonLetterKSprite)
        {
            *out_offset = NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_K;
            return TRUE;
        }
        if (symbol == &llIFCommonAnnounceCommonLetterLSprite)
        {
            *out_offset = NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_L;
            return TRUE;
        }
        if (symbol == &llIFCommonAnnounceCommonLetterMSprite)
        {
            *out_offset = NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_M;
            return TRUE;
        }
        if (symbol == &llIFCommonAnnounceCommonLetterNSprite)
        {
            *out_offset = NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_N;
            return TRUE;
        }
        if (symbol == &llIFCommonAnnounceCommonLetterOSprite)
        {
            *out_offset = NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_O;
            return TRUE;
        }
        if (symbol == &llIFCommonAnnounceCommonLetterPSprite)
        {
            *out_offset = NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_P;
            return TRUE;
        }
        if (symbol == &llIFCommonAnnounceCommonLetterRSprite)
        {
            *out_offset = NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_R;
            return TRUE;
        }
        if (symbol == &llIFCommonAnnounceCommonLetterSSprite)
        {
            *out_offset = NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_S;
            return TRUE;
        }
        if (symbol == &llIFCommonAnnounceCommonLetterUSprite)
        {
            *out_offset = NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_U;
            return TRUE;
        }
        if (symbol == &llIFCommonAnnounceCommonLetterXSprite)
        {
            *out_offset = NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_X;
            return TRUE;
        }
        if (symbol == &llIFCommonAnnounceCommonLetterYSprite)
        {
            *out_offset = NDS_RELOC_SYMBOL_IF_ANNOUNCE_LETTER_Y;
            return TRUE;
        }
    }
    if (loaded->asset_id == NDS_RELOC_ASSET_OPENING_COMMON)
    {
        if (symbol == &llMVOpeningCommonMarioCamAnimJoint)
        {
            *out_offset = NDS_RELOC_SYMBOL_OPENING_COMMON_MARIO_CAM_ANIM;
            return TRUE;
        }
        if (symbol == &llMVOpeningCommonDonkeyCamAnimJoint)
        {
            *out_offset = NDS_RELOC_SYMBOL_OPENING_COMMON_DONKEY_CAM_ANIM;
            return TRUE;
        }
        if (symbol == &llMVOpeningCommonSamusCamAnimJoint)
        {
            *out_offset = NDS_RELOC_SYMBOL_OPENING_COMMON_SAMUS_CAM_ANIM;
            return TRUE;
        }
        if (symbol == &llMVOpeningCommonFoxCamAnimJoint)
        {
            *out_offset = NDS_RELOC_SYMBOL_OPENING_COMMON_FOX_CAM_ANIM;
            return TRUE;
        }
        if (symbol == &llMVOpeningCommonLinkCamAnimJoint)
        {
            *out_offset = NDS_RELOC_SYMBOL_OPENING_COMMON_LINK_CAM_ANIM;
            return TRUE;
        }
        if (symbol == &llMVOpeningCommonYoshiCamAnimJoint)
        {
            *out_offset = NDS_RELOC_SYMBOL_OPENING_COMMON_YOSHI_CAM_ANIM;
            return TRUE;
        }
        if (symbol == &llMVOpeningCommonPikachuCamAnimJoint)
        {
            *out_offset = NDS_RELOC_SYMBOL_OPENING_COMMON_PIKACHU_CAM_ANIM;
            return TRUE;
        }
        if (symbol == &llMVOpeningCommonKirbyCamAnimJoint)
        {
            *out_offset = NDS_RELOC_SYMBOL_OPENING_COMMON_KIRBY_CAM_ANIM;
            return TRUE;
        }
    }
    if (loaded->asset_id == NDS_RELOC_ASSET_OPENING_PORTRAITS_SET1)
    {
        if (symbol == &llMVOpeningPortraitsSet1SamusSprite)
        {
            *out_offset = NDS_RELOC_SYMBOL_OPENING_PORTRAITS_SET1_SAMUS;
            return TRUE;
        }
        if (symbol == &llMVOpeningPortraitsSet1MarioSprite)
        {
            *out_offset = NDS_RELOC_SYMBOL_OPENING_PORTRAITS_SET1_MARIO;
            return TRUE;
        }
        if (symbol == &llMVOpeningPortraitsSet1FoxSprite)
        {
            *out_offset = NDS_RELOC_SYMBOL_OPENING_PORTRAITS_SET1_FOX;
            return TRUE;
        }
        if (symbol == &llMVOpeningPortraitsSet1PikachuSprite)
        {
            *out_offset = NDS_RELOC_SYMBOL_OPENING_PORTRAITS_SET1_PIKACHU;
            return TRUE;
        }
        if (symbol == &llMVOpeningPortraitsSet1CoverSprite)
        {
            *out_offset = NDS_RELOC_SYMBOL_OPENING_PORTRAITS_SET1_COVER;
            return TRUE;
        }
    }
    if (loaded->asset_id == NDS_RELOC_ASSET_OPENING_PORTRAITS_SET2)
    {
        if (symbol == &llMVOpeningPortraitsSet2LinkSprite)
        {
            *out_offset = NDS_RELOC_SYMBOL_OPENING_PORTRAITS_SET2_LINK;
            return TRUE;
        }
        if (symbol == &llMVOpeningPortraitsSet2KirbySprite)
        {
            *out_offset = NDS_RELOC_SYMBOL_OPENING_PORTRAITS_SET2_KIRBY;
            return TRUE;
        }
        if (symbol == &llMVOpeningPortraitsSet2DonkeySprite)
        {
            *out_offset = NDS_RELOC_SYMBOL_OPENING_PORTRAITS_SET2_DONKEY;
            return TRUE;
        }
        if (symbol == &llMVOpeningPortraitsSet2YoshiSprite)
        {
            *out_offset = NDS_RELOC_SYMBOL_OPENING_PORTRAITS_SET2_YOSHI;
            return TRUE;
        }
    }
    if (loaded->asset_id == NDS_RELOC_ASSET_OPENING_RUN)
    {
        if (symbol == &llMVOpeningRunWallpaperSprite)
        {
            *out_offset = NDS_RELOC_SYMBOL_OPENING_RUN_WALLPAPER;
            return TRUE;
        }
    }
    if (loaded->asset_id == NDS_RELOC_ASSET_OPENING_YAMABUKI)
    {
        if (symbol == &llMVOpeningYamabukiWallpaperSprite)
        {
            *out_offset = NDS_RELOC_SYMBOL_OPENING_YAMABUKI_WALLPAPER;
            return TRUE;
        }
    }
    if (loaded->asset_id == NDS_RELOC_ASSET_OPENING_SECTOR)
    {
        if (symbol == &llMVOpeningSectorCockpitSprite)
        {
            *out_offset = NDS_RELOC_SYMBOL_OPENING_SECTOR_COCKPIT;
            return TRUE;
        }
    }
    if (loaded->asset_id == NDS_RELOC_ASSET_MN_TITLE)
    {
        if (symbol == &llMNTitleLogoAnimFullSprite)
        {
            *out_offset = NDS_RELOC_SYMBOL_TITLE_LOGO_FULL;
            return TRUE;
        }
        if (symbol == &llMNTitleBorderUpperSprite)
        {
            *out_offset = NDS_RELOC_SYMBOL_TITLE_BORDER_UPPER;
            return TRUE;
        }
        if (symbol == &llMNTitleTMSprite)
        {
            *out_offset = NDS_RELOC_SYMBOL_TITLE_TM;
            return TRUE;
        }
        if (symbol == &llMNTitleCutoutSprite)
        {
            *out_offset = NDS_RELOC_SYMBOL_TITLE_CUTOUT;
            return TRUE;
        }
        if (symbol == &llMNTitleTMUnkSprite)
        {
            *out_offset = NDS_RELOC_SYMBOL_TITLE_TM_UNK;
            return TRUE;
        }
        if (symbol == &llMNTitleCopyrightSprite)
        {
            *out_offset = NDS_RELOC_SYMBOL_TITLE_COPYRIGHT;
            return TRUE;
        }
        if (symbol == &llMNTitlePressStartSprite)
        {
            *out_offset = NDS_RELOC_SYMBOL_TITLE_PRESS_START;
            return TRUE;
        }
        if (symbol == &llMNTitleSuperSprite)
        {
            *out_offset = NDS_RELOC_SYMBOL_TITLE_SUPER;
            return TRUE;
        }
        if (symbol == &llMNTitleSmashSprite)
        {
            *out_offset = NDS_RELOC_SYMBOL_TITLE_SMASH;
            return TRUE;
        }
        if (symbol == &llMNTitleBrosSprite)
        {
            *out_offset = NDS_RELOC_SYMBOL_TITLE_BROS;
            return TRUE;
        }
    }
    if (loaded->asset_id == NDS_RELOC_ASSET_MN_TITLE_FIRE_ANIM)
    {
        for (i = 0; i < ARRAY_COUNT(sNdsTitleFireAnimFrameSymbols); i++)
        {
            if (symbol == sNdsTitleFireAnimFrameSymbols[i].symbol)
            {
                *out_offset = sNdsTitleFireAnimFrameSymbols[i].offset;
                return TRUE;
            }
        }
    }
    if (loaded->asset_id == NDS_RELOC_ASSET_MN_COMMON)
    {
        for (i = 0; i < ARRAY_COUNT(sNdsMNCommonSymbols); i++)
        {
            if (symbol == sNdsMNCommonSymbols[i].symbol)
            {
                *out_offset = sNdsMNCommonSymbols[i].offset;
                return TRUE;
            }
        }
    }
    if (loaded->asset_id == NDS_RELOC_ASSET_MN_VS_MODE)
    {
        for (i = 0; i < ARRAY_COUNT(sNdsMNVSModeSymbols); i++)
        {
            if (symbol == sNdsMNVSModeSymbols[i].symbol)
            {
                *out_offset = sNdsMNVSModeSymbols[i].offset;
                return TRUE;
            }
        }
    }

    for (i = 0; i < (sizeof(sNdsRelocSymbolProbes) / sizeof(sNdsRelocSymbolProbes[0])); i++)
    {
        if (symbol == sNdsRelocSymbolProbes[i].marker)
        {
            if (loaded->asset_id != sNdsRelocSymbolProbes[i].asset_id)
            {
                return FALSE;
            }
            *out_offset = sNdsRelocSymbolProbes[i].offset;
            return TRUE;
        }
    }

    if (raw_symbol < loaded->data_size)
    {
        *out_offset = (u32)raw_symbol;
        return TRUE;
    }
    return FALSE;
}

static void ndsRelocProbeOpeningRoomFirstEventData(NDSRelocLoadedFile *loaded)
{
    DObjDesc *dobjdesc;
    AObjEvent32 **anim_joints;
    u32 mask = 0;
    u32 dl_count = 0;
    u32 anim_count = 0;
    u32 i;

    if ((loaded == NULL) || (loaded->asset_id != NDS_RELOC_ASSET_MV_COMMON))
    {
        return;
    }

    if (ndsRelocRangeInLoadedFile(loaded, NDS_RELOC_SYMBOL_MVCOMMON_PENCILS_DOBJ,
                                  sizeof(DObjDesc) *
                                  NDS_OPENING_ROOM_PENCILS_DOBJ_ENTRIES) == FALSE)
    {
        return;
    }
    if (ndsRelocRangeInLoadedFile(loaded, NDS_RELOC_SYMBOL_MVCOMMON_PENCILS_ANIM,
                                  sizeof(AObjEvent32 *) *
                                  NDS_OPENING_ROOM_PENCILS_ANIM_JOINTS) == FALSE)
    {
        return;
    }

    dobjdesc = (DObjDesc *)((u8 *)loaded->data +
                            NDS_RELOC_SYMBOL_MVCOMMON_PENCILS_DOBJ);
    anim_joints = (AObjEvent32 **)((u8 *)loaded->data +
                                   NDS_RELOC_SYMBOL_MVCOMMON_PENCILS_ANIM);

    if ((dobjdesc[0].id == 0) &&
        (dobjdesc[1].id == 1) &&
        (dobjdesc[2].id == 1) &&
        (dobjdesc[3].id == DOBJ_ARRAY_MAX))
    {
        gNdsOpeningRoomFirstEventPencilsDObjEntries =
            NDS_OPENING_ROOM_PENCILS_DOBJ_ENTRIES;
        mask |= NDS_OPENING_ROOM_FIRST_EVENT_DATA_DOBJ_IDS_READY;
    }

    for (i = 0; i < NDS_OPENING_ROOM_PENCILS_RENDER_DOBJS; i++)
    {
        if (ndsRelocPointerRangeInLoadedFile(loaded, dobjdesc[i].dl,
                                             sizeof(Gfx)) != FALSE)
        {
            dl_count++;
        }
    }
    gNdsOpeningRoomFirstEventPencilsDLPtrs = dl_count;
    if (dl_count == NDS_OPENING_ROOM_PENCILS_RENDER_DOBJS)
    {
        mask |= NDS_OPENING_ROOM_FIRST_EVENT_DATA_DOBJ_DLS_READY;
    }

    for (i = 0; i < NDS_OPENING_ROOM_PENCILS_ANIM_JOINTS; i++)
    {
        if (ndsRelocPointerRangeInLoadedFile(loaded, anim_joints[i],
                                             sizeof(AObjEvent32)) != FALSE)
        {
            anim_count++;
        }
    }
    gNdsOpeningRoomFirstEventPencilsAnimJoints = anim_count;
    if (anim_count == NDS_OPENING_ROOM_PENCILS_ANIM_JOINTS)
    {
        mask |= NDS_OPENING_ROOM_FIRST_EVENT_DATA_ANIM_TABLE_READY;
    }

    if (ndsRelocPointerRangeInLoadedFile(loaded, anim_joints[0],
                                         sizeof(AObjEvent32)) != FALSE)
    {
        gNdsOpeningRoomFirstEventPencilsAnimFirstOpcode =
            NDS_AOBJ_EVENT32_OPCODE(anim_joints[0]->u);
        if (gNdsOpeningRoomFirstEventPencilsAnimFirstOpcode ==
            nGCAnimEvent32SetValBlock)
        {
            mask |= NDS_OPENING_ROOM_FIRST_EVENT_DATA_ANIM_OPCODE_READY;
        }
    }

    gNdsOpeningRoomFirstEventDataMask |= mask;
    if ((gNdsOpeningRoomFirstEventDataMask &
         NDS_OPENING_ROOM_FIRST_EVENT_DATA_READY_MASK) ==
        NDS_OPENING_ROOM_FIRST_EVENT_DATA_READY_MASK)
    {
        gNdsOpeningRoomFirstEventDataResult =
            NDS_OPENING_ROOM_FIRST_EVENT_DATA_PASS;
    }
}

static void ndsRelocProbeOpeningRoomSymbols(void **files)
{
    u32 probe_mask = 0;
    u32 first_event_mask = 0;
    u32 i;

    if (files == NULL)
    {
        return;
    }

    for (i = 0; i < (sizeof(sNdsRelocSymbolProbes) / sizeof(sNdsRelocSymbolProbes[0])); i++)
    {
        NDSRelocLoadedFile *loaded =
            ndsRelocFindLoadedFileByAsset(sNdsRelocSymbolProbes[i].asset_id);
        void *file = (loaded != NULL) ? loaded->data : NULL;
        void *resolved = ndsRelocGetFileData(file, sNdsRelocSymbolProbes[i].marker);

        if (resolved == ((u8 *)file + sNdsRelocSymbolProbes[i].offset))
        {
            probe_mask |= sNdsRelocSymbolProbes[i].bit;

            if (sNdsRelocSymbolProbes[i].marker == &llMVCommonRoomPencilsDObjDesc)
            {
                first_event_mask |= NDS_OPENING_ROOM_FIRST_EVENT_PENCILS_DOBJ_READY;
                gNdsOpeningRoomFirstEventPencilsDObjOffset =
                    sNdsRelocSymbolProbes[i].offset;
            }
            if (sNdsRelocSymbolProbes[i].marker == &llMVCommonRoomPencilsAnimJoint)
            {
                first_event_mask |= NDS_OPENING_ROOM_FIRST_EVENT_PENCILS_ANIM_READY;
                gNdsOpeningRoomFirstEventPencilsAnimOffset =
                    sNdsRelocSymbolProbes[i].offset;
            }
            if (sNdsRelocSymbolProbes[i].marker == &llMVOpeningRoomScene1CamAnimJoint)
            {
                gNdsOpeningRoomLogoCameraAssetMask |=
                    NDS_OPENING_ROOM_LOGO_CAMERA_ASSET_CAMANIM_READY;
                gNdsOpeningRoomLogoCameraAnimOffset =
                    sNdsRelocSymbolProbes[i].offset;
            }
            if (sNdsRelocSymbolProbes[i].marker == &llMVOpeningRoomScene2CamAnimJoint)
            {
                gNdsOpeningRoomScene2CameraAssetMask |=
                    NDS_OPENING_ROOM_SCENE2_CAMERA_ASSET_CAMANIM_READY;
                gNdsOpeningRoomScene2CameraAnimOffset =
                    sNdsRelocSymbolProbes[i].offset;
            }
            if (sNdsRelocSymbolProbes[i].marker == &llMVCommonRoomLogoDObjDesc)
            {
                gNdsOpeningRoomLogoAssetMask |=
                    NDS_OPENING_ROOM_LOGO_ASSET_DOBJ_READY;
                gNdsOpeningRoomLogoDObjOffset =
                    sNdsRelocSymbolProbes[i].offset;
            }
            if (sNdsRelocSymbolProbes[i].marker == &llMVCommonRoomDeskDObjDesc)
            {
                gNdsOpeningRoomDeskAssetMask |=
                    NDS_OPENING_ROOM_DESK_ASSET_DOBJ_READY;
                gNdsOpeningRoomDeskDObjOffset =
                    sNdsRelocSymbolProbes[i].offset;
            }
            if (sNdsRelocSymbolProbes[i].marker == &llMVCommonRoomLogoMObjSub)
            {
                gNdsOpeningRoomLogoAssetMask |=
                    NDS_OPENING_ROOM_LOGO_ASSET_MOBJ_READY;
                gNdsOpeningRoomLogoMObjOffset =
                    sNdsRelocSymbolProbes[i].offset;
            }
            if (sNdsRelocSymbolProbes[i].marker == &llMVCommonRoomLogoMatAnimJoint)
            {
                gNdsOpeningRoomLogoAssetMask |=
                    NDS_OPENING_ROOM_LOGO_ASSET_MATANIM_READY;
                gNdsOpeningRoomLogoMatAnimOffset =
                    sNdsRelocSymbolProbes[i].offset;
            }
            if (sNdsRelocSymbolProbes[i].marker == &llMVCommonRoomBossShadowDisplayList)
            {
                gNdsOpeningRoomBossShadowAssetMask |=
                    NDS_OPENING_ROOM_BOSS_SHADOW_ASSET_DISPLAY_READY;
                gNdsOpeningRoomBossShadowDisplayListOffset =
                    sNdsRelocSymbolProbes[i].offset;
            }
            if (sNdsRelocSymbolProbes[i].marker == &llMVCommonRoomBossShadowAnimJoint)
            {
                gNdsOpeningRoomBossShadowAssetMask |=
                    NDS_OPENING_ROOM_BOSS_SHADOW_ASSET_ANIM_READY;
                gNdsOpeningRoomBossShadowAnimOffset =
                    sNdsRelocSymbolProbes[i].offset;
            }
            if (sNdsRelocSymbolProbes[i].marker == &llMVCommonRoomSpotlightDisplayList)
            {
                gNdsOpeningRoomSpotlightAssetMask |=
                    NDS_OPENING_ROOM_SPOTLIGHT_ASSET_DISPLAY_READY;
                gNdsOpeningRoomSpotlightDisplayListOffset =
                    sNdsRelocSymbolProbes[i].offset;
            }
            if (sNdsRelocSymbolProbes[i].marker == &llMVCommonRoomSpotlightMObjSub)
            {
                gNdsOpeningRoomSpotlightAssetMask |=
                    NDS_OPENING_ROOM_SPOTLIGHT_ASSET_MOBJ_READY;
                gNdsOpeningRoomSpotlightMObjOffset =
                    sNdsRelocSymbolProbes[i].offset;
            }
            if (sNdsRelocSymbolProbes[i].marker == &llMVCommonRoomSpotlightMatAnimJoint)
            {
                gNdsOpeningRoomSpotlightAssetMask |=
                    NDS_OPENING_ROOM_SPOTLIGHT_ASSET_MATANIM_READY;
                gNdsOpeningRoomSpotlightMatAnimOffset =
                    sNdsRelocSymbolProbes[i].offset;
            }
        }
    }

    gNdsOpeningRoomRelocSymbolProbeMask |= probe_mask;
    gNdsOpeningRoomFirstEventProbeMask |= first_event_mask;
    if ((gNdsOpeningRoomFirstEventProbeMask &
         NDS_OPENING_ROOM_FIRST_EVENT_READY_MASK) ==
        NDS_OPENING_ROOM_FIRST_EVENT_READY_MASK)
    {
        gNdsOpeningRoomFirstEventTick = 280;
        gNdsOpeningRoomFirstEventResult = NDS_OPENING_ROOM_FIRST_EVENT_PASS;
        ndsRelocProbeOpeningRoomFirstEventData(
            ndsRelocFindLoadedFileByAsset(NDS_RELOC_ASSET_MV_COMMON));
    }
}

static size_t ndsRelocAssetAllocSize(u32 asset_id)
{
    NDSRelocAssetHeader header;

    if ((asset_id != NDS_RELOC_ASSET_INVALID) &&
        (ndsRelocAssetReadHeader(asset_id, &header) != FALSE))
    {
        return (size_t)NDS_RELOC_ALIGN(header.data_size);
    }
    return 0;
}

void lbRelocInitSetup(LBRelocSetup *setup)
{
    (void)setup;
    sNdsRelocInitCount++;

    if (gSCManagerSceneData.scene_curr == nSCKindOpeningRoom)
    {
        gNdsOpeningRoomRelocInitCount++;
    }
}

size_t lbRelocGetFileSize(const void *file_id)
{
    u32 asset_id = ndsRelocAssetIDForToken(ndsRelocFileID(file_id));
    size_t asset_size = ndsRelocAssetAllocSize(asset_id);

    if (asset_size != 0)
    {
        return asset_size;
    }

    return sizeof(Sprite);
}

void *lbRelocGetExternHeapFile(const void *file_id, void *heap)
{
    u32 asset_id = ndsRelocAssetIDForToken(ndsRelocFileID(file_id));
    size_t asset_size;
    NDSRelocAssetHeader header;
    NDSRelocLoadedFile *loaded;

    if ((asset_id == NDS_RELOC_ASSET_INVALID) || (heap == NULL))
    {
        return heap;
    }

    asset_size = ndsRelocAssetAllocSize(asset_id);
    if (asset_size == 0)
    {
        return heap;
    }
    if (ndsRelocAssetLoadData(asset_id, heap, asset_size, &header) == FALSE)
    {
        return heap;
    }

    loaded = ndsRelocRegisterLoadedFile(asset_id, 0, heap, &header);
    if (loaded == NULL)
    {
        return heap;
    }
    if ((ndsRelocApplyWordByteSwap(loaded) != FALSE) &&
        (ndsRelocApplyInternalPointerFixups(loaded) != FALSE) &&
        (asset_id == NDS_RELOC_ASSET_N64_LOGO))
    {
        ndsRelocNormalizeN64LogoSprite(loaded);
        gNdsStartupLogoRelocResult = NDS_STARTUP_LOGO_RELOC_PASS;
        gNdsStartupLogoRelocSize = header.data_size;
    }
    return heap;
}

size_t lbRelocGetAllocSize(u32 *ids, u32 len)
{
    size_t total = 0;
    u32 i;

    for (i = 0; i < len; i++)
    {
        u32 asset_id = ndsRelocAssetIDForToken(ids[i]);
        size_t asset_size = ndsRelocAssetAllocSize(asset_id);

        total = NDS_RELOC_ALIGN(total);
        if (asset_size != 0)
        {
            total += asset_size;
        }
        else
        {
            total += sizeof(uintptr_t);
        }
    }
    return total;
}

size_t lbRelocLoadFilesExtern(u32 *ids, u32 len, void **files, void *heap)
{
    u32 i;
    u32 mask = 0;
    u32 header_mask = 0;
    u32 payload_mask = 0;
    u32 word_swap_mask = 0;
    u32 pointer_fixup_mask = 0;
    uintptr_t heap_ptr = (uintptr_t)heap;
    uintptr_t heap_start = (uintptr_t)heap;

    if ((gSCManagerSceneData.scene_curr == nSCKindOpeningRoom) ||
        (gSCManagerSceneData.scene_curr == nSCKindOpeningPortraits) ||
        (ndsOpeningIsImportedNameScene(gSCManagerSceneData.scene_curr) !=
         FALSE) ||
        (gSCManagerSceneData.scene_curr == nSCKindTitle) ||
        (gSCManagerSceneData.scene_curr == nSCKindVSMode))
    {
        ndsRelocResetLoadedFiles();
    }

    for (i = 0; i < len; i++)
    {
        u32 token = ids[i];
        u32 asset_id = ndsRelocAssetIDForToken(token);
        u32 bit = ndsRelocOpeningRoomBitForAsset(asset_id);
        NDSRelocAssetHeader header;
        size_t asset_size = 0;
        void *file_alloc = (void *)(uintptr_t)token;

        mask |= bit;

        if ((asset_id != NDS_RELOC_ASSET_INVALID) &&
            (ndsRelocAssetReadHeader(asset_id, &header) != FALSE))
        {
            header_mask |= bit;
            asset_size = (size_t)NDS_RELOC_ALIGN(header.data_size);

            if ((heap != NULL) && (asset_size != 0))
            {
                heap_ptr = NDS_RELOC_ALIGN(heap_ptr);
                file_alloc = (void *)heap_ptr;

                if (ndsRelocAssetLoadData(asset_id, file_alloc, asset_size, &header) != FALSE)
                {
                    NDSRelocLoadedFile *loaded;

                    payload_mask |= bit;
                    if (bit != 0u)
                    {
                        gNdsOpeningRoomRelocBytesLoaded += header.data_size;
                        gNdsOpeningRoomRelocLastFileID = asset_id;
                        gNdsOpeningRoomRelocLastSize = header.data_size;
                    }

                    loaded = ndsRelocRegisterLoadedFile(asset_id, bit, file_alloc, &header);
                    if (ndsRelocApplyWordByteSwap(loaded) != FALSE)
                    {
                        word_swap_mask |= bit;
                        if (ndsRelocApplyInternalPointerFixups(loaded) != FALSE)
                        {
                            pointer_fixup_mask |= bit;
                            if (asset_id == NDS_RELOC_ASSET_MV_COMMON)
                            {
                                ndsRelocNormalizeMVCommonMObjSubs(loaded);
                            }
                            if ((asset_id ==
                                 NDS_RELOC_ASSET_OPENING_PORTRAITS_SET1) ||
                                (asset_id ==
                                 NDS_RELOC_ASSET_OPENING_PORTRAITS_SET2))
                            {
                                ndsRelocNormalizeOpeningPortraitsSprites(
                                    loaded);
                            }
                            if (asset_id ==
                                NDS_RELOC_ASSET_IF_COMMON_ANNOUNCE)
                            {
                                ndsRelocNormalizeIFAnnounceMarioSprites(
                                    loaded);
                            }
                            if (asset_id ==
                                NDS_RELOC_ASSET_MN_TITLE_FIRE_ANIM)
                            {
                                ndsRelocNormalizeTitleFireSprites(loaded);
                            }
                        }
                    }
                    heap_ptr += asset_size;
                }
            }
        }

        if (files != NULL)
        {
            files[i] = file_alloc;
        }
    }

    if (gSCManagerSceneData.scene_curr == nSCKindOpeningRoom)
    {
        gNdsOpeningRoomRelocLoadCount += len;
        gNdsOpeningRoomRelocFileMask |= mask;
        gNdsOpeningRoomRelocHeaderMask |= header_mask;
        gNdsOpeningRoomRelocPayloadMask |= payload_mask;
        gNdsOpeningRoomRelocWordSwapMask |= word_swap_mask;
        gNdsOpeningRoomRelocPointerFixupMask |= pointer_fixup_mask;
        gNdsOpeningRoomRelocContentReady =
            ((gNdsOpeningRoomRelocPayloadMask & NDS_RELOC_OPENING_ROOM_FILE_MASK) ==
             NDS_RELOC_OPENING_ROOM_FILE_MASK) ? 1u : 0u;
        if (((gNdsOpeningRoomRelocWordSwapMask & NDS_RELOC_OPENING_ROOM_FILE_MASK) ==
             NDS_RELOC_OPENING_ROOM_FILE_MASK) &&
            ((gNdsOpeningRoomRelocPointerFixupMask & NDS_RELOC_OPENING_ROOM_FILE_MASK) ==
             NDS_RELOC_OPENING_ROOM_FILE_MASK))
        {
            ndsRelocProbeOpeningRoomSymbols(files);
        }
        /* Full data fixup still requires mixed-width struct fixups and
         * renderer-specific texture/display-list fixups. Keep the existing
         * gate false until those contracts are implemented and verified. */
        gNdsOpeningRoomRelocFixupReady = 0;

        if ((len == NDS_RELOC_OPENING_ROOM_FILE_COUNT) &&
            ((gNdsOpeningRoomRelocFileMask & NDS_RELOC_OPENING_ROOM_FILE_MASK) ==
             NDS_RELOC_OPENING_ROOM_FILE_MASK))
        {
            gNdsOpeningRoomRelocResult = NDS_OPENING_ROOM_RELOC_PASS;
        }
    }
    if (gSCManagerSceneData.scene_curr == nSCKindOpeningMario)
    {
        if ((ndsRelocFindLoadedFileByAsset(
                 NDS_RELOC_ASSET_IF_COMMON_ANNOUNCE) != NULL) &&
            (ndsRelocFindLoadedFileByAsset(
                 NDS_RELOC_ASSET_OPENING_COMMON) != NULL) &&
            (gNdsOpeningMarioSpriteNormalizeCount >= 5u) &&
            (gNdsOpeningMarioSpriteNormalizeFailCount == 0))
        {
            gNdsOpeningMarioRelocResult = NDS_OPENING_MARIO_RELOC_PASS;
        }
    }

    return (heap != NULL) ? (size_t)(heap_ptr - heap_start) : 0;
}

void *ndsRelocGetFileData(void *file, const void *symbol)
{
    NDSRelocLoadedFile *loaded = ndsRelocFindLoadedFileByData(file);
    u32 offset;

    if (file == NULL)
    {
        gNdsOpeningRoomRelocSymbolResolveFailCount++;
        return NULL;
    }
    if (loaded == NULL)
    {
        return file;
    }
    if ((ndsRelocResolveSymbolOffset(loaded, symbol, &offset) == FALSE) ||
        (offset >= loaded->data_size))
    {
        gNdsOpeningRoomRelocSymbolResolveFailCount++;
        return NULL;
    }

    if (loaded->asset_id != NDS_RELOC_ASSET_N64_LOGO)
    {
        gNdsOpeningRoomRelocSymbolResolveCount++;
        gNdsOpeningRoomRelocLastSymbolOffset = offset;
    }
    return (u8 *)file + offset;
}

sb32 (*dLBCommonFuncMatrixList[])(void) = { NULL };

f32 scSubsysFighterGetLightAngleX(void)
{
    return 0.0F;
}

f32 scSubsysFighterGetLightAngleY(void)
{
    return 0.0F;
}

void scSubsysFighterSetLightParams(f32 light_angle_x, f32 light_angle_y,
                                    u8 r, u8 g, u8 b, u8 a)
{
    (void)light_angle_x;
    (void)light_angle_y;
    (void)r;
    (void)g;
    (void)b;
    (void)a;
}
