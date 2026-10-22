/* Relocation asset loading, symbols and extern trees: the symbol memo and
 * resolver, the extern tree loads and their slices, the P2 fighters' allocation
 * sizes and the extern heap files. A slice of reloc_backend.c. */

/* The known-symbol table is const, so its answer for (asset, symbol) never
 * changes. lbRelocGetFileData resolves ~70 symbols a battle frame (HUD sprites
 * among them) and each scanned the whole table; a direct-mapped memo of the
 * scan's result (found with its offset, or absent) answers repeats. */
#define NDS_RELOC_SYMBOL_MEMO_SLOTS 64u
typedef struct NDSRelocSymbolMemo
{
    const void *symbol;
    u32 asset_id;
    u32 offset;
    u32 state;                  /* 0 empty, 1 in the table, 2 not in it */
} NDSRelocSymbolMemo;
static NDSRelocSymbolMemo sNdsRelocSymbolMemo[NDS_RELOC_SYMBOL_MEMO_SLOTS];

static s32 ndsRelocResolveSymbolOffset(NDSRelocLoadedFile *loaded,
                                        const void *symbol, u32 *out_offset)
{
    uintptr_t raw_symbol = (uintptr_t)symbol;
    NDSRelocSymbolMemo *memo;
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
    memo = &sNdsRelocSymbolMemo[((raw_symbol >> 2) ^ loaded->asset_id) &
                                (NDS_RELOC_SYMBOL_MEMO_SLOTS - 1u)];
    if ((memo->state == 0u) || (memo->symbol != symbol) ||
        (memo->asset_id != loaded->asset_id))
    {
        memo->symbol = symbol;
        memo->asset_id = loaded->asset_id;
        memo->state = 2u;
        for (i = 0; i < ARRAY_COUNT(sNdsKnownAssetSymbols); i++)
        {
            if ((sNdsKnownAssetSymbols[i].asset_id != NDS_RELOC_ASSET_INVALID) &&
                (loaded->asset_id == sNdsKnownAssetSymbols[i].asset_id) &&
                (symbol == sNdsKnownAssetSymbols[i].symbol))
            {
                memo->offset = sNdsKnownAssetSymbols[i].offset;
                memo->state = 1u;
                break;
            }
        }
    }
    if (memo->state == 1u)
    {
        *out_offset = memo->offset;
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
    if ((loaded->asset_id == NDS_RELOC_ASSET_GR_PUPUPU_MAP) &&
        (symbol == &llGRPupupuMapMapHeader))
    {
        *out_offset = NDS_RELOC_SYMBOL_GR_PUPUPU_MAP_HEADER;
        return TRUE;
    }
    if ((loaded->asset_id == NDS_RELOC_ASSET_GR_INISHIE_MAP) &&
        (symbol == &llGRInishieMapMapHeader))
    {
        *out_offset = NDS_RELOC_SYMBOL_GR_INISHIE_MAP_HEADER;
        return TRUE;
    }
    if ((loaded->asset_id == NDS_RELOC_ASSET_GR_HYRULE_MAP) &&
        (symbol == &llGRHyruleMapMapHeader))
    {
        *out_offset = NDS_RELOC_SYMBOL_GR_HYRULE_MAP_HEADER;
        return TRUE;
    }
#if NDS_P2_1P_GAME
    if ((loaded->asset_id == NDS_RELOC_ASSET_GR_LAST_MAP) &&
        (symbol == &llGRLastMapMapHeader))
    {
        *out_offset = NDS_RELOC_SYMBOL_GR_LAST_MAP_HEADER;
        return TRUE;
    }
#endif
    if ((loaded->asset_id == NDS_RELOC_ASSET_STAGE_DREAM_LAND) &&
        (symbol == &llStageDreamLandSprite))
    {
        *out_offset = NDS_RELOC_SYMBOL_STAGE_DREAM_LAND_SPRITE;
        return TRUE;
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

    if (raw_symbol < ndsRelocNativeSourceSize(loaded))
    {
        *out_offset = (u32)raw_symbol;
        return TRUE;
    }
    if ((raw_symbol >= 0x02000000u) && (raw_symbol < 0x04000000u))
    {
        uintptr_t initialized_offset = *(const uintptr_t *)symbol;

        if (initialized_offset < ndsRelocNativeSourceSize(loaded))
        {
            *out_offset = (u32)initialized_offset;
            return TRUE;
        }
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

static s32 ndsRelocSeenAsset(u32 *seen, u32 seen_count, u32 asset_id)
{
    u32 i;

    for (i = 0; i < seen_count; i++)
    {
        if (seen[i] == asset_id)
        {
            return TRUE;
        }
    }
    return FALSE;
}

static s32 ndsRelocAddSeenAsset(u32 *seen, u32 *seen_count, u32 asset_id)
{
    if ((seen == NULL) || (seen_count == NULL))
    {
        return FALSE;
    }
    if (ndsRelocSeenAsset(seen, *seen_count, asset_id) != FALSE)
    {
        return FALSE;
    }
    if (*seen_count >= NDS_RELOC_EXTERN_FILE_ID_CAPACITY)
    {
        ndsRelocRecordExternalFixupFail(asset_id);
        return FALSE;
    }
    seen[*seen_count] = asset_id;
    (*seen_count)++;
    return TRUE;
}

static size_t ndsRelocExternTreeAllocSize(u32 asset_id, u32 *seen,
                                          u32 *seen_count)
{
    NDSRelocAssetHeader header;
    u16 extern_ids[NDS_RELOC_EXTERN_FILE_ID_CAPACITY];
    u32 extern_count = 0;
    size_t total;
    u32 i;

    if ((asset_id == NDS_RELOC_ASSET_INVALID) ||
        (ndsRelocAddSeenAsset(seen, seen_count, asset_id) == FALSE))
    {
        return 0;
    }
    if (ndsRelocFindStatusNode(sNdsRelocStatusBuffer,
                               sNdsRelocStatusBufferCount,
                               asset_id) != NULL)
    {
        return 0;
    }
    if (ndsRelocAssetReadHeader(asset_id, &header) == FALSE)
    {
        return 0;
    }

    total = (size_t)NDS_RELOC_ALIGN(header.data_size);
    if ((header.extern_file_ids_num == 0u) ||
        (header.extern_file_ids_num > NDS_RELOC_EXTERN_FILE_ID_CAPACITY) ||
        (ndsRelocAssetReadExternFileIDs(asset_id,
                                        extern_ids,
                                        NDS_RELOC_EXTERN_FILE_ID_CAPACITY,
                                        &extern_count) == FALSE))
    {
        return total;
    }

    for (i = 0; i < extern_count; i++)
    {
        u32 dep_asset_id = ndsRelocAssetIDForToken(extern_ids[i]);

        if ((ndsRelocNativeEntryOwnsDependency(asset_id, dep_asset_id) !=
             FALSE) ||
            (ndsShieldPoseServesDependency(asset_id, dep_asset_id) != FALSE))
        {
            continue;
        }
        total = (size_t)NDS_RELOC_ALIGN(total);
        total += ndsRelocExternTreeAllocSize(dep_asset_id, seen, seen_count);
    }
    return total;
}

/* ndsRelocExternTreeAllocSize less every file already loaded: the tree
 * loader takes a loaded file as it is (ndsRelocLoadExternTreeAsset), so this
 * is the heap a tree load still writes. */
static size_t ndsRelocExternTreeUnloadedSizeFor(u32 asset_id, u32 *seen,
                                                u32 *seen_count)
{
    NDSRelocAssetHeader header;
    u16 extern_ids[NDS_RELOC_EXTERN_FILE_ID_CAPACITY];
    u32 extern_count = 0;
    size_t total;
    u32 i;

    if ((asset_id == NDS_RELOC_ASSET_INVALID) ||
        (ndsRelocAddSeenAsset(seen, seen_count, asset_id) == FALSE) ||
        (ndsRelocFindStatusNode(sNdsRelocStatusBuffer,
                                sNdsRelocStatusBufferCount,
                                asset_id) != NULL) ||
        (ndsRelocFindLoadedFileByAsset(asset_id) != NULL) ||
        (ndsRelocAssetReadHeader(asset_id, &header) == FALSE))
    {
        return 0;
    }
    total = (size_t)NDS_RELOC_ALIGN(header.data_size);
    if ((header.extern_file_ids_num == 0u) ||
        (header.extern_file_ids_num > NDS_RELOC_EXTERN_FILE_ID_CAPACITY) ||
        (ndsRelocAssetReadExternFileIDs(asset_id,
                                        extern_ids,
                                        NDS_RELOC_EXTERN_FILE_ID_CAPACITY,
                                        &extern_count) == FALSE))
    {
        return total;
    }
    for (i = 0; i < extern_count; i++)
    {
        u32 dep_asset_id = ndsRelocAssetIDForToken(extern_ids[i]);

        if ((ndsRelocNativeEntryOwnsDependency(asset_id, dep_asset_id) !=
             FALSE) ||
            (ndsShieldPoseServesDependency(asset_id, dep_asset_id) != FALSE))
        {
            continue;
        }
        total = (size_t)NDS_RELOC_ALIGN(total);
        total += ndsRelocExternTreeUnloadedSizeFor(dep_asset_id, seen,
                                                   seen_count);
    }
    return total;
}

/* The stage select's preview loads a stage's packet assets a tic apiece,
 * then its map file's tree: that load needs only this much heap. */
size_t ndsRelocExternTreeUnloadedSize(const void *file_id)
{
    u32 seen[NDS_RELOC_EXTERN_FILE_ID_CAPACITY];
    u32 seen_count = 0u;

    return ndsRelocExternTreeUnloadedSizeFor(
        ndsRelocAssetIDForToken(ndsRelocFileID(file_id)), seen, &seen_count);
}

/* One file and what its tree still needs, loaded as lbRelocGetExternHeapFile
 * loads a tree: into a heap taken from the current region, named in the
 * scene's status buffer (ndsRelocEnsureLoadedAsset is the battle's preload,
 * after its tree loads, and may take a harness buffer instead). */
s32 ndsRelocLoadAssetTree(u32 asset_id)
{
    u32 seen[NDS_RELOC_EXTERN_FILE_ID_CAPACITY];
    u32 seen_count = 0u;
    u32 unused = 0u;
    size_t bytes;
    uintptr_t heap_ptr;

    ndsRelocPrepareSceneCache();
    bytes = ndsRelocExternTreeUnloadedSizeFor(asset_id, seen, &seen_count);
    /* A loaded tree is taken as it is and writes nothing. */
    heap_ptr = (bytes != 0u) ?
        (uintptr_t)syTaskmanMalloc(bytes, NDS_RELOC_ALIGN_BYTES) :
        (uintptr_t)&unused;
    return (ndsRelocLoadExternTreeAsset(asset_id, &heap_ptr) != NULL) ?
        TRUE : FALSE;
}

typedef struct NDSP2FighterPayloadSizeRow
{
    u16 asset_id;
    u16 payload_units_16;
} NDSP2FighterPayloadSizeRow;

#define NDS_P2_PAYLOAD_SIZE_ROW(id_, bytes_) \
    { (u16)(id_), (u16)((bytes_) >> 4) },
static const NDSP2FighterPayloadSizeRow sNdsP2BaseFighterPayloadSizes[] =
{
    NDS_P2_BASE_FIGHTER_PAYLOAD_SIZE_ROWS(NDS_P2_PAYLOAD_SIZE_ROW)
};
#if NDS_P2_LUIGI
static const NDSP2FighterPayloadSizeRow sNdsP2LuigiPayloadSizes[] =
{
    NDS_P2_LUIGI_PAYLOAD_SIZE_ROWS(NDS_P2_PAYLOAD_SIZE_ROW)
};
#endif
#if NDS_P2_DONKEY
static const NDSP2FighterPayloadSizeRow sNdsP2DonkeyPayloadSizes[] =
{
    NDS_P2_DONKEY_PAYLOAD_SIZE_ROWS(NDS_P2_PAYLOAD_SIZE_ROW)
};
#endif
#if NDS_P2_CAPTAIN
static const NDSP2FighterPayloadSizeRow sNdsP2CaptainPayloadSizes[] =
{
    NDS_P2_CAPTAIN_PAYLOAD_SIZE_ROWS(NDS_P2_PAYLOAD_SIZE_ROW)
};
#endif
static const NDSP2FighterPayloadSizeRow sNdsP2SamusPayloadSizes[] =
{
    NDS_P2_SAMUS_PAYLOAD_SIZE_ROWS(NDS_P2_PAYLOAD_SIZE_ROW)
};
#if NDS_P2_LINK
static const NDSP2FighterPayloadSizeRow sNdsP2LinkPayloadSizes[] =
{
    NDS_P2_LINK_PAYLOAD_SIZE_ROWS(NDS_P2_PAYLOAD_SIZE_ROW)
};
#endif
static const NDSP2FighterPayloadSizeRow sNdsP2PikachuPayloadSizes[] =
{
    NDS_P2_PIKACHU_PAYLOAD_SIZE_ROWS(NDS_P2_PAYLOAD_SIZE_ROW)
};
static const NDSP2FighterPayloadSizeRow sNdsP2YoshiPayloadSizes[] =
{
    NDS_P2_YOSHI_PAYLOAD_SIZE_ROWS(NDS_P2_PAYLOAD_SIZE_ROW)
};
static const NDSP2FighterPayloadSizeRow sNdsP2NessPayloadSizes[] =
{
    NDS_P2_NESS_PAYLOAD_SIZE_ROWS(NDS_P2_PAYLOAD_SIZE_ROW)
};
static const NDSP2FighterPayloadSizeRow sNdsP2PurinPayloadSizes[] =
{
    NDS_P2_PURIN_PAYLOAD_SIZE_ROWS(NDS_P2_PAYLOAD_SIZE_ROW)
};
static const NDSP2FighterPayloadSizeRow sNdsP2KirbyPayloadSizes[] =
{
    NDS_P2_KIRBY_PAYLOAD_SIZE_ROWS(NDS_P2_PAYLOAD_SIZE_ROW)
};
#undef NDS_P2_PAYLOAD_SIZE_ROW

static size_t ndsRelocP2FindGeneratedPayloadSize(
    const NDSP2FighterPayloadSizeRow *rows, u32 count, u32 asset_id)
{
    u32 lo = 0u;
    u32 hi = count;

    while (lo < hi)
    {
        u32 mid = lo + ((hi - lo) >> 1);
        u32 row_id = (u32)rows[mid].asset_id;

        if (asset_id < row_id)
        {
            hi = mid;
        }
        else if (asset_id > row_id)
        {
            lo = mid + 1u;
        }
        else
        {
            return (size_t)rows[mid].payload_units_16 << 4;
        }
    }
    return 0u;
}

static size_t ndsRelocP2GeneratedPayloadSize(u32 asset_id)
{
    size_t size = ndsRelocP2FindGeneratedPayloadSize(
        sNdsP2BaseFighterPayloadSizes,
        sizeof(sNdsP2BaseFighterPayloadSizes) /
            sizeof(sNdsP2BaseFighterPayloadSizes[0]),
        asset_id);

    if (size != 0u)
    {
        return size;
    }
#if NDS_P2_LUIGI
    size = ndsRelocP2FindGeneratedPayloadSize(
        sNdsP2LuigiPayloadSizes,
        sizeof(sNdsP2LuigiPayloadSizes) / sizeof(sNdsP2LuigiPayloadSizes[0]),
        asset_id);
    if (size != 0u) return size;
#endif
#if NDS_P2_DONKEY
    size = ndsRelocP2FindGeneratedPayloadSize(
        sNdsP2DonkeyPayloadSizes,
        sizeof(sNdsP2DonkeyPayloadSizes) / sizeof(sNdsP2DonkeyPayloadSizes[0]),
        asset_id);
    if (size != 0u) return size;
#endif
#if NDS_P2_CAPTAIN
    size = ndsRelocP2FindGeneratedPayloadSize(
        sNdsP2CaptainPayloadSizes,
        sizeof(sNdsP2CaptainPayloadSizes) /
            sizeof(sNdsP2CaptainPayloadSizes[0]),
        asset_id);
    if (size != 0u) return size;
#endif
    size = ndsRelocP2FindGeneratedPayloadSize(
        sNdsP2SamusPayloadSizes,
        sizeof(sNdsP2SamusPayloadSizes) / sizeof(sNdsP2SamusPayloadSizes[0]),
        asset_id);
    if (size != 0u) return size;
#if NDS_P2_LINK
    size = ndsRelocP2FindGeneratedPayloadSize(
        sNdsP2LinkPayloadSizes,
        sizeof(sNdsP2LinkPayloadSizes) / sizeof(sNdsP2LinkPayloadSizes[0]),
        asset_id);
    if (size != 0u) return size;
#endif
    size = ndsRelocP2FindGeneratedPayloadSize(
        sNdsP2PikachuPayloadSizes,
        sizeof(sNdsP2PikachuPayloadSizes) /
            sizeof(sNdsP2PikachuPayloadSizes[0]),
        asset_id);
    if (size != 0u) return size;
    size = ndsRelocP2FindGeneratedPayloadSize(
        sNdsP2YoshiPayloadSizes,
        sizeof(sNdsP2YoshiPayloadSizes) / sizeof(sNdsP2YoshiPayloadSizes[0]),
        asset_id);
    if (size != 0u) return size;
    size = ndsRelocP2FindGeneratedPayloadSize(
        sNdsP2NessPayloadSizes,
        sizeof(sNdsP2NessPayloadSizes) / sizeof(sNdsP2NessPayloadSizes[0]),
        asset_id);
    if (size != 0u) return size;
    size = ndsRelocP2FindGeneratedPayloadSize(
        sNdsP2PurinPayloadSizes,
        sizeof(sNdsP2PurinPayloadSizes) / sizeof(sNdsP2PurinPayloadSizes[0]),
        asset_id);
    if (size != 0u) return size;
    size = ndsRelocP2FindGeneratedPayloadSize(
        sNdsP2KirbyPayloadSizes,
        sizeof(sNdsP2KirbyPayloadSizes) / sizeof(sNdsP2KirbyPayloadSizes[0]),
        asset_id);
    if (size != 0u) return size;
    return 0u;
}

static NDSRelocLoadedFile *ndsRelocLoadExternTreeAsset(u32 asset_id,
                                                       uintptr_t *heap_ptr)
{
    NDSRelocLoadedFile *loaded;
    NDSRelocAssetHeader header;
    u32 i;
    void *status_file;
    void *file_alloc;
    size_t asset_size;
    u16 extern_ids[NDS_RELOC_EXTERN_FILE_ID_CAPACITY];
    u32 extern_count = 0u;
    sb32 is_generated_payload = FALSE;

    if ((asset_id == NDS_RELOC_ASSET_INVALID) || (heap_ptr == NULL))
    {
        return NULL;
    }

    status_file = ndsRelocFindStatusNode(sNdsRelocStatusBuffer,
                                         sNdsRelocStatusBufferCount,
                                         asset_id);
    if (status_file != NULL)
    {
        return ndsRelocFindLoadedFileByData(status_file);
    }

    loaded = ndsRelocFindLoadedFileByAsset(asset_id);
    if (loaded != NULL)
    {
        if (ndsRelocFinalizeLoadedFile(loaded) == FALSE)
        {
            return NULL;
        }
        ndsRelocNormalizeGroundMapAsset(loaded);
        ndsRelocNormalizeStageDreamLandSprite(loaded);
        ndsRelocAddStatusBufferFile(asset_id, loaded->data);
        return loaded;
    }

    asset_size = ndsRelocP2GeneratedPayloadSize(asset_id);
    if (asset_size != 0u)
    {
        is_generated_payload = TRUE;
    }
    else
    {
        asset_size = ndsRelocAssetAllocSize(asset_id);
        if ((asset_size == 0u) ||
            (ndsRelocAssetReadHeader(asset_id, &header) == FALSE))
        {
            ndsRelocRecordExternalFixupFail(asset_id);
            return NULL;
        }
    }
    if (asset_size == 0u)
    {
        ndsRelocRecordExternalFixupFail(asset_id);
        return NULL;
    }

    *heap_ptr = NDS_RELOC_ALIGN(*heap_ptr);
    file_alloc = (void *)*heap_ptr;
    *heap_ptr += asset_size;

    if (is_generated_payload != FALSE)
    {
        if ((ndsRelocAssetLoadDataAndExternIDs(
                 asset_id, file_alloc, asset_size, &header, extern_ids,
                 NDS_RELOC_EXTERN_FILE_ID_CAPACITY, &extern_count) == FALSE) ||
            ((size_t)NDS_RELOC_ALIGN(header.data_size) != asset_size))
        {
            ndsRelocRecordExternalFixupFail(asset_id);
            return NULL;
        }
    }
    else
    if (ndsRelocAssetLoadData(asset_id, file_alloc, asset_size, &header) ==
        FALSE)
    {
        ndsRelocRecordExternalFixupFail(asset_id);
        return NULL;
    }

    if (is_generated_payload != FALSE)
    {
        loaded = ndsRelocRegisterLoadedFileImpl(
            asset_id, 0, file_alloc, &header, extern_ids, extern_count, TRUE);
    }
    else
    {
        loaded = ndsRelocRegisterLoadedFile(asset_id, 0, file_alloc, &header);
    }
    if (loaded == NULL)
    {
        ndsRelocRecordExternalFixupFail(asset_id);
        return NULL;
    }
    /* loaded->data, not file_alloc: a compact polygon model's record names
     * its source offsets from a base below its storage. */
    ndsRelocAddStatusBufferFile(asset_id, loaded->data);

    if (ndsRelocApplyWordByteSwap(loaded) == FALSE)
    {
        return NULL;
    }

    for (i = 0; i < loaded->extern_count; i++)
    {
        u32 dep_asset_id = ndsRelocAssetIDForToken(loaded->extern_file_ids[i]);

        /* A ShieldPose file a native guard package answers for this Main
         * (ndsShieldPoseResolveExternalFixup) is never read raw: the size
         * pass above skipped it too, so it is neither reserved nor loaded. */
        if ((ndsRelocNativeEntryOwnsDependency(asset_id, dep_asset_id) !=
             FALSE) ||
            (ndsShieldPoseServesDependency(asset_id, dep_asset_id) != FALSE))
        {
            continue;
        }

        /* AN UNRESOLVED DEPENDENCY IS THE COMMON FAILURE AND IT USED TO BE
         * ANONYMOUS. ndsRelocLoadExternTreeAsset returns NULL for an INVALID
         * asset id without recording anything, so the only evidence was one
         * fixup failure charged to the PARENT -- naming the file that has the
         * dependency rather than the dependency that is missing. Publishing
         * the raw token here turns four boot-and-disassemble rounds into one
         * counter read (Castle, 2026-09-03: token 0x5a, the wallpaper, rowed
         * for its path but not for its token). */
        if (dep_asset_id == NDS_RELOC_ASSET_INVALID)
        {
            gNdsRelocUnresolvedDepCount++;
            gNdsRelocUnresolvedDepToken = loaded->extern_file_ids[i];
            gNdsRelocUnresolvedDepParent = asset_id;
        }
        if (ndsRelocLoadExternTreeAsset(dep_asset_id, heap_ptr) == NULL)
        {
            loaded->external_fixup_fail_count++;
            ndsRelocRecordExternalFixupFail(asset_id);
            return NULL;
        }
    }

    if (ndsRelocFinalizeLoadedFile(loaded) == FALSE)
    {
        return NULL;
    }
    ndsRelocNormalizeGroundMapAsset(loaded);
    ndsRelocNormalizeStageDreamLandSprite(loaded);
    return loaded;
}

/* Character-select fighter closures use the same reloc graph as the blocking
 * loader above, but one synchronous recursive walk can consume many presented
 * frames' ARM9 budget. Keep the DFS cursor in the caller's existing fixed
 * fighter arena and stream only a bounded payload range per continuation.
 *
 * The O2R parser in nds_reloc_assets.c defines a 0x40-byte resource header,
 * followed by file id/reloc offsets/extern count (12 bytes), the u16 extern-id
 * table, then a u32 payload-size word. The public stream API reads raw file
 * offsets, so the payload starts at 0x50 + 2 * extern_count. */
#define NDS_RELOC_EXTERN_TREE_SLICE_O2R_DATA_BASE 0x50u
#define NDS_RELOC_EXTERN_TREE_SLICE_MAX_DEPTH NDS_RELOC_LOADED_FILE_CAPACITY

enum
{
    NDS_RELOC_EXTERN_TREE_SLICE_FAIL = 0,
    NDS_RELOC_EXTERN_TREE_SLICE_IN_PROGRESS = 1,
    NDS_RELOC_EXTERN_TREE_SLICE_DONE = 2
};

typedef struct NDSRelocExternTreeSliceFrame
{
    u32 asset_id;
    u16 next_extern;
    u16 reserved;
} NDSRelocExternTreeSliceFrame;

typedef struct NDSRelocExternTreeSlice
{
    u32 root_token;
    u32 root_asset_id;
    u32 depth;
    u32 root_started;
    NDSRelocExternTreeSliceFrame frames[NDS_RELOC_EXTERN_TREE_SLICE_MAX_DEPTH];

    NdsRelocAssetStream stream;
    NDSRelocAssetHeader current_header;
    void *current_data;
    SYMallocRegion *current_region;
    void *current_alloc_mark;
    u32 current_asset_id;
    u32 current_data_offset;
    u32 current_payload_read;
    u32 current_positioned;
    u32 current_active;
} NDSRelocExternTreeSlice;

_Static_assert(sizeof(NDSRelocExternTreeSlice) <= 1024u,
               "CSS extern-tree slice cursor must stay under 1 KiB");

static void ndsRelocExternTreeSliceCleanupCurrent(
    NDSRelocExternTreeSlice *slice)
{
    if (slice == NULL)
    {
        return;
    }

    ndsRelocAssetStreamClose(&slice->stream);
    if (slice->current_data != NULL)
    {
        size_t bytes = (size_t)NDS_RELOC_ALIGN(slice->current_header.data_size);

        if (bytes != 0u)
        {
            ndsRelocReleaseHeapRange(slice->current_data, bytes);
        }
    }
    /* The slice loader is used inside a resettable taskman subarena. An active
     * asset is always the most recent allocation in that arena until publish,
     * so cancellation can reclaim the payload immediately instead of waiting
     * for the caller to reset the entire resident block. */
    if ((slice->current_region != NULL) &&
        (slice->current_alloc_mark != NULL) &&
        ((uintptr_t)slice->current_alloc_mark >=
         (uintptr_t)slice->current_region->start) &&
        ((uintptr_t)slice->current_alloc_mark <=
         (uintptr_t)slice->current_region->ptr) &&
        ((uintptr_t)slice->current_region->ptr <=
         (uintptr_t)slice->current_region->end))
    {
        slice->current_region->ptr = slice->current_alloc_mark;
    }
    slice->current_data = NULL;
    slice->current_region = NULL;
    slice->current_alloc_mark = NULL;
    slice->current_asset_id = NDS_RELOC_ASSET_INVALID;
    slice->current_data_offset = 0u;
    slice->current_payload_read = 0u;
    slice->current_positioned = 0u;
    slice->current_active = 0u;
}

static s32 ndsRelocExternTreeSliceFail(NDSRelocExternTreeSlice *slice)
{
    ndsRelocExternTreeSliceCleanupCurrent(slice);
    return NDS_RELOC_EXTERN_TREE_SLICE_FAIL;
}

static s32 ndsRelocExternTreeSliceReadSequential(
    NDSRelocExternTreeSlice *slice, void *dst, u32 bytes, sb32 is_payload)
{
    if ((slice == NULL) || (dst == NULL) || (bytes == 0u) ||
        (ndsRelocAssetStreamReadNext(&slice->stream, dst, bytes) == FALSE))
    {
        return FALSE;
    }
    if (is_payload != FALSE)
    {
        gNdsRelocAssetPayloadReadCount++;
        NDS_K0_MARK(gNdsK0AfterGoFatReads, slice->current_asset_id);
    }
    return TRUE;
}

static s32 ndsRelocExternTreeSliceFindExisting(
    u32 asset_id, NDSRelocLoadedFile **out_loaded)
{
    NDSRelocLoadedFile *loaded;
    void *status_file;

    if (out_loaded != NULL)
    {
        *out_loaded = NULL;
    }
    status_file = ndsRelocFindStatusNode(sNdsRelocStatusBuffer,
                                         sNdsRelocStatusBufferCount,
                                         asset_id);
    if (status_file != NULL)
    {
        loaded = ndsRelocFindLoadedFileByData(status_file);
        if (loaded == NULL)
        {
            return -1;
        }
        if (out_loaded != NULL)
        {
            *out_loaded = loaded;
        }
        return 1;
    }

    loaded = ndsRelocFindLoadedFileByAsset(asset_id);
    if (loaded == NULL)
    {
        return 0;
    }
    if (ndsRelocFinalizeLoadedFile(loaded) == FALSE)
    {
        return -1;
    }
    ndsRelocNormalizeGroundMapAsset(loaded);
    ndsRelocNormalizeStageDreamLandSprite(loaded);
    ndsRelocAddStatusBufferFile(asset_id, loaded->data);
    if (out_loaded != NULL)
    {
        *out_loaded = loaded;
    }
    return 1;
}

static s32 ndsRelocExternTreeSliceStartAsset(NDSRelocExternTreeSlice *slice,
                                              u32 asset_id)
{
    const char *path;
    size_t expected_size;
    size_t asset_size;

    if ((slice == NULL) || (asset_id == NDS_RELOC_ASSET_INVALID) ||
        (slice->current_active != 0u))
    {
        return FALSE;
    }
    if ((ndsRelocAssetReadHeader(asset_id, &slice->current_header) == FALSE) ||
        (slice->current_header.extern_file_ids_num >
         NDS_RELOC_EXTERN_FILE_ID_CAPACITY))
    {
        ndsRelocRecordExternalFixupFail(asset_id);
        return FALSE;
    }
    asset_size = (size_t)NDS_RELOC_ALIGN(slice->current_header.data_size);
    if (asset_size == 0u)
    {
        ndsRelocRecordExternalFixupFail(asset_id);
        return FALSE;
    }

    expected_size = ndsRelocP2GeneratedPayloadSize(asset_id);
    if ((expected_size != 0u) && (expected_size != asset_size))
    {
        ndsRelocRecordExternalFixupFail(asset_id);
        return FALSE;
    }

    path = ndsRelocAssetGetPath(asset_id);
    if (ndsRelocAssetStreamOpen(&slice->stream, path) == FALSE)
    {
        ndsRelocRecordExternalFixupFail(asset_id);
        return FALSE;
    }
    slice->current_region = ndsTaskmanGetMallocRegion();
    slice->current_alloc_mark = (slice->current_region != NULL) ?
        slice->current_region->ptr : NULL;
    slice->current_data = syTaskmanMalloc(asset_size, NDS_RELOC_ALIGN_BYTES);
    if (slice->current_data == NULL)
    {
        ndsRelocAssetStreamClose(&slice->stream);
        slice->current_region = NULL;
        slice->current_alloc_mark = NULL;
        ndsRelocRecordExternalFixupFail(asset_id);
        return FALSE;
    }

    slice->current_asset_id = asset_id;
    slice->current_data_offset = NDS_RELOC_EXTERN_TREE_SLICE_O2R_DATA_BASE +
        (slice->current_header.extern_file_ids_num * sizeof(u16));
    slice->current_payload_read = 0u;
    slice->current_positioned = 0u;
    slice->current_active = 1u;
    return TRUE;
}

static s32 ndsRelocExternTreeSlicePublishCurrent(
    NDSRelocExternTreeSlice *slice)
{
    NDSRelocLoadedFile *loaded;

    if ((slice == NULL) || (slice->current_active == 0u) ||
        (slice->current_payload_read != slice->current_header.data_size))
    {
        return FALSE;
    }
    if (slice->depth >= NDS_RELOC_EXTERN_TREE_SLICE_MAX_DEPTH)
    {
        ndsRelocRecordExternalFixupFail(slice->current_asset_id);
        return FALSE;
    }
    ndsRelocAssetStreamClose(&slice->stream);
    loaded = ndsRelocRegisterLoadedFile(slice->current_asset_id, 0u,
                                         slice->current_data,
                                         &slice->current_header);
    if (loaded == NULL)
    {
        ndsRelocRecordExternalFixupFail(slice->current_asset_id);
        return FALSE;
    }
    ndsRelocAddStatusBufferFile(slice->current_asset_id, slice->current_data);
    slice->frames[slice->depth].asset_id = slice->current_asset_id;
    slice->frames[slice->depth].next_extern = 0u;
    slice->frames[slice->depth].reserved = 0u;
    slice->depth++;
    slice->current_data = NULL;
    slice->current_region = NULL;
    slice->current_alloc_mark = NULL;
    slice->current_asset_id = NDS_RELOC_ASSET_INVALID;
    slice->current_data_offset = 0u;
    slice->current_payload_read = 0u;
    slice->current_positioned = 0u;
    slice->current_active = 0u;
    return TRUE;
}

void *ndsRelocExternTreeSliceBegin(const void *file_id)
{
    NDSRelocExternTreeSlice *slice;
    u32 token = ndsRelocFileID(file_id);
    u32 asset_id = ndsRelocAssetIDForToken(token);

    if (asset_id == NDS_RELOC_ASSET_INVALID)
    {
        return NULL;
    }
    ndsRelocPrepareSceneCache();
    slice = syTaskmanMalloc(sizeof(*slice), 4u);
    if (slice == NULL)
    {
        return NULL;
    }
    memset(slice, 0, sizeof(*slice));
    slice->root_token = token;
    slice->root_asset_id = asset_id;
    slice->current_asset_id = NDS_RELOC_ASSET_INVALID;
    return slice;
}

s32 ndsRelocExternTreeSliceStep(void *cursor, u32 byte_budget,
                                u32 node_budget, void **out_root,
                                u32 *out_payload_bytes)
{
    NDSRelocExternTreeSlice *slice = cursor;
    u32 payload_bytes = 0u;
    u32 nodes_loaded = 0u;

    if (out_root != NULL)
    {
        *out_root = NULL;
    }
    if (out_payload_bytes != NULL)
    {
        *out_payload_bytes = 0u;
    }
    if ((slice == NULL) || (byte_budget < sizeof(u32)) || (node_budget == 0u))
    {
        return ndsRelocExternTreeSliceFail(slice);
    }

    for (;;)
    {
        NDSRelocLoadedFile *loaded;
        s32 existing;

        if ((slice->root_started == 0u) && (slice->current_active == 0u) &&
            (slice->depth == 0u))
        {
            existing = ndsRelocExternTreeSliceFindExisting(
                slice->root_asset_id, &loaded);
            if (existing < 0)
            {
                return ndsRelocExternTreeSliceFail(slice);
            }
            slice->root_started = 1u;
            if (existing != 0)
            {
                ndsFighterManagerRecordExternToken(slice->root_token,
                                                    loaded->data);
                if (out_root != NULL)
                {
                    *out_root = loaded->data;
                }
                return NDS_RELOC_EXTERN_TREE_SLICE_DONE;
            }
            if (ndsRelocExternTreeSliceStartAsset(slice,
                                                   slice->root_asset_id) == FALSE)
            {
                return ndsRelocExternTreeSliceFail(slice);
            }
        }

        if (slice->current_active != 0u)
        {
            /* StreamOpen leaves the file at byte zero. Consume the small O2R
             * prefix once, then every continuation is a sequential fread.
             * The public StreamRead API seeks to an absolute offset before
             * every read; on libfat that seek dominated small CSS slices. */
            if (slice->current_positioned == 0u)
            {
                u8 prefix[NDS_RELOC_EXTERN_TREE_SLICE_O2R_DATA_BASE +
                          (NDS_RELOC_EXTERN_FILE_ID_CAPACITY * sizeof(u16))];

                if ((slice->current_data_offset > sizeof(prefix)) ||
                    (ndsRelocExternTreeSliceReadSequential(
                         slice, prefix, slice->current_data_offset,
                         FALSE) == FALSE))
                {
                    ndsRelocRecordExternalFixupFail(slice->current_asset_id);
                    return ndsRelocExternTreeSliceFail(slice);
                }
                slice->current_positioned = 1u;
            }
            u32 remaining = slice->current_header.data_size -
                slice->current_payload_read;
            u32 allowance = byte_budget - payload_bytes;
            u32 chunk = (remaining < allowance) ? remaining : allowance;

            /* Keep every non-final slice word-aligned so the next slice starts
             * on the same u32 boundary the atomic byte-swap loop used. */
            if ((chunk < remaining) && ((chunk & (sizeof(u32) - 1u)) != 0u))
            {
                chunk &= ~(sizeof(u32) - 1u);
            }
            if (chunk == 0u)
            {
                if (payload_bytes == 0u)
                {
                    ndsRelocRecordExternalFixupFail(slice->current_asset_id);
                    return ndsRelocExternTreeSliceFail(slice);
                }
                if (out_payload_bytes != NULL)
                {
                    *out_payload_bytes = payload_bytes;
                }
                return NDS_RELOC_EXTERN_TREE_SLICE_IN_PROGRESS;
            }
            if (ndsRelocExternTreeSliceReadSequential(
                    slice,
                    (u8 *)slice->current_data + slice->current_payload_read,
                    chunk, TRUE) == FALSE)
            {
                ndsRelocRecordExternalFixupFail(slice->current_asset_id);
                return ndsRelocExternTreeSliceFail(slice);
            }
            if (ndsRelocApplyWordByteSwapRange(
                    slice->current_asset_id, slice->current_data,
                    slice->current_payload_read, chunk,
                    (slice->current_payload_read == 0u) ? TRUE : FALSE) == FALSE)
            {
                return ndsRelocExternTreeSliceFail(slice);
            }
            slice->current_payload_read += chunk;
            payload_bytes += chunk;

            if (slice->current_payload_read != slice->current_header.data_size)
            {
                if (out_payload_bytes != NULL)
                {
                    *out_payload_bytes = payload_bytes;
                }
                return NDS_RELOC_EXTERN_TREE_SLICE_IN_PROGRESS;
            }
            if (ndsRelocExternTreeSlicePublishCurrent(slice) == FALSE)
            {
                return ndsRelocExternTreeSliceFail(slice);
            }
            nodes_loaded++;
        }

        while (slice->depth != 0u)
        {
            NDSRelocExternTreeSliceFrame *frame =
                &slice->frames[slice->depth - 1u];
            u32 dep_asset_id;

            loaded = ndsRelocFindLoadedFileByAsset(frame->asset_id);
            if (loaded == NULL)
            {
                return ndsRelocExternTreeSliceFail(slice);
            }
            if (frame->next_extern < loaded->extern_count)
            {
                u16 dep_token = loaded->extern_file_ids[frame->next_extern];

                dep_asset_id = ndsRelocAssetIDForToken(dep_token);
                if (dep_asset_id == NDS_RELOC_ASSET_INVALID)
                {
                    gNdsRelocUnresolvedDepCount++;
                    gNdsRelocUnresolvedDepToken = dep_token;
                    gNdsRelocUnresolvedDepParent = frame->asset_id;
                    loaded->external_fixup_fail_count++;
                    ndsRelocRecordExternalFixupFail(frame->asset_id);
                    return ndsRelocExternTreeSliceFail(slice);
                }

                existing = ndsRelocExternTreeSliceFindExisting(dep_asset_id,
                                                                NULL);
                if (existing < 0)
                {
                    loaded->external_fixup_fail_count++;
                    ndsRelocRecordExternalFixupFail(frame->asset_id);
                    return ndsRelocExternTreeSliceFail(slice);
                }
                frame->next_extern++;
                if (existing != 0)
                {
                    continue;
                }
                if ((nodes_loaded >= node_budget) ||
                    (payload_bytes >= byte_budget))
                {
                    /* This dependency has not been started yet. Revisit it on
                     * the next continuation so the cap is exact. */
                    frame->next_extern--;
                    if (out_payload_bytes != NULL)
                    {
                        *out_payload_bytes = payload_bytes;
                    }
                    return NDS_RELOC_EXTERN_TREE_SLICE_IN_PROGRESS;
                }
                if (ndsRelocExternTreeSliceStartAsset(slice,
                                                       dep_asset_id) == FALSE)
                {
                    loaded->external_fixup_fail_count++;
                    ndsRelocRecordExternalFixupFail(frame->asset_id);
                    return ndsRelocExternTreeSliceFail(slice);
                }
                break;
            }

            if (ndsRelocFinalizeLoadedFile(loaded) == FALSE)
            {
                return ndsRelocExternTreeSliceFail(slice);
            }
            ndsRelocNormalizeGroundMapAsset(loaded);
            ndsRelocNormalizeStageDreamLandSprite(loaded);
            slice->depth--;
        }

        if (slice->current_active != 0u)
        {
            continue;
        }
        if (slice->depth == 0u)
        {
            loaded = ndsRelocFindLoadedFileByAsset(slice->root_asset_id);
            if (loaded == NULL)
            {
                return ndsRelocExternTreeSliceFail(slice);
            }
            ndsFighterManagerRecordExternToken(slice->root_token, loaded->data);
            if (out_root != NULL)
            {
                *out_root = loaded->data;
            }
            if (out_payload_bytes != NULL)
            {
                *out_payload_bytes = payload_bytes;
            }
            return NDS_RELOC_EXTERN_TREE_SLICE_DONE;
        }
    }
}

void ndsRelocExternTreeSliceCancel(void *cursor)
{
    NDSRelocExternTreeSlice *slice = cursor;

    if (slice == NULL)
    {
        return;
    }
    ndsRelocExternTreeSliceCleanupCurrent(slice);
}

void ndsRelocReleaseSceneStatusBuffers(void)
{
    sNdsRelocStatusBuffer = NULL;
    sNdsRelocStatusBufferCount = 0;
    sNdsRelocStatusBufferMax = 0;
    sNdsRelocForceStatusBuffer = NULL;
    sNdsRelocForceStatusBufferCount = 0;
    sNdsRelocForceStatusBufferMax = 0;
}

void lbRelocInitSetup(LBRelocSetup *setup)
{
    sNdsRelocInitCount++;
    ndsRelocPrepareSceneCache();

    if (setup != NULL)
    {
        sNdsRelocStatusBuffer = setup->status_buffer;
        sNdsRelocStatusBufferMax = setup->status_buffer_size;
        sNdsRelocStatusBufferCount = 0;
        sNdsRelocForceStatusBuffer = setup->force_status_buffer;
        sNdsRelocForceStatusBufferMax = setup->force_status_buffer_size;
        sNdsRelocForceStatusBufferCount = 0;
    }

    if (gSCManagerSceneData.scene_curr == nSCKindOpeningRoom)
    {
        gNdsOpeningRoomRelocInitCount++;
    }
}

typedef struct NDSP2FighterAllocSizeRow
{
    u16 asset_id;
    u16 alloc_units_16;
} NDSP2FighterAllocSizeRow;

#define NDS_P2_ALLOC_SIZE_ROW(id_, bytes_) \
    { (u16)(id_), (u16)((bytes_) >> 4) },
static const NDSP2FighterAllocSizeRow sNdsP2BaseFighterAllocSizes[] =
{
    NDS_P2_BASE_FIGHTER_ALLOC_SIZE_ROWS(NDS_P2_ALLOC_SIZE_ROW)
};
#if NDS_P2_LUIGI
static const NDSP2FighterAllocSizeRow sNdsP2LuigiAllocSizes[] =
{
    NDS_P2_LUIGI_ALLOC_SIZE_ROWS(NDS_P2_ALLOC_SIZE_ROW)
};
#endif
#if NDS_P2_DONKEY
static const NDSP2FighterAllocSizeRow sNdsP2DonkeyAllocSizes[] =
{
    NDS_P2_DONKEY_ALLOC_SIZE_ROWS(NDS_P2_ALLOC_SIZE_ROW)
};
#endif
#if NDS_P2_CAPTAIN
static const NDSP2FighterAllocSizeRow sNdsP2CaptainAllocSizes[] =
{
    NDS_P2_CAPTAIN_ALLOC_SIZE_ROWS(NDS_P2_ALLOC_SIZE_ROW)
};
#endif
static const NDSP2FighterAllocSizeRow sNdsP2SamusAllocSizes[] =
{
    NDS_P2_SAMUS_ALLOC_SIZE_ROWS(NDS_P2_ALLOC_SIZE_ROW)
};
#if NDS_P2_LINK
static const NDSP2FighterAllocSizeRow sNdsP2LinkAllocSizes[] =
{
    NDS_P2_LINK_ALLOC_SIZE_ROWS(NDS_P2_ALLOC_SIZE_ROW)
};
#endif
static const NDSP2FighterAllocSizeRow sNdsP2PikachuAllocSizes[] =
{
    NDS_P2_PIKACHU_ALLOC_SIZE_ROWS(NDS_P2_ALLOC_SIZE_ROW)
};
static const NDSP2FighterAllocSizeRow sNdsP2YoshiAllocSizes[] =
{
    NDS_P2_YOSHI_ALLOC_SIZE_ROWS(NDS_P2_ALLOC_SIZE_ROW)
};
static const NDSP2FighterAllocSizeRow sNdsP2NessAllocSizes[] =
{
    NDS_P2_NESS_ALLOC_SIZE_ROWS(NDS_P2_ALLOC_SIZE_ROW)
};
static const NDSP2FighterAllocSizeRow sNdsP2PurinAllocSizes[] =
{
    NDS_P2_PURIN_ALLOC_SIZE_ROWS(NDS_P2_ALLOC_SIZE_ROW)
};
static const NDSP2FighterAllocSizeRow sNdsP2KirbyAllocSizes[] =
{
    NDS_P2_KIRBY_ALLOC_SIZE_ROWS(NDS_P2_ALLOC_SIZE_ROW)
};
#undef NDS_P2_ALLOC_SIZE_ROW

static size_t ndsRelocP2FindGeneratedAllocSize(
    const NDSP2FighterAllocSizeRow *rows, u32 count, u32 asset_id)
{
    u32 lo = 0u;
    u32 hi = count;

    while (lo < hi)
    {
        u32 mid = lo + ((hi - lo) >> 1);
        u32 row_id = (u32)rows[mid].asset_id;

        if (asset_id < row_id)
        {
            hi = mid;
        }
        else if (asset_id > row_id)
        {
            lo = mid + 1u;
        }
        else
        {
            return (size_t)rows[mid].alloc_units_16 << 4;
        }
    }
    return 0u;
}

static size_t ndsRelocP2GeneratedAllocSize(u32 asset_id)
{
    size_t size;

    /* P2-3: BattleShip's ftManagerSetupFileSize asks the immutable reloc table
     * for every fighter motion's transitive allocation size before allocating
     * any of them.  The N64 implementation gets that answer from its ROM table;
     * opening a NitroFS path/header for every motion is a DS port artifact, not
     * source behavior.  These generated values are the same 16-byte-aligned,
     * dependency-deduplicated answer computed offline from the pinned US O2R
     * headers.  Only use them when the status buffer is empty: once a caller has
     * resident dependencies, the source size routine deliberately subtracts
     * those and the generic recursive path remains authoritative.
     *
     * Sizes are stored in 16-byte units because every source allocation is
     * reloc-aligned.  The sorted u16/u16 table is ~half the linked footprint of
     * the compiler's sparse switch table and binary search is still tiny beside
     * the NitroFS metadata walk it replaces. */
    size = ndsRelocP2FindGeneratedAllocSize(
        sNdsP2BaseFighterAllocSizes,
        sizeof(sNdsP2BaseFighterAllocSizes) /
            sizeof(sNdsP2BaseFighterAllocSizes[0]),
        asset_id);
    if (size != 0u)
    {
        return size;
    }
#if NDS_P2_LUIGI
    size = ndsRelocP2FindGeneratedAllocSize(
        sNdsP2LuigiAllocSizes,
        sizeof(sNdsP2LuigiAllocSizes) / sizeof(sNdsP2LuigiAllocSizes[0]),
        asset_id);
    if (size != 0u)
    {
        return size;
    }
#endif
#if NDS_P2_DONKEY
    size = ndsRelocP2FindGeneratedAllocSize(
        sNdsP2DonkeyAllocSizes,
        sizeof(sNdsP2DonkeyAllocSizes) / sizeof(sNdsP2DonkeyAllocSizes[0]),
        asset_id);
    if (size != 0u)
    {
        return size;
    }
#endif
#if NDS_P2_CAPTAIN
    size = ndsRelocP2FindGeneratedAllocSize(
        sNdsP2CaptainAllocSizes,
        sizeof(sNdsP2CaptainAllocSizes) / sizeof(sNdsP2CaptainAllocSizes[0]),
        asset_id);
    if (size != 0u)
    {
        return size;
    }
#endif
    size = ndsRelocP2FindGeneratedAllocSize(
        sNdsP2SamusAllocSizes,
        sizeof(sNdsP2SamusAllocSizes) / sizeof(sNdsP2SamusAllocSizes[0]),
        asset_id);
    if (size != 0u)
    {
        return size;
    }
#if NDS_P2_LINK
    size = ndsRelocP2FindGeneratedAllocSize(
        sNdsP2LinkAllocSizes,
        sizeof(sNdsP2LinkAllocSizes) / sizeof(sNdsP2LinkAllocSizes[0]),
        asset_id);
    if (size != 0u)
    {
        return size;
    }
#endif
    size = ndsRelocP2FindGeneratedAllocSize(
        sNdsP2PikachuAllocSizes,
        sizeof(sNdsP2PikachuAllocSizes) / sizeof(sNdsP2PikachuAllocSizes[0]),
        asset_id);
    if (size != 0u)
    {
        return size;
    }
    size = ndsRelocP2FindGeneratedAllocSize(
        sNdsP2YoshiAllocSizes,
        sizeof(sNdsP2YoshiAllocSizes) / sizeof(sNdsP2YoshiAllocSizes[0]),
        asset_id);
    if (size != 0u)
    {
        return size;
    }
    size = ndsRelocP2FindGeneratedAllocSize(
        sNdsP2NessAllocSizes,
        sizeof(sNdsP2NessAllocSizes) / sizeof(sNdsP2NessAllocSizes[0]),
        asset_id);
    if (size != 0u)
    {
        return size;
    }
    size = ndsRelocP2FindGeneratedAllocSize(
        sNdsP2PurinAllocSizes,
        sizeof(sNdsP2PurinAllocSizes) / sizeof(sNdsP2PurinAllocSizes[0]),
        asset_id);
    if (size != 0u)
    {
        return size;
    }
    size = ndsRelocP2FindGeneratedAllocSize(
        sNdsP2KirbyAllocSizes,
        sizeof(sNdsP2KirbyAllocSizes) / sizeof(sNdsP2KirbyAllocSizes[0]),
        asset_id);
    if (size != 0u)
    {
        return size;
    }
    return 0u;
}

size_t lbRelocGetFileSize(const void *file_id)
{
    u32 token = ndsRelocFileID(file_id);
    u32 asset_id = ndsRelocAssetIDForToken(token);
    size_t css_selected_size =
        ndsBattleShipCSSSelectedFigatreeSize(file_id);
    size_t generated_alloc_size = 0u;
    u32 seen[NDS_RELOC_EXTERN_FILE_ID_CAPACITY];
    u32 seen_count = 0;
    size_t asset_size;

    if (css_selected_size != 0u)
    {
        return css_selected_size;
    }
    if (sNdsRelocStatusBufferCount == 0)
    {
        generated_alloc_size = ndsRelocP2GeneratedAllocSize(asset_id);
        if (generated_alloc_size != 0u)
        {
            return generated_alloc_size;
        }
    }
    asset_size = ndsRelocExternTreeAllocSize(asset_id, seen, &seen_count);
    if (asset_size != 0)
    {
        return asset_size;
    }

    /* P2-3f48 witness. The fallback below is a HAZARD for any caller that
     * allocates what this returns and then hands the buffer to
     * lbRelocGetExternHeapFile, because that loader computes its own size and
     * writes the whole extern tree regardless -- 82,976 bytes into a 68-byte
     * allocation, in the case that motivated this. Publish which step gave up
     * so the shortfall is attributable rather than silently absorbed. */
    if (gNdsRelocFileSizeFallbackCount == 0u)
    {
        gNdsRelocFileSizeFallbackToken = token;
        gNdsRelocFileSizeFallbackAsset = asset_id;
        gNdsRelocFileSizeFallbackStatusHit =
            (ndsRelocFindStatusNode(sNdsRelocStatusBuffer,
                                    sNdsRelocStatusBufferCount,
                                    asset_id) != NULL) ? 1u : 0u;
        gNdsRelocFileSizeFallbackStatusCount =
            (u32)sNdsRelocStatusBufferCount;
    }
    gNdsRelocFileSizeFallbackCount++;
    return sizeof(Sprite);
}

/* R02/R03 witness. Non-zero Unresolved means the demo token never mapped;
 * non-zero LoadFail means it mapped and the payload did not survive loading or
 * normalization. Exactly one of them answers whether packing the Results
 * animations was enough. */
volatile u32 gNdsRelocExternHeapUnresolvedCount __attribute__((used));
volatile u32 gNdsRelocExternHeapLastUnresolvedToken __attribute__((used));
volatile u32 gNdsRelocExternHeapNoHeapCount __attribute__((used));
volatile u32 gNdsRelocExternHeapLoadFailCount __attribute__((used));
volatile u32 gNdsRelocExternHeapLastLoadFailAsset __attribute__((used));
volatile u32 gNdsRelocExternHeapOkCount __attribute__((used));

void *lbRelocGetExternHeapFile(const void *file_id, void *heap)
{
    u32 token = ndsRelocFileID(file_id);
    u32 asset_id = ndsRelocAssetIDForToken(token);
    NDSRelocLoadedFile *loaded;
    uintptr_t heap_ptr = (uintptr_t)heap;

    /* R02/R03. THIS FUNCTION FAILS SILENTLY BY CONSTRUCTION. Every arm below
     * returns the caller's heap untouched, and ftMainSetStatus (ftmain.c:4623)
     * assigns `fp->figatree = fp->figatree_heap` unconditionally and discards
     * the return value -- so a fighter binds whatever the heap already held,
     * with no decline, no reject and nothing to notice. That is why 142 missing
     * Results animations looked like a pose bug for months.
     *
     * The two existing counters, gNdsRelocForceFighterAnimResolveCount and
     * ...FallbackCount, both live INSIDE the ndsRelocIsFighterAnimID arm, so a
     * token that fails HERE increments neither. These three split the failure
     * exactly: Unresolved means ndsRelocAssetIDForToken did not map the token
     * at all (the token table or its gating is wrong); LoadFail means it mapped
     * and the payload did not load or normalize (suspect the AObj16 header
     * contract, which submotion payloads had never been put through). */
    if (asset_id == NDS_RELOC_ASSET_INVALID)
    {
        NDS_DIAG(gNdsRelocExternHeapUnresolvedCount++);
        gNdsRelocExternHeapLastUnresolvedToken = token;
        return heap;
    }
    if (heap == NULL)
    {
        NDS_DIAG(gNdsRelocExternHeapNoHeapCount++);
        return heap;
    }
    ndsRelocPrepareSceneCache();

    loaded = ndsRelocLoadExternTreeAsset(asset_id, &heap_ptr);
    if (loaded == NULL)
    {
        NDS_DIAG(gNdsRelocExternHeapLoadFailCount++);
        gNdsRelocExternHeapLastLoadFailAsset = asset_id;
        return heap;
    }
    NDS_DIAG(gNdsRelocExternHeapOkCount++);
    if (asset_id == NDS_RELOC_ASSET_N64_LOGO)
    {
        ndsRelocNormalizeN64LogoSprite(loaded);
        gNdsStartupLogoRelocResult = NDS_STARTUP_LOGO_RELOC_PASS;
        gNdsStartupLogoRelocSize = loaded->data_size;
    }
    ndsFighterManagerRecordExternToken(token, loaded->data);
    return loaded->data;
}

/* What ndsRelocLoadExternTreeAssetID would take for `asset_id`: 0 when it is
 * already resident. The VS select sizes a content's preview block by it (S15).
 * Not for a fighter Main file: its size walk can load a guard-pose package. */
u32 ndsRelocExternTreeAssetBytes(u32 asset_id)
{
    u32 seen[NDS_RELOC_EXTERN_FILE_ID_CAPACITY];
    u32 seen_count = 0u;

    if ((asset_id == NDS_RELOC_ASSET_INVALID) ||
        (ndsRelocFindLoadedFileByAsset(asset_id) != NULL))
    {
        return 0u;
    }
    return (u32)ndsRelocExternTreeAllocSize(asset_id, seen, &seen_count);
}

s32 ndsRelocLoadExternTreeAssetID(u32 asset_id)
{
    u32 seen[NDS_RELOC_EXTERN_FILE_ID_CAPACITY];
    u32 seen_count = 0u;
    size_t bytes;
    void *heap;
    uintptr_t heap_ptr;

    if (asset_id == NDS_RELOC_ASSET_INVALID)
    {
        return FALSE;
    }
    ndsRelocPrepareSceneCache();
    if (ndsRelocFindLoadedFileByAsset(asset_id) != NULL)
    {
        return TRUE;
    }
    bytes = ndsRelocExternTreeAllocSize(asset_id, seen, &seen_count);
    if (bytes == 0u)
    {
        return (ndsRelocFindStatusNode(sNdsRelocStatusBuffer,
                                      sNdsRelocStatusBufferCount,
                                      asset_id) != NULL) ? TRUE : FALSE;
    }
    heap = syTaskmanMalloc(bytes, 0x10u);
    if (heap == NULL)
    {
        return FALSE;
    }
    heap_ptr = (uintptr_t)heap;
    return (ndsRelocLoadExternTreeAsset(asset_id, &heap_ptr) != NULL) ? TRUE :
                                                                       FALSE;
}

static void ndsRelocCopyLoadedFileToHeap(const NDSRelocLoadedFile *loaded,
                                         void *heap)
{
    uintptr_t src_base;
    uintptr_t dst_base;
    u32 words;
    u32 i;

    if ((loaded == NULL) || (loaded->data == NULL) || (heap == NULL))
    {
        return;
    }

    memcpy(heap, loaded->data, loaded->data_size);

    src_base = (uintptr_t)loaded->data;
    dst_base = (uintptr_t)heap;
    words = loaded->data_size / sizeof(u32);
    for (i = 0; i < words; i++)
    {
        void *slot = (u8 *)heap + (i * sizeof(u32));
        uintptr_t value = (uintptr_t)ndsRelocReadNative32(slot);

        if ((value >= src_base) &&
            ((value - src_base) < (uintptr_t)loaded->data_size))
        {
            ndsRelocWriteNativePointer(slot,
                                       (void *)(dst_base +
                                                (value - src_base)));
        }
    }
}
