# 44.3% of resident ITCM never executes — and the placement bias the campaign was accused of runs the other way

**Date:** 2026-08-16 · **Branch:** `codex/r2-runtime2` · **base HEAD `f5e13aa3e27`**
**Zero builds. Zero emulator runs.** Everything here is `nm`/`objdump` over ELFs already
in `builds/`, joined to a per-PC CSV already in `artifacts/performance/`.
**UNITS: bytes. Execution is over the 1,600-frame gate window, frames 439–2038.**

```text
CENSUS    EVERY ITCM RESIDENT, NOT JUST THE ONE ANYONE ASKED ABOUT.  32,180
          walked bytes on build-c200-trackprof-off; 17,938 execute; 14,242
          (44.3%) never execute a single instruction in the whole window.
          That splits 2,448 B in 33 WHOLLY dead blocks (reproducing last
          cycle's 2,454 to within 6 B) and 11,010 B COLD INSIDE 52 LIVE
          blocks -- 4.5x the whole-symbol pool.  Section 1.

SCANLIST  ndsRendererScanList, censused for the first time: 7,728 B resident,
          7,620 B of code, 3,508 B executed = 46.0%.  4,112 BYTES -- 12.6% of
          the entire 32,736 B region -- NEVER EXECUTE, in the single largest
          resident.  Four more renderer functions hold another 5,372 the same
          way.  None of it is reclaimable by evicting a symbol.  Section 2.

FRSUB     456 B, AND IT IS UNREACHABLE BY CONSTRUCTION, NOT MERELY SILENT.
          Three reads on the CURRENT build's linked image: no branch to any of
          the three symbols from outside the blob; no word in ANY allocatable
          PROGBITS section holds any of their addresses; zero executed
          instructions.  This is the strongest evidence class available and it
          is the only ITCM eviction on the board that needs no reachability
          argument.  NOT TAKEN THIS CYCLE -- section 5 says why.  Section 3.

PREMISE   THE BIAS IS REAL BUT IT IS NOT WHERE IT WAS PLACED, AND ON THE ANCHOR
          ROW IT RUNS TOWARD FIXED POINT.  Read from the linked ELF:
            guMtxCatF                       0x020356e8   .main
            ndsRendererMtxMul20p12          0x01ff9dac   ITCM
          The 5.14x prior's FIXED arm is the ITCM-resident one and its float
          arm is not, so 5.14x OVERSTATES what a fixed rewrite converts at.
          Every other pair the campaign measured has BOTH bodies in .main:
          camera, sim leaf, collision.  Section 4.

REAL BIAS ONE LEVEL DOWN, AND ALREADY MEASURED WITH A SIGN, TWICE.  A float
          body in .main runs its arithmetic inside ITCM-resident library leaves
          and pays `bl` overhead instead of fetch; a fixed body inlines that
          arithmetic into its own fetch-charged .main bytes.  CAMERA_Q20_12
          section 6 measured that trade directly -- +3,032 B of inlined leaves
          turned -4,736 into +1,600 -- and the collision ring's K-ICACHE null
          is the same mechanism.  NOTHING REOPENS.  Section 4.2.

BUDGET    220 B free on the INSTRUMENT, 2,572 B on the PROOF ROM.  Taking the
          frsub blob: 676 / 3,028.  A particle/quad fixed kernel needs
          1,100-1,500 B, so the warm re-test DOES NOT FIT on the instrument --
          which is the only target that can price it.  Section 5.
```

---

## 1. The census, and what it is measured on

| | |
|---|---|
| symbol/placement oracle | `builds/build-c200-trackprof-off/…-tickhud-hwtri.elf`, `objdump -d --section=.itcm` |
| execution oracle | `../2026-08-15_ftanim-dispatch-attribution/c200-off-pc.csv`, column `all_instructions` |
| window | 1,600 presented frames, 439–2038, `regions=1601`, `marginal_frames=80` |
| current-inventory oracle | `builds/build-c219-animitcm-ship/…-tickhud-hwtri.elf` |
| script | `census.py` (this directory); raw output `c200-census.txt`, `arm-placement.txt` |

`build-c200-trackprof-off` is the only build in the tree with per-PC execution data, so it
is where execution is measured. Its ITCM **inventory** is not today's: §1.2 diffs it against
the shipping build and every conclusion below is stated against the current one.

**The disassembly's linear walk IS the non-overlapping decomposition.** `objdump` emits one
header per address, so three overlapping aliases of one blob (`__aeabi_frsub` / `__subsf3` /
`__addsf3`) come out as three adjacent blocks whose sizes sum to the blob. Summing `nm`
sizes instead double-counts and produced a total larger than the region last cycle
(`[[read-arrays-as-arrays]]`, applied to a symbol table).

