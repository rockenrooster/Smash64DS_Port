# 508 bytes of DTCM buys 43,200 ticks — the largest banked win of the campaign

112 scattered scalar statics moved into DTCM by linker script alone. No source
file changed. **WORK-H P50 −43,200, P95 −43,072**, which is 3.1× the 14,080
cross-build significance floor and 9.2% of the 468,544 gap.

Sizing and provenance: `…2026-09-17_p2-2p8-locality-sizing/`.

## The measurement

Same target, same build directory, same roster, 1,972 samples each. Control is
the 2026-09-17 `hats-off` arm, which reproduces the banked baseline to the byte.

| bucket P50 | control | DTCM arm | delta |
|---|---:|---:|---:|
| **WORK-H** | 1,580,544 | **1,537,344** | **−43,200** |
| WORK-H P95 | 2,320,768 | 2,277,696 | −43,072 |
| ALL | 1,678,016 | 1,677,888 | −128 |
| STG | 337,472 | 324,992 | −12,480 |
| SRC | 556,160 | 543,936 | −12,224 |
| GCRA | 550,528 | 538,240 | −12,288 |
| MISC | 249,856 | 238,144 | −11,712 |
| SINT | 260,416 | 254,272 | −6,144 |
| FTR | 354,432 | 349,184 | −5,248 |
| OTHR | 283,968 | 280,896 | −3,072 |
| SPHD | 118,016 | 115,648 | −2,368 |

Frames over the 1,120,000 gate: 92.5% → **91.6%**. ALL is unchanged because it
is VBlank-quantised; the win is real work removed inside the same quantum.

## The gate: correctness clean, one bookkeeping assertion

**Three** independent runs of this binary agree: WORK-H P50 **1,537,344**,
**1,536,960**, **1,537,344**. The measurement is reproducible, not a single
draw.

Correctness and the divergence witnesses are clean and identical to the control:

| witness | control | DTCM arm |
|---|---:|---:|
| `gNdsRendererNativeFailure.count` | 0 | **0** |
| `gNdsFighterDLAllDrawP0HardwareTriangleCount` | 349,031 | **349,031** |
| `gNdsFighterDLAllDrawP1HardwareTriangleCount` | 333,618 | **333,618** |
| `gNdsFighterDLAllDrawSlotTriangleMask` | 15 | **15** |
| `gNdsLabPoseJointCapEvaluated` | 258,836 | **258,836** |
| direct rejects | 0 | **0** |

Same workload, same geometry, nothing declining. The run nevertheless exits
non-zero, on a **sample-window** assertion at
`scripts/verify-p2-four-fighter-stress.ps1:410`:

```
Four-CPU timing rows are not the requested inclusive window:
artifact=3..1973, coverage=1..1973, requested timing=2..1973
```

The binary reliably produces **five ring-stop seams** where two samples share one
presented-frame label — the harness's own warning says these are "distinct real
iterations that share a frame LABEL" and that "percentiles are over the
iterations and stand". Both runs report the **same five seam rows** (222, 510,
990, 1374, 1950), so this is deterministic for this binary rather than noise, and
re-running does not clear it. The effect is that the collector's first labelled
frame is 3 rather than 2, which the window assertion refuses.

**Matching the window does not fix it, and that is the finding.** Re-running with
`-StartFrame 3 -Samples 1971` produced `artifact=4..1973` against
`requested=3..1973` — the collector's first labelled frame is always
`requested + 1`. The seam rows moved by exactly one with the window (221, 509,
989, 1373, 1949), so the seams sit at fixed absolute frames and it is the *first
sample* that is systematically relabelled. Chasing `StartFrame` is a treadmill;
three runs were spent establishing that.

This is a plausible second-order consequence of the arm rather than a defect:
frames are cheaper, so the guest presents frames at different offsets relative
to the ring drain and labels collide. It is **not** a correctness failure and it
does not touch the percentiles — the harness says so itself. But the assertion
at `:404-410` requires `sample.startFrame == StartFrame` exactly, and **no
choice of window satisfies it on this binary**. Accepting this lane therefore
needs either a harness change (an owner call, since that assertion exists to
stop timing and identity coming from different matches) or a diagnosis of why
the first sample is relabelled. Recorded rather than worked around.

### Diagnosed: the guest is identical, only the first row's LABEL moves

The sampler labels rows by counting **backward** from the presented-frame counter
read at each ring stop (`sample-tick-hud-buckets.ps1:1044-1052`): the first row of
stop *k* is `frame − delta + 1`. `startFrame` in the artifact is then simply
`$frames[0]`, the first row's label (`:1447`). So `startFrame` is a *derived*
quantity, not the window that was requested — and the sampler's own comment says
it deliberately refuses to assert the skew that feeds it is zero.

