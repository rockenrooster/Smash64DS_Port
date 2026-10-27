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
 * WHAT IS LOCKED. Two bounds, in this order. First, which fighters EXIST:
 * ten of the twelve are P2-3, so the mask is Mario|Fox and the other ten
 * portraits draw the source's own question-mark plate -- the thing it draws
 * for a locked fighter (mnplayersvs.c:374). Second, which fighters the SAVE
 * has unlocked: the source gates a portrait on `gSCManagerBackupData
 * .fighter_mask` (mnplayersvs.c:296), snapshotted at init (mnplayersvs.c:4694),
 * and only the four newcomers can be locked by it (mnplayersvs.c:300-313;
 * starters take the default-FALSE arm). ndsMenuShellCssFighterLocked applies
 * both: unadmitted fighters are locked whatever the save says, and admitted
 * newcomers were locked until mnMessageApplyUnlock earned them
 * (mnmessage.c:284-301). Since 2026-10-01 (owner: every fighter works) the
 * save no longer locks any VS cell (ndsMenuShellCssSaveLocked); the plate
 * draws every admitted portrait unlocked, and ndsMenuShellCssSyncLockedCells,
 * which covered a save-locked newcomer's cell with its CSS_LOCKED_* surface,
 * now finds none to cover.
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
 *   - THE TIME/STOCK ARROWS are no longer a narrowing (owner, 2026-10-12):
 *     the rule bar at the header's centre steps the time or the stocks as the
 *     source's does (ndsMenuShellCssRuleArrow); a wireless guest shows the
 *     host's.
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

/* P4 (docs/P4/P4_STATUS.md): a build with Remix contents runs this screen on
 * Smash Remix's 30-cell grid (CharacterSelect.asm; scripts/p4/p4_css.py writes
 * the tables, generate_mn_ui_kit.py bakes the art). A slot's sCssFkind is then
 * a SELECTION: an original's fkind, or NDS_P4_SEL_BASE + content for a Remix
 * selection, committed as its setup parent's kind plus the content. The
 * kind-indexed tables below stay the twelve originals'. */
#include "nds_p4_css.generated.inc"
#define NDS_CSS_CELLS NDS_P4_CSS_CELLS
/* The token's centre at PORTRAIT_SCALE (adjust_created_token_), and its box. */
#define NDS_CSS_P4_PUCK_CX 11
#define NDS_CSS_P4_PUCK_CY 10
#define NDS_CSS_P4_PUCK_W 21
#define NDS_CSS_P4_PUCK_H 20
/* The P4 screen (p4_css.py SCREEN, owner 2026-10-11): the mode label and
 * BACK sit HEADER_DY source units higher; the player cards scale by
 * CARD_KN/CARD_KD at CARD_PITCH from CARD_X, ending at the frame's bottom
 * edge, so a source position on panel `panel` (p * 69) maps through
 * NDS_CSS_CARD_X/Y; PRESS START sits in the band between grid and cards. */
#define NDS_CSS_HEADER_DY NDS_P4_CSS_HEADER_DY
#define NDS_CSS_CARD_X(panel, sx)                                   \
    (NDS_P4_CSS_CARD_X + ((panel) / 69) * NDS_P4_CSS_CARD_PITCH +   \
     ((((s32)(sx)) - 22) * NDS_P4_CSS_CARD_KN) / NDS_P4_CSS_CARD_KD)
#define NDS_CSS_CARD_Y(sy)                                          \
    (NDS_P4_CSS_CARD_Y +                                            \
     ((((s32)(sy)) - 126) * NDS_P4_CSS_CARD_KN) / NDS_P4_CSS_CARD_KD)
#define NDS_CSS_CARD_LEN(v) (((v) * NDS_P4_CSS_CARD_KN) / NDS_P4_CSS_CARD_KD)
#define NDS_CSS_PRESS_Y NDS_P4_CSS_PRESS_Y

/* A selection's battle identity: its kind (a Remix selection's setup parent)
 * and its P4 content (0 for the original cast). */
static u32 ndsMenuShellCssSelFkind(u32 sel)
{
    const NDSP4Fighter *p4 = ndsP4Fighter(ndsP4SelContent(sel));

    if (p4 != NULL)
    {
        return (u32)p4->parent_kind;
    }
    return sel;
}

static u32 ndsMenuShellCssSelContent(u32 sel)
{
    return ndsP4SelContent(sel);
}

/* mnPlayersVSAdjustCursor's clamps and its full-deflection step. The P4
 * grid starts at x 0 (eleven 29-unit columns), so a token on its first column
 * (x 3-24) lay left of the hand's grab point at the source's minimum (cursor
 * x + 25): MRGAW's CP badge could not be picked up (owner BUGS.md
 * 2026-10-12). The hand may run 20 units past the left edge there. */
#define NDS_CSS_CURSOR_X_MIN (-20)
#define NDS_CSS_CURSOR_X_MAX 280
#define NDS_CSS_CURSOR_Y_MIN (10 + NDS_CSS_HEADER_DY)
#define NDS_CSS_CURSOR_Y_MAX NDS_CSS_CARD_Y(205)
#define NDS_CSS_CURSOR_STEP 4

/* nMNPlayersCursorStatus*, in the order mnPlayersVSUpdateCursor's own
 * cursor_offsets[] indexes them (mnplayersvs.c:1723). */
#define NDS_CSS_STATUS_POINTER 0u
#define NDS_CSS_STATUS_GRAB 1u
#define NDS_CSS_STATUS_HOVER 2u

