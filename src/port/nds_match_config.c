/* The match descriptor and its presets -- P2-1a.
 *
 * `include/nds/nds_match_config.h` owns the contract; this file owns the two
 * halves of it. `ndsMatchConfigLoadMarioFoxDreamLand` decides WHAT the match is
 * (and is the only reader of the build-time match flags); `ndsMatchConfigApply`
 * installs it into the engine and is the only writer of the canonical match
 * configuration.
 *
 * BEHAVIOUR IS UNCHANGED BY CONSTRUCTION, and that is the whole point of the
 * row. Both functions together reproduce, field for field, what
 * `scene_harness.c` used to write inline for mode 163 -- including the two
 * things that are easy to lose when straight-line code becomes a struct:
 *
 *   - The base copy `gSCManagerTransferBattleState = dSCManagerDefaultBattleState`
 *     is load-bearing and stays. It supplies every SCBattleState field the
 *     descriptor deliberately does not model -- game_type, is_show_score,
 *     is_not_teamshadows, and each slot's tag/place/score/falls/
 *     damage totals/stale ring. Dropping it would zero a match's worth of state
 *     that Results reads.
 *   - The trailing `dSCManagerDefaultBattleState = gSCManagerTransferBattleState`
 *     snapshot stays too. `ndsMNVSResultsSetLoadScene` re-runs the whole apply
 *     on a START rematch precisely so match two starts from byte-identical
 *     state rather than from whatever match one left behind, and that only
 *     works because the snapshot is what the base copy reads on re-entry.
 */

#include <ft/fighter.h>
#include <if/interface.h>
#include <nds/nds_match_config.h>
#include <nds/nds_scene_harness.h>
#include <sc/scene.h>
#include <it/item.h>
#include "nds_build_config.h"

NdsMatchConfig gNdsMatchConfig;

#if NDS_P2_PROOF_FIGHTER0 >= 0
/* nFTKindPlayableEnd is inclusive (== nFTKindNess), as mnplayersvs reads it. */
_Static_assert(NDS_P2_PROOF_FIGHTER0 <= nFTKindPlayableEnd,
               "NDS_P2_PROOF_FIGHTER0 must be a playable fighter kind");
#if NDS_P2_PROOF_FIGHTER0 == 4 && !NDS_P2_LUIGI
/* ft/fighter.h / BattleShip fttypes.h: nFTKindLuigi == 4. The preprocessor
 * cannot compare an enum identifier (an undefined token becomes 0 in #if), so
 * pin the source integer here while the C static assertion above owns range. */
#error "Luigi proof fighter requires NDS_P2_LUIGI=1"
#endif
#if NDS_P2_PROOF_FIGHTER0 == 2 && !NDS_P2_DONKEY
/* BattleShip fttypes.h: nFTKindDonkey == 2. Keep the proof selector incapable
 * of creating a fighter whose source assets/native owner were not admitted. */
#error "Donkey proof fighter requires NDS_P2_DONKEY=1"
#endif
#if NDS_P2_PROOF_FIGHTER0 == 7 && !NDS_P2_CAPTAIN
/* BattleShip fttypes.h: nFTKindCaptain == 7. Same rule as the two above. */
#error "Captain Falcon proof fighter requires NDS_P2_CAPTAIN=1"
#endif
#if NDS_P2_PROOF_FIGHTER0 == 3 && !NDS_P2_SAMUS
/* BattleShip fttypes.h: nFTKindSamus == 3. Samus's first production landing
 * uses the generic renderer, but the source assets/status/weapon TUs must still
 * be admitted before the direct proof descriptor can instantiate her. */
#error "Samus proof fighter requires NDS_P2_SAMUS=1"
#endif
#if NDS_P2_PROOF_FIGHTER0 == 5 && !NDS_P2_LINK
/* BattleShip fttypes.h: nFTKindLink == 5. Keep the proof selector bound to the
 * same production-admission flag that stages Link's full source closure and
 * native owner. */
#error "Link proof fighter requires NDS_P2_LINK=1"
#endif
#if NDS_P2_PROOF_FIGHTER0 == 9 && !NDS_P2_PIKACHU
/* BattleShip fttypes.h: nFTKindPikachu == 9. Same rule: the proof selector
 * cannot instantiate a fighter whose source closure was not admitted. */
#error "Pikachu proof fighter requires NDS_P2_PIKACHU=1"
#endif
#if NDS_P2_PROOF_FIGHTER0 == 6 && !NDS_P2_YOSHI
/* BattleShip fttypes.h: nFTKindYoshi == 6. Same rule: the proof selector
 * cannot instantiate a fighter whose source closure was not admitted. */
