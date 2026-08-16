# The per-joint setup is not redundant with the frame's other work — the renderer *calls* it — and at literally zero cost it is 10,110 at rank-80; the load-time resolver is the same 3,542 as the fast resolver, because moving work off the frame changes when it happens and not how big it is

**Date:** 2026-08-16 · **Branch:** `codex/r2-runtime2` · **base HEAD `08bb139e481`**
**0 lab builds, 0 emulator runs of my own, 0 production source edits, 0 defaults
flipped, 0 ROMs published, both root ROMs byte-unchanged and not rebuilt.**
Every number is re-derived from artifacts already in the tree plus BattleShip and
port source.
**UNITS: 1 project tick = 1 `cpuGetTiming()` tick = 2 ARM9 cycles.** Every table
states its window.

```text
REQUIREMENT  +65,297 net ticks per presented frame at rank-80.  BASIS
             build-c220-camship, rank-80 1,210,624 raw / 1,185,677 net against
             the 1,120,380 gate (apparatus 24,947), band 41-120 1,218,356;
             shipping renderer (GX_COMPOSE 0), mode 163 one-minute match,
             NDS_R2_BOTH_CPU=1, 1,600 samples, frames 439-2038, DLDI ON,
             slips=0.  REPRODUCED: lanes.py prints the control before any
             result, and shdt_size.py re-derives both bases.  UNMOVED --
             this cycle banks nothing.

ANSWERED     THE REDUNDANCY DOES NOT EXIST, AND THE SHARING ALREADY DOES.
             The renderer does not build a competing fighter joint matrix: it
             CONSUMES the collision chain's.  ndsRendererAdapterBuildJointAttach-
             Mtx CALLS func_ovl2_800EDBA4 and reads parts->mtx_translate out of
             it; ndsRendererAdapterBuildFighterPartsMtx reads
             parts->unk_dobjtrans_0x10 gated on the same transform_update_mode
             latch.  One producer, two consumers, memoised by the source's own
             four dirty flags.  The memo the brief looked for IS the dirty flag.
             And the DIRECTION is the other way: func_ovl2_800ED490 is entered
             22.77 times on an engaged frame against 0.76 on a control frame, so
             the renderer drives under one compose a frame and hit detection
             drives all 22.01 of the rise.  The frame has NOT already built these
             matrices when hit detection asks -- it builds them BECAUSE hit
             detection asks.  Section 1.

SIZED        AND IT WOULD NOT MATTER IF IT HAD.  The whole per-joint setup --
             local matrix, world compose, 3x3 cofactor inverse, axis scales and
             their trig, 70,660 tk/fr on the 57 engaged frames = 68.6% of the
             chain -- at LITERALLY ZERO COST is 10,110 at rank-80, level +65,297
             -> +55,187, band 1,197,943.  That is 15.5% of the requirement and it
             is UNDER the >=14,080 cross-build floor.  Flat across the whole
             plausible fraction range (8,780 at f=0.15 .. 10,110 at f=0.245), so
             the answer does not depend on the estimate.  Even the WHOLE chain
             free is 18,954.  Section 2.

REFUSED      THE LOAD-TIME RESOLVER IS THE SAME 3,542, AND THAT IS THE POINT.
             Moving ndsRelocAssetIDForToken off the gameplay frame removes
             4,118 tk/fr from the 288 event frames -- arithmetically identical to
             making it free, because rank-80 is an order statistic over the
             window's 1,600 GAMEPLAY frames and a match load sits outside it.
             convert.py prints ONE row for both: 3,542, level +61,755.  The
             file's second clause names a MECHANISM, not an exemption from its
             first clause's SIZE, and E11's recorded failure was a MEASUREMENT
             failure that the mechanism does not touch.  A ~1.5 KB runtime table
             makes the change byte-POSITIVE where E11's was byte-NEGATIVE and
             still lost 15,744 at P95.  NOT BUILT.  Section 3.

CORRECTED    MY BRIEF CALLED SHDT "THE LARGEST SINGLE ROW ON THE BOARD".  On
             this basis, ranking every lane by its own exact ceiling, SITR's
             excess is 86,528 (level -21,231) and SHDT's is 68,928 (-3,631):
             SITR is larger and is the only lane whose excess alone closes the
             gate.  The brief's ordering (SHDT 50,240 > SITR 45,056) is the
             CLUSTER rows -- dominant-excess-owner subsets -- not the lanes.
             docs/HANDOFF.md line 5 already records "DO NOT BRIEF SHDT AS AN
             88%-OF-THE-GAP LEVER AGAIN"; the repo record wins.  Section 4.

NEW          THE MEDIAN FRAME PASSES THE GATE BY 218,767.  The sixteen lane
             medians sum to 926,560 against a raw gate of 1,145,327, so the
             entire remaining +65,297 is EXCURSION above that baseline -- and
             the smallest proportional cut of a lane that closes it is FTR 22%,
             STG 38%, SITR 39%, MISC 49%, GCRARES 67%, SPHD 69%, SHDT 80%, and
             NEVER for the other nine, deletion included.  Section 4.
```

