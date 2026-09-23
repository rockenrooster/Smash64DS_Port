# Four-fighter frame, re-read as an architecture question (2026-09-22)

No new ROM was built or run for this record. It re-reads two existing four-CPU
measurements to answer a different question from the 09-16/09-17 ledger: not
"which leaf is next" but "which machinery is the frame made of, and which of it
runs in the frames that decide P99". It is the evidence base for
`docs/p2/FOUR_FIGHTER_30FPS_ARCHITECTURE.md`.

## Inputs

| input | what it is |
|---|---|
| `../2026-09-17_p2-2p8-dtcm-hot-scalars/fourcpu-rows.csv` | Whole-match tick-HUD rows, four CPUs (Donkey/Samus/Link/Kirby), Dream Land, items on, DTCM-scalar build; frames 440-1973 (1,535 presented frames) |
| `../2026-09-16_p2-2p8-n0409-profile/arm9-profile.csv` | Per-PC melonDS ARM9 profile, same roster, 129 regions (one region = one presented frame), 6,048,201 PC rows |
| `builds/build-p2p8-n0409-profile/smash64ds-p2-fourcpu-tickhud-hwtri.elf` | The profiled ELF; symbols and source files via `arm-none-eabi-nm -l -S --defined-only` |

Units: timer ticks at 33.513982 MHz (1 tick = 2 ARM9 cycles). Profile ticks are
`cycles / 2 / regions`. The two inputs are different runs of the same roster; the
tick-HUD rows are the gate instrument, the profile is attribution only.

## 1. Bucket bands (`bands.py` -> `bands_dtcm.txt`)

| bucket | mean | P50 | P95 | P99 |
|---|---:|---:|---:|---:|
| ALL (wall, VBlank-quantized) | 1,998,178 | 1,678,016 | 2,798,208 | 3,358,336 |
| WORK-H | 1,667,445 | 1,586,816 | 2,302,656 | 2,917,824 |
| SRC | 636,962 | 567,104 | 1,075,904 | 1,459,200 |
| FTR | 380,975 | 353,024 | 750,336 | 875,968 |
| STG | 326,161 | 325,056 | 332,864 | 335,040 |
| MISC | 281,817 | 267,328 | 490,240 | 606,720 |

VBlanks per presented frame: 3: 832, 4: 560, 5: 118, 6: 19, 7: 5, 8: 1.
**No frame presented in two VBlanks. Mean presentation rate 16.77 FPS.**

Band means of the frames ranked by WORK-H:

| band | WORK-H | SRC | SINT | SCPU | SPHD | SHDT | SPRM | SCAT | FTR | STG | MISC | AUD | HUD |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| P40-60 | 1,584,246 | 578,533 | 287,281 | 77,849 | 119,626 | 30,319 | 4,951 | 2,338 | 344,446 | 325,803 | 295,918 | 12,355 | 68,396 |
| P90-95 | 2,191,934 | 941,972 | 482,255 | 74,671 | 170,259 | 109,573 | 23,284 | 5,353 | 559,933 | 327,385 | 310,987 | 24,522 | 80,582 |
| P95-99 | 2,496,513 | 1,126,525 | 473,442 | 68,677 | 179,650 | 218,830 | 78,296 | 12,554 | 626,995 | 327,334 | 357,574 | 31,152 | 91,908 |
| P99+ | 3,235,572 | 1,670,808 | 442,632 | 65,852 | 217,440 | 416,080 | 310,720 | 95,748 | 785,268 | 327,400 | 404,236 | 20,996 | 25,976 |

Reading:

- Render (FTR + STG + MISC) is **966K of the 1.58M median frame (61%)** and 1.52M
  of the P99+ frame. STG is a flat per-frame constant; FTR and MISC grow in the tail.
- The SRC tail is **event work**: from median to P99+, hit detection (SHDT) grows
  +386K, params/status change (SPRM) +306K, catch (SCAT) +93K, physics (SPHD) +98K.
  The CPU AI (SCPU) does not grow.
- HUD spikes ~450K every 9-10 frames and AUD ~120K every ~13 frames (periodicity
  measured in the rows). HUD is the tick-HUD's own console refresh
  (`src/port/taskman_seam_battle_host.c:1016` sums foreground + profile HUD ticks;
  the refresh "runs about twice a second"). AUD is the BGM refill cadence.
  Removing both spikes moves WORK-H P95 only 2.30M -> 2.24M: they are not what
  holds the gate, but the instrument must not stay inside a P99 gate.

## 2. Mechanism attribution (`attrib.py` -> `classes.txt`, `attrib_n0409.json`)

Self time grouped by owning mechanism (function name + source file). Helper
classes (soft-float, mem*, integer division) are reported as themselves, not
charged to callers. Average ticks per presented frame; total non-idle work
1,616,422.

