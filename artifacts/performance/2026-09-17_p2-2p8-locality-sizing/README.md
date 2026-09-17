# Data locality, sized — and the ceiling that was 113% is 90.6%

Follows `2026-09-16_p2-2p8-dcache-value/` and `…_stall-budget/`. Two things
here: a correction to the campaign's central number, and a per-structure sizing
of what layout can actually recover.

## 1. The correction (verified here, and applied to the source document)

`…_dcache-value/README.md` concluded that perfect data locality lands at
**1,028,189**, clearing the gate by 91,811, giving locality a ceiling of
**560,739 = 113% of the gap** — the only class in the campaign whose ceiling
exceeded the requirement. **The subtrahend is wrong.**

560,739 is *all data stall*. **Layout can only remove line fills.** The rest of
that bucket is not layout-addressable:

| component | tk/fr | source |
|---|---:|---|
| I/O register, FIFO, DMA wait | 27,311 | `…_stall-budget/STALL_CLASS_SIZING.md:102` |
| store / write-buffer drain | 41,733 | 560,739 − load stall 519,006 |
| icache fetch landing on load PCs | ~29,300 | `…RENDERER_STREAMING_SIZING.md` |
| load-use interlock | 18,000–45,000 | same |

`RENDERER_STREAMING_SIZING.md` had already isolated the fill component when it
corrected this directory's traffic figure, though it did not draw the ceiling
conclusion. Its penalty table gives it away:

| miss penalty | fills/fr | product |
|---|---:|---:|
| 40 cyc | 10,608 | 424,320 |
| **44 cyc (central)** | **9,644** | **424,336** |
| 48 cyc | 8,840 | 424,320 |

All three rows multiply to the same number because the table was **constructed**
by dividing one fixed stall figure by the penalty. That figure is the fill
stall, and it is the layout ceiling.

| | tk/fr |
|---|---:|
| WORK-H P50 (arm D, pinned arena) | 1,588,544 |
| less **every line fill in the frame** | −424,336 |
| **WORK-H with a perfect data cache** | **1,164,208** |
| gate | 1,120,000 |
| **margin** | **OVER by 44,208** |

Against the baseline `…_dcache-value/` itself used (1,616,382) it is
**1,192,046 — over by 72,046**. Taking the interlock term at its high end
lowers the ceiling to ~396,500 and the margin worsens.

**The conclusion is robust across the entire uncertainty band: removing 100% of
the frame's data-cache line fills does not reach the gate.**

Corrected ceiling **424,336 = 90.6% of the 468,544 gap**. Locality is still by
a wide margin the largest class and the only one worth spending builds on. It is
no longer a class that can finish the job alone.

### Why this was not caught earlier

The 1,394,560 figure — what disabling the dcache costs — is a *measurement*, and
it is sound. But it answers "what does the cache currently save", not "what
would a perfect cache additionally save". The second question was answered by
subtracting the stall bucket wholesale, and that bucket had already been
decomposed two directories away into parts that layout cannot touch. Two correct
artifacts, one unjoined seam.

## 2. Per-structure sizing

Attribution below is from the per-PC profile
`2026-09-16_p2-2p8-n0409-profile/arm9-profile.csv` (6,048,201 PCs,
instructions 148,735,753, cycles 487,368,912, regions 129) joined against
`objdump`/`nm` of the matching ELF, with base registers resolved backwards to
their defining instruction. Units `tk/fr = (cycles − instructions) / (2 × 129)`.

**Provenance.** Part 1 above is verified directly against the banked artifacts.
Part 2 is a delegated per-PC analysis whose pipeline self-validates — its
whole-program memory-op PC stall reproduces `STALL_CLASS_SIZING.md`'s 644,328 to
**one tick** — but whose per-symbol rows have **not** been independently
re-derived here. Treat the ranking as sound and any single row as owed a check
before it is built.

### 2.1 The finding: 314 scalar statics own 207 cache lines

