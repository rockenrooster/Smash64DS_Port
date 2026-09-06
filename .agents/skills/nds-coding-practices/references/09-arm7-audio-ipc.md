# ARM7, Audio, IPC, and Runtime Services

## Prefer the current service architecture

Modern devkitPro libnds uses Calico as its low-level foundation. Start with the
current default ARM7/runtime services and the project's supported audio API.
Write a custom ARM7 program only when a concrete requirement cannot be met by
the stock service path.

A custom ARM7 is a second embedded program with its own startup, memory, IRQ,
service, and compatibility obligations—not a convenient place to dump slow
ARM9 work.

## Concrete Calico IPC starting point

For the reviewed libnds 2.x stack, the old custom `fifoSendValue32` API is gone.
Use Calico PXI: reserve `PxiChannel_User0..7`, register an ARM7 mailbox with
`mailboxPrepare` + `pxiSetMailbox`, block the worker on `mailboxRecv`, and respond
with `pxiReply`. ARM9 can wait for registration with `pxiWaitRemote`, then use
`pxiSendAndReceive` for a request/reply. These waits have no timeout parameter;
keep one protocol owner and do not put an unbounded wait in a hard frame budget.
`pxiSend` avoids the reply wait, but transmission can still block.

Simple immediates are **26-bit**. Extended messages use a 16-bit immediate plus
1..32 data words; do not send arbitrary pointers or 32-bit values as though the
simple immediate preserved all bits. A native `Mailbox` is a same-CPU queue;
PXI forwards received words to it. The default mailbox adapter discards a failed
`mailboxTrySend`, so overflowing it can silently lose a request. Use bounded
in-flight messages/credits and enough slots for the actual framing.

Use `../examples/pxi/README.md` for the small matching ARM9/ARM7 echo/stop pair.
It preserves existing ARM7 startup/services, uses no shared-pointer payload,
and permits one outstanding request. It is not a reason to write a custom ARM7
when the stock services already meet the task. API/source lookup:
`17-libnds2-calico-facts.md`.

## ARM7 ownership

The ARM7 typically owns low-level sound hardware and runtime services. Exact
ownership depends on the current library generation and project configuration.
Confirm before changing:

- audio mixer/channel service;
- touch and input auxiliary services;
- power/lid behavior;
- RTC/time services;
- Wi-Fi/network services;
- PXI/message channels;
- storage/card coordination;
- timers and DMA channels on ARM7.

Do not initialize a second competing service for hardware already owned by the
runtime.

## Message design

Prefer small, typed messages and owned buffers.

A message should include as needed:

- type/version;
- payload length;
- sequence/generation;
- handle/index rather than raw pointer;
- explicit units;
- flags with reserved bits validated;
- completion/error status.

Reject unknown types, bad lengths, stale generations, and out-of-range handles.
Do not trust the other CPU merely because it is in the same device.

## Pointer rules

A raw ARM9 pointer is safe for ARM7 only if all of these are true:

- the address range is visible to ARM7;
- the memory mapping will remain stable;
- the payload lifetime exceeds ARM7's last read/write;
- ARM9 cache data is published correctly;
- ARM9 invalidates before reading ARM7-produced data;
- both sides agree on alignment, layout, and size;
- no owner mutates the data concurrently.

Handles into a fixed shared pool are easier to validate than arbitrary pointers.

## Queue rules

Use bounded queues with explicit overflow behavior. Never block both CPUs while
each waits for the other.

Good design:

- ARM9 submits a command if a slot is available;
- ARM7 consumes independently;
- completion is a separate bounded queue or sequence field;
- resets increment a generation and drain/ignore stale messages;
- queue pressure is observable in debug builds;
- optional commands can be dropped deliberately, critical commands use
  backpressure at a safe point.

Bad design:

- ARM9 spins with interrupts disabled waiting for ARM7;
- ARM7 waits for a buffer ARM9 will only release after receiving ARM7's reply;
- both sides update one shared count;
- scene teardown frees buffers before acknowledgements arrive.

## Audio asset practices

Prepare audio offline for the chosen runtime:

- supported PCM/ADPCM/stream format;
- sample rate and channel count;
- loop points aligned to codec/block requirements;
- normalized/clipped deliberately;
- byte count and duration recorded;
- streaming chunks sized for latency and storage behavior.

Do not decode general-purpose formats in a frame-critical path unless the
project intentionally includes a bounded decoder and budget.

## Sample lifetime

For one-shot or streaming playback, define who owns sample bytes until the
mixer/hardware is done. Do not play from:

- a stack buffer;
- a scene arena about to reset;
- a streaming buffer that the producer immediately refills;
- an ARM9 cached buffer that was never published;
- an address remapped or reused before completion.

Use channel/voice handles with generations so a late completion cannot stop or
modify a newly reused channel.

## Mixing and channel ownership

Centralize:

- channel/voice allocation;
- priority and stealing policy;
- master/group volume;
- pan and pitch units;
- looping and stop/fade semantics;
- music versus SFX streaming buffers;
- scene teardown.

A “stop channel N” command is unsafe if channel N may have been reassigned.
Send `(channel, generation)` or use an opaque handle.

## Streaming audio

A robust double/ring-buffer pipeline separates:

1. storage read;
2. decode/conversion if any;
3. ARM9 cache publication if ARM7 will read main RAM;
4. ready notification;
5. ARM7 consumption;
6. completion/recycle notification.

Maintain enough buffered duration for storage jitter but not so much that input
latency, memory, and seek behavior suffer. Underflow policy—silence, repeat,
fade, or stop—must be explicit.

Never perform filesystem access in an audio IRQ.

## Latency and batching

Do not send one inter-CPU message for every tiny mixer parameter when a compact
state snapshot or batch can be published once per logical frame. Conversely,
do not batch critical sound starts so long that latency becomes audible.

Measure queue depth, command latency, and underflows separately from ARM9 frame
performance.

## Custom ARM7 checklist

For a custom ARM7, verify the following against the existing core; a new
standalone architecture document is not required:

- exact reason stock services are insufficient;
- startup/handshake ordering;
- memory map and shared regions;
- IRQ/timer/DMA ownership;
- sound service and mixer responsibilities;
- touch/power/RTC/Wi-Fi compatibility as applicable;
- panic/error reporting path;
- reset/shutdown protocol;
- matching ARM9 library ABI/version;
- independent ARM7 stack and CPU budget.

Treat old ARM7 templates as historical references, not drop-in code for current
libnds/Calico.

## Common failures

### Sounds randomly play with old parameters

A queue slot or channel handle was reused without a generation, or payload was
published after the notification.

### Audio crackles only on hardware

Check ARM9 cache publication, storage jitter, buffer lifetime, queue starvation,
ARM7 CPU load, and DMA/timer conflicts.

### Scene exit crashes later

Outstanding audio/IPC users still reference scene-owned memory. Stop, drain, or
transfer ownership before resetting the arena.

### Both CPUs freeze

Look for circular waits, interrupts disabled during waits, full queues with no
consumer progress, or a startup handshake where both sides wait first.

## Review checklist

- [ ] Stock/current services are used unless a custom ARM7 is justified.
- [ ] Message types, lengths, units, and generations are validated.
- [ ] Shared pointers have visibility, cache, mapping, and lifetime proof.
- [ ] Queues are bounded and cannot deadlock both CPUs.
- [ ] Audio buffers remain alive until completion.
- [ ] Channel handles prevent stale operations after reuse.
- [ ] Filesystem work never occurs in an audio IRQ.
