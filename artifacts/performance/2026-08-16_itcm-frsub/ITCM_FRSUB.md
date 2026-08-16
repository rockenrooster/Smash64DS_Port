# The 456-byte `__aeabi_frsub` eviction does not exist: the mechanism that takes it takes a 684-byte section, and 228 of those bytes run 2,397 instructions a frame

**Date:** 2026-08-16 · **Branch:** `codex/r2-runtime2` · **base HEAD `31c4bca3922`**
**0 builds, 0 emulator runs for this item.** Everything below is `nm`/`objdump` over ELFs
already in `builds/`, joined to a per-PC CSV already in `artifacts/performance/`.
**UNITS: bytes; 2 profile cycles = 1 project tick.** Every table states its window.

```text
REQUIREMENT  +65,297 net ticks per presented frame at rank-80.  Basis
             build-c220-camship, rank-80 1,210,624 raw / 1,185,677 net against the
             1,120,380 gate (apparatus 24,947), band 41-120 1,218,356; shipping
             renderer (GX_COMPOSE 0), bore 0, mode 163 one-minute match,
             NDS_R2_BOTH_CPU=1, 1,600 samples, frames 439-2038, slips=0
             (../2026-08-16_camera-ship/CAMERA_SHIP.md section 3).

REFUTED      "__aeabi_frsub's 456 B blob is proven unreachable, so evict it and ITCM
             free goes 220 -> 676 B."  The UNREACHABILITY is correct and is not
             disputed.  The EVICTION is not available: the three dead symbols and
             four live ones share ONE 684-byte `.itcm` input section in
             `_arm_addsubsf3.o`, and NDS_TASK9_FLOAT_MAIN_MEMBERS -- the mechanism
             ITCM_CENSUS.md section 3 names -- operates on the MEMBER.  Listing it
             frees 684 B and moves 228 B of live code with it.  Section 1.

SIZE         THE LIVE TAIL IS HOT.  __aeabi_ui2f / __floatsisf / __aeabi_l2f run
             2,396.6 instructions per frame whole match and cost 3,544.7 tk/fr on
             the marginal-80 -- today, from zero-wait ITCM.  __aeabi_ui2f alone has
             99 call sites in 21 functions.  So the eviction is not "zero ticks on
             its own": it is a placement REGRESSION of unmeasured size, spent to buy
             space.  Section 2.

CONSEQUENCE  ITEM B WAS NOT BUILT, and the reason is not only its blocker.  Ranking
             every .main symbol of 16-800 bytes by the metric ANIM_ITCM.md section 6
             itself used -- marginal-80 icache_fill ticks per resident byte -- puts
             ndsRendererHardwareBindTextureName SEVENTH at 14.19, behind ftGetStruct
             (32.59), ndsStageCollisionLoopGeometryReady (27.62), __aeabi_lmul
             (26.27), get_fat.isra.0 (24.33), ndsR2AnimBuildTrackTable (22.52) and
             DynamicArrayGet (39.06).  Spending the freed ITCM on the seventh-best
             candidate is not the best available move and this cycle did not make
             it.  Section 3.

AVAILABLE    A FREE-SPACE ROUTE THAT COSTS NOTHING EXISTS AND IS NOT THE FRSUB BLOB.
             Four port-side ITCM residents execute ZERO instructions in the window
             and are one-line NDS_TASK82_ITCM_CODE removals apiece:
             ndsRendererNativeEmitDenseRawRun 256, ndsRendererNativeApplyStateSpan
             192, ...EmitProductionRawTexturedRun 128,
             ...EmitProductionRawUntexturedRun 112 = 688 B, taking the instrument
             from 220 B free to 908 B and the proof ROM from 2,572 to 3,260.
             NOT TAKEN -- it needs the shipping-configuration confirmation in
             section 4 and a build this cycle did not spend.  Section 4.
```

---

## 1. The refutation, from the object file the build itself emitted

`ITCM_CENSUS.md` §3 established that the blob cannot execute, three ways, and none of that
is disputed here. What it also said was:

> **Mechanism to take it, already built:** add the member to `NDS_TASK9_FLOAT_MAIN_MEMBERS`
> (`Makefile`), which keeps it extracted, `--redefine-sym`'d and in `$(OFILES)` and only
> skips `--rename-section`.

The member is `_arm_addsubsf3.o`. Read it as the build emitted it
(`builds/build-c220-camship/_arm_addsubsf3.itcm.o`, `frsub-evidence.txt` §1):

```text
Idx Name          Size      VMA       LMA       File off  Algn
  5 .itcm         000002ac  00000000  00000000  00000034  2**2      <- ONE section, 684 B

00000000 000001c8 T __aeabi_frsub                          ]
00000008 000001c0 T __subsf3 / __nds_task16_..._fsub_golden ]  456 B, DEAD
0000000c 000001bc T __addsf3 / __nds_task16_..._fadd_golden ]
000001c8 00000028 T __aeabi_ui2f / __floatunsisf           ]
000001d0 00000020 T __floatsisf / ..._i2f_golden           ]  228 B, LIVE
000001f0 000000bc T __aeabi_ul2f / __floatundisf           ]
00000200 000000ac T __aeabi_l2f  / __floatdisf             ]
```

`--rename-section` and the `<stem>.mainram.o` filename both act on the whole member, and a
linker cannot split an input section. **684 bytes move or none do.** The same seven symbols
occupy `0x01ff8020..0x01ff82cc` in the linked basis image, with `__aeabi_fmul` starting
immediately after, so the object layout and the image layout agree
(`frsub-evidence.txt` §2).

> The 456 in `ITCM_CENSUS.md` §3 and the "220 → 676" in its §5 are the size of the *dead
> region*, not the size of what the named mechanism moves. Both documents' arithmetic is
> internally right; the step that was never taken is reading the object file.

---

## 2. What the live tail costs, today, in ITCM

`c200-off-pc.csv`, 1,600 presented frames 439–2038, `regions=1601`, `marginal_frames=80`.
The member sits at the identical address in `build-c200-trackprof-off` and
`build-c220-camship`, so the per-PC rows map directly.

| range | bytes | PCs with any execution | whole instr/frame | marginal-80 tk/fr |
|---|---:|---:|---:|---:|
| `__aeabi_frsub` / `__subsf3` / `__addsf3` | 456 | **0** | **0.0** | **0.0** |
| `__aeabi_ui2f` / `__floatsisf` / `__aeabi_ul2f` / `__aeabi_l2f` | 228 | **42** | **2,396.6** | **3,544.7** |
| — `__aeabi_ui2f` + `__floatsisf` shared tail | 40 | 8 | 612.8 | 1,137.7 |
| — `__aeabi_ul2f` / `__floatundisf` | 16 | 0 | 0.0 | 0.0 |
| — `__aeabi_l2f` / `__floatdisf` | 172 | 34 | 1,783.7 | 2,407.0 |

**Callers, from `objdump -d` over the whole c220 image** (`frsub-evidence.txt` §4):

```text
__aeabi_ui2f   99 call sites in 21 functions   lbParticleUpdateStruct 17,
                                               ndsRendererAdapterPrepareMaterialSegment 14,
                                               ndsRendererAdapterBuildNativeMaterialSnapshot 14,
                                               ndsOpeningRoomRecordDObjDraw 14,
                                               gcParseMObjMatAnimJoint 8, gcParseDObjAnimJoint 6, ...
__aeabi_l2f     3 call sites in  1 function    ndsRendererHardwarePrepareLitDirection
__addsf3        1 call site  in  1 function    __aeabi_frsub  (internal to the dead blob)
```

`__floatsisf`'s 727,249 whole-match instructions arrive with **no direct caller**: it is the
shared tail `__aeabi_ui2f` branches into eight bytes above it. That is why a symbol with no
`bl` still executes, and it is why the two must be priced as one 40-byte block.

**So the eviction is not free.** It removes 3,544.7 tk/fr of marginal-80 execution from
zero-wait memory and makes it fetch-charged `.main`. How much of that returns as icache fill
is not measured here and this document does not guess — it is stated as a cost of unknown
size, which is exactly why it is not a "buys zero ticks on its own" move.

The 456 B alone is not separable by any tool in this toolchain. `objcopy` renames whole
sections; `ld` cannot split an input section; the only remaining route is regenerating the
member, i.e. hand-authored replacements for `__aeabi_ui2f`/`__floatsisf`/`__aeabi_l2f`. Those
are shared soft-float leaves reached from 21 functions, so that is a correctness risk taken
to buy 456 bytes, and it is refused here.

