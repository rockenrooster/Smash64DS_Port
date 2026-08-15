# Pricing the +172,032 B of arena that arm G runs on

**Date:** 2026-08-15 · **Branch:** `codex/r2-runtime2` · **base HEAD `a85ac2dfd5e`**
**Native Battle Kernel slice 1 — can arm G ship, and at what reserve.**
Predecessor: `…/2026-08-15_battlepack-mechanism/BATTLEPACK_MECHANISM.md`.

---

## 0. Outcome first

```text
TASK A      THE GROWTH IS LEGITIMATE, AND THE REASON IS NOT THE ONE THE
            BRIEF ASSUMED.  +172,032 B of arena does not BUY room -- it
            REPAYS the pack's own 189,632 B of extra reservation.  Arm G
            leaves taskman 17,600 B LESS than the shipping arm, not more.

            Predicted from the source constants: -17,600 of heap.
            Measured on the stress battery:      -17,472.
            One mechanism, fully accounted, nothing hidden.

RESERVE     (a) grantable libnds heap   16,384 B under the measured ceiling
                                        (AllocFailCount 0 on arm G's binary)
            (b) taskman general heap    52,400 low-water = +19,632 over the
                                        32,768 floor, +26,800 over the
                                        25,600 GObj latch, GObj cap unfired
            (c) static image           319,808 B proven -- CONTEXT ONLY;
                                        it is the instrument that has now
                                        failed three times on this class

THE TEST    660 s, 12 battle-scene entries, 7 completed matches, 7 START
            restarts, 4 SUDDEN DEATHS, verdict NO-FREEZE.  Run on
            build-c168-packfix-bp1 itself -- the exact binary whose rank-80
            is banked -- so the reserve and the tick figure share one ROM.
            The control ran the identical battery and differs on every
            counter that must differ.

AND        arm G REFUSES NOTHING (Rejects 0/Overflows 0) where the shipping
            control refuses 21 animation loads, because the un-packed
            fighter alone fits 163,840 where two fighters did not fit
            262,144.

CORRECTION  "Boundary pins gNdsTaskmanArenaChosenSize == 1376256" is in
            plan.md 13, HANDOFF and kernel-doc 11, and Boundary does not
            read it.  Both runtime sites sit inside if ($Task34StageStreamCensus),
            a switch Boundary never passes.  The pin is a Task 34 census LAB
            gate on a lab build that is built at defaults, where 1,376,256
            is still exactly right.  NOTHING WAS RETAUGHT; loosening it would
            have deleted a working check to fix a problem that does not exist.

TASK B      ARM G CANNOT SHIP TODAY, AND IT IS NOT THE RAM THAT STOPS IT.
            Boundary: flag 0 GREEN, flag 1 AT THE SHIPPING ARENA GREEN, arm G
            RED.  The middle arm exonerates the pack -- residency, streaming,
            carve and the whole dispatch pass; only the arm that MOVES THE
            ALLOCATOR fails, on ONE term:

              locked-30 pacing ... (logicLag=2 drawLead=-1 phaseLag=1)

            drawLead = DrawCalls - PresentedFrames = -1 is not a state the
            guest can be in: one increment site each, ordered, no return
            between, reset together (the only other DrawCalls++ is the
            fast-logic path, excluded on this arm).  So it is a STALE GDB
            READ -- ARMv5::ReadMem has no DCache lookup -- and it is the
            SAME defect as R2-04 E2, on a different counter group.

            docs/KNOWN_ISSUES.md:173 has been carrying this since 2026-08-09
            as an open either/or ("a real off-by-one ... or the stop-phase
            model is incomplete") and holding NDS_R2_CAMERA_MATRIX_LEAN=3
            off by default for it.  There is a third answer, and it was
            proven six days later for the FPS-HUD group: the remedy is a
            DC_FlushRange publication seam, precedent at
            nds_platform.c:2261.  NOT ATTEMPTED THIS CYCLE -- it changes the
            shipped binary and needs its own build, Boundary and gate
            re-measure (CLAUDE.OPUS.md rail 1).  Handed forward.
```

---

## 1. What +172,032 B actually buys — and it is not headroom

