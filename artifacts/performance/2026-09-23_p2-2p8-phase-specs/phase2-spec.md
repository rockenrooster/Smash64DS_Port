# Phase 2 spec: Stage + MISC (pillar A1): compiled stage, native draw list
Read-only investigation, 2026-09-22, worktree as found (Phase 0 edits in progress). MEASURED = cited artifact/profile or a static count off generated
tables; ESTIMATE = arithmetic. Abbrev: OWN/AST/TFX/NRC/PRE = src/nds/nds_renderer_{native_owners,assets,textures_effects,native_common,preamble}.c;
RAS = src/port/renderer_adapter_stage.c; RBM = src/port/reloc_backend_movement.c; OMB = src/port/opening_movie_backend.c; TSB = src/port/
taskman_seam_battle_host.c; SEL = src/nds/nds_native_stage_select.inc; GEN = scripts/stages/generate_nds_native_stage.py; n0409 = self tk/fr in
artifacts/performance/2026-09-22_p2-2p8-architecture-baseline/attrib_n0409.json; T103/T104 = docs/optimization/archive/ClaudeOpus5_Task10{3,4}_*.md.
Census = my per-run count over src/nds/nds_native_stage_*.generated.inc with the runtime's effective rigid mask (rigid & ~camera, SEL:3607-3609).

## 0. Verdicts
1. **STG (326K) is executor work.** The bracket is four exhaustive, non-nesting sites (A1). OWN:3046-3048's "~331,300 unattributed" is Task 103's
   opening hypothesis, which the same task closed: E3 partitioned the whole bucket to 192 ticks (0.05%): prepare 60%, display commit 40%,
   DObj traversal 0 (T103:115-136). Task 53's "stage prep reappeared as OTHR" (docs/PERF_LEDGER.md:6653-6664) predates the WAIT split. OTHR was
   mostly VBlank idle then (TSB:1034-1043), and Task 104 banked −22,016 STG as −26,240 WORK-H with +36,416 WAIT (T104:72-95).
2. **Almost no stage triangle needs CPU per-vertex work.** Every one is rigid within its binding. The only exceptions are cross-binding
   corners (DL 10, MK 4) and eye-plane straddlers. Cross corners also fit the GX with MTX_STORE/RESTORE (A3). The "non-rigid" counts
   (Hyrule 128, Saffron 134, MK 64) come from `rigid_binding_mask = 0 /* not derivable here */` (SEL:1243 Sector, 1303 Hyrule, 1365-1368 MK,
   1423-1426 Zebes, 1481-1502 Saffron). Nothing deforms.
3. **The near_inside bug is CONFIRMED in code** (A8). The compiled design removes it.
4. **The MISC residual is traversal, capture and HUD work outside every sub-bracket, plus tick-HUD instrument.** B names each one with a
   counter. MWPN/MEFX/MPTC/MTEX size the submits.

## A. STAGE
### A1. What runs inside the STG bracket (per presented frame, fourcpu tick-HUD config)
| # | site (writes gNdsTickHudStageTicks) | body | evidence |
|---|---|---|---|
| 1 | RBM:15550-15566, 1 call | ndsRendererAdapterPrepareNativeStageOwner (RAS:3537-3900): BG2 wallpaper (:3573-3580), Task44 admission (:3595-3624), rigid validate stride 8 (:3743-3747), camera + 27 non-rigid binding matrices on DL (:3753, :3135-3294), materials (:3770), hidden mask (:3813-3820), ndsRendererPrepareNativeStageOwner (OWN:4210): r2 reuse skips PrepareRun and the whole preflight (:4268-4285, :4535-4540, :4706-4712) | T103 E4: 234,926 July, before T104/R2 |
| 2 | RBM:14977-14999, once per displayed GObj of any kind | ndsRendererAdapterCommitNativeStageDisplay (RAS:3903-3969): an 8-pointer segment-match loop for every GObj. On a match: materials (:3489-3511) + ndsRendererCommitNativeStageSegment (OWN:5081-5524), which waits on packet DMA (:5086), runs the texture proof (:5142), Task36 begin/replay (:5197-5210, :5287-5335, synchronous DMA spin :3174-3181), BeginRun + generic emit (:5345-5426) | 27.6 calls/fr, 2-fighter era (T103:126) |
| 3 | RBM:15244-15281 | DObj traversal: 0, because proc_display is skipped when the commit handles the GObj (OMB:1700-1703) | T103:128 |
| 4 | RBM:15590-15609 | Finish (OWN:5526-5569), Task36 finish-frame | 417/fr (T103:127) |
Not in STG: gcCaptureCameraGObj (OMB:1637-1715), RecordCapturedDisplay's own capture/classify (RBM:14913-14975) and MarkDisplayProcHeads (:14925)
all land in MISC. **Current composition (n0409 self tk/fr, membership ESTIMATE):** commit side ~130K (CommitNativeStageSegment 27,835;
EmitNoZTriangle 18,199; BeginRun 16,447; EmitNoZVertex 14,155; LoadNoZMatrix 11,291; Task36EnsureWorld 6,904; CommitNativeStageDisplay 6,876;
BindingHidden 5,479; Task36ReplayRun 4,632 at CPI 12.2, i.e. DMA spin; cross 3,386 + 2,965; AccountShortfall 3,443). Prepare side ~45K
(BuildPersistentStageWorldMatrix 15,010; adapter prepare 7,699; SourceKeyMatches 5,673; FindStageWorldEntry 4,714; renderer prepare 3,708;
BindingMatrix 3,308; EnsureStageWorldCache 2,195; wallpaper 902). Shared matrix kernels, memcpy/memset of 64 B matrices and stall make up the rest.
Unattributed-candidate verdicts: stage display-proc traversal (not in STG); RecordCapturedDisplay (MISC, except the per-GObj loop in row 2);
Task36 capture (once per scene), replay (in STG, DMA spin); per-segment Begin/Commit (in STG, named above); texture binds (none during battle,
T104:156-157; BeginRun rewrites TEXIMAGE on 18/21 runs, OWN:1334-1337).

