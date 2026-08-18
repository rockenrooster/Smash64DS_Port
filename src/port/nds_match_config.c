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
#include <nds/nds_match_config.h>
#include <nds/nds_scene_harness.h>
#include <sc/scene.h>
#include "nds_build_config.h"

NdsMatchConfig gNdsMatchConfig;

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

        /* `player` is the slot index while teams are off; the team-battle form
         * (source: mnplayersvs.c:4396) belongs to P2-2, which is when teams
         * become selectable. */
        player->player = (u8)i;
        player->fkind = slot->fkind;
        player->pkind = slot->pkind;
        player->level = slot->level;
        player->handicap = slot->handicap;
        player->team = slot->team;
        player->costume = slot->costume;
        player->shade = slot->shade;
        player->color = slot->color;
        player->is_single_stockicon = is_single_stockicon;
        /* Derived, never stored twice: an occupied slot starts on the match's
         * stock count, an empty one on nothing. */
        player->stock_count =
            (s8)((slot->pkind != nFTPlayerKindNot) ? cfg->stocks : 0);

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
