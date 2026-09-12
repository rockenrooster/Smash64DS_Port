# Bug Fixing Process

`PROJECT_GOAL.md` owns the product contract; `docs/VERIFYING.md` owns verification commands. `docs/BUGS.md` is the owner queue; `docs/p2/BUG_NOTES.md` holds investigations and evidence links.

## Hard rules

**Every built ROM is native-only**, including debug, bring-up and profiling (`P2_PLAN.md`, law 8). Exclude generic renderers/software scene compositors from build inputs and linked binaries; verify before packaging. Reference rendering stays host-side. No game-content or target exceptions.

**Native rejection is a failure.** Implement the missing native capability; never hide, skip, disable, or substitute required content to satisfy a check. Making a ROM halt instead of falling back is containment, not a completed fix.

**Preserve observable behavior, not the N64 implementation.** Generated, precomputed, specialized DS implementations are encouraged. Treat `decomp/` as read-only; honor documented owner-approved changes.

## Workflow

### 1. Record the symptom

Preserve the owner's wording and uncertainty. Record ROM hash/build/configuration, relevant dirty paths, scene/object state, input/preceding event, expected versus observed behavior, frequency and shortest trigger. Reuse valid failing evidence; reproduce only to answer a missing question.

### 2. Derive the observable contract

Trace the relevant BattleShip constructor/move through descriptors, assets and actual consumers before editing. Bound the complete feature: children, reachable sibling states, model-part/joint bindings, materials and resource lifetimes. Derive test expectations independently from source—not by repeating the generator's tables. Define observations covering the entire symptom.

Visual checks include joint/attachment, world/screen position, scale, geometry, texture/frame, color/alpha/blend, motion, spawn timing, lifetime, and layer. Audio checks include cue, volume, pitch/rate, duration, envelope, pan, timing, stop reason, and mix behavior. Cite owner-approved presentation deltas instead of undoing them.

### 3. Find the first divergence

Compare actual values against the contract along the full chain:

> VFX: trigger → arguments → joint/transform → asset → update → native draw → pixels
>
> Audio: trigger → cue → pack → channel → mix → PCM

Use source/CodeGraph and existing artifacts, then static/AOT/host checks, then an existing ROM; instrument/build only for an unresolved prediction.

Collect distinct failures with bounded existing diagnostics when safe; a first-failure latch is not a coverage census. Keep its hard failure verdict, report collection limits, and stop on unsafe execution. Any missing collector/checker capability is implementation work under the existing row, not something a doc update supplies.

For hangs/corruption, distinguish allocator spin, GX/display-list failure, guest abort, IRQ/wait state and a slow live frame. Require positive engagement; unexercised zero counters prove nothing. Re-localize failed theories; no speculative patch stacks or closed-case reruns without new evidence.

### 4. Fix the owning seam

Repair the owning seam and affected siblings; reuse existing native owners. No arbitrary offsets, frame-specific hacks, duplicated state, synthetic-input fixes or proof-only production branches.

Keep DS/backend behavior in `src/nds` or `src/port`, compatibility declarations in `include`, and preserve unrelated dirty work. Correct repeatable checker/tooling/workflow defects in the same scoped change when safe; otherwise record the actionable follow-up.

### 5. Prove the candidate

Follow `VERIFYING.md`: cheap focused checks while editing, then one widest relevant verifier per coherent batch/configuration. Combine compatible evidence collection without dropping required per-unit, sibling or configuration coverage. Stable inputs and reusable proofs prevent duplicate runs, not necessary acceptance.

On the exact candidate's **natural shipping path**, prove engagement, required pixels/audio, source contract, resource safety, native-only enforcement and applicable cadence. Changed active-frame cost/pacing needs matched performance evidence; honor owner optimization deferrals without declaring final performance accepted.

Store permanent visual/performance evidence as repository policy requires. Remove temporary probes unless retained as runnable, validated regression checks.

### 6. Obtain owner acceptance and close

Request required subjective acceptance after measurable checks pass, with captures/audio. Owner rejection identifies an unresolved dimension: measure it and keep the symptom open.

**FIXED requires root-cause repair, full natural-path/sibling proof, required verifier, native-only output, acceptable performance, probe cleanup and required owner acceptance.** Imports, compilation, admission, triangle counts, an unengaged zero, no-op owner or one good frame are not completion. Unexplained corruption, missing output, nondeterminism or contradictory evidence keeps the bug open.

Distinguish implemented candidate, independently verified portion and accepted fix. Main owns remaining integration/verification even when a worker only implements. A shared blocker leaves dependent acceptance open, not independent correct work. Commit reproducible progress per `VERIFYING.md`; preserve explicit deferrals.

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