# Smash64DS Runtime 2 — Plan of Record

The single Runtime 2 document: charter, design rules, budgets, phase plan,
and the definition of the switch.

Status: **in execution.** R2-00a/b/c, R2-01 and R2-02 are gated; R2-03 shipped
E12/E28/E29/E46; R2-04's loading and rate clauses are met. **Both remaining P1
levers are now owner decisions, not experiments** — see §10. The board
(`docs/P1_EXECUTION_BOARD.md`) is the live queue; this file stays the charter and
is not a status log.

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
| fighter compiler | `scripts/generate_nds_native_owners.py` → `src/nds/nds_native_fighter_owner.generated.inc` (dense vertices, baked positions, runs, epochs, per-epoch material/cull policy, FIFO plans — Mario and Fox) |
| stage compiler | `scripts/generate_nds_native_stage.py` → `src/nds/nds_native_stage_owner.generated.inc`; `scripts/generate_dreamland_ds_mesh.py` → `src/nds/dreamland_ds_mesh.generated.inc` |
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

### 3.10 Fidelity floor

Keep the approved full-3D Dream Land presentation and SSB64 visual identity.
Runtime 2 reduces runtime overhead, not stage content; Runtime 1 already
proved large visual reductions were poor trades. The render fidelity doctrine
applies: screenshot diffs plus the owner's visual approval, with the owner as
the visual oracle.

---

## 4. Frame budget

`ALL` is VBlank-quantized at 560,190 ticks/VBlank; today the frame is pinned
at 3 VBlanks with excursions to 4–5. The switch's whole purpose in one line:

```text
move the histogram from 3-VBlank frames to 2-VBlank frames
2 VBlanks = 1,120,380 ticks = the P95 gate (PROJECT_GOAL: P95 <= 1.12M)
```

Provisional per-subsystem targets (frozen at R2-00 after a fresh census; each
also receives a traffic budget from the stall attributor's baseline):

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
  `TASK_STANDING_RULES.md` governs how each phase is measured and judged.
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

- **Bug #10 closes first.** Its root cause (DS facing/cull behavior on
  sub-pixel generated triangles vs the N64 rasterizer) is a hardware fact the
  R2 render generator must encode as data. It is one E0 build from a verdict.
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
3. Performance gate: P95 ≤ 1.12M ticks/presented frame on the accuracy
   melonDS fork; device A/B reported as the 2/3/4/5+ VBlank histogram with
   max interval.
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
  identity proof. Encode the bug #10 lesson (facing/cull exemptions for
  sub-pixel geometry) in the generator as data, not runtime checks.
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
`163`, and `PROJECT_GOAL.md`'s P95 gate is defined on *representative* gameplay —
so a P95 read off the stress config is a harder number than the milestone
requires and must be reported as such, never swapped in silently for the
Boundary figure.

- Mario CPU vs Fox CPU on Dream Land through `NDS_R2_PATH`: 60 Hz gameplay,
  collision, damage, knockback, shields, CPU behavior, camera, 30 FPS
  presentation. Effects/audio still Runtime 1 machinery where needed.
- First full-frame gate: the 2-VBlank share becomes the headline metric.
- Gate: Boundary green; soak clean; ~~histogram materially better than the
  Runtime 1 A-side on the same commit~~.

**Amended 2026-07-29 by R2-06 E0.** The struck clause is the wrong gate and was
measured to be so. With engagement verified (`ndsR2BattleRun` present in one ELF
and absent from the other, config dumps reading `NDS_R2_PATH` 1 and 0), the two
arms are indistinguishable on the two-CPU stress config: 2-VBlank share 66.1% vs
66.7%, `WORK-H` P50 +896, P95 +12,544 — all inside the 5,000–7,000 placement
floor.

That is not a defect. `ndsR2BattleRun` is deliberately the same loop shape as the
Runtime 1 body it replaces, and every saving the campaign has produced (R2-02
stage direct, R2-03 fighter direct plus E32/E64b, R2-04 loading and rate) is
already enabled on *both* sides of the switch. **The switch is an architecture and
correctness step; the performance belongs to the phases that feed it.** Requiring
it to improve the histogram measures the wrong thing and would block a correct
switch indefinitely.

**R2-06 therefore gates on Boundary green (done) plus equivalence and a clean
soak.** Report the histogram, but as evidence of *no regression*, not of a win.
Also note the stress config is not the Boundary figure — see §7 R2-06's own
warning and `NDS_R2_BOTH_CPU` in the Makefile.

### R2-07 — Effects, audio, HUD, match flow

- Ported particle banks (from the P1 row), SFX/voice/BGM, HUD, GAME SET →
  results flow. Cosmetic systems get explicit budgets so they cannot erase
  the headroom.
- Gate: full demo loop (Mario CPU vs Fox CPU) within total budget; P95 still ≤ 1.12M.

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

## 10. Immediate next actions

Superseded as of 2026-07-29. The original two items (owner approves the plan;
add R2-00 to the board and start it) are both done, along with R2-01 through
R2-04's loading and rate clauses.

**Autonomous execution has run out of unblocked P1 levers.** Both remaining ones
are owner decisions, and each is now precisely priced rather than a hypothesis:

1. **E32 — the fighter fallback, worth −51,136 P95 and four frames.** Blocked on
   the hurt-flash visual regression. Six experiments (E48–E59) failed to find a
   mechanism and that line is closed: it is not vertex colour, material colour,
   light colour, the fold arithmetic, E16's hardware lighting, or
   `color_modulate`. It is therefore a **fidelity-budget question** under §3.10
   and `PROJECT_GOAL.md` — the owner's visual approval — not a measurement.
2. **The fixed-point cubic — worth ~50,000 ticks/frame, on every frame.**
   Blocked on the Task 9 state hash, which asserts bit-exactness where
   `PROJECT_GOAL.md` requires only mechanical equivalence and explicitly permits
   "fixed-point replacements". Confined to `gcGetInterpValueCubic` evaluating
   already-parsed track state; its only path to gameplay is
   `gmCollisionGetFighterPartsWorldPosition`, so the honest acceptance test is a
   hitbox-overlap differential over a full match rather than the hash.

`108,928 − 51,136 − ~50,000 ≈ 7,800`. **Those two together close the gate**;
neither closes it alone. R2-05's generator-reproducibility gate already passes,
and R2-06/07/08 are gated behind the budget being met, so there is no way to
proceed on P1 performance without answering at least one of the two above.
