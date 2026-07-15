---
name: smash64ds-verifier-router
description: Choose the smallest currently executable Smash64DS verification path for a diff or candidate. Use before copying commands from docs, before an emulator run, after a verifier failure, or when deciding between focused checks, DevFast, Boundary, and Current. Reads actual wrappers and registry behavior, rejects retired list-only profiles, checks build/ROM freshness, and emits exact stop-ordered commands. Does not snapshot or claim acceptance.
---

# Objective

Minimize verification latency without skipping the evidence required by the
actual changed surface.

# 1. Establish command truth

Read:

- `AGENTS.md` verifier policy;
- `scripts/verify-all.ps1` around profile validation;
- the wrapper being considered (`verify-dev-fast.ps1`,
  `verify-boundary.ps1`, `verify-current.ps1`, or a focused verifier);
- `scripts/lib/harness-registry.ps1` only when membership or mode ownership is
  relevant.

Executable script behavior decides whether a command can run. Documentation may
explain intent, but it cannot override a wrapper that throws.

In the July 15 two-ROM workflow, treat `Full`, `P1Gate`, `Regression*`, and the
other legacy multi-ROM profiles as inventory-only unless the live scripts no
longer reject execution. `-List` may inspect membership; it is not a passing
runtime gate.

# 2. Classify the change

Use the diff and work packet to select the lowest sufficient tier.

## Static or host-only change

Examples: generator, fixture, manifest, documentation, or pure host analysis.
Run the focused static/host check. Do not build or launch the ROM unless the
output is linked or changes runtime behavior.

## Isolated runtime subsystem

Run its focused verifier in canonical mode 163 or the existing natural-runtime
route. Prefer `-NoBuild` only when the exact required ROM was built from the
current source and its hash/stamp is known.

## Renderer performance candidate

Use the focused host fixture and profile-1 lab falsifier first. Profile-1 lab
ROMs never publish the shipped filename. Build profile 2 only after the live
performance gate is met. Run renderer semantic/oracle/state/capture checks before
broad runtime gates.

## Finished runtime slice

Run the focused verifier, then `verify-dev-fast.ps1`, then
`verify-boundary.ps1` when the slice affects the canonical one-minute scene.

## Original launch path or shared startup/runtime

Add `verify-current.ps1 -Build` after focused and DevFast/Boundary checks.

## Release/integration checkpoint

Use only profiles that the current scripts can execute. Include exact artifact
identity, parity, required focused captures/soaks, and current canonical routes.
Never resurrect a retired profile just because an older release checklist names
it.

# 3. Preserve fast iteration

- Use one incremental build per source revision.
- Reuse `-NoBuild` for repeated measurements of the same exact ROM.
- Never combine `-Build` and `-NoBuild`.
- Do not run clean or forced matrix builds unless a current script or diagnosed
  stale-artifact condition requires it.
- Stop on the first deterministic failure; do not bury it under later gates.
- Retry a transport-class failure at most once and only through the project’s
  supported retry behavior. Do not relabel a semantic mismatch as transport.
- Keep runner slots, build directories, and GDB ports isolated per lane.

# 4. Validate artifact freshness

Before `-NoBuild`, record or confirm:

- source/commit plus dirty-tree identity;
- target and build directory;
- ROM path, timestamp, size, and SHA-256;
- required ELF/map identity when applicable;
- profile and runtime selector;
- expected canonical versus laboratory filename.

Reject stale or cross-configuration evidence.

# 5. Report workflow drift

Search active docs for commands that the live wrappers reject. Report the exact
file/section and the valid replacement route. Do not edit central docs from a
worker lane.

# 6. Output

Return a stop-ordered plan:

```text
CHANGE CLASS:
ARTIFACT TO PROVE:
BUILD NEEDED: yes/no and why
1. command — purpose — stop condition
2. command — purpose — stop condition
...
NOT RUN:
RETIRED/INVALID COMMANDS FOUND:
EVIDENCE TO RETURN:
```

This skill routes verification; it does not perform release publication or run
the Lean snapshot.
