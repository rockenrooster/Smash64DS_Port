# Profile-0 Native Campaign — plan of action

Status: **Task 48 kill criterion PASSED (2026-07-23) — campaign premise
measured true; Tasks 51/52 authorized. Otherwise proposal.** Standing rules
apply (`docs/optimization/TASK_STANDING_RULES.md`).

The ask: demote today's Profile 0 to Profile 1 and today's Profile 1 to Profile
2, and build a new Profile 0 that is DS-native front to back — precompiled,
pre-converted, pre-processed.

This document is the grounded version of that ask. Every number in it was
re-measured from artifacts in this repo before it was written; where the two
external reviews are wrong, it says so and gives the measured value.

---

## 1. What the measurements actually say

**Source A — ARM9 symbol census**, `artifacts/task37-census/census.json`,
500,810,896 cycles over 128 presented frames of the tick-HUD ROM:

| group | cycles | % of total |
|---|---|---|
| `ndsRenderer*` (the whole display path) | 246,310,543 | **49.18%** |
| — texture resolve/bind/find/upload/sync | 57,002,231 | 11.38% |
| — vertex/triangle emit | 63,477,264 | 12.67% |
| — stage native path (incl. Task 36) | 68,254,691 | 13.63% |
| — matrix build + load | 41,224,048 | **8.23%** |
| — material/state resolution | 36,756,158 | 7.34% |
| soft float (`__aeabi_*`, `__ieee754_*`) | 48,595,234 | 9.70% |
| — of which divide + sqrt only | 11,548,900 | 2.31% |
| `memset`/`memcpy`/`memcmp` | 39,383,889 | 7.86% |
| `armWaitForIrq` (idle) | 46,140,563 | 9.21% |
| fighter (`ft*`, `ndsFT*`, `ndsFighter*`) | 31,033,576 | 6.20% |
| GObj/anim (`gc*`) | 19,718,496 | 3.94% |

**Half the program is the display path.** That single fact is what justifies the
whole campaign. Nothing else in the census is within 5× of it.

**Source B — tick-HUD device-equivalent buckets**, 256 samples, frames 438–693,
ROM `20D25EE0…` (`artifacts/performance/tick-hud-buckets-fork-20260722-run3.json`):

| bucket | P50 | P95 |
|---|---|---|
| ALL | 1,680,448 | 2,800,512 |
| STG | 597,632 | 605,440 |
| FTR | 590,144 | 598,848 |
| SRC | 324,096 | 955,904 |
| OTHR | 134,080 | 546,624 |
| HUD | 1,024 | 198,272 |
| MISC | 82,432 | 129,152 |
| AUD | 1,344 | 65,216 |
| BG | 4,032 | 4,096 |

VBlanks per presentation over 693 frames: **2 → 0**, 3 → 516, 4 → 147, 5+ → 30.
We never once hit 30 FPS in the sampled window.

**The gap, stated exactly.** Two VBlanks ≈ 1,119,000 ticks.

- P50 must fall **1,680,448 → ≤1,119,000 = −562,000 (−33%)**
- P95 must fall **2,800,512 → ≤1,119,000 = −1,682,000 (−60%)**

STG + FTR alone are 1,187,776 at P50 — **70.7% of the P50 budget**. If the native
path takes those two to Nintendo-shaped numbers, P50 clears with room. **P95 does
not**, because P95 is dominated by SRC (955,904) and OTHR (546,624), which the
renderer does not touch. That split governs the whole plan: *the native renderer
buys the median, a second axis buys the tail.*

---

## 2. The finding that decides whether any of this works

Task 34 measured the current baked stage stream and found **0 of 6,894 words
immutable (0.000%)**. Task 36 recovered 58% conservation only by adding a CPU
compose step. Both reviews propose "bake it and replay it" without engaging with
that result. If the reason for 0.000% is intrinsic, the campaign is dead on
arrival.

It is not intrinsic. `src/nds/nds_renderer.c:12821` is the reason:

```c
ndsRendererHardwareSetMatrixMode(GL_MODELVIEW);
glLoadMatrix4x4(&modelview_hw);      /* a CPU-composed view*model product */
```

We compose `view × model` **on the CPU** and load the 4×4 product. So every
emitted word downstream of a matrix load is camera-dependent by construction —
which is precisely what Task 34 measured. It is not that the geometry moves; it
is that we hand the GPU a product it was built to compute itself.

Nintendo's shape is `MTX_PUSH` → `MTX_MULT4x3(constant model matrix)` →
model-space vertices → `MTX_POP`, with the view loaded **once** per frame. Under
that shape a static Dream Land part's stream is **100% immutable**, and the live
word count per frame is the camera, not the scene.

