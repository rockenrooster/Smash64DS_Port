# Agent execution and integration contract

## 1. The unit of work

Use one task ID from `tasks.json` under existing P2-2p8. Read the current board and handoff, the task card, its named source, shared contracts and relevant test sections. Do not restart implemented optimizations or reread every historical campaign on every run. Source files/functions are pinned anchors; re-resolve after rebasing.

Each assigned task must have a specific behavior/operation to remove. “Optimize the renderer” is not a task. A task is also not complete when it only adds infrastructure that every future agent must maintain. Enabling work must name its immediate consumer and eventual deletion of the old path.

## 2. Source ownership and concurrency

| Lane | Owns | May work in parallel with | Must serialize |
|---|---|---|---|
| Integrator | Shared ABI, linker, Makefile, generated outputs, current baseline and board | Read-only review/host fixture authoring | Every shared generated-output update and target build |
| Renderer | Bound descriptors, native packet/actor emitters and renderer state | Disjoint numeric fixtures, gameplay source analysis | Other edits to included renderer fragments/shared statics |
| Numeric/pose | Numeric kernels, event clocks, native pose contracts | Disjoint generator test authoring | Shared numeric header, pose consumer ABI changes |
| Gameplay | Physics/collision/AI/phase-owned query state | Disjoint stage/UI work | Shared FTStruct/consumer migration and scheduler changes |
| Content/services | Stage/VFX/UI or audio/offload, one claimed source slice | Independent host tests | Resource manifest/bank handoff and service ownership |
| Reviewer | Independent source/geometry/numeric/correctness proof | All disjoint source implementation | Final acceptance writes and timing runs |

Do not force a fixed number of agents when their edit sets overlap. Additional helpers can inspect or write disjoint host fixtures, not repeatedly touch the same giant translation unit. The project procedure allows isolated correctness runners but requires one build at a time and no concurrent timing/exact visual acceptance. Preserve that policy. [S03]

The current board reports many pre-existing dirty auxiliary worktrees. Inspect existing workspace ownership before creating another. Never use `git reset --hard`, `git clean`, broad deletion of `decomp/`/`artifacts/`, or replacement of owner inputs to make a check green.

## 3. Task handoff template

```text
Task: Nxx.yy — exact title
Parent: P2-2p8
Baseline: commit + intended dirty overlay + ROM/ELF/config/asset hashes
Dependencies: IDs and accepted evidence paths
Allowed edits: concrete files/symbols and one owner for shared interfaces
Work to remove: repeated operations + current measured/bounded cost
New runtime cost: patches/copies/service/memory introduced
Contracts: numeric class, mutation owners, event order, lifetimes
Tests: relevant fixture IDs + existing or task-created invocation
Limits: original-DS RAM/TCM/transient resources; no required content loss
First falsifier: cheapest test that would invalidate the mechanism
Finish: code + producers + outputs + tests + final hard-on evidence
Stop: exact correctness/performance/resource condition causing revert
Status return: KEEP / REVERT / BLOCKED_WITH_SPECIFIC_CAUSE
```

The task-specific cards already fill most of these fields. Add current hashes and measured costs; do not invent projected savings or report task metadata as runtime evidence.

## 4. Per-task execution loop

Read current implementation and identify already-landed parts. Run the cheapest relevant source/host check. Freeze inputs and coordinate the edit boundary. Implement the smallest complete vertical slice and its negative tests. Build once through the integrator. Run a focused source-controlled target case with positive engagement. Decide KEEP/REVERT; for a kept batch collect the widest relevant integrated proof, then rebuild/qualify the final hard-on shipping shape when due. Retire the losing route and update the existing board row/evidence.

Do not perform a long unchanging suite after every line edit. Do not skip required integrated/final coverage merely because a host test passed. Use the documented eight-frame synchronized comparison for early iteration; long-run P95 and cadence require whole-match evidence. Respect the documented cross-build noise/significance handling rather than forcing tiny deltas into a victory. [S03]

