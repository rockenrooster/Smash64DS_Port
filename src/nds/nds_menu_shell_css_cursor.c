/* Native character select, the cursors: movement, the flashes, grabbing and
 * placing a token, the kind it applies, recall and the commit. A slice of
 * nds_menu_shell.c. */

/* Per frame, and deliberately separate from the populate above: the cursor and
 * a carried token move every frame while the screen's CONTENT changes only on
 * an action, so composing text every frame would show up as a climbing
 * gNdsUiKitTextComposeCount for a screen that is not changing. */
static void ndsMenuShellCssMove(void)
{
    u32 image = NDS_MN_UI_KIT_IMAGE_CSS_CURSOR_POINT;
    s32 tag_dx = 7;
    s32 tag_dy = 15;
    u32 i;
    /* mnPlayersVSArrowThreadUpdate's 10-tic toggle, recomputed from the
     * screen's tic count for the same reason the VS menu's 30-tic one is. */
    u32 shown = (((sMenuTics / NDS_CSS_ARROW_BLINK) & 1u) == 0u) ? TRUE : FALSE;

    if (shown != sCssArrowsShown)
    {
        sCssArrowsShown = shown;
        ndsMenuShellCssDrawArrows();
    }

    if (sCssStatus == NDS_CSS_STATUS_GRAB)
    {
        image = NDS_MN_UI_KIT_IMAGE_CSS_CURSOR_GRAB;
        tag_dx = 9;
        tag_dy = 10;
    }
    else if (sCssStatus == NDS_CSS_STATUS_HOVER)
    {
        image = NDS_MN_UI_KIT_IMAGE_CSS_CURSOR_HOVER;
        tag_dx = 9;
        tag_dy = 15;
    }
    /* mnPlayersVSUpdateCursor appends the player's gradient tag after the hand
     * at offsets {7,15}/{9,10}/{9,15}.  Only the 1P cursor is reachable on a
     * single DS, and its source PRIM/ENV IA gradient is baked into this image.
     * Owner ruling 2026-08-21: the hand cursor and the carried token are the
     * INTERACTION layer and stay topmost (priority 0), above the preview
     * fighters and every panel furniture; the source's cursor camera (25) sits
     * below the fighter camera (30), but a hand hidden behind a preview model
     * is unreadable on the DS's smaller panel. */
    (void)ndsUiKitSetSpriteBlend(
        NDS_CSS_SPRITE_CURSOR_TAG,
        ndsMenuShellCssHumanArt(NDS_CSS_ART_CURSOR, (u32)NDS_CSS_CURSOR_SLOT),
        NDS_CSS_DS(sCssCursorX + tag_dx), NDS_CSS_DS(sCssCursorY + tag_dy),
        15u, 0u, 0u);
    (void)ndsUiKitSetSpriteBlend(
        NDS_CSS_SPRITE_CURSOR, image, NDS_CSS_DS(sCssCursorX),
        NDS_CSS_DS(sCssCursorY), 15u, 0u, 0u);
    for (i = 0u; i < (u32)NDS_CSS_SLOTS; i++)
    {
        if (sCssPkind[i] != (u8)nFTPlayerKindNot)
        {
            ndsUiKitMoveSprite(NDS_CSS_SPRITE_PUCK0 + i,
                               NDS_CSS_DS((s32)sCssPuckX[i]),
                               NDS_CSS_DS((s32)sCssPuckY[i]));
        }
    }
    /* P3: every other human's cursor where its owner last put it, with the
     * same hand states and tag offsets as this console's own. */
    for (i = 0u; i < (u32)NDS_CSS_SLOTS; i++)
    {
        static const u8 kHand[3] = {
            NDS_MN_UI_KIT_IMAGE_CSS_CURSOR_POINT,
            NDS_MN_UI_KIT_IMAGE_CSS_CURSOR_GRAB,
            NDS_MN_UI_KIT_IMAGE_CSS_CURSOR_HOVER,
        };
        static const s8 kTagDx[3] = { 7, 9, 9 };
        static const s8 kTagDy[3] = { 15, 10, 15 };
        const u32 state = sCssNetCursor[i];
        const u32 tag = NDS_CSS_SPRITE_NET_CURSOR0 + 2u * i;

        if ((ndsMenuShellCssNetHumanOther(i) == FALSE) || (state == 0u) ||
            (state > 3u))
        {
            ndsUiKitHideSprite(tag);
            ndsUiKitHideSprite(tag + 1u);
            continue;
        }
        (void)ndsUiKitSetSpriteBlend(
            tag, ndsMenuShellCssHumanArt(NDS_CSS_ART_CURSOR, i),
            NDS_CSS_DS((s32)sCssNetCursorX[i] + kTagDx[state - 1u]),
            NDS_CSS_DS((s32)sCssNetCursorY[i] + kTagDy[state - 1u]),
            15u, 0u, 0u);
        (void)ndsUiKitSetSpriteBlend(
            tag + 1u, kHand[state - 1u], NDS_CSS_DS((s32)sCssNetCursorX[i]),
            NDS_CSS_DS((s32)sCssNetCursorY[i]), 15u, 0u, 0u);
    }
}

/* --- Portrait flash ------------------------------------------------------- */

#define NDS_CSS_FLASH_TICS 16u
#define NDS_CSS_FLASH_KIND_NONE 0xffu
#define NDS_CSS_FLASH_KIND_MARIO 0u
#define NDS_CSS_FLASH_KIND_FOX 1u
#define NDS_CSS_FLASH_KIND_LUIGI 2u
#define NDS_CSS_FLASH_KIND_LINK 3u
/* The P4 grid flashes every cell: the kind IS the cell, and the bake emits
 * CSS_FLASH_CELL<n>_{OFF,ON}_READY{0,1} contiguously, four per cell. */
