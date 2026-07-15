# Nintendo DS architecture probe for the BattleShip Smash 64 decompilation.
.SUFFIXES:

ifeq ($(strip $(DEVKITARM)),)
$(error "DEVKITARM is not set. Install devkitPro and set DEVKITARM to devkitARM")
endif

# Keep the documented local C:/devkitPro setup usable under MSYS make rules.
override DEVKITPRO := $(patsubst c:/%,/c/%,$(patsubst C:/%,/c/%,$(DEVKITPRO)))
override DEVKITARM := $(patsubst c:/%,/c/%,$(patsubst C:/%,/c/%,$(DEVKITARM)))

GAME_TITLE     := Smash 64 DS Port
GAME_SUBTITLE1 := BattleShip architecture probe
GAME_SUBTITLE2 := Built with devkitPro/libnds

PROJECT_ROOT ?= $(CURDIR)
TARGET := smash64ds
BUILD := build
BUILD_OUTPUT_ROOT ?= builds
ifeq ($(BUILD),$(notdir $(BUILD)))
ifneq ($(filter build%,$(BUILD)),)
override BUILD := $(BUILD_OUTPUT_ROOT)/$(BUILD)
endif
endif
NDS_DEV_SCENE_HARNESS ?= normal
NDS_DEV_LIVE_INPUT_PREVIEW ?= 0
NDS_HARNESS_FAST_LOGIC ?= 0
NDS_RENDERER_HW_TRIANGLES ?= 0
NDS_RENDERER_HW_DEBUG_TEXTURE_ONLY ?= 0
NDS_RENDERER_PROFILE_LEVEL ?= 2
NDS_RENDERER_M2_DETAILED_LEDGER ?= 0
NDS_RENDERER_BENCHMARK_MODE ?= 0
NDS_SCENE_MIP_CACHE_LAB ?= 0
NDS_DEBUG_HUD ?= 1
NDS_AUDIO_FGM_ARM7_ACK_DIAGNOSTICS ?= 0
NDS_RENDERER_FAST_RUN_DEFAULT ?= $(if $(filter smash64ds-battle-playable-coarse-hwtri,$(TARGET)),8,0)
ifeq ($(TARGET),smash64ds-battle-playable-canonical-hwtri)
override NDS_DEBUG_HUD := 0
override NDS_RENDERER_PROFILE_LEVEL := 0
override NDS_SCENE_MIP_CACHE_LAB := 1
override NDS_AUDIO_FGM_ARM7_ACK_DIAGNOSTICS := 0
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
ifeq ($(TARGET),smash64ds-battle-playable-coarse-mipcache-hwtri)
# Cut-G is a separate falsifier target. Keep the exact coarse ROM untouched.
override NDS_DEV_SCENE_HARNESS := battle_playable_realtime
override NDS_DEV_LIVE_INPUT_PREVIEW := 1
override NDS_HARNESS_FAST_LOGIC := 0
override NDS_RENDERER_HW_TRIANGLES := 1
override NDS_DEBUG_HUD := 0
override NDS_RENDERER_PROFILE_LEVEL := 1
override NDS_RENDERER_FAST_RUN_DEFAULT := 8
override NDS_SCENE_MIP_CACHE_LAB := 1
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
NDS_INISHIE_SOURCE_SCALE_HARNESSES := \
	battle_mariofox_stage_inishie_scale_loop \
	menu_chain_mariofox_stage_inishie_scale_loop \
	battle_mariofox_stage_mppassive_recover_loop \
	menu_chain_mariofox_stage_mppassive_recover_loop \
	battle_mariofox_stage_mpdamage_recover_loop \
	menu_chain_mariofox_stage_mpdamage_recover_loop \
	battle_mariofox_stage_mplivehit_status_loop \
	menu_chain_mariofox_stage_mplivehit_status_loop
