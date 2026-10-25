# The target name and the feature knobs every build reads: the roster and
# fighter switches (NDS_<KIND>), the shell rosters, the native owner image
# switches and the defaults the play, walk and gate ROMs build from.
# Included in place from the Makefile (P8 G); its text is unchanged.

TARGET := smash64ds
BUILD := build
NDS_OUTPUT_BASENAME ?= $(TARGET)
BUILD_OUTPUT_ROOT ?= builds
ifeq ($(BUILD),$(notdir $(BUILD)))
ifneq ($(filter build%,$(BUILD)),)
override BUILD := $(BUILD_OUTPUT_ROOT)/$(BUILD)
endif
endif
NDS_PUBLISHED_TARGETS := smash64ds
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
# script file nds_draw_hot.inc". The fix belongs in the harness -- resolve
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
# The DS hardware triangle path is the only one (2026-10-20, old machinery:
# the software-triangle branches are gone from the sources). The define stays
# in the config header for generated code that still tests it.
override NDS_RENDERER_HW_TRIANGLES := 1
NDS_RENDERER_PROFILE_LEVEL ?= 2
NDS_SHIP_TELEMETRY ?= 1
NDS_TICK_HUD ?= 0
# P2-2p8 (2026-10-04), owner ruling D13: fighter joint worlds stay fixed point
# end to end. The hurtbox reject's per-epoch world cache serves
# gmCollisionGetFighterPartsWorldPosition and the held item's 0x52 matrix in
# place of the float latch walk (src/port/nds_hurtbox_reject.c). Re-baselines
# the replay digest; the P2 VS targets only (the frozen P1 target keeps the
# float walk). gNdsJointResident is the same-ROM A/B word.
NDS_JOINT_RESIDENT ?= $(if $(filter smash64ds-p2-fourcpu-tickhud-hwtri smash64ds-p2-shell-hwtri smash64ds-p2-shell-freeplay-hwtri smash64ds-p2-shell-loop-hwtri smash64ds,$(TARGET)),1,0)
# P2-2p8 T1 (A7 memory): 1 stages each of the nine VS ground maps (GR<Stage>Map)
# as a COMPACT map -- the stage wallpaper's Sprite and Bitmap[44] table (776 B)
# appended, MPGroundData.wallpaper pointing at them -- so the map's extern tree
# stops holding the 158,928-byte wallpaper container: 158,144 B less resident in
# every VS battle (scripts/stages/generate_compact_ground_maps.py; runtime seams
# in src/port/reloc_backend_assets.c). The container's pixels were never read;
# the background is the converted BG2 image. Default 1 on the P2 targets that
# load VS stages -- the four-CPU stress target and the shell family (the
# published smash64ds is the free-play twin) -- and 0 everywhere else, notably
# the frozen P1 battle-playable target, whose NitroFS map files, packet rows and
# Dream Land golden pin stay the source sizes. `NDS_COMPACT_GROUND_MAPS=0` is
# the same-ROM A/B control. The staged maps follow the flag through a per-build
# stamp (nds_compact_ground_maps.stamp), so a flip inside one BUILD dir re-stages
# them instead of shipping the other arm's files.
NDS_COMPACT_GROUND_MAPS ?= \
	$(if $(filter smash64ds-p2-fourcpu-tickhud-hwtri smash64ds-p2-shell-hwtri smash64ds-p2-shell-freeplay-hwtri smash64ds-p2-shell-loop-hwtri smash64ds,$(TARGET)),1,0)
NDS_RENDERER_M2_DETAILED_LEDGER ?= 0
NDS_NATIVE_STAGE_GENERATED_SEGMENT0_ENABLE ?= 0
NDS_STAGE_HW_COMPOSE ?= 0

# The fighter DS-native primitive streams. Compiles the immutable Mario/Fox
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
# One-binary A/B on gNdsFighterStripRoute (NDS_STRIP_ROUTE=1, same ROM sha,
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
NDS_FIGHTER_PRIMITIVES ?= 2
# Task 51 native stage path. When on, the STAGE owner emits the 42 baked
# constant world matrices via MTX_MULT4x3 under a once-loaded view (instead of
# CPU-composing projection x view x model per binding per frame). Generalizes
# Task 36's rigid-subset replay to the full 42-binding model-space form.
# Default off; the published ROM stays byte-identical at 0. Ship-ready only
# once the differ (Tier 1 = 0, Tier 2 <= 1.0 px) and STG budget clear.
NDS_STAGE_BAKED_WORLD ?= 0
# Task 62: generated Dream Land DS-native static 3D mesh (candidate c120).
# Default off; the published ROM stays byte-identical at 0. Ship-ready only
# once the owner's visual A/B + perf A/B (Commit 5 KEEP gate) clears.
NDS_DREAMLAND_DS_MESH ?= 0
# P2-2p8 joint-cap lab arm (owner 2026-09-16: test builds of experimental
# optimizations are allowed). 0 = the shipping skeleton. N > 0 prunes every
# common-part DObj whose container index is >= N at fighter setup, which is a
# genuinely smaller skeleton rather than a skipped loop: the pruned node never
# exists, so its descendants fail their `parent == NULL` test and prune with it,
# and every downstream per-joint loop -- pose, anim keys, the invalidate walk,
# matrix build, draw traversal -- never sees it. Lab only: limbs disappear, and
# a fighter whose attributes name a pruned joint may abort, which is a result
# (that cap is not implementable) and not a defect in the instrument. Exists to
# PRICE the joint lever that the gap sizing calls the only per-fighter
# candidate left. See artifacts/performance/2026-09-16_p2-2p8-joint-cap-ladder/.
NDS_LAB_JOINT_CAP ?= 0
# The same lever's animation half, and the arm that is actually runnable.
# NDS_LAB_JOINT_CAP=1 engages and then ABORTS in the CPU AI: a pruned joint
# leaves fp->joints[] NULL and ndsBaseFTComputerSetFighterDamageDetectSize
# dereferences it (decomp ft/ftcomputer.c:7970), which is itself the finding --
# a smaller skeleton is a per-fighter data re-derivation, not a switch. This cap
# creates no NULL: every DObj stays, and only pose entries at index >= N decline
# to evaluate. It prices the animation lane and is therefore a FLOOR for the
# joint lever, never the lever. 0 = shipping.
NDS_LAB_POSE_JOINT_CAP ?= 0
# P2-2p8 N05.03. Slots in the flattened fighter-parts invalidation cache
# (`reloc_backend_source_shims.c`). Must be a power of two. The shipped value is
# 4, chosen when the cache was believed to be keyed per fighter root; it is
# keyed per invalidated JOINT, so four slots cannot hold a steady state. Raising
# it costs 396 bytes of .bss per slot and is the whole of the candidate -- the
# flatten walk it avoids is 59% of a 20,348 tk/fr bucket.
NDS_FTPARTS_FLAT_SLOTS ?= 4
# Per-slot capacity for the same table; a slot costs 4*MAX + 12 bytes, so this
# is what decides whether more slots are affordable at all. 96 is the shipped
# bound, chosen when there were four slots and the table was 1,584 bytes total.
# Exceeding it is correct but silently returns to the unflattened walk, which
# gNdsFtPartsFlatOverflows counts and gNdsFtPartsFlatCountMax bounds.
#
# STAYS 96. Shrinking it to 48 was built and measured 2026-09-17 and bought
# NOTHING on any axis: WORK-H P50 +1,216 / P95 -6,016 (both far under the
# 14,080 cross-build floor), and gNdsTaskmanArenaChosenSize, AllocFailCount and
# GeneralHeapFreeMin were all BYTE-IDENTICAL to control, because the arena steps
# in 4,096-byte pages and 768 bytes does not cross one. Every divergence witness
# matched, so it was a valid arm, not a bad one. Against zero measured benefit
# it carries a real risk: a subtree over 48 silently reverts to the unflattened
# walk, and the counter that would catch it (gNdsFtPartsFlatOverflows) has been
# removed. Reverted.
NDS_FTPARTS_FLAT_MAX ?= 96
# P2-2p8 stall budget, falsifier. 1 makes gSYSinTable non-cacheable through its
# own MPU region. The table is exactly 4,096 bytes -- the whole data cache --
# and is indexed randomly, so it both misses constantly and evicts everything
# else. Measured delta solves for the uncached-word cost that every VRAM
# candidate in the ranking is quoted against.
NDS_RENDERER_SCREEN_SPACE_CENSUS ?= 0
# Cycle 116. Same instrument, applied to the Task 56 fighter strips: at 1 the
# raw-corner emitters and the primitive-group emitter are BOTH compiled and
# `gNdsFighterStripRoute` selects between them at run time, so the strip A/B
# runs on ONE binary. Task 56 was killed in 2026-07 against a control built
# three days earlier and ~31 KB smaller, which is not a control at all on a ROM
# whose pacing is placement-sensitive. Needs NDS_FIGHTER_PRIMITIVES >= 1
# for the strip arm to exist; at 0 the route folds away and the selected
# emitter is whatever that flag chose.
NDS_STRIP_ROUTE ?= 0
# P2-2's direct-battle standing gate. Unlike NDS_BOTH_CPU this has no
# historical two-player meaning to preserve: it seeds four level-3 CPU fighter
# INSTANCES (Mario/Fox/Mario/Fox) while the generated renderer owners remain the
# two fighter KINDS. Only the dedicated P2 stress target below enables it.
NDS_FOUR_CPU_STRESS ?= 0
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
# `NDS_FOUR_CPU_ROSTER=0` still builds the mirror arm and is the A/B control;
# the stress harness stamps which roster produced every figure into
# `p2-2-fourcpu-memory.json` (`fighterRoster`), because a tick number from one
# roster is not comparable with a tick number from the other.
#
# The "needs the four-name roster" check is a `#error` in nds_match_config.c
# rather than a `$(error)` here, because NDS_DONKEY is not defined until
# below and a make-time test would read it empty and never fire.
NDS_FOUR_CPU_ROSTER ?= \
	$(if $(filter smash64ds-p2-fourcpu-tickhud-hwtri,$(TARGET)),1,0)
# The four kinds the roster arm instantiates (BattleShip fttypes.h ordinals:
# Mario 0, Fox 1, Donkey 2, Samus 3, Luigi 4, Link 5, Yoshi 6, Captain 7,
# Kirby 8, Pikachu 9, Purin 10, Ness 11). These remain generic defaults for
# fighter-row A/B work; the dedicated P2-2 stress target below overrides them
# to the current pack-capacity argmax. nds_match_config.c refuses a kind whose
# admission flag is off, and the verifier reads these back out of
# nds_build_config.h to name the roster it expects.
NDS_FOUR_CPU_KIND0 ?= 3
NDS_FOUR_CPU_KIND1 ?= 1
NDS_FOUR_CPU_KIND2 ?= 7
NDS_FOUR_CPU_KIND3 ?= 2
# P2-2p8 Phase 1 slice 6 (LAB ONLY, the four-CPU roster arm): `NDS_LAB_FOURCPU_TWO=1`
# keeps roster slots 0/1 and leaves slots 2/3 empty, so the direct battle is a
# two-fighter match -- the source's HIGH-detail case (scvsbattle.c selects Low
# only for 3+ fighters), which a four-fighter match reaches only at pause and
# DeadUp. It measures the high-detail fighter lists on every frame. Empty (the
# default) emits nothing, so every other build's config header is unchanged.
NDS_LAB_FOURCPU_TWO ?=
# P2-2p8 (LAB ONLY, the four-CPU stress target): `NDS_LAB_FOURCPU_SWEEP=1`
# compiles two boot-pokable words into the stress preset
# (src/port/nds_match_config.c), so one ROM sweeps the any-4-fighters x
# any-stage gate (owner 2026-09-27): gNdsLabFourCpuGkind (nGRKind: 0 Castle,
# 1 Sector Z, 2 Jungle, 3 Zebes, 4 Hyrule, 5 Yoshi's Island, 6 Dream Land,
# 7 Saffron, 8 Mushroom Kingdom) and gNdsLabFourCpuKinds (four nFTKind
# bytes, slot 0 lowest; admitted kinds only). 0xFF keeps the preset. Empty
# (the default) emits nothing, so the gate ROM carries neither word.
# `NDS_LAB_FOURCPU_SWEEP_GKIND=<n>` sets the stage word's initial value, for
# harnesses that cannot poke it (the profile census);
# `NDS_LAB_FOURCPU_SWEEP_KINDS=<decimal word>` does the same for the roster,
# and `NDS_LAB_FOURCPU_SWEEP_P4=<decimal word>` for gNdsLabP4Content (one P4
# content a slot; the slot becomes it on its setup parent).
# `NDS_LAB_FOURCPU_WORDS=1` compiles the same words WITHOUT the sweep's lab
# instruments (stage/Fox GX hashes, item/PIM/baked accumulators, lean-event
# and status-change columns): the per-stage worst case on the gate's own
# configuration.
NDS_LAB_FOURCPU_SWEEP ?=
NDS_LAB_FOURCPU_WORDS ?=
# P3 lab: two consoles agree a fixed two-human match at boot and play it in
# lockstep; NDS_NET_LAB_INPUT scripts each console's pad.
NDS_NET_LAB_INPUT ?= 0
# P3 lab: drive the real menus through host/join, lobby, stage and rematch
# on the two-instance harness (src/nds/net/nds_net_autopilot.c).
NDS_NET_LAB_LOBBY ?= 0
# P3 lab: with the autopilot, the guest's radio drops out mid-match (1: for
# 40 s once, past the 30 s stall bound, the match ends; N > 1: for N VBlanks
# every 1,200 batches on each console in turn, which the lockstep must ride
# out; gNdsNetLabDropMode 1 freezes the game thread instead).
NDS_NET_LAB_DROP ?= 0
# P3 lab: with the autopilot, ten matches whose descriptor the host fills in:
# each VS stage in turn, two level-9 CPUs, every item at the highest rate.
NDS_NET_LAB_SWEEP ?= 0
# P3 lab: with the sweep, NDS_NET_LAB_SWEEP_REPRO=N replays reported setup N
# instead (nds_net_lobby.c, ndsNetLabSweepDescriptor). 0 = the sweep.
NDS_NET_LAB_SWEEP_REPRO ?= 0
# P3 lab (plan section 12, P3-0C): the four-fighter gate with the radio
# running. Compiles gNdsNetLabGateRadio; set to 1 at boot, the battle opens a
# room and runs the lockstep as a one-human net match, so the same ROM
# measures with and without the radio.
NDS_LAB_FOURCPU_SWEEP_GKIND ?=
NDS_LAB_FOURCPU_SWEEP_KINDS ?=
NDS_LAB_FOURCPU_SWEEP_P4 ?=
# LAB ONLY: the lean renderer's root-reuse census (ndsFtrLeanRootCensus,
# gNdsLabLeanRootCensus). Empty = not compiled.
NDS_LAB_LEAN_ROOT_CENSUS ?=
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
#
# 2026-09-17: RAISED TO 8. That release condition was met on 2026-09-09 by
# d99a89f8741, "Give character select four fighter slots instead of the whole
# roster" -- entry now takes four fixed 80 KiB blocks, a 64 KiB shared tree and
# an arena reserve, none of which scale with the roster. The ladder was simply
# never re-raised. Verified rather than assumed: the shipping shell probe
# enters, plays, commits and exits the character select with no SIGILL and no
# ABORT (artifacts/verification/2026-09-17_p2-shell_builds/rung8-jigglypuff.txt).
#
# 2026-09-17 LATER: RAISED TO 10. The rungs 9/10 verdict above was WRONG, and
# wrong in a way worth recording so it is not re-derived. It rested on two
# errors: reading scene kind 21 as the character select when 21 is Maps (16 is
# the CSS), and a resident-budget theory that the O1 experiment then refuted by
# returning 20,480 bytes and changing nothing.
#
# Measured with the shell probe's abort breakpoint, all three rungs play a
# COMPLETE LAP -- 16 CSS -> 21 Maps -> 22 VSBattle -> 24 Results -> 16 -- and
# all three then hit the SAME post-lap abort on the second pass:
#
#   rung 8  (already shipping)  SHELLABORT lr=00504340 sp=02fffd9f mode=3 scene=16
#   rung 9  (+ Ness)            SHELLABORT lr=00504340 sp=02fffd9f mode=3 scene=16
#   rung 10 (+ Kirby)           SHELLABORT lr=00504340 sp=02fffd9f mode=3 scene=16
#
# Byte-identical. The abort is PRE-EXISTING and independent of the roster, so
# rungs 9 and 10 are no worse than the rung that already ships. It is
# nondeterministic -- an earlier run of the same rung-9 ELF gave a data abort at
# Maps instead -- so it is corruption or a race, tracked separately. Admitting
# fighters neither causes nor worsens it.
#
# Costs, still true and still worth knowing: Jigglypuff +9,440 B (3 arena
# pages), Ness +22,576 B, Kirby a further 13 pages. None of it blocks anything.
NDS_SHELL_ROSTER ?= 10

# THE ROSTER LADDER, DEFINED ONCE AND EVALUATED IN ALL THREE SHELL TARGETS.
#
# Rung N is N fighters beyond Mario and Fox, cumulatively, so lowering
# NDS_SHELL_ROSTER is how a fighter comes back OUT of the owner ROM without
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
define NDS_SHELL_ROSTER_LADDER
override NDS_LUIGI := $(if $(filter 0,$(NDS_SHELL_ROSTER)),0,1)
override NDS_DONKEY := $(if $(filter 2 3 4 5 6 7 8 9 10,$(NDS_SHELL_ROSTER)),1,0)
override NDS_CAPTAIN := $(if $(filter 3 4 5 6 7 8 9 10,$(NDS_SHELL_ROSTER)),1,0)
override NDS_SAMUS := $(if $(filter 4 5 6 7 8 9 10,$(NDS_SHELL_ROSTER)),1,0)
override NDS_LINK := $(if $(filter 5 6 7 8 9 10,$(NDS_SHELL_ROSTER)),1,0)
override NDS_PIKACHU := $(if $(filter 6 7 8 9 10,$(NDS_SHELL_ROSTER)),1,0)
override NDS_YOSHI := $(if $(filter 7 8 9 10,$(NDS_SHELL_ROSTER)),1,0)
override NDS_PURIN := $(if $(filter 8 9 10,$(NDS_SHELL_ROSTER)),1,0)
override NDS_NESS := $(if $(filter 9 10,$(NDS_SHELL_ROSTER)),1,0)
override NDS_KIRBY := $(if $(filter 10,$(NDS_SHELL_ROSTER)),1,0)
endef
# P2-6, the 1P Game campaign.
#
# The ladder tables and bonus counters come from the textual include of
# sc1pgame.c in battleship_sc1pgame_runtime.c (2026-09-04); the transcribed
# battleship_sc1pgame_tables.c was deleted once the include owned them.
NDS_1P_GAME ?= 0
# The 1P venues -- the five arenas and the 25 bonus boards -- ride the 1P Game
# flag: their NDS_STAGE_<VENUE> defines (the guards every registry site
# carries, see register_native_stage_runtime.py) are derived from
# NDS_1P_GAME in the config header rather than each carrying a ?=.
# Until 2026-09-05 nothing defined them at all, so the five arenas' adapter
# capture tables were compiled out of every configuration.
NDS_1P_STAGE_FLAGS := PUPUPUSMALL YOSTERSMALL METAL ZAKO LAST BONUS3 BONUS1_MARIO BONUS1_FOX BONUS1_DONKEY BONUS1_SAMUS BONUS1_LUIGI BONUS1_LINK BONUS1_YOSHI BONUS1_CAPTAIN BONUS1_KIRBY BONUS1_PIKACHU BONUS1_PURIN BONUS1_NESS BONUS2_MARIO BONUS2_FOX BONUS2_DONKEY BONUS2_SAMUS BONUS2_LUIGI BONUS2_LINK BONUS2_YOSHI BONUS2_CAPTAIN BONUS2_KIRBY BONUS2_PIKACHU BONUS2_PURIN BONUS2_NESS
NDS_LUIGI ?= 0
# Donkey is the first structurally different P2-3 owner.  Keep admission
# sequential: native-owner slots are a dense ABI (Mario/Fox/Luigi/Donkey), so a
# Donkey build also carries the already-qualified Luigi owner instead of
# inventing a hole or a build-dependent owner number.
NDS_DONKEY ?= 0
ifeq ($(NDS_DONKEY),1)
ifneq ($(NDS_LUIGI),1)
$(error NDS_DONKEY=1 requires NDS_LUIGI=1 so native-owner slots stay dense)
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
# worst case NDS_SHELL_ROSTER is sized against is still Luigi versus Donkey
# at 36,276 B of resident owner images; Falcon has none, so admitting him to the
# roster today would put his model on the N64 interpreter path with no
# measurement behind it. Build the slice with:
#
#   make TARGET=smash64ds BUILD=build-<lab> NDS_CAPTAIN=1
#
# THERE IS DELIBERATELY NO "REQUIRES NDS_DONKEY=1" LADDER CHECK HERE, unlike
# the Donkey/Luigi pair above, and the reason is not laziness in both
# directions. That check exists because a native-owner SLOT is a dense ABI, and
# Falcon occupies no slot -- he has no generated owner tables at all. A check
# would also be unreadable at this point in the file: every shell target sets
# NDS_LUIGI/NDS_DONKEY by `override` around line 2500, more than 1,900
# lines BELOW this one, so a test here reads the `?=` defaults and reds on a
# command line that is in fact correct -- the same evaluation-order trap the
# NDS_FOUR_CPU_ROSTER comment above had to push into a C `#error`. Reinstate
# the ladder when Falcon becomes a native owner, and put it where the flags are
# final.
NDS_CAPTAIN ?= 0
# P2-3 Samus source-gameplay admission. Her native owner is source-derived from
# the non-contiguous setup_parts mask; the generator mirrors BattleShip's
# per-descriptor bit walk instead of requiring the older prefix-shaped masks.
NDS_SAMUS ?= 0
# P2-3 fighter 5. Link remains opt-in until his source state machines, real
# boomerang weapon, LinkBomb item client, native owner, CSS/audio surfaces and
# runtime proofs are all admitted. The production manifest may know his files
# before this flips in a shipping shell; that is intentional staging.
NDS_LINK ?= 0
# P2-3 fighter 6. Pikachu stays opt-in until his source specials (Thunder
# Jolt, Thunder, Quick Attack), both weapon articles, native owner, CSS/audio
# surfaces and runtime proofs are admitted; the manifest knows his files first.
NDS_PIKACHU ?= 0
# P2-3 fighter 7. Yoshi stays opt-in until his source specials (Egg Lay's
# two-body capture, Egg Throw, Yoshi Bomb), both weapon articles, the egg
# shield / double-jump armor seams, native owner, CSS/audio surfaces and
# runtime proofs are admitted; the manifest knows his files first.
NDS_YOSHI ?= 0
# P2-3 fighter: Ness stays opt-in until his source specials, articles, native
# owner, CSS/audio surfaces and runtime proofs are admitted (admit_fighter.py).
NDS_NESS ?= 0
# P2-3 fighter: Purin stays opt-in until his source specials, articles, native
# owner, CSS/audio surfaces and runtime proofs are admitted (admit_fighter.py).
NDS_PURIN ?= 0
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
# THE ASSIGNMENT IS DEFERRED ON PURPOSE. NDS_KIRBY is defaulted BELOW this
# line, so a `:=` here would read an empty value for him and silently leave
# his stars stubbed on a Kirby-only build. This is the same trap the
# NDS_CAPTAIN comment above describes for its ladder check, avoided rather
# than worked around: `=` is evaluated where it is used, by which point every
# admission flag is final.
# P2-4 adds a second reason to need the item core, and it is not a fighter:
# Peach's Castle's only hazard is an item. grcastle.c:57 spawns the bumper
# through itManagerMakeItemSetupCommon, so a Castle build without the core
# does not link at all. Mushroom Kingdom and Saffron City will join this list
# for the same reason.
NDS_ITEM_CORE = $(if $(filter 1,$(NDS_LINK) $(NDS_NESS) \
	$(NDS_FOUR_CPU_STRESS) \
	$(NDS_PIKACHU) $(NDS_PURIN) $(NDS_KIRBY) \
	$(NDS_STAGE_CASTLE) \
	$(NDS_STAGE_YAMABUKI) \
	$(NDS_STAGE_INISHIE)),1,0)