---

## 1. The question, answered from the port's own source

**The brief's hypothesis:** *"The fighter's joints already have world matrices for
rendering and for the animation attach. If the hit-detection setup rebuilds what
another subsystem built the same frame, that is a memo or a share."*

It is the right question and its answer is no, for a reason stronger than a
measurement: **the port's renderer is a consumer of the collision chain's
matrices, not a second producer of its own.**

### 1.1 The producer, and its four latches

`decomp/BattleShip-main/decomp/src/gm/gmcollision.c`. `FTParts` carries four
dirty flags and each guards exactly one product:

| flag | guards | built by |
|---|---|---|
| `transform_update_mode` | `unk_dobjtrans_0x10`, the joint's **local** matrix | `gmCollisionTransformMatrixAll` (`:29-79`) — 6 trig + up to 9 scale multiplies |
| `unk_dobjtrans_0x5` | `mtx_translate`, the joint's **world** matrix | `func_ovl2_800EDBA4` (`:332-452`) walks to the nearest valid ancestor, then `func_ovl2_800ED490` (`:208-225`) composes back down, 36 `fmul` + 27 `fadd` per level |
| `unk_dobjtrans_0x7` | `unk_dobjtrans_0x9C`, the **inverse** | `gmCollisionSetInvertMatrix` (`:228-278`), 3×3 cofactor + reciprocal, 61 float ops |
| `unk_dobjtrans_0x6` | `vec_scale`, the **axis scales** | `func_ovl2_800EDE5C` (`:472`), 3 × (3 `fmul` + 2 `fadd` + 1 `sqrtf`) |

`func_ovl2_800EDE00` (`:455-469`) is the setup entry hit detection calls, and it
is nothing but those latches: `if (unk_dobjtrans_0x7 == 0) { if
(unk_dobjtrans_0x5 == 0) func_ovl2_800EDBA4(...); gmCollisionSetInvertMatrix(...);
unk_dobjtrans_0x7 = 1; }`.

### 1.2 The renderer reads those same products

**`src/port/reloc_backend_renderer_dl.c:1464-1499`**,
`ndsRendererAdapterBuildJointAttachMtx` — the `NDS_RENDERER_ADAPTER_JOINT_ATTACH_MTX_KIND`
matrix builder:

```c
    func_ovl2_800EDBA4(attach);
    sNdsRendererAdapterMvpRecalcScaleX =
        sqrtf((parts->mtx_translate[0][0] * parts->mtx_translate[0][0]) + ...);
    if (ndsRendererAdapterF2LFixedWExact(&parts->mtx_translate, &mtx) == FALSE)
        syMatrixF2LFixedW(&parts->mtx_translate, &mtx);
    ndsRendererAdapterMtxFromN64(&mtx, out);
```

It calls **the collision chain's own world walk** and then quantises
`parts->mtx_translate` float → N64 16.16 → DS 20.12. It does not build a world
matrix; it converts the one that already exists.

