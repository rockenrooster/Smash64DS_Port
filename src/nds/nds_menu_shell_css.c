/* --- Screen: VS character select (P2-1e) ---------------------------------
 *
 * mn/mnplayers/mnplayersvs.c. A pointer screen, not a row screen: a hand
 * cursor roams the frame, picks a player's TOKEN up off its portrait, carries
 * it, and drops it on the fighter that player will use. Everything below is
 * transcribed from that file; what is NOT transcribed is named where it is
 * dropped, and there is exactly one mechanism substitution, disclosed here:
 *
 * THE STICK. mnPlayersVSAdjustCursor moves the cursor by
 * `stick_range / 20` per frame with a deadzone of 8 (mnplayersvs.c:3090/:3113).
 * The DS has no analog stick, and the port already fixed the exchange rate:
 * `ndsControllerMapPad` gives a held D-pad direction `stick_range = +-80`
 * (src/port/controller_backend.c:60). 80/20 is 4, so a held direction moves the
 * cursor at exactly the speed a fully deflected N64 stick moves it, and the
 * only expression the DS loses is partial deflection. That is the port's own
 * established mapping composed with the source's own divisor, not a number
 * chosen here.
 *
 * THE FRAME. Every hit test, clamp and layout constant below is in the
 * SOURCE's 320x240 frame, unaltered -- the portrait pitch of 45, the row bands
 * (35,79) and (78,122), the 69 px panel pitch, the button rectangles at
 * y 127..145 and y 197..216, the BACK box at x 244..292. The DS screen is
 * exactly 0.8 of that frame on BOTH axes (256/320 = 192/240), so drawing is
 * one multiply: NDS_CSS_DS(v) = v * 4 / 5. Keeping the logic in source units
 * means no constant here had to be re-derived, and a probe reads back the same
 * numbers mnplayersvs.c is written in.
 *
 * WHAT IS LOCKED. The source gates a portrait on `gSCManagerBackupData
 * .fighter_mask` (mnplayersvs.c:296), which the harness sets to ALL because
 * this build's save data is a fully unlocked cart. The gate this build needs is
 * a different one with the same mechanism: which fighters EXIST. Ten of the
 * twelve are P2-3, so the mask is Mario|Fox and the other ten portraits draw
 * the source's own question-mark plate -- the thing it draws for a locked
 * fighter (mnplayersvs.c:374). Same bitmask over fkind, different bound.
 *
 * DELIBERATE NARROWINGS, each a plan non-goal rather than an omission:
 *   - TEAMS ARE NO LONGER A NARROWING. P2-2 adds the source RED/BLUE/GREEN
 *     selector on all four panels, team gate palettes, team costume/shade
 *     updates and READY same-team rejection. Single-console still has only one
 *     human cursor; additional human cursors belong to P3 wireless.
 *   - FREE-FOR-ALL COSTUME BUTTONS. C-buttons pick alternate costumes
 *     (mnplayersvs.c:3372); the DS pad has no C-buttons and P2-3 owns alternate
 *     costume input. Team costumes are not optional presentation: P2-2 applies
 *     the source's team costume IDs automatically when Team Battle is active.
 *   - THE TIME/STOCK ARROWS at the top of the source's CSS duplicate the VS
 *     menu P2-1d already transcribed, so the rules stay that screen's.
 *   - THE 5-MINUTE IDLE RETURN (mnplayersvs.c:4470) is attract behaviour and
 *     belongs to P2-7, exactly as it does on the mode select.
 *   - THE RECALL TOSS. mnPlayersVSPuckAdjustRecall arcs the token back over 11
 *     frames and re-grabs it at tic 11 (mnplayersvs.c:3927). The END STATE is
 *     transcribed -- the token is back in the cursor's hand -- and the arc is
 *     not; it is presentation, and this screen has no SObj velocity model.
 *   - STAGE SELECT is no longer a narrowing: P2-1f landed the screen, so the
 *     ready START takes the source's own `is_stage_select` branch to
 *     `nSCKindMaps` (mnplayersvs.c:4497). The branch's OTHER arm -- randomise
 *     the ground here and go straight to the battle -- is still not the
 *     source's, because eight of the nine grounds are P2-4; it is unreachable
 *     in every configuration this build ships. */

/* Source frame -> DS pixels. 256/320 == 192/240 == 4/5, exactly. */
#define NDS_CSS_DS(v) (((v) * 4) / 5)

#define NDS_CSS_SLOTS 4
#define NDS_CSS_PORTRAITS 12

/* mnPlayersVSAdjustCursor's clamps and its full-deflection step. */
#define NDS_CSS_CURSOR_X_MIN 0
#define NDS_CSS_CURSOR_X_MAX 280
#define NDS_CSS_CURSOR_Y_MIN 10
#define NDS_CSS_CURSOR_Y_MAX 205
#define NDS_CSS_CURSOR_STEP 4

/* nMNPlayersCursorStatus*, in the order mnPlayersVSUpdateCursor's own
 * cursor_offsets[] indexes them (mnplayersvs.c:1723). */
#define NDS_CSS_STATUS_POINTER 0u
#define NDS_CSS_STATUS_GRAB 1u
#define NDS_CSS_STATUS_HOVER 2u

/* mnPlayersVSMakeCursor's player-0 seat, mnplayersvs.c:3684. */
#define NDS_CSS_CURSOR_HOME_X 40
#define NDS_CSS_CURSOR_HOME_Y 170
/* mnPlayersVSMakePuck's parking spot for a slot with no fighter yet. */
#define NDS_CSS_PUCK_HOME_X 51
#define NDS_CSS_PUCK_HOME_Y 161
/* The offset a carried token keeps from the cursor, mnplayersvs.c:3529. */
#define NDS_CSS_PUCK_CARRY_DX 11
#define NDS_CSS_PUCK_CARRY_DY (-14)
/* mnPlayersVSSelectFighter's re-grab lockout, mnplayersvs.c:2837. */
#define NDS_CSS_REGRAB_TICS 30
/* mnPlayersVSFuncRun's ready-START delay and its one-second input guard. */
#define NDS_CSS_START_WAIT 30
#define NDS_CSS_START_ARM_TICS 60
/* mnPlayersVSDetectBack: B held for forty tics leaves, mnplayersvs.c:3247. */
#define NDS_CSS_BACK_HOLD_TICS 40
/* mnPlayersVSReadyProcUpdate: a 40-tic cycle, lit for the first 30. */
#define NDS_CSS_READY_BLINK 40
#define NDS_CSS_READY_LIT 30

/* THE CUES, by the source's own REGION_US FGM ids. Derived by parsing
 * gm/gmsound.h's gmFGMVoiceID enum with REGION_US honoured and cross-checked
 * against every id this tree already pins -- Escape 11, GuardOn 13, FoxLanding
 * 74, MarioLanding 77, UnkGrind4 85, AltitudeWarn 153, DeadExplodeL 154,
 * MenuSelect 158, MenuScroll1 163, MenuScroll2 164, MenuDenied 165,
 * TitlePressStart 157, GamePause 278 -- all thirteen landing where the tree
 * already has them before any new id was trusted.
 *
 * FIVE OF THESE ARE IN THE FGM PACK AND THREE ARE NOT, which the miss ring
 * proves rather than this comment asserting it: 164/165 came in with P2-1c-1,
 * 157 with P2-1d-1, and 486/499 (the Fox and Mario announcer names) and 618
 * (the crowd cheer) were already packed for the Results sequence. 121, 127 and
 * 167 -- the two dash sounds the CSS reuses as its grab/announce whooshes and
 * the player-slot whoosh -- are NOT packed, and neither is 512
 * (FREE-FOR-ALL). Row P2-1e-1 renders them; this seam asks with the real ids so
 * the gap is measured. */
