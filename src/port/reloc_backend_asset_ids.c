/* Relocation asset loading, file IDs to assets: the fighter manager's extern bits,
 * the per-fighter animation file IDs, the token-to-asset chain, loaded ranges and
 * the status buffers. A slice of reloc_backend.c. */

static u32 ndsRelocFileID(const void *file_id)
{
    return (u32)(uintptr_t)file_id;
}

extern size_t ndsBattleShipCSSSelectedFigatreeSize(const void *file_id);
extern void *ndsBattleShipLoadCSSSelectedFigatree(const void *file_id,
                                                  void *heap);
extern sb32 ndsBattleShipIsCSSSelectedFigatreeJoint(const void *ptr);

static u32 ndsFighterManagerExternBit(u32 token)
{
    if (token == ndsRelocFileID(&llFTManagerCommonFileID))
    {
        return NDS_FIGHTER_MANAGER_EXTERN_COMMON;
    }
    if (token == ndsRelocFileID(&llFTCommonMovesetFileID))
    {
        return NDS_FIGHTER_MANAGER_EXTERN_COMMON_MOVESET;
    }
    if (token == ndsRelocFileID(&llMarioMainFileID))
    {
        return NDS_FIGHTER_MANAGER_EXTERN_MARIO_MAIN;
    }
    if (token == ndsRelocFileID(&llFoxMainFileID))
    {
        return NDS_FIGHTER_MANAGER_EXTERN_FOX_MAIN;
    }
    return 0u;
}

static u32 ndsFighterManagerStatusBit(u32 token)
{
    if (token == ndsRelocFileID(&llMarioMainMotionFileID))
    {
        return NDS_FIGHTER_MANAGER_STATUS_MARIO_MAINMOTION;
    }
    if (token == ndsRelocFileID(&llMarioModelFileID))
    {
        return NDS_FIGHTER_MANAGER_STATUS_MARIO_MODEL;
    }
    if (token == ndsRelocFileID(&llMarioShieldPoseFileID))
    {
        return NDS_FIGHTER_MANAGER_STATUS_MARIO_SHIELD;
    }
    if (token == ndsRelocFileID(&llMarioSpecial1FileID))
    {
        return NDS_FIGHTER_MANAGER_STATUS_MARIO_SPECIAL1;
    }
    if (token == ndsRelocFileID(&llMarioSpecial2FileID))
    {
        return NDS_FIGHTER_MANAGER_STATUS_MARIO_SPECIAL2;
    }
    if (token == ndsRelocFileID(&llMarioSpecial3FileID))
    {
        return NDS_FIGHTER_MANAGER_STATUS_MARIO_SPECIAL3;
    }
    if (token == ndsRelocFileID(&llFoxMainMotionFileID))
    {
        return NDS_FIGHTER_MANAGER_STATUS_FOX_MAINMOTION;
    }
    if (token == ndsRelocFileID(&llFoxModelFileID))
    {
        return NDS_FIGHTER_MANAGER_STATUS_FOX_MODEL;
    }
    if (token == ndsRelocFileID(&llFoxShieldPoseFileID))
    {
        return NDS_FIGHTER_MANAGER_STATUS_FOX_SHIELD;
    }
    if (token == ndsRelocFileID(&llFoxSpecial1FileID))
    {
        return NDS_FIGHTER_MANAGER_STATUS_FOX_SPECIAL1;
    }
    if (token == ndsRelocFileID(&llFoxSpecial2FileID))
    {
        return NDS_FIGHTER_MANAGER_STATUS_FOX_SPECIAL2;
    }
    if (token == ndsRelocFileID(&llFoxSpecial3FileID))
    {
        return NDS_FIGHTER_MANAGER_STATUS_FOX_SPECIAL3;
    }
    if (token == ndsRelocFileID(&llFoxSpecial4FileID))
    {
        return NDS_FIGHTER_MANAGER_STATUS_FOX_SPECIAL4;
    }
    return 0u;
}

NDS_COLD static void ndsFighterManagerRefreshProof(void)
{
    u32 mask = 0u;

    mask |= 1u << 0;
    if ((gNdsFighterManagerExternMask &
         NDS_FIGHTER_MANAGER_EXTERN_REQUIRED_MASK) ==
        NDS_FIGHTER_MANAGER_EXTERN_REQUIRED_MASK)
    {
        mask |= 1u << 1;
    }
    if ((gNdsFighterManagerStatusBufferMask &
         NDS_FIGHTER_MANAGER_STATUS_REQUIRED_MASK) ==
        NDS_FIGHTER_MANAGER_STATUS_REQUIRED_MASK)
    {
        mask |= 1u << 2;
    }
    gNdsFighterManagerFigatreeHeapSize = (u32)gFTManagerFigatreeHeapSize;
    if (gNdsFighterManagerFigatreeHeapSize != 0u)
    {
        mask |= 1u << 3;
    }
    if ((gNdsFighterManagerFighterMask & 0x3u) == 0x3u)
    {
        mask |= 1u << 4;
    }
    if ((gNdsFighterManagerDataMask & 0x3u) == 0x3u)
    {
        mask |= 1u << 5;
    }
    if (((gNdsFighterManagerWaitMask | gNdsFighterManagerEntryMask) & 0x3u) ==
        0x3u)
    {
        mask |= 1u << 6;
    }
    if (gNdsFighterManagerStatusBufferHitCount >= 13u)
    {
        mask |= 1u << 7;
    }

    gNdsFighterManagerMask = mask;
    if ((mask & 0xffu) == 0xffu)
    {
        gNdsFighterManagerResult = NDS_FIGHTER_MANAGER_PASS;
    }
}

static void ndsFighterManagerRecordExternToken(u32 token, const void *file)
{
    u32 bit;

    if (file == NULL)
    {
        return;
    }
    bit = ndsFighterManagerExternBit(token);
    if (bit != 0u)
    {
        gNdsFighterManagerExternMask |= bit;
        ndsFighterManagerRefreshProof();
    }
}

static void ndsFighterManagerRecordStatusToken(u32 token, const void *file)
{
    u32 bit;

    if (file == NULL)
    {
        return;
    }
    bit = ndsFighterManagerStatusBit(token);
    if (bit != 0u)
    {
        gNdsFighterManagerStatusBufferMask |= bit;
        gNdsFighterManagerStatusBufferHitCount++;
        ndsFighterManagerRefreshProof();
    }
}

static u32 ndsFloatBits(f32 value)
{
    union {
        f32 f;
        u32 u;
    } bits;

    bits.f = value;
    return bits.u;
}


static u32 ndsPupupuStageAssetBit(u32 asset_id)
{
    if (asset_id == NDS_RELOC_ASSET_GR_PUPUPU_MAP) return 1u << 0;
    if (asset_id == NDS_RELOC_ASSET_STAGE_DREAM_LAND) return 1u << 1;
    if (asset_id == NDS_RELOC_ASSET_EXTERN_DATA_BANK_103) return 1u << 2;
    if (asset_id == NDS_RELOC_ASSET_EXTERN_DATA_BANK_104) return 1u << 3;
    if (asset_id == NDS_RELOC_ASSET_MISC_DATA_BANK_152) return 1u << 4;
    return 0;
}

/* P2-4. Dream Land has had per-asset fixup counters since P1 and they are
 * what proves its reloc tree resolved; the two opt-in stages have none,
 * which is exactly why neither has ever been accepted on a ROM. One helper
 * covers both rather than one per stage, so the six still to come add a line
 * each. The bit numbering is stable: a mask read off a running build names
 * which files were fixed up. */
NDS_COLD static u32 ndsOptInStageAssetBit(u32 asset_id)
{
    if (asset_id == NDS_RELOC_ASSET_GR_YOSTER_MAP) return 1u << 0;
    if (asset_id == NDS_RELOC_ASSET_STAGE_YOSHI) return 1u << 1;
    if (asset_id == NDS_RELOC_ASSET_EXTERN_DATA_BANK_111) return 1u << 2;
    if (asset_id == NDS_RELOC_ASSET_EXTERN_DATA_BANK_110) return 1u << 26;
    if (asset_id == NDS_RELOC_ASSET_MISC_DATA_BANK_154) return 1u << 3;
    if (asset_id == NDS_RELOC_ASSET_GR_CASTLE_MAP) return 1u << 4;
    if (asset_id == NDS_RELOC_ASSET_EXTERN_DATA_BANK_106) return 1u << 5;
    if (asset_id == NDS_RELOC_ASSET_MISC_DATA_BANK_156) return 1u << 6;
    if (asset_id == NDS_RELOC_ASSET_GR_JUNGLE_MAP) return 1u << 7;
    if (asset_id == NDS_RELOC_ASSET_STAGE_JUNGLE) return 1u << 8;
    if (asset_id == NDS_RELOC_ASSET_EXTERN_DATA_BANK_108) return 1u << 9;
    if (asset_id == NDS_RELOC_ASSET_MISC_DATA_BANK_158) return 1u << 10;
    if (asset_id == NDS_RELOC_ASSET_GR_ZEBES_MAP) return 1u << 11;
    if (asset_id == NDS_RELOC_ASSET_STAGE_ZEBES) return 1u << 12;
    if (asset_id == NDS_RELOC_ASSET_EXTERN_DATA_BANK_105) return 1u << 13;
    if (asset_id == NDS_RELOC_ASSET_MISC_DATA_BANK_157) return 1u << 14;
    if (asset_id == NDS_RELOC_ASSET_GR_YAMABUKI_MAP) return 1u << 15;
    if (asset_id == NDS_RELOC_ASSET_STAGE_POKEMON) return 1u << 16;
    if (asset_id == NDS_RELOC_ASSET_EXTERN_DATA_BANK_112) return 1u << 17;
    if (asset_id == NDS_RELOC_ASSET_MISC_DATA_BANK_159) return 1u << 18;
    if (asset_id == NDS_RELOC_ASSET_MISC_DATA_BANK_160) return 1u << 19;
    if (asset_id == NDS_RELOC_ASSET_STAGE_HYRULE_WALLPAPER) return 1u << 20;
    if (asset_id == NDS_RELOC_ASSET_EXTERN_DATA_BANK_107) return 1u << 21;
    if (asset_id == NDS_RELOC_ASSET_MISC_DATA_BANK_155) return 1u << 22;
    if (asset_id == NDS_RELOC_ASSET_GR_SECTOR_MAP) return 1u << 23;
    if (asset_id == NDS_RELOC_ASSET_STAGE_SECTOR) return 1u << 24;
    if (asset_id == NDS_RELOC_ASSET_MISC_DATA_BANK_153) return 1u << 25;
    return 0;
}

__attribute__((used)) volatile u32 gNdsStageOptInExternalFixupCount;
__attribute__((used)) volatile u32 gNdsStageOptInExternalFixupFailCount;
__attribute__((used)) volatile u32 gNdsStageOptInAssetMask;
__attribute__((used)) volatile u32 gNdsStageOptInDependencyMask;

static u32 ndsFighterMarioFoxAssetBit(u32 asset_id)
{
    if (asset_id == NDS_RELOC_ASSET_FT_MANAGER_COMMON) return 1u << 0;
    if (asset_id == NDS_RELOC_ASSET_MARIO_MAIN_MOTION) return 1u << 1;
    if (asset_id == NDS_RELOC_ASSET_MARIO_MAIN) return 1u << 2;
    if (asset_id == NDS_RELOC_ASSET_MARIO_SPECIAL1) return 1u << 3;
    if (asset_id == NDS_RELOC_ASSET_MARIO_MODEL) return 1u << 4;
    if (asset_id == NDS_RELOC_ASSET_MARIO_SPECIAL3) return 1u << 5;
    if (asset_id == NDS_RELOC_ASSET_MARIO_SHIELD_POSE) return 1u << 6;
    if (asset_id == NDS_RELOC_ASSET_MARIO_SPECIAL2) return 1u << 7;
    if (asset_id == NDS_RELOC_ASSET_FOX_SPECIAL3) return 1u << 8;
    if (asset_id == NDS_RELOC_ASSET_FOX_MAIN_MOTION) return 1u << 9;
    if (asset_id == NDS_RELOC_ASSET_FOX_MAIN) return 1u << 10;
    if (asset_id == NDS_RELOC_ASSET_FOX_SPECIAL1) return 1u << 11;
    if (asset_id == NDS_RELOC_ASSET_FOX_MODEL) return 1u << 12;
    if (asset_id == NDS_RELOC_ASSET_FOX_SHIELD_POSE) return 1u << 13;
    if (asset_id == NDS_RELOC_ASSET_FOX_SPECIAL4) return 1u << 14;
    if (asset_id == NDS_RELOC_ASSET_FOX_SPECIAL2) return 1u << 15;
    return 0;
}

static u32 ndsFighterMarioFoxDependencyBit(u32 asset_id)
{
    if (ndsFighterMarioFoxAssetBit(asset_id) != 0u)
    {
        return ndsFighterMarioFoxAssetBit(asset_id);
    }
    if (asset_id == NDS_RELOC_ASSET_MISC_DATA_201) return 1u << 16;
    if (asset_id == NDS_RELOC_ASSET_MISC_DATA_299) return 1u << 17;
    if (asset_id == NDS_RELOC_ASSET_MISC_DATA_315) return 1u << 18;
    if (asset_id == NDS_RELOC_ASSET_EXTERN_DATA_BANK_109) return 1u << 19;
    return 0;
}

NDS_COLD static void ndsRelocRecordExternalFixupSuccess(u32 source_asset_id,
                                               u32 dep_asset_id)
{
    if (ndsPupupuStageAssetBit(source_asset_id) != 0u)
    {
        NDS_DIAG(gNdsStagePupupuExternalFixupCount++);
        gNdsStagePupupuRelocDependencyMask |=
            ndsPupupuStageAssetBit(dep_asset_id);
    }
    if (ndsFighterMarioFoxAssetBit(source_asset_id) != 0u)
    {
        NDS_DIAG(gNdsFighterMarioFoxExternalFixupCount++);
        gNdsFighterMarioFoxRelocDependencyMask |=
            ndsFighterMarioFoxDependencyBit(dep_asset_id);
    }
    if (ndsOptInStageAssetBit(source_asset_id) != 0u)
    {
        gNdsStageOptInExternalFixupCount++;
        gNdsStageOptInAssetMask |= ndsOptInStageAssetBit(source_asset_id);
        gNdsStageOptInDependencyMask |= ndsOptInStageAssetBit(dep_asset_id);
    }
}

/* Every external fixup failure, not only the stage's and Mario/Fox's.
 * The two subsystem counters below were the ONLY record, so a fighter
 * outside them could fail every cross-file pointer in silence and present
 * as a data abort three frames away (P2-3f50, Purin, 2026-09-02). */
__attribute__((used)) volatile u32 gNdsRelocExternalFixupFailCount;
__attribute__((used)) volatile u32 gNdsRelocExternalFixupFailFirstAsset;
__attribute__((used)) volatile u32 gNdsRelocExternalFixupFailFirstDep;
__attribute__((used)) volatile u32 gNdsRelocExternalFixupFailLastAsset;
__attribute__((used)) volatile u32 gNdsRelocExternalFixupFailIndex;
__attribute__((used)) volatile u32 gNdsRelocExternalFixupFailSlot;
__attribute__((used)) volatile u32 gNdsRelocExternalFixupFailDeclared;
__attribute__((used)) volatile u32 gNdsRelocExternalFixupFailDataSize;
/* P2-3f48. lbRelocGetFileSize's sizeof(Sprite) fallback, named. */
__attribute__((used)) volatile u32 gNdsRelocFileSizeFallbackCount;
__attribute__((used)) volatile u32 gNdsRelocFileSizeFallbackToken;
__attribute__((used)) volatile u32 gNdsRelocFileSizeFallbackAsset;
__attribute__((used)) volatile u32 gNdsRelocFileSizeFallbackStatusHit;
__attribute__((used)) volatile u32 gNdsRelocFileSizeFallbackStatusCount;
__attribute__((used)) volatile u32 gNdsRelocExternalFixupFailFirstLR;
__attribute__((used)) volatile u32 gNdsRelocExternalFixupFailLastLR;
/* P2-4. A dependency whose token no row resolves, named at the point the
 * resolver gives up rather than inferred from the parent afterwards. */
__attribute__((used)) volatile u32 gNdsRelocUnresolvedDepCount;
__attribute__((used)) volatile u32 gNdsRelocUnresolvedDepToken;
__attribute__((used)) volatile u32 gNdsRelocUnresolvedDepParent;

static __attribute__((noinline)) void ndsRelocRecordExternalFixupFail(
    u32 source_asset_id)
{
    u32 caller_lr = (u32)(uintptr_t)__builtin_return_address(0);

    if (gNdsRelocExternalFixupFailCount == 0u)
    {
        gNdsRelocExternalFixupFailFirstAsset = source_asset_id;
        gNdsRelocExternalFixupFailFirstLR = caller_lr;
    }
    gNdsRelocExternalFixupFailLastLR = caller_lr;
    /* THE GDB STUB READS MAIN RAM BEHIND THE ARM9 DATA CACHE. A witness the
     * CPU wrote moments ago is still a dirty line, so the probe reads its old
     * value -- which for a fresh counter is zero, i.e. "this never happened".
     * That cost a whole round here: three new site counters all read 0 while
     * the failure count read 1, and the zeroes were taken as evidence that the
     * site was none of them, when in fact the caller address published beside
     * them named one of those very sites. `probe-arena-overflow.ps1` documents
     * the same trap for the malloc-halt globals. This path runs once per
     * failed fixup and never in a healthy boot, so flushing here is free. */
    DC_FlushAll();
    gNdsRelocExternalFixupFailCount++;
    gNdsRelocExternalFixupFailLastAsset = source_asset_id;
    if (ndsPupupuStageAssetBit(source_asset_id) != 0u)
    {
        gNdsStagePupupuExternalFixupFailCount++;
    }
    if (ndsFighterMarioFoxAssetBit(source_asset_id) != 0u)
    {
        gNdsFighterMarioFoxExternalFixupFailCount++;
    }
    if (ndsOptInStageAssetBit(source_asset_id) != 0u)
    {
        gNdsStageOptInExternalFixupFailCount++;
    }
}

static u32 ndsRelocReadBe32(const void *addr)
{
    const u8 *bytes = addr;

    return ((u32)bytes[0] << 24) |
           ((u32)bytes[1] << 16) |
           ((u32)bytes[2] << 8) |
           (u32)bytes[3];
}

/* Task 85. memcpy(&value, addr, 4) is the portable way to read a possibly
 * unaligned word, and on most targets the compiler turns it into one load. It
 * cannot here: the ARM946E-S has no unaligned-access support, so GCC must emit
 * a real call that handles the misaligned case byte by byte.
 *
 * Task 85 measured the result -- 51% of every memcpy call in the frame moves 2
 * or 4 bytes, and at ~70 ticks of call overhead each that is around 30,000
 * ticks/frame spent almost entirely on function-call machinery.
 *
 * The aligned test below is two instructions and the fallback keeps the exact
 * original behaviour, so this needs no proof about where these pointers come
 * from. That matters more than it looks: an unaligned LDR on this core does not
 * fault, it rotates, so a wrong alignment assumption would corrupt silently
 * rather than crash. */
static u32 ndsRelocReadNative32(const void *addr)
{
    u32 value;

    if ((((uintptr_t)addr) & 3u) == 0u)
    {
        return *(const u32 *)addr;
    }
    memcpy(&value, addr, sizeof(value));
    return value;
}

static void ndsRelocWriteNative32(void *addr, u32 value)
{
    if ((((uintptr_t)addr) & 3u) == 0u)
    {
        *(u32 *)addr = value;
        return;
    }
    memcpy(addr, &value, sizeof(value));
}

static void ndsRelocWriteNativePointer(void *addr, void *target)
{
    ndsRelocWriteNative32(addr, (u32)(uintptr_t)target);
}

static s32 ndsRelocIsMarioFoxAnimID(u32 asset_id)
{
    return (((asset_id >= NDS_RELOC_ASSET_MARIO_ANIM_WAIT) &&
             (asset_id <= NDS_RELOC_ASSET_MARIO_ANIM_FIRE_FLOWER_AIR)) ||
            ((asset_id >= NDS_RELOC_ASSET_FOX_ANIM_FIRST) &&
             (asset_id <= NDS_RELOC_ASSET_FOX_ANIM_LAST))) ?
        TRUE :
        FALSE;
}