Read from the source constants, not from a delta table
(`src/port/diagnostics.c:7750-7776`, `src/port/reloc_backend_assets.c:6490-6529`):

```text
                                   control (flag 0)   arm G (flag 1 + KEEP_CACHE)   delta
NDS_TASKMAN_ARENA_SIZE                  1,376,256            1,548,288           +172,032
  NDS_R2_ANIM_CACHE_ARENA_BYTES           262,144              451,776           +189,632
    = blob reserve (287,904 -> 32B line)        -              287,936
    + raw file cache                      262,144              163,840
arena left to taskman                   1,114,112            1,096,512            -17,600
```

**The growth does not add room; it repays 90.7% of what the pack's own
reservation takes out.** Arm G leaves the taskman **17,600 B less** arena than
the shipping control does, so its general heap must end up *tighter* than the
control's, not looser. That is the prediction this cycle's measurement tests,
and it is the opposite of how "+172,032 of arena" reads.

## 2. The stress battery — arm G, the exact `build-c168-packfix-bp1` binary

`soak-freeze-watch.ps1 -Build build-c168-packfix-bp1 -NoBuild -MinutesToRun 11
-PollSeconds 5 -IdenticalFramesToTrip 16 -PressStartSeconds 60
-PressStartOnResults`, 660 s of wall clock, canonical one-minute match,
`BOTH_CPU 1`, DLDI on. **No build was spent: this is the binary whose rank-80 is
banked at 1,170,048 / 1,145,101**, so the reserve and the tick figure come from
one ROM.

```text
verdict                                    NO-FREEZE
gNdsSCVSBattlePlacementInitCount                 12   battle-scene entries
gNdsVSResultsStartCount                           7   completed matches
gNdsVSResultsRematchCount                         7   START restarts
gNdsSCVSBattleSuddenDeathPrepareCount             4   Sudden Deaths entered
```

The four allocator gates `docs/VERIFYING.md` names, plus the two that price the
margin:

| counter | arm G | requirement |
|---|---:|---|
| `gNdsTaskmanArenaChosenSize` | **1,548,288** | == requested |
| `gNdsTaskmanArenaAllocFailCount` | **0** | == 0 (the step-down loop never ran) |
| `gNdsR2AnimCacheArenaReserveFailCount` | **0** | == 0 |
| `gNdsR2AnimCacheRejects` | **0** | == 0 — the acceptance test on the 163,840 cache |
| `gNdsSyMallocOverflowCount` | **0** | == 0 |
| `gNdsTaskmanGeneralHeapFreeMin` | **52,400** | > 32,768 mandated floor; > 25,600 GObj latch |
| `sGCCommonsMaxNum` | **−1** | the `ifCommonSetMaxNumGObj` cap never fired |
| `gNdsBattlePackLoadFails` | **0** | the blob arrived on all 12 entries (`LoadSteps` 216 = 18×12) |

## 3. The control, under the identical battery — and the prediction holds to 128 bytes

`build-c168-default-check` is this tree at defaults (`NDS_R2_BATTLEPACK 0`),
same target, same `BOTH_CPU 1`, same `SOAK_MATCH_MINUTES 0`. Same soak
invocation, same 660 s.

| | control (flag 0) | **arm G** | delta |
|---|---:|---:|---:|
| verdict | NO-FREEZE | **NO-FREEZE** | — |
| battle-scene entries | 10 | 12 | +2 |
| completed matches / restarts | 8 / 8 | 7 / 7 | −1 |
| Sudden Deaths | 1 | **4** | +3 |
| `gNdsTaskmanArenaChosenSize` | 1,376,256 | 1,548,288 | +172,032 |
| `gNdsTaskmanArenaAllocFailCount` | 0 | 0 | — |
| `gNdsR2AnimCacheArenaReservedBytes` | 262,144 | 451,776 | +189,632 |
| `gNdsR2AnimCacheRejects` / `Overflows` | **21 / 21** | **0 / 0** | −21 |
| `gNdsBattlePackHits` / `Misses` | 0 / 3,352 | 1,745 / 1,483 | control reads 0 |
| `gNdsSyMallocOverflowCount` | 0 | 0 | — |
| `sGCCommonsMaxNum` | −1 | −1 | cap never fired on either |
| `gNdsGCDrawsActiveMax` | 130 | 129 | −1 |
| `gNdsEffectPoolFreeMin` | 4 | 4 | unchanged — pre-existing, not arm G's |
| `gNdsRendererTask36ReplayArenaStaleCount` | **0** | **16,914** | see §5 |
| **`gNdsTaskmanGeneralHeapFreeMin`** | **69,872** | **52,400** | **−17,472** |

