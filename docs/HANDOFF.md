# Handoff

Updated: 2026-08-14. **THE GATE IS RE-BANKED ON THE SETTLED HEAD: raw 1,184,064 / net 1,159,117**
against 1,120,380 = **+63,684 raw / +38,737 net**; P50 939,136 (`build-c158-gate`, git `a159069af0d`,
`BOTH_CPU 1`/`DRAW 1`, DLDI ON, frames 440–2039, 1,600 samples, 123 s wall —
`…/2026-08-14_runtime2-p95-closure/GATE_ARM_OWNERS.md`). Apparatus 24,947 (`RESIDUE.md` §5), owner-
approved net scoring. **The −26,880 vs the old 1,210,944 is NOT a cost win — the matches differ
(P1 damage 58 → 76 after the Fox bore-84 fix), so no lane may be sized from it.** The old 1,087,296
"canonical both-CPU" was `BOTH_CPU 0` (`…_c126-armcheck/ARM_MISLABEL.md`); every `EXHAUSTION.md`
ceiling is dead. **The requirement is now 64,452 at the 80th-largest frame, not 91,844.**
**Boundary is GREEN (below), so the bank sits on a verified tree.**

## R2-07 `BUGS.md` — symmetry FIXED / OWNER-CONFIRMED; three P1 atlas-coverage cells remain open

`docs/BUGS.md` carries the owner's own wording and is the queue — do not reword it. The 2026-08-12 playtest
closed old rows 2/3, fire burn and Fox gun (`artifacts/bugs/2026-08-12_r2-07-cluster/`,
`…/2026-08-12_c130-fire-gate/GATE.md`), and the "all three rows converge on ONE texture-residency capability"
framing was wrong — never reinstate it. **A magenta bar beside Fox is the BEAM ITSELF**, relocData 316's
RGBA(219,0,134), not a debug quad. Rows 1/2 DECIDED in `…_c-residue/OWNER_DECISIONS.md` §1/§2.

- **Whispy — FIXED, OWNER-CONFIRMED 2026-08-14.** Dynamic stage matrices now validate every frame; direct A/B + owner eye closed it.
- **Fox — FIXED, OWNER-CONFIRMED 2026-08-14** ("perfect"). Shared beam+flash+collision bore **84**, radius **20**; crouch
  clears by **45.181**, standing overlaps; both facings/reflection stay world +84. ROM **3EBB8033…**; `…/2026-08-14_fox-bore84-collision/FOX_BORE_COLLISION_V5.md`.
- **Dream Land BG edges — FIXED, OWNER-CONFIRMED 2026-08-14.** DS-only centered **K=9/8** covers far-floor overscan;
  affine q16 scale metadata stays bit-exact. `artifacts/verification/2026-08-14_dreamland-bgstretch/`.
- **P1 VFX symmetry — FIXED, OWNER-CONFIRMED 2026-08-14.** MASKS/MASKT was missing from the common-atlas submit; **19**
  masked scripts reconstruct 2/4 pieces over the same rect; ledge FlashMiddle is script5/texture2/**MASKST**. ROM **8C7507F0…**; `…/2026-08-14_p1-vfx-texture-audit/`.
- **P1 coverage 4/11/14 — IMPLEMENTED 2026-08-14, NEEDS THE OWNER'S EYE ON A SHIELD BREAK AND A SIDE KO.** Source **32x32**, cell cap **64**, frame cap **1**, **same four 8 KiB sheets**. It was **packer waste, not VRAM** — 27,520 of 32,768 held, and the shelf packer's 5,248 free texels were 16-tall tails no 32x32 cell can use, so the **64 -> 32 cell-cap shortcut was never necessary**.
  Palettes went **one per sheet** (zero VRAM; `glColorTableEXT` already ran per sheet), which stops the extra cells pulling the shared k-means: **24 of 31 decode BETTER, 2 worse by <=0.002, none dropped**. 28/31/35/36 stay out **by name** (`QUAD_P1_DEFERRED`) — admitting 31+35 cost texture 33 **+39%**. ROM **2015FBD1…**; `…/2026-08-14_p1-texture-4-11-14/EVIDENCE.md`.
- **THE TREE IS SETTLED AND THE TWO REDS ARE ATTRIBUTED (2026-08-14).** `HEAD DID NOT COMPILE`: `33d7cc5d3b7` shipped `nds_renderer.c` with `mirror_mask` in `ndsRendererSubmitParticleQuad`'s definition while its declaration stayed in the worktree. All four owner-confirmed 08-14 fixes are now committed (`9b6c9e72a25`).
  **`check-gbi-decode-fixtures.ps1` IS GREEN.** Two separate faults, one hidden behind the other: the M3 consumed-field certificate still classified `workspace.slice44_validate_cursor` under `…PrepareNativeStageMatrices` after the Whispy fix removed that read (narrowed in the generator + manifest, the field still lives under `…PrepareNativeStageOwner` and the rigid guard); and `33d7cc5d3b7` respelled `ndsRendererScanList` to `NDS_R2_CENSUS_EVICTED_CODE`, so the five-hot-path count read four (`813207773c1` teaches the count the alias **and pins the alias to `NDS_RENDERER_HOT_CODE` at census 0**).
- **BOUNDARY IS GREEN 2026-08-14 — THE RED WAS A CORRUPT DLDI SD IMAGE, NOT A COMMIT, AND THE FIVE-COMMIT BISECT WINDOW IS REFUTED** (`artifacts/verification/2026-08-14_boundary-red/BOUNDARY_RED.md`; log `boundary-after-dldi-reset.log`, 0 `Exception:`, marker capture **27.8 s of the 120 s ceiling**). **No source byte changed.** `emulators/melonds/dldi.bin` (gitignored, 536,870,912 B) made the ROM load NOTHING — `sNdsRelocLoadedFileCount 0`, `gNdsRelocAssetPayloadReadCount 0` — so `gMPCollisionGroundData->wallpaper` stayed the raw token **`0x3eb`** and `lbCommonMakeSObjForGObj`'s `sprite->bmsiz` (`[0x41c]`) took a data abort. Calico's `__excpt_entry` then **disables the PU** and `blx`es junk slot `0x205`, so the ARM9 slides through zeroed RAM in ABORT mode (`cpsr=0x400000b7`, `r12=0x205`, `lr_usr=lbCommonMakeSpriteGObj+26`) and every capture times out. Three arms: 512 MB image = ABORT · DLDI **off** = SYSTEM (diagnosis only, DLDI-on is retail parity and ~29,696 P95) · **fresh 16,957,440 B image = SYSTEM and green**. Old image kept at `dldi.bin.broken-2026-08-14`. **The window is refuted by measurement, not argued**: `build-c-collfixed` (08-13 19:25) and `build-c156-vfxsymmetry` (08-14 10:44) — both BEFORE the window — abort identically. Eight of the eleven commits ship no byte anyway (`54d7d7862e4` is comment-only; `813207773c1` is a checker the Makefile never invokes). **`gdb-markers.ps1` now classifies a timeout**: gdb's attach line reading `in ?? ()` means the guest had already crashed, so do not raise the ceiling. **A second attach is impossible — melonDS's stub refuses every reconnection**; sample the PC with a FIRST attach instead. A probe that may be killed still needs `set logging enabled on`.
- **R2-08 IS ONE MAKEFILE LINE, STAGED NOT LANDED — §6 items 1 and 4 RE-MEASURED ON THIS TREE 2026-08-13**
  (`…/2026-08-13_c-r2path-recheck/SWITCH_READY.md`): Boundary green through `NDS_R2_PATH=1`; soak `NO-FREEZE`, 2 START rematches;
  gate arm 929,344/**1,204,352** vs control 924,864/1,210,944 — inside the floors, opposite in sign, invariants identical,
  **+80 B text**. Owner-only: `:391` retail test and `:385` the eye — take the eye AFTER the flip, Boundary shoots `PATH 0`.

## The two arms — same 60 s match; ONLY `BOTH_CPU 1` is R2-07's gate

| arm | role | `WORK-H` P50 | P95 |
|---|---|---:|---:|
| `battle_playable_realtime` (`BOTH_CPU 0`) | shipped, verifier GREEN | 899,136 | 1,087,616 |
| **both-CPU (`BOTH_CPU 1`)** | **R2-07 GATE, FAILS** | **939,136** | **1,184,064 raw / 1,159,117 net** |

**CADENCE IS READ FROM `NDS_TICK_HUD_DRAW=0` (owner, 2026-08-14; `plan.md` §6).** Measured on the
gate arm with no gdb stub attached: **`DRAW 0` VBI 2:1427 3:162 4:10 6:1, max 6, two-VBlank 89.1%**
vs `DRAW 1` 2:1359 3:229 4:9 5:1 6:1 10:1, max 10, **84.9%**. The HUD draw burst is worth **4.2
cadence points**, so the deficit against ≥95% is REAL and is not the instrument. Label every cadence
figure with its `NDS_TICK_HUD_DRAW`. That `DRAW 0` arm carries `NDS_TASK37_PROFILE=1`; a clean one
is one build away.

**LEDGER INDEX (2026-08-13, superseded as a bank, kept as a method)** — `…/2026-08-13_c-ledger-index/LEDGER_INDEX.md`. **The shield attach path paid for a SEARCH, not work**: each attach hit `ndsAObjEvent32FindNormalized`, a linear scan of a 1,177-entry ledger at 8.05 tk/iteration — **5,123 tk/attach, ~88% scan**. An O(1) index took P95 1,250,368 → 1,210,944, P50 flat, `SINT` −23,936, 40 frames 3→2, 12,667 oracle lookups 0 mismatches, A2 falsifier brackets control 2,752. The anim-joint fix's **+49,216 came back −39,424**, now +9,792.

**`analyze-tick-hud-excursion.ps1 -Ceilings` emits both arms' tables** (`…/2026-08-12_c130-fire-gate/LANES_BOTHCPU.md`); c125's ceilings are dead, and so is `SITR_NEXT_CUT.md`'s cut (`…/2026-08-13_sitr-aobj-layout/`). **`RESIDUE.md` §0 corrects all sizing. SLICE 50 SPENT §4 row 0 — leg A landed (195 calls/frame → 8 sweeps a match, `STG` −11,328 flat, viewport pixel-identical); legs B and C REFUTED, do not re-brief that row.** **B is blocked by RAM**: 828 B against the texture cache `_Static_assert`'s **72 B** of slack. **C does not exist** — slice 1 converted every per-corner writer; the residue is the shared `gl*` → `ndsRendererTask29Gl*` wrappers, which have stage callers, not an `#if`.
**Apparatus is 24,947 not 18,675** (third lane `sNdsEffectPacketArmed`, `#if NDS_TICK_HUD`, 6,272 on EVERY GX command), so the product-side gap is ≈62,300. **Owner package: `…/2026-08-13_c-residue/OWNER_DECISIONS.md`.** **RUNG 2
(quarter-rate particles) REFUTED, no build (`…/2026-08-13_c-particle-rate/`): `MISC` is a DRAW
residual** (`taskman_seam.c:5104`), so its 17,152 never priced the update half — which is `SRC`,
7,364 tk/fr, and pays **−7,493** quarter-rate (ALL particles, both halves, −33,818). **And it shares
ONE LCG with the level-3 AI**, so any cadence change diverges the match: **CHECK EVERY SUB-RATING
FOR `syUtilsRandFloat`.**

**A PROFILE CYCLE IS HALF A TICK.** `ticks/frame = cycles / (2 × regions)`; regions P50 2,240,838
cyc = 2.0001× two VBlanks, so `cycles/2` reproduces the gate arm's `ALL` histogram and every
`SITR`/`AObj`/`AnimValueQ`/`__aeabi_lmul` crumb halves and is gone (`RESIDUE.md` §0).
`analyze-symbol-line-profile.py` reads `regions` and prints its `basis`. Only `FTR`/`STG` are flat
where P95 lives (band min 296,320/171,520), so flat cuts there pay 1:1 and ADD.
**`…/2026-08-13_c-flagsweep/c123-pc-cycles.csv` caches that profile as 98,346 rows — a 79-second
one-pass replacement for the 2.6 GB scan. RANK THE INLINE ATTRIBUTION, NOT ONLY THE SYMBOL
CENSUS: the whole of `RESIDUE.md` §4 row 0 is invisible to a symbol ranking.**

**THE BIGGEST LEVER IS PLACEMENT.** Memory stall is **1,236,685,107 cycles, 33.8% of the match** ≈ 386,000 tk/frame — past `SINT`/`SHDT` by an order, and why slice 48's identical pair differed 94,976.

**SLICE 49 (reclaim dead ITCM) is REFUTED without a build — do not re-open it until the Task 37 port
group is understood.** `.itcm` is NOT full (30 of 82 residents never execute, 2,594 B idle), but the
census's 87,033,153 "in reach" stall cycles assume admitting ~3,118 B of mostly PORT functions, and
`NDS_TASK37_ITCM_LEAVES`' PORT bit is held on CORRECTNESS (owner: the enabled lab build misbehaves). **Eviction alone pays nothing.**

**Lane-sizing traps, now encoded in `-Ceilings`:** medians do not add (it invented a 110,336 lane in c122); `OTHR` CONTAINS `WAIT`; only `WORK-H` is spendable. A `-Ceilings` ceiling flattens a lane to its own MEDIAN, so it prices the EXCURSION only — not what deleting the lane pays (`FTR` 8,512 vs 311,744); `RESIDUE.md` §2 has both. **Profile with `NDS_TICK_HUD_DRAW=0` or you profile the instrument**; the GATE keeps `DRAW=1`, cadence comes from `DRAW=0`. Soak length is `NDS_R2_SOAK_MATCH_MINUTES`; `Makefile:382` forbids reporting a
both-CPU P95 as Boundary's. **Route to ATTRIBUTE, re-bank to BANK. Collision paid and is BANKED**
(slices 35/36/37, −10,752). Float in `mp*`/`ftMain*`/`ftComputer` is FROZEN; `gmcollision` was UNFROZEN
2026-08-13, but the LINKED ELF says only **37–52%** is reachable (`…_c-collision-seam/`, slice 52).

## What is dead, so nobody re-derives it

- **`SPRM` 13,056, `AUD` 13,824, `BG` 3,968 — closed by arithmetic 2026-08-13** on a WHOLE-MATCH MEAN; **`SPRM`'s excess on the 80 frames that SET P95 is +49,377 (25.8x presence)**, so re-price that one on the marginal frames before re-closing it. `SCPU` needs −32.1% and reads 896 on the rank-80 frame. **All 169 `?=` flags audited — ZERO unshipped wins (`…/2026-08-13_c-flagsweep/FLAG_SWEEP.md`).**
- **THE P95 EXCESS IS ~90% SIMULATION, AND IT REPRODUCES ON TWO DIFFERENT MATCHES** (`…/2026-08-14_runtime2-p95-closure/{MARGINAL_OWNERS,GATE_ARM_OWNERS}.md`; `scripts/census-tick-hud-p95-set.py` re-derives it in one command). On the 80 frames that literally SET P95 the excess over a 2-VBlank frame is **+508,993** (c147: +520,718), `SRC` **+456,480 = 89.7%**, **all inside `GCRA`** — `SRC−GCRA` = **−22** here, −68/−64 there: three populations, three zeroes. Nesting `SRC ⊃ GCRA ⊃ {SINT ⊃ SCPU, SHDT, SPHD/SPHC, SCAT, SPRM}`; never add a parent to its child. **But the ranking BELOW `SRC` is match-specific — only `SITR` survives**: `SITR` +171,234 → **+188,907**, while `SHDT` 119,920 → 80,837, `SPHD` 112,833 → 75,236, `SCPU` +7,222 → **−8,669**, and **`SPHC` +62 → +52,780 (59.3x)**. Size nothing off one match's events. **`FTR` 1.03x, `STG` 1.00x, `MISC` 1.23x — the draw side is 7.1%,** confirmed independently by the v3 profile's draw closure at **8.4%**. **Cut 64,452 at the 80th-largest frame.**
- **THE FIRST v3 STALL CAPTURE ON THE GATE ARM EXISTS** (`…/v3-gate-arm/`, `build-c159-profile-bothcpu`, `BOTH_CPU 1`/`DRAW 0`, 1,601 regions, 3.62 GiB, residual −15; reduced CSVs `gate-p95-pc.csv` + `gate-band-pc.csv` are committed so **nobody re-scans it**). Whole match tk/fr: icache 354,678 · dcache 269,944 · halt 214,226 · issue 208,689 · wbuf 48,914 · intlk 43,122 · bus 36,738 · dma 8,450. **On the 80 P95 frames the excess is icache_fill +155,795 (40.0%) · dcache +96,800 (24.8%) · issue +94,029 (24.1%)** — still no single owner, and **no function exceeds 3.6%**. **`plan.md` §7's hot-footprint re-admission test FAILS on its own condition: the draw closure holds only 11.5% of the icache excess** (+17,944 of +155,795) while the fighter-proc closure holds 41.4%. **Soft float does NOT promote**: 38.0% of the family's caller-attributed 74,283 tk/fr is inside the fighter procs, **31.2% is draw side**, largest single caller 5,402. **NEXT TARGET: in-match animation-asset load I/O — +93,436 on the 80, +51,276 after the outlier falsifier**, every symbol at 2.6–19.2x presence. **Land a per-frame asset-acquisition counter on the gate arm BEFORE any code.**
- **THE 160-FRAME CADENCE SET IS 64% INSTRUMENT.** 102 of the 160 are ALREADY under the 1,116,096 cadence boundary in `WORK-H` and 98 of those carry the HUD draw burst; only **58** are `WORK-H`-bound, needing a mean **43,916** (94,848 is the WORST frame, not the set). A burst frame presents in 2 VBlanks 3 times in 1,360. **The 123,773 write_buffer/interlock/bus pool reproduces to the tick and is NOT a lane** — largest holder 9.1%, and every top holder is already owned (GX FIFO, fighter draw, texture upload, or `tickGetCount`/`cpuGetTiming`, 13,406 tk/fr of apparatus). Only `memset` 10,570 + `ndsRendererSyncTextureTile` 3,945 are pool-shaped, and they sum under the floor.
- **NEWLIB FORMATTED I/O IS REACHABLE AT `NDS_TICK_HUD=0` — the "instrument-only" reason was wrong**, the verdict is not: `--gc-sections` keeps the family and `ndsRelocAssetFindEntry`'s NitroFS path builder calls `sniprintf` on every animation lookup. Bound **≤769 tk/frame**, 20x under floor. Closed on cost, not on reachability.
- **`FTR` — −93,612 landed (c116); its "0/80, NOT a P95 lever" verdict is BOUNDARY-arm and the gate
  arm's 8,512 is the EXCURSION ceiling, not what deletion pays (311,744).** DS-native AOT geometry
  ships (`NDS_TASK56_FIGHTER_PRIMITIVES ?= 2`) **SHIPPED BROKEN** — 35.6% of the fighter backfacing
  with Boundary green: **a passing verifier is not visual verification.**
- **Effect DObj submits** — Boundary-only. **Projectiles** · **texture thrash** · **`Find`** ·
  **`Material`** · force-load seam. **`MISC` is the DRAW residual, not "particles"**; particles are
  FLAT. **AOT animation bake** (32): SIZE dead. **In-match FGM I/O + `FindPlanned`**: closed
  2026-08-13, `SHDT` block above.
- **Animation playback ARITHMETIC** (slices 34, 41): idle-joint skip (33), lazy track table (31), AObj walk and dispatch all under the floor. **Slice 41 spent the last lever**: 30 Hz poses cost **+7,040** *and* diverged the match (damage 130/51 vs 33/65). **Don't blanket-convert `ndsBaseGcPlayMObjMatAnim`** — 5 tracks pack 0xRRGGBBAA in f32. STRUCTURAL LAYOUT cuts closed 2026-08-13; call count is the lever.
- **THE CALL FRAME IS CLOSED 2026-08-14. Its 64,863-tick ceiling does NOT convert: best package 10,544 cyc = 5,272 ticks, 3.0x short of gate** (`…/2026-08-14_call-frame-slice/CALL_FRAME_SLICE.md`, `scripts/census-frame-candidates.py`). **A split recovers the REGISTER DELTA, not the frame** — fitted `push(N)≈1.6+1.2N`, `pop+pc(N)≈5.0+1.6N`, so **~9.3 cyc/call survive any split** (pc-load flush). `ftGetStruct` corrected to −3,770 cyc, not −7,141. **`ndsR2AnimValueQ` (6,919, largest) is BLOCKED** — its `noinline,target("arm")` is measured: Thumb has no SMULL, that arm cost **WORK-H P50 +25,472**. **202 diagnostic-SHAPED names = 98,871 cyc hold only 7,849 cyc of real diagnostic**; `…RecordCapturedDisplay` is the stage hook, `ndsIFCommonRecordHUDState` drives the HUD (`nds_platform.c:2616-2760`), renderer `Record*` IS the RDP state machine. **Method: size against the LINKED ELF** — codegraph returned `#else` non-hwtri stub bodies and nearly bought a false 14,800-cyc lead. Still worth taking WHEN BUILDING ANYWAY: `ndsFighterDisplayContractCountFlags` 7,849 cyc = 3,925 ticks, no runtime reader; gate the traversal, keep globals `used`.
- **UNITS: 2 profile cycles = 1 tick.** The dcache census compared cycles against tick floors and overstated everything 2x; the DMA0CNT poll is **8,315 ticks, UNDER the ±8,544 floor**, not "at the bankable bar". **RETRACTED with it: "88.9% of data-load excess is cacheable" was the classifier FAILING OPEN** — an unresolved base became `cacheable`; fail-closed reads **cacheable 0.1%, unknown 91.7%**. Only 0.1% was ever PROVEN cacheable; that STOP stands, its basis does not.
- **THE v3 STALL ATTRIBUTOR IS ALREADY IN-REPO: `emulators/melonds-attributor/melonDS.exe`** (since 07-27, emits `profile-v3` with `icache_fill`). An earlier note said "never adopted" — WRONG, it checked `emulators/melonds/` only. Drive it with `run-task37-profile-census.ps1 -MelonDS emulators\melonds-attributor\melonDS.exe -Build <dir> -NoBuild -StartFrame/-Frames matching that build's config`. Source: `D:\Stuff\DevFolder\melonDS-Accurate` (`r2-stall-attributor`, `4a1abf61`).
- **CODE PLACEMENT CLOSED 2026-08-14 ON MEASURED TEMPORAL EVIDENCE** (`…/2026-08-14_icache-temporal/ICACHE_TEMPORAL.md`; nothing built). The earlier "33.6x union footprint" verdict was **WITHDRAWN** — a whole-match union does not govern cache behaviour, and "evicted between calls regardless" was asserted without measuring the interval. Geometry VERIFIED from `melonDS-Accurate/src/CP15_Constants.h`: 8 KB / 32 B / 4-way / **64 sets** / **round-robin** (melonDS names WAYS "sets"). **v3: `icache_fill` = 339,275 ticks/frame, 29.7% of match, 37.5% of non-idle, 1.87x `issue`** — 20x the target. **But CAPACITY, measured**: sweeping the cutoff so set population varies, sets that FIT the 4 ways refill at 1,252 fill/1k vs 1,233 for oversubscribed (top-256); 1,407 vs 1,297 (top-512). **Room in a set does not make its lines survive.** Phase 3 DID refute the old claim — 3 hot clusters fit and are scattered over 160–713 KB, hottest edge 413 KB apart sharing 10 sets — **but they hold 0.14% of conflict.** Model gate passed only after the FIRST falsifier scored −2.3%, beating the principled layouts (it padded by a line, spreading sets); rebuilt to align bases to the 2048 B period → +27.8%.
- **HOT FOOTPRINT IS THE LIVE LANE AND IT SIZES ABOVE THE BAR** (`…/2026-08-14_hot-footprint/HOT_FOOTPRINT.md`, census only). Of **288,352 B in the 9,011 lines the match pays to fetch**: live 213,040 (73.9%), **literal pool 5,780 (2.0%, NOT removable — Thumb-1 needs the constants)**, **cold code 42,892 (14.9%, the real lever)**, padding 26,640. **ALWAYS split the pool out before quoting "dead-in-line" — raw 26.1% reads as ~88,000 ticks and is a confound**; objdump shows pool words as instructions and no PC profile reports them. Ceiling 218,820 B needed = 6,839 vs 9,011 lines = **24.1% = ~81,800 ticks**; realistic third-to-half = **25,000–40,000 ticks**. **Two objects hold 42% of all fetch**: `scene_backend.o` (61,664 B fetched, 47.3% exec) + `nds_renderer.o` (55,008, 48.6%). Main text 914,634 B, only **43.4% in functions that execute at all**. `ndsR2AnimValueQ` spends **81.4% of its own cycles fetching itself**; `.text.hot`+`.text.hot.draw` = 9,844 B is already **1.2x the cache**.
- **NEXT EXPERIMENT: `-freorder-blocks-and-partition`** (cold blocks → `.text.unlikely`). THREE GATES FIRST: (1) changes codegen, so NOT "same objects" — Boundary must re-verify gameplay/collision/RNG; (2) `.text` will likely GROW while *fetched* text shrinks — right trade, but price it against the GObj-cap RAM threshold; (3) confirm `.text.unlikely` is non-empty in the map for `-mthumb` ARMv5TE and reaches those two objects — **if empty, STOP, no build spent**. Primary evidence = v3 `stall_icache_fill` delta, NOT WORK-H. **Closed cheaply: clone bloat is 168 B total.** **Not the fetch lever:** 66,488 B of never-executed objects survive `--gc-sections` — zero fill cost, a RAM question. **Chase:** `_vfiprintf_r`/`__ssvfiscanf_r` are in the hot fetched set — newlib formatted I/O running in a battle match.
- **The 20.12 kernels' ARITHMETIC (slice 42)** — sub-floor, non-additive. **Local-matrix memo dead twice.** **Flower rigid-mask +3,200, wrong sign.** **token→asset_id MEMO dead** (Task 74). **Six more lanes closed by MEASUREMENT**, numbers in `…/2026-08-1{1_c122,2_c123}-rebank/SLICE4{5,6,8}.md`: `ndsRelocFinalizeLoadedFile` as gate; anim-cache arena growth (Rejects 0); `OTHR` ceiling; **BGM sizing**; **every memo is healthy**.

## RAM — price a change before writing it

`check-boot-headroom.ps1 -Build <dir>` after every lab build. Highest `fake_heap_start` proven to
boot **`0x02294804`**, lowest to fail **`0x02294b24`**. **Text counts as much as bss**; a failing
arm reads as a hung emulator. `gSYTaskmanGeneralHeap` free-min **72,188**, floor 32,768.
**2026-08-14 RAM:** HW wallpaper decode cache **153,600 -> 135,000 B**; `.main.bss -18,592 B`; proof `fake_heap_start 0x02260c24` = **211,936 B proven headroom** (`…_ram-decode-cache/RAM_DECODE_CACHE.md`).
## Landed slices and the lanes they leave

**SLICE 43 WITHDRAWN 2026-08-11.** All targets force `NDS_R2_FIGHTER_GX_COMPOSE=0`; do not re-enable
without owner proof — it measured **−13,632 P95** but the matrix stack leaks ~3 pushes/frame,
wrapping mod 32 (`nds_platform.c:3197`, whose `|| NDS_TICK_HUD` is pinned by
`check-gbi-decode-fixtures.ps1:2247`). **SLICE 46 KEPT — 1,213,440 → 1,196,224** (`…/SLICE46.md`):
warm preload covered 57 of the 87 used ids; the measured 87, 4 per scene update, take **misses 32 →
2** and the arena 257,200 → 192,240 (it SHRINKS).

**SLICE 48 KEPT — read its SIZE, not its bank (`…/SLICE48.md`).** The FAT lane is **BGM**; **`AUD`
at 0.2% does NOT clear it** (a bucket brackets only its own thread). Shipped: worker created at
`MAIN_THREAD_PRIO + 1`, switched to `- 1` once playing; **deprioritizing during the MATCH was
REFUTED** (+8,064 wrong way). **SLICE 45 KEPT — 1,225,280 → 1,213,440** (`…/SLICE45.md`): resolves
16,002 → 1,143, **−12,160**. **The fighter LOCAL matrix build is NOT a P95 lane — refuted c122**;
the local-matrix memo is **DO-NOT-RETRY, killed twice**. **`SINT` is the fighter INTERRUPT proc with
`SCPU` nested, not an animation bucket** — mis-attributed an A/B in c119. **Zero-copy force-load is
closed:** `ftmain.c:4623` DISCARDS the return value.

**`SHDT` IS CLOSED — bar 47,424 tk/fr, not −26.6%: the band is the transform chain, four dirty flags
so nothing to memoise, band-only cuts saturate at 78,016, fixed point only**
(`…/2026-08-13_shdt-{broadphase,band-owner}/`, RESIDUE §4 row 1). **Its file-I/O co-fire is closed
too** (`…/2026-08-13_c-band-io/`): the **SOUND-EFFECT load, not the animation one** (anim prices **+0**).
**FGM IS CLOSED BY COUNTED MISSES, not inference (`…/2026-08-13_c-collision-stack/STACK.md`): 188 plays, 38 hits, 150 misses, 143 evicting resident data, ≤5 of 8 slots ever pinned. The victim rule's strict `<` made it a ONE-slot cache per size class; fixing that bought +9 hits = −764 P95-equivalent and REVERTED on its own falsifier. Working set 59 cues / 575,760 B vs 204,800, so a repartition extrapolates to ~−2,300 and TRADES class eligibility (dropped sounds). `FindPlanned` is REFUTED too — 1,188 entries a match, 13 tk/fr not 665; that profile PC range is `FindNormalized` inlined at a second site.**
A GATE LANE IS A MASK FOR THE PROFILE; a MECHANISM NEEDS NO MASK — sum over the region axis
(`analyze-io-lane-series.py`), worst-case-pairing first.

**Do not bring a micro-fix** — R2-06 E11: a load-frame-only ~8,000 cannot be banked. Clear ~16,000
in one change, or **use the `.data` route on ONE binary** (only if the change cannot alter gameplay
state). **Every change needs an engagement counter on BOTH sides**; slices 45, 46 and 48 were all
found by READING counters the code already kept, on the gate arm, for the first time. `.text.hot`
is closed both directions.
**R2-07 STRESS GATE PASSED — `…/2026-08-13_c-stress/STRESS_GATE.md`.** 3 successive matches, 5
entries, **Sudden Death UNFORCED** and played to a KO, `NO-FREEZE`, risk counters clean; the
five-minute match ran at 98.7% coverage for `WORK-H` 929,344 / **1,205,760** — **length does not
accumulate cost**. **BOTH ITS ANOMALIES ARE ATTRIBUTED, no build spent
(`…/2026-08-13_c-anim-anomalies/ANOMALIES.md`):**
- **Runaway — FIXED 2026-08-13 (`…/2026-08-13_c-animjoint-fix/ANIMJOINT_FIX.md`). There was no
  missing CLEAR; there was a missing SET.** `lbCommonAddDObjAnimJointAll` (`lb/lbcommon.c:785`) was
  an **empty stub** (`bx lr`), so `ftCommonGuardInitJoints` set `is_anim_joint` while every joint
  still held the GuardOn figatree. Five-minute arm: Figatree misreads **144 → 0** (it was 144 of
  144), runaway **50 → 0**; the runaway counter saw only 2/3 of the class — **48 of 144 decoded to
  a legal opcode and were silent**. **PRICE +49,216 P95** (bank above). Never loosen the bound.
- **AObj cliff = CAPACITY, not a leak — FIXED.** Four zero-growth stops against reuse firing 16-19/stop kills the leak theory; the shipping 1-minute arm already stood at **889/1,024**, and a LEDGER cannot be evicted (the repack has no spare bit) so capacity is the lever: **`NDS_AOBJ_EVENT32_NORMALIZED_MAX` 1024 → 2048**, +8,192 B bss, headroom **167,936**. **Corpus is 1,598 of 2,048 post-fix — 450 spare, 1.28x, NOT the 1,029/2x the source comment claimed (corrected 2026-08-13).**
**START PAUSES THE MATCH** and the old whole-window freeze hash could not see it — the watch hashes the TOP band and `-PressStartOnResults` presses only on a detected Results screen.
**WHISPY LOW-FPS FACE FIXED / OWNER-CONFIRMED 2026-08-14:** Slice 44 passed `allow_stale=TRUE` to dynamic stage bindings, freezing Whispy's world matrix for up to 8 presented frames despite 30 Hz DObj motion. Dynamic bindings now validate every frame while unchanged source keys still reuse. Control `m11` stayed **3872** through live scale 0.948/0.605/0.104/0.104/0.606/0.948/1.0; candidate tracks **3872/2464/416/416/2480/3872/4096**. Evidence: `artifacts/verification/2026-08-13_whispy-slice44-fix/WHISPY_FIX_EVIDENCE.md`.
## Measurement rules that change your FIRST action — board owns the rest

- **The sampler is bit-deterministic — never repeat a run.** Same ROM twice gives byte-identical
  buckets, so ANY cross-build delta is placement. `-Samples 1600 -RingDump`. A duplicate frame
  LABEL at a ring seam is warned; IDENTICAL payload is a stale read and always fatal, as is one
  away from a seam. **THE PLACEMENT FLOOR IS ~17,000 P95, NOT ±5,376** — 2026-08-13, three near-identical arms (`…_c-collision-stack`): the falsifier, carrying the candidate's bss and 16 fewer bytes of text, beat the control by **16,832**. Under ~17,000 a two-build comparison measures the linker; only a flag falsifier means anything. **Compare ELF SECTIONS, not the `.nds`** — NitroFS packs directory entries nondeterministically, so identical source can give 14 differing ROM bytes with `.text`/`.data`/`.rodata` identical.
- **Judge on `WORK-H`**; buckets locate, they never decide (floor ≥8,544). **`ALL` is
  VBlank-quantized** — it hid a +52,928 once. **1.85 cycles of `FTR` mean per byte of added ARM
  text.** **A bucket only sees its OWN thread** (slice 48).
- **A census row in tk/fr sizes a P50 lever, NOT a P95 one.** Slices 36/37 had equal mean self
  cost and P95 wins **2.45x apart**. **Presence is the tell** — and a lane that is BIMODAL at the
  percentile returns less than its mean (`RESIDUE.md` §1).
- **A route A/B is valid only for a change that cannot alter gameplay state
  (slice 41) AND only if the poke lands before the value is READ.** `-SetGlobals`
  fires at the first frame-complete marker; record what was actually applied or
  the control is the candidate relabelled (slice 48 got 1,102,208 on both arms).
- **Per-line/per-PC attribution BEFORE designing — no build, and it routinely names a different
  lever than the source reads like.** Slice 44's guard looked like a compare but four cold `ldr`s
  were 39%; 85.5% of `ndsAObjEvent32NormalizeScript` is two pointer scans; the whole of RESIDUE §4
  row 0 is inlined helpers a symbol census cannot see.
- **An arm that cannot produce the event reads 0 either way** — check the control differs first.
  **And a zero one level DOWNSTREAM of a rejected request reads exactly like a dead lane**: row
  2's four zeroes were correct readings of a request killed by a NULL script pointer upstream,
  and each was read as "nothing asks for this".

## Restart surface — parked items live on the board's **Parked** list

**PUSH IS UNBLOCKED — `a4100b7`, 2026-08-13**, the owner's "scrub the 16" (`OWNER_DECISIONS.md` §9):
16 Cargo blobs under `decomp/BattleShip-main/decomp/tools/` baked the build machine's user directory.
Untracked + gitignored, byte-identical on disk; they **stay in pushed HISTORY — the owner accepted.**
Scan is `git grep -l -i -e <owner-given-name> HEAD`, now **17 → 1**: the survivor, sm64's IDO
`usr/lib/copt`, is a FALSE POSITIVE. `AGENTS.md` still miscalls `/decomp/` gitignored — 26,260 tracked.

`AGENTS.md` owns the start-of-cycle commands; `docs/P1_EXECUTION_BOARD.md` is the only dynamic
queue; `docs/BUGS.md` carries the owner's verdicts — preserve their wording. A clean checkout builds
through `build.ps1`, not bare `make`: four of six `.inc` are gitignored and **`build.ps1`'s
generator is not run by `make`**. `make p1-tick` builds the measuring ROM, `make p1` the published
pair. Never pass `-j`, never override `MAKEFLAGS`, one build at a time, never build a published
target name for lab work. Preserve mode 163, renderer mode 9, mip 0, static textures, source
countdown, Dream Land water frame 0, Task 16 `1/1/1`. Never edit `decomp/`. Run
`New-Smash64DSSnapshot.ps1` last.
