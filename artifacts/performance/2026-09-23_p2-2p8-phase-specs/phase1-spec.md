# Phase 1 (A1 Fighters) - Lean Fighter Path implementation spec
Read-only investigation, 2026-09-22. Paths relative to repo root. RAF/RAM = src/port/renderer_adapter_{fighter,matrix}.c; RAS = src/port/renderer_adapter_stage.c;
NRC/TFX/PRE/AST = src/nds/nds_renderer_{native_common,textures_effects,preamble,assets}.c; FPR = src/nds/nds_renderer_native_fighter_production.c;
INC = src/nds/nds_native_fighter_owner.generated.inc; GEN = scripts/fighters/generate_nds_native_owners.py; FDM = decomp/BattleShip-main/decomp/src/ft/ftdisplaymain.c;
SHIM = src/port/reloc_backend_compat_shims.c. MEASURED self tk/fr = artifacts/performance/2026-09-22_p2-2p8-architecture-baseline/attrib_n0409.json (4 CPUs, 129 regions);
MEASURED packet stats = artifacts/performance/2026-09-15_p2-2p8-prechecked-replay/candidate-stress.json "extras"; P0 = artifacts/performance/2026-09-23_p2-2p8-phase0-baseline/README.md
(Phase 0 baseline, nocam rows). Everything else labelled ESTIMATE is my arithmetic.

## 0. Verdict
- A fighter packet is today ONE recorded stream per battle slot (sNdsFighterPackets[4], PRE:3477) keyed by 6 hash words (NRC:9618-9672); any key move re-records through
  production (NRC:10456-10537). Of the 34 inputs in section 1, every per-frame one (rows 9, 18, 19, 22, 28-33) is a patchable word or a visibility test; everything that
  changes list STRUCTURE (rows 5-8, 10-14, 23) is an event the source already signals (status generation RAF:712-729) or a flag the kernel reads anyway (row 8).
- Lean path = head (kept) -> 4-field tuple compare -> ARM ITCM joint kernel (Q43.20 from 16.16 locals) -> patch LOAD4x3/P'/tint/light/texture words -> flush -> 1 DMA0.
  ESTIMATE FTR P50 ~65-80K (head is the uncertain term, see R1), P99 ~90K; the tail term (+~75K/record, +441K P99 band) becomes event-time materialization (~5-7K/event).
- Misses: recommend HOST-GENERATED lists (section 3); the runtime recorder survives only as the lab oracle until the last admitted kind is word-proven, then is deleted (D2).
- Phase 0 facts folded in: the P95 owner is a tint episode (P0:42-61) caused by r49's per-packet rule - a tinted packet re-records whenever its prim changes
  (NRC:9789-9799, row 19) - so a fighter with a flashing colour re-runs production and re-resolves all its textures every frame (the tile set itself is stable, row 24)
  -> lean tint = every tile the fighter can show resident + a TEXIMAGE/PLTT word patch + the shade re-derive, never a re-record (2.6); the match-start native failures
  (Link AppearL, use_texture FALSE, P0:62-67) -> every fighter texture is admitted and pinned at fighter creation, before GO (2.8); polygon RAM is not binding
  (P50 559 / max 675 of 2,048, P0:35-37) -> no LOD or list-size constraint from it.
- First slice (section 6): Samus LOW canonical on an ADOPTED recorded packet, new kernel + tuple fast path, same-ROM .data route with an exact word oracle vs TryReplay.

