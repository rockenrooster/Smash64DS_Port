# Nintendo DS architecture probe for the BattleShip Smash 64 decompilation.
.SUFFIXES:

ifeq ($(strip $(DEVKITARM)),)
$(error "DEVKITARM is not set. Install devkitPro and set DEVKITARM to devkitARM")
endif

GAME_TITLE     := Smash 64 DS Port
GAME_SUBTITLE1 := BattleShip architecture probe
GAME_SUBTITLE2 := Built with devkitPro/libnds

PROJECT_ROOT ?= $(CURDIR)
TARGET := smash64ds
BUILD := build
NDS_DEV_SCENE_HARNESS ?= normal
NITROFS_DIR := $(PROJECT_ROOT)/$(BUILD)/nitrofs
NITRO_FILES := $(NITROFS_DIR)

include $(DEVKITARM)/ds_rules

BATTLESHIP_DECOMP := decomp/BattleShip-main/decomp
BATTLESHIP_SYS := $(BATTLESHIP_DECOMP)/src/sys
BATTLESHIP_O2R := $(PROJECT_ROOT)/decomp/BattleShip-main/BattleShip_o2r

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
else
$(error Unknown NDS_DEV_SCENE_HARNESS "$(NDS_DEV_SCENE_HARNESS)"; use normal, title, vs_setup, or battle_fd)
endif
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
	battleship_mntitle.c
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

NDS_STARTUP_RELOC_FILES := \
	reloc_misc_named/N64Logo

export NDS_NITROFS_RELOC_FILES := \
	$(foreach file,$(NDS_STARTUP_RELOC_FILES),$(NITROFS_DIR)/reloc/$(file)) \
	$(foreach file,$(NDS_OPENING_ROOM_RELOC_FILES),$(NITROFS_DIR)/reloc/$(file)) \
	$(foreach file,$(NDS_OPENING_PORTRAITS_RELOC_FILES),$(NITROFS_DIR)/reloc/$(file)) \
	$(foreach file,$(NDS_OPENING_MARIO_RELOC_FILES),$(NITROFS_DIR)/reloc/$(file)) \
	$(foreach file,$(NDS_OPENING_ACTION_RELOC_FILES),$(NITROFS_DIR)/reloc/$(file)) \
	$(foreach file,$(NDS_TITLE_RELOC_FILES),$(NITROFS_DIR)/reloc/$(file))

.PHONY: all clean run $(BUILD)

all: $(BUILD)

$(BUILD):
	@mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

clean:
	@echo clean ...
	@rm -fr $(BUILD) $(TARGET).elf $(TARGET).nds $(TARGET).ds.gba

run: $(BUILD)
	@echo "ROM ready: $(TARGET).nds"

else

DEPENDS := $(OFILES:.o=.d)

$(OUTPUT).nds: $(OUTPUT).elf $(NDS_NITROFS_RELOC_FILES)
$(OUTPUT).elf: $(OFILES)

$(NITROFS_DIR)/reloc/%: $(BATTLESHIP_O2R)/%
	@mkdir -p $(dir $@)
	@cp $< $@

-include $(DEPENDS)

endif
