/* Native character select, its modes: the 1P and Training options, the
 * menu walk's drive, the wireless guest and team battles, and the scene
 * entry. A slice of nds_menu_shell.c. */

#define NDS_CSS_ONEP_LEFT 2u
/* mnPlayers1PGameCursorProcUpdate's furniture (:2672-2693), in its order:
 * the time arrows, BACK, then LEVEL and STOCK (right arrow first). TRUE when
 * the press was spent, NDS_CSS_ONEP_LEFT when it left the screen. */
static u32 ndsMenuShellCssOnePCheckOptions(void)
{
    s32 step = 0;
    u8 *value = NULL;

    if (NDS_CSS_BONUS)
    {
        /* mnPlayers1PBonusCursorProcUpdate (:2116-2126): the title flips the
         * kind (CheckGameModeInRange :2056; UpdateGameMode :2081 re-makes
         * the title -- announcing the new kind, MakeLabels :901 -- the
         * record and the total row), then BACK. */
        if (ndsMenuShellCssBoxHit(27, 207, 14 + NDS_CSS_HEADER_DY,
                                  35 + NDS_CSS_HEADER_DY) != FALSE)
        {
            sCssBonusKind = (sCssBonusKind == 0u) ? 1u : 0u;
            sCssBonusTotalKey = ndsMenuShellCssBonusTotalKey();
            ndsMenuShellCssBonusShowRows();
            ndsMenuShellCssAnnounceMode();
            return TRUE;
        }
        if (ndsMenuShellCssBoxHit(244, 292, 13 + NDS_CSS_HEADER_DY,
                                  34 + NDS_CSS_HEADER_DY) != FALSE)
        {
            ndsMenuShellCssBack();
            ndsMenuShellCssCue(NDS_CSS_FGM_SCROLL2);
            return NDS_CSS_ONEP_LEFT;
        }
        return FALSE;
    }
    if ((ndsMenuShellCssBoxHit(210, 230, 12 + NDS_CSS_HEADER_DY,
                               35 + NDS_CSS_HEADER_DY) != FALSE) ||
        (ndsMenuShellCssBoxHit(140, 160, 12 + NDS_CSS_HEADER_DY,
                               35 + NDS_CSS_HEADER_DY) != FALSE))
    {
        /* GetNext/PrevTimeValue (:3339-3349): 5 and INFINITE, either way. */
        sCssOnePTime = (sCssOnePTime == 5u) ?
            (u8)SCBATTLE_TIMELIMIT_INFINITE : 5u;
        ndsMenuShellCssOnePShowOptions();
        ndsMenuShellCssCue(NDS_CSS_FGM_SCROLL2);
        return TRUE;
    }
    if (ndsMenuShellCssBoxHit(244, 292, 13 + NDS_CSS_HEADER_DY,
                              34 + NDS_CSS_HEADER_DY) != FALSE)
    {
        ndsMenuShellCssCue(NDS_CSS_FGM_SCROLL2);
        ndsMenuShellCssBack();
        return NDS_CSS_ONEP_LEFT;
    }
    if (ndsMenuShellCssBoxHit(258, 280, 155, 174) != FALSE)
    {
        value = &sCssOnePLevel;
        step = 1;
    }
    else if (ndsMenuShellCssBoxHit(190, 212, 155, 174) != FALSE)
    {
        value = &sCssOnePLevel;
        step = -1;
    }
    else if (ndsMenuShellCssBoxHit(258, 280, 175, 194) != FALSE)
    {
        value = &sCssOnePStock;
        step = 1;
    }
    else if (ndsMenuShellCssBoxHit(190, 212, 175, 194) != FALSE)
    {
        value = &sCssOnePStock;
        step = -1;
    }
    if (value == NULL)
    {
        return FALSE;
    }
    /* VeryEasy..VeryHard and 0..4 stocks, the cue only on a move. */
    if (((step > 0) && (*value < 4u)) || ((step < 0) && (*value > 0u)))
    {
        *value = (u8)((s32)*value + step);
        ndsMenuShellCssCue(NDS_CSS_FGM_SCROLL2);
        ndsMenuShellCssOnePShowOptions();
        ndsMenuShellCssDrawArrows();
    }
    return TRUE;
}

/* mnPlayersVSUpdateCursorNoRecall, mnplayersvs.c:3128. The band is 38..124 --
 * NOT the 36..122 the grid itself uses, which is the source's own two-pixel
 * asymmetry and is kept. */
/* P2-3 (owner, 2026-08-23: "should be able to change skins by selecting the 3d
 * preview").  The DS pad has no C-buttons, which is the input the source spends
 * on alternate costumes (mnplayersvs.c:3369) and the one narrowing P2-1e
 * recorded for this screen.  The owner's replacement is the PREVIEW ITSELF: A
 * on a slot's live fighter cycles that slot to the next costume the source
 * would allow.  Everything behind it -- the four `ftParamGetCostumeCommonID`
 * ids, `mnPlayersVSCheckCostumeUsed`, the shade and `ftParamInitAllParts` --
 * stays the source's, in the imported PlayersVS TU.
 *
 * THE BOX is the gate card's own body, minus the two rows that already own
 * their presses: `mnPlayersVSMakeGate` puts the 66 px card at `p*69+22`, y 126
 * (mnplayersvs.c:1025-1048), the player-kind button occupies y 127..145
 * (:963), and the name/CP-level row starts at y 201 (:632).  The fighter
 * stands between them. */
static u32 ndsMenuShellCssCheckPreviewCostume(void)
{
    u32 slot;

    if (sCssOneP != 0u)
    {
        s32 costume;

        /* The 1P card's body between its tag and its name (MakeGate :880,
         * the name at y 202); the bonus selects' card is 33 px right
         * (mnPlayers1PBonusMakeGate :755). */
        const s32 card_x = NDS_CSS_BONUS ? 58 : 25;

        if ((ndsMenuShellCssBoxHit(card_x, card_x + 82, 146, 200) == FALSE) ||
            (sCssSelected[0] == 0u) || (sCssFkind[0] == (u8)nFTKindNull))
        {
            return FALSE;
        }
        costume = ndsMNPlayersVSPreviewCycleCostume(0u);
        ndsMenuShellCssCue((costume < 0) ? NDS_CSS_FGM_DENIED :
                                           NDS_CSS_FGM_SCROLL2);
        return TRUE;
    }
    for (slot = 0u; slot < (u32)NDS_CSS_SLOTS; slot++)
    {
        s32 panel = (s32)(slot * 69u);
        s32 costume;

        if (ndsMenuShellCssBoxHit(NDS_CSS_CARD_X(panel, 22),
                                  NDS_CSS_CARD_X(panel, 88),
                                  NDS_CSS_CARD_Y(146), NDS_CSS_CARD_Y(200)) == FALSE)
        {
            continue;
        }
        if ((sCssSelected[slot] == 0u) ||
            (sCssFkind[slot] == (u8)nFTKindNull))
        {
            continue;
        }
        /* P3: each human changes only their own costume. A guest's press is
         * a request the host resolves (ndsMenuShellCssNetSync), and the
         * host's snapshot brings the result back. */
        if (ndsMenuShellCssNetHumanOther(slot) != FALSE)
        {
            continue;
        }
        if (ndsMenuShellCssNetGuest() != FALSE)
        {
            if (slot != (u32)NDS_CSS_CURSOR_SLOT)
            {
                continue;
            }
            sCssNetCostumeReq++;
            ndsMenuShellCssCue(NDS_CSS_FGM_SCROLL2);
            return TRUE;
        }
        costume = ndsMNPlayersVSPreviewCycleCostume(slot);
        if (costume < 0)
        {
            /* mnPlayersVSUpdateCostume's own refusal cue (:3291). */
            ndsMenuShellCssCue(NDS_CSS_FGM_DENIED);
        }
        else
        {
            ndsMenuShellCssCue(NDS_CSS_FGM_SCROLL2);
            NDS_DIAG(gNdsMenuShellCssCostumeCycleCount++);
        }
        return TRUE;
    }
    return FALSE;
}

static void ndsMenuShellCssUpdateStatus(void)
{
    if ((sCssCursorY > 124) || (sCssCursorY < 38))
    {
        sCssStatus = NDS_CSS_STATUS_POINTER;
    }

    else if (sCssHeld < 0)
    {
        sCssStatus = NDS_CSS_STATUS_HOVER;
    }
    else
    {
        sCssStatus = NDS_CSS_STATUS_GRAB;
    }
    /* mnPlayers1PGameUpdateCursorNoRecall (:2404): the 1P man's placed token
     * keeps is_selected, so a pointer over it hovers (and can grab it). */
    if (NDS_CSS_ONEP && (sCssStatus == NDS_CSS_STATUS_POINTER) &&
        (sCssSelected[0] != 0u) && (sCssHeld < 0) &&
        (ndsMenuShellCssPuckInRange(0u) != FALSE))
    {
        sCssStatus = NDS_CSS_STATUS_HOVER;
    }
    /* The source has one more branch here -- a POINTER cursor over a placed
     * token becomes a HOVER one -- gated on the CURSOR PLAYER's own
     * `is_selected`, which mnPlayersVSUpdatePuckDisplay clears every frame for
     * a human slot (mnplayersvs.c:2320). It can therefore never run for a human
     * cursor, which is the only kind this hardware has, so it is absent rather
     * than transcribed-and-dead. */
}

