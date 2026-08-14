# The collision fixed-point seam, corrected against the linked ELF — and the live domain measured

**Outcome: cycle A's §6 seam is wrong for three of its six symbols, the
convertible work is 37–52% of what §6 sized rather than ~90%, and the two
largest proven kernels cannot be reached at all at the only seam that exists.
The kernels themselves are re-proven, on real matrices this time: 0 declines and
0.0004 world units against a 0.0200 bound, more accurate than the float original
it would replace.**

No build was spent. No ROM was produced. No gate figure is claimed. Both root
ROMs are untouched — nothing in this cycle compiles into either.

This cycle was briefed to wire, prove map absence, run the differential, and take
the gate A/B/A. It did not, and the reason is the first section: the wiring the
brief describes cannot be written as specified, because three of the six float
bodies it requires to leave the map have referrers outside the collision cluster.
Establishing that cost no build; writing 600 lines against the wrong seam would
have cost the whole cycle.

---

## 1. The reader oracle: who actually references the six float bodies

`arm-none-eabi-objdump -d` over the shipped `smash64ds-battle-playable-hwtri.elf`,
attributing every relocated reference to its containing function. Full output:
`elf-referrers.txt` beside this file. Memory *"Linked ELF is the reader oracle"*
is the reason this is the evidence and the `rg` sweep is not.

| symbol | Thumb bytes | referrers | can it leave the map? |
|---|---:|---|---|
| `gmCollisionSetInvertMatrix` | 716 | 9, **all in the ring** (8 `gmCollisionCheck*` + `func_ovl2_800EE018`) | **YES** |
| `func_ovl2_800EDE5C` | 198 | 8, **all in the ring** | **YES** |
| `gmCollisionTestRectangle` | 1,680 | 3, **all in the ring** | **YES** |
| `gmCollisionTestSphere` | 1,196 | 5, **all in the ring** | **YES** (seam dependency, §6 of DESIGN) |
| `gmCollisionGetWorldPosition` | 196 | **9; only `TestRectangle` and `TestSphere` are replaced, so SEVEN survive** — `gmCollisionGetFighterPartsWorldPosition`, `func_ovl2_800EEEAC`, `ndsBaseFTComputerSetFighterDamageDetectSize`, `ndsBaseFTCommonCapturePulledRotateScale`, and all three of `gmCollisionGet{Fighter,WeaponAttackFighter,ItemAttackFighter}…DamagePosition` | **NO** |
| `gmCollisionTransformMatrixAll` | 430 | 3 — `ftParamSetAnimLocks`, `gmCollisionGetFighterPartsWorldPosition`, `func_ovl2_800EDBA4` | **NO** |
| `func_ovl2_800ED490` | 580 | 1 — `func_ovl2_800EDBA4` | **NO** (see §2) |

DESIGN's checklist item 4 asks `nm` to show zero of all six. Three of them cannot
be zero: `gmCollisionGetWorldPosition` is read by the CPU's damage-detect sizing,
the capture/pulled rotation, all three attack-position helpers and the weapon-vs-attack
test, none of which this cluster touches. **That item is retracted as written.**

Two more facts the same read settles, and they are good news:

- **`func_ovl2_800EDE00` is not in the ELF at all** — the compiler inlined it into
  its nine callers. That is why `gmCollisionSetInvertMatrix` has exactly nine
  referrers, and it *confirms* L7's ring: the eight `gmCollisionCheck*` entry
  points plus `func_ovl2_800EE018` are precisely the direct callers of the invert.
- **No in-TU caller keeps a dead copy alive.** The nine ring functions are
  referenced only from `battleship_ftMainSearchHitFighter`,
  `battleship_ftMainSearchFighterCatch`, `battleship_ftMainSearchHitWeapon`,
  `battleship_ftMainSearchHitItem` and `ndsBaseFTCommonAttackS4ProcUpdate` — all
  outside `gmcollision.c`. So renaming the nine decomp definitions before the
  `#include` and defining the live names port-side leaves the renamed originals
  unreferenced, and `-ffunction-sections` + `--gc-sections` (`Makefile:2144`,
  `:2227`) drops them together with the four float bodies they exclusively call.

