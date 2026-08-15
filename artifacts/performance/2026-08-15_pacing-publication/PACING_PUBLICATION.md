# The publication seam: the counter reads current, and the wrong form becomes inexpressible

**Date:** 2026-08-15 · **Branch:** `codex/r2-runtime2` · **base HEAD `450470c716a`**
**Native Battle Kernel slice 1 — fix the publication seam, get arm G green, re-measure the gate.**
Predecessor: `…/2026-08-15_battlepack-arena-price/ARENA_PRICE.md`.

---

## 0. Outcome first

```text
ARM G       GREEN.  Boundary passes at NDS_R2_BATTLEPACK=1 +
            NDS_R2_BATTLEPACK_KEEP_CACHE=1, arena 1,548,288, on the binary
            carrying the fix.  0 `Exception:` in the log.  The blocker that
            has stood since the arena cycle is gone.

THE PROOF   AND IT IS NOT THE RED-TO-GREEN FLIP, BECAUSE A NO-SEAM CONTROL
            BUILT THIS CYCLE ALSO PASSES.  The evidence is a QUANTITY, and it
            separates perfectly over six runs and four configurations:

              WITH the seam     presented = 212, 212, 212, 212   (4 arms)
              WITHOUT it        presented = 211, 211             (2 arms)
              prev cycle, RED   presented = 212, draws = 211     (TORN)

            The harness reads gNdsBattlePlayablePacingPresentedFrames at the
            frame-complete marker.  Unpublished it reads ONE BEHIND -- which
            is legal, because every member is one behind TOGETHER -- until the
            arm that moved the allocator let PresentedFrames reach RAM while
            DrawCalls stayed dirty, and the group read TORN.  Published, all
            four arms read the CURRENT value.  A stale read is always behind,
            never ahead: that is the whole prediction, and it is what the
            counter does.

THE FIX     NOT a third bespoke DC_FlushRange.  Each debugger-read counter
            group is now ONE X-macro list beside its externs, the publish is
            GENERATED from that list, and check-gbi-decode-fixtures.ps1 pins
            each list against the marker printf the harness actually reads.
            Adding a field to BPLAY_PACE without publishing it now fails
            Boundary instead of surfacing six days later as a red tuple.
            FALSIFIER RUN: dropping one member turns the checker RED and it
            names the member.

THE SHAPE   Publishing BPLAY_PACE ALONE WOULD HAVE BEEN A HALF FIX.  The
            harness also derives taskmanPresentLead = GCRUNALL_TASKMAN[1] -
            2*BPLAY_PACE[4] and requires 0..2.  At the frame-complete marker
            that difference RESTS AT EXACTLY 0 -- no low-side slack -- so
            pinning one side coherent and leaving the other free to read
            stale converts `drawLead=-1` into `taskmanPresentLead=-1`.  Both
            groups go through the seam.  This was found by the blast-radius
            sweep, not by a failure.

GATE        RE-MEASURED ON THE BINARY THAT WOULD SHIP, and it is unchanged.
            rank-80 WORK-H 1,177,920 raw / 1,152,973 net against the
            1,120,380 gate -- NET GAP +32,593.  The banked no-seam arm was
            1,170,048 / 1,145,101, i.e. +7,872 raw, BELOW the >=14,080
            cross-build floor, so this is "unchanged", not "a cost".  VBI
            2:1745 3:272 4:13 5+:8 max 19, slips 0, violations 0.  Same fight:
            end-of-match damage 0/76 on this arm, on c168 and on the flag-0
            control alike.  STRESS BATTERY on the same ROM: 660 s, NO-FREEZE,
            10 entries / 8 matches / 8 restarts / 2 SUDDEN DEATHS, every
            allocator gate 0, heap low-water 52,472.

TASK B      THE LEAN=3 ROW DOES NOT CLOSE THE WAY THE BRIEF EXPECTED, AND
            THE MEASUREMENT IS THE REASON.  Boundary at
            NDS_R2_CAMERA_MATRIX_LEAN=3 PASSES ON THIS HEAD WITH THE SEAM
            REMOVED -- control run, seam absent from the ELF by `nm`.  The
            2026-08-09 `phaseLag=-1` reproduction is DEAD; it cannot be
            credited to this fix because it no longer reproduces without it.
            The seam therefore cannot be shown to fix that row.  What the
            row's either/or IS answered by is the drawLead evidence -- but
            that symptom is dormant on this tree too (see THE PROOF).  Both
            rows are now defended by construction rather than by luck.
```

