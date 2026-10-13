# P5 generated-data rules (docs/P5/New_Stages.md). Included from the Makefile's
# inner pass when NDS_P5=1, after NDS_P5_GEN and NDS_P4_STAGING are set. Every
# output lands in the build tree: the select art derives from the user's ROM.

# The generator rewrites a file only when its bytes change, so a rerun leaves
# the objects and the kit bake that depend on it alone.

# T1: the registry rows every shared site reads (include/nds/nds_p5.h).
NDS_P5_STAGES_HEADER := $(NDS_P5_GEN)/nds_p5_stages.generated.h
$(NDS_P5_STAGES_HEADER): $(PROJECT_ROOT)/scripts/p5/stages.json \
		$(PROJECT_ROOT)/scripts/p5/p5_stages.py
	python "$(PROJECT_ROOT)/scripts/p5/p5_stages.py" --header "$@"
$(OFILES): $(NDS_P5_STAGES_HEADER)

# T6: the 6x3 select -- its spec for the UI kit bake (sss.json), the shell's
# tables, the 3D preview's rows, and the donor's icon and logo files.
NDS_P5_SSS_DIR := $(NDS_P5_GEN)/sss
NDS_P5_SSS_OUT := $(NDS_P5_SSS_DIR)/sss.json \
	$(NDS_P5_SSS_DIR)/nds_p5_sss.generated.inc \
	$(NDS_P5_SSS_DIR)/nds_p5_preview.generated.h
$(NDS_P5_SSS_OUT) &: $(NDS_P4_STAGING)/ssb64asm.z64 \
		$(PROJECT_ROOT)/scripts/p5/stages.json \
		$(PROJECT_ROOT)/scripts/p5/p5_stages.py \
		$(PROJECT_ROOT)/scripts/p4/remix_rom.py
	python "$(PROJECT_ROOT)/scripts/p5/p5_stages.py" --sss "$(NDS_P5_SSS_DIR)" \
		--staging "$(NDS_P4_STAGING)"
nds_menu_shell.o: $(NDS_P5_SSS_DIR)/nds_p5_sss.generated.inc
battleship_mnmaps.o: $(NDS_P5_SSS_DIR)/nds_p5_preview.generated.h
