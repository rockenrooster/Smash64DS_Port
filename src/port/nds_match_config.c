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
 *     descriptor deliberately does not model -- game_type, damage_ratio,
 *     is_show_score, is_not_teamshadows, and each slot's tag/place/score/falls/
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
#include "nds_build_config.h"

NdsMatchConfig gNdsMatchConfig;

#if NDS_P2_PROOF_FIGHTER0 >= 0
_Static_assert(NDS_P2_PROOF_FIGHTER0 < nFTKindPlayableEnd,
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
#endif

#if NDS_P2_FOUR_CPU_ROSTER && (!NDS_P2_LUIGI || !NDS_P2_DONKEY || !NDS_P2_CAPTAIN || !NDS_P2_SAMUS)
#error "NDS_P2_FOUR_CPU_ROSTER=1 needs landed owner admission: NDS_P2_LUIGI=1 NDS_P2_DONKEY=1 NDS_P2_CAPTAIN=1 NDS_P2_SAMUS=1"
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
    cfg->time_limit = 3;
    cfg->stocks = 2;
    cfg->handicap_mode = nSCBattleHandicapOff;
    cfg->item_appearance_rate = nSCBattleItemSwitchMiddle;
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
    cfg->fighters[0].fkind = nFTKindSamus;
    cfg->fighters[0].costume = (u8)ftParamGetCostumeCommonID(nFTKindSamus, 0);
    cfg->fighters[2].fkind = nFTKindCaptain;
    cfg->fighters[2].costume = (u8)ftParamGetCostumeCommonID(nFTKindCaptain, 0);
    cfg->fighters[3].fkind = nFTKindDonkey;
    cfg->fighters[3].costume = (u8)ftParamGetCostumeCommonID(nFTKindDonkey, 0);
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

void ndsMatchConfigApply(const NdsMatchConfig *cfg)
{
    s32 i;
    u8 pl_count = 0;
    u8 cp_count = 0;
    /* BattleShip mnplayersvs.c:4417 derives this from the time-rule bit. */
    ub8 is_single_stockicon =
        (cfg->game_rules & SCBATTLE_GAMERULE_TIME) ? TRUE : FALSE;

    dSCManagerDefaultSceneData.gkind = cfg->gkind;
    gSCManagerSceneData.gkind = cfg->gkind;

    gSCManagerTransferBattleState = dSCManagerDefaultBattleState;

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

        /* Do not seed player->stock_count here. The CSS source does not touch
         * it. scVSBattleStartBattle passes the match-wide `stocks` value in the
         * FTDesc for each occupied slot, and ftManagerMakeFighter publishes that
         * value to gSCManagerBattleState->players[player].stock_count
         * (ftmanager.c:702). Sudden Death likewise owns its explicit zero. */

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