---

## 1. Where the seam belongs, and why one line later would have been wrong

The realtime harness's last stop before it reads every battle marker is
`tbreak ndsBattlePlayableFrameCompleteMarker`
(`verify-battle-mariofox-gcrunall-loop-harness.ps1:1953`, `:1976`), and GDB
breaks on a function's **entry**. So a publish *inside* the marker executes
after the stop has already read memory. The seam is the statement before the
call, in `ndsBattlePlayableFinalizePresentedIteration`
(`src/port/taskman_seam.c:5250`).

That is also the only point in the iteration where every gated difference is at
its resting value:

```text
                      at the marker       harness rule
logicLag  = 2*presented - logic   0       0 or 2
drawLead  = draw - presented      0       0 or 1
phaseLag  = presented - phaseSum  0       0 or 1
taskmanPresentLead = tm - 2*draw  0       0..2      <- no low-side slack
```

**`armDCacheFlush` is clean-AND-invalidate, then drain** (`c7,c14,1` then
`armDrainWriteBuffer`; disassembled from the linked ELF, not assumed). So after
the publish the group's lines are not resident, the next `x++` linefills and
then hits, and RAM holds the published tuple until something evicts the line —
which is what makes the observable set collapse onto the boundary row.

## 2. The structural form — and why a bespoke flush was refused

`CLAUDE.OPUS.md`'s meta-rule: *a documented lesson gets broken twice; a
structural one cannot be.* This is the **third** diagnosis of one defect
(R2-04 E2's FPS-HUD group, 2026-08-09's `phaseLag=-1`, 2026-08-15's
`drawLead=-1`), so a third hand-written `DC_FlushRange` block was the wrong
answer.

| piece | where | what it makes impossible |
|---|---|---|
| `NDS_PUBLISH_DEBUGGER_GROUP` + `_MEMBER` | `include/nds/nds_platform.h` | a group published by hand, or as a span that depends on linker adjacency |
| `NDS_BATTLE_PLAYABLE_PACING_GROUP(X)` (14) | `include/nds/nds_startup.h`, beside the externs | a member declared but not published — the list *is* the flush |
| `NDS_GCRUNALL_TASKMAN_GROUP(X)` (6) | same | the same, for the other side of `taskmanPresentLead` |
| `NDS_BATTLE_FPS_HUD_GROUP(X)` (4) | `nds_platform.h` | the 2026-08-15 bespoke block, now retired into the shared form |
| the checker block | `scripts/check-gbi-decode-fixtures.ps1` (Boundary runs it) | a marker field added without a group member, in either direction |

The checker parses the X-macro list and the harness's `printf "MARKER=…"`
argument list and requires **set equality in both directions** — unpublished
members *and* published-but-unread members are both errors, because a group
that has drifted in either direction is a group nobody is maintaining.

**The falsifier was run before the zero was believed.** Removing
`gNdsBattlePlayablePacingCadenceViolationCount` from the list:

```text
BPLAY_PACE group drift: the harness reads 14 globals and
NDS_BATTLE_PLAYABLE_PACING_GROUP publishes 13. Unpublished:
[gNdsBattlePlayablePacingCadenceViolationCount]; unread: []. Every member of a
debugger-read counter group must be in its publish list.
```

## 3. The engagement proof, and the retraction it forced

**A red-to-green flip is what this cycle expected to report, and it would have
been wrong.** An arm G control was built deliberately with the single call
commented out — `nm` confirms `ndsPlatformPublishBattleFrameCompleteGroups` is
absent from the ELF, and the generated config confirms `NDS_R2_BATTLEPACK 1`,
`NDS_R2_BATTLEPACK_KEEP_CACHE 1`. **It passes Boundary too.** So "the fix made
arm G green" is not supportable, and the inherited premise *"arm G is
Boundary-RED"* does not reproduce on this tree.

