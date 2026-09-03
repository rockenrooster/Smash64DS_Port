/* --- Screen: VS stage select (P2-1f) --------------------------------------
 *
 * mn/mnmaps/mnmaps.c. A GRID screen, not a pointer one: a red frame sits on
 * one of ten cells -- nine grounds in a 5x2 block plus RANDOM -- and moves a
 * whole cell at a time. Everything below is transcribed from that file, in the
 * SOURCE's own 320x240 frame exactly as the character select is, so a probe
 * reads back the numbers mnmaps.c is written in.
 *
 * THE GRID, mnMapsMakeIcons:530 and mnMapsSetCursorPosition:865. Icon i sits
 * at (i*50 + 30, 30) for i < 5 and (i*50 - 220, 68) for i >= 5, so the columns
 * are x = 30/80/130/180/230 and the rows y = 30/68; the cursor sits at exactly
 * (icon - 7, icon - 7). mnMapsGetGroundKind:453 is what a slot MEANS, and slot
 * 9 is `0xDE` -- the source's own spelling of RANDOM, kept rather than renamed.
 *
 * WHAT IS LOCKED, and it is the same substitution the character select makes.
 * The source's `mnMapsCheckLocked` (mnmaps.c:166) locks exactly one ground --
 * Mushroom Kingdom, behind `LBBACKUP_UNLOCK_MASK_INISHIE` -- because a retail
 * cart's other eight are always available. The gate this build needs is which
 * grounds EXIST: eight of the nine are P2-4, so the mask is Dream Land alone.
 * Same bitmask over gkind, different bound, and the RANDOM cell is never
 * locked because the source never locks it.
 *
 * THE ONE GENERALISATION, disclosed rather than buried. The source's UP/DOWN
 * arms test the destination with `mnMapsCheckLocked` and refuse the move; its
 * LEFT/RIGHT arms do NOT, because the only lockable ground is slot 4 and the
 * left/right wraps SPECIAL-CASE slot 4 by hand (`case 0: locked(4) ? 3 : 4`,
 * `case 3: locked(4) ? 0 : 4`). With eight locked cells that hand-written case
 * stops covering the lock, so left/right here SKIP locked cells in the
 * direction of travel around the row's own cycle. That reproduces the source's
 * table exactly on every case it enumerates -- left from 0 reaches 4, finds it
 * locked and lands on 3; right from 3 reaches 4, finds it locked and wraps to
 * 0; right from 4 is 0; left from 5 is 9; right from 9 is 5; everything else
 * is +-1 -- and it extends to a lock set the source never had. The scan is
 * bounded by the row length so an all-locked row cannot spin.
 *
 * DELIBERATE NARROWINGS, each a plan non-goal rather than an omission:
 *   - THE 3D PREVIEW. mnMapsMakePreview loads the ground's map file into one
 *     of two model heaps, builds up to four layer GObjs from its own DObj
 *     descriptors, and orbits a camera over them (mnmaps.c:1096/:1027/:1330).
 *     That is the RSP/RDP scene graph this target does not have, and it is the
 *     single most expensive thing on the source's screen. The preview PANEL is
 *     kept and holds the selected ground's own icon instead.
 *   - THE PLAQUE AND THE EMBLEM. mnMapsMakePlaque draws an 84x85 CI8 wooden
 *     circle with a 256-entry TLUT and five bitmap bands, and the emblem on it
 *     is a per-series FTEmblem sprite (mnmaps.c:245/:790). Main OBJ VRAM has
 *     8,448 B left after this row's three icons; the circle alone is 8,192 of
 *     it for one decoration. The words it frames are kept.
 *   - THE STAGE NAME AND HEADER as SPRITES. The source draws
 *     `llMNMaps<Ground>TextSprite` and `llMNMapsStageSelectTextSprite`; this
 *     composes the same words out of the source's own menu font, which is the
 *     P2-1c kit's whole reason for existing and exactly what P2-1e did with
 *     the fighter names. REGION_US draws no subtitle at all
 *     (mnMapsMakeSubtitle is `return;` under REGION_US, mnmaps.c:659).
 *   - THE 5-MINUTE IDLE RETURN (mnmaps.c:1451) is attract behaviour and
 *     belongs to P2-7, exactly as it does on the mode select and the CSS.
 *   - TRAINING MODE. `sMNMapsIsTrainingMode` is set when scene_prev is
 *     nSCKindPlayers1PTraining (mnmaps.c:1415); that scene is P2-7, so the
 *     flag is FALSE by construction here and its two branches -- the training
 *     wallpaper set and the 1PTrainingMode destination -- are absent rather
 *     than transcribed-and-dead.
 *
 * ONE LOCKED CELL IS DRAWN WHERE THE SOURCE DRAWS NOTHING, and it is the row's
 * only presentation ADDITION. mnMapsMakeIcons simply skips a locked ground, so
 * a build with eight locked grounds would show two icons floating in an empty
 * field and the 5x2 grid the cursor moves around would be invisible. The cell
 * gets the source's own locked plate -- the question-mark sprite
 * mnPlayersVSMakePortrait draws for a locked FIGHTER, already in the pack from
 * P2-1e -- so the grid reads as a grid. Zero new bytes, and it is the
 * precedent the character select already set. */

