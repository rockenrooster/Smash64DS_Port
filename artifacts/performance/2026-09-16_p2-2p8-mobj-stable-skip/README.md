# P2-2p8 N04.08 material-animation stable-zero skip — census and same-ROM A/B

Three records, in order: the census that sized the candidate, the same-ROM route
A/B that attributed it, and the hard-on implementation with its focused four-CPU
verification and Boundary result. **This is not an acceptance record.** P2-2p8
remains RED against the locked-30 product target; nothing here clears it.

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

---

# Hard-on implementation

The census and the route word are gone. `gcPlayAnimAll` now calls
`ndsGcPlayAnimAllStableSkip` unconditionally, and
`gNdsMObjMatAnimStableSkipCount` stays as permanent engagement proof, read by
`verify-p2-four-fighter-stress.ps1` on every future four-CPU run.
`nds_build_config.h` hashes **identical to the N04.05 Boundary config**
(`23314B62...5415F3`), so this is the shipping configuration with no lab flag.

Two changes beyond deleting the scaffolding:

1. **NaN agreement.** The skip predicate is now "`anim_speed` is `+/-0` **and**
   `anim_wait` is positive, finite and non-zero", tested as
   `(bits(anim_wait) - 1u) < 0x7f7fffff`. The route arm used `NDS_FCMP_GT0`,
   which orders a positive NaN above zero where the decomp parser's
   `anim_wait > 0.0F` does not — on a NaN wait the parser would fall through and
   consume an event while the skip suppressed the player. That is the one input
   that can make the skip disagree with the parser's early return, so it is
   excluded. `+Inf` goes with it; refusing to skip is always safe. All three
   AObj sentinels are strictly negative (`AOBJ_ANIM_NULL` is `F32_MIN`,
   `CHANGED` `F32_MIN/2`, `END` `F32_MIN/3`), so the predicate also excludes
   every sentinel edge and the `END -> NULL` transition can never be skipped.
2. **80 ITCM bytes returned.** `ndsBaseGcPlayAnimAll` lost its last call site and
   the linker does not drop it, so its `.itcm` attribute is gone and the dead
   decomp body sits in main RAM. ITCM is `0x7f78` of `0x7fe0` — **104 bytes
   free**, against 120 at the N04.05 baseline and 16 in the measured route
   build. `gcPlayAnimAll` is `0x210` versus `0x1b0` at baseline, the traversal
   inlined into it.

## Safety

A read-only audit of the skip against the decomp player, the parser and every
other writer of the fields involved returned **SAFE WITH CONDITION**. The three
conditions hold today and two are now recorded in the code at the skip itself:

1. `gcPlayAnimAll`'s unconditional `ndsAObjEvent32CorrectMObjColors(mobj, FALSE)`
   pass must keep running. It and the player are the only writers of the five
   colour tracks in the tree, and it re-derives all five from the live AObj
   chain, so it is what makes colour safe — not the skip's own reasoning.
2. The ten scalar material tracks (13..22) have no restore pass. They are safe
   while no non-player writer touches a track that also carries a live matanim
   AObj on the same MObj. The gameplay setters that write
   `texture_id_curr/next` and `palette_id` target MObjs with no matanim on that
   track, and the renderer's `MOBJ_FLAG_FRAC` path is idempotent under the skip
   because `lfrac` keeps the value the last player call wrote.
3. The NaN case, closed above.

Also established by the audit: `gcParseMObjMatAnimJoint` returns before
collecting `mat_aobjs[]`/`matspecial_aobjs[]` and before its event loop, so the
chain the skipped player would have seen is byte-identical to the previous
call's; `gcGetTreeDObjNext` is a verbatim extraction of the decomp's inline
child/sib_next/parent walk; and `was_stable_zero` is captured and consumed
within one loop iteration, with nothing but the parser in between.

## Engagement is stage-local, and that is the point

MObjs start at `anim_speed = 1.0f` and only an explicit speed setter can zero
one. The single MObj-targeted zero-speed write in the tree is Dream Land's
frozen water (`src/import/battleship_grpupupu_ground.c`), whose freeze
fingerprint bit-pins `texture_id_curr`, `texture_id_next`, `lfrac`, `trau`,
`trav`, `scrollu`, `scrollv` and `palette_id` before it zeroes the speed — the
same fields the skipped player would rewrite. `gcSetAnimSpeed` touches DObjs
only, so fighter freezes never produce a stable-zero MObj.

So this removes the per-frame replay of an animation the port already froze on
purpose, and the census's uniform `7,892 x (1 Linear + 3 Cubic + 2 Step)` is
four water MObjs of six nodes each, every presented frame.

**On a stage with no frozen material animation the skip fires zero times and
costs one predicate.** The gate's movement is Dream Land's; it is not a
whole-stage or whole-roster win and must not be banked as one.

## Focused four-CPU verification

`builds/verify-p2p8-mobj-stable-skip-stress.log`, 1,972 samples, frames
2..1973, DLDI ON, git `0f640202b64+dirty(25)`. Lab identity
`builds/build-p2p8-mobj-stable-skip/`:

- ROM SHA-256 `143165BD18556708C251F153FAE45DD5C98EA565DF79A324238E5FC4A90F9126`
- ELF SHA-256 `C98DB88BEE85132289FAAAF47D3E8E3D14164EF93FB77211C549EC0C60AC5357`
- config SHA-256 `23314B62FBBECCAC413A9D677555EB1233137F84C26D5D9DF0177F801F5415F3`

| Bucket | P50 | P95 | mean |
|---|---|---|---|
| ALL | 1,677,952 | 2,798,144 | 1,967,609 |
| WORK-H | 1,587,328 | 2,323,584 | 1,636,445 |
| WORK | 1,635,712 | 2,432,640 | 1,700,183 |
| SRC | 553,920 | 1,061,184 | 608,415 |
| GCRA | 548,096 | 1,055,360 | 602,669 |
| FTR | 356,608 | 743,552 | 368,295 |
| STG | 344,320 | 387,648 | 348,655 |

`named` 1,671,801 (85.0% of ALL). VBlank 2/3/4/5+ **128/949/703/193**, max
interval **12**, slips **0**. One timer-overflow correction (frame 1694),
landing back on the run median.

**`gNdsMObjMatAnimStableSkipCount` = 7,892** — equal to the census total and to
the route candidate arm, from a third independent build.

Native failures/direct rejects **0/0**. Fighter draw-plan **618 / 6,217 / 0**.
Fighter kinds `151,389,187`, draw-slot triangle mask `15`, P0/P1 hardware
triangles 349,031 / 333,618 — all identical to the N04.05 checkpoint run.
Heap low-water **111,680 B**; arena 1,355,520 B; libc high-water 32,928 B,
top-chunk min 10,280 B against the 40,960 B reserve; weapon pool 10, high-water
2, refusals 0.

### The lab directory build was INCOMPLETE — do not quote the numbers above

Those figures come from a private lab directory, `build-p2p8-mobj-stable-skip`,
created by invoking `make TARGET=... BUILD=...` directly. **That ROM is not the
same game.** Its NitroFS holds **365 files / 28,320,664 B** against the canonical
`build-p2-fourcpu-tickhud`'s **710 files / 29,325,885 B** — 345 files and
1,005,221 bytes missing:

| Missing | Files | Bytes |
|---|---|---|
| `reloc/reloc_animations/` (Kirby) | 328 | 730,512 |
| `fighters/preview/*.fpc` | 12 | 222,708 |
| `fighters/{yoshi,pikachu}_{high,low}.bin` | 4 | 47,940 |
| `fighters/shield_pose/09.bin` | 1 | 4,061 |

