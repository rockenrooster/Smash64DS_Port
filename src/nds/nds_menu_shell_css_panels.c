/* Native character select, the panels: the 1P and bonus cards, their
 * records and digits, the preview underlay and the slot doors. A slice of
 * nds_menu_shell.c, after nds_menu_shell_css.c. */

/* The 1P card: the fighter the token is on (or over, carried) by its gate
 * index, which is the VS card's (CSS_FIGHTER_TOKEN order), else empty. */
_Static_assert(NDS_MN_UI_KIT_SURFACE_ONEP_P4_READY_ON ==
                   NDS_MN_UI_KIT_SURFACE_ONEP_P4_GATE_F0 + NDS_CSS_GATE_FIGHTERS,
               "1P cards must be one per gate fighter, contiguous");
_Static_assert(NDS_MN_UI_KIT_SURFACE_ONEP_P4_LEVEL_VERY_HARD ==
                   NDS_MN_UI_KIT_SURFACE_ONEP_P4_LEVEL_VERY_EASY + 4u,
               "1P level surfaces must stay contiguous in level order");

_Static_assert(NDS_MN_UI_KIT_SURFACE_BONUS_P4_RECORD_NONE ==
                   NDS_MN_UI_KIT_SURFACE_BONUS_P4_GATE_F0 + NDS_CSS_GATE_FIGHTERS,
               "bonus cards must be one per gate fighter, contiguous");

static NdsUiKitSurfaceId ndsMenuShellCssOnePGateSurface(void)
{
    u32 state = ndsMenuShellCssGateState(0u);

    if ((state >= NDS_CSS_GATE_MAN_F0) && (state < NDS_CSS_GATE_COM_F0))
    {
        return (NdsUiKitSurfaceId)(
            (NDS_CSS_BONUS ? NDS_MN_UI_KIT_SURFACE_BONUS_P4_GATE_F0 :
                             NDS_MN_UI_KIT_SURFACE_ONEP_P4_GATE_F0) +
            (state - NDS_CSS_GATE_MAN_F0));
    }
    return NDS_CSS_BONUS ? NDS_MN_UI_KIT_SURFACE_BONUS_P4_GATE_EMPTY :
                           NDS_MN_UI_KIT_SURFACE_ONEP_P4_GATE_EMPTY;
}

/* The puck fighter's record (mnPlayers1PGameMakeFighterRecord :1469-1561),
 * for every selection: the source's row for an original, C3's wide row for
 * Metal Mario and a content (nds_p7_records.c), read once as the select opens
 * (ndsMenuShellCssInitOneP). The bonus selects read the same rows' bonus
 * fields (C2). */
typedef struct NDSCssOnePRecord
{
    u32 hiscore;
    u32 bonuses;
    u32 bonus_time[2];
    u8 bonus_count[2];
    u8 difficulty;
} NDSCssOnePRecord;
static NDSCssOnePRecord sCssOnePWide[NDS_P7_SEL_COUNT - GMCOMMON_FIGHTERS_PLAYABLE_NUM];
/* 1 + the selection id shown (1: none), 0 before the first draw. */
static u32 sCssOnePRecordShown;

static void ndsMenuShellCssOnePCacheRecord(u32 sel, const LBBackup1PRecord *row,
                                           void *ctx)
{
    NDSCssOnePRecord *dst = &sCssOnePWide[sel - GMCOMMON_FIGHTERS_PLAYABLE_NUM];

    (void)ctx;
    dst->hiscore = row->spgame_hiscore;
    dst->bonuses = row->spgame_total_bonuses;
    dst->bonus_time[0] = row->bonus1_time;
    dst->bonus_time[1] = row->bonus2_time;
    dst->bonus_count[0] = row->bonus1_task_count;
    dst->bonus_count[1] = row->bonus2_task_count;
    dst->difficulty = row->spgame_best_difficulty;
}

_Static_assert(NDS_MN_UI_KIT_SURFACE_ONEP_P4_RECORD_LOGO_5 ==
                   NDS_MN_UI_KIT_SURFACE_ONEP_P4_RECORD_LOGO_1 + 4u,
               "1P record logos must stay contiguous in difficulty order");
_Static_assert(NDS_MN_UI_KIT_SURFACE_ONEP_P4_BLUE_DIGIT_9 ==
                   NDS_MN_UI_KIT_SURFACE_ONEP_P4_BLUE_DIGIT_0 + 9u,
               "1P record blue digits must stay contiguous in digit order");

/* mnPlayers1PGameMakeNumber (:205-241), is_fixed_digit_count TRUE: `places`
 * digits, the ones at right - 8 and each higher place 8 px further left. */
static u32 ndsMenuShellCssOnePNumber(NdsUiKitSurfaceId *surface, s16 *site, u32 n,
                                     u32 digit0, u32 value, s32 right, s32 y,
                                     u32 places)
{
    s32 x = right;

    while (places-- != 0u)
    {
        x -= 8;
        surface[n] = (NdsUiKitSurfaceId)(digit0 + (value % 10u));
        site[2u * n] = (s16)(((x * 4) + 2) / 5);
        site[(2u * n) + 1u] = (s16)(((y * 4) + 2) / 5);
        value /= 10u;
        n++;
    }
    return n;
}

static void ndsMenuShellCssOnePShowRecord(void)
{
    NdsUiKitSurfaceId surface[1u + 1u + 8u + 3u];
    s16 site[2u * (1u + 1u + 8u + 3u)];
    u32 state = ndsMenuShellCssGateState(0u);
    u32 shown = 1u;
    u32 hiscore = 0u;
    u32 bonuses = 0u;
    u32 difficulty = 0u;
    u32 n = 0u;

    if ((state >= NDS_CSS_GATE_MAN_F0) && (state < NDS_CSS_GATE_COM_F0))
    {
        u32 sel = (u32)sCssFkind[0];
        u32 p7 = ndsP7SelOf((s32)ndsMenuShellCssSelFkind(sel),
                            ndsMenuShellCssSelContent(sel));

        if (p7 < GMCOMMON_FIGHTERS_PLAYABLE_NUM)
        {
            const LBBackup1PRecord *row = &gSCManagerBackupData.spgame_records[p7];

            hiscore = row->spgame_hiscore;
            bonuses = row->spgame_total_bonuses;
            difficulty = row->spgame_best_difficulty;
            shown = 2u + p7;
        }
        else if (p7 < NDS_P7_SEL_COUNT)
        {
            const NDSCssOnePRecord *row =
                &sCssOnePWide[p7 - GMCOMMON_FIGHTERS_PLAYABLE_NUM];

            hiscore = row->hiscore;
            bonuses = row->bonuses;
            difficulty = row->difficulty;
            shown = 2u + p7;
        }
    }
    if (sCssOnePRecordShown == shown)
    {
        return;
    }
    surface[n] = (shown == 1u) ? NDS_MN_UI_KIT_SURFACE_ONEP_P4_RECORD_OFF :
                                 NDS_MN_UI_KIT_SURFACE_ONEP_P4_RECORD_ON;
    site[0] = NDS_UI_KIT_SITE_BAKED;
    site[1] = 0;
    n++;
    if (shown != 1u)
    {
        if ((difficulty >= 1u) && (difficulty <= 5u))
        {
            surface[n] = (NdsUiKitSurfaceId)(NDS_MN_UI_KIT_SURFACE_ONEP_P4_RECORD_LOGO_1 +
                                             (difficulty - 1u));
            site[2u * n] = (s16)(((126 * 4) + 2) / 5);
            site[(2u * n) + 1u] = (s16)(((198 * 4) + 2) / 5);
            n++;
        }
        n = ndsMenuShellCssOnePNumber(surface, site, n,
                                      NDS_MN_UI_KIT_SURFACE_BONUS_DIGIT_0,
                                      hiscore, 256, 198, 8u);
        n = ndsMenuShellCssOnePNumber(surface, site, n,
                                      NDS_MN_UI_KIT_SURFACE_ONEP_P4_BLUE_DIGIT_0,
                                      bonuses, 285, 198, 3u);
    }
    if (ndsUiKitBlitSurfacesAt(surface, site, n) != FALSE)
    {
        sCssOnePRecordShown = shown;
        gNdsMenuShellCssPanelBlitCount++;
    }
}