This also explains the 8.23% matrix cost: `MtxMul20p12` (1.53%),
`LoadHardwareMatrixPair` (1.14%), `AdapterBuildDObjLocalMatrix` (0.98%),
`MtxMulAffine20p12` (0.86%), `MtxLoadN64ToDS20p12` (0.75%) are all CPU work the
geometry engine does for free, plus we load 16 words where 12 suffice
(`MATRIX_MULT4x3` — Nintendo's `Matrix4x3`, and the reviews are right about that
one).

**This is the first experiment and the campaign's kill criterion** (Task 48
below). If a model-space + `MULT4x3` stream does not measure ≫58% immutable
words, stop; nothing after it pays.

---

## 3. Where the two reviews are right, wrong, and incomplete

**Right, and worth taking:**
- The DS hardware divider and square-root units are the correct home for hot
  divides. Verified: `REG_DIVCNT` (0x04000280), `divf32`, `sqrtf32` exist in
  libnds and **`src/` and `include/` contain zero references to any of them.**
  We are paying 2.31% for divide and sqrt in software with idle silicon
  on-chip. This is free money and needs no new pipeline.
- 4×3 matrices instead of 4×4 (see §2).
- Material state resolved at build time, not from `G_SETCOMBINE` at runtime
  (7.34% of cycles are material/state resolution).
- HUD on OAM rather than through the model renderer.
- Split 60 Hz gameplay events from 30 Hz visual pose generation.

**Wrong on facts:**
- GPT5.6's "fighters ~1.04M P95" — 1,046,080 is FTR's **max**. P95 is 598,848.
  Its HUD "~328K" is 198,272. The fighter overshoot is real but ~40% smaller
  than quoted.
- Both describe `decomp/sm64ds-decomp` as a readable engine with
  `ModelComponents::Render()`, `Fix12i` and model records. It is a **symbol-level
  matching decomp: 10,914 single-function files and no headers at all.** Most
  are still `func_020XXXXX`. It is a good source for *specific algorithms*
  (`Matrix4x3_*`, `Geometry_MatrixMultiply3x3`, `CopyToViewMat`, quaternion
  paths) and for structural evidence via mangled names. It is **not** an
  architecture you can read off and copy. Plan the time accordingly.
- `decomp/sm64-nds` **is** readable and is the real reference: 1,285 lines,
  `ITCM_CODE` on nine hot functions, `DTCM_BSS` on all hot state, `BATCH_SIZE 96`.

**Incomplete — the biggest omission in both:**
- **The bake pipeline already exists.** `scripts/generate_nds_native_stage.py`
  (4,236 lines) already emits an immutable Dream Land packet with every field
  classified `immutable_generation` / `live_camera_dependent` /
  `live_camera_independent` / `callback_visible_mutation_output`.
  `scripts/generate_nds_native_owners.py` (2,939 lines) does the Mario/Fox
  owner IR. Textures are already pre-converted
  (`assets/renderer/battle_playable_static_textures.rgb5a1.bin`).
  **We pre-converted the data and then kept a per-frame interpreter over it.**
  The campaign is not "build a converter" — it is "delete the interpreter."
  That is a much better starting position than either review assumes.
- Neither review costs the **P95 tail**, which is where 30 FPS actually lives.
- Neither gives a kill criterion. Every one of Tasks 21R–47 that shipped a
  promise without one got reverted.

---

## 4. The renumbering — do it in naming, not in 700 `#if` sites

`NDS_RENDERER_PROFILE_LEVEL` appears at **391 sites in `nds_renderer.c`**, 159 in
`reloc_backend_renderer_dl.c`, 32 in `taskman_seam.c`, 32 in
`check-gbi-decode-fixtures.ps1`, plus `nds_platform.c`, `nds_ifcommon_oam.c`,
`sprite_preview_backend.c` and others — and many are ordinal (`>= 1`, `!= 1`,
`== 2`). A literal renumber is a ~700-site mechanical edit that changes no
behavior, invalidates every doc, artifact name and script default, and risks the
published ROM for zero measured gain.

**Recommendation: add the axis the request describes, and leave the existing
knob alone.**

```make
NDS_BATTLE_PROFILE ?= 1     # 0 = DS-native precompiled (new)
                            # 1 = today's shipping translation path (demoted)
                            # 2 = instrumented / semantic oracle (demoted)
```

`NDS_RENDERER_PROFILE_LEVEL` keeps its current values and is documented for what
it actually is — the *renderer instrumentation level within profiles 1 and 2*
(0 lean, 1 phase timers, 2 full oracle). The user-facing semantics are exactly
what was asked for: **P0 native, old P0 → P1, old P1 → P2.** The demoted path
becomes the correctness oracle for the new one, which is the right role for it
and the reason not to spend effort polishing it.

