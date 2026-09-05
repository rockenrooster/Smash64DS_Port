# Nintendo DS architecture probe for the BattleShip Smash 64 decompilation.
.SUFFIXES:

ifeq ($(strip $(DEVKITARM)),)
$(error "DEVKITARM is not set. Install devkitPro and set DEVKITARM to devkitARM")
endif

# Keep both inherited Windows backslashes and the documented C:/devkitPro
# spelling deterministic under MSYS make rules.  Some libnds assertions embed
# header paths in the ROM, so this normalization is part of build identity.
NDS_NORMALIZE_DEVKIT_PATH = $(patsubst %/,%,$(patsubst c:/%,/c/%,$(patsubst C:/%,/c/%,$(subst \,/,$(strip $(1))))))
override DEVKITPRO := $(call NDS_NORMALIZE_DEVKIT_PATH,$(DEVKITPRO))
override DEVKITARM := $(call NDS_NORMALIZE_DEVKIT_PATH,$(DEVKITARM))

# Build parallelism, set here rather than expected on the command line.
#
# GNU Make defaults to one job and nothing in this repo or in the environment
# ever set -j, so every build in the campaign ran single-threaded -- roughly
# thirteen minutes of wall clock for a full tickhud rebuild on a 32-thread
# machine. A flag you have to remember is a flag that gets forgotten, and this
# one was, for months. Setting it in the Makefile makes it apply to every
# caller: the owner's builds, an agent's builds, and any harness that shells out
# to `make` without knowing to ask.
#
# One build at a time is still the rule, and this does not change that. The
# asset generators write into shared paths such as include/nds/generated/, which
# live OUTSIDE $(BUILD), so two concurrent makes with different flags would
# corrupt each other's generated headers no matter what -j is. This parallelises
# within a build; nothing parallelises across builds.
#
# `make NDS_JOBS=1 ...` forces a serial build. Keep that escape hatch: an
# under-declared prerequisite in a generator does not fail loudly under -j, it
# races and yields a subtly wrong binary, and on this project that surfaces as
# an unexplained measurement rather than an error. Serial is how you bisect it.
NDS_JOBS ?= $(shell nproc 2>/dev/null || echo $(NUMBER_OF_PROCESSORS))
ifneq ($(strip $(NDS_JOBS)),)
MAKEFLAGS += -j$(strip $(NDS_JOBS))
endif

# The DS system-menu banner. P2-1h, owner ruling 2026-08-18: this is a port,
# so the original branding ships -- and the banner is the first branding a
# player sees, before the ROM boots at all. The three lines are what ndstool
# packs into the banner's title field; the copyright is the original's own,
# transcribed from `llMNTitleCopyrightSprite`, the sprite the title screen
# draws at its foot (mntitle.c:64, kind nMNTitleSpriteKindFooter).
#
# GAME_ICON must be set BEFORE `include $(DEVKITARM)/ds_rules`, which only
# defaults it to calico's own placeholder when it is empty.
PROJECT_ROOT ?= $(CURDIR)
GAME_TITLE     := Super Smash Bros.
GAME_SUBTITLE1 := Smash64DS
GAME_SUBTITLE2 := (C)1999 Nintendo/HAL Laboratory, Inc.
NDS_BANNER_ICON := $(PROJECT_ROOT)/assets/banner/smash64ds_icon.bmp
GAME_ICON      := $(NDS_BANNER_ICON)

TARGET := smash64ds
BUILD := build
NDS_OUTPUT_BASENAME ?= $(TARGET)
BUILD_OUTPUT_ROOT ?= builds
ifeq ($(BUILD),$(notdir $(BUILD)))
ifneq ($(filter build%,$(BUILD)),)
override BUILD := $(BUILD_OUTPUT_ROOT)/$(BUILD)
endif
endif
NDS_PUBLISHED_TARGETS := smash64ds smash64ds-battle-playable-hwtri
override NDS_PUBLISH_USER_ROM := $(if $(filter $(TARGET),$(NDS_PUBLISHED_TARGETS)),1,0)
NDS_OUTPUT_ROOT ?= $(if $(filter 1,$(NDS_PUBLISH_USER_ROM)),$(PROJECT_ROOT),$(PROJECT_ROOT)/$(BUILD))
# A PUBLISHED TARGET NAME PUBLISHES, whatever BUILD says. That is intended --
# the lab and the published ROM must be the same program -- but it means a lab
# build with flags on leaves ITS ROM at the path the owner double-clicks, and
# the only symptom is a game that behaves nothing like the published one. It
# has cost a real "P95 regressed hard before GO" report (2026-08-01: the root
# held an 11,755,520-byte both-CPU particle-draw build instead of the
# 11,557,888-byte published one). Say so at build time; rebuild the plain
# target afterwards.
ifeq ($(NDS_PUBLISH_USER_ROM),1)
# BUILD has already been rewritten from `build` to `builds/build` above, so the
# default spelling must be excluded too or this note fires on every bare make.
ifeq ($(filter build builds/build,$(BUILD)),)
$(info NOTE: BUILD=$(BUILD) is writing the PUBLISHED ROM $(TARGET).nds into the project root.)
$(info       Run `make TARGET=$(TARGET)` with no overrides afterwards, or the root ROM stays this lab build.)
# AND THE .elf/.nds PAIR AT THAT PATH IS SHARED BETWEEN BUILD DIRECTORIES, which
# is the sharper edge: make only tracks each directory's own objects, so after a
# published build overwrites the pair, re-running the LAB build is a NO-OP -- its
# objects are still up to date relative to files that are no longer its output.
# The harnesses then read whichever pair was written last, silently. Measured:
# a soak reported "this ROM does not define 64 counter(s)" for the very build
# that defines them.
#
# FORCING THE RECIPE HERE DOES NOT WORK, and both shapes were tried. A force on
# the .nds alone relinks the ROM from the STALE .elf (a ROM that is neither
# build); a force on the .elf pulls the link into the top-level make, which does
# not have the sub-make's working directory and dies with "cannot open linker
# script file nds_task32_draw_hot.inc". The fix belongs in the harness -- resolve
# a per-build .elf -- not at this seam.
#
# Until then: run a lab build IMMEDIATELY before its measurement, and if a
# published build has intervened, `rm` the root .elf/.nds pair first.
endif
endif
ifeq ($(NDS_PUBLISH_USER_ROM),0)
ifeq ($(abspath $(NDS_OUTPUT_ROOT)),$(abspath $(PROJECT_ROOT)))
$(error Non-published target "$(TARGET)" may not write into the project root)
endif
endif
# P1 does not ship smash64ds.nds (owner, 2026-08-02), so a build that FALLS
# INTO the default target is usually a mistake worth flagging: it costs a full
# P2 build and overwrites the root pair the harnesses read. The note fires only
# when TARGET was defaulted at the top level -- an explicit TARGET=smash64ds on
# the command line (verify-all, build.ps1, publish work) stays silent.
ifeq ($(MAKELEVEL),0)
ifeq ($(TARGET)/$(origin TARGET),smash64ds/file)
ifeq ($(strip $(filter-out all run,$(MAKECMDGOALS))),)
$(info NOTE: TARGET defaulted to smash64ds -- the P2 ROM, not needed for P1.)
$(info       P1 iteration wants `make p1` (published battle ROM + tick-HUD sibling) or `make p1-tick`.)
endif
endif
endif
NDS_DEV_SCENE_HARNESS ?= normal
NDS_DEV_LIVE_INPUT_PREVIEW ?= 0
NDS_HARNESS_FAST_LOGIC ?= 0
# Verifier-only on-demand presentation for the bounded fast battle harness.
# Fast logic normally renders once after its entire update run, which is ideal
# for state-only proofs but cannot verify source behavior that depends on one
# live presentation between gameplay states.  When enabled, proof code may ask
# the harness to submit the ordinary DS hardware renderer after the current
# source tick. Shipping targets leave this off; the request changes presentation
# only and never creates, releases, or mutates gameplay state.
NDS_HARNESS_FAST_PRESENT_ON_REQUEST ?= 0
NDS_RENDERER_HW_TRIANGLES ?= 0
NDS_RENDERER_HW_DEBUG_TEXTURE_ONLY ?= 0
NDS_RENDERER_PROFILE_LEVEL ?= 2
NDS_SHIP_TELEMETRY ?= 1
NDS_TICK_HUD ?= 0
NDS_RENDERER_M2_DETAILED_LEDGER ?= 0
NDS_RENDERER_M3_PHASE0_PROFILE ?= 0
NDS_NATIVE_STAGE_GENERATED_SEGMENT0_ENABLE ?= 0
NDS_TASK29_GX_CENSUS ?= 0
NDS_TASK34_STAGE_STREAM_CENSUS ?= 0
# Task 49 GX equivalence differ capture (Part 2 instrument). Records the
# per-owner GX stream word-for-word for host-side profile-0-vs-profile-1
# diffing. Lab-only; built via command line, never in a shipping target.
NDS_TASK49_GX_DIFFER ?= 0
NDS_TASK36_HW_COMPOSE ?= 0
# R2-07 E2. Route each stage segment to Task 36 replay or the live path at
# RUNTIME, in one binary, so the second-entry corruption can be attributed to a
# route rather than to a rebuild. Default 0 keeps the shipping build byte-clean;
# at 1 the gate reads four volatiles that gdb sets, and reports which route each
# segment actually took so a picture change is checkable rather than assumed.
# Separately linked A/B ROMs are the wrong instrument here -- this ROM's pacing
# is cache-placement sensitive and that has already confused two comparisons.
NDS_R2_STAGE_ROUTE_PROBE ?= 0

# Task 53: default-off re-activation guard for the Task 36 rigid-stage
# replay path. The robust downward-stepping arena allocator at
# src/port/diagnostics.c:7368 secures anywhere from the 0x130000 floor
# (after stepping down from NDS_TASKMAN_ARENA_SIZE) up to the full
# arena; the legacy exact-arena guard at src/nds/nds_renderer.c:4223
# was a stale "pristine environment only" check. Setting this flag to 1
# relaxes the guard; the published and tick-HUD target blocks don't
# override it (default 0 keeps the published ROM 1818AA77-sh equivalent).
NDS_TASK53_REPLAY_ARENA_FIX ?= 0
# Task 55: redundant state-write elision at capture. GFX_COLOR/GFX_TEX_COORD
# are persistent geometry-engine state; ndsRendererNativeStageEmitNoZVertex
# (src/nds/nds_renderer.c:20448) re-writes color unconditionally per vertex.
# E0 measured 618/1197 of those state words (COLOR 556 + TEX_COORD 62) as
# identical to the immediately preceding write -- pure redundancy. Setting
# this flag to 1 elides them at capture so owner->words[] is 20.6% smaller
# and bit-identical render (lossless). Default 0 keeps the published ROM
# byte-identical; the published/tick-HUD blocks do NOT override it.
#
# CLOSED, do not reopen as a gate lever (PERF_LEDGER, "Task 55 ... STOP"). It is
# lossless in the replay buffer and it still fails twice: STG -4,224 against
# OTHR +7,616, because a COLOR/TEX_COORD write updates a state register and does
# not trigger a vertex transform -- the stage floor is the 606 VERTEX16
# transforms and nothing else -- and the owner's visual A/B found surfaces
# PULSATING IN COLOR at 1. The surviving finding is the one Task 56 acts on:
# only fewer VERTEX16 commands move that floor.
NDS_TASK55_STAGE_GEOM ?= 0
# Task 56: fighter DS-native primitive streams. Compiles the immutable Mario/Fox
# topology offline into GL_TRIANGLE_STRIP / GL_QUAD primitive descriptors (one
# GFX_BEGIN per primitive group + an N+2-vertex strip sequence) instead of the
# current GL_TRIANGLES (3 verts/tri, no sharing). E0 measured mode 2 (within-run
# reorder strips) at 47.0% / 882 fewer VERTEX16 submissions per two-fighter
# traversal -- the VERTEX16-transform floor Tasks 53/55 proved is invariant.
#   0 = current native-fighter GL_TRIANGLES emission (control)
#   1 = exact source-order strips (E0: 9.9%)
#   2 = within-run opaque-run topology reorder strips (E0: 47.0%)
# Topology is compiled host-side (no runtime strip finding). The published and
# tick-HUD blocks do NOT override it, so this default is what both ship.
#
# GRADUATED TO 2 in cycle 116. The KILL that kept this at 0 rested on a defect,
# not on strips: the generator emitted 35.6% of the fighter's 626 triangles with
# REVERSED WINDING, so a third of the model was culled away on hardware with no
# assert to say so, and the runtime emitter was `cold`/`Os` in .main branching on
# `textured` once per vertex while its raw siblings sat in ITCM. Both fixed;
# scripts/fighters/check_native_owner_geometry_closure.py is the standing proof that
# every source triangle is drawn exactly once with the source winding.
#
# The old note here read "NOT A GATE LEVER ... a lever that touches only FTR
# cannot change the over-gate COUNT". That is still true of the COUNT and still
# irrelevant to the BUDGET: the gate is P95 ticks, FTR is on nearly every frame,
# and Requirement 7 of the 2026-08-10 goal is explicit that flat savings count.
# One-binary A/B on gNdsR2FighterStripRoute (NDS_R2_STRIP_ROUTE=1, same ROM sha,
# 1600 samples an arm): FTR P50 313,856 -> 302,272, WORK-H P50 952,512 ->
# 941,312. The c115 per-PC census is why: ~28 of a corner's 40.5 cycles are the
# GX write and the stall is per VERTEX, so 2,148 raw corners a frame becoming
# ~1,160 strip corners is the only lever the emit has.
#
# It shipped broken once, on 2026-08-10, and the owner saw missing geometry on
# both fighters immediately. Cause: the emitter issued BEGIN_VTXS only when the
# group TYPE changed, so ADJACENT STRIP GROUPS were welded into one vertex list
# -- two bogus bridging triangles per join and the wrong parity for everything
# after it, hence culled. The first run alone has six consecutive strip groups.
# The static checker could not see it because it expanded each group
# independently, i.e. it validated the data under a BEGIN policy the runtime
# did not follow. Both are fixed and the checker now models the policy: under
# the old one it reports mode 2 drawing 744 triangles against 626 source across
# 29 runs. Run it after touching either side.
NDS_TASK56_FIGHTER_PRIMITIVES ?= 2
# Task 51 native stage path. When on, the STAGE owner emits the 42 baked
# constant world matrices via MTX_MULT4x3 under a once-loaded view (instead of
# CPU-composing projection x view x model per binding per frame). Generalizes
# Task 36's rigid-subset replay to the full 42-binding model-space form.
# Default off; the published ROM stays byte-identical at 0. Ship-ready only
# once the differ (Tier 1 = 0, Tier 2 <= 1.0 px) and STG budget clear.
NDS_TASK51_STAGE_NATIVE ?= 0
# Task 62: generated Dream Land DS-native static 3D mesh (candidate c120).
# Default off; the published ROM stays byte-identical at 0. Ship-ready only
# once the owner's visual A/B + perf A/B (Commit 5 KEEP gate) clears.
NDS_DREAMLAND_DS_MESH ?= 0
# Task 63 §5: Dream Land backdrop-card cull *visualization*. Lets the owner see
# what an authorised scenery reduction would actually cost before deciding.
# Compiles in a 64-bit run mask (gNdsDreamLandCardCullMask) that suppresses
# whole projected-no-Z stage runs; the mask defaults to 0, so flag=1 with an
# unset mask renders identically to flag=0. Lab instrument, never shipped.
NDS_DREAMLAND_CARD_CULL ?= 0
# Baked cull set, one bit per sNdsNativeStageRuns[] entry (MASK0 = runs 0-31,
# MASK1 = runs 32-53). Baked rather than poked because the Task 36 replay
# captures the stage stream once: a mask applied after capture would simply be
# replayed away. E0 §5 candidate sets, cheapest screen coverage first:
#   cheapest10: MASK0=0x000E03C0 MASK1=0x000C0800  -> 19 tris, 10.9%
#   cheapest16: MASK0=0x037E03C0 MASK1=0x002C0800  -> 36 tris, 20.6%
NDS_DREAMLAND_CARD_CULL_MASK0 ?= 0
NDS_DREAMLAND_CARD_CULL_MASK1 ?= 0
# Battle pipeline selector. Orthogonal to NDS_RENDERER_PROFILE_LEVEL, which
# is the *instrumentation* level within profiles 1 and 2 and keeps its
# existing values (0 lean, 1 phase timers, 2 full oracle).
#   0 = DS-native precompiled path (Tasks 51/52; not implemented yet)
#   1 = today's shipping translation path -- the correctness oracle
#   2 = instrumented / semantic oracle build of that same path
NDS_BATTLE_PROFILE ?= 1
NDS_TASK22_WALLPAPER_RUN_LAB ?= 0
NDS_RENDERER_SCREEN_SPACE_CENSUS ?= 0
# Task 90 E0 lab probe. Counts dense-vertex shade iterations in the native
# fighter owner and how many of them recompute a value the prepared array
# already holds. Lab only; it adds a 541-entry key array and per-iteration
# counters, so it must never be on in a measured or published build.
NDS_TASK90_SHADE_CENSUS ?= 0
# Task 93 E0 lab probe. Counts texture-key rebuilds in
# ndsRendererHardwareResolveOrBindTexture and records the key-hash request
# sequence, so a front-cache is sized from the measured trace the way Task 90
# sized the light-shade LUT. Lab only.
NDS_TASK93_TEXKEY_CENSUS ?= 0
# Task 107. Lab-only renderer-state redundancy census. Counts exact repeated
# texture-tile republishes at the four surviving call sites and texture-bind
# request/elision/revisit behavior. Default off: the arrays and bookkeeping do
# not exist in shipping or ordinary performance builds.
NDS_TASK107_RENDER_STATE_CENSUS ?= 0
# Task 108. Lab-only dynamic decomposition of the three callbacks dispatched
# from ftMainProcUpdateInterrupt. The census temporarily interposes the live
# FTStruct callback pointers while that one proc is running and records the
# exact (fighter kind, status, proc slot, original target) call/tick tuple.
# Default off; no callback interposition or storage exists in shipping builds.
NDS_TASK108_SITR_CALLBACK_CENSUS ?= 0
# BUGS.md #10 lab probe. Holding SELECT draws every polygon two-sided, so the
# owner can A/B culling in place at one fixed camera. The original culling
# probe compared two separate captures and was judged on an image that did not
# contain the bug; this exists so that mistake cannot repeat. Lab only.
NDS_LAB_CULL_PROBE ?= 0
# Which bits of the run index the tint probe shows. 0 = low three bits, 3 =
# next three, so two captures name a run exactly out of the 67 there are.
NDS_LAB_TINT_SHIFT ?= 0
# BUGS.md #10 / P2-3r17 seam probe. At 1 it compiles a four-arm runtime probe
# into the FIGHTER batch path -- both the production owner's
# ndsRendererNativeBeginDirectBatch (modes 8/9) and mode 10's hierarchy batch --
# cycled by SELECT and printed on HUD row 3:
#
#   0 shipped GX chain   1 POLY_CULL_NONE   2 POLY_CULL_FRONT
#   3 strips off   4 BattleShip source-world matrices
#
# Arm 1 splits "the geometry never reached the GX" from "the GX culled it",
# which no counter can tell apart. Arm 2 INVERTS the cull, so a probe that is
# not reaching the geometry cannot be mistaken for one that is. Arm 3 only
# exists when NDS_R2_STRIP_ROUTE compiled both fighter emitters in; it drives
# gNdsR2FighterStripRoute. The final arm is the matrix-only P2-3r17 A/B: all
# geometry/material/primitive state stays identical while Mario/DK switch from
# the shipped GX local-chain compose to BattleShip's FTParts::mtx_translate.
#
# It is a RUNTIME cycle on purpose. Until 2026-08-25 this flag patched only the
# hierarchy batch, which the production owner never calls, so the probe sat on a
# path fighters do not take and a build that found nothing looked exactly like a
# build that had tested something -- which is how the 2026-07-27 "culling
# REFUTED" verdict was reached against an arm that never ran. One binary, one
# camera, SELECT between the arms: pair it with capture-melonds.ps1's
# -SelectPresses/-SecondOutput, or poke gNdsLabSeamArm with its -SetGlobals.
#
# REQUIRES NDS_R2_FIGHTER_PACKET=0 (a build error otherwise). The shipped
# fighter draw is a DMA replay of a recorded GX stream, which ignores every arm
# and makes two different arms produce byte-identical frames -- the same
# "the probe found nothing" shape, one layer further in.
#
# AND A CULL ARM IS NOT AN ORACLE FOR MISSING GEOMETRY. Drawing both faces
# fills a hole's COLOUR in without closing the hole, so an arm judged on "does
# it look better" reports a fix that is not there (owner, 2026-08-25). Judge
# only on whether the seam is gone, and only against a FRAME-LOCKED control:
# capture timing on a live battle drifts pose run to run by far more than the
# defect (measured 2026-08-25: eight captures at two fixed delays, every pair
# hundreds of thousands of pixels apart).
#
# Fighters only -- un-culling the stage as well blows past the polygon limits
# and hangs the ROM. Needs the battle FPS HUD for its arm indicator, and owns
# SELECT, so it cannot be combined with NDS_R2_CAMERA_FIXED_TOGGLE (both are
# build errors).
NDS_LAB_NO_CULL ?= 0
# Task 91 E1 lab probe. Times the generic DObj tree walk and the native-owner
# revalidation inside the fighter draw, on the tick-HUD ROM -- the split the M2
# ledger measures but cannot report for the Boundary configuration.
NDS_TASK91_DRAW_PHASE_CENSUS ?= 0
# R2-03 E43. E38's before/after span split is bracketed around
# ndsRendererNativeApplyStateSpan, but ndsRendererNativeApplyStateDelta opens
# with a per-delta census block -- E20's identical-operand arrays plus E25c's
# effect histogram -- that sits INSIDE that bracket and runs 134.5 times a frame
# on the before-span alone. docs/HANDOFF.md already warned the State bracket is
# inflated by it. This arm keeps E38's brackets and compiles the per-delta block
# out, so the bracket measures the replay's real work. E26 is sized against that
# number, not against 33,708.
NDS_R2_SPAN_LEAN_TIMING ?= 0
# R2-03 E46. Places the whole fighter state-delta path in ITCM. The switch is
# already there (0x01ff9934 in the census ELF); the span loop (0x02003a14) and
# the Record* helpers it calls (0x0200d4e8) are not, so every one of the
# before-span's 134.5 applications a frame leaves zero-wait ITCM for
# icache-served main RAM. ~1,088 bytes against 2,912 free in .itcm. Placement
# only -- no behaviour change, so a win is attributable to instruction fetch.
NDS_R2_DELTA_PATH_ITCM ?= 0
# R2-04 E1. Task 75 absorption: keeps each fighter animation's byte-swapped
# pre-fixup payload resident so the frame that needs a move does not re-walk
# NitroFS and re-read the cartridge. E0 measured 53 of 82 force-loads (64.6%)
# repeating an asset already loaded, over a working set of 29 distinct
# animations. Every failure path degrades to the uncached load.
NDS_R2_ANIM_CACHE ?= 0
# R2-06 E13. Apply ndsRelocNormalizeFighterAObj16File to each warmed animation
# ONCE, at warm time, so the force-load path can skip it. Cycle 107 priced that
# pass at 10,236,800 ticks a match -- 16.2% of the SINT excursion, ~29,000 per
# force-load frame -- and E11's rule is that a load-frame cost can only be banked
# by moving it off the frame, not by making it faster. The transform is
# position-independent (it reads offsets and never touches the pointer table),
# and nothing runs between the internal fixups and it, so warm time and load time
# see identical bytes. Requires NDS_R2_ANIM_CACHE. Declines are per asset and
# fall back to today's path; see the block comment in reloc_backend_assets.c.
NDS_R2_AOBJ16_PREBAKE ?= 0
# Native Battle Kernel slice 1 phase 5. The resident figatree pack: one fighter's
# clips linked into .rodata in the parser's own byte order, with per-slot tables
# of blob-relative offsets, so an action change resolves to a const pointer
# instead of rebuilding an N64 loaded-file image at the fighter's heap. This is
# NOT a bigger cache -- a cache HIT still memcpy'd the payload, re-registered the
# file, re-ran finalize/fixups/AObj16 normalization and stripped alias nodes, and
# that sequence sat on 62 of the 80 frames that set P95 against 174 of the 1,520
# body frames (6.8x).
#
# THE BLOB IS NOW ARENA-RESIDENT (2026-08-15). At 1 the Fox pack ships as a
# NitroFS payload and is streamed into the taskman animation arena at scene
# setup, replacing the 262,144 B raw-file cache; the ARM9 image does not grow.
# The `.incbin` route it replaced is DISQUALIFIED and must not come back:
# +288,992 B of static image was MEASURED to push `gNdsTaskmanArenaChosenSize`
# from 0x150000 to 0x140000 with 16 alloc failures -- the arena is calloc'd from
# the same libnds heap `fake_heap_start` bounds, so `.rodata` and the arena come
# out of the same bytes -- projecting a general-heap low-water of ~6,652 against
# the mandated 32,768 floor. The boot ladder does NOT catch that: the failing arm
# had 66,784 B of proven headroom.
# OWNER DECISION 2026-08-17: THE DEFAULT IS 1, taken with KEEP_CACHE below.
# `artifacts/performance/2026-08-17_ship-cadence/SHIP_CADENCE.md`. The trade the
# previous comment called unpriced is priced: measured on the shipping defaults,
# the pack is -34,304 at rank-80 (2.44x the >=14,080 cross-build floor, 2.1x the
# +16,209 requirement a pack-off ROM carries) and 13 of the frames the cadence
# arm counts, against 17,600 B of general heap -- low-water 70,736 without it
# and 53,136 with it, on the mandated 32,768 floor, which is exactly the figure
# the comment at :345-347 predicted before the run.
#
# What made this a decision rather than a candidate: every requirement, every
# candidate size and BOTH gate arms of this campaign had already been measured
# with the flag ON while the published ROM shipped without it, nine artifacts
# deep. The flip does not FIND 34,304 ticks; it stops the measurement basis and
# the published ROM disagreeing.
# `artifacts/performance/2026-08-15_battlepack-pool/BATTLEPACK_POOL.md` is the
# pool history; the 2026-08-15 stress battery is in ARENA_PRICE.md.
NDS_R2_BATTLEPACK ?= 1
# Source-normalized AObj16 clips remain in NitroFS and are read directly into
# each fighter's existing figatree heap on a cache miss. Unlike the resident
# BattlePack this spends no scene RAM and covers every landed fighter family.
NDS_R2_FTANIM_STREAM ?= 1
# The arm that isolates the pack from the cache it displaces. Phase 8 measured
# the resident pack at 2.9x the gate and attributed all of it to the carve
# DELETING the raw file cache (262,144 -> 4,096 B; Rejects 0 -> 126); the
# isolation arm then refuted that attribution, and the mechanism cycle found the
# real owner -- a resolver ordering defect, now fixed. With the defect gone the
# carve is the whole remaining fare: H - G = +265,856 at rank-80.
#
# At 1 the animation arena reserves the blob PLUS a raw cache, and
# NDS_TASKMAN_ARENA_SIZE grows so the general heap is not robbed to pay for it.
# It is independent of NDS_R2_BATTLEPACK on purpose: at BATTLEPACK=0 it grows the
# arena and nothing else, which is the matched control for the pair (same arena
# size, same addresses, same Task 53 guard -- only the pack differs).
#
# "LAB ONLY, NOT SHIPPABLE AS CONFIGURED" STOOD HERE AND IS WITHDRAWN 2026-08-15
# (artifacts/performance/2026-08-15_battlepack-arena-price/ARENA_PRICE.md). Two
# of its three premises were wrong. (1) The growth is 172,032 B, not 258,048:
# the cache behind the pack is 163,840, not 262,144. (2) The growth does not
# spend boot headroom on top of the pack -- it REPAYS the pack's own reservation.
# 1,548,288 - 451,776 leaves taskman 1,096,512 against the shipping arm's
# 1,376,256 - 262,144 = 1,114,112, i.e. 17,600 B LESS, so this arm's general heap
# ends tighter than the control's rather than looser. (3) Measured on the stress
# battery rather than projected: 12 battle entries, 7 matches, 7 START restarts,
# 4 Sudden Deaths, NO-FREEZE, with ChosenSize 1,548,288 / AllocFail 0 /
# ReserveFail 0 / Rejects 0 / SyMallocOverflow 0, general-heap low-water 52,400
# against the 32,768 floor and the GObj cap never firing.
#
# What is still true and still binding: this arena is not free RAM. It is
# grantable libnds heap, the measured grantable ceiling is 1,564,672, and 0x17a000
# sits 16,384 B under it. Any static growth on top of this arm eats that 16,384
# one for one, silently, and the ONLY thing that sees it is
# gNdsTaskmanArenaChosenSize -- never check-boot-headroom.ps1, which meters the
# static image. Re-read it after any change here.
#
# OWNER DECISION 2026-08-17: THE DEFAULT IS 1, and it moves as a PAIR with
# NDS_R2_BATTLEPACK above. The -34,304 was measured with both flags on, and the
# pack WITHOUT this arm is the phase-8 configuration whose carve deleted the raw
# file cache and cost +2,261,376 at rank-80. Never ship one of the two without
# the other; a build that sets only one is a configuration nothing has measured.
NDS_R2_BATTLEPACK_KEEP_CACHE ?= 1
# THE SLICE-51 FALSIFIER, and it exists because two arms that disagree about the
# arena, the cache size (40x) and the ROM loads (14x) still landed 384 ticks
# apart at rank-80 (`.../2026-08-15_battlepack-isolation/BATTLEPACK_ISOLATION.md`).
# The only thing they share is the pack PATH, and "the pack path" is still two
# things: the blob's PRESENCE (streamed, carved, resident in the arena) and its
# DISPATCH (a clip pointer handed to the animation machinery).
#
# At 0 the blob is streamed, validated, adopted and carved out of the arena
# exactly as at 1 -- every residency counter reads the same -- but
# ndsBattlePackFindFigatree answers NULL, so nothing dispatches through it and
# gNdsBattlePackHits reads 0 against a control that reads 197. Presence-only cost
# ~0 says the cost is in the dispatch; presence alone reproducing it says the
# pack format is exonerated and residency itself is the problem.
#
# Runtime-gated rather than #if'd on purpose: both arms then carry the same
# instructions and the same layout, so the comparison does not also measure the
# linker. Only the .data initializer differs. Requires NDS_R2_BATTLEPACK=1; at
# BATTLEPACK=0 the global does not exist and this is inert.
NDS_R2_BATTLEPACK_DISPATCH ?= 1
# Cycle 109. Builds BOTH arms of the two animation cuts into one binary, selected
# at runtime by gNdsR2AnimCutRoute, so they can be priced without a second link.
# This is standing rule 7's route, and after the determinism finding it is the
# ONLY method that can separate a cut in the 1,000-5,000 tick class from the
# 14,080-tick placement term.
#
# Default 0 and it must STAY 0 for anything published: at 1 the joint loop pays a
# register test per AObj node plus a spill, and "replace, don't coexist" is a
# board rule with a measured price (1.85 cycles of FTR mean per byte of added ARM
# text). At 0 every route test folds to a constant and the pre-cut arms are
# dead-coded away, so the shipped ROM is byte-for-byte the no-route program.
NDS_R2_ANIM_CUT_ROUTE ?= 0
# Cycle 116. Same instrument, applied to the Task 56 fighter strips: at 1 the
# raw-corner emitters and the primitive-group emitter are BOTH compiled and
# `gNdsR2FighterStripRoute` selects between them at run time, so the strip A/B
# runs on ONE binary. Task 56 was killed in 2026-07 against a control built
# three days earlier and ~31 KB smaller, which is not a control at all on a ROM
# whose pacing is placement-sensitive. Needs NDS_TASK56_FIGHTER_PRIMITIVES >= 1
# for the strip arm to exist; at 0 the route folds away and the selected
# emitter is whatever that flag chose.
NDS_R2_STRIP_ROUTE ?= 0
# Cycle 118. The same instrument for the map-collision lane. At 1 the endpoint
# memo in `ndsMPFindLineEndpoints` is selected by `gNdsR2MPRoute` at run time, so
# its A/B runs on ONE binary -- which is the only way to read a cut of its size
# (5,861 tk/fr of a flat 543-cycle function) against the +-8,544 cross-build
# placement floor. At 0 the test folds to a constant, the memo ships, and the
# ROM carries no route check. The memo is FILLED in both arms so the control
# pays the identical fill and only the lookup differs.
NDS_R2_MP_ROUTE ?= 0
# Slice 45. Same-binary A/B for the fighter-AObj16 alias-scan reorder in
# ndsRelocRemoveFighterAObj16StatusAliases. At 1 the ROM carries both arms and
# `-SetGlobals gNdsR2RelocAliasRoute=0` selects the original operand order at
# identical placement; at 0 the test folds to a constant and the reorder ships
# with no route check.
#
# BANKED at 0. One binary, builds/build-c122-alias, 1600 frames from 438,
# NDS_R2_BOTH_CPU=1, DLDI ON: Resolves 16,002 -> 1,143 of the same 16,067 node
# visits (-92.9%), WORK-H P95 1,227,456 -> 1,215,296 (-12,160), P50 936,448 ->
# 936,704 (+256, noise). Set to 1 only to re-measure it.
NDS_R2_RELOC_ALIAS_ROUTE ?= 0
# R2-03 E47. The native fighter owner derives its material colour and its
# use-material predicate from `stats` per epoch, the way the generic path does,
# instead of reading a baked policy flag and always taking prim_color. The
# generic path picks env_color whenever the combiner outputs ENVIRONMENT and
# returns 0 with no combiner at all, and its predicate is a function of the live
# combiner -- so on hitlag frames, where Task 39's hurt flash is the only thing
# that varies the epoch state (E34), the baked answer draws the struck fighter
# dark maroon against the generic path's light grey (E32). Once per epoch,
# 46.4/frame.
NDS_R2_MATERIAL_DYNAMIC ?= 0
# R2-03 E48 lab probe. Counts which branch of the generic colour path draws each
# vertex, per presented frame, latched at one hitlag frame and one ordinary one.
# Four hypotheses have been spent guessing at E32's regression (E36, E41, E42,
# E47) and each cost a build; this measures the branch instead. Delete with the
# experiment.
NDS_R2_FLASH_PROBE ?= 0
# R2-03 E61. Counting-only interposition on gcPlayDObjAnimJoint (the Task 95
# mechanism), reporting the AObj kind mix, the anim_speed value set and the
# NOANIM skip count. E60 priced this path at 146,942 ticks/frame inclusive and
# 280 ticks per node; these three integers decide whether the repair is a
# load-time pose table (which needs an integral frame index), a fixed-point
# cubic, or neither. Delete with the experiment.
NDS_R2_ANIM_CENSUS ?= 0
# R2-03 E64. The cubic Hermite in gcPlayDObjAnimJoint evaluated in Q12 fixed
# point instead of 14 soft-float operations. E60/E61 priced the cubic at 149.4
# evaluations a frame x ~405 ticks = 60,509 ticks/frame, which is 99.6% of the
# animation path's float. Step and Linear nodes keep the decomp's own
# expressions, so 45.3% of nodes stay bit-identical.
#
# NOT bit-exact by design, and owner-authorized 2026-07-29 on that basis:
# PROJECT_GOAL.md requires mechanical equivalence and lists "fixed-point
# replacements" as allowed, while the Task 9 state hash asserts the stronger
# bit-exact property. Expect that hash to move.
NDS_R2_CUBIC_FIXED ?= 0
# R2-06 E8. Price ndsRelocFinalizeLoadedFile's five passes separately. E8 traced
# 8 of the 9 over-gate frames to the 16 frames on which that function runs -- they
# carry WORK-H median 1,113,152 against 974,080 clean, all of it in SRC, while the
# clean-frame P95 of 1,056,640 is INSIDE the 1.12M gate by 63,360. So the milestone
# turns on moving the relocation out of the gameplay frame, and which pass
# dominates decides whether the repair is a post-fixup cache or one destination
# buffer per resident animation. Instrument only, no behaviour change, lab default
# off: it adds 8 words of BSS and this ROM's placement noise floor is 5,000-7,000,
# so a run with it on is not comparable to an ordinary tick-HUD baseline.
NDS_R2_RELOC_FIXUP_TIMING ?= 0
# R2-06 E10. Price the ACTION CHANGE, which E8 showed is ~78% of the load-frame
# premium -- the relocation is only 21.5%, so the gate lever is here rather than in
# the reloc path. Brackets the two already-interposed animation-add wrappers
# (gcAddDObjAnimJoint, gcAddAnimJointAll), splitting the O2R script normalization
# from the decomp's own setup. Instrument only, lab default off.
NDS_R2_LOADFRAME_TIMING ?= 0
# Switch plan R2-06 harness prerequisite, owner-requested 2026-07-29. On the
# historical direct-boot R2 targets it makes player 0 a level-3 CPU as well, so
# both existing fighters attack continuously with no recorded input stream. P2-2
# reuses the switch on MENU-SHELL builds as the four-CPU seed instead: the match
# descriptor adds Mario/Fox mirrors in P3/P4 at level 3. The direct-boot meaning
# is deliberately unchanged so its banked measurements remain comparable.
# Either form is a deliberate STRESS case: it maximises the live hitbox
# population that R2-03 E35 named as the owner of the SRC P95 excursion, which
# also makes it the configuration that most exercises E64b's Q12 cubic (more hit
# decisions to flip) and E32's hitlag fallback (more bursts).
#
# NEVER report a P95 from this build as the Boundary figure. The switch plan
# defines the shipped Boundary as Mario human vs level-3 Fox CPU at mode 163 and
# PROJECT_GOAL.md's gate as representative gameplay; this is harder than either.
NDS_R2_BOTH_CPU ?= 0
# P2-2's direct-battle standing gate. Unlike NDS_R2_BOTH_CPU this has no
# historical two-player meaning to preserve: it seeds four level-3 CPU fighter
# INSTANCES (Mario/Fox/Mario/Fox) while the generated renderer owners remain the
# two fighter KINDS. Only the dedicated P2 stress target below enables it.
NDS_P2_FOUR_CPU_STRESS ?= 0
# P2 STRESS SET: FOUR DISTINCT LANDED KINDS RATHER THAN FOUR INSTANCES.
#
# The four-CPU stress arm seeds four VSBattle INSTANCES of the two kinds the
# content set had when it was written (Mario/Fox/Mario/Fox). PROJECT_GOAL's P2
# gate asks for something stricter and content-dependent -- "the measured
# hardest fighter set", an argmax over LANDED content -- and the roster is now
# four names. This flag seeds slots 2/3 with Luigi and Donkey Kong instead of
# the mirrors, which is simultaneously the heaviest single-console case a
# player can reach and the measurement that answers the owner's 2026-08-23
# report that "some combinations of fighters run at really low FPS".
#
# PROMOTED TO THE GATE DEFAULT ON THE STRESS TARGET (board row P2-3r15,
# 2026-08-25). It was a lab flag until P2-3r13 showed four distinct kinds fit
# the SHIPPING configuration -- pack resident, cache untrimmed, general-heap
# low-water 49,956 B against the 25,600 floor -- at which point the mirror
# roster stopped being an honest argmax over landed content. Boundary's
# `p2_fourcpu_stress` therefore builds Mario/Fox/Luigi/Donkey.
#
# `NDS_P2_FOUR_CPU_ROSTER=0` still builds the mirror arm and is the A/B control;
# the stress harness stamps which roster produced every figure into
# `p2-2-fourcpu-memory.json` (`fighterRoster`), because a tick number from one
# roster is not comparable with a tick number from the other.
#
# The "needs the four-name roster" check is a `#error` in nds_match_config.c
# rather than a `$(error)` here, because NDS_P2_DONKEY is not defined until
# below and a make-time test would read it empty and never fire.
NDS_P2_FOUR_CPU_ROSTER ?= \
	$(if $(filter smash64ds-p2-fourcpu-tickhud-hwtri,$(TARGET)),1,0)
# The four kinds the roster arm instantiates (BattleShip fttypes.h ordinals:
# Mario 0, Fox 1, Donkey 2, Samus 3, Luigi 4, Link 5, Yoshi 6, Captain 7,
# Kirby 8, Pikachu 9, Purin 10, Ness 11). The defaults are the P2-3f22 argmax
# Samus/Fox/Captain/Donkey; a fighter row measures itself under the stress
# config by overriding a slot (`NDS_P2_FOUR_CPU_KIND0=6 NDS_P2_YOSHI=1`), and
# nds_match_config.c refuses a kind whose admission flag is off. The verifier
# reads these back out of nds_build_config.h to name the roster it expects.
NDS_P2_FOUR_CPU_KIND0 ?= 3
NDS_P2_FOUR_CPU_KIND1 ?= 1
NDS_P2_FOUR_CPU_KIND2 ?= 7
NDS_P2_FOUR_CPU_KIND3 ?= 2
# P2-3 fighter-production admission flag.  A fighter is staged behind its own
# flag until the source-derived asset graph, source status table, native owner,
# CSS/audio surfaces and focused runtime proof are all green.  This prevents a
# half-imported fighter from changing the P2-2 standing Boundary merely because
# its files exist in the tree.  Luigi is the first pipeline prover.
# P2-3 shell roster switch (owner, 2026-08-23: "I want to be able to test out
# Luigi and DK").  How many IN-PROGRESS fighters every shell configuration
# carries, in admission order: 0 = Mario/Fox only (the control arm for any
# regression that lands with a wider roster), 1 = + Luigi, 2 = + Donkey Kong.
#
# THIS IS A RAM BUDGET, NOT A PREFERENCE.  A fighter's generated native-owner
# tables cost the taskman arena, which is calloc'd from whatever the heap has
# left, so they are charged either to the ARM9 binary (always, for every
# fighter built) or to the arena (only while a fighter is in the scene).  Board
# row P2-3r4 moved them to the second: the tables ship as NitroFS images and
# are loaded for the fighters a match actually uses.
#
# Measured on the shell target at the battle's own high water, gdb, one build
# each.  The first three rows are the pre-P2-3r4 arrangement (tables in the
# binary) and are kept because they are what makes the fourth row legible:
#
#   2026-08-23, tables in the ARM9 binary
#     roster 0  arena 1,548,288  used 1,456,624  headroom 91,664  battle OK
#     roster 1  arena 1,515,520  used 1,458,384  headroom 57,136  battle OK
#     roster 2  arena 1,470,464  used 1,456,624  headroom 13,840  ABORT
#   2026-08-24, tables in NitroFS images (P2-3r4)
#     roster 2  arena 1,503,232  used 1,458,384  headroom 44,848  battle OK
#
# At roster 2 the battle used to die at `ifCommonCountdownMakeInterface + 120`
# (data abort, cpsr ABT) because the countdown interface's taskman allocation
# came back NULL.  The image move buys back 31,008 B of headroom and the same
# walk now completes a Donkey Kong match -- that measurement is a DK battle,
# not a Mario one, because at roster 2 the walk's old "locked" negative control
# is a BUILT fighter and the token drops on it.  The walk's control moved to
# Link for exactly that reason; the arena figure above is the DK match it
# accidentally proved first.
#
# WORST CASE IS LUIGI VERSUS DONKEY, both images resident at once: 36,276 B
# against the 20,200 B the measured run held, so 28,772 B of headroom.  That is
# the number a future fighter has to be sized against, not the 44,848.
# 2026-09-04: BACK TO 7. Rung 8 (Jigglypuff) makes the character select read
# ten full fighter closures from NitroFS -- 904,656 B against 832,288 at rung
# 7 -- and the Boundary shell lap then hung inside libfat `get_fat` on the
# transition into the select screen, blowing the harness's 3000 s budget on
# both the run and its retry. The eager load is the real defect, not
# Jigglypuff: see docs/p2/P2-3-fighter-production.md. Raise this again once
# the character select stops loading every roster member at once.
NDS_P2_SHELL_ROSTER ?= 7

# THE ROSTER LADDER, DEFINED ONCE AND EVALUATED IN ALL THREE SHELL TARGETS.
#
# Rung N is N fighters beyond Mario and Fox, cumulatively, so lowering
# NDS_P2_SHELL_ROSTER is how a fighter comes back OUT of the owner ROM without
# touching anything else. The in-progress roster ships in EVERY shell
# configuration -- the published ROM, its free-play twin, the gate's realtime
# arm and the loop arm -- so the playable roster is the verifier-covered one,
# and the CSS marks every unfinished production fighter with the question-mark
# overlay the generator bakes (owner, 2026-08-23: "I want to be able to test
# out Luigi and DK").
#
# It lives in a `define` because it used to be COPIED into those three target
# blocks. Rung 7 shipped broken for exactly that reason: only Samus's filter
# was extended, so Donkey and Captain silently dropped out of the ROM and the
# built config header was the only thing that said so. Three identical copies
# of a cumulative filter list cannot be kept in step by hand, and the publish
# law wants the owner ROM and the arms that verify it to be ONE configuration.
# Now they cannot diverge: add a rung here and all three targets get it.
#
# Link is board row P2-3f33 PARTIAL (static and native checks green, runtime
# acceptance owed), so he rides as playable-but-unaccepted; the question-mark
# overlay is what says so on screen. Rung 8 is Jigglypuff, added 2026-09-04:
# both his known defects closed 2026-09-03 runtime-confirmed (P2-3f50 and
# P2-3f51 -- 76 presented frames with every fixup, openfail, streamfail and
# anim-fallback counter at 0), and at 72,368 B he is the smallest closure after
# Mario. He borrows 77 Kirby animation files, which P2-3f51 proved resolve from
# a Purin-only build.
define NDS_P2_SHELL_ROSTER_LADDER
override NDS_P2_LUIGI := $(if $(filter 0,$(NDS_P2_SHELL_ROSTER)),0,1)
override NDS_P2_DONKEY := $(if $(filter 2 3 4 5 6 7 8,$(NDS_P2_SHELL_ROSTER)),1,0)
override NDS_P2_CAPTAIN := $(if $(filter 3 4 5 6 7 8,$(NDS_P2_SHELL_ROSTER)),1,0)
override NDS_P2_SAMUS := $(if $(filter 4 5 6 7 8,$(NDS_P2_SHELL_ROSTER)),1,0)
override NDS_P2_LINK := $(if $(filter 5 6 7 8,$(NDS_P2_SHELL_ROSTER)),1,0)
override NDS_P2_PIKACHU := $(if $(filter 6 7 8,$(NDS_P2_SHELL_ROSTER)),1,0)
override NDS_P2_YOSHI := $(if $(filter 7 8,$(NDS_P2_SHELL_ROSTER)),1,0)
override NDS_P2_PURIN := $(if $(filter 8,$(NDS_P2_SHELL_ROSTER)),1,0)
endef
# P2-6, the 1P Game campaign.
#
# The ladder tables and bonus counters come from the textual include of
# sc1pgame.c in battleship_sc1pgame_runtime.c (2026-09-04); the transcribed
# battleship_sc1pgame_tables.c was deleted once the include owned them.
NDS_P2_1P_GAME ?= 0
NDS_P2_LUIGI ?= 0
# Donkey is the first structurally different P2-3 owner.  Keep admission
# sequential: native-owner slots are a dense ABI (Mario/Fox/Luigi/Donkey), so a
# Donkey build also carries the already-qualified Luigi owner instead of
# inventing a hole or a build-dependent owner number.
NDS_P2_DONKEY ?= 0
ifeq ($(NDS_P2_DONKEY),1)
ifneq ($(NDS_P2_LUIGI),1)
$(error NDS_P2_DONKEY=1 requires NDS_P2_LUIGI=1 so native-owner slots stay dense)
endif
endif
# P2-3f5. Captain Falcon's admission flag.
#
# THIS DEFAULTS TO 0 AND NO SHIPPED CONFIGURATION SETS IT YET, and that is an
# honest statement of how far the fighter has come, not an oversight. The flag
# turns on his source status table, his three special state machines
# (ftcaptainspecialn/lw/hi), Falcon Dive's victim side, and his two-status entry
# ladder -- those are complete and link. What is NOT done is everything that
# would make him SELECTABLE: the native-owner tables (this row unblocked the
# decode, but Falcon is not a runtime owner yet), the CSS/HUD surfaces, the
# audio ordinals, and the arena budget a third in-progress fighter costs. The
# worst case NDS_P2_SHELL_ROSTER is sized against is still Luigi versus Donkey
# at 36,276 B of resident owner images; Falcon has none, so admitting him to the
# roster today would put his model on the N64 interpreter path with no
# measurement behind it. Build the slice with:
#
#   make TARGET=smash64ds BUILD=build-<lab> NDS_P2_CAPTAIN=1
#
# THERE IS DELIBERATELY NO "REQUIRES NDS_P2_DONKEY=1" LADDER CHECK HERE, unlike
# the Donkey/Luigi pair above, and the reason is not laziness in both
# directions. That check exists because a native-owner SLOT is a dense ABI, and
# Falcon occupies no slot -- he has no generated owner tables at all. A check
# would also be unreadable at this point in the file: every shell target sets
# NDS_P2_LUIGI/NDS_P2_DONKEY by `override` around line 2500, more than 1,900
# lines BELOW this one, so a test here reads the `?=` defaults and reds on a
# command line that is in fact correct -- the same evaluation-order trap the
# NDS_P2_FOUR_CPU_ROSTER comment above had to push into a C `#error`. Reinstate
# the ladder when Falcon becomes a native owner, and put it where the flags are
# final.
NDS_P2_CAPTAIN ?= 0
# P2-3 Samus source-gameplay admission. Her native owner is source-derived from
# the non-contiguous setup_parts mask; the generator mirrors BattleShip's
# per-descriptor bit walk instead of requiring the older prefix-shaped masks.
NDS_P2_SAMUS ?= 0
# P2-3 fighter 5. Link remains opt-in until his source state machines, real
# boomerang weapon, LinkBomb item client, native owner, CSS/audio surfaces and
# runtime proofs are all admitted. The production manifest may know his files
# before this flips in a shipping shell; that is intentional staging.
NDS_P2_LINK ?= 0
# P2-3 fighter 6. Pikachu stays opt-in until his source specials (Thunder
# Jolt, Thunder, Quick Attack), both weapon articles, native owner, CSS/audio
# surfaces and runtime proofs are admitted; the manifest knows his files first.
NDS_P2_PIKACHU ?= 0
# P2-3 fighter 7. Yoshi stays opt-in until his source specials (Egg Lay's
# two-body capture, Egg Throw, Yoshi Bomb), both weapon articles, the egg
# shield / double-jump armor seams, native owner, CSS/audio surfaces and
# runtime proofs are admitted; the manifest knows his files first.
NDS_P2_YOSHI ?= 0
# P2-3 fighter: Ness stays opt-in until his source specials, articles, native
# owner, CSS/audio surfaces and runtime proofs are admitted (admit_fighter.py).
NDS_P2_NESS ?= 0
# P2-3 fighter: Purin stays opt-in until his source specials, articles, native
# owner, CSS/audio surfaces and runtime proofs are admitted (admit_fighter.py).
NDS_P2_PURIN ?= 0
# DERIVED, not a knob: BattleShip's shared item subsystem (itmain/itmap/
# itmanager/itprocess/itvisuals, imported by battleship_item_link_core.c).
# It is not Link's -- LinkBomb was only its first client -- so it compiles for
# any fighter whose articles include an item. Ness's PK Fire pillar is the
# second; P2-5 makes it unconditional. Setting this by hand does nothing useful.
#
# P2-3f48 adds three more clients and one evaluation-order fix. Kirby's spit
# and lose-copy stars and the Master Ball entry article for Pikachu and
# Jigglypuff all read their descriptors out of gITManagerCommonData, which
# this subsystem's itManagerInitItems is what loads -- so a ROM admitting any
# of them needs the item core for the same reason Link and Ness do.
#
# THE ASSIGNMENT IS DEFERRED ON PURPOSE. NDS_P2_KIRBY is defaulted BELOW this
# line, so a `:=` here would read an empty value for him and silently leave
# his stars stubbed on a Kirby-only build. This is the same trap the
# NDS_P2_CAPTAIN comment above describes for its ladder check, avoided rather
# than worked around: `=` is evaluated where it is used, by which point every
# admission flag is final.
# P2-4 adds a second reason to need the item core, and it is not a fighter:
# Peach's Castle's only hazard is an item. grcastle.c:57 spawns the bumper
# through itManagerMakeItemSetupCommon, so a Castle build without the core
# does not link at all. Mushroom Kingdom and Saffron City will join this list
# for the same reason.
NDS_P2_ITEM_CORE = $(if $(filter 1,$(NDS_P2_LINK) $(NDS_P2_NESS) \
	$(NDS_P2_PIKACHU) $(NDS_P2_PURIN) $(NDS_P2_KIRBY) \
	$(NDS_P2_STAGE_CASTLE) \
	$(NDS_P2_STAGE_YAMABUKI) \
	$(NDS_P2_STAGE_INISHIE)),1,0)
# P2-3 fighter: Kirby stays opt-in until his source specials, articles, native
# owner, CSS/audio surfaces and runtime proofs are admitted (admit_fighter.py).
NDS_P2_KIRBY ?= 0
# P2-3 fighter: GDonkey stays opt-in until his source specials, articles, native
# owner, CSS/audio surfaces and runtime proofs are admitted (admit_fighter.py).
NDS_P2_GDONKEY ?= 0
# P2-3 fighter: MMario stays opt-in until his source specials, articles, native
# owner, CSS/audio surfaces and runtime proofs are admitted (admit_fighter.py).
NDS_P2_MMARIO ?= 0
# P2-4 first stage: Yoshi's Island (Yoster). It belongs HERE with the other
# admission flags and not down beside its reloc file list, because the CFILES
# gate at :3851 and the nds_build_config.h line both read it before that point
# in the file. A `?=` placed later left the config header emitting
# `#define NDS_P2_STAGE_YOSTER` with an EMPTY value, which is not the same as 0
# -- `#if` on an empty macro is "#if with no expression", a hard compile error,
# and `#if !` on one is "operator '!' has no right operand". Both fired on the
# default build.
NDS_P2_STAGE_YOSTER ?= 0
# P2-4 stage 2: Peach's Castle (Castle). Same reason as Yoster above for
# living here rather than beside its reloc list.
NDS_P2_STAGE_CASTLE ?= 0
# P2-4 stage 3: Congo Jungle (Jungle). Same reason as the two above for
# living here rather than beside its reloc list.
NDS_P2_STAGE_JUNGLE ?= 0
# P2-4 stage 4: Planet Zebes (Zebes).
NDS_P2_STAGE_ZEBES ?= 0
# P2-4 stage 5: Hyrule Castle (Hyrule).
NDS_P2_STAGE_HYRULE ?= 0
# P2-4 stage 6: Saffron City (Yamabuki).
NDS_P2_STAGE_YAMABUKI ?= 0
# P2-4 stage 7: Mushroom Kingdom (Inishie).
NDS_P2_STAGE_INISHIE ?= 0
# P2-4 stage 8: Sector Z (Sector), the largest stage in the game.
NDS_P2_STAGE_SECTOR ?= 0
# P2-3f9. THE HEAVIEST ROSTER A PLAYER CAN REACH, MEASURED FROM THE SHELL.
#
# `NDS_P2_FOUR_CPU_ROSTER` above is a DIRECT-BATTLE arm: its target sets
# NDS_P2_FOUR_CPU_STRESS and, in nds_match_config.c's own words, "bypasses
# PlayersVS". The shell is a different entry path -- its character select makes
# every selectable kind's main files resident for the live previews, and its
# larger ARM9 binary leaves a SMALLER scene arena -- so a four-kind figure from
# that arm does not predict one from the shell, and until this flag existed
# nobody had ever run four distinct kinds through the shipped menus at any
# roster.
#
# 1 seeds the P2-1a descriptor with the measured argmax over landed content.
# Since P2-3f22 that is Samus human + Fox/Captain/Donkey level-3 CPUs, so the
# character select opens on the same four kinds the direct stress arm measures.
# The CSS walk presses START without disturbing the four seeded slots. It changes
# nothing else: the whole shell runs, the battle loads whatever the CSS committed,
# and the arena spends what it spends.
# Deliberately NOT folded into NDS_P2_FOUR_CPU_ROSTER -- one flag naming two
# different rosters on two different entry paths is how a measurement gets
# attributed to the wrong arm.
NDS_P2_SHELL_ARGMAX_ROSTER ?= 0
# P2-3r4. WHERE A P2-3 OWNER'S GENERATED TABLES LIVE.
#
# 1 = NitroFS image (the default, and the only thing that scales): the owner's
# 21 generated arrays are compiled into a standalone object, objcopied into
# `nitrofs/fighters/<owner>_<detail>.bin`, and guarded OUT of the ARM9 binary,
# where each byte costs the taskman arena one byte. The runtime loads only the
# owners a match actually uses.
#
# 0 = the pre-P2-3r4 arrangement, arrays in the binary. Kept because it is the
# control arm for the byte-equality check and the only way to bisect a
# suspected loader fault against known-good tables -- not because it is a
# supported shipping configuration. It does not fit a four-name roster.
NDS_NATIVE_OWNER_IMAGE ?= 1
# The proof arm for the flag above. 1 keeps the arrays AND loads the image,
# then compares them member by member at fighter creation. It is only
# meaningful with NDS_NATIVE_OWNER_IMAGE=0 -- with the arrays guarded out
# there is nothing to compare against, and the compare compiles to nothing.
NDS_NATIVE_OWNER_IMAGE_VERIFY ?= 0
NDS_NATIVE_OWNER_IMAGE_LUIGI = $(if $(filter 1,$(NDS_NATIVE_OWNER_IMAGE)),$(NDS_P2_LUIGI),0)
NDS_NATIVE_OWNER_IMAGE_DONKEY = $(if $(filter 1,$(NDS_NATIVE_OWNER_IMAGE)),$(NDS_P2_DONKEY),0)
NDS_NATIVE_OWNER_IMAGE_CAPTAIN = $(if $(filter 1,$(NDS_NATIVE_OWNER_IMAGE)),$(NDS_P2_CAPTAIN),0)
NDS_NATIVE_OWNER_IMAGE_SAMUS = $(if $(filter 1,$(NDS_NATIVE_OWNER_IMAGE)),$(NDS_P2_SAMUS),0)
NDS_NATIVE_OWNER_IMAGE_LINK = $(if $(filter 1,$(NDS_NATIVE_OWNER_IMAGE)),$(NDS_P2_LINK),0)
NDS_NATIVE_OWNER_IMAGE_PIKACHU = $(if $(filter 1,$(NDS_NATIVE_OWNER_IMAGE)),$(NDS_P2_PIKACHU),0)
NDS_NATIVE_OWNER_IMAGE_YOSHI = $(if $(filter 1,$(NDS_NATIVE_OWNER_IMAGE)),$(NDS_P2_YOSHI),0)
NDS_NATIVE_OWNER_IMAGE_NESS = $(if $(filter 1,$(NDS_NATIVE_OWNER_IMAGE)),$(NDS_P2_NESS),0)
NDS_NATIVE_OWNER_IMAGE_PURIN = $(if $(filter 1,$(NDS_NATIVE_OWNER_IMAGE)),$(NDS_P2_PURIN),0)
NDS_NATIVE_OWNER_IMAGE_KIRBY = $(if $(filter 1,$(NDS_NATIVE_OWNER_IMAGE)),$(NDS_P2_KIRBY),0)
NDS_NATIVE_OWNER_IMAGE_MMARIO = $(if $(filter 1,$(NDS_NATIVE_OWNER_IMAGE)),$(NDS_P2_MMARIO),0)
# P2-3 focused fighter-production proof selector. -1 leaves the canonical
# Mario-vs-Fox descriptor byte-for-byte unchanged; a non-negative value is an
# nFTKind* integer used only for fighter slot 0 in direct-battle proof builds.
# It does NOT admit assets by itself: Luigi is therefore exercised with
# NDS_P2_LUIGI=1 NDS_P2_PROOF_FIGHTER0=4, and later fighters reuse this same
# descriptor seam behind their own production flags.
NDS_P2_PROOF_FIGHTER0 ?= -1
# P2-3 Samus source-state tour.  This is a proof-only guest driver layered on
# the existing mode-163 controller playback.  It may stage geometry/damage
# preconditions in guest code so writes are ARM9-cache coherent, but it never
# assigns fighter status/motion or calls a status setter: BattleShip collision
# and common-state interrupts must select every accepted ledge/tumble state.
NDS_P2_SAMUS_STATE_TOUR ?= 0
ifneq ($(filter 0 1,$(NDS_P2_SAMUS_STATE_TOUR)),$(NDS_P2_SAMUS_STATE_TOUR))
$(error NDS_P2_SAMUS_STATE_TOUR must be 0 or 1)
endif
NDS_P2_SAMUS_TUMBLE_TOUR ?= 0
ifneq ($(filter 0 1,$(NDS_P2_SAMUS_TUMBLE_TOUR)),$(NDS_P2_SAMUS_TUMBLE_TOUR))
$(error NDS_P2_SAMUS_TUMBLE_TOUR must be 0 or 1)
endif
NDS_P2_SAMUS_DAMAGEFLY_TOUR ?= 0
ifneq ($(filter 0 1,$(NDS_P2_SAMUS_DAMAGEFLY_TOUR)),$(NDS_P2_SAMUS_DAMAGEFLY_TOUR))
$(error NDS_P2_SAMUS_DAMAGEFLY_TOUR must be 0 or 1)
endif
NDS_P2_SAMUS_ATTACK_TOUR ?= 0
ifneq ($(filter 0 1,$(NDS_P2_SAMUS_ATTACK_TOUR)),$(NDS_P2_SAMUS_ATTACK_TOUR))
$(error NDS_P2_SAMUS_ATTACK_TOUR must be 0 or 1)
endif
# P2-3 LinkBomb source-lifecycle proof.  The guest driver feeds ordinary
# controller input only; BattleShip must select SpecialLw, manufacture the
# shared item, throw it, run the critical fuse/explosion and destroy it.
NDS_P2_LINK_BOMB_TOUR ?= 0
ifneq ($(filter 0 1,$(NDS_P2_LINK_BOMB_TOUR)),$(NDS_P2_LINK_BOMB_TOUR))
$(error NDS_P2_LINK_BOMB_TOUR must be 0 or 1)
endif
NDS_P2_LINK_SPECIAL_TOUR ?= 0
ifneq ($(filter 0 1,$(NDS_P2_LINK_SPECIAL_TOUR)),$(NDS_P2_LINK_SPECIAL_TOUR))
$(error NDS_P2_LINK_SPECIAL_TOUR must be 0 or 1)
endif
ifeq ($(NDS_P2_LINK_SPECIAL_TOUR),1)
ifneq ($(NDS_P2_LINK),1)
$(error NDS_P2_LINK_SPECIAL_TOUR=1 requires NDS_P2_LINK=1)
endif
ifneq ($(NDS_P2_PROOF_FIGHTER0),5)
$(error NDS_P2_LINK_SPECIAL_TOUR=1 requires NDS_P2_PROOF_FIGHTER0=5)
endif
ifneq ($(NDS_P2_LINK_BOMB_TOUR),0)
$(error Link special and LinkBomb tours are separate proof arms)
endif
ifneq ($(NDS_P2_SAMUS_STATE_TOUR),0)
$(error Link special and Samus state tours are separate proof arms)
endif
ifneq ($(NDS_P2_SAMUS_TUMBLE_TOUR),0)
$(error Link special and Samus tumble tours are separate proof arms)
endif
ifneq ($(NDS_P2_SAMUS_DAMAGEFLY_TOUR),0)
$(error Link special and Samus damage-fly tours are separate proof arms)
endif
ifneq ($(NDS_P2_SAMUS_ATTACK_TOUR),0)
$(error Link special and Samus attack tours are separate proof arms)
endif
endif
ifeq ($(NDS_P2_LINK_BOMB_TOUR),1)
ifneq ($(NDS_P2_LINK),1)
$(error NDS_P2_LINK_BOMB_TOUR=1 requires NDS_P2_LINK=1)
endif
ifneq ($(NDS_P2_PROOF_FIGHTER0),5)
$(error NDS_P2_LINK_BOMB_TOUR=1 requires NDS_P2_PROOF_FIGHTER0=5)
endif
ifneq ($(NDS_P2_SAMUS_STATE_TOUR),0)
$(error LinkBomb and Samus state tours are separate proof arms)
endif
ifneq ($(NDS_P2_SAMUS_TUMBLE_TOUR),0)
$(error LinkBomb and Samus tumble tours are separate proof arms)
endif
ifneq ($(NDS_P2_SAMUS_DAMAGEFLY_TOUR),0)
$(error LinkBomb and Samus damage-fly tours are separate proof arms)
endif
ifneq ($(NDS_P2_SAMUS_ATTACK_TOUR),0)
$(error LinkBomb and Samus attack tours are separate proof arms)
endif
endif
ifeq ($(NDS_P2_SAMUS_ATTACK_TOUR),1)
ifneq ($(NDS_P2_SAMUS),1)
$(error NDS_P2_SAMUS_ATTACK_TOUR=1 requires NDS_P2_SAMUS=1)
endif
ifneq ($(NDS_P2_PROOF_FIGHTER0),3)
$(error NDS_P2_SAMUS_ATTACK_TOUR=1 requires NDS_P2_PROOF_FIGHTER0=3)
endif
ifneq ($(NDS_P2_SAMUS_STATE_TOUR),0)
$(error NDS_P2_SAMUS_ATTACK_TOUR and NDS_P2_SAMUS_STATE_TOUR are separate proof arms)
endif
ifneq ($(NDS_P2_SAMUS_TUMBLE_TOUR),0)
$(error NDS_P2_SAMUS_ATTACK_TOUR and NDS_P2_SAMUS_TUMBLE_TOUR are separate proof arms)
endif
ifneq ($(NDS_P2_SAMUS_DAMAGEFLY_TOUR),0)
$(error NDS_P2_SAMUS_ATTACK_TOUR and NDS_P2_SAMUS_DAMAGEFLY_TOUR are separate proof arms)
endif
endif
ifeq ($(NDS_P2_SAMUS_TUMBLE_TOUR),1)
ifneq ($(NDS_P2_SAMUS),1)
$(error NDS_P2_SAMUS_TUMBLE_TOUR=1 requires NDS_P2_SAMUS=1)
endif
ifneq ($(NDS_P2_PROOF_FIGHTER0),3)
$(error NDS_P2_SAMUS_TUMBLE_TOUR=1 requires NDS_P2_PROOF_FIGHTER0=3)
endif
ifneq ($(NDS_P2_SAMUS_STATE_TOUR),0)
$(error NDS_P2_SAMUS_TUMBLE_TOUR and NDS_P2_SAMUS_STATE_TOUR are separate proof arms)
endif
ifneq ($(NDS_P2_SAMUS_DAMAGEFLY_TOUR),0)
$(error NDS_P2_SAMUS_TUMBLE_TOUR and NDS_P2_SAMUS_DAMAGEFLY_TOUR are separate proof arms)
endif
endif
ifeq ($(NDS_P2_SAMUS_DAMAGEFLY_TOUR),1)
ifneq ($(NDS_P2_SAMUS),1)
$(error NDS_P2_SAMUS_DAMAGEFLY_TOUR=1 requires NDS_P2_SAMUS=1)
endif
ifneq ($(NDS_P2_PROOF_FIGHTER0),3)
$(error NDS_P2_SAMUS_DAMAGEFLY_TOUR=1 requires NDS_P2_PROOF_FIGHTER0=3)
endif
ifneq ($(NDS_P2_SAMUS_STATE_TOUR),0)
$(error NDS_P2_SAMUS_DAMAGEFLY_TOUR and NDS_P2_SAMUS_STATE_TOUR are separate proof arms)
endif
endif
ifeq ($(NDS_P2_SAMUS_STATE_TOUR),1)
ifneq ($(NDS_P2_SAMUS),1)
$(error NDS_P2_SAMUS_STATE_TOUR=1 requires NDS_P2_SAMUS=1)
endif
ifneq ($(NDS_P2_PROOF_FIGHTER0),3)
$(error NDS_P2_SAMUS_STATE_TOUR=1 requires NDS_P2_PROOF_FIGHTER0=3)
endif
endif
# THE FREEZE SOAK'S MATCH LENGTH IN MINUTES, and nothing else's. 0 = leave the
# harness seeding alone, which is the canonical one-minute Time match; non-zero
# overrides scene_harness.c's time_limit.
#
# It exists because this used to ride on NDS_R2_BOTH_CPU: that branch seeded a
# 7-minute match so a long soak would stay in gameplay, and the side effect was
# that the both-CPU GATE arm sampled a 420-second match through a window sized
# for a 60-second one. Measured 2026-08-05: 12.6% coverage against Boundary's
# 86.7%, which superseded every both-CPU tick figure in the campaign. Owner's
# ruling: "the soak was only meant to catch freezes, boundary and both cpu gates
# should be the 60 sec match".
#
# SO IT MUST STAY 0 FOR ANY MEASURING RUN, and soak-freeze-watch.ps1 is the only
# harness that sets it. It derives the value from its own -MinutesToRun instead
# of hardcoding one, so the match can never again be shorter than the run that
# watches it, and it verifies the value in-guest rather than trusting this flag.
NDS_R2_SOAK_MATCH_MINUTES ?= 0
# The negative control for the shield anim-joint fix. ftParamUpdateAnimKeys
# picks the parser from fp->anim_desc.flags.is_anim_joint for the WHOLE fighter
# (decomp ft/ftparam.c:386), so a joint still holding a figatree while that flag
# is set is read by the 32-bit parser -- the misread class attributed in
# artifacts/performance/2026-08-13_c-anim-anomalies/ANOMALIES.md. This counts
# that dispatch directly, which is strictly stronger than gNdsObjAnimRunawayCount:
# the runaway counter only fires when the misread word happens to decode to an
# ILLEGAL opcode, while a misread that lands on 0..23 corrupts silently.
# Lab only -- it walks the loaded-file table per shielding joint -- and it costs
# nothing when no fighter is shielding.
NDS_ANIM_JOINT_AUDIT ?= 0
# Qualification oracle for the normalized-ledger index (battleship_sys_objanim.c).
# On EVERY index lookup it also runs the linear scan the index replaced and
# compares the two answers, counting any disagreement. The index is only correct
# because the ledger's keys are unique, and that is a property of the insert path
# rather than of the lookup, so it is proven by running both and not by argument.
# Lab only: it restores the whole cost the index removes, so a run carrying this
# flag is a correctness run and can never be a gate figure.
NDS_AOBJ_EVENT32_HASH_ORACLE ?= 0
# The A/B/A third arm for that index, and it has to be a flag rather than a
# rebuild: this build is byte-reproducible (build-c144-ctl and build-c145-ctl2,
# same source, different directory, identical ROM SHA-256) and the tick-HUD
# sampler is bit-deterministic, so re-running the control cannot bracket
# anything. At 0 the lookup falls back to the linear scan it replaced while the
# 8,192-byte index, its counters, and every section after them stay exactly
# where the shipping arm puts them -- i.e. the candidate's PLACEMENT with the
# control's BEHAVIOUR, which is the only arm that can tell the two apart.
NDS_AOBJ_EVENT32_LEDGER_INDEX ?= 1
# R2-03 E49. Teaches the native fighter owner the precedence E48 measured: an
# epoch whose vertices carry a valid vertex colour and no material is emitted
# from that colour raw and is NOT lit. The generic path has always done this
# (ndsRendererHardwarePackedValidVertexColor never calls LitShadeColor on that
# route); the owner decided epoch_lit from geometry_mode & LIGHTING alone and ran
# the geometry engine's light, which is E32's dark-maroon hurt flash. Drops
# POLY_FORMAT_LIGHT0 for such an epoch and writes GFX_COLOR from the baked dense
# rgba instead of GFX_NORMAL at all four emit sites. Requires
# NDS_R2_MATERIAL_DYNAMIC for the epoch predicate.
NDS_R2_UNLIT_VERTEX_EPOCH ?= 0
# R2-04 E2. Shadow copy of the FPS-HUD publish, from the same locals in the
# same breath, to separate "something rewrites the primary afterwards" from
# "the harness BUS_CLOCK constant is wrong". Lab probe.
NDS_R204_FPSHUD_SHADOW ?= 0
# Task 103 lab probe (docs/optimization/RASTER_AXIS_CAMPAIGN.md fork B).
# Task 99 left the stage bucket ~89% fixed and Task 100 refuted the last
# proposed currency for it, so ~331,300 ticks/frame are still unattributed.
# Over 54 runs that is ~6,135 per run, which points at per-operation
# scaffolding -- the currency Task 99 §4 named and nothing has isolated.
#
# Splits ndsRendererTask36ReplayRun into its three spans: the begin-run
# scaffolding, the GFX_FIFO word-push loop, and the end-batch tail. That
# distinguishes the two live explanations directly. If the cost is in the push
# loop it is FIFO stall, which is geometry-side backpressure and would mean
# Task 55 E2's "words are free" needs re-reading; if it is in begin/end, run
# count is the currency and the lever is run structure.
#
# Deliberately measures in place rather than by removing runs: Task 99 arm C
# culled 27 of 54 and measured +109,888 because that disarms the Task 36
# capture-once replay. Counters are cumulative; sample with a two-stop delta.
# Lab only, default 0.
NDS_TASK103_STAGE_RUN_PHASE ?= 0
# Task 104: elide the dead stats traffic on a Task 36 replay hit.
#
# The hit path cleared 1,292 bytes of `preflight_stats`, then overwrote all
# 1,292 with `*stats = owner->segment_stats[i]`, to transport exactly one live
# member -- `sync_command_count`, the only field read after the segment loop.
# Every other member is overwritten before its next read, because the following
# segment starts with `ndsRendererInitStats`.
#
# Task 84 E1 priced this struct at 2.74 ticks/byte: 1,292 bytes span ~41 cache
# lines that the renderer evicts between segments. Three hit segments a frame
# were paying the clear plus both sides of the copy -- ~3,876 bytes each.
#
# Task 103 E7 removed only the clear and realised 28% of its predicted saving:
# the copy still touched those lines, so the misses relocated into it instead of
# disappearing (Task 84 E1.4's mechanism). This removes both accesses together,
# which is the difference between a miss that moves and a miss that vanishes.
#
# Measured KEEP, default on: STG P50 -22,016, WORK-H P50 -26,240, P95 -28,352,
# FTR flat (-704), SRC flat (+1,088), VBlank 4-interval 39 -> 28. Kept as a flag
# so the A/B stays reproducible.
NDS_TASK104_STAGE_STATS_ELISION ?= 1
# Task 106 E0: logical updates per presented frame. Ships at 2 (the source 60 Hz
# simulation against a 30 Hz present). Building with 1 prices what a 30 Hz
# simulation would save; it is a sizing arm only, uncompensated, and plays at
# half speed. Do not publish a ROM built with 1.
NDS_TASK106_UPDATES_PER_PRESENT ?= 2
# Task 75 E0: count completed file loads and ride the per-frame delta on the
# same census ring the Task 70 fallback counter uses. Answers Task 71 section
# 5's open obligation -- one expensive SRC frame was profiled and found to be a
# cartridge load, but it was never shown that every high-SRC frame is one.
# Task 106 made that the gate's question: the SRC excursion survives halving the
# update rate unchanged, so the tail is loading, not simulation.
# Sample with -FallbackCensus -RingDump; lab only, default 0.
NDS_TASK75_LOAD_CENSUS ?= 0
# 2026-08-16: does the fighter DRAW CONTRACT actually change every frame?
# FTR_LANE.md section 5 sized the capture pass at 34,307 tk/fr and called it a
# CEILING, because the contract-change rate had never been measured and no
# counter existed for it. This flag adds that counter and nothing else: after
# each capture, hash the emitted event list, the DL pointers, the preambles and
# a candidate DObj-tree key, and count changed/unchanged against that slot's
# previous frame. It answers whether the pass is a memo, a load-time table, or
# dead, without building a memo first. Lab only, default 0 -- it costs a ~640-
# byte hash per fighter per frame inside the FTR span, so a ROM built with it
# must never be read for ticks.
NDS_R2_FTR_CONTRACT_CENSUS ?= 0
# 2026-08-16: the INITIAL VALUE of gNdsFtrDrawMemoRoute -- the fighter draw
# contract memo. The census above measured the answer: the contract is unchanged
# on 4,025 of 4,076 captures (98.75%), and every one of the 51 changes is decided
# in the HEAD of ftDisplayMainProcDisplay, not in the DObj tree. So the walk
# (ftDisplayMainDrawAll -> ftDisplayMainDrawDefault, 19,300 tk/fr) is memoisable
# against a key made of the head's OWN output, while the head keeps running for
# its side effects (the off-screen arrow HUD, the fog statics, the scene light).
# 1 = memo live; 0 = the walk runs every capture exactly as before. The runtime
# poke reaches gNdsFtrDrawMemoRoute (its own 32-byte line), so an A/B is one
# binary and two arms.
NDS_R2_FTR_DRAW_MEMO ?= 1
# Cycle 100: the INITIAL VALUE of gNdsFtrPlanRoute (the baked fighter draw
# plan). 0 = the eligibility pass and the owner-validate cache run every draw,
# exactly as shipped; 1 = both are replaced by replaying a plan baked at scene
# entry.
#
# This exists because the runtime poke cannot reach that flag, and the reason is
# worth stating so nobody re-derives it: `-SetGlobals` writes main RAM through
# the GDB stub, but gNdsFtrPlanRoute shares its 32-byte ARM9 D-cache line with
# gNdsTickHudVBlankWaitTicks, which the tick HUD writes every frame. The line is
# therefore permanently resident and dirty, so the guest keeps reading its stale
# cached 0 and each writeback stamps that 0 back over the poke. Measured cycle
# 100: poked 7, read back 7 in the same stop, 0 plan hits over 1,216 draws, and
# 0 at end of run -- while a sibling four bytes lower, in the PREVIOUS cache
# line, survived the same batch. Both arms compile identical text and both keep
# the flag in .data (see diagnostics.c), so the two builds differ in exactly one
# word of initialised data and nothing else moves.
#
# DEFAULT 1 = KEEP, cycle 100. Equivalence first: gNdsFtrPlanVerifyMismatch 0
# over 3,960 verified draws on the both-CPU arm and 3,954 on Boundary, with
# fighter triangles byte-identical on both (636,480/604,044 and
# 612,800/624,852), and the validate call count falling 3,963 -> 3 per match.
# Price, three A/B pairs of layout-identical builds, whole match, 1,600 samples:
# WORK-H P50 -3,776 (Boundary), -5,056 and -9,472 (both-CPU); over-gate frames
# -6, -38, -31. It is a P50/mean lever and NOT gate progress -- the P95 delta
# read -8,832, -2,368 and +5,248 across those same pairs, so P95 is not
# resolvable for a change this size and no gate figure may be banked from it.
NDS_FTR_PLAN_ROUTE ?= 1
# P2-3r6 lab instrument: record the modelview/projection, poly format and
# emitted triangle count of each Mario entry-pipe root into globals, where a
# gdb stub read is sound. Default 0; the shipped ROM pays nothing.
NDS_ENTRY_EFFECT_DIAG ?= 0
# Cycle 100: arm the baked plan's equivalence check (derive the plan live on
# every baked draw and memcmp it against the baked one). Build-time for the same
# cache-line reason as the route above, and the reason is sharper here:
# gNdsFtrPlanVerify shares its line with gNdsFtrPlanHit, which increments on
# every baked draw -- so on the arm you would want to verify, the line is
# guaranteed dirty and the poke is guaranteed to be stamped back to 0.
# Never read ticks from a build with this on; it computes both paths by design.
NDS_FTR_PLAN_VERIFY ?= 0
# Second-entry (Sudden Death / rematch) MObj chain validator. Records the
# fighter material chain before the counting pass and again immediately before
# the writing pass, so "when does the list first become invalid" is measured
# rather than inferred from wherever the overflow happened to surface. Bounded
# node count, cycle detection, arena containment, owning DObj, and the taskman
# heap generation at probe time. Lab only, default 0: it walks every chain twice
# per DObj per frame and keeps a 64-entry seen list on the stack.
NDS_R2_SECOND_ENTRY_DIAG ?= 0
# P2-1b scene-loop walk. Number of menu -> battle -> results -> menu loops the
# bounded scene tails in taskman_seam.c drive automatically before they park
# again. 0 (the default, and the value every published and Boundary
# configuration carries) compiles the three walk tails out entirely, so mode
# 163 parks exactly where it always did. Set it on a LAB build to produce the
# per-scene arena high-water evidence the row closes on -- the ring lives in
# gNdsSceneManagerRing* and is read by scripts/probe-scene-loop-walk.ps1.
# Needs a fast-logic harness: the walk's battle leg is a bounded run, not a
# match, because this measures the scene BOUNDARY, not gameplay.
NDS_R2_SCENE_LOOP_WALK ?= 0
# P2-1c. The VS shell's 2D UI kit: the SSB64 menu font, the hand cursor, the
# Mario/Fox CSS portraits, menu SFX, and the retained text/sprite slots
# P2-1d/1e/1f draw their screens out of. 0 compiles the whole thing out --
# src/nds/nds_ui_kit.c is one #if, the NitroFS pack below is not staged, and
# both published ROMs are byte-identical to a build without the row. P2-1d
# turns it on for real.
NDS_P2_UI_KIT ?= 0
# P2-1d. The VS shell's real screens: splash, title, main menu, and the VS menu
# that carries the rules (src/nds/nds_menu_shell.c). It turns the kit above on
# for the first time, replaces the bounded Startup/Title/VSMode park branches
# with screens the player drives, gives nSCKindModeSelect a scene where it had
# an NDS_SCENE_STUB, and moves the mode-163 BOOT SCENE from the match to the
# splash. That last one is why it is a flag and not a default: at 0 -- every
# published and Boundary configuration -- mode 163 boots straight into the
# battle exactly as it always has, no menu symbol is linked, and the NitroFS
# pack is not staged.
NDS_P2_MENU_SHELL ?= 0
# P2-1L item (11). The BOOT SELF-TEST TEXT -- "NitroFS: PASS", "OS
# queues/threads: PASS", "Original boot: PARTIAL" -- printed to the sub
# engine's libnds console during `main`. It is a lab instrument: every one of
# those lines is also a published global (gNdsRelocAssetInitResult,
# gNdsBootSelfTestResult, gNdsOriginalBootStage) that the verifiers read over
# gdb, so the console copy exists for a human watching a boot, not for any
# check. On a ROM handed to the OWNER it is three lines of diagnostic text
# sitting on the bottom screen under every menu, which is what item (11)
# reports. 1 everywhere (lab and verifier targets keep it); 0 in the free-play
# block below. The console itself still initialises -- this gates the three
# prints, not the sub engine's setup, so nothing else that writes there moves.
NDS_BOOT_DIAG_TEXT ?= 1
# Scripted menu walk, lab only, N loops. Feeds the SCREENS' OWN input handlers
# a fixed button script -- title START, main menu down/confirm, then every VS
# row, both value directions and the refusal, then VS START -- with a dwell
# between steps so each screen is both measurable and capturable. Zero means no
# injection at all and the shell is driven only by the player.
NDS_P2_MENU_WALK ?= 0
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
# R2-02 E3 falsifier, lab only. Hashes the prepared run and dense data the four
# actor segments consume, once a frame, and counts the frames it changes on. It
# outlived the cut it was written for: it proved the *prepared* data constant,
# which was true and was never the question -- the actor segments' matrices are
# what move. Kept because it is the cheap way to re-check that half.
NDS_R2_STAGE_ACTORS_PROOF ?= 0
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
# Historical lab escape retained so old measurement commands remain parseable.
# The accepted route now ships directly, so canonical targets no longer need it.
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
# R2-03 E34. Falsifier for E26. Hashes the complete renderer state each epoch
# hands to its runs, per epoch index, and counts frames whose value differs from
# the one already stored. E26's baked per-epoch state is only possible if that
# hash is a function of the epoch index -- and ndsRendererNativeApplyMaterial
# writes the same stats fields from a live input, so contamination can propagate
# into later epochs. Lab only.
NDS_R2_FIGHTER_EPOCH_STATE_PROOF ?= 0
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
# R2-03 E28 control arm. E16 left the software light preparation
# (ndsRendererHardwarePrepareLitDirection's transform + sqrt + three divides,
# and the shade LUT) running per lit epoch even though the hardware path skips
# the only loop that reads them. E28 removes it; this restores it, so the cut
# can be A/B'd against a control built from the same tree. Delete this flag with
# the experiment.
NDS_R2_FIGHTER_SOFT_LIGHT_KEEP ?= 0
# R2-03 E18. Skips the fighter's per-vertex software lighting outright, so the
# ceiling of E16's hardware-lighting cut can be measured rather than inferred
# from a bracket that also contains the epoch preamble. Fighters draw with stale
# colours -- lab only, never a playable configuration.
NDS_R2_FIGHTER_SHADE_SKIP ?= 0
# R2-03 E19. Same one-build pricing method as E18, applied to the epoch state
# spans -- the next ranked item after the shade at ~52,000/frame. Skips the
# recorded state-delta replay either side of the material. Lab only; the
# fighters' render state goes wrong, which is the engagement proof.
NDS_R2_FIGHTER_STATESPAN_SKIP ?= 0
# R2-03 E13. Skips a fighter's draw outright, which is how the campaign prices a
# whole fighter end to end: the phase census partitions the draw, this measures
# what the frame costs without it. Bit 0 = Mario, bit 1 = Fox. Lab only; a ROM
# built with this is not a playable configuration.
NDS_R2_DRAW_SUPPRESS_MASK ?= 0
NDS_RENDER_ECONOMY ?= 0
# Owner 5 is the only census-ranked Dream Land cut that passed the canonical
# 500-pixel ratchet.  The enclosing economy flag remains off by default.
NDS_RENDER_ECONOMY_OWNER_MASK ?= 32
NDS_RENDERER_BENCHMARK_MODE ?= 0
NDS_SCENE_MIP_CACHE_LAB ?= 0
NDS_FAST_WALLPAPER_AFFINE ?= 0
# R2-07 R2b. Admit the VS Results wallpaper to the same hardware-affine BG the
# Dream Land battle wallpaper already uses. R0h measured the Results background
# layer at 1,746,558 ticks/frame across four software stages -- blit, 153,600-byte
# clear, 320x240 -> 256x192 downscale, 98,304-byte VRAM copy -- for a STATIC image.
# The affine path decodes once into BG VRAM and then pushes four affine registers
# per frame. Both arms were built and measured before it graduated: 10 VBlanks ->
# 7, 5,601,900 -> 3,921,330 ticks/frame on the phase-aligned histogram, and the
# census row for the software wallpaper vanishes rather than shrinking -- the
# layer is gone, not cheaper. Owner approved the matched-tic capture pair on
# 2026-07-30 ("the affine one looks perfect"), so it is on by default:
#   artifacts/visibility/2026-07-30_r207-r2b-results-control-software.png
#   artifacts/visibility/2026-07-30_r207-r2b-results-candidate-affine.png
NDS_R2_RESULTS_AFFINE ?= 1
# R2-07 R4b. Skip the whole foreground software compositor -- staging clear,
# blits, 320x240 -> 256x192 downscale, 98,304-byte VRAM copy -- on frames whose
# foreground draw set is byte-identical to the image already resident in BG
# VRAM. Two independent censuses put those four stages at 41.03%/44.38% of the
# Results frame.
#
# GRADUATED 2026-07-30. Measured -2,245,333 ticks/frame (-28.6%), work -44.3%,
# instructions -49.9%, and the output is PIXEL-IDENTICAL: 253,344 guest pixels
# compared at Results tic 160 on a matched-source-tic pair, 0 differing, max
# channel delta 0, while skipping 97.7% of foreground layers. Evidence at
# artifacts/task37-census/r207-r4b-memo-on/ and
# artifacts/visibility/2026-07-30_r207-r4b-results-tic160-{control,candidate}.png
NDS_R2_RESULTS_LAYER_MEMO ?= 1
# R2-07 R4d. Suppress the fallback main-loop present (main.c) on iterations where
# a scene loop already presented. Measured 2026-07-30: VS Results runs 2.00
# ndsPlatformEndFrame calls per source tic -- one from the scene loop
# (taskman_seam.c:7049) that submits and flushes the frame, and one from the
# main loop that submits nothing, so its flush is skipped but its unconditional
# swiWaitForVBlank is not. The battle path runs 1.00 per logical frame, so this
# is Results-only and every battle P95 in the campaign is unaffected.
#
# GRADUATED 2026-07-30. Matched-source A/B on smash64ds-results-lab-hwtri:
# -560,190 ticks per source tic (-19.9%), 5.03 -> 4.03 VBlanks, which is exactly
# one VBlank and exactly one swiWaitForVBlank. Fighter draw is unchanged
# (1,709,228 -> 1,710,498, +0.07%) and GX submits/flushes stay at 1.00 per tic,
# so nothing that renders was touched. The guest picture is PIXEL-IDENTICAL:
# 240,000 guest-viewport pixels at Results tic 160, 0 differing, max channel
# delta 0. Evidence at artifacts/performance/r4d-main-present-guard-{off,on}
# -20260730.json and artifacts/visibility/2026-07-30_r207-r4d-results-tic160-*.png
NDS_R2_MAIN_PRESENT_GUARD ?= 1
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
# Lab-only BGM falsifier for the 5-VBlank dip investigation. Skips BGM
# open/read/flush/play while preserving all BGM state/counters so the rest of
# the system believes BGM is active. Never set in a published target.
NDS_BGM_FALSIFIER_OFF ?= 0
NDS_RENDERER_BATTLE_STATIC_TEXTURE_DEFAULT ?= 0
NDS_IFCOMMON_HYBRID_OAM ?= 0
NDS_DEBUG_HUD ?= 0
ifneq ($(NDS_DEBUG_HUD),0)
$(error NDS_DEBUG_HUD legacy debug wall is retired; use the focused battle phase HUD)
endif
NDS_AUDIO_FGM_ARM7_ACK_DIAGNOSTICS ?= 0
NDS_FREEZE_DIAGNOSTICS ?= 0
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
NDS_TASK9_STATE_HASH ?= 0
NDS_TASK10_HARDWARE_CALIBRATION ?= 0
NDS_TASK20_STACK_PROFILE ?= 0
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
# Draw the shield as one camera-facing quad instead of interpreting its source
# model's display list. EXPERIMENT, OFF BY DEFAULT -- see the block comment in
# battleship_efmanager.c. Owner observed 2026-08-06 that the N64 shield is
# always camera facing, and the source asset's drawing node is a 21-command DL
# over four vertices, so both routes draw the same textured quad and only the
# submit differs. The model route stays default because the owner bought it
# deliberately on 2026-08-04 ("36k p95 is worth it for correctness"); this flag
# exists to re-measure that price on the whole-match instrument, since the 36k
# came off a 128-frame window.
NDS_R2_SHIELD_QUAD ?= 0
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
# Cache ndsParticleSetCurrentCamera's answer for as long as its inputs hold.
# DEFAULT ON, cycle 102 -- measured, pixel-identical. See the block comment in
# battleship_lbparticle.c for the mechanism.
#
# That function is called once per quad by all three quad entry points, and each
# call rebuilds a perspective matrix, a look-at basis (three sqrtf) and a full
# 4x4 float guMtxCatF for a camera that cannot move within a frame. Measured
# 2026-08-09 on ROM 3B1159ED (artifacts/performance/2026-08-09_mtxcat-callers
# .json): it is 81.8% of the guMtxCatF + syMatrixLookAtF class, and that class
# is 24.2% of all __aeabi_fadd + __aeabi_fmul -- so with its own direct share it
# is about 23% of the frame's float and the largest single float consumer.
#
# THE RESULT, both-CPU arm, whole match, 1,600 samples, frames 442-2041, DLDI
# ON, slips=0, no repeated frames on either arm (artifacts/performance/
# 2026-08-09_c102-camcache-{ctl,cand}{,-rows.csv}.json):
#
#   WORK-H P50  1,129,664 -> 1,112,896   -16,768
#   WORK-H P95  1,665,856 -> 1,649,088   -16,768
#   MISC   P50    119,872 ->   102,848   -17,024   (owns the whole win)
#   FTR    P50    418,560 ->   418,432      -128   (flat -- the falsifier)
#
# Cache ran 3,054 hits / 1,458 misses. That is the CEILING, not a thrash: the
# camera moves every frame, so the frame's first cacheable call must rebuild and
# the other two hit.
#
# BIT-EXACT, AND MEASURED AS SUCH rather than argued: a hit replays the identical
# float result. Four matched-tic frames (900/901/1200/1201, software renderer)
# compare PIXEL-IDENTICAL on the game screen, max channel delta 0. Crop the top
# screen at 400x298, NOT 400x300 -- melonDS puts the screen boundary at row 298,
# and a 300-row crop catches two rows of tick-HUD text whose numbers differ
# between arms by construction. That cost one false "regression" reading.
#
# THE A/B PAIRING IS WHY THIS READS AT ALL. Gated at RUNTIME on a .data word,
# not on this flag, so both arms link byte-identical. The first attempt used a
# #if: 672 bytes moved `.main` and cost FTR +19,712 P50 -- a bucket the change
# never calls, and 15x what the board's 1.85-cycles-per-added-byte rule entitles
# 672 bytes to. That placement artifact INVERTED the sign of a real -16,768 win.
NDS_R2_PARTICLE_CAMERA_CACHE ?= 1
# Drop the four pieces of gmCameraLookAtFuncMatrix that are dead or redundant.
# All BIT-EXACT; the derivations are in battleship_gmcamera.c.
#
#   W1   the max > 32000 rescale branch re-runs syMatrixLookAtReflectF with
#        byte-identical arguments, and guMtxCatF writes gGMCameraMatrix rather
#        than the look-at, so the first result is still live.
#   W2   the closing syMatrixF2L writes through the caller's out-pointer, and
#        the fighter display-contract caller (reloc_backend_renderer_dl.c:12614)
#        discards it -- once per fighter per frame.
#   W2b  the projection Mtx is WRITE-ONLY on this port. decomp publishes it as
#        sGCMatrixProjectL for objdisplay.c, which this port does not compile;
#        its renderer builds its own 20.12 projection from the same CObj. A
#        literal-pool scan of the linked image finds exactly two references and
#        both are those stores. A second 16-element conversion plus a 64-byte
#        graphics-heap Mtx, twice a frame, feeding nothing.
#   W3s  the concat is 69% zeros. syMatrixPerspFastF writes five non-zero
#        elements; guMtxCatF still does 64 multiplies and 48 adds, 44+44 of them
#        against a literal zero. Specialised to 20 multiplies and 4 adds with
#        the left-to-right association preserved.
#
# MEASURED 2026-08-09 on the both-CPU tick-HUD ROM, whole 1,600-frame match,
# ONE binary with the route poked at the first frame marker -- so every arm is
# the same bytes and there is no cross-build placement floor at all:
#
#   level 1 vs 0    W1+W2       WORK-H P50 -1,728  FTR P50 -1,152 P95 -1,216
#   level 2 vs 1    +W3s        WORK-H P50 -1,664  FTR P50 -3,584 P95 -3,776
#   level 3 vs 1    +W3s+W2b    WORK-H P50 -3,200  FTR P50 -5,248 P95 -5,312
#
# FTR is the bucket the work lives in and the only one that reads consistently
# across P50, P95 and the repeat-free sub-window; WORK-H P95 spans 34K between
# readings and is not resolvable at this magnitude, as the board's cycle-100 row
# already established. Shipped cumulative FTR P50 -4,736 (levels 1 and 2).
#
# LEVEL 3 IS MEASURED AND HELD, and splitting it out is the only reason that was
# discoverable. The first pass shipped 2 and 3 together on the argument that
# "both are strictly less work at identical placement, so no decision depends on
# the split" -- which was wrong, and cost a red Boundary plus four profile runs
# to unpick. W2b does not merely remove work: dropping syMatrixAdvanceW stops
# consuming 64 bytes of gSYTaskmanGraphicsHeap per call and MOVES EVERY LATER
# ALLOCATION IN THE FRAME. Route 3 fails the Boundary realtime verifier's
# locked-30 phase accounting (phaseLag=-1) deterministically; route 2 and route 0
# pass; all three are the same ROM. See battleship_gmcamera.c and
# docs/KNOWN_ISSUES.md. A change that moves an allocator is not a placement-free
# change, whatever the .data route pairing says about its bytes.
#
# W1 NEVER FIRES in this milestone: gNdsCameraMatrixLeanRescaleCount stayed 0
# across all 1,600 frames in every arm, so none of the win is W1's. It is kept
# because it is correct and free, not because it was measured to pay.
#
# Pixel bar ZERO and met: at the time_remain=3000 lock the arms are
# PIXEL-IDENTICAL over the whole 400x298 top screen at both captured tics, while
# two ADJACENT tics of the same build differ on 83.0% of it -- that floor is what
# makes the identity mean something. Captured for levels 1 and 3; level 2 is
# level 3 minus a dead store, so it is bracketed by the pair, and the Boundary
# profile's own visible-capture gates pass on the shipped level-2 build.
#
# This is the bit-exact work. A fixed-point rebuild of what remains -- one
# look-at, one perspective, one 20-multiply concat -- is a separate arm with an
# error budget, kept separate so a pixel delta stays attributable. Note before
# briefing it that nds_task9_state_hash.c hashes gGMCameraStruct, which
# syMatrixLookAtReflectF writes, so an approximated look-at moves the Task 9
# hash and is not a free fidelity trade.
NDS_R2_CAMERA_MATRIX_LEAN ?= 2
# Q20.12 camera + projection chain, the falsifier for the draw-side soft-float
# exchange rate (include/nds/nds_r2_camera_fixed.h). This only sets the INITIAL
# value of gNdsR2CameraFixedEnabled, which is a `.data` word: the A/B is one
# binary poked through both arms with -SetGlobals, and this flag exists so a
# capture harness that cannot poke a global still gets a candidate ROM whose
# layout is byte-identical to the control's (one initialised word differs, no
# section moves).
#
# DEFAULT 1 SINCE 2026-08-16, ON THE OWNER'S DECISION. Precision changes pixels
# and the owner's eye is the acceptance gate; they played build-c205-camtoggle,
# said "otherwise it looks fine" of the picture, and then accepted the arm --
# "I think camera fixed point is ok". That is the acceptance the 6.5350%
# top-screen pixel delta of ../artifacts/performance/2026-08-16_camera-fixedpoint
# section 8 was BLOCKED on, and it is the first draw-side precision ceiling this
# project has set. Supporting evidence, both on disk: -4,736 tk/fr paired median
# on a same-binary route, and presented cadence on the ROM they played is very
# slightly BETTER on this arm (1,956/69/5/13 against 1,953/72/5/13, four VBlanks
# shorter over 2,043 frames, 0 cadence violations --
# 2026-08-16_camera-cadence/CADENCE.md section 2).
#
# Still overridable per build (NDS_R2_CAMERA_FIXED=0 restores the float chain
# byte for byte) because the route word stays `.data` and both arms stay linked;
# that is what makes the A/B a same-binary poke rather than a cross-build pair.
NDS_R2_CAMERA_FIXED ?= 1
# LAB ONLY, and never set by a target block. Binds SELECT -- the one DS key the
# battle input map leaves unbound (controller_backend.c maps A/B/X/Y/L/R/START
# and the d-pad, and nothing maps SELECT) -- to flip gNdsR2CameraFixedEnabled
# mid-match, and prints the live arm on the battle text HUD. The route word is
# already `.data` and already read per call site, so one binary carries both
# arms and the owner can A/B on the same frame, same camera, same fight rather
# than reloading two ROMs.
#
# DEFAULT 0, and it must stay 0 for every measurement and every published ROM:
# this adds a keypad edge test to ndsPlatformReadInput and a row to the HUD, so
# a build carrying it is NOT the instrument the campaign's tick figures were
# taken on. Pass NDS_R2_CAMERA_FIXED_TOGGLE=1 on the command line for the
# owner's play-test ROM only.
NDS_R2_CAMERA_FIXED_TOGGLE ?= 0
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
# Dream Land fireball map collision without the generic BattleShip
# mpProcessUpdateMain/wpMapProcAll path. The fast path uses a compact AOT copy of
# Pupupu's seven source collision lines, sweeps the fireball diamond directly,
# and publishes the same MPCollData mask/normal/line fields consumed by
# wpMapCheckAllRebound. Other stages fall back to wpMapTestAll. Owner-playtested
# and accepted 2026-08-07; ON BY DEFAULT. Engagement/fallback counters make an
# accidental generic route visible.
NDS_R2_FIREBALL_NATIVE_MAP_COLL ?= 1
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
# Forces the Mario/Luigi fireball into the source's map-collision debug display
# (wpDisplayMapCollisions, gated on display_mode == nDBDisplayModeMapCollision),
# which draws the fireball's stage-collision diamond as two scaled boxes. The
# visualization is built into wpdisplay.c -- this flag only sets the fireball's
# display_mode at spawn so the existing path runs. For tuning the scale above.
NDS_R2_FIREBALL_MAP_COLL_DEBUG ?= 0
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
# Accepted second RebirthHalo pass (2026-08-08): immutable AABB admission,
# exact AOT RotY, split source projection/modelview on GX, and packed DMA vertex
# submission. At the deterministic normal-+90 equivalent it differs from source
# in 374/120000 pixels (0.312%), all on the platform's subpixel raster edges,
# while effect-draw P50 falls 230240 -> 112224 ticks (-51.3%).
NDS_R2_REBIRTH_HALO_FULL_OFFLOAD ?= 1
# Bit per generated RebirthHalo group eligible for model-space GX submission.
# 0x3f is the all-GX experiment; final acceptance may narrow this if the DS
# transformer's subpixel rounding changes source coverage for a specific group.
NDS_R2_REBIRTH_HALO_GX_GROUP_MASK ?= 0x3f
# Accepted GX route: keep source projection/modelview split on the DS and use
# natural GX Z. A previous blue-screen rejection was invalid external capture
# interference; the clean rerun is visually correct and measured above.
NDS_R2_REBIRTH_HALO_SPLIT_MTX ?= 1
# Optional companion to SPLIT_MTX: preserve the accepted renderer's synthetic
# per-triangle painter Z by replacing only projection column Z with depth*W.
# X/Y/W still use the true split modelview/projection hardware path.
NDS_R2_REBIRTH_HALO_SPLIT_NOZ ?= 0
# Packed vertex-stream mode. Software-lit COLOR words are the only live words;
# packet topology/model-space geometry/texcoords are cached once and those live
# colors are patched before the DMA submission.
# 0 = ordinary GX MMIO writes, 1 = cached packed GXFIFO CPU push,
# 2 = cached packed GXFIFO DMA0 push. Mode 2 is accepted: the packet transport
# itself is pixel-identical to the split renderer and saves another ~37.5K P50.
NDS_R2_REBIRTH_HALO_PACKED_FIFO ?= 2
# Rebuild a packed packet from the accepted CPU-projected X/Y each draw instead
# of requiring model-space GX submission. Lab-only: this prices FIFO/DMA savings
# while retaining the exact software projection/raster contract.
NDS_R2_REBIRTH_HALO_PACKED_PROJECTED ?= 0
# DS lighting for the two source-lit RebirthHalo groups. Owner-approved after
# visual inspection of the all-optimization lab ROM; keep it enabled by default.
NDS_R2_REBIRTH_HALO_HW_LIGHT ?= 1
# Lab-only cycle census for the native RebirthHalo hot path.  The counters are
# cumulative and intentionally compiled out of production builds.
NDS_R2_REBIRTH_HALO_PHASE_PROFILE ?= 0
# RebirthHalo is positively identified by the effect-tree owner before its
# immutable generated lists reach the generic DL adapter. Bypass loaded-file
# discovery, segment-E material preparation, callbacks and interpreter setup;
# the two lists on the same child also share one matrix/setup. Deterministic
# post-KO A/B: 112,224 -> 82,304 effect ticks P50 with 0 changed DS pixels
# versus the previously accepted software-light native route.
NDS_R2_REBIRTH_HALO_FAST_ADAPTER ?= 1
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
# R2-07 slice 53 -- the CONSUMER half, and it exists to measure ONE number.
#
# Slice 52 converted the three PRODUCERS and measured an exchange rate of 1.001
# whole match / 1.014 at rank-80: the fixed text costs exactly what the float it
# deletes costs. The lane's remaining hope is RESIDENCY -- keeping the fixed
# representation so the f32 boundary stops being paid -- and residency's price
# is dominated by gmCollisionTestRectangle, whose per-call byte footprint the
# 2026-08-15 captures could only bracket between 0.052 and 0.467 tk/byte/call, a
# 9x spread that straddles the requirement.
#
# At 1 this compiles ndsR2CfxTestFighterDamage into the ring and lets the
# src/import/battleship_gmcollision.c wrapper answer the fighter hurtbox test
# from ndsR2CfxTestRectangle instead of calling the decomp float body. The
# decision itself therefore moves into port code for the first time, which is
# why it is a MEASURING flag with a default of 0 and not a shipping candidate.
NDS_R2_COLLISION_FIXED_NARROW ?= 0
# The falsifier arm for the line above, same mechanism and same reason as
# NDS_R2_COLLISION_FIXED_DISPATCH: one initialised word of .data
# (gNdsCfxNarrowEnable) read through a volatile, so both arms link byte-identical
# text and scripts/compare-elf-sections.py can assert the pair differs in exactly
# one byte.
NDS_R2_COLLISION_FIXED_NARROW_DISPATCH ?= $(NDS_R2_COLLISION_FIXED_NARROW)
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
# Lab SAME-BINARY route for the tile-sync memo (2026-08-16). The memo itself is
# unconditional and ships; this flag only adds the `.data` selector and the two
# engagement counters that price it. Task 107's census measured 72.835% of
# ndsRendererSyncTextureTile's 146,221 whole-match calls to be exact repeats
# against an 8,867 tk/fr owner -- far under the >=14,080 rank-80 cross-build
# floor, so a two-build A/B cannot decide it.
#   gNdsR2TileSyncRoute  0 = republish always (the pre-memo behaviour)
#                        1 = skip the proven-redundant republish (ships)
# Both arms evaluate the predicate and both advance the sync serial, so
# gNdsR2TileSyncSkips/Runs must be IDENTICAL on the two arms.
NDS_R2_TILESYNC_ROUTE ?= 0
# 2026-08-16. ndsR2AnimValueQ into ITCM. The marginal-80 per-PC census charges
# the kernel 26,664 tk/fr and 21,719 of that -- 81.4% -- is `icache_fill` on
# 1,028 bytes entered 370.6 times a frame. That is the largest single
# fidelity-neutral item on the board (0.296x of the +73,425 level) and the
# arithmetic does not change, because only the address does. Paid for by
# NDS_TASK9_FLOAT_MAIN_MEMBERS (332 B) plus the light-shade LUT builder (404 B),
# both measured at ZERO executed instructions across the gate window.
NDS_R2_ANIM_Q_ITCM_ON ?= 1
# Lab SAME-BINARY route for that placement: two out-of-line copies of one body,
# one in .itcm and one in .main, selected by a `.data` word. Costs 1,028 B of
# .main and nothing in the shipped build.
#   gNdsR2AnimItcmRoute  0 = call the .main copy (the pre-move behaviour)
#                        1 = call the .itcm copy (ships)
# Both arms execute identical instructions on identical inputs, so
# gNdsR2CubicEvals must read IDENTICALLY on the two arms.
NDS_R2_ANIM_ITCM_ROUTE ?= 0
# Task 44 stage steady-state excision: generation-based admission, dense
# rigid/dynamic binding lists, and the hoisted GX capture-active test. Requires
# the Task 36 hardware-compose stage owner; meaningless without it.
NDS_TASK44_STAGE_STEADY ?= 0
# Task 37 census instrument. Opens the repo melonDS build's ARM9 per-PC profiler
# over a fixed window of settled battle frames (reset at START, dump START+FRAMES
# later) so the census measures the steady-state loop and not boot. Lab only:
# never set in a published target, and it compiles to nothing at 0.
# Task 68 lab: count how often the fighter draw abandons the native production
# owner for the generic display-list interpreter. Default 0 -- enabling it adds
# BSS, and this ROM is cache-placement sensitive enough (see Task 37) that the
# extra bytes shift frame pacing. Any run with it on needs its own baseline.
NDS_TASK68_FALLBACK_CENSUS ?= 0
# Task 86: inline the 64-byte matrix struct copies GCC turns into memcpy calls.
# KEEP: WORK-H P95 -18,432, P50 -14,208, better on 118/128 frames, 3-VBlank
# frames 490 -> 499. Default on.
NDS_TASK86_MATRIX_COPY ?= 1
# Task 85: take an aligned load/store in the reloc native accessors instead of
# a memcpy call. 51% of every memcpy call in the frame moves 2 or 4 bytes.
# KEEP: SRC P95 -76,544, WORK-H P95 -40,384, VBlank histogram better on all
# four measures. Default on.
NDS_TASK85_ALIGNED_NATIVE_ACCESS ?= 1
# Task 82 ITCM re-pack: evict the animated-CI4 texel path (dead while Dream
# Land water is frozen) and admit five higher-stall functions. Placement only.
# KEEP on measurement: WORK-H P95 -52,224 with both evictions; this ships the
# owner's one-eviction variant. Default on.
NDS_TASK82_ITCM_REPACK ?= 1
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
NDS_TASK37_ITCM_LIBC = $(if $(filter 1 3 5 7,$(NDS_TASK37_ITCM_LEAVES)),1,0)
NDS_TASK37_ITCM_LIBM = $(if $(filter 2 3 6 7,$(NDS_TASK37_ITCM_LEAVES)),1,0)
NDS_TASK37_ITCM_PORT = $(if $(filter 4 5 6 7,$(NDS_TASK37_ITCM_LEAVES)),1,0)
# Layout control: N bytes of never-executed padding in .main, nothing in ITCM.
# Separates "ITCM residency breaks it" from "any layout change breaks it".
NDS_TASK37_LAYOUT_PROBE ?= 0
NDS_TASK37_LAYOUT_PROBE_ITCM ?= 0
# Lab-only: drop the controller devices from the Task 9 state hash. They are
# filled from the ARM7 on real time while fast-logic runs the ARM9 unpaced, so
# they encode execution speed. Diagnostic for Task 37 only.
NDS_TASK9_STATE_HASH_SKIP_CONTROLLERS ?= 0
# Lab-only: bitmask of NDSTask9StateRecordKind values the state hash includes.
# Diagnostic for Task 37 region isolation only; a filtered hash is a weaker gate.
NDS_TASK9_STATE_HASH_REGION_MASK ?= 0xFFFFFFFF
# Verification-only cadence selector. 1 preserves the original Task 9 contract:
# hash the complete active game state after every source update. Fighter-unit
# determinism replays may raise this to sample that SAME complete state at a
# fixed source-update cadence while still running the full one-minute match.
NDS_TASK9_STATE_HASH_STRIDE ?= 1
# Lab-only: snapshot raw FTStruct bytes for both fighters on updates N-1 and N,
# so the Task 37 divergence names an offset instead of a 3,012-byte blob.
# Costs ~12 KiB of BSS when enabled; observation only, never in a shipping build.
NDS_TASK9_FTSTRUCT_SNAPSHOT ?= 0
NDS_TASK9_FTSTRUCT_SNAPSHOT_UPDATE ?= 0
NDS_TASK10_GIT_SHORT ?= $(shell git rev-parse --short=7 HEAD 2>/dev/null || echo unknown)
ifeq ($(NDS_FAST_WALLPAPER_AFFINE),1)
ifneq ($(NDS_SCENE_MIP_CACHE_LAB),0)
$(error NDS_FAST_WALLPAPER_AFFINE=1 requires NDS_SCENE_MIP_CACHE_LAB=0)
endif
endif
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

ifneq ($(filter $(NDS_TASK53_REPLAY_ARENA_FIX),0 1),)
else
$(error NDS_TASK53_REPLAY_ARENA_FIX must be 0 or 1)
endif
# A mistyped value here silently produces a wrong ARENA, which reads as a clean
# measurement of the wrong configuration -- the exact failure this arm exists to
# repair. Fail the build instead.
ifneq ($(filter $(NDS_R2_BATTLEPACK_KEEP_CACHE),0 1),)
else
$(error NDS_R2_BATTLEPACK_KEEP_CACHE must be 0 or 1)
endif
ifneq ($(filter $(NDS_R2_BATTLEPACK_DISPATCH),0 1),)
else
$(error NDS_R2_BATTLEPACK_DISPATCH must be 0 or 1)
endif
ifneq ($(filter $(NDS_R2_FTANIM_STREAM),0 1),)
else
$(error NDS_R2_FTANIM_STREAM must be 0 or 1)
endif
ifneq ($(filter $(NDS_TASK55_STAGE_GEOM),0 1),)
else
$(error NDS_TASK55_STAGE_GEOM must be 0 or 1)
endif
ifneq ($(filter $(NDS_DREAMLAND_CARD_CULL),0 1),)
else
$(error NDS_DREAMLAND_CARD_CULL must be 0 or 1)
endif
# Task 56: NDS_TASK56_FIGHTER_PRIMITIVES must be 0, 1, or 2.
ifneq ($(NDS_TASK56_FIGHTER_PRIMITIVES),$(filter $(NDS_TASK56_FIGHTER_PRIMITIVES),0 1 2))
$(error NDS_TASK56_FIGHTER_PRIMITIVES must be 0, 1, or 2; got "$(NDS_TASK56_FIGHTER_PRIMITIVES)")
endif
# NDS_BATTLE_PROFILE must be exactly 0, 1, or 2. Anything else is a typo or a
# stale command-line; fail loudly rather than fall through to profile 1.
ifneq ($(NDS_BATTLE_PROFILE),$(filter $(NDS_BATTLE_PROFILE),0 1 2))
$(error NDS_BATTLE_PROFILE must be 0, 1, or 2; got "$(NDS_BATTLE_PROFILE)")
endif
# NDS_BATTLE_PROFILE=0 (DS-native precompiled path) is not implemented yet --
# it lands with Task 51. It must NEVER silently fall through to profile 1:
# this project has paid for silent no-ops three times (Task 37 := vs =, Task 46
# gc-sections). Fail the build with an actionable message until the native
# path exists.
ifeq ($(NDS_BATTLE_PROFILE),0)
$(error NDS_BATTLE_PROFILE=0 (DS-native precompiled path) is not implemented yet; it lands with Task 51. Set NDS_BATTLE_PROFILE=1.)
endif
NDS_RENDERER_FAST_RUN_DEFAULT ?= $(if $(filter smash64ds-battle-playable-coarse-hwtri,$(TARGET)),8,0)
ifeq ($(TARGET),smash64ds-battle-playable-hwtri)
# This is the only published P1 battle ROM. Keep the complete realtime
# configuration intrinsic to the target so direct builds and verifiers cannot
# silently publish a software-rendered or non-interactive variant.
override NDS_DEV_SCENE_HARNESS := battle_playable_realtime
override NDS_DEV_LIVE_INPUT_PREVIEW := 1
override NDS_HARNESS_FAST_LOGIC := 0
override NDS_RENDERER_HW_TRIANGLES := 1
override NDS_DEBUG_HUD := 0
override NDS_RENDERER_PROFILE_LEVEL := 0
override NDS_SHIP_TELEMETRY := 0
override NDS_TICK_HUD := 0
override NDS_RENDERER_FAST_RUN_DEFAULT := 9
override NDS_NATIVE_STAGE_GENERATED_SEGMENT0_ENABLE := 1
override NDS_TASK36_HW_COMPOSE := 2
# R2-08: THE SWITCH (SwitchPlan §6). The Boundary configuration is produced by
# the Runtime 2 battle path. The Runtime 1 loop stays in-tree as the oracle, so
# it needs a way to be built: `override` defeats a command-line 0, exactly as it
# does for NDS_R2_CUBIC_FIXED below, and without an escape hatch a Runtime 1
# control arm silently builds identical to the candidate.
ifneq ($(NDS_R2_LAB_R1_PATH),1)
override NDS_R2_PATH := 1
endif
# R2-03 E17: split matrix load. Stops composing modelview x projection on the
# CPU and lets the geometry engine multiply, -17,600 FTR P50. Boundary green in
# both flag states, geometry proven identical (136,640 P0 triangles either way
# over the same 480-frame window), owner-approved 2026-07-28.
override NDS_R2_FIGHTER_HW_MTX := 1
# Slice 43 was withdrawn on 2026-08-11 after the owner found a periodic one-frame
# fighter disappearance. The later full-match repair closed the GX matrix-stack
# leak, and the 2026-08-15 matched-tic diff masks were explicitly owner-accepted
# (0.0358-0.1742% battle-screen variance, GXSTAT 0x06000000, gameplay invariants
# unchanged). Owner policy 2026-08-16: accepted optimisations ship enabled.
override NDS_R2_FIGHTER_GX_COMPOSE := 1
# Slice 44: the stage's per-frame transform revalidation round-robins over 8
# frames instead of re-proving all 42 bindings constant every frame. Matched
# control from the same tree, 1,600-frame both-CPU gate: WORK-H P50 -17,088 /
# P95 -35,904, i.e. 2.0x and 4.2x the +/-8,544 floor, and it lands where it was
# aimed -- STG P50 -19,904 / P95 -24,192 with FTR flat. VBlank histogram
# 2:1621->2:1644 with 3/4/5+ all down. Engagement on both sides: RigidChecks
# 6,627 of 53,014 sweep slots is exactly 1/8, and 53,014 = 26 x 2,039 means
# every frame swept, so the rigid mask never once demoted. StaleReuse 28,518
# against 28,546 predicted. The game viewport is pixel-identical at two
# frame-locked tics (118,000 px, max channel delta 0) and both arms read
# damage 130/51 stock x1 at the lock, so neither presentation nor simulation
# moved. Not graduated onto the BUGS.md #9 floor arms below: those exist to
# compare the rigid path against the CPU path and a stride would confound them.
override NDS_R2_STAGE_VALIDATE_STRIDE := 8
# R2-03 E16: the fighter's per-vertex lighting runs on the geometry engine
# instead of the CPU. -35,072 FTR P50, VBlank histogram 2:381->2:418, geometry
# bit-identical (181,440 P0 triangles either way). Requires HW_MTX above for the
# vector matrix. Boundary green, owner-approved 2026-07-28.
override NDS_R2_FIGHTER_HW_LIGHT := 1
# R2-03 E32: the hitlag shuffle no longer knocks the native fighter owner off
# its path, so the generic display-list interpreter stops running as a second
# renderer for the 5 frames of a hitlag burst. -51,136 WORK-H P95, 17/128
# over-gate frames -> 13/128.
#
# ACCEPTED WITH A KNOWN VISUAL RESIDUAL, owner-approved 2026-07-29: the struck
# fighter does not flash white during those frames. E62 established this is a
# generator gap, not a runtime bug -- the flash clears G_LIGHTING and draws
# vertex colours raw, and the owner has no flash-colour data baked. Every
# non-flash frame is pixel-identical to the generic path. Tracked in
# KNOWN_ISSUES.md; do NOT enable NDS_R2_UNLIT_VERTEX_EPOCH as a fix, E62 proved
# it emits packed normals and is visibly worse.
override NDS_R2_FIGHTER_SHUFFLE_FOLD := 1
# R2-03 E64b: the animation cubic Hermite in Q12 fixed point. -26,944 WORK-H
# P95, -20,352 P50, SRC P50 -9,344, over-gate 12/128 -> 9/128, 135,871
# evaluations with zero saturations, Boundary green. Owner-authorized 2026-07-29
# as a non-bit-exact change. Step and Linear nodes keep the decomp's own
# expressions, so 45.3% of AObj nodes stay bit-identical.
#
# EQUIVALENCE, settled by E65 -- and note the correction path, because two
# earlier versions of this comment were wrong in opposite directions. The first
# claimed "the Task 9 state hash did NOT move"; it had never run
# (`NDS_TASK9_STATE_HASH ?= 0`, nothing in verify-all.ps1 references it). The
# second called the hash "the right instrument"; it is not. The hash asserts
# bit-exactness and this change is authorized NON-bit-exact, so it can only ever
# report "differs", which says nothing about whether gameplay moved.
#
# The right instrument is an error bound, and it now runs on every profile:
# scripts/check_r2_cubic_error_bound.py, wired into
# check-gbi-decode-fixtures.ps1. It extracts this kernel from between the
# NDS_R2_CUBIC_FIXED_KERNEL_BEGIN/END markers plus gcGetInterpValueCubic from the
# decomp, compiles both on the host and sweeps. Worst deviation 0.0028 rad on
# rotation tracks and 0.0067 world units on translation tracks, against a 0.02
# gate set by hitbox scale. Joint values reach gameplay only through
# gmCollisionGetFighterPartsWorldPosition, so that cannot flip a hit decision.
#
# E65 also lifted the basis to Q16 and moved the evaluator to ARM
# (__attribute__((noinline, target("arm")))) because this TU builds -mthumb,
# which has no SMULL: the 64-bit multiplies were eight `bl __aeabi_lmul` a call.
# A FURTHER -35,584 P95 on top of the numbers above, so the totals here are the
# E64b step alone. Do not remove that attribute.
#
# Arm A added a 256-entry conversion cache and REGRESSED (+21,632 P95) on 10 KB
# of BSS plus a 1,824-byte .text.hot member. Do not re-add the cache; the
# reasoning is in battleship_sys_objanim.c.
#
# NDS_R2_LAB_CUBIC_OFF=1 is the escape hatch that makes this A/B-able at all:
# `override` defeats a command-line 0, so without it the control arm silently
# builds identical to the candidate and the comparison reads "no difference".
ifneq ($(NDS_R2_LAB_CUBIC_OFF),1)
override NDS_R2_CUBIC_FIXED := 1
endif
# R2-03 E46: the fighter state-delta path into ITCM. The switch was already
# resident; the span loop and every Record*/SyncTextureTile helper it dispatches
# to were in main RAM, so all 134.5 before-span applications a frame left
# zero-wait ITCM for icache-served code. Placement only -- no behaviour change.
# FTR P50 -12,032, WORK P50 -12,416 over the same 128-frame window, with the
# untouched STG/SRC buckets moving +768/-1,216 to bound the noise. The gain is
# 4x the state-span bracket's -3,128 because ApplyMaterial (27.7/frame) and the
# texture prepare (46.4/frame) call the same helpers. +1,016 bytes of .itcm,
# 1,896 still free.
override NDS_R2_DELTA_PATH_ITCM := 1
# R2-04 E1/E4/E5: fighter animation payload cache plus a stepped warm preload.
# lbRelocGetForceExternHeapFile re-read an animation off the cartridge on every
# call. The cache keys on asset_id and stores the byte-swapped pre-fixup image,
# re-running fixups per destination so "force" still restores pristine data. The
# warm list makes the match's 41 measured animations (91,104 bytes) resident one
# per scene update across the countdown; loading all 41 in one call at
# scVSBattleStartBattle misses a BGM buffer seam and kills the music for the
# match. Misses 29 -> 2, WORK-H P95 1,364,992 -> 1,232,640. Cycle 105 re-measured
# the list at 85 animations / 197,184 bytes and resized the arena to match; the
# 41 above is the Boundary-derived figure it replaced.
override NDS_R2_ANIM_CACHE := 1
# Cycle 108: run the AObj16 lane swap + successor scan ONCE per warmed animation,
# at warm time, instead of on every force load. The pass is position-independent,
# so the transformed image survives the per-destination copy; the intrusive fixup
# list is recorded and restored around it because applying fixups consumes it.
# Measured on ONE binary with the runtime route (standing rule 7): WORK-H P95
# -15,693 over 68.4% of the change, so ~-23,000 whole -- which the separately
# linked arm reads as -22,806. Coverage 351/351 hits, 85/85 prebaked, all four
# decline counters 0, heap free-min unchanged at 42,136.
override NDS_R2_AOBJ16_PREBAKE := 1
# Task 53: re-activate Task 36 rigid-stage replay. Relaxes the arena admission
# guard (nds_renderer.c:4195/:4247) from the legacy exact-0x150000 check to
# "admit any usable arena >= 0x130000" -- the robust downward-stepping allocator
# (src/port/diagnostics.c:7368) cannot secure the full 0x150000 on the DS heap,
# so replay had been silently DISABLED since the allocator was made robust.
# E2 (2026-07-24): replay bit-exact with the generic emit (Task 49 differ
# ZERO_DEVIATION, 2860/2860 words); STG P50 -187,648 (-33%), VBlank tail up
# (3-VBlank share 426->474, 4->80, 5+->12), ALL P50 flat (saved CPU
# redistributes to OTHR). Owner visual approved 2026-07-24. Default-on here;
# the flag is one-line revertable if a device A/B rejects the pacing gain.
override NDS_TASK53_REPLAY_ARENA_FIX := 1
# Task 49: the battle-pipeline selector. Profile 1 = today's shipping path
# (the correctness oracle). Kept explicit here so the published ROM cannot
# silently slip to the not-yet-implemented profile 0.
override NDS_BATTLE_PROFILE := 1
# Task 44: stage steady-state admission + dense binding lists. Exact (no
# fidelity change); ships on with Task 36 replay.
override NDS_TASK44_STAGE_STEADY := 1
# R2-02: the three kept stage cuts, graduated 2026-07-28. Together they take STG
# P50 351,488 -> 212,480 (-139,008, -40%) and put the frame at 2 VBlanks where
# the previous shipping ROM sat at 3.
#
# All three are exactness-preserving, so none of them spends the PROJECT_GOAL.md
# fidelity budget and none needs the owner's visual-oracle call:
#   E1a  reuses a prepared run table that is a pure function of the generated
#        tables and a traversal state Task 44 already proves unchanged;
#        -94,784, down on 128/128 frames, 4-VBlank frames 50 -> 12 of 566.
#   E2   sends the identical Task 36 word stream by GXFIFO DMA instead of a CPU
#        store loop; -30,912.
#   E7   hoists the camera operands out of the per-binding compose loop;
#        -11,840, and binding_composed[] is bit-identical to the pre-E7 arm for
#        all 42 bindings at frames 260/420/500/700/1100/1700.
override NDS_R2_STAGE_DIRECT := 1
override NDS_R2_STAGE_DMA := 1
override NDS_R2_STAGE_VIEWPROJ := 1
# R2-02 E8, 2026-07-28. Elides the owner preflight for the five segments the
# Task 36 replay does not serve, whose outputs nothing reads once E1a's table is
# valid. STG P50 212,480 -> 177,088, under the 180,000 phase budget; 2-VBlank
# frames 13 -> 198 of 565. Exact: the DS top screen is pixel-identical to the
# pre-E8 arm at the simulation-clock lock, and the elision counter reads exactly
# 5 per frame.
override NDS_R2_STAGE_PREFLIGHT := 1
# R2-03 E9: the fighter-parts matrix path converted float -> N64 16.16 -> DS
# 20.12, and the intermediate was a lossless round trip -- COMBINE_INTEGRAL/
# COMBINE_FRACTIONAL split exactly what MtxCellS16p16 recombines. Converting
# straight to 20.12 is bit-exact by construction, not by tolerance. A/B on
# identical source: MatrixPrep 122,765 -> 110,777, world build 105,425 ->
# 93,830. Exact: 0 mismatches over 8,108 conversions against the two-step, and
# 0 fallbacks to the source routine.
override NDS_R2_FIGHTER_MTX_DIRECT := 1
# R2-03 E12: the native fighter path has no Gfx command site, so the resolver's
# existing site cache -- keyed on state->source_command_site -- misses on every
# fighter run and the full resolve runs each time. The memo is that same cache
# re-keyed on run_index, which E5 proved is what the answer depends on. A/B on
# identical source: texture prepare 45,952 -> 12,362, the whole function 82,042
# -> 49,318. Nine distinct textured runs are resolved once each for the entire
# match: 1,074 hits, 9 fills, 0 stale entries, and 0 mismatches over 1,083
# level-2 comparisons.
override NDS_R2_FIGHTER_RUN_MEMO := 1
# Task 37: seven hot leaves (memset, memcpy, memcmp, __ieee754_sqrtf and three
# renderer/fighter helpers, 906 bytes) into ITCM free space. Named work P50
# -59,328 ticks, 3-VBlank share 71.7% -> 76.0%, 5+ VBlank 5.2% -> 3.1%.
#
# Shipping on the owner's explicit decision, 2026-07-22, with the state-hash
# gate still RED. That is deliberate and the reasoning is on the record:
# Task 45 dumped the raw FTStruct bytes of both builds and found all 215
# differing words are main-RAM heap pointers offset by exactly +0x180 -- the
# image shrinks 384 bytes when this code leaves .main, so every heap object
# below it relocates. Zero gameplay values differ. The gate is reporting
# relocated addresses as changed state. The leak was NOT root-caused (two
# mechanism hypotheses were falsified), so the gate stays red rather than being
# adjusted to pass. See ClaudeFable5_Task45_FTStructLocalize_20260722.md.
#
# P2 correctness rebank, 2026-08-30.  LEAVES is a bitmask
# (1=libc 2=libm 4=port).  The later shipping v4-c238 census measured the
# Task-37 libm __ieee754_sqrtf leaf at just 60 cycles total / 236 code bytes;
# keep the still-hot libc + port leaves (mask 5) and return that stale libm
# admission to main RAM.  This is placement-only and pairs with the
# _arm_fixunssfsi main-RAM placement above to restore hard ITCM headroom for
# the source-faithful live model-part root resolver.
override NDS_TASK37_ITCM_LEAVES := 5
override NDS_SCENE_MIP_CACHE_LAB := 0
# Device-proven: boots to GO on melonDS and retail hardware with no OOM.
override NDS_FAST_WALLPAPER_AFFINE := 1
override NDS_RENDERER_BATTLE_STATIC_TEXTURE_DEFAULT := 1
override NDS_IFCOMMON_HYBRID_OAM := 0
override NDS_AUDIO_FGM_ARM7_ACK_DIAGNOSTICS := 0
override NDS_TASK16_FLOAT_COMPARE := 1
override NDS_TASK16_FLOAT_I2F := 1
override NDS_TASK16_FLOAT_ADDSUB := 1
# Retail Task-32 A/B reduced the normalized 4+/5+ pacing tail.
override NDS_TASK32_DRAW_HOT_TEXT := 1
# Task 39: the owner-approved hurt flash, hit sparks, and flat 2D shield.
override NDS_TASK39_FX_SPRITES := 1
override NDS_TASK39_FX_FLASH := 1
endif
ifneq ($(filter $(TARGET),smash64ds-battle-playable-tickhud-hwtri smash64ds-p2-fourcpu-tickhud-hwtri smash64ds-battle-playable-proof-hwtri smash64ds-battle-playable-audio-fgm-hwtri smash64ds-results-lab-hwtri),)
# Profile-0 shipping path plus either the lightweight Task 41 timers or the
# full diagnostic publications required by GDB proof runs.
#
# smash64ds-results-lab-hwtri and the focused audio-FGM proof target ride this
# block deliberately. Results must differ only in the scene it boots; audio-FGM
# must differ only in what its verifier observes. Both therefore compile the
# same accepted renderer/gameplay path as the shipping/proof ROM rather than
# inheriting stale generic defaults. Adding them to the filter instead of
# cloning the block is what keeps future accepted-path flags from drifting.
ifeq ($(TARGET),smash64ds-results-lab-hwtri)
override NDS_DEV_SCENE_HARNESS := results_playable
else
override NDS_DEV_SCENE_HARNESS := battle_playable_realtime
endif
override NDS_DEV_LIVE_INPUT_PREVIEW := 1
override NDS_HARNESS_FAST_LOGIC := 0
override NDS_RENDERER_HW_TRIANGLES := 1
override NDS_DEBUG_HUD := 0
override NDS_RENDERER_PROFILE_LEVEL := 0
ifneq ($(filter $(TARGET),smash64ds-battle-playable-tickhud-hwtri smash64ds-p2-fourcpu-tickhud-hwtri smash64ds-results-lab-hwtri),)
override NDS_SHIP_TELEMETRY := 0
override NDS_TICK_HUD := 1
else
override NDS_SHIP_TELEMETRY := 1
override NDS_TICK_HUD := 0
endif
override NDS_RENDERER_FAST_RUN_DEFAULT := 9
override NDS_NATIVE_STAGE_GENERATED_SEGMENT0_ENABLE := 1
override NDS_TASK36_HW_COMPOSE := 2
# R2-08: THE SWITCH (SwitchPlan §6), the measurement/proof half. This block must
# carry every published flag or the instrument measures a path the user is not
# running, and check-tickhud-parity.ps1 fails a one-sided edit (BENCH_MAKE_R2_PATH,
# Makefile:4203).
#
# smash64ds-results-lab-hwtri is EXCLUDED, and it is not a taste call: this
# block's filter admits it, but it overrides NDS_DEV_SCENE_HARNESS to
# results_playable above, and both taskman_seam.c and src/nds/r2/nds_r2_battle.c
# `#error` when NDS_R2_PATH=1 under any harness but battle_playable. Without the
# guard the Results lab stops compiling. check-tickhud-parity.ps1 compares only
# the published and tick-HUD targets, so the exclusion cannot hide drift.
ifneq ($(TARGET),smash64ds-results-lab-hwtri)
ifneq ($(NDS_R2_LAB_R1_PATH),1)
override NDS_R2_PATH := 1
endif
endif
# R2-03 E17/E16: match the published block. Any flag there is on this one too.
override NDS_R2_FIGHTER_HW_MTX := 1
# GX compose is owner-accepted and now ships enabled. Measurement/proof siblings
# must match the published renderer rather than requiring the historical _LAB
# escape, or the instrument would measure a path the user is not running.
override NDS_R2_FIGHTER_GX_COMPOSE := 1
# Slice 44. This is the instrument every measurement runs on, so it has to stay
# flag-identical to the published block -- a stride on one and not the other
# would put ~35,900 of WORK-H P95 between the ROM being judged and the ROM
# doing the judging.
override NDS_R2_STAGE_VALIDATE_STRIDE := 8
override NDS_R2_FIGHTER_HW_LIGHT := 1
# R2-03 E32. See the published block for the accepted visual residual.
override NDS_R2_FIGHTER_SHUFFLE_FOLD := 1
# R2-03 E64b/E65. See the published block, including how the equivalence was
# finally settled -- with a host error bound, not the state hash.
ifneq ($(NDS_R2_LAB_CUBIC_OFF),1)
override NDS_R2_CUBIC_FIXED := 1
endif
# R2-03 E46: the fighter state-delta path into ITCM. The switch was already
# resident; the span loop and every Record*/SyncTextureTile helper it dispatches
# to were in main RAM, so all 134.5 before-span applications a frame left
# zero-wait ITCM for icache-served code. Placement only -- no behaviour change.
# FTR P50 -12,032, WORK P50 -12,416 over the same 128-frame window, with the
# untouched STG/SRC buckets moving +768/-1,216 to bound the noise. The gain is
# 4x the state-span bracket's -3,128 because ApplyMaterial (27.7/frame) and the
# texture prepare (46.4/frame) call the same helpers. +1,016 bytes of .itcm,
# 1,896 still free.
override NDS_R2_DELTA_PATH_ITCM := 1
# R2-04 E1/E4/E5: fighter animation payload cache plus a stepped warm preload.
# lbRelocGetForceExternHeapFile re-read an animation off the cartridge on every
# call. The cache keys on asset_id and stores the byte-swapped pre-fixup image,
# re-running fixups per destination so "force" still restores pristine data. The
# warm list makes the match's 41 measured animations (91,104 bytes) resident one
# per scene update across the countdown; loading all 41 in one call at
# scVSBattleStartBattle misses a BGM buffer seam and kills the music for the
# match. Misses 29 -> 2, WORK-H P95 1,364,992 -> 1,232,640.
override NDS_R2_ANIM_CACHE := 1
# Cycle 108: matches the published block. See its comment for the measurement.
override NDS_R2_AOBJ16_PREBAKE := 1
# Task 53: matches the published block -- replay must be active here too or
# every tick-HUD STG bucket reads a different binary than the shipping ROM.
override NDS_TASK53_REPLAY_ARENA_FIX := 1
# Task 49: battle-pipeline selector. Standing rule: any flag on the published
# block is on this block too -- a tick-HUD/proof reading a different binary
# silently corrupts every measurement.
override NDS_BATTLE_PROFILE := 1
# Task 44: stage steady-state admission + dense binding lists. Exact (no
# fidelity change); ships on with Task 36 replay.
override NDS_TASK44_STAGE_STEADY := 1
# R2-02 E1a/E2/E7, graduated with the published block 2026-07-28. Defaulted on
# so a bare tick-HUD build measures the shipping stage program, but deliberately
# NOT `override`: these three are the live A/B surface for the rest of R2-02,
# and an override beats the command line, which would make the measurement
# target unable to measure the thing it exists to measure.
ifneq ($(origin NDS_R2_STAGE_DIRECT),command line)
NDS_R2_STAGE_DIRECT := 1
endif
ifneq ($(origin NDS_R2_STAGE_DMA),command line)
NDS_R2_STAGE_DMA := 1
endif
ifneq ($(origin NDS_R2_STAGE_VIEWPROJ),command line)
NDS_R2_STAGE_VIEWPROJ := 1
endif
ifneq ($(origin NDS_R2_STAGE_PREFLIGHT),command line)
NDS_R2_STAGE_PREFLIGHT := 1
endif
ifneq ($(origin NDS_R2_FIGHTER_MTX_DIRECT),command line)
NDS_R2_FIGHTER_MTX_DIRECT := 1
endif
ifneq ($(origin NDS_R2_FIGHTER_RUN_MEMO),command line)
NDS_R2_FIGHTER_RUN_MEMO := 1
endif
# Must track the published block above. These two targets exist to measure and
# prove the shipping program, so any flag that is on there and off here makes
# every tick-HUD bucket and every GDB proof a reading of a different binary.
# Track the published placement exactly (2026-08-30 rebank: libc + port).
override NDS_TASK37_ITCM_LEAVES := 5
ifeq ($(TARGET),smash64ds-results-lab-hwtri)
ifeq ($(NDS_TASK91_DRAW_PHASE_CENSUS),1)
# Task 91 instruments the fighter owner itself, including ITCM-resident hot
# helpers.  The shipping-equivalent Results lab keeps only 736 bytes of spare
# ITCM with the accepted Task-37 placement, so adding the census overflows the
# linker before it can measure anything.  Placement is not the mechanism Task
# 91 measures; give this diagnostic build its code headroom without changing
# the ordinary Results lab or any published configuration.
override NDS_TASK37_ITCM_LEAVES := 0
endif
endif
override NDS_SCENE_MIP_CACHE_LAB := 0
override NDS_FAST_WALLPAPER_AFFINE := 1
override NDS_RENDERER_BATTLE_STATIC_TEXTURE_DEFAULT := 1
override NDS_IFCOMMON_HYBRID_OAM := 0
override NDS_AUDIO_FGM_ARM7_ACK_DIAGNOSTICS := 0
override NDS_TASK16_FLOAT_COMPARE := 1
override NDS_TASK16_FLOAT_I2F := 1
override NDS_TASK16_FLOAT_ADDSUB := 1
ifeq ($(TARGET),smash64ds-results-lab-hwtri)
ifeq ($(NDS_TASK91_DRAW_PHASE_CENSUS),1)
# The Task-91 counters add 296 bytes inside already-ITCM native-owner code.
# The generic Task-16 compare + i2f placement is 328 bytes in this build and
# is outside the mechanism under census, so evict only those two diagnostic
# leaves.  Keep add/sub and all native renderer/matrix placement unchanged.
override NDS_TASK16_FLOAT_COMPARE := 0
override NDS_TASK16_FLOAT_I2F := 0
endif
endif
override NDS_TASK32_DRAW_HOT_TEXT := 1
override NDS_TASK39_FX_SPRITES := 1
override NDS_TASK39_FX_FLASH := 1
ifeq ($(TARGET),smash64ds-p2-fourcpu-tickhud-hwtri)
# P2-2 standing stress arm. It deliberately boots straight into the source
# VSBattle path instead of walking the CSS: the gate is measuring four-fighter
# gameplay, not menu automation. Restore the source effect capacity too; a
# four-way burst measured through P1's reduced 12-entry pool would be a different
# game-state policy, not merely a tighter memory budget.
override NDS_P2_FOUR_CPU_STRESS := 1
override NDS_R2_EFFECT_POOL := 38
ifeq ($(NDS_P2_FOUR_CPU_ROSTER),1)
# P2-3r15: THIS IS NOW THE DEFAULT ARM, not the lab arm -- the flag defaults to
# 1 on this target (see its declaration above). `NDS_P2_FOUR_CPU_ROSTER=0`
# rebuilds the Mario/Fox mirror roster and is the control every A/B against a
# pre-2026-08-25 figure has to use.
#
# P2-3r13. THE ROSTER ARM IS SHIPPING-SHAPED: NOTHING IS OVERRIDDEN HERE
# EXCEPT THE ROSTER ITSELF.
#
# P2-3r11 got four distinct kinds through a whole match by setting
# `NDS_R2_BATTLEPACK := 0` on this arm plus a 32,768 B animation-cache trim,
# because each distinct fighter kind pays its own main-file tree out of the
# taskman arena at battle setup -- Mario 54,048, Fox 119,040, Luigi 57,104,
# Donkey 79,648 (generated alloc-size tables,
# include/nds/generated/nds_fighter_production.generated.h) -- and the arm died
# in `ftManagerSetupFilesMainKind(nFTKindDonkey)` asking 77,360 B with 8,300 B
# free (artifacts/verification/2026-08-25_r11-arena-overflow.txt).
#
# Both overrides are GONE, and the ~186 KB they were paying for came from
# somewhere that costs no gameplay CPU at all: 185,696 B of ARM9 .bss -- the
# title / opening-action / Peach's-Castle scene file store, which a VSBattle
# never touches -- left the static image for the scene arena
# (`ndsRelocSceneFileBuffer`, src/port/reloc_backend_assets.c), and
# NDS_TASKMAN_ARENA_SIZE rose 0x17a000 -> 0x1a7000 by 184,320 of it. Fox's
# prebuilt clip pack and the 262,144 B animation reservation are therefore
# resident on this arm exactly as they are in builds/build-p2-shell, so a tick
# figure from here is comparable with the mirror roster's again.
#
# THIS ARM IS STILL NOT THE ARGMAX, AND P2-3f10 MEASURED WHY TWICE.
#
# The real argmax over landed content is Mario/Fox/Captain/Donkey = 348,320 B
# of unique per-kind arena against this roster's 289,712 B: LUIGI (41,552 B) is
# the cheapest of the five, not Mario (54,048 B), so he is the kind that drops
# out. P2-3f8 and P2-3f9 both had that backwards and proposed
# Luigi/Fox/Captain/Donkey, which is 12,496 B lighter than the real thing.
#
# P2-3f9's stated blocker is GONE. It put Captain in slot 0, saw **about 30x
# the wall time for the same guest frames**, and backed out. P2-3f10 attributed
# that to an ABORT-mode data abort in `efManagerCaptainEntryCarMakeEffect` --
# the effect-desc deferral table in battleship_efmanager.c held four slots and
# the landed roster defers seven descs, so Falcon's Flyer was neutralised with
# no way back and the source maker walked a NULL DObj. What the wall clock
# measured afterwards was melonDS grinding through a wandering ARM9, which is
# why every memory counter read clean and identical to the control. Fixed and
# asserted.
#
# P2-3f11 re-pointed this arm to the then-measured landed-content argmax,
# Mario/Fox/Captain/Donkey. P2-3f22 moves it again now that Samus is landed:
# SamusMain is 85,296 B standalone / 83,008 B unique after the same 2,288 B
# shared pair, so Samus replaces Mario and the six-kind argmax is
# Samus/Fox/Captain/Donkey. Luigi remains admitted because native-owner slots are
# a dense ABI (Donkey and Captain build on the already-qualified Luigi slot), but
# neither Luigi nor Mario is instantiated by the direct battle descriptor.
override NDS_P2_LUIGI := 1
override NDS_P2_DONKEY := 1
override NDS_P2_CAPTAIN := 1
override NDS_P2_SAMUS := 1
endif
endif
endif
# Source-state gameplay proof target.  Fast logic cannot use NDS_R2_PATH (the
# R2 battle loop is the realtime loop), but its renderer must still be a
# coherent current DS configuration.  This target had been relying on generic
# defaults and drifted as accepted renderer features graduated globally, first
# producing an impossible profile-2/Task56 pair and then a partial GX-compose
# configuration.  Keep the bounded gameplay loop, but pin the same accepted
# renderer/backend bundle as the scene-boundary fast target below.  No tick
# figure from this target is a performance figure.
ifeq ($(TARGET),smash64ds-battle-playable-fast-hwtri)
override NDS_DEV_SCENE_HARNESS := battle_playable
override NDS_DEV_LIVE_INPUT_PREVIEW := 0
override NDS_HARNESS_FAST_LOGIC := 1
# The battlepack blob buys gameplay-frame CPU at the price of ~288 KiB of
# arena residency. This target's charter (above) says no tick figure from it
# is a performance figure, and with the pack resident the bounded proof's
# memory ledger ends the run at 95,424 B headroom against its 131,072 B
# reserve (P2-3r3, 2026-08-23). BATTLEPACK=0 with KEEP_CACHE=1 is the
# measured isolation-control arm (2026-08-15 BATTLEPACK_ISOLATION.md): the
# grown arena and raw cache stay exactly as the shipping pair has them, only
# the blob is absent — headroom returns without changing any address the
# pack arm measured.
override NDS_R2_BATTLEPACK := 0
override NDS_R2_BATTLEPACK_KEEP_CACHE := 1
override NDS_RENDERER_HW_TRIANGLES := 1
override NDS_DEBUG_HUD := 0
override NDS_RENDERER_PROFILE_LEVEL := 0
override NDS_SHIP_TELEMETRY := 1
override NDS_TICK_HUD := 0
override NDS_RENDERER_FAST_RUN_DEFAULT := 9
override NDS_NATIVE_STAGE_GENERATED_SEGMENT0_ENABLE := 1
override NDS_TASK36_HW_COMPOSE := 2
override NDS_R2_FIGHTER_HW_MTX := 1
override NDS_R2_FIGHTER_GX_COMPOSE := 1
# Task 40's cycler is profile-1-only by design: its large diagnostic arrays and
# markers must never enter the profile-0 shipping image. The historical coarse
# profile target predates the current native-owner renderer and no longer forms
# a coherent build (GX compose + profile instrumentation violates the current
# soft-light contract). Reuse this bounded, non-performance gameplay target for
# the audit instead. Keep the accepted split-matrix path, but let the CPU own the
# hierarchy compose while profile-1 instrumentation is present; the audit is
# proving animation acquisition/playback, not renderer timing or GX equivalence.
ifneq ($(NDS_FIGHTER_ANIM_AUDIT),0)
override NDS_RENDERER_PROFILE_LEVEL := 1
override NDS_R2_FIGHTER_GX_COMPOSE := 0
endif
override NDS_R2_STAGE_VALIDATE_STRIDE := 8
override NDS_R2_FIGHTER_HW_LIGHT := 1
override NDS_R2_FIGHTER_SHUFFLE_FOLD := 1
override NDS_R2_CUBIC_FIXED := 1
override NDS_R2_DELTA_PATH_ITCM := 1
override NDS_R2_ANIM_CACHE := 1
override NDS_R2_AOBJ16_PREBAKE := 1
override NDS_TASK53_REPLAY_ARENA_FIX := 1
override NDS_BATTLE_PROFILE := 1
override NDS_TASK44_STAGE_STEADY := 1
override NDS_R2_STAGE_DIRECT := 1
override NDS_R2_STAGE_DMA := 1
override NDS_R2_STAGE_VIEWPROJ := 1
override NDS_R2_STAGE_PREFLIGHT := 1
override NDS_R2_FIGHTER_MTX_DIRECT := 1
override NDS_R2_FIGHTER_RUN_MEMO := 1
override NDS_TASK37_ITCM_LEAVES := 5
override NDS_SCENE_MIP_CACHE_LAB := 0
override NDS_FAST_WALLPAPER_AFFINE := 1
override NDS_RENDERER_BATTLE_STATIC_TEXTURE_DEFAULT := 1
override NDS_IFCOMMON_HYBRID_OAM := 0
override NDS_AUDIO_FGM_ARM7_ACK_DIAGNOSTICS := 0
override NDS_TASK16_FLOAT_COMPARE := 1
override NDS_TASK16_FLOAT_I2F := 1
override NDS_TASK16_FLOAT_ADDSUB := 1
override NDS_TASK32_DRAW_HOT_TEXT := 1
override NDS_TASK39_FX_SPRITES := 1
override NDS_TASK39_FX_FLASH := 1
endif
# P2-1b scene-loop-walk lab target. Its OWN block for the same reason the Task
# 49 differ has one: appending a member to the tickhud/proof filter above
# breaks the structural pin at check-gbi-decode-fixtures.ps1:1847.
#
# It is the proof build with ONE flag changed -- NDS_HARNESS_FAST_LOGIC := 1.
# The walk measures the SCENE BOUNDARY (does entry k+1 into a scene reach the
# same arena high-water as entry k), not gameplay, and it cannot run at
# NDS_HARNESS_FAST_LOGIC := 0 for a structural reason: the VS Results branch of
# syTaskmanRunTask loops until sSYTaskmanStatus becomes LoadScene, which in
# realtime means "until a human presses START", so the loop would never close
# on its own. Everything else is copied from the proof block deliberately --
# a walk over a differently-configured binary would be measuring a scene
# boundary the shipping ROM does not have.
#
# NEVER PUBLISHED AND NEVER A PERFORMANCE SURFACE. Fast logic is not the
# shipping cadence; no tick figure from this target means anything.
ifeq ($(TARGET),smash64ds-p2-1b-scene-walk-hwtri)
override NDS_DEV_SCENE_HARNESS := battle_playable_realtime
override NDS_DEV_LIVE_INPUT_PREVIEW := 1
override NDS_HARNESS_FAST_LOGIC := 1
override NDS_RENDERER_HW_TRIANGLES := 1
override NDS_DEBUG_HUD := 0
override NDS_RENDERER_PROFILE_LEVEL := 0
override NDS_SHIP_TELEMETRY := 1
override NDS_TICK_HUD := 0
override NDS_RENDERER_FAST_RUN_DEFAULT := 9
override NDS_NATIVE_STAGE_GENERATED_SEGMENT0_ENABLE := 1
override NDS_TASK36_HW_COMPOSE := 2
# NDS_R2_PATH is deliberately NOT set here, and it is the one place this target
# is not the proof build: taskman_seam.c:21 refuses NDS_R2_PATH=1 with
# NDS_HARNESS_FAST_LOGIC=1, because the R2 battle loop IS the realtime loop.
# The walk keeps fast logic and loses the R2 battle path. That is sound for
# what it measures and unsound for anything else: scene entry and teardown are
# syTaskmanStartTask on both paths, so the per-entry arena high-water is the
# same question -- but the battle scene's own allocation set is not identical
# to the shipping one, so a number from this target says "the boundary does not
# leak", never "the shipping battle costs N".
override NDS_R2_FIGHTER_HW_MTX := 1
override NDS_R2_FIGHTER_GX_COMPOSE := 1
override NDS_R2_STAGE_VALIDATE_STRIDE := 8
override NDS_R2_FIGHTER_HW_LIGHT := 1
override NDS_R2_FIGHTER_SHUFFLE_FOLD := 1
override NDS_R2_CUBIC_FIXED := 1
override NDS_R2_DELTA_PATH_ITCM := 1
override NDS_R2_ANIM_CACHE := 1
override NDS_R2_AOBJ16_PREBAKE := 1
override NDS_TASK53_REPLAY_ARENA_FIX := 1
override NDS_BATTLE_PROFILE := 1
override NDS_TASK44_STAGE_STEADY := 1
override NDS_R2_STAGE_DIRECT := 1
override NDS_R2_STAGE_DMA := 1
override NDS_R2_STAGE_VIEWPROJ := 1
override NDS_R2_STAGE_PREFLIGHT := 1
override NDS_R2_FIGHTER_MTX_DIRECT := 1
override NDS_R2_FIGHTER_RUN_MEMO := 1
override NDS_TASK37_ITCM_LEAVES := 5
override NDS_SCENE_MIP_CACHE_LAB := 0
override NDS_FAST_WALLPAPER_AFFINE := 1
override NDS_RENDERER_BATTLE_STATIC_TEXTURE_DEFAULT := 1
override NDS_IFCOMMON_HYBRID_OAM := 0
override NDS_AUDIO_FGM_ARM7_ACK_DIAGNOSTICS := 0
override NDS_TASK16_FLOAT_COMPARE := 1
override NDS_TASK16_FLOAT_I2F := 1
override NDS_TASK16_FLOAT_ADDSUB := 1
override NDS_TASK32_DRAW_HOT_TEXT := 1
override NDS_TASK39_FX_SPRITES := 1
override NDS_TASK39_FX_FLASH := 1
endif
# P2-1d VS shell lab target: the playable menu flow, in the SHIPPING battle
# configuration.
#
# It is the proof block with three flags added -- NDS_P2_UI_KIT,
# NDS_P2_MENU_SHELL and NDS_P2_MENU_WALK -- and nothing removed, which is the
# whole point. P2-1c's kit target could use `normal` because it drew a surface
# and touched no battle; this row's flow ENDS IN THE MATCH, so the match it
# reaches has to be the canonical one or the run proves nothing about entering
# a battle through menus. Harness 163 keeps the ARM renderer, the static
# texture pack, and every gameplay flag the Boundary arm carries; the shell
# flag moves only the BOOT SCENE, from the battle to the splash.
#
# NDS_P2_MENU_WALK := 1 drives the screens' own input handlers through one
# scripted pass with a dwell on each screen, so a single run is both the
# cadence measurement and the capture window for every screenshot. The battle
# it enters then runs for real at NDS_HARNESS_FAST_LOGIC := 0.
#
# NEVER PUBLISHED. The boot scene differs from the shipped ROM's, so no tick
# figure from it is a Boundary figure.
#
# P2-1d/1e/1f EACH ADDED A NAME, NOT A BLOCK -- the character select and the
# stage select are more screens inside NDS_P2_MENU_SHELL and wanted exactly
# these flags, so all three selected this one block and the only measurable
# difference between them was the ROM's filename.
#
# P2-1g COLLAPSES THE THREE INTO ONE PHASE NAME. Every row in P2-1 is closed,
# the shell is one screen set rather than three in progress, and a per-row lab
# name outliving its row is scaffolding: the next reader cannot tell which of
# three identical ROMs is current. `smash64ds-p2-shell-hwtri` is the shipping-
# configuration shell arm -- the cadence, screenshot and realtime-through-menus
# surface the phase closes on -- and the loop block below carries the other.
NDS_P2_MENU_SHELL_TARGETS := \
	smash64ds-p2-shell-hwtri
ifneq ($(filter $(TARGET),$(NDS_P2_MENU_SHELL_TARGETS)),)
override NDS_DEV_SCENE_HARNESS := battle_playable_realtime
override NDS_DEV_LIVE_INPUT_PREVIEW := 1
override NDS_HARNESS_FAST_LOGIC := 0
override NDS_RENDERER_HW_TRIANGLES := 1
override NDS_DEBUG_HUD := 0
override NDS_RENDERER_PROFILE_LEVEL := 0
override NDS_TICK_HUD := 0
override NDS_SHIP_TELEMETRY := 1
override NDS_RENDERER_FAST_RUN_DEFAULT := 9
override NDS_NATIVE_STAGE_GENERATED_SEGMENT0_ENABLE := 1
override NDS_TASK36_HW_COMPOSE := 2
override NDS_R2_FIGHTER_HW_MTX := 1
override NDS_R2_FIGHTER_GX_COMPOSE := 1
override NDS_R2_STAGE_VALIDATE_STRIDE := 8
override NDS_R2_FIGHTER_HW_LIGHT := 1
override NDS_R2_FIGHTER_SHUFFLE_FOLD := 1
override NDS_R2_CUBIC_FIXED := 1
override NDS_R2_DELTA_PATH_ITCM := 1
override NDS_R2_ANIM_CACHE := 1
override NDS_R2_AOBJ16_PREBAKE := 1
override NDS_TASK53_REPLAY_ARENA_FIX := 1
override NDS_BATTLE_PROFILE := 1
override NDS_TASK44_STAGE_STEADY := 1
override NDS_R2_STAGE_DIRECT := 1
override NDS_R2_STAGE_DMA := 1
override NDS_R2_STAGE_VIEWPROJ := 1
override NDS_R2_STAGE_PREFLIGHT := 1
override NDS_R2_FIGHTER_MTX_DIRECT := 1
override NDS_R2_FIGHTER_RUN_MEMO := 1
override NDS_TASK37_ITCM_LEAVES := 5
override NDS_SCENE_MIP_CACHE_LAB := 0
override NDS_FAST_WALLPAPER_AFFINE := 1
override NDS_RENDERER_BATTLE_STATIC_TEXTURE_DEFAULT := 1
override NDS_IFCOMMON_HYBRID_OAM := 0
override NDS_AUDIO_FGM_ARM7_ACK_DIAGNOSTICS := 0
override NDS_TASK16_FLOAT_COMPARE := 1
override NDS_TASK16_FLOAT_I2F := 1
override NDS_TASK16_FLOAT_ADDSUB := 1
override NDS_TASK32_DRAW_HOT_TEXT := 1
override NDS_TASK39_FX_SPRITES := 1
override NDS_TASK39_FX_FLASH := 1
override NDS_P2_UI_KIT := 1
override NDS_P2_MENU_SHELL := 1
# The roster ladder; it is defined once, near NDS_P2_SHELL_ROSTER.
$(eval $(NDS_P2_SHELL_ROSTER_LADDER))
# The eight opt-in VS stages. Overridden HERE rather than defaulted to 1 in
# the flag block, because a global default would also grow the P1 proof ROM
# and every lab target that names no stage flag. All eight boot and play
# under the scripted lap with stage identity asserted; the item core derives
# itself from them, so the items come with them.
override NDS_P2_STAGE_YOSTER := 1
override NDS_P2_STAGE_CASTLE := 1
override NDS_P2_STAGE_JUNGLE := 1
override NDS_P2_STAGE_ZEBES := 1
override NDS_P2_STAGE_HYRULE := 1
override NDS_P2_STAGE_YAMABUKI := 1
override NDS_P2_STAGE_INISHIE := 1
override NDS_P2_STAGE_SECTOR := 1
## P2-2 source parity. BattleShip's efmanager.c owns 38 EFStructs and keeps its
## own last-four forced-effect reserve. The old 12-entry P1 cap changes which
## cosmetic effects survive a four-way burst, so the four-fighter shell restores
## the source depth; the P1 direct-boot/perf targets keep their measured cap.
override NDS_R2_EFFECT_POOL := 38
override NDS_P2_MENU_WALK := 1
# P2-1M gate catch (2026-08-19): the P1 demo ladder is direct-boot demo
# behaviour. In the shell game the CHARACTER SELECT decides Fox's level, so
# the ladder must not ride any shell configuration; the gcrunall pin derives
# its expected level from this define and the CSS commits 3.
override NDS_DEMO_FOX_CPU_LADDER := 0
endif
# P2-1L item (10) FREE-PLAY lab target: the shipping-configuration shell
# above, minus the one flag that makes that ROM scripted. It is a full copy
# of the NDS_P2_MENU_SHELL_TARGETS block rather than a delta on it, on
# purpose -- P2-1b's own comment (above) already ruled out appending a name
# to a shared filter list, and the loop-verifier block below is the
# precedent for "a distinct shell variant gets its own full block".
#
# NDS_P2_MENU_WALK IS NOT OVERRIDDEN, so it stays the Makefile default (0)
# -- and 0 is not merely inert, it is ABSENT. Every walk site is
# `#if NDS_P2_MENU_WALK`: the dwell scripts (kNdsMenuWalk*), the tap
# injector (ndsMenuShellWalkTap, nds_menu_shell.c:485), and the Results
# auto-START (ndsMenuShellWalkWantsResultsStart, :580, and its
# NDS_P2_MENU_SHELL && NDS_P2_MENU_WALK call site in
# nds_platform.c:503) all fail to exist in this translation unit rather than
# merely reading zero. What is left in `ndsMenuShellReadTaps` is
# `ndsPlatformReadInput()` calling `scanKeys()`/`keysHeld()` -- the same live
# keypad read controller_backend.c's `osContGetReadData` uses for the battle
# this shell opens into -- so the whole shell, menus through the match, runs
# on nothing but a human DS pad. NDS_HARNESS_FAST_LOGIC stays 0 and
# NDS_DEV_SCENE_HARNESS stays battle_playable_realtime, so the match is the
# real realtime one, not a bounded proof run.
#
# P2-1M (owner, 2026-08-19): "smash64ds is the base now." The published base
# ROM and the free-play lab ROM are the SAME configuration by construction --
# one flag block, two output names. `smash64ds` hardcodes its output to the
# project root like every published name; the lab twin stays in builds/ for
# side-by-side testing without touching the published artifact. The gate's
# battle arm runs this configuration plus NDS_P2_MENU_WALK (unattended pass),
# so the published ROM is verifier-covered per the publish law. No tick
# figure from the free-play name is a Boundary figure; the gate measures its
# own walk arm.
NDS_P2_MENU_SHELL_FREEPLAY_TARGETS := \
	smash64ds-p2-shell-freeplay-hwtri \
	smash64ds
ifneq ($(filter $(TARGET),$(NDS_P2_MENU_SHELL_FREEPLAY_TARGETS)),)
override NDS_DEV_SCENE_HARNESS := battle_playable_realtime
override NDS_DEV_LIVE_INPUT_PREVIEW := 1
override NDS_HARNESS_FAST_LOGIC := 0
override NDS_RENDERER_HW_TRIANGLES := 1
override NDS_DEBUG_HUD := 0
override NDS_RENDERER_PROFILE_LEVEL := 0
override NDS_TICK_HUD := 0
override NDS_SHIP_TELEMETRY := 1
override NDS_RENDERER_FAST_RUN_DEFAULT := 9
override NDS_NATIVE_STAGE_GENERATED_SEGMENT0_ENABLE := 1
override NDS_TASK36_HW_COMPOSE := 2
override NDS_R2_FIGHTER_HW_MTX := 1
override NDS_R2_FIGHTER_GX_COMPOSE := 1
override NDS_R2_STAGE_VALIDATE_STRIDE := 8
override NDS_R2_FIGHTER_HW_LIGHT := 1
override NDS_R2_FIGHTER_SHUFFLE_FOLD := 1
override NDS_R2_CUBIC_FIXED := 1
override NDS_R2_DELTA_PATH_ITCM := 1
override NDS_R2_ANIM_CACHE := 1
override NDS_R2_AOBJ16_PREBAKE := 1
override NDS_TASK53_REPLAY_ARENA_FIX := 1
override NDS_BATTLE_PROFILE := 1
override NDS_TASK44_STAGE_STEADY := 1
override NDS_R2_STAGE_DIRECT := 1
override NDS_R2_STAGE_DMA := 1
override NDS_R2_STAGE_VIEWPROJ := 1
override NDS_R2_STAGE_PREFLIGHT := 1
override NDS_R2_FIGHTER_MTX_DIRECT := 1
override NDS_R2_FIGHTER_RUN_MEMO := 1
override NDS_TASK37_ITCM_LEAVES := 5
override NDS_SCENE_MIP_CACHE_LAB := 0
override NDS_FAST_WALLPAPER_AFFINE := 1
override NDS_RENDERER_BATTLE_STATIC_TEXTURE_DEFAULT := 1
override NDS_IFCOMMON_HYBRID_OAM := 0
override NDS_AUDIO_FGM_ARM7_ACK_DIAGNOSTICS := 0
override NDS_TASK16_FLOAT_COMPARE := 1
override NDS_TASK16_FLOAT_I2F := 1
override NDS_TASK16_FLOAT_ADDSUB := 1
override NDS_TASK32_DRAW_HOT_TEXT := 1
override NDS_TASK39_FX_SPRITES := 1
override NDS_TASK39_FX_FLASH := 1
override NDS_P2_UI_KIT := 1
override NDS_P2_MENU_SHELL := 1
# The roster ladder; it is defined once, near NDS_P2_SHELL_ROSTER.
$(eval $(NDS_P2_SHELL_ROSTER_LADDER))
# The eight opt-in VS stages. Overridden HERE rather than defaulted to 1 in
# the flag block, because a global default would also grow the P1 proof ROM
# and every lab target that names no stage flag. All eight boot and play
# under the scripted lap with stage identity asserted; the item core derives
# itself from them, so the items come with them.
override NDS_P2_STAGE_YOSTER := 1
override NDS_P2_STAGE_CASTLE := 1
override NDS_P2_STAGE_JUNGLE := 1
override NDS_P2_STAGE_ZEBES := 1
override NDS_P2_STAGE_HYRULE := 1
override NDS_P2_STAGE_YAMABUKI := 1
override NDS_P2_STAGE_INISHIE := 1
override NDS_P2_STAGE_SECTOR := 1
override NDS_R2_EFFECT_POOL := 38
# P2-1M gate catch: same rule as the walk block above — the CSS decides
# Fox's level in the shell game; the P1 demo ladder never rides it.
override NDS_DEMO_FOX_CPU_LADDER := 0
# P2-1L item (11): the one line that separates the OWNER's ROM from its lab
# siblings besides the walk. Nothing reads the console copy; the three globals
# behind it are unchanged and every verifier still reads them over gdb.
override NDS_BOOT_DIAG_TEXT := 0
endif
# P2-1g LOOP-VERIFIER TARGET: the same shell, walked twenty times, and the ROM
# the Boundary profile's loop arm runs. P2-1d/1e/1f each had their own name for
# this too; the phase is closed and one name replaces the three.
#
# NDS_HARNESS_FAST_LOGIC := 1 for the block's original reason -- the walk's
# BATTLE leg is a bounded run, because this measures the scene BOUNDARY over
# many laps and not gameplay. Every gameplay figure comes from the realtime
# shell arm above and from Boundary's own mode-163 arm; nothing here is a
# performance surface.
#
# NDS_R2_SCENE_LOOP_WALK := 0, AND THAT IS THE ROW'S POINT. P2-1b's substitute
# hop used to carry the Results -> menu leg, which closed a lap without ever
# running `ndsMNVSResultsSetLoadScene` -- the very function P2-1f rewrote for
# the shell and could not exercise. The walk now presses START on Results
# through the real keypad latch (`ndsMenuShellWalkWantsResultsStart`), so the
# lap closes through the source's own exit test and the rematch body runs on
# every one of the twenty laps. Every other leg already had a non-walk path:
# VS START and the stage select's confirm are the shell's own transitions, and
# battle -> Results is the source's (scvsbattle.c:560).
#
# NDS_P2_MENU_WALK := 20 is the phase-close gate written into the ROM, and
# `gNdsMenuShellWalkBudget` makes it a SEED rather than a pin -- a smoke run at
# three laps pokes that variable and costs no build.
#
# NEVER PUBLISHED.
NDS_P2_MENU_WALK_TARGETS := \
	smash64ds-p2-shell-loop-hwtri
ifneq ($(filter $(TARGET),$(NDS_P2_MENU_WALK_TARGETS)),)
override NDS_R2_SCENE_LOOP_WALK := 0
override NDS_P2_MENU_WALK := 20
override NDS_DEV_SCENE_HARNESS := battle_playable_realtime
override NDS_DEV_LIVE_INPUT_PREVIEW := 1
override NDS_HARNESS_FAST_LOGIC := 1
override NDS_RENDERER_HW_TRIANGLES := 1
override NDS_DEBUG_HUD := 0
override NDS_RENDERER_PROFILE_LEVEL := 0
override NDS_SHIP_TELEMETRY := 1
override NDS_TICK_HUD := 0
override NDS_RENDERER_FAST_RUN_DEFAULT := 9
override NDS_NATIVE_STAGE_GENERATED_SEGMENT0_ENABLE := 1
override NDS_TASK36_HW_COMPOSE := 2
override NDS_R2_FIGHTER_HW_MTX := 1
override NDS_R2_FIGHTER_GX_COMPOSE := 1
override NDS_R2_STAGE_VALIDATE_STRIDE := 8
override NDS_R2_FIGHTER_HW_LIGHT := 1
override NDS_R2_FIGHTER_SHUFFLE_FOLD := 1
override NDS_R2_CUBIC_FIXED := 1
override NDS_R2_DELTA_PATH_ITCM := 1
override NDS_R2_ANIM_CACHE := 1
override NDS_R2_AOBJ16_PREBAKE := 1
override NDS_TASK53_REPLAY_ARENA_FIX := 1
override NDS_BATTLE_PROFILE := 1
override NDS_TASK44_STAGE_STEADY := 1
override NDS_R2_STAGE_DIRECT := 1
override NDS_R2_STAGE_DMA := 1
override NDS_R2_STAGE_VIEWPROJ := 1
override NDS_R2_STAGE_PREFLIGHT := 1
override NDS_R2_FIGHTER_MTX_DIRECT := 1
override NDS_R2_FIGHTER_RUN_MEMO := 1
override NDS_TASK37_ITCM_LEAVES := 5
override NDS_SCENE_MIP_CACHE_LAB := 0
override NDS_FAST_WALLPAPER_AFFINE := 1
override NDS_RENDERER_BATTLE_STATIC_TEXTURE_DEFAULT := 1
override NDS_IFCOMMON_HYBRID_OAM := 0
override NDS_AUDIO_FGM_ARM7_ACK_DIAGNOSTICS := 0
override NDS_TASK16_FLOAT_COMPARE := 1
override NDS_TASK16_FLOAT_I2F := 1
override NDS_TASK16_FLOAT_ADDSUB := 1
override NDS_TASK32_DRAW_HOT_TEXT := 1
override NDS_TASK39_FX_SPRITES := 1
override NDS_TASK39_FX_FLASH := 1
override NDS_P2_UI_KIT := 1
override NDS_P2_MENU_SHELL := 1
# The roster ladder; it is defined once, near NDS_P2_SHELL_ROSTER.
$(eval $(NDS_P2_SHELL_ROSTER_LADDER))
# The eight opt-in VS stages. Overridden HERE rather than defaulted to 1 in
# the flag block, because a global default would also grow the P1 proof ROM
# and every lab target that names no stage flag. All eight boot and play
# under the scripted lap with stage identity asserted; the item core derives
# itself from them, so the items come with them.
override NDS_P2_STAGE_YOSTER := 1
override NDS_P2_STAGE_CASTLE := 1
override NDS_P2_STAGE_JUNGLE := 1
override NDS_P2_STAGE_ZEBES := 1
override NDS_P2_STAGE_HYRULE := 1
override NDS_P2_STAGE_YAMABUKI := 1
override NDS_P2_STAGE_INISHIE := 1
override NDS_P2_STAGE_SECTOR := 1
override NDS_R2_EFFECT_POOL := 38
endif
# Task 49 GX-differ lab target. Its OWN block (appending to the tickhud/proof
# block breaks the structural pin at check-gbi-decode-fixtures.ps1:1847).
# Profile 1 (oracle instrumentation), HW_COMPOSE=2 (capture the real shipping
# compose stream), affine off (profile 1 + affine OOMs the arena), and NO
# NDS_TASK37_ITCM_LEAVES so the differ hook has ITCM headroom. The tickhud
# block overrides ITCM_LEAVES:=7 and PROFILE_LEVEL:=0; those overrides beat
# the command line, so the differ must NOT build on the tickhud target.
ifeq ($(TARGET),smash64ds-battle-playable-task49-differ-hwtri)
override NDS_DEV_SCENE_HARNESS := battle_playable_realtime
override NDS_DEV_LIVE_INPUT_PREVIEW := 1
override NDS_HARNESS_FAST_LOGIC := 0
override NDS_RENDERER_HW_TRIANGLES := 1
override NDS_DEBUG_HUD := 0
override NDS_RENDERER_PROFILE_LEVEL := 1
override NDS_FAST_WALLPAPER_AFFINE := 0
override NDS_RENDERER_FAST_RUN_DEFAULT := 9
override NDS_NATIVE_STAGE_GENERATED_SEGMENT0_ENABLE := 1
override NDS_TASK36_HW_COMPOSE := 2
override NDS_TASK49_GX_DIFFER := 1
# Task 55 E2: the differ target keeps the replay path live (Task 53) so the
# Task 55 elision can be captured. NDS_TASK55_STAGE_GEOM stays command-line
# controlled here (0 = baseline stream, 1 = elided stream) so a single target
# captures both halves of the A/B.
override NDS_TASK53_REPLAY_ARENA_FIX := 1
endif
NDS_TASK37_DEVICE_TARGETS := \
	smash64ds-battle-playable-task37-on-hwtri \
	smash64ds-battle-playable-task37-off-hwtri
ifneq ($(filter $(TARGET),$(NDS_TASK37_DEVICE_TARGETS)),)
# Nonpublishing retail A/B pair for the Task 37 device checkpoint. Identical to
# the profile-0 tick-HUD build except for NDS_TASK37_ITCM_LEAVES; the distinct
# target and build names keep one ROM from overwriting the other. Placement is
# the device-only class -- the emulator now models icache/dcache, but TCM
# residency is still the category where melonDS was historically blind.
override NDS_DEV_SCENE_HARNESS := battle_playable_realtime
override NDS_DEV_LIVE_INPUT_PREVIEW := 1
override NDS_HARNESS_FAST_LOGIC := 0
override NDS_RENDERER_HW_TRIANGLES := 1
override NDS_DEBUG_HUD := 0
override NDS_RENDERER_PROFILE_LEVEL := 0
override NDS_SHIP_TELEMETRY := 0
override NDS_TICK_HUD := 1
override NDS_RENDERER_FAST_RUN_DEFAULT := 9
override NDS_NATIVE_STAGE_GENERATED_SEGMENT0_ENABLE := 1
override NDS_TASK36_HW_COMPOSE := 2
override NDS_TASK44_STAGE_STEADY := 1
# 7 = all seven leaves. This read 1 and was stale: LEAVES was a boolean when the
# device pair was built and measured, and 729c3a2 made it a bitmask, so 1 had
# silently narrowed this A/B to the libc leaves only -- a smaller change than the
# one the -59,328 figure and the owner's play test refer to.
override NDS_TASK37_ITCM_LEAVES := $(if $(filter %-task37-on-hwtri,$(TARGET)),7,0)
override NDS_SCENE_MIP_CACHE_LAB := 0
override NDS_FAST_WALLPAPER_AFFINE := 1
override NDS_RENDERER_BATTLE_STATIC_TEXTURE_DEFAULT := 1
override NDS_IFCOMMON_HYBRID_OAM := 0
override NDS_AUDIO_FGM_ARM7_ACK_DIAGNOSTICS := 0
override NDS_TASK16_FLOAT_COMPARE := 1
override NDS_TASK16_FLOAT_I2F := 1
override NDS_TASK16_FLOAT_ADDSUB := 1
override NDS_TASK32_DRAW_HOT_TEXT := 1
override NDS_TASK39_FX_SPRITES := 1
override NDS_TASK39_FX_FLASH := 1
endif
NDS_TASK44_DEVICE_TARGETS := \
	smash64ds-battle-playable-task44-on-hwtri \
	smash64ds-battle-playable-task44-off-hwtri
ifneq ($(filter $(TARGET),$(NDS_TASK44_DEVICE_TARGETS)),)
# Nonpublishing retail A/B pair for the Task 44 device checkpoint. The distinct
# target and build names prevent one ROM from overwriting the other. This is the
# only place Task 44 is allowed off in a profile-0 configuration.
#
# This pair is NOT tick-HUD-equivalent any more: it predates Task 37 shipping and
# both arms still build with NDS_TASK37_ITCM_LEAVES at the default 0, matching the
# ROMs already queued in builds/device-queue/task44-stage-steady-pair/. The A/B is
# still internally valid (the arms differ only in NDS_TASK44_STAGE_STEADY), but it
# measures Task 44 against a pre-Task-37 baseline. Rebuild both arms with LEAVES
# set to 7 before drawing any conclusion about Task 44 on the shipping program.
override NDS_DEV_SCENE_HARNESS := battle_playable_realtime
override NDS_DEV_LIVE_INPUT_PREVIEW := 1
override NDS_HARNESS_FAST_LOGIC := 0
override NDS_RENDERER_HW_TRIANGLES := 1
override NDS_DEBUG_HUD := 0
override NDS_RENDERER_PROFILE_LEVEL := 0
override NDS_SHIP_TELEMETRY := 0
override NDS_TICK_HUD := 1
override NDS_RENDERER_FAST_RUN_DEFAULT := 9
override NDS_NATIVE_STAGE_GENERATED_SEGMENT0_ENABLE := 1
override NDS_TASK36_HW_COMPOSE := 2
override NDS_TASK44_STAGE_STEADY := $(if $(filter %-task44-on-hwtri,$(TARGET)),1,0)
override NDS_SCENE_MIP_CACHE_LAB := 0
override NDS_FAST_WALLPAPER_AFFINE := 1
override NDS_RENDERER_BATTLE_STATIC_TEXTURE_DEFAULT := 1
override NDS_IFCOMMON_HYBRID_OAM := 0
override NDS_AUDIO_FGM_ARM7_ACK_DIAGNOSTICS := 0
override NDS_TASK16_FLOAT_COMPARE := 1
override NDS_TASK16_FLOAT_I2F := 1
override NDS_TASK16_FLOAT_ADDSUB := 1
override NDS_TASK32_DRAW_HOT_TEXT := 1
override NDS_TASK39_FX_SPRITES := 1
override NDS_TASK39_FX_FLASH := 1
endif
NDS_FREEZE_DIAGNOSTIC_TARGETS := \
	smash64ds-battle-playable-freeze-diagnostics-on-hwtri \
	smash64ds-battle-playable-freeze-diagnostics-off-hwtri
ifneq ($(filter $(TARGET),$(NDS_FREEZE_DIAGNOSTIC_TARGETS)),)
# These nonpublishing A/B targets are release-equivalent except for the
# diagnostics switch. Their distinct target and build names prevent one ROM
# from overwriting the other.
override NDS_DEV_SCENE_HARNESS := battle_playable_realtime
override NDS_DEV_LIVE_INPUT_PREVIEW := 1
override NDS_HARNESS_FAST_LOGIC := 0
override NDS_RENDERER_HW_TRIANGLES := 1
override NDS_DEBUG_HUD := 0
override NDS_RENDERER_PROFILE_LEVEL := 0
override NDS_RENDERER_FAST_RUN_DEFAULT := 9
override NDS_NATIVE_STAGE_GENERATED_SEGMENT0_ENABLE := 1
override NDS_SCENE_MIP_CACHE_LAB := 0
# Device-proven: boots to GO on melonDS and retail hardware with no OOM.
override NDS_FAST_WALLPAPER_AFFINE := 1
override NDS_RENDERER_BATTLE_STATIC_TEXTURE_DEFAULT := 1
override NDS_IFCOMMON_HYBRID_OAM := 0
override NDS_AUDIO_FGM_ARM7_ACK_DIAGNOSTICS := 0
override NDS_TASK16_FLOAT_COMPARE := 1
override NDS_TASK16_FLOAT_I2F := 1
override NDS_TASK16_FLOAT_ADDSUB := 1
override NDS_TASK32_DRAW_HOT_TEXT := 1
override NDS_TASK39_FX_SPRITES := 1
override NDS_TASK39_FX_FLASH := 1
override NDS_FREEZE_DIAGNOSTICS := $(if $(filter %-on-hwtri,$(TARGET)),1,0)
endif
ifeq ($(TARGET),smash64ds-battle-playable-freeze-diagnostics-off-hwtri)
# The off build is the canonical release payload in an isolated build tree.
override NDS_OUTPUT_BASENAME := smash64ds-battle-playable-hwtri
endif
NDS_BUG9_FLOOR_TARGETS := \
	smash64ds-bug9-rigidon-hwtri \
	smash64ds-bug9-rigidoff-hwtri
ifneq ($(filter $(TARGET),$(NDS_BUG9_FLOOR_TARGETS)),)
# BUGS.md #9 -- pause-orbit floor seam. Nonpublishing manual-test A/B pair,
# release-equivalent except that the OFF arm zeroes the Task 36 rigid binding
# mask, which routes every stage binding through the CPU-composed submit path
# that binding 29 (the middle slab, the one piece the bug does not move) already
# uses. If the seam survives the OFF arm the rigid hardware-compose path is
# exonerated and the defect is in the shared no-Z path; if it disappears the
# defect is in the rigid path. Distinct target names keep either ROM from
# overwriting the other or the published one.
#
# Both arms drop to NDS_TASK36_HW_COMPOSE=1, i.e. hardware compose WITHOUT the
# baked replay, because the replay segment set is fixed at 0/5/7 on the contract
# that every binding in those segments is rigid; replaying a dynamic binding's
# per-triangle LOAD4x4 stream would pin that geometry to the capture frame's
# camera and produce a far louder artifact than the one under test. The ON arm
# carries the same drop so the pair differs in exactly one variable -- it is the
# control for this A/B, NOT a stand-in for the published ROM.
# NDS_TASK53_REPLAY_ARENA_FIX follows to 0 because its guard below requires
# replay mode 2.
override NDS_DEV_SCENE_HARNESS := battle_playable_realtime
override NDS_DEV_LIVE_INPUT_PREVIEW := 1
override NDS_HARNESS_FAST_LOGIC := 0
override NDS_RENDERER_HW_TRIANGLES := 1
override NDS_DEBUG_HUD := 0
override NDS_RENDERER_PROFILE_LEVEL := 0
override NDS_SHIP_TELEMETRY := 0
override NDS_TICK_HUD := 0
override NDS_RENDERER_FAST_RUN_DEFAULT := 9
override NDS_NATIVE_STAGE_GENERATED_SEGMENT0_ENABLE := 1
override NDS_TASK36_HW_COMPOSE := 1
override NDS_TASK36_RIGID_BINDING_MASK := \
	$(if $(filter %-rigidoff-hwtri,$(TARGET)),0ULL,)
override NDS_R2_FIGHTER_HW_MTX := 1
# Match the published fighter path. GX compose is owner-accepted and ships on;
# stage-floor A/Bs therefore carry the same fighter renderer as the user ROM.
override NDS_R2_FIGHTER_GX_COMPOSE := 1
override NDS_R2_FIGHTER_HW_LIGHT := 1
override NDS_R2_FIGHTER_SHUFFLE_FOLD := 1
override NDS_R2_CUBIC_FIXED := 1
override NDS_R2_DELTA_PATH_ITCM := 1
override NDS_R2_ANIM_CACHE := 1
override NDS_R2_AOBJ16_PREBAKE := 1
override NDS_TASK53_REPLAY_ARENA_FIX := 0
override NDS_BATTLE_PROFILE := 1
override NDS_TASK44_STAGE_STEADY := 1
override NDS_R2_STAGE_DIRECT := 1
override NDS_R2_STAGE_DMA := 1
override NDS_R2_STAGE_VIEWPROJ := 1
override NDS_R2_STAGE_PREFLIGHT := 1
override NDS_R2_FIGHTER_MTX_DIRECT := 1
override NDS_R2_FIGHTER_RUN_MEMO := 1
override NDS_TASK37_ITCM_LEAVES := 5
override NDS_SCENE_MIP_CACHE_LAB := 0
override NDS_FAST_WALLPAPER_AFFINE := 1
override NDS_RENDERER_BATTLE_STATIC_TEXTURE_DEFAULT := 1
override NDS_IFCOMMON_HYBRID_OAM := 0
override NDS_AUDIO_FGM_ARM7_ACK_DIAGNOSTICS := 0
override NDS_TASK16_FLOAT_COMPARE := 1
override NDS_TASK16_FLOAT_I2F := 1
override NDS_TASK16_FLOAT_ADDSUB := 1
# Off in both arms, unlike the published ROM. The 8 KiB .text.hot.draw ceiling
# is sized for replay mode 2, where ndsRendererTask36ReplayRun stands in for the
# whole commit path; at mode 1 the live path lands in the section and the link
# fails. This costs speed, not pixels, and it costs both arms equally.
override NDS_TASK32_DRAW_HOT_TEXT := 0
override NDS_TASK39_FX_SPRITES := 1
override NDS_TASK39_FX_FLASH := 1
endif
ifeq ($(TARGET),smash64ds-battle-playable-coarse-hwtri)
# This is the user-testable fast-iteration ROM, not a generic build alias.
# Keep its complete realtime/live-input configuration intrinsic to the target
# so a direct make invocation cannot silently emit a non-user-facing ROM.
override NDS_DEV_SCENE_HARNESS := battle_playable_realtime
override NDS_DEV_LIVE_INPUT_PREVIEW := 1
override NDS_HARNESS_FAST_LOGIC := 0
override NDS_RENDERER_HW_TRIANGLES := 1
override NDS_DEBUG_HUD := 0
override NDS_RENDERER_PROFILE_LEVEL := 1
override NDS_RENDERER_FAST_RUN_DEFAULT := 8
endif
ifeq ($(TARGET),smash64ds-battle-playable-bgm-off-hwtri)
# BGM-stall falsifier B ROM: byte-for-byte identical to smash64ds-battle-playable-
# coarse-hwtri except NDS_BGM_FALSIFIER_OFF=1. BGM open/read/flush/play become
# no-ops while every BGM state word and counter still advances, so the rest of
# the system believes BGM is active. Run both ROMs through the same heavy-combat
# minute on device; if the 5-VBlank dips vanish under B, synchronous BGM I/O is
# the tail source. Never publish this target.
override NDS_DEV_SCENE_HARNESS := battle_playable_realtime
override NDS_DEV_LIVE_INPUT_PREVIEW := 1
override NDS_HARNESS_FAST_LOGIC := 0
override NDS_RENDERER_HW_TRIANGLES := 1
override NDS_DEBUG_HUD := 0
override NDS_RENDERER_PROFILE_LEVEL := 1
override NDS_RENDERER_FAST_RUN_DEFAULT := 8
override NDS_BGM_FALSIFIER_OFF := 1
endif
ifeq ($(TARGET),smash64ds-task10-hardware-calibration)
# Standalone lab payload: it boots from main before any game or harness setup.
override NDS_DEV_SCENE_HARNESS := normal
override NDS_DEV_LIVE_INPUT_PREVIEW := 0
override NDS_HARNESS_FAST_LOGIC := 0
override NDS_RENDERER_HW_TRIANGLES := 1
override NDS_DEBUG_HUD := 0
override NDS_RENDERER_PROFILE_LEVEL := 0
override NDS_TASK10_HARDWARE_CALIBRATION := 1
endif
ifeq ($(TARGET),smash64ds-battle-playable-forensic-hwtri)
override NDS_DEBUG_HUD := 0
override NDS_RENDERER_PROFILE_LEVEL := 2
endif
ifeq ($(TARGET),smash64ds-battle-playable-coarse-triangle-noop-hwtri)
override NDS_DEBUG_HUD := 0
override NDS_RENDERER_PROFILE_LEVEL := 1
override NDS_RENDERER_BENCHMARK_MODE := 1
endif
ifeq ($(TARGET),smash64ds-battle-playable-coarse-cpu-prep-no-gx-hwtri)
override NDS_DEBUG_HUD := 0
override NDS_RENDERER_PROFILE_LEVEL := 1
override NDS_RENDERER_BENCHMARK_MODE := 2
endif
ifeq ($(TARGET),smash64ds-battle-playable-coarse-warm-no-upload-hwtri)
override NDS_DEBUG_HUD := 0
override NDS_RENDERER_PROFILE_LEVEL := 1
override NDS_RENDERER_BENCHMARK_MODE := 4
endif
override NDS_IMPORT_BATTLESHIP_FTMAIN := 1
override NDS_IMPORT_BATTLESHIP_FTMANAGER := 1
# BUGS.md #1: graduated live. The private gate compiled mpprocess.c without
# linking it, so the shipping ROM ran the bounded port reimplementations and
# took mpProcessRun{L,R}WallCollisionAdjNew / mpProcessRunCeilEdgeAdjust from
# the weak no-op bridges in battleship_wpmanager_core.c -- no wall push-out and
# no ceiling-edge adjust at all. Both gates stay switchable; the private mode is
# still what verify-mpprocess-private-import.ps1 drives.
NDS_IMPORT_BATTLESHIP_MPPROCESS_LIVE ?= 1
NDS_IMPORT_BATTLESHIP_MPPROCESS_PRIVATE ?= 0
ifeq ($(NDS_IMPORT_BATTLESHIP_MPPROCESS_LIVE)$(NDS_IMPORT_BATTLESHIP_MPPROCESS_PRIVATE),11)
$(error NDS_IMPORT_BATTLESHIP_MPPROCESS_LIVE=1 requires NDS_IMPORT_BATTLESHIP_MPPROCESS_PRIVATE=0)
endif
override NDS_IMPORT_BATTLESHIP_FTCOMPUTER := 1
override NDS_IMPORT_BATTLESHIP_NORMAL_MOVESET := 1
NDS_IMPORT_BATTLESHIP_BATTLE_PLAYABLE ?= 1
override NDS_IMPORT_BATTLESHIP_VS_RESULTS := 1
override NDS_IMPORT_BATTLESHIP_IFCOMMON := 1
override NDS_IMPORT_BATTLESHIP_WEAPON_MANAGER := 1
override NDS_IMPORT_BATTLESHIP_MARIO_FIREBALL := 1
override NDS_IMPORT_BATTLESHIP_FOX_BLASTER := 1
override NDS_IMPORT_BATTLESHIP_EFFECT_MANAGER := 1
override NDS_IMPORT_BATTLESHIP_FOX_REFLECTOR := 1
override NDS_IMPORT_BATTLESHIP_MARIO_SPECIAL_HI := 1
override NDS_IMPORT_BATTLESHIP_MARIO_SPECIAL_LW := 1
override NDS_IMPORT_BATTLESHIP_FOX_SPECIAL_HI := 1
# BUGS.md crowd row: compile ft/ftpublic.c in place so the audience actor,
# its thresholds, cooldowns, repeat limits and defeated-voice queue are the
# source's rather than a translation.
#
# The actor costs 3,332 bytes. NDS_R2_WEAPON_POOL now retains six entries
# against a measured P1 high-water of one; the shipping soak must keep
# gNdsTaskmanGeneralHeapFreeMin above the 25,600-byte GObj latch.
NDS_IMPORT_BATTLESHIP_FT_PUBLIC ?= 1
# The eleven efManager*MakeEffect seams that spawn a particle and nothing else:
# the hit sparks, the running/landing dust, the fire grind, the sparkles, the
# flash and the set-off. ON, and the reason it is on is that routing them is
# CHEAPER than the stand-in it replaces, not merely prettier.
#
# Each source maker takes an EFStruct and calls
# gcMakeGObjSPAfter(nGCCommonKindEffect, NULL, ...) -- whose second argument is
# a run function, not a DObjDesc (objman.c:1724) -- so it builds NO DObj at all
# and draws entirely through lbParticleDrawTextures. The stand-in it displaces,
# ndsEFManagerMakeVisualEffect, takes the same EFStruct AND a gcAddDObjForGObj,
# so every stand-in is 136 bytes of general heap the match never gives back.
# Watch gNdsGCDrawsActiveMax across this flag, not just the heap low-water.
#
# It was previously behind NDS_R2_SOURCE_EFFECTS_FULL because routing the group
# "froze the ROM" -- an attribution that has since been retracted. That freeze
# was the EFDesc offsets holding symbol ADDRESSES, which sent gcSetupCustomDObjs
# walking garbage and allocating a 136-byte DObj per bogus node until syMallocSet
# gave up; the per-effect cost it was blamed on never existed. The generator
# already treats all eleven as P1 seams and has packed their scripts and
# textures (docs/optimization/NDS_PARTICLE_BANKS.generated.json, reach.p1_seams).
NDS_R2_SOURCE_EFFECTS_PARTICLE ?= 1
# The attachment-position diagnostic for the two 2026-08-12 BUGS.md rows: Fox's
# muzzle flash/beam Y, and where the burn flames land
# (artifacts/bugs/2026-08-12_r2-07-position/CONTRACT.md). READ-ONLY -- it records
# what the shipped code already computes and, beside it, what the source route
# would have produced, and it changes no gameplay value. That is deliberate:
# both rows consume gmCollisionGetFighterPartsWorldPosition, so the A-vs-B
# reading has to be taken on code that still behaves like the ROM the owner
# played, or the measurement describes the fix instead of the defect.
NDS_R2_POSITION_PROBE ?= 0
# NDS_R2_SOURCE_EFFECTS_FULL is GONE (2026-08-04). It gated the six DObj-tree
# makers -- damage slash, impact wave, catch swirl and the three random spawn
# showers -- plus the shield, rebirth halo and Fox reflector, and the reason it
# stayed 0 ("the battle hardware path does not consume source effect DL links")
# was a link-coverage gap in reloc_backend_movement.c, closed in cycles 50-59.
# The owner priced the flip at P95 +36,032 / P50 +5,440 and took it: the source
# models are the tracked default and the procedural stand-ins they displaced are
# deleted, so there is no second mode left to select. See docs/BUGS.md "GATE 6".
# Effect-instance pool depth (source EFFECT_ALLOC_NUM is 38). Bounding it bounds
# the DObj peak, which is what gcGetDObjSetNextAlloc grows out of the general
# heap and never gives back -- see include/nds/nds_effects.h for the full
# argument and for why the source's own five-free reserve keeps the forced KO
# burst spawnable. Depth minus four is the concurrent cosmetic-effect budget, so
# read gNdsEffectPoolFreeMin from a soak before changing it: pinned at 4 means
# saturated and refusing, well above 4 means the depth is bigger than the game
# needs.
#
# Routing the effects to their source makers made this pool LESS pressured, not
# more, because a source particle effect holds an EFStruct for its particle's
# lifetime while the DS stand-in it replaced also held a DObj. Measured on the
# same tree with six times the effect volume, free-min went 4 (saturated and
# refusing) -> 5 -> 7.
#
# STAYS AT TWELVE. Eight was tried on 2026-08-01 to buy heap back for that
# routing and it bought nothing: EFFECT_ALLOC_NUM is spent through
# syTaskmanMalloc, which comes out of the taskman ARENA in 4,096-byte steps,
# not out of gSYTaskmanGeneralHeap, so four fewer entries did not move
# gNdsTaskmanGeneralHeapFreeMin at all -- the whole measured gain that run was
# the weapon pool. It did move the free-min straight back to 4, the floor at
# which non-forced effects start being refused. Cut something that is actually
# on the heap instead.
NDS_R2_EFFECT_POOL ?= 12
# Pull Dream Land's blast zones to a quarter so a passive both-CPU soak produces
# KOs. Needed because the canonical one-minute match never yields one: measured
# gNdsKOBurstAttemptCount == 0 over both a 2.5-minute and a 4.5-minute run on
# 2026-08-01, which left "the KO burst freezes the game" untestable. Drives the
# ordinary bound check in ftcommondead.c, so the burst, scoring and respawn all
# run for real. NEVER ship this: these are the gameplay blast zones.
NDS_R2_KO_STRESS ?= 0
override NDS_IMPORT_BATTLESHIP_AUDIO_ASSETS := 1
override NDS_IMPORT_BATTLESHIP_AUDIO_BGM := 1
override NDS_IMPORT_BATTLESHIP_AUDIO_FGM := 1
ifeq ($(NDS_IMPORT_BATTLESHIP_MARIO_FIREBALL),1)
override NDS_IMPORT_BATTLESHIP_WEAPON_MANAGER := 1
endif
ifeq ($(NDS_IMPORT_BATTLESHIP_FOX_BLASTER),1)
override NDS_IMPORT_BATTLESHIP_WEAPON_MANAGER := 1
endif
ifeq ($(NDS_IMPORT_BATTLESHIP_FOX_REFLECTOR),1)
override NDS_IMPORT_BATTLESHIP_EFFECT_MANAGER := 1
endif
ifneq ($(NDS_FIGHTER_ANIM_AUDIT),0)
ifneq ($(NDS_RENDERER_PROFILE_LEVEL),1)
$(error NDS_FIGHTER_ANIM_AUDIT requires NDS_RENDERER_PROFILE_LEVEL=1)
endif
ifneq ($(filter -1 0 1 2 3 4 7,$(NDS_FIGHTER_ANIM_CYCLER_KIND)),$(NDS_FIGHTER_ANIM_CYCLER_KIND))
$(error NDS_FIGHTER_ANIM_CYCLER_KIND must be -1 or a landed fighter: 0 Mario, 1 Fox, 2 Donkey, 3 Samus, 4 Luigi, 7 Captain)
endif
endif
# Task 49 GX differ hooks the HW-triangle GX command funnel
# (ndsRendererTask29GXRecord), which is compiled only under HW triangles.
ifeq ($(NDS_TASK49_GX_DIFFER),1)
ifneq ($(NDS_RENDERER_HW_TRIANGLES),1)
$(error NDS_TASK49_GX_DIFFER=1 requires NDS_RENDERER_HW_TRIANGLES=1)
endif
endif
# Checked here, after every target block has applied its overrides, so a target
# that turns both on is accepted while a bare command-line NDS_TASK44_STAGE_STEADY=1
# without the Task 36 stage owner still fails loudly.
ifeq ($(NDS_TASK44_STAGE_STEADY),1)
ifeq ($(NDS_TASK36_HW_COMPOSE),0)
$(error NDS_TASK44_STAGE_STEADY=1 requires NDS_TASK36_HW_COMPOSE)
endif
endif
# Same post-override placement as the Task 44 check above: the tick-HUD target
# overrides NDS_TASK36_HW_COMPOSE := 2 in its block (line ~285), so this cross-
# check must run after the overrides or a command-line
# NDS_TASK53_REPLAY_ARENA_FIX=1 against a TASK36-forcing target is wrongly
# rejected (TASK36 still reads its ?= 0 default at the top of the file).
ifeq ($(NDS_TASK53_REPLAY_ARENA_FIX),1)
ifneq ($(NDS_TASK36_HW_COMPOSE),2)
$(error NDS_TASK53_REPLAY_ARENA_FIX=1 requires NDS_TASK36_HW_COMPOSE=2)
endif
endif
# Task 55 elides redundant COLOR/TEX_COORD words inside the Task 36 capture
# path (ndsRendererTask36ReplayCapture), so it needs both Task 36 compose==2
# and the replay path admitted. Same post-override placement as Task 53/44.
ifeq ($(NDS_TASK55_STAGE_GEOM),1)
ifneq ($(NDS_TASK36_HW_COMPOSE),2)
$(error NDS_TASK55_STAGE_GEOM=1 requires NDS_TASK36_HW_COMPOSE=2)
endif
ifeq ($(NDS_TASK53_REPLAY_ARENA_FIX),0)
$(error NDS_TASK55_STAGE_GEOM=1 requires NDS_TASK53_REPLAY_ARENA_FIX=1 (capture path must be live))
endif
endif
NDS_ENABLE_INISHIE_SOURCE_SCALE_SETUP ?= 0
NITROFS_DIR := $(PROJECT_ROOT)/$(BUILD)/nitrofs
NITRO_FILES := $(NITROFS_DIR)

include $(DEVKITARM)/ds_rules

# devkitARM emits phony dependency targets for headers. On this Windows setup,
# some devkitPro headers appear as C:devkitPro/... in .d files, which GNU make
# reads as malformed target patterns on the next incremental harness build.
NDS_BASE_ERROR_FILTER := $(ERROR_FILTER)
ERROR_FILTER = $(NDS_BASE_ERROR_FILTER) && if [ -f "$(DEPSDIR)/$*.d" ]; \
	then sed -i -e 's!\([A-Za-z]\):devkitPro!\1:/devkitPro!g' \
	"$(DEPSDIR)/$*.d"; fi

BATTLESHIP_DECOMP := decomp/BattleShip-main/decomp
BATTLESHIP_SYS := $(BATTLESHIP_DECOMP)/src/sys
BATTLESHIP_O2R := $(PROJECT_ROOT)/decomp/BattleShip-main/BattleShip_o2r
BATTLESHIP_RELOCDATA := $(PROJECT_ROOT)/decomp/BattleShip-main/decomp/assets/us/relocData

# decomp/ is immutable source of truth. Nine DS adaptations need source-level
# interposition inside imported BattleShip translation units; generate those
# into the per-build include tree instead of ever editing decomp/. New
# adaptations belong directly in src/import/src/port and are added here only
# when no include-side seam exists: the ninth (sc1pgame.c, 2026-09-05) compiles
# out the N64 title-signature check, a call through a data pointer inside
# sc1PGameFuncStart that no wrapper or macro can skip.
NDS_BATTLESHIP_IMPORT_OVERLAY := $(PROJECT_ROOT)/$(BUILD)/battleship_overlay
NDS_BATTLESHIP_IMPORT_OVERLAY_STAMP := $(NDS_BATTLESHIP_IMPORT_OVERLAY)/.stamp
NDS_BATTLESHIP_IMPORT_OVERLAY_GENERATOR := $(PROJECT_ROOT)/scripts/generate-battleship-import-overlay.ps1
NDS_BATTLESHIP_IMPORT_OVERLAY_PATCHES := $(wildcard $(PROJECT_ROOT)/scripts/import-overlays/battleship/*.patch)
NDS_BATTLESHIP_IMPORT_OVERLAY_INPUTS := \
	$(PROJECT_ROOT)/$(BATTLESHIP_DECOMP)/src/ft/ftanim.c \
	$(PROJECT_ROOT)/$(BATTLESHIP_DECOMP)/src/mn/mncommon/mnstartup.c \
	$(PROJECT_ROOT)/$(BATTLESHIP_DECOMP)/src/mv/mvopening/mvopeningroom.c \
	$(PROJECT_ROOT)/$(BATTLESHIP_DECOMP)/src/sc/scmanager.c \
	$(PROJECT_ROOT)/$(BATTLESHIP_DECOMP)/src/sc/sc1pmode/sc1pgame.c \
	$(PROJECT_ROOT)/$(BATTLESHIP_DECOMP)/src/sys/objanim.c \
	$(PROJECT_ROOT)/$(BATTLESHIP_DECOMP)/src/sys/objhelper.c \
	$(PROJECT_ROOT)/$(BATTLESHIP_DECOMP)/src/sys/objman.c \
	$(PROJECT_ROOT)/$(BATTLESHIP_DECOMP)/src/sys/taskman.c
NDS_BATTLESHIP_IMPORT_OVERLAY_OFILES := \
	battleship_ftanim.o battleship_mnstartup.o battleship_mvopeningroom.o \
	battleship_scmanager.o battleship_sys_objanim.o battleship_sys_objhelper.o \
	battleship_sys_objman.o battleship_sys_taskman.o battleship_sc1pgame_runtime.o

# BattleShip source files are compiled in place. They remain the source of truth.
SOURCES := src/nds src/nds/r2 src/port src/import $(BATTLESHIP_SYS)
# Do not add BattleShip's full include root globally: its N64 libc headers
# intentionally shadow stddef/string/etc. Compatibility headers expose the
# narrow ABI needed by each imported source slice.
INCLUDES := include $(BATTLESHIP_DECOMP)/src $(BATTLESHIP_SYS)

ARCH := -march=armv5te -mtune=arm946e-s -mthumb
# Use gnu11 (not the GCC 15 default of gnu23). The BattleShip decomp source was
# written for C89/C99 and uses `bool`/`true`/`false` as ordinary identifiers
# (e.g. a parameter named `bool` in sys/taskman.c). In C23 these are keywords,
# which breaks that source; gnu11 keeps them as ordinary identifiers.
CFLAGS := -std=gnu11 -g -Wall -Wextra -O2 -ffunction-sections -fdata-sections \
	$(ARCH) $(INCLUDE) -DARM9 -D_LANGUAGE_C -DSSB64_TARGET_NDS \
	-DREGION_US -DAVOID_UB -Wno-error=incompatible-pointer-types \
	-Wno-error=int-conversion -Wno-error=maybe-uninitialized -Wundef
CFLAGS += -include $(PROJECT_ROOT)/$(BUILD)/nds_build_config.h
ifeq ($(NDS_DEV_SCENE_HARNESS),normal)
NDS_DEV_SCENE_HARNESS_ID := 0
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_playable)
NDS_DEV_SCENE_HARNESS_ID := 163
# The scripted profile-2 diagnostic ROM is substantially larger than the
# shipped renderer; keep its scene-reserve coverage size-optimized.
CFLAGS += -Os
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_playable_realtime)
NDS_DEV_SCENE_HARNESS_ID := 163
# The canonical P1 runtime/performance anchor needs latency optimization.
CFLAGS += -O2
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_playable_match_lifecycle)
NDS_DEV_SCENE_HARNESS_ID := 163
# The fast-logic timer/Results diagnostic is a reserve proof, not a benchmark.
CFLAGS += -Os
else ifeq ($(NDS_DEV_SCENE_HARNESS),results_playable)
NDS_DEV_SCENE_HARNESS_ID := 164
# Boots straight into VS Results with a finished match seeded, so R2-07 work
# can profile and A/B the scene without emulating the whole minute first. It is
# a latency surface for the same reason battle_playable_realtime is: the numbers
# taken here are per-frame Results cost, so it must not be size-optimized.
CFLAGS += -O2
else
$(error Unknown NDS_DEV_SCENE_HARNESS "$(NDS_DEV_SCENE_HARNESS)"; use normal, a verifier harness name from scripts/lib/harness-registry.ps1, or a lab-only harness named in this block such as results_playable)
endif

# Profile 2 is an exact semantic oracle rather than a latency benchmark.  Its
# instrumentation and paired native/generic paths are large enough that an O2
# realtime build can crowd the BattleShip scene arena below its verified
# 0x130000-byte floor.  Keep only the forensic target size-optimized; profile
# 0/1 battle-playable ROMs retain the O2 performance contract.
ifeq ($(TARGET),smash64ds-battle-playable-forensic-hwtri)
CFLAGS += -Os
endif

CXXFLAGS := $(CFLAGS) -fno-rtti -fno-exceptions
ASFLAGS := -g $(ARCH)
NDS_HOT_TEXT_SPECS := $(PROJECT_ROOT)/linker/ds9_hot_text.specs
NDS_HOT_TEXT_LINKER_SCRIPT := $(PROJECT_ROOT)/linker/nds_hot_text.ld
NDS_TASK32_DRAW_HOT_FRAGMENT := $(PROJECT_ROOT)/$(BUILD)/nds_task32_draw_hot.inc
NDS_TASK39_HIT_SPARKS_INC := $(PROJECT_ROOT)/src/nds/generated/task39_hit_sparks.generated.inc
NDS_TASK39_HIT_SPARKS_ASSET := $(PROJECT_ROOT)/assets/effects/task39_hit_sparks.rgb5a1.bin
NDS_PARTICLE_BANKS_INC := $(PROJECT_ROOT)/src/nds/generated/nds_particle_banks.generated.inc
NDS_PARTICLE_BANKS_HEADER := $(PROJECT_ROOT)/include/nds/generated/nds_particle_banks.generated.h
# Slice 32. The baked fighter-animation bank: 145,873 20-byte write records plus
# 71,500 u16 control entries, replacing the figatree interpretation at runtime.
# Generated from the 297 AObj16 files in $(BATTLESHIP_O2R)/reloc_animations,
# which the build otherwise copies into nitrofs verbatim.
#
# DEFAULT OFF, and it must stay that way until a reader exists. The blob is
# ~3.1 MB and the particle comment below records what that costs when nothing
# opens it. Turning this on without the runtime bind adds ROM and changes
# nothing else.
NDS_R2_FTANIM_DENSE ?= 0
NDS_FTANIM_DENSE_ASSET := $(PROJECT_ROOT)/assets/animation/ftanim_dense_bank.bin
NDS_FTANIM_DENSE_SOURCES := \
	$(wildcard $(BATTLESHIP_O2R)/reloc_animations/FTMarioAnim*) \
	$(wildcard $(BATTLESHIP_O2R)/reloc_animations/FTFoxAnim*)
NDS_FTANIM_STREAM_ASSET := $(PROJECT_ROOT)/assets/animation/ftanim_stream_pack.bin
NDS_FTANIM_STREAM_SOURCES := \
	$(wildcard $(BATTLESHIP_O2R)/reloc_animations/FTMarioAnim*) \
	$(wildcard $(BATTLESHIP_O2R)/reloc_animations/FTFoxAnim*) \
	$(wildcard $(BATTLESHIP_O2R)/reloc_animations/FTDonkeyAnim*) \
	$(wildcard $(BATTLESHIP_O2R)/reloc_animations/FTSamusAnim*) \
	$(wildcard $(BATTLESHIP_O2R)/reloc_animations/FTLuigiAnim*) \
	$(wildcard $(BATTLESHIP_O2R)/reloc_animations/FTCaptainAnim*)

# Slice 1 phase 5's resident figatree pack. ONE fighter, because the taskman
# arena holds 287,904 (Fox) or 271,728 (Mario) but not the ~559,632 both need.
# Fox is the resident: it is the autonomous level-3 CPU on the Boundary arm, so
# its action changes dominate the acquisitions there, and it is the larger blob,
# which makes it the binding fit test.
# `--items-off` drops the 38 clips proven unreachable from the linked battle ELF.
#
# STAGED INTO NitroFS, NOT `.incbin`. The first build linked it into .rodata and
# +288,992 B of ARM9 image was MEASURED to push gNdsTaskmanArenaChosenSize
# 0x150000 -> 0x140000 with 16 alloc failures: the arena is one calloc from the
# same libnds heap the static image bounds, so image growth and arena size come
# out of the same bytes. check-boot-headroom.ps1 CANNOT see that -- it reported
# 66,784 B of proven headroom on that arm. As a NitroFS payload the blob costs
# the ARM9 image nothing and the runtime streams it into the arena at setup, in
# place of the 262,144 B raw-file cache it replaces.
NDS_BATTLEPACK_DIR := $(PROJECT_ROOT)/assets/animation
NDS_BATTLEPACK_BLOB := $(NDS_BATTLEPACK_DIR)/battlepack_fox.bin

# Task 3 stage 3 -- the AOT typed track rows the dense fighter-animation runtime
# steps (`src/nds/nds_ftanim_track.c`).
#
# THIS ONE *IS* .rodata, and the reason is the opposite of the battlepack's. The
# rows do not REPLACE the o2r payload here: both are resident at once, which is
# what makes the route bit a SAME-BINARY A/B with no placement floor and what
# lets the stage-4 oracle compare the dense stepper against the generic parser
# on the same joint of the same frame. That costs image, so it is bounded by
# measurement rather than by the boot-headroom ladder: static growth comes out
# of the same bytes as the taskman arena, and `gNdsTaskmanGeneralHeapFreeMin`
# read 53,136 on the c193 gate arm against the mandated 32,768 B reserve.
# 12,288 B of rows plus ~3,552 B of cursor blocks leaves ~37,000.
#
# `--max-bytes` selects in ASCENDING ASSET ID, which is ascending motion index,
# so a tight budget buys the common motions rather than a coverage lottery. The
# admitted clip count and the runtime's own bind/step counters are the coverage
# figure; nothing here estimates it.
NDS_R2_FTANIM_TRACK ?= 0
NDS_R2_FTANIM_TRACK_DISPATCH ?= 1
NDS_R2_FTANIM_TRACK_ORACLE ?= 0

# P2-2p6 (owner ruling 2026-08-23: "do both"). The fighter pose engine,
# `src/nds/nds_ft_pose.c`: the figatree script machine and the Q12 evaluator
# over compact per-fighter tracks in place of the per-tick AObj-list parse and
# play, and -- NDS_FT_POSE_HOLD -- body-joint evaluation on the last source tick
# of each presented frame only (TransN/XRotN/YRotN and the attach tick stay
# 60 Hz). NDS_FT_POSE_ORACLE is the lab proof: the engine runs on shadow joints
# beside the generic path and compares every pose/clock field bit for bit
# (`gNdsFtPoseOracleMismatches` must read 0 over a whole match).
#
# DEFAULT-ON 2026-08-23: the oracle read 0 mismatches over 189,251 joint
# compares with the hold and the Q12 clock (artifacts/verification/
# 2026-08-23_ft-pose-oracle.txt), and the four-CPU stress banked
# WORK-H P50/P95 1,264,512/1,836,800 -> 1,244,608/1,777,408 across the
# engine+hold+Q12 series (board row P2-2p6). ORACLE stays a lab flag.
NDS_FT_POSE ?= 1
NDS_FT_POSE_HOLD ?= 1
NDS_FT_POSE_ORACLE ?= 0

# Fox Blaster's shared beam/flash/collision bore line (`nds_effects.h`).
# SETTLED BY THE OWNER 2026-08-15: "bore should be zero, no offset, not needed
# anymore". The 84 was eye-tuned 2026-08-14 to compensate a gun-joint pose the
# segment-phase parser defect left a frame stale; `64c41c361a7` repaired the
# pose, so the compensation is gone with it. This variable stays so a trial
# value costs no source edit.
NDS_FOX_BLASTER_BORE_OFFSET_Y ?= 0
NDS_FTANIM_TRACK_MAX_BYTES ?= 12288
NDS_FTANIM_TRACK_HEADER := $(PROJECT_ROOT)/include/nds/generated/nds_ftanim_track_pack.generated.h
ifeq ($(NDS_R2_FTANIM_TRACK),1)
NDS_FTANIM_TRACK_PREREQ := $(NDS_FTANIM_TRACK_HEADER)
else
NDS_FTANIM_TRACK_PREREQ :=
endif

NDS_PARTICLE_TEXTURE_ASSET := $(PROJECT_ROOT)/assets/particles/efcommon_particle_textures.ds.bin
NDS_WHISPY_NATIVE_ASSET := $(PROJECT_ROOT)/assets/particles/grpupupu_whispy_native.ds.bin
# The draw path's own payload: the admitted textures as RGB555+A1, which is the
# format the renderer's texture cache uploads. Separate from the file above
# because that one is per-texture DS formats with palettes and the cache has no
# palette slot in its key.
NDS_PARTICLE_QUAD_ASSET := $(PROJECT_ROOT)/assets/particles/efcommon_particle_quads.a5i3.bin
# The battle static-texture metadata include and its NitroFS payload. Both are
# build products of one generator invocation, and until 2026-08-05 NOTHING in
# this Makefile knew how to produce either: only build.ps1 ran the generator, so
# an incremental `make` linked whatever copy happened to be on disk. That is how
# the .inc stayed at its Aug-3 content across three builds while its generator
# and the census it imports had both moved -- harmless only because that day's
# delta was a header comment. The next such delta would be data, and it would
# reach a measured ROM with no error anywhere (see CLAUDE.md: this class "races
# into a subtly wrong binary rather than an error").
NDS_BATTLE_STATIC_TEXTURE_INC := $(PROJECT_ROOT)/src/nds/generated/battle_playable_static_textures.generated.inc
NDS_BATTLE_STATIC_TEXTURE_ASSET := $(PROJECT_ROOT)/assets/renderer/battle_playable_static_textures.rgb5a1.bin
# P2-1c. Same shape and the same reason: the manifest is compiled in and the
# texel/intensity payload ships in NitroFS, both written by one generator so a
# stale pair cannot link.
NDS_MN_UI_KIT_INC := $(PROJECT_ROOT)/src/nds/generated/mn_ui_kit.generated.inc
NDS_MN_UI_KIT_ASSET := $(PROJECT_ROOT)/assets/menus/mn_ui_kit.bin
# P2-2/P2-3. The lower battle HUD is AOT-only: source IFCommon digits and each
# admitted fighter's portrait/stock icon are baked straight into tiled 4bpp
# sub-OBJ cells.
# There is deliberately no NitroFS payload or runtime decoder for this asset.
NDS_BATTLE_HUD_INC := $(PROJECT_ROOT)/src/nds/generated/battle_hud.generated.inc
# Match-entry presentation.  Mario's pipe and Fox's Arwing keep BattleShip's
# live DObj animation but consume an AOT DS-native mesh/texture packet.  Unlike
# a review-only manifest this include is compiled directly by nds_renderer.c,
# so its generator belongs on the normal dependency graph: stale generated
# geometry must never survive an O2R or decoder change into a measured ROM.
NDS_ENTRY_EFFECT_INC := $(PROJECT_ROOT)/src/nds/nds_entry_effects.generated.inc
# P2-1h. The backdrop art is a SECOND payload from the same generator: the OBJ
# pack is read and hashed on every kit entry, so a title screen living in it
# would cost the character select bytes it never draws.
NDS_MN_UI_SURFACE_ASSET := $(PROJECT_ROOT)/assets/menus/mn_surfaces.bin
# P2-1k (d). The title pop animation's pose table. A SECOND generator writes it
# -- `decode_mn_title_anim.py` owns the animation, transcribes objanim.c and
# carries the oracles -- but it reads the kit generator as a module, so the two
# agree on every raster size by construction rather than by convention. It
# depends on the kit's source as well as its own for exactly that reason.
NDS_MN_TITLE_ANIM_INC := \
	$(PROJECT_ROOT)/src/nds/generated/mn_title_anim.generated.inc
LDFLAGS := -specs=$(NDS_HOT_TEXT_SPECS) -g $(ARCH) \
	-Wl,-Map,$(notdir $*.map),--gc-sections \
	-Wl,-T,$(NDS_HOT_TEXT_LINKER_SCRIPT)
ifeq ($(NDS_TASK16_FLOAT_COMPARE),1)
LDFLAGS += -Wl,--undefined=__nds_task9_libgcc_fcmpeq_golden \
	-Wl,--undefined=__nds_task16_libgcc_fcmpun_golden
endif
ifeq ($(NDS_TASK16_FLOAT_I2F),1)
LDFLAGS += -Wl,--undefined=__nds_task16_libgcc_i2f_golden
endif
ifeq ($(NDS_TASK16_FLOAT_ADDSUB),1)
LDFLAGS += -Wl,--undefined=__nds_task16_libgcc_fadd_golden
endif

NDS_TASK9_FLOAT_WRAP_SYMBOLS := \
	__aeabi_fadd __aeabi_fsub __aeabi_frsub __aeabi_fmul __aeabi_fdiv \
	__aeabi_fcmpeq __aeabi_fcmplt __aeabi_fcmple __aeabi_fcmpge \
	__aeabi_fcmpgt __aeabi_fcmpun __aeabi_f2iz __aeabi_f2uiz \
	__aeabi_i2f __aeabi_ui2f __aeabi_l2f __aeabi_ul2f __aeabi_f2d \
	__aeabi_d2f __aeabi_dadd __aeabi_dsub __aeabi_drsub __aeabi_dmul \
	__aeabi_ddiv __aeabi_dcmpeq __aeabi_dcmplt __aeabi_dcmple \
	__aeabi_dcmpge __aeabi_dcmpgt __aeabi_dcmpun __aeabi_d2iz \
	__aeabi_i2d __aeabi_ui2d __aeabi_l2d __aeabi_ul2d
ifeq ($(NDS_TASK9_FLOAT_CENSUS),1)
LDFLAGS += $(foreach symbol,$(NDS_TASK9_FLOAT_WRAP_SYMBOLS),-Wl,--wrap=$(symbol))
endif

# Task 9 Phase 1 uses the exact objects from the selected Thumb multilib. The
# objects contain ARM-state implementations; only their section placement is
# changed. Phase 2 renames the stock fcmpeq wrapper as an in-ROM golden and
# links one integer-only leaf replacement ahead of libgcc. Keep the list narrow
# so the renderer's existing ITCM ownership is never displaced by low-frequency
# double-precision helpers.
NDS_TASK9_FLOAT_LIBGCC_SHA256 := \
	c755adc33eca252260360327904591b8462cce5c25e48b0e881ac0b295953f48
NDS_TASK9_FLOAT_ITCM_MEMBERS := \
	_arm_addsubsf3.o _arm_muldivsf3.o _arm_cmpsf2.o \
	_arm_unordsf2.o _arm_fixsfsi.o _arm_fixunssfsi.o
# The suffix is load-bearing, not cosmetic. `linker/nds_hot_text.ld:113` reads
#   *.itcm.* (.text .stub .text.* .gnu.linkonce.t.*)
# so a file NAMED `*.itcm.*` has its .text placed in ITCM whatever its section
# is called. Dropping --rename-section alone therefore frees nothing; the member
# has to leave the filename pattern as well. `.mainram.o` matches neither that
# rule nor the `*.32.o` one beside it.
NDS_TASK9_FLOAT_ITCM_OFILES := \
	$(foreach member,$(NDS_TASK9_FLOAT_ITCM_MEMBERS), \
		$(basename $(member))$(if $(filter $(member),$(NDS_TASK9_FLOAT_MAIN_MEMBERS)),.mainram.o,.itcm.o))

# Task 37 measured leaves. The census split every profiled instruction's stall
# cycles by whether the instruction touched data, because only the non-memory
# half is recoverable by moving code. These five sit in .main today and carry
# 7,387,317 non-memory stall cycles across 906 bytes -- the densest set that
# fits ITCM's 1,060 free bytes with no eviction of any current resident.
#
#   memset       140 B   2,391,465    memcpy       170 B   1,173,373
#   memcmp        70 B   1,249,372    __ieee754_sqrtf 236 B 1,054,412
#   (port side: ndsFTParamsInvalidateFighterParts, TextureSourceBytes, PolyFmt)
#
# Same mechanism as Task 9: extract from a SHA-verified private copy of the
# archive and rename the section. The code is byte-identical; only where it
# lives changes, so the A/B measures placement and nothing else.
NDS_TASK37_LIBC_SHA256 := \
	01424211f6f671e0b07b52fb72086f14e18000fca089e9ecfe45aa77b36873e2
NDS_TASK37_LIBM_SHA256 := \
	b437e8747f520c891d2784df015ab6f8cd30bb91cd02430f03657c20027d6685
NDS_TASK37_LIBC_MEMBERS := libc_a-memset.o libc_a-memcpy-stub.o libc_a-memcmp.o
NDS_TASK37_LIBM_MEMBERS := libm_a-ef_sqrt.o
NDS_TASK37_ITCM_OFILES := \
	$(if $(filter 1,$(NDS_TASK37_ITCM_LIBC)),$(addsuffix .itcm.o,$(basename $(NDS_TASK37_LIBC_MEMBERS)))) \
	$(if $(filter 1,$(NDS_TASK37_ITCM_LIBM)),$(addsuffix .itcm.o,$(basename $(NDS_TASK37_LIBM_MEMBERS))))

LIBS := -lfat -lfilesystem -lnds9 -lm
LIBDIRS := $(LIBNDS)

ifneq ($(abspath $(PROJECT_ROOT)/$(BUILD)),$(abspath $(CURDIR)))

export OUTPUT := $(NDS_OUTPUT_ROOT)/$(NDS_OUTPUT_BASENAME)
export VPATH := $(foreach dir,$(SOURCES),$(CURDIR)/$(dir))
export DEPSDIR := $(CURDIR)/$(BUILD)

# Keep this list explicit. Adding an original subsystem is a deliberate port step.
NDS_PRIVATE_CHECK_CFILES :=
NDS_MPPROCESS_SOURCE_CFILES := battleship_mpprocess_edge_support.c \
	battleship_mpprocess.c
CFILES := main.c nds_platform.c nds_ifcommon_oam.c nds_task39_effect_census.c nds_reloc_assets.c nds_audio_assets.c nds_audio_bgm.c nds_audio_fgm.c nds_renderer.c battle_playable_static_textures.c nds_battlepack_anim.c port_probe.c n64_stubs.c coroutine.c \
	libultra_os.c os_selftest.c boot_stubs.c battleship_sys_main.c \
	scheduler_backend.c controller_backend.c battleship_sys_scheduler.c \
	battleship_sys_controller.c battleship_sys_maindevice.c \
	battleship_sys_video.c battleship_sys_malloc.c \
	battleship_sys_framebuffer.c battleship_sys_zbuffer.c video_bootstrap.c \
	battleship_sys_sintable.c battleship_sys_matrix.c \
	battleship_libultra_gu_normalize.c battleship_libultra_gu_mtxcatf.c \
	battleship_scmanager.c battleship_mnstartup.c scene_backend.c scene_harness.c nds_match_config.c nds_scene_manager.c utils.c vector.c \
	battleship_scsubsyscontroller.c \
	battleship_sys_taskman.c battleship_sys_objman.c \
	battleship_sys_objhelper.c battleship_sys_objanim.c \
	battleship_ftdisplaylights.c battleship_ftdisplaymain.c \
	battleship_sys_interp.c battleship_mvopeningroom.c \
	battleship_mvopeningportraits.c battleship_mvopeningmario.c \
	battleship_mvopeningdonkey.c battleship_mvopeninglink.c \
	battleship_mvopeningsamus.c battleship_mvopeningyoshi.c \
	battleship_mvopeningkirby.c battleship_mvopeningfox.c \
	battleship_mvopeningpikachu.c battleship_mntitlefiles.c \
	battleship_mntitle.c battleship_mnvsmode.c \
	battleship_mnplayersvs.c battleship_mnmaps.c \
	battleship_gmcommon.c battleship_gmcollision.c battleship_scvsbattle.c \
	battleship_grpupupu_ground.c \
	battleship_grinishie_scale.c \
	battleship_ftcommon_wait.c \
	battleship_ftcommon_walk.c battleship_ftcommon_turn.c \
	battleship_ftcommon_turnrun.c \
	battleship_ftcommon_dash.c \
battleship_ftcommon_attack1.c \
battleship_ftcommon_attack100.c \
battleship_ftcommon_attackdash.c \
battleship_ftcommon_attackair.c \
battleship_mariofox_mainmotion.c \
battleship_ftcommon_appeal.c \
battleship_ftcommon_catch.c \
battleship_ftcommon_guard.c \
battleship_ftcommon_escape.c \
battleship_ftcommon_run.c battleship_ftcommon_runbrake.c \
	battleship_ftcommon_kneebend.c battleship_ftcommon_jump.c \
	battleship_ftcommon_pass.c \
	battleship_ftcommon_fall.c battleship_ftcommon_ottotto.c \
	battleship_ftcommon_landing.c battleship_ftcommon_stopceil.c \
	battleship_ftcommon_cliffcatchwait.c \
	battleship_ftcommon_cliffclimb.c \
	battleship_ftcommon_cliffattack.c \
	battleship_ftcommon_cliffescape.c \
	battleship_ftcommon_walldamage.c \
	battleship_ftcommon_rebound.c \
	battleship_ftcommon_twister.c \
	battleship_ftcommon_furasleep.c \
	battleship_ftcommon_damage.c \
	battleship_ftcommon_damagefall.c \
	battleship_ftcommon_passive.c \
	battleship_ftcommon_passivestand.c \
	battleship_ftcommon_downwaitbounce.c \
	battleship_ftcommon_downattack.c \
	battleship_ftcommon_downforwardback.c \
	battleship_ftcommon_downstand.c
# Runtime 2 battle path (R2-01). Added only when the flag is on, so the default
# arm's link input set is unchanged rather than merely equivalent -- an empty
# translation unit still enters the link and this project has measured
# re-addressing collateral from far less (Tasks 87-89/94/95).
ifeq ($(NDS_R2_PATH),1)
CFILES += nds_r2_battle.c
endif
# P2-4 first stage: Yoshi's Island ground logic (decomp gryoster.c import).
# Flag off, the TU is not linked at all, so the default arm is unchanged.
ifeq ($(NDS_P2_STAGE_YOSTER),1)
CFILES += battleship_gryoster_ground.c
endif
# P2-4 stage 2: Peach's Castle ground logic (decomp grcastle.c import).
# Flag off, the TU is not linked at all, so the default arm is unchanged.
ifeq ($(NDS_P2_STAGE_CASTLE),1)
CFILES += battleship_grcastle_ground.c
endif
# P2-4 stage 3: Congo Jungle ground logic (decomp grjungle.c import).
ifeq ($(NDS_P2_STAGE_JUNGLE),1)
CFILES += battleship_grjungle_ground.c
endif
# P2-4 stage 4: Planet Zebes ground logic (decomp grzebes.c import).
ifeq ($(NDS_P2_STAGE_ZEBES),1)
CFILES += battleship_grzebes_ground.c
endif
# P2-4 stage 5: Hyrule Castle ground logic (decomp grhyrule.c import). It
# adds no reloc payload: its map declares only StageCastle and
# ExternDataBank113, both already staged and rowed for other reasons.
ifeq ($(NDS_P2_STAGE_HYRULE),1)
CFILES += battleship_grhyrule_ground.c
endif
# P2-4 stage 6: Saffron City ground logic (decomp gryamabuki.c import).
ifeq ($(NDS_P2_STAGE_YAMABUKI),1)
CFILES += battleship_gryamabuki_ground.c
endif
# P2-4 stage 7: Mushroom Kingdom ground logic (decomp grinishie.c import).
# MUTUALLY EXCLUSIVE with the older scale-only import: that file defines
# grInishieMakeScale on its own, and this one brings in the whole stage
# including it, so linking both is a duplicate definition.
ifeq ($(NDS_P2_STAGE_INISHIE),1)
ifeq ($(NDS_ENABLE_INISHIE_SOURCE_SCALE_SETUP),1)
$(error NDS_P2_STAGE_INISHIE and NDS_ENABLE_INISHIE_SOURCE_SCALE_SETUP both define grInishieMakeScale; pick one)
endif
CFILES += battleship_grinishie_ground.c
endif
# P2-4 stage 8: Sector Z ground logic (decomp grsector.c import).
ifeq ($(NDS_P2_STAGE_SECTOR),1)
CFILES += battleship_grsector_ground.c
endif
ifeq ($(NDS_R2_FIXED_SQRT),1)
CFILES += nds_r2_sqrtf.c
# The ARM-state arm of the sqrtf route. Lab only: at NDS_R2_HWMATH_ROUTE 0 it is
# not in CFILES at all, so a published ROM's link input set is unchanged rather
# than merely equivalent.
ifeq ($(NDS_R2_HWMATH_ROUTE),1)
CFILES += nds_r2_sqrtf_arm.c
endif
endif
ifeq ($(NDS_R2_COLLISION_FIXED),1)
CFILES += nds_r2_collision_fixed.c nds_r2_collision_ring.c
endif
ifeq ($(NDS_R2_SIM_MAC_SHADOW),1)
CFILES += nds_r2_sim_mac_fixed.c
endif
ifeq ($(NDS_R2_HWMATH_BENCH),1)
CFILES += nds_r2_hwmath_bench.c
endif
# Conditional so a published ROM stays byte-identical: at flag 0 the TU is not
# linked at all, rather than linked as ten `used` counters nothing writes.
ifeq ($(NDS_R2_FTANIM_TRACK),1)
CFILES += nds_ftanim_track.c
endif
CFILES += nds_ft_pose.c
ifeq ($(NDS_IMPORT_BATTLESHIP_NORMAL_MOVESET),1)
CFILES += battleship_ftcommon_normal_moveset.c
# ...and the real bodies for two of that file's weak stubs.
# ftParamProcPauseEffect and ftParamProcResumeEffect were no-ops there, so
# a smash's proc_lagstart/lagend and the Captain and Kirby specials never
# paused their attached effects during hitstop. Transcribed rather than
# textually included: the decomp region also defines ftParamProcStopEffect,
# which already has a strong port definition and would collide.
CFILES += battleship_ftparam_effectprocs.c
endif
# P2-6, compiled only when the campaign flag is on.
ifeq ($(NDS_P2_1P_GAME),1)
# P2-6 step 5 (2026-09-04): Break the Targets / Board the Platforms, source imports.
CFILES += battleship_sc1pbonusstage.c battleship_sc1pbonusstagefiles.c
# P2-6 step 1 (2026-09-04): the campaign driver and the runtime half of sc1pgame.c.
CFILES += battleship_sc1pmanager.c battleship_sc1pgame_runtime.c
# P2-6 step 2 (2026-09-05): the stage-clear tally scene, whole TU (it owns the
# 58-row bonus table; the transcribed tables TU is gone).
CFILES += battleship_sc1pstageclear.c
# P2-6 step 8 (2026-09-05): the stage intro and the challenger screen, source imports.
CFILES += battleship_sc1pintro.c battleship_sc1pchallenger.c
# P2-7 item 5 (2026-09-04): Options, Backup Clear and Sound Test, source imports
# (unreachable from the native shell until P2-7 item 9 wires them).
CFILES += battleship_mnoption.c battleship_mnbackupclear.c battleship_mnsoundtest.c
# P2-7 items 2, 3 and 4 (2026-09-05): the unlock message, Training (scene + its
# character select) and the DATA menus, source imports; same reachability note.
CFILES += battleship_mnmessage.c battleship_sc1ptrainingmode.c battleship_mntraining.c
CFILES += battleship_mndata.c battleship_mnvsrecord.c battleship_mncharacters.c
# P2-6 step 8 (2026-09-05): the 1P Game menus, source imports (mnmessage.c is above).
CFILES += battleship_mn1pmode.c battleship_mn1pcontinue.c battleship_mnplayers1pgame.c
CFILES += battleship_mnplayers1pbonus.c battleship_mncongra.c
# P2-6 step 8 tail (2026-09-05): the ending movie and the credits, source imports.
CFILES += battleship_mvending.c battleship_scstaffroll.c
# P2-7 items 7 and 8 (2026-09-05): the attract demo and How to Play, source imports
# with their file-setup companions.
CFILES += battleship_scautodemo.c battleship_scautodemofiles.c
CFILES += battleship_scexplain.c battleship_scexplainfiles.c
# P2-6 step 7 (2026-09-05): Master Hand -- the boss scene, the finger-gun bullet and
# the boss status files, source imports.
CFILES += battleship_sc1pgameboss.c battleship_wpbossbullet.c
CFILES += battleship_ftboss.c battleship_ftboss_status_1.c battleship_ftboss_status_2.c
CFILES += battleship_ftboss_status_3.c battleship_ftboss_status_4.c
# P2-6 step 6 (2026-09-05): Race to the Finish, the scrolling course logic (grbonus3.c).
CFILES += battleship_grbonus3.c
endif
CFILES += battleship_ftchar_data_slots.c battleship_scsubsysdata_ft.c \
	battleship_ftdata.c reloc_backend_ftdata_stubs.c \
	reloc_backend_ftdata_symbols.c
CFILES += battleship_ftanim.c battleship_ftanimend.c battleship_ftkey.c
ifeq ($(NDS_IMPORT_BATTLESHIP_FTMANAGER),1)
CFILES += battleship_ftmanager.c
ifeq ($(NDS_P2_DONKEY),1)
# P2-3 DK is the first non-Mario archetype.  Compile BattleShip's own special
# and cargo state machines as one port TU rather than re-implementing their
# update/interrupt/physics/map ordering in DS glue.
CFILES += battleship_donkey.c
endif
ifneq ($(filter 1,$(NDS_P2_DONKEY) $(NDS_P2_LINK)),)
# DK first needed the items-off common throw subset; Link graduates the same TU
# to BattleShip's full shared item-throw runtime. Keep one owner TU when both
# fighters are enabled.
CFILES += battleship_ftcommon_itemthrow.c
endif
ifeq ($(NDS_P2_CAPTAIN),1)
# P2-3f5. Falcon Punch / Falcon Kick / Falcon Dive as one port TU, plus the
# VICTIM side of Falcon Dive -- which is an ftcommon TU, not an ftcaptain one,
# because nFTCommonStatusCaptureCaptain is a status any fighter can end up in.
CFILES += battleship_captain.c battleship_ftcommon_capturecaptain.c
endif
ifeq ($(NDS_P2_SAMUS),1)
# BattleShip owns Charge Shot storage/cancel/release, Screw Attack and Bomb
# behavior; the companion TU owns the source Charge Shot/Bomb weapons.
CFILES += battleship_samus.c battleship_samus_weapons.c
endif
ifeq ($(NDS_P2_LINK),1)
# BattleShip owns Link's rapid jab, entry pair, Boomerang/return state machine,
# Spin Attack and Bomb-pull/throw transitions. The shared weapon manager owns
# Boomerang and grounded Spin Attack; LinkBomb graduates through the item owner.
CFILES += battleship_link.c battleship_link_weapons.c battleship_link_bomb.c
endif
ifeq ($(NDS_P2_ITEM_CORE),1)
# The shared item owner, compiled for whichever fighters bring an item article.
CFILES += battleship_item_link_core.c
# P2-5i1 GBumper (stage bumper, kind 23): same gate, no new asset work --
# its art is the already-resident shared ITCommonData (reloc 0xfb).
CFILES += battleship_item_gbumper.c
# The item map/physics helper the common items need. Only
# itMainSetGroundAllowPickup lives there: the two itmap.c helpers beside it in
# source are already defined by battleship_item_link_core.c's own itmap.c
# import, and defining them twice is a link error.
CFILES += battleship_item_map_core.c
# P2-5 common items, all drawing from the resident ITCommonData. Touch-consumed
# first, then the swing-and-throw three, then the Hammer's fighter-state seam.
CFILES += battleship_item_tomato.c battleship_item_heart.c
CFILES += battleship_item_star.c battleship_item_hammer.c
CFILES += battleship_item_sword.c battleship_item_bat.c battleship_item_harisen.c
# The four containers. They roll a payload and spawn it when broken, which is
# the machinery the Poke Ball reuses, so they come before the monsters.
CFILES += battleship_item_box.c battleship_item_taru.c
CFILES += battleship_item_capsule.c battleship_item_egg.c
# Ammo shooters and self-actors.
CFILES += battleship_item_starrod.c battleship_item_lgun.c
CFILES += battleship_item_fflower.c battleship_item_msbomb.c
CFILES += battleship_item_bombhei.c battleship_item_nbumper.c
CFILES += battleship_item_gshell.c battleship_item_rshell.c
# The Poke Ball closes the common twenty. It rolls through itMainMakeMonster
# (battleship_item_map_core.c); until the Pokemon kinds land, that roll ends at
# itManagerMakeItemKind's bound and returns NULL, which the source's own
# monster-bus NULL check already covers -- the ball opens and nothing comes out.
CFILES += battleship_item_mball.c
# The Pokemon it rolls. Each is one kind's TU against the shared monster bus;
# they land in batches, and until all thirteen are here the roll can select a
# kind with no maker, which itManagerMakeItemKind refuses by returning NULL.
CFILES += battleship_item_nyars.c battleship_item_dogas.c
CFILES += battleship_item_kabigon.c battleship_item_tosakinto.c
CFILES += battleship_item_mew.c
CFILES += battleship_item_iwark.c battleship_item_lizardon.c
CFILES += battleship_item_spear.c battleship_item_kamex.c
CFILES += battleship_item_mlucky.c battleship_item_starmie.c
CFILES += battleship_item_sawamura.c battleship_item_pippi.c
# Stage-spawned kinds: Mushroom Kingdom's POW block and Piranha, the bonus
# target, and Saffron City's five Pokemon. These are placed by their stage
# rather than by the item spawn law, but they are ITStructs and go through
# the same maker table.
# battleship_item_target.c is a BONUS STAGE item: it calls
# sc1PBonusStageUpdateTargetCount and its descriptor names
# gSC1PBonusStageItemFile, both provided by battleship_sc1pbonusstage.c
# behind NDS_P2_1P_GAME since 2026-09-04 (P2-6 step 5), so it rides that flag.
CFILES += battleship_item_pakkun.c
ifeq ($(NDS_P2_1P_GAME),1)
CFILES += battleship_item_target.c
endif
# Six of the ten itground/ kinds reach back into their own stage's ground
# code, so they ride that stage's flag rather than the item core -- the core
# is DERIVED from the landed fighters too, so a Link build with items on and
# these stages off failed to link on exactly these six. Undefined symbols
# scanned from the objects, not guessed:
#   powerblock  -> grInishiePowerBlockSetWait/SetDamage
#   glucky, hitokage, marumine, porygon, fushigibana
#               -> grYamabukiGateSetClosedWait/ClearMonsterGObj,
#                  dGRYamabukiMonsterAttackKind
# All five Yamabuki kinds are the Saffron City GATE monsters (decomp puts
# them in it/itground/, not it/itmonster/), so gating them costs no Poke Ball
# content: the thirteen Poke Ball Pokemon are the itmonster/ set above and
# reference no stage symbol.
ifeq ($(NDS_P2_STAGE_INISHIE),1)
CFILES += battleship_item_powerblock.c
endif
ifeq ($(NDS_P2_STAGE_YAMABUKI),1)
CFILES += battleship_item_glucky.c battleship_item_marumine.c
CFILES += battleship_item_porygon.c
CFILES += battleship_item_hitokage.c battleship_item_fushigibana.c
endif
# The FIGHTER half of pickup. Without it every item above is scenery: nothing
# called itMainSetFighterHold, and every ground attack's "is there an item
# here?" check was a shim returning FALSE.
CFILES += battleship_ftcommon_get.c
# ...and the fighter half of USING one: the Ray Gun's ammo, the Fire
# Flower's flame and the Star Rod's star were all ported and all
# unreachable behind empty SetStatus shims and weak proc stubs.
CFILES += battleship_ftcommon_itemuse.c
# ...and the fighter half of HOLDING the Hammer: pickup started its timer and
# music, but eight status callbacks were weak stubs and four more seams were
# no-op shims, so the fighter never entered any Hammer state.
CFILES += battleship_ftcommon_hammer.c
endif
ifeq ($(NDS_P2_PIKACHU),1)
# BattleShip owns Thunder Jolt, Thunder and Quick Attack; the companion TU owns
# the source Thunder Jolt (air/ground) and Thunder (head/trail) weapons.
CFILES += battleship_pikachu.c battleship_pikachu_weapons.c
endif
ifeq ($(NDS_P2_YOSHI),1)
# BattleShip owns Egg Lay, Egg Throw and Yoshi Bomb; the companion TUs own the
# source egg/star weapons and Egg Lay's victim-side common statuses.
CFILES += battleship_yoshi.c battleship_yoshi_weapons.c \
	battleship_ftcommon_captureyoshi.c
endif
ifeq ($(NDS_P2_NESS),1)
# BattleShip owns Ness's specials and articles verbatim (admit_fighter.py).
CFILES += battleship_ness.c battleship_ness_weapons.c battleship_ness_items.c
endif
ifeq ($(NDS_P2_PURIN),1)
# BattleShip owns Purin's specials and articles verbatim (admit_fighter.py).
CFILES += battleship_purin.c
endif
ifeq ($(NDS_P2_KIRBY),1)
# BattleShip owns Kirby's specials and articles verbatim (admit_fighter.py).
CFILES += battleship_kirby.c battleship_kirby_copy.c battleship_kirby_weapons.c battleship_ftcommon_capturekirby.c
endif
ifeq ($(NDS_P2_GDONKEY),1)
# BattleShip owns GDonkey's specials and articles verbatim (admit_fighter.py).
CFILES += battleship_gdonkey.c
endif
ifeq ($(NDS_P2_MMARIO),1)
# BattleShip owns MMario's specials and articles verbatim (admit_fighter.py).
CFILES += battleship_mmario.c
endif
ifeq ($(NDS_IMPORT_BATTLESHIP_MPPROCESS_LIVE),1)
CFILES += $(NDS_MPPROCESS_SOURCE_CFILES) \
	battleship_mpprocess_live_bridge.c
else ifeq ($(NDS_IMPORT_BATTLESHIP_MPPROCESS_PRIVATE),1)
NDS_PRIVATE_CHECK_CFILES += $(NDS_MPPROCESS_SOURCE_CFILES)
endif
CFILES += battleship_ftstatus_callback_aliases.c \
	battleship_ftstatus_map_physics_shims.c \
	battleship_ftstatus_inactive_stubs.c \
	battleship_ftcommon_shieldbreakfly.c \
	battleship_ftcommon_shieldbreakfall.c \
	battleship_ftcommon_shieldbreakdown.c \
	battleship_ftcommon_shieldbreakstand.c \
	battleship_ftcommon_furafura.c
endif
ifeq ($(NDS_IMPORT_BATTLESHIP_FTCOMPUTER),1)
CFILES += battleship_ftcomputer.c
endif
ifeq ($(NDS_IMPORT_BATTLESHIP_BATTLE_PLAYABLE),1)
CFILES += battleship_gmcamera.c battleship_ftcommon_dead.c \
	battleship_ftcommon_rebirth.c battleship_ftcommon_sleep.c \
	battleship_ftcommon_entry.c \
	battleship_grwallpaper.c \
	battle_playable_compat_stubs.c
endif
ifeq ($(NDS_IMPORT_BATTLESHIP_VS_RESULTS),1)
CFILES += battleship_lbtransition.c battleship_mnvsresults.c \
	battleship_scsubsysfighter.c battleship_scsubsysdata.c
endif
CFILES += battleship_ifscreenflash.c
ifeq ($(NDS_IMPORT_BATTLESHIP_IFCOMMON),1)
CFILES += battleship_ifcommon.c
CFILES += nds_battle_hud.c
endif
ifeq ($(NDS_IMPORT_BATTLESHIP_WEAPON_MANAGER),1)
CFILES += battleship_wpmanager_core.c
endif
ifneq ($(filter 1,$(NDS_IMPORT_BATTLESHIP_MARIO_FIREBALL) $(NDS_IMPORT_BATTLESHIP_FOX_BLASTER) $(NDS_IMPORT_BATTLESHIP_MARIO_SPECIAL_HI) $(NDS_IMPORT_BATTLESHIP_FOX_SPECIAL_HI)),)
CFILES += battleship_special_common.c
endif
ifeq ($(NDS_IMPORT_BATTLESHIP_MARIO_FIREBALL),1)
CFILES += battleship_mario_fireball.c
endif
ifeq ($(NDS_IMPORT_BATTLESHIP_FOX_BLASTER),1)
CFILES += battleship_fox_blaster.c
endif
ifeq ($(NDS_IMPORT_BATTLESHIP_EFFECT_MANAGER),1)
CFILES += battleship_efmanager.c
endif
ifeq ($(NDS_R2_PARTICLE_RUNTIME),1)
ifneq ($(NDS_IMPORT_BATTLESHIP_EFFECT_MANAGER),1)
$(error NDS_R2_PARTICLE_RUNTIME=1 requires NDS_IMPORT_BATTLESHIP_EFFECT_MANAGER=1: ef/efdisplay.c owns the efcommon bank request)
endif
# The generated pack is in the build now (src/nds/generated/
# nds_particle_banks.generated.inc, verified byte-reproducing by
# check-nds-particle-banks.ps1), so nds_particle_banks_placeholder.c is gone --
# both it and this line said to delete it with the commit that lands the
# generated data, and that commit is this one. Its weak empty definitions had
# also drifted: it spelled the count NDS_PARTICLE_SCRIPT_IDS and the sentinel
# NDS_PARTICLE_UNPACKED_OFFSET, neither of which the current generator emits.
# nds_particle_banks.c is that replacement -- the .inc is data with no
# translation unit of its own, so without it the link fails on
# gNdsParticleScriptBank and friends.
CFILES += nds_particle_banks.c
# guMtxIdentF, called by lbParticleGetPosVelDObj. Nothing else in the build
# defines it, so it is scoped to the flag that needs it.
CFILES += battleship_libultra_gu_mtxutil.c
CFILES += battleship_lbparticle.c
# DS-native FireGrind. The accepted path is on by default; setting
# NDS_R2_FIREGRIND_NATIVE=0 retains no-op stubs for control/fallback builds.
# Lives with the particle runtime because its draw integration is inside
# lbParticleDrawTextures.
CFILES += nds_firegrind.c
endif
ifeq ($(NDS_IMPORT_BATTLESHIP_FOX_REFLECTOR),1)
CFILES += battleship_fox_reflector.c
endif
# Fox's blaster model part, as data. Unconditional because the module's whole
# body is inside #if NDS_R2_FOX_GUN_OVERLAY -- at 0 it compiles to nothing and
# costs no ROM, which is cheaper to reason about than a second flag in the file
# list that has to stay in step with the one in the sources.
CFILES += nds_fox_gun.c
# P2-7 save data: the FAT-backed SRAM image and the transcribed lbbackup.c.
CFILES += nds_backup.c battleship_lbbackup.c
ifeq ($(NDS_IMPORT_BATTLESHIP_FT_PUBLIC),1)
CFILES += battleship_ftpublic.c
endif
ifneq ($(filter 1,$(NDS_IMPORT_BATTLESHIP_MARIO_SPECIAL_HI) $(NDS_IMPORT_BATTLESHIP_FOX_SPECIAL_HI)),)
CFILES += battleship_ftcommon_fallspecial.c
endif
ifeq ($(NDS_IMPORT_BATTLESHIP_MARIO_SPECIAL_HI),1)
CFILES += battleship_mario_special_hi.c
endif
ifeq ($(NDS_IMPORT_BATTLESHIP_MARIO_SPECIAL_LW),1)
CFILES += battleship_mario_special_lw.c
endif
ifeq ($(NDS_IMPORT_BATTLESHIP_FOX_SPECIAL_HI),1)
CFILES += battleship_fox_special_hi.c
endif
CFILES += battleship_ftmain.c
ifeq ($(NDS_TASK9_FLOAT_CENSUS),1)
CFILES += nds_task9_float_census.c
endif
ifeq ($(NDS_TASK9_STATE_HASH),1)
CFILES += nds_task9_state_hash.c
endif
ifeq ($(NDS_TASK49_GX_DIFFER),1)
CFILES += nds_task49_gx_differ.c
endif
ifeq ($(NDS_TASK10_HARDWARE_CALIBRATION),1)
CFILES += nds_task10_hardware_calibration.c
endif
# grModelSetupGroundDObjs is shared stage-model setup, not Mushroom Kingdom
# specific: Congo Jungle builds its barrel cannon with it too
# (grjungle.c:119). The gate is the OR of everyone who needs it, so the
# translation unit is linked once and no stage carries a private copy.
ifeq ($(NDS_ENABLE_INISHIE_SOURCE_SCALE_SETUP)$(NDS_P2_STAGE_JUNGLE)$(NDS_P2_STAGE_INISHIE)$(NDS_P2_STAGE_SECTOR),0000)
else
CFILES += battleship_grmodelsetup.c
endif
CPPFILES :=
SFILES := coroutine_arm.s
ifeq ($(NDS_TASK9_FLOAT_PHASE2),1)
SFILES += nds_task9_float_phase2.s
endif
ifeq ($(NDS_TASK16_FLOAT_COMPARE),1)
SFILES += nds_task16_float_compare.s
endif
ifeq ($(NDS_TASK16_FLOAT_I2F),1)
SFILES += nds_task16_float_i2f.s
endif
ifeq ($(NDS_TASK16_FLOAT_ADDSUB),1)
SFILES += nds_task16_float_addsub.s
endif
ifeq ($(NDS_FREEZE_DIAGNOSTICS),1)
CFILES += nds_freeze_diagnostics.c
SFILES += nds_freeze_diagnostics_irq.s
endif
ifeq ($(NDS_P2_UI_KIT),1)
CFILES += nds_ui_kit.c
endif
ifeq ($(NDS_P2_MENU_SHELL),1)
CFILES += nds_menu_shell.c
endif

export LD := $(CC)
export OFILES := \
	$(if $(filter 1,$(NDS_TASK9_FLOAT_ITCM)),$(NDS_TASK9_FLOAT_ITCM_OFILES)) \
	$(NDS_TASK37_ITCM_OFILES) \
	$(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)
export NDS_PRIVATE_CHECK_OFILES := $(NDS_PRIVATE_CHECK_CFILES:.c=.o)
export NDS_MPPROCESS_STRICT_OFILES := $(NDS_PRIVATE_CHECK_OFILES) \
	$(if $(filter 1,$(NDS_IMPORT_BATTLESHIP_MPPROCESS_LIVE)),$(NDS_MPPROCESS_SOURCE_CFILES:.c=.o) battleship_mpprocess_live_bridge.o)
export INCLUDE := $(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
	$(foreach dir,$(LIBDIRS),-I$(dir)/include) -I$(CURDIR)/$(BUILD)
export LIBPATHS := $(foreach dir,$(LIBDIRS),-L$(dir)/lib)
export PROJECT_ROOT := $(PROJECT_ROOT)
export NITROFS_DIR := $(NITROFS_DIR)
export BATTLESHIP_O2R := $(BATTLESHIP_O2R)
export BATTLESHIP_RELOCDATA := $(BATTLESHIP_RELOCDATA)
# P2-4s1: generators gate Yoster rows on this env var, so export it to every recipe (ui kit + particle banks).
export NDS_P2_STAGE_YOSTER := $(NDS_P2_STAGE_YOSTER)
export NDS_P2_STAGE_CASTLE := $(NDS_P2_STAGE_CASTLE)
export NDS_P2_STAGE_JUNGLE := $(NDS_P2_STAGE_JUNGLE)
export NDS_P2_STAGE_ZEBES := $(NDS_P2_STAGE_ZEBES)
export NDS_P2_STAGE_HYRULE := $(NDS_P2_STAGE_HYRULE)
export NDS_P2_STAGE_YAMABUKI := $(NDS_P2_STAGE_YAMABUKI)
export NDS_P2_STAGE_INISHIE := $(NDS_P2_STAGE_INISHIE)
export NDS_P2_STAGE_SECTOR := $(NDS_P2_STAGE_SECTOR)

NDS_OPENING_ROOM_RELOC_FILES := \
	reloc_movies/MVCommon \
	reloc_transitions/MVOpeningRoomTransition \
	reloc_movies/MVOpeningRoomScene1 \
	reloc_movies/MVOpeningRoomScene2 \
	reloc_movies/MVOpeningRoomScene3 \
	reloc_movies/MVOpeningRoomScene4 \
	reloc_movies/MVOpeningRunCrash \
	reloc_movies/MVOpeningRoomWallpaper

NDS_OPENING_PORTRAITS_RELOC_FILES := \
	reloc_movies/MVOpeningPortraitsSet1 \
	reloc_movies/MVOpeningPortraitsSet2

NDS_OPENING_MARIO_RELOC_FILES := \
	reloc_interface/IFCommonAnnounceCommon \
	reloc_movies/MVOpeningCommon

NDS_OPENING_ACTION_RELOC_FILES := \
	reloc_movies/MVOpeningRun \
	reloc_movies/MVOpeningYamabuki \
	reloc_movies/MVOpeningSector

NDS_TITLE_RELOC_FILES := \
	reloc_menus/MNTitle \
	reloc_menus/MNTitleFireAnim

NDS_VS_MODE_RELOC_FILES := \
	reloc_menus/MNCommon \
	reloc_menus/MNVSMode

NDS_PLAYERS_VS_RELOC_FILES := \
	reloc_menus/MNPlayersCommon \
	reloc_menus/MNCommon \
	reloc_fighters_common/FTEmblemSprites \
	reloc_menus/MNSelectCommon \
	reloc_menus/MNPlayersGameModes \
	reloc_menus/MNPlayersPortraits \
	reloc_menus/MNPlayersSpotlight

NDS_MAPS_RELOC_FILES := \
	reloc_fighters_common/FTEmblemSprites \
	reloc_menus/MNSelectCommon \
	reloc_menus/MNMaps \
	reloc_menus/MNCommonFonts \
	reloc_stages/GRWallpaperTrainingBlack

NDS_PUPUPU_STAGE_RELOC_FILES := \
	reloc_stages/GRPupupuMap \
	reloc_stages/StageDreamLand \
	reloc_extern_data/ExternDataBank103 \
	reloc_extern_data/ExternDataBank104 \
	reloc_extern_data/MiscDataBank152

NDS_STAGE_SCOUT_RELOC_FILES := \
	reloc_stages/GRInishieMap \
	reloc_stages/GRHyruleMap \
	reloc_stages/StageCastle \
	reloc_extern_data/ExternDataBank113

# P2-4 first stage: Yoshi's Island (Yoster), opt-in behind NDS_P2_STAGE_YOSTER.
# Payload mirrors the Pupupu set above: map header (263), wallpaper sprite
# container (StageYoshi, file 0x5d), geometry/display (ExternDataBank111 =
# StageYosterFile2) and map nodes (MiscDataBank154 = StageYosterFile3).
# The flag itself is defaulted with the other admission flags near :722.
ifeq ($(NDS_P2_STAGE_YOSTER),1)
NDS_YOSTER_STAGE_RELOC_FILES := \
	reloc_stages/GRYosterMap \
	reloc_stages/StageYoshi \
	reloc_extern_data/ExternDataBank110 \
	reloc_extern_data/ExternDataBank111 \
	reloc_extern_data/MiscDataBank154
else
NDS_YOSTER_STAGE_RELOC_FILES :=
endif

# P2-4 stage 2: Peach's Castle (Castle), opt-in behind NDS_P2_STAGE_CASTLE.
# Payload mirrors the two sets above. A bank name carries the relocData file
# number in decimal and the asset id is that number in hex, which is how each
# of these was derived rather than guessed: map header 259 = 0x103
# (GRCastleMap), wallpaper sprite container 90 = 0x5a
# (MVOpeningRoomWallpaper -- the map header references
# dMVOpeningRoomWallpaper_sprite_0x26C88, so Castle borrows the opening
# movie's room wallpaper where Yoster borrows StageYoshi's; it is already
# staged unconditionally at :4138 and rowed as asset 90, so it is not
# repeated below), geometry and
# display 106 = 0x6a (ExternDataBank106 = StageCastleFile2), and map nodes
# 156 = 0x9c (MiscDataBank156 = StageCastleFile3). The sprite list
# reloc_stages/StageCastle (95 = 0x5f) is already staged unconditionally
# above.
ifeq ($(NDS_P2_STAGE_CASTLE),1)
NDS_CASTLE_STAGE_RELOC_FILES := \
	reloc_stages/GRCastleMap \
	reloc_extern_data/ExternDataBank106 \
	reloc_extern_data/MiscDataBank156
else
NDS_CASTLE_STAGE_RELOC_FILES :=
endif

# P2-4 stage 3: Congo Jungle (Jungle), opt-in behind NDS_P2_STAGE_JUNGLE.
# Derived by the same bank-number rule and then CHECKED against the map
# header's own extern list rather than assumed: GRJungleMap (261 = 0x105)
# declares exactly 0x5c, 0x6c and 0x9e, which are StageJungle (92),
# ExternDataBank108 (StageJungleFile2, geometry and display) and
# MiscDataBank158 (StageJungleFile3, map nodes and the cannon animations).
# All three declare no externs of their own, so the closure is complete.
ifeq ($(NDS_P2_STAGE_JUNGLE),1)
NDS_JUNGLE_STAGE_RELOC_FILES := \
	reloc_stages/GRJungleMap \
	reloc_stages/StageJungle \
	reloc_extern_data/ExternDataBank108 \
	reloc_extern_data/MiscDataBank158
else
NDS_JUNGLE_STAGE_RELOC_FILES :=
endif

# P2-4 stage 4: Planet Zebes, checked against the map header the same way:
# GRZebesMap (257 = 0x101) declares exactly 0x59, 0x69 and 0x9d -- StageZebes
# (89), ExternDataBank105 (StageZebesFile2) and MiscDataBank157
# (StageZebesFile3) -- and none of the three declares an extern of its own.
ifeq ($(NDS_P2_STAGE_ZEBES),1)
NDS_ZEBES_STAGE_RELOC_FILES := \
	reloc_stages/GRZebesMap \
	reloc_stages/StageZebes \
	reloc_extern_data/ExternDataBank105 \
	reloc_extern_data/MiscDataBank157
else
NDS_ZEBES_STAGE_RELOC_FILES :=
endif

# P2-4 stage 6: Saffron City. GRYamabukiMap (264 = 0x108) declares four:
# StagePokemon (94), ExternDataBank112, MiscDataBank159 and MiscDataBank160.
# It is the only stage so far whose map needs two Misc banks.
ifeq ($(NDS_P2_STAGE_YAMABUKI),1)
NDS_YAMABUKI_STAGE_RELOC_FILES := \
	reloc_stages/GRYamabukiMap \
	reloc_stages/StagePokemon \
	reloc_extern_data/ExternDataBank112 \
	reloc_extern_data/MiscDataBank159 \
	reloc_extern_data/MiscDataBank160
else
NDS_YAMABUKI_STAGE_RELOC_FILES :=
endif

# P2-4 stage 7: Mushroom Kingdom. GRInishieMap (260 = 0x104) declares three:
# StageHyruleWallpaper (91), ExternDataBank107 and MiscDataBank155. The map
# itself is already rowed unconditionally.
ifeq ($(NDS_P2_STAGE_INISHIE),1)
NDS_INISHIE_STAGE_RELOC_FILES := \
	reloc_stages/GRInishieMap \
	reloc_stages/StageHyruleWallpaper \
	reloc_extern_data/ExternDataBank107 \
	reloc_extern_data/MiscDataBank155
else
NDS_INISHIE_STAGE_RELOC_FILES :=
endif

# P2-4 stage 8: Sector Z. GRSectorMap (262 = 0x106) declares three:
# StageSector (99), ExternDataBank109 and MiscDataBank153.
ifeq ($(NDS_P2_STAGE_SECTOR),1)
NDS_SECTOR_STAGE_RELOC_FILES := \
	reloc_stages/GRSectorMap \
	reloc_stages/StageSector \
	reloc_extern_data/ExternDataBank109 \
	reloc_extern_data/MiscDataBank153
else
NDS_SECTOR_STAGE_RELOC_FILES :=
endif

# P2-6 step 7 (boss): Final Destination's runtime map (Last), behind
# NDS_P2_1P_GAME. Derived by the same bank-number rule as the stages above:
# a bank name carries the relocData file number in decimal and the asset id
# is that number in hex -- map 266 = 0x10a (GRLastMap) and geometry/display
# 114 = 0x72 (ExternDataBank114 = StageLastFile2). The map's extern table also
# names the wallpaper sprite container 96 = 0x60 (StageLastBackground, held
# by the o2r container NAMED StageYamabukiWallpaper -- the o2r's supplemental
# wallpaper names are a consistent mislabelling, see nds_reloc_assets.c): the
# native packet draws the wallpaper, but the
# extern-tree loader refuses a parent with an unrowed dependency, so it is
# staged to satisfy the fixup. No new NDS_P2_STAGE_LAST flag: the 1P arenas
# ride the 1P Game flag. The list also carries Meta Crystal (GRMetalMap 269 =
# 0x10d, StageMetalFile2 117 = 0x75, wallpaper 98 = 0x62 held by the container
# named StageLastWallpaper),
# Small Yoshi's Island (GRYosterSmallMap 270 = 0x10e, StageYosterSmallFile2 118 =
# 0x76; its images bank 110 and the StageYoshi wallpaper are staged with Yoshi's
# Island) and Beta Dream Land (GRPupupuSmallMap 256 = 0x100, StagePupupuBeta1
# 101 = 0x65, StagePupupuBetaImages 100 = 0x64; the StageDreamLand wallpaper is
# staged since P1) and Duel Zone (GRZakoMap 268 = 0x10c, StageBattlefieldFile2
# 116 = 0x74, wallpaper 97 = 0x61 held by the container named
# StageInishieWallpaper).
ifeq ($(NDS_P2_1P_GAME),1)
NDS_LAST_STAGE_RELOC_FILES := \
	reloc_stages/GRLastMap \
	reloc_extern_data/ExternDataBank114 \
	reloc_stages/StageYamabukiWallpaper \
	reloc_stages/GRMetalMap \
	reloc_extern_data/ExternDataBank117 \
	reloc_stages/StageLastWallpaper \
	reloc_stages/GRYosterSmallMap \
	reloc_extern_data/ExternDataBank118 \
	reloc_stages/GRPupupuSmallMap \
	reloc_extern_data/ExternDataBank101 \
	reloc_extern_data/ExternDataBank100 \
	reloc_stages/GRZakoMap \
	reloc_extern_data/ExternDataBank116 \
	reloc_stages/StageInishieWallpaper
else
NDS_LAST_STAGE_RELOC_FILES :=
endif

NDS_MARIOFOX_FIGHTER_RELOC_FILES := \
	reloc_fighters_common/FTManagerCommon \
	reloc_fighters_main/MarioMain \
	reloc_fighters_main/MarioMainMotion \
	reloc_fighters_main/MarioModel \
	reloc_fighters_main/MarioShieldPose \
	reloc_fighters_main/MarioSpecial1 \
	reloc_fighters_main/MarioSpecial2 \
	reloc_fighters_main/MarioSpecial3 \
	reloc_fighters_main/FoxMain \
	reloc_fighters_main/FoxMainMotion \
	reloc_fighters_main/FoxModel \
	reloc_fighters_main/FoxShieldPose \
	reloc_fighters_main/FoxSpecial1 \
	reloc_fighters_main/FoxSpecial2 \
	reloc_fighters_main/FoxSpecial3 \
	reloc_fighters_main/FoxSpecial4 \
	reloc_extern_data/MiscData201 \
	reloc_extern_data/MiscData299 \
	reloc_extern_data/MiscData315 \
	reloc_extern_data/ExternDataBank109 \
	reloc_animations/FTMarioAnimWait \
	reloc_animations/FTMarioAnim001 \
	reloc_animations/FTMarioAnim002 \
	reloc_animations/FTMarioAnim003 \
	reloc_animations/FTMarioAnim004 \
	reloc_animations/FTMarioAnim005 \
	reloc_animations/FTMarioAnim006 \
	reloc_animations/FTMarioAnim007 \
	reloc_animations/FTMarioAnim008 \
	reloc_animations/FTMarioAnim009 \
	reloc_animations/FTMarioAnim010 \
	reloc_animations/FTMarioAnim011 \
	reloc_animations/FTMarioAnim012 \
	reloc_animations/FTMarioAnim013 \
	reloc_animations/FTMarioAnim014 \
	reloc_animations/FTMarioAnim015 \
	reloc_animations/FTMarioAnim016 \
	reloc_animations/FTMarioAnim017 \
	reloc_animations/FTMarioAnim018 \
	reloc_animations/FTMarioAnim019 \
	reloc_animations/FTMarioAnim020 \
	reloc_animations/FTMarioAnim021 \
	reloc_animations/FTMarioAnim022 \
	reloc_animations/FTMarioAnim023 \
	reloc_animations/FTMarioAnim024 \
	reloc_animations/FTMarioAnim025 \
	reloc_animations/FTMarioAnim026 \
	reloc_animations/FTMarioAnim027 \
	reloc_animations/FTMarioAnim028 \
	reloc_animations/FTMarioAnim029 \
	reloc_animations/FTMarioAnim030 \
	reloc_animations/FTMarioAnim031 \
	reloc_animations/FTMarioAnim032 \
	reloc_animations/FTMarioAnim033 \
	reloc_animations/FTMarioAnim034 \
	reloc_animations/FTMarioAnim035 \
	reloc_animations/FTMarioAnim036 \
	reloc_animations/FTMarioAnim037 \
	reloc_animations/FTMarioAnim038 \
	reloc_animations/FTMarioAnim039 \
	reloc_animations/FTMarioAnim040 \
	reloc_animations/FTMarioAnim041 \
	reloc_animations/FTMarioAnim042 \
	reloc_animations/FTMarioAnim043 \
	reloc_animations/FTMarioAnimDownBounceD \
	reloc_animations/FTMarioAnim045 \
	reloc_animations/FTMarioAnimDownStandD \
	reloc_animations/FTMarioAnim047 \
	reloc_animations/FTMarioAnim048 \
	reloc_animations/FTMarioAnim049 \
	reloc_animations/FTMarioAnim050 \
	reloc_animations/FTMarioAnim051 \
	reloc_animations/FTMarioAnim052 \
	reloc_animations/FTMarioAnim053 \
	reloc_animations/FTMarioAnim054 \
	reloc_animations/FTMarioAnim055 \
	reloc_animations/FTMarioAnim056 \
	reloc_animations/FTMarioAnim057 \
	reloc_animations/FTMarioAnim058 \
	reloc_animations/FTMarioAnim059 \
	reloc_animations/FTMarioAnim060 \
	reloc_animations/FTMarioAnim061 \
	reloc_animations/FTMarioAnim062 \
	reloc_animations/FTMarioAnim063 \
	reloc_animations/FTMarioAnim064 \
	reloc_animations/FTMarioAnim065 \
	reloc_animations/FTMarioAnim066 \
	reloc_animations/FTMarioAnim067 \
	reloc_animations/FTMarioAnim068 \
	reloc_animations/FTMarioAnim069 \
	reloc_animations/FTMarioAnim070 \
	reloc_animations/FTMarioAnim071 \
	reloc_animations/FTMarioAnim072 \
	reloc_animations/FTMarioAnim073 \
	reloc_animations/FTMarioAnim074 \
	reloc_animations/FTMarioAnim075 \
	reloc_animations/FTMarioAnim076 \
	reloc_animations/FTMarioAnim077 \
	reloc_animations/FTMarioAnim078 \
	reloc_animations/FTMarioAnim079 \
	reloc_animations/FTMarioAnim080 \
	reloc_animations/FTMarioAnim081 \
	reloc_animations/FTMarioAnim082 \
	reloc_animations/FTMarioAnim083 \
	reloc_animations/FTMarioAnim084 \
	reloc_animations/FTMarioAnim085 \
	reloc_animations/FTMarioAnim086 \
	reloc_animations/FTMarioAnim087 \
	reloc_animations/FTMarioAnim088 \
	reloc_animations/FTMarioAnim089 \
	reloc_animations/FTMarioAnim090 \
	reloc_animations/FTMarioAnim091 \
	reloc_animations/FTMarioAnim092 \
	reloc_animations/FTMarioAnim093 \
	reloc_animations/FTMarioAnim094 \
	reloc_animations/FTMarioAnim095 \
	reloc_animations/FTMarioAnim096 \
	reloc_animations/FTMarioAnim097 \
	reloc_animations/FTMarioAnim098 \
	reloc_animations/FTMarioAnim099 \
	reloc_animations/FTMarioAnim100 \
	reloc_animations/FTMarioAnim101 \
	reloc_animations/FTMarioAnim102 \
	reloc_animations/FTMarioAnim103 \
	reloc_animations/FTMarioAnim104 \
	reloc_animations/FTMarioAnim105 \
	reloc_animations/FTMarioAnim106 \
	reloc_animations/FTMarioAnim107 \
	reloc_animations/FTMarioAnim108 \
	reloc_animations/FTMarioAnim109 \
	reloc_animations/FTMarioAnim110 \
	reloc_animations/FTMarioAnim111 \
	reloc_animations/FTMarioAnim112 \
	reloc_animations/FTMarioAnim113 \
	reloc_animations/FTMarioAnim114 \
	reloc_animations/FTMarioAnim115 \
	reloc_animations/FTMarioAnim116 \
	reloc_animations/FTMarioAnim117 \
	reloc_animations/FTMarioAnim118 \
	reloc_animations/FTMarioAnim119 \
	reloc_animations/FTMarioAnim120 \
	reloc_animations/FTMarioAnim121 \
	reloc_animations/FTMarioAnim122 \
	reloc_animations/FTMarioAnim123 \
	reloc_animations/FTMarioAnim124 \
	reloc_animations/FTMarioAnim125 \
	reloc_animations/FTMarioAnim126 \
	reloc_animations/FTMarioAnim127 \
	reloc_animations/FTMarioAnim128 \
	reloc_animations/FTMarioAnim129 \
	reloc_animations/FTMarioAnim130 \
	reloc_animations/FTMarioAnim131 \
	reloc_animations/FTMarioAnim132 \
	reloc_animations/FTMarioAnim133 \
	reloc_animations/FTMarioAnim134 \
	reloc_animations/FTMarioAnim135 \
	reloc_animations/FTMarioAnim136 \
	reloc_animations/FTMarioAnim137 \
	reloc_animations/FTMarioAnim138 \
	reloc_animations/FTMarioAnim139 \
	reloc_animations/FTMarioAnim140 \
	reloc_animations/FTMarioAnim141 \
	reloc_animations/FTMarioAnim142 \
	reloc_animations/FTFoxAnim000 \
	reloc_animations/FTFoxAnim001 \
	reloc_animations/FTFoxAnim002 \
	reloc_animations/FTFoxAnim003 \
	reloc_animations/FTFoxAnim004 \
	reloc_animations/FTFoxAnim005 \
	reloc_animations/FTFoxAnim006 \
	reloc_animations/FTFoxAnim007 \
	reloc_animations/FTFoxAnim008 \
	reloc_animations/FTFoxAnim009 \
	reloc_animations/FTFoxAnim010 \
	reloc_animations/FTFoxAnim011 \
	reloc_animations/FTFoxAnim012 \
	reloc_animations/FTFoxAnim013 \
	reloc_animations/FTFoxAnim014 \
	reloc_animations/FTFoxAnim015 \
	reloc_animations/FTFoxAnim016 \
	reloc_animations/FTFoxAnim017 \
	reloc_animations/FTFoxAnim018 \
	reloc_animations/FTFoxAnim019 \
	reloc_animations/FTFoxAnim020 \
	reloc_animations/FTFoxAnim021 \
	reloc_animations/FTFoxAnim022 \
	reloc_animations/FTFoxAnim023 \
	reloc_animations/FTFoxAnim024 \
	reloc_animations/FTFoxAnim025 \
	reloc_animations/FTFoxAnim026 \
	reloc_animations/FTFoxAnim027 \
	reloc_animations/FTFoxAnim028 \
	reloc_animations/FTFoxAnim029 \
	reloc_animations/FTFoxAnim030 \
	reloc_animations/FTFoxAnim031 \
	reloc_animations/FTFoxAnim032 \
	reloc_animations/FTFoxAnim033 \
	reloc_animations/FTFoxAnim034 \
	reloc_animations/FTFoxAnim035 \
	reloc_animations/FTFoxAnim036 \
	reloc_animations/FTFoxAnim037 \
	reloc_animations/FTFoxAnim038 \
	reloc_animations/FTFoxAnim039 \
	reloc_animations/FTFoxAnim040 \
	reloc_animations/FTFoxAnim041 \
	reloc_animations/FTFoxAnim042 \
	reloc_animations/FTFoxAnim043 \
	reloc_animations/FTFoxAnim044 \
	reloc_animations/FTFoxAnim045 \
	reloc_animations/FTFoxAnim046 \
	reloc_animations/FTFoxAnim047 \
	reloc_animations/FTFoxAnim048 \
	reloc_animations/FTFoxAnim049 \
	reloc_animations/FTFoxAnim050 \
	reloc_animations/FTFoxAnim051 \
	reloc_animations/FTFoxAnim052 \
	reloc_animations/FTFoxAnim053 \
	reloc_animations/FTFoxAnim054 \
	reloc_animations/FTFoxAnim055 \
	reloc_animations/FTFoxAnim056 \
	reloc_animations/FTFoxAnim057 \
	reloc_animations/FTFoxAnim058 \
	reloc_animations/FTFoxAnim059 \
	reloc_animations/FTFoxAnim060 \
	reloc_animations/FTFoxAnim061 \
	reloc_animations/FTFoxAnim062 \
	reloc_animations/FTFoxAnim063 \
	reloc_animations/FTFoxAnim064 \
	reloc_animations/FTFoxAnim065 \
	reloc_animations/FTFoxAnim066 \
	reloc_animations/FTFoxAnim067 \
	reloc_animations/FTFoxAnim068 \
	reloc_animations/FTFoxAnim069 \
	reloc_animations/FTFoxAnim070 \
	reloc_animations/FTFoxAnim071 \
	reloc_animations/FTFoxAnim072 \
	reloc_animations/FTFoxAnim073 \
	reloc_animations/FTFoxAnim074 \
	reloc_animations/FTFoxAnim075 \
	reloc_animations/FTFoxAnim076 \
	reloc_animations/FTFoxAnim077 \
	reloc_animations/FTFoxAnim078 \
	reloc_animations/FTFoxAnim079 \
	reloc_animations/FTFoxAnim080 \
	reloc_animations/FTFoxAnim081 \
	reloc_animations/FTFoxAnim082 \
	reloc_animations/FTFoxAnim083 \
	reloc_animations/FTFoxAnim084 \
	reloc_animations/FTFoxAnim085 \
	reloc_animations/FTFoxAnim086 \
	reloc_animations/FTFoxAnim087 \
	reloc_animations/FTFoxAnim088 \
	reloc_animations/FTFoxAnim089 \
	reloc_animations/FTFoxAnim090 \
	reloc_animations/FTFoxAnim091 \
	reloc_animations/FTFoxAnim092 \
	reloc_animations/FTFoxAnim093 \
	reloc_animations/FTFoxAnim094 \
	reloc_animations/FTFoxAnim095 \
	reloc_animations/FTFoxAnim096 \
	reloc_animations/FTFoxAnim097 \
	reloc_animations/FTFoxAnim098 \
	reloc_animations/FTFoxAnim099 \
	reloc_animations/FTFoxAnim100 \
	reloc_animations/FTFoxAnim101 \
	reloc_animations/FTFoxAnim102 \
	reloc_animations/FTFoxAnim103 \
	reloc_animations/FTFoxAnim104 \
	reloc_animations/FTFoxAnim105 \
	reloc_animations/FTFoxAnim106 \
	reloc_animations/FTFoxAnim107 \
	reloc_animations/FTFoxAnim108 \
	reloc_animations/FTFoxAnim109 \
	reloc_animations/FTFoxAnim110 \
	reloc_animations/FTFoxAnim111 \
	reloc_animations/FTFoxAnim112 \
	reloc_animations/FTFoxAnim113 \
	reloc_animations/FTFoxAnim114 \
	reloc_animations/FTFoxAnim115 \
	reloc_animations/FTFoxAnim116 \
	reloc_animations/FTFoxAnim117 \
	reloc_animations/FTFoxAnim118 \
	reloc_animations/FTFoxAnim119 \
	reloc_animations/FTFoxAnim120 \
	reloc_animations/FTFoxAnim121 \
	reloc_animations/FTFoxAnim122 \
	reloc_animations/FTFoxAnim123 \
	reloc_animations/FTFoxAnim124 \
	reloc_animations/FTFoxAnim125 \
	reloc_animations/FTFoxAnim126 \
	reloc_animations/FTFoxAnim127 \
	reloc_animations/FTFoxAnim128 \
	reloc_animations/FTFoxAnim129 \
	reloc_animations/FTFoxAnim130 \
	reloc_animations/FTFoxAnim131 \
	reloc_animations/FTFoxAnim132 \
	reloc_animations/FTFoxAnim133 \
	reloc_animations/FTFoxAnim134 \
	reloc_animations/FTFoxAnim135 \
	reloc_animations/FTFoxAnim136 \
	reloc_animations/FTFoxAnim137 \
	reloc_animations/FTFoxAnim138 \
	reloc_animations/FTFoxAnim139 \
	reloc_animations/FTFoxAnim140 \
	reloc_animations/FTFoxAnim141 \
	reloc_animations/FTFoxAnim142 \
	reloc_animations/FTFoxAnim143 \
	reloc_animations/FTFoxAnim144 \
	reloc_animations/FTFoxAnim145 \
	reloc_animations/FTFoxAnim146 \
	reloc_animations/FTFoxAnim147 \
	reloc_animations/FTFoxAnim148 \
	reloc_animations/FTFoxAnim149 \
	reloc_animations/FTFoxAnim150 \
	reloc_animations/FTFoxAnim151 \
	reloc_animations/FTFoxAnim152 \
	reloc_animations/FTFoxAnim153 \
	reloc_animations/FTFoxAnim154 \
	reloc_animations/FTFoxAnim155 \
	reloc_animations/FTFoxAnim156 \
	reloc_animations/FTFoxAnim157

# P2-3 appends only the files a new fighter adds beyond Mario/Fox.  The fragment
# is generated from BattleShip FTData + generated relocData names + O2R headers;
# no fighter file inventory belongs in this Makefile from this point forward.
include $(PROJECT_ROOT)/scripts/fighters/fighter_production_files.mk
NDS_P2_FIGHTER_RELOC_FILES :=
ifeq ($(NDS_P2_LUIGI),1)
NDS_P2_FIGHTER_RELOC_FILES += $(NDS_P2_LUIGI_FIGHTER_RELOC_FILES)
endif
ifeq ($(NDS_P2_DONKEY),1)
NDS_P2_FIGHTER_RELOC_FILES += $(NDS_P2_DONKEY_FIGHTER_RELOC_FILES)
endif
ifeq ($(NDS_P2_CAPTAIN),1)
NDS_P2_FIGHTER_RELOC_FILES += $(NDS_P2_CAPTAIN_FIGHTER_RELOC_FILES)
endif
ifeq ($(NDS_P2_SAMUS),1)
NDS_P2_FIGHTER_RELOC_FILES += $(NDS_P2_SAMUS_FIGHTER_RELOC_FILES)
endif
ifeq ($(NDS_P2_LINK),1)
NDS_P2_FIGHTER_RELOC_FILES += $(NDS_P2_LINK_FIGHTER_RELOC_FILES)
endif
ifeq ($(NDS_P2_PIKACHU),1)
NDS_P2_FIGHTER_RELOC_FILES += $(NDS_P2_PIKACHU_FIGHTER_RELOC_FILES)
endif
ifeq ($(NDS_P2_YOSHI),1)
NDS_P2_FIGHTER_RELOC_FILES += $(NDS_P2_YOSHI_FIGHTER_RELOC_FILES)
endif
ifeq ($(NDS_P2_NESS),1)
NDS_P2_FIGHTER_RELOC_FILES += $(NDS_P2_NESS_FIGHTER_RELOC_FILES)
endif
ifeq ($(NDS_P2_PURIN),1)
NDS_P2_FIGHTER_RELOC_FILES += $(NDS_P2_PURIN_FIGHTER_RELOC_FILES)
endif
ifeq ($(NDS_P2_KIRBY),1)
NDS_P2_FIGHTER_RELOC_FILES += $(NDS_P2_KIRBY_FIGHTER_RELOC_FILES)
endif
ifeq ($(NDS_P2_GDONKEY),1)
NDS_P2_FIGHTER_RELOC_FILES += $(NDS_P2_GDONKEY_FIGHTER_RELOC_FILES)
endif
ifeq ($(NDS_P2_MMARIO),1)
NDS_P2_FIGHTER_RELOC_FILES += $(NDS_P2_MMARIO_FIGHTER_RELOC_FILES)
endif

# BPS1 replaces these AObj16 O2R payloads rather than duplicating them. Keeping
# both crosses the four-CPU ROM's measured 16 MiB runner boundary; more
# importantly, every retained source file would be dead ROM once the verified
# compact clip is the authoritative acquisition path. AObj32 entry/effect files
# remain on the ordinary reloc loader and are restored after the prefix filter.
NDS_FTANIM_STREAM_REPLACED_RELOC_FILES :=
ifeq ($(NDS_R2_FTANIM_STREAM),1)
NDS_FTANIM_STREAM_PATTERNS := \
	reloc_animations/FTMarioAnim% \
	reloc_animations/FTFoxAnim% \
	reloc_animations/FTLuigiAnim% \
	reloc_animations/FTDonkeyAnim% \
	reloc_animations/FTSamusAnim% \
	reloc_animations/FTCaptainAnim%
NDS_FTANIM_STREAM_AOBJ32_FILES := \
	reloc_animations/FTMarioAnim134 \
	reloc_animations/FTMarioAnim135 \
	reloc_animations/FTFoxAnim135 \
	reloc_animations/FTFoxAnim136 \
	reloc_animations/FTDonkeyAnim132 \
	reloc_animations/FTDonkeyAnim133 \
	reloc_animations/FTSamusAnim137 \
	reloc_animations/FTSamusAnim138 \
	reloc_animations/FTCaptainAnim136 \
	reloc_animations/FTCaptainAnim137 \
	reloc_animations/FTCaptainAnim138 \
	reloc_animations/FTCaptainAnim139
NDS_MARIOFOX_FIGHTER_RELOC_FILES_FULL := $(NDS_MARIOFOX_FIGHTER_RELOC_FILES)
NDS_P2_FIGHTER_RELOC_FILES_FULL := $(NDS_P2_FIGHTER_RELOC_FILES)
NDS_FTANIM_STREAM_REPLACED_RELOC_FILES := $(filter \
	$(NDS_FTANIM_STREAM_PATTERNS),$(NDS_MARIOFOX_FIGHTER_RELOC_FILES_FULL) \
	$(NDS_P2_FIGHTER_RELOC_FILES_FULL))
NDS_MARIOFOX_FIGHTER_RELOC_FILES := $(filter-out \
	$(NDS_FTANIM_STREAM_PATTERNS),$(NDS_MARIOFOX_FIGHTER_RELOC_FILES_FULL)) \
	$(filter $(NDS_FTANIM_STREAM_AOBJ32_FILES),$(NDS_MARIOFOX_FIGHTER_RELOC_FILES_FULL))
NDS_P2_FIGHTER_RELOC_FILES := $(filter-out \
	$(NDS_FTANIM_STREAM_PATTERNS),$(NDS_P2_FIGHTER_RELOC_FILES_FULL)) \
	$(filter $(NDS_FTANIM_STREAM_AOBJ32_FILES),$(NDS_P2_FIGHTER_RELOC_FILES_FULL))
endif

NDS_EFFECT_RELOC_FILES := \
	reloc_effects/EFCommonEffects1 \
	reloc_effects/EFCommonEffects2 \
	reloc_effects/EFCommonEffects3

# P2-3f48. The item subsystem's shared data file, and the one file its 68
# external pointers all resolve into. ITCommonData carries the descriptors,
# models, textures and animation for every common, monster and stage item, so
# the whole of P2-5 waits on it being resident; it is 3,392 bytes of payload.
# MiscData086 is 79,584 and Yoshi's own reloc closure already stages it, which
# is why this pair costs 3,392 bytes on a ROM carrying him and 82,976 on one
# that is not. Duplicated staging is harmless -- the NitroFS copy rule is
# idempotent and the asset table returns the first matching row.
NDS_ITEM_RELOC_FILES := \
	reloc_items/ITCommonData \
	reloc_extern_data/MiscData086 \
	reloc_interface/IFCommonItem

NDS_1P_RELOC_FILES := \
	reloc_scene/SC1PStageClear1 \
	reloc_scene/SC1PStageClear2 \
	reloc_scene/SC1PStageClear3 \
	reloc_scene/SC1PChallenger \
	reloc_scene/SC1PIntro \
	reloc_menus/MN1P \
	reloc_menus/MN1PContinue \
	reloc_menus/MNPlayers1PMode \
	reloc_menus/MNMessage \
	reloc_scene/SCStaffroll \
	reloc_movies/MVEnding \
	reloc_menus/MNCongraCaptainBottom \
	reloc_menus/MNCongraCaptainTop \
	reloc_menus/MNCongraDonkeyBottom \
	reloc_menus/MNCongraDonkeyTop \
	reloc_menus/MNCongraFoxBottom \
	reloc_menus/MNCongraFoxTop \
	reloc_menus/MNCongraKirbyBottom \
	reloc_menus/MNCongraKirbyTop \
	reloc_menus/MNCongraLinkBottom \
	reloc_menus/MNCongraLinkTop \
	reloc_menus/MNCongraLuigiBottom \
	reloc_menus/MNCongraLuigiTop \
	reloc_menus/MNCongraMarioBottom \
	reloc_menus/MNCongraMarioTop \
	reloc_menus/MNCongraNessBottom \
	reloc_menus/MNCongraNessTop \
	reloc_menus/MNCongraPikachuBottom \
	reloc_menus/MNCongraPikachuTop \
	reloc_menus/MNCongraPurinBottom \
	reloc_menus/MNCongraPurinTop \
	reloc_menus/MNCongraSamusBottom \
	reloc_menus/MNCongraSamusTop \
	reloc_menus/MNCongraYoshiBottom \
	reloc_menus/MNCongraYoshiTop \
	reloc_menus/MNPlayersDifficulty \
	reloc_misc_named/CharacterNames \
	reloc_bonus/BonusPicture \
	reloc_bonus/BonusPicturePlatform

NDS_MODES_RELOC_FILES := \
	reloc_menus/MNData \
	reloc_menus/MNDataCommon \
	reloc_menus/MNVSRecordMain \
	reloc_menus/MNCharacters \
	reloc_menus/MNOption \
	reloc_menus/MNBackupClear \
	reloc_menus/MNBackupClearHeaderOption \
	reloc_menus/MNSoundTest \
	reloc_scene/SC1PTrainingMode \
	reloc_stages/GRWallpaperTrainingBlack \
	reloc_stages/GRWallpaperTrainingBlue \
	reloc_stages/GRWallpaperTrainingYellow \
	reloc_scene/SCExplainGraphics \
	reloc_scene/SCExplainMain

NDS_VSBATTLE_RELOC_FILES := \
	reloc_interface/IFCommonPlayer \
	reloc_interface/IFCommonGameStatus \
	reloc_interface/IFCommonPlayerDamage \
	reloc_interface/IFCommonTimer \
	reloc_interface/IFCommonDigits \
	reloc_interface/IFCommonBattlePause \
	reloc_interface/IFCommonPlayerTags \
	reloc_interface/IFCommonAnnounceCommon \
	reloc_misc_named/SYKseg1Validate

NDS_VS_RESULTS_RELOC_FILES :=
ifeq ($(NDS_IMPORT_BATTLESHIP_VS_RESULTS),1)
NDS_VS_RESULTS_RELOC_FILES := \
	reloc_menus/MNVSResults \
	reloc_fighters_common/FTEmblemModels \
	reloc_fighters_common/FTStocksZako \
	reloc_transitions/LBTransitionAeroplane \
	reloc_transitions/LBTransitionCheck \
	reloc_transitions/LBTransitionGakubuthi \
	reloc_transitions/LBTransitionKannon \
	reloc_transitions/LBTransitionStar \
	reloc_transitions/LBTransitionSudare1 \
	reloc_transitions/LBTransitionSudare2 \
	reloc_transitions/LBTransitionCamera \
	reloc_transitions/LBTransitionBlock \
	reloc_transitions/LBTransitionRotScale \
	reloc_transitions/LBTransitionCurtain
endif

NDS_STARTUP_RELOC_FILES := \
	reloc_misc_named/N64Logo

NDS_AUDIO_FILES := \
	audio/S1_music_sbk \
	audio/B1_sounds1_ctl \
	audio/B1_sounds1_tbl \
	audio/B1_sounds2_ctl \
	audio/B1_sounds2_tbl \
	audio/fgm_unk \
	audio/fgm_tbl \
	audio/fgm_ucd

NDS_AUDIO_DERIVED_FILES :=
ifeq ($(NDS_IMPORT_BATTLESHIP_AUDIO_BGM),1)
NDS_AUDIO_DERIVED_FILES := \
	audio/bgm_pupupu_ima.bin \
	audio/bgm_win_mario_ima.bin \
	audio/bgm_win_fox_ima.bin \
	audio/bgm_results_ima.bin \
	audio/bgm_mode_select_ima.bin \
	audio/bgm_battle_select_ima.bin
# P2-4 Yoster BGM: staged asset lands only when the stage flag is on. Like
# every BGM here the asset lives under the gitignored assets/ tree and is
# rendered offline rather than by a rule, so reproduce it with:
#   python scripts/sfx/bgm/render-audio-bgm.py --sequence-index 8 \
#          --output assets/audio/bgm_yoster_ima.bin
# Sequence 8 is nSYAudioBGMYoster (gm/gmsound.h:31-40, sequential from 0).
# Nested inside the BGM-import gate like every other derived BGM so flag-off
# output is identical.
ifeq ($(NDS_P2_STAGE_YOSTER),1)
NDS_AUDIO_DERIVED_FILES += \
	audio/bgm_yoster_ima.bin
endif
# P2-4 Castle BGM, same shape. Reproduce with:
#   python scripts/sfx/bgm/render-audio-bgm.py --sequence-index 6 \
#          --output assets/audio/bgm_castle_ima.bin
# Sequence 6 is nSYAudioBGMCastle (gm/gmsound.h:31-37).
ifeq ($(NDS_P2_STAGE_CASTLE),1)
NDS_AUDIO_DERIVED_FILES += \
	audio/bgm_castle_ima.bin
endif
# Reproduce: python scripts/sfx/bgm/render-audio-bgm.py --sequence-index 1 \
#          --output assets/audio/bgm_zebes_ima.bin
ifeq ($(NDS_P2_STAGE_ZEBES),1)
NDS_AUDIO_DERIVED_FILES += \
	audio/bgm_zebes_ima.bin
endif
# Reproduce: python scripts/sfx/bgm/render-audio-bgm.py --sequence-index 9 \
#          --output assets/audio/bgm_hyrule_ima.bin
ifeq ($(NDS_P2_STAGE_HYRULE),1)
NDS_AUDIO_DERIVED_FILES += \
	audio/bgm_hyrule_ima.bin
endif
# Reproduce: python scripts/sfx/bgm/render-audio-bgm.py --sequence-index 7 \
#          --output assets/audio/bgm_yamabuki_ima.bin
ifeq ($(NDS_P2_STAGE_YAMABUKI),1)
NDS_AUDIO_DERIVED_FILES += \
	audio/bgm_yamabuki_ima.bin
endif
# Reproduce: python scripts/sfx/bgm/render-audio-bgm.py --sequence-index 2 \
#          --output assets/audio/bgm_inishie_ima.bin
ifeq ($(NDS_P2_STAGE_INISHIE),1)
NDS_AUDIO_DERIVED_FILES += \
	audio/bgm_inishie_ima.bin
endif
# Reproduce: python scripts/sfx/bgm/render-audio-bgm.py --sequence-index 5 \
#          --output assets/audio/bgm_jungle_ima.bin
# Held until 2026-09-03 on a doubling suspicion: its loop start sits at
# 50.06% of the track. A read of render-audio-bgm.py:271-273 settles it --
# a song whose last note runs past one period renders periods_needed=2, so
# loop_start = base + P against loop_end = base + 2P, which IS the midpoint
# when the intro is short. The six accepted tracks all render one period.
# Not a unit bug: bytes and samples convert consistently at :286-287, :334
# and :625, and the two channels agree on the period to within 80 ticks.
ifeq ($(NDS_P2_STAGE_JUNGLE),1)
NDS_AUDIO_DERIVED_FILES += \
	audio/bgm_jungle_ima.bin
endif
# Reproduce: python scripts/sfx/bgm/render-audio-bgm.py --sequence-index 4 \
#          --output assets/audio/bgm_sector_ima.bin
# Same two-period shape as Jungle above, loop start at 53.62%.
ifeq ($(NDS_P2_STAGE_SECTOR),1)
NDS_AUDIO_DERIVED_FILES += \
	audio/bgm_sector_ima.bin
endif
endif

# Removed Task 42 PCM assets can survive an incremental build-directory reuse
# and are otherwise silently repacked by ndstool.
export NDS_AUDIO_OBSOLETE_DERIVED_FILES := \
	audio/bgm_pupupu_pcm16.raw \
	audio/bgm_win_mario_pcm16.raw \
	audio/bgm_win_fox_pcm16.raw \
	audio/bgm_results_pcm16.raw
ifeq ($(NDS_IMPORT_BATTLESHIP_AUDIO_FGM),1)
NDS_AUDIO_DERIVED_FILES += \
	audio/fgm_phase_pack_ima.bin
endif

NDS_INISHIE_SCALE_RELOCDATA_FILES :=
ifeq ($(NDS_ENABLE_INISHIE_SOURCE_SCALE_SETUP),1)
NDS_INISHIE_SCALE_RELOCDATA_FILES := \
	155.vpk0.bin
endif

export NDS_NITROFS_RELOC_FILES := \
	$(foreach file,$(NDS_STARTUP_RELOC_FILES),$(NITROFS_DIR)/reloc/$(file)) \
	$(foreach file,$(NDS_OPENING_ROOM_RELOC_FILES),$(NITROFS_DIR)/reloc/$(file)) \
	$(foreach file,$(NDS_OPENING_PORTRAITS_RELOC_FILES),$(NITROFS_DIR)/reloc/$(file)) \
	$(foreach file,$(NDS_OPENING_MARIO_RELOC_FILES),$(NITROFS_DIR)/reloc/$(file)) \
	$(foreach file,$(NDS_OPENING_ACTION_RELOC_FILES),$(NITROFS_DIR)/reloc/$(file)) \
	$(foreach file,$(NDS_TITLE_RELOC_FILES),$(NITROFS_DIR)/reloc/$(file)) \
	$(foreach file,$(NDS_VS_MODE_RELOC_FILES),$(NITROFS_DIR)/reloc/$(file)) \
	$(foreach file,$(NDS_PLAYERS_VS_RELOC_FILES),$(NITROFS_DIR)/reloc/$(file)) \
	$(foreach file,$(NDS_MAPS_RELOC_FILES),$(NITROFS_DIR)/reloc/$(file)) \
	$(foreach file,$(NDS_PUPUPU_STAGE_RELOC_FILES),$(NITROFS_DIR)/reloc/$(file)) \
	$(foreach file,$(NDS_YOSTER_STAGE_RELOC_FILES),$(NITROFS_DIR)/reloc/$(file)) \
	$(foreach file,$(NDS_CASTLE_STAGE_RELOC_FILES),$(NITROFS_DIR)/reloc/$(file)) \
	$(foreach file,$(NDS_JUNGLE_STAGE_RELOC_FILES),$(NITROFS_DIR)/reloc/$(file)) \
	$(foreach file,$(NDS_ZEBES_STAGE_RELOC_FILES),$(NITROFS_DIR)/reloc/$(file)) \
	$(foreach file,$(NDS_YAMABUKI_STAGE_RELOC_FILES),$(NITROFS_DIR)/reloc/$(file)) \
	$(foreach file,$(NDS_INISHIE_STAGE_RELOC_FILES),$(NITROFS_DIR)/reloc/$(file)) \
	$(foreach file,$(NDS_SECTOR_STAGE_RELOC_FILES),$(NITROFS_DIR)/reloc/$(file)) \
	$(foreach file,$(NDS_LAST_STAGE_RELOC_FILES),$(NITROFS_DIR)/reloc/$(file)) \
	$(foreach file,$(NDS_STAGE_SCOUT_RELOC_FILES),$(NITROFS_DIR)/reloc/$(file)) \
	$(foreach file,$(NDS_MARIOFOX_FIGHTER_RELOC_FILES),$(NITROFS_DIR)/reloc/$(file)) \
	$(foreach file,$(NDS_P2_FIGHTER_RELOC_FILES),$(NITROFS_DIR)/reloc/$(file)) \
	$(foreach file,$(NDS_EFFECT_RELOC_FILES),$(NITROFS_DIR)/reloc/$(file)) \
	$(foreach file,$(NDS_ITEM_RELOC_FILES),$(NITROFS_DIR)/reloc/$(file)) \
	$(foreach file,$(NDS_VSBATTLE_RELOC_FILES),$(NITROFS_DIR)/reloc/$(file)) \
	$(foreach file,$(NDS_MODES_RELOC_FILES),$(NITROFS_DIR)/reloc/$(file)) \
	$(foreach file,$(NDS_1P_RELOC_FILES),$(NITROFS_DIR)/reloc/$(file)) \
	$(foreach file,$(NDS_VS_RESULTS_RELOC_FILES),$(NITROFS_DIR)/reloc/$(file))

export NDS_NITROFS_AUDIO_FILES := \
	$(foreach file,$(NDS_AUDIO_FILES),$(NITROFS_DIR)/$(file)) \
	$(foreach file,$(NDS_AUDIO_DERIVED_FILES),$(NITROFS_DIR)/$(file))

export NDS_NITROFS_RELOCDATA_FILES := \
	$(foreach file,$(NDS_INISHIE_SCALE_RELOCDATA_FILES),$(NITROFS_DIR)/relocdata/us/$(file))

export NDS_NITROFS_BATTLE_STATIC_TEXTURE_FILES :=
ifeq ($(NDS_DEV_SCENE_HARNESS_ID),163)
ifeq ($(NDS_RENDERER_HW_TRIANGLES),1)
NDS_NITROFS_BATTLE_STATIC_TEXTURE_FILES := \
	$(NITROFS_DIR)/renderer/battle_playable_static_textures.rgb5a1.bin
endif
endif

# P2-1c's UI pack ships only with the runtime that reads it. Empty by default,
# so a ROM built without NDS_P2_UI_KIT does not carry 24,384 bytes nothing
# opens -- which is what makes the published ROMs byte-identical across this
# row.
export NDS_NITROFS_MN_UI_KIT_FILES :=
ifeq ($(NDS_P2_UI_KIT),1)
NDS_NITROFS_MN_UI_KIT_FILES := $(NITROFS_DIR)/menus/mn_ui_kit.bin
# P2-1h's backdrop art rides the same flag and the same reason: without the kit
# there is no reader, and 177,900 bytes of ROM nothing opens is what keeps the
# published ROMs byte-identical across this row.
NDS_NITROFS_MN_UI_KIT_FILES += $(NITROFS_DIR)/menus/mn_surfaces.bin
endif

# The efcommon payloads only ship with the interpreter that reads them; without
# the runtime they are 200,896 bytes of ROM nothing opens. Two files, two
# encodings of the same texels: the .ds.bin is per-texture DS formats with
# palettes (the pack), the .rgb5a1.bin is the 22 admitted textures as RGB555+A1
# (the draw path, which uploads through a texture cache with no palette slot).
# Slice 32's baked animation bank ships only when a reader is compiled in. Empty
# by default, so the published ROM is byte-identical until the runtime bind
# lands -- see NDS_R2_FTANIM_DENSE above.
export NDS_NITROFS_FTANIM_FILES :=
ifeq ($(NDS_R2_FTANIM_DENSE),1)
NDS_NITROFS_FTANIM_FILES := $(NITROFS_DIR)/animation/ftanim_dense_bank.bin
endif
ifeq ($(NDS_R2_FTANIM_STREAM),1)
NDS_NITROFS_FTANIM_FILES += $(NITROFS_DIR)/zz_stream/ftanim_stream_pack.bin
endif

# Slice 1 phase 5's resident figatree pack. Empty unless a reader is compiled
# in, for the same reason as the bank above: without NDS_R2_BATTLEPACK it is
# 287,904 bytes of ROM nothing opens.
export NDS_NITROFS_BATTLEPACK_FILES :=
ifeq ($(NDS_R2_BATTLEPACK),1)
NDS_NITROFS_BATTLEPACK_FILES := $(NITROFS_DIR)/animation/battlepack_fox.bin
endif

export NDS_NITROFS_PARTICLE_FILES :=
ifeq ($(NDS_R2_PARTICLE_RUNTIME),1)
NDS_NITROFS_PARTICLE_FILES := \
	$(NITROFS_DIR)/particles/efcommon_particle_textures.ds.bin \
	$(NITROFS_DIR)/particles/efcommon_particle_quads.a5i3.bin
ifeq ($(NDS_R2_WHISPY_NATIVE_TEXTURES),1)
NDS_NITROFS_PARTICLE_FILES += \
	$(NITROFS_DIR)/particles/grpupupu_whispy_native.ds.bin
endif
endif

# The Task 39 hit-spark sheet. Unlike the payload above this one has a live
# reader (ndsTask39PrepareHitSparks), so the ROM does not boot correctly
# without it whenever the sprite lane is on.
export NDS_NITROFS_EFFECT_FILES :=
ifeq ($(NDS_TASK39_FX_SPRITES),1)
NDS_NITROFS_EFFECT_FILES := \
	$(NITROFS_DIR)/effects/task39_hit_sparks.rgb5a1.bin
endif

.PHONY: all clean clean-generated distclean run $(BUILD) prune-obsolete-audio p2-fighter-production-manifest

all: $(BUILD)



$(BUILD):
	@mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile PROJECT_ROOT=$(PROJECT_ROOT) BUILD=$(BUILD) BUILD_OUTPUT_ROOT=$(BUILD_OUTPUT_ROOT) NDS_OUTPUT_ROOT=$(NDS_OUTPUT_ROOT) NDS_PUBLISH_USER_ROM=$(NDS_PUBLISH_USER_ROM)

clean:
	@echo clean ...
	@rm -fr $(BUILD) $(NDS_OUTPUT_ROOT)/$(NDS_OUTPUT_BASENAME).elf $(NDS_OUTPUT_ROOT)/$(NDS_OUTPUT_BASENAME).nds $(NDS_OUTPUT_ROOT)/$(NDS_OUTPUT_BASENAME).ds.gba

clean-generated:
	@powershell -NoProfile -ExecutionPolicy Bypass -File scripts/clean-generated.ps1 -Force

distclean: clean-generated

run: $(BUILD)
	@echo "ROM ready: $(NDS_OUTPUT_ROOT)/$(NDS_OUTPUT_BASENAME).nds"

# P2-3 source inventory. This is deliberately explicit rather than an implicit
# ROM prerequisite: updating the tracked manifest is a reviewable fighter-
# production step, not a side effect of every ordinary build.
p2-fighter-production-manifest:
	@python scripts/fighters/generate_fighter_production_manifest.py --repo-root .

# P1 convenience goals. P1 ships smash64ds-battle-playable-hwtri.nds and is
# measured on its flag-identical tick-HUD sibling; bare `make` builds the P2
# smash64ds.nds the milestone does not need. These wrap the exact TARGET/BUILD
# pairs the Boundary verifier and sample-tick-hud-buckets.ps1 already use, so
# alias builds stay incremental with harness builds. The sub-makes run
# sequentially -- one build at a time is still the rule -- and `p1-tick` alone
# is the cheap compile check during iteration.
.PHONY: p1 p1-tick
p1:
	@+$(MAKE) --no-print-directory TARGET=smash64ds-battle-playable-hwtri BUILD=build-battle-playable-canonical-hwtri-harness
	@+$(MAKE) --no-print-directory TARGET=smash64ds-battle-playable-tickhud-hwtri BUILD=build-tick-hud-buckets

p1-tick:
	@+$(MAKE) --no-print-directory TARGET=smash64ds-battle-playable-tickhud-hwtri BUILD=build-tick-hud-buckets

else

# The outer make exports the full NitroFS reloc prerequisite inventory so this
# recursive make can inherit it without rebuilding the list.  P2-3 fighter
# staging makes that inventory large enough to overflow the Windows/MSYS child
# process environment even though Make itself can still consume the value.
# Retain it as Make dependency metadata, but keep it out of every recipe's
# environment; native devkitARM tools otherwise fail to launch with ENOENT.
unexport NDS_NITROFS_RELOC_FILES

DEPENDS := $(OFILES:.o=.d) $(NDS_PRIVATE_CHECK_OFILES:.o=.d)
NDS_BUILD_CONFIG := $(PROJECT_ROOT)/$(BUILD)/nds_build_config.h
NDS_SCENE_HARNESS_CONFIG := $(PROJECT_ROOT)/$(BUILD)/nds_scene_harness_config.h
SCENE_BACKEND_SLICES := \
	$(PROJECT_ROOT)/src/port/diagnostics.c \
	$(PROJECT_ROOT)/src/port/taskman_seam.c \
	$(PROJECT_ROOT)/src/port/reloc_backend.c \
	$(PROJECT_ROOT)/src/port/reloc_backend_compat_shims.c \
	$(PROJECT_ROOT)/src/port/reloc_backend_assets.c \
	$(PROJECT_ROOT)/src/port/reloc_backend_fighter_model.c \
	$(PROJECT_ROOT)/src/port/reloc_backend_renderer_dl.c \
	$(PROJECT_ROOT)/src/port/reloc_backend_movement.c \
	$(PROJECT_ROOT)/src/port/reloc_backend_mp_collision.c \
	$(PROJECT_ROOT)/src/port/reloc_backend_cliff_ledge.c \
	$(PROJECT_ROOT)/src/port/reloc_backend_diagnostic_recorders.c \
	$(PROJECT_ROOT)/src/port/sprite_preview_backend.c \
	$(PROJECT_ROOT)/src/port/opening_movie_backend.c \
	$(PROJECT_ROOT)/src/port/title_backend.c

.PHONY: all FORCE prune-obsolete-audio

all: $(OUTPUT).nds

# P2-3r4. THE NATIVE-OWNER TABLE IMAGES.
#
# A P2-3 fighter's generated geometry used to be `static const` arrays in the
# ARM9 binary, and on this hardware that costs the taskman arena one byte for
# one byte: the arena is calloc'd from whatever the heap has left. Measured, a
# Luigi+Donkey shell was left with 13,840 B of arena headroom and the battle
# died in `ifCommonCountdownMakeInterface` when the countdown interface's
# allocation returned NULL.
#
# These objects are compiled for their BYTES, never linked into the ARM9 image:
# `objcopy -O binary` lifts the `.fighter_image` section into a NitroFS payload
# the runtime loads for the fighters a match actually uses. The struct layout is
# the compiler's own, so the image and the runtime's offsetof cannot disagree.
#
# Same "ships only with its reader" rule as the UI kit above: no image files are
# staged unless the owner is built, so a Mario/Fox ROM carries none of them.
NDS_NATIVE_IMAGE_DIR := $(NITROFS_DIR)/fighters
NDS_NATIVE_IMAGE_SRC_DIR := $(PROJECT_ROOT)/src/nds/generated
NDS_NATIVE_IMAGE_GENERATOR := 	$(PROJECT_ROOT)/scripts/fighters/generate_nds_native_owner_images.py
NDS_NATIVE_IMAGE_HEADER := 	$(PROJECT_ROOT)/include/nds/generated/nds_native_fighter_image.generated.h
export NDS_NITROFS_NATIVE_IMAGE_FILES :=
NDS_NATIVE_IMAGE_OWNERS :=
ifeq ($(NDS_P2_LUIGI),1)
NDS_NATIVE_IMAGE_OWNERS += luigi
endif
ifeq ($(NDS_P2_DONKEY),1)
NDS_NATIVE_IMAGE_OWNERS += donkey
endif
ifeq ($(NDS_P2_CAPTAIN),1)
NDS_NATIVE_IMAGE_OWNERS += captain
endif
ifeq ($(NDS_P2_SAMUS),1)
NDS_NATIVE_IMAGE_OWNERS += samus
endif
ifeq ($(NDS_P2_LINK),1)
NDS_NATIVE_IMAGE_OWNERS += link
endif
ifeq ($(NDS_P2_PIKACHU),1)
NDS_NATIVE_IMAGE_OWNERS += pikachu
endif
ifeq ($(NDS_P2_YOSHI),1)
NDS_NATIVE_IMAGE_OWNERS += yoshi
endif
ifeq ($(NDS_P2_NESS),1)
NDS_NATIVE_IMAGE_OWNERS += ness
endif
ifeq ($(NDS_P2_PURIN),1)
NDS_NATIVE_IMAGE_OWNERS += purin
endif
ifeq ($(NDS_P2_KIRBY),1)
NDS_NATIVE_IMAGE_OWNERS += kirby
endif
ifeq ($(NDS_P2_MMARIO),1)
NDS_NATIVE_IMAGE_OWNERS += mmario
endif
NDS_NITROFS_NATIVE_IMAGE_FILES := $(foreach owner,$(NDS_NATIVE_IMAGE_OWNERS),	$(NDS_NATIVE_IMAGE_DIR)/$(owner)_high.bin 	$(NDS_NATIVE_IMAGE_DIR)/$(owner)_low.bin)

$(NDS_NATIVE_IMAGE_HEADER): $(NDS_NATIVE_IMAGE_GENERATOR)
	python "$(NDS_NATIVE_IMAGE_GENERATOR)"
	@touch $(NDS_NATIVE_IMAGE_HEADER)

$(BUILD)/native_image_%.o: $(NDS_NATIVE_IMAGE_SRC_DIR)/nds_native_fighter_%.image.c 		$(NDS_NATIVE_IMAGE_HEADER) 		$(PROJECT_ROOT)/include/nds/nds_native_fighter_tables.h
	@mkdir -p $(dir $@)
	# The include paths are explicit here because this rule runs in the OUTER
	# make, where ds_rules' per-build INCLUDES have not been composed yet.
	$(CC) -c $(CFLAGS) -I $(PROJECT_ROOT)/include -I $(BUILD) -o $@ $<

$(NDS_NATIVE_IMAGE_DIR)/%.bin: $(BUILD)/native_image_%.o
	@mkdir -p $(dir $@)
	$(OBJCOPY) -O binary --only-section=.fighter_image $< $@


# The blob must exist before the config is written: the config carries its byte
# count, and the runtime carves exactly that much at the head of every arena
# generation. Generated from the blob rather than hardcoded so it cannot drift
# from the asset -- a stale constant here is a pack that silently never becomes
# resident.
ifeq ($(NDS_R2_BATTLEPACK),1)
$(NDS_BUILD_CONFIG): $(NDS_BATTLEPACK_BLOB)
endif

$(NDS_BUILD_CONFIG): FORCE
	@tmp="$@.tmp"; \
	{ \
		echo '#ifndef NDS_BUILD_CONFIG_H'; \
		echo '#define NDS_BUILD_CONFIG_H'; \
		echo '#define NDS_DEV_LIVE_INPUT_PREVIEW $(NDS_DEV_LIVE_INPUT_PREVIEW)'; \
		echo '#define NDS_HARNESS_FAST_LOGIC $(NDS_HARNESS_FAST_LOGIC)'; \
		echo '#define NDS_HARNESS_FAST_PRESENT_ON_REQUEST $(NDS_HARNESS_FAST_PRESENT_ON_REQUEST)'; \
		echo '#define NDS_RENDERER_HW_TRIANGLES $(NDS_RENDERER_HW_TRIANGLES)'; \
		echo '#define NDS_RENDERER_HW_DEBUG_TEXTURE_ONLY $(NDS_RENDERER_HW_DEBUG_TEXTURE_ONLY)'; \
		echo '#define NDS_RENDERER_PROFILE_LEVEL $(NDS_RENDERER_PROFILE_LEVEL)'; \
		echo '#define NDS_SHIP_TELEMETRY $(NDS_SHIP_TELEMETRY)'; \
		echo '#define NDS_TICK_HUD $(NDS_TICK_HUD)'; \
		echo '#define NDS_RENDERER_M2_DETAILED_LEDGER $(NDS_RENDERER_M2_DETAILED_LEDGER)'; \
		echo '#define NDS_RENDERER_M3_PHASE0_PROFILE $(NDS_RENDERER_M3_PHASE0_PROFILE)'; \
		echo '#define NDS_NATIVE_STAGE_GENERATED_SEGMENT0_ENABLE $(NDS_NATIVE_STAGE_GENERATED_SEGMENT0_ENABLE)'; \
		echo '#define NDS_TASK29_GX_CENSUS $(NDS_TASK29_GX_CENSUS)'; \
		echo '#define NDS_TASK34_STAGE_STREAM_CENSUS $(NDS_TASK34_STAGE_STREAM_CENSUS)'; \
		echo '#define NDS_TASK49_GX_DIFFER $(NDS_TASK49_GX_DIFFER)'; \
		echo '#define NDS_TASK36_HW_COMPOSE $(NDS_TASK36_HW_COMPOSE)'; \
		$(if $(NDS_TASK36_RIGID_BINDING_MASK),echo '#define NDS_RENDERER_TASK36_RIGID_BINDING_MASK $(NDS_TASK36_RIGID_BINDING_MASK)'; ,) \
		echo '#define NDS_R2_STAGE_ROUTE_PROBE $(NDS_R2_STAGE_ROUTE_PROBE)'; \
		echo '#define NDS_TASK51_STAGE_NATIVE $(NDS_TASK51_STAGE_NATIVE)'; \
		echo '#define NDS_DREAMLAND_DS_MESH $(NDS_DREAMLAND_DS_MESH)'; \
		echo '#define NDS_DREAMLAND_CARD_CULL $(NDS_DREAMLAND_CARD_CULL)'; \
		echo '#define NDS_DREAMLAND_CARD_CULL_MASK0 $(NDS_DREAMLAND_CARD_CULL_MASK0)u'; \
		echo '#define NDS_DREAMLAND_CARD_CULL_MASK1 $(NDS_DREAMLAND_CARD_CULL_MASK1)u'; \
		echo '#define NDS_TASK53_REPLAY_ARENA_FIX $(NDS_TASK53_REPLAY_ARENA_FIX)'; \
		echo '#define NDS_TASK55_STAGE_GEOM $(NDS_TASK55_STAGE_GEOM)'; \
		echo '#define NDS_TASK56_FIGHTER_PRIMITIVES $(NDS_TASK56_FIGHTER_PRIMITIVES)'; \
		echo '#define NDS_BATTLE_PROFILE $(NDS_BATTLE_PROFILE)'; \
		echo '#define NDS_TASK44_STAGE_STEADY $(NDS_TASK44_STAGE_STEADY)'; \
		echo '#define NDS_TASK68_FALLBACK_CENSUS $(NDS_TASK68_FALLBACK_CENSUS)'; \
		echo '#define NDS_TASK86_MATRIX_COPY $(NDS_TASK86_MATRIX_COPY)'; \
		echo '#define NDS_TASK85_ALIGNED_NATIVE_ACCESS $(NDS_TASK85_ALIGNED_NATIVE_ACCESS)'; \
		echo '#define NDS_TASK82_ITCM_REPACK $(NDS_TASK82_ITCM_REPACK)'; \
		echo '#define NDS_TASK37_PROFILE $(NDS_TASK37_PROFILE)'; \
		echo '#define NDS_TASK37_ITCM_LEAVES $(NDS_TASK37_ITCM_LEAVES)'; \
		echo '#define NDS_TASK37_ITCM_PORT $(NDS_TASK37_ITCM_PORT)'; \
		echo '#define NDS_TASK37_LAYOUT_PROBE $(NDS_TASK37_LAYOUT_PROBE)'; \
		echo '#define NDS_TASK37_LAYOUT_PROBE_ITCM $(NDS_TASK37_LAYOUT_PROBE_ITCM)'; \
		echo '#define NDS_TASK9_STATE_HASH_SKIP_CONTROLLERS $(NDS_TASK9_STATE_HASH_SKIP_CONTROLLERS)'; \
		echo '#define NDS_TASK9_STATE_HASH_REGION_MASK $(NDS_TASK9_STATE_HASH_REGION_MASK)u'; \
		echo '#define NDS_TASK9_STATE_HASH_STRIDE $(NDS_TASK9_STATE_HASH_STRIDE)u'; \
		echo '#define NDS_TASK9_FTSTRUCT_SNAPSHOT $(NDS_TASK9_FTSTRUCT_SNAPSHOT)'; \
		echo '#define NDS_TASK9_FTSTRUCT_SNAPSHOT_UPDATE $(NDS_TASK9_FTSTRUCT_SNAPSHOT_UPDATE)u'; \
		echo '#define NDS_TASK37_PROFILE_START $(NDS_TASK37_PROFILE_START)u'; \
		echo '#define NDS_TASK37_PROFILE_FRAMES $(NDS_TASK37_PROFILE_FRAMES)u'; \
		echo '#define NDS_TASK37_PROFILE_PER_FRAME_REGION $(NDS_TASK37_PROFILE_PER_FRAME_REGION)'; \
		echo '#define NDS_TASK37_PROFILE_RESULTS $(NDS_TASK37_PROFILE_RESULTS)'; \
		echo '#define NDS_TASK22_WALLPAPER_RUN_LAB $(NDS_TASK22_WALLPAPER_RUN_LAB)'; \
		echo '#define NDS_RENDERER_SCREEN_SPACE_CENSUS $(NDS_RENDERER_SCREEN_SPACE_CENSUS)'; \
		echo '#define NDS_TASK90_SHADE_CENSUS $(NDS_TASK90_SHADE_CENSUS)'; \
		echo '#define NDS_TASK93_TEXKEY_CENSUS $(NDS_TASK93_TEXKEY_CENSUS)'; \
		echo '#define NDS_TASK107_RENDER_STATE_CENSUS $(NDS_TASK107_RENDER_STATE_CENSUS)'; \
		echo '#define NDS_TASK108_SITR_CALLBACK_CENSUS $(NDS_TASK108_SITR_CALLBACK_CENSUS)'; \
		echo '#define NDS_LAB_CULL_PROBE $(NDS_LAB_CULL_PROBE)'; \
		echo '#define NDS_LAB_TINT_SHIFT $(NDS_LAB_TINT_SHIFT)'; \
		echo '#define NDS_LAB_NO_CULL $(NDS_LAB_NO_CULL)'; \
		echo '#define NDS_TASK91_DRAW_PHASE_CENSUS $(NDS_TASK91_DRAW_PHASE_CENSUS)'; \
		echo '#define NDS_R2_SPAN_LEAN_TIMING $(NDS_R2_SPAN_LEAN_TIMING)'; \
		echo '#define NDS_R2_DELTA_PATH_ITCM $(NDS_R2_DELTA_PATH_ITCM)'; \
		echo '#define NDS_R2_ANIM_CACHE $(NDS_R2_ANIM_CACHE)'; \
		echo '#define NDS_R2_BATTLEPACK $(NDS_R2_BATTLEPACK)'; \
		echo '#define NDS_R2_FTANIM_STREAM $(NDS_R2_FTANIM_STREAM)'; \
		echo '#define NDS_R2_BATTLEPACK_KEEP_CACHE $(NDS_R2_BATTLEPACK_KEEP_CACHE)'; \
		echo '#define NDS_R2_BATTLEPACK_DISPATCH $(NDS_R2_BATTLEPACK_DISPATCH)'; \
		echo '#define NDS_R2_FTANIM_TRACK $(NDS_R2_FTANIM_TRACK)'; \
		echo '#define NDS_R2_FTANIM_TRACK_DISPATCH $(NDS_R2_FTANIM_TRACK_DISPATCH)'; \
		echo '#define NDS_R2_FTANIM_TRACK_ORACLE $(NDS_R2_FTANIM_TRACK_ORACLE)'; \
		echo '#define NDS_FT_POSE $(NDS_FT_POSE)'; \
		echo '#define NDS_FT_POSE_HOLD $(NDS_FT_POSE_HOLD)'; \
		echo '#define NDS_FT_POSE_ORACLE $(NDS_FT_POSE_ORACLE)'; \
		echo '#define NDS_FOX_BLASTER_BORE_OFFSET_Y $(NDS_FOX_BLASTER_BORE_OFFSET_Y)'; \
		echo "#define NDS_R2_BATTLEPACK_BLOB_BYTES $$(test -f '$(NDS_BATTLEPACK_BLOB)' && wc -c < '$(NDS_BATTLEPACK_BLOB)' || echo 0)u"; \
		echo '#define NDS_R2_AOBJ16_PREBAKE $(NDS_R2_AOBJ16_PREBAKE)'; \
		echo '#define NDS_R2_ANIM_CUT_ROUTE $(NDS_R2_ANIM_CUT_ROUTE)'; \
		echo '#define NDS_R2_STRIP_ROUTE $(NDS_R2_STRIP_ROUTE)'; \
		echo '#define NDS_R2_MP_ROUTE $(NDS_R2_MP_ROUTE)'; \
		echo '#define NDS_R2_RELOC_ALIAS_ROUTE $(NDS_R2_RELOC_ALIAS_ROUTE)'; \
		echo '#define NDS_R2_MATERIAL_DYNAMIC $(NDS_R2_MATERIAL_DYNAMIC)'; \
		echo '#define NDS_R2_FLASH_PROBE $(NDS_R2_FLASH_PROBE)'; \
		echo '#define NDS_R2_ANIM_CENSUS $(NDS_R2_ANIM_CENSUS)'; \
		echo '#define NDS_R2_CUBIC_FIXED $(NDS_R2_CUBIC_FIXED)'; \
		echo '#define NDS_R2_RELOC_FIXUP_TIMING $(NDS_R2_RELOC_FIXUP_TIMING)'; \
		echo '#define NDS_R2_LOADFRAME_TIMING $(NDS_R2_LOADFRAME_TIMING)'; \
		echo '#define NDS_R2_BOTH_CPU $(NDS_R2_BOTH_CPU)'; \
		echo '#define NDS_P2_FOUR_CPU_STRESS $(NDS_P2_FOUR_CPU_STRESS)'; \
		echo '#define NDS_P2_FOUR_CPU_ROSTER $(NDS_P2_FOUR_CPU_ROSTER)'; \
		echo '#define NDS_P2_FOUR_CPU_KIND0 $(NDS_P2_FOUR_CPU_KIND0)'; \
		echo '#define NDS_P2_FOUR_CPU_KIND1 $(NDS_P2_FOUR_CPU_KIND1)'; \
		echo '#define NDS_P2_FOUR_CPU_KIND2 $(NDS_P2_FOUR_CPU_KIND2)'; \
		echo '#define NDS_P2_FOUR_CPU_KIND3 $(NDS_P2_FOUR_CPU_KIND3)'; \
		echo '#define NDS_P2_LUIGI $(NDS_P2_LUIGI)'; \
		echo '#define NDS_NATIVE_OWNER_IMAGE_LUIGI $(NDS_NATIVE_OWNER_IMAGE_LUIGI)'; \
		echo '#define NDS_NATIVE_OWNER_IMAGE_DONKEY $(NDS_NATIVE_OWNER_IMAGE_DONKEY)'; \
		echo '#define NDS_NATIVE_OWNER_IMAGE_VERIFY $(NDS_NATIVE_OWNER_IMAGE_VERIFY)'; \
		echo '#define NDS_P2_DONKEY $(NDS_P2_DONKEY)'; \
		echo '#define NDS_P2_CAPTAIN $(NDS_P2_CAPTAIN)'; \
		echo '#define NDS_P2_SAMUS $(NDS_P2_SAMUS)'; \
		echo '#define NDS_P2_LINK $(NDS_P2_LINK)'; \
		echo '#define NDS_P2_PIKACHU $(NDS_P2_PIKACHU)'; \
		echo '#define NDS_P2_YOSHI $(NDS_P2_YOSHI)'; \
		echo '#define NDS_P2_STAGE_YOSTER $(NDS_P2_STAGE_YOSTER)'; \
		echo '#define NDS_P2_STAGE_CASTLE $(NDS_P2_STAGE_CASTLE)'; \
		echo '#define NDS_P2_STAGE_JUNGLE $(NDS_P2_STAGE_JUNGLE)'; \
		echo '#define NDS_P2_STAGE_ZEBES $(NDS_P2_STAGE_ZEBES)'; \
		echo '#define NDS_P2_STAGE_HYRULE $(NDS_P2_STAGE_HYRULE)'; \
		echo '#define NDS_P2_STAGE_YAMABUKI $(NDS_P2_STAGE_YAMABUKI)'; \
		echo '#define NDS_P2_STAGE_INISHIE $(NDS_P2_STAGE_INISHIE)'; \
		echo '#define NDS_P2_STAGE_SECTOR $(NDS_P2_STAGE_SECTOR)'; \
		echo '#define NDS_P2_NESS $(NDS_P2_NESS)'; \
		echo '#define NDS_P2_PURIN $(NDS_P2_PURIN)'; \
		echo '#define NDS_P2_KIRBY $(NDS_P2_KIRBY)'; \
		echo '#define NDS_P2_GDONKEY $(NDS_P2_GDONKEY)'; \
		echo '#define NDS_P2_MMARIO $(NDS_P2_MMARIO)'; \
		echo '#define NDS_P2_ITEM_CORE $(NDS_P2_ITEM_CORE)'; \
		echo '#define NDS_P2_1P_GAME $(NDS_P2_1P_GAME)'; \
		echo '#define NDS_P2_SHELL_ARGMAX_ROSTER $(NDS_P2_SHELL_ARGMAX_ROSTER)'; \
		echo '#define NDS_NATIVE_OWNER_IMAGE_CAPTAIN $(NDS_NATIVE_OWNER_IMAGE_CAPTAIN)'; \
		echo '#define NDS_NATIVE_OWNER_IMAGE_SAMUS $(NDS_NATIVE_OWNER_IMAGE_SAMUS)'; \
		echo '#define NDS_NATIVE_OWNER_IMAGE_LINK $(NDS_NATIVE_OWNER_IMAGE_LINK)'; \
		echo '#define NDS_NATIVE_OWNER_IMAGE_PIKACHU $(NDS_NATIVE_OWNER_IMAGE_PIKACHU)'; \
		echo '#define NDS_NATIVE_OWNER_IMAGE_YOSHI $(NDS_NATIVE_OWNER_IMAGE_YOSHI)'; \
		echo '#define NDS_NATIVE_OWNER_IMAGE_NESS $(NDS_NATIVE_OWNER_IMAGE_NESS)'; \
		echo '#define NDS_NATIVE_OWNER_IMAGE_PURIN $(NDS_NATIVE_OWNER_IMAGE_PURIN)'; \
		echo '#define NDS_NATIVE_OWNER_IMAGE_KIRBY $(NDS_NATIVE_OWNER_IMAGE_KIRBY)'; \
		echo '#define NDS_NATIVE_OWNER_IMAGE_MMARIO $(NDS_NATIVE_OWNER_IMAGE_MMARIO)'; \
		echo '#define NDS_NATIVE_OWNER_IMAGE_GDONKEY $(NDS_NATIVE_OWNER_IMAGE_GDONKEY)'; \
		echo '#define NDS_P2_PROOF_FIGHTER0 $(NDS_P2_PROOF_FIGHTER0)'; \
		echo '#define NDS_P2_SAMUS_STATE_TOUR $(NDS_P2_SAMUS_STATE_TOUR)'; \
		echo '#define NDS_P2_SAMUS_TUMBLE_TOUR $(NDS_P2_SAMUS_TUMBLE_TOUR)'; \
		echo '#define NDS_P2_SAMUS_DAMAGEFLY_TOUR $(NDS_P2_SAMUS_DAMAGEFLY_TOUR)'; \
		echo '#define NDS_P2_SAMUS_ATTACK_TOUR $(NDS_P2_SAMUS_ATTACK_TOUR)'; \
		echo '#define NDS_P2_LINK_BOMB_TOUR $(NDS_P2_LINK_BOMB_TOUR)'; \
		echo '#define NDS_P2_LINK_SPECIAL_TOUR $(NDS_P2_LINK_SPECIAL_TOUR)'; \
		echo '#define NDS_R2_SOAK_MATCH_MINUTES $(NDS_R2_SOAK_MATCH_MINUTES)'; \
		echo '#define NDS_ANIM_JOINT_AUDIT $(NDS_ANIM_JOINT_AUDIT)'; \
		echo '#define NDS_AOBJ_EVENT32_HASH_ORACLE $(NDS_AOBJ_EVENT32_HASH_ORACLE)'; \
		echo '#define NDS_AOBJ_EVENT32_LEDGER_INDEX $(NDS_AOBJ_EVENT32_LEDGER_INDEX)'; \
		echo '#define NDS_R2_UNLIT_VERTEX_EPOCH $(NDS_R2_UNLIT_VERTEX_EPOCH)'; \
		echo '#define NDS_R204_FPSHUD_SHADOW $(NDS_R204_FPSHUD_SHADOW)'; \
		echo '#define NDS_TASK103_STAGE_RUN_PHASE $(NDS_TASK103_STAGE_RUN_PHASE)'; \
		echo '#define NDS_TASK104_STAGE_STATS_ELISION $(NDS_TASK104_STAGE_STATS_ELISION)'; \
		echo '#define NDS_TASK106_UPDATES_PER_PRESENT $(NDS_TASK106_UPDATES_PER_PRESENT)u'; \
		echo '#define NDS_TASK75_LOAD_CENSUS $(NDS_TASK75_LOAD_CENSUS)'; \
		echo '#define NDS_R2_FTR_CONTRACT_CENSUS $(NDS_R2_FTR_CONTRACT_CENSUS)'; \
		echo '#define NDS_R2_FTR_DRAW_MEMO $(NDS_R2_FTR_DRAW_MEMO)u'; \
		echo '#define NDS_FTR_PLAN_ROUTE $(NDS_FTR_PLAN_ROUTE)u'; \
		echo '#define NDS_ENTRY_EFFECT_DIAG $(NDS_ENTRY_EFFECT_DIAG)'; \
		echo '#define NDS_FTR_PLAN_VERIFY $(NDS_FTR_PLAN_VERIFY)u'; \
		echo '#define NDS_R2_SECOND_ENTRY_DIAG $(NDS_R2_SECOND_ENTRY_DIAG)'; \
		echo '#define NDS_R2_SCENE_LOOP_WALK $(NDS_R2_SCENE_LOOP_WALK)u'; \
		echo '#define NDS_P2_UI_KIT $(NDS_P2_UI_KIT)'; \
		echo '#define NDS_P2_MENU_SHELL $(NDS_P2_MENU_SHELL)'; \
		echo '#define NDS_P2_MENU_WALK $(NDS_P2_MENU_WALK)u'; \
		echo '#define NDS_BOOT_DIAG_TEXT $(NDS_BOOT_DIAG_TEXT)'; \
		echo '#define NDS_R2_PATH $(NDS_R2_PATH)'; \
		echo '#define NDS_R2_STAGE_DIRECT $(NDS_R2_STAGE_DIRECT)'; \
		echo '#define NDS_R2_FIXED_SQRT $(NDS_R2_FIXED_SQRT)'; \
		echo '#define NDS_R2_STAGE_DMA $(NDS_R2_STAGE_DMA)'; \
		echo '#define NDS_R2_STAGE_ACTORS_PROOF $(NDS_R2_STAGE_ACTORS_PROOF)'; \
		echo '#define NDS_R2_STAGE_VIEWPROJ $(NDS_R2_STAGE_VIEWPROJ)'; \
		echo '#define NDS_R2_STAGE_PREFLIGHT $(NDS_R2_STAGE_PREFLIGHT)'; \
		echo '#define NDS_R2_FIGHTER_SHADE_PROOF $(NDS_R2_FIGHTER_SHADE_PROOF)'; \
		echo '#define NDS_R2_FIGHTER_RUN_PROOF $(NDS_R2_FIGHTER_RUN_PROOF)'; \
		echo '#define NDS_R2_FIGHTER_MTX_DIRECT $(NDS_R2_FIGHTER_MTX_DIRECT)'; \
		echo '#define NDS_R2_FIGHTER_HW_MTX $(NDS_R2_FIGHTER_HW_MTX)'; \
		echo '#define NDS_R2_FIGHTER_GX_COMPOSE $(NDS_R2_FIGHTER_GX_COMPOSE)'; \
		echo '#define NDS_R2_STAGE_VALIDATE_STRIDE $(NDS_R2_STAGE_VALIDATE_STRIDE)'; \
		echo '#define NDS_R2_FIGHTER_HW_LIGHT $(NDS_R2_FIGHTER_HW_LIGHT)'; \
		echo '#define NDS_R2_FIGHTER_SOFT_LIGHT_KEEP $(NDS_R2_FIGHTER_SOFT_LIGHT_KEEP)'; \
		echo '#define NDS_TICK_HUD_DRAW $(NDS_TICK_HUD_DRAW)'; \
		echo '#define NDS_R2_FIGHTER_SHUFFLE_FOLD $(NDS_R2_FIGHTER_SHUFFLE_FOLD)'; \
		echo '#define NDS_R2_FIGHTER_EPOCH_STATE_PROOF $(NDS_R2_FIGHTER_EPOCH_STATE_PROOF)'; \
		echo '#define NDS_R2_FIGHTER_SHADE_SKIP $(NDS_R2_FIGHTER_SHADE_SKIP)'; \
		echo '#define NDS_R2_FIGHTER_STATESPAN_SKIP $(NDS_R2_FIGHTER_STATESPAN_SKIP)'; \
		echo '#define NDS_R2_DRAW_SUPPRESS_MASK $(NDS_R2_DRAW_SUPPRESS_MASK)'; \
		echo '#define NDS_R2_FIGHTER_RUN_MEMO $(NDS_R2_FIGHTER_RUN_MEMO)'; \
		echo '#define NDS_R2_FIGHTER_PACKET $(NDS_R2_FIGHTER_PACKET)'; \
		echo '#define NDS_RENDER_ECONOMY $(NDS_RENDER_ECONOMY)'; \
		echo '#define NDS_RENDER_ECONOMY_OWNER_MASK $(NDS_RENDER_ECONOMY_OWNER_MASK)'; \
		echo '#define NDS_RENDERER_BENCHMARK_MODE $(NDS_RENDERER_BENCHMARK_MODE)'; \
		echo '#define NDS_RENDERER_FAST_RUN_DEFAULT $(NDS_RENDERER_FAST_RUN_DEFAULT)'; \
		echo '#define NDS_SCENE_MIP_CACHE_LAB $(NDS_SCENE_MIP_CACHE_LAB)'; \
		echo '#define NDS_FAST_WALLPAPER_AFFINE $(NDS_FAST_WALLPAPER_AFFINE)'; \
		echo '#define NDS_R2_RESULTS_AFFINE $(NDS_R2_RESULTS_AFFINE)'; \
		echo '#define NDS_R2_RESULTS_LAYER_MEMO $(NDS_R2_RESULTS_LAYER_MEMO)'; \
		echo '#define NDS_R2_MAIN_PRESENT_GUARD $(NDS_R2_MAIN_PRESENT_GUARD)'; \
		echo '#define NDS_R2_FIGHTER_NO_ORACLE $(NDS_R2_FIGHTER_NO_ORACLE)'; \
		echo '#define NDS_BGM_FALSIFIER_OFF $(NDS_BGM_FALSIFIER_OFF)'; \
		echo '#define NDS_RENDERER_BATTLE_STATIC_TEXTURE_DEFAULT $(NDS_RENDERER_BATTLE_STATIC_TEXTURE_DEFAULT)'; \
		echo '#define NDS_IFCOMMON_HYBRID_OAM $(NDS_IFCOMMON_HYBRID_OAM)'; \
		echo '#define NDS_BUILD_HARNESS_VARIANT "$(NDS_DEV_SCENE_HARNESS)"'; \
		echo '#define NDS_DEBUG_HUD $(NDS_DEBUG_HUD)'; \
		echo '#define NDS_AUDIO_FGM_ARM7_ACK_DIAGNOSTICS $(NDS_AUDIO_FGM_ARM7_ACK_DIAGNOSTICS)'; \
		echo '#define NDS_FREEZE_DIAGNOSTICS $(NDS_FREEZE_DIAGNOSTICS)'; \
		echo '#define NDS_TASK9_FLOAT_CENSUS $(NDS_TASK9_FLOAT_CENSUS)'; \
		echo '#define NDS_TASK9_FLOAT_ITCM $(NDS_TASK9_FLOAT_ITCM)'; \
		echo '#define NDS_TASK9_FLOAT_PHASE2 $(NDS_TASK9_FLOAT_PHASE2)'; \
		echo '#define NDS_TASK16_FLOAT_COMPARE $(NDS_TASK16_FLOAT_COMPARE)'; \
		echo '#define NDS_TASK16_FLOAT_I2F $(NDS_TASK16_FLOAT_I2F)'; \
		echo '#define NDS_TASK16_FLOAT_ADDSUB $(NDS_TASK16_FLOAT_ADDSUB)'; \
		echo '#define NDS_TASK9_STATE_HASH $(NDS_TASK9_STATE_HASH)'; \
		echo '#define NDS_TASK10_HARDWARE_CALIBRATION $(NDS_TASK10_HARDWARE_CALIBRATION)'; \
		echo '#define NDS_TASK20_STACK_PROFILE $(NDS_TASK20_STACK_PROFILE)'; \
		echo '#define NDS_TASK32_DRAW_HOT_TEXT $(NDS_TASK32_DRAW_HOT_TEXT)'; \
		echo '#define NDS_FIGHTER_ANIM_AUDIT $(NDS_FIGHTER_ANIM_AUDIT)'; \
		echo '#define NDS_FIGHTER_ANIM_CYCLER_KIND $(NDS_FIGHTER_ANIM_CYCLER_KIND)'; \
		echo '#define NDS_R2_PARTICLE_DRAW $(NDS_R2_PARTICLE_DRAW)'; \
		echo '#define NDS_R2_SHIELD_QUAD $(NDS_R2_SHIELD_QUAD)'; \
		echo '#define NDS_R2_FIREBALL_QUAD $(NDS_R2_FIREBALL_QUAD)'; \
		echo '#define NDS_R2_PARTICLE_CAMERA_CACHE $(NDS_R2_PARTICLE_CAMERA_CACHE)'; \
		echo '#define NDS_R2_CAMERA_MATRIX_LEAN $(NDS_R2_CAMERA_MATRIX_LEAN)'; \
		echo '#define NDS_R2_CAMERA_FIXED $(NDS_R2_CAMERA_FIXED)'; \
		echo '#define NDS_R2_CAMERA_FIXED_TOGGLE $(NDS_R2_CAMERA_FIXED_TOGGLE)'; \
		echo '#define NDS_R2_FOX_BLASTER_QUAD $(NDS_R2_FOX_BLASTER_QUAD)'; \
		echo '#define NDS_R2_FOX_BLASTER_GLOW_AOT $(NDS_R2_FOX_BLASTER_GLOW_AOT)'; \
		echo '#define NDS_R2_FOX_GUN_OVERLAY $(NDS_R2_FOX_GUN_OVERLAY)'; \
		echo '#define NDS_R2_FIREBALL_NATIVE_MAP_COLL $(NDS_R2_FIREBALL_NATIVE_MAP_COLL)'; \
		echo '#define NDS_R2_FIREGRIND_NATIVE $(NDS_R2_FIREGRIND_NATIVE)'; \
		echo '#define NDS_R2_WHISPY_NATIVE_TEXTURES $(NDS_R2_WHISPY_NATIVE_TEXTURES)'; \
		echo '#define NDS_R2_WHISPY_NATIVE_AOT $(NDS_R2_WHISPY_NATIVE_AOT)'; \
		echo '#define NDS_R2_FIREBALL_MAP_COLL_SCALE $(NDS_R2_FIREBALL_MAP_COLL_SCALE)F'; \
		echo '#define NDS_R2_FIREBALL_MAP_COLL_DEBUG $(NDS_R2_FIREBALL_MAP_COLL_DEBUG)'; \
		echo '#define NDS_R2_IMPACT_WAVE_NATIVE $(NDS_R2_IMPACT_WAVE_NATIVE)'; \
		echo '#define NDS_R2_REBIRTH_HALO_NATIVE $(NDS_R2_REBIRTH_HALO_NATIVE)'; \
		echo '#define NDS_R2_REBIRTH_HALO_FULL_OFFLOAD $(NDS_R2_REBIRTH_HALO_FULL_OFFLOAD)'; \
		echo '#define NDS_R2_REBIRTH_HALO_GX_GROUP_MASK $(NDS_R2_REBIRTH_HALO_GX_GROUP_MASK)'; \
		echo '#define NDS_R2_REBIRTH_HALO_SPLIT_MTX $(NDS_R2_REBIRTH_HALO_SPLIT_MTX)'; \
		echo '#define NDS_R2_REBIRTH_HALO_SPLIT_NOZ $(NDS_R2_REBIRTH_HALO_SPLIT_NOZ)'; \
		echo '#define NDS_R2_REBIRTH_HALO_PACKED_FIFO $(NDS_R2_REBIRTH_HALO_PACKED_FIFO)'; \
		echo '#define NDS_R2_REBIRTH_HALO_PACKED_PROJECTED $(NDS_R2_REBIRTH_HALO_PACKED_PROJECTED)'; \
		echo '#define NDS_R2_REBIRTH_HALO_HW_LIGHT $(NDS_R2_REBIRTH_HALO_HW_LIGHT)'; \
		echo '#define NDS_R2_REBIRTH_HALO_PHASE_PROFILE $(NDS_R2_REBIRTH_HALO_PHASE_PROFILE)'; \
		echo '#define NDS_R2_REBIRTH_HALO_FAST_ADAPTER $(NDS_R2_REBIRTH_HALO_FAST_ADAPTER)'; \
		echo '#define NDS_R2_FOX_CPU_DEFAULT $(NDS_R2_FOX_CPU_DEFAULT)'; \
		echo '#define NDS_DEMO_FOX_CPU_LADDER $(NDS_DEMO_FOX_CPU_LADDER)'; \
		echo '#define NDS_R2_COLLISION_L7_ORACLE $(NDS_R2_COLLISION_L7_ORACLE)'; \
		echo '#define NDS_R2_COLLISION_FIXED $(NDS_R2_COLLISION_FIXED)'; \
		echo '#define NDS_R2_COLLISION_FIXED_DISPATCH $(NDS_R2_COLLISION_FIXED_DISPATCH)u'; \
		echo '#define NDS_R2_COLLISION_FIXED_NARROW $(NDS_R2_COLLISION_FIXED_NARROW)'; \
		echo '#define NDS_R2_COLLISION_FIXED_NARROW_DISPATCH $(NDS_R2_COLLISION_FIXED_NARROW_DISPATCH)u'; \
		echo '#define NDS_R2_SIM_MAC_SHADOW $(NDS_R2_SIM_MAC_SHADOW)'; \
		echo '#define NDS_R2_CFX_HWMATH $(NDS_R2_CFX_HWMATH)'; \
		echo '#define NDS_R2_HWMATH_BENCH $(NDS_R2_HWMATH_BENCH)'; \
		echo '#define NDS_R2_HWMATH_ROUTE $(NDS_R2_HWMATH_ROUTE)'; \
		echo '#define NDS_R2_TILESYNC_ROUTE $(NDS_R2_TILESYNC_ROUTE)'; \
		echo '#define NDS_R2_ANIM_Q_ITCM_ON $(NDS_R2_ANIM_Q_ITCM_ON)'; \
		echo '#define NDS_R2_ANIM_ITCM_ROUTE $(NDS_R2_ANIM_ITCM_ROUTE)'; \
		echo '#define NDS_TASK39_FX_SPRITES $(NDS_TASK39_FX_SPRITES)'; \
		echo '#define NDS_TASK39_FX_FLASH $(NDS_TASK39_FX_FLASH)'; \
		echo '#define NDS_R2_PARTICLE_RUNTIME $(NDS_R2_PARTICLE_RUNTIME)'; \
		echo '#define NDS_TASK10_GIT_SHORT "$(NDS_TASK10_GIT_SHORT)"'; \
		echo '#define NDS_IMPORT_BATTLESHIP_FTMAIN $(NDS_IMPORT_BATTLESHIP_FTMAIN)'; \
		echo '#define NDS_IMPORT_BATTLESHIP_FTMANAGER $(NDS_IMPORT_BATTLESHIP_FTMANAGER)'; \
		echo '#define NDS_IMPORT_BATTLESHIP_MPPROCESS_LIVE $(NDS_IMPORT_BATTLESHIP_MPPROCESS_LIVE)'; \
		echo '#define NDS_IMPORT_BATTLESHIP_MPPROCESS_PRIVATE $(NDS_IMPORT_BATTLESHIP_MPPROCESS_PRIVATE)'; \
		echo '#define NDS_IMPORT_BATTLESHIP_FTCOMPUTER $(NDS_IMPORT_BATTLESHIP_FTCOMPUTER)'; \
		echo '#define NDS_IMPORT_BATTLESHIP_NORMAL_MOVESET $(NDS_IMPORT_BATTLESHIP_NORMAL_MOVESET)'; \
		echo '#define NDS_IMPORT_BATTLESHIP_BATTLE_PLAYABLE $(NDS_IMPORT_BATTLESHIP_BATTLE_PLAYABLE)'; \
		echo '#define NDS_IMPORT_BATTLESHIP_VS_RESULTS $(NDS_IMPORT_BATTLESHIP_VS_RESULTS)'; \
		echo '#define NDS_IMPORT_BATTLESHIP_IFCOMMON $(NDS_IMPORT_BATTLESHIP_IFCOMMON)'; \
		echo '#define NDS_IMPORT_BATTLESHIP_WEAPON_MANAGER $(NDS_IMPORT_BATTLESHIP_WEAPON_MANAGER)'; \
		echo '#define NDS_IMPORT_BATTLESHIP_MARIO_FIREBALL $(NDS_IMPORT_BATTLESHIP_MARIO_FIREBALL)'; \
		echo '#define NDS_IMPORT_BATTLESHIP_FOX_BLASTER $(NDS_IMPORT_BATTLESHIP_FOX_BLASTER)'; \
		echo '#define NDS_IMPORT_BATTLESHIP_EFFECT_MANAGER $(NDS_IMPORT_BATTLESHIP_EFFECT_MANAGER)'; \
		echo '#define NDS_IMPORT_BATTLESHIP_FOX_REFLECTOR $(NDS_IMPORT_BATTLESHIP_FOX_REFLECTOR)'; \
		echo '#define NDS_IMPORT_BATTLESHIP_FT_PUBLIC $(NDS_IMPORT_BATTLESHIP_FT_PUBLIC)'; \
		echo '#define NDS_R2_SOURCE_EFFECTS_PARTICLE $(NDS_R2_SOURCE_EFFECTS_PARTICLE)'; \
		echo '#define NDS_R2_POSITION_PROBE $(NDS_R2_POSITION_PROBE)'; \
		echo '#define NDS_R2_EFFECT_POOL $(NDS_R2_EFFECT_POOL)'; \
		echo '#define NDS_R2_KO_STRESS $(NDS_R2_KO_STRESS)'; \
		echo '#define NDS_IMPORT_BATTLESHIP_MARIO_SPECIAL_HI $(NDS_IMPORT_BATTLESHIP_MARIO_SPECIAL_HI)'; \
		echo '#define NDS_IMPORT_BATTLESHIP_MARIO_SPECIAL_LW $(NDS_IMPORT_BATTLESHIP_MARIO_SPECIAL_LW)'; \
		echo '#define NDS_IMPORT_BATTLESHIP_FOX_SPECIAL_HI $(NDS_IMPORT_BATTLESHIP_FOX_SPECIAL_HI)'; \
		echo '#define NDS_IMPORT_BATTLESHIP_AUDIO_ASSETS $(NDS_IMPORT_BATTLESHIP_AUDIO_ASSETS)'; \
		echo '#define NDS_IMPORT_BATTLESHIP_AUDIO_BGM $(NDS_IMPORT_BATTLESHIP_AUDIO_BGM)'; \
		echo '#define NDS_IMPORT_BATTLESHIP_AUDIO_FGM $(NDS_IMPORT_BATTLESHIP_AUDIO_FGM)'; \
		echo '#endif'; \
	} > "$$tmp"; \
	if test -f "$@" && cmp -s "$$tmp" "$@"; then rm "$$tmp"; else mv "$$tmp" "$@"; fi

$(NDS_SCENE_HARNESS_CONFIG): FORCE
	@tmp="$@.tmp"; \
	{ \
		echo '#ifndef NDS_SCENE_HARNESS_CONFIG_H'; \
		echo '#define NDS_SCENE_HARNESS_CONFIG_H'; \
		echo '#define NDS_DEV_SCENE_HARNESS $(NDS_DEV_SCENE_HARNESS_ID)'; \
		echo '#define NDS_ENABLE_INISHIE_SOURCE_SCALE_SETUP $(NDS_ENABLE_INISHIE_SOURCE_SCALE_SETUP)'; \
		echo '#endif'; \
	} > "$$tmp"; \
	if test -f "$@" && cmp -s "$$tmp" "$@"; then rm "$$tmp"; else mv "$$tmp" "$@"; fi

$(NDS_TASK32_DRAW_HOT_FRAGMENT): FORCE
	@tmp="$@.tmp"; \
	{ \
		if test "$(NDS_TASK32_DRAW_HOT_TEXT)" = 1; then \
			echo '*scene_backend.o(.text.ndsRendererAdapterFindDObjWorldMatrix)'; \
			echo '*scene_backend.o(.text.ndsRendererAdapterBuildDObjWorldMatrix)'; \
			echo '*nds_renderer.o(.text.ndsRendererNativeStagePrepareRun.constprop.0)'; \
			echo '*nds_renderer.o(.text.ndsRendererMtxMul20p12)'; \
			echo '*nds_renderer.o(.text.ndsRendererMtxMulAffine20p12)'; \
			echo '*nds_renderer.o(.text.ndsRendererMtxLoadN64ToDS20p12)'; \
			echo '*nds_renderer.o(.text.ndsRendererLoadHardwareMatrixPair.isra.0)'; \
			if test "$(NDS_TASK36_HW_COMPOSE)" = 2; then \
				echo '*nds_renderer.o(.text.ndsRendererTask36ReplayRun)'; \
			else \
				echo '*nds_renderer.o(.text.ndsRendererCommitNativeStageSegment)'; \
			fi; \
			echo '*nds_renderer.o(.text.ndsRendererNativeStageLoadNoZMatrix)'; \
			echo '*nds_renderer.o(.text.ndsRendererNativeStageEmitNoZTriangle)'; \
			echo '*nds_renderer.o(.text.ndsRendererNativeApplyStateDelta.part.0)'; \
			echo '*nds_renderer.o(.text.ndsRendererNativeApplyStateSpan)'; \
			echo '*nds_renderer.o(.text.ndsRendererSyncTextureTile)'; \
		else \
			echo '/* Task 32 draw hot text disabled. */'; \
		fi; \
	} > "$$tmp"; \
	if test -f "$@" && cmp -s "$$tmp" "$@"; then rm "$$tmp"; else mv "$$tmp" "$@"; fi

# The .inc carries only the sheet's path, size and hash; the 22,528 bytes
# themselves are the NitroFS payload, produced by the same invocation.
$(NDS_TASK39_HIT_SPARKS_INC) $(NDS_TASK39_HIT_SPARKS_ASSET) &: \
		$(PROJECT_ROOT)/scripts/2d_vfx/generate_task39_hit_sparks.py \
		$(BATTLESHIP_O2R)/particles/efcommon_particle_scb \
		$(BATTLESHIP_O2R)/particles/efcommon_particle_txb
	python "$(PROJECT_ROOT)/scripts/2d_vfx/generate_task39_hit_sparks.py"

$(NITROFS_DIR)/effects/task39_hit_sparks.rgb5a1.bin: $(NDS_TASK39_HIT_SPARKS_ASSET)
	@mkdir -p $(dir $@)
	@cp $< $@

# R2-07: the packed EFCommon particle bank. The reachable-script set is derived
# from the P1 effect seams in generate_task39_effect_census.py plus the bank's
# own spawn graph, so both scripts and efmanager.c are prerequisites. The header
# and the JSON report are committed; the .inc is a build product under the
# gitignored src/nds/generated, which is why the ELF depends on it.
#
# One generator invocation produces the .inc AND the texel/palette payload, so
# they are a grouped target (`&:`) rather than two rules that would each run it.
# The payload is not linked -- see the header comment: .rodata is taken out of
# the boot-time taskman arena one-for-one and this pack is big enough to push
# that search past its floor.
# The bake varies with NDS_P2_STAGE_YOSTER -- generate_nds_particle_banks.py:114
# reads it from the environment and this Makefile exports it -- while the .inc,
# all three payloads and the committed JSON land on SHARED paths. The per-BUILD
# config header below is therefore not enough on its own, for exactly the reason
# the mn_ui_kit stamp records: bake A touches the shared outputs, and build B
# whose own config header is OLDER then reuses A's bake at a different flag
# value. That is how the committed NDS_PARTICLE_BANKS.generated.json came to
# differ from the BattleShip sources on 2026-09-03. This stamp carries the flag
# value itself and is rewritten only when it changes, so the bake is keyed on
# what varies rather than on a timestamp another build controls.
NDS_PARTICLE_BANKS_FLAGS := $(NDS_P2_STAGE_YOSTER)
NDS_PARTICLE_BANKS_STAMP := $(PROJECT_ROOT)/src/nds/generated/nds_particle_banks.flags.stamp

$(NDS_PARTICLE_BANKS_STAMP): FORCE
	@mkdir -p $(dir $@)
	@printf %s "$(NDS_PARTICLE_BANKS_FLAGS)" > $@.tmp
	@if ! cmp -s $@.tmp $@; then mv -f $@.tmp $@; else rm -f $@.tmp; fi

$(NDS_PARTICLE_BANKS_INC) $(NDS_PARTICLE_TEXTURE_ASSET) $(NDS_PARTICLE_QUAD_ASSET) $(NDS_WHISPY_NATIVE_ASSET) &: \
		$(PROJECT_ROOT)/scripts/generate_nds_particle_banks.py \
		$(PROJECT_ROOT)/scripts/2d_vfx/generate_task39_effect_census.py \
		$(BATTLESHIP_O2R)/particles/efcommon_particle_scb \
		$(BATTLESHIP_O2R)/particles/efcommon_particle_txb \
		$(BATTLESHIP_O2R)/particles/grpupupu_particle_scb \
		$(BATTLESHIP_O2R)/particles/grpupupu_particle_txb \
		$(BATTLESHIP_O2R)/particles/gryoster_particle_scb \
		$(BATTLESHIP_O2R)/particles/gryoster_particle_txb \
		$(NDS_BUILD_CONFIG) \
		$(NDS_PARTICLE_BANKS_STAMP) \
		$(PROJECT_ROOT)/$(BATTLESHIP_DECOMP)/src/ef/efmanager.c
	python "$(PROJECT_ROOT)/scripts/generate_nds_particle_banks.py"

$(NDS_PARTICLE_BANKS_HEADER): $(NDS_PARTICLE_BANKS_INC)

# Slice 32's baked animation bank. `--verify` is not optional here: the emitter
# range-checks every field as it encodes and then decodes the whole blob back
# against the bake, and its failure mode without that is a silently saturated
# field in one joint of one animation -- no crash, no counter, nothing a
# screenshot shows. It costs about thirty seconds against a build that takes
# minutes, and it is the only thing standing between a bank drift and wrong
# animation data shipping.
$(NDS_FTANIM_DENSE_ASSET): \
		$(PROJECT_ROOT)/scripts/generate_ftanim_dense_bank.py \
		$(PROJECT_ROOT)/scripts/ftanim_reloc_probe.py \
		$(PROJECT_ROOT)/scripts/ftanim_script_model.py \
		$(PROJECT_ROOT)/scripts/ftanim_bake.py \
		$(NDS_FTANIM_DENSE_SOURCES)
	@mkdir -p $(dir $@)
	python "$(PROJECT_ROOT)/scripts/generate_ftanim_dense_bank.py" \
		--verify --out "$@"

$(NITROFS_DIR)/animation/ftanim_dense_bank.bin: $(NDS_FTANIM_DENSE_ASSET)
	@mkdir -p $(dir $@)
	@cp $< $@

# The resident pack. `--blob-out` runs the slot-level equivalence check before it
# writes: every DObj slot of every clip is decoded OUT OF THE EMITTED BLOB, at
# the blob's own offsets, and compared command-for-command against the same slot
# decoded from the o2r file through the ROM's own pipeline. It is not optional
# for the same reason the dense bank's --verify is not: a wrong offset in one
# slot of one clip is a garbage script handed to the parser, and the parser's
# failure mode on garbage is a freeze, not a diagnostic.
$(NDS_BATTLEPACK_BLOB): \
		$(PROJECT_ROOT)/scripts/generate_battlepack_anim.py \
		$(PROJECT_ROOT)/scripts/ftanim_reloc_probe.py \
		$(NDS_FTANIM_DENSE_SOURCES)
	@mkdir -p $(dir $@)
	python "$(PROJECT_ROOT)/scripts/generate_battlepack_anim.py" \
		--fighter fox --items-off --blob-out "$@"

$(NITROFS_DIR)/animation/battlepack_fox.bin: $(NDS_BATTLEPACK_BLOB)
	@mkdir -p $(dir $@)
	@cp $< $@

$(NDS_FTANIM_STREAM_ASSET): \
		$(PROJECT_ROOT)/scripts/generate_battlepack_anim.py \
		$(PROJECT_ROOT)/scripts/ftanim_reloc_probe.py \
		$(NDS_FTANIM_STREAM_SOURCES)
	@mkdir -p $(dir $@)
	python "$(PROJECT_ROOT)/scripts/generate_battlepack_anim.py" \
		--stream-out "$@"

$(NITROFS_DIR)/zz_stream/ftanim_stream_pack.bin: $(NDS_FTANIM_STREAM_ASSET)
	@mkdir -p $(dir $@)
	@cp $< $@

# The dense runtime's rows. `--verify` is deliberately NOT passed (it is the
# 90-second three-layer corpus proof and belongs to a checker run, not to every
# build), but LAYER D always runs inside `--emit-c` and the generator returns
# non-zero on it: every emitted figatree entry is decoded back out of the rows
# at the offset the directory publishes and compared against the o2r script that
# entry actually points at. A pack that binds joint 7 to joint 8's script is an
# animation that plays on the wrong bone with no crash and no counter, so this
# is not optional for the same reason the battlepack's slot check is not.
$(NDS_FTANIM_TRACK_HEADER): \
		$(PROJECT_ROOT)/scripts/generate_ftanim_track_pack.py \
		$(PROJECT_ROOT)/scripts/ftanim_reloc_probe.py \
		$(PROJECT_ROOT)/scripts/ftanim_script_model.py \
		$(NDS_FTANIM_DENSE_SOURCES)
	@mkdir -p $(dir $@)
	python "$(PROJECT_ROOT)/scripts/generate_ftanim_track_pack.py" \
		--items-off --fighter fox --emit-c "$@" \
		--max-bytes $(NDS_FTANIM_TRACK_MAX_BYTES)

$(NITROFS_DIR)/particles/efcommon_particle_textures.ds.bin: $(NDS_PARTICLE_TEXTURE_ASSET)
	@mkdir -p $(dir $@)
	@cp $< $@

$(NITROFS_DIR)/particles/efcommon_particle_quads.a5i3.bin: $(NDS_PARTICLE_QUAD_ASSET)
	@mkdir -p $(dir $@)
	@cp $< $@

$(NITROFS_DIR)/particles/grpupupu_whispy_native.ds.bin: $(NDS_WHISPY_NATIVE_ASSET)
	@mkdir -p $(dir $@)
	@cp $< $@

prune-obsolete-audio:
	@rm -f $(foreach file,$(NDS_AUDIO_OBSOLETE_DERIVED_FILES),$(NITROFS_DIR)/$(file))

.PHONY: prune-streamed-ftanim
prune-streamed-ftanim:
ifeq ($(NDS_R2_FTANIM_STREAM),1)
	@rm -f \
		$(NITROFS_DIR)/reloc/reloc_animations/FTMarioAnim* \
		$(NITROFS_DIR)/reloc/reloc_animations/FTFoxAnim* \
		$(NITROFS_DIR)/reloc/reloc_animations/FTLuigiAnim* \
		$(NITROFS_DIR)/reloc/reloc_animations/FTDonkeyAnim* \
		$(NITROFS_DIR)/reloc/reloc_animations/FTSamusAnim* \
		$(NITROFS_DIR)/reloc/reloc_animations/FTCaptainAnim* \
		$(NITROFS_DIR)/animation/ftanim_stream_pack.bin
endif

# The prune must finish before Make decides whether any retained reloc target
# needs its normal copy recipe. This also prevents two concurrent cp processes
# racing on the named AObj32 files in an incremental build.
$(NDS_NITROFS_RELOC_FILES): | prune-streamed-ftanim

$(OUTPUT).nds: prune-obsolete-audio prune-streamed-ftanim $(OUTPUT).elf $(NDS_NITROFS_RELOC_FILES) $(NDS_NITROFS_RELOCDATA_FILES) $(NDS_NITROFS_AUDIO_FILES) $(NDS_NITROFS_BATTLE_STATIC_TEXTURE_FILES) $(NDS_NITROFS_PARTICLE_FILES) $(NDS_NITROFS_EFFECT_FILES) $(NDS_NITROFS_FTANIM_FILES) $(NDS_NITROFS_BATTLEPACK_FILES) $(NDS_NITROFS_MN_UI_KIT_FILES) $(NDS_NITROFS_NATIVE_IMAGE_FILES) $(NDS_BANNER_ICON)
$(OUTPUT).elf: $(OFILES) $(NDS_PRIVATE_CHECK_OFILES) \
	$(NDS_HOT_TEXT_SPECS) $(NDS_HOT_TEXT_LINKER_SCRIPT) \
	$(NDS_TASK32_DRAW_HOT_FRAGMENT) $(NDS_PARTICLE_BANKS_INC) \
	$(NDS_BATTLE_STATIC_TEXTURE_INC) $(NDS_ENTRY_EFFECT_INC) \
	$(NDS_NATIVE_STAGE_OWNER_INC) $(NDS_NATIVE_STAGE_YOSTER_INC) \
	$(NDS_NATIVE_STAGE_JUNGLE_INC) $(NDS_NATIVE_STAGE_CASTLE_INC) \
	$(NDS_NATIVE_STAGE_SECTOR_INC) $(NDS_NATIVE_STAGE_HYRULE_INC) \
	$(NDS_NATIVE_STAGE_INISHIE_INC) \
	$(NDS_NATIVE_STAGE_ZEBES_INC) \
	$(NDS_NATIVE_STAGE_YAMABUKI_INC) \
	$(NDS_NATIVE_STAGE_PUPUPUSMALL_INC) \
	$(NDS_NATIVE_STAGE_YOSTERSMALL_INC) \
	$(NDS_NATIVE_STAGE_METAL_INC) \
	$(NDS_NATIVE_STAGE_ZAKO_INC) \
	$(NDS_NATIVE_STAGE_LAST_INC) \
	$(if $(filter 1,$(NDS_IMPORT_BATTLESHIP_IFCOMMON)),$(NDS_BATTLE_HUD_INC)) \
	$(if $(filter 1,$(NDS_P2_UI_KIT)),$(NDS_MN_UI_KIT_INC) \
		$(NDS_MN_TITLE_ANIM_INC))
$(OFILES) $(NDS_PRIVATE_CHECK_OFILES): $(PROJECT_ROOT)/Makefile $(NDS_BUILD_CONFIG) $(NDS_FTANIM_TRACK_PREREQ)
# EVERY OBJECT THAT INCLUDES A GENERATED HEADER NAMES IT HERE, AND THE .d FILE
# IS NOT A SUBSTITUTE. The rules above spell these paths with $(PROJECT_ROOT),
# which MSYS make expands to `/d/Stuff/...`, while gcc -MMD writes
# `D:/Stuff/...` into the .d -- two DIFFERENT nodes in make's graph, so the .d
# edge carries no ordering. Builds are parallel, so the generator and the
# compile of its consumer are siblings under $(OUTPUT).elf and race.
#
# MEASURED, 2026-08-25 (row P2-3f8): a UI-kit bake change produced
# mn_surfaces.bin and mn_ui_kit.generated.inc at 12:23 while nds_ui_kit.o
# stayed at 12:14, and the ROM linked from them failed p2_shell_loop with
# `BACKDROP SURFACES: mismatch=145` -- the stale metric table's per-surface
# FNV against the fresh pack. The very next `make` with no source change
# recompiled the file, which is the proof it was an ordering race and not a
# bad bake. The failure mode is silent wrong art whenever the runtime does
# not happen to hash-check the asset, so add a line here with any new
# generated include rather than trusting -MMD.
nds_ui_kit.o: $(NDS_MN_UI_KIT_INC) $(NDS_MN_TITLE_ANIM_INC)
nds_menu_shell.o: $(NDS_MN_UI_KIT_INC)
nds_battle_hud.o: $(NDS_BATTLE_HUD_INC)
battle_playable_static_textures.o: $(NDS_BATTLE_STATIC_TEXTURE_INC)
nds_particle_banks.o: $(NDS_PARTICLE_BANKS_INC)
nds_renderer.o: $(NDS_ENTRY_EFFECT_INC) $(NDS_PARTICLE_BANKS_INC)
# The outer build exports NDS_NITROFS_RELOC_FILES so the recursive inner make
# receives the exact ROM prerequisite inventory.  P2-3's staged fighter banks
# make that one variable roughly 72 KiB; together with the normal build
# environment it is large enough that MSYS bash can start, but a native
# devkitARM child spawned *through* bash fails with ENOENT on Windows.  Normal
# compile recipes do not cross that shell seam.  Keep the prerequisite export
# intact and scrub only this oversized make-only variable when a shell must
# launch a native tool for the private ITCM archive work below.
NDS_SHELL_DEVKIT_ENV := env -u NDS_NITROFS_RELOC_FILES
ifeq ($(NDS_TASK9_FLOAT_ITCM),1)
NDS_TASK9_FLOAT_LIBGCC := $(shell $(NDS_SHELL_DEVKIT_ENV) $(CC) $(ARCH) -print-libgcc-file-name)
NDS_TASK9_FLOAT_AR := $(shell $(NDS_SHELL_DEVKIT_ENV) $(CC) -print-prog-name=ar)
# Keep the installed archive out of Make's prerequisite graph: `make -B` would
# otherwise try to rebuild that external .a through an implicit archive rule.
# One grouped recipe makes one verified private copy and extracts only from it.
$(NDS_TASK9_FLOAT_ITCM_OFILES) &: $(PROJECT_ROOT)/Makefile $(NDS_BUILD_CONFIG)
	@echo "$(NDS_TASK9_FLOAT_LIBGCC_SHA256) *$(NDS_TASK9_FLOAT_LIBGCC)" | sha256sum -c -
	@# Remove BOTH spellings, not just the ones this configuration emits. A build
	@# directory that previously held `<stem>.itcm.o` keeps it when the member
	@# moves to `<stem>.mainram.o`, and the two then look like one member
	@# claiming two placements. check-task9-float-itcm.ps1 fails closed on that,
	@# which is how this was caught rather than shipped.
	@rm -rf ".task9-float-itcm" $(foreach member,$(NDS_TASK9_FLOAT_ITCM_MEMBERS),$(basename $(member)).itcm.o $(basename $(member)).mainram.o)
	@mkdir -p ".task9-float-itcm"
	@cp "$(NDS_TASK9_FLOAT_LIBGCC)" ".task9-float-itcm/libgcc.a"
	@cd ".task9-float-itcm" && $(NDS_SHELL_DEVKIT_ENV) $(NDS_TASK9_FLOAT_AR) x "libgcc.a" $(NDS_TASK9_FLOAT_ITCM_MEMBERS)
	@for member in $(NDS_TASK9_FLOAT_ITCM_MEMBERS); do \
		stem="$${member%.o}"; \
		phase2_filter=""; \
		if test "$(NDS_TASK9_FLOAT_PHASE2)" = "1" && test "$$member" = "_arm_cmpsf2.o"; then \
			phase2_filter="--redefine-sym __aeabi_fcmpeq=__nds_task9_libgcc_fcmpeq_golden"; \
		fi; \
		if test "$(NDS_TASK16_FLOAT_COMPARE)" = "1" && test "$$member" = "_arm_cmpsf2.o"; then \
			phase2_filter="$$phase2_filter --redefine-sym __aeabi_fcmplt=__nds_task16_libgcc_fcmplt_golden --redefine-sym __aeabi_fcmple=__nds_task16_libgcc_fcmple_golden --redefine-sym __aeabi_fcmpge=__nds_task16_libgcc_fcmpge_golden --redefine-sym __aeabi_fcmpgt=__nds_task16_libgcc_fcmpgt_golden"; \
		fi; \
		if test "$(NDS_TASK16_FLOAT_COMPARE)" = "1" && test "$$member" = "_arm_unordsf2.o"; then \
			phase2_filter="$$phase2_filter --redefine-sym __aeabi_fcmpun=__nds_task16_libgcc_fcmpun_golden"; \
		fi; \
		if test "$(NDS_TASK16_FLOAT_ADDSUB)" = "1" && test "$$member" = "_arm_addsubsf3.o"; then \
			phase2_filter="$$phase2_filter --redefine-sym __aeabi_fadd=__nds_task16_libgcc_fadd_golden --redefine-sym __aeabi_fsub=__nds_task16_libgcc_fsub_golden"; \
		fi; \
		if test "$(NDS_TASK16_FLOAT_I2F)" = "1" && test "$$member" = "_arm_addsubsf3.o"; then \
			phase2_filter="$$phase2_filter --redefine-sym __aeabi_i2f=__nds_task16_libgcc_i2f_golden"; \
		fi; \
		rename_filter="--rename-section .text=.itcm,alloc,load,readonly,code,contents"; \
		out="$$stem.itcm.o"; \
		case " $(strip $(NDS_TASK9_FLOAT_MAIN_MEMBERS)) " in \
			*" $$member "*) rename_filter=""; out="$$stem.mainram.o";; \
		esac; \
		$(NDS_SHELL_DEVKIT_ENV) $(OBJCOPY) $$phase2_filter $$rename_filter \
			".task9-float-itcm/$$member" "$$out" || exit $$?; \
	done
	@rm -rf ".task9-float-itcm"
endif
ifneq ($(strip $(NDS_TASK37_ITCM_OFILES)),)
NDS_TASK37_LIBC := $(shell $(NDS_SHELL_DEVKIT_ENV) $(CC) $(ARCH) -print-file-name=libc.a)
NDS_TASK37_LIBM := $(shell $(NDS_SHELL_DEVKIT_ENV) $(CC) $(ARCH) -print-file-name=libm.a)
NDS_TASK37_AR := $(shell $(NDS_SHELL_DEVKIT_ENV) $(CC) -print-prog-name=ar)
# Grouped recipe, one verified private copy per archive, extract only the named
# members. Same shape as the Task 9 block above and for the same reason: the
# installed archives must stay out of Make's prerequisite graph.
$(NDS_TASK37_ITCM_OFILES) &: $(PROJECT_ROOT)/Makefile $(NDS_BUILD_CONFIG)
	@test "$(NDS_TASK37_ITCM_LIBC)" != "1" || echo "$(NDS_TASK37_LIBC_SHA256) *$(NDS_TASK37_LIBC)" | sha256sum -c -
	@test "$(NDS_TASK37_ITCM_LIBM)" != "1" || echo "$(NDS_TASK37_LIBM_SHA256) *$(NDS_TASK37_LIBM)" | sha256sum -c -
	@rm -rf ".task37-itcm" $(NDS_TASK37_ITCM_OFILES)
	@mkdir -p ".task37-itcm"
	@cp "$(NDS_TASK37_LIBC)" ".task37-itcm/libc.a"
	@cp "$(NDS_TASK37_LIBM)" ".task37-itcm/libm.a"
	@test "$(NDS_TASK37_ITCM_LIBC)" != "1" || (cd ".task37-itcm" && $(NDS_SHELL_DEVKIT_ENV) $(NDS_TASK37_AR) x "libc.a" $(NDS_TASK37_LIBC_MEMBERS))
	@test "$(NDS_TASK37_ITCM_LIBM)" != "1" || (cd ".task37-itcm" && $(NDS_SHELL_DEVKIT_ENV) $(NDS_TASK37_AR) x "libm.a" $(NDS_TASK37_LIBM_MEMBERS))
	@for member in $(if $(filter 1,$(NDS_TASK37_ITCM_LIBC)),$(NDS_TASK37_LIBC_MEMBERS)) $(if $(filter 1,$(NDS_TASK37_ITCM_LIBM)),$(NDS_TASK37_LIBM_MEMBERS)); do \
		stem="$${member%.o}"; \
		$(NDS_SHELL_DEVKIT_ENV) $(OBJCOPY) \
			--rename-section .text=.itcm,alloc,load,readonly,code,contents \
			".task37-itcm/$$member" "$$stem.itcm.o" || exit $$?; \
	done
	@rm -rf ".task37-itcm"
endif
ifneq ($(strip $(NDS_MPPROCESS_STRICT_OFILES)),)
$(NDS_MPPROCESS_STRICT_OFILES): CFLAGS += -Werror=implicit-function-declaration -Werror=incompatible-pointer-types -Werror=int-conversion -Werror=return-type
endif
# The measured renderer is cache-resident on retail hardware and wins in ARM
# state despite melonDS's main-RAM fetch model.
#
# Keyed on the LATENCY SURFACES, not on harness 163 alone. Until 2026-07-30 this
# read `ifeq ($(NDS_DEV_SCENE_HARNESS_ID),163)`, so `results_playable` (164) --
# added specifically so R2-07's Results numbers would be comparable with the
# battle ones -- built this TU `-mthumb` while every battle ROM built it `-marm`.
# ARMv5TE Thumb has no SMULL, so all the 20.12 fixed-point matrix and vertex
# math became `bl __aeabi_lmul`: the Results census put that helper at 7.79% of
# the frame, third behind the idle spin and the fighter root, out of 86 bytes.
# Restoring the flag is worth -511,174 ticks per Results source tic (-22.7%).
#
# The harness block near line 884 states that `results_playable` must differ
# from the tick-HUD ROM "in the scene it boots and in NOTHING else"; keying a
# codegen flag on the harness ID silently broke that promise, so any new latency
# surface belongs in this list rather than in a new ID comparison.
NDS_ARM_RENDERER_HARNESS_IDS := 163 164
ifneq ($(filter $(NDS_DEV_SCENE_HARNESS_ID),$(NDS_ARM_RENDERER_HARNESS_IDS)),)
nds_renderer.o: CFLAGS += -marm
endif

# R2-07 L7a REFUTED 2026-07-31, do not re-propose. Building
# battleship_gmcollision.o -marm -- the same move that was worth -511,174 ticks
# per Results tic for nds_renderer.o just above -- measured WORK-H P95
# 1,147,200 -> 1,144,896, i.e. -2,304, inside the +/-5,376 cross-build floor and
# far under E11's ~16,000 bar. SRC, the bucket it was aimed at, went the wrong
# way by the same 2,304. Noise.
#
# The reason it does not transfer: the renderer's win was SMULL, which ARMv5TE
# Thumb lacks, so every 20.12 multiply became `bl __aeabi_lmul` in the CALLER.
# gmcollision.c has no doubles and no unsuffixed literals, so its float work is
# genuine f32 helper calls -- and those helpers are libgcc code whose own mode
# the caller's flag does not change. -marm can only buy the call sites, and
# there is not enough there to clear the floor. The soft-float cost L6 measured
# has to be removed by not doing the float arithmetic (L7), not by recompiling
# it.
#
# nds_r2_collision_fixed.o IS that change -- the whole-cluster fixed-point
# narrow phase -- and it is the case where the flag is load-bearing rather than
# marginal. Its arithmetic is 64-bit integer products (60 SMULL/SMLAL measured),
# ARMv5TE Thumb has no SMULL, and -mthumb would turn every one of them into
# `bl __aeabi_lmul`; the renderer's rule just above exists for exactly that
# reason and was worth -511,174 ticks per Results tic. L7a's refutation does not
# transfer, and its own paragraph says why: it was measuring FLOAT code, whose
# helpers are libgcc and do not change mode with the caller.
# scripts/check-r2-collision-fixed.ps1 fails on any __aeabi_lmul in this object,
# so losing this line is a red verifier rather than a silent regression.
#
# src/port/nds_r2_collision_ring.c, the wiring, is deliberately NOT in this
# list. It is Thumb glue -- control flow, f32 loads and f32 stores -- that calls
# the ndsR2CollisionFixed* entry points here, so every 64-bit product in the
# cluster stays inside the one object the check script disassembles. Building
# the ring -marm too would grow it for nothing; inlining the kernels INTO it
# would put SMULL-shaped code in a Thumb object and no gate would notice.
nds_r2_collision_fixed.o: CFLAGS += -marm
# Same rule, same reason: src/port/nds_r2_sim_mac_fixed.c holds the shadow
# bodies of the warm-MAC exchange-rate instrument, and its whole content is
# 64-bit integer products. A Thumb build of it would price __aeabi_lmul at 4.49
# cyc/multiply instead of a hardware umull at 2.00 and report an exchange rate
# that no shipped conversion would ever see.
nds_r2_sim_mac_fixed.o: CFLAGS += -marm
# Same rule for the same reason, and here it is also the THING BEING MEASURED:
# one of this object's arms is include/nds/nds_r2_sqrtf.h compiled in ARM state,
# against the shipped Thumb build of the identical header in
# src/nds/r2/nds_r2_sqrtf.c. Building this object -mthumb would make both arms
# call __aeabi_lmul and the comparison would read zero.
nds_r2_hwmath_bench.o: CFLAGS += -marm
# Same rule, and this one is the SHIPPED half of it. include/nds/nds_r2_sqrtf.h
# line 73 needs a 48-bit `root * root`; ARMv5TE Thumb has no UMULL, so in Thumb
# state that one expression is `bl __aeabi_lmul` -- visible at 0x0208b10c in
# build-c206-shipgx0's own disassembly. src/nds/r2/nds_r2_sqrtf.c was the last
# R2 kernel object still built -mthumb. -marm costs +24 B of text and takes the
# 11,608 tk/fr sqrtf lane (8,068 sim + 3,540 draw at marginal-80) down by
# 37%-41%. Bit-identical, not merely equivalent: build-c213-hwmath4's
# gNdsR2HwMathBenchSqrtfMismatch is 0 over 65,536 inputs comparing exactly these
# two builds of the header.
#
# EXCEPT in the route build, where this object IS THE CONTROL ARM and must stay
# -mthumb while nds_r2_sqrtf_arm.o carries the -marm body. Building both -marm
# would make the two arms identical and the measurement would read zero -- the
# same trap the bench block above records.
ifeq ($(NDS_R2_HWMATH_ROUTE),1)
nds_r2_sqrtf_arm.o: CFLAGS += -marm
else
nds_r2_sqrtf.o: CFLAGS += -marm
endif

scene_backend.o: $(SCENE_BACKEND_SLICES) $(NDS_SCENE_HARNESS_CONFIG)
scene_harness.o battleship_grinishie_scale.o: $(NDS_SCENE_HARNESS_CONFIG)
nds_ifcommon_oam.o: $(NDS_TASK39_HIT_SPARKS_INC)

$(NDS_BATTLESHIP_IMPORT_OVERLAY_STAMP): \
		$(NDS_BATTLESHIP_IMPORT_OVERLAY_GENERATOR) \
		$(NDS_BATTLESHIP_IMPORT_OVERLAY_PATCHES) \
		$(NDS_BATTLESHIP_IMPORT_OVERLAY_INPUTS)
	pwsh -NoProfile -ExecutionPolicy Bypass -File "$(NDS_BATTLESHIP_IMPORT_OVERLAY_GENERATOR)" \
		-OutputRoot "$(NDS_BATTLESHIP_IMPORT_OVERLAY)"

$(NDS_BATTLESHIP_IMPORT_OVERLAY_OFILES): $(NDS_BATTLESHIP_IMPORT_OVERLAY_STAMP)


$(NITROFS_DIR)/reloc/%: $(BATTLESHIP_O2R)/%
	@mkdir -p $(dir $@)
	@cp $< $@

$(NITROFS_DIR)/relocdata/us/%: $(BATTLESHIP_RELOCDATA)/%
	@mkdir -p $(dir $@)
	@cp $< $@

$(NITROFS_DIR)/audio/bgm_pupupu_ima.bin: $(PROJECT_ROOT)/assets/audio/bgm_pupupu_ima.bin
	@mkdir -p $(dir $@)
	@cp $< $@

# P2-4 Yoster BGM: same shape as the per-track rules above; gated on the
# stage flag so the default NitroFS set is unchanged.
ifeq ($(NDS_P2_STAGE_YOSTER),1)
$(NITROFS_DIR)/audio/bgm_yoster_ima.bin: $(PROJECT_ROOT)/assets/audio/bgm_yoster_ima.bin
	@mkdir -p $(dir $@)
	@cp $< $@
endif

ifeq ($(NDS_P2_STAGE_CASTLE),1)
$(NITROFS_DIR)/audio/bgm_castle_ima.bin: $(PROJECT_ROOT)/assets/audio/bgm_castle_ima.bin
	@mkdir -p $(dir $@)
	@cp $< $@
endif

ifeq ($(NDS_P2_STAGE_ZEBES),1)
$(NITROFS_DIR)/audio/bgm_zebes_ima.bin: $(PROJECT_ROOT)/assets/audio/bgm_zebes_ima.bin
	@mkdir -p $(dir $@)
	@cp $< $@
endif

ifeq ($(NDS_P2_STAGE_HYRULE),1)
$(NITROFS_DIR)/audio/bgm_hyrule_ima.bin: $(PROJECT_ROOT)/assets/audio/bgm_hyrule_ima.bin
	@mkdir -p $(dir $@)
	@cp $< $@
endif

ifeq ($(NDS_P2_STAGE_YAMABUKI),1)
$(NITROFS_DIR)/audio/bgm_yamabuki_ima.bin: $(PROJECT_ROOT)/assets/audio/bgm_yamabuki_ima.bin
	@mkdir -p $(dir $@)
	@cp $< $@
endif

ifeq ($(NDS_P2_STAGE_INISHIE),1)
$(NITROFS_DIR)/audio/bgm_inishie_ima.bin: $(PROJECT_ROOT)/assets/audio/bgm_inishie_ima.bin
	@mkdir -p $(dir $@)
	@cp $< $@
endif

ifeq ($(NDS_P2_STAGE_JUNGLE),1)
$(NITROFS_DIR)/audio/bgm_jungle_ima.bin: $(PROJECT_ROOT)/assets/audio/bgm_jungle_ima.bin
	@mkdir -p $(dir $@)
	@cp $< $@
endif

ifeq ($(NDS_P2_STAGE_SECTOR),1)
$(NITROFS_DIR)/audio/bgm_sector_ima.bin: $(PROJECT_ROOT)/assets/audio/bgm_sector_ima.bin
	@mkdir -p $(dir $@)
	@cp $< $@
endif

$(NITROFS_DIR)/audio/bgm_win_mario_ima.bin: $(PROJECT_ROOT)/assets/audio/bgm_win_mario_ima.bin
	@mkdir -p $(dir $@)
	@cp $< $@

$(NITROFS_DIR)/audio/bgm_win_fox_ima.bin: $(PROJECT_ROOT)/assets/audio/bgm_win_fox_ima.bin
	@mkdir -p $(dir $@)
	@cp $< $@

$(NITROFS_DIR)/audio/bgm_results_ima.bin: $(PROJECT_ROOT)/assets/audio/bgm_results_ima.bin
	@mkdir -p $(dir $@)
	@cp $< $@

$(NITROFS_DIR)/audio/bgm_mode_select_ima.bin: $(PROJECT_ROOT)/assets/audio/bgm_mode_select_ima.bin
	@mkdir -p $(dir $@)
	@cp $< $@

$(NITROFS_DIR)/audio/bgm_battle_select_ima.bin: $(PROJECT_ROOT)/assets/audio/bgm_battle_select_ima.bin
	@mkdir -p $(dir $@)
	@cp $< $@

$(NITROFS_DIR)/audio/fgm_phase_pack_ima.bin: $(PROJECT_ROOT)/assets/audio/fgm_phase_pack_ima.bin
	@mkdir -p $(dir $@)
	@cp $< $@

# One generator invocation produces the metadata .inc AND the RGB555+A1 payload,
# so they are a grouped target (`&:`) exactly like the particle bank above --
# two separate rules would each run the generator. The census script is a
# prerequisite because generate_battle_playable_static_textures.py imports it
# (see its own comment at generate_battle_playable_texture_census.py:258): when
# the census digest moved on 2026-08-05 the .inc's provenance stamp moved with
# it, and nothing rebuilt. The generator verifies its own decomp inputs against
# pinned sha256s, so corpus drift still fails closed and loudly rather than
# being silently absorbed here.
#
# The `touch` is required, not cosmetic: the generator is deliberately
# write-if-changed (generate_battle_playable_static_textures.py:1652 rewrites
# only when the bytes differ), so a run that confirms the outputs are already
# correct leaves both mtimes untouched. Without the touch the targets stay older
# than their prerequisites and make re-runs the whole generator on EVERY build
# forever -- correct output, wasted minutes.
$(NDS_BATTLE_STATIC_TEXTURE_INC) $(NDS_BATTLE_STATIC_TEXTURE_ASSET) &: \
		$(PROJECT_ROOT)/scripts/generate_battle_playable_static_textures.py \
		$(PROJECT_ROOT)/scripts/generate_battle_playable_texture_census.py
	python "$(PROJECT_ROOT)/scripts/generate_battle_playable_static_textures.py" --repo-root "$(PROJECT_ROOT)"
	@touch $(NDS_BATTLE_STATIC_TEXTURE_INC) $(NDS_BATTLE_STATIC_TEXTURE_ASSET)

# P2-4n1. The native stage packets, one include per baked stage. Same shape
# and the same reason as the block above: until 2026-09-04 only build.ps1 ran
# this generator, so an incremental `make` linked whatever include was on disk
# and a fresh clone had none at all (both are gitignored). The generator fails
# closed on a descriptor pin mismatch (generate_nds_native_stage.py:4534), so
# a stale descriptor stops the build here instead of reaching a ROM. Dream
# Land is the generator's default stage. Yoster's include is generated
# unconditionally so it cannot go stale behind NDS_P2_STAGE_YOSTER; the
# translation unit includes it only when that flag is 1.
NDS_NATIVE_STAGE_OWNER_INC := $(PROJECT_ROOT)/src/nds/nds_native_stage_owner.generated.inc
NDS_NATIVE_STAGE_YOSTER_INC := $(PROJECT_ROOT)/src/nds/nds_native_stage_yoster.generated.inc
NDS_NATIVE_STAGE_JUNGLE_INC := $(PROJECT_ROOT)/src/nds/nds_native_stage_jungle.generated.inc
NDS_NATIVE_STAGE_CASTLE_INC := $(PROJECT_ROOT)/src/nds/nds_native_stage_castle.generated.inc
NDS_NATIVE_STAGE_SECTOR_INC := $(PROJECT_ROOT)/src/nds/nds_native_stage_sector.generated.inc
NDS_NATIVE_STAGE_HYRULE_INC := $(PROJECT_ROOT)/src/nds/nds_native_stage_hyrule.generated.inc
NDS_NATIVE_STAGE_INISHIE_INC := $(PROJECT_ROOT)/src/nds/nds_native_stage_inishie.generated.inc
NDS_NATIVE_STAGE_ZEBES_INC := $(PROJECT_ROOT)/src/nds/nds_native_stage_zebes.generated.inc
NDS_NATIVE_STAGE_YAMABUKI_INC := $(PROJECT_ROOT)/src/nds/nds_native_stage_yamabuki.generated.inc
NDS_NATIVE_STAGE_PUPUPUSMALL_INC := $(PROJECT_ROOT)/src/nds/nds_native_stage_pupupusmall.generated.inc
NDS_NATIVE_STAGE_YOSTERSMALL_INC := $(PROJECT_ROOT)/src/nds/nds_native_stage_yostersmall.generated.inc
NDS_NATIVE_STAGE_METAL_INC := $(PROJECT_ROOT)/src/nds/nds_native_stage_metal.generated.inc
NDS_NATIVE_STAGE_ZAKO_INC := $(PROJECT_ROOT)/src/nds/nds_native_stage_zako.generated.inc
NDS_NATIVE_STAGE_LAST_INC := $(PROJECT_ROOT)/src/nds/nds_native_stage_last.generated.inc
NDS_NATIVE_STAGE_GENERATOR_PREREQ := \
	$(PROJECT_ROOT)/scripts/stages/generate_nds_native_stage.py \
	$(PROJECT_ROOT)/scripts/stages/native_matrix_math.py \
	$(wildcard $(PROJECT_ROOT)/scripts/stages/native_stage_descriptors/*.py)
$(NDS_NATIVE_STAGE_OWNER_INC): $(NDS_NATIVE_STAGE_GENERATOR_PREREQ)
	python "$(PROJECT_ROOT)/scripts/stages/generate_nds_native_stage.py" --repo-root "$(PROJECT_ROOT)"
	@touch $(NDS_NATIVE_STAGE_OWNER_INC)
$(NDS_NATIVE_STAGE_YOSTER_INC): $(NDS_NATIVE_STAGE_GENERATOR_PREREQ)
	python "$(PROJECT_ROOT)/scripts/stages/generate_nds_native_stage.py" --repo-root "$(PROJECT_ROOT)" --stage yoster
	@touch $(NDS_NATIVE_STAGE_YOSTER_INC)
$(NDS_NATIVE_STAGE_JUNGLE_INC): $(NDS_NATIVE_STAGE_GENERATOR_PREREQ)
	python "$(PROJECT_ROOT)/scripts/stages/generate_nds_native_stage.py" --repo-root "$(PROJECT_ROOT)" --stage jungle
	@touch $(NDS_NATIVE_STAGE_JUNGLE_INC)
$(NDS_NATIVE_STAGE_CASTLE_INC): $(NDS_NATIVE_STAGE_GENERATOR_PREREQ)
	python "$(PROJECT_ROOT)/scripts/stages/generate_nds_native_stage.py" --repo-root "$(PROJECT_ROOT)" --stage castle
	@touch $(NDS_NATIVE_STAGE_CASTLE_INC)
$(NDS_NATIVE_STAGE_SECTOR_INC): $(NDS_NATIVE_STAGE_GENERATOR_PREREQ)
	python "$(PROJECT_ROOT)/scripts/stages/generate_nds_native_stage.py" --repo-root "$(PROJECT_ROOT)" --stage sector
	@touch $(NDS_NATIVE_STAGE_SECTOR_INC)
$(NDS_NATIVE_STAGE_HYRULE_INC): $(NDS_NATIVE_STAGE_GENERATOR_PREREQ)
	python "$(PROJECT_ROOT)/scripts/stages/generate_nds_native_stage.py" --repo-root "$(PROJECT_ROOT)" --stage hyrule
	@touch $(NDS_NATIVE_STAGE_HYRULE_INC)
$(NDS_NATIVE_STAGE_INISHIE_INC): $(NDS_NATIVE_STAGE_GENERATOR_PREREQ)
	python "$(PROJECT_ROOT)/scripts/stages/generate_nds_native_stage.py" --repo-root "$(PROJECT_ROOT)" --stage inishie
	@touch $(NDS_NATIVE_STAGE_INISHIE_INC)
$(NDS_NATIVE_STAGE_ZEBES_INC): $(NDS_NATIVE_STAGE_GENERATOR_PREREQ)
	python "$(PROJECT_ROOT)/scripts/stages/generate_nds_native_stage.py" --repo-root "$(PROJECT_ROOT)" --stage zebes
	@touch $(NDS_NATIVE_STAGE_ZEBES_INC)
$(NDS_NATIVE_STAGE_YAMABUKI_INC): $(NDS_NATIVE_STAGE_GENERATOR_PREREQ)
	python "$(PROJECT_ROOT)/scripts/stages/generate_nds_native_stage.py" --repo-root "$(PROJECT_ROOT)" --stage yamabuki
	@touch $(NDS_NATIVE_STAGE_YAMABUKI_INC)
$(NDS_NATIVE_STAGE_PUPUPUSMALL_INC): $(NDS_NATIVE_STAGE_GENERATOR_PREREQ)
	python "$(PROJECT_ROOT)/scripts/stages/generate_nds_native_stage.py" --repo-root "$(PROJECT_ROOT)" --stage pupupusmall
	@touch $(NDS_NATIVE_STAGE_PUPUPUSMALL_INC)
$(NDS_NATIVE_STAGE_YOSTERSMALL_INC): $(NDS_NATIVE_STAGE_GENERATOR_PREREQ)
	python "$(PROJECT_ROOT)/scripts/stages/generate_nds_native_stage.py" --repo-root "$(PROJECT_ROOT)" --stage yostersmall
	@touch $(NDS_NATIVE_STAGE_YOSTERSMALL_INC)
$(NDS_NATIVE_STAGE_METAL_INC): $(NDS_NATIVE_STAGE_GENERATOR_PREREQ)
	python "$(PROJECT_ROOT)/scripts/stages/generate_nds_native_stage.py" --repo-root "$(PROJECT_ROOT)" --stage metal
	@touch $(NDS_NATIVE_STAGE_METAL_INC)
$(NDS_NATIVE_STAGE_ZAKO_INC): $(NDS_NATIVE_STAGE_GENERATOR_PREREQ)
	python "$(PROJECT_ROOT)/scripts/stages/generate_nds_native_stage.py" --repo-root "$(PROJECT_ROOT)" --stage zako
	@touch $(NDS_NATIVE_STAGE_ZAKO_INC)
$(NDS_NATIVE_STAGE_LAST_INC): $(NDS_NATIVE_STAGE_GENERATOR_PREREQ)
	python "$(PROJECT_ROOT)/scripts/stages/generate_nds_native_stage.py" --repo-root "$(PROJECT_ROOT)" --stage last
	@touch $(NDS_NATIVE_STAGE_LAST_INC)
nds_renderer_assets.o: $(NDS_NATIVE_STAGE_OWNER_INC) $(NDS_NATIVE_STAGE_YOSTER_INC) $(NDS_NATIVE_STAGE_JUNGLE_INC) $(NDS_NATIVE_STAGE_CASTLE_INC) $(NDS_NATIVE_STAGE_SECTOR_INC) $(NDS_NATIVE_STAGE_HYRULE_INC) $(NDS_NATIVE_STAGE_INISHIE_INC) $(NDS_NATIVE_STAGE_ZEBES_INC) $(NDS_NATIVE_STAGE_YAMABUKI_INC) $(NDS_NATIVE_STAGE_PUPUPUSMALL_INC) $(NDS_NATIVE_STAGE_YOSTERSMALL_INC) $(NDS_NATIVE_STAGE_METAL_INC) $(NDS_NATIVE_STAGE_ZAKO_INC) $(NDS_NATIVE_STAGE_LAST_INC)


$(NITROFS_DIR)/renderer/battle_playable_static_textures.rgb5a1.bin: $(NDS_BATTLE_STATIC_TEXTURE_ASSET)
	@mkdir -p $(dir $@)
	@cp $< $@

# P2-1c. Grouped and touched for the same two reasons as the block above: one
# invocation writes both outputs, and it is write-if-changed. `include/
# reloc_data.h` is a real prerequisite -- the generator reads every sprite
# offset out of it rather than carrying a second copy, so a moved offset must
# re-bake.
# P2-4: the bake now varies with NDS_P2_STAGE_YOSTER, so the config header is a
# prerequisite. Without it a flag flip leaves the previous kit in place and the
# stage select shows the wrong preview set -- generated output under a shared
# path that no longer matches the flags that produced it.
# ...AND THE CONFIG HEADER IS NOT ENOUGH, because the kit is emitted to a
# SHARED path while the config header is per-BUILD. Bake A touches the shared
# inc; build B whose config is older then reuses A of a different flag set.
# That is not hypothetical: a Planet Zebes build linked against a kit baked
# without its preview and failed on the undeclared surface id. The stamp
# below carries the flag values themselves and is rewritten only when they
# change, so the bake is keyed on what actually varies rather than on a
# timestamp that another build controls.
NDS_MN_UI_KIT_FLAGS := $(NDS_P2_STAGE_YOSTER)$(NDS_P2_STAGE_CASTLE)$(NDS_P2_STAGE_JUNGLE)$(NDS_P2_STAGE_ZEBES)$(NDS_P2_STAGE_HYRULE)$(NDS_P2_STAGE_YAMABUKI)$(NDS_P2_STAGE_INISHIE)$(NDS_P2_STAGE_SECTOR)
NDS_MN_UI_KIT_STAMP := $(PROJECT_ROOT)/src/nds/generated/mn_ui_kit.flags.stamp

$(NDS_MN_UI_KIT_STAMP): FORCE
	@mkdir -p $(dir $@)
	@printf %s "$(NDS_MN_UI_KIT_FLAGS)" > $@.tmp
	@if ! cmp -s $@.tmp $@; then mv -f $@.tmp $@; else rm -f $@.tmp; fi

$(NDS_MN_UI_KIT_INC) $(NDS_MN_UI_KIT_ASSET) $(NDS_MN_UI_SURFACE_ASSET) &: \
		$(PROJECT_ROOT)/scripts/menus/generate_mn_ui_kit.py \
		$(NDS_BUILD_CONFIG) \
		$(NDS_MN_UI_KIT_STAMP) \
		$(PROJECT_ROOT)/include/reloc_data.h
	NDS_P2_STAGE_YOSTER=$(NDS_P2_STAGE_YOSTER) NDS_P2_STAGE_CASTLE=$(NDS_P2_STAGE_CASTLE) NDS_P2_STAGE_JUNGLE=$(NDS_P2_STAGE_JUNGLE) NDS_P2_STAGE_ZEBES=$(NDS_P2_STAGE_ZEBES) NDS_P2_STAGE_HYRULE=$(NDS_P2_STAGE_HYRULE) NDS_P2_STAGE_YAMABUKI=$(NDS_P2_STAGE_YAMABUKI) NDS_P2_STAGE_INISHIE=$(NDS_P2_STAGE_INISHIE) NDS_P2_STAGE_SECTOR=$(NDS_P2_STAGE_SECTOR) python "$(PROJECT_ROOT)/scripts/menus/generate_mn_ui_kit.py" --repo-root "$(PROJECT_ROOT)"
	@touch $(NDS_MN_UI_KIT_INC) $(NDS_MN_UI_KIT_ASSET) $(NDS_MN_UI_SURFACE_ASSET)

# P2-2 lower-screen HUD.  Keep every source container the bake reads on the
# dependency edge so an o2r refresh cannot silently leave a stale C include.
$(NDS_BATTLE_HUD_INC): \
		$(PROJECT_ROOT)/scripts/menus/generate_battle_hud.py \
		$(PROJECT_ROOT)/scripts/menus/generate_mn_ui_kit.py \
		$(PROJECT_ROOT)/include/reloc_data.h \
		$(BATTLESHIP_O2R)/reloc_interface/IFCommonPlayerDamage \
		$(BATTLESHIP_O2R)/reloc_interface/IFCommonTimer \
		$(BATTLESHIP_O2R)/reloc_interface/IFCommonDigits \
		$(BATTLESHIP_O2R)/reloc_menus/MNPlayersPortraits \
		$(BATTLESHIP_O2R)/reloc_fighters_main/MarioModel \
		$(BATTLESHIP_O2R)/reloc_fighters_main/FoxModel \
		$(BATTLESHIP_O2R)/reloc_fighters_main/LuigiModel
	python "$(PROJECT_ROOT)/scripts/menus/generate_battle_hud.py" --repo-root "$(PROJECT_ROOT)"
	@touch $(NDS_BATTLE_HUD_INC)

# Source-entry AOT packet.  The generator imports the same display-list/texture
# decoders as the static battle bake and SHA-pins all three source containers;
# list those files explicitly as dependencies as well so a refreshed O2R causes
# the check to run immediately rather than waiting for somebody to invoke the
# script by hand.
$(NDS_ENTRY_EFFECT_INC): \
		$(PROJECT_ROOT)/scripts/3d_vfx/generate_nds_entry_effects.py \
		$(PROJECT_ROOT)/scripts/generate_battle_playable_static_textures.py \
		$(PROJECT_ROOT)/scripts/generate_battle_playable_texture_census.py \
		$(BATTLESHIP_O2R)/reloc_fighters_main/MarioSpecial2 \
		$(BATTLESHIP_O2R)/reloc_fighters_main/FoxSpecial3 \
		$(BATTLESHIP_O2R)/reloc_fighters_main/DonkeySpecial2 \
		$(BATTLESHIP_O2R)/reloc_fighters_main/SamusSpecial2 \
		$(BATTLESHIP_O2R)/reloc_fighters_main/CaptainSpecial2 \
		$(BATTLESHIP_O2R)/reloc_fighters_main/LinkSpecial2 \
		$(BATTLESHIP_O2R)/reloc_fighters_main/LinkModel \
		$(BATTLESHIP_O2R)/reloc_fighters_main/LinkSpecial3 \
		$(BATTLESHIP_O2R)/reloc_extern_data/ExternDataBank109
	python "$(PROJECT_ROOT)/scripts/3d_vfx/generate_nds_entry_effects.py"
	@touch $(NDS_ENTRY_EFFECT_INC)

# P2-1k (d). Ordered after the kit's own rule by the surface-pack prerequisite:
# the pose table's oracles compare against the composite the kit bakes, so a
# stale pack would be checked instead of the one that ships.
$(NDS_MN_TITLE_ANIM_INC): \
		$(PROJECT_ROOT)/scripts/menus/decode_mn_title_anim.py \
		$(PROJECT_ROOT)/scripts/menus/generate_mn_ui_kit.py \
		$(NDS_MN_UI_SURFACE_ASSET) \
		$(PROJECT_ROOT)/include/reloc_data.h
	python "$(PROJECT_ROOT)/scripts/menus/decode_mn_title_anim.py" \
		--repo-root "$(PROJECT_ROOT)" --emit
	@touch $(NDS_MN_TITLE_ANIM_INC)

$(NITROFS_DIR)/menus/mn_ui_kit.bin: $(NDS_MN_UI_KIT_ASSET)
	@mkdir -p $(dir $@)
	@cp $< $@

$(NITROFS_DIR)/menus/mn_surfaces.bin: $(NDS_MN_UI_SURFACE_ASSET)
	@mkdir -p $(dir $@)
	@cp $< $@

# P2-1h's banner icon. Generated like every other asset here rather than
# checked in as a bitmap, so the emblem it comes from stays traceable to the
# reloc offset it is decoded at. Unconditional: the banner is not behind a
# feature flag, so EVERY target -- published and lab -- carries it, and the
# next published build is where it reaches the owner's system menu.
$(NDS_BANNER_ICON): $(PROJECT_ROOT)/scripts/menus/generate_nds_banner_icon.py \
		$(PROJECT_ROOT)/scripts/menus/generate_mn_ui_kit.py \
		$(PROJECT_ROOT)/include/reloc_data.h
	python "$(PROJECT_ROOT)/scripts/menus/generate_nds_banner_icon.py" --repo-root "$(PROJECT_ROOT)"
	@touch $(NDS_BANNER_ICON)

$(NITROFS_DIR)/audio/%: $(BATTLESHIP_O2R)/audio/%
	@mkdir -p $(dir $@)
	@cp $< $@

# A killed compiler can leave its .d file before ERROR_FILTER gets a chance to
# repair Windows drive paths. Sanitize existing dependency files before make
# parses them so the next incremental build can recover without a clean.
NDS_EXISTING_DEPENDS := $(wildcard $(DEPENDS))
ifneq ($(strip $(NDS_EXISTING_DEPENDS)),)
$(shell sed -i -e 's!\([A-Za-z]\):devkitPro!\1:/devkitPro!g' $(NDS_EXISTING_DEPENDS))
endif

-include $(DEPENDS)

endif

# Read-only benchmark metadata.  Keep this target independent from the build
# graph so verifier `-NoBuild` runs can still report the exact flags selected
# by this Makefile.
.PHONY: print-benchmark-flags
print-benchmark-flags:
	@printf '%s\n' 'BENCH_MAKE_TARGET=$(TARGET)'
	@printf '%s\n' 'BENCH_MAKE_HARNESS=$(NDS_DEV_SCENE_HARNESS)'
	@printf '%s\n' 'BENCH_MAKE_HARNESS_ID=$(NDS_DEV_SCENE_HARNESS_ID)'
	@printf '%s\n' 'BENCH_MAKE_PROFILE=$(NDS_RENDERER_PROFILE_LEVEL)'
	@printf '%s\n' 'BENCH_MAKE_SHIP_TELEMETRY=$(NDS_SHIP_TELEMETRY)'
	@printf '%s\n' 'BENCH_MAKE_TICK_HUD=$(NDS_TICK_HUD)'
	@printf '%s\n' 'BENCH_MAKE_M2_DETAILED_LEDGER=$(NDS_RENDERER_M2_DETAILED_LEDGER)'
	@printf '%s\n' 'BENCH_MAKE_M3_PHASE0_PROFILE=$(NDS_RENDERER_M3_PHASE0_PROFILE)'
	@printf '%s\n' 'BENCH_MAKE_NATIVE_STAGE_GENERATED_SEGMENT0_ENABLE=$(NDS_NATIVE_STAGE_GENERATED_SEGMENT0_ENABLE)'
	@printf '%s\n' 'BENCH_MAKE_TASK29_GX_CENSUS=$(NDS_TASK29_GX_CENSUS)'
	@printf '%s\n' 'BENCH_MAKE_TASK34_STAGE_STREAM_CENSUS=$(NDS_TASK34_STAGE_STREAM_CENSUS)'
	@printf '%s\n' 'BENCH_MAKE_TASK49_GX_DIFFER=$(NDS_TASK49_GX_DIFFER)'
	@printf '%s\n' 'BENCH_MAKE_TASK53_REPLAY_ARENA_FIX=$(NDS_TASK53_REPLAY_ARENA_FIX)'
	@printf '%s\n' 'BENCH_MAKE_TASK55_STAGE_GEOM=$(NDS_TASK55_STAGE_GEOM)'
	@printf '%s\n' 'BENCH_MAKE_TASK56_FIGHTER_PRIMITIVES=$(NDS_TASK56_FIGHTER_PRIMITIVES)'
	@printf '%s\n' 'BENCH_MAKE_TASK36_HW_COMPOSE=$(NDS_TASK36_HW_COMPOSE)'
	@printf '%s\n' 'BENCH_MAKE_TASK51_STAGE_NATIVE=$(NDS_TASK51_STAGE_NATIVE)'
	@printf '%s\n' 'BENCH_MAKE_DREAMLAND_DS_MESH=$(NDS_DREAMLAND_DS_MESH)'
	@printf '%s\n' 'BENCH_MAKE_DREAMLAND_CARD_CULL=$(NDS_DREAMLAND_CARD_CULL)'
	@printf '%s\n' 'BENCH_MAKE_BATTLE_PROFILE=$(NDS_BATTLE_PROFILE)'
	@printf '%s\n' 'BENCH_MAKE_TASK44_STAGE_STEADY=$(NDS_TASK44_STAGE_STEADY)'
	@printf '%s\n' 'BENCH_MAKE_TASK37_PROFILE=$(NDS_TASK37_PROFILE)'
	@printf '%s\n' 'BENCH_MAKE_TASK37_ITCM_LEAVES=$(NDS_TASK37_ITCM_LEAVES)'
	@printf '%s\n' 'BENCH_MAKE_TASK22_WALLPAPER_RUN_LAB=$(NDS_TASK22_WALLPAPER_RUN_LAB)'
	@printf '%s\n' 'BENCH_MAKE_SCREEN_SPACE_CENSUS=$(NDS_RENDERER_SCREEN_SPACE_CENSUS)'
	@printf '%s\n' 'BENCH_MAKE_RENDER_ECONOMY=$(NDS_RENDER_ECONOMY)'
	@printf '%s\n' 'BENCH_MAKE_RENDER_ECONOMY_OWNER_MASK=$(NDS_RENDER_ECONOMY_OWNER_MASK)'
	@printf '%s\n' 'BENCH_MAKE_RENDERER_BENCHMARK_MODE=$(NDS_RENDERER_BENCHMARK_MODE)'
	@printf '%s\n' 'BENCH_MAKE_FAST_RUN_DEFAULT=$(NDS_RENDERER_FAST_RUN_DEFAULT)'
	@printf '%s\n' 'BENCH_MAKE_SCENE_MIP_CACHE_LAB=$(NDS_SCENE_MIP_CACHE_LAB)'
	@printf '%s\n' 'BENCH_MAKE_FAST_WALLPAPER_AFFINE=$(NDS_FAST_WALLPAPER_AFFINE)'
	@printf '%s\n' 'BENCH_MAKE_BATTLE_STATIC_TEXTURE_DEFAULT=$(NDS_RENDERER_BATTLE_STATIC_TEXTURE_DEFAULT)'
	@printf '%s\n' 'BENCH_MAKE_IFCOMMON_HYBRID_OAM=$(NDS_IFCOMMON_HYBRID_OAM)'
	@printf '%s\n' 'BENCH_MAKE_TASK9_FLOAT_CENSUS=$(NDS_TASK9_FLOAT_CENSUS)'
	@printf '%s\n' 'BENCH_MAKE_TASK9_FLOAT_ITCM=$(NDS_TASK9_FLOAT_ITCM)'
	@printf '%s\n' 'BENCH_MAKE_TASK9_FLOAT_PHASE2=$(NDS_TASK9_FLOAT_PHASE2)'
	@printf '%s\n' 'BENCH_MAKE_TASK16_FLOAT_COMPARE=$(NDS_TASK16_FLOAT_COMPARE)'
	@printf '%s\n' 'BENCH_MAKE_TASK16_FLOAT_I2F=$(NDS_TASK16_FLOAT_I2F)'
	@printf '%s\n' 'BENCH_MAKE_TASK16_FLOAT_ADDSUB=$(NDS_TASK16_FLOAT_ADDSUB)'
	@printf '%s\n' 'BENCH_MAKE_TASK9_STATE_HASH=$(NDS_TASK9_STATE_HASH)'
	@printf '%s\n' 'BENCH_MAKE_TASK10_HARDWARE_CALIBRATION=$(NDS_TASK10_HARDWARE_CALIBRATION)'
	@printf '%s\n' 'BENCH_MAKE_TASK20_STACK_PROFILE=$(NDS_TASK20_STACK_PROFILE)'
	@printf '%s\n' 'BENCH_MAKE_TASK32_DRAW_HOT_TEXT=$(NDS_TASK32_DRAW_HOT_TEXT)'
	@printf '%s\n' 'BENCH_MAKE_TASK39_FX_SPRITES=$(NDS_TASK39_FX_SPRITES)'
	@printf '%s\n' 'BENCH_MAKE_TASK39_FX_FLASH=$(NDS_TASK39_FX_FLASH)'
	@printf '%s\n' 'BENCH_MAKE_R2_PATH=$(NDS_R2_PATH)'
	@printf '%s\n' 'BENCH_MAKE_R2_STAGE_DIRECT=$(NDS_R2_STAGE_DIRECT)'
	@printf '%s\n' 'BENCH_MAKE_R2_FIXED_SQRT=$(NDS_R2_FIXED_SQRT)'
	@printf '%s\n' 'BENCH_MAKE_R2_STAGE_DMA=$(NDS_R2_STAGE_DMA)'
	@printf '%s\n' 'BENCH_MAKE_R2_STAGE_VIEWPROJ=$(NDS_R2_STAGE_VIEWPROJ)'
	@printf '%s\n' 'BENCH_MAKE_R2_STAGE_PREFLIGHT=$(NDS_R2_STAGE_PREFLIGHT)'
	@printf '%s\n' 'BENCH_MAKE_CFLAGS_COMMON=$(strip $(CFLAGS))'
	@printf '%s\n' 'BENCH_MAKE_CFLAGS_RENDERER=$(strip $(CFLAGS) $(if $(filter 163,$(NDS_DEV_SCENE_HARNESS_ID)),-marm))'
	@printf '%s\n' 'BENCH_MAKE_CFLAGS_SCENE=$(strip $(CFLAGS))'

# Nonbuilding semantic probe for the toolchain-path identity checker.  Its
# caller supplies an unused outer BUILD path so dependency files are not read
# or repaired while the Makefile is parsed.
.PHONY: print-toolchain-paths
print-toolchain-paths:
	@printf '%s\n' 'TOOLCHAIN_DEVKITPRO=$(DEVKITPRO)'
	@printf '%s\n' 'TOOLCHAIN_DEVKITARM=$(DEVKITARM)'
	@printf '%s\n' 'TOOLCHAIN_CALICO=$(CALICO)'
	@printf '%s\n' 'TOOLCHAIN_LIBNDS=$(LIBNDS)'