The cause is the opposite of "the fresh build is broken". `NITRO_FILES :=
$(NITROFS_DIR)` hands the whole build directory to the packer, so a build ships
every file present in its `nitrofs/`, not the set its own flags produce. The
Makefile records this for audio (`NDS_AUDIO_OBSOLETE_DERIVED_FILES`: "superseded
BGM assets can survive an incremental build-directory reuse and are otherwise
silently repacked by ndstool") and prunes exactly two categories,
`prune-obsolete-audio` and `prune-streamed-ftanim`. Nothing prunes fighter
images, CSS previews, shield poses or reloc animations.

So the 345 extra files are **stale leftovers in the long-lived canonical
directory**, dated 2026-09-11 and 2026-09-13, from builds with different flags;
only 3 files there were written on 2026-09-16. The fresh directory holds what the
current configuration actually produces. Nothing failed in either run — native
failures stayed 0/0 and the engagement counter read the right 7,892.

Three independent confirmations:

- **Two separate fresh builds of this target agree to the byte** — the N04.08
  candidate directory and the N04.09 profile directory both produced exactly
  **365 files / 28,320,664 B**. A partial build would not reproduce a byte-exact
  payload twice.
- `fighters/preview/` is gated behind `NDS_P2_1P_GAME` / `NDS_P2_MENU_SHELL`
  (`Makefile:7668`). The four-CPU stress target boots straight into VSBattle and
  has no menu shell, so a build of it *should* omit those 12 FPCs; their presence
  in `build-p2-fourcpu-tickhud` is a leftover, not a requirement.
- The stale files' own timestamps: 2026-09-11 and 2026-09-13, against 3 files
  written on the day of the comparison.

`builds/build`, the directory that produces the published `smash64ds.nds`, holds
**955 files / 53.6 MB with the oldest dated 2026-08-01** and 4 written that day,
so the published artifact is subject to the same accumulation.

The whole apparent gain was in **STG**: 344,320 against 398,848 at P50, 387,648
against 442,048 at P95, mean 348,655 against 402,273, a flat ~54,500 ticks of
stage work per frame that moved the `ALL` median across a VBlank quantum (3
intervals instead of 4). `GCRA` and `FTR` matched between the two builds to
within 1,500 ticks, which is the tell: the lever landed identically, and
everything else moved.

**Earlier revisions of this file read that difference as an arena-carve lever
and called it a candidate larger than every N04.0x change. That was wrong and is
retracted.** The arena and heap figures do differ (1,355,520 vs 1,347,328 B;
111,680 vs 108,096 B) but they track the payload difference, not a lever.

**What is NOT established is that the stale payload causes the STG difference.**
A file nobody opens costs ROM size and FAT chain length, not stage time. The
honest claim is that these two ROMs are not comparable, not that staleness costs
54,500 ticks a frame. Separating the two needs a clean rebuild of the canonical
directory, which is a deliberate re-baseline, not a side effect of a candidate.

The census and route builds were also private directories, which is why their
absolute levels sit away from the checkpoint's. It does not touch the same-ROM
A/B: both of its arms were one binary in one directory, so the comparison is
internally valid regardless of what that binary was missing.

**Check before trusting any cross-build figure:** the candidate's NitroFS file
count and total size must match the baseline's.

## Qualification — canonical directory

Comparison against the N04.05 checkpoint, both in `build-p2-fourcpu-tickhud`,
both through `verify-p2-four-fighter-stress.ps1`, 1,972 samples:

| Bucket | N04.05 | N04.08 | delta |
|---|---|---|---|
| WORK-H P50 | 1,649,728 | 1,639,808 | **-9,920** |
| WORK-H P95 | 2,373,632 | 2,365,120 | **-8,512** |
| WORK-H mean | 1,696,317 | 1,690,473 | -5,844 |
| FTR P50 | 357,248 | 357,312 | +64 |
| FTR P95 | 743,616 | 744,640 | +1,024 |
| FTR mean | 368,779 | 368,806 | +27 |
| Heap low-water | 108,096 B | 108,096 B | 0 |

N04.08 canonical run: ALL 2,237,504 / 2,798,208, SRC 552,192 / 1,055,360, GCRA
546,688 / 1,047,168, STG 398,848 / 442,048, WORK 1,693,504 / 2,491,840, named
1,727,747 (85.6%). VBlank 2/3/4/5+ **104/845/809/215**, max interval **13**,
slips **0**. Native failures/direct rejects **0/0**. Arena 1,347,328 B, alloc
failures 85 — identical to the checkpoint.
**`gNdsMObjMatAnimStableSkipCount` = 7,892**, a fourth independent build
reproducing the census total exactly.

**The two methods agree.** The same-ROM route attributed **-8,640** WORK-H P50 to
the skip; the canonical cross-build re-bank reads **-9,920**. A ~1,300-tick gap
between a placement-free attribution and a re-bank is far tighter than this
campaign usually sees, and both move the same direction at P50, P95 and mean.

-9,920 is below the documented **14,080-tick** cross-build significance floor,
and on its own that would not carry a KEEP — it is what sank N04.07. The
difference is that N04.07 had a sub-floor movement, a **P95 regression** and no
attribution, while N04.08 has a sub-floor movement, a **P95 improvement** and a
same-ROM attribution that independently predicts it. Route to attribute, re-bank
to bank: here the two agree, so the work removal is real and the banked movement
is honest.

## Boundary

`builds/verify-p2p8-mobj-stable-skip-boundary.log`, full profile, `-Build`:

- `p2_shell_loop` — **passed**: 1 lap, 10 scene entries, deterministic-menu
  high-waters flat, free floor **114,628 B**, zero faults. Identical to N04.05.
- `p2_battle_realtime` — **passed**: 212 frames, presentation **26.3 FPS**
  (`present=263 x0.1 fps`) against N04.05's 25.7 and N04.07's 25.9. The
  locked-30 warning still stands; it is a published metric, not a pass.
  `water=2/0/1` confirms Dream Land's water objects are live in this arm.
- `p2_fourcpu_stress` — **passed**, the canonical-directory run tabled above.

`Boundary verification profile passed.`

## Verdict

**KEEP.** The skip removes 7,892 material-player calls and 47,352 AObj node
evaluations per 1,972 gameplay frames, attributed at **-8,640** WORK-H P50
same-ROM and re-banked at **-9,920 / -8,512** P50/P95 in the canonical
directory, with heap, native failures, direct rejects, draw-plan and slips all
unchanged, and Boundary GREEN.

**P2-2p8 remains RED.** WORK-H P50 1,639,808 against a target near 1.12m is not
moved by this, and the win is Dream Land's, not the roster's or the stage set's.

One thing this run banks beyond the candidate:

**A fresh `BUILD=` directory produces an incomplete ROM, and an incomplete ROM
measures faster.** `make TARGET=... BUILD=<new dir>` here shipped 365 of 710
NitroFS files, and the resulting 54,500 ticks/frame of missing stage work read
as a 52,480-tick WORK-H P50 "win" that survived long enough to be written down
as an arena-carve candidate. Match the candidate's NitroFS file count and size
against the baseline's before quoting any cross-build delta, and prefer building
into the baseline's own directory.
