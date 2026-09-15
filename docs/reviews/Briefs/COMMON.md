# Candidate implementation contract

This package supplements the existing bug owners; it is not another dynamic queue.
Read the actual checkout's `AGENTS.md`, `PROJECT_GOAL.md`, `docs/HANDOFF.md`,
`docs/P2_EXECUTION_BOARD.md`, `docs/BUG_FIXING_PROCESS.md` and `docs/VERIFYING.md`.
Those documents retain authority. The latest owner's wording is preserved in the
36 issue documents (35 actionable reports plus an empty Captain Falcon heading).

## What the files mean

`runtime/*.patch` changes target C and/or the actual asset producers. These are
**candidate changes**, not accepted fixes. `experiments/alpha_isobands.py` is working
offline experimental code, not a linked renderer. The issue `.md` files include
conditional implementation proposals where a defensible target diff cannot yet
be specified. Do not count them as implemented code.

There is deliberately no apply-all runtime command. R02 changes shared effect
state and needs a callback/material sibling audit. R03 is lifetime hardening,
not a solution to CSS loading latency or roster completeness. R04 repairs the
Link-only/disabled-Kirby helper configuration; it may not explain the reported
ROM. R05 requires a correctly configured asset rebake and staging.

## Reconcile before applying

The inspected public branch was `master` at
`a5c5bc08d8e8661658865216798d600462db948e`. Do not reset a newer branch or dirty
working tree to that snapshot. Do not assume the historical runtime2 branch
still exists. Run the local policy preflight in PowerShell 7 and inspect status.
Use `tools/check_candidates.py` for read-only exact-preimage and `git apply --check`
checks. A mismatch requires a source rebase and renewed applicability review,
not `--reject`, forced overwrites, broad search/replace or disabled assertions.

Record the actual ROM/ELF hashes, effective build header, base/dirty overlay,
source/generated assets, input/seed and emulator identity. A file name alone
does not identify the code/configuration that produced a symptom.

## Non-negotiable behavior

Every ROM is native-only, including debug and profiling. Never add an interpreter,
generic graphics fallback, compatibility compositor or opaque stand-in. Required
content rejected by a native owner is still missing content, even when rejection
is safe. No whole-fighter unhide, hidden hazard, global Z bias, blanket alpha
clamp, new fixed door oscillator or roster-gate bypass is an accepted repair.

Read original BattleShip move/descriptor/material consumers before implementing.
Keep `decomp/` read-only. Fix producers, not generated includes/binaries. Use the
existing native owners; avoid a new general rendering or job framework when a
small owner-specific state machine suffices. Inspect the mandated DS reference
trees before substantial new renderer/memory architecture, especially before
integrating the gradient experiment or a redesigned streaming loader.

## Build and proof

One serialized build, including across worktrees. The Makefile owns parallelism:
no `-j`, `-Jobs` override or MAKEFLAGS modification. Freeze shared generated inputs
while consumers build/run. Follow the checkout's current command parameters and
profile registry. Boundary is for battle-only integration; Latest is for normal
startup/menu/shared changes. Choose one widest relevant profile rather than
stacking redundant profiles. A diagnostic fast-logic walk is not cadence proof.

Use the project's accuracy-focused melonDS with interpreter/JIT disabled from
boot, not routine retail measurements. Prove the natural input path, positive
native owner engagement, the actual missing/incorrect pixels or audio, source
behavior, resource lifetime and current cadence/performance requirements. A
short matched eight-frame A/B can reject a hypothesis; it is not full P95 proof.

Do not equate a mock C test with a target build, patch application with behavioral
proof, or an unengaged zero counter with coverage. Keep each dependent symptom
open until its own observation passes. Retain known-good source-derived work;
older notes are search leads, not instructions to repeat a closed fix.

Update existing owners without replacing the user's edits. `docs/BUGS.md` agent
annotations stay bold and at most 20 words. FIXED requires all closure conditions,
including any required owner acceptance. Report implemented, host-tested,
ROM-tested and accepted scope separately.