| | value |
|---|---:|
| named statics ≤ 32 B reached through a literal-pool base | 314 |
| total bytes of data in them | 1,670 B (52 lines' worth) |
| **distinct 32-byte lines they occupy** | **207** (162% of the 128-line dcache) |
| accesses/frame | 10,368 |
| measured stall | **54,528 tk/fr** |
| line efficiency | 1,670 B useful / 39,680 B fetched = **4.2%** |

About **13% of the frame's entire fill budget is spent on 0.5% of its data**,
because `-fdata-sections` (`Makefile:3795`) gives each of these its own input
section and the linker scatters them.

Heaviest, with set index `(addr>>5) & 31`:

| tk/fr | acc/fr | symbol | addr | B | set |
|---:|---:|---|---|---:|---:|
| 2,194 | 265.7 | `s_highTickCount` | 0x0225e838 | 8 | 1 |
| 1,766 | 963.8 | `sNdsRendererTask36CaptureActive` | 0x02201d2c | 4 | 9 |
| 1,193 | 226.5 | `gSYTaskmanDLHeads` | 0x02250878 | 16 | 3 |
| 1,105 | 813.6 | `gMPCollisionGeometry` | 0x0224bf4c | 4 | 26 |
| 1,072 | 225.8 | `gNdsMPLineYakumonoHits` | 0x0222ff08 | 4 | 24 |
| 1,068 | 48.6 | `sNdsRendererBattleStaticTextureArmed` | 0x021fdd5c | 4 | 10 |
| 1,060 | 36.7 | `dSYTaskmanFrameCount` | 0x02251998 | 4 | 12 |
| 980 | 136.8 | `gGRCommonLayerGObjs` | 0x02257b78 | 16 | 27 |
| 849 | 453.9 | `sNdsEffectPacketArmed` | 0x02201d34 | 4 | 9 |

**There is no nameable pairwise conflict to fix.** Stall-weighted occupancy runs
440 (set 22) to 5,515 (set 12), but 342 hot lines over 32 four-way sets is ~10.7
lines per set — **every set is ~2.7× oversubscribed by the small statics alone**.
This is the same conclusion `…_placement-hazard/` reached from the other
direction, and it explains why re-phasing failed: **re-phasing moves lines, it
does not reduce their number.**

### 2.2 `GObj` and `DObj` — 136 B each, two lines per hot consumer

`decomp/BattleShip-main/decomp/src/sys/objtypes.h:188-222` and `:414-470`.
`id`@0 / `link_next`@4 / `func_run`@20 are line 0; `obj`@116, `flags`@124,
`user_data`@132 are lines 3–4. Every hot walker reads one of each.

Second-line stall, measured: `ndsFtPoseUpdate` `ldr r3,[r5,#116]` 2,787;
`ndsBaseGcRunAll` `ldr r3,[r0,#124]` 1,948; `ftGetStruct` `ldr r3,[r0,#132]`
1,925; `gcCaptureCameraGObj` 900; `gcParseDObjAnimJoint` 602; remainder 835.
**8,997 tk/fr**, plus `ndsFighterPacketInvalidate`'s `DObj.user_data`
`ldr r2,[r6,#132]` at **1,870**. Total **10,867 = 2.3% of the gap**.

### 2.3 Bulk movers

| site | tk/fr | geometry |
|---|---:|---|
| `memset` destinations | 19,466 | 885.3 iter/fr × 16 B = **14,165 B/fr cleared**, 442.7 lines at 40.3 tk/line |
| `memcpy` | 14,090 | 777.1 iter/fr × 16 B = 12,434 B/fr |
| `ndsFighterPacketStoreSplitModelview` | 8,669 | 54.8 calls/fr, 64 B in + 64 B out; compulsory floor 4,822, excess 3,847 consistent with a 64 B source spanning 3 lines |
| `armCopyMem32` | 4,173 | 99.1 iter/fr × 32 B, `ldm r1!,{8}` at 74.0 cyc/x — more than one full fill per 32 B iteration |
| **PC-relative literal-pool loads** | **67,858** | 11,467 exec/fr at 12.84 cyc/exec. On ARM946E-S these are data-side reads of `.text`, so they allocate **D-cache lines holding code**. Never attributed before |

Two single sites sit at impossible per-execution costs —
`ndsRendererTask36ReplayRun` `ldr r3,[r1,#184]` at 1,022.4 cyc/exec (3,547
tk/fr) and `ndsFighterPacketTryReplay` `ldr r1,[pc,#1960]` at 1,660.1 cyc/exec
(3,183 tk/fr). A fill is ~88 cycles; these are FIFO/DMA/write-buffer waits
booked on the next memory op. **6,730 tk/fr that no layout touches.**

## 3. Proposals, ranked

| rank | proposal | sized win | files | risk |
|---:|---|---:|---|---|
| 1 | hot scalar statics → DTCM / packed output section | **34,444–49,365** (7.4–10.5%) | `linker/nds_hot_text.ld` only | LOW-MED |
| 2 | delete redundant pre-overwrite clears | ≤19,466 (4.2%) | ~4 renderer call sites | MED |
| 3 | `GObj`/`DObj` field reorder | 10,867 (2.3%) | new `include/sys/objtypes.h` | **HIGH** |
| 4 | 32-byte-align split-modelview and `armCopyMem32` buffers | ≤6,027 (1.3%) | 2 declarations | LOW |
| 5 | `-fsection-anchors` | unsized | `Makefile:3795` | LOW-MED |

**Two closed lanes are re-opened on the grounds that they were closed on the
wrong candidate.**

**DTCM.** `…_dtcm-falsifier/` killed DTCM because "break-even is ~13 refetches
per resident line per frame" and the candidate it tested — `sNdsFtPartsFlat`, a
contiguous table walked once — yields K≈1. That is true of *that* candidate and
false for this set: these are 4-byte scalars re-read with the whole rest of the
frame in between. `sNdsRendererTask36CaptureActive` 963.8 acc/fr,
`gMPCollisionGeometry` 813.6, `sNdsEffectPacketArmed` 453.9. **K is 100–900.**
It also cannot repeat N05.03, which *added* a 3,264 B resident table (80% of the
cache); this strictly shrinks the resident set. Budget is hard: DTCM free is
**1,992 B** (`linker/nds_hot_text.ld:171`), and spending it all forecloses every
other DTCM candidate.

Greedy by stall-per-byte, with a conservative 1.0 tk/access interlock floor
subtracted: 512 B → 34,444; 1,024 B → 42,474; all 1,992 B → 49,365.

**`GObj`/`DObj` repack.** `STALL_CLASS_SIZING.md` recorded this blocked because
the struct lives in a byte-pristine `decomp/` header with no `include/sys/`
shadow. **The mechanism claim is wrong.** `Makefile:3768` puts `include` first
on `-I`, every include site uses the angle form `<sys/objtypes.h>`, and angle
includes search only the `-I` list — verified directly in the compile lines of
the 2026-09-17 four-CPU build. A new `include/sys/objtypes.h` shadows the decomp
header for **every** TU with no byte of `decomp/` changed, exactly as 43
existing port headers already do.

That makes it possible, not advisable. It is ranked HIGH risk because it
permanently forks a decomp type — the recorded `port-header-shadow-starves-decomp`
hazard — and `src/nds/nds_ft_pose.c:169-178` builds a table of
`(u8)offsetof(DObj, …)` with an unguarded cast that truncates silently past
offset 255.

## 5. Why the DTCM budget argument inverts for scattered scalars

`…_p2-2p8-dtcm-falsifier/` closes with "can DTCM hold anything that pays? **no**
— 1,992 usable bytes, and the break-even is ~13 refetches per resident line per
frame." That break-even is derived in `…_stall-budget/STALL_CLASS_SIZING.md:129`:

> 5,704 bytes is 178 cache lines. DTCM saves one fill per resident line per
> refetch avoided, so reaching 10% of the gap (49,638 tk/fr = 2,256 fills at 44
> cycles) needs every one of those 178 lines refetched 12.7 times per frame.

**5,704 bytes is 178 lines only if the bytes are contiguous.** 5,704 / 32 = 178.
That is the right conversion for the candidate it was written about — a
flattened parts table — and the wrong one for a 4-byte scalar, which occupies a
whole line by itself. The measured set here is **1,670 bytes across 207 lines**,
not the 52 that 32:1 would predict.

The break-even is stated per *line*, which makes it look like a fixed bar. It is
not: how high a bar your budget can clear depends on how many lines the budget
buys.

| candidate | DTCM bytes | lines | tk/fr | **per DTCM byte** |
|---|---:|---:|---:|---:|
| `sNdsFtPartsFlat`, 4 slots (measured, reverted) | 1,584 | 49 | 10,176 | **6.4** |
| hot scalars, 512 B arm (sized, unbuilt) | 509 | ~117 | 34,444 | **67.7** |

**10.5× better per byte**, from 7.4× more lines per byte. The scalar arm sits at
~6.8 refetches per line — *below* the falsifier's 12.7 — and still returns 7.4%
of the gap, because it buys 7.4× the lines for a third of the budget.

This does not overturn the falsifier's measurement, which stands: moving that
table bought −10,176, below the 14,080 cross-build significance floor, and
consuming 80% of a scarce budget for an under-floor result was correctly
refused. It overturns the generalisation drawn from it.

**Owed before this is banked:** the 34,444 is a delegated per-PC derivation that
has not been independently re-derived here, and its own falsifier is a build —
if WORK-H does not fall by ≳28,000 (2× the significance floor) the attribution
is wrong and the lane dies for one build. `scripts/check-dtcm-residency.py`
(added `46c7373cdf4`) must pass first, because a linker input-section pattern
that matches nothing gathers silently and reads exactly like a dead lever.

## 4. Verdict

Everything sized here at its ceiling totals **70,804–85,725 tk/fr = 15.1–18.3%**
of the 468,544 gap. With the previously-sized VRAM arena (55,669 / 11.2%) that
reaches ~26–30%.

**No subset reaches the gap, and the reason is structural rather than a failure
of search.** The gap is 10,649 line fills at 44 cycles; the frame performs
~9,644–9,880. Even a perfect data cache leaves the frame 44,208 over.

The two things that would change this answer are outside layout: reducing the
**number** of objects transformed per frame (`gNdsGCDrawsActiveMax` = 203), or a
fidelity decision under the PROJECT_GOAL Sacrifice Order. Both are already on
the board, and the owner decision is owed.
