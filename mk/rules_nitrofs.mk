# The inner make: NitroFS staging -- compact ground maps, polygons, items,
# audio -- and the native stage packets and blobs.
# Included in place from the Makefile (P8 G); its text is unchanged.

NDS_COMPACT_GROUND_MAP_CONTAINERS := \
	$(BATTLESHIP_O2R)/reloc_movies/MVOpeningRoomWallpaper \
	$(BATTLESHIP_O2R)/reloc_stages/StageSector \
	$(BATTLESHIP_O2R)/reloc_stages/StageJungle \
	$(BATTLESHIP_O2R)/reloc_stages/StageZebes \
	$(BATTLESHIP_O2R)/reloc_stages/StageCastle \
	$(BATTLESHIP_O2R)/reloc_stages/StageYoshi \
	$(BATTLESHIP_O2R)/reloc_stages/StageDreamLand \
	$(BATTLESHIP_O2R)/reloc_stages/StagePokemon \
	$(BATTLESHIP_O2R)/reloc_stages/StageHyruleWallpaper \
	$(BATTLESHIP_O2R)/reloc_stages/StageYamabukiWallpaper \
	$(BATTLESHIP_O2R)/reloc_stages/StageInishieWallpaper \
	$(BATTLESHIP_O2R)/reloc_stages/StageLastWallpaper \
	$(BATTLESHIP_O2R)/reloc_stages/StageMetalWallpaper
NDS_COMPACT_GROUND_MAPS_STAMP := $(PROJECT_ROOT)/$(BUILD)/nds_compact_ground_maps.stamp

$(NDS_COMPACT_GROUND_MAPS_STAMP): FORCE
	@mkdir -p $(dir $@)
	@printf %s "$(NDS_COMPACT_GROUND_MAPS)" > $@.tmp
	@if ! cmp -s $@.tmp $@; then mv -f $@.tmp $@; else rm -f $@.tmp; fi

NDS_COMPACT_GROUND_MAP_TARGETS := \
	$(foreach file,$(NDS_COMPACT_GROUND_MAP_FILES),$(NITROFS_DIR)/reloc/$(file))

$(NDS_COMPACT_GROUND_MAP_TARGETS): $(NITROFS_DIR)/reloc/%: $(BATTLESHIP_O2R)/%
	@mkdir -p $(dir $@)
	@if test "$(NDS_COMPACT_GROUND_MAPS)" = 1; then \
		python "$(PROJECT_ROOT)/scripts/stages/generate_compact_ground_maps.py" \
			--o2r-root "$(BATTLESHIP_O2R)" --map "$*" --output "$@" || exit 1; \
	else \
		cp "$(BATTLESHIP_O2R)/$*" "$@"; \
	fi

$(NDS_COMPACT_GROUND_MAP_TARGETS): $(NDS_COMPACT_GROUND_MAP_CONTAINERS) \
		$(NDS_COMPACT_GROUND_MAPS_STAMP) \
		$(PROJECT_ROOT)/scripts/stages/generate_compact_ground_maps.py

# P7 C1 (2026-10-19): compact polygon models. The Fighting Polygon Team loads
# all eleven polygon models, and ~127 KB of them is N64 geometry the DS never
# reads (the native owners draw from generated programs keyed by each display
# list's source offset). scripts/fighters/generate_compact_polygon_models.py
# keeps the joint tree and an identity cell per named display list; the
# runtime side is ndsRelocCompactModelProbe (src/port/reloc_backend_assets.c),
# which a plain file passes unchanged, so `NDS_COMPACT_POLYGON_MODELS=0`
# stages the plain copies as the A/B control. Default 1 with the 1P game, the
# only scene family that loads them; the stamp re-stages on a flip.
NDS_COMPACT_POLYGON_MODELS ?= $(if $(filter 1,$(NDS_1P_GAME)),1,0)
NDS_COMPACT_POLYGON_MODEL_FILES := \
	reloc_fighters_main/NMarioModel reloc_fighters_main/NFoxModel \
	reloc_fighters_main/NYoshiModel reloc_fighters_main/NKirbyModel \
	reloc_fighters_main/NPurinModel reloc_fighters_main/NPikachuModel \
	reloc_fighters_main/NDonkeyModel reloc_fighters_main/NSamusModel \
	reloc_fighters_main/NLinkModel reloc_fighters_main/NCaptainModel \
	reloc_fighters_main/NNessModel
NDS_COMPACT_POLYGON_MODEL_REFERRERS := \
	$(foreach main,NMarioMain NLuigiMain NFoxMain NYoshiMain NKirbyMain \
		NPurinMain NPikachuMain NDonkeyMain NSamusMain NLinkMain \
		NCaptainMain NNessMain,$(BATTLESHIP_O2R)/reloc_fighters_main/$(main))
NDS_COMPACT_POLYGON_MODELS_STAMP := $(PROJECT_ROOT)/$(BUILD)/nds_compact_polygon_models.stamp

$(NDS_COMPACT_POLYGON_MODELS_STAMP): FORCE
	@mkdir -p $(dir $@)
	@printf %s "$(NDS_COMPACT_POLYGON_MODELS)" > $@.tmp
	@if ! cmp -s $@.tmp $@; then mv -f $@.tmp $@; else rm -f $@.tmp; fi

NDS_COMPACT_POLYGON_MODEL_TARGETS := \
	$(foreach file,$(NDS_COMPACT_POLYGON_MODEL_FILES),$(NITROFS_DIR)/reloc/$(file))

$(NDS_COMPACT_POLYGON_MODEL_TARGETS): $(NITROFS_DIR)/reloc/%: $(BATTLESHIP_O2R)/%
	@mkdir -p $(dir $@)
	@if test "$(NDS_COMPACT_POLYGON_MODELS)" = 1; then \
		python "$(PROJECT_ROOT)/scripts/fighters/generate_compact_polygon_models.py" \
			--o2r-root "$(BATTLESHIP_O2R)" --model "$*" --output "$@" || exit 1; \
	else \
		cp "$(BATTLESHIP_O2R)/$*" "$@"; \
	fi

$(NDS_COMPACT_POLYGON_MODEL_TARGETS): $(NDS_COMPACT_POLYGON_MODEL_REFERRERS) \
		$(NDS_COMPACT_POLYGON_MODELS_STAMP) \
		$(PROJECT_ROOT)/scripts/fighters/generate_compact_polygon_models.py \
		$(PROJECT_ROOT)/scripts/stages/generate_compact_ground_maps.py

$(NITROFS_DIR)/relocdata/us/%: $(BATTLESHIP_RELOCDATA)/%
	@mkdir -p $(dir $@)
	@cp $< $@

