/* --- Screen: mode select -------------------------------------------------
 *
 * mnmodeselect.c: four entries in the order 1P GAME / VS MODE / OPTION / DATA;
 * UP or RIGHT decrements the cursor and wraps Start -> End, DOWN or LEFT
 * increments and wraps End -> Start (mnmodeselect.c:806/:826); the move cue is
 * MenuScroll2 and the confirm cue is MenuSelect; B returns to the title, and
 * the source spends no cue on it. An entry whose scene is not registered
 * refuses with MenuDenied -- the id the source itself spends on a refused
 * selection -- and one that is registered routes to it (1P GAME -> nSCKind1PMode,
 * OPTION -> nSCKindOption, DATA -> nSCKindData, per mnmodeselect.c:731-768). P2-1i stopped GREYING them: the source draws all four entries
 * identically and distinguishes only the selected one, so a locked colour
 * would be invented state on top of the source's own art. The refusal cue is
 * what says "not built", and it says it at the moment it is true.
 *
 * The five-minute idle return to the title (mnmodeselect.c:702) is attract
 * behaviour and belongs to P2-7; it is deliberately absent rather than
 * stubbed.
 *
 * NO UNLOCK GATE HERE. The source consults no save mask on this screen: all
 * four entries draw identically and route by cursor alone (mnmodeselect.c:731),
 * and the only refusal is an unregistered scene. P2-7 item 9 moves CSS/SSS
 * gating to the save; this screen honours nothing by construction. */
/* The entry count is the BAKE'S, not a second opinion: the four sites the
 * generator recorded and the four bright icons it packed are the same four
 * entries this screen moves between, so a fifth entry has to arrive in both
 * places or in neither. */
#define NDS_MENU_MODE_ENTRIES NDS_MN_UI_KIT_MODE_ENTRY_COUNT
/* The four entries in the source's own order (mnmodeselect.c:731-768 and the
 * four entry sites kNdsUiKitModeEntrySite): 1P GAME, VS MODE, OPTION, DATA. */
#define NDS_MENU_MODE_1P 0u
#define NDS_MENU_MODE_VS 1u
#define NDS_MENU_MODE_OPTION 2u
#define NDS_MENU_MODE_DATA 3u
/* Slot 0 held the invented hand; it now holds the lit entry icon. */
#define NDS_MENU_SPRITE_MODE_LIT NDS_MENU_SPRITE_CURSOR

static u32 sMenuModeCursor;

static void ndsMenuShellPopulateMode(void)
{
    /* P2-1i, owner finding (2). THE WHOLE PLATE IS THE SOURCE'S OWN ART now:
     * the MODE SELECT heading, both decal bars, the SMASH emblem, the four
     * entry icons in their unselected form and the four red English labels are
     * one baked MODE_SELECT surface (mnModeSelectMakeDecals/MakeLabels and
     * mnModeSelectMake<Entry>, mnmodeselect.c:151-579), blitted into BG2 once
     * at entry. The font-composed "MODE SELECT"/"P GAME"/"VS MODE"/"OPTION"/
     * "DATA" rows and the digit-sprite "1" that used to stand in for them are
     * gone.
     *
     * THE ONLY THING THE CURSOR CHANGES is which entry is lit, and the source
     * does that by swapping that one entry's sprite: `...IconSprite` at white
     * with a black env colour when selected (:161-175), `...IconDarkSprite` at
     * grey 0x96 otherwise (:213-223). The dark four are already in the
     * surface, so this is ONE OBJ drawn over the selected one -- at the exact
     * pixels the bake recorded for its dark twin, which is why the light-up is
     * a recolour in place and not a sprite that lands near it.
     *
     * AND THERE IS NO CURSOR ON THIS SCREEN. `mnmodeselect.c` contains no
     * cursor of any kind -- not a hand, not a frame; the light-up IS the
     * selection feedback. P2-1d's hand here was invented, and it is removed
     * rather than kept beside the source's own mechanism. */
    ndsUiKitSetSprite(NDS_MENU_SPRITE_MODE_LIT,
                      NDS_MN_UI_KIT_IMAGE_MODE_ICON_1P + sMenuModeCursor,
                      kNdsUiKitModeEntrySite[sMenuModeCursor][0],
                      kNdsUiKitModeEntrySite[sMenuModeCursor][1]);
}

