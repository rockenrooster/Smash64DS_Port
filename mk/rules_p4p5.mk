# The inner make: the P4 contents (export, generated data, Kirby powers,
# native owners) and the P5 stages.
# Included in place from the Makefile (P8 G); its text is unchanged.

# P4 contents (docs/P4/P4_STATUS.md). The export and the generated data derive
# from the user's ROM, so they live under the ignored build tree. Each
# registry content's export -- $(1) name, $(3) Remix name, $(5) base -- for
# every row, so an authored content's base exports when only the authored one
# is enabled: remix_export.py's for a Remix fighter; for a P6 authored content,
# scripts/p6/p6_author.py's from its base's export and its spec.
define NDS_P4_EXPORT_RULE
ifeq ($(5),-)
$$(NDS_P4_EXPORT)/$(1)/resolved.json: $$(NDS_P4_STAGING)/ssb64asm.z64 \
		$$(PROJECT_ROOT)/scripts/p4/remix_export.py $$(PROJECT_ROOT)/scripts/p4/remix_rom.py
	python "$$(PROJECT_ROOT)/scripts/p4/remix_export.py" --staging "$$(NDS_P4_STAGING)" \
		--fighter $(3) --out "$$(NDS_P4_EXPORT)/$(1)" --no-o2r
else
# Its clips: each recipe in art/p6/<name>/anims built by anim.py from the
# staged ROM (ROM-derived, so under builds/); its motion scripts are
# committed text the author assembles.
$$(PROJECT_ROOT)/builds/p6-art/$(1)/anims/%.o2r: $$(PROJECT_ROOT)/art/p6/$(1)/anims/%.json \
		$$(NDS_P4_STAGING)/ssb64asm.z64 $$(PROJECT_ROOT)/scripts/p6/anim.py $$(PROJECT_ROOT)/scripts/p6/pose_ops.py $$(PROJECT_ROOT)/scripts/p6/keyposes.py \
		$$(PROJECT_ROOT)/scripts/p6/ultimate.py \
		$$(wildcard $$(PROJECT_ROOT)/assets/p6-art/$(1)/3ds2ds/scene_high/scene.json)
	python "$$(PROJECT_ROOT)/scripts/p6/anim.py" --build "$$<" --staging "$$(NDS_P4_STAGING)" --out "$$@"
