# The inner make (the build directory): the exported inventories, the
# phony rules and the ARM7 binaries (the audio service and the wireless
# module).
# Included in place from the Makefile (P8 G); its text is unchanged.


# The outer make exports the full NitroFS reloc prerequisite inventory so this
# recursive make can inherit it without rebuilding the list.  P2-3 fighter
# staging makes that inventory large enough to overflow the Windows/MSYS child
# process environment even though Make itself can still consume the value.
# Retain it as Make dependency metadata, but keep it out of every recipe's
# environment; native devkitARM tools otherwise fail to launch with ENOENT.
unexport NDS_NITROFS_RELOC_FILES

DEPENDS := $(OFILES:.o=.d)
NDS_BUILD_CONFIG := $(PROJECT_ROOT)/$(BUILD)/nds_build_config.h
NDS_BUILD_REVISION := $(PROJECT_ROOT)/$(BUILD)/nds_build_revision.h
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

# A8: build the project-owned ARM7 from source and package that exact ELF.
# This uses the installed Calico services and contains no graphics backend.
NDS_ARM7_ELF := $(CURDIR)/nds-audio-arm7.elf
NDS_ARM7_SOURCE := $(PROJECT_ROOT)/src/nds/arm7/nds_audio_main.c
NDS_ARM7_HEADER := $(PROJECT_ROOT)/include/nds/nds_audio_storage.h
NDS_ARM7_OBJECTS := nds-audio-arm7.o nds-audio-dldi-arm7.o nds-audio-extent-arm7.o \
	nds-bgm-stream-arm7.o nds-bgm-service-arm7.o nds-net-arm7.o
NDS_ARM7_FLAGS := -march=armv4t -mtune=arm7tdmi -mthumb -g -Os \
	-Wall -Wextra -ffunction-sections -fdata-sections -DARM7 -D__NDS__ \
	-I$(PROJECT_ROOT)/include -I$(CALICO)/include
_ARM7_ELF := -7 $(NDS_ARM7_ELF)
.PHONY: arm7-service
arm7-service: $(NDS_ARM7_ELF)
nds-audio-arm7.o: $(NDS_ARM7_SOURCE) $(NDS_ARM7_HEADER)
	$(CC) $(NDS_ARM7_FLAGS) -MMD -MP -MF nds-audio-arm7.d -c $< -o $@
nds-audio-dldi-arm7.o: $(PROJECT_ROOT)/src/nds/arm7/nds_audio_dldi.c
	$(CC) $(NDS_ARM7_FLAGS) -MMD -MP -MF nds-audio-dldi-arm7.d -c $< -o $@
nds-audio-extent-arm7.o: $(PROJECT_ROOT)/src/nds/nds_audio_extent.c $(PROJECT_ROOT)/include/nds/nds_audio_extent.h
	$(CC) $(NDS_ARM7_FLAGS) -MMD -MP -MF nds-audio-extent-arm7.d -c $< -o $@
nds-bgm-stream-arm7.o: $(PROJECT_ROOT)/src/nds/nds_bgm_stream.c $(PROJECT_ROOT)/include/nds/nds_bgm_stream.h
	$(CC) $(NDS_ARM7_FLAGS) -MMD -MP -MF nds-bgm-stream-arm7.d -c $< -o $@
nds-bgm-service-arm7.o: $(PROJECT_ROOT)/src/nds/arm7/nds_audio_bgm_service.c $(PROJECT_ROOT)/include/nds/nds_bgm_ipc.h
	$(CC) $(NDS_ARM7_FLAGS) -MMD -MP -MF nds-bgm-service-arm7.d -c $< -o $@
nds-net-arm7.o: $(PROJECT_ROOT)/src/nds/arm7/nds_net_arm7.c $(PROJECT_ROOT)/include/nds/nds_net_link.h
	$(CC) $(NDS_ARM7_FLAGS) -MMD -MP -MF nds-net-arm7.d -c $< -o $@
NDS_ARM7_DS7_LD := $(PROJECT_ROOT)/linker/nds_arm7_ds7_arm9wram.ld
$(NDS_ARM7_ELF): $(NDS_ARM7_OBJECTS) $(PROJECT_ROOT)/linker/nds_arm7_contract.ld $(NDS_ARM7_DS7_LD)
	$(CC) $(NDS_ARM7_FLAGS) -specs=$(CALICO)/share/ds7.specs \
		-L$(CALICO)/lib -Wl,-Map,nds-audio-arm7.map \
		-Wl,-T,$(NDS_ARM7_DS7_LD),--section-start=.main=0x02ff0000 \
		-Wl,-T,$(PROJECT_ROOT)/linker/nds_arm7_contract.ld,--wrap=armCopyMem32 $(NDS_ARM7_OBJECTS) -lcalico_ds7 -o $@