static void ndsMenuShellUpdateMode(u32 held, u32 taps)
{
    u32 moved = FALSE;

    if (ndsMenuShellDirection(held, taps,
                              NDS_INPUT_UP | NDS_INPUT_RIGHT) != FALSE)
    {
        sMenuModeCursor = (sMenuModeCursor == 0u) ?
            (NDS_MENU_MODE_ENTRIES - 1u) : (sMenuModeCursor - 1u);
        moved = TRUE;
    }
    else if (ndsMenuShellDirection(held, taps,
                                   NDS_INPUT_DOWN | NDS_INPUT_LEFT) != FALSE)
    {
        sMenuModeCursor = (sMenuModeCursor + 1u) % NDS_MENU_MODE_ENTRIES;
        moved = TRUE;
    }
    if (moved != FALSE)
    {
        ndsUiKitSfx(NDS_UI_KIT_SFX_MOVE);
        ndsMenuShellPopulateMode();
    }

    if ((taps & (NDS_INPUT_A | NDS_INPUT_START)) != 0u)
    {
        /* mnmodeselect.c:731-768: A/START on 1P GAME goes to nSCKind1PMode
         * (:733-740), VS MODE to nSCKindVSMode (:742-749), OPTION to
         * nSCKindOption (:751-758), DATA to nSCKindData (:760-767). Each entry
         * routes through ndsMenuShellGoto only when the scene is registered
         * (ndsSceneManagerFind returns non-NULL); an unbuilt scene keeps
         * today's denial with its cue and counter, so it is refused loudly.
         * No flag test here: the registry is the gate. */
        u32 want_kind = 0xffffffffu;

        if (sMenuModeCursor == NDS_MENU_MODE_VS)
        {
            ndsUiKitSfx(NDS_UI_KIT_SFX_CONFIRM);
            ndsMenuShellGoto((u32)nSCKindVSMode);
            return;
        }
        if (sMenuModeCursor == NDS_MENU_MODE_1P)
        {
            want_kind = (u32)nSCKind1PMode;
        }
        else if (sMenuModeCursor == NDS_MENU_MODE_OPTION)
        {
            want_kind = (u32)nSCKindOption;
        }
        else if (sMenuModeCursor == NDS_MENU_MODE_DATA)
        {
            want_kind = (u32)nSCKindData;
        }
        if ((want_kind != 0xffffffffu) &&
            (ndsSceneManagerFind(want_kind) != NULL))
        {
            ndsUiKitSfx(NDS_UI_KIT_SFX_CONFIRM);
            ndsMenuShellGoto(want_kind);
            return;
        }
        ndsUiKitSfx(NDS_UI_KIT_SFX_BACK);
        gNdsMenuShellDeniedCount++;
    }
    else if ((taps & NDS_INPUT_B) != 0u)
    {
        /* P2-1k (g). mnmodeselect.c:774 -- the main menu's B arm stops all BGM
         * before it requests the title. The title's own mnTitleInitVars stop
         * would cover it, but both are the source's and transcribing only one
         * would leave the other's absence looking deliberate. */
        ndsAudioBgmStopAll();
        ndsMenuShellGoto((u32)nSCKindTitle);
    }
}

