# P2-2p8 Phase 1 slice 1: lean fighter path (Samus LOW, program 0)

Brief: `phase1-impl-brief.md` (coordinator). Spec: Phase 1 spec section 6 (6.1-6.6), 2.6 (tint tile-word patch) and 2.8 (admission).
Target `smash64ds-p2-fourcpu-tickhud-hwtri`, build dir `build-p2-fourcpu-tickhud`, tree HEAD 5c7f9db9f93 plus uncommitted changes.
Final ROM sha256 `BA98D3A23CFD40AC...` (`final-*.json`). Roster: DK slot 0, **Samus slot 1**, Link slot 2, Kirby slot 3.
All runs: sampler on runner slot 9 / GDB 3423, 1,972 samples from frame 2, `-RingDump`.

## Verdict

All four DONE criteria are met on the final ROM:

| criterion | result |
|---|---|
| Route 2 oracle mismatch counters all 0, whole match, Samus engaged | **0 in all 7 classes.** 1,217 TryReplay hits compared, 2,547,181 words. Key moved 0. 266 old-path re-records under an armed shadow: 0 words differ outside patch sites. 1,483 shadow patches out of 1,685 Samus attempts. |
| Route 0 vs route 1 replay digests identical | **IDENTICAL over 1,972 frames** (`digest-final-route0-vs-final-route1.json`). Routes 2 and 3 and the admission arm are identical too. Route 0 is also identical to the Phase 0 `nocam` ROM (`digest-p0nocam-vs-final-route0.json`). |
| Native failures do not increase | 573 failures / 254 direct rejects in routes 0, 1, 2 and 3, the same as Phase 0. First failure unchanged: Link status 225, reason 2. |
| Route 0 vs route 1 band means | See the table below. Route 1 FTR P95 is -284,012 (-11.0%); P50 is flat. |

Route 1 drew Samus through the lean path **1,483 times** (`gNdsFtrLean.draws`). Its declines by reason were:
- adoption pending: 52
- tuple: 135
- animlock: 6
- tint set: 3
- residency: 2
- rebind: 4

Every other reason stayed at 0: camera, material, preamble, fence, kernel, tint, topology and skeleton. `gNdsFighterMarioFoxDLAllDrawCount` is 6,862 in both routes, so the bookkeeping agrees.

## Route 0 vs route 1 (final ROM, one run each; ticks = cpuGetTiming units)

| | route 0 | route 1 | delta |
|---|---:|---:|---:|
| FTR P50 | 364,288 | 362,656 | -1,632 (-0.4%) |
| FTR P95 | 2,580,726 | 2,296,714 | **-284,012 (-11.0%)** |
| FTR P99 | 2,780,604 | 2,780,300 | -304 |
| FTR mean | 715,899 | 681,744 | -34,155 (-4.8%) |
| FTR P95/P50 spread | 7.08 | 6.33 | |
| WORK-H P50 | 1,695,904 | 1,698,144 | +2,240 (+0.1%) |
| WORK-H P95 | 4,179,261 | 3,902,035 | **-277,226 (-6.6%)** |
| WORK-H P99 | 4,724,643 | 4,520,642 | -204,001 (-4.3%) |
| WORK-H mean | 2,044,418 | 2,009,601 | -34,817 (-1.7%) |
| WORK-H P95/P50 spread | 2.464 | 2.298 | |

Band means (bands are WORK-H ranks within each run):

| band | FTR r0 | FTR r1 | WORK-H r0 | WORK-H r1 |
|---|---:|---:|---:|---:|
| P40-60 | 363,657 | 363,539 | 1,698,472 | 1,700,971 |
| P90-95 | 2,571,793 | 2,273,506 | 4,059,368 | 3,781,901 |
| P95-99 | 2,581,138 | 2,367,074 | 4,340,775 | 4,108,802 |
| P99+ | 4,359,904 | 4,292,557 | 6,121,715 | 6,070,131 |

Both runs have the same digests, so they played the same frames. That makes per-frame deltas valid (`regions-final.txt`). The paired mean is -34,155 FTR/frame and the median is -3,424. The gain sits in one episode:

| frames | FTR r0 | FTR r1 | paired | MTEX r0 |
|---|---:|---:|---:|---:|
| 2-195 (entry) | 380,164 | 371,589 | -8,575 | 13,532 |
| 196-797 | 459,508 | 457,512 | -1,996 | 584 |
| **798-1043 (P0 episode)** | **2,608,838** | **2,347,908** | **-260,930** | **25,536** |
| 1044-1974 | 451,185 | 450,864 | -321 | 1,949 |

**The P0 episode is a fence storm, not a tint re-record storm.** In this ROM, r49's prim-change re-record never fires: `gNdsFighterPacketTintRerecords` = 0 and `tint_rerecords[0..3]` = 0 in every arm. Inside the episode, fighter textures upload every frame (MTEX 25.5K/frame, against about 0.6-2K elsewhere). Each upload moves the global fence (key[5]), so every needs_fence packet re-records every frame.

Samus's packet needs the fence **only** because of its 2 tint-tile binds (`adopt_fence_other` 0). The lean copy validates each tile per draw instead (below), so it keeps drawing:
- Records drop from 1,388 to 1,141 (-247, about one per episode frame).
- Route 2 counts 614 fence re-keys that the lean copy correctly ignored.

Route 0 vs Phase 0 `nocam` (older ROM, same digest):

| | Phase 0 nocam | route 0 |
|---|---:|---:|
| FTR P50 | 363,200 | 364,288 |
| FTR P95 | 2,604,163 | 2,580,726 |
| FTR mean | 718,205 | 715,899 |

Per-range means also agree within 1%. The route-0 cost of the slice is not measurable at this resolution.

Lean draw cost in route 1, per draw:

| part | ticks |
|---|---:|
| guard | 9,946 |
| kernel (24.1 joints/draw, 32.3% of joints on the slow path) | 26,329 |
| patch | 8,910 |
| submit | 9,603 |
| **total** | **54,788** |

Outside the episode, lean saves only about 2-9K ticks/frame against a replay hit. The next lever is the kernel's slow joints: 11,542 of 35,772 go through the adapter's own float builder.

## Kernel placement (measured both ways; the final ROM uses B)

| placement | route 0 FTR mean / P95 | route 1 FTR mean / P95 | kernel ticks |
|---|---|---|---|
| A: kernel in ITCM, `ndsRendererNativePrepareProductionRun` (2,600 B) evicted to main RAM | 736,270 / 2,679,875 | 696,010 / 2,362,413 | 35,050,112 |
| **B: kernel in main RAM (ARM), PrepareProductionRun left in ITCM** | **715,899 / 2,580,726** | **681,744 / 2,296,714** | 39,046,464 |

The eviction cost route 0 +20,371 mean and +99,149 at P95, because PrepareProductionRun runs on every record, and the episode is all records. Moving the kernel to main RAM costs about +2,700 ticks per lean draw. The brief allowed the eviction but did not require it, so B stays and **ITCM is unchanged by the slice**. .itcm is 0x7f88.
- Placement A runs: `route0`, `route1`, `route2`, `route3`, `route0-admit1`.
- Placement B runs: `route0-B`, `route1-B` and `final-*`.

## Route 2 / route 3 oracle (final)

**Route 2 (old path draws with the Q43.20 source compose forced for Samus; H4):**
- `oracle_mismatch[0..6]` = 0 (projection, basis, row 3, shade, light, other, tint tile).
- `oracle_key_moved[0..7]` = 0.
- `oracle_record_under_hit` = 266 same / 0 differs.
- `oracle_source_miss` = 0 and `oracle_unconsumed` = 0.

The kernel is bit-exact with `ndsRendererAdapterComposeOwnerWorldsSource` across the match: 35,772 joints, 0 kernel failures.

**Route 3 (old path on the shipped flat Q20.12 compose):** the lean words differ in these places:
- basis: 52,166 words, max 3 LSB
- row 3: 9,785 words, max 3 LSB
- projection: none

This is the shipped compose's rounding against the exact one.

**Record-under-hit classification (6.4).** The spec gate is "0 words outside patch sites". Inside patch sites, route 2 still shows `oracle_record_diff[3]` = 2,208 shade words. That is a pre-existing live-vs-replay difference; see "Finding: flash shade" below. It is not a lean hole: the lean words equal the replay's words on every one of the 1,217 hits.

