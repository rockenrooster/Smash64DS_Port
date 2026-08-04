# Bug Fixing Process
do not build smash64ds.nds for P1 work. smash64ds.nds is for P2 work.

This document owns the process for taking a user-reported playtest bug in
`BUGS.md` from symptom to verified closure. It does not own bug status, product
truth, verifier commands, or history; those stay in their owning documents.
Every `BUGS.md` row blocks P1 unless the owner explicitly changes its scope.

## The two laws

The complete N64 source is in `decomp/BattleShip-main/decomp`. Correct behavior
— including exactly where an effect appears, how big it is, how it moves, and
how a cue sounds — is derivable from source as numbers. Therefore:

1. **The source is the oracle. The owner is the final confirmation, never the
   debugger.** Convert every "looks wrong / sounds off" report into measurable
   quantities and iterate against those on your own instruments. Request an
   owner check only when every quantity is green and you can predict, in one
   sentence, what the owner will see or hear. An owner reply of "still not
   right" on a candidate you could not predict is a process failure, not new
   information — the candidate was sent before it was derived.
2. **A ROM build is spent only to confirm a written prediction or to plant
   batched probes.** Never build to "see whether it looks right now." Most
   runtime values are readable on the existing ROM through GDB and captures.

The expensive resources are owner round-trips and ROM builds. Source reading,
GDB reads on a live ROM, static checkers, and image analysis are nearly free.
Spend accordingly.

## What "fixed" means

A fix is one continuous chain:

> symptom -> observable contract from source -> first divergent measured value
> -> root-cause fix at the owning seam -> every contract row green on the
> natural path -> widest relevant verifier -> batched owner acceptance

Compilation, a packed asset, a non-zero counter, one good frame, or failure to
reproduce is not closure. Disabling the feature, avoiding its trigger, or
substituting other content is containment, not a fix.

"1:1" means correct observable source behavior, not identical N64 code or
architecture. Generated, specialized, or DS-native implementations are
encouraged when they preserve the same observables. A candidate that pushes the
scene's representative P95 over its budget or materially worsens the 2/3/4/5+
VBlank histogram is intermediate, not fixed; record "optimization needed" as
non-blocking debt only after the performance gate is met.

## Work clusters, not single rows

Rows sharing a subsystem (today: particles/VFX; audio cues) share
instrumentation, captures, builds, and the owner's acceptance session. Write
the contracts for the whole cluster first, plant one batched probe build that
answers every open question in it, and present one acceptance batch. Each row
still keeps its own contract and proof. One bug per build is how a queue
stalls; "prefer larger slices of work" is owner doctrine.

## The evidence ladder

Use the cheapest rung that can falsify the current claim; escalate only when a
rung is genuinely inconclusive.

0. **Source + existing artifacts.** BattleShip constants and tables, CodeGraph
   traces of the port path, prior captures and JSON under `artifacts/`.
