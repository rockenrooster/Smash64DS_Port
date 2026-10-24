# The outer make's targets: all, the build directory's recursive make,
# clean, run and the reviewable fighter-production manifest.
# Included in place from the Makefile (P8 G); its text is unchanged.

.PHONY: all clean clean-generated distclean run $(BUILD) prune-obsolete-audio p2-fighter-production-manifest
.PHONY: prepare-particle-banks

# Generated-file rules live in the inner build. An absolute file goal at the
# repository root can silently be a no-op; expose a configuration-aware entry.
prepare-particle-banks:
	@mkdir -p $(BUILD)
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile PROJECT_ROOT=$(PROJECT_ROOT) BUILD=$(BUILD) BUILD_OUTPUT_ROOT=$(BUILD_OUTPUT_ROOT) NDS_OUTPUT_ROOT=$(NDS_OUTPUT_ROOT) NDS_PUBLISH_USER_ROM=$(NDS_PUBLISH_USER_ROM) prepare-particle-banks

.PHONY: arm7-service
arm7-service:
	@mkdir -p $(BUILD)
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile PROJECT_ROOT=$(PROJECT_ROOT) BUILD=$(BUILD) arm7-service

all: $(BUILD)



$(BUILD):
	@mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile PROJECT_ROOT=$(PROJECT_ROOT) BUILD=$(BUILD) BUILD_OUTPUT_ROOT=$(BUILD_OUTPUT_ROOT) NDS_OUTPUT_ROOT=$(NDS_OUTPUT_ROOT) NDS_PUBLISH_USER_ROM=$(NDS_PUBLISH_USER_ROM)

clean:
	@echo clean ...
	@rm -fr $(BUILD) $(NDS_OUTPUT_ROOT)/$(NDS_OUTPUT_BASENAME).elf $(NDS_OUTPUT_ROOT)/$(NDS_OUTPUT_BASENAME).nds $(NDS_OUTPUT_ROOT)/$(NDS_OUTPUT_BASENAME).ds.gba

clean-generated:
	@powershell -NoProfile -ExecutionPolicy Bypass -File scripts/clean-generated.ps1 -Force

distclean: clean-generated

run: $(BUILD)
	@echo "ROM ready: $(NDS_OUTPUT_ROOT)/$(NDS_OUTPUT_BASENAME).nds"

# P2-3 source inventory. This is deliberately explicit rather than an implicit
# ROM prerequisite: updating the tracked manifest is a reviewable fighter-
# production step, not a side effect of every ordinary build.
p2-fighter-production-manifest:
	@python scripts/fighters/generate_fighter_production_manifest.py --repo-root .