#define NDS_CSS_FGM_ANNOUNCE_WHOOSH 121u /* nSYAudioFGMMarioDash        */
#define NDS_CSS_FGM_GRAB 127u            /* nSYAudioFGMSamusDash        */
#define NDS_CSS_FGM_PRESS_START 157u     /* nSYAudioFGMTitlePressStart  */
#define NDS_CSS_FGM_SCROLL2 164u         /* nSYAudioFGMMenuScroll2      */
#define NDS_CSS_FGM_DENIED 165u          /* nSYAudioFGMMenuDenied       */
#define NDS_CSS_FGM_SLOT_WHOOSH 167u     /* nSYAudioFGMPlayerSlotWhoosh */
#define NDS_CSS_VOICE_FREE_FOR_ALL 512u  /* nSYAudioVoiceAnnounceFreeForAll */
#define NDS_CSS_VOICE_CHEER 618u         /* nSYAudioVoicePublicCheer    */
/* nSYAudioBGMBattleSelect, mnplayersvs.c:4899. Not one of the five tracks the
 * BGM pack carries (0/12/16/22/44), so ndsAudioBgmPlay counts it in
 * gNdsAudioBgmUnsupportedTrackCount -- the same measured gap, on the BGM side.
 * P2-1e-1 renders it. */
#define NDS_CSS_BGM_BATTLE_SELECT 10

/* mnPlayersVSAnnounceFighter's own table, indexed by fkind (mnplayersvs.c:2547
 * -- twelve entries, transcribed whole rather than trimmed to the two fighters
 * that exist, because it is the SOURCE's array and trimming it would make P2-3
 * edit this file again for no benefit). */
static const u16 kNdsCssAnnounceVoice[NDS_CSS_PORTRAITS] = {
    499u, 486u, 483u, 513u, 498u, 497u,
    535u, 485u, 496u, 507u, 508u, 501u
};

/* mnPlayersVSGetFighterKind: which fighter each of the twelve portrait cells
 * holds (mnplayersvs.c:2120). Cells 0-5 are the top row, 6-11 the bottom. */
static const u8 kNdsCssPortraitFighter[NDS_CSS_PORTRAITS] = {
    (u8)nFTKindLuigi, (u8)nFTKindMario, (u8)nFTKindDonkey,
    (u8)nFTKindLink, (u8)nFTKindSamus, (u8)nFTKindCaptain,
    (u8)nFTKindNess, (u8)nFTKindYoshi, (u8)nFTKindKirby,
    (u8)nFTKindFox, (u8)nFTKindPikachu, (u8)nFTKindPurin
};

/* mnPlayersVSGetPortrait, the inverse (mnplayersvs.c:2168). */
static const u8 kNdsCssFighterPortrait[NDS_CSS_PORTRAITS] = {
    1u, 9u, 2u, 4u, 0u, 3u, 7u, 5u, 8u, 10u, 11u, 6u
};

/* Which fighters this build HAS. Same shape as the source's fighter_mask; a
 * production fighter is admitted here only after its renderer/CSS/audio seams
 * all exist in the same configuration. */
#if NDS_P2_LINK
#define NDS_CSS_FIGHTER_MASK \
    (LBBACKUP_MASK_FIGHTER(nFTKindMario) | LBBACKUP_MASK_FIGHTER(nFTKindFox) | \
     LBBACKUP_MASK_FIGHTER(nFTKindLuigi) | \
     LBBACKUP_MASK_FIGHTER(nFTKindDonkey) | \
     LBBACKUP_MASK_FIGHTER(nFTKindCaptain) | \
     LBBACKUP_MASK_FIGHTER(nFTKindSamus) | \
     LBBACKUP_MASK_FIGHTER(nFTKindLink))
#elif NDS_P2_SAMUS
#define NDS_CSS_FIGHTER_MASK \
    (LBBACKUP_MASK_FIGHTER(nFTKindMario) | LBBACKUP_MASK_FIGHTER(nFTKindFox) | \
     LBBACKUP_MASK_FIGHTER(nFTKindLuigi) | \
     LBBACKUP_MASK_FIGHTER(nFTKindDonkey) | \
     LBBACKUP_MASK_FIGHTER(nFTKindCaptain) | \
     LBBACKUP_MASK_FIGHTER(nFTKindSamus))
#elif NDS_P2_CAPTAIN
#define NDS_CSS_FIGHTER_MASK \
    (LBBACKUP_MASK_FIGHTER(nFTKindMario) | LBBACKUP_MASK_FIGHTER(nFTKindFox) | \
     LBBACKUP_MASK_FIGHTER(nFTKindLuigi) | \
     LBBACKUP_MASK_FIGHTER(nFTKindDonkey) | \
     LBBACKUP_MASK_FIGHTER(nFTKindCaptain))
#elif NDS_P2_DONKEY
#define NDS_CSS_FIGHTER_MASK \
    (LBBACKUP_MASK_FIGHTER(nFTKindMario) | LBBACKUP_MASK_FIGHTER(nFTKindFox) | \
     LBBACKUP_MASK_FIGHTER(nFTKindLuigi) | \
     LBBACKUP_MASK_FIGHTER(nFTKindDonkey))
#elif NDS_P2_LUIGI
#define NDS_CSS_FIGHTER_MASK \
    (LBBACKUP_MASK_FIGHTER(nFTKindMario) | LBBACKUP_MASK_FIGHTER(nFTKindFox) | \
     LBBACKUP_MASK_FIGHTER(nFTKindLuigi))
#else
#define NDS_CSS_FIGHTER_MASK \
    (LBBACKUP_MASK_FIGHTER(nFTKindMario) | LBBACKUP_MASK_FIGHTER(nFTKindFox))
#endif

static u8 sCssPkind[NDS_CSS_SLOTS];
static u8 sCssFkind[NDS_CSS_SLOTS];
static u8 sCssLevel[NDS_CSS_SLOTS];
static u8 sCssTeam[NDS_CSS_SLOTS];
static u8 sCssSelected[NDS_CSS_SLOTS]; /* is_fighter_selected */
static s16 sCssPuckX[NDS_CSS_SLOTS];
static s16 sCssPuckY[NDS_CSS_SLOTS];
static s32 sCssCursorX;
static s32 sCssCursorY;
static u32 sCssStatus;
static s32 sCssHeld;      /* slot whose token the cursor carries, -1 = none */
static u32 sCssRegrabTic;
static u32 sCssBackTics;
/* The FFA/Team-Battle mode, seeded from the transfer state on CSS entry the way
 * the source seeds sMNPlayersVSIsTeamBattle (mnplayersvs.c:4679), flipped by
 * the mode-label toggle, and written back through the descriptor on commit.
 * P2-2 also makes sCssTeam[] live through the source RED/BLUE/GREEN controls. */
static u8 sCssIsTeamBattle;
/* P2-1N (3): the shutter, in the source's own units. door_offset slides
 * 2/tic between 0 (open — halves parked at the panel edges, hidden by the
 * frame) and 41 (shut — halves meeting at +22/+47, which is exactly the
 * baked NA gate). mnPlayersVSShutterProcUpdate/mnPlayersVSUpdateShutter. */
