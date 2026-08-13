# The default-off flag sweep, and what the instrument built to check it found

**No build, no emulator run, no runtime source change.** Every figure below comes
off two artifacts already on disk plus the Makefile and the linked ELF:

- `../2026-08-12_c123-rebank/profile/` — `build-c123-profile`, `BOTH_CPU 1`,
  `NDS_TICK_HUD_DRAW 0`, `regions=1601`. Ticks/frame = `cycles / 3202`
  (`../2026-08-13_c-residue/RESIDUE.md` §0).
- `../2026-08-12_c130-fire-gate/c130-gate-rows.csv` — the 1,600-sample
  `BOTH_CPU 1` gate run on current code.

New tooling, in tree: **`scripts/analyze-inline-attribution.py`** ranks a profile
by **innermost inlined function across every caller**, which no existing
instrument did — `census.txt` and `analyze-symbol-line-profile.py` both rank
**symbols**, and a `static inline` helper has none. `c123-pc-cycles.csv` here is
its cache: the whole profile collapsed to 98,346 rows of
`pc,cycles,instructions,symbol`, a **79-second one-pass replacement for the
2.6 GB scan**, so every later question is a small-file read.
`global-inline-attribution.txt`, `gxrecord-and-texvalid-owner-line.txt` and
`glbindtexture-owner-line.txt` are its outputs and are reproducible with:

```
python scripts/analyze-inline-attribution.py --build builds/build-c123-profile \
  --profile artifacts/performance/2026-08-12_c123-rebank/profile \
  --cache artifacts/performance/2026-08-13_c-flagsweep/c123-pc-cycles.csv [--function X]
```

**Resolution stability, measured (`--verify-resolution`): `addr2line` is NOT
bit-deterministic here.** 57 of 98,346 addresses are named inconsistently
between two identical runs — nested inlines sharing a parent's `low_pc` — and
they carry **1,514,572 cycles = 473 tk/frame in total, across the whole table**.
Every figure this document rests on (14,577 / 9,369 / 5,544) is 10x or more above
that whole-table bound and both large rows reproduced across runs; **a row under
~500 tk/frame from this instrument is not resolvable.** Unlike the tick sampler,
a small delta between two runs of this tool is noise, not a finding.

---

## Outcome, in one line each

- **Task A — the flag seam is EXHAUSTED, and the sweep is now complete rather
  than partial.** 169 `?=` flags, 41 overridden by the published block, 128 at
  default (77 zero, 51 non-zero). **Not one default-off flag carries an
  unshipped whole-match positive of ≥8,000.** The board's cycle-105 verdict
  ("the dormant-flag seam is EXHAUSTED … do not re-audit the flag list") is
  confirmed on a wider enumeration and on the corrected gate arm.
- **The one prize-class find is not a `?= 0` flag at all** — it is an unshipped
  **level** of a shipped flag, `NDS_R2_CAMERA_MATRIX_LEAN=3`: measured on the
  whole-match one-binary route, never shipped, and worth **−1,536 FTR P95** over
  the shipped level 2. Blocked on a deterministic red Boundary. See row 13.
- **Task B — the composition exists, its ceiling is 20,562, and its realistic
  band straddles 16,000.** Three per-frame re-discovery taxes the residue's
  symbol table could not see, because **all three are inlined helpers with no
  symbol of their own**. §4.

---

## 1. What is actually shipped

| | count |
|---|---:|
| `NDS_*` flags with a `?=` default in the Makefile | **169** |
| overridden by the `smash64ds-battle-playable-hwtri` block (`Makefile:1310-1528`) | **41** |
| left at their `?=` default in the shipped/gate configuration | **128** |
| …of those, default `0` (OFF) | **77** |
| …of those, default non-zero (already ON, or a parameter of another flag) | **51** |

The 41 are exactly the `override` lines of that block; the counts above are
reproducible by matching `^\s*NDS_[A-Z0-9_]+\s*\?=` against the block's
`override` set.

**The 51 non-zero defaults are where the shipped optimizations live**, and every
large one is already on: `NDS_R2_FIXED_SQRT` (1 — R2-07 L10 turned it on, which
is the "flat mean can be a gate lever" precedent this cycle was told to re-apply
and it has already been discharged), `NDS_TASK56_FIGHTER_PRIMITIVES` (2),
`NDS_TASK104_STAGE_STATS_ELISION` (1), `NDS_FTR_PLAN_ROUTE` (1),
`NDS_R2_PARTICLE_CAMERA_CACHE` (1), the whole RebirthHalo family, the Results
pair, `NDS_TASK82/85/86`, `NDS_TASK9_FLOAT_ITCM/PHASE2`.