/* mnPlayers1PBonusGetMins/Sec/CSec (:963-983); the hundredths' float
 * division ((tics % 6) / 0.554F, truncated) is its table. */
static void ndsMenuShellCssBonusSplit(u32 tics, u32 *out)
{
    static const u8 kHundredths[6] = { 0u, 1u, 3u, 5u, 7u, 9u };
    const u32 in_min = tics % (u32)TIME_MIN;

    out[0] = tics / (u32)TIME_MIN;
    out[1] = in_min / (u32)TIME_SEC;
    out[2] = (((in_min % (u32)TIME_SEC) / 6u) * 10u) + kHundredths[in_min % 6u];
}

/* mnPlayers1PBonusGetBestTime (:938-960): under an hour. */
static u32 ndsMenuShellCssBonusBestTime(u32 tics)
{
    return (tics > (u32)(I_HRS_TO_TICS(1) - 1)) ?
        (u32)(I_HRS_TO_TICS(1) - 1) : tics;
}

/* The total row (mnPlayers1PBonusMakeTotalTime :1187-1244), made as the
 * select opens and on a title toggle (:2860, :2093) once every original has
 * cleared the kind (CheckBonusCompleteAll :2799, the twelve source rows): the
 * unlocked originals' minutes, seconds and hundredths summed apart
 * (GetTotalMins/Sec/CSec :985-1031, CheckFighterLocked :380 on the save's
 * mask) with its own carries. Its key, 0 for no row. */
static u32 ndsMenuShellCssBonusTotalKey(void)
{
    const u32 kind = (u32)sCssBonusKind;
    u32 sum[3] = { 0u, 0u, 0u };
    u32 carry;
    s32 fkind;

    for (fkind = nFTKindPlayableStart; fkind <= nFTKindPlayableEnd; fkind++)
    {
        const LBBackup1PRecord *row = &gSCManagerBackupData.spgame_records[fkind];

        if (((kind == 0u) ? row->bonus1_task_count : row->bonus2_task_count) !=
            SCBATTLE_BONUSGAME_TASK_MAX)
        {
            return 0u;
        }
    }
    for (fkind = nFTKindPlayableStart; fkind <= nFTKindPlayableEnd; fkind++)
    {
        const LBBackup1PRecord *row = &gSCManagerBackupData.spgame_records[fkind];
        u32 split[3];

        if (((fkind == nFTKindNess) || (fkind == nFTKindPurin) ||
             (fkind == nFTKindCaptain) || (fkind == nFTKindLuigi)) &&
            ((gSCManagerBackupData.fighter_mask & (1 << fkind)) == 0))
        {
            continue;
        }
        ndsMenuShellCssBonusSplit(ndsMenuShellCssBonusBestTime(
                                      (kind == 0u) ? row->bonus1_time :
                                                     row->bonus2_time),
                                  split);
        sum[0] += split[0];
        sum[1] += split[1];
        sum[2] += split[2];
    }
    carry = sum[2] / 100u;
    sCssBonusTotal[2] = sum[2] % 100u;
    sum[1] += carry;
    sCssBonusTotal[1] = sum[1] % (u32)TIME_SEC;
    sCssBonusTotal[0] = sum[0] + (sum[1] / (u32)TIME_SEC);
    return (1u << 31) | ((sCssBonusTotal[0] % 1000u) << 14) |
           ((sCssBonusTotal[1] % 100u) << 7) | (sCssBonusTotal[2] % 100u);
}

/* The bonus selects' rows over BONUS_P4_CSS_SCREEN, in one open: the kind's
 * title (MakeLabels :877-910), the puck fighter's record (MakeHiScore
 * :1171-1178, re-made by the puck process every frame :2248: the best time
 * once it has cleared the kind's tasks -- CheckBonusComplete :1153, MakeBestTime
 * :1033-1094 -- else the best count and the kind's word, MakeBestTaskCount
 * :1107-1150, REGION_US; nothing with no fighter) and the total row, each
 * re-blitted only when its key changed. A content's and Metal Mario's record
 * is C3's wide row's (sCssOnePWide). Digits are the vanilla presenter's
 * keyed glyphs at mnPlayers1PBonusMakeNumber's sites (nds_menu_shell_onep.c). */