/* --- Screen: VS mode + rules --------------------------------------------
 *
 * In SSB64 the VS menu IS the rules screen: mnvsmode.c draws four buttons --
 * VS START, RULE, the TIME/STOCK value, and VS OPTIONS -- and UP/DOWN moves
 * between them while LEFT/RIGHT edits the one under the cursor. Transcribed
 * from the source:
 *
 *   cursor  UP wraps Start -> Options, DOWN wraps Options -> Start
 *           (mnvsmode.c:1371/:1412), cue MenuScroll2.
 *   RULE    LEFT/RIGHT walk the rule, CLAMPED at both ends rather than
 *           wrapping (mnvsmode.c:1445/:1476), cue MenuScroll1.
 *   VALUE   time 1..99 then INFINITE; left from 1 lands on INFINITE, right
 *           from 100 lands on 1 (mnvsmode.c:1524/:1568). Stock is stored 0..98
 *           and shown +1. Cue MenuScroll1.
 *   A/START on VS START saves the settings and leaves (mnvsmode.c:1318),
 *           cue MenuSelect.
 *   B       saves the settings and returns to the mode select
 *           (mnvsmode.c:1347), and the source spends no cue.
 *
 * The source's full four-value rule range is live here: TIME, STOCK, TIME TEAM,
 * STOCK TEAM.  P2-2 made Team Battle a real engine mode, so keeping the former
 * P2-1 two-value clamp would now be a behavior difference. VS OPTIONS is the
 * one deliberate narrowing left: it keeps its row and its place in the cursor
 * cycle but refuses, because handicap/damage/item-switch are P2-5/P2-7.
 *
 * The rules this screen commits are the descriptor fields it owns -- rule,
 * time limit, stock count -- written into gNdsMatchConfig and installed with
 * ndsMatchConfigApply. The FIGHTER half of the same descriptor belongs to the
 * character select below (P2-1e), which this screen's VS START now leads to. */
#define NDS_MENU_VS_ENTRIES 4u
#define NDS_MENU_VS_START 0u
#define NDS_MENU_VS_RULE 1u
#define NDS_MENU_VS_VALUE 2u
#define NDS_MENU_VS_OPTIONS 3u

#define NDS_MENU_RULE_TIME 0u
#define NDS_MENU_RULE_STOCK 1u
#define NDS_MENU_RULE_TIME_TEAM 2u
#define NDS_MENU_RULE_STOCK_TEAM 3u
#define NDS_MENU_RULE_MAX NDS_MENU_RULE_STOCK_TEAM

#define NDS_MENU_TIME_MAX 100 /* SCBATTLE_TIMELIMIT_INFINITE */
#define NDS_MENU_STOCK_MAX 98

/* P2-1j, owner finding (b). THE SOURCE HAS NO CURSOR ON THIS SCREEN, and the
 * hand P2-1d put here was an invention that survived P2-1i only because
 * removing it would have left the screen with no selection feedback at all
 * (that row's own note). This is the feedback the source uses:
 * `mnVSModeUpdateButton` (mnvsmode.c:231) recolours the BUTTON the cursor
 * index names, through the IA combiner's two-colour ramp, and the four buttons
 * are the screen's only moving part.
 *
 * WHAT A BUTTON IS: `llMNCommonOptionTab{Left,Middle,Right}Sprite` with the
 * middle tiled over `arg3 * 8` px (:281), arg3 = 17 everywhere here, plus the
 * button's own black text -- 168x29 in the source's frame, 134x23 on the DS.
 * That is larger than a 64x64 bitmap-OBJ cell in both axes' worth of area, and
 * two states of four buttons would be 34,816 B of main OBJ against 16,640 B
 * free, so each button-state is a BG2 surface baked WITH the collage that sits
 * under it (generate_mn_ui_kit.py `under=`). Opaque, so a state change is a
 * whole-row DMA that overwrites the previous state exactly.
 *
 * THE COST IS PAID ON A CURSOR MOVE AND NOWHERE ELSE. Only the two buttons
 * whose state actually changed are re-blitted, so a move reads 12,328 B of
 * NitroFS against the 560,190-tick 60 Hz budget, and a screen holding still
 * reads none. `gNdsMenuShellVsButtonBlitCount` is what keeps that honest.
 *
 * THE ARROWS ARE OBJ, and that is a blink fact rather than a size one: both
 * pairs toggle on a 30-tic cycle while the cursor is on their row
 * (`mnVSModeAnimateRuleArrows` :466, `...TimeStockArrows` :539), and hiding an
 * OBJ is free where re-blitting a surface is a NitroFS read. */
