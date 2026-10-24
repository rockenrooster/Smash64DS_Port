# The outer make (the repository root): the output names and the explicit
# source lists -- every original subsystem, port seam and DS module a
# configuration compiles (CFILES).
# Included in place from the Makefile (P8 G); its text is unchanged.


export OUTPUT := $(NDS_OUTPUT_ROOT)/$(NDS_OUTPUT_BASENAME)
export VPATH := $(foreach dir,$(SOURCES),$(CURDIR)/$(dir))
export DEPSDIR := $(CURDIR)/$(BUILD)

# Keep this list explicit. Adding an original subsystem is a deliberate port step.
NDS_MPPROCESS_SOURCE_CFILES := battleship_mpprocess_edge_support.c \
	battleship_mpprocess.c
CFILES := main.c nds_platform.c nds_native_wallpaper.c nds_ifcommon_oam.c nds_results_oam.c nds_source2d.c nds_task39_effect_census.c nds_reloc_assets.c nds_native_stage_blob.c nds_audio_assets.c nds_audio_bgm.c nds_audio_fgm.c nds_audio_storage.c nds_renderer.c battle_playable_static_textures.c nds_battlepack_anim.c n64_stubs.c coroutine.c \
	libultra_os.c os_selftest.c boot_stubs.c battleship_sys_main.c \
	scheduler_backend.c controller_backend.c battleship_sys_scheduler.c \
	battleship_sys_controller.c battleship_sys_maindevice.c \
	battleship_sys_video.c battleship_sys_malloc.c \
	battleship_sys_framebuffer.c battleship_sys_zbuffer.c video_bootstrap.c video_blackout.c \
	battleship_sys_sintable.c battleship_sys_matrix.c \
	battleship_libultra_gu_normalize.c battleship_libultra_gu_mtxcatf.c \
	battleship_scmanager.c battleship_mnstartup.c scene_backend.c scene_harness.c nds_match_config.c nds_scene_manager.c nds_frontend_overlay.c nds_kirby_hat_residency.c nds_audio_extent.c battleship_sys_utils.c vector.c nds_replay_digest.c \
	battleship_scsubsyscontroller.c \
	battleship_sys_taskman.c battleship_sys_objman.c \
	battleship_sys_objhelper.c battleship_sys_objanim.c \
	battleship_ftdisplaylights.c battleship_ftdisplaymain.c \
	battleship_sys_interp.c battleship_mvopeningroom.c \
	battleship_mvopeningportraits.c battleship_mvopeningmario.c \
	battleship_mvopeningdonkey.c battleship_mvopeninglink.c \
	battleship_mvopeningsamus.c battleship_mvopeningyoshi.c \
	battleship_mvopeningkirby.c battleship_mvopeningfox.c \
	battleship_mvopeningpikachu.c battleship_mntitlefiles.c \
	battleship_mntitle.c battleship_mnvsmode.c \
	battleship_mnplayersvs.c battleship_mnmaps.c \
	battleship_gmcommon.c battleship_gmcollision.c battleship_scvsbattle.c \
	battleship_grpupupu_ground.c \
	battleship_grinishie_scale.c \
	battleship_ftcommon_wait.c \
	battleship_ftcommon_walk.c battleship_ftcommon_turn.c \
	battleship_ftcommon_turnrun.c \
	battleship_ftcommon_dash.c \
battleship_ftcommon_attack1.c \
battleship_ftcommon_attack100.c \
battleship_ftcommon_attackdash.c \
battleship_ftcommon_attackair.c \
battleship_mariofox_mainmotion.c \
battleship_ftcommon_appeal.c \
battleship_ftcommon_catch.c \
battleship_ftcommon_guard.c \
battleship_ftcommon_escape.c \
battleship_ftcommon_run.c battleship_ftcommon_runbrake.c \
	battleship_ftcommon_kneebend.c battleship_ftcommon_jump.c \
	battleship_ftcommon_pass.c \
	battleship_ftcommon_fall.c battleship_ftcommon_ottotto.c \
	battleship_ftcommon_landing.c battleship_ftcommon_stopceil.c \
	battleship_ftcommon_cliffcatchwait.c \
	battleship_ftcommon_cliffclimb.c \
	battleship_ftcommon_cliffattack.c \
	battleship_ftcommon_cliffescape.c \
	battleship_ftcommon_walldamage.c \
	battleship_ftcommon_rebound.c \
	battleship_ftcommon_twister.c \
	battleship_ftcommon_furasleep.c \
	battleship_ftcommon_damage.c \
	battleship_ftcommon_damagefall.c \
	battleship_ftcommon_passive.c \
	battleship_ftcommon_passivestand.c \
	battleship_ftcommon_downwaitbounce.c \
	battleship_ftcommon_downattack.c \
	battleship_ftcommon_downforwardback.c \
	battleship_ftcommon_downstand.c \
	battleship_ftcommon_dokan.c
