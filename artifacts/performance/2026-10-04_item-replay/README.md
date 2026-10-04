# Item draw replay (Beam Swords), 2026-10-04

## Why

247 of the gate's 280 over-1.12M frames fall after presented frame 800, and
the only items alive then are Beam Swords (a gdb walk of
`gGCCommonLinks[4]` every 50 frames): one lies from ~780, Link holds it
from ~970, a second lies from ~1440. MITM read ~33K ticks a frame for a lying
sword and ~62K for a held one -- two roots (an 11-triangle blade, a
2-triangle hilt). A lab split of the item draw (`gNdsLabItemAcc`, lab ROM,
frames 1440-1900, one held and one lying sword) put a frame's item draw at
95K: the held item's attach 21.6K once a frame (the exact source latch walk,
kept), and per root 15.6K -- world 3.0K and other matrix prep 0.8K,
`ndsNativeItemWave1Emit` 5.7K (DMA wait and batch close 1.3K, owner-memo
texture bind 1.0K, matrix load 0.5K, batch open 0.9K, corners 1.9K),
traversal init and generated setup 2.9K, fast-lane prep and tail 2.0K --
plus 2.3K a draw for the stage traversal's begin and end. On the gate rows,
halving MITM priced at P95 -30K and over-1.12M 280 -> 224.

## Change

`ndsRendererAdapterSubmitItemDObjTreeReplay` (src/port/renderer_adapter_stage.c)
takes the item draw: the same tree walk the submit does, a verbatim key (the
item's root and kind, the initial geometry mode, the traversal's light, and
per list its DObj, list, head, route and the head's captured colours and
blend modes), and two cached draws. A matching draw replays: per list the
fast lane's matrix preparation, unchanged (the held item's latch walk
included -- its FTParts writes are gameplay), then the recorded emits
(`ndsNativeItemReplayEmits`, src/nds/nds_native_item_wave1_emit.exec.inc):
the DMA wait and batch close, the owner-memo hit tail of the bind, the split
matrix load, the batch open with the recorded polygon format and alpha-test
words, and the corners from the recorded per-vertex words. Otherwise the
draw runs its owners inside the stage traversal while
`ndsNativeItemWave1Emit` records into a sink; any emit it cannot reproduce
(a shared matrix generation, fog, a bind outside the texture cache, a full
pool) fails the recording. A recorded texture must still be resident (slot,
name, key generation), else the owners run and record again. The skipped
owners' persistent stats are dead: every reader begins a traversal first.
Only the Sword's route replays for now. Same-ROM A/B word `gNdsItemReplay`.
BSS +2.0 KB.

## Equivalence

Lab verify mode (`gNdsItemReplayVerify=1`, lab ROM `build-lab-items`, frames
1000-1900): every draw that would have replayed instead ran its owners into
a scratch recording and compared it with the cached one (every emit field
and per-vertex word): 1,548 comparisons, 0 mismatches. Replay digest
IDENTICAL against `2026-10-04_itcm-pack4/gate-pack2` over 1,960 samples.

## Result (official gate, `build-gate-1004p`)

| | WORK P50 | WORK P95 | P99 | > 1.12M | two-VBlank |
|---|---:|---:|---:|---:|---:|
| `itcm-pack4/gate-pack2` (HEAD) | 920,704 | 1,259,712 | 1,583,104 | 280 | 1,678 / 1,961 |
| item replay | **906,432** | **1,247,232** | 1,573,952 | **241** | **1,714 / 1,961** |

By window (MITM mean / window P95): frames 800-1400 49.6K -> 36.5K,
1,298,496 -> 1,287,040; frames 1400-2000 85.8K -> 60.5K, 1,331,968 ->
1,295,808. Lab, frames 1440-1900: item draw 95.4K -> 59.8K a frame (attach
22.1K of it). Replays 1,549, records 4, record failures 0, native failures 0.