#if NDS_MENU_WALK
/* THE WALK'S CANONICAL SNAPSHOT (2026-09-04 rung-7 catch). The CSS cursor moves
 * in PIXELS (4 px a held frame), not cells, so the tour's travel distances are
 * roster-independent -- but its A presses are NOT: A over a portrait DROPS the
 * carried token when that fighter is admitted and is REFUSED (DENIED cue, token
 * stays in hand) when it is locked (ndsMenuShellCssFighterLocked over
 * NDS_CSS_FIGHTER_MASK). The tour was tuned around particular cells being
 * locked (the "locked Donkey negative control"); admitting Link/Pikachu/Yoshi
 * turned refusals into real pickups/drops and the walk committed Samus/Fox
 * instead of Mario/Fox. Re-tuning the press counts per roster would break again
 * at rung 8, so the walk keeps wandering (every grab/drop/refuse/announce
 * counter still proves the screen was exercised) and this snapshot -- run on the
 * START tap, BEFORE the ready/READY computation -- forces the two gate
 * slots back to canonical by DIRECT ASSIGNMENT: slot 0 Mario selected, slot 1
 * Fox selected. Direct assignment is roster-independent because it never touches
 * the cursor or a portrait cell: Mario and Fox are in EVERY NDS_CSS_FIGHTER_MASK
 * arm (rungs 0..7), and admission only ever ADDS bits, so no later rung can
 * re-lock them. Deliberately minimal: pkind/level/teams/mode/slots 2-3 are
 * untouched. Slot kinds stay whatever the wander left (the tour never visits
 * slots 0/1 kind boxes, so they are still the entry Man/Com); slot 1's level
 * stays for the clamp tour below to normalize (eight decrements floor ANY entry
 * to 1, one increment commits exactly 2 -- that property survives because this
 * snapshot does not write levels); slots 2-3 stay under the tour's own net-zero
 * kind toggles. Excluded for the Link-proof and argmax tours, whose descriptors
 * legitimately commit other fighters. Runs only while the walk is driving
 * (gNdsMenuShellWalkLoops < gNdsMenuShellWalkBudget); never after the walk
 * parks and the player owns the pad. */
/* The 1P walk's fighter: a poked content (gNdsP4OnePlayerContent), else the
 * campaign walk's portrait (gNdsMenuShellWalk1PPortrait, the source's cell
 * order), else the VS walk's slot-0 kind. */
static u32 ndsMenuShellCssWalkOnePTarget(void)
{
    const u32 content = (u32)gNdsP4OnePlayerContent;
    const u32 portrait = gNdsMenuShellWalk1PPortrait;

    if ((content != 0u) && (ndsP4Fighter(content) != NULL) &&
        (ndsMenuShellCssFighterLocked(NDS_P4_SEL_BASE + content) == FALSE))
    {
        return NDS_P4_SEL_BASE + content;
    }
    if ((portrait < (u32)NDS_CSS_PORTRAITS) &&
        (ndsMenuShellCssFighterLocked(
             (u32)kNdsCssPortraitFighter[portrait]) == FALSE))
    {
        return (u32)kNdsCssPortraitFighter[portrait];
    }
    /* Metal Mario has no source portrait: the walk names him by his kind. */
    if ((portrait == (u32)nFTKindMMario) &&
        (ndsMenuShellCssFighterLocked((u32)nFTKindMMario) == FALSE))
    {
        return (u32)nFTKindMMario;
    }
    return ndsMenuShellCssWalkTargetKind((u32)0);
}

static void ndsMenuShellCssWalkRestoreGate(void)
{
    u32 walk_kind0 = ndsMenuShellCssWalkTargetKind((u32)0);
    u32 walk_kind1 = ndsMenuShellCssWalkTargetKind((u32)1);

    if (sCssOneP != 0u)
    {
        walk_kind0 = ndsMenuShellCssWalkOnePTarget();
        sCssFkind[(u32)0] = (u8)walk_kind0;
        sCssSelected[(u32)0] = 1u;
        ndsMenuShellCssCenterPuck((u32)0, walk_kind0);
        sCssHeld = -1;
        ndsMenuShellCssUpdateStatus();
        ndsMenuShellCssPopulate();
        return;
    }

    sCssFkind[(u32)0] = (u8)walk_kind0;
    sCssSelected[(u32)0] = 1u;
    sCssFkind[(u32)1] = (u8)walk_kind1;
    sCssSelected[(u32)1] = 1u;
    ndsMenuShellCssCenterPuck((u32)0, walk_kind0);
    ndsMenuShellCssCenterPuck((u32)1, walk_kind1);
    /* A token left in the hand would fail the ready test (a token in the hand
     * is not a choice yet); release it and recompute the cursor state from the
     * cursor position exactly as the source updates it after every action. */
    sCssHeld = -1;
    ndsMenuShellCssUpdateStatus();
    ndsMenuShellCssPopulate();
}

/* THE WALK MUST PUT EVERY ADMITTED FIGHTER ON SCREEN, not just the ones its
 * wander happens to drop on (owner, 2026-09-17: "the CSS walk should go over
 * and select ALL fighters").
 *
 * The tour used to commit whatever its roster-independent pixel wander landed
 * on. Measured on the shipping probe that was Mario, Fox, Luigi and Samus --
 * four of nine admitted kinds (CSSFTRKIND mask=0x23b). Yoshi's preview had been
 * drawing nothing at all since the character select moved onto compact packs,
 * and no scripted run could have caught it, because no scripted run ever
 * selected him. A screen that previews twelve fighters needs a walk that
 * previews twelve fighters; anything less makes "this one is invisible" an
 * owner playtest instead of a counter.
 *
 * DIRECT ASSIGNMENT, for exactly the reason ndsMenuShellCssWalkRestoreGate
 * gives above: an A press over a portrait is refused when that cell is locked,
 * so press counts are roster-dependent and re-tuning them per rung has broken
 * this walk once already. Assigning the slot never touches the cursor or a
 * cell, so it cannot be refused and cannot drift with the roster.
 *
 * THE TOUR IS THE WALK NOW, not a prologue to it (owner, 2026-09-17: "the new
 * walk should replace the old CSS walk, with a faster pace"). The wander was
 * 1,651 presented frames to select four kinds; the tour is 24 tics a kind and
 * then presses its own START, so a nine-fighter rung spends roughly 220 frames
 * to select all nine. That is both the faster pace and better coverage, and it
 * makes a per-rung probe cheap enough to bisect the roster with.
 *
 * 24 tics is the floor plus margin, not a guess: NDS_PLAYERS_VS_PREVIEW_DWELL_TICKS
 * is 13 before a closure load may begin, the residency budget then spends ONE
 * action a tic through retire/load/prepare (measured retry=3 per acquire), and
 * a few frames after that actually submit triangles. Raise it if CSSTOUR ever
 * shows a kind bit set with no triangles for a fighter that is known good --
 * the counter says which, so this number never has to be guessed at again. */
#define NDS_CSS_WALK_TOUR_HOLD_TICS 24u

/* Canonical declaration is include/nds/nds_startup.h:2401. This file is
 * textually included by nds_menu_shell.c, which pulls only the shell's own
 * headers, and the tour is the one thing here that needs to know whether the
 * renderer drew anything. Declaring it rather than dragging nds_startup.h into
 * every shell screen. */
extern volatile u32 gNdsFighterDLAllDrawP0HardwareTriangleCount;

static u8 sCssWalkTourKind;
static u8 sCssWalkTourFinished;
static u32 sCssWalkTourHold;
static u32 sCssWalkTourTriBase;

/* Which kinds the tour parked on, and which of those actually drew. A bit set
 * in Kind but clear in Drew is a fighter whose preview is invisible, which is
 * the whole point of the tour. */
__attribute__((used)) volatile u32 gNdsMenuShellCssWalkTourKindMask;
__attribute__((used)) volatile u32 gNdsMenuShellCssWalkTourDrewMask;
__attribute__((used)) volatile u32 gNdsMenuShellCssWalkTourDoneCount;
/* Non-zero means the tour finished but the restored gate was not ready, so the
 * lap fell back to the old input path. That is a defect in the snapshot, not a
 * tolerable outcome, and it must read 0. */
__attribute__((used)) volatile u32 gNdsMenuShellCssWalkTourNotReadyCount;
/* The selection the random chooser drew in the walk's lap (0xff: never). */
__attribute__((used)) volatile u32 gNdsMenuShellCssWalkChooserSel = 0xffu;
__attribute__((used)) volatile u32
    gNdsMenuShellCssWalkTourTriangles[NDS_CSS_PORTRAITS];

static void ndsMenuShellCssWalkTourSelect(u32 fkind)
{
    sCssFkind[(u32)0] = (u8)fkind;
    sCssSelected[(u32)0] = 1u;
    ndsMenuShellCssCenterPuck((u32)0, fkind);
    /* A token in the hand is not a choice yet, and the tour never grabs one. */
    sCssHeld = -1;
    ndsMenuShellCssUpdateStatus();
    ndsMenuShellCssPopulate();
}

