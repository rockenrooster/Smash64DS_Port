# ITCM fourth pack: the wall sweep's fast path in, its slow path out, 2026-10-04

The late-window lab census (`artifacts/task37-census/sz-lateprof03`, section D)
ranked `ndsMPWallSweepStaticMiss` first among main-RAM code by non-memory stall
per byte (~11,200 cycles a byte at 524 B; 110 calls a frame at ~350 cycles a
call, Thumb in main RAM). Its slow path, `ndsStageMPAdjustFloorLoopWallSweep`,
was admitted to ITCM on 2026-09-27 when every wall sweep ran it; since the
all-reject fast path (2026-10-02) answers most calls first, its 2,016 B rented
at ~840 cycles a byte.

The slow path went back to main RAM, and the bytes went to the fast path and
24 more section D functions by name in `linker/nds_hot_text.ld` (`.text.hot`
members untouched): the wall sweep truncation, `ndsAObjEvent32CollectActiveMObjs`,
the hot-stack thunks, the stage traversal begin, the particle float
conversion, two map-collision helpers, camera bounds checks and small leaves.

The first pack (`build-gate-1004j`, `gate-pack.*`) also named
`ndsRendererAdapterNdlDispatchEffect` and `ndsFtrLeanRunOnHotStack`: 112 B and
10 B in the tick-HUD gate ROM, but 1,300 B and 2,232 B in the shipping ROM,
which inlines their bodies -- the shipping link overflowed ITCM by 1,680 B.
Both left the list (`build-gate-1004l`, `gate-pack2.*`, the committed
configuration). ITCM: gate 32,352 -> 32,416 B, shipping 32,352 -> 32,072 B of
32,736.

Official gate against a same-session rerun of `build-gate-1004h`:

| | WORK P50 | WORK P95 | > 1.12M | two-VBlank | SRC P50 | MISC P50 |
|---|---:|---:|---:|---:|---:|---:|
| base (`build-gate-1004h`, rerun) | 925,376 | 1,273,664 | 286 | 1,668 / 1,961 | 398,464 | 173,120 |
| first pack (`build-gate-1004j`) | 919,744 | 1,261,248 | 280 | 1,675 / 1,961 | 395,072 | 169,664 |
| committed pack (`build-gate-1004l`) | **920,704** | **1,259,712** | **280** | **1,678 / 1,961** | 393,664 | 171,072 |

Committed pack paired by frame: 1,788 of 1,960 frames better, median -5,376.
Replay digest identical on all 1,960 rows (both packs).
