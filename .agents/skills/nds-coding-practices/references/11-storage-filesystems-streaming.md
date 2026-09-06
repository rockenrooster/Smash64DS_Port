# Storage, Filesystems, and Streaming

## Choose the correct storage model

Common DS project choices include:

- **NitroFS** for read-only assets packaged with the ROM;
- **FAT through the selected filesystem/DLDI stack** for mutable files;
- ROM-embedded/generated C data for small always-resident assets;
- project-specific card/network storage where applicable.

Calico-based devkitPro libnds 2.x uses **libdvm**, which replaces libfat and
libfilesystem while retaining compatibility interfaces. Legacy projects may
still correctly use libfat; do not infer the implementation from a header name.

Use the installed library's current initialization and API. Storage behavior
varies by device, DLDI driver, emulator, and access pattern.

## Never perform filesystem work in an IRQ

Filesystem operations can allocate, lock, wait, issue card I/O, and take
unbounded time. They do not belong in VBlank, audio, timer, or other interrupt
handlers.

An IRQ may set a request flag or enqueue a small command. Main/thread context
performs the I/O.

## Validate all I/O

For every open/read/seek/write:

- check the return value;
- handle partial reads/writes;
- verify file size before allocation/narrowing;
- reject unexpected format versions and counts;
- guard integer overflow in `count * element_size`;
- define missing/corrupt asset behavior;
- close or transfer ownership on every error path.

```c
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

static bool read_exact(FILE *file, void *destination, size_t byte_count)
{
    uint8_t *out = (uint8_t *)destination;
    while (byte_count != 0) {
        const size_t n = fread(out, 1, byte_count, file);
        if (n == 0) {
            return false;
        }
        out += n;
        byte_count -= n;
    }
    return true;
}
```

This helper still requires caller limits and an error policy.

## Binary formats

Design DS-facing formats for bounded parsing:

- fixed magic and version;
- little-endian fields decoded deliberately;
- explicit byte sizes/offsets;
- count limits before allocation;
- alignment independent of C struct padding;
- checksum/hash when corruption matters;
- table-of-contents bounds validated against file size;
- no raw host pointers or ABI-dependent enums.

Treat packed structs as format descriptions, not permission for unaligned typed
loads.

## Keep I/O off the visible critical path

Avoid first-use stalls by:

- preloading required scene assets;
- reading larger contiguous chunks;
- maintaining a bounded read-ahead buffer;
- using build-time packing to reduce seeks and tiny files;
- decompressing during loading or incrementally under a budget;
- caching metadata and frequently used tiny assets;
- separating storage, decode, and upload stages.

Do not assume an emulator's near-instant filesystem represents flashcart
hardware.

## Streaming pipeline

A robust stream uses explicit buffer states:

```text
FREE -> READING -> READY -> CONSUMING -> FREE
```

Each slot needs:

- generation;
- valid byte count;
- absolute stream/file offset;
- owner/state;
- alignment and cache rule;
- error/end-of-stream marker.

The producer may not refill a slot until the consumer releases the matching
generation. The consumer must not read beyond `valid byte count` at end of file.

## Buffer sizing

Choose chunk size based on:

- worst-case storage latency and throughput;
- decoder granularity;
- audio/video consumption rate;
- RAM budget;
- cache/DMA alignment;
- seek behavior;
- acceptable startup and recovery latency.

A larger buffer can reduce underflows but increase memory and response latency.
Measure on the slowest supported target class.

## Writes and save data

For mutable data:

- use a versioned format;
- write to a temporary file then flush/close and replace where supported;
- include validity marker/checksum;
- keep previous known-good data until the new write is committed;
- handle full media, removal, permission, and partial-write failure;
- avoid writing every frame;
- rate-limit logs and telemetry;
- never assume `fclose` guarantees power-loss durability on all media.

A journal or dual-slot scheme is often safer than in-place mutation.

## Paths and naming

- Keep path construction bounded.
- Do not pass untrusted strings as `printf` format strings.
- Normalize project asset naming at build time.
- Avoid case assumptions that differ between host tools and target filesystem.
- Keep NitroFS and FAT roots explicit in one filesystem layer.

## Asset packaging

Thousands of tiny files can magnify seek and metadata overhead. Consider a
versioned pack file with:

- sorted/indexed entries;
- aligned payloads;
- validated offsets and lengths;
- optional per-entry compression;
- host-side manifest and extraction tool;
- deterministic build output.

Do not build an elaborate virtual filesystem when a few sequential files are
sufficient.

## Common failures

### Smooth in emulator, stutters on flashcart

I/O is occurring during visible frames, reads are tiny/random, or buffering was
sized for emulator latency.

### Random crash loading a corrupt file

Counts or offsets were trusted before checking file size and multiplication
overflow.

### Stream repeats old data

Buffer generation/valid length was not updated atomically, or the consumer read
a recycled slot after a late notification.

### Save occasionally becomes empty

In-place writes or truncate-before-success destroyed the previous copy. Use a
transactional/dual-slot scheme.

## Review checklist

- [ ] No filesystem call occurs in an IRQ.
- [ ] All reads/writes handle partial completion and errors.
- [ ] Counts, offsets, and products are bounds/overflow checked.
- [ ] Visible gameplay does not perform unbudgeted first-use I/O.
- [ ] Streaming slots have explicit state, generation, and valid length.
- [ ] Save writes preserve a previous valid copy until commit.
- [ ] Hardware/flashcart latency is not inferred from emulator behavior.