/* TRUE while the tour owns the screen. */
static u32 ndsMenuShellCssWalkTourStep(void)
{
    u32 kind;

    if ((sCssWalkTourFinished != 0u) ||
        (gNdsMenuShellWalkLoops >= gNdsMenuShellWalkBudget) ||
        (sCssStartWait != 0u) || NDS_CSS_ONEP)
    {
        /* The 1P select is walked by its own script (one START). */
        return FALSE;
    }
    /* Close the previous kind before advancing: the triangle delta over its
     * hold is the evidence that the kind drew at all. Sampling a delta rather
     * than the absolute counter keeps this correct whatever else is on screen. */
    if (sCssWalkTourHold != 0u)
    {
        sCssWalkTourHold--;
        if (sCssWalkTourHold != 0u)
        {
            return TRUE;
        }
        kind = (u32)sCssWalkTourKind;
        if (kind < (u32)NDS_CSS_PORTRAITS)
        {
            u32 drawn = gNdsFighterDLAllDrawP0HardwareTriangleCount -
                sCssWalkTourTriBase;

            gNdsMenuShellCssWalkTourTriangles[kind] = drawn;
            if (drawn != 0u)
            {
                gNdsMenuShellCssWalkTourDrewMask |= 1u << kind;
            }
        }
        sCssWalkTourKind++;
    }

    while (((u32)sCssWalkTourKind < (u32)NDS_CSS_PORTRAITS) &&
           (ndsMenuShellCssFighterLocked((u32)sCssWalkTourKind) != FALSE))
    {
        sCssWalkTourKind++;
    }
    if ((u32)sCssWalkTourKind >= (u32)NDS_CSS_PORTRAITS)
    {
        sCssWalkTourFinished = 1u;
        gNdsMenuShellCssWalkTourDoneCount++;
        /* The random chooser, once a lap (owner 2026-10-17): slot 0's token
         * dropped on its cell takes a random unlocked selection
         * (gNdsMenuShellCssWalkChooserSel; the gate restore below puts the
         * walk's own pair back). */
        if (ndsMenuShellCssP4Cell(NDS_P4_SEL_RANDOM) >= 0)
        {
            ndsMenuShellCssCenterPuck((u32)0, NDS_P4_SEL_RANDOM);
            sCssFkind[(u32)0] = (u8)nFTKindNull;
            sCssSelected[(u32)0] = 0u;
            sCssHeld = 0;
            sCssStatus = NDS_CSS_STATUS_GRAB;
            (void)ndsMenuShellCssSelectFighter();
            gNdsMenuShellCssWalkChooserSel = (u32)sCssFkind[(u32)0];
        }
        /* The roster tour consumed the walk's input while it owned the
         * screen. Replay that input from the unchanged initial cursor after
         * restoring the pair: its CPU-arrow leg proves that CSS, not the
         * preset, commits the requested level. Jumping straight to START
         * silently skipped that leg and left Fox at the preset's level 3. */
        ndsMenuShellCssWalkRestoreGate();
        sMenuWalkCursor = 0u;
        sMenuWalkTimer = 0u;
        sMenuWalkHold = 0u;
        sMenuWalkHeld = 0u;
        return TRUE;
    }

    kind = (u32)sCssWalkTourKind;
    gNdsMenuShellCssWalkTourKindMask |= 1u << kind;
    /* Baseline AFTER the select, so the kind's own first frames are counted. */
    ndsMenuShellCssWalkTourSelect(kind);
    sCssWalkTourTriBase = gNdsFighterDLAllDrawP0HardwareTriangleCount;
    sCssWalkTourHold = (u32)NDS_CSS_WALK_TOUR_HOLD_TICS;
    return TRUE;
}

static void ndsMenuShellCssWalkTourReset(void)
{
    sCssWalkTourKind = 0u;
    sCssWalkTourFinished = 0u;
    sCssWalkTourHold = 0u;
    sCssWalkTourTriBase = 0u;
}
#endif

/* --- P3: the character select as the wireless lobby ------------------------
 *
 * Every console runs this screen with its own cursor on its own port. The
 * host's cursor also owns the CPU slots, the kind buttons, the mode and the
 * START; a guest's owns its token and its team. Each frame the host folds the
 * guests' proposals into its slots and publishes all four, and a guest copies
 * the host's slots except its own and proposes its own, cursor included:
 * every other human's cursor and token are drawn where their owner last put
 * them (plan section 10: cosmetic, never part of the match). A guest's costume
 * presses travel as a count the host turns into costume changes, and every
 * console shows the costumes the host resolved. */
static u32 ndsMenuShellCssNetGuest(void)
{
    return ((sCssNet != 0u) && (ndsNetRole() == NDS_NET_ROLE_GUEST)) ? TRUE : FALSE;
}

static u32 ndsMenuShellCssNetHumanOther(u32 slot)
{
    return ((sCssNet != 0u) && (slot != (u32)NDS_CSS_CURSOR_SLOT) &&
            (((ndsNetLobbyHumanMask() >> slot) & 1u) != 0u)) ? TRUE : FALSE;
}

/* Host: a guest still on Results has not proposed since the match. Its pick
 * and costume stand, but it is not ready and the room sees it unchosen. */
static u32 ndsMenuShellCssNetAbsent(u32 slot)
{
    return ((ndsMenuShellCssNetHumanOther(slot) != FALSE) &&
            (ndsNetRole() == NDS_NET_ROLE_HOST) &&
            (ndsNetLobbyHostMemberPresent(slot) == 0u)) ? TRUE : FALSE;
}

static void ndsMenuShellCssNetSlotOut(u32 slot, NdsNetLobbySlot *s)
{
    s->pkind = sCssPkind[slot];
    s->fkind = sCssFkind[slot];
    s->team = sCssTeam[slot];
    s->level = sCssLevel[slot];
    s->handicap = sCssHandicap[slot];
    s->selected = (ndsMenuShellCssNetAbsent(slot) != FALSE) ? 0u :
                                                            sCssSelected[slot];
    s->puck_x = sCssPuckX[slot];
    s->puck_y = sCssPuckY[slot];
    s->costume = (u8)ndsMNPlayersVSPreviewGetAppearance(slot);
    if (slot == (u32)NDS_CSS_CURSOR_SLOT)
    {
        s->cursor = (u8)(sCssStatus + 1u);
        s->cursor_x = (s16)sCssCursorX;
        s->cursor_y = (s16)sCssCursorY;
    }
    else
    {
        s->cursor = sCssNetCursor[slot];
        s->cursor_x = sCssNetCursorX[slot];
        s->cursor_y = sCssNetCursorY[slot];
    }
}

static u32 ndsMenuShellCssNetSlotIn(u32 slot, const NdsNetLobbySlot *s, u32 all)
{
    u32 changed = FALSE;
    const u32 was_selected = sCssSelected[slot];

    if ((all != FALSE) && (sCssPkind[slot] != s->pkind) &&
        (s->pkind <= (u8)nFTPlayerKindNot))
    {
        sCssPkind[slot] = s->pkind;
        changed = TRUE;
    }
    if ((sCssFkind[slot] != s->fkind) &&
        ((s->fkind == (u8)nFTKindNull) ||
         (ndsMenuShellCssFighterLocked((u32)s->fkind) == FALSE)))
    {
        sCssFkind[slot] = s->fkind;
        changed = TRUE;
    }
    if (sCssSelected[slot] != (s->selected ? 1u : 0u))
    {
        sCssSelected[slot] = s->selected ? 1u : 0u;
        changed = TRUE;
    }
    if ((sCssTeam[slot] != s->team) && (s->team <= (u8)nSCBattleTeamIDGreen))
    {
        sCssTeam[slot] = s->team;
        if (sCssIsTeamBattle != FALSE)
        {
            ndsMenuShellCssDrawTeamSelect(slot);
        }
        changed = TRUE;
    }
    if (all != FALSE)
    {
        if ((s->level >= 1u) && (s->level <= 9u))
        {
            sCssLevel[slot] = s->level;
        }
        if ((s->handicap >= 1u) && (s->handicap <= 9u))
        {
            sCssHandicap[slot] = s->handicap;
        }
    }
    /* Token and cursor positions are cosmetic and move every frame:
     * ndsMenuShellCssMove draws them, so they never repopulate the screen. */
    sCssPuckX[slot] = s->puck_x;
    sCssPuckY[slot] = s->puck_y;
    sCssNetCursor[slot] = s->cursor;
    sCssNetCursorX[slot] = s->cursor_x;
    sCssNetCursorY[slot] = s->cursor_y;
    if (all != FALSE)
    {
        /* The host resolved this slot's costume by the source's rules. */
        (void)ndsMNPlayersVSPreviewSetCostume(
            slot, (s32)ndsMenuShellCssSelFkind((u32)s->fkind),
            (s32)s->costume);
    }
    if ((was_selected == 0u) && (sCssSelected[slot] != 0u) &&
        (sCssFkind[slot] != (u8)nFTKindNull))
    {
        ndsMenuShellCssAnnounce(slot);
        ndsMenuShellCssFlashStart(slot);
    }
    return changed;
}