```text
walked           32,180 B      85 blocks
executed         17,938 B
never executed   14,242 B      44.3%
  of which:
    33 wholly-dead blocks       2,448 B
    cold inside 52 live blocks 11,010 B
    literal .word pools           896 B  (resident and fetched, never "executed")
```

The 2,448 reproduces `../2026-08-16_anim-itcm/itcm-census.txt`'s 2,454 to within 6 B: 2 B is
padding `nm` does not count on `ndsFTParamsInvalidateFighterParts` (54 against 56) and 8 B is
the data-only labels `tiny` and `one`, which `objdump` emits with no instruction lines at
all. Two independent decompositions of the same region agreeing to 6 B is the control.

### 1.1 What is still cold in the build that ships

`build-c219-animitcm-ship`'s ITCM is `build-c200-trackprof-off`'s minus twelve blocks
(the two libgcc float-compare members and the light-shade LUT builder, all evicted last
cycle) plus `ndsR2AnimValueQ`. Every other block is common, so the c200 execution column
maps onto it directly:

| block | bytes | note |
|---|---:|---|
| `__addsf3` / `…fadd_golden` | 444 | **unreachable, §3** |
| `threadUnblockAllByValue` | 312 | kernel path — leave |
| `ndsRendererNativeEmitDenseRawRun` | 256 | strip-route gated |
| `ndsRendererNativeApplyStateSpan` | 192 | |
| `ndsRendererNativeEmitProductionRawTexturedRun` | 128 | strip-route gated |
| `ndsRendererNativeEmitProductionRawUntexturedRun` | 112 | strip-route gated |
| `ndsFTParamsInvalidateFighterParts` | 56 | |
| `__excpt_entry` | 48 | crash path — leave |
| `armDCacheFlushAll` | 40 | kernel path — leave |
| `__aeabi_fcmpun` | 28 | |
| `__excpt_vectors` | 24 | crash path — leave |
| `__aeabi_ul2f` / `__floatundisf` | 16 | |
| `armICacheInvalidateAll` | 12 | kernel path — leave |
| `__arm_excpt_und` / `_dabt` / `_rst` / `_pabt` | 32 | crash path — leave |
| `__aeabi_frsub` + `…fsub_golden` | 12 | **unreachable, §3** |
| **total** | **1,712** | |

Split by strength of evidence:

```text
   456 B  unreachable by construction (§3)          -- takeable with no argument
   788 B  silent in the window, non-kernel          -- takeable with a per-symbol argument
   468 B  crash / cache-maintenance / thread paths  -- LEAVE.  Gate-window silence is
                                                       not proof of unreachability, and
                                                       a crash handler is resident
                                                       precisely for the case that
                                                       never happens in a good match.
```

---

## 2. `ndsRendererScanList`: 23.8% of ITCM, and more than half of it cold

It had never been censused. It is the largest resident in the region by a factor of two.

| symbol | resident | code | executed | **cold** | cold % |
|---|---:|---:|---:|---:|---:|
| `ndsRendererScanList` | 7,728 | 7,620 | 3,508 | **4,112** | 54% |
| `ndsRendererNativePrepareProductionRun` | 3,720 | 3,608 | 1,552 | **2,056** | 57% |
| `ndsRendererSubmitHardwareTriangle` | 3,304 | 3,196 | 1,708 | **1,488** | 47% |
| `ndsRendererHardwareSubmitVertex` | 2,688 | 2,644 | 1,528 | **1,116** | 42% |
| `ndsRendererExecuteNativeFighterOwnerProduction` | 3,508 | 3,368 | 2,656 | **712** | 21% |
| **five renderer functions** | **20,948** | | | **9,484** | 45% |

**None of this is reclaimable by evicting a symbol.** All five are hot — 43–79% of their
bytes run every frame — so eviction would move their *executed* half into main RAM and pay
fetch on it. Reclaiming the cold half means **splitting cold basic blocks out of a live
function**, which is a source change with codegen risk, not a placement flip.

That is the opposite of the judgement `ANIM_ITCM.md` §2 made about `ndsR2AnimValueQ`'s 320
never-fetched bytes, and both are right: there, ITCM space had no claimant, so splitting
would have bought space and no fetch. Here there is a claimant queue (§5) and the space is
the binding constraint — so 9,484 B in five functions is the largest ITCM reserve in the
tree, and it is the one that costs engineering to reach.

