#ifndef NDS_MATCH_CONFIG_H
#define NDS_MATCH_CONFIG_H

/* The match descriptor -- P2-1a (docs/p2/P2-1-vs-shell.md work item 1).
 *
 * ONE struct describes a VS match: who fights, where, under which rules. It is
 * the only input the battle takes. Before this seam the canonical match was
 * spelled out as straight-line writes into `gSCManagerTransferBattleState`
 * inside `src/port/scene_harness.c`, so the configuration and the act of
 * installing it were the same code and there was nothing for a menu to fill in.
 * Now the two are separate: a *preset* fills the descriptor, and
 * `ndsMatchConfigApply` installs it. P2-1e's character-select screen replaces
 * the preset and nothing else.
 *
 * MEANING MIRRORED FROM BATTLESHIP, STRUCTURE NOT (PROJECT_GOAL.md's rule).
 * The original carries the same state in two halves: `sMNPlayersVSSlots[]` --
 * per-slot fkind/pkind/cpu_level/handicap/team/costume/shade, written by the
 * character-select screen -- plus the rules the VS-mode screens own
 * (game_rules/time_limit/stocks/is_team_battle, item switch, stage). At READY
 * the original commits both halves into `gSCManagerTransferBattleState`
 * (`mn/mnplayers/mnplayersvs.c:4379 mnPlayersVSSetSceneData`), and `sc/` copies
 * that into the live battle (`sc/sccommon/scvsbattle.c:240,515`). This
 * descriptor is those two halves in one struct, and `ndsMatchConfigApply` is
 * the commit step.
 *
 * WHAT IS *NOT* IN HERE, ON PURPOSE. Every field below is one the canonical
 * configuration actually sets. Anything the original derives at commit time is
 * derived by `ndsMatchConfigApply` instead of being stored twice: the slot's
 * `player` identity, `is_single_stockicon` (a restatement of the Time rule), and
 * `pl_count` / `cp_count` (a census of `pkind`). Per-player `stock_count` is
 * later battle-owned state: scVSBattle puts the match-wide `stocks` value in
 * each fighter descriptor and ftManagerMakeFighter publishes it, exactly as the
 * source does. Two representations of one fact drift.
 */

#include <ssb_types.h>
#include <sc/scene.h>

/* BattleShip's VS engine is four slots wide, and P2-2 now carries that same
 * bound through the DS match/menu/renderer bridges. GMCOMMON_PLAYERS_MAX is the
 * engine's own bound, so the descriptor and source battle can never disagree. */
#define NDS_MATCH_FIGHTERS_MAX GMCOMMON_PLAYERS_MAX

/* One slot. An empty slot is `pkind == nFTPlayerKindNot` (normally paired with
 * `nFTKindNull`). The commit still copies the source CSS selection fields for
 * every slot; VSBattle is what skips Not slots when it creates fighters. */
typedef struct NdsMatchFighterConfig {
    u8 fkind;    /* character: nFTKind*, nFTKindNull when the slot is empty */
    u8 pkind;    /* nFTPlayerKindMan / nFTPlayerKindCom / nFTPlayerKindNot */
    u8 level;    /* CPU level 1..9; ignored for a human slot */
    u8 handicap; /* per-slot handicap; ignored for a CPU slot, as in the source */
    u8 team;
    u8 costume;
    u8 shade;
    u8 color;
} NdsMatchFighterConfig;

typedef struct NdsMatchConfig {
    NdsMatchFighterConfig fighters[NDS_MATCH_FIGHTERS_MAX];
    u8 gkind;                /* stage: nGRKind* */
    u8 game_rules;           /* SCBATTLE_GAMERULE_TIME or _STOCK */
    u8 time_limit;           /* minutes; SCBATTLE_TIMELIMIT_INFINITE for none */
    u8 stocks;
    u8 handicap_mode;        /* nSCBattleHandicapOff / On / Auto */
    u8 item_appearance_rate; /* nSCBattleItemSwitch* */
    ub8 is_team_battle;
    ub8 is_team_attack;
    ub8 is_stage_select;
    ub8 is_reset_players;
    u32 item_toggles;        /* item switch mask; 0 = every item off */
} NdsMatchConfig;

/* The descriptor the battle is currently running. Menus fill it (P2-1e); until
 * then the preset below does. */
extern NdsMatchConfig gNdsMatchConfig;

/* PRESET: the canonical VS demo match -- Mario versus a level-3 CPU Fox on
 * Dream Land, one-minute Time match, items off. This is what harness mode 163
 * used to spell out inline, and it is the only place the build-time match flags
 * (`NDS_DEV_LIVE_INPUT_PREVIEW`, `NDS_DEMO_FOX_CPU_LADDER`, `NDS_R2_BOTH_CPU`,
 * `NDS_R2_SOAK_MATCH_MINUTES`) are read. Keeping them here leaves the apply
 * step free of build configuration, so exactly one function decides what the
 * match is. */
void ndsMatchConfigLoadMarioFoxDreamLand(NdsMatchConfig *cfg);

/* Install the descriptor into the engine's battle state and scene data. The
 * only writer of the canonical match configuration. */
void ndsMatchConfigApply(const NdsMatchConfig *cfg);

/* The Item Switch screen's fifteen toggleable kinds, in the source's own screen
 * order (decomp mn/mnvsmode/mnvsitemswitch.c:39-57, whose index 0 is the
 * appearance rate rather than a kind, so this array starts at its index 1).
 * A screen shows one row per entry; the order is the row order. */
#define NDS_ITEM_SWITCH_TOGGLE_COUNT 15u
extern const u8 kNdsItemSwitchToggleKinds[NDS_ITEM_SWITCH_TOGGLE_COUNT];

/* Turn fifteen on/off rows into the item_toggles mask the engine reads, exactly
 * as mnVSItemSwitchSetItemToggles (:647) and mnVSItemSwitchSetItemSettings
 * (:589) do together. Three rules that are easy to lose and all gameplay:
 * every row off means NO items at all, not "only the containers"; Green Shell
 * carries Red Shell, which has no row of its own; and while any row is on the
 * four containers are forced on whether or not the player wants them.
 *
 * Split out from any screen because the rule is the game's, not the menu's --
 * a preset or a test can ask the same question without drawing anything.
 * `statuses` is NDS_ITEM_SWITCH_TOGGLE_COUNT entries, nonzero meaning on. */
u32 ndsMatchConfigItemTogglesFromRows(const u8 *statuses);

/* The inverse, for a screen opening on the current settings
 * (mnVSItemSwitchGetItemSettings, :572): fill `statuses` from a mask. */
void ndsMatchConfigItemRowsFromToggles(u32 toggles, u8 *statuses);

#endif