**`:1247-1306`**, `ndsRendererAdapterBuildFighterPartsMtx` — the
`NDS_RENDERER_ADAPTER_FIGHTER_PARTS_MTX_KIND` builder — is gated on
`if (parts->transform_update_mode != 0)` and reads `parts->unk_dobjtrans_0x10`,
the **local** matrix `gmCollisionTransformMatrixAll` writes. Same relationship,
one level down.

> **So the 20.12 matrices are downstream of the float ones, not parallel to
> them.** `BAND_OWNER.md` §4's row *"reuse the renderer's joint matrices — the
> renderer builds every joint's world matrix each frame in 20.12"* describes a
> producer that does not exist. There is nothing to substitute in; the substitution
> already happened, in the only direction that is available.

### 1.3 And the drive count says the same thing, quantitatively

If the renderer forced the world build every frame for every joint, the setup
would be free to hit detection and `func_ovl2_800ED490` would run ~14 times on an
*ordinary* frame. `SHDT_MECHANISM.md` §3.1's engaged-57 leaf attribution divides
out to exact invocation counts (1,434.7 leaf calls engaged and 47.9 control, at a
source-checked 63.01 leaf calls per invocation):

| | engaged frame | control frame | rise |
|---|---:|---:|---:|
| `func_ovl2_800ED490` invocations | **22.77** | **0.76** | **+22.01** |

**0.76.** The renderer's joint-attach kind drives under one compose a frame; the
entire rise belongs to hit detection. The frame has not already built these
matrices when hit detection asks — **it builds them because hit detection asks**,
and the renderer then gets whichever ones it needs for free off the same latch.

### 1.4 Two configuration facts, so nobody re-derives them

`builds/build-c220-camship/nds_build_config.h` reads `NDS_R2_SIM_MAC_SHADOW 0`
and `NDS_R2_COLLISION_FIXED 0`. So on the basis ROM:

- `src/import/battleship_gmcollision.c:226-316`'s shadow wrappers around
  `func_ovl2_800ED490` and `gmCollisionGetWorldPosition` — each of which carries
  an unconditional counter increment — are **compiled out**. The shipping ROM
  runs the decomp bodies unwrapped, so `SHDT_MECHANISM.md`'s c191 attribution
  applies with no apparatus correction. (That file's own comment already records
  those wrappers measured *"0.082 entries a frame for the transform and EXACTLY
  ZERO for the compose"*.)
- `src/port/nds_r2_collision_ring.c:253-323`'s `ndsR2CfxPrepareFighterJoint` —
  which is **already a fixed-point implementation of exactly steps 3 and 4**
  (`ndsR2CollisionFixedInvertF32`, `ndsR2CollisionFixedAxisScalesF32`) — is also
  compiled out. `../2026-08-15_cfx-narrow-exchange/EXCHANGE.md` measured that lane
  at an exchange rate of **2.68** (it costs more) with a 0.47× ceiling at zero.
  **Not reopened.**

### 1.5 One consequence for a route that was already declined

`SHDT_MECHANISM.md` §4.2 declined a fixed-point `func_ovl2_800ED490` as a
gameplay-fidelity change worth 0.103× of the requirement. §1.2 above **widens**
that: `parts->mtx_translate` is also what the renderer quantises for the
joint-attach matrix kind, so a fixed-point rewrite would move drawn geometry as
well as hits. It is more blocked than it was, not less. Recorded, not proposed.

---

## 2. And the size, which closes it without needing §1 at all

`SHDT_MECHANISM.md` §2's natural experiment proved the setup is **per engaged
frame** — 44-pair frames run exactly 2.00× the pair tests of 22-pair frames while
every per-joint symbol stays flat at 0.93–1.04× — so the setup lives in the
`SHDT` **excursion** (bucket minus its own run P50), not in the flat baseline.

Its size, from that document's engaged-57 leaf attribution (self + leaf tk/fr on
the 57 engaged frames of `build-c191-sitr-profile-c185`):