$(NITROFS_DIR)/audio/bgm_pupupu_ima.bin: $(PROJECT_ROOT)/assets/audio/bgm_pupupu_ima.bin
	@mkdir -p $(dir $@)
	@cp $< $@

# P2-4 Yoster BGM: same shape as the per-track rules above; gated on the
# stage flag so the default NitroFS set is unchanged.
ifeq ($(NDS_STAGE_YOSTER),1)
$(NITROFS_DIR)/audio/bgm_yoster_ima.bin: $(PROJECT_ROOT)/assets/audio/bgm_yoster_ima.bin
	@mkdir -p $(dir $@)
	@cp $< $@
endif

ifeq ($(NDS_STAGE_CASTLE),1)
$(NITROFS_DIR)/audio/bgm_castle_ima.bin: $(PROJECT_ROOT)/assets/audio/bgm_castle_ima.bin
	@mkdir -p $(dir $@)
	@cp $< $@
endif

ifeq ($(NDS_STAGE_ZEBES),1)
$(NITROFS_DIR)/audio/bgm_zebes_ima.bin: $(PROJECT_ROOT)/assets/audio/bgm_zebes_ima.bin
	@mkdir -p $(dir $@)
	@cp $< $@
endif

ifeq ($(NDS_STAGE_HYRULE),1)
$(NITROFS_DIR)/audio/bgm_hyrule_ima.bin: $(PROJECT_ROOT)/assets/audio/bgm_hyrule_ima.bin
	@mkdir -p $(dir $@)
	@cp $< $@
endif

ifeq ($(NDS_STAGE_YAMABUKI),1)
$(NITROFS_DIR)/audio/bgm_yamabuki_ima.bin: $(PROJECT_ROOT)/assets/audio/bgm_yamabuki_ima.bin
	@mkdir -p $(dir $@)
	@cp $< $@
endif

ifeq ($(NDS_STAGE_INISHIE),1)
$(NITROFS_DIR)/audio/bgm_inishie_ima.bin: $(PROJECT_ROOT)/assets/audio/bgm_inishie_ima.bin
	@mkdir -p $(dir $@)
	@cp $< $@
endif

ifeq ($(NDS_STAGE_JUNGLE),1)
$(NITROFS_DIR)/audio/bgm_jungle_ima.bin: $(PROJECT_ROOT)/assets/audio/bgm_jungle_ima.bin
	@mkdir -p $(dir $@)
	@cp $< $@
endif

ifeq ($(NDS_STAGE_SECTOR),1)
$(NITROFS_DIR)/audio/bgm_sector_ima.bin: $(PROJECT_ROOT)/assets/audio/bgm_sector_ima.bin
	@mkdir -p $(dir $@)
	@cp $< $@
endif

# 2026-09-05: the 33 remaining gmMusicID tracks, rendered offline like the
# stage tracks above (see NDS_AUDIO_DERIVED_FILES for the reproduce line).
$(NITROFS_DIR)/audio/bgm_win_default_ima.bin: $(PROJECT_ROOT)/assets/audio/bgm_win_default_ima.bin
	@mkdir -p $(dir $@)
	@cp $< $@
$(NITROFS_DIR)/audio/bgm_win_metroid_ima.bin: $(PROJECT_ROOT)/assets/audio/bgm_win_metroid_ima.bin
	@mkdir -p $(dir $@)
	@cp $< $@
$(NITROFS_DIR)/audio/bgm_win_donkey_ima.bin: $(PROJECT_ROOT)/assets/audio/bgm_win_donkey_ima.bin
	@mkdir -p $(dir $@)
	@cp $< $@
$(NITROFS_DIR)/audio/bgm_win_kirby_ima.bin: $(PROJECT_ROOT)/assets/audio/bgm_win_kirby_ima.bin
	@mkdir -p $(dir $@)
	@cp $< $@
$(NITROFS_DIR)/audio/bgm_win_mother_ima.bin: $(PROJECT_ROOT)/assets/audio/bgm_win_mother_ima.bin
	@mkdir -p $(dir $@)
	@cp $< $@
$(NITROFS_DIR)/audio/bgm_win_yoshi_ima.bin: $(PROJECT_ROOT)/assets/audio/bgm_win_yoshi_ima.bin
	@mkdir -p $(dir $@)
	@cp $< $@
$(NITROFS_DIR)/audio/bgm_win_fzero_ima.bin: $(PROJECT_ROOT)/assets/audio/bgm_win_fzero_ima.bin
	@mkdir -p $(dir $@)
	@cp $< $@
$(NITROFS_DIR)/audio/bgm_win_pmonsters_ima.bin: $(PROJECT_ROOT)/assets/audio/bgm_win_pmonsters_ima.bin
	@mkdir -p $(dir $@)
	@cp $< $@
$(NITROFS_DIR)/audio/bgm_win_zelda_ima.bin: $(PROJECT_ROOT)/assets/audio/bgm_win_zelda_ima.bin
	@mkdir -p $(dir $@)
	@cp $< $@
ifeq ($(NDS_STAGE_INISHIE),1)
$(NITROFS_DIR)/audio/bgm_inishie_hurry_ima.bin: $(PROJECT_ROOT)/assets/audio/bgm_inishie_hurry_ima.bin
	@mkdir -p $(dir $@)
	@cp $< $@
endif
ifeq ($(NDS_ITEM_CORE),1)
$(NITROFS_DIR)/audio/bgm_hammer_ima.bin: $(PROJECT_ROOT)/assets/audio/bgm_hammer_ima.bin
	@mkdir -p $(dir $@)
	@cp $< $@
$(NITROFS_DIR)/audio/bgm_star_ima.bin: $(PROJECT_ROOT)/assets/audio/bgm_star_ima.bin
	@mkdir -p $(dir $@)
	@cp $< $@
endif
ifeq ($(NDS_1P_GAME),1)
$(NITROFS_DIR)/audio/bgm_boss_stage_ima.bin: $(PROJECT_ROOT)/assets/audio/bgm_boss_stage_ima.bin
	@mkdir -p $(dir $@)
	@cp $< $@
$(NITROFS_DIR)/audio/bgm_boss_entry_ima.bin: $(PROJECT_ROOT)/assets/audio/bgm_boss_entry_ima.bin
	@mkdir -p $(dir $@)
	@cp $< $@
$(NITROFS_DIR)/audio/bgm_last_ima.bin: $(PROJECT_ROOT)/assets/audio/bgm_last_ima.bin
	@mkdir -p $(dir $@)
	@cp $< $@
$(NITROFS_DIR)/audio/bgm_bonus_stage_ima.bin: $(PROJECT_ROOT)/assets/audio/bgm_bonus_stage_ima.bin
	@mkdir -p $(dir $@)
	@cp $< $@