static void ndsMenuShellCssNetSetTeamBattle(u32 team_battle)
{
    NdsUiKitSurfaceId label;

    sCssIsTeamBattle = (team_battle != 0u) ? 1u : 0u;
    label = (sCssIsTeamBattle != FALSE) ? NDS_MN_UI_KIT_SURFACE_CSS_MODE_TEAM :
                                          NDS_MN_UI_KIT_SURFACE_CSS_MODE_FFA;
    {
        /* The host's toggle (the box over the top portrait row): the locked
         * covers ride the same blit and the box draws around them. */
        NdsUiKitSurfaceId box[1u + NDS_P4_CSS_CELLS];
        u32 box_count;

        box[0] = label;
        box_count = ndsMenuShellCssLockedCellCovers(
            box, 1u, (sCssReadyShown == 1u) ? 1u : 0u);
        if (ndsUiKitBlitSurfacesSparing(box, box_count) != FALSE)
        {
            gNdsMenuShellCssPanelBlitCount += box_count;
            if (sCssEntryStatesShown != 0u)
            {
                sCssEntryStatesShown = (u32)label + 1u;
            }
        }
    }
    ndsMenuShellCssSyncPanels((u32)NDS_CSS_SLOTS);
    ndsMenuShellCssSyncTeamSelectAll();
}

static void ndsMenuShellCssNetLeave(void)
{
    ndsNetSessionClose();
    sCssNet = 0u;
    ndsNetUiClear();
    ndsAudioBgmStopAll();
    ndsMenuShellGoto((u32)nSCKindVSMode);
}

/* Returns TRUE when the screen is leaving. */
static u32 ndsMenuShellCssNetSync(void)
{
    NdsNetLobbySlot slots[NDS_CSS_SLOTS];
    u32 changed = FALSE;
    u32 i;

    if (sCssNet == 0u)
    {
        return FALSE;
    }
    if (ndsNetLobbyLost() != 0u)
    {
        ndsMenuShellCssNetLeave();
        return TRUE;
    }
    if (ndsNetRole() == NDS_NET_ROLE_HOST)
    {
        const u32 mask = ndsNetLobbyHumanMask();

        for (i = 1u; i < (u32)NDS_CSS_SLOTS; i++)
        {
            NdsNetLobbySlot proposal;

            if (((mask >> i) & 1u) != 0u)
            {
                if (sCssPkind[i] != (u8)nFTPlayerKindMan)
                {
                    sCssPkind[i] = (u8)nFTPlayerKindMan;
                    sCssFkind[i] = (u8)nFTKindNull;
                    sCssSelected[i] = 0u;
                    sCssPuckX[i] = (s16)NDS_CSS_PUCK_HOME_SLOT_X(i);
                    sCssPuckY[i] = (s16)NDS_CSS_PUCK_HOME_Y;
                    sCssNetCursor[i] = 0u;
                    sCssNetCostumeSeen[i] = 0u;
                    changed = TRUE;
                }
                if (ndsNetLobbyHostTakeProposal(i, &proposal) != 0u)
                {
                    u32 presses =
                        (u8)(proposal.costume - sCssNetCostumeSeen[i]);
                    const u32 moved =
                        ndsMenuShellCssNetSlotIn(i, &proposal, FALSE);

                    changed |= moved;
                    /* That guest's costume presses, each the source's own
                     * costume change (mnPlayersVSFuncRun's C buttons), on the
                     * fighter they were pressed on: while this console's
                     * preview is still bringing that fighter in, they wait
                     * (cycled now, they were refused and lost). */
                    if ((presses != 0u) && (proposal.selected != 0u) &&
                        ((moved != FALSE) ||
                         (ndsMNPlayersVSPreviewSettled(i) == FALSE)))
                    {
                        continue;
                    }
                    sCssNetCostumeSeen[i] = proposal.costume;
                    if (presses > 3u)
                    {
                        presses = 1u; /* a count from an older visit */
                    }
                    while (presses-- != 0u)
                    {
                        (void)ndsMNPlayersVSPreviewCycleCostume(i);
                    }
                }
            }
            else if (sCssPkind[i] == (u8)nFTPlayerKindMan)
            {
                /* That player left the room. */
                sCssNetCursor[i] = 0u;
                sCssPkind[i] = (u8)nFTPlayerKindNot;
                sCssFkind[i] = (u8)nFTKindNull;
                sCssSelected[i] = 0u;
                ndsMenuShellCssCenterPuck(i, (u32)nFTKindNull);
                changed = TRUE;
            }
        }
        for (i = 0u; i < (u32)NDS_CSS_SLOTS; i++)
        {
            ndsMenuShellCssNetSlotOut(i, &slots[i]);
        }
        ndsNetLobbyHostPublish(slots, sCssIsTeamBattle);
    }
    else
    {
        u32 team_battle;
        NdsNetLobbySlot mine;

        if (ndsNetLobbyGuestStartReady() != 0u)
        {
            ndsNetUiClear();
            ndsAudioBgmStopAll();
            ndsMenuShellGoto((u32)nSCKindVSBattle);
            return TRUE;
        }
        if (ndsNetLobbyGuestSnapshot(slots, &team_battle) != 0u)
        {
            for (i = 0u; i < (u32)NDS_CSS_SLOTS; i++)
            {
                if (i != (u32)NDS_CSS_CURSOR_SLOT)
                {
                    changed |= ndsMenuShellCssNetSlotIn(i, &slots[i], TRUE);
                }
                else
                {
                    /* Its own costume is the host's to resolve as well. The
                     * preview's kind is the selection's (a Remix fighter's
                     * parent), as for the other slots: the raw selection
                     * refused every Remix fighter's costume. */
                    (void)ndsMNPlayersVSPreviewSetCostume(
                        i, (s32)ndsMenuShellCssSelFkind((u32)slots[i].fkind),
                        (s32)slots[i].costume);
                }
            }
            if ((team_battle != 0u) != (sCssIsTeamBattle != 0u))
            {
                ndsMenuShellCssNetSetTeamBattle(team_battle);
                changed = TRUE;
            }
            {
                u8 rules[3];

                /* The host's rule bar, which only the host steps. */
                if (ndsNetLobbyGuestRules(rules) != 0u)
                {
                    sCssRuleIsStock =
                        (rules[0] != SCBATTLE_GAMERULE_TIME) ? 1u : 0u;
                    sCssRuleTime = rules[1];
                    sCssRuleStock = rules[2];
                    ndsMenuShellCssShowRule();
                }
            }
        }
        ndsMenuShellCssNetSlotOut((u32)NDS_CSS_CURSOR_SLOT, &mine);
        mine.pkind = (u8)nFTPlayerKindMan;
        mine.costume = sCssNetCostumeReq;
        ndsNetLobbyGuestPropose(&mine);
    }
    if (changed != FALSE)
    {
        ndsMenuShellCssPopulate();
    }
    ndsNetUiLobbyStatus();
    return FALSE;
}

static void ndsMenuShellUpdateCss(u32 held, u32 taps)
{
    /* ONE panel surface a frame, and BEFORE this frame's input, so a kind
     * change lands on the frame AFTER the press rather than on the press's own
     * frame -- which is also the frame the source spends an FGM cue on. See
     * ndsMenuShellCssSyncPanels for the measurement behind the budget. */
    ndsMenuShellCssSyncPanels(1u);
    ndsMenuShellCssStepDoors();
    ndsMenuShellCssStepFlashes();

    if (ndsMenuShellCssNetSync() != FALSE)
    {
        return;
    }
    if ((ndsMenuShellCssNetGuest() != FALSE) &&
        (ndsNetLobbyPhase() == NDS_NET_PHASE_STAGE))
    {
        /* The host is choosing the stage: watch it (plan section 10). */
        ndsMenuShellGoto((u32)nSCKindMaps);
        return;
    }
    if ((ndsMenuShellCssNetGuest() != FALSE) &&
        (ndsNetLobbyPhase() != NDS_NET_PHASE_CSS))
    {
        /* The match is starting: only leaving is allowed. */
        if ((held & NDS_INPUT_B) != 0u)
        {
            if (++sCssBackTics >= (u32)NDS_CSS_BACK_HOLD_TICS)
            {
                ndsMenuShellCssNetLeave();
                return;
            }
        }
        else
        {
            sCssBackTics = 0u;
        }
        ndsMenuShellCssMove();
        return;
    }

    /* mnPlayersVSFuncRun's start delay: once START is accepted the screen stops
     * taking input and counts down before the scene changes. */
    if (sCssStartWait != 0u)
    {
        sCssStartWait--;
        if (sCssStartWait == 0u)
        {
            ndsMenuShellCssCommit();
            if (sCssNet != 0u)
            {
                /* P3: the host always picks the stage, so the start
                 * handshake has one place to run (the stage select). */
                ndsNetLobbyHostSetPhase(NDS_NET_PHASE_STAGE);
                ndsMenuShellGoto((u32)nSCKindMaps);
                return;
            }
            /* mnPlayers1PTrainingFuncRun:2963: training always opens the
             * stage select. */
            if (sCssTraining != 0u)
            {
                ndsMenuShellGoto((u32)nSCKindMaps);
                return;
            }
            /* mnPlayers1PGameFuncRun:3310: the 1P select starts the game. */
            if (NDS_CSS_ONEP)
            {
                ndsMenuShellGoto((u32)nSCKind1PGame);
                return;
            }
            /* mnPlayersVSFuncRun:4493-4508. The source opens Stage Select when
             * enabled and otherwise rolls a different unlocked ground before
             * entering battle. */
            if (gNdsMatchConfig.is_stage_select == FALSE)
            {
                ndsMenuShellCssRandomizeStage();
            }
            ndsMenuShellGoto(
                (gNdsMatchConfig.is_stage_select != FALSE) ?
                    (u32)nSCKindMaps : (u32)nSCKindVSBattle);
        }
        return;
    }

#if NDS_MENU_WALK
    /* The roster tour owns the screen while it runs, which also swallows the
     * walk's START tap -- see the tour's own comment. Everything below is the
     * player's input path and has nothing to drive. */
    if (ndsMenuShellCssWalkTourStep() != FALSE)
    {
        ndsMenuShellCssMove();
        gNdsMenuShellCssCursorX = sCssCursorX;
        gNdsMenuShellCssCursorY = sCssCursorY;
        gNdsMenuShellCssCursorStatus = sCssStatus;
        return;
    }
#endif

    /* 1. The cursor. */
    if ((held & NDS_INPUT_RIGHT) != 0u)
    {
        sCssCursorX += NDS_CSS_CURSOR_STEP;
    }
    if ((held & NDS_INPUT_LEFT) != 0u)
    {
        sCssCursorX -= NDS_CSS_CURSOR_STEP;
    }
    if ((held & NDS_INPUT_DOWN) != 0u)
    {
        sCssCursorY += NDS_CSS_CURSOR_STEP;
    }
    if ((held & NDS_INPUT_UP) != 0u)
    {
        sCssCursorY -= NDS_CSS_CURSOR_STEP;
    }
    if (sCssCursorX < NDS_CSS_CURSOR_X_MIN)
    {
        sCssCursorX = NDS_CSS_CURSOR_X_MIN;
    }
    if (sCssCursorX > NDS_CSS_CURSOR_X_MAX)
    {
        sCssCursorX = NDS_CSS_CURSOR_X_MAX;
    }
    if (sCssCursorY < NDS_CSS_CURSOR_Y_MIN)
    {
        sCssCursorY = NDS_CSS_CURSOR_Y_MIN;
    }
    if (sCssCursorY > NDS_CSS_CURSOR_Y_MAX)
    {
        sCssCursorY = NDS_CSS_CURSOR_Y_MAX;
    }
    /* mnPlayers1PGameAdjustCursor (:2366): the 1P cursor stops at y 205,
     * the source frame's, its card unscaled. */
    if (NDS_CSS_ONEP && (sCssCursorY > 205))
    {
        sCssCursorY = 205;
    }

    /* 2. A carried token rides the cursor, and while it rides it the fighter
     * under it is tracked live -- mnPlayersVSPuckProcUpdate:3542 is what makes
     * the portrait under the token light the name up before you let go. */
    if (sCssHeld >= 0)
    {
        u32 slot = (u32)sCssHeld;
        u32 fkind;

        sCssPuckX[slot] = (s16)(sCssCursorX + NDS_CSS_PUCK_CARRY_DX);
        sCssPuckY[slot] = (s16)(sCssCursorY + NDS_CSS_PUCK_CARRY_DY);
        fkind = ndsMenuShellCssPuckFighterKind(slot);
        if ((u8)fkind != sCssFkind[slot])
        {
            sCssFkind[slot] = (u8)fkind;
            /* mnPlayersVSPuckProcUpdate's Not -> Man promotion: dropping a
             * token on a portrait re-mans an empty slot that has a controller
             * (mnplayersvs.c:3550). */
            if ((sCssPkind[slot] == (u8)nFTPlayerKindNot) &&
                (slot == (u32)NDS_CSS_CURSOR_SLOT) &&
                (fkind != (u32)nFTKindNull))
            {
                sCssPkind[slot] = (u8)nFTPlayerKindMan;
            }
            ndsMenuShellCssPopulate();
        }
    }

    /* 3. A, in mnPlayersVSCursorProcUpdate's own order (mnplayersvs.c:3310):
     * the player-kind buttons first, then a DROP, then a GRAB, then the frame
     * furniture. Each returns TRUE only if it consumed the press. */
    if ((taps & NDS_INPUT_A) != 0u)
    {
        if (sCssOneP != 0u)
        {
            /* mnPlayers1PGameCursorProcUpdate (:2662): a drop, a grab, then
             * its furniture; the preview's costume press (the VS screen's,
             * for the C-buttons the DS lacks) last. */
            if ((ndsMenuShellCssSelectFighter() == FALSE) &&
                (ndsMenuShellCssCheckGrab() == FALSE))
            {
                const u32 spent = ndsMenuShellCssOnePCheckOptions();

                if (spent == NDS_CSS_ONEP_LEFT)
                {
                    return;
                }
                if (spent == FALSE)
                {
                    (void)ndsMenuShellCssCheckPreviewCostume();
                }
            }
        }
        else
        if (ndsMenuShellCssCheckKindButton() == FALSE)
        {
            if (ndsMenuShellCssSelectFighter() == FALSE)
            {
                if (ndsMenuShellCssCheckGrab() == FALSE)
                {
                    /* mnPlayersVSCursorProcUpdate checks the rule arrows, game
                     * mode, BACK, then the team selectors in exactly this
                     * order (:3318-3367). */
                    if (ndsMenuShellCssRuleArrow() != FALSE)
                    {
                        return;
                    }
                    if ((ndsMenuShellCssBoxHit(27, 137, 14 + NDS_CSS_HEADER_DY,
                                               35 + NDS_CSS_HEADER_DY) != FALSE) &&
                        (ndsMenuShellCssNetGuest() == FALSE) &&
                        (sCssTraining == 0u))
                    {
                        NdsUiKitSurfaceId label;

                        sCssIsTeamBattle = (sCssIsTeamBattle != FALSE) ?
                            0u : 1u;
                        label = (sCssIsTeamBattle != FALSE) ?
                            NDS_MN_UI_KIT_SURFACE_CSS_MODE_TEAM :
                            NDS_MN_UI_KIT_SURFACE_CSS_MODE_FFA;
                        {
                            /* The mode label box (27,24,113,22) crosses the
                             * top portrait row and its state surface
                             * restores the unlocked base there: the locked
                             * covers ride the same blit and the box draws
                             * around them (as the READY band's,
                             * ndsMenuShellCssShowReady). */
                            NdsUiKitSurfaceId box[1u + NDS_P4_CSS_CELLS];
                            u32 box_count;

                            box[0] = label;
                            box_count = ndsMenuShellCssLockedCellCovers(
                                box, 1u, (sCssReadyShown == 1u) ? 1u : 0u);
                            if (ndsUiKitBlitSurfacesSparing(box, box_count) !=
                                FALSE)
                            {
                                gNdsMenuShellCssPanelBlitCount += box_count;
                                if (sCssEntryStatesShown != 0u)
                                {
                                    sCssEntryStatesShown = (u32)label + 1u;
                                }
                            }
                        }
                        /* mnPlayersVSUpdateGameMode:1918 stops every current FGM
                         * before the scroll and new mode announcement. */
                        ndsAudioFgmStopAll();
                        ndsMenuShellCssCue(NDS_CSS_FGM_SCROLL2);
                        ndsMenuShellCssAnnounceMode();
                        /* UpdateGateAll and Make/DestroyTeamSelectAll are same-
                         * frame source operations. Gate palettes are BG2; the
                         * source DL-34 selectors are foreground BG3. */
                        ndsMenuShellCssSyncPanels((u32)NDS_CSS_SLOTS);
                        ndsMenuShellCssSyncTeamSelectAll();
                        gNdsMenuShellCssModeToggleCount++;
                        return;
                    }
                    if (ndsMenuShellCssBoxHit(244, 292, 13 + NDS_CSS_HEADER_DY,
                                              34 + NDS_CSS_HEADER_DY) != FALSE)
                    {
                        ndsMenuShellCssCue(NDS_CSS_FGM_SCROLL2);
                        if (sCssNet != 0u)
                        {
                            ndsMenuShellCssNetLeave();
                            return;
                        }
                        ndsMenuShellCssBack();
                        return;
                    }
                    if (ndsMenuShellCssCheckTeamSelect() == FALSE)
                    {
                        if (ndsMenuShellCssCheckLevelArrows() == FALSE)
                        {
                            (void)ndsMenuShellCssCheckPreviewCostume();
                        }
                    }
                }
            }
        }
    }

    if (NDS_CSS_ONEP)
    {
        /* mnPlayers1PGameCursorProcUpdate's tail (:2735-2744): B recalls a
         * placed token; with none placed, a B tap past the screen's first
         * ten tics goes back (DetectBack, :2647). */
        if ((taps & NDS_INPUT_B) != 0u)
        {
            if ((sCssSelected[0] != 0u) && (sCssHeld < 0))
            {
                ndsMenuShellCssRecall();
            }
            else if (sMenuTics >= 10u)
            {
                ndsMenuShellCssBack();
                return;
            }
        }
    }
    else
    {
        /* 4. B recalls your own placed token (mnplayersvs.c:3411). */
        if ((taps & NDS_INPUT_B) != 0u)
        {
            ndsMenuShellCssRecall();
        }

        /* 5. mnPlayersVSDetectBack: B HELD for forty tics leaves the
         * screen. */
        if ((held & NDS_INPUT_B) != 0u)
        {
            sCssBackTics++;
            if (sCssBackTics >= (u32)NDS_CSS_BACK_HOLD_TICS)
            {
                if (sCssNet != 0u)
                {
                    ndsMenuShellCssNetLeave();
                    return;
                }
                ndsMenuShellCssBack();
                return;
            }
        }
        else
        {
            sCssBackTics = 0u;
        }
    }

    /* 6. The cursor's own state, last, exactly as the source updates it after
     * every action rather than before them. */
    ndsMenuShellCssUpdateStatus();

    /* 7. READY TO FIGHT, and the START it invites. */
    {
        u32 ready;
        u32 lit;
        /* The walk's START placed its fighter this frame (below). */
        u32 walk_placed = FALSE;
#if NDS_MENU_WALK
        /* Walk-only canonical snapshot: wander first, restore, then run the
         * ordinary ready/START path below on the restored state. See the
         * snapshot's own comment for why this is roster-independent.
         * "Walk-only" means the walk is DRIVING, not merely compiled in:
         * the published shell ROM is a walk build, and until 2026-09-07 this
         * ran on the player's own START too, so a picked Yoshi reached the
         * stage select as Mario (owner report, docs/BUGS.md). The walk's
         * budget test is ndsMenuShellWalkTap's own. */
        if (((taps & NDS_INPUT_START) != 0u) &&
            (gNdsMenuShellWalkLoops < gNdsMenuShellWalkBudget) &&
            (sMenuTics > (u32)NDS_CSS_START_ARM_TICS))
        {
            ndsMenuShellCssWalkRestoreGate();
            walk_placed = TRUE;
        }
#endif
        (void)walk_placed;
        ready = ndsMenuShellCssCheckReady();

        if (ready != FALSE)
        {
            sCssReadyBlink++;
            if (sCssReadyBlink >= (u32)NDS_CSS_READY_BLINK)
            {
                sCssReadyBlink = 0u;
            }
            lit = (sCssReadyBlink < (u32)NDS_CSS_READY_LIT) ? TRUE : FALSE;
        }
        else
        {
            sCssReadyBlink = 0u;
            lit = FALSE;
        }
        if (lit != sCssReadyShown)
        {
            ndsMenuShellCssShowReady(lit);
        }

        if (NDS_CSS_BONUS)
        {
            /* mnPlayers1PBonusFuncRun (:2711-2723): a pick arms a 140-tic
             * wait (SelectFighterPuck :288) that a recall or a re-grab drops;
             * START, or the wait running out, starts the kind's board at
             * once -- no START delay, no refusal cue. The source has no
             * cheer here either; the owner asked for VS's (BUGS.md CSS row,
             * 2026-10-19), so the board starts with it. */
            if (sCssSelected[0] == 0u)
            {
                sCssBonusWait = 0u;
            }
            else if (sCssBonusPicked == 0u)
            {
                sCssBonusWait = 140u;
            }
            sCssBonusPicked = sCssSelected[0];
            /* The walk's START is its pick: the board starts on the wait,
             * the select showing the fighter meanwhile. */
            if ((sCssBonusWait != 0u) &&
                ((--sCssBonusWait == 0u) ||
                 (((taps & NDS_INPUT_START) != 0u) && (walk_placed == FALSE))))
            {
                sCssBonusWait = 0u;
                ndsMenuShellCssCheer();
                ndsMenuShellCssCommit();
                gNdsMenuShellCssStartCount++;
                ndsMenuShellGoto((u32)nSCKind1PBonusStage);
                return;
            }
        }
        else
        if (((taps & NDS_INPUT_START) != 0u) &&
            (sMenuTics > (u32)NDS_CSS_START_ARM_TICS) &&
            (ndsMenuShellCssNetGuest() == FALSE))
        {
            if (ready != FALSE)
            {
                ndsMenuShellCssCheer();
                ndsMenuShellCssIdleSlotsNot();
                sCssStartWait = (u32)NDS_CSS_START_WAIT;
                gNdsMenuShellCssStartCount++;
                ndsMenuShellCssPopulate();
            }
            else
            {
                ndsMenuShellCssCue(NDS_CSS_FGM_DENIED);
                gNdsMenuShellCssStartDeniedCount++;
            }
        }
    }

    ndsMenuShellCssMove();
    gNdsMenuShellCssCursorX = sCssCursorX;
    gNdsMenuShellCssCursorY = sCssCursorY;
    gNdsMenuShellCssCursorStatus = sCssStatus;
}