.PHONY: native-only-arm7-check
native-only-arm7-check: $(NDS_ARM7_ELF)
	@echo $(NDS_ARM7_OBJECTS) > native-arm7-objects.list
	@python "$(PROJECT_ROOT)/scripts/check_native_only_rom.py" --elf "$(NDS_ARM7_ELF)" --objects-list native-arm7-objects.list --build-dir "$(CURDIR)"
$(OUTPUT).nds: $(NDS_ARM7_ELF)
$(OUTPUT).nds: | native-only-arm7-check

# P3: the ARM7 radio module (include/nds/nds_net_link.h). It links at the
# net area base against the resident ARM7 ELF, so only the Mitsumi driver,
# netbuf and wlan objects come from the archive; the ARM9 copies
# net/net7.bin into place when a session starts.
NDS_NET7_ELF := $(CURDIR)/nds-net7.elf
nds-net7-module.o: $(PROJECT_ROOT)/src/nds/arm7/nds_net7_module.c $(PROJECT_ROOT)/src/nds/arm7/calico_mwl_common.h $(PROJECT_ROOT)/include/nds/nds_net_link.h
	$(CC) $(NDS_ARM7_FLAGS) -MMD -MP -MF nds-net7-module.d -c $< -o $@
# The module's own RX drain replaces Calico's mwl_rx.o (it defines all four of
# that object's public functions, so the archive member is never pulled).
nds-net7-mwl-rx.o: $(PROJECT_ROOT)/src/nds/arm7/nds_net7_mwl_rx.c $(PROJECT_ROOT)/src/nds/arm7/calico_mwl_common.h $(PROJECT_ROOT)/include/nds/nds_net_link.h
	$(CC) $(NDS_ARM7_FLAGS) -MMD -MP -MF nds-net7-mwl-rx.d -c $< -o $@
$(NDS_NET7_ELF): nds-net7-module.o nds-net7-mwl-rx.o $(NDS_ARM7_ELF) $(PROJECT_ROOT)/linker/nds_arm7_net7.ld
	$(CC) -march=armv4t -mthumb -nostartfiles -Wl,-Map,nds-net7.map \
		-Wl,-T,$(PROJECT_ROOT)/linker/nds_arm7_net7.ld -Wl,--just-symbols=$(NDS_ARM7_ELF) \
		nds-net7-module.o nds-net7-mwl-rx.o -L$(CALICO)/lib -lcalico_ds7 -lgcc -lc -lgcc -o $@
$(NITROFS_DIR)/net/net7.bin: $(NDS_NET7_ELF)
	@mkdir -p $(dir $@)
	$(OBJCOPY) -O binary $< $@
$(OUTPUT).nds: $(NITROFS_DIR)/net/net7.bin

# P3 build identity (plan section 7.1): net/build.id hashes the loadable
# ARM9 and ARM7 bytes and every other NitroFS file, and consoles refuse a
# room whose id differs from their own. It has to see the finished NitroFS
# tree, so it runs in the ROM recipe itself, right before ndstool; this
# recipe replaces ds_rules' %.nds one and keeps its ndstool line.
$(OUTPUT).nds:
	@python "$(PROJECT_ROOT)/scripts/nds_net_build_id.py" --arm9 "$(OUTPUT).elf" --arm7 "$(NDS_ARM7_ELF)" --nitrofs "$(NITROFS_DIR)" --out "$(NITROFS_DIR)/net/build.id"
	$(SILENTCMD)ndstool -c $@ -9 $(OUTPUT).elf $(_ARM7_ELF) -b $(GAME_ICON) "$(GAME_TITLE);$(GAME_SUBTITLE1);$(GAME_SUBTITLE2)" $(_ADDFILES)
	$(SILENTMSG) built ... $(notdir $@)
-include nds-net7-module.d
-include nds-net7-mwl-rx.d
-include $(NDS_ARM7_OBJECTS:.o=.d)

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
NDS_NATIVE_IMAGE_OWNERS := mario_skeleton1 fox_skeleton1
ifeq ($(NDS_NATIVE_OWNER_IMAGE_MARIOFOX),1)
NDS_NATIVE_IMAGE_OWNERS += mariofox
endif
NDS_NATIVE_KIRBY_HAT_IDS := 03 04 05 06 07 08 09 10 11 12 13
NDS_NATIVE_KIRBY_HAT_STEMS :=
ifeq ($(NDS_P2_LUIGI),1)
NDS_NATIVE_IMAGE_OWNERS += luigi
endif