### A2. Run classes: per-frame inputs and compiled form
Census (tris): DL 202 = noZ rigid 76 / camera 23 / dynamic 17 / cross 10; raw 66; range 10. Castle 136 = noZ 45/4/13; raw rigid 46 + dyn 10; range
rigid 10 + dyn 8. Hyrule 206 = noZ dyn 128; raw 13; range 65. MK 176 = noZ dyn 64; raw cam 36 + dyn 40; range 32 + cross 4. Jungle 182 = noZ rigid 84 /
cam 52; raw rigid 6 + dyn 20; range rigid 20. Sector 299 = noZ cam 4 + dyn 27; raw 81; range 187. Saffron 243 = noZ cam 4 + dyn 130; raw 73; range 36.
YI 164 = noZ rigid 69 / dyn 18; raw 35; range 42 (all rigid). Zebes 151 = raw 92; range 59. CPU per-triangle no-Z today = camera + dynamic + cross
(DL 50, not the 27 in INVESTIGATION_RENDER Q4, which omitted the camera subtraction).
| class | today (per frame) | inputs per frame | compiled form (words in list; P = patch site) |
|---|---|---|---|
| segment prologue | Task36BeginSegment: projection + camera LOAD4x4 (OWN:2594-2647) | camera view; FOV (constant in match, AST:6567-6588) | MTX_MODE P + LOAD4x4 P' (row 3 scaled 2^-8 as :2619-2627; P on FOV change) + LOAD4x3 view (P, one shared camera context/frame) |
| binding, STATIC | PUSH + MULT4x4 world (:2649-2707) | none | PUSH + MULT4x3 baked world (GEN baked_stage_world_matrices :2082) ... POP |
| binding, ANIMATED (AObj/grcommon joints) | CPU persistent world cache (RAS:3245) + CPU proj×view×model | DObj TRS | PUSH + MULT4x3 P (Phase-1 ITCM compose, ~450 tk ESTIMATE) ... POP |
| binding, CAMERA (flags 2/4/8, MVP-recalc 46/48/0x47) | ApplyMvpRecalc path (RAS:3232-3253) | camera rows + DObj translation | PUSH + MULT4x3 P (billboard rows from view, ~150 tk ESTIMATE) |
| raw (class 0) | 1 CPU matrix/run (OWN:2910-2920) | binding world | binding block above; source Z; GX clip |
| range (class 6) | shifted coords, identity proj (:2921-2951) | binding world | vertices pre-shifted (packed_cache_shift); shift folded into 4x3 scale |
| projected no-Z | per triangle LOAD4x4 proj, z col = w·c (:2783-2803, :3297-3308); CPU LoadNoZMatrix for non-rigid (:3314-3347) | live painter counter (TFX:13669-13714) | per triangle: MTX_MODE PROJ + LOAD4x4 P_i + MTX_MODE POS. The P_i z-column word(s) are patched each frame from the live counter (see note) |
| cross (flag bit 0) | CPU transform + clip + LOAD4x4/vertex (:3405-3504) | the corners' binding worlds | MTX_STORE each involved binding to slot >= 8 once per segment; per corner MTX_RESTORE slot + VTX (static words; GE +36 cyc/corner) |
| Task 36 replay | runtime-captured words, DMA0 sync (:3112-3240), segs 5/7 only (AST:6514-6516) | none | deleted; generated lists replace it |
| material anim | MObj snapshot + commit (RAS:3436-3511) | MObj texture_id/palette/prim | TEXIMAGE_PARAM/PLTT_BASE P from an image→word table built at load; COLOR P only for runs whose material colour animates |
| hidden binding | per-run BindingHidden (OWN:3235) | DOBJ_FLAG_NOTEXTURE | job table split at binding boundaries, rebuilt only when the 64-bit hidden mask changes |
Painter note: today the depth counter is global and shared with no-Z effects and weapons. Weapons clear Z before link 14 (RBM:13550-13555).
Replay bakes capture-time depths and decrements the counter in bulk (OWN:5302-5308), so a weapon drawn before seg 7 can tie a baked slot
(INFERRED latent defect). A projection baked once and repatched only on FOV change is therefore not exact. Patch the 1-2 z-column words per
triangle from the live counter at segment submit: DL 126 no-Z = ~126-252 word writes (ESTIMATE ~1.5K tk). Band switching stays as
EnterProjectedForeground (TFX:13696-13714) at the first Z run.

