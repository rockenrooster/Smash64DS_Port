# The inner make: particle banks, the dense animation bank, the frontend
# overlay checks, the hot-text (ITCM/DTCM) placement and per-object flags.
# Included in place from the Makefile (P8 G); its text is unchanged.

NDS_PARTICLE_BANKS_STAMP := $(PROJECT_ROOT)/src/nds/generated/nds_particle_banks.flags.stamp

$(NDS_PARTICLE_BANKS_STAMP): FORCE
	@mkdir -p $(dir $@)
	@printf %s "$(NDS_PARTICLE_BANKS_FLAGS)" > $@.tmp
	@if ! cmp -s $@.tmp $@; then mv -f $@.tmp $@; else rm -f $@.tmp; fi

$(NDS_PARTICLE_BANKS_INC) $(NDS_PARTICLE_TEXTURE_ASSET) $(NDS_PARTICLE_QUAD_ASSET) $(NDS_WHISPY_NATIVE_ASSET) $(NDS_HYRULE_NATIVE_ASSET) &: \
		$(PROJECT_ROOT)/scripts/generate_nds_particle_banks.py \
		$(PROJECT_ROOT)/scripts/2d_vfx/generate_task39_effect_census.py \
		$(BATTLESHIP_O2R)/particles/efcommon_particle_scb \
		$(BATTLESHIP_O2R)/particles/efcommon_particle_txb \
		$(BATTLESHIP_O2R)/particles/grpupupu_particle_scb \
		$(BATTLESHIP_O2R)/particles/grpupupu_particle_txb \
		$(BATTLESHIP_O2R)/particles/gryoster_particle_scb \
		$(BATTLESHIP_O2R)/particles/gryoster_particle_txb \
		$(BATTLESHIP_O2R)/particles/grhyrule_particle_scb \
		$(BATTLESHIP_O2R)/particles/grhyrule_particle_txb \
		$(NDS_BUILD_CONFIG) \
		$(NDS_PARTICLE_BANKS_STAMP) \
		$(PROJECT_ROOT)/$(BATTLESHIP_DECOMP)/src/ef/efmanager.c
	python "$(PROJECT_ROOT)/scripts/generate_nds_particle_banks.py"

$(NDS_PARTICLE_BANKS_HEADER): $(NDS_PARTICLE_BANKS_INC)

.PHONY: prepare-particle-banks
prepare-particle-banks: $(NDS_PARTICLE_BANKS_INC) $(NDS_PARTICLE_BANKS_HEADER)

# Slice 32's baked animation bank. `--verify` is not optional here: the emitter
# range-checks every field as it encodes and then decodes the whole blob back
# against the bake, and its failure mode without that is a silently saturated
# field in one joint of one animation -- no crash, no counter, nothing a
# screenshot shows. It costs about thirty seconds against a build that takes
# minutes, and it is the only thing standing between a bank drift and wrong
# animation data shipping.
$(NDS_FTANIM_DENSE_ASSET): \
		$(PROJECT_ROOT)/scripts/generate_ftanim_dense_bank.py \
		$(PROJECT_ROOT)/scripts/ftanim_reloc_probe.py \
		$(PROJECT_ROOT)/scripts/ftanim_script_model.py \
		$(PROJECT_ROOT)/scripts/ftanim_bake.py \
		$(NDS_FTANIM_DENSE_SOURCES)
	@mkdir -p $(dir $@)
	python "$(PROJECT_ROOT)/scripts/generate_ftanim_dense_bank.py" \
		--verify --out "$@"

$(NITROFS_DIR)/animation/ftanim_dense_bank.bin: $(NDS_FTANIM_DENSE_ASSET)
	@mkdir -p $(dir $@)
	@cp $< $@

# The resident pack. `--blob-out` runs the slot-level equivalence check before it
# writes: every DObj slot of every clip is decoded OUT OF THE EMITTED BLOB, at
# the blob's own offsets, and compared command-for-command against the same slot
# decoded from the o2r file through the ROM's own pipeline. It is not optional
# for the same reason the dense bank's --verify is not: a wrong offset in one
# slot of one clip is a garbage script handed to the parser, and the parser's
# failure mode on garbage is a freeze, not a diagnostic.
$(NDS_BATTLEPACK_BLOB): \
		$(PROJECT_ROOT)/scripts/generate_battlepack_anim.py \
		$(PROJECT_ROOT)/scripts/ftanim_reloc_probe.py \
		$(NDS_FTANIM_DENSE_SOURCES)
	@mkdir -p $(dir $@)
	python "$(PROJECT_ROOT)/scripts/generate_battlepack_anim.py" \
		--fighter fox --items-off --blob-out "$@"

$(NITROFS_DIR)/animation/battlepack_fox.bin: $(NDS_BATTLEPACK_BLOB)
	@mkdir -p $(dir $@)
	@cp $< $@

$(NDS_FTANIM_STREAM_ASSET): \
		$(PROJECT_ROOT)/scripts/generate_battlepack_anim.py \
		$(PROJECT_ROOT)/scripts/ftanim_reloc_probe.py \
		$(NDS_FTANIM_STREAM_SOURCES)
	@mkdir -p $(dir $@)
	python "$(PROJECT_ROOT)/scripts/generate_battlepack_anim.py" \
		--stream-out "$@"

$(NITROFS_DIR)/animation/ftanim_stream_pack.bin: $(NDS_FTANIM_STREAM_ASSET)
	@mkdir -p $(dir $@)
	@cp $< $@