**What separates the arms is a counter, and it separates perfectly.** Every
Boundary run prints its own stop-time read of
`gNdsBattlePlayablePacingPresentedFrames` in the pacing smoke line:

| arm | seam | `presented` | pacing smoke |
|---|:--:|---:|---|
| arm G, pacing group only | yes | **212** | `fps=251/500 ticks=282,545,792` |
| arm G, both groups | yes | **212** | `fps=251/501 ticks=282,160,512` |
| flag 0 | yes | **212** | `fps=238/475 ticks=297,374,272` |
| flag 1, shipping arena | yes | **212** | `fps=270/539 ticks=262,257,856` |
| `CAMERA_MATRIX_LEAN=3` | yes | **212** | `fps=239/476 ticks=296,814,144` |
| `LEAN=3` control | **no** | **211** | `fps=241/480 ticks=293,066,432` |
| arm G control | **no** | **211** | `fps=255/508 ticks=276,739,648` |
| *arm G, previous cycle, **RED*** | **no** | **212**, draws **211** | `fps=255/508 ticks=276,709,760` |

Four seam arms at 212 against two no-seam arms at 211, across configurations
whose measured frame rates run from 23.8 to 27.0 fps. **Unpublished, the group
reads one behind — uniformly, so the tuple stays self-consistent and the
harness passes anyway. Published, it reads current.** The previous cycle's RED
is the third state: `PresentedFrames` reached RAM (212) while `DrawCalls` did
not (211), and the group was read **torn**.

That is the mechanism's own prediction — *a stale read is always behind, never
ahead* — measured on the quantity itself rather than inferred from a verdict.
It is also the rail-4 pair: the seam firing (212 on four arms) and a negative
control showing it inert where the code is absent (211 on two).

**So the honest verdict on the blocker is this**: arm G's RED was a *torn*
read, not a permanently red gate, and whether any given build shows torn,
uniformly-stale, or current depends on which lines are resident at the halt —
which is exactly why leaving it to chance was the defect. The fix removes the
dependence. It does not "unblock arm G"; arm G was not blocked on this tree.

**Caveat, stated because it bounds the claim.** The no-seam control is built
from a tree that still *contains* the seam's code (garbage-collected), so it is
not byte-identical to the previous cycle's red binary, and this cycle cannot
say why that binary tore and this one does not. It does not need to: the
publication makes the answer irrelevant.

**A second, independent engagement signal fell out of the gate run.** The
tick-HUD sampler stitches its 128-slot ring by reading
`gNdsBattlePlayablePacingPresentedFrames` over GDB at each ring stop and
recording the skew between the frame it wanted and the frame it got:

| run | seam | ring stops | stops with `skew != 0` | `ringSlots != presentedFrames` |
|---|:--:|---:|---:|---:|
| `c170-seam-bp1` | **yes** | 16 | **0** | **0** |
| `c168-packfix-bp1` | no | 16 | **5** (+1,+1,+1,−1,+1) | **5** |

Sixteen of sixteen stops land exactly on target with the group published, against
five skewed stops and five slot-count mismatches without it. The c168 run also
emitted `WARNING: Tick-HUD samples repeated a presented frame (3 of 1600) …
Seam rows: 288,671,1343`; the c170 run does not. **The instrument this whole
campaign measures on was reading its own frame counter one behind at a third of
its stops**, and the same one-line seam fixes that too.

## 4. The blast radius, taken once and structurally

The exposed shape is not "a group that is printed". It is **a group whose
members a harness compares to another live counter at a stop that can land
mid-update**. Every cross-marker relation in the realtime assert region
(`verify-battle-mariofox-gcrunall-loop-harness.ps1:2900-3520`) was enumerated
mechanically rather than by reading:

| relation | line | status |
|---|---|---|
| `logicLag` / `drawLead` / `phaseLag`, internal to BPLAY_PACE | 769-816 | **published** |
| `taskmanPresentLead = $tmPace[1] - (2 * $bp[4])` | 2963, 3256 | **published** (this cycle) |
| `$fpsHudExpected` from `FPS_HUD[2]`,`[3]` vs `[0]` | 3292-3300 | **published** (R2-04 E2) |
| `$bs[7] -eq $bp[2]` | 3402 | not an exposure — the *same global* read twice in one stop |
| `$safety[16] -eq $fdc[8]` | 3126 | **candidate, unfixed, not measured broken** |
| `$textHud[4..9]` vs `$sourceHud` / `$sourceLower` | 3384-3389 | **candidate, unfixed, not measured broken** |

The last two are named rather than fixed on purpose. They are cross-marker
*equalities between accumulators and HUD mirrors*, not a strictly-ordered pair
with zero slack, they have never been observed red, and fixing what has not
been measured broken is the phantom-defect failure mode. With the group law in
place, adding either is a three-line change: one X-macro list, one line in the
seam, one checker pair.

**Whole-run totals read at an end-of-run stop remain out of scope** — that
finding from the arena cycle stands and was not re-derived.

## 5. Task B — the LEAN=3 row, measured, and the answer is a negative

`docs/KNOWN_ISSUES.md` has held `NDS_R2_CAMERA_MATRIX_LEAN=3` off by default
since 2026-08-09 because Boundary failed its locked-30 phase accounting with
`phaseLag=-1` on a byte-identical binary. The brief's hypothesis was that the
publication seam fixes it. **It cannot be shown to, because the failure no
longer happens without the seam.**

The control was built deliberately and proven to be the control:

```text
build   NDS_R2_CAMERA_MATRIX_LEAN 3   NDS_R2_BATTLEPACK 0     (generated config)
nm      ndsPlatformPublishBattleFrameCompleteGroups   ABSENT   (the only call was
                                                                commented out, so
                                                                --gc-sections
                                                                dropped it)
result  Boundary verification profile passed.   0 `Exception:`
        WARNING: ... present=241 x0.1 fps  (pre-existing; see §5)
```

So on this HEAD, at the shipping default, route 3 passes Boundary **without**
any publication seam. The recorded reproduction is dead. That is consistent
with the mechanism — the row's own text says the trigger is that dropping a
64-byte `syMatrixAdvanceW` *moves every later allocation in the frame*, and
which lines are resident at the stop is exactly what heap layout decides — but
consistency is not proof, and a green arm cannot distinguish "the seam fixed
it" from "the bug is dormant".

**What is therefore true and what is not:**

- The row's *mechanism* question ("a real off-by-one … or the stop-phase model
  is incomplete") **is** answered, and §3 is what answers it: the group is
  measured reading **one behind** on every unpublished arm and **current** on
  every published one. Neither counter has a second write site and neither
  needs one — the write was always coherent and the *read* was not.
- The row's *blocker* ("Level 3 is held off by default pending that answer") no
  longer has a live symptom to point at: LEAN=3 is Boundary-green today with
  and without the seam. Neither does arm G's `drawLead=-1` (§3) — **both
  symptoms are dormant on this tree, and that is the point**: which of
  *current*, *uniformly stale* and *torn* a build shows was never under
  anyone's control, and now the first is the only reachable state.
- **This cycle did not flip it.** A default change is the owner's
  (`BLOCKED(decision: NDS_R2_CAMERA_MATRIX_LEAN default)`), and the honest
  statement of the evidence is "the reason it was held is not reproducible and
  the class is now structurally prevented at this stop", not "the fix closed
  it".

## 6. One warning that is NOT this change

Every arm — including the flag-0 LEAN=3 control with no seam in the binary —
emits:

```text
WARNING: battle_playable Pupupu locked-30 presentation slipped below its
target: present=241..251 x0.1 fps
```

It is a `Write-Warning`, not an assert (`…loop-harness.ps1:3407`, fires outside
295..305), the harness's own gate `$bp[6] -le 305` passes, and it appears on an
arm built without the seam. It is a property of this HEAD and this emulator, not
of the publication fix. Recorded so the next reader does not attribute it here.

## 7. The stress battery, on the shipping-candidate binary itself

