# Knob validation and the target families: what each TARGET overrides
# (the shipping smash64ds, the four-CPU gate, the shell and lab targets).
# Included in place from the Makefile (P8 G); its text is unchanged.

# NDS_FIGHTER_PRIMITIVES (the fighter primitive streams) must be 0, 1, or 2.
ifneq ($(NDS_FIGHTER_PRIMITIVES),$(filter $(NDS_FIGHTER_PRIMITIVES),0 1 2))
$(error NDS_FIGHTER_PRIMITIVES must be 0, 1, or 2; got "$(NDS_FIGHTER_PRIMITIVES)")
endif
NDS_RENDERER_FAST_RUN_DEFAULT ?= 0
# R1 retirement, done (2026-10-20, old machinery): fighter hierarchy mode 7, the
# per-root hardware executor, the CPU triangle rasteriser and the resident
# raw-path tables are gone from the sources; every target builds the production
# fighter path, which nds_renderer.h still checks (profile 0, mode 8/9,
# NO_ORACLE, HW light, the fighter primitive streams).
override NDS_FIGHTER_LEGACY_EXEC := 0
ifeq ($(TARGET),smash64ds-p2-fourcpu-tickhud-hwtri)
# Profile-0 shipping path plus either the lightweight Task 41 timers or the
# full diagnostic publications required by GDB proof runs.
#
override NDS_DEV_SCENE_HARNESS := battle_playable_realtime
override NDS_DEV_LIVE_INPUT_PREVIEW := 1
override NDS_HARNESS_FAST_LOGIC := 0
override NDS_RENDERER_HW_TRIANGLES := 1
override NDS_RENDERER_PROFILE_LEVEL := 0
override NDS_SHIP_TELEMETRY := 0
override NDS_TICK_HUD := 1
override NDS_RENDERER_FAST_RUN_DEFAULT := 9
override NDS_NATIVE_STAGE_GENERATED_SEGMENT0_ENABLE := 1
override NDS_STAGE_HW_COMPOSE := 2
# R2-08: THE SWITCH (SwitchPlan §6), the measurement/proof half. This block must
# carry every published flag or the instrument measures a path the user is not
# running, and check-tickhud-parity.ps1 fails a one-sided edit (BENCH_MAKE_PATH,
# Makefile:4203).
ifneq ($(NDS_LAB_R1_PATH),1)
override NDS_PATH := 1
endif
# Slice 44. This is the instrument every measurement runs on, so it has to stay
# flag-identical to the published block -- a stride on one and not the other
# would put ~35,900 of WORK-H P95 between the ROM being judged and the ROM
# doing the judging.
override NDS_STAGE_VALIDATE_STRIDE := 8
override NDS_FIGHTER_HW_LIGHT := 1
# R2-03 E64b/E65. See the published block, including how the equivalence was
# finally settled -- with a host error bound, not the state hash.
ifneq ($(NDS_LAB_CUBIC_OFF),1)
endif
# Task 44: stage steady-state admission + dense binding lists. Exact (no
# fidelity change); ships on with Task 36 replay.
override NDS_STAGE_STEADY := 1
# Must track the published block above. These two targets exist to measure and
# prove the shipping program, so any flag that is on there and off here makes
# every tick-HUD bucket and every GDB proof a reading of a different binary.
# Track the published placement exactly (2026-08-30 rebank: libc + port).
override NDS_ITCM_LIB_LEAVES := 5
override NDS_RENDERER_BATTLE_STATIC_TEXTURE_DEFAULT := 1
ifeq ($(TARGET),smash64ds-p2-fourcpu-tickhud-hwtri)
# P2-2 standing stress arm. It deliberately boots straight into the source
# VSBattle path instead of walking the CSS: the gate is measuring four-fighter
# gameplay, not menu automation. Restore the source effect capacity too; a
# four-way burst measured through P1's reduced 12-entry pool would be a different
# game-state policy, not merely a tighter memory budget.
override NDS_FOUR_CPU_STRESS := 1
override NDS_COMPACT_BATTLE_FIGHTERS := 1
override NDS_EFFECT_POOL := 38
# P2-2p8 Phase 0 (docs/p2/FOUR_FIGHTER_30FPS_ARCHITECTURE.md A9): the gate
# measures the game, not the instrument. The on-screen tick-HUD block cost
# ~345-450K ticks on every ninth or tenth presented frame -- inside WORK-H and
# on exactly the frames P95 is decided on -- and the GDB sampler reads the ring,
# never the printed table. The published ROM has no such block.
override NDS_TICK_HUD_DRAW := 0
# The FPS console's periodic text, the same A9 class: five iprintf lines every
# half second, +25-40K ticks on one presented frame in fifteen.
override NDS_BATTLE_FPS_HUD_DRAW := 0
# D12a: the fine span clocks too (~120 reads a frame); a run that wants the
# per-bucket breakdown pokes gNdsTickHudSpans=1 at boot.
override NDS_TICK_HUD_SPANS_DEFAULT := 0
# 2026-10-18: the ring's bss is battle heap the published ROM has (the arena
# follows .main.bss). 34 buckets x 128 samples were 17,408 B, and in the ship
# layout the gate ROM declined stage GX programs and shrank the motion cache
# where the play ROM did not. 32 samples a ring stop give 13,056 B back.
ifneq ($(origin NDS_TICK_HUD_WINDOW),command line)
NDS_TICK_HUD_WINDOW := 32
endif
ifeq ($(NDS_FOUR_CPU_ROSTER),1)
# P2-3r15: THIS IS NOW THE DEFAULT ARM, not the lab arm -- the flag defaults to
# 1 on this target (see its declaration above). `NDS_FOUR_CPU_ROSTER=0`
# rebuilds the Mario/Fox mirror roster and is the control every A/B against a
# pre-2026-08-25 figure has to use.
#
# P2-3r13. THE ROSTER ARM IS SHIPPING-SHAPED: NOTHING IS OVERRIDDEN HERE
# EXCEPT THE ROSTER ITSELF.
#
# P2-3r11 got four distinct kinds through a whole match by setting
# `NDS_BATTLEPACK := 0` on this arm plus a 32,768 B animation-cache trim,
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
override NDS_LUIGI := 1
override NDS_DONKEY := 1
override NDS_CAPTAIN := 1
override NDS_SAMUS := 1
# P2-2 capacity discriminator (2026-09-10 semantic-pack census): the resolved
# lower endpoint peaks at Donkey/Samus/Link/Kirby. Keep the older landed owner
# admissions above compiled into this shipping-shaped stress build, but
# instantiate the exact four kinds whose resident set the capacity row judges.
override NDS_LINK := 1
override NDS_KIRBY := 1
# The four kinds are pinned because the capacity row judges this exact set. They
# are overridable ONLY through NDS_LAB_FOURCPU_KINDS, and only to answer a
# question the capacity argmax does not: whether the PERFORMANCE gap is
# roster-dependent. Every tick figure in P2-2p8 is measured on this one roster,
# while the contract is every legal four-fighter lineup on every selectable VS
# stage, and this campaign has already seen a bank accepted at -8,096 on
# Mario+Fox cost +22,848 at four. Capacity argmax is not frame-cost argmax.
#
# Format: four space-separated BattleShip fttypes.h ordinals, e.g.
# `NDS_LAB_FOURCPU_KINDS="7 4 2 8"` for Captain/Luigi/Donkey/Kirby. Only kinds
# whose NDS_<NAME> admission is 1 above can be instantiated -- this target
# admits Captain, Donkey, Kirby, Link, Luigi and Samus -- and nds_match_config.c
# refuses any other. Empty (the default) keeps the pinned capacity roster, so a
# normal build and the registry gate are byte-unchanged.
NDS_LAB_FOURCPU_KINDS ?=
ifeq ($(strip $(NDS_LAB_FOURCPU_KINDS)),)
override NDS_FOUR_CPU_KIND0 := 2
override NDS_FOUR_CPU_KIND1 := 3
override NDS_FOUR_CPU_KIND2 := 5
override NDS_FOUR_CPU_KIND3 := 8
else
override NDS_FOUR_CPU_KIND0 := $(word 1,$(NDS_LAB_FOURCPU_KINDS))
override NDS_FOUR_CPU_KIND1 := $(word 2,$(NDS_LAB_FOURCPU_KINDS))
override NDS_FOUR_CPU_KIND2 := $(word 3,$(NDS_LAB_FOURCPU_KINDS))
override NDS_FOUR_CPU_KIND3 := $(word 4,$(NDS_LAB_FOURCPU_KINDS))
endif
endif
endif
endif
# P2-1d VS shell lab target: the playable menu flow, in the SHIPPING battle
# configuration.
#
# It is the proof block with three flags added -- NDS_UI_KIT,
# NDS_MENU_SHELL and NDS_MENU_WALK -- and nothing removed, which is the
# whole point. P2-1c's kit target could use `normal` because it drew a surface
# and touched no battle; this row's flow ENDS IN THE MATCH, so the match it
# reaches has to be the canonical one or the run proves nothing about entering
# a battle through menus. Harness 163 keeps the ARM renderer, the static
# texture pack, and every gameplay flag the Boundary arm carries; the shell
# flag moves only the BOOT SCENE, from the battle to the splash.
#
# NDS_MENU_WALK := 1 drives the screens' own input handlers through one
# scripted pass with a dwell on each screen, so a single run is both the
# cadence measurement and the capture window for every screenshot. The battle
# it enters then runs for real at NDS_HARNESS_FAST_LOGIC := 0.
#
# NEVER PUBLISHED. The boot scene differs from the shipped ROM's, so no tick
# figure from it is a Boundary figure.
#
# P2-1d/1e/1f EACH ADDED A NAME, NOT A BLOCK -- the character select and the
# stage select are more screens inside NDS_MENU_SHELL and wanted exactly
# these flags, so all three selected this one block and the only measurable
# difference between them was the ROM's filename.
#
# P2-1g COLLAPSES THE THREE INTO ONE PHASE NAME. Every row in P2-1 is closed,
# the shell is one screen set rather than three in progress, and a per-row lab
# name outliving its row is scaffolding: the next reader cannot tell which of
# three identical ROMs is current. `smash64ds-p2-shell-hwtri` is the shipping-
# configuration shell arm -- the cadence, screenshot and realtime-through-menus
# surface the phase closes on -- and the loop block below carries the other.
NDS_MENU_SHELL_TARGETS := \
	smash64ds-p2-shell-hwtri
