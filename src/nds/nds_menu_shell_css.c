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
#if NDS_P4
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
#else
#define NDS_CSS_CELLS NDS_CSS_PORTRAITS
#define NDS_CSS_HEADER_DY 0
#define NDS_CSS_CARD_X(panel, sx) ((panel) + (sx))
#define NDS_CSS_CARD_Y(sy) (sy)
#define NDS_CSS_CARD_LEN(v) (v)
#define NDS_CSS_PRESS_Y 219
#endif

/* A selection's battle identity: its kind (a Remix selection's setup parent)
 * and its P4 content (0 for the original cast). */
static u32 ndsMenuShellCssSelFkind(u32 sel)
{
#if NDS_P4
    const NDSP4Fighter *p4 = ndsP4Fighter(ndsP4SelContent(sel));

    if (p4 != NULL)
    {
        return (u32)p4->parent_kind;
    }
#endif
    return sel;
}

static u32 ndsMenuShellCssSelContent(u32 sel)
{
#if NDS_P4
    return ndsP4SelContent(sel);
#else
    (void)sel;
    return 0u;
#endif
}

/* mnPlayersVSAdjustCursor's clamps and its full-deflection step. The P4
 * grid starts at x 0 (eleven 29-unit columns), so a token on its first column
 * (x 3-24) lay left of the hand's grab point at the source's minimum (cursor
 * x + 25): MRGAW's CP badge could not be picked up (owner BUGS.md
 * 2026-10-12). The hand may run 20 units past the left edge there. */
#if NDS_P4
#define NDS_CSS_CURSOR_X_MIN (-20)
#else
#define NDS_CSS_CURSOR_X_MIN 0
#endif
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
#if NDS_P4
#define NDS_CSS_PUCK_HOME_SLOT_X(i) NDS_CSS_CARD_X(69 * (s32)(i), 51)
#else
#define NDS_CSS_PUCK_HOME_SLOT_X(i) (NDS_CSS_PUCK_HOME_X + 68 * (s32)(i))
#endif
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
#if NDS_P2_KIRBY
/* Kirby lands behind his own flag; every earlier opt-in fighter rides its own
 * flag in this arm (admit_fighter.py). */
#define NDS_CSS_FIGHTER_MASK \
    (LBBACKUP_MASK_FIGHTER(nFTKindMario) | LBBACKUP_MASK_FIGHTER(nFTKindFox) | \
     LBBACKUP_MASK_FIGHTER(nFTKindLuigi) | \
     LBBACKUP_MASK_FIGHTER(nFTKindDonkey) | \
     LBBACKUP_MASK_FIGHTER(nFTKindCaptain) | \
     LBBACKUP_MASK_FIGHTER(nFTKindSamus) | \
     (NDS_P2_LINK ? LBBACKUP_MASK_FIGHTER(nFTKindLink) : 0u) | \
     (NDS_P2_PIKACHU ? LBBACKUP_MASK_FIGHTER(nFTKindPikachu) : 0u) | \
     (NDS_P2_YOSHI ? LBBACKUP_MASK_FIGHTER(nFTKindYoshi) : 0u) | \
     (NDS_P2_NESS ? LBBACKUP_MASK_FIGHTER(nFTKindNess) : 0u) | \
     (NDS_P2_PURIN ? LBBACKUP_MASK_FIGHTER(nFTKindPurin) : 0u) | \
     LBBACKUP_MASK_FIGHTER(nFTKindKirby))
#elif NDS_P2_PURIN
/* Purin lands behind his own flag; every earlier opt-in fighter rides its own
 * flag in this arm (admit_fighter.py). */
#define NDS_CSS_FIGHTER_MASK \
    (LBBACKUP_MASK_FIGHTER(nFTKindMario) | LBBACKUP_MASK_FIGHTER(nFTKindFox) | \
     LBBACKUP_MASK_FIGHTER(nFTKindLuigi) | \
     LBBACKUP_MASK_FIGHTER(nFTKindDonkey) | \
     LBBACKUP_MASK_FIGHTER(nFTKindCaptain) | \
     LBBACKUP_MASK_FIGHTER(nFTKindSamus) | \
     (NDS_P2_LINK ? LBBACKUP_MASK_FIGHTER(nFTKindLink) : 0u) | \
     (NDS_P2_PIKACHU ? LBBACKUP_MASK_FIGHTER(nFTKindPikachu) : 0u) | \
     (NDS_P2_YOSHI ? LBBACKUP_MASK_FIGHTER(nFTKindYoshi) : 0u) | \
     (NDS_P2_NESS ? LBBACKUP_MASK_FIGHTER(nFTKindNess) : 0u) | \
     LBBACKUP_MASK_FIGHTER(nFTKindPurin))
