# agent-sim: DS-native simulation architecture review (Smash64DS)
Read-only review. Paths relative to D:\Stuff\DevFolder\Smash64DS_Port; D = decomp/BattleShip-main/decomp/src.
tk = timer tick = 2 ARM9 cycles. "1v1" = Aug Mario/Fox both-CPU records; "4CPU" = DK/Samus/Link/Kirby, Dream Land.
Baseline files: artifacts/performance/2026-09-22_p2-2p8-architecture-baseline/{bands_dtcm,classes,tail_symbols}.txt,
attrib_n0409.json (per-symbol self tk/fr, 129-region average).

## Q1 Hit detection (SHDT)
Order: every fighter's physics (prio 4/3) runs before any catch (prio 2) or hit search (prio 1) (D/ft/ftmanager.c:858-863).
`ftMainProcSearchHitAll` (D/ft/ftmain.c:3795) -> SearchHitFighter :2999, SearchHitItem :3424, SearchHitWeapon :3227,
SearchGroundHit :3643, ProcessHitCollisionStatsMain :2666 only if a hit was logged. Bracket: reloc_backend_ftmain_runtime.c:561-579.
Per victim x other fighter (12 pairs/tick): scan 4 attack_colls x 4 attack_records (ftmain.c:3043-3076, integer, `k==0`
early-out); clash only if both grounded (:3082-3151); per live hitbox `gmCollisionCheckFighterInFighterRange`
(D/gm/gmcollision.c:1132) = attack centre vs victim ORIGIN box `attr->hit_detect_range` (+-1200 x -600..+1200 for
DK/Samus/Link, 800/400 Kirby; relocData/213,217,225,229_*Main.c); shield sphere (:1416 -> TestSphere :780); then each live
hitbox x each hurtbox (11 DK/Samus/Link, 8 Kirby) `gmCollisionCheckFighterAttackDamageCollide` (:1379) =
`func_ovl2_800EDE00` (world chain via 800EDBA4/800ED490 + `gmCollisionSetInvertMatrix`) + `800EDE5C` (3 sqrtf) +
`gmCollisionTestRectangle` (:661: 3 fdiv radius/scale, 2 point transforms x 18 ops, swept clip loop with 2 fdiv/step).
Worst case 4x3x4x11 = 528 pair tests/tick. Latches FTParts unk_dobjtrans_0x5/6/7 (D/ft/fttypes.h:652-681) are cleared for
the whole tree at the end of each fighter's physics (ftmain.c:1847), so each ENGAGED victim rebuilds every hurtbox joint once
per tick. SCAT uses the same gateway for grabbable hurtboxes (ftmain.c:3761) and runs first, so its P99+ 95,748 is largely
the same chain (plus catch/capture status changes). Hitbox centres are made in SPHD (ftmain.c:1862-1913, parent walk).
MEASURED 1v1 (2026-08-16_shdt-mechanism/SHDT_MECHANISM.md:109-165,195-231): cost is per engaged FRAME, not per pair
(22 vs 44 pairs: per-joint symbols flat, OLS -440 tk/pair R2=0.003); 102,988 tk per engaged frame / 13.96 joints =
7,377 tk per joint; 800ED490 = exactly 63 soft-float calls, inverse 61, E5C 18; 32.7 cycles per helper call; 93.5% real work.
Pair-level reject refuted (>=97.09% of pair evaluations exit before geometry: 2026-08-13_shdt-broadphase/REFUTED_PAIR_REJECT.md:94-106);
chain-sum reach bound WouldSkip 0 of 2,373 (docs/archive/P1_EXECUTION_BOARD.md:2427-2432).
ESTIMATE 4CPU: engaged victim-tick = 11 x 7,377 = ~81K tk. P99+ SHDT 416,080 (MEASURED) = ~5.1 engaged victim-ticks per
presented frame of 8 possible; median band 30,319 = ~0.4. Why: in a 4-way brawl almost every live hitbox sits inside some
victim's +-1200 box, and each engagement pays that victim's whole soft-float chain+inverse; the hit then co-fires
effects/SFX (SHDT), the damage status change (SPRM) and asset I/O (SHDT_MECHANISM.md:253-266).
Connect path: ftMainUpdateDamageStatFighter :2146 (interact records, captured damage, damage queue/lag :2119, hit log <=10,
stats + stale queue, resisted-hit effect, ftMainPlayHitSFX :2107); ProcessHitCollisionStatsMain :2666 (per log entry
ftParamGetCommonKnockback + impact midpoint + element effects; max-knockback winner sets angle/element/lr/joint/index,
hitlag_mul); hitlag/status in SPRM (ftMainProcParams :3856-4000).
Prior art (R2-07, 2026-08-15_cfx-narrow-exchange/EXCHANGE.md:11-36,189-199,247-279): fixed producers exchange 1.001, fixed
consumer 2.68 (libgcc bit-loop 64-bit divide 4x/entry; cold I-cache at 0.97 entries/frame; f32 LoadF32 edges), 2.19-2.08 even
with HW divide (2026-08-16_hwmath-units/HWMATH.md:40-45); decisions 1,938/1,938 identical, 20/20 hits. The soft-float
library is hot at ~1,545 calls/frame, a rare kernel is cold: a native kernel must be BATCHED per tick and ARM/ITCM.
KERNEL SPEC: per tick, after the last physics proc, one pass over the engaged-victim set (the source's own early-outs
decide membership, so no new reject rule is needed for exactness):
- per hurtbox joint: world basis Q30 s32 + translation Q20 s64 (the renderer's proven SMULL form,
  renderer_adapter_matrix.c:6564-6573; Q12 is too coarse: ESTIMATE 2000/4096 = ~0.5 unit per level); inverse = cofactor with HW
  divider (or R^T*S^-2 when a precomputed per-joint flag says no shear); vec_scale via HW isqrt; 48 B/joint (~2 KB/4 fighters).
- per hitbox: pos_curr/pos_prev/radius Q20, state, swept AABB. Narrow phase: 2 point transforms (18 SMLAL) + slab/clip on
  integers; divides only in clip steps.
- ESTIMATE warm cost ~600 cycles/joint vs 14,754 today (~25x) -> P99+ SHDT ~40-60K (pair tests + hit consequences remain).
- EXACT: hit/no hit, shield, clash, FIRST hurtbox in array order (loop breaks :3209-3211), hit-log order, knockback winner
  (strict `<` :2845), all integer operands. TOLERANT: impact/effect positions. Rule: kernel answers only when |slab margin| >
  analytic bound B(depth); inside B it builds the float chain for that joint and calls the decomp test (exact by construction).
- Shadow oracle (lab flag, same ROM, one .data word like gNdsCfxNarrowEnable, EXCHANGE.md:38-42): both paths every pair;
  counters pairs/hits/guard-fallbacks/flips/max|dev| + first flip (fighter, joint, tick); gate = flips 0 and whole-match
  witnesses (damage, stocks, hit count, status histogram) equal to the control arm.

## Q2 Status change (SPRM/SINT)
SPRM = ftMainProcParams only (reloc_backend_ftmain_runtime.c:581-600): its tail is the victim's damage status change
(ftParamUpdateDamage, ftCommonDamage*, ftParamSetDamageShuffle, hitlag, rumble: ftmain.c:3856-4000). Attacker/AI SetStatus is in SINT.
`ftMainSetStatus` (ftmain.c:4365-4823): forward-effect events + proc_accessory :4397; detail swap :4408; ClearAttackCollAll :4414;
hitstatus resets :4422; ResetFighterDamageCollsAll :4433; model/texture part resets :4437; colanim :4445; ProcStopEffect :4449;
3x rumble stop + TopN rot reset :4468; StopLoopSFX :4529; MoveDLLink :4544; status-desc index :4550-4579; SetMotionID/
SetStatUpdate stale queue :4581; proc_status :4594; FIGATREE ACQUIRE `lbRelocGetForceExternHeapFile` :4621; hidden-part
add/eject/materialise (gcAddDObjForGObj + MObj) :4638-4652,4085-4200; TRS reset of every joint :4653-4703; figatree bind
`lbCommonAddFighterPartsFigatree` :4704; TransN reparent + animlocks :4710-4735; script ptr :4746-4777; FIRST PLAY
`ftMainPlayAnimEvents{All,Forward}` + colanim :4787-4795; proc installs :4801-4822.
Port adds (src/import/battleship_ftmain.c:191-236): pre-hook, flat-walk cache flush, renderer status-cache invalidate (next
draw re-PRODUCES fighter packets, ~75K of the P99 excess, baseline README.md sec.3), force-file resolve. Acquisition
(reloc_backend_assets.c:15219-15297 -> :14889): token->asset id; `ndsRelocPrepareFighterAnimHeapOverwrite` (:6908: linear
scan+memmove of the loaded-file table, `ndsAObjEvent32ForgetRange` linear over a ledger with high-water 1,266
(battleship_sys_objanim.c:1381-1422; EXCHANGE.md:236), alias purge); anim-cache memcpy of the clip + register + finalize,
or NitroFS/FatFs read + swap + fixups on a miss. 4-kind matches can DECLINE the cache arena for RAM (reloc_backend_assets.c:13510-13523).
MEASURED P99-class excess over median (tail_symbols.txt): ndsFtPoseParse +20,840, get_fat +17,782, f_lseek +11,081,
ndsFtPosePlay +10,806, memcpy +10,650, armCopyMem32 +10,284, AnimAssetIDForToken +9,490, NativeAssetAddress +9,014,
ForgetRange +8,960, BindEntry +6,069, PreviewFileOffset +5,931, battleship_ftMainSetStatus +5,524 (acquire/ledger/I-O ~83K).
1v1 precedent (2026-08-16_sitr-attach-lane/ATTACH_LANE.md:20-45): parse 28,094 + evaluate 26,813 + attach 23,801 tk per
transition frame; the evaluate half is SetStatus's own first play = gameplay (pose/collision on the transition tick). Keep it.
PRECOMPUTE per (kind, motion_id) at load: resident clip pointer (normalised, position-independent stream form), anim_desc +
hidden-part delta mask, pre-parsed bind (track slot map + frame-0 event block), absolute event pointer, shieldpose/animlock/
transN/xrot/yrot flags, hurtbox-override list, per-motion hurtbox bound, pre-resolved effect/SFX descriptors.
DYNAMIC: stale queue, proc_status, rumble, hitstatus, forward seek (frame_begin != 0), colanim, first play. Hidden-part DObjs:
pre-create per fighter at load and relink (item-heavy joint is read by capture: battleship_ftmain.c:207-213).
RECORD 16 B `{clip*, events*, u32 anim_desc, s16 hurt_r, u8 flags, u8 ovr}`; ESTIMATE 160-250 motions/kind (DK anim files
800-952+381-392, Samus 953-1102+393-403) -> <=16 KB for 4 kinds. Leaner: rewrite FTMotionDesc {anim_file_id, offset,
anim_desc} in place with tagged resident pointers so decomp ftMainSetStatus runs unchanged and the port's force-load becomes a
tag test (0 B extra). BINDING CONSTRAINT IS RAM: the Mario/Fox clip pack alone is 287,936 B (reloc_backend_assets.c:13633-13640);
heap low-water 111,200 B (docs/P2_EXECUTION_BOARD.md:49) above a 25,600 B GObj latch (Makefile:2190) = ~85 KB headroom;
4 full kinds (~4x144 KB ESTIMATE) do not fit today's format -> compact clips + reachable-motion closure + kind dedupe first.

## Q3 Motion events
ftMainUpdateMotionEventsAll (ftmain.c:697), ...Forward (:755, seek: skips hitbox/effect/SFX), ...ForwardEffect (:852, end of
physics :1851: control + effect opcodes only); opcodes in ftMainParseMotionEvent (:152-692): hitbox create :193-267
(ftParamGetStaledDamage, ftParamGetJointID, record copy), effects :422-450 (ftParamMakeEffect -> joint world pos,
D/ft/ftparam.c:1799,1890), SFX/voice :344-398, flags/hurtbox/model/texture/colanim/rumble. Clock is float
(`script_wait -= anim_speed`, F32_MAX sentinel :709-731): keep it float-exact (non-integer anim_speed exists).
MEASURED interpreter self: MotionEventsAll 2,082 + ForwardEffect 1,546 tk/fr. Generated C saves <=~4K/fr: NOT worth it.
Worth it: pre-resolved operands (joint index, effect/SFX descriptors, absolute goto/subroutine targets) and O(1) callees:
pooled pre-initialised effect records; resident hit/voice SFX (`ndsAudioFgmPlayAtPan` 16x on engaged frames, SHDT_MECHANISM.md:262).

## Q4 Map collision
Source: `mpProcessUpdateMain` (D/mp/mpprocess.c:399-464) runs proc_coll `diff/250+1` times per tick (up to ~10 under
knockback = the SPHD tail, +98K P99+); grounded proc = L/R wall sweeps, floor-new, edge adjust, floor re-test
(D/mp/mpcommon.c:163-219) + slope contour, speed line, exist-line. CPU AI adds `func_ovl2_800F8FFC` (one FCCommonFloor per
floor line) per target per CPU (D/ft/ftcomputer.c:3744).
Port (src/port/reloc_backend_mp_collision.c): endpoints :995-1166, kind :830-900, yakumono :1174-1260 are already per-line memo
HITS; f32 vertex memo; extent rejects :700-768. What remains is per-QUERY overhead: `ndsStageCollisionLoopGeometryReady`
(6 pointer tests, :24-47) on every query, route tests, O2R halfword reads, lab proof-enable calls inside each floor query
(:1552,:1655,:1677), soft-float compares/interp. MEASURED: FCCommonFloor 818 cyc x 45,372 calls/match and FindLineEndpoints
543 cyc x 38,890 (1v1; docs/optimization/review/15_DREAM_LAND_AOT_COLLISION_LOOKUP_TABLES.md:30-40); 4CPU MAP_COLLISION
97,128 tk/fr and FLAT in the tail (99,778 med vs 99,233 tail, classes.txt): a median cost, not a tail driver.
Lines per stage (ESTIMATE from (line_info - vertex_links)/4 in relocData/1xx_Stage*File2.c): Dream Land 7 (MEASURED 7,
docs/optimization/archive/P1_EXECUTION_BOARD_pre-cycle79.md:7394), Zebes 5, Jungle 5, Sector 9, Yoster 10, Hyrule 15, Castle 16,
Yamabuki 16, Inishie 19; <=44 vertex ids. Spatial bins buy nothing; per-query overhead is the cost.
DESIGN: build-time per-stage tables indexed by line id: {kind, yakumono, flags, seg0, nseg, x/y extents s16}; per segment
{x1,y1,x2,y2,dx,dy s16, class H/V/general, normal as exact f32 bits}; per-kind lists; edge adjacency. One validity check per
scene, one struct load per query. Platforms: keep the source convention (query point minus yakumono translate,
reloc_backend_mp_collision.c:1592-1597) with a per-tick yakumono snapshot. Memo `func_ovl2_800F8FFC(pos)` by (pos bits,
yakumono generation): pure function, 3 AI askers share one answer.
ESTIMATE: bit-exact float (board: map collision FROZEN, exact only) removes overhead only, ~40-50K tk/fr; a proven
fixed-point core (integer vertices, guard band on floor-distance sign) ~70-80K tk/fr. Both median savings.

## Q5 Joint transforms
Gameplay producers and cadence: (a) hitbox centres, every live hitbox every tick (ftmain.c:1882,1907 -> gmcollision.c:491-529:
parent walk, TransformMatrixAll local = 6 trig + 21 ops when transform_update_mode==0, 18-op transform per level);
(b) hurtbox chains per engaged victim-tick (Q1); (c) effects/spawns: ftParamMakeEffect (ftparam.c:1795,1890), special-N
spawn joints (ftlinkspecialn.c:37, ftsamusspecialn.c:37, ...), item throw/drop (it/itmain.c:330,458), catch
(ftcommoncatch2.c:51), capture (ftcommoncapturepulled.c:31, ftcommoncapturecaptain.c:20), shield (YRotN, gmcollision.c:1945),
Link afterimage (ftmain.c:4029); (d) draw-time attach matrices read the collision world (D/lb/lbcommon.c:1452-1625,1951-2018).
The AI's hurtbox read (ftcomputer.c:7944-8005) runs at SETUP only (ftmanager.c:903): the joint-cap abort was a setup NULL.
Relevant joints: hurtboxes DK{5,6,8,9,12,14,15,20,21,25,26} Samus{5,6,8,9,13,15,16,27,28,32,33} Link{5,6,8,9,13,14,23,26,27,31,32}
Kirby{5,6,10,11,15,16,24,29}; effect joints DK{12,15,21,26,9} Samus{13,16,28,33,9} Link{23,14,27,32,9} Kirby{6,16,22,27,11}
(relocData *Main.c damage_coll_descs/effect_joint_ids); + hand/item joints, per-motion hitbox joints (from events, bind at
load), ancestors, joints 0-3. ESTIMATE 15-20 of ~27-37 joints per fighter.
CRITICAL: the renderer composes from 16.16-quantised N64 locals into Q43.20 worlds on purpose, and FTParts::mtx_translate
"was tested and is observably different" (renderer_adapter_matrix.c:6550-6573). One world matrix cannot serve both without
breaking the render contract or moving collision off the float source. Share LOCAL TRS (the pose engine's Q12 output) +
topology; compose per consumer (docs/optimization/SRC.md:221-227). MEASURED today: POSE ndsFtPosePlay 35,988 + Parse 21,452 +
Update 17,190; renderer matrix stack ~100K (MtxMulAffine20p12 25,410, MtxMul20p12 17,595, BuildDObjXObjMatrix 16,442,
ComposeOwnerWorlds 13,258, SourceWorldMulLocal 9,722, BuildDObjLocalMatrix 7,754); only the local build is shareable.
Cost/saving ESTIMATE: batched fixed pass for all 4 fighters' hurtbox chains ~11 x 600 cyc x 4 = 26K cyc/tick = ~26K tk per
presented frame always-on, against SHDT+SCAT P99+ ~510K; lazy-but-batched (engaged set only) keeps P50 flat. Exact 60 Hz
hurtboxes need per-tick evaluation of the gameplay subset (today body poses are held on the non-final tick and "body hurtboxes
read one tick stale", include/nds/nds_ft_pose.h:31-42): ESTIMATE +15-20K tk/fr POSE (half of ndsFtPosePlay 35,988) and it
changes the current ROM's fights toward source: owner call.
Procedural writes (all end in the source's own ftParamsUpdateFighterPartsTransform[All], which the port owns at
reloc_backend_compat_shims.c:3145-3195 - key the authority's dirtiness there): TopN rot.y = lr*90 and slope rot.x
(ftmain.c:4477-4481,654); guard YRotN/XRotN (ftcommonguard1.c:256,358); damage joint 4 (ftcommondamage.c:210); Fox/Ness up-B
aim (ftfoxspecialhi.c:143, ftnessspecialhi.c:519); Pikachu up-B (ftpikachuspecialhi.c:159); animlocks with NCS scale
compensation (gmcollision.c:82-193, ftparam.c:2352-2418); translate_scales (ftparam.c:377-392); hidden-part relink/TransN
reparent (ftmain.c:4638-4722); DK cargo/capture (ftdonkeythrowfdamage.c:50); per-kind size (Giant DK).

## Q6 Camera
Gameplay consumers (grep of ft/ it/ wp/ gr/ for gGMCameraGObj/gGMCameraMatrix/gGMCameraStruct): wplinkboomerang.c:104-107
(projects the boomerang through gGMCameraMatrix + viewport every 9th tick; off-screen -> return), itstar.c:88-97 (spawn
velocity sign from cobj->vec.at.x), ftcommondead.c:440-447 (DeadUp position from eye). Visual only: ftdisplaymain.c:1095-1136,
if/ifcommon.c:1839,1904, ef/efmanager.c:3775, gr/grwallpaper.c:60,201.
FINDING: gGMCameraMatrix is written only by gmCameraLookAtFuncMatrix at DRAW (gmcamera.c:985-1018; port
src/import/battleship_gmcamera.c:1092-1164), and draw runs once per presented frame (taskman_seam_battle_host.c:738-742), so
the boomerang test on the 2nd tick reads a matrix one tick older than source (source draws every tick). The port comment
"There is no simulation reader" (battleship_gmcamera.c:1085-1091) is stale since Link weapons were imported
(src/import/battleship_link_weapons.c:43-93); the fixed camera path's Q->f32 publish (:1092-1098) is therefore gameplay-visible.
DECISION: keep func_camera (interest/zoom smoothing -> eye/at) at 60 Hz; produce gGMCameraMatrix per tick as a gameplay product
(float-exact lookat x persp, ~one 4x4 compose; ESTIMATE <5K tk/fr vs CAMERA 46,410 now) only while a reader can fire (live
boomerang); GX upload once per presented frame.

## Q7 CPU AI
SCPU median 77,849, P99+ band 65,852 (flat): a median cost; its bucket P99 196,608 falls on other frames (bands_dtcm.txt).
Hot self (attrib_n0409.json): CheckDetectTarget 4,537, CheckFindTarget 3,447, GetObjectiveStatus 2,801 (CPI 9.4),
UpdateInputs 2,436, FollowObjectiveWalk 2,320, CheckEvadeDistance 2,130, ProcessAll 1,307+1,146; FT_AI 28,931 self, the rest
is soft-float and map queries charged elsewhere. Decisions gated by input_wait (ftcomputer.c:7812-7817); FindTarget has 9 call
sites in decision paths (:5975, :6094, :6322, :6638, :6673, :6726, :6799, :6849, :7541), some twice per decision tick.
Exact shared perception: (1) memo `func_ovl2_800F8FFC(target TopN)` by (x,y bits, stage-dynamic generation) (FindTarget
:3744; Yamabuki :3959-3966); (2) per-fighter "targetable" facts (status >= Wait, bounds, cliff status: :3739-3764) keyed the
same way; (3) pair (dx,dy,dx^2+dy^2) only when computed with the identical expression order (:3766); EvadeDistance uses a
velocity-predicted target (:3817-3819), not shareable. Never cache across syUtilsRandFloat calls (:3971, :3685-3710,
:3884) and never reorder them. ESTIMATE <=10-20K tk/fr, mostly map-query work removed from FindTarget.

## Q8 Port-only machinery (MEASURED avg tk/fr, attrib_n0409.json)
- ndsFTParamsInvalidateSubtree 20,349: re-flattens invalidated joint subtrees to clear latches; 4-slot cache misses 49.7%
  (SRC.md:339). Protects the lazy world/inverse/scale latches. A per-tick authority with per-fighter epoch + preorder subtree
  interval (SRC.md:356-375) makes invalidation O(1) - but only once EVERY latch reader migrates (risk 3).
- ndsRelocGetFileData 10,039 (per call: loaded-file lookup + symbol resolve + preview span scan, reloc_backend_assets.c:15595-15626):
  pre-bind pointers at load.
- ftGetStruct 9,983 (ITCM function with harness-provenance checks, reloc_backend_compat_shims.c:12172-12215; 246 calls/frame
  1v1): back to the source macro (D/ft/fighter.h:21) once bridge/stub structs are gone.
- ndsStageCollisionLoopGeometryReady 9,223: scene-time validity.
- Acquisition/ledger: AnimAssetIDForToken 3,399, ForgetRange 2,861, NativeAssetAddress 1,810, FindLoadedFileContaining 1,174
  (+~33K more in tail frames): deleted by pre-bound motions.
- Lab hooks on hot paths: stage proof-enable calls per floor query, SearchHitFighter shield-proof rescan
  (reloc_backend_ftmain_runtime.c:441-508), pose-cap volatile in ndsFtPoseRun (SRC.md:434-436): compile out of production.
ESTIMATE ~58K tk/fr average + ~40K extra in tail frames is machinery the native design removes outright.

## TOP RISKS (gameplay equivalence)
1. Boundary flips in fixed narrow phase/clip; Q12 is too coarse -> Q30/Q20-s64 + guard band + float fallback + flip oracle = 0.
2. Order semantics: first hurtbox in array order wins (ftmain.c:3197-3213), hit log order (<=10), strict-< knockback winner
   (:2845), attack-record group logic (:3055-3071); a symmetric/"best hit" kernel changes damage_joint_id -> reactions.
3. Latch readers outside SHDT (lbcommon attach, efmanager, battleship_fox_blaster.c:248, renderer_adapter_matrix.c:2898-3052,
   capture, afterimage): an authority must publish identical values into FTParts or migrate all readers at once.
4. Pose-hold policy: current ROM uses 1-tick-stale body hurtboxes on the held tick; "exact 60 Hz" changes today's fights.
5. RAM for resident motion banks (Q2); a bank that silently falls back to I/O keeps the tail.
6. Camera matrix is already 30 Hz for the boomerang (Q6): fix first or the oracle flags it forever.
7. Map collision numerics frozen; substep count is float (mpprocess.c:423-440).
8. Motion-event clocks are float; compiled events must keep them.
9. I-cache: rare fixed kernels lose (EXCHANGE.md:274-279); ITCM is full (baseline README sec.4) -> batch per tick.
10. RNG order (AI :3971 etc.; effect scatter) - no caching across RNG calls, no reordering of effect spawns.

## RECOMMENDED FIRST SLICE
"Batched hurtbox frames + guarded fixed narrow phase", fighter-vs-fighter damage/shield/catch only.
1. Hook: the two gateways already wrapped with zero in-TU callers (battleship_gmcollision.c:162-195, 319-403). On first entry
   for a victim in a tick, prepare ALL that victim's hurtbox joints in one ARM pass (Q30 basis / Q20 s64 translation, HW
   divide + isqrt via NDS_R2_CFX_HWMATH), reading pose-engine Q12 locals where owned (no f32 edge) into a PORT-SIDE frame
   store. Do NOT set the FTParts latches/fields (unlike the ring contract, :323-331): capture (ftcommoncapturepulled.c:31),
   afterimage, effects and draw attach keep computing the float chain, so nothing outside the decision can drift.
2. Narrow phase in fixed with margin; |margin| <= B -> decomp float body (unchanged). B derived per chain depth, reported.
3. Harness: same-ROM .data arm word (fixed off/on/shadow); shadow runs both per pair: counters pairs, fixed answers, guard
   fallbacks, flips, max matrix deviation, first flip. Accept: flips 0 on 4CPU stress + 1v1 matches, whole-match witnesses equal,
   SHDT/SCAT P99+ measured on the tick-HUD whole-match instrument (not a 128-frame window).
4. Expected (ESTIMATE): SHDT P99+ 416K -> ~60-100K, SCAT P99+ 96K -> ~20K; P50 within noise (engaged set only).
Fallback if the guard band misbehaves: bit-exact incremental float chain - reuse each joint's world rotation block, inverse
cofactors and vec_scale when all ancestor local rotations are bit-identical to last tick (held pose/hitlag), recompute only
translation columns (18+18 ops vs ~180); provable by per-joint bit compare against full recompute; ESTIMATE 25-40% of the chain.
SPRM slice 2: in-place tagged FTMotionDesc for motions whose clips are resident (stream form, no copy/ledger/I-O); exact by
construction (same bytes; existing NDS_FT_POSE_ORACLE, nds_ft_pose.h:44-48).
