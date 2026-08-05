# P1 Execution Board

Updated: 2026-08-05 (cycle 79). Boundary: `battle_playable_realtime`, mode `163`.

This board was rewritten from a 10,207-line append log into a queue. Every
verdict, baseline, and instrument note carried forward unchanged; the full
pre-rewrite text is `docs/optimization/archive/P1_EXECUTION_BOARD_pre-cycle79.md`.
Closed work goes to that archive (append a dated section), not back onto this
board. The charter is `docs/Smash64DS_Runtime2_SwitchPlan.md`; measurement and
workflow rules live in `docs/VERIFYING.md`.

## Banked baselines — whole match, the only gate instrument

1,600 samples, frames 440–2040, `dldi=ON`, git `f24f0cc1`, ROM `F04F5D98…`,
`sample-tick-hud-buckets.ps1 -RingDump` (stride 96, ROM byte-identical).
**Never take a gate reading on a 128-frame window** — it reads the cheapest 6%
of the match (P95 understated ~306,000, over-gate rate 5×).

| arm | role | `WORK-H` P50 | P95 | over gate | VBI 2/3/4/5+ (max) |
|---|---|---:|---:|---:|---|
| **both-CPU** `NDS_R2_BOTH_CPU=1` | **THE GATE (owner, 2026-08-05)** | 1,098,240 | **1,605,440** | 704/1600 (44.0%) | 1118/822/90/9 (20) |
| **Boundary** mode 163 | shipped configuration | 1,092,032 | 1,463,104 | 713/1600 (44.6%) | 1161/827/41/10 (20) |

