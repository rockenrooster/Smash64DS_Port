---
name: smash64ds-memory-vram-audit
description: Audit a measured Smash64DS memory, texture, VRAM, transfer, TCM, heap, stack, reserve, or ARM9/ARM7 sharing problem. Use when conversion/upload, residency, bank ownership, clears/copies, DMA waits, duplicate representations, or memory high-water is an exclusive blocker. Requires byte, lifetime, invalidation, bank, and tick evidence before proposing movement or offload. Do not use for speculative caching or DMA.
---

# Objective

Remove measured critical-path memory/transfer work while preventing stale data,
VRAM-bank conflict, cache/coherency bugs, audio interference, and reserve loss.

# 1. Build the ownership/lifetime table

For every relevant buffer, asset, texture, palette, command block, or shared
queue, record:

- producer and consumer;
- exact representation and byte size;
- storage class and alignment;
- creation, mutation, invalidation key, and destruction;
- changed bytes versus transferred/cleared bytes;
- update frequency and match phase;
- conversion/preparation/copy/upload/wait P50/P95;
- VRAM bank or ITCM/DTCM ownership by phase;
- cache flush/invalidate requirements;
- fallback and stale-generation behavior;
- heap, stack, BSS, reserve, and ROM impact;
- ARM7/audio/DMA channel users that can conflict.

Do not infer active use from allocation or initialization. Prove reads, writes,
binds, transfers, and final ownership.

# 2. Inspect artifacts and counters

Use the map, `nm`, `readelf`, stack-usage output, generated manifests, runtime
counters, and exact captures to inventory:

- ROM/rodata versus `.data`/`.bss`/heap/stack;
- ITCM/DTCM symbols and total occupancy;
- duplicate source, converted, staging, packet, and resident forms;
- per-frame clears/copies/conversion/upload traffic;
- texture/palette key cardinality and VRAM fragmentation;
- DMA channels, alignment, cache maintenance, and synchronization;
- first-use hitches, evictions, replacements, and stale-generation rejects;
- reserve high-water through one-minute lifecycle and Results.

# 3. Test one lifetime/representation change

Candidate classes:

1. Generate immutable data into ROM instead of preparing it in play.
2. Prepare/convert during scene setup or pre-GO rather than each frame.
3. Keep a finite exact resident set with full logical keys and explicit lifetime.
4. Transfer only exact changed ranges when the overwrite/dirty proof is cheaper
   than the current traffic.
5. Bind/repoint resident data instead of recreating/uploading it.
6. Compact hot runtime state or move cold forensic state out of profile 0.
7. Use double buffering or overlap only with a no-stale/no-lag generation proof.
8. A/B DMA only after accounting for setup, cache maintenance, wait, and channel
   ownership.
9. Consider ARM7 offload only after proving the work is independent of GX/source
   update order and defining audio, coherency, latency, queue, reserve, and
   failure contracts.

Do not use final-frame caching, guessed residency keys, hidden post-GO loads, or
unbounded caches. Do not assume DMA or ARM7 is faster.

# 4. M4-specific gate

For texture residency, enumerate full output-determining keys after original
battle setup and before GO. Arm a violation fence at GO. Supported gameplay must
report zero conversion, palette/decode work, allocation, decompression, file I/O,
GL create/upload/delete, eviction, replacement/refresh, and manifest fallback.
Normal resident binds are not violations.

Prove VRAM bank fit, main-RAM metadata/staging cost, project reserve, animated
content behavior, and exact texture/material/upload hashes. Treat animated water
as a separate representation problem when the corpus cannot fit in VRAM.

# 5. Decide

Use identical ROM/configuration and phase windows. Report both bytes and ticks.
KEEP only when the intended critical-path work falls, output remains exact,
reserve stays within the live requirement, and no hitch, lag, audio failure, or
stale generation appears.

# Output

Use `references/memory-evidence-table.md` and return KEEP / REWORK / REVERT with
one next action.