# P2-2p8 Phase 1 slice 1: the lean fighter path's ARM-mode joint kernel
# (include/nds/renderer_fighter_lean.h). Route word default 0 = today's draw.
CFILES += nds_ftr_lean_kernel.c
# Runtime 2 battle path (R2-01). Added only when the flag is on, so the default
# arm's link input set is unchanged rather than merely equivalent -- an empty
# translation unit still enters the link and this project has measured
# re-addressing collateral from far less (Tasks 87-89/94/95).
ifeq ($(NDS_R2_PATH),1)
CFILES += nds_r2_battle.c
endif
# P2-4 first stage: Yoshi's Island ground logic (decomp gryoster.c import).
# Flag off, the TU is not linked at all, so the default arm is unchanged.
ifeq ($(NDS_P2_STAGE_YOSTER),1)
CFILES += battleship_gryoster_ground.c
endif
# P2-4 stage 2: Peach's Castle ground logic (decomp grcastle.c import).
# Flag off, the TU is not linked at all, so the default arm is unchanged.
ifeq ($(NDS_P2_STAGE_CASTLE),1)
CFILES += battleship_grcastle_ground.c
endif
# P2-4 stage 3: Congo Jungle ground logic (decomp grjungle.c import).
ifeq ($(NDS_P2_STAGE_JUNGLE),1)
CFILES += battleship_grjungle_ground.c
endif
# P2-4 stage 4: Planet Zebes ground logic (decomp grzebes.c import).
ifeq ($(NDS_P2_STAGE_ZEBES),1)
CFILES += battleship_grzebes_ground.c
endif
# P2-4 stage 5: Hyrule Castle ground logic (decomp grhyrule.c import). It
# adds no reloc payload: its map declares only StageCastle and
# ExternDataBank113, both already staged and rowed for other reasons.
ifeq ($(NDS_P2_STAGE_HYRULE),1)
CFILES += battleship_grhyrule_ground.c
endif
# P2-4 stage 6: Saffron City ground logic (decomp gryamabuki.c import).
ifeq ($(NDS_P2_STAGE_YAMABUKI),1)
CFILES += battleship_gryamabuki_ground.c
endif
# P2-4 stage 7: Mushroom Kingdom ground logic (decomp grinishie.c import).
# MUTUALLY EXCLUSIVE with the older scale-only import: that file defines
# grInishieMakeScale on its own, and this one brings in the whole stage
# including it, so linking both is a duplicate definition.
ifeq ($(NDS_P2_STAGE_INISHIE),1)
ifeq ($(NDS_ENABLE_INISHIE_SOURCE_SCALE_SETUP),1)
$(error NDS_P2_STAGE_INISHIE and NDS_ENABLE_INISHIE_SOURCE_SCALE_SETUP both define grInishieMakeScale; pick one)
endif
CFILES += battleship_grinishie_ground.c
endif
# P2-4 stage 8: Sector Z ground logic (decomp grsector.c import).
ifeq ($(NDS_P2_STAGE_SECTOR),1)
CFILES += battleship_grsector_ground.c
endif
ifeq ($(NDS_R2_FIXED_SQRT),1)
CFILES += nds_r2_sqrtf.c
# The ARM-state arm of the sqrtf route. Lab only: at NDS_R2_HWMATH_ROUTE 0 it is
# not in CFILES at all, so a published ROM's link input set is unchanged rather
# than merely equivalent.
ifeq ($(NDS_R2_HWMATH_ROUTE),1)
CFILES += nds_r2_sqrtf_arm.c
endif
endif
ifeq ($(NDS_R2_COLLISION_FIXED),1)
CFILES += nds_r2_collision_fixed.c nds_r2_collision_ring.c
endif
ifeq ($(NDS_P2_HURTBOX_REJECT),1)
CFILES += nds_p2_hurtbox_reject.c
endif
ifeq ($(NDS_FAST_MEM),1)
CFILES += nds_fast_mem.c
endif
ifeq ($(NDS_R2_SIM_MAC_SHADOW),1)
CFILES += nds_r2_sim_mac_fixed.c
endif
ifeq ($(NDS_R2_HWMATH_BENCH),1)
CFILES += nds_r2_hwmath_bench.c
endif
# Conditional so a published ROM stays byte-identical: at flag 0 the TU is not
# linked at all, rather than linked as ten `used` counters nothing writes.
ifeq ($(NDS_R2_FTANIM_TRACK),1)
CFILES += nds_ftanim_track.c
endif
CFILES += nds_shield_pose.c
CFILES += nds_ft_pose.c
ifeq ($(NDS_IMPORT_BATTLESHIP_NORMAL_MOVESET),1)
CFILES += battleship_ftcommon_normal_moveset.c
# ...and the real bodies for two of that file's weak stubs.
# ftParamProcPauseEffect and ftParamProcResumeEffect were no-ops there, so
# a smash's proc_lagstart/lagend and the Captain and Kirby specials never
# paused their attached effects during hitstop. Transcribed rather than
# textually included: the decomp region also defines ftParamProcStopEffect,
# which already has a strong port definition and would collide.
CFILES += battleship_ftparam_effectprocs.c
endif
ifneq ($(filter 1,$(NDS_P2_MENU_SHELL) $(NDS_P2_1P_GAME)),)
# These source menus need no campaign fighter or stage runtime.
CFILES += battleship_mnoption.c battleship_mnbackupclear.c battleship_mnsoundtest.c battleship_mnscreenadjust.c
endif
# P2-6, compiled only when the campaign flag is on.
ifeq ($(NDS_P2_1P_GAME),1)
# P2-6 step 5 (2026-09-04): Break the Targets / Board the Platforms, source imports.
CFILES += battleship_sc1pbonusstage.c battleship_sc1pbonusstagefiles.c
# P2-6 step 1 (2026-09-04): the campaign driver and the runtime half of sc1pgame.c.
CFILES += battleship_sc1pmanager.c battleship_sc1pgame_runtime.c
# P2-6 step 2 (2026-09-05): the stage-clear tally scene, whole TU (it owns the
# 58-row bonus table; the transcribed tables TU is gone).
CFILES += battleship_sc1pstageclear.c
# P2-6 step 8 (2026-09-05): the stage intro and the challenger screen, source imports.
CFILES += battleship_sc1pintro.c battleship_sc1pchallenger.c
# P2-7 items 2, 3 and 4 (2026-09-05): the unlock message, Training (scene + its
# character select) and the DATA menus, source imports; same reachability note.
CFILES += battleship_mnmessage.c battleship_sc1ptrainingmode.c battleship_mntraining.c
CFILES += battleship_mndata.c battleship_mnvsrecord.c battleship_mncharacters.c
# P2-6 step 8 (2026-09-05): the 1P Game menus, source imports (mnmessage.c is above).
CFILES += battleship_mn1pmode.c battleship_mn1pcontinue.c battleship_mnplayers1pgame.c
CFILES += battleship_mnplayers1pbonus.c battleship_mncongra.c
# P2-6 step 8 tail (2026-09-05): the ending movie and the credits, source imports.
CFILES += battleship_mvending.c battleship_scstaffroll.c
# P2-7 items 7 and 8 (2026-09-05): the attract demo and How to Play, source imports
# with their file-setup companions.
CFILES += battleship_scautodemo.c battleship_scautodemofiles.c
CFILES += battleship_scexplain.c battleship_scexplainfiles.c
# P2-6 step 7 (2026-09-05): Master Hand -- the boss scene, the finger-gun bullet and
# the boss status files, source imports.
CFILES += battleship_sc1pgameboss.c battleship_wpbossbullet.c
CFILES += battleship_ftboss.c battleship_ftboss_status_1.c battleship_ftboss_status_2.c
CFILES += battleship_ftboss_status_3.c battleship_ftboss_status_4.c
# P2-6 step 6 (2026-09-05): Race to the Finish, the scrolling course logic (grbonus3.c).
CFILES += battleship_grbonus3.c
endif
CFILES += battleship_ftchar_data_slots.c battleship_scsubsysdata_ft.c \
	battleship_ftdata.c reloc_backend_ftdata_stubs.c \
	reloc_backend_ftdata_symbols.c