**§1 predicted −17,600 from the source constants alone. The measurement is
−17,472.** The whole heap cost of arm G is the arena its own reservation does not
repay, to within 128 bytes; there is no second mechanism, no accumulation across
the chain, and nothing unexplained. That the two arms drew different fights
(4 Sudden Deaths against 1) and still land on the arithmetic is what makes it a
property of the configuration rather than of the match.

**The control refuses assets and arm G does not.** The control's 262,144 B arena
serves *both* fighters and overflowed on 21 requests; arm G's 163,840 B half
serves only the un-packed Mario and refused nothing, peaking at 137,136 (83.7%).
The pack does not merely displace the cache — it shrinks the cache's remaining
job below the point where it refuses anything. Measured on this battery.

**Both `sGCCommonsMaxNum` read −1**, so the 25,600 `ifCommonSetMaxNumGObj` latch
did not fire on either arm; and `gNdsEffectPoolFreeMin 4` is identical on both,
so that pool sitting one below the source's 5-free refusal cut is a pre-existing
property of the match and not something arm G introduced.

## 4. The three reserves, each named with its instrument

A margin nobody can name is not a margin, so all three are stated separately —
they are different pools and a healthy figure in one says nothing about another.
That is the whole content of the "`check-boot-headroom.ps1` meters static image,
not grantable heap" lesson, now on its third recurrence.

**(a) Grantable libnds heap, at boot — 16,384 B, inherited.**
`ndsTaskmanArenaBytes` (`diagnostics.c:7814`) asks for `NDS_TASKMAN_ARENA_SIZE`
and walks down in 0x1000 steps. The failed +258,048 arm asked `0x18f000` and was
given **1,564,672** after 17 step-downs; arm G asks `0x17a000` = 1,548,288,
**16,384 B (4 pages) under that ceiling**, and `AllocFailCount 0` proves on arm
G's own binary that the loop never stepped.
*Caveat, stated because it is inherited rather than re-measured:* the 1,564,672
came from a different HEAD. It is a sound transfer only because the two binaries
have near-identical static footprints — the arena size is a `calloc` argument,
not an allocation — and arm G's static image is **768 B** larger than the
shipping control's (`fake_heap_start` `0x022463c4` → `0x022466c4`), so the
transfer error is under 0.05% of the figure. **Any further static growth on this
arm eats those 16,384 bytes one for one and only
`gNdsTaskmanArenaChosenSize` can see it.**

**(b) Taskman general heap, during play — 19,632 B over the mandated floor.**
Measured, not projected: low-water **52,400** against the 32,768 floor and
**26,800** over the 25,600 `ifCommonSetMaxNumGObj` latch, with `sGCCommonsMaxNum`
still −1. The source comment at `reloc_backend_assets.c:6515` projected ~54,588
and was right to within 2,188.

**(c) Static image, for future growth — 319,808 B proven.**
`check-boot-headroom.ps1 -Build build-c168-packfix-bp1`: `fake_heap_start`
`0x022466c4`, footprint text 986,172 / data 148,288 / bss 1,307,080.
**This is context, not the answer to this cycle's question** — it is the
instrument that has now failed three times on exactly this class, and it cannot
see (a) or (b). It is reported so the next change can be priced, and because
(a) and (c) are the same bytes: spending (c) spends (a).

## 5. A correction — Boundary never reads this pin

`plan.md` §13, `docs/HANDOFF.md` and kernel-doc §11 all record that "Boundary
pins `gNdsTaskmanArenaChosenSize == 1376256`", and treat that as the reason arm
G "cannot pass Boundary". **The pin is real, but it is not Boundary's.**