## 1. Every input that changes a packet's words or validity (a = constant per match, b = event, c = per frame)
| # | input -> today's consumer | source of the value | cls | lean handling |
|---|---|---|---|---|
| 1 | owner/kind -> key[1] bits 0-7 (RAF:4060-4065), plan file identity (RAF:2353-2356) | fp->fkind via ndsFighterGetNativeOwnerSlot RAF:2689 | a | instance constant |
| 2 | battle slot -> key[1] bits 9-10, arena region (NRC:10240, 10489-10495) | fp->nds_slot | a | instance index |
| 3 | costume, shade -> key[1] bits 11-26 (RAF:4064-4065) | fp->costume/shade set by ftParamInitAllParts (SHIM:1847-1962) | a | PLTT/TEXIMAGE words resolved per instance at materialization |
| 4 | generated tables + heap gen -> key[2] (NRC:9664-9665) | sNdsNativeFighterActiveTables, gNdsTaskmanHeapGeneration | a | per-instance heap_gen check (1 compare) |
| 5 | detail LOW/HIGH -> key[1] bit 8, key[2], plan key (RAF:2357) | fp->detail_curr (RAF:3357); set by status reset (ftmain.c:4408-4413), DeadUp HIGH (ftcommondead.c:529), pause (ifcommon.c:2955, :3104) through SHIM:6757-6782 | b | tuple field |
| 6 | selected root vector = program -> key[3] root_offset, key[4] input_count (NRC:9634, :9667); plan (RAF:3708-3772) | joint->dl = f(modelpart_status[].modelpart_id_curr, detail) (ftparam.c:748-818, SHIM:6725-6755); SelectRootProgram AST:5487-5608 | b | tuple field (program id) resolved once per event |
| 7 | hidden-part DObj topology -> plan status generation (RAF:2358) | ftMainSetStatus hidden-part add/eject/reparent; bump at src/import/battleship_ftmain.c:214-215 -> RAF:712-729 | b | event: rebuild joint table |
| 8 | DOBJ_FLAG_HIDDEN / NOTEXTURE (source walk FDM:765, 780, 792-806; hidden hides the subtree FDM:823-826) | figatree SetFlags src/nds/nds_ft_pose.c:951-952 - in-status, NOT signalled; memo bypassed only for camera_mode Entry (RAF:1012-1016) | b* | read per frame in the kernel's preorder walk (free); mask change -> NOP-pad/restore that binding's geometry range |
| 9 | whole-fighter visibility (head early returns) | fp->is_invisible FDM:1087-1091; magnify/off-screen FDM:1093-1160; DeadUp FDM:1099-1102 | c | head unchanged; event_count==0 -> no draw (RAF:5213) |
| 10 | electric skeleton -> other root vector (FDM:847-); program 0xFE (AST:5420-5424, 5502-5514) | fp->colanim.skeleton_id (memo key RAF:760-761); owners exist for Mario/Fox only (src/nds/generated/nds_native_skeletons.generated.inc:2, 62-72) | c (2 of 5 frames) | tuple field; both variants live in the instance's 2 entries, so toggles cost no copy |
| 11 | Kirby head / trio body / copy hat (RAF:1218-1271; AST:5561-5585, 5636-5688) | modelpart_status[6-CommonStart].modelpart_id_curr; passive_vars.kirby.copy_id | b | program id (heads 1..12 are programs); hats for roster copies resident |
| 12 | Fox gun root stripped to a sidecar (RAF:1122-1196) | modelpart on joint 17 (Fox, Kirby-Fox copy) | b | gun sub-list inside the list (tuple bit) |
| 13 | Link/Kirby boomerang donor file 0x146 (RAF:2405-2440) | modelpart joint 11 / hidden part 12 | b | already a program |
| 14 | MObj chain identity -> key[0] (RAS:4284-4305 via RAF:4054-4056) | gcRemoveMObjAll + lbCommonAddMObjForFighterPartsDObj (ftparam.c:772-782) on part/costume change; invalidation SHIM:2610-2623 | b | event: re-bind site MObj pointers |
| 15 | MObj texture_id_curr/next -> key[0] (RAS:4257-4258) => RE-RECORD today | ftParamSetTexturePartID SHIM:2659-2716 (eyes, damage face; motion events, no renderer invalidation); ResetTexturePartAll SHIM:2725-2774 | b (frequent, in-status) | texture site table: read texture_id_curr per animated site, write TEXIMAGE/PLTT from a per-instance word table |
| 16 | MObj palette_id, lfrac -> key[0] (RAS:4259-4260) | matanim tracks | b/c | palette word via the same site table; lfrac blends UNKNOWN (INVESTIGATION_RENDER Q2) |
| 17 | MObj prim/env/blend/light1/light2 colours -> key[0] (RAS:4249-4256) | gcPlayMObjMatAnim (costume/main matanim) | a mostly, c if animated | shade sites carry the MObj; re-derive DIF_AMB when its 6-word colour run differs |
| 18 | colour modulate -> DIF_AMB re-derive, deliberately not keyed (NRC:9641-9642, 9765-9850) | colanim.color1 when is_use_color1 (RAM:374-390; ftmain.c:1055-1066) | c | same math (NDS R2 MaterialColor15 + Clamp, NRC:9812-9845), skipped when unchanged |
| 19 | tinted prim (untextured run of a non-white-prim epoch draws TEXEL(8x8 tile of prim) x clamp(l2+l1.dot), r49, TFX:3740-3768) -> tile bound by name at record (NRC:7108-7117, 8858-8868), DIF_AMB = raw light; prim change => re-record (NRC:9789-9799) | MObj prim (costume matanim overwrites MObjSub prim, TFX:3759-3762) + root preamble prim (RAF:496-500) | b, c when animated (P0 episode: every frame) | tiles for every colour the fighter can show resident from admission; a prim change = patch that epoch's TEXIMAGE/PLTT words to the tile of the new colour + the ApplyTint shade re-derive (2.6); never a re-record |
| 20 | env colour -> key[3] (NRC:9643-9644); non-white rejects textured families (NRC:8678-8684) | colanim.color2 / fp->fog_color / white (FDM:1180-1196, RAF:391-397) | c | NOT native today: keep today's behaviour + counter; POLY_ATTR/DIF_AMB sites later (coverage) |
| 21 | geometry/cycle/render mode, initial geometry mode -> key[3] (NRC:9635-9647) | head FDM:1176-1178, DecideFogDraw FDM:687-, FTParts NOFOG | a | baked; asserted at materialization |
| 22 | light valid flag -> key[3] (NRC:9645); light direction patch (NRC:10372-10386) | DrawReflect FDM:1168-1175 (colanim light uses fp->lr) and FDM:1238-1242 -> RAF:359-383; MEASURED 0 dir changes in 22,296 epochs (NRC:3900-3902) | a / c in colanim light | patch LIGHT_VECTOR only when (x,y,z) differs |
| 23 | material_count per root -> key[3] (NRC:9648-9649) | MObj chain length | b | fixed per variant |
| 24 | global tint-tile set generation -> key[3] of every packet (NRC:9658-9661) | bumped on tile create/evict/VRAM reset (TFX:3829, 3936, 3959; 16-slot shared LRU TFX:3769-3786); MEASURED stable over the P0 match (builds 9, misses 24, evictions 0, set generation 11, hits 5,248 - coordinator's Phase 0 counters) | b (load-time in practice) | not the churn; still dropped from selection because tiles are resident per instance (2.8) |
| 25 | GX-compose shape -> key[4] (NRC:9650-9656) | CaptureOwnerChainsGx RAM:6321-6413 (declines at 4P, RAM:6168-6185) | a | deleted (no GX compose, owner ruling) |
| 26 | texture residency / fence -> key[5] + TexturesResident (NRC:9717-9735, 10147-10153) | shared texture cache LRU (evictions by stage/effects) | c risk | every reachable fighter texture admitted + pinned before GO (2.8); no per-frame test |
| 27 | Captain HIGH -> never packetised (NRC:10192-10200, 10250-10268) | alpha-test register state outside the FIFO | b | D5 decision (colour-0-transparent palette is a candidate) |
| 28 | texgen TEXCOORD words (Link) (NRC:9906-10019; precheck NRC:10216) | camera LookAt x binding world | c | lean texgen sites (same math) -> HW normal TEXGEN + texture matrix after the Task 49 differ |
| 29 | projection per root (NRC:10325-10330, 10360-10364) | camera look-at x perspective (RAM:5668-5670), cached per frame (RAM:5562-5594) | c | P' once per list |
| 30 | binding modelviews (NRC:10331-10371) | DObj TRS + FTParts | c | kernel -> LOAD4x3 |
| 31 | hitlag shuffle -> compose seed (RAM:7420-7436, 6091-6108) | fp->shuffle_tics / is_shuffle_electric / shuffle_frame_index | c | kernel adds (sx,sy) to binding translations (exact, see 4) |
| 32 | animation locks (RAM:7437-7454, 6708-6998) | fp->is_use_animlocks + FTParts vec_scale | b/c | kernel slow branch (existing BuildAnimLockInvariantMtx) |
| 33 | FTParts warm gameplay local (RAM:2676-2720, 6999-7006) | parts->transform_update_mode, unk_dobjtrans_0x10 | c | kernel branch (F2LFixedWExact) |
| 34 | draw-time texture resolve/bind -> native reject when it fails (NRC:8752-8758) | ndsRendererHardwareBindTexture on the record/direct path; texture keys from live MObj state | c (on misses) | none at draw time: TEXIMAGE/PLTT words are pre-resolved per instance at admission (2.8) |
MEASURED miss causes (2-fighter lab, artifacts/verifier-temp/slot6/packet_counters_probe.gdb.out:7): of 92 records, key[4] (program) 81, key[0] (material identity) 14.
Four-CPU match: 6,673 hits / 178 records / 0 declines; plan hit 6,217 / build 618. Rows 6, 10, 11, 15 are the whole miss population the lean path must absorb.
Phase 0 (P0, same roster, frames 2-1973): row 19 owns P95 - frames 798-1043 (246 presented) run FTR ~2.6M/frame (median 363K) because one fighter's prim changes every
frame (a flashing colour animation), so its tinted packet re-records every frame (NRC:9789-9799) and production re-resolves all of its textures; other fighters are not
re-recorded and the tile set is stable (row 24). Per-PC profile of the episode: ndsRendererHardwareResolveOrBindTexture 935,578 + ndsRendererHardwareTextureColor
226,072 tk/fr, ndsFighterPacketCmd 141K, production 90K, PrepareProductionRun 48K. Row 34 at match start: first cause Link status 225 nFTLinkStatusAppearL, use_texture FALSE at NRC:8758, 573 native failures /
254 direct rejects (the 2026-09-17 checkpoint was 0/0). WORK-H P95 is 4,207,488 with, and 2,639,232 without, the entry (186-200) and tint (798-1043) frames.

## 2. The lean per-frame path
### 2.1 Stages today -> lean (costs = INVESTIGATION_RENDER Q1 unless noted)
| stage (tk/fr) | lean | why |
|---|---|---|
| 1 head + capture (~42.6K) | RUN | side effects (2.2). Hoist gmCameraLookAtFuncMatrix (RAF:981-983, MEASURED 3,680) to once per frame: camera-only input |
| 2 plan/validate (~9.4K) | SKIP on hit | tuple compare (4 loads); DrawPlanResolve + SelectRootProgram run once per event |
| 3 matrices (~112.6K, ~1,100/joint) | REPLACE | kernel (section 4), ESTIMATE ~450/joint |
| 4 material identity/refresh/fence (45.7K DrawForSlot self incl.) | SKIP | MObj state read only at animated texture/colour sites (rows 15-17) |
| 5 precheck: key 7.7K, texgen 8.1K, residency 3.2K | SKIP | no key; residency by pinning; texgen moves to patch |
| 6 patches (TryReplay 24.4K + StoreSplitModelview 12.0K MEASURED) | RUN, smaller | 1 P' + 12-word LOAD4x3 per binding instead of 2x16-word LOAD4x4 per root; tint/light/texture only on change |
| 7 flush (~1-2K) | RUN | flush patched lines only (matrix params grouped per binding, 12 words = <=2 lines) |
| 8 DMA0 (~3.2K) | RUN | one job per fighter, not waited (as NRC:10393-10398) |
| 9 miss/record (~12K mean, +~75K tail) | DELETED | event-time materialization from templates (~5-7K/event ESTIMATE) |
### 2.2 Head side effects, verified against source (all must survive; the head stays source C)
FDM:1084-1085 fog statics reset; FDM:1089/1101/1133-1138/1151 fp->is_magnify_show + magnify_pos; FDM:1140-1147 gIFCommonPlayerInterface.magnify_mode + ifCommonPlayerArrowsUpdateFlags
(off-screen arrow HUD; BoundsFailCount=142 in the canonical match, RAF:607-608); FDM:1156-1158 magnify viewport (non-main camera); FDM:1162 gLBCommonScale = 1;
FDM:1168-1175 + 1238-1242 scene light twice -> port RAF:359-383 (the next fighter's and later drawers' light); FDM:1180-1196 sFTDisplayMainSkyFogAlpha + env; FDM:600-663
sFTDisplayMainFogColor (CalcFogColor); FDM:1224-1228 root xobj kind swap (non-main camera only). Capture adds gmCameraLookAtFuncMatrix(NULL,...) writes of gGCMatrixPerspF /
sGCMatrixProjectL / gGMCameraMatrix (RAF:965-984) that FDM:1129/1136 read. The hitlag gSPMatrix (FDM:1205-1217) lands in scratch DL heads; the renderer uses fp->shuffle_* (row 31).
The walk (FDM:1218-1229) only feeds the contract in Master mode (RAF:623-628), so on a memo hit it stays collapsed; the lean path never needs the event list except on event frames.
### 2.3 Data structures (new file src/port/renderer_fighter_lean.c + src/nds/nds_ftr_lean_kernel.c)
```c
typedef struct { u16 teximage, pltt; u8 binding, mobj_index, animated, id_count; const u32 *words; } NDSFtrLeanTexSite; /* words[id*2] resolved per instance */
typedef struct {                        /* generated per (kind, detail, program[, gun]); resident for the match */
    const u32 *words; u16 word_count, proj_index, light_index, light_root;
    u8 binding_count, joint_count;      const u32 *root_offsets;             /* program identity, AST:5597 */
    const u8 *joint_parent, *joint_binding;   u32 needed_joint_mask;         /* preorder JointSchedule, INC:494-498 */
    const u16 *mtx_site;                /* [binding] -> 12 LOAD4x3 params (+STORE slot if cross-referenced) */
    const u16 *geo_first, *geo_len;     /* [binding] header-aligned geometry range, NOP-padded when hidden */
    const NDSFtrLeanTexSite *tex; u8 tex_count; const NDSFighterPacketShadeSite *shade; u8 shade_count; /* PRE:3346 */
    const NDSFighterPacketTexgenGroup *tg; const NDSFighterPacketTexgenSite *ts; u8 tg_count; u16 ts_count; /* PRE:3373-3393 */
} NDSFtrLeanTemplate;
typedef struct {                        /* 2 per instance (active + previous), each 4,420 words of the slot's 35,360 B region (PRE:3310-3322) */
    u32 *words; const NDSFtrLeanTemplate *tpl;
    u8 program, detail, gun, valid;     /* THE STATE TUPLE (program covers Kirby heads/hats and skeleton 0xFE) */
    u32 visible_mask, tint_modulate, tint_prim_hash; s32 light[3]; u16 tex_id[24];
} NDSFtrLeanEntry;
typedef struct {
    NDSFtrLeanEntry e[2]; u8 active, rebind, joint_count;
    u32 status_gen, heap_gen;           /* sNdsFighterStatusGeneration[slot] (RAF:691), gNdsTaskmanHeapGeneration */
    DObj *joint[NDS_RENDERER_NATIVE_FIGHTER_JOINT_MAX]; FTParts *parts[...]; u8 parent[...], binding[...];
    MObj *tex_mobj[24]; MObj *shade_mobj[64]; u8 root_event[32];            /* preamble index per binding (tint/light) */
    struct { u32 rgb, teximage, pltt; } tint[16]; u8 tint_count;           /* resident 8x8 Pal16 tiles (index 1 everywhere, TFX:3813-3815),
                                        one per colour this fighter can show, created at admission; words patched per tinted epoch */
    struct { u32 teximage, pltt[2]; u16 rgb15[2]; u8 front; } tint_spare;    /* fallback for a colour not enumerated: palette rewrite */
} NDSFtrLeanInstance;
static NDSFtrLeanInstance sNdsFtrLean[GMCOMMON_PLAYERS_MAX];                  /* ~1 KB each */
```
### 2.4 Per-frame algorithm (called at RAF:5267 in place of DrawForSlot)
1. Head: ndsFighterDisplayContractCapture unchanged (RAF:919-1027); return if event_count==0 (RAF:5213).
2. Tuple check: in->status_gen==sNdsFighterStatusGeneration[slot] && in->heap_gen==gNdsTaskmanHeapGeneration && e->detail==fp->detail_curr
   && (e->program==0xFE)==(fp->colanim.skeleton_id!=0) && !in->rebind. Fail -> event path (2.5).
3. Kernel over in->joint[] (section 4): writes LOAD4x3 params in place, returns visible_mask; mask != e->visible_mask -> NOP-fill / restore geo ranges from tpl (rare).
4. Patches: P' (16 words, once per frame, shared value); tint (NRC:9765-9850 logic over tpl->shade with live modulate RAM:374-390 and preamble prim
   sNdsFighterDisplayReplayPreambles[root_event[i]]) only if (modulate, prim hash) moved; light word (NRC:9873-9892) only if dir moved; texture sites: for animated
   sites whose tex_mobj->texture_id_curr/palette_id differs from e->tex_id, copy 2 words; tinted epochs: if the prim moved, find its rgb in in->tint[] (<=16 compares)
   and patch that epoch's TEXIMAGE_PARAM/PLTT_BASE words, then the ApplyTint shade re-derive above - no key, no record, no texture resolve; texgen (Link) every frame.