/* mnPlayers1PTrainingInitVars (mnplayers1ptraining.c:3058-3112): the man
 * keeps the scene's training_man fighter (none on a first visit, so the token
 * opens in the hand), the COM keeps training_com's or, with none, takes a
 * random unlocked one; the other two slots are shut. Each slot's content
 * rides gNdsP4TrainingContent. No team battle in training. */
static void ndsMenuShellCssInitTraining(void)
{
    const u32 man = (u32)NDS_CSS_CURSOR_SLOT;
    const u32 com = (man == 0u) ? 1u : 0u;
    u32 i;

    sCssIsTeamBattle = 0u;
    for (i = 0u; i < (u32)NDS_CSS_SLOTS; i++)
    {
        u32 fkind = (u32)nFTKindNull;
        u32 slot_index = (i == man) ? 0u : 1u;

        if ((i != man) && (i != com))
        {
            sCssPkind[i] = (u8)nFTPlayerKindNot;
            sCssFkind[i] = (u8)nFTKindNull;
            sCssSelected[i] = 0u;
            ndsMenuShellCssCenterPuck(i, (u32)nFTKindNull);
            continue;
        }
        sCssPkind[i] = (i == man) ? (u8)nFTPlayerKindMan : (u8)nFTPlayerKindCom;
        fkind = (i == man) ? (u32)gSCManagerSceneData.training_man_fkind :
                             (u32)gSCManagerSceneData.training_com_fkind;
        /* mnPlayers1PTrainingInitPlayer :3003 / :3098. */
        sCssEntryCostume[i] = (i == man) ?
            (s16)gSCManagerSceneData.training_man_costume :
            (s16)gSCManagerSceneData.training_com_costume;
        if ((fkind != (u32)nFTKindNull) &&
            (ndsP4Fighter(gNdsP4TrainingContent[slot_index]) != NULL))
        {
            fkind = NDS_P4_SEL_BASE + (u32)gNdsP4TrainingContent[slot_index];
        }
        if ((fkind != (u32)nFTKindNull) &&
            (ndsMenuShellCssFighterLocked(fkind) != FALSE))
        {
            fkind = (u32)nFTKindNull;
        }
        if ((i == com) && (fkind == (u32)nFTKindNull))
        {
            fkind = ndsMenuShellCssP4RandomSel();
            sCssEntryCostume[i] = -1;
        }
        sCssFkind[i] = (u8)fkind;
        sCssLevel[i] = 3u;
        sCssTeam[i] = (u8)((i == man) ? nSCBattleTeamIDRed : nSCBattleTeamIDBlue);
        sCssSelected[i] = (fkind != (u32)nFTKindNull) ? 1u : 0u;
        ndsMenuShellCssCenterPuck(i, fkind);
    }
}

/* mnPlayers1PGameInitVars / InitPlayer (:3387/:3351): the man keeps the
 * scene's fighter -- a Remix selection when the content beside that kind is
 * one -- or, with none (a first visit), opens with the token in the hand;
 * the time, level and stocks are the scene's and the save's; the other three
 * slots are shut. */
static void ndsMenuShellCssInitOneP(void)
{
    const NDSP4Fighter *p4 = ndsP4Fighter(gNdsP4OnePlayerContent);
    u32 fkind = (u32)gSCManagerSceneData.fkind;
    u32 i;

    sCssIsTeamBattle = 0u;
    if ((fkind != (u32)nFTKindNull) && (p4 != NULL) &&
        ((u32)p4->parent_kind == fkind))
    {
        fkind = NDS_P4_SEL_BASE + (u32)gNdsP4OnePlayerContent;
    }
    else if ((fkind == (u32)nFTKindMario) && (gNdsP7OnePlayerMetal != 0u))
    {
        fkind = (u32)nFTKindMMario;
    }
    /* C3: the wide rows' records, once (ndsMenuShellCssOnePShowRecord). */
    ndsP7OnePRecordsVisit(GMCOMMON_FIGHTERS_PLAYABLE_NUM,
                          NDS_P7_SEL_COUNT - GMCOMMON_FIGHTERS_PLAYABLE_NUM,
                          ndsMenuShellCssOnePCacheRecord, NULL);
    sCssOnePRecordShown = 0u;
    if ((fkind != (u32)nFTKindNull) &&
        (ndsMenuShellCssFighterLocked(fkind) != FALSE))
    {
        fkind = (u32)nFTKindNull;
    }
    for (i = 0u; i < (u32)NDS_CSS_SLOTS; i++)
    {
        sCssPkind[i] = (i == 0u) ? (u8)nFTPlayerKindMan : (u8)nFTPlayerKindNot;
        sCssFkind[i] = (i == 0u) ? (u8)fkind : (u8)nFTKindNull;
        sCssSelected[i] = ((i == 0u) && (fkind != (u32)nFTKindNull)) ? 1u : 0u;
        /* mnPlayers1PGameInitPlayer :3358. */
        sCssEntryCostume[i] = (i == 0u) ? (s16)gSCManagerSceneData.costume : -1;
        sCssTeam[i] = (u8)nSCBattleTeamIDRed;
        sCssLevel[i] = 3u;
        ndsMenuShellCssCenterPuck(i, (u32)sCssFkind[i]);
    }
    sCssOnePLevel = (u8)((gSCManagerBackupData.spgame_difficulty > 4u) ? 4u :
                         gSCManagerBackupData.spgame_difficulty);
    sCssOnePStock = (u8)((gSCManagerBackupData.spgame_stock_count > 4u) ? 4u :
                         gSCManagerBackupData.spgame_stock_count);
    sCssOnePTime = (gSCManagerSceneData.spgame_time_limit == 5u) ?
        5u : (u8)SCBATTLE_TIMELIMIT_INFINITE;
    sCssOnePShown = 0u;
}

