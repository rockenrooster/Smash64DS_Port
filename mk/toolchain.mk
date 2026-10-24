# devkitARM's rules, the compiler and linker flags, the decomp and generated
# asset paths, the soft-float and libc objects the hot-text script places,
# and the libraries.
# Included in place from the Makefile (P8 G); its text is unchanged.

include $(DEVKITARM)/ds_rules

# devkitARM emits phony dependency targets for headers. On this Windows setup,
# some devkitPro headers appear as C:devkitPro/... in .d files, which GNU make
# reads as malformed target patterns on the next incremental harness build.
NDS_BASE_ERROR_FILTER := $(ERROR_FILTER)
ERROR_FILTER = $(NDS_BASE_ERROR_FILTER) && if [ -f "$(DEPSDIR)/$*.d" ]; \
	then sed -i -e 's!\([A-Za-z]\):devkitPro!\1:/devkitPro!g' \
	"$(DEPSDIR)/$*.d"; fi

BATTLESHIP_DECOMP := decomp/BattleShip-main/decomp
BATTLESHIP_SYS := $(BATTLESHIP_DECOMP)/src/sys
BATTLESHIP_O2R := $(PROJECT_ROOT)/decomp/BattleShip-main/BattleShip_o2r
BATTLESHIP_RELOCDATA := $(PROJECT_ROOT)/decomp/BattleShip-main/decomp/assets/us/relocData