**Gap to the gate: 485,060** (both-CPU 1,605,440 against 1,120,380 = 2
VBlanks); the shipped Boundary configuration trails at 343,104. The owner set
the bar (2026-08-05, confirming the charter §7 stress gate): **the whole match
must sit under the P95 budget on the both-CPU config — the most stressful way
the game is played — loading states excluded** (stated rule: drop frames with
`SRC` > 2× that arm's own `SRC` median). The shipped ROM remains
`smash64ds-battle-playable-hwtri.nds` (Boundary, mode 163). Label every figure
with its arm; never present a both-CPU figure as the Boundary number
(`Makefile:305-308`). Boundary clean-frame P95 ~1,056,640 is ~63K inside the
budget; no both-CPU clean-frame figure is banked yet.

Noise floors: `WORK-H` P95 cross-build ±5,376; per-bucket placement ≥8,544 —
buckets locate, `WORK-H` decides. 1.85 cycles of `FTR` mean per byte of added
ARM text: a change that adds text must beat its own footprint.

## The diagnosis the lane was built on — **BOUNDARY-ONLY, re-priced 2026-08-05**

**Every figure in this section is a Boundary-arm figure.** It was banked without
an arm label, and cycle 79 measured it on the both-CPU arm the owner's gate
actually reads (G2a, commit `62fe823d`): the prize is 4–9× smaller there. Do not
quote these numbers as gate-arm numbers, and do not rebuild G3's case from them.

- **Effect DObj submits are the tail** *(Boundary)*: 99.3% of the `MISC`
  excursion, 359,717 ticks/frame on over-gate frames, **0** on clean ones. Net
  recoverable ~315,000 (part displaces `FTR`).
  **On the both-CPU gate arm: 71.5% of the `MISC` excursion, and `MISC` is only
  16.9% of the WORK-H excursion — so effect submits are ~12.1% of it. Measured
  recoverable 33,699–75,264, not ~315,000.**
- **The cost is a per-list constant** *(Boundary)* (~102,730 Exec ticks/list,
  1,360 lists/match, 16.1 tris/list): exact nine-phase partition — generic DL
  interpreter 65.57% (77,440/list), texture resolve 21.41% (25,289/list),
  Matrix 6.93% (8,179/list), everything else ~5.8%.
  **The constant does not hold on the gate arm: 527–563 lists/match at 83,632
  ticks/list (44,073,856 total, versus Boundary's ~139,714,000) — 41% of the
  lists and 81% of the per-list cost.**
- **The interpreter is honestly generic**: every list terminates at `G_ENDDL`
  (1,360/1,360, none at the 8192 cap), 160.1 commands/list at **626
  ticks/command**. No overrun to fix — **the precompiled-packet path is the
  answer, not a workaround.**
- **Dead, do not re-derive**: projectiles (44 ticks/frame median); particles
  (flat ~47K, a P50 lever, never the gate); texture thrash (1 upload/1,408
  frames — `Tex` is cache-*hit* key/hash/lookup cost); `Find` 0.44%; `Material`
  0.25%; `FTR` as the gate (anti-correlated with the tail); the `Tex`
  (dl-pointer, bind-ordinal) memo (built as approved: 4.56% hit rate, `Tex`
  went *up* 20% — reverted, flag deleted); L7 fixed-point collision (+534 won
  vs 6,481 lost to its own text); asset loads as the tail owner (refuted three
  times); Task 56 strips (ROM hangs the present loop — never completed a run).
- **The ROM is ~1.4–2.2 KB from a boot cliff**: `fake_heap_start` probe —
  +1,408 bytes boots, +2,208 does not; a failing arm never reaches presented
  frame 9. **Text counts as much as bss.** Every new table/code states its byte
  cost and takes an 8-sample `-StartFrame 60` boot probe (~50 s) before any
  measuring run.

## THE GATE LANE — in order, one row live at a time

### G1 — MEASURED (cycle 79). Mechanism proven, gate unmoved. Not shipped.

`sNdsRendererStageTextureSites` (`nds_renderer.c:11086`), enabled in
`ndsRendererProfileSetOwner` (`nds_renderer.c:28819` — the old `29241-29247`
reference was stale by ~450 lines and is corrected here). Mode 9 now joins the
4/7/8 list behind `gNdsG1SiteCacheRoute`; **route 0 is the default and
reproduces shipped behaviour exactly**, so nothing about the shipped ROM
changed.

**The memo works, and the ~175-key working-set fear was wrong.** Whole match,
both-CPU, one binary, both routes (`builds/build-c79-g1-bothcpu`):

| | route 1 (on) | route 0 (off) |
|---|---:|---:|
| `Tex` per list | **7,203** | 20,780 |
| hit rate | **78.06%** (2,305/2,953) | — |
| overwrites / occupancy | **0** / 26 of 128 | — |
| WORK-H P50 | 1,089,024 | 1,095,552 |
| WORK-H P95 | 1,615,872 | 1,612,032 |
| over gate | 682/1600 | 710/1600 |
| `MISC` P95 | 396,096 | 473,536 |

`Tex`/list falls **65.3%** and `Exec` total falls **9,591,232 ticks/match**
(~5,994/frame mean). `MISC` P95 falls 77,440 — 9.1x the ≥8,544 bucket floor.

**But WORK-H P95 moved +3,840, INSIDE the ±5,376 floor: the gate did not
move**, and this closes none of the 485,060 gap. Buckets locate; WORK-H
decides. `ALL` P95 (+128) and the VBI histogram are unchanged, so pacing is
unchanged.

Two findings the next cycle should not re-derive:

- **The both-CPU gate arm exercises this path ~3.5x LESS than Boundary** —
  2,953 consults over 563 lists, against Boundary's 10,336 over 1,360. The
  banked 21.41% / 25,289-per-list `Tex` figure is a Boundary-config number.
  The gate arm is the *worst* case for any effect-texture lever, which is
  worth knowing before G3 is sized against it.
- **The refuted `(dl-pointer, bind-ordinal)` memo's failure does not
  transfer.** It took 471 hits on 10,336 with 7,517 evictions of 7,525 fills;
  this key takes 78% with **zero** evictions and 26 live slots. The site
  address points into static source display-list data and is stable across
  frames; the dl pointer was not.

**Open before this can ship:** the owner's visual gate on shield / revival
platform / impact wave / reflector (not run — see Inherited), and the byte
cost. The measured **+1,924 bytes (text +1,796, bss +128)** is the
*instrumented* build and sits INSIDE the +1,408/+2,208 cliff band; it is
dominated by six census counters inlined at several sites, not by the flip
(one condition). **The shipping cost is unmeasured** — G2 must measure the
flip alone, because G3's packet builder spends from the same budget.

### G2 — footprint map DONE (cycle 79). Failing allocation NOT yet named; 32 KB NOT yet demonstrated.

Authoritative, from the **shipped** ROM's matching ELF pair
(`smash64ds-battle-playable-hwtri.elf`, Aug 4 20:33, pairs with the published
`.nds`). Sections: **text 891,836 / data 147,712 / bss 1,709,640**.

Top `.bss`, which is where the budget actually is (symbol total 1,709,401 of
1,709,640 — so the ranking is essentially complete, not a sample):

| symbol | bytes | share of bss |
|---|---:|---:|
| `gSYFramebufferSets` | 441,600 | 25.8% |
| `sNdsAudioFgmCache` | 204,800 | 12.0% |
| `sNdsRelocSceneFileBuffer` | 185,696 | 10.9% |
| **`sOriginalSpritePreview`** | **153,600** | **9.0%** |
| **`sOriginalSpriteDisplayPreview`** | **153,600** | **9.0%** |
| `gSYZBuffer` | 140,800 | 8.2% |
| `sNdsRendererHardwareTextureScratch` | 32,768 | 1.9% |
| `sNdsRendererTask36ReplayOwner` | 30,880 | 1.8% |
| `sNdsRelocLoadedFiles` | 29,184 | 1.7% |
| `sNdsFighterDLAllDrawStates` | 27,136 | 1.6% |

The top six are **74.9% of all bss**. Top `.text`:
`ndsResetStartupDiagnostics` 33,260, `__dldi_start` 16,384, `categories`
14,328, `ndsRendererHardwareResolveOrBindTexture` 10,944,
`ndsRendererPrepareNativeStageOwner` 10,888, `ndsOpeningRoomRenderDLPreview`
8,756. Top `.data` is `gNdsParticleScriptBank` 10,912 — note `nm` reports
`__sp_usr` at 184,600,960, which is an absolute stack address and **not** a
size; exclude it from any ranking.

**Leading candidate, NOT yet verified removable.** The two sprite preview
buffers total **307,200 bytes (300 KB, 18% of bss)** — nearly 10x the 32 KB
exit on their own, and `sOriginalDLPreview` (13,824) plus
`sOriginalDLDisplayPreview` (7,776) add 21,600 more. All four are declared
**unguarded** in `src/nds/nds_platform.c:114/196/202/205`, so they are
allocated in every configuration including the shipped battle ROM, even if
the battle scene never populates them. `src/port/port_probe.c:53` says the
"original asset previews now own the top-screen visual signal", which is a
**dev preview** role.

**The cheap next step is already wired:** `gNdsOriginalSpritePreviewReady`
(`nds_platform.c:218`) is a published `volatile u32` set at `:617`/`:768`.
Read it in battle on the existing tick-HUD ROM — zero build. If it stays 0
through a match, the battle configuration never populates 300 KB of preview
buffer and the deletion/guarding case is made on measurement rather than on
the name. **Do not delete on the name alone**; `AGENTS.md` requires tracing
unfamiliar assets before removing them, and these have a live dev role.

**Not done this cycle:** the +2,208 failing allocation is **not named** — that
needs a `fake_heap_start` build plus a gdb probe for the `syMallocSet` spin,
and naming it by inference was explicitly out of scope. No headroom freed, no
32 KB demonstrated, no build made for this row.

### G2 (original row) — RAM headroom before any new code lands

The boot cliff blocks every candidate that adds text or data (it is what
actually killed the Tex memo arm). Produce the authoritative footprint map:
rank `.text`/`.data`/`.bss` and fixed pools from the map file; identify which
boot-time allocation fails at +2,208 (the failing arm dies before frame 9, so
it is a boot/scene-entry peak, not steady state); free or defer the cheapest
candidates. **Exit: ≥32 KB static headroom demonstrated by the same
`fake_heap_start` probe**, so G3's builder text plus arena bookkeeping fit with
margin. No performance claim — this row is measured in bytes, not ticks.

### G3 — RE-PRICED ON THE GATE ARM (cycle 79). The prize is 4–9x smaller than this row claims.

**Every number below this heading is Boundary-derived and carries no arm
label. The gate reads on both-CPU, and on that arm they do not hold.**
Measured on `build-c79-g1-bothcpu`, route 0 (shipped), whole match, 1600
samples, frames 441–2040, stride 96, DLDI on:

| | Boundary (banked, unlabelled) | **both-CPU (gate arm)** |
|---|---:|---:|
| effect display lists / match | 1,360 | **527–563** |
| Exec ticks / list | ~102,730 | **83,632** |
| effect submits as share of `MISC` excursion | 99.3% | **71.5%** |
| recoverable on WORK-H P95 | ~315,000 | **33,699 – 75,264** |

The recoverable is a bracket, both ends measured on this arm: 33,699 charging
each ring stop's effect ticks uniformly across its 96 frames, 75,264 charging
all of them to that stop's most expensive frames (concentration-favourable
upper bound). **Removing 100% of effect DObj submits leaves WORK-H P95 at
1,536,768–1,578,333 against a 1,120,380 gate — a residual gap of
416,388–457,953.** G3 cannot close the gate on the arm the gate reads on.

**RETRACTED (cycle 79, same author): "`OTHR` owns 48.3% of the gate-arm
excursion" was wrong.** `OTHR` is not a region's cost, it is an accounting
remainder — `taskman_seam.c:5137` computes it as `ALL - named`, and `named`
does **not** include `WAIT`, so `OTHR` still contains the VBlank idle that
Task 66 later broke out separately. The retracted table ranked `OTHR` while
excluding `WAIT` as untargetable, double-counting the same idle time; their
excursions differed by 0.04%.

The exact identity, verified frame-by-frame with **max error 0** over 1600
frames:

```
WORK-H = (FTR + STG + BG + AUD + SRC + MISC) + (OTHR - WAIT)
```

`OTHR - WAIT`, the true unattributed work, is **flat ~19,159 ticks/frame**
(P50 19,136, P95 19,776, range 17,984-20,352) and contributes **89 ticks** to
the hot-vs-clean excursion. It is a P50 constant and is not a lever in any
form. **`OTHR` needs no further attribution; this closes it.**

**What actually owns the tail — and the two arms are INVERTED.** Mean on
over-gate frames minus mean on clean frames (the metric that separates gate
levers from P50 levers). Owners sum exactly to the WORK-H delta on both arms:

| owner | **both-CPU** (gate) | **Boundary** |
|---|---:|---:|
| **SRC** | **195,361 (69.6%)** | 91,350 (27.8%) |
| **MISC** | 81,675 (29.1%) | **232,263 (70.6%)** |
| AUD | 12,013 (4.3%) | 7,602 (2.3%) |
| STG | 1,933 | 2,146 |
| `OTHR-WAIT` | 89 | 158 |
| FTR | −10,401 | −4,393 |
| WORK-H hot−cold | 280,685 | 329,127 |

**G3's lane was built on Boundary, where `MISC` genuinely is the tail at
70.6%. The gate reads on both-CPU, where `SRC` is the tail at 69.6% and
`MISC` is secondary.** `SRC` is inflated 1.54x at P95 by the stress config
(P95 547,648 → 842,816) but is **not** a config artefact: it is still 27.8%
of Boundary's excursion. FTR is anti-correlated on both arms, independently
reproducing the existing Parked note.

**Levers priced on the gate arm**, counterfactual "bucket never exceeds its
own clean-frame mean" (an **upper bound** per lever — it assumes the entire
hot-frame excess is removable, which for `SRC` it is not, since some excess
is genuine extra AI work):

| | WORK-H P95 | delta | over gate | residual vs 1,120,380 |
|---|---:|---:|---:|---:|
| baseline | 1,612,032 | — | 710 | 491,652 |
| **SRC capped** | 1,217,623 | **394,409** | 235 | 97,243 |
| `MISC` capped | 1,553,792 | 58,240 | 506 | 433,412 |
| **SRC + MISC** | 1,062,592 | **549,440** | 62 | **−57,788** |

The `MISC` figure (58,240) falls inside the independently-derived
33,699–75,264 effect bracket, which cross-validates the method. **`SRC` is
worth 6.8x the `MISC` lever, and the two together put the gate arm inside
budget.**

The combined figure is **super-additive** — 394,409 + 58,240 = 452,649, but
capping both moved P95 by 549,440. That is P95 being a position in a sorted
list rather than a sum. It is not an arithmetic error; do not "correct" it
into an addition.

**Owner decision 2026-08-05: both tracks are in scope. G3 is NOT parked.**
These numbers force it — SRC alone leaves 97,243 over gate, `MISC` alone
leaves 433,412, and only both together land inside. SRC is necessary but not
sufficient. G3's disposition reads **"required, second in order, and primary
for the shipped configuration"** — not "refuted". What was refuted is only
the claim that it alone closes the gate. The second, independent reason to
keep it: **the shipped ROM is Boundary**, and on Boundary `MISC` is 70.6% of
the excursion against SRC's 27.8%, so the packet path is the *dominant*
lever for the configuration that actually ships. Two arms, two different
primary owners, both legitimate. G2's ≥32 KB headroom is therefore back on,
because it funds G3.

