# Worktree and Branch Consolidation Report

Audited: 2026-07-17 against the live `docs/goal-objective.md`

## Outcome

- Audited all 17 registered worktrees, four detached heads, and all 19 local
  branches. No remote refs are configured.
- Pre-consolidation `master` (`d2993b7ec84c`) was an exact ancestor of the
  verified integration tip (`4e0983094f3c`): 0 master-only / 233 integration-only
  commits. The integration line is the only merge surface promoted to `master`.
- No side branch is a safe or useful wholesale merge. Its work is either
  contained, semantically superseded, explicitly rejected, or incomplete.
- The live objective edit is preserved in the consolidation checkpoint. The
  restart surface now names `master`.
- No worktree was deleted. Most contain ignored ROM/build/capture data, so this
  report separates source redundancy from deletion safety.

Verdicts: **KEEP** is authoritative; **KEEP-REF** preserves useful Git evidence
without a worktree; **REMOVE-CANDIDATE** has no source work to merge; **HOLD**
requires an evidence or live-file decision first.

## Worktree Progress

Ignored counts come from `git status --ignored --untracked-files=all`; a clean
tracked tree can still contain material data.

| Worktree | Audited ref / head | State | Progress against objective | Verdict |
|---|---|---:|---|---|
| `D:\Stuff\DevFolder\Smash64DS_Port` | `codex/wip-natural-combat-source-start-collision` / `4e0983094f3c` | Live objective edit; 46,856 ignored entries | Canonical verified P1 integration line; promoted by fast-forward and switched to `master` | **KEEP** |
| `D:\Stuff\DevFolder\Smash64DS_Port\.tura\control-task8-cut-e` | detached `3155901cdd7e` | Untracked 6,130-byte runtime log; 5 ignored entries | Cut E is an ancestor of the integration line; its measurements are already in `PERF_LEDGER.md` | **HOLD**, then remove after the controller is stopped and the log/evidence is explicitly discarded or archived |
| `D:\Stuff\DevFolder\Smash64DS_Port-worktrees\attack` | `codex/five-effects-attack` / `e56003c2adab` | Git-clean; 1,192 ignored entries | Attack/Tornado audio feasibility is represented by the larger current pack | **REMOVE-CANDIDATE** after ignored-data decision |
| `D:\Stuff\DevFolder\Smash64DS_Port-worktrees\audio-union` | `codex/five-effects-audio-union` / `cb7b70c89579` | Git-clean; 1,188 ignored entries | Its only patch is equivalent to integrated `7cd139af32`; pack blobs match | **REMOVE-CANDIDATE** after ignored-data decision |
| `D:\Stuff\DevFolder\Smash64DS_Port-worktrees\countdown-refine` | `codex/countdown-reference-v2` / `f9f0deda5a4d` | Git-clean; 3,015 ignored entries | Source-decoded hybrid countdown is already integrated; inherited audio is superseded | **REMOVE-CANDIDATE** after ignored-data decision |
| `D:\Stuff\DevFolder\Smash64DS_Port-worktrees\fox` | `codex/five-effects-fox` / `636737c555fb` | Git-clean; 1,178 ignored entries | Fox cue behavior is semantically present in the unified current pack | **REMOVE-CANDIDATE** after ignored-data decision |
| `D:\Stuff\DevFolder\Smash64DS_Port-worktrees\hit` | `codex/five-effects-hit` / `85d0ac30202b` | Git-clean; 1,189 ignored entries | Fail-closed hit contracts and exclusions are present in current | **REMOVE-CANDIDATE** after ignored-data decision |
| `D:\Stuff\DevFolder\Smash64DS_Port-worktrees\integration` | `codex/five-effects-integration` / `7cd139af32f2` | Git-clean; 1,148 ignored entries | Exact ancestor merged by `0feaf5d84f` | **REMOVE-CANDIDATE** after ignored-data decision |
| `D:\Stuff\DevFolder\Smash64DS_Port-worktrees\mario` | `codex/five-effects-mario` / `c76d0d1022f2` | Git-clean; 1,837 ignored entries | Mario cue exclusions and crisp IFCommon alpha are integrated/evolved | **REMOVE-CANDIDATE** after ignored-data decision |
| `D:\Stuff\DevFolder\Smash64DS_Port-worktrees\visual` | `codex/five-effects-visual` / `4d23dedf6ca3` | Git-clean; 1,190 ignored entries | Both bounded effects and source RNG scatter patches are equivalent in current | **REMOVE-CANDIDATE** after ignored-data decision |
| `D:\Stuff\DevFolder\Smash64DS_Port-worktrees\visual-control` | detached `701759abcdf1` | Git-clean; 574 ignored entries | Exact ancestor; head is a tree-empty completion marker | **REMOVE-CANDIDATE** after ignored-data decision |
| `D:\Stuff\DevFolder\Smash64DS_Port-wt-audio` | `codex/p1-audio-ids` / `4891bacb0083` | Git-clean; 2,258 ignored entries | Current supersedes its 11-entry pack; the branch retains incomplete pitch/fork-voice evidence for known debt | Remove worktree after ignored-data decision; **KEEP-REF** |
| `D:\Stuff\DevFolder\Smash64DS_Port-wt-gameplay` | `codex/p1-fireball` / `534f032bc19b` | Git-clean; 1,680 ignored entries (~157 MB) | Early partial Fireball renderer is superseded by the natural current gate and stronger lifetime/collision proof | **REMOVE-CANDIDATE** after artifact decision |
| `D:\Stuff\DevFolder\Smash64DS_Port-wt-m4-aot` | `codex/m4-aot-generator` / `601f2be4c378` | Git-clean; 2,253 ignored entries | Absent runtime code is the rejected 1.56 MB animated-water path; it violates frozen frame-0 P1 policy | **REMOVE-CANDIDATE; DO NOT MERGE** after ignored-data decision |
| `D:\Stuff\DevFolder\Smash64DS_Port-wt-root-countdown-review` | detached `1b61f7eff84d` | Git-clean; no ignored entries | Early countdown patches are equivalent and then superseded | **REMOVE-CANDIDATE; SAFE FIRST REMOVAL** |
| `D:\Stuff\DevFolder\Smash64DS_Port-wt-soak` | `codex/renderer-parity-contract` / `54379201b245` | Git-clean; 1,685 ignored entries (~170 MB) | Only absent code is the rejected whole-owner transaction prototype; current owner fixtures replace its useful contract | **REMOVE-CANDIDATE** after artifact decision |
| `D:\Stuff\DevFolder\Smash64DS_Port_baseline_audit` | detached `50d8e87d8e8d` | Git-clean; 566 ignored entries (~71 MB) | Source is a reachable ancestor; ignored output includes the recorded `E08C6C9E...` compile-checkpoint ROM | **HOLD** until that binary evidence is archived or explicitly discarded |