# P2-3 fighter: Kirby stays opt-in until his source specials, articles, native
# owner, CSS/audio surfaces and runtime proofs are admitted (admit_fighter.py).
NDS_KIRBY ?= 0
# P2-3 fighter: GDonkey stays opt-in until his source specials, articles, native
# owner, CSS/audio surfaces and runtime proofs are admitted (admit_fighter.py).
NDS_GDONKEY ?= 0
# P2-3 fighter: MMario stays opt-in until his source specials, articles, native
# owner, CSS/audio surfaces and runtime proofs are admitted (admit_fighter.py).
NDS_MMARIO ?= 0
# P2-3 fighter: NMario stays opt-in until his source specials, articles, native
# owner, CSS/audio surfaces and runtime proofs are admitted (admit_fighter.py).
NDS_NMARIO ?= 0
# P2-3 fighter: NFox stays opt-in until his source specials, articles, native
# owner, CSS/audio surfaces and runtime proofs are admitted (admit_fighter.py).
NDS_NFOX ?= 0
# P4 (docs/P4/P4_STATUS.md): Smash Remix fighters, generated at build time from
# the user's own ROM through the staged donor build (scripts/p4). Opt-in until
# admitted; NDS_P4 follows any enabled content. The staging and export roots are
# local, ignored build inputs prepared once by scripts/p4/stage_remix.py and
# scripts/p4/remix_export.py.
# The contents come from scripts/p4/contents.json, one name:NAME:TOKEN:id:BASE
# row each (TOKEN the Remix name, or NAME for a P6 authored content, whose
# BASE is the Remix content it starts from; - for the others);
# NDS_P4_<NAME>=1 compiles one (NDS_P4_FALCO=1, ...).
NDS_P4_REGISTRY := $(shell python "$(PROJECT_ROOT)/scripts/p4/p4_contents.py" --list)
nds_p4_field = $(word $(2),$(subst :, ,$(1)))
$(foreach row,$(NDS_P4_REGISTRY),$(eval NDS_P4_$(call nds_p4_field,$(row),2) ?= 0))
NDS_P4_ENABLED_ROWS := $(foreach row,$(NDS_P4_REGISTRY),$(if $(filter 1,$(NDS_P4_$(call nds_p4_field,$(row),2))),$(row)))
NDS_P4_ENABLED := $(foreach row,$(NDS_P4_ENABLED_ROWS),$(call nds_p4_field,$(row),1))
override NDS_P4 := $(if $(NDS_P4_ENABLED),1,0)
# The source files the enabled contents run (src/port/nds_p4_<name>.c): each
# content's own, or, for an authored content without one, its base's
# (scripts/p4/p4_contents.py code_row), each once.
nds_p4_code_name = $(if $(wildcard $(PROJECT_ROOT)/src/port/nds_p4_$(1).c),$(1),$(if $(filter-out -,$(2)),$(if $(wildcard $(PROJECT_ROOT)/src/port/nds_p4_$(2).c),$(2))))
nds_p4_uniq = $(if $(1),$(firstword $(1)) $(call nds_p4_uniq,$(filter-out $(firstword $(1)),$(1))))
NDS_P4_CODE_NAMES := $(call nds_p4_uniq,$(foreach row,$(NDS_P4_ENABLED_ROWS),$(call nds_p4_code_name,$(call nds_p4_field,$(row),1),$(call nds_p4_field,$(row),5))))
# S12: the EXTRA staging (scripts/p4/stage_extra.py) is Remix at the same pin
# plus EXTRA's appended Meta Knight, MRGAW and Snake; the fifteen main-tree
# contents generate the same data from it as from Remix's own, but for the
# Remix RAM addresses a few synthesized scripts carry (2026-10-11).
NDS_P4_STAGING ?= $(PROJECT_ROOT)/builds/p4-staging/extra-96621af
NDS_P4_EXPORT ?= $(PROJECT_ROOT)/builds/p4-staging/export-x
# Lab builds only: a donor status routine with no DS port yet runs a stand-in
# (generate_p4_fighter.py --lab-fallback, listed in each manifest) instead of
# failing the generator, so every content can be built and probed early.
NDS_P4_LAB_FALLBACK ?= 0
# P5 (docs/P5/New_Stages.md): the new VS stages and the 6x3 stage select,
# from scripts/p5/stages.json; Remix's select art comes from the P4 staging
# above and Meta Crystal, Duel Zone and Final Destination are the 1P venues
# (NDS_1P_GAME). Opt-in until admitted.
NDS_P5 ?= 0
# P2-3 fighter: NDonkey stays opt-in until his source specials, articles, native
# owner, CSS/audio surfaces and runtime proofs are admitted (admit_fighter.py).
NDS_NDONKEY ?= 0
# P2-3 fighter: NSamus stays opt-in until his source specials, articles, native
# owner, CSS/audio surfaces and runtime proofs are admitted (admit_fighter.py).
NDS_NSAMUS ?= 0
# P2-3 fighter: NLuigi stays opt-in until his source specials, articles, native
# owner, CSS/audio surfaces and runtime proofs are admitted (admit_fighter.py).
NDS_NLUIGI ?= 0
# P2-3 fighter: NLink stays opt-in until his source specials, articles, native
# owner, CSS/audio surfaces and runtime proofs are admitted (admit_fighter.py).
NDS_NLINK ?= 0
# P2-3 fighter: NYoshi stays opt-in until his source specials, articles, native
# owner, CSS/audio surfaces and runtime proofs are admitted (admit_fighter.py).
NDS_NYOSHI ?= 0
# P2-3 fighter: NCaptain stays opt-in until his source specials, articles, native
# owner, CSS/audio surfaces and runtime proofs are admitted (admit_fighter.py).
NDS_NCAPTAIN ?= 0
# P2-3 fighter: NKirby stays opt-in until his source specials, articles, native
# owner, CSS/audio surfaces and runtime proofs are admitted (admit_fighter.py).
NDS_NKIRBY ?= 0
# P2-3 fighter: NPikachu stays opt-in until his source specials, articles, native
# owner, CSS/audio surfaces and runtime proofs are admitted (admit_fighter.py).
NDS_NPIKACHU ?= 0
# P2-3 fighter: NPurin stays opt-in until his source specials, articles, native
# owner, CSS/audio surfaces and runtime proofs are admitted (admit_fighter.py).
NDS_NPURIN ?= 0
# P2-3 fighter: NNess stays opt-in until his source specials, articles, native
# owner, CSS/audio surfaces and runtime proofs are admitted (admit_fighter.py).
NDS_NNESS ?= 0
# P2-4 first stage: Yoshi's Island (Yoster). It belongs HERE with the other
# admission flags and not down beside its reloc file list, because the CFILES
# gate at :3851 and the nds_build_config.h line both read it before that point
# in the file. A `?=` placed later left the config header emitting
# `#define NDS_STAGE_YOSTER` with an EMPTY value, which is not the same as 0
# -- `#if` on an empty macro is "#if with no expression", a hard compile error,
# and `#if !` on one is "operator '!' has no right operand". Both fired on the
# default build.
NDS_STAGE_YOSTER ?= 0
# P2-4 stage 2: Peach's Castle (Castle). Same reason as Yoster above for
# living here rather than beside its reloc list.
NDS_STAGE_CASTLE ?= 0
# P2-4 stage 3: Congo Jungle (Jungle). Same reason as the two above for
# living here rather than beside its reloc list.
NDS_STAGE_JUNGLE ?= 0
# P2-4 stage 4: Planet Zebes (Zebes).
NDS_STAGE_ZEBES ?= 0
# P2-4 stage 5: Hyrule Castle (Hyrule).
NDS_STAGE_HYRULE ?= 0
# P2-4 stage 6: Saffron City (Yamabuki).
NDS_STAGE_YAMABUKI ?= 0
# P2-4 stage 7: Mushroom Kingdom (Inishie).
NDS_STAGE_INISHIE ?= 0
# P2-4 stage 8: Sector Z (Sector), the largest stage in the game.
NDS_STAGE_SECTOR ?= 0
NDS_COMPACT_BATTLE_FIGHTERS ?= 0
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
NDS_NATIVE_OWNER_IMAGE_LUIGI = $(if $(filter 1,$(NDS_NATIVE_OWNER_IMAGE)),$(NDS_LUIGI),0)
NDS_NATIVE_OWNER_IMAGE_DONKEY = $(if $(filter 1,$(NDS_NATIVE_OWNER_IMAGE)),$(NDS_DONKEY),0)
NDS_NATIVE_OWNER_IMAGE_CAPTAIN = $(if $(filter 1,$(NDS_NATIVE_OWNER_IMAGE)),$(NDS_CAPTAIN),0)
NDS_NATIVE_OWNER_IMAGE_SAMUS = $(if $(filter 1,$(NDS_NATIVE_OWNER_IMAGE)),$(NDS_SAMUS),0)
NDS_NATIVE_OWNER_IMAGE_LINK = $(if $(filter 1,$(NDS_NATIVE_OWNER_IMAGE)),$(NDS_LINK),0)
NDS_NATIVE_OWNER_IMAGE_PIKACHU = $(if $(filter 1,$(NDS_NATIVE_OWNER_IMAGE)),$(NDS_PIKACHU),0)
NDS_NATIVE_OWNER_IMAGE_YOSHI = $(if $(filter 1,$(NDS_NATIVE_OWNER_IMAGE)),$(NDS_YOSHI),0)
NDS_NATIVE_OWNER_IMAGE_NESS = $(if $(filter 1,$(NDS_NATIVE_OWNER_IMAGE)),$(NDS_NESS),0)
NDS_NATIVE_OWNER_IMAGE_PURIN = $(if $(filter 1,$(NDS_NATIVE_OWNER_IMAGE)),$(NDS_PURIN),0)
NDS_NATIVE_OWNER_IMAGE_KIRBY = $(if $(filter 1,$(NDS_NATIVE_OWNER_IMAGE)),$(NDS_KIRBY),0)
NDS_NATIVE_OWNER_IMAGE_MMARIO = $(if $(filter 1,$(NDS_NATIVE_OWNER_IMAGE)),$(NDS_MMARIO),0)
NDS_NATIVE_OWNER_IMAGE_NMARIO = $(if $(filter 1,$(NDS_NATIVE_OWNER_IMAGE)),$(NDS_NMARIO),0)
NDS_NATIVE_OWNER_IMAGE_NFOX = $(if $(filter 1,$(NDS_NATIVE_OWNER_IMAGE)),$(NDS_NFOX),0)
NDS_NATIVE_OWNER_IMAGE_NDONKEY = $(if $(filter 1,$(NDS_NATIVE_OWNER_IMAGE)),$(NDS_NDONKEY),0)
NDS_NATIVE_OWNER_IMAGE_NSAMUS = $(if $(filter 1,$(NDS_NATIVE_OWNER_IMAGE)),$(NDS_NSAMUS),0)
NDS_NATIVE_OWNER_IMAGE_NLINK = $(if $(filter 1,$(NDS_NATIVE_OWNER_IMAGE)),$(NDS_NLINK),0)
NDS_NATIVE_OWNER_IMAGE_NYOSHI = $(if $(filter 1,$(NDS_NATIVE_OWNER_IMAGE)),$(NDS_NYOSHI),0)
NDS_NATIVE_OWNER_IMAGE_NCAPTAIN = $(if $(filter 1,$(NDS_NATIVE_OWNER_IMAGE)),$(NDS_NCAPTAIN),0)
NDS_NATIVE_OWNER_IMAGE_NKIRBY = $(if $(filter 1,$(NDS_NATIVE_OWNER_IMAGE)),$(NDS_NKIRBY),0)
NDS_NATIVE_OWNER_IMAGE_NPIKACHU = $(if $(filter 1,$(NDS_NATIVE_OWNER_IMAGE)),$(NDS_NPIKACHU),0)
NDS_NATIVE_OWNER_IMAGE_NPURIN = $(if $(filter 1,$(NDS_NATIVE_OWNER_IMAGE)),$(NDS_NPURIN),0)
NDS_NATIVE_OWNER_IMAGE_NNESS = $(if $(filter 1,$(NDS_NATIVE_OWNER_IMAGE)),$(NDS_NNESS),0)
# Mario's and Fox's shared table set (P8 R7): in every build, so imaged
# exactly when images are.
NDS_NATIVE_OWNER_IMAGE_MARIOFOX = $(if $(filter 1,$(NDS_NATIVE_OWNER_IMAGE)),1,0)
# Giant DK's echo landed (0b564186676) without this line, so the config header
# emitted `#define NDS_NATIVE_OWNER_IMAGE_GDONKEY` with an EMPTY value -- the
# same "#if with no expression" trap as NDS_STAGE_YOSTER above, latent only
# because no source tests it yet. check_build_flag_census.py reports EMPTY.
NDS_NATIVE_OWNER_IMAGE_GDONKEY = $(if $(filter 1,$(NDS_NATIVE_OWNER_IMAGE)),$(NDS_GDONKEY),0)
# Master Hand (P2-6): the boss owner export tests this flag in the generated
# owner include; he rides the 1P Game flag like his reloc files.
NDS_NATIVE_OWNER_IMAGE_BOSS = $(if $(filter 1,$(NDS_NATIVE_OWNER_IMAGE)),$(NDS_1P_GAME),0)
# Ness Up-B VFX-only proof.  This keeps the normal Ness source-special tour's
# immediate PK Thunder self-hit steering unchanged; when enabled, the proof
# controller lets PK Thunder fly naturally so head, spawned trail and attached
# Wave can each receive presented frames before the verifier exits.
NDS_NESS_VFX_PROOF ?= 0
ifneq ($(filter 0 1,$(NDS_NESS_VFX_PROOF)),$(NDS_NESS_VFX_PROOF))
$(error NDS_NESS_VFX_PROOF must be 0 or 1)
endif
ifeq ($(NDS_NESS_VFX_PROOF),1)
ifneq ($(NDS_NESS),1)
$(error NDS_NESS_VFX_PROOF=1 requires NDS_NESS=1)
endif
$(error NDS_NESS_VFX_PROOF=1 needs a slot-0 proof fighter; that selector is gone)
endif
# Yoshi BUGS.md visual proof.  This is a controller-only verifier arm: the
# source game still owns SpecialHi / SpecialN statuses, EggThrow, EggLay,
# collision, animation, lifetimes and renderer submission.
NDS_YOSHI_BUG_PROOF ?= 0
ifneq ($(filter 0 1,$(NDS_YOSHI_BUG_PROOF)),$(NDS_YOSHI_BUG_PROOF))
$(error NDS_YOSHI_BUG_PROOF must be 0 or 1)
endif
ifeq ($(NDS_YOSHI_BUG_PROOF),1)
ifneq ($(NDS_YOSHI),1)
$(error NDS_YOSHI_BUG_PROOF=1 requires NDS_YOSHI=1)
endif
$(error NDS_YOSHI_BUG_PROOF=1 needs a slot-0 proof fighter; that selector is gone)
endif
# THE FREEZE SOAK'S MATCH LENGTH IN MINUTES, and nothing else's. 0 = leave the
# harness seeding alone, which is the canonical one-minute Time match; non-zero
# overrides scene_harness.c's time_limit.
#
# It exists because this used to ride on NDS_BOTH_CPU: that branch seeded a
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
NDS_SOAK_MATCH_MINUTES ?= 0
# Task 106 E0: logical updates per presented frame. Ships at 2 (the source 60 Hz
# simulation against a 30 Hz present). Building with 1 prices what a 30 Hz
# simulation would save; it is a sizing arm only, uncompensated, and plays at
# half speed. Do not publish a ROM built with 1.
NDS_UPDATES_PER_PRESENT ?= 2
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
NDS_FTR_DRAW_MEMO ?= 1
# P2-1c. The VS shell's 2D UI kit: the SSB64 menu font, the hand cursor, the
# Mario/Fox CSS portraits, menu SFX, and the retained text/sprite slots
# P2-1d/1e/1f draw their screens out of. 0 compiles the whole thing out --
# src/nds/nds_ui_kit.c is one #if, the NitroFS pack below is not staged, and
# both published ROMs are byte-identical to a build without the row. P2-1d
# turns it on for real.
NDS_UI_KIT ?= 0
# P2-1d. The VS shell's real screens: splash, title, main menu, and the VS menu
# that carries the rules (src/nds/nds_menu_shell.c). It turns the kit above on
# for the first time, replaces the bounded Startup/Title/VSMode park branches
# with screens the player drives, gives nSCKindModeSelect a scene where it had
# an NDS_SCENE_STUB, and moves the mode-163 BOOT SCENE from the match to the
# splash. That last one is why it is a flag and not a default: at 0 -- every
# published and Boundary configuration -- mode 163 boots straight into the
# battle exactly as it always has, no menu symbol is linked, and the NitroFS
# pack is not staged.
NDS_MENU_SHELL ?= 0
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
NDS_MENU_WALK ?= 0
# LAB ONLY. 1 builds the 1P intro with its live fighter cards and the gdb-driven
# display-capture hook that bakes the shipping intro's fighter stills
# (scripts/menus/bake_1p_intro_stills.ps1). Never 1 in anything published.
NDS_1P_INTRO_BAKE ?= 0
# LAB ONLY (with NDS_MENU_WALK): the 1P ladder stage the walk starts at; -1
# is the source's own start. For instruments that run without a debugger.
NDS_1P_WALK_START_STAGE ?= -1
# PLAYTEST ROMs only (owner 2026-10-06: "a way to easily playtest masterhand
# without going through the whole 1p campaign"): a 1P game that starts at the
# first stage starts at this ladder stage instead (13 = Master Hand); -1 is
# the source's own ladder. Never set in the published ROM.
NDS_1P_PLAYTEST_START_STAGE ?= -1
# The bake runs the live intro, whose Link needs the intro root program; no
# shipping scene does (generate_nds_native_owners.py LINK_INTRO_PROGRAM). The
# owners are generated into the shared source tree, so the value is stamped
# there and a change of value regenerates them before anything compiles.
export NDS_OWNER_LINK_INTRO := $(NDS_1P_INTRO_BAKE)