**SRC is not a charter §7 question yet and must not be escalated as one.**
`PROJECT_GOAL.md` requires exhausting specialization, approximation,
precomputation, lower-frequency processing, interpolation, event-driven
updates, simplified representations and DS-specific implementations *first*,
and it explicitly encourages fighter- and move-specific native code,
precomputed hitbox trajectories, large lookup tables, and compile-time
baking. `decomp/` is read-only as a **tree**, not as an **algorithm**: a
mechanically equivalent DS-optimized port-side equivalent is wanted, not a
compromise. Rate reduction and simulation-rate change are the LAST resort
and the owner's call.

**All of the above is PROVISIONAL on standing rule 1's window finding** —
these both-CPU shares were computed inside a window now measured to cover
12.6% of that arm's match.

### G3 (original row, Boundary-derived) — the effect packet path

Build the GX packet per unique effect display list **at match load**, reserve
patch offsets for matrix and dynamic colour words, patch per frame, submit.
No re-parse, no per-list config rebuild, no per-command dispatch.

Design constraints (all standing law, see charter §3):
- §3.11 — fixed arena allocated at match load, sized by a unique-list census
  (1,360 list *instances*/match; count the unique templates first), explicit
  overflow policy, exercised in a soak. No gameplay-time heap allocation.
- §3.12 — packets are re-derived at scene entry; nothing keyed on pointers
  that survive a scene boundary.
- Byte-cost table + boot probe before the first measuring run (G2's headroom
  is the budget it spends from).
- Dream Land water frozen at frame 0; same geometry/textures/materials — the
  effect models are a closed owner-approved set. A change that alters a
  visible pixel needs the owner.

**Iteration protocol — one build, one run per decision.** Ship both routes in
ONE tickhud binary behind a gdb-settable runtime route (the
`NDS_R2_STAGE_ROUTE_PROBE` pattern): route 0 = interpreter, route 1 = packets.
Because the cost is a **per-list constant**, ticks/list from a few stops is a
valid iteration metric — flip the route mid-run and read both constants from
the same run, same frames, zero placement noise, zero extra builds. The
whole-match sampling run is reserved for the KEEP decision and re-baseline.
Success at iteration scale: packet-route ticks/list ≪ 83,632 on the gate arm
(≪ 102,730 on Boundary) — the submit-only residue should be a few thousand.

**There is no longer a gate-scale success criterion for this row.** The prior
one ("P95 moves by most of the ~315K recoverable in both arms") was written from
the unlabelled Boundary diagnosis and is refuted: on the gate arm, removing
*100%* of effect submits leaves WORK-H P95 at 1,536,768–1,578,333 against the
1,120,380 budget — a residual gap of 416,388–457,953. **G3 cannot close the gate
alone.** It remains a real Boundary-arm win and a partial gate-arm win; it is no
longer the lane's answer, and G2's ≥32 KB exit exists only to fund it.

### G4 — Re-baseline and pick the next lever from the residue

After G3 KEEP: bank new whole-match baselines (both arms — run them
concurrently on two runner slots once the parked calibration row passes).
**The gate decision reads on the both-CPU arm** (owner, 2026-08-05); bank its
load-frame-excluded P95 explicitly — the Boundary clean-frame figure
(~1,056,640, inside the budget) has no banked both-CPU sibling yet. If a
residual gap remains, promote from Parked in this order: the +52,928
regression bisect (largest known flat cost), `Tex` residue on non-effect
paths, then the charter §7 contingency ladder (rate reduction → fidelity →
owner-approved 30 Hz) — never widen the gate.

## Red Queue

The P1 acceptance-level rows, highest impact first. The gate lane above is
row 1's execution plan.

1. **Stable 30 FPS** — qualify the whole match at P95 ≤ 1.12M ARM9 ticks per
   presented frame on the **both-CPU stress config**, loading states excluded
   (owner, 2026-08-05; gap 485,060; lane G1–G4), on the accuracy melonDS
   fork. The shipped ROM stays the Boundary hwtri configuration. Hardware
   remains the final check for mechanisms the emulator cannot referee.
2. **Mario/Fox completeness** — replace battle-reachable weak status callbacks
   with source-backed behavior and prove both complete movesets naturally.
3. **Dream Land completeness** — close the remaining Whispy material/animation
   presentation debt without reintroducing gameplay-time texture conversion.
4. **Audio completeness** — implement or explicitly qualify every reachable
   voice, pitch schedule, composite cue, and overlapping match-audio path.
5. **Final acceptance** — the CPU-on one-minute match, complete-match capture,
   owner play/listen pass, reserve gate, Results transition, and teardown
   proof on the exact candidate ROM.

## Parked — open items with owners' notes, promote deliberately

- **+52,928 ticks/frame regression** between `2494daf9ad` and `e49a98167c`,
  null control, real, NOT in the three reverted hunks. Untested suspects:
  `38bba475` BLENDPE prim/env bake + `key_generation` fence, `0a060c7b`
  alpha/blend recogniser, `e8c675d3`/`999fcdf8`. Re-open against the
  whole-match instrument only.
- **Concurrency calibration** (workflow, cheap): same tickhud ROM, solo run vs
  two concurrent runs on slots 2/3 — guest tick series should be identical
  (deterministic emulation; host load moves wall clock, not guest ticks). If
  clean, bless 2-concurrent measuring runs (Boundary + both-CPU
  simultaneously) and functional-verify overlap. Watch harness wall-clock
  liveness thresholds (STALLED/TOO SLOW) — they read observed frames/s.
- **`check-decomp-header-mirror.py` RED on HEAD** — `FTSTAT_OPENING1_START`,
  `nSYAudioBGMExplain`; pre-existing; a guard blind to its class of bug.
- **`sNdsRendererRuntimeTextureCacheEvictCount` liveness unproven** — read 0
  all run, never shown able to be non-zero. Do not cite evictions from it.
- **Per-build ELF resolution in harnesses** (`Makefile:60-90` names the fix):
  the root `.elf`/`.nds` pair is shared between build dirs; a published build
  intervening between a lab build and its measurement silently swaps the pair.
  Until fixed: lab build immediately before its measurement.
