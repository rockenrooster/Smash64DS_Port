# Handoff

The destination is P2 under `PROJECT_GOAL.md` and `P2_PLAN.md`.
`P2_EXECUTION_BOARD.md` owns live focus, decisions, artifacts and the
**Execution cursor**. This file is a route, not another task or metric ledger.

## Continue, do not restart

With an intact context, execute the cursor's next unfinished action. Do not
repeat startup, capability discovery, task selection, profiling or completed
verification merely because the goal is repeated or a turn ended.

After context loss, read the cursor and its linked batch receipt once. Reconcile
only relevant source/asset/config changes and an owned live job before relying
on that state. Missing identity blocks reuse of that proof, not unrelated safe
work. A settled experiment needs a named new invalidator to reopen.

An unseeded cursor is initialized once from the newest applicable local evidence
and relevant diff. Replace the unseeded state immediately; do not leave a standing
instruction to rediscover a named historical candidate. A newer live cursor wins
over this documentation package's migration seed.

## Owners

`VERIFYING.md` owns command lifecycle, test validity and publication. For P2-2p8,
`p2/native-optimization/13_AGENT_EXECUTION.md` defines cursor phases and fields.
Read only contracts needed by the next action. The board chooses the next batch
only after recording an outcome, a concrete blocker or an owner priority change.

Keep unavailable helpers and denied Git operations recorded with their retry
condition; use permitted serial implementation instead of repeating setup.
Preserve owner edits and qualified artifacts. A documentation change does not
repair a transport failure, authorize denied access or establish a game PASS.

## Standing lesson from 2026-09-21

Two rows in that session were "fixed" against the wrong object, because the
symptom was matched instead of the producer. The owner reported a pistol-shot
effect; the model was removed (wrong), then the blaster glow was suppressed at
its muzzle and then at all seven of its callbacks with a measured zero spawn
count (also wrong -- the flash survived). Each attempt was cheap to build and
expensive to disprove, because disproving it costs an owner playtest.

Before changing a producer, name it from the running build. For an effect that
is the kind dispatcher and the colour-animation entry under the exact status,
not the maker whose name matches the words in the report. The counters exist;
a probe that enumerates is worth more than two that confirm a guess.

The same session also found three dead effects sharing one cause, recorded in
the receipt: BattleShip's `ll*` reloc symbols are linker-absolute, so the
symbol's ADDRESS is the offset. A `static uintptr_t` satisfies the compiler and
makes every derived base garbage; the registry keys on `&sym`, so a static can
never resolve; and a descriptor whose offsets address a different file is
span-rejected and deferred with `proc_display` cleared, after which its maker
returns a GObj with a NULL DObj and faults. Check those three before concluding
an effect is a rendering problem.

## Standing lesson from 2026-09-21, second pass

Nine rows closed in one day, and **six of them had a wrong premise written down
in this repo before work started**. The premise was the expensive part, not the
repair. Concretely:

- P01's "CI4 with a luminance-ramp TLUT" described the **air** jolt. The ground
  owner is IA8 and loads no TLUT at all.
- P03's "no native bake" was a misread failure code. `NO_PROGRAM` means nothing
  *claimed* the root, not that geometry is missing. The bake had always existed.
- K04's "GObj-starved" did not survive reading the reserve: the cap is raised by
  eight after a mid-match latch, leaving ~10 free slots.
- S04's earlier "root and first child read (0,0)" sampled DObj 0, whose
  anim-joint slot is NULL by construction, so it could never have shown motion.
- K03 and L01 were filed together on matching symptom wording and share nothing:
  one is a producer reading a `.bss` address, the other a renderer applying a
  translate twice.
- The Kirby heap number was real but described the wrong regime -- it only holds
  **after a copy**, and a plain match sits 16 KB clear of the floor.

**How to apply:** before implementing from a recorded diagnosis, re-derive its
first fact from the running build. Cheaply -- one grep, one decode, one counter.
A stale premise reads exactly like a fresh one, and every hour spent on it
produces a confident repair to something that was never broken.

Corollary, learned the same day: **failure codes are names, not explanations.**
Read the enum's definition before letting the word in it choose your repair.

### Falsifiers must follow the producer

Two checks went red this session for the same structural reason: a validator
held a literal the producer had since redefined. `check_nds_native_stage.py`
pinned a mask to "source transform flags" after the generator widened it to
include animated bindings, and the stage falsifier had never classified a
parent-scale walk that had grown into `ndsRendererAdapterApplyMvpRecalc`. The
second one **failed the whole build** the moment an unrelated edit made its rule
re-run -- it had been latently broken for an unknown period, invisible only
because its output was up to date. When a producer's definition widens, widen
the check against the producer, never against a copy of the old number.

## Standing lesson from 2026-09-21, third pass: the sibling census

**Three regressions in one day, all the same shape, none caught by any check.**

  * The custom matrix-kind repair fixed Link's slash and broke three spark
    effects.
  * The ground Thunder Jolt's texture converter fixed the terrain jolt and broke
    the AIR jolt.
  * The Poke Ball admission made the ball visible and broke the opening rays.

Every one of these was correct for the consumer it was written for, derived from
the source, and shipped with a mutation-tested check. Every one still broke a
sibling, and all three came back from the owner playing the ROM rather than from
CI.

In each case **a change to a shared path was validated against ONE of its
users**: a switch arm six descriptors reach, a texture path the air owner also
uses, an item submit path the rays are submitted right behind.

**The rule.** Before landing a change to a shared switch arm, adapter path,
cache or allocator: grep the arm's other users and state in the commit what
happens to each. One command, one sentence per user. The `0x45` census took a
single command and found six users — after the fact.

**The corollary, which matters more.** A mutation test proves the repair does
what it claims *for its own case*. It says nothing about siblings. A shared-path
change needs a **sibling census**, not a stronger unit test. Both regressions
had green, genuinely adversarial checks.

**The repair pattern when a sibling does break:** scope, do not revert. The
matrix fix was restricted to DObjs carrying more than one transform, which keeps
the proven case and leaves every unexamined one exactly as it shipped. Reverting
would have lost a real fix; scoping lost nothing.

## Standing lesson: static routing is not a runtime load

R02/R03 packed 142 missing Results animations, closed 47 of 47 cells, and
changed nothing on screen. The payloads are staged and the mapping resolves
statically — the failure is downstream, in the runtime load or the AObj16
normalization, and **the counter that would tell the two apart was identified in
the original report and never built.**

`ftMainSetStatus` assigns `fp->figatree` unconditionally and discards the return
value, so a failed load is silent by construction. Both existing counters sit
inside the success arm and cannot see it.

When a load path can fail silently, the instrument that observes the failure is
part of the repair, not a follow-up. Build it in the same change.
