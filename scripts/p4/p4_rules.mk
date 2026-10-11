# P4 generated-data rules beside the Makefile's NDS_P4_CONTENT_RULES. Kept in
# their own fragment: every object depends on the Makefile itself, so a rule
# added here does not recompile the whole build. Included from the Makefile
# when NDS_P4=1, after NDS_P4_GEN, NDS_P4_STAGING, NDS_P4_EXPORT and
# NDS_P4_ENABLED are set.

# Colour animations: Remix's extended GFX routines the enabled contents reach
# (scripts/p4/p4_colanims.py); reloc_backend_compat_shims.c appends them to
# dGMColScriptsDescs.
NDS_P4_COLANIM_INC := $(NDS_P4_GEN)/nds_p4_colanims.generated.inc
$(NDS_P4_COLANIM_INC): $(NDS_P4_STAGING)/ssb64asm.z64 \
		$(PROJECT_ROOT)/scripts/p4/p4_colanims.py $(PROJECT_ROOT)/scripts/p4/remix_rom.py \
		$(foreach name,$(NDS_P4_ENABLED),$(NDS_P4_EXPORT)/$(name)/resolved.json)
	python "$(PROJECT_ROOT)/scripts/p4/p4_colanims.py" --staging "$(NDS_P4_STAGING)" \
		--export-root "$(NDS_P4_EXPORT)" --contents "$(NDS_P4_ENABLED)" --out "$@"
# reloc_backend_compat_shims.c is a slice of scene_backend.o.
scene_backend.o: $(NDS_P4_COLANIM_INC)

# S8: Kirby's copies of the enabled contents (scripts/p4/p4_kirby.py): his
# statuses for their powers (the generated data and header nds_p4_data.c,
# nds_p4.c and the contents' routines include), the motion rows in
# nitro:/p4/kirby.tab, and the power animations and script file beside the
# contents' files in nitro:/reloc/p4.
NDS_P4_KIRBY_GEN := $(NDS_P4_GEN)/kirby
NDS_P4_KIRBY_OUT := $(NDS_P4_KIRBY_GEN)/nds_p4_kirby.generated.c \
	$(NDS_P4_KIRBY_GEN)/nds_p4_kirby.generated.h $(NDS_P4_KIRBY_GEN)/nds_p4_kirby.tables.c
$(NDS_P4_EXPORT)/kirby/resolved.json: $(NDS_P4_STAGING)/ssb64asm.z64 \
		$(PROJECT_ROOT)/scripts/p4/remix_export.py $(PROJECT_ROOT)/scripts/p4/remix_rom.py
	python "$(PROJECT_ROOT)/scripts/p4/remix_export.py" --staging "$(NDS_P4_STAGING)" \
		--fighter KIRBY --out "$(NDS_P4_EXPORT)/kirby" --no-o2r
$(NDS_P4_KIRBY_OUT) &: $(NDS_P4_EXPORT)/kirby/resolved.json $(NDS_P4_EXPORT_FILES) \
		$(PROJECT_ROOT)/scripts/p4/p4_kirby.py $(PROJECT_ROOT)/scripts/p4/generate_p4_fighter.py \
		$(PROJECT_ROOT)/scripts/p4/p4_anim_compact.py $(PROJECT_ROOT)/scripts/p4/contents.json
	python "$(PROJECT_ROOT)/scripts/p4/p4_kirby.py" --staging "$(NDS_P4_STAGING)" \
		--kirby-export "$(NDS_P4_EXPORT)/kirby" --export-root "$(NDS_P4_EXPORT)" \
		--contents "$(NDS_P4_ENABLED)" --out "$(NDS_P4_KIRBY_GEN)"
$(BUILD)/p4_tables_kirby.o: $(NDS_P4_KIRBY_GEN)/nds_p4_kirby.tables.c \
		$(NDS_BUILD_CONFIG) $(NDS_P4_CONTENTS_HEADER) $(PROJECT_ROOT)/include/nds/nds_p4.h
	@mkdir -p $(dir $@)
	$(CC) -c $(CFLAGS) -I $(PROJECT_ROOT)/include -I $(BUILD) -I $(NDS_P4_GEN) \
		-o $@ $(NDS_P4_KIRBY_GEN)/nds_p4_kirby.tables.c
$(NITROFS_DIR)/p4/kirby.tab: $(BUILD)/p4_tables_kirby.o
	@mkdir -p $(dir $@)
	$(OBJCOPY) -O binary --only-section=.p4_tables $< $@
$(NDS_P4_GEN)/.nitrofs-kirby-staged: $(NDS_P4_KIRBY_GEN)/nds_p4_kirby.generated.c
	@mkdir -p $(NITROFS_DIR)/reloc/p4
	@for f in $(NDS_P4_KIRBY_GEN)/o2r/*; do cp -f "$$f" "$(NITROFS_DIR)/reloc/p4/$${f##*/}" || exit 1; done
	@touch $@
$(OUTPUT).nds: $(NITROFS_DIR)/p4/kirby.tab $(NDS_P4_GEN)/.nitrofs-kirby-staged
nds_p4_data.o: $(NDS_P4_KIRBY_GEN)/nds_p4_kirby.generated.c
nds_p4.o $(foreach name,$(NDS_P4_ENABLED),nds_p4_$(name).o): \
	$(NDS_P4_KIRBY_GEN)/nds_p4_kirby.generated.h

