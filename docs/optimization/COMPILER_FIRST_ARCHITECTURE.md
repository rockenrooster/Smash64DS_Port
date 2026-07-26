# Compiler-First DS-Native Architecture

**Owner:** the campaign's architectural direction — what gets replaced, in what
order, and what each replacement must prove.
`docs/P1_EXECUTION_BOARD.md` still owns priority and current state,
`docs/PERF_LEDGER.md` still owns measurements, and
`docs/optimization/NATIVE_RENDERER_PLAN.md` still owns the M2-M4 renderer
implementation contract and the two build axes. This file does not restate any
of them.

## Purpose

This replaces the earlier runtime-rewrite roadmap with a more efficient path to
the `PROJECT_GOAL.md` objective:

> Preserve SSB64 behavior and identity, but rebuild expensive runtime systems
> around what works best on Nintendo DS hardware.

The project should **not** spend months incrementally optimizing generic
BattleShip-derived runtime systems that will eventually be removed. It should
also **not** attempt one giant all-at-once engine rewrite.

```text
BattleShip behavior + assets
          |
          v
   generic host compilers
          |
          v
generated DS-native fighter/stage programs
          |
          v
very small specialized match runtime
```

> **Build tooling should be generic. Runtime code should be specialized.**

BattleShip remains the behavioral oracle. It does not need to remain the runtime
architecture.

---

## This is what `NDS_BATTLE_PROFILE=0` was reserved for

`NATIVE_RENDERER_PLAN.md` already defines the axis this campaign lands on:

- `NDS_BATTLE_PROFILE=1` — today's translation path. The correctness oracle.
- `NDS_BATTLE_PROFILE=0` — the DS-native precompiled path. Declared, unbuilt,
  and failing closed with `$(error)` until it exists.

Generated fighter and stage programs are the contents of profile 0. This matters
practically: the A/B structure every task below needs — exact old path against
generated new path, same ROM configuration — is already expressed as profile 1
versus profile 0, and the tick-HUD instrument already reports both. **Do not
build a parallel selector.** A generated subsystem enters under profile 0 and
the profile-1 path stays the oracle until profile 0 beats it on both gates.

---

## Why this is the most efficient path

Profiling changed the optimization picture. Approximate per-frame work:

```text
renderer-related work      ~794K ticks / ~52% of real work
soft-float                 ~177K
mem*                       ~139K
matrix work                ~137K
texture resolution         ~114K
GX submission               ~90K
```

**These are overlapping categories, not a partition.** Soft-float, `mem*`, and
matrix work all occur *inside* renderer work. They must not be summed, and a
task may not claim two of them as independent budgets.

Two further constraints on how these numbers are read:

- **Task 65 measured 62% of frame work as stall.** A generated program that
  removes instructions but preserves a pointer-chasing data layout will not
  recover that. Flat, contiguous, compile-time-ordered arrays are what recovers
  it — which is an argument *for* this architecture, but it means the win comes
  from the data layout at least as much as from the generated code.
- **Gameplay simulation proper is small but not as small as it looks.** Task 65's
  split puts it near ~150K; a later estimate put it at 60–80K. The two have not
  been reconciled. The conclusion below survives either figure, but no task may
  cite the smaller number as established.

The largest opportunities are not in hitboxes, physics, CPU AI, or match rules.
They are in generic renderer infrastructure, animation infrastructure, resource
relocation and loading, matrix work, soft-float, repeated material and texture
resolution, generic runtime interpretation, and the memory stalls caused by
pointer-heavy general systems.

The project gets more value by removing runtime abstractions than by making
those abstractions a few percent cheaper.

---

## Core engineering rule

Do not ask:

> "How do we optimize this generic BattleShip-derived runtime system?"

Ask:

> "Does this runtime abstraction need to exist at all on the DS?"

If the answer is no: compile it away, bake it, generate it, quantize it,
precompute it, specialize it, move it to load time, or replace it with a
DS-native representation.

The old runtime should survive only where it is already cheap enough.

---

## What not to do

- No giant `rewrite-engine` branch replacing everything at once.
- No twelve hand-written fighter engines.
- Do not rewrite gameplay systems merely because `PROJECT_GOAL.md` allows it.
- Do not keep polishing generic runtime systems a near-term compiler will delete.
- Do not pursue sub-10K optimizations that disturb code or data layout unless a
  more precise instrument proves the effect. The measured placement noise floor
  is ±8,000 ticks; this is Task 74's lesson and it is not negotiable.