/* VS Results demo poses (BUGS.md R02/R03/K06), and this is the only route that
 * can make them load.
 *
 * `mnVSResultsSetFighterStatus` gives every present fighter a demo status, and
 * `ftMainSetStatus` maps it through an identity table, so `status - 0x10000` IS
 * the `dFT<Kind>SubMotionDescs` row: Win1/Win2/Win3 are rows 1/2/3 and DemoLose
 * -- which No Contest gives EVERYONE -- is row 5. Those rows name Win/Claps
 * animations that live under `reloc_submotions/`, and nothing in any fighter's
 * motion or extern-table closure reaches that directory, so the build staged
 * none of them and no resolver answered their tokens.
 *
 * THE FAILURE WAS SILENT, which is why it survived as three separate bug rows.
 * ftmain.c does
 *
 *     lbRelocGetForceExternHeapFile(motion_desc->anim_file_id, fp->figatree_heap);
 *     fp->figatree = fp->figatree_heap;
 *
 * -- the assignment is unconditional and the return value is discarded. On an
 * unmappable token `lbRelocGetExternHeapFile` returns the heap untouched, so
 * the fighter bound whatever figatree the heap still held: his last battle
 * motion. No decline, no reject, no halt. Only row 0 resolved, which is exactly
 * why every fighter stood in his row-0 pose (Mario `Wait`, Kirby and nine
 * others `EggLay`).
 *
 * These rows are separate from `sNdsP2FighterAnimTokens` for two reasons, both
 * load-bearing. Mario and Fox are the always-compiled base pair and have no
 * `NDS_P2_*` flag, yet three of their demo files are needed (Mario's Claps
 * serves Luigi's DemoLose too); and their base-pair bridges are contiguous
 * index banks -- `asset_id = BASE + i` -- which cannot carry a sparse id in
 * the 357..498 submotion space. Keeping the demo rows in their own table also
 * keeps `sNdsP2FighterAnimTokens`'s measured numeric early-out bound at its
 * current 0x320 floor instead of dropping it to 0x165, so no range-missing
 * numeric id starts paying a ~1,300-row scan it does not pay today. */
typedef struct NDSRelocDemoAnimTokenRow
{
    const void *token;
    u16 asset_id;
} NDSRelocDemoAnimTokenRow;

static u16 sNdsRelocDemoAnimIdMin;
static u16 sNdsRelocDemoAnimIdMax;
static sb32 sNdsRelocDemoAnimBoundsReady;

static const NDSRelocDemoAnimTokenRow sNdsRelocDemoAnimTokens[] =
{
#define NDS_RELOC_DEMO_ANIM_TOKEN_ROW(symbol_, id_, path_) \
    { &symbol_, (u16)(id_) },
    NDS_MARIOFOX_DEMO_ANIM_ASSET_ROWS(NDS_RELOC_DEMO_ANIM_TOKEN_ROW)
#if NDS_P2_LUIGI
    NDS_P2_LUIGI_DEMO_ANIM_ASSET_ROWS(NDS_RELOC_DEMO_ANIM_TOKEN_ROW)
#endif
#if NDS_P2_DONKEY
    NDS_P2_DONKEY_DEMO_ANIM_ASSET_ROWS(NDS_RELOC_DEMO_ANIM_TOKEN_ROW)
#endif
#if NDS_P2_CAPTAIN
    NDS_P2_CAPTAIN_DEMO_ANIM_ASSET_ROWS(NDS_RELOC_DEMO_ANIM_TOKEN_ROW)
#endif
    NDS_P2_SAMUS_DEMO_ANIM_ASSET_ROWS(NDS_RELOC_DEMO_ANIM_TOKEN_ROW)
#if NDS_P2_LINK
    NDS_P2_LINK_DEMO_ANIM_ASSET_ROWS(NDS_RELOC_DEMO_ANIM_TOKEN_ROW)
#endif
    NDS_P2_PIKACHU_DEMO_ANIM_ASSET_ROWS(NDS_RELOC_DEMO_ANIM_TOKEN_ROW)
    NDS_P2_YOSHI_DEMO_ANIM_ASSET_ROWS(NDS_RELOC_DEMO_ANIM_TOKEN_ROW)
    NDS_P2_NESS_DEMO_ANIM_ASSET_ROWS(NDS_RELOC_DEMO_ANIM_TOKEN_ROW)
    NDS_P2_PURIN_DEMO_ANIM_ASSET_ROWS(NDS_RELOC_DEMO_ANIM_TOKEN_ROW)
    NDS_P2_KIRBY_DEMO_ANIM_ASSET_ROWS(NDS_RELOC_DEMO_ANIM_TOKEN_ROW)
    /* The 1P scenes' rows: the intro's IntroL/IntroR poses (rows 13/14) and
     * the figure's DollFall/DollRevival (rows 9/10). Unresolved, every intro
     * fighter replayed his DemoNull Wait from the figatree heap and every
     * team member stood on the formation origin. */
#if NDS_P2_1P_GAME
    NDS_1P_DEMO_ANIM_ASSET_ROWS(NDS_RELOC_DEMO_ANIM_TOKEN_ROW)
#endif
#undef NDS_RELOC_DEMO_ANIM_TOKEN_ROW
};

#define NDS_RELOC_DEMO_ANIM_TOKEN_COUNT \
    (sizeof(sNdsRelocDemoAnimTokens) / sizeof(sNdsRelocDemoAnimTokens[0]))

/* Answers both token shapes, like the P2 animation table: the `&ll...FileID`
 * symbol address BattleShip's call sites actually pass, and the numeric O2R id
 * an extern table carries. The symbol's VALUE is never read -- these `ll*`
 * symbols are linker-absolute in BattleShip and are plain stubbed globals here,
 * so the registry keys on `&sym` and a `0u` stub resolves exactly as well as a
 * pinned one. */
static u32 ndsRelocDemoAnimAssetIDForToken(u32 token)
{
    u32 i;

    if (token <= 0xffffu)
    {
        if (sNdsRelocDemoAnimBoundsReady == FALSE)
        {
            u32 j;

            sNdsRelocDemoAnimIdMin = 0xffffu;
            sNdsRelocDemoAnimIdMax = 0u;
            for (j = 0u; j < NDS_RELOC_DEMO_ANIM_TOKEN_COUNT; j++)
            {
                u16 id = sNdsRelocDemoAnimTokens[j].asset_id;

                if (id < sNdsRelocDemoAnimIdMin)
                {
                    sNdsRelocDemoAnimIdMin = id;
                }
                if (id > sNdsRelocDemoAnimIdMax)
                {
                    sNdsRelocDemoAnimIdMax = id;
                }
            }
            sNdsRelocDemoAnimBoundsReady = TRUE;
        }
        if ((token < (u32)sNdsRelocDemoAnimIdMin) ||
            (token > (u32)sNdsRelocDemoAnimIdMax))
        {
            return NDS_RELOC_ASSET_INVALID;
        }
    }
    for (i = 0u; i < NDS_RELOC_DEMO_ANIM_TOKEN_COUNT; i++)
    {
        if ((token == (u32)sNdsRelocDemoAnimTokens[i].asset_id) ||
            (token == ndsRelocFileID(sNdsRelocDemoAnimTokens[i].token)))
        {
            return (u32)sNdsRelocDemoAnimTokens[i].asset_id;
        }
    }
    return NDS_RELOC_ASSET_INVALID;
}

/* A demo pose file is a figatree like any other battle animation, so it must
 * answer the fighter-animation predicate too: that is what runs the AObj16
 * header normalizer and gives the file fighter scratch-heap lifetime. Without
 * it the file would load and then be parsed as the wrong kind. */
static s32 ndsRelocIsDemoAnimID(u32 asset_id)
{
    return (ndsRelocDemoAnimAssetIDForToken(asset_id) != NDS_RELOC_ASSET_INVALID)
        ? TRUE : FALSE;
}

/* P2-3 widens the generic fighter-animation ownership without widening P2-2's
 * Mario/Fox diagnostics.  In particular, K0 and the old 301-id R2-04 census
 * intentionally keep their original two-fighter universe; callers that care
 * about parser type, scratch-heap lifetime or relocation ownership use this
 * predicate instead. */
static u32 ndsRelocP2FighterAnimAssetIDForToken(u32 token);
static s32 ndsRelocIsFighterAnimID(u32 asset_id)
{
    if (ndsRelocIsMarioFoxAnimID(asset_id) != FALSE)
    {
        return TRUE;
    }
#if NDS_P2_LUIGI
    if ((asset_id >= NDS_P2_LUIGI_ANIM_FIRST) &&
        (asset_id <= NDS_P2_LUIGI_ANIM_LAST))
    {
        return TRUE;
    }
#endif
#if NDS_P2_DONKEY
    if ((asset_id >= NDS_P2_DONKEY_ANIM_FIRST) &&
        (asset_id <= NDS_P2_DONKEY_ANIM_LAST))
    {
        return TRUE;
    }
#endif
#if NDS_P2_CAPTAIN
    if ((asset_id >= NDS_P2_CAPTAIN_ANIM_FIRST) &&
        (asset_id <= NDS_P2_CAPTAIN_ANIM_LAST))
    {
        return TRUE;
    }
#endif
    if ((asset_id >= NDS_P2_SAMUS_ANIM_FIRST) &&
        (asset_id <= NDS_P2_SAMUS_ANIM_LAST))
    {
        return TRUE;
    }
#if NDS_P2_LINK
    if ((asset_id >= NDS_P2_LINK_ANIM_FIRST) &&
        (asset_id <= NDS_P2_LINK_ANIM_LAST))
    {
        return TRUE;
    }
#endif
    if ((asset_id >= NDS_P2_PIKACHU_ANIM_FIRST) &&
        (asset_id <= NDS_P2_PIKACHU_ANIM_LAST))
    {
        return TRUE;
    }
    if ((asset_id >= NDS_P2_YOSHI_ANIM_FIRST) &&
        (asset_id <= NDS_P2_YOSHI_ANIM_LAST))
    {
        return TRUE;
    }
    if ((asset_id >= NDS_P2_NESS_ANIM_FIRST) &&
        (asset_id <= NDS_P2_NESS_ANIM_LAST))
    {
        return TRUE;
    }
    if ((asset_id >= NDS_P2_PURIN_ANIM_FIRST) &&
        (asset_id <= NDS_P2_PURIN_ANIM_LAST))
    {
        return TRUE;
    }
    if ((asset_id >= NDS_P2_KIRBY_ANIM_FIRST) &&
        (asset_id <= NDS_P2_KIRBY_ANIM_LAST))
    {
        return TRUE;
    }
    /* Borrowed animation ids live outside the borrower's own FIRST..LAST
     * range (Purin's 77 Kirby files, Kirby's 2 Purin files), so no range arm
     * above can own them when the lender's flag is off. The generated token
     * table is the exact per-build reference set: an id the resolver would
     * answer is a fighter animation. Reached only on range miss, so every
     * admitted fighter's own files still return above at zero added cost. */
    if (ndsRelocP2FighterAnimAssetIDForToken(asset_id) != NDS_RELOC_ASSET_INVALID)
    {
        return TRUE;
    }
    if (ndsP4IsFighterAnim(asset_id) != FALSE)
    {
        return TRUE;
    }
    /* Last, not first (2026-10-02): the demo table is a linear scan with a
     * token resolve per row, and a battle clip's id usually falls inside its
     * id bounds, so every status change of a fighter other than Mario or Fox
     * paid the whole scan before the range arms above answered. The
     * predicate is an OR of pure tests; their order changes no answer. */
    if (ndsRelocIsDemoAnimID(asset_id) != FALSE)
    {
        return TRUE;
    }
    return FALSE;
}

/* BattleShip's Mario battle animation files are one contiguous reloc bank.
 * The imported motion descriptors retain the addresses of the original
 * llFTMarioAnim*FileID symbols, so bridge each symbol to its exact source file
 * number instead of leaving unproven motions with a null figatree. */
static const uintptr_t *const sNdsRelocMarioBattleAnimFileIDs[] =
{
    &llFTMarioAnimWaitFileID, /* 499 */
    &llFTMarioAnimWalk1FileID, /* 500 */
    &llFTMarioAnimWalk2FileID, /* 501 */
    &llFTMarioAnimWalk3FileID, /* 502 */
    &llFTMarioAnimWalkEndFileID, /* 503 */
    &llFTMarioAnimDashFileID, /* 504 */
    &llFTMarioAnimRunFileID, /* 505 */
    &llFTMarioAnimRunBrakeFileID, /* 506 */
    &llFTMarioAnimTurnFileID, /* 507 */
    &llFTMarioAnimTurnRunFileID, /* 508 */
    &llFTMarioAnimJumpFFileID, /* 509 */
    &llFTMarioAnimJumpBFileID, /* 510 */
    &llFTMarioAnimJumpAerialFFileID, /* 511 */
    &llFTMarioAnimJumpAerialBFileID, /* 512 */
    &llFTMarioAnimFallFileID, /* 513 */
    &llFTMarioAnimFallAerialFileID, /* 514 */
    &llFTMarioAnimCrouchFileID, /* 515 */
    &llFTMarioAnimCrouchIdleFileID, /* 516 */
    &llFTMarioAnimCrouchEndFileID, /* 517 */
    &llFTMarioAnimLandingAirXFileID, /* 518 */
    &llFTMarioAnimShieldDropFileID, /* 519 */
    &llFTMarioAnimTeeterFileID, /* 520 */
    &llFTMarioAnimTeeterstartFileID, /* 521 */
    &llFTMarioAnimSleepFileID, /* 522 */
    &llFTMarioAnimDamaged1FileID, /* 523 */
    &llFTMarioAnimDamaged2FileID, /* 524 */
    &llFTMarioAnimFalconDivePulledFileID, /* 525 */
    &llFTMarioAnimDamageX1FileID, /* 526 */
    &llFTMarioAnimDamageX2FileID, /* 527 */
    &llFTMarioAnimDamageX3FileID, /* 528 */
    &llFTMarioAnimDamaged3FileID, /* 529 */
    &llFTMarioAnimDamaged4FileID, /* 530 */
    &llFTMarioAnimDamaged5FileID, /* 531 */
    &llFTMarioAnimDamaged6FileID, /* 532 */
    &llFTMarioAnimDamageAirFileID, /* 533 */
    &llFTMarioAnimDamaged7FileID, /* 534 */
    &llFTMarioAnimDamageFlyX1FileID, /* 535 */
    &llFTMarioAnimDamageFlyX2FileID, /* 536 */
    &llFTMarioAnimDamage2FileID, /* 537 */
    &llFTMarioAnimShieldBreakFileID, /* 538 */
    &llFTMarioAnimDamageFlyTopFileID, /* 539 */
    &llFTMarioAnimDamagedFileID, /* 540 */
    &llFTMarioAnimFallSpecialFileID, /* 541 */
    &llFTMarioAnimCeilingBonkFileID, /* 542 */
    &llFTMarioAnimDownBounceDFileID, /* 543 */
    &llFTMarioAnimStunLandUFileID, /* 544 */
    &llFTMarioAnimDownStandDFileID, /* 545 */
    &llFTMarioAnimStunStartUFileID, /* 546 */
    &llFTMarioAnimTechFFileID, /* 547 */
    &llFTMarioAnimTechBFileID, /* 548 */
    &llFTMarioAnimDownForwardDFileID, /* 549 */
    &llFTMarioAnimDownForwardUFileID, /* 550 */
    &llFTMarioAnimDownBackDFileID, /* 551 */
    &llFTMarioAnimDownBackUFileID, /* 552 */
    &llFTMarioAnimDownAttackDFileID, /* 553 */
    &llFTMarioAnimDownAttackUFileID, /* 554 */
    &llFTMarioAnimTechFileID, /* 555 */
    &llFTMarioAnimClangRecoilFileID, /* 556 */
    &llFTMarioAnimShieldOnFileID, /* 557 */
    &llFTMarioAnimShieldOffFileID, /* 558 */
    &llFTMarioAnimRollFFileID, /* 559 */
    &llFTMarioAnimRollBFileID, /* 560 */
    &llFTMarioAnimCatchFileID, /* 561 */
    &llFTMarioAnimCatchPullFileID, /* 562 */
    &llFTMarioAnimThrowFFileID, /* 563 */
    &llFTMarioAnimThrowBFileID, /* 564 */
    &llFTMarioAnimEggLayPulledFileID, /* 565 */
    &llFTMarioAnimThrownDKPulledFileID, /* 566 */
    &llFTMarioAnimThrownMarioBrosFileID, /* 567 */
    &llFTMarioAnimThrownDKFileID, /* 568 */
    &llFTMarioAnimThrown2FileID, /* 569 */
    &llFTMarioAnimThrown1FileID, /* 570 */
    &llFTMarioAnimTauntFileID, /* 571 */
    &llFTMarioAnimCliffCatchFileID, /* 572 */
    &llFTMarioAnimCliffWaitFileID, /* 573 */
    &llFTMarioAnimCliffQuickFileID, /* 574 */
    &llFTMarioAnimCliffClimbQuick1FileID, /* 575 */
    &llFTMarioAnimCliffClimbQuick2FileID, /* 576 */
    &llFTMarioAnimCliffSlowFileID, /* 577 */
    &llFTMarioAnimCliffClimbSlow1FileID, /* 578 */
    &llFTMarioAnimCliffClimbSlow2FileID, /* 579 */
    &llFTMarioAnimCliffAttackQuick1FileID, /* 580 */
    &llFTMarioAnimCliffAttackQuick2FileID, /* 581 */
    &llFTMarioAnimCliffAttackSlow1FileID, /* 582 */
    &llFTMarioAnimCliffAttackSlow2FileID, /* 583 */
    &llFTMarioAnimCliffEscapeQuick1FileID, /* 584 */
    &llFTMarioAnimCliffEscapeQuick2FileID, /* 585 */
    &llFTMarioAnimCliffEscapeSlow1FileID, /* 586 */
    &llFTMarioAnimCliffEscapeSlow2FileID, /* 587 */
    &llFTMarioAnimLightItemPickupFileID, /* 588 */
    &llFTMarioAnimItemThrowSmashBFileID, /* 589 */
    &llFTMarioAnimItemThrowSmashUFileID, /* 590 */
    &llFTMarioAnimItemThrowSmashDFileID, /* 591 */
    &llFTMarioAnimItemThrowDashFileID, /* 592 */
    &llFTMarioAnimItemDropFileID, /* 593 */
    &llFTMarioAnimItemThrowAirSmashBFileID, /* 594 */
    &llFTMarioAnimItemThrowAirSmashUFileID, /* 595 */
    &llFTMarioAnimItemThrowAirSmashFFileID, /* 596 */
    &llFTMarioAnimHeavyItemPickupFileID, /* 597 */
    &llFTMarioAnimHeavyItemThrowSmashBFileID, /* 598 */
    &llFTMarioAnimStarRodNeutralFileID, /* 599 */
    &llFTMarioAnimStarRodTiltFileID, /* 600 */
    &llFTMarioAnimStarRodSmashFileID, /* 601 */
    &llFTMarioAnimStarRodDashFileID, /* 602 */
    &llFTMarioAnimHammerIdleFileID, /* 603 */
    &llFTMarioAnimHammerLandingFileID, /* 604 */
    &llFTMarioAnimFireFlowerShootFileID, /* 605 */
    &llFTMarioAnimJab1FileID, /* 606 */
    &llFTMarioAnimJab2FileID, /* 607 */
    &llFTMarioAnimJab3FileID, /* 608 */
    &llFTMarioAnimDashAttackFileID, /* 609 */
    &llFTMarioAnimFTiltHighFileID, /* 610 */
    &llFTMarioAnimFTiltFileID, /* 611 */
    &llFTMarioAnimFTiltLowFileID, /* 612 */
    &llFTMarioAnimUTiltFileID, /* 613 */
    &llFTMarioAnimDTiltFileID, /* 614 */
    &llFTMarioAnimFSmashHighFileID, /* 615 */
    &llFTMarioAnimFSmashMidHighFileID, /* 616 */
    &llFTMarioAnimFSmashFileID, /* 617 */
    &llFTMarioAnimFSmashMidLowFileID, /* 618 */
    &llFTMarioAnimFSmashLowFileID, /* 619 */
    &llFTMarioAnimUSmashFileID, /* 620 */
    &llFTMarioAnimDSmashFileID, /* 621 */
    &llFTMarioAnimAttackAirNFileID, /* 622 */
    &llFTMarioAnimAttackAirFFileID, /* 623 */
    &llFTMarioAnimAttackAirBFileID, /* 624 */
    &llFTMarioAnimAttackAirUFileID, /* 625 */
    &llFTMarioAnimAttackAirDFileID, /* 626 */
    &llFTMarioAnimLandingAirFFileID, /* 627 */
    &llFTMarioAnimLandingAirBFileID, /* 628 */
    &llFTMarioAnimLandingAirUFileID, /* 629 */
    &llFTMarioAnimEnterPipeFileID, /* 630 */
    &llFTMarioAnimExitPipeFileID, /* 631 */
    &llFTMarioAnimExitPipeWalkFileID, /* 632 */
    &llFTMarioAnimAppear1FileID, /* 633 */
    &llFTMarioAnimAppear2FileID, /* 634 */
    &llFTMarioAnimFireballGroundFileID, /* 635 */
    &llFTMarioAnimFireballAirFileID, /* 636 */
    &llFTMarioAnimSuperJumpPunchAirFileID, /* 637 */
    &llFTMarioAnimMarioTornadoGroundFileID, /* 638 */
    &llFTMarioAnimMarioTornadoAirFileID, /* 639 */
    &llFTMarioAnimDamageFileID, /* 640 */
    &llFTMarioAnimFireFlowerShootAirFileID, /* 641 */
};

static u32 ndsRelocMarioBattleAnimAssetIDForToken(u32 token)
{
    u32 i;

    for (i = 0u;
         i < (sizeof(sNdsRelocMarioBattleAnimFileIDs) /
              sizeof(sNdsRelocMarioBattleAnimFileIDs[0]));
         i++)
    {
        if (token == ndsRelocFileID(sNdsRelocMarioBattleAnimFileIDs[i]))
        {
            return NDS_RELOC_ASSET_MARIO_ANIM_WAIT + i;
        }
    }
    return NDS_RELOC_ASSET_INVALID;
}

/* BattleShip files 642..799 are Fox's complete contiguous animation bank. */
static const uintptr_t *const sNdsRelocFoxAnimFileIDs[] =
{
    &llFTFoxAnimEggLayFileID, /* 642 */
    &llFTFoxAnimWalk1FileID, /* 643 */
    &llFTFoxAnimWalk2FileID, /* 644 */
    &llFTFoxAnimWalk3FileID, /* 645 */
    &llFTFoxAnimWalkEndFileID, /* 646 */
    &llFTFoxAnimDashFileID, /* 647 */
    &llFTFoxAnimRunFileID, /* 648 */
    &llFTFoxAnimRunBrakeFileID, /* 649 */
    &llFTFoxAnimTurnFileID, /* 650 */
    &llFTFoxAnimTurnRunFileID, /* 651 */
    &llFTFoxAnimJumpFFileID, /* 652 */
    &llFTFoxAnimJumpBFileID, /* 653 */
    &llFTFoxAnimJumpAerialFFileID, /* 654 */
    &llFTFoxAnimJumpAerialBFileID, /* 655 */
    &llFTFoxAnimFallFileID, /* 656 */
    &llFTFoxAnimFallAerialFileID, /* 657 */
    &llFTFoxAnimCrouchFileID, /* 658 */
    &llFTFoxAnimCrouchIdleFileID, /* 659 */
    &llFTFoxAnimCrouchEndFileID, /* 660 */
    &llFTFoxAnimLandingAirXFileID, /* 661 */
    &llFTFoxAnimShieldDropFileID, /* 662 */
    &llFTFoxAnimTeeterFileID, /* 663 */
    &llFTFoxAnimTeeterstartFileID, /* 664 */
    &llFTFoxAnimSleepFileID, /* 665 */
    &llFTFoxAnimDamaged1FileID, /* 666 */
    &llFTFoxAnimDamaged2FileID, /* 667 */
    &llFTFoxAnimFalconDivePulledFileID, /* 668 */
    &llFTFoxAnimDamageX1FileID, /* 669 */
    &llFTFoxAnimDamageX2FileID, /* 670 */
    &llFTFoxAnimDamageX3FileID, /* 671 */
    &llFTFoxAnimDamaged3FileID, /* 672 */
    &llFTFoxAnimDamaged4FileID, /* 673 */
    &llFTFoxAnimDamaged5FileID, /* 674 */
    &llFTFoxAnimDamaged6FileID, /* 675 */
    &llFTFoxAnimDamageAirFileID, /* 676 */
    &llFTFoxAnimDamaged7FileID, /* 677 */
    &llFTFoxAnimDamageFlyX1FileID, /* 678 */
    &llFTFoxAnimDamageFlyX2FileID, /* 679 */
    &llFTFoxAnimDamage2FileID, /* 680 */
    &llFTFoxAnimShieldBreakFileID, /* 681 */
    &llFTFoxAnimDamageFlyTopFileID, /* 682 */
    &llFTFoxAnimDamagedFileID, /* 683 */
    &llFTFoxAnimFallSpecialFileID, /* 684 */
    &llFTFoxAnimCeilingBonkFileID, /* 685 */
    &llFTFoxAnimStunLandDFileID, /* 686 */
    &llFTFoxAnimStunLandUFileID, /* 687 */
    &llFTFoxAnimStunStartDFileID, /* 688 */
    &llFTFoxAnimStunStartUFileID, /* 689 */
    &llFTFoxAnimTechFFileID, /* 690 */
    &llFTFoxAnimTechBFileID, /* 691 */
    &llFTFoxAnimDownForwardDFileID, /* 692 */
    &llFTFoxAnimDownForwardUFileID, /* 693 */
    &llFTFoxAnimDownBackDFileID, /* 694 */
    &llFTFoxAnimDownBackUFileID, /* 695 */
    &llFTFoxAnimDownAttackDFileID, /* 696 */
    &llFTFoxAnimDownAttackUFileID, /* 697 */
    &llFTFoxAnimTechFileID, /* 698 */
    &llFTFoxAnimClangRecoilFileID, /* 699 */
    &llFTFoxAnimShieldOnFileID, /* 700 */
    &llFTFoxAnimShieldOffFileID, /* 701 */
    &llFTFoxAnimRollFFileID, /* 702 */
    &llFTFoxAnimRollBFileID, /* 703 */
    &llFTFoxAnimCatchFileID, /* 704 */
    &llFTFoxAnimCatchPullFileID, /* 705 */
    &llFTFoxAnimThrowFFileID, /* 706 */
    &llFTFoxAnimThrowBFileID, /* 707 */
    &llFTFoxAnimEggLayPulledFileID, /* 708 */
    &llFTFoxAnimThrownDKPulledFileID, /* 709 */
    &llFTFoxAnimThrownMarioBrosFileID, /* 710 */
    &llFTFoxAnimThrownDKFileID, /* 711 */
    &llFTFoxAnimThrown2FileID, /* 712 */
    &llFTFoxAnimThrown1FileID, /* 713 */
    &llFTFoxAnimThrown3FileID, /* 714 */
    &llFTFoxAnimThrownFoxBFileID, /* 715 */
    &llFTFoxAnimTauntFileID, /* 716 */
    &llFTFoxAnimCliffCatchFileID, /* 717 */
    &llFTFoxAnimCliffWaitFileID, /* 718 */
    &llFTFoxAnimCliffQuickFileID, /* 719 */
    &llFTFoxAnimCliffClimbQuick1FileID, /* 720 */
    &llFTFoxAnimCliffClimbQuick2FileID, /* 721 */
    &llFTFoxAnimCliffSlowFileID, /* 722 */
    &llFTFoxAnimCliffClimbSlow1FileID, /* 723 */
    &llFTFoxAnimCliffClimbSlow2FileID, /* 724 */
    &llFTFoxAnimCliffAttackQuick1FileID, /* 725 */
    &llFTFoxAnimCliffAttackQuick2FileID, /* 726 */
    &llFTFoxAnimCliffAttackSlow1FileID, /* 727 */
    &llFTFoxAnimCliffAttackSlow2FileID, /* 728 */
    &llFTFoxAnimCliffEscapeQuick1FileID, /* 729 */
    &llFTFoxAnimCliffEscapeQuick2FileID, /* 730 */
    &llFTFoxAnimCliffEscapeSlow1FileID, /* 731 */
    &llFTFoxAnimCliffEscapeSlow2FileID, /* 732 */
    &llFTFoxAnimLightItemPickupFileID, /* 733 */
    &llFTFoxAnimItemThrowSmashBFileID, /* 734 */
    &llFTFoxAnimItemThrowSmashUFileID, /* 735 */
    &llFTFoxAnimItemThrowSmashDFileID, /* 736 */
    &llFTFoxAnimItemThrowDashFileID, /* 737 */
    &llFTFoxAnimItemDropFileID, /* 738 */
    &llFTFoxAnimItemThrowAirSmashBFileID, /* 739 */
    &llFTFoxAnimItemThrowAirSmashUFileID, /* 740 */
    &llFTFoxAnimItemThrowAirSmashFFileID, /* 741 */
    &llFTFoxAnimHeavyItemPickupFileID, /* 742 */
    &llFTFoxAnimHeavyItemThrowSmashBFileID, /* 743 */
    &llFTFoxAnimStarRodNeutralFileID, /* 744 */
    &llFTFoxAnimStarRodTiltFileID, /* 745 */
    &llFTFoxAnimStarRodSmashFileID, /* 746 */
    &llFTFoxAnimStarRodDashFileID, /* 747 */
    &llFTFoxAnimHammerIdleFileID, /* 748 */
    &llFTFoxAnimHammerLandingFileID, /* 749 */
    &llFTFoxAnimFireFlowerShootFileID, /* 750 */
    &llFTFoxAnimJab1FileID, /* 751 */
    &llFTFoxAnimJab2FileID, /* 752 */
    &llFTFoxAnimJabLoopStartFileID, /* 753 */
    &llFTFoxAnimJabLoopFileID, /* 754 */
    &llFTFoxAnimJabLoopEndFileID, /* 755 */
    &llFTFoxAnimDashAttackFileID, /* 756 */
    &llFTFoxAnimFTiltHighFileID, /* 757 */
    &llFTFoxAnimFTiltMidHighFileID, /* 758 */
    &llFTFoxAnimFTiltFileID, /* 759 */
    &llFTFoxAnimFTiltMidLowFileID, /* 760 */
    &llFTFoxAnimFTiltLowFileID, /* 761 */
    &llFTFoxAnimUTiltFileID, /* 762 */
    &llFTFoxAnimDTiltFileID, /* 763 */
    &llFTFoxAnimFSmashFileID, /* 764 */
    &llFTFoxAnimUSmashFileID, /* 765 */
    &llFTFoxAnimDSmashFileID, /* 766 */
    &llFTFoxAnimAttackAirNFileID, /* 767 */
    &llFTFoxAnimAttackAirFFileID, /* 768 */
    &llFTFoxAnimAttackAirBFileID, /* 769 */
    &llFTFoxAnimAttackAirUFileID, /* 770 */
    &llFTFoxAnimAttackAirDFileID, /* 771 */
    &llFTFoxAnimLandingAirFFileID, /* 772 */
    &llFTFoxAnimLandingAirBFileID, /* 773 */
    &llFTFoxAnimEnterPipeFileID, /* 774 */
    &llFTFoxAnimExitPipeFileID, /* 775 */
    &llFTFoxAnimExitPipeWalkFileID, /* 776 */
    &llFTFoxAnimAppearFileID, /* 777 */
    &llFTFoxAnimArwingFileID, /* 778 */
    &llFTFoxAnimLaserFileID, /* 779 */
    &llFTFoxAnimLaserAerialFileID, /* 780 */
    &llFTFoxAnimFireFoxStartGroundFileID, /* 781 */
    &llFTFoxAnimReadyingFireFoxGroundFileID, /* 782 */
    &llFTFoxAnimFireFoxGroundFileID, /* 783 */
    &llFTFoxAnimFireFoxEndGroundFileID, /* 784 */
    &llFTFoxAnimFireFoxStartAerialFileID, /* 785 */
    &llFTFoxAnimReadyingFireFoxAirFileID, /* 786 */
    &llFTFoxAnimFireFoxAirFileID, /* 787 */
    &llFTFoxAnimFireFoxEndAirFileID, /* 788 */
    &llFTFoxAnimLandingWhileFireFoxAirFileID, /* 789 */
    &llFTFoxAnimShineStartFileID, /* 790 */
    &llFTFoxAnimSwitchDirectionShineFileID, /* 791 */
    &llFTFoxAnimReflectingFileID, /* 792 */
    &llFTFoxAnimShineFileID, /* 793 */
    &llFTFoxAnimShireStartAirFileID, /* 794 */
    &llFTFoxAnimSwitchDirectionShineAirFileID, /* 795 */
    &llFTFoxAnimUnknownFileID, /* 796 */
    &llFTFoxAnimShineAirEndFileID, /* 797 */
    &llFTFoxAnimDamageFileID, /* 798 */
    &llFTFoxAnimFireFlowerShootAirFileID, /* 799 */
};

static u32 ndsRelocFoxAnimAssetIDForToken(u32 token)
{
    u32 i;

    for (i = 0u;
         i < (sizeof(sNdsRelocFoxAnimFileIDs) /
              sizeof(sNdsRelocFoxAnimFileIDs[0]));
         i++)
    {
        if (token == ndsRelocFileID(sNdsRelocFoxAnimFileIDs[i]))
        {
            return NDS_RELOC_ASSET_FOX_ANIM_FIRST + i;
        }
    }
    return NDS_RELOC_ASSET_INVALID;
}

/* Task 74 tried memoizing this. ndsRelocFileID returns (u32)(uintptr_t)file_id,
 * the address of the file-id global rather than a value read from it, so every
 * token below is a link-time constant and the chain is a pure function of its
 * argument -- a direct-mapped cache in front of it was provably safe, and Task
 * 71 had priced the chain at 9,306 ticks/frame on a load frame.
 *
 * It measured worse. SRC, the bucket it targets, rose 1,920 at P95 and WORK-H
 * rose 11,584 at P50, while STG -- which a token lookup cannot touch -- moved
 * 8,128. The 512 bytes of cache arrays shift layout, and this ROM's placement
 * sensitivity puts the noise floor above the effect being chased. A hundred
 * compares against link-time immediates are branch-predictable and already
 * resident; three lookup arrays in .main.bss are not.
 *
 * Do not re-attempt without an instrument that resolves below ~8,000 ticks.
 *
 * R2-06 E10 built that instrument -- per-frame profiler regions split by a
 * marker symbol, ~1 tick resolution -- and R2-06 E11 then used it to REFUTE the
 * whole line, including a fix that added no data at all. What it measured here:
 *
 *   - 630 calls on the 16 load frames of window 796..923, ZERO on all 112 clean
 *     frames, 39,475 ticks per load frame, 1,003 cycles / 550 instructions each.
 *   - Task 71's 9,306 was 4.2x low, because ndsRelocMarioBattleAnimAssetIDForToken,
 *     ndsRelocFoxAnimAssetIDForToken and ndsRelocIsMarioFoxAnimID all inline into
 *     this function; it priced the compare chain and not the scans behind it.
 *   - The chain is NOT the cost. 74.3% of calls resolve inside it. The two
 *     inlined pointer scans are 51.6% of the function on 14.3% of calls, because
 *     a full miss walks all 143 + 158 entries. So hoisting the scans, the
 *     obvious-looking fix, would have made 74.3% of calls pay 301 iterations.
 *   - The Mario/Fox pointer arrays cannot become index arithmetic: their targets
 *     span 1.7 MB non-monotonically (density 0.0%).
 *
 * E11 hoisted the ndsRelocIsMarioFoxAnimID range check above both scans and
 * deleted the five dead WAIT/WALK compares it subsumed -- provably identical,
 * NEGATIVE bytes added, and it worked: the function fell to 31,808 (-7,667,
 * -5,103 instructions) with the load-frame set bit-identical. It still lost.
 * Against a matched control (r206-e11-{control,tokenfirst}-128) the body improved
 * at every percentile below P90 and the tail did not: WORK-H P95 +15,744, P99
 * +59,200, over-gate 9 -> 11, and the two added frames were load frames 828 and
 * 847. Control-to-control noise on this ROM is P95 +5,376 and +/-1 over-gate.
 *
 * The lesson is about size, not about this function: a load-frame-only saving of
 * ~8,000 ticks cannot be banked through P95 here, because relinking moves the
 * tail by more than the saving. Do not bring another small load-frame cut. Either
 * remove this work in one change large enough to clear ~16,000 of tail movement,
 * or move it off the gameplay frame entirely, which changes WHEN the work happens
 * instead of shuffling where the code sits. */
typedef struct NDSP2FighterAnimTokenRow
{
    const void *token;
    u16 asset_id;
} NDSP2FighterAnimTokenRow;

/* BattleShip's reloc ABI passes &llFT*Anim*FileID. P2-3 aliases that were not
 * named in the historical port symbol table are zero-valued, so their ADDRESS
 * is the identity we must preserve. A giant generated if-chain made DK alone
 * add ~5.9 KiB of executable code. Store the exact generated address->u16 id
 * relation instead: O2R file ids are u16, the table is cold load-time data, and
 * numeric external-dependency ids take the O(1) range path below. */
/* Numeric bounds over the table below, measured once on first use. See the
 * early-out in ndsRelocP2FighterAnimAssetIDForToken. */
static u16 sNdsP2FighterAnimTokenIdMin;
static u16 sNdsP2FighterAnimTokenIdMax;
static sb32 sNdsP2FighterAnimTokenBoundsReady;

static const NDSP2FighterAnimTokenRow sNdsP2FighterAnimTokens[] =
{
#define NDS_P2_FIGHTER_ANIM_TOKEN_ROW(symbol_, id_, path_) \
    { &symbol_, (u16)(id_) },
#if NDS_P2_LUIGI
    NDS_P2_LUIGI_ANIM_ASSET_ROWS(NDS_P2_FIGHTER_ANIM_TOKEN_ROW)
#endif
#if NDS_P2_DONKEY
    NDS_P2_DONKEY_ANIM_ASSET_ROWS(NDS_P2_FIGHTER_ANIM_TOKEN_ROW)
#endif
#if NDS_P2_CAPTAIN
    NDS_P2_CAPTAIN_ANIM_ASSET_ROWS(NDS_P2_FIGHTER_ANIM_TOKEN_ROW)
#endif
    NDS_P2_SAMUS_ANIM_ASSET_ROWS(NDS_P2_FIGHTER_ANIM_TOKEN_ROW)
#if NDS_P2_LINK
    NDS_P2_LINK_ANIM_ASSET_ROWS(NDS_P2_FIGHTER_ANIM_TOKEN_ROW)
#endif
    NDS_P2_PIKACHU_ANIM_ASSET_ROWS(NDS_P2_FIGHTER_ANIM_TOKEN_ROW)
    NDS_P2_YOSHI_ANIM_ASSET_ROWS(NDS_P2_FIGHTER_ANIM_TOKEN_ROW)
    NDS_P2_NESS_ANIM_ASSET_ROWS(NDS_P2_FIGHTER_ANIM_TOKEN_ROW)
    NDS_P2_PURIN_ANIM_ASSET_ROWS(NDS_P2_FIGHTER_ANIM_TOKEN_ROW)
    NDS_P2_KIRBY_ANIM_ASSET_ROWS(NDS_P2_FIGHTER_ANIM_TOKEN_ROW)
#undef NDS_P2_FIGHTER_ANIM_TOKEN_ROW
};

#define NDS_P2_FIGHTER_ANIM_TOKEN_COUNT \
    ((u32)(sizeof(sNdsP2FighterAnimTokens) / sizeof(sNdsP2FighterAnimTokens[0])))
_Static_assert(sizeof(sNdsP2FighterAnimTokens) / sizeof(sNdsP2FighterAnimTokens[0]) <= 0xffffu,
               "token index rows are u16");
/* Row indices ordered by (asset id, row) and by (symbol address, row): the
 * row component keeps the first row in table order first among equal keys. */
static u16 sNdsP2FighterAnimTokenById[NDS_P2_FIGHTER_ANIM_TOKEN_COUNT];
static u16 sNdsP2FighterAnimTokenByAddress[NDS_P2_FIGHTER_ANIM_TOKEN_COUNT];
static sb32 sNdsP2FighterAnimTokenIndexReady;

static sb32 ndsRelocP2FighterAnimTokenBefore(u32 a, u32 b, sb32 by_address)
{
    u32 ka = by_address ?
        ndsRelocFileID(sNdsP2FighterAnimTokens[a].token) :
        (u32)sNdsP2FighterAnimTokens[a].asset_id;
    u32 kb = by_address ?
        ndsRelocFileID(sNdsP2FighterAnimTokens[b].token) :
        (u32)sNdsP2FighterAnimTokens[b].asset_id;

    return ((ka < kb) || ((ka == kb) && (a < b))) ? TRUE : FALSE;
}

/* Once, at the first lookup: a shell sort of two u16 index arrays (no
 * allocation; the keys are unique with the row index, so the order is fixed). */
NDS_COLD static void __attribute__((noinline, cold))
ndsRelocP2FighterAnimTokenIndexSort(u16 *order, sb32 by_address)
{
    static const u16 gaps[] = { 701u, 301u, 132u, 57u, 23u, 10u, 4u, 1u };
    u32 g;
    u32 i;

    for (i = 0u; i < NDS_P2_FIGHTER_ANIM_TOKEN_COUNT; i++)
    {
        order[i] = (u16)i;
    }
    for (g = 0u; g < (u32)(sizeof(gaps) / sizeof(gaps[0])); g++)
    {
        u32 gap = gaps[g];

        for (i = gap; i < NDS_P2_FIGHTER_ANIM_TOKEN_COUNT; i++)
        {
            u16 value = order[i];
            u32 j = i;

            while ((j >= gap) &&
                   ndsRelocP2FighterAnimTokenBefore(value, order[j - gap],
                                                    by_address))
            {
                order[j] = order[j - gap];
                j -= gap;
            }
            order[j] = value;
        }
    }
}

static inline void ndsRelocP2FighterAnimTokenIndexBuild(void)
{
    if (sNdsP2FighterAnimTokenIndexReady == FALSE)
    {
        ndsRelocP2FighterAnimTokenIndexSort(sNdsP2FighterAnimTokenById, FALSE);
        ndsRelocP2FighterAnimTokenIndexSort(sNdsP2FighterAnimTokenByAddress,
                                            TRUE);
        sNdsP2FighterAnimTokenIndexReady = TRUE;
    }
}