#define NDS_CSS_FLASH_KIND_COUNT NDS_P4_CSS_CELLS
_Static_assert(NDS_MN_UI_KIT_SURFACE_CSS_FLASH_CELL01_OFF_READY0 ==
                   NDS_MN_UI_KIT_SURFACE_CSS_FLASH_CELL00_OFF_READY0 + 4u,
               "P4 flash surfaces must stay four per cell");

static u32 ndsMenuShellCssFlashKindFromFighter(u32 fkind)
{
    s32 cell = ndsMenuShellCssP4Cell(fkind);

    return (cell < 0) ? NDS_CSS_FLASH_KIND_NONE : (u32)cell;
    if (fkind == (u32)nFTKindMario)
    {
        return NDS_CSS_FLASH_KIND_MARIO;
    }
    if (fkind == (u32)nFTKindFox)
    {
        return NDS_CSS_FLASH_KIND_FOX;
    }
#if NDS_LUIGI
    if (fkind == (u32)nFTKindLuigi)
    {
        return NDS_CSS_FLASH_KIND_LUIGI;
    }
#endif
#if NDS_LINK
    if (fkind == (u32)nFTKindLink)
    {
        return NDS_CSS_FLASH_KIND_LINK;
    }
#endif
    return NDS_CSS_FLASH_KIND_NONE;
}

static NdsUiKitSurfaceId ndsMenuShellCssFlashSurface(u32 kind, u32 visible)
{
    u32 ready = (sCssReadyShown == 1u) ? 1u : 0u;

    return (NdsUiKitSurfaceId)(NDS_MN_UI_KIT_SURFACE_CSS_FLASH_CELL00_OFF_READY0 +
                               (kind * 4u) + (ready * 2u) +
                               ((visible != FALSE) ? 1u : 0u));
}

static u32 ndsMenuShellCssFlashAggregate(u32 kind)
{
    u32 slot;

    for (slot = 0u; slot < (u32)NDS_CSS_SLOTS; slot++)
    {
        if ((sCssFlashRemain[slot] != 0u) &&
            (sCssFlashVisible[slot] != 0u) &&
            ((u32)sCssFlashKind[slot] == kind))
        {
            return TRUE;
        }
    }
    return FALSE;
}

static void ndsMenuShellCssDrawFlashKind(u32 kind, u32 visible)
{
    NdsUiKitSurfaceId surface;

    if (kind >= NDS_CSS_FLASH_KIND_COUNT)
    {
        return;
    }
    surface = ndsMenuShellCssFlashSurface(kind, visible);
    if (ndsUiKitBlitSurfaces(&surface, 1u) != FALSE)
    {
        gNdsMenuShellCssPanelBlitCount++;
    }
}

static void ndsMenuShellCssSyncFlashKind(u32 kind)
{
    u32 visible;

    if (kind >= NDS_CSS_FLASH_KIND_COUNT)
    {
        return;
    }
    visible = ndsMenuShellCssFlashAggregate(kind);
    if (visible == (u32)sCssFlashShown[kind])
    {
        return;
    }
    ndsMenuShellCssDrawFlashKind(kind, visible);
    sCssFlashShown[kind] = (u8)((visible != FALSE) ? 1u : 0u);
}

static void ndsMenuShellCssFlashStart(u32 slot)
{
    u32 old_kind;
    u32 kind;

    if (slot >= (u32)NDS_CSS_SLOTS)
    {
        return;
    }
    kind = ndsMenuShellCssFlashKindFromFighter((u32)sCssFkind[slot]);
    if (kind >= NDS_CSS_FLASH_KIND_COUNT)
    {
        return;
    }
    old_kind = (u32)sCssFlashKind[slot];
    sCssFlashRemain[slot] = (u8)NDS_CSS_FLASH_TICS;
    sCssFlashVisible[slot] = 1u;
    sCssFlashKind[slot] = (u8)kind;
    if ((old_kind < NDS_CSS_FLASH_KIND_COUNT) && (old_kind != kind))
    {
        ndsMenuShellCssSyncFlashKind(old_kind);
    }
    ndsMenuShellCssSyncFlashKind(kind);
}

static void ndsMenuShellCssFlashCancel(u32 slot)
{
    u32 kind;

    if (slot >= (u32)NDS_CSS_SLOTS)
    {
        return;
    }
    kind = (u32)sCssFlashKind[slot];
    sCssFlashRemain[slot] = 0u;
    sCssFlashVisible[slot] = 0u;
    sCssFlashKind[slot] = (u8)NDS_CSS_FLASH_KIND_NONE;
    if (kind < NDS_CSS_FLASH_KIND_COUNT)
    {
        ndsMenuShellCssSyncFlashKind(kind);
    }
}

static void ndsMenuShellCssStepFlashes(void)
{
    /* The flashing kinds this tic, one per slot at most: the P4 grid has more
     * cells than a u32 has bits. A kind listed twice syncs once in effect
     * (ndsMenuShellCssSyncFlashKind redraws only on a change). */
    u8 dirty[NDS_CSS_SLOTS];
    u32 dirty_count = 0u;
    u32 slot;
    u32 kind;

    for (slot = 0u; slot < (u32)NDS_CSS_SLOTS; slot++)
    {
        if (sCssFlashRemain[slot] == 0u)
        {
            continue;
        }
        kind = (u32)sCssFlashKind[slot];
        if (kind < NDS_CSS_FLASH_KIND_COUNT)
        {
            dirty[dirty_count++] = (u8)kind;
        }
        sCssFlashRemain[slot]--;
        if (sCssFlashRemain[slot] == 0u)
        {
            sCssFlashVisible[slot] = 0u;
            sCssFlashKind[slot] = (u8)NDS_CSS_FLASH_KIND_NONE;
        }
        else
        {
            /* mnPlayersVSPortraitFlashThreadUpdate toggles hidden every tic. */
            sCssFlashVisible[slot] = (sCssFlashVisible[slot] != 0u) ? 0u : 1u;
        }
    }
    for (slot = 0u; slot < dirty_count; slot++)
    {
        ndsMenuShellCssSyncFlashKind((u32)dirty[slot]);
    }
}