### A3. CPU per-vertex work that remains
- Cross triangles: DL 10 (flowers, seg 3/6), MK 4 (scale strings). They move to STORE/RESTORE (A2) and the stack gate stays flat.
- Eye-plane straddlers. With z=c·w both clip planes collapse to w=0 and straddling triangles drop whole. Per frame and per ANIMATED/CAMERA no-Z
  binding, test the generator's bounding sphere against the eye plane (~60 tk). Only a straddling binding routes its triangles to the kept
  cold clipper (:3506-3578), with inside-ness computed live. Expected 0/frame on backdrops. MEASURED hint: YI NearFanCount is exactly
  4/present (docs/p2/BUG_NOTES.md:3091-3094), and the Castle roof straddles during the entry pan (:3555-3557).
- Nothing else. Stage vertices are rigid within their binding. The 128/134/64 figures are the rigid mask defaulting to 0 (0. item 2).

### A4. Per-stage dynamic inputs, grouped into what the stage actor provides
The actor does not change: source procs keep running at 60 Hz. The actor supplies matrices and words (packet side) and a native draw for
ground actors currently drawn in MISC (RBM:15213-15235).
| stage | packet-side per-frame inputs | ground/effect actors (MISC today) | stage items |
|---|---|---|---|
| Dream Land | Whispy eyes/mouth AObj + MatAnim swaps (decomp gr/grcommon/grpupupu.c:74-111, 571-621); flower wind (:407-560; seg 3/6 cross); 11 camera bindings; 4 material events on bindings 20,22,31,32 (RAS:3442-3443); water MatAnim frozen | Bronto/Dedede EfBronto (RBM:14318-14410); Whispy wind particles | none |
| Peach's Castle | map_nodes AnimJoint on the ground GObj (grcastle.c:43-46): sliding platform; 2 camera bindings | Lakitu EfLakitu (RBM:14189-14316) | GBumper (palette flash, RAS:6163-6167) |
| Sector Z | 6 camera bindings; Great Fox range 187 tris (source Z) | Arwing (grsector.c:334, :1116; RBM:14529); rocket/ship (efground) | Arwing lasers as weapons (RAS:6168-6173) |
| Congo Jungle | platforms (grjungle.c:41); 15 camera bindings; waterfall UNKNOWN | TaruCann barrel (grjungle.c:120-123; RBM:14413); bird | none |
| Planet Zebes | map AnimAll (grzebes.c:94-102); 19 material events; 14 camera bindings | acid (RBM:14507); Ridley/ship | none |
| Hyrule Castle | none derived (masks 0/0) | tornado = ground obstacle + particles | none |
| Yoshi's Island | layer0 AnimJoint (gryoster.c:226-246); 2 material events | 3 cloud actors with prim alpha (RBM:14101-14188); birds | none |
| Saffron City | gate open/close AObj, bindings 17-19 (gryamabuki.c:121-122, 263; GEN:4958-4981) | gate (RBM:14551); birds | Pokémon: glucky/porygon/hitokage/fushigibana/marumine (RAS:6179-6194) |
| Mushroom Kingdom | scale retract AObj (grinishie.c:260-261); 2 camera bindings; 4 material events | scales (RBM:14633) | POW, Pakkun (RAS:6087-6097) |
All nine stages share one interface: {view (shared), per-binding 4x3 source (STATIC baked / ANIMATED DObj / CAMERA), material words, hidden
mask, painter base}. Ground actors become NDL records (B2) with compiled bodies, counted as MACT (B3).

### A5. Generator changes (GEN and friends)
1. Derive the per-binding class for every stage from source instead of pins. Inputs: grdisplay.c:208 `gcAddAnimAll(gr_desc->anim_joints,
   p_matanim_joints)` per layer, the grcommon runtime attach sites listed in A4 (extend `_ANIMATED_JOINT_TABLES` GEN:4958 beyond Saffron), and
   DObj flags 2/4/8 (GEN:5125-5139), with ancestor propagation (GEN:5091-5106). Emit rigid, animated, camera and cross masks. Retire the pins
   `_BLOB_RIGID_MASKS` (GEN:4935-4938) and the SEL:1365... zeros. The checker asserts pinned == derived (pattern: test_yamabuki_gate_animation.py).