static u32 ndsRelocP2FighterAnimAssetIDForToken(u32 token)
{
#if NDS_P2_LUIGI
    if ((token >= NDS_P2_LUIGI_ANIM_FIRST) &&
        (token <= NDS_P2_LUIGI_ANIM_LAST))
    {
        return token;
    }
#endif
#if NDS_P2_DONKEY
    if ((token >= NDS_P2_DONKEY_ANIM_FIRST) &&
        (token <= NDS_P2_DONKEY_ANIM_LAST))
    {
        return token;
    }
#endif
#if NDS_P2_CAPTAIN
    if ((token >= NDS_P2_CAPTAIN_ANIM_FIRST) &&
        (token <= NDS_P2_CAPTAIN_ANIM_LAST))
    {
        return token;
    }
#endif
    if ((token >= NDS_P2_SAMUS_ANIM_FIRST) &&
        (token <= NDS_P2_SAMUS_ANIM_LAST))
    {
        return token;
    }
#if NDS_P2_LINK
    if ((token >= NDS_P2_LINK_ANIM_FIRST) &&
        (token <= NDS_P2_LINK_ANIM_LAST))
    {
        return token;
    }
#endif
    if ((token >= NDS_P2_PIKACHU_ANIM_FIRST) &&
        (token <= NDS_P2_PIKACHU_ANIM_LAST))
    {
        return token;
    }
    if ((token >= NDS_P2_YOSHI_ANIM_FIRST) &&
        (token <= NDS_P2_YOSHI_ANIM_LAST))
    {
        return token;
    }
    if ((token >= NDS_P2_NESS_ANIM_FIRST) &&
        (token <= NDS_P2_NESS_ANIM_LAST))
    {
        return token;
    }
    if ((token >= NDS_P2_PURIN_ANIM_FIRST) &&
        (token <= NDS_P2_PURIN_ANIM_LAST))
    {
        return token;
    }
    if ((token >= NDS_P2_KIRBY_ANIM_FIRST) &&
        (token <= NDS_P2_KIRBY_ANIM_LAST))
    {
        return token;
    }
    /* Borrowed animation files are answered here, not by the range arms
     * above: a borrowed id lies outside the borrowing fighter's own
     * FIRST..LAST span, so no per-owner range can claim it, and no numeric
     * early-out may fire before this table is reached. The table holds every
     * motion-referenced file of every admitted fighter -- local or borrowed
     * -- and each row answers both token shapes: the symbol address for
     * BattleShip's &ll...FileID tokens and the numeric id for extern-table
     * ids. Cost: the range arms above are untouched and still answer every
     * admitted fighter's own numeric ids without reaching this loop, and
     * pointer tokens take the same single pass as before with one added u16
     * compare per row; only a range-missing numeric pays the full pass, and
     * those already fall through to the longer core/dependency chains below.
     * The numeric arm leads so numeric tokens never pay an address load.
     *
     * THE NUMERIC EARLY-OUT IS RESTORED, CORRECTLY. Deleting it outright made
     * every range-missing numeric id scan all ~690 rows, and startup resolves
     * a lot of those -- core and dependency ids like LinkModel 0x144 -- long
     * before the core table below. That is what the deleted comment was
     * protecting, and removing it cost the one-minute battle arm its budget
     * outright: 3,600 seconds to reach presented frame 224. The bound below
     * keeps the rejection O(1) while still admitting borrowed ids, because it
     * is measured from the table itself rather than from one fighter's own
     * FIRST..LAST span. One pass, once. */
    if (token <= 0xffffu)
    {
        if (sNdsP2FighterAnimTokenBoundsReady == FALSE)
        {
            u32 j;

            sNdsP2FighterAnimTokenIdMin = 0xffffu;
            sNdsP2FighterAnimTokenIdMax = 0u;
            for (j = 0u;
                 j < (sizeof(sNdsP2FighterAnimTokens) /
                      sizeof(sNdsP2FighterAnimTokens[0]));
                 j++)
            {
                u16 id = sNdsP2FighterAnimTokens[j].asset_id;

                if (id < sNdsP2FighterAnimTokenIdMin)
                {
                    sNdsP2FighterAnimTokenIdMin = id;
                }
                if (id > sNdsP2FighterAnimTokenIdMax)
                {
                    sNdsP2FighterAnimTokenIdMax = id;
                }
            }
            sNdsP2FighterAnimTokenBoundsReady = TRUE;
        }
        if ((token < (u32)sNdsP2FighterAnimTokenIdMin) ||
            (token > (u32)sNdsP2FighterAnimTokenIdMax))
        {
            return NDS_RELOC_ASSET_INVALID;
        }
    }
    /* A numeric token can only equal an asset id and an address token only a
     * row's symbol address (addresses are above 0xffff), so the table scan this
     * replaces answered "some row has this id" for the first and "the first row
     * in table order with this address" for the second. Both are binary
     * searches over the index below: ~10 probes instead of a ~690-row walk,
     * which several lookups per status change paid on a four-kind roster. */
    ndsRelocP2FighterAnimTokenIndexBuild();
    if (token <= 0xffffu)
    {
        u32 lo = 0u;
        u32 hi = NDS_P2_FIGHTER_ANIM_TOKEN_COUNT;

        while (lo < hi)
        {
            u32 mid = (lo + hi) >> 1;

            if ((u32)sNdsP2FighterAnimTokens[
                    sNdsP2FighterAnimTokenById[mid]].asset_id < token)
            {
                lo = mid + 1u;
            }
            else
            {
                hi = mid;
            }
        }
        if ((lo < NDS_P2_FIGHTER_ANIM_TOKEN_COUNT) &&
            ((u32)sNdsP2FighterAnimTokens[
                 sNdsP2FighterAnimTokenById[lo]].asset_id == token))
        {
            return token;
        }
    }
    else
    {
        u32 lo = 0u;
        u32 hi = NDS_P2_FIGHTER_ANIM_TOKEN_COUNT;

        while (lo < hi)
        {
            u32 mid = (lo + hi) >> 1;

            if (ndsRelocFileID(sNdsP2FighterAnimTokens[
                    sNdsP2FighterAnimTokenByAddress[mid]].token) < token)
            {
                lo = mid + 1u;
            }
            else
            {
                hi = mid;
            }
        }
        if ((lo < NDS_P2_FIGHTER_ANIM_TOKEN_COUNT) &&
            (ndsRelocFileID(sNdsP2FighterAnimTokens[
                 sNdsP2FighterAnimTokenByAddress[lo]].token) == token))
        {
            return (u32)sNdsP2FighterAnimTokens[
                sNdsP2FighterAnimTokenByAddress[lo]].asset_id;
        }
    }
    return NDS_RELOC_ASSET_INVALID;
}