NDS_ENABLE_INISHIE_SOURCE_SCALE_SETUP ?= $(if $(filter $(NDS_INISHIE_SOURCE_SCALE_HARNESSES),$(NDS_DEV_SCENE_HARNESS)),1,0)
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
else ifeq ($(NDS_DEV_SCENE_HARNESS),title)
NDS_DEV_SCENE_HARNESS_ID := 1
else ifeq ($(NDS_DEV_SCENE_HARNESS),vs_setup)
NDS_DEV_SCENE_HARNESS_ID := 2
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_fd)
NDS_DEV_SCENE_HARNESS_ID := 3
else ifeq ($(NDS_DEV_SCENE_HARNESS),vs_start_transition)
NDS_DEV_SCENE_HARNESS_ID := 4
else ifeq ($(NDS_DEV_SCENE_HARNESS),players_setup)
NDS_DEV_SCENE_HARNESS_ID := 5
else ifeq ($(NDS_DEV_SCENE_HARNESS),maps_setup)
NDS_DEV_SCENE_HARNESS_ID := 6
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_vsbattle)
NDS_DEV_SCENE_HARNESS_ID := 7
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_pupupu_stage)
NDS_DEV_SCENE_HARNESS_ID := 8
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_pupupu_update)
NDS_DEV_SCENE_HARNESS_ID := 9
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_pupupu_update)
NDS_DEV_SCENE_HARNESS_ID := 10
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_model)
NDS_DEV_SCENE_HARNESS_ID := 11
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_model)
NDS_DEV_SCENE_HARNESS_ID := 12
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_struct)
NDS_DEV_SCENE_HARNESS_ID := 13
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_struct)
NDS_DEV_SCENE_HARNESS_ID := 14
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_init)
NDS_DEV_SCENE_HARNESS_ID := 15
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_init)
NDS_DEV_SCENE_HARNESS_ID := 16
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_wait)
NDS_DEV_SCENE_HARNESS_ID := 17
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_wait)
NDS_DEV_SCENE_HARNESS_ID := 18
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_wait_tick)
NDS_DEV_SCENE_HARNESS_ID := 19
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_wait_tick)
NDS_DEV_SCENE_HARNESS_ID := 20
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_wait_ground)
NDS_DEV_SCENE_HARNESS_ID := 21
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_wait_ground)
NDS_DEV_SCENE_HARNESS_ID := 22
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_display_probe)
NDS_DEV_SCENE_HARNESS_ID := 23
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_display_probe)
NDS_DEV_SCENE_HARNESS_ID := 24
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_dl_scan)
NDS_DEV_SCENE_HARNESS_ID := 25
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_dl_scan)
NDS_DEV_SCENE_HARNESS_ID := 26
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_dl_execute)
NDS_DEV_SCENE_HARNESS_ID := 27
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_dl_execute)
NDS_DEV_SCENE_HARNESS_ID := 28
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_dl_draw)
NDS_DEV_SCENE_HARNESS_ID := 29
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_dl_draw)
NDS_DEV_SCENE_HARNESS_ID := 30
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_dl_draw_multi)
NDS_DEV_SCENE_HARNESS_ID := 31
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_dl_draw_multi)
NDS_DEV_SCENE_HARNESS_ID := 32
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_dl_draw_all)
NDS_DEV_SCENE_HARNESS_ID := 33
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_dl_draw_all)
NDS_DEV_SCENE_HARNESS_ID := 34
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_walk_input)
NDS_DEV_SCENE_HARNESS_ID := 35
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_walk_input)
NDS_DEV_SCENE_HARNESS_ID := 36
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_walk_loop)
NDS_DEV_SCENE_HARNESS_ID := 37
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_walk_loop)
NDS_DEV_SCENE_HARNESS_ID := 38
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_dash_run)
NDS_DEV_SCENE_HARNESS_ID := 39
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_dash_run)
NDS_DEV_SCENE_HARNESS_ID := 40
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_jump_loop)
NDS_DEV_SCENE_HARNESS_ID := 41
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_jump_loop)
NDS_DEV_SCENE_HARNESS_ID := 42
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_landing_loop)
NDS_DEV_SCENE_HARNESS_ID := 43
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_landing_loop)
NDS_DEV_SCENE_HARNESS_ID := 44
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_process_loop)
NDS_DEV_SCENE_HARNESS_ID := 45
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_process_loop)
NDS_DEV_SCENE_HARNESS_ID := 46
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_scheduler_loop)
NDS_DEV_SCENE_HARNESS_ID := 47
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_scheduler_loop)
NDS_DEV_SCENE_HARNESS_ID := 48
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_controller_loop)
NDS_DEV_SCENE_HARNESS_ID := 49
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_controller_loop)
NDS_DEV_SCENE_HARNESS_ID := 50
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_preview_loop)
NDS_DEV_SCENE_HARNESS_ID := 51
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_preview_loop)
NDS_DEV_SCENE_HARNESS_ID := 52
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_gcrunall_loop)
NDS_DEV_SCENE_HARNESS_ID := 53
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_gcrunall_loop)
NDS_DEV_SCENE_HARNESS_ID := 54
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_live_preview)
NDS_DEV_SCENE_HARNESS_ID := 55
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_live_preview)
NDS_DEV_SCENE_HARNESS_ID := 56
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_stage_gcdrawall_loop)
NDS_DEV_SCENE_HARNESS_ID := 59
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_stage_gcdrawall_loop)
NDS_DEV_SCENE_HARNESS_ID := 60
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_stage_collision_loop)
NDS_DEV_SCENE_HARNESS_ID := 61
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_stage_collision_loop)
NDS_DEV_SCENE_HARNESS_ID := 62
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_stage_floor_follow_loop)
NDS_DEV_SCENE_HARNESS_ID := 63
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_stage_floor_follow_loop)
NDS_DEV_SCENE_HARNESS_ID := 64
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_stage_floor_edge_loop)
NDS_DEV_SCENE_HARNESS_ID := 65
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_stage_floor_edge_loop)
NDS_DEV_SCENE_HARNESS_ID := 66
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_stage_mpprocess_floor_loop)
NDS_DEV_SCENE_HARNESS_ID := 67
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_stage_mpprocess_floor_loop)
NDS_DEV_SCENE_HARNESS_ID := 68
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_stage_mpupdate_floor_loop)
NDS_DEV_SCENE_HARNESS_ID := 69
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_stage_mpupdate_floor_loop)
NDS_DEV_SCENE_HARNESS_ID := 70
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_stage_mpsweep_floor_loop)
NDS_DEV_SCENE_HARNESS_ID := 71
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_stage_mpsweep_floor_loop)
NDS_DEV_SCENE_HARNESS_ID := 72
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_stage_mpcross_floor_loop)
NDS_DEV_SCENE_HARNESS_ID := 73
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_stage_mpcross_floor_loop)
NDS_DEV_SCENE_HARNESS_ID := 74
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_stage_mpadjust_floor_loop)
NDS_DEV_SCENE_HARNESS_ID := 75
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_stage_mpadjust_floor_loop)
NDS_DEV_SCENE_HARNESS_ID := 76
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_stage_mpedge_floor_loop)
NDS_DEV_SCENE_HARNESS_ID := 77
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_stage_mpedge_floor_loop)
NDS_DEV_SCENE_HARNESS_ID := 78
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_stage_mpwall_floor_loop)
NDS_DEV_SCENE_HARNESS_ID := 79
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_stage_mpwall_floor_loop)
NDS_DEV_SCENE_HARNESS_ID := 80
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_stage_mpstale_floor_loop)
NDS_DEV_SCENE_HARNESS_ID := 81
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_stage_mpstale_floor_loop)
NDS_DEV_SCENE_HARNESS_ID := 82
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_stage_mplivestale_floor_loop)
NDS_DEV_SCENE_HARNESS_ID := 83
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_stage_mplivestale_floor_loop)
NDS_DEV_SCENE_HARNESS_ID := 84
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_stage_mpmotionstale_floor_loop)
NDS_DEV_SCENE_HARNESS_ID := 85
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_stage_mpmotionstale_floor_loop)
NDS_DEV_SCENE_HARNESS_ID := 86
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_stage_mpcliffstatus_floor_loop)
NDS_DEV_SCENE_HARNESS_ID := 87
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_stage_mpcliffstatus_floor_loop)
NDS_DEV_SCENE_HARNESS_ID := 88
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_stage_mpclifftick_floor_loop)
NDS_DEV_SCENE_HARNESS_ID := 89
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_stage_mpclifftick_floor_loop)
NDS_DEV_SCENE_HARNESS_ID := 90
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_stage_mpfallmap_floor_loop)
NDS_DEV_SCENE_HARNESS_ID := 91
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_stage_mpfallmap_floor_loop)
NDS_DEV_SCENE_HARNESS_ID := 92
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_stage_mpfallland_floor_loop)
NDS_DEV_SCENE_HARNESS_ID := 93
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_stage_mpfallland_floor_loop)
NDS_DEV_SCENE_HARNESS_ID := 94
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_stage_mpceil_floor_loop)
NDS_DEV_SCENE_HARNESS_ID := 95
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_stage_mpceil_floor_loop)
NDS_DEV_SCENE_HARNESS_ID := 96
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_stage_mpceilstatus_floor_loop)
NDS_DEV_SCENE_HARNESS_ID := 97
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_stage_mpceilstatus_floor_loop)
NDS_DEV_SCENE_HARNESS_ID := 98
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_stage_mpcliffcatch_floor_loop)
NDS_DEV_SCENE_HARNESS_ID := 99
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_stage_mpcliffcatch_floor_loop)
NDS_DEV_SCENE_HARNESS_ID := 100
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_stage_mpcliffwait_floor_loop)
NDS_DEV_SCENE_HARNESS_ID := 101
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_stage_mpcliffwait_floor_loop)
NDS_DEV_SCENE_HARNESS_ID := 102
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_stage_mpcliffattack_floor_loop)
NDS_DEV_SCENE_HARNESS_ID := 103
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_stage_mpcliffattack_floor_loop)
NDS_DEV_SCENE_HARNESS_ID := 104
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_stage_mpcliffattack_action_loop)
NDS_DEV_SCENE_HARNESS_ID := 105
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_stage_mpcliffattack_action_loop)
NDS_DEV_SCENE_HARNESS_ID := 106
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_stage_mpcliffcommon2_loop)
NDS_DEV_SCENE_HARNESS_ID := 107
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_stage_mpcliffcommon2_loop)
NDS_DEV_SCENE_HARNESS_ID := 108
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_stage_mpcliffescape_action_loop)
NDS_DEV_SCENE_HARNESS_ID := 109
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_stage_mpcliffescape_action_loop)
NDS_DEV_SCENE_HARNESS_ID := 110
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_stage_mpcliffescape_common2_loop)
NDS_DEV_SCENE_HARNESS_ID := 111
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_stage_mpcliffescape_common2_loop)
NDS_DEV_SCENE_HARNESS_ID := 112
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_stage_mpcliffclimb_floor_loop)
NDS_DEV_SCENE_HARNESS_ID := 113
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_stage_mpcliffclimb_floor_loop)
NDS_DEV_SCENE_HARNESS_ID := 114
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_stage_mpcliffclimb_action_loop)
NDS_DEV_SCENE_HARNESS_ID := 115
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_stage_mpcliffclimb_action_loop)
NDS_DEV_SCENE_HARNESS_ID := 116
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_stage_mpcliffclimb_common2_loop)
NDS_DEV_SCENE_HARNESS_ID := 117
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_stage_mpcliffclimb_common2_loop)
NDS_DEV_SCENE_HARNESS_ID := 118
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_stage_mpcliffclimb_finish_loop)
NDS_DEV_SCENE_HARNESS_ID := 119
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_stage_mpcliffclimb_finish_loop)
NDS_DEV_SCENE_HARNESS_ID := 120
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_stage_mpcliffwait_damage_loop)
NDS_DEV_SCENE_HARNESS_ID := 121
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_stage_mpcliffwait_damage_loop)
NDS_DEV_SCENE_HARNESS_ID := 122
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_stage_mppassive_loop)
NDS_DEV_SCENE_HARNESS_ID := 123
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_stage_mppassive_loop)
NDS_DEV_SCENE_HARNESS_ID := 124
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_stage_mpdownwait_loop)
NDS_DEV_SCENE_HARNESS_ID := 125
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_stage_mpdownwait_loop)
NDS_DEV_SCENE_HARNESS_ID := 126
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_stage_turn_loop)
NDS_DEV_SCENE_HARNESS_ID := 127
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_stage_turn_loop)
NDS_DEV_SCENE_HARNESS_ID := 128
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_stage_mpdownrecover_loop)
NDS_DEV_SCENE_HARNESS_ID := 129
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_stage_mpdownrecover_loop)
NDS_DEV_SCENE_HARNESS_ID := 130
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_stage_mpcliffledge_loop)
NDS_DEV_SCENE_HARNESS_ID := 131
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_stage_mpcliffledge_loop)
NDS_DEV_SCENE_HARNESS_ID := 132
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_stage_mpclifflive_loop)
NDS_DEV_SCENE_HARNESS_ID := 133
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_stage_mpclifflive_loop)
NDS_DEV_SCENE_HARNESS_ID := 134
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_stage_mpwallhit_floor_loop)
NDS_DEV_SCENE_HARNESS_ID := 135
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_stage_mpwallhit_floor_loop)
NDS_DEV_SCENE_HARNESS_ID := 136
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_stage_mpwallcopy_floor_loop)
NDS_DEV_SCENE_HARNESS_ID := 137
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_stage_mpwallcopy_floor_loop)
NDS_DEV_SCENE_HARNESS_ID := 138
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_stage_mppass_floor_loop)
NDS_DEV_SCENE_HARNESS_ID := 139
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_stage_mppass_floor_loop)
NDS_DEV_SCENE_HARNESS_ID := 140
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_stage_mpplatform_floor_loop)
NDS_DEV_SCENE_HARNESS_ID := 141
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_stage_mpplatform_floor_loop)
NDS_DEV_SCENE_HARNESS_ID := 142
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_stage_mpplatform_active_floor_loop)
NDS_DEV_SCENE_HARNESS_ID := 143
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_stage_mpplatform_active_floor_loop)
NDS_DEV_SCENE_HARNESS_ID := 144
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_stage_mpplatform_tick_floor_loop)
NDS_DEV_SCENE_HARNESS_ID := 145
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_stage_mpplatform_tick_floor_loop)
NDS_DEV_SCENE_HARNESS_ID := 146
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_stage_mppass_input_loop)
NDS_DEV_SCENE_HARNESS_ID := 147
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_stage_mppass_input_loop)
NDS_DEV_SCENE_HARNESS_ID := 148
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_stage_mpplatform_pos_floor_loop)
NDS_DEV_SCENE_HARNESS_ID := 149
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_stage_mpplatform_pos_floor_loop)
NDS_DEV_SCENE_HARNESS_ID := 150
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_stage_mpplatform_speed_floor_loop)
NDS_DEV_SCENE_HARNESS_ID := 151
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_stage_mpplatform_speed_floor_loop)
NDS_DEV_SCENE_HARNESS_ID := 152
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_stage_inishie_scale_loop)
NDS_DEV_SCENE_HARNESS_ID := 153
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_stage_inishie_scale_loop)
NDS_DEV_SCENE_HARNESS_ID := 154
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_stage_mppassive_recover_loop)
NDS_DEV_SCENE_HARNESS_ID := 155
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_stage_mppassive_recover_loop)
NDS_DEV_SCENE_HARNESS_ID := 156
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_stage_mpdamage_recover_loop)
NDS_DEV_SCENE_HARNESS_ID := 157
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_stage_mpdamage_recover_loop)
NDS_DEV_SCENE_HARNESS_ID := 158
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_stage_mplivehit_status_loop)
NDS_DEV_SCENE_HARNESS_ID := 161
# Mode 161 links the latest battle boundary plus inherited proof setup.
CFLAGS += -Os
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_stage_mplivehit_status_loop)
NDS_DEV_SCENE_HARNESS_ID := 162
# Mode 162 links the full menu chain plus the latest battle boundary.
CFLAGS += -Os
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
$(error Unknown NDS_DEV_SCENE_HARNESS "$(NDS_DEV_SCENE_HARNESS)"; use a harness name from scripts/lib/harness-registry.ps1, including battle_mariofox_stage_mplivehit_status_loop or menu_chain_mariofox_stage_mplivehit_status_loop)
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
LDFLAGS := -specs=ds_arm9.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map),--gc-sections