| setup component | self | leaf | total |
|---|---:|---:|---:|
| `func_ovl2_800ED490` — world compose | 7,006 | 20,696 | 27,702 |
| `gmCollisionSetInvertMatrix` — 3×3 cofactor | 5,301 | 9,329 | 14,630 |
| `func_ovl2_800EDE5C` — axis scales, 3 `sqrtf` | 1,509 | 9,686 | 11,195 |
| `gmCollisionTransformMatrixAll` — local matrix | 5,060 | 5,791 | 10,851 |
| `lbCommonSin` + `lbCommonCos` — its trig | 3,805 | 2,477 | 6,282 |
| **SETUP** | | | **70,660** |
| *(the consumers, excluded: `GetWorldPosition` 14,737 + `TestRectangle` 11,880)* | | | |
| *(whole chain, for scale)* | 35,277 | 67,712 | *102,988* |

70,660 × 57 frames = 4,027,620 tk against the basis's own `SHDT` excursion of
16,416,192 tk = **f = 0.245**. Exact re-rank of the basis's 1,600 rows:

| intervention | rank-80 | moved | band 41–120 | **level** |
|---|---:|---:|---:|---:|
| *(control)* | 1,210,624 | — | 1,218,356 | **+65,297** |
| the whole per-joint SETUP free (f = 0.245) | 1,200,514 | **10,110** | 1,197,943 | **+55,187** |
| setup excluding trig (f = 0.224) | 1,200,519 | 10,105 | 1,199,852 | +55,192 |
| the whole CHAIN free (f = 0.358) | 1,191,670 | 18,954 | 1,188,160 | +46,343 |
| `SHDT` down to its own P50, every frame | 1,141,696 | 68,928 | 1,146,876 | −3,631 |
| the WHOLE `SHDT` bucket, every frame | 1,137,088 | 73,536 | 1,142,335 | −8,239 |

**10,110 is 15.5% of the requirement and it is under the ≥14,080 cross-build
floor.** The estimate is not load-bearing — the curve is flat across the whole
plausible range:

```text
f=0.150  moved  8,780     f=0.245  moved 10,110     f=0.350  moved 17,548
f=0.200  moved 10,099     f=0.300  moved 14,688     f=0.500  moved 39,424
```

To reach even the floor the setup would have to be **f ≈ 0.30**, i.e. 122,000
tk/fr on an engaged frame — more than the whole measured chain. And the 70,660 is
already **generous on the current basis**: `func_ovl2_800EDE5C`'s 11,195 is
c191-era, and `../2026-08-16_hwmath-route/HWROUTE.md` has since banked `sqrtf`
into ARM state with its leading `SQRTCNT` poll deleted (−12,416 at rank-80), so
part of that row is already collected.

> **The answer to Item A is therefore doubly closed: the redundancy does not
> exist (§1), and the work it would have removed does not reach the measurement
> floor even if it were free (§2).** No build was spent and none should be.

---

## 3. Item B — why the second clause does not lower the bar

`src/port/reloc_backend_assets.c:1876-1921` states it verbatim:

> *"Do not bring another small load-frame cut. Either remove this work in one
> change large enough to clear ~16,000 of tail movement, or move it off the
> gameplay frame entirely, which changes WHEN the work happens instead of
> shuffling where the code sits."*

The brief read the second clause as an alternative that exempts a small change
from the first clause's size. **The arithmetic says it is not**, and the reason is
mechanical:

`ndsRelocAssetIDForToken` costs **4,118 tk/fr on the 288 event frames and 0 on the
other 1,312** — `../2026-08-16_sitr-excursion/attribution-event288.txt` line 28
(`4118 / 0`) and line 85 (`2.61 calls/frame event / 0.00 rest`). Rank-80 is an
order statistic over the window's **1,600 gameplay frames**, frames 439–2038, and
a match's own load happens before frame 439. **Work moved to load time leaves the
ranked window entirely — which is the same subtraction as work made free.**
`convert.py` prints one row for both:

| | rank-80 | moved | level |
|---|---:|---:|---:|
| make the resolver free | 1,207,082 | 3,542 | +61,755 |
| move the resolver to load time | 1,207,082 | 3,542 | +61,755 |

