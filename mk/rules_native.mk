# The inner make: the native owner images and every generated native packet
# (fighters, items, stage actors, effects) with their generator rules.
# Included in place from the Makefile (P8 G); its text is unchanged.

ifeq ($(NDS_P2_DONKEY),1)
NDS_NATIVE_IMAGE_OWNERS += donkey
endif
ifeq ($(NDS_P2_CAPTAIN),1)
NDS_NATIVE_IMAGE_OWNERS += captain
endif
ifeq ($(NDS_P2_SAMUS),1)
NDS_NATIVE_IMAGE_OWNERS += samus
endif
ifeq ($(NDS_P2_LINK),1)
NDS_NATIVE_IMAGE_OWNERS += link
endif
ifeq ($(NDS_P2_PIKACHU),1)
NDS_NATIVE_IMAGE_OWNERS += pikachu
endif
ifeq ($(NDS_P2_YOSHI),1)
NDS_NATIVE_IMAGE_OWNERS += yoshi
endif
ifeq ($(NDS_P2_NESS),1)
NDS_NATIVE_IMAGE_OWNERS += ness
endif
ifeq ($(NDS_P2_PURIN),1)
NDS_NATIVE_IMAGE_OWNERS += purin
endif
ifeq ($(NDS_P2_KIRBY),1)
NDS_NATIVE_IMAGE_OWNERS += kirby
NDS_NATIVE_KIRBY_HAT_STEMS += $(foreach id,$(NDS_NATIVE_KIRBY_HAT_IDS),kirby_hat_$(id)_high kirby_hat_$(id)_low)
endif
ifeq ($(NDS_P2_MMARIO),1)
NDS_NATIVE_IMAGE_OWNERS += mmario
endif
ifeq ($(NDS_P2_NMARIO),1)
NDS_NATIVE_IMAGE_OWNERS += nmario
endif
ifeq ($(NDS_P2_NFOX),1)
NDS_NATIVE_IMAGE_OWNERS += nfox
endif
ifeq ($(NDS_P2_NDONKEY),1)
NDS_NATIVE_IMAGE_OWNERS += ndonkey
endif
ifeq ($(NDS_P2_NSAMUS),1)
NDS_NATIVE_IMAGE_OWNERS += nsamus
endif
ifeq ($(NDS_P2_NLINK),1)
NDS_NATIVE_IMAGE_OWNERS += nlink
endif
ifeq ($(NDS_P2_NYOSHI),1)
NDS_NATIVE_IMAGE_OWNERS += nyoshi
endif
ifeq ($(NDS_P2_NCAPTAIN),1)
NDS_NATIVE_IMAGE_OWNERS += ncaptain
endif
ifeq ($(NDS_P2_NKIRBY),1)
NDS_NATIVE_IMAGE_OWNERS += nkirby
endif
ifeq ($(NDS_P2_NPIKACHU),1)
NDS_NATIVE_IMAGE_OWNERS += npikachu
endif
ifeq ($(NDS_P2_NPURIN),1)
NDS_NATIVE_IMAGE_OWNERS += npurin
endif
ifeq ($(NDS_P2_NNESS),1)
NDS_NATIVE_IMAGE_OWNERS += nness
endif
ifeq ($(NDS_P2_1P_GAME),1)
NDS_NATIVE_IMAGE_OWNERS += boss
endif
NDS_NITROFS_NATIVE_IMAGE_FILES := $(foreach owner,$(NDS_NATIVE_IMAGE_OWNERS),	$(NDS_NATIVE_IMAGE_DIR)/$(owner)_high.bin 	$(NDS_NATIVE_IMAGE_DIR)/$(owner)_low.bin)
NDS_NITROFS_NATIVE_IMAGE_FILES += $(foreach stem,$(NDS_NATIVE_KIRBY_HAT_STEMS),$(NDS_NATIVE_IMAGE_DIR)/$(stem).bin)

# Keep intermediate image objects so an incremental ROM build can reuse them.
.SECONDARY: $(foreach owner,$(NDS_NATIVE_IMAGE_OWNERS),$(BUILD)/native_image_$(owner)_high.o $(BUILD)/native_image_$(owner)_low.o)
.SECONDARY: $(foreach stem,$(NDS_NATIVE_KIRBY_HAT_STEMS),$(BUILD)/native_image_$(stem).o)

# P2-4 native stage actors + ABI v2 fighter images: fresh-clone generation rules.
#
# GAP (2026-09-06): the four packets below live under gitignored
# src/nds/generated/ (like the stage .generated.inc set) but had NO Makefile
# rule -- only build.ps1 or a by-hand generator run produced them -- so a fresh
# clone failed compiling nds_renderer_native_owners.c (textually included by
# nds_renderer.c:6) and the image consumers below, and a source/generator edit
# never retriggered the bake. Each rule mirrors the P2-4n1 stage-packet shape:
# one grouped target per generator invocation (`&:` -- a second rule per output
# would run the generator twice under -j), write-if-changed generators plus the
# standing `touch`, and unconditional emission so outputs cannot go stale behind
# a stage flag. The ignored bulk image .c files stay ignored; this rule builds
# them, it does not force-track them.
NDS_NATIVE_ACTOR_DIR := $(PROJECT_ROOT)/src/nds/generated
NDS_NATIVE_ACTOR_YOSTER_PACKET := $(NDS_NATIVE_ACTOR_DIR)/nds_native_actor_yoster_cloud.generated.inc
NDS_NATIVE_ACTOR_TARU_PACKET := $(NDS_NATIVE_ACTOR_DIR)/nds_native_actor_tarucann.generated.inc
NDS_NATIVE_ACTOR_TARU_HEADER := $(PROJECT_ROOT)/include/nds/generated/nds_native_actor_tarucann.generated.h
NDS_NATIVE_CHARGESHOT_PACKET := $(PROJECT_ROOT)/src/nds/generated/nds_native_samus_chargeshot.generated.inc
NDS_NATIVE_CHARGESHOT_HEADER := $(PROJECT_ROOT)/include/nds/generated/nds_native_samus_chargeshot.generated.h
NDS_NATIVE_CHARGESHOT_PREREQ := \
	$(PROJECT_ROOT)/scripts/stages/generate_nds_native_samus_chargeshot.py \
	$(PROJECT_ROOT)/scripts/stages/generate_nds_native_stage.py
