# R2-07 stress/soak acceptance battery — 2026-08-13, HEAD `3cd70d9b745`

Executes SwitchPlan §7's stress gate and §6 acceptance 4 on the both-CPU arm.
Every figure below carries its window. **A number without its window is not a
result** — the two instruments here measure different things and one of them
(the in-ROM pacing block) resets on a seam that is not the match boundary.

## ROM identity

| | |
|---|---|
| target | `smash64ds-battle-playable-tickhud-hwtri` (the measuring sibling; `NDS_TICK_HUD 1`, `NDS_TICK_HUD_DRAW 1`) |
| 1-minute arm | `builds/build-c132-stress` — `NDS_R2_BOTH_CPU 1`, `NDS_R2_SOAK_MATCH_MINUTES 0` |
| 5-minute arm | `builds/build-c132-stress5` — `NDS_R2_BOTH_CPU 1`, `NDS_R2_SOAK_MATCH_MINUTES 5` |
| boot headroom | 176,128 bytes proven (`boot-headroom-c132-stress.txt`), `fake_heap_start 0x02269804` |
| emulator | repo-local `emulators/melonds/melonDS.exe`, SHA-256 `DE80E46B…5715`, DLDI ON |
| root ROMs | unchanged this cycle — `smash64ds.nds` `54C07FAC…A68A`, `smash64ds-battle-playable-hwtri.nds` `524448C9…ADEE` |

## Verdicts

| # | battery item | verdict |
|---|---|---|
| 1 | restart chaining + Sudden Death, both-CPU | **PASS** — 3 completed matches in one session (run 2), 5 battle entries, Sudden Death played to a KO in each run |
| 2 | risk counters (SweepFail / EpochBump / AObj / heap) | **PASS** |
| 3 | visual/state anomaly sweep | **PASS**, with one instrument defect found and fixed |
| 4 | demo-loop gate readings (1-minute and 5-minute) | **5-minute RAN at 98.7% coverage — `WORK-H` P95 1,205,760, i.e. a five-minute match costs what a one-minute match costs.** The 1-minute arm was NOT re-measured on this build |
| 5 | Boundary coverage | **cycle 10's green stands — no source changed** |

---

## Run 1 — chain, cadence-driven presses (`chain-soak.{log,json}`)

`soak-freeze-watch.ps1 -Build build-c132-stress -MinutesToRun 10 -PollSeconds 5
-IdenticalFramesToTrip 16 -PressStartSeconds 165 -PressStartCount 3
-PressStartEverySeconds 140`, 600 s of wall clock, filmstrip every 5 s
(`artifacts/visibility/2026-08-13_c-stress/chain-frames/`, 121 PNGs).

**Verdict `NO-FREEZE`. 2 completed matches, 3 battle-scene entries, and one
Sudden Death played to a KO.** The chain is structurally sound; the run reached
only 2 matches because 264 of its 600 seconds were spent PAUSED — see the
instrument defect below, which is this run's most valuable finding.

### The scene ledger, from counters that count entries rather than pixels

| counter | value | reading |
|---|---:|---|
| `gNdsSCVSBattlePlacementInitCount` | **3** | three battle-scene entries |
| `gNdsRendererSceneTextureVramResetCount` | **3** | the scene-owned texture-VRAM reset ran on every one |
| `gNdsRendererBattleStaticTexturePrepareCount` | **3** | ditto for the pinned static set |
| `gNdsSCVSBattleSuddenDeathPrepareCount` | **1** | `scVSBattleStartSuddenDeath` ran once (`battleship_scvsbattle.c:342-348`) |
| `gNdsVSResultsStartCount` | **2** | two Results screens, i.e. two completed matches |
| `gNdsVSResultsRematchCount` | **1** | one START-driven restart fired |
| `gNdsRelocSceneReentryEvictCount` | **2** | both same-kind re-entries were caught stale and evicted |
| `gNdsSCVSBattleLifecycleArenaAdapterCount` | **3** | three `scManagerFuncUpdate` battle runs |
| `sIFCommonBattlePlace` | **0** | the GAME SET announcement fired |
| `gNdsRendererBattleStaticTextureViolationCount` | **0** | no cache discarded while still marked prepared |

Sequence, reconstructed from the filmstrip and the clock/damage readings the HUD
prints: match 1 (KO, `FOX WINS`) → Results 1 → START → match 2 → time-up tie →
**Sudden Death** → KO → GAME SET → Results 2 (up when the run ended).