**3,542 is 22% of the file's own ~16,000 bar and 25% of the ≥14,080 cross-build
floor.** The clause distinguishes the *mechanism* of a saving; it does not change
its *magnitude*, and the failure it was written from was a **measurement**
failure: E11's change was provably identical, added **negative** bytes, cut the
function 7,667, kept the load-frame set bit-identical — and still read `WORK-H`
P95 **+15,744**, P99 +59,200, over-gate 9 → 11, against control-to-control noise
of ±5,376. Placement noise beat a larger effect than this one.

And the byte ledger runs the wrong way. `ndsRelocFileID` is
`return (u32)(uintptr_t)file_id`, so every key is a link-time address and a
resolved table must be built at run time: ~411 entries ≈ **1.5 KB** of
`.data`/`.rodata` in main RAM. E11 lost while removing bytes; this proposal adds
them, against `[[ram-is-not-free-gobj-cap]]`'s heap low-water of 24,404 versus the
25,600 GObj-cap threshold, and Task 74's own postmortem — *"three lookup arrays in
`.main.bss` are not [resident]"* — is that objection already measured once.

**Verdict: NOT BUILT.** Shipping it would put an unmeasurable change into the
tree, and the brief's own rule — size every candidate on the conversion curve
before building it — is what rejects it. It remains correct to take **if it ever
rides along with something large**, exactly like `ATTACH_LANE.md` §3.1's 298-tick
`AObjToQConvert` store.

---

## 4. The board correction, and what the ranking actually says

`lanes.py` re-ranks the basis's own 1,600 rows for **every** leaf lane under three
interventions. `FRACTION` (remove f × lane[frame] from every frame) is what a
cheaper implementation produces; `EXCESS` (remove everything above the lane's own
run P50) is a per-frame variable and is a **ceiling**, which is the exact error
`ATTACH_LANE.md` §1 corrected on `SITR`.

| lane | P50 | band 41–120 | × P50 | **smallest f that closes** | EXCESS moved | level | WHOLE moved | level |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| `SITR` | 104,320 | 210,060 | 2.01 | **39%** | **86,528** | **−21,231** | 184,704 | −119,407 |
| `SHDT` | 4,608 | 82,239 | 17.85 | 80% | 68,928 | −3,631 | 73,536 | −8,239 |
| `SPHD` | 72,288 | 102,233 | 1.41 | 69% | 31,776 | +33,521 | 99,584 | −34,287 |
| `MISC` | 107,872 | 137,152 | 1.27 | 49% | 26,368 | +38,929 | 133,760 | −68,463 |
| `GCRARES` | 81,632 | 95,895 | 1.17 | 67% | 13,600 | +51,697 | 95,232 | −29,935 |
| `SCPU` | 53,696 | 57,455 | 1.07 | never | 10,880 | +54,417 | 44,352 | +20,945 |
| `SPRM` | 2,112 | 12,807 | 6.06 | never | 10,176 | +55,121 | 12,288 | +53,009 |
| `AUD` | 2,688 | 20,938 | 7.79 | never | 6,528 | +58,769 | 9,216 | +56,081 |
| **`FTR`** | **290,432** | 290,400 | **1.00** | **22%** | 4,224 | +61,073 | 294,528 | −229,231 |
| `STG` | 175,424 | 177,477 | 1.01 | 38% | 2,624 | +62,673 | 178,048 | −112,751 |
| `OTHRW` | 19,776 | 19,900 | 1.01 | never | 384 | +64,913 | 20,160 | +45,137 |
| `SCAT` · `SWRM` · `SRCRES` · `SPHC` · `BG` | ≤4,416 | — | ≈1.0 | never | ≤192 | — | ≤4,352 | — |

Three things it settles.

1. **My brief's ordering is wrong at the lane level.** `SITR`'s excess is 86,528
   (level −21,231) against `SHDT`'s 68,928 (−3,631). `SITR` is larger and is the
   only lane whose excess alone closes the gate. The brief's "`SHDT` 50,240 >
   `SITR` 45,056" is the **cluster** rows — dominant-excess-owner subsets of the
   top 80 — which is a different quantity from the lane. Both are ceilings, and
   both lanes' levers are already refuted (`SHDT_MECHANISM.md`,
   `SITR_EXCURSION.md` + `ATTACH_LANE.md`).

