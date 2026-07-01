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
NDS_DEV_SCENE_HARNESS ?= normal
NDS_DEV_LIVE_INPUT_PREVIEW ?= 0
NDS_RENDERER_HW_TRIANGLES ?= 0
NDS_INISHIE_SOURCE_SCALE_HARNESSES := \
	battle_mariofox_stage_inishie_scale_loop \
	menu_chain_mariofox_stage_inishie_scale_loop \
	battle_mariofox_stage_mppassive_recover_loop \
	menu_chain_mariofox_stage_mppassive_recover_loop \
	battle_mariofox_stage_mpdamage_recover_loop \
	menu_chain_mariofox_stage_mpdamage_recover_loop \
	battle_mariofox_stage_mplivehit_damage_loop \
	menu_chain_mariofox_stage_mplivehit_damage_loop \
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
	-Wno-error=int-conversion
ifeq ($(NDS_DEV_SCENE_HARNESS),normal)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=0
else ifeq ($(NDS_DEV_SCENE_HARNESS),title)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=1
else ifeq ($(NDS_DEV_SCENE_HARNESS),vs_setup)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=2
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_fd)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=3
else ifeq ($(NDS_DEV_SCENE_HARNESS),vs_start_transition)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=4
else ifeq ($(NDS_DEV_SCENE_HARNESS),players_setup)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=5
else ifeq ($(NDS_DEV_SCENE_HARNESS),maps_setup)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=6
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_vsbattle)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=7
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_pupupu_stage)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=8
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_pupupu_update)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=9
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_pupupu_update)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=10
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_model)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=11
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_model)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=12
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_struct)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=13
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_struct)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=14
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_init)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=15
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_init)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=16
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_wait)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=17
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_wait)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=18
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_wait_tick)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=19
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_wait_tick)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=20
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_wait_ground)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=21
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_wait_ground)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=22
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_display_probe)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=23
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_display_probe)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=24
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_dl_scan)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=25
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_dl_scan)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=26
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_dl_execute)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=27
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_dl_execute)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=28
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_dl_draw)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=29
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_dl_draw)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=30
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_dl_draw_multi)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=31
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_dl_draw_multi)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=32
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_dl_draw_all)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=33
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_dl_draw_all)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=34
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_walk_input)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=35
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_walk_input)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=36
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_walk_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=37
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_walk_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=38
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_dash_run)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=39
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_dash_run)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=40
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_jump_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=41
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_jump_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=42
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_landing_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=43
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_landing_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=44
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_process_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=45
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_process_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=46
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_scheduler_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=47
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_scheduler_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=48
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_controller_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=49
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_controller_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=50
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_preview_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=51
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_preview_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=52
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_gcrunall_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=53
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_gcrunall_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=54
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_live_preview)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=55
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_live_preview)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=56
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_gcdrawall_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=57
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_gcdrawall_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=58
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_stage_gcdrawall_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=59
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_stage_gcdrawall_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=60
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_stage_collision_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=61
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_stage_collision_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=62
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_stage_floor_follow_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=63
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_stage_floor_follow_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=64
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_stage_floor_edge_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=65
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_stage_floor_edge_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=66
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_stage_mpprocess_floor_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=67
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_stage_mpprocess_floor_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=68
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_stage_mpupdate_floor_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=69
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_stage_mpupdate_floor_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=70
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_stage_mpsweep_floor_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=71
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_stage_mpsweep_floor_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=72
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_stage_mpcross_floor_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=73
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_stage_mpcross_floor_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=74
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_stage_mpadjust_floor_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=75
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_stage_mpadjust_floor_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=76
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_stage_mpedge_floor_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=77
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_stage_mpedge_floor_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=78
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_stage_mpwall_floor_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=79
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_stage_mpwall_floor_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=80
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_stage_mpstale_floor_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=81
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_stage_mpstale_floor_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=82
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_stage_mplivestale_floor_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=83
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_stage_mplivestale_floor_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=84
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_stage_mpmotionstale_floor_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=85
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_stage_mpmotionstale_floor_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=86
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_stage_mpcliffstatus_floor_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=87
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_stage_mpcliffstatus_floor_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=88
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_stage_mpclifftick_floor_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=89
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_stage_mpclifftick_floor_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=90
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_stage_mpfallmap_floor_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=91
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_stage_mpfallmap_floor_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=92
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_stage_mpfallland_floor_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=93
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_stage_mpfallland_floor_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=94
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_stage_mpceil_floor_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=95
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_stage_mpceil_floor_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=96
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_stage_mpceilstatus_floor_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=97
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_stage_mpceilstatus_floor_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=98
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_stage_mpcliffcatch_floor_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=99
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_stage_mpcliffcatch_floor_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=100
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_stage_mpcliffwait_floor_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=101
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_stage_mpcliffwait_floor_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=102
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_stage_mpcliffattack_floor_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=103
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_stage_mpcliffattack_floor_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=104
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_stage_mpcliffattack_action_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=105
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_stage_mpcliffattack_action_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=106
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_stage_mpcliffcommon2_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=107
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_stage_mpcliffcommon2_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=108
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_stage_mpcliffescape_action_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=109
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_stage_mpcliffescape_action_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=110
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_stage_mpcliffescape_common2_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=111
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_stage_mpcliffescape_common2_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=112
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_stage_mpcliffclimb_floor_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=113
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_stage_mpcliffclimb_floor_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=114
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_stage_mpcliffclimb_action_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=115
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_stage_mpcliffclimb_action_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=116
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_stage_mpcliffclimb_common2_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=117
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_stage_mpcliffclimb_common2_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=118
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_stage_mpcliffclimb_finish_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=119
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_stage_mpcliffclimb_finish_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=120
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_stage_mpcliffwait_damage_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=121
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_stage_mpcliffwait_damage_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=122
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_stage_mppassive_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=123
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_stage_mppassive_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=124
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_stage_mpdownwait_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=125
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_stage_mpdownwait_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=126
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_stage_turn_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=127
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_stage_turn_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=128
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_stage_mpdownrecover_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=129
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_stage_mpdownrecover_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=130
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_stage_mpcliffledge_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=131
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_stage_mpcliffledge_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=132
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_stage_mpclifflive_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=133
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_stage_mpclifflive_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=134
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_stage_mpwallhit_floor_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=135
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_stage_mpwallhit_floor_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=136
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_stage_mpwallcopy_floor_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=137
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_stage_mpwallcopy_floor_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=138
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_stage_mppass_floor_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=139
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_stage_mppass_floor_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=140
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_stage_mpplatform_floor_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=141
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_stage_mpplatform_floor_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=142
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_stage_mpplatform_active_floor_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=143
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_stage_mpplatform_active_floor_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=144
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_stage_mpplatform_tick_floor_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=145
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_stage_mpplatform_tick_floor_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=146
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_stage_mppass_input_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=147
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_stage_mppass_input_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=148
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_stage_mpplatform_pos_floor_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=149
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_stage_mpplatform_pos_floor_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=150
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_stage_mpplatform_speed_floor_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=151
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_stage_mpplatform_speed_floor_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=152
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_stage_inishie_scale_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=153
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_stage_inishie_scale_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=154
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_stage_mppassive_recover_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=155
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_stage_mppassive_recover_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=156
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_stage_mpdamage_recover_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=157
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_stage_mpdamage_recover_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=158
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_stage_mplivehit_damage_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=159
# Mode 159 links the full live-hit damage proof boundary.
CFLAGS += -Os
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_stage_mplivehit_damage_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=160
# Mode 160 links the full menu chain plus the latest battle boundary.
CFLAGS += -Os
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_mariofox_stage_mplivehit_status_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=161
# Mode 161 links the latest battle boundary plus inherited proof setup.
CFLAGS += -Os
else ifeq ($(NDS_DEV_SCENE_HARNESS),menu_chain_mariofox_stage_mplivehit_status_loop)
CFLAGS += -DNDS_DEV_SCENE_HARNESS=162
# Mode 162 links the full menu chain plus the latest battle boundary.
CFLAGS += -Os
else
$(error Unknown NDS_DEV_SCENE_HARNESS "$(NDS_DEV_SCENE_HARNESS)"; use a harness name from scripts/lib/harness-registry.ps1, including battle_mariofox_stage_mplivehit_status_loop or menu_chain_mariofox_stage_mplivehit_status_loop)
endif
CFLAGS += -DNDS_DEV_LIVE_INPUT_PREVIEW=$(NDS_DEV_LIVE_INPUT_PREVIEW)
CFLAGS += -DNDS_ENABLE_INISHIE_SOURCE_SCALE_SETUP=$(NDS_ENABLE_INISHIE_SOURCE_SCALE_SETUP)
CFLAGS += -DNDS_RENDERER_HW_TRIANGLES=$(NDS_RENDERER_HW_TRIANGLES)
CXXFLAGS := $(CFLAGS) -fno-rtti -fno-exceptions
ASFLAGS := -g $(ARCH)
LDFLAGS := -specs=ds_arm9.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map),--gc-sections