### Sudden Death, unforced

No force switch was needed. Two level-3 CPUs tied at time-up and the source
selected Sudden Death itself. `f00559s-4E4D4677.png` is the proof frame:
**`GAME SET` over Dream Land, `CPU L3 [MARIO] DMG 312% STOCK --`,
`CPU L3 [FOX] DMG 300% STOCK x1`, `TIME 01:00`** — the 300% start is SD's own
rule and no ordinary match begins there, and Mario's exhausted stock is the KO
that ended it. Its own presented cadence, from the pacing block that resets at
that battle entry: **218 presented frames, 2 VBlanks × 186, 3 × 19, 4 × 0,
5+ × 13, max 20, `CadenceViolationCount` 0.**

### Risk counters (item 2) — all clean across three entries

| counter | value | required | verdict |
|---|---:|---|---|
| `gNdsR2TexProofSweepFailCount` | **0** | 0 in every match | **PASS** — slice 50's certificate survived two rematch entries and a Sudden Death |
| `gNdsR2TextureEpochBumpCount` | **168** | non-zero across a scene entry | **PASS** |
| `gNdsR2TexProofFastCount` / `SweepCount` | 132,566 / 10,008 | fast path dominant | **PASS** — the win is still being taken |
| `sNdsAObjEvent32NormalizedCount` | **245** of 1,024 | must not climb to the cap | **PASS — the cliff is not real on a chain** |
| `gNdsAObjEvent32NormalizeFailCount` | **0** | 0 | **PASS** — no reason-12 overflow reject |
| `gNdsTaskmanGeneralHeapFreeMin` | **67,652** | ≥ 32,768 | **PASS** (also clear of the 25,600 GObj cap) |
| `sGCCommonsMaxNum` | **-1** | never latched | **PASS** |
| `gNdsTaskmanArenaAllocFailCount` / `gNdsSyMallocOverflowCount` | 0 / 0 | 0 | **PASS** |
| `gNdsRelocHeapDeclineCount` | 0 | 0 | **PASS** |
| `gNdsObjmanPanicCount` / `gNdsObjAnimRunawayCount` | 0 / 0 | 0 | **PASS** — both freeze guards silent |
| `gNdsR2AnimCacheArenaOverflows` / `Rejects` / `RangeFaults` | 0 / 0 / 0 | 0 | **PASS** (`GenerationMismatches` 4 = the ordinary second-entry drop) |

**HANDOFF's latent AObj cliff is HALF closed, and the other half got worse.**
`sNdsAObjEvent32Normalized` is cleared by `ndsAObjEvent32ResetNormalizedScripts()`,
called from `ndsRelocResetLoadedFiles()` (`src/port/reloc_backend_assets.c:2093`),
which every unload/scene-cache-eviction path funnels through. So the table is
**per-scene, not per-run**: three battle entries plus two Results scenes read
**245**, five entries read **297**, and the overflow branch never ran. **A
restart chain cannot fill it.** But item 4's five-minute match reads **1,019 of
1,024 inside a single scene** — see there. The cliff is a MATCH-LENGTH cliff,
not a chain one, and it is five slots from the edge.

### Results-screen cadence, same run

`gNdsVSResultsPresentIntervalBucket[]` over **5,423** samples: 2 VBlanks × 4,712,
3 × 1, 4 × 7, 5 × 3, 6 × 3, 15+ × 1 (696 samples below 2 VBlanks are not
bucketed). **P95 = 2 VBlanks**, i.e. the Results screen holds 30 Hz;
`gNdsVSResultsPresentIntervalMax` 25,736 is the first interval after the scene
change measured against a stale last-present stamp, not a hitch.

### The instrument defect this run found (and the fix)

**`soak-freeze-watch.ps1` could not see a stopped game.** It hashed the whole
client area, and on the measuring ROM the bottom screen is the tick HUD, whose
digits change on every presented frame. Every START press that lands during a
match PAUSES it (the port implements START as pause), so the 3-press bursts left
the match paused on odd parity.

Measured, per-poll, top screen versus bottom screen
(`chain-filmstrip-motion.csv`, `filmstrip-motion.py`):