/* Source frame -> DS pixels, same exact 4/5 the character select uses. */
#define NDS_SSS_DS(v) NDS_CSS_DS(v)

#define NDS_SSS_SLOTS 10u
#define NDS_SSS_ROW 5u
/* mnMapsGetGroundKind:453 -- slot 9's ground kind. */
#define NDS_SSS_GKIND_RANDOM 0xdeu

/* mnMapsMakeIcons' grid, in source units. */
#define NDS_SSS_ICON_X0 30
#define NDS_SSS_ICON_PITCH 50
#define NDS_SSS_ICON_Y0 30
#define NDS_SSS_ICON_Y1 68
/* mnMapsSetCursorPosition (mnmaps.c:845/:851): the frame sits 7 px up and
 * left of the icon -- `slot * 50 + 23` against the icon's `slot * 50 + 30`.
 * P2-1L (6) applies it directly: with the grid and the cursor both at the
 * frame's own 4/5, the source's own offset is the only one that keeps a 50x40
 * frame around a 38x29 icon. */
#define NDS_SSS_CURSOR_DX (-7)
#define NDS_SSS_CURSOR_DY (-7)
/* P2-1L (9) DELETED `NDS_SSS_ICON_INSET_X/Y` with the placeholder that needed
 * them: they centred P2-1f's 5/8 icon inside the 4/5 grid footprint, and the
 * preview panel was the last draw on this screen still at 5/8. Nothing on the
 * stage select is at any ratio but the frame's own 4/5 now. */

/* mnMapsFuncRun's own input gate and repeat (mnmaps.c:1440/:1523). */
#define NDS_SSS_INPUT_ARM_TICS 10
#define NDS_SSS_SCROLL_WAIT 12

/* THE CUES, by the source's own REGION_US FGM ids, derived by the same
 * gm/gmsound.h parse P2-1c-1/P2-1d-1/P2-1e-1 used and cross-checked against
 * every id this tree already pins.
 *   164 MenuScroll2  -- every cursor move (mnmaps.c:1508 and the three arms
 *                       below it). IN THE PACK since P2-1c-1.
 *   159 StageSelect  -- the A/START confirm (mnmaps.c:1470). NOT IN THE PACK:
 *                       the pack carries 158/163/164/165 plus P2-1d-1's 157
 *                       and P2-1e-1's 121/127/167/512, and 159 is none of
 *                       them. The seam asks with the real id so the FGM miss
 *                       ring measures the gap; row P2-1f-1 renders it.
 * B PLAYS NOTHING. mnMapsFuncRun's B arm transitions silently (mnmaps.c:1483)
 * -- no `func_800269C0_275C0` call at all -- which is worth saying because
 * every other screen in this shell cues its back-out. */
#define NDS_SSS_FGM_SCROLL2 164u    /* nSYAudioFGMMenuScroll2 */
#define NDS_SSS_FGM_CONFIRM 159u    /* nSYAudioFGMStageSelect */

/* mnMapsFuncStart starts NO music (mnmaps.c:1595 makes cameras and sprites and
 * nothing else), so the character select's BGM 10 plays straight through this
 * screen -- which is why mnPlayersVSFuncStart's own track is gated on
 * `scene_prev != nSCKindMaps` (mnplayersvs.c:4899), the condition P2-1e already
 * transcribed against a scene that could not yet be scene_prev. It can now. */

