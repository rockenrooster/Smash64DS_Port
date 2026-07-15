---
name: smash64ds-integrate-checkpoint
description: Integrate and publish a completed Smash64DS work packet after it has passed its first falsifier. Use only from the integration/release lane to review ownership, reproduce evidence, merge a coherent candidate, run the smallest current promotion gates, update central truth docs and artifact identity, and optionally create the final Lean snapshot. Never execute retired list-only profiles and never snapshot before all work is complete.
---

# Objective

Convert a worker result into one reproducible accepted checkpoint, or reject it
without contaminating main, central docs, or published ROM identity.

This skill is integration-lane only. It is intentionally explicit-invocation
only.

# 1. Validate the returned packet

Require:

- packet ID and P1 row;
- worker branch/worktree and commit/patch identity;
- exact files changed and proof they stayed within ownership;
- hypothesis, baseline, first falsifier, and decision-grade evidence;
- build/profile/mode plus ROM/ELF hashes;
- focused/static/host/runtime commands and results;
- semantic/state/capture/resource evidence;
- temporary selectors/probes/fallbacks and cleanup status;
- unresolved risks and worker KEEP/REWORK/REVERT recommendation.

Reject an incomplete return before merging. Do not reconstruct missing evidence
from a worker’s prose claim.

# 2. Review scope and shared seams

1. Reconcile main and all in-flight work.
2. Inspect the full diff and focused diffs around shared interfaces.
3. Confirm no `decomp/` edits, unrelated reformatting, generated-artifact drift,
   verifier weakening, or central-doc edits from a worker lane.
4. Check source semantics for behavior changes and the native owner contract for
   renderer changes.
5. Confirm temporary lab selectors and high-frequency probes are removed or
   compiled out of the production path.
6. Merge/cherry-pick only a coherent, reviewable change. Resolve shared seams in
   the integration lane.

# 3. Reproduce before widening

Re-run the smallest evidence that can invalidate the packet:

- host/static exactness;
- first runtime falsifier;
- one synchronized capture/state check;
- affected resource/fallback counters.

Then invoke `$smash64ds-verifier-router` on the integrated diff. Use the actual
wrappers and registry. In the current two-ROM workflow, never execute or
prebuild `Full`, `P1Gate`, `Regression*`, or another profile that the live
scripts mark list-only. Inventory via `-List` is not a passing gate.

For performance candidates, reproduce phase-aware P50/P95 from the exact
integrated ROM. For gameplay candidates, prove continuous natural mode-163
behavior. For audio, preserve cadence/underrun/reserve and required audible or
PCM/channel evidence. For renderer changes, require the applicable independent
profile-2 oracle/state/capture contract after the live performance gate.

# 4. Refresh project truth

Only after integrated evidence passes:

- update the P1 board row and lane state;
- keep `docs/STATUS.md` and `docs/HANDOFF.md` within the repo line limits;
- append decision-grade measurements to `docs/PERF_LEDGER.md`;
- append durable implementation history to `docs/PORTING.md`;
- update the optimization/native plan only when its contract changed;
- remove stale commands that conflict with current scripts;
- refresh canonical/shipped ROM size and SHA only after the final relevant build;
- record exact capture/log/artifact paths and hashes.

Do not duplicate long marker strings or old increment stacks across active docs.

# 5. Final artifact and snapshot discipline

Before publication or snapshot:

1. Inspect `git status --short` and the final diff.
2. Confirm both permitted root ROMs and the canonical publication/parity rule.
3. Confirm no laboratory ROM published the shipped filename.
4. Run every currently executable required focused/canonical gate in stop order.
5. Confirm final exact-ROM identity, required dated capture/manual evidence,
   memory reserve, audio safety, lifecycle, and unresolved red rows.
6. Commit the coherent checkpoint when that is part of the user’s workflow.

Run:

```powershell
.\scripts\New-Smash64DSSnapshot.ps1 -Mode Lean
```

only when the user requested a completed checkpoint/snapshot and all code, docs,
checks, verifier runs, artifact refreshes, and status inspection are finished.
It must be the final project command. Run no command afterward; only report the
result.

# 6. Decision

Return one of:

- `ACCEPTED`: integrated commit/artifact identities and evidence;
- `REWORK`: one named residual blocker and the packet returned to a lane;
- `REJECTED`: why the result failed and what runtime portion was removed while
  preserving useful exact fixtures;
- `BLOCKED`: a genuinely external/user qualification item, without claiming the
  row green.