/* mnPlayersVSMakeCursor's player-0 seat, mnplayersvs.c:3684. */
#define NDS_CSS_CURSOR_HOME_X 40
#define NDS_CSS_CURSOR_HOME_Y NDS_CSS_CARD_Y(170)
/* mnPlayersVSMakePuck's parking spot for a slot with no fighter yet. */
#define NDS_CSS_PUCK_HOME_X NDS_CSS_CARD_X(0, 51)
#define NDS_CSS_PUCK_HOME_Y NDS_CSS_CARD_Y(161)
#define NDS_CSS_PUCK_HOME_SLOT_X(i) NDS_CSS_CARD_X(69 * (s32)(i), 51)
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
/* Kirby lands behind his own flag; every earlier opt-in fighter rides its own
 * flag in this arm (admit_fighter.py). */
#define NDS_CSS_FIGHTER_MASK \
    (LBBACKUP_MASK_FIGHTER(nFTKindMario) | LBBACKUP_MASK_FIGHTER(nFTKindFox) | \
     LBBACKUP_MASK_FIGHTER(nFTKindLuigi) | \
     LBBACKUP_MASK_FIGHTER(nFTKindDonkey) | \
     LBBACKUP_MASK_FIGHTER(nFTKindCaptain) | \
     LBBACKUP_MASK_FIGHTER(nFTKindSamus) | \
     (NDS_LINK ? LBBACKUP_MASK_FIGHTER(nFTKindLink) : 0u) | \
     (NDS_PIKACHU ? LBBACKUP_MASK_FIGHTER(nFTKindPikachu) : 0u) | \
     (NDS_YOSHI ? LBBACKUP_MASK_FIGHTER(nFTKindYoshi) : 0u) | \
     (NDS_NESS ? LBBACKUP_MASK_FIGHTER(nFTKindNess) : 0u) | \
     (NDS_PURIN ? LBBACKUP_MASK_FIGHTER(nFTKindPurin) : 0u) | \
     LBBACKUP_MASK_FIGHTER(nFTKindKirby))

static u8 sCssPkind[NDS_CSS_SLOTS];
static u8 sCssFkind[NDS_CSS_SLOTS];
static u8 sCssLevel[NDS_CSS_SLOTS];
static u8 sCssHandicap[NDS_CSS_SLOTS];
static u8 sCssTeam[NDS_CSS_SLOTS];
static u8 sCssSelected[NDS_CSS_SLOTS]; /* is_fighter_selected */
/* The costume the last visit committed for the fighter a slot keeps, -1 for
 * none: set where the screen's init reads the fighter, worn from entry. */
static s16 sCssEntryCostume[NDS_CSS_SLOTS];
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
/* P7 C4 (owner, 2026-10-18: "Training mode enabled"). This screen is also
 * the training select (mnplayers1ptraining.c): the man in the cursor's slot
 * and the COM in the other of 0/1 (mnPlayers1PTrainingInitVars), the other
 * two shut; no mode toggle, rule bar, kind buttons or CPU level (the training
 * menu owns the COM). START always opens the stage select, B goes back to the
 * 1P menu, and the commit writes the scene's training_man/com pair
 * (mnPlayers1PTrainingSetSceneData) and each one's content. */
static u8 sCssTraining;
/* P7 C1 (owner, 2026-10-18: "1p all chars allowed in campaign"). With Remix
 * contents compiled this screen is also the 1P game's select
 * (mnplayers1pgame.c): one human card in that select's own lower half over
 * Remix's grid (generate_mn_ui_kit.py ONEP_P4_*), slots 1-3 shut. Its own
 * furniture is transcribed from that file: the time selector's two values
 * (:3339), LEVEL and STOCK with their arrows (:2476/:2556), BACK on A or a B
 * tap (:2647), START once the fighter is placed (:3250); the commit is
 * mnPlayers1PGameSetSceneData's (:3262) with the content beside the kind
 * (gNdsP4OnePlayerContent). sCssOnePShown is 1 + the time/level key the
 * option surfaces last showed. */
static u8 sCssOneP;
static u8 sCssOnePLevel;
static u8 sCssOnePStock;
static u8 sCssOnePTime;
static u32 sCssOnePShown;
/* P7 C2 (owner, 2026-10-18: "Bonus stages for all chars"). sCssOneP 2: the
 * Bonus 1/2 Practice selects (mnplayers1pbonus.c), the same one card 33 px
 * right (MakeGate :755) with no time, level or stock: the bonus kind's title,
 * toggled by an A on it (UpdateGameMode :2081, sCssBonusKind 0 Break the
 * Targets, 1 Board the Platforms), the puck fighter's record and the
 * all-fighter total under the card (MakeHiScore :1171, MakeTotalTime :1187);
 * START, or 140 tics after a pick (SelectFighterPuck :288, FuncRun :2714),
 * starts the kind's board. sCssBonusShown/RecordShown/TotalShown are 1 + the
 * key each row last showed; sCssBonusPicked the last frame's pick. */
#define NDS_CSS_ONEP_GAME 1u
#define NDS_CSS_ONEP_BONUS 2u
static u8 sCssBonusKind;
static u8 sCssBonusWait;
static u8 sCssBonusPicked;
static u32 sCssBonusShown;
static u32 sCssBonusRecordShown;
static u32 sCssBonusTotalShown;
static u32 sCssBonusTotal[3];
static u32 sCssBonusTotalKey;
#define NDS_CSS_ONEP (sCssOneP != 0u)
#define NDS_CSS_BONUS (sCssOneP == NDS_CSS_ONEP_BONUS)
/* 1 + the mode label surface ndsMenuShellCssPopulate last blitted with BACK
 * over this entry's base surfaces; 0 until it has. */