`soak-freeze-watch.ps1 -Build build-c170-seam-bp1 -MinutesToRun 11 -PollSeconds 5
-IdenticalFramesToTrip 16 -PressStartSeconds 60 -PressStartOnResults`, 660 s of
wall clock, `BOTH_CPU 1`, DLDI on, canonical one-minute match — the **same
binary** the gate figure below is measured on, so reserve and ticks share one
ROM. Run **before** the gate run, because the arm moves the allocator.

```text
verdict                                    NO-FREEZE
gNdsSCVSBattlePlacementInitCount                 10   battle-scene entries
gNdsVSResultsStartCount                           8   completed matches
gNdsVSResultsRematchCount                         8   START restarts
gNdsSCVSBattleSuddenDeathPrepareCount             2   Sudden Deaths entered
```

| counter | value | requirement | previous cycle (`c168`, no seam) |
|---|---:|---|---:|
| `gNdsTaskmanArenaChosenSize` | **1,548,288** | == requested | 1,548,288 |
| `gNdsTaskmanArenaAllocFailCount` | **0** | == 0 | 0 |
| `gNdsR2AnimCacheArenaReserveFailCount` | **0** | == 0 | 0 |
| `gNdsR2AnimCacheRejects` | **0** | == 0 | 0 |
| `gNdsSyMallocOverflowCount` | **0** | == 0 | 0 |
| `gNdsBattlePackLoadFails` | **0** | == 0 | 0 |
| `gNdsTaskmanGeneralHeapFreeMin` | **52,472** | > 32,768 floor, > 25,600 GObj latch | 52,400 |
| `sGCCommonsMaxNum` | **−1** | cap never fired | −1 |
| `gNdsBattlePackHits` / `Misses` | 1,756 / 1,555 | pack engaged | 1,745 / 1,483 |
| `gNdsEffectPoolFreeMin` | 4 | unchanged, pre-existing | 4 |

**The heap low-water moved +72 bytes** against the previous cycle's identical
battery (52,472 vs 52,400) on a different fight (2 Sudden Deaths against 4), so
the seam's static and dynamic footprint changes nothing the allocator can see —
which is what a function of 82 Thumb instructions and no allocation should do.

## 8. The gate, re-measured on the binary that would ship

`sample-tick-hud-buckets.ps1 -RunnerSlot 2 -Build build-c170-seam-bp1 -Samples 1600
-RingDump`, `NDS_R2_BOTH_CPU=1` + `NDS_TICK_HUD_DRAW=1`, DLDI **on**, mode 163
one-minute match, window = presented frames **439-2038**, rank-80 of 1,600 (the
campaign's convention). Apparatus 24,947, owner-approved. Same build directory,
same target and same ROM the stress battery above ran on.

```text
WORK-H  P50   940,320
        P90 1,091,520 (rank 160)
        P95 1,177,920 (rank 80)      <- BANKED
        top-1% 1,518,528 (rank 16)
        max 5,277,248
        P95 neighbourhood  r77 1,181,248  r78 1,181,184  r79 1,178,240
                           r80 1,177,920  r81 1,177,664  r82 1,176,128  r83 1,174,656

        net of apparatus 1,152,973      raw gap +57,540      NET GAP +32,593
```

**Against the banked arm G figure, this is unchanged.** `c168` (identical
configuration, no seam) measured 1,170,048 / 1,145,101; `c170` measures
1,177,920 / 1,152,973. **+7,872 raw**, against a ≥14,080 cross-build floor
(`plan.md` §2) — *below the floor is "unchanged", not "a cost"*, and §9 bounds
the seam's own price at ≤450 ticks/frame from the image.

**Presented-interval histogram, and it is the arm's own** (harness counters, not
derived):

| | 2 VBI | 3 | 4 | 5+ | max | total | slips | cadence violations |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| **c170 + seam** | **1,745** | 272 | 13 | 8 | 19 | 2,038 | 0 | 0 |
| c168, no seam | 1,745 | 273 | 12 | 8 | 19 | 2,038 | 0 | 0 |

Two-VBlank share **85.6%** on this `NDS_TICK_HUD_DRAW=1` arm.