static void ndsMenuShellCssBonusShowRows(void)
{
    const u32 total_key = sCssBonusTotalKey;
    NdsUiKitSurfaceId surface[1u + (1u + 6u) + (1u + 7u)];
    s16 site[2u * (1u + (1u + 6u) + (1u + 7u))];
    const u32 kind = (u32)sCssBonusKind;
    const u32 state = ndsMenuShellCssGateState(0u);
    u32 record = 0u;
    u32 split[3] = { 0u, 0u, 0u };
    u32 count = 0u;
    u32 n = 0u;
    u32 shown[3];

    shown[0] = sCssBonusShown;
    shown[1] = sCssBonusRecordShown;
    shown[2] = sCssBonusTotalShown;
    if ((state >= NDS_CSS_GATE_MAN_F0) && (state < NDS_CSS_GATE_COM_F0))
    {
        const u32 sel = (u32)sCssFkind[0];
        const u32 p7 = ndsP7SelOf((s32)ndsMenuShellCssSelFkind(sel),
                                  ndsMenuShellCssSelContent(sel));
        u32 time = 0u;

        if (p7 < GMCOMMON_FIGHTERS_PLAYABLE_NUM)
        {
            const LBBackup1PRecord *row = &gSCManagerBackupData.spgame_records[p7];

            time = (kind == 0u) ? row->bonus1_time : row->bonus2_time;
            count = (kind == 0u) ? row->bonus1_task_count : row->bonus2_task_count;
            record = 1u;
        }
        else if (p7 < NDS_P7_SEL_COUNT)
        {
            const NDSCssOnePRecord *row =
                &sCssOnePWide[p7 - GMCOMMON_FIGHTERS_PLAYABLE_NUM];

            time = row->bonus_time[kind];
            count = row->bonus_count[kind];
            record = 1u;
        }
        if (record != 0u)
        {
            if (count == SCBATTLE_BONUSGAME_TASK_MAX)
            {
                ndsMenuShellCssBonusSplit(ndsMenuShellCssBonusBestTime(time),
                                          split);
                record = (1u << 28) | ((split[0] % 100u) << 16) |
                         ((split[1] % 100u) << 8) | (split[2] % 100u);
            }
            else
            {
                record = (2u << 28) | (kind << 24) | (count % 100u);
            }
        }
    }
    if (sCssBonusShown != kind + 1u)
    {
        surface[n] = (kind == 0u) ? NDS_MN_UI_KIT_SURFACE_BONUS_P4_TITLE_TARGETS :
                                    NDS_MN_UI_KIT_SURFACE_BONUS_P4_TITLE_PLATFORMS;
        site[2u * n] = NDS_UI_KIT_SITE_BAKED;
        site[(2u * n) + 1u] = 0;
        n++;
        shown[0] = kind + 1u;
    }
    if (sCssBonusRecordShown != record + 1u)
    {
        site[2u * n] = NDS_UI_KIT_SITE_BAKED;
        site[(2u * n) + 1u] = 0;
        if ((record >> 28) == 1u)
        {
            /* BEST TIME and the marks; minutes, seconds and hundredths end at
             * 237, 259 and 283 on row 195. */
            surface[n++] = NDS_MN_UI_KIT_SURFACE_BONUS_P4_RECORD_TIME;
            n = ndsMenuShellCssOnePNumber(surface, site, n,
                                          NDS_MN_UI_KIT_SURFACE_BONUS_DIGIT_0,
                                          split[0], 237, 195, 2u);
            n = ndsMenuShellCssOnePNumber(surface, site, n,
                                          NDS_MN_UI_KIT_SURFACE_BONUS_DIGIT_0,
                                          split[1], 259, 195, 2u);
            n = ndsMenuShellCssOnePNumber(surface, site, n,
                                          NDS_MN_UI_KIT_SURFACE_BONUS_DIGIT_0,
                                          split[2], 283, 195, 2u);
        }
        else if ((record >> 28) == 2u)
        {
            /* The kind's word, two places ending at 225 on row 194. */
            surface[n++] = (kind == 0u) ?
                NDS_MN_UI_KIT_SURFACE_BONUS_P4_RECORD_TARGETS :
                NDS_MN_UI_KIT_SURFACE_BONUS_P4_RECORD_PLATFORMS;
            n = ndsMenuShellCssOnePNumber(surface, site, n,
                                          NDS_MN_UI_KIT_SURFACE_BONUS_DIGIT_0,
                                          count % 100u, 225, 194, 2u);
        }
        else
        {
            surface[n++] = NDS_MN_UI_KIT_SURFACE_BONUS_P4_RECORD_NONE;
        }
        shown[1] = record + 1u;
    }
    if (sCssBonusTotalShown != total_key + 1u)
    {
        site[2u * n] = NDS_UI_KIT_SITE_BAKED;
        site[(2u * n) + 1u] = 0;
        if (total_key == 0u)
        {
            surface[n++] = NDS_MN_UI_KIT_SURFACE_BONUS_P4_TOTAL_NONE;
        }
        else
        {
            /* TOTAL BEST TIME and the marks; hundredths and seconds end at
             * 283 and 259, the minutes' three places at 237, on row 206. */
            surface[n++] = NDS_MN_UI_KIT_SURFACE_BONUS_P4_TOTAL_TIME;
            n = ndsMenuShellCssOnePNumber(surface, site, n,
                                          NDS_MN_UI_KIT_SURFACE_BONUS_DIGIT_0,
                                          sCssBonusTotal[2], 283, 206, 2u);
            n = ndsMenuShellCssOnePNumber(surface, site, n,
                                          NDS_MN_UI_KIT_SURFACE_BONUS_DIGIT_0,
                                          sCssBonusTotal[1], 259, 206, 2u);
            n = ndsMenuShellCssOnePNumber(surface, site, n,
                                          NDS_MN_UI_KIT_SURFACE_BONUS_DIGIT_0,
                                          sCssBonusTotal[0], 237, 206, 3u);
        }
        shown[2] = total_key + 1u;
    }
    if ((n != 0u) && (ndsUiKitBlitSurfacesAt(surface, site, n) != FALSE))
    {
        sCssBonusShown = shown[0];
        sCssBonusRecordShown = shown[1];
        sCssBonusTotalShown = shown[2];
        gNdsMenuShellCssPanelBlitCount++;
    }
}

/* mnPlayers1PGameMakeTimeSelect / MakeLevel (:1019/:1192): the time and the
 * level as baked states, both in one open when either changed. */
static void ndsMenuShellCssOnePShowOptions(void)
{
    NdsUiKitSurfaceId surfaces[2];
    u32 key = ((u32)sCssOnePLevel << 8) | (u32)sCssOnePTime;

    if (sCssOnePShown == key + 1u)
    {
        return;
    }
    surfaces[0] = (sCssOnePTime == (u8)SCBATTLE_TIMELIMIT_INFINITE) ?
        NDS_MN_UI_KIT_SURFACE_ONEP_P4_TIME_INF :
        NDS_MN_UI_KIT_SURFACE_ONEP_P4_TIME_5;
    surfaces[1] = (NdsUiKitSurfaceId)(NDS_MN_UI_KIT_SURFACE_ONEP_P4_LEVEL_VERY_EASY +
                                      (u32)sCssOnePLevel);
    if (ndsUiKitBlitSurfaces(surfaces, 2u) != FALSE)
    {
        sCssOnePShown = key + 1u;
        gNdsMenuShellCssPanelBlitCount += 2u;
    }
}

/* mnPlayers1PGameLevelThreadUpdate / StockThreadUpdate (:1136/:1250): each
 * row's arrows blink on a 10-tic cycle and the one at an end of its range is
 * gone. The stocks show as the source's count + 1 in the digit art the
 * original 1P select's presenter uses (nds_menu_shell_onep.c). */