2. Emit per stage x segment a GX program in commit order (head order 0,2,1,3 for DLLink packets, OWN:5100). Resolve at generation what
   PrepareRun resolves every build today (OWN:1737-1987): poly_fmt, alpha test/ref, packed vertex colour, UV scale and offset, coordinate
   shift, cull. Consume the state policies, deltas and spans. The runtime ApplyStateSpan/PrepareRun head go away.
3. Emit a patch table {word offset, kind: VIEW | PROJ_BASE | NOZ_Z(i) | BIND_WORLD(b) | TEXIMAGE(run) | PLTT(run) | MAT_COLOR(ev) | SPLIT(b)},
   texture sites as symbolic (texture key → VRAM word at load), and per-binding bounding spheres.
4. Put it in the stage blob (src/nds/nds_native_stage_blob.c loader, GEN build_stage_blob :5289) and in the linked Dream Land include. Reuse
   task36_replay_word_upper_bound (:1758) and build_generated_segment0_program (:3756) as the emit precedent.
5. Checker (scripts/stages/check_nds_native_stage.py): decode the program back to triangles and compare with dense tables and corners
   (bit-exact). Transform sample cameras with native_matrix_math.py against the CPU path (≤1 px). Check patch coverage (every non-STATIC
   binding has BIND_WORLD, no STATIC one does). Pin word counts and sizes per stage.

### A6. Runtime structures, API, hook points
- `typedef struct { u16 word_count, patch_count, first_patch, noz_count; u32 words_off; } NDSStageProgSeg;` + `NDSStageProgPatch {u16 off; u8 kind,
  arg;}`. The words are a mutable RAM copy, ~30 KB DL (INVESTIGATION_RENDER Q6; ESTIMATE ≤~30 KB for any VS stage).
- `ndsStageProgPrepareFrame(camera)` replaces RAS:3537 when the word is on. It keeps the admission guard (sNdsRelocStageAssetMutation,
  RAS:3602-3606) and the wallpaper call, and writes VIEW, BIND_WORLD (animated + camera), MAT, NOZ_Z and SPLIT patches. It runs the straddle
  tests and flushes the patched lines (or writes through the uncached mirror if calico exposes one; measure both).
- `ndsStageProgSubmitSegment(k)` replaces OWN:5081 inside RAS:3940/3956 and starts async GXFIFO DMA0 jobs. The next CPU GX writer keeps
  NDS_FIGHTER_PACKET_DMA_WAIT (OWN:4214, :5086, NRC:1092). `ndsStageProgFinishFrame()` replaces RBM:15598.
- The GObj→segment lookup replaces RAS:3915-3923's per-GObj loop (a side table keyed by GObj, filled at topology collection).

### A7. Deleted by the end of the phase (D2; sizes MEASURED nm -S, fourcpu tick-HUD ELF)
sNdsRendererTask36ReplayOwner 24,256; sNdsRendererAdapterNativeStageWorkspace 15,816; sNdsNativeStageOwnerExecution 7,360; sNdsNativeStagePreparedDense
6,904; sNdsRendererAdapterStageVertexCache 5,780; sNdsRendererStageTextureSites 5,632; sNdsNativeStageValidationCache 1,948; StagePersistentStats/State
1,300+848; sNdsNativeStageHeadStats 2 x NDSRendererStats (OWN:4208) = ~73 KB BSS. Code: Task36 replay/capture (OWN:3044-3240;
AST:6479-6800+); PrepareRun (ends OWN:1989), ApplyStateSpan, preflight (OWN:4210-~4975; ndsRendererPrepareNativeStageOwner alone is
12,384 B .text); generic no-Z/raw emitters and the stage persistent world cache (renderer_adapter_matrix.c). Kept as cold path: the near
clipper (OWN:3506-3578), for straddlers only.