2. **The median frame is not the problem.** The sixteen lane medians sum to
   **926,560** against a raw gate of **1,145,327**: a frame that spends every lane
   at its own median passes with **218,767 to spare**. The whole remaining
   +65,297 is excursion carried by the worst ~80 frames.

3. **But nine of sixteen lanes cannot close it even deleted**, and the seven that
   can need a *proportional* cut of 22–80%. `FTR` is the cheapest at **22%** —
   it is dead flat (1.00× at the band), so it converts **1:1**, which is why a
   flat lane pays better per tick removed than any excursion lane
   (`[[a-flat-lane-is-the-best-converting-lane]]`). It is also the largest lane in
   the run at 290,432 tk/fr and has never been attributed per-PC in the shipping
   configuration.

**That is the shape of the remaining problem and it is the honest brief for the
next cycle:** there is no lane whose excursion is a lever, and the cheapest
1:1 conversion is a fractional cut of the flat fighter bracket.

---

## 5. What this cycle did NOT do

- **No production source was edited, no default flipped, no ROM built or
  published, no emulator run of my own.** The only new files are this document,
  `lanes.py` and `lanes.txt`.
- **The per-joint setup was not changed** — §1 says there is nothing to share and
  §2 says it would be under the floor if there were.
- **The load-time resolver was not built** (§3), and the `AObjToQConvert` `None`
  store was not hoisted (298 ticks, `ATTACH_LANE.md` §3.1).
- **No fidelity decision was taken or asked for.** §1.5 widens an already-declined
  route rather than escalating it; `BLOCKED(decision: transition-frame animation
  play)` from `ATTACH_LANE.md` §4 is untouched and still not recommended.
- **`EXCHANGE.md`'s 2.68 and `SHDT_MECHANISM.md`'s ceiling were not reopened.**
- **`FTR` was not attributed.** §4's item 3 is a sizing, not an account: no
  per-PC census of the shipping configuration's largest lane exists, and
  `v3-c221` (`GX_COMPOSE=0`, 1,600 frames) is the capture that would answer it
  without a build.
- **Nothing was re-banked.** The requirement is +65,297 before and after.
- `decomp/` untouched; `build-c205-camtoggle` not rebuilt; both root ROMs
  unchanged (§6).
- **The 2^22 sampler filter needed no check**: no run was taken. The basis rows
  carry 0 corrections (`shdt_size.py` prints `applied to 0 frames`), because
  `ship220-rows.csv` was written by the corrected sampler; the c219 basis it is
  compared against still shows its 5.

---

## 6. Verification and hashes

```text
root ROMs, unchanged and not rebuilt by me this cycle:
  smash64ds.nds                        54c07fac80c50418949908701f7c2bdbf27512c5f96ac09086fabbb0df6ac68a
  smash64ds-battle-playable-hwtri.nds  6c939434c53c9b3a76ff016540b810a84f207b1a4e24540b8653b15717367c99
```

**Boundary green, exit 0, `0 Exception:`** — verdict lines in
`boundary.trimmed.log`. Boundary links its own
`smash64ds-battle-playable-proof-hwtri`, so its realtime pacing smoke is the
control that this cycle changed no production code, and it is exact against the
last six Boundary runs on this tree.

---

## 7. Reproduction

From the repo root, needing **no emulator and no build**:

```powershell
python artifacts\performance\2026-08-16_collision-setup-share\lanes.py   # sections 2 and 4
python artifacts\performance\2026-08-16_shdt-mechanism\shdt_size.py `
       artifacts\performance\2026-08-16_camera-ship\ship220-rows.csv     # SHDT on this basis
python artifacts\performance\2026-08-16_sitr-attach-lane\convert.py      # section 3
```

All three print the control (`rank-80 1,210,624 / +65,297`) before any result.
Run them from a normal PowerShell session, not from a Git-Bash-spawned shell.
