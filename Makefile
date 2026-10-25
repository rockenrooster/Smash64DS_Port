# Nintendo DS architecture probe for the BattleShip Smash 64 decompilation.
.SUFFIXES:
.DEFAULT_GOAL := all

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

# The Makefile is this file and the topic fragments in mk/ (P8 G), each
# included in place; objects that depend on the Makefile depend on all of
# them (NDS_MAKEFILES).
NDS_MAKEFILES := $(PROJECT_ROOT)/Makefile $(sort $(wildcard $(PROJECT_ROOT)/mk/*.mk))

include $(PROJECT_ROOT)/mk/config.mk
include $(PROJECT_ROOT)/mk/config_lab.mk
include $(PROJECT_ROOT)/mk/targets.mk
include $(PROJECT_ROOT)/mk/toolchain.mk
ifneq ($(abspath $(PROJECT_ROOT)/$(BUILD)),$(abspath $(CURDIR)))
include $(PROJECT_ROOT)/mk/sources.mk
include $(PROJECT_ROOT)/mk/nitrofs_files.mk
include $(PROJECT_ROOT)/mk/outer_targets.mk
else
include $(PROJECT_ROOT)/mk/rules_arm7.mk
include $(PROJECT_ROOT)/mk/rules_native.mk
include $(PROJECT_ROOT)/mk/rules_config.mk
include $(PROJECT_ROOT)/mk/rules_link.mk
include $(PROJECT_ROOT)/mk/rules_p4p5.mk
include $(PROJECT_ROOT)/mk/rules_nitrofs.mk
include $(PROJECT_ROOT)/mk/rules_rom.mk
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
	@printf '%s\n' 'BENCH_MAKE_NATIVE_STAGE_GENERATED_SEGMENT0_ENABLE=$(NDS_NATIVE_STAGE_GENERATED_SEGMENT0_ENABLE)'
	@printf '%s\n' 'BENCH_MAKE_TASK56_FIGHTER_PRIMITIVES=$(NDS_TASK56_FIGHTER_PRIMITIVES)'
	@printf '%s\n' 'BENCH_MAKE_TASK36_HW_COMPOSE=$(NDS_TASK36_HW_COMPOSE)'
	@printf '%s\n' 'BENCH_MAKE_TASK51_STAGE_NATIVE=$(NDS_TASK51_STAGE_NATIVE)'
	@printf '%s\n' 'BENCH_MAKE_DREAMLAND_DS_MESH=$(NDS_DREAMLAND_DS_MESH)'
	@printf '%s\n' 'BENCH_MAKE_STAGE_STEADY=$(NDS_STAGE_STEADY)'
	@printf '%s\n' 'BENCH_MAKE_CPU_PROFILE=$(NDS_CPU_PROFILE)'
	@printf '%s\n' 'BENCH_MAKE_ITCM_LIB_LEAVES=$(NDS_ITCM_LIB_LEAVES)'
	@printf '%s\n' 'BENCH_MAKE_SCREEN_SPACE_CENSUS=$(NDS_RENDERER_SCREEN_SPACE_CENSUS)'
	@printf '%s\n' 'BENCH_MAKE_RENDERER_BENCHMARK_MODE=$(NDS_RENDERER_BENCHMARK_MODE)'
	@printf '%s\n' 'BENCH_MAKE_FAST_RUN_DEFAULT=$(NDS_RENDERER_FAST_RUN_DEFAULT)'
	@printf '%s\n' 'BENCH_MAKE_FIGHTER_LEGACY_EXEC=$(NDS_FIGHTER_LEGACY_EXEC)'
	@printf '%s\n' 'BENCH_MAKE_BATTLE_STATIC_TEXTURE_DEFAULT=$(NDS_RENDERER_BATTLE_STATIC_TEXTURE_DEFAULT)'
	@printf '%s\n' 'BENCH_MAKE_R2_PATH=$(NDS_R2_PATH)'
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
