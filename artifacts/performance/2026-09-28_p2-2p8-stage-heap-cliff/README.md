# P2-2p8: the stage-draw layout cliff is a heap cliff (2026-09-28)

Two lab ROMs 1,888 B of `.bss` apart read Dream Land STG 169K vs 230K and
Jungle 204K vs 270K. Replay digests were identical. This was not cache phase.
The run rows and logs are in `../2026-09-26_p2-2p8-ftr-item-tail/`, under the
arm names below.

## Mechanism

- The slower layout's taskman arena was 8,192 B smaller: 1,163,008 vs
  1,171,200, with 130 vs 128 failed page probes.
- That flipped two optional stage accelerators past their keep-free floors:
  - Jungle: the stage GX body was declined. `gNdsP2StageProgReason` 3
    (heap), 0 loads, 0 program draws, 7,892 declines. The heap low-water
    rose by exactly the 28,064 B the body would have taken.
  - Dream Land: the stage world cache was refused
    (`sNdsRendererAdapterStageWorldCache` 0). The low-water moved by exactly
    its 4,608 B.
- Why 1,888 B cost 8,192 B: newlib's `malloc_extend_top` asks sbrk for the
  whole request past the break and does not count the free top chunk in
  front of it. The chooser probed from whatever top chunk boot left, which
  was 25,088 B in the slower layout (`gNdsTaskmanArenaPreTrimTop`, arm
  `cff`). The first probe that fit was freed, which trims the top, so the
  kept block sat under a break that could have taken more. With the search
  trimmed first, the same layout's arena grew by 23,552 B and libc's top
  shrank by 3,072 B, so 20,480 B had been left past the break. How much was
  left depended on layout.
- Earlier "cache-phase" readings that put tens of thousands of ticks into STG
  from a small `.bss` change may have been this cliff. Check the arena size
  and these two counters before reading one as cache phase.

Arms `stc`/`stn` (counters) and `hpc`/`hpn` (heap addresses) are the two
original ROMs.

## Fix: `ndsTaskmanArenaBytes` (src/port/diagnostics_taskman_heap.c)

1. `malloc_trim(0)` before the page search, so the search sees the break the
   kept block will use.
2. After the kept calloc, the arena returns only what libc's top chunk lacks
   of `NDS_TASKMAN_LIBC_TOP_BYTES` (the reserve + 0x2100). Before, it
   returned a fixed reserve on top of the rounding slop.

The arena now tracks `.bss` byte for byte, less at most one 4 KiB sbrk page.
Adding a 1 KiB pad moved it by exactly 1,024. libc's top chunk is never
smaller than any layout could have given it before. It no longer gets the
unclaimed space past the break, which no lab run reached.

Step 1 alone was tried first (`cff`). It gave libc 41,720 B, the reserve plus
760. Dream Land's fighter texture admission then stopped at its 16 KiB libc
floor, the on-demand path exhausted libnds' heap, and the run faulted at
frame 1368 in `glGetColorTableParameterEXT`. The old unclaimed tail had been
hiding that margin. Step 2 closes it.

## Results

The lab ROM is all stages with Pikachu, Yoshi, Ness and Purin, at 1,972
samples. Layout A is HEAD's layout; layout B adds 1 KiB of `.bss`. All four
builds came from one tree snapshot. Replay is IDENTICAL for every candidate
against `ctl2`.

| Arm | Arena | Dream Land WORK-H P50/P95 | DL STG P50 | Jungle WORK-H P50/P95 | J STG P50 |
|---|---|---|---|---|---|
| `ctl2` (A, before) | 1,163,008 | 1,001,536 / 1,394,496 | 224,640 | 1,063,040 / 1,482,560 | 267,328 |
| `fix2` (A, after) | 1,177,600 | 948,736 / 1,334,976 | 170,432 | 1,000,832 / 1,413,376 | 204,736 |
| `ctl2pad` (B, before) | 1,167,104 | 947,456 / 1,332,224 | 170,048 | 1,001,728 / 1,418,816 | 206,976 |
| `fix2pad` (B, after) | 1,176,576 | 948,736 / 1,335,808 | 170,688 | 1,003,776 / 1,420,288 | 207,552 |

- Paired, layout A: Dream Land -53.1K median (1,968 of 1,972 frames better),
  Jungle -61.5K (1,962 of 1,972).
- Paired, layout B, which was already fast: +0.6K and +1.3K.
- libc's top chunk now starts at 49,432 B in both layouts, against
  44,792-47,640 before. Its minimum was 18,424 B on Dream Land and 21,808 B
  on Jungle. Texture admission failures: 0.

**Gate** (`smash64ds-p2-fourcpu-tickhud-hwtri`, empty bootset, `scg_ctl` vs
`scg_fix`, one snapshot):

- P50/P95 went from 931,584 / 1,298,816 to 931,584 / 1,299,968. The paired
  median is +128.
- Arena 1,335,040 -> 1,344,256. libc top 46,488 -> 49,560 (minimum 18,584).
- The heap low-water is 69,340 in both. Replay is IDENTICAL, and also
  against `glm1`.

`scripts/menus/test_arena_capacity.py` now models the untrimmed top chunk
and checks the fixed libc share. It had not compiled since the 1 KiB
alignment slack landed.