$$(NDS_P4_EXPORT)/$(1)/resolved.json: $$(NDS_P4_EXPORT)/$(5)/resolved.json \
		$$(NDS_P4_STAGING)/ssb64asm.z64 \
		$$(PROJECT_ROOT)/scripts/p6/p6_author.py $$(PROJECT_ROOT)/scripts/p6/mscript.py \
		$$(PROJECT_ROOT)/scripts/p6/coverage.py $$(PROJECT_ROOT)/scripts/p6/anim.py \
		$$(PROJECT_ROOT)/scripts/p6/skirt_keys.py $$(PROJECT_ROOT)/scripts/p6/dress_check.py \
		$$(PROJECT_ROOT)/scripts/p6/pose_ops.py \
		$$(wildcard $$(PROJECT_ROOT)/assets/p6-art/$(1)/3ds2ds/scene_high/scene.json) \
		$$(wildcard $$(PROJECT_ROOT)/scripts/p6/$(1)/spec.json) \
		$$(PROJECT_ROOT)/scripts/p6/p6_audio.py $$(wildcard $$(PROJECT_ROOT)/scripts/p6/$(1)/audio.json) \
		$$(wildcard $$(PROJECT_ROOT)/scripts/p6/$(1)/motions/*.ms) \
		$$(wildcard $$(PROJECT_ROOT)/assets/p6-art/$(1)/look/*) $$(wildcard $$(PROJECT_ROOT)/assets/p6-art/$(1)/gate/*) \
		$$(patsubst $$(PROJECT_ROOT)/art/p6/$(1)/anims/%.json,$$(PROJECT_ROOT)/builds/p6-art/$(1)/anims/%.o2r,$$(wildcard $$(PROJECT_ROOT)/art/p6/$(1)/anims/*.json))
	python "$$(PROJECT_ROOT)/scripts/p6/p6_author.py" --base-export "$$(NDS_P4_EXPORT)/$(5)" \
		--staging "$$(NDS_P4_STAGING)" --content $(1) --out "$$(NDS_P4_EXPORT)/$(1)"
endif
endef
# One rule set per enabled content -- $(1) name, $(2) NAME, $(3) token, $(4)
# id, $(5) preview kind (NDS_P4_SEL_BASE + id) -- from scripts/p4/contents.json:
# the generated data, the character-select preview pack and the two native
# images (NitroFS payloads like every P2 owner's). The O2R containers are
# staged into NitroFS for all contents at once (below).
define NDS_P4_CONTENT_RULES
$$(NDS_P4_GEN)/$(1)/nds_p4_$(1).generated.c: $$(NDS_P4_EXPORT)/$(1)/resolved.json \
		$$(PROJECT_ROOT)/scripts/p4/generate_p4_fighter.py $$(PROJECT_ROOT)/scripts/p4/remix_rom.py \
		$$(PROJECT_ROOT)/scripts/p4/ft_layout.py $$(PROJECT_ROOT)/scripts/menus/generate_battle_hud.py \
		$$(PROJECT_ROOT)/scripts/p4/contents.json $$(NDS_P4_LAB_FALLBACK_STAMP) \
		$$(PROJECT_ROOT)/scripts/p4/p4_shield_pose.py $$(PROJECT_ROOT)/scripts/p4/p4_articles.py \
		$$(PROJECT_ROOT)/scripts/p4/p4_anim_compact.py \
		$$(PROJECT_ROOT)/scripts/fighters/generate_nds_shield_pose_pack.py
	python "$$(PROJECT_ROOT)/scripts/p4/generate_p4_fighter.py" --staging "$$(NDS_P4_STAGING)" \
		--export "$$(NDS_P4_EXPORT)/$(1)" --out "$$(NDS_P4_GEN)/$(1)" \
		$$(if $$(filter 1,$$(NDS_P4_LAB_FALLBACK)),--lab-fallback)
$$(NDS_P4_GEN)/$(1)/preview.fpc: $$(NDS_P4_GEN)/$(1)/nds_p4_$(1).generated.c \
		$$(PROJECT_ROOT)/scripts/p4/p4_preview_pack.py $$(PROJECT_ROOT)/scripts/p4/p4_native_owner.py \
		$$(PROJECT_ROOT)/scripts/p4/p4_contents.py $$(PROJECT_ROOT)/scripts/p4/contents.json \
		$$(PROJECT_ROOT)/scripts/p4/ft_layout.py $$(PROJECT_ROOT)/include/nds/nds_preview_pack.h \
		$$(PROJECT_ROOT)/scripts/p4/owners/$(1).json
	python "$$(PROJECT_ROOT)/scripts/p4/p4_preview_pack.py" --o2r "$$(NDS_P4_GEN)/$(1)/o2r" \
		--content $(1) --export-root "$$(NDS_P4_EXPORT)" --out "$$@" \
		--battle-out "$$(NDS_P4_GEN)/$(1)/battle.fpc"
# Written with the pack: the Main pointers a match restores (BEX1 rows), and
# the low-detail pack a battle of three or four fighters loads.
$$(NDS_P4_GEN)/$(1)/preview.ext: $$(NDS_P4_GEN)/$(1)/preview.fpc ; @:
$$(NDS_P4_GEN)/$(1)/battle.fpc: $$(NDS_P4_GEN)/$(1)/preview.fpc ; @:
$$(NDS_P4_GEN)/$(1)/articles.json: $$(NDS_P4_GEN)/$(1)/nds_p4_$(1).generated.c \
		$$(PROJECT_ROOT)/scripts/p4/p4_articles.py $$(PROJECT_ROOT)/scripts/p4/contents.json
	python "$$(PROJECT_ROOT)/scripts/p4/p4_articles.py" --o2r "$$(NDS_P4_GEN)/$(1)/o2r" \
		--export "$$(NDS_P4_EXPORT)/$(1)" --content $(1) --out "$$@"
$$(NITROFS_DIR)/fighters/preview/$(5).fpc: $$(NDS_P4_GEN)/$(1)/preview.fpc
	@mkdir -p $$(dir $$@)
	@cp $$< $$@
$$(NITROFS_DIR)/fighters/preview/$(5).ext: $$(NDS_P4_GEN)/$(1)/preview.ext
	@mkdir -p $$(dir $$@)
	@cp $$< $$@
$$(NITROFS_DIR)/fighters/battle/$(5).fpc: $$(NDS_P4_GEN)/$(1)/battle.fpc
	@mkdir -p $$(dir $$@)
	@cp $$< $$@
# The native guard-pose package (empty when the content has none of its own),
# written with the generated source (scripts/p4/p4_shield_pose.py).
$$(NDS_P4_GEN)/$(1)/shield_pose.bin: $$(NDS_P4_GEN)/$(1)/nds_p4_$(1).generated.c ; @:
$$(NITROFS_DIR)/fighters/shield_pose/$(5).bin: $$(NDS_P4_GEN)/$(1)/shield_pose.bin
	@mkdir -p $$(dir $$@)
	@cp $$< $$@
# The content's match tables (motion descriptors, CPU rows): compiled alone and
# lifted from .p4_tables into NitroFS, read into a scene's heap only for the
# contents it makes fighters of (nds_p4.h NDSP4TablesHeader).
$$(BUILD)/p4_tables_$(1).o: $$(NDS_P4_GEN)/$(1)/nds_p4_$(1).generated.c \
		$$(NDS_BUILD_CONFIG) $$(NDS_P4_CONTENTS_HEADER) $$(PROJECT_ROOT)/include/nds/nds_p4.h
	@mkdir -p $$(dir $$@)
	$$(CC) -c $$(CFLAGS) -I $$(PROJECT_ROOT)/include -I $$(BUILD) -I $$(NDS_P4_GEN) \
		-o $$@ $$(NDS_P4_GEN)/$(1)/nds_p4_$(1).tables.c
$$(NITROFS_DIR)/p4/$(1).tab: $$(BUILD)/p4_tables_$(1).o
	@mkdir -p $$(dir $$@)
	$$(OBJCOPY) -O binary --only-section=.p4_tables $$< $$@
$$(BUILD)/native_image_$(1)_%.o: $$(NDS_P4_NATIVE)/.stamp $$(NDS_BUILD_CONFIG)
	@mkdir -p $$(dir $$@)
	$$(CC) -c $$(CFLAGS) -I $$(PROJECT_ROOT)/include -I $$(BUILD) -I $$(NDS_P4_NATIVE) \
		-o $$@ $$(NDS_P4_NATIVE)/nds_native_fighter_$(1)_$$*.image.c
$$(NITROFS_DIR)/fighters/$(1)_%.bin: $$(BUILD)/native_image_$(1)_%.o
	@mkdir -p $$(dir $$@)
	$$(OBJCOPY) -O binary --only-section=.fighter_image $$< $$@
endef
ifeq ($(NDS_P4),1)
# The generated data follow NDS_P4_LAB_FALLBACK: this file holds its value and
# is rewritten only when it changes.
NDS_P4_LAB_FALLBACK_STAMP := $(NDS_P4_GEN)/.lab-fallback
$(NDS_P4_LAB_FALLBACK_STAMP): FORCE
	@mkdir -p $(dir $@); v='$(NDS_P4_LAB_FALLBACK)'; \
	[ "$$(cat $@ 2>/dev/null)" = "$$v" ] || echo "$$v" > $@
$(foreach row,$(NDS_P4_REGISTRY),$(eval $(call NDS_P4_EXPORT_RULE,$(call nds_p4_field,$(row),1),$(call nds_p4_field,$(row),2),$(call nds_p4_field,$(row),3),$(call nds_p4_field,$(row),4),$(call nds_p4_field,$(row),5))))
# A clip whose frames ease into another recipe's (ultimate.py "blend_into")
# is rebuilt with that recipe: Palutena's dash with her run.
$(PROJECT_ROOT)/builds/p6-art/palutena/anims/dash_run.o2r: $(PROJECT_ROOT)/art/p6/palutena/anims/run.json
$(foreach row,$(NDS_P4_ENABLED_ROWS),$(eval $(call NDS_P4_CONTENT_RULES,$(call nds_p4_field,$(row),1),$(call nds_p4_field,$(row),2),$(call nds_p4_field,$(row),3),$(call nds_p4_field,$(row),4),$(shell echo $$((64 + $(call nds_p4_field,$(row),4)))))))
# Every content's O2R containers, staged into NitroFS beside the vanilla reloc
# files in one recipe: contents share files (Marth's and Roy's), and two
# parallel copies of one file race. A file two contents ship must match.
$(NDS_P4_GEN)/.nitrofs-staged: \
		$(foreach name,$(NDS_P4_ENABLED),$(NDS_P4_GEN)/$(name)/nds_p4_$(name).generated.c)
	@mkdir -p $(NITROFS_DIR)/reloc/p4
	@seen=""; for name in $(NDS_P4_ENABLED); do \
		for f in $(NDS_P4_GEN)/$$name/o2r/*; do \
			id=$${f##*/}; t=$(NITROFS_DIR)/reloc/p4/$$id; \
			case " $$seen " in \
			*" $$id "*) cmp -s "$$f" "$$t" || { echo "P4 file $$id differs between contents"; exit 1; } ;; \
			*) cp -f "$$f" "$$t" || exit 1; seen="$$seen $$id" ;; \
			esac; \
		done; \
	done
	@touch $@
