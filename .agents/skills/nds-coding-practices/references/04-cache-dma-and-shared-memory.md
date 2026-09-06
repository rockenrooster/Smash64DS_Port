# Cache, DMA, and Shared-Memory Correctness

## Start with producer, consumer, and lifetime

Before using DMA or sharing a buffer, write down five facts in code or nearby
documentation:

1. who writes the bytes;
2. who reads the bytes;
3. whether the ARM9 address is cached;
4. when the last asynchronous reader is finished;
5. which mechanism publishes completion.

A pointer alone does not establish any of these facts.

## ARM9 data-cache model

Main RAM used by ARM9 is normally cached. DMA engines, ARM7, and other hardware
do not snoop dirty ARM9 cache lines. Therefore a value can be correct in the
ARM9 cache and stale in physical memory.

### ARM9 produces, DMA or another device consumes

The safe sequence is:

1. finish all CPU writes to the payload;
2. flush the exact source byte range with `DC_FlushRange`;
3. start the transfer or notify the consumer;
4. keep the payload alive and unchanged until the consumer is finished;
5. wait at the first point that actually needs completion.

Use `examples/dma_cache.c` for the checked outbound pattern. It treats zero
bytes as a no-op before any cache or hardware access, checks alignment/channel
and the ARM9 21-bit transfer count, and refuses to reprogram a busy channel.
The caller still owns capacity, DMA addressability, and exclusive channel use.
Its outbound destination is uncached/device memory; a cached main-RAM
destination additionally needs the inbound protocol below.

The libnds DMA wrappers take **bytes**, but the hardware counts halfwords or
words. Reject unit remainders rather than allowing silent truncation. On ARM9,
a zero count field means 0x200000 transfer units, not zero bytes. Check the
hardware count limit before narrowing or starting the channel. ARM7 has
different count limits; the ARM9 helper is not an ARM7 implementation.

**DMA cannot access ITCM or DTCM.** A correctly aligned, flushed, live stack
array can still be inaccessible. Use DMA-accessible staging memory or a CPU
copy. Verify placement using the actual linker/runtime, not a guessed address.

### DMA or another device produces, ARM9 consumes

Use a destination allocation whose cache-line ownership is dedicated to that
transfer. On the ARM9, cache invalidation operates on 32-byte lines. The
published libnds API requires the invalidated base and size to be 32-byte
aligned.

A robust protocol is:

1. ensure the destination cache lines do not contain unrelated dirty data;
2. invalidate the aligned destination range before the producer starts when
   stale cache lines could later be written back;
3. start the producer;
4. wait for completion;
5. invalidate the aligned range again;
6. only then read the payload on ARM9.

Prefer a 32-byte-aligned allocation whose full size is a cache-line multiple;
invalidate that owned range without rounding arbitrary pointers outward.
`examples/dma_cache.c` demonstrates this contract.

Invalidating before a transfer may discard dirty bytes. It is safe only when
the old contents are disposable and the producer overwrites every byte that
will later be used. For a partial-line update with bytes to preserve, clean the
owned lines before handoff, prevent CPU access during the transfer, and
invalidate after completion; or use a dedicated staging buffer. Never discard
an unrelated object's dirty bytes by rounding outward.

## DMA is not automatically faster

DMA has setup, bus, channel-ownership, cache-maintenance, and completion costs.
Use it for suitable copies/fills or hardware-timed streams. A small CPU copy can
be faster and simpler. During DMA, useful ARM9 execution is limited to cached
or TCM accesses: external-memory/I/O accesses and cache misses can stall.
Starting an asynchronous transfer does not promise useful CPU overlap; even
polling its MMIO status may stall on the shared bus.

Correct DMA use requires:

- a documented channel owner;
- source and destination alignment suitable for the selected unit;
- byte counts checked before narrowing and before unit truncation;
- no unsupported overlap;
- no source or destination reuse before completion;
- no TCM source/destination, and no source that expires before completion;
- no remapping of VRAM while a transfer targets it;
- a completion strategy that does not immediately destroy all overlap benefit.

### Channel ownership

Each CPU has four DMA channels. Do not select channel 3 merely because a helper
defaults to it if another subsystem, library, display capture path, audio path,
or card transfer owns that channel. Centralize reservations:

```c
enum DmaOwner {
    DMA_OWNER_NONE,
    DMA_OWNER_VIDEO_UPLOAD,
    DMA_OWNER_STREAMING,
    DMA_OWNER_CAPTURE,
};
```

A lightweight debug allocator is preferable to silent channel collision.