- Do not revive the Dream Land geometry-reduction campaign.

---

## Target architecture

```text
                 BattleShip Decomp
                       |
         +-------------+-------------+
         |                           |
      behavior                      assets
       oracle                         |
         |                            |
         |                    host extraction
         |                            |
         +--------------+-------------+
                        |
                        v
                CANONICAL BUILD IR
                        |
         +--------------+--------------+
         |              |              |
         v              v              v
     Fighter IR      Stage IR       Asset IR
         |              |              |
         +--------------+--------------+
                        |
                        v
              DS-NATIVE CODE GENERATORS
                        |
       +----------------+----------------+
       |                                 |
       v                                 v
 generated fighter programs       generated stage programs
       |                                 |
       +----------------+----------------+
                        |
                        v
                 MATCH PREPARATION
                        |
                        v
                 DS MATCH RUNTIME
```

Runtime ideally performs only: gameplay logic that genuinely changes, minimal
pose evaluation, minimal live matrices, visibility decisions, dynamic
materials and effects, and direct GX submission.

---

## The compiler is the product

The scalable asset is not `Mario_Draw()` and `Fox_Draw()` by themselves. It is
the host pipeline that can generate them.

```text
compile_fighter.py mario
compile_fighter.py fox
compile_fighter.py link
```

emitting

```text
generated/mario_skeleton.inc   generated/fox_skeleton.inc
generated/mario_anim.inc       generated/fox_anim.inc
generated/mario_materials.inc  generated/fox_materials.inc
generated/mario_render.inc     generated/fox_render.inc
```

The runtime may still expose `Mario_UpdatePose()` / `Mario_Draw()`, but those
implementations should be mostly generated from BattleShip data and shared
compiler logic. That is how the project reaches twelve fighters without solving
the same problem twelve times.

Generated output is allowed to be ugly, repetitive, and fighter-specific. The
build tooling carries the generality; the runtime carries the speed.

---

## Development strategy

Every native replacement uses the existing path as its oracle:

```text
current exact path
        |
        +------------------+
        |                  |
        v                  v
      OLD PATH          NEW PATH
    (profile 1)        (profile 0)
                           |
                           v
                          A/B
                           |
           +---------------+---------------+
           |                               |
      correctness                      performance
           |                               |
           +---------------+---------------+
                           |
                           v
                         KEEP
                           |
                           v
                 retire old hot path
```

Each replacement must be independently measurable, independently revertible,
correctness-gated, performance-gated, and small enough to understand.

### Two gates, not one

`AGENTS.md` splits these and this campaign must not blur them:

- **Gameplay, collision, rules, state, camera meaning, and flow** stay
  mechanically equivalent and **verifier-gated**.
- **Presentation** gates on a reported fidelity budget — synchronized screenshot
  diffs plus the owner's visual approval — not pixel exactness.

The classification of a given output is decided by what consumes it, not by
which subsystem produces it. See the bone-classification rule in Task 78.

---

## Small optimizations policy

Small changes are still worth taking when the return is unusually good. Task 72
is the model: a tiny implementation change, ~79K `WORK-H` P95, low correctness
risk, Boundary verified. Task 76 repeated it for another ~44K.

```text
~5K clever change              usually skip
~20K-30K simple exact change   often worth taking
~50K-100K measured win         high priority
~100K-200K subsystem replace   very high priority
~300K+ architectural win       ideal
```

Prefer large measured kernels and large architectural ceilings.

---

## Task numbering

Tasks 69–76 are closed and their numbers are spent. Task 76 is the sizing-open
removal (`ClaudeOpus5_Task76_DropTheSizingOpen_20260726.md`), **not** the
compiler foundation. This campaign therefore begins at **77**.

## Amended by Task 77 E0 — much of this is already built

`ClaudeOpus5_Task77_FighterIRAudit_20260726.md` audited the repo before writing
the compiler this document specifies, and found `generate_nds_native_owners.py`
(3,164 lines) already emitting "the canonical Mario/Fox native-owner IR": 541
dense vertices, 1,878 corners, 67 render runs, 70 state deltas, per-fighter
`BindingParents` / `BindingJoints` / `JointSchedule`, and — decisively —
`sNdsNativeMarioFifoWords[4034]` with `FifoMatrixPatches[14]`, a **prepared GX
command stream with matrix patch points**.

That last one is Task 79's deliverable, already generated. The consequence is a
reorder, not a retraction — the compiler-first thesis is confirmed, it is simply
further along here than the draft assumed:

- **Skeleton, render runs, materials, prepared GX** — built. Tasks 79 and 80
  become "finish wiring and prove the runtime actually consults these" rather
  than new subsystems.
- **Animation tracks** — genuinely absent. This is the real frontier and should
  be the next implementation task.
- **`gameplay_load_bearing`** — absent, and its source of truth is now located
  (see the audit's §3: 21 joint references across 16 structs in
  `include/ft/fighter.h`, split gameplay vs cosmetic).

The roadmap at the end of this document is superseded on ordering by the audit's
§2, and again by Task 78 E0 below.

## Amended by Task 78 E0 — the animation lever is 82,807, not ≥100,000

`ClaudeOpus5_Task78_AnimationLeverE0_20260726.md` measured the animation path
over the standard 128-frame window before writing the compiler. The whole
family — 32 symbols, `gcPlayDObjAnimJoint` through
`battleship_ftAnimParseDObjFigatree` — is **82,807 ticks/frame**, plus 1,743 for
joint hierarchy transform. A perfect animation compiler falls 15,450 short of
this task's own target, and Task 77 E1 already removed the approximations that
would have widened it.

Measured ranking of the frame (work = 1,515,768 after idle):

| family | ticks/frame | % of work |
|---|---|---|
| renderer | 739,715 | 48.8% |
| soft-float | 161,471 | 10.7% |
| texture / material resolution | 159,968 | 10.6% |
| matrix | 158,500 | 10.5% |
| `mem*` | 137,193 | 9.1% |
| animation | 82,807 | 5.5% |

**Revised order: prepared texture/material first** (this document's Task 80 —
1.9× the animation lever, generated data already exists in
`sNdsNativeFighterStateDeltas`/`StateSequence`, no fidelity risk), then
soft-float and `mem*`, then the animation compiler re-scoped against 82,807.

Two reorders in two tasks now point the same way: the renderer is where the
frame is, and its generated tables are **under-used rather than absent**. That
is a different campaign from the one this document opens with — the compiler
largely exists; what is missing is the runtime consuming it.

---

## Task 75 — Minimal animation preload bridge (optional)

### Status: unblocked, and deliberately optional

Task 75's implementation was blocked on a choice between two arena designs. This
document resolves it by choosing the raw-arena design below, and that choice
**clears the STOP rule** — an unregistered arena needs no new loaded-file
system, no private relocation framework, no renderer change, and no alias
tracking. Only the registered-arena variant tripped those.

### Known facts

Mario + Fox animation data: 301 files, ~711 KiB raw, ~800 KiB estimated prepared.
The BattleShip caller discards the returned pointer and uses the slot it passed
(`ftmain.c:4623`), so animation data must end up in `fp->figatree_heap` and one
RAM copy is unavoidable without changing the behavioral source path.
`NDS_RELOC_LOADED_FILE_CAPACITY` is 96, so the generic loaded-file registry
cannot be the home of 301 preloaded animations — and registering them would also
let an arena copy capture `ndsRelocFindLoadedFileContaining`, which the renderer
uses to resolve fighter display lists.

### Design

```text
MATCH LOAD:                      GAMEPLAY:

NitroFS                          preload arena
  |                                |
  v                                v
read animation                   RAM memcpy -> figatree_heap
  |                                |
  v                                v
byte swap once                   existing finalize/relocation
  |                                |
  v                                v
private raw/pre-fixup arena      use animation
```

The arena is never registered, so nothing the renderer queries can alias it.

### The trade, stated plainly

If Task 78 lands, generated animation data is linked or loaded as one blob and
the 301-file path disappears — **this arena is throwaway by construction.** It is
worth building only if the P95 gate is needed before the compiler chain
completes. Task 76 also reduced what it wins: the NitroFS walk it removes now
costs one third of what it did when Task 75 was sized. Skip it without regret if
Task 77 starts immediately.

### Required RAM gate

Measure peak runtime RAM over the canonical match and require a safe reserve
after the arena is allocated, before reserving it.

---

## Task 77 — Fighter compiler foundation

### Goal

The reusable build-time pipeline converting BattleShip fighter data into a
canonical DS-oriented fighter IR. Mario and Fox first, same pipeline for both.

### Inputs

Skeleton/DObj hierarchy, animation tables, material assignments, texture
references, display-list ownership, part visibility data, matrix relationships,
per-part render metadata.

### Output