LIBS := -lfat -lfilesystem -lnds9 -lm
LIBDIRS := $(LIBNDS)

ifneq ($(BUILD),$(notdir $(CURDIR)))

export OUTPUT := $(CURDIR)/$(TARGET)
export VPATH := $(foreach dir,$(SOURCES),$(CURDIR)/$(dir))
export DEPSDIR := $(CURDIR)/$(BUILD)

# Keep this list explicit. Adding an original subsystem is a deliberate port step.
CFILES := main.c nds_platform.c nds_reloc_assets.c nds_renderer.c port_probe.c n64_stubs.c coroutine.c \
	libultra_os.c os_selftest.c boot_stubs.c battleship_sys_main.c \
	scheduler_backend.c controller_backend.c battleship_sys_scheduler.c \
	battleship_sys_controller.c battleship_sys_maindevice.c \
	battleship_sys_video.c battleship_sys_malloc.c \
	battleship_sys_framebuffer.c battleship_sys_zbuffer.c video_bootstrap.c \
	battleship_scmanager.c battleship_mnstartup.c scene_backend.c scene_harness.c utils.c vector.c \
	battleship_scsubsyscontroller.c \
	battleship_sys_taskman.c battleship_sys_objman.c \
	battleship_sys_objhelper.c battleship_sys_objanim.c \
	battleship_sys_interp.c battleship_mvopeningroom.c \
	battleship_mvopeningportraits.c battleship_mvopeningmario.c \
	battleship_mvopeningdonkey.c battleship_mvopeninglink.c \
	battleship_mvopeningsamus.c battleship_mvopeningyoshi.c \
	battleship_mvopeningkirby.c battleship_mvopeningfox.c \
	battleship_mvopeningpikachu.c battleship_mntitlefiles.c \
	battleship_mntitle.c battleship_mnvsmode.c \
	battleship_mnplayersvs.c battleship_mnmaps.c \
	battleship_gmcommon.c battleship_scvsbattle.c \
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
	reloc_extern_data/ExternDataBank109

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

