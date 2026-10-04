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
texgen, fast-lane routes, off-screen culls, rigid stage bindings: `p2/P2-6-one-player.md`); 10-03 VS venues pin
static bindings (mean -7..-20K; arch. log). **10-02 VS regression fixed (`319c15bd292`):** 1P growth had shrunk the
arena 93 KB -- VS CSS hid every preview, VS Results froze at tic 120. A duplicate FAT mount (65,832 B) is gone;
CSS reserve 230,112 free (needs 183,072), Results podium fits on three rosters. Re-probe both after any static or
NitroFS growth (scratchpad `resprobe.ps1`). Pikachu self-hit burst toward camera (`53e26f7b7ee`); Hitmonlee draws (`59390301261`). **10-02 owner playtests r67-r71** — every row with its cause and commit in `p2/P2-6-one-player.md` (r67, r69/r70, r71 tables): latest are the bonus select revisit freeze, Kirby's board map, the bonus timer, COMPLETE!/FAILURE letters, no-Z particle passes (MH/Fox VFX), the ending room 3.6 -> 2.47 VBlanks and its fill-sink failures. Then Fox's board animates (event32 plans in the ledger tail, `79792bd6fb6`) and SFX keep off the BGM pair (`30973a4b778`; the port calls Calico's soundInit). Giant DK libc fault fixed (`0d0a754568c`); collision sweeps reject by per-line spans (`3543009dcac`, Race 47 -> 54%; off-screen items skip their draw, 58%); BTP platform solves memoized per loop (`137aef3dd8d`: Yoshi 0 -> 88%, Pikachu 4 -> 89%, Luigi 48/91%) + board pins (`8e6821aeed0`, Fox 49%) + texture memo/yakumono cuts (Fox 73%, Samus 93%). Saffron's gate draws; the full 1P walk has 0 native failures (`eb040d98fa8`). Battle entries hold the leaving frame through the load (bank C: 1P intro 174 held/5 black); 1P battle exits hold it to the tally, its photo (bank D, `2b853e8a60f`). Purin board 71%. Open: credits text.
**10-02 final P95 pass, 1P first** (natural play, share within 2 VBlanks; table and levers in `p2/P2-6-one-player.md`): Link/Fox/Targets/Pikachu/Samus/Metal 97-99%; Master Hand 94.4% (packets replay + wallpaper fixed cubic, `5ce05818f39`); Kirby Team 94.4%; Board the Platforms 91.3% (two-way texture memo, `e30b6e3747c`); Yoshi Team 84-91%; Giant DK 84-89%; Polygon 88%; Mario Bros 81-86%; Race 40-52%. Profiles `artifacts/task37-census/1p-pf21s*` (scratchpad `cf95.py` ranks P95 drops).
**10-03..04 owner r75 rows:** every reachable particle packs (`d4acc472432`: P1 cuts gone, Kirby/Yoshi banks; the build fails on an unseated texture); countdown lamps keep their shading, GO is area-sampled (`f929d9451c7`, evidence `e3f69e90f99`); the 1P walk reaches any portrait (`6c3f644d8df`); the Continue screen draws the fallen fighter in its spotlight and VS Results survives ties and the 4P podium (`979102bf1f9`: IDO frame order hid an out-of-range read in `mnVSResultsGetSpot`; Results display lists take measured bounds). Open: Sector Z's Arwing draws seven roots a frame -- the body ~25K ticks and six two-triangle glows ~13-18K each, matrix prep 5-10K of every glow (`artifacts/task37-census/sz-*`); Fox's intermittent Sector Z stage failures. **10-04 later:** the Fire Flower's head draws (`51789f8726c`: it inherits the stem's unlit two-sided geometry mode; the executors re-seed the display mode per root); Sector Z's Arwing draws in two passes (`0135f6b6578`: matrices first, then submits; Arwing frames WORK -52K mean, p95 1.576M -> 1.534M, pixel-identical); the respawn halo draws through the near plane on the split-matrix route (`cd27927037b`, the Fox Sector Z failures). r76 playtest built (Results/Continue/VFX/countdown), r77 adds these. **10-04 night:** 12-fighter campaign sweep (walk-all6, scratchpad `cwall.ps1` + `congrawatch.ps1`): every fighter reaches Congra with 0 native failures (heap low-water >= 54,180) except Fox's Sector Z dust, which declined at the near plane beside the 1P camera (`6c14ffb250d`, BUG_NOTES V1; replayed walk 0 failures, 72 clipped triangles). Yoshi loses Master Hand to the level-9 walk CPU; his ending was walked with MH's damage poked (0 failures). r78 = r77 + the dust clip. Sector Z's Arwing: lab profile `sz-szprof02` puts its premium at ~201K cycles a frame, 43K of it the 15 KB executor reached once per root; a replay-only submit for FoxSpecial3 roots (`ndsRendererReplayNativeEntryEffectFox`) takes Arwing frames' MiscActor 116K -> 75K, WORK p50 1.170M -> 1.126M, p95 1.552M -> 1.521M (some of it reappears as GX wait); every GX input hashes identical to the executor's on 411 frames (`artifacts/performance/2026-10-04_arwing-replay`). Window captures cannot pair A/B frames on this emulator (variable UI lag); compare GX-input hashes keyed by logic frame instead. Item/effect draws cost 22-50K ticks per list, CPU-bound (Castle 4P census: PIM 7-21K, baked submit ~23K; memory `item-draws-cost-25-45k-each-cpu-bound`). Gate-scenario profile `sz-gateprof01` (lab window P50 888K, P95 1.159M): P95 drivers are the lean kernel 59K, Dream Land's stage GX draw 49K (stage path ~107K a frame in all), fadd 40K, pose parse/play 32K/30K. **10-04 P95 pass:** gate at HEAD 960,064/1,324,672 (384 over; `artifacts/performance/2026-10-04_gate-head`): 344 of the over-gate frames follow frame 800, when items pile up, and carry SRC +222K, MISC +139K and FTR +69K over sub-1M frames (STG flat). Stage segments commit in one pass and one DMA (`fa35dd61607`: STG P50 -16K, P95 1,316,224, 1,597/1,961 in two VBlanks) and the stage prepare reuses the frame camera (`cbbe041ca76`). A held item's attach cannot be skipped at draw time (it writes latches the simulation reads). The stage DL fast lane (routed items, the Charge Shot, baked roots, visual effects) runs on the DTCM hot stack (`debe30ee031`: gate P50 946,304 -> 938,368, over 355 -> 330, 1,623/1,961 in two VBlanks; static reach 5,728 B of 6,144); the late-window lab profile is `artifacts/task37-census/sz-lateprof01` (over-gate premium: hurtbox rejects, soft float, pose parses, effects; the Charge Shot frames are combat frames). Then the NDL dispatch and entry effects joined the hot stack, which fits because the DS modelview stack is one slot (G_MTX push is host-reference only; `NDSRendererTraversalState` 3,000 -> 888 B; `2ae07aeb07a`), the camera loop dropped a dead five-recogniser census and pre-tests stage segments with a pointer bloom (`0ac832f3bd7`), and the collision readiness check is inline (`e89136078b8`): gate 925,888/1,271,488, 288 over, 1,666/1,961 in two VBlanks. Rejected (receipts): inline matrix copies in the stage fast pass (STG +8.7K) and an inline `ftGetStruct` fast path (+3.8K P50, I-cache). The wall sweep's fast path took its slow path's ITCM with a census section D pack (`09dc70e015f`: gate 920,704/1,259,712, 280 over, 1,678/1,961 in two VBlanks; receipt `2026-10-04_itcm-pack4`). Rejected: Q15 hurtbox locals with DTCM sines (0 flips, same-ROM P95 -1K: noise; receipt `2026-10-04_hurtbox-q15`); a head-order fast pass for DLLink stages (Sector Z's segments interleave display heads, so `ndsStageGxDraw` keeps its ITCM). After frame 800 the gate draws only Beam Swords (one held by Link), ~33K ticks a lying sword and ~62K a held one; item draws now record their emits once and replay them under fresh matrices (`cacd4fd246f`, Sword route; the held item's latch walk still runs: gate 906,432/1,247,232, 241 over, 1,714/1,961 in two VBlanks; receipt `2026-10-04_item-replay`).

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
