# P2-2p8 N04.08 material-animation stable-zero skip — census and same-ROM A/B

Phase: MEASURE. This directory records the observation and the paired same-ROM
route comparison only. It is not an acceptance record: P2-2p8 remains RED, and a
hard-on implementation still owes focused four-CPU verification plus full
Boundary before it can be claimed as a checkpoint.

## Why this candidate

The float campaign's largest unsolved call-elimination candidate is
`ndsBaseGcPlayMObjMatAnim`: 7,923 soft-float helper calls per rank-80 frame in
the shipping-rebank measurement, and the largest single soft-float caller in the
build. Its ITCM wrapper already sits beside it, so the lever is not placement —
it is not entering the player at all.

The condition tested is *stable zero*: `anim_speed` is `+/-0` and `anim_wait` is
positive **before** `gcParseMObjMatAnimJoint` runs. In that state the parser
returns before consuming an event, and the material player then adds `+/-0` to
every AObj length and recomputes byte-identical outputs.

## Census (observation only, no behaviour change)

Build flag `NDS_R2_MOBJ_ANIM_CENSUS=1`, four-CPU stress, 1,972 samples
(`builds/verify-p2p8-mobj-census-stress.log`):

| Counter | Value |
|---|---|
| Active MObj calls via `gcPlayAnimAll` | 12,106 |
| Direct `gcPlayMObjMatAnim` calls | 246,680 |
| …of those, active | 1,953 |
| Total active calls | 14,059 |
| **Stable-zero calls** | **7,892 (56.1% of active)** |
| `anim_speed == 1.0` calls | 6,167 |
| Other speeds | 0 |
| Live non-None AObj nodes | 60,263 |
| **Stable-zero nodes** | **47,352 (78.6%)** |
| Stable-zero kinds (Linear / Cubic / Step / other) | 7,892 / 23,676 / 15,784 / 0 |
| `nGCAnimKindNone` nodes | 31 |
| Longest AObj chain | 7 |
| `AOBJ_ANIM_END` calls | 12 |

The stable-zero kind split is exactly `7,892 x (1 Linear + 3 Cubic + 2 Step)`:
six nodes per call, four calls per presented frame, with no variance. Native
failures/direct rejects 0/0; heap low-water 111,680 B.

Two thirds of the *nodes* the player walks each frame belong to objects whose
animation cannot have moved since the previous call.

## Same-ROM A/B

One ROM, one ELF, both arms. `gNdsR2MObjStableSkipRoute` is a `.data` word
(32-byte aligned; a whole-word poke, never a byte poke) read on the hot path.
Route 0 is the control and route 1 skips only the player call. **Both arms
execute the same explicit copy of the decomp `gcPlayAnimAll` traversal**, so
code layout, I-cache placement and the route test itself cancel; the only
difference between the arms is whether `ndsBaseGcPlayMObjMatAnim` is entered for
a stable-zero MObj. The parser runs unchanged in both. Direct
`gcPlayMObjMatAnim` callers are excluded because the proof depends on the
parser->player ordering that only `gcPlayAnimAll` owns.

Lab identity — `builds/build-p2p8-mobj-skip-route/`,
`NDS_R2_MOBJ_STABLE_SKIP_ROUTE=1`, `NDS_R2_MOBJ_ANIM_CENSUS=0`:

- ROM `smash64ds-p2-fourcpu-tickhud-hwtri.nds` SHA-256
  `4E0880C005197127ECC4C4DAF9BBD319005C880C38EC167CA1D4A7E9F0E0BCFD`
- ELF SHA-256
  `1C9637DA4BDE9E3ED71159C9BDAED609783EE2C42923FA42DC03F6F2DAAE7345`
- `nds_build_config.h` SHA-256
  `A515169DA5BD196CFFF324AD4ECBB829C770C4F0595E89B046BB3CAA10D6F538`

Each arm is 1,972 samples, frames 2..1973, DLDI ON, melonDS
`sha=DE80E46BDCF1FD98`. Control `builds/verify-p2p8-mobj-skip-control.log`,
candidate `builds/verify-p2p8-mobj-skip-candidate.log`.

### The arms are different runs

`gNdsR2MObjStableSkipCount` reads **4** in the control and **7,892** in the
candidate. The candidate figure equals the independent census's stable-zero call
count exactly, from a separate build, so the lever is engaged and the workload
is deterministic across builds. The control's 4 are the increments taken before
the route word is poked to zero at the start of the run.

### Result (candidate minus control, ticks)

| Bucket | Control P50 | Candidate P50 | dP50 | Control P95 | Candidate P95 | dP95 | dMean |
|---|---|---|---|---|---|---|---|
| ALL | 1,677,952 | 1,677,952 | **0** | 2,798,144 | 2,798,144 | **0** | -7,669 |
| WORK-H | 1,595,392 | 1,586,752 | **-8,640** | 2,322,176 | 2,320,576 | **-1,600** | -5,792 |
| WORK | 1,642,752 | 1,636,032 | -6,720 | 2,426,240 | 2,428,544 | +2,304 | -6,021 |
| SRC | 561,664 | 556,224 | **-5,440** | 1,068,544 | 1,060,480 | **-8,064** | -5,795 |
| GCRA | 556,096 | 550,656 | **-5,440** | 1,062,976 | 1,054,912 | **-8,064** | -5,800 |
| OTHR | 285,184 | 283,520 | -1,664 | 562,752 | 562,752 | 0 | -1,648 |
| FTR | 356,864 | 356,672 | -192 | 743,872 | 745,600 | +1,728 | +7 |
| STG | 341,376 | 341,568 | +192 | 385,024 | 384,384 | -640 | -5 |
| SINT | 259,136 | 260,160 | +1,024 | 596,608 | 594,944 | -1,664 | -45 |

`named` falls 1,676,969 -> 1,670,948 (-6,021).

The saving lands where the work was removed: the whole GCRA P50 reduction is
also the whole SRC P50 reduction, and GCRA is 33% of ALL P50. `ALL` P50 and P95
are unchanged because they are VBlank-quantized; the cadence histogram moves in
the candidate's favour without crossing a quantum — control VBI 2:128 3:933
4:716 5+:196, candidate 2:129 3:951 4:704 5+:189, max interval 13 and slips 0 in
both.

FTR P95 +1,728 and WORK P95 +2,304 are recorded as costs, not reinterpreted as
wins. They are not where the change acts and they sit inside this arm's
run-to-run spread; WORK-H, the harness-excluded work figure the campaign gates
on, falls at P50, P95 and mean together.

### Invariants held identical in both arms

Native failures **0**, direct rejects **0**, general-heap low-water
**111,680 B**, fighter draw-plan build/hit/mismatch **618 / 6,217 / 0**, fighter
kinds word `151,389,187`, draw-slot triangle mask `15`, slips **0**.

The control arm's `cpuGetTiming()` 2^22 overflow artifact was corrected on 3 of
1,972 samples (frames 454, 704, 1749), each landing back on the run median; the
candidate arm needed no correction.

## Verdict

The lever engages, engages deterministically, and removes real work in the
bucket that contains it. **-8,640 WORK-H P50** is 0.54% of a WORK-H P50 that
must fall from 1,586,752 to the product target; it is a keep-sized step, not a
solution to P2-2p8, which remains RED.

Next: strip the census and the route word, make the skip unconditional, and
qualify the hard-on build through focused four-CPU verification and full
Boundary. Engagement must remain readable in the qualifying run — a skip that
silently stops firing is indistinguishable from one that fires and saves
nothing.