---

## 2. The 77 default-off flags, classified

**65 of 77 are instrumentation and cannot be gate levers by construction** —
censuses, probes, falsifiers, proofs, phase timers, oracles, differs, lab
suppressors that produce an unplayable ROM, arm/harness selectors, and masks
belonging to another flag. Turning any of them on *adds* work. Named, so nobody
re-derives the list:

`BGM_FALSIFIER_OFF` · `FIGHTER_ANIM_AUDIT` · `FREEZE_DIAGNOSTICS` ·
`FTR_PLAN_VERIFY` · `LAB_CULL_PROBE` · `LAB_NO_CULL` · `LAB_TINT_SHIFT` ·
`R204_FPSHUD_SHADOW` · `R2_ANIM_CENSUS` · `R2_ANIM_CUT_ROUTE` · `R2_BOTH_CPU` ·
`R2_COLLISION_L7_ORACLE` · `R2_DRAW_SUPPRESS_MASK` ·
`R2_FIGHTER_EPOCH_STATE_PROOF` · `R2_FIGHTER_RUN_PROOF` ·
`R2_FIGHTER_SHADE_PROOF` · `R2_FIGHTER_SHADE_SKIP` ·
`R2_FIGHTER_SOFT_LIGHT_KEEP` · `R2_FIGHTER_STATESPAN_SKIP` ·
`R2_FIREBALL_MAP_COLL_DEBUG` · `R2_FLASH_PROBE` · `R2_KO_STRESS` ·
`R2_LOADFRAME_TIMING` · `R2_MP_ROUTE` · `R2_POSITION_PROBE` ·
`R2_REBIRTH_HALO_PHASE_PROFILE` · `R2_RELOC_ALIAS_ROUTE` ·
`R2_RELOC_FIXUP_TIMING` · `R2_SECOND_ENTRY_DIAG` · `R2_SOAK_MATCH_MINUTES` ·
`R2_SPAN_LEAN_TIMING` · `R2_STAGE_ACTORS_PROOF` · `R2_STAGE_ROUTE_PROBE` ·
`R2_STRIP_ROUTE` · `RENDERER_BENCHMARK_MODE` · `RENDERER_HW_DEBUG_TEXTURE_ONLY` ·
`RENDERER_M2_DETAILED_LEDGER` · `RENDERER_M3_PHASE0_PROFILE` ·
`RENDERER_SCREEN_SPACE_CENSUS` · `TASK103_STAGE_RUN_PHASE` ·
`TASK10_HARDWARE_CALIBRATION` · `TASK20_STACK_PROFILE` ·
`TASK22_WALLPAPER_RUN_LAB` · `TASK29_GX_CENSUS` · `TASK34_STAGE_STREAM_CENSUS` ·
`TASK37_LAYOUT_PROBE`(+`_ITCM`) · `TASK37_PROFILE`(+`_PER_FRAME_REGION`,
`_RESULTS`) · `TASK49_GX_DIFFER` · `TASK68_FALLBACK_CENSUS` ·
`TASK75_LOAD_CENSUS` · `TASK90_SHADE_CENSUS` · `TASK91_DRAW_PHASE_CENSUS` ·
`TASK93_TEXKEY_CENSUS` · `TASK9_FLOAT_CENSUS` ·
`TASK9_FTSTRUCT_SNAPSHOT`(+`_UPDATE`) · `TASK9_STATE_HASH`(+`_SKIP_CONTROLLERS`) ·
`DREAMLAND_CARD_CULL_MASK0/1` · `ENABLE_INISHIE_SOURCE_SCALE_SETUP` ·
`IMPORT_BATTLESHIP_MPPROCESS_PRIVATE`.

**Three of those deserve a note, because their measured positive ALREADY SHIPS
at the `0` default and reading them as dormant is a trap:**
`R2_STRIP_ROUTE` (strips ship through `TASK56_FIGHTER_PRIMITIVES=2`),
`R2_MP_ROUTE` (the `ndsMPFindLineEndpoints` memo ships; at 0 the route test
folds away), `R2_RELOC_ALIAS_ROUTE` (slice 45's −12,160 reorder ships; at 0 the
ROM carries no route check). At `1` each ROM is **slower**, by exactly the route
test the flag adds.

### The 12 remaining, each with its verdict