# The content list and the data aggregate (scripts/p4/p4_contents.py),
# rewritten only when they change: every TU sees the list through nds_p4.h.
$(NDS_P4_CONTENTS_HEADER) $(NDS_P4_GEN)/nds_p4_contents_data.generated.inc &: \
		$(PROJECT_ROOT)/scripts/p4/p4_contents.py $(PROJECT_ROOT)/scripts/p4/contents.json \
		$(NDS_P4_EXPORT_FILES)
	python "$(PROJECT_ROOT)/scripts/p4/p4_contents.py" --contents "$(NDS_P4_ENABLED)" \
		--export-root "$(NDS_P4_EXPORT)" --out "$(NDS_P4_GEN)"
$(OFILES): $(NDS_P4_CONTENTS_HEADER)
nds_p4_data.o: $(NDS_P4_GEN)/nds_p4_contents_data.generated.inc \
	$(foreach name,$(NDS_P4_ENABLED),$(NDS_P4_GEN)/$(name)/nds_p4_$(name).generated.c)
# Every enabled content's native owner in one run: the shared image header,
# each owner's runtime program and its two image TUs.
$(NDS_P4_NATIVE)/.stamp: \
		$(foreach name,$(NDS_P4_ENABLED),$(NDS_P4_GEN)/$(name)/nds_p4_$(name).generated.c \
			$(PROJECT_ROOT)/scripts/p4/owners/$(name).json) \
		$(PROJECT_ROOT)/scripts/p4/p4_native_owner.py $(PROJECT_ROOT)/scripts/p4/p4_contents.py \
		$(PROJECT_ROOT)/scripts/p4/contents.json \
		$(PROJECT_ROOT)/scripts/fighters/generate_nds_native_owners.py \
		$(PROJECT_ROOT)/scripts/fighters/generate_nds_native_owner_images.py
	python "$(PROJECT_ROOT)/scripts/p4/p4_native_owner.py" --export-root "$(NDS_P4_EXPORT)" \
		--contents "$(NDS_P4_ENABLED)" --gen-root "$(NDS_P4_GEN)" --emit "$(NDS_P4_NATIVE)"
	@touch $@