$(NITROFS_DIR)/audio/bgm_stage_clear_ima.bin: $(PROJECT_ROOT)/assets/audio/bgm_stage_clear_ima.bin
	@mkdir -p $(dir $@)
	@cp $< $@
$(NITROFS_DIR)/audio/bgm_bonus_stage_clear_ima.bin: $(PROJECT_ROOT)/assets/audio/bgm_bonus_stage_clear_ima.bin
	@mkdir -p $(dir $@)
	@cp $< $@
$(NITROFS_DIR)/audio/bgm_game_clear_ima.bin: $(PROJECT_ROOT)/assets/audio/bgm_game_clear_ima.bin
	@mkdir -p $(dir $@)
	@cp $< $@
$(NITROFS_DIR)/audio/bgm_bonus_stage_failure_ima.bin: $(PROJECT_ROOT)/assets/audio/bgm_bonus_stage_failure_ima.bin
	@mkdir -p $(dir $@)
	@cp $< $@
$(NITROFS_DIR)/audio/bgm_game_end_choice_ima.bin: $(PROJECT_ROOT)/assets/audio/bgm_game_end_choice_ima.bin
	@mkdir -p $(dir $@)
	@cp $< $@
$(NITROFS_DIR)/audio/bgm_game_over_ima.bin: $(PROJECT_ROOT)/assets/audio/bgm_game_over_ima.bin
	@mkdir -p $(dir $@)
	@cp $< $@
$(NITROFS_DIR)/audio/bgm_opening_ima.bin: $(PROJECT_ROOT)/assets/audio/bgm_opening_ima.bin
	@mkdir -p $(dir $@)
	@cp $< $@
$(NITROFS_DIR)/audio/bgm_explain_ima.bin: $(PROJECT_ROOT)/assets/audio/bgm_explain_ima.bin
	@mkdir -p $(dir $@)
	@cp $< $@
$(NITROFS_DIR)/audio/bgm_intro_ima.bin: $(PROJECT_ROOT)/assets/audio/bgm_intro_ima.bin
	@mkdir -p $(dir $@)
	@cp $< $@
$(NITROFS_DIR)/audio/bgm_zako_ima.bin: $(PROJECT_ROOT)/assets/audio/bgm_zako_ima.bin
	@mkdir -p $(dir $@)
	@cp $< $@
$(NITROFS_DIR)/audio/bgm_metal_ima.bin: $(PROJECT_ROOT)/assets/audio/bgm_metal_ima.bin
	@mkdir -p $(dir $@)
	@cp $< $@
$(NITROFS_DIR)/audio/bgm_ending_ima.bin: $(PROJECT_ROOT)/assets/audio/bgm_ending_ima.bin
	@mkdir -p $(dir $@)
	@cp $< $@
$(NITROFS_DIR)/audio/bgm_staffroll_ima.bin: $(PROJECT_ROOT)/assets/audio/bgm_staffroll_ima.bin
	@mkdir -p $(dir $@)
	@cp $< $@
$(NITROFS_DIR)/audio/bgm_message_ima.bin: $(PROJECT_ROOT)/assets/audio/bgm_message_ima.bin
	@mkdir -p $(dir $@)
	@cp $< $@
$(NITROFS_DIR)/audio/bgm_challenger_ima.bin: $(PROJECT_ROOT)/assets/audio/bgm_challenger_ima.bin
	@mkdir -p $(dir $@)
	@cp $< $@
$(NITROFS_DIR)/audio/bgm_training_mode_ima.bin: $(PROJECT_ROOT)/assets/audio/bgm_training_mode_ima.bin
	@mkdir -p $(dir $@)
	@cp $< $@
$(NITROFS_DIR)/audio/bgm_data_ima.bin: $(PROJECT_ROOT)/assets/audio/bgm_data_ima.bin
	@mkdir -p $(dir $@)
	@cp $< $@
endif

$(NITROFS_DIR)/audio/bgm_win_mario_ima.bin: $(PROJECT_ROOT)/assets/audio/bgm_win_mario_ima.bin
	@mkdir -p $(dir $@)
	@cp $< $@

$(NITROFS_DIR)/audio/bgm_win_fox_ima.bin: $(PROJECT_ROOT)/assets/audio/bgm_win_fox_ima.bin
	@mkdir -p $(dir $@)
	@cp $< $@

$(NITROFS_DIR)/audio/bgm_results_ima.bin: $(PROJECT_ROOT)/assets/audio/bgm_results_ima.bin
	@mkdir -p $(dir $@)
	@cp $< $@

$(NITROFS_DIR)/audio/bgm_mode_select_ima.bin: $(PROJECT_ROOT)/assets/audio/bgm_mode_select_ima.bin
	@mkdir -p $(dir $@)
	@cp $< $@

$(NITROFS_DIR)/audio/bgm_battle_select_ima.bin: $(PROJECT_ROOT)/assets/audio/bgm_battle_select_ima.bin
	@mkdir -p $(dir $@)
	@cp $< $@

$(NITROFS_DIR)/audio/fgm_phase_pack_ima.bin: $(PROJECT_ROOT)/assets/audio/fgm_phase_pack_ima.bin
	@mkdir -p $(dir $@)
	@cp $< $@

# One generator invocation produces the metadata .inc AND the RGB555+A1 payload,
# so they are a grouped target (`&:`) exactly like the particle bank above --
# two separate rules would each run the generator. The census script is a
# prerequisite because generate_battle_playable_static_textures.py imports it
# (see its own comment at generate_battle_playable_texture_census.py:258): when
# the census digest moved on 2026-08-05 the .inc's provenance stamp moved with
# it, and nothing rebuilt. The generator verifies its own decomp inputs against
# pinned sha256s, so corpus drift still fails closed and loudly rather than
# being silently absorbed here.
#
# The `touch` is required, not cosmetic: the generator is deliberately
# write-if-changed (generate_battle_playable_static_textures.py:1652 rewrites
# only when the bytes differ), so a run that confirms the outputs are already
# correct leaves both mtimes untouched. Without the touch the targets stay older
# than their prerequisites and make re-runs the whole generator on EVERY build
# forever -- correct output, wasted minutes.
$(NDS_BATTLE_STATIC_TEXTURE_INC) $(NDS_BATTLE_STATIC_TEXTURE_ASSET) &: \
		$(PROJECT_ROOT)/scripts/generate_battle_playable_static_textures.py \
		$(PROJECT_ROOT)/scripts/generate_battle_playable_texture_census.py
	python "$(PROJECT_ROOT)/scripts/generate_battle_playable_static_textures.py" --repo-root "$(PROJECT_ROOT)"
	@touch $(NDS_BATTLE_STATIC_TEXTURE_INC) $(NDS_BATTLE_STATIC_TEXTURE_ASSET)

