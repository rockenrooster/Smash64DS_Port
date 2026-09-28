# Lane 3 - what is actually dead in the A1 renderer retirement (P2-2p8 RAM supply)

Read-only investigation, 2026-09-28. No build, no emulator, no source edit. Everything here comes from the
two saved ELFs, their linker maps and relocation tables, the source at HEAD `76f5a57631a` (the renderer files
cited are clean in the working tree), and the run JSONs already under `artifacts/performance/`.

| ELF | path | sha256 (prefix) | linked |
|---|---|---|---|
| shipping-like (FP) | `builds/build-fp-argmax/smash64ds-p2-shell-freeplay-hwtri.elf` | `2299a1e1754b9e59` | 09-28 12:19 |
| four-CPU gate (GATE) | `builds/build-yos-gatechk/smash64ds-p2-fourcpu-tickhud-hwtri.elf` | `2594fae9fb874ba5` | 09-28 14:55 (**relinked by someone else while I worked**; I re-ran on a copy, unit sizes were byte-identical to the 13:27 link) |

FP is the published shell configuration plus `NDS_P2_MENU_WALK=1` and `NDS_P2_SHELL_ARGMAX_ROSTER=1`
(`nds_build_config.h:221,168`), so it is not byte-identical to the published `smash64ds`. GATE is a tick-HUD lab ROM
(`NDS_TICK_HUD 1`, 1P off, menu shell off), so it also carries lab counters and the lean oracle routes.

