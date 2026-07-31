---
name: smash64ds-measure-frame-pacing
description: Measure, attribute, and review Nintendo DS frame pacing and performance in Smash64DS_Port, including hardware timers, VBlank intervals, WORK/WAIT/WORK-H buckets, P50/P95 tails, subsystem counters, profiler windows, emulator versus device classification, and controlled A/B design. Use whenever FPS, ticks, VBlank slips, performance regressions, instrumentation, or optimization verdicts are involved.
---

# Measure Frame Pacing

## Mission

Produce evidence that identifies work, separates steady-state from tails, and supports a reversible KEEP/REVERT verdict.

## Canonical defaults

Unless the task explicitly owns another scene-level capability:

- Boundary: `battle_playable_realtime`
- Mode: `163`
- Scenario: Mario human versus imported level-3 Fox CPU, Dream Land, items off, one-minute Time mode
- Target: stable 30 FPS
- Nominal gate: `WORK-H` P95 at or below the project target in `PROJECT_GOAL.md`

Read the current board/handoff and standing rules because metric ownership can evolve.

## Metric rules

- **Search on `WORK-H` P50.**
- **Gate on `WORK-H` P95.**
- Report the P50-to-P95 spread.
- Use `ALL` only to confirm a whole-VBlank boundary was crossed.
- `ALL` contains VBlank wait and is quantized.
- Same-ROM repeat runs in the current harness are deterministic.
- Build-to-build placement variance is real; vary one build flag in the same tree.
- A same-sign delta on nearly every frame is a mechanism, not noise.
- Never use min FPS or a short average as the primary device verdict.
- Device reports use normalized 2/3/4/5+ VBlank-interval histogram and max interval.

## Workflow

1. **Define the question.**
   Example: "Does removing the stage segment stats copy reduce stage work?" not "Is the stage faster?"

2. **Name the expected currency.**
   Calls, bytes, vertices, runs, FIFO words, cache misses, loads, transforms, copies, or service ticks.

3. **Choose the smallest instrument.**
   - cumulative counter sampled as a two-stop delta;
   - bounded request trace;
   - phase timer;
   - per-PC profiler window;
   - synchronized screenshot;
   - typed tick bucket.
   Do not add a large debug wall.

4. **Control observer effect.**
   - Compile diagnostics out at zero.
   - Keep both A/B arms equally instrumented.
   - Never measure a full-console redraw as shipping work.
   - Record BSS/code-size changes that may alter placement.
   - Remove temporary probes before handoff unless they are proven shared diagnostics.

5. **Build A/B from one tree.**
   - One flag or isolated edit.
   - Exact same ROM configuration and capture window.
   - Record hashes and build flags.
   - Confirm the candidate feature engaged.

6. **Interpret by pattern.**
   - P50 improves, P95 flat: steady-state win but tail remains.
   - P50 flat, P95 improves: tail/burst win.
   - owner bucket improves, WORK-H flat: another owner or placement offset absorbed it.
   - `ALL` flat, WORK-H improves: expected below a VBlank boundary.
   - all frames regress same sign: real added work or layout collateral.
   - max-only spike with load event: I/O tail, not steady-state CPU.
   - counter improves but ticks do not: wrong currency or non-dominant cost.

7. **Classify platform evidence.**
   - Work removal visible in the custom accuracy emulator can be melonDS-sufficient.
   - Cache/TCM, DMA, card timing, and VBlank-edge pacing are device-only.
   - Queue device-only A/Bs for the batched checkpoint.

## Instrumentation ownership

Use or extend existing focused facilities rather than creating parallel telemetry:

- tick HUD and typed buckets;
- platform VBlank/tick counters;
- task-specific default-off Makefile flags;
- per-PC census tooling;
- screenshot capture and analysis;
- permanent evidence under `artifacts/performance` or `artifacts/visibility`.

Do not promote a lab flag into shipping behavior accidentally.

## Verification and closeout

- Use the smallest focused checker while editing.
- Run the widest relevant verifier once for a kept checkpoint.
- Update handoff/porting only with concise durable truth.
- Follow branch/merge, push, and snapshot rules from `AGENTS.md` and standing rules.

## Required performance table

Include:

| Metric | Control | Candidate | Delta | Interpretation |
|---|---:|---:|---:|---|
| WORK-H P50 | | | | |
| WORK-H P95 | | | | |
| owner P50/P95 | | | | |
| VBlank 2/3/4/5+ | | | | |
| mechanism counter | | | | |

Also report frame-sign distribution, ROM hashes, flags, verifier result, and device classification.

End with one verdict: KEEP, REVERT, STOP, or WIP.