5. Clean patched lines; EndBatch/glEnable/glDisable/DMA0/tracker invalidation exactly as NRC:10389-10407; credit stats as NRC:10409-10453 (verifiers read them).
Order: submit all four heads+kernels first, then patch+DMA per fighter (RAF:5308-5340 loop), so no DObj read runs under a DMA burst (R2).
### 2.5 Event path and invalidation
- Triggers: status generation (RAF:712-729 from battleship_ftmain.c:215 and SHIM:2614), detail, skeleton, heap generation, rebind flag.
- Steps: the memo was invalidated with the generation (RAF:728), so this frame's events are live -> ndsFighterDrawPlanResolve (RAF:2444-2588) ->
  SelectRootProgram (AST:5487-5608) + Kirby head (RAF:1218-1243) + gun bit (RAF:1142-1196) -> tuple. Tuple in e[0]/e[1] -> activate; else materialize into the
  LRU entry: copy tpl words, copy the instance's pre-resolved TEXIMAGE/PLTT words (2.8: nothing is resolved or uploaded here), flush the whole buffer
  (ESTIMATE ~2.5K words copy ~2K + words ~0.3K + flush ~1K).
  Always rebuild the joint table (CollectFighterTopology RAM:1796 once, binding map as RAM:7092-7112) and re-bind site MObjs; verify against tpl JointSchedule (decline counter).
