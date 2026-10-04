# Stage DL fast lane on the DTCM hot stack, 2026-10-04

`ndsRendererAdapterSubmitStageDLImpl` now runs `ndsRendererAdapterSubmitStageDLFast`
through `ndsDtcmHotStackRun`. The fast lane carries the routed item, weapon and
effect owners (Beam Sword, Samus's Charge Shot, baked roots, visual effects);
their 3,000-byte `NDSRendererTraversalState` frames and the lane's own frames
were on the main-RAM stack, and the late-match lab profile
(`artifacts/task37-census/sz-lateprof01`) charged the lane ~10 cycles an
instruction, its worst rows stack pops and frame loads (`pop` 77 cycles,
`ldr r1, [sp, #52]` 160 cycles in `ndsRendererAdapterPrepareInitialMatrices`).
Same-ROM A/B word `gNdsStageDLFastHot` (default 1).

## Depth

- Static reach from the lane (direct calls, the route table's owners, libc and
  libnds callees; assert/printf paths excluded): 5,728 B, through a baked
  owner's texture allocation (`glTexImage2D` -> `vramBlock` -> `malloc`), of
  the 6,144 B stack. Typical owner paths are 4.5-5.2 KB.
- calico runs IRQ handlers on the IRQ-mode stack, so interrupts add nothing.
- Lab high-water (`gNdsDtcmHotStackHighWater`, sampled every 256th entry) after
  1,900 frames with the lane on: 5,136 B.
- The lane reads no storage, switches no coroutine, and hands DMA only static
  buffers (texture scratch, packet words; palettes go through `svcCpuSet`).
- A lane reached from a subtree already on the hot stack (the fighter display's
  magnifier) runs in place, as it did before.

## Lab (four-CPU lab ROM `build-lab-hot`, same ROM, frames 300-2040)

| | WORK P50 | WORK P95 | WORK mean | MISC mean | MITM mean | > 1.12M |
|---|---:|---:|---:|---:|---:|---:|
| `gNdsStageDLFastHot=0` | 1,060,480 | 1,433,792 | 1,086,191 | 225,983 | 60,306 | 658 |
| `gNdsStageDLFastHot=1` | 1,048,768 | 1,405,952 | 1,076,206 | 216,149 | 52,345 | 621 |

Frames 1,200-2,040 (the item phase): WORK mean -20.6K, MISC -15.5K, MITM -12.8K.
Replay digest identical on every frame; 0 native failures.

## Gate (official target, same ROM `build-gate-1004d`, ring dump, 1,960 samples)

| | WORK P50 | WORK P95 | > 1.12M | MISC P50 | MITM P50 | two-VBlank |
|---|---:|---:|---:|---:|---:|---:|
| `gNdsStageDLFastHot=0` | 946,304 | 1,309,184 | 355 | 189,888 | 62,784 | 1,597 / 1,961 |
| `gNdsStageDLFastHot=1` | **938,368** | **1,302,464** | **330** | **183,040** | **55,680** | **1,623 / 1,961** |

Replay digest (DGSA/DGSB) identical on all 1,960 rows. `gate-hot0.*`,
`gate-hot1.*`.