static u32 ndsRelocAssetIDForTokenChain(u32 token)
{
    /* A P4 numeric donor id lies past every row below -- the source's own
     * ids end at 0x853, and the rows compare those or link addresses -- so
     * the last row's answer is given first rather than after several hundred
     * compares (perf, 2026-10-18: a P4 clip fetch whose token the memo had
     * lost walked the whole chain, 14.5K cycles). */
    if ((token >= 0x854u) && (token < 0x10000u))
    {
        return (ndsP4RelocAssetPath(token) != NULL) ?
            token : NDS_RELOC_ASSET_INVALID;
    }
    u32 p2_anim_asset_id = ndsRelocP2FighterAnimAssetIDForToken(token);

    if (p2_anim_asset_id != NDS_RELOC_ASSET_INVALID)
    {
        return p2_anim_asset_id;
    }
    /* Core P2-3 symbols have real source IDs in the decomp-derived port table,
     * but keep this generated bridge beside the animation table so the
     * admission catalog remains the authority. */
#define NDS_P2_FIGHTER_TOKEN_ROW(symbol_, id_, path_) \
    if ((token == ndsRelocFileID(&symbol_)) || (token == (id_))) return (id_);
#define NDS_P2_FIGHTER_DEPENDENCY_TOKEN_ROW(id_, path_) \
    if (token == (id_)) return (id_);
#if NDS_P2_LUIGI
    NDS_P2_LUIGI_CORE_ASSET_ROWS(NDS_P2_FIGHTER_TOKEN_ROW)
    NDS_P2_LUIGI_DEPENDENCY_ASSET_ROWS(NDS_P2_FIGHTER_DEPENDENCY_TOKEN_ROW)
#endif
#if NDS_P2_DONKEY
    NDS_P2_DONKEY_CORE_ASSET_ROWS(NDS_P2_FIGHTER_TOKEN_ROW)
    NDS_P2_DONKEY_DEPENDENCY_ASSET_ROWS(NDS_P2_FIGHTER_DEPENDENCY_TOKEN_ROW)
#endif
#if NDS_P2_CAPTAIN
    NDS_P2_CAPTAIN_CORE_ASSET_ROWS(NDS_P2_FIGHTER_TOKEN_ROW)
    NDS_P2_CAPTAIN_DEPENDENCY_ASSET_ROWS(NDS_P2_FIGHTER_DEPENDENCY_TOKEN_ROW)
#endif
    NDS_P2_SAMUS_CORE_ASSET_ROWS(NDS_P2_FIGHTER_TOKEN_ROW)
    NDS_P2_SAMUS_DEPENDENCY_ASSET_ROWS(NDS_P2_FIGHTER_DEPENDENCY_TOKEN_ROW)
#if NDS_P2_LINK
    NDS_P2_LINK_CORE_ASSET_ROWS(NDS_P2_FIGHTER_TOKEN_ROW)
    NDS_P2_LINK_DEPENDENCY_ASSET_ROWS(NDS_P2_FIGHTER_DEPENDENCY_TOKEN_ROW)
#endif
    NDS_P2_PIKACHU_CORE_ASSET_ROWS(NDS_P2_FIGHTER_TOKEN_ROW)
    NDS_P2_PIKACHU_DEPENDENCY_ASSET_ROWS(NDS_P2_FIGHTER_DEPENDENCY_TOKEN_ROW)
    NDS_P2_YOSHI_CORE_ASSET_ROWS(NDS_P2_FIGHTER_TOKEN_ROW)
    NDS_P2_YOSHI_DEPENDENCY_ASSET_ROWS(NDS_P2_FIGHTER_DEPENDENCY_TOKEN_ROW)
    NDS_P2_NESS_CORE_ASSET_ROWS(NDS_P2_FIGHTER_TOKEN_ROW)
    NDS_P2_NESS_DEPENDENCY_ASSET_ROWS(NDS_P2_FIGHTER_DEPENDENCY_TOKEN_ROW)
    NDS_P2_PURIN_CORE_ASSET_ROWS(NDS_P2_FIGHTER_TOKEN_ROW)
    NDS_P2_PURIN_DEPENDENCY_ASSET_ROWS(NDS_P2_FIGHTER_DEPENDENCY_TOKEN_ROW)
    NDS_P2_KIRBY_CORE_ASSET_ROWS(NDS_P2_FIGHTER_TOKEN_ROW)
    NDS_P2_KIRBY_DEPENDENCY_ASSET_ROWS(NDS_P2_FIGHTER_DEPENDENCY_TOKEN_ROW)
#undef NDS_P2_FIGHTER_DEPENDENCY_TOKEN_ROW
#undef NDS_P2_FIGHTER_TOKEN_ROW
    if (token == ndsRelocFileID(&llN64LogoFileID)) return NDS_RELOC_ASSET_N64_LOGO;
    if (token == ndsRelocFileID(&llIFCommonPlayerFileID)) return NDS_RELOC_ASSET_IF_COMMON_PLAYER;
    if (token == ndsRelocFileID(&llIFCommonGameStatusFileID)) return NDS_RELOC_ASSET_IF_COMMON_GAME_STATUS;
    if (token == ndsRelocFileID(&llIFCommonItemFileID)) return NDS_RELOC_ASSET_IF_COMMON_ITEM;
    if (token == ndsRelocFileID(&llIFCommonPlayerDamageFileID)) return NDS_RELOC_ASSET_IF_COMMON_PLAYER_DAMAGE;
    if (token == ndsRelocFileID(&llIFCommonTimerFileID)) return NDS_RELOC_ASSET_IF_COMMON_TIMER;
    if (token == ndsRelocFileID(&llIFCommonDigitsFileID)) return NDS_RELOC_ASSET_IF_COMMON_DIGITS;
    if (token == ndsRelocFileID(&llIFCommonBattlePauseFileID)) return NDS_RELOC_ASSET_IF_COMMON_BATTLE_PAUSE;
    if (token == ndsRelocFileID(&llIFCommonPlayerTagsFileID)) return NDS_RELOC_ASSET_IF_COMMON_PLAYER_TAGS;
    if (token == ndsRelocFileID(&llIFCommonAnnounceCommonFileID)) return NDS_RELOC_ASSET_IF_COMMON_ANNOUNCE;
    if (token == ndsRelocFileID(&llMNVSResultsFileID)) return NDS_RELOC_ASSET_MN_VS_RESULTS;
    if (token == ndsRelocFileID(&ll_29_FileID)) return NDS_RELOC_ASSET_SC1P_TRAINING_SPRITES;
    /* Its raw id too: SC1PTrainingMode's 118 sprite pointers are externs
     * into it (P7 C4), and an extern list carries file ids. */
    if (token == NDS_RELOC_ASSET_SC1P_TRAINING_SPRITES) return NDS_RELOC_ASSET_SC1P_TRAINING_SPRITES;
    if (token == ndsRelocFileID(&llNNessModelFileID)) return NDS_RELOC_ASSET_NNESS_MODEL;
    if (token == ndsRelocFileID(&llNNessMainFileID)) return NDS_RELOC_ASSET_NNESS_MAIN;
    if (token == ndsRelocFileID(&llNPurinModelFileID)) return NDS_RELOC_ASSET_NPURIN_MODEL;
    if (token == ndsRelocFileID(&llNPurinMainFileID)) return NDS_RELOC_ASSET_NPURIN_MAIN;
    if (token == ndsRelocFileID(&llNPikachuModelFileID)) return NDS_RELOC_ASSET_NPIKACHU_MODEL;
    if (token == ndsRelocFileID(&llNPikachuMainFileID)) return NDS_RELOC_ASSET_NPIKACHU_MAIN;
    if (token == ndsRelocFileID(&llNKirbyModelFileID)) return NDS_RELOC_ASSET_NKIRBY_MODEL;
    if (token == ndsRelocFileID(&llNKirbyMainFileID)) return NDS_RELOC_ASSET_NKIRBY_MAIN;
    if (token == ndsRelocFileID(&llNCaptainModelFileID)) return NDS_RELOC_ASSET_NCAPTAIN_MODEL;
    if (token == ndsRelocFileID(&llNCaptainMainFileID)) return NDS_RELOC_ASSET_NCAPTAIN_MAIN;
    if (token == ndsRelocFileID(&llNYoshiModelFileID)) return NDS_RELOC_ASSET_NYOSHI_MODEL;
    if (token == ndsRelocFileID(&llNYoshiMainFileID)) return NDS_RELOC_ASSET_NYOSHI_MAIN;
    if (token == ndsRelocFileID(&llNLinkModelFileID)) return NDS_RELOC_ASSET_NLINK_MODEL;
    if (token == ndsRelocFileID(&llNLinkMainFileID)) return NDS_RELOC_ASSET_NLINK_MAIN;
    if (token == ndsRelocFileID(&llNLuigiMainFileID)) return NDS_RELOC_ASSET_NLUIGI_MAIN;
    if (token == ndsRelocFileID(&llNSamusModelFileID)) return NDS_RELOC_ASSET_NSAMUS_MODEL;
    if (token == ndsRelocFileID(&llNSamusMainFileID)) return NDS_RELOC_ASSET_NSAMUS_MAIN;
    if (token == ndsRelocFileID(&llNDonkeyModelFileID)) return NDS_RELOC_ASSET_NDONKEY_MODEL;
    if (token == ndsRelocFileID(&llNDonkeyMainFileID)) return NDS_RELOC_ASSET_NDONKEY_MAIN;
    if (token == ndsRelocFileID(&llNFoxModelFileID)) return NDS_RELOC_ASSET_NFOX_MODEL;
    if (token == ndsRelocFileID(&llNFoxMainFileID)) return NDS_RELOC_ASSET_NFOX_MAIN;
    if (token == ndsRelocFileID(&llNMarioModelFileID)) return NDS_RELOC_ASSET_NMARIO_MODEL;
    if (token == ndsRelocFileID(&llNMarioMainFileID)) return NDS_RELOC_ASSET_NMARIO_MAIN;
    /* The same files named by file id, as extern dependencies: a polygon
     * main names its model and MasterHandIcon, each model the shared polygon
     * texture, Master Hand's main its motion and model. Without these the
     * extern tree refused every polygon main and its pointers stayed raw.
     */
    switch (token)
    {
    case NDS_RELOC_ASSET_NNESS_MODEL:
    case NDS_RELOC_ASSET_NPURIN_MODEL:
    case NDS_RELOC_ASSET_NPIKACHU_MODEL:
    case NDS_RELOC_ASSET_NKIRBY_MODEL:
    case NDS_RELOC_ASSET_NCAPTAIN_MODEL:
    case NDS_RELOC_ASSET_NYOSHI_MODEL:
    case NDS_RELOC_ASSET_NLINK_MODEL:
    case NDS_RELOC_ASSET_NSAMUS_MODEL:
    case NDS_RELOC_ASSET_NDONKEY_MODEL:
    case NDS_RELOC_ASSET_NFOX_MODEL:
    case NDS_RELOC_ASSET_NMARIO_MODEL:
    case NDS_RELOC_ASSET_BOSS_MODEL:
    case NDS_RELOC_ASSET_BOSS_MAIN_MOTION:
    case NDS_RELOC_ASSET_NCOMMON_TEXTURE:
    case NDS_RELOC_ASSET_MASTER_HAND_ICON:
        return token;
    default:
        break;
    }
#if NDS_P2_GDONKEY
    /* Giant DK's own file is his main; model, motion, specials and icon are
     * Donkey Kong's (GDonkeyMain's extern table), rowed with Donkey. */
    if (token == ndsRelocFileID(&llGDonkeyMainFileID)) return NDS_RELOC_ASSET_GDONKEY_MAIN;
#endif
#if NDS_P2_MMARIO
    if (token == ndsRelocFileID(&llMMarioMainFileID)) return NDS_RELOC_ASSET_MMARIO_MAIN;
    if (token == ndsRelocFileID(&llMMarioMainMotionFileID)) return NDS_RELOC_ASSET_MMARIO_MAIN_MOTION;
    if (token == ndsRelocFileID(&llMMarioModelFileID)) return NDS_RELOC_ASSET_MMARIO_MODEL;
    if (token == NDS_RELOC_ASSET_MMARIO_MAIN_MOTION) return NDS_RELOC_ASSET_MMARIO_MAIN_MOTION;
    if (token == NDS_RELOC_ASSET_MMARIO_MODEL) return NDS_RELOC_ASSET_MMARIO_MODEL;
#endif
    if (token == ndsRelocFileID(&llBossModelFileID)) return NDS_RELOC_ASSET_BOSS_MODEL;
    if (token == ndsRelocFileID(&llBossMainMotionFileID)) return NDS_RELOC_ASSET_BOSS_MAIN_MOTION;
    if (token == ndsRelocFileID(&llBossMainFileID)) return NDS_RELOC_ASSET_BOSS_MAIN;
    if (token == ndsRelocFileID(&llMNScreenAdjustFileID)) return NDS_RELOC_ASSET_MN_SCREEN_ADJUST;
    if (token == ndsRelocFileID(&llBonus2CommonFileID)) return NDS_RELOC_ASSET_BONUS2_COMMON;
    if (token == ndsRelocFileID(&llBonusPicturePlatformFileID)) return NDS_RELOC_ASSET_BONUS_PICTURE_PLATFORM;
    if (token == ndsRelocFileID(&llBonusPictureFileID)) return NDS_RELOC_ASSET_BONUS_PICTURE;
    if (token == ndsRelocFileID(&llCharacterNamesFileID)) return NDS_RELOC_ASSET_CHARACTER_NAMES;
    if (token == ndsRelocFileID(&llMNPlayersDifficultyFileID)) return NDS_RELOC_ASSET_MN_PLAYERS_DIFFICULTY;
    if (token == ndsRelocFileID(&llSCExplainMainFileID)) return NDS_RELOC_ASSET_SC_EXPLAIN_MAIN;
    if (token == ndsRelocFileID(&llSCExplainGraphicsFileID)) return NDS_RELOC_ASSET_SC_EXPLAIN_GRAPHICS;
    if (token == ndsRelocFileID(&llGRWallpaperTrainingYellowFileID)) return NDS_RELOC_ASSET_GR_WALLPAPER_TRAINING_YELLOW;
    if (token == ndsRelocFileID(&llGRWallpaperTrainingBlueFileID)) return NDS_RELOC_ASSET_GR_WALLPAPER_TRAINING_BLUE;
    if (token == ndsRelocFileID(&llSC1PTrainingModeFileID)) return NDS_RELOC_ASSET_SC1P_TRAINING_MODE;
    if (token == ndsRelocFileID(&llMNSoundTestFileID)) return NDS_RELOC_ASSET_MN_SOUND_TEST;
    if (token == ndsRelocFileID(&llMNBackupClearHeaderOptionFileID)) return NDS_RELOC_ASSET_MN_BACKUP_CLEAR_HEADER_OPTION;
    if (token == ndsRelocFileID(&llMNBackupClearFileID)) return NDS_RELOC_ASSET_MN_BACKUP_CLEAR;
    if (token == ndsRelocFileID(&llMNOptionFileID)) return NDS_RELOC_ASSET_MN_OPTION;
    if (token == ndsRelocFileID(&llMNCharactersFileID)) return NDS_RELOC_ASSET_MN_CHARACTERS;
    if (token == ndsRelocFileID(&llMNVSRecordMainFileID)) return NDS_RELOC_ASSET_MNVS_RECORD_MAIN;
    if (token == ndsRelocFileID(&llMNDataCommonFileID)) return NDS_RELOC_ASSET_MN_DATA_COMMON;
    if (token == ndsRelocFileID(&llMNDataFileID)) return NDS_RELOC_ASSET_MN_DATA;
    if (token == ndsRelocFileID(&llMNCongraYoshiTopFileID)) return NDS_RELOC_ASSET_MN_CONGRA_YOSHI_TOP;
    if (token == ndsRelocFileID(&llMNCongraYoshiBottomFileID)) return NDS_RELOC_ASSET_MN_CONGRA_YOSHI_BOTTOM;
    if (token == ndsRelocFileID(&llMNCongraSamusTopFileID)) return NDS_RELOC_ASSET_MN_CONGRA_SAMUS_TOP;
    if (token == ndsRelocFileID(&llMNCongraSamusBottomFileID)) return NDS_RELOC_ASSET_MN_CONGRA_SAMUS_BOTTOM;
    if (token == ndsRelocFileID(&llMNCongraPurinTopFileID)) return NDS_RELOC_ASSET_MN_CONGRA_PURIN_TOP;
    if (token == ndsRelocFileID(&llMNCongraPurinBottomFileID)) return NDS_RELOC_ASSET_MN_CONGRA_PURIN_BOTTOM;
    if (token == ndsRelocFileID(&llMNCongraPikachuTopFileID)) return NDS_RELOC_ASSET_MN_CONGRA_PIKACHU_TOP;
    if (token == ndsRelocFileID(&llMNCongraPikachuBottomFileID)) return NDS_RELOC_ASSET_MN_CONGRA_PIKACHU_BOTTOM;
    if (token == ndsRelocFileID(&llMNCongraNessTopFileID)) return NDS_RELOC_ASSET_MN_CONGRA_NESS_TOP;
    if (token == ndsRelocFileID(&llMNCongraNessBottomFileID)) return NDS_RELOC_ASSET_MN_CONGRA_NESS_BOTTOM;
    if (token == ndsRelocFileID(&llMNCongraMarioTopFileID)) return NDS_RELOC_ASSET_MN_CONGRA_MARIO_TOP;
    if (token == ndsRelocFileID(&llMNCongraMarioBottomFileID)) return NDS_RELOC_ASSET_MN_CONGRA_MARIO_BOTTOM;
    if (token == ndsRelocFileID(&llMNCongraLuigiTopFileID)) return NDS_RELOC_ASSET_MN_CONGRA_LUIGI_TOP;
    if (token == ndsRelocFileID(&llMNCongraLuigiBottomFileID)) return NDS_RELOC_ASSET_MN_CONGRA_LUIGI_BOTTOM;
    if (token == ndsRelocFileID(&llMNCongraLinkTopFileID)) return NDS_RELOC_ASSET_MN_CONGRA_LINK_TOP;
    if (token == ndsRelocFileID(&llMNCongraLinkBottomFileID)) return NDS_RELOC_ASSET_MN_CONGRA_LINK_BOTTOM;
    if (token == ndsRelocFileID(&llMNCongraKirbyTopFileID)) return NDS_RELOC_ASSET_MN_CONGRA_KIRBY_TOP;
    if (token == ndsRelocFileID(&llMNCongraKirbyBottomFileID)) return NDS_RELOC_ASSET_MN_CONGRA_KIRBY_BOTTOM;
    if (token == ndsRelocFileID(&llMNCongraFoxTopFileID)) return NDS_RELOC_ASSET_MN_CONGRA_FOX_TOP;
    if (token == ndsRelocFileID(&llMNCongraFoxBottomFileID)) return NDS_RELOC_ASSET_MN_CONGRA_FOX_BOTTOM;
    if (token == ndsRelocFileID(&llMNCongraDonkeyTopFileID)) return NDS_RELOC_ASSET_MN_CONGRA_DONKEY_TOP;
    if (token == ndsRelocFileID(&llMNCongraDonkeyBottomFileID)) return NDS_RELOC_ASSET_MN_CONGRA_DONKEY_BOTTOM;
    if (token == ndsRelocFileID(&llMNCongraCaptainTopFileID)) return NDS_RELOC_ASSET_MN_CONGRA_CAPTAIN_TOP;
    if (token == ndsRelocFileID(&llMNCongraCaptainBottomFileID)) return NDS_RELOC_ASSET_MN_CONGRA_CAPTAIN_BOTTOM;
    if (token == ndsRelocFileID(&llMVEndingFileID)) return NDS_RELOC_ASSET_MV_ENDING;
    if (token == ndsRelocFileID(&llSCStaffrollFileID)) return NDS_RELOC_ASSET_SC_STAFFROLL;
    if (token == ndsRelocFileID(&llMNMessageFileID)) return NDS_RELOC_ASSET_MN_MESSAGE;
    if (token == ndsRelocFileID(&llMNPlayers1PModeFileID)) return NDS_RELOC_ASSET_MN_PLAYERS1P_MODE;
    if (token == ndsRelocFileID(&llMN1PContinueFileID)) return NDS_RELOC_ASSET_MN1P_CONTINUE;
    if (token == ndsRelocFileID(&llMN1PFileID)) return NDS_RELOC_ASSET_MN1P;
    if (token == ndsRelocFileID(&llSC1PIntroFileID)) return NDS_RELOC_ASSET_SC1P_INTRO;
    if (token == ndsRelocFileID(&llSC1PChallengerFileID)) return NDS_RELOC_ASSET_SC1P_CHALLENGER;
    if (token == ndsRelocFileID(&llSC1PStageClear3FileID)) return NDS_RELOC_ASSET_SC1P_STAGE_CLEAR3;
    if (token == ndsRelocFileID(&llSC1PStageClear2FileID)) return NDS_RELOC_ASSET_SC1P_STAGE_CLEAR2;
    if (token == ndsRelocFileID(&llSC1PStageClear1FileID)) return NDS_RELOC_ASSET_SC1P_STAGE_CLEAR1;
    if (token == ndsRelocFileID(&llFTEmblemModelsFileID)) return NDS_RELOC_ASSET_FT_EMBLEM_MODELS;
    if (token == ndsRelocFileID(&llFTStocksZakoFileID)) return NDS_RELOC_ASSET_FT_STOCKS_ZAKO;
    if (token == ndsRelocFileID(&llLBTransitionAeroplaneFileID)) return NDS_RELOC_ASSET_TRANSITION_AEROPLANE;
    if (token == ndsRelocFileID(&llLBTransitionCheckFileID)) return NDS_RELOC_ASSET_TRANSITION_CHECK;
    if (token == ndsRelocFileID(&llLBTransitionGakubuthiFileID)) return NDS_RELOC_ASSET_TRANSITION_GAKUBUTHI;
    if (token == ndsRelocFileID(&llLBTransitionKannonFileID)) return NDS_RELOC_ASSET_TRANSITION_KANNON;
    if (token == ndsRelocFileID(&llLBTransitionStarFileID)) return NDS_RELOC_ASSET_TRANSITION_STAR;
    if (token == ndsRelocFileID(&llLBTransitionSudare1FileID)) return NDS_RELOC_ASSET_TRANSITION_SUDARE1;
    if (token == ndsRelocFileID(&llLBTransitionSudare2FileID)) return NDS_RELOC_ASSET_TRANSITION_SUDARE2;
    if (token == ndsRelocFileID(&llLBTransitionBlockFileID)) return NDS_RELOC_ASSET_TRANSITION_BLOCK;
    if (token == ndsRelocFileID(&llLBTransitionRotScaleFileID)) return NDS_RELOC_ASSET_TRANSITION_ROTSCALE;
    if (token == ndsRelocFileID(&llLBTransitionCurtainFileID)) return NDS_RELOC_ASSET_TRANSITION_CURTAIN;
    if (token == ndsRelocFileID(&llLBTransitionCameraFileID)) return NDS_RELOC_ASSET_TRANSITION_CAMERA;
    if (token == ndsRelocFileID(&llSYKseg1ValidateFileID)) return NDS_RELOC_ASSET_SY_KSEG1_VALIDATE;
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
    if (token == ndsRelocFileID(&llMNPlayersCommonFileID)) return NDS_RELOC_ASSET_MN_PLAYERS_COMMON;
    if (token == ndsRelocFileID(&llMNPlayersGameModesFileID)) return NDS_RELOC_ASSET_MN_PLAYERS_GAME_MODES;
    if (token == ndsRelocFileID(&llMNPlayersPortraitsFileID)) return NDS_RELOC_ASSET_MN_PLAYERS_PORTRAITS;
    if (token == ndsRelocFileID(&llFTEmblemSpritesFileID)) return NDS_RELOC_ASSET_FT_EMBLEM_SPRITES;
    if (token == ndsRelocFileID(&llMNSelectCommonFileID)) return NDS_RELOC_ASSET_MN_SELECT_COMMON;
    if (token == ndsRelocFileID(&llMNPlayersSpotlightFileID)) return NDS_RELOC_ASSET_MN_PLAYERS_SPOTLIGHT;
    if (token == ndsRelocFileID(&llGRWallpaperTrainingBlackFileID)) return NDS_RELOC_ASSET_GR_WALLPAPER_TRAINING_BLACK;
    if (token == ndsRelocFileID(&llMNMapsFileID)) return NDS_RELOC_ASSET_MN_MAPS;
    if (token == ndsRelocFileID(&llMNCommonFontsFileID)) return NDS_RELOC_ASSET_MN_COMMON_FONTS;
    if (token == ndsRelocFileID(&llStageDreamLandFileID)) return NDS_RELOC_ASSET_STAGE_DREAM_LAND;
    if (token == ndsRelocFileID(&llStageCastleFileID)) return NDS_RELOC_ASSET_STAGE_CASTLE;
    if (token == ndsRelocFileID(&ll_113_FileID)) return NDS_RELOC_ASSET_EXTERN_DATA_BANK_113;
    if (token == ndsRelocFileID(&llGRPupupuMapFileID)) return NDS_RELOC_ASSET_GR_PUPUPU_MAP;
    if (token == ndsRelocFileID(&llGRInishieMapFileID)) return NDS_RELOC_ASSET_GR_INISHIE_MAP;
    if (token == ndsRelocFileID(&llGRHyruleMapFileID)) return NDS_RELOC_ASSET_GR_HYRULE_MAP;
    /* P2-4s1 Yoster: &ll...FileID symbol arms for BattleShip tokens plus
     * numeric arms for extern-table ids, mirroring the Dream Land rows. */
#if NDS_P2_STAGE_YOSTER
    if ((token == ndsRelocFileID(&llGRYosterMapFileID)) ||
        (token == NDS_RELOC_ASSET_GR_YOSTER_MAP))
    {
        return NDS_RELOC_ASSET_GR_YOSTER_MAP;
    }
    if ((token == ndsRelocFileID(&llStageYoshiFileID)) ||
        (token == 0x5du))
    {
        return NDS_RELOC_ASSET_STAGE_YOSHI;
    }
    if (token == NDS_RELOC_ASSET_EXTERN_DATA_BANK_110) return NDS_RELOC_ASSET_EXTERN_DATA_BANK_110;
    if (token == NDS_RELOC_ASSET_EXTERN_DATA_BANK_111) return NDS_RELOC_ASSET_EXTERN_DATA_BANK_111;
    if (token == NDS_RELOC_ASSET_MISC_DATA_BANK_154) return NDS_RELOC_ASSET_MISC_DATA_BANK_154;
#endif
#if NDS_P2_STAGE_CASTLE
    /* P2-4s2 Castle: same two shapes. The wallpaper it borrows from the
     * opening movie is already rowed unconditionally (asset 90), so only the
     * map and its two banks are new here. */
    if ((token == ndsRelocFileID(&llGRCastleMapFileID)) ||
        (token == NDS_RELOC_ASSET_GR_CASTLE_MAP))
    {
        return NDS_RELOC_ASSET_GR_CASTLE_MAP;
    }
    if (token == NDS_RELOC_ASSET_EXTERN_DATA_BANK_106) return NDS_RELOC_ASSET_EXTERN_DATA_BANK_106;
    if (token == NDS_RELOC_ASSET_MISC_DATA_BANK_156) return NDS_RELOC_ASSET_MISC_DATA_BANK_156;
    /* The wallpaper Castle borrows from the opening movie. It was already
     * rowed in nds_reloc_assets.c and staged, which is exactly why its
     * absence HERE was invisible: the path table and the token resolver are
     * two different tables, and a dependency the resolver cannot name comes
     * back as NDS_RELOC_ASSET_INVALID and takes the silent NULL return in
     * ndsRelocLoadExternTreeAsset. That is what aborted the first boot of
     * this stage. */
    if (token == NDS_RELOC_ASSET_MV_OPENING_ROOM_WALLPAPER)
    {
        return NDS_RELOC_ASSET_MV_OPENING_ROOM_WALLPAPER;
    }
#endif
#if NDS_P2_STAGE_JUNGLE
    /* P2-4s3 Congo Jungle: both shapes for the map, numeric for the three
     * files it declares. StageJungle has no ll...FileID symbol in the port
     * header, so it is numeric only -- the same shape the Yoster banks use. */
    if ((token == ndsRelocFileID(&llGRJungleMapFileID)) ||
        (token == NDS_RELOC_ASSET_GR_JUNGLE_MAP))
    {
        return NDS_RELOC_ASSET_GR_JUNGLE_MAP;
    }
    if (token == 0x5cu) return NDS_RELOC_ASSET_STAGE_JUNGLE;
    if (token == NDS_RELOC_ASSET_EXTERN_DATA_BANK_108) return NDS_RELOC_ASSET_EXTERN_DATA_BANK_108;
    if (token == NDS_RELOC_ASSET_MISC_DATA_BANK_158) return NDS_RELOC_ASSET_MISC_DATA_BANK_158;
#endif
#if NDS_P2_STAGE_ZEBES
    if ((token == ndsRelocFileID(&llGRZebesMapFileID)) ||
        (token == NDS_RELOC_ASSET_GR_ZEBES_MAP))
    {
        return NDS_RELOC_ASSET_GR_ZEBES_MAP;
    }
    if (token == 0x59u) return NDS_RELOC_ASSET_STAGE_ZEBES;
    if (token == NDS_RELOC_ASSET_EXTERN_DATA_BANK_105) return NDS_RELOC_ASSET_EXTERN_DATA_BANK_105;
    if (token == NDS_RELOC_ASSET_MISC_DATA_BANK_157) return NDS_RELOC_ASSET_MISC_DATA_BANK_157;
#endif
#if NDS_P2_STAGE_YAMABUKI
    if ((token == ndsRelocFileID(&llGRYamabukiMapFileID)) ||
        (token == NDS_RELOC_ASSET_GR_YAMABUKI_MAP))
    {
        return NDS_RELOC_ASSET_GR_YAMABUKI_MAP;
    }
    if (token == 0x5eu) return NDS_RELOC_ASSET_STAGE_POKEMON;
    if (token == NDS_RELOC_ASSET_EXTERN_DATA_BANK_112) return NDS_RELOC_ASSET_EXTERN_DATA_BANK_112;
    if (token == NDS_RELOC_ASSET_MISC_DATA_BANK_159) return NDS_RELOC_ASSET_MISC_DATA_BANK_159;
    if (token == NDS_RELOC_ASSET_MISC_DATA_BANK_160) return NDS_RELOC_ASSET_MISC_DATA_BANK_160;
#endif
#if NDS_P2_STAGE_INISHIE
    if (token == 0x5bu) return NDS_RELOC_ASSET_STAGE_HYRULE_WALLPAPER;
    if (token == NDS_RELOC_ASSET_EXTERN_DATA_BANK_107) return NDS_RELOC_ASSET_EXTERN_DATA_BANK_107;
    if (token == NDS_RELOC_ASSET_MISC_DATA_BANK_155) return NDS_RELOC_ASSET_MISC_DATA_BANK_155;
#endif
#if NDS_P2_STAGE_SECTOR
    if ((token == ndsRelocFileID(&llGRSectorMapFileID)) ||
        (token == NDS_RELOC_ASSET_GR_SECTOR_MAP))
    {
        return NDS_RELOC_ASSET_GR_SECTOR_MAP;
    }
    if (token == 0x63u) return NDS_RELOC_ASSET_STAGE_SECTOR;
    if (token == NDS_RELOC_ASSET_EXTERN_DATA_BANK_109_STAGE) return NDS_RELOC_ASSET_EXTERN_DATA_BANK_109_STAGE;
    if (token == NDS_RELOC_ASSET_MISC_DATA_BANK_153) return NDS_RELOC_ASSET_MISC_DATA_BANK_153;
#endif
#if NDS_P2_1P_GAME
    /* P2-6 step 7 (boss). Both shapes for the map -- the symbol address the
     * boss TU passes plus the numeric id the map's own extern table carries
     * -- numeric for the geometry bank and for the wallpaper container (96),
     * the Castle shape; the wallpaper is rowed only because the map's extern
     * table names it and the tree loader refuses an unrowed dependency. */
    if ((token == ndsRelocFileID(&llGRLastMapFileID)) ||
        (token == NDS_RELOC_ASSET_GR_LAST_MAP))
    {
        return NDS_RELOC_ASSET_GR_LAST_MAP;
    }
    if (token == NDS_RELOC_ASSET_EXTERN_DATA_BANK_114) return NDS_RELOC_ASSET_EXTERN_DATA_BANK_114;
    if (token == 0x60u) return NDS_RELOC_ASSET_STAGE_LAST_WALLPAPER;
    /* The other 1P arenas: both shapes for each map, numeric for the banks
     * and wallpaper containers (the Castle shape). */
    if ((token == ndsRelocFileID(&llGRMetalMapFileID)) ||
        (token == NDS_RELOC_ASSET_GR_METAL_MAP))
    {
        return NDS_RELOC_ASSET_GR_METAL_MAP;
    }
    if (token == NDS_RELOC_ASSET_EXTERN_DATA_BANK_117) return NDS_RELOC_ASSET_EXTERN_DATA_BANK_117;
    if (token == 0x62u) return NDS_RELOC_ASSET_STAGE_METAL_WALLPAPER;
    if ((token == ndsRelocFileID(&llGRYosterSmallMapFileID)) ||
        (token == NDS_RELOC_ASSET_GR_YOSTER_SMALL_MAP))
    {
        return NDS_RELOC_ASSET_GR_YOSTER_SMALL_MAP;
    }
    if (token == NDS_RELOC_ASSET_EXTERN_DATA_BANK_118) return NDS_RELOC_ASSET_EXTERN_DATA_BANK_118;
    if ((token == ndsRelocFileID(&llGRPupupuSmallMapFileID)) ||
        (token == NDS_RELOC_ASSET_GR_PUPUPU_SMALL_MAP))
    {
        return NDS_RELOC_ASSET_GR_PUPUPU_SMALL_MAP;
    }
    if (token == NDS_RELOC_ASSET_EXTERN_DATA_BANK_101) return NDS_RELOC_ASSET_EXTERN_DATA_BANK_101;
    if (token == NDS_RELOC_ASSET_EXTERN_DATA_BANK_100) return NDS_RELOC_ASSET_EXTERN_DATA_BANK_100;
    if ((token == ndsRelocFileID(&llGRZakoMapFileID)) ||
        (token == NDS_RELOC_ASSET_GR_ZAKO_MAP))
    {
        return NDS_RELOC_ASSET_GR_ZAKO_MAP;
    }
    if (token == NDS_RELOC_ASSET_EXTERN_DATA_BANK_116) return NDS_RELOC_ASSET_EXTERN_DATA_BANK_116;
    if (token == 0x61u) return NDS_RELOC_ASSET_STAGE_ZAKO_WALLPAPER;
    /* The bonus boards: both shapes for each map, numeric for banks, images,
     * the wallpaper, the target-object file and Kirby's copy table. */
    if (token == 0x77u) return NDS_RELOC_ASSET_STAGE_BONUS_WALLPAPER;
    if (token == NDS_RELOC_ASSET_EXTERN_DATA_BANK_120) return NDS_RELOC_ASSET_EXTERN_DATA_BANK_120;
    if (token == NDS_RELOC_ASSET_EXTERN_DATA_BANK_121) return NDS_RELOC_ASSET_EXTERN_DATA_BANK_121;
    if (token == NDS_RELOC_ASSET_EXTERN_DATA_BANK_122) return NDS_RELOC_ASSET_EXTERN_DATA_BANK_122;
    if (token == NDS_RELOC_ASSET_EXTERN_DATA_BANK_123) return NDS_RELOC_ASSET_EXTERN_DATA_BANK_123;
    if ((token == ndsRelocFileID(&llGRBonus1MarioMapFileID)) ||
        (token == NDS_RELOC_ASSET_GR_BONUS1_MARIO_MAP))
    {
        return NDS_RELOC_ASSET_GR_BONUS1_MARIO_MAP;
    }
    if (token == NDS_RELOC_ASSET_EXTERN_DATA_BANK_124) return NDS_RELOC_ASSET_EXTERN_DATA_BANK_124;
    if ((token == ndsRelocFileID(&llGRBonus1FoxMapFileID)) ||
        (token == NDS_RELOC_ASSET_GR_BONUS1_FOX_MAP))
    {
        return NDS_RELOC_ASSET_GR_BONUS1_FOX_MAP;
    }
    if (token == NDS_RELOC_ASSET_EXTERN_DATA_BANK_125) return NDS_RELOC_ASSET_EXTERN_DATA_BANK_125;
    if ((token == ndsRelocFileID(&llGRBonus1DonkeyMapFileID)) ||
        (token == NDS_RELOC_ASSET_GR_BONUS1_DONKEY_MAP))
    {
        return NDS_RELOC_ASSET_GR_BONUS1_DONKEY_MAP;
    }
    if (token == NDS_RELOC_ASSET_EXTERN_DATA_BANK_126) return NDS_RELOC_ASSET_EXTERN_DATA_BANK_126;
    if ((token == ndsRelocFileID(&llGRBonus1SamusMapFileID)) ||
        (token == NDS_RELOC_ASSET_GR_BONUS1_SAMUS_MAP))
    {
        return NDS_RELOC_ASSET_GR_BONUS1_SAMUS_MAP;
    }
    if (token == NDS_RELOC_ASSET_EXTERN_DATA_BANK_127) return NDS_RELOC_ASSET_EXTERN_DATA_BANK_127;
    if ((token == ndsRelocFileID(&llGRBonus1LuigiMapFileID)) ||
        (token == NDS_RELOC_ASSET_GR_BONUS1_LUIGI_MAP))
    {
        return NDS_RELOC_ASSET_GR_BONUS1_LUIGI_MAP;
    }
    if (token == NDS_RELOC_ASSET_EXTERN_DATA_BANK_128) return NDS_RELOC_ASSET_EXTERN_DATA_BANK_128;
    if ((token == ndsRelocFileID(&llGRBonus1LinkMapFileID)) ||
        (token == NDS_RELOC_ASSET_GR_BONUS1_LINK_MAP))
    {
        return NDS_RELOC_ASSET_GR_BONUS1_LINK_MAP;
    }
    if (token == NDS_RELOC_ASSET_EXTERN_DATA_BANK_129) return NDS_RELOC_ASSET_EXTERN_DATA_BANK_129;
    if ((token == ndsRelocFileID(&llGRBonus1YoshiMapFileID)) ||
        (token == NDS_RELOC_ASSET_GR_BONUS1_YOSHI_MAP))
    {
        return NDS_RELOC_ASSET_GR_BONUS1_YOSHI_MAP;
    }
    if (token == NDS_RELOC_ASSET_EXTERN_DATA_BANK_130) return NDS_RELOC_ASSET_EXTERN_DATA_BANK_130;
    if ((token == ndsRelocFileID(&llGRBonus1CaptainMapFileID)) ||
        (token == NDS_RELOC_ASSET_GR_BONUS1_CAPTAIN_MAP))
    {
        return NDS_RELOC_ASSET_GR_BONUS1_CAPTAIN_MAP;
    }
    if (token == NDS_RELOC_ASSET_EXTERN_DATA_BANK_131) return NDS_RELOC_ASSET_EXTERN_DATA_BANK_131;
    if ((token == ndsRelocFileID(&llGRBonus1KirbyMapFileID)) ||
        (token == NDS_RELOC_ASSET_GR_BONUS1_KIRBY_MAP))
    {
        return NDS_RELOC_ASSET_GR_BONUS1_KIRBY_MAP;
    }
    if (token == NDS_RELOC_ASSET_EXTERN_DATA_BANK_132) return NDS_RELOC_ASSET_EXTERN_DATA_BANK_132;
    if ((token == ndsRelocFileID(&llGRBonus1PikachuMapFileID)) ||
        (token == NDS_RELOC_ASSET_GR_BONUS1_PIKACHU_MAP))
    {
        return NDS_RELOC_ASSET_GR_BONUS1_PIKACHU_MAP;
    }
    if (token == NDS_RELOC_ASSET_EXTERN_DATA_BANK_133) return NDS_RELOC_ASSET_EXTERN_DATA_BANK_133;
    if ((token == ndsRelocFileID(&llGRBonus1PurinMapFileID)) ||
        (token == NDS_RELOC_ASSET_GR_BONUS1_PURIN_MAP))
    {
        return NDS_RELOC_ASSET_GR_BONUS1_PURIN_MAP;
    }
    if (token == NDS_RELOC_ASSET_EXTERN_DATA_BANK_134) return NDS_RELOC_ASSET_EXTERN_DATA_BANK_134;
    if ((token == ndsRelocFileID(&llGRBonus1NessMapFileID)) ||
        (token == NDS_RELOC_ASSET_GR_BONUS1_NESS_MAP))
    {
        return NDS_RELOC_ASSET_GR_BONUS1_NESS_MAP;
    }
    if (token == NDS_RELOC_ASSET_EXTERN_DATA_BANK_135) return NDS_RELOC_ASSET_EXTERN_DATA_BANK_135;
    if ((token == ndsRelocFileID(&llGRBonus2MarioMapFileID)) ||
        (token == NDS_RELOC_ASSET_GR_BONUS2_MARIO_MAP))
    {
        return NDS_RELOC_ASSET_GR_BONUS2_MARIO_MAP;
    }
    if (token == NDS_RELOC_ASSET_BONUS_DATA_BANK_137) return NDS_RELOC_ASSET_BONUS_DATA_BANK_137;
    if ((token == ndsRelocFileID(&llGRBonus2FoxMapFileID)) ||
        (token == NDS_RELOC_ASSET_GR_BONUS2_FOX_MAP))
    {
        return NDS_RELOC_ASSET_GR_BONUS2_FOX_MAP;
    }
    if (token == NDS_RELOC_ASSET_BONUS_DATA_BANK_138) return NDS_RELOC_ASSET_BONUS_DATA_BANK_138;
    if ((token == ndsRelocFileID(&llGRBonus2DonkeyMapFileID)) ||
        (token == NDS_RELOC_ASSET_GR_BONUS2_DONKEY_MAP))
    {
        return NDS_RELOC_ASSET_GR_BONUS2_DONKEY_MAP;
    }
    if (token == NDS_RELOC_ASSET_BONUS_DATA_BANK_139) return NDS_RELOC_ASSET_BONUS_DATA_BANK_139;
    if ((token == ndsRelocFileID(&llGRBonus2SamusMapFileID)) ||
        (token == NDS_RELOC_ASSET_GR_BONUS2_SAMUS_MAP))
    {
        return NDS_RELOC_ASSET_GR_BONUS2_SAMUS_MAP;
    }
    if (token == NDS_RELOC_ASSET_BONUS_DATA_BANK_140) return NDS_RELOC_ASSET_BONUS_DATA_BANK_140;
    if ((token == ndsRelocFileID(&llGRBonus2LuigiMapFileID)) ||
        (token == NDS_RELOC_ASSET_GR_BONUS2_LUIGI_MAP))
    {
        return NDS_RELOC_ASSET_GR_BONUS2_LUIGI_MAP;
    }
    if (token == NDS_RELOC_ASSET_BONUS_DATA_BANK_141) return NDS_RELOC_ASSET_BONUS_DATA_BANK_141;
    if ((token == ndsRelocFileID(&llGRBonus2LinkMapFileID)) ||
        (token == NDS_RELOC_ASSET_GR_BONUS2_LINK_MAP))
    {
        return NDS_RELOC_ASSET_GR_BONUS2_LINK_MAP;
    }
    if (token == NDS_RELOC_ASSET_BONUS_DATA_BANK_142) return NDS_RELOC_ASSET_BONUS_DATA_BANK_142;
    if ((token == ndsRelocFileID(&llGRBonus2YoshiMapFileID)) ||
        (token == NDS_RELOC_ASSET_GR_BONUS2_YOSHI_MAP))
    {
        return NDS_RELOC_ASSET_GR_BONUS2_YOSHI_MAP;
    }
    if (token == NDS_RELOC_ASSET_BONUS_DATA_BANK_143) return NDS_RELOC_ASSET_BONUS_DATA_BANK_143;
    if ((token == ndsRelocFileID(&llGRBonus2CaptainMapFileID)) ||
        (token == NDS_RELOC_ASSET_GR_BONUS2_CAPTAIN_MAP))
    {
        return NDS_RELOC_ASSET_GR_BONUS2_CAPTAIN_MAP;
    }
    if (token == NDS_RELOC_ASSET_BONUS_DATA_BANK_144) return NDS_RELOC_ASSET_BONUS_DATA_BANK_144;
    if ((token == ndsRelocFileID(&llGRBonus2KirbyMapFileID)) ||
        (token == NDS_RELOC_ASSET_GR_BONUS2_KIRBY_MAP))
    {
        return NDS_RELOC_ASSET_GR_BONUS2_KIRBY_MAP;
    }
    if (token == NDS_RELOC_ASSET_BONUS_DATA_BANK_145) return NDS_RELOC_ASSET_BONUS_DATA_BANK_145;
    if ((token == ndsRelocFileID(&llGRBonus2PikachuMapFileID)) ||
        (token == NDS_RELOC_ASSET_GR_BONUS2_PIKACHU_MAP))
    {
        return NDS_RELOC_ASSET_GR_BONUS2_PIKACHU_MAP;
    }
    if (token == NDS_RELOC_ASSET_BONUS_DATA_BANK_146) return NDS_RELOC_ASSET_BONUS_DATA_BANK_146;
    if ((token == ndsRelocFileID(&llGRBonus2PurinMapFileID)) ||
        (token == NDS_RELOC_ASSET_GR_BONUS2_PURIN_MAP))
    {
        return NDS_RELOC_ASSET_GR_BONUS2_PURIN_MAP;
    }
    if (token == NDS_RELOC_ASSET_BONUS_DATA_BANK_147) return NDS_RELOC_ASSET_BONUS_DATA_BANK_147;
    if ((token == ndsRelocFileID(&llGRBonus2NessMapFileID)) ||
        (token == NDS_RELOC_ASSET_GR_BONUS2_NESS_MAP))
    {
        return NDS_RELOC_ASSET_GR_BONUS2_NESS_MAP;
    }
    if (token == NDS_RELOC_ASSET_BONUS_DATA_BANK_148) return NDS_RELOC_ASSET_BONUS_DATA_BANK_148;
    if ((token == ndsRelocFileID(&llGRBonus3MapFileID)) ||
        (token == NDS_RELOC_ASSET_GR_BONUS3_MAP))
    {
        return NDS_RELOC_ASSET_GR_BONUS3_MAP;
    }
    if (token == NDS_RELOC_ASSET_BONUS_DATA_BANK_149) return NDS_RELOC_ASSET_BONUS_DATA_BANK_149;
    if (token == NDS_RELOC_ASSET_MISC_DATA_162) return NDS_RELOC_ASSET_MISC_DATA_162;
    if (token == NDS_RELOC_ASSET_IT_BONUS1_OBJECT) return NDS_RELOC_ASSET_IT_BONUS1_OBJECT;
    /* The source loads both of these by row address (sc1pbonusstage.c:430,
     * sc1pgame.c:2081), not only as an extern dependency by id; an id-only
     * row left the target attributes and Kirby Team's PK Fire file unloaded
     * (the load returns the caller's heap untouched). */
    if ((token == ndsRelocFileID(&llITBonus1ObjectHeaderFileID)) ||
        (token == NDS_RELOC_ASSET_IT_BONUS1_OBJECT_HEADER))
    {
        return NDS_RELOC_ASSET_IT_BONUS1_OBJECT_HEADER;
    }
    if ((token == ndsRelocFileID(&llKirbySpecial1FileID)) ||
        (token == NDS_RELOC_ASSET_KIRBY_SPECIAL1))
    {
        return NDS_RELOC_ASSET_KIRBY_SPECIAL1;
    }
#endif
    if (token == 0x58u) return NDS_RELOC_ASSET_STAGE_DREAM_LAND;
    if (token == 0x5fu) return NDS_RELOC_ASSET_STAGE_CASTLE;
    if (token == NDS_RELOC_ASSET_EXTERN_DATA_BANK_113) return NDS_RELOC_ASSET_EXTERN_DATA_BANK_113;
    if (token == NDS_RELOC_ASSET_EXTERN_DATA_BANK_103) return NDS_RELOC_ASSET_EXTERN_DATA_BANK_103;
    if (token == NDS_RELOC_ASSET_EXTERN_DATA_BANK_104) return NDS_RELOC_ASSET_EXTERN_DATA_BANK_104;
    if (token == NDS_RELOC_ASSET_MISC_DATA_BANK_152) return NDS_RELOC_ASSET_MISC_DATA_BANK_152;
    if (token == ndsRelocFileID(&llFTManagerCommonFileID)) return NDS_RELOC_ASSET_FT_MANAGER_COMMON;
    if (token == ndsRelocFileID(&llMarioMainMotionFileID)) return NDS_RELOC_ASSET_MARIO_MAIN_MOTION;
    if (token == ndsRelocFileID(&llMarioMainFileID)) return NDS_RELOC_ASSET_MARIO_MAIN;
    if (token == ndsRelocFileID(&llMarioSpecial1FileID)) return NDS_RELOC_ASSET_MARIO_SPECIAL1;
    if (token == ndsRelocFileID(&llMarioModelFileID)) return NDS_RELOC_ASSET_MARIO_MODEL;
    if (token == ndsRelocFileID(&llMarioSpecial3FileID)) return NDS_RELOC_ASSET_MARIO_SPECIAL3;
    if (token == ndsRelocFileID(&llMarioShieldPoseFileID)) return NDS_RELOC_ASSET_MARIO_SHIELD_POSE;
    if (token == ndsRelocFileID(&llMarioSpecial2FileID)) return NDS_RELOC_ASSET_MARIO_SPECIAL2;
    if (token == ndsRelocFileID(&llFoxSpecial3FileID)) return NDS_RELOC_ASSET_FOX_SPECIAL3;
    if (token == ndsRelocFileID(&llFoxMainMotionFileID)) return NDS_RELOC_ASSET_FOX_MAIN_MOTION;
    if (token == ndsRelocFileID(&llFoxMainFileID)) return NDS_RELOC_ASSET_FOX_MAIN;
    if (token == ndsRelocFileID(&llFoxSpecial1FileID)) return NDS_RELOC_ASSET_FOX_SPECIAL1;
    if (token == ndsRelocFileID(&llFoxModelFileID)) return NDS_RELOC_ASSET_FOX_MODEL;
    if (token == ndsRelocFileID(&llFoxShieldPoseFileID)) return NDS_RELOC_ASSET_FOX_SHIELD_POSE;
    if (token == ndsRelocFileID(&llFoxSpecial4FileID)) return NDS_RELOC_ASSET_FOX_SPECIAL4;
    if (token == ndsRelocFileID(&llFoxSpecial2FileID)) return NDS_RELOC_ASSET_FOX_SPECIAL2;
    if (token == ndsRelocFileID(&llEFCommonEffects1FileID)) return NDS_RELOC_ASSET_EF_COMMON_EFFECTS1;
    if (token == ndsRelocFileID(&llEFCommonEffects2FileID)) return NDS_RELOC_ASSET_EF_COMMON_EFFECTS2;
    if (token == ndsRelocFileID(&llEFCommonEffects3FileID)) return NDS_RELOC_ASSET_EF_COMMON_EFFECTS3;
    if (token == ndsRelocFileID(&llFTCommonMovesetFileID)) return NDS_RELOC_ASSET_MISC_DATA_201;
    if (token == NDS_RELOC_ASSET_FT_MANAGER_COMMON) return NDS_RELOC_ASSET_FT_MANAGER_COMMON;
    if (token == NDS_RELOC_ASSET_MARIO_MAIN_MOTION) return NDS_RELOC_ASSET_MARIO_MAIN_MOTION;
    if (token == NDS_RELOC_ASSET_MARIO_MAIN) return NDS_RELOC_ASSET_MARIO_MAIN;
    if (token == NDS_RELOC_ASSET_MARIO_SPECIAL1) return NDS_RELOC_ASSET_MARIO_SPECIAL1;
    if (token == NDS_RELOC_ASSET_MARIO_MODEL) return NDS_RELOC_ASSET_MARIO_MODEL;
    if (token == NDS_RELOC_ASSET_MARIO_SPECIAL3) return NDS_RELOC_ASSET_MARIO_SPECIAL3;
    if (token == NDS_RELOC_ASSET_MARIO_SHIELD_POSE) return NDS_RELOC_ASSET_MARIO_SHIELD_POSE;
    if (token == NDS_RELOC_ASSET_MARIO_SPECIAL2) return NDS_RELOC_ASSET_MARIO_SPECIAL2;
    if (token == NDS_RELOC_ASSET_FOX_SPECIAL3) return NDS_RELOC_ASSET_FOX_SPECIAL3;
    if (token == NDS_RELOC_ASSET_FOX_MAIN_MOTION) return NDS_RELOC_ASSET_FOX_MAIN_MOTION;
    if (token == NDS_RELOC_ASSET_FOX_MAIN) return NDS_RELOC_ASSET_FOX_MAIN;
    if (token == NDS_RELOC_ASSET_FOX_SPECIAL1) return NDS_RELOC_ASSET_FOX_SPECIAL1;
    if (token == NDS_RELOC_ASSET_FOX_MODEL) return NDS_RELOC_ASSET_FOX_MODEL;
    if (token == NDS_RELOC_ASSET_FOX_SHIELD_POSE) return NDS_RELOC_ASSET_FOX_SHIELD_POSE;
    if (token == NDS_RELOC_ASSET_FOX_SPECIAL4) return NDS_RELOC_ASSET_FOX_SPECIAL4;
    if (token == NDS_RELOC_ASSET_FOX_SPECIAL2) return NDS_RELOC_ASSET_FOX_SPECIAL2;
    if (token == NDS_RELOC_ASSET_EF_COMMON_EFFECTS1) return NDS_RELOC_ASSET_EF_COMMON_EFFECTS1;
    if (token == NDS_RELOC_ASSET_EF_COMMON_EFFECTS2) return NDS_RELOC_ASSET_EF_COMMON_EFFECTS2;
    if (token == NDS_RELOC_ASSET_EF_COMMON_EFFECTS3) return NDS_RELOC_ASSET_EF_COMMON_EFFECTS3;
    if (token == NDS_RELOC_ASSET_MISC_DATA_201) return NDS_RELOC_ASSET_MISC_DATA_201;
    if (token == NDS_RELOC_ASSET_MISC_DATA_299) return NDS_RELOC_ASSET_MISC_DATA_299;
    if (token == NDS_RELOC_ASSET_MISC_DATA_315) return NDS_RELOC_ASSET_MISC_DATA_315;
    if (token == NDS_RELOC_ASSET_EXTERN_DATA_BANK_109) return NDS_RELOC_ASSET_EXTERN_DATA_BANK_109;
    /* P2-3f48. Two token SHAPES, not one, and this is the trap: ndsRelocFileID
     * does not dereference -- the token IS the address of BattleShip's
     * &ll...FileID symbol, which is why the generated fighter rows above test
     * `token == ndsRelocFileID(&symbol_) || token == id_`. A numeric-only row
     * here matched nothing, lbRelocGetFileSize fell through to its
     * sizeof(Sprite) fallback, and the item file was allocated 68 bytes.
     *
     * The item subsystem passes the symbol address (decomp it/itmanager.c:148);
     * the external fixup pass resolves ITCommonData's 68 pointers by the
     * numeric dependency id instead. Both arms are needed.
     *
     * MiscData086 is listed unconditionally even though Yoshi's generated
     * dependency rows above already claim it -- those rows only exist when his
     * flag is on, and the item file needs it either way. Whichever arm answers
     * first returns the same id. */
    if ((token == ndsRelocFileID(&llITCommonDataFileID)) ||
        (token == NDS_RELOC_ASSET_IT_COMMON_DATA))
    {
        return NDS_RELOC_ASSET_IT_COMMON_DATA;
    }
    if (token == NDS_RELOC_ASSET_MISC_DATA_086) return NDS_RELOC_ASSET_MISC_DATA_086;
    {
        u32 mario_anim_asset_id =
            ndsRelocMarioBattleAnimAssetIDForToken(token);

        if (mario_anim_asset_id != NDS_RELOC_ASSET_INVALID)
        {
            return mario_anim_asset_id;
        }
    }
    {
        u32 fox_anim_asset_id = ndsRelocFoxAnimAssetIDForToken(token);

        if (fox_anim_asset_id != NDS_RELOC_ASSET_INVALID)
        {
            return fox_anim_asset_id;
        }
    }
    /* VS Results demo poses. Kept beside the two battle-animation resolvers
     * because it answers the same `&ll...FileID` token ABI; its ids are the
     * 357..498 `reloc_submotions/` space, disjoint from both banks above and
     * from every generated animation id, so no arm can shadow another. */
    {
        u32 demo_anim_asset_id = ndsRelocDemoAnimAssetIDForToken(token);

        if (demo_anim_asset_id != NDS_RELOC_ASSET_INVALID)
        {
            return demo_anim_asset_id;
        }
    }
    if (token == NDS_RELOC_ASSET_MARIO_ANIM_WAIT) return NDS_RELOC_ASSET_MARIO_ANIM_WAIT;
    if (token == NDS_RELOC_ASSET_MARIO_ANIM_WALK1) return NDS_RELOC_ASSET_MARIO_ANIM_WALK1;
    if (token == NDS_RELOC_ASSET_MARIO_ANIM_WALK2) return NDS_RELOC_ASSET_MARIO_ANIM_WALK2;
    if (token == NDS_RELOC_ASSET_MARIO_ANIM_WALK3) return NDS_RELOC_ASSET_MARIO_ANIM_WALK3;
    if (token == NDS_RELOC_ASSET_MARIO_ANIM_WALK_END) return NDS_RELOC_ASSET_MARIO_ANIM_WALK_END;
    if (ndsRelocIsMarioFoxAnimID(token) != FALSE) return token;
    /* P4 descriptors carry plain numeric donor IDs (include/nds/nds_p4.h). */
    if ((token >= 0x854u) && (token < 0x10000u) &&
        (ndsP4RelocAssetPath(token) != NULL))
    {
        return token;
    }
    return NDS_RELOC_ASSET_INVALID;
}