#define NDS_MENU_VS_SURFACE_NONE 0xffffu
/* mnVSModeAnimateRuleArrows' own blink period (30 tics on, 30 off). */
#define NDS_MENU_VS_ARROW_BLINK 30u
/* This screen's layout is now kept in the SOURCE's own 320x240 units, exactly
 * as the character select's and the stage select's are, because every number
 * on it is quoted from mnvsmode.c. The DS screen is exactly 0.8 of that frame
 * on both axes (256/320 = 192/240), so drawing is one multiply. */
#define NDS_MENU_VS_DS(v) (((v) * 4) / 5)

static u32 sMenuVsCursor;
static u32 sMenuVsRule;
static s32 sMenuVsTime;
static s32 sMenuVsStock;
static NdsUiKitSurfaceId sMenuVsButtonSurface[NDS_MENU_VS_ENTRIES];
static u32 sMenuVsArrowsShown;

static u32 ndsMenuShellVsIsTime(void)
{
    return ((sMenuVsRule == NDS_MENU_RULE_TIME) ||
            (sMenuVsRule == NDS_MENU_RULE_TIME_TEAM)) ? TRUE : FALSE;
}

static u32 ndsMenuShellVsIsTeam(void)
{
    return (sMenuVsRule >= NDS_MENU_RULE_TIME_TEAM) ? TRUE : FALSE;
}

static s32 ndsMenuShellVsValue(void)
{
    return (ndsMenuShellVsIsTime() != FALSE) ? sMenuVsTime : (sMenuVsStock + 1);
}

/* Which baked state each button should be showing right now. The pairs are the
 * source's own branches: the rule button carries its VALUE word and the
 * time/stock button its PERIOD word, both of which change with the rule
 * (mnvsmode.c:337/:679), so those two buttons have a TIME and a STOCK bake. */
static NdsUiKitSurfaceId ndsMenuShellVsWantSurface(u32 button)
{
    u32 lit = (sMenuVsCursor == button) ? TRUE : FALSE;

    switch (button)
    {
    case NDS_MENU_VS_START:
        return (NdsUiKitSurfaceId)((lit != FALSE) ?
            NDS_MN_UI_KIT_SURFACE_VS_BTN_START_HI :
            NDS_MN_UI_KIT_SURFACE_VS_BTN_START_NOT);
    case NDS_MENU_VS_RULE:
        if (sMenuVsRule == NDS_MENU_RULE_TIME)
        {
            return (NdsUiKitSurfaceId)((lit != FALSE) ?
                        NDS_MN_UI_KIT_SURFACE_VS_BTN_RULE_TIME_HI :
                        NDS_MN_UI_KIT_SURFACE_VS_BTN_RULE_TIME_NOT);
        }
        if (sMenuVsRule == NDS_MENU_RULE_STOCK)
        {
            return (NdsUiKitSurfaceId)((lit != FALSE) ?
                        NDS_MN_UI_KIT_SURFACE_VS_BTN_RULE_STOCK_HI :
                        NDS_MN_UI_KIT_SURFACE_VS_BTN_RULE_STOCK_NOT);
        }
        if (sMenuVsRule == NDS_MENU_RULE_TIME_TEAM)
        {
            return (NdsUiKitSurfaceId)((lit != FALSE) ?
                        NDS_MN_UI_KIT_SURFACE_VS_BTN_RULE_TIME_TEAM_HI :
                        NDS_MN_UI_KIT_SURFACE_VS_BTN_RULE_TIME_TEAM_NOT);
        }
        return (NdsUiKitSurfaceId)((lit != FALSE) ?
                    NDS_MN_UI_KIT_SURFACE_VS_BTN_RULE_STOCK_TEAM_HI :
                    NDS_MN_UI_KIT_SURFACE_VS_BTN_RULE_STOCK_TEAM_NOT);
    case NDS_MENU_VS_VALUE:
        if (ndsMenuShellVsIsTime() != FALSE)
        {
            return (NdsUiKitSurfaceId)((lit != FALSE) ?
                        NDS_MN_UI_KIT_SURFACE_VS_BTN_TIME_HI :
                        NDS_MN_UI_KIT_SURFACE_VS_BTN_TIME_NOT);
        }
        return (NdsUiKitSurfaceId)((lit != FALSE) ? NDS_MN_UI_KIT_SURFACE_VS_BTN_STOCK_HI :
                                     NDS_MN_UI_KIT_SURFACE_VS_BTN_STOCK_NOT);
    default:
        break;
    }
    return (NdsUiKitSurfaceId)((lit != FALSE) ? NDS_MN_UI_KIT_SURFACE_VS_BTN_OPTIONS_HI :
                                 NDS_MN_UI_KIT_SURFACE_VS_BTN_OPTIONS_NOT);
}