Comparing the two artifacts' ring reads directly:

| | control | DTCM arm |
|---|---|---|
| ring stops | 21 | 21 |
| stop presented frames | 128, 224, … 1952, 1973 | **identical** |
| `gNdsBattlePlayablePacingLogicFrames` at each stop | 256, 448, … 3904, 3946 | **identical, all 21** |
| `ringStartRead.frame` | 1 | 1 |
| last stop frame | 1973 | 1973 |
| stop 0 `fromFrame` | 2 | 3 |
| first row label (`startFrame`) | 2 | **4** |
| label span vs rows | 1,972 = 1,972 | 1,970 vs **1,971** |

**The guest ran the same match.** Every logic-frame counter matches at every one
of the 21 stops, and the stops fall on the same presented frames. Three of the
four conditions in the `:405-410` assertion pass in every run — `endFrame`,
coverage start and coverage end all match. Only `startFrame == StartFrame` fails,
and it fails by exactly one, because the stitcher's first row carries
`fromFrame + 1`.

So the property that assertion exists to protect — that timing and identity come
from one match — is **demonstrably intact**, and is independently witnessed by
the correctness table above (native 0, identical triangles, identical
pose-cap evaluations).

**What this costs and what it needs.** The assertion conflates "the window I
requested" with "the first label the stitcher produced". A minimal fix is to
keep the three conditions that pin the match and compare the first label against
the recorded label span rather than against `StartFrame` — the excess-rows count
is already derivable from the artifact (`samples − (endFrame − startFrame + 1)`,
0 for the control and 1 here). That is a change to a gate assertion, so it is an
owner call, not something to take unilaterally; but it is now a one-line decision
with the cause pinned rather than an open question.

## The confound, and why it does not explain this

Moving 508 bytes out of `.main.bss` re-phases `.bss`, and
`…_p2-2p8-placement-hazard/` measured swings of **±45,760** from layout alone
with no code change. **−43,200 sits inside that band**, so the magnitude by
itself proves nothing. The shape does.

The placement hazard has a signature, and it is STG-dominant — in both of its
arms the STG swing **exceeds the whole WORK-H swing**:

| placement arm | WORK-H | STG | STG as share of WORK-H |
|---|---:|---:|---:|
| C − A (unpinned) | +42,240 | +44,096 | **104%** |
| E − D (pinned) | +32,320 | +45,760 | **142%** |

This arm does not look like that:

| bucket | share of the WORK-H delta |
|---|---:|
| STG | **28.9%** |
| SRC | 28.3% |
| MISC | 27.1% |
| SINT | 14.2% |
| FTR | 12.1% |
| OTHR | 7.1% |
| SPHD | 5.5% |

The win is spread across every subsystem, which is what removing scattered
cache-line pressure should look like — these statics are read by the stage
walk, the source update, the fighter draw and the interrupt path alike. A
placement re-phasing concentrates in STG and does not.

Three further arguments, none sufficient alone:

1. **The prediction was registered before the build.** The sizing said 32,438
   recoverable for this set after exclusions. Measured 43,200. A placement swing
   is sign-random; this one matched a signed, pre-stated prediction and beat it.
2. **P50 and P95 moved by nearly the same absolute amount** (−43,200 / −43,072),
   a uniform shift rather than a tail effect.
3. `ALL` did not move, so this is not a cadence artifact.

**What would settle it** is a per-PC re-profile: rebuild with
`NDS_TASK37_PROFILE`, re-run `run-task37-profile-census.ps1`, and read
`cyc/x` on the moved symbols' own load PCs. Those must collapse toward the
load-use interlock floor. That check is immune to placement because it looks at
the instructions themselves rather than the frame total. **Owed.**

## What was moved, and what was deliberately not

112 symbols, **508 bytes** — 40 into the loaded `.dtcm` (10 initialised
statics) and 468 into NOLOAD `.dtcm.bss` (102 zero-initialised). DTCM ends at
`0x02ff2a34` against the `0x02ff3000` assert ceiling, leaving **1,484 bytes
free** of the 1,992 that were available.

The `.data`/`.bss` split is load-bearing: a `.data` static placed in NOLOAD
`.dtcm.bss` links cleanly and **silently loses its initialiser**. The two lists
are separate blocks in the script for that reason.

**Excluded on purpose:**

- `gNdsCameraMatrixLeanEnabled` and `gNdsR2CameraFixedEnabled` are declared
  `__attribute__((section(".data")))` at `src/import/battleship_gmcamera.c:90`
  and `:843`, deliberately, so both arms of a route A/B link identically. They
  land in a plain `.data` that occurs six times across the link, so a gather
  would sweep in unrelated data, and moving them would destroy the same-ROM A/B
  property the comments were written to protect. Cost of excluding: −311 tk/fr.