`func_ovl2_800EE24C`, `func_ovl2_800EE2C0` and the three item-damage
`gmCollisionCheck*` are **absent from the ELF** — the first two inlined, the last
three dead with items off. So `gmCollisionTestRectangle`'s three live referrers
are exactly the three in the ring, with none left over.

---

## 2. The consequence: the forward chain is out of reach, and two proven kernels with it

DESIGN §6 derives, from an enumeration of `FTParts` readers, that only
`unk_dobjtrans_0x9C` may be reinterpreted and that **the world and local matrices
stay float**. That is right, and this cycle re-verified its load-bearing half at
link time: `sNdsFighterPartsPool`, `ndsFighterPartsSyncDObj` and
`ndsFighterPartsSetIdentity` are **absent from both** the shipped battle ELF and
the tick-HUD gate ELF, so the port writer that would fill the inverse slot with a
float identity is not linked. `probe-collision-fixed-domain.ps1` asserts this
rather than inheriting it.

But §1's table then says the forward chain cannot be converted, because keeping
`mtx_translate` float means keeping `func_ovl2_800EDBA4` — and `func_ovl2_800EDBA4`
is shared infrastructure with **15 referrers**, including
`ndsRendererAdapterBuildDObjXObjMatrix`, `battleship_ftMainProcParams` and
`func_ovl0_800C9A38`. Keeping it keeps its two callees,
`gmCollisionTransformMatrixAll` and `func_ovl2_800ED490`.

So the two largest kernels cycle A proved and measured —
`ndsR2CollisionFixedBuildLocal` (1,180 B ARM) and `ndsR2CollisionFixedCompose`
(344 B) — **have no call site at this seam**. 1,524 of the 4,448 proven bytes are
unreachable, and with them go the sine kernels' 6,406 tk/frame, which are only
reachable through `gmCollisionTransformMatrixAll` and `gmCollisionSetMatrixNcs`.

Moving `func_ovl2_800EDBA4` port-side would recover `func_ovl2_800ED490`, but it
would have to emit a **float** `mtx_translate` for its fifteen readers — twelve
fixed→float conversions per joint — and handle the `is_use_animlocks` branch
through `gmCollisionSetMatrixNcs`, which is explicitly out of scope. That is L7's
losing shape (convert a leaf, pay the conversions), and it is not proposed.

---

## 3. Corrected sizing, from the cycle-6 band table

`../2026-08-13_shdt-band-owner/BAND_OWNER.md`, profile tk/frame on the 88 `SHDT`
band frames. Dilution to gate tk/frame is that document's factor 2.03.

| function | total tk/fr | fate at the achievable seam |
|---|---:|---|
| `gmCollisionSetInvertMatrix` | 8,526 | **converted** |
| `gmCollisionTestRectangle` | 7,300 | **converted** |
| `func_ovl2_800EDE5C` | 6,498 | **converted** (fused into the frame) |
| `gmCollisionGetWorldPosition` | 8,954 | **partly** — the calls inside `TestRectangle`/`TestSphere` go; the six other referrers keep the body |
| `func_ovl2_800ED490` | 16,564 | stays |
| `lbCommonSin` + `lbCommonCos` | 6,406 | stays |
| `gmCollisionTransformMatrixAll` | 6,246 | stays |
| `gmCollisionGetFighterPartsWorldPosition` | 3,041 | stays |
| `func_ovl2_800EDBA4` | 1,637 | stays |
| `gmCollisionCheckFighterAttackDamageCollide` | 1,336 | moves port-side, same work |
| `gmCollisionCheckFighterInFighterRange` | 467 | stays |
| **chain total** | **67,230** | |

**Convertible: 22,324 tk/frame certain, up to 31,278 with the whole of
`gmCollisionGetWorldPosition`** — the true figure is inside that range and this
cycle did not split it, because splitting it needs a per-call-site count that
only a profile run produces. Against DESIGN §9's **60,494**, that is **37–52%**.