static void ndsMenuShellCssRedrawVisibleFlashes(void)
{
    u32 kind;

    for (kind = 0u; kind < NDS_CSS_FLASH_KIND_COUNT; kind++)
    {
        if (ndsMenuShellCssFlashAggregate(kind) != FALSE)
        {
            ndsMenuShellCssDrawFlashKind(kind, TRUE);
            sCssFlashShown[kind] = 1u;
        }
    }
}

/* --- Ready ---------------------------------------------------------------- */

/* mnPlayersVSGetReadyPlayerCount + mnPlayersVSCheckSingleTeam +
 * mnPlayersVSCheckNoPuckOnPortraitAll + mnPlayersVSCheckReady,
 * mnplayersvs.c:4268/:4314/:4337/:4352. */
static u32 ndsMenuShellCssCheckReady(void)
{
    u32 ready = 0u;
    s32 ready_team = -1;
    u32 has_other_team = FALSE;
    u32 i;

    if (NDS_CSS_ONEP)
    {
        /* mnPlayers1PGameCheckReady (:3250): the fighter is placed. */
        return (sCssSelected[0] != 0u) ? TRUE : FALSE;
    }
    for (i = 0u; i < (u32)NDS_CSS_SLOTS; i++)
    {
        if ((sCssPkind[i] != (u8)nFTPlayerKindNot) && (sCssSelected[i] != 0u))
        {
            ready++;
            if (ready_team < 0)
            {
                ready_team = (s32)sCssTeam[i];
            }
            else if (ready_team != (s32)sCssTeam[i])
            {
                has_other_team = TRUE;
            }
        }
    }
    if (ready < 2u)
    {
        return FALSE;
    }
    if (sCssNet != 0u)
    {
        const u32 mask = ndsNetLobbyHumanMask();

        for (i = 0u; i < (u32)NDS_CSS_SLOTS; i++)
        {
            if (((mask >> i) & 1u) != 0u && sCssSelected[i] == 0u)
            {
                return FALSE;
            }
        }
    }
    if ((sCssIsTeamBattle != FALSE) && (has_other_team == FALSE))
    {
        return FALSE;
    }
    /* A token in the hand is not a choice yet. */
    return (sCssStatus == NDS_CSS_STATUS_GRAB) ? FALSE : TRUE;
}

static void ndsMenuShellCssShowReady(u32 lit)
{
    /* mnPlayersVSMakeReady owns TWO GObjs on the same 40/30 blink but on
     * DIFFERENT display links. The banner + READY TO FIGHT is link 38 through
     * the priority-10 Ready camera; REGION_US PRESS + START is link 28 through
     * the priority-50 player-kind camera. Keep the existing BG2 state swap for
     * the flash-overlap restoration, then mirror only the link-38 band onto
     * transparent foreground BG3 so it also has the source's depth over OBJ. */
    NdsUiKitSurfaceId state = (lit != FALSE) ?
        NDS_MN_UI_KIT_SURFACE_CSS_READY_ON :
        NDS_MN_UI_KIT_SURFACE_CSS_READY_OFF;
    /* The READY band (0,71,320,22) crosses the bottom portrait row, and its
     * state surface restores the unlocked base there: the covers of what the
     * save locks ride the same blit, and the band draws around them
     * (ndsUiKitBlitSurfacesSparing). Re-covering after the foreground and
     * flash blits below (several NitroFS opens) let the display scan the band
     * out between; drawing the covers right behind the band in one open still
     * left the slices the covers stream in -- frames on a slow card -- so a
     * blink edge showed a locked fighter (owner BUGS.md, CSS, 2026-10-18 and
     * again 2026-10-20). */
    NdsUiKitSurfaceId band[1u + NDS_P4_CSS_CELLS];
    u32 band_count;

    band[0] = state;
    if (sCssOneP != 0u)
    {
        /* The 1P select's band, over its own base (the same grid). */
        band[0] = (lit != FALSE) ? NDS_MN_UI_KIT_SURFACE_ONEP_P4_READY_ON :
                                   NDS_MN_UI_KIT_SURFACE_ONEP_P4_READY_OFF;
    }
    band_count = ndsMenuShellCssLockedCellCovers(band, 1u,
                                                 (lit != FALSE) ? 1u : 0u);
    if (ndsUiKitBlitSurfacesSparing(band, band_count) != FALSE)
    {
        gNdsMenuShellCssPanelBlitCount += band_count;
    }
    ndsUiKitClearForegroundRect(NDS_CSS_DS(0), NDS_CSS_DS(71),
                                (u32)NDS_CSS_DS(320),
                                18u); /* baked 22-source-row box -> 18 DS rows */
    if (lit != FALSE)
    {
        NdsUiKitSurfaceId foreground =
            NDS_MN_UI_KIT_SURFACE_CSS_READY_FOREGROUND;

        if (ndsUiKitBlitForegroundSurfaces(&foreground, 1u) != FALSE)
        {
            gNdsMenuShellCssPanelBlitCount++;
        }
        /* P3: only the host's START starts the match, so a guest sees
         * READY TO FIGHT without the prompt. */
        if (ndsMenuShellCssNetGuest() == FALSE)
        {
            /* The 1P select's PRESS START sits under its options
             * (mnPlayers1PGameMakeReady :3152, y 219). */
            const s32 press_y = NDS_CSS_ONEP ? 219 : NDS_CSS_PRESS_Y;

            ndsUiKitSetSprite(NDS_CSS_SPRITE_READY_PRESS,
                              NDS_MN_UI_KIT_IMAGE_CSS_READY_PRESS,
                              NDS_CSS_DS(133), NDS_CSS_DS(press_y));
            ndsUiKitSetSprite(NDS_CSS_SPRITE_READY_START,
                              NDS_MN_UI_KIT_IMAGE_CSS_READY_START,
                              NDS_CSS_DS(162), NDS_CSS_DS(press_y));
        }
    }
    else
    {
        ndsUiKitHideSprite(NDS_CSS_SPRITE_READY_PRESS);
        ndsUiKitHideSprite(NDS_CSS_SPRITE_READY_START);
    }
    sCssReadyShown = (lit != FALSE) ? 1u : 0u;
    /* READY is source camera priority 10, in front of the priority-73 portrait
     * flash.  Its state surface overwrites the overlap, so re-apply only live
     * flash rectangles using the matching READY-aware baked variant. */
    ndsMenuShellCssRedrawVisibleFlashes();
}

