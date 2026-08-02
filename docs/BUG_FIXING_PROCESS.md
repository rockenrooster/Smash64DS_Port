# Bug Fixing Process

This document owns the process for taking a user-reported playtest bug in
`BUGS.md` from symptom to verified closure. It does not own current bug status,
product truth, verifier commands, or project history; those stay in their
existing owning documents.

Every `BUGS.md` row blocks P1 unless the owner explicitly changes its scope.

## What "fixed" means

A fix must establish one continuous proof chain:

> reported symptom -> source contract -> earliest wrong seam -> root-cause fix
> -> natural runtime proof -> regression gate -> owner acceptance when subjective

Compilation, a packed asset, a non-zero counter, one good frame, or failure to
reproduce is not closure. The exact candidate that passes must be the candidate
being offered to the owner.

Disabling the affected feature, avoiding its trigger, or substituting unrelated
content is containment, not a fix.

"1:1" means correct observable source behavior, not identical N64 code or
architecture. Inspect BattleShip first and make the first implementation as
source-faithful as practical. The implementation may be generated, specialized,
or direct DS-native code when it preserves the same behavior; do not build a
known-slow software renderer or interpreter when the correct direct path is
already clear. A source-exact software/interpreter arm may still serve as a
temporary oracle.

An oracle or first implementation is intermediate if it pushes the scene's
representative P95 above its applicable budget or materially worsens the
2/3/4/5+ VBlank histogram or maximum interval. Iterate toward a direct DS-native
path before marking the bug fixed. Record "optimization needed" as non-blocking
debt only after the candidate meets the current performance gate.

## Keep `BUGS.md` lean

`BUGS.md` is the user-facing queue, not an investigation log.

- Preserve the user's symptom wording and ordering. Do not silently delete,
  merge, split, or narrow a report.
- Prefix a fully closed row with `**FIXED** (YYYY-MM-DD)`.
- Use `**PARTLY FIXED** (YYYY-MM-DD)` only when an independently verified part
  is closed and the remaining visible symptom is named.
- For any row that is not fixed, append at most a 20-word current finding or
  next step. Put evidence and reasoning in the owning documents below.
- An umbrella report is not closed by one representative case. Track every
  named case in the work packet and prove each one naturally.
- A shared root cause may fix several rows, but each user-visible row keeps its
  own acceptance proof.

## Priority

Fix the highest class first; within a class, prefer the shared owner that can
close several reports without weakening any contract.

1. Freeze, crash, corruption, nondeterminism, or data loss.
2. Gameplay, input, collision, state, timing, or scene-flow defects.
3. Missing or wrong gameplay telegraphs, VFX, SFX, camera, HUD, or results.
4. Cosmetic or acoustic mismatch that does not change gameplay meaning.
5. Tooling defects that make evidence from classes 1-4 unreliable.

Correctness bugs outrank optimization unless the owner explicitly changes the
order. A performance regression introduced by a proposed fix remains part of
that fix. A tooling defect that invalidates the current proof moves ahead of
the bug that depends on that proof.

## Verification economy

Use the cheapest evidence capable of falsifying the current claim. Expensive
proof happens once, when promoting the final candidate.

- Diagnose from source, existing artifacts, and current runtime evidence before
  building. Reuse a valid existing failing capture instead of reproducing it.
- Do not build a ROM for every theory. Build only after one hypothesis predicts
  a specific result, and prefer the smallest focused checker when it can answer.
- During iteration, run the shortest targeted event. Do not wait through a full
  match when the bug can be reached or captured earlier without changing it.
- Do not repeat an unchanged ROM or require routine A/B/A, long soaks, or
  32/128-frame samples. Escalate only when the first result is inconclusive.
- Run a performance A/B only when active-frame work changed, the mechanism is
  plausibly hot, or pacing regressed. Start with the synchronized eight-frame
  comparison owned by `VERIFYING.md`.
- Run one widest relevant verifier only after deciding to keep the candidate;
  do not stack equivalent profiles or rerun an already-passing unchanged ROM.
- An agent may pass named objective source, counter, image-analysis, and verifier
  gates. Batch related owner visual/listen/play checks into one acceptance run.
- Escalate to longer, broader, or retail-hardware proof only for conflicting
  evidence, intermittent failures, release qualification, or device-specific
  mechanisms.

## The workflow

### 1. Capture the report without interpreting it

Record the observable facts before proposing a cause:

- exact ROM/candidate identity, branch, dirty paths, and emulator/device setup;
- scene, fighter states, action, input, match time, and preceding event;
- expected result, observed result, frequency, and shortest known trigger;
- screenshot, video, audio note, freeze capture, or owner description;
- whether the report is deterministic, intermittent, or not yet reproduced.

The user's description is evidence of a symptom, not a diagnosis. Preserve
uncertainty such as "sounds too loud" until the audio path is measured and the
owner listens to the candidate.

### 2. Reproduce the failure on a known configuration

Start with the verifier-covered configuration that exposes the real user path.
Use an existing exact-candidate failure when it is sufficient; otherwise use the
shortest natural trigger and capture one failing control before editing.

- Hold ROM flags, emulator settings, inputs, and observation window constant.
- Confirm that the guest reached the state under test; host title-bar motion or
  a changing window hash is not guest liveness.
- For intermittent bugs, record attempts and failures over a bounded exposure
  instead of reporting only the successful attempt.
- A diagnostic may accelerate a natural event, but it may not change the
  behavior being judged. Final proof must exercise the natural path.
- If the bug is not reproduced, do not guess a fix. Improve the observation
  surface or obtain the missing trigger/candidate details.

### 3. Define the source contract

Before changing behavior, inspect the relevant BattleShip source and write down
the exact expected event, state transition, timing, cue, effect, asset, branch,
or cleanup behavior. Use CodeGraph first to trace the current port path.

For substantial DS backend, renderer, memory, asset, or hardware changes, also
inspect the comparable `sm64-nds` and `sm64ds-decomp` designs. `decomp/` remains
read-only.

Write four short statements in the work packet:

1. **Expected:** what BattleShip requires.
2. **Observed:** what this exact candidate does.
3. **Earliest divergence:** the first seam where they differ.
4. **Acceptance:** the observation that would falsify the proposed fix.

### 4. Trace to the owning seam

Follow the whole path rather than patching the last visible symptom:

> trigger -> source state/event -> data or asset resolution -> runtime object or
> channel -> backend submission -> visible/audible result -> cleanup

Fix the earliest incorrect step shared by all affected callers. Check every
caller and sibling path before editing the owner.

Common classifications:

| Symptom | First evidence to collect |
|---|---|
| Wrong gameplay or scene flow | Source state transition, live status/input trace, first differing state |
| Missing or wrong VFX | Trigger/effect ID, create/drop, asset/bank resolve, update, draw submission, client pixels |
| Missing or wrong audio | Trigger/cue ID, pack lookup, enqueue/channel result, duration/stop reason, audible output |
| Freeze or corruption | Guest frame/counter, PC and instruction, stack, allocator/DL/GFX/abort counters |
| Performance regression | Same-tree matched A/B, engagement, typed ticks, P50/P95, VBlank histogram and maximum |
| Harness or detector defect | Known-bad positive control, expected artifact, sample/hit count, guest-owned signal |

For a freeze, classify the stopped guest before naming an owner. An allocator
spin, display-list overflow, data abort, IRQ/wait state, and a slow live frame
need different fixes even when the window looks equally frozen.

### 5. Run one falsifiable experiment

State one hypothesis and its predicted observation before changing code. Use
existing counters, checkers, captures, and scripts before adding a probe.

- Change one causal variable at a time. Use a matched control when attribution,
  performance, or visible-output comparison requires it; do not manufacture one
  for an objective focused check with an existing failing control.
- Define KEEP, REVERT, and inconclusive outcomes before reading the result.
- Every probe needs an engagement count or positive control. Zero must mean
  "measured zero," not "the hook never ran" or "the linker removed it."
- Verify debugger symbols exist in the candidate ELF before adding capture
  commands; one missing GDB symbol can discard the rest of the batch.
- Check that the intervention happened independently of the result counters.
- When two plausible hypotheses fail, re-trace the flow instead of stacking a
  third speculative patch.

Follow `optimization/TASK_STANDING_RULES.md` for measurement-specific traps and
time boxes.

### 6. Fix the root cause once

Make the smallest mechanically correct change at the owning seam.

- Reuse the existing source-backed or DS-native owner before adding another
  path, cache, selector, or abstraction.
- Do not hide a shared defect with a frame check, arbitrary offset, synthetic
  input, bug-specific effect, or permanent proof-only branch.
- Keep port behavior under `src/nds` or `src/port`; compatibility declarations
  belong under `include`.
- Preserve unrelated dirty work and reverse only the experiment's own hunks if
  it is rejected.
- Remove temporary probes. Keep a diagnostic only when it prevents recurrence
  and has its own runnable check.
- If the bug exposed a repeatable workflow failure, improve the existing helper,
  checker, or owning document in the same scoped change when safe.

