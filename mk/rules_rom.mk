# The inner make: the ROM image and what it stages last (battle core files,
# the UI kit, the title animation) and the dependency files.
# Included in place from the Makefile (P8 G); its text is unchanged.

$(OUTPUT).nds: $(NDS_NITROFS_NATIVE_STAGE_BLOB_FILES)

ifneq ($(filter 1,$(NDS_1P_GAME) $(NDS_MENU_SHELL)),)
# Source-derived Main/Model data for all twelve character-select previews.
# 1P and VS CSS share the same native compact format; VS keeps at most four
# packs live in resettable slot arenas instead of raw full fighter closures.
# Generate outside NitroFS so intermediate metadata never enters the ROM.
NDS_PREVIEW_CORE_DIR := $(PROJECT_ROOT)/$(BUILD)/preview-core
NDS_PREVIEW_CORE_NAMES := 00.fpc 01.fpc 02.fpc 03.fpc 04.fpc 05.fpc 06.fpc 07.fpc 08.fpc 09.fpc 10.fpc 11.fpc
NDS_PREVIEW_CORE_FILES := $(addprefix $(NDS_PREVIEW_CORE_DIR)/,$(NDS_PREVIEW_CORE_NAMES))
NDS_PREVIEW_NITRO_FILES := $(addprefix $(NITROFS_DIR)/fighters/preview/,$(NDS_PREVIEW_CORE_NAMES))
NDS_PREVIEW_CORE_DEPS := \
	$(PROJECT_ROOT)/scripts/fighters/generate_preview_core_packs.py \
	$(NDS_OWNER_LINK_INTRO_STAMP) \
	$(PROJECT_ROOT)/scripts/fighters/preview_source_metadata.py \
	$(PROJECT_ROOT)/scripts/fighters/generate_nds_native_owners.py \
	$(PROJECT_ROOT)/scripts/stages/generate_nds_native_stage.py \
	$(PROJECT_ROOT)/scripts/fighters/native_owner_image_arrays.py \
	$(PROJECT_ROOT)/scripts/fighters/fighter_production_manifest.json \
	$(PROJECT_ROOT)/scripts/_paths.py \
	$(wildcard $(PROJECT_ROOT)/scripts/stages/native_stage_descriptors/*.py) \
	$(PROJECT_ROOT)/include/nds/nds_preview_pack.h \
	$(PROJECT_ROOT)/$(BATTLESHIP_DECOMP)/src/ft/ftparam.c \
	$(wildcard $(PROJECT_ROOT)/$(BATTLESHIP_DECOMP)/src/sc/scsubsys/scsubsysdata*.c) \
	$(wildcard $(BATTLESHIP_RELOCDATA)/*.c $(BATTLESHIP_RELOCDATA)/*.reloc $(BATTLESHIP_RELOCDATA)/*.h) \
	$(wildcard $(BATTLESHIP_O2R)/*/*)
$(NDS_PREVIEW_CORE_FILES) &: $(NDS_PREVIEW_CORE_DEPS)
	python "$(PROJECT_ROOT)/scripts/fighters/generate_preview_core_packs.py" --output-dir "$(NDS_PREVIEW_CORE_DIR)"
$(NITROFS_DIR)/fighters/preview/%.fpc: $(NDS_PREVIEW_CORE_DIR)/%.fpc
	@mkdir -p $(dir $@)
	@cp $< $@
$(OUTPUT).nds: $(NDS_PREVIEW_NITRO_FILES)
endif