/* Re-blit the buttons whose state changed, AT MOST `budget` of them, in one
 * call so a batch costs one NitroFS open.
 *
 * THE BUDGET IS THE 60 Hz FRAME, and it is measured rather than assumed. The
 * first cut of this blitted every changed button on the frame the change
 * happened, which for a cursor move is TWO -- the one going dark and the one
 * lighting up, 12,328 B of NitroFS -- and the shipping-configuration probe
 * priced that frame at **606,336 ticks against the 560,190-tick single-VBlank
 * budget**, the one 2-VBlank present in 1,811 VS-menu frames
 * (artifacts/verification/2026-08-18_p2-1j-shell-before.txt, MSVB2/MSMAX w2).
 * A screen ENTRY can afford all four because it is already a load frame; a
 * cursor move gets one a frame, so the two-button swap completes in two
 * frames, 33 ms, and no single present carries the burst.
 *
 * The order that falls out is also the right one to watch: the loop below
 * takes buttons in cursor order, so the button that just LIT UP is written
 * first whenever the cursor moved down, and the stale highlight is cleared on
 * the following frame. */
static void ndsMenuShellVsSyncButtons(u32 budget)
{
    NdsUiKitSurfaceId list[NDS_MENU_VS_ENTRIES];
    NdsUiKitSurfaceId wanted[NDS_MENU_VS_ENTRIES];
    u32 index[NDS_MENU_VS_ENTRIES];
    u32 count = 0u;
    u32 i;

    for (i = 0u; i < NDS_MENU_VS_ENTRIES; i++)
    {
        wanted[i] = ndsMenuShellVsWantSurface(i);
        if ((wanted[i] != sMenuVsButtonSurface[i]) && (count < budget))
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
        /* A refused blit leaves the tracker alone so the next frame retries,
         * rather than recording a state the screen is not showing. */
        return;
    }
    for (i = 0u; i < count; i++)
    {
        sMenuVsButtonSurface[index[i]] = list[i];
    }
    gNdsMenuShellVsButtonBlitCount += count;
}

/* mnVSModeMakeTimeStockValue, mnvsmode.c:634. `x` is the RIGHT edge -- the
 * first digit lands at `x - 11` and the rest walk left at that pitch, which is
 * exactly ndsUiKitSetNumber's contract -- and the value is drawn in white at
 * y = 116, or the infinity glyph at (162, 118) for the infinite time limit. */
static void ndsMenuShellVsDrawValue(void)
{
    s32 value = ndsMenuShellVsValue();
    s32 right;
    u32 i;

    for (i = NDS_MENU_SPRITE_NUM0; i < NDS_MENU_SPRITE_NUM2 + 1u; i++)
    {
        ndsUiKitHideSprite(i);
    }
    if (value == NDS_MENU_TIME_MAX)
    {
        ndsUiKitSetSprite(NDS_MENU_SPRITE_NUM0, NDS_MN_UI_KIT_IMAGE_INFINITY,
                          NDS_MENU_VS_DS(162), NDS_MENU_VS_DS(118));
        return;
    }
    if (ndsMenuShellVsIsTime() != FALSE)
    {
        right = (value < 10) ? 185 : 190;
    }
    else
    {
        right = (value < 10) ? 210 : 215;
    }
    (void)ndsUiKitSetNumber(NDS_MENU_SPRITE_NUM0, 2u, value,
                            NDS_MENU_VS_DS(right), NDS_MENU_VS_DS(116));
}