nds_renderer.o battleship_ftmanager.o battleship_mnplayersvs.o: $(NDS_P4_NATIVE)/.stamp
# P4 S6: the entry-effect packet with every enabled content's article roots
# appended (scripts/p4/p4_articles.py); nds_renderer_assets.c includes it in
# place of the tracked one.
NDS_P4_ENTRY_EFFECT_INC := $(NDS_P4_GEN)/nds_p4_entry_effects.generated.inc
$(NDS_P4_ENTRY_EFFECT_INC): $(NDS_ENTRY_EFFECT_INC) \
		$(foreach name,$(NDS_P4_ENABLED),$(NDS_P4_GEN)/$(name)/articles.json)
	python "$(PROJECT_ROOT)/scripts/3d_vfx/generate_nds_entry_effects.py" --p4 \
		$(foreach name,$(NDS_P4_ENABLED),"$(NDS_P4_GEN)/$(name)/articles.json") \
		$(NDS_P5_ARTICLES_ARGS) --out "$@"
nds_renderer.o: $(NDS_P4_ENTRY_EFFECT_INC)
# Further P4 generated-data rules (a fragment, so adding one does not rebuild
# every object the way a Makefile edit does).
include $(PROJECT_ROOT)/scripts/p4/p4_rules.mk
endif
# P4 character select: Remix's grid, its donor portrait/name/emblem files for
# the UI kit bake, and the runtime tables nds_menu_shell_css.c includes.
ifeq ($(NDS_P4),1)
$(NDS_P4_CSS_DIR)/css.json $(NDS_P4_CSS_DIR)/nds_p4_css.generated.inc &: \
		$(NDS_P4_STAGING)/ssb64asm.z64 $(PROJECT_ROOT)/scripts/p4/p4_css.py \
		$(PROJECT_ROOT)/scripts/p4/remix_rom.py $(NDS_BUILD_CONFIG) \
		$(PROJECT_ROOT)/scripts/p6/p6_present.py $(PROJECT_ROOT)/scripts/p6/p6_audio.py \
		$(wildcard $(PROJECT_ROOT)/assets/p6-art/*/portrait.png) $(wildcard $(PROJECT_ROOT)/scripts/p6/*/audio.json)
	python "$(PROJECT_ROOT)/scripts/p4/p4_css.py" --staging "$(NDS_P4_STAGING)" \
		--out "$(NDS_P4_CSS_DIR)" --contents "$(NDS_P4_CONTENTS)"