**What this does NOT say.** A cold basic block is not dead code. These are switch arms,
format branches and error paths whose gate-window silence says only that this match does
not take them. Splitting one out must keep it linked and correct; it moves where it lives,
never whether it exists.

---

## 3. `__aeabi_frsub`: 456 B, unreachable by construction

The blob is three adjacent blocks that fall through into one another:

```text
01ff8020   8 B  __aeabi_frsub                         eor r0,r0,#0x80000000 ; b __addsf3
01ff8028   4 B  __subsf3 / __nds_task16_fsub_golden
01ff802c 444 B  __addsf3 / __nds_task16_fadd_golden
           456 B total, ending 01ff81e8
```

The port defines its own `fadd`/`fsub` at `0x01fff458` / `0x01fff454`, and the Task 16
`--redefine-sym` filter renames libgcc's originals to `*_golden`, which **by construction
have no caller**. `__aeabi_frsub` was never renamed, so it survives — and it falls through
into libgcc's adder. If it were ever called it would run libgcc's body rather than the
port's.

**It is not called. Three reads on `build-c219-animitcm-ship`'s linked image:**

1. `objdump -d` over the **whole** ELF: every reference to `frsub`, `addsf3`, `subsf3`,
   `fadd_golden` or `fsub_golden` is either a symbol header or a branch **internal to the
   blob** (`__addsf3+0x…`). There is no `bl` and no `b` into it from anywhere else.
2. A word scan of **every allocatable PROGBITS section** for the three addresses
   (`0x01ff8020`, `0x01ff8028`, `0x01ff802c`): **zero hits.** No function-pointer table,
   no literal pool, no dispatch entry reaches it.
3. Zero executed instructions across the 1,600-frame window.

Read 3 alone would be `[[gate-window-silence-is-not-unreachability]]`. Reads 1 and 2 are
static and make it a link-time property: **456 B that cannot execute in this image.**

**Mechanism to take it, already built:** add the member to `NDS_TASK9_FLOAT_MAIN_MEMBERS`
(`Makefile`), which keeps it extracted, `--redefine-sym`'d and in `$(OFILES)` and only skips
`--rename-section`. The suffix is the load-bearing part — `linker/nds_hot_text.ld:113`
matches `*.itcm.*` by **filename**, so the member must leave the pattern as
`<stem>.mainram.o` or nothing moves. `check-task9-float-itcm.ps1` reads each member's
placement from the build's own emitted object name and will need the new member declared.

---

## 4. The premise: measured, and it does not hold in the form it was stated

The objection under test: *every fixed-point candidate was built as ordinary `.text` in main
RAM while the float helpers it was compared against are ITCM-resident, so those A/Bs
measured warm float against cold fixed.*

`arm-placement.txt`, from `nm` on the linked ELF:

| pair | float arm | fixed arm | asymmetric? |
|---|---|---|---|
| **the 5.14× prior** | `guMtxCatF` `0x020356e8` **.main** | `ndsRendererMtxMul20p12` `0x01ff9dac` **ITCM** | **yes — toward FIXED** |
| camera Q20.12 | `syMatrixLookAtReflectF` `0x02034144` .main | `ndsR2CameraLookAtReflect20p12` `0x02098640` .main | no |
| camera projection | `syMatrixPerspFastF` `0x0203465c` .main | `ndsR2CameraPerspFast20p12` `0x020989f0` .main | no |
| sim leaf transform | `gmCollisionGetWorldPosition` `0x020824d8` .main | `ndsR2SimMacShadowTransform` `0x0208b4d8` .main | no |
| sim leaf compose | `ndsR2SimMacBaseCompose` `0x0207f020` .main | `ndsR2SimMacShadowCompose` `0x0208b9dc` .main | no |
| collision ring | `mpCollision*` `0x0205…`/`0x0206…` .main | not linked in the shipping build | no |

**The float bodies this campaign replaced are all in `.main`, exactly like the fixed bodies
that replaced them.** The single ITCM-resident member of any pair is the **fixed** arm of
the anchor prior. So `5.14×` is the one measurement with a placement asymmetry, and it
**overstates** what a fixed rewrite converts at — which strengthens, rather than weakens,
every "closed" verdict measured against it.

### 4.1 What *is* at `0x01ff…`

`__aeabi_fmul` (408 B), `__aeabi_fadd` (424), `__aeabi_fdiv` (352) and `__ieee754_sqrtf`
(236) are ITCM-resident. Those are the soft-float **library leaves** a float body *calls* —
not the float body itself. That is the observation the objection was built on, and it is
correct; the inference that the *bodies* differed in placement is not.

### 4.2 The real asymmetry, and why it does not reopen anything