### A8. The near_inside bug: CONFIRMED in code (visual effect UNMEASURED)
near_inside is written only in PrepareRun, for PROJECTED_NO_Z vertices of non-rigid bindings, from `binding_composed` (camera × world;
OWN:1895-1948). It is read by EmitNoZTriangle (cull if 0 inside, CPU clip if partial, OWN:3845-3864). The r2 key {valid, textures proven,
topology generation, stamp, config pointer, asset bases} (OWN:4268-4279) has no camera term. On a hit (steady state, NDS_R2_STAGE_DIRECT=1
and PREFLIGHT=1 in builds/build-p2-fourcpu-tickhud/nds_build_config.h), the preflight loop `continue`s (:4535-4540) and PrepareRun is skipped
(:4706-4712). The key is invalidated only on fallback (:4969) or a texture reset (TFX:3150). So near_inside, and the w==0 decline
(:1926-1935), stay frozen at the camera of the last rebuild frame. That is the first armed battle frame (entry camera) or the frame after
any owner reject: 197 rebuilds in one measured match, all after rejects (AST:6455-6459). Affected: camera/dynamic no-Z
triangles (DL 40, Hyrule 128, Saffron 134, MK 64, Jungle 52, Sector 31, YI 18, Castle 17). Cross triangles recompute clip live
(OWN:3457-3476) and are unaffected. Rigid ones are forced TRUE (:1899-1905), which goes stale only if the runtime rigid mask drops to 0
(RAS:3402, :3426). A frozen "inside" draws through the HW (drops if straddling). A frozen "outside" culls a visible triangle. A frozen
"partial" CPU-clips every frame: YI's constant 4/present fits this. The compiled design has no cached per-vertex camera state and tests
straddling live per binding (A3), so the bug disappears. Cheap confirmation: during a gdb pause-camera orbit,
gNdsR2StagePrepareBuildCount stays flat and gNdsNativeStageNearFanCount/present stays constant.

### A9. Budget (ESTIMATE, DL): view 0 (shared) + 16 animated x 450 + 11 camera x 150 + 126 NOZ_Z patches ~1.5K + 4 material ~0.4K + 8
segment kicks ~1.2K + flush ~3K + admission/wallpaper ~2.5K ≈ **18-22K CPU**. The GE side stays at ~15K cycles (INVESTIGATION_RENDER Q4). Saffron if
classified (3 animated + 5 camera): ~10K. If unclassified (21 animated): ~17K. Every VS stage stays ≤40K CPU. Words ≈ tris x 12 + noZ x 21 +
bindings x 14 + runs x 8 → ≤~7K words (≤28 KB) per stage.

## B. MISC
### B1. What MISC holds, and every display proc reached in battle for non-fighter, non-stage GObjs
MISC = max(DRAW − FTR − STG − BG − FG, 0) + FLUSH (TSB:952-959). DRAW = TSB:739-749. BG is never accumulated (only reset, TSB:1204).
Traversal order = the main camera's six capture passes {1,2} {4} {6,7,9,10,11,12} {13,14,15} {16,17,18} {19,20} (decomp gm/gmcamera.c:1040-1102).
Each pass walks links in ascending order (OMB:1649-1713). Under GL_TRANS_MANUALSORT (src/nds/nds_platform.c:3430) submission order is blend order.
| proc (source) | link | per frame in the port | bucket; plausible residual |
|---|---|---|---|
| gmCameraDefaultProcDisplay (gmcamera.c:1040) + func_80017DBC/EC0 (OMB:1767-1828) + gcCaptureCameraGObj (OMB:1637) | cam | 6 passes over all links; RecordCapturedDisplay + OpeningRoom record per GObj | residual: gcCaptureCameraGObj 10,438; RecordCapturedDisplay 11,341; gcDrawAll 1,766; camera proc 976 (n0409) |
| (port hook, every GObj) RBM:14894-14975 | all | MarkDisplayProcHeads (RAS, 5,285), weapon/item/effect capture records (IsEffectDisplay 1,769), ClassifyGObj, IsEfBronto (1,132) ... | residual ~20K |
| efDisplayCLDProcDisplay / efDisplayXLUProcDisplay (ef/efdisplay.c:5-22) | 15, 18 | render-mode-only procs; the port folds DL-head writes back (RAS:1213-1308; n0409 988) | residual |
| wpDisplayDLHead1 / DObjDLLinks / DObjTreeDLLinks → wpDisplayMain (wp/wpdisplay.c:147-195); PKThunder (:197); YoshiEggThrow (import/battleship_yoshi_weapons.c:89) | 14 | Z-off modes, then RecordDObjDraw (RBM:15157) → SubmitWeaponDObj (:13439) → tree (RAS:12150) → SubmitStageDL candidate chain (RAS:6062-~11850) | MWPN |
| itDisplayOPA/XLU/ColAnimOPA/ColAnimXLU (port import/battleship_item_link_core.c:923-967; it/itdisplay.c:189-336); itKabigon* (battleship_item_kabigon.c:180,284); itPippi* (battleship_item_pippi.c:183,210; _spear.c:454) | 11 | ColAnim env to heads → SubmitItemDObj (RBM:13647; head capture RAS:1369) | MEFX (item half) |
| EFDesc procs: gcDrawDObjTree/TreeDLLinks/DLHead0/1, lbCommonDObjScaleX; efManager{ImpactWave :3281 (XLU-Z mode, prim alpha = ep->effect_vars.impact_wave.alpha), Shield :4106 (prim/env from typed player id, a=0xC0), YoshiShield :4148, PikachuThunderTrail :4488, NessPKThunderTrail :5012}ProcDisplay (ef/efmanager.c, included at import/battleship_efmanager.c:210) | 2, 10, 15, 18, 20 | render-only DL-head writes, then SubmitEffectDObj (RBM:13789): colour capture, tree, SubmitStageDL, ~50 owner candidates, material, matrices (RAS:9729-9813) | MEFX: ImpactWave 6,078; DamageSlash fill 11,937 + submit 1,913; PrepareInitialMatrices 5,736; GetFrameCameraMatrices 9,884 (shared) |
| efground actors, gcDrawDObjTreeForGObj (ef/efground.c:1406) | 4 | admitted via link 4 (RBM:13166-13174); EfLakitu/EfBronto native, others tree walk | MEFX |
| grcommon ground actors (TaruCann, clouds, acid, Arwing, gate, scales) | stage | ClassifyGObj-rejected branch (RBM:15213-15235) → ScanDObjs → SubmitStageDL | MEFX |
| efDisplayZPersp{CLD,XLU,AAXLU}ProcDisplay (ef/efdisplay.c:43-89; 4 GObjs :92-106) | 18/15/25/10 | lbParticleDrawTextures (import/battleship_lbparticle.c:4138-4901): camera per pass (:4198), FireGrind sim (:4184-4194), float affine per particle | MPTC: ~36K named (lbParticleDrawTextures 11,155; SetCurrentCamera 5,300; SubmitParticleQuad 5,637 ...) |
| ifCommon* procs (ifcommon.c) + lbCommonDrawSObjAttr (src/port/sprite_preview_backend.c:942-1055) | 25+ | **ndsIFCommonRecordHUDState on every SObj call (:990, :1003, :1054)**, outside the FG brackets (:780-805, :839-850, :913-935) | residual: RecordHUDState 9,613; PlayerDamageProcDisplay 3,469; GetBattleHudDamageState 1,471; PlayerTag 1,277 |
| (tick-HUD only) G3 effect census + packet capture (RAS:9951-9961, 11710-11716, 11816; PRE:931-1000, 1171-1175) | — | timer pairs per vertex/tri (NDS_EFFECT_PHASE_*), word hash per GX word | instrument: ndsEffectPacketRecord 3,569 + census BSS 9,216 B |
| ftShadowProcDisplay | — | no hit anywhere in src/ (shadows not drawn today) | 0 |
Flush: ≤128 tk/fr (docs/optimization/archive/P1_EXECUTION_BOARD_pre-cycle79.md:132-134). The end-of-frame DMA wait is <439 (INVESTIGATION_RENDER Q8).