nds_menu_shell.o: $(NDS_P4_CSS_DIR)/nds_p4_css.generated.inc
endif
# P5 generated-data rules (a fragment, as P4's are).
ifeq ($(NDS_P5),1)
include $(PROJECT_ROOT)/scripts/p5/p5_rules.mk
endif
ifeq ($(NDS_P4),1)
# The sound pack, its entry count, the victory songs and their BGM rows come
# from one run.
$(NDS_P4_AUDIO_DIR)/fgm_p4.bin $(NDS_P4_AUDIO_DIR)/nds_p4_bgm.generated.inc \
		$(NDS_P4_AUDIO_DIR)/nds_p4_fgm.generated.h &: \
		$(NDS_P4_STAGING)/ssb64asm.z64 $(PROJECT_ROOT)/scripts/p4/p4_audio.py \
		$(PROJECT_ROOT)/scripts/sfx/render-audio-fgm-phase-pack.py \
		$(PROJECT_ROOT)/scripts/sfx/bgm/render-audio-bgm.py \
		$(PROJECT_ROOT)/scripts/sfx/vadpcm_decode.py \
		$(PROJECT_ROOT)/scripts/p6/p6_audio.py $(wildcard $(PROJECT_ROOT)/scripts/p6/*/audio.json) \
		$(NDS_P4_EXPORT_FILES) $(PROJECT_ROOT)/assets/audio/fgm_phase_pack_ima.bin \
		$(NDS_BUILD_CONFIG)
	python "$(PROJECT_ROOT)/scripts/p4/p4_audio.py" --staging "$(NDS_P4_STAGING)" \
		$(foreach content,$(NDS_P4_CONTENTS),--content $(content)) --out "$(NDS_P4_AUDIO_DIR)" \
		--export-root "$(NDS_P4_EXPORT)"
$(NDS_P4_AUDIO_DIR)/.nitrofs-staged: $(NDS_P4_AUDIO_DIR)/fgm_p4.bin
	@mkdir -p $(NITROFS_DIR)/p4/audio
	@cp $(NDS_P4_AUDIO_DIR)/*.bin $(NITROFS_DIR)/p4/audio/
	@touch $@
nds_audio_bgm.o: $(NDS_P4_AUDIO_DIR)/nds_p4_bgm.generated.inc
nds_audio_fgm.o: $(NDS_P4_AUDIO_DIR)/nds_p4_fgm.generated.h
endif
$(NDS_BATTLESHIP_IMPORT_OVERLAY_OFILES): $(NDS_BATTLESHIP_IMPORT_OVERLAY_STAMP)


$(NITROFS_DIR)/reloc/%: $(BATTLESHIP_O2R)/%
	@mkdir -p $(dir $@)
	@cp $< $@

# P2-2p8 T1: the nine VS ground maps are the one reloc group whose staged bytes
# follow a flag. Flag 1 stages the compact map (wallpaper Sprite/Bitmap stub
# appended, the 158,928-byte container dropped from its extern tree); flag 0 is
# the plain copy the generic rule above makes, byte for byte. An explicit
# static-pattern rule overrides the generic one for these nine only. The
# containers themselves stay staged by their own lists: the opening movies load
# them directly, and each map's generation reads its container. The stamp is
# per BUILD and OUTSIDE nitrofs/ (the ROM packs that whole directory), and only
# changes when the flag does, so a flip re-stages the maps and nothing else.
NDS_COMPACT_GROUND_MAP_FILES := \
	reloc_stages/GRCastleMap reloc_stages/GRSectorMap \
	reloc_stages/GRJungleMap reloc_stages/GRZebesMap \
	reloc_stages/GRHyruleMap reloc_stages/GRYosterMap \
	reloc_stages/GRPupupuMap reloc_stages/GRYamabukiMap \
	reloc_stages/GRInishieMap
# The 1P arenas and boards follow the same flag (2026-10-01): each still
# loaded a 158,928-byte wallpaper container into every 1P battle.
ifeq ($(NDS_1P_GAME),1)
NDS_COMPACT_GROUND_MAP_FILES += \
	reloc_stages/GRYosterSmallMap reloc_stages/GRMetalMap \
	reloc_stages/GRZakoMap reloc_stages/GRLastMap \
	reloc_stages/GRBonus1MarioMap reloc_stages/GRBonus1FoxMap \
	reloc_stages/GRBonus1DonkeyMap reloc_stages/GRBonus1SamusMap \
	reloc_stages/GRBonus1LuigiMap reloc_stages/GRBonus1LinkMap \
	reloc_stages/GRBonus1YoshiMap reloc_stages/GRBonus1CaptainMap \
	reloc_stages/GRBonus1KirbyMap reloc_stages/GRBonus1PikachuMap \
	reloc_stages/GRBonus1PurinMap reloc_stages/GRBonus1NessMap \
	reloc_stages/GRBonus2MarioMap reloc_stages/GRBonus2FoxMap \
	reloc_stages/GRBonus2DonkeyMap reloc_stages/GRBonus2SamusMap \
	reloc_stages/GRBonus2LuigiMap reloc_stages/GRBonus2LinkMap \
	reloc_stages/GRBonus2YoshiMap reloc_stages/GRBonus2CaptainMap \
	reloc_stages/GRBonus2KirbyMap reloc_stages/GRBonus2PikachuMap \
	reloc_stages/GRBonus2PurinMap reloc_stages/GRBonus2NessMap
endif