A float body in `.main` executes its arithmetic **inside** those ITCM leaves: the arithmetic
is fetch-free and it pays `bl` overhead instead. A fixed body inlines the same arithmetic
into **its own `.main` bytes**, which are fetch-charged. So a float→fixed conversion trades
call overhead for compulsory instruction fetch, and the exchange rate depends on entry rate.

**That trade has already been measured directly, with a sign, twice, and both times against
fixed point:**

- `CAMERA_Q20_12.md` §6: inlining the fixed kernel's leaves (+3,032 B of `.main`) at 8.138
  entries/frame turned a **−4,736** paired median into **+1,600**. Same arithmetic, same
  binary family; only the fetched byte count changed.
- The collision ring's `K-ICACHE` null: the whole arithmetic win lost to compulsory fetch at
  0.97 entries/frame.

So the mechanism the objection names is real, it is already quantified, and it explains the
low measured rates **without** implying they were unfairly measured. **No closed lane
reopens on this evidence**, and this document does not ask for one to.

### 4.3 What a "warm re-test" would and would not prove

Putting a fixed body in ITCM would isolate arithmetic from fetch. It would **not** be a
symmetric test, because the float body it replaces is *already* not in ITCM — it would be a
new asymmetry favouring fixed, of exactly the shape §4 found in the 5.14×. And it is not a
deployable configuration: see §5.

---

## 5. The budget, and why the warm re-test was not run

```text
region 0x7fe0 = 32,736 B (linker/nds_hot_text.ld:18, ITCM minus the vectors)

smash64ds-battle-playable-tickhud-hwtri   (the measurement instrument)
   build-c219-animitcm-ship / build-c220-camship   .itcm 0x7f04 = 32,516     220 B free
   after taking the frsub blob                                               676 B free

smash64ds-battle-playable-proof-hwtri     (what Boundary builds and grades)
   Boundary manifest, this tree                     .itcm       30,164     2,572 B free
   after taking the frsub blob                                             3,028 B free
```

The particle/quad candidate the brief names needs, from the sizes of the bodies involved
(`syMatrixLookAtF` 712 B + `syMatrixOrthoF` 222 + `guMtxCatF` 140 = 1,074 B of float, and
the camera's fixed look-at came out at 1,148 B against a 988 B float original),
**1,100–1,500 B for the fixed body alone.** 676 B does not hold it, and it must be held on
the **instrument** — the tighter of the two targets — because that is the only target that
produces a tick number.

> **Record this as a structural constraint, not a one-off.** The measurement instrument has
> **220 B** free and the shipping ROM has **2,572 B**. A placement candidate can therefore
> fit the ROM that ships and still be unpriceable, because it does not fit the ROM that
> measures. `ANIM_ITCM.md` §6's Item B (268 B) sits exactly in that gap: it does not fit
> today's instrument, and it **does** fit after §3's eviction.

**So the honest outcome is a budget and a census, not a re-test.** The brief allowed for
this explicitly. What the next cycle inherits is in §6.

---

## 6. What this cycle did NOT do

- **The frsub blob was not evicted.** It is proven takeable (§3) and the mechanism is a
  one-line Makefile change, but freeing ITCM buys **zero ticks** on its own — the bytes are
  never fetched — and it needs its own build, its own Boundary, and a
  `check-task9-float-itcm.ps1` member declaration. It is banked as a budget, not spent.
- **No cold block was split out of a live renderer function.** §2 sizes the reserve at
  9,484 B in five functions and says why it costs engineering rather than a flag.
- **The warm re-test was not run** (§5: it does not fit the instrument), and §4 argues it
  would not settle what it was proposed to settle.
- **No lane was reopened.** The bias is real, mis-located, and already priced with a sign.
- **`ndsR2AnimValueQ` was not re-censused.** `ANIM_ITCM.md` §2 already has it per cache line
  (162 of 257 slots, 320 B never fetched); it is folded into §1's totals via c219's
  inventory, not re-derived.
- **The 468 B of crash, cache-maintenance and thread paths were not touched**, and should
  not be.

---

## 7. Reproduction

```powershell
arm-none-eabi-objdump -d --section=.itcm `
  builds\build-c200-trackprof-off\smash64ds-battle-playable-tickhud-hwtri.elf > c200-itcm.dis
python census.py c200-itcm.dis ..\2026-08-15_ftanim-dispatch-attribution\c200-off-pc.csv
```

§3's reads are `objdump -d` over the whole ELF grepped for the three names, and a word scan
of the allocatable PROGBITS sections for their addresses. §4 is `nm -S --defined-only`.
**No build and no emulator run is required for any of it.**
