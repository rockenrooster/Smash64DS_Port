# Handoff

P2 follows `PROJECT_GOAL.md` and `P2_PLAN.md`; `P2_EXECUTION_BOARD.md` owns focus, decisions, artifacts and the
**Execution cursor**. This file is a route, not another task or metric ledger.

## Current route (2026-09-26)

P2-2p8 four-fighter 30 FPS runs on `p2/FOUR_FIGHTER_30FPS_ARCHITECTURE.md`
(rulings D1-D9 in its section 8, phase log in section 6). On resume read its
sections 0, 6 and 8, then the board cursor. Evidence:
`artifacts/performance/2026-09-2*_p2-2p8-*`; `scripts/compare-replay-digest.py`
is the gameplay-equivalence check every phase runs.

Phase 1 (fighters): slices 1-7 through `98ebd1e2e51` (slice 7 IMPLEMENTED_NOT_ACCEPTED;
receipt `artifacts/performance/2026-09-24_p2-2p8-phase1-slice7/README.md`). Root is still r54.
**Owner 09-27: VS Mode first, 1P deferred. Any 4 fighters on any stage hit the P95 gate
with items on over a full 1-minute match (plus sudden death); CSS and SSS 100%; Results
and transitions/loading seamless. CSS/SSS audio delay + in-match SFX glitches logged (BUG_NOTES A1/A2).**
**Phase 2/3:** all-VS stage compilation, NDL, MISC owners, view-space particles, A8 BGM (receipts
`2026-09-26_p2-2p8-phase{2,3}-*`). Global renderer retirement remains debt. Bank measured wins (D9).
**09-26..30 solo** (receipts `2026-09-{26..30}_p2-2p8-*`). **09-30 owner checkpoint: optimization
paused "not 100% complete"; the 1P campaign is next.** Gate WORK P50/P95 922,240/1,240,256 (target P95
1.12M, RED; `94ea559f062`; 10-02 re-bank 910,144/1,234,240, 88.8% in 2 VBlanks); lab P95 1.21-1.58M by roster/stage; SZ owner roster 1.37M. 09-30 landed: baked
native owners (`1fe468cc3bb`), transition hold (owner r64, `VERIFIED-hold.md`), world-cache GO retry. Playtest
r65b. Open: owner's rematch VFX loss (not reproduced on 3 stages/2 rosters; ask roster, stage, which VFX),
Link's 3 entry frames on SZ/Saffron, lean remats (DK 13/match), Results photo (tic 0-80 black: the wipe model on link 32 has no native route or program; a display-capture photo + route prototype is parked in `artifacts/bugs/2026-10-02_results-slow/results-photo-wipe-wip.patch`, it needs the 10 wipe models' native programs).
**10-01 1P campaign:** the walk ROM plays 0-13, bonuses, Master Hand, Ending, Staffroll, Congra with 0 battle
failures (table `p2/P2-6-one-player.md`). Landed: intro stills/poses, staff roll, Master Hand, Race/board lights, HUD
anchors, bonus map colours (RSP lighting baked, `390d513c5cb`), Ending room + figure (NitroFS room table), platform
bake. Owner playtest 10-01 (Kirby): 15 of 24 bonus boards drew no map (3-colour TLUT, `98be04a7540`); Polygon Team
froze on a copy hat the match never admitted (`1c1c67b90f4`). Owner floor for content work: P50 < 1.12M
(>50% of frames within 2 VBlanks). 10-02: every stage 80-98%, the Race 50.2% (GX
texgen, fast-lane routes, off-screen culls, rigid stage bindings: `p2/P2-6-one-player.md`); VS venues still ship
rigid mask 0 (pin needs a VS heap check). **10-02 VS regression fixed (`319c15bd292`):** 1P growth had shrunk the
arena 93 KB -- VS CSS hid every preview, VS Results froze at tic 120. A duplicate FAT mount (65,832 B) is gone;
CSS reserve 230,112 free (needs 183,072), Results podium fits on three rosters. Re-probe both after any static or
NitroFS growth (scratchpad `resprobe.ps1`). Pikachu self-hit burst toward camera (`53e26f7b7ee`); Hitmonlee draws (`59390301261`). **10-02 owner playtests r67-r71** — every row with its cause and commit in `p2/P2-6-one-player.md` (r67, r69/r70, r71 tables): latest are the bonus select revisit freeze, Kirby's board map, the bonus timer, COMPLETE!/FAILURE letters, no-Z particle passes (MH/Fox VFX), the ending room 3.6 -> 2.47 VBlanks and its fill-sink failures; MH intro is source-exact. Then Fox's board animates (event32 plans in the ledger tail, `79792bd6fb6`) and SFX keep off the BGM pair with Calico auto-update (`30973a4b778`; Calico's soundInit is now called by the port). the walk's Giant DK libc fault fixed (`0d0a754568c`); collision sweeps reject by per-line spans (`3543009dcac`, Race 47 -> 54%); BTP platform solves memoized per loop (`137aef3dd8d`: Yoshi 0 -> 88%, Pikachu 4 -> 89%, Luigi 48/91%) + board pins (`8e6821aeed0`, Fox 49%) + texture memo/yakumono cuts (Fox 73%, Samus 93%). Saffron's gate draws; the full 1P walk has 0 native failures (`eb040d98fa8`). Battle entries hold the leaving frame through the load (bank-C snapshot: 1P intro 174 held/5 black). Open: Purin board 33%, credits text, 1P battle exit black.
**10-02 final P95 pass, 1P first** (natural play, share within 2 VBlanks; table and levers in `p2/P2-6-one-player.md`): Link/Fox/Targets/Pikachu/Samus/Metal 97-99%; Master Hand 94.4% (packets replay + wallpaper fixed cubic, `5ce05818f39`); Kirby Team 94.4%; Board the Platforms 91.3% (two-way texture memo, `e30b6e3747c`); Yoshi Team 84-91%; Giant DK 84-89%; Polygon 88%; Mario Bros 81-86%; Race 40-52%. Profiles `artifacts/task37-census/1p-pf21s*` (scratchpad `cf95.py` ranks P95 drops).

## Continue, do not restart

With an intact context, execute the cursor's next unfinished action. Do not
repeat startup, capability discovery, task selection, profiling or completed
verification merely because the goal is repeated or a turn ended.

After context loss, read the cursor and its linked batch receipt once. Reconcile
only relevant source/asset/config changes and an owned live job before relying
on that state. Missing identity blocks reuse of that proof, not unrelated safe
work. A settled experiment needs a named new invalidator to reopen.

## Owners

`VERIFYING.md` owns command lifecycle, test validity and publication. For P2-2p8,
`p2/native-optimization/13_AGENT_EXECUTION.md` defines cursor phases and fields.
Read only contracts needed by the next action. The board chooses the next batch
only after recording an outcome, a concrete blocker or an owner priority change.

Record unavailable helpers and denied Git operations with their retry condition. Preserve owner
edits and qualified artifacts; a doc change does not repair a failure or establish a game PASS.
Bug-sweep lessons (2026-09-19..22): `p2/BUG_NOTES.md` "Standing lessons".