/* mnMapsGetGroundKind, mnmaps.c:453. */
static const u8 kNdsSssSlotGkind[NDS_SSS_SLOTS] = {
    (u8)nGRKindCastle, (u8)nGRKindJungle, (u8)nGRKindHyrule,
    (u8)nGRKindZebes, (u8)nGRKindInishie, (u8)nGRKindYoster,
    (u8)nGRKindPupupu, (u8)nGRKindSector, (u8)nGRKindYamabuki,
    (u8)NDS_SSS_GKIND_RANDOM
};

/* Which grounds this build HAS. Same shape as the source's ground_mask
 * (LBBACKUP_MASK_STAGE, sc/scene.h:107), and it is the whole lock table.
 *
 * P2-4 opens one slot per landed stage, so the mask is built additively: each
 * stage contributes its own bit or zero, and the remaining six extend the
 * chain by two lines each rather than doubling a conditional. With every flag
 * off the value is exactly the old Dream Land-only one. */
#if NDS_P2_STAGE_YOSTER
#define NDS_SSS_MASK_YOSTER LBBACKUP_MASK_STAGE(nGRKindYoster)
#else
#define NDS_SSS_MASK_YOSTER 0u
#endif
#if NDS_P2_STAGE_CASTLE
#define NDS_SSS_MASK_CASTLE LBBACKUP_MASK_STAGE(nGRKindCastle)
#else
#define NDS_SSS_MASK_CASTLE 0u
#endif
#if NDS_P2_STAGE_JUNGLE
#define NDS_SSS_MASK_JUNGLE LBBACKUP_MASK_STAGE(nGRKindJungle)
#else
#define NDS_SSS_MASK_JUNGLE 0u
#endif
#if NDS_P2_STAGE_ZEBES
#define NDS_SSS_MASK_ZEBES LBBACKUP_MASK_STAGE(nGRKindZebes)
#else
#define NDS_SSS_MASK_ZEBES 0u
#endif
#if NDS_P2_STAGE_HYRULE
#define NDS_SSS_MASK_HYRULE LBBACKUP_MASK_STAGE(nGRKindHyrule)
#else
#define NDS_SSS_MASK_HYRULE 0u
#endif
#define NDS_SSS_GROUND_MASK (LBBACKUP_MASK_STAGE(nGRKindPupupu) | \
                             NDS_SSS_MASK_YOSTER | \
                             NDS_SSS_MASK_CASTLE | \
                             NDS_SSS_MASK_JUNGLE | \
                             NDS_SSS_MASK_ZEBES | \
                             NDS_SSS_MASK_HYRULE)

/* P2-1k (c). THE STAGE SELECT'S TEXT IS SOURCE ART NOW and this screen draws
 * no kit text at all: the STAGE SELECT header is `llMNMapsStageSelectTextSprite`
 * baked into SSS_SCREEN, and the per-stage name is
 * `llMNMaps<Ground>TextSprite` baked into one of the ten SSS_PLAQUE surfaces.
 *
 * The plaque is its own surface for the reason the VS menu's buttons are:
 * `mnMapsMakeNameAndEmblem` (mnmaps.c:818) ejects and re-makes the pair on
 * every cursor move, and all ten share one declared box composited over the
 * furniture beneath them, so a move is a single blit that overwrites the last
 * state exactly. Indexed by SLOT, which is what the cursor holds. */
static const NdsUiKitSurfaceId kNdsSssPlaqueSurface[NDS_SSS_SLOTS] = {
    NDS_MN_UI_KIT_SURFACE_SSS_PLAQUE_0,
    NDS_MN_UI_KIT_SURFACE_SSS_PLAQUE_1,
    NDS_MN_UI_KIT_SURFACE_SSS_PLAQUE_2,
    NDS_MN_UI_KIT_SURFACE_SSS_PLAQUE_3,
    NDS_MN_UI_KIT_SURFACE_SSS_PLAQUE_4,
    NDS_MN_UI_KIT_SURFACE_SSS_PLAQUE_5,
    NDS_MN_UI_KIT_SURFACE_SSS_PLAQUE_6,
    NDS_MN_UI_KIT_SURFACE_SSS_PLAQUE_7,
    NDS_MN_UI_KIT_SURFACE_SSS_PLAQUE_8,
    NDS_MN_UI_KIT_SURFACE_SSS_PLAQUE_9
};