# P2-4n1. The native stage packets, one include per baked stage. Same shape
# and the same reason as the block above: until 2026-09-04 only build.ps1 ran
# this generator, so an incremental `make` linked whatever include was on disk
# and a fresh clone had none at all (both are gitignored). The generator fails
# closed on a descriptor pin mismatch (generate_nds_native_stage.py:4534), so
# a stale descriptor stops the build here instead of reaching a ROM. Dream
# Land is the generator's default stage. Yoster's include is generated
# unconditionally so it cannot go stale behind NDS_STAGE_YOSTER; the
# translation unit includes it only when that flag is 1.
NDS_NATIVE_STAGE_OWNER_INC := $(PROJECT_ROOT)/src/nds/nds_native_stage_owner.generated.inc
NDS_NATIVE_STAGE_YOSTER_INC := $(PROJECT_ROOT)/src/nds/nds_native_stage_yoster.generated.inc
NDS_NATIVE_STAGE_JUNGLE_INC := $(PROJECT_ROOT)/src/nds/nds_native_stage_jungle.generated.inc
NDS_NATIVE_STAGE_CASTLE_INC := $(PROJECT_ROOT)/src/nds/nds_native_stage_castle.generated.inc
NDS_NATIVE_STAGE_SECTOR_INC := $(PROJECT_ROOT)/src/nds/nds_native_stage_sector.generated.inc
NDS_NATIVE_STAGE_HYRULE_INC := $(PROJECT_ROOT)/src/nds/nds_native_stage_hyrule.generated.inc
NDS_NATIVE_STAGE_INISHIE_INC := $(PROJECT_ROOT)/src/nds/nds_native_stage_inishie.generated.inc
NDS_NATIVE_STAGE_ZEBES_INC := $(PROJECT_ROOT)/src/nds/nds_native_stage_zebes.generated.inc
NDS_NATIVE_STAGE_YAMABUKI_INC := $(PROJECT_ROOT)/src/nds/nds_native_stage_yamabuki.generated.inc
NDS_NATIVE_STAGE_PUPUPUSMALL_INC := $(PROJECT_ROOT)/src/nds/nds_native_stage_pupupusmall.generated.inc
NDS_NATIVE_STAGE_YOSTERSMALL_INC := $(PROJECT_ROOT)/src/nds/nds_native_stage_yostersmall.generated.inc
NDS_NATIVE_STAGE_METAL_INC := $(PROJECT_ROOT)/src/nds/nds_native_stage_metal.generated.inc
NDS_NATIVE_STAGE_ZAKO_INC := $(PROJECT_ROOT)/src/nds/nds_native_stage_zako.generated.inc
NDS_NATIVE_STAGE_LAST_INC := $(PROJECT_ROOT)/src/nds/nds_native_stage_last.generated.inc
NDS_NATIVE_STAGE_BONUS3_INC := $(PROJECT_ROOT)/src/nds/nds_native_stage_bonus3.generated.inc
NDS_NATIVE_STAGE_BONUS1_MARIO_INC := $(PROJECT_ROOT)/src/nds/nds_native_stage_bonus1_mario.generated.inc
NDS_NATIVE_STAGE_BONUS1_FOX_INC := $(PROJECT_ROOT)/src/nds/nds_native_stage_bonus1_fox.generated.inc
NDS_NATIVE_STAGE_BONUS1_DONKEY_INC := $(PROJECT_ROOT)/src/nds/nds_native_stage_bonus1_donkey.generated.inc
NDS_NATIVE_STAGE_BONUS1_SAMUS_INC := $(PROJECT_ROOT)/src/nds/nds_native_stage_bonus1_samus.generated.inc
NDS_NATIVE_STAGE_BONUS1_LUIGI_INC := $(PROJECT_ROOT)/src/nds/nds_native_stage_bonus1_luigi.generated.inc
NDS_NATIVE_STAGE_BONUS1_LINK_INC := $(PROJECT_ROOT)/src/nds/nds_native_stage_bonus1_link.generated.inc
NDS_NATIVE_STAGE_BONUS1_YOSHI_INC := $(PROJECT_ROOT)/src/nds/nds_native_stage_bonus1_yoshi.generated.inc
NDS_NATIVE_STAGE_BONUS1_CAPTAIN_INC := $(PROJECT_ROOT)/src/nds/nds_native_stage_bonus1_captain.generated.inc
NDS_NATIVE_STAGE_BONUS1_KIRBY_INC := $(PROJECT_ROOT)/src/nds/nds_native_stage_bonus1_kirby.generated.inc
NDS_NATIVE_STAGE_BONUS1_PIKACHU_INC := $(PROJECT_ROOT)/src/nds/nds_native_stage_bonus1_pikachu.generated.inc
NDS_NATIVE_STAGE_BONUS1_PURIN_INC := $(PROJECT_ROOT)/src/nds/nds_native_stage_bonus1_purin.generated.inc
NDS_NATIVE_STAGE_BONUS1_NESS_INC := $(PROJECT_ROOT)/src/nds/nds_native_stage_bonus1_ness.generated.inc
NDS_NATIVE_STAGE_BONUS2_MARIO_INC := $(PROJECT_ROOT)/src/nds/nds_native_stage_bonus2_mario.generated.inc
NDS_NATIVE_STAGE_BONUS2_FOX_INC := $(PROJECT_ROOT)/src/nds/nds_native_stage_bonus2_fox.generated.inc
NDS_NATIVE_STAGE_BONUS2_DONKEY_INC := $(PROJECT_ROOT)/src/nds/nds_native_stage_bonus2_donkey.generated.inc
NDS_NATIVE_STAGE_BONUS2_SAMUS_INC := $(PROJECT_ROOT)/src/nds/nds_native_stage_bonus2_samus.generated.inc
NDS_NATIVE_STAGE_BONUS2_LUIGI_INC := $(PROJECT_ROOT)/src/nds/nds_native_stage_bonus2_luigi.generated.inc
NDS_NATIVE_STAGE_BONUS2_LINK_INC := $(PROJECT_ROOT)/src/nds/nds_native_stage_bonus2_link.generated.inc
NDS_NATIVE_STAGE_BONUS2_YOSHI_INC := $(PROJECT_ROOT)/src/nds/nds_native_stage_bonus2_yoshi.generated.inc
NDS_NATIVE_STAGE_BONUS2_CAPTAIN_INC := $(PROJECT_ROOT)/src/nds/nds_native_stage_bonus2_captain.generated.inc
NDS_NATIVE_STAGE_BONUS2_KIRBY_INC := $(PROJECT_ROOT)/src/nds/nds_native_stage_bonus2_kirby.generated.inc
NDS_NATIVE_STAGE_BONUS2_PIKACHU_INC := $(PROJECT_ROOT)/src/nds/nds_native_stage_bonus2_pikachu.generated.inc
NDS_NATIVE_STAGE_BONUS2_PURIN_INC := $(PROJECT_ROOT)/src/nds/nds_native_stage_bonus2_purin.generated.inc
NDS_NATIVE_STAGE_BONUS2_NESS_INC := $(PROJECT_ROOT)/src/nds/nds_native_stage_bonus2_ness.generated.inc
NDS_NATIVE_STAGE_GENERATOR_PREREQ := \
	$(PROJECT_ROOT)/scripts/stages/generate_nds_native_stage.py \
	$(PROJECT_ROOT)/scripts/stages/native_matrix_math.py \
	$(wildcard $(PROJECT_ROOT)/scripts/stages/native_stage_descriptors/*.py)
$(NDS_NATIVE_STAGE_OWNER_INC): $(NDS_NATIVE_STAGE_GENERATOR_PREREQ)
	python "$(PROJECT_ROOT)/scripts/stages/generate_nds_native_stage.py" --repo-root "$(PROJECT_ROOT)"
	@touch $(NDS_NATIVE_STAGE_OWNER_INC)
$(NDS_NATIVE_STAGE_YOSTER_INC): $(NDS_NATIVE_STAGE_GENERATOR_PREREQ)
	python "$(PROJECT_ROOT)/scripts/stages/generate_nds_native_stage.py" --repo-root "$(PROJECT_ROOT)" --stage yoster
	@touch $(NDS_NATIVE_STAGE_YOSTER_INC)
$(NDS_NATIVE_STAGE_JUNGLE_INC): $(NDS_NATIVE_STAGE_GENERATOR_PREREQ)
	python "$(PROJECT_ROOT)/scripts/stages/generate_nds_native_stage.py" --repo-root "$(PROJECT_ROOT)" --stage jungle
	@touch $(NDS_NATIVE_STAGE_JUNGLE_INC)
$(NDS_NATIVE_STAGE_CASTLE_INC): $(NDS_NATIVE_STAGE_GENERATOR_PREREQ)
	python "$(PROJECT_ROOT)/scripts/stages/generate_nds_native_stage.py" --repo-root "$(PROJECT_ROOT)" --stage castle
	@touch $(NDS_NATIVE_STAGE_CASTLE_INC)
$(NDS_NATIVE_STAGE_SECTOR_INC): $(NDS_NATIVE_STAGE_GENERATOR_PREREQ)
	python "$(PROJECT_ROOT)/scripts/stages/generate_nds_native_stage.py" --repo-root "$(PROJECT_ROOT)" --stage sector
	@touch $(NDS_NATIVE_STAGE_SECTOR_INC)
$(NDS_NATIVE_STAGE_HYRULE_INC): $(NDS_NATIVE_STAGE_GENERATOR_PREREQ)
	python "$(PROJECT_ROOT)/scripts/stages/generate_nds_native_stage.py" --repo-root "$(PROJECT_ROOT)" --stage hyrule
	@touch $(NDS_NATIVE_STAGE_HYRULE_INC)
$(NDS_NATIVE_STAGE_INISHIE_INC): $(NDS_NATIVE_STAGE_GENERATOR_PREREQ)
	python "$(PROJECT_ROOT)/scripts/stages/generate_nds_native_stage.py" --repo-root "$(PROJECT_ROOT)" --stage inishie
	@touch $(NDS_NATIVE_STAGE_INISHIE_INC)
$(NDS_NATIVE_STAGE_ZEBES_INC): $(NDS_NATIVE_STAGE_GENERATOR_PREREQ)
	python "$(PROJECT_ROOT)/scripts/stages/generate_nds_native_stage.py" --repo-root "$(PROJECT_ROOT)" --stage zebes
	@touch $(NDS_NATIVE_STAGE_ZEBES_INC)
$(NDS_NATIVE_STAGE_YAMABUKI_INC): $(NDS_NATIVE_STAGE_GENERATOR_PREREQ)
	python "$(PROJECT_ROOT)/scripts/stages/generate_nds_native_stage.py" --repo-root "$(PROJECT_ROOT)" --stage yamabuki
	@touch $(NDS_NATIVE_STAGE_YAMABUKI_INC)
$(NDS_NATIVE_STAGE_PUPUPUSMALL_INC): $(NDS_NATIVE_STAGE_GENERATOR_PREREQ)
	python "$(PROJECT_ROOT)/scripts/stages/generate_nds_native_stage.py" --repo-root "$(PROJECT_ROOT)" --stage pupupusmall
	@touch $(NDS_NATIVE_STAGE_PUPUPUSMALL_INC)
$(NDS_NATIVE_STAGE_YOSTERSMALL_INC): $(NDS_NATIVE_STAGE_GENERATOR_PREREQ)
	python "$(PROJECT_ROOT)/scripts/stages/generate_nds_native_stage.py" --repo-root "$(PROJECT_ROOT)" --stage yostersmall
	@touch $(NDS_NATIVE_STAGE_YOSTERSMALL_INC)
$(NDS_NATIVE_STAGE_METAL_INC): $(NDS_NATIVE_STAGE_GENERATOR_PREREQ)
	python "$(PROJECT_ROOT)/scripts/stages/generate_nds_native_stage.py" --repo-root "$(PROJECT_ROOT)" --stage metal
	@touch $(NDS_NATIVE_STAGE_METAL_INC)
$(NDS_NATIVE_STAGE_ZAKO_INC): $(NDS_NATIVE_STAGE_GENERATOR_PREREQ)
	python "$(PROJECT_ROOT)/scripts/stages/generate_nds_native_stage.py" --repo-root "$(PROJECT_ROOT)" --stage zako
	@touch $(NDS_NATIVE_STAGE_ZAKO_INC)
$(NDS_NATIVE_STAGE_LAST_INC): $(NDS_NATIVE_STAGE_GENERATOR_PREREQ)
	python "$(PROJECT_ROOT)/scripts/stages/generate_nds_native_stage.py" --repo-root "$(PROJECT_ROOT)" --stage last
	@touch $(NDS_NATIVE_STAGE_LAST_INC)
$(NDS_NATIVE_STAGE_BONUS3_INC): $(NDS_NATIVE_STAGE_GENERATOR_PREREQ)
	python "$(PROJECT_ROOT)/scripts/stages/generate_nds_native_stage.py" --repo-root "$(PROJECT_ROOT)" --stage bonus3
	@touch $(NDS_NATIVE_STAGE_BONUS3_INC)
$(NDS_NATIVE_STAGE_BONUS1_MARIO_INC): $(NDS_NATIVE_STAGE_GENERATOR_PREREQ)
	python "$(PROJECT_ROOT)/scripts/stages/generate_nds_native_stage.py" --repo-root "$(PROJECT_ROOT)" --stage bonus1_mario
	@touch $(NDS_NATIVE_STAGE_BONUS1_MARIO_INC)
$(NDS_NATIVE_STAGE_BONUS1_FOX_INC): $(NDS_NATIVE_STAGE_GENERATOR_PREREQ)
	python "$(PROJECT_ROOT)/scripts/stages/generate_nds_native_stage.py" --repo-root "$(PROJECT_ROOT)" --stage bonus1_fox
	@touch $(NDS_NATIVE_STAGE_BONUS1_FOX_INC)
$(NDS_NATIVE_STAGE_BONUS1_DONKEY_INC): $(NDS_NATIVE_STAGE_GENERATOR_PREREQ)
	python "$(PROJECT_ROOT)/scripts/stages/generate_nds_native_stage.py" --repo-root "$(PROJECT_ROOT)" --stage bonus1_donkey
	@touch $(NDS_NATIVE_STAGE_BONUS1_DONKEY_INC)
$(NDS_NATIVE_STAGE_BONUS1_SAMUS_INC): $(NDS_NATIVE_STAGE_GENERATOR_PREREQ)
	python "$(PROJECT_ROOT)/scripts/stages/generate_nds_native_stage.py" --repo-root "$(PROJECT_ROOT)" --stage bonus1_samus
	@touch $(NDS_NATIVE_STAGE_BONUS1_SAMUS_INC)
$(NDS_NATIVE_STAGE_BONUS1_LUIGI_INC): $(NDS_NATIVE_STAGE_GENERATOR_PREREQ)
	python "$(PROJECT_ROOT)/scripts/stages/generate_nds_native_stage.py" --repo-root "$(PROJECT_ROOT)" --stage bonus1_luigi
	@touch $(NDS_NATIVE_STAGE_BONUS1_LUIGI_INC)
$(NDS_NATIVE_STAGE_BONUS1_LINK_INC): $(NDS_NATIVE_STAGE_GENERATOR_PREREQ)
	python "$(PROJECT_ROOT)/scripts/stages/generate_nds_native_stage.py" --repo-root "$(PROJECT_ROOT)" --stage bonus1_link
	@touch $(NDS_NATIVE_STAGE_BONUS1_LINK_INC)
$(NDS_NATIVE_STAGE_BONUS1_YOSHI_INC): $(NDS_NATIVE_STAGE_GENERATOR_PREREQ)
	python "$(PROJECT_ROOT)/scripts/stages/generate_nds_native_stage.py" --repo-root "$(PROJECT_ROOT)" --stage bonus1_yoshi
	@touch $(NDS_NATIVE_STAGE_BONUS1_YOSHI_INC)
$(NDS_NATIVE_STAGE_BONUS1_CAPTAIN_INC): $(NDS_NATIVE_STAGE_GENERATOR_PREREQ)
	python "$(PROJECT_ROOT)/scripts/stages/generate_nds_native_stage.py" --repo-root "$(PROJECT_ROOT)" --stage bonus1_captain
	@touch $(NDS_NATIVE_STAGE_BONUS1_CAPTAIN_INC)
$(NDS_NATIVE_STAGE_BONUS1_KIRBY_INC): $(NDS_NATIVE_STAGE_GENERATOR_PREREQ)
	python "$(PROJECT_ROOT)/scripts/stages/generate_nds_native_stage.py" --repo-root "$(PROJECT_ROOT)" --stage bonus1_kirby
	@touch $(NDS_NATIVE_STAGE_BONUS1_KIRBY_INC)
$(NDS_NATIVE_STAGE_BONUS1_PIKACHU_INC): $(NDS_NATIVE_STAGE_GENERATOR_PREREQ)
	python "$(PROJECT_ROOT)/scripts/stages/generate_nds_native_stage.py" --repo-root "$(PROJECT_ROOT)" --stage bonus1_pikachu
	@touch $(NDS_NATIVE_STAGE_BONUS1_PIKACHU_INC)
$(NDS_NATIVE_STAGE_BONUS1_PURIN_INC): $(NDS_NATIVE_STAGE_GENERATOR_PREREQ)
	python "$(PROJECT_ROOT)/scripts/stages/generate_nds_native_stage.py" --repo-root "$(PROJECT_ROOT)" --stage bonus1_purin
	@touch $(NDS_NATIVE_STAGE_BONUS1_PURIN_INC)
$(NDS_NATIVE_STAGE_BONUS1_NESS_INC): $(NDS_NATIVE_STAGE_GENERATOR_PREREQ)
	python "$(PROJECT_ROOT)/scripts/stages/generate_nds_native_stage.py" --repo-root "$(PROJECT_ROOT)" --stage bonus1_ness
	@touch $(NDS_NATIVE_STAGE_BONUS1_NESS_INC)
$(NDS_NATIVE_STAGE_BONUS2_MARIO_INC): $(NDS_NATIVE_STAGE_GENERATOR_PREREQ)
	python "$(PROJECT_ROOT)/scripts/stages/generate_nds_native_stage.py" --repo-root "$(PROJECT_ROOT)" --stage bonus2_mario
	@touch $(NDS_NATIVE_STAGE_BONUS2_MARIO_INC)
$(NDS_NATIVE_STAGE_BONUS2_FOX_INC): $(NDS_NATIVE_STAGE_GENERATOR_PREREQ)
	python "$(PROJECT_ROOT)/scripts/stages/generate_nds_native_stage.py" --repo-root "$(PROJECT_ROOT)" --stage bonus2_fox
	@touch $(NDS_NATIVE_STAGE_BONUS2_FOX_INC)
$(NDS_NATIVE_STAGE_BONUS2_DONKEY_INC): $(NDS_NATIVE_STAGE_GENERATOR_PREREQ)
	python "$(PROJECT_ROOT)/scripts/stages/generate_nds_native_stage.py" --repo-root "$(PROJECT_ROOT)" --stage bonus2_donkey
	@touch $(NDS_NATIVE_STAGE_BONUS2_DONKEY_INC)
$(NDS_NATIVE_STAGE_BONUS2_SAMUS_INC): $(NDS_NATIVE_STAGE_GENERATOR_PREREQ)
	python "$(PROJECT_ROOT)/scripts/stages/generate_nds_native_stage.py" --repo-root "$(PROJECT_ROOT)" --stage bonus2_samus
	@touch $(NDS_NATIVE_STAGE_BONUS2_SAMUS_INC)
$(NDS_NATIVE_STAGE_BONUS2_LUIGI_INC): $(NDS_NATIVE_STAGE_GENERATOR_PREREQ)
	python "$(PROJECT_ROOT)/scripts/stages/generate_nds_native_stage.py" --repo-root "$(PROJECT_ROOT)" --stage bonus2_luigi
	@touch $(NDS_NATIVE_STAGE_BONUS2_LUIGI_INC)
$(NDS_NATIVE_STAGE_BONUS2_LINK_INC): $(NDS_NATIVE_STAGE_GENERATOR_PREREQ)
	python "$(PROJECT_ROOT)/scripts/stages/generate_nds_native_stage.py" --repo-root "$(PROJECT_ROOT)" --stage bonus2_link
	@touch $(NDS_NATIVE_STAGE_BONUS2_LINK_INC)
$(NDS_NATIVE_STAGE_BONUS2_YOSHI_INC): $(NDS_NATIVE_STAGE_GENERATOR_PREREQ)
	python "$(PROJECT_ROOT)/scripts/stages/generate_nds_native_stage.py" --repo-root "$(PROJECT_ROOT)" --stage bonus2_yoshi
	@touch $(NDS_NATIVE_STAGE_BONUS2_YOSHI_INC)
$(NDS_NATIVE_STAGE_BONUS2_CAPTAIN_INC): $(NDS_NATIVE_STAGE_GENERATOR_PREREQ)
	python "$(PROJECT_ROOT)/scripts/stages/generate_nds_native_stage.py" --repo-root "$(PROJECT_ROOT)" --stage bonus2_captain
	@touch $(NDS_NATIVE_STAGE_BONUS2_CAPTAIN_INC)
$(NDS_NATIVE_STAGE_BONUS2_KIRBY_INC): $(NDS_NATIVE_STAGE_GENERATOR_PREREQ)
	python "$(PROJECT_ROOT)/scripts/stages/generate_nds_native_stage.py" --repo-root "$(PROJECT_ROOT)" --stage bonus2_kirby
	@touch $(NDS_NATIVE_STAGE_BONUS2_KIRBY_INC)
$(NDS_NATIVE_STAGE_BONUS2_PIKACHU_INC): $(NDS_NATIVE_STAGE_GENERATOR_PREREQ)
	python "$(PROJECT_ROOT)/scripts/stages/generate_nds_native_stage.py" --repo-root "$(PROJECT_ROOT)" --stage bonus2_pikachu
	@touch $(NDS_NATIVE_STAGE_BONUS2_PIKACHU_INC)
$(NDS_NATIVE_STAGE_BONUS2_PURIN_INC): $(NDS_NATIVE_STAGE_GENERATOR_PREREQ)
	python "$(PROJECT_ROOT)/scripts/stages/generate_nds_native_stage.py" --repo-root "$(PROJECT_ROOT)" --stage bonus2_purin
	@touch $(NDS_NATIVE_STAGE_BONUS2_PURIN_INC)
$(NDS_NATIVE_STAGE_BONUS2_NESS_INC): $(NDS_NATIVE_STAGE_GENERATOR_PREREQ)
	python "$(PROJECT_ROOT)/scripts/stages/generate_nds_native_stage.py" --repo-root "$(PROJECT_ROOT)" --stage bonus2_ness
	@touch $(NDS_NATIVE_STAGE_BONUS2_NESS_INC)
# nds_renderer.o, not nds_renderer_assets.o: the .c is textually included by
# nds_renderer.c and no rule builds an nds_renderer_assets.o, so this line
# used to recompile nothing when a stage packet was regenerated (2026-09-07).
nds_renderer.o: $(NDS_NATIVE_STAGE_OWNER_INC) $(NDS_NATIVE_STAGE_YOSTER_INC) $(NDS_NATIVE_STAGE_JUNGLE_INC) $(NDS_NATIVE_STAGE_CASTLE_INC) $(NDS_NATIVE_STAGE_SECTOR_INC) $(NDS_NATIVE_STAGE_HYRULE_INC) $(NDS_NATIVE_STAGE_INISHIE_INC) $(NDS_NATIVE_STAGE_ZEBES_INC) $(NDS_NATIVE_STAGE_YAMABUKI_INC) $(NDS_NATIVE_STAGE_PUPUPUSMALL_INC) $(NDS_NATIVE_STAGE_YOSTERSMALL_INC) $(NDS_NATIVE_STAGE_METAL_INC) $(NDS_NATIVE_STAGE_ZAKO_INC) $(NDS_NATIVE_STAGE_LAST_INC) $(NDS_NATIVE_STAGE_BONUS3_INC) $(NDS_NATIVE_STAGE_BONUS1_MARIO_INC) $(NDS_NATIVE_STAGE_BONUS1_FOX_INC) $(NDS_NATIVE_STAGE_BONUS1_DONKEY_INC) $(NDS_NATIVE_STAGE_BONUS1_SAMUS_INC) $(NDS_NATIVE_STAGE_BONUS1_LUIGI_INC) $(NDS_NATIVE_STAGE_BONUS1_LINK_INC) $(NDS_NATIVE_STAGE_BONUS1_YOSHI_INC) $(NDS_NATIVE_STAGE_BONUS1_CAPTAIN_INC) $(NDS_NATIVE_STAGE_BONUS1_KIRBY_INC) $(NDS_NATIVE_STAGE_BONUS1_PIKACHU_INC) $(NDS_NATIVE_STAGE_BONUS1_PURIN_INC) $(NDS_NATIVE_STAGE_BONUS1_NESS_INC) $(NDS_NATIVE_STAGE_BONUS2_MARIO_INC) $(NDS_NATIVE_STAGE_BONUS2_FOX_INC) $(NDS_NATIVE_STAGE_BONUS2_DONKEY_INC) $(NDS_NATIVE_STAGE_BONUS2_SAMUS_INC) $(NDS_NATIVE_STAGE_BONUS2_LUIGI_INC) $(NDS_NATIVE_STAGE_BONUS2_LINK_INC) $(NDS_NATIVE_STAGE_BONUS2_YOSHI_INC) $(NDS_NATIVE_STAGE_BONUS2_CAPTAIN_INC) $(NDS_NATIVE_STAGE_BONUS2_KIRBY_INC) $(NDS_NATIVE_STAGE_BONUS2_PIKACHU_INC) $(NDS_NATIVE_STAGE_BONUS2_PURIN_INC) $(NDS_NATIVE_STAGE_BONUS2_NESS_INC)
# P2-4 packet residency (2026-09-05). Every stage but Dream Land is
# blob-resident: the ROM links only its registry row (NDS_NATIVE_STAGE_LINKED_
# <STAGE> defaults to 0) and its tables ship as one relocatable NitroFS blob,
# nitro:/stages/native_stage_<stage>.bin, that src/nds/nds_native_stage_blob.c
# reads into the scene arena at stage start. The blob is emitted beside the
# stage's include from the same generation: `--check` refuses a stale include
# and the include is a prerequisite, so the two cannot drift, and the blob
# recipe never rewrites an include a compile may be reading. The maxima
# header keeps the MAX-sized workspaces in nds_renderer_assets.c sized over
# every registered packet while none but Dream Land is linked (select.inc
# folds it into NDS_NATIVE_STAGE_MAX_*). Blobs are flag-independent but live
# under $(BUILD) like the rest of the NitroFS inventory.
NDS_NATIVE_STAGE_BLOB_DIR := $(PROJECT_ROOT)/$(BUILD)/stages
NDS_NATIVE_STAGE_BLOB_STAGES := yoster jungle castle sector hyrule inishie zebes \
	yamabuki pupupusmall yostersmall metal zako last bonus3 \
	bonus1_mario bonus1_fox bonus1_donkey bonus1_samus bonus1_luigi bonus1_link bonus1_yoshi \
	bonus1_captain bonus1_kirby bonus1_pikachu bonus1_purin bonus1_ness bonus2_mario bonus2_fox \
	bonus2_donkey bonus2_samus bonus2_luigi bonus2_link bonus2_yoshi bonus2_captain bonus2_kirby \
	bonus2_pikachu bonus2_purin bonus2_ness
NDS_NATIVE_STAGE_BLOBS := $(foreach stage,$(NDS_NATIVE_STAGE_BLOB_STAGES),$(NDS_NATIVE_STAGE_BLOB_DIR)/native_stage_$(stage).bin)
# These verified blobs are build products, not disposable copy intermediates.
.SECONDARY: $(NDS_NATIVE_STAGE_BLOBS)
export NDS_NITROFS_NATIVE_STAGE_BLOB_FILES := $(foreach stage,$(NDS_NATIVE_STAGE_BLOB_STAGES),$(NITROFS_DIR)/stages/native_stage_$(stage).bin)
$(NDS_NATIVE_STAGE_BLOB_DIR)/native_stage_%.bin: $(PROJECT_ROOT)/src/nds/nds_native_stage_%.generated.inc $(NDS_NATIVE_STAGE_GENERATOR_PREREQ)
	@mkdir -p $(dir $@)
	python "$(PROJECT_ROOT)/scripts/stages/generate_nds_native_stage.py" --repo-root "$(PROJECT_ROOT)" --stage $* --check --emit-blob --blob-output "$@"
$(NITROFS_DIR)/stages/native_stage_%.bin: $(NDS_NATIVE_STAGE_BLOB_DIR)/native_stage_%.bin
	@mkdir -p $(dir $@)
	@cp $< $@
# The VS stages' dead geometry bytes (display lists and vertex arrays their
# packets were generated from), lent to a battle once the packet loads
# (reloc_backend_compat_shims.c ndsNativeStageLendDeadSpans).
NDS_STAGE_DEAD_SPANS_H := $(NDS_NATIVE_STAGE_BLOB_DIR)/nds_stage_dead_spans.generated.h
$(NDS_STAGE_DEAD_SPANS_H): $(PROJECT_ROOT)/scripts/stages/generate_stage_dead_spans.py $(NDS_NATIVE_STAGE_GENERATOR_PREREQ)
	@mkdir -p $(dir $@)
	python "$(PROJECT_ROOT)/scripts/stages/generate_stage_dead_spans.py" --output "$@"
scene_backend.o: $(NDS_STAGE_DEAD_SPANS_H)
CFLAGS += -I$(NDS_NATIVE_STAGE_BLOB_DIR)

# Phase 2 GX templates live on NitroFS; no duplicated template slab in main RAM.
NDS_STAGE_GX_STAGES := castle sector jungle zebes hyrule yoster dreamland yamabuki inishie
# P2-6: the 1P campaign's venues take the same templates (sNdsStageGxPaths).
ifeq ($(NDS_1P_GAME),1)
NDS_STAGE_GX_STAGES += pupupusmall yostersmall metal zako last bonus3 \
	$(foreach f,mario fox donkey samus luigi link yoshi captain kirby pikachu purin ness,bonus1_$(f) bonus2_$(f))
endif
NDS_STAGE_GX_FILES := $(foreach stage,$(NDS_STAGE_GX_STAGES),$(NITROFS_DIR)/stages/$(stage).gxp)
$(NITROFS_DIR)/stages/%.gxp: $(NDS_NATIVE_STAGE_GENERATOR_PREREQ) $(PROJECT_ROOT)/scripts/stages/compile_nds_stage_gx.py
	@mkdir -p $(dir $@)
	python "$(PROJECT_ROOT)/scripts/stages/compile_nds_stage_gx.py" --stage $* --output "$@"
$(OUTPUT).nds: $(NDS_STAGE_GX_FILES)
# Under src/nds/generated/ because that whole directory is gitignored (.gitignore
# line 39); at src/nds/ the header matched no ignore pattern (the neighbouring
# rule only covers nds_native_stage_*.generated.inc) and would have shown up
# untracked after the first build. 2026-09-05.
NDS_NATIVE_STAGE_BLOB_MAXIMA_INC := $(PROJECT_ROOT)/src/nds/generated/nds_native_stage_blob_maxima.generated.h
$(NDS_NATIVE_STAGE_BLOB_MAXIMA_INC): $(NDS_NATIVE_STAGE_OWNER_INC) $(foreach stage,$(NDS_NATIVE_STAGE_BLOB_STAGES),$(PROJECT_ROOT)/src/nds/nds_native_stage_$(stage).generated.inc) $(PROJECT_ROOT)/scripts/stages/emit_native_stage_runtime_rows.py
	@mkdir -p $(dir $@)
	python "$(PROJECT_ROOT)/scripts/stages/emit_native_stage_runtime_rows.py" --stage dreamland --maxima > "$@.tmp" && mv -f "$@.tmp" "$@"
# nds_renderer_assets.c is textually included by nds_renderer.c (line 2), so
# the object that consumes this header is nds_renderer.o. Until 2026-09-07 the
# dependency named nds_renderer_assets.o, an object no rule ever builds, so the
# header was never generated, __has_include silently took the "no blob stages"
# arm, and every blob stage with more dense vertices than Dream Land overflowed
# the validator's stack mask and was rejected with reason 6.
nds_renderer.o: $(NDS_NATIVE_STAGE_BLOB_MAXIMA_INC)
$(OUTPUT).elf: $(NDS_NATIVE_STAGE_BLOB_MAXIMA_INC)
