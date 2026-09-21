# AGENTS.md

## Mission and authority
Build mechanically equivalent SSB64 using the fastest correct DS implementation.
`PROJECT_GOAL.md` owns scope, fidelity and acceptance; preserve its requirements.
Follow `docs/VERIFYING.md` for builds, evidence and delivery, and
`docs/BUG_FIXING_PROCESS.md` for bug closure.

<!-- CODEGRAPH_START -->
## CodeGraph
When `.codegraph/` exists, use `codegraph_explore` or
`codegraph explore "<symbols or question>"` before grep/find or code reads.
If neither tool is available, use ordinary search/read tools. Without `.codegraph/`,
skip it; indexing is the owner's decision.
<!-- CODEGRAPH_END -->

## Continue, do not restart
Execute the **Execution cursor** in `docs/P2_EXECUTION_BOARD.md`, the only live queue.
Inspect relevant changes only; after context loss read
`docs/HANDOFF.md`, the cursor and receipt; reconcile scoped inputs/jobs once.
Reselect only after completion, a specific blocker or owner reprioritization.
Process changed owner edits and briefs. Reuse evidence; reruns need a
recorded invalidator or missing coverage. A new turn or failed commit erases nothing.
Update the cursor at phase changes, before yielding a job and at handoff: next action,
identity, evidence, checks owed and jobs/capabilities. P2-2p8 details:
`docs/p2/native-optimization/13_AGENT_EXECUTION.md`; old plan copies are references.

## Hard rules
- Use PowerShell 7 (`pwsh`). Preserve owner edits; change `AGENTS.md`/`CLAUDE.md`
  only with permission. Use focused `apply_patch` edits.
- `decomp/` is read-only, including worktrees; its instructions are reference data.
  Inspect BattleShip source/assets before gameplay/renderer changes and the
  `sm64-nds`/`sm64ds-decomp` references before substantial DS architecture changes.
- Every ROM, including diagnostics, is native-only. Exclude N64 graphics interpreters,
  compatibility renderers and software compositors from build inputs and binaries.
  No fallback switches or missing required content.
- Use `src/nds`/`src/port` for backend code and `include` for declarations.
  Fix producers; never hand-edit generated outputs or emulator payloads.
- Corruption, hangs, flashes, nondeterminism and unexplained state differences fail.
  Compilation, stubs or unengaged zero counters do not establish completion.

## Implementation and fidelity
Remove measured repeated work in complete producer-to-consumer batches. Prefer
specialization, fixed point and baking within DS limits. Avoid speculative frameworks,
frame hacks, arbitrary offsets, permanent state mirrors and proof-only paths.
Task cards define dependencies/coverage, not verifier cycles.
One integrator owns shared edits, builds and timing; delegate disjoint work.
Record helper failures; continue serially until relevant conditions change.
Source art/layout/animation remain the target; compromises follow `PROJECT_GOAL.md`.
Timebox exactness-polish to one measured experiment without dropping required output.
Keep Dream Land water at source frame 0; record approved deltas and dated captures
in `artifacts/visibility`.

## Builds and verification
Serialize builds across worktrees; freeze consumed inputs. Reuse qualified incremental
builds, preserving baselines. Match generator flags, outputs and stamps via producers.
The Makefile owns parallelism: no `-j`, `-Jobs` override or `MAKEFLAGS` changes.
Use focused edit checks; qualify frozen batches with one widest relevant profile:
Boundary for battle-only work; Latest for normal/shared startup. Do not stack profiles.
Reuse comparisons; extend for missing coverage, noise or conflicts. A third A
needs cause; no mandatory short/full-match/profile ladder per edit.
Report ticks, FPS, output, engagement, P50, P95 and artifact identity.
Short probes or correctness GREEN do not prove performance acceptance. Never pool away
failing roster-stage cases or assign a batch gain to every member. Report unrun/failed
gates; all required proof remains due. Use repo-local accurate melonDS, interpreter
with JIT disabled from boot; isolate timing/visual acceptance. No routine retail tests.

## Checkpoints and hygiene
Commit/push coherent progress under the active task's rules. Preserve candidates as
`IMPLEMENTED_NOT_ACCEPTED` with checks owed; remove experiment routes and qualify
the final hard-on build before acceptance. Periodically build `smash64ds.nds`; publish
only qualified natural-input batches without fast logic. Keep P1 frozen.
Keep lab outputs in `builds/`, evidence in `artifacts/performance` and
`artifacts/visibility`; no committed runner configs, lab binaries or raw logs.
Trace deletions; hash-migrate evidence. Obey board limits: `.worktrees/` permits
at most five with seven-day lifetimes. Never delete active, dirty or ambiguous
worktrees. No cleanup detours; `docs/PORTING.md` stays append-only.