# The dense runtime's rows. `--verify` is deliberately NOT passed (it is the
# 90-second three-layer corpus proof and belongs to a checker run, not to every
# build), but LAYER D always runs inside `--emit-c` and the generator returns
# non-zero on it: every emitted figatree entry is decoded back out of the rows
# at the offset the directory publishes and compared against the o2r script that
# entry actually points at. A pack that binds joint 7 to joint 8's script is an
# animation that plays on the wrong bone with no crash and no counter, so this
# is not optional for the same reason the battlepack's slot check is not.
$(NDS_FTANIM_TRACK_HEADER): \
		$(PROJECT_ROOT)/scripts/generate_ftanim_track_pack.py \
		$(PROJECT_ROOT)/scripts/ftanim_reloc_probe.py \
		$(PROJECT_ROOT)/scripts/ftanim_script_model.py \
		$(NDS_FTANIM_DENSE_SOURCES)
	@mkdir -p $(dir $@)
	python "$(PROJECT_ROOT)/scripts/generate_ftanim_track_pack.py" \
		--items-off --fighter fox --emit-c "$@" \
		--max-bytes $(NDS_FTANIM_TRACK_MAX_BYTES)

$(NITROFS_DIR)/particles/efcommon_particle_textures.ds.bin: $(NDS_PARTICLE_TEXTURE_ASSET)
	@mkdir -p $(dir $@)
	@cp $< $@

$(NITROFS_DIR)/particles/efcommon_particle_quads.a5i3.bin: $(NDS_PARTICLE_QUAD_ASSET)
	@mkdir -p $(dir $@)
	@cp $< $@

$(NITROFS_DIR)/particles/grpupupu_whispy_native.ds.bin: $(NDS_WHISPY_NATIVE_ASSET)
	@mkdir -p $(dir $@)
	@cp $< $@

$(NITROFS_DIR)/particles/grhyrule_native.ds.bin: $(NDS_HYRULE_NATIVE_ASSET)
	@mkdir -p $(dir $@)
	@cp $< $@

prune-obsolete-audio:
	@rm -f $(foreach file,$(NDS_AUDIO_OBSOLETE_DERIVED_FILES),$(NITROFS_DIR)/$(file))

.PHONY: prune-streamed-ftanim
# Prune ONLY the files the AObj16 pack replaces, never the retained AObj32 /
# spline files. Until 2026-09-07 this globbed FT*Anim*, which also removed
# FTMarioAnim134/135 and FTFoxAnim135/136 -- the four event32 entry
# animations the runtime opens as loose files -- and under -j Make had
# already judged those four up to date before the rm ran, so about half the
# builds shipped without them: `openfail errno=2 ... FTFoxAnim136` on
# Castle/Yoster/Inishie, `direct_step=2`, and the Fox Arwing / Mario pipe
# entries fell back. The order-only prerequisite below cannot protect a
# file Make never rebuilds.
NDS_FTANIM_STREAM_PRUNE_FILES := $(filter-out \
	$(NDS_FTANIM_STREAM_AOBJ32_FILES) $(NDS_FTANIM_STREAM_SPLINE_FILES),\
	$(NDS_FTANIM_STREAM_REPLACED_RELOC_FILES))
prune-streamed-ftanim:
	@rm -f \
		$(foreach file,$(NDS_FTANIM_STREAM_PRUNE_FILES),$(NITROFS_DIR)/reloc/$(file)) \
		$(NITROFS_DIR)/zz_stream/ftanim_stream_pack.bin

# The prune must finish before Make decides whether any retained reloc target
# needs its normal copy recipe. This also prevents two concurrent cp processes
# racing on the named AObj32 files in an incremental build.
$(NDS_NITROFS_RELOC_FILES): | prune-streamed-ftanim

$(OUTPUT).nds: prune-obsolete-audio prune-streamed-ftanim $(OUTPUT).elf $(NDS_NITROFS_RELOC_FILES) $(NDS_NITROFS_P4_FILES) $(NDS_NITROFS_RELOCDATA_FILES) $(NDS_NITROFS_AUDIO_FILES) $(NDS_NITROFS_BATTLE_STATIC_TEXTURE_FILES) $(NDS_NITROFS_PARTICLE_FILES) $(NDS_NITROFS_EFFECT_FILES) $(NDS_NITROFS_FTANIM_FILES) $(NDS_NITROFS_BATTLEPACK_FILES) $(NDS_NITROFS_MN_UI_KIT_FILES) $(NDS_NITROFS_NATIVE_IMAGE_FILES) $(NDS_NITROFS_SHIELD_POSE_FILES) $(NDS_NITROFS_ADMISSION_FILES) $(NDS_BANNER_ICON)

# All targets share this packaging boundary, including diagnostics and P1.
# Audit actual link objects and their textual compiler inputs before ndstool.
.PHONY: native-only-rom-check
native-only-rom-check: $(OUTPUT).elf
	$(file >$(CURDIR)/native-rom-objects.list,$(OFILES))
	@python "$(PROJECT_ROOT)/scripts/check_native_only_rom.py" --elf "$(OUTPUT).elf" --objects-list "$(CURDIR)/native-rom-objects.list" --build-dir "$(CURDIR)"
$(OUTPUT).nds: | native-only-rom-check

# A reclaimed frontend range must never be reached by battle code. Review
# exact resident callback/entry pairs, and keep this gate before packaging.
.PHONY: frontend-overlay-rom-check
frontend-overlay-rom-check: $(OUTPUT).elf
	@python "$(PROJECT_ROOT)/scripts/check_frontend_overlay.py" "$(OUTPUT).elf" $(if $(filter 1,$(NDS_P2_MENU_SHELL)),--required,) --allowlist "$(PROJECT_ROOT)/scripts/frontend_overlay_allowlist.json" --json "$(CURDIR)/frontend-overlay-check.json" > "$(CURDIR)/frontend-overlay-check.log" || { cat "$(CURDIR)/frontend-overlay-check.log"; exit 1; }
	@echo FRONTEND_OVERLAY_PASS: $(notdir $(OUTPUT).elf)
$(OUTPUT).nds: | frontend-overlay-rom-check

# Custom lab output roots need not exist yet. Directory timestamps never
# invalidate the ELF; this only ensures the linker can create its output.
$(OUTPUT).elf: | $(dir $(OUTPUT))
$(dir $(OUTPUT)):
	@mkdir -p "$@"