"Heap bytes" below means bytes in `.main`, `.main.rw`, `.main.bss`, `.text.hot`, `.text.hot.draw`,
`.text.frontend_resident` (FP: 2,604,088 B; GATE: 2,436,928 B). Those sections sit before the overlay and the heap,
so each byte freed grows the battle arena one for one (`2026-09-28_p2-2p8-stage-heap-cliff/README.md`: the arena "tracks
`.bss` byte for byte"). ITCM, DTCM and `.ovl.frontend*` are reported separately because deleting from them does not
grow the battle heap.

## 0. Answer in numbers

| # | unit (all symbols only the named entry keeps alive) | FP heap B | GATE heap B | verdict | needs owner input? | risk |
|---|---|---:|---:|---|---|---|
| R1 | **P0** fighter modes that cannot run in shipping: hierarchy mode 7, per-root hardware executor, CPU triangle rasteriser, resident raw-path tables | **43,437** | 41,414 | **DEAD** | no | low |
| R2 | **DIAG** `ndsResetStartupDiagnostics` + word tables + reset helpers + counters only it links (net of script-read counters and non-zero initialisers) | **38,356** (gross 42,056) | 38,200 (gross 42,176) | dead code by config, not a renderer item | no (verifier retarget) | low-medium |
| R3 | **P1** old fighter production / packet record+replay / plan / validate path and its state | **81,920** | 85,252 | **FALLBACK-ONLY** (fired in 59 pre-fix runs today, always ended in a native failure too) | **yes: 1P** | medium-high |
| R4 | 1P compiled out of the shipping flag block (unblocks R3, frees its own tables) | est. 20-56 KB (not measured, not in the totals below) | n/a | REACHABLE today | **yes** | product decision |
| | **R1 + R2 + R3** | **163,713** (159.9 KiB) | 164,866 | | | |
| | of which needs no owner decision (R1 + R2) | 81,793 | 79,614 | | | |

That is 6.3% of the FP heap-relevant static image. The A7 ledger row "A1 retirement ~260-360K" is not available
today: the rest of what it lists (texture scratch 75,280 B, stage native machinery 91,018 B, effect packets
19,758 B, the 52 native MISC owners of 82,148 B) is REACHABLE in shipping and retires only when its replacement lands
(section 5).

**1P is not gated off in shipping.** `NDS_P2_1P_GAME` is 1 in the FP config (`builds/build-fp-argmax/nds_build_config.h:137`)
and the free-play/`smash64ds` block forces it (`Makefile:3397-3402`, `override NDS_P2_1P_GAME := 1`, comment: "the published
owner ROM is the all-content build"). Only GATE has it 0. "VS first, 1P deferred" (owner 09-27) is a work priority,
not a configuration. Several verdicts below turn on this.

## 1. How a fighter is drawn at HEAD, and every way into the old path

```
gcDrawAll -> ftDisplayMainProcDisplay -> ndsFighterDisplayContractSubmit   renderer_adapter_fighter.c:5353
 1 not a tracked battle fighter (1P intro actor)        -> ndsFighterIntroTransientSubmit :5263 -> DrawForSlot (OLD)   1P only
 2 ndsFighterDisplayContractCapture                                                                                    live
 3 lean route (route 1 is the boot default, NDS_FTR_LEAN_ROUTE_BOOT) -> ndsFtrLeanRun   renderer_fighter_lean.c:1563
      eligible only for owner slots 0-11 (the 12 VS kinds), FastRunMode 8|9, not an intro transient (:797-816)
      TRUE  -> list patched + one DMA (ndsFtrLeanPacketSubmit)                                                        live
      FALSE -> (non-lean kind | one of 20 decline reasons) -> ndsFighterMarioFoxDLAllDrawForSlot  :5534-5537         OLD
 4 OLD = DrawForSlot :3413-5159 (8,800 B):
      plan / validate / material prep / production inputs (:3859-4400)
      -> ndsRendererExecuteNativeFighterOwnerProduction (packet replay hit | record | direct)  :4404-4436          [P1]
      -> native_owner_started per-root executor :4881-4910                                                          [P0, unreachable]
      -> hierarchy mode :3789-3806, :4169-4180, :4418-4423                                                          [P0, unreachable]
      -> else ndsFighterRejectNativeRender (fail closed, records a native failure) :4911-4915
```

`DrawForSlot` has exactly one caller in the ELF (`ndsFighterDisplayContractSubmit`, by the reloc graph); the
`ndsFighterMarioFoxRecordDLAllDrawFromDisplayCallback` proof caller (:5629) is not linked. There is no generic
display-list interpreter left in the fighter fallback: the tail is a fail-closed reject, so the "generic renderer"
is already gone from the image.

### 1.1 Why P0 is dead (no lean coverage needed)

* `gNdsRendererFastRunMode` is a `.dtcm` word initialised to `NDS_RENDERER_FAST_RUN_DEFAULT` = 9
  (`nds_renderer_preamble.c:2611-2622`, config line 249, Makefile overrides at 2593/2837/3100/3170/3254/3358/3470/...).
  `rg` finds no assignment anywhere in `src/` or `include/`; readers: `renderer_adapter_fighter.c:3761-3791`,
  `renderer_fighter_lean.c:808-811`, `nds_renderer_dispatch_profile.c:173-208`, `reloc_backend_movement.c:15913,16000`,
  `nds_renderer_native_owners.c:4225,4243`, `renderer_adapter_stage.c:4530`. Hierarchy mode needs `== 7` (:3789-3791).
* The per-root executor needs `native_owner_started`, set only when `native_owner_enabled` and
  `native_owner_production_attempted == FALSE` (:4513-4520). Production is attempted whenever the native owner is
  enabled, `production_mode` (mode 8/9), `detailed_output == FALSE` (constant, `NDS_R2_FIGHTER_NO_ORACLE 1`) and
  `no_oracle != FALSE` (:4363-4366, :4417). Both wrappers that call `DrawForSlot` set no-oracle TRUE first (:5306, :5506).
  A rejected production sets `native_owner_enabled = FALSE` (:4446-4458), so no path reaches `Begin`/`ExecuteRoot`.
* Two lab targets still use mode 8 (`Makefile:3755-3780`: `smash64ds-battle-playable-coarse-hwtri`,
  `...-bgm-off-hwtri`); every other non-shipping target defaults to mode 0 and profile level 2 (`Makefile:138,2580`).

### 1.2 Why P1 is fallback-only, and every trigger

| trigger | where | reachable in shipping? | has it fired in a recorded run? |
|---|---|---|---|
| owner slot >= 12 (Metal Mario, 11 polygon-team kinds, Master Hand) | `ndsFtrLeanEligible` :797-816; header comment `renderer_fighter_lean.h:256-259` "stay on the old path" | **yes, 1P** (VS CSS cannot select them; `NDS_P2_MMARIO 0`, `NDS_P2_N* 1`, `NDS_NATIVE_OWNER_IMAGE_BOSS 1`) | not measurable: no 1P run carries lean counters |
| 1P intro transient actors (`sNdsIntroTransientActive`) | `renderer_adapter_fighter.c:5263-5324`; gated on `nSCKind1PIntro` | **yes, 1P**; live-intro lean coverage is WITHDRAWN (`2026-09-24_p2-2p8-phase1-slice8/README.md`) | no counters |
| decline `Texgen` (Link) | `nds_renderer_native_common.c:13925-13956` | yes (VS) | **yes: 180 declines in 59 runs, 09-28 02:33-16:46Z**, gkind 1/5/7 (Sector, Yoster, Yamabuki); every one ended in `DirectReject` + `NativeFailure` + `PacketRecords/Faults` (= the old path failed too). **0 declines in the 13 Link runs after 16:50Z incl. HEAD** (`la_*`, `lm_gate`, `et_gate`) |
| decline `Validate` (Ness) | `nNDSFtrLeanDeclineValidate` | yes (VS) | 12 declines in 2 runs, 09-24 pass 1 (`phase1-slice7/iterations/pass1/s7-hi-puns-*.json`), 0 since |
| the other 18 decline reasons (Camera, Capacity, Kernel, Stale, Tint, Skeleton, ...) | `renderer_fighter_lean.c:1597-1902` | yes | never |
| lean route off / FastRunMode not 8|9 | :5516-5526, :808-811 | no (route 1 and mode 9 are boot values; only a gdb poke changes them) | poked only in lab A/B arms |

Recorded lean coverage (807 tick-HUD four-CPU runs since 09-24, `lane3_lean_coverage.py`; runs are A/B arms of a
few rosters, not independent): every one of the 12 VS kinds was recorded on every one of the 9 stages;
attempts / draws per kind: Mario 24,270/24,270, Fox 58,653/58,653, Luigi 27,940/27,940, Donkey 1,111,829/1,111,829,
Captain 76,779/76,779, Samus 1,137,156/1,137,156, **Link 1,156,967/1,156,787**, Pikachu 247,380/247,380, Yoshi
87,025/87,025, **Ness 128,154/128,142**, Purin 94,830/94,830, Kirby 1,188,621/1,188,621. HEAD gate run `lm_gate.json`:
draws = attempts = 6,862, `gNdsFighterPacketHits/Records/Faults/Declines` = 0, `gNdsRendererNativeDirectReject.count` = 0.

CSS and Results: no per-kind counters exist in a shipping-like build (`gNdsFtrLean` is tick-HUD only; FP has
`gNdsFighterPacket*`, `gNdsFtrDecline*`, `gNdsRendererNativeFailure`, `gNdsRendererNativeDirectReject`,
`gNdsFtrRejectCountBySlot`). Evidence is indirect. The default camera xobj set `gcAddCameraMatrixSets` (PerspFastF 3 + LookAt 6;
`decomp/BattleShip-main/decomp/src/sys/objhelper.c:283-287,545`) is accepted by lean (`renderer_adapter_matrix.c:5759-5810`;
lean declines only a valid camera modelview, kinds 7/9/11), but I did not audit which camera constructor the CSS and Results
fighter cameras actually use (UNPROVEN 1). Slice 7's natural Results/CSS captures differ from the old-path control by 76 and
44 of 120,000 pixels, "confined to fighter edges" (`phase1-slice7/README.md`), i.e. the accepted LOAD4x3/P' rounding (Mario/Fox only).

## 2. Family table, both ELFs (bytes: `.text / .rodata / .data / .bss`; heap B = main-RAM sections only)

Generated by `tools/lane3_render_md.py` from `lane3_tables.json`. "In old unit" = only the old-fighter entry keeps it alive.

| family | FP text / rodata / data / bss | FP heap B | GATE text / rodata / data / bss | GATE heap B | FP ITCM / DTCM |
|---|---|---:|---|---:|---|
| ndsFighterPacket* recorder side (in old unit) | 7,584 / 0 / 0 / 15,448 | 23,032 | 7,836 / 0 / 0 / 15,448 | 23,284 | 0 / 0 |
| ndsFighterPacket* shared with lean | 2,072 / 0 / 0 / 68 | 2,140 | 2,184 / 0 / 0 / 68 | 2,252 | 476 / 8 |
| \*Production\* (in old unit) | 9,188 / 0 / 0 / 24 | 9,212 | 9,296 / 0 / 0 / 0 | 9,296 | 0 / 0 |
| \*Production\* shared with lean | 1,156 / 0 / 0 / 516 | 1,672 | 1,156 / 0 / 0 / 516 | 1,672 | 0 / 0 |
| owner-image bind / ensure / hat image | 24,616 / 0 / 4 / 1,996 | 26,616 | 13,892 / 0 / 4 / 1,996 | 15,892 | 0 / 0 |
| native-owner material state (old only) | 0 / 0 / 0 / 5,888 | 5,888 | 0 / 0 / 0 / 5,888 | 5,888 | 0 / 0 |
| native-owner materials / workspace (lean uses) | 0 / 0 / 0 / 50,576 | 50,576 | 0 / 0 / 0 / 46,832 | 46,832 | 0 / 0 |
| texture scratch / refresh / key pools | 0 / 0 / 0 / 75,280 | 75,280 | 0 / 0 / 0 / 75,280 | 75,280 | 0 / 0 |
| \*Task36\* remnants | 756 / 0 / 0 / 0 | 756 | 732 / 0 / 0 / 0 | 732 | 0 / 0 |
| ndsRendererAdapterSubmitStageDL | 28,848 / 0 / 0 / 0 | 28,848 | 20,516 / 0 / 0 / 0 | 20,516 | 0 / 0 |
| ndsRendererNativeStageEmit\* | 2,956 / 0 / 0 / 0 | 2,956 | 2,924 / 0 / 0 / 0 | 2,924 | 0 / 0 |
| all native-stage symbols | 36,980 / 16,790 / 20 / 37,228 | 91,018 | 35,560 / 13,707 / 20 / 34,628 | 83,915 | 4,064 / 36 |
| ndsFighterMarioFox\* (in old unit) | 9,248 / 0 / 0 / 0 | 9,248 | 9,036 / 0 / 0 / 0 | 9,036 | 0 / 0 |
| ndsFighterMarioFox\* (rest) | 1,486 / 0 / 0 / 0 | 1,486 | 1,538 / 0 / 0 / 0 | 1,538 | 4 / 0 |
| rebirth halo packets + submit | 7,124 / 5,626 / 48 / 6,960 | 19,758 | 7,200 / 5,626 / 48 / 6,960 | 19,834 | 0 / 0 |
| whispy packet + submit | 15,208 / 288 / 204 / 4,488 | 20,188 | 15,272 / 288 / 204 / 4,492 | 20,256 | 0 / 24 |
| gSYFramebufferSets | 0 / 0 / 0 / 147,840 | 147,840 | 0 / 0 / 0 / 147,840 | 147,840 | 0 / 0 |
| lean path (context, stays) | 27,106 / 0 / 0 / 6,544 | 33,650 | 38,408 / 0 / 0 / 9,416 | 47,824 | 6,664 / 2,084 |
| diagnostics_\*.c globals (all) | 620 / 0 / 5,090 / 22,317 | 28,027 | 660 / 0 / 4,918 / 22,697 | 28,275 | 0 / 158 |
| oracle / shadow / witness / census / probe / proof names | 1,218 / 880 / 38 / 1,076 | 3,212 | 9,128 / 880 / 38 / 9,672 | 19,718 | 8 / 48 |

Units:

| unit | members FP / GATE | FP text / rodata / data / bss | FP heap B | GATE text / rodata / data / bss | GATE heap B |
|---|---:|---|---:|---|---:|
| P0 | 80 / 51 | 26,292 / 12,949 / 0 / 4,196 | 43,437 | 25,236 / 11,970 / 0 / 4,208 | 41,414 |
| P1 | 191 / 161 | 33,804 / 877 / 8 / 47,231 | 81,920 | 36,960 / 617 / 4 / 47,671 | 85,252 |
| OLD_ALL (P0 + P1) | 271 / 212 | 60,096 / 13,826 / 8 / 51,427 | 125,357 | 62,196 / 12,587 / 4 / 51,879 | 126,666 |
| DIAG | 3,162 / 3,196 | 22,688 / 6,968 / 4 / 12,396 | 42,056 | 22,672 / 6,968 / 4 / 12,532 | 42,176 |

Whole-image context, `size -A` (FP / GATE): `.main` 1,525,176 / 1,389,336; `.main.rw` 241,932 / 210,652; `.main.bss` 824,048 /
826,408; `.text.hot` 4,472 / 3,956; `.text.hot.draw` 6,568 / 6,576; `.text.frontend_resident` 1,892 / 0;
`.itcm` 32,336 / 32,528; `.ovl.frontend` 284,256 / 80,480 (+ bss 20,544 / 7,776).

### 2.1 Verdict per family

| family | verdict | evidence |
|---|---|---|
| ndsFighterPacket\* recorder/replay (`BeginRoot`, `Record*`, `FinishRecord`, `TryReplay`, `BuildKey`, `Matches`, `Cmd*`, `EmitCorner*`, `LoadGxComposedRecord`, `LoadSplitMatricesRecord`, ...) + `sNdsFighterPackets` 15,360 B + `sNdsFighterPacketRecorder` | **FALLBACK-ONLY** (P1) | only reached through `ndsRendererExecuteNativeFighterOwnerProduction` and `ndsRendererFighterPacketPrecheck`, both under `DrawForSlot`; `sNdsFighterPackets` is kept only by four `.valid = 0` hooks (`InvalidateSlot`, `Release`, `ndsFtrLeanMaterialize` :12266) |
| ndsFighterPacket\* shared with lean: `ApplyTintPrims` (ITCM), `PatchTexgen`, `LightWord`, `InvalidateSlot/Release/IdleRegion/DmaWait`, `sNdsFighterPacketDmaPending`, `gNdsFighterPacket*` counters | **REACHABLE** (stays) | called from `ndsFtrLeanPacketPatch` / `ndsFtrLeanMaterialize`; counters are read by 4 probes |
| \*Production\* execute + prepare (`ndsRendererExecuteNativeFighterOwnerProduction`, `PrepareProductionRun`, `RebuildProductionRunUv`, `ShadeProductionActions`, `EmitProduction*`, `AbortProductionRun`) | **FALLBACK-ONLY** (P1) | sole caller chain `DrawForSlot`; hierarchy raw emitters are P0 |
| `ndsRendererNativePreflightProductionOwner`, `BindProductionRoot`, `PrimeProductionInputs`, `sNdsNativeProductionResolved*` | **REACHABLE** (stays) | `ndsFtrLeanMaterialize` calls the preflight |
| owner images: `ndsRendererNativeBindOwnerImage` 14,552 B, `EnsureOwnerImage`, `BindKirbyHatImage` 7,864 B, `sNdsNativeOwnerImage` | **REACHABLE** | lean materialisation reads the bound tables; `battleship_ftmanager.c:230-263` ensures images at fighter creation. The 1P-only share of the bind switch is 12 of the 24 owners enabled in FP: the 11 polygon-team owners and Master Hand (`NDS_NATIVE_OWNER_IMAGE_BOSS`); Metal Mario is disabled there (`NDS_NATIVE_OWNER_IMAGE_MMARIO 0`) (see R4) |
| `sNdsRendererAdapterNativeOwnerMaterials` 28,800 B, `OwnerWorkspace` 12,432, `OwnerValidationCache` 7,200, `OwnerModelviews` 2,048 (aliased as `sNdsFtrLeanWorlds`, `renderer_fighter_lean.c:350`) | **REACHABLE** | `ndsFtrLeanMaterializeFor`, `ndsFtrLeanEvent`, `ndsFtrLeanRefreshInputs` reference them. Only `MaterialKeys` 3,456, `TextureCurr/Next/Counts` 2,432 are old-only (in P1) |
| `sNdsRendererHardwareTextureScratch` 32,768, `RefreshLarge` 16,384, `RefreshSmall` 4,096, `IdentPool` 5,852, `TextureCache` 8,128, ... (75,280 B) | **REACHABLE** | users: `ndsRendererHardwareResolveOrBindTexture`, `ConvertTexel01Ci4Direct`, `PrepareBattleStaticTextures`, `PrepareIFCommonAtlas`, `PrepareParticleAtlas` (load time) and per-frame `ndsRendererSubmitParticleQuad`, `ndsRendererSubmitNativeDamageSlash` (reloc graph). Retires only after "all battle textures pre-converted" (architecture A1) |
| Task 36: `ndsRendererNativeStageTask36EnsureWorld/BuildWorld` 756 B | **REACHABLE** (stage world cache; name is historical) | the 24,256 B replay owner, recorder and routing flag are already gone (phase log 09-24) |
| `ndsRendererAdapterSubmitStageDL` 28,848 B (GATE 20,516) | **REACHABLE**, and not a CPU emitter | out-edges (reloc graph) include 52 native-owner callees (49 `ndsRendererSubmitNative*` + `TryNativeEntryEffect`, `PikachuThunder`, `SubmitDamageFlyMDust`; 82,148 B together, 49 of them 69,420 B referenced by nothing else; `ndsRendererSubmitNative*` overall is 98,324 B text); tail is `ndsStageRejectNativeRender` (`renderer_adapter_stage.c:12734,12839`), no display-list interpreter. Callers: every `gcDraw*DObj*` proc, stage scan, results emblem |
| `ndsRendererNativeStageEmit*` 2,956 B (CPU emission of deforming no-Z triangles) | **REACHABLE** | `ndsRendererCommitNativeStageSegment` calls them; architecture doc keeps CPU emission for Dream Land 27, Hyrule 128, Saffron 134, Mushroom Kingdom 64 triangles |
| ndsFighterMarioFox\*: `DLAllDrawForSlot` 8,800, `VisitDLDrawCommand` 448 | **FALLBACK-ONLY** (P1) | the name is historical; it is the live old-path draw, not a proof path |
| ndsFighterMarioFox\* rest (`StageGCDrawAllLoop*`, 8 `*UpdateEnabled` 4 B stubs, 8 `*RunVSBattleUpdate` 2 B stubs) | **REACHABLE** / trivial | battle present chain |
| census / oracle / audit / shadow | mostly compiled out (Audit 0 B, Census 68 B, Oracle 712 B, Verify 8 B). The one big item is **`ndsResetStartupDiagnostics`** (below). GATE-only lab code: 19,718 B (tick-HUD instrument) | `nm` + `lane3_prefix_census.py` |
| rebirth halo packets (`sNdsRebirthHaloPackets` 6,888 + submit) / whispy packet (`sNdsRendererWhispyPacket` 4,144 + submit) | **REACHABLE** | `ndsRendererSubmitNativeRebirthHalo` is sole-referenced from `SubmitStageDL` (every KO); whispy is Dream Land only. Candidates for scene-scoped placement, not deletion |
| `gSYFramebufferSets` 147,840 B | **REACHABLE** | lean lists own 141,440 B: `NDS_FIGHTER_PACKET_ARENA_WORDS 35360` = 4 regions x 8,840 words (`nds_renderer_preamble.c:3254-3301`), each region two lean entries (halves) that start with an embedded `NDSFighterPacket` header (`nds_renderer_native_common.c:11011-11040`); an absent player's region is lent to `ndsBattleIdleScratchAlloc`. The last 6,400 B alias the z-buffer pointer tail (`include/sys/video.h`) and are not used. `ndsRendererFighterPacketRelease` rewrites the arena for the Results wipe (:10793-10806) |
| **DIAG** `ndsResetStartupDiagnostics` 20,712 B text + 50 `words.*` tables 6,968 B rodata + 5 reset helpers + 3,100 counters | **DEAD CODE (config)** | one caller, `mnStartupStartScene` (patched into the decomp copy: `scripts/import-overlays/battleship/src_mn_mncommon_mnstartup.patch:29`, `scripts/publish/patches/ssb-decomp-re-ds.patch:31`); it stores 0 / 0xffffffff / 1 into 1,714 table words and 2,510 direct assignments, i.e. values `crt0` already zero-fills except 422 non-zero ones (`taskman_seam_core.c:52-4596`) |

## 3. Ranked deletion plan

Ranked by bytes / risk. Byte figures are symbol-size sums (static upper bound); the real delta is the link delta minus at
most one 32 B alignment step per removed section and plus whatever GCC re-inlines. Measure with `arm-none-eabi-size -A`.

### R1 - P0: config-dead fighter modes (FP 43,437 B / GATE 41,414 B, risk low, no owner input)

Delete (file:line of the definition; bytes FP):
* `nds_renderer_native_owners.c:254` `ndsRendererExecuteNativeFighterOwnerHierarchy` 7,092.
* `nds_renderer_native_common.c`: `ndsRendererExecuteNativeFighterRootHardware` :15751 6,576; `ndsRendererNativePrepareHierarchyRun` :9119 1,576;
  `ndsRendererNativePrepareDirectRun` :369 1,572; `ndsRendererNativeEmitDenseRawRun` :15310 288; `ndsRendererNativeEmitProductionRaw{Textured,Untextured}Run`
  :9197/:9222 312; `ndsRendererNativeMatrix3Mul20p12` :17028 256; `ndsRendererNativeVisitSourceCommand` :3140 200.
* `nds_renderer_native_fighter_production.c`: `ndsRendererBeginNativeFighterOwner` :545, `EndNativeFighterOwner` :588, `AbortNativeFighterOwner` :635,
  `ndsRendererExecuteNativeFighterRoot` :1338 (580 B).
* CPU rasteriser in `nds_renderer_textures_effects.c`: `ndsRendererSubmitHardwareTriangle` :16578 2,720; `ndsRendererHardwareSubmitVertex` :15847 1,796 (+ `.constprop`,
  `RawZCold` :15823, `DecalCold`); `ClipVertexNdcDepth` :15707 764; `SubmitNearClippedTriangle` :16325 528; `EmitClippedVertex` :16282;
  `TriangleInsideNearPlane` :16418; `ndsRendererHardwareGetLightShadeLut` :1488 404; `sNdsRendererHardwareLightShadeCache(+Next)` 4,196 B BSS
  (`nds_renderer_preamble.c:6431`); `nds_renderer_dl_core.c` `ndsRendererTransformCachedVertex` :1217, `EnsureTransformedVertex` :1252;
  `nds_renderer_textures_effects.c:15` `ndsRendererRecordTransformedTriangle` (92 B).
* `renderer_adapter_matrix.c:8210` `ndsRendererAdapterPrepareNativeOwnerHierarchy` (480 B).
* Resident canonical tables no reader is left for (`nds_native_fighter_owner.generated.inc`): `sNdsNativeFighterPackedCorners` :4724 4,260 and `...Low` :28374 2,742
  (images already omit them: `NDS_NATIVE_FIGHTER_IMAGE_HAS_PACKED_CORNERS 0`, `nds_native_fighter_image.generated.h:16-23`; the only readers, `EmitProductionRaw*Run`
  and `EmitDenseRawRun`, are P0), `sNdsNativeFighterDenseCorners` :2590 4,260, `sNdsNativeFighterRunFirstCorner(+Low)` 256, 22 `*JointSchedule` tables (1,122 B) and
  22 `*BindingJoints` tables (309 B), 1,431 B together, hierarchy only. Stop emitting them in `scripts/fighters/generate_nds_native_owners.py`.
* Source edits that make the compiler drop the rest: make `gNdsRendererFastRunMode` a constant 9 (`nds_renderer_preamble.c:2611-2622`, and the enum trims in
  `include/nds/nds_renderer.h:241-250`), make `no_oracle`/`detailed_output` constants, delete `native_owner_hierarchy_mode`, `native_owner_started` and the branches at
  `renderer_adapter_fighter.c:3789-3806, 4169-4180, 4371-4375, 4418-4423, 4513-4535, 4881-4910`.

Must stay: everything `lane3_boundary_old_fp.csv` lists (122 symbols >= 100 B, 290,972 B, all kept by lean, stage or effect code): e.g.
`ndsRendererHardwareResolveOrBindTexture` 16,052, `ndsRendererNativeApplyMaterial` 1,748, `ndsRendererNativeApplyStateDelta` 884,
`ndsRendererFastPrepareRawSlots` 1,412 (kept by ImpactWave/VisualEffect), `ndsRendererHardwareClipVertex` 952, `ndsRendererInitTraversalState`,
`ndsRendererAdapterGetHierarchyCameraMatrices` 368 (kept by TaruCann/YosterCloud).

Flags to retire once R1 lands: `NDS_RENDERER_FAST_RUN_DEFAULT` (Makefile:2580 and 17 refs; the two mode-8 lab targets at :3755-3780 must move to 9 or go),
`NDS_R2_FIGHTER_NO_ORACLE` (:1851, 11 refs), `NDS_RENDERER_PROFILE_LEVEL` > 0 (776 refs in 37 files; oracle profiles, no bytes in shipping),
`NDS_RENDERER_M2_DETAILED_LEDGER` (417 refs, 10 files), `NDS_TASK91_DRAW_PHASE_CENSUS` (73 refs), `NDS_R2_STRIP_ROUTE`, `NDS_R2_FIGHTER_SOFT_LIGHT_KEEP`,
`NDS_R2_FIGHTER_SHADE_SKIP`, `NDS_R2_FIGHTER_STATESPAN_SKIP`, `NDS_R2_DRAW_SUPPRESS_MASK` (all default 0), `NDS_TASK56_FIGHTER_PRIMITIVES` 0/1 variants (:302, default 2).
Linker: `linker/nds_hot_text.ld:392,397` (two `gNdsFighterPacket*` DTCM placements stay in R1).

Checkers/tests that name these (`lane3_script_refs_old_unit.csv`): `scripts/fighters/check_nds_native_owner_hierarchy.py`, `generate_nds_native_owners.py`,
`task56_fighter_topology_census.py`, `test_native_actor_tarucann.py` (names the hierarchy executor), `scripts/check-gbi-decode-fixtures.ps1`,
`scripts/check-renderer-itcm-placement.ps1`, `scripts/probe-task56-fighter-path.ps1`, `benchmark-renderer-fast-raw.ps1`/`compare-renderer-fast-raw.ps1`,
`census-fighter-draw-phases.ps1`, the `*-state-hash-ab.ps1` / `check-task12-renderer-codegen.ps1` / `capture-*` scripts that pass `NDS_RENDERER_FAST_RUN_DEFAULT`.

Verify: `size -A` delta on FP and GATE, replay digest identical (`scripts/compare-replay-digest.py --sequence`), `check-renderer-itcm-placement.ps1`.

### R2 - DIAG: startup diagnostics reset (FP 38,356 B net / 42,056 gross; GATE 38,200 / 42,176; risk low-medium)

Delete `ndsResetDiagnosticWords` + `ndsResetStartupDiagnostics` (`src/port/taskman_seam_core.c:52-4596`, 4,532 lines), the call the decomp patches insert into
`mnStartupStartScene` (`scripts/import-overlays/battleship/src_mn_mncommon_mnstartup.patch:29`, `scripts/publish/patches/ssb-decomp-re-ds.patch:31`), the declaration
`include/nds/nds_startup.h:729`, and the helpers only it calls: `ndsAudioFgmDiagnosticsReset` (`nds_audio_fgm.c:1984`, 736 B), `ndsAudioBgmDiagnosticsReset`
(`nds_audio_bgm.c:1067`, 688), `ndsAudioAssetDiagnosticsReset` (`nds_audio_assets.c:592`, 212), `ndsCollisionRuntimeDiagnosticsReset`
(`diagnostics_collision_runtime.c:1`, 56). Two helpers are functional resets, not counters, and need a new home or a check that boot state is already zero:
`ndsControllerPlaybackReset` (`controller_backend.c:119`, 188) and `ndsPlatformClearOriginalSpritePreview` (`nds_platform.c:2006`, 48).
What frees: text 22,688 + rodata 6,968 + bss 8,700. What does not: 2,344 B of counters named by a script or probe (keep the symbols; 590 of the 3,162 are named,
480 of them by `scripts/verify-battle-mariofox-gcrunall-loop-harness.ps1`, 47 by `scripts/menus/test_sobj_repeat.py`, 31 by `scripts/verify-opening-skip.ps1`) and 1,372 B of
globals the function sets non-zero (422 of them: 139 all-ones table words plus 283 direct assignments): make them initialised data.
`lane3_diag_net.py` reproduces the split. Test: `scripts/test_diagnostic_reset_words.py` (delete or replace). Docs naming the counters: `docs/DIAGNOSTIC_REFERENCE.md`.

### R3 - P1: the old production / packet / plan path (FP 81,920 B / GATE 85,252 B; risk medium-high; needs the 1P decision)

Delete: `ndsFighterMarioFoxDLAllDrawForSlot` (`renderer_adapter_fighter.c:3413-5159`, 8,800 B incl. the static `persistent_renderer_vertices` 5,780 B BSS),
`ndsFighterIntroTransientSubmit` (:5263-5324) or route it (R4), `ndsFighterMarioFoxVisitDLDrawCommand` / `ndsFighterDLDrawAppendTriangle`
(`renderer_adapter_legacy_dl_probes.c:405-543`), the production execute (`nds_renderer_native_fighter_production.c:36` 3,056 B, `:14` `AbortProductionRun`),
`ndsRendererNativePrepareProductionRun` (:9106, 2,468), `RebuildProductionRunUv` (:8530), `ShadeProductionActions` (:7318), `EmitProduction{PrimitiveGroups,CrossRun}(+Packet)`
(:9314-9560), `ndsRendererR2RunTextureMemo{Apply,Fill,Fence}` + `sNdsR2RunTextureMemo` (:7991, 7,504 B), `ndsRendererR2OpenTintBatch` (:7196), `ndsRendererR2WriteLightVector` (:6828),
`ndsRendererNativeFighterBindingParents` (:16323) and `CrossPaletteSlots` (:16674) (1,076 B each), the whole recorder (`ndsFighterPacketBeginRoot ... FinishRecord`,
`TryReplay` :10402 2,712, `BuildKey`, `Matches`, `Precheck`, `LoadGxComposedRecord`, `LoadSplitMatricesRecord`, `Record*`, `EmitCorner*`), `ndsRendererLoadHardwareGxComposedMatrices`,
the source-compose helpers (`ndsRendererAdapterComposeOwnerWorldsFlat/Source`, `SourceWorld*`, `PrepareOwnerMatricesPerBinding`), and the state only they use:
`sNdsFighterPackets` 15,360, `sNdsNativeFighterRunUvInputs` 7,504, `sNdsFighterDrawPlan` 3,776, `sNdsRendererAdapterNativeOwnerMaterialKeys` 3,456,
`sNdsRendererAdapterNativeOwnerTextureCurr/Next/Counts` 2,432, `sNdsR2GxSlotTable` 800, `sNdsFighterPacketRecorder` 60. Full list, 191 members with file:line:
`lane3_members_p1_fp.csv`; source spans `lane3_spans_p1_fp.csv` (about 5,700 lines).
Hook edits (these keep dead state linked today, found by `--cut-edge` analysis): `ndsRendererNativeForgetFighterRunUvTables` (RunUvInputs),
`ndsRelocPrepareSceneCache` / `ndsFighterIntroTransientReset` / `ndsFighterDisplayContractSubmit` (DrawPlan), `ndsFighterRendererInvalidateMaterialCachesForSlot` (MaterialKeys),
`ndsRendererFighterPacketInvalidateSlot` / `Release` / `ndsFtrLeanMaterialize` :12266 (`sNdsFighterPackets`), `ndsRendererNativeApplyMaterial` (recorder hook).
Replace the fallback with a fail-closed stub in `ndsFighterDisplayContractSubmit` that calls `ndsFighterRejectNativeRender` (240 B, keep it: it feeds `gNdsFtrRejectCountBySlot`
and `gNdsRendererNativeFailure`, which 12 scripts read) and, to keep today's CSS assertion, `ndsPreviewPackLoadHalt` (`renderer_adapter_fighter.c:4593-4648` halts on a packed
preview that lost its native owner; after deletion that becomes an invisible fighter unless the stub keeps the halt).
Must stay (lean needs them): `NDSFighterPacket` (the lean entries embed it), `ndsFighterPacketPatchTexgen`, `ApplyTintPrims`, `LightWord`, `ndsRendererNativePreflightProductionOwner`,
`ndsRendererR2BuildDenseNormals`, `ndsRendererR2ResolveEpochShade`, `ndsRendererNativePrepareTexgenDirectionQ15`, `ndsFighterDrawPlanResolve` (kept by `ndsFtrLeanEvent`),
`ndsRendererAdapterValidateNativeOwnerCached`, `ndsRendererAdapterBuildNativeMaterialSnapshot`, `sNdsRendererAdapterNativeOwnerMaterials/Workspace/Modelviews`,
`sNdsNativeFighterOwnerExecution`, `sNdsFighterDisplayContract`, `gSYFramebufferSets`. Flag `NDS_R2_FIGHTER_PACKET` cannot simply go to 0: `NDS_FTR_LEAN_LIVE` requires it
(`renderer_fighter_lean.h:75-84`); rename it to a lean-arena flag. Also retire `NDS_R2_FIGHTER_RUN_MEMO`, `NDS_FTR_PLAN_ROUTE` (:1476) and the `gNdsFtrPlan*`,
`gNdsFtrDecline*` lab words (read by `verify-p2-four-fighter-stress.ps1`, `probe-yoshi-tour-full.ps1`, `census-results-frame-cost.ps1`, `verify-p2-falcon-modelpart.ps1`).
Checkers/tests: `scripts/fighters/test_camera_texgen_state.py`, `test_intro_transient_context.py`, `test_renderer_storage.py`, `test_native_failure_record.py`,
`fighter_list_emitter.py`, `check_nds_native_owner_packet.py` (compares recorded packets to the host list; the recorder is its only runtime reference), `fighter_list_proof.py`
(host-only, stays), `check_native_owner_geometry_closure.py`, `scripts/check-r2-shade-twin.py`, `check-r2-light-stretch.py`, `check-fighter-draw-global-texture-state.py`,
`analyze-fighter-draw-reconciliation.py`, `probe-p2-fourcpu-sparse.ps1` and `menus/probe-p2-shell.ps1` (read `gNdsFighterPacket*`; the four counters are kept anyway), `verify-p2-samus-state-tour.ps1`.

Gates before deleting (all measurable without a new ROM design):
1. Owner decision on 1P (R4 options). Polygon team, Metal Mario, Master Hand battles and the intro transient have no lean coverage (`renderer_fighter_lean.h:256-259`);
   after R3 they draw nothing.
2. A natural-play probe on the shipping configuration over CSS, VS on all 9 stages, Results, autodemo and Training, reading `gNdsFighterPacketRecords`,
   `gNdsRendererNativeDirectReject.count` and `gNdsFtrRejectCountBySlot` (all present in FP): all must stay 0. Today's evidence for those scenes is indirect.
3. Fix or accept the Link Texgen decline: it fired in pre-fix builds today and the old path failed on the same frames, so R3 changes nothing there but the stub must not
   corrupt lean state (`ndsFtrLeanInvalidate`).

Alternative if 1P stays and R3 cannot wait: place the P1 functions in `.ovl.frontend` behind a scene-kind guard (the 1P-overlay pattern, `2026-09-28_p2-2p8-1p-overlay/README.md`),
so the code is resident only when 1P runs and the VS loan grows by up to 81,920 B. That is a move, not a deletion, and it needs the ELF crossing gate's reviewed pairs.

### R4 - 1P out of the shipping flag block (owner decision; est. 20-56 KB more, not measured)

`Makefile:3402` `override NDS_P2_1P_GAME := 1` (and the `NDS_P2_N*`, `NDS_NATIVE_OWNER_IMAGE_BOSS` derivations) is what keeps R3 needed. What flipping it frees
beyond R3, by name: 1P owner tables `sNdsNative{N*,Boss,MMario}*` rodata 7,640 B and 2,976 B of mutable image-table BSS, the 1P half of the
`ndsRendererNativeBindOwnerImage` switch (+7.2 KB, 1P-overlay README "Left resident"), 1P campaign BSS (~1.7 KB `sc1pgame.c` ...); the named items sum to about 19.5 KB,
which is the low end of the range. A same-target pair of saved builds, `builds/build-p2-shell`
(1P 0, 05:37) vs `builds/build-ovl1p` (1P 1 after the overlay change, 11:16), differs by +56.5 KB in `.main + .main.rw + .main.bss`, but the pair is six hours of
unrelated commits apart, so treat 20-56 KB as the range and measure with `make smash64ds-p2-shell-freeplay-hwtri NDS_P2_1P_GAME=0` (not run here).

### Not deletable now (REACHABLE), with what retires each

| item | FP heap B | blocked on |
|---|---:|---|
| texture scratch / refresh / key pools | 75,280 | battle textures pre-converted; per-frame users `SubmitParticleQuad`, `SubmitNativeDamageSlash` |
| native-stage machinery (`ndsRendererPrepareNativeStageOwner` 11,808, workspaces, dense, caches) | 91,018 | stage compiler for the CPU-emitted deforming triangles |
| `SubmitStageDL` hub + 52 native MISC owners (82,148 B; `ndsRendererSubmitNative*` overall 98,324 B text) | 28,848 + owners | NDL / native draw list (Phase 2) |
| halo, whispy, entry-effect composed/modelview packets | 19,758 + 8,192 | NDL; or scene-scoped placement |
| owner materials / workspace / owner-execution (lean state) | 50,576 + 8,824 | lean-list slimming, not deletion |

## 4. Ledger check: A7 "A1 retirement" row against the ELF

| ledger item (architecture A1, `INVESTIGATION_RENDER.md` Q6) | claimed | in FP now |
|---|---|---|
| fighter BSS ~98 KB | 98 KB | old-only BSS 51,427 B (`sNdsFighterPackets` 15,360, RunUvInputs 7,504, RunTextureMemo 7,504, persistent vertices 5,780, LightShadeCache 4,196, DrawPlan 3,776, MaterialKeys 3,456, ...); the fighter BSS that stays is lean state, not counted in that figure (OwnerMaterials 28,800, Workspace 12,432, OwnerExecution 8,824, DrawMemo 5,408, DisplayContract 5,472 = 60,936 B) |
| stage BSS ~70 KB | 70 KB | Task 36 replay owner (24,256) already gone; remaining stage BSS 37,228 is live |
| effect packets ~19 KB | 19 KB | 19,758 B live |
| texture scratch / refresh / key pools ~72 KB | 72 KB | 75,280 B live |
| executor `.text` ~180 KB | 180 KB | old-only text 60,096 B (33%); the rest is stage native, MISC owners, matrix adapter, lean |

## 5. Risks

* Deleting P1 removes the only path that turns a lean decline into anything but a missing fighter for that frame, and removes the CSS packed-preview halt unless the stub keeps it.
* It also removes the only runtime word-level oracle for lean lists (routes 2/3 compare against the recorder; lab-only, tick-HUD ROMs). `fighter_list_proof.py` and the replay digest stay.
* R2 deletes a boot-time reset that the startup scene may run more than once per session (BattleShip's autodemo exits to `nSCKindStartup`; the DS scene manager notes it "lands here on every
  boot", `nds_scene_manager.c:278`). Counters would then accumulate across attract loops. Harmless to gameplay, not to verifiers that read per-scene deltas.
* Layout: freeing 82-164 KB shifts `.main` and cache phase; the 09-28 heap-cliff note says two ROMs 1,888 B apart read STG 169K vs 230K until the arena chooser was fixed. Re-measure with the fixed chooser.
* The FP ELF includes walk and argmax flags; run the size check on the published `smash64ds` configuration before quoting a delta.
* Other lanes are editing the same tree and re-linking shared build directories (the gate ELF changed under me at 14:55).

## 6. UNPROVEN

1. CSS, Results, autodemo and Training lean coverage per kind in the shipping ROM (no lean counters there; indirect evidence only, Mario/Fox captures), including which camera
   constructor the CSS and Results fighter cameras use (only the default xobj set 3 + 6 was checked against lean's camera rule).
2. Any 1P behaviour: polygon team, Metal Mario, Master Hand, intro transient. Known to need the old path, never measured.
3. 18 of the 20 decline reasons were never observed; unrecorded costumes, hats, item states or stage hazards could trigger them. The recorded runs are tick-HUD lab builds and heavily repeated rosters.
4. Every byte figure is a sum of symbol sizes from a graph of `--emit-relocs` relocations (58,264 edges FP). Calls through `bx rX` that are not preceded by a relocated pointer would be missed; a cross-check against a literal-pool scan
   differs by 86 edges the scan missed and 131 it invented, and changed OLD_ALL by 568 B. Not a link.
5. R2: whether the startup scene re-enters within a session; whether `ndsControllerPlaybackReset` and `ndsPlatformClearOriginalSpritePreview` do anything at boot beyond zeroing.
6. R4's 20-56 KB range (no 1P-off shipping-shell ELF from a common commit exists).
7. That a build with P0 removed still links with the two mode-8 lab targets and the profile-level 1/2 configs (they are compiled out of both saved ELFs and were not built).

## 7. Reproduce

Everything is in `tools/` (Python 3, devkitARM binutils, no build):
`lane3_maptab.py` (linker map -> per-section rows), `lane3_symtab.py` (nm sysv), `lane3_srcmap.py` (nm -l source lines), `lane3_relocgraph.py build` (exact graph from `.rel.*`),
`lane3_refgraph.py` (literal-scan graph, cross-check), `lane3_unit.py` / `lane3_cutset.py` / `lane3_boundary.py` / `lane3_reset_hooks.py` (units, must-stay set, hook finder),
`lane3_report_tables.py` (all tables and member CSVs), `lane3_render_md.py`, `lane3_unit_summary.py`, `lane3_spans.py`, `lane3_script_refs.py`, `lane3_diag_net.py`,
`lane3_scan_counters.py` / `lane3_native_failures.py` / `lane3_lean_coverage.py` (run JSON evidence), `lane3_fn_referrers.py`, `lane3_callees.py`, `lane3_prefix_census.py`, `lane3_tu_sizes.py`, `lane3_filesyms.py`.
Data here: `lane3_tables.json`, `lane3_members_{p0,p1,old_all,diag}_{fp,gate}.csv`, `lane3_boundary_old_{fp,gate}.csv`, `lane3_script_refs_{old_unit,diag_unit}.csv`, `lane3_spans_p1_fp.csv`.
Sequence: `nm --format=sysv -S <elf>`, `nm -l -S --defined-only <elf>`, `lane3_srcmap.py`, `lane3_relocgraph.py build <elf> <nm> <pkl>`, then `lane3_report_tables.py <dir with fp/gate .graph.pkl + .src.csv> <out>`.
