---
name: smash64ds-harden-platform-input-irqs
description: Analyze, implement, optimize, or debug Smash64DS_Port platform glue, startup, main loop, input polling, controller mapping, VBlank or other interrupts, volatile/shared state, BIOS waits, freeze behavior, display initialization, lid/power/RTC/microphone/Slot-2/Wi-Fi integration, or other low-level DS services not owned by a more specific skill. Use for nondeterminism, freezes, missed/stuck input, interrupt races, startup failures, or new DS platform subsystem integration.
---

# Harden Platform, Input, and Interrupts

## Mission

Keep the DS host deterministic, interrupt-safe, and minimal. Platform code should initialize hardware once, poll or service at defined seams, and avoid hidden per-frame work.

## Owning seams

Use CodeGraph, then inspect:

- `src/nds/nds_platform.c`;
- `include/nds/nds_platform.h`;
- `src/nds/nds_controller.*` and controller compatibility declarations;
- startup/boot/video/freeze diagnostic modules under `src/nds`;
- VBlank counters and presentation scheduling;
- ARM7/shared-state users;
- local DS references for the same hardware service.

Route audio, GX, 2D, memory, DMA, and math work to their dedicated skills.

## Workflow

1. **Reproduce deterministically.**
   Record ROM hash, boundary/mode, input sequence, frame/tick, and whether the failure is cold-boot, transition, steady-state, or shutdown.

2. **Trace ownership.**
   For every shared variable or hardware register, identify:
   - writer context: main, VBlank IRQ, timer IRQ, ARM7, DMA;
   - reader context;
   - update cadence;
   - atomic width/alignment;
   - required ordering;
   - reset/lifetime.

3. **Input rules.**
   - Poll hardware once at the established update seam.
   - Derive held/pressed/released edges from one coherent sample.
   - Map DS buttons to source controller semantics explicitly.
   - Do not read hardware independently from multiple gameplay systems.
   - Preserve two-updates-per-present behavior and edge semantics.
   - Test pause, simultaneous buttons, rapid taps, held inputs, lid/resume if supported.

4. **Interrupt rules.**
   - Keep handlers bounded and non-blocking.
   - Do not allocate, perform file I/O, print, or wait in an IRQ.
   - Acknowledge hardware through established APIs.
   - Use volatile only for genuinely shared state; volatile is not a lock or memory barrier.
   - Protect multiword or compound state with a brief critical section or sequence protocol.
   - Avoid main/IRQ ownership of the same hardware unit without arbitration.
   - Keep counters monotonic and overflow-aware.

5. **Wait/pacing rules.**
   - Use the established VBlank/presentation scheduler.
   - Do not add busy waits.
   - Distinguish intentional `swiWaitForVBlank` idle from work.
   - Do not "fix" a freeze by skipping synchronization without tracing the owner.
   - Load the frame-pacing skill for timing changes.

6. **Startup/display rules.**
   - Initialize banks, engines, IRQs, filesystems, audio, and scene state in a defined order.
   - Fail closed with a visible/diagnosable result.
   - Avoid reinitializing global hardware on ordinary scene transitions.
   - Test repeat entry/exit and reset paths.
   - Treat one-frame flashes or corrupt initialization as failures.

7. **New peripheral work.**
   Wi-Fi, RTC, microphone, Slot-2, lid, and power-management features must:
   - have a concrete project requirement;
   - identify ARM7/firmware dependencies;
   - define ownership and sleep/resume behavior;
   - remain outside the active battle loop unless needed;
   - include a hardware-specific test plan.

## Freeze/nondeterminism checklist

Check:

- stale or torn shared state;
- IRQ enabled before state initialization;
- unacknowledged interrupt;
- nested/long handler;
- waiting for ARM7/DMA/GX from an IRQ;
- queue full/empty race;
- generation counter wrap or stale handle;
- cache maintenance around shared/DMA buffers;
- stack overflow;
- error path that leaves hardware half-configured;
- debug print from timing-sensitive context.

## Verification

- Use focused controller/startup/freeze checks.
- Run the relevant boundary verifier after kept changes.
- Capture counters or traces that prove the race/state transition.
- Test cold boot and at least one restart/scene transition.
- Use device testing for firmware, lid, power, Wi-Fi, microphone, or timing-sensitive IRQ behavior.

## Required result

Report:

- owner/read/write context table;
- failing sequence;
- root race/state-machine cause;
- synchronization or ownership change;
- input/timing compatibility result;
- cold boot/restart result;
- verifier result;
- device test requirement;
- KEEP, REVERT, STOP, or WIP.
