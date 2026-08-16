# The fighter draw contract is memoised: −12,864 at rank-80, same binary, geometry bit-identical

**Date:** 2026-08-16.
**Build:** `build-c223-ftrmemo`, target `smash64ds-battle-playable-tickhud-hwtri`,
`NDS_R2_BATTLEPACK=1 NDS_R2_BATTLEPACK_KEEP_CACHE=1 NDS_R2_BOTH_CPU=1`, mode 163,
DLDI ON. **ONE ELF, `sha=DE80E46BDCF1FD98`, TWO ARMS**, differing in exactly one
poked `.data` word — `gNdsFtrDrawMemoRoute.route`, 0 = control (the walk runs
every capture), 1 = candidate (the memo). No cross-build placement term.
1,600 samples, frames 440–2039, `slips=0` on both arms.
**Prior basis:** `build-c220-camship`, rank-80 **1,210,624 raw / 1,185,677 net**,
band 41–120 **1,218,356**, **REQUIREMENT +65,297**.
**UNITS: 2 profile cycles = 1 project tick.** Apparatus 24,947, gate 1,120,380.

**Status: 1 lab build, 2 whole-match runs of my own, 1 Boundary. 1 default
introduced (`NDS_R2_FTR_DRAW_MEMO ?= 1`), nothing published, `decomp/`
byte-pristine and no new import-overlay input.** Both root ROMs SHA-256
identical before and after (`root-roms-before.txt` / `root-roms-after.txt`),
`54C07FAC…` / `6C939434…`.

---

## 0. The level

```text
CONTROL   rank-80 1206272 raw / 1181325 net   level +60945   band 41-120 1216202
CANDIDATE rank-80 1193408 raw / 1168461 net   level +48081   band 41-120 1200254
MOVED     12864 at rank-80 on this binary        (price.py, price.txt)
```

**NEW BASIS `build-c223-ftrmemo` route 1 — the shipping configuration, since the
flag defaults to 1: rank-80 1,193,408 raw / 1,168,461 net, band 41–120
1,200,254, REQUIREMENT +65,297 → +48,081.**

The 17,216 between the two bases is **not one measurement**:

| term | size | how it was measured |
|---|---:|---|
| the memo | **12,864** | same binary, one poked word, **floor-free** |
| the `CountFlags` deletion | 4,352 | this binary's CONTROL against `c220` — **cross-build, inside the ≥14,080 floor** |

Only the first is a price. The second is this binary's control arm sitting 4,352
under the `c220` basis for identical memo behaviour, which is the expected
signature of removing `ndsFighterDisplayContractCountFlags`
(`FTR_LANE.md` profiled it at **4,117**) — right sign, 106% of size, but a
cross-build residual and therefore corroboration, not a banked figure.

## 1. The memo's own price, paired

The instrument is bit-deterministic and both arms are the same ROM and the same
seed, so row *i* is the same presented frame on both. **Compare frames, not
ranks:**

| | control | candidate | delta | frames won |
|---|---:|---:|---:|---:|
| `WORK-H` paired median | — | — | **−12,992** | **1,488 / 1,600 (93.0%)** |
| `FTR` paired median | — | — | **−13,312** | **1,578 / 1,600 (98.6%)** |
| `WORK-H` rank-80 | 1,206,272 | 1,193,408 | −12,864 | |
| `WORK-H` band 41–120 | 1,216,202 | 1,200,254 | −15,949 | |
| `FTR` P50 | 290,464 | 276,800 | −13,664 | |
| `FTR` band 41–120 | 298,682 | 292,832 | −5,850 | |

**Nothing else moved.** `STG` rank-80 −64, `ALL` −64, `SRC` +960 — all inside the
64-tick quantum or its own noise. The change is confined to `FTR`, which is where
the deleted work lives.

The ≥14,080 figure is the **cross-build** placement floor and does not apply to a
same-binary poked-word pair; the comparable same-binary floor is the ~5,440
paired median `CAMERA_SHIP.md` measured, and this is 2.4× it with 93% of frames
winning.

**The 2^22 sampler filter ran live and was load-bearing again:** the control arm
carried **3** corrected rows (frames 954, 1333, 2003) and the candidate **0**.
Uncorrected, the pair would have carried three spurious 4,194,304-tick rows on
one side only. Both CSVs are post-correction (trailing `WRAPFIX` column).

## 2. Engagement, against the census's predicted 98.75%

