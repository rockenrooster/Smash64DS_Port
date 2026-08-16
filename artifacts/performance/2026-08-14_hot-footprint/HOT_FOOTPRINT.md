# Hot code footprint — where the 339,275 ticks of instruction fetch actually go

**Date:** 2026-08-14
**Owner named by:** `../2026-08-14_icache-temporal/ICACHE_TEMPORAL.md`, which
closed link-order placement on measured temporal evidence and named footprint
reduction as the successor.
**Status:** census only. **Nothing was built, no ROM changed.** One lane sizes
above the 17,000-tick bar and is specified for the next cycle.

**UNITS: 2 profile cycles = 1 project tick.**

---

## 1. The quantity being attacked

From the v3 stall attributor (`stall_partition_residual = 0`):

```
icache_fill  1,086,361,126 cycles = 678,551/frame = 339,275 ticks/frame
             29.7% of the match, 37.5% of non-idle, 1.87x the cost of `issue`
```

Placement cannot reach it: sets whose hot population fits inside the 4 ways
refill at the same rate as oversubscribed sets. The remaining lever is **fewer
bytes fetched**, and the unit of fetch is a **32-byte line = 16 Thumb
instructions**, paid in full whether one instruction in it runs or sixteen.

## 2. What is inside the lines we pay for

`scripts/census-fetch-density.py` and `scripts/census-literal-pools.py`.

9,011 non-ITCM lines are fetched at least once — **288,352 bytes paid**. Split by
what actually occupies them:

| | bytes | share of paid | removable? |
|---|---:|---:|---|
| **live** — instructions that executed | 213,040 | 73.9% | no, this is the work |
| **literal pool** — `[pc, #N]` load targets | 5,780 | 2.0% | **no** — Thumb-1 cannot materialise 32-bit constants inline; the code needs them |
| **cold code** — never executed, not a pool | **42,892** | **14.9%** | **yes** |
| alignment / inter-function padding | 26,640 | 9.2% | partly, and cheaply |

**The literal-pool split is why this document exists rather than a headline.**
The raw "dead-in-line" figure is 26.1%, which against 339,275 ticks reads as
~88,000 ticks of headroom. Thumb-1 emits constants into pools inside `.text`;
objdump disassembles them as instructions and they never appear in a PC profile,
so a naive count books them as removable. Resolving every `[pc, #N]` target shows
pools are only 2.0% here — so the confound is small and the lever survives — but
the check had to be made before sizing anything, not after.

## 3. The ceiling, and the honest discount

Every cold byte counted is, by construction, inside a line that **also** holds
live code — a line of pure cold code is never fetched and costs nothing. So cold
bytes are only recoverable insofar as removing them lets the live code compact
into fewer lines.

```
needed:   213,040 live + 5,780 pool = 218,820 B  ->  6,839 lines
current:                                             9,011 lines
                                          reduction:   24.1%
```

**Ceiling: 24.1% of 339,275 = ~81,800 ticks/frame**, and that assumes perfect
compaction — live bytes made fully contiguous, which basic-block granularity will
not deliver. A realistic capture of a third to a half of that is
**~25,000–40,000 ticks/frame**, which clears the 17,000 bar with margin. This is
the first lane this week to size above the bar with measured backing rather than
inference.

## 4. Where the fetch lives, by object

`scripts/census-text-owners.py` (linker map joined to the profile).

312 objects in main text, **914,634 B total, of which 397,236 B (43.4%) is in
functions that execute at all** and 276,352 B sits in fetched lines.

| text B | exec B | exec% | fetched B | object |
|---:|---:|---:|---:|---|
| 199,080 | 94,204 | 47.3 | **61,664** | `scene_backend.o` |
| 187,376 | 91,156 | 48.6 | **55,008** | `nds_renderer.o` |
| 22,896 | 19,968 | 87.2 | 14,848 | `battleship_ftmain.o` |
| 23,448 | 17,908 | 76.4 | 13,472 | `battleship_lbparticle.o` |
| 23,712 | 17,148 | 72.3 | 11,616 | `battleship_ftcomputer.o` |
| 18,240 | 12,332 | 67.6 | 9,504 | `battleship_sys_objanim.o` |

**Two objects carry 42% of all instruction fetch**: `scene_backend.o` and
`nds_renderer.o`, 116,672 fetched bytes between them — and each is only ~48%
executed, so half of what they contribute never runs.

## 5. Two findings that are not the main lever but should not be lost

**Fully-cold objects linked into main text.** These never execute, so they cost
**zero** I-cache fill — they are a ROM/RAM question, not a fetch one, and must not
be conflated with the lever above:

| text B | exec% | object |
|---:|---:|---|
| 22,088 | **0.0** | `battleship_mnplayersvs.o` |
| 18,392 | 11.3 | `libfat.a(ff.o)` |
| 15,348 | **0.0** | `battleship_mnvsresults.o` |
| 14,420 | **0.0** | `libg.a(libc_a-categories.o)` |
| 7,388 | **0.0** | `battleship_mnmaps.o` |
| 7,244 | **0.0** | `battle_playable_static_textures.o` |

66,488 bytes of never-executed text survives `--gc-sections`. Given
`RAM is not free — the GObj cap` (heap low-water already under threshold, and
+14 KB of bss once stopped the ROM booting), that is worth a look on its own
terms.

**newlib formatted I/O is in the hot fetched set.** `_vfiprintf_r` (1,034 live
bytes, 45.1% cold), `_svfiprintf_r` (956), `__ssvfiscanf_r` (714) are being
*fetched during a battle match*. `syDebugPrintf` is reachable from
`syTaskmanCheckBufferLengths`'s overflow path; something is calling formatted I/O
in steady state. Small (2,704 live bytes) but it is pure waste and it points at a
live overflow or a stray diagnostic.

**GCC clone bloat is a dead lever: 168 bytes** across 5 families
(`ndsRendererTransformVertex20p12` 72 B, `syUtilsArcTan` 28 B, and three smaller).
Checked and closed for the cost of one scan.

## 6. The specified next experiment

> **STOP — step 1 below is REFUTED, 2026-08-15, zero builds spent.** The
> compiler capability gate fails outright: devkitARM **GCC 15.2.0** answers
> `cc1.exe: note: '-freorder-blocks-and-partition' not supported on this
> architecture` and emits **no** `.text.unlikely`/cold section at all. Checked
> three ways on a probe TU with a bulky `__builtin_expect(...,0)` path, under
> the repo's exact `-O2 -ffunction-sections -fdata-sections`: `-march=armv5te
> -mthumb` (the build's `ARCH`), `-march=armv5te -marm`, and `-march=armv7-a
> -marm`. The section table is **identical with and without the flag** in every
> case, so this is an ARM back-end limitation, not a Thumb-1 or `-Os`/`-O2`
> artifact and not something a different `-march` or a section attribute
> unblocks. **The linker was never the obstacle** — `linker/nds_hot_text.ld`'s
> `.main` already collects `*(.text.unlikely .text.*_unlikely .text.unlikely.*)`
> ahead of `.text.hot` and the `.text.*` catch-all, so a cold partition would
> have been grouped away from its hot bodies for free if one had ever existed.
>
> §3's 42,892 B is therefore reachable only through **step 3** — manual
> out-of-lining of cold fallback/error paths — which is hand work per function,
> not a flag, and must be sized against the ≥16,000 tk/fr floor before it is
> started. Do not re-run step 1.

**Hot/cold basic-block splitting.** GCC's `-freorder-blocks-and-partition` moves
cold basic blocks out of a function's body into `.text.unlikely`, which is exactly
the transformation §3 sizes: it converts cold-bytes-sharing-a-line-with-live-code
into lines that are simply never fetched.

Before it is run, three things must be settled, because this is **not** a
placement change — it alters code generation:

1. **It is not "same objects".** Gameplay, collision and RNG behaviour must be
   re-verified, not assumed. Boundary at minimum.
2. **Total `.text` will likely GROW** (extra branches to the split-out blocks)
   while *fetched* text shrinks. That is the correct trade here, but the RAM
   ledger must be checked against the GObj-cap threshold, not waved through.
3. **Verify it does anything on this target.** Check `.text.unlikely` actually
   appears in the map for `-mthumb` ARMv5TE, and that the two dominant objects
   (`scene_backend.o`, `nds_renderer.o`) are affected. If the section is empty,
   stop there — no build spent.

The measurement is already specified and cheap, because the instrument now
exists: re-run the v3 attributor and compare `stall_icache_fill` directly. **That
is a far stronger signal than WORK-H moving**, and it is the check the placement
lane lacked until this week.

Sequencing note: because it changes codegen, this should be measured with the
same one-control/one-candidate discipline as any other change, but the primary
evidence is the fill delta, not the tick delta.

## Reproduce

```bash
arm-none-eabi-objdump -d builds/build-c125-profile/smash64ds-battle-playable-tickhud-hwtri.elf > c125.dis
V3=artifacts/performance/2026-08-14_icache-temporal/v3-baseline/arm9-profile.csv
python scripts/census-fetch-density.py  $V3 --dis c125.dis --regions 1601
python scripts/census-literal-pools.py  $V3 --dis c125.dis
python scripts/census-text-owners.py    $V3 --dis c125.dis --map builds/build-c125-profile/.map
```