NDS_STARTUP_RELOC_FILES := \
	reloc_misc_named/N64Logo

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
	$(foreach file,$(NDS_VSBATTLE_RELOC_FILES),$(NITROFS_DIR)/reloc/$(file))

export NDS_NITROFS_RELOCDATA_FILES := \
	$(foreach file,$(NDS_INISHIE_SCALE_RELOCDATA_FILES),$(NITROFS_DIR)/relocdata/us/$(file))

.PHONY: all clean clean-generated distclean run $(BUILD)

all: $(BUILD)

$(BUILD):
	@mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

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
SCENE_BACKEND_SLICES := \
	$(PROJECT_ROOT)/src/port/diagnostics.c \
	$(PROJECT_ROOT)/src/port/taskman_seam.c \
	$(PROJECT_ROOT)/src/port/reloc_backend.c \
	$(PROJECT_ROOT)/src/port/sprite_preview_backend.c \
	$(PROJECT_ROOT)/src/port/opening_movie_backend.c \
	$(PROJECT_ROOT)/src/port/title_backend.c

$(OUTPUT).nds: $(OUTPUT).elf $(NDS_NITROFS_RELOC_FILES) $(NDS_NITROFS_RELOCDATA_FILES)
$(OUTPUT).elf: $(OFILES)
scene_backend.o: $(SCENE_BACKEND_SLICES)

$(NITROFS_DIR)/reloc/%: $(BATTLESHIP_O2R)/%
	@mkdir -p $(dir $@)
	@cp $< $@

$(NITROFS_DIR)/relocdata/us/%: $(BATTLESHIP_RELOCDATA)/%
	@mkdir -p $(dir $@)
	@cp $< $@

-include $(DEPENDS)

endif
