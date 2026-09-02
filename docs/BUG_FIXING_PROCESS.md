# Bug Fixing Process

This document defines the shortest path from a user-reported bug in `BUGS.md`
to a verified fix. Product truth lives in `PROJECT_GOAL.md`; verifier commands
live in `VERIFYING.md`; history and status stay in their owning documents.

## Core rules

1. **BattleShip is the behavior oracle.** Inspect the relevant source before
   changing gameplay, rendering, VFX, audio, timing, or scene behavior. Convert
   subjective reports into measurable expected values whenever possible.
2. **Find the first wrong value, then fix its owning seam.** Do not patch the
   visible symptom with arbitrary offsets, frame checks, duplicated state, or
   bug-specific exceptions.
3. **Use the cheapest useful evidence first.** Prefer source, CodeGraph, static
   checks, existing captures, GDB, and the existing ROM before creating a new
   instrumented build.
4. **Build with a prediction.** Do not build a ROM merely to see whether a
   change looks right. A build should confirm a written expectation or collect
   specific missing evidence.
5. **Final proof uses the natural shipping path.** Diagnostics may accelerate
   reproduction or temporarily use reference/interpreter paths, but closure
   must exercise the real path and shipping configuration.
6. **Native DS runtime is the endpoint.** When a bug touches rendering, VFX,
   fighter/stage geometry, materials, animation presentation, or another area
   with an applicable native path, the finished fix must end in the DS-native
   renderer/runtime path. The generic/source-exact renderer may be used as an
   oracle or temporary diagnostic, but a fix that still routes its final result
   through the generic renderer is intermediate, not complete. Build tooling
   may stay generic; runtime specialization is preferred.

## What "fixed" means

A fix follows one chain:

> symptom -> source contract -> first measured divergence -> root-cause fix ->
> natural-path proof -> widest relevant verifier -> owner acceptance when needed

Compilation, a packed asset, one good frame, a non-zero counter, or failure to
reproduce is not closure. Disabling, bypassing, hiding, or substituting the
broken behavior is containment, not a fix.

Correct behavior matters more than matching the N64 implementation. Generated,
precomputed, specialized, and DS-native implementations are encouraged when
they preserve the required observable behavior and performance.

A correctness fix that misses the applicable performance gate remains
intermediate until the gate is restored.

## Workflow

### 1. Record the symptom

Capture only the facts needed to reproduce and measure it:

- exact ROM/build and relevant dirty paths;
- scene, fighter/object state, action/input, and preceding event;
- expected vs observed behavior;
- frequency and shortest known trigger;
- existing screenshot, capture, log, or owner description.

Preserve the user's wording and uncertainty. Reuse a valid failing capture
instead of reproducing it again without purpose.

### 2. Derive the observable contract

Read the relevant BattleShip code and write the values that define correct
behavior. A contract is complete when meeting it makes the reported symptom
impossible.

Typical visual quantities: attachment/joint, world and screen position, scale,
geometry, texture/frame, color, blend, motion, spawn timing, lifetime, layer.

Typical audio quantities: cue, volume, pitch/rate, duration, envelope, pan,
timing, stop reason, and mix behavior.

Use source constants/tables first, then host-side fixtures or asset-pipeline
inputs where useful. A generic/source-exact interpreter may be enabled
temporarily as an oracle, but it is not the desired shipping renderer.

Owner-approved presentation deltas override raw source; cite the recorded
approval instead of "fixing" an intentional difference.

### 3. Localize the first divergence

Walk the full chain and compare actual values to the contract. The first wrong
value identifies the owning seam.

Examples:

> VFX: trigger -> effect args -> transform/joint -> asset -> update -> native draw -> pixels
>
> Audio: trigger -> cue id -> pack -> channel -> mix -> PCM

For freezes/corruption, classify the stopped guest before changing code:
allocator spin, display-list/GX failure, abort, IRQ/wait state, or merely a slow
live frame require different fixes.

Every diagnostic needs an engagement count or positive control so that zero
means "measured zero," not "the probe never ran."

### 4. Fix the owning seam

Make the smallest mechanically correct change shared by the affected callers.
Check sibling paths before editing.

- Reuse or extend the existing DS-native owner when possible.
- For renderer bugs, move the corrected behavior into the native renderer/path
  when applicable; do not leave the generic renderer in the final shipping
  route merely because it is easier to make correct.