- **Particle `sqrtf` axis magnitudes** (`ndsParticleTransformForDraw`): move
  two `sqrtf` calls inside the existing `transform_id` guard; ~200K calls a
  match. P50/foreground lever, not the gate. Watch
  `gNdsTickHudForegroundTicks`.
- **Particle atlas admission is stale**: 24 live textures, sheet admits 14 of
  47; texture 1 is in `QUAD_MEASURED_LIVE` and lost its slot. Re-run
  `scripts/generate_nds_particle_banks.py` and re-derive; budget question is
  VRAM cache contention (PORTING.md: 16K/32K sheets failed via
  `PrepareRun` drops), not RAM.
- **GATE 6 price correction on record**: the source-effects flip was sold at
  +36,032 P95 on the bad window; real cost ~360,000 on every effect-active
  frame. The decision stands (make the submit path cheap, do not delete the
  models) — the number behind it did not.
- **`check-one-minute-match-verifier.ps1` has drifted from its owner**
  (2026-08-03): 55 `Assert-Text` pins against exact source text, at least two
  red on refactors that changed nothing they guard. Regrade the pins against
  what each actually protects, or delete the ones already asserted by the
  owner's own gates.

## Standing measurement rules (the ones that gate evidence)

1. Whole-match `-RingDump` sampling is the only gate instrument; label every
   figure with its arm **and its window**; DLDI-on only.
   **"Whole match" is FALSE on the both-CPU gate arm (measured 2026-08-05).**
   `scene_harness.c:221` seeds `time_limit = 7` under `NDS_R2_BOTH_CPU` — a
   420 s match, sized for the freeze soak, never for tick sampling — against
   `:182`'s `time_limit = 1` for Boundary. Both arms sample frames 440–2040.
   Measured with `scripts/probe-match-window.ps1`:

   | | Boundary (163) | both-CPU (gate) |
   |---|---:|---:|
   | configured match | 60 s | **420 s** |
   | clock at frame 440 → 2040 | 52 s → **0 s** | 412 s → 359 s |
   | **fraction of match covered** | **86.7%** | **12.6%** |
   | logic : presented | 2.000 | 2.000 |

   Boundary's window is the last 52 s ending exactly at the buzzer, so its
   label is essentially true and it does **not** spill into Results. The
   both-CPU window is the **first 53 s of a 7-minute match**. The frames-to-
   seconds conversion is 1,600 presented = 3,200 logic = 53.3 s, because the
   sim runs 60 Hz and presents 30 Hz (ratio measured at exactly 2.000, not
   assumed from the VBI histogram).

   **Consequences, all live:** the banked both-CPU P95 is an early-match
   figure and the 485,060 gap derived from it may be optimistic; and the
   SRC/MISC inversion between arms **may be a window artefact rather than a
   config difference**, since Boundary's window includes the KO-heavy endgame
   and the buzzer while both-CPU's covers opening play only. Every both-CPU
   share below (SRC 69.6% / MISC 29.1%) and every lever price derived from
   them is **PROVISIONAL** until re-measured on a comparable window. Shares
   already drift 2.1x (both-CPU `MISC` 104,076–221,815) and 4.2x (Boundary
   `MISC` 94,756–399,021) across 200-frame blocks *inside* the sampled
   window, so stability across the match should not be assumed.

   Fixing this is a window/harness decision for the owner, not a silent
   re-run: either sample the both-CPU arm across its whole 420 s, or seed the
   stress arm at `time_limit = 1` so the two arms are the same match length.
   The soak's own need for 7 minutes is why the value is 7, so the two uses
   now conflict and cannot both be served by one seed.
2. Verify a counter is live in the shipped configuration BEFORE the measuring
   run; a proof-scoped counter reads 0, indistinguishable from clean.
3. Eliminate candidates with a liveness probe on an already-built ROM before
   spending a measuring run.
4. `ALL` is VBlank-quantized; judge on `WORK-H`.
5. Do not multiply a number back by what you divided it by and call the
   agreement a finding.