`memo-on-run.log` / `memo-off-run.log`, `extras:` lines:

| counter | candidate | control |
|---|---:|---:|
| `gNdsFtrDrawMemoBoundary` | 3,914 | 2 |
| `gNdsFtrDrawMemoHits` | **3,765** | **0** |
| `gNdsFtrDrawMemoInvalidations` | **147** | 0 |
| `gNdsFtrDrawMemoFills` | 149 | 2 |
| `gNdsFtrDrawMemoBypass` | 164 | 4,076 |
| `gNdsFtrDrawMemoReplayEvents` | 60,462 | 0 |

**Three identities that could have failed and did not:**

1. `Boundary + Bypass = 3,914 + 164 = 4,078 = 2 × 2,039` presented frames
   **exactly** — the same bootstrap arithmetic the census used, so the memo saw
   every capture of the match.
2. `Hits + Fills = 3,765 + 149 = 3,914 = Boundary`, and
   `Fills = Invalidations + 2` (one first fill per slot).
3. **`Bypass = 164` is exactly the census's `ZeroEvents = 164`** — the 142
   magnify and 22 `is_invisible` early returns. The memo never arms on a capture
   that produced no events, by construction rather than by tuning: on those paths
   the head returns before it emits its fog colour, which is the boundary.

**The control arm is a real negative control**: 0 hits, 0 replayed events, 4,076
bypasses. It can fail and does not.

**Hit rate 3,765 / 3,914 = 96.19%, against the census's contract-change rate of
98.75%.** The 147 invalidations against the census's 51 contract changes are
**96 deliberate false misses**: the key is strictly stronger than the contract
hash, because it also carries `fp->lr`, `shade`, `costume`, `detail_curr`,
`is_modelpart_modify`, `colanim.skeleton_id`, `fp->attr` and the tree root
pointer — state that can change without changing the contract. A false miss costs
one walk and cannot cost correctness. At ~9,650 tk per capture-walk over 2,039
frames those 96 are worth **≈454 tk/fr**, and buying them back means weakening the
key, which is not obviously the right trade.

## 3. Equivalence: every geometry counter is identical to the tick

Whole match, both arms, from the same `extras:` lines:

```text
gNdsFighterDisplayContractSelectedCount        62,952 == 62,952
gNdsFighterDisplayContractSubmittedCount       62,952 == 62,952
gNdsFighterDisplayContractLightDirectionCount   4,010 ==  4,010
gNdsFighterDisplayContractBoundsPassCount       3,823 ==  3,823
gNdsFighterDisplayContractBoundsFailCount         142 ==    142
gNdsFighterDLAllDrawP0HardwareTriangleCount   600,000 == 600,000
gNdsFighterDLAllDrawP1HardwareTriangleCount   623,934 == 623,934
gNdsStageGCDrawAllLoopHardwareFighterTriangleCount 1,223,934 == 1,223,934
```

`SelectedCount` is identical because a hit adds its replayed event count to it —
the contract still selected those display lists, only the derivation was
replayed. `BoundsFailCount = 142` reproduces the census's 142 on a different
build, so the magnify branch still fires exactly as often: **the head is
untouched, which was the whole point.**

**Boundary PASSED, exit 0, 0 `Exception:` over 316,161 lines**
(`boundary.log`), `DECOMP_PRISTINE=PASS pinned_historical_files=10 ds_markers=0
decomp_patch_pipeline=absent`. Its pacing smoke is **exact** against the previous
seven runs on every counter:

```text
ftrContract=6784/6784/geom0x222005/cycle0x100000/rm0xc4112078/light424/424/bounds424/0
ftrTri=132712/p067840/p164872/own424   binds=54 vtx=2484 tri=828
itcm=30164/32768 free=2604   renderer=12896
```

**One number in that line moved, and it is the one that must:**
`ticks=294353408 → 294160832`, **−192,576 (0.065%)**. That is the smoke's
VBlank-quantised wall time, not a geometry counter. A ROM that provably does less
work per frame and still reported an identical tick total would be the suspicious
result; the seven-run streak on `ticks` ends here **by design**, and every
counter that describes what was drawn is bit-identical.

## 4. Boot headroom — measured, not inherited