# decomp/ is immutable source of truth. Twelve DS adaptations need source-level
# interposition inside imported BattleShip translation units; generate those
# into the per-build include tree instead of ever editing decomp/. New
# adaptations belong directly in src/import/src/port and are added here only
# when no include-side seam exists: the ninth (sc1pgame.c, 2026-09-05) compiles
# out the N64 title-signature check, a call through a data pointer inside
# sc1PGameFuncStart that no wrapper or macro can skip; the tenth
# (mnbackupclear.c) gives the implicit-int helper its source-correct void
# return type, which no header declares. The eleventh (scvsbattle.c) keeps
# N64 validation off DS and starts audio after timer/fade construction. The
# twelfth (scstaffroll.c, 2026-09-06) stores the five credit character-ID
# tables as s8 instead of s32; every measured value fits with exact promotion.
NDS_BATTLESHIP_IMPORT_OVERLAY := $(PROJECT_ROOT)/$(BUILD)/battleship_overlay
NDS_BATTLESHIP_IMPORT_OVERLAY_STAMP := $(NDS_BATTLESHIP_IMPORT_OVERLAY)/.stamp
NDS_BATTLESHIP_IMPORT_OVERLAY_GENERATOR := $(PROJECT_ROOT)/scripts/generate-battleship-import-overlay.ps1
NDS_BATTLESHIP_IMPORT_OVERLAY_PATCHES := $(wildcard $(PROJECT_ROOT)/scripts/import-overlays/battleship/*.patch)
NDS_BATTLESHIP_IMPORT_OVERLAY_INPUTS := \
	$(PROJECT_ROOT)/$(BATTLESHIP_DECOMP)/src/ft/ftanim.c \
	$(PROJECT_ROOT)/$(BATTLESHIP_DECOMP)/src/mn/mncommon/mnstartup.c \
	$(PROJECT_ROOT)/$(BATTLESHIP_DECOMP)/src/mn/mnoption/mnbackupclear.c \
	$(PROJECT_ROOT)/$(BATTLESHIP_DECOMP)/src/mv/mvopening/mvopeningroom.c \
	$(PROJECT_ROOT)/$(BATTLESHIP_DECOMP)/src/sc/scmanager.c \
	$(PROJECT_ROOT)/$(BATTLESHIP_DECOMP)/src/sc/sc1pmode/sc1pgame.c \
	$(PROJECT_ROOT)/$(BATTLESHIP_DECOMP)/src/sc/sccommon/scvsbattle.c \
	$(PROJECT_ROOT)/$(BATTLESHIP_DECOMP)/src/sc/sccommon/scstaffroll.c \
	$(PROJECT_ROOT)/$(BATTLESHIP_DECOMP)/src/sys/objanim.c \
	$(PROJECT_ROOT)/$(BATTLESHIP_DECOMP)/src/sys/objhelper.c \
	$(PROJECT_ROOT)/$(BATTLESHIP_DECOMP)/src/sys/objman.c \
	$(PROJECT_ROOT)/$(BATTLESHIP_DECOMP)/src/sys/taskman.c
NDS_BATTLESHIP_IMPORT_OVERLAY_OFILES := \
	battleship_ftanim.o battleship_mnstartup.o battleship_mnbackupclear.o battleship_mvopeningroom.o \
	battleship_scmanager.o battleship_sys_objanim.o battleship_sys_objhelper.o \
	battleship_sys_objman.o battleship_sys_taskman.o battleship_sc1pgame_runtime.o battleship_scvsbattle.o \
	battleship_scstaffroll.o

# BattleShip source files are compiled in place. They remain the source of truth.
SOURCES := src/nds src/nds/r2 src/nds/net src/port src/import $(BATTLESHIP_SYS)
# Do not add BattleShip's full include root globally: its N64 libc headers
# intentionally shadow stddef/string/etc. Compatibility headers expose the
# narrow ABI needed by each imported source slice.
INCLUDES := include $(BATTLESHIP_DECOMP)/src $(BATTLESHIP_SYS)

# Profiling must use the native runtime. The old semantic and ablation modes
# select reference rendering or suppress required drawing; keep them host-only.
ifeq ($(NDS_RENDERER_PROFILE_LEVEL),2)
$(error NDS_RENDERER_PROFILE_LEVEL=2 is host-only; ROMs must use native profile 0 or 1)
endif
ifneq ($(NDS_RENDERER_BENCHMARK_MODE),0)
$(error Renderer ablation modes are host-only; ROMs require NDS_RENDERER_BENCHMARK_MODE=0)
endif
ifneq ($(NDS_ENABLE_INISHIE_SOURCE_SCALE_SETUP),0)
$(error The Inishie source-scale preview is host-only; use the native full stage)
endif

ARCH := -march=armv5te -mtune=arm946e-s -mthumb
# Use gnu11 (not the GCC 15 default of gnu23). The BattleShip decomp source was
# written for C89/C99 and uses `bool`/`true`/`false` as ordinary identifiers
# (e.g. a parameter named `bool` in sys/taskman.c). In C23 these are keywords,
# which breaks that source; gnu11 keeps them as ordinary identifiers.
CFLAGS := -std=gnu11 -g -Wall -Wextra -O2 -ffunction-sections -fdata-sections \
	$(ARCH) $(INCLUDE) -DARM9 -D_LANGUAGE_C -DSSB64_TARGET_NDS \
	-DREGION_US -DAVOID_UB -Wno-error=incompatible-pointer-types \
	-Wno-error=int-conversion -Wno-error=maybe-uninitialized -Wundef \
	-fmax-errors=8
# -fmax-errors=8 stops a single broken typedef from cascading into megabytes of
# repeated errors (the 9-bad-TU build2 shape: one root cause per TU, hundreds of
# follow-ons). The first 8 errors per TU still print, so real type/return/
# implicit-declaration failures stay visible; only the tail is cut.
CFLAGS += -include $(PROJECT_ROOT)/$(BUILD)/nds_build_config.h
# P4: the content list and data (scripts/p4/p4_contents.py) and the donor
# native owners' image header and runtime program (p4_native_owner.py).
ifeq ($(NDS_P4),1)
CFLAGS += -I$(PROJECT_ROOT)/$(BUILD)/p4 -I$(PROJECT_ROOT)/$(BUILD)/p4/native
endif
# P4 character select tables (scripts/p4/p4_css.py).
ifeq ($(NDS_P4),1)
CFLAGS += -I$(PROJECT_ROOT)/$(BUILD)/p4/css -I$(PROJECT_ROOT)/$(BUILD)/p4/audio
endif
# P5 stage registry header and the stage select's tables (scripts/p5).
ifeq ($(NDS_P5),1)
CFLAGS += -I$(PROJECT_ROOT)/$(BUILD)/p5 -I$(PROJECT_ROOT)/$(BUILD)/p5/sss \
	-I$(PROJECT_ROOT)/$(BUILD)/p5/audio
endif
# P3 build identity (plan 7.1): libnds's inline asserts embed their headers'
# __FILE__, i.e. the devkitPro install path (C:/devkitPro/libnds/include/...),
# so the same commit built with the toolchain installed elsewhere had another
# identity. Map the prefix so it does not.
ifneq ($(strip $(DEVKITPRO)),)
CFLAGS += -ffile-prefix-map=$(DEVKITPRO)=devkitPro
endif
ifeq ($(NDS_DEV_SCENE_HARNESS),normal)
NDS_DEV_SCENE_HARNESS_ID := 0
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_playable)
NDS_DEV_SCENE_HARNESS_ID := 163
# The scripted profile-2 diagnostic ROM is substantially larger than the
# shipped renderer; keep its scene-reserve coverage size-optimized.
CFLAGS += -Os
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_playable_realtime)
NDS_DEV_SCENE_HARNESS_ID := 163
# The canonical P1 runtime/performance anchor needs latency optimization.
CFLAGS += -O2
else ifeq ($(NDS_DEV_SCENE_HARNESS),battle_playable_match_lifecycle)
NDS_DEV_SCENE_HARNESS_ID := 163
# The fast-logic timer/Results diagnostic is a reserve proof, not a benchmark.
CFLAGS += -Os
else ifeq ($(NDS_DEV_SCENE_HARNESS),results_playable)
NDS_DEV_SCENE_HARNESS_ID := 164
# Boots straight into VS Results with a finished match seeded, so R2-07 work
# can profile and A/B the scene without emulating the whole minute first. It is
# a latency surface for the same reason battle_playable_realtime is: the numbers
# taken here are per-frame Results cost, so it must not be size-optimized.
CFLAGS += -O2
else
$(error Unknown NDS_DEV_SCENE_HARNESS "$(NDS_DEV_SCENE_HARNESS)"; use normal or a harness named in this block)
endif


CXXFLAGS := $(CFLAGS) -fno-rtti -fno-exceptions
ASFLAGS := -g $(ARCH)
NDS_HOT_TEXT_SPECS := $(PROJECT_ROOT)/linker/ds9_hot_text.specs
NDS_HOT_TEXT_LINKER_SCRIPT := $(PROJECT_ROOT)/linker/nds_hot_text.ld
NDS_MEMORY_LINKER_SCRIPT := $(PROJECT_ROOT)/linker/nds_memory.ld
NDS_FRONTEND_OVERLAY_LINKER_SCRIPT := $(PROJECT_ROOT)/linker/nds_frontend_overlay.ld
ifeq ($(NDS_P5),1)
# P5: Bowser's Stadium's bombs run the Race to the Finish bomb's code in VS,
# whose battles borrow the whole overlay, so P5 builds keep that object
# resident: the same script without its two input-section rows.
NDS_FRONTEND_OVERLAY_LINKER_SCRIPT := $(PROJECT_ROOT)/$(BUILD)/nds_frontend_overlay_p5.ld
$(NDS_FRONTEND_OVERLAY_LINKER_SCRIPT): $(PROJECT_ROOT)/linker/nds_frontend_overlay.ld
	@mkdir -p $(dir $@)
	sed -e '/battleship_item_tarubomb\.o (\.data/,/)/d' \
		-e '/battleship_item_tarubomb\.o/d' $< > $@
endif
NDS_TASK32_DRAW_HOT_FRAGMENT := $(PROJECT_ROOT)/$(BUILD)/nds_task32_draw_hot.inc
NDS_TASK39_HIT_SPARKS_INC := $(PROJECT_ROOT)/src/nds/generated/task39_hit_sparks.generated.inc
NDS_TASK39_HIT_SPARKS_ASSET := $(PROJECT_ROOT)/assets/effects/task39_hit_sparks.rgb5a1.bin
NDS_PARTICLE_BANKS_INC := $(PROJECT_ROOT)/src/nds/generated/nds_particle_banks.generated.inc
NDS_PARTICLE_BANKS_HEADER := $(PROJECT_ROOT)/include/nds/generated/nds_particle_banks.generated.h
# Slice 32. The baked fighter-animation bank: 145,873 20-byte write records plus
# 71,500 u16 control entries, replacing the figatree interpretation at runtime.
# Generated from the 297 AObj16 files in $(BATTLESHIP_O2R)/reloc_animations,
# which the build otherwise copies into nitrofs verbatim.
#
# DEFAULT OFF, and it must stay that way until a reader exists. The blob is
# ~3.1 MB and the particle comment below records what that costs when nothing
# opens it. Turning this on without the runtime bind adds ROM and changes
# nothing else.
NDS_R2_FTANIM_DENSE ?= 0
NDS_FTANIM_DENSE_ASSET := $(PROJECT_ROOT)/assets/animation/ftanim_dense_bank.bin
NDS_FTANIM_DENSE_SOURCES := \
	$(wildcard $(BATTLESHIP_O2R)/reloc_animations/FTMarioAnim*) \
	$(wildcard $(BATTLESHIP_O2R)/reloc_animations/FTFoxAnim*)
NDS_FTANIM_STREAM_ASSET := $(PROJECT_ROOT)/assets/animation/ftanim_stream_pack.bin
NDS_FTANIM_STREAM_SOURCES := \
	$(wildcard $(BATTLESHIP_O2R)/reloc_animations/FTMarioAnim*) \
	$(wildcard $(BATTLESHIP_O2R)/reloc_animations/FTFoxAnim*) \
	$(wildcard $(BATTLESHIP_O2R)/reloc_animations/FTDonkeyAnim*) \
	$(wildcard $(BATTLESHIP_O2R)/reloc_animations/FTSamusAnim*) \
	$(wildcard $(BATTLESHIP_O2R)/reloc_animations/FTLuigiAnim*) \
	$(wildcard $(BATTLESHIP_O2R)/reloc_animations/FTLinkAnim*) \
	$(wildcard $(BATTLESHIP_O2R)/reloc_animations/FTKirbyAnim*) \
	$(wildcard $(BATTLESHIP_O2R)/reloc_animations/FTKirbyCopyAnim*) \
	$(wildcard $(BATTLESHIP_O2R)/reloc_animations/FTCaptainAnim*) \
	$(wildcard $(BATTLESHIP_O2R)/reloc_animations/FTPurinAnim*) \
	$(wildcard $(BATTLESHIP_O2R)/reloc_animations/FTNessAnim*) \
	$(wildcard $(BATTLESHIP_O2R)/reloc_animations/FTYoshiAnim*) \
	$(wildcard $(BATTLESHIP_O2R)/reloc_animations/FTPikachuAnim*)

# Slice 1 phase 5's resident figatree pack. ONE fighter, because the taskman
# arena holds 287,904 (Fox) or 271,728 (Mario) but not the ~559,632 both need.
# Fox is the resident: it is the autonomous level-3 CPU on the Boundary arm, so
# its action changes dominate the acquisitions there, and it is the larger blob,
# which makes it the binding fit test.
# `--items-off` drops the 38 clips proven unreachable from the linked battle ELF.
#
# STAGED INTO NitroFS, NOT `.incbin`. The first build linked it into .rodata and
# +288,992 B of ARM9 image was MEASURED to push gNdsTaskmanArenaChosenSize
# 0x150000 -> 0x140000 with 16 alloc failures: the arena is one calloc from the
# same libnds heap the static image bounds, so image growth and arena size come
# out of the same bytes. check-boot-headroom.ps1 CANNOT see that -- it reported
# 66,784 B of proven headroom on that arm. As a NitroFS payload the blob costs
# the ARM9 image nothing and the runtime streams it into the arena at setup, in
# place of the 262,144 B raw-file cache it replaces.
NDS_BATTLEPACK_DIR := $(PROJECT_ROOT)/assets/animation
NDS_BATTLEPACK_BLOB := $(NDS_BATTLEPACK_DIR)/battlepack_fox.bin

NDS_R2_FTANIM_TRACK_DISPATCH ?= 1

# P2-2p6 (owner ruling 2026-08-23: "do both"). The fighter pose engine,
# `src/nds/nds_ft_pose.c`: the figatree script machine and the Q12 evaluator
# over compact per-fighter tracks in place of the per-tick AObj-list parse and
# play, and -- NDS_FT_POSE_HOLD -- body-joint evaluation on the last source tick
# of each presented frame only (TransN/XRotN/YRotN and the attach tick stay
# 60 Hz). NDS_FT_POSE_ORACLE is the lab proof: the engine runs on shadow joints
# beside the generic path and compares every pose/clock field bit for bit
# (`gNdsFtPoseOracleMismatches` must read 0 over a whole match).
#
# DEFAULT-ON 2026-08-23: the oracle read 0 mismatches over 189,251 joint
# compares with the hold and the Q12 clock (artifacts/verification/
# 2026-08-23_ft-pose-oracle.txt), and the four-CPU stress banked
# WORK-H P50/P95 1,264,512/1,836,800 -> 1,244,608/1,777,408 across the
# engine+hold+Q12 series (board row P2-2p6). ORACLE stays a lab flag.
NDS_FT_POSE ?= 1

# Fox Blaster's shared beam/flash/collision bore line (`nds_effects.h`).
# SETTLED BY THE OWNER 2026-08-15: "bore should be zero, no offset, not needed
# anymore". The 84 was eye-tuned 2026-08-14 to compensate a gun-joint pose the
# segment-phase parser defect left a frame stale; `64c41c361a7` repaired the
# pose, so the compensation is gone with it. This variable stays so a trial
# value costs no source edit.
NDS_FOX_BLASTER_BORE_OFFSET_Y ?= 0
NDS_FTANIM_TRACK_MAX_BYTES ?= 12288
NDS_FTANIM_TRACK_HEADER := $(PROJECT_ROOT)/include/nds/generated/nds_ftanim_track_pack.generated.h
NDS_FTANIM_TRACK_PREREQ :=

NDS_PARTICLE_TEXTURE_ASSET := $(PROJECT_ROOT)/assets/particles/efcommon_particle_textures.ds.bin
NDS_WHISPY_NATIVE_ASSET := $(PROJECT_ROOT)/assets/particles/grpupupu_whispy_native.ds.bin
NDS_HYRULE_NATIVE_ASSET := $(PROJECT_ROOT)/assets/particles/grhyrule_native.ds.bin
# The draw path's own payload: the admitted textures as RGB555+A1, which is the
# format the renderer's texture cache uploads. Separate from the file above
# because that one is per-texture DS formats with palettes and the cache has no
# palette slot in its key.
NDS_PARTICLE_QUAD_ASSET := $(PROJECT_ROOT)/assets/particles/efcommon_particle_quads.a5i3.bin
# The battle static-texture metadata include and its NitroFS payload. Both are
# build products of one generator invocation, and until 2026-08-05 NOTHING in
# this Makefile knew how to produce either: only build.ps1 ran the generator, so
# an incremental `make` linked whatever copy happened to be on disk. That is how
# the .inc stayed at its Aug-3 content across three builds while its generator
# and the census it imports had both moved -- harmless only because that day's
# delta was a header comment. The next such delta would be data, and it would
# reach a measured ROM with no error anywhere (see CLAUDE.md: this class "races
# into a subtly wrong binary rather than an error").
NDS_BATTLE_STATIC_TEXTURE_INC := $(PROJECT_ROOT)/src/nds/generated/battle_playable_static_textures.generated.inc
NDS_BATTLE_STATIC_TEXTURE_ASSET := $(PROJECT_ROOT)/assets/renderer/battle_playable_static_textures.rgb5a1.bin
# P2-1c. Same shape and the same reason: the manifest is compiled in and the
# texel/intensity payload ships in NitroFS, both written by one generator so a
# stale pair cannot link.
NDS_MN_UI_KIT_INC := $(PROJECT_ROOT)/src/nds/generated/mn_ui_kit.generated.inc
NDS_MN_UI_KIT_ASSET := $(PROJECT_ROOT)/assets/menus/mn_ui_kit.bin
# P3: the networked character select's 2P-4P player art (its own pack).
NDS_MN_UI_KIT_NET_ASSET := $(PROJECT_ROOT)/assets/menus/mn_ui_kit_net.bin
NDS_NATIVE_WALLPAPER_INC := $(PROJECT_ROOT)/src/nds/generated/native_wallpapers.generated.inc
NDS_NATIVE_WALLPAPER_NAMES := pupupu zebes jungle yoster yamabuki castle sector hyrule inishie last zako metal bonus training_black training_yellow training_blue results
NDS_NATIVE_WALLPAPER_ASSETS := $(foreach name,$(NDS_NATIVE_WALLPAPER_NAMES),$(PROJECT_ROOT)/assets/wallpapers/native_wallpaper_$(name).bin)
# P2-2/P2-3. The lower battle HUD is AOT-only: source IFCommon digits and each
# admitted fighter's portrait/stock icon are baked straight into tiled 4bpp
# sub-OBJ cells.
# Graphics upload once from NitroFS; metrics/palettes remain resident.
NDS_BATTLE_HUD_INC := $(PROJECT_ROOT)/src/nds/generated/battle_hud.generated.inc
NDS_BATTLE_HUD_ASSET := $(PROJECT_ROOT)/assets/menus/battle_hud.bin
# Match-entry presentation.  Mario's pipe and Fox's Arwing keep BattleShip's
# live DObj animation but consume an AOT DS-native mesh/texture packet.  Unlike
# a review-only manifest this include is compiled directly by nds_renderer.c,
# so its generator belongs on the normal dependency graph: stale generated
# geometry must never survive an O2R or decoder change into a measured ROM.
NDS_ENTRY_EFFECT_INC := $(PROJECT_ROOT)/src/nds/nds_entry_effects.generated.inc
# P2-1h. The backdrop art is a SECOND payload from the same generator: the OBJ
# pack is read and hashed on every kit entry, so a title screen living in it
# would cost the character select bytes it never draws.
NDS_MN_UI_SURFACE_ASSET := $(PROJECT_ROOT)/assets/menus/mn_surfaces.bin
# P2-1k (d). The title pop animation's pose table. A SECOND generator writes it
# -- `decode_mn_title_anim.py` owns the animation, transcribes objanim.c and
# carries the oracles -- but it reads the kit generator as a module, so the two
# agree on every raster size by construction rather than by convention. It
# depends on the kit's source as well as its own for exactly that reason.
NDS_MN_TITLE_ANIM_INC := \
	$(PROJECT_ROOT)/src/nds/generated/mn_title_anim.generated.inc
LDFLAGS := -specs=$(NDS_HOT_TEXT_SPECS) -g $(ARCH) \
	-Wl,-Map,$(NDS_OUTPUT_BASENAME).map,--gc-sections,--emit-relocs \
	-Wl,--wrap=nitroromGetSelf,--wrap=dvmMountVolume,--wrap=dvmUnmountVolume \
	-Wl,--wrap=cpuGetTiming \
	-Wl,--wrap=glTexImage2D,--wrap=glColorTableEXT,--wrap=glColorSubTableEXT \
	-Wl,-T,$(NDS_MEMORY_LINKER_SCRIPT) \
	-Wl,-T,$(NDS_FRONTEND_OVERLAY_LINKER_SCRIPT) \
	-Wl,-T,$(NDS_HOT_TEXT_LINKER_SCRIPT)
LDFLAGS += -Wl,--undefined=__nds_task9_libgcc_fcmpeq_golden \
	-Wl,--undefined=__nds_task16_libgcc_fcmpun_golden
LDFLAGS += -Wl,--undefined=__nds_task16_libgcc_i2f_golden
LDFLAGS += -Wl,--undefined=__nds_task16_libgcc_fadd_golden

# Task 9 Phase 1 uses the exact objects from the selected Thumb multilib. The
# objects contain ARM-state implementations; only their section placement is
# changed. Phase 2 renames the stock fcmpeq wrapper as an in-ROM golden and
# links one integer-only leaf replacement ahead of libgcc. Keep the list narrow
# so the renderer's existing ITCM ownership is never displaced by low-frequency
# double-precision helpers.
NDS_TASK9_FLOAT_LIBGCC_SHA256 := \
	c755adc33eca252260360327904591b8462cce5c25e48b0e881ac0b295953f48
NDS_TASK9_FLOAT_ITCM_MEMBERS := \
	_arm_addsubsf3.o _arm_muldivsf3.o _arm_cmpsf2.o \
	_arm_unordsf2.o _arm_fixsfsi.o _arm_fixunssfsi.o
# P2-2p8 2026-09-27: src/nds/nds_float_conv.c supplies the unsigned and 64-bit
# int->float conversions and __floatsisf (host-checked exact), so the extracted
# _arm_addsubsf3.o -- otherwise dead Task 16 goldens in ITCM -- has its copies
# renamed below and lives in main RAM. Needs the Task 16 __aeabi_i2f.
NDS_P2_FLOAT_CONV ?= 1
ifeq ($(NDS_P2_FLOAT_CONV),1)
NDS_TASK9_FLOAT_MAIN_MEMBERS += _arm_addsubsf3.o
endif
# The suffix is load-bearing, not cosmetic. `linker/nds_hot_text.ld:113` reads
#   *.itcm.* (.text .stub .text.* .gnu.linkonce.t.*)
# so a file NAMED `*.itcm.*` has its .text placed in ITCM whatever its section
# is called. Dropping --rename-section alone therefore frees nothing; the member
# has to leave the filename pattern as well. `.mainram.o` matches neither that
# rule nor the `*.32.o` one beside it.
NDS_TASK9_FLOAT_ITCM_OFILES := \
	$(foreach member,$(NDS_TASK9_FLOAT_ITCM_MEMBERS), \
		$(basename $(member))$(if $(filter $(member),$(NDS_TASK9_FLOAT_MAIN_MEMBERS)),.mainram.o,.itcm.o))

# Task 37 measured leaves. The census split every profiled instruction's stall
# cycles by whether the instruction touched data, because only the non-memory
# half is recoverable by moving code. These five sit in .main today and carry
# 7,387,317 non-memory stall cycles across 906 bytes -- the densest set that
# fits ITCM's 1,060 free bytes with no eviction of any current resident.
#
#   memset       140 B   2,391,465    memcpy       170 B   1,173,373
#   memcmp        70 B   1,249,372    __ieee754_sqrtf 236 B 1,054,412
#   (port side: ndsFTParamsInvalidateFighterParts, TextureSourceBytes, PolyFmt)
#
# Same mechanism as Task 9: extract from a SHA-verified private copy of the
# archive and rename the section. The code is byte-identical; only where it
# lives changes, so the A/B measures placement and nothing else.
NDS_LIBC_ARCHIVE_SHA256 := \
	01424211f6f671e0b07b52fb72086f14e18000fca089e9ecfe45aa77b36873e2
NDS_LIBM_ARCHIVE_SHA256 := \
	b437e8747f520c891d2784df015ab6f8cd30bb91cd02430f03657c20027d6685
NDS_ITCM_LIBC_MEMBERS := libc_a-memset.o libc_a-memcpy-stub.o libc_a-memcmp.o
NDS_ITCM_LIBM_MEMBERS := libm_a-ef_sqrt.o
NDS_ITCM_LIB_OFILES := \
	$(if $(filter 1,$(NDS_ITCM_LIBC)),$(addsuffix .itcm.o,$(basename $(NDS_ITCM_LIBC_MEMBERS)))) \
	$(if $(filter 1,$(NDS_ITCM_LIBM)),$(addsuffix .itcm.o,$(basename $(NDS_ITCM_LIBM_MEMBERS))))

LIBS := -lfat -lfilesystem -lnds9 -lm
LIBDIRS := $(LIBNDS)

# Campaign Zako waves choose all twelve polygon kinds at runtime
# (sc1pgame.c:1160-1201,2090-2097). Their 23 own files ride NDS_1P_RELOC_FILES;
# dFTN*Data also needs each base fighter's motion/shield files. NLuigi reuses
# NMario's model/owner and Mario's shield. Mario/Fox are already unconditional.
# Apply after shell roster selection and override partial command lines so
# campaign builds cannot silently omit a required opponent or donor.
# Run in both make passes: recursive make writes nds_build_config.h.
ifeq ($(NDS_P2_1P_GAME),1)
override NDS_P2_NMARIO := 1
override NDS_P2_NFOX := 1
override NDS_P2_NDONKEY := 1
override NDS_P2_NSAMUS := 1
override NDS_P2_NLUIGI := 1
override NDS_P2_NLINK := 1
override NDS_P2_NYOSHI := 1
override NDS_P2_NCAPTAIN := 1
override NDS_P2_NKIRBY := 1
override NDS_P2_NPIKACHU := 1
override NDS_P2_NPURIN := 1
override NDS_P2_NNESS := 1
override NDS_P2_LUIGI := 1
override NDS_P2_DONKEY := 1
override NDS_P2_CAPTAIN := 1
override NDS_P2_SAMUS := 1
override NDS_P2_LINK := 1
override NDS_P2_PIKACHU := 1
override NDS_P2_YOSHI := 1
override NDS_P2_NESS := 1
override NDS_P2_PURIN := 1
override NDS_P2_KIRBY := 1
# The campaign's own variants: Giant Donkey Kong (stage 7, sc1pgame.c:394)
# and Metal Mario (stage 11). Without them the stage's fighter has no
# attributes and ftManagerMakeFighter faults on its parts container.
override NDS_P2_GDONKEY := 1
override NDS_P2_MMARIO := 1
endif