---

## 3. Item B ranks seventh on its own metric

`ANIM_ITCM.md` §6 ranked three candidates by marginal-80 `icache_fill` tk/fr per resident
byte. Ranking **every** `.main` symbol of 16–800 bytes by that same metric, on the same CSV
(`candidate-ranking.txt`):

| # | symbol | bytes | marg-80 icache tk/fr | **tk/fr per byte** |
|--:|---|---:|---:|---:|
| — | *`cpuGetTiming` (\*)* | *24* | *1,703.3* | *70.97* |
| 1 | `__syscall_getreent` | 20 | 784.0 | 39.20 |
| 2 | `DynamicArrayGet` | 22 | 859.4 | 39.06 |
| — | *`tickGetCount` (\*)* | *92* | *3,525.2* | *38.32* |
| 3 | **`ftGetStruct`** | 92 | **2,998.5** | **32.59** |
| 4 | `ndsStageCollisionLoopGeometryReady` | 68 | 1,878.0 | 27.62 |
| 5 | `__aeabi_lmul` | 86 | 2,259.5 | 26.27 |
| 6 | `get_fat.isra.0` | 352 | 8,562.6 | 24.33 |
| 7 | `ndsR2AnimBuildTrackTable.constprop.0.isra.0` | 72 | 1,621.4 | 22.52 |
| … | `glBindTexture` | 92 | 1,645.8 | 17.89 |
| … | **`ndsRendererHardwareBindTextureName`** | **268** | **3,802.1** | **14.19** |
| … | `gcPlayDObjAnimJoint` | 604 | 8,218.6 | 13.61 |

(\*) `cpuGetTiming` and `tickGetCount` are the **tick-HUD apparatus's own timing leaves**.
Moving them changes the instrument and its 24,947-tick apparatus subtraction, not the
product. They are listed so nobody re-derives them as candidates, and they are excluded.

Three things follow, and all three are stated as sizes rather than predictions — the CSV is
`build-c200-trackprof-off` at `GX_COMPOSE=1`/`FTANIM_TRACK=1` against a `0`/`0` ship, the
caveat `ANIM_ITCM.md` §2 already flagged and which the renderer rows carry most:

1. **`ftGetStruct` is 2.3× Item B's rate in 34% of its bytes.** `SITR_DIRECT_CHILDREN.md` §8
   independently measured it at **264.24 calls per marginal frame**, the highest call rate of
   any symbol in the fighter subtree. Its two-copy route is *not* free the way Item B's is —
   it has hundreds of call sites, so the route needs a dispatcher whose cost appears on
   **both** arms and therefore cancels in the paired difference, but which the shipped form
   would not carry. That is a design note, not a blocker.
2. **Six rows are the card-read path and they are enormous on the marginal mask and tiny
   whole-match**: `get_fat.isra.0` 8,562.6 marginal against 979.7 whole, `mutexUnlock`
   3,461.1 against 315.0, `_FAT_read_r` 1,280.7 against 78.8, `read` 661.6 against 37.3,
   `__syscall_getreent` 784.0 against 83.8, `__getreent` 411.8 against 66.9. Together
   **792 bytes carrying 15,810 tk/fr of marginal-80 instruction fetch**, essentially all of
   it on the seven card-read frames `../2026-08-16_sitr-excursion/` identifies. That is a
   *placement* share of the card-read premium, and it is new.
3. **`gcPlayDObjAnimJoint` (604 B, 8,218.6) now fits** a freed budget, and the `.text.hot`
   note in `linker/nds_hot_text.ld:180-200` records that exactly this move was tried as
   Task 94 and regressed WORK-H P50 by 6,144. That verdict was taken on a **128-frame
   window** and as a **cross-build** pair — the instrument the campaign retired on 2026-08-04
   and the comparison class with a ≥14,080 rank-80 floor. It is recorded here as re-testable
   *by the same-binary route method only*, and this cycle did not re-open it.

---

## 4. The free-space route that is actually available

Four port-side ITCM residents execute **zero instructions** across the window — no per-PC row
exists for any address in any of them, which is a stronger statement than a zero count:

| symbol | bytes (c220) | PCs with execution | how it moves |
|---|---:|---:|---|
| `ndsRendererNativeEmitDenseRawRun` | 256 | 0 | drop `NDS_TASK82_ITCM_CODE` |
| `ndsRendererNativeApplyStateSpan` | 192 | 0 | drop `NDS_TASK82_ITCM_CODE` |
| `ndsRendererNativeEmitProductionRawTexturedRun` | 128 | 0 | drop `NDS_TASK82_ITCM_CODE` |
| `ndsRendererNativeEmitProductionRawUntexturedRun` | 112 | 0 | drop `NDS_TASK82_ITCM_CODE` |
| **total** | **688** | | |
| *(also zero: `ndsFTParamsInvalidateFighterParts` 54)* | *54* | *0* | *same* |

None of the five is pinned by `scripts/check-renderer-itcm-placement.ps1`, which names
`ndsRendererApplyVertexCommand`, `…HardwareLitShadeColorPrepared`, `…HardwareSubmitVertex`,
`…SubmitHardwareTriangle`, `ndsRendererScanList` and the three native-fighter production
functions and nothing else. `ANIM_ITCM.md` §6 already nominated two of them ("the two raw-run
emitters are 240 B and are the cheapest honest next slice").

```text
region 0x7fe0 = 32,736 B (linker/nds_hot_text.ld:18, ITCM minus the vectors)

smash64ds-battle-playable-tickhud-hwtri   (the measurement instrument)
   build-c220-camship          .itcm 0x7f04 = 32,516        220 B free
   after the four evictions                                 908 B free
smash64ds-battle-playable-proof-hwtri     (what Boundary builds and grades)
   this tree                                30,164        2,572 B free
   after the four evictions                               3,260 B free
```

**The one premise this needs and does not yet have.** The zeros are read on
`build-c200-trackprof-off`, which is `NDS_R2_FIGHTER_GX_COMPOSE=1` against the ship's `0`;
`NDS_R2_STRIP_ROUTE` is `0` in **both**, so the strip-route gate is not the difference. The
raw-run emitters have call sites inside `#if NDS_R2_FIGHTER_GX_COMPOSE` blocks
(`src/nds/nds_renderer.c:28729` among them), so a shipping-configuration confirmation is owed
before the eviction is taken. `../2026-08-16_sitr-excursion/`'s `build-c221-sitrprof` capture
is that configuration and answers it at no extra cost — see that document's §7.

---

## 5. What this cycle did NOT do

- **The frsub blob was not evicted, and this document argues it should not be** by the named
  mechanism. Taking `_arm_addsubsf3.o` costs an unmeasured placement regression on 3,544.7
  tk/fr of live soft-float conversion to buy 684 bytes.
- **Item B (`ndsRendererHardwareBindTextureName`, 268 B, 3,802 tk/fr ceiling) was not built.**
  §3 says why: it is the seventh-ranked candidate on its own metric and the cycle's budget
  went to the item that is twelve times larger.
- **No ITCM eviction was taken at all**, so no ITCM byte count changed and no ROM was linked
  for this item. §4 is a route with a stated open premise, not a change.
- **`gcPlayDObjAnimJoint` was not re-tested**, and `linker/nds_hot_text.ld`'s prohibition
  stands. §3 records only that the evidence behind it predates both the whole-match
  instrument and the same-binary route.
- **`ftGetStruct` was not routed.** §3 sizes it and names the dispatcher design; nothing was
  written.
- **No default flipped, no ROM published, `decomp/` untouched.**

---

## 6. Reproduction

```powershell
arm-none-eabi-objdump -h builds\build-c220-camship\_arm_addsubsf3.itcm.o
arm-none-eabi-nm -S -n --defined-only builds\build-c220-camship\_arm_addsubsf3.itcm.o
```

`frsub-evidence.txt` is those two plus a per-PC sum over the two halves of
`../2026-08-15_ftanim-dispatch-attribution/c200-off-pc.csv` and a `bl`/`b` target scan of
`objdump -d` over the whole c220 ELF. `candidate-ranking.txt` is `nm -S` over
`builds/build-c200-trackprof-off`'s ELF joined to the same CSV. **No build and no emulator
run is required for any of it.**