```text
FighterIR
  fighter
  skeleton
    bones[]
      parent
      bind transform
      render parts[]
      gameplay_load_bearing      <-- required, see below
  animations[]
    tracks[]
  materials[]
  textures[]
  render_runs[]
```

The IR is build-time only and need not resemble BattleShip's runtime structures.

### The gameplay-bone flag is mandatory in the IR

Every bone must carry whether any collision volume, hurtbox, hitbox, grab point,
or article spawn attaches to it. This cannot be added downstream: Task 78's
quantization and rate-reduction decisions are per-bone, and without the flag in
the IR there is nothing to enforce them against. Bones that carry gameplay
attachments are subject to the verifier-gated equivalence contract; purely
cosmetic bones fall under the visual fidelity budget.

### Requirements

- BattleShip remains the source of truth.
- Extraction deterministic, build output reproducible.
- Do not hard-code fighter behavior into the extractor unless it is genuinely
  fighter-specific content.

### Proof obligation

The IR ships with a checker, following the precedent already in this repo:
`scripts/dreamland_world_mesh.py`'s `check_ir` proves determinism (a rebuilt IR
hashes identically) *and* coverage (every source element is represented exactly
once, mapped back through provenance). Fighter IR needs the same, so "the IR is
correct" is a result rather than a claim. Deliverable is the host tool plus its
checker; no runtime rewrite in this task.

---

## Task 78 — Generated DS animation engine

### Goal

Replace generic figatree/DObj animation evaluation for Mario and Fox with
generated DS-native animation programs.

### Path to eliminate

```text
animation asset -> relocation -> figatree parser -> generic DObj animation
  -> generic hierarchy traversal -> float-heavy transforms -> matrix generation
```

### Target path

```text
Fighter IR
  |
host animation compiler
  |
  +-- fixed skeleton order      +-- pre-expanded metadata
  +-- fixed parent indices      +-- fixed-point coefficients
  +-- quantized tracks          +-- optional pose tables
  |
  v
generated DS animation data -> Mario_UpdatePose() / Fox_UpdatePose()
```

### Allowed techniques, split by gate

**On cosmetic bones** — quantized rotations and translations, precomputed
interpolation metadata, common-pose tables, lower skeletal update rates where
visually acceptable.

> **Measured, Task 77 E1: this set is currently empty.** For both Mario and Fox
> every joint in `effect_joint_ids` is also a hurtbox joint, so no joint is safe
> to quantize on the grounds of being decorative. Mario: 24 joints present, 18
> gameplay, 7 unclassified, 0 cosmetic-only. Fox: 26 / 19 / 8 / 0.
> (`artifacts/task77-fighter-joints.json`.) Quantization, rate reduction and
> lossy pose tables are therefore unavailable on current evidence, and this
> task's win must come from removing interpretation and memory stalls rather
> than from approximating the pose — which is consistent with 62% of frame work
> being stall. The exactness-preserving techniques below are unaffected.

**On gameplay-load-bearing bones** — fixed-point tracks and precomputed
traversal order are fine because they are exactness-preserving reorganizations.
Quantization and rate reduction are **not**, unless the resulting transform is
proven equivalent under the verifier. Hitbox placement is gameplay, not
presentation, regardless of the fact that a renderer subsystem computes it.

Both gates apply independently: cosmetic changes still need the owner's visual
approval, gameplay bones still need a green verifier.

### Performance target

A combined reduction on the order of **≥100K ticks** across animation, matrices,
soft-float, hierarchy, and memory stalls. This is a hypothesis derived from
overlapping category totals, not a measured budget — treat a shortfall as
information about the estimate, not automatically as a failed task.

---

## Task 79 — Generated fighter render programs

### Goal

Mario and Fox render directly from native pose output. The runtime stops
translating native fighter state back into generic BattleShip-shaped render
structures.

### Path to remove

```text
DObj -> MObj -> generic display lists -> generic material resolver
  -> texture key construction -> texture hash/probe/memcmp
  -> generic matrix lookup -> generic GX recorder -> GX
```

### Target

```text
native pose matrices
      |
generated fighter render program
      +-- known part order       +-- known matrix source
      +-- known material         +-- prepared GX run
      +-- known texture
      |
      v
direct GX submission
```

Conceptually `Mario_Draw(&mario_state)`. The generated program knows at compile
time: part order, bone ownership, render-run ownership, texture and palette
identity, polygon state, alpha mode, material state, GX primitive sequence,
immutable state transitions, matrix source.