| window | top-screen change | bottom-screen change | state |
|---|---:|---:|---|
| t+25…96 | 0.75–0.99 | 0.02–0.13 | match 1 |
| t+101…162 | 0.07–0.11 | **0.000** | Results 1 (HUD blanked) |
| **t+190…322** | **0.000** | 0.010–0.021 | **PAUSED, 132 s** |
| t+328…339 | 0.64–0.96 | 0.02–0.04 | running |
| **t+344…476** | **0.000** | 0.011–0.019 | **PAUSED, 132 s** |
| t+482…564 | 0.79–0.99 | 0.02–0.07 | match 2 → Sudden Death → GAME SET |
| t+569…604 | 0.05–0.13 | **0.000** | Results 2 |

Two fixes landed, both structural rather than advisory:

1. `Get-MelonDSWindowFrameHash -Half Top|Bottom|All` (lib). The soak's freeze
   verdict now hashes the **top half only** — the game picture, which is what a
   player calls frozen. A paused or hung game can no longer hide behind the HUD.
2. `soak-freeze-watch.ps1 -PressStartOnResults`. The press is now **detected,
   not timed**: two consecutive polls whose bottom band hashes identically mean
   the HUD is blanked, i.e. the battle scene is not up, and only then is START
   pressed (with a cooldown longer than a poll). Verified offline against run
   1's own frames before spending a run, on the exact band the detector uses:

   | state | top band changed | bottom band changed |
   |---|---:|---:|
   | Results | 12.39% | **0.0000%** |
   | battle | 89.65% | 2.49% |
   | paused | **0.0000%** | 1.76% |

   **The band is 45%, not 50%, and that mattered.** The client area is the menu
   bar plus two screens, so a 50/50 split leaves ~14 rows of the *top* screen
   inside the bottom region — enough for the animating Results screen to keep
   the "HUD" hash moving. The first corrected run sat through **152 s of
   Results without pressing once**; discarding the middle 10% fixed it and made
   the split independent of menu height and DPI scaling.

`-SaveFramesTo` was added in the same pass: the watch was already capturing a
frame every poll to hash it and was throwing the pixels away, so a FROZEN
verdict could not show the frame it froze on.

---

## Run 2 — chain, DETECTED presses (`chain2-soak.{log,json}`)

Same ROM and same 5 s filmstrip, `-MinutesToRun 11 -PressStartSeconds 60
-PressStartOnResults`. **Verdict `NO-FREEZE`. `gNdsVSResultsStartCount` 3 —
three completed successive matches in one emulator session — with
`gNdsVSResultsRematchCount` 3, five battle-scene entries, and one more Sudden
Death.** This is the item-1 acceptance run; run 1 is kept because it carries the
pause finding and its own clean counter set.

| counter | run 1 (3 entries) | run 2 (5 entries) |
|---|---:|---:|
| `gNdsVSResultsStartCount` (completed matches) | 2 | **3** |
| `gNdsVSResultsRematchCount` | 1 | **3** |
| `gNdsSCVSBattlePlacementInitCount` / `SceneTextureVramResetCount` | 3 / 3 | **5 / 5** |
| `gNdsSCVSBattleSuddenDeathPrepareCount` | 1 | 1 |
| `gNdsR2TexProofSweepFailCount` | **0** | **0** |
| `gNdsR2TextureEpochBumpCount` | 168 | 241 |
| `sNdsAObjEvent32NormalizedCount` / `NormalizeFailCount` | 245 / **0** | 297 / **0** |
| `gNdsTaskmanGeneralHeapFreeMin` | 67,652 | **70,384** |
| `AllocFailCount` / `SyMallocOverflowCount` / `RelocHeapDeclineCount` | 0 / 0 / 0 | 0 / 0 / 0 |
| `sGCCommonsMaxNum` (GObj cap) | −1 | −1 |
| `gNdsObjmanPanicCount` | 0 | 0 |
| **`gNdsObjAnimRunawayCount`** | **0** | **17** ← see below |
| `gNdsGcRunAllTapLostCount` | 0 | 0 |

### ANOMALY — 17 DObj animation-script runaway faults on the deeper chain

`gNdsObjAnimRunawayCount` **17**, `Mask` **1** (bit 0 = the **DObj** parser),
`Script` **0x023C138A**, `Opcode` **100**. Run 1, which reached three battle
entries, read **0**; run 2 reached five and read 17.