## CPU copy versus DMA decision

Prefer a CPU copy when:

- the payload is small;
- source or destination is awkwardly aligned;
- the CPU will immediately wait;
- cache maintenance dominates;
- the copy can be fused with conversion, clipping, or validation;
- DMA channels are scarce or already reserved.

A CPU copy whose destination is VRAM, palette RAM, or OAM must use halfword or
word accesses: the hardware ignores 8-bit writes, and a `memcpy` tail or
byte-path `memset` silently drops data there.

Prefer DMA when:

- the transfer is large and contiguous;
- bytes are already in the final format;
- alignment and lifetime are controlled;
- useful CPU work overlaps the transfer;
- the timing mode is tied to display, card, FIFO, or another hardware event;
- repeated fills/copies amortize setup.

Measure the whole owning operation, not just the copy body.

## ARM9 and ARM7 shared data

Use the current runtime's message-passing/service API when it fits. A raw shared
mailbox is a protocol, not a struct both CPUs casually edit.

### Single-producer/single-consumer publication

A safe one-way mailbox usually contains:

- fixed-size payload or indices into owned buffers;
- a sequence/generation number;
- a state or count written last by the producer;
- no pointers unless both CPUs can address the memory and lifetime is proven.

ARM9 producer sequence:

1. wait until the slot is free;
2. write payload;
3. flush payload and publication fields;
4. send a PXI/FIFO notification or update the protocol sequence;
5. do not mutate the slot until acknowledged.

ARM9 consumer sequence for ARM7-produced main-RAM data:

1. receive a completion notification;
2. invalidate the dedicated aligned range;
3. validate sequence, type, and bounds;
4. consume;
5. acknowledge/release according to the protocol.

On ARM7, use the synchronization primitives and memory barriers provided by the
current runtime. Do not invent compiler-only barriers and assume they solve bus
visibility.

## Ring queues

For a bounded SPSC queue:

- one CPU owns `write_index`;
- the other owns `read_index`;
- payload is committed before the producer publishes the new write index;
- consumer validates index range and message type;
- overflow policy is explicit: reject, overwrite-oldest, or backpressure;
- index wrap is deliberate;
- reset increments a generation so stale notifications are rejected.

Never let both CPUs update a shared `count` without synchronization. Separate
producer and consumer indexes are easier to reason about.

## `volatile` and barriers

`volatile` can force the compiler to perform an access, which is useful for
MMIO and carefully designed flags. It does not:

- write dirty ARM9 cache lines back to RAM;
- invalidate stale cache lines;
- order two processors by itself;
- protect multiword payloads;
- make `counter++` atomic;
- keep an asynchronous buffer alive.

Use the runtime's synchronization API and explicit cache maintenance.

## Common incorrect patterns

### Asynchronous DMA from a local array

```c
void wrong(void)
{
    uint32_t temporary[64];
    build_data(temporary);
    DC_FlushRange(temporary, sizeof temporary);
    dmaCopyWordsAsynch(0, temporary, destination, sizeof temporary);
} // temporary lifetime ends while DMA may still be reading it
```

First check whether the array is in DTCM: waiting does not make it DMA-readable.
Use DMA-accessible persistent staging, or a CPU copy. The reviewed Calico
ARM9 main stack is in DTCM by default; `glCallList` uses DMA too. A larger
`__stacksize__` can change main-stack placement, but is not the default repair
for one transfer (see `17-libnds2-calico-facts.md`). Only for an accessible
buffer is waiting before return sufficient to fix the lifetime part.

### Invalidating an arbitrary unaligned object

```c
DC_InvalidateRange(object, object_size); // API alignment contract violated
```

Fix allocation and ownership first. Do not silently round outward across other
objects.

### Flushing the pointer, not the payload

```c
DC_FlushRange(&buffer_pointer, sizeof buffer_pointer); // wrong bytes
```

Flush `buffer_pointer` over the actual payload size.

### Immediate wait after every asynchronous start

This is correct but often pointless. Start a transfer only when the system can
do independent work or when an asynchronous interface simplifies scheduling.

## Review checklist

- [ ] Producer, consumer, lifetime, and completion are explicit.
- [ ] Cached ARM9 source is flushed before external consumption.
- [ ] ARM9 destination invalidation respects 32-byte cache-line ownership.
- [ ] Zero, count limit, unit, alignment, channel, and DMA accessibility are checked.
- [ ] Async buffers cannot be freed, remapped, or overwritten early.
- [ ] Shared messages use generations and bounded payloads.
- [ ] `volatile` is not being used as a coherency protocol.