### B2. Native per-frame draw list (NDL)
- **Record:** `NDSNdlRecord {GObj *gobj; u32 serial; u16 owner; u8 kind /*WEAPON,ITEM,EFFECT,GROUND,PARTICLE_PASS,STATE_ONLY*/; u8 flags;
  const NDSNdlProgram *body; u32 hdr[16];}` lives in a side table indexed by objman pool slot (pools from gcSetupObjman, decomp sys/objman.c:2242).
  The owner is bound once per GObj lifetime (the first dispatch runs today's candidate predicates once) and dropped at eject.
- **Dispatch:** in gcCaptureCameraGObj's inner loop (OMB:1657-1707), before RecordCapturedDisplay. A GObj with a record and the word armed
  runs `ndsNdlEmit(rec, pass, link)`, updates frame_draw_last, and skips capture, classification and proc_display. This keeps source order
  exactly, with no second registry to keep in sync, and removes classify + MarkDisplayProcHeads + proc + RecordDObjDraw + tree + candidate chain.
- **State-only procs:** the efDisplayCLD/XLU GObjs become STATE_ONLY records that set a per-link `mode` word (CLD: Z off + alpha threshold +
  blend a=8; XLU: Z on). Later records read it. This replaces the DL-head scan (RAS:1270-1308).
- **Owners:** each keeps its typed inputs: WPStruct, ITStruct colanim (it/itdisplay.c:237-304), MObj CURRENT_IMAGE/PRIM/LIGHT1/2
  (nds_native_damage_slash.exec.inc:34-38), effect world from the DObj, and the prim/env the shield proc writes (as a typed field).
- **Bodies:** shared, immutable GX words (source Z, or painter no-Z with z-col patches as in A2). Per instance, only a 10-20-word header is
  written (LOAD4x3 world·view, POLY_ATTR alpha/ID, TEXIMAGE/PLTT, COLOR). DMA header + body are chained on DMA0, so concurrent instances need
  no copies.
- **Particles:** simulation stays in SRC. FireGrind moves out of the draw seam. One DTCM camera context per frame (not per pass) and
  fixed-point view-space centres. ±size·(sx,sy) corners go under a once-per-pass projection plus scaled identity. One POLY_ATTR + TEXIMAGE + PLTT
  + BEGIN per (alpha, sheet, palette) run in submission order (no texture sort), POLY_ID 0, alpha 0 skipped (TFX:7014-7020). KO ENV palettes
  come from a prebuilt closure (docs/optimization/MISC.md:191-217). One DMA per pass buffer.
- **DamageSlash:** all 13 frames resident (10,752 B VRAM; 6,656 B with S-mirror once proved, MISC.md:146-172). Per frame it writes a
  TEXIMAGE word only. The texel fill (nds_native_damage_slash.exec.inc:51-114) and the mid-draw fenced glTexImage2D (:176-210) are deleted.
  That glTexImage2D is not render-retired (MISC.md:174-187), so this is also a correctness fix.
- **ImpactWave:** today it CPU-projects 18 vertices with s64 divides (NRC:1083-1390). It becomes a 16-triangle body with a resident texture
  per variant, and per instance: world·view LOAD4x3 + alpha/colour. It needs a Tier-2 check (moves from CPU projection to GX transform).
- **HUD:** call RecordHUDState once, after the traversal. It is idempotent: it recomputes from battle state (battleship_ifcommon.c:757-819),
  and the last of N calls equals one call at the end.

### B3. Counters that size the residual per owner (exact partition, T103 E3 method; tick-HUD, published as buckets beside nds_startup.h:4569-4572)
Keep MWPN/MEFX/MPTC (RBM:15201-15237, lbparticle.c:4172/4900). **Split MEFX** into MITM (:15209-15210), MEFX (:15211-15212) and MACT (:15213-15235).
Add these:
- MCAP: the per-GObj span in gcCaptureCameraGObj from before RecordCapturedDisplay to before proc_display (OMB:1673-1699), minus the STG
  delta inside it (snapshot gNdsTickHudStageTicks before and after).
- MPROC[kind]: the proc_display span (OMB:1700-1703) indexed by gobj->id (weapon/item/effect/ground/interface/particle/other), minus the nested
  MWPN/MITM/MEFX/MACT/MPTC/FG/FTR deltas.
- MCAM: DRAW minus (Σ per-GObj spans + stage prepare/finish + SubmitStageFighters + SObj end frame).
- MFLS = gNdsTickHudFlushTicks (published separately).
- MINS: A/B once with a new NDS_P2_EFFECT_CENSUS=0 that compiles out the G3 census and packet capture (it is gated only by NDS_TICK_HUD
  today, PRE:1171).
- Conservation: MISC − Σ ≤ 2% on every row, or the partition is wrong (the RBM:14869-14882 rule).
- Counts per frame: display GObjs visited (gNdsStageGCDrawAllLoopCapturedDisplayCount), per-kind submits, particle quads
  (gNdsParticleQuadEmitCount), DamageSlash updates (gNdsDamageSlashTextureUpdateCount), effect DLs (gNdsEffectDLSubmitCount).

### B4. Budget (ESTIMATE): traversal ~35K → ~8K (link walk + ~150/record); HUD ~18K → ~10K; DamageSlash −12K; particles ~36K → ~18K;
ImpactWave −4K/wave; effect/weapon/item submits → ~1-2K per instance. That is −60..−90K P50 on the measured roster. The tail (spawn bursts)
follows instance count. MISC ≤150K P50 is plausible, but the gate is set only after B3 has sized the rest.

## C. First vertical slices (same ROM A/B via a u32 word; harness `set variable` as scripts/capture-melonds.ps1:416-421; add -SetWord to
scripts/verify-p2-four-fighter-stress.ps1. The word must be u32, since a 1-byte poke is unsafe on melonDS. It must be explicitly in .data:
`__attribute__((section(".data"))) volatile u32 ...` (a zero initializer lands in .bss). It needs a reader, because --gc-sections drops
debugger-only globals (TSB:960-964))
- **S1 stage: Dream Land, all 8 segments.** `gNdsP2StageProg` (0 = today's path) and `gNdsP2StageProgSegMask = 0xFF` for bisection.
  Hooks: RBM:15553 (prepare), RAS:3940/3956 (commit), RBM:15598 (finish). Bring-up order: seg 5/7 (word-compare against the
  runtime Task36 capture, sNdsRendererTask36ReplayOwner.words with VIEW/NOZ_Z/BIND masked: a free oracle), then seg 0 + 4 (camera/static),
  then 1/2/3/6 (animated + cross). Engagement counters, compiled in both arms: frames, seg submits[8], DMA jobs/words, patch writes, animated/
  camera binds, straddle-CPU binds, declines[reason], FOV repatches, hidden rebuilds. Checks: GEN checker (A5.5); Task 49 differ owner 0
  (scripts/run-task49-gx-differ.ps1), which records at the software funnel (PRE:1177-1180) that DMA bypasses, so add a DMA-job recorder under
  NDS_TASK49_GX_DIFFER (Tier 1 bit-exact, Tier 2 ≤1 px, scripts/analyze-task49-gx-differ.ps1); matched-tic captures + crops of the backdrop,
  Whispy and flowers (scripts/compare-capture-pair.ps1); GXSTAT stack level flat (nds_platform.c:3403-3407); pause-camera orbit for FOV.
  Gate: STG P50 ≤40K, WORK-H falls by ≥ STG saving − 10%; OTHR/MISC flat.
- **M1 MISC: NDL skeleton + ImpactWave + DamageSlash (resident).** `gNdsP2Ndl = 0`. Hooks: OMB:1673 dispatch; efManager impact-wave latch
  (RAS:12091-12099) and DamageSlash candidate → record binding; delete the fill under the word. Counters: NdlDispatch[kind], NdlFallback[kind],
  procs skipped, DamageSlash uploads (must read 0 armed), B3 buckets. Checks: extend the Task 49 differ Owner range with EFFECT; Tier 1
  non-matrix exact; ImpactWave Tier 2; hit-heavy matched-tic capture pairs cropped to the changed geometry (compare-capture-pair.ps1
  -CropX/Y/W/H). Gate: MEFX and MISC fall, WORK-H follows.

## RISKS
1. GE/DMA: synchronous replay spins today (OWN:3176-3180). Async stage DMA moves the drain wait to the next GX writer's DMA_WAIT, which
   lands in MISC/FTR. Judge by WORK-H, not STG. Task 54's "GE-bound 720K" is unresolved (Phase 0 measures it).
2. Painter depth is a frame-global sequence (A2 note). A baked-only projection is wrong whenever weapons or effects draw no-Z before a segment.
   The foreground band has only 128 slots (PRE:1924-1927). A DL+4-weapon frame must be counted: gNdsPainterSlot* (TFX:13681-13730).
3. Eye-plane straddle: a sphere test that misses one drops a triangle whole. Keep the cold clipper, count straddles, run the entry pan per stage.
4. Classification: a binding wrongly derived STATIC freezes geometry (R2-02 E3/E4 precedent, AST:6492-6498). The derived-vs-observed check
   (the gNdsRendererTask36ObservedDynamicMask probe, RAS:3152-3183) must read 0 mismatches per stage over a whole match.
5. Stack: STORE slots for cross corners share the 31 levels with PUSH users and fighter slots. Stage slots are transient (stage runs before
   fighters) but must not collide with Whispy raw PUSH/POP or the particle pop (nds_platform.c:3387-3398).
6. Proc side effects: every skipped display proc must be render-only. Audit each owner. HUD and camera procs are not skipped.
7. VRAM: DamageSlash residency +9,216 B into an allocator documented as full (nds_native_damage_slash.exec.inc:176-181). It needs the
   pillar-A7 VRAM census.
8. Order: fighters already draw after gcDrawAll (RBM:15624). The NDL must not reorder translucent effects further. Differ per pass.
9. The differ's Owner range covers only STAGE/MARIO/FOX. The stress ROM carries a MISC instrument (B1 last row) that the shipping ROM
   lacks, so A/B arms must match on it.
10. The DL-only stress run cannot prove eight other stages. Every stage needs an entry-pan + orbit capture and a whole match with counters.

## ORDER OF WORK
1. Phase 0 prerequisites: B3 buckets + MINS A/B; GE-busy/DMA-stall; painter-slot census; confirm A8 with a gdb pause-orbit (rebuild count
   stays flat while the camera moves).
2. GEN: per-binding class derivation for all 9 VS stages + pins → checker (A5.1). Then program + patch-table emit + decode checker (A5.2-5).
3. S1 runtime (A6) behind gNdsP2StageProg: seg 5/7 oracle, then 0/4, then 1/2/3/6. Differ + captures, then stress A/B (STG, WORK-H).
4. Remaining VS stages one per build: YI (rigid-heavy), Castle, Jungle, Zebes (no no-Z), Sector (range-heavy), Hyrule, Saffron, MK (cross).
   Entry pan + orbit per stage.
5. Delete Task36 replay/capture, PrepareRun/preflight, persistent world cache, stage BSS (A7). Re-measure; the phase keeps no dual path.
6. M1 NDL + ImpactWave + DamageSlash → then weapons/items/efground/grcommon actors, the ~50 SubmitStageDL candidates migrated owner by owner
   → particle batcher → HUD once/frame → delete RecordCapturedDisplay classification, MarkDisplayProcHeads, tree walk, candidate chain.
7. Whole-phase gate: STG ≤40K on every stage; MISC per the B3 sizing; four-CPU stress A/B; 1P stages (pupupusmall, yostersmall, metal, zako,
   last, bonus*: generated incs exist) after VS closes.