**The guard contained it** — this is the bounded parser the 2026-08-02/03 freeze
work added, so the fault was recorded and the walk abandoned instead of spinning
in an unbounded loop, and the run finished `NO-FREEZE` with every allocator
counter clean. **It is not attributed, and it is not this cycle's to fix.** Two
candidate mechanisms worth splitting first, in cost order:

1. a script reference that survives a scene boundary into the 4th/5th entry —
   SwitchPlan §3.12's own failure class, and the reason to check it against
   entry count rather than against elapsed time;
2. an artefact of run 2's stray START presses (see run 1's finding — presses
   that land mid-match pause the game), which run 1 did not have in the same
   places.

The discriminator is cheap and needs no build: re-run the detected-press chain to
five entries with `-PressStartOnResults` and no other input, and read the same
four counters per entry. If the count tracks entries, it is (1).

---

## Item 3 — visual/state anomaly sweep

**No flashes, no corruption, no missing textures, no stuck geometry, no hang.**

*Automated, over every frame of the run rather than over a chosen few.*
`filmstrip-motion.py` also counts distinct colours in the game picture, which is
the cheapest corruption floor there is: a frame that lost its textures, palette
or geometry collapses toward a handful of colours. Over the **114 post-boot
frames** of run 1 — two matches, one Sudden Death, two Results screens, three
scene loads and two GAME SETs — the floor is **1,317 colours** (median 2,047,
max 5,408); over run 2's **123 post-boot frames** across five battle entries it
is **1,305** (median 2,496, max 4,660). The only degenerate frames in either run
are the 3-colour boot screens before t+25. Nothing in either chain ever
approached a flat frame, in any scene, at any entry depth.

*By eye, at the transitions that matter* (frames named in
`artifacts/visibility/2026-08-13_c-stress/chain-frames/`):

The five frames named below are copied out of the filmstrip under
`artifacts/visibility/2026-08-13_c-stress/chain1-*.png` and tracked; the full
121-frame filmstrips stay on disk beside them (`chain-frames/`,
`chain2-frames/`) and are summarised by the tracked motion CSVs.

| moment | frame | what it shows |
|---|---|---|
| match 1, mid-match | `f00035s` | full Dream Land — tree, three platforms, pond, flower beds, fence; both fighters; HUD live |
| Results 1 | `f00162s` | `FOX WINS`, both fighters on the podium, KOs/TKO/Pts/Place panels, HUD blanked |
| match 2, first frames | `f00266s` | fresh 1-minute clock (`TIME 00:59`), both damages back to 0%, stage complete, HUD live |
| match 2, late | `f00508s` | `TIME 00:32`, damages 96%/53%, stage complete at a wide camera |
| Sudden Death → GAME SET | `f00559s` | `GAME SET` over Dream Land, Mario 312% stock exhausted, Fox 300% |

**A cross-match pixel comparison is undefined here and was not attempted, and
that is a property of the ROM, not a gap in the sweep** — stated as such rather
than substituted with a number that would not mean what it looks like.
Match 2 is a different fight from match 1 — the level-3 CPUs share one
`syUtilsRandFloat` stream that keeps running — so the two matches never occupy
the same state at the same clock. What re-entry corruption would actually look
like is covered by the counters instead, and every one of them is clean:
`SceneTextureVramResetCount` 3 (once per entry), `BattleStaticTextureViolationCount`
0, `TexProofSweepFailCount` 0, `StagePrepareReuseCount` 14,255 against
`BuildCount` 6, `Task36ReplayArenaStaleCount` 0,
`RendererProfileTextureRejectReasonMask` 0.

---

## Item 4 — the demo-loop gate readings

### The five-minute acceptance match — RAN, and it does not degrade

`sample-tick-hud-buckets.ps1 -Build build-c132-stress5 -RingDump -Samples 8448
-StartFrame 438`, DLDI ON, `NDS_TICK_HUD_DRAW 1`, both-CPU, one match, **frames
439–8887, `slips=0`**. `fivemin.{json,-rows.csv}`, `fivemin-run.log`.

**Coverage is measured, not assumed:** the guest reports
`gSCManagerTransferBattleState.time_limit` **5** and
`gNdsBattlePlayablePacingLogicFrames` **17,772** of the match's 18,000 —
**98.7% of a five-minute match**, against 86.7% for the standard 1,600-sample
one-minute window. This is the widest gate window this project has taken.

