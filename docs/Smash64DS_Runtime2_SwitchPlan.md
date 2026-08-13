# Smash64DS Runtime 2 — Plan of Record
Read `PROJECT_GOAL.md`

The smash64ds.nds ROM is not needed for P1, so no need to build it yet.

The single Runtime 2 document: charter, design rules, budgets, phase plan,
and the definition of the switch.

Status: **in execution.** R2-00a/b/c, R2-01, R2-02 gated; R2-05 complete;
R2-03 shipped E12/E28/E29/E46/E32/E64b/E65/E67/E69; R2-06 has Boundary green
and equivalence. **R2-07 is the live phase.** The board
(`docs/P1_EXECUTION_BOARD.md`) is the live queue and carries the banked
baselines, the settled diagnosis, and the gate lane (G1–G4); this file stays
the charter and is not a status log.

**The battle-frame P95 gate is NOT met, and the owner has set the bar at the
both-CPU stress config (2026-08-05, confirming §7's stress gate): the whole
match under P95 ≤ 1.12M, loading states excluded, while the shipped ROM stays
the Boundary hwtri configuration.**

**Current standing (2026-08-12, supersedes the 2026-08-04 figures below).**
Slices 43–48 have since landed and the arm behind the banked "canonical
both-CPU" figure was mislabelled — see
`artifacts/performance/2026-08-12_c126-armcheck/ARM_MISLABEL.md`.
`HANDOFF.md` carries the corrected pair:

| arm | role | `WORK-H` P50 | P95 |
|---|---|---:|---:|
| Boundary (`BOTH_CPU 0`) | shipped config, **PASSES** | 899,136 | 1,087,616 |
| **both-CPU (`BOTH_CPU 1`)** | **R2-07 gate, FAILS** | 938,368 | **1,207,616** |

**Gap to the gate: +87,236**, not the 485,060 recorded below. `SRC` owns 84.4%
of the excursion and its ceiling (207,104) is 2.07x the gap, so the gate is
reachable inside `SRC` alone; the next cut is specified in
`artifacts/performance/2026-08-12_c130-fire-gate/SITR_NEXT_CUT.md`
(`gcPlayDObjAnimJoint`'s AObj pointer chase). Lane table:
`…/2026-08-12_c130-fire-gate/LANES_BOTHCPU.md`.

**Everything from here to the end of this paragraph is the 2026-08-04 era and
is kept only so nobody re-derives it.** Gap to that gate: 485,060 — both-CPU
whole-match `WORK-H` P95 1,605,440; the Boundary arm trails at 1,463,104
(gap 343,104) (2026-08-04, git `f24f0cc1`, dldi=ON; tables on the board). The settled
attribution: **effect DObj submits own the tail** — 99.3% of the `MISC`
excursion, ~360K ticks on over-gate frames, 0 on clean ones, ~315K net
recoverable — and the cost is a **per-list constant** (generic DL
interpretation 65.57%, texture resolve 21.41%, matrix 6.93%). Every list
terminates honestly at `G_ENDDL` at 626 ticks/command, so **the
precompiled-packet path is the answer, not a workaround** (board lane G3).
Clean-frame P95 ~1,056,640 is ~63K inside the gate. Separately, **the ROM is
~1.4–2.2 KB from a RAM boot cliff** (+1,408 bytes boots, +2,208 does not; text
counts as much as bss), so headroom recovery (G2) precedes any candidate that
adds text or data.

Superseded diagnosis eras — E8's "every over-gate frame is an asset-load
frame", L6's "hit-detection frame, 66.2% soft-float", the `FTR` flat-cut
argument, and every 128-frame window figure (the window read the cheapest 6%
of the match: P95 understated ~306,000, over-gate rate five-fold) — are
refuted and archived. History lives in
`docs/optimization/archive/P1_EXECUTION_BOARD_pre-cycle79.md` and this file's
former header in
`docs/optimization/archive/Smash64DS_Runtime2_SwitchPlan_pre-cycle79.md`.
Do not price anything against a pre-2026-08-04 number: earlier baselines were
taken under a GObj cap that no longer fires, on the bad window, or both.

---

## 1. Why Runtime 2

The optimization campaign's conclusion is architectural, not anecdotal:

- Task 65 census: frame work ~1,527K ticks vs the 1,120K target; **62% of the
  frame is stall**, renderer ~52% of work, simulation only ~10%.
- The big generic costs survived dozens of exact tasks: fighter scaffolding,
  stage preflight, texture re-resolution, large temporary structures.
- Task 104's emblem: a 1,292-byte clear plus 1,292-byte copy transported one
  live 4-byte field — ~22K stage ticks and ~28K WORK-H P95 for nothing. The
  cost was cache lines touched, not instructions retired.
- The closed-lever list is long: soft-float frozen (73%), ITCM essentially
  packed, placement closed, wholesale animation rewrite stopped, texture
  memoization dead. Micro-wins are now at or below the noise floor.

The lesson: the current runtime was not designed around a DS CPU, cache, bus,
and memory-traffic budget. Runtime 2 is. Runtime 1 is not wasted — it remains
the behavioral reference, the correctness oracle, the profiling laboratory,
and the source of every proven conversion and harness.

```text
Runtime 1: keep harvesting exact >=20K-30K wins while R2 is built
Runtime 2: prevent the architecture that produced those costs
```

---

## 2. What Runtime 2 actually is: a promotion, not a greenfield rewrite

Runtime 2 is **not** a parallel engine in a new directory. The repo already
contains its embryo, proven and shipping:

| Runtime 2 concept | Already in the tree |
|---|---|
| fighter compiler | `scripts/fighters/generate_nds_native_owners.py` → `src/nds/nds_native_fighter_owner.generated.inc` (dense vertices, baked positions, runs, epochs, per-epoch material/cull policy, FIFO plans — Mario and Fox) |
| stage compiler | `scripts/stages/generate_nds_native_stage.py` → `src/nds/nds_native_stage_owner.generated.inc`; `scripts/stages/dreamland/generate_dreamland_ds_mesh.py` → `src/nds/dreamland_ds_mesh.generated.inc` |
| static-stage direct replay | Task 36/53 rigid capture/replay, shipped default-on (STG −33%) |
| prepared per-run state | `NDSNativeHierarchyPreparedRun` with baked `poly_fmt`/texture bindings |
| measurement suite | tick-HUD ROM, WORK-H buckets, ring dump, Task 49 GX differ, screenshot analysis |

Fighters already draw through generated runs and epochs every frame. What the
campaign measured as remaining architectural cost is the **generic scaffolding
around those generated paths**: `PrepareProductionRun` policy re-verification,
`NDSRendererTraversalState`/`NDSRendererStats` temporaries, per-frame texture
identity proof, the stage `PrepareRun` preflight, and the object-manager
traversal that reaches the owners.

Therefore Runtime 2 is scoped as:

```text
extend the generators downward   (more facts baked at build time)
delete the scaffolding upward    (less discovery at run time)
```

until the generic layer is gone from the battle hot path. Every phase reuses
proven components and produces a shippable intermediate. There is no big-bang
replacement and no long-lived fork.

Gameplay is **kept, not rewritten or bridged**. The imported BattleShip
gameplay code (`src/import`, `src/port`) is source-faithful, verifier-covered,
and ~10% of the frame. Runtime 2 does not touch it; there is no oracle
comparison because it is the same code. The gameplay/visual split happens
entirely on the visual side.

---

## 3. Design rules

These bind every Runtime 2 phase.

### 3.1 Design backwards from the budget

Never: implement subsystem → discover it costs 400K → spend 30 tasks cutting
it. Always: assign budget → design → measure immediately → within budget
continue, over budget **redesign now**. A subsystem that fundamentally misses
its budget is never carried forward on the assumption that optimization will
rescue it.

### 3.2 Two budgets per subsystem

Every hot subsystem carries a CPU-tick budget **and** a memory-traffic budget
(bytes read/written/copied/cleared, cache lines touched, where practical).
With 62% of the frame stalled, instruction counts alone cannot referee a
design. The goal is not only "execute fewer instructions" but "move less data
through the DS memory system."

### 3.3 Memory-traffic priority order

```text
1. Do not store or touch data that is unnecessary.
2. Use the smallest representation that contains the live state.
3. Keep hot data contiguous.
4. Keep access predictable.
5. Only then optimize individual instructions.
```

Before any hot copy or clear, answer: which fields are read afterward, how
many bytes are truly live, can they be carried directly, and can **both**
memory operations be removed instead of one? A traffic optimization is judged
by cache lines that stopped being touched, not instructions that vanished.

### 3.4 Banned hot-path shapes

No blanket `memset()` of large structures; no full-structure copies or
snapshots where a few fields survive; no giant general-purpose structs passed
to consumers that read one field; no hash tables, linked lists, string or
path comparisons, generic scene-graph traversal, generic display-list
interpretation, or dynamic material discovery — **unless a measurement
justifies the exception**. The default: if a fact is immutable for the match,
the compiler bakes it.

### 3.5 Preserve 60 Hz gameplay; render at 30

```text
gameplay mechanics      60 Hz   (unchanged Runtime 1 code)
rendering               30 Hz
visual fighter pose     30 Hz
particles               15-30 Hz
background              15 Hz where practical
lighting                on change / low rate
audio                   event-driven
```

Do not begin by compromising the simulation: 30 Hz gameplay creates
correctness risk across one-frame hitboxes, collision crossings, landing,
shields, grabs, input timing, and CPU behavior — and the evidence says
gameplay is cheap. Make the surrounding machinery cheap enough that 60 Hz
mechanics simply fit. (`PROJECT_GOAL.md`'s sacrifice order still governs a
genuine end-game conflict.)

### 3.6 Separate gameplay state from visual state

The renderer must never require `render skeleton == gameplay skeleton`.
Gameplay keeps the joint data its mechanics read (collision points, hitboxes,
movement state); the renderer consumes a compact generated pose evaluated at
presentation rate. They may share source data; they must not share one
expensive runtime representation.

### 3.7 Data-oriented layout, hot/cold separation

Prefer compact contiguous arrays, fixed sizes, indices over pointers, linear
iteration, small hot working sets. Progression: remove unused state → shrink
representation → pack contiguously → iterate predictably. Contiguity alone is
not the goal — a contiguous 1,292-byte struct with four live bytes is still a
failure. Keep cold fields (descriptors, diagnostics, loader state) out of hot
structs; keep cold code (loading, validation, debug, parsing) off the hot
path. Code placement and traffic both measurably move frame time.

### 3.8 P95 is designed out, not tuned out

No gameplay-time work whose cost depends on first-use loading, asset
discovery, cache construction, relocation, file reads, growth, occasional
rebuilds, or generic fallback paths. Prefer predictable constant work over
"usually cheap, occasionally catastrophic." Match loading is allowed to be
expensive; active gameplay is not. Preload everything the known configuration
implies, subject to RAM limits — and measure peak RAM early.

### 3.9 Measurement discipline

One synchronized 8-frame A/B per decision; report P50/P95 and the 2/3/4/5+
VBlank-interval histogram — never a mean, never min FPS. Third run only when
noisy, near a gate, or surprising. Noise policy: <10K ignore unless free;
10–20K usually too small for architecture work; 20–50K consider if simple and
exact; 50–100K valuable; 100K+ major target. Do not accept a subsystem on P50
if it creates P95 spikes.

**The per-bucket placement floor is ≥8,544, and it is NOT the ±5,376 that
applies to `WORK-H` P95.** Calibrated 2026-08-04 by an arm that only *removed*
code: `FTR` moved **+8,544** while the change touched nothing in `FTR`. Buckets
partly cancel when summed, which is why `WORK-H` is the more stable series — the
same arm moved `WORK-H` median +576. So a per-bucket delta in the 5–8K range is
**not a result**, however clean the story around it, and several historical
per-bucket "wins" in that band cannot be distinguished from placement. Judge a
change on `WORK-H`; use bucket deltas to locate, never to decide.

**Two configurations, two numbers, never merged.** `NDS_R2_BOTH_CPU=1` (Mario
CPU vs Fox CPU, continuous attacks, maximal live hitbox population) is **the
gate** (owner, 2026-08-05): the whole match must sit under the P95 budget on
this, the most stressful way the game is actually played — the default
Boundary understates cost because with no controller attached the human Mario
stands still. `NDS_R2_BOTH_CPU=0` at mode 163 remains the **shipped
configuration** and the milestone's reported Boundary figure;
`Makefile:305-308` still forbids presenting a stress P95 as the Boundary
number. Label every figure with the arm it came from.

**A 128-frame window is a weak instrument for anything transient.** It is ~4.3 s
of a 60 s match at a fixed early offset, so it can miss every KO, respawn and
revival platform — i.e. exactly the content the owner reports as the most
demanding in the game (2026-08-04). It also puts P95 at sorted index 120 with
only seven frames above it. `sample-tick-hud-buckets.ps1` takes repeated ring
dumps as of `58ca8723`; use a whole-match span for any gate reading.

All gate readings are taken **DLDI-on** — the owner's I/O configuration,
pinned in the harness 2026-07-29. DLDI-off is optimistic by roughly 30K P95;
report it only as a secondary, clearly labelled number, never as the gate
figure.

### 3.10 Fidelity floor

Keep the approved full-3D Dream Land presentation and SSB64 visual identity.
Runtime 2 reduces runtime overhead, not stage content; Runtime 1 already
proved large visual reductions were poor trades. The render fidelity doctrine
applies: screenshot diffs plus the owner's visual approval, with the owner as
the visual oracle.

### 3.11 No gameplay-time heap allocation

`syMallocSet` spins forever (`while (TRUE);`) on exhaustion — the BattleShip
allocator cannot fail, only freeze, so any mid-match leak presents as a random
total freeze (the 2026-07-29 freeze class, root-caused on the board).
Therefore: no allocation from the shared heap during gameplay frames. Any
mid-match cache gets a fixed arena, sized and allocated at match load, with an
explicit overflow/eviction policy exercised in a soak. This binds the
particle-bank port and every remaining R2-07 subsystem.

### 3.12 Anything that survives a scene boundary is re-derived, never trusted

The sibling of §3.11, and the most expensive law this campaign has paid for —
**five incidents, five subsystems, one mistake.** The taskman arena is rewound
on every scene entry, so a bump allocator hands the next scene the *same
addresses*. Therefore a pointer, a name, a size, an allocation order, or a
buffer cursor that outlives a scene boundary is not evidence of anything, and
code that trusts one is broken whether or not it currently misbehaves:

```text
key on something the boundary MOVES     heap generation, topology generation/stamp
re-derive what the boundary INVALIDATES texture names, GObj pointers, DL cursors
own the resource's LIFETIME explicitly  reset it at entry; do not reproduce a layout
```

The incidents, all closed, all in `docs/PORTING.md`: the animation cache keyed on
a rewound pointer (freeze); the stage prepared-run cache keyed on the config
pointer (second-entry corruption); texture VRAM with no owner (the same
corruption, one layer down — and note that *reproducing entry one's allocation
order* was not sufficient, only resetting the allocator was); the source
display-list heads never rewound in the battle loop (freeze ~8 s into match
two); and `sMNVSResultsFighterGObjs` trusted across a Results re-entry (ARM9
data abort at match two's GAME SET). A guard that reads
`prepared && prepared_file == file_data` *looks* self-invalidating, which is
exactly why three of these survived review.

Corollary for instruments: a counter that is identical before and after the
window cannot be evidence about the window, and a counter whose writer is
compiled out reads 0 — which is indistinguishable from clean.

---

## 4. Frame budget

`ALL` is VBlank-quantized at 560,190 ticks/VBlank; today the frame is pinned
at 3 VBlanks with excursions to 4–5. The switch's whole purpose in one line:

```text
move the histogram from 3-VBlank frames to 2-VBlank frames
2 VBlanks = 1,120,380 ticks = the P95 gate (PROJECT_GOAL: P95 <= 1.12M)
```

The original provisional targets are retained for history; **this table is no
longer the allocation authority** — see the note below it:

```text
60 Hz gameplay core, two logical ticks       150K
fighter visual pose / animation              100K
fighter rendering (both fighters)            250K
Dream Land                                   180K
background                                    40K
effects + audio                               80K
camera + miscellaneous                        80K
platform / presentation                       80K
headroom                                     160K
                                            -----
total                                      ~1.12M
```

**Budget authority today (2026-07-30).** Measured reality replaced the table:
`FTR` runs ~391,744 P50 against its 250K line and `SRC` ~309,120. Allocate
from these instead:

- **Battle frame:** the gate itself — DLDI-on P95 ≤ 1.12M, with the remaining
  gap owned by effect-active frames (see Status; the earlier asset-load
  reading is refuted). Clean frames are ~63K under gate.
- **Cosmetic systems (R2-07):** the measured margin, not the table — on the
  frames that carry the most effects, roughly 23K DLDI-off and negative
  DLDI-on until the load work lands. For scale, 23K buys ~fourteen
  textured-quad binds (Task 98: ~1,621 ticks/bind) plus a few thousand
  interpreter steps. The table's 80K "effects + audio" line is fiction; do
  not allocate from it.

---

## 5. Repo shape and governance

- Runtime 2 code lives under `src/nds/r2/` (repo rule: DS/backend behavior
  under `src/nds` or `src/port`); generated data stays in the existing
  `src/nds/*.generated.inc` convention; generators stay in `scripts/`.
- Phases ship as additional build targets behind an `NDS_R2_*` make-flag
  family, all default 0. The published ROMs stay pure Runtime 1 until the
  switch. No second repo, no divergent asset pipeline.
- This file plus `PERF_LEDGER.md` rows are the only Runtime 2 documents.
  R2 work items enter `docs/P1_EXECUTION_BOARD.md` as ordinary rows;
  `docs/VERIFYING.md` plus the board's standing rules govern how each phase is
  measured and judged (`TASK_STANDING_RULES.md` was archived 2026-08-05).
- New harness modes are legitimate for the early phases (they are scene-level
  capabilities); each phase's diagnostic mode is deleted or graduated when
  the next phase lands. No proof-only branches, no permanent seed/restore
  wrappers.
- Reuse aggressively where it saves time without inheriting runtime cost:
  BattleShip sources, extraction/conversion tooling, generated asset data,
  harnesses, melonDS instrumentation. Do not inherit architecture merely
  because it exists: object-manager traversal, generic render-owner
  abstraction, runtime relocation, texture-key construction, display-list
  interpretation, and stats snapshots are exactly what R2 deletes.

Sequencing against in-flight P1 work:

- **Particle banks** (P1 board row) land in Runtime 1; R2 Phase 7 consumes
  the ported banks as data. Porting them twice would be waste.
- **Task 75** (preload fighter animations; owns the P95 tail) is absorbed
  into Phase R2-04's match-preparation work unless it proves to be a trivial
  exact win earlier.

---

## 6. The switch, precisely

The switch is the moment the Boundary configuration —
`battle_playable_realtime`, mode 163, Mario vs level-3 Fox CPU, Dream Land,
items off, 3600 ticks — is produced by the Runtime 2 battle path and both
published ROMs (plus the tick-HUD sibling, flag-identical) are rebuilt from
it, with the public-build pin updated in the same kept change.

After the switch, the Runtime 1 battle path remains in-tree behind its flag
as reference/oracle until the migration is declared mature, then is deleted
per the obsolete-mode rule.

Switch acceptance (all required):

1. Boundary verifier green on the Runtime 2 battle path.
2. Visual gate: synchronized screenshot diffs within the reported fidelity
   budget plus the owner's visual approval.
3. Performance gate: P95 ≤ 1.12M ticks/presented frame, **DLDI-on**, on the
   accuracy melonDS fork; device A/B reported as the 2/3/4/5+ VBlank
   histogram with max interval.
4. Stability: full 3600-tick soak with zero flashes, corruption, hangs, or
   unexplained state differences.
5. Owner play test on retail hardware, recorded (ROM hash, hardware,
   duration, what was watched for).

---

## 7. Phase plan

Each phase is one board row with three gates — correctness, visual,
performance+traffic. A phase that misses its budget is redesigned before the
next phase starts.

### R2-00 — Baseline, budgets, stall attributor

- Re-run the frame-work census (Task 65 method) on current master to replace
  the stale 1,527K baseline; freeze §4's budgets against it.
- Build the stall attributor Task 108 named: per-bucket icache/dcache-miss
  and bus-stall counters in the melonDS fork, ring-dump transport. Validate
  it against two known historical results (e.g., Task 104's removed traffic).
- No runtime code. Gate: attributor agrees with the known results.

### R2-01 — Battle-path skeleton and flag family

- `src/nds/r2/` entry point that boots the battle scene, runs the existing
  60 Hz gameplay tick unchanged, and presents frames with the Runtime 1
  renderer still drawing. `NDS_R2_PATH=1` selects it. This isolates the
  scene-flow seam once so later phases swap renderers under a stable roof.
- Gate: tick-identical gameplay (Boundary verifier); frame cost within noise
  of Runtime 1.

### R2-02 — Dream Land direct runtime

- Compile Dream Land by what changes, not legacy segment ownership:

```text
STATIC RIGID     -> extend Task 36/53 replay into a fully direct owned path:
                    no generic preflight, no stats temporaries, no per-frame
                    texture resolution; the runtime shape is DreamLand_Run17(),
                    not discover/validate/rebuild/resolve/prepare/submit
DYNAMIC VISUAL   -> Whispy + flowers: small specialized update+draw path
GAMEPLAY         -> stage collision unchanged (it is gameplay code)
```

- Generator work extends `generate_nds_native_stage.py` /
  `generate_dreamland_ds_mesh.py`; runtime work is mostly deleting the stage
  `PrepareRun` head. Do not make the static majority pay for the dynamic
  minority. Dream Land water stays frozen at source frame 0.
- Gate: screenshot-identical static stage; STG P50/P95 within budget
  (provisional 180K); traffic reduced vs baseline; Boundary green.

### R2-03 — Fighter direct draw (static pose)

- Promote the native fighter owner to a direct path: per-epoch generated
  submit consuming only baked facts (`poly_fmt`, texture slot, palette,
  matrix binding, corner stream) — no `PrepareProductionRun` policy
  re-checks, no traversal-state/stats dependency, no per-frame texture
  identity proof. Encode facing/cull exemptions for sub-pixel geometry in the
  generator as data, not runtime checks — DS behaviour there does not match the
  N64 rasteriser, which is a hardware fact and the durable lesson of a closed
  bug (see `PORTING.md`), so the generator has to carry it whether or not any
  bug row is open.
- Mario first, fixed pose. Measure draw cost, GX words, stall share, bytes
  touched.
- Gate: pixel parity against Runtime 1 on the same pose (Task 49 GX differ +
  screenshot); combined-fighter render budget (provisional 250K) on track.

### R2-04 — Fighter visual pose at presentation rate

- Generated visual-pose evaluation feeding the direct draw path, decoupled
  from the gameplay skeleton (§3.6): evaluated once per presented frame
  (30 Hz), not per gameplay tick. Do not assume full cubic pose evaluation
  must run twice per rendered frame because gameplay is 60 Hz.
- Absorbs Task 75: all animation streams for the match prepared at load; no
  first-use loading during gameplay (§3.8).
- Gate: animation timing visually correct (owner check); SRC-class P95
  excursions gone from the histogram; pose budget (provisional 100K) met.

### R2-05 — Fox through the same pipeline

- Same generators, same direct path, zero fighter-specific runtime special
  cases. This phase proves the compiler generalizes; any hand-patched Mario
  exception found here is a generator defect to fix.
- Gate: both fighters within combined budgets; generators reproduce the
  `.inc` files byte-identically from a clean checkout.

### R2-06 — Full Runtime 2 battle

**Mario CPU vs Fox CPU is a harness configuration, not a product change.** It
exists so R2-06/R2-07 can be gated without a recorded input stream, and as a
deliberate stress case: two CPUs attack continuously, which maximises the live
hitbox population that R2-03 E35 measured as the owner of the `SRC` P95
excursion. The shipped Boundary stays Mario human vs level-3 Fox CPU at mode
`163`. Since 2026-08-05 the owner gates the milestone on the stress config
itself (§3.9): the both-CPU whole match must sit under the P95 budget, loading
states excluded. Still never swap a stress figure in silently for the Boundary
number (`NDS_R2_BOTH_CPU` in the Makefile).

- Mario CPU vs Fox CPU on Dream Land through `NDS_R2_PATH`: 60 Hz gameplay,
  collision, damage, knockback, shields, CPU behavior, camera, 30 FPS
  presentation. Effects/audio still Runtime 1 machinery where needed.
- Gate: Boundary green, equivalence between the two arms, and a clean soak.
  Report the 2/3/4/5+ VBlank histogram as evidence of *no regression*, not of
  a win.

**The switch is an architecture and correctness step; the performance belongs
to the phases that feed it.** Measured 2026-07-29 (R2-06 E0), with engagement
verified in both ELFs: the two arms are indistinguishable on the stress config
because every saving the campaign produced is enabled on *both* sides of the
switch. Requiring the switch itself to improve the histogram measures the
wrong thing and would block a correct switch indefinitely.

### R2-07 — Effects, audio, HUD, match flow, and Bugs

- START on the Results screen restarts the match (P1-specific; row closed in
  `BUGS.md` — keep it closed).
- Ported particle banks (from the P1 row), SFX/voice/BGM, HUD, GAME SET →
  results flow. Cosmetic systems get explicit budgets so they cannot erase
  the headroom.
- **All rows in `BUGS.md` fixed before optimization work.** this is a P1 Bugs list and are required to be fixed for P1.
  **Status 2026-08-13 — both open rows are measured to exhaustion with NO
  divergence from BattleShip found** (`artifacts/bugs/2026-08-12_fox-crouch/`).
  Fox: spawn X/Y/Z, attack radius 20, all 11 hurtbox descriptors, camera
  identity, composition (0.004 px), gun attachment, pose phase, baked model
  geometry and quad anchor all source-exact; the beam is geometrically not
  duckable ON SOURCE NUMBERS. Whispy: script decode, normalize, playback, the
  60/30 Hz clock, transform application and matrix consumption (XObj kind 28 =
  `TraRotRpyRSca`) all correct — six distinct `scale.y` values reach a
  scale-bearing matrix over six consecutive presented frames. Per
  `BUG_FIXING_PROCESS.md`, a candidate whose every contract row is source-exact
  means the CONTRACT is under-specified: each row needs one symptom detail from
  the owner (Fox: flash, beam, or both, and when; Whispy: which of blink / eye
  turn / mouth / wind looks stepped) before any edit would be anything but an
  invented offset.
- Gate: full demo loop (Mario CPU vs Fox CPU, 1-minute and 5-minute match
  lengths) within total budget; battle P95 still ≤ 1.12M DLDI-on. The
  5-minute run is an **owner-instructed acceptance exception** to the
  standing "never launch the five-minute configuration" rule in `AGENTS.md`;
  it applies to this gate only, not to routine iteration.

**Where the gate work stands (2026-08-05).** The tail is the effect DObj
submit path, and the work is the board's gate lane: **G1** (resolver site
cache on for mode 9) → **G2** (RAM headroom against the boot cliff) → **G3**
(the effect packet path) → **G4** (re-baseline, pick the next lever from the
residue). Specs, protocols, and the settled diagnosis live on the board.

The collision era of this clause is closed history, kept compact so nobody
re-derives it: **L7 was wired, measured and reverted** (+534 cycles/frame won
in `SRC` against 6,481 lost in `FTR`+`STG` to its own ARM text; what survives
is the oracle-only header behind `NDS_R2_COLLISION_L7_ORACLE`, `Makefile:716`,
default 0 — not compiled into the ROM). Collision is **2.9%** of the over-gate
premium; L6's 66.2% soft-float attribution did not survive; the asset-load
reading was refuted three times. **L9** (SSB64's sine table, −37,248 P95) and
**L10** (the hardware square root, −12,160 P95) are banked.
`NDS_TASK56_FIGHTER_PRIMITIVES=2` cannot be priced: that ROM hangs the present
loop and has never completed a run in three attempts. The standing design
constraint survives all of it — **1.85 cycles of `FTR` mean per byte of added
ARM text: a change that adds text must beat its own footprint** — which is why
deletion beats substitution and why the boot cliff (board G2) gates every
candidate. Full history: the board archive.

The contingency ladder if the packet path lands short, in the order
`PROJECT_GOAL.md`'s sacrifice list implies:

1. **Buy headroom flat.** The P95 frame pays every flat bucket in full: `FTR`
   at ~389K against its 250K line and `STG` at ~195K against 180K are the
   flat levers, and deletion is the preferred instrument.
2. **Run cosmetic systems below simulation rate.** 15 Hz particles are
   allowed — but round-robin a quarter of the generators per frame, never
   "every fourth frame, update everything": batching quarter-rate work onto
   one frame lowers the mean while **raising P95** (R2-03 E30: when the
   median falls and the P95 does not, stop cutting the median).
3. **Reduce visual fidelity** — a cheaper source-derived approximation with
   the visible delta recorded is the contract-compliant answer.
4. **A COMPENSATED 30 Hz simulation — the OWNER'S CALL, in writing.**
   `NDS_TASK106_UPDATES_PER_PRESENT=1` measured the uncompensated ceiling at
   **−119,744 P95**, playing at half speed — a ceiling, not a candidate.
   Compensating it (advancing timers, physics integration and animation two
   frames per tick) is the only remaining move with the measured size to
   close the gate alone, and the judgement is reserved for the owner.
   
   
** STRESS TEST Gate ** Mario CPU vs Fox CPU on Dream Land, Full Match (including testing Sudden Death via force switch) - 
   match stress test gameplay start to finish must not exceed 1.12M P95 (minus loading states)
   pressing start at results screen restarts the P1 match, up to infinite successive matches.
   **Optimization ideas are contained here to help get under 1.12M P95 Gate: ".\docs\optimization\OPTIMIZATION_IDEAS.md"**


Do **not** resolve any of it by widening the gate. The gate is the product
contract, and 1.12M is the number `PROJECT_GOAL.md` sets.

### R2-08 — The switch

- Execute §6. Flip the Boundary to the Runtime 2 path, rebuild the published
  ROMs, update the pin, record the owner's retail play test. Runtime 1 stays
  behind its flag as oracle until retired.

---

## 8. Kill criteria

Runtime 2 is not automatically correct because it is new. Pause or redesign a
direction when:

- static Dream Land cannot fit a reasonable stage budget
- a generated direct path is not materially cheaper than its Runtime 1
  equivalent at its phase gate
- the new architecture recreates generic runtime discovery, becomes
  pointer-heavy, or large generic structs / blanket clears / copies reappear
  in hot paths
- loading-time work leaks into gameplay
- visual and gameplay state become tightly coupled again
- one subsystem consumes its budget with no credible architectural lever left
- P95 stays dominated by unpredictable one-off work
- memory traffic is high even where instruction counts look acceptable

The purpose of Runtime 2 is to eliminate the old structural problems, not
rename them.

---

## 9. Philosophy

For every piece of Runtime 2 work, the standing questions are: does this work
have to happen during gameplay; does this data have to be discovered — or
touched — at all; does this pointer, abstraction, or temporary have to exist;
which bytes are actually live; can the compiler know this already; can
loading do it once; can RAM store the answer; can visual work run less often
than gameplay?

```text
Fastest correct DS implementation wins.
Do not optimize moving data. Avoid moving the data.

BattleShip is the behavioral specification.
Runtime 1 is the reference implementation and optimization laboratory.
Runtime 2 is the Nintendo DS engine.
```

---

## 10. Owner decisions and the queue

The board (`docs/P1_EXECUTION_BOARD.md`) is the **only** queue. This section
is not a status log; it holds only decisions that are the owner's to make,
plus pointers to where everything else lives. When this section and §7
disagree, §7 is the contract — a stale entry here already stopped a live
phase for a day (2026-07-29). Measurements and history belong on the board
and in `PORTING.md`, never here.

**Open owner decisions: none.** The two formerly listed here are resolved and
shipped: the hurt-flash blocker turned out to be a generator gap, not a
visual-approval call (E62), and E32 graduated at −52,416 P95; the fixed-point
cubic graduated as E64b (−26,944 P95) with equivalence settled by E65,
retiring the Task 9 bit-exactness question for that path. Their full history
lives on the board.

Durable lessons from the superseded versions of this section were promoted
into the charter instead of being left as status: the DLDI-on canonical
configuration (§3.9), the no-gameplay-time-allocation rule (§3.11), and the
2026-07-30 measurement-discipline findings (compiler behaviour invisible in
the C; quantised instruments inventing ticks), which the board's standing
rules 8–11 carry.