/* P2-1L (9). THE PREVIEW PANEL'S ART, on the same terms as the plaque above.
 * `mnMapsMakePreviewWallpaper` (mnmaps.c:909) draws the SELECTED ground's own
 * 300x220 background at scale 0.37 -- or RANDOM's own 110x82 plate unscaled --
 * over the fill and the seven tiles SSS_SCREEN already carries, and
 * `mnMapsMakePreview` (:1100) rebuilds it on every cursor move exactly as
 * `mnMapsMakeNameAndEmblem` rebuilds the plaque.  So it is a per-state surface
 * sharing one declared box, blitted on a change, for the plaque's own reasons.
 *
 * INDEXED BY SLOT and TOTAL: a locked slot has no preview because the cursor
 * cannot land on it (`mnMapsCheckLocked` refuses the move, mnmaps.c:166), so
 * its entry is the NONE sentinel and the sync leaves the panel alone rather
 * than blitting art for a stage this build does not have. */
/* P2-4s1. Slot 5's preview, on the mask's own terms above: the Yoster bake row
 * (SSS_PREVIEW_WALLPAPER in generate_mn_ui_kit.py) only emits
 * SSS_PREVIEW_YOSHIS_ISLAND -- manifest id
 * NDS_MN_UI_KIT_SURFACE_SSS_PREVIEW_YOSHIS_ISLAND -- when the kit bakes with
 * NDS_P2_STAGE_YOSTER=1, so the id is only named when the flag is on. Off, the
 * NONE sentinel leaves the panel on the last preview rather than blitting a
 * wrong stage (the sync skips NONE entries), and the plaque (SSS_PLAQUE_5,
 * baked for all ten slots already) still names the stage. */
#if NDS_P2_STAGE_YOSTER
#define NDS_SSS_PREVIEW_YOSTER NDS_MN_UI_KIT_SURFACE_SSS_PREVIEW_YOSHIS_ISLAND
#else
#define NDS_SSS_PREVIEW_YOSTER NDS_MENU_VS_SURFACE_NONE
#endif
/* P2-4s2 slot 0, on the same terms: the kit only emits
 * SSS_PREVIEW_PEACHS_CASTLE when it bakes with NDS_P2_STAGE_CASTLE=1, so the
 * id is only named when the flag is on. */
#if NDS_P2_STAGE_CASTLE
#define NDS_SSS_PREVIEW_CASTLE NDS_MN_UI_KIT_SURFACE_SSS_PREVIEW_PEACHS_CASTLE
#else
#define NDS_SSS_PREVIEW_CASTLE NDS_MENU_VS_SURFACE_NONE
#endif
#if NDS_P2_STAGE_JUNGLE
#define NDS_SSS_PREVIEW_JUNGLE NDS_MN_UI_KIT_SURFACE_SSS_PREVIEW_CONGO_JUNGLE
#else
#define NDS_SSS_PREVIEW_JUNGLE NDS_MENU_VS_SURFACE_NONE
#endif
#if NDS_P2_STAGE_ZEBES
#define NDS_SSS_PREVIEW_ZEBES NDS_MN_UI_KIT_SURFACE_SSS_PREVIEW_PLANET_ZEBES
#else
#define NDS_SSS_PREVIEW_ZEBES NDS_MENU_VS_SURFACE_NONE
#endif
#if NDS_P2_STAGE_HYRULE
#define NDS_SSS_PREVIEW_HYRULE NDS_MN_UI_KIT_SURFACE_SSS_PREVIEW_HYRULE_CASTLE
#else
#define NDS_SSS_PREVIEW_HYRULE NDS_MENU_VS_SURFACE_NONE
#endif
static const NdsUiKitSurfaceId kNdsSssPreviewSurface[NDS_SSS_SLOTS] = {
    NDS_SSS_PREVIEW_CASTLE, NDS_SSS_PREVIEW_JUNGLE,
    NDS_SSS_PREVIEW_HYRULE, NDS_SSS_PREVIEW_ZEBES,
    NDS_MENU_VS_SURFACE_NONE, NDS_SSS_PREVIEW_YOSTER,
    NDS_MN_UI_KIT_SURFACE_SSS_PREVIEW_DREAM_LAND,
    NDS_MENU_VS_SURFACE_NONE, NDS_MENU_VS_SURFACE_NONE,
    NDS_MN_UI_KIT_SURFACE_SSS_PREVIEW_RANDOM
};

static NdsUiKitSurfaceId sSssPlaqueSurface;
static NdsUiKitSurfaceId sSssPreviewSurface;

