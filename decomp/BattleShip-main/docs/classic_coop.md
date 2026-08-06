# Classic Mode 2-Player Co-op

Port feature (2026-06-10): a second human can join the entire Classic (1P
Game) run. Gated behind **Settings → Gameplay → "Classic Co-op (Needs
reload)"** — default **ON**, latched once at boot
(`port_classic_coop_latch()` in `port/enhancements/ClassicCoop.cpp`).

## Design

- **CSS**: with the toggle on, 1P Mode → "1P Game" opens the **VS-mode CSS**
  (`nSCKindPlayersVS`) instead of the 1P CSS. A port-side *context flag*
  (`port_classic_coop_context()`) tells the VS CSS overlay it is serving
  Classic mode; it is set only at the `mn1pmode.c` redirect, so the
  BootToVSCSS enhancement never trips it. In classic context the CSS:
  - forces stock rule / no team battle; the stock counter clamps to 1–5;
  - replaces the FFA/Team-Battle header toggle with a **difficulty
    selector** (sprites from `llMNPlayersDifficultyFileID`, appended as
    `dMNPlayersVSFileIDs[7]`);
  - lets a **single** confirmed human start (solo classic run, stock
    behavior); only `pkind == Man` slots count toward ready;
  - B backs out to the 1P Mode menu; confirm jumps straight to
    `nSCKind1PGame` (no stage select) via
    `mnPlayersVSSetSceneDataClassicCoop()`, which writes P1 into
    `gSCManagerSceneData.player/fkind/costume` (mirroring
    `mnPlayers1PGameSetSceneData`) and persists difficulty/stocks to
    `gSCManagerBackupData` + `lbBackupWrite()`.
- **Handoff**: `SCCommonData` grew 4 `#ifdef PORT` tail fields:
  `coop_player2` (controller port, `SCCOMMON_COOP_NO_PLAYER2`/0xFF = no P2),
  `coop_fkind2`, `coop_costume2`, `coop_shade2`. Humans are collected in
  ascending slot order, so `player < coop_player2` always.
  "Co-op active" everywhere is one predicate: `sc1PManagerIsCoopActive()`
  (`sc1pmanager.c`) = latched toggle AND `coop_player2 != 0xFF`.
  The handoff is scrubbed at boot (`scmanager.c`), on every 1P Mode menu
  entry (`mn1PModeInitVars`), and by the stock 1P CSS — stale-state paths
  (training, bonus practice, solo runs) can never see a phantom P2.
- **Slots**: P2 keeps their real controller port as the slot index (input
  binds `&gSYControllerDevices[slot]`); the manager re-points
  `ally_players[0]` to that port, so every "ally slot" path treats P2 as
  the partner. `ally_players[1]` becomes the next non-human port.
  `sc1PGameGetNextFreePlayerPort` skips both human ports in co-op (enemy
  assignment can't clobber a human); the appear/HUD walkers use the new
  `sc1PGameCoopNextActivePort` since actives are no longer consecutive.
- **Per stage** (`sc1PGameSetupStageAll`): P2 is re-activated after the
  blanket slot clear on all common stages **including Master Hand**;
  challenger duels (14–17) stay P1-solo. Stages with authored ally spawn
  points (Mario Bros., Giant DK) spawn P2 there; everywhere else P2 uses
  P1's spawn point +30 X. Mario Bros.: P2 *is* the partner (random CPU
  ally suppressed); Giant DK keeps one random CPU ally (dedup vs. P2's
  character is automatic). Team stages (Yoshi/Polygon) drop from 3 to 2
  simultaneous enemies — the rest arrive via the respawn path, total
  enemy count unchanged. Kirby Team already runs 2.
- **Bonus stages** (`sc1pbonusstage.c`): map selected by **P1's** fkind
  (by design); both players spawn (P2 at +30 X); targets/platforms are
  stage-global so completion is shared. A fallen player stays out
  (`ftcommondead.c` marks `stock_count = -1`, rebirth path sleeps them);
  the attempt only fails when both are out (or time).
- **Lose/continue** (`sc1pmanager.c`): game over only when **both** humans
  have `stock_count == -1` (or time out). A continue revives both.
  The end-of-stage announcement (`sc1PGameSetPlayerDefeatStats`) fires only
  when the second human falls.
- **Polish (2026-06-10)**: CSS slots 3/4 are locked closed in classic
  context (forced `Not`, kind button inert — max two players). On
  shared-spawn stages P2 starts 120 units from P1 (bonus stages keep a
  30-unit offset; narrow start platforms). The 1P intro screen shows P2
  beside P1 on every battle stage by reusing the Mario Bros. two-card
  layout for stages that normally pose P1 alone (one extra fighter
  alloc/figatree heap appended at the last index; Mario/DK show P2
  natively, bonus intros have no fighters). **Friendly fire between the
  partners is OFF by default** — "Co-op Friendly Fire" checkbox under the
  Classic Co-op toggle (live, applies from the next stage); stage setup
  overrides `is_team_attack`, and bonus stages get team-battle wiring
  (both humans team 0, `is_not_teamshadows` kept) so the setting governs
  there too.
- **Score/records**: run score sums both players' KO scores; bonus stats,
  No-Miss/Total falls, hi-score records, and challenger unlock criteria
  stay keyed to P1 (records save under P1's character). The
  protect-your-CPU-ally bonuses (Good/True Friend, DK Defender/Perfect)
  are suppressed in co-op. Camera anchors (Race to the Finish, boss cam)
  stay on P1.

## Files touched

`port/enhancements/ClassicCoop.cpp` (new), `enhancements.h`,
`port/gui/PortMenu.cpp`, `port/port.cpp`,
`decomp/src/sc/sctypes.h`, `decomp/src/sc/scmanager.c`,
`decomp/src/sc/sc1pmode/sc1pmanager.{c,h}`,
`decomp/src/sc/sc1pmode/sc1pgame.c`,
`decomp/src/sc/sc1pmode/sc1pbonusstage.c`,
`decomp/src/mn/mn1pmode/mn1pmode.c`,
`decomp/src/mn/mnplayers/mnplayersvs.c`,
`decomp/src/mn/mnplayers/mnplayers1pgame.c`,
`decomp/src/ft/ftcommon/ftcommondead.c`.

## Debug / testing

Direct-boot a co-op run without touching the CSS:

```bash
SSB64_START_SCENE=52 SSB64_SPGAME_STAGE=<0-13> SSB64_SPGAME_FKIND=<p1 fkind> \
SSB64_COOP_P2=<port 1-3> SSB64_COOP_P2_FKIND=<p2 fkind> ./BattleShip
```

(`SSB64_SPGAME_FKIND` is required on direct boots — without it the intro
scene crashes on a junk fkind; pre-existing artifact, not co-op related.)

Verified headless 2026-06-10 (1200–1800 frames each, zero crashes, no
stale-token storms): stages 0, 1, 3, 4, 6, 11, 13 with P2 on port 1; stages
0, 4, 6 with P2 on port 2 (non-adjacent port → enemy port-skip path);
solo control runs; normal VS CSS boot (extra file in
`dMNPlayersVSFileIDs` did not overflow the 120-entry reloc status buffer);
title/menu boots.

**Still needs hands-on verification (two controllers):** CSS flow in
classic context (difficulty arrows hit zones at x≈47/122, stock clamp,
ready gate with 1 vs 2 humans, B-out, CPU-slot toggles being ignored),
difficulty text pixel positions, in-game feel (HUD spread in bonus stages —
P2 meter at x=200), continue screen revive-both, full run to
ending/credits, and the toggle-off byte-identical regression.