static u32 sCssEntryStatesShown;
/* THE RULE BAR (owner, 2026-10-12): the source's sMNPlayersVSGameRule,
 * TimeValue and StockValue, seeded at entry from the transfer state
 * (mnplayersvs.c:4676-4681), stepped by the selector's arrows (:3318-3353)
 * and committed with the rest (mnPlayersVSSetSceneData :4383). Wireless:
 * only the host steps them; a guest shows the host's. sCssRuleShown is 1 +
 * the state ndsMenuShellCssShowRule last blitted, 0 until it has. */
static u8 sCssRuleIsStock;
static u8 sCssRuleTime;
static u8 sCssRuleStock;
static u32 sCssRuleShown;
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
static u8 sCssFlashShown[NDS_CSS_CELLS];

/* One cursor: the DS has one keypad, so exactly one player has a controller.
 * mnPlayersVSUpdateControllerOrders would report orders[0] = 0 and -1 for the
 * rest, and every branch below that reads a controller order is written from
 * that fact rather than from a general N-cursor model. */
/* The cursor's own slot: P1 offline; in a wireless room, this console's port
 * (P3: "a guest who owns P3 is P3 on every console"). */
static u32 sCssCursorSlot;
#define NDS_CSS_CURSOR_SLOT ((s32)sCssCursorSlot)
/* P3: nonzero while this screen is a wireless lobby. */
static u32 sCssNet;
/* P3: the other humans' cursors as their owners last published them (state
 * + 1, 0 for none); whether the 2P-4P art is loaded; this guest's costume
 * presses (a count the host turns into costume changes) and, on the host,
 * the count last seen from each guest. */
static u8 sCssNetCursor[NDS_CSS_SLOTS];
static s16 sCssNetCursorX[NDS_CSS_SLOTS];
static s16 sCssNetCursorY[NDS_CSS_SLOTS];
static u8 sCssNetCostumeSeen[NDS_CSS_SLOTS];
static u8 sCssNetCostumeReq;
static u32 sCssNetArt;

/* --- Audio seam ---------------------------------------------------------- */

static u32 ndsMenuShellCssNetGuest(void);
static u32 ndsMenuShellCssNetHumanOther(u32 slot);
static u32 ndsMenuShellCssNetAbsent(u32 slot);

static void ndsMenuShellCssCue(u32 id)
{
    gNdsMenuShellCssCueCount++;
    gNdsMenuShellCssCueLastId = id;
    /* Unguarded, like the kit's own seam: NDS_IMPORT_BATTLESHIP_AUDIO_FGM is an
     * unconditional Makefile override, so this symbol always exists. */
    (void)ndsAudioFgmPlay((u16)id);
}

/* START's crowd cheer. Owner BUGS.md CSS row (2026-10-19): it was silent when
 * START followed a pick closely -- the name the pick called still held the
 * cue arena the previews leave the select, and the cheer's body had no room
 * -- so it waits for room instead (ndsAudioFgmPlayWhenRoom). */
static void ndsMenuShellCssCheer(void)
{
    gNdsMenuShellCssCueCount++;
    gNdsMenuShellCssCueLastId = NDS_CSS_VOICE_CHEER;
    (void)ndsAudioFgmPlayWhenRoom((u16)NDS_CSS_VOICE_CHEER, 64u);
}

/* P2-1N (4): the mode announcer voice, shared by the source's entry tail and
 * mode-toggle handler — FREE FOR ALL (512) or TEAM BATTLE (526). */
static void ndsMenuShellCssAnnounceMode(void)
{
    if (sCssOneP != 0u)
    {
        /* mnPlayers1PGameFuncStart's tail (:3478). A bonus select announces
         * its kind in mnPlayers1PBonusMakeLabels (:901-903, from FuncStart
         * :2858 and from the kind toggle, UpdateGameMode :2090) -- missed
         * here until the owner's BUGS.md row (2026-10-19). Its 33-36 KiB
         * voices wait for room (ndsAudioFgmPlayWhenRoom) like the cheer. */
        if (NDS_CSS_BONUS == 0)
        {
            ndsMenuShellCssCue((u32)nSYAudioVoiceAnnounceSelectPlayer);
        }
        else
        {
            const u32 id = (sCssBonusKind == 0u) ?
                (u32)nSYAudioVoiceAnnounceBreakTheTargets :
                (u32)nSYAudioVoiceAnnounceBoardThePlatforms;

            gNdsMenuShellCssCueCount++;
            gNdsMenuShellCssCueLastId = id;
            (void)ndsAudioFgmPlayWhenRoom((u16)id, 64u);
        }
        return;
    }
    if (sCssTraining != 0u)
    {
        /* nSYAudioVoiceAnnounceTrainingMode, mnplayers1ptraining.c:3209. */
        ndsMenuShellCssCue(530u);
        return;
    }
    ndsMenuShellCssCue((sCssIsTeamBattle != FALSE) ? 526u : 512u);
}