/* mnPlayers1PBonusInitVars / InitPlayer / ResetPlayer (:2755/:2743/:1533):
 * the kind is the scene's, the man's token opens in the hand (no fighter is
 * kept from a last visit), the other three slots are shut; the records as
 * the 1P select reads them, and the total row's value. */
static void ndsMenuShellCssInitBonus(void)
{
    u32 i;

    sCssIsTeamBattle = 0u;
    sCssBonusKind = ((u32)gSCManagerSceneData.scene_curr ==
                     (u32)nSCKind1PBonus1Players) ? 0u : 1u;
    sCssBonusWait = 0u;
    sCssBonusPicked = 0u;
    sCssBonusShown = 0u;
    sCssBonusRecordShown = 0u;
    sCssBonusTotalShown = 0u;
    ndsP7OnePRecordsVisit(GMCOMMON_FIGHTERS_PLAYABLE_NUM,
                          NDS_P7_SEL_COUNT - GMCOMMON_FIGHTERS_PLAYABLE_NUM,
                          ndsMenuShellCssOnePCacheRecord, NULL);
    sCssBonusTotalKey = ndsMenuShellCssBonusTotalKey();
    for (i = 0u; i < (u32)NDS_CSS_SLOTS; i++)
    {
        sCssPkind[i] = (i == 0u) ? (u8)nFTPlayerKindMan : (u8)nFTPlayerKindNot;
        sCssFkind[i] = (u8)nFTKindNull;
        sCssSelected[i] = 0u;
        sCssTeam[i] = (u8)nSCBattleTeamIDRed;
        sCssLevel[i] = 3u;
        ndsMenuShellCssCenterPuck(i, (u32)nFTKindNull);
    }
}

/* mnPlayersVSInitVars + mnPlayersVSInitPlayer + mnPlayersVSInitSlot,
 * mnplayersvs.c:4670/:4579/:4698, reading the P2-1a descriptor instead of
 * gSCManagerTransferBattleState: the descriptor is what the battle consumes, so
 * it is also what the screen that fills it should open from. */
static void ndsMenuShellCssInit(void)
{
    u32 i;

#if NDS_MENU_WALK
    /* Per entry, not per boot: a second visit tours again, and the masks are
     * cumulative so a kind that drew on either visit reads as drawn. */
    ndsMenuShellCssWalkTourReset();
#endif
    sCssNet = ndsNetInSession();
    sCssNetArt = 0u;
    sCssNetCostumeReq = 0u;
    for (i = 0u; i < (u32)NDS_CSS_SLOTS; i++)
    {
        sCssNetCursor[i] = 0u;
        sCssNetCostumeSeen[i] = 0u;
    }
    if (sCssNet != 0u)
    {
        ndsNetLobbyMatchOver();
    }
    else
    {
        ndsNetLockstepDisarm();
    }
    ndsNetUiClear();
    sCssTraining = ((sCssNet == 0u) &&
                    ((u32)gSCManagerSceneData.scene_curr ==
                     (u32)nSCKindPlayers1PTraining)) ? 1u : 0u;
    sCssCursorSlot = (sCssNet != 0u) ? ndsNetLobbyLocalPort() :
        (((sCssTraining != 0u) && (gSCManagerSceneData.player < 2u)) ?
             (u32)gSCManagerSceneData.player : 0u);
    /* mnPlayersVSMakeCursor seats player p at x 40 + 68p. */
    sCssCursorX = NDS_CSS_CURSOR_HOME_X + 68 * (s32)sCssCursorSlot;
    sCssCursorY = NDS_CSS_CURSOR_HOME_Y;
    sCssOneP = 0u;
    if (sCssNet == 0u)
    {
        const u32 scene = (u32)gSCManagerSceneData.scene_curr;

        if (scene == (u32)nSCKind1PGamePlayers)
        {
            sCssOneP = NDS_CSS_ONEP_GAME;
        }
        else if ((scene == (u32)nSCKind1PBonus1Players) ||
                 (scene == (u32)nSCKind1PBonus2Players))
        {
            sCssOneP = NDS_CSS_ONEP_BONUS;
        }
    }
    if (sCssOneP != 0u)
    {
        /* mnPlayers1PGameMakeCursor (:2948), mnPlayers1PBonusMakeCursor
         * (:2366: x 80). */
        sCssCursorX = NDS_CSS_BONUS ? 80 : 60;
        sCssCursorY = 170;
    }
    sCssStatus = NDS_CSS_STATUS_POINTER;
    sCssHeld = -1;
    sCssRegrabTic = 0u;
    sCssBackTics = 0u;
    sCssStartWait = 0u;
    sCssReadyBlink = 0u;
    sCssReadyShown = 0xffffffffu; /* forces the first ShowReady to publish */
    sCssArrowsShown = TRUE;
    memset(sCssFlashShown, 0, sizeof(sCssFlashShown));
    /* P2-1N (4): seeded from the transfer state exactly as the source seeds
     * sMNPlayersVSIsTeamBattle on scene entry (mnplayersvs.c:4679). */
    sCssIsTeamBattle = (gSCManagerTransferBattleState.is_team_battle != 0) ?
        1u : 0u;
    /* The rule bar, as mnPlayersVSInitVars seeds it (:4676-4681). */
    sCssRuleIsStock = (gSCManagerTransferBattleState.game_rules !=
                       SCBATTLE_GAMERULE_TIME) ? 1u : 0u;
    sCssRuleTime = (u8)gSCManagerTransferBattleState.time_limit;
    sCssRuleStock = (u8)gSCManagerTransferBattleState.stocks;

    for (i = 0u; i < (u32)NDS_CSS_SLOTS; i++)
    {
        /* Nothing of this screen is on BG2 yet, so every panel differs and all
         * four blit on the entry frame -- the same tracker reset the VS menu's
         * buttons take in ndsMenuShellVsLoadRules. */
        sCssPanelSurface[i] = NDS_MENU_VS_SURFACE_NONE;
        sCssFlashRemain[i] = 0u;
        sCssFlashVisible[i] = 0u;
        sCssFlashKind[i] = (u8)NDS_CSS_FLASH_KIND_NONE;
    }

    for (i = 0u; i < (u32)NDS_CSS_SLOTS; i++)
    {
        const NdsMatchFighterConfig *cfg = &gNdsMatchConfig.fighters[i];

        sCssPkind[i] = cfg->pkind;
        sCssFkind[i] = cfg->fkind;
        if (ndsP4Fighter(cfg->p4_content) != NULL)
        {
            sCssFkind[i] = (u8)(NDS_P4_SEL_BASE + cfg->p4_content);
        }
        /* mnPlayersVSInitPlayer :4626: the transfer state's costume. */
        sCssEntryCostume[i] = (s16)cfg->costume;
        sCssLevel[i] = (cfg->level < 1u) ? 1u : ((cfg->level > 9u) ? 9u :
                                                 cfg->level);
        sCssHandicap[i] = ndsMatchConfigClampPublishedHandicap(cfg->handicap);
        sCssTeam[i] = (cfg->team <= (u8)nSCBattleTeamIDGreen) ? cfg->team :
            (u8)((i < 2u) ? nSCBattleTeamIDRed : nSCBattleTeamIDBlue);
        /* mnPlayersVSUpdateGate, mnplayersvs.c:4193: a slot with no controller
         * cannot be HUMAN. Only slot 0 has one here, so any other slot the
         * descriptor calls human arrives as empty -- which is the same
         * correction the source makes, one frame later, in its gate pass. */
        if ((sCssPkind[i] == (u8)nFTPlayerKindMan) &&
            (i != (u32)NDS_CSS_CURSOR_SLOT) &&
            (ndsMenuShellCssNetHumanOther(i) == FALSE))
        {
            sCssPkind[i] = (u8)nFTPlayerKindNot;
            sCssFkind[i] = (u8)nFTKindNull;
        }
        if (sCssPkind[i] == (u8)nFTPlayerKindNot)
        {
            sCssFkind[i] = (u8)nFTKindNull;
        }
        if (ndsMenuShellCssFighterLocked((u32)sCssFkind[i]) != FALSE)
        {
            sCssFkind[i] = (u8)nFTKindNull;
        }
        sCssSelected[i] = (sCssFkind[i] != (u8)nFTKindNull) ? 1u : 0u;
        ndsMenuShellCssCenterPuck(i, (u32)sCssFkind[i]);
        gNdsMenuShellCssCommitSlot[i] = 0u;
    }
    if (sCssTraining != 0u)
    {
        ndsMenuShellCssInitTraining();
    }
    if (NDS_CSS_BONUS)
    {
        ndsMenuShellCssInitBonus();
    }
    else if (sCssOneP != 0u)
    {
        ndsMenuShellCssInitOneP();
    }
    /* The 1P card stands its fighter where the 1P select does (the bonus
     * selects' where theirs do). */
    ndsMNPlayersVSPreviewSetOnePlayerLayout((u32)sCssOneP);
    /* P3: this console's own port is always a human in a room. */
    if ((sCssNet != 0u) &&
        (sCssPkind[NDS_CSS_CURSOR_SLOT] != (u8)nFTPlayerKindMan))
    {
        sCssPkind[NDS_CSS_CURSOR_SLOT] = (u8)nFTPlayerKindMan;
        sCssFkind[NDS_CSS_CURSOR_SLOT] = (u8)nFTKindNull;
        sCssSelected[NDS_CSS_CURSOR_SLOT] = 0u;
    }
    /* A human slot with no fighter yet holds its own token, so the screen opens
     * with the token already in the cursor's hand (mnplayersvs.c:4604). */
    if ((sCssPkind[NDS_CSS_CURSOR_SLOT] == (u8)nFTPlayerKindMan) &&
        (sCssFkind[NDS_CSS_CURSOR_SLOT] == (u8)nFTKindNull))
    {
        sCssHeld = NDS_CSS_CURSOR_SLOT;
        sCssPuckX[NDS_CSS_CURSOR_SLOT] =
            (s16)(sCssCursorX + NDS_CSS_PUCK_CARRY_DX);
        sCssPuckY[NDS_CSS_CURSOR_SLOT] =
            (s16)(sCssCursorY + NDS_CSS_PUCK_CARRY_DY);
    }
    /* A kept fighter opens in its committed costume (mnPlayersVSInitSlot's
     * mnPlayersVSMakeFighter). */
    for (i = 0u; i < (u32)NDS_CSS_SLOTS; i++)
    {
        ndsMNPlayersVSPreviewSetEntryCostume(
            i, (u32)sCssFkind[i],
            (sCssSelected[i] != 0u) ? (s32)sCssEntryCostume[i] : -1);
    }
    /* P2-1N (3): mnPlayersVSMakeGate seeds EVERY slot at door_offset=41
     * (closed), then mnPlayersVSShutterProcUpdate opens each occupied slot by
     * 2/tic. Do not shortcut occupied slots to zero on entry: that deletes the
     * source's entrance shutter animation. The CSS_DOORS strip is cached here
     * for those slides. */
    for (i = 0u; i < (u32)NDS_CSS_SLOTS; i++)
    {
        sCssDoorOffset[i] = 41u;
    }
    if (NDS_CSS_ONEP)
    {
        /* No shutter on the 1P card: no door strip, no card copies (the
         * arena keeps them for the preview). */
        sCssUnderlayBlock = NULL;
    }
    else
    {
        (void)ndsUiKitCacheSurface((u32)NDS_MN_UI_KIT_SURFACE_CSS_DOORS);
        ndsMenuShellCssUnderlayReserve();
    }
    ndsMenuShellCssVramUnderlayReset();
}