`gNdsTaskmanGeneralHeapFreeMin = 53,136` on **both arms**, i.e. **27,536 bytes of
margin above the 25,600 `ifCommonSetMaxNumGObj` latch**, with the cache already
in the binary. `[[ram-is-not-free-gobj-cap]]`'s 24,404 is the stale pre-arena
figure; `ARENA_PRICE.md`'s 2026-08-15 stress battery read 52,400, and this
build's 53,136 agrees with that, not with the memory index.

The memo's static cost, from `nm -S` on the linked ELF:

```text
gNdsFtrDrawMemoRoute      D 0x020e9640  32 B  (.data, owns its cache line)
sNdsFtrDrawMemo           b 0xa90 = 2,704 B   (2 x [16-word key + 32 events + 32 preambles])
sNdsFtrDrawMemoStub       b 0x88  =   136 B
sNdsFtrDrawMemoKey        b 0x40  =    64 B
five scalars              b            20 B
ndsFighterDisplayContractHeadBoundary  T 0x1cc = 460 B of .main
```

**≈2,924 B of bss and 460 B of code**, against 27,536 B of margin. The 2.6 KB the
design predicted was right to the struct.

**The route's placement needed a second build and the reason is worth keeping.**
A bare `volatile u32 … aligned(32)` is **not** enough: `aligned(32)` fixes the
start, not the size, and the linker packed `gNdsFtrPlanVerify`,
`gNdsFtrPlanRoute`, `sLastSceneCurr.0` and a 16-byte `memo` into the same line —
**the exact line `diagnostics.c` already records a failed poke in.** The route is
now a 32-byte cell (`gNdsFtrDrawMemoRoute.route`) that owns the whole line, and
`nm` confirms `020e9640 00000020 D`. The sampler's `-SetGlobals` grammar accepts
a dotted field path and rejects array subscripts, which is why it is a struct and
not a `[8]`.

## 5. The mechanism, and why it is not where the brief expected

The design brief specified redirecting `ftDisplayMainDrawAll` "through the
existing import shim". **That is not expressible.** The decision cannot be taken
before the head runs — the key *is* the head's output — and it must be taken
before the walk, but head and walk are welded inside one decomp function, and the
preprocessor cannot rename a definition apart from its uses. `ftDisplayMainDrawAll`
is defined at `ftdisplaymain.c:929` and called only at `:1220` and `:1226`,
inside the same translation unit, so any `#define` renames the definition and both
call sites together. `ld --wrap` does not reach a reference the defining object
resolves itself.

**The brief's fallback — an import-overlay patch — is forbidden by the repo.**
`Makefile:2424-2427`: *"New adaptations belong directly in src/import/src/port and
must not be added to this list."* Followed the Makefile. `decomp/` is untouched,
`DECOMP_PRISTINE=PASS`, and `scripts/import-overlays/battleship/` still holds the
same eight patches.

What is available is the head's **last contract-visible action**.
`ftDisplayMainProcDisplay:1211-1213` emits exactly one fog colour before the
walk, and `ftdisplaymain.c` holds only three `gDPSetFogColor` sites (`:668`,
`:676`, `:682`), all inside `ftDisplayMainSetFogColor` / `…DecideFogColor`, all
with `fp` in scope — and `gDPSetFogColor` is a macro **the import shim owns**. The
shim calls `ndsFighterDisplayContractHeadBoundary` from it; the renderer makes it
one-shot per capture, so the walk's own `ftDisplayMainDecideFogDraw` cannot
re-arm it.

On a hit the walk is **collapsed, not skipped**: `gNdsFtrDrawMemoSkipRoot` is
pointed at this fighter's live root DObj and the shim's `DObjGetStruct` hands
`ftDisplayMainDrawAll` an empty HIDDEN stub, so
`ftDisplayMainDrawDefault(stub)` is a flag test and a NULL sibling test.
**Nothing in the live tree is written.** Disarmed, `SkipRoot` holds the stub's own
address, so no live root can match and the expansion is one load and one compare.

## 6. Soundness: what the key covers, and the two states it refuses

The key (16 words, built at the boundary) carries the head's own contract output
(geometry mode, cycle type, render mode, prim/env colour, an FNV of the `Light`,
its valid flag and count), the two head statics the walk reads
(`sFTDisplayMainSkyFogAlpha`, `sFTDisplayMainIsShadeFog`), and the `fp` state the
walk and `ftDisplayMainDrawAll` read directly, plus the tree root pointer and
`fp->attr` so a rebuilt fighter invalidates by construction.