| # | flag | default | banked verdict + provenance | engages on current code? | honest gate-arm expectation |
|---|---|---|---|---|---|
| 1 | `NDS_TASK55_STAGE_GEOM` | 0 | **(a) REFUTED TWICE.** `Makefile:141-160` and PERF_LEDGER "Task 55 … STOP": lossless in the replay buffer, and still `STG −4,224` against `OTHR +7,616` — a COLOR/TEX_COORD write sets a state register and triggers no vertex transform, so the stage floor is the 606 `VERTEX16` transforms and nothing else. **And the owner's visual A/B found surfaces PULSATING IN COLOR at 1.** | **YES.** `include/nds/nds_renderer.h:191` requires `TASK36_HW_COMPOSE==2`, which the published block sets; sites at `nds_renderer.c:5564/5931/6170` sit inside the Task 36 capture window that Task 53 keeps live. The "20.6% fewer replay words" claim is still true and still worth nothing. | **0.** Wrong sign at the whole-frame level, and visually rejected. |
| 2 | `NDS_TASK51_STAGE_NATIVE` | 0 | **(a) REFUTED on performance**, board cycle 109: `P50 +13,376`, `STG +2,368`, **88 frames lost from 30 FPS**. | yes (`nds_renderer.c:29992/30736`) | **0** — known blocker. |
| 3 | `NDS_DREAMLAND_DS_MESH` | 0 | **(a) REFUTED.** Not "needs the owner's visual A/B" — it **had one and failed it** (Task 62, reverted 2026-07-25); two checkers enforce `=0`. | yes (`dreamland_ds_mesh.generated.inc`) | **0** — visually rejected. |
| 4 | `NDS_R2_UNLIT_VERTEX_EPOCH` | 0 | **(a) REFUTED.** `Makefile:415-420` + the E32 comment: E62 proved it **emits packed normals and is visibly worse**; the Makefile says do NOT enable it as a fix for the missing hurt-flash white. | yes, but requires `R2_MATERIAL_DYNAMIC` | **0** — visually rejected. |
| 5 | `NDS_RENDER_ECONOMY` (+`_OWNER_MASK ?= 32`) | 0 | **(c) MEASURED POSITIVE ON AN UNUSABLE WINDOW.** PERF_LEDGER `TASK11-SCREEN-CENSUS-STAGE-ECONOMY-20260717`: owner 5 (Whispy mouth) is the one census-ranked cut that passed the 500-pixel ratchet — **117/49,152 meaningful pixels, `−7,072` stage / `−7,104` active P50**. Owners 6 and 1 both REVERT. But that figure is an **8-frame idle window (600..607), 2026-07-17** — pre-2026-08-04 era boundary, pre-Task-53 replay, and pre the R2-02 stage cuts that took `STG` from ~478K to **174,656**. | **YES** — `nds_renderer.c:31810` + `:31961`, selected once per frame from the real `game_status`, `check-gbi-decode-fixtures.ps1:2433-2434` pins the shape. | **≈ −2,600 at best.** The lane it cuts has shrunk 2.7x since the number was taken, and it changes visible pixels, so it is an owner call for less than a fifth of the bar. |
| 6 | `NDS_R2_SHIELD_QUAD` | 0 | **(c) UNUSABLE WINDOW, and the owner already chose.** `Makefile:866-875` asks for exactly this re-price; the owner bought the model route on 2026-08-04 (*"36k p95 is worth it for correctness"*) and that 36k came off a **128-frame** window. | yes (`battleship_efmanager.c:1311/1545`) | **unquantified, and structurally small.** The shield is an event lane, not a flat one — per RESIDUE §1 a proportional cut in a bimodal lane returns less than its mean. Re-pricing it costs a build to answer a question the owner has already answered once. |
| 7 | `NDS_DREAMLAND_CARD_CULL` (+ 2 masks) | 0 | **(e) NEVER PRICED IN TICKS.** `Makefile:213-226` gives only geometry: `cheapest10` = 19 tris / 10.9%, `cheapest16` = 36 tris / 20.6%. Explicitly *"lets the owner see what an authorised scenery reduction would actually cost before deciding"*. | **YES**, and correctly: the mask is **baked**, so the Task 36 capture omits the culled runs and replay replays the reduced stream (`nds_renderer.c:31982`). | **bounded above by the no-Z band, 22,510** (RESIDUE §6 rung 3). 20.6% of stage triangles cannot pay more than 20.6% of the band's emit ≈ **4,600**, plus whatever run-level scaffolding a whole skipped run removes. Owner-gated (visible scenery). |
| 8 | `NDS_R2_PATH` | 0 | **(e) never measured as a lever.** R2-01's specialized battle loop; `Makefile:526-530` says the family stays 0 and the published ROMs stay pure Runtime 1 **until plan S5**. Setting it to 1 also gives the shared taskman_seam helpers external linkage, so the 0 arm is byte-identical to a build without the family. | compiles; fails the build closed on any non-`battle_playable` harness | **not a cut.** It is a *loop* substitution with no measured delta; SwitchPlan reserves it for the switch. |
| 9 | `NDS_R2_FTANIM_DENSE` | 0 | **(b) SUPERSEDED / no reader.** `Makefile:2148-2157`: *"DEFAULT OFF, and it must stay that way until a reader exists … turning this on without the runtime bind adds ROM and changes nothing else."* Slice 32's AOT animation bake is **SIZE dead** per HANDOFF. | asset generation only — **no runtime bind** | **0** — +3.1 MB of ROM, zero ticks. |
| 10 | `NDS_R2_MATERIAL_DYNAMIC` | 0 | **wrong sign.** R2-03 E47 *derives* material colour and the use-material predicate per epoch instead of reading a baked flag — 46.4 calls/frame of added work. It is a **correctness** lever for E32's dark-maroon hurt flash, not a perf one. | yes | **negative.** |
| 11 | `NDS_R2_REBIRTH_HALO_PACKED_PROJECTED` | 0 | **(e) lab-only** — prices FIFO/DMA savings while retaining the exact software projection contract. | yes | **~0 on the gate arm.** RebirthHalo is post-KO; the gate's rank-80 frames are mid-match. |
| 12 | `NDS_R2_REBIRTH_HALO_SPLIT_NOZ` | 0 | **(e) optional companion** to the accepted `SPLIT_MTX`, preserving synthetic painter Z. | yes | **~0**, same reason. |

