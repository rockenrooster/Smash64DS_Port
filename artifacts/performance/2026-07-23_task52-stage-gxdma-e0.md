# Task 52 — Dream Land stage GX DMA replay: E0 STOP (replay path is inactive)

**Date:** 2026-07-23
**Branch:** `codex/task52-stage-gxdma-replay` (E0 only)
**Parent:** `a4ca08e` (master, verified)
**Outcome:** **STOP at E0.** Task 36 replay is **structurally disabled** in the
shipping profile-0 ROM. The FIFO-replay loop this task was chartered to
DMA-replace does not run. No DMA implementation was admitted.

## The E0 gate (spec, "STOP at E0 and report if the active stage does not
actually use Task 36 replay in the canonical profile-0 path. Do not implement
DMA against an inactive or fallback-only path.")

The spec's first proof is: *confirm profile-0 steady-state Dream Land executes
through `ndsRendererTask36ReplayRun()`, not the compose or generic fallback
path.* It does not.

### How it was measured

The `gNdsRendererTask36Replay*` counter globals are compile-gated to
`NDS_RENDERER_PROFILE_LEVEL==1` (src/nds/nds_renderer.c:2076, inside the
profile-1 block at :2041), so the profile-0 tick-HUD ROM exports no counter
globals. But the internal owner struct `sNdsRendererTask36ReplayOwner`
(gated only on `NDS_TASK36_HW_COMPOSE==2`, which the tick-HUD and published
targets both set) is always compiled in, and GDB resolves its fields by name
from the ELF debug info. That struct is the ground truth for "is replay active."

Probed `sNdsRendererTask36ReplayOwner` at
`ndsBattlePlayableFrameCompleteMarker`, frames 438–445, on the **profile-0
tick-HUD ROM** (the shipping program), and separately on the **published
`smash64ds-battle-playable-hwtri` ROM** (`1818AA77…`):

```
# profile-0 tick-HUD ROM, frames 438-440 (REPLAY_PROBE + ARENA_GATE)
REPLAY_PROBE: state=3(DISABLED) word_count=0 frame_capture=0 frame_replay=0
              capture_fault=0 captured_segment_mask=0 current_run=UINT_MAX
ARENA_GATE:   arena_chosen_size=0x14C000  arena_alloc_fail_count=4
              rigid_binding_mask=0x00000381c00fffff  (== RIGID_BINDING_MASK)
              state=3(DISABLED)

# published hwtri ROM 1818AA77, frames 438-440
PUB_ARENA:    arena_chosen_size=0x14E000  arena_alloc_fail_count=2
              state=3(DISABLED)
```

### Root cause: the arena-size admission guard rejects replay

`ndsRendererTask36ReplayBeginFrame` (src/nds/nds_renderer.c:4164) gates replay
admission on:

```c
if ((gNdsTaskmanArenaChosenSize != 0x150000u) ||
    (gNdsTaskmanArenaAllocFailCount != 0u))
{   /* ... set DISABLED, return; */ }
```
(nds_renderer.c:4195-4208)

The rigid-binding mask **matches** (`0x00000381c00fffff ==
NDS_RENDERER_TASK36_RIGID_BINDING_MASK`, include/nds/nds_renderer.h:100), so
that gate passes. But `gNdsTaskmanArenaChosenSize` is **not** `0x150000` in
either shipping ROM:

- tick-HUD ROM: `0x14C000` (short by 16 KiB), 4 alloc fails
- published ROM: `0x14E000` (short by 8 KiB), 2 alloc fails

`gNdsTaskmanArenaChosenSize` is set by a **downward-stepping allocator**
(src/port/diagnostics.c:7368-7381) that starts at `NDS_TASKMAN_ARENA_SIZE`
and decreases by 4 KiB (`0x1000`) per failed `calloc` until one succeeds,
deliberately — the comment at diagnostics.c:7364-7367 says a coarse
`0x150000 → 0x140000` jump discarded up to 60 KiB and could erase the verified
128 KiB post-BGM reserve. On the DS heap the full `0x150000` allocation does
not fit, so the allocator steps down to whatever fits, and the replay admission
guard (which demands *exactly* `0x150000`) disables replay.

**Net:** Task 36 replay has been dead code in the shipping profile-0 ROM since
the arena allocator was made robust. With `state==DISABLED`,
`ndsRendererTask36ReplayBeginFrame` returns before setting `frame_replay`
(nds_renderer.c:4193/4207), so at the segment call site
(nds_renderer.c:21220-21222) `task36_replay_segment` is FALSE and
`ndsRendererTask36ReplayRun` is **never called**. The 8 rigid layer0 bindings
draw through the **generic per-word emit loop** (nds_renderer.c:21241-21375:
`ndsRendererNativeStageBeginRun(..., FALSE)` + per-triangle
`ndsRendererNativeStageEmitNoZTriangle`), not any replay FIFO.

## E0 fresh mode-0 baseline (captured before the path proof, for the record)

