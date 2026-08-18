# AGENTS.md

<!-- CODEGRAPH_START -->
## CodeGraph

In repositories indexed by CodeGraph (a `.codegraph/` directory exists at the repo root), reach for it BEFORE grep/find or reading files when you need to understand or locate code:

- **MCP tool** (when available): `codegraph_explore` answers most code questions in one call — the relevant symbols' verbatim source plus the call paths between them, including dynamic-dispatch hops grep can't follow. Name a file or symbol in the query to read its current line-numbered source. If it's listed but deferred, load it by name via tool search.
- **Shell** (always works): `codegraph explore "<symbol names or question>"` prints the same output.

If there is no `.codegraph/` directory, skip CodeGraph entirely — indexing is the user's decision.
<!-- CODEGRAPH_END -->

## Mission
@PROJECT_GOAL.md
This repo recreates SSB64 on Nintendo DS with BattleShip as the behavioral
reference: **original SSB64 behavior + the fastest correct Nintendo DS
implementation = Smash64DS**. Preserve mechanically equivalent SSB64 behavior and
feel; the DS implementation may differ radically from the original engine.

## Hard Rules

- Treat `decomp/` as read-only reference source. Our Source of Truth. Never edit it. 
- Never edit Agents.md or Claude.md unless given permission.
- Inspect relevant BattleShip source before changing gameplay or renderer behavior.
- Inspect `decomp/sm64-nds` and `decomp/sm64ds-decomp` before substantial DS
  renderer, memory, asset, hardware, or backend architecture changes, or when stuck on an issue.
- Reuse original code when competitive. Generated, precomputed, manually
  rewritten, and fighter/stage/move-specific DS implementations are encouraged
  when they are faster and mechanically equivalent.
- Put DS/backend behavior under `src/nds` or `src/port`; compatibility declarations belong under `include`.
- Graduate imported subsystems live. Do not add proof-only branch reruns, one-bit proof masks, or permanent seed/restore wrappers.
- Migrate or delete obsolete bounded modes when natural runtime replaces them.
- New harness modes are only for scene-level capabilities.
- Fix bugs at their owning seam. Specialization is allowed; do not hide shared
  defects with arbitrary offsets/constants, duplicated state, or frame checks.
- Treat flashes, corruption, nondeterminism, hangs, and unexplained state
  differences as failures.
- Respect DS CPU, RAM, VRAM, bandwidth, alignment, fixed-point, and graphics
  limits.
- Treat generated outputs and emulator payloads as generated; never hand-edit them.
- Publish exactly `smash64ds.nds` for P2 and
  `smash64ds-battle-playable-hwtri.nds` for P1; all lab outputs stay in `builds/`.
- User-facing ROMs must be verifier-covered configurations.
- Use only repo-local scripted melonDS. Do not commit runner configs, binaries,
  logs, or shard artifacts.
- In a Task 24 quiet slot, hash-migrate permanent performance and visibility
  evidence before deleting any closed lab build or worktree. Rotate only
  uncited verifier/emulator telemetry; `artifacts/performance` and
  `artifacts/visibility` are permanent evidence. Never combine cleanup with an
  active implementation or remove an ambiguous/dirty worktree.
- Use the custom accuracy-focused melonDS build as the primary development and
  performance reference. Ordinary optimization does not block on repeated
  retail-hardware tests; reserve them for hardware-specific risk and acceptance.
- Rendering-side changes may approximate: See PROJECT_GOAL.md
- Device A/B reports must show the 2/3/4/5+ VBlank-interval histogram and the
  max interval, and P50/P95
- Run `scripts/New-Smash64DSSnapshot.ps1` after verified progress during an autonomous work cycle.

## DS Visual Fidelity

Gameplay, collision, rules, state, camera meaning, and flow stay mechanically
equivalent to the source contract in `PROJECT_GOAL.md`.
Presentation must remain recognizable, readable, and consistent with SSB64's
identity. Timebox cosmetic exactness to one measured experiment, then keep the
cheapest acceptable source-derived approximation.
Record its source, visible delta, measured reason, and `artifacts/visibility`
screenshot. Never accept changed telegraphs, missing/corrupt presentation, or
unexplained behavior. Dream Land water is frozen at source frame 0.

## Operating Model

Start each cycle with:

```powershell
.\scripts\verify-all.ps1 -Profile Boundary -List
git status --short
```

Then read `docs/P2_EXECUTION_BOARD.md` and `docs/HANDOFF.md`. The board is the
only dynamic queue; handoff contains only the restart surface. Select its
highest-impact unowned red row (P2 phase plans: `docs/P2_PLAN.md` + `docs/p2/`).

Preserve a known-good checkpoint before risky changes. On regression, find the
first bad change before layering fixes; trace shared dependencies before edits.

For performance iteration, use one synchronized eight-frame A/B comparison on an
identical ROM/configuration/window. Primary evidence is ticks, FPS, a dated
screenshot, and automated screenshot analysis; semantic/state/geometry counters are
cheap correctness guards. Stop on a decisive KEEP or REVERT; run a third A only when
A/B is noisy, near its gate, surprising, or internally inconsistent. Do not require
routine A/B/A, 32-frame, or 128-frame promotion runs. Milestone tick targets are
directional, not per-cut discard gates: keep every repeatable
correctness-preserving gain and accumulate it toward the target.

Use the smallest focused checker or benchmark while editing. Run one widest
relevant verifier for a kept checkpoint: Boundary for battle-only work, or
Latest instead when normal/shared startup is affected. Do not stack DevFast,
Boundary, and Latest when they cover the same runtime. The registry exposes
only Latest and Boundary; the retired diagnostic fleet must not return.

Subagent switch: **OFF**. A `/goal` block's agent instruction **overrides this
switch** — it is the newer and more specific order, so follow the goal block and
do not stop to reconcile the two.

* `OFF`: let already-running subagents finish, but do not spawn, follow up, or reassign one until the user explicitly switches this back to `ON`.
* `ON`: keep up to **10** long-lived helper agent/agents and assign tasks with appropriate model and effort. Do not manufacture work merely to fill the slot. Your role is **Planner/Reviewer** and the subagent is **Implementer**. Prefer resuming the same subagent, avoid duplicating its investigation/work, and require concise results. Quality takes priority over token savings. New worktrees should be in D:\Stuff\DevFolder\Smash64DS_Port_worktrees
* `OPENCODE-AGENT`: please keep up to **5** opencode agents working for you concurrently using the "opencode-agent" skill. May also be useful for line count limited docs like HANDOFF.md since Opus 5 struggles with that. These are free and cost nothing.

Prefer deletion, existing helpers, fixed DS hardware paths, and the fastest
correct mechanically equivalent implementation. At equal cost, less code wins.
Do not add speculative abstractions, selectors, caches, or tooling.

Milestones cover every requirement assigned by `PROJECT_GOAL.md`; compilation
or one good frame is not completion.

## Builds

Builds parallelise themselves: the Makefile sets `MAKEFLAGS += -j$(NDS_JOBS)` from
`nproc`. **Never pass `-j`, and never clear or override `MAKEFLAGS`** — an explicit
flag wins, which is how every scripted build once ran at half speed. Harnesses pass
no `-j`; the three with a `-Jobs` parameter default it to `0` ("let the Makefile
decide") and **a new harness must not add one back**. Run one build at a time
regardless: the asset generators write into shared paths outside `$(BUILD)`, so
concurrent builds corrupt each other's generated headers whatever `-j` says.
`make NDS_JOBS=1` forces serial for bisecting a generator ordering bug.

## Continuous Improvement
I hate wasting time.
Every new finding, mistake, or inefficiency must improve the next cycle. Fix its
root cause and update the existing shared code, helper, checker, or owning doc that
prevents recurrence. If that is not safe and in scope, record one concise actionable
item in the owning doc; do not detour into unrelated cleanup. This applies to every
aspect of the project, not just code and the end goal — hygiene and docs included. 
An efficient project workflow gets the goal done faster with less wasted effort, time, and tokens.
Prefer larger slices of work.
Examples of inefficiencies:
-Don't wait 120 seconds for a 60 second match timer.
-Don't build a ROM just to test the smallest code changes (unless its for fixing bugs).
-when testing a scene, go directly to the scene instead of waiting for a timer or a match to complete

## Current Boundary

Boundary has **two arms** since the P2-1 phase close (row P2-1g, 2026-08-18),
and `verify-all.ps1 -Profile Boundary -List` is the membership authority:

1. `p2_shell_loop` — twenty full laps of the VS shell (title → main menu → VS
   rules → character select → stage select → battle → results → START →
   character select) under scripted input, asserting per-scene-kind arena
   high-waters flat, the arena free floor, one input entry per step, the exact
   lap pattern, and no CPU abort. It is a **scene-boundary** instrument at
   `NDS_HARNESS_FAST_LOGIC=1`: no tick figure from it is a cadence figure.
2. `battle_playable_realtime`, mode `163`: Mario human versus the imported
   level-3 Fox CPU on Dream Land, items off, one-minute (`3600` tick) Time
   mode — unchanged, and the **P1 regression guard** `docs/P2_PLAN.md` law 4
   keeps green throughout P2. It stays the only gameplay/performance arm.

A diagnostic ROM may pause Fox decision/input only; proof runs and milestone
acceptance enable it. Never launch the obsolete five-minute configuration, except for specific instruction to do so.
Menu cadence and the realtime pass through the menus are measured beside the
profile, not inside it: `scripts/menus/probe-p2-shell.ps1`
(`smash64ds-p2-shell-hwtri`, fast logic 0). The Boundary definition evolves at
P2 phase closes by board row (`docs/P2_PLAN.md` law 4); this section is updated
when it does.

**Both gate arms run the one-minute match** (owner, 2026-08-05: *"the soak was
only meant to catch freezes, boundary and both cpu gates should be the 60 sec
match"*). `NDS_R2_BOTH_CPU=1` is the stress arm: same 60 s, Mario also a level-3
CPU; never published as the Boundary figure. **The soak's long match is a
separate flag** — it must not ride on the gate seed, and a soak that quietly
drops to 60 s reads NO-FREEZE having exercised almost nothing. Board has both.

## Documentation Ownership

`PROJECT_GOAL.md` owns the product contract; `docs/README.md` owns other roles.
Do not duplicate current truth. `PORTING.md` is append-only; screenshots stay in `artifacts/visibility`.
Keep documentation current and LEAN except for append only docs.
`.\docs\HANDOFF.md` should be 200 lines max (owner, 2026-07-31; was 150).

## Editing

- Use `apply_patch` for manual source and documentation edits.
- Preserve user changes and unrelated dirty-tree work.
- Prefer focused edits over whole-file replacement. Trace unfamiliar code or
  assets before deleting them.
- Remove temporary probes before handoff; keep only verified diagnostics.
- Do not add broad compatibility headers or call a stub a completed subsystem.