/* --- Actions -------------------------------------------------------------- */

/* mnPlayersVSSetCursorGrab, mnplayersvs.c:2932. */
static void ndsMenuShellCssGrab(u32 slot)
{
    /* mnPlayersVSSetCursorGrab destroys this slot's existing portrait flash. */
    ndsMenuShellCssFlashCancel(slot);
    sCssHeld = (s32)slot;
    sCssStatus = NDS_CSS_STATUS_GRAB;
    sCssSelected[slot] = 0u;
    sCssPuckX[slot] = (s16)(sCssCursorX + NDS_CSS_PUCK_CARRY_DX);
    sCssPuckY[slot] = (s16)(sCssCursorY + NDS_CSS_PUCK_CARRY_DY);
    ndsMenuShellCssCue(NDS_CSS_FGM_GRAB);
    gNdsMenuShellCssGrabCount++;
    ndsMenuShellCssPopulate();
}

/* mnPlayersVSSelectFighterPuck, mnplayersvs.c:167. */
static void ndsMenuShellCssDrop(u32 slot)
{
    sCssSelected[slot] = 1u;
    sCssHeld = -1;
    sCssStatus = NDS_CSS_STATUS_HOVER;
    sCssRegrabTic = sMenuTics + (u32)NDS_CSS_REGRAB_TICS;
    ndsMenuShellCssAnnounce(slot);
    ndsMenuShellCssFlashStart(slot);
    gNdsMenuShellCssDropCount++;
    ndsMenuShellCssPopulate();
}

/* mnPlayersVSSelectFighter, mnplayersvs.c:2825. */
static u32 ndsMenuShellCssSelectFighter(void)
{
    if (sCssStatus != NDS_CSS_STATUS_GRAB)
    {
        return FALSE;
    }
    if (sCssHeld < 0)
    {
        return FALSE;
    }
    if (sCssFkind[sCssHeld] != (u8)nFTKindNull)
    {
        ndsMenuShellCssDrop((u32)sCssHeld);
        return TRUE;
    }
    {
        /* The random chooser (owner 2026-10-17, "like SSS has"): a token
         * dropped on it takes a random unlocked selection, moves to that
         * cell and drops there, as if put on it -- the slot re-manned as a
         * portrait drop re-mans it (mnplayersvs.c:3550). */
        u32 slot = (u32)sCssHeld;
        s32 cell = ndsMenuShellCssPuckP4Cell(slot);

        if ((cell >= 0) && ((u32)kNdsP4CssCellSel[cell] == NDS_P4_SEL_RANDOM))
        {
            u32 pick = ndsMenuShellCssP4RandomSel();

            sCssFkind[slot] = (u8)pick;
            if ((sCssPkind[slot] == (u8)nFTPlayerKindNot) &&
                (slot == (u32)NDS_CSS_CURSOR_SLOT))
            {
                sCssPkind[slot] = (u8)nFTPlayerKindMan;
            }
            ndsMenuShellCssCenterPuck(slot, pick);
            gNdsMenuShellCssRandomChooserCount++;
            ndsMenuShellCssDrop(slot);
            return TRUE;
        }
    }
    /* The token is over a locked cell or off the grid: the source refuses with
     * the cue it spends on every refusal, and the token stays in hand. */
    ndsMenuShellCssCue(NDS_CSS_FGM_DENIED);
    gNdsMenuShellCssDropRefuseCount++;
    return FALSE;
}

/* mnPlayersVSCheckCursorPuckGrab, mnplayersvs.c:2957. Its loop runs 3 -> 0 so
 * a higher slot's token wins an overlap, and a slot that is NOT the cursor's
 * own is grabbable only when it is a CPU. */
static u32 ndsMenuShellCssCheckGrab(void)
{
    s32 i;

    if (sMenuTics < sCssRegrabTic)
    {
        return FALSE;
    }
    if (sCssStatus != NDS_CSS_STATUS_HOVER)
    {
        return FALSE;
    }
    for (i = NDS_CSS_SLOTS - 1; i >= 0; i--)
    {
        u32 slot = (u32)i;

        if (ndsMenuShellCssPuckInRange(slot) == FALSE)
        {
            continue;
        }
        if (slot == (u32)NDS_CSS_CURSOR_SLOT)
        {
            if (sCssPkind[slot] != (u8)nFTPlayerKindNot)
            {
                ndsMenuShellCssGrab(slot);
                return TRUE;
            }
        }
        else if ((sCssPkind[slot] == (u8)nFTPlayerKindCom) &&
                 (ndsMenuShellCssNetGuest() == FALSE))
        {
            ndsMenuShellCssGrab(slot);
            return TRUE;
        }
    }
    return FALSE;
}

/* mnPlayersVSUpdatePlayerKind, mnplayersvs.c:2181, reduced to the one-cursor
 * case this hardware has. Its three arms all begin the same way: whatever the
 * cursor was carrying is DROPPED (selected) before the slot changes kind. */