/* The chain above is a pure function of the token (link-time symbol
 * addresses and constants) and walks several hundred compares before it
 * answers a late row. Effect makers ask it on every effect they create
 * (ndsRelocGetLoadedFileSize under the mapped-desc resolve): in four-Pikachu
 * Thunder frames it was ~27K cycles of the heavy frames' excess. A direct-
 * mapped memo answers a repeat token at once; a miss only refills its line.
 * Keys hold token + 1 so the zeroed table starts empty. */
#define NDS_RELOC_TOKEN_MEMO_LINES 64u
static u32 sNdsRelocTokenMemoKey[NDS_RELOC_TOKEN_MEMO_LINES];
static u32 sNdsRelocTokenMemoAsset[NDS_RELOC_TOKEN_MEMO_LINES];

static u32 ndsRelocAssetIDForToken(u32 token)
{
    u32 line = ((token * 2654435761u) >> 26) &
        (NDS_RELOC_TOKEN_MEMO_LINES - 1u);
    u32 asset_id;

    if (sNdsRelocTokenMemoKey[line] == token + 1u)
    {
        return sNdsRelocTokenMemoAsset[line];
    }
    asset_id = ndsRelocAssetIDForTokenChain(token);
    sNdsRelocTokenMemoKey[line] = token + 1u;
    sNdsRelocTokenMemoAsset[line] = asset_id;
    return asset_id;
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

extern void ndsAObjEvent32ResetNormalizedScripts(void);
extern void ndsAObjEvent32ForgetRange(const void *base, size_t size);
extern sb32 ndsAObjEvent32ConfigureNormalizedCapacity(u32 gkind);

/* Task 44 item 3: the Dream Land stage-asset mutation generation.
 *
 * The complete-stage owner used to re-look-up its four assets and rebuild the
 * 57-DObj topology stamp on every preparation just to learn nothing had moved.
 * Steady-state admission now compares this counter instead, so EVERY path that
 * can replace, unload, or invalidate one of those four files must bump it.
 * The complete enumeration is exactly three seams, all of them below:
 *
 *   1. ndsRelocResetLoadedFiles  - every unload/reset path funnels here
 *      (scene-cache eviction, title backend teardown).
 *   2. ndsRelocRegisterLoadedFile - the only writer of NDSRelocLoadedFile::data;
 *      covers both a first load and a same-asset-id replacement in place.
 *      Bumped only for the four Dream Land ids, so fighter/animation loads do
 *      not force stage revalidation.
 *   3. ndsRelocPrepareSceneCache - bumps sNdsRelocSceneGeneration; covered
 *      explicitly because that path skips the reset when nothing was resident.
 *
 * It starts at 1 so a zero in the owner workspace always means "never
 * admitted" and takes the full validation path.
 */
static u32 sNdsRelocStageAssetMutation = 1u;

static s32 ndsRelocIsDreamLandStageAsset(u32 asset_id)
{
    return ((asset_id == NDS_RELOC_ASSET_EXTERN_DATA_BANK_103) ||
            (asset_id == NDS_RELOC_ASSET_EXTERN_DATA_BANK_104) ||
            (asset_id == NDS_RELOC_ASSET_MISC_DATA_BANK_152) ||
            (asset_id == NDS_RELOC_ASSET_GR_PUPUPU_MAP)) ? TRUE : FALSE;
}

static void ndsRelocBumpStageAssetMutation(void)
{
    sNdsRelocStageAssetMutation++;
    if (sNdsRelocStageAssetMutation == 0u)
    {
        sNdsRelocStageAssetMutation = 1u;
    }
}

static void ndsRelocResetLoadedFiles(void)
{
    ndsAObjEvent32ResetNormalizedScripts();
    ndsRelocBumpStageAssetMutation();
    memset(sNdsRelocLoadedFiles, 0, sizeof(sNdsRelocLoadedFiles));
    sNdsRelocLoadedFileCount = 0;
    sNdsRelocLoadedFilesEpoch++;
    memset(sNdsRelocNormalizedMObjSubs, 0,
           sizeof(sNdsRelocNormalizedMObjSubs));
    sNdsRelocNormalizedMObjSubCount = 0u;
    memset(sNdsRelocNormalizedWeaponAttrs, 0,
           sizeof(sNdsRelocNormalizedWeaponAttrs));
    sNdsRelocNormalizedWeaponAttrCount = 0u;
    ndsFighterMarioFoxResetFileSlots();
}

/* The gkind this scene's event32 ledger is sized for (see the call below). */
static u32 ndsRelocEvent32CapacityGKind(void)
{
    u32 gkind;

    if (gSCManagerBattleState == NULL)
    {
        return 0xffffffffu;
    }
    gkind = (u32)gSCManagerBattleState->gkind;
    if (gSCManagerSceneData.scene_curr == nSCKindVSBattle)
    {
        return gkind;
    }
    if ((gSCManagerSceneData.scene_curr == nSCKind1PBonusStage) &&
        (ndsGrBonusClass((s32)gkind) != 0u))
    {
        return gkind;
    }
    return 0xffffffffu;
}

NDS_COLD static void ndsRelocPrepareSceneCache(void)
{
    u32 scene = (u32)gSCManagerSceneData.scene_curr;
    u32 evicted_files = 0u;
    u32 evicted_bytes = 0u;
    u32 i;

    /* Scene KIND is not scene INSTANCE, and this guard used to conflate them.
     *
     * Everything below is the port's only invalidation for a scene change: it
     * evicts the resident reloc files, discards the battle texture keys that
     * hold native pointers INTO those files, and bumps sNdsRelocSceneGeneration
     * -- which becomes each asset's owner_generation, which is where
     * topology_generation comes from, which is the only thing that resets the
     * Task 36 replay. Skipping it leaves every generation-keyed cache believing
     * data is live after the taskman heap has been rewound under it.
     *
     * Skipping on an equal kind is right for a repeated call WITHIN a scene and
     * wrong for a RE-ENTRY into the same kind. Sudden Death is exactly that --
     * nSCKindVSBattle to nSCKindVSBattle -- so it evicted nothing, discarded no
     * texture keys, and never advanced the generation. The owner's screenshot of
     * that state is duplicated fighter sprites at wrong positions over corrupt
     * stage geometry, drawn at a healthy 28.9 FPS with FTR 385,728: not a cost
     * defect, stale bindings replayed against reused memory.
     *
     * The heap generation below is therefore the ownership test.  Do NOT add a
     * pointer-vs-cursor fallback here: this reloc table also tracks live files in
     * fighter-manager intern buffers, caller-provided heaps and static stores.
     * During Mario/Mario construction the old range heuristic classified one of
     * those legal residents as stale while an external fixup was recursively
     * loading a dependency, memset the outer NDSRelocLoadedFile slot, and the
     * resumed fixup wrote through loaded->data == NULL. */
    if (sNdsRelocOwnerScene == scene)
    {
        /* THE HEAP GENERATION IS THE AUTHORITY, not the cursor.
         *
         * The cursor test was this guard's original re-entry fix, and it
         * is unsound for the same reason it was unsound in the animation cache:
         * a rewind does not move our data, it moves the CURSOR, and the new
         * scene's own allocations push that cursor straight back past the stale
         * files. Every one of them is then inside [start, cursor) again, the
         * scan could find nothing, and this function would early-return having evicted
         * nothing, discarded no texture keys or OAM names, and never advanced
         * sNdsRelocSceneGeneration. Whether it fires is a race between when
         * this runs and how much the new scene has allocated -- which is why
         * the symptom is intermittent and why it presents as wrong textures on
         * a second entry rather than as a crash.
         *
         * gNdsTaskmanHeapGeneration cannot be raced: it is bumped at the two
         * primitives that rewind the heap, before any new-scene allocation.  It
         * also does not inspect unrelated caller-owned address ranges, which is
         * what makes recursive dependency loading safe. */
        if (sNdsRelocResidentHeapGeneration == gNdsTaskmanHeapGeneration)
        {
            return;
        }
        gNdsRelocSceneReentryGenerationEvictCount++;
        gNdsRelocSceneReentryEvictCount++;
    }

    for (i = 0; i < sNdsRelocLoadedFileCount; i++)
    {
        evicted_files++;
        evicted_bytes += sNdsRelocLoadedFiles[i].data_size;
    }

    if (evicted_files != 0u)
    {
        /* Texture keys hold native pointers into the scene-owned reloc files.
         * Drop every battle entry before those files become stale. */
        ndsRendererHardwareDiscardBattleStaticTextures();
        /* And the native OAM path's retained texture NAMES, which point at the
         * VRAM the line above just released. They were cleared only by
         * ndsIFCommonNativeOamInit, which runs once at boot, so after a scene
         * reload ndsIFCommonNativeOamPrepareClouds saw them non-zero and
         * early-returned TRUE without re-uploading -- leaving the sprite path
         * emitting handles into reused VRAM. Same shape as the
         * sNdsRendererBattleStaticTexturePrepared latch, which already had its
         * invalidation wired; this one did not. */
        ndsIFCommonNativeOamDiscardTextures();
        ndsRelocResetLoadedFiles();
    }

    /* The event32 normalizer's large command ledger used to reserve Planet
     * Zebes' worst case in static BSS for every stage. Allocate it from this
     * scene's taskman heap instead, after any old ledger/files were discarded.
     * Only VS battle has a per-stage corpus proven by the nine-stage census;
     * every other scene keeps the 5120 ceiling rather than borrowing a stale
     * battle gkind from gSCManagerBattleState, except a bonus board, whose
     * battle state sc1PBonusStageInitVars (sc1pbonusstage.c:357-407) has
     * already pointed at this scene's board before sc1PBonusStageSetupFiles
     * makes the first load; only a bonus gkind is taken from it. */
    (void)ndsAObjEvent32ConfigureNormalizedCapacity(
        ndsRelocEvent32CapacityGKind());
    sNdsRelocStatusBufferCount = 0;
    sNdsRelocForceStatusBufferCount = 0;

    sNdsRelocOwnerScene = scene;
    /* Stamp the heap generation this resident set belongs to. Read here, after
     * the eviction above, because this is the point the set becomes current. */
    sNdsRelocResidentHeapGeneration = gNdsTaskmanHeapGeneration;
    sNdsRelocSceneGeneration++;
    /* Seam 3: a scene change with nothing resident skips the reset above, so
     * the stage owner would otherwise never learn its generation moved. */
    ndsRelocBumpStageAssetMutation();
    /* The adapter's frame-local matrix storage lives in this scene's original
     * taskman heap. Drop its pointer before that heap can be reused. */
    ndsRendererAdapterResetSceneCaches();
    gNdsMemoryLedgerEvictedFiles = evicted_files;
    gNdsMemoryLedgerEvictedBytes = evicted_bytes;
}

static s32 ndsRelocAssetIsInterface(u32 asset_id)
{
    switch (asset_id)
    {
    case NDS_RELOC_ASSET_IF_COMMON_ANNOUNCE:
    case NDS_RELOC_ASSET_IF_COMMON_PLAYER:
    case NDS_RELOC_ASSET_IF_COMMON_GAME_STATUS:
    case NDS_RELOC_ASSET_IF_COMMON_PLAYER_DAMAGE:
    case NDS_RELOC_ASSET_IF_COMMON_TIMER:
    case NDS_RELOC_ASSET_IF_COMMON_DIGITS:
    case NDS_RELOC_ASSET_IF_COMMON_BATTLE_PAUSE:
    case NDS_RELOC_ASSET_IF_COMMON_PLAYER_TAGS:
    case NDS_RELOC_ASSET_SY_KSEG1_VALIDATE:
        return TRUE;
    default:
        return FALSE;
    }
}

static s32 ndsRelocAssetIsStage(u32 asset_id)
{
    switch (asset_id)
    {
    case NDS_RELOC_ASSET_STAGE_CASTLE:
    case NDS_RELOC_ASSET_STAGE_DREAM_LAND:
    case NDS_RELOC_ASSET_EXTERN_DATA_BANK_113:
    case NDS_RELOC_ASSET_EXTERN_DATA_BANK_103:
    case NDS_RELOC_ASSET_EXTERN_DATA_BANK_104:
    case NDS_RELOC_ASSET_MISC_DATA_BANK_152:
    case NDS_RELOC_ASSET_GR_PUPUPU_MAP:
    case NDS_RELOC_ASSET_GR_INISHIE_MAP:
    case NDS_RELOC_ASSET_GR_HYRULE_MAP:
#if NDS_P2_1P_GAME
    case NDS_RELOC_ASSET_GR_LAST_MAP:
    case NDS_RELOC_ASSET_EXTERN_DATA_BANK_114:
    case NDS_RELOC_ASSET_STAGE_LAST_WALLPAPER:
    case NDS_RELOC_ASSET_GR_METAL_MAP:
    case NDS_RELOC_ASSET_EXTERN_DATA_BANK_117:
    case NDS_RELOC_ASSET_STAGE_METAL_WALLPAPER:
    case NDS_RELOC_ASSET_GR_YOSTER_SMALL_MAP:
    case NDS_RELOC_ASSET_EXTERN_DATA_BANK_118:
    case NDS_RELOC_ASSET_GR_PUPUPU_SMALL_MAP:
    case NDS_RELOC_ASSET_EXTERN_DATA_BANK_101:
    case NDS_RELOC_ASSET_EXTERN_DATA_BANK_100:
    case NDS_RELOC_ASSET_GR_ZAKO_MAP:
    case NDS_RELOC_ASSET_EXTERN_DATA_BANK_116:
    case NDS_RELOC_ASSET_STAGE_ZAKO_WALLPAPER:
    case NDS_RELOC_ASSET_STAGE_BONUS_WALLPAPER:
    case NDS_RELOC_ASSET_EXTERN_DATA_BANK_120:
    case NDS_RELOC_ASSET_EXTERN_DATA_BANK_121:
    case NDS_RELOC_ASSET_EXTERN_DATA_BANK_122:
    case NDS_RELOC_ASSET_EXTERN_DATA_BANK_123:
    case NDS_RELOC_ASSET_GR_BONUS1_MARIO_MAP:
    case NDS_RELOC_ASSET_EXTERN_DATA_BANK_124:
    case NDS_RELOC_ASSET_GR_BONUS1_FOX_MAP:
    case NDS_RELOC_ASSET_EXTERN_DATA_BANK_125:
    case NDS_RELOC_ASSET_GR_BONUS1_DONKEY_MAP:
    case NDS_RELOC_ASSET_EXTERN_DATA_BANK_126:
    case NDS_RELOC_ASSET_GR_BONUS1_SAMUS_MAP:
    case NDS_RELOC_ASSET_EXTERN_DATA_BANK_127:
    case NDS_RELOC_ASSET_GR_BONUS1_LUIGI_MAP:
    case NDS_RELOC_ASSET_EXTERN_DATA_BANK_128:
    case NDS_RELOC_ASSET_GR_BONUS1_LINK_MAP:
    case NDS_RELOC_ASSET_EXTERN_DATA_BANK_129:
    case NDS_RELOC_ASSET_GR_BONUS1_YOSHI_MAP:
    case NDS_RELOC_ASSET_EXTERN_DATA_BANK_130:
    case NDS_RELOC_ASSET_GR_BONUS1_CAPTAIN_MAP:
    case NDS_RELOC_ASSET_EXTERN_DATA_BANK_131:
    case NDS_RELOC_ASSET_GR_BONUS1_KIRBY_MAP:
    case NDS_RELOC_ASSET_EXTERN_DATA_BANK_132:
    case NDS_RELOC_ASSET_GR_BONUS1_PIKACHU_MAP:
    case NDS_RELOC_ASSET_EXTERN_DATA_BANK_133:
    case NDS_RELOC_ASSET_GR_BONUS1_PURIN_MAP:
    case NDS_RELOC_ASSET_EXTERN_DATA_BANK_134:
    case NDS_RELOC_ASSET_GR_BONUS1_NESS_MAP:
    case NDS_RELOC_ASSET_EXTERN_DATA_BANK_135:
    case NDS_RELOC_ASSET_GR_BONUS2_MARIO_MAP:
    case NDS_RELOC_ASSET_BONUS_DATA_BANK_137:
    case NDS_RELOC_ASSET_GR_BONUS2_FOX_MAP:
    case NDS_RELOC_ASSET_BONUS_DATA_BANK_138:
    case NDS_RELOC_ASSET_GR_BONUS2_DONKEY_MAP:
    case NDS_RELOC_ASSET_BONUS_DATA_BANK_139:
    case NDS_RELOC_ASSET_GR_BONUS2_SAMUS_MAP:
    case NDS_RELOC_ASSET_BONUS_DATA_BANK_140:
    case NDS_RELOC_ASSET_GR_BONUS2_LUIGI_MAP:
    case NDS_RELOC_ASSET_BONUS_DATA_BANK_141:
    case NDS_RELOC_ASSET_GR_BONUS2_LINK_MAP:
    case NDS_RELOC_ASSET_BONUS_DATA_BANK_142:
    case NDS_RELOC_ASSET_GR_BONUS2_YOSHI_MAP:
    case NDS_RELOC_ASSET_BONUS_DATA_BANK_143:
    case NDS_RELOC_ASSET_GR_BONUS2_CAPTAIN_MAP:
    case NDS_RELOC_ASSET_BONUS_DATA_BANK_144:
    case NDS_RELOC_ASSET_GR_BONUS2_KIRBY_MAP:
    case NDS_RELOC_ASSET_BONUS_DATA_BANK_145:
    case NDS_RELOC_ASSET_GR_BONUS2_PIKACHU_MAP:
    case NDS_RELOC_ASSET_BONUS_DATA_BANK_146:
    case NDS_RELOC_ASSET_GR_BONUS2_PURIN_MAP:
    case NDS_RELOC_ASSET_BONUS_DATA_BANK_147:
    case NDS_RELOC_ASSET_GR_BONUS2_NESS_MAP:
    case NDS_RELOC_ASSET_BONUS_DATA_BANK_148:
    case NDS_RELOC_ASSET_GR_BONUS3_MAP:
    case NDS_RELOC_ASSET_BONUS_DATA_BANK_149:
    case NDS_RELOC_ASSET_MISC_DATA_162:
    case NDS_RELOC_ASSET_IT_BONUS1_OBJECT:
    case NDS_RELOC_ASSET_IT_BONUS1_OBJECT_HEADER:
    case NDS_RELOC_ASSET_KIRBY_SPECIAL1:
#endif
        return TRUE;
    default:
        return FALSE;
    }
}

static s32 ndsRelocAssetIsFighter(u32 asset_id)
{
#define NDS_P2_FIGHTER_ASSET_TEST(symbol_, id_, path_) \
    if (asset_id == (id_)) return TRUE;
#define NDS_P2_FIGHTER_DEPENDENCY_TEST(id_, path_) \
    if (asset_id == (id_)) return TRUE;
#if NDS_P2_LUIGI
    NDS_P2_LUIGI_CORE_ASSET_ROWS(NDS_P2_FIGHTER_ASSET_TEST)
    NDS_P2_LUIGI_DEPENDENCY_ASSET_ROWS(NDS_P2_FIGHTER_DEPENDENCY_TEST)
#endif
#if NDS_P2_DONKEY
    NDS_P2_DONKEY_CORE_ASSET_ROWS(NDS_P2_FIGHTER_ASSET_TEST)
    NDS_P2_DONKEY_DEPENDENCY_ASSET_ROWS(NDS_P2_FIGHTER_DEPENDENCY_TEST)
#endif
#if NDS_P2_CAPTAIN
    NDS_P2_CAPTAIN_CORE_ASSET_ROWS(NDS_P2_FIGHTER_ASSET_TEST)
    NDS_P2_CAPTAIN_DEPENDENCY_ASSET_ROWS(NDS_P2_FIGHTER_DEPENDENCY_TEST)
#endif
    NDS_P2_SAMUS_CORE_ASSET_ROWS(NDS_P2_FIGHTER_ASSET_TEST)
    NDS_P2_SAMUS_DEPENDENCY_ASSET_ROWS(NDS_P2_FIGHTER_DEPENDENCY_TEST)
#if NDS_P2_LINK
    NDS_P2_LINK_CORE_ASSET_ROWS(NDS_P2_FIGHTER_ASSET_TEST)
    NDS_P2_LINK_DEPENDENCY_ASSET_ROWS(NDS_P2_FIGHTER_DEPENDENCY_TEST)
#endif
    NDS_P2_PIKACHU_CORE_ASSET_ROWS(NDS_P2_FIGHTER_ASSET_TEST)
    NDS_P2_PIKACHU_DEPENDENCY_ASSET_ROWS(NDS_P2_FIGHTER_DEPENDENCY_TEST)
    NDS_P2_YOSHI_CORE_ASSET_ROWS(NDS_P2_FIGHTER_ASSET_TEST)
    NDS_P2_YOSHI_DEPENDENCY_ASSET_ROWS(NDS_P2_FIGHTER_DEPENDENCY_TEST)
    NDS_P2_NESS_CORE_ASSET_ROWS(NDS_P2_FIGHTER_ASSET_TEST)
    NDS_P2_NESS_DEPENDENCY_ASSET_ROWS(NDS_P2_FIGHTER_DEPENDENCY_TEST)
    NDS_P2_PURIN_CORE_ASSET_ROWS(NDS_P2_FIGHTER_ASSET_TEST)
    NDS_P2_PURIN_DEPENDENCY_ASSET_ROWS(NDS_P2_FIGHTER_DEPENDENCY_TEST)
    NDS_P2_KIRBY_CORE_ASSET_ROWS(NDS_P2_FIGHTER_ASSET_TEST)
    NDS_P2_KIRBY_DEPENDENCY_ASSET_ROWS(NDS_P2_FIGHTER_DEPENDENCY_TEST)
#undef NDS_P2_FIGHTER_DEPENDENCY_TEST
#undef NDS_P2_FIGHTER_ASSET_TEST
    switch (asset_id)
    {
    case NDS_RELOC_ASSET_FT_MANAGER_COMMON:
    case NDS_RELOC_ASSET_MARIO_MAIN:
    case NDS_RELOC_ASSET_MARIO_MAIN_MOTION:
    case NDS_RELOC_ASSET_MARIO_MODEL:
    case NDS_RELOC_ASSET_MARIO_SHIELD_POSE:
    case NDS_RELOC_ASSET_MARIO_SPECIAL1:
    case NDS_RELOC_ASSET_MARIO_SPECIAL2:
    case NDS_RELOC_ASSET_MARIO_SPECIAL3:
    case NDS_RELOC_ASSET_FOX_MAIN:
    case NDS_RELOC_ASSET_FOX_MAIN_MOTION:
    case NDS_RELOC_ASSET_FOX_MODEL:
    case NDS_RELOC_ASSET_FOX_SHIELD_POSE:
    case NDS_RELOC_ASSET_FOX_SPECIAL1:
    case NDS_RELOC_ASSET_FOX_SPECIAL2:
    case NDS_RELOC_ASSET_FOX_SPECIAL3:
    case NDS_RELOC_ASSET_FOX_SPECIAL4:
    case NDS_RELOC_ASSET_MISC_DATA_201:
    case NDS_RELOC_ASSET_MISC_DATA_299:
    case NDS_RELOC_ASSET_MISC_DATA_315:
    case NDS_RELOC_ASSET_EXTERN_DATA_BANK_109:
        return TRUE;
    default:
        break;
    }
    return ndsRelocIsFighterAnimID(asset_id);
}

static s32 ndsRelocAssetIsMenu(u32 asset_id)
{
    switch (asset_id)
    {
    case NDS_RELOC_ASSET_SC1P_TRAINING_SPRITES:
    case NDS_RELOC_ASSET_NNESS_MODEL:
    case NDS_RELOC_ASSET_NNESS_MAIN:
    case NDS_RELOC_ASSET_NPURIN_MODEL:
    case NDS_RELOC_ASSET_NPURIN_MAIN:
    case NDS_RELOC_ASSET_NPIKACHU_MODEL:
    case NDS_RELOC_ASSET_NPIKACHU_MAIN:
    case NDS_RELOC_ASSET_NKIRBY_MODEL:
    case NDS_RELOC_ASSET_NKIRBY_MAIN:
    case NDS_RELOC_ASSET_NCAPTAIN_MODEL:
    case NDS_RELOC_ASSET_NCAPTAIN_MAIN:
    case NDS_RELOC_ASSET_NYOSHI_MODEL:
    case NDS_RELOC_ASSET_NYOSHI_MAIN:
    case NDS_RELOC_ASSET_NLINK_MODEL:
    case NDS_RELOC_ASSET_NLINK_MAIN:
    case NDS_RELOC_ASSET_NLUIGI_MAIN:
    case NDS_RELOC_ASSET_NSAMUS_MODEL:
    case NDS_RELOC_ASSET_NSAMUS_MAIN:
    case NDS_RELOC_ASSET_NDONKEY_MODEL:
    case NDS_RELOC_ASSET_NDONKEY_MAIN:
    case NDS_RELOC_ASSET_NFOX_MODEL:
    case NDS_RELOC_ASSET_NFOX_MAIN:
    case NDS_RELOC_ASSET_NMARIO_MODEL:
    case NDS_RELOC_ASSET_NMARIO_MAIN:
#if NDS_P2_GDONKEY
    case NDS_RELOC_ASSET_GDONKEY_MAIN:
#endif
#if NDS_P2_MMARIO
    case NDS_RELOC_ASSET_MMARIO_MAIN:
    case NDS_RELOC_ASSET_MMARIO_MAIN_MOTION:
    case NDS_RELOC_ASSET_MMARIO_MODEL:
#endif
    case NDS_RELOC_ASSET_BOSS_MODEL:
    case NDS_RELOC_ASSET_BOSS_MAIN_MOTION:
    case NDS_RELOC_ASSET_BOSS_MAIN:
    case NDS_RELOC_ASSET_NCOMMON_TEXTURE:
    case NDS_RELOC_ASSET_MASTER_HAND_ICON:
    case NDS_RELOC_ASSET_MN_SCREEN_ADJUST:
    case NDS_RELOC_ASSET_BONUS2_COMMON:
    case NDS_RELOC_ASSET_BONUS_PICTURE_PLATFORM:
    case NDS_RELOC_ASSET_BONUS_PICTURE:
    case NDS_RELOC_ASSET_CHARACTER_NAMES:
    case NDS_RELOC_ASSET_MN_PLAYERS_DIFFICULTY:
    case NDS_RELOC_ASSET_SC_EXPLAIN_MAIN:
    case NDS_RELOC_ASSET_SC_EXPLAIN_GRAPHICS:
    case NDS_RELOC_ASSET_GR_WALLPAPER_TRAINING_YELLOW:
    case NDS_RELOC_ASSET_GR_WALLPAPER_TRAINING_BLUE:
    case NDS_RELOC_ASSET_SC1P_TRAINING_MODE:
    case NDS_RELOC_ASSET_MN_SOUND_TEST:
    case NDS_RELOC_ASSET_MN_BACKUP_CLEAR_HEADER_OPTION:
    case NDS_RELOC_ASSET_MN_BACKUP_CLEAR:
    case NDS_RELOC_ASSET_MN_OPTION:
    case NDS_RELOC_ASSET_MN_CHARACTERS:
    case NDS_RELOC_ASSET_MNVS_RECORD_MAIN:
    case NDS_RELOC_ASSET_MN_DATA_COMMON:
    case NDS_RELOC_ASSET_MN_DATA:
    case NDS_RELOC_ASSET_MN_CONGRA_YOSHI_TOP:
    case NDS_RELOC_ASSET_MN_CONGRA_YOSHI_BOTTOM:
    case NDS_RELOC_ASSET_MN_CONGRA_SAMUS_TOP:
    case NDS_RELOC_ASSET_MN_CONGRA_SAMUS_BOTTOM:
    case NDS_RELOC_ASSET_MN_CONGRA_PURIN_TOP:
    case NDS_RELOC_ASSET_MN_CONGRA_PURIN_BOTTOM:
    case NDS_RELOC_ASSET_MN_CONGRA_PIKACHU_TOP:
    case NDS_RELOC_ASSET_MN_CONGRA_PIKACHU_BOTTOM:
    case NDS_RELOC_ASSET_MN_CONGRA_NESS_TOP:
    case NDS_RELOC_ASSET_MN_CONGRA_NESS_BOTTOM:
    case NDS_RELOC_ASSET_MN_CONGRA_MARIO_TOP:
    case NDS_RELOC_ASSET_MN_CONGRA_MARIO_BOTTOM:
    case NDS_RELOC_ASSET_MN_CONGRA_LUIGI_TOP:
    case NDS_RELOC_ASSET_MN_CONGRA_LUIGI_BOTTOM:
    case NDS_RELOC_ASSET_MN_CONGRA_LINK_TOP:
    case NDS_RELOC_ASSET_MN_CONGRA_LINK_BOTTOM:
    case NDS_RELOC_ASSET_MN_CONGRA_KIRBY_TOP:
    case NDS_RELOC_ASSET_MN_CONGRA_KIRBY_BOTTOM:
    case NDS_RELOC_ASSET_MN_CONGRA_FOX_TOP:
    case NDS_RELOC_ASSET_MN_CONGRA_FOX_BOTTOM:
    case NDS_RELOC_ASSET_MN_CONGRA_DONKEY_TOP:
    case NDS_RELOC_ASSET_MN_CONGRA_DONKEY_BOTTOM:
    case NDS_RELOC_ASSET_MN_CONGRA_CAPTAIN_TOP:
    case NDS_RELOC_ASSET_MN_CONGRA_CAPTAIN_BOTTOM:
    case NDS_RELOC_ASSET_MV_ENDING:
    case NDS_RELOC_ASSET_SC_STAFFROLL:
    case NDS_RELOC_ASSET_MN_MESSAGE:
    case NDS_RELOC_ASSET_MN_PLAYERS1P_MODE:
    case NDS_RELOC_ASSET_MN1P_CONTINUE:
    case NDS_RELOC_ASSET_MN1P:
    case NDS_RELOC_ASSET_SC1P_INTRO:
    case NDS_RELOC_ASSET_SC1P_CHALLENGER:
    case NDS_RELOC_ASSET_SC1P_STAGE_CLEAR3:
    case NDS_RELOC_ASSET_SC1P_STAGE_CLEAR2:
    case NDS_RELOC_ASSET_SC1P_STAGE_CLEAR1:
    case NDS_RELOC_ASSET_MN_COMMON:
    case NDS_RELOC_ASSET_MN_TITLE:
    case NDS_RELOC_ASSET_MN_TITLE_FIRE_ANIM:
    case NDS_RELOC_ASSET_MN_VS_MODE:
    case NDS_RELOC_ASSET_MN_PLAYERS_COMMON:
    case NDS_RELOC_ASSET_MN_PLAYERS_GAME_MODES:
    case NDS_RELOC_ASSET_MN_PLAYERS_PORTRAITS:
    case NDS_RELOC_ASSET_FT_EMBLEM_SPRITES:
    case NDS_RELOC_ASSET_MN_SELECT_COMMON:
    case NDS_RELOC_ASSET_MN_PLAYERS_SPOTLIGHT:
    case NDS_RELOC_ASSET_GR_WALLPAPER_TRAINING_BLACK:
    case NDS_RELOC_ASSET_MN_MAPS:
    case NDS_RELOC_ASSET_MN_COMMON_FONTS:
        return TRUE;
    default:
        return FALSE;
    }
}

static s32 ndsRelocAssetIsOpening(u32 asset_id)
{
    switch (asset_id)
    {
    case NDS_RELOC_ASSET_N64_LOGO:
    case NDS_RELOC_ASSET_MV_COMMON:
    case NDS_RELOC_ASSET_OPENING_COMMON:
    case NDS_RELOC_ASSET_OPENING_ROOM_TRANSITION:
    case NDS_RELOC_ASSET_OPENING_ROOM_SCENE1:
    case NDS_RELOC_ASSET_OPENING_ROOM_SCENE2:
    case NDS_RELOC_ASSET_OPENING_ROOM_SCENE3:
    case NDS_RELOC_ASSET_OPENING_ROOM_SCENE4:
    case NDS_RELOC_ASSET_OPENING_RUN:
    case NDS_RELOC_ASSET_OPENING_YAMABUKI:
    case NDS_RELOC_ASSET_OPENING_SECTOR:
    case NDS_RELOC_ASSET_OPENING_RUN_CRASH:
    case NDS_RELOC_ASSET_OPENING_ROOM_WALLPAPER:
    case NDS_RELOC_ASSET_OPENING_PORTRAITS_SET1:
    case NDS_RELOC_ASSET_OPENING_PORTRAITS_SET2:
        return TRUE;
    default:
        return FALSE;
    }
}

static void ndsRelocUpdateMemoryLedger(void)
{
    u32 scene = (u32)gSCManagerSceneData.scene_curr;
    u32 capacity = (u32)ndsTaskmanArenaSize();
    u32 used = gNdsTaskmanGeneralHeapUsed;
    u32 i;

    if ((gSYTaskmanGeneralHeap.start != NULL) &&
        (gSYTaskmanGeneralHeap.ptr != NULL) &&
        ((uintptr_t)gSYTaskmanGeneralHeap.ptr >=
         (uintptr_t)gSYTaskmanGeneralHeap.start))
    {
        used = (u32)((uintptr_t)gSYTaskmanGeneralHeap.ptr -
                     (uintptr_t)gSYTaskmanGeneralHeap.start);
    }

    gNdsMemoryLedgerResult = 0;
    if ((gNdsMemoryLedgerScene != scene) ||
        (gNdsMemoryLedgerGeneration != sNdsRelocSceneGeneration))
    {
        gNdsMemoryLedgerArenaHighWater = 0u;
    }
    gNdsMemoryLedgerScene = scene;
    gNdsMemoryLedgerGeneration = sNdsRelocSceneGeneration;
    gNdsMemoryLedgerArenaCapacity = capacity;
    gNdsMemoryLedgerArenaUsed = used;
    if (used > gNdsMemoryLedgerArenaHighWater)
    {
        gNdsMemoryLedgerArenaHighWater = used;
    }
    gNdsMemoryLedgerArenaHeadroom = (capacity > used) ? (capacity - used) : 0u;
    gNdsMemoryLedgerDLBytes =
        gNdsTaskmanContexts * (u32)((sizeof(Gfx) * 7680u) +
                                    (sizeof(Gfx) * 2560u));
    gNdsMemoryLedgerGraphicsBytes =
        gNdsTaskmanContexts * gNdsTaskmanGraphicsHeapSize;
    gNdsMemoryLedgerRdpBytes = gNdsTaskmanRdpBufferSize;
    gNdsMemoryLedgerFigatreeHeapSize = (u32)gFTManagerFigatreeHeapSize;
    gNdsMemoryLedgerRelocFiles = sNdsRelocLoadedFileCount;
    gNdsMemoryLedgerRelocBytes = 0u;
    gNdsMemoryLedgerRelocStageBytes = 0u;
    gNdsMemoryLedgerRelocFighterBytes = 0u;
    gNdsMemoryLedgerRelocInterfaceBytes = 0u;
    gNdsMemoryLedgerRelocMenuBytes = 0u;
    gNdsMemoryLedgerRelocOpeningBytes = 0u;
    gNdsMemoryLedgerRelocOtherBytes = 0u;
    gNdsMemoryLedgerRelocStaleFiles = 0u;
    gNdsMemoryLedgerRelocStaleBytes = 0u;

    for (i = 0; i < sNdsRelocLoadedFileCount; i++)
    {
        NDSRelocLoadedFile *loaded = &sNdsRelocLoadedFiles[i];
        u32 bytes = loaded->data_size;

        gNdsMemoryLedgerRelocBytes += bytes;
        if (loaded->owner_scene != scene)
        {
            gNdsMemoryLedgerRelocStaleFiles++;
            gNdsMemoryLedgerRelocStaleBytes += bytes;
        }
        if (ndsRelocAssetIsInterface(loaded->asset_id) != FALSE)
        {
            gNdsMemoryLedgerRelocInterfaceBytes += bytes;
        }
        else if (ndsRelocAssetIsStage(loaded->asset_id) != FALSE)
        {
            gNdsMemoryLedgerRelocStageBytes += bytes;
        }
        else if (ndsRelocAssetIsFighter(loaded->asset_id) != FALSE)
        {
            gNdsMemoryLedgerRelocFighterBytes += bytes;
        }
        else if (ndsRelocAssetIsMenu(loaded->asset_id) != FALSE)
        {
            gNdsMemoryLedgerRelocMenuBytes += bytes;
        }
        else if (ndsRelocAssetIsOpening(loaded->asset_id) != FALSE)
        {
            gNdsMemoryLedgerRelocOpeningBytes += bytes;
        }
        else
        {
            gNdsMemoryLedgerRelocOtherBytes += bytes;
        }
    }

    if ((gNdsMemoryLedgerArenaHeadroom >=
         NDS_RELOC_MEMORY_LEDGER_RESERVE_BYTES) &&
        (gNdsMemoryLedgerRelocStaleFiles == 0u) &&
        (gNdsMemoryLedgerRelocStaleBytes == 0u))
    {
        gNdsMemoryLedgerResult = NDS_MEMORY_LEDGER_PASS;
    }
}

NDS_COLD void ndsRelocRecordSceneMemory(const SYTaskmanSceneSetup *setup)
{
    /* Final source-heap state, sampled before the next scene rewinds it.
     * Use the actual DS setup passed to taskman; the old diagnostic fields
     * describe N64 template sizes and may still be zero during construction. */
    ndsRelocUpdateMemoryLedger();
    gNdsMemoryLedgerDLBytes = (u32)setup->contexts_num *
        (u32)(setup->dl_buffer0_size + setup->dl_buffer1_size +
              setup->dl_buffer2_size + setup->dl_buffer3_size);
    gNdsMemoryLedgerGraphicsBytes = (u32)setup->contexts_num *
        (u32)setup->graphics_arena_size;
    gNdsMemoryLedgerRdpBytes = (u32)setup->rdp_output_buffer_size;
    /* The debugger reads main RAM at the next scene's eviction boundary.
     * Publish the finished snapshot out of ARM9's write-back cache before
     * returning; volatile alone does not make those reads coherent. */
    DC_FlushAll();
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

s32 ndsRelocGetLoadedAssetView(u32 asset_id, const void **out_data,
                               u32 *out_size)
{
    NDSRelocLoadedFile *loaded;

    if ((out_data == NULL) || (out_size == NULL))
    {
        return FALSE;
    }
    *out_data = NULL;
    *out_size = 0u;
    loaded = ndsRelocFindLoadedFileByAsset(asset_id);
    if ((loaded == NULL) || (loaded->data == NULL) ||
        (loaded->data_size == 0u) ||
        (loaded->owner_scene != (u32)gSCManagerSceneData.scene_curr) ||
        (loaded->owner_generation != sNdsRelocSceneGeneration))
    {
        return FALSE;
    }
    *out_data = loaded->data;
    *out_size = loaded->data_size;
    return TRUE;
}

/* lbRelocGetFileData asks this ~70 times a battle frame, mostly for the same
 * file, and materialization asks it per native asset address; the scan over
 * the whole table was the cost. Memo the last answer (the FIRST entry with
 * this data, or none) for the current table epoch. */
static NDSRelocLoadedFile *ndsRelocFindLoadedFileByData(void *file)
{
    static void *s_file;
    static u32 s_epoch = 0xffffffffu;
    static s32 s_index = -1;
    u32 i;

    if ((file == s_file) && (s_epoch == sNdsRelocLoadedFilesEpoch))
    {
        return (s_index >= 0) ? &sNdsRelocLoadedFiles[s_index] : NULL;
    }
    s_file = file;
    s_epoch = sNdsRelocLoadedFilesEpoch;
    s_index = -1;
    for (i = 0; i < sNdsRelocLoadedFileCount; i++)
    {
        if (sNdsRelocLoadedFiles[i].data == file)
        {
            s_index = (s32)i;
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

static void *ndsRelocFindStatusNode(LBFileNode *nodes, s32 count, u32 token)
{
    s32 i;

    for (i = 0; i < count; i++)
    {
        if (nodes[i].id == token)
        {
            return nodes[i].addr;
        }
    }
    return NULL;
}

static void ndsRelocAddStatusNode(LBFileNode *nodes, s32 *count,
                                  s32 max_count, u32 token, void *addr)
{
    if ((nodes == NULL) || (count == NULL) || (addr == NULL) ||
        (max_count <= 0))
    {
        return;
    }
    if (ndsRelocFindStatusNode(nodes, *count, token) != NULL)
    {
        return;
    }
    if (*count >= max_count)
    {
        gNdsRelocAssetOpenFailCount++;
        return;
    }
    nodes[*count].id = token;
    nodes[*count].addr = addr;
    (*count)++;
}

static void ndsRelocSetStatusNode(LBFileNode *nodes, s32 *count,
                                  s32 max_count, u32 token, void *addr)
{
    s32 i;

    if ((nodes == NULL) || (count == NULL) || (addr == NULL))
    {
        return;
    }
    for (i = 0; i < *count; i++)
    {
        if (nodes[i].id == token)
        {
            nodes[i].addr = addr;
            return;
        }
    }
    ndsRelocAddStatusNode(nodes, count, max_count, token, addr);
}

static void ndsRelocAddStatusBufferFile(u32 token, void *addr)
{
    ndsRelocAddStatusNode(sNdsRelocStatusBuffer, &sNdsRelocStatusBufferCount,
                          sNdsRelocStatusBufferMax, token, addr);
}

static void ndsRelocSetStatusBufferFile(u32 token, void *addr)
{
    ndsRelocSetStatusNode(sNdsRelocStatusBuffer, &sNdsRelocStatusBufferCount,
                          sNdsRelocStatusBufferMax, token, addr);
}

static void ndsRelocSetForceStatusBufferFile(u32 token, void *addr)
{
    ndsRelocSetStatusNode(sNdsRelocForceStatusBuffer,
                          &sNdsRelocForceStatusBufferCount,
                          sNdsRelocForceStatusBufferMax, token, addr);
}