$(OUTPUT).elf: $(OFILES) \
	$(NDS_HOT_TEXT_SPECS) $(NDS_HOT_TEXT_LINKER_SCRIPT) $(NDS_MEMORY_LINKER_SCRIPT) $(NDS_FRONTEND_OVERLAY_LINKER_SCRIPT) \
	$(NDS_TASK32_DRAW_HOT_FRAGMENT) $(NDS_PARTICLE_BANKS_INC) \
	$(NDS_BATTLE_STATIC_TEXTURE_INC) $(NDS_ENTRY_EFFECT_INC) \
	$(NDS_NATIVE_STAGE_OWNER_INC) $(NDS_NATIVE_STAGE_YOSTER_INC) \
	$(NDS_NATIVE_STAGE_JUNGLE_INC) $(NDS_NATIVE_STAGE_CASTLE_INC) \
	$(NDS_NATIVE_STAGE_SECTOR_INC) $(NDS_NATIVE_STAGE_HYRULE_INC) \
	$(NDS_NATIVE_STAGE_INISHIE_INC) \
	$(NDS_NATIVE_STAGE_ZEBES_INC) \
	$(NDS_NATIVE_STAGE_YAMABUKI_INC) \
	$(NDS_NATIVE_STAGE_PUPUPUSMALL_INC) \
	$(NDS_NATIVE_STAGE_YOSTERSMALL_INC) \
	$(NDS_NATIVE_STAGE_METAL_INC) \
	$(NDS_NATIVE_STAGE_ZAKO_INC) \
	$(NDS_NATIVE_STAGE_LAST_INC) \
	$(NDS_NATIVE_STAGE_BONUS3_INC) \
	$(NDS_NATIVE_STAGE_BONUS1_MARIO_INC) \
	$(NDS_NATIVE_STAGE_BONUS1_FOX_INC) \
	$(NDS_NATIVE_STAGE_BONUS1_DONKEY_INC) \
	$(NDS_NATIVE_STAGE_BONUS1_SAMUS_INC) \
	$(NDS_NATIVE_STAGE_BONUS1_LUIGI_INC) \
	$(NDS_NATIVE_STAGE_BONUS1_LINK_INC) \
	$(NDS_NATIVE_STAGE_BONUS1_YOSHI_INC) \
	$(NDS_NATIVE_STAGE_BONUS1_CAPTAIN_INC) \
	$(NDS_NATIVE_STAGE_BONUS1_KIRBY_INC) \
	$(NDS_NATIVE_STAGE_BONUS1_PIKACHU_INC) \
	$(NDS_NATIVE_STAGE_BONUS1_PURIN_INC) \
	$(NDS_NATIVE_STAGE_BONUS1_NESS_INC) \
	$(NDS_NATIVE_STAGE_BONUS2_MARIO_INC) \
	$(NDS_NATIVE_STAGE_BONUS2_FOX_INC) \
	$(NDS_NATIVE_STAGE_BONUS2_DONKEY_INC) \
	$(NDS_NATIVE_STAGE_BONUS2_SAMUS_INC) \
	$(NDS_NATIVE_STAGE_BONUS2_LUIGI_INC) \
	$(NDS_NATIVE_STAGE_BONUS2_LINK_INC) \
	$(NDS_NATIVE_STAGE_BONUS2_YOSHI_INC) \
	$(NDS_NATIVE_STAGE_BONUS2_CAPTAIN_INC) \
	$(NDS_NATIVE_STAGE_BONUS2_KIRBY_INC) \
	$(NDS_NATIVE_STAGE_BONUS2_PIKACHU_INC) \
	$(NDS_NATIVE_STAGE_BONUS2_PURIN_INC) \
	$(NDS_NATIVE_STAGE_BONUS2_NESS_INC) \
	$(NDS_BATTLE_HUD_INC) \
	$(if $(filter 1,$(NDS_P2_UI_KIT)),$(NDS_MN_UI_KIT_INC) \
		$(NDS_MN_TITLE_ANIM_INC)) \
	$(NDS_NATIVE_ACTOR_YOSTER_PACKET) \
	$(NDS_NATIVE_ACTOR_TARU_PACKET) $(NDS_NATIVE_ACTOR_TARU_HEADER) \
	$(NDS_NATIVE_IMAGE_HEADER)
$(OFILES): $(NDS_MAKEFILES) $(NDS_BUILD_CONFIG) $(NDS_FTANIM_TRACK_PREREQ)
# Revision telemetry owners only. Every other TU must NOT depend on the
# revision header, or the per-commit recompile this split removes returns.
nds_platform.o: $(NDS_BUILD_REVISION)
# EVERY OBJECT THAT INCLUDES A GENERATED HEADER NAMES IT HERE, AND THE .d FILE
# IS NOT A SUBSTITUTE. The rules above spell these paths with $(PROJECT_ROOT),
# which MSYS make expands to `/d/Stuff/...`, while gcc -MMD writes
# `D:/Stuff/...` into the .d -- two DIFFERENT nodes in make's graph, so the .d
# edge carries no ordering. Builds are parallel, so the generator and the
# compile of its consumer are siblings under $(OUTPUT).elf and race.
#
# MEASURED, 2026-08-25 (row P2-3f8): a UI-kit bake change produced
# mn_surfaces.bin and mn_ui_kit.generated.inc at 12:23 while nds_ui_kit.o
# stayed at 12:14, and the ROM linked from them failed p2_shell_loop with
# `BACKDROP SURFACES: mismatch=145` -- the stale metric table's per-surface
# FNV against the fresh pack. The very next `make` with no source change
# recompiled the file, which is the proof it was an ordering race and not a
# bad bake. The failure mode is silent wrong art whenever the runtime does
# not happen to hash-check the asset, so add a line here with any new
# generated include rather than trusting -MMD.
nds_ui_kit.o: $(NDS_MN_UI_KIT_INC) $(NDS_MN_TITLE_ANIM_INC)
nds_native_wallpaper.o: $(NDS_NATIVE_WALLPAPER_INC)
$(OUTPUT).nds: $(foreach name,$(NDS_NATIVE_WALLPAPER_NAMES),$(NITROFS_DIR)/wallpapers/native_wallpaper_$(name).bin)