/* THE CURSOR IS THE ONLY OBJ LEFT ON THIS SCREEN. P2-1L (6) moved the ten grid
 * cells into the backdrop surface and (9) moved the preview panel into two
 * surfaces of its own, so the depth-ordered slot list this once needed is one
 * entry -- and it keeps id 0 because the cursor frame draws over the cell it
 * selects (see NDS_UI_KIT_SPRITE_SLOTS). */
#define NDS_SSS_SPRITE_CURSOR 0u

/* P2-1f's two colour constants are DELETED with this row, and the reason they
 * existed is the thing P2-1k fixed: the name plate's text is BLACK in the
 * source (mnmaps.c:600) and P2-1f had no plate to put it on, so it borrowed the
 * header's light tone. The plate is baked now (llMNMapsPlate{Left,Middle,Right}
 * over mnMapsLabelsProcDisplay's own drop-shadow fill), so the name is the
 * source's own sprite in the source's own black. */

static u32 sSssCursorSlot;
static u32 sSssScrollWait;

static void ndsMenuShellSssCue(u32 id)
{
    gNdsMenuShellSssCueCount++;
    gNdsMenuShellSssCueLastId = id;
    (void)ndsAudioFgmPlay((u16)id);
}

/* mnMapsCheckLocked, mnmaps.c:166 -- against this build's ground mask. RANDOM
 * is never locked, which is the source's own `else return FALSE`. */
static u32 ndsMenuShellSssGroundLocked(u32 gkind)
{
    if (gkind == NDS_SSS_GKIND_RANDOM)
    {
        return FALSE;
    }
    if (gkind > (u32)nGRKindInishie)
    {
        return TRUE;
    }
    return ((NDS_SSS_GROUND_MASK & (1u << gkind)) != 0u) ? FALSE : TRUE;
}

static u32 ndsMenuShellSssSlotLocked(u32 slot)
{
    return (slot >= NDS_SSS_SLOTS) ?
        TRUE : ndsMenuShellSssGroundLocked((u32)kNdsSssSlotGkind[slot]);
}

/* mnMapsGetSlot, mnmaps.c:471 -- the inverse, used only to restore the cursor
 * from gSCManagerSceneData.maps_vsmode_gkind on re-entry. */
static u32 ndsMenuShellSssSlotOfGkind(u32 gkind)
{
    u32 i;

    for (i = 0u; i < NDS_SSS_SLOTS; i++)
    {
        if ((u32)kNdsSssSlotGkind[i] == gkind)
        {
            return i;
        }
    }
    return 6u; /* nGRKindPupupu's slot: the only ground this build has. */
}

static s32 ndsMenuShellSssIconX(u32 slot)
{
    return (slot < NDS_SSS_ROW) ?
        (s32)((slot * (u32)NDS_SSS_ICON_PITCH)) + NDS_SSS_ICON_X0 :
        (s32)(((slot - NDS_SSS_ROW) * (u32)NDS_SSS_ICON_PITCH)) +
            NDS_SSS_ICON_X0;
}

static s32 ndsMenuShellSssIconY(u32 slot)
{
    return (slot < NDS_SSS_ROW) ? NDS_SSS_ICON_Y0 : NDS_SSS_ICON_Y1;
}

/* THE TWO SURFACES THE CURSOR OWNS, at most `budget` of them a frame.
 *
 * Shaped exactly like `ndsMenuShellVsSyncButtons`, and for the reason P2-1j
 * MEASURED there: re-blitting every changed surface on the frame the change
 * happens put 12,328 B of NitroFS inside one 60 Hz frame and priced it at
 * 606,336 ticks against a 560,190-tick budget. A move between Dream Land and
 * RANDOM changes BOTH surfaces (8,162 B of plaque and 11,748 B of preview), so
 * the same burst is available here; spending one a frame retires it in two
 * frames against a 12-tic scroll wait, and the CURSOR still moves on the press
 * because OAM costs nothing.
 *
 * A refused blit leaves the tracker alone so the next frame retries, rather
 * than recording a state the screen is not showing. */
static void ndsMenuShellSssSyncSurfaces(u32 budget)
{
    NdsUiKitSurfaceId list[2];
    NdsUiKitSurfaceId *tracker[2];
    u32 count = 0u;
    u32 i;
    NdsUiKitSurfaceId wanted;

    wanted = kNdsSssPlaqueSurface[sSssCursorSlot];
    if ((wanted != sSssPlaqueSurface) && (count < budget))
    {
        list[count] = wanted;
        tracker[count] = &sSssPlaqueSurface;
        count++;
    }
    wanted = kNdsSssPreviewSurface[sSssCursorSlot];
    if ((wanted != NDS_MENU_VS_SURFACE_NONE) &&
        (wanted != sSssPreviewSurface) && (count < budget))
    {
        list[count] = wanted;
        tracker[count] = &sSssPreviewSurface;
        count++;
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
        *tracker[i] = list[i];
    }
    gNdsMenuShellSssPlaqueBlitCount += count;
}

