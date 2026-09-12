# 11 — Filesystems and streaming

Select the actual storage backend: NitroFS for ROM-packaged read-only assets; the installed FAT/DLDI stack for mutable files; linked data for small resident assets. Calico-backed libnds 2.x uses libdvm compatibility interfaces, not necessarily the libfat implementation suggested by an include name. Keep NitroFS/FAT roots explicit and paths bounded.

No filesystem calls in IRQs: they can allocate, lock, wait and trigger block I/O. Prepare/decode during loading or a deliberately budgeted worker stage. Moving the call to ARM7 does not remove its latency. Pack related entries for sequential access rather than thousands of tiny scattered reads. Benchmark the supported device or the project's accepted model, not host storage.

Validate opens, reads, seeks, writes and close results; handle short completion, missing files, corrupt versions, oversized allocation, integer products, and error cleanup. This exact-read helper requires a valid stream and destination of the requested capacity; false can leave a partially written destination. Load transactionally before installing new live state.

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

File formats need magic/version, explicit endian/field widths, bounded counts/offsets, aligned payload rules and integrity checks where needed. Do not read native C structs or host pointers as portable files.

A bounded stream uses `FREE -> READING -> READY -> CONSUMING -> FREE`, with generation, valid length, file offset, owner and EOF/error. Reuse only after the matching consumer releases it. Size chunks/buffers against worst service latency, consumption rate, decoder granularity, cache/DMA alignment and RAM reserve. Measure storage, decode/fixup/publication and upload separately. A byte-capped slice is not a time-bounded frame: measure one real step, retry count, total completion span and tail before implementing a large resumable loader. Check for an existing compact consumer-specific representation before slicing full source closures.

For saves, preserve the previous valid copy until a supported commit succeeds. Use a versioned dual-slot/journal or temp-write/flush/close/replace strategy supported by the real filesystem. Handle partial writes, full/removed media and reset. Desktop atomic rename assumptions and `fclose()` are not universal power-loss guarantees. Rate-limit logs and saves.