Carried through the same arithmetic §9 used: 45,300–63,500 gate tk/frame against
a bar of **47,424 for 16,000 of P95**, i.e. **0.96x–1.34x the bar** rather than
§9's 2.6x. And that is the work *replaced*, not the work *removed* — the fixed
path still costs something, and the added ARM text is charged at 1.85 cycles of
`FTR` mean per byte. **A cycle C that wires this should expect a small win, a
wash, or a loss, and should be briefed as a measurement rather than as a
landing.** The projection is a projection; only an A/B/A decides.

### Text delta, corrected

| | bytes |
|---|---:|
| deletable float bodies (`SetInvertMatrix` 716, `EDE5C` 198, `TestRectangle` 1,680, `TestSphere` 1,196) | 3,790 |
| the nine ring bodies, re-emitted port-side | 1,442 |
| **deleted, Thumb** | **5,232** |
| reachable proven kernels (`MakeFrame` 1,212, `TestRectangle` 1,504, `WorldToLocal` 108) | 2,824 |
| unreachable proven kernels (`BuildLocal` 1,180, `Compose` 344) — **not linked** | 0 |
| a sphere kernel, not yet written | *unmeasured* |
| a float→`NDSR2CfxMtx` builder, not yet written | *unmeasured* |
| the nine ring bodies, port-side (Thumb, no 64-bit math) | ≈1,442 |

The measured half is 5,232 out against 4,266 in. The unmeasured half is the
sphere kernel and the frame builder. **Do not quote a net until both exist** —
cycle A's `+648` assumed all six bodies left and both large kernels linked, and
neither is true here.

---

## 4. The live domain, and the decline rate — checklist item 6, closed

Cycle A proved the kernels over a chain **built in fixed point**. The seam that
can ship feeds the frame from the **float** `FTParts::mtx_translate`, quantised on
the way in. That input domain had never been graded, and the guards' live decline
rate was the one item cycle A explicitly left open.

`scripts/probe-collision-fixed-domain.ps1` samples the live gate arm
(`build-c144-ledgeridx`, `NDS_R2_BOTH_CPU=1`, `NDS_TICK_HUD_DRAW=1`, mode 163)
and prints every joint world matrix whose inverse the hit path built that frame.
It reads only globals and pointer derefs — `gSCManagerBattleState->players[p]
.fighter_gobj` → `FTStruct::joints[j]` → `DObj::user_data.p` → `FTParts` — so
CLAUDE.OPUS.md's "stack locals lie through this stub" and "never call a guest
function" rails are both satisfied by construction, and no register is read.

`scripts/grade-r2-collision-live-domain.c` then compiles **the shipping header**
on the host and grades it on those matrices against (a) a transcription of the
decomp float originals and (b) a double-precision exact reference.

**Two instants were wrong before the third was right, and the failure is worth
recording**: sampling at `ndsBattlePlayableFrameCompleteMarker` read
`latch5=latch6=latch7=0` on 52 live joints for three rounds running, and so did
sampling at `ftParamsUpdateFighterPartsTransformAll`. That is not a dead latch —
`ndsFTParamsInvalidateFighterParts` (`reloc_backend_compat_shims.c:1638`) zeroes
`unk_dobjtrans_word` for every joint every frame, and by both of those instants it
has already run. The instant that works is *inside* the hit path: break on
`gmCollisionCheckFighterAttackDamageCollide` with an ignore count, which also
self-selects the frames that have collision activity at all.

### Result — 152 live joint matrices, two runs, whole match

`live-domain-grade.txt` beside this file carries the exact command and output;
`domain-capture-a.txt` / `-b.txt` / `-merged.txt` are the raw rows. Arm is
`build-c144-ledgeridx` — the shipped gate arm, `NDS_R2_BOTH_CPU=1`,
`NDS_TICK_HUD_DRAW=1`, mode 163. Frames 592–1625, i.e. the body of the match.

