# 09 — ARM7, audio, and IPC

Keep the SDK's established ARM7 services alive. Offload only a bounded job whose CPU benefit exceeds communication, cache, contention, and retained-buffer costs. ARM7 lacks ARM9 instructions/caches and is not an RSP; moving a blocking read or a floating-point loop there is not automatically a win.

## Protocol design

Define channel owner, message schema, payload widths, generation/request ID, capacity, overflow/backpressure, cancellation/reset, completion and buffer lifetime. Use stable handles plus explicit ranges for shared data, not unchecked source pointers. A timeout alone does not make an old writer safe: quarantine or acknowledge cancellation before reusing memory.

At the pinned Calico baseline, ordinary PXI payloads are 26 bits; extended messages use a 16-bit immediate and 1–32 payload words. Use application user channels, not service channels. PXI callbacks run in IRQ context; they may enqueue supported nonblocking work but cannot block, allocate or call libc. A mailbox adapter can drop data when full unless the protocol prevents or handles it. Wait-ready/receive and send paths can block; do not describe a connected peer as guaranteed or a send as inherently asynchronous.

Calico `Mailbox` is a same-CPU bounded word queue (1–255 slots). For cross-CPU data use PXI plus the explicit cache/lifetime contract in [04](04-cache-dma-and-shared-memory.md). The [PXI example](../examples/pxi/README.md) is a composed one-request echo/stop demo, not a general request router or timeout service.

## Audio

Preserve channel ownership, sample representation, alignment, frequency units, loop offsets/units, voice priority, envelope/pan behavior, and stop acknowledgement. Allocate/update samples only while CPU owns them; flush ARM9-produced samples before external playback and retain them until the service/hardware releases them. Do not free a sample immediately after sending stop without establishing completion.

Streaming needs a demand rate, worst service latency, bounded buffers, valid byte counts, end-of-stream handling and explicit underrun policy. Keep filesystem/decode outside audio IRQs. Ring-buffer generations prevent stale completion only when producers and consumers check them. Test loop boundaries, simultaneous effects, channel exhaustion, cancel/restart and scene transitions.

An N64 ADPCM stream is not interchangeable with DS ADPCM merely because both names include ADPCM. Decode/re-encode with the correct predictors, loop state and target format; source semantics belong to the N64 companion. Muting required sound or dropping cues is not a performance fix.