static u8 sCssDoorOffset[NDS_CSS_SLOTS];
static u32 sCssStartWait;
static u32 sCssReadyBlink;
static u32 sCssReadyShown;
/* mnPlayersVSMakePortraitFlash owns one independently-timed flash GObj per
 * player.  The GObj's position is fixed when it is created, so keep the
 * Mario/Fox content id separately from sCssFkind[]: a later kind/holder change
 * must not drag an already-live flash to a different portrait. */
static u8 sCssFlashRemain[NDS_CSS_SLOTS];
static u8 sCssFlashVisible[NDS_CSS_SLOTS];
static u8 sCssFlashKind[NDS_CSS_SLOTS];
static u8 sCssFlashShown[4];

/* One cursor: the DS has one keypad, so exactly one player has a controller.
 * mnPlayersVSUpdateControllerOrders would report orders[0] = 0 and -1 for the
 * rest, and every branch below that reads a controller order is written from
 * that fact rather than from a general N-cursor model. */
#define NDS_CSS_CURSOR_SLOT 0

/* --- Audio seam ---------------------------------------------------------- */

static void ndsMenuShellCssCue(u32 id)
{
    gNdsMenuShellCssCueCount++;
    gNdsMenuShellCssCueLastId = id;
    /* Unguarded, like the kit's own seam: NDS_IMPORT_BATTLESHIP_AUDIO_FGM is an
     * unconditional Makefile override, so this symbol always exists. */
    (void)ndsAudioFgmPlay((u16)id);
}

/* P2-1N (4): the mode announcer voice, shared by the source's entry tail and
 * mode-toggle handler — FREE FOR ALL (512) or TEAM BATTLE (526). */
static void ndsMenuShellCssAnnounceMode(void)
{
    ndsMenuShellCssCue((sCssIsTeamBattle != FALSE) ? 526u : 512u);
}

/* mnPlayersVSAnnounceFighter, mnplayersvs.c:2545: the whoosh then the name. */
static void ndsMenuShellCssAnnounce(u32 slot)
{
    u32 fkind = (u32)sCssFkind[slot];

    if (fkind >= NDS_CSS_PORTRAITS)
    {
        return;
    }
    ndsMenuShellCssCue(NDS_CSS_FGM_ANNOUNCE_WHOOSH);
    ndsMenuShellCssCue((u32)kNdsCssAnnounceVoice[fkind]);
    gNdsMenuShellCssAnnounceCount++;
}

/* --- Source geometry ----------------------------------------------------- */

static u32 ndsMenuShellCssFighterLocked(u32 fkind)
{
    if (fkind >= NDS_CSS_PORTRAITS)
    {
        return TRUE;
    }
    return ((NDS_CSS_FIGHTER_MASK & (1u << fkind)) != 0u) ? FALSE : TRUE;
}

/* mnPlayersVSMakePortraitShadow's own placement, mnplayersvs.c:2412. */
static s32 ndsMenuShellCssPortraitX(u32 portrait)
{
    return (s32)(((portrait >= 6u) ? (portrait - 6u) : portrait) * 45u) + 25;
}

static s32 ndsMenuShellCssPortraitY(u32 portrait)
{
    return (s32)(((portrait >= 6u) ? 1u : 0u) * 43u) + 36;
}

/* mnPlayersVSCenterPuckInPortrait, mnplayersvs.c:3454. */
static void ndsMenuShellCssCenterPuck(u32 slot, u32 fkind)
{
    u32 portrait;

    if (fkind >= NDS_CSS_PORTRAITS)
    {
        sCssPuckX[slot] = (s16)NDS_CSS_PUCK_HOME_X;
        sCssPuckY[slot] = (s16)NDS_CSS_PUCK_HOME_Y;
        return;
    }
    portrait = (u32)kNdsCssFighterPortrait[fkind];
    if (portrait >= 6u)
    {
        sCssPuckX[slot] = (s16)((portrait * 45u) - (6u * 45u) + 36u);
        sCssPuckY[slot] = (s16)89;
    }
    else
    {
        sCssPuckX[slot] = (s16)((portrait * 45u) + 36u);
        sCssPuckY[slot] = (s16)46;
    }
}

/* mnPlayersVSGetPuckFighterKind, mnplayersvs.c:3001 -- which fighter the
 * token's CENTRE is over, or nFTKindNull. */
static u32 ndsMenuShellCssPuckFighterKind(u32 slot)
{
    s32 x = (s32)sCssPuckX[slot] + 13;
    s32 y = (s32)sCssPuckY[slot] + 12;
    u32 fkind;

    if ((x <= 24) || (x >= 295))
    {
        return (u32)nFTKindNull;
    }
    if ((y > 35) && (y < 79))
    {
        fkind = (u32)kNdsCssPortraitFighter[(x - 25) / 45];
    }
    else if ((y > 78) && (y < 122))
    {
        fkind = (u32)kNdsCssPortraitFighter[((x - 25) / 45) + 6];
    }
    else
    {
        return (u32)nFTKindNull;
    }
    return (ndsMenuShellCssFighterLocked(fkind) != FALSE) ?
        (u32)nFTKindNull : fkind;
}

/* mnPlayersVSCheckPuckInRange, mnplayersvs.c:2157 -- the cursor's puck hotspot
 * is (+25, +3) and a token is 26x24. */
static u32 ndsMenuShellCssPuckInRange(u32 slot)
{
    s32 x = sCssCursorX + 25;
    s32 y = sCssCursorY + 3;

    if ((x < (s32)sCssPuckX[slot]) || (x > ((s32)sCssPuckX[slot] + 26)))
    {
        return FALSE;
    }
    if ((y < (s32)sCssPuckY[slot]) || (y > ((s32)sCssPuckY[slot] + 24)))
    {
        return FALSE;
    }
    return TRUE;
}

/* The BUTTON hotspot is (+20, +3), a different offset from the token one --
 * mnPlayersVSCheckPlayerKindSelectInRange:2138 against :2163. */
static u32 ndsMenuShellCssBoxHit(s32 x0, s32 x1, s32 y0, s32 y1)
{
    s32 x = sCssCursorX + 20;
    s32 y = sCssCursorY + 3;

    return ((x >= x0) && (x <= x1) && (y >= y0) && (y <= y1)) ? TRUE : FALSE;
}

/* --- Drawing ------------------------------------------------------------- */

/* Text slots. P2-1k gave the four per-slot fighter names back: they are source
 * art baked into the gate surface now, so this screen holds four of the eight
 * and the other four are free for the next element that needs one. */
#define NDS_CSS_SLOT_MODE 0u
#define NDS_CSS_SLOT_BACK 1u
#define NDS_CSS_SLOT_READY 2u

/* Sprite slots, in DEPTH order: lower OAM ids draw in front. The source's
 * REGION_US PRESS/START GObj is display link 28 (player-kind camera priority
 * 50), not the link-38 READY banner GObj. It therefore stays behind cursor 20
 * and puck 25 just like the other link-28 panel art. The banner itself is
 * reinforced on foreground BG3 below so its separate camera-10 depth is kept. */
