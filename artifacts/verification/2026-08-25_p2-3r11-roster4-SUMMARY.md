# P2-3r11 — four DISTINCT fighter kinds on the stress arm, 2026-08-25

**READ EVERY TICK FIGURE HERE AS PACK-OFF.** This arm is built with
`NDS_R2_BATTLEPACK=0` and `NDS_R2_ANIM_CACHE_ARENA_BYTES = 229,376`, because the
four distinct kinds took the difference (board row P2-3r11). It is therefore
**not comparable** to any four-CPU figure banked on the Mario/Fox mirror roster,
all of which were measured with Fox's clip pack resident. The memory artifact
now carries `fighterRoster` and `battlePackResident` for exactly this reason.

- Target `smash64ds-p2-fourcpu-tickhud-hwtri`, build `build-p2-fourcpu-roster4`
  (`NDS_P2_FOUR_CPU_ROSTER=1`), ROM SHA-256
  `A70EBD3368C2DDC4E8B503B57C1893EB084C3741ADE8E18805338609AAC904CE`.
- Mario / Fox / Luigi / Donkey Kong, all four level-3 CPUs, Dream Land,
  one-minute Time, items off.

## The run completed and the identity is the configuration under test

Frames 1..1973, guest clock 60 -> 1 (59/60 s = 98.33%), **0 humans, 4 CPUs,
4 fighter GObjs, active mask `0xF`**, 0 pacing slips, `syMallocOverflowCount 0`,
`objmanPanicCount 0`.

## Timing — over the gate, and stated as such

| bucket | P50 | P95 |
|---|---|---|
| ALL | 1,964,992 | 3,085,760 |
| WORK | 1,672,960 | 2,927,680 |
| WORK-H | 1,530,752 | 2,708,480 |
| FTR | 1,572,928 | 2,514,112 |
| SRC | 545,728 | 1,187,776 |

VBlank intervals `2:63 3:65 4:17 5+:1828`, max 21, total 1,973. Against
`PROJECT_GOAL.md`'s P95 <= 1,120,000 and >=95% two-VBlank cadence this
configuration is a long way from the gate. That is the honest first measurement
of the gate's own stated content set, not a regression in anything previously
accepted — nothing had ever run it.

## Both heap figures on this row are the SAME ROM

`verify-p2-four-fighter-stress.ps1` hardcodes
`$target = 'smash64ds-p2-fourcpu-tickhud-hwtri'`, and that is the only four-CPU
target the Makefile defines — **there is no uninstrumented twin of this arm.**
The 25,208 B figure (`…-packoff-only.txt` / `…-packoff-only-stress.log`) and the
57,316 B figure below are the same target and the same build directory at two
source revisions. `gNdsTaskmanArenaChosenSize` is **1,511,424 in both**, which is
the proof: the arena is granted by stepping down 4,096 at a time until the heap
allows it, so two binaries of different size land on different arenas, and these
did not. The only difference is `NDS_R2_ANIM_CACHE_ARENA_BYTES`, 262,144 versus
229,376 — a 32,768 B constant that moved the low-water by 32,108 B.

Because the instrument is the only build, **57,316 B is the instrument's own
headroom, and no less-instrumented four-kind build's margin is known.** Past the
floor this allocator halts rather than degrades, so re-read
`generalHeapFreeMinBytes` before trusting any future result from this arm.

## Memory — the point of the row

`generalHeapFreeMinBytes` **57,316** against the 25,600 B floor (**31,716 B
margin**), arena 1,511,424 with `AllocFailCount` 9. Predicted 57,976 before the
run from the 25,208 B pack-off-only measurement plus the 32,768 B cache trim;
measured 57,316, i.e. 660 B of layout noise. DObj high-water 193, effects 12/38,
particles 35/112 + 14/24 + 17/80, 0 rejects, AObj normalize/hash failures 0.

## The price, measured

`gNdsR2AnimCacheMisses` 216, **`gNdsR2AnimCacheRejects` 212**. The animation
arena is refusing entries, which the reservation's own acceptance test
(`Rejects == 0`) calls the failure condition.

**Half of the attribution came free from the same Boundary run.** The MIRROR
arm — untouched by this row, `BATTLEPACK=1`, 163,840 B cache — reports
`animCacheRejects` **18** (misses 31). The reservation was *already* refusing
before anything here changed, so rejects are not something this row introduced,
and asserting `Rejects == 0` would have been wrong — which is why the gate
reports it instead of failing on it.

| | mirror roster | four distinct kinds |
|---|---|---|
| `battlePackResident` | true | false |
| arena chosen | 1,548,288 | 1,511,424 |
| general-heap low-water | 31,252 | **57,316** |
| margin above floor | 5,652 | **31,716** |
| `ftPoseBindFull` | 0 | 0 |
| `animCacheMisses` | 31 | 216 |
| `animCacheRejects` | 18 | **212** |

Note the four-kind arm now ends with *more* headroom than the standing gate arm
(31,716 vs 5,652), so if the 212 rejects prove expensive there is room to give
some of the 32,768 back — sized against measurement, on P2-3r13.

**What is still unseparated and must not be assumed:** four distinct kinds have roughly twice the
animation working set of two kinds mirrored (the warm list covers only Mario and
Fox; Luigi and Donkey have no warm entries at all), so a reservation sized for
two kinds was always going to refuse on this roster -- the 32,768 B trim
deepened that, it did not create it. Splitting the two needs an arm at 262,144
on this roster, which is board row P2-3r13's question and deliberately not
answered here. Every reject degrades to the on-demand load, so this is a
performance outcome and never a correctness one.

`gNdsFtPoseBinds` 751 with **`gNdsFtPoseBindFull` 0**: the four-slot pose pool is
exactly filled and never refuses on this arm, so board row P2-3r8's
character-select leak does not reach it -- this target boots straight into the
source VSBattle path. The gate now asserts that.

## Artifacts

- `2026-08-25_p2-3r11-roster4-tickhud.json` / `.csv` — buckets and rows.
  **Local only, deliberately:** `/artifacts/` is gitignored and the bulk
  bucket dumps are rotatable telemetry by the same convention that leaves
  `p2-2-fourcpu-tickhud.json` untracked. The figures quoted above are
  reproduced in this file and on board row P2-3r11 precisely so the
  citation survives the rotation.
- `2026-08-25_p2-3r11-roster4-coverage.json` — window/identity
- `2026-08-25_p2-3r11-roster4-memory.json` — memory + flags the run was measured under
- `2026-08-25_p2-3r11-roster4-packoff-only.txt` and
  `2026-08-25_p2-3r11-roster4-packoff-only-stress.log` — the intermediate arm
  that ran the match but ended 392 B under the floor, which is why the cache
  trim exists. Renamed off the accepted run's name prefix on purpose: a
  superseded run must not share an identity with the run that superseded it.
- `2026-08-25_r11-trace-create.txt`, `2026-08-25_r11-trace-gap.txt`,
  `2026-08-25_r11-overflow-halt.txt`, `2026-08-25_r11-arena-overflow.txt`,
  `2026-08-25_r11-trace-fixed.txt`, `2026-08-25_r11-setup-pools.txt` — the
  root-cause chain and the ruled-out alternatives
