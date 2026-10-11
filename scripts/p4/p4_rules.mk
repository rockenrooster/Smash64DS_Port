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
