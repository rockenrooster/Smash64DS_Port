# Bug Fixing Process

`PROJECT_GOAL.md` owns the product contract; `docs/VERIFYING.md` owns verification commands. `docs/BUGS.md` is the owner queue; `docs/p2/BUG_NOTES.md` holds investigations and evidence links.

## Hard rules

**Every built ROM is native-only, including debug and profiling builds.** Exclude generic renderers and software scene compositors from ROM build inputs and linked binaries; verify before packaging. No fallback switches or target exceptions. Reference rendering and generic build tooling stay host-side. This covers fighters (including Mario/Fox), stages, actors, effects, particles, UI, and menus.

**Native rejection is a failure.** Implement the missing native capability; never hide, skip, disable, or substitute required content to satisfy a check. Making a ROM halt instead of falling back is containment, not a completed fix.

**Preserve observable behavior, not the N64 implementation.** Generated, precomputed, specialized DS implementations are encouraged. Treat `decomp/` as read-only; honor documented owner-approved changes.

## Workflow

### 1. Record the symptom

Preserve the owner's wording, uncertainty, and latest observations. Record exact ROM hash/build/configuration, relevant dirty paths, scene/object state, input and preceding event, expected versus observed behavior, frequency, and shortest trigger. Reuse valid failing captures, logs, or owner descriptions instead of reproducing without purpose.

### 2. Derive the observable contract

Read relevant BattleShip source, constants/tables, and assets before changing behavior. Define measurable values that make the **entire reported symptom impossible**, not merely prove an internal mechanism works.

Visual checks include joint/attachment, world/screen position, scale, geometry, texture/frame, color/alpha/blend, motion, spawn timing, lifetime, and layer. Audio checks include cue, volume, pitch/rate, duration, envelope, pan, timing, stop reason, and mix behavior. Cite owner-approved presentation deltas instead of undoing them.

### 3. Find the first divergence

Compare actual values against the contract along the full chain:

> VFX: trigger → arguments → joint/transform → asset → update → native draw → pixels
>
> Audio: trigger → cue → pack → channel → mix → PCM

Use the cheapest useful evidence first: source/CodeGraph/existing artifacts → static/AOT/host tests → existing ROM with GDB/captures → one batched instrumented build only for unresolved measurements.

For hangs/corruption, distinguish allocator spin, display-list/GX failure, guest abort, IRQ/wait state, and a slow live frame before editing. Every diagnostic needs an engagement count or positive control: zero from an unexercised probe proves nothing. Re-localize after a failed theory; do not stack speculative patches or repeat refuted theories without new evidence.

### 4. Fix the owning seam

Make the smallest mechanically correct repair shared by affected callers; inspect sibling paths first. Reuse or extend existing native owners. No arbitrary offsets, frame-specific hacks, duplicated state, synthetic-input fixes, or proof-only production branches.

Keep DS/backend behavior in `src/nds` or `src/port`, compatibility declarations in `include`, and preserve unrelated dirty work. Correct repeatable checker/tooling/workflow defects in the same scoped change when safe; otherwise record the actionable follow-up.

### 5. Prove the candidate

Build only to test a written prediction or obtain specific missing evidence. Batch related fixes, builds, captures, and acceptance passes; keep build inputs stable during builds/verifiers. Run the shortest useful trigger, not an unnecessary full match or soak. Do not rerun unchanged green checks without a reason.

On the **exact candidate and natural shipping path**, prove every contract value, required visible content, native-only enforcement, and at least one affected sibling/adjacent path. Run the widest relevant verifier from `docs/VERIFYING.md` on the shipping configuration. If active-frame cost changes or pacing regresses, run matched performance A/B and verify applicable tick/VBlank gates; a performance regression belongs to the fix.

Store permanent visual/performance evidence as repository policy requires. Remove temporary probes unless retained as runnable, validated regression checks.

### 6. Obtain owner acceptance and close

Request subjective visual/audio acceptance only after measurable checks pass; provide captures/audio and the predicted result. Owner rejection means a missing contract dimension: measure it and keep the symptom open, rather than blindly iterating.

**FIXED requires the corrected root cause, full natural-path proof, sibling check, relevant verifier, native-only rendering, acceptable performance, probe cleanup, and any required owner acceptance.** Compilation, asset presence, admission, triangle counts, one good frame, or failure to reproduce are not closure. Unexplained flashes, corruption, missing content, state differences, nondeterminism, or contradictory evidence keep the bug open.

Commit/publish verified progress under current repository policy; honor explicit pauses and snapshot instructions.

## Priority and reporting

Follow explicit owner priorities. Otherwise: freezes/crashes/corruption/nondeterminism/data loss → gameplay/input/collision/state/timing/flow → telegraphs/VFX/SFX/camera/HUD/results → cosmetic/acoustic mismatch → tooling defects invalidating that evidence. Prefer shared root causes within each class. Non-native rendering is always a failure, never an accepted compromise.

Preserve report wording/order in `docs/BUGS.md`; never silently delete, merge, split, or narrow reports. All agent annotations must be **bold** and at most **20 words**. Use `**FIXED (YYYY-MM-DD)**` only after closure; `**PARTLY FIXED: ...**` only for an independently verified portion with the remainder explicit. Otherwise state the unresolved issue and next action. Keep details in `docs/p2/BUG_NOTES.md`:

```text
Bug: <verbatim report>
Candidate: <ROM hash/build/configuration + relevant dirty paths>
Trigger: <shortest natural trigger>
Expected: <source-backed values / approved delta>
Divergence: <first measured wrong value + owning seam>
Fix: <root-cause change; native path>
Proof: <contract + sibling + verifier + performance>
Evidence: <artifact paths>
Owner: <not needed | pending | PASS | FAIL: dimension>
Remaining: <none or explicit open requirement>
```