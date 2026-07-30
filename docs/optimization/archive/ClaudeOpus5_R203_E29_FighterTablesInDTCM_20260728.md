# R2-03 E29 — the fighter's hot vertex tables move to DTCM

**Date:** 2026-07-28
**Verdict:** **KEEP.** FTR −26,816 ticks/frame (median of 128 paired frames),
128/128 frames improved, geometry bit-identical.

## The finding

The fighter emit reads two tables, once per corner, 1,878 corners a frame, in an
order set by `sNdsNativeFighterPackedCorners` — i.e. randomly:

| table | bytes | contents |
|---|---:|---|
| `sNdsNativeFighterPreparedDense` | 8,656 | `gx_xy`, `gx_z`, `s`, `t` per dense vertex |
| `sNdsNativeFighterDenseNormals` | 2,164 | one packed normal per dense vertex |

Both sat in main RAM behind the ARM9's **4 KB data cache**. 10,820 bytes of
randomly-indexed working set against 4 KB is a 2.7x overcommit: essentially every
corner missed.

Meanwhile **DTCM — 16 KB of single-cycle, uncached, CPU-local memory — held 184
bytes.** Dumping DTCM at match frame 900 found 12,948 contiguous untouched bytes.

## Why DTCM was free, and why that was not obvious

`linker/nds_hot_text.ld` puts the initial user stack at the top of DTCM
(`__sp_usr = 0x02ff3e80`) and the `dtcm` MEMORY region spans the space it grows
down into, so the region length is not available space and the linker cannot
catch a collision on its own. That is a good reason for caution and it is why
the space had gone unused.

Measured rather than assumed. At the frame-complete marker `sp = 0x02296530` —
**main RAM**: game code runs on a Calico thread stack, and the DTCM stack is only
the boot stack, before the scheduler starts. Its deepest reach is `0x02ff3340`,
2,880 bytes below `__sp_usr`. Everything below that is dead space.

Both tables were audited against the `check-task20-dtcm-layout.ps1` placement
requirement — DTCM is invisible to DMA and to the ARM7 — and both are written and
read only by ARM9 code in the fighter draw. The renderer's only GXFIFO DMA is
R2-02 E2's stage replay out of `owner->words` in main RAM. The gate's own
independent scan agrees: `forbiddenDmaRefs=0`.

## The struct shrink, and an honest note about it

`NDSNativePreparedDenseVertex` carried `shaded_rgba` (4) and `packed_color` (2)
that are dead under `NDS_R2_FIGHTER_HW_LIGHT`: all four emit paths write
`GFX_NORMAL`, never `packed_color`, and every epoch is lit
(`gNdsR2ShadeLitEpochs == gNdsR2ExecEpochCalls == 22,296` over 480 frames), so
the loop that writes them never runs. Dropping them takes the struct 16 -> 12
bytes and the table 8,656 -> 6,492.

A `_Static_assert` demanded 16 bytes, "must stay power-of-two", and that
assertion was **right on its own terms**: at 16 bytes two entries fit a 32-byte
cache line and none straddles; at 12 bytes one access in three spans two lines.
Measured in main RAM the shrink was worth a median of −5,376 with a mean of
−1,122 — at the noise floor, the straddle penalty eating the footprint win. It
is **not** a cut on its own and is not claimed as one.

In DTCM there are no cache lines, so 12 bytes is strictly better than 16 and the
2,164 bytes it saves are 2,164 more bytes of margin under the boot stack. The
two changes ship together for that reason.

## Evidence

Control = E28 (`5cd9b86`), candidate = this change. Same target, emulator,
window; ring dump, 128 samples, frames 439..566, paired by frame number.

| bucket | better | worse | median delta | worst regression |
|---|---:|---:|---:|---:|
| **FTR** | **128/128** | **0** | **−26,816** | −9,472 (still an improvement) |
| STG | 108 | 20 | −1,280 | +1,792 |
| WORK | 120 | 8 | −28,096 | +315,328 (excursion frame) |

`STG` improving is the second-order effect worth noting: the stage never touched
these tables, and got faster anyway because 10,820 bytes stopped competing for
the data cache. Whole-frame cache pressure is a shared resource, so a table moved
out of main RAM pays subsystems that never referenced it.

VBlank interval histogram: E28 `2:438 3:117 4:9 5+:2 max:18` ->
E29 `2:446 3:109 4:9 5+:2 max:18`.
WORK frames over the 1,120,000 gate: 40/128 -> 39/128.

**Structural:** `gNdsFighterDLAllDrawP0HardwareTriangleCount` = 136,640 and
`...P1...` = 146,880 over frames 439..919, identical to the control. Boundary
green.

## Guard rails added, because this one can fail silently

- **Linker `ASSERT( __dtcm_bss_end <= 0x02ff3000 )`.** The region length covers
  the boot stack's growth space, so the linker's own overflow check does not
  cover a collision. The ceiling keeps 832 bytes under the measured low-water
  mark. Raising it requires re-running the measurement, not editing the number.
- **New `.dtcm.fighter` input section, placed first and followed by
  `. = ALIGN(32)`.** The fighter tables' combined size is data-driven (541 dense
  vertices); without the realign they pushed Calico's `__irq_table` off its
  32-byte boundary, which is exactly what the Task 20 gate caught.
- **`check-task20-dtcm-layout.ps1` allow-list extended** with the two owners,
  all-or-none, with the 32-byte rounding written down.

## Two process failures, both worth keeping

1. **`make` does not regenerate the generated includes — `build.ps1` does.**
   The first E29 build changed `NDSNativePreparedDenseVertex` from six fields to
   four while `nds_native_fighter_owner.generated.inc` still held six positional
   initializers, dated four days earlier. GCC warned about excess elements and
   assigned `gx_z = 0`, `s = 0xfd40`. It built, it ran, and it produced a
   complete A/B (FTR −5,376, 116/128) **on a ROM with every Z coordinate
   zeroed**. Any change to a generated struct's layout must regenerate before
   measuring. The generator now emits designated initializers, so this specific
   mismatch cannot silently recur.
2. **The build-output filter hid the warning.** The `Select-String` pattern was
   narrow enough to drop `warning: excess elements in struct initializer`. Filter
   build output for *new* warnings, not for a fixed list of the ones already
   expected.

## Where R2-03 stands

Graduated: E17 17,600 + E16 35,072 + E28 31,488 + E29 26,816 = **110,976** of the
250,833 gap (44%).

Remaining ranked items in the fighter execute, unchanged by this cut because it
removed memory stalls rather than work: state replay 72,798 coupled to
`PrepareProductionRun` 41,928 (E26's target), and the per-root 44,785.

**And a new one this opens.** 7,144 bytes of DTCM remain between the tables and
`__sp_usr`, 4,264 of them under the boot stack's measured low-water mark. The
next-hottest randomly-indexed table is `sNdsNativeFighterPackedCorners` at 3,756
bytes, which fits. It is streamed rather than randomly indexed, so it benefits
less from the move — but it is now a cheap experiment rather than an idea.