ifneq ($(filter $(TARGET),$(NDS_MENU_SHELL_TARGETS)),)
override NDS_DEV_SCENE_HARNESS := battle_playable_realtime
override NDS_DEV_LIVE_INPUT_PREVIEW := 1
override NDS_HARNESS_FAST_LOGIC := 0
override NDS_RENDERER_HW_TRIANGLES := 1
override NDS_RENDERER_PROFILE_LEVEL := 0
override NDS_TICK_HUD := 0
override NDS_SHIP_TELEMETRY := 1
override NDS_RENDERER_FAST_RUN_DEFAULT := 9
override NDS_NATIVE_STAGE_GENERATED_SEGMENT0_ENABLE := 1
override NDS_STAGE_HW_COMPOSE := 2
override NDS_STAGE_VALIDATE_STRIDE := 8
override NDS_FIGHTER_HW_LIGHT := 1
override NDS_STAGE_STEADY := 1
override NDS_ITCM_LIB_LEAVES := 5
override NDS_RENDERER_BATTLE_STATIC_TEXTURE_DEFAULT := 1
override NDS_UI_KIT := 1
override NDS_MENU_SHELL := 1
# The roster ladder; it is defined once, near NDS_SHELL_ROSTER.
$(eval $(NDS_SHELL_ROSTER_LADDER))
# The eight opt-in VS stages. Overridden HERE rather than defaulted to 1 in
# the flag block, because a global default would also grow the P1 proof ROM
# and every lab target that names no stage flag. All eight boot and play
# under the scripted lap with stage identity asserted; the item core derives
# itself from them, so the items come with them.
override NDS_STAGE_YOSTER := 1
override NDS_STAGE_CASTLE := 1
override NDS_STAGE_JUNGLE := 1
override NDS_STAGE_ZEBES := 1
override NDS_STAGE_HYRULE := 1
override NDS_STAGE_YAMABUKI := 1
override NDS_STAGE_INISHIE := 1
override NDS_STAGE_SECTOR := 1
## P2-2 source parity. BattleShip's efmanager.c owns 38 EFStructs and keeps its
## own last-four forced-effect reserve. The old 12-entry P1 cap changes which
## cosmetic effects survive a four-way burst, so the four-fighter shell restores
## the source depth; the P1 direct-boot/perf targets keep their measured cap.
override NDS_EFFECT_POOL := 38
override NDS_MENU_WALK := 1
# P2-1M gate catch (2026-08-19): the P1 demo ladder is direct-boot demo
# behaviour. In the shell game the CHARACTER SELECT decides Fox's level, so
# the ladder must not ride any shell configuration; the gcrunall pin derives
# its expected level from this define and the CSS commits 3.
override NDS_DEMO_FOX_CPU_LADDER := 0
endif
# P2-1L item (10) FREE-PLAY lab target: the shipping-configuration shell
# above, minus the one flag that makes that ROM scripted. It is a full copy
# of the NDS_MENU_SHELL_TARGETS block rather than a delta on it, on
# purpose -- P2-1b's own comment (above) already ruled out appending a name
# to a shared filter list, and the loop-verifier block below is the
# precedent for "a distinct shell variant gets its own full block".
#
# NDS_MENU_WALK IS NOT OVERRIDDEN, so it stays the Makefile default (0)
# -- and 0 is not merely inert, it is ABSENT. Every walk site is
# `#if NDS_MENU_WALK`: the dwell scripts (kNdsMenuWalk*), the tap
# injector (ndsMenuShellWalkTap, nds_menu_shell.c:485), and the Results
# auto-START (ndsMenuShellWalkWantsResultsStart, :580, and its
# NDS_MENU_SHELL && NDS_MENU_WALK call site in
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
# battle arm runs this configuration plus NDS_MENU_WALK (unattended pass),
# so the published ROM is verifier-covered per the publish law. No tick
# figure from the free-play name is a Boundary figure; the gate measures its
# own walk arm.
NDS_MENU_SHELL_FREEPLAY_TARGETS := \
	smash64ds-p2-shell-freeplay-hwtri \
	smash64ds