### 7. Prove the candidate

Use the least work that can falsify the fix, then one widest relevant verifier.
`VERIFYING.md` owns the current commands and profile choice; do not copy stale
command sequences into bug reports.

The final proof packet contains only the applicable items:

1. the focused check or deterministic reproducer;
2. the natural user path on the exact candidate;
3. the expected state/output and the absence of the original failure;
4. one adjacent or sibling path that could regress from the same change;
5. the widest relevant verifier in the configuration that will ship;
6. memory, pacing, cleanup, or drop guards relevant to the changed subsystem.

Additional acceptance is mandatory by domain:

- **Gameplay:** source-equivalent state and timing evidence plus verifier
  coverage and an owner play test when feel is involved.
- **Visuals:** source-derived presentation, trigger/create/draw evidence, and a
  synchronized control/candidate capture in `artifacts/visibility`. An agent may
  pass the objective image gates; batch final subjective approval with the
  owner's next visual check.
- **Audio:** event-to-cue-to-channel evidence, no unexplained drop/underrun, and
  owner listen approval. A packed cue alone is not audible proof.
- **Freeze/memory:** a classified failing capture before the fix, a bounded
  stress run after it, and the relevant reserve/overflow counters remaining
  healthy. A changing picture alone is not liveness.
- **Performance, when required by the verification-economy rule:** a matched
  same-configuration A/B with engagement, typed P50/P95, the 2/3/4/5+ VBlank
  histogram, maximum interval, and correctness guards. Use the accuracy-focused
  custom melonDS fork unless the mechanism is device-specific.

Treat unexplained flashes, corruption, state differences, missing content,
nondeterminism, or verifier contradictions as failures even if the original
symptom disappeared.

### 8. Record evidence in its owner

Do not turn `BUGS.md` into a second history or evidence ledger.

| Owner | What belongs there |
|---|---|
| `BUGS.md` | Original symptom, status prefix, and at most a 20-word open summary |
| `P1_EXECUTION_BOARD.md` | Current blocker, decision, candidate identity, and dynamic queue impact |
| `PORTING.md` | Append-only root cause, fix, and chronological result |
| `PERF_LEDGER.md` | Reproducible performance evidence and rejected performance experiments |
| `KNOWN_ISSUES.md` | Durable unresolved gap that outlives the current bug cycle |
| `HANDOFF.md` | Only the immediate restart surface when this bug is the next work |
| `artifacts/visibility` | Permanent synchronized visual evidence |
| `artifacts/performance` | Permanent measurements cited by a kept conclusion |

Record source evidence, commands, candidate identity, result, and remaining
uncertainty where another person can reproduce the conclusion. Do not duplicate
volatile truth across documents.

### 9. Close or leave it honestly open

Mark `**FIXED**` only when all applicable items below are true:

- the original symptom wording and acceptance scope are preserved;
- the failure was reproduced or its prior evidence was strong enough to
  classify, and the root cause was demonstrated rather than inferred;
- the fix engaged on the natural path of the exact candidate;
- the focused check and widest relevant verifier passed on the shipping arm;
- adjacent behavior, resource reserve, cleanup, and pacing remain acceptable;
- permanent evidence is stored and cited;
- the owner completed any required visual, listen, feel, or retail-hardware
  acceptance, which may be batched across related fixes.

If only a subset passes, mark `**PARTLY FIXED**` and name the remaining visible
failure in 20 words or fewer. If a checker, counter, or capture contradicts the
story, leave the row open.

After verified progress, follow the repository's commit and snapshot policy.
The snapshot must be the final project command, with nothing run afterward.

## Bug work packet

Use this as a temporary investigation note or as the compact structure for the
owning history entry. Do not paste the whole packet into `BUGS.md`.

```text
Bug: <verbatim user report>
Status: OPEN | PARTLY FIXED | FIXED
Candidate: <ROM/build, branch, dirty paths, emulator/device configuration>
Repro: <shortest natural trigger, frequency, failing control evidence>
Expected: <BattleShip contract and source location>
Observed: <exact candidate behavior>
Earliest divergence: <first wrong owner/seam>
Hypothesis: <one causal claim>
Falsifier: <predicted measurement and KEEP/REVERT threshold>
Fix seam: <shared owner and affected callers>
Focused proof: <command/result>
Natural proof: <event/state/output result>
Widest verifier: <profile and result>
Artifacts: <permanent evidence paths>
Owner gate: <visual/listen/play/hardware result, batched queue, or pending>
Remaining: <none, or one precise visible failure>
```
