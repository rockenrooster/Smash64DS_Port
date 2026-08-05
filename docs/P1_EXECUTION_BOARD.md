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

## The diagnosis the lane is built on (settled 2026-08-04/05)

- **Effect DObj submits are the tail**: 99.3% of the `MISC` excursion,
  359,717 ticks/frame on over-gate frames, **0** on clean ones. Net recoverable
  ~315,000 (part displaces `FTR`).
- **The cost is a per-list constant** (~102,730 Exec ticks/list, 1,360
  lists/match, 16.1 tris/list): exact nine-phase partition — generic DL
  interpreter 65.57% (77,440/list), texture resolve 21.41% (25,289/list),
  Matrix 6.93% (8,179/list), everything else ~5.8%.
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

### G1 — Flip the resolver's site cache on for mode 9 (live, next step)

`sNdsRendererStageTextureSites` (`nds_renderer.c:11086`): a 128-entry memo
keyed on `state->source_command_site` (exact `Gfx` command address — a better
key than the refuted memo), already built, 12 KB already spent.
`ndsRendererProfileSetOwner` (`nds_renderer.c:29241-29247`) enables it only for
fast-run modes 4/7/8; every measured ROM builds mode **9** (added after both
lists, neither updated). One line, **zero new RAM**.

Protocol: confirm the static read at runtime first (liveness probe on the
existing tickhud ROM); its own arm and A/B; owner's visual gate only if a
visible pixel of shield/revival platform/impact wave/reflector changes.
Target: part of the 21.41% `Tex` share (~25,289/list) on effect frames.

### G2 — RAM headroom before any new code lands

The boot cliff blocks every candidate that adds text or data (it is what
actually killed the Tex memo arm). Produce the authoritative footprint map:
rank `.text`/`.data`/`.bss` and fixed pools from the map file; identify which
boot-time allocation fails at +2,208 (the failing arm dies before frame 9, so
it is a boot/scene-entry peak, not steady state); free or defer the cheapest
candidates. **Exit: ≥32 KB static headroom demonstrated by the same
`fake_heap_start` probe**, so G3's builder text plus arena bookkeeping fit with
margin. No performance claim — this row is measured in bytes, not ticks.

### G3 — The effect packet path (the 65.57% + 6.93% shares)

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
Success at iteration scale: packet-route ticks/list ≪ 102,730 (the submit-only
residue should be a few thousand); at gate scale: whole-match P95 moves by
most of the ~315K recoverable in both arms.

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
   figure with its arm; DLDI-on only.
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
   and split builds have confused two comparisons.

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
| Stable 30 FPS, representative P95 <= 1.12M ticks | Red | Gap 343,104; gate lane G1–G4 above |
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