- `s_highTickCount` and `tickRef` are archive members
  (`libcalico_ds9.a(tick.o)`, `libnds9.a(timers.o)`) belonging to
  `tickGetCount`, which `…_gate-decision/` classifies as **instrument inside
  WORK-H**. Moving them would have credited this arm with 2,754 tk/fr of
  measurement cost that the shipping config does not pay. `delay`
  (`libnds9.a(keys.o)`) excluded with them.

Excluding the instrument is why the honest prediction was 32,438 rather than
34,444, and the arm beat the honest number.

### The initialisers were checked, not assumed

Residency does not prove initialisation. Dumping `.dtcm` in the linked ELF and
comparing each moved `.data` symbol's bytes against the same symbol in the
pre-move ELF:

| symbol | B | was | now | |
|---|---:|---|---|---|
| `sNdsRendererRuntimeOwner` | 1 | `09` | `09` | identical |
| `gNdsRendererFastRunMode` | 4 | `09000000` | `09000000` | identical |
| `gNdsWhispyAOTRoute` | 4 | `07000000` | `07000000` | identical |
| `dLBParticleCurrentTransformID` | 1 | `7b` | `7b` | identical |
| `gMPCollisionYakumonoDObjs` | 4 | `0224be48` | `0224bd70` | pointer moved |
| `sNdsFighterDisplayReplayEvents` | 4 | `022339b0` | `02233900` | pointer moved |
| `sNdsNativeFighterActiveTables` | 4 | `0211e864` | `0211e850` | pointer moved |
| `sNdsNativeFighterActiveOwner` | 4 | `0211e7b4` | `0211e7a0` | pointer moved |
| `gNdsFtrDrawMemoStubRoot` | 4 | `022320fc` | `02232050` | pointer moved |
| `sNdsNativeStagePacketActive` | 4 | `021282c8` | `021282b8` | pointer moved |

The four scalars are byte-identical, which is the case a dropped initialiser
would have shown as zero. The six pointers changed value because removing 508
bytes from `.main` shifted what they point at — and each still resolves to the
**same symbol**: `sNdsMPCollisionYakumonoDObjs`, `sNdsFighterDisplayContract`,
`sNdsNativeFighterHighTables`, `sNdsNativeMarioHighOwner`,
`sNdsFtrDrawMemoStub`, `sNdsNativeStagePacketDreamLand`. Nothing was lost.

## The gather is proven, not assumed

`scripts/check-dtcm-residency.py` parses the intended set out of the linker
script's own `DTCM-RESIDENT HOT SCALARS: BEGIN/END` blocks and asserts every
named symbol resolved inside `[0x02ff0000, 0x02ff3000)` in the linked ELF:

```
  DTCM residency: 112 of 112 intended symbols inside [0x02ff0000, 0x02ff3000)
  span 0x02ff2220..0x02ff2a30
DTCM_RESIDENCY_OK
```

This is not ceremony. A linker input-section pattern that matches nothing is not
an error — it gathers zero bytes, the link succeeds, and a dead lever reads
exactly like a lever that does not work. Each entry names one section for that
reason; wildcards are rejected by the checker. All 112 section names were also
confirmed against `objdump -t` of the build's own object files before the block
was written, and none is ambiguous across translation units.

## Why the closed DTCM lane was closed on the wrong candidate

`…_p2-2p8-dtcm-falsifier/` concluded "can DTCM hold anything that pays? **no**",
from a break-even of ~13 refetches per resident line per frame. That number is
`2,256 fills / 178 lines`, and **178 is 5,704 bytes ÷ 32** — the right
conversion for the contiguous table it tested, and the wrong one for a 4-byte
scalar that occupies a whole line by itself.

| candidate | DTCM bytes | lines | tk/fr | per DTCM byte |
|---|---:|---:|---:|---:|
| `sNdsFtPartsFlat`, 4 slots (reverted) | 1,584 | 49 | 10,176 | 6.4 |
| **hot scalars (this arm)** | **508** | **~112** | **43,200** | **85.0** |

**13.2× better per byte.** The falsifier's measurement stands and its refusal was
right: −10,176 was under the floor and cost 80% of the budget. What was wrong was
the generalisation.

## Status

`IMPLEMENTED_NOT_ACCEPTED`. Owed: the four-CPU gate's correctness/memory verdict
and divergence witnesses on this exact binary, and the per-PC re-profile that
separates lever from placement beyond the shape argument above.
