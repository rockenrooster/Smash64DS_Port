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
ifeq ($(NDS_PUBLISH_USER_ROM),0)
ifeq ($(abspath $(NDS_OUTPUT_ROOT)),$(abspath $(PROJECT_ROOT)))
$(error Non-published target "$(TARGET)" may not write into the project root)
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
NDS_TASK36_HW_COMPOSE ?= 0
NDS_TASK22_WALLPAPER_RUN_LAB ?= 0
NDS_RENDERER_SCREEN_SPACE_CENSUS ?= 0
NDS_RENDER_ECONOMY ?= 0
# Owner 5 is the only census-ranked Dream Land cut that passed the canonical
# 500-pixel ratchet.  The enclosing economy flag remains off by default.
NDS_RENDER_ECONOMY_OWNER_MASK ?= 32
NDS_RENDERER_BENCHMARK_MODE ?= 0
NDS_SCENE_MIP_CACHE_LAB ?= 0
NDS_FAST_WALLPAPER_AFFINE ?= 0
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
NDS_TASK39_FX_SHIELD ?= 0
# Task 44 stage steady-state excision: generation-based admission, dense
# rigid/dynamic binding lists, and the hoisted GX capture-active test. Requires
# the Task 36 hardware-compose stage owner; meaningless without it.
NDS_TASK44_STAGE_STEADY ?= 0
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
# Task 44: stage steady-state admission + dense binding lists. Exact (no
# fidelity change); ships on with Task 36 replay.
override NDS_TASK44_STAGE_STEADY := 1
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
override NDS_TASK39_FX_SHIELD := 1
endif
NDS_TASK44_DEVICE_TARGETS := \
	smash64ds-battle-playable-task44-on-hwtri \
	smash64ds-battle-playable-task44-off-hwtri
ifneq ($(filter $(TARGET),smash64ds-battle-playable-tickhud-hwtri smash64ds-battle-playable-proof-hwtri $(NDS_TASK44_DEVICE_TARGETS)),)
# Profile-0 shipping path plus either the lightweight Task 41 timers or the
# full diagnostic publications required by GDB proof runs. The two Task 44
# targets are the nonpublishing retail A/B pair: release-equivalent to the
# tick-HUD build except for NDS_TASK44_STAGE_STEADY, with distinct target and
# build names so one ROM cannot overwrite the other.
override NDS_DEV_SCENE_HARNESS := battle_playable_realtime
override NDS_DEV_LIVE_INPUT_PREVIEW := 1
override NDS_HARNESS_FAST_LOGIC := 0
override NDS_RENDERER_HW_TRIANGLES := 1
override NDS_DEBUG_HUD := 0
override NDS_RENDERER_PROFILE_LEVEL := 0
ifeq ($(TARGET),smash64ds-battle-playable-proof-hwtri)
override NDS_SHIP_TELEMETRY := 1
override NDS_TICK_HUD := 0
else
override NDS_SHIP_TELEMETRY := 0
override NDS_TICK_HUD := 1
endif
override NDS_RENDERER_FAST_RUN_DEFAULT := 9
override NDS_NATIVE_STAGE_GENERATED_SEGMENT0_ENABLE := 1
override NDS_TASK36_HW_COMPOSE := 2
# Task 44: stage steady-state admission + dense binding lists. Exact (no
# fidelity change); ships on with Task 36 replay. The device A/B pair is the
# only place this is allowed off in a profile-0 build.
override NDS_TASK44_STAGE_STEADY := \
	$(if $(filter smash64ds-battle-playable-task44-off-hwtri,$(TARGET)),0,1)
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
override NDS_TASK39_FX_SHIELD := 1
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
override NDS_TASK39_FX_SHIELD := 1
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
NDS_IMPORT_BATTLESHIP_MPPROCESS_LIVE ?= 0
NDS_IMPORT_BATTLESHIP_MPPROCESS_PRIVATE ?= 1
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
# Checked here, after every target block has applied its overrides, so a target
# that turns both on is accepted while a bare command-line NDS_TASK44_STAGE_STEADY=1
# without the Task 36 stage owner still fails loudly.
ifeq ($(NDS_TASK44_STAGE_STEADY),1)
ifeq ($(NDS_TASK36_HW_COMPOSE),0)
$(error NDS_TASK44_STAGE_STEADY=1 requires NDS_TASK36_HW_COMPOSE)
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
SOURCES := src/nds src/port src/import $(BATTLESHIP_SYS)
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
else
$(error Unknown NDS_DEV_SCENE_HARNESS "$(NDS_DEV_SCENE_HARNESS)"; use normal or a harness name from scripts/lib/harness-registry.ps1)
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
ifeq ($(NDS_IMPORT_BATTLESHIP_FOX_REFLECTOR),1)
CFILES += battleship_fox_reflector.c
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
		echo '#define NDS_TASK36_HW_COMPOSE $(NDS_TASK36_HW_COMPOSE)'; \
		echo '#define NDS_TASK44_STAGE_STEADY $(NDS_TASK44_STAGE_STEADY)'; \
		echo '#define NDS_TASK22_WALLPAPER_RUN_LAB $(NDS_TASK22_WALLPAPER_RUN_LAB)'; \
		echo '#define NDS_RENDERER_SCREEN_SPACE_CENSUS $(NDS_RENDERER_SCREEN_SPACE_CENSUS)'; \
		echo '#define NDS_RENDER_ECONOMY $(NDS_RENDER_ECONOMY)'; \
		echo '#define NDS_RENDER_ECONOMY_OWNER_MASK $(NDS_RENDER_ECONOMY_OWNER_MASK)'; \
		echo '#define NDS_RENDERER_BENCHMARK_MODE $(NDS_RENDERER_BENCHMARK_MODE)'; \
		echo '#define NDS_RENDERER_FAST_RUN_DEFAULT $(NDS_RENDERER_FAST_RUN_DEFAULT)'; \
		echo '#define NDS_SCENE_MIP_CACHE_LAB $(NDS_SCENE_MIP_CACHE_LAB)'; \
		echo '#define NDS_FAST_WALLPAPER_AFFINE $(NDS_FAST_WALLPAPER_AFFINE)'; \
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
		echo '#define NDS_TASK39_FX_SPRITES $(NDS_TASK39_FX_SPRITES)'; \
		echo '#define NDS_TASK39_FX_FLASH $(NDS_TASK39_FX_FLASH)'; \
		echo '#define NDS_TASK39_FX_SHIELD $(NDS_TASK39_FX_SHIELD)'; \
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

