# 04 — Cache, DMA, and publication

Determine the physical producer/consumer, CPU cacheability, transfer width, complete owned cache lines, capacity, and release event first. ARM9 cache lines are 32 bytes. Neither `volatile` nor a CPU/compiler barrier writes back dirty cache data. Cache maintenance does not make ITCM/DTCM DMA-visible. Do not invent uncached aliases under Calico's MPU map.

## Direction matters

| Transfer | Correct ownership sequence |
|---|---|
| ARM9 cached RAM → DMA/ARM7 | Finish writes; flush the owned source range using the runtime's cache/barrier contract; submit; retain immutable data until the external reader releases it. |
| DMA/ARM7 → ARM9 cached RAM | Use dedicated full cache lines. Before handoff, clean/invalidate as needed so dirty old lines cannot overwrite incoming data; invalidate after completion before CPU reads. Invalidate-only before handoff is valid only when discarded old contents and whole lines are exclusively owned. |
| ARM9 → video memory by CPU | Use legal aligned halfword/word stores and a safe video update period; do not use byte loops or assume generic `memcpy` selects a legal path. |

Rounding a maintenance range outward can damage adjacent objects if ownership is not cache-line aligned. DMA cannot fix this. Avoid speculative/shared access while an external writer owns the buffer. For overlapping ranges use a CPU routine with an explicit overlap contract, not DMA copy.

DMA counts and widths are API-specific. Validate channel ownership, transfer granularity/count limit, both address domains, pointer arithmetic and destination capacity. Busy-check then start is not an atomic allocator: reserve channels through the project owner. Starting asynchronous DMA does not permit source reuse. Poll/wait only where progress is possible; never block an IRQ waiting for another service. See [dma_cache.c](../examples/dma_cache.c) and [17](17-libnds2-calico-facts.md).

## Publishing a raw mailbox

Prefer existing PXI services for small values. A raw mailbox needs separate producer/consumer cache lines, generation, sequence, length, full/empty policy, notification/acknowledgement, reset handling, and referenced-buffer lifetime. [shared_mailbox.h](../examples/shared_mailbox.h) is a **layout**, not a complete concurrent implementation.

For an ARM9 producer whose publication marker is in cached memory:

1. Acquire a genuinely free slot using a coherent acknowledgement; fill and validate the payload.
2. Flush the payload's owned lines. Then **write the publication sequence**, flush its separate owned line, and complete the required write-buffer/barrier ordering.
3. Notify through the runtime. Keep the slot and referenced buffers unchanged until a matching-generation acknowledgement.

Never flush the marker and then modify it without another flush. A consumer waits for notification, invalidates the dedicated externally written range before reading it, validates generation/sequence/type/length/handles, consumes, then acknowledges with the reverse-direction protocol. Polling requires its own coherent marker reads and ordering; a `volatile` loop is insufficient. Sequence wrap and reset cannot be inferred from equality alone.

Calico's `Mailbox` is a same-CPU word queue; it is not a cache-coherent cross-CPU shared-memory protocol. IRQ callbacks may use supported nonblocking wake/try-send operations, not mutexes or filesystem calls.