**The per-DObj state the walk also branches on — `dobj->dl`/`dls`/`dv`/`flags`
and `FTParts` flags — is not hashed, and does not need to be in this build.**
This is the piece `CAPTURE_MEMO.md` §6 proposed to close with a dirty-set hook in
`ftparam.c`, and that hook is **unnecessary**: decomp's `ftparam.c` is not
compiled at all. `ftParamSetModelPartID`, `ftParamResetModelPartAll` and
`ftParamHideModelPartAll` are port bodies in
`src/port/reloc_backend_compat_shims.c` (`:6708`, `:1467`, `:1602`) which
**deliberately leave `joint->dl` alone** — their own comment says so — and move
only `fp->modelpart_status` and `fp->is_modelpart_modify`, the latter of which is
in the key. And no `DOBJ_FLAG_HIDDEN` writer in the compiled tree touches a
fighter joint: every one is an item, a weapon, the stage or a menu
(`grsector.c`, `it*/`, `mnmaps.c`, `scexplain.c`, `wplinkboomerang.c`).

Two states are **refused outright** rather than keyed, because for them a
collapsed walk would be wrong rather than merely stale:

- a `display_mode` other than `nDBDisplayModeMaster` — the MapCollision and
  hit-outline blocks (`:1245-1311`, `:1314-1325`) re-read `DObjGetStruct` **after**
  the walk, so the stub must not still be armed there;
- a pending afterimage draw (`fp->afterimage.drawstatus >= 2`) —
  `ftDisplayMainDrawAfterImage` runs inside `ftDisplayMainDrawAll` and builds
  fresh scratch geometry every frame, so its event list is not replayable.

Both refusals show up as `Bypass`/invalidation, never as a wrong draw.

## 7. What was NOT done

- **The 4,901 tk/fr of memo overhead was not attacked.** At the measured 96.19%
  the gross ceiling is 0.9619 × 19,300 = **18,565**; `FTR` P50 moved **13,664**.
  The gap is the 16-word key build and compare, the 460 B of new cold code, and
  **1,280 B of `memcpy` a frame** copying the cached events and preambles back
  into `sNdsFighterDisplayContract` — in a lane that is 28.9% D-cache fill. The
  copy is removable: the submit path is per-slot and runs immediately after the
  capture for that slot, so the consumer could read the slot cache directly
  instead. Not built, not sized by measurement.
- **The 96 false misses were not bought back.** Naming which key word fires them
  (`fp->lr` on every turnaround is the obvious suspect) needs a per-word counter,
  which is one more instrumented build.
- **`CountFlags` was gated, not deleted.** Its body now compiles only under
  `NDS_R2_FTR_CONTRACT_CENSUS`, because that census needs the walk for its
  candidate tree key. The two globals stay defined so
  `verify-battle-mariofox-gcrunall-loop-harness.ps1:2065` and `probe-ko-vfx.ps1`
  still link; both now read 0 and neither asserts on them.
- **`gmCameraLookAtFuncMatrix`'s 5,143 rider** (`FTR_LANE.md` §5) untouched.
  **`STG`** (P50 175,424) untouched. No closed lane reopened.
- Nothing published; `smash64ds.nds` and `smash64ds-battle-playable-hwtri.nds`
  are byte-identical before and after.

## Reproduce

```powershell
make TARGET=smash64ds-battle-playable-tickhud-hwtri BUILD=build-c223-ftrmemo `
     NDS_R2_BATTLEPACK=1 NDS_R2_BATTLEPACK_KEEP_CACHE=1 NDS_R2_BOTH_CPU=1
# candidate (route defaults to 1)
pwsh -File scripts\sample-tick-hud-buckets.ps1 -NoBuild -Build build-c223-ftrmemo `
     -RingDump -Samples 1600 -StartFrame 439 -TimeoutSeconds 5400 `
     -RowsCsv artifacts\performance\2026-08-16_ftr-draw-memo\memo-on-rows.csv `
     -JsonOut artifacts\performance\2026-08-16_ftr-draw-memo\memo-on.json `
     -ExtraGlobals gNdsFtrDrawMemoHits,...,gNdsTaskmanGeneralHeapFreeMin
# control: same command plus
#     -SetGlobals gNdsFtrDrawMemoRoute.route=0
python artifacts/performance/2026-08-16_ftr-draw-memo/price.py
cmd /c "pwsh -NoProfile -File scripts\verify-all.ps1 -Profile Boundary > log 2>&1"
```
