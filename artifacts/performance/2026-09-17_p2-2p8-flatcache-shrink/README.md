# Shrinking the flat-parts cache buys nothing, and that closes the lane

**Measured negative, reverted.** Third arm on this table, third negative, and
this one says why the other two came out as they did.

## What was tried and why

`ndsFTParamsInvalidateSubtree` is 20,349 tk/fr at CPI 5.60 and 82% stall, and
its three hottest instructions are the flatten walk's pointer chase, which only
runs on a cache miss. Two arms already exist:

| arm | result |
|---|---|
| **widen to 16 slots** (N05.03) | miss rate 49.7% → 3.6%, **SRC −15,040** — then **STG +51,520**, **WORK-H +33,984**. 3,264 B is 80% of the 4 KB dcache. *"The fix works, and it cannot be paid for."* |
| **relocate to DTCM** at 4 slots (dtcm-falsifier) | **STG −7,936, SRC −4,992, WORK-H −10,176.** Mechanism confirmed. Under the 14,080 floor, and it spends 1,584 of 1,992 usable DTCM bytes |

Both move the table's *dcache footprint*. So the third arm moves it the cheap
way: **shrink the per-slot capacity** rather than grow or relocate. N05.03
reported `Overflows = 0` at `MAX = 48` in two arms, so 96 was headroom nothing
reaches — 1,584 → 816 bytes, 39% → 20% of the dcache, at zero behaviour cost.

## The instrument caught a void arm first

The first measurement came back with **nine lanes byte-identical to control** —
ALL, WORK-H, SRC, STG, FTR, MISC, OTHR, WAIT, WORK. That is not a null result,
it is one binary measured twice.

The cause: the Makefile emits `#define NDS_FTPARTS_FLAT_MAX 96u` into the
force-included `nds_build_config.h`, so the `#ifndef` default in
`reloc_backend_compat_shims.c` **never applies to a ROM**. Editing the source
default changed nothing. Confirmed from the ELF, not inferred:
`sNdsFtPartsFlat` still measured `0x630` = 1,584 bytes after the "changed"
build, and the ELF's mtime was newer than the edit, so it had relinked without
taking the change.

The arm was re-run against `Makefile:287`, and the falsifier was checked
**before** any lane was read: `sNdsFtPartsFlat` = `0x330` = **816 bytes**,
`nds_build_config.h` = `48u`.

## The valid arm

Divergence witnesses identical to control — `P0/P1HardwareTriangleCount`
349,031 / 333,618, `gNdsFtrPlanHit` 6,217, `gNdsGCDrawsActiveMax` 203,
`gNdsDamageSlashEffectsSeen` 540, `slips` 0, native failures 0. Same workload.

| | control | MAX = 48 | delta |
|---|---:|---:|---:|
| **WORK-H P50** | 1,600,960 | 1,602,176 | **+1,216** |
| **WORK-H P95** | 2,320,576 | 2,314,560 | **−6,016** |
| SRC P50 | 543,040 | 545,728 | +2,688 |
| STG P50 | 385,088 | 385,472 | +384 |
| FTR P50 | 350,144 | 349,696 | −448 |
| MISC P50 | 238,720 | 237,824 | −896 |
| OTHR P50 | 284,096 | 281,088 | −3,008 |

Both WORK-H figures are far under the **14,080-tick cross-build floor**.

**And it moved nothing on the memory axis either**, which was the second reason
to try it. All three byte-identical to control:

| | control | MAX = 48 |
|---|---:|---:|
| `gNdsTaskmanArenaChosenSize` | 1,351,424 | 1,351,424 |
| `gNdsTaskmanArenaAllocFailCount` | 84 | 84 |
| `gNdsTaskmanGeneralHeapFreeMin` | 111,200 | 111,200 |

The arena steps in 4,096-byte pages and 768 bytes does not cross one. This is
worth knowing for its own sake: the Yoshi egg lost a page for ~1 KB
(`…/2026-09-17_p2-3f53-yoshi-egg-owner/`), and **returning 768 B does not buy it
back** — the page boundary is not symmetric at this granularity.

## Why it was reverted rather than kept

Zero measured benefit on ticks, arena or heap, against a real risk: a subtree
over 48 parts silently reverts to the unflattened walk, and
`gNdsFtPartsFlatOverflows` — the counter that would catch it — was **removed**
when the census overflowed `.itcm` by 40 bytes. An unmeasurable change that can
only degrade silently is not worth carrying.

## What the three arms establish together

Removing **768 bytes** of the table's dcache footprint changed nothing, while
removing the **whole table** to DTCM moved −10,176. So what matters is whether
this table is in the dcache *at all*, not how large it is — **there is no
partial credit to collect on size.** That is why the shrink was flat and why no
intermediate capacity will pay either.

The lane's complete ledger is now: widening costs +33,984, relocating gains
10,176 (sub-floor, and spends 79% of remaining DTCM), shrinking gains nothing.
**The flat-parts cache is exhausted.** It should not be opened a fourth time
without a new mechanism, and the note recording that now sits in both the
Makefile knob and the source.