/* mnPlayersVSAnnounceFighter, mnplayersvs.c:2545: the whoosh then the name. */
static void ndsMenuShellCssAnnounce(u32 slot)
{
    u32 fkind = (u32)sCssFkind[slot];

    if (ndsMenuShellCssSelContent(fkind) != 0u)
    {
        /* add_to_css's own announcer FGM for a Remix selection. */
        ndsMenuShellCssCue(NDS_CSS_FGM_ANNOUNCE_WHOOSH);
        ndsMenuShellCssCue(
            (u32)kNdsP4CssAnnounceFgm[ndsMenuShellCssSelContent(fkind)]);
        gNdsMenuShellCssAnnounceCount++;
        return;
    }
#ifdef NDS_P4_CSS_ANNOUNCE_KIND_13
    if (fkind == (u32)nFTKindMMario)
    {
        ndsMenuShellCssCue(NDS_CSS_FGM_ANNOUNCE_WHOOSH);
        ndsMenuShellCssCue(NDS_P4_CSS_ANNOUNCE_KIND_13);
        gNdsMenuShellCssAnnounceCount++;
        return;
    }
#endif
    if (fkind >= NDS_CSS_PORTRAITS)
    {
        return;
    }
    ndsMenuShellCssCue(NDS_CSS_FGM_ANNOUNCE_WHOOSH);
    ndsMenuShellCssCue((u32)kNdsCssAnnounceVoice[fkind]);
    gNdsMenuShellCssAnnounceCount++;
}

/* --- Source geometry ----------------------------------------------------- */

/* Runtime harness id for the dev-open gate below comes from the parent
 * nds_menu_shell.c include set; this fragment must not inject headers mid-TU. */

/* mnPlayersVSCheckFighterLocked, mnplayersvs.c:296-314, consults the save
 * mask for the four newcomers (Luigi, Ness, Captain, Purin -- the set
 * mnMessageApplyUnlock writes, mnmessage.c:287), and a fresh cart starts with
 * all four locked (scmanager.c:313-314): the ? plate over a dark shadow.
 * Owner 2026-10-01 opened all twelve; P7 (docs/P7/P7_Implementation_Plan.md
 * U7, assumption A3, approved 2026-10-13) restores the source's lock and adds
 * its own for Metal Mario and the contents (nds_p7_progress.c), with SELECT on
 * VS Options opening everything (U8). Dev-open builds (harness, lab, walk)
 * still see every fighter (ndsP7DevOpen). */
/* The save's fighter bits for the source's four newcomers: this console's,
 * or in a wireless room the host's (its unlock byte carries the same four:
 * mnMessageApplyUnlock writes both, mnmessage.c:287-293; plan 7.1, P7 A1). */
static u32 ndsMenuShellCssFighterMask(void)
{
    if (ndsNetInSession() != 0u)
    {
        u32 unlock = ndsNetLobbyUnlockMask();
        u32 mask = 0u;

        mask |= ((unlock & LBBACKUP_UNLOCK_MASK_LUIGI) != 0u) ? (1u << nFTKindLuigi) : 0u;
        mask |= ((unlock & LBBACKUP_UNLOCK_MASK_NESS) != 0u) ? (1u << nFTKindNess) : 0u;
        mask |= ((unlock & LBBACKUP_UNLOCK_MASK_CAPTAIN) != 0u) ? (1u << nFTKindCaptain) : 0u;
        mask |= ((unlock & LBBACKUP_UNLOCK_MASK_PURIN) != 0u) ? (1u << nFTKindPurin) : 0u;
        return mask;
    }
    return (u32)gSCManagerBackupData.fighter_mask;
}

/* A P7 fighter bit: this console's, or in a room the host's (A1). */
static u32 ndsMenuShellCssP7FighterOpen(u32 sel)
{
    if (ndsNetInSession() != 0u)
    {
        return ndsNetLobbyP7FighterOpen(sel);
    }
    return ndsP7FighterUnlocked(sel);
}

static u32 ndsMenuShellCssSaveLocked(u32 fkind)
{
    if (ndsP7DevOpen() != FALSE)
    {
        return FALSE;
    }
    switch (fkind)
    {
    case (u32)nFTKindNess:
    case (u32)nFTKindPurin:
    case (u32)nFTKindCaptain:
    case (u32)nFTKindLuigi:
        return ((ndsMenuShellCssFighterMask() & (1u << fkind)) != 0u) ? FALSE : TRUE;
    default:
        return FALSE;
    }
}

/* P7: the save's lock on a selection this build compiles -- the source's four
 * newcomers by its fighter mask, Metal Mario and the contents by the P7 block
 * (by selection id; in a wireless room the host's, A1). */
static u32 ndsMenuShellCssProgressLocked(u32 sel)
{
    if (ndsMenuShellCssSelContent(sel) != 0u)
    {
        return (ndsMenuShellCssP7FighterOpen(NDS_P7_SEL_CONTENT(ndsMenuShellCssSelContent(sel))) != FALSE) ?
            FALSE : TRUE;
    }
    if (sel == (u32)nFTKindMMario)
    {
        return (ndsMenuShellCssP7FighterOpen(NDS_P7_SEL_METAL) != FALSE) ? FALSE : TRUE;
    }
    return ndsMenuShellCssSaveLocked(sel);
}

