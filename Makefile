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

GAME_TITLE     := Smash 64 DS Port
GAME_SUBTITLE1 := BattleShip architecture probe
GAME_SUBTITLE2 := Built with devkitPro/libnds

PROJECT_ROOT ?= $(CURDIR)
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
# Topology is compiled host-side (no runtime strip finding). Default 0 keeps the
# published ROM byte-identical; the published/tick-HUD blocks do NOT override it.
#
# NOT A GATE LEVER, and this is a structural statement rather than another
# measurement. 2026-08-01 ranked the over-gate population per frame: on the
# frames that miss the gate `FTR` is FLAT -- 1,536 ticks BELOW its clean median
# on the worst frame of the run -- while `SRC` runs +250K to +440K and `MISC`
# +77K to +120K. A lever that touches only `FTR` therefore cannot change the
# over-gate COUNT by more than a couple of frames whatever its sign, and its
# sign was already measured negative once (PERF_LEDGER, "Task 56 ... KILL":
# FTR +5,824, +1.0%, against a 47% vertex-submission cut). A 2026-08-01 re-test
# was started and abandoned for that reason, not because it failed.
NDS_TASK56_FIGHTER_PRIMITIVES ?= 0
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
# BUGS.md #10 lab probe. Holding SELECT draws every polygon two-sided, so the
# owner can A/B culling in place at one fixed camera. The original culling
# probe compared two separate captures and was judged on an image that did not
# contain the bug; this exists so that mistake cannot repeat. Lab only.
NDS_LAB_CULL_PROBE ?= 0
# Which bits of the run index the tint probe shows. 0 = low three bits, 3 =
# next three, so two captures name a run exactly out of the 67 there are.
NDS_LAB_TINT_SHIFT ?= 0
# Renders both sides of every polygon. Splits "the geometry never reached the
# GX" from "the GX culled it", which no counter can tell apart.
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
# Switch plan R2-06 harness prerequisite, owner-requested 2026-07-29. Makes
# player 0 a level-3 CPU as well, so both fighters attack continuously with no
# recorded input stream. A deliberate STRESS case: it maximises the live hitbox
# population that R2-03 E35 named as the owner of the SRC P95 excursion, which
# also makes it the configuration that most exercises E64b's Q12 cubic (more hit
# decisions to flip) and E32's hitlag fallback (more bursts).
#
# NEVER report a P95 from this build as the Boundary figure. The switch plan
# defines the shipped Boundary as Mario human vs level-3 Fox CPU at mode 163 and
# PROJECT_GOAL.md's gate as representative gameplay; this is harder than either.
NDS_R2_BOTH_CPU ?= 0
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
# R2-03 E17. Loads the fighter's projection and modelview separately and lets the
# geometry engine perform the multiply, instead of composing on the CPU and
# loading the product. Measured -17,600 FTR P50 / -18,560 WORK P50, and it leaves
# the modelview alone in the vector matrix rather than the composed MVP, which is
# what hardware lighting needs. Rendering-side -- positions now round in hardware
# -- so it gates on a screenshot pair plus the owner's approval.
NDS_R2_FIGHTER_HW_MTX ?= 0
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
# R2-03 E17: split matrix load. Stops composing modelview x projection on the
# CPU and lets the geometry engine multiply, -17,600 FTR P50. Boundary green in
# both flag states, geometry proven identical (136,640 P0 triangles either way
# over the same 480-frame window), owner-approved 2026-07-28.
override NDS_R2_FIGHTER_HW_MTX := 1
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
# match. Misses 29 -> 2, WORK-H P95 1,364,992 -> 1,232,640.
override NDS_R2_ANIM_CACHE := 1
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
# 7, not 1: LEAVES was a boolean when this was measured and play-tested, and
# became a bitmask (1=libc 2=libm 4=port) in 729c3a2. All seven leaves is 7.
override NDS_TASK37_ITCM_LEAVES := 7
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
ifneq ($(filter $(TARGET),smash64ds-battle-playable-tickhud-hwtri smash64ds-battle-playable-proof-hwtri smash64ds-results-lab-hwtri),)
# Profile-0 shipping path plus either the lightweight Task 41 timers or the
# full diagnostic publications required by GDB proof runs.
#
# smash64ds-results-lab-hwtri rides this block deliberately: R2-07's Results
# numbers have to be comparable with the battle ones, so it must differ from
# the tick-HUD ROM in the scene it boots and in NOTHING else. Adding it to the
# filter rather than cloning the block is what guarantees that -- a copied
# block would drift the moment either half was edited.
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
ifneq ($(filter $(TARGET),smash64ds-battle-playable-tickhud-hwtri smash64ds-results-lab-hwtri),)
override NDS_SHIP_TELEMETRY := 0
override NDS_TICK_HUD := 1
else
override NDS_SHIP_TELEMETRY := 1
override NDS_TICK_HUD := 0
endif
override NDS_RENDERER_FAST_RUN_DEFAULT := 9
override NDS_NATIVE_STAGE_GENERATED_SEGMENT0_ENABLE := 1
override NDS_TASK36_HW_COMPOSE := 2
# R2-03 E17/E16: match the published block. Any flag there is on this one too.
override NDS_R2_FIGHTER_HW_MTX := 1
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
# Task 37 shipped on 2026-07-22 and this block was not updated with it.
override NDS_TASK37_ITCM_LEAVES := 7
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
ifneq ($(filter -1 0 1,$(NDS_FIGHTER_ANIM_CYCLER_KIND)),$(NDS_FIGHTER_ANIM_CYCLER_KIND))
$(error NDS_FIGHTER_ANIM_CYCLER_KIND must be -1, 0 (Mario), or 1 (Fox))
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
NDS_TASK9_FLOAT_ITCM_OFILES := \
	$(addsuffix .itcm.o,$(basename $(NDS_TASK9_FLOAT_ITCM_MEMBERS)))

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
CFILES := main.c nds_platform.c nds_ifcommon_oam.c nds_task39_effect_census.c nds_reloc_assets.c nds_audio_assets.c nds_audio_bgm.c nds_audio_fgm.c nds_renderer.c battle_playable_static_textures.c port_probe.c n64_stubs.c coroutine.c \
	libultra_os.c os_selftest.c boot_stubs.c battleship_sys_main.c \
	scheduler_backend.c controller_backend.c battleship_sys_scheduler.c \
	battleship_sys_controller.c battleship_sys_maindevice.c \
	battleship_sys_video.c battleship_sys_malloc.c \
	battleship_sys_framebuffer.c battleship_sys_zbuffer.c video_bootstrap.c \
	battleship_sys_sintable.c battleship_sys_matrix.c \
	battleship_libultra_gu_normalize.c battleship_libultra_gu_mtxcatf.c \
	battleship_scmanager.c battleship_mnstartup.c scene_backend.c scene_harness.c utils.c vector.c \
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
ifeq ($(NDS_R2_FIXED_SQRT),1)
CFILES += nds_r2_sqrtf.c
endif
ifeq ($(NDS_IMPORT_BATTLESHIP_NORMAL_MOVESET),1)
CFILES += battleship_ftcommon_normal_moveset.c
endif
CFILES += battleship_ftchar_data_slots.c battleship_scsubsysdata_ft.c \
	battleship_ftdata.c reloc_backend_ftdata_stubs.c \
	reloc_backend_ftdata_symbols.c
CFILES += battleship_ftanim.c battleship_ftanimend.c battleship_ftkey.c
ifeq ($(NDS_IMPORT_BATTLESHIP_FTMANAGER),1)
CFILES += battleship_ftmanager.c
ifeq ($(NDS_IMPORT_BATTLESHIP_MPPROCESS_LIVE),1)
CFILES += $(NDS_MPPROCESS_SOURCE_CFILES) \
	battleship_mpprocess_live_bridge.c
else ifeq ($(NDS_IMPORT_BATTLESHIP_MPPROCESS_PRIVATE),1)
NDS_PRIVATE_CHECK_CFILES += $(NDS_MPPROCESS_SOURCE_CFILES)
endif
CFILES += battleship_ftstatus_callback_aliases.c \
	battleship_ftstatus_map_physics_shims.c \
	battleship_ftstatus_inactive_stubs.c
endif
ifeq ($(NDS_IMPORT_BATTLESHIP_FTCOMPUTER),1)
CFILES += battleship_ftcomputer.c
endif
ifeq ($(NDS_IMPORT_BATTLESHIP_BATTLE_PLAYABLE),1)
CFILES += battleship_gmcamera.c battleship_ftcommon_dead.c \
	battleship_ftcommon_rebirth.c battleship_grwallpaper.c \
	battle_playable_compat_stubs.c
endif
ifeq ($(NDS_IMPORT_BATTLESHIP_VS_RESULTS),1)
CFILES += battleship_lbtransition.c battleship_mnvsresults.c \
	battleship_scsubsysfighter.c battleship_scsubsysdata.c
endif
CFILES += battleship_ifscreenflash.c
ifeq ($(NDS_IMPORT_BATTLESHIP_IFCOMMON),1)
CFILES += battleship_ifcommon.c
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
ifeq ($(NDS_ENABLE_INISHIE_SOURCE_SCALE_SETUP),1)
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

NDS_EFFECT_RELOC_FILES := \
	reloc_effects/EFCommonEffects1 \
	reloc_effects/EFCommonEffects2 \
	reloc_effects/EFCommonEffects3

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
	audio/bgm_results_ima.bin
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
	$(foreach file,$(NDS_STAGE_SCOUT_RELOC_FILES),$(NITROFS_DIR)/reloc/$(file)) \
	$(foreach file,$(NDS_MARIOFOX_FIGHTER_RELOC_FILES),$(NITROFS_DIR)/reloc/$(file)) \
	$(foreach file,$(NDS_EFFECT_RELOC_FILES),$(NITROFS_DIR)/reloc/$(file)) \
	$(foreach file,$(NDS_VSBATTLE_RELOC_FILES),$(NITROFS_DIR)/reloc/$(file)) \
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

# The efcommon payloads only ship with the interpreter that reads them; without
# the runtime they are 200,896 bytes of ROM nothing opens. Two files, two
# encodings of the same texels: the .ds.bin is per-texture DS formats with
# palettes (the pack), the .rgb5a1.bin is the 22 admitted textures as RGB555+A1
# (the draw path, which uploads through a texture cache with no palette slot).
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

.PHONY: all clean clean-generated distclean run $(BUILD) prune-obsolete-audio

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

$(NDS_BUILD_CONFIG): FORCE
	@tmp="$@.tmp"; \
	{ \
		echo '#ifndef NDS_BUILD_CONFIG_H'; \
		echo '#define NDS_BUILD_CONFIG_H'; \
		echo '#define NDS_DEV_LIVE_INPUT_PREVIEW $(NDS_DEV_LIVE_INPUT_PREVIEW)'; \
		echo '#define NDS_HARNESS_FAST_LOGIC $(NDS_HARNESS_FAST_LOGIC)'; \
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
		echo '#define NDS_LAB_CULL_PROBE $(NDS_LAB_CULL_PROBE)'; \
		echo '#define NDS_LAB_TINT_SHIFT $(NDS_LAB_TINT_SHIFT)'; \
		echo '#define NDS_LAB_NO_CULL $(NDS_LAB_NO_CULL)'; \
		echo '#define NDS_TASK91_DRAW_PHASE_CENSUS $(NDS_TASK91_DRAW_PHASE_CENSUS)'; \
		echo '#define NDS_R2_SPAN_LEAN_TIMING $(NDS_R2_SPAN_LEAN_TIMING)'; \
		echo '#define NDS_R2_DELTA_PATH_ITCM $(NDS_R2_DELTA_PATH_ITCM)'; \
		echo '#define NDS_R2_ANIM_CACHE $(NDS_R2_ANIM_CACHE)'; \
		echo '#define NDS_R2_MATERIAL_DYNAMIC $(NDS_R2_MATERIAL_DYNAMIC)'; \
		echo '#define NDS_R2_FLASH_PROBE $(NDS_R2_FLASH_PROBE)'; \
		echo '#define NDS_R2_ANIM_CENSUS $(NDS_R2_ANIM_CENSUS)'; \
		echo '#define NDS_R2_CUBIC_FIXED $(NDS_R2_CUBIC_FIXED)'; \
		echo '#define NDS_R2_RELOC_FIXUP_TIMING $(NDS_R2_RELOC_FIXUP_TIMING)'; \
		echo '#define NDS_R2_LOADFRAME_TIMING $(NDS_R2_LOADFRAME_TIMING)'; \
		echo '#define NDS_R2_BOTH_CPU $(NDS_R2_BOTH_CPU)'; \
		echo '#define NDS_R2_SOAK_MATCH_MINUTES $(NDS_R2_SOAK_MATCH_MINUTES)'; \
		echo '#define NDS_R2_UNLIT_VERTEX_EPOCH $(NDS_R2_UNLIT_VERTEX_EPOCH)'; \
		echo '#define NDS_R204_FPSHUD_SHADOW $(NDS_R204_FPSHUD_SHADOW)'; \
		echo '#define NDS_TASK103_STAGE_RUN_PHASE $(NDS_TASK103_STAGE_RUN_PHASE)'; \
		echo '#define NDS_TASK104_STAGE_STATS_ELISION $(NDS_TASK104_STAGE_STATS_ELISION)'; \
		echo '#define NDS_TASK106_UPDATES_PER_PRESENT $(NDS_TASK106_UPDATES_PER_PRESENT)u'; \
		echo '#define NDS_TASK75_LOAD_CENSUS $(NDS_TASK75_LOAD_CENSUS)'; \
		echo '#define NDS_FTR_PLAN_ROUTE $(NDS_FTR_PLAN_ROUTE)u'; \
		echo '#define NDS_FTR_PLAN_VERIFY $(NDS_FTR_PLAN_VERIFY)u'; \
		echo '#define NDS_R2_SECOND_ENTRY_DIAG $(NDS_R2_SECOND_ENTRY_DIAG)'; \
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
		echo '#define NDS_R2_FIGHTER_HW_LIGHT $(NDS_R2_FIGHTER_HW_LIGHT)'; \
		echo '#define NDS_R2_FIGHTER_SOFT_LIGHT_KEEP $(NDS_R2_FIGHTER_SOFT_LIGHT_KEEP)'; \
		echo '#define NDS_TICK_HUD_DRAW $(NDS_TICK_HUD_DRAW)'; \
		echo '#define NDS_R2_FIGHTER_SHUFFLE_FOLD $(NDS_R2_FIGHTER_SHUFFLE_FOLD)'; \
		echo '#define NDS_R2_FIGHTER_EPOCH_STATE_PROOF $(NDS_R2_FIGHTER_EPOCH_STATE_PROOF)'; \
		echo '#define NDS_R2_FIGHTER_SHADE_SKIP $(NDS_R2_FIGHTER_SHADE_SKIP)'; \
		echo '#define NDS_R2_FIGHTER_STATESPAN_SKIP $(NDS_R2_FIGHTER_STATESPAN_SKIP)'; \
		echo '#define NDS_R2_DRAW_SUPPRESS_MASK $(NDS_R2_DRAW_SUPPRESS_MASK)'; \
		echo '#define NDS_R2_FIGHTER_RUN_MEMO $(NDS_R2_FIGHTER_RUN_MEMO)'; \
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
		echo '#define NDS_R2_FOX_BLASTER_QUAD $(NDS_R2_FOX_BLASTER_QUAD)'; \
		echo '#define NDS_R2_FOX_BLASTER_GLOW_AOT $(NDS_R2_FOX_BLASTER_GLOW_AOT)'; \
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
		echo '#define NDS_R2_COLLISION_L7_ORACLE $(NDS_R2_COLLISION_L7_ORACLE)'; \
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
$(NDS_PARTICLE_BANKS_INC) $(NDS_PARTICLE_TEXTURE_ASSET) $(NDS_PARTICLE_QUAD_ASSET) $(NDS_WHISPY_NATIVE_ASSET) &: \
		$(PROJECT_ROOT)/scripts/generate_nds_particle_banks.py \
		$(PROJECT_ROOT)/scripts/2d_vfx/generate_task39_effect_census.py \
		$(BATTLESHIP_O2R)/particles/efcommon_particle_scb \
		$(BATTLESHIP_O2R)/particles/efcommon_particle_txb \
		$(BATTLESHIP_O2R)/particles/grpupupu_particle_scb \
		$(BATTLESHIP_O2R)/particles/grpupupu_particle_txb \
		$(PROJECT_ROOT)/$(BATTLESHIP_DECOMP)/src/ef/efmanager.c
	python "$(PROJECT_ROOT)/scripts/generate_nds_particle_banks.py"

$(NDS_PARTICLE_BANKS_HEADER): $(NDS_PARTICLE_BANKS_INC)

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

$(OUTPUT).nds: prune-obsolete-audio $(OUTPUT).elf $(NDS_NITROFS_RELOC_FILES) $(NDS_NITROFS_RELOCDATA_FILES) $(NDS_NITROFS_AUDIO_FILES) $(NDS_NITROFS_BATTLE_STATIC_TEXTURE_FILES) $(NDS_NITROFS_PARTICLE_FILES) $(NDS_NITROFS_EFFECT_FILES)
$(OUTPUT).elf: $(OFILES) $(NDS_PRIVATE_CHECK_OFILES) \
	$(NDS_HOT_TEXT_SPECS) $(NDS_HOT_TEXT_LINKER_SCRIPT) \
	$(NDS_TASK32_DRAW_HOT_FRAGMENT) $(NDS_PARTICLE_BANKS_INC) \
	$(NDS_BATTLE_STATIC_TEXTURE_INC)
$(OFILES) $(NDS_PRIVATE_CHECK_OFILES): $(PROJECT_ROOT)/Makefile $(NDS_BUILD_CONFIG)
ifeq ($(NDS_TASK9_FLOAT_ITCM),1)
NDS_TASK9_FLOAT_LIBGCC := $(shell $(CC) $(ARCH) -print-libgcc-file-name)
NDS_TASK9_FLOAT_AR := $(shell $(CC) -print-prog-name=ar)
# Keep the installed archive out of Make's prerequisite graph: `make -B` would
# otherwise try to rebuild that external .a through an implicit archive rule.
# One grouped recipe makes one verified private copy and extracts only from it.
$(NDS_TASK9_FLOAT_ITCM_OFILES) &: $(PROJECT_ROOT)/Makefile $(NDS_BUILD_CONFIG)
	@echo "$(NDS_TASK9_FLOAT_LIBGCC_SHA256) *$(NDS_TASK9_FLOAT_LIBGCC)" | sha256sum -c -
	@rm -rf ".task9-float-itcm" $(NDS_TASK9_FLOAT_ITCM_OFILES)
	@mkdir -p ".task9-float-itcm"
	@cp "$(NDS_TASK9_FLOAT_LIBGCC)" ".task9-float-itcm/libgcc.a"
	@cd ".task9-float-itcm" && $(NDS_TASK9_FLOAT_AR) x "libgcc.a" $(NDS_TASK9_FLOAT_ITCM_MEMBERS)
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
		$(OBJCOPY) $$phase2_filter \
			--rename-section .text=.itcm,alloc,load,readonly,code,contents \
			".task9-float-itcm/$$member" "$$stem.itcm.o" || exit $$?; \
	done
	@rm -rf ".task9-float-itcm"
endif
ifneq ($(strip $(NDS_TASK37_ITCM_OFILES)),)
NDS_TASK37_LIBC := $(shell $(CC) $(ARCH) -print-file-name=libc.a)
NDS_TASK37_LIBM := $(shell $(CC) $(ARCH) -print-file-name=libm.a)
NDS_TASK37_AR := $(shell $(CC) -print-prog-name=ar)
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
	@test "$(NDS_TASK37_ITCM_LIBC)" != "1" || (cd ".task37-itcm" && $(NDS_TASK37_AR) x "libc.a" $(NDS_TASK37_LIBC_MEMBERS))
	@test "$(NDS_TASK37_ITCM_LIBM)" != "1" || (cd ".task37-itcm" && $(NDS_TASK37_AR) x "libm.a" $(NDS_TASK37_LIBM_MEMBERS))
	@for member in $(if $(filter 1,$(NDS_TASK37_ITCM_LIBC)),$(NDS_TASK37_LIBC_MEMBERS)) $(if $(filter 1,$(NDS_TASK37_ITCM_LIBM)),$(NDS_TASK37_LIBM_MEMBERS)); do \
		stem="$${member%.o}"; \
		$(OBJCOPY) \
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
scene_backend.o: $(SCENE_BACKEND_SLICES) $(NDS_SCENE_HARNESS_CONFIG)
scene_harness.o battleship_grinishie_scale.o: $(NDS_SCENE_HARNESS_CONFIG)
nds_ifcommon_oam.o: $(NDS_TASK39_HIT_SPARKS_INC)

$(NITROFS_DIR)/reloc/%: $(BATTLESHIP_O2R)/%
	@mkdir -p $(dir $@)
	@cp $< $@

$(NITROFS_DIR)/relocdata/us/%: $(BATTLESHIP_RELOCDATA)/%
	@mkdir -p $(dir $@)
	@cp $< $@

$(NITROFS_DIR)/audio/bgm_pupupu_ima.bin: $(PROJECT_ROOT)/assets/audio/bgm_pupupu_ima.bin
	@mkdir -p $(dir $@)
	@cp $< $@

$(NITROFS_DIR)/audio/bgm_win_mario_ima.bin: $(PROJECT_ROOT)/assets/audio/bgm_win_mario_ima.bin
	@mkdir -p $(dir $@)
	@cp $< $@

$(NITROFS_DIR)/audio/bgm_win_fox_ima.bin: $(PROJECT_ROOT)/assets/audio/bgm_win_fox_ima.bin
	@mkdir -p $(dir $@)
	@cp $< $@

$(NITROFS_DIR)/audio/bgm_results_ima.bin: $(PROJECT_ROOT)/assets/audio/bgm_results_ima.bin
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

$(NITROFS_DIR)/renderer/battle_playable_static_textures.rgb5a1.bin: $(NDS_BATTLE_STATIC_TEXTURE_ASSET)
	@mkdir -p $(dir $@)
	@cp $< $@

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