Both runtime sites — `verify-battle-mariofox-gcrunall-loop-harness.ps1:2006`
(the gdb `quit 86` gate) and `:2573` (`$task34ArenaBoot[0] -eq 1376256`) — are
inside `if ($Task34StageStreamCensus)`. That switch is passed only by
`benchmark-renderer-fast-raw.ps1 -Task34StageStreamCensus`, whose own build
directory is `builds/build-task34-stage-stream-census-lab`. Boundary reaches the
harness through `verify-battle-playable-realtime-harness.ps1` →
`verify-battle-playable-harness.ps1:142`, whose argument list does not contain
the switch, so it defaults `$false` and neither site is compiled into the run.

So there is nothing to reteach: **the pin is a Task 34 census lab gate on a lab
build that is built at defaults, where 1,376,256 is still the correct and exact
expected value.** Loosening it would delete a working check for no reason.
What Boundary *does* enforce is `check-gbi-decode-fixtures.ps1:2493`, a
**source-text** assert that the string `gNdsTaskmanArenaChosenSize != 1376256`
still appears in the verifier — so the literal cannot be edited without updating
that checker, but no ROM's arena is compared against it during Boundary.

**And a coupling that would have mattered and does not.** The Task 36 replay
admission guard has a legacy form that is exactly `gNdsTaskmanArenaChosenSize !=
0x150000` (`nds_renderer.c:5735`), which arm G's 0x17a000 would fail — silently
disabling the rigid-stage replay path and quietly confounding every tick
comparison in this campaign. It does not fire: both the published
`smash64ds-battle-playable-hwtri` block (`Makefile:1566`) and the tick-HUD block
(`:1717`) force `NDS_TASK53_REPLAY_ARENA_FIX := 1`, whose guard is
`< 0x130000`. Arm G's own `gNdsRendererTask36ReplayArenaStaleCount` **16,914**
is the positive confirmation: that counter exists to count frames the relaxed
guard admits and the legacy guard would have blocked, and on arm G that is every
replay frame.

## 6. Task B — Boundary. Flag 0 GREEN, arm G RED, and the blocker is a KNOWN class

Three arms, `verify-all.ps1 -Profile Boundary`, this tree, published target:

| arm | env | arena | cache | result |
|---|---|---:|---:|---|
| flag 0 (shipping default) | — | 1,376,256 | 262,144 | **GREEN** |
| flag 1 at the shipping arena (= arm **H**'s config) | `NDS_R2_BATTLEPACK=1` | 1,376,256 | 4,096 | **GREEN** |
| **arm G** | `…=1` + `NDS_R2_BATTLEPACK_KEEP_CACHE=1` | 1,548,288 | 163,840 | **RED** |

**That middle row is what makes the attribution.** The pack — its residency, its
streaming, its carve and its whole dispatch, which is the entire architectural
change slice 1 exists for — passes Boundary at the shipping arena. Only the arm
that *moves the allocator* fails, which is precisely the class
`KNOWN_ISSUES.md:173` filed in August and could not attribute.

```text
battle_playable locked-30 pacing failed the 2:1 update/draw ratio, the reachable
stop-phase skews, the 30Hz present cap, cadence, or phase accounting contract
(logicLag=2 drawLead=-1 phaseLag=1 taskmanPresentLead=2).

BPLAY_PACE=0x42505443,0,422,212,211,276709760,255,508,494,2,18,0,195,16,0,0,0,72,0,0,0,0
             magic   mode logic pres draw
MEMARENA=0,22,2,1548288,...      <- the pack configuration did take
```

Everything else in the run passed: `check-gbi-decode-fixtures`,
`check-nds-particle-banks`, `check-harness-registry`, the whole battle, and the
counter dump. The failure is one term of `Test-BattlePlayablePacingStopPhase`
(`verify-battle-mariofox-gcrunall-loop-harness.ps1:769-816`): `drawLead` must be
0 or 1 and reads **−1**.

**`drawLead = −1` is not a state the guest can be in, and that is checkable
rather than arguable.** `gNdsBattlePlayablePacingDrawCalls` is incremented at
`taskman_seam.c:4903` and `gNdsBattlePlayablePacingPresentedFrames` at `:4935`,
in that order, in one straight-line region with no `return` between them, and
both reset sites (`:726-727`, `:4539-4540`) zero them together. A whole-tree
grep finds one other `DrawCalls++`, at `:7963` — and it is guarded by
`use_realtime_presentation == 0`, i.e. the fast-logic path, which is mutually
exclusive with `:4903` and is not the Boundary arm. (That second site is exactly
why the grep was worth running: the one-writer claim was not free.)
So `DrawCalls ≥ PresentedFrames` is invariant in-guest, and a halted read
showing otherwise is **a read artifact, not a dropped draw**.

**The mechanism is already proven in this repo, six days after the issue that
records it was filed.** `docs/KNOWN_ISSUES.md:173-187` (2026-08-09):

> Freeing a per-frame graphics-heap allocation can turn the Boundary locked-30
> phase accounting red (`phaseLag=-1`) … each counter has exactly one write
> site … ordered … with no early return between them, and both reset sites zero
> them together. **So either the pacing accounting has a real off-by-one that
> only some allocation layouts expose, or the stop-phase model is incomplete.**

There is a third answer that entry could not have had. `docs/VERIFYING.md`
(2026-08-15): `ARMv5::ReadMem` special-cases ITCM and DTCM and otherwise falls
through to `BusRead32` with **no DCache lookup**, so a global still dirty in the
ARM9 data cache reads **stale** over GDB, and a group published together reads
**torn** — because ARM946E-S does not write-allocate, a store to a non-resident
line reaches RAM while the next store to the same line, after a load has filled
it, only marks it dirty and aborts the bus write. That is the whole of R2-04 E2,
measured frame by frame in
`artifacts/verification/2026-08-15_fpshud-publication.txt` and fixed by
`DC_FlushRange` at the publication seam (`ndsPlatformPublishBattleFpsHudGroup`,
`nds_platform.c:2261`).

**Both locked-30 tuples fall out of it, and only staleness explains both signs.**
A stale read is always *behind*, never ahead:

```text
2026-08-09  phaseLag = presented - phaseSum = -1   -> PRESENTED read stale (written first)
2026-08-15  drawLead = draw - presented   = -1     -> DRAWCALLS read stale (written first)
```

In each case the counter written *earlier* in the iteration is the one that reads
low, which is what a line still dirty at the stop looks like — and in each case
the trigger was **a change that moved heap layout**, which is what decides which
lines are resident when the debugger halts.

**It is deterministic, and that was checked rather than assumed.** Arm G was
built and run twice, independently. `BPLAY_PACE` came back **byte-for-byte
identical** both times — including the wall-tick field:

```text
run 1  BPLAY_PACE=0x42505443,0,422,212,211,276709760,255,508,494,2,18,0,195,16,0,0,0,72,0,0,0,0
run 2  BPLAY_PACE=0x42505443,0,422,212,211,276709760,255,508,494,2,18,0,195,16,0,0,0,72,0,0,0,0
```

A deterministic reading is not noise, and this one is not a race: it is a
property of where the arm's lines sit when the debugger halts.

**What this does and does not mean.**

- It does **not** exonerate arm G. Boundary is red at arm G's configuration and
  that is a stop rule; arm G is **not shippable today** whatever §2–§4 say about
  the RAM.
- It does mean the blocker is **not the arena's legitimacy, not the pack's
  correctness, and not gameplay** — no gameplay counter moved, the match ran, and
  the same binary class passes every allocator gate and the whole stress battery.
- The remedy is named and precedented: publish the four `BPLAY_PACE` counters
  through a `DC_FlushRange` seam exactly as the FPS-HUD group was, so the
  harness reads a coherent group. That is a **functional** change to the shipped
  binary, so it needs its own build, its own Boundary run and its own gate
  re-measure — a chain this cycle could not start and finish honestly
  (`CLAUDE.OPUS.md` rail 1). **It is handed forward, not attempted.**
- It also fixes a lever the owner has been holding since 2026-08-09:
  `NDS_R2_CAMERA_MATRIX_LEAN=3` is held off by default pending exactly this
  answer.

## 7. Phase 6's same-build oracle — what the arms would share, and what that hides

Not built this cycle (it needs the direct evaluator, which is slice 1's later
half). This is the design constraint it must be built against, because the
campaign has already paid for getting it wrong once.

**The paid lesson.** Phase 4 scored `mismatch = 0` across 297 clips while the
pack shipped the wrong bit order. The host probe's `normalize()` implements
pipeline stages 4a–4b; the ROM also applies **4c**
(`ndsRelocAObj16EncodeForNativeBitfields`), and the direct path skips
`ndsRelocFinalizeLoadedFile`, so nothing applied 4c at runtime either. **Both
sides of the equivalence test used the same disk-order decoder**, so the missing
stage was invisible to it and a ROM data abort found it instead.

**The rule that follows, and it is structural rather than advisory:**

> An oracle proves nothing about any stage that is *upstream of, and common to,*
> both of its arms. Place the comparison point downstream of everything the
> change touches, and make each arm produce its own bytes through its own
> complete production path.

For phase 6 that means:

| component | shared? | what the sharing makes invisible |
|---|---|---|
| the AObj/figatree **evaluator** | **yes, deliberately** | a defect in the evaluator itself — acceptable, slice 1 does not change it, and it is what makes the comparison a *representation* test |
| **byte production** (FAT read → swap → reloc fixups → AObj16 normalize → 4c native encode) vs the build-time pack | **no** | — this is the whole change and must not be shared |
| the **host converter's** derivation of the pack | **danger** | if the converter replays the runtime pipeline's *implementation*, a bug in that implementation is copied into the pack and both arms agree. This is the exact shape of the phase-4 failure |
| the **comparison harness** itself | yes | a harness that cannot report a mismatch. Answered only by a falsifier |

**Two counters, not one.** Value mismatch cannot see ordering: two `func_anim`
callbacks with identical payloads in swapped order compare equal value-wise. So
`mismatch == 0` **and** `event-order mismatch == 0` are separate acceptance
conditions, exactly as §K1 phase 6 words them.

**The falsifier is mandatory and is the only defence against the converter row.**
Prove the oracle can be non-zero before believing its zero: corrupt one clip
(flip one key value; swap two adjacent events) and require the two counters to
rise and the fail-closed path to engage. §K-POOL already has the precedent —
"disk-order decode raises on **75 of 2,713** scripts, native **0**" — and that
falsifier is what turned a shared-decoder equivalence claim into a real one.

**Phase 7's after-GO assertion has a second requirement nobody has stated.** The
seven K0 counters must be gated on GO *and separated by asset class*: `get_fat` /
`f_lseek` in this match are **majority BGM** (HANDOFF), so an ungated counter
reads non-zero for reasons that have nothing to do with the animation
architecture. Its negative control already exists and must stay non-zero while
only one fighter is packed: `gNdsBattlePackMisses` (1,483 over the 12 entries
measured here) is precisely the un-packed fighter's acquisitions.

## 8. What this cycle did NOT do

- **No default flip.** `NDS_R2_BATTLEPACK` / `NDS_R2_BATTLEPACK_KEEP_CACHE` /
  `NDS_R2_BATTLEPACK_DISPATCH` remain 0 / 0 / 1. Turning the pack on by default
  is `BLOCKED(decision: …)` for the owner even with everything green.
- **No harness edit.** §5 is the reason: the pin Boundary was said to enforce is
  not Boundary's, and the pin that exists is correct where it lives.
- **No flag restructuring.** Folding `KEEP_CACHE` into `NDS_R2_BATTLEPACK` would
  make arm G one flag and would also destroy the matched control the pair exists
  for (at `BATTLEPACK=0`, `KEEP_CACHE` grows the arena and nothing else). Left
  alone deliberately.
- **No re-measurement of the gate by re-running it.** The banked
  1,170,048 / 1,145,101 was taken on `build-c168-packfix-bp1`, and this cycle's
  source edits are comments only; §7 proves by ELF section byte-compare that the
  arm which would ship is the arm that was measured. That is the same method the
  mechanism cycle established, and it is both cheaper and stronger than a second
  bit-deterministic run.
- **The grantable ceiling was not re-measured on arm G's own binary.** 1,564,672
  is inherited from a same-footprint binary at a different HEAD (§4a), and
  `AllocFailCount 0` is what proves the request was granted here.
- **Phase 6's oracle and phase 7's assertion are designed (§7), not built.**
- **The locked-30 publication fix is not written.** §6 names it, prices the
  chain it starts, and hands it forward rather than half-landing it.
- **No 5-minute-match soak arm.** The chain battery covers Sudden Death and the
  restart chain, which is where the per-player figatree heaps and the scene
  re-entries live; a long *single* match was not run, and the heap question it
  would answer (within-match accumulation) is the one the flat 52,864 → 52,400
  reading already argues against.

## 9. Builds, artifacts and root ROMs

**The four `boundary-*-trimmed.log` files are reduced.** Each raw log was ~19 MB,
of which essentially all was devkitARM `-W` diagnostics from `decomp/` and
`src/port/`. Kept verbatim: every checker verdict, the `Running verifier:` line,
and the complete harness outcome block (for the red arms that block *is* the
counter dump, `BPLAY_PACE` and `MEMARENA` included). The full logs were written
to disk first and filtered afterwards, never filtered in the pipeline — a
pattern filter cannot see a multi-line throw.


**Zero lab builds.** Both soaks ran `-NoBuild` on binaries that already existed
(`build-c168-packfix-bp1` = arm G, `build-c168-default-check` = the control),
which is why the reserve and the banked rank-80 share one ROM. The four
Boundary runs each rebuilt the published target as `verify-all.ps1` always does.

**Root ROMs — restored and verified unchanged across the cycle.** Both files were
copied out before the first build and copied back after the last, because a
published target name hardcodes its output to the project root whatever `BUILD=`
says, and NitroFS packs directory entries nondeterministically, so a rebuild is
not guaranteed to reproduce a hash even from identical source.

```text
smash64ds.nds                          54c07fac80c50418949908701f7c2bdbf27512c5f96ac09086fabbb0df6ac68a
smash64ds-battle-playable-hwtri.nds    2015fbd1f68b81c03626d8c6d473c8bcbcf527a3a26dfe86ff19bd74ecbb1360
```

**No inertness byte-diff is claimed for arm G's binary.** The source edits this
cycle are comments only (`Makefile`, `diagnostics.c`, `reloc_backend_assets.c`)
and Boundary went green at two configurations with them in place, but
`build-c168-packfix-bp1` was not rebuilt, so "identical instructions" is
**stated as an argument, not as a measurement**. It is deliberately left for the
cycle that lands the publication seam, which changes the binary functionally and
must re-measure anyway — proving byte-identity now would be proving it about a
binary that is already superseded.

## 10. Reproduction

```powershell
# Task A, the decisive read.  No build: build-c168-packfix-bp1 IS arm G.
# -BothCpu defaults $true, so this survives `pwsh -File` (a [bool] parameter
# does not -- `-File` hands the string "1" to the binder and it throws).
.\scripts\soak-freeze-watch.ps1 -Build build-c168-packfix-bp1 -NoBuild `
    -MinutesToRun 11 -PollSeconds 5 -IdenticalFramesToTrip 16 `
    -PressStartSeconds 60 -PressStartOnResults

# the control, identical battery
.\scripts\soak-freeze-watch.ps1 -Build build-c168-default-check -NoBuild `
    -MinutesToRun 11 -PollSeconds 5 -IdenticalFramesToTrip 16 `
    -PressStartSeconds 60 -PressStartOnResults

# Boundary at arm G's configuration.  The env vars reach make because both
# flags are `?=` and neither is `override`n by the published target's block.
# verify-all.ps1 writes child output with [Console]::Out.Write, so no
# PowerShell redirection captures it -- use an OS-level redirect.
$env:NDS_R2_BATTLEPACK='1'; $env:NDS_R2_BATTLEPACK_KEEP_CACHE='1'
cmd /c "pwsh -NoProfile -ExecutionPolicy Bypass -File scripts\verify-all.ps1 -Profile Boundary > b.log 2>&1"
```

**Read the counters, not the verdict line.** `NO-FREEZE` on its own is
pixels-only; the allocator gates are in the block `soak-freeze-watch.ps1` prints
after it, and the four that matter here are `ChosenSize == requested`,
`AllocFailCount == 0`, `ReserveFailCount == 0`, `Rejects == 0`
(`docs/VERIFYING.md`).