LIBS := -lfat -lfilesystem -lnds9 -lm
LIBDIRS := $(LIBNDS)

ifneq ($(abspath $(PROJECT_ROOT)/$(BUILD)),$(abspath $(CURDIR)))

export OUTPUT := $(CURDIR)/$(TARGET)
export VPATH := $(foreach dir,$(SOURCES),$(CURDIR)/$(dir))
export DEPSDIR := $(CURDIR)/$(BUILD)

# Keep this list explicit. Adding an original subsystem is a deliberate port step.
CFILES := main.c nds_platform.c nds_ifcommon_oam.c nds_reloc_assets.c nds_audio_assets.c nds_audio_bgm.c nds_audio_fgm.c nds_renderer.c port_probe.c n64_stubs.c coroutine.c \
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
CFILES += battleship_ftmanager.c \
	battleship_ftstatus_callback_aliases.c \
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
ifeq ($(NDS_ENABLE_INISHIE_SOURCE_SCALE_SETUP),1)
CFILES += battleship_grmodelsetup.c
endif
CPPFILES :=
SFILES := coroutine_arm.s

export LD := $(CC)
export OFILES := $(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)
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
	reloc_animations/FTFoxAnim016 \
	reloc_animations/FTFoxAnim017 \
	reloc_animations/FTFoxAnim018 \
	reloc_animations/FTFoxAnim019 \
	reloc_animations/FTFoxAnim090 \
	reloc_animations/FTFoxAnim091 \
	reloc_animations/FTFoxAnim092 \
	reloc_animations/FTFoxAnim093 \
	reloc_animations/FTFoxAnim109 \
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
	reloc_animations/FTFoxAnim147

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
	audio/bgm_pupupu_pcm16.raw \
	audio/bgm_win_mario_pcm16.raw \
	audio/bgm_win_fox_pcm16.raw \
	audio/bgm_results_pcm16.raw