- Do not add arbitrary offsets, frame-specific hacks, synthetic input,
  proof-only branches, or duplicate paths that hide a shared defect.
- Keep DS/backend behavior under `src/nds` or `src/port` and compatibility
  declarations under `include`.
- Preserve unrelated dirty work.
- Remove temporary probes after proof unless they are durable regression
  checks with their own runnable validation.

If the bug reveals a repeatable workflow/tooling failure, improve the owning
checker, helper, or document in the same scoped change when safe.

### 5. Prove the candidate

Before calling it fixed:

1. Every contract value is green on the exact candidate and natural path.
2. At least one affected sibling/adjacent path is checked.
3. Rendering fixes are confirmed to use the intended native runtime path when
   applicable, with no accidental fallback through the generic renderer.
4. Run the widest relevant verifier from `VERIFYING.md` on the shipping
   configuration.
5. Store permanent visual/performance evidence when required by repository
   policy.
6. If active-frame cost changed or pacing regressed, run a matched performance
   A/B and verify the applicable tick/VBlank gate.

Unexplained flashes, corruption, missing content, state differences,
nondeterminism, or verifier contradictions keep the bug open.

### 6. Owner acceptance only when needed

Ask for subjective visual/audio confirmation only after the measurable contract
is green. State what the owner should see/hear and provide the relevant capture
or audio evidence.

If the owner rejects a source-backed candidate, treat that as a missing contract
dimension: identify and measure the new dimension instead of blindly iterating.

Batch related bugs that share a subsystem, build, capture, or acceptance pass.
Do not spend one ROM build or owner round-trip per row when one batch can prove
several.

## Evidence order

Escalate only as needed:

1. BattleShip/source + existing artifacts + CodeGraph.
2. Static/AOT/host-side checks.
3. Existing ROM with GDB/captures.
4. One batched instrumented build for unresolved measurements.
5. Fix-candidate build.
6. Widest relevant verifier and owner acceptance if subjective confirmation is
   still required.

Run the shortest event that reaches the bug. Do not wait through a full match
for a trigger reachable in seconds, rerun unchanged ROMs without a question to
answer, or require long soaks for ordinary fixes.

## `BUGS.md` stays lean

`BUGS.md` is the user-facing queue, not the investigation log.

- Preserve the user's symptom wording and ordering.
- Do not silently delete, merge, split, or narrow reports.
- Mark a row `**FIXED** (YYYY-MM-DD)` only after verified closure.
- Use `**PARTLY FIXED**` only when an independently verifiable portion is
  closed and the remaining symptom is explicit.
- Keep investigation details in the working notes/evidence, not the queue.

## Priority

1. Freeze, crash, corruption, nondeterminism, or data loss.
2. Gameplay, input, collision, state, timing, or scene-flow defects.
3. Missing/wrong telegraphs, VFX, SFX, camera, HUD, or results.
4. Cosmetic or acoustic mismatch without gameplay meaning.
5. Tooling defects that invalidate evidence for the above.

Within a class, prefer the shared root cause that closes several bugs. A
performance regression introduced by a fix belongs to that fix.

## Close honestly

Mark `FIXED` only when the demonstrated root cause is corrected on the natural
shipping path, the relevant contract and verifier pass, native rendering owns
the final result where applicable, performance remains acceptable, temporary
diagnostics are cleaned up, and any required subjective acceptance is complete.

After verified progress, follow the repository's commit/snapshot policy.

## Minimal bug work note

```text
Bug: <verbatim report>
Candidate: <ROM/build + relevant dirty paths>
Trigger: <shortest natural trigger>
Expected: <source-backed observable values>
Measured divergence: <first wrong value + owning seam>
Fix: <root-cause change; native runtime/render path if applicable>
Proof: <contract result + sibling check + widest verifier>
Evidence: <artifact paths if required>
Owner verdict: <not needed | pending | PASS | FAIL(dimension)>
Remaining: <none or one explicit open dimension>
```

## Anti-patterns

- Building a ROM just to "see if it looks right."
- Fixing the visible symptom instead of the first wrong value.
- Calling a generic/interpreter renderer result the final renderer fix when a
  native path should own it.
- Verifying a mechanism but not the user's observable symptom.
- Sending a candidate for owner review before you can predict its result.
- Stacking speculative patches instead of re-localizing after a failed theory.
- Waiting through long scenarios for a short trigger.
- Re-proving unchanged green behavior without a reason.
