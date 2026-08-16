# The attach lane does not convert: its 72,768 "ceiling" is worth 13,376–37,027 at rank-80, its parse/evaluate half is the transition's own second animation play, and the only pure-engineering item in it is 3,542 — under every floor this repo has

**Date:** 2026-08-16 · **Branch:** `codex/r2-runtime2` · **base HEAD `a186ee6a570`**
**0 builds, 0 emulator runs, 0 production source edits, 0 defaults flipped, 0 ROMs published,
both root ROMs byte-unchanged.** Every number below is re-derived from artifacts already in the
tree plus BattleShip source.
**UNITS: 1 project tick = 1 `cpuGetTiming()` tick = 2 ARM9 cycles.** Every table states its window.

```text
REQUIREMENT  +65,297 net ticks per presented frame at rank-80.  BASIS build-c220-camship,
             rank-80 1,210,624 raw / 1,185,677 net against the 1,120,380 gate (apparatus
             24,947), band 41-120 1,218,356; shipping renderer (GX_COMPOSE 0), bore 0,
             mode 163 one-minute match, NDS_R2_BOTH_CPU=1, 1,600 samples, frames 439-2038,
             slips=0.  REPRODUCED here: convert.py re-derives 1,210,624 / 1,185,677 /
             +65,297 from ../2026-08-16_camera-ship/ship220-rows.csv before doing anything
             else.  UNMOVED -- this cycle banks nothing.

CORRECTED    THE 72,768 IS NOT 1.11x THE REQUIREMENT.  It is the re-rank of clipping every
             one of the 288 event frames' SITR back to the run median -- a per-frame
             VARIABLE amount that reaches 227,968 on the cluster.  A uniform per-frame
             saving, which is what an engineering change produces, converts at 0.42-0.86.
             Sizing the named mechanism itself (parse 28,094 + evaluate 26,813 + attach
             23,801 = 78,708 tk/fr, all three MEANS over the 288):
               capped at each frame's own SITR excess   rank-80 -13,376   level +51,921
               uncapped (strictly optimistic)           rank-80 -59,520   level  +5,777
             Neither is -7,471.  Section 1.

ANSWERED     THE PARSE/EVALUATE WORK IS NOT REDUNDANT ACROSS JOINTS.  Per joint it is flat
             or cheaper on an event frame -- ndsR2AnimValueQ 0.98x/call, gcPlayDObjAnimJoint
             0.91x/call, ftParamUpdateAnimKeys 0.98x/call.  There is no clip header parsed
             14 times and no memo to take.  What runs 1.52x is the WHOLE animation play,
             because ftMainSetStatus calls one itself (decomp ftmain.c:4787-4795).  That is
             100% of the evaluate group and ~49% of the parse group.  The other ~51% of
             parse is that a fresh AOBJ_ANIM_CHANGED attach costs 1.62x per call because it
             must consume the new clip's first event block instead of early-outing.  Both
             halves ARE the transition.  Sections 2 and 3.

BLOCKED      (decision: transition-frame animation play).  Suppressing the extra play is
             worth D = 49,251 tk/fr on the 288 = rank-80 -13,376 (capped) to -37,027
             (uncapped), level +51,921 to +28,270.  It is the largest item in the lane and
             the only one that clears the >=14,080 cross-build floor.  It is a gameplay
             change: ftMainSetStatus plays the animation so the new status's pose and
             collision are correct ON the transition frame, and ftMainRunUpdateColAnim runs
             immediately after it.  Not proposed, not built.  Section 4.

REFUSED      THE TOKEN RESOLVER IS NOT A SMALL CERTAIN WIN, IT IS A SMALL CUT ITS OWN FILE
             ALREADY FORBIDS.  +4,118 tk/fr on the 288 = 3,542 at rank-80, level +61,755.
             reloc_backend_assets.c:1876-1921 records two measured failures on this exact
             function and states the bar: "Do not bring another small load-frame cut.
             Either remove this work in one change large enough to clear ~16,000 of tail
             movement, or move it off the gameplay frame entirely."  3,542 is not 16,000
             and it is under the >=14,080 rank-80 cross-build floor.  Section 5.

INHERITED    30 OF THE TOP-80 FRAMES CARRY NEITHER AN ATTACH NOR A FORCE-LOAD, and SHDT
             owns 22 of those 30 at 47.66x its run median.  Across the whole top-80 SHDT
             owns 32 frames and SITR 25.  The attach lane cannot reach 37.5% of the rank.
             Section 6.
```

---

## 1. The ceiling does not convert