/* The two arrow pairs, at their own source positions. Only the pair belonging
 * to the row the cursor is on is ever shown (`else ... GOBJ_FLAG_HIDDEN` in
 * both animate threads), the rule pair drops its LEFT arrow at the first rule
 * and its RIGHT at the last (:487/:501 -- Time and Stock Team respectively),
 * and the pair blinks on the source's own 30-tic cycle. */
static void ndsMenuShellVsDrawArrows(void)
{
    s32 x_left;
    s32 y;
    u32 show_left;
    u32 show_right;

    if (sMenuVsCursor == NDS_MENU_VS_RULE)
    {
        x_left = 165;
        y = 70;
        show_left = (sMenuVsRule == NDS_MENU_RULE_TIME) ? FALSE : TRUE;
        show_right = (sMenuVsRule == NDS_MENU_RULE_STOCK_TEAM) ? FALSE : TRUE;
    }
    else if (sMenuVsCursor == NDS_MENU_VS_VALUE)
    {
        x_left = (ndsMenuShellVsIsTime() != FALSE) ? 155 : 165;
        y = 109;
        show_left = TRUE;
        show_right = TRUE;
    }
    else
    {
        ndsUiKitHideSprite(NDS_MENU_SPRITE_ARROW_L);
        ndsUiKitHideSprite(NDS_MENU_SPRITE_ARROW_R);
        return;
    }
    if (sMenuVsArrowsShown == FALSE)
    {
        ndsUiKitHideSprite(NDS_MENU_SPRITE_ARROW_L);
        ndsUiKitHideSprite(NDS_MENU_SPRITE_ARROW_R);
        return;
    }
    if (show_left != FALSE)
    {
        ndsUiKitSetSprite(NDS_MENU_SPRITE_ARROW_L,
                          NDS_MN_UI_KIT_IMAGE_VS_ARROW_L,
                          NDS_MENU_VS_DS(x_left), NDS_MENU_VS_DS(y));
    }
    else
    {
        ndsUiKitHideSprite(NDS_MENU_SPRITE_ARROW_L);
    }
    if (show_right != FALSE)
    {
        ndsUiKitSetSprite(NDS_MENU_SPRITE_ARROW_R,
                          NDS_MN_UI_KIT_IMAGE_VS_ARROW_R,
                          NDS_MENU_VS_DS((sMenuVsCursor ==
                                          NDS_MENU_VS_RULE) ? 250 : 230),
                          NDS_MENU_VS_DS(y));
    }
    else
    {
        ndsUiKitHideSprite(NDS_MENU_SPRITE_ARROW_R);
    }
}

/* A STATE CHANGE, not an entry: the OBJ half only. The buttons follow on the
 * next frames through the per-frame sync above, which is what keeps a cursor
 * move inside one 60 Hz present. */
static void ndsMenuShellRefreshVs(void)
{
    ndsMenuShellVsDrawValue();
    ndsMenuShellVsDrawArrows();
}

/* Screen ENTRY. This one runs on a load frame -- ndsMenuShellRun calls it once
 * after the backdrop, before the loop -- so it may write all four buttons. */
static void ndsMenuShellPopulateVs(void)
{
    ndsMenuShellVsSyncButtons(NDS_MENU_VS_ENTRIES);
    ndsMenuShellRefreshVs();
}

/* mnVSModeFuncStartVars: the screen opens on the rules the battle state
 * already carries, so a return trip shows what the last visit committed. */
static void ndsMenuShellVsLoadRules(void)
{
    u32 i;

    for (i = 0u; i < NDS_MENU_VS_ENTRIES; i++)
    {
        /* Nothing is on the screen yet, so every button differs from what the
         * freshly cleared BG2 shows and all four blit on the entry frame. */
        sMenuVsButtonSurface[i] = NDS_MENU_VS_SURFACE_NONE;
    }
    sMenuVsArrowsShown = TRUE;
    sMenuVsCursor = NDS_MENU_VS_START;
    if (gSCManagerTransferBattleState.is_team_battle == FALSE)
    {
        sMenuVsRule = (gSCManagerTransferBattleState.game_rules ==
                       SCBATTLE_GAMERULE_TIME) ? NDS_MENU_RULE_TIME :
                                                 NDS_MENU_RULE_STOCK;
    }
    else
    {
        sMenuVsRule = (gSCManagerTransferBattleState.game_rules ==
                       SCBATTLE_GAMERULE_TIME) ? NDS_MENU_RULE_TIME_TEAM :
                                                 NDS_MENU_RULE_STOCK_TEAM;
    }
    sMenuVsTime = (s32)gSCManagerTransferBattleState.time_limit;
    sMenuVsStock = (s32)gSCManagerTransferBattleState.stocks;
}

