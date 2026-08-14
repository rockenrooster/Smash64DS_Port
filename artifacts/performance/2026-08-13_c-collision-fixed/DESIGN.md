# The fighter hurtbox narrow phase in fixed point — kernels proven, not wired

**Outcome: the kernel set is implemented and proven; the design the plan
prescribed is REFUTED and a different one is proven in its place.**

`docs/optimization/OPTIMIZATION_IDEAS.md`'s Phase 4 names the row-scaled
near-orthogonal inverse — `R^-1[c][r] = M[r][c] / s_r²`, three reciprocals and
nine multiplies — as the collision experiment worth running. It was
implemented, and it measures **0.1035 world units against a 0.0200 bound**, 5.2x
over, while its own orthogonality guard declines 92% of live cases. The cause is
not fixed point: it is **SSB64's own sine table**, which leaves the joint rows up
to **1.05% out of square** after a joint chain. The **cofactor** inverse ships
instead, at **0.0012** — sixteen times inside the bound, and at long reach more
accurate than the f32 original it replaces.

No emulator run, no gate figure, no wiring. One lab compile, for the link proof.
Both root ROMs byte-identical: `smash64ds.nds` `54C07FAC80C5…BBB0DF6AC68A`,
`smash64ds-battle-playable-hwtri.nds` `524448C99C31…342B3223ADEE`.