Runtime work reduces to loading current pose matrices, visibility, truly dynamic
material changes, dynamic effects, and direct GX submission.

### Performance target

**≥100K `FTR` reduction** with full visual correctness. Same caveat as Task 78:
the figure is a hypothesis until Task 81 re-derives it.

---

## Task 80 — Prepared texture and material runtime

### Goal

Eliminate repeated texture/material identity proof during gameplay. Task 65
measured the texture-resolution family near ~114K ticks/frame. The generic
resolver should not rebuild and re-prove mostly immutable state every frame.

```text
PreparedRun { texture, palette, polygon state, material state, generation stamp }

if generation unchanged: direct reuse
else:                    resolve/update once
```

**The goal is to avoid calling the resolver, not to make the resolver faster.**
Do not micro-optimize the existing hash path.

Fold this into Task 79's generated render programs where possible rather than
creating another large generic subsystem. At equal cost, less code wins.

---

## Task 81 — Re-profile the new runtime

After 77–80, run a fresh census. **Do not continue using old hotspot rankings
after major architecture changes** — and specifically, re-derive the category
totals at the top of this document as a partition rather than as overlapping
families, with stall separated from instruction cost.

Measure: `WORK-H` P50 and P95, `FTR`, `SRC`, `STG`, `MISC`, `AUD`, memory stall,
non-memory stall, soft-float, matrix, texture resolution, GX submission, `mem*`,
cart read.

Rank the new top kernels. The next task comes from the new profile.

---

## Task 82+ — Scale the compiler

Same compiler, different fighter input, different generated specialized output.
Likewise for stages — Dream Land, Hyrule, Sector Z — each of which may generate a
completely different runtime representation where that is fastest.

Do not copy Mario's runtime.

---

## Budget gates

- **RAM.** The Task 75 arena has an explicit gate. Generated data needs one too:
  pose tables, quantized tracks, and per-fighter render programs across twelve
  fighters is exactly the budget that is easy to blow late and expensive to
  unwind. Each generator reports its ROM and RAM cost per fighter, and the
  twelve-fighter projection is checked before the second fighter is added.
- **Published ROM identity.** New generators are build-time; the published ROM
  hash must not move until a task deliberately moves it.

---

## Gameplay policy

Do not rewrite gameplay systems first. Keep BattleShip gameplay code where
profiling shows it is cheap. Gameplay simulation proper is small enough that
replacing physics, collision, hitboxes, knockback, CPU AI, move state machines,
or match rules is not worth the correctness risk before the
renderer/animation/resource architecture is fixed.

Later, if profiling changes, generated or specialized gameplay code
(`Mario_Update()`, `DreamLand_Update()`) is allowed — driven by evidence.

---

## Dream Land policy

Keep the accepted full stage presentation. Do not resume backdrop card deletion,
aggressive culling, generic mesh simplification, reduced visual replacements that
remove approved scenery, or material-incomplete stage generators.

Dream Land may still receive generated render programs, prepared texture/material
state, static GX streams, fixed-point transforms, load-time preparation, and
stage-specific DS code.

> Optimize how Dream Land is rendered, not what the owner-approved presentation
> contains.

---

## Final roadmap

```text
75  Minimal preload bridge (OPTIONAL - throwaway if 78 lands soon)
    |
    v
77  Fighter compiler foundation        BattleShip -> canonical Fighter IR
    |                                  + gameplay-bone flag + IR checker
    v
78  Generated DS animation engine      Mario + Fox
    |                                  two gates, split per bone
    v
79  Generated fighter render programs  consume native animation directly
    |
    v
80  Prepared texture/material bindings preferably folded into 79
    |
    v
81  Re-profile the new architecture    re-derive the budget as a partition
    |
    v
82+ Scale the compiler to more fighters/stages
```

---

## Final engineering principle

Treat BattleShip increasingly as behavioral specification, asset source, and
validation oracle — and decreasingly as runtime architecture.

The most efficient route to the full project goal is not a cleaner N64-style
runtime running on DS. It is **a compiler-driven DS-native runtime generated
from SSB64 behavior and content.**

Keep cheap original gameplay code where it already performs well. Replace
expensive generic runtime systems where profiling proves they dominate. Prefer
host-side generality and runtime specialization. Prefer precomputation over
repeated interpretation. Prefer prepared RAM over gameplay-time file work.
Prefer generated direct paths over runtime discovery.

**Fastest correct DS implementation wins.**
