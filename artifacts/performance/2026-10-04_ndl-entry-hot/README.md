# NDL dispatch and entry effects on the DTCM hot stack, 2026-10-04

Two more draw subtrees join the stage DL fast lane
(`../2026-10-04_stage-dl-fast-hot`) on the DTCM hot stack:

- `ndsRendererAdapterNdlDispatchEffect` keeps its cheap admission tests on the
  caller's stack and runs the rest (the GObj's record, its owner's native
  emit: impact waves, the Link bomb, damage slashes, ground and weapon owners)
  through `ndsDtcmHotStackRun`. Same-ROM A/B word `gNdsNdlHot`.
- `ndsRendererAdapterTryNativeEntryEffect` (entry packets, the rebirth halo),
  reached when the fast lane declines. A/B word `gNdsStageDLEntryHot`.

What makes them fit: `NDS_RENDERER_MODELVIEW_STACK_SIZE` is 1 on DS. G_MTX
push and G_POPMTX are interpreted only by the host reference
(`src/host/graphics_reference/nds_renderer_reference.c`); no DS path pushes, so
the 32-entry modelview stack in every `NDSRendererTraversalState` was 2,176 B
of dead frame. The state is now 888 B (was 3,000 B).

## Depth (static, direct calls plus the route table's owners; assert/printf
paths excluded)

| hot-stack entry | before | now |
|---|---:|---:|
| stage DL fast lane | 5,736 B | 3,632 B |
| NDL dispatch (deepest: an impact wave's ring through a texture allocation) | 6,672 B | 4,560 B |
| entry effects (`TryNativeEntryEffect`'s own 2,880 B frame) | -- | 5,040 B |

The 6,144 B stack; IRQ handlers run on calico's IRQ stack. None of the three
reaches a coroutine swap, a storage read or a nested hot-stack call. Official
ROM after 1,900 frames with all three on: high-water 4,508 B (sampled every
256th entry), 0 native failures.

## Gate (official target, same ROM `build-gate-1004e`, ring dump, 1,960 samples)

| | WORK P50 | WORK P95 | > 1.12M | MISC P50 | two-VBlank |
|---|---:|---:|---:|---:|---:|
| `gNdsNdlHot=0`, `gNdsStageDLEntryHot=0` | 938,304 | 1,295,936 | 334 | 182,144 | 1,619 / 1,961 |
| `gNdsNdlHot=1`, `gNdsStageDLEntryHot=1` | **934,144** | **1,280,448** | **311** | **177,152** | **1,639 / 1,961** |

Replay digest (DGSA/DGSB) identical on all 1,960 rows. The off arm against the
previous ROM (`build-gate-1004d`, fast lane hot): P50 938,368 -> 938,304, P95
1,302,464 -> 1,295,936 -- the smaller state alone is inside layout noise.
`gate-off.*`, `gate-on.*`.