## 2.6 tint tile-word patch

Samus LOW's canonical packet has **2 tinted sites and 2 tint-tile binds**. Before this change, adoption refused it 1,627 times with 0 shadow runs (`route2-a`).

What slice 1 does now:
1. **Recorder.** Each tile bind is recorded with its TEX_FORMAT/PAL_FORMAT word indices, its root, prim-from-root and its colour. The words are unchanged.
2. **Fence causes.** The recorder counts the needs_fence causes that are not tint binds (`fence_other`).
3. **Tile words.** Each tile's TEXIMAGE/PLTT words are captured once at creation. These are reads of libnds state only; there is no GX write.
4. **Adoption.** Adoption takes a tinted packet whose binds are complete. The copy's tinted sites re-derive: they were recorded as raw light with no material, so the re-derive does not depend on prim.
5. **Per draw.** `ndsFtrLeanPatchTintTiles` looks up the live colour's resident tile and patches its two words. The colour is the root's prim unless the display list overrode it.
6. **Declines.** It declines on a white prim (the live path would draw untinted), on a missing tile, or when the palette word's presence differs.
7. **Guard.** It skips the global fence only when `fence_other == 0`.

In this match the prim never moved: `tint_patch_binds` 2,966, with moved, miss, white and shape all 0. So the prim-change path is built but was not exercised by this roster's match. Route 2 would compare it against the forced re-record (record-under-hit) the first time it fires.

The tint-set generation guard is kept (3 declines). The set moves 11 times per match, and it also decides fold versus tile for untinted sites.

## 2.8 admission (gNdsFtrLeanAdmit) and the census (printed first)

**Census at GO (frame 196), printed in every arm:**
- **Texture VRAM** is banks A+B = 262,144 B (`src/nds/nds_platform.c:442-443`).
- **Texture cache:** 98 live entries of 124, of which 44 are static stage entries. 26 slots are free. Live texels total **107,008 B**.
- **Four kinds' recorded union** (cache slots their packets referenced by GO):

  | kind | textures | bytes |
  |---|---:|---:|
  | DK | 25 | 27,008 |
  | Samus | 16 | 9,024 |
  | Link | 33 | 21,184 |
  | Kirby | 2 | 768 |
  | **total** | **76** | **57,984** |

- **After GO:** 408 fighter uploads, 224,512 B. The reachable set does not stay resident today.

**Admission arm (`final-route0-admit1`, route 0).** This is a pin-on-record stand-in: the first record of each fighter texture pins its cache entry.
- Pinned: 72 textures, 49,152 B.
- Fighter uploads after GO: 408 to 18. Fighter texture rejects: 254 to 99.
- Episode FTR: 2,608,838 to 1,561,893 (-40%). MTEX in the episode: 25,536 to 57.
- FTR P95: 2,580,726 to 1,563,670 (-39%). WORK-H P95: 4,179,261 to 3,331,069 (-20%).
- **But** native failures rose 573 to **928**, and records rose 1,388 to 2,147.
- Frames 1044-1974 cost FTR 451,185 to **680,114 (+51%)**. FTR P50 rose 364,288 to 377,344.

Pinning 49 KB without a budget starves the later working set. **Not shippable; the default stays 0.**

The real 2.8 admission is blocked in slice 1 for two reasons:
1. It needs the creation-time walk of every reachable template, including Entry/Appear programs. Slice 1 adopts recorded packets, which name a texture only after its first use, so no packet-driven scheme can admit Link's entry texture before the frame-156 burst.
2. The census says the fighter union, at least 58 KB at GO, cannot be pinned on top of today's residents without a VRAM budget. The cache itself held only 99-107 KB of texels when VRAM was already full.

That is why `src/import/battleship_ftmanager.c` is **not edited**: the stand-in needs no creation seam.

## Link AppearL entry failure: root cause

This comes from the reject witness (`reject_first[]`), the first fighter-owned reject since boot. It is identical in every non-admit arm.