endif
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

.PHONY: all clean clean-generated distclean run $(BUILD)

all: $(BUILD)

$(BUILD):
	@mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile PROJECT_ROOT=$(PROJECT_ROOT) BUILD=$(BUILD) BUILD_OUTPUT_ROOT=$(BUILD_OUTPUT_ROOT)

clean:
	@echo clean ...
	@rm -fr $(BUILD) $(TARGET).elf $(TARGET).nds $(TARGET).ds.gba

clean-generated:
	@powershell -NoProfile -ExecutionPolicy Bypass -File scripts/clean-generated.ps1 -Force

distclean: clean-generated

run: $(BUILD)
	@echo "ROM ready: $(TARGET).nds"

else

DEPENDS := $(OFILES:.o=.d)
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

.PHONY: all FORCE

all: $(OUTPUT).nds
ifeq ($(TARGET),smash64ds-battle-playable-canonical-hwtri)
all: $(PROJECT_ROOT)/smash64ds-battle-playable-hwtri.nds
$(PROJECT_ROOT)/smash64ds-battle-playable-hwtri.nds: $(OUTPUT).nds FORCE
	@cp $< $@
endif

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
		echo '#define NDS_RENDERER_M2_DETAILED_LEDGER $(NDS_RENDERER_M2_DETAILED_LEDGER)'; \
		echo '#define NDS_RENDERER_BENCHMARK_MODE $(NDS_RENDERER_BENCHMARK_MODE)'; \
		echo '#define NDS_RENDERER_FAST_RUN_DEFAULT $(NDS_RENDERER_FAST_RUN_DEFAULT)'; \
		echo '#define NDS_SCENE_MIP_CACHE_LAB $(NDS_SCENE_MIP_CACHE_LAB)'; \
		echo '#define NDS_BUILD_HARNESS_VARIANT "$(NDS_DEV_SCENE_HARNESS)"'; \
		echo '#define NDS_DEBUG_HUD $(NDS_DEBUG_HUD)'; \
		echo '#define NDS_AUDIO_FGM_ARM7_ACK_DIAGNOSTICS $(NDS_AUDIO_FGM_ARM7_ACK_DIAGNOSTICS)'; \
		echo '#define NDS_IMPORT_BATTLESHIP_FTMAIN $(NDS_IMPORT_BATTLESHIP_FTMAIN)'; \
		echo '#define NDS_IMPORT_BATTLESHIP_FTMANAGER $(NDS_IMPORT_BATTLESHIP_FTMANAGER)'; \
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

