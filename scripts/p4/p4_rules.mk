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