#elif NDS_P2_NESS
/* Ness lands behind his own flag; every earlier opt-in fighter rides its own
 * flag in this arm (admit_fighter.py). */
#define NDS_CSS_FIGHTER_MASK \
    (LBBACKUP_MASK_FIGHTER(nFTKindMario) | LBBACKUP_MASK_FIGHTER(nFTKindFox) | \
     LBBACKUP_MASK_FIGHTER(nFTKindLuigi) | \
     LBBACKUP_MASK_FIGHTER(nFTKindDonkey) | \
     LBBACKUP_MASK_FIGHTER(nFTKindCaptain) | \
     LBBACKUP_MASK_FIGHTER(nFTKindSamus) | \
     (NDS_P2_LINK ? LBBACKUP_MASK_FIGHTER(nFTKindLink) : 0u) | \
     (NDS_P2_PIKACHU ? LBBACKUP_MASK_FIGHTER(nFTKindPikachu) : 0u) | \
     (NDS_P2_YOSHI ? LBBACKUP_MASK_FIGHTER(nFTKindYoshi) : 0u) | \
     LBBACKUP_MASK_FIGHTER(nFTKindNess))
#elif NDS_P2_YOSHI
/* Yoshi lands behind his own flag; Link and Pikachu ride their own flags in
 * this arm for the same reason Pikachu's arm below carries Link. */
#define NDS_CSS_FIGHTER_MASK \
    (LBBACKUP_MASK_FIGHTER(nFTKindMario) | LBBACKUP_MASK_FIGHTER(nFTKindFox) | \
     LBBACKUP_MASK_FIGHTER(nFTKindLuigi) | \
     LBBACKUP_MASK_FIGHTER(nFTKindDonkey) | \
     LBBACKUP_MASK_FIGHTER(nFTKindCaptain) | \
     LBBACKUP_MASK_FIGHTER(nFTKindSamus) | \
     (NDS_P2_LINK ? LBBACKUP_MASK_FIGHTER(nFTKindLink) : 0u) | \
     (NDS_P2_PIKACHU ? LBBACKUP_MASK_FIGHTER(nFTKindPikachu) : 0u) | \
     LBBACKUP_MASK_FIGHTER(nFTKindYoshi))
#elif NDS_P2_PIKACHU
/* Pikachu lands behind his own flag while Link's CSS admission is still in
 * flight, so this arm carries Link only when that flag is also set. */
#define NDS_CSS_FIGHTER_MASK \
    (LBBACKUP_MASK_FIGHTER(nFTKindMario) | LBBACKUP_MASK_FIGHTER(nFTKindFox) | \
     LBBACKUP_MASK_FIGHTER(nFTKindLuigi) | \
     LBBACKUP_MASK_FIGHTER(nFTKindDonkey) | \
     LBBACKUP_MASK_FIGHTER(nFTKindCaptain) | \
     LBBACKUP_MASK_FIGHTER(nFTKindSamus) | \
     (NDS_P2_LINK ? LBBACKUP_MASK_FIGHTER(nFTKindLink) : 0u) | \
     LBBACKUP_MASK_FIGHTER(nFTKindPikachu))
#elif NDS_P2_LINK
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
static u8 sCssHandicap[NDS_CSS_SLOTS];
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
/* P7 C4 (owner, 2026-10-18: "Training mode enabled"). This screen is also
 * the training select (mnplayers1ptraining.c): the man in the cursor's slot
 * and the COM in the other of 0/1 (mnPlayers1PTrainingInitVars), the other
 * two shut; no mode toggle, rule bar, kind buttons or CPU level (the training
 * menu owns the COM). START always opens the stage select, B goes back to the
 * 1P menu, and the commit writes the scene's training_man/com pair
 * (mnPlayers1PTrainingSetSceneData) and each one's content. */
static u8 sCssTraining;
#if NDS_P4
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
#else
#define NDS_CSS_ONEP 0
#define NDS_CSS_BONUS 0
#endif
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
#if NDS_P4
    if (sCssOneP != 0u)
    {
        /* mnPlayers1PGameFuncStart's tail (:3478); the bonus selects
         * announce nothing (mnPlayers1PBonusFuncStart :2814-2873). */
        if (NDS_CSS_BONUS == 0)
        {
            ndsMenuShellCssCue((u32)nSYAudioVoiceAnnounceSelectPlayer);
        }
        return;
    }