$(NDS_TASK39_HIT_SPARKS_INC): \
		$(PROJECT_ROOT)/scripts/generate_task39_hit_sparks.py \
		$(BATTLESHIP_O2R)/particles/efcommon_particle_scb \
		$(BATTLESHIP_O2R)/particles/efcommon_particle_txb
	python "$(PROJECT_ROOT)/scripts/generate_task39_hit_sparks.py"

prune-obsolete-audio:
	@rm -f $(foreach file,$(NDS_AUDIO_OBSOLETE_DERIVED_FILES),$(NITROFS_DIR)/$(file))

$(OUTPUT).nds: prune-obsolete-audio $(OUTPUT).elf $(NDS_NITROFS_RELOC_FILES) $(NDS_NITROFS_RELOCDATA_FILES) $(NDS_NITROFS_AUDIO_FILES) $(NDS_NITROFS_BATTLE_STATIC_TEXTURE_FILES)
$(OUTPUT).elf: $(OFILES) $(NDS_PRIVATE_CHECK_OFILES) \
	$(NDS_HOT_TEXT_SPECS) $(NDS_HOT_TEXT_LINKER_SCRIPT) \
	$(NDS_TASK32_DRAW_HOT_FRAGMENT)
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
ifneq ($(strip $(NDS_MPPROCESS_STRICT_OFILES)),)
$(NDS_MPPROCESS_STRICT_OFILES): CFLAGS += -Werror=implicit-function-declaration -Werror=incompatible-pointer-types -Werror=int-conversion -Werror=return-type
endif
# The measured mode-163 renderer is cache-resident on retail hardware and
# wins in ARM state despite melonDS's main-RAM fetch model.
ifeq ($(NDS_DEV_SCENE_HARNESS_ID),163)
nds_renderer.o: CFLAGS += -marm
endif
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

$(NITROFS_DIR)/renderer/battle_playable_static_textures.rgb5a1.bin: $(PROJECT_ROOT)/assets/renderer/battle_playable_static_textures.rgb5a1.bin
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
	@printf '%s\n' 'BENCH_MAKE_TASK36_HW_COMPOSE=$(NDS_TASK36_HW_COMPOSE)'
	@printf '%s\n' 'BENCH_MAKE_TASK44_STAGE_STEADY=$(NDS_TASK44_STAGE_STEADY)'
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
	@printf '%s\n' 'BENCH_MAKE_TASK39_FX_SHIELD=$(NDS_TASK39_FX_SHIELD)'
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