ifneq ($(filter $(TARGET),$(NDS_MENU_SHELL_FREEPLAY_TARGETS)),)
override NDS_DEV_SCENE_HARNESS := battle_playable_realtime
override NDS_DEV_LIVE_INPUT_PREVIEW := 1
override NDS_HARNESS_FAST_LOGIC := 0
override NDS_RENDERER_HW_TRIANGLES := 1
override NDS_RENDERER_PROFILE_LEVEL := 0
override NDS_TICK_HUD := 0
override NDS_SHIP_TELEMETRY := 1
override NDS_RENDERER_FAST_RUN_DEFAULT := 9
override NDS_NATIVE_STAGE_GENERATED_SEGMENT0_ENABLE := 1
override NDS_STAGE_HW_COMPOSE := 2
# Runtime 2 is the accepted published battle path. Keep the free-play/root
# shell on the same path as the published battle and realtime proof targets;
# otherwise menu-launched matches silently fall back to the retired R1 loop.
ifneq ($(NDS_LAB_R1_PATH),1)
override NDS_PATH := 1
endif
override NDS_STAGE_VALIDATE_STRIDE := 8
override NDS_FIGHTER_HW_LIGHT := 1
override NDS_STAGE_STEADY := 1
override NDS_ITCM_LIB_LEAVES := 5
override NDS_RENDERER_BATTLE_STATIC_TEXTURE_DEFAULT := 1
override NDS_UI_KIT := 1
override NDS_MENU_SHELL := 1
# The published owner ROM is the all-content build. Keep the 1P campaign
# inseparable from the free-play shell so `make` cannot publish a VS-complete
# ROM that silently omits 1P Game, its campaign-only opponents, bonus boards,
# Final Destination/Master Hand, stage-clear/continue scenes, or campaign BGM.
override NDS_1P_GAME := 1
# The all-content shell must use the source-exact compact battle packs. Raw
# Main/Model residency exhausts the taskman arena after Ness before Fox's
# 116,752-byte Main allocation; the compact path is the existing full-roster
# battle representation and preserves the same fighter content.
override NDS_COMPACT_BATTLE_FIGHTERS := 1
# The roster ladder; it is defined once, near NDS_SHELL_ROSTER.
$(eval $(NDS_SHELL_ROSTER_LADDER))
# The eight opt-in VS stages. Overridden HERE rather than defaulted to 1 in
# the flag block, because a global default would also grow the P1 proof ROM
# and every lab target that names no stage flag. All eight boot and play
# under the scripted lap with stage identity asserted; the item core derives
# itself from them, so the items come with them.
override NDS_STAGE_YOSTER := 1
override NDS_STAGE_CASTLE := 1
override NDS_STAGE_JUNGLE := 1
override NDS_STAGE_ZEBES := 1
override NDS_STAGE_HYRULE := 1
override NDS_STAGE_YAMABUKI := 1
override NDS_STAGE_INISHIE := 1
override NDS_STAGE_SECTOR := 1
override NDS_EFFECT_POOL := 38
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
# NDS_SCENE_LOOP_WALK := 0, AND THAT IS THE ROW'S POINT. P2-1b's substitute
# hop used to carry the Results -> menu leg, which closed a lap without ever
# running `ndsMNVSResultsSetLoadScene` -- the very function P2-1f rewrote for
# the shell and could not exercise. The walk now presses START on Results
# through the real keypad latch (`ndsMenuShellWalkWantsResultsStart`), so the
# lap closes through the source's own exit test and the rematch body runs on
# every one of the twenty laps. Every other leg already had a non-walk path:
# VS START and the stage select's confirm are the shell's own transitions, and
# battle -> Results is the source's (scvsbattle.c:560).
#
# NDS_MENU_WALK := 20 is the phase-close gate written into the ROM, and
# `gNdsMenuShellWalkBudget` makes it a SEED rather than a pin -- a smoke run at
# three laps pokes that variable and costs no build.
#
# NEVER PUBLISHED.
NDS_MENU_WALK_TARGETS := \
	smash64ds-p2-shell-loop-hwtri