/* mnVSModeSaveSettings, through the P2-1a descriptor rather than straight into
 * the battle state: the descriptor is the battle's only input, so the menu
 * writes the fields it owns and ndsMatchConfigApply installs the whole match. */
static void ndsMenuShellVsSaveRules(void)
{
    gNdsMatchConfig.game_rules = (ndsMenuShellVsIsTime() != FALSE) ?
        (u8)SCBATTLE_GAMERULE_TIME : (u8)SCBATTLE_GAMERULE_STOCK;
    gNdsMatchConfig.time_limit = (u8)sMenuVsTime;
    gNdsMatchConfig.stocks = (u8)sMenuVsStock;
    /* mnVSModeSaveSettings, mnvsmode.c:1144-1162: the rule itself carries the
     * FFA/Team bit. CSS exposes the same state through its source mode toggle;
     * whichever screen changes it last becomes the descriptor's one truth. */
    gNdsMatchConfig.is_team_battle =
        (ndsMenuShellVsIsTeam() != FALSE) ? TRUE : FALSE;
    ndsMatchConfigApply(&gNdsMatchConfig);

    gNdsMenuShellCommitCount++;
    gNdsMenuShellCommitRule = (u32)gNdsMatchConfig.game_rules;
    gNdsMenuShellCommitTime = (u32)gNdsMatchConfig.time_limit;
    gNdsMenuShellCommitStocks = (u32)gNdsMatchConfig.stocks;
}

static void ndsMenuShellVsAdjust(s32 direction)
{
    if (sMenuVsCursor == NDS_MENU_VS_RULE)
    {
        /* Clamped, not wrapped -- the source's own shape. */
        if ((direction < 0) && (sMenuVsRule > NDS_MENU_RULE_TIME))
        {
            sMenuVsRule--;
        }
        else if ((direction > 0) && (sMenuVsRule < NDS_MENU_RULE_MAX))
        {
            sMenuVsRule++;
        }
        else
        {
            return;
        }
        ndsUiKitSfx(NDS_UI_KIT_SFX_VALUE);
        ndsMenuShellRefreshVs();
        return;
    }
    if (sMenuVsCursor != NDS_MENU_VS_VALUE)
    {
        return;
    }
    if (ndsMenuShellVsIsTime() != FALSE)
    {
        if (direction < 0)
        {
            sMenuVsTime = (sMenuVsTime == 1) ? NDS_MENU_TIME_MAX :
                                               (sMenuVsTime - 1);
        }
        else
        {
            sMenuVsTime = (sMenuVsTime == NDS_MENU_TIME_MAX) ? 1 :
                                                               (sMenuVsTime + 1);
        }
    }
    else
    {
        if (direction < 0)
        {
            sMenuVsStock = (sMenuVsStock == 0) ? NDS_MENU_STOCK_MAX :
                                                 (sMenuVsStock - 1);
        }
        else
        {
            sMenuVsStock = (sMenuVsStock == NDS_MENU_STOCK_MAX) ? 0 :
                                                                  (sMenuVsStock + 1);
        }
    }
    ndsUiKitSfx(NDS_UI_KIT_SFX_VALUE);
    ndsMenuShellRefreshVs();
}