## Branch Progress

| Local branch | Audited head | Progress / purpose | Recommendation |
|---|---|---|---|
| `master` | `d2993b7ec84c` | Was 233 commits behind with no unique commits; consolidation target | **KEEP**, fast-forward to the consolidation checkpoint |
| `codex/wip-natural-combat-source-start-collision` | `4e0983094f3c` | Sole authoritative integration line before consolidation | Delete ref after `master` is confirmed at the same checkpoint |
| `codex/five-effects-integration` | `7cd139af32f2` | Ancestor of integration | Delete after its worktree is removed |
| `codex/five-effects-audio-union` | `cb7b70c89579` | Patch-equivalent integration alternative | Delete after its worktree is removed |
| `codex/five-effects-attack` | `e56003c2adab` | Superseded audio feasibility branch | Delete after its worktree is removed |
| `codex/five-effects-fox` | `636737c555fb` | Superseded Fox cue branch | Delete after its worktree is removed |
| `codex/five-effects-hit` | `85d0ac30202b` | Superseded hit-cue branch | Delete after its worktree is removed |
| `codex/five-effects-mario` | `c76d0d1022f2` | Integrated/evolved Mario audio and IFCommon alpha | Delete after its worktree is removed |
| `codex/five-effects-visual` | `4d23dedf6ca3` | Patch-equivalent effects/RNG work | Delete after its worktree is removed |
| `codex/countdown-reference-v2` | `f9f0deda5a4d` | Integrated hybrid countdown plus superseded audio | Delete after its worktree is removed |
| `codex/p1-audio-ids` | `4891bacb0083` | Older implementation, but preserves evidence for exact pitch schedules and fork voice 685 | **KEEP-REF**, do not merge wholesale; remove only its worktree |
| `codex/p1-fireball` | `534f032bc19b` | Superseded early Fireball hardware path | Delete after its worktree/artifacts are resolved |
| `codex/m4-aot-generator` | `601f2be4c378` | Rejected animated-water runtime | Delete after its worktree is removed |
| `codex/renderer-parity-contract` | `54379201b245` | Rejected transaction prototype; useful contract evolved in current | Delete after its worktree/artifacts are resolved |
| `codex/p1-five-minute-soak` | `1edb09dc2784` | Obsolete five-minute policy and older countdown/lifecycle variants | Delete; no worktree is attached |
| `codex/wip-hw-visibility-gates` | `cc340aa5305b` | Fully contained early visibility work | Delete; no worktree is attached |
| `codex/wip-renderer-fidelity-2026-07-09` | `24b95d43f35e` | Fully contained renderer fidelity history | Delete after `master` promotion |
| `codex/wip-renderer-sampler-boundary-20260710` | `98b4851bf3c6` | Fully contained earlier renderer subset | Delete after `master` promotion |
| `wip/ftmain-import` | `8273c5c9dd87` | Fully contained source-faithful import milestone | Delete; no worktree is attached |

## Merge and Issue Decisions

- Audio side merges were rejected: alternative attack/Fox/hit branches conflict
  heavily with the current unified pack. `codex/p1-audio-ids` would regress 18
  entries to 11 and its unique pitch/fork code is not naturally qualified.
- The M4 animated-water branch is contrary to the explicit frozen-water contract.
- The renderer transaction prototype was measured as a 124,288-tick regression
  and is already recorded as rejected in `PERF_LEDGER.md`.
- The stale handoff branch and its over-100-line failure were repaired by naming
  `master`, removing obsolete merge guidance, and shortening duplicated text.

## Removal Recommendation

1. Remove `Smash64DS_Port-wt-root-countdown-review` first; it is the only unused
   worktree with neither tracked/untracked changes nor ignored files.
2. Stop the Task-8 controller, then decide whether to archive or discard its
   untracked runtime log and five ignored evidence files before removal.
3. For every other remove-candidate, preserve any unique ROM hash, performance
   JSON, or dated visibility capture (or explicitly approve its loss). Rebuild
   directories, verifier temp files, logs, and emulator copies are reproducible.
4. Hold `Smash64DS_Port_baseline_audit` until the
   `E08C6C9EA29F671EE5AA9D9D6491B1B12E80A1DBC348AF99468CA72BE072425F`
   ROM evidence is archived or explicitly discarded.
5. Remove the `p1-audio-ids` worktree but retain its branch as evidence. After
   all other worktrees are removed, delete the redundant refs listed above and
   prune worktree metadata. Keep only `master` plus the one evidence branch.

Actual deletion is intentionally deferred because the request asked for a
removal recommendation, and ignored/untracked evidence is not recoverable from
the Git refs.