1. **Static and AOT checkers (seconds, no ROM).** `check-nds-particle-banks.ps1`,
   the audio fixture checkers, `scripts/sfx/render-audio-fgm-phase-pack.py
   --derive <id>` (prints a cue's full source selector chain), the asset
   generators, and host fixture oracles (Step 2).
2. **The existing ROM (minutes, no build).** GDB through the melonDS stub:
   break at the maker/trigger, read arguments, structs, transforms;
   frame-step captures; screenshots plus `compare-capture-pair.ps1`.
3. **One batched instrumented build** carrying every probe the cluster needs.
   Write each probe's predicted value before building; a run that fills no
   prediction row was a wasted build — record why it happened.
4. **The fix-candidate build**, confirming the predicted contract rows.
5. **One widest relevant verifier** on the kept candidate (`VERIFYING.md` owns
   commands and profile choice), then batched owner acceptance.

Standing rules: run the shortest event that reaches the trigger — never wait
through a full match for a bug reachable in seconds; do not rerun an unchanged
ROM or require routine A/B/A or long soaks; a performance A/B is owed only when
active-frame work changed, the mechanism is plausibly hot, or pacing regressed;
escalate to retail hardware only for conflicting evidence, intermittent
failures, release qualification, or device-specific mechanisms.

## Step 1 — Intake

Record the observable facts before proposing a cause: exact candidate identity
(ROM/build, branch, dirty paths, emulator config); scene, fighter states,
action, inputs, match time, preceding event; expected vs observed; frequency
and shortest known trigger; capture or owner description; deterministic or
intermittent. Preserve the user's wording and uncertainty — "sounds too loud"
is a symptom, not a diagnosis. Reuse a valid existing failing capture instead
of re-reproducing it. If the bug cannot be reproduced, do not guess a fix;
improve the observation surface or obtain the missing trigger details.

## Step 2 — Write the observable contract

Decompose the symptom into the quantities that define "correct." **The
contract is complete when matching every row within tolerance makes the
reported symptom impossible.** Skipping this step is what made candidates
bounce off the owner.

Visual dimensions: world position and attachment (does it follow its owner?),
screen position, size/scale, count, texture/frame identity, color/palette,
blend/opacity, motion (velocity, gravity, spread), spawn frame relative to
trigger, lifetime, layer/z.

Audio dimensions: cue identity, level (peak/RMS), rate/pitch, duration and
stop reason, envelope shape, pan, timing vs trigger, and the mix against
concurrent channels — a cue can be internally source-exact and still wrong in
the mix.

Each row: quantity; expected value with BattleShip `file:line`; tolerance; the
DS probe that reads it. Pick tolerances from representation error (fixed-point
conversion, projection rounding, resample ratio), never taste, and write them
down. Example, from the Whispy row:

| Quantity | Expected (source) | Tol | Probe |
|---|---|---|---|
| Dust world x | -715 or -205 by `lr_players` (`ground.h:19`) | ±4 | GDB read of spawn args + `dust_xf->translate` |
| Side selection | matches `gGRCommonStruct.pupupu.lr_players` | exact | same break |
| Screen position | projection of the chosen side beside the tree at -525 | ±6 px | capture + image analysis |
| Spawn timing | blow start + source frame offset | ±1 frame | frame-step capture |

Expected values come from, in order:

- **Constants and tables in source** (effect scripts, cue selector tables).
- **A host fixture oracle** when the value is algorithmic: compile the source
  algorithm into a host-side checker (existing pattern:
  `scripts/native_light_sidecar_oracle.c`, `scripts/mp_topology_fixture.c`,
  `scripts/fixtures/*_expected.json`). No emulator and no eye needed.
- **The asset pipeline's own inputs** for appearance identity: the converted
  source texture/frame is the definition of what the effect should show.
- **A temporarily enabled source-exact interpreter arm** as an on-DS oracle for
  side-by-side comparison; it remains intermediate if over budget.
- **The owner, once, for genuine feel** — and then as a choice between
  prepared alternatives (an A/B pair), never as an open "is this right?"

Owner-approved presentation deltas (the fidelity rules in `AGENTS.md`, e.g.
Dream Land water frozen at source frame 0) override raw source in the
contract; cite the recorded approval instead of "fixing" an approved delta.

## Step 3 — Localize: walk the chain and find the first wrong number

Follow the whole path and measure each stage's actual value against the
contract; the first divergent value names the owning seam. Do not stare at the
composite picture and guess.

> VFX: trigger -> source state/event -> maker + args -> LBTransform ->
> bank/atlas resolve -> update -> draw submission -> pixels
>
> Audio: trigger -> cue id -> pack entry -> enqueue/channel -> mix -> PCM

| Symptom | First evidence to collect |
|---|---|
| Wrong gameplay or scene flow | Source state transition, live status/input trace, first differing state |
| Missing or wrong VFX | Trigger/effect ID, create/drop, asset/atlas resolve, update, draw submission, client pixels |
| Missing or wrong audio | Trigger/cue ID, pack lookup, enqueue/channel result, duration/stop reason, rendered PCM |
| Freeze or corruption | Guest frame/counter, PC and instruction, stack, allocator/DL/GFX/abort counters |
| Performance regression | Matched same-tree A/B, engagement, typed ticks, P50/P95, VBlank histogram and maximum |
| Harness or detector defect | Known-bad positive control, expected artifact, sample/hit count, guest-owned signal |

Probe discipline (each learned the hard way):

- GDB command batches must be nm-checked; one absent symbol silently aborts
  the rest. A halted-core screenshot can show a stale buffer — break on the
  constructor, then step.
- **`break *ADDR` on an optimized build: disassemble the address first, and
  never trust gdb's "file X, line N" reply as proof you are on that statement.**
  At -O2 the line table attributes reordered and merged blocks freely. A 2026-08-03
  probe broke on an address gdb called line 8572 (an early `return`); it was
  actually a high-water-mark update on the *completed* path, so it reported a
  rejection that never happened and published `dl`/`dobj` values read from a
  clobbered r4 and a stack address. Prefer breaking on a **function entry by
  name**, where the ABI guarantees r0–r3 hold the arguments. If an interior
  address is unavoidable, quote the disassembly that proves which registers are
  live in the probe's own comment, and sanity-check the result: a value that
  never changes across many hits and many objects is a fixed address, not a
  per-object pointer.
- A counter with no compiled writer reads 0, which looks clean. Every probe
  needs an engagement count or positive control: zero must mean "measured
  zero," never "the hook never ran" or "the linker removed it."
- **When the thing you are hunting is a hang, the probe's TIMEOUT is the
  expected path — put the abort read in a `catch`, not after the call.**
  `Invoke-GdbMarkerScript` throws on timeout, so an abort read placed after it
  never executes and the `finally` block kills the emulator before anything is
  read. A 2026-08-03 session caught the hang it was looking for and still lost
  `lr_abt` to this. (The streamed breakpoint output survives inside the
  exception message, so a timed-out run is not necessarily a lost run — read
  the exception before re-running.)
- **Derive what CORRECT looks like from the source before calling a measured
  structure broken, and read the constants rather than assuming them.** On
  2026-08-03 a one-node DObj was reported as "the tree was never built" for
  three cycles. It was correct: that effect's EFDesc omits flag `0x4`, so
  `efManagerMakeEffect` builds exactly one DObj holding a raw display list.
  The assumption that flipped it was `EFFECT_FLAG_USERDATA == 0x1`; it is `0x2`
  (`efdef.h:7`), which selects a different branch entirely. Also confirm the
  thing you are debugging actually OCCURRED in the run — three of those four
  effects never spawned once in two 901-frame captures, so every number
  attributed to them was really about the fourth.
- Compare captures against the synchronized control arm and crop metrics to
  the changed region; a full-frame metric hides a fully-wrong island, and a
  candidate judged alone destroys things the control would have caught.
- Change one causal variable at a time; define KEEP, REVERT, and inconclusive
  before reading the result. When two hypotheses fail, re-trace the chain
  instead of stacking a third speculative patch.
- A diagnostic may accelerate the natural event but not change the behavior
  being judged; final proof exercises the natural path. Confirm the guest
  reached the state under test — host window motion is not guest liveness.
- For a freeze, classify the stopped guest before naming an owner: an
  allocator spin, DL overflow, data abort, IRQ/wait state, and a slow live
  frame need different fixes even when the window looks equally frozen.

`optimization/TASK_STANDING_RULES.md` owns measurement traps and time boxes.

## Step 4 — Fix the root cause once

Make the smallest mechanically correct change at the first divergent seam,
shared by all affected callers — check every caller and sibling path first.

- Reuse the existing source-backed or DS-native owner before adding a path,
  cache, selector, or abstraction.
- Never hide a shared defect with a frame check, arbitrary offset, synthetic
  input, bug-specific effect, or permanent proof-only branch.
- Port behavior lives under `src/nds` or `src/port`; compatibility
  declarations under `include`. Preserve unrelated dirty work; revert only
  your own hunks when an experiment is rejected.
- Remove temporary probes; keep a diagnostic only when it prevents recurrence
  and has its own runnable check. If the bug exposed a repeatable workflow
  failure, improve the owning helper/checker/doc in the same scoped change.

## Step 5 — Prove the candidate

1. Every contract row measured green on the exact candidate, natural path.
2. One adjacent or sibling path that could regress from the same change.
3. The widest relevant verifier in the configuration that will ship.
4. Domain guards: **visuals** — synchronized control/candidate capture in
   `artifacts/visibility` passing the image gates; **audio** —
   event-to-cue-to-channel evidence and no unexplained drop/underrun (a packed
   cue alone is not audible proof); **freeze/memory** — a classified failing
   capture before the fix, a bounded stress run after it, reserve/overflow
   counters healthy; **performance, when owed** — matched same-configuration
   A/B with engagement, typed P50/P95, VBlank histogram and maximum.

Unexplained flashes, corruption, state differences, missing content,
nondeterminism, or verifier contradictions are failures even if the original
symptom disappeared.

## Step 6 — Owner acceptance: batched, predicted, structured

Ask for the owner's eye or ear only when, for every row in the batch: the
contract is green on the exact candidate; a one-sentence prediction of what
the owner will perceive is written; the side-by-side or listenable evidence is
attached from `artifacts/`. Batch every ready row of a cluster into one
session. Present each row as:

> **Prediction:** what you will see/hear, one sentence.
> **Evidence:** capture/PCM path. **Verdict:** PASS / FAIL(dimension)

On FAIL the owner names the wrong dimension from the contract vocabulary
(position, size, count, motion, color, timing, loudness, pitch, mix, other).
That answer is a new contract row: measure it, fix it, and re-verify only it
plus adjacents — never re-prove rows already green, never resend an unchanged
candidate.

**If the owner rejects a candidate whose every row is source-exact, the
contract was under-specified** — a dimension is missing (mix vs cue gain,
atlas admission vs script correctness, screen vs world space). Widen the
contract and continue; that, not another build, is what "Check Source" means.

## When the gap is a decision, stop iterating

If the divergence traces to a resource or architecture bound (atlas byte
budget, pool size, VRAM/arena margin, admission policy), cosmetic iteration
cannot close the row. Write a decision request on the board — the bound, the
options with measured costs, one recommendation — mark the row
`BLOCKED(decision: ...)`, and take the next row. Polishing inside a constraint
that guarantees failure is the most expensive form of non-progress.

## Keep `BUGS.md` lean, with visible stages

`BUGS.md` is the user-facing queue, not an investigation log.

- Preserve the user's symptom wording and ordering; never silently delete,
  merge, split, or narrow a report.
- Prefix closed rows `**FIXED** (YYYY-MM-DD)`; use `**PARTLY FIXED**` only when
  an independently verified part is closed and the remaining symptom is named.
- For open rows, append one line of at most 20 words, starting with the stage:
  `CONTRACT` (expected values written) -> `LOCALIZED` (first divergent seam
  named) -> `MEASURED` (all rows green on candidate) -> `OWNER-QUEUED`
  (prediction and evidence ready) -> or `BLOCKED(decision: ...)`. Stages make
  progress legible between owner sessions.
- An umbrella report is not closed by one representative case: track every
  named case in the packet and prove each naturally. A shared root cause may
  fix several rows; each row keeps its own acceptance.

## Priority

1. Freeze, crash, corruption, nondeterminism, or data loss.
2. Gameplay, input, collision, state, timing, or scene-flow defects.
3. Missing or wrong telegraphs, VFX, SFX, camera, HUD, or results.
4. Cosmetic or acoustic mismatch without gameplay meaning.
5. Tooling defects that make evidence for classes 1–4 unreliable.

Within a class, prefer the shared owner that closes several rows. Correctness
outranks optimization unless the owner reorders. A performance regression
introduced by a fix belongs to that fix. A tooling defect invalidating a
current proof moves ahead of the bug depending on that proof.

## Where evidence lives

| Owner | What belongs there |
|---|---|
| `BUGS.md` | Symptom, status prefix, one ≤20-word stage line |
| `P1_EXECUTION_BOARD.md` | Current blocker, decisions, candidate identity, queue impact |
| `PORTING.md` | Append-only root cause, fix, chronological result |
| `PERF_LEDGER.md` | Reproducible performance evidence, rejected experiments |
| `KNOWN_ISSUES.md` | Durable unresolved gaps outliving the cycle |
| `HANDOFF.md` | Restart surface only |
| `artifacts/visibility` | Permanent synchronized visual evidence |
| `artifacts/performance` | Permanent measurements cited by kept conclusions |

Record commands, candidate identity, results, and remaining uncertainty where
another person can reproduce the conclusion. Do not duplicate volatile truth.

## Close honestly

Mark `**FIXED**` only when: symptom wording and scope are preserved; the root
cause was demonstrated, not inferred; the fix engaged on the natural path of
the exact candidate; the contract, focused check, and widest relevant verifier
passed on the shipping arm; adjacent behavior, reserves, cleanup, and pacing
remain acceptable; permanent evidence is stored and cited; the owner completed
any subjective acceptance (batched is fine). A checker, counter, or capture
that contradicts the story keeps the row open. After verified progress, follow
the repository's commit and snapshot policy; the snapshot is the final project
command, with nothing run afterward.

## Anti-patterns — each has already cost a full loop

- Building a ROM to "see if it looks right now."
- Verifying the mechanism and calling the symptom fixed (transform applied ≠
  dust beside the tree).
- Sending the owner a candidate whose appearance you could not predict.
- Re-proving already-green rows after a FAIL instead of measuring the named
  dimension.
- Iterating cosmetics against a resource bound that needs an owner decision.
- One bug per build when five share the subsystem.
- Waiting through a full match for a trigger reachable in seconds.

## Bug work packet

The working note per row (keep it in the investigation, not in `BUGS.md`):

```text
Bug: <verbatim user report>
Stage: CONTRACT | LOCALIZED | MEASURED | OWNER-QUEUED | BLOCKED(...) | FIXED
Candidate: <ROM/build, branch, dirty paths, emulator config>
Trigger: <shortest natural trigger; how the probe reaches it>
Contract:
| quantity | expected (file:line) | tol | probe | measured | verdict |
Earliest divergence: <first red row and its seam>
Fix seam: <owning seam and affected callers>
Prediction: <one sentence — what the owner will see/hear>
Proof: <focused check; natural-path result; widest verifier result>
Artifacts: <permanent evidence paths>
Owner verdict: <pending batch | PASS | FAIL(dimension)>
Remaining: <none, or the one open dimension>
```
