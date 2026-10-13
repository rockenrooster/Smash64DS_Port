# P5 generated-data rules (docs/P5/New_Stages.md). Included from the Makefile's
# inner pass when NDS_P5=1, after NDS_P5_GEN and NDS_P4_STAGING are set. Every
# output lands in the build tree: the select art and the Remix stages derive
# from the user's ROM.

# The generators rewrite a file only when its bytes change, so a rerun leaves
# the objects and the kit bake that depend on it alone.

# T1: the registry rows every shared site reads (include/nds/nds_p5.h).
NDS_P5_STAGES_HEADER := $(NDS_P5_GEN)/nds_p5_stages.generated.h
$(NDS_P5_STAGES_HEADER): $(PROJECT_ROOT)/scripts/p5/stages.json \
		$(PROJECT_ROOT)/scripts/p5/p5_stages.py
	python "$(PROJECT_ROOT)/scripts/p5/p5_stages.py" --header "$@"
# Order-only: the objects that include a generated header track it through
# their dependency files, so a changed row rebuilds those alone.
$(OFILES): | $(NDS_P5_STAGES_HEADER)

# T3: the landed Remix stages' files and resolved headers.
NDS_P5_STAGES_DIR := $(NDS_P5_GEN)/stages
NDS_P5_EXPORT_STAMP := $(NDS_P5_STAGES_DIR)/.export
$(NDS_P5_EXPORT_STAMP): $(NDS_P4_STAGING)/ssb64asm.z64 \
		$(PROJECT_ROOT)/scripts/p5/stages.json \
		$(PROJECT_ROOT)/scripts/p5/stage_export.py \
		$(PROJECT_ROOT)/scripts/p5/p5_stages.py \
		$(PROJECT_ROOT)/scripts/p4/remix_rom.py
	python "$(PROJECT_ROOT)/scripts/p5/stage_export.py" --staging "$(NDS_P4_STAGING)" \
		--out "$(NDS_P5_STAGES_DIR)" --landed
	@touch $@

# T6: the 6x3 select -- its spec for the UI kit bake (sss.json), the shell's
# tables, the 3D preview's rows, and the donor's icon and logo files.
NDS_P5_SSS_DIR := $(NDS_P5_GEN)/sss
NDS_P5_SSS_OUT := $(NDS_P5_SSS_DIR)/sss.json \
	$(NDS_P5_SSS_DIR)/nds_p5_sss.generated.inc \
	$(NDS_P5_SSS_DIR)/nds_p5_preview.generated.h
$(NDS_P5_SSS_OUT) &: $(NDS_P4_STAGING)/ssb64asm.z64 \
		$(PROJECT_ROOT)/scripts/p5/stages.json \
		$(PROJECT_ROOT)/scripts/p5/p5_stages.py \
		$(PROJECT_ROOT)/scripts/p4/remix_rom.py \
		$(NDS_P5_EXPORT_STAMP)
	python "$(PROJECT_ROOT)/scripts/p5/p5_stages.py" --sss "$(NDS_P5_SSS_DIR)" \
		--staging "$(NDS_P4_STAGING)" --stages-root "$(NDS_P5_STAGES_DIR)"
nds_menu_shell.o: $(NDS_P5_SSS_DIR)/nds_p5_sss.generated.inc
battleship_mnmaps.o: $(NDS_P5_SSS_DIR)/nds_p5_preview.generated.h

# T4: each landed Remix stage's native packet (include, NitroFS blob, GX
# template) and converted wallpaper, through the vanilla stage generator, and
# the C rows the runtime reads for them (blob and template paths, adapter
# rows, reloc files, wallpaper rows, the maxima they raise).
NDS_P5_RUNTIME_OUT := $(NDS_P5_GEN)/nds_p5_native_stages.generated.h \
	$(NDS_P5_GEN)/nds_p5_adapter.generated.inc \
	$(NDS_P5_GEN)/nds_p5_wallpapers.generated.inc