ifneq ($(filter 1,$(NDS_COMPACT_BATTLE_FIGHTERS)),)
# Source-exact LOW-detail VSBattle core packs.  These are deliberately
# separate from the HIGH-only CSS/1P preview packs above: BattleShip's 3/4
# player path constructs LOW commonparts and falls back to HIGH per joint when
# a LOW DObjDesc has no display.  The generator derives both structural
# closures from the pinned O2Rs and replaces only Gfx/Vtx geometry with native
# root identities.
NDS_BATTLE_CORE_DIR := $(PROJECT_ROOT)/$(BUILD)/battle-core
NDS_BATTLE_CORE_IDS := 00 01 02 03 04 05 06 07 08 09 10 11
NDS_BATTLE_CORE_FPC_FILES := $(foreach id,$(NDS_BATTLE_CORE_IDS),$(NDS_BATTLE_CORE_DIR)/$(id).fpc)
NDS_BATTLE_CORE_EXT_FILES := $(foreach id,$(NDS_BATTLE_CORE_IDS),$(NDS_BATTLE_CORE_DIR)/$(id).ext)
NDS_BATTLE_CORE_FILES := $(NDS_BATTLE_CORE_FPC_FILES) $(NDS_BATTLE_CORE_EXT_FILES)
NDS_BATTLE_CORE_NITRO_FPC := $(foreach id,$(NDS_BATTLE_CORE_IDS),$(NITROFS_DIR)/fighters/battle/$(id).fpc)
NDS_BATTLE_CORE_NITRO_EXT := $(foreach id,$(NDS_BATTLE_CORE_IDS),$(NITROFS_DIR)/fighters/battle/$(id).ext)
NDS_BATTLE_CORE_NITRO_FILES := $(NDS_BATTLE_CORE_NITRO_FPC) $(NDS_BATTLE_CORE_NITRO_EXT)
NDS_BATTLE_CORE_MANIFEST := $(PROJECT_ROOT)/docs/optimization/archive/NDS_BATTLE_CORE_PACKS.generated.json
NDS_BATTLE_CORE_DEPS := \
	$(PROJECT_ROOT)/scripts/fighters/generate_battle_core_packs.py \
	$(NDS_OWNER_LINK_INTRO_STAMP) \
	$(PROJECT_ROOT)/scripts/fighters/generate_nds_fighter_admission.py \
	$(PROJECT_ROOT)/scripts/fighters/generate_preview_core_packs.py \
	$(PROJECT_ROOT)/scripts/fighters/preview_source_metadata.py \
	$(PROJECT_ROOT)/scripts/fighters/estimate_fighter_pack.py \
	$(PROJECT_ROOT)/scripts/fighters/generate_nds_native_owners.py \
	$(PROJECT_ROOT)/scripts/fighters/native_skeletons.py \
	$(PROJECT_ROOT)/scripts/stages/generate_nds_native_stage.py \
	$(PROJECT_ROOT)/scripts/fighters/native_owner_image_arrays.py \
	$(PROJECT_ROOT)/scripts/fighters/fighter_production_manifest.json \
	$(PROJECT_ROOT)/docs/optimization/archive/NDS_SHIELD_POSE_ASSETS.generated.json \
	$(PROJECT_ROOT)/scripts/_paths.py \
	$(wildcard $(PROJECT_ROOT)/scripts/stages/native_stage_descriptors/*.py) \
	$(PROJECT_ROOT)/include/nds/nds_preview_pack.h \
	$(PROJECT_ROOT)/$(BATTLESHIP_DECOMP)/src/ft/ftparam.c \
	$(wildcard $(PROJECT_ROOT)/$(BATTLESHIP_DECOMP)/src/sc/scsubsys/scsubsysdata*.c) \
	$(wildcard $(BATTLESHIP_RELOCDATA)/*.c $(BATTLESHIP_RELOCDATA)/*.reloc $(BATTLESHIP_RELOCDATA)/*.h) \
	$(wildcard $(BATTLESHIP_O2R)/*/*)
$(NDS_BATTLE_CORE_FILES) $(NDS_BATTLE_CORE_MANIFEST) &: $(NDS_BATTLE_CORE_DEPS)
	python "$(PROJECT_ROOT)/scripts/fighters/generate_battle_core_packs.py" --output-dir "$(NDS_BATTLE_CORE_DIR)" --kinds mario,fox,donkey,samus,luigi,link,yoshi,captain,kirby,pikachu,purin,ness --emit-shared
	@touch $(NDS_BATTLE_CORE_FILES) $(NDS_BATTLE_CORE_MANIFEST)
$(NITROFS_DIR)/fighters/battle/%.fpc: $(NDS_BATTLE_CORE_DIR)/%.fpc
	@mkdir -p $(dir $@)
	@cp $< $@
$(NITROFS_DIR)/fighters/battle/%.ext: $(NDS_BATTLE_CORE_DIR)/%.ext
	@mkdir -p $(dir $@)
	@cp $< $@
$(OUTPUT).nds: $(NDS_BATTLE_CORE_NITRO_FILES)
endif


