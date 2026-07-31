---
name: smash64ds-audit-ds-subsystem-change
description: Review an existing Smash64DS_Port low-level Nintendo DS change or agent reply across subsystem boundaries. Use when evaluating a patch, proposed task, implementation report, or performance claim involving ARM9, memory, GX, 2D/OAM, DMA/I/O, audio/IPC, math, timers, or platform code. Checks ownership, hardware assumptions, evidence quality, regressions, verification, and whether the change follows the project's fastest-correct-DS contract.
---

# Audit a DS Subsystem Change

## Mission

Review the implementation, not the confidence of the report. Reconstruct the live mechanism from code and evidence, then issue a concrete accept/revise/reject result.

## Inputs

Accept any combination of:

- branch/diff/commit;
- agent report;
- task file;
- performance artifact;
- screenshots;
- verifier output;
- current working tree.

Use CodeGraph first to trace changed symbols and their callers. Read `AGENTS.md`, the project contract, current board/handoff, standing rules, and verifier ownership.

## Audit sequence

1. **Scope and ownership**
   - What subsystem does the patch claim to change?
   - Which subsystem actually owns the state/cost?
   - Did the patch touch the owning seam or mask it elsewhere?
   - Are changes confined to allowed port-side paths?

2. **Source/reference fidelity**
   - Was BattleShip inspected for behavior?
   - Were local DS references inspected for substantial backend/hardware architecture?
   - Is mechanical equivalence named?
   - Are rendering approximations explicitly budgeted?

3. **Hardware correctness**
   Select and load the relevant domain skill. Check:
   - alignment/range/lifetime;
   - cache/DMA coherency;
   - IRQ/main/ARM7 ownership;
   - matrix/primitive/state semantics;
   - VRAM banks and display layers;
   - channel/handle generations;
   - fixed-point overflow/rounding;
   - VBlank/presentation sequencing.

4. **Performance mechanism**
   - Is there a measured baseline owner?
   - Is the proposed currency proven?
   - Is the A/B from one tree with one changed flag?
   - Did the candidate engage?
   - Are `WORK-H` P50 and P95 reported?
   - Is `ALL` interpreted correctly?
   - Is frame-sign distribution shown for surprising deltas?
   - Is device-only work queued rather than overclaimed from emulator data?

5. **Correctness and fidelity evidence**
   - Appropriate focused checker;
   - one widest relevant verifier;
   - screenshot/differ for visual output;
   - owner listen test for audio quality;
   - cold-boot/restart for lifetime/platform changes;
   - no unexplained hashes/state differences;
   - no silent fallback or content removal.

6. **Project hygiene**
   - task flag default and published/tick-HUD parity;
   - temporary probes removed;
   - generated outputs not hand-edited;
   - no obsolete proof branch retained;
   - docs updated only in owning locations;
   - repository push policy respected;
   - final snapshot rule respected.

## Severity

Classify findings:

- **Blocker** — corruption, nondeterminism, wrong gameplay, missing content, invalid evidence, unsafe hardware ownership, or unverifiable patch.
- **Major** — mechanism not proved, wrong metric, device-only overclaim, stale fallback, incomplete published/tick-HUD integration.
- **Minor** — maintainability or evidence clarity issue that does not invalidate the result.
- **Observation** — useful next experiment, not required for current correctness.

## Review output

Start with findings ordered by severity. For each finding include:

- exact file/symbol or report claim;
- why it is wrong/risky;
- concrete correction;
- required proof.

Then include:

- mechanism reconstruction;
- evidence table;
- missing gates;
- final verdict: ACCEPT, ACCEPT WITH FOLLOW-UP, REVISE, or REJECT.

Do not rewrite the entire task unless the existing plan is structurally unsalvageable.