static u32 ndsMenuShellCssFighterLocked(u32 fkind)
{
    /* A Remix selection exists when this build compiles its content, and is
     * open when the save has unlocked it (P7). */
    if (ndsMenuShellCssSelContent(fkind) != 0u)
    {
        return ((ndsP4Fighter(ndsMenuShellCssSelContent(fkind)) == NULL) ||
                (ndsMenuShellCssProgressLocked(fkind) != FALSE)) ? TRUE : FALSE;
    }
#ifdef NDS_P4_CSS_GATE_KIND_13
    /* Metal Mario: the source kind P4 makes selectable (p4_css.py KINDS); no
     * save bit unlocks him in the source, the P7 block does. */
    if (fkind == (u32)nFTKindMMario)
    {
        return ndsMenuShellCssProgressLocked(fkind);
    }
#endif
    if (fkind >= NDS_CSS_PORTRAITS)
    {
        return TRUE;
    }
    /* Existence first: a fighter with no renderer/audio seam is locked
     * whatever the save says. Then the save, which is what draws the
     * question-mark plate ndsMenuShellCssSyncLockedCells covers the cell
     * with (mnplayersvs.c:429-446) and what refuses the drop
     * (mnplayersvs.c:3017) and rerolls the random pick (mnplayersvs.c:3483). */
    if ((NDS_CSS_FIGHTER_MASK & (1u << fkind)) == 0u)
    {
        return TRUE;
    }
    return ndsMenuShellCssSaveLocked(fkind);
}

/* Poked by a diagnostic before the walk reaches this screen, and refused
 * unless this build actually HAS that fighter -- the same test the drop path
 * uses -- so a stale poke cannot commit a portrait the ROM cannot render and
 * then be misread as a native-render finding. Defined here, above every user. */
__attribute__((used)) volatile u32 gNdsMenuShellCssWalkTargetKind =
    NDS_CSS_WALK_TARGET_AUTO;
__attribute__((used)) volatile u32 gNdsMenuShellCssWalkTargetKind2 =
    NDS_CSS_WALK_TARGET_AUTO;

u32 ndsMenuShellCssWalkTargetKind(u32 slot)
{
    u32 want = (slot == (u32)1) ? gNdsMenuShellCssWalkTargetKind2 :
                                  gNdsMenuShellCssWalkTargetKind;

    if ((want != NDS_CSS_WALK_TARGET_AUTO) &&
        (ndsMenuShellCssFighterLocked(want) == FALSE))
    {
        return want;
    }
    return (slot == (u32)1) ? (u32)nFTKindFox : (u32)nFTKindMario;
}

/* The P4 grid cell holding a selection, or -1. */
static s32 ndsMenuShellCssP4Cell(u32 sel)
{
    u32 cell;

    if (sel == (u32)nFTKindNull)
    {
        return -1;
    }
    for (cell = 0u; cell < (u32)NDS_P4_CSS_CELLS; cell++)
    {
        if ((u32)kNdsP4CssCellSel[cell] == sel)
        {
            return (s32)cell;
        }
    }
    return -1;
}

/* Remix's random draws among every available selection: one time-RNG draw
 * over the unlocked cells, in cell order (the random chooser is no fighter:
 * ndsMenuShellCssFighterLocked holds it locked, so it is never drawn). */
static u32 ndsMenuShellCssP4RandomSel(void)
{
    u32 available = 0u;
    u32 pick;
    u32 cell;

    for (cell = 0u; cell < (u32)NDS_P4_CSS_CELLS; cell++)
    {
        if (ndsMenuShellCssFighterLocked((u32)kNdsP4CssCellSel[cell]) == FALSE)
        {
            available++;
        }
    }
    pick = (u32)syUtilsRandTimeUCharRange((s32)available);
    for (cell = 0u; cell < (u32)NDS_P4_CSS_CELLS; cell++)
    {
        if (ndsMenuShellCssFighterLocked((u32)kNdsP4CssCellSel[cell]) != FALSE)
        {
            continue;
        }
        if (pick-- == 0u)
        {
            return (u32)kNdsP4CssCellSel[cell];
        }
    }
    return (u32)nFTKindMario;
}