$(NITROFS_DIR)/renderer/battle_static_textures.rgb5a1.bin: $(NDS_BATTLE_STATIC_TEXTURE_ASSET)
	@mkdir -p $(dir $@)
	@cp $< $@

# P2-1c. Grouped and touched for the same two reasons as the block above: one
# invocation writes both outputs, and it is write-if-changed. `include/
# reloc_data.h` is a real prerequisite -- the generator reads every sprite
# offset out of it rather than carrying a second copy, so a moved offset must
# re-bake.
# P2-4: the bake now varies with NDS_STAGE_YOSTER, so the config header is a
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
NDS_MN_UI_KIT_FLAGS := $(NDS_STAGE_YOSTER)$(NDS_STAGE_CASTLE)$(NDS_STAGE_JUNGLE)$(NDS_STAGE_ZEBES)$(NDS_STAGE_HYRULE)$(NDS_STAGE_YAMABUKI)$(NDS_STAGE_INISHIE)$(NDS_STAGE_SECTOR)
# P4 bakes the VS character select on the Remix grid, with gate art for each
# compiled content (NDS_P4_CSS below).
ifeq ($(NDS_P4),1)
NDS_MN_UI_KIT_FLAGS := $(NDS_MN_UI_KIT_FLAGS)P4$(NDS_P4_CONTENTS)
NDS_MN_UI_KIT_P4_CSS := $(NDS_P4_CSS_DIR)/css.json
endif
# P5 bakes the stage select on the 6x3 grid (scripts/p5/p5_stages.py --sss).
ifeq ($(NDS_P5),1)
NDS_MN_UI_KIT_FLAGS := $(NDS_MN_UI_KIT_FLAGS)P5
NDS_MN_UI_KIT_P5_SSS := $(NDS_P5_GEN)/sss/sss.json
endif
NDS_MN_UI_KIT_STAMP := $(PROJECT_ROOT)/src/nds/generated/mn_ui_kit.flags.stamp

$(NDS_MN_UI_KIT_STAMP): FORCE
	@mkdir -p $(dir $@)
	@printf %s "$(NDS_MN_UI_KIT_FLAGS)" > $@.tmp
	@if ! cmp -s $@.tmp $@; then mv -f $@.tmp $@; else rm -f $@.tmp; fi

$(NDS_MN_UI_KIT_INC) $(NDS_MN_UI_KIT_ASSET) $(NDS_MN_UI_SURFACE_ASSET) $(NDS_MN_UI_KIT_NET_ASSET) &: \
		$(PROJECT_ROOT)/scripts/menus/generate_mn_ui_kit.py \
		$(NDS_BUILD_CONFIG) \
		$(NDS_MN_UI_KIT_STAMP) \
		$(NDS_MN_UI_KIT_P4_CSS) \
		$(NDS_MN_UI_KIT_P5_SSS) \
		$(PROJECT_ROOT)/include/reloc_data.h
	NDS_P4_CSS=$(NDS_MN_UI_KIT_P4_CSS) NDS_P5_SSS=$(NDS_MN_UI_KIT_P5_SSS) NDS_STAGE_YOSTER=$(NDS_STAGE_YOSTER) NDS_STAGE_CASTLE=$(NDS_STAGE_CASTLE) NDS_STAGE_JUNGLE=$(NDS_STAGE_JUNGLE) NDS_STAGE_ZEBES=$(NDS_STAGE_ZEBES) NDS_STAGE_HYRULE=$(NDS_STAGE_HYRULE) NDS_STAGE_YAMABUKI=$(NDS_STAGE_YAMABUKI) NDS_STAGE_INISHIE=$(NDS_STAGE_INISHIE) NDS_STAGE_SECTOR=$(NDS_STAGE_SECTOR) python "$(PROJECT_ROOT)/scripts/menus/generate_mn_ui_kit.py" --repo-root "$(PROJECT_ROOT)"
	@touch $(NDS_MN_UI_KIT_INC) $(NDS_MN_UI_KIT_ASSET) $(NDS_MN_UI_SURFACE_ASSET) $(NDS_MN_UI_KIT_NET_ASSET)