- ndsFighterRendererInvalidateMaterialCachesForSlot (RAM:532-540) sets in->rebind instead of dropping entries; scene reset clears sNdsFtrLean.
### 2.6 Special cases
- Model-part programs (Samus Catch/MorphUnfold/MorphBall/FSmash, Link Entry/Catch/SpecialN/Claps, Yoshi, Ness): tuple program; first use per match = materialize, later toggles = entry swap.
- Kirby: heads 1..12 + Stone + CopyLink + CopyTransition = 16 programs (AST:5529-5534, NDS_NATIVE_KIRBY_TRIO_HEAD_COUNT 12); copy-hat templates for the roster's
  copyable kinds loaded at match load instead of the deferred per-slot hat image (AST:5626-5688); head->body vertex dependency lives inside each head program.
- Hidden flips inside a status (Entry, figatree SetFlags): kernel flag walk; geometry range NOP-padded, matrix prologue (LOAD4x3 + STORE) kept so cross RESTOREs stay valid.
- Link texgen: port PatchTexgen to tpl->tg/ts using the kernel's 20.12 world for texgen bindings (exact, ~8K MEASURED for Link); later HW TEXGEN (~17 words/root, A1).
- Tint (keeps r49's order clamp31(l2 + l1.dot) x prim, TFX:3740-3757): the generator marks tinted epochs; their words are a TEXIMAGE_PARAM + PLTT_BASE patch site
  (the tile), the tile-centre TEXCOORD (NRC:7124-7134) and raw-light DIF_AMB sites. At admission (2.8) the instance gets a resident tile for every colour its tinted
  materials can show (costume prims + the colour keys of its matanim/colanim prim tracks; the whole P0 match needed 9 tiles), so frame 1 is exact (today's first frame of
  a colour draws the capped fold, TFX:3762-3764). A prim change patches the two words to the new colour's tile and re-derives the shade words exactly as ApplyTint does
  (NRC:9765-9850; colour modulate keeps acting on the raw-light words, NRC:9786-9788) - never a re-record, so the P0 episode costs ~1K instead of production +
  a full texture re-resolve. A colour outside the admitted set (an interpolated track) uses tint_spare: palette entry 1 rewritten at the frame-boundary seam where tiles
  are created today (TFX:3877-3961), double-buffered by the "two frames" rule (TFX:3911-3913), 1 presented frame late, counted. Lean fighters never touch the shared
  colour-keyed table (TFX:3836-3875), so nothing they do moves gNdsR2FighterTintSetGeneration for old-path fighters during the transition.
- Hurt flash: row 18. Texture animation (eyes, damage face): row 15 site table, all ids pinned (2.8). Detail hi/lo: tuple; both details' images are already loaded at
  fighter creation for low-detail battles (src/import/battleship_ftmanager.c:694-701), so the HIGH switch at DeadUp/pause never reads storage.
### 2.7 Cost (ESTIMATE, four fighters)
head 20-30K (ProcDisplay 6,476 + HeadBoundary 3,671 + ProjectTarget 1,666 + DrawReflect 1,372 + bounds 993 + ContractSubmit self 11,687 MEASURED, minus the hoisted
LookAt) + kernel 103 joints x ~450 = ~46K (fewer with needed_joint_mask) + patch/flush/DMA ~4K + Link texgen 0.5-8K => ~70-88K P50; hurt flash +1-3K; event frames +5-7K
each; an animated tinted prim +~1K (<=16 compares, 2 words, shade re-derive) where the P0 episode pays ~2.2M/frame today.
### 2.8 Texture admission before GO (replaces draw-time bind, row 34)
Seam: ndsFTManagerEnsureOwnerImages (src/import/battleship_ftmanager.c:665-729), which already runs at fighter creation for battle setup and CSS rebuilds, followed by one
roster pass after the last fighter is made (Kirby hats need the roster). For each instance and each reachable template (every program incl. Entry/Appear programs, both
details, the roster's copy hats), walk tpl->tex: for every (MObjSub, texture id, palette id) the site can show (texture-part ids from FTTexturePartContainer, matanim
ids, costume palettes) convert + upload once (dedupe by the existing key), mark the cache entry pinned (PRE:5053-5076) and store the TEXIMAGE/PLTT words in the instance
table; create one resident tint tile per colour the tinted materials can show (2.6) plus tint_spare. Any failure latches (kind, program, site, key) in
gNdsFtrLeanAdmitFail and fails the load, never a frame. Gate: admit failures 0, fighter texture uploads after GO 0 (P0's MTEX column restricted to fighters), native
failures 0 including the entry frames 186-200.

## 3. Eliminating misses: recommendation = host generator
- Warm-up recording at match load is rejected: it keeps production (FPR 7.8K .text, NRC fighter half, material rows 19,200 B BSS...) alive against D2, and recording a
  program needs the live DObj/MObj tree in that program's state (ftParamSetModelPartID allocates/frees MObjs, SHIM:6725-6755) - a load-time mutation of gameplay objects.
- The generator already owns every ingredient (roots/epochs/runs, 12.4 VTX16, strips, cross corners, program tables, JointSchedule) and emitted a whole-owner FIFO template with
  patch tables (GEN:9165-9245 -> INC:7938-18350, Mario 4,034 words/320 tris). That fixture is stale (per-vertex COLOR, LOAD4x4 per root, INC:13160-13208) and must be
  rebuilt to today's words: NORMAL + HW-light DIF_AMB/SPE_EMI per epoch, Task 56 strips, POLY_ATTR, TEXIMAGE/PLTT sites, STORE/RESTORE cross corners, P' + LOAD4x3.
  Proof: host word-compare against runtime-recorded packets with matrix/texture words masked (scripts/fighters/check_nds_native_owner_packet.py) + the runtime oracle (6.4).
- Fallback if the host port of the emitter stalls after DK: a load-time C list compiler fed by generated tables + MObjSub descriptors (never live DObjs) - decide at that gate.
Variants (tuple values a match can reach): DK canonical x2 details (+Giant = matrices); Samus 5 programs (AST:5516-5520) x2; Link 5 (AST:5522-5527) x2; Kirby 16 x2, of which
~6-8 reachable per roster (canonical, own faces, Stone, the roster's copy hats, CopyLink/Transition only with Link); Mario/Fox (+skeleton 0xFE), Yoshi 3, Ness 2, Captain LOW
(+HIGH, D5); Fox gun as a sub-list variant. Entry/Appear programs (Link Entry = the P0 failure) are ordinary variants and must be admitted before GO (2.8). HIGH is
reachable through DeadUp (ftcommondead.c:529) and pause (ifcommon.c:2955); both details' images are already loaded at creation (battleship_ftmanager.c:694-701), so all
variants are resident with no storage read after GO. Polygon RAM does not bound the variant set (MEASURED P50 559 / max 675 of 2,048 polygons, P0:35-37).
Tint is not a variant: resident per-colour tiles + a TEXIMAGE/PLTT patch + shade re-derive (2.6) replace today's re-record, so an animated prim adds 0 variants/records.
RAM (ESTIMATE): today's LOW packets DK 2,710 / Samus 2,088 / Link 2,890 words (2026-09-15 packet footprint); LOAD4x3/P' saves ~23 words per root -> ~1.8-2.5K words LOW,
<=~4K HIGH (MEASURED HIGH max 2,221 words for Mario/Fox, slot6 probe). Per instance: 2 x 4,420 words = the existing 35,360 B region (PRE:3293-3296) -> 0 new bytes.
Templates per kind (all LOW programs as shared sub-lists + HIGH canonical + tables): ~20-30 KB, ~100-120 KB for the measured roster, replacing owner images (DK 15,232+21,340,
Samus 14,544+19,316, Link 15,892+19,744, Kirby 24,268+30,160 B + hats) and ~98 KB of production BSS (section 5). VRAM: tint tiles, 9 over the whole P0 match
(coordinator counters) + 1 spare per instance, x (32 B texels + 32 B palette) <1 KB; the pinned fighter texture union is UNCENSUSED (R5) - the admission pass prints it first.

## 4. Matrix kernel (replaces stage 3)
Inputs per joint j (preorder, flattened at the event): DObj translate/rotate/scale f32 + flags, FTParts transform_update_mode/unk_dobjtrans_0x10/vec_scale, fp animlock flag.
(A3/Phase 4 later hands Q values directly and removes the float edges; this kernel does not wait for it.)
1. 16.16 local L (s32 cells, NOT the N64 split Mtx: ndsRendererMtxCellS16p16 costs 11,178 tk/fr MEASURED) following BuildSourceFighterLocalMtx (RAM:6860-7043) branch order:
   no FighterParts xobj -> no local (W = parent, RAM:7175-7179); animlocks -> existing BuildAnimLockInvariantMtx (RAM:6733); warm cache -> F2LFixedWExact (RAM:6999-7006);
   scale != 1 -> syMatrixTraRotRpyRSca (float, 6,836 MEASURED; keep, count); else FAST: ndsFighterMatrixAngleToIndexExact x3, FloatPow2ToS32(t,16) x3, gSYSinTable
   (RAM:2237-2248), cells exactly RAM:2293-2327 (L00=(cp*cy)>>14 ... L22=(cr*cp)>>14, T=16.16); any conversion failure -> syMatrixTraRotRpyR (RAM:7032-7039).
2. Compose in Q43.20 (fidelity contract RAM:6548-6573): root W.b=L.b*16, W.t=(s64)L.t*16 (RAM:6607-6626); else W.b[r][c]=rs16(sum_k L[r][k]*P.b[k][c]),
   W.t[c]=rs16(sum_k L.t[k]*P.b[k][c])+P.t[c] (RAM:6628-6671), rs = round half away from zero (RAM:6574-6588). 27+9 SMULL/SMLAL, s64 accumulate.
3. Binding out: R=rs8(W.b) Q20->Q12, T=rs8(W.t) with the s32 range check (RAM:6680-6703); hitlag T += (sx,sy) Q12 - bit-identical to today's seed multiply because
   R x I and t x 4096 >> 12 are exact (RAM:7420-7436); then the world-unit shift PRE:1912: params = R rows 0-2 (9 words) + RoundShiftS32Signed(T,8) (TFX:44, :13139-13143).
   Texgen bindings also keep the Q12 world for 2.6.
4x3 trap: today's split load keeps m33 = 4096>>8 in the modelview (TFX:13126-13144). LOAD4x3 forces m33 = 1, so load P' = P with row 3 (all 4 cells) RoundShiftS32Signed(.,8)
once per list: (v/256.R + T/256, 1).P' = (world.P)/256 -> x/w, y/w, z/w unchanged; P row 3 carries the view translation (look-at x perspective, RAM:5668-5670), so >>8 keeps
>=12 significant bits (ESTIMATE); differ-proof required (Task 49). Light stays written under an identity vector matrix before the first LOAD4x3 (NRC:3833-3836).
Placement/size: target("arm"), section ".itcm", noinline, <=2,048 B incl. the fast TRS (ESTIMATE ~1.5 KB); joint scratch 30 x 60 B = 1.8 KB in DTCM (A7: freed Mario tables).
ITCM is full (32,704/32,768 MEASURED, prechecked-replay README) -> evict miss-path-only ndsRendererNativePrepareProductionRun (2,412 B ITCM in the 4-CPU nm) in the same batch.
Cost: ESTIMATE ~450 tk/joint (fast TRS ~150, compose ~110, output ~40, loads/stall ~150) vs MEASURED ~1,100 (Direct20p12 278/call, MtxMulAffine20p12 347/call).
Deletes from the fighter path: PrepareNativeOwnerMatrices (RAM:7326-7599), ComposeOwnerWorldsFlat (6416-6546), ComposeOwnerWorldsSource (7053-7208), SourceWorld*
(6574-6706), BuildSourceFighterLocalMtx (6860; absorbed), CaptureOwnerChainsGx/BuildGxSlotTable (6159-6413), PrepareOwnerMatricesPerBinding (7218-7318), SetShuffleOffset
(6091-6108), TFX LoadHardwareGxComposedMatrices (13204-13276, 668 B ITCM) + StoreSplitModelview/record twins (13047-13352). BuildDObjLocalMatrix/XObj/FighterPartsMtx/
Direct20p12 stay until Phase 2 (effects attached to fighter joints use them, RAM:6132-6138).

## 5. Deletion once every admitted fighter (and every fighter scene) is lean
Delete: FPR whole file (ndsRendererExecuteNativeFighterOwnerProduction 3,568 B ITCM); NRC production + packet half: PreflightProductionOwner (:3638), PrepareProductionRun
(:9030), EmitProductionPrimitiveGroups/CrossRun (:9238/:9312), SubmitProductionRun (:10627), ShadeProductionActions (:6480), packet key/match/precheck/TryReplay/record
(:9604-10590, keep a renamed Release :10577-10590 because the lean regions borrow gSYFramebufferSets), hierarchy executor (NRC:12982, src/nds/nds_renderer_native_owners.c:249);
PRE recorder + hooks (PRE:3271-3830); RAF DrawForSlot (3227-4975), CollectAllDObjsWithDL (1029-1070), production/hierarchy inputs (1417-1860), plan Hit/Gather/Apply/Verify
(2291-2360, 2590-2680), GX topology census; RAM section-4 list; material rows/keys/snapshots (RAM:394-448, RAS:4307-, RAF:4093-4147); texture run memo + fence (NRC:7947/:8167);
the shared colour-keyed tint table + set generation (TFX:3769-3961) and the draw-time tile bind (NRC:7108-7134) once no old-path fighter remains.
BSS (MEASURED nm -S, INVESTIGATION_RENDER Q6): NativeOwnerMaterials 19,200; sNdsFighterPackets 14,192; NativeOwnerWorkspace 12,432; sNdsNativeFighterOwnerExecution 8,824;
sNdsR2RunTextureMemo 7,504; sNdsNativeFighterRunUvInputs 7,504; persistent_renderer_vertices 5,780; DrawPlan 3,776+916; ValidationCache 3,408; MaterialKeys 2,304;
TextureCurr/Next 1,536 (~87 KB); DTCM PreparedDense 6,130 + DenseNormals 2,452. Generated: production-only runtime tables in INC and the #if 0 fixture (INC:7938-18350).
Remains: head/capture/memo (RAF:262-1027) + contract macros (src/import/battleship_ftdisplaymain.c:81-164); status-generation writers (battleship_ftmain.c:214-215,
SHIM:2610-2623); DrawPlanResolve + SelectRootProgram as the event-path resolver (until a generated modelpart-vector -> program map replaces the walk); tint/light/texgen math
(moved into the patcher) with ndsRendererR2MaterialColor15 (ITCM); texture cache with pinning; slim decline witnesses; the packed-1P halt contract (RAF:4405-4460) re-seated
on lean declines. The recorder stays only as the lab oracle until the last kind passes, then goes; warm-up recording would have kept it plus production permanently.

## 6. First vertical slice (one batch, same ROM)
Scope: Samus (owner NDS_RENDERER_NATIVE_FIGHTER_OWNER_SAMUS), LOW, program 0, no animlocks; everything else takes today's path. The list is the RECORDED packet ADOPTED once
(same layout: split projection at roots[i].local_index[0], modelview at seed_index, PRE:3326-3336, TFX:13155-13201), so the slice proves the kernel, tuple fast path,
patch/DMA and oracle without the generator; ORDER step 5 swaps in the generated LOAD4x3/P' list.
6.1 Flag: `volatile u32 gNdsFtrLeanRoute = 0;` (.data, runtime poke, same ROM): 0 off (shipping default, byte-identical behaviour); 1 lean draws; 2 oracle-exact (old path
draws with the Q43.20 compose forced for Samus, lean computes into its entry, compare, expect 0); 3 oracle-shipped (old path unchanged, report LSB deltas of flat Q20.12 vs lean).
6.2 Hooks: H1 RAF:5267 call site -> `if (route==1 && ndsFtrLeanDraw(slot,fp)) {} else { if (route>=2) ndsFtrLeanShadow(slot,fp); ndsFighterMarioFoxDLAllDrawForSlot(...); }`
inside the FTR bracket (RAF:5193-5195, :5286-5287); lean must bump gNdsFighterMarioFoxDLAllDrawCount/triangle counters (RAF:5272-5285). H2 NRC:10386/10387 (TryReplay after
the light patch, before DC_FlushRange): `ndsFtrLeanOracleCompare(battle_slot, packet)` under NDS_FTR_LEAN_ORACLE. H3 NRC:10139 (FinishRecord valid=1): note adoption candidate
(slot, key[0..5], root_count, sNdsFighterDrawPlan[slot] program/bindings RAF:2301-2324). H4 RAM:7469-7483: add `|| (gNdsFtrLeanRoute==2u && slot==SAMUS)` to force the source
compose. H5 NRC:10489-10496: when route != 0, recorder capacity = lower 4,420 words of the slot region; lean entry = upper 4,420 (Samus LOW 2,088 fits; >4,420 faults cleanly,
counted by gNdsFighterPacketFaults). H6 RAM:532-540: set sNdsFtrLean[slot].rebind.
6.3 Lean draw (slice): admit -> tuple (2.4 step 2) -> slice-only exactness guards for inputs not yet patchable: material identity RAS:4284-4305 == adopted key[0] (blinks/
texture anim decline; ~2K ESTIMATE), per-root preamble env/flags/geometry/cycle/render == adopted (key[3] inputs), textures resident + touch (NRC:9717-9761) -> kernel
-> projection copy per root + split modelview per root -> ApplyTint/light as NRC:9765-9850, :10372-10386 -> flush + DMA + credits (2.4 step 5). No set-generation guard (row 24 is stable); adoption
counts tinted sites (shade site reserved[0] != 0, NRC:9793; an epoch is tinted when untextured, material-coloured and prim != white, NRC:7083-7088). If Samus's LOW
canonical packet has none, nothing else is needed. If it has some, slice 1 also carries the 2.6 tile-word patch: a one-line recorder hook after the tile bind
(NRC:7113-7117 -> PRE:3649-3660) notes the TEX_FORMAT/PAL_FORMAT word indices so adoption turns them into tint patch sites over the 2.8 resident tiles, and a prim
change becomes a patch instead of today's re-record (NRC:9789-9799).
Adoption happens at the first lean attempt after H3 with matching (status_gen, detail, program 0, root_count): copy words + patch table (roots, sites, textures, light, tint state).
A second .data word gNdsFtrLeanAdmit runs the 2.8 admission at fighter creation: A/B arms (admit 1, route 0) vs (admit 1, route 1) isolate the draw, (admit 1 vs 0,
route 0) prices admission alone (it should also clear the old path's entry failures), and (0, 0) is today's load byte-for-byte.
6.4 Oracle (routes 2/3): compare lean entry words vs TryReplay's packet at every patch site: projection (16/root), modelview basis (12/root), modelview row 3 (4/root), each
shade DIF_AMB word, light word; plus soundness: when the old path re-records while the lean tuple hit and no guard declined, compare the new words outside patch sites
with the lean entry: identical -> RecordUnderHitSame (e.g. a set-generation-only re-key, row 24), different -> KeyMoved[word of the key that moved] = a hole in section 1's
classification. Route 2 gate: all mismatch counters and KeyMoved 0 over a whole four-CPU match, including the P0 episode frames 798-1043.
6.5 Counters (NDS_TICK_HUD, sampled into rows): gNdsFtrLeanDraws, gNdsFtrLeanAdopts, gNdsFtrLeanDecline[16] (0 route/kind, 1 detail/program, 2 tuple, 3 adoption pending,
4 animlocks, 5 material identity, 6 preamble, 7 binds a tint tile, 8 residency, 10 kernel range, 11 topology mismatch), gNdsFtrLeanAdmitFail (+ latched first key),
gNdsFtrLeanAdmitTextures/Bytes, gNdsFtrLeanPostGoUploads, gNdsFtrLeanTintPatches, gNdsFtrLeanTintSpare, gNdsFtrLeanKernelJoints, gNdsFtrLeanKernelSlowJoints
(RSca/animlock/F2L/float fallback), gNdsFtrLeanKernelTicks, gNdsFtrLeanHeadTicks, gNdsFtrLeanPatchTicks, gNdsFtrLeanDmaWaitTicks, gNdsFtrLeanOracleRuns,
gNdsFtrLeanOracleMismatch[6] (proj, basis, row3, shade, light, other), gNdsFtrLeanOracleMaxLsb[3], gNdsFtrLeanOracleKeyMoved[6], gNdsFtrLeanOracleRecordUnderHit[2]
(same, differs).
6.6 Gates: route 2 exact (0/0); GXSTAT stack level flat; native failures 0; route 1 vs 0 four-CPU stress, one build directory, whole match: FTR and WORK-H P50/P95, OTHR and
MISC flat (relocated stall), engagement = Samus draws x ~1 minus counted declines. ESTIMATE: Samus ~86K FTR share (INVESTIGATION_RENDER first slice) -> ~20-25K, FTR P50 -55..-65K,
WORK-H must fall >=45K or stop (arch doc section 6 stop rule).

## RISKS
R1 Head is the largest lean term: MEASURED ~30K/fr of head+capture self (2.7) vs the arch doc's ~12K; if the capture glue (ContractSubmit self 11,687) does not shrink, FTR P50 ~80-88K.
R2 DMA is not free: loads stall 1,022-1,660 cycles behind GXFIFO bursts (INVESTIGATION_RENDER Q8); the kernel reads scattered DObjs -> two-phase submit (2.4), measure DmaWait/OTHR.
R3 Float edges remain until A3: angle-index exactness failures and RSca (6,836 MEASURED) run float; FTParts pointer chase per joint is stall-bound (64% of cycles are stall).
R4 Generator/runtime divergence: production has content-specific behaviour (state deltas, epochs, tint tiles, primitive groups, HW-light folding); lists must be word-exact
   and the recorder-based oracle must outlive the last kind. Tint is the costliest instance: today a tinted packet re-records on every prim change (NRC:9789-9799) and
   re-resolves all the fighter's textures (P0 episode); the lists must carry TEXIMAGE/PLTT tint patch sites + shade sites, and every tile the fighter can show must be
   resident, so a prim change is a patch plus the shade re-derive, never a re-record. Env colour (NRC:8678-8684) still has no patch model.
R5 VRAM: pinning every reachable fighter texture id (eyes, damage faces, costume palettes, hats, Entry programs) is uncensused against A+B 256 KB / F+G 32 KB, which stage,
   effects, particles and the startup-only entry props (TFX:3972-3977) share at GO. The P0 entry failure (NRC:8758) may itself be VRAM or key exhaustion introduced since
   09-17 (cause unknown): root-cause it with the admission census before relying on admission to hide it; a load failure is the honest outcome if the union does not fit.
R6 D2 scope: CSS (4 HIGH previews, NRC:10457-10459), VS Results (hand variants AST:5689-5720), 1P intro transient (21 actors, RAF:5042-5139), autodemo and packed-1P halts
   (RAF:4405-4460) all use this renderer; "no dual paths" needs them lean too, or an explicit owner ruling that production survives for menus until Phase 2.
R7 Not yet native: electric skeleton exists only for Mario/Fox (skeletons.generated.inc:62-72) - DK/Samus/Link/Kirby skeleton frames decline in rosters with electric
   attackers; env colour; Captain HIGH alpha (D5); linear texgen N-*; Pikachu/Purin accessories (FDM:776-822, UNKNOWN).
R8 Possible existing bug (inferred, unmeasured): the draw plan keys only on status generation (RAF:2338-2359) while figatree SetFlags (nds_ft_pose.c:951-952) can change
   visibility inside a status; Entry bypasses the memo (RAF:1012-1016) but not the plan. Check with gNdsFtrPlanVerify (RAF:2645-2679) on an Entry; the lean flag walk avoids it.
R9 Hidden bindings that donate cross corners: N64 would read a stale vertex cache; lean keeps their matrix prologue, geometry differs only in that undefined case.
R10 ITCM full (64 B free): kernel funded by evicting PrepareProductionRun, which slows record frames in BOTH arms until step 8 (tail, not P50).
R11 4x3/P' precision and stack: prove with the Task 49 differ; STORE only cross-referenced bindings (<=14, Yoshi), slots 0-7 left to PUSH users (arch doc A1).
R12 Event bursts: 4 simultaneous events on a hit frame ~24K ESTIMATE (vs +75-441K today). HIGH images are already resident at creation (battleship_ftmanager.c:694-701);
   polygon RAM is not a risk (MEASURED max 675 of 2,048, P0:35-37).
R13 Tint colour enumeration: resident tiles cover the colours admission can enumerate (costume prims, matanim/colanim colour keys); the patch is then same-frame. An
   interpolated colour track can show colours outside the set -> tint_spare (palette rewrite at the frame-boundary seam, 1 presented frame late, LCDC window at
   TFX:3877-3961 as today). Count spare use; if it is not ~0 on the roster sweep, enumerate better rather than widen the fallback.
R14 The P0 episode also carries an effect drawing ~144K/frame (MISC, P0:42-47); Phase 1 removes the fighter half (re-record + texture re-resolve). Name the flashing
   fighter and its colour source (per-slot tinted re-records) before claiming the P95 recovery.

## ORDER OF WORK
1. Phase 0 leftovers: GE-busy/DMA0-wait counters, FTR sub-phase counters (head, kernel, patch), per-slot tinted re-records and fighter texture uploads in the rows (P0 added
   GPOL/GVTX, MISC split and the digest).
2. Slice 1 (section 6): Samus LOW canonical, adopted packet, kernel in ITCM, routes 0-3, oracle exact, 2.8 admission for the four kinds. Gate 6.6 + admit failures 0.
3. Slice 2: the P0 episode's flashing tinted fighter with 2.6 resident per-colour tiles + tile-word/shade patch; gate: frames 798-1043 re-record 0 packets and resolve
   0 fighter textures (ResolveOrBindTexture/TextureColor ~0 in a per-PC profile of 820-868); r49 face/body order checked by capture (owner as oracle).
4. Kernel to all four measured kinds on adopted packets (DK cross slots and source seam are already Q43.20; Link texgen via ported PatchTexgen; Kirby programs), 2 entries per
   instance, tuple incl. programs/detail/skeleton; texture-site patch replaces the identity guard; oracle 0 per kind.
5. Generator: rebuild the FIFO emitter (GEN:9165-9245) to today's words in LOAD4x3/P' layout with patch tables (matrix, shade, texture, tint TEXIMAGE/PLTT, texgen, light); host
   word-compare vs recorded packets; runtime oracle semantic compare. Order Samus, DK, Link, Kirby (roster hats resident). Swap adoption for templates.
6. HW texgen for Link after the differ; P' + LOAD4x3 in every list; flush only patched lines.
7. Coverage: remaining kinds (skeletons for all, Fox gun sub-list, Yoshi/Ness programs, Captain per D5, accessories), then CSS/Results/intro/1P/autodemo; lean decline = packed halt.
8. Delete section 5 in one batch (incl. the shared tint table TFX:3769-3961 and gNdsR2FighterTintSetGeneration); ITCM/DTCM reassignment (kernel resident, joint scratch in
   DTCM); four-CPU stress whole match: FTR <= 90K P99, native failures 0, fighter uploads after GO 0, WORK-H falls with FTR.