#if NDS_MENU_WALK
/* nds_menu_shell_core.c's walk: the 1P select takes its own script. */
static u32 ndsMenuShellCssWalkOneP(void)
{
    return NDS_CSS_ONEP ? TRUE : FALSE;
}
#endif

/* --- M03/M04 loader witnesses --------------------------------------------
 *
 * These counters are written in battleship_mnplayersvs.c and reloc_preview_pack.c
 * and read nowhere else, and `--gc-sections` discards a section nothing reads
 * even when the symbol is `used`: that is how four freshly added witnesses all
 * resolved to one stale word on 2026-09-21. The copy below comes from surviving
 * per-frame code, so the linker keeps their sections, and the array it lands in
 * is itself a single place a capture can read the whole transaction from.
 *
 * Row meaning:
 *   0 loader steps run          1 largest single step span, bytes
 *   2 largest one-update span   3 stale completions rejected
 *   4 warm commits              5 preparation stages run
 *   6 pack steps                7 largest pack step span, bytes
 *   8 pack publications         9 pack transactions cancelled */
extern volatile u32 gNdsPlayersVSPreviewServiceStepCount;
extern volatile u32 gNdsPlayersVSPreviewServiceByteMax;
extern volatile u32 gNdsPlayersVSPreviewServiceUpdateByteMax;
extern volatile u32 gNdsPlayersVSPreviewStaleCommitRejectCount;
extern volatile u32 gNdsPlayersVSPreviewWarmCommitCount;
extern volatile u32 gNdsPlayersVSPreviewPrepareStageCount;
extern volatile u32 gNdsPreviewPackStepCount;
extern volatile u32 gNdsPreviewPackStepByteMax;
extern volatile u32 gNdsPreviewPackStageCommitCount;
extern volatile u32 gNdsPreviewPackStageCancelCount;
__attribute__((used)) volatile u32 gNdsMenuShellCssLoaderWitness[10];

static void ndsMenuShellCssPublishLoaderWitness(void)
{
    gNdsMenuShellCssLoaderWitness[0] = gNdsPlayersVSPreviewServiceStepCount;
    gNdsMenuShellCssLoaderWitness[1] = gNdsPlayersVSPreviewServiceByteMax;
    gNdsMenuShellCssLoaderWitness[2] = gNdsPlayersVSPreviewServiceUpdateByteMax;
    gNdsMenuShellCssLoaderWitness[3] =
        gNdsPlayersVSPreviewStaleCommitRejectCount;
    gNdsMenuShellCssLoaderWitness[4] = gNdsPlayersVSPreviewWarmCommitCount;
    gNdsMenuShellCssLoaderWitness[5] = gNdsPlayersVSPreviewPrepareStageCount;
    gNdsMenuShellCssLoaderWitness[6] = gNdsPreviewPackStepCount;
    gNdsMenuShellCssLoaderWitness[7] = gNdsPreviewPackStepByteMax;
    gNdsMenuShellCssLoaderWitness[8] = gNdsPreviewPackStageCommitCount;
    gNdsMenuShellCssLoaderWitness[9] = gNdsPreviewPackStageCancelCount;
}

static void ndsMenuShellCssSyncPreviews(void)
{
    u32 slot;

    /* The source's mode/team state must be visible to mnPlayersVSUpdateFighter
     * BEFORE any slot can ask mnPlayersVSGetFreeCostume/GetShade. This call also
     * performs UpdateGameMode/TeamSelect's in-place material changes for
     * fighters whose kind did not change. */
    ndsMNPlayersVSPreviewSyncRules(
        (sCssIsTeamBattle != FALSE) ? TRUE : FALSE,
        sCssTeam, (u32)NDS_CSS_SLOTS);
    /* Source mnPlayersVSInitSlotAll explicitly initializes 0,1,2,3 and every
     * player-kind / puck update can rebuild that slot's fighter. Mirror all
     * four here now that the native owner path is instance-safe for mirrors. */
    for (slot = 0u; slot < (u32)NDS_CSS_SLOTS; slot++)
    {
        ndsMNPlayersVSPreviewSetContent(
            slot, ndsMenuShellCssSelContent((u32)sCssFkind[slot]));
        ndsMNPlayersVSPreviewSync(
            slot, (s32)sCssPkind[slot],
            (s32)((sCssFkind[slot] == (u8)nFTKindNull) ?
                  (u32)nFTKindNull :
                  ndsMenuShellCssSelFkind((u32)sCssFkind[slot])),
            (sCssSelected[slot] != 0u) ? TRUE : FALSE);
    }
    ndsMenuShellCssPublishLoaderWitness();
}

/* Screen ENTRY, on a load frame: the only place all four panels are written at
 * once. Every later change goes through ndsMenuShellCssMove's one-a-frame
 * sync. */
static void ndsMenuShellPopulateCssScreen(void)
{
    /* The base surfaces were just blitted under the mode label, BACK and the
     * rule bar. */
    sCssEntryStatesShown = 0u;
    sCssRuleShown = 0u;
    sCssOnePShown = 0u;
    ndsMenuShellCssSyncPanels((u32)NDS_CSS_SLOTS);
    ndsMenuShellCssPopulate();
    ndsMenuShellCssSyncTeamSelectAll();
    ndsMenuShellCssShowReady(FALSE);
    /* After every surface that restores the unlocked base: the save's own
     * locked cells go on last, at entry, in one blit. */
    ndsMenuShellCssSyncLockedCells();
    ndsMenuShellCssMove();
}