static void ndsMenuShellCssApplyKind(u32 slot)
{
    if ((sCssHeld >= 0) && ((u32)sCssHeld == slot))
    {
        /* mnPlayersVSUpdatePlayerKind flashes a token it is about to release
         * before changing that slot's ownership/kind state. */
        ndsMenuShellCssFlashStart(slot);
        sCssHeld = -1;
        sCssStatus = NDS_CSS_STATUS_HOVER;
    }
    switch (sCssPkind[slot])
    {
    case (u8)nFTPlayerKindMan:
        /* The slot goes back to holding its own token, which for the cursor's
         * own slot means the cursor picks it up again. */
        sCssSelected[slot] = 0u;
        sCssFkind[slot] = (u8)nFTKindNull;
        if (slot == (u32)NDS_CSS_CURSOR_SLOT)
        {
            sCssHeld = (s32)slot;
            sCssStatus = NDS_CSS_STATUS_GRAB;
            sCssPuckX[slot] = (s16)(sCssCursorX + NDS_CSS_PUCK_CARRY_DX);
            sCssPuckY[slot] = (s16)(sCssCursorY + NDS_CSS_PUCK_CARRY_DY);
        }
        ndsMenuShellCssCue(NDS_CSS_FGM_SLOT_WHOOSH);
        break;

    case (u8)nFTPlayerKindCom:
        sCssSelected[slot] = 1u;
        if (sCssFkind[slot] == (u8)nFTKindNull)
        {
            u32 fkind;

            /* mnPlayersVSRandFighterKind, mnplayersvs.c:3471, samples the full
             * playable range with syUtilsRandTimeUCharRange and rerolls crossed
             * / locked fighters. FighterCrossed is a source stub returning
             * FALSE; this build's EXISTENCE mask is the locked predicate. Do
             * not substitute frame parity: it is distribution-equivalent for
             * two choices but not the source RNG state/sequence. */
            /* P4: Remix's random draws among every available selection. */
            fkind = ndsMenuShellCssP4RandomSel();
            sCssFkind[slot] = (u8)fkind;
        }
        ndsMenuShellCssCenterPuck(slot, (u32)sCssFkind[slot]);
        ndsMenuShellCssAnnounce(slot);
        /* mnPlayersVSCheckPlayerKindSelect makes a fresh flash after the COM
         * transition, replacing any one UpdatePlayerKind just created. */
        ndsMenuShellCssFlashStart(slot);
        break;

    default:
        sCssSelected[slot] = 0u;
        sCssFkind[slot] = (u8)nFTKindNull;
        ndsMenuShellCssCue(NDS_CSS_FGM_SLOT_WHOOSH);
        break;
    }
    /* mnPlayersVSCheckPlayerKindSelect spends this on EVERY kind change, after
     * the per-kind cue above (mnplayersvs.c:2513). */
    ndsMenuShellCssCue(NDS_CSS_FGM_PRESS_START);
}

/* mnPlayersVSCheckPlayerKindSelectAllPlayer, mnplayersvs.c:2521. */
static u32 ndsMenuShellCssCheckKindButton(void)
{
    u32 slot;

    if (sCssTraining != 0u)
    {
        return FALSE;
    }
    for (slot = 0u; slot < (u32)NDS_CSS_SLOTS; slot++)
    {
        s32 panel = (s32)(slot * 69u);
        u32 next;

        if (ndsMenuShellCssBoxHit(NDS_CSS_CARD_X(panel, 60),
                                  NDS_CSS_CARD_X(panel, 88),
                                  NDS_CSS_CARD_Y(127), NDS_CSS_CARD_Y(145)) == FALSE)
        {
            continue;
        }
        /* P3: only the host changes a slot's kind, and never a guest's. */
        if ((ndsMenuShellCssNetGuest() != FALSE) ||
            (ndsMenuShellCssNetHumanOther(slot) != FALSE))
        {
            return FALSE;
        }
        next = (u32)sCssPkind[slot] + 1u;
        if (next > (u32)nFTPlayerKindNot)
        {
            /* The wrap depends on whether the slot HAS a controller
             * (mnplayersvs.c:2477): with one, HUMAN is in the cycle; without
             * one, it is not, so an unmanned slot alternates CP and NA. */
            next = (slot == (u32)NDS_CSS_CURSOR_SLOT) ?
                (u32)nFTPlayerKindMan : (u32)nFTPlayerKindCom;
        }
        sCssPkind[slot] = (u8)next;
        ndsMenuShellCssApplyKind(slot);
        gNdsMenuShellCssKindToggleCount++;
        ndsMenuShellCssPopulate();
        return TRUE;
    }
    return FALSE;
}

/* mnPlayersVSCheckTeamSelectInRangeAll, mnplayersvs.c:1985. The source draws a
 * selector for every panel in Team Battle but only lets an occupied panel
 * consume A. Each press cycles Red -> Blue -> Green -> Red, recolors the gate,
 * switches the fighter to its team costume/shade, and spends TitlePressStart.
 * The appearance mutation itself remains in the imported PlayersVS helper so
 * mnPlayersVSGetShade/ftParamGetCostumeTeamID stay the behavioral authority. */
static u32 ndsMenuShellCssCheckTeamSelect(void)
{
    u32 slot;

    if (sCssIsTeamBattle == FALSE)
    {
        return FALSE;
    }
    for (slot = 0u; slot < (u32)NDS_CSS_SLOTS; slot++)
    {
        s32 panel = (s32)(slot * 69u);

        if ((sCssPkind[slot] == (u8)nFTPlayerKindNot) ||
            (ndsMenuShellCssBoxHit(NDS_CSS_CARD_X(panel, 34),
                                   NDS_CSS_CARD_X(panel, 58),
                                   NDS_CSS_CARD_Y(131), NDS_CSS_CARD_Y(141)) == FALSE))
        {
            continue;
        }
        /* P3: a player picks their own team; the host also picks for CPUs. */
        if ((ndsMenuShellCssNetGuest() != FALSE) ?
                (slot != (u32)NDS_CSS_CURSOR_SLOT) :
                (ndsMenuShellCssNetHumanOther(slot) != FALSE))
        {
            return FALSE;
        }
        sCssTeam[slot] =
            (sCssTeam[slot] == (u8)nSCBattleTeamIDGreen) ?
                (u8)nSCBattleTeamIDRed : (u8)(sCssTeam[slot] + 1u);
        /* The source swaps THIS panel's LUT + team selector immediately. Do not
         * route it through SyncPanels(1): if an older panel update were pending,
         * that budget could spend itself on an earlier slot and visibly delay
         * the clicked team. This direct one-surface replacement is the exact
         * DS equivalent of UpdateTeamSelect + SetGateLUT. */
        {
            NdsUiKitSurfaceId panel_surface = ndsMenuShellCssGateSurface(slot);

            if (ndsUiKitBlitSurfaces(&panel_surface, 1u) != FALSE)
            {
                sCssPanelSurface[slot] = panel_surface;
                gNdsMenuShellCssPanelBlitCount++;
            }
        }
        ndsMenuShellCssDrawTeamSelect(slot);
        ndsMenuShellCssCue(NDS_CSS_FGM_PRESS_START);
        return TRUE;
    }
    return FALSE;
}