$(NITROFS_DIR)/wallpapers/%.bin: $(PROJECT_ROOT)/assets/wallpapers/%.bin
	@mkdir -p $(dir $@)
	cp $< $@

# P2-6. The 1P intro's fighter stills, baked from this renderer by the lab
# ROM NDS_1P_INTRO_BAKE=1 (scripts/menus/bake_1p_intro_stills.ps1, packed by
# scripts/menus/pack_1p_intro_stills.py). Campaign builds only.
ifeq ($(NDS_P2_1P_GAME),1)
NDS_1P_INTRO_STILLS := $(wildcard $(PROJECT_ROOT)/assets/intro/*.s1i)
$(OUTPUT).nds: $(patsubst $(PROJECT_ROOT)/assets/intro/%,$(NITROFS_DIR)/intro/%,$(NDS_1P_INTRO_STILLS))
$(NITROFS_DIR)/intro/%.s1i: $(PROJECT_ROOT)/assets/intro/%.s1i
	@mkdir -p $(dir $@)
	cp $< $@
endif

$(NDS_NATIVE_WALLPAPER_INC) $(NDS_NATIVE_WALLPAPER_ASSETS) &: \
		$(PROJECT_ROOT)/scripts/stages/generate_native_wallpapers.py \
		$(PROJECT_ROOT)/scripts/menus/generate_mn_ui_kit.py \
		$(BATTLESHIP_O2R)/reloc_stages/StageDreamLand \
		$(BATTLESHIP_O2R)/reloc_stages/StageZebes \
		$(BATTLESHIP_O2R)/reloc_stages/StageJungle \
		$(BATTLESHIP_O2R)/reloc_stages/StageYoshi \
		$(BATTLESHIP_O2R)/reloc_stages/StagePokemon \
		$(BATTLESHIP_O2R)/reloc_movies/MVOpeningRoomWallpaper \
		$(BATTLESHIP_O2R)/reloc_stages/StageSector \
		$(BATTLESHIP_O2R)/reloc_stages/StageCastle \
		$(BATTLESHIP_O2R)/reloc_stages/StageHyruleWallpaper \
		$(BATTLESHIP_O2R)/reloc_stages/StageYamabukiWallpaper \
		$(BATTLESHIP_O2R)/reloc_stages/StageInishieWallpaper \
		$(BATTLESHIP_O2R)/reloc_stages/StageLastWallpaper \
		$(BATTLESHIP_O2R)/reloc_stages/StageMetalWallpaper \
		$(BATTLESHIP_O2R)/reloc_menus/MNVSResults
	python "$(PROJECT_ROOT)/scripts/stages/generate_native_wallpapers.py" --repo-root "$(PROJECT_ROOT)" --output-dir "$(PROJECT_ROOT)/assets/wallpapers" --header "$(NDS_NATIVE_WALLPAPER_INC)"
nds_menu_shell.o: $(NDS_MN_UI_KIT_INC)
nds_battle_hud.o: $(NDS_BATTLE_HUD_INC)
battle_playable_static_textures.o: $(NDS_BATTLE_STATIC_TEXTURE_INC)
nds_particle_banks.o: $(NDS_PARTICLE_BANKS_INC)
# GCC .d paths use D:/ while Make's producer paths use /d/. Name the producer
# dependency explicitly so a configuration switch cannot compile a stale header.
battleship_efmanager.o battleship_lbparticle.o scene_backend.o: $(NDS_PARTICLE_BANKS_HEADER)
nds_shield_pose.o: $(if $(strip $(NDS_SHIELD_POSE_IDS)),$(NDS_SHIELD_POSE_HEADER))
nds_renderer.o: $(NDS_NATIVE_ITEM_BAT_PACKET) $(NDS_NATIVE_ITEM_BAT_HEADER) $(NDS_NATIVE_ITEM_BOMBHEI_PACKET) $(NDS_NATIVE_ITEM_BOMBHEI_HEADER) $(NDS_NATIVE_ITEM_RSHELL_PACKET) $(NDS_NATIVE_ITEM_RSHELL_HEADER) $(NDS_NATIVE_ITEM_LGUN_PACKET) $(NDS_NATIVE_ITEM_LGUN_HEADER) $(NDS_NATIVE_ITEM_HARISEN_PACKET) $(NDS_NATIVE_ITEM_HARISEN_HEADER) $(NDS_NATIVE_ITEM_HEART_PACKET) $(NDS_NATIVE_ITEM_HEART_HEADER) $(NDS_NATIVE_ITEM_STARROD_PACKET) $(NDS_NATIVE_ITEM_STARROD_HEADER) $(NDS_NATIVE_ITEM_FFLOWER_PACKET) $(NDS_NATIVE_ITEM_FFLOWER_HEADER) $(NDS_NATIVE_ITEM_MSBOMB_PACKET) $(NDS_NATIVE_ITEM_MSBOMB_HEADER) $(NDS_NATIVE_ITEM_NBUMPER_PACKET) $(NDS_NATIVE_ITEM_NBUMPER_HEADER) $(NDS_NATIVE_ITEM_BOX_PACKET) $(NDS_NATIVE_ITEM_BOX_HEADER) $(NDS_NATIVE_ITEM_TARU_PACKET) $(NDS_NATIVE_ITEM_TARU_HEADER) $(NDS_NATIVE_ITEM_EGG_PACKET) $(NDS_NATIVE_ITEM_EGG_HEADER) $(NDS_NATIVE_ITEM_IWARK_PACKET) $(NDS_NATIVE_ITEM_IWARK_HEADER) $(NDS_NATIVE_ITEM_STAR_PACKET) $(NDS_NATIVE_ITEM_STAR_HEADER) $(NDS_NATIVE_ITEM_SWORD_PACKET) $(NDS_NATIVE_ITEM_SWORD_HEADER) $(NDS_NATIVE_ITEM_HAMMER_PACKET) $(NDS_NATIVE_ITEM_HAMMER_HEADER) $(NDS_NATIVE_ITEM_MBALL_PACKET) $(NDS_NATIVE_ITEM_MBALL_HEADER) $(NDS_NATIVE_ITEM_KIRBYSTAR_PACKET) $(NDS_NATIVE_ITEM_KIRBYSTAR_HEADER) $(NDS_NATIVE_ITEM_GSHELL_PACKET) $(NDS_NATIVE_ITEM_GSHELL_HEADER) $(NDS_NATIVE_ITEM_GLUCKY_PACKET) $(NDS_NATIVE_ITEM_GLUCKY_HEADER) $(NDS_NATIVE_ITEM_PORYGON_PACKET) $(NDS_NATIVE_ITEM_PORYGON_HEADER) $(NDS_NATIVE_ITEM_HITOKAGE_PACKET) $(NDS_NATIVE_ITEM_HITOKAGE_HEADER) $(NDS_NATIVE_ITEM_FUSHIGIBANA_PACKET) $(NDS_NATIVE_ITEM_FUSHIGIBANA_HEADER) $(NDS_ENTRY_EFFECT_INC) $(NDS_PARTICLE_BANKS_INC) $(NDS_NATIVE_ACTOR_YOSTER_PACKET) $(NDS_NATIVE_ACTOR_TARU_PACKET) $(NDS_NATIVE_ACTOR_TARU_HEADER) $(NDS_NATIVE_SECTOR_LASER_PACKET) $(NDS_NATIVE_SECTOR_LASER_HEADER) $(NDS_NATIVE_CASTLE_BUMPER_PACKET) $(NDS_NATIVE_CASTLE_BUMPER_HEADER) $(NDS_NATIVE_LINK_BOMB_PACKET) $(NDS_NATIVE_LINK_BOMB_HEADER) $(NDS_NATIVE_MARUMINE_PACKET) $(NDS_NATIVE_MARUMINE_HEADER) $(NDS_NATIVE_INISHIE_POWBLOCK_PACKET) $(NDS_NATIVE_INISHIE_POWBLOCK_HEADER) $(NDS_NATIVE_THUNDERJOLT_PACKET) $(NDS_NATIVE_THUNDERJOLT_HEADER) $(NDS_NATIVE_THUNDERGROUND_PACKET) $(NDS_NATIVE_THUNDERGROUND_HEADER) $(NDS_NATIVE_THUNDERJOLTFX_PACKET) $(NDS_NATIVE_THUNDERJOLTFX_HEADER) $(NDS_NATIVE_DAMAGE_SLASH_PACKET) $(NDS_NATIVE_DAMAGE_SLASH_HEADER) $(NDS_NATIVE_ITEM_TOMATO_PACKET) $(NDS_NATIVE_ITEM_TOMATO_HEADER) $(NDS_NATIVE_CHARGESHOT_PACKET) $(NDS_NATIVE_CHARGESHOT_HEADER) $(NDS_NATIVE_OWNER_IR) $(NDS_NATIVE_IMAGE_HEADER)
scene_backend.o: $(NDS_NATIVE_ITEM_BAT_HEADER) $(NDS_NATIVE_ITEM_BOMBHEI_HEADER) $(NDS_NATIVE_ITEM_RSHELL_HEADER) $(NDS_NATIVE_ITEM_LGUN_HEADER) $(NDS_NATIVE_ITEM_HARISEN_HEADER) $(NDS_NATIVE_ITEM_HEART_HEADER) $(NDS_NATIVE_ITEM_STARROD_HEADER) $(NDS_NATIVE_ITEM_FFLOWER_HEADER) $(NDS_NATIVE_ITEM_MSBOMB_HEADER) $(NDS_NATIVE_ITEM_NBUMPER_HEADER) $(NDS_NATIVE_ITEM_BOX_HEADER) $(NDS_NATIVE_ITEM_TARU_HEADER) $(NDS_NATIVE_ITEM_EGG_HEADER) $(NDS_NATIVE_ITEM_IWARK_HEADER) $(NDS_NATIVE_ITEM_STAR_HEADER) $(NDS_NATIVE_ITEM_SWORD_HEADER) $(NDS_NATIVE_ITEM_HAMMER_HEADER) $(NDS_NATIVE_ITEM_MBALL_HEADER) $(NDS_NATIVE_ITEM_KIRBYSTAR_HEADER) $(NDS_NATIVE_ITEM_GSHELL_HEADER) $(NDS_NATIVE_ITEM_GLUCKY_HEADER) $(NDS_NATIVE_ITEM_PORYGON_HEADER) $(NDS_NATIVE_ITEM_HITOKAGE_HEADER) $(NDS_NATIVE_ITEM_FUSHIGIBANA_HEADER) $(NDS_NATIVE_SECTOR_LASER_HEADER) $(NDS_NATIVE_CASTLE_BUMPER_HEADER) $(NDS_NATIVE_LINK_BOMB_HEADER) $(NDS_NATIVE_MARUMINE_HEADER) $(NDS_NATIVE_INISHIE_POWBLOCK_HEADER) $(NDS_NATIVE_THUNDERJOLT_HEADER) $(NDS_NATIVE_THUNDERGROUND_HEADER) $(NDS_NATIVE_THUNDERJOLTFX_HEADER) $(NDS_NATIVE_DAMAGE_SLASH_HEADER) $(NDS_NATIVE_ITEM_TOMATO_HEADER) $(NDS_NATIVE_CHARGESHOT_HEADER)
battleship_ftmanager.o battleship_mnplayersvs.o: $(NDS_NATIVE_IMAGE_HEADER)
nds_renderer.o: $(NDS_NATIVE_ITEM_CAPSULE_PACKET) $(NDS_NATIVE_ITEM_CAPSULE_HEADER)
scene_backend.o: $(NDS_NATIVE_ITEM_CAPSULE_HEADER)
scene_backend.o: $(NDS_NATIVE_ACTOR_TARU_HEADER)
nds_renderer.o: $(NDS_NATIVE_NESS_PKFIRE_PACKET) $(NDS_NATIVE_NESS_PKFIRE_HEADER)
scene_backend.o: $(NDS_NATIVE_NESS_PKFIRE_HEADER)
nds_renderer.o: $(NDS_NATIVE_NESS_PKTHUNDER_PACKET) $(NDS_NATIVE_NESS_PKTHUNDER_HEADER)
scene_backend.o: $(NDS_NATIVE_NESS_PKTHUNDER_HEADER)
nds_renderer.o: $(NDS_NATIVE_YOSHI_EGG_PACKET) $(NDS_NATIVE_YOSHI_EGG_HEADER)
scene_backend.o: $(NDS_NATIVE_YOSHI_EGG_HEADER)
nds_renderer.o: $(NDS_NATIVE_YOSHI_EGGLAY_PACKET) $(NDS_NATIVE_YOSHI_EGGLAY_HEADER)
nds_renderer.o: $(NDS_NATIVE_PURIN_SING_PACKET) $(NDS_NATIVE_PURIN_SING_HEADER)
nds_renderer.o: $(NDS_NATIVE_KIRBY_VULCAN_PACKET) $(NDS_NATIVE_KIRBY_VULCAN_HEADER)
scene_backend.o: $(NDS_NATIVE_KIRBY_VULCAN_HEADER)
nds_renderer.o: $(NDS_NATIVE_PIKACHU_THUNDER_PACKET) $(NDS_NATIVE_PIKACHU_THUNDER_HEADER)
scene_backend.o: $(NDS_NATIVE_PIKACHU_THUNDER_HEADER)
nds_renderer.o: $(NDS_NATIVE_SAMUS_BOMB_PACKET) $(NDS_NATIVE_SAMUS_BOMB_HEADER)
scene_backend.o: $(NDS_NATIVE_SAMUS_BOMB_HEADER)
nds_renderer.o: $(NDS_NATIVE_NESS_PKTAIL_PACKET) $(NDS_NATIVE_NESS_PKTAIL_HEADER)
scene_backend.o: $(NDS_NATIVE_NESS_PKTAIL_HEADER)
scene_backend.o: $(NDS_NATIVE_PURIN_SING_HEADER)
scene_backend.o: $(NDS_NATIVE_YOSHI_EGGLAY_HEADER)
nds_renderer.o: $(NDS_NATIVE_YOSHI_ENTRYEGG_PACKET) $(NDS_NATIVE_YOSHI_ENTRYEGG_HEADER)
scene_backend.o: $(NDS_NATIVE_YOSHI_ENTRYEGG_HEADER)
# The outer build exports NDS_NITROFS_RELOC_FILES so the recursive inner make
# receives the exact ROM prerequisite inventory.  P2-3's staged fighter banks
# make that one variable roughly 72 KiB; together with the normal build
# environment it is large enough that MSYS bash can start, but a native
# devkitARM child spawned *through* bash fails with ENOENT on Windows.  Normal
# compile recipes do not cross that shell seam.  Keep the prerequisite export
# intact and scrub only this oversized make-only variable when a shell must
# launch a native tool for the private ITCM archive work below.
NDS_SHELL_DEVKIT_ENV := env -u NDS_NITROFS_RELOC_FILES
NDS_TASK9_FLOAT_LIBGCC := $(shell $(NDS_SHELL_DEVKIT_ENV) $(CC) $(ARCH) -print-libgcc-file-name)
NDS_TASK9_FLOAT_AR := $(shell $(NDS_SHELL_DEVKIT_ENV) $(CC) -print-prog-name=ar)
# Keep the installed archive out of Make's prerequisite graph: `make -B` would
# otherwise try to rebuild that external .a through an implicit archive rule.
# One grouped recipe makes one verified private copy and extracts only from it.
$(NDS_TASK9_FLOAT_ITCM_OFILES) &: $(NDS_MAKEFILES) $(NDS_BUILD_CONFIG)
	@echo "$(NDS_TASK9_FLOAT_LIBGCC_SHA256) *$(NDS_TASK9_FLOAT_LIBGCC)" | sha256sum -c -
	@# Remove BOTH spellings, not just the ones this configuration emits. A build
	@# directory that previously held `<stem>.itcm.o` keeps it when the member
	@# moves to `<stem>.mainram.o`, and the two then look like one member
	@# claiming two placements. check-task9-float-itcm.ps1 fails closed on that,
	@# which is how this was caught rather than shipped.
	@rm -rf ".task9-float-itcm" $(foreach member,$(NDS_TASK9_FLOAT_ITCM_MEMBERS),$(basename $(member)).itcm.o $(basename $(member)).mainram.o)
	@mkdir -p ".task9-float-itcm"
	@cp "$(NDS_TASK9_FLOAT_LIBGCC)" ".task9-float-itcm/libgcc.a"
	@cd ".task9-float-itcm" && $(NDS_SHELL_DEVKIT_ENV) $(NDS_TASK9_FLOAT_AR) x "libgcc.a" $(NDS_TASK9_FLOAT_ITCM_MEMBERS)
	@for member in $(NDS_TASK9_FLOAT_ITCM_MEMBERS); do \
		stem="$${member%.o}"; \
		phase2_filter=""; \
		if test "$$member" = "_arm_cmpsf2.o"; then \
			phase2_filter="--redefine-sym __aeabi_fcmpeq=__nds_task9_libgcc_fcmpeq_golden"; \
		fi; \
		if test "$$member" = "_arm_cmpsf2.o"; then \
			phase2_filter="$$phase2_filter --redefine-sym __aeabi_fcmplt=__nds_task16_libgcc_fcmplt_golden --redefine-sym __aeabi_fcmple=__nds_task16_libgcc_fcmple_golden --redefine-sym __aeabi_fcmpge=__nds_task16_libgcc_fcmpge_golden --redefine-sym __aeabi_fcmpgt=__nds_task16_libgcc_fcmpgt_golden"; \
		fi; \
		if test "$$member" = "_arm_unordsf2.o"; then \
			phase2_filter="$$phase2_filter --redefine-sym __aeabi_fcmpun=__nds_task16_libgcc_fcmpun_golden"; \
		fi; \
		if test "$$member" = "_arm_addsubsf3.o"; then \
			phase2_filter="$$phase2_filter --redefine-sym __aeabi_fadd=__nds_task16_libgcc_fadd_golden --redefine-sym __aeabi_fsub=__nds_task16_libgcc_fsub_golden"; \
		fi; \
		if test "$$member" = "_arm_addsubsf3.o"; then \
			phase2_filter="$$phase2_filter --redefine-sym __aeabi_i2f=__nds_task16_libgcc_i2f_golden"; \
		fi; \
		if test "$(NDS_P2_FLOAT_CONV)" = "1" && test "$$member" = "_arm_addsubsf3.o"; then \
			phase2_filter="$$phase2_filter --redefine-sym __aeabi_ui2f=__nds_p2_libgcc_ui2f_golden --redefine-sym __floatunsisf=__nds_p2_libgcc_floatunsisf_golden --redefine-sym __aeabi_l2f=__nds_p2_libgcc_l2f_golden --redefine-sym __floatdisf=__nds_p2_libgcc_floatdisf_golden --redefine-sym __floatsisf=__nds_p2_libgcc_floatsisf_golden"; \
		fi; \
		rename_filter="--rename-section .text=.itcm,alloc,load,readonly,code,contents"; \
		out="$$stem.itcm.o"; \
		case " $(strip $(NDS_TASK9_FLOAT_MAIN_MEMBERS)) " in \
			*" $$member "*) rename_filter=""; out="$$stem.mainram.o";; \
		esac; \
		$(NDS_SHELL_DEVKIT_ENV) $(OBJCOPY) $$phase2_filter $$rename_filter \
			".task9-float-itcm/$$member" "$$out" || exit $$?; \
	done
	@rm -rf ".task9-float-itcm"
ifneq ($(strip $(NDS_TASK37_ITCM_OFILES)),)
NDS_TASK37_LIBC := $(shell $(NDS_SHELL_DEVKIT_ENV) $(CC) $(ARCH) -print-file-name=libc.a)
NDS_TASK37_LIBM := $(shell $(NDS_SHELL_DEVKIT_ENV) $(CC) $(ARCH) -print-file-name=libm.a)
NDS_TASK37_AR := $(shell $(NDS_SHELL_DEVKIT_ENV) $(CC) -print-prog-name=ar)
# Grouped recipe, one verified private copy per archive, extract only the named
# members. Same shape as the Task 9 block above and for the same reason: the
# installed archives must stay out of Make's prerequisite graph.
$(NDS_TASK37_ITCM_OFILES) &: $(NDS_MAKEFILES) $(NDS_BUILD_CONFIG)
	@test "$(NDS_TASK37_ITCM_LIBC)" != "1" || echo "$(NDS_TASK37_LIBC_SHA256) *$(NDS_TASK37_LIBC)" | sha256sum -c -
	@test "$(NDS_TASK37_ITCM_LIBM)" != "1" || echo "$(NDS_TASK37_LIBM_SHA256) *$(NDS_TASK37_LIBM)" | sha256sum -c -
	@rm -rf ".task37-itcm" $(NDS_TASK37_ITCM_OFILES)
	@mkdir -p ".task37-itcm"
	@cp "$(NDS_TASK37_LIBC)" ".task37-itcm/libc.a"
	@cp "$(NDS_TASK37_LIBM)" ".task37-itcm/libm.a"
	@test "$(NDS_TASK37_ITCM_LIBC)" != "1" || (cd ".task37-itcm" && $(NDS_SHELL_DEVKIT_ENV) $(NDS_TASK37_AR) x "libc.a" $(NDS_TASK37_LIBC_MEMBERS))
	@test "$(NDS_TASK37_ITCM_LIBM)" != "1" || (cd ".task37-itcm" && $(NDS_SHELL_DEVKIT_ENV) $(NDS_TASK37_AR) x "libm.a" $(NDS_TASK37_LIBM_MEMBERS))
	@for member in $(if $(filter 1,$(NDS_TASK37_ITCM_LIBC)),$(NDS_TASK37_LIBC_MEMBERS)) $(if $(filter 1,$(NDS_TASK37_ITCM_LIBM)),$(NDS_TASK37_LIBM_MEMBERS)); do \
		stem="$${member%.o}"; \
		$(NDS_SHELL_DEVKIT_ENV) $(OBJCOPY) \
			--rename-section .text=.itcm,alloc,load,readonly,code,contents \
			".task37-itcm/$$member" "$$stem.itcm.o" || exit $$?; \
	done
	@rm -rf ".task37-itcm"
endif
ifneq ($(strip $(NDS_MPPROCESS_STRICT_OFILES)),)
$(NDS_MPPROCESS_STRICT_OFILES): CFLAGS += -Werror=implicit-function-declaration -Werror=incompatible-pointer-types -Werror=int-conversion -Werror=return-type
endif
# Imported BattleShip TUs wrap decomp sources verbatim (src/import/battleship_*.c
# textually includes decomp/...), so their data tables and callbacks carry the
# source's own -Wmissing-braces (13,438 hits, e.g. ft/ftdata.c) and
# -Wunused-parameter style. Both are proven expected floods in the 37 MB
# reference log and carry no signal for DS-owned code, whose warnings stay fully
# enabled. Scope is deliberately narrow: only these two warning classes, only on
# battleship_*.o. Type/return/implicit-declaration/bounds/uninitialized stay on
# everywhere, including here.
battleship_%.o: CFLAGS += -Wno-missing-braces -Wno-unused-parameter
# ARENA. Every text byte comes out of the one heap the battle arena is carved
# from, and the battle arena is what decides whether heavier rosters start a
# match at all and whether the source's 25 KiB GObj latch drops their effects
# (artifacts/performance/2026-09-19_remaining-bugs.md, arena census). Menu
# translation units never execute inside a battle frame, so size-optimizing
# them costs no battle tick. The -Os harness ROMs already prove this code
# generation end to end.
battleship_mn%.o nds_menu_shell.o nds_ui_kit.o: CFLAGS += -Os
# Same rule for the other scenes that never host a battle frame: the opening
# movies, staff roll, how-to-play, and the 1P intro / stage-clear / challenger
# screens. sc1pgame, training, bonus stages and the auto demo ARE battles and
# keep -O2.
battleship_mv%.o battleship_scstaffroll.o battleship_scexplain.o battleship_sc1pintro.o battleship_sc1pstageclear.o battleship_sc1pchallenger.o: CFLAGS += -Os
# A P4 fighter's own routines (src/port/nds_p4_<name>.c) run only for that
# fighter, in its specials, but all fifteen are resident in every scene, the
# character select's included: their bytes are size-optimized.
nds_p4_falco.o nds_p4_bowser.o nds_p4_marth.o nds_p4_roy.o nds_p4_wario.o nds_p4_peach.o nds_p4_crash.o nds_p4_lanky.o nds_p4_sheik.o nds_p4_sonic.o nds_p4_dedede.o nds_p4_wolf.o nds_p4_ike.o nds_p4_palutena.o: CFLAGS += -Os
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
#
# nds_r2_collision_fixed.o IS that change -- the whole-cluster fixed-point
# narrow phase -- and it is the case where the flag is load-bearing rather than
# marginal. Its arithmetic is 64-bit integer products (60 SMULL/SMLAL measured),
# ARMv5TE Thumb has no SMULL, and -mthumb would turn every one of them into
# `bl __aeabi_lmul`; the renderer's rule just above exists for exactly that
# reason and was worth -511,174 ticks per Results tic. L7a's refutation does not
# transfer, and its own paragraph says why: it was measuring FLOAT code, whose
# helpers are libgcc and do not change mode with the caller.
# scripts/check-r2-collision-fixed.ps1 fails on any __aeabi_lmul in this object,
# so losing this line is a red verifier rather than a silent regression.
#
# src/port/nds_r2_collision_ring.c, the wiring, is deliberately NOT in this
# list. It is Thumb glue -- control flow, f32 loads and f32 stores -- that calls
# the ndsR2CollisionFixed* entry points here, so every 64-bit product in the
# cluster stays inside the one object the check script disassembles. Building
# the ring -marm too would grow it for nothing; inlining the kernels INTO it
# would put SMULL-shaped code in a Thumb object and no gate would notice.
nds_r2_collision_fixed.o: CFLAGS += -marm
# Same reason: 64-bit products throughout (the fixed chain from
# nds_r2_collision_fixed.h), which Thumb would turn into __aeabi_lmul calls.
nds_p2_hurtbox_reject.o: CFLAGS += -marm
# Same rule, same reason: src/port/nds_r2_sim_mac_fixed.c holds the shadow
# bodies of the warm-MAC exchange-rate instrument, and its whole content is
# 64-bit integer products. A Thumb build of it would price __aeabi_lmul at 4.49
# cyc/multiply instead of a hardware umull at 2.00 and report an exchange rate
# that no shipped conversion would ever see.
nds_r2_sim_mac_fixed.o: CFLAGS += -marm
# Same rule for the same reason, and here it is also the THING BEING MEASURED:
# one of this object's arms is include/nds/nds_r2_sqrtf.h compiled in ARM state,
# against the shipped Thumb build of the identical header in
# src/nds/r2/nds_r2_sqrtf.c. Building this object -mthumb would make both arms
# call __aeabi_lmul and the comparison would read zero.
nds_r2_hwmath_bench.o: CFLAGS += -marm
# Same rule, and this one is the SHIPPED half of it. include/nds/nds_r2_sqrtf.h
# line 73 needs a 48-bit `root * root`; ARMv5TE Thumb has no UMULL, so in Thumb
# state that one expression is `bl __aeabi_lmul` -- visible at 0x0208b10c in
# build-c206-shipgx0's own disassembly. src/nds/r2/nds_r2_sqrtf.c was the last
# R2 kernel object still built -mthumb. -marm costs +24 B of text and takes the
# 11,608 tk/fr sqrtf lane (8,068 sim + 3,540 draw at marginal-80) down by
# 37%-41%. Bit-identical, not merely equivalent: build-c213-hwmath4's
# gNdsR2HwMathBenchSqrtfMismatch is 0 over 65,536 inputs comparing exactly these
# two builds of the header.
#
# EXCEPT in the route build, where this object IS THE CONTROL ARM and must stay
# -mthumb while nds_r2_sqrtf_arm.o carries the -marm body. Building both -marm
# would make the two arms identical and the measurement would read zero -- the
# same trap the bench block above records.
ifeq ($(NDS_R2_HWMATH_ROUTE),1)
nds_r2_sqrtf_arm.o: CFLAGS += -marm
else
nds_r2_sqrtf.o: CFLAGS += -marm
endif

scene_backend.o: $(SCENE_BACKEND_SLICES) $(NDS_SCENE_HARNESS_CONFIG)
scene_harness.o battleship_grinishie_scale.o: $(NDS_SCENE_HARNESS_CONFIG)
nds_ifcommon_oam.o: $(NDS_TASK39_HIT_SPARKS_INC)

$(NDS_BATTLESHIP_IMPORT_OVERLAY_STAMP): \
		$(NDS_BATTLESHIP_IMPORT_OVERLAY_GENERATOR) \
		$(NDS_BATTLESHIP_IMPORT_OVERLAY_PATCHES) \
		$(NDS_BATTLESHIP_IMPORT_OVERLAY_INPUTS)
	pwsh -NoProfile -ExecutionPolicy Bypass -File "$(NDS_BATTLESHIP_IMPORT_OVERLAY_GENERATOR)" \
		-OutputRoot "$(NDS_BATTLESHIP_IMPORT_OVERLAY)"