#define NDS_CSS_SPRITE_CURSOR_TAG 0u
#define NDS_CSS_SPRITE_CURSOR 1u
#define NDS_CSS_SPRITE_PUCK0 2u
#define NDS_CSS_SPRITE_KIND0 6u
#define NDS_CSS_SPRITE_LEVEL0 10u
#define NDS_CSS_SPRITE_DIGIT0 14u
/* P2-1j, owner finding (e): the player panel's own art. The tag is the
 * mnPlayersVSMakePlayerKind sprite (1P / CP, :2003), the colon and the two
 * blinking arrows are mnPlayersVSMakeHandicapLevel's and
 * mnPlayersVSArrowThreadUpdate's (:2696/:2626). All four are behind the
 * tokens and the cursor in id order, which is the depth the source draws them
 * at (display 35 under the puck's 37). */
#define NDS_CSS_SPRITE_READY_PRESS 18u
#define NDS_CSS_SPRITE_READY_START 19u
#define NDS_CSS_SPRITE_TAG0 20u
#define NDS_CSS_SPRITE_COLON0 24u
#define NDS_CSS_SPRITE_ARROWL0 28u
#define NDS_CSS_SPRITE_ARROWR0 32u

/* mnPlayersVSArrowThreadUpdate's own blink (10 tics on, 10 off) and the two
 * values at which an arrow is EJECTED rather than blinked (:2643/:2660). */
#define NDS_CSS_ARROW_BLINK 10u
#define NDS_CSS_LEVEL_MIN 1u
#define NDS_CSS_LEVEL_MAX 9u

/* mnPlayersVSMakeLabels' own colours, mnplayersvs.c:1391. */
#define NDS_CSS_RGB_MODE 0x00e3ac04u
/* mnPlayersVSMakeReady's ready-text primitive, mnplayersvs.c:4128. */

static u32 ndsMenuShellCssKindImage(u32 pkind)
{
    /* LABEL_HMN/CP/NA are baked as three consecutive entries in nFTPlayerKind
     * order, which the generator asserts (check_kind_label_block), so this is
     * the source's own offsets[pkind] indexing (mnplayersvs.c:934). */
    if (pkind > (u32)nFTPlayerKindNot)
    {
        pkind = (u32)nFTPlayerKindNot;
    }
    return NDS_MN_UI_KIT_IMAGE_LABEL_HMN + pkind;
}

/* P2-1j, owner finding (e), EXTENDED BY P2-1k (a)/(b). THE PANEL IS ONE CARD
 * AND EIGHT PALETTES in the source -- `mnPlayersVSSetGateLUT`
 * (mnplayersvs.c:900) swaps the CI4 gate card's TLUT between `GateMan<N>PLUT`
 * and `GateCom<N>PLUT` -- and an EMPTY slot additionally has its two shutter
 * doors closed over it (door_offset 41, :640). A direct-colour DS bitmap has no
 * palette to swap, so the bake resolves each reachable result per slot and the
 * runtime blits the one the slot's state names.
 *
 * P2-1k adds the two sprites `mnPlayersVSMakeGate` builds that this shell had
 * never drawn -- the owner's "the preview boxes aren't rendering the fighter".
 * `mnPlayersVSMakeNameAndEmblem` (:578) puts the SERIES EMBLEM at (p*69+24,
 * 143), tinted 0x1E for a human slot and 0x44 otherwise, and the FIGHTER NAME
 * at (p*69+22, 201). They are baked INTO the card rather than drawn as OBJ
 * cells because one fighter's pair needs 10,240 B of main OBJ against the
 * 3,456 B P2-1j left free, and because folding them in keeps every state of a
 * slot exactly one overwriting blit.
 *
 * THREE BASE STATES PLUS THREE STATES PER LANDED FIGHTER. The third fighter
 * state is the source's, not an invention: the NAME and the CP LEVEL row
 * occupy the SAME row (y 201), and
 * `mnPlayersVSUpdateHandicapLevel` opens by hiding the name (:2788) while
 * `mnPlayersVSHandicapLevelProcUpdate` destroys the level row the moment the
 * slot stops being a settled fighter (:2689). So a settled CPU shows CP LEVEL
 * (drawn as OBJ over this surface) and a CPU whose token is in the cursor's
 * hand shows its name again. */
#define NDS_CSS_GATE_NA 0u
#define NDS_CSS_GATE_MAN 1u
#define NDS_CSS_GATE_COM 2u
#define NDS_CSS_GATE_FIGHTERS 7u
#define NDS_CSS_GATE_MAN_F0 3u
#define NDS_CSS_GATE_COM_F0 (NDS_CSS_GATE_MAN_F0 + NDS_CSS_GATE_FIGHTERS)
#define NDS_CSS_GATE_HOLD_F0 (NDS_CSS_GATE_COM_F0 + NDS_CSS_GATE_FIGHTERS)
#define NDS_CSS_GATE_STATES (NDS_CSS_GATE_HOLD_F0 + NDS_CSS_GATE_FIGHTERS)

/* The bake emits FFA states in [player][state] order, just as its Team block
 * is [team][player][state]. Keep both arithmetic: it scales with the landed
 * roster and lets the generated enum itself prove contiguity instead of
 * duplicating an ever-growing hand table here. */
_Static_assert(NDS_MN_UI_KIT_SURFACE_CSS_GATE_1_NA ==
                   NDS_MN_UI_KIT_SURFACE_CSS_GATE_0_NA + NDS_CSS_GATE_STATES,
               "FFA gate surfaces must stay contiguous by player");
_Static_assert(NDS_MN_UI_KIT_SURFACE_CSS_GATE_3_HOLD_LINK ==
                   NDS_MN_UI_KIT_SURFACE_CSS_GATE_0_NA +
                       (NDS_CSS_SLOTS * NDS_CSS_GATE_STATES) - 1u,
               "FFA gate block must contain all landed fighter states");

/* P2-2a: the generated Team-Battle panel variants are intentionally contiguous
 * in [team][player][state] order. BattleShip maps team ids Red/Blue/Green to
 * gate color ids 0/1/3. The RED/BLUE/GREEN selector itself is a SEPARATE source
 * display object on DL 34, in front of the fighter camera on DL 33, so its
 * twelve exact baked rasters live in a separate foreground block below. */
#define NDS_CSS_TEAM_COUNT 3u
#define NDS_CSS_TEAM_GATE_STRIDE (NDS_CSS_SLOTS * NDS_CSS_GATE_STATES)
_Static_assert(nSCBattleTeamIDRed == 0 && nSCBattleTeamIDBlue == 1 &&
                   nSCBattleTeamIDGreen == 2,
               "CSS team surface arithmetic follows BattleShip team ids");
_Static_assert(NDS_MN_UI_KIT_SURFACE_CSS_GATE_TEAM_BLUE_0_NA ==
                   NDS_MN_UI_KIT_SURFACE_CSS_GATE_TEAM_RED_0_NA +
                       NDS_CSS_TEAM_GATE_STRIDE,
               "team gate surfaces must stay contiguous by team");
_Static_assert(NDS_MN_UI_KIT_SURFACE_CSS_GATE_TEAM_GREEN_0_NA ==
                   NDS_MN_UI_KIT_SURFACE_CSS_GATE_TEAM_RED_0_NA +
                       (2u * NDS_CSS_TEAM_GATE_STRIDE),
               "team gate surfaces must stay contiguous by team");
_Static_assert(NDS_MN_UI_KIT_SURFACE_CSS_GATE_TEAM_GREEN_3_HOLD_LINK ==
                   NDS_MN_UI_KIT_SURFACE_CSS_GATE_TEAM_RED_0_NA +
                       (NDS_CSS_TEAM_COUNT * NDS_CSS_TEAM_GATE_STRIDE) - 1u,
               "team gate surface block must contain every landed fighter state");