### 13. The prize-class row is not in that table

`NDS_R2_CAMERA_MATRIX_LEAN ?= 2` is **non-zero**, so a `?= 0` audit cannot see
it — and `Makefile:950-992` records a **class-(d)** result:

```text
level 2 vs 1    +W3s        WORK-H P50 -1,664  FTR P50 -3,584 P95 -3,776   <- SHIPPED
level 3 vs 1    +W3s+W2b    WORK-H P50 -3,200  FTR P50 -5,248 P95 -5,312   <- NOT SHIPPED
```

Measured on the **whole-match 1,600-frame both-CPU tick-HUD arm, one binary,
route poked at the first frame marker** — the strongest instrument this campaign
has. Level 3 is **−1,536 FTR P95 better than what ships**, and it is held for a
correctness reason, not a perf one: W2b drops `syMatrixAdvanceW`, which stops
consuming 64 bytes of `gSYTaskmanGraphicsHeap` per call and **moves every later
allocation in the frame**, so route 3 fails the Boundary realtime verifier's
locked-30 phase accounting (`phaseLag=-1`) deterministically on the same ROM
that passes at route 2.

**Verdict: correctly held, and 10x under the bar.** Recorded so the next reader
does not rediscover it as a lever. It is also the sweep's methodological point —
**auditing `?= 0` is not auditing the flags**; three of the levels worth checking
(`CAMERA_MATRIX_LEAN`, `TASK56_FIGHTER_PRIMITIVES`, `REBIRTH_HALO_PACKED_FIFO`)
are graded, and only one of them had an unshipped rung.

---

## 3. Class counts

| class | count | flags |
|---|---:|---|
| instrumentation — cannot be a lever | 65 | §2 list |
| **(a)** measured NEGATIVE / refuted | **4** | `TASK55_STAGE_GEOM`, `TASK51_STAGE_NATIVE`, `DREAMLAND_DS_MESH`, `R2_UNLIT_VERTEX_EPOCH` |
| **(b)** measured positive, superseded/absorbed | **1** (+3 route flags whose positives ship at `0`) | `R2_FTANIM_DENSE`; `R2_STRIP_ROUTE`, `R2_MP_ROUTE`, `R2_RELOC_ALIAS_ROUTE` |
| **(c)** positive on a pre-2026-08-04 window — UNUSABLE | **2** | `RENDER_ECONOMY`, `R2_SHIELD_QUAD` |
| **(d)** positive on the whole-match instrument, NEVER SHIPPED | **1**, and it is a graded level, not a `?= 0` | `R2_CAMERA_MATRIX_LEAN=3` — **−1,536**, blocked on a red Boundary |
| **(e)** never measured | **4** | `DREAMLAND_CARD_CULL`, `R2_PATH`, `HALO_PACKED_PROJECTED`, `HALO_SPLIT_NOZ` |
| wrong sign | 1 | `R2_MATERIAL_DYNAMIC` |