$(OUTPUT).nds: $(OUTPUT).elf $(NDS_NITROFS_RELOC_FILES) $(NDS_NITROFS_RELOCDATA_FILES) $(NDS_NITROFS_AUDIO_FILES)
$(OUTPUT).elf: $(OFILES)
$(OFILES): $(PROJECT_ROOT)/Makefile $(NDS_BUILD_CONFIG)
# The source DS renderer is ARM-state code, with its hottest handlers copied
# to ITCM. Keep that interworking shape for mode 163 without spending the code
# size in normal startup or the archived narrow harness fleet.
ifeq ($(NDS_DEV_SCENE_HARNESS_ID),163)
nds_renderer.o: CFLAGS += -marm
endif
scene_backend.o: $(SCENE_BACKEND_SLICES) $(NDS_SCENE_HARNESS_CONFIG)
scene_harness.o battleship_grinishie_scale.o: $(NDS_SCENE_HARNESS_CONFIG)

$(NITROFS_DIR)/reloc/%: $(BATTLESHIP_O2R)/%
	@mkdir -p $(dir $@)
	@cp $< $@

$(NITROFS_DIR)/relocdata/us/%: $(BATTLESHIP_RELOCDATA)/%
	@mkdir -p $(dir $@)
	@cp $< $@

$(NITROFS_DIR)/audio/bgm_pupupu_pcm16.raw: $(PROJECT_ROOT)/assets/audio/bgm_pupupu_pcm16.raw
	@mkdir -p $(dir $@)
	@cp $< $@