| | |
|---|---:|
| joint matrices | **152** |
| **admitted by the cofactor frame** | **152 (100.0000%)** |
| declined — f32→fixed overflow / range / row scale / determinant / inverse cell | **0 / 0 / 0 / 0 / 0** |
| comparisons (probes at 1, 4, 16, 64 units on each axis) | 5,472 |
| **max fixed vs float** | **0.0003662** against the **0.0200** bound — 55x inside |
| mean fixed vs float | 0.0000710 |
| max fixed vs exact | 0.0003176 |
| max **float** vs exact | 0.0003170 |
| max `vec_scale` delta (149 with the scale latch live) | 0.0001220 |

**The guard's live decline rate is zero, on all five guards, over the whole
match.** That closes checklist item 6 the way cycle A predicted it would and
settles INTEGRATION LAW item 2: the decline path becomes a recorded fail-closed
path (counter plus safe result), not a retained float body. A non-zero counter in
a later cycle is a stop, not noise.

**The fixed frame and the f32 original are the same accuracy here, not one
better than the other** — 0.0003176 against 0.0003170 from exact. Cycle A's
"more accurate at long reach" holds at reach ±4,096; at the reach a hurtbox test
uses they are equal to four significant figures. Run A alone read 0.000204 against
0.000310 and *looked* like a win for fixed; the merged set says it is a wash. The
claim to carry forward is "indistinguishable", not "better".

### The inherited live scale domain is WRONG, and this is the finding that would have bitten cycle C

`nds_r2_collision_mtx.h:51` and DESIGN §2 both record the live joint scale as
**1.1138–1.1199**, from L7's 460-sample oracle on 2026-07-31, and cycle A sized
the *gated* falsifier rows on it. This capture measures **0.9937 – 2.0479**.

The 2.0479 is not a stale read: player 0, joint 15, frame 592, with the source's
own scale latch `unk_dobjtrans_0x6 == 1`, all three axes at 2.046–2.048. So a
live joint reaches **1.83x the inherited maximum**, and it sits **outside the top
of the widest domain cycle A even reported** (0.25–2.00).

Nothing breaks: `NDS_R2_CFX_S2_MAX` is 16 (s ≤ 4), so the guard admits it, and it
is inside the 152 matrices that measured 0.0003662. But **the safety margin is
not what the design assumed**: `NDS_R2_CFX_ROT_MAX` declines a row cell past 4.0,
and a row cell of a joint at scale 2.0479 reaches 2.05 — **1.95x of margin, not
the ~3.6x a 1.12 scale implies.** Still comfortable, still measured at zero
declines, but it is the guard with the least room and cycle C should know that
before it narrows anything.

Both headers state a range that is false, and both are corrected in this cycle
(`nds_r2_collision_mtx.h`, `nds_r2_collision_fixed.h` — comments only, no
arithmetic touched). **Cycle C must widen the gated falsifier sweep to at least
0.95–2.10 before it quotes cycle A's bounded numbers.**

---

## 5. What cycle C has to do, and it is now mechanical

1. `#define` **nine** decomp symbols in `src/import/battleship_gmcollision.c`
   before the `#include` at line 131 — the eight `gmCollisionCheck*` of the ring
   plus `func_ovl2_800EE018` — to `..._floatorig` names. §1 proves no in-TU caller
   survives to keep them alive, so `--gc-sections` drops them and, with them, the
   four float bodies they exclusively call.
2. Define the nine live names in a port TU, **Thumb** (they carry no 64-bit
   arithmetic, and Thumb is where their 1,442 bytes came from). They call the ARM
   kernels across an interworking branch, which is free next to what they replace.
3. The prepare step replaces the inlined `func_ovl2_800EDE00` + `func_ovl2_800EDE5C`
   pair: on `unk_dobjtrans_0x7 == 0`, call the decomp `func_ovl2_800EDBA4` if
   `0x5 == 0` (it stays, it is shared, it produces the float `mtx_translate`),
   quantise that matrix, build the cofactor frame into `unk_dobjtrans_0x9C`, set
   `0x7`; on `0x6 == 0`, write `vec_scale` from the frame's row scales and set
   `0x6`. **§3.12 is satisfied structurally, not by a guard**: the slot's only
   validity key is `unk_dobjtrans_0x7`, and `ndsFTParamsInvalidateFighterParts`
   zeroes the whole latch word for every joint of every fighter each frame, so a
   stale frame cannot be read — there is no lifetime to get wrong and nothing to
   zero at scene entry.