static void ndsMenuShellCssOnePDrawArrows(void)
{
    static const s16 kRowY[2] = { 158, 178 };
    const u32 value[2] = { sCssOnePLevel, sCssOnePStock };
    u32 row;

    for (row = 0u; row < (u32)NDS_CSS_SLOTS; row++)
    {
        if (row >= 2u)
        {
            ndsUiKitHideSprite(NDS_CSS_SPRITE_ARROWL0 + row);
            ndsUiKitHideSprite(NDS_CSS_SPRITE_ARROWR0 + row);
            continue;
        }
        if ((sCssArrowsShown != FALSE) && (value[row] > 0u))
        {
            ndsUiKitSetSprite(NDS_CSS_SPRITE_ARROWL0 + row,
                              NDS_MN_UI_KIT_IMAGE_CSS_ARROW_L,
                              NDS_CSS_DS(194), NDS_CSS_DS(kRowY[row]));
        }
        else
        {
            ndsUiKitHideSprite(NDS_CSS_SPRITE_ARROWL0 + row);
        }
        if ((sCssArrowsShown != FALSE) && (value[row] < 4u))
        {
            ndsUiKitSetSprite(NDS_CSS_SPRITE_ARROWR0 + row,
                              NDS_MN_UI_KIT_IMAGE_CSS_ARROW_R,
                              NDS_CSS_DS(269), NDS_CSS_DS(kRowY[row]));
        }
        else
        {
            ndsUiKitHideSprite(NDS_CSS_SPRITE_ARROWR0 + row);
        }
    }
    ndsUiKitSetSprite(NDS_CSS_SPRITE_DIGIT0,
                      NDS_MN_UI_KIT_IMAGE_DIGIT_0 + (u32)sCssOnePStock + 1u,
                      NDS_CSS_DS(207), NDS_CSS_DS(178));
}

/* P2-1N (3): mnPlayersVSShutterProcUpdate + mnPlayersVSUpdateShutter,
 * transcribed. door_offset walks 2/tic toward 41 (shut) for an empty slot and
 * 0 (open) otherwise; the close cue fires on arrival at 41
 * (nSYAudioFGMPlayerSlotClose = 166 — asks; the miss ring reports it until a
 * pack row lands). Mid-slide, the slot's current gate surface re-blits as the
 * underlay and both card halves draw from the cached CSS_DOORS strip at their
 * slid positions, clipped to the gate box exactly as the source's frame art
 * masks the pocketed part. Terminal frames land the baked gate, so steady
 * state costs nothing. */
/* ONE NITROFS OPEN A FRAME, NOT ONE PER SLIDING SLOT.
 *
 * This walked the slots and called ndsUiKitBlitSurfaces(&blit, 1u) inside the
 * loop. That function takes a LIST precisely because it opens the surface pack
 * once and then streams every id in it (ndsUiKitBlitSurfacesLayer), so calling
 * it per slot paid a separate ndsRelocAssetStreamOpen/Close for each one. At
 * character-select entry all four gates slide at once, so a single frame paid
 * FOUR opens -- on a screen whose own header states that one NitroFS open costs
 * more than a whole frame's budget (nds_ui_kit.h), and whose worst frame was
 * already at 71% of budget before any of this (ndsMenuShellCssSyncPanels).
 *
 * Collect first, blit once, then draw the doors. Identical bytes read and
 * identical pixels; the slide just stops paying for the pack four times.
 *
 * This does NOT fix the underlying waste -- the mid-slide underlay is the SAME
 * 7,738-byte surface re-read every tic purely to repaint what the doors
 * trampled, and removing that needs somewhere to cache a panel band or a
 * foreground layer for the halves. The owner reported "Low FPS/Flashing during
 * gate openings"; this removes three quarters of the file opens behind it. */
/* THE CARD UNDER A SLIDING DOOR IS STATIC, SO IT IS READ ONCE.
 *
 * Mid-slide, every tic repaints the slot's card and then the two door halves
 * over it. The card came from NitroFS each time -- an open, a 7,844-byte read,
 * a hash and a flush per sliding slot per tic, twenty tics a slide, which is
 * the measured multi-frame sync spike behind "low FPS / flashing during gate
 * openings" (9.6% of CSS frames missed 60 Hz). Each slot now keeps a resident
 * copy for the length of its slide: one verified read on the first mid-slide
 * tic, DMA rows from main RAM after. The terminal frame still takes the baked
 * gate from the pack, once.
 *
 * ARENA, NOT .bss: four cards are 31-41 KB and a static buffer would cost
 * every scene, battle included. The block comes out of this scene's arena and
 * dies with it; the heap generation says whether the pointer is still ours.
 * A refusal (no room, bad read) falls back to the streamed blit unchanged.
 *
 * RESERVED AT SCENE LOAD, NEVER MID-SCENE. The preview slice loader rewinds
 * the arena cursor to its own mark when a load is cancelled, on the stated
 * invariant that its payload is the newest allocation. A lazy allocation on
 * the first slide broke that: re-entering the CSS from Results slides the
 * doors while the previous fighters' previews are still loading, the cancel
 * rewound under this block, and the next card copy landed in a preview pack
 * -- a wandering-PC abort a few frames later.
 *
 * SIZED BY THE BAKE: the largest gate surface (NDS_MN_UI_KIT_CSS_GATE_MAX_BYTES,
 * 7,844 B on the original select, 10,370 B for the P4 select's scaled cards).
 * A card larger than its copy fails the load on every slide and falls back
 * to the streamed blit, which is how the scaled cards brought the slow,
 * flashing doors back (owner, 2026-10-11). */
#define NDS_CSS_UNDERLAY_BYTES ((NDS_MN_UI_KIT_CSS_GATE_MAX_BYTES + 3u) & ~3u)
/* The underlay is a cache, so it takes only bytes the select's preview
 * fighters will not need after it: their objects took 25,880 B with two
 * previews (all-content walk ROM, 2026-10-07), about 40 KiB with four. On
 * the all-content image this declines and the doors take the streamed blit
 * (reloc_backend_assets.c, NDS_ANIM_CACHE_CSS_KEEP_FREE). */
#define NDS_CSS_UNDERLAY_KEEP_FREE (48u * 1024u)
extern volatile u32 gNdsTaskmanHeapGeneration;
sb32 ndsSyMallocWouldFit(const struct SYMallocRegion *bp, size_t size,
                         u32 alignment);
static u8 *sCssUnderlayBlock;
static u32 sCssUnderlayGeneration;
static u32 sCssUnderlaySurface[NDS_CSS_SLOTS];
volatile u32 gNdsMenuShellCssUnderlayBlitCount;
volatile u32 gNdsMenuShellCssUnderlayDeclineCount;

static void ndsMenuShellCssUnderlayReserve(void)
{
    u32 i;

    sCssUnderlayBlock = NULL;
    for (i = 0u; i < (u32)NDS_CSS_SLOTS; i++)
    {
        sCssUnderlaySurface[i] = 0xffffffffu;
    }
    if (ndsSyMallocWouldFit(&gSYTaskmanGeneralHeap,
                            NDS_CSS_UNDERLAY_BYTES * (u32)NDS_CSS_SLOTS +
                                NDS_CSS_UNDERLAY_KEEP_FREE,
                            4u) == FALSE)
    {
        NDS_DIAG(gNdsMenuShellCssUnderlayDeclineCount++);
        return;
    }
    sCssUnderlayBlock = syTaskmanMalloc(
        NDS_CSS_UNDERLAY_BYTES * (u32)NDS_CSS_SLOTS, 4u);
    sCssUnderlayGeneration = gNdsTaskmanHeapGeneration;
}

static u8 *ndsMenuShellCssUnderlay(u32 slot, u32 surface)
{
    u8 *buffer;

    if ((sCssUnderlayBlock == NULL) ||
        (sCssUnderlayGeneration != gNdsTaskmanHeapGeneration))
    {
        return NULL;
    }
    buffer = sCssUnderlayBlock + (slot * NDS_CSS_UNDERLAY_BYTES);
    if (sCssUnderlaySurface[slot] != surface)
    {
        sCssUnderlaySurface[slot] = 0xffffffffu;
        if (ndsUiKitLoadSurfaceCopy(surface, buffer,
                                    NDS_CSS_UNDERLAY_BYTES) == FALSE)
        {
            NDS_DIAG(gNdsMenuShellCssUnderlayDeclineCount++);
            return NULL;
        }
        sCssUnderlaySurface[slot] = surface;
    }
    return buffer;
}

/* THE SAME COPY IN VRAM WHERE THE ARENA HAS NO ROOM.
 *
 * The all-content image never has the four copies (and the 48 KiB behind
 * them) at the select's load, so there every sliding tic streamed its card
 * again: an open, a 10 KB read and a hash per sliding slot per tic, twenty
 * tics a slide, four slots at once at the entry and back from a match -- the
 * owner's "card door open/close is very slow" (BUGS.md 2026-10-12). BG2's
 * offscreen rows (ndsUiKitBackgroundSpare, 32 KB) take the copy instead: the
 * card's on-screen rectangle right after its first streamed blit, before the
 * halves draw over it, then VRAM to VRAM for the rest of the slide. Three P4
 * cards (10,370 B) or four original ones (7,844 B) fit; a slot without a
 * line streams as before. gNdsMenuShellCssVramUnderlay is the A/B word. */
#define NDS_CSS_VRAM_LINES 4u
#define NDS_CSS_VRAM_LINE_FREE 0xffu
volatile u32 gNdsMenuShellCssVramUnderlay __attribute__((used)) = 1u;
volatile u32 gNdsMenuShellCssVramUnderlayBlitCount;
volatile u32 gNdsMenuShellCssVramUnderlaySaveCount;
static u8 sCssVramLineSlot[NDS_CSS_VRAM_LINES];
static u16 sCssVramLineSurface[NDS_CSS_VRAM_LINES];

static void ndsMenuShellCssVramUnderlayReset(void)
{
    u32 line;

    for (line = 0u; line < NDS_CSS_VRAM_LINES; line++)
    {
        sCssVramLineSlot[line] = NDS_CSS_VRAM_LINE_FREE;
    }
}

static u16 *ndsMenuShellCssVramLine(u32 line)
{
    u32 bytes = 0u;
    u16 *spare = ndsUiKitBackgroundSpare(&bytes);

    if ((spare == NULL) || (((line + 1u) * NDS_CSS_UNDERLAY_BYTES) > bytes))
    {
        return NULL;
    }
    return spare + (line * (NDS_CSS_UNDERLAY_BYTES / sizeof(u16)));
}

static void ndsMenuShellCssVramUnderlayRelease(u32 slot)
{
    u32 line;

    for (line = 0u; line < NDS_CSS_VRAM_LINES; line++)
    {
        if (sCssVramLineSlot[line] == (u8)slot)
        {
            sCssVramLineSlot[line] = NDS_CSS_VRAM_LINE_FREE;
        }
    }
}

static sb32 ndsMenuShellCssVramUnderlayRestore(u32 slot, u32 surface)
{
    u32 line;

    for (line = 0u; line < NDS_CSS_VRAM_LINES; line++)
    {
        const u16 *copy;

        if (sCssVramLineSlot[line] != (u8)slot)
        {
            continue;
        }
        copy = ndsMenuShellCssVramLine(line);
        if ((sCssVramLineSurface[line] == (u16)surface) && (copy != NULL) &&
            (ndsUiKitRestoreSurfaceRect(surface, copy) != FALSE))
        {
            gNdsMenuShellCssVramUnderlayBlitCount++;
            return TRUE;
        }
        sCssVramLineSlot[line] = NDS_CSS_VRAM_LINE_FREE;
    }
    return FALSE;
}

static void ndsMenuShellCssVramUnderlaySave(u32 slot, u32 surface)
{
    u32 line;
    u16 *copy;

    ndsMenuShellCssVramUnderlayRelease(slot);
    if (gNdsMenuShellCssVramUnderlay == 0u)
    {
        return;
    }
    for (line = 0u; line < NDS_CSS_VRAM_LINES; line++)
    {
        if (sCssVramLineSlot[line] == NDS_CSS_VRAM_LINE_FREE)
        {
            break;
        }
    }
    copy = (line < NDS_CSS_VRAM_LINES) ? ndsMenuShellCssVramLine(line) : NULL;
    if ((copy != NULL) &&
        (ndsUiKitSaveSurfaceRect(surface, copy, NDS_CSS_UNDERLAY_BYTES) !=
         FALSE))
    {
        sCssVramLineSlot[line] = (u8)slot;
        sCssVramLineSurface[line] = (u16)surface;
        gNdsMenuShellCssVramUnderlaySaveCount++;
    }
}

static void ndsMenuShellCssStepDoors(void)
{
    NdsUiKitSurfaceId blits[NDS_CSS_SLOTS];
    u32 blit_slot[NDS_CSS_SLOTS];
    u8 blit_is_terminal[NDS_CSS_SLOTS];
    u8 sliding[NDS_CSS_SLOTS];
    u32 blit_count = 0u;
    u32 i;

    if (NDS_CSS_ONEP)
    {
        /* The 1P card has no shutter (mnPlayers1PGameMakeGate). */
        return;
    }
    for (i = 0u; i < (u32)NDS_CSS_SLOTS; i++)
    {
        u32 target = (sCssPkind[i] == (u8)nFTPlayerKindNot) ? 41u : 0u;
        u32 offset = (u32)sCssDoorOffset[i];

        sliding[i] = 0u;
        if (offset == target)
        {
            continue;
        }
        if (offset < target)
        {
            offset += 2u;
            if (offset >= 41u)
            {
                offset = 41u;
                ndsMenuShellCssCue(166u);
            }
        }
        else
        {
            offset = (offset >= 2u) ? (offset - 2u) : 0u;
        }
        sCssDoorOffset[i] = (u8)offset;
        blit_slot[blit_count] = i;
        if ((offset == 41u) || (offset == 0u))
        {
            /* Terminal frame: the baked gate lands and the doors are done. */
            blits[blit_count] = ndsMenuShellCssGateSurface(i);
            blit_is_terminal[blit_count] = 1u;
            ndsMenuShellCssVramUnderlayRelease(i);
        }
        else
        {
            /* Mid-slide: the slot's current card is the underlay the halves
             * are drawn over -- from the resident copy when there is one. */
            const u8 *underlay = ndsMenuShellCssUnderlay(i, sCssPanelSurface[i]);

            sliding[i] = 1u;
            if ((underlay != NULL) &&
                (ndsUiKitBlitSurfaceCopy(sCssPanelSurface[i], underlay) != FALSE))
            {
                NDS_DIAG(gNdsMenuShellCssUnderlayBlitCount++);
                continue;
            }
            if (ndsMenuShellCssVramUnderlayRestore(i, sCssPanelSurface[i]) !=
                FALSE)
            {
                continue;
            }
            blits[blit_count] = sCssPanelSurface[i];
            blit_is_terminal[blit_count] = 0u;
        }
        blit_count++;
    }

    /* No early return on an empty list: a slot whose card came from its
     * resident copy still owes its two door halves below. */
    if ((blit_count != 0u) &&
        (ndsUiKitBlitSurfaces(blits, blit_count) != FALSE))
    {
        for (i = 0u; i < blit_count; i++)
        {
            if (blit_is_terminal[i] != 0u)
            {
                sCssPanelSurface[blit_slot[i]] = blits[i];
            }
            else
            {
                /* The clean card, before the halves below draw over it. */
                ndsMenuShellCssVramUnderlaySave(blit_slot[i], blits[i]);
            }
        }
        gNdsMenuShellCssPanelBlitCount += blit_count;
    }

    for (i = 0u; i < (u32)NDS_CSS_SLOTS; i++)
    {
        s32 start = (s32)(i * 69u);
        u32 offset = (u32)sCssDoorOffset[i];

        if (sliding[i] == 0u)
        {
            continue;
        }
        {
            /* THE BAKE'S OWN ROUNDING, not NDS_CSS_DS: frame_pos rounds half
             * up while the plain macro truncates, and the two disagree by a
             * pixel exactly where it shows — the right half drew 32 of its
             * 33 columns and the pair sat one row above the gate (owner
             * screenshot, 2026-08-19). One rule for every door number. */
            #define NDS_CSS_DSR(v) (((v) * 4 + 2) / 5)
            s32 clip0 = NDS_CSS_DSR(NDS_CSS_CARD_X(start, 22));
            s32 clip1 = NDS_CSS_DSR(NDS_CSS_CARD_X(start, 88));
            s32 dy = NDS_CSS_DSR(NDS_CSS_CARD_Y(126));
            /* Both source door sprites are 41x92. Closed, the left occupies
             * source x=22..62 and the right x=47..87, overlapping by 16 px;
             * their alpha masks form the source's zigzag seam. The cache keeps
             * both complete masks side-by-side (41+41) and these destination
             * positions recreate that overlap. Cutting the cache at x=25 was
             * the old straight-seam defect: it discarded those 16 left-door
             * pixels before the two source masks could interlock. */
            u32 split = (u32)NDS_CSS_DSR(NDS_CSS_CARD_LEN(41));
            u32 strip_w = (u32)NDS_CSS_DSR(NDS_CSS_CARD_LEN(41) * 2);

            (void)ndsUiKitDrawCachedSub(0u, split,
                                        NDS_CSS_DSR(NDS_CSS_CARD_X(start, -19 + (s32)offset)),
                                        dy, clip0, clip1);
            (void)ndsUiKitDrawCachedSub(split, strip_w - split,
                                        NDS_CSS_DSR(NDS_CSS_CARD_X(start, 88 - (s32)offset)),
                                        dy, clip0, clip1);
            gNdsMenuShellCssDoorSlideFrames++;
            #undef NDS_CSS_DSR
        }
    }
}

/* Only the panels whose kind changed, at most `budget` of them, in one call.
 * The VS menu's rule, for the VS menu's measured reason: a panel is 7,738 B of
 * NitroFS and the character select's own worst frame already sits at 71% of
 * the 60 Hz budget before one is read, so the screen ENTRY (a load frame)
 * writes all four and everything after it writes one a frame. A kind change
 * only ever moves one slot anyway; the budget is what keeps that true when the
 * next row adds a reason for two. */
static void ndsMenuShellCssSyncPanels(u32 budget)
{
    NdsUiKitSurfaceId list[NDS_CSS_SLOTS];
    NdsUiKitSurfaceId wanted[NDS_CSS_SLOTS];
    u32 index[NDS_CSS_SLOTS];
    u32 count = 0u;
    u32 i;

    if (sCssOneP != 0u)
    {
        NdsUiKitSurfaceId card = ndsMenuShellCssOnePGateSurface();

        (void)list;
        (void)wanted;
        (void)index;
        (void)count;
        if ((budget != 0u) && (card != sCssPanelSurface[0]) &&
            (ndsUiKitBlitSurfaces(&card, 1u) != FALSE))
        {
            sCssPanelSurface[0] = card;
            gNdsMenuShellCssPanelBlitCount++;
        }
        if ((budget != 0u) && NDS_CSS_BONUS)
        {
            ndsMenuShellCssBonusShowRows();
        }
        else if (budget != 0u)
        {
            ndsMenuShellCssOnePShowRecord();
        }
        return;
    }
    for (i = 0u; i < (u32)NDS_CSS_SLOTS; i++)
    {
        wanted[i] = ndsMenuShellCssGateSurface(i);
        /* P2-1N (3): while a slot is closing (pkind Not, doors mid-slide),
         * the card must stay visible UNDER the sliding halves — the shut NA
         * bake lands only when the doors arrive (the animator blits it and
         * cues the close). The open underlay for an emptying slot is its
         * COM-lut card with no fighter, which is exactly the COM state. */
        if ((sCssPkind[i] == (u8)nFTPlayerKindNot) &&
            (sCssDoorOffset[i] < 41u))
        {
            wanted[i] = ndsMenuShellCssGateSurfaceForState(
                i, NDS_CSS_GATE_COM);
        }
        if ((wanted[i] != sCssPanelSurface[i]) && (count < budget))
        {
            list[count] = wanted[i];
            index[count] = i;
            count++;
        }
    }
    if (count == 0u)
    {
        return;
    }
    if (ndsUiKitBlitSurfaces(list, count) == FALSE)
    {
        return;
    }
    for (i = 0u; i < count; i++)
    {
        sCssPanelSurface[index[i]] = list[i];
    }
    gNdsMenuShellCssPanelBlitCount += count;
}