`convert.py` re-ranks the basis's own 1,600 `WORK-H` rows with a **fixed** subtraction `D`
applied to the 288 attach/force-load frames, and reads the 80th value. Control first: with
`D = 0` it returns rank-80 **1,210,624**, net **1,185,677**, level **+65,297** — the published
basis, so the harness is the same one `SITR_EXCURSION.md` §1 used.

Two models bracket any real change. **Capped** limits the saving on each frame to that frame's
own `SITR`-above-median excess (what the diagnosis's re-rank did). **Uncapped** lets the full
`D` come off every event frame — strictly more optimistic than any real fix, because the group
sizes are *means* and a frame with one attach cannot save what a frame with three does.

| candidate (its §4.4 size, tk/fr on the 288) | D | capped: moved / level | uncapped: moved / level |
|---|---:|---:|---:|
| `ndsRelocAssetIDForToken` | 4,118 | 3,542 / **+61,755** | 3,542 / +61,755 |
| + `AObjToQConvert` None store | 6,749 | 3,840 / +61,457 | 4,224 / +61,073 |
| ATTACH chain group | 23,801 | 10,496 / +54,801 | 11,968 / +53,329 |
| ANIM evaluate group | 26,813 | 10,496 / +54,801 | 14,845 / +50,452 |
| ANIM parse group | 28,094 | 11,134 / +54,163 | 16,126 / +49,171 |
| parse + evaluate | 54,907 | 13,376 / +51,921 | 42,683 / +22,614 |
| parse + evaluate + attach | 78,708 | **13,376 / +51,921** | **59,520 / +5,777** |
| *(diagnosis's ceiling: clip all SITR excess)* | var. | 72,768 / −7,471 | — |

**The gap between the last two rows is the whole correction.** The diagnosis's 72,768 comes from
removing a per-frame variable that reaches 227,968 on the cluster; the mechanism it named is
78,708 as a *mean*. The two are not the same quantity, and only the mean is what an
implementation delivers.

The marginal rate is not monotone, which matters when sizing a candidate:

```text
D=1,000   moved   424  ratio 0.424        D=20,000  moved  8,704  ratio 0.435
D=2,000   moved 1,424  ratio 0.712        D=30,000  moved 11,968  ratio 0.399
D=4,118   moved 3,542  ratio 0.860        D=54,907  moved 13,376  ratio 0.244
D=6,749   moved 3,840  ratio 0.569        D=78,708  moved 13,376  ratio 0.170   (capped)
```

Small savings convert *best* (0.86 at 4,118) because they bite exactly on the frames sitting
just above rank-80; large ones saturate as the event population sinks below the 1,312 frames
the lever cannot touch. **Size every future candidate on this curve before building it** —
`convert.py` needs no emulator and runs in under a second.

Two guards on the population itself: 288 of 1,600 frames carry an event, but only **50 of them
sit at or above rank-80**; and if all 288 were made free, rank-80 would be **1,069,376**
(level −75,951), so the population is not itself the limit — the conversion rate is.

---

## 2. What the excursion is made of, per call

Entry-PC call rates and self time from `../2026-08-16_sitr-excursion/attribution-event288.txt`
(`build-c221-sitrprof`, the shipping configuration, window frames 439–2038, event288 vs the
derived `rest` = the other 1,313 frames). **tk/call is self time divided by that function's own
entry-PC count, both from the same capture.**

| symbol | tk/fr ev | tk/fr rest | calls ev | calls rest | **tk/call ev** | **tk/call rest** | per-call ratio |
|---|---:|---:|---:|---:|---:|---:|---:|
| `ftParamUpdateAnimKeys` | 14,500 | 9,696 | 5.65 | 3.71 | 2,566 | 2,613 | **0.98×** |
| `ndsR2AnimValueQ` | 25,066 | 16,024 | 386.15 | 242.05 | 64.9 | 66.2 | **0.98×** |
| `gcPlayDObjAnimJoint` | 26,611 | 18,641 | 100.91 | 64.04 | 263.7 | 291.1 | **0.91×** |
| `ndsR2FtAnimParseDObjFigatree` | 28,853 | 11,262 | 100.83 | 63.69 | 286.2 | 176.8 | **1.62×** |
| `ndsR2AnimAObjToQConvert` | 2,970 | 340 | 208.99 | 16.48 | 14.2 | 20.6 | 0.69× |
| `ndsRelocAssetIDForToken` | 4,118 | 0 | 2.61 | 0.00 | 1,578 | — | — |

**Three of the four large animation functions are flat or cheaper per call.** Nothing in the
evaluate half costs more per joint on a transition frame; `gcPlayDObjAnimJoint` costs 9% *less*,
because a freshly attached chain is still full of `nGCAnimKindNone` nodes its loop skips. So the
evaluate group's +26,813 is **entirely a call-count rise** — there is no per-joint price to
attack and no value to memoize.

Only `ndsR2FtAnimParseDObjFigatree` is more expensive per call, at 1.62×. Its own counters say
why: over the 25-frame cluster `gNdsR2FtAnimParseStepped` runs **2.27×** its run median while
`gNdsR2FtAnimParseEarlyOut` runs **1.02×** (`SITR_EXCURSION.md` §2). The stepped fraction goes
from 20.8% to 35.8%. A parse that early-outs is ~9 instructions; a stepped one walks the script.

---

## 3. Why the play count rises — the BattleShip source says it outright

`decomp/BattleShip-main/decomp/src/ft/ftmain.c:4787-4795`, inside `ftMainSetStatus`:

```c
        if (frame_begin != 0.0F)
        {
            ftMainPlayAnimEventsForward(fighter_gobj);
        }
        else
        {
            ftMainPlayAnimEventsAll(fighter_gobj);
            ftMainRunUpdateColAnim(fighter_gobj);
        }
```

Both arms reach `ftMainPlayAnim` (`ftmain.c:933`), which calls `ftParamUpdateAnimKeys`
(`ftparam.c:364`) — the loop over every `fp->joints[]` that runs `gcParseDObjAnimJoint` +
`gcPlayDObjAnimJoint` per joint. **The status transition plays the animation itself, and the
fighter's ordinary frame update plays it again.**

The census agrees quantitatively, on an independent instrument:

```text
battleship_ftMainSetStatus    1.22 /fr on the 288,  0.00 elsewhere
ftParamUpdateAnimKeys         5.65 /fr on the 288,  3.71 elsewhere   ->  +1.94
                              => 1.59 extra whole animation plays per status transition
```

Decomposing the parse growth against that (`rest` per-play rate 63.69/3.71 = 17.166 parses
per play; event 100.83/5.65 = 17.846):

```text
extra plays at the rest per-play rate   5.65 x 17.166 - 63.69 = +33.30  of +37.14   89.7%
more joints walked per play             5.65 x  0.68          =  +3.84  of +37.14   10.3%
```

and multiplicatively against parse's own +17,591 of self time: **~37% call count, ~40% per-call
cost, ~23% interaction** — attributing the interaction proportionally, roughly half and half.

> **The answer to "redundant across joints, or genuinely per-joint?" is neither.** It is
> redundant across *plays*, and the second play is not a repeat — it runs on a clip that was
> attached between the two, at a different animation frame. A memo keyed on anything the joint
> loop sees would miss on every one of them.

### 3.1 The one genuinely redundant store, and why it is not worth taking

`ndsR2AnimAObjToQConvert`'s `nGCAnimKindNone` arm (`battleship_ftanim.c:362-369`) writes
`length_invert = Q(1.0)` and returns **without changing `a->kind`**, so `ndsR2AnimAObjToQ`'s
`kind >= NDS_R2_AQ_KIND_BASE` early-out never catches it and the same constant is re-stored on
every `ndsR2AnimBuildTrackTable` for as long as the AObj stays `None`. Since the decomp's
`gcAddDObjAnimJoint` (`objanim.c:137-149`) sets **every** AObj in the chain to `None`, an attach
arms ~209 of these per event frame — matching the measured 208.99 calls/frame exactly.

It is real, it is provably idempotent, and it is **worth 298 ticks at rank-80** (the 4,118 →
6,749 row of §1: 3,542 → 3,840). Recorded, not built. Hoisting the `None` arm into the inline
wrapper would be the fix if it ever rides along with something large.

---

## 4. `BLOCKED(decision: transition-frame animation play)`

**What it is.** Suppress or defer the `ftMainPlayAnimEventsAll` / `ftMainPlayAnimEventsForward`
call that `ftMainSetStatus` makes, so a status transition costs one animation play instead of
two.

**What it is worth.** D = 0.897 × (28,094 + 26,813) = **49,251 tk/fr on the 288 event frames**:

| model | rank-80 | moved | level |
|---|---:|---:|---:|
| capped at each frame's SITR excess | 1,197,248 | 13,376 | **+51,921** |
| uncapped | 1,173,597 | **37,027** | **+28,270** |

It is the only item in this lane whose uncapped size clears the ≥14,080 rank-80 cross-build
floor, and it does not close the gate on its own.

**Why it is the owner's call and not an engineering choice.** The play is not bookkeeping. It
establishes the new status's pose on the transition frame, and `ftMainRunUpdateColAnim` runs on
the very next line against those joints. Deferring it means one presented frame of the old pose
and, more seriously, one logic tick of collision computed from stale joint transforms — a
gameplay-fidelity trade under `PROJECT_GOAL.md`'s sacrifice order, not a representation change.

**Options, priced, not recommended:**

1. **Defer the play to the ordinary frame update** (drop it entirely). Full 49,251. Changes the
   transition frame's pose and the collision derived from it.
2. **Keep the play, drop only the parse half** — reuse the previous play's parsed AObj state
   for joints whose figatree entry did not change across the transition. Bounded by parse's
   share, ~28,094 × 0.897 ≈ 25,200; needs a per-joint "did this entry change" test that is
   cheaper than the parse it skips, which §2 says is 286 tk.
3. **Do nothing here** and spend the cycle on the 30 top-80 frames this lane cannot reach (§6).

The same escalation covers the attach chain itself (+23,801, capped 10,496 / uncapped 11,968):
whether `gcAddDObjAnimJoint`'s 22.06 per-joint attaches may be spread across frames is the same
question about the same transition, and `SITR_EXCURSION.md` §5.3 already declined to propose it.

---

## 5. The token resolver, and why it is refused rather than shipped

`ndsRelocAssetIDForToken` (`src/port/reloc_backend_assets.c:1922-2054`) is ~110 `if`s against
link-time symbol addresses — `ndsRelocFileID` is `return (u32)(uintptr_t)file_id`, so GCC cannot
build a switch and emits a literal-pool load per compare — followed on a miss by two pointer
scans over 143 + 158 ids. It costs **1,578 tk/call, 2.61 calls/frame on the 288, +4,118 tk/fr**,
and **2,023 of that 4,118 is icache** (`attribution-event288.txt`'s own column).

**It converts to 3,542 at rank-80, level +65,297 → +61,755.** That is 5.4% of the requirement.

Its own file already answers this, at `reloc_backend_assets.c:1876-1921`, from two measured
attempts:

- **Task 74** memoized it. `SRC`, the bucket it targets, rose 1,920 at P95 and `WORK-H` rose
  11,584 at P50, while `STG` — which a token lookup cannot touch — moved 8,128.
- **R2-06 E11** hoisted `ndsRelocIsMarioFoxAnimID` above both scans and deleted the five dead
  compares it subsumes: provably identical, **negative bytes added**, function down 7,667 to
  31,808, load-frame set bit-identical. Against a matched control it still lost — `WORK-H` P95
  **+15,744**, P99 +59,200, over-gate 9 → 11. Control-to-control noise on this ROM is P95
  ±5,376 and ±1 over-gate.

and states the bar verbatim:

> *"Do not bring another small load-frame cut. Either remove this work in one change large
> enough to clear ~16,000 of tail movement, or move it off the gameplay frame entirely, which
> changes WHEN the work happens instead of shuffling where the code sits."*

3,542 is not 16,000, and it is under the **≥14,080 rank-80 cross-build floor**. Shipping it
would be the third instance of the same mistake. **My brief asked for it as item 1; the file's
rule is the repo rule and it wins.**

A table is also not free here. The keys are link-time addresses, so any lookup must be built at
runtime: ~411 entries is ~1.5 KB of `.data`/`.rodata` in main RAM, against
`[[ram-is-not-free-gobj-cap]]`'s heap low-water of 24,404 versus the 25,600 GObj-cap threshold.
E11's own conclusion — *"three lookup arrays in `.main.bss` are not [resident]"* — is the same
objection measured.

**What would satisfy the file's second clause** (recorded for whoever prices it, not started):
the tokens resolved during a match are a bounded, static set — one fighter's status table maps
status → figatree token, and both fighters are known at match start. Resolving that table once
during load and storing the asset ids beside the statuses removes the call from the gameplay
frame entirely instead of making it faster. That is a load-time change, not a lookup change,
and it is what "move it off the gameplay frame" means here.

---

## 6. What this lane cannot reach

`outside.py`, same basis, splits the top-80 by whether the frame carries an attach or a
force-load:

| | n | `WORK-H` median | dominant leaf |
|---|---:|---:|---|
| in the lane | 50 | 1,344,448 | `SITR` 21, `SHDT` 10, `SPRM` 8, `SPHD` 8, `MISC` 2, `AUD` 1 |
| **outside it** | **30** | 1,302,304 | **`SHDT` 22**, `SITR` 4, `MISC` 2, `AUD` 2 |

The 30 outside frames read `SHDT` **219,616** against a run median of 4,608 — **47.66×** — and
`SCPU` 106,944 against 53,696 (1.99×). Across the whole top-80, `SHDT` owns **32** frames and
`SITR` **25**. Of the 118 frames over the gate, **41 carry neither an attach nor a force-load**.

This is consistent with, not a contradiction of, `SITR_EXCURSION.md` §1: its own re-rank table
already put the `SHDT` cluster (32 frames) at **50,240 moved, level +15,057** — larger than the
`SITR` cluster's 45,056. `f5e13aa3e27` closed a hit-detection lever at +2,543 on a per-frame
basis; the *clustered* 32-frame `SHDT` excursion is a different quantity and, on this basis, the
single largest one in the table. It is not re-opened here and it is not sized beyond what §1
of the diagnosis already published.

---

## 7. What this cycle did NOT do

- **No production source was edited, no default flipped, no ROM built, no ROM published.** No
  emulator was launched. The only new files are this document and the two analysis scripts.
- **The token resolver was not rewritten** (§5) and **the `AObjToQConvert` None store was not
  hoisted** (§3.1). Both are sized and refused on the floor, not forgotten.
- **The transition-frame play was not changed, and no option in §4 is recommended.**
- **The `SHDT` lane was not opened**, only pointed at with the diagnosis's own numbers.
- **Nothing was re-banked.** The requirement is +65,297 on `build-c220-camship` before and after.
- **`decomp/` untouched**; both root ROMs byte-unchanged and not rebuilt (§8).
- **`build-c205-camtoggle` was not rebuilt.**
- The `2^22` sampler artifact needs no re-check here: no run was taken. The basis rows carry the
  two live corrections (frames 1464 and 1849) that `pf220-run.log` records, and neither is an
  event frame or a top-80 frame.

---

## 8. Verification and hashes

```text
root ROMs, unchanged and not rebuilt this cycle:
  smash64ds.nds                        54c07fac80c50418949908701f7c2bdbf27512c5f96ac09086fabbb0df6ac68a
  smash64ds-battle-playable-hwtri.nds  6c939434c53c9b3a76ff016540b810a84f207b1a4e24540b8653b15717367c99
```

**Boundary green, exit 0, `0 Exception:`** — `boundary.trimmed.log` (verdict lines only; the
raw log was 316,172 lines and is not committed). `DECOMP_PRISTINE=PASS`
(`pinned_historical_files=10 ds_markers=0 decomp_patch_pipeline=absent`), GBI decode fixtures,
particle bank pack, harness registry, sprite/collision/ftanim exactness gates, Task 9 float ITCM
(`itcm=30164/32768 free=2604`), renderer ITCM placement (`itcm=30164 renderer=12896`),
`battle_playable_realtime`.

**Its realtime pacing smoke is the control that this cycle changed no production code, and it
is exact.** Against the last five Boundary runs on this tree (`c209b` / `c217` / `c219` / `c220`
/ `c221`):

| | previous | this cycle | Δ |
|---|---|---|---|
| `binds` / `vtx` / `tri` | `54 / 2484 / 828` | `54 / 2484 / 828` | **identical** |
| `ftrTri` | `132712/p067840/p164872/own424` | `132712/p067840/p164872/own424` | **identical** |
| `frames` / `fps` / `rprof` | `212 / 241/480 / 0` | `212 / 241/480 / 0` | identical |
| `ticks` | 294,353,408 | **294,353,408** | **0** |
| proof-ROM `.itcm` | 30,164 / `renderer=12896` | 30,164 / `renderer=12896` | identical |

Boundary links its own `smash64ds-battle-playable-proof-hwtri`, so a tick count identical **to
the tick** across separately-invoked builds is the strongest available statement that the
production tree is byte-unchanged. Neither root ROM was rebuilt and neither hash moved.

## 9. Reproduction

From the repo root, needing **no emulator and no build**:

```powershell
python artifacts\performance\2026-08-16_sitr-attach-lane\convert.py    # sections 1, 3.1, 4, 5
python artifacts\performance\2026-08-16_sitr-attach-lane\outside.py    # section 6
```

Both read only `../2026-08-16_camera-ship/ship220-rows.csv` and the two counter-run CSVs from
`../2026-08-16_sitr-excursion/`, and both print the control (`rank-80 1,210,624 / +65,297`)
before any result.
