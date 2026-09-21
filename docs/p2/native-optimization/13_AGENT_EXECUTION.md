# Agent execution and integration contract

This document governs P2-2p8 execution, not product requirements. `AGENTS.md`
owns entry routing, `docs/VERIFYING.md` testing/publication, and
`docs/P2_EXECUTION_BOARD.md` the only live queue. A goal describes the destination;
it does not start the whole workflow again on each turn.

## 1. Preserve the endpoint; persist the next action

Every legal four-fighter lineup on every selectable VS stage, fixed-point runtime
including removal of integer IEEE emulation, native-only output in every ROM,
source-equivalent mechanics/content, resource safety and 30 Hz menus remain required.
Keep current fidelity/rates unless an existing explicit permission applies; any
new compromise needs the contract's approval. No missing output, disabled required
content, safe refusal of a legal match or correctness-only PASS closes performance.
Original DS limits and the existing work/cadence gates are unchanged. Rare overruns
remain allowed as the product contract defines them.

The task graph is a dependency/coverage map, not one build or full verifier per card.
The board's current focus chooses work within P2. Static PLANNED fields, research
chronology, a repeated goal and an uncommitted candidate do not reset progress.

## 2. One execution cursor in the existing board

Keep a compact replace-in-place block under **Current integration checkpoint**.
Link one existing batch evidence receipt for detailed facts; do not create another
queue, mandatory report per edit or duplicate status table in HANDOFF.md.

```text
Focus / batch / existing task IDs / owner:
Phase: SELECT | IMPLEMENT | CHECK | VERIFY | RECORD | BLOCKED
Identity/receipt: scoped diff or commit + candidate artifacts/config when applicable
Completed/rejected: action IDs + result/receipt; not merely "already done"
Next: one exact unfinished edit, command, result collection or decision
Checks owed / temporary routes / acceptance status:
Job: none | command + origin/handle namespace + log + inputs + observed state
Capability limits: failed feature + evidence + condition for a legitimate retry
Owner/review watermark: last inspected changed inputs + recorded dispositions
```

Initialize a missing cursor once from the newest applicable local work and evidence;
use RECONCILE_ONCE only during that migration. Replace it with an actual phase before
new implementation or measurement. Unknown fields stay unknown, never invented.
Preserve a newer cursor when adopting document revisions.

Update at a phase transition, before yielding a long-running command, and before
handoff/context loss. Do not rewrite it after every read. Recording completed work
must not depend on a successful Git commit: a permitted on-disk receipt and scoped
diff can preserve an unaccepted candidate. If the board is not writable, report the
exact pending cursor update and permitted receipt path; do not bypass restrictions.

## 3. Continue the phase, not the opening checklist

| Situation | Next action |
|---|---|
| Same intact context, no relevant change | Execute Next; no repeated full startup, plan read or task selection |
| New/lost context | Read cursor + receipt; check relevant inputs/owned job once, then continue the recorded phase |
| Incomplete job still running | Use its supported original continuation; do not start another copy |
| Job finished | Collect/validate its output and record outcome; do not rerun to recreate the conversation |
| New owner edit or changed brief | Preserve it, inspect the relevant delta, record disposition and affected invalidations |
| Local result already KEEP/REVERT | Continue its unfinished integration/acceptance action or the next batch; do not reopen the experiment without new evidence |
| Capability unavailable | Record the limitation and retry condition; continue permitted work without that capability |
| Batch closed or specifically blocked | Record outcome/blocker, then SELECT the next eligible work under the owner's focus |

A turn ending, stale narrative summary or broad performance RED is not by itself
an invalidator. Changed source/layout/assets/config, a broken receipt or contradictory
observations can invalidate specific proof. `VERIFYING.md` controls that validity;
there is no exemption from new final timing after an executable/layout change.

For an action already recorded complete with matching inputs, return its receipt
and advance. A repeat requires a named invalidator, missing coverage or conflicting
result. The action identity is batch + phase + relevant inputs + operation/arguments;
use existing hashes/receipts rather than implementing another cache or scheduler.

## 4. Select once; implement a coherent reduction

At SELECT, use applicable exclusive attribution to name a substantial repeated
operation and its complete native replacement. Assign actual edit boundaries,
input/range/lifetime prerequisites, replacement copy/patch/service costs, a cheap
falsifier and required final coverage. Do not add nested buckets or independent
P95 gains, infer cycles saved from code size, or promise an unmeasured gain.