#define NDS_CSS_TEAM_SELECT_STRIDE NDS_CSS_SLOTS
_Static_assert(NDS_MN_UI_KIT_SURFACE_CSS_TEAM_SELECT_BLUE_0 ==
                   NDS_MN_UI_KIT_SURFACE_CSS_TEAM_SELECT_RED_0 +
                       NDS_CSS_TEAM_SELECT_STRIDE,
               "team selector surfaces must stay contiguous by team");
_Static_assert(NDS_MN_UI_KIT_SURFACE_CSS_TEAM_SELECT_GREEN_3 ==
                   NDS_MN_UI_KIT_SURFACE_CSS_TEAM_SELECT_RED_0 +
                       (NDS_CSS_TEAM_COUNT * NDS_CSS_TEAM_SELECT_STRIDE) - 1u,
               "team selector surface block must contain 3x4 variants");

static NdsUiKitSurfaceId sCssPanelSurface[NDS_CSS_SLOTS];
static u32 sCssArrowsShown;

static u32 ndsMenuShellCssGateState(u32 slot);

static NdsUiKitSurfaceId ndsMenuShellCssGateSurfaceForState(u32 slot, u32 state)
{
    u32 team;

    if (sCssIsTeamBattle == FALSE)
    {
        return (NdsUiKitSurfaceId)(NDS_MN_UI_KIT_SURFACE_CSS_GATE_0_NA +
                                   (slot * NDS_CSS_GATE_STATES) + state);
    }
    team = (u32)sCssTeam[slot];
    if (team >= NDS_CSS_TEAM_COUNT)
    {
        team = (u32)nSCBattleTeamIDRed;
    }
    return (NdsUiKitSurfaceId)(
        NDS_MN_UI_KIT_SURFACE_CSS_GATE_TEAM_RED_0_NA +
        (team * NDS_CSS_TEAM_GATE_STRIDE) +
        (slot * NDS_CSS_GATE_STATES) + state);
}

static NdsUiKitSurfaceId ndsMenuShellCssGateSurface(u32 slot)
{
    return ndsMenuShellCssGateSurfaceForState(
        slot, ndsMenuShellCssGateState(slot));
}

static s32 ndsMenuShellCssRoundSource(s32 value)
{
    return (value * 4 + 2) / 5;
}

static NdsUiKitSurfaceId ndsMenuShellCssTeamSelectSurface(u32 slot)
{
    u32 team = (u32)sCssTeam[slot];

    if (team >= NDS_CSS_TEAM_COUNT)
    {
        team = (u32)nSCBattleTeamIDRed;
    }
    return (NdsUiKitSurfaceId)(NDS_MN_UI_KIT_SURFACE_CSS_TEAM_SELECT_RED_0 +
                               (team * NDS_CSS_TEAM_SELECT_STRIDE) + slot);
}

static void ndsMenuShellCssClearTeamSelect(u32 slot)
{
    s32 source_x = (s32)(slot * 69u) + 30;

    /* The source sprite sits at x+34,y=131 and its hit rectangle ends at
     * x+58,y=141. Clear a deliberately slightly wider transparent BG3 pocket
     * so antialiased/keyed edge pixels from the previous team cannot survive a
     * color change. No other CSS foreground owner occupies this rectangle. */
    ndsUiKitClearForegroundRect(ndsMenuShellCssRoundSource(source_x),
                                ndsMenuShellCssRoundSource(128),
                                (u32)ndsMenuShellCssRoundSource(34),
                                (u32)ndsMenuShellCssRoundSource(16));
}

static void ndsMenuShellCssDrawTeamSelect(u32 slot)
{
    NdsUiKitSurfaceId surface;

    ndsMenuShellCssClearTeamSelect(slot);
    if (sCssIsTeamBattle == FALSE)
    {
        return;
    }
    surface = ndsMenuShellCssTeamSelectSurface(slot);
    if (ndsUiKitBlitForegroundSurfaces(&surface, 1u) != FALSE)
    {
        gNdsMenuShellCssPanelBlitCount++;
    }
}

static void ndsMenuShellCssSyncTeamSelectAll(void)
{
    u32 slot;

    for (slot = 0u; slot < (u32)NDS_CSS_SLOTS; slot++)
    {
        ndsMenuShellCssDrawTeamSelect(slot);
    }
}

/* Which of the nine the slot is in, transcribed from the two source predicates
 * above: the GObj is hidden entirely for an empty slot
 * (`mnPlayersVSUpdateNameAndEmblem`, :2401) and draws nothing when the fighter
 * is Null (`mnPlayersVSMakeNameAndEmblem`, :609); a settled CPU has its name
 * hidden by the level row and an unsettled one has it back. */
