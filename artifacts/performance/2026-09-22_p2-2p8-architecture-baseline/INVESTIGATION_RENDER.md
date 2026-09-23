# Agent-render: compiled render pipeline - evidence report (Smash64DS)
Read-only; paths relative to repo root. MEASURED = repo artifact/profile (cited); ESTIMATE = my arithmetic. calls/fr = entry-PC counts / 129 regions (1 region
= 1 presented frame) in artifacts/performance/2026-09-16_p2-2p8-n0409-profile/arm9-profile.csv (DK/Samus/Link/Kirby, Dream Land); tk/fr = self time from artifacts/performance/2026-09-22_p2-2p8-architecture-baseline/attrib_n0409.json.
VERDICT: the design is an evolution of what exists (host GX FIFO templates + runtime-recorded packets replayed by GXFIFO DMA0). It can take FTR from
344K to ~70K and STG from 326K to ~20-40K (ESTIMATE), barely dents MISC (~200K of it is unexplained), and cannot deliver P99 <= 1,120,000: SRC alone is
1,670,808 in the P99+ band. Two premises need correcting: GX-side matrix work already lost at 4 fighters, and DMA "overlap" costs CPU bus time.

## Q1 Current fighter path per presented frame (FTR bucket = src/port/renderer_adapter_fighter.c:5195-5287)
present -> ndsFighterDisplayContractSubmitStageFighters (RAF:5308) -> ndsFighterDisplayContractSubmit (RAF:5139, 8.16 calls/fr; 2nd call per fighter exits at
:5186) -> ndsFighterMarioFoxDLAllDrawForSlot (:3227, 3.91 calls/fr); same hit path in docs/optimization/FTR.md:175-188. RAF/RAM = src/port/renderer_adapter_
{fighter,matrix}.c; NRC/TFX/OWN/AST/PRE = src/nds/nds_renderer_{native_common,textures_effects,native_owners,assets,preamble}.c.
| stage | where (costs = sums of MEASURED symbols) | tk/fr |
|---|---|---:|
| 1 display head + contract capture | source ftDisplayMainProcDisplay under capture hooks (RAF:537-591); memo skips the walk when unchanged (98.75%); the head is NOT read-only: offscreen arrow HUD, gLBCommonScale, fog statics, scene light (RAF:593-610) | ~42.6K |
| 2 draw plan / validate | plan hit (RAF:3393-3407) else CollectAllDObjsWithDL + DrawPlanResolve (:2444) + SelectRootProgram (:3755) + ValidateNativeOwnerCached (:3775); hit 6,217 / build 618 (2026-09-16_p2-2p8-packet-input-refresh/README.md) | ~9.4K |
| 3 matrix build, every joint | PrepareNativeOwnerMatrices (RAM:7326) -> ComposeOwnerWorldsFlat (:6416, 2.92/fr) or ...Source (DK seam repair :7469-7483); per joint BuildDObjLocalMatrix 77.5/fr -> BuildDObjXObjMatrix 77.7 -> TraRotRpyDirect20p12 44.8 / Exact 15.6 -> MtxCellS16p16 905/fr; MtxMulAffine20p12 73.2/fr at 347 tk | 112.6K (~1,100/joint) |
| 4 material identity, live-input refresh, texture-memo fence | RAF:4054, :1556/:4078, :4069, all inlined into DrawForSlot | 45.7K (DrawForSlot self) |
| 5 packet precheck | NRC:10161: BuildKey 7.7K, PatchTexgen 8.1K, TexturesResident 3.2K, rest 2.6K | ~21.6K |
| 6 replay patches | TryReplay (NRC:10229) 24.4K: tint :10319, projection :10325, per-root matrices :10331-10371, light :10372; StoreSplitModelview 54.8/fr x 220 tk (TFX:13126): every root takes the CPU split path | ~36.4K |
| 7 cache maintenance | DC_FlushRange of the packet (NRC:10387); armDCacheFlush 7.1K/fr over 29.4 calls, all owners | ~1-2K |
| 8 DMA | DMA0 GXFIFO, not waited (NRC:10393-10398), waited before glFlush (src/nds/nds_platform.c:3422); bus stall at TryReplay 0x02022830 avg 1,660 cyc | ~3.2K |
| 9 miss / record | material prep (RAF:4093-4140), production inputs (:4190), ExecuteNativeFighterOwnerProduction + recorder; 178 records / 6,673 hits (2026-09-15_p2-2p8-prechecked-replay/candidate-stress.json) = 2.6% of draws, ~10% of frames (FTR.md:154-169) | ~12K mean, +~75K tail (tail_symbols.txt) |
RENDER_FIGHTER 198.1K + fighter-owned RENDER_COMMON kernels 88.2K = ~286K of the 344,446 FTR P40-60 band; ~58K is memcpy/softfloat/flush/stall.
Per joint: 103 joints (JointSchedule DK 26 + Samus 24 + Link 30 + Kirby 23), 56 drawn bindings -> 3,344 tk/joint, 6,151/binding (ESTIMATE). Only ~1,100/joint
is matrix math; ~2,200/joint is plumbing (head ~410, plan ~90, orchestration ~440, precheck+patch ~560, stall ~560). The 347,132 "joint-proportional"
model included 138,714 of SRC-side ftMainPlayAnim (docs/archive/P2_CLOSED_ROWS.md:321-323): ~2,900/joint was never render-only. The frame touches 675
functions / 284 KB of code (baseline footprint.txt) vs an 8 KB icache, and renderer data stall is 254K tk/fr of scattered control-plane reads, measured
compulsory (2026-09-16_p2-2p8-stall-budget/RENDERER_STREAMING_SIZING.md). Deleting the walks, not reordering them, is the lever.