**No (d) or (e) candidate reaches an ≥8,000 expectation**, so no flag earns the
design note the brief reserved for that class. Known blockers behaved as
briefed: `TASK37_ITCM_PORT` is bit 2 of `NDS_TASK37_ITCM_LEAVES` (shipped at
`7`, all seven leaves — the `?= 0` in HANDOFF's slice-49 note refers to the
*derived* `Makefile:1236` variable in a non-shipping configuration),
`R2_FIGHTER_GX_COMPOSE` is forced `0` in all four target blocks,
`TASK51_STAGE_NATIVE` refuted, `TASK56_FIGHTER_PRIMITIVES=2` ships.

---

## 4. Task B — what the sweep's instrument found instead

Checking "does this flag still *engage*" meant attributing the profile by
**innermost inlined function** rather than by symbol. That is a different
ranking, and it exposes three taxes **that do not appear anywhere in
`RESIDUE.md` §3 because they have no symbol of their own.**

### 4.1 `ndsRendererTask29GXRecord` — 14,577 tk/fr, and it would rank 6th in the ROM

It is `static inline` and lands inside every GX writer. Its two live lines, from
`gxrecord-and-texvalid-owner-line.txt`:

| line | source | tk/fr |
|---|---|---:|
| `nds_renderer.c:1281` | `NDS_TASK36_REPLAY_RECORD(...)` → `if (sNdsRendererTask36CaptureActive != 0u)` | **8,219** |
| `nds_renderer.c:1284` | `if (sNdsEffectPacketArmed != 0u)` — **`#if NDS_TICK_HUD` only** | **6,272** |

**Line 1284 is a THIRD apparatus lane.** The published ROM builds
`NDS_TICK_HUD=0` and executes none of it; the gate ROM does. RESIDUE §5's
apparatus figure rises from 18,675 to **≈24,947 tk/frame**.

**Line 1281 is product work, and 2,681 of it is provably dead.** Task 36 capture
is armed only around a *stage run*
(`ndsRendererTask36ReplayCaptureBeginRun`/`…EndRun` bracket inside
`ndsRendererCommitNativeStageSegment`, which faults on a non-stage run index),
so the flag is **FALSE at every fighter corner and every effect quad** — the
argument is already written out at `nds_renderer.c:1624-1646`, where slice 1
built `NDS_RENDERER_GX_RECORD_FIGHTER` and capture-free writers for the fighter
*vertex/colour/texcoord* path and the split matrix loader. **Five call sites
were never converted:**

| owner | tk/fr on line 1281 | side |
|---|---:|---|
| `ndsRendererNativePrepareProductionRun` | 1,064 | fighter |
| `ndsRendererNativeEmitProductionPrimitiveGroups` | 728 | fighter |
| `ndsRendererNativeEmitProductionCrossRun.constprop.0` | 411 | fighter |
| `ndsRendererPrepareWhispyQuadState` | 163 | effect |
| `ndsRendererR2WriteLightVector` | 115 | fighter |
| `ndsRendererSubmitParticleQuad` | 89 | effect |
| `ndsRendererExecuteNativeFighterOwnerProduction` | 73 | fighter |
| `ndsRendererSubmitNativeImpactWave` | 38 | effect |
| **subtotal** | **2,681** | |

A further **1,669** sits in four genuinely shared helpers (`HardwareEndBatch`
758, `LoadHardwareMatrixPair` 427 — it has a stage caller at `:30189`, so it is
*not* fighter-only, `BindTextureName` 338, `ApplyTextureParams` 146); splitting
those by caller costs text at 1.85 cyc/byte and should be priced separately.
The remaining **3,812** is stage/replay, where the flag can genuinely be true —
for the one capture in a 3,600-tick match.

### 4.2 `ndsRendererNativeStagePreparedTextureValid` — 9,369 tk/fr

This is R2 SwitchPlan §2's **"per-frame texture identity proof"**, named in the
charter as scaffolding Runtime 2 exists to delete, and it is still running:
**≈195 calls/frame at ≈48 ticks each**, re-proving that a prepared stage run's
texture cache entry still holds what the run recorded.