static void ndsMenuShellUpdateVs(u32 held, u32 taps)
{
    u32 moved = FALSE;
    /* The arrows' own blink, mnvsmode.c:474/:545: a tic counter that toggles
     * the GObj's HIDDEN flag every 30 tics. Recomputed from sMenuTics rather
     * than kept as a countdown, so a screen re-entry restarts the phase where
     * the source's own timer restarts it -- at the top. */
    u32 shown = (((sMenuTics / NDS_MENU_VS_ARROW_BLINK) & 1u) == 0u) ? TRUE :
                                                                       FALSE;

    /* ONE button surface a frame, and never more (see ndsMenuShellVsSyncButtons
     * for the 606,336-tick measurement that put this here). A frame on which
     * nothing changed compares four bytes and returns. */
    ndsMenuShellVsSyncButtons(1u);

    if (shown != sMenuVsArrowsShown)
    {
        sMenuVsArrowsShown = shown;
        ndsMenuShellVsDrawArrows();
    }

    if (ndsMenuShellDirection(held, taps, NDS_INPUT_UP) != FALSE)
    {
        sMenuVsCursor = (sMenuVsCursor == NDS_MENU_VS_START) ?
            (NDS_MENU_VS_ENTRIES - 1u) : (sMenuVsCursor - 1u);
        moved = TRUE;
    }
    else if (ndsMenuShellDirection(held, taps, NDS_INPUT_DOWN) != FALSE)
    {
        sMenuVsCursor = (sMenuVsCursor + 1u) % NDS_MENU_VS_ENTRIES;
        moved = TRUE;
    }
    if (moved != FALSE)
    {
        ndsUiKitSfx(NDS_UI_KIT_SFX_MOVE);
        ndsMenuShellRefreshVs();
    }
    else if (ndsMenuShellDirection(held, taps, NDS_INPUT_LEFT) != FALSE)
    {
        ndsMenuShellVsAdjust(-1);
    }
    else if (ndsMenuShellDirection(held, taps, NDS_INPUT_RIGHT) != FALSE)
    {
        ndsMenuShellVsAdjust(1);
    }

    if ((taps & (NDS_INPUT_A | NDS_INPUT_START)) != 0u)
    {
        if (sMenuVsCursor == NDS_MENU_VS_START)
        {
            ndsUiKitSfx(NDS_UI_KIT_SFX_CONFIRM);
            ndsMenuShellVsSaveRules();
            /* VS START goes to the CHARACTER SELECT, which is where it goes in
             * the source too (mnvsmode.c's VS START leads to nSCKindPlayersVS,
             * and the imported scene's own transition probe asserts exactly
             * that pair). P2-1d sent it straight to the battle because there
             * was no character select yet; P2-1e is that screen, and it is now
             * the only route from this menu into a match.
             *
             * The walk spends its hop here when it is armed, so the P2-1b loop
             * budget still counts TWO hops a loop -- Results -> VS Mode and
             * VS Mode -> here. The CSS -> battle leg is the screen's own
             * transition and deliberately spends no hop, which is what keeps
             * that budget the same shape as when P2-1b defined it. With the
             * walk off this returns FALSE and the screen makes the transition
             * itself; both arms request the same scene. */
            if (ndsSceneWalkAdvance((u32)nSCKindPlayersVS) == FALSE)
            {
                ndsMenuShellGoto((u32)nSCKindPlayersVS);
            }
            else
            {
                u32 slot = gNdsMenuShellTransitionCount %
                           NDS_MENU_SHELL_RING;

                gNdsMenuShellTransitionRing[slot] =
                    ((sMenuScreen & 0xffu) << 8) |
                    ((u32)nSCKindPlayersVS & 0xffu);
                gNdsMenuShellTransitionCount++;
                sMenuNextScene = 0xffffffffu;
                sMenuLeaving = TRUE;
            }
            return;
        }
        /* VS OPTIONS opens its screen (mnvsoptions.c:1284-1297 is the
         * return trip). Rules are committed on the way out for the same
         * reason the B path below commits them: the option screens edit
         * the same descriptor, so leaving stale rules behind would let a
         * later commit overwrite what the player just set. */
        ndsMenuShellVsSaveRules();
        ndsMenuShellGoto((u32)nSCKindVSOptions);
    }
    else if ((taps & NDS_INPUT_B) != 0u)
    {
        ndsMenuShellVsSaveRules();
        ndsMenuShellGoto((u32)nSCKindModeSelect);
    }
}