6. New tables/code: byte cost stated + boot probe before measuring (see G2).
7. Prefer one dual-route binary over separately-linked A/B ROMs wherever the
   change can be routed at runtime; this ROM's pacing is placement-sensitive
   and split builds have confused two comparisons. `sample-tick-hud-buckets.ps1
   -SetGlobals name=value` is the mechanism (cycle 79).
8. **A routed arm must prove the route took before its ticks are read.** A poke
   that silently fails still produces a complete, plausible percentile table,
   which reads exactly like a candidate that engaged and saved nothing —
   `-SetGlobals` did this on its first two runs (see `VERIFYING.md`). Pair
   every `-SetGlobals` with an `-ExtraGlobals` counter that cannot be zero if
   the route engaged.

## Acceptance Matrix

As last graded (cycle 76); a row changes state only when its gate runs.

| Acceptance condition | State | Current evidence / blocker |
|---|---|---|
| Mario human vs original level-3 Fox CPU, Dream Land, one-minute Time, items off | Pass configuration | Boundary registry exposes only canonical mode 163 |
| Original Wait -> countdown -> GO, timer, scoring, Time Up, Results | Focused gates pass | Final exact-ROM CPU-on owner run remains red |
| Mario and Fox complete source-equivalent gameplay behavior | Red | Battle-reachable weak callbacks remain |
| Dream Land collision, platforms, blast zones, wind, camera | Pass for current P1 stage | Dynamic presentation debt remains red separately |
| Recognizable Dream Land presentation and required animation | Red | Whispy material/animation debt; Task 62 candidate rejected |
| Complete overlapping BGM, FGM, voices, announcer, crowd | Red | Exact pitch/composite/voice coverage and listen gates remain |
| Stable 30 FPS, representative P95 <= 1.12M ticks | Red | Gap **485,060 on the both-CPU gate arm** (343,104 is the Boundary figure and is not the gate); lane re-aiming, see the re-priced diagnosis above |
| Stable reserve, no corruption, clean teardown | Focused gates pass | Requalify after the final content/performance candidate |
| Reproducible public artifact | Red | Current local root ROM differs from the pinned public identity |

## Artifact Identity

Pinned public-build identity from `README.md`:

```text
smash64ds-battle-playable-hwtri.nds
11,428,864 bytes
SHA-256 4D795B4E83B335598B20A3B5953FDB1821797CC5E0A825FA96A0643ABBA4A090
```

Current shipping pair (cycle 75), re-verified on disk 2026-08-05:

```text
smash64ds-battle-playable-hwtri.nds   12,129,280 bytes
SHA-256 D16815BEA6A1BA2592B679CA84F747F0A9B9682FF4AE20B9D0A1E22657D47825
smash64ds.nds                         11,790,336 bytes
SHA-256 369FA9993823605A377C0FAC269711A61E7E4773E8066ECB8EAD2F445BD61EF3
tick-HUD sibling (builds/build-c75-tickhud-publish)   12,131,328 bytes
SHA-256 15FD0F8E1467878CC1D65C41ADC895F1102E51DAEE21634937958E1123CCE2CC
```

ROM hashes are not reproducible across rebuilds of identical source; compare
sizes and the build log, not hashes, when attributing a ROM to a tree.

## Lane Ownership

| Surface | Owner |
|---|---|
| Goal, fidelity, milestone, definition of done | `PROJECT_GOAL.md` |
| Dynamic queue, artifact identity, blockers | this file |
| Exact restart surface and next packet | `HANDOFF.md` |
| Verification workflow and measurement law | `VERIFYING.md` + Standing rules above |
| Stable architecture | `ARCHITECTURE.md` |
| Durable unresolved gaps | `KNOWN_ISSUES.md` |
| Measurements and rejected experiments | `PERF_LEDGER.md` |
| Chronological history | `PORTING.md` |

## Integration Rule

Keep only correctness-preserving, verifier-covered progress. Rendering may use
the fidelity budget in `PROJECT_GOAL.md`; gameplay must remain mechanically
equivalent to the original. Run the smallest relevant check, then one widest
relevant verifier for a kept checkpoint.