CFILES += battleship_ftanim.c battleship_ftanimend.c battleship_ftkey.c
ifeq ($(NDS_IMPORT_BATTLESHIP_FTMANAGER),1)
CFILES += battleship_ftmanager.c
ifeq ($(NDS_P2_DONKEY),1)
# P2-3 DK is the first non-Mario archetype.  Compile BattleShip's own special
# and cargo state machines as one port TU rather than re-implementing their
# update/interrupt/physics/map ordering in DS glue.
CFILES += battleship_donkey.c
endif
ifneq ($(filter 1,$(NDS_P2_DONKEY) $(NDS_P2_LINK) $(NDS_P2_ITEM_CORE)),)
# DK first needed the items-off common throw subset. Link or the shared P2 item
# core graduates the same TU to BattleShip's full item-throw runtime; the TU's
# own source gate is NDS_P2_LINK || NDS_P2_ITEM_CORE, so keep the build-input
# admission in sync with that contract even on the Mario/Fox mirror control.
CFILES += battleship_ftcommon_itemthrow.c
endif
ifeq ($(NDS_P2_CAPTAIN),1)
# P2-3f5. Falcon Punch / Falcon Kick / Falcon Dive as one port TU, plus the
# VICTIM side of Falcon Dive -- which is an ftcommon TU, not an ftcaptain one,
# because nFTCommonStatusCaptureCaptain is a status any fighter can end up in.
CFILES += battleship_captain.c battleship_ftcommon_capturecaptain.c
endif
ifeq ($(NDS_P2_SAMUS),1)
# BattleShip owns Charge Shot storage/cancel/release, Screw Attack and Bomb
# behavior; the companion TU owns the source Charge Shot/Bomb weapons.
CFILES += battleship_samus.c battleship_samus_weapons.c
endif
ifeq ($(NDS_P2_LINK),1)
# BattleShip owns Link's rapid jab, entry pair, Boomerang/return state machine,
# Spin Attack and Bomb-pull/throw transitions. The shared weapon manager owns
# Boomerang and grounded Spin Attack; LinkBomb graduates through the item owner.
CFILES += battleship_link.c battleship_link_weapons.c battleship_link_bomb.c
endif
ifeq ($(NDS_P2_ITEM_CORE),1)
# The shared item owner, compiled for whichever fighters bring an item article.
CFILES += battleship_item_link_core.c
# P2-5i1 GBumper (stage bumper, kind 23): same gate, no new asset work --
# its art is the already-resident shared ITCommonData (reloc 0xfb).
CFILES += battleship_item_gbumper.c
# The item map/physics helper the common items need. Only
# itMainSetGroundAllowPickup lives there: the two itmap.c helpers beside it in
# source are already defined by battleship_item_link_core.c's own itmap.c
# import, and defining them twice is a link error.
CFILES += battleship_item_map_core.c
# P2-5 common items, all drawing from the resident ITCommonData. Touch-consumed
# first, then the swing-and-throw three, then the Hammer's fighter-state seam.
CFILES += battleship_item_tomato.c battleship_item_heart.c
CFILES += battleship_item_star.c battleship_item_hammer.c
CFILES += battleship_item_sword.c battleship_item_bat.c battleship_item_harisen.c
# The four containers. They roll a payload and spawn it when broken, which is
# the machinery the Poke Ball reuses, so they come before the monsters.
CFILES += battleship_item_box.c battleship_item_taru.c
CFILES += battleship_item_capsule.c battleship_item_egg.c
# Ammo shooters and self-actors.
CFILES += battleship_item_starrod.c battleship_item_lgun.c
CFILES += battleship_item_fflower.c battleship_item_msbomb.c
CFILES += battleship_item_bombhei.c battleship_item_nbumper.c
CFILES += battleship_item_gshell.c battleship_item_rshell.c
# The Poke Ball closes the common twenty. It rolls through itMainMakeMonster
# (battleship_item_map_core.c); until the Pokemon kinds land, that roll ends at
# itManagerMakeItemKind's bound and returns NULL, which the source's own
# monster-bus NULL check already covers -- the ball opens and nothing comes out.
CFILES += battleship_item_mball.c
# The Pokemon it rolls. Each is one kind's TU against the shared monster bus;
# they land in batches, and until all thirteen are here the roll can select a
# kind with no maker, which itManagerMakeItemKind refuses by returning NULL.
CFILES += battleship_item_nyars.c battleship_item_dogas.c
CFILES += battleship_item_kabigon.c battleship_item_tosakinto.c
CFILES += battleship_item_mew.c
CFILES += battleship_item_iwark.c battleship_item_lizardon.c
CFILES += battleship_item_spear.c battleship_item_kamex.c
CFILES += battleship_item_mlucky.c battleship_item_starmie.c
CFILES += battleship_item_sawamura.c battleship_item_pippi.c
# Stage-spawned kinds: Mushroom Kingdom's POW block and Piranha, the bonus
# target, and Saffron City's five Pokemon. These are placed by their stage
# rather than by the item spawn law, but they are ITStructs and go through
# the same maker table.
# battleship_item_target.c is a BONUS STAGE item: it calls
# sc1PBonusStageUpdateTargetCount and its descriptor names
# gSC1PBonusStageItemFile, both provided by battleship_sc1pbonusstage.c
# behind NDS_P2_1P_GAME since 2026-09-04 (P2-6 step 5), so it rides that flag.
CFILES += battleship_item_pakkun.c
# battleship_item_tarubomb.c is Race to the Finish's timed barrel bomb
# (grbonus3.c:53-55 spawns nITKindTaruBomb every 180 ticks); its descriptor
# reads the Bonus3 ground's item head, so it rides the same flag.
ifeq ($(NDS_P2_1P_GAME),1)
CFILES += battleship_item_target.c battleship_item_tarubomb.c
endif
# Six of the ten itground/ kinds reach back into their own stage's ground
# code, so they ride that stage's flag rather than the item core -- the core
# is DERIVED from the landed fighters too, so a Link build with items on and
# these stages off failed to link on exactly these six. Undefined symbols
# scanned from the objects, not guessed:
#   powerblock  -> grInishiePowerBlockSetWait/SetDamage
#   glucky, hitokage, marumine, porygon, fushigibana
#               -> grYamabukiGateSetClosedWait/ClearMonsterGObj,
#                  dGRYamabukiMonsterAttackKind
# All five Yamabuki kinds are the Saffron City GATE monsters (decomp puts
# them in it/itground/, not it/itmonster/), so gating them costs no Poke Ball
# content: the thirteen Poke Ball Pokemon are the itmonster/ set above and
# reference no stage symbol.
ifeq ($(NDS_P2_STAGE_INISHIE),1)
CFILES += battleship_item_powerblock.c
endif
ifeq ($(NDS_P2_STAGE_YAMABUKI),1)
CFILES += battleship_item_glucky.c battleship_item_marumine.c
CFILES += battleship_item_porygon.c
CFILES += battleship_item_hitokage.c battleship_item_fushigibana.c
endif
# The FIGHTER half of pickup. Without it every item above is scenery: nothing
# called itMainSetFighterHold, and every ground attack's "is there an item
# here?" check was a shim returning FALSE.
CFILES += battleship_ftcommon_get.c
# ...and the fighter half of USING one: the Ray Gun's ammo, the Fire
# Flower's flame and the Star Rod's star were all ported and all
# unreachable behind empty SetStatus shims and weak proc stubs.
CFILES += battleship_ftcommon_itemuse.c
# ...and the fighter half of HOLDING the Hammer: pickup started its timer and
# music, but eight status callbacks were weak stubs and four more seams were
# no-op shims, so the fighter never entered any Hammer state.
CFILES += battleship_ftcommon_hammer.c
endif
ifeq ($(NDS_P2_PIKACHU),1)
# BattleShip owns Thunder Jolt, Thunder and Quick Attack.
CFILES += battleship_pikachu.c
endif
ifneq ($(filter 1,$(NDS_P2_PIKACHU) $(NDS_P2_KIRBY)),)
# Pikachu's source weapon TU also owns the Thunder Jolt maker used by Kirby's
# copied neutral special. Kirby therefore needs this source support even when
# Pikachu itself is not an admitted fighter in the selected battle roster.
CFILES += battleship_pikachu_weapons.c
endif
ifeq ($(NDS_P2_YOSHI),1)
# BattleShip owns Egg Lay, Egg Throw and Yoshi Bomb; the companion TU owns the
# source egg/star weapons.
CFILES += battleship_yoshi.c battleship_yoshi_weapons.c
endif
ifneq ($(filter 1,$(NDS_P2_YOSHI) $(NDS_P2_KIRBY)),)
# Kirby's copied Egg Lay enters BattleShip's victim-side Yoshi capture state,
# so that shared source status is part of Kirby's behavior closure too.
CFILES += battleship_ftcommon_captureyoshi.c
endif
ifeq ($(NDS_P2_NESS),1)
# BattleShip owns Ness's fighter-side specials verbatim (admit_fighter.py).
CFILES += battleship_ness.c
endif
ifneq ($(filter 1,$(NDS_P2_NESS) $(NDS_P2_KIRBY)),)
# Ness's source weapon/item pair owns PK Fire and its pillar, which Kirby's
# copied neutral special calls even when Ness himself is not instantiated.
CFILES += battleship_ness_weapons.c battleship_ness_items.c
endif
ifeq ($(NDS_P2_PURIN),1)
# BattleShip owns Purin's specials and articles verbatim (admit_fighter.py).
CFILES += battleship_purin.c
endif
ifeq ($(NDS_P2_KIRBY),1)
# BattleShip owns Kirby's specials and articles verbatim (admit_fighter.py).
CFILES += battleship_kirby.c battleship_kirby_copy.c battleship_kirby_weapons.c battleship_ftcommon_capturekirby.c
endif
ifeq ($(NDS_P2_GDONKEY),1)
# BattleShip owns GDonkey's specials and articles verbatim (admit_fighter.py).
CFILES += battleship_gdonkey.c
endif
ifeq ($(NDS_P2_MMARIO),1)
# BattleShip owns MMario's specials and articles verbatim (admit_fighter.py).
CFILES += battleship_mmario.c
endif
ifeq ($(NDS_P2_NMARIO),1)
# BattleShip owns NMario's specials and articles verbatim (admit_fighter.py).
CFILES += battleship_ftn_polygons.c
endif
ifeq ($(NDS_P2_NFOX),1)
# BattleShip owns NFox's specials and articles verbatim (admit_fighter.py).
CFILES += battleship_ftn_polygons.c
endif
ifeq ($(NDS_P2_NDONKEY),1)
# BattleShip owns NDonkey's specials and articles verbatim (admit_fighter.py).
CFILES += battleship_ftn_polygons.c
endif
ifeq ($(NDS_P2_NSAMUS),1)
# BattleShip owns NSamus's specials and articles verbatim (admit_fighter.py).
CFILES += battleship_ftn_polygons.c
endif
ifeq ($(NDS_P2_NLUIGI),1)
# BattleShip owns NLuigi's specials and articles verbatim (admit_fighter.py).
CFILES += battleship_ftn_polygons.c
endif
ifeq ($(NDS_P2_NLINK),1)
# BattleShip owns NLink's specials and articles verbatim (admit_fighter.py).
CFILES += battleship_ftn_polygons.c
endif
ifeq ($(NDS_P2_NYOSHI),1)
# BattleShip owns NYoshi's specials and articles verbatim (admit_fighter.py).
CFILES += battleship_ftn_polygons.c
endif
ifeq ($(NDS_P2_NCAPTAIN),1)
# BattleShip owns NCaptain's specials and articles verbatim (admit_fighter.py).
CFILES += battleship_ftn_polygons.c
endif
ifeq ($(NDS_P2_NKIRBY),1)
# BattleShip owns NKirby's specials and articles verbatim (admit_fighter.py).
CFILES += battleship_ftn_polygons.c
endif
ifeq ($(NDS_P2_NPIKACHU),1)
# BattleShip owns NPikachu's specials and articles verbatim (admit_fighter.py).
CFILES += battleship_ftn_polygons.c
endif
ifeq ($(NDS_P2_NPURIN),1)
# BattleShip owns NPurin's specials and articles verbatim (admit_fighter.py).
CFILES += battleship_ftn_polygons.c
endif
ifeq ($(NDS_P2_NNESS),1)
# BattleShip owns NNess's specials and articles verbatim (admit_fighter.py).
CFILES += battleship_ftn_polygons.c
endif
# mp/mpprocess.c runs live (BUGS.md #1); the bridge only forwards.
CFILES += $(NDS_MPPROCESS_SOURCE_CFILES) \
	battleship_mpprocess_live_bridge.c
CFILES += battleship_ftstatus_callback_aliases.c \
	battleship_ftstatus_map_physics_shims.c \
	battleship_ftstatus_inactive_stubs.c \
	battleship_ftcommon_shieldbreakfly.c \
	battleship_ftcommon_shieldbreakfall.c \
	battleship_ftcommon_shieldbreakdown.c \
	battleship_ftcommon_shieldbreakstand.c \
	battleship_ftcommon_furafura.c
endif
ifeq ($(NDS_IMPORT_BATTLESHIP_FTCOMPUTER),1)
CFILES += battleship_ftcomputer.c battleship_ftcomputer_fixed.c
endif
ifeq ($(NDS_IMPORT_BATTLESHIP_BATTLE_PLAYABLE),1)
CFILES += battleship_gmcamera.c battleship_gmcamera_fixed.c battleship_ftcommon_dead.c \
	battleship_ftcommon_rebirth.c battleship_ftcommon_sleep.c \
	battleship_ftcommon_entry.c \
	battleship_grwallpaper.c \
	battle_playable_compat_stubs.c
endif
ifeq ($(NDS_IMPORT_BATTLESHIP_VS_RESULTS),1)
CFILES += battleship_lbtransition.c battleship_mnvsresults.c \
	battleship_mnvsresults_spot.c \
	battleship_scsubsysfighter.c battleship_scsubsysdata.c
endif
CFILES += battleship_ifscreenflash.c
CFILES += battleship_lbfade.c
ifeq ($(NDS_IMPORT_BATTLESHIP_IFCOMMON),1)
CFILES += battleship_ifcommon.c
CFILES += nds_battle_hud.c
# P3 local multiplayer (include/nds/nds_net.h).
CFILES += nds_net_link.c nds_net_session.c nds_net_lobby.c nds_net_ui.c nds_net_autopilot.c
endif
ifeq ($(NDS_IMPORT_BATTLESHIP_WEAPON_MANAGER),1)
CFILES += battleship_wpmanager_core.c
endif
ifneq ($(filter 1,$(NDS_IMPORT_BATTLESHIP_MARIO_FIREBALL) $(NDS_IMPORT_BATTLESHIP_FOX_BLASTER) $(NDS_IMPORT_BATTLESHIP_MARIO_SPECIAL_HI) $(NDS_IMPORT_BATTLESHIP_FOX_SPECIAL_HI)),)
CFILES += battleship_special_common.c
endif
ifeq ($(NDS_IMPORT_BATTLESHIP_MARIO_FIREBALL),1)
CFILES += battleship_mario_fireball.c
endif
ifeq ($(NDS_IMPORT_BATTLESHIP_FOX_BLASTER),1)
CFILES += battleship_fox_blaster.c
endif
ifeq ($(NDS_IMPORT_BATTLESHIP_EFFECT_MANAGER),1)
CFILES += battleship_efmanager.c
endif
# efground.c owns every VS stage's background actors (Lakitu, Sector rocket /
# ship, Jungle bird, Zebes Ridley / ship, Yoster clouds and birds, Bronto Burt
# / Dedede, Saffron birds); grcommonsetup.c:34 summons them at every stage
# start, so this TU is unconditional like the grounds that stage them.
CFILES += battleship_efground.c
ifeq ($(NDS_R2_PARTICLE_RUNTIME),1)
ifneq ($(NDS_IMPORT_BATTLESHIP_EFFECT_MANAGER),1)
$(error NDS_R2_PARTICLE_RUNTIME=1 requires NDS_IMPORT_BATTLESHIP_EFFECT_MANAGER=1: ef/efdisplay.c owns the efcommon bank request)
endif
# The generated pack is in the build now (src/nds/generated/
# nds_particle_banks.generated.inc, verified byte-reproducing by
# check-nds-particle-banks.ps1), so nds_particle_banks_placeholder.c is gone --
# both it and this line said to delete it with the commit that lands the
# generated data, and that commit is this one. Its weak empty definitions had
# also drifted: it spelled the count NDS_PARTICLE_SCRIPT_IDS and the sentinel
# NDS_PARTICLE_UNPACKED_OFFSET, neither of which the current generator emits.
# nds_particle_banks.c is that replacement -- the .inc is data with no
# translation unit of its own, so without it the link fails on
# gNdsParticleScriptBank and friends.
CFILES += nds_particle_banks.c
# guMtxIdentF, called by lbParticleGetPosVelDObj. Nothing else in the build
# defines it, so it is scoped to the flag that needs it.
CFILES += battleship_libultra_gu_mtxutil.c
CFILES += battleship_lbparticle.c
# DS-native FireGrind. The accepted path is on by default; setting
# NDS_R2_FIREGRIND_NATIVE=0 retains no-op stubs for control/fallback builds.
# Lives with the particle runtime because its draw integration is inside
# lbParticleDrawTextures.
CFILES += nds_firegrind.c
endif
ifeq ($(NDS_IMPORT_BATTLESHIP_FOX_REFLECTOR),1)
CFILES += battleship_fox_reflector.c
endif
# Fox's blaster model part, as data. Unconditional because the module's whole
# body is inside #if NDS_R2_FOX_GUN_OVERLAY -- at 0 it compiles to nothing and
# costs no ROM, which is cheaper to reason about than a second flag in the file
# list that has to stay in step with the one in the sources.
CFILES += nds_fox_gun.c
# P2-7 save data: the FAT-backed SRAM image and the transcribed lbbackup.c;
# P7's progress block in the same image (docs/P7/P7_Implementation_Plan.md U1),
# its unlock registry's evaluator (U3; the 1P manager's seam reads it) and the
# wide VS records beside the save (U2) and the VS Record screen's numbers
# over them (U9).
CFILES += nds_backup.c battleship_lbbackup.c nds_p7_progress.c nds_p7_unlocks.c nds_p7_records.c nds_p7_vsrecord.c
ifeq ($(NDS_IMPORT_BATTLESHIP_FT_PUBLIC),1)
CFILES += battleship_ftpublic.c
endif
ifneq ($(filter 1,$(NDS_IMPORT_BATTLESHIP_MARIO_SPECIAL_HI) $(NDS_IMPORT_BATTLESHIP_FOX_SPECIAL_HI)),)
CFILES += battleship_ftcommon_fallspecial.c
endif
ifeq ($(NDS_IMPORT_BATTLESHIP_MARIO_SPECIAL_HI),1)
CFILES += battleship_mario_special_hi.c
endif
ifeq ($(NDS_IMPORT_BATTLESHIP_MARIO_SPECIAL_LW),1)
CFILES += battleship_mario_special_lw.c
endif
ifeq ($(NDS_IMPORT_BATTLESHIP_FOX_SPECIAL_HI),1)
CFILES += battleship_fox_special_hi.c
endif
CFILES += battleship_ftmain.c
# grModelSetupGroundDObjs is shared stage-model setup, not Mushroom Kingdom
# specific: Congo Jungle builds its barrel cannon with it too
# (grjungle.c:119). The gate is the OR of everyone who needs it, so the
# translation unit is linked once and no stage carries a private copy.
ifeq ($(NDS_ENABLE_INISHIE_SOURCE_SCALE_SETUP)$(NDS_P2_STAGE_JUNGLE)$(NDS_P2_STAGE_INISHIE)$(NDS_P2_STAGE_SECTOR),0000)
else
CFILES += battleship_grmodelsetup.c
endif
CPPFILES :=
SFILES := coroutine_arm.s
ifeq ($(NDS_TASK9_FLOAT_PHASE2),1)
SFILES += nds_task9_float_phase2.s
endif
ifeq ($(NDS_TASK16_FLOAT_COMPARE),1)
SFILES += nds_task16_float_compare.s
endif
ifeq ($(NDS_TASK16_FLOAT_I2F),1)
SFILES += nds_task16_float_i2f.s
endif
ifeq ($(NDS_P2_FLOAT_CONV),1)
CFILES += nds_float_conv.c
endif
ifeq ($(NDS_TASK16_FLOAT_ADDSUB),1)
SFILES += nds_task16_float_addsub.s
endif
ifeq ($(NDS_FREEZE_DIAGNOSTICS),1)
CFILES += nds_freeze_diagnostics.c
SFILES += nds_freeze_diagnostics_irq.s
endif
ifeq ($(NDS_P2_UI_KIT),1)
CFILES += nds_ui_kit.c
endif
ifeq ($(NDS_P2_MENU_SHELL),1)
CFILES += nds_menu_shell.c
endif
CFILES += nds_p4.c
# Every content's generated data in one TU, plus the native code the contents
# run (src/port/nds_p4_<name>.c, NDS_P4_CODE_NAMES).
ifeq ($(NDS_P4),1)
CFILES += nds_p4_data.c \
	$(foreach name,$(NDS_P4_CODE_NAMES),nds_p4_$(name).c)