| bucket | P50 | P95 | mean | 1-minute reference (slice 50, `build-c131-cand`) |
|---|---:|---:|---:|---:|
| **`WORK-H`** | **929,344** | **1,205,760** | 965,908 | 923,392 / **1,210,880** |
| `ALL` | 1,118,272 | 1,678,720 | 1,221,216 | 1,118,208 / 1,678,720 |
| `FTR` | 298,304 | 325,632 | 292,560 | 298,048 / 324,032 |
| `STG` | 161,728 | 165,760 | 162,522 | 161,600 / 165,888 |
| `SRC` | 329,856 | 579,008 | 360,263 | — |
| `WAIT` | 209,472 | 481,856 | 221,441 | 211,136 (P50) |

**VBlank interval histogram: 2 × 7,415, 3 × 1,394, 4 × 58, 5+ × 19, max 26**,
over 8,886 presented frames. 83.4% of presented frames hold the 2-VBlank (30 Hz)
cadence and 99.1% are within 3.

**The finding is the comparison, not the number.** A five-minute match costs
what a one-minute match costs: `WORK-H` P95 **1,205,760** against the one-minute
arm's **1,210,880** — a 5,120 difference across *different builds*, well inside
the ±14,080 cross-build P95 floor `docs/VERIFYING.md` calibrates, and P50 differs
by 5,952 against a ~5,700 cross-build P50 floor. **Match length does not
accumulate cost**: no leak, no growing arena, no degrading cadence over five
times the duration. The 3-VBlank share does rise (16.6% and 1,394 frames against
the one-minute arm's 267 of 2,038 = 13.1%), which is the only length-dependent
signal in the run.

**The gate itself still fails, exactly as the board already records.**
1,205,760 is **85,760 over** the 1.12M budget; this cycle measured the stress
configuration, it did not close the gap, and nothing here changes the standing
lane.

### The one-minute arm was NOT re-measured on this cycle's build

The reference column above is slice 50's banked figure from
`builds/build-c131-cand`, which is **not** this HEAD: `src/nds/nds_renderer.c`
was edited at 12:58 after that build linked at 12:42. It is quoted as a
reference and must not be re-banked from here. A fresh 1,600-sample run on
`build-c132-stress` is the cheapest missing piece of this battery (~7 minutes,
no build).

### Two counters that only the long match could produce

| counter | 1-min chains | **5-min match** | reading |
|---|---:|---:|---|
| `sNdsAObjEvent32NormalizedCount` | 245 / 297 | **1,019 of 1,024** | **five slots from the cap, inside ONE scene** |
| `gNdsAObjEvent32NormalizeFailCount` | 0 | **0** | the overflow branch has still never run |
| `gNdsObjAnimRunawayCount` | 0 (3 entries) / 17 (5 entries) | **50** | **and this run had NO input at all** |

**The AObj cliff is real for long matches.** The table only resets on a scene
teardown, so a five-minute match fills it monotonically to 1,019 — and the
overflow path does not fail loudly, it rejects the script (reason 12) and
**silently skips the animation attach**. Nothing in the P1 milestone runs longer
than a minute, so this is not a P1 blocker; a demo loop at five minutes is five
scripts away from it.

**The runaway anomaly is NOT an artefact of the stray START presses.** This run
drove no input whatsoever and still recorded **50** DObj-parser runaways, more
than the 17 that run 2's five entries produced. Whatever it is, it scales with
time-in-scene, and the two candidate mechanisms in run 2's section are now down
to one: something inside the battle scene, not the chain and not the input.

---

## Item 5 — Boundary coverage

**No source file changed this cycle** — the diff is three `.ps1` harnesses, two
docs and the artifacts below (`git diff --stat`). The ROM under test is HEAD's
own build, so **cycle 10's Boundary green stands as current**:
`artifacts/performance/2026-08-13_c-threeleg/boundary.log`, run against
`builds/build-battle-playable-proof-hwtri-harness` built from this same HEAD,
0 `Exception:`, 0 `FAIL`, 0 `RED`. Re-running it here would re-test an identical
ROM hash, which `docs/VERIFYING.md` explicitly says not to do.

Both root ROMs are byte-identical to their cycle-start values, before and after
this cycle's two lab builds:
`smash64ds.nds` `54C07FAC…A68A`, `smash64ds-battle-playable-hwtri.nds`
`524448C9…ADEE`.