/* The CPU-level arrows, mnPlayersVSArrowThreadUpdate (:2626). Drawn only for a
 * slot that shows the CP LEVEL row at all, dropped at the ends of the 1..9
 * range exactly as the source ejects the SObj there, and blinking on its own
 * 10-tic cycle. Split from the populate below because the blink is per frame
 * and the row's existence is not. */
static void ndsMenuShellCssDrawArrows(void)
{
    u32 i;

    if (NDS_CSS_BONUS)
    {
        /* The bonus selects have no LEVEL or STOCK row. */
        for (i = 0u; i < (u32)NDS_CSS_SLOTS; i++)
        {
            ndsUiKitHideSprite(NDS_CSS_SPRITE_ARROWL0 + i);
            ndsUiKitHideSprite(NDS_CSS_SPRITE_ARROWR0 + i);
        }
        ndsUiKitHideSprite(NDS_CSS_SPRITE_DIGIT0);
        return;
    }
    if (sCssOneP != 0u)
    {
        ndsMenuShellCssOnePDrawArrows();
        return;
    }
    for (i = 0u; i < (u32)NDS_CSS_SLOTS; i++)
    {
        s32 panel = (s32)(i * 69u);
        u32 is_cpu = ((sCssPkind[i] == (u8)nFTPlayerKindCom) &&
                      (sCssSelected[i] != 0u)) ? TRUE : FALSE;
        u32 is_human_handicap =
            ((sCssPkind[i] == (u8)nFTPlayerKindMan) &&
             (sCssSelected[i] != 0u) &&
             (gNdsMatchConfig.handicap_mode == (u8)nSCBattleHandicapOn)) ?
            TRUE : FALSE;
        u32 value = (is_cpu != FALSE) ? (u32)sCssLevel[i] :
            (u32)sCssHandicap[i];
        u32 live = (((is_cpu != FALSE) || (is_human_handicap != FALSE)) &&
                    (sCssArrowsShown != FALSE) && (sCssTraining == 0u)) ?
            TRUE : FALSE;

        if ((live != FALSE) && (value > NDS_CSS_LEVEL_MIN))
        {
            ndsUiKitSetSprite(NDS_CSS_SPRITE_ARROWL0 + i,
                              NDS_MN_UI_KIT_IMAGE_CSS_ARROW_L,
                              NDS_CSS_DS(NDS_CSS_CARD_X(panel, 25)), NDS_CSS_DS(NDS_CSS_CARD_Y(201)));
        }
        else
        {
            ndsUiKitHideSprite(NDS_CSS_SPRITE_ARROWL0 + i);
        }
        if ((live != FALSE) && (value < NDS_CSS_LEVEL_MAX))
        {
            ndsUiKitSetSprite(NDS_CSS_SPRITE_ARROWR0 + i,
                              NDS_MN_UI_KIT_IMAGE_CSS_ARROW_R,
                              NDS_CSS_DS(NDS_CSS_CARD_X(panel, 79)), NDS_CSS_DS(NDS_CSS_CARD_Y(201)));
        }
        else
        {
            ndsUiKitHideSprite(NDS_CSS_SPRITE_ARROWR0 + i);
        }
    }
}