static u32 ndsMenuShellCssGateState(u32 slot)
{
    u32 fkind = (u32)sCssFkind[slot];
    u32 fighter;

    if (sCssPkind[slot] == (u8)nFTPlayerKindNot)
    {
        return NDS_CSS_GATE_NA;
    }
    if (fkind == (u32)nFTKindMario)
    {
        fighter = 0u;
    }
    else if (fkind == (u32)nFTKindFox)
    {
        fighter = 1u;
    }
#if NDS_P2_LUIGI
    else if (fkind == (u32)nFTKindLuigi)
    {
        fighter = 2u;
    }
#endif
#if NDS_P2_DONKEY
    else if (fkind == (u32)nFTKindDonkey)
    {
        fighter = 3u;
    }
#endif
#if NDS_P2_CAPTAIN
    else if (fkind == (u32)nFTKindCaptain)
    {
        fighter = 4u;
    }
#endif
#if NDS_P2_SAMUS
    else if (fkind == (u32)nFTKindSamus)
    {
        fighter = 5u;
    }
#endif
#if NDS_P2_LINK
    else if (fkind == (u32)nFTKindLink)
    {
        fighter = 6u;
    }
#endif
    else
    {
        return (sCssPkind[slot] == (u8)nFTPlayerKindCom) ?
            NDS_CSS_GATE_COM : NDS_CSS_GATE_MAN;
    }
    if (sCssPkind[slot] != (u8)nFTPlayerKindCom)
    {
        return NDS_CSS_GATE_MAN_F0 + fighter;
    }
    return ((sCssSelected[slot] != 0u) ? NDS_CSS_GATE_COM_F0 :
                                         NDS_CSS_GATE_HOLD_F0) + fighter;
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
static void ndsMenuShellCssStepDoors(void)
{
    u32 i;

    for (i = 0u; i < (u32)NDS_CSS_SLOTS; i++)
    {
        s32 start = (s32)(i * 69u);
        u32 target = (sCssPkind[i] == (u8)nFTPlayerKindNot) ? 41u : 0u;
        u32 offset = (u32)sCssDoorOffset[i];
        NdsUiKitSurfaceId blit;

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
        if ((offset == 41u) || (offset == 0u))
        {
            blit = ndsMenuShellCssGateSurface(i);
            if (ndsUiKitBlitSurfaces(&blit, 1u) != FALSE)
            {
                sCssPanelSurface[i] = blit;
                gNdsMenuShellCssPanelBlitCount++;
            }
            continue;
        }
        blit = sCssPanelSurface[i];
        if (ndsUiKitBlitSurfaces(&blit, 1u) != FALSE)
        {
            gNdsMenuShellCssPanelBlitCount++;
        }
        {
            /* THE BAKE'S OWN ROUNDING, not NDS_CSS_DS: frame_pos rounds half
             * up while the plain macro truncates, and the two disagree by a
             * pixel exactly where it shows — the right half drew 32 of its
             * 33 columns and the pair sat one row above the gate (owner
             * screenshot, 2026-08-19). One rule for every door number. */
            #define NDS_CSS_DSR(v) (((v) * 4 + 2) / 5)
            s32 clip0 = NDS_CSS_DSR(start + 22);
            s32 clip1 = NDS_CSS_DSR(start + 88);
            s32 dy = NDS_CSS_DSR(126);
            /* Both source door sprites are 41x92. Closed, the left occupies
             * source x=22..62 and the right x=47..87, overlapping by 16 px;
             * their alpha masks form the source's zigzag seam. The cache keeps
             * both complete masks side-by-side (41+41) and these destination
             * positions recreate that overlap. Cutting the cache at x=25 was
             * the old straight-seam defect: it discarded those 16 left-door
             * pixels before the two source masks could interlock. */
            u32 split = (u32)NDS_CSS_DSR(41);
            u32 strip_w = (u32)NDS_CSS_DSR(41 + 41);

            (void)ndsUiKitDrawCachedSub(0u, split,
                                        NDS_CSS_DSR(start - 19 + (s32)offset),
                                        dy, clip0, clip1);
            (void)ndsUiKitDrawCachedSub(split, strip_w - split,
                                        NDS_CSS_DSR(start + 88 - (s32)offset),
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

    for (i = 0u; i < (u32)NDS_CSS_SLOTS; i++)
    {
        s32 panel = (s32)(i * 69u);
        u32 level = (u32)sCssLevel[i];
        u32 live = ((sCssPkind[i] == (u8)nFTPlayerKindCom) &&
                    (sCssSelected[i] != 0u) && (sCssArrowsShown != FALSE)) ?
            TRUE : FALSE;

        if ((live != FALSE) && (level > NDS_CSS_LEVEL_MIN))
        {
            ndsUiKitSetSprite(NDS_CSS_SPRITE_ARROWL0 + i,
                              NDS_MN_UI_KIT_IMAGE_CSS_ARROW_L,
                              NDS_CSS_DS(panel + 25), NDS_CSS_DS(201));
        }
        else
        {
            ndsUiKitHideSprite(NDS_CSS_SPRITE_ARROWL0 + i);
        }
        if ((live != FALSE) && (level < NDS_CSS_LEVEL_MAX))
        {
            ndsUiKitSetSprite(NDS_CSS_SPRITE_ARROWR0 + i,
                              NDS_MN_UI_KIT_IMAGE_CSS_ARROW_R,
                              NDS_CSS_DS(panel + 79), NDS_CSS_DS(201));
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

    /* P2-1N (2)+(4): the mode label and BACK ship the source's own sprites
     * (`mnPlayersVSMakeLabels` at (27,24) with the toggle's FFA tint;
     * `llMNPlayersCommonBackButtonSprite` at (244,23)), blitted as baked
     * states over the base. The mode label is a two-state surface so the
     * FFA/Team toggle is a swap; BACK is static and blits once here. */
    {
        NdsUiKitSurfaceId entry_states[2];

        entry_states[0] = (sCssIsTeamBattle != FALSE) ?
            NDS_MN_UI_KIT_SURFACE_CSS_MODE_TEAM :
            NDS_MN_UI_KIT_SURFACE_CSS_MODE_FFA;
        entry_states[1] = NDS_MN_UI_KIT_SURFACE_CSS_BACK;
        if (ndsUiKitBlitSurfaces(entry_states, 2u) != FALSE)
        {
            gNdsMenuShellCssPanelBlitCount += 2u;
        }
    }

    /* P2-1L (5): EVERY portrait cell is backdrop art now. The old twelve OBJ
     * slots were dead after that migration and are no longer reserved; P2-1
     * closeout reuses the resulting OAM id space for source cursor/READY art. */

    for (i = 0u; i < (u32)NDS_CSS_SLOTS; i++)
    {
        s32 panel = (s32)(i * 69u);

        /* The player-kind button, mnplayersvs.c:963. */
        ndsUiKitSetSprite(NDS_CSS_SPRITE_KIND0 + i,
                          ndsMenuShellCssKindImage((u32)sCssPkind[i]),
                          NDS_CSS_DS(panel + 64), NDS_CSS_DS(131));

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
         * the team plate. */
        if (sCssPkind[i] == (u8)nFTPlayerKindCom)
        {
            (void)ndsUiKitSetSpriteBlend(
                NDS_CSS_SPRITE_TAG0 + i, NDS_MN_UI_KIT_IMAGE_PANEL_CP,
                NDS_CSS_DS(panel + 26), NDS_CSS_DS(131), 15u, 0u, 2u);
        }
        else if (sCssPkind[i] == (u8)nFTPlayerKindMan)
        {
            (void)ndsUiKitSetSpriteBlend(
                NDS_CSS_SPRITE_TAG0 + i, NDS_MN_UI_KIT_IMAGE_PANEL_1P,
                NDS_CSS_DS(panel + 30), NDS_CSS_DS(131), 15u, 0u, 2u);
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

        /* CP LEVEL and its value, mnplayersvs.c:2762/:2790. The source shows
         * this row when the slot is a selected CPU, or when handicap is on for
         * the cursor's own human slot -- handicap is Off in every configuration
         * this build reaches (the descriptor's handicap_mode), so only the CPU
         * arm can run and only it is drawn. */
        if ((sCssPkind[i] == (u8)nFTPlayerKindCom) && (sCssSelected[i] != 0u))
        {
            ndsUiKitSetSprite(NDS_CSS_SPRITE_LEVEL0 + i,
                              NDS_MN_UI_KIT_IMAGE_CP_LEVEL,
                              NDS_CSS_DS(panel + 34), NDS_CSS_DS(201));
            /* P2-1j (e): the colon between label and value is its own sprite
             * in the source too (llMNCommonColonSprite at `p*69+61`, y 202,
             * white -- mnplayersvs.c:2740), and it was simply absent. */
            ndsUiKitSetSprite(NDS_CSS_SPRITE_COLON0 + i,
                              NDS_MN_UI_KIT_IMAGE_COLON,
                              NDS_CSS_DS(panel + 61), NDS_CSS_DS(202));
            (void)ndsUiKitSetNumber(NDS_CSS_SPRITE_DIGIT0 + i, 1u,
                                    (s32)sCssLevel[i],
                                    NDS_CSS_DS(panel + 67) +
                                        NDS_UI_KIT_DIGIT_PITCH,
                                    NDS_CSS_DS(200));
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
                    NDS_MN_UI_KIT_IMAGE_PUCK_1P,
                NDS_CSS_DS((s32)sCssPuckX[i]),
                NDS_CSS_DS((s32)sCssPuckY[i]), 15u, 0u, 0u);
        }
    }
    ndsMenuShellCssDrawArrows();
}

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
        NDS_CSS_SPRITE_CURSOR_TAG, NDS_MN_UI_KIT_IMAGE_CSS_CURSOR_1P,
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
}

/* --- Portrait flash ------------------------------------------------------- */

#define NDS_CSS_FLASH_TICS 16u
#define NDS_CSS_FLASH_KIND_NONE 0xffu
#define NDS_CSS_FLASH_KIND_MARIO 0u
#define NDS_CSS_FLASH_KIND_FOX 1u
#define NDS_CSS_FLASH_KIND_LUIGI 2u
#define NDS_CSS_FLASH_KIND_LINK 3u
#define NDS_CSS_FLASH_KIND_COUNT 4u

static u32 ndsMenuShellCssFlashKindFromFighter(u32 fkind)
{
    if (fkind == (u32)nFTKindMario)
    {
        return NDS_CSS_FLASH_KIND_MARIO;
    }
    if (fkind == (u32)nFTKindFox)
    {
        return NDS_CSS_FLASH_KIND_FOX;
    }
#if NDS_P2_LUIGI
    if (fkind == (u32)nFTKindLuigi)
    {
        return NDS_CSS_FLASH_KIND_LUIGI;
    }
#endif
#if NDS_P2_LINK
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

    if (kind == NDS_CSS_FLASH_KIND_MARIO)
    {
        if (ready != 0u)
        {
            return (visible != FALSE) ?
                NDS_MN_UI_KIT_SURFACE_CSS_FLASH_MARIO_ON_READY1 :
                NDS_MN_UI_KIT_SURFACE_CSS_FLASH_MARIO_OFF_READY1;
        }
        return (visible != FALSE) ?
            NDS_MN_UI_KIT_SURFACE_CSS_FLASH_MARIO_ON_READY0 :
            NDS_MN_UI_KIT_SURFACE_CSS_FLASH_MARIO_OFF_READY0;
    }
    if (kind == NDS_CSS_FLASH_KIND_FOX)
    {
        if (ready != 0u)
        {
            return (visible != FALSE) ?
                NDS_MN_UI_KIT_SURFACE_CSS_FLASH_FOX_ON_READY1 :
                NDS_MN_UI_KIT_SURFACE_CSS_FLASH_FOX_OFF_READY1;
        }
        return (visible != FALSE) ?
            NDS_MN_UI_KIT_SURFACE_CSS_FLASH_FOX_ON_READY0 :
            NDS_MN_UI_KIT_SURFACE_CSS_FLASH_FOX_OFF_READY0;
    }
    if (kind == NDS_CSS_FLASH_KIND_LUIGI)
    {
        if (ready != 0u)
        {
            return (visible != FALSE) ?
                NDS_MN_UI_KIT_SURFACE_CSS_FLASH_LUIGI_ON_READY1 :
                NDS_MN_UI_KIT_SURFACE_CSS_FLASH_LUIGI_OFF_READY1;
        }
        return (visible != FALSE) ?
            NDS_MN_UI_KIT_SURFACE_CSS_FLASH_LUIGI_ON_READY0 :
            NDS_MN_UI_KIT_SURFACE_CSS_FLASH_LUIGI_OFF_READY0;
    }
    if (ready != 0u)
    {
        return (visible != FALSE) ?
            NDS_MN_UI_KIT_SURFACE_CSS_FLASH_LINK_ON_READY1 :
            NDS_MN_UI_KIT_SURFACE_CSS_FLASH_LINK_OFF_READY1;
    }
    return (visible != FALSE) ?
        NDS_MN_UI_KIT_SURFACE_CSS_FLASH_LINK_ON_READY0 :
        NDS_MN_UI_KIT_SURFACE_CSS_FLASH_LINK_OFF_READY0;
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
    u32 dirty = 0u;
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
            dirty |= (1u << kind);
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
    for (kind = 0u; kind < NDS_CSS_FLASH_KIND_COUNT; kind++)
    {
        if ((dirty & (1u << kind)) != 0u)
        {
            ndsMenuShellCssSyncFlashKind(kind);
        }
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

    if (ndsUiKitBlitSurfaces(&state, 1u) != FALSE)
    {
        gNdsMenuShellCssPanelBlitCount++;
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
        ndsUiKitSetSprite(NDS_CSS_SPRITE_READY_PRESS,
                          NDS_MN_UI_KIT_IMAGE_CSS_READY_PRESS,
                          NDS_CSS_DS(133), NDS_CSS_DS(219));
        ndsUiKitSetSprite(NDS_CSS_SPRITE_READY_START,
                          NDS_MN_UI_KIT_IMAGE_CSS_READY_START,
                          NDS_CSS_DS(162), NDS_CSS_DS(219));
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
        else if (sCssPkind[slot] == (u8)nFTPlayerKindCom)
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
            do
            {
                fkind = (u32)syUtilsRandTimeUCharRange(
                    (s32)nFTKindPlayableEnd + 1);
            }
            while (ndsMenuShellCssFighterLocked(fkind) != FALSE);
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

    for (slot = 0u; slot < (u32)NDS_CSS_SLOTS; slot++)
    {
        s32 panel = (s32)(slot * 69u);
        u32 next;

        if (ndsMenuShellCssBoxHit(panel + 60, panel + 88, 127, 145) == FALSE)
        {
            continue;
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
            (ndsMenuShellCssBoxHit(panel + 34, panel + 58, 131, 141) == FALSE))
        {
            continue;
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

    for (slot = 0u; slot < (u32)NDS_CSS_SLOTS; slot++)
    {
        s32 panel = (s32)(slot * 69u);

        if ((sCssPkind[slot] != (u8)nFTPlayerKindCom) ||
            (sCssSelected[slot] == 0u))
        {
            continue;
        }
        if (ndsMenuShellCssBoxHit(panel + 68, panel + 90, 197, 216) != FALSE)
        {
            if (sCssLevel[slot] < 9u)
            {
                sCssLevel[slot]++;
                ndsMenuShellCssCue(NDS_CSS_FGM_SCROLL2);
                gNdsMenuShellCssLevelChangeCount++;
                ndsMenuShellCssPopulate();
            }
            return TRUE;
        }
        if (ndsMenuShellCssBoxHit(panel + 21, panel + 43, 197, 216) != FALSE)
        {
            if (sCssLevel[slot] > 1u)
            {
                sCssLevel[slot]--;
                ndsMenuShellCssCue(NDS_CSS_FGM_SCROLL2);
                gNdsMenuShellCssLevelChangeCount++;
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
static void ndsMenuShellCssCommit(void)
{
    u32 i;

    for (i = 0u; i < (u32)NDS_MATCH_FIGHTERS_MAX; i++)
    {
        NdsMatchFighterConfig *slot = &gNdsMatchConfig.fighters[i];

        slot->fkind = sCssFkind[i];
        slot->pkind = sCssPkind[i];
        slot->level = sCssLevel[i];
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
    ndsMatchConfigApply(&gNdsMatchConfig);
    gNdsMenuShellCssCommitCount++;
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
    ndsMenuShellGoto((u32)nSCKindVSMode);
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

    for (slot = 0u; slot < (u32)NDS_CSS_SLOTS; slot++)
    {
        s32 panel = (s32)(slot * 69u);
        s32 costume;

        if (ndsMenuShellCssBoxHit(panel + 22, panel + 88, 146, 200) == FALSE)
        {
            continue;
        }
        if ((sCssSelected[slot] == 0u) ||
            (sCssFkind[slot] == (u8)nFTKindNull))
        {
            continue;
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
            gNdsMenuShellCssCostumeCycleCount++;
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
    /* The source has one more branch here -- a POINTER cursor over a placed
     * token becomes a HOVER one -- gated on the CURSOR PLAYER's own
     * `is_selected`, which mnPlayersVSUpdatePuckDisplay clears every frame for
     * a human slot (mnplayersvs.c:2320). It can therefore never run for a human
     * cursor, which is the only kind this hardware has, so it is absent rather
     * than transcribed-and-dead. */
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

    /* mnPlayersVSFuncRun's start delay: once START is accepted the screen stops
     * taking input and counts down before the scene changes. */
    if (sCssStartWait != 0u)
    {
        sCssStartWait--;
        if (sCssStartWait == 0u)
        {
            ndsMenuShellCssCommit();
            /* mnPlayersVSFuncRun:4493 -- P2-1f. The source picks the STAGE
             * SELECT when `gSCManagerTransferBattleState.is_stage_select` is
             * set and randomises the ground itself when it is not; the
             * descriptor's `is_stage_select` IS that field (P2-1a applies it
             * verbatim), so this is the source's own branch on the source's
             * own bit. P2-1e went straight to the battle because
             * `nSCKindMaps` had no screen; it has one now.
             *
             * THE ELSE ARM IS STILL NOT THE SOURCE'S. The source randomises
             * over every unlocked ground there, and eight of the nine are
             * P2-4; until then a no-stage-select match keeps whatever ground
             * the descriptor already carries, which is the same narrowing
             * P2-1e recorded and it is unreachable in every configuration
             * this build ships (the preset sets the bit). */
            ndsMenuShellGoto(
                (gNdsMatchConfig.is_stage_select != FALSE) ?
                    (u32)nSCKindMaps : (u32)nSCKindVSBattle);
        }
        return;
    }

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
        if (ndsMenuShellCssCheckKindButton() == FALSE)
        {
            if (ndsMenuShellCssSelectFighter() == FALSE)
            {
                if (ndsMenuShellCssCheckGrab() == FALSE)
                {
                    /* mnPlayersVSCursorProcUpdate checks game mode, BACK, then
                     * the team selectors in exactly this order (:3356-3367). */
                    if (ndsMenuShellCssBoxHit(27, 137, 14, 35) != FALSE)
                    {
                        NdsUiKitSurfaceId label;

                        sCssIsTeamBattle = (sCssIsTeamBattle != FALSE) ?
                            0u : 1u;
                        label = (sCssIsTeamBattle != FALSE) ?
                            NDS_MN_UI_KIT_SURFACE_CSS_MODE_TEAM :
                            NDS_MN_UI_KIT_SURFACE_CSS_MODE_FFA;
                        if (ndsUiKitBlitSurfaces(&label, 1u) != FALSE)
                        {
                            gNdsMenuShellCssPanelBlitCount++;
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
                    if (ndsMenuShellCssBoxHit(244, 292, 13, 34) != FALSE)
                    {
                        ndsMenuShellCssCue(NDS_CSS_FGM_SCROLL2);
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

    /* 4. B recalls your own placed token (mnplayersvs.c:3411). */
    if ((taps & NDS_INPUT_B) != 0u)
    {
        ndsMenuShellCssRecall();
    }

    /* 5. mnPlayersVSDetectBack: B HELD for forty tics leaves the screen. */
    if ((held & NDS_INPUT_B) != 0u)
    {
        sCssBackTics++;
        if (sCssBackTics >= (u32)NDS_CSS_BACK_HOLD_TICS)
        {
            ndsMenuShellCssBack();
            return;
        }
    }
    else
    {
        sCssBackTics = 0u;
    }

    /* 6. The cursor's own state, last, exactly as the source updates it after
     * every action rather than before them. */
    ndsMenuShellCssUpdateStatus();

    /* 7. READY TO FIGHT, and the START it invites. */
    {
        u32 ready = ndsMenuShellCssCheckReady();
        u32 lit;

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

        if (((taps & NDS_INPUT_START) != 0u) &&
            (sMenuTics > (u32)NDS_CSS_START_ARM_TICS))
        {
            if (ready != FALSE)
            {
                ndsMenuShellCssCue(NDS_CSS_VOICE_CHEER);
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

/* mnPlayersVSInitVars + mnPlayersVSInitPlayer + mnPlayersVSInitSlot,
 * mnplayersvs.c:4670/:4579/:4698, reading the P2-1a descriptor instead of
 * gSCManagerTransferBattleState: the descriptor is what the battle consumes, so
 * it is also what the screen that fills it should open from. */
static void ndsMenuShellCssInit(void)
{
    u32 i;

    sCssCursorX = NDS_CSS_CURSOR_HOME_X;
    sCssCursorY = NDS_CSS_CURSOR_HOME_Y;
    sCssStatus = NDS_CSS_STATUS_POINTER;
    sCssHeld = -1;
    sCssRegrabTic = 0u;
    sCssBackTics = 0u;
    sCssStartWait = 0u;
    sCssReadyBlink = 0u;
    sCssReadyShown = 0xffffffffu; /* forces the first ShowReady to publish */
    sCssArrowsShown = TRUE;
    sCssFlashShown[NDS_CSS_FLASH_KIND_MARIO] = 0u;
    sCssFlashShown[NDS_CSS_FLASH_KIND_FOX] = 0u;
    sCssFlashShown[NDS_CSS_FLASH_KIND_LUIGI] = 0u;
    sCssFlashShown[NDS_CSS_FLASH_KIND_LINK] = 0u;
    /* P2-1N (4): seeded from the transfer state exactly as the source seeds
     * sMNPlayersVSIsTeamBattle on scene entry (mnplayersvs.c:4679). */
    sCssIsTeamBattle = (gSCManagerTransferBattleState.is_team_battle != 0) ?
        1u : 0u;

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
        sCssLevel[i] = (cfg->level < 1u) ? 1u : ((cfg->level > 9u) ? 9u :
                                                 cfg->level);
        sCssTeam[i] = (cfg->team <= (u8)nSCBattleTeamIDGreen) ? cfg->team :
            (u8)((i < 2u) ? nSCBattleTeamIDRed : nSCBattleTeamIDBlue);
        /* mnPlayersVSUpdateGate, mnplayersvs.c:4193: a slot with no controller
         * cannot be HUMAN. Only slot 0 has one here, so any other slot the
         * descriptor calls human arrives as empty -- which is the same
         * correction the source makes, one frame later, in its gate pass. */
        if ((sCssPkind[i] == (u8)nFTPlayerKindMan) &&
            (i != (u32)NDS_CSS_CURSOR_SLOT))
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
    /* P2-1N (3): mnPlayersVSMakeGate seeds EVERY slot at door_offset=41
     * (closed), then mnPlayersVSShutterProcUpdate opens each occupied slot by
     * 2/tic. Do not shortcut occupied slots to zero on entry: that deletes the
     * source's entrance shutter animation. The CSS_DOORS strip is cached here
     * for those slides. */
    for (i = 0u; i < (u32)NDS_CSS_SLOTS; i++)
    {
        sCssDoorOffset[i] = 41u;
    }
    (void)ndsUiKitCacheSurface((u32)NDS_MN_UI_KIT_SURFACE_CSS_DOORS);
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
        ndsMNPlayersVSPreviewSync(slot, (s32)sCssPkind[slot],
                                  (s32)sCssFkind[slot],
                                  (sCssSelected[slot] != 0u) ? TRUE : FALSE);
    }
}

/* Screen ENTRY, on a load frame: the only place all four panels are written at
 * once. Every later change goes through ndsMenuShellCssMove's one-a-frame
 * sync. */
static void ndsMenuShellPopulateCssScreen(void)
{
    ndsMenuShellCssSyncPanels((u32)NDS_CSS_SLOTS);
    ndsMenuShellCssPopulate();
    ndsMenuShellCssSyncTeamSelectAll();
    ndsMenuShellCssShowReady(FALSE);
    ndsMenuShellCssMove();
}
