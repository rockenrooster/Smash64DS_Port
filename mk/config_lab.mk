# Lab and runtime knobs: the renderer, collision, simulation and profiling
# switches (NDS_R2_*, NDS_TASK*_*), most of them off in every shipping and
# gate configuration; the intro bake stamp.
# Included in place from the Makefile (P8 G); its text is unchanged.

NDS_OWNER_LINK_INTRO_STAMP := $(PROJECT_ROOT)/src/nds/generated/.owner_link_intro
$(shell mkdir -p "$(dir $(NDS_OWNER_LINK_INTRO_STAMP))"; [ "$$(cat "$(NDS_OWNER_LINK_INTRO_STAMP)" 2>/dev/null)" = "$(NDS_OWNER_LINK_INTRO)" ] || echo "$(NDS_OWNER_LINK_INTRO)" > "$(NDS_OWNER_LINK_INTRO_STAMP)")
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
# Owner 5 is the only census-ranked Dream Land cut that passed the canonical
# 500-pixel ratchet.  The enclosing economy flag remains off by default.
NDS_RENDERER_BENCHMARK_MODE ?= 0
NDS_RENDERER_BATTLE_STATIC_TEXTURE_DEFAULT ?= 0
# The shipping free-play ROM carries the freeze watchdog and the exception
# report (owner, 2026-10-11: a frozen console showed nothing to photograph,
# and an out-of-memory halt spins silently); every measurement target keeps
# it off.
NDS_FREEZE_DIAGNOSTICS ?= $(if $(filter smash64ds,$(TARGET)),1,0)
# Bounded pipelining (owner 2026-10-19): a battle frame's next game logic
# may start this many scanlines before its swap retrace (131 = half a
# VBlank interval); 0 is the old loop. gNdsPresentLeadLines A/Bs at boot.
NDS_PRESENT_LEAD ?= 131
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
# Replace EFCommon script 0x62 with its closed nine-visible-tick size program
# and upload texture 27 as the source's exact PAL16 16x8 half-disc. The DS
# texture unit mirrors T in hardware, reproducing the N64 MASKT tile as one
# 16x16 flash without a second quad or duplicated texels. Defaults with the
# beam quad so the Fox-native lab arm covers the complete shot presentation;
# it remains independently switchable for attribution A/Bs. Owner-playtested
# and accepted 2026-08-09; ON BY DEFAULT through the beam route.
NDS_R2_FOX_BLASTER_GLOW_AOT ?= 1
# Multiplies the Mario/Luigi fireball's stage (map) collision diamond -- the
# top/center/bottom/width fields from the weapon attributes reloc data -- by a
# port-side factor at spawn. The source values load straight from ROM reloc
# data and the port has no port-side entry to retune them, so this applies the
# scale after wpManagerMakeWeapon sets them, before the first per-frame stage
# collision pass reads them. 1.0 is source-exact; >1 enlarges the rebound feel.
# Owner playtest 2026-08-07: the source fireball felt slightly too small.
NDS_R2_FIREBALL_MAP_COLL_SCALE ?= 1.5
# Bit per generated RebirthHalo group eligible for model-space GX submission.
# 0x3f is the all-GX experiment; final acceptance may narrow this if the DS
# transformer's subpixel rounding changes source coverage for a specific group.
NDS_R2_REBIRTH_HALO_GX_GROUP_MASK ?= 0x3f
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
# The falsifier arm for the wiring above, and it is separate from the flag for a
# measurement reason. Setting NDS_R2_COLLISION_FIXED to 0 removes the objects
# from the link, which is a DIFFERENT link input set and therefore a different
# placement -- exactly the confound a flag falsifier exists to remove. This one
# keeps every byte and flips one initialised word of .data
# (gNdsCfxRingEnable, read through a volatile so no fold can reach it), so arm A
# and arm B are expected to have byte-identical .text.
NDS_R2_COLLISION_FIXED_DISPATCH ?= 0
# Task 44 stage steady-state excision: generation-based admission, dense
# rigid/dynamic binding lists, and the hoisted GX capture-active test. Requires
# the Task 36 hardware-compose stage owner; meaningless without it.
NDS_TASK44_STAGE_STEADY ?= 0
# Task 82 ITCM re-pack: evict the animated-CI4 texel path (dead while Dream
# Land water is frozen) and admit five higher-stall functions. Placement only.
# KEEP on measurement: WORK-H P95 -52,224 with both evictions; this ships the
# owner's one-eviction variant. Default on.
NDS_CPU_PROFILE ?= 0
NDS_CPU_PROFILE_START ?= 438
NDS_CPU_PROFILE_FRAMES ?= 128
# Number each profiled frame as its own profiler region, so the host ledger can
# be differenced against the tick-HUD ring frame by frame instead of only over
# the window. Window totals cannot separate "accurate everywhere" from
# "over-counts on clean frames, under-counts on the tail", and the tail is what
# the P95 gate is decided on.
NDS_CPU_PROFILE_PER_FRAME_REGION ?= 0
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
NDS_ITCM_LIB_LEAVES ?= 0
# Deferred (=), not immediate (:=). The per-target blocks below override
# NDS_ITCM_LIB_LEAVES, and they are parsed AFTER these lines -- with := these
# would latch the default 0 and the override would silently do nothing, so the
# published target and the device A/B pair would both build with no leaves moved
# and produce a byte-identical ROM. Environment-driven builds were unaffected
# (the env value is in place before line 110), which is why the lab A/B runs
# were valid and this stayed hidden.
NDS_ITCM_LIBC = 0
NDS_ITCM_LIBM = $(if $(filter 2 3 6 7,$(NDS_ITCM_LIB_LEAVES)),1,0)
# Lab-only: snapshot raw FTStruct bytes for both fighters on updates N-1 and N,
# so the Task 37 divergence names an offset instead of a 3,012-byte blob.
# Costs ~12 KiB of BSS when enabled; observation only, never in a shipping build.
NDS_TASK10_GIT_SHORT ?= $(shell git rev-parse --short=7 HEAD 2>/dev/null || echo unknown)