Then stay with that batch through a recorded decision. Do not repeatedly hunt for
"the largest remaining cost" while its implementation or valid measurement is
unfinished. Re-profile only to resolve a decision-changing gap, stale attribution
or a contradictory observation. Cheap small wins can accompany the batch; they
must not take over the main lane. No numerical savings quota forces unsafe scope.

Combine enabling numeric/binding/generator changes with an immediate consumer and
retirement of the old repeated work in that domain. Preserve unconverted required
paths and real dependency proof. No permanent old/new authority mirrors or broad
unchecked rewrite. The 81 task cards retain all completion/coverage obligations;
a global inventory need not finish before independently safe scoped implementation.

## 5. Optional delegation with persistent failure handling

One integrator owns shared interfaces, included renderer fragments, generators,
linker, build inputs and authoritative timing. Delegate only disjoint concrete edits
and tests when useful and permitted; zero helpers is valid. Do not spend each turn
discovering/spawning helpers just because an old goal mentioned parallelism.

If availability is unknown, confirm one useful helper initializes before launching
a group. After a shared initialization/transport error, record it as unavailable
for that environment and continue serially. Retry only after an observable relevant
change and a successful bounded initialization check; a new chat or another tool
listing alone is not evidence of repair. Do not inject purported trusted environment
metadata into task text to bypass a missing-environment error.

Give workers files/symbols, contracts, patch output and focused tests, not duplicate
broad scouting prompts. Never reassign live overlapping work. Preserve safe workspace
ownership; no forced worktree creation, cleanup, reset or deletion of owner inputs.
Shared generators/builds and authoritative timing remain serialized; disjoint work
must neither change frozen measurement inputs nor introduce material host contention.

## 6. Check and qualify without losing state

CHECK uses applicable source/host/negative fixtures and one discriminating target
check where needed. Require engagement and relevant output. Record the scoped verdict
and outstanding requirements immediately. A rejected theory stays rejected until a
new causal observation changes it; preserved evidence is not a ban on a genuinely
different implementation with an explicit mechanism.

VERIFY freezes the integrated candidate and uses the widest relevant existing
profile. Collect compatible whole-match work/cadence, memory, native and output
proof together; add required rare-state/lifecycle/configuration coverage not included.
No fixed eight-frame/128-frame/full-match/full-profile ladder per edit. Extend a
probe for noise, missing event coverage or contradictory evidence, not a fresh turn.
Remove experiment routes and qualify the final hard-on shape before acceptance.

RECORD stores KEEP/REVERT/BLOCKED_WITH_SPECIFIC_CAUSE or IMPLEMENTED_NOT_ACCEPTED
with exact scope, remaining checks and next action. A neutral enabling change names
its consumer. A combined-batch speedup is not a speedup proved for every member.
Commit/push coherent progress when allowed; an access failure leaves publication
pending, not the implementation undone. Never bypass a denied action. Preserve
qualified artifacts and do not substitute a candidate for an accepted release.

## 7. Owner changes, reviews and imported plan copies

Preserve owner edits; adopt applicable changes intentionally, not wholesale. On
first recovery inspect relevant existing changes/briefs; afterwards use changed
identities and the intake watermark. Record brief path/version and disposition
(applied, already satisfied, not applicable, deferred with reason, or blocked)
in the existing receipt. A changed brief can invalidate that disposition.
Neither an unchanged briefs folder nor a static PLANNED list is a new assignment.

`docs/p2/native-optimization/` is the canonical implementation specification.
`docs/optimization/` contains supporting research and an imported package copy;
those are lookup material, not another active execution/status owner. Read a named
source or archived failure when needed; do not reinstall or synchronize legacy
package copies as routine implementation work.

## 8. Reporting and release closure

Report the delta: operation removed, relevant test result or blocker, and Next.
Do not replay the opening plan, history or a stack of prior commentary. For a
measurement report label P50/P95, units, population and ROM/ELF/config; include
required cadence/resource/native/numeric results and actual coverage. Missing
outputs or uncertain identity remain unknown, not PASS.

DEV_FAST, SCREEN and RELEASE_EXHAUSTIVE remain coverage scopes, not invented
registry switches. All requirements in `16_ALL_ROSTERS_ALL_STAGES.md` remain:
exact case sets, no pooled P95, independent duplicates, permutations/variants,
resource/semantic/visual/audio proof, compatible evidence and visible failed,
blocked, stale and unrun cases. Scoped progress is not universal completion.
A deadline or desire to reduce repetition changes execution priority, not verdicts.