/* mnMapsMakeNameAndEmblem, mnmaps.c:1015: the name follows the cursor, and
 * RANDOM has no name (the source draws only the question-mark emblem for it).
 * The preview panel follows it too -- mnMapsMakePreview:1096. */
static void ndsMenuShellSssShowSelection(u32 budget)
{
    u32 gkind = (u32)kNdsSssSlotGkind[sSssCursorSlot];

    ndsMenuShellSssSyncSurfaces(budget);
    ndsUiKitSetSprite(NDS_SSS_SPRITE_CURSOR, NDS_MN_UI_KIT_IMAGE_MAP_CURSOR,
                      NDS_SSS_DS(ndsMenuShellSssIconX(sSssCursorSlot) +
                                 NDS_SSS_CURSOR_DX),
                      NDS_SSS_DS(ndsMenuShellSssIconY(sSssCursorSlot) +
                                 NDS_SSS_CURSOR_DY));
    gNdsMenuShellSssCursorSlot = sSssCursorSlot;
    gNdsMenuShellSssCursorGkind = gkind;
}

static void ndsMenuShellSssPopulate(void)
{
    /* P2-1L (6): THE GRID IS BACKDROP ART NOW. `mnMapsMakeIcons` draws a 48x36
     * icon on a 50x38 pitch (mnmaps.c:540/:546), which is a near-continuous
     * mosaic; P2-1f's OBJ cells were 30x23 at 5/8 inside that 4/5 footprint,
     * so eight columns and six rows of stone showed inside every cell. The
     * lock set is a compile-time constant, so the whole grid is fixed for the
     * life of the screen and belongs in the surface -- where 4/5 costs no OBJ
     * cell. P2-1L (9) took the preview panel the same way, so the CURSOR is
     * the only OBJ this screen still owns and `ndsMenuShellHideRows` (called by
     * the screen entry, before this) is the only thing that has to clear the
     * previous screen's slots.
     *
     * The ENTRY frame spends both surfaces at once, as the VS menu's entry
     * does: it is already the frame that blits 98,304 B of SSS_SCREEN, and a
     * screen whose first present had no stage name or preview on it would show
     * the panel empty for two frames. */
    ndsMenuShellSssShowSelection(2u);
}

/* One cell of travel, with the source's own wrap. `step` is +1 or -1 and the
 * scan is bounded by the row length -- see the generalisation note above. */
static u32 ndsMenuShellSssStepRow(u32 slot, s32 step)
{
    u32 base = (slot < NDS_SSS_ROW) ? 0u : NDS_SSS_ROW;
    u32 index = slot - base;
    u32 tries;

    for (tries = 0u; tries < NDS_SSS_ROW; tries++)
    {
        index = (step > 0) ? ((index + 1u) % NDS_SSS_ROW) :
                             ((index + NDS_SSS_ROW - 1u) % NDS_SSS_ROW);
        if (ndsMenuShellSssSlotLocked(base + index) == FALSE)
        {
            return base + index;
        }
    }
    return slot;
}

static void ndsMenuShellSssMoveTo(u32 slot)
{
    if (slot == sSssCursorSlot)
    {
        gNdsMenuShellSssBlockedCount++;
        return;
    }
    sSssCursorSlot = slot;
    gNdsMenuShellSssMoveCount++;
    ndsMenuShellSssCue(NDS_SSS_FGM_SCROLL2);
    /* Budget ZERO: the cursor moves on the press, the surfaces follow from the
     * next frame's sync. This frame already carries the FGM cue's own sample
     * read, and P2-1j's measurement is that a cue frame is exactly the wrong
     * one to hang a NitroFS blit on. */
    ndsMenuShellSssShowSelection(0u);
}

/* mnMapsSaveSceneData, mnmaps.c:1367 -- through the P2-1a descriptor, which is
 * the battle's only input, rather than straight into the scene data. The
 * descriptor's STAGE FIELD is the only thing this screen writes: the fighters
 * belong to the character select and the rules to the VS menu, and
 * ndsMatchConfigApply re-installs the whole struct so the other two halves
 * survive untouched. */
static void ndsMenuShellSssCommit(void)
{
    u32 slot_gkind = (u32)kNdsSssSlotGkind[sSssCursorSlot];
    u32 gkind = slot_gkind;

    if (slot_gkind == NDS_SSS_GKIND_RANDOM)
    {
        /* The source's own pick: a ground that is neither locked nor the one
         * already loaded (mnmaps.c:1377). THE SECOND CLAUSE IS UNSATISFIABLE
         * IN THIS BUILD -- there is exactly one unlocked ground and it is
         * always the one already loaded -- so the do-while is bounded and
         * falls back to dropping the no-repeat clause, which is the weaker of
         * the two and the one with no meaning when there is only one place to
         * go. With two or more grounds landed (P2-4) the first arm resolves
         * and the fallback never runs; gNdsMenuShellSssRandomFallbackCount is
         * what says which one did, rather than a comment claiming it. */
        u32 tries;

        gkind = NDS_SSS_GKIND_RANDOM; /* "unresolved" -- never a ground id */
        for (tries = 0u; tries < 32u; tries++)
        {
            /* mnmaps.c:1383 rolls syUtilsRandTimeUCharRange(9) -- every ground
             * id 0..8, not just the first five. The old Inishie+1 bound could
             * never roll Pupupu itself, which with one unlocked ground made
             * every try fail and left the fallback to pick Dream Land. */
            u32 pick = (u32)syUtilsRandTimeUCharRange(9);

            if ((ndsMenuShellSssGroundLocked(pick) == FALSE) &&
                (pick != (u32)gSCManagerSceneData.gkind))
            {
                gkind = pick;
                gNdsMenuShellSssRandomCount++;
                break;
            }
        }
        if (gkind == NDS_SSS_GKIND_RANDOM)
        {
            u32 scan;

            gkind = (u32)nGRKindPupupu;
            for (scan = 0u; scan <= (u32)nGRKindInishie; scan++)
            {
                if (ndsMenuShellSssGroundLocked(scan) == FALSE)
                {
                    gkind = scan;
                    break;
                }
            }
            gNdsMenuShellSssRandomFallbackCount++;
        }
    }
    gNdsMatchConfig.gkind = (u8)gkind;
    ndsMatchConfigApply(&gNdsMatchConfig);
    /* mnMapsSaveSceneData's second write: the CURSOR's own ground, 0xde and
     * all, so a return trip opens on the cell the player last chose
     * (mnMapsInitVars:1418 reads it back). ndsMatchConfigApply owns
     * gSCManagerSceneData.gkind and does not touch this field. */
    gSCManagerSceneData.maps_vsmode_gkind = (u8)slot_gkind;
    gNdsMenuShellSssCommitGkind = gkind;
    gNdsMenuShellSssCommitSlotGkind = slot_gkind;
    gNdsMenuShellSssCommitCount++;
}