```c
entry = prepared->texture_entry;                                  /* 1,332 */
if (prepared->textured == FALSE) { … }                            /* 2,009 */
return ((entry != NULL) && (entry->ready != FALSE) &&
        ((u32)entry->name == prepared->texture_name) &&
        (entry->key_generation == prepared->texture_generation));  /* 4,487 */
                                                                  /* +1,395 tail */
```

**It is not the compares — it is 7.3 to 10.9 cycles per instruction**, i.e.
almost entirely cache-miss cost on two different arrays (`prepared[]`, then the
texture cache entry). Same shape as slice 44, whose lever turned out to be
"stop touching the objects" and paid **−35,904 P95**.

The design is already half-present in the data: `entry->key_generation` is
compared against a single global `sNdsRendererHardwareTextureKeyGeneration`
epoch. **Invalidate at the writer** (this repo's own standing pattern) and one
epoch compare per frame replaces ~195 four-load proofs.

**Risk, stated up front:** Task 103 E7 removed a clear whose lines the following
copy still touched and realised **28%** of its prediction (`Makefile:463`, Task
104's comment). If the bind path touches `entry` anyway, part of this 9,369
relocates rather than disappearing. Realistic band **2,600 (28%) to 9,000**.

### 4.3 `glBindTexture` — 5,544 tk/fr of libnds `DynamicArray` lookup

**54.9 binds/frame at ≈101 ticks each**, resolving a GL name to hardware words
before writing `TEXIMAGE_PARAM`/`PLTT_BASE`. The replacement already exists in
this tree for three textures: `NDSRendererWhispyNativeBinding`
(`nds_renderer.c:4439-4456`) *"capture the exact two hardware words and the
matching libnds active-name state once at upload"*. Generalising it to the
stage/fighter binds is exactness-preserving by construction. Expect
**~4,500-4,800**.

### 4.4 The verdict

| leg | lane | ceiling | realistic |
|---|---|---:|---:|
| A — texture identity proof → one epoch compare | `STG` (flat) | 9,369 | 2,600-9,000 |
| B — `glBindTexture` → cached hardware words | `STG`+`FTR` (flat) | 5,544 | 4,500-4,800 |
| C — Task 36 capture test off the 8 fighter/effect GX sites | `FTR` (flat) | 2,681 (+1,669 shared) | ~2,700 |
| **total** | | **20,562** | **9,800-16,500** |

`FTR` and `STG` are the only lanes flat at the percentile, and flat deltas in
them **add exactly** (RESIDUE §1), so the sum is the P95 delta and not a model.

**Answer to the brief's question, stated honestly: NO — there is no ONE change
that removes ≥16,000, and this is not one either.** It is three, and its
realistic band straddles the bar with leg A carrying a documented 28%-realisation
precedent. What it is: **the first candidate this campaign has found that is
≥16,000 at its ceiling, entirely exactness-preserving (no pixel, no gameplay, no
allocation, no frozen float), and on no DO-NOT-RETRY list** — and it is one
coherent theme, the charter's own *"delete the scaffolding upward"*, so it is one
commit and one A/B rather than three.

**Sizing law it obeys, and the two it does not need:** all three legs *delete*
code, so the 1.85 cyc/byte text tax is negative, not positive; none of them
touches `gm*`/`mp*`/`ftMain*`/`ftComputer` float; none allocates. Leg C is a
pure `#if` extension of a pattern already shipped in the same file.

**Why the residue missed all three:** `RESIDUE.md` §3 is a **symbol** ranking.
`ndsRendererTask29GXRecord`, `ndsRendererNativeStagePreparedTextureValid` and
`glBindTexture`'s per-call cost are inlined or library-resident and have no
census row. *Rank the inline attribution, not only the symbol census* — the
cached PC map in this directory makes that a 30-second question from now on.

---

## 5. What this cycle did NOT do

- No build, no emulator run, no runtime source change. Both root ROMs untouched.
- Did **not** measure legs A/B/C. Every figure above is c123-profile
  attribution; the P95 realisation of each is an A/B, not a prediction.
- Did **not** re-price `NDS_R2_SHIELD_QUAD` or `NDS_RENDER_ECONOMY` on the
  whole-match instrument. Both would cost a build to answer a question whose
  honest ceiling is under a fifth of the bar, and both change visible pixels.
- Did **not** open `NDS_R2_PATH`. It is SwitchPlan S5's, not a cut.
