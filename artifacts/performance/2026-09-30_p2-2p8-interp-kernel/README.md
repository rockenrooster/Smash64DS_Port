# syInterpGetFracFrame's bisection in ARM state on bit patterns (2026-09-30)

## Why

Owner (2026-09-30): the Sector Z Arwing is what drives that stage's P95. It is a
moving platform, so its TraI path is gameplay state on every tick. A per-frame
split of the Sector Z lab profile (`builds/p2p8-prof-sz4`, frames with the
Arwing's spline or draw vs the rest) puts ~235K ticks on each Arwing frame:
~104K in the arc-length bisection (soft float) and ~125K drawing its eight
FoxSpecial3 roots. With the owner's roster (Kirby/Fox/Yoshi/Pikachu, no Samus,
so every call is the Arwing's) the match makes 1,950 calls, 0 whole-call memo
hits, ~10 new bisection nodes and ~62 quartic evaluations each.

## Change

`include/nds/nds_interp_exact.h` runs the Bezier/Catrom arm of
syInterpGetFracFrame from `time_scale` on, on binary32 bit patterns, in ARM
state: the source's operations in the source's order, with multiply and add as
the library's own calls (ITCM), `4.0F*q`, `2.0F*q`, `/8`, `/2.0F` as exact
exponent adjustments of normal values, IEEE compares on the bits, positional
sample reuse (child's even samples = parent's first five after a step left,
first sample = parent's last after a step right) and the existing path reuse.
`src/import/battleship_sys_interp.c` calls it after the memo; the float
replica stays behind the same-ROM A/B word `gNdsInterpFracKernel` (1 = kernel).

## Exactness

- Host: `scripts/test_interp_exact_kernel.c` (build line in its header) compares
  every piece and the whole bisection with decomp sys/interp.c's own text:
  160M operation checks (zeros, subnormals, infinities, NaNs, overflow,
  underflow included), 4M quartics, 900K bisection calls over 3,000 random
  Bezier segments walked in small steps (7.96M nodes reused, 7.16M computed):
  **0 mismatches** at -O2 and -O0 (two NaNs compare equal: a NaN's sign never
  reaches the result).
- Device: `gNdsInterpFracOracle=1` on the Sector Z lab ROM ran the source beside
  every call: **1,950 compares, 0 mismatches**.
- Replay digest kernel on vs off: IDENTICAL over 1,972 samples.

## Cost

Same lab ROM (`build-p2p8-lab-cw`), Sector Z, owner roster, run-s6, frames >= 64:

| arm | P50 | P95 | P99 |
| --- | ---: | ---: | ---: |
| kernel 0 (`iyk0_sz`) | 1,072,064 | 1,461,056 | 1,671,936 |
| kernel 0 (`iyk0b_sz`) | 1,072,064 | 1,462,656 | 1,671,744 |
| kernel 1 (`iyk1_sz`) | 1,071,872 | 1,452,864 | 1,669,248 |
| kernel 1 (`iyk1b_sz`) | 1,071,936 | 1,453,312 | 1,671,744 |

P95 -8.8K (pair means), P50 flat (the Arwing is out on ~40% of frames). Frames
align 1:1 (identical digest): the kernel moves 733 frames, by -13.4K and -7.7K
on average in the two pairs; control vs control moves 30 frames by -1.8K.
Native failures 0, heap low-water unchanged.

**A first variant lost.** With the integer add/multiply inlined (host-proved
too), GCC grew the kernel to 12.5 KB of ARM code in main RAM (ndsIxQuart 4,128
B, ndsIxIntegral 3,968 B, ndsIxBisect 4,412 B) -- over the 8 KB I-cache -- and
Sector Z P95 went UP 48K (`ixk1_sz` 1,509,952 vs `ixk0_sz` 1,461,312; oracle
0 mismatches). The shipped variant keeps libgcc's ITCM add/multiply (1.9 KB of
kernel). On the host the integer forms remain, so the proof covers them.

## What is left

The bisection's float work itself (~62 quartics a call through libgcc) is now
most of the ~75K per Arwing frame. Exact alternatives: a per-pattern result
table for the Arwing's eight fixed flight patterns (each plays from frame 0 the
same way every time), or fewer calls. The Arwing draw (~125K a frame) is the
larger remaining piece.