**Cadence from the `NDS_TICK_HUD_DRAW=0` arm, which is the one `plan.md` §1
item 3 gates on** (`build-c170-seam-bp1-draw0`, same configuration otherwise,
1,600 samples):

```text
VBI 2:1853  3:170  4:7  5+:8   max 19   total 2038   slips 0
two-VBlank share  90.9%          acceptance  >= 95%        SHORT BY 4.1 POINTS
WORK-H  P50 945,024   P95 1,185,408   (not the gate arm; the HUD draw is out)
```

**Acceptance item 3 is NOT met and this cycle does not claim it.** Removing the
HUD draw lifts the two-VBlank share from 85.6% to 90.9% — i.e. the instrument
owns about 5.3 points of the cadence and the product still owns the remaining
4.1. That is a renderer/frame-cost item, not a publication-seam item, and it is
the same gap the +32,593 net tick excess describes from the other side.

**End-of-match invariant pair: `gNdsBattleTextHudP0Damage` 0 /
`gNdsBattleTextHudP1Damage` 76** — identical to `c168` *and* to the flag-0
control `c164-gate-bp0`. The three arms drew the same fight, which is what makes
the tick comparison a comparison.

**Engagement, identical to the banked arm to the unit:** `BattlePackHits` 197 ·
`Misses` 158 · `LoadSteps` 18 · `Bytes` 287,904 · `Dispatch` 1 ·
`AnimCacheRejects` 0 · `ArenaChosenSize` 1,548,288 · `AllocFail` 0 ·
`RelocResolveOffsetCount` **3,629 = 3,629** · heap low-water 52,864.

