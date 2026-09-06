# Input, IRQs, Timers, and the Frame Loop

## Sample once; consume edges once

At a defined update boundary, call `scanKeys()` once and distribute a snapshot
of `keysDown()`, `keysHeld()`, `keysUp()`, and valid touch data. Do not rescan in
each subsystem. The complete typed example is `../examples/frame_loop.c`.

For catch-up updates, distribute each input edge once, not once per repeated
simulation step. Define the logical tick rate separately from display refresh.
Keep gameplay timers and audio events in logical units; cap catch-up after a
stall and define pause/sleep behavior. A lower rendering rate must not silently
change gameplay or duplicate collision/audio work.

## Simple frame-loop shape

```c
while (pmMainLoop()) {
    /* One input snapshot, simulation, shadow BG/OAM preparation. */
    /* Submit GX once, with one finalization owner when 3D is used. */
    swiWaitForVBlank();
    /* Bounded BG/OAM/register commits, for initialized owners only. */
}
```

This is a shape, not a universal pipeline order. Follow the actual renderer's
presentation contract. `pmMainLoop()` is the current Calico lifecycle guard;
other SDKs have their own path. A wait for the next VBlank does not prove the
preceding work met the deadline. For performance work, distinguish active
processing, hardware waits, VBlank waiting, and missed presentation intervals.

## VBlank is a window; an ISR is an execution context

A main-thread update performed after a VBlank wait is **not** an IRQ handler.
Prepare shadow state before the commit window. Keep OAM/palette/register commits
bounded; large uploads need a deliberate loading/blanking or multi-frame schedule.
VBlank does not make an arbitrarily long copy safe or free.

Use an ordinary thread waiting for VBlank when that is all the task requires.
Add a custom ISR only for an actual hardware event/capture requirement.

## Calico scheduling facts that prevent hangs

Calico is priority-preemptive without timeslicing. Numerically lower priority
values run ahead of higher values. A worker must block when idle; a high-priority
poll loop can starve the consumer it is waiting for. `threadYield()` only helps
share with equal-priority runnable threads; it does not yield to lower priority.

Native thread setup uses `threadPrepare`, an 8-byte-aligned **stack top**, then
`threadStart`. A worker using libc or TLS needs attached local storage. With
`threadAttachLocalStorage(&thread, NULL)`, it consumes the worker's own stack;
budget `threadGetLocalStorageSize()` plus call depth and runtime overhead.
Standard POSIX/C/C++ threading interfaces are also supported on the current
stack. Do not reject threads merely because legacy DS programs avoided them.

## IRQ contract

Calico handlers, tick callbacks, and PXI callbacks execute in IRQ mode, on a
separate stack, without nesting. They must not use libc, TLS, allocation,
filesystem access, blocking, or ordinary thread calls. The documented exceptions
include `threadUnblock*` and `mailboxTrySend` to wake a worker.

A useful handoff is a mailbox token. The ISR calls `mailboxTrySend`; the worker
blocks on `mailboxRecv` and does the work outside IRQ context. Define what happens
when the mailbox is full: a wake hint may coalesce, but a lossless input record
or audio buffer completion must not silently disappear. Never retry until success
inside the IRQ.

`../examples/irq_worker.c` demonstrates a one-slot coalescing wake-up, worker TLS,
priority selection, and stop/join ordering. It is not a lossless event queue.
Keep the mailbox, stack, and thread alive until the worker has stopped.

## Same-CPU snapshots

For a naturally aligned counter with one IRQ writer, a same-CPU reader can take
a word-sized sample when wrap is tolerated. For several related fields, the
simple starting point is a short `irqLock()` / `irqUnlock(saved_state)` copy;
see `../examples/irq_handoff.c`. Restore the previous interrupt state rather than
blindly enabling interrupts. Keep the protected region to the copy only.

A sequence lock or a double buffer can be justified later, but it adds ownership
rules. `volatile struct` alone does not make a coherent multiword snapshot. A
same-CPU critical section also does **not** stop ARM7 or perform cache maintenance.

## Source enable is separate from registration

`irqSet` installs a handler and `irqEnable` enables the controller bit. LCD source
configuration is separate, for example through `lcdSetIrqMask`,
`lcdSetHBlankIrq`, or `lcdSetVCountCompare` as appropriate. Preserve other owners'
mask bits, handlers, and runtime waits. Do not disable VBlank globally when a
thread still relies on `threadWaitForVBlank()` for progress or shutdown.

## Timers and profiling

Calico owns timers 2 and 3. `tickGetCount()` from `<calico/system/tick.h>` supplies
a monotonic tick counter at `TICK_FREQ`, **not** the ARM9 clock rate. Do not
reinitialize the runtime's tick machinery. Use tick tasks or the thread timer
API for periodic activity, remembering that tick-task callbacks themselves are
IRQs. For finer timing, reserve a genuinely free timer (consider 0/1), account
for its prescaler/rollover, and inspect the project's measurement conventions.

A timer's ownership, units, and wrap behavior can live in its wrapper. Do not
write a new timer inventory for every input or frame-loop change. When reading
cascaded counters, handle rollover between low/high reads.

## Lifecycle and diagnostics

On scene teardown, stop event producers before reclaiming their queues or
worker storage. Drain/stop/join as required. Use runtime sleep/exit handling;
reestablish only the state the application owns on resume or repeated entry.
Reset input edges deliberately after a pause.

For a timing investigation, separate update, preparation, submission, transfer,
VBlank wait, and overruns. For a simple feature, compile and exercise that feature
without introducing a profiler. Common faults are duplicated input edges,
unbounded catch-up, a worker polling at the wrong priority, disabling a wake IRQ,
or destroying a mailbox while its thread still waits on it.

## Targeted review

- Input edges are consumed once; logical and presentation rates are deliberate.
- IRQ work is bounded and uses only documented IRQ-safe operations.
- Workers block; priority, TLS, queue-full behavior, and stop/join are correct.
- Snapshot critical sections are short and restore prior state.
- Timer ownership, tick units, and LCD source/controller enables are correct.

API sources and compact lookup: `17-libnds2-calico-facts.md`.