/* mnPlayersVSCenterPuckInPortrait, mnplayersvs.c:3454. */
static void ndsMenuShellCssCenterPuck(u32 slot, u32 fkind)
{
    u32 portrait;

    {
        /* token_autoposition_: the token's centre on its cell's centre. */
        s32 cell = ndsMenuShellCssP4Cell(fkind);

        if (cell < 0)
        {
            sCssPuckX[slot] = (s16)NDS_CSS_PUCK_HOME_X;
            sCssPuckY[slot] = (s16)NDS_CSS_PUCK_HOME_Y;
            return;
        }
        sCssPuckX[slot] = (s16)(NDS_P4_CSS_X +
                                ((cell % NDS_P4_CSS_COLUMNS) *
                                 NDS_P4_CSS_PITCH) +
                                (NDS_P4_CSS_PITCH / 2) - NDS_CSS_P4_PUCK_CX);
        sCssPuckY[slot] = (s16)(NDS_P4_CSS_Y +
                                ((cell / NDS_P4_CSS_COLUMNS) *
                                 NDS_P4_CSS_PITCH) +
                                (NDS_P4_CSS_PITCH / 2) - NDS_CSS_P4_PUCK_CY);
        return;
    }

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

/* The P4 grid cell under the (scaled) token's centre, or -1 off the grid
 * (Remix's get_character_id_). */
static s32 ndsMenuShellCssPuckP4Cell(u32 slot)
{
    s32 cx = (s32)sCssPuckX[slot] + NDS_CSS_P4_PUCK_CX - NDS_P4_CSS_X;
    s32 cy = (s32)sCssPuckY[slot] + NDS_CSS_P4_PUCK_CY - NDS_P4_CSS_Y;

    if ((cx < 0) || (cy < 0) ||
        (cx >= (NDS_P4_CSS_COLUMNS * NDS_P4_CSS_PITCH)) ||
        (cy >= (NDS_P4_CSS_ROWS * NDS_P4_CSS_PITCH)))
    {
        return -1;
    }
    return ((cy / NDS_P4_CSS_PITCH) * NDS_P4_CSS_COLUMNS) + (cx / NDS_P4_CSS_PITCH);
}

/* mnPlayersVSGetPuckFighterKind, mnplayersvs.c:3001 -- which fighter the
 * token's CENTRE is over, or nFTKindNull. */
static u32 ndsMenuShellCssPuckFighterKind(u32 slot)
{
    s32 x = (s32)sCssPuckX[slot] + 13;
    s32 y = (s32)sCssPuckY[slot] + 12;
    u32 fkind;

    {
        /* get_character_id_: the cell under the (scaled) token's centre. Over
         * the random chooser it is no fighter until the token drops. */
        s32 cell = ndsMenuShellCssPuckP4Cell(slot);

        if (cell < 0)
        {
            return (u32)nFTKindNull;
        }
        fkind = (u32)kNdsP4CssCellSel[cell];
        return (ndsMenuShellCssFighterLocked(fkind) != FALSE) ?
            (u32)nFTKindNull : fkind;
    }

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
    const s32 w = NDS_CSS_P4_PUCK_W;
    const s32 h = NDS_CSS_P4_PUCK_H;

    if ((x < (s32)sCssPuckX[slot]) || (x > ((s32)sCssPuckX[slot] + w)))
    {
        return FALSE;
    }
    if ((y < (s32)sCssPuckY[slot]) || (y > ((s32)sCssPuckY[slot] + h)))
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

/* --- The rule bar -------------------------------------------------------- */

_Static_assert(NDS_MN_UI_KIT_SURFACE_CSS_RULE_DIGIT_9 ==
                   NDS_MN_UI_KIT_SURFACE_CSS_RULE_DIGIT_0 + 9u,
               "rule digit glyphs must stay contiguous in digit order");

/* llMNPlayersCommon<N>DarkSprite's widths: MakeGameRuleNumber steps left by
 * each digit's own (mnplayersvs.c:1196-1216). */
static const u8 kCssRuleDigitWidth[10] = { 10, 7, 11, 10, 10, 11, 9, 10, 10, 9 };

/* mnPlayersVSGetNextTimeValue / GetPrevTimeValue's table (:4539-4576). */
static const u8 kCssRuleTimes[8] = {
    2, 3, 5, 10, 15, 30, 60, SCBATTLE_TIMELIMIT_INFINITE
};

static u8 ndsMenuShellCssRuleStepTime(u8 value, s32 step)
{
    s32 i;

    if (step > 0)
    {
        if (value == kCssRuleTimes[7])
        {
            return kCssRuleTimes[0];
        }
        for (i = 0; i < 8; i++)
        {
            if (value < kCssRuleTimes[i])
            {
                return kCssRuleTimes[i];
            }
        }
        return kCssRuleTimes[7];
    }
    if (value == kCssRuleTimes[0])
    {
        return kCssRuleTimes[7];
    }
    for (i = 7; i >= 0; i--)
    {
        if (value > kCssRuleTimes[i])
        {
            return kCssRuleTimes[i];
        }
    }
    return kCssRuleTimes[7];
}

/* mnPlayersVSMakeTimeSelect / MakeStockSelect (:1252/:1305): the selector,
 * then MakeTimeSetting's Infinity at (194,24) or the value right-aligned at
 * x 208 (one digit) or 212, MakeStockSetting's stocks + 1 at 210 or 214, both
 * at y 23 -- one open for the whole bar, only when the state changed. */
static void ndsMenuShellCssShowRule(void)
{
    NdsUiKitSurfaceId surfaces[3];
    s16 sites[6];
    u32 count = 1u;
    u32 key = (sCssRuleIsStock != 0u) ? (0x100u | sCssRuleStock) :
                                        (u32)sCssRuleTime;

    if (sCssRuleShown == key + 1u)
    {
        return;
    }
    surfaces[0] = (sCssRuleIsStock != 0u) ? NDS_MN_UI_KIT_SURFACE_CSS_RULE_STOCK :
                                            NDS_MN_UI_KIT_SURFACE_CSS_RULE_TIME;
    sites[0] = NDS_UI_KIT_SITE_BAKED;
    sites[1] = 0;
    if ((sCssRuleIsStock == 0u) &&
        (sCssRuleTime == SCBATTLE_TIMELIMIT_INFINITE))
    {
        surfaces[1] = NDS_MN_UI_KIT_SURFACE_CSS_RULE_INFINITY;
        sites[2] = (s16)NDS_CSS_DS(194);
        sites[3] = (s16)NDS_CSS_DS(24 + NDS_CSS_HEADER_DY);
        count = 2u;
    }
    else
    {
        u32 number = (sCssRuleIsStock != 0u) ? ((u32)sCssRuleStock + 1u) :
                                               (u32)sCssRuleTime;
        s32 x = (sCssRuleIsStock != 0u) ? ((number < 10u) ? 210 : 214) :
                                          ((number < 10u) ? 208 : 212);
        u32 places = (number >= 10u) ? 2u : 1u;

        while (places != 0u)
        {
            u32 digit = number % 10u;

            x -= (s32)kCssRuleDigitWidth[digit];
            surfaces[count] = (NdsUiKitSurfaceId)
                (NDS_MN_UI_KIT_SURFACE_CSS_RULE_DIGIT_0 + digit);
            sites[2u * count] = (s16)NDS_CSS_DS(x);
            sites[(2u * count) + 1u] = (s16)NDS_CSS_DS(23 + NDS_CSS_HEADER_DY);
            count++;
            number /= 10u;
            places--;
        }
    }
    if (ndsUiKitBlitSurfacesAt(surfaces, sites, count) != FALSE)
    {
        sCssRuleShown = key + 1u;
        gNdsMenuShellCssPanelBlitCount += count;
    }
}

/* mnPlayersVSCursorProcUpdate's first furniture tests (:3318-3353): the right
 * arrow, then the left, at the button hotspot over the header band (y 12-35).
 * TRUE when the press stepped the rule. The descriptor follows at once, so a
 * wireless room's lobby shows the host's new rule. */
static u32 ndsMenuShellCssRuleArrow(void)
{
    s32 step;

    if (sCssTraining != 0u)
    {
        return FALSE;
    }
    if (ndsMenuShellCssBoxHit(210, 230, 12 + NDS_CSS_HEADER_DY,
                              35 + NDS_CSS_HEADER_DY) != FALSE)
    {
        step = 1;
    }
    else if (ndsMenuShellCssBoxHit(140, 160, 12 + NDS_CSS_HEADER_DY,
                                   35 + NDS_CSS_HEADER_DY) != FALSE)
    {
        step = -1;
    }
    else
    {
        return FALSE;
    }
    if (ndsMenuShellCssNetGuest() != FALSE)
    {
        return FALSE;
    }
    if (sCssRuleIsStock == 0u)
    {
        sCssRuleTime = ndsMenuShellCssRuleStepTime(sCssRuleTime, step);
    }
    else if (step > 0)
    {
        sCssRuleStock = (u8)((sCssRuleStock >= 98u) ? 0u : (sCssRuleStock + 1u));
    }
    else
    {
        sCssRuleStock = (u8)((sCssRuleStock == 0u) ? 98u : (sCssRuleStock - 1u));
    }
    gNdsMatchConfig.time_limit = sCssRuleTime;
    gNdsMatchConfig.stocks = sCssRuleStock;
    ndsMenuShellCssShowRule();
    ndsMenuShellCssCue(NDS_CSS_FGM_SCROLL2);
    return TRUE;
}

/* --- Drawing ------------------------------------------------------------- */

/* Text slots. P2-1k gave the four per-slot fighter names back: they are source
 * art baked into the gate surface now, so this screen holds four of the eight
 * and the other four are free for the next element that needs one. */

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
/* P3: the other humans' cursors, tag then hand per slot (36 + 2 * slot),
 * behind this console's own cursor and the tokens. */
#define NDS_CSS_SPRITE_NET_CURSOR0 36u

/* A human slot's own player art: the 1P..4P cursor tag, token and panel tag
 * (mnplayersvs.c:1719, :3434, :976). Only the 1P set is resident; the
 * networked select loads 2P-4P (ndsUiKitLoadNetPlayerImages). */
#define NDS_CSS_ART_CURSOR 0u
#define NDS_CSS_ART_PUCK 1u
#define NDS_CSS_ART_PANEL 2u

static u32 ndsMenuShellCssHumanArt(u32 art, u32 slot)
{
    static const u8 kArt[3][4] = {
        { NDS_MN_UI_KIT_IMAGE_CSS_CURSOR_1P, NDS_MN_UI_KIT_IMAGE_CSS_CURSOR_2P,
          NDS_MN_UI_KIT_IMAGE_CSS_CURSOR_3P, NDS_MN_UI_KIT_IMAGE_CSS_CURSOR_4P },
        { NDS_MN_UI_KIT_IMAGE_PUCK_1P, NDS_MN_UI_KIT_IMAGE_PUCK_2P,
          NDS_MN_UI_KIT_IMAGE_PUCK_3P, NDS_MN_UI_KIT_IMAGE_PUCK_4P },
        { NDS_MN_UI_KIT_IMAGE_PANEL_1P, NDS_MN_UI_KIT_IMAGE_PANEL_2P,
          NDS_MN_UI_KIT_IMAGE_PANEL_3P, NDS_MN_UI_KIT_IMAGE_PANEL_4P },
    };

    if ((sCssNetArt == 0u) || (slot >= 4u))
    {
        slot = 0u;
    }
    return kArt[art][slot];
}

/* mnPlayersVSArrowThreadUpdate's own blink (10 tics on, 10 off) and the two
 * values at which an arrow is EJECTED rather than blinked (:2643/:2660). */
#define NDS_CSS_ARROW_BLINK 10u
#define NDS_CSS_LEVEL_MIN 1u
#define NDS_CSS_LEVEL_MAX 9u

/* mnPlayersVSMakeLabels' own colours, mnplayersvs.c:1391. */
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
/* The twelve originals, then each compiled Remix selection in bake order. */
#define NDS_CSS_GATE_FIGHTERS (12u + NDS_P4_CSS_GATE_FIGHTERS)
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
#define NDS_CSS_GATE_LAST_FFA NDS_P4_CSS_GATE_LAST_FFA
#define NDS_CSS_GATE_LAST_TEAM NDS_P4_CSS_GATE_LAST_TEAM
_Static_assert(NDS_CSS_GATE_LAST_FFA ==
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
_Static_assert(NDS_CSS_GATE_LAST_TEAM ==
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

/* The save's own portrait treatment, per cell, at screen entry. The source
 * draws a locked fighter as the fire box, the fighter's noise-dithered
 * shadow and the question-mark plate (mnPlayersVSMakePortraitShadow,
 * mnplayersvs.c:374-426) INSTEAD of the portrait (mnplayersvs.c:443-446), so
 * covering the plate's unlocked portrait with the cell's opaque CSS_LOCKED_*
 * surface is the entry-time equivalent of taking the other branch. The mask
 * read is the source's init snapshot (mnplayersvs.c:4694) through
 * ndsMenuShellCssSaveLocked, and the save cannot change under this screen --
 * unlocks are earned in battle scenes via mnMessageApplyUnlock
 * (mnmessage.c:284-301) -- so entry is the only seam that must place them.
 * One blit for all four keeps it a single NitroFS open on a load frame, the
 * same budget the four gate panels already pay. */
/* P7 U7: a cell the save locks draws Remix's NONE plate (the bake's
 * CSS_LOCKED_CELL<n>_READY<r>, READY-aware as the flashes are); a selection
 * this build does not compile is baked so already. Appends READY state
 * `ready`'s covers to `list` from `n`; returns the new count. */
static u32 ndsMenuShellCssLockedCellCovers(NdsUiKitSurfaceId *list, u32 n,
                                           u32 ready)
{
    u32 cell;

    for (cell = 0u; cell < (u32)NDS_P4_CSS_CELLS; cell++)
    {
        u32 sel = (u32)kNdsP4CssCellSel[cell];

        if ((sel == (u32)nFTKindNull) || (sel == NDS_P4_SEL_RANDOM) ||
            (ndsMenuShellCssProgressLocked(sel) == FALSE))
        {
            continue;
        }
        list[n++] = (NdsUiKitSurfaceId)(NDS_MN_UI_KIT_SURFACE_CSS_LOCKED_CELL00_READY0 +
                                        (cell * 2u) + ready);
    }
    return n;
}

static void ndsMenuShellCssSyncLockedCells(void)
{
    NdsUiKitSurfaceId list[4];
    u32 count = 0u;
    u32 portrait;

    {
        NdsUiKitSurfaceId cells[NDS_P4_CSS_CELLS];
        u32 n = ndsMenuShellCssLockedCellCovers(
            cells, 0u, (sCssReadyShown == 1u) ? 1u : 0u);

        (void)list;
        (void)count;
        (void)portrait;
        if ((n != 0u) && (ndsUiKitBlitSurfaces(cells, n) != FALSE))
        {
            gNdsMenuShellCssPanelBlitCount += n;
        }
        return;
    }

    for (portrait = 0u; portrait < (u32)NDS_CSS_PORTRAITS; portrait++)
    {
        u32 fkind = (u32)kNdsCssPortraitFighter[portrait];
        NdsUiKitSurfaceId surface;

        if (ndsMenuShellCssSaveLocked(fkind) == FALSE)
        {
            continue;
        }
        switch (fkind)
        {
        case (u32)nFTKindLuigi:
            surface =
                (NdsUiKitSurfaceId)NDS_MN_UI_KIT_SURFACE_CSS_LOCKED_LUIGI;
            break;
        case (u32)nFTKindCaptain:
            surface =
                (NdsUiKitSurfaceId)NDS_MN_UI_KIT_SURFACE_CSS_LOCKED_CAPTAIN;
            break;
        case (u32)nFTKindNess:
            surface =
                (NdsUiKitSurfaceId)NDS_MN_UI_KIT_SURFACE_CSS_LOCKED_NESS;
            break;
        case (u32)nFTKindPurin:
            surface =
                (NdsUiKitSurfaceId)NDS_MN_UI_KIT_SURFACE_CSS_LOCKED_PURIN;
            break;
        default:
            /* Starters take the source's default-FALSE arm
             * (mnplayersvs.c:312-313) and never reach this switch locked. */
            continue;
        }
        list[count] = surface;
        count++;
    }
    if (count == 0u)
    {
        return;
    }
    if (ndsUiKitBlitSurfaces(list, count) != FALSE)
    {
        gNdsMenuShellCssPanelBlitCount += count;
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
#if NDS_LUIGI
    else if (fkind == (u32)nFTKindLuigi)
    {
        fighter = 2u;
    }
#endif
#if NDS_DONKEY
    else if (fkind == (u32)nFTKindDonkey)
    {
        fighter = 3u;
    }
#endif
#if NDS_CAPTAIN
    else if (fkind == (u32)nFTKindCaptain)
    {
        fighter = 4u;
    }
#endif
    else if (fkind == (u32)nFTKindSamus)
    {
        fighter = 5u;
    }
#if NDS_LINK
    else if (fkind == (u32)nFTKindLink)
    {
        fighter = 6u;
    }
#endif
    else if (fkind == (u32)nFTKindPikachu)
    {
        fighter = 7u;
    }
    else if (fkind == (u32)nFTKindYoshi)
    {
        fighter = 8u;
    }
    else if (fkind == (u32)nFTKindNess)
    {
        fighter = 9u;
    }
    else if (fkind == (u32)nFTKindPurin)
    {
        fighter = 10u;
    }
    else if (fkind == (u32)nFTKindKirby)
    {
        fighter = 11u;
    }
    else if ((ndsMenuShellCssSelContent(fkind) != 0u) &&
             (kNdsP4CssGateIndex[ndsMenuShellCssSelContent(fkind)] !=
              0xffu) &&
             (ndsP4Fighter(ndsMenuShellCssSelContent(fkind)) != NULL))
    {
        fighter = 12u +
            (u32)kNdsP4CssGateIndex[ndsMenuShellCssSelContent(fkind)];
    }
#ifdef NDS_P4_CSS_GATE_KIND_13
    else if (fkind == (u32)nFTKindMMario)
    {
        fighter = 12u + NDS_P4_CSS_GATE_KIND_13;
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
