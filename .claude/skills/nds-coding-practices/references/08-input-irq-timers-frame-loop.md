# 08 — Input, IRQs, timers, and frame loops

Use one normalized input owner. Sample hardware once per intended sampling interval and distribute a snapshot; do not let menu/gameplay consumers independently advance edge state. If multiple simulation ticks consume one presentation interval, deliver press/release edges at their defined tick rather than repeating `keysDown()` across every catch-up update.

VBlank is a synchronization event, not permission to put a frame's work in an interrupt. Prepare simulation, geometry, decompression, storage and uploads in ordinary context; commit bounded BG/OAM/palette changes at the safe video boundary. A dropped presentation must not implicitly drop authoritative game ticks. Use the project's clock contract rather than assuming exactly 60.000 Hz from a function name.

## Interrupt context

Keep handlers bounded and nonblocking: acknowledge/record hardware events and use supported nonblocking wake operations. No filesystem, allocation/free, printf/libc/TLS-dependent work, mutexes, waits, or long processing. Keep shared snapshots consistent with a short critical section or a proven handoff. Save and restore prior interrupt state; unconditionally enabling interrupts on exit breaks callers.

At the pinned Calico baseline, IRQs use a separate stack, do not nest, and are not ordinary thread/TLS context. Check another runtime before applying those details. `volatile` helps represent asynchronous accesses but does not make a multivariable snapshot atomic. [irq_handoff.c](../examples/irq_handoff.c) demonstrates a bounded snapshot; [irq_worker.c](../examples/irq_worker.c) demonstrates deliberate event coalescing, not a queue of every simulation tick.

## Scheduling

Calico lower numeric priority means higher priority. There is no automatic equal-priority time slicing; `threadYield()` yields only to equal-priority runnable threads. A busy main thread can starve a lower-priority worker. Block/sleep on the actual event, assign priorities deliberately, and make stop/join possible after disabling the producer.

Thread stacks need correct size, alignment, top pointer and any TLS reservation; see [17](17-libnds2-calico-facts.md). Main, worker, IRQ and exception stacks are different budgets. Avoid large per-frame automatic arrays. A one-slot notification may coalesce repeated events only when that is the intended contract.

Reserve timers and DMA through existing owners. Calico normally owns timers 2/3; do not take them because an old example uses them. Account wrap, prescalers, IRQ latency and instrumentation overhead in timing. Do not subtract unlike tick units or treat waiting time as CPU computation.