If a checker fails because of an existing owner overlay or infrastructure issue, preserve it and report the exact blocker plus focused evidence. Do not mark the umbrella green, waive native/content checks, delete the owner's input, or repeatedly rebuild until a noisy run passes.

## 5. Commands and build discipline

The pinned procedure uses PowerShell 7 and configured devkitPro/devkitARM. Verify script parameter blocks before copying historical commands. Existing startup commands:

```powershell
git status --short
.\scripts\verify-all.ps1 -Profile Boundary -List
```

On a prepared host, the P2 build entry is `make TARGET=smash64ds`. For an unprepared host, `build.ps1 -Rom <owner-provided baserom path>` is the documented acquisition/extraction entry, with prerequisites checked first. This package does not contain or supply a copyrighted game ROM or generated ROM assets.

Never pass `-j`, override `-Jobs`, or change `MAKEFLAGS`; the existing Makefile owns build parallelism. Shared generated files make parallel builds unsafe even in separate BUILD directories. Do not use a custom BUILD with a publish target as though it could not overwrite the root ROM. Lab targets and matching per-build ROM/ELF pairs are for experiments. [S03]

Boundary is three named runtime entries; Latest adds the normal runtime entry. Choose the widest relevant profile for a batch instead of stacking every profile. A green Boundary does not rebuild the root shipping ROM automatically. `-NoBuild` requires validated matching artifacts. Do not change the established profiles to hide a slow product result; N00.04 adds explicit performance evaluation.

Use repo-local accurate melonDS and JIT-disabled interpreter from boot; keep the owner's manual instance untouched. Timing runs are serialized. Disposable per-run save/DLDI/storage paths and coherent guest observation/publication are part of the identity, not optional bookkeeping.

## 6. Progress and evidence format

Keep the dynamic board concise. A kept work item can read:

```text
N03.04 KEEP — replay-hit immutable preflight retired; native/state guards pass.
Evidence: <path>; ROM <hash>; WORK-H/cadence <measured values>.
Remaining: <specific unconverted family or final acceptance gap>.
```

Do not use **FIXED** without required coverage. A report should distinguish source presence, host checks, target engagement, integrated correctness, product performance, resource closure and published artifact. Store detailed source reasoning and logs under the existing evidence owners; do not paste them into every code comment and board row.

## 7. Safely adopting or revising this plan

The package is additive and was not pushed to GitHub. Review the proposed shared interfaces and first task at the active branch before committing the plan. If a newer commit already implements a task, link its equivalent evidence and move to its remaining obligation rather than reimplementing it.

Update static dependencies when a real ownership requirement changes. Do not create a new parallel campaign simply because task IDs are inconvenient. Keep the plan validator/combined view generated from one source set; the existing P2 board remains the live queue.

## 8. Matrix execution without slowing every iteration

Use **DEV_FAST** for a local discriminating edit, **SCREEN** to prioritize measured mechanisms/leaders, and **RELEASE_EXHAUSTIVE** to qualify the declared full roster-stage universe. A local KEEP is not full-product PASS. Do not run the full matrix after every small edit and do not use DEV_FAST/SCREEN as release evidence.

Case enumeration, resource solving and result reconciliation are host tools; reuse the existing runner, ring collector and single build ownership. No parallel shared builds or concurrent timing acceptance. Sharding is a resumable work partition, not permission for simultaneous emulator timing.

Use the required source catalogue as the queue input, not successful output folders. Persist failed/blocked/stale/not-run cases and actual guest roster/stage/slot attestations. A resumed job consumes only compatible completed evidence; a fresh result never silently erases an earlier reproducible game failure.

On handoff report cases required, valid passing, failing, blocked, not run, stale and infrastructure-invalid; list any property-scoped reuse separately. Report separate cost leaders and the next concrete failing legal case. Existing P2_EXECUTION_BOARD.md stays the only live status queue; machine-generated coverage is evidence, not a second manually maintained board.

Implementation commands for new runner/catalogue/reconciler tasks are proposed until those tools exist. This revision includes a host-only planning count helper and stricter planning tests, not a working ROM scenario driver or game acceptance engine.