# S8: Remix's Kirby hats (scripts/p4/p4_kirby_hats.py): native copy-hat images
# built through the owner generator's own copy-hat path, their trio programs
# (nds_renderer_assets.c includes them, in nds_renderer.o) and each power's
# hat texture file (into o2r/, staged with the Kirby files above).
NDS_P4_KIRBY_HATS := $(NDS_P4_KIRBY_GEN)/hats
NDS_P4_KIRBY_HAT_STAMP := $(NDS_P4_KIRBY_HATS)/.stamp
$(NDS_P4_KIRBY_HAT_STAMP): $(NDS_P4_KIRBY_GEN)/nds_p4_kirby.generated.c \
		$(PROJECT_ROOT)/scripts/p4/p4_kirby_hats.py \
		$(PROJECT_ROOT)/scripts/fighters/generate_nds_native_owners.py \
		$(PROJECT_ROOT)/scripts/fighters/generate_nds_native_owner_images.py
	python "$(PROJECT_ROOT)/scripts/p4/p4_kirby_hats.py" --staging "$(NDS_P4_STAGING)" \
		--manifest "$(NDS_P4_KIRBY_GEN)/manifest.json" --out "$(NDS_P4_KIRBY_GEN)"
	@touch $@
# Each image TU compiled alone and lifted from .fighter_image into NitroFS.
$(NDS_P4_GEN)/.nitrofs-kirby-hats: $(NDS_P4_KIRBY_HAT_STAMP) $(NDS_BUILD_CONFIG) \
		$(PROJECT_ROOT)/include/nds/nds_native_fighter_tables.h
	@mkdir -p $(NITROFS_DIR)/fighters/p4 $(BUILD)/p4_kirby_hats
	@for f in $(NDS_P4_KIRBY_HATS)/nds_native_fighter_p4kirbyhat_*.image.c; do \
		n=$${f##*/nds_native_fighter_p4kirbyhat_}; n=$${n%.image.c}; \
		$(CC) -c $(CFLAGS) -I $(PROJECT_ROOT)/include -I $(BUILD) -I $(NDS_P4_KIRBY_HATS) \
			-o $(BUILD)/p4_kirby_hats/$$n.o $$f || exit 1; \
		$(OBJCOPY) -O binary --only-section=.fighter_image $(BUILD)/p4_kirby_hats/$$n.o \
			$(NITROFS_DIR)/fighters/p4/kirby_hat_$$n.bin || exit 1; \
	done
	@touch $@
$(NDS_P4_GEN)/.nitrofs-kirby-staged: $(NDS_P4_KIRBY_HAT_STAMP)
$(OUTPUT).nds: $(NDS_P4_GEN)/.nitrofs-kirby-hats
nds_renderer.o: $(NDS_P4_KIRBY_HAT_STAMP)
# The weapon Kirby holds with a content's power is a sidecar root of that
# content: p4_articles.py merges p4_kirby.py's row (articles_<content>.json).
$(foreach name,$(NDS_P4_ENABLED),$(NDS_P4_GEN)/$(name)/articles.json): \
	$(NDS_P4_KIRBY_GEN)/nds_p4_kirby.generated.c

# Selectable Metal Mario, the source MMario kind (scripts/p4/p4_css.py KINDS):
# his VS select preview's pack, built like a content's from his source files
# (scripts/p4/p4_source_pack.py; reloc_preview_pack.c loads it on that screen
# only).
$(NDS_P4_GEN)/mmario/13.fpc: $(PROJECT_ROOT)/scripts/p4/p4_source_pack.py \
		$(PROJECT_ROOT)/scripts/p4/p4_preview_pack.py $(PROJECT_ROOT)/scripts/p4/p4_native_owner.py \
		$(PROJECT_ROOT)/scripts/p4/ft_layout.py $(PROJECT_ROOT)/include/nds/nds_preview_pack.h \
		$(BATTLESHIP_O2R)/reloc_fighters_main/MMarioMain $(BATTLESHIP_O2R)/reloc_fighters_main/MMarioModel
	python "$(PROJECT_ROOT)/scripts/p4/p4_source_pack.py" --kind 13 \
		--work "$(NDS_P4_GEN)/mmario" --out "$@"
$(NITROFS_DIR)/fighters/preview/13.fpc: $(NDS_P4_GEN)/mmario/13.fpc
	@mkdir -p $(dir $@)
	@cp $< $@

# A content-set change rewrites nds_build_config.h (its NDS_P4_<NAME> rows):
# the aggregates over the enabled contents follow it, as the select's tables
# do. Their export prerequisites alone miss it when a build directory is
# seeded from another (build_p4play.ps1 copies with mtimes): the copied
# aggregates stay newer than a new content's export (play-1011sn kept
# NDS_P4_CONTENT_MAX_ID 16 with 18 contents).
$(NDS_P4_CONTENTS_HEADER) $(NDS_P4_COLANIM_INC) $(NDS_P4_KIRBY_OUT): $(NDS_BUILD_CONFIG)

# The later contents' routines, size-optimized as the Makefile's first
# fourteen are (resident in every scene; they run only in their specials).
nds_p4_piano.o nds_p4_metaknight.o nds_p4_mrgaw.o nds_p4_snake.o: CFLAGS += -Os