$(NITROFS_DIR)/audio/bgm_win_mario_pcm16.raw: $(PROJECT_ROOT)/assets/audio/bgm_win_mario_pcm16.raw
	@mkdir -p $(dir $@)
	@cp $< $@

$(NITROFS_DIR)/audio/bgm_win_fox_pcm16.raw: $(PROJECT_ROOT)/assets/audio/bgm_win_fox_pcm16.raw
	@mkdir -p $(dir $@)
	@cp $< $@

$(NITROFS_DIR)/audio/bgm_results_pcm16.raw: $(PROJECT_ROOT)/assets/audio/bgm_results_pcm16.raw
	@mkdir -p $(dir $@)
	@cp $< $@

$(NITROFS_DIR)/audio/fgm_phase_pack_ima.bin: $(PROJECT_ROOT)/assets/audio/fgm_phase_pack_ima.bin
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
# by this Makefile, including the mode-163 ARM-state renderer override.
.PHONY: print-benchmark-flags
print-benchmark-flags:
	@printf '%s\n' 'BENCH_MAKE_TARGET=$(TARGET)'
	@printf '%s\n' 'BENCH_MAKE_HARNESS=$(NDS_DEV_SCENE_HARNESS)'
	@printf '%s\n' 'BENCH_MAKE_HARNESS_ID=$(NDS_DEV_SCENE_HARNESS_ID)'
	@printf '%s\n' 'BENCH_MAKE_PROFILE=$(NDS_RENDERER_PROFILE_LEVEL)'
	@printf '%s\n' 'BENCH_MAKE_M2_DETAILED_LEDGER=$(NDS_RENDERER_M2_DETAILED_LEDGER)'
	@printf '%s\n' 'BENCH_MAKE_RENDERER_BENCHMARK_MODE=$(NDS_RENDERER_BENCHMARK_MODE)'
	@printf '%s\n' 'BENCH_MAKE_CFLAGS_COMMON=$(strip $(CFLAGS))'
	@printf '%s\n' 'BENCH_MAKE_CFLAGS_RENDERER=$(strip $(CFLAGS) $(if $(filter 163,$(NDS_DEV_SCENE_HARNESS_ID)),-marm))'
	@printf '%s\n' 'BENCH_MAKE_CFLAGS_SCENE=$(strip $(CFLAGS))'
