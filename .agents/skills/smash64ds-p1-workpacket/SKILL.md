---
name: smash64ds-p1-workpacket
description: Select and define the next highest-impact unowned Smash64DS P1 work packet. Use when deciding what to do next, assigning a subagent or worktree, recovering an idle lane, or splitting a large blocker. Produces disjoint file ownership, build and runner allocation, one hypothesis, a first falsifier, stop gates, and an evidence-return contract. Do not implement the packet.
---

# Objective

Turn the live P1 queue into one bounded packet that can be executed in parallel
without shared-file churn or ambiguous acceptance.

# 1. Reconcile current truth

Read only the minimum authority needed to select work:

1. `AGENTS.md`.
2. `docs/P1_EXECUTION_BOARD.md`.
3. `git status --short`, `git diff --stat`, and recent commits.
4. The one technical contract relevant to the candidate:
   `docs/optimization/NATIVE_RENDERER_PLAN.md`, BattleShip source, or the focused
   gameplay/audio document.
5. `docs/PERF_LEDGER.md` only for the candidate keywords and rejected ancestors.
6. The executable verifier wrappers/scripts that the packet expects to use.

Do not trust an active-looking command merely because it appears in a document.
If a wrapper throws or the live verifier marks a profile list-only, route around
it and report the documentation drift.

# 2. Rank candidate packets

Consider only unowned red or reopened P1 rows. Rank them by:

1. Release-blocking value.
2. Measured performance ceiling or amount of missing user-visible behavior.
3. Readiness for a fast first falsifier.
4. Ability to use a disjoint additive surface.
5. Integration and verifier cost.
6. Risk of colliding with an active lane.

Do not select:

- an already-owned surface;
- P2 work while a required P1 row is red;
- a renderer micro-optimization below the live minimum useful ceiling;
- a previously rejected architecture without a materially different cost model;
- tooling work that does not shorten an active P1 path;
- a new harness when mode 163 or an existing focused gate can prove the result.

# 3. Design the packet

Use `references/work-packet-template.md`. Every packet must name:

- board row and packet ID;
- one outcome, not a broad theme;
- source-of-truth behavior or renderer contract;
- one causal hypothesis;
- measured upper bound or functional closure value;
- first falsifier and exact stop condition;
- KEEP / REWORK / REVERT thresholds from the live board/plan;
- owned files/directories and explicitly forbidden shared files;
- branch/worktree, build directory, target/profile/mode, runner slot, and ports;
- smallest focused checks before any emulator run;
- evidence that must be returned to integration;
- integration seam required before the worker starts;
- cleanup obligations for selectors, probes, temporary fixtures, or obsolete
  modes.

Workers should normally own additive modules, generators, fixtures, or focused
runtime files. The integration/release lane owns `Makefile`, shared headers,
central registration seams, verifier registry, active truth docs, publication,
and final artifact identity unless the board explicitly says otherwise.

# 4. Use subagents safely

Keep three subagent slots active only when three useful independent packets
exist. Prefer parallel work for:

- source or codebase exploration;
- host fixture generation;
- log/ledger analysis;
- disjoint additive modules;
- static verifier development on isolated files.

Do not run parallel write-heavy work against the same renderer core, shared
header, Makefile, registry, active docs, build directory, or emulator slot.

A worker returns a coherent commit or patch plus the evidence packet. It does
not rewrite central status documents or publish ROMs.

# 5. Output

Return:

1. the selected packet;
2. why it outranks the next two candidates;
3. the exact ownership and resource allocation;
4. the first command/evidence sequence;
5. the stop/return conditions;
6. any authority or workflow drift that integration must fix.

Do not start implementation while using this skill.