## Q2 Fighter special cases -> DS mechanism (31 enumerated; OK = precompiled list + small state)
SHIM=src/port/reloc_backend_compat_shims.c, FDM=decomp/BattleShip-main/decomp/src/ft/ftdisplaymain.c, GEN=scripts/fighters/generate_nds_native_owners.py,
INC=src/nds/nds_native_fighter_owner.generated.inc. Replay already rewrites only DIF_AMB/tint, texgen TEXCOORD, light vector, matrices (NRC:10229-10398).
- Texgen, regular (Link P2-3f31, Metal Mario, all N* owners): CPU LookAt x modelview -> per-vertex TEXCOORD patches (NRC:8355-8451, :9894-10019; Link 28
  unique verts/detail :9900). DS: normal-source TEXGEN + per-root texture matrix (~17 words/root/frame). OK; fidelity check (DS uses the object normal).
- Texgen LINEAR (N-Link/N-Captain/N-Ness, state 0x000c0000 INC:237936/260829/302979): rejected today (NRC:8474-8481, :8659). FLAG: not affine in the normal.
- Hurt/colanim flash: modulate (RAM:374-390), light-word blend (TFX:1760-1791), replay re-derive (NRC:9763-9850); 222 of 350 re-records were flash
  (PRE:3338-3345). DS: DIF_AMB + SPE_EMI words, exact only for untextured white-prim runs; the source blends via fog colour (FDM:600-663) and DS fog is
  frame-global. PARTIAL FLAG (today's approximation, not a regression).
- Env colour / translucency (colanim color2, Master Hand fade): non-white env rejected (NRC:8681-8684). Needs POLYGON_ATTR alpha/ID patch sites per run.
- Light direction (decomp ft/ftdisplaylights.c:10-27, FDM:1168-1175; 0 changes in 22,296 epochs, NRC:3901-3904): LIGHT_VECTOR per fighter under an identity
  vector matrix (NRC:3833-3836). Light colours/material fold (GEN:6146-6184) -> DIF_AMB per epoch. Tint tiles (TFX:3734-3875) -> per-costume texture words.
  Unlit uniform roots baked lit/ambient (GEN:7482-7569); unlit multi-colour runs -> COLOR words (GEN:6474-6515). All OK.
- Hidden parts / is_invisible / Entry: the source walk picks lists (FDM:765, :861, :1087-1092); Entry flips visibility inside one status (RAF:998-1016).
  Skip part sub-lists; invisible = no DMA. OK except cross-vertex dependencies (next).
- Model-part programs (Samus Catch/FSmash/Morph, Link Entry/SpecialN/Catch/Claps, Yoshi Catch/Throw, Ness Win3, Kirby Stone/CopyLink/CopyTransition, hand
  variants AST:5414-5712; plan constant 3,961 same / 0 variant RAF:3398-3400): one list per program. Cross-joint vertices and Kirby head->body dependency
  (NRC:9490-9540, RAF:1199-1271, 12 heads): MTX_RESTORE inside strips + one body list per head. OK.
- Texture animation (blink, damage face, matanim: 9 animatable words RAM:410-437, SHIM:2685-2774): TEXIMAGE/PLTT words from a prebuilt table, every frame
  VRAM-resident. Detail hi/lo (RAF:3354-3357; pause/KO switch too): choose list. Kirby hats (AST:3579-3894, NitroFS kirby_hat_03..13): list per
  (hat, detail). Fox gun overlay (RAF:1072-1197, TFX:8653-8758), Link boomerang donor (RAF:2405-2439), electric skeleton (Mario/Fox, 2 of 5 frames,
  SHIM:674-726), held items, animlocks + hitlag shuffle (RAM:6708-6733, :7426-7453), respawn halo (NRC:2166-2930), Metal/Giant/Polygon/Master Hand
  (RAF:2689-2885; Giant DK = root scale), costume (SHIM:1871-1959): all OK as list selection + matrices + words.
- Shadows: stubbed, ftShadowMakeShadow returns NULL (SHIM:12755-12759). FLAG if restored: floor-fitted per-frame geometry (decomp ft/ftshadow.c:55-429).
- Afterimage trails (FDM:397-597): not drawn today (RAF:656-659, :957-961). FLAG: variable per-frame strip, CPU-written.
- Captain HIGH alpha test (NRC:10192-10200, :10250-10268): alpha-test enable/ref are registers, not FIFO commands, and frame-global. FLAG (4P uses LOW).
- UNKNOWN: fighter drawn inside the magnifier bubble (none found, FDM:1093-1160), Pikachu/Jigglypuff accessories (FDM:724-822), lfrac texture blends.
Hazard: packed 1P builds halt on any native decline (RAF:4405-4460): coverage must be total, not fail-closed-to-generic.

## Q3 Joints, bindings, multi-matrix parts
From INC JointSchedule/BindingJoints/CrossPaletteSlots (5-bit parent|binding|GX-slot fields, INC:494-498), joints/bindings/cross GX slots: Mario 25/14/8,
Fox 27/18/2, Luigi 25/14/8, DK 26/16/10, Captain 26/17/0, Samus 24/14/0 (Catch 21, FSmash 16), Link 30/19/6 (Entry 19, SpecialN 20, Catch 22, Claps 19),
Pikachu 27/16/11, Yoshi 27/18/14 (Catch/Throw 19), Ness 27/14/0, Purin 23/7/4, Kirby 23/7/4 (CopyLink 8). Caps: 37 (decomp ft/ftdef.h:10), roots 32 (PRE:3291).
- Nothing exceeds 30 when the CPU composes: the stack holds per-BINDING matrices (max 22, Link Catch). Joint count bites only when GX composes: Slice 43
  reserved the union of every owner's cross slots (16..25) and allocated parents downward from 30 ("Donkey alone needs ten", RAM:6168-6185, :6229-6305).
  Per-draw slot maps remove that, since the GE consumes fighters sequentially.
- Multi-matrix parts: 9 of 12 fighters have cross runs (N64 vertex-cache sharing across DObjs; rigid per vertex, no weights). Today: MATRIX_STORE at
  root start (src/nds/nds_renderer_native_fighter_production.c:292), per-corner MATRIX_RESTORE when the corner's binding differs, restore back
  (NRC:9483-9540). Mario template: 36 cross of 320 tris, 8 stores, 70 restores (INC:13183-13205). Static in a compiled list; only stored matrices change.

## Q4 Stage
Classes (enum PRE:2619-2630; commit loop OWN:5221-5405):
- no-Z (class 3, source layers with Z off): the DS cannot disable depth per polygon (TFX:13707-13710), so each triangle gets a painter depth pz counting
  down from 0x1000 by 1/4096 (TFX:13669-13679) and the projection z column becomes w*pz/4096 (ndsRendererNativeStageSetNoZColumn OWN:3296-3308).
  Rigid binding: PUSH+MULT4x4 world under the live camera (OWN:2649-2707), then PER TRIANGLE MTX_MODE+LOAD4x4 projection (OWN:2783-2803) + 3 x (COLOR,
  TEXCOORD, VTX16); GX clips. Non-rigid: PrepareRun CPU-transforms every dense vertex for near_inside (OWN:1908-1948), CPU near clip with a LOAD4x4 per
  clipped vertex (OWN:3506-3578), else a CPU-built LoadNoZMatrix per triangle (OWN:3314-3347).
- cross-matrix (flag bit 0): CPU transform + clip per corner, one LOAD4x4 per vertex (OWN:3405-3504). raw (class 0): one CPU matrix per run, model-space
  VTX16, source z, GX clip (OWN:2910-2920). range (class 6): shifted coordinates + identity projection (OWN:2921-2951).
- replay (NDS_TASK36_HW_COMPOSE 2): runtime-captured words, GXFIFO DMA0 (OWN:3112-3240); Dream Land segments 5/7 only, rigid bindings only
  (AST:6483-6516); projection baked, declined on FOV change (AST:6567-6588); max 2,487 words (src/nds/nds_native_stage_owner.generated.inc:20).
Dream Land (stage generated.inc:4-19): 202 tris, 54 runs, 312 dense verts, 42 bindings; raw 66, no-Z 126, range 10, cross 5 runs/10 tris. seg0 layer0 54 no-Z
rigid, live CPU emit, 54 projection loads; seg1/2 Whispy eyes/mouth 12 no-Z non-rigid; seg3/6 flowers 15 no-Z incl. 10 cross; seg4 layer1 66 raw + 10
range (Z-buffered); seg5/7 45 no-Z replayed. Per frame 157 tris live + 45 replayed, all independent triangles. MEASURED (older builds): actor segments
43,998 tk/fr for ~21-27 tris (OWN:1317-1319); no-Z 98,816 tk for 126 tris vs raw 9,088 for 66 (docs/PERF_LEDGER.md:4181-4193).
Why CPU emission/clipping: early matrix-in-glBegin path replaced by pre-divided CPU vertices (docs/HW_RENDERER_VISIBILITY_FINDINGS.md:29-31); clip-space
Z for no-Z clipped the scene out (:58-61); dropping near-crossing triangles opened holes (TFX:14414-14419); w~0 after >>8 builds degenerate matrices
(OWN:3550-3557); stated design: rigid bindings use the GX clip, dynamic ones keep the exact CPU clip (OWN:1860-1868, :1899-1905); replaying
camera-dependent actors froze them (AST:6486-6498).
Hardware clipping with z = c*w (my algebra): near z >= -w <=> (1+c)w >= 0 <=> w >= 0; far likewise. Both planes collapse onto the eye plane; the source
near plane (256 units, decomp gm/gmcamera.c:16) vanishes. 0<w<near corners draw (harmless); triangles crossing w=0 drop whole (POLYGON_ATTR bit 12 never set,
TFX:1279-1304). z = K or z = a*w + b restores a near plane but moves depth by far more than one painter step. Verdict: HW clip is viable for no-Z geometry
that never crosses the eye plane - every layer0 backdrop (below) - and Dream Land's 99 rigid no-Z tris already depend on it. For a standard perspective P
the no-Z P_i differs from P only in its z column, (0,0,-c_i,0) (my derivation): baked per-painter-slot LOAD4x4s need repatching only on FOV change.
Camera-surrounding: none. Every VS sky is already a 2D wallpaper (decomp gr/grwallpaper.c:267-301); the port's BG2 owner took 8 VS backgrounds, Dream
Land's backdrop is DObj geometry (docs/p2/BUG_NOTES.md:4472-4475). layer0 z ranges, eye at +z (ESTIMATE, descriptor tables x baked worlds): DL -2767..-198,
Castle -1095..-346, Sector Z -16109..-296, Jungle -5207..343, Hyrule -2274..-253, YI -1797..360, Saffron -4287..-359, MK -6630..600. Big Z-buffered
geometry (Zebes acid plane, Sector Z Great Fox 187 range tris) keeps source z and GX near-clips. MEASURED near activity: YI 4 CPU-clipped no-Z tris per
present, Castle/MK 0 (BUG_NOTES.md:3091-3094); Castle roof crosses near during the entry pan (OWN:3555-3557). "Backdrop -> BG layer" only changes Dream
Land and flattens its 54-tri parallax card set: low value.
Dynamic inputs: DL Whispy AnimJoint + MatAnim swaps (decomp gr/grcommon/grpupupu.c:74-111), flower wind (:407-560), 16 dynamic bindings recomposed per frame
(src/port/renderer_adapter_stage.c:3187-3258), water MatAnim frozen (src/import/battleship_grpupupu_ground.c:238-334), Bronto; Castle bumper palette
flash, Lakitu, sliding platform (non-rigid); Sector Z Arwing + lasers (rigid mask 0); Jungle barrel, platforms 17-18, waterfall UNKNOWN; Zebes acid DObj
translation + 19 material events; Hyrule tornado (particles, rigid mask 0); YI clouds (live prim alpha); Saffron gate + Pokemon owners; MK scale strings
(cross), POW, Piranha. Non-rigid (CPU) no-Z tris: DL 27, Castle 13, YI 18, Sector Z 31, Hyrule 128, Saffron 134, MK 64 (sub-agent count).
Possible bug (inferred, unmeasured): near_inside is camera-dependent (OWN:1308-1311) but PrepareRun is skipped when the R2 reuse key matches, and that key
has no camera input (OWN:4263-4285, :4706-4712).
Compiled stage (ESTIMATE): GE ~15K cyc/frame for Dream Land (99 x 69 + 26 x 88 + 17 x 67 + 10 x 135 + 76 x 33 + 0.7K); CPU = ~42 binding modelview patches
(~150 tk) + 16 dynamic binding matrices (~450) + matanim words + ~8 DMA jobs = ~15-25K, plus whatever non-rigid no-Z stays CPU-emitted (DL 27 tris, but
134 on Saffron) -> ~20-40K vs 326K, IF the 326K is executor work (OWN:3046-3048 calls ~331,300 tk/fr of it unattributed; Task 53 saw removed stage prep
reappear as OTHR - see risk 7). STG.md (2026-09-17) ranks: compact live executor, generated run schedule, patchable GX chunks, dirty inputs, endpoint
deformation for mixed-binding strings; painter grouping demoted (STG.md:284-376); Task 55 word elision pulsed colours (STG.md:154-160).

## Q5 MISC
MISC = max(DRAW - (FTR+STG+BG+FG), 0) + FLUSH (src/port/taskman_seam_battle_host.c:949-956; DRAW :736-746; FLUSH nds_platform.c:3417-3428): a residual
holding whole-GObj traversal (src/port/opening_movie_backend.c:1638), per-display classification (src/port/reloc_backend_movement.c:14894-14975; only the
commit :14980-14988 is STG), weapon/item/effect submits (:15190-15241), the particle pass (src/import/battleship_lbparticle.c:4138-4901, incl. FireGrind
sim :4176-4194), DamageSlash texel fill + glTexImage2D mid-draw (src/nds/nds_native_damage_slash.exec.inc:51-114, :176-210). MEASURED MISC min 96,128,
P50 267,328, P99 606,720 (2026-09-17_p2-2p8-dtcm-hot-scalars/fourcpu-rows.csv). Only fighters and Task36 stage runs use a DMA'd list today.
Paths: effects SubmitEffectDObj (reloc_backend_movement.c:13789) -> SubmitEffectDObjTree (renderer_adapter_stage.c:12074) -> SubmitStageDL (:6062):
file find, material snapshot (:9729-9790), CPU projection+modelview per object (:9802-9813), ~50 owner checks (:9965-11497); ImpactWave CPU-projects 18
verts with s64 divides, 48 immediate corners (NRC:1083-1390). Particles: camera per pass (battleship_lbparticle.c:4198), per particle float affine + 2
sqrtf (:3646-3735), SubmitParticleQuad waits DMA0 + 10 float->fixed (TFX:6918-6968); (alpha, texture) runs already batched (TFX:7801-7888). Weapons
(reloc_backend_movement.c:13439) and items (:13647-13723) -> owners with a split matrix per draw (src/nds/nds_native_item_wave1_emit.exec.inc:75-98).
Costs (MEASURED tk/fr): DamageSlashTextureFill 11,937; RecordCapturedDisplay 11,341; lbParticleDrawTextures 11,155; gcCaptureCameraGObj 10,438; ImpactWave
6,078; SubmitParticleQuad 5,637; ParticleSetCurrentCamera 5,300 -> ~85K named + 33.1K shared helpers; ~200K of mean MISC matches no top-400 symbol
(UNKNOWN; gNdsMisc{Weapon,Effect,Particle,TexUpload}DrawTicks exist but are not sampled into the rows).
Batching design: one DTCM camera context per frame; billboards in view space (projection + scaled identity once per batch; CPU adds +-size*sx/sy to the
centre); precomputed TEXIMAGE/PLTT words per (sheet, palette) incl. ENV variants (docs/optimization/MISC.md M7); one POLY_ATTR+BEGIN per (alpha, sheet,
palette) run, NO texture sort (GL_TRANS_MANUALSORT: submission order is blend order, nds_platform.c:3426), POLY_ID(0) for particles, skip alpha 0
(renders wireframe, TFX:7014-7020); precompiled lists for Z-buffered models (ImpactWave, DamageSlash, items) with per-instance patches on DMA0; painter
no-Z models keep CPU vertices or the per-list constant-depth column (TFX:6855-6900) - verbatim packets were refuted (docs/archive/P1_EXECUTION_BOARD.md:
3613-3660); DamageSlash textures resident first (MISC.md M3a/M3b). Saving ESTIMATE 36-48K (12-16% of P40-60 MISC). Traversal/classification (~24K) needs
a native per-frame draw list instead of gcDrawAll display procs; the ~200K residual must be sized before any MISC promise.

## Q6 Sizes
Existing (MEASURED): (a) host GX templates kept under `#if 0` (INC:7938-7944; GEN:9165-9245): Mario 4,034 words = 16,136 B for 320 tris/14 roots, Fox
3,936 = 15,744 B. My decode of Mario: VTX16 960 (independent tris), COLOR 960 (pre-HW-light), TEXCOORD 192, LOAD4x4 15, STORE 8, RESTORE 70, 18 epochs -
stale vs the shipped path (builds/build-p2-fourcpu-tickhud/nds_build_config.h: NDS_R2_FIGHTER_HW_LIGHT 1, NDS_TASK56_FIGHTER_PRIMITIVES 2). (b) recorded
low-detail packets (2026-09-15_p2-2p8-packet-footprint.txt PKSLOT): DK 2,710 words (10,840 B, 16 roots), Samus 2,088 (8,352 B, 14), Link 2,890 (11,560 B,
19); arena 141,440 B borrowed from gSYFramebufferSets (PRE:3293-3296). (c) owner images (NitroFS): DK low/high 15,232/21,340 B, Samus 14,544/19,316, Link
15,892/19,744, Kirby 24,268/30,160 + 11 hats 3.5-8.5 KB (builds/build-p2-fourcpu-tickhud/nitrofs/fighters/*.bin).
ESTIMATE compiled fighter (all programs): source tris (INC headers: DK low 314, Samus 391, Link 226, Kirby 491) x 8-12 words x 4 B = ~11-24 KB low, x1.3-1.6
high - about today's owner image, which it replaces. PLTT/TEXIMAGE words differ per costume and VRAM slot -> per-instance copies, 4 x 13-20 KB = 52-80 KB,
the same order as the 4 x 35,360 B packet regions retired. Dream Land ~7.5K words (~30 KB) incl. per-triangle projection loads (54x42+27x60+76x15+2,487).
RAM retired (MEASURED nm -S of builds/build-p2-fourcpu-tickhud/smash64ds-p2-fourcpu-tickhud-hwtri.elf; sums ESTIMATE):
- fighter ~98 KB BSS: ...NativeOwnerMaterials 19,200; sNdsFighterPackets 14,192; ...NativeOwnerWorkspace 12,432; sNdsNativeFighterOwnerExecution 8,824;
  sNdsR2RunTextureMemo 7,504; sNdsNativeFighterRunUvInputs 7,504; persistent_renderer_vertices.22 5,780; sNdsFtrDrawMemo 5,408; sNdsFighterDisplayContract
  4,704 + 768; sNdsFighterDrawPlan 3,776 + 916; ...ValidationCache 3,408; ...MaterialKeys 2,304; TextureCurr/Next 1,536.
- DTCM: Mario-canonical sNdsNativeFighterPreparedDense 6,130 + DenseNormals 2,452 at 0x02ff0000-0x02ff2185 = 8,582 of the 10,828 B DTCM image, though
  this roster never draws Mario (verify no other consumer). DTCM paid -43,200 WORK-H for 508 B of hot scalars (docs/optimization/STG.md:26).
- stage ~70 KB: sNdsRendererTask36ReplayOwner 24,256; ...NativeStageWorkspace 15,816; sNdsNativeStageOwnerExecution 7,360; ...StagePreparedDense 6,904;
  ...StageVertexCache 5,780; ...StageTextureSites 5,632; ~4.1K caches. Effects: RebirthHaloPackets 6,888, WhispyPacket 4,128, EntryEffect 8,192. If all
  textures are pre-converted: TextureScratch 32,768 + RefreshLarge 16,384 + RefreshSmall 4,096 + KeyPool 18,644.
- code: executors to delete ~180 KB .text (NRC 60.5K, adapter_stage 39.2K, OWN 35.6K, RAM 22.6K, RAF 14.7K, fighter_production 7.8K; nm -l sums).

## Q7 Generators to reuse; runtime to delete
Reuse: GEN already emits roots/epochs/runs, 12.4-scaled VERTEX16 with seam guards (GEN:4750-4780), N64 state deltas -> DS direct policies (INC:488-508),
Task 56 strips, joint schedule + GX slots, cross corners and the FIFO template + patch tables (GEN:9165-9245; checker scripts/fighters/check_nds_native_
owner_packet.py): promote that fixture to product (per owner x detail x program sub-lists with NORMAL, strips, TEXIMAGE/PLTT and LOAD4x3 patch maps).
Container scripts/fighters/generate_nds_native_owner_images.py (NitroFS struct image, offsetof ABI, :1-45); topology native_skeletons.py, derive_native_owner_
tables.py; stage/objects scripts/stages/generate_nds_native_stage.py (+emit_native_stage_runtime_rows.py, dreamland/) and generate_nds_native_*.py; particles
scripts/generate_nds_particle_banks.py; textures scripts/generate_battle_playable_static_textures.py (66,690 B pack) -> all battle textures. Oracles:
check_native_owner_geometry_closure.py, check_nds_native_owner_hierarchy.py, check_hidden_part_root_coverage.py, check_model_part_mutation_coverage.py,
Task 49 GX differ (scripts/run-task49-gx-differ.ps1); the runtime recorder is a free word-level oracle. Delete/shrink (lines/.text/data): NRC 13,152/60.5K/
26.9K (fighter half); nds_renderer_native_fighter_production.c 1,370/7.8K; RAF 5,380/14.7K/21.7K -> head call + program read + job build; RAM 8,726/22.6K/
59.1K -> one ITCM kernel; PRE recorder; adapter_stage 12,294/39.2K + OWN 5,595/35.6K + Task36 replay -> stage submitter; TFX 15,357/85.2K -> effect batcher.

## Q8 CPU cost model and GX risks
Baseline (MEASURED P40-60 means, fourcpu-rows.csv): WORK-H 1,584,246 = SRC 578,533 + FTR 344,446 + STG 325,803 + MISC 295,918 + AUD 12,355 + other 27,191
(OTHR 128,955 - WAIT 101,764; my check). Render = 966,167 (61%).
Compiled (ESTIMATE): heads kept ~12K (ProcDisplay 6.5K + fog/light helpers ~5.5K MEASURED); 103 joints x ~450 = ~46K (existing ITCM fixed TRS kernel
Direct20p12 278 tk/call MEASURED, f32 inputs RAM:2343-2350, + ARM-mode SMULL 4x3 compose ~130 vs 347 today + scale/store ~40); 56 LOAD4x3 patches +
STOREs only for cross-referenced bindings ~2-3K; tint/texgen/program words ~4-8K; ~20-25 IRQ-chained DMA jobs ~3-5K => FIGHTERS ~70K (50-80K).
STAGE ~20-40K (Q4). MISC ~230-250K (Q5). Render ~0.30-0.37M + 618K non-render -> WORK-H P50 ~0.92-0.99M: under 1.12M with ~130-200K margin.
Not at P99: SRC is 1,670,808 in the P99+ band and 1,126,525 in P95-99 (MEASURED bands_dtcm.txt) - over the gate with zero render (SHDT/SPRM spikes).
GE budget (ESTIMATE, docs/N64_vs_Nintendo_DS_Hardware_Reference.md:458-473): fighters ~9-10K words/fr today, ~1,800 lit textured verts x ~20 cyc + ~8K
matrix/restore cyc = ~45K cyc; Dream Land ~15K; effects ~10-20K -> ~70-90K of 1,120K GE cycles per 30 FPS frame. Observed backpressure agrees: explicit
DMA-busy stalls ~6.7K tk/fr (Task36ReplayRun 0x01ff9b54 avg 1,022 cyc; TryReplay 0x02022830 avg 1,660) and the end-of-frame DMA wait <439 tk/fr. That
contradicts the July Task 53/54 claim that the stage is GX-throughput-bound (2,996 words ~ 720K, artifacts/performance/2026-07-24_task54-stage-dma-e0.md:
80-100 = ~240 cyc/word, 10x published costs): unresolved. A post-swap GE halt is unlikely: the loop waits >=1 VBlank after glFlush (nds_platform.c:3328-3340).
DMA is not free CPU time: GXFIFO DMA refills 112-word bursts while the GE keeps pace, and ARM9 main-RAM/I-O accesses wait (the 1,022-1,660-cycle single-load
stalls above). ~10-12K words/fr already go by DMA0 today (packets 7.7K for 3 fighters + Kirby ~2K + replay <=2,487). Overlapping with the next tick would move that
contention into SRC, which runs from main RAM (ITCM has 104 B free, RENDERER_STREAMING_SIZING.md). With GE load at ~10%, the overlap buys little.
GX-compose precedent: engaging the matrix-stack compose cost +22,848 P50 / +67,456 P95 at 4 fighters because STORE/RESTORE/MULT FIFO traffic outweighed the
CPU multiply (artifacts/performance/2026-09-16_p2-2p8-gx-compose-decline/README.md:3, :64-82). Do not move compose to GX; cut words instead (per-binding
LOAD4x3 ~13 words vs today's projection + modelview 4x4 pair ~36; drop ~55 per-root projection reloads).
Stack: 31 levels shared with PUSH users (Whispy raw PUSH/POP, R2WriteLightVector, conditional particle pop: nds_platform.c:3387-3395, RAM:6171-6174). Reserve
0-7 for pushes and STORE only cross-referenced bindings (<=14, Yoshi); storing every binding needs 22 (Link Catch) of 23. Gate: GXSTAT level flat, bit 15
clear (the 2026-08 leak blinked fighters every 32 frames, docs/archive/P1_CLOSURE_PLAN.md:1100).
4x3 trap: modelview row 3 carries the 2^-8 world-unit shift including m33 (TFX:13123-13144; shift 8 at PRE:1912; FTR.md:427-431). Fix: load P' = P with row
3 scaled 2^-8 once per owner and 4x3 [R; T*2^-8]: x/w, y/w, z/w unchanged (my derivation); needs the differ + a 20.12 precision check of P row 3.
Polygon RAM: 2-fighter frames accept P50 465 / max 510 (2026-08-15_gxstack-io-draw/gxstat-c183-rows.csv); DK 177, Samus 169 tris/fr (dtcm-hot-scalars fourcpu.json totals/1,971) -> 4P ~850-1,100 of 2,048 (ESTIMATE).
MEASURE FIRST: (1) GE busy per owner and per frame (GXSTAT bit 27 + FIFO level at owner boundaries); (2) CPU stall caused by DMA bursts (same frame, DMA vs
CPU words); (3) the ~200K MISC residual (split counters); (4) the lean joint kernel on target incl. ITCM room; (5) poly/vertex RAM at 4P + effects; (6) fidelity
of the projection re-derivation and hardware texgen (Task 49 differ).

## TOP RISKS / WHERE THE DESIGN BREAKS
1. P99 is not a render problem: SRC alone exceeds the gate in the P95-99 and P99+ bands. The design can bring P50 under the gate (ESTIMATE ~0.92-0.99M).
2. MISC barely moves: ~200K of 296K is unexplained; batching removes 36-48K. Without a native per-frame draw list replacing gcDrawAll display procs,
   MISC stays the largest render lane after the rewrite and eats the margin.
3. "Per frame only matrices" is optimistic: the display head must still run (side effects, RAF:604-608); programs, hidden flags and texture-animation words
   must still be read per frame (Entry flips inside a status); pose output is f32, so every joint pays float->fixed unless the pose producer changes (an
   earlier producer-side pilot measured FTR +14,336, FTR.md:144-152).
4. No GX call/jump: part skips, programs and per-instance patches mean many DMA jobs (IRQ chain) or per-instance copies; cross-vertex dependencies (Kirby
   head->body, cross runs) forbid naive part skipping, so lists must be per program (the generator already enumerates programs).
5. Matrix traffic: any scheme adding mid-stream STORE/RESTORE/MULT per binding repeats the measured 4-fighter loss. LOAD4x3 patch-in-place + STORE only
   where cross corners need it; raw PUSH/POP writers share the 31 levels with those absolute slots (stack-balance gate).
6. DMA overlap into SRC trades GE idle time (plentiful) for CPU bus stalls (scarce; the frame is 64% memory stall). Don't overlap until (1)/(2) are measured.
7. GE throughput unresolved: the July GX-bound claim and today's backpressure data disagree 10x; measure before building on either.
8. No-Z stage: one projection load per painter slot stays; baked painter order breaks if run order or visibility changes (OWN:5235-5238); w<0 triangles drop
   whole; Hyrule/Saffron/MK have 64-134 non-rigid no-Z tris each that need per-frame matrices and cross corners, not a static list.
9. Items that are not "a few words": linear texgen (N* owners), flash on textured runs, Captain HIGH alpha test registers, shadows/afterimage if restored,
   the 4x3 trap.
10. Packed 1P builds halt on any decline (RAF:4405-4460): incomplete program coverage becomes a crash, not a fallback.
11. Memory: ITCM 104 B free for the joint kernel; per-instance lists 52-80 KB; arena headroom (+204 B of texels cost a 4,096 B arena page, FTR.md:17).

## RECOMMENDED FIRST SLICE (one fighter, end to end)
Owner: Samus LOW, default program (14 bindings, 0 cross slots, no texgen, one recorded packet 2,088 words): no RESTORE, no texgen, no hats. DK (cross slots,
seam repair), Link (texgen, 4 programs) and Kirby (hats) follow in that order.
0. Instrument first, same ROM: GE busy per owner (GXSTAT bit 27 + FIFO level sampled at owner boundaries); DMA0 wait counters; gNdsMisc* split counters into
   the tick-HUD rows. These decide items 1-2 of MEASURE FIRST before any list ships.
1. Host: GEN emits Samus LOW default-program list (NORMAL + Task 56 strips, DIF_AMB per epoch, TEXIMAGE/PLTT patch sites, 14 LOAD4x3 patch sites under P')
   into the Samus owner image. Host oracle: word-compare against a runtime-recorded packet for the same inputs, matrix words masked.
2. Runtime, behind a lab flag in DrawForSlot: Samus + plan hit + default program skips stages 2-9 - run the head, build 24 joint matrices with an ARM ITCM
   compose around the existing Direct20p12 TRS builder, write 14 LOAD4x3 + tint words, flush, DMA0. Anything else takes today's path. Engagement and
   decline counters compiled in both arms.
3. Gates: Task 49 GX differ / capture equality within the fidelity doctrine; GXSTAT stack flat; native failures 0; four-CPU stress A/B on one build
   directory, flag on/off: FTR P50/P95 plus OTHR and MISC (to catch relocated stall). Target (ESTIMATE): Samus's ~86K FTR share falls to <= ~20K
   (FTR P50 -60K or better) with WORK-H falling by at least 45K.
4. Stop rule: if FTR falls but WORK-H does not (stall relocated), or GE busy exceeds ~50% of the frame, stop and re-plan before scaling to the roster.