At profile frame **156** (before GO at 196), the Link owner (7) needs a texture:
- Reason: **TEXIMAGE** (reject mask 0x1000). `glTexImage2D` could not allocate VRAM.
- Format: N64 CI 4-bit (format 2, size 0).

The cache at that instant:
- 87 live entries and **37 free slots**, 44 of them pinned static.
- 43 entries touched this frame, so **0 were evictable**.
- 99,072 B of live cache texels.

So ResolveOrBind cannot upload and cannot evict. `use_texture == FALSE` at `nds_renderer_native_common.c:8759-8761` (8758 in the Phase 0 tree) gives native failure status 225 (nFTLinkStatusAppearL), reason 2. The 254 direct rejects are the same event repeating (254 packet faults).

The binding resource is **texture VRAM during the entry burst, not cache slots**. Only about 99 KB of the 256 KB is cache texels. The rest is non-cache residents plus first-fit fragmentation; that part is inferred from the census, not measured per allocation. One known non-cache class is live exactly then: fighter entry props are "direct GL residents rather than cache entries", released only after the intro (`nds_renderer_textures_effects.c:4025-4031`, `gNdsEntryEffectStartupTextureRelease*`). The fix is admission with a VRAM budget before GO (2.8), and it has to budget VRAM, not slots.

## Finding: flash shade differs between live record and replay (pre-existing, not the slice's)

Route 2's self-check (`record_shade_*`, oracle routes only) re-derives every freshly recorded DIF_AMB word from its own recorded inputs at the packet's `tint_modulate`. **9,126 of 24,621 recorded shade sites (377 packets) are not reproduced.**

Witness: tint_modulate `0xffffff24` (a colanim white flash, alpha 36).
- Recorded ambient: 0x192c (12,9,6), which is light2 0x664c33 unmodulated.
- Re-derived: 0x298f.

**Mechanism.** The native production execute starts its traversal state with `config == NULL` (`src/nds/nds_renderer_native_fighter_production.c:181`, via `nds_renderer_dl_core.c:1370`). So the live draw's `state->color_modulate` is 0 and a live record never flash-modulates DIF_AMB. `ndsFighterPacketApplyTint` re-derives with `inputs[0].config->color_modulate`, the fighter's colanim flash. Today, then, a replayed frame shows the flash on lit shade and a freshly recorded frame does not.

The lean path equals the replay (0 mismatches), so route 1 shows it on every lean frame. The 2,208 record-under-hit shade differences are this effect: the witness at frame 828 shows tint_modulate 0xffffff34, live ambient 9, replay/lean ambient 13. Whichever behaviour is intended, the two paths should agree. **Flagged for the owner; the slice does not touch it.**

## Changes (file:line, final tree)