128 presented-frame samples, frame 438, profile-0 tick-HUD target, ROM
`9B0A295D…`, melonDS `DE80E46B…` (repo fork, models icache/dcache), git
`a4ca08e`. Same ROM/window for any future A/B; no historical window mixed in.

| bucket | P50 | P95 | min | max |
|---|---|---|---|---|
| ALL | 1,680,256 | 2,241,024 | 1,679,552 | 3,361,152 |
| FTR | 576,384 | 1,013,760 | 571,392 | 1,022,080 |
| **STG** | **569,280** | **575,744** | 563,904 | 584,512 |
| SRC | 317,248 | 950,016 | 161,088 | 1,286,784 |
| OTHR | 163,712 | 432,768 | 20,352 | 561,472 |
| MISC | 48,448 | 158,528 | 47,232 | 196,544 |
| HUD | 960 | 316,160 | 896 | 380,800 |
| BG | 4,160 | 4,224 | 3,968 | 4,288 |
| AUD | 2,368 | 64,960 | 1,152 | 66,752 |

VBlank-interval histogram over 565 presented frames: **2→0, 3→426, 4→122,
5+→17, max 18**; cadence slips 0. Never hit 30 FPS (mean 3.16 VBlanks).

STG P50 569,280 reconciles with the original Task 51 controlled A-side
(569,216 / 574,208) — same window, not the 610K HUD capture. STG is 33.9% of
ALL P50.

## E0 cost split — NOT PERFORMED (path inactive)

The spec's E0 cost split (CPU issue vs GX backpressure) instruments the
FIFO-replay loop. That loop does not execute. There is no
`ndsRendererTask36ReplayRun` invocation to instrument, no `owner->words`
replay, no per-word `GFX_FIFO` store loop in the steady state. The STG cost
that exists is the **generic emit path** (per-triangle vertex/matrix emit),
which is a different mechanism than the one this task was chartered to
optimize, and is not a candidate for FIFO-DMA replay transport.

## Written proceed/stop verdict — STOP

Per the spec's STOP conditions ("STOP at E0 if: the CPU FIFO loop is a minor
owner; most elapsed time is unavoidable GX backpressure; no useful safe
overlap interval exists") — all three hold trivially, because **the CPU FIFO
loop is a zero owner**: it never runs.

- Removable CPU transport cost (the FIFO replay loop): **0 ticks** (path
  inactive). Proceed condition 1 (≥150,000 ticks removable at P50/P95) is not
  remotely met.
- No DMA implementation is admitted. No DMA channel selected for use. No
  `NDS_TASK52_STAGE_GXDMA_MODE` runtime path was added.
- The DMA ownership census completed (channels 1 and 2 are unused throughout
  the tree; channel 0 is live during stage draw for texture staging, channel 3
  for mid-frame fills) — retained as recoverable evidence should a future task
  revive the replay path.

## The decisive open question (for the owner)

Task 36 replay is disabled by `gNdsTaskmanArenaChosenSize != 0x150000` because
the robust downward-stepping arena allocator (diagnostics.c:7368) cannot secure
the full 0x150000 on the DS heap. Two distinct paths forward, neither of which
this task takes without owner direction:

1. **The arena guard is the bug.** The replay admission was written assuming a
   full 0x150000 arena; the allocator was later made robust and now steps
   down. If Task 36 replay is meant to ship, the guard should admit the
   replay buffer whenever it fits the *actual* chosen arena (e.g. size the
   replay capacity to the chosen arena, or drop the exact-0x150000 check).
   That is a Task-36-correctness fix, not a Task-52-DMA task, and it may be
   the reason Task 36's measured STG win never materialized.

2. **The replay path is intentionally retired.** If the generic emit path is
   now the intended stage draw, then the Task 36 replay machinery is
   vestigial and the campaign's STG 569K is owned by the generic path — which
   DMA-replay cannot help (it is not a replay loop).

This STOP also reframes Task 51's KILL claim. Task 51 reported "the 8 drawing
bindings are rigid, so they take the existing LoadNoZMatrix (LOAD4X4) path in
both A and B" and attributed STG 569K to "the rigid layer0 bindings drawing
through LoadNoZMatrix — and Task 36 is the path that owns the rigid subset."
The actual shipping state is that **Task 36 replay is disabled**, so those
bindings draw through the *generic* emit (which also uses LoadNoZMatrix per
run via BeginRun), not Task 36's replay. The STG cost is real; its attribution
to "Task 36 replay" was not.

## Build environment note

Git Bash's direct `make` hits the documented `/opt/devkitpro` recursive
sub-make path quirk. Build through the devkitPro msys2 bash:
`C:/devkitPro/msys2/usr/bin/bash.exe -lc 'cd repo && make TARGET=... BUILD=... -j16'`.

## Disposition

STOP at E0. Nothing merges — the published ROM stays `1818AA77…`. The branch
`codex/task52-stage-gxdma-replay` holds the E0 evidence (this file, the probe
script, the baseline JSON). The replay-path-inactive finding is the deliverable;
it prevents a DMA implementation against a path that does not run.