#error "Yoshi proof fighter requires NDS_P2_YOSHI=1"
#endif
#if NDS_P2_PROOF_FIGHTER0 == 11 && !NDS_P2_NESS
#error "Ness proof fighter requires NDS_P2_NESS=1"
#endif
#if NDS_P2_PROOF_FIGHTER0 == 10 && !NDS_P2_PURIN
#error "Purin proof fighter requires NDS_P2_PURIN=1"
#endif
#if NDS_P2_PROOF_FIGHTER0 == 8 && !NDS_P2_KIRBY
#error "Kirby proof fighter requires NDS_P2_KIRBY=1"
#endif
#if NDS_P2_PROOF_FIGHTER0 == 26 && !NDS_P2_GDONKEY
#error "GDonkey proof fighter requires NDS_P2_GDONKEY=1"
#endif
#if NDS_P2_PROOF_FIGHTER0 == 13 && !NDS_P2_MMARIO
#error "MMario proof fighter requires NDS_P2_MMARIO=1"
#endif
#if NDS_P2_PROOF_FIGHTER0 == 14 && !NDS_P2_NMARIO
#error "NMario proof fighter requires NDS_P2_NMARIO=1"
#endif
#if NDS_P2_PROOF_FIGHTER0 == 15 && !NDS_P2_NFOX
#error "NFox proof fighter requires NDS_P2_NFOX=1"
#endif
#if NDS_P2_PROOF_FIGHTER0 == 16 && !NDS_P2_NDONKEY
#error "NDonkey proof fighter requires NDS_P2_NDONKEY=1"
#endif
#if NDS_P2_PROOF_FIGHTER0 == 17 && !NDS_P2_NSAMUS
#error "NSamus proof fighter requires NDS_P2_NSAMUS=1"
#endif
#if NDS_P2_PROOF_FIGHTER0 == 18 && !NDS_P2_NLUIGI
#error "NLuigi proof fighter requires NDS_P2_NLUIGI=1"
#endif
#if NDS_P2_PROOF_FIGHTER0 == 19 && !NDS_P2_NLINK
#error "NLink proof fighter requires NDS_P2_NLINK=1"
#endif
#if NDS_P2_PROOF_FIGHTER0 == 20 && !NDS_P2_NYOSHI
#error "NYoshi proof fighter requires NDS_P2_NYOSHI=1"
#endif
#if NDS_P2_PROOF_FIGHTER0 == 21 && !NDS_P2_NCAPTAIN
#error "NCaptain proof fighter requires NDS_P2_NCAPTAIN=1"
#endif
#if NDS_P2_PROOF_FIGHTER0 == 22 && !NDS_P2_NKIRBY
#error "NKirby proof fighter requires NDS_P2_NKIRBY=1"
#endif
#if NDS_P2_PROOF_FIGHTER0 == 23 && !NDS_P2_NPIKACHU
#error "NPikachu proof fighter requires NDS_P2_NPIKACHU=1"
#endif
#if NDS_P2_PROOF_FIGHTER0 == 24 && !NDS_P2_NPURIN
#error "NPurin proof fighter requires NDS_P2_NPURIN=1"
#endif
#if NDS_P2_PROOF_FIGHTER0 == 25 && !NDS_P2_NNESS
#error "NNess proof fighter requires NDS_P2_NNESS=1"
#endif
#endif

#if NDS_P2_FOUR_CPU_ROSTER && (!NDS_P2_LUIGI || !NDS_P2_DONKEY || !NDS_P2_CAPTAIN || !NDS_P2_SAMUS)
#error "NDS_P2_FOUR_CPU_ROSTER=1 needs landed owner admission: NDS_P2_LUIGI=1 NDS_P2_DONKEY=1 NDS_P2_CAPTAIN=1 NDS_P2_SAMUS=1"
#endif
#if NDS_P2_FOUR_CPU_ROSTER
/* The roster arm's four kinds come from the build (Makefile
 * NDS_P2_FOUR_CPU_KIND0..3, BattleShip fttypes.h ordinals) so a fighter row
 * can measure itself under the stress config without editing this file. A
 * kind whose source closure is not admitted cannot be instantiated. */
/* Each clause is its own parenthesised term. The four P2-3 roster-close
 * clauses were misgrouped -- the opening paren before `(k) == 6` was
 * never closed before its `||`, so Yoshi, Ness, Purin and Kirby shared
 * one term and the #if below could accept a kind whose admission flag is
 * off. It balanced, so it compiled. */