# The jab and rapid-jab files' copy a content runs (Remix's jab tables).
CFILES += battleship_ftcommon_jab_p4.c
endif
# P5: the Remix stages' ground routines and hazards.
ifeq ($(NDS_P5),1)
CFILES += nds_p5_stages.c
endif

export LD := $(CC)
export OFILES := \
	$(if $(filter 1,$(NDS_TASK9_FLOAT_ITCM)),$(NDS_TASK9_FLOAT_ITCM_OFILES)) \
	$(NDS_TASK37_ITCM_OFILES) \
	$(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)
export NDS_MPPROCESS_STRICT_OFILES := $(NDS_MPPROCESS_SOURCE_CFILES:.c=.o) \
	battleship_mpprocess_live_bridge.o
# libnds/Calico are system headers: -isystem keeps -Wundef (and the other -Wall/
# -Wextra diagnostics) firing on project sources while silencing the proven
# `__ASSEMBLER__`/`__cplusplus` -Wundef flood from calico.h/types.h, which
# repeats once per TU that includes nds.h (68 hits in the 37 MB reference log).
# Ordering is stable: project and generated names are distinct from libnds, so
# searching libnds as a system path changes no resolution, only the warning class.
export INCLUDE := $(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
	$(foreach dir,$(LIBDIRS),-isystem $(dir)/include) \
	-I$(CURDIR)/$(BUILD)
export LIBPATHS := $(foreach dir,$(LIBDIRS),-L$(dir)/lib)
export PROJECT_ROOT := $(PROJECT_ROOT)
export NITROFS_DIR := $(NITROFS_DIR)
export BATTLESHIP_O2R := $(BATTLESHIP_O2R)
export BATTLESHIP_RELOCDATA := $(BATTLESHIP_RELOCDATA)
# P2-4s1: generators gate Yoster rows on this env var, so export it to every recipe (ui kit + particle banks).
export NDS_P2_STAGE_YOSTER := $(NDS_P2_STAGE_YOSTER)
export NDS_P2_STAGE_CASTLE := $(NDS_P2_STAGE_CASTLE)
export NDS_P2_STAGE_JUNGLE := $(NDS_P2_STAGE_JUNGLE)
export NDS_P2_STAGE_ZEBES := $(NDS_P2_STAGE_ZEBES)
export NDS_P2_STAGE_HYRULE := $(NDS_P2_STAGE_HYRULE)
export NDS_P2_STAGE_YAMABUKI := $(NDS_P2_STAGE_YAMABUKI)
export NDS_P2_STAGE_INISHIE := $(NDS_P2_STAGE_INISHIE)
export NDS_P2_STAGE_SECTOR := $(NDS_P2_STAGE_SECTOR)

NDS_OPENING_ROOM_RELOC_FILES := \
	reloc_movies/MVCommon \
	reloc_transitions/MVOpeningRoomTransition \
	reloc_movies/MVOpeningRoomScene1 \
	reloc_movies/MVOpeningRoomScene2 \
	reloc_movies/MVOpeningRoomScene3 \
	reloc_movies/MVOpeningRoomScene4 \
	reloc_movies/MVOpeningRunCrash \
	reloc_movies/MVOpeningRoomWallpaper

NDS_OPENING_PORTRAITS_RELOC_FILES := \
	reloc_movies/MVOpeningPortraitsSet1 \
	reloc_movies/MVOpeningPortraitsSet2

NDS_OPENING_MARIO_RELOC_FILES := \
	reloc_interface/IFCommonAnnounceCommon \
	reloc_movies/MVOpeningCommon

NDS_OPENING_ACTION_RELOC_FILES := \
	reloc_movies/MVOpeningRun \
	reloc_movies/MVOpeningYamabuki \
	reloc_movies/MVOpeningSector

NDS_TITLE_RELOC_FILES := \
	reloc_menus/MNTitle \
	reloc_menus/MNTitleFireAnim

NDS_VS_MODE_RELOC_FILES := \
	reloc_menus/MNCommon \
	reloc_menus/MNVSMode

NDS_PLAYERS_VS_RELOC_FILES := \
	reloc_menus/MNPlayersCommon \
	reloc_menus/MNCommon \
	reloc_fighters_common/FTEmblemSprites \
	reloc_menus/MNSelectCommon \
	reloc_menus/MNPlayersGameModes \
	reloc_menus/MNPlayersPortraits \
	reloc_menus/MNPlayersSpotlight