| class | tk/fr | share | CPI |
|---|---:|---:|---:|
| RENDER_STAGE | 213,429 | 13.2% | 3.00 |
| RENDER_COMMON (matrix kernels, adapters) | 210,182 | 13.0% | 2.35 |
| RENDER_FIGHTER | 202,667 | 12.5% | 4.45 |
| SOFTFLOAT | 150,372 | 9.3% | 1.22 |
| MAP_COLLISION | 97,128 | 6.0% | 3.05 |
| FT_LOGIC | 96,908 | 6.0% | 5.83 |
| POSE (`nds_ft_pose.c`) | 78,374 | 4.8% | 2.46 |
| OTHER | 74,859 | 4.6% | 2.86 |
| MEM (memset/memcpy/...) | 59,970 | 3.7% | 3.51 |
| ANIM (objanim, anim keys, MObj) | 47,309 | 2.9% | 3.03 |
| CAMERA | 46,410 | 2.9% | 4.27 |
| RENDER_EFFECT | 44,483 | 2.8% | 4.25 |
| SYSLIB (FatFs, libnds GL wrappers, mutex) | 40,958 | 2.5% | 2.87 |
| INSTRUMENT (debug HUD, tick reads) | 37,231 | 2.3% | 1.99 |
| OBJMAN | 33,308 | 2.1% | 6.52 |
| RELOC (runtime asset resolution) | 29,223 | 1.8% | 2.97 |
| INTHELPER | 29,104 | 1.8% | 2.06 |
| FT_AI | 28,931 | 1.8% | 6.10 |
| PARTICLE | 22,779 | 1.4% | 5.78 |
| HUD_LOGIC | 22,424 | 1.4% | 4.74 |
| HIT_COLLISION (self only) | 11,380 | 0.7% | — |

The classifier is heuristic (see `classify()`); treat class boundaries as +-10%.

## 3. What the P99 frames add (`tail.py` -> `tail_symbols.txt`)

Top 13 profile regions by work (mean 2.17M) against the median band (1.55M),
per symbol. The +616K is, in order: the debug HUD and printf/console (~110K,
instrument); fighter packet **production** on state change (~75K:
`ndsFighterPacketCmd`, `ndsRendererExecuteNativeFighterOwnerProduction`,
`ndsRendererNativePrepareProductionRun`, `ndsRendererNativeApplyStateDelta`, ...);
soft-float (+60K); motion **bind/parse/resolve** (~66K: `ndsFtPoseParse`,
`ndsRelocP2FighterAnimAssetIDForToken`, `ndsRelocNativeAssetAddress`,
`ndsAObjEvent32ForgetRange`, `ndsFtPoseBindEntry`, `ndsPreviewFileOffset`,
`battleship_ftMainSetStatus`); **storage I/O** (~29K+ `get_fat`, `f_lseek`,
plus copies); SFX start (`ndsAudioFgmPlayAtPan` +7.8K).

## 4. Working set (`footprint.py` -> `footprint.txt`)

Per presented frame the ARM9 executes a median **675 distinct functions (max
882) whose code totals 284,182 bytes (max 352,234)**, touching ~47,600 distinct
instruction addresses (~114 KB of executed code). The ARM9 has an 8 KB
instruction cache and a 32 KB ITCM that is already full. At most 32 KB of the
~114 KB can come from ITCM; the other ~80 KB stream from main RAM through the
8 KB I-cache every frame (an uncached Thumb fetch averages 4.5 bus cycles per
opcode, hardware reference section 9.2). That, before any data traffic, is the
structural reason 64.3% of the frame is memory stall
(`../2026-09-16_p2-2p8-stall-budget/`).

## 5. Investigations (read-only, 2026-09-22)

Three read-only investigations answered the design questions behind
`docs/p2/FOUR_FIGHTER_30FPS_ARCHITECTURE.md`. Every claim carries a file:line and
a MEASURED/ESTIMATE label; the figures the architecture relies on were re-checked
against the tree before inclusion.

| file | question |
|---|---|
| `INVESTIGATION_RENDER.md` | where fighter/stage/MISC render time goes; special cases; joints and cross-part corners; stage run classes and clipping; compiled-list sizes; retired RAM; GE/DMA risks; first slice |
| `INVESTIGATION_RESIDENCY.md` | motion acquisition path (681 reads, 0 cache hits); motion/SFX/BGM bytes; RAM map lab vs shipping; overlays; extent-map reads; ARM7 audio; first slice |
| `INVESTIGATION_SIM.md` | hit-detection mechanism (7,377 tk per hurtbox joint); status-change path; motion events; map collision; joint consumers; camera readers; CPU AI; port-only machinery; first slice |

## Reproduce

```
arm-none-eabi-nm -l -S --defined-only <elf> > nm_l.txt
python attrib.py nm_l.txt <arm9-profile.csv> attrib_n0409.json
python tail.py nm_l.txt <arm9-profile.csv>
python footprint.py nm_l.txt <arm9-profile.csv>
python bands.py <fourcpu-rows.csv> 440
```

`tail.py` and `footprint.py` import the symbol loader and classifier from
`attrib.py` in the same folder.