Owner authorization for this work: the collision float cluster was unfrozen for
a whole-cluster fixed-point replacement, 2026-08-13, in chat. The L7-era freeze
recorded in `docs/HANDOFF.md` ("float in `gmcollision`/`mp*`/`ftMain*`/
`ftComputer` is FROZEN") no longer binds this cluster.

---

## 1. Scope

The target is the cycle-6 band attribution's engagement chain
(`../2026-08-13_shdt-band-owner/BAND_OWNER.md`): +67,230 tk/frame on the 88
`SHDT` band frames, 42.3% of the band premium, 65% of it soft float. Everything
below lives in one decomp translation unit, `gm/gmcollision.c`, compiled into the
ROM through `src/import/battleship_gmcollision.c` — which `#include`s the decomp
`.c` at line 131 and has **no import flag**: it is unconditional, in `CFILES`,
and `decomp/` itself is never edited.

| decomp function | band calls/fr | in | out | kernel | status |
|---|---:|---|---|---|---|
| `lbCommonSin` / `lbCommonCos` (`lb/lbcommon.c:321`, port shim `reloc_backend_compat_shims.c:13519`) | 64.69 | f32 angle | f32 | `ndsR2CfxSinQ15` / `Cos` → **Q15 int, no conversion out** | PROVEN, exhaustive |
| `gmCollisionTransformMatrixAll` (`:29`) | 16.17 | DObj TRS (f32) | `Mtx44f` local | `ndsR2CfxBuildLocal` → `NDSR2CfxMtx` | PROVEN |
| `func_ovl2_800ED490` (`:208`) | 14.50 | 2 × `Mtx44f` | `Mtx44f` world | `ndsR2CfxCompose` | PROVEN |
| `gmCollisionSetInvertMatrix` (`:228`) | 9.02 | `Mtx44f` world | `Mtx44f` inverse | `ndsR2CfxMakeFrameCofactor` → `NDSR2CfxFrame` | PROVEN |
| `func_ovl2_800EDE5C` (`:472`), 3 × `sqrtf` | 17.60 | `Mtx44f` world | `vec_scale` | fused into the frame (`ndsR2CfxRowScales`) | PROVEN |
| `gmCollisionGetWorldPosition` (`:196`) | 26.49 | `Mtx44f` + point | point | `ndsR2CfxWorldToLocal` (inverse) / `ndsR2CfxTransformPoint` (forward) | PROVEN |
| `gmCollisionTestRectangle` (`:661`) | 17.60 | 2 points, radius, box | `sb32` | `ndsR2CfxTestRectangle` | PROVEN, differentially |
| **covered** | **~90% of the chain's 67,230** | | | | |

**Deliberately out of scope, each for a stated reason:**

| function | why not |
|---|---|
| `gmCollisionSetMatrixNcs` (`:82`) | the `is_use_animlocks` branch. **It is not in the band table at all**, while `gmCollisionTransformMatrixAll` is there at 16.17 calls/frame — so the band runs the other branch. Keeping it float also keeps the one construction that can make rows non-orthogonal on the float path. |
| `func_ovl2_800EDBA4` (`:332`) | control flow, not arithmetic: the ancestor walk. 1,637 tk/fr, all self, no float. It stays and calls the kernels. |
| `gmCollisionTestSphere` (`:780`) | the shield path. It *consumes* the inverse, so it is a **hard dependency of the seam** — see §6. Converting it is a net **−1,196 bytes** and is recommended for cycle B, not assumed here. |
| `func_ovl2_800EE050`, `func_ovl2_800EEEAC` (weapon-vs-attack) | 231 tk/fr combined, 1.9x band ratio. Under any floor. |

**Representation, end to end.** The float boundary is crossed **twice per joint
per frame**, not once per call:

```
  animation Vec3f rot/scale/trans      (f32, 15 soft-float ops per joint: the
              |                         six sine-table indices only)
              v
  ndsR2CfxBuildLocal    -> NDSR2CfxMtx  local   (Q26 rows, Q12 translation)
              v
  ndsR2CfxCompose       -> NDSR2CfxMtx  world              [integer only]
              v
  ndsR2CfxMakeFrameCofactor -> NDSR2CfxFrame              [integer only]
              |                    |
              |                    +--> vec_scale (Q12) -> f32, 3 conversions
              v
  ndsR2CfxTestRectangle(pos_curr, pos_prev quantised to Q12) -> boolean
```

Nothing in the middle converts. That is the difference from R2-07 L7, which
converted one leaf and paid two conversions per call to do it.

---

## 2. Formats, per value class

| class | format | raw bound | why that width |
|---|---|---|---|
| rotation rows `M[0..2]` | Q26 (6.26) | \|cell\| ≤ 4 | L7's finding: the dominant error is quantising the INPUT, and rows span ±scale, so six integer bits is ample and twenty-six buy precision. Live scale 1.1138–1.1199. |
| translation row `M[3]` | Q12 (20.12) | \|t\| < 2^17 = 131,072 | `MPGroundData::map_bound_*` are **s16**, so ±32,767 is the widest coordinate the stage format can express. The guard is 4x that: **no reachable coordinate can decline on range** (verified, T7a, 20,000 cases at ±32,767, zero declines). |
| sin/cos | Q15 | \|v\| ≤ 2^15 | `gSYSinTable` is u16 Q15 already. Reading it as an integer deletes the `__aeabi_i2f` + `__aeabi_fmul` `lbCommonSin` pays on the way out. |
| row square `s²` | Q26 | 1/16 … 16 | guarded; feeds the inverse and `vec_scale` from one set of nine products. |
| `1/s²`, `1/s` | Q26 | ≤ 16, ≤ 4 | `1/s = s · (1/s²)`, so `radius / scale` needs **no divide**. |
| axis scale `s` | Q12 | ≤ 4 | what `FTParts::vec_scale` holds. |
| points, sizes, offsets | Q12 | \|v\| < 2^17 | same class as `M[3]`. |

Every intermediate width, so no reduction is silent:

```
  compose rot    Q26 x Q26 -> Q52 int64 (peak 2^58)          >> 26
  compose trans  Q26 x Q12 -> Q38 int64                      >> 26
  s^2            Q26 x Q26 -> Q52 int64                      >> 26
  1/s^2          2^52 / s2_raw, s2_raw >= 2^22 by guard       -> Q26 int32
  s              isqrt64(s2_raw << 22) = s * 2^24            >> 12 -> Q12
  cofactor       Q26 x Q26 -> Q52 int64                      >> 26 -> int32
  inverse cell   Q26 x Q26 -> Q52 int64                      >> 26
  world->local   Q12 x Q26 -> Q38 int64 (|d|<2^30, |inv|<2^28, x3 -> 2^59.6)
  radius/scale   Q12 x Q26 -> Q38 int64                      >> 26
```

**Every product is a single SMULL.** That is a performance design, not a
neatness one: L7 measured what happens when an operand leaves int32 — the
inverse became 64×64 multiplies, 583 ARM instructions, and **+50,368 P95**,
dearer than the 295-instruction soft-float original.

Reductions use round-half-up (`(v + half) >> s`, one add and one shift) rather
than round-half-away-from-zero, which needs a compare, a branch and a 64-bit
negate. The bias it costs is half a quantum of 2^-27, four orders under the
bound. The float edges keep `ndsR2CollisionRoundShift`, L7's away-from-zero
form, so the two fixed-point families still agree on the boundary case.

---

## 3. Proof

`scripts/check-r2-collision-fixed.c` + `.ps1`, registered in
`verify-dev-fast.ps1`. Full output: `proof-run.txt` beside this file. It
compiles the **shipping header**, not a transcription, and grades it against a
transcription of the decomp float originals. Bound 0.0200 world units on a
transformed point — the E64b/E65 figure R2-07 L7 already carries, reused rather
than re-invented.

### Enumerated — exhaustive, not sampled

| kernel / domain | cases | result |
|---|---:|---|
| sine table, every reachable index | 4,096 | **0 error, exact** |
| sin/cos index arithmetic vs the port shim | 800,001 | **0 mismatches** |
| `isqrt64`, exhaustive 0 … 2^20 | 1,048,577 | **0 failures** |
| `isqrt64`, **every raw `s²` the live scale domain produces** | 914,399 | **0 failures** |
| `isqrt64` guard corners | 10 | 0 failures |

### Bounded — max world-unit error, gated column is vs float

The `float vs exact` column is the attribution, computed in double off the
game's own table values. Where the gated column is large **and that column is
large with it, the REFERENCE moved, not the kernel**.

| domain | cases | vs float | float vs exact | vs exact | bound |
|---|---:|---:|---:|---:|---:|
| forward chain, depth 6, \|t\|≤400 | 200,000 | **0.0014219** | 0.0000984 | 0.0014088 | 0.0200 |
| forward chain, depth 12 | 100,000 | **0.0018318** | 0.0001309 | 0.0018206 | 0.0200 |
| forward chain, depth 6, \|t\|≤32,767 | 100,000 | **0.0072490** | **0.0070889** | 0.0012726 | 0.0200 |
| frame (cofactor), depth 6, reach ±64 | 200,000 | **0.0012375** | 0.0001472 | 0.0012414 | 0.0200 |
| frame (cofactor), depth 12, reach ±64 | 100,000 | **0.0016533** | 0.0001389 | 0.0016443 | 0.0200 |
| frame (cofactor), depth 6, reach ±4,096 | 100,000 | **0.0027041** | **0.0021447** | 0.0011551 | 0.0200 |
| `vec_scale` | 200,000 | **0.0001223** | — | — | 0.0200 |
| frame (**row-scaled**), depth 6, reach ±64 | 16,071 admitted | **0.1035117** | — | — | 0.0200 **RED** |

Wider scale domains (0.90–1.10, 0.50–1.50, 0.25–2.00) are reported, not gated,
and the cofactor form holds all of them: worst 0.0026 at 0.25–2.00.

At reach ±4,096 the fixed frame is **more accurate than the f32 original**
(0.0012 from exact against the reference's own 0.0021). The `|t| ≤ 32,767` row
is the same story: 0.0071 of the 0.0072 is the f32 reference's own error at a
coordinate where `ulp` is 0.00195.

### Why the Phase 4 inverse lost — measured, in double, off the source

| relative row skew \|row_i · row_j\| / (s_i s_j) | max | mean |
|---|---:|---:|
| one local matrix | 0.00157 | 0.00027 |
| after 6 composes | 0.00882 | 0.00146 |
| after 12 composes | 0.01049 | 0.00174 |

`M^-1[c][r] = M[r][c]/s_r²` is exact only for exactly orthonormal rows.
`gSYSinTable` spans 0…π **inclusive** over 2048 samples — a one-sample stretch
the port's own shim documents as worth ~0.0016 against a true sine — so
`sin² + cos² ≠ 1` and the rows come out of a joint chain up to 1% out of square.
The skew reaches the local coordinate as roughly `skew · |p − t| / s`, which at a
hurtbox reach of 64 units is 0.06–0.6 world units. Measured 0.1035. There is no
tolerance that both admits the game's matrices and holds the bound: at 2^-10 the
guard declines 92% of live cases **and the 8% it admits are still over bound**.

This is a property of SSB64's data, not of fixed point, so it would have killed
the row-scaled inverse in float too. **Do not re-propose it.**

---

## 4. Decision-flip analysis for `gmCollisionTestRectangle`

The test's inputs carry the position error, so the only figure that matters is
how often the **boolean** differs. Measured differentially — float chain and
fixed chain on identical inputs, over hitbox radii 2–40, hurtbox half extents
1–18, offsets ±12, and a swept segment up to 80 units, i.e. the range the
fighter data uses (Fox's blaster hitbox is radius 20, the shield sphere 30, and
`ftparam.c:713` halves every `damage_coll` size at setup).

`margin` is the **perturbation margin**: the smallest change to the box half
extents that flips the float decision, found by bisection. It is the world units
of box a case has to spare, and a case is at risk exactly when its margin is
under the position error.

| | depth 6 | depth 12 |
|---|---:|---:|
| cases | 200,000 | 100,000 |
| **decision mismatches** | **0** | **1 (0.001%)** |
| largest margin at a mismatch | — | **0.00057** |
| cases with margin < 0.0200 | 185 (0.0925%) | 97 (0.097%) |
| smallest margin seen | 0.0001001 | 0.0001745 |

Margin histogram, depth 6: `<0.001` 14 · `<0.01` 87 · `<0.1` 840 · `<1` 8,542 ·
`<10` 81,521 · `≥10` 108,996.

**Prediction for cycle B's differential match run.** About **0.09% of pair
evaluations sit within the error bound of a face** and are therefore eligible to
flip; the measured flip rate is **two to three orders below that**, because the
kernel's actual error (0.0012–0.0017) is 12–16x under the bound the risk set is
counted at. Scaled to the gate arm's 17.60 `gmCollisionTestRectangle` calls per
band frame, a one-minute match runs on the order of 10^4–10^5 evaluations, so
**single-digit flips per match is the expectation, and zero is not surprising.**

A flip is not a behavioural change of a kind SSB64 can express: a hit that flips
at 0.0006 world units of margin is a hit that lands one simulation tick earlier
or later, on a hitbox that is 20 units across and a segment that sweeps tens of
units per tick. That is the "small numerical differences … do not materially
alter gameplay" clause of `PROJECT_GOAL.md`, and it is the same class of
difference E64b/E65 already accepted at this bound.

**The gate is not "zero flips."** A quantised representation must disagree on a
case whose margin is under its own quantum, and asserting otherwise would be
asserting bit-exactness, which the product contract explicitly does not require.
The gate the falsifier enforces is that **every flip is inside the position
bound** — a disagreement on a case with more than 0.0200 world units of box to
spare would mean the arithmetic is wrong rather than merely quantised.

---

## 5. Text delta, against the 1.85 cycles/byte law

Measured, not estimated. `text-delta.txt` beside this file carries the `nm`
output.

| | bytes |
|---|---:|
| float bodies deleted (Thumb, from the shipped battle ROM) | **3,800** |
| fixed kernels added (ARM, `-march=armv5te -marm -O2`) | **4,448** |
| **net** | **+648** |

At **1.85 cycles of `FTR` mean per byte of added ARM text** that is **≈ +1,199
cycles/frame** of placement cost — **2.5% of the 47,424 tk/frame the band needs
to shed to buy 16,000 of P95.**

Read that against L7, which is the whole reason this ledger exists: L7 added
**2,332 bytes for a 534-cycle win**, i.e. it paid 4,264 to earn 534. Here the
ratio is inverted by two orders, and the reason is structural rather than lucky:
**the float bodies leave the map.** In instruction count the cluster gets
*smaller* — 1,112 ARM instructions against 1,900 Thumb, a 41% reduction — the
byte delta is positive only because ARM instructions are twice as wide.

Two ways cycle B can take the delta negative, both already sized:

- converting `gmCollisionTestSphere` (1,196 B Thumb) costs ~700 B ARM → net
  **−496**, and it is a **dependency of the seam anyway** (§6);
- `ndsR2CollisionFixedBuildLocal` is 1,180 B, the largest kernel, and two `bl
  memcpy` calls for 48- and 72-byte struct copies are still in the object.

**Boot headroom price of this cycle: zero bytes.** `--gc-sections` drops the
entire object — the lab build at `NDS_R2_COLLISION_FIXED=1` links with **no
`ndsR2CollisionFixed` symbol in the ELF**. Headroom on that build reads
**193,568 bytes proven** (`check-boot-headroom.ps1`, highest booting
`0x02294804`). The flag still defaults to **0**, because an object entering the
link changes the link *input set* and this project has measured re-addressing
collateral from less, against a cross-build P95 floor of ±5,376.

---

## 6. The seam for cycle B

### The L7 scaffolding: usable, and one part of it is load-bearing

| L7 asset | verdict |
|---|---|
| `include/nds/nds_r2_collision_mtx.h` | **reused, not superseded.** `ndsR2CollisionF32ToFixed` / `FixedToF32` are the float-edge conversions this header calls — exponent arithmetic on the IEEE bits, ~15 instructions, against the ~2 soft-float ops a naive `(int)(v*4096+0.5f)` would cost per cell. Already graded by `check-r2-collision-mtx.ps1`. |
| `scripts/check-r2-collision-mtx.{c,ps1}` | **the pattern was copied wholesale**: compile the shipping header on the host, grade in world units, then re-compile for the real target and fail on soft float. The new falsifier adds a per-function attribution of the ARM helper calls, a soft-float budget, and an SMULL floor, because L7's regex silently `continue`d when a kernel inlined away. |
| **The hook** (`nds_r2_collision_mtx.h:66`) | **still correct and still the answer.** `gmCollisionSetInvertMatrix` cannot be intercepted — the `#define` include seam renames a decomp definition and its internal call sites together, and its only caller `func_ovl2_800EDE00` is in the same file, as are that function's nine callers. **The first externally-visible ring is the eight `gmCollisionCheck*` entry points plus `func_ovl2_800EE018`.** L7 measured engagement over 128 frames: 691 fills, 0 declines, 41 already-prepared. |
| `NDS_R2_COLLISION_L7_ORACLE` (`Makefile:1200`) | **do not re-run.** Its question is answered (scale 1.1138–1.1199) and building it once aborted the ROM at the GO countdown — its `.text` alone is more taskman arena than the tree has spare. |
| `ndsR2CollisionInvertMatrix44` (the wired L7 form) | **superseded.** It produces a float `Mtx44f` so decomp consumers stay untouched; this cluster removes those consumers, so the float round trip is pure loss. |

### Where the fixed state lives — and the one slot that can hold it

Measured by enumerating readers, because a matrix slot with an unnoticed reader
is silent corruption:

| `FTParts` slot | float bytes | readers | reinterpretable? |
|---|---:|---|---|
| `unk_dobjtrans_0x10` (local) | 64 | `gmcollision.c`, **`lbcommon.c` (`syMatrixF2LFixedW` — the renderer)**, `ftparam.c` | **NO** |
| `mtx_translate` (world) | 64 | `gmcollision.c` ×14, `lbcommon.c` ×10, `ftmain.c` ×6, `scsubsysfighter.c`, `ftcomputer.c`, `ftcommoncapturepulled.c`, and six port files | **NO** |
| `unk_dobjtrans_0x9C` (inverse) | 64 | **`gmcollision.c` only**, plus flag-gated diagnostics and `ndsFighterPartsSyncDObj`, which is **absent from the shipped battle ROM** (`nm`: no `sNdsFighterPartsPool`) | **YES** |

So the seam is: **the inverse slot is reinterpreted, the world and local
matrices stay float.** `NDSR2CfxFrame` is 72 bytes and does not fit 64 — but its
`scale[3]` field duplicates `FTParts::vec_scale`, which stays float and is
written anyway. Dropping it gives **60 bytes**, inside the slot. That is the one
struct change cycle B makes, and it changes no arithmetic, so re-running the
falsifier is the whole re-proof.

**No new state, no new lifetime, no bss.** §3.11 and §3.12 are satisfied by
construction: the kernels are pure functions and the frame reuses a slot that is
already cleared and re-derived per frame by the existing
`unk_dobjtrans_0x7` latch.

### The consequence that must not be missed

`gmCollisionTestSphere` reads `unk_dobjtrans_0x9C`. Reinterpreting the slot
**breaks the shield path unless `TestSphere` is converted in the same change.**
Its size makes that the cheap direction (−1,196 B Thumb for ~+700 B ARM), and it
needs one more kernel — the sphere test is `(p − t)·R^-1`, then divide by the
same `center` this cluster already builds, then `syVectorMag3D ≤ 1`. Treat it as
in-scope for cycle B, not as a follow-up.

### Cycle B integration checklist

1. `NDS_R2_COLLISION_FIXED=1`, and trim `NDSR2CfxFrame::scale` to fit 60 bytes.
   Re-run `check-r2-collision-fixed.ps1`; it must stay green with the same
   numbers.
2. Convert `gmCollisionTestSphere` and add its kernel to the falsifier with its
   own differential decision test.
3. Wire at **the eight `gmCollisionCheck*` entry points plus
   `func_ovl2_800EE018`**, per the L7 hook. `#define` the six replaced decomp
   symbols in `src/import/battleship_gmcollision.c` *before* the `#include` at
   line 131 — the file already documents that trap for
   `lbParticleMakeGenerator` and the rule that a symbol named exactly once (its
   definition) can be moved without capturing a call site.
4. **The originals must LEAVE THE MAP.** `nm` the linked ELF: zero
   `gmCollisionSetInvertMatrix`, `func_ovl2_800ED490`,
   `gmCollisionGetWorldPosition`, `gmCollisionTestRectangle`,
   `func_ovl2_800EDE5C`, `gmCollisionTransformMatrixAll`. **A wrapper is a
   failed change** — that is precisely what L7 did, and the 1.85 cyc/byte law
   then charges for both copies. `text-delta.txt` is the before column.
5. Verifier pins that move: `scripts/check-r2-collision-fixed.ps1` (new, in
   dev-fast) must stay green; `check-decomp-header-mirror.py` if any `FTParts`
   field is touched; `check-melonds-policy.ps1` is unaffected (no new
   `Start-Process`). Re-run `check-boot-headroom.ps1 -Build <lab> -Target
   smash64ds-battle-playable-proof-hwtri` after the wired build.
6. **Engagement counters on BOTH sides**, per the standing rule. Producer: fills
   of the fixed frame, and declines (the guard's fallback to float — a decline
   rate above ~0 on live play is a finding, not noise, because T7 says nothing
   reachable should decline on range). Consumer: `gmCollisionTestRectangle`
   calls served by the fixed frame against calls served by float. The ratio is
   the check — count both, and predict each before the run.
7. Behaviour: a differential match run. Compare end-of-match damage, stock, and
   hit counters against the control. A route A/B **cannot** price this change —
   it alters gameplay-visible state, and slice 41's arms ended 130/51 against
   33/65 on ticks alone. Read an end-of-match counter before believing any tick
   delta.
8. A/B/A with the **flag falsifier**, per slice 51: a rebuilt control is
   byte-identical and brackets nothing. The A2 arm is
   `NDS_R2_COLLISION_FIXED=1` with the call sites reverted — the candidate's
   placement carrying the control's behaviour. Judge on `WORK-H` P95, gate arm
   `NDS_R2_BOTH_CPU=1`, 1,600 samples, `-RingDump`, DLDI on, `NDS_TICK_HUD_DRAW=1`.
9. Sizing to expect, and it is a **projection, not a measurement**: the six
   converted functions are 60,494 of the chain's 67,230 profile tk/frame, ≈122,800
   gate tk/frame after the 2.03 dilution factor. The bar is 47,424 for 16,000 of
   P95, and a band-only cut saturates at −78,016. Whether the fixed path lands
   inside that window is exactly what cycle B measures.

---

## 7. What this cycle did NOT do

- **Did not wire anything.** No call site changed; no float body was deleted.
  The kernels are a compiled, unit-proven, unreferenced subset behind a
  default-off flag.
- **Did not run an emulator.** No gate figure, no P95, no tick number of any
  kind is claimed for this change.
- **Did not convert** `gmCollisionSetMatrixNcs` (animlocks branch) or
  `gmCollisionTestSphere` (shields). The second is a seam dependency, §6.
- **Did not touch `decomp/`.** The float originals are in place and unmodified.
- **Did not measure the live decline rate.** T7 proves nothing reachable
  declines on *range*; the guard also declines on determinant, and only a live
  counter settles that. It is item 6 of the checklist.
- **Did not re-derive the live scale domain.** It was read off the running game
  on 2026-07-31 and `Makefile:1196` records that re-running that oracle aborted
  the ROM.