$(NDS_P5_RUNTIME_OUT) &: $(NDS_P5_EXPORT_STAMP) \
		$(PROJECT_ROOT)/scripts/p5/stage_generate.py \
		$(wildcard $(PROJECT_ROOT)/scripts/p5/pins/*.json) \
		$(PROJECT_ROOT)/scripts/stages/generate_nds_native_stage.py \
		$(PROJECT_ROOT)/scripts/stages/compile_nds_stage_gx.py \
		$(PROJECT_ROOT)/scripts/stages/generate_native_wallpapers.py
	python "$(PROJECT_ROOT)/scripts/p5/stage_generate.py" \
		--stages-root "$(NDS_P5_STAGES_DIR)" --runtime-out "$(NDS_P5_GEN)"
$(OFILES): | $(NDS_P5_RUNTIME_OUT)

# T7: the landed Remix stages' songs, as rows after the P4 songs.
NDS_P5_AUDIO_DIR := $(NDS_P5_GEN)/audio
NDS_P5_BGM_INC := $(NDS_P5_AUDIO_DIR)/nds_p5_bgm.generated.inc
$(NDS_P5_BGM_INC): $(NDS_P5_EXPORT_STAMP) \
		$(PROJECT_ROOT)/scripts/p5/p5_audio.py \
		$(PROJECT_ROOT)/scripts/p5/render_remix_bgm.py \
		$(PROJECT_ROOT)/scripts/p5/remix_bank.py \
		$(PROJECT_ROOT)/scripts/p4/p4_audio.py \
		$(PROJECT_ROOT)/scripts/sfx/bgm/render-audio-bgm.py \
		$(PROJECT_ROOT)/scripts/sfx/vadpcm_decode.py
	python "$(PROJECT_ROOT)/scripts/p5/p5_audio.py" --staging "$(NDS_P4_STAGING)" \
		--stages-root "$(NDS_P5_STAGES_DIR)" --out "$(NDS_P5_AUDIO_DIR)"
nds_audio_bgm.o: $(NDS_P5_BGM_INC)

# NitroFS: the stages' donor files beside the contents' (reloc/p4), their
# blobs and templates, their wallpapers, and their songs.
NDS_P5_REMIX_LANDED := $(shell python "$(PROJECT_ROOT)/scripts/p5/p5_stages.py" --remix-landed)
NDS_P5_LANDED_NAMES := $(foreach r,$(NDS_P5_REMIX_LANDED),$(word 1,$(subst :, ,$(r))))
NDS_P5_LANDED_CNAMES := $(foreach r,$(NDS_P5_REMIX_LANDED),$(word 2,$(subst :, ,$(r))))
NDS_P5_NITROFS_STAMP := $(NDS_P5_GEN)/.nitrofs-staged
$(NDS_P5_NITROFS_STAMP): $(NDS_P5_RUNTIME_OUT) $(NDS_P5_BGM_INC)
	@mkdir -p $(NITROFS_DIR)/reloc/p4 $(NITROFS_DIR)/stages $(NITROFS_DIR)/wallpapers $(NITROFS_DIR)/p5/audio
	@for n in $(NDS_P5_LANDED_NAMES); do \
		for f in $(NDS_P5_STAGES_DIR)/$$n/nitro/*; do cp -f "$$f" "$(NITROFS_DIR)/reloc/p4/$${f##*/}" || exit 1; done; \
	done
	@for c in $(NDS_P5_LANDED_CNAMES); do \
		cp -f $(NDS_P5_STAGES_DIR)/*/native_stage_$$c.bin $(NITROFS_DIR)/stages/ || exit 1; \
		cp -f $(NDS_P5_STAGES_DIR)/*/$$c.gxp $(NITROFS_DIR)/stages/ || exit 1; \
		cp -f $(NDS_P5_STAGES_DIR)/*/native_wallpaper_p5_$$c.bin $(NITROFS_DIR)/wallpapers/ || exit 1; \
	done
	@for f in $(NDS_P5_AUDIO_DIR)/bgm_p5_*.bin; do if [ -e "$$f" ]; then cp -f "$$f" $(NITROFS_DIR)/p5/audio/ || exit 1; fi; done
	@touch $@
$(OUTPUT).nds: $(NDS_P5_NITROFS_STAMP)