**Two frames, and only two, differ from `c168` by more than the noise** — 1843
(`SINT` 4,342,976 against 275,712) and 1937 (`STG` 4,373,952 against 180,544),
each about +4.1M in a single bucket, neither at a ring stop. Every other frame
in 1,600 matches closely. **They do not touch the banked percentile** (rank-16
top-1% is 1,518,528 against `c168`'s 1,510,784) and isolated multi-megatick
frames are a property of this instrument across arms, not of this change:
`c169` has 3 (frames 1819, 1826, 779), `c166` has 4, `c164-bp0` and `c168` have
0. **Unexplained, characterised, and handed forward rather than explained away.**

## 9. What the seam costs, bounded from the image rather than from a delta

Disassembled from `builds/build-c170-seam-bp1`'s ELF:
`ndsPlatformPublishBattleFrameCompleteGroups` is **82 Thumb instructions** and
**20 `blx armDCacheFlush`**, once per presented iteration. `armDCacheFlush` is
7 ARM instructions for a 4-byte object plus one `mcr c7,c14,1` and a tail branch
to `armDrainWriteBuffer`. Upper bound with every line dirty: ~220 core
instructions + 20 line writebacks ≈ **≤450 ticks/frame, ~0.04% of the gate**.
Most of these counters (`Result`, `Mode`, the FPS words, `PhaseSlipCount`)
change rarely, and the literal pool shows their addresses are consecutive
(`0x02234160`, `…64`, `…68`, …), so several cleans hit the same line and the
real figure is lower.

**That is far under the ±14,080 cross-build floor (`plan.md` §2), so this cycle
does not try to price the seam from a two-build delta and does not claim one.**
It is also *not* apparatus: the published ROM executes it, unlike the 24,947
tick-HUD instrument, so it is real product cost and is inside the gate figure
rather than subtracted from it.

## 10. What this cycle did NOT do

- **No default flip.** `NDS_R2_BATTLEPACK` / `…_KEEP_CACHE` remain 0 / 0, and
  `NDS_R2_CAMERA_MATRIX_LEAN` remains 2. Both are
  `BLOCKED(decision: shipping default)`.
- **No fix for the two remaining cross-marker relations** (§3). Named, not
  measured broken, not touched.
- **Phase 6's oracle and phase 7's assertion were not built, and the reason is
  structural rather than budgetary.** There is no `NDSAnimClip`, no
  `NDSAnimInstance` and no direct evaluator anywhere in the tree (whole-tree
  search, `src/` and `include/`): K1 phase 5 is unbuilt, so the pack still feeds
  the generic path. **An oracle whose candidate arm does not exist has one arm**,
  which is precisely the shared-path failure phase 4 already paid for — building
  it now would produce a `mismatch = 0` that means nothing. Phase 6 is blocked
  on phase 5, not on this cycle's budget.
- **The seam was not priced by a two-build A/B** (§6): its analytic cost is an
  order of magnitude under the floor that comparison would have to clear, so a
  flag-0-with-seam gate arm would have spent a 1,600-sample run to produce a
  number whose error bars swallow the effect. The image is the better
  instrument here.
- **The `c168` → `c170` comparison is a cross-build one and is treated as such.**
  `c168` was built from `98508e7ad25`+dirty(12); `git diff 98508e7ad25..450470c716a
  -- src/ include/ Makefile` is non-empty (the pack fix and its Makefile block),
  and how much of that was already in c168's dirty tree is not recoverable. So
  the figure below is banked as **c170's own measurement**, and any difference
  from 1,170,048 inside ±14,080 is read as *unchanged*, not as an effect.
- **No published ROM was rebuilt.** Boundary builds
  `smash64ds-battle-playable-proof-hwtri`; the two root ROMs were hashed before
  the first build and after the last and are byte-identical.

## 11. Reproduction

```powershell
# Boundary, one arm.  verify-all.ps1 writes child output with
# [Console]::Out.Write, so ONLY an OS-level redirect captures it -- and
# `cmd /c` must be issued from PowerShell, not from the Bash tool, where the
# quoting drops the /c and you get an interactive shell that exits 0.
$env:NDS_R2_BATTLEPACK='1'; $env:NDS_R2_BATTLEPACK_KEEP_CACHE='1'
cmd /c "pwsh -NoProfile -ExecutionPolicy Bypass -File scripts\verify-all.ps1 -Profile Boundary > b.log 2>&1"

# the LEAN=3 control: same, with NDS_R2_CAMERA_MATRIX_LEAN='3' and the single
# call in taskman_seam.c commented out (the checker's call-site assert must be
# neutered with it).  Confirm the control is really the control:
#   arm-none-eabi-nm <proof.elf> | grep PublishBattleFrameCompleteGroups   -> empty
#   grep NDS_R2_CAMERA_MATRIX_LEAN builds/build-battle-playable-proof-hwtri-harness/nds_build_config.h

# the gate binary: soak FIRST (allocator arm), then sample.  -BothCpu is a
# [bool] and defaults true, so call the script directly -- `pwsh -File` hands
# the binder the string "1" and it throws.
.\scripts\soak-freeze-watch.ps1 -Build build-c170-seam-bp1 -MinutesToRun 11 `
    -PollSeconds 5 -IdenticalFramesToTrip 16 -PressStartSeconds 60 `
    -PressStartOnResults -MakeFlags NDS_R2_BATTLEPACK=1,NDS_R2_BATTLEPACK_KEEP_CACHE=1

.\scripts\sample-tick-hud-buckets.ps1 -RunnerSlot 2 -Build build-c170-seam-bp1 `
    -MakeFlags NDS_R2_BOTH_CPU=1,NDS_R2_BATTLEPACK=1,NDS_R2_BATTLEPACK_KEEP_CACHE=1 `
    -Samples 1600 -RingDump -TimeoutSeconds 2400 -ExtraGlobals <as banked> `
    -RowsCsv …/c170-seam-bp1-rows.csv -JsonOut …/c170-seam-bp1.json
python scripts/census-tick-hud-p95-set.py --rows …/c170-seam-bp1-rows.csv --apparatus 24947

# the checker's falsifier, no build: delete one X(...) line from
# NDS_BATTLE_PLAYABLE_PACING_GROUP and run
pwsh -NoProfile -File scripts\check-gbi-decode-fixtures.ps1
```

`-PressStartSeconds 60` resolves the soak's match timer to **0** = "canonical
one-minute match, no override", which is what lets `-NoBuild`/a matching build
soak the same binary the gate samples. Passing `-MatchMinutes` instead builds a
*different* ROM.