`smash64ds-battle-playable-hwtri` keeps building `NDS_BATTLE_PROFILE=1` until a
native task earns the flip. Every existing verifier, target block, census script
and artifact keeps working unchanged on day one.

---

## 5. The campaign

Eight tasks. Task 47 was withdrawn, so this starts at 48. Each carries a budget,
a gate and a kill criterion; each is an E0 census before an E1 implementation,
per house rules. **Nothing merges without the owner's play test.**

### Task 48 — Model-space stream census *(the kill criterion; no shipped code)*
Spec: `ClaudeFable5_Task48_ModelSpaceStreamCensus_20260723.md`.

Re-run the Task 34 word-immutability census against a **model-space +
`MATRIX_MULT4x3`** formulation of the same Dream Land bindings: view loaded once
per frame, constant per-part model matrices, vertices untransformed. Stage A is
host-only — the Task 34 raw captures are still on disk, so the first half of the
answer costs zero builds and zero emulator runs.

- **Measures:** immutable-word fraction, live words per frame, matrix words
  saved by 4×3.
- **Budget:** none — instrument only, default off.
- **KILL if:** immutable fraction ≤ 58% (Task 36's existing figure). Then the
  camera contamination is intrinsic, the whole bake premise fails, and the
  campaign reduces to Tasks 50 and 53 alone.
- **Cost:** ~1 day. This is the cheapest possible test of the campaign's core
  assumption and it must run first.
- **VERDICT (2026-07-23): H1 AND H2 MEASURED TRUE; CAMPAIGN ALIVE.** Stage A
  (host-only, zero builds): non-matrix stage stream words are **100.000%**
  bit-identical across 24 frames — Task 34's 0.000% was entirely the
  CPU-composed 4×4 matrix load (`src/nds/nds_renderer.c:12849-12868`). Stage B
  (one lab ROM, 8 frames × 3 windows, 4 emulator runs): **all 42 static stage
  DObjs have a bit-identical camera-free world matrix** across frames (42/42 in
  countdown + Whispy; 42/43 = 97.674% in combat, one animated stage part = the
  predicted handful of live words). Projected immutable share of a model-space
  stage stream = **99.002% ≫ 58%**. **Tasks 51 and 52 are authorized.** This
  task ships no runtime behavior; the branch is the checkpoint. Full result:
  `ClaudeFable5_Task48_ModelSpaceStreamCensus_20260723.md` (Results section).

### Task 49 — `NDS_BATTLE_PROFILE` axis + GX equivalence differ
Introduce the flag (§4), build the three targets, and stand up the oracle:
`NDS_TASK29_GX_CENSUS` already records the GX command stream at profile level 1.
Extend it to diff a profile-0 stream against the profile-1 stream for the same
frame, modulo matrix representation.

- **Gate:** profile 1 ROM byte-identical to today's published ROM. The differ
  reports 0 divergences when profile 0 is a pass-through.
- **Why now:** every later task is judged by this differ. Building it before any
  native code exists is what keeps "it looks right" from becoming the standard.

### Task 50 — Hardware divider and square root *(independent of everything else)*
Route hot divides and sqrt through `REG_DIVCNT` / `sqrtf32`, starting with
`__ieee754_sqrtf` (0.88%), `__aeabi_fdiv` (0.75%), `__udivsi3` (0.25%).

- **Budget:** −1.5% to −2.3% of total cycles ≈ −25,000 to −38,000 P50 ticks.
- **Gate:** Task 9 state hash **exact** — hardware and software divide must agree
  bit-for-bit on gameplay values, or the call site is not eligible.
- **Note:** the DS divider is asynchronous with a busy wait; a naive swap can be
  *slower* at low call density. Measure per call site, not wholesale.
- Runs in parallel with 48; it needs no new pipeline and no owner decision.

### Task 51 — Dream Land as one native program
Bake the stage into packed GX display lists — one per material bucket, model
space, materials pre-resolved into `POLYGON_ATTR`/`TEXIMAGE_PARAM` words inside
the list, textures VRAM-resident with names resolved at load. Per frame: load
camera, submit lists, patch the handful of live words (Whispy eyes/mouth, water
UV, flowers).

- **Budget:** STG P50 **597,632 → ≤120,000** (−478,000).
- **Gate:** GX differ 0 divergences on the static bindings; owner visual
  approval; retail A/B pair.
- **Kill:** > 200,000 after one measured attempt.
- This is the largest single win available and the one Task 48 unblocks.

### Task 52 — Mario and Fox as rigid-part native models
Per-part model-space packed lists plus a per-frame bone matrix palette.
Smash 64 fighters are hierarchical rigid parts — no arbitrary weighted skinning
is needed, which is why this is tractable at all.

- **Budget:** FTR P50 **590,144 → ≤250,000** (−340,000).
- **Gate:** Task 9 state hash exact (this must not touch gameplay state); GX
  differ on the pose stream; owner visual approval.
- **Kill:** > 350,000 after one measured attempt.
- **Watch:** Task 46 established the fighters do **not** run through the GObj
  process graph in the shipping profile — the seven
  `ndsFighterMarioFox*RunVSBattleUpdate` loops are 2-byte stubs. Find the real
  fighter draw entry before designing against the wrong one.

### Task 53 — The P95 tail: SRC and OTHR
Independent of the native renderer, and **required** for 30 FPS. SRC P95 955,904
vs P50 324,096 is a 3× spread; OTHR P95 546,624 vs P50 134,080 is 4×. Something
bursty lives in both.

- **E0:** census what makes the spike frames different. Candidates on the record:
  the double source update per presentation, BGM refill (Task 30 was reverted on
  exactly this histogram), `memcmp` at ~3,900 calls/frame, `_svfprintf_r`
  (0.25%, and there should be no string formatting in a battle frame at all).
- **Budget:** SRC P95 −400,000, OTHR P95 −300,000.
- Do not start before Task 48 reports; do not skip it because the median looks
  good.

### Task 54 — Textures as indices
`ndsRendererHardwareResolveOrBindTexture` is the single largest symbol in the
program at 21.8M cycles (4.36%); the texture group totals 11.38%. The withdrawn
Task 47 census measured **100% cache hit, 0 uploads, 3,654 key constructions per
63 frames** — we hash and look up keys for textures that never change. With
build-time names baked into the display lists, this becomes an array index.

- **Budget:** −8% to −10% of total cycles. Largely *subsumed* by Tasks 51/52 if
  those bake names into the lists — schedule it after, and only if residual
  cost survives.

### Task 55 — HUD to OAM
HUD P50 is 1,024 and P95 is 198,272 — it is nearly free most frames and then
costs a fifth of a VBlank. Preloaded digit tiles, OAM stock icons and markers,
dirty writes only.

- **Budget:** HUD P95 → ≤20,000.
- Small in the median, real in the tail; sequence it with Task 53.

---

## 6. Sequencing

```
Task 48 (kill criterion) ─┬─▶ Task 51 (stage)  ──▶ Task 52 (fighters) ──▶ Task 54 (textures)
                          │
Task 49 (axis + differ) ──┘
Task 50 (hw divider)  ── parallel, independent, ships on its own
Task 53 (P95 tail)    ── parallel after 48 reports; required for 30 FPS
Task 55 (HUD OAM)     ── with 53
```

**Arithmetic if 51 + 52 + 50 land at budget:**
P50 1,680,448 − 478,000 − 340,000 − 30,000 = **832,000** — under the 1,119,000
two-VBlank line with 25% headroom. The median reaches 30 FPS.
**P95 without Task 53:** 2,800,512 − ~830,000 = 1,970,000 — still 1.8×. The tail
is not optional.

---

## 7. Standing constraints this plan is written under

- Gameplay stays bit-exact. Every native task is gated on the Task 9 state hash
  being exact, not "close." Presentation targets ~90% likeness with the owner as
  the visual oracle (`render-fidelity-doctrine`).
- `decomp/` is read-only. That protects the reference, not the algorithm —
  DS-native equivalents are the point of the project.
- The tick-HUD ROM is rebuilt with the published ROM, every time, and
  `check-tickhud-parity.ps1` gates it.
- Device evidence is the 2/3/4/5+ VBlank histogram normalized by sample count.
  Never min-FPS. Perf is reported P50/P95, never a mean on a bursty bucket.
- KEEP ships enabled in the published profile at merge time. STOP/REJECT/WIP do
  not merge. No agent pushes to master, ever.
- Back up before major edits. Time-box open-ended debugging to ~10 emulator runs
  or ~1 hour, then checkpoint and report.

## 8. Decisions — ruled by the owner, 2026-07-23

1. **Renumbering — additive.** `NDS_BATTLE_PROFILE` is adopted as specified in
   §4. `NDS_RENDERER_PROFILE_LEVEL` keeps its current values and is documented
   as the renderer instrumentation level within profiles 1 and 2. No literal
   renumber.
2. **Task 46 route 1 — deferred.** The `src/import/battleship_sys_objman.c`
   include-seam question is not ruled on now. Task 46 stays checkpointed at E0
   on `codex/task46-gencached-schedule`; do not start E1. If a native task needs
   the same boundary, raise it then rather than assuming this deferral answers
   it.
3. **Start order — Task 48 first.** Spec written:
   `ClaudeFable5_Task48_ModelSpaceStreamCensus_20260723.md`. Task 50 may start
   in parallel; Tasks 49, 51, 52 wait on Task 48's verdict.
