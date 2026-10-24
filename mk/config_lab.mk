# Lab and runtime knobs: the renderer, collision, simulation and profiling
# switches (NDS_R2_*, NDS_TASK*_*), most of them off in every shipping and
# gate configuration; the intro bake stamp.
# Included in place from the Makefile (P8 G); its text is unchanged.

NDS_OWNER_LINK_INTRO_STAMP := $(PROJECT_ROOT)/src/nds/generated/.owner_link_intro
$(shell mkdir -p "$(dir $(NDS_OWNER_LINK_INTRO_STAMP))"; [ "$$(cat "$(NDS_OWNER_LINK_INTRO_STAMP)" 2>/dev/null)" = "$(NDS_OWNER_LINK_INTRO)" ] || echo "$(NDS_OWNER_LINK_INTRO)" > "$(NDS_OWNER_LINK_INTRO_STAMP)")
# PROBE BUILDS ONLY. 1 makes the reason-20 preview halt count instead of
# spinning forever, so the shell walk can cross a CSS where one fighter's
# preview has no native owner (measured 2026-09-22: kind 11, Ness). Never 1 in
# anything published -- that halt is deliberate, and a silent decline is
# precisely the failure it exists to expose.
NDS_PREVIEW_HALT_NONFATAL ?= 0
ifneq ($(filter 0 1,$(NDS_PREVIEW_HALT_NONFATAL)),$(NDS_PREVIEW_HALT_NONFATAL))
$(error NDS_PREVIEW_HALT_NONFATAL must be 0 or 1)
endif
# Runtime 2 (docs/Smash64DS_Runtime2_SwitchPlan.md). The whole family defaults
# to 0 and the published ROMs stay pure Runtime 1 until the switch (plan S5).
#
# R2-01. Selects the src/nds/r2 battle path: the same 60 Hz gameplay tick and
# the same Runtime 1 renderer, driven by a loop specialized for the Boundary
# configuration instead of the NDS_DEV_SCENE_HARNESS chain's runtime flags.
# Only meaningful with the battle_playable harness; the C fails the build closed
# on any other. Setting this to 1 also gives the shared taskman_seam helpers
# external linkage, so the 0 arm stays byte-identical to a build without the
# family at all.
NDS_R2_PATH ?= 0
# R2-02 E1a. Elides the per-frame stage PrepareRun phase and reuses the prepared
# run table, which is a pure function of the generated tables and a traversal
# state Task 44 already proves unchanged. Sized at 67,119 ticks/frame by bracket
# attribution, realized 94,784 on STG P50 over 128/128 frames. Independent of
# NDS_R2_PATH -- it is a renderer change, measurable on either battle path.
NDS_R2_STAGE_DIRECT ?= 0
# R2-03 E1. Replaces newlib's sqrtf with a correctly-rounded one built on the DS
# hardware square-root unit. E0 measured __ieee754_sqrtf at 223.1 ticks per call
# and 14,258 ticks/frame. Bit-exact by construction, so the Task 37 state hash
# must not move -- sqrtf is on the gameplay path and a last-bit difference is a
# pose difference.
#
# R2-07 L10 turned it ON. E1 measured -6,040 ticks/frame, bit-exact, Boundary
# green, and then left the flag at 0 -- it read as a mean-only lever because the
# 8-frame A/B was flat inside the placement floor. L6's over-gate split says
# otherwise: __ieee754_sqrtf runs 87 times on an over-gate frame against 26 on a
# clean one (3.34x) for a 26,007-cycle delta, so the saving concentrates on
# exactly the frames that miss the gate. That makes it a P95 lever, and P95 is
# what the gate is. scripts/check-r2-fixed-sqrt.ps1: 12,807,569 inputs,
# 8,775,610 handled, 0 mismatches.
NDS_R2_FIXED_SQRT ?= 1
# R2-02 E2. Pushes the Task 36 rigid-replay command stream to the GX with GXFIFO
# DMA instead of a CPU store loop. ~4,200 words/frame at Task 103's measured
# 9.51 ticks/word; the point is the ~16 KB/frame of cache-line fills that stop
# happening, which is how the switch plan's §3.3 says traffic work is judged.
NDS_R2_STAGE_DMA ?= 0
# R2-02 E7. Composes the sixteen dynamic stage bindings from one hoisted
# view-projection instead of rebuilding the camera operands per binding. The old
# path cost 54,901 ticks/frame, 44.6% of the stage preflight, and the arithmetic
# was never the bulk of it: per binding it ran the camera-cache lookup and three
# 64-byte MTXCOPY memcpys to produce operands identical for all sixteen.
# STG P50 224,320 -> 212,480, P95 232,640 -> 219,072.
#
# Exact, not approximate. For the battle camera BuildCameraMatrices leaves
# modelview_valid FALSE and returns projection = MtxMul(lookat, persp), so the
# old compose was already world x (lookat x persp) -- one multiply, not two --
# and BuildTask36StageCameraMatrices derives its two halves from the same
# syMatrixLookAtReflect/syMatrixPerspFast calls, so the hoisted product is
# bit-identical rather than reassociated. Proven against the pre-E7 arm:
# binding_composed[] identical for all 42 bindings at frames
# 260/420/500/700/1100/1700.
NDS_R2_STAGE_VIEWPROJ ?= 0
# R2-02 E8, and the switch plan's §7 read literally: "no generic preflight, no
# stats temporaries". For the five stage segments the Task 36 replay does not
# serve, the owner preflight clears a 1,292-byte NDSRendererStats, initialises a
# traversal state, and replays 21 run-level and 16 binding-level state spans --
# 13,565 ticks/frame for the clears alone and 20,370 for the run spans -- to
# produce a preflight_stats and a traversal state that, once E1a's prepared run
# table is valid, nothing reads. The single member that outlives the loop is
# sync_command_count, which this memoises alongside epoch_mask.
# Requires NDS_R2_STAGE_DIRECT (it is that memo's guard) and HW_COMPOSE 2.
NDS_R2_STAGE_PREFLIGHT ?= 0
# R2-03 E1 falsifier, lab only. Hashes the inputs and the outputs of the fighter
# shade loop once a frame and counts the frames each changes on. 48,422
# ticks/frame re-light 541 dense vertices; this says whether they need to.
NDS_R2_FIGHTER_SHADE_PROOF ?= 0
# R2-03 E5 falsifier, lab only. Hashes the facts PrepareProductionRun computes
# per run and counts the frames they change on. Section 7 for this phase asks
# for a submit consuming only baked facts -- "no PrepareProductionRun policy
# re-checks, no per-frame texture identity proof" -- which is R2-02 E1a's cut
# (94,784) moved to a table of the same shape. Three hashes, because they imply
# different cuts: STABLE is the bakeable candidate, MATERIAL is the live colour
# alone, and FULL adds the texture cache pointer -- a hardware-cache pointer
# moving is a different verdict from the drawn facts moving.
# Hooked in the production path, not the hierarchy preflight: canonical mode 9
# never writes hierarchy_runs[], so that hook measured nothing. Read
# gNdsR2RunCallCount first; zero means the instrument did not run.
NDS_R2_FIGHTER_RUN_PROOF ?= 0
# R2-03 E9. The fighter-parts matrix path converts float -> N64 16.16 -> DS
# 20.12, and E8 established 97.5% of local-matrix builds take it. The
# intermediate is a lossless round trip, so the direct conversion is bit-exact
# by construction, not by tolerance.
#   1 = direct conversion.
#   2 = run both and compare, because "by construction" is the kind of claim
#       E8 proved gets read wrong.
# Promote 2 -> 1 only on a zero gNdsR2MtxDirectVerifyFail run.
NDS_R2_FIGHTER_MTX_DIRECT ?= 0
# R2-03 E12. E5 proved PrepareProductionRun is a pure function of run_index and
# E11 measured the cost: 45.3 of 60.9 calls a frame re-resolve a texture whose
# identity never changes, at 1,013 ticks each, because the caller resets
# texture_prepare_valid per DObj. The memo caches that identity per run and skips
# SyncTextureTile, the key build, its hash and the lookup -- but still binds,
# still refreshes the eviction LRU, and revalidates residency, because E5
# deliberately excluded the cache entry pointer from its stable set.
#   1 = memo live.
#   2 = memo never taken; the full path runs and the memo is asked whether it
#       would have disagreed. E8's key was incomplete three times running and a
#       verify arm caught it each time.
# Promote 2 -> 1 only on a zero gNdsR2TexMemoVerifyFail run.
NDS_R2_FIGHTER_RUN_MEMO ?= 0
# P2-2 fighter packet. Captures each fighter's whole GX command stream once as
# packed GXFIFO words (in the battle-idle gSYFramebufferSets) and replays it by
# DMA every frame, patching only the projection, the joint-chain matrices and
# the light vector. The four-CPU stress arm measured the four fighter draws at
# 607,040 of a 1,600,832-tick median frame (NDS_R2_DRAW_SUPPRESS_MASK=15 A/B,
# 2026-08-23); this is the structural cut of that lane. Promoted to the default
# the same day after four gate-green stress runs (WORK-H P50/P95 1,600,832 /
# 2,069,824 -> 1,281,728 / 1,866,432), pixel-identical entry-series captures
# and a green shell loop; `NDS_R2_FIGHTER_PACKET=0` on the command line is the
# control arm. Compiled out by itself where its preconditions are missing
# (software triangles, profile level 2, no GX compose / HW matrices / HW light).
NDS_R2_FIGHTER_PACKET ?= 1
# R2-03 E17. Loads the fighter's projection and modelview separately and lets the
# geometry engine perform the multiply, instead of composing on the CPU and
# loading the product. Measured -17,600 FTR P50 / -18,560 WORK P50, and it leaves
# the modelview alone in the vector matrix rather than the composed MVP, which is
# what hardware lighting needs. Rendering-side -- positions now round in hardware
# -- so it gates on a screenshot pair plus the owner's approval.
NDS_R2_FIGHTER_HW_MTX ?= 0
# Slice 43. E17 moved the modelview x projection multiply onto the geometry
# engine; this moves the JOINT chain there too. The c119 attribution puts 52.5
# ndsRendererMtxMulAffine20p12 calls a frame, 18,560 ticks on 80/80 tail frames,
# in one loop -- ndsRendererAdapterComposeOwnerWorldsFlat -- and the composed
# world has exactly one consumer, the GL_MODELVIEW load. The production root loop
# already stores per-root matrices to the GX palette and binding_parents[i] < i
# is preorder, so RESTORE(parent) / MTX_MULT_4x4(chain) / STORE(i) rebuilds the
# tree in the loop's own order with the palette as the parent store.
# Requires NDS_R2_FIGHTER_HW_MTX.
#   0 = CPU compose (control arm).
#   1 = GX compose (shipped; owner-accepted 2026-08-15).
#   2 = GX compose AND keep the CPU compose, comparing the two per binding.
#       Costs more than either arm; it exists because "same order, same operands"
#       is exactly the claim E8 proved gets read wrong.
# Promote 2 -> 1 only on a zero gNdsR2GxComposeVerifyFail run. That verification,
# the full-match stack fix, and the owner's matched-tic visual acceptance are now
# complete, so the accepted route is the default rather than a lab-only bank.
NDS_R2_FIGHTER_GX_COMPOSE ?= 1
NDS_R2_FIGHTER_GX_COMPOSE_REQUESTED := $(NDS_R2_FIGHTER_GX_COMPOSE)
# Explicit controls require this escape and a separate lab output directory.
NDS_R2_FIGHTER_GX_COMPOSE_LAB ?= 0
# Slice 44. Round-robins the stage's per-frame transform revalidation over N
# frames instead of running the whole sweep every frame. Requires
# NDS_TASK36_HW_COMPOSE and NDS_TASK44_STAGE_STEADY.
#
# What it deletes, measured on the c120 profile (1600-frame both-CPU gate):
# ndsRendererAdapterStageWorldSourceKeyMatches runs 54.0x a frame for 7,719
# ticks, and ndsRendererAdapterBuildPersistentStageWorldMatrix another 9,017.
# Neither builds anything. The 26 rigid bindings are re-proved constant every
# frame and the 16 dynamic ones re-walk a parent chain whose cached answer they
# then reuse. --pc-detail says the cost is not the compare: the four hottest
# instructions are `ldrb r1,[r1,#4]` (xobj->kind, 27.6 cyc/insn),
# `ldrb r3,[r1,r3]` (22.7), `ldrb r2,[r0,r3]` (22.3) and `ldr r3,[r0,#76]`
# (dobj->vec, 17.3) -- 9.7M cycles of cold pointer chase into 42 DObjs and
# their XObjs, every frame, against a 4 KB D-cache. Making the compare cheaper
# buys nothing; the only lever is not touching the objects.
#
# Why a stride and not a deletion: the guard is what demotes a binding that
# stops being rigid, and cycles 52 and the 0x4F/0x50 kinds each cost a build
# proving that a frozen matrix is a real fidelity bug. A stride keeps every
# binding checked -- just not all of them on the same frame -- so a binding
# that does move is still caught, within N frames, and the mask still drops.
# On Dream Land it never fires: gNdsRendererTask36RigidConstancyMismatchCount
# is 0 over 1600 frames and R2-02 arm C found all 16 dynamic source keys
# matching for the whole match.
#
# Why round-robin and not "full sweep every Nth frame": at N=8 the second shape
# makes 12.5% of frames expensive, and P95 would land on one of them and read
# flat. Spreading 42/N checks across every frame is what moves P50 and P95
# together. See memory cluster-where-the-percentile-lives.
#   0 = validate everything every frame (shipped before slice 44).
#   N = each binding is revalidated once every N frames.
NDS_R2_STAGE_VALIDATE_STRIDE ?= 0
# R2-03 E16. Requires NDS_R2_FIGHTER_HW_MTX (E17), which is what puts the
# modelview rather than the composed MVP into the vector matrix. Moves the
# fighter's per-vertex lighting onto the DS geometry engine: a load-time
# GFX_NORMAL table, one GFX_DIFFUSE_AMBIENT per epoch carrying the source light
# colours folded with the material and the damage flash, GFX_NORMAL instead of
# GFX_COLOR in the emit, and POLY_FORMAT_LIGHT0. Rendering-side -- the DS light
# model is RGB15 throughout where the software path kept an RGB8 intermediate --
# so it gates on a screenshot pair plus the owner's approval.
NDS_R2_FIGHTER_HW_LIGHT ?= 0
# R2-03 E32. Folds SSB64's hitlag shuffle into the fighter's world matrix
# instead of switching the whole native owner off for the duration of every hit.
# The source (ftdisplaymain.c:1205) is one PUSH + syMatrixTra(x, y, 0) + POP
# around the entire fighter draw, and lbcommon.c:1627 expresses the same effect
# as an add into the world matrix's translation row -- which is exactly where
# this puts it, so it is mechanically equivalent rather than an approximation.
# E31 measured the fallback it removes at 41.9% of the P95 tail's excess.
NDS_R2_FIGHTER_SHUFFLE_FOLD ?= 0
# R2-03 E30. Draws the tick HUD's percentile block on screen. The ring is
# sampled either way, so a GDB-scripted measurement reads exactly the same data
# with this off -- and off is the configuration that resembles the published
# ROM, which has no such block. On for a device read or a screenshot.
NDS_TICK_HUD_DRAW ?= 1
# Samples per bucket in the tick HUD's ring (src/nds/nds_platform.c). The gate
# ROM takes 32 (below); scripts/sample-tick-hud-buckets.ps1 reads it from the ELF.
NDS_TICK_HUD_WINDOW ?= 128
# P2-2p8 (2026-10-04): the developer FPS console's periodic rows (FPS/UP, SLIP,
# VBI, GIT) beside it, same reasoning: the sampler reads the counters, never
# the console text (src/nds/nds_platform.c).
NDS_BATTLE_FPS_HUD_DRAW ?= 1
# P2-2p8 (2026-10-04, owner ruling D12a): boot value of gNdsTickHudSpans, the
# tick HUD's fine per-bucket span clocks (include/nds/nds_startup.h). 0 leaves
# FTR/STG/MISC and their splits at 0 and keeps the clock reads out of WORK.
NDS_TICK_HUD_SPANS_DEFAULT ?= 1
# P2-2p8 (2026-10-04, owner ruling D12a): development tallies -- gNds* counters
# that only gdb probes and verifiers read -- go through NDS_DIAG(...). 0
# compiles them out: the published ROM and the four-CPU gate, the
# configuration that ships. Every other target keeps them; a probe that needs
# them on a shipping build passes NDS_DIAG_COUNTERS=1.
NDS_DIAG_COUNTERS ?= $(if $(filter smash64ds smash64ds-p2-fourcpu-tickhud-hwtri,$(TARGET)),0,1)
# R2-03 E13. Skips a fighter's draw outright, which is how the campaign prices a
# whole fighter end to end: the phase census partitions the draw, this measures
# what the frame costs without it. Bit 0 = Mario, bit 1 = Fox. Lab only; a ROM
# built with this is not a playable configuration.
NDS_R2_DRAW_SUPPRESS_MASK ?= 0
# Owner 5 is the only census-ranked Dream Land cut that passed the canonical
# 500-pixel ratchet.  The enclosing economy flag remains off by default.
NDS_RENDERER_BENCHMARK_MODE ?= 0
NDS_SCENE_MIP_CACHE_LAB ?= 0
# R2-07 R4c. Select the NATIVE FIGHTER OWNER on the Results path, the renderer
# the match already uses. `no_oracle` is not a proof switch: it is what
# reloc_backend_renderer_dl.c:12603 tests to enter the native owner, so it picks
# the renderer (R4g). The battle present brackets its whole draw
# (reloc_backend_movement.c:13724/:13810), which is why the match gets the
# native owner -- while VS Results reached the same submit with no bracket on
# its path and got the generic DL interpreter at four times the cost.
#
# GRADUATED 2026-07-30, owner approved the fighter look on sight ("use the
# native renderer, it's already been approved"). Measured on the Results lab,
# matched source: 3.04 -> 1.04 VBlanks per source tic, 1,701,577 -> 581,197
# ticks, -1,120,380 (-65.9%), which takes Results from 1.52x OVER the 1.12M gate
# to 0.52x -- inside it. Fighter draw 1,449,776 -> 364,784.
#
# Battle is unaffected and that is measured, not assumed: the bracket saves and
# restores rather than clearing, so it is a no-op wherever the flag is already
# set, which is the whole battle path. Clean matched-window 128-sample A/B at
# -StartFrame 600: ALL p95 1,680,064 -> 1,120,384, WORK p95 1,197,760 ->
# 1,106,112, FTR p95 390,208 -> 391,040 (noise). Evidence in
# artifacts/performance/r4c-fighter-no-oracle-on-20260730.json and
# artifacts/visibility/2026-07-30_r207-r4c-*.png.
NDS_R2_FIGHTER_NO_ORACLE ?= 1
NDS_RENDERER_BATTLE_STATIC_TEXTURE_DEFAULT ?= 0
NDS_IFCOMMON_HYBRID_OAM ?= 0
NDS_DEBUG_HUD ?= 0
ifneq ($(NDS_DEBUG_HUD),0)
$(error NDS_DEBUG_HUD legacy debug wall is retired; use the focused battle phase HUD)
endif
NDS_AUDIO_FGM_ARM7_ACK_DIAGNOSTICS ?= 0
# The shipping free-play ROM carries the freeze watchdog and the exception
# report (owner, 2026-10-11: a frozen console showed nothing to photograph,
# and an out-of-memory halt spins silently); every measurement target keeps
# it off.
NDS_FREEZE_DIAGNOSTICS ?= $(if $(filter smash64ds,$(TARGET)),1,0)
# Bounded pipelining (owner 2026-10-19): a battle frame's next game logic
# may start this many scanlines before its swap retrace (131 = half a
# VBlank interval); 0 is the old loop. gNdsPresentLeadLines A/Bs at boot.
NDS_PRESENT_LEAD ?= 131
NDS_TASK9_FLOAT_CENSUS ?= 0
NDS_TASK9_FLOAT_ITCM ?= 1
# 2026-08-16 ITCM reclaim. Two of the six Task 9 libgcc members are DEAD CODE in
# ITCM: the port defines __aeabi_fcmpeq/lt/le/ge/gt/un itself, and the Task 9/16
# --redefine-sym filters below rename libgcc's copies to `*_golden` names, which
# by construction have no caller. The per-PC census agrees from the other side:
# `_arm_cmpsf2.o` (0x01ff85c4..0x01ff86d8, 276 B: __gesf2/__lesf2/__cmpsf2/
# __aeabi_cfrcmple/__aeabi_cfcmpeq + five fcmp goldens) and `_arm_unordsf2.o`
# (0x01ff86d8..0x01ff8710, 56 B) execute ZERO instructions across the whole
# 1,600-frame gate window (frames 439-2038, c200-off-pc.csv whole-match column).
# 332 bytes of zero-wait memory carrying code the match never runs.
#
# Listing a member here keeps it extracted, keeps every --redefine-sym filter,
# and keeps it in $(OFILES) -- only the --rename-section is skipped, so its code
# lands in .main. PLACEMENT ONLY: identical bytes, every symbol still defined,
# every reference still satisfied. Set empty to restore the old layout.
# 2026-08-30 ITCM rebank for the widened P2 fighter owner.  The shipping
# v4-c238 census measured _arm_fixunssfsi at only 208,842 cycles across the
# whole gate (84 B, 2,486 cycles/byte), while the native-owner/model-part
# correctness work now needs those bytes in the hard 32 KiB ITCM budget.
# This list is placement-only: the extracted libgcc member is byte-identical
# and merely retains its original .text section in main RAM.
NDS_TASK9_FLOAT_MAIN_MEMBERS ?= _arm_cmpsf2.o _arm_unordsf2.o _arm_fixunssfsi.o
NDS_TASK9_FLOAT_PHASE2 ?= 1
NDS_TASK16_FLOAT_COMPARE ?= 0
NDS_TASK16_FLOAT_I2F ?= 0
NDS_TASK16_FLOAT_ADDSUB ?= 0
NDS_TASK32_DRAW_HOT_TEXT ?= 0
NDS_FIGHTER_ANIM_AUDIT ?= 0
NDS_FIGHTER_ANIM_CYCLER_KIND ?= -1
NDS_TASK39_FX_SPRITES ?= 0
NDS_TASK39_FX_FLASH ?= 0
# R2-07: compile the original particle bytecode interpreter (lb/lbparticle.c,
# ef/efparticle.c) in place and let the real efcommon scripts run instead of the
# recoloured 16-vertex stand-ins.
#
# DEFAULT 1 as of 2026-08-01. Measured on three tick-HUD ROMs differing only in
# these flags: this one is INDISTINGUISHABLE from the control -- NO-FREEZE, a
# full match to Results, ViolationCount 0, StagePrepareBuildCount 2 -- while
# running 14 scripts and 138,274 visible particles inside its fixed pools
# (41/48 structs, 8/10 generators). SwitchPlan 3.11's no-gameplay-allocation
# clause is satisfied by measurement rather than by argument.
#
# IT IS 1,176 BYTES FROM A HARD FAILURE and that is not a figure of speech:
# ifCommonSetMaxNumGObj caps the GObj pool when the general heap drops under
# 25 KiB free and the GO countdown then dereferences a NULL, and .text costs
# taskman arena one for one here. Read `general heap free bytes` and
# sGCCommonsMaxNum on any soak after adding image.
NDS_R2_PARTICLE_RUNTIME ?= 1
# R2-07 particle DRAW. At 1 the atlas is bound and camera-facing quads are
# emitted, which is what makes the real efcommon scripts visible instead of the
# recoloured 16-vertex stand-ins six BUGS.md VFX rows describe.
#
# DEFAULT 1 as of 2026-08-01, on a measured A/B: 117,937 quads emitted with
# ZERO atlas misses, NO-FREEZE, and the stage fast path fully engaged
# (StagePrepareBuildCount 2, reuse 2,041 -- the control's own numbers). Cost is
# about 10,100 ticks (MISC P50 45,120 -> 55,232) and five extra three-VBlank
# frames; the five-plus population is unchanged at 4 of 566.
#
# Three defects are closed and recorded at their seams rather than here: a
# `glEnd()` that wedged the geometry engine (GXSTAT=0e008900 --
# check-gbi-decode-fixtures.ps1 pins that count at 1); a GO-countdown abort
# that was ifCommonSetMaxNumGObj capping the GObj pool under 25 KiB free
# (PORTING.md); and 196 five-VBlank frames caused by a 32,768-byte atlas
# starving the stage's texture resolve -- fixed by the 8,192-byte sheet, which
# is a measured hard bound, not a budget (generate_nds_particle_banks.py).
NDS_R2_PARTICLE_DRAW ?= 1
# Draw Mario's fireball as one camera-facing quad instead of interpreting its
# source model's display list. Owner-playtested and accepted 2026-08-07; ON BY
# DEFAULT. See the block comment in reloc_backend_movement.c.
#
# The asset is why this is worth trying: relocData 297's Vtx[0..3] is a single
# flat 300x300 quad and its texture is CI4 16x16 whose palette entry 0 is
# transparent, which is bit-for-bit DS GL_RGB16 + COLOR0_TRANSPARENT. So unlike
# the shield -- which had to trade index bits against alpha bits -- this bakes
# with nothing quantised, and both routes draw the same four vertices.
#
# Measured on the whole-match instrument (ROM E61D608B, 2026-08-06): one live
# fireball costs 64,700 ticks/frame at P50, MISC 33,088 of it. This flag
# addresses that half. The other half is SRC 26,752 -- wpMapTestAll running
# full stage collision for the projectile every frame -- and is a separate
# seam that this does not touch.
NDS_R2_FIREBALL_QUAD ?= 1
# The battle camera's matrix chain is Q20.12 only since 2026-10-06: the float
# arms, the lean levels (NDS_R2_CAMERA_MATRIX_LEAN, shipped level 2) and the
# route word / SELECT toggle (NDS_R2_CAMERA_FIXED, NDS_R2_CAMERA_FIXED_TOGGLE)
# were deleted. Their A/Bs: artifacts/performance/2026-08-16_camera-fixedpoint
# and the 2026-08-09 lean-level notes in battleship_gmcamera.c's history.
# Draw Fox's source blaster model as its four baked, untextured vertices instead
# of walking and decoding relocData 316's nine-command display list every
# frame. Owner-playtested and accepted 2026-08-09; ON BY DEFAULT. The source
# beam is a solid magenta quad;
# there is no texture to approximate or upload. The native arm admits only the
# exact horizontal spawn trajectory used by Fox in the P1 boundary and falls
# through to the restored source display route after a hop/reflection changes
# that contract. See reloc_backend_movement.c.
NDS_R2_FOX_BLASTER_QUAD ?= 1
# Replace EFCommon script 0x62 with its closed nine-visible-tick size program
# and upload texture 27 as the source's exact PAL16 16x8 half-disc. The DS
# texture unit mirrors T in hardware, reproducing the N64 MASKT tile as one
# 16x16 flash without a second quad or duplicated texels. Defaults with the
# beam quad so the Fox-native lab arm covers the complete shot presentation;
# it remains independently switchable for attribution A/Bs. Owner-playtested
# and accepted 2026-08-09; ON BY DEFAULT through the beam route.
NDS_R2_FOX_BLASTER_GLOW_AOT ?= $(NDS_R2_FOX_BLASTER_QUAD)
# BUGS.md "Fox's pistol model is missing". Fox's gun is model part 13 on joint
# 17; source draws it by pointing the joint DObj's own `dl` at a display list in
# reloc asset 0x13b. The DS cannot copy that: ndsFighterDrawPlanResolve rejects
# the whole selected collection when any dl resolves outside the fighter's model
# asset, so the assignment would push the ENTIRE fighter off the native draw
# path for one 22-triangle part. This instead submits the part's baked mesh at
# joint 17's world matrix right after the fighter's own run, leaving the baked
# body untouched. Geometry, texture and palette are resolved offline by
# scripts/fox_gun_bake.py; nothing is walked or converted at runtime.
NDS_R2_FOX_GUN_OVERLAY ?= 1
# FireGrind as a tiny DS-native effect: replaces the BattleShip root particle +
# three generators + six interpreter sparks with a fixed pool of three source-
# derived quads per fireball bounce, drawn inside the existing particle GX batch.
# No root LBParticle, no generators, no LBTransform, no bytecode, no runtime trig
# or texture conversion. 3 quads/bounce instead of 6, reusing texture 5's already-
# resident atlas row. Owner-playtested and accepted 2026-08-07; ON BY DEFAULT.
# See include/nds/nds_firegrind.h.
NDS_R2_FIREGRIND_NATIVE ?= 1
# Dream Land Whispy native path. BattleShip still owns every root, generator,
# transform, child particle, update, and ejection; only the final draw selects
# three preconverted DS-native textures. Owner-playtested and accepted
# 2026-08-08; ON BY DEFAULT.
NDS_R2_WHISPY_NATIVE_TEXTURES ?= 1
# This keeps the source pools and source constructors, but AOT-specializes the
# three exact Dream Land generator/update scripts and submits their rigid
# billboards through the fixed-point GX quad path. Route 7 is the measured
# fastest path. Owner-playtested and accepted 2026-08-08; ON BY DEFAULT.
NDS_R2_WHISPY_NATIVE_AOT ?= 1
ifeq ($(NDS_R2_WHISPY_NATIVE_AOT),1)
ifneq ($(NDS_R2_WHISPY_NATIVE_TEXTURES),1)
$(error NDS_R2_WHISPY_NATIVE_AOT=1 requires NDS_R2_WHISPY_NATIVE_TEXTURES=1)
endif
endif
ifeq ($(NDS_R2_FOX_BLASTER_GLOW_AOT),1)
ifneq ($(NDS_R2_WHISPY_NATIVE_AOT),1)
$(error NDS_R2_FOX_BLASTER_GLOW_AOT=1 requires NDS_R2_WHISPY_NATIVE_AOT=1)
endif
endif
# Multiplies the Mario/Luigi fireball's stage (map) collision diamond -- the
# top/center/bottom/width fields from the weapon attributes reloc data -- by a
# port-side factor at spawn. The source values load straight from ROM reloc
# data and the port has no port-side entry to retune them, so this applies the
# scale after wpManagerMakeWeapon sets them, before the first per-frame stage
# collision pass reads them. 1.0 is source-exact; >1 enlarges the rebound feel.
# Owner playtest 2026-08-07: the source fireball felt slightly too small.
NDS_R2_FIREBALL_MAP_COLL_SCALE ?= 1.5
# ImpactWave native-mesh lab. The source effect remains the owner of spawn,
# lifetime, AObj/MObj animation and colour/index semantics, but its hot draw
# skips the generic N64 display-list/texture path: the 18 source vertices and 16
# source triangles are compiled into the port, while DL_0x7C28's real CI4 16x32
# image is stored directly in DS PAL16 nibble order with five AOT RGB555 palettes for
# the five source PRIM colours. Scene entry uploads those tiny resident names;
# impact frames do no N64 texel/TLUT conversion or texture allocation. The
# pixel-identical presented-frame A/B is owner-approved, so keep this on by
# default; setting it to 0 remains the source/interpreted control arm.
NDS_R2_IMPACT_WAVE_NATIVE ?= 1
# Source RebirthHalo model (EFCommonEffects3) compiled into DS-native triangle
# and texture tables. Runtime keeps the source GObj attachment/scale/rotation
# owner but does no N64 DL/Vtx/CI4/TLUT/I4 conversion for the three halo lists.
# The visible post-KO +60-frame A/B is pixel-identical and the native path cuts
# roughly 259K effect-draw ticks per active presented frame, so ship it on by
# default; setting it to 0 remains the source/interpreted control arm.
NDS_R2_REBIRTH_HALO_NATIVE ?= 1
# Bit per generated RebirthHalo group eligible for model-space GX submission.
# 0x3f is the all-GX experiment; final acceptance may narrow this if the DS
# transformer's subpixel rounding changes source coverage for a specific group.
NDS_R2_REBIRTH_HALO_GX_GROUP_MASK ?= 0x3f
# SEEDS gNdsBattlePlayableFoxCpuEnabled FOR A ROM NOBODY DRIVES WITH GDB. 1 is
# the published source-normal battle (3/2/1/GO, live match timer, level-3 Fox);
# 0 skips the countdown, unlocks at GO, freezes the timer and leaves Fox
# standing still. The scripted harnesses keep writing the variable at
# scVSBattleStartBattle and are unaffected either way -- this exists purely so a
# hand-played inspection ROM can be built, which it could not be before
# 2026-08-06. MUST STAY 1 for anything published or measured: a frozen Fox is
# not representative gameplay and its ticks are not a Boundary figure.
NDS_R2_FOX_CPU_DEFAULT ?= 1
# THE P1 DEMO LADDER (owner, 2026-08-17): Fox opens at CPU level 1 and gains a
# level each time Mario wins and START restarts from Results, wrapping 9 -> 1.
# The battle text HUD already prints the live level as "CPU L<n> [Fox]" from
# gNdsIFCommonHUDP1Level, so this adds no HUD code.
#
# It changes what the SHIPPED demo plays, so it also changes the first match of
# a Boundary run from level 3 to level 1. That is the owner's call and it is
# deliberate; the NDS_R2_BOTH_CPU stress arm re-pins both fighters at level 3
# (scene_harness.c) so every banked gate figure stays comparable. Build with 0
# for a fixed level-3 Fox.
NDS_DEMO_FOX_CPU_LADDER ?= 1
# NDS_R2_PARTICLE_V16_HEADROOM IS DELETED, AND A FIXED FACTOR IS WHY.
#
# It bought one extra bit of range (x16 -> x8, reach +/-2047.9 -> +/-4095.9) at
# the price of half the sub-unit resolution of EVERY particle, and it was still
# not enough: the owner re-filed the same symptom against the build that had it
# on. The reach a particle pass needs is not a constant -- an ordinary hit spark
# lives inside 2,000 units while the Star KO sparkle follows the dying fighter
# out to z = -14,999 (ftcommondead.c) -- so no single factor is both wide enough
# and precise enough. ndsRendererSubmitParticleQuad now picks the factor per
# batch and escalates only when a quad actually reaches the rail, which keeps
# the full x16 precision on ordinary frames and buys 8x the range on the frames
# that need it. Verify with gNdsParticleWorldClampCount (must be 0) and
# gNdsParticleScaleShiftMax; gNdsWhispyDrawClamped mirrors the first.
#
# THE LESSON, for the second time in a fortnight: a measured, working fix parked
# at a default of 0 is invisible to everyone including its author -- the fixed
# sqrt sat that way for a month on 2026-07-31. Audit the 0 flags whenever a
# symptom matches one of their descriptions.
#
# R2-07 L7 step one. Read-only oracle: re-does the collision joint inverse in
# 20.12 alongside the decomp's float one and records the deviation on the joints
# a real match inverts. Decides nothing and changes nothing.
#
# ITS QUESTION IS ANSWERED -- DO NOT RE-RUN IT. This comment used to say the
# live scale domain "has never been read off the running game"; it was read on
# 2026-07-31 and the result is recorded in include/nds/nds_r2_collision_mtx.h:
# 460 samples, joint scale 1.1138-1.1199, deviation 2/5/21 in 1/4096 world units
# at the 1/4/16-unit probes against a bound of 82, zero over bound, zero
# singular. Building it again on 2026-08-01 on the strength of that stale
# sentence ABORTED the ROM at the GO countdown -- GENERALFREE 20,272 against the
# 25,600 GObj latch, COMMONSMAX 45, MALLOCOVF 0 -- because its .text alone is
# more taskman arena than the tree has spare. That is the standing lesson: the
# shipping tree has roughly five kilobytes of arena margin, and a measurement
# has to be sized against it like any other code.
NDS_R2_COLLISION_L7_ORACLE ?= 0
# R2-07, the whole-cluster fixed-point fighter hurtbox narrow phase. Compiles
# src/port/nds_r2_collision_fixed.c, the out-of-line ARM entry points for the
# kernels in include/nds/nds_r2_collision_fixed.h.
#
# WIRED 2026-08-15 (slice 52). At 1 this also compiles
# src/port/nds_r2_collision_ring.c and turns on the wrapper in
# src/import/battleship_gmcollision.c, which runs func_ovl2_800EDBA4,
# func_ovl2_800EDE00 and func_ovl2_800EDE5C in fixed point behind an f32
# boundary. gmCollisionTestRectangle and gmCollisionTestSphere are NOT
# converted: every collision decision stays in decomp float code reading the
# same f32 fields, which is what makes the change gradeable by a bound rather
# than by a flip count. include/nds/nds_r2_collision_ring.h owns that argument.
#
# DEFAULT STAYS 0 UNTIL THE OWNER FLIPS IT. The shipping default is an owner
# decision (BLOCKED(decision: shipping default) on the board), and separately
# an object entering the link changes the link INPUT SET -- this project has
# measured re-addressing collateral from less than that (Tasks 87-89/94/95)
# against a cross-build P95 floor of +/-5,376, so a measuring cycle must not
# move the published ROM's placement as a side effect.
#
# The kernels are proven at this flag's 1 by scripts/check-r2-collision-fixed.ps1,
# which compiles them for the real target independently of the flag, so 0 costs
# no coverage. NDS_R2_COLLISION_FIXED_DISPATCH below is the falsifier arm.
NDS_R2_COLLISION_FIXED ?= 0
# The falsifier arm for the wiring above, and it is separate from the flag for a
# measurement reason. Setting NDS_R2_COLLISION_FIXED to 0 removes the objects
# from the link, which is a DIFFERENT link input set and therefore a different
# placement -- exactly the confound a flag falsifier exists to remove. This one
# keeps every byte and flips one initialised word of .data
# (gNdsCfxRingEnable, read through a volatile so no fold can reach it), so arm A
# and arm B are expected to have byte-identical .text.
NDS_R2_COLLISION_FIXED_DISPATCH ?= $(NDS_R2_COLLISION_FIXED)
# THE WARM-MAC EXCHANGE-RATE INSTRUMENT (include/nds/nds_r2_sim_mac_fixed.h).
# LAB ONLY, DEFAULT 0, and at 0 the translation unit is not linked at all so a
# published ROM stays byte-identical.
#
# It is a SHADOW, not a route: at every arm the decomp float body still runs and
# still writes the same result, and the fixed form is evaluated beside it and
# discarded. That is deliberate -- a replacement route cannot price these two
# bodies, because they feed gameplay decisions and the two arms would play
# different fights (one such A/B on this exact code ended with damage 130/51
# against 33/65 on the same ELF, one poked bit apart). The shadow's arms are
# bit-identical matches by construction, so every whole-match invariant must be
# equal and an unequal one means the INSTRUMENT is broken.
#
# gNdsR2SimMacShadowArm is a `.data` word poked with -SetGlobals: bit 0 runs the
# fixed point-x-matrix transform, bit 1 the fixed 3x4 affine compose, bit 2
# grades the transform against the float body's own result on the same inputs.
NDS_R2_SIM_MAC_SHADOW ?= 0
# THE TWO HOOKS include/nds/nds_r2_collision_fixed.h:205,215 HAS CARRIED SINCE
# IT WAS WRITTEN, finally bound. At 1, NDS_R2_CFX_DIV64 and NDS_R2_CFX_ISQRT64
# reach the ARM9 divide and square-root coprocessors instead of libgcc's
# bit-by-bit __aeabi_ldivmod and the header's own restoring digit-by-digit root.
# EXCHANGE.md section 0.4 names that portable divide -- four calls per
# narrow-phase entry -- as the measured cause of the collision ring's 2.68x
# exchange rate.
#
# It follows NDS_R2_COLLISION_FIXED rather than defaulting to 0 on its own,
# because at NDS_R2_COLLISION_FIXED=0 the translation unit that binds them is
# not linked at all and the flag cannot change a byte. There is no arm to
# choose between: the arithmetic is proven identical (scripts/check-r2-hwmath.ps1
# for the algorithm, gNdsR2HwMathBench*Mismatch for the unit) and the hardware
# form is not slower, so a default-off "candidate" here would only be a flag
# nobody flips.
NDS_R2_CFX_HWMATH ?= $(NDS_R2_COLLISION_FIXED)
# Lab price-and-equivalence instrument for those units
# (src/port/nds_r2_hwmath_bench.c). Default 0 and NOT linked at 0, so a
# published ROM is byte-identical with or without it. It answers a per-operation
# question -- what does one hardware divide, one hardware root, one
# hardware-unit f32 divide and one ARM-state sqrtf cost against the software
# form each would replace -- which is transferable across lanes in a way a
# whole-match A/B of one lane is not.
NDS_R2_HWMATH_BENCH ?= 0
# Lab SAME-BINARY route for the two hardware-math changes banked on 2026-08-16.
# Default 0; at 0 there is no route word, no selector and no second sqrtf body,
# so a published ROM carries only the shipped form. At 1 the binary holds both
# arms and one `.data` word (gNdsR2HwMathRoute, src/nds/r2/nds_r2_sqrtf.c)
# picks between them, which is the only way to price either change: sqrtf in ARM
# state is worth 4,300-4,750 tk/fr and the leading busy poll 2,491, both FAR
# under the >=14,080 rank-80 cross-build floor, so a two-build A/B cannot decide
# them.
#   bit 0  NDS_R2_HWMATH_ROUTE_SQRTF_ARM  sqrtf body from the -marm object
#   bit 1  NDS_R2_HWMATH_ROUTE_NO_LEAD    skip the leading DIVCNT/SQRTCNT poll
# Arm 0 is exactly what shipped before this date; arm 3 is exactly what ships
# after it.
NDS_R2_HWMATH_ROUTE ?= 0
# Task 44 stage steady-state excision: generation-based admission, dense
# rigid/dynamic binding lists, and the hoisted GX capture-active test. Requires
# the Task 36 hardware-compose stage owner; meaningless without it.
NDS_TASK44_STAGE_STEADY ?= 0
# Task 82 ITCM re-pack: evict the animated-CI4 texel path (dead while Dream
# Land water is frozen) and admit five higher-stall functions. Placement only.
# KEEP on measurement: WORK-H P95 -52,224 with both evictions; this ships the
# owner's one-eviction variant. Default on.
NDS_TASK37_PROFILE ?= 0
NDS_TASK37_PROFILE_START ?= 438
NDS_TASK37_PROFILE_FRAMES ?= 128
# Number each profiled frame as its own profiler region, so the host ledger can
# be differenced against the tick-HUD ring frame by frame instead of only over
# the window. Window totals cannot separate "accurate everywhere" from
# "over-counts on clean frames, under-counts on the tail", and the tail is what
# the P95 gate is decided on.
NDS_TASK37_PROFILE_PER_FRAME_REGION ?= 0
# Drive the profiler window from the VS Results loop instead of the battle loop.
# The battle site keys off presented frames and the Results loop never increments
# that counter, so a battle-keyed window opens and dumps during the match and says
# nothing about Results. Exactly one of the two tick sites is live.
NDS_TASK37_PROFILE_RESULTS ?= 0
# Task 37 placement repack: admit the measured non-memory-stall toppers into the
# 1,060 free ITCM bytes. Placement only -- no ISA switch, no optimization change,
# no eviction of any current resident.
#
# A BITMASK, not a boolean, so the admissions can be bisected. The 0/1 A/B on
# 2026-07-22 failed its state-hash gate and the owner confirmed the enabled lab
# build misbehaves, so which of these groups is at fault has to be established
# before any of it can be trusted:
#   1  libc leaves   memset, memcpy, memcmp
#   2  libm leaf     __ieee754_sqrtf
#   4  port leaves   TextureSourceBytes, PolyFmt, FTParamsInvalidateFighterParts
#   7  all of them (what the failing A/B ran)
NDS_TASK37_ITCM_LEAVES ?= 0
# Deferred (=), not immediate (:=). The per-target blocks below override
# NDS_TASK37_ITCM_LEAVES, and they are parsed AFTER these lines -- with := these
# would latch the default 0 and the override would silently do nothing, so the
# published target and the device A/B pair would both build with no leaves moved
# and produce a byte-identical ROM. Environment-driven builds were unaffected
# (the env value is in place before line 110), which is why the lab A/B runs
# were valid and this stayed hidden.
NDS_TASK37_ITCM_LIBC = $(if $(filter 1,$(NDS_FAST_MEM)),0,$(if $(filter 1 3 5 7,$(NDS_TASK37_ITCM_LEAVES)),1,0))
NDS_TASK37_ITCM_LIBM = $(if $(filter 2 3 6 7,$(NDS_TASK37_ITCM_LEAVES)),1,0)
NDS_TASK37_ITCM_PORT = $(if $(filter 4 5 6 7,$(NDS_TASK37_ITCM_LEAVES)),1,0)
# Layout control: N bytes of never-executed padding in .main, nothing in ITCM.
# Separates "ITCM residency breaks it" from "any layout change breaks it".
NDS_TASK37_LAYOUT_PROBE ?= 0
# Lab-only: snapshot raw FTStruct bytes for both fighters on updates N-1 and N,
# so the Task 37 divergence names an offset instead of a 3,012-byte blob.
# Costs ~12 KiB of BSS when enabled; observation only, never in a shipping build.
NDS_TASK10_GIT_SHORT ?= $(shell git rev-parse --short=7 HEAD 2>/dev/null || echo unknown)
ifeq ($(NDS_TASK9_FLOAT_PHASE2),1)
ifneq ($(NDS_TASK9_FLOAT_ITCM),1)
$(error NDS_TASK9_FLOAT_PHASE2=1 requires NDS_TASK9_FLOAT_ITCM=1)
endif
endif
ifeq ($(NDS_TASK16_FLOAT_COMPARE),1)
ifneq ($(NDS_TASK9_FLOAT_PHASE2),1)
$(error NDS_TASK16_FLOAT_COMPARE=1 requires NDS_TASK9_FLOAT_PHASE2=1)
endif
endif
ifeq ($(NDS_TASK16_FLOAT_I2F),1)
ifneq ($(NDS_TASK9_FLOAT_ITCM),1)
$(error NDS_TASK16_FLOAT_I2F=1 requires NDS_TASK9_FLOAT_ITCM=1)
endif
endif
ifeq ($(NDS_TASK16_FLOAT_ADDSUB),1)
ifneq ($(NDS_TASK9_FLOAT_PHASE2),1)
$(error NDS_TASK16_FLOAT_ADDSUB=1 requires NDS_TASK9_FLOAT_PHASE2=1)
endif
endif