#endif
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

#if NDS_P4
    if (ndsMenuShellCssSelContent(fkind) != 0u)
    {
        /* add_to_css's own announcer FGM for a Remix selection. */
        ndsMenuShellCssCue(NDS_CSS_FGM_ANNOUNCE_WHOOSH);
        ndsMenuShellCssCue(
            (u32)kNdsP4CssAnnounceFgm[ndsMenuShellCssSelContent(fkind)]);
        gNdsMenuShellCssAnnounceCount++;
        return;
    }
#endif
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
#if NDS_P4
    if (ndsMenuShellCssSelContent(sel) != 0u)
    {
        return (ndsMenuShellCssP7FighterOpen(NDS_P7_SEL_CONTENT(ndsMenuShellCssSelContent(sel))) != FALSE) ?
            FALSE : TRUE;
    }
#endif
    if (sel == (u32)nFTKindMMario)
    {
        return (ndsMenuShellCssP7FighterOpen(NDS_P7_SEL_METAL) != FALSE) ? FALSE : TRUE;
    }
    return ndsMenuShellCssSaveLocked(sel);
}

static u32 ndsMenuShellCssFighterLocked(u32 fkind)
{
#if NDS_P4
    /* A Remix selection exists when this build compiles its content, and is
     * open when the save has unlocked it (P7). */
    if (ndsMenuShellCssSelContent(fkind) != 0u)
    {
        return ((ndsP4Fighter(ndsMenuShellCssSelContent(fkind)) == NULL) ||
                (ndsMenuShellCssProgressLocked(fkind) != FALSE)) ? TRUE : FALSE;
    }
#endif
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

/* mnPlayersVSMakePortraitShadow's own placement, mnplayersvs.c:2412. */
static s32 ndsMenuShellCssPortraitX(u32 portrait)
{
    return (s32)(((portrait >= 6u) ? (portrait - 6u) : portrait) * 45u) + 25;
}

static s32 ndsMenuShellCssPortraitY(u32 portrait)
{
    return (s32)(((portrait >= 6u) ? 1u : 0u) * 43u) + 36;
}

#if NDS_P4
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
#endif

/* mnPlayersVSCenterPuckInPortrait, mnplayersvs.c:3454. */
static void ndsMenuShellCssCenterPuck(u32 slot, u32 fkind)
{
    u32 portrait;

#if NDS_P4
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
#endif

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

#if NDS_P4
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
#endif

/* mnPlayersVSGetPuckFighterKind, mnplayersvs.c:3001 -- which fighter the
 * token's CENTRE is over, or nFTKindNull. */
static u32 ndsMenuShellCssPuckFighterKind(u32 slot)
{
    s32 x = (s32)sCssPuckX[slot] + 13;
    s32 y = (s32)sCssPuckY[slot] + 12;
    u32 fkind;

#if NDS_P4
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
#endif

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
#if NDS_P4
    const s32 w = NDS_CSS_P4_PUCK_W;
    const s32 h = NDS_CSS_P4_PUCK_H;
#else
    const s32 w = 26;
    const s32 h = 24;
#endif

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
#if NDS_P4
/* The twelve originals, then each compiled Remix selection in bake order. */
#define NDS_CSS_GATE_FIGHTERS (12u + NDS_P4_CSS_GATE_FIGHTERS)
#else
#define NDS_CSS_GATE_FIGHTERS 12u
#endif
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
#if NDS_P4
#define NDS_CSS_GATE_LAST_FFA NDS_P4_CSS_GATE_LAST_FFA
#define NDS_CSS_GATE_LAST_TEAM NDS_P4_CSS_GATE_LAST_TEAM
#else
#define NDS_CSS_GATE_LAST_FFA NDS_MN_UI_KIT_SURFACE_CSS_GATE_3_HOLD_KIRBY
#define NDS_CSS_GATE_LAST_TEAM \
    NDS_MN_UI_KIT_SURFACE_CSS_GATE_TEAM_GREEN_3_HOLD_KIRBY
#endif
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
#if NDS_P4
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
#endif

static void ndsMenuShellCssSyncLockedCells(void)
{
    NdsUiKitSurfaceId list[4];
    u32 count = 0u;
    u32 portrait;

#if NDS_P4
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
#endif

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
#if NDS_P2_PIKACHU
    else if (fkind == (u32)nFTKindPikachu)
    {
        fighter = 7u;
    }
#endif
#if NDS_P2_YOSHI
    else if (fkind == (u32)nFTKindYoshi)
    {
        fighter = 8u;
    }
#endif
#if NDS_P2_NESS
    else if (fkind == (u32)nFTKindNess)
    {
        fighter = 9u;
    }
#endif
#if NDS_P2_PURIN
    else if (fkind == (u32)nFTKindPurin)
    {
        fighter = 10u;
    }
#endif
#if NDS_P2_KIRBY
    else if (fkind == (u32)nFTKindKirby)
    {
        fighter = 11u;
    }
#endif
#if NDS_P4
    else if ((ndsMenuShellCssSelContent(fkind) != 0u) &&
             (kNdsP4CssGateIndex[ndsMenuShellCssSelContent(fkind)] !=
              0xffu) &&
             (ndsP4Fighter(ndsMenuShellCssSelContent(fkind)) != NULL))
    {
        fighter = 12u +
            (u32)kNdsP4CssGateIndex[ndsMenuShellCssSelContent(fkind)];
    }
#endif
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

#if NDS_P4
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
#endif

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
 * (reloc_backend_assets.c, NDS_R2_ANIM_CACHE_CSS_KEEP_FREE). */
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

#if NDS_P4
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
#endif
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

#if NDS_P4
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
#endif
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
#if NDS_P4
    if (NDS_CSS_BONUS)
    {
        ndsMenuShellCssBonusShowRows();
    }
    else if (sCssOneP != 0u)
    {
        ndsMenuShellCssOnePShowOptions();
    }
    else
#endif
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
#if NDS_P4
/* The P4 grid flashes every cell: the kind IS the cell, and the bake emits
 * CSS_FLASH_CELL<n>_{OFF,ON}_READY{0,1} contiguously, four per cell. */
#define NDS_CSS_FLASH_KIND_COUNT NDS_P4_CSS_CELLS
_Static_assert(NDS_MN_UI_KIT_SURFACE_CSS_FLASH_CELL01_OFF_READY0 ==
                   NDS_MN_UI_KIT_SURFACE_CSS_FLASH_CELL00_OFF_READY0 + 4u,
               "P4 flash surfaces must stay four per cell");
#else
#define NDS_CSS_FLASH_KIND_COUNT 4u
#endif

static u32 ndsMenuShellCssFlashKindFromFighter(u32 fkind)
{
#if NDS_P4
    s32 cell = ndsMenuShellCssP4Cell(fkind);

    return (cell < 0) ? NDS_CSS_FLASH_KIND_NONE : (u32)cell;
#endif
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

#if NDS_P4
    return (NdsUiKitSurfaceId)(NDS_MN_UI_KIT_SURFACE_CSS_FLASH_CELL00_OFF_READY0 +
                               (kind * 4u) + (ready * 2u) +
                               ((visible != FALSE) ? 1u : 0u));
#else
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
#endif
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
#if NDS_P4
    /* The READY band (0,71,320,22) crosses the bottom portrait row, and its
     * state surface restores the unlocked base there: the covers of what the
     * save locks ride the same blit, right behind it. Re-covering after the
     * foreground and flash blits below (several NitroFS opens) let the
     * display scan the band out between, so a blink edge sometimes showed a
     * locked fighter (owner BUGS.md, CSS). */
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
    if (ndsUiKitBlitSurfaces(band, band_count) != FALSE)
    {
        gNdsMenuShellCssPanelBlitCount += band_count;
    }
#else
    if (ndsUiKitBlitSurfaces(&state, 1u) != FALSE)
    {
        gNdsMenuShellCssPanelBlitCount++;
    }
#endif
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
#if !NDS_P4
    /* The READY band (0,71,320,22) crosses the bottom portrait row, and its
     * state surface restores the unlocked base there -- so a blink edge would
     * erase rows 71..93 of a save-locked cell. Re-cover what the save locks. */
    ndsMenuShellCssSyncLockedCells();
#endif
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
#if NDS_P4
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
#endif
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
#if NDS_P4
            /* P4: Remix's random draws among every available selection. */
            fkind = ndsMenuShellCssP4RandomSel();
#else
            do
            {
                fkind = (u32)syUtilsRandTimeUCharRange(
                    (s32)nFTKindPlayableEnd + 1);
            }
            while (ndsMenuShellCssFighterLocked(fkind) != FALSE);
#endif
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
#if NDS_P4
    gNdsP4TrainingContent[0] = (u8)ndsMenuShellCssSelContent((u32)sCssFkind[man]);
    gNdsP4TrainingContent[1] = (u8)ndsMenuShellCssSelContent((u32)sCssFkind[com]);
#endif
    gNdsMenuShellCssCommitCount++;
}

#if NDS_P4
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
#endif

static void ndsMenuShellCssCommit(void)
{
    u32 i;

    if (sCssTraining != 0u)
    {
        ndsMenuShellCssCommitTraining();
        return;
    }
#if NDS_P4
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
#endif
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

#if NDS_P4
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
         * the title, the record and the total row, no cue), then BACK. */
        if (ndsMenuShellCssBoxHit(27, 207, 14 + NDS_CSS_HEADER_DY,
                                  35 + NDS_CSS_HEADER_DY) != FALSE)
        {
            sCssBonusKind = (sCssBonusKind == 0u) ? 1u : 0u;
            sCssBonusTotalKey = ndsMenuShellCssBonusTotalKey();
            ndsMenuShellCssBonusShowRows();
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
#endif

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

#if NDS_P4
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
#endif
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

#if NDS_P2_MENU_WALK && !NDS_P2_SHELL_ARGMAX_ROSTER
#if !((NDS_P2_LINK && (NDS_P2_PROOF_FIGHTER0 == 5)) || \
      (NDS_P2_CAPTAIN && (NDS_P2_PROOF_FIGHTER0 == 7)))
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
#if NDS_P4
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
#endif

static void ndsMenuShellCssWalkRestoreGate(void)
{
    u32 walk_kind0 = ndsMenuShellCssWalkTargetKind((u32)0);
    u32 walk_kind1 = ndsMenuShellCssWalkTargetKind((u32)1);

#if NDS_P4
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
#endif

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
#if NDS_P4
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
#endif
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

static void ndsMenuShellCssNetSlotOut(u32 slot, NdsNetLobbySlot *s)
{
    s->pkind = sCssPkind[slot];
    s->fkind = sCssFkind[slot];
    s->team = sCssTeam[slot];
    s->level = sCssLevel[slot];
    s->handicap = sCssHandicap[slot];
    s->selected = sCssSelected[slot];
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
    if (ndsUiKitBlitSurfaces(&label, 1u) != FALSE)
    {
        gNdsMenuShellCssPanelBlitCount++;
        if (sCssEntryStatesShown != 0u)
        {
            sCssEntryStatesShown = (u32)label + 1u;
        }
    }
    ndsMenuShellCssSyncPanels((u32)NDS_CSS_SLOTS);
    ndsMenuShellCssSyncTeamSelectAll();
    ndsMenuShellCssSyncLockedCells();
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

                    changed |= ndsMenuShellCssNetSlotIn(i, &proposal, FALSE);
                    /* That guest's costume presses, each the source's own
                     * costume change (mnPlayersVSFuncRun's C buttons). */
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
                else if ((ndsNetLobbyHostMemberPresent(i) == 0u) &&
                         (sCssSelected[i] != 0u))
                {
                    /* Still on Results: not ready until they are back. */
                    sCssSelected[i] = 0u;
                    changed = TRUE;
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
                    /* Its own costume is the host's to resolve as well. */
                    (void)ndsMNPlayersVSPreviewSetCostume(
                        i, (s32)slots[i].fkind, (s32)slots[i].costume);
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

#if NDS_P2_MENU_WALK && !NDS_P2_SHELL_ARGMAX_ROSTER
#if !((NDS_P2_LINK && (NDS_P2_PROOF_FIGHTER0 == 5)) || \
      (NDS_P2_CAPTAIN && (NDS_P2_PROOF_FIGHTER0 == 7)))
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
#if NDS_P4
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
#endif
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
#if NDS_P4
                        {
                            /* The mode label box (27,24,113,22) crosses the
                             * top portrait row and its state surface
                             * restores the unlocked base there: the locked
                             * covers ride the same blit (as the READY band's,
                             * ndsMenuShellCssShowReady). */
                            NdsUiKitSurfaceId box[1u + NDS_P4_CSS_CELLS];
                            u32 box_count;

                            box[0] = label;
                            box_count = ndsMenuShellCssLockedCellCovers(
                                box, 1u, (sCssReadyShown == 1u) ? 1u : 0u);
                            if (ndsUiKitBlitSurfaces(box, box_count) != FALSE)
                            {
                                gNdsMenuShellCssPanelBlitCount += box_count;
                                if (sCssEntryStatesShown != 0u)
                                {
                                    sCssEntryStatesShown = (u32)label + 1u;
                                }
                            }
                        }
#else
                        if (ndsUiKitBlitSurfaces(&label, 1u) != FALSE)
                        {
                            gNdsMenuShellCssPanelBlitCount++;
                            if (sCssEntryStatesShown != 0u)
                            {
                                sCssEntryStatesShown = (u32)label + 1u;
                            }
                        }
#endif
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
#if !NDS_P4
                        /* The mode label box (27,24,113,22) crosses the top
                         * portrait row, and its state surface restores the
                         * unlocked base there -- re-cover what the save locks. */
                        ndsMenuShellCssSyncLockedCells();
#endif
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
#if NDS_P2_MENU_WALK && !NDS_P2_SHELL_ARGMAX_ROSTER
#if !((NDS_P2_LINK && (NDS_P2_PROOF_FIGHTER0 == 5)) || \
      (NDS_P2_CAPTAIN && (NDS_P2_PROOF_FIGHTER0 == 7)))
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

#if NDS_P4
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
#endif
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
#if NDS_P4
        if ((fkind != (u32)nFTKindNull) &&
            (ndsP4Fighter(gNdsP4TrainingContent[slot_index]) != NULL))
        {
            fkind = NDS_P4_SEL_BASE + (u32)gNdsP4TrainingContent[slot_index];
        }
#else
        (void)slot_index;
#endif
        if ((fkind != (u32)nFTKindNull) &&
            (ndsMenuShellCssFighterLocked(fkind) != FALSE))
        {
            fkind = (u32)nFTKindNull;
        }
        if ((i == com) && (fkind == (u32)nFTKindNull))
        {
#if NDS_P4
            fkind = ndsMenuShellCssP4RandomSel();
#else
            do
            {
                fkind = (u32)syUtilsRandTimeUCharRange(
                    (s32)nFTKindPlayableEnd + 1);
            }
            while (ndsMenuShellCssFighterLocked(fkind) != FALSE);
#endif
        }
        sCssFkind[i] = (u8)fkind;
        sCssLevel[i] = 3u;
        sCssTeam[i] = (u8)((i == man) ? nSCBattleTeamIDRed : nSCBattleTeamIDBlue);
        sCssSelected[i] = (fkind != (u32)nFTKindNull) ? 1u : 0u;
        ndsMenuShellCssCenterPuck(i, fkind);
    }
}

#if NDS_P4
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
#endif

/* mnPlayersVSInitVars + mnPlayersVSInitPlayer + mnPlayersVSInitSlot,
 * mnplayersvs.c:4670/:4579/:4698, reading the P2-1a descriptor instead of
 * gSCManagerTransferBattleState: the descriptor is what the battle consumes, so
 * it is also what the screen that fills it should open from. */
static void ndsMenuShellCssInit(void)
{
    u32 i;

#if NDS_P2_MENU_WALK && !NDS_P2_SHELL_ARGMAX_ROSTER
#if !((NDS_P2_LINK && (NDS_P2_PROOF_FIGHTER0 == 5)) || \
      (NDS_P2_CAPTAIN && (NDS_P2_PROOF_FIGHTER0 == 7)))
    /* Per entry, not per boot: a second visit tours again, and the masks are
     * cumulative so a kind that drew on either visit reads as drawn. */
    ndsMenuShellCssWalkTourReset();
#endif
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
#if NDS_P4
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
#endif
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
#if NDS_P4
        if (ndsP4Fighter(cfg->p4_content) != NULL)
        {
            sCssFkind[i] = (u8)(NDS_P4_SEL_BASE + cfg->p4_content);
        }
#endif
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
#if NDS_P4
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
#endif
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

#if NDS_P2_MENU_WALK
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
#if NDS_P4
    sCssOnePShown = 0u;
#endif
    ndsMenuShellCssSyncPanels((u32)NDS_CSS_SLOTS);
    ndsMenuShellCssPopulate();
    ndsMenuShellCssSyncTeamSelectAll();
    ndsMenuShellCssShowReady(FALSE);
    /* After every surface that restores the unlocked base: the save's own
     * locked cells go on last, at entry, in one blit. */
    ndsMenuShellCssSyncLockedCells();
    ndsMenuShellCssMove();
}