static void ndsMenuShellCssPopulate(void)
{
    u32 i;

    /* P3: the 2P-4P art loads on the first populate of a wireless lobby --
     * the screen's init runs before the router enters the kit. */
    if ((sCssNet != 0u) && (sCssNetArt == 0u))
    {
        sCssNetArt = (ndsUiKitLoadNetPlayerImages() != FALSE) ? 1u : 0u;
    }

    /* P2-1N (2)+(4): the mode label and BACK ship the source's own sprites
     * (`mnPlayersVSMakeLabels` at (27,24) with the toggle's FFA tint;
     * `llMNPlayersCommonBackButtonSprite` at (244,23)), blitted as baked
     * states over the base. The mode label is a two-state surface so the
     * FFA/Team toggle is a swap; BACK is static and blits once here. The 1P
     * select's title and BACK are in its base; its time and level are
     * states. */
    if (NDS_CSS_BONUS)
    {
        ndsMenuShellCssBonusShowRows();
    }
    else if (sCssOneP != 0u)
    {
        ndsMenuShellCssOnePShowOptions();
    }
    else
    {
        NdsUiKitSurfaceId entry_states[2];

        entry_states[0] = (sCssTraining != 0u) ?
            NDS_MN_UI_KIT_SURFACE_CSS_MODE_TRAINING :
            ((sCssIsTeamBattle != FALSE) ?
                 NDS_MN_UI_KIT_SURFACE_CSS_MODE_TEAM :
                 NDS_MN_UI_KIT_SURFACE_CSS_MODE_FFA);
        entry_states[1] = NDS_MN_UI_KIT_SURFACE_CSS_BACK;
        /* 2026-10-12: once per entry and mode, not per populate: a carried
         * token re-populates on every portrait it crosses, and these two
         * reads (5.8 KB from NitroFS) were ~230K ticks of each such frame. */
        if ((sCssEntryStatesShown != (u32)entry_states[0] + 1u) &&
            (ndsUiKitBlitSurfaces(entry_states, 2u) != FALSE))
        {
            sCssEntryStatesShown = (u32)entry_states[0] + 1u;
            gNdsMenuShellCssPanelBlitCount += 2u;
        }
    }
    if ((sCssTraining == 0u) && !NDS_CSS_ONEP)
    {
        ndsMenuShellCssShowRule();
    }

    /* P2-1L (5): EVERY portrait cell is backdrop art now. The old twelve OBJ
     * slots were dead after that migration and are no longer reserved; P2-1
     * closeout reuses the resulting OAM id space for source cursor/READY art. */

    for (i = 0u; i < (u32)NDS_CSS_SLOTS; i++)
    {
        s32 panel = (s32)(i * 69u);

        /* The player-kind button, mnplayersvs.c:963. The training select
         * has none (its slots' kinds are fixed). */
        if ((sCssTraining != 0u) || NDS_CSS_ONEP)
        {
            ndsUiKitHideSprite(NDS_CSS_SPRITE_KIND0 + i);
        }
        else
        {
            ndsUiKitSetSprite(NDS_CSS_SPRITE_KIND0 + i,
                              ndsMenuShellCssKindImage((u32)sCssPkind[i]),
                              NDS_CSS_DS(NDS_CSS_CARD_X(panel, 64)), NDS_CSS_DS(NDS_CSS_CARD_Y(131)));
        }

        /* P2-1j (e): the panel's own player tag, mnPlayersVSMakePlayerKind
         * (:969) -- a CPU slot draws the shared CP art at `p*69+26` and a
         * human one its own `<N>P` art at `pos_x[p] + p*69 + 22`, both black
         * on the card at y 131. Team Battle does NOT remove this object; its
         * later DL-34 team selector simply draws over it. Keep the tag live
         * and let foreground BG3 reproduce that source ordering.
         *
         * OBJ priority 2 is the owner-ratified CSS stack (2026-08-21): the
         * 3D fighters are BG0 priority 1 and the team selector is BG3
         * priority 0, so a priority-2 tag stays above its BG2 gate card (OBJ
         * wins the tie) while BOTH the fighters and the team plate draw in
         * front of it. The old 0/1 tag stood on the fighter model and over
         * the team plate. The 1P card's tag is in its bake. */
        if (NDS_CSS_ONEP)
        {
            ndsUiKitHideSprite(NDS_CSS_SPRITE_TAG0 + i);
        }
        else if (sCssPkind[i] == (u8)nFTPlayerKindCom)
        {
            (void)ndsUiKitSetSpriteBlend(
                NDS_CSS_SPRITE_TAG0 + i, NDS_MN_UI_KIT_IMAGE_PANEL_CP,
                NDS_CSS_DS(NDS_CSS_CARD_X(panel, 26)), NDS_CSS_DS(NDS_CSS_CARD_Y(131)), 15u, 0u, 2u);
        }
        else if (sCssPkind[i] == (u8)nFTPlayerKindMan)
        {
            /* `pos_x[p] + p*69 + 22` with pos_x {8, 5, 5, 5} (:982). */
            const u32 tag = ndsMenuShellCssHumanArt(NDS_CSS_ART_PANEL, i);

            (void)ndsUiKitSetSpriteBlend(
                NDS_CSS_SPRITE_TAG0 + i, tag,
                NDS_CSS_DS(NDS_CSS_CARD_X(panel,
                           ((tag == NDS_MN_UI_KIT_IMAGE_PANEL_1P) ? 30 : 27))),
                NDS_CSS_DS(NDS_CSS_CARD_Y(131)), 15u, 0u, 2u);
        }
        else
        {
            ndsUiKitHideSprite(NDS_CSS_SPRITE_TAG0 + i);
        }

        /* P2-1k (a)/(b): THE FIGHTER'S NAME AND SERIES EMBLEM ARE SOURCE ART
         * NOW, and they are not drawn here -- they are baked into the gate
         * surface `ndsMenuShellCssSyncPanels` blits, at the source's own sites
         * (emblem (p*69+24,143), name (p*69+22,201) -- mnplayersvs.c:614/:632),
         * because one fighter's pair is 10,240 B of OBJ against 3,456 B free.
         * The font-composed name P2-1e drew at (p*69+22,146) is deleted with
         * this row; that position was the EMBLEM's row, not the name's, so the
         * substitution was in the wrong place as well as the wrong medium. */

        /* CP LEVEL / HANDICAP and its value, mnplayersvs.c:2700-2799. Auto and
         * On both show a human handicap row; only On gets editable arrows
         * (:2792-2798, :2803-2821). CPU level remains editable in every mode. */
        if ((sCssTraining == 0u) && !NDS_CSS_ONEP &&
            (((sCssPkind[i] == (u8)nFTPlayerKindCom) &&
              (sCssSelected[i] != 0u)) ||
             ((sCssPkind[i] == (u8)nFTPlayerKindMan) &&
              (sCssSelected[i] != 0u) &&
              (gNdsMatchConfig.handicap_mode != (u8)nSCBattleHandicapOff))))
        {
            u32 value;

            if (sCssPkind[i] == (u8)nFTPlayerKindCom)
            {
                ndsUiKitSetSprite(NDS_CSS_SPRITE_LEVEL0 + i,
                                  NDS_MN_UI_KIT_IMAGE_CP_LEVEL,
                                  NDS_CSS_DS(NDS_CSS_CARD_X(panel, 34)), NDS_CSS_DS(NDS_CSS_CARD_Y(201)));
                value = (u32)sCssLevel[i];
            }
            else
            {
                ndsUiKitSetSprite(NDS_CSS_SPRITE_LEVEL0 + i,
                                  NDS_MN_UI_KIT_IMAGE_HANDICAP,
                                  NDS_CSS_DS(NDS_CSS_CARD_X(panel, 35)), NDS_CSS_DS(NDS_CSS_CARD_Y(201)));
                value = (u32)sCssHandicap[i];
            }
            /* P2-1j (e): the colon between label and value is its own sprite
             * in the source too (llMNCommonColonSprite at `p*69+61`, y 202,
             * white -- mnplayersvs.c:2740), and it was simply absent. */
            ndsUiKitSetSprite(NDS_CSS_SPRITE_COLON0 + i,
                              NDS_MN_UI_KIT_IMAGE_COLON,
                              NDS_CSS_DS(NDS_CSS_CARD_X(panel, 61)), NDS_CSS_DS(NDS_CSS_CARD_Y(202)));
            (void)ndsUiKitSetNumber(NDS_CSS_SPRITE_DIGIT0 + i, 1u,
                                    (s32)value,
                                    NDS_CSS_DS(NDS_CSS_CARD_X(panel, 67)) +
                                        NDS_UI_KIT_DIGIT_PITCH,
                                    NDS_CSS_DS(NDS_CSS_CARD_Y(200)));
        }
        else
        {
            ndsUiKitHideSprite(NDS_CSS_SPRITE_LEVEL0 + i);
            ndsUiKitHideSprite(NDS_CSS_SPRITE_DIGIT0 + i);
            ndsUiKitHideSprite(NDS_CSS_SPRITE_COLON0 + i);
        }

        /* The token. mnPlayersVSUpdatePuckDisplay hides it for an empty slot
         * (mnplayersvs.c:2310) and draws the CP token for a CPU one. Priority
         * 0 with the cursor (owner, 2026-08-21): the carried badge is part of
         * the interaction layer and must stay readable above the previews. */
        if (sCssPkind[i] == (u8)nFTPlayerKindNot)
        {
            ndsUiKitHideSprite(NDS_CSS_SPRITE_PUCK0 + i);
        }
        else
        {
            (void)ndsUiKitSetSpriteBlend(
                NDS_CSS_SPRITE_PUCK0 + i,
                (sCssPkind[i] == (u8)nFTPlayerKindCom) ?
                    NDS_MN_UI_KIT_IMAGE_PUCK_CP :
                    ndsMenuShellCssHumanArt(NDS_CSS_ART_PUCK, i),
                NDS_CSS_DS((s32)sCssPuckX[i]),
                NDS_CSS_DS((s32)sCssPuckY[i]), 15u, 0u, 0u);
        }
    }
    ndsMenuShellCssDrawArrows();
}