NDS_SHIELD_POSE_HEADER := $(PROJECT_ROOT)/include/nds/generated/nds_shield_pose_assets.generated.h
NDS_SHIELD_POSE_MANIFEST := $(PROJECT_ROOT)/docs/optimization/archive/NDS_SHIELD_POSE_ASSETS.generated.json
NDS_SHIELD_POSE_SOURCE_DIR := $(PROJECT_ROOT)/assets/fighters/shield_pose
NDS_SHIELD_POSE_ALL_IDS := 01 02 03 05 07 08 09 10 11
NDS_SHIELD_POSE_SOURCE_FILES := $(foreach id,$(NDS_SHIELD_POSE_ALL_IDS),$(NDS_SHIELD_POSE_SOURCE_DIR)/$(id).bin)
NDS_SHIELD_POSE_IDS :=
ifneq ($(filter 1,$(NDS_P2_COMPACT_BATTLE_FIGHTERS) $(NDS_P2_DONKEY) $(NDS_P2_SAMUS) $(NDS_P2_LINK) $(NDS_P2_KIRBY) $(NDS_P2_CAPTAIN) $(NDS_P2_PIKACHU) $(NDS_P2_PURIN) $(NDS_P2_NESS)),)
NDS_SHIELD_POSE_IDS += 01
endif
ifeq ($(NDS_P2_DONKEY),1)
NDS_SHIELD_POSE_IDS += 02
endif
ifeq ($(NDS_P2_SAMUS),1)
NDS_SHIELD_POSE_IDS += 03
endif
ifeq ($(NDS_P2_LINK),1)
NDS_SHIELD_POSE_IDS += 05
endif
ifeq ($(NDS_P2_CAPTAIN),1)
NDS_SHIELD_POSE_IDS += 07
endif
ifeq ($(NDS_P2_KIRBY),1)
NDS_SHIELD_POSE_IDS += 08
endif
ifeq ($(NDS_P2_PIKACHU),1)
NDS_SHIELD_POSE_IDS += 09
endif
ifeq ($(NDS_P2_PURIN),1)
NDS_SHIELD_POSE_IDS += 10
endif
ifeq ($(NDS_P2_NESS),1)
NDS_SHIELD_POSE_IDS += 11
endif
# P7 C1: Mario's and Yoshi's packages (the Polygon Team's Mario, Luigi and
# Yoshi guard with them too).
NDS_SHIELD_POSE_IDS += 00
ifeq ($(NDS_P2_YOSHI),1)
NDS_SHIELD_POSE_IDS += 06
endif
NDS_NITROFS_SHIELD_POSE_FILES := $(foreach id,$(NDS_SHIELD_POSE_IDS),$(NITROFS_DIR)/fighters/shield_pose/$(id).bin)
# P2-2p8 Phase 1 slice 2b: the fighter texture admission table, one NitroFS
# payload for all twelve kinds (the runtime reads only the kinds in a match).
# The tracked header carries its layout; `generate_nds_fighter_admission.py
# --check` fails when the header is stale against a regeneration.
NDS_FIGHTER_ADMISSION_HEADER := $(PROJECT_ROOT)/include/nds/generated/nds_fighter_admission.generated.h
NDS_NITROFS_ADMISSION_FILES := $(NITROFS_DIR)/fighters/admission.bin
NDS_FIGHTER_ADMISSION_PREREQ := \
	$(PROJECT_ROOT)/scripts/fighters/generate_nds_fighter_admission.py \
	$(PROJECT_ROOT)/scripts/fighters/estimate_fighter_pack.py \
	$(PROJECT_ROOT)/scripts/_paths.py \
	$(NDS_FIGHTER_ADMISSION_HEADER) \
	$(PROJECT_ROOT)/$(BATTLESHIP_DECOMP)/src/ft/ftparam.c \
	$(wildcard $(BATTLESHIP_RELOCDATA)/*.c $(BATTLESHIP_RELOCDATA)/*.reloc $(BATTLESHIP_RELOCDATA)/*.h)
NDS_SHIELD_POSE_PREREQ := \
	$(PROJECT_ROOT)/scripts/fighters/generate_nds_shield_pose_pack.py \
	$(PROJECT_ROOT)/scripts/fighters/generate_fighter_production_manifest.py \
	$(PROJECT_ROOT)/scripts/fighters/estimate_fighter_pack.py \
	$(PROJECT_ROOT)/scripts/fighters/fighter_production_manifest.json \
	$(PROJECT_ROOT)/decomp/BattleShip-main/decomp/src/sys/objtypes.h \
	$(PROJECT_ROOT)/$(BATTLESHIP_DECOMP)/src/ft/ftdata.c \
	$(PROJECT_ROOT)/$(BATTLESHIP_DECOMP)/src/sc/scsubsys/scsubsysdatafox.c \
	$(PROJECT_ROOT)/$(BATTLESHIP_DECOMP)/src/sc/scsubsys/scsubsysdataness.c \
	$(PROJECT_ROOT)/$(BATTLESHIP_DECOMP)/src/sc/scsubsys/scsubsysdatadonkey.c \
	$(PROJECT_ROOT)/$(BATTLESHIP_DECOMP)/src/sc/scsubsys/scsubsysdatasamus.c \
	$(PROJECT_ROOT)/$(BATTLESHIP_DECOMP)/src/sc/scsubsys/scsubsysdatalink.c \
	$(PROJECT_ROOT)/$(BATTLESHIP_DECOMP)/src/sc/scsubsys/scsubsysdatakirby.c \
	$(PROJECT_ROOT)/$(BATTLESHIP_DECOMP)/src/sc/scsubsys/scsubsysdatapurin.c \
	$(PROJECT_ROOT)/$(BATTLESHIP_DECOMP)/src/sc/scsubsys/scsubsysdatacaptain.c \
	$(PROJECT_ROOT)/$(BATTLESHIP_DECOMP)/src/sc/scsubsys/scsubsysdatapikachu.c \
	$(PROJECT_ROOT)/$(BATTLESHIP_DECOMP)/src/sc/scsubsys/scsubsysdatamario.c \
	$(PROJECT_ROOT)/$(BATTLESHIP_DECOMP)/src/sc/scsubsys/scsubsysdatayoshi.c \
	$(PROJECT_ROOT)/$(BATTLESHIP_DECOMP)/src/relocData/213_DonkeyMain.c \
	$(PROJECT_ROOT)/$(BATTLESHIP_DECOMP)/src/relocData/209_FoxMain.c \
	$(PROJECT_ROOT)/$(BATTLESHIP_DECOMP)/src/relocData/239_NessMain.c \
	$(PROJECT_ROOT)/$(BATTLESHIP_DECOMP)/src/relocData/217_SamusMain.c \
	$(PROJECT_ROOT)/$(BATTLESHIP_DECOMP)/src/relocData/225_LinkMain.c \
	$(PROJECT_ROOT)/$(BATTLESHIP_DECOMP)/src/relocData/229_KirbyMain.c \
	$(PROJECT_ROOT)/$(BATTLESHIP_DECOMP)/src/relocData/233_PurinMain.c \
	$(PROJECT_ROOT)/$(BATTLESHIP_DECOMP)/src/relocData/236_CaptainMain.c \
	$(PROJECT_ROOT)/$(BATTLESHIP_DECOMP)/src/relocData/243_PikachuMain.c \
	$(BATTLESHIP_O2R)/reloc_fighters_main/DonkeyMain \
	$(BATTLESHIP_O2R)/reloc_fighters_main/FoxMain \
	$(BATTLESHIP_O2R)/reloc_fighters_main/NessMain \
	$(BATTLESHIP_O2R)/reloc_fighters_main/SamusMain \
	$(BATTLESHIP_O2R)/reloc_fighters_main/LinkMain \
	$(BATTLESHIP_O2R)/reloc_fighters_main/KirbyMain \
	$(BATTLESHIP_O2R)/reloc_fighters_main/PurinMain \
	$(BATTLESHIP_O2R)/reloc_fighters_main/CaptainMain \
	$(BATTLESHIP_O2R)/reloc_fighters_main/PikachuMain \
	$(BATTLESHIP_O2R)/reloc_fighters_main/DonkeyShieldPose \
	$(BATTLESHIP_O2R)/reloc_fighters_main/FoxShieldPose \
	$(BATTLESHIP_O2R)/reloc_fighters_main/NessShieldPose \
	$(BATTLESHIP_O2R)/reloc_fighters_main/SamusShieldPose \
	$(BATTLESHIP_O2R)/reloc_fighters_main/LinkShieldPose \
	$(BATTLESHIP_O2R)/reloc_fighters_main/KirbyShieldPose \
	$(BATTLESHIP_O2R)/reloc_fighters_main/PurinShieldPose \
	$(BATTLESHIP_O2R)/reloc_fighters_main/CaptainShieldPose \
	$(BATTLESHIP_O2R)/reloc_fighters_main/PikachuShieldPose
NDS_NATIVE_CASTLE_BUMPER_PACKET := $(PROJECT_ROOT)/src/nds/generated/nds_native_castle_bumper.generated.inc
NDS_NATIVE_CASTLE_BUMPER_HEADER := $(PROJECT_ROOT)/include/nds/generated/nds_native_castle_bumper.generated.h
NDS_NATIVE_CASTLE_BUMPER_PREREQ := \
	$(PROJECT_ROOT)/scripts/stages/generate_nds_native_castle_bumper.py \
	$(PROJECT_ROOT)/scripts/stages/generate_nds_native_stage.py
NDS_NATIVE_LINK_BOMB_PACKET := $(PROJECT_ROOT)/src/nds/generated/nds_native_link_bomb.generated.inc
# The adapter admits the bomb in one translation unit and the executor draws
# both of its lists in another, so the pinned constants are a shared header.
NDS_NATIVE_LINK_BOMB_HEADER := $(PROJECT_ROOT)/include/nds/generated/nds_native_link_bomb.generated.h
NDS_NATIVE_LINK_BOMB_PREREQ := \
	$(PROJECT_ROOT)/scripts/stages/generate_nds_native_link_bomb.py \
	$(PROJECT_ROOT)/scripts/stages/generate_nds_native_stage.py \
	$(PROJECT_ROOT)/decomp/BattleShip-main/include/reloc_data.us.h \
	$(PROJECT_ROOT)/$(BATTLESHIP_DECOMP)/src/it/itfighter/itlinkbomb.c \
	$(PROJECT_ROOT)/$(BATTLESHIP_DECOMP)/src/it/itdisplay.c \
	$(PROJECT_ROOT)/$(BATTLESHIP_DECOMP)/src/it/itmanager.c \
	$(PROJECT_ROOT)/$(BATTLESHIP_DECOMP)/src/lb/lbcommon.c
NDS_NATIVE_THUNDERGROUND_PACKET := $(PROJECT_ROOT)/src/nds/generated/nds_native_pikachu_thunderground.generated.inc
NDS_NATIVE_THUNDERGROUND_HEADER := $(PROJECT_ROOT)/include/nds/generated/nds_native_pikachu_thunderground.generated.h
NDS_NATIVE_NESS_PKFIRE_PACKET := $(PROJECT_ROOT)/src/nds/generated/nds_native_ness_pkfire.generated.inc
NDS_NATIVE_NESS_PKFIRE_HEADER := $(PROJECT_ROOT)/include/nds/generated/nds_native_ness_pkfire.generated.h
NDS_NATIVE_NESS_PKFIRE_PREREQ := \
	$(PROJECT_ROOT)/scripts/stages/generate_nds_native_ness_pkfire.py \
	$(PROJECT_ROOT)/scripts/stages/generate_nds_native_stage.py \
	$(PROJECT_ROOT)/decomp/BattleShip-main/include/reloc_data.us.h \
	$(PROJECT_ROOT)/$(BATTLESHIP_DECOMP)/src/wp/wpness/wpnesspkfire.c
NDS_NATIVE_NESS_PKTHUNDER_PACKET := $(PROJECT_ROOT)/src/nds/generated/nds_native_ness_pkthunder.generated.inc
NDS_NATIVE_NESS_PKTHUNDER_HEADER := $(PROJECT_ROOT)/include/nds/generated/nds_native_ness_pkthunder.generated.h
NDS_NATIVE_NESS_PKTHUNDER_PREREQ := \
	$(PROJECT_ROOT)/scripts/stages/generate_nds_native_ness_pkthunder.py \
	$(PROJECT_ROOT)/scripts/stages/generate_nds_native_stage.py \
	$(PROJECT_ROOT)/decomp/BattleShip-main/BattleShip_o2r/reloc_fighters_main/NessModel
NDS_NATIVE_YOSHI_EGG_PACKET := $(PROJECT_ROOT)/src/nds/generated/nds_native_yoshi_egg.generated.inc
NDS_NATIVE_YOSHI_EGG_HEADER := $(PROJECT_ROOT)/include/nds/generated/nds_native_yoshi_egg.generated.h
NDS_NATIVE_YOSHI_EGG_PREREQ := \
	$(PROJECT_ROOT)/scripts/stages/generate_nds_native_yoshi_egg.py \
	$(PROJECT_ROOT)/scripts/stages/generate_nds_native_stage.py \
	$(PROJECT_ROOT)/decomp/BattleShip-main/BattleShip_o2r/reloc_fighters_main/YoshiModel
NDS_NATIVE_YOSHI_EGGLAY_PACKET := $(PROJECT_ROOT)/src/nds/generated/nds_native_yoshi_egglay.generated.inc
NDS_NATIVE_PURIN_SING_PACKET := $(PROJECT_ROOT)/src/nds/generated/nds_native_purin_sing.generated.inc
NDS_NATIVE_PURIN_SING_HEADER := $(PROJECT_ROOT)/include/nds/generated/nds_native_purin_sing.generated.h
NDS_NATIVE_KIRBY_VULCAN_PACKET := $(PROJECT_ROOT)/src/nds/generated/nds_native_kirby_vulcan.generated.inc
NDS_NATIVE_KIRBY_VULCAN_HEADER := $(PROJECT_ROOT)/include/nds/generated/nds_native_kirby_vulcan.generated.h
NDS_NATIVE_PIKACHU_THUNDER_PACKET := $(PROJECT_ROOT)/src/nds/generated/nds_native_pikachu_thunder.generated.inc
NDS_NATIVE_PIKACHU_THUNDER_HEADER := $(PROJECT_ROOT)/include/nds/generated/nds_native_pikachu_thunder.generated.h
NDS_NATIVE_SAMUS_BOMB_PACKET := $(PROJECT_ROOT)/src/nds/generated/nds_native_samus_bomb.generated.inc
NDS_NATIVE_SAMUS_BOMB_HEADER := $(PROJECT_ROOT)/include/nds/generated/nds_native_samus_bomb.generated.h
NDS_NATIVE_NESS_PKTAIL_PACKET := $(PROJECT_ROOT)/src/nds/generated/nds_native_ness_pktail.generated.inc
NDS_NATIVE_NESS_PKTAIL_HEADER := $(PROJECT_ROOT)/include/nds/generated/nds_native_ness_pktail.generated.h
NDS_NATIVE_PURIN_SING_PREREQ := $(PROJECT_ROOT)/scripts/stages/generate_nds_native_purin_sing.py $(PROJECT_ROOT)/scripts/stages/generate_nds_native_stage.py $(PROJECT_ROOT)/decomp/BattleShip-main/BattleShip_o2r/reloc_fighters_main/PurinSpecial2
NDS_NATIVE_YOSHI_EGGLAY_HEADER := $(PROJECT_ROOT)/include/nds/generated/nds_native_yoshi_egglay.generated.h
NDS_NATIVE_YOSHI_EGGLAY_PREREQ := \
	$(PROJECT_ROOT)/scripts/stages/generate_nds_native_yoshi_egglay.py \
	$(PROJECT_ROOT)/scripts/stages/generate_nds_native_stage.py \
	$(PROJECT_ROOT)/decomp/BattleShip-main/BattleShip_o2r/reloc_fighters_main/YoshiSpecial3
NDS_NATIVE_YOSHI_ENTRYEGG_PACKET := $(PROJECT_ROOT)/src/nds/generated/nds_native_yoshi_entryegg.generated.inc
NDS_NATIVE_YOSHI_ENTRYEGG_HEADER := $(PROJECT_ROOT)/include/nds/generated/nds_native_yoshi_entryegg.generated.h
NDS_NATIVE_YOSHI_ENTRYEGG_PREREQ := \
	$(PROJECT_ROOT)/scripts/stages/generate_nds_native_yoshi_entryegg.py \
	$(PROJECT_ROOT)/scripts/stages/generate_nds_native_stage.py \
	$(PROJECT_ROOT)/decomp/BattleShip-main/BattleShip_o2r/reloc_fighters_main/YoshiSpecial2
NDS_NATIVE_THUNDERGROUND_PREREQ := 	$(PROJECT_ROOT)/scripts/stages/generate_nds_native_pikachu_thunderground.py 	$(PROJECT_ROOT)/scripts/stages/generate_nds_native_stage.py 	$(PROJECT_ROOT)/decomp/BattleShip-main/include/reloc_data.us.h 	$(PROJECT_ROOT)/$(BATTLESHIP_DECOMP)/src/wp/wppikachu/wppikachuthunder.c
NDS_NATIVE_ITEM_TOMATO_PACKET := $(PROJECT_ROOT)/src/nds/generated/nds_native_item_tomato.generated.inc
NDS_NATIVE_ITEM_TOMATO_HEADER := $(PROJECT_ROOT)/include/nds/generated/nds_native_item_tomato.generated.h
NDS_NATIVE_ITEM_TOMATO_PREREQ := 	$(PROJECT_ROOT)/scripts/stages/generate_nds_native_item_tomato.py 	$(PROJECT_ROOT)/scripts/stages/generate_nds_native_stage.py 	$(PROJECT_ROOT)/decomp/BattleShip-main/include/reloc_data.us.h
NDS_NATIVE_ITEM_BAKED_PACKET := $(PROJECT_ROOT)/src/nds/generated/nds_native_item_baked.generated.inc
NDS_NATIVE_ITEM_BAKED_HEADER := $(PROJECT_ROOT)/include/nds/generated/nds_native_item_baked.generated.h
NDS_NATIVE_ITEM_BAKED_PREREQ := 	$(PROJECT_ROOT)/scripts/stages/generate_nds_native_item_baked.py 	$(PROJECT_ROOT)/scripts/stages/generate_nds_native_stage.py
NDS_NATIVE_ITEM_GLUCKY_PACKET := $(PROJECT_ROOT)/src/nds/generated/nds_native_item_glucky.generated.inc
NDS_NATIVE_ITEM_GLUCKY_HEADER := $(PROJECT_ROOT)/include/nds/generated/nds_native_item_glucky.generated.h
NDS_NATIVE_ITEM_GLUCKY_PREREQ := 	$(PROJECT_ROOT)/scripts/stages/generate_nds_native_item_glucky.py 	$(PROJECT_ROOT)/scripts/stages/generate_nds_native_stage.py 	$(PROJECT_ROOT)/decomp/BattleShip-main/include/reloc_data.us.h
NDS_NATIVE_ITEM_PORYGON_PACKET := $(PROJECT_ROOT)/src/nds/generated/nds_native_item_porygon.generated.inc
NDS_NATIVE_ITEM_PORYGON_HEADER := $(PROJECT_ROOT)/include/nds/generated/nds_native_item_porygon.generated.h
NDS_NATIVE_ITEM_PORYGON_PREREQ := 	$(PROJECT_ROOT)/scripts/stages/generate_nds_native_item_porygon.py 	$(PROJECT_ROOT)/scripts/stages/generate_nds_native_stage.py 	$(PROJECT_ROOT)/decomp/BattleShip-main/include/reloc_data.us.h
NDS_NATIVE_ITEM_HITOKAGE_PACKET := $(PROJECT_ROOT)/src/nds/generated/nds_native_item_hitokage.generated.inc
NDS_NATIVE_ITEM_HITOKAGE_HEADER := $(PROJECT_ROOT)/include/nds/generated/nds_native_item_hitokage.generated.h
NDS_NATIVE_ITEM_HITOKAGE_PREREQ := 	$(PROJECT_ROOT)/scripts/stages/generate_nds_native_item_hitokage.py 	$(PROJECT_ROOT)/scripts/stages/generate_nds_native_yamabuki_live_item.py 	$(PROJECT_ROOT)/scripts/stages/generate_nds_native_stage.py 	$(PROJECT_ROOT)/decomp/BattleShip-main/include/reloc_data.us.h
NDS_NATIVE_ITEM_FUSHIGIBANA_PACKET := $(PROJECT_ROOT)/src/nds/generated/nds_native_item_fushigibana.generated.inc
NDS_NATIVE_ITEM_FUSHIGIBANA_HEADER := $(PROJECT_ROOT)/include/nds/generated/nds_native_item_fushigibana.generated.h
NDS_NATIVE_ITEM_FUSHIGIBANA_PREREQ := 	$(PROJECT_ROOT)/scripts/stages/generate_nds_native_item_fushigibana.py 	$(PROJECT_ROOT)/scripts/stages/generate_nds_native_yamabuki_live_item.py 	$(PROJECT_ROOT)/scripts/stages/generate_nds_native_stage.py 	$(PROJECT_ROOT)/decomp/BattleShip-main/include/reloc_data.us.h
# The twenty wave-1 native item owners are one producer with the item's slug as
# its first positional argument, so they were twenty copies of one recipe plus
# sixty copies of three variable definitions. A copied recipe is a place to
# forget when the producer's prerequisites change, and a stale copy rebuilds
# nothing rather than failing, so the drift is silent.
#
# Each entry is <slug>/<VAR>. The pair is written out rather than derived with a
# 26-deep subst chain: it is read far more often than it is edited, and a
# mis-cased derivation would expand to an EMPTY variable and a rule with no
# target, not to an error.
NDS_NATIVE_ITEM_WAVE1 := \
	bat/BAT \
	bombhei/BOMBHEI \
	box/BOX \
	capsule/CAPSULE \
	egg/EGG \
	fflower/FFLOWER \
	gshell/GSHELL \
	hammer/HAMMER \
	harisen/HARISEN \
	heart/HEART \
	iwark/IWARK \
	lgun/LGUN \
	kirbystar/KIRBYSTAR \
	mball/MBALL \
	msbomb/MSBOMB \
	nbumper/NBUMPER \
	rshell/RSHELL \
	star/STAR \
	starrod/STARROD \
	sword/SWORD \
	taru/TARU
NDS_NATIVE_ITEM_WAVE1_PRODUCER := $(PROJECT_ROOT)/scripts/stages/generate_nds_native_item_wave1_core.py

define NDS_NATIVE_ITEM_WAVE1_RULE
NDS_NATIVE_ITEM_$(2)_PACKET := $$(PROJECT_ROOT)/src/nds/generated/nds_native_item_$(1).generated.inc
NDS_NATIVE_ITEM_$(2)_HEADER := $$(PROJECT_ROOT)/include/nds/generated/nds_native_item_$(1).generated.h
NDS_NATIVE_ITEM_$(2)_PREREQ := $$(NDS_NATIVE_ITEM_WAVE1_PRODUCER) $(PROJECT_ROOT)/scripts/stages/generate_nds_native_stage.py $(PROJECT_ROOT)/decomp/BattleShip-main/include/reloc_data.us.h
$$(NDS_NATIVE_ITEM_$(2)_PACKET) $$(NDS_NATIVE_ITEM_$(2)_HEADER) &: $$(NDS_NATIVE_ITEM_$(2)_PREREQ)
	python "$$(NDS_NATIVE_ITEM_WAVE1_PRODUCER)" $(1) --emit
	@touch $$(NDS_NATIVE_ITEM_$(2)_PACKET) $$(NDS_NATIVE_ITEM_$(2)_HEADER)
endef

$(foreach p,$(NDS_NATIVE_ITEM_WAVE1),$(eval $(call NDS_NATIVE_ITEM_WAVE1_RULE,$(word 1,$(subst /, ,$(p))),$(word 2,$(subst /, ,$(p))))))

NDS_NATIVE_THUNDERJOLTFX_PACKET := $(PROJECT_ROOT)/src/nds/generated/nds_native_pikachu_thunderjolt_effect.generated.inc
NDS_NATIVE_THUNDERJOLTFX_HEADER := $(PROJECT_ROOT)/include/nds/generated/nds_native_pikachu_thunderjolt_effect.generated.h
NDS_NATIVE_THUNDERJOLTFX_PREREQ := 	$(PROJECT_ROOT)/scripts/stages/generate_nds_native_pikachu_thunderjolt_effect.py 	$(PROJECT_ROOT)/scripts/stages/generate_nds_native_stage.py 	$(PROJECT_ROOT)/decomp/BattleShip-main/include/reloc_data.us.h 	$(PROJECT_ROOT)/$(BATTLESHIP_DECOMP)/src/ef/efmanager.c 	$(PROJECT_ROOT)/$(BATTLESHIP_DECOMP)/src/wp/wppikachu/wppikachuthunderjolt.c
NDS_NATIVE_DAMAGE_SLASH_PACKET := $(PROJECT_ROOT)/src/nds/generated/nds_native_damage_slash.generated.inc
NDS_NATIVE_DAMAGE_SLASH_HEADER := $(PROJECT_ROOT)/include/nds/generated/nds_native_damage_slash.generated.h
NDS_NATIVE_DAMAGE_FLY_MDUST_PACKET := $(PROJECT_ROOT)/src/nds/generated/nds_native_damage_fly_mdust.generated.inc
NDS_NATIVE_DAMAGE_FLY_MDUST_HEADER := $(PROJECT_ROOT)/include/nds/generated/nds_native_damage_fly_mdust.generated.h
NDS_NATIVE_DAMAGE_FLY_MDUST_PREREQ := \
	$(PROJECT_ROOT)/scripts/3d_vfx/generate_nds_native_damage_fly_mdust.py \
	$(PROJECT_ROOT)/scripts/stages/generate_nds_native_stage.py \
	$(PROJECT_ROOT)/decomp/BattleShip-main/include/reloc_data.us.h \
	$(PROJECT_ROOT)/$(BATTLESHIP_DECOMP)/src/ef/efmanager.c \
	$(PROJECT_ROOT)/$(BATTLESHIP_DECOMP)/src/ef/efdef.h \
	$(PROJECT_ROOT)/$(BATTLESHIP_DECOMP)/src/sys/objtypes.h \
	$(PROJECT_ROOT)/$(BATTLESHIP_DECOMP)/src/relocData/83_EFCommonEffects1.c \
	$(BATTLESHIP_O2R)/reloc_effects/EFCommonEffects1
NDS_NATIVE_DAMAGE_SLASH_PREREQ := 	$(PROJECT_ROOT)/scripts/3d_vfx/generate_nds_damage_slash.py 	$(PROJECT_ROOT)/scripts/stages/generate_nds_native_stage.py 	$(PROJECT_ROOT)/decomp/BattleShip-main/include/reloc_data.us.h 	$(PROJECT_ROOT)/$(BATTLESHIP_DECOMP)/src/ef/efmanager.c 	$(PROJECT_ROOT)/$(BATTLESHIP_DECOMP)/src/relocData/83_EFCommonEffects1.c 	$(BATTLESHIP_O2R)/reloc_effects/EFCommonEffects1
NDS_NATIVE_THUNDERJOLT_PACKET := $(PROJECT_ROOT)/src/nds/generated/nds_native_pikachu_thunderjolt.generated.inc
NDS_NATIVE_THUNDERJOLT_HEADER := $(PROJECT_ROOT)/include/nds/generated/nds_native_pikachu_thunderjolt.generated.h
NDS_NATIVE_THUNDERJOLT_PREREQ := 	$(PROJECT_ROOT)/scripts/stages/generate_nds_native_pikachu_thunderjolt.py 	$(PROJECT_ROOT)/scripts/stages/generate_nds_native_stage.py 	$(PROJECT_ROOT)/decomp/BattleShip-main/include/reloc_data.us.h
NDS_NATIVE_INISHIE_POWBLOCK_PACKET := $(PROJECT_ROOT)/src/nds/generated/nds_native_inishie_powblock.generated.inc
NDS_NATIVE_INISHIE_POWBLOCK_HEADER := $(PROJECT_ROOT)/include/nds/generated/nds_native_inishie_powblock.generated.h
NDS_NATIVE_INISHIE_POWBLOCK_PREREQ := 	$(PROJECT_ROOT)/scripts/stages/generate_nds_native_inishie_powblock.py 	$(PROJECT_ROOT)/scripts/stages/generate_nds_native_stage.py 	$(PROJECT_ROOT)/scripts/stages/native_stage_descriptors/inishie.py 	$(PROJECT_ROOT)/$(BATTLESHIP_DECOMP)/src/gr/grcommon/grinishie.c 	$(PROJECT_ROOT)/$(BATTLESHIP_DECOMP)/src/it/itground/itpowerblock.c 	$(PROJECT_ROOT)/$(BATTLESHIP_DECOMP)/src/relocData/155_StageInishieFile3.c 	$(PROJECT_ROOT)/$(BATTLESHIP_DECOMP)/src/relocData/260_GRInishieMap.c
NDS_NATIVE_MARUMINE_PACKET := $(PROJECT_ROOT)/src/nds/generated/nds_native_yamabuki_marumine.generated.inc
# The adapter admits the item in one translation unit and the executor draws it
# in another, so the pinned constants are a shared header.
NDS_NATIVE_MARUMINE_HEADER := $(PROJECT_ROOT)/include/nds/generated/nds_native_yamabuki_marumine.generated.h
NDS_NATIVE_MARUMINE_PREREQ := \
	$(PROJECT_ROOT)/scripts/stages/generate_nds_native_yamabuki_marumine.py \
	$(PROJECT_ROOT)/scripts/stages/generate_nds_native_stage.py \
	$(PROJECT_ROOT)/decomp/BattleShip-main/include/reloc_data.us.h \
	$(PROJECT_ROOT)/$(BATTLESHIP_DECOMP)/src/it/itground/itmarumine.c \
	$(PROJECT_ROOT)/$(BATTLESHIP_DECOMP)/src/it/itdisplay.c \
	$(PROJECT_ROOT)/$(BATTLESHIP_DECOMP)/src/it/itmanager.c \
	$(PROJECT_ROOT)/$(BATTLESHIP_DECOMP)/src/lb/lbcommon.c \
	$(PROJECT_ROOT)/$(BATTLESHIP_DECOMP)/src/gr/grcommon/gryamabuki.c
NDS_NATIVE_SECTOR_LASER_PACKET := $(PROJECT_ROOT)/src/nds/generated/nds_native_sector_arwing_laser.generated.inc
# The adapter admits the laser in one translation unit and the executor draws
# it in another, so the pinned constants are emitted as a header both include.
NDS_NATIVE_SECTOR_LASER_HEADER := $(PROJECT_ROOT)/include/nds/generated/nds_native_sector_arwing_laser.generated.h
NDS_NATIVE_SECTOR_LASER_PREREQ := \
	$(PROJECT_ROOT)/scripts/stages/generate_nds_native_sector_arwing_laser.py \
	$(PROJECT_ROOT)/scripts/stages/generate_nds_native_stage.py \
	$(PROJECT_ROOT)/$(BATTLESHIP_DECOMP)/src/gr/grcommon/grsector.c \
	$(PROJECT_ROOT)/$(BATTLESHIP_DECOMP)/src/wp/wpmanager.c \
	$(PROJECT_ROOT)/$(BATTLESHIP_DECOMP)/src/relocData/153_StageSectorFile3.c \
	$(PROJECT_ROOT)/$(BATTLESHIP_DECOMP)/src/relocData/262_GRSectorMap.c
NDS_NATIVE_ACTOR_YOSTER_PREREQ := \
	$(PROJECT_ROOT)/scripts/stages/generate_nds_native_yoster_clouds.py \
	$(PROJECT_ROOT)/scripts/stages/generate_nds_native_stage.py \
	$(PROJECT_ROOT)/scripts/stages/native_stage_descriptors/yoster.py \
	$(PROJECT_ROOT)/$(BATTLESHIP_DECOMP)/src/gr/grcommon/gryoster.c \
	$(PROJECT_ROOT)/$(BATTLESHIP_DECOMP)/src/relocData/154_StageYosterFile3.c
NDS_NATIVE_OWNERS_GENERATOR := $(PROJECT_ROOT)/scripts/fighters/generate_nds_native_owners.py
NDS_NATIVE_OWNER_IR := $(PROJECT_ROOT)/src/nds/nds_native_fighter_owner.generated.inc
NDS_NATIVE_ACTOR_TARU_PREREQ := \
	$(NDS_NATIVE_OWNERS_GENERATOR) \
	$(NDS_OWNER_LINK_INTRO_STAMP) \
	$(PROJECT_ROOT)/scripts/fighters/native_owner_image_arrays.py \
	$(PROJECT_ROOT)/scripts/stages/native_stage_descriptors/jungle.py \
	$(PROJECT_ROOT)/$(BATTLESHIP_DECOMP)/src/gr/grcommon/grjungle.c \
	$(PROJECT_ROOT)/$(BATTLESHIP_DECOMP)/src/relocData/158_StageJungleFile3.c

$(NDS_NATIVE_CHARGESHOT_PACKET) $(NDS_NATIVE_CHARGESHOT_HEADER) &: $(NDS_NATIVE_CHARGESHOT_PREREQ)
	python "$(PROJECT_ROOT)/scripts/stages/generate_nds_native_samus_chargeshot.py" --emit
	@touch $(NDS_NATIVE_CHARGESHOT_PACKET) $(NDS_NATIVE_CHARGESHOT_HEADER)

$(NDS_SHIELD_POSE_HEADER) $(NDS_SHIELD_POSE_MANIFEST) $(NDS_SHIELD_POSE_SOURCE_FILES) &: $(NDS_SHIELD_POSE_PREREQ)
	python "$(PROJECT_ROOT)/scripts/fighters/generate_nds_shield_pose_pack.py" --emit-assets
	@touch $(NDS_SHIELD_POSE_HEADER) $(NDS_SHIELD_POSE_MANIFEST) $(NDS_SHIELD_POSE_SOURCE_FILES)

$(NITROFS_DIR)/fighters/shield_pose/%.bin: $(NDS_SHIELD_POSE_SOURCE_DIR)/%.bin
	@mkdir -p "$(dir $@)"
	@cp "$<" "$@"

$(NDS_NITROFS_ADMISSION_FILES): $(NDS_FIGHTER_ADMISSION_PREREQ)
	@mkdir -p "$(dir $@)"
	python "$(PROJECT_ROOT)/scripts/fighters/generate_nds_fighter_admission.py" --no-header --out "$@"

$(NDS_NATIVE_CASTLE_BUMPER_PACKET) $(NDS_NATIVE_CASTLE_BUMPER_HEADER) &: $(NDS_NATIVE_CASTLE_BUMPER_PREREQ)
	python "$(PROJECT_ROOT)/scripts/stages/generate_nds_native_castle_bumper.py" --emit
	@touch $(NDS_NATIVE_CASTLE_BUMPER_PACKET) $(NDS_NATIVE_CASTLE_BUMPER_HEADER)

$(NDS_NATIVE_LINK_BOMB_PACKET) $(NDS_NATIVE_LINK_BOMB_HEADER) &: $(NDS_NATIVE_LINK_BOMB_PREREQ)
	python "$(PROJECT_ROOT)/scripts/stages/generate_nds_native_link_bomb.py" --emit
	@touch $(NDS_NATIVE_LINK_BOMB_PACKET) $(NDS_NATIVE_LINK_BOMB_HEADER)

$(NDS_NATIVE_MARUMINE_PACKET) $(NDS_NATIVE_MARUMINE_HEADER) &: $(NDS_NATIVE_MARUMINE_PREREQ)
	python "$(PROJECT_ROOT)/scripts/stages/generate_nds_native_yamabuki_marumine.py" --emit
	@touch $(NDS_NATIVE_MARUMINE_PACKET) $(NDS_NATIVE_MARUMINE_HEADER)

$(NDS_NATIVE_INISHIE_POWBLOCK_PACKET) $(NDS_NATIVE_INISHIE_POWBLOCK_HEADER) &: $(NDS_NATIVE_INISHIE_POWBLOCK_PREREQ)
	python "$(PROJECT_ROOT)/scripts/stages/generate_nds_native_inishie_powblock.py" --emit
	@touch $(NDS_NATIVE_INISHIE_POWBLOCK_PACKET) $(NDS_NATIVE_INISHIE_POWBLOCK_HEADER)

$(NDS_NATIVE_THUNDERJOLT_PACKET) $(NDS_NATIVE_THUNDERJOLT_HEADER) &: $(NDS_NATIVE_THUNDERJOLT_PREREQ)
	python "$(PROJECT_ROOT)/scripts/stages/generate_nds_native_pikachu_thunderjolt.py" --emit
	@touch $(NDS_NATIVE_THUNDERJOLT_PACKET) $(NDS_NATIVE_THUNDERJOLT_HEADER)

$(NDS_NATIVE_NESS_PKFIRE_PACKET) $(NDS_NATIVE_NESS_PKFIRE_HEADER) &: $(NDS_NATIVE_NESS_PKFIRE_PREREQ)
	python "$(PROJECT_ROOT)/scripts/stages/generate_nds_native_ness_pkfire.py" --emit
	@touch $(NDS_NATIVE_NESS_PKFIRE_PACKET) $(NDS_NATIVE_NESS_PKFIRE_HEADER)

$(NDS_NATIVE_NESS_PKTHUNDER_PACKET) $(NDS_NATIVE_NESS_PKTHUNDER_HEADER) &: $(NDS_NATIVE_NESS_PKTHUNDER_PREREQ)
	python "$(PROJECT_ROOT)/scripts/stages/generate_nds_native_ness_pkthunder.py" --emit
	@touch $(NDS_NATIVE_NESS_PKTHUNDER_PACKET) $(NDS_NATIVE_NESS_PKTHUNDER_HEADER)

$(NDS_NATIVE_YOSHI_EGG_PACKET) $(NDS_NATIVE_YOSHI_EGG_HEADER) &: $(NDS_NATIVE_YOSHI_EGG_PREREQ)
	python "$(PROJECT_ROOT)/scripts/stages/generate_nds_native_yoshi_egg.py" --emit
	@touch $(NDS_NATIVE_YOSHI_EGG_PACKET) $(NDS_NATIVE_YOSHI_EGG_HEADER)

$(NDS_NATIVE_YOSHI_EGGLAY_PACKET) $(NDS_NATIVE_YOSHI_EGGLAY_HEADER) &: $(NDS_NATIVE_YOSHI_EGGLAY_PREREQ)
	python "$(PROJECT_ROOT)/scripts/stages/generate_nds_native_yoshi_egglay.py" --emit
	@touch $(NDS_NATIVE_YOSHI_EGGLAY_PACKET) $(NDS_NATIVE_YOSHI_EGGLAY_HEADER)

$(NDS_NATIVE_PURIN_SING_PACKET) $(NDS_NATIVE_PURIN_SING_HEADER) &: $(NDS_NATIVE_PURIN_SING_PREREQ)
	python "$(PROJECT_ROOT)/scripts/stages/generate_nds_native_purin_sing.py" --emit
	@touch $(NDS_NATIVE_PURIN_SING_PACKET) $(NDS_NATIVE_PURIN_SING_HEADER)

# Prerequisites through a named _PREREQ, like every other native owner.
# These four wrote theirs inline, which make honours but check-native-owner-wiring.py
# cannot audit -- it reads the variable, so an inline list looks like no list at
# all and the owner reads as unwired. Same prerequisites, same rule, named.
NDS_NATIVE_KIRBY_VULCAN_PREREQ := $(PROJECT_ROOT)/scripts/stages/generate_nds_native_kirby_vulcan.py $(PROJECT_ROOT)/scripts/stages/generate_nds_native_stage.py $(BATTLESHIP_O2R)/reloc_fighters_main/KirbySpecial2
$(NDS_NATIVE_KIRBY_VULCAN_PACKET) $(NDS_NATIVE_KIRBY_VULCAN_HEADER) &: $(NDS_NATIVE_KIRBY_VULCAN_PREREQ)
	python "$(PROJECT_ROOT)/scripts/stages/generate_nds_native_kirby_vulcan.py" --emit
	@touch $(NDS_NATIVE_KIRBY_VULCAN_PACKET) $(NDS_NATIVE_KIRBY_VULCAN_HEADER)

# Prerequisites through a named _PREREQ, like every other native owner.
# These four wrote theirs inline, which make honours but check-native-owner-wiring.py
# cannot audit -- it reads the variable, so an inline list looks like no list at
# all and the owner reads as unwired. Same prerequisites, same rule, named.
NDS_NATIVE_PIKACHU_THUNDER_PREREQ := $(PROJECT_ROOT)/scripts/stages/generate_nds_native_pikachu_thunder.py $(PROJECT_ROOT)/scripts/stages/generate_nds_native_stage.py $(BATTLESHIP_O2R)/reloc_fighters_main/PikachuModel $(BATTLESHIP_O2R)/reloc_fighters_main/PikachuSpecial2
$(NDS_NATIVE_PIKACHU_THUNDER_PACKET) $(NDS_NATIVE_PIKACHU_THUNDER_HEADER) &: $(NDS_NATIVE_PIKACHU_THUNDER_PREREQ)
	python "$(PROJECT_ROOT)/scripts/stages/generate_nds_native_pikachu_thunder.py" --emit
	@touch $(NDS_NATIVE_PIKACHU_THUNDER_PACKET) $(NDS_NATIVE_PIKACHU_THUNDER_HEADER)
# Prerequisites through a named _PREREQ, like every other native owner.
# These four wrote theirs inline, which make honours but check-native-owner-wiring.py
# cannot audit -- it reads the variable, so an inline list looks like no list at
# all and the owner reads as unwired. Same prerequisites, same rule, named.
NDS_NATIVE_SAMUS_BOMB_PREREQ := $(PROJECT_ROOT)/scripts/stages/generate_nds_native_samus_bomb.py $(PROJECT_ROOT)/scripts/stages/generate_nds_native_stage.py $(BATTLESHIP_O2R)/reloc_fighters_main/SamusModel
$(NDS_NATIVE_SAMUS_BOMB_PACKET) $(NDS_NATIVE_SAMUS_BOMB_HEADER) &: $(NDS_NATIVE_SAMUS_BOMB_PREREQ)
	python "$(PROJECT_ROOT)/scripts/stages/generate_nds_native_samus_bomb.py" --emit
	@touch $(NDS_NATIVE_SAMUS_BOMB_PACKET) $(NDS_NATIVE_SAMUS_BOMB_HEADER)
# Prerequisites through a named _PREREQ, like every other native owner.
# These four wrote theirs inline, which make honours but check-native-owner-wiring.py
# cannot audit -- it reads the variable, so an inline list looks like no list at
# all and the owner reads as unwired. Same prerequisites, same rule, named.
NDS_NATIVE_NESS_PKTAIL_PREREQ := $(PROJECT_ROOT)/scripts/stages/generate_nds_native_ness_pktail.py $(PROJECT_ROOT)/scripts/stages/generate_nds_native_stage.py $(BATTLESHIP_O2R)/reloc_fighters_main/NessModel
$(NDS_NATIVE_NESS_PKTAIL_PACKET) $(NDS_NATIVE_NESS_PKTAIL_HEADER) &: $(NDS_NATIVE_NESS_PKTAIL_PREREQ)
	python "$(PROJECT_ROOT)/scripts/stages/generate_nds_native_ness_pktail.py" --emit
	@touch $(NDS_NATIVE_NESS_PKTAIL_PACKET) $(NDS_NATIVE_NESS_PKTAIL_HEADER)

$(NDS_NATIVE_YOSHI_ENTRYEGG_PACKET) $(NDS_NATIVE_YOSHI_ENTRYEGG_HEADER) &: $(NDS_NATIVE_YOSHI_ENTRYEGG_PREREQ)
	python "$(PROJECT_ROOT)/scripts/stages/generate_nds_native_yoshi_entryegg.py" --emit
	@touch $(NDS_NATIVE_YOSHI_ENTRYEGG_PACKET) $(NDS_NATIVE_YOSHI_ENTRYEGG_HEADER)

$(NDS_NATIVE_THUNDERGROUND_PACKET) $(NDS_NATIVE_THUNDERGROUND_HEADER) &: $(NDS_NATIVE_THUNDERGROUND_PREREQ)
	python "$(PROJECT_ROOT)/scripts/stages/generate_nds_native_pikachu_thunderground.py" --emit
	@touch $(NDS_NATIVE_THUNDERGROUND_PACKET) $(NDS_NATIVE_THUNDERGROUND_HEADER)

$(NDS_NATIVE_THUNDERJOLTFX_PACKET) $(NDS_NATIVE_THUNDERJOLTFX_HEADER) &: $(NDS_NATIVE_THUNDERJOLTFX_PREREQ)
	python "$(PROJECT_ROOT)/scripts/stages/generate_nds_native_pikachu_thunderjolt_effect.py" --emit
	@touch $(NDS_NATIVE_THUNDERJOLTFX_PACKET) $(NDS_NATIVE_THUNDERJOLTFX_HEADER)

$(NDS_NATIVE_DAMAGE_SLASH_PACKET) $(NDS_NATIVE_DAMAGE_SLASH_HEADER) &: $(NDS_NATIVE_DAMAGE_SLASH_PREREQ)
	python "$(PROJECT_ROOT)/scripts/3d_vfx/generate_nds_damage_slash.py" --emit
	@touch $(NDS_NATIVE_DAMAGE_SLASH_PACKET) $(NDS_NATIVE_DAMAGE_SLASH_HEADER)

$(NDS_NATIVE_DAMAGE_FLY_MDUST_PACKET) $(NDS_NATIVE_DAMAGE_FLY_MDUST_HEADER) &: $(NDS_NATIVE_DAMAGE_FLY_MDUST_PREREQ)
	python "$(PROJECT_ROOT)/scripts/3d_vfx/generate_nds_native_damage_fly_mdust.py" --emit
	@touch $(NDS_NATIVE_DAMAGE_FLY_MDUST_PACKET) $(NDS_NATIVE_DAMAGE_FLY_MDUST_HEADER)

nds_renderer.o: $(NDS_NATIVE_DAMAGE_FLY_MDUST_PACKET) $(NDS_NATIVE_DAMAGE_FLY_MDUST_HEADER)
scene_backend.o: $(NDS_NATIVE_DAMAGE_FLY_MDUST_HEADER)

$(NDS_NATIVE_ITEM_TOMATO_PACKET) $(NDS_NATIVE_ITEM_TOMATO_HEADER) &: $(NDS_NATIVE_ITEM_TOMATO_PREREQ)
	python "$(PROJECT_ROOT)/scripts/stages/generate_nds_native_item_tomato.py" --emit
	@touch $(NDS_NATIVE_ITEM_TOMATO_PACKET) $(NDS_NATIVE_ITEM_TOMATO_HEADER)

$(NDS_NATIVE_ITEM_BAKED_PACKET) $(NDS_NATIVE_ITEM_BAKED_HEADER) &: $(NDS_NATIVE_ITEM_BAKED_PREREQ)
	python "$(PROJECT_ROOT)/scripts/stages/generate_nds_native_item_baked.py" --emit
	@touch $(NDS_NATIVE_ITEM_BAKED_PACKET) $(NDS_NATIVE_ITEM_BAKED_HEADER)
nds_renderer.o: $(NDS_NATIVE_ITEM_BAKED_PACKET) $(NDS_NATIVE_ITEM_BAKED_HEADER)
renderer_adapter_stage.o: $(NDS_NATIVE_ITEM_BAKED_HEADER)
battleship_mvending.o: $(NDS_NATIVE_ITEM_BAKED_HEADER)
# P2-6 (2026-10-01): the 1P ending's room roots, same generator, as a NitroFS
# file the ending loads into its own heap (battleship_mvending.c): no static
# image carries them.
ifeq ($(NDS_P2_1P_GAME),1)
NDS_NITROFS_ROOM_BAKED := $(NITROFS_DIR)/movies/room_baked.bin
$(NDS_NITROFS_ROOM_BAKED): $(NDS_NATIVE_ITEM_BAKED_PREREQ)
	@mkdir -p $(dir $@)
	python "$(PROJECT_ROOT)/scripts/stages/generate_nds_native_item_baked.py" --room-out "$@"
$(OUTPUT).nds: $(NDS_NITROFS_ROOM_BAKED)
endif

$(NDS_NATIVE_ITEM_GLUCKY_PACKET) $(NDS_NATIVE_ITEM_GLUCKY_HEADER) &: $(NDS_NATIVE_ITEM_GLUCKY_PREREQ)
	python "$(PROJECT_ROOT)/scripts/stages/generate_nds_native_item_glucky.py" --emit
	@touch $(NDS_NATIVE_ITEM_GLUCKY_PACKET) $(NDS_NATIVE_ITEM_GLUCKY_HEADER)

$(NDS_NATIVE_ITEM_PORYGON_PACKET) $(NDS_NATIVE_ITEM_PORYGON_HEADER) &: $(NDS_NATIVE_ITEM_PORYGON_PREREQ)
	python "$(PROJECT_ROOT)/scripts/stages/generate_nds_native_item_porygon.py" --emit
	@touch $(NDS_NATIVE_ITEM_PORYGON_PACKET) $(NDS_NATIVE_ITEM_PORYGON_HEADER)

$(NDS_NATIVE_ITEM_HITOKAGE_PACKET) $(NDS_NATIVE_ITEM_HITOKAGE_HEADER) &: $(NDS_NATIVE_ITEM_HITOKAGE_PREREQ)
	python "$(PROJECT_ROOT)/scripts/stages/generate_nds_native_item_hitokage.py" --emit
	@touch $(NDS_NATIVE_ITEM_HITOKAGE_PACKET) $(NDS_NATIVE_ITEM_HITOKAGE_HEADER)

$(NDS_NATIVE_ITEM_FUSHIGIBANA_PACKET) $(NDS_NATIVE_ITEM_FUSHIGIBANA_HEADER) &: $(NDS_NATIVE_ITEM_FUSHIGIBANA_PREREQ)
	python "$(PROJECT_ROOT)/scripts/stages/generate_nds_native_item_fushigibana.py" --emit
	@touch $(NDS_NATIVE_ITEM_FUSHIGIBANA_PACKET) $(NDS_NATIVE_ITEM_FUSHIGIBANA_HEADER)

$(NDS_NATIVE_SECTOR_LASER_PACKET) $(NDS_NATIVE_SECTOR_LASER_HEADER) &: $(NDS_NATIVE_SECTOR_LASER_PREREQ)
	python "$(PROJECT_ROOT)/scripts/stages/generate_nds_native_sector_arwing_laser.py" --emit
	@touch $(NDS_NATIVE_SECTOR_LASER_PACKET) $(NDS_NATIVE_SECTOR_LASER_HEADER)

$(NDS_NATIVE_ACTOR_YOSTER_PACKET): $(NDS_NATIVE_ACTOR_YOSTER_PREREQ)
	python "$(PROJECT_ROOT)/scripts/stages/generate_nds_native_yoster_clouds.py" --emit-packet
	@touch $(NDS_NATIVE_ACTOR_YOSTER_PACKET)

# The owners generator emits TaruCann AND the fighter-owner IR plus the
# consumed-fields manifest from one invocation; the pair above is the grouped
# target so a parallel build runs it once, and the owner IR/manifest refresh as
# an untracked side effect rather than racing a second recipe.
$(NDS_NATIVE_ACTOR_TARU_PACKET) $(NDS_NATIVE_ACTOR_TARU_HEADER) $(NDS_NATIVE_OWNER_IR) &: $(NDS_NATIVE_ACTOR_TARU_PREREQ)
	python "$(NDS_NATIVE_OWNERS_GENERATOR)"
	@touch $(NDS_NATIVE_ACTOR_TARU_PACKET) $(NDS_NATIVE_ACTOR_TARU_HEADER) $(NDS_NATIVE_OWNER_IR)

# ABI v2 images: the generator writes the tracked header AND all 46 ignored
# per-owner image .c files in one invocation, so all 47 are one grouped target
# -- the old header-only rule left every image.c without a recipe and a fresh
# clone failed at native_image_%.o with "no rule to make target". The owner list
# is P2_IMAGE_OWNERS verbatim (generate_nds_native_owner_images.py:62-63); the
# build only compiles/links the config-enabled subset via NDS_NATIVE_IMAGE_OWNERS.
# THE OWNER IR BELONGS HERE, AND ITS ABSENCE HUNG THE CHARACTER SELECT.
#
# These deps named the owners GENERATOR but not its OUTPUT, so a regenerated
# nds_native_fighter_owner.generated.inc left the image header alone. The
# header sizes the image structs; the image .c files are initialised from the
# same tables the .inc carries. When the two drift, the excess initializers are
# a GCC warning and the data is silently TRUNCATED -- the image still builds,
# still loads, and is short.
#
# Measured: Ness's header declared state_sequence[177] while the tables held
# 195, so ness_high.bin shipped 18 entries short while sNdsNativeNessRoots kept
# indexing tail states at 188..193. Validator clause 6 rejected root 0, the
# adapter cleared native_owner_enabled, and the packed CSS preview reached
# ndsPreviewPackLoadHalt(20) -- a deliberate for(;;). Hovering Ness hung the
# ROM.
#
# This edge closes the generator direction. It cannot close the other one: a
# git operation that reverts the tracked header leaves the GITIGNORED
# src/nds/generated/*.image.c alone and gives the header a FRESH mtime, so make
# sees nothing to do. Only a content check catches that, which is
# scripts/fighters/check_native_owner_image_spans.py -- run it after any owners
# regeneration and after any checkout that touches the header.
NDS_NATIVE_IMAGE_GENERATOR_DEPS := \
	$(NDS_NATIVE_IMAGE_GENERATOR) \
	$(NDS_NATIVE_OWNERS_GENERATOR) \
	$(NDS_OWNER_LINK_INTRO_STAMP) \
	$(NDS_NATIVE_OWNER_IR) \
	$(PROJECT_ROOT)/scripts/fighters/native_owner_image_arrays.py \
	$(PROJECT_ROOT)/scripts/fighters/native_skeletons.py \
	$(PROJECT_ROOT)/include/nds/nds_native_fighter_tables.h
NDS_NATIVE_IMAGE_ALL_OWNERS := luigi donkey captain samus link pikachu yoshi ness purin kirby mmario nmario nfox ndonkey nsamus nlink nyoshi ncaptain nkirby npikachu npurin nness boss mariofox mario_skeleton1 fox_skeleton1
NDS_NATIVE_IMAGE_ALL_SRCS := $(foreach owner,$(NDS_NATIVE_IMAGE_ALL_OWNERS),$(NDS_NATIVE_IMAGE_SRC_DIR)/nds_native_fighter_$(owner)_high.image.c $(NDS_NATIVE_IMAGE_SRC_DIR)/nds_native_fighter_$(owner)_low.image.c)
NDS_NATIVE_IMAGE_ALL_SRCS += $(foreach id,$(NDS_NATIVE_KIRBY_HAT_IDS),$(NDS_NATIVE_IMAGE_SRC_DIR)/nds_native_fighter_kirby_hat_$(id)_high.image.c $(NDS_NATIVE_IMAGE_SRC_DIR)/nds_native_fighter_kirby_hat_$(id)_low.image.c)
NDS_NATIVE_IMAGE_ALL_SRCS += $(NDS_NATIVE_IMAGE_SRC_DIR)/nds_native_skeletons.generated.inc

$(NDS_NATIVE_IMAGE_HEADER) $(NDS_NATIVE_IMAGE_ALL_SRCS) &: $(NDS_NATIVE_IMAGE_GENERATOR_DEPS)
	python "$(NDS_NATIVE_IMAGE_GENERATOR)"
	@touch $(NDS_NATIVE_IMAGE_HEADER) $(NDS_NATIVE_IMAGE_ALL_SRCS)

# CFLAGS force-includes nds_build_config.h. Keep that generated header as an
# explicit edge here: these image objects are outer-make rules and otherwise
# race the config writer on a fresh parallel build.
$(BUILD)/native_image_%.o: $(NDS_NATIVE_IMAGE_SRC_DIR)/nds_native_fighter_%.image.c 		$(NDS_NATIVE_IMAGE_HEADER) 		$(PROJECT_ROOT)/include/nds/nds_native_fighter_tables.h 		$(NDS_BUILD_CONFIG)
	@mkdir -p $(dir $@)
	# The include paths are explicit here because this rule runs in the OUTER
	# make, where ds_rules' per-build INCLUDES have not been composed yet.
	$(CC) -c $(CFLAGS) -I $(PROJECT_ROOT)/include -I $(BUILD) -o $@ $<

$(NDS_NATIVE_IMAGE_DIR)/%.bin: $(BUILD)/native_image_%.o
	@mkdir -p $(dir $@)
	$(OBJCOPY) -O binary --only-section=.fighter_image $< $@


# The blob must exist before the config is written: the config carries its byte
# count, and the runtime carves exactly that much at the head of every arena
# generation. Generated from the blob rather than hardcoded so it cannot drift
# from the asset -- a stale constant here is a pack that silently never becomes
# resident.
ifeq ($(NDS_R2_BATTLEPACK),1)
$(NDS_BUILD_CONFIG): $(NDS_BATTLEPACK_BLOB)
endif

ifneq ($(NDS_TICK_HUD),1)
override NDS_TEX_IDENT_SHADOW := 0
override NDS_FTR_LEAN_KTIME := 0
endif