static void ndsMenuShellUpdateSss(u32 held, u32 taps)
{
    /* ONE surface a frame, and never more (see ndsMenuShellSssSyncSurfaces).
     * Ahead of the input gate on purpose: a move made on the frame before the
     * gate closes still has its surfaces retired. A frame on which nothing
     * changed compares two bytes and returns. */
    ndsMenuShellSssSyncSurfaces(1u);

    /* mnMapsFuncRun:1436 gates EVERYTHING on ten tics having passed, which is
     * what stops the A that left the character select from being read again
     * here. sMenuTics is this screen's own frame counter. */
    if (sMenuTics < (u32)NDS_SSS_INPUT_ARM_TICS)
    {
        return;
    }
    if (sSssScrollWait != 0u)
    {
        sSssScrollWait--;
    }
    /* mnmaps.c:1450: the wait is forced to zero the moment no direction is
     * held, so a tap always acts at once. */
    if ((held & (NDS_INPUT_UP | NDS_INPUT_DOWN | NDS_INPUT_LEFT |
                 NDS_INPUT_RIGHT)) == 0u)
    {
        sSssScrollWait = 0u;
    }

    /* A or START. mnmaps.c:1464 takes both, and it commits BEFORE it cues. */
    if ((taps & (NDS_INPUT_A | NDS_INPUT_START)) != 0u)
    {
        ndsMenuShellSssCommit();
        ndsMenuShellSssCue(NDS_SSS_FGM_CONFIRM);
        gNdsMenuShellSssConfirmCount++;
#if NDS_P2_MENU_WALK
        /* A lap is a completed menu pass into the match, counted where the
         * pass actually ends. P2-1d counted it on the VS screen and P2-1e
         * moved it to the character select for the same reason: this screen
         * is now the last stop before the battle. */
        gNdsMenuShellWalkLoops++;
#endif
        ndsMenuShellGoto((u32)nSCKindVSBattle);
        return;
    }
    /* B. mnmaps.c:1481 -- it commits on the way out too, and it plays NOTHING. */
    if ((taps & NDS_INPUT_B) != 0u)
    {
        ndsMenuShellSssCommit();
        gNdsMenuShellSssBackCount++;
        ndsMenuShellGoto((u32)nSCKindPlayersVS);
        return;
    }

    if (sSssScrollWait != 0u)
    {
        return;
    }
    /* The four direction arms in the source's own order (mnmaps.c:1494 down):
     * UP and DOWN change ROW and refuse a locked destination outright; LEFT
     * and RIGHT cycle within the row. Each arm reloads the wait to twelve and
     * returns, so exactly one arm runs per frame -- including the arms that
     * refuse, which is why a blocked press still costs the repeat delay. */
    if ((held & NDS_INPUT_UP) != 0u)
    {
        if ((sSssCursorSlot >= NDS_SSS_ROW) &&
            (ndsMenuShellSssSlotLocked(sSssCursorSlot - NDS_SSS_ROW) == FALSE))
        {
            ndsMenuShellSssMoveTo(sSssCursorSlot - NDS_SSS_ROW);
        }
        else
        {
            gNdsMenuShellSssBlockedCount++;
        }
        sSssScrollWait = (u32)NDS_SSS_SCROLL_WAIT;
        return;
    }
    if ((held & NDS_INPUT_DOWN) != 0u)
    {
        if ((sSssCursorSlot < NDS_SSS_ROW) &&
            (ndsMenuShellSssSlotLocked(sSssCursorSlot + NDS_SSS_ROW) == FALSE))
        {
            ndsMenuShellSssMoveTo(sSssCursorSlot + NDS_SSS_ROW);
        }
        else
        {
            gNdsMenuShellSssBlockedCount++;
        }
        sSssScrollWait = (u32)NDS_SSS_SCROLL_WAIT;
        return;
    }
    if ((held & NDS_INPUT_LEFT) != 0u)
    {
        ndsMenuShellSssMoveTo(ndsMenuShellSssStepRow(sSssCursorSlot, -1));
        sSssScrollWait = (u32)NDS_SSS_SCROLL_WAIT;
        return;
    }
    if ((held & NDS_INPUT_RIGHT) != 0u)
    {
        ndsMenuShellSssMoveTo(ndsMenuShellSssStepRow(sSssCursorSlot, 1));
        sSssScrollWait = (u32)NDS_SSS_SCROLL_WAIT;
    }
    (void)taps;
}

/* mnMapsInitVars, mnmaps.c:1404. The cursor opens on the cell the last visit
 * chose -- `maps_vsmode_gkind`, which is 0xde after a RANDOM pick -- and the
 * harness seeds that field to nGRKindPupupu for a cold boot
 * (scene_harness.c:39). The source keys this on `scene_prev` being
 * nSCKindPlayersVS or nSCKindPlayers1PTraining; only the first exists here. */
static void ndsMenuShellSssInit(void)
{
    sSssCursorSlot =
        ndsMenuShellSssSlotOfGkind((u32)gSCManagerSceneData.maps_vsmode_gkind);
    if (ndsMenuShellSssSlotLocked(sSssCursorSlot) != FALSE)
    {
        sSssCursorSlot = ndsMenuShellSssSlotOfGkind((u32)nGRKindPupupu);
    }
    sSssScrollWait = 0u;
    /* The screen entry re-blits SSS_SCREEN over BG2, which erases whatever
     * plaque the last visit left there -- so the cached "which plaque is on
     * screen" must be invalidated with it, or a re-entry on the same slot
     * would skip the blit and show a stage select with no name on it. Same
     * reason NDS_MENU_VS_SURFACE_NONE exists for the gates and the buttons. */
    sSssPlaqueSurface = NDS_MENU_VS_SURFACE_NONE;
    sSssPreviewSurface = NDS_MENU_VS_SURFACE_NONE;
    sSssEnterCount++;
}