#define NDS_P2_KIND_ADMITTED(k) \
    ((k) == 0 || (k) == 1 || ((k) == 4 && NDS_P2_LUIGI) || \
     ((k) == 2 && NDS_P2_DONKEY) || ((k) == 7 && NDS_P2_CAPTAIN) || \
     ((k) == 3 && NDS_P2_SAMUS) || ((k) == 5 && NDS_P2_LINK) || \
     ((k) == 9 && NDS_P2_PIKACHU) || ((k) == 6 && NDS_P2_YOSHI) || \
     ((k) == 11 && NDS_P2_NESS) || ((k) == 10 && NDS_P2_PURIN) || \
     ((k) == 8 && NDS_P2_KIRBY || \
     ((k) == 26 && NDS_P2_GDONKEY || \
     ((k) == 13 && NDS_P2_MMARIO || \
     ((k) == 14 && NDS_P2_NMARIO || \
     ((k) == 15 && NDS_P2_NFOX || \
     ((k) == 16 && NDS_P2_NDONKEY || \
     ((k) == 17 && NDS_P2_NSAMUS || \
     ((k) == 18 && NDS_P2_NLUIGI || \
     ((k) == 19 && NDS_P2_NLINK || \
     ((k) == 20 && NDS_P2_NYOSHI || \
     ((k) == 21 && NDS_P2_NCAPTAIN || \
     ((k) == 22 && NDS_P2_NKIRBY || \
     ((k) == 23 && NDS_P2_NPIKACHU || \
     ((k) == 24 && NDS_P2_NPURIN || \
     ((k) == 25 && NDS_P2_NNESS))
#if !NDS_P2_KIND_ADMITTED(NDS_P2_FOUR_CPU_KIND0) || \
    !NDS_P2_KIND_ADMITTED(NDS_P2_FOUR_CPU_KIND1) || \
    !NDS_P2_KIND_ADMITTED(NDS_P2_FOUR_CPU_KIND2) || \
    !NDS_P2_KIND_ADMITTED(NDS_P2_FOUR_CPU_KIND3)
#error "NDS_P2_FOUR_CPU_KIND0..3 names a fighter whose NDS_P2_<X> admission flag is off"
#endif
#endif

void ndsMatchConfigLoadMarioFoxDreamLand(NdsMatchConfig *cfg)
{
    s32 i;

    /* The two-fighter Dream Land base, as the canonical configuration has
     * always declared it. `item_toggles`/`item_appearance_rate` are the
     * source's own defaults (scmanager.c:578,583 -- every item on at the middle
     * rate); the inline version inherited them through the base copy instead of
     * naming them, which is the same value by a less legible route. The live
     * arm below turns items off, which is what mode 163 actually ships. */
    cfg->gkind = nGRKindPupupu;
    cfg->game_rules = SCBATTLE_GAMERULE_TIME;
    cfg->game_type = NDS_MATCH_NO_GAME_TYPE;
    cfg->spgame_stage = NDS_MATCH_NO_SPGAME_STAGE;
    cfg->time_limit = 3;
    cfg->stocks = 2;
    cfg->handicap_mode = nSCBattleHandicapOff;
    cfg->item_appearance_rate = nSCBattleItemSwitchMiddle;
    /* scmanager.c:577, the source's own default. The base copy was already
     * supplying this value, so naming it here changes nothing today and
     * gives the VS Options Damage row somewhere to write. */
    cfg->damage_ratio = 100;
    cfg->is_team_battle = FALSE;
    cfg->is_team_attack = FALSE;
    cfg->is_stage_select = TRUE;
    cfg->is_reset_players = FALSE;
    cfg->item_toggles = ~0u;

    for (i = 0; i < NDS_MATCH_FIGHTERS_MAX; i++)
    {
        cfg->fighters[i].fkind = nFTKindNull;
        cfg->fighters[i].pkind = nFTPlayerKindNot;
        cfg->fighters[i].level = 1;
        cfg->fighters[i].handicap = 0;
        cfg->fighters[i].team = 0;
        cfg->fighters[i].costume = 0;
        cfg->fighters[i].shade = 0;
        cfg->fighters[i].color = 0;
        /* 1P-owned; the VS path never reads them (see the header). Sentinel
         * so a reused descriptor cannot leak ladder state into a VS commit. */
        cfg->fighters[i].stock_count = 0;
        cfg->fighters[i].is_spgame_enemy = FALSE;
        cfg->fighters[i].copy_kind = NDS_MATCH_NO_COPY_KIND;
    }

    cfg->fighters[0].fkind = nFTKindMario;
    cfg->fighters[0].pkind = nFTPlayerKindMan;
    cfg->fighters[0].handicap = 9;
    cfg->fighters[0].team = 0;
#if NDS_P2_PROOF_FIGHTER0 >= 0
    /* Focused P2-3 proof only.  Keep this at the descriptor owner instead of
     * teaching scene_harness or the combat runtime about individual roster
     * additions: BattleShip still receives an ordinary FTDesc with the chosen
     * fkind, and all status/motion/attribute dispatch remains source-owned. */
    cfg->fighters[0].fkind = (u8)NDS_P2_PROOF_FIGHTER0;
#endif

    cfg->fighters[1].fkind = nFTKindFox;
    cfg->fighters[1].pkind = nFTPlayerKindMan;
    cfg->fighters[1].handicap = 9;
    cfg->fighters[1].team = 1;

#if NDS_DEV_LIVE_INPUT_PREVIEW
    /* The shipped match: one-minute Time, items off, Fox on the CPU. */
    cfg->game_rules = SCBATTLE_GAMERULE_TIME;
    cfg->time_limit = 1;
    cfg->item_toggles = 0u;
    cfg->item_appearance_rate = nSCBattleItemSwitchNone;
    cfg->fighters[1].pkind = nFTPlayerKindCom;
#if NDS_DEMO_FOX_CPU_LADDER
    /* The demo ladder owns Fox's level: 1 at boot, +1 per Mario win, wrapping
     * 9 -> 1 (owner, 2026-08-17). `ndsMNVSResultsSetLoadScene` advances it just
     * before this preset re-runs on a START restart, so the descriptor picks up
     * the new level on the match it belongs to rather than one match late. The
     * NDS_R2_BOTH_CPU stress arm below deliberately does NOT ride the ladder --
     * it re-pins both fighters at level 3 so every banked measurement stays
     * comparable. */
    cfg->fighters[1].level = (u8)gNdsDemoFoxCpuLevel;
#else
    cfg->fighters[1].level = 3;
#endif
#if NDS_R2_BOTH_CPU
    /* Switch plan R2-06's harness prerequisite, owner-requested 2026-07-29.
     * Mario becomes a level-3 CPU too, so both fighters attack continuously
     * without a recorded input stream. That is what makes it useful: it
     * maximises the live hitbox population, which R2-03 E35 measured as the
     * owner of the SRC P95 excursion, so it is a deliberate STRESS case.
     *
     * The plan is explicit that this is a harness configuration and not a
     * product change: "The shipped Boundary stays Mario human vs level-3 Fox CPU
     * at mode 163, and PROJECT_GOAL.md's P95 gate is defined on *representative*
     * gameplay -- so a P95 read off the stress config is a harder number than
     * the milestone requires and must be reported as such, never swapped in
     * silently for the Boundary figure." Honour that: never publish a number
     * from this build as the Boundary P95.
     *
     * OFF THE DEMO LADDER ON PURPOSE. Every banked gate figure was measured
     * with both fighters at level 3, so this arm re-pins Fox even when
     * NDS_DEMO_FOX_CPU_LADDER seeded it from the ladder above. Letting the
     * ladder reach this arm would make a rematched measurement incomparable to
     * the one before it, silently.
     *
     * THE MATCH LENGTH IS NOT THIS FLAG'S BUSINESS. This branch changes WHO
     * plays, never HOW LONG -- the one-minute Time match set above stands.
     *
     * It used to seed time_limit = 7 here, for the freeze soak, and that made
     * the gate arm sample a 420-second match through a window sized for a
     * 60-second one: measured 2026-08-05, this arm's banked "whole match"
     * baseline covered 12.6% of its own match (the opening minute) against
     * Boundary's 86.7%. Every both-CPU tick figure in the campaign, and the
     * SRC/MISC split derived from them, was superseded by that one line.
     * Owner's ruling the same day: *"the soak was only meant to catch freezes,
     * boundary and both cpu gates should be the 60 sec match"*.
     *
     * The soak's long match is real and still needed -- see the block below --
     * but it is a soak property, not a stress-config property, so it lives on
     * its own flag where reading it cannot be mistaken for reading the gate. */
    cfg->fighters[0].pkind = nFTPlayerKindCom;
    cfg->fighters[0].level = 3;
    cfg->fighters[1].level = 3;
#if NDS_P2_MENU_SHELL
    /* P2-2 standing stress seed.  Keep the old direct-boot R2 arm byte-for-byte
     * two-fighter so its banked measurements remain comparable, but when that
     * same stress switch is used on the P2 shell it means what the phase plan
     * now says: FOUR level-3 CPUs, using only the two ported fighter kinds.
     *
     * Mario/Fox/Mario/Fox deliberately exercises same-kind mirror instances in
     * P3/P4.  The CSS is still the source-of-truth appearance owner: on entry
     * mnPlayersVSGetFreeCostume assigns distinct FFA costumes in slot order, and
     * Team Battle reassigns team costumes/shades through the imported source
     * helpers.  RRBB is BattleShip mnPlayersVSResetPlayer's default team split.
     * No shipping shell target enables NDS_R2_BOTH_CPU by default. */
    cfg->fighters[0].team = nSCBattleTeamIDRed;
    cfg->fighters[1].team = nSCBattleTeamIDRed;

    cfg->fighters[2].fkind = nFTKindMario;
    cfg->fighters[2].pkind = nFTPlayerKindCom;
    cfg->fighters[2].level = 3;
    cfg->fighters[2].team = nSCBattleTeamIDBlue;

    cfg->fighters[3].fkind = nFTKindFox;
    cfg->fighters[3].pkind = nFTPlayerKindCom;
    cfg->fighters[3].level = 3;
    cfg->fighters[3].team = nSCBattleTeamIDBlue;
#endif
#endif
#if NDS_P2_FOUR_CPU_STRESS
    /* P2-2 standing gate: four source VSBattle instances, not four renderer
     * owners. The content set is still only Mario/Fox, so mirrors deliberately
     * occupy P3/P4 and exercise every instance-indexed fighter/render path.
     *
     * This direct-battle target bypasses PlayersVS, so reproduce the state that
     * the source CSS would have committed rather than inventing a stress-only
     * appearance policy: FFA shade is 0; the second Mario/Fox each take common
     * costume 1 after slots 0/1 already occupy common costume 0; reset teams are
     * Red/Red/Blue/Blue (mnPlayersVSResetPlayer). Team fields are dormant in FFA
     * but remain source-valid if this descriptor is inspected before apply. */
    cfg->fighters[0].pkind = nFTPlayerKindCom;
    cfg->fighters[0].level = 3;
    cfg->fighters[0].team = nSCBattleTeamIDRed;
    cfg->fighters[0].costume = (u8)ftParamGetCostumeCommonID(nFTKindMario, 0);
    cfg->fighters[0].shade = 0;

    cfg->fighters[1].pkind = nFTPlayerKindCom;
    cfg->fighters[1].level = 3;
    cfg->fighters[1].team = nSCBattleTeamIDRed;
    cfg->fighters[1].costume = (u8)ftParamGetCostumeCommonID(nFTKindFox, 0);
    cfg->fighters[1].shade = 0;

    cfg->fighters[2].fkind = nFTKindMario;
    cfg->fighters[2].pkind = nFTPlayerKindCom;
    cfg->fighters[2].level = 3;
    cfg->fighters[2].team = nSCBattleTeamIDBlue;
    cfg->fighters[2].costume = (u8)ftParamGetCostumeCommonID(nFTKindMario, 1);
    cfg->fighters[2].shade = 0;

    cfg->fighters[3].fkind = nFTKindFox;
    cfg->fighters[3].pkind = nFTPlayerKindCom;
    cfg->fighters[3].level = 3;
    cfg->fighters[3].team = nSCBattleTeamIDBlue;
    cfg->fighters[3].costume = (u8)ftParamGetCostumeCommonID(nFTKindFox, 1);
    cfg->fighters[3].shade = 0;
    /* PROJECT_GOAL's P2 gate is "all items on". The tick-HUD target family
     * forces NDS_DEV_LIVE_INPUT_PREVIEW (Makefile:2526), whose block above
     * zeroes item_toggles for the shipped one-minute demo, and until
     * 2026-09-04 nothing here put them back -- every four-CPU figure was an
     * items-off figure while the board said "Stress = items ON". Restore the
     * source defaults the CSS would have committed: every kind enabled and
     * the rate scmanager.c:583 seeds, nSCBattleItemSwitchMiddle. */
    cfg->item_toggles = ~0u;
    cfg->item_appearance_rate = nSCBattleItemSwitchMiddle;
#if NDS_P2_FOUR_CPU_ROSTER
    /* THE STRESS ARM FOLLOWS THE LANDED ROSTER, which is what PROJECT_GOAL's
     * P2 gate actually asks for: "the measured hardest fighter set", a measured
     * argmax over landed content rather than a guess frozen when the content
     * set was two names. P2-3f22 adds Samus to that calculation. Her source
     * SamusMain allocation is 85,296 B, or 83,008 B unique after the same
     * 2,288 B already-resident pair used by the earlier budget rows. That is
     * heavier than Mario's 54,048 B and lighter than Captain/Fox, so the new
     * six-kind argmax is Samus/Fox/Captain/Donkey. Luigi and Mario remain
     * admitted/compiled where their dense-owner ABI requires it; neither is
     * instantiated by this stress descriptor.
     *
     * All four are distinct, so common costume 0 is source-legal for every
     * slot and no duplicate-costume rule is involved. */
    cfg->fighters[0].fkind = (FTKind)NDS_P2_FOUR_CPU_KIND0;
    cfg->fighters[0].costume =
        (u8)ftParamGetCostumeCommonID((FTKind)NDS_P2_FOUR_CPU_KIND0, 0);
    cfg->fighters[1].fkind = (FTKind)NDS_P2_FOUR_CPU_KIND1;
    cfg->fighters[1].costume =
        (u8)ftParamGetCostumeCommonID((FTKind)NDS_P2_FOUR_CPU_KIND1, 0);
    cfg->fighters[2].fkind = (FTKind)NDS_P2_FOUR_CPU_KIND2;
    cfg->fighters[2].costume =
        (u8)ftParamGetCostumeCommonID((FTKind)NDS_P2_FOUR_CPU_KIND2, 0);
    cfg->fighters[3].fkind = (FTKind)NDS_P2_FOUR_CPU_KIND3;
    cfg->fighters[3].costume =
        (u8)ftParamGetCostumeCommonID((FTKind)NDS_P2_FOUR_CPU_KIND3, 0);
    /* BattleShip's scvsbattle.c selects Low detail for every 3+ fighter match;
     * the ordinary fighter creation path carries that policy into all four. */
#endif
#endif
#if NDS_P2_SHELL_ARGMAX_ROSTER
    /* P2-3f9/f22: THE HEAVIEST ROSTER A PLAYER CAN REACH, SEEDED INTO THE
     * SCREEN THAT LETS THEM REACH IT.
     *
     * PROJECT_GOAL's P2 gate asks for "the measured hardest fighter set", and
     * with Samus landed the source main-file charges put her at 83,008 B unique
     * (85,296 B standalone minus the 2,288 B pair already resident in a
     * multi-kind match). That makes the six-kind argmax
     * Fox/Captain/Samus/Donkey: Samus displaces Mario from the P2-3f11 roster.
     *
     * This is a DESCRIPTOR seed, not a battle seed: it is what the character
     * select opens on (ndsMenuShellCssInit reads exactly these fields), so the
     * whole shell still runs -- four live 3D previews, four gates, the
     * announcer, the stage select -- and the battle loads whatever the CSS
     * commits. That is the difference from NDS_P2_FOUR_CPU_STRESS below, which
     * writes the battle state directly and never enters PlayersVS.
     *
     * Slot 0 stays the HUMAN. One console is one controller, so 1 human + 3
     * CPUs is the configuration P2 actually ships; four distinct kinds cost the
     * arena the same either way. */
#if !NDS_P2_MENU_SHELL
#error "NDS_P2_SHELL_ARGMAX_ROSTER=1 is a shell configuration: needs NDS_P2_MENU_SHELL=1"
#endif
#if !NDS_P2_CAPTAIN || !NDS_P2_DONKEY || !NDS_P2_LUIGI || !NDS_P2_SAMUS
#error "NDS_P2_SHELL_ARGMAX_ROSTER=1 needs the six-name roster (NDS_P2_SHELL_ROSTER=4)"
#endif
    cfg->fighters[0].fkind = nFTKindSamus;
    cfg->fighters[0].pkind = nFTPlayerKindMan;
    cfg->fighters[0].level = 3;
    cfg->fighters[0].team = nSCBattleTeamIDRed;
    cfg->fighters[0].costume = (u8)ftParamGetCostumeCommonID(nFTKindSamus, 0);
    cfg->fighters[0].shade = 0;

    cfg->fighters[1].fkind = nFTKindFox;
    cfg->fighters[1].pkind = nFTPlayerKindCom;
    cfg->fighters[1].level = 3;
    cfg->fighters[1].team = nSCBattleTeamIDRed;
    cfg->fighters[1].costume = (u8)ftParamGetCostumeCommonID(nFTKindFox, 0);
    cfg->fighters[1].shade = 0;

    cfg->fighters[2].fkind = nFTKindCaptain;
    cfg->fighters[2].pkind = nFTPlayerKindCom;
    cfg->fighters[2].level = 3;
    cfg->fighters[2].team = nSCBattleTeamIDBlue;
    cfg->fighters[2].costume = (u8)ftParamGetCostumeCommonID(nFTKindCaptain, 0);
    cfg->fighters[2].shade = 0;

    cfg->fighters[3].fkind = nFTKindDonkey;
    cfg->fighters[3].pkind = nFTPlayerKindCom;
    cfg->fighters[3].level = 3;
    cfg->fighters[3].team = nSCBattleTeamIDBlue;
    cfg->fighters[3].costume = (u8)ftParamGetCostumeCommonID(nFTKindDonkey, 0);
    cfg->fighters[3].shade = 0;
#endif
#if NDS_R2_SOAK_MATCH_MINUTES
    /* THE FREEZE SOAK'S LONG MATCH, on its own flag, off by default.
     *
     * The owner's rule this satisfies: *"if you want to run a longer soak for
     * any reason, then you also need to change the match timer to match the
     * soak time"*. Without it a long soak spends one minute in gameplay and the
     * rest watching a Results screen, so a 7-minute run reads NO-FREEZE having
     * exercised less play than the 3.5-minute run that caught the original
     * heap-exhaustion hang. Two runs on 2026-08-02 were wasted exactly that way.
     *
     * soak-freeze-watch.ps1 computes this from its own -MinutesToRun rather
     * than hardcoding a constant, so the match can no longer be shorter than
     * the run that watches it. It is deliberately NOT tied to NDS_R2_BOTH_CPU:
     * the soak runs single-CPU too (-BothCpu:$false), and a gate arm and a soak
     * arm must be able to differ in match length without differing in anything
     * else. Both gate arms build with this at 0 and get the 60-second match. */
    cfg->time_limit = NDS_R2_SOAK_MATCH_MINUTES;
#endif
#else
    cfg->game_rules = SCBATTLE_GAMERULE_STOCK;
    cfg->time_limit = SCBATTLE_TIMELIMIT_INFINITE;
    cfg->stocks = 8;
#endif
}

#if NDS_P2_1P_GAME
/* decomp sc/sctypes.h:33-43 verbatim layout: the per-port 1P setup the ladder
 * seeds (copy_kind at sc1pgame.c:1219) and the included creation loop
 * (sc1pgame.c:2159) and wave hook (:1399) copy to FTDesc.copy_kind. Declared
 * here as a layout mirror -- not a second owner -- so the 1P-only apply block
 * below can commit the descriptor's copy_kind exactly where the source writes
 * it, without this VS-owned TU including the 1P import. VS builds
 * (NDS_P2_1P_GAME=0) never declare the symbol, so they cannot reference it. */
typedef struct NdsSC1PGamePlayerSetupMirror {
    s32 mapobj_kind;
    void *figatree;
    s32 copy_kind;
    s32 team_order;
    sb32 is_skip_entry;
    sb32 is_magnify_ignore;
    u8 cp_trait;
    u8 _pad[3];
    f32 camera_frame_mul;
} NdsSC1PGamePlayerSetupMirror;
extern NdsSC1PGamePlayerSetupMirror
    sSC1PGamePlayerSetups[GMCOMMON_PLAYERS_MAX];
_Static_assert(sizeof(NdsSC1PGamePlayerSetupMirror) == 32,
               "1P setup mirror must match decomp SC1PGameFighter");
#endif

/* P2-7 item 6. THE ATTRACT BRIDGE, decided: the battle entry accepts the
 * battle state the demo set directly, and this file is why that is safe.
 *
 * scAutoDemoInitDemo (scautodemo.c:546-579) copies dSCManagerDefaultBattleState
 * into its own sSCAutoDemoBattleState and points gSCManagerBattleState at it
 * -- game_type Demo, cycling dSCAutoDemoGroundOrder stage, four level-9 COM
 * fighters, starting damage 0-30 for the title's two picks and 40-100 past
 * them -- and scExplainSetBattleState (scexplain.c:151-169) does the same for
 * its own static (game_type Explain, nGRKindExplain, Mario + Luigi as
 * GameKey). Neither touches gSCManagerTransferBattleState, and neither kind
 * dispatches through scVSBattleStartScene (scvsbattle.c:513-515), the one
 * seam that re-points gSCManagerBattleState at the transfer block -- so this
 * apply step, the VS shell's only writer of that block, simply never runs on
 * the demo path and there is nothing to seed through NdsMatchConfig. Seeding
 * a descriptor up front would duplicate InitDemo's own shuffle, stage cycle
 * and damage rolls (a second implementation of source logic); letting the
 * demo's func_start own its state keeps one. Return trips need nothing here
 * either: the tap exits write nSCKindTitle and the focus/phase ends write
 * nSCKindStartup / nSCKindCharacters, all honoured by the seam pump. */
void ndsMatchConfigApply(const NdsMatchConfig *cfg)
{
    s32 i;
    s32 human = -1;
    u8 pl_count = 0;
    u8 cp_count = 0;
    /* BattleShip mnplayersvs.c:4417 derives this from the time-rule bit. */
    ub8 is_single_stockicon =
        (cfg->game_rules & SCBATTLE_GAMERULE_TIME) ? TRUE : FALSE;
    /* 1P ally color (sc1pgame.c:1074) is the human's port, found here so the
     * slot loop below stays a single pass. -1 is impossible on a 1P
     * descriptor (the bridge always seats one Man); the clamp keeps a
     * hand-built one from producing 0xFF. */
    if (cfg->game_type == (u8)nSCBattleGameType1PGame)
    {
        for (i = 0; i < NDS_MATCH_FIGHTERS_MAX; i++)
        {
            if (cfg->fighters[i].pkind == (u8)nFTPlayerKindMan)
            {
                human = i;
                break;
            }
        }
        if (human < 0)
        {
            human = 0;
        }
    }

    dSCManagerDefaultSceneData.gkind = cfg->gkind;
    gSCManagerSceneData.gkind = cfg->gkind;

    gSCManagerTransferBattleState = dSCManagerDefaultBattleState;

    /* 1P only (sc1pgame.c:2901). The VS preset leaves the sentinel, so the
     * base copy -- and the trailing snapshot below -- keep yesterday's
     * byte-identical VS behaviour. */
    if (cfg->game_type != NDS_MATCH_NO_GAME_TYPE)
    {
        gSCManagerTransferBattleState.game_type = cfg->game_type;
    }
    /* 1P only: the descriptor owns the ladder index (sc1pgame.c:979-980), so
     * apply publishes it beside the gkind siblings above. VS leaves the
     * sentinel and this never runs. */
    if (cfg->spgame_stage != NDS_MATCH_NO_SPGAME_STAGE)
    {
        dSCManagerDefaultSceneData.spgame_stage = cfg->spgame_stage;
        gSCManagerSceneData.spgame_stage = cfg->spgame_stage;
    }
    gSCManagerTransferBattleState.game_rules = cfg->game_rules;
    gSCManagerTransferBattleState.time_limit = cfg->time_limit;
    gSCManagerTransferBattleState.stocks = cfg->stocks;
    gSCManagerTransferBattleState.handicap = cfg->handicap_mode;
    gSCManagerTransferBattleState.is_team_battle = cfg->is_team_battle;
    gSCManagerTransferBattleState.is_team_attack = cfg->is_team_attack;
    gSCManagerTransferBattleState.is_stage_select = cfg->is_stage_select;
    gSCManagerTransferBattleState.is_reset_players = cfg->is_reset_players;
    gSCManagerTransferBattleState.item_toggles = cfg->item_toggles;
    gSCManagerTransferBattleState.item_appearance_rate =
        cfg->item_appearance_rate;
    gSCManagerTransferBattleState.damage_ratio = cfg->damage_ratio;

    for (i = 0; i < NDS_MATCH_FIGHTERS_MAX; i++)
    {
        const NdsMatchFighterConfig *slot = &cfg->fighters[i];
        SCPlayerData *player = &gSCManagerTransferBattleState.players[i];

        /* BattleShip mnplayersvs.c:4388-4423. `player` is the slot in FFA and
         * the team in Team Battle.  The source only rewrites `team` in the team
         * branch, and only rewrites the field that is meaningful for the
         * selected player kind: CPU level for COM, handicap otherwise.  Keeping
         * those branches matters on a reused transfer block -- writing both
         * fields here made the descriptor a subtly different commit contract. */
        if (cfg->is_team_battle == FALSE)
        {
            player->player = (u8)i;
        }
        else
        {
            player->player = slot->team;
            player->team = slot->team;
        }
        player->fkind = slot->fkind;
        player->pkind = slot->pkind;
        player->costume = slot->costume;
        player->shade = slot->shade;
        if (slot->pkind == nFTPlayerKindMan)
        {
            player->color = (cfg->is_team_battle == FALSE) ? (u8)i :
                dIFCommonPlayerTeamColorIDs[slot->team];
        }
        else if (cfg->is_team_battle == FALSE)
        {
            player->color = GMCOMMON_PLAYERS_MAX;
        }
        else
        {
            player->color = dIFCommonPlayerTeamColorIDs[slot->team];
        }
        player->tag = (slot->pkind == nFTPlayerKindMan) ?
            (u8)i : (u8)GMCOMMON_PLAYERS_MAX;
        player->is_single_stockicon = is_single_stockicon;
        if (slot->pkind == nFTPlayerKindCom)
        {
            player->level = slot->level;
        }
        else
        {
            player->handicap = slot->handicap;
        }

        /* 1P only: the ladder seeds what the VS CSS never touches, exactly
         * where the siblings above land. Enemy single-stock plus the enemy
         * flag (sc1pgame.c:967-968), ally single-stock without it (:1077-1078),
         * player stock plus cleared flag (sc1pmanager.c:286-287; challenger
         * single-stock at :518) all arrive in the descriptor slots. Color/tag
         * need the same treatment for the two slots the generic path gets
         * wrong: the player (source colors by port, sc1pmanager.c:282-283, not
         * by team) and allies (color = player port at :1074, tag = Heart at
         * :1075, where generic Com handling would give CP). Enemies need no
         * override: team color table [Com] is 4 = nSCBattlePlayerColorCP
         * (:964), Com tag is 4 = nIFPlayerTagKindCP (:965), and the TIME-bit
         * icon is TRUE (:966). The player icon is FALSE on common stages and
         * TRUE on challenger ones (:1034/:1039). */
        if (cfg->game_type == (u8)nSCBattleGameType1PGame)
        {
            player->stock_count = slot->stock_count;
            player->is_spgame_enemy = slot->is_spgame_enemy;
#if NDS_P2_1P_GAME
            /* 1P only: the opening Kirby copy power, exactly where the source
             * writes it (sSC1PGamePlayerSetups[player].copy_kind,
             * sc1pgame.c:1219). The included creation loop (:2159) and wave
             * hook (:1399) carry it to FTDesc.copy_kind (fttypes.h:559), which
             * ftManagerMakeFighter publishes as the Kirby copy_id
             * (ftmanager.c:615). VS slots carry NDS_MATCH_NO_COPY_KIND and
             * this never runs, so the VS setups stand byte-identical. */
            if (slot->copy_kind != NDS_MATCH_NO_COPY_KIND)
            {
                sSC1PGamePlayerSetups[i].copy_kind = slot->copy_kind;
            }
#endif
            if (slot->pkind == nFTPlayerKindMan)
            {
                player->color = (u8)i;
                player->tag = (u8)i;
                player->is_single_stockicon =
                    (cfg->spgame_stage > (u8)nSC1PGameStageCommonEnd) ? TRUE :
                    FALSE;
            }
            else if (slot->is_spgame_enemy == FALSE)
            {
                player->color = (u8)human;
                player->tag = (u8)nIFPlayerTagKindHeart;
            }
        }

        /* Do not seed player->stock_count here on the VS path. The CSS source
         * does not touch it. scVSBattleStartBattle passes the match-wide
         * `stocks` value in the FTDesc for each occupied slot, and
         * ftManagerMakeFighter publishes that value to
         * gSCManagerBattleState->players[player].stock_count
         * (ftmanager.c:702). Sudden Death likewise owns its explicit zero.
         * The 1P branch above is the deliberate exception: there the source
         * seeds every slot before creation (sc1pgame.c:967,1077;
         * sc1pmanager.c:286,518). */

        /* Derived, as the source derives it (mnplayersvs.c:4425-4439): the
         * counts are a census of the slots, not an independent setting. */
        if (slot->pkind == nFTPlayerKindMan)
        {
            pl_count++;
        }
        else if (slot->pkind == nFTPlayerKindCom)
        {
            cp_count++;
        }
    }

    gSCManagerTransferBattleState.pl_count = pl_count;
    gSCManagerTransferBattleState.cp_count = cp_count;

    dSCManagerDefaultBattleState = gSCManagerTransferBattleState;
}

/* decomp mn/mnvsmode/mnvsitemswitch.c:39-57. Its index 0 is the appearance
 * rate, not a kind, so the fifteen kinds below are its entries 1..15 and the
 * row order is the screen's own. Red Shell is deliberately absent: it has no
 * row and rides on Green Shell's. */
const u8 kNdsItemSwitchToggleKinds[NDS_ITEM_SWITCH_TOGGLE_COUNT] = {
    nITKindSword,   nITKindBat,     nITKindHammer,  nITKindHarisen,
    nITKindMSBomb,  nITKindBombHei, nITKindNBumper, nITKindGShell,
    nITKindMBall,   nITKindLGun,    nITKindFFlower, nITKindStarRod,
    nITKindTomato,  nITKindHeart,   nITKindStar
};

u32 ndsMatchConfigItemTogglesFromRows(const u8 *statuses)
{
    u32 toggles = 0u;
    u32 i;

    if (statuses == NULL)
    {
        return 0u;
    }
    /* mnVSItemSwitchCheckAllTogglesOff (:632) runs FIRST and short-circuits
     * the whole thing: with every row off the mask is zero, containers
     * included. Building the mask and then clearing it would be the same
     * answer here but not the same function, and the containers below are
     * exactly what a later reader would expect to survive. */
    for (i = 0u; i < NDS_ITEM_SWITCH_TOGGLE_COUNT; i++)
    {
        if (statuses[i] != 0u)
        {
            break;
        }
    }
    if (i == NDS_ITEM_SWITCH_TOGGLE_COUNT)
    {
        return 0u;
    }

    for (i = 0u; i < NDS_ITEM_SWITCH_TOGGLE_COUNT; i++)
    {
        u32 kind = kNdsItemSwitchToggleKinds[i];

        if (statuses[i] != 0u)
        {
            toggles |= (1u << kind);
            if (kind == (u32)nITKindGShell)
            {
                toggles |= (1u << nITKindRShell);
            }
        }
    }
    /* :655. The containers are not optional while anything is on -- they are
     * what carries the payload the other rows selected. */
    toggles |= ((1u << nITKindEgg) | (1u << nITKindCapsule) |
                (1u << nITKindTaru) | (1u << nITKindBox));
    return toggles;
}

void ndsMatchConfigItemRowsFromToggles(u32 toggles, u8 *statuses)
{
    u32 i;

    if (statuses == NULL)
    {
        return;
    }
    for (i = 0u; i < NDS_ITEM_SWITCH_TOGGLE_COUNT; i++)
    {
        statuses[i] = ((toggles & (1u << kNdsItemSwitchToggleKinds[i])) != 0u) ?
            1u : 0u;
    }
}