| file | lines | what |
|---|---|---|
| `include/nds/renderer_fighter_lean.h` (new, 256 lines) | whole | `NDS_FTR_LEAN_LIVE`, routes 0-3, decline/adopt enums, `NDSFtrLeanCounters` (6.5 + Phase 0 leftovers + oracle diagnostics), kernel and packet-side API, `ndsFtrLeanTintTileWords` |
| `src/nds/nds_ftr_lean_kernel.c` (new, 405 lines, CFILES) | whole | ARM-mode joint kernel: Q43.20 compose from 16.16 locals, fast local, slow-path callback, To20p12 + shuffle fold |
| `src/port/renderer_fighter_lean.c` (new, 554 lines, `#include`d at the end of RAF, TU B) | 17-26 | route/admit words (`.dtcm.bss`, used, u32) and counters |
| | 216 | `ndsFtrLeanRun`: eligibility, decline ladder, guard, kernel, patch, shadow or submit, RAF-tail bookkeeping |
| | 437 | `ndsFtrLeanAfterOldPath` (adoption) |
| | 519 | `ndsFtrLeanFrameEnd` (per-match reset, admission census, GO latch, GE-busy sample, publish) |
| `src/port/renderer_adapter_fighter.c` | 5139-5147 | include and prototypes |
| | 5211-5217 | head sub-phase timing |
| | 5287-5306 | **H1**: route != 0 tries lean; otherwise `DrawForSlot` followed by adoption. Route 0 is the plain call. |
| | 5381 | `ndsFtrLeanFrameEnd()` |
| | 5431 | `#include "renderer_fighter_lean.c"` |
| `src/port/renderer_adapter_matrix.c` | 11 | include |
| | 541-542 | **H6** rebind note |
| | 7473-7500 | **H4** route 2 forces the source compose for Samus and sets `gNdsFtrLeanOracleSourceOk` |
| `src/nds/nds_renderer_native_common.c` | 6459-6460, 7091 | epoch tint colour |
| | 8872-8875 | pending tint bind before the prepare hook |
| | 9697-9727 | fence-cause split (`needs_fence` unchanged) |
| | 10066-10072 | forward declarations |
| | 10168 | **H2** FinishRecord to OnRecorded |
| | 10352 | per-slot tint re-record counter |
| | 10417 | **H3** replay hit to oracle |
| | 10522-10527 | **H5** recorder capped to the lower half when route != 0 |
| | 10546-10548, 10569-10570 | record/arm resets |
| | 10601, 10619 | invalidation drops the lean copy |
| | 10638-11560 | packet side: region-half copy, Adopt, Guard, PatchTintTiles, Patch, Submit, oracle class/key/compare, census, OnRecorded (+ admission pin + shade self-check), reject/upload witnesses, publish |
| | 11560-11674 | stubs |
| `src/nds/nds_renderer_preamble.c` | 21 | include |
| | 3409-3420 | `NDSFighterPacketTintBind` |
| | 3474-3481 | packet metadata (`tint_bind_count`/`overflow`, `fence_other`, `tint_binds[16]`) |
| | 3500-3502 | recorder pending tint |
| | 3545-3553 | packet DMA-wait counters |
| | 3686-3736 | RecordBoundTexture keeps its indices and records tint binds; same words |
| | 3802 | pending tint cleared |
| `src/nds/nds_renderer_textures_effects.c` | 3779-3783 | tile TEXIMAGE/PLTT fields |
| | 3962-3975 | capture at tile creation (reads only) |
| | 3982-4023 | `ndsFtrLeanTintTileWords` + stub |
| | 9698-9700 | reject witness hook |
| | 12521-12522 | upload counter hook |
| `Makefile` | 4301-4303 | `CFILES += nds_ftr_lean_kernel.c` |

Not edited: `battleship_ftmanager.c` (see 2.8), `decomp/`, generated outputs, and `NDSTickHudBucket` (no new HUD rows; all counters are plain globals read with `-ExtraGlobals`).

## Counters (`gNdsFtrLean.*`, published with DC_FlushRange once per frame; all used and nm-checked)

- **Draw/decline:**
  - `draws`, `shadow_runs`, `attempts`, `adopts`
  - `decline[16]`:
    - 0 kind
    - 1 adoption pending
    - 2 tuple
    - 3 animlock
    - 4 camera
    - 5 material
    - 6 preamble
    - 7 tint set
    - 8 residency
    - 9 fence
    - 10 kernel
    - 11 tint
    - 12 topology
    - 13 skeleton
    - 14 rebind
  - `adopt_refuse[8]`: 1 invalid, 2 shape, 3 texgen, 4 tinted, 5 capacity, 6 topology, 7 projection index
  - `adopt_*` shape of the last adopted packet, including `adopt_tint_binds` and `adopt_fence_other`
- **Kernel/sub-phases:** `kernel_joints`, `kernel_slow_joints`, `kernel_fail`, `head/guard/kernel/patch/submit_ticks`, `dma_wait_ticks/spins`
- **Oracle:**
  - `oracle_runs`, `oracle_words`
  - `oracle_mismatch[7]`, `oracle_max_lsb[3]` (route 3), `oracle_key_moved[8]`
  - `oracle_record_under_hit[2]`, `oracle_record_diff[8]` (0-4 patch classes, 5 outside patch sites, 6 tint tile, 7 structure)
  - `oracle_fence_rekey`, `oracle_unconsumed`, `oracle_source_miss`
  - `oracle_shade_witness[4][12]`, `record_shade_checked/inconsistent/inconsistent_packets/witness[2][9]`
- **Tint:** `tint_patch_binds/moved/miss/white/shape`, `tint_rerecords[4]` (per slot)
- **Phase 0 leftovers:** `ge_busy_samples/hits`, `packet_dma_waits/wait_ticks`, `fighter_uploads`, `fighter_uploads_after_go`, `fighter_upload_bytes_after_go`, `go_frame`
- **Admission/census:** `admit_runs`, `admit_pinned`, `admit_pinned_bytes`, `union_textures[4]`, `union_bytes[4]`, `census_live/free/pinned/static/live_bytes/fighter_bytes`, `reject_count`, `reject_mask`, `reject_first[12]`
- **Words:** `gNdsFtrLeanRoute` and `gNdsFtrLeanAdmit` are u32 in `.dtcm` (0x02ff2224 and 0x02ff2220), plus `gNdsFtrLeanOracleSourceOk`.

`counters-final.txt` has every value for the five final arms.

## Deviations, declines, what did not work

- **Route words live in DTCM, not `.data`.** DTCM is uncached, so a gdb poke is never hidden by a stale D-cache line. The sampler read back every poke (`stuck: True`). Their default is 0.
- **The kernel is ARM mode but not ITCM-resident.** The brief asked for an "ARM-mode ITCM joint kernel". ITCM had 128 B free, so ITCM residency meant evicting PrepareProductionRun, and that measured +20K mean / +99K P95 FTR in route 0. The kernel lost only about 2.7K ticks per draw in main RAM (see "Kernel placement"). To restore ITCM residency (placement A), change two attributes: add `section(".itcm")` back to `NDS_FTR_LEAN_KERNEL_CODE` (`nds_ftr_lean_kernel.c:36-37`), and switch PrepareProductionRun (`nds_renderer_native_common.c:9036-9037`) to `NDS_RENDERER_NATIVE_FIGHTER_MAIN_CODE`.
- **Admission is a stand-in, and it regresses.** The template-walk 2.8 is blocked; see 2.8.
- **Two adoption refusals were hit and fixed during bring-up:**
  - tinted packets: 2.6, above
  - tint-only fence: guard plus oracle
- **Oracle key[5].** Early route 2 runs counted 614 key[5] moves as KeyMoved. With the tint-only fence they are not a hole, so they are now counted apart as `oracle_fence_rekey`. That change let the word compare run on all 1,217 hits (previously 870).
- **RAM.** The slice adds 10,388 B of lean BSS plus about 1.2 KB of packet/tint metadata. `gNdsTaskmanGeneralHeapFreeMin` = **87,060** on the final ROM (`final-route1-heap`), against 111,636 on the Phase 0 nocam ROM. That is -24,576 B; the slice's own static RAM is about 11.6 KB of it, and the rest comes from tree changes since be5df97fa71 or arena granularity (not separated). It is still 61 KB above the 25,600 GObj-cap latch.
  - The shrink candidates are inputs/worlds sized 32 where 14 roots are needed.
- **Not exercised by this match:**
  - Samus prim change (tile repatch)
  - the kernel range decline
  - camera, material and preamble declines

## Runs in this folder

**Final ROM:**
- `final-route0`
- `final-route1`
- `final-route2`
- `final-route3`
- `final-route0-admit1`
- `final-route1-heap`

**Bring-up and experiments:**
- `route2-a`: tinted refusal
- `route2-b`: tile patch
- `route2-c`: tint-only fence rekey split
- `route2-d`: shade witness
- `route2-e`: shade self-check
- `route0`, `route1`, `route2`, `route3`, `route0-admit1`: placement A
- `route0-B`, `route1-B`: placement B, same code as final minus comments

**Other files:**
- digests: `digest-*.json`
- bands: `bands-final.txt`
- per-range means: `regions-final.txt`
- builds: `build*.log`

## Reproduce

```
pwsh -NoProfile -File tools/build-slice1.ps1 -LogName build.log        # waits for and holds builds/.p2p8-build.lock
pwsh -NoProfile -File tools/run-slice1.ps1 -Arm final-route1 -Route 1   # sampler, slot 9 / GDB 3423; -Admit 1 for the admission arm
python scripts/compare-replay-digest.py final-route0-rows.csv final-route1-rows.csv
python tools/bands.py final-route0-rows.csv final-route1-rows.csv
python tools/summ.py final-route0 final-route1
```
