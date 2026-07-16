# AGENTS.md

## Mission

This repo is a Nintendo DS source port of the BattleShip Smash 64 decompilation:

```text
Original Smash 64 decomp game code + Nintendo DS backend = playable port
```

Do not turn it into a handwritten Smash clone or DS-native gameplay rewrite.

## Hard Rules

- Treat `decomp/` as read-only reference source. Never edit it.
- Inspect relevant BattleShip source before changing gameplay or renderer behavior.
- Inspect `decomp/sm64-nds` before DS backend architecture changes.
- Prefer coherent original translation-unit imports under `src/import`.
- Put DS/backend behavior under `src/nds` or `src/port`; compatibility declarations
  belong under `include`.
- Graduate imported subsystems live. Do not add proof-only branch reruns,
  one-bit proof masks, or permanent seed/restore wrappers.
- Migrate or delete obsolete bounded modes when natural runtime replaces them.
- New harness modes are only for scene-level capabilities.
- Fix root causes at the shared seam. Do not hide symptoms with arbitrary
  offsets/constants, duplicated state, frame checks, or asset-specific hacks.
- Treat flashes, corruption, nondeterminism, hangs, and unexplained state
  differences as failures.
- Respect DS CPU, RAM, VRAM, bandwidth, alignment, fixed-point, and graphics
  limits.
- Treat generated outputs and emulator payloads as generated; never hand-edit them.
- Publish exactly `smash64ds.nds` and
  `smash64ds-battle-playable-hwtri.nds`; all lab outputs stay in `builds/`.
- User-facing ROMs must be verifier-covered configurations.
- Use only repo-local scripted melonDS. Do not commit runner configs, binaries,
  logs, or shard artifacts.
- Run `scripts/New-Smash64DSSnapshot.ps1 -Mode Lean` after verified progress as
  the final project command. Run nothing after it.

## DS Visual Fidelity

Gameplay, collision, rules, state, camera meaning, and flow stay source-faithful.
Presentation targets roughly 90% overall likeness, not pixel identity. Timebox
cosmetic exactness to one measured experiment; on a tick, memory, or P1 miss,
keep the cheapest recognizable source-derived approximation and move on.
Prefer still frames, reduced cadence, simpler geometry/layout, and DS-native
effects. Record the source, visible delta, measured reason, and screenshot under
`artifacts/visibility`. Never approximate gameplay semantics or telegraphs and
never accept missing or corrupt presentation. Dream Land water is frozen at
exact source frame 0 on the original geometry; its animated replacement is gone.

## Operating Model

Start each cycle with:

```powershell
.\scripts\verify-all.ps1 -Profile Boundary -List
git status --short
```

Then read `docs/P1_EXECUTION_BOARD.md` and `docs/HANDOFF.md`. The board is the
only dynamic queue; handoff contains only the restart surface. Select its
highest-impact unowned red P1 row.

Preserve a known-good checkpoint before risky changes. On regression, identify
the first bad change before layering another fix. Trace dependencies before
changing shared renderer, object, animation, memory, or state code.

For performance iteration, use one synchronized eight-frame A/B comparison on
an identical ROM/configuration/window. Primary evidence is ticks, FPS, a dated
screenshot, and automated screenshot analysis; semantic/state/geometry counters
are cheap correctness guards. Stop on a decisive KEEP or REVERT. Run a third A
only when A/B is noisy, near its gate, surprising, or internally inconsistent.
Do not require routine A/B/A, 32-frame, or 128-frame promotion runs.

Use the smallest focused checker or benchmark while editing. Run one widest
relevant verifier for a kept checkpoint: Boundary for battle-only work, or
Current instead when normal/shared startup is affected. Do not stack DevFast,
Boundary, and Current when they cover the same runtime. The registry exposes
only Latest and Boundary; the retired diagnostic fleet must not return.

Subagent switch: **OFF**.

- `OFF`: let already-running subagents finish, but do not spawn, follow up, or
  reassign one until the user explicitly switches this back to `ON`.
- `ON`: when work cleanly parallelizes, keep up to three helper agents on
  bounded, disjoint lanes; do not manufacture work merely to fill slots.

Prefer deletion, existing helpers, fixed DS hardware paths, and the smallest
source-backed implementation. Equivalent output from less code or fewer ticks
wins. Do not add speculative abstractions, selectors, caches, or tooling.

Milestones cover natural initialization, gameplay, transitions, cleanup, and
repeat execution; compilation or one good frame is not completion.

## Current Boundary

Canonical Boundary is `battle_playable_realtime`, mode `163`: Mario human versus
the imported level-3 Fox CPU on Dream Land, items off, one-minute (`3600` tick)
Time mode. The public ROM temporarily pauses Fox decision/input only; proof runs
enable it. Never launch the obsolete five-minute configuration.

## Documentation Ownership

- `P1_EXECUTION_BOARD.md`: dynamic state, artifact identity, blockers, dates.
- `HANDOFF.md`: exact restart command and next packet only.
- `VERIFYING.md`: iteration, A/B, verifier, emulator, capture, snapshot policy.
- `HARNESSES.md`: registry and harness naming only.
- `ARCHITECTURE.md`: stable ownership and component boundaries.
- `KNOWN_ISSUES.md`: unresolved durable gaps only.
- `optimization/NATIVE_RENDERER_PLAN.md`: current M2-M4 implementation contract.
- `PERF_LEDGER.md`: measurements and rejected experiments.
- `DIAGNOSTIC_REFERENCE.md`: marker definitions and manual diagnostics.
- `PORTING.md`: append-only history; do not rewrite or use as a planning surface.

Do not duplicate current hashes, milestone histories, or command stacks across
active docs. Put screenshots only under `artifacts/visibility`.

## Editing

- Use `apply_patch` for manual source and documentation edits.
- Preserve user changes and unrelated dirty-tree work.
- Prefer focused edits over whole-file replacement. Trace unfamiliar code or
  assets before deleting them.
- Remove temporary probes before handoff; keep only verified diagnostics.
- Do not add broad compatibility headers or call a stub a completed subsystem.