4. Trim `NDSR2CfxFrame::scale[3]` so the struct is 60 bytes and fits the 64-byte
   slot. `ndsR2CfxRowScales` already hands `scale` back separately; the caller
   consumes it for `vec_scale` and does not store it.
5. Write the sphere kernel and give it its own enumerated + differential rows in
   `check-r2-collision-fixed.c`. It is `(p − t)·R^-1`, divide by the same `center`
   the rectangle test builds, then `syVectorMag3D ≤ 1`, plus the quadratic sweep
   for the non-transfer case. This is the only genuinely new arithmetic left.
6. The decline path is **fail-closed and recorded**: §4 measures the live rate, so
   a decline is a counter plus a safe result, never a retained float body. A
   non-zero counter on a match is a stop, not noise.
7. Then, and only then, the brief's chain: map proof by `nm`, one lab-only
   differential run, the gate A/B/A with the slice-51 flag falsifier (A2 =
   `NDS_R2_COLLISION_FIXED=1` with the nine `#define`s reverted), the five-minute
   qualification, Boundary.

### Two simplifications the ELF hands cycle C for free

- **`gmCollisionTestRectangle`'s `mtx` is never NULL in the shipped ROM.** The
  only caller that passes NULL is `gmCollisionCheckFighterAttackItemDamageCollide`
  (`gmcollision.c:1412`), and that function is **absent from the ELF** — items are
  off. The three live callers all pass `parts->unk_dobjtrans_0x9C`. So
  `ndsR2CfxTestRectangle`'s `frame != NULL` branch is the only reachable one; the
  NULL branch stays for the falsifier and costs a compare.
- **The sphere kernel needs all three `sphit_kind` values.** The five live
  callers use kind 0 (`FighterAttackShield`), 1 (`Weapon`/`ItemAttackShield`) and
  2 (`Weapon`/`ItemAttackSpecial`). The two Item ones are linked (from
  `battleship_ftMainSearchHitItem`) even though items off means they do not
  execute, so the kernel must be correct for all three rather than only the two
  that run.

### Two seam hazards §6 did not enumerate

- **`ftGetStruct`'s stub** (`reloc_backend_compat_shims.c:13852`) hands back a
  `bzero`'d `FTParts` with `0x5/0x6/0x7` all set and `vec_scale` 1.0, so its
  `unk_dobjtrans_0x9C` is sixty-four zero bytes. Reinterpreted, `inv` is zero —
  which matches the float path, where a zero matrix also maps every point to the
  origin — but `inv_scale` is zero too, so the fixed `center` is `size` where the
  float one is `size + radius/1.0`. **That is a real behavioural difference on
  that path.** Cycle C must either fill the stub's slot with an identity frame or
  prove the stub cannot reach the ring.
- **`gmCollisionSetMatrixNcs` stays** (the `is_use_animlocks` branch) and is
  reached only through `func_ovl2_800EDBA4`, which also stays. It therefore
  cannot drag a deleted body back in: its callees are `lbCommonSin`/`Cos`, which
  stay anyway. Verified in `elf-referrers.txt`.

---

## 6. What this cycle did NOT do

- **Did not wire anything.** No call site changed, no float body was deleted, no
  `#define` was added. `NDS_R2_COLLISION_FIXED` still defaults to 0 and
  `--gc-sections` still drops the whole object.
- **Did not build a ROM**, so there is no P95, no A/B/A, no differential match
  run, no five-minute qualification and no Boundary run in this cycle. The
  measurement chain the brief specifies starts at a wiring that does not exist
  yet.
- **Did not split `gmCollisionGetWorldPosition`'s 8,954** between the call sites
  that convert and the six referrers that do not. That needs a per-call-site
  count from a profile run.
- **Did not write the sphere kernel.** It is the one piece of new arithmetic left
  and it is unproven.
- **Did not touch `decomp/`.**
