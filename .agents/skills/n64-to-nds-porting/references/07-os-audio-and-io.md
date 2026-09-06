# 07 — Replace source services, not the entire N64 operating system

## Resolve the actual purpose of each source service

| N64 dependency | Native port boundary |
|---|---|
| `OSTask`, SP/DP task completion | Existing target render/audio work owners and completion points |
| VI messages/swap | Canonical simulation clock plus target presentation owner |
| PI reads/DMA | Asset IDs and bounded load/stream layer on the actual DS storage path |
| Message queues/threads | Preserve required ordering with existing target queues/workers or simple state machines |
| N64 cache/address macros | Remove at the source boundary; use target cache/ownership rules where needed |
| Controller/accessories | Normalized input and explicitly supported accessory replacements |
| EEPROM/SRAM/Flash/Pak operations | Defined save format, persistence boundary, and failure handling |

Do not map `osWritebackDCache()` mechanically to a target flush everywhere it
appears. The source and target buffers, producers, and consumers may be entirely
different after the port. Determine the target ownership and use the companion's
cache/DMA contracts.

Nor is ARM7 a replacement RSP. Use existing DS services and move only work whose
ownership, compute cost, communication, and memory lifetime make sense. Maintaining
a second source compatibility runtime across CPUs can cost more than the work
removed. Keep actual ARM7/PXI/thread/IRQ details in `nds-coding-practices`.

## Threads and queues: preserve ordering, remove unnecessary machinery

A render/audio job graph originally designed around source coprocessors can often
become a simpler target owner-driven sequence. But do not delete a queue merely
because the port is “single threaded”: it may encode required event order, deferred
completion, or lifetime. A state machine or bounded native queue can retain that
meaning without recreating the entire libultra interface.

Blocking/wakeup behavior and callback context must follow the installed target
runtime, not old source assumptions. Never wait in an IRQ for work that requires
that IRQ or a blocked lower-priority owner to make progress.

## Audio is another compiler boundary

Preserve musical/event behavior: instrument selection, tempo, note timing,
envelopes, pitch, pan, voice priority/stealing, loop boundaries, stop/cancel, and
source lifetime. Source audio microcode and command buffers are implementation
machinery, not a requirement to run the same mixer on ARM9.

Decode N64 compressed samples with their actual predictor/codebook and loop
metadata on the host, then emit a target-supported representation. N64 ADPCM
includes predictor state needed for correct looping; the word “ADPCM” does not
make a source byte stream interchangeable with another codec. [Source ADPCM
format][adpcm] The selected DS sound interface exposes target-supported formats;
match its encoder and loop/alignment contract rather than relabeling source data.
[Pinned sound API][sound]

Retain loop units explicitly: source samples, decoded samples, encoded bytes,
blocks, or target API units. Re-encoding can move legal loop boundaries; verify
seamless playback and pitch instead of converting offsets with a guessed ratio.
Different source sounds can share immutable samples but need distinct envelope,
loop, or channel instances.

Pre-rendering music can remove sequence/mixer work when music is fixed, but is
not automatically suitable for dynamic layers, tempo, instruments, or interactive
transitions. Streaming decoded music trades CPU for bandwidth and buffers. Use
the project's fidelity/resource contract to choose; do not mute music or drop
required cues under the label of a native port.

## Saves and source binary layouts

Keep a source-compatible export/import path only when needed. A target-native
save can use explicit version, endian, field widths, length checks, and integrity
checks rather than serializing native C structs or old pointers. Preserve the
semantics of progress, unlocks, and settings. Use a failure-aware commit strategy
supported by the actual filesystem; do not assume desktop atomic-rename behavior
or that a failed write left the previous data intact.

Avoid porting source controller-pak filesystem machinery unless required content
needs it. Translate the game's save payload and operations at the narrowest useful
boundary. Distinguish unsupported accessories from corrupted saved data.

## Scene teardown includes external consumers

Source reset/reload code may rely on task completion that no longer exists after
porting. Define the target completion points explicitly for GX submission,
textures, audio voices, transfers, workers, and filesystem requests. Do not free
an arena because source simulation stopped referencing it while a target consumer
still does. Invalidate generations and handle stale completions locally.

[adpcm]: https://ultra64.ca/files/documentation/online-manuals/man/pro-man/pro19/index19.2.html
[sound]: https://github.com/devkitPro/libnds/blob/84e6082ce27c87ed218fb369a9944644aa2243a6/include/nds/arm9/sound.h