# P2-2 lower-screen HUD.  Keep every source container the bake reads on the
# dependency edge so an o2r refresh cannot silently leave a stale C include.
$(NDS_BATTLE_HUD_INC) $(NDS_BATTLE_HUD_ASSET) &: \
		$(PROJECT_ROOT)/scripts/menus/generate_battle_hud.py \
		$(PROJECT_ROOT)/scripts/generate_nds_particle_banks.py \
		$(BATTLESHIP_O2R)/particles/efcommon_particle_txb \
		$(PROJECT_ROOT)/scripts/menus/generate_mn_ui_kit.py \
		$(PROJECT_ROOT)/include/reloc_data.h \
		$(BATTLESHIP_O2R)/reloc_interface/IFCommonPlayerDamage \
		$(BATTLESHIP_O2R)/reloc_interface/IFCommonTimer \
		$(BATTLESHIP_O2R)/reloc_interface/IFCommonDigits \
		$(BATTLESHIP_O2R)/reloc_menus/MNPlayersPortraits \
		$(BATTLESHIP_O2R)/reloc_fighters_main/MarioModel \
		$(BATTLESHIP_O2R)/reloc_fighters_main/FoxModel \
		$(BATTLESHIP_O2R)/reloc_fighters_main/LuigiModel \
		$(BATTLESHIP_O2R)/reloc_fighters_main/YoshiModel \
		$(BATTLESHIP_O2R)/reloc_fighters_main/MMarioModel \
		$(BATTLESHIP_O2R)/reloc_fighters_common/FTStocksZako
	python "$(PROJECT_ROOT)/scripts/menus/generate_battle_hud.py" --repo-root "$(PROJECT_ROOT)"
	@touch $(NDS_BATTLE_HUD_INC) $(NDS_BATTLE_HUD_ASSET)

$(NITROFS_DIR)/menus/battle_hud.bin: $(NDS_BATTLE_HUD_ASSET)
	@mkdir -p $(dir $@)
	cp $< $@

$(OUTPUT).nds: $(NITROFS_DIR)/menus/battle_hud.bin

# P7 U6: the unlock messages' texels (scripts/p7/generate_p7_messages.py, our
# own rendering, committed); the message screen reads its row from NitroFS.
NDS_P7_MESSAGES_ASSET := $(PROJECT_ROOT)/assets/p7/messages.bin
$(NITROFS_DIR)/p7/messages.bin: $(NDS_P7_MESSAGES_ASSET)
	@mkdir -p $(dir $@)
	cp $< $@

$(OUTPUT).nds: $(NITROFS_DIR)/p7/messages.bin

# Source-entry AOT packet.  The generator imports the same display-list/texture
# decoders as the static battle bake and SHA-pins all three source containers;
# list those files explicitly as dependencies as well so a refreshed O2R causes
# the check to run immediately rather than waiting for somebody to invoke the
# script by hand.
$(NDS_ENTRY_EFFECT_INC): \
		$(PROJECT_ROOT)/scripts/3d_vfx/generate_nds_entry_effects.py \
		$(PROJECT_ROOT)/scripts/generate_battle_static_textures.py \
		$(PROJECT_ROOT)/scripts/generate_battle_texture_census.py \
		$(BATTLESHIP_O2R)/reloc_fighters_main/MarioSpecial2 \
		$(BATTLESHIP_O2R)/reloc_fighters_main/FoxSpecial3 \
		$(BATTLESHIP_O2R)/reloc_fighters_main/DonkeySpecial2 \
		$(BATTLESHIP_O2R)/reloc_fighters_main/SamusSpecial2 \
		$(BATTLESHIP_O2R)/reloc_fighters_main/CaptainSpecial2 \
		$(BATTLESHIP_O2R)/reloc_fighters_main/LinkSpecial2 \
		$(BATTLESHIP_O2R)/reloc_fighters_main/LinkModel \
		$(BATTLESHIP_O2R)/reloc_fighters_main/LinkSpecial3 \
		$(BATTLESHIP_O2R)/reloc_fighters_common/FTManagerCommon \
		$(BATTLESHIP_O2R)/reloc_fighters_main/FoxSpecial2 \
		$(BATTLESHIP_O2R)/reloc_effects/EFCommonEffects2 \
		$(BATTLESHIP_O2R)/reloc_effects/EFCommonEffects3 \
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

$(NITROFS_DIR)/menus/mn_ui_kit_net.bin: $(NDS_MN_UI_KIT_NET_ASSET)
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