ifneq ($(filter $(TARGET),$(NDS_MENU_WALK_TARGETS)),)
override NDS_MENU_WALK := 20
override NDS_DEV_SCENE_HARNESS := battle_playable_realtime
override NDS_DEV_LIVE_INPUT_PREVIEW := 1
override NDS_HARNESS_FAST_LOGIC := 1
override NDS_RENDERER_HW_TRIANGLES := 1
override NDS_RENDERER_PROFILE_LEVEL := 0
override NDS_SHIP_TELEMETRY := 1
override NDS_TICK_HUD := 0
override NDS_RENDERER_FAST_RUN_DEFAULT := 9
override NDS_NATIVE_STAGE_GENERATED_SEGMENT0_ENABLE := 1
override NDS_STAGE_HW_COMPOSE := 2
override NDS_STAGE_VALIDATE_STRIDE := 8
override NDS_FIGHTER_HW_LIGHT := 1
override NDS_STAGE_STEADY := 1
override NDS_ITCM_LIB_LEAVES := 5
override NDS_RENDERER_BATTLE_STATIC_TEXTURE_DEFAULT := 1
override NDS_UI_KIT := 1
override NDS_MENU_SHELL := 1
# The roster ladder; it is defined once, near NDS_SHELL_ROSTER.
$(eval $(NDS_SHELL_ROSTER_LADDER))
# The eight opt-in VS stages. Overridden HERE rather than defaulted to 1 in
# the flag block, because a global default would also grow the P1 proof ROM
# and every lab target that names no stage flag. All eight boot and play
# under the scripted lap with stage identity asserted; the item core derives
# itself from them, so the items come with them.
override NDS_STAGE_YOSTER := 1
override NDS_STAGE_CASTLE := 1
override NDS_STAGE_JUNGLE := 1
override NDS_STAGE_ZEBES := 1
override NDS_STAGE_HYRULE := 1
override NDS_STAGE_YAMABUKI := 1
override NDS_STAGE_INISHIE := 1
override NDS_STAGE_SECTOR := 1
override NDS_EFFECT_POOL := 38
endif
# NDS_SOURCE_EFFECTS_FULL is GONE (2026-08-04). It gated the six DObj-tree
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
NDS_EFFECT_POOL ?= 12
# Checked here, after every target block has applied its overrides, so a target
# that turns both on is accepted while a bare command-line NDS_STAGE_STEADY=1
# without the Task 36 stage owner still fails loudly.
ifeq ($(NDS_STAGE_STEADY),1)
ifeq ($(NDS_STAGE_HW_COMPOSE),0)
$(error NDS_STAGE_STEADY=1 requires NDS_STAGE_HW_COMPOSE)
endif
endif
# The stage replay arena needs the Task 36 compose every target forces
# (NDS_STAGE_HW_COMPOSE := 2 in its block). Checked after the overrides, as
# the Task 44 check above is, or the ?= 0 default at the top would be read.
ifneq ($(NDS_STAGE_HW_COMPOSE),2)
$(error the stage replay arena needs NDS_STAGE_HW_COMPOSE=2)
endif
NDS_ENABLE_INISHIE_SOURCE_SCALE_SETUP ?= 0
NITROFS_DIR := $(PROJECT_ROOT)/$(BUILD)/nitrofs
NITRO_FILES := $(NITROFS_DIR)