/* mnPlayersVSCheckHandicapArrowInRangeAll, mnplayersvs.c:2027 -- clamped 1..9
 * both ways, and the cue only fires when the value actually moves. */
static u32 ndsMenuShellCssCheckLevelArrows(void)
{
    u32 slot;

    /* P3: CPU levels and handicaps are the host's. Training has neither. */
    if ((ndsMenuShellCssNetGuest() != FALSE) || (sCssTraining != 0u))
    {
        return FALSE;
    }

    for (slot = 0u; slot < (u32)NDS_CSS_SLOTS; slot++)
    {
        s32 panel = (s32)(slot * 69u);

        u32 is_cpu = (sCssPkind[slot] == (u8)nFTPlayerKindCom) ? TRUE : FALSE;
        u32 is_human_handicap =
            ((sCssPkind[slot] == (u8)nFTPlayerKindMan) &&
             (gNdsMatchConfig.handicap_mode == (u8)nSCBattleHandicapOn)) ?
            TRUE : FALSE;
        u8 *value = (is_cpu != FALSE) ? &sCssLevel[slot] :
            &sCssHandicap[slot];

        if ((sCssSelected[slot] == 0u) ||
            ((is_cpu == FALSE) && (is_human_handicap == FALSE)))
        {
            continue;
        }
        if (ndsMenuShellCssBoxHit(NDS_CSS_CARD_X(panel, 68),
                                  NDS_CSS_CARD_X(panel, 90),
                                  NDS_CSS_CARD_Y(197), NDS_CSS_CARD_Y(216)) != FALSE)
        {
            if (*value < 9u)
            {
                (*value)++;
                ndsMenuShellCssCue(NDS_CSS_FGM_SCROLL2);
                if (is_cpu != FALSE)
                {
                    gNdsMenuShellCssLevelChangeCount++;
                }
                else
                {
                    NDS_DIAG(gNdsMenuShellCssHandicapChangeCount++);
                }
                ndsMenuShellCssPopulate();
            }
            return TRUE;
        }
        if (ndsMenuShellCssBoxHit(NDS_CSS_CARD_X(panel, 21),
                                  NDS_CSS_CARD_X(panel, 43),
                                  NDS_CSS_CARD_Y(197), NDS_CSS_CARD_Y(216)) != FALSE)
        {
            if (*value > 1u)
            {
                (*value)--;
                ndsMenuShellCssCue(NDS_CSS_FGM_SCROLL2);
                if (is_cpu != FALSE)
                {
                    gNdsMenuShellCssLevelChangeCount++;
                }
                else
                {
                    NDS_DIAG(gNdsMenuShellCssHandicapChangeCount++);
                }
                ndsMenuShellCssPopulate();
            }
            return TRUE;
        }
    }
    return FALSE;
}

/* mnPlayersVSRecallPuck, mnplayersvs.c:3198 -- B with your own fighter placed
 * pulls the token back. The END STATE is what is transcribed: the token is in
 * the hand again (mnPlayersVSPuckAdjustRecall re-grabs it at recall tic 11). */
static void ndsMenuShellCssRecall(void)
{
    u32 slot = (u32)NDS_CSS_CURSOR_SLOT;

    if ((sCssSelected[slot] == 0u) || (sCssHeld >= 0) ||
        (sCssPkind[slot] != (u8)nFTPlayerKindMan))
    {
        return;
    }
    sCssSelected[slot] = 0u;
    sCssHeld = (s32)slot;
    sCssStatus = NDS_CSS_STATUS_GRAB;
    sCssPuckX[slot] = (s16)(sCssCursorX + NDS_CSS_PUCK_CARRY_DX);
    sCssPuckY[slot] = (s16)(sCssCursorY + NDS_CSS_PUCK_CARRY_DY);
    /* This shell jumps directly to mnPlayersVSPuckAdjustRecall's tic-11 end
     * state, whose SetCursorGrab call destroys the portrait flash. */
    ndsMenuShellCssFlashCancel(slot);
    gNdsMenuShellCssRecallCount++;
    ndsMenuShellCssPopulate();
}

/* mnPlayersVSSetSceneData, mnplayersvs.c:4379 -- through the P2-1a descriptor,
 * which is the battle's only input, rather than straight into the battle
 * state. The rules half stays whatever the VS menu committed. */
/* mnPlayers1PTrainingSetSceneData (mnplayers1ptraining.c:2911): the man's
 * and the COM's kind and costume, each a selection's setup parent with its
 * content beside it. */
static void ndsMenuShellCssCommitTraining(void)
{
    const u32 man = (u32)NDS_CSS_CURSOR_SLOT;
    const u32 com = (man == 0u) ? 1u : 0u;

    gSCManagerSceneData.training_man_fkind =
        (u8)ndsMenuShellCssSelFkind((u32)sCssFkind[man]);
    gSCManagerSceneData.training_man_costume =
        (u8)(ndsMNPlayersVSPreviewGetAppearance(man) & 0xffu);
    gSCManagerSceneData.training_com_fkind =
        (u8)ndsMenuShellCssSelFkind((u32)sCssFkind[com]);
    gSCManagerSceneData.training_com_costume =
        (u8)(ndsMNPlayersVSPreviewGetAppearance(com) & 0xffu);
    gNdsP4TrainingContent[0] = (u8)ndsMenuShellCssSelContent((u32)sCssFkind[man]);
    gNdsP4TrainingContent[1] = (u8)ndsMenuShellCssSelContent((u32)sCssFkind[com]);
    gNdsMenuShellCssCommitCount++;
}

/* mnPlayers1PGameSetSceneData (:3262), on START and on the way back alike:
 * the time, level and stocks and the placed fighter -- a Remix selection's
 * setup parent with its content beside it -- then the source's save write. */
extern u8 gNdsP7OnePlayerMetal;

static void ndsMenuShellCssOnePCommit(void)
{
    const u32 sel = (u32)sCssFkind[0];

    gSCManagerSceneData.spgame_time_limit = sCssOnePTime;
    gSCManagerBackupData.spgame_difficulty = sCssOnePLevel;
    gSCManagerSceneData.spgame_stage = 0;
    gSCManagerBackupData.spgame_stock_count = sCssOnePStock;
    gNdsP7OnePlayerMetal = 0u;
    if ((sCssSelected[0] != 0u) && (sel != (u32)nFTKindNull))
    {
        gSCManagerSceneData.fkind = (u8)ndsMenuShellCssSelFkind(sel);
        gNdsP4OnePlayerContent = (u8)ndsMenuShellCssSelContent(sel);
    }
    else
    {
        gSCManagerSceneData.fkind = (u8)nFTKindNull;
        gNdsP4OnePlayerContent = 0u;
    }
    gSCManagerSceneData.costume =
        (u8)(ndsMNPlayersVSPreviewGetAppearance(0u) & 0xffu);
    if (gSCManagerSceneData.fkind == (u8)nFTKindMMario)
    {
        /* Metal Mario plays the campaign under Mario's kind and is made as
         * himself (battleship_ftmanager.c ndsP7OnePlayerMetalActive). */
        gSCManagerSceneData.fkind = (u8)nFTKindMario;
        gSCManagerSceneData.costume = 0u;
        gNdsP7OnePlayerMetal = 1u;
    }
    lbBackupWrite();
    gNdsMenuShellCssCommitCount++;
}

/* mnPlayers1PBonusSetSceneData (:2678): the placed fighter and its costume
 * (the man player stays the scene's), a Remix selection's setup parent with
 * its content beside it and Metal Mario as Mario with the metal flag, as the
 * 1P select's (C1); the practice board reads them (battleship_sc1pbonusstage
 * files, nds_p4.c ndsP4MatchContent). No save write here, as the source. */
extern u8 gNdsP7BonusPracticeMetal;

static void ndsMenuShellCssBonusCommit(void)
{
    const u32 sel = (u32)sCssFkind[0];

    gNdsP7BonusPracticeMetal = 0u;
    gNdsP7BonusPracticeContent = 0u;
    gSCManagerSceneData.bonus_fkind = (u8)nFTKindNull;
    if ((sCssSelected[0] != 0u) && (sel != (u32)nFTKindNull))
    {
        gSCManagerSceneData.bonus_fkind = (u8)ndsMenuShellCssSelFkind(sel);
        gNdsP7BonusPracticeContent = (u8)ndsMenuShellCssSelContent(sel);
    }
    gSCManagerSceneData.bonus_costume =
        (u8)(ndsMNPlayersVSPreviewGetAppearance(0u) & 0xffu);
    if (gSCManagerSceneData.bonus_fkind == (u8)nFTKindMMario)
    {
        gSCManagerSceneData.bonus_fkind = (u8)nFTKindMario;
        gSCManagerSceneData.bonus_costume = 0u;
        gNdsP7BonusPracticeMetal = 1u;
    }
    /* The kind's select is the scene the board and the 1P menu are left
     * from (mnPlayers1PBonusFuncRun :2716, BackTo1PMode :2032): the shell's
     * exit makes the current scene the previous one. */
    gSCManagerSceneData.scene_curr = (sCssBonusKind == 0u) ?
        (u8)nSCKind1PBonus1Players : (u8)nSCKind1PBonus2Players;
    gNdsMenuShellCssCommitCount++;
}

static void ndsMenuShellCssCommit(void)
{
    u32 i;

    if (sCssTraining != 0u)
    {
        ndsMenuShellCssCommitTraining();
        return;
    }
    if (NDS_CSS_BONUS)
    {
        ndsMenuShellCssBonusCommit();
        return;
    }
    if (sCssOneP != 0u)
    {
        ndsMenuShellCssOnePCommit();
        return;
    }
    for (i = 0u; i < (u32)NDS_MATCH_FIGHTERS_MAX; i++)
    {
        NdsMatchFighterConfig *slot = &gNdsMatchConfig.fighters[i];

        slot->fkind = (u8)ndsMenuShellCssSelFkind((u32)sCssFkind[i]);
        slot->p4_content = (u8)ndsMenuShellCssSelContent((u32)sCssFkind[i]);
        slot->pkind = sCssPkind[i];
        if (sCssPkind[i] == (u8)nFTPlayerKindCom)
        {
            slot->level = sCssLevel[i];
        }
        else
        {
            slot->handicap = sCssHandicap[i];
            /* ndsMatchConfigApply deliberately preserves the live On/Auto
             * handicap block. Publish this explicit CSS edit there first, just
             * as mnPlayersVSSetSceneData writes transfer.players[i].handicap. */
            gSCManagerTransferBattleState.players[i].handicap =
                sCssHandicap[i];
        }
        /* mnPlayersVSSetSceneData writes the slot's selected team. The old
         * array-index assignment even produced team id 3 for slot 4, outside
         * the source's Red/Blue/Green enum. */
        slot->team = sCssTeam[i];
        /* The source CSS commits the appearance it is actually showing. Ask
         * the imported PlayersVS state instead of re-deriving team/free-color
         * allocation here; this preserves mnPlayersVSGetShade's slot-order
         * behavior for mirrored Mario/Fox fighters. */
        {
            u32 appearance = ndsMNPlayersVSPreviewGetAppearance(i);

            slot->costume = (u8)(appearance & 0xffu);
            slot->shade = (u8)((appearance >> 8) & 0xffu);
        }
        gNdsMenuShellCssCommitSlot[i] =
            (((u32)sCssFkind[i] & 0xffu) << 16) |
            (((u32)sCssPkind[i] & 0xffu) << 8) | ((u32)sCssLevel[i] & 0xffu);
    }
    /* P2-2a: mode and all four source slot-team selectors are live. The
     * descriptor therefore carries the same CSS state mnPlayersVSSetSceneData
     * commits for FFA and Team Battle. */
    gNdsMatchConfig.is_team_battle = (sCssIsTeamBattle != FALSE) ? 1u : 0u;
    /* mnPlayersVSSetSceneData :4383-4384: the rule bar's values. A wireless
     * guest's match takes the host's from the descriptor instead. */
    gNdsMatchConfig.time_limit = sCssRuleTime;
    gNdsMatchConfig.stocks = sCssRuleStock;
    ndsMatchConfigApply(&gNdsMatchConfig);
    gNdsMenuShellCssCommitCount++;
}

/* Defined by the SSS fragment later in the concatenated nds_menu_shell.c TU.
 * Keeping one lock/build predicate prevents CSS random selection from
 * inventing a second idea of which grounds exist in this ROM/save. */
static u32 ndsMenuShellSssGroundLocked(u32 gkind);
#if NDS_P5
/* P5: the stage select's draw over every ground it offers. */
static u32 ndsMenuShellSssRandomGround(void);
#endif

/* mnPlayersVSFuncRun:4497-4508. Stage Select Off enters battle directly and
 * rolls one source time-RNG ground per retry. The retail source can assume its
 * whole ground set exists; the DS build cannot, so use the same built/unlocked
 * predicate and bounded fallback as the SSS random cell. */
static void ndsMenuShellCssRandomizeStage(void)
{
    u32 available = 0u;
    u32 gkinds_num =
        (gSCManagerBackupData.unlock_mask & LBBACKUP_UNLOCK_MASK_INISHIE) ?
        ((u32)nGRKindUnlockEnd + 1u) : ((u32)nGRKindStarterEnd + 1u);
    u32 gkind;
    u32 pick = (u32)nGRKindInishie + 1u;
    u32 tries;

#if NDS_P5
    /* Remix's Stage Select Off draws from its stages, new ones included. */
    pick = ndsMenuShellSssRandomGround();
    if (pick != 0xdeu)
    {
        gNdsMatchConfig.gkind = (u8)pick;
        dSCManagerDefaultSceneData.gkind = (u8)pick;
        gSCManagerSceneData.gkind = (u8)pick;
        gNdsMenuShellCssRandomCount++;
        return;
    }
    pick = (u32)nGRKindInishie + 1u;
#endif
    for (gkind = 0u; gkind <= (u32)nGRKindInishie; gkind++)
    {
        if (ndsMenuShellSssGroundLocked(gkind) == FALSE)
        {
            available++;
        }
    }

    for (tries = 0u; tries < 32u; tries++)
    {
        /* mnplayersvs.c:4500 derives the draw range from the Inishie unlock;
         * :4504 consumes exactly one time-RNG draw per retry. */
        u32 rolled = (u32)syUtilsRandTimeUCharRange(gkinds_num);

        if ((ndsMenuShellSssGroundLocked(rolled) == FALSE) &&
            ((available <= 1u) ||
             (rolled != (u32)gSCManagerSceneData.gkind)))
        {
            pick = rolled;
            break;
        }
    }
    if (pick > (u32)nGRKindInishie)
    {
        for (gkind = 0u; gkind <= (u32)nGRKindInishie; gkind++)
        {
            if ((ndsMenuShellSssGroundLocked(gkind) == FALSE) &&
                ((available <= 1u) ||
                 (gkind != (u32)gSCManagerSceneData.gkind)))
            {
                pick = gkind;
                break;
            }
        }
        if (pick > (u32)nGRKindInishie)
        {
            /* Pupupu is always present in NDS_SSS_GROUND_MASK. */
            pick = (u32)nGRKindPupupu;
        }
        gNdsMenuShellCssRandomFallbackCount++;
    }

    gNdsMatchConfig.gkind = (u8)pick;
    dSCManagerDefaultSceneData.gkind = (u8)pick;
    gSCManagerSceneData.gkind = (u8)pick;
    gNdsMenuShellCssRandomCount++;
}

/* mnPlayersVSSetIdlePlayerNotAll, mnplayersvs.c:4294. */
static void ndsMenuShellCssIdleSlotsNot(void)
{
    u32 i;

    for (i = 0u; i < (u32)NDS_CSS_SLOTS; i++)
    {
        if (sCssSelected[i] == 0u)
        {
            sCssPkind[i] = (u8)nFTPlayerKindNot;
            sCssFkind[i] = (u8)nFTKindNull;
        }
    }
}

/* mnPlayersVSBackToVSMode, mnplayersvs.c:3227: the CSS commits on the way out
 * too, so a return trip shows what the last visit chose. */
static void ndsMenuShellCssBack(void)
{
    ndsMenuShellCssCommit();
    gNdsMenuShellCssBackCount++;
    /* P2-1k (g). mnPlayersVSBackToVSMode, mnplayersvs.c:3234: the character
     * select STOPS all BGM on its way out, and the VS menu it returns to
     * starts the mode-select track again from its own func_start. Without the
     * stop, BGM 10 would keep playing under a VS menu that then starts 44 over
     * the top of it. */
    ndsAudioBgmStopAll();
    /* mnPlayers1PTrainingBackTo1PMode (:2046): the training select's way out
     * is the 1P menu, with the same commit and stop. */
    ndsMenuShellGoto(((sCssTraining != 0u) || NDS_CSS_ONEP) ?
                         (u32)nSCKind1PMode : (u32)nSCKindVSMode);
}
