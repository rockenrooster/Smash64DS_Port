# The inner make: the build configuration header (nds_build_config.h, every
# knob a source reads) and the build revision.
# Included in place from the Makefile (P8 G); its text is unchanged.

$(NDS_BUILD_CONFIG): FORCE
	@tmp="$@.tmp"; \
	{ \
		echo '#ifndef NDS_BUILD_CONFIG_H'; \
		echo '#define NDS_BUILD_CONFIG_H'; \
		echo '#define NDS_DEV_LIVE_INPUT_PREVIEW $(NDS_DEV_LIVE_INPUT_PREVIEW)'; \
		echo '#define NDS_HARNESS_FAST_LOGIC $(NDS_HARNESS_FAST_LOGIC)'; \
		echo '#define NDS_RENDERER_HW_TRIANGLES $(NDS_RENDERER_HW_TRIANGLES)'; \
		echo '#define NDS_RENDERER_PROFILE_LEVEL $(NDS_RENDERER_PROFILE_LEVEL)'; \
		echo '#define NDS_SHIP_TELEMETRY $(NDS_SHIP_TELEMETRY)'; \
		echo '#define NDS_TICK_HUD $(NDS_TICK_HUD)'; \
		echo '#define NDS_P2_COMPACT_GROUND_MAPS $(NDS_P2_COMPACT_GROUND_MAPS)'; \
		echo '#define NDS_FTR_LEAN_KTIME $(NDS_FTR_LEAN_KTIME)'; \
		echo '#define NDS_FTR_LEAN_RELAXED $(NDS_FTR_LEAN_RELAXED)'; \
		echo '#define NDS_RENDERER_M2_DETAILED_LEDGER $(NDS_RENDERER_M2_DETAILED_LEDGER)'; \
		echo '#define NDS_NATIVE_STAGE_GENERATED_SEGMENT0_ENABLE $(NDS_NATIVE_STAGE_GENERATED_SEGMENT0_ENABLE)'; \
		echo '#define NDS_TASK36_HW_COMPOSE $(NDS_TASK36_HW_COMPOSE)'; \
		$(if $(NDS_TASK36_RIGID_BINDING_MASK),echo '#define NDS_RENDERER_TASK36_RIGID_BINDING_MASK $(NDS_TASK36_RIGID_BINDING_MASK)'; ,) \
		echo '#define NDS_R2_STAGE_ROUTE_PROBE $(NDS_R2_STAGE_ROUTE_PROBE)'; \
		echo '#define NDS_TASK51_STAGE_NATIVE $(NDS_TASK51_STAGE_NATIVE)'; \
		echo '#define NDS_DREAMLAND_DS_MESH $(NDS_DREAMLAND_DS_MESH)'; \
		echo '#define NDS_TASK56_FIGHTER_PRIMITIVES $(NDS_TASK56_FIGHTER_PRIMITIVES)'; \
		echo '#define NDS_TASK44_STAGE_STEADY $(NDS_TASK44_STAGE_STEADY)'; \
		echo '#define NDS_TASK37_PROFILE $(NDS_TASK37_PROFILE)'; \
		echo '#define NDS_TASK37_ITCM_LEAVES $(NDS_TASK37_ITCM_LEAVES)'; \
		echo '#define NDS_TASK37_ITCM_PORT $(NDS_TASK37_ITCM_PORT)'; \
		echo '#define NDS_TASK37_LAYOUT_PROBE $(NDS_TASK37_LAYOUT_PROBE)'; \
		echo '#define NDS_TASK37_PROFILE_START $(NDS_TASK37_PROFILE_START)u'; \
		echo '#define NDS_TASK37_PROFILE_FRAMES $(NDS_TASK37_PROFILE_FRAMES)u'; \
		echo '#define NDS_TASK37_PROFILE_PER_FRAME_REGION $(NDS_TASK37_PROFILE_PER_FRAME_REGION)'; \
		echo '#define NDS_TASK37_PROFILE_RESULTS $(NDS_TASK37_PROFILE_RESULTS)'; \
		echo '#define NDS_LAB_JOINT_CAP $(NDS_LAB_JOINT_CAP)u'; \
		echo '#define NDS_LAB_POSE_JOINT_CAP $(NDS_LAB_POSE_JOINT_CAP)u'; \
		echo '#define NDS_FTPARTS_FLAT_SLOTS $(NDS_FTPARTS_FLAT_SLOTS)u'; \
		echo '#define NDS_FTPARTS_FLAT_MAX $(NDS_FTPARTS_FLAT_MAX)u'; \
		echo '#define NDS_RENDERER_SCREEN_SPACE_CENSUS $(NDS_RENDERER_SCREEN_SPACE_CENSUS)'; \
		echo '#define NDS_R2_ANIM_CACHE $(NDS_R2_ANIM_CACHE)'; \
		echo '#define NDS_R2_BATTLEPACK $(NDS_R2_BATTLEPACK)'; \
		echo '#define NDS_R2_BATTLEPACK_KEEP_CACHE $(NDS_R2_BATTLEPACK_KEEP_CACHE)'; \
		echo '#define NDS_R2_FTANIM_TRACK_DISPATCH $(NDS_R2_FTANIM_TRACK_DISPATCH)'; \
		echo '#define NDS_FT_POSE $(NDS_FT_POSE)'; \
		echo '#define NDS_FT_POSE_HOLD $(NDS_FT_POSE_HOLD)'; \
		echo '#define NDS_FT_POSE_ORACLE $(NDS_FT_POSE_ORACLE)'; \
		echo '#define NDS_FOX_BLASTER_BORE_OFFSET_Y $(NDS_FOX_BLASTER_BORE_OFFSET_Y)'; \
		echo "#define NDS_R2_BATTLEPACK_BLOB_BYTES $$(test -f '$(NDS_BATTLEPACK_BLOB)' && wc -c < '$(NDS_BATTLEPACK_BLOB)' || echo 0)u"; \
		echo '#define NDS_R2_ANIM_CUT_ROUTE $(NDS_R2_ANIM_CUT_ROUTE)'; \
		echo '#define NDS_R2_STRIP_ROUTE $(NDS_R2_STRIP_ROUTE)'; \
		echo '#define NDS_R2_FLASH_PROBE $(NDS_R2_FLASH_PROBE)'; \
		echo '#define NDS_R2_CUBIC_FIXED $(NDS_R2_CUBIC_FIXED)'; \
		echo '#define NDS_R2_BOTH_CPU $(NDS_R2_BOTH_CPU)'; \
		echo '#define NDS_P2_FOUR_CPU_STRESS $(NDS_P2_FOUR_CPU_STRESS)'; \
		echo '#define NDS_NET_LAB_INPUT $(NDS_NET_LAB_INPUT)'; \
		echo '#define NDS_NET_LAB_LOBBY $(NDS_NET_LAB_LOBBY)'; \
		echo '#define NDS_NET_LAB_DROP $(NDS_NET_LAB_DROP)'; \
		echo '#define NDS_NET_LAB_SWEEP $(NDS_NET_LAB_SWEEP)'; \
		echo '#define NDS_NET_LAB_SWEEP_REPRO $(NDS_NET_LAB_SWEEP_REPRO)'; \
		echo '#define NDS_NET_LAB_SOAK $(NDS_NET_LAB_SOAK)'; \
		echo '#define NDS_P2_FOUR_CPU_ROSTER $(NDS_P2_FOUR_CPU_ROSTER)'; \
		echo '#define NDS_P2_FOUR_CPU_KIND0 $(NDS_P2_FOUR_CPU_KIND0)'; \
		echo '#define NDS_P2_FOUR_CPU_KIND1 $(NDS_P2_FOUR_CPU_KIND1)'; \
		echo '#define NDS_P2_FOUR_CPU_KIND2 $(NDS_P2_FOUR_CPU_KIND2)'; \
		echo '#define NDS_P2_FOUR_CPU_KIND3 $(NDS_P2_FOUR_CPU_KIND3)'; \
		$(if $(strip $(NDS_LAB_FOURCPU_TWO)),echo '#define NDS_LAB_FOURCPU_TWO 1';) \
		$(if $(strip $(NDS_LAB_FOURCPU_SWEEP)),echo '#define NDS_LAB_FOURCPU_SWEEP 1';) \
		$(if $(strip $(NDS_LAB_FOURCPU_SWEEP)$(NDS_LAB_FOURCPU_WORDS)),echo '#define NDS_LAB_FOURCPU_WORDS 1';) \
		$(if $(strip $(NDS_LAB_FOURCPU_SWEEP_GKIND)),echo '#define NDS_LAB_FOURCPU_SWEEP_GKIND $(NDS_LAB_FOURCPU_SWEEP_GKIND)u';) \
		$(if $(strip $(NDS_LAB_FOURCPU_SWEEP_KINDS)),echo '#define NDS_LAB_FOURCPU_SWEEP_KINDS $(NDS_LAB_FOURCPU_SWEEP_KINDS)u';) \
		$(if $(strip $(NDS_LAB_FOURCPU_SWEEP_P4)),echo '#define NDS_LAB_FOURCPU_SWEEP_P4 $(NDS_LAB_FOURCPU_SWEEP_P4)u';) \
		$(if $(strip $(NDS_LAB_LEAN_ROOT_CENSUS)),echo '#define NDS_LAB_LEAN_ROOT_CENSUS 1';) \
		$(if $(strip $(NDS_LAB_TEXMISS)),echo '#define NDS_LAB_TEXMISS 1';) \
		echo '#define NDS_P2_LUIGI $(NDS_P2_LUIGI)'; \
		echo '#define NDS_NATIVE_OWNER_IMAGE_LUIGI $(NDS_NATIVE_OWNER_IMAGE_LUIGI)'; \
		echo '#define NDS_NATIVE_OWNER_IMAGE_DONKEY $(NDS_NATIVE_OWNER_IMAGE_DONKEY)'; \
		echo '#define NDS_P2_DONKEY $(NDS_P2_DONKEY)'; \
		echo '#define NDS_P2_CAPTAIN $(NDS_P2_CAPTAIN)'; \
		echo '#define NDS_P2_SAMUS $(NDS_P2_SAMUS)'; \
		echo '#define NDS_P2_LINK $(NDS_P2_LINK)'; \
		echo '#define NDS_P2_PIKACHU $(NDS_P2_PIKACHU)'; \
		echo '#define NDS_P2_YOSHI $(NDS_P2_YOSHI)'; \
		echo '#define NDS_P2_STAGE_YOSTER $(NDS_P2_STAGE_YOSTER)'; \
		echo '#define NDS_P2_STAGE_CASTLE $(NDS_P2_STAGE_CASTLE)'; \
		echo '#define NDS_P2_STAGE_JUNGLE $(NDS_P2_STAGE_JUNGLE)'; \
		echo '#define NDS_P2_STAGE_ZEBES $(NDS_P2_STAGE_ZEBES)'; \
		echo '#define NDS_P2_STAGE_HYRULE $(NDS_P2_STAGE_HYRULE)'; \
		echo '#define NDS_P2_STAGE_YAMABUKI $(NDS_P2_STAGE_YAMABUKI)'; \
		echo '#define NDS_P2_STAGE_INISHIE $(NDS_P2_STAGE_INISHIE)'; \
		echo '#define NDS_P2_STAGE_SECTOR $(NDS_P2_STAGE_SECTOR)'; \
		echo '#define NDS_P2_NESS $(NDS_P2_NESS)'; \
		echo '#define NDS_P2_PURIN $(NDS_P2_PURIN)'; \
		echo '#define NDS_P2_KIRBY $(NDS_P2_KIRBY)'; \
		echo '#define NDS_P2_GDONKEY $(NDS_P2_GDONKEY)'; \
		echo '#define NDS_P2_MMARIO $(NDS_P2_MMARIO)'; \
		echo '#define NDS_P2_NMARIO $(NDS_P2_NMARIO)'; \
		echo '#define NDS_P2_NFOX $(NDS_P2_NFOX)'; \
		echo '#define NDS_P4 $(NDS_P4)'; \
		$(foreach row,$(NDS_P4_REGISTRY),echo '#define NDS_P4_$(call nds_p4_field,$(row),2) $(NDS_P4_$(call nds_p4_field,$(row),2))';) \
		echo '#define NDS_P5 $(NDS_P5)'; \
		echo '#define NDS_P2_NDONKEY $(NDS_P2_NDONKEY)'; \
		echo '#define NDS_P2_NSAMUS $(NDS_P2_NSAMUS)'; \
		echo '#define NDS_P2_NLUIGI $(NDS_P2_NLUIGI)'; \
		echo '#define NDS_P2_NLINK $(NDS_P2_NLINK)'; \
		echo '#define NDS_P2_NYOSHI $(NDS_P2_NYOSHI)'; \
		echo '#define NDS_P2_NCAPTAIN $(NDS_P2_NCAPTAIN)'; \
		echo '#define NDS_P2_NKIRBY $(NDS_P2_NKIRBY)'; \
		echo '#define NDS_P2_NPIKACHU $(NDS_P2_NPIKACHU)'; \
		echo '#define NDS_P2_NPURIN $(NDS_P2_NPURIN)'; \
		echo '#define NDS_P2_NNESS $(NDS_P2_NNESS)'; \
		echo '#define NDS_P2_ITEM_CORE $(NDS_P2_ITEM_CORE)'; \
		echo '#define NDS_P2_1P_GAME $(NDS_P2_1P_GAME)'; \
		$(foreach venue,$(NDS_P2_1P_STAGE_FLAGS),echo '#define NDS_P2_STAGE_$(venue) $(NDS_P2_1P_GAME)';) \
		echo '#define NDS_P2_COMPACT_BATTLE_FIGHTERS $(NDS_P2_COMPACT_BATTLE_FIGHTERS)'; \
		echo '#define NDS_NATIVE_OWNER_IMAGE_CAPTAIN $(NDS_NATIVE_OWNER_IMAGE_CAPTAIN)'; \
		echo '#define NDS_NATIVE_OWNER_IMAGE_SAMUS $(NDS_NATIVE_OWNER_IMAGE_SAMUS)'; \
		echo '#define NDS_NATIVE_OWNER_IMAGE_LINK $(NDS_NATIVE_OWNER_IMAGE_LINK)'; \
		echo '#define NDS_NATIVE_OWNER_IMAGE_PIKACHU $(NDS_NATIVE_OWNER_IMAGE_PIKACHU)'; \
		echo '#define NDS_NATIVE_OWNER_IMAGE_YOSHI $(NDS_NATIVE_OWNER_IMAGE_YOSHI)'; \
		echo '#define NDS_NATIVE_OWNER_IMAGE_NESS $(NDS_NATIVE_OWNER_IMAGE_NESS)'; \
		echo '#define NDS_NATIVE_OWNER_IMAGE_PURIN $(NDS_NATIVE_OWNER_IMAGE_PURIN)'; \
		echo '#define NDS_NATIVE_OWNER_IMAGE_KIRBY $(NDS_NATIVE_OWNER_IMAGE_KIRBY)'; \
		echo '#define NDS_NATIVE_OWNER_IMAGE_MMARIO $(NDS_NATIVE_OWNER_IMAGE_MMARIO)'; \
		echo '#define NDS_NATIVE_OWNER_IMAGE_NMARIO $(NDS_NATIVE_OWNER_IMAGE_NMARIO)'; \
		echo '#define NDS_NATIVE_OWNER_IMAGE_NFOX $(NDS_NATIVE_OWNER_IMAGE_NFOX)'; \
		echo '#define NDS_NATIVE_OWNER_IMAGE_MARIOFOX $(NDS_NATIVE_OWNER_IMAGE_MARIOFOX)'; \
		echo '#define NDS_NATIVE_OWNER_IMAGE_NDONKEY $(NDS_NATIVE_OWNER_IMAGE_NDONKEY)'; \
		echo '#define NDS_NATIVE_OWNER_IMAGE_NSAMUS $(NDS_NATIVE_OWNER_IMAGE_NSAMUS)'; \
		echo '#define NDS_NATIVE_OWNER_IMAGE_NLINK $(NDS_NATIVE_OWNER_IMAGE_NLINK)'; \
		echo '#define NDS_NATIVE_OWNER_IMAGE_NYOSHI $(NDS_NATIVE_OWNER_IMAGE_NYOSHI)'; \
		echo '#define NDS_NATIVE_OWNER_IMAGE_NCAPTAIN $(NDS_NATIVE_OWNER_IMAGE_NCAPTAIN)'; \
		echo '#define NDS_NATIVE_OWNER_IMAGE_NKIRBY $(NDS_NATIVE_OWNER_IMAGE_NKIRBY)'; \
		echo '#define NDS_NATIVE_OWNER_IMAGE_NPIKACHU $(NDS_NATIVE_OWNER_IMAGE_NPIKACHU)'; \
		echo '#define NDS_NATIVE_OWNER_IMAGE_NPURIN $(NDS_NATIVE_OWNER_IMAGE_NPURIN)'; \
		echo '#define NDS_NATIVE_OWNER_IMAGE_NNESS $(NDS_NATIVE_OWNER_IMAGE_NNESS)'; \
		echo '#define NDS_NATIVE_OWNER_IMAGE_GDONKEY $(NDS_NATIVE_OWNER_IMAGE_GDONKEY)'; \
		echo '#define NDS_NATIVE_OWNER_IMAGE_BOSS $(NDS_NATIVE_OWNER_IMAGE_BOSS)'; \
		echo '#define NDS_P2_NESS_VFX_PROOF $(NDS_P2_NESS_VFX_PROOF)'; \
		echo '#define NDS_P2_YOSHI_BUG_PROOF $(NDS_P2_YOSHI_BUG_PROOF)'; \
		echo '#define NDS_P2_KIRBY_COPYLINK_PROOF $(NDS_P2_KIRBY_COPYLINK_PROOF)'; \
		echo '#define NDS_R2_SOAK_MATCH_MINUTES $(NDS_R2_SOAK_MATCH_MINUTES)'; \
		echo '#define NDS_R2_UNLIT_VERTEX_EPOCH $(NDS_R2_UNLIT_VERTEX_EPOCH)'; \
		echo '#define NDS_TASK106_UPDATES_PER_PRESENT $(NDS_TASK106_UPDATES_PER_PRESENT)u'; \
		echo '#define NDS_R2_FTR_CONTRACT_CENSUS $(NDS_R2_FTR_CONTRACT_CENSUS)'; \
		echo '#define NDS_R2_FTR_DRAW_MEMO $(NDS_R2_FTR_DRAW_MEMO)u'; \
		echo '#define NDS_R2_SCENE_LOOP_WALK $(NDS_R2_SCENE_LOOP_WALK)u'; \
		echo '#define NDS_P2_UI_KIT $(NDS_P2_UI_KIT)'; \
		echo '#define NDS_P2_MENU_SHELL $(NDS_P2_MENU_SHELL)'; \
		echo '#define NDS_P2_MENU_WALK $(NDS_P2_MENU_WALK)u'; \
		echo '#define NDS_1P_INTRO_BAKE $(NDS_1P_INTRO_BAKE)'; \
		echo '#define NDS_1P_WALK_START_STAGE ($(NDS_1P_WALK_START_STAGE))'; \
		echo '#define NDS_1P_PLAYTEST_START_STAGE ($(NDS_1P_PLAYTEST_START_STAGE))'; \
		echo '#define NDS_BOOT_DIAG_TEXT $(NDS_BOOT_DIAG_TEXT)'; \
		echo '#define NDS_R2_PATH $(NDS_R2_PATH)'; \
		echo '#define NDS_R2_STAGE_DIRECT $(NDS_R2_STAGE_DIRECT)'; \
		echo '#define NDS_R2_FIGHTER_SHADE_PROOF $(NDS_R2_FIGHTER_SHADE_PROOF)'; \
		echo '#define NDS_R2_FIGHTER_RUN_PROOF $(NDS_R2_FIGHTER_RUN_PROOF)'; \
		echo '#define NDS_R2_FIGHTER_HW_MTX $(NDS_R2_FIGHTER_HW_MTX)'; \
		echo '#define NDS_R2_STAGE_VALIDATE_STRIDE $(NDS_R2_STAGE_VALIDATE_STRIDE)'; \
		echo '#define NDS_R2_FIGHTER_HW_LIGHT $(NDS_R2_FIGHTER_HW_LIGHT)'; \
		echo '#define NDS_TICK_HUD_DRAW $(NDS_TICK_HUD_DRAW)'; \
		echo '#define NDS_TICK_HUD_WINDOW $(NDS_TICK_HUD_WINDOW)u'; \
		echo '#define NDS_BATTLE_FPS_HUD_DRAW $(NDS_BATTLE_FPS_HUD_DRAW)'; \
		echo '#define NDS_TICK_HUD_SPANS_DEFAULT $(NDS_TICK_HUD_SPANS_DEFAULT)'; \
		echo '#define NDS_DIAG_COUNTERS $(NDS_DIAG_COUNTERS)'; \
		echo '#if NDS_DIAG_COUNTERS'; \
		echo '#define NDS_DIAG(...) do { __VA_ARGS__; } while (0)'; \
		echo '#else'; \
		echo '#define NDS_DIAG(...) ((void)0)'; \
		echo '#endif'; \
		echo '#define NDS_R2_DRAW_SUPPRESS_MASK $(NDS_R2_DRAW_SUPPRESS_MASK)'; \
		echo '#define NDS_R2_FIGHTER_PACKET $(NDS_R2_FIGHTER_PACKET)'; \
		echo '#define NDS_RENDERER_BENCHMARK_MODE $(NDS_RENDERER_BENCHMARK_MODE)'; \
		echo '#define NDS_RENDERER_FAST_RUN_DEFAULT $(NDS_RENDERER_FAST_RUN_DEFAULT)'; \
		echo '#define NDS_FIGHTER_LEGACY_EXEC $(NDS_FIGHTER_LEGACY_EXEC)'; \
		echo '#define NDS_RENDERER_BATTLE_STATIC_TEXTURE_DEFAULT $(NDS_RENDERER_BATTLE_STATIC_TEXTURE_DEFAULT)'; \
		echo '#define NDS_BUILD_HARNESS_VARIANT "$(NDS_DEV_SCENE_HARNESS)"'; \
		echo '#define NDS_FREEZE_DIAGNOSTICS $(NDS_FREEZE_DIAGNOSTICS)'; \
		echo '#define NDS_PRESENT_LEAD $(NDS_PRESENT_LEAD)u'; \
		echo '#define NDS_FIGHTER_ANIM_AUDIT $(NDS_FIGHTER_ANIM_AUDIT)'; \
		echo '#define NDS_R2_PARTICLE_DRAW $(NDS_R2_PARTICLE_DRAW)'; \
		echo '#define NDS_R2_FIREBALL_QUAD $(NDS_R2_FIREBALL_QUAD)'; \
		echo '#define NDS_R2_FOX_BLASTER_GLOW_AOT $(NDS_R2_FOX_BLASTER_GLOW_AOT)'; \
		echo '#define NDS_R2_FOX_GUN_OVERLAY $(NDS_R2_FOX_GUN_OVERLAY)'; \
		echo '#define NDS_R2_FIREGRIND_NATIVE $(NDS_R2_FIREGRIND_NATIVE)'; \
		echo '#define NDS_R2_FIREBALL_MAP_COLL_SCALE $(NDS_R2_FIREBALL_MAP_COLL_SCALE)F'; \
		echo '#define NDS_R2_IMPACT_WAVE_NATIVE $(NDS_R2_IMPACT_WAVE_NATIVE)'; \
		echo '#define NDS_R2_REBIRTH_HALO_GX_GROUP_MASK $(NDS_R2_REBIRTH_HALO_GX_GROUP_MASK)'; \
		echo '#define NDS_R2_FOX_CPU_DEFAULT $(NDS_R2_FOX_CPU_DEFAULT)'; \
		echo '#define NDS_DEMO_FOX_CPU_LADDER $(NDS_DEMO_FOX_CPU_LADDER)'; \
		echo '#define NDS_R2_COLLISION_L7_ORACLE $(NDS_R2_COLLISION_L7_ORACLE)'; \
		echo '#define NDS_R2_COLLISION_FIXED $(NDS_R2_COLLISION_FIXED)'; \
		echo '#define NDS_P2_HURTBOX_REJECT $(NDS_P2_HURTBOX_REJECT)'; \
		echo '#define NDS_P2_JOINT_RESIDENT $(NDS_P2_JOINT_RESIDENT)'; \
		echo '#define NDS_FAST_MEM $(NDS_FAST_MEM)'; \
		echo '#define NDS_FTR_LEAN_PHASE_TICKS $(NDS_FTR_LEAN_PHASE_TICKS)'; \
		echo '#define NDS_CAMERA_FRAME_WITNESS $(NDS_CAMERA_FRAME_WITNESS)'; \
		echo '#define NDS_P2_ARM9_WRAM $(NDS_P2_ARM9_WRAM)'; \
		echo '#define NDS_R2_COLLISION_FIXED_DISPATCH $(NDS_R2_COLLISION_FIXED_DISPATCH)u'; \
		echo '#define NDS_R2_SIM_MAC_SHADOW $(NDS_R2_SIM_MAC_SHADOW)'; \
		echo '#define NDS_R2_CFX_HWMATH $(NDS_R2_CFX_HWMATH)'; \
		echo '#define NDS_R2_HWMATH_BENCH $(NDS_R2_HWMATH_BENCH)'; \
		echo '#define NDS_R2_HWMATH_ROUTE $(NDS_R2_HWMATH_ROUTE)'; \
		echo '#define NDS_TASK39_FX_SPRITES $(NDS_TASK39_FX_SPRITES)'; \
		echo '#define NDS_R2_PARTICLE_RUNTIME $(NDS_R2_PARTICLE_RUNTIME)'; \
		echo '#define NDS_IMPORT_BATTLESHIP_FTMANAGER $(NDS_IMPORT_BATTLESHIP_FTMANAGER)'; \
		echo '#define NDS_IMPORT_BATTLESHIP_BATTLE_PLAYABLE $(NDS_IMPORT_BATTLESHIP_BATTLE_PLAYABLE)'; \
		echo '#define NDS_IMPORT_BATTLESHIP_FT_PUBLIC $(NDS_IMPORT_BATTLESHIP_FT_PUBLIC)'; \
		echo '#define NDS_R2_SOURCE_EFFECTS_PARTICLE $(NDS_R2_SOURCE_EFFECTS_PARTICLE)'; \
		echo '#define NDS_R2_EFFECT_POOL $(NDS_R2_EFFECT_POOL)'; \
		echo '#define NDS_IMPORT_BATTLESHIP_AUDIO_FGM $(NDS_IMPORT_BATTLESHIP_AUDIO_FGM)'; \
		echo '#endif'; \
	} > "$$tmp"; \
	if test -f "$@" && cmp -s "$$tmp" "$@"; then rm "$$tmp"; else mv "$$tmp" "$@"; fi

# The git revision changes on every commit but is read by exactly one TU
# (nds_platform.c). It used to live in
# nds_build_config.h, so every commit rewrote that header and the explicit
# $(OFILES): $(NDS_BUILD_CONFIG) edge recompiled every C TU. It now lives
# here, in a header only its owners include and depend on; the config header
# stays stable across commits. Same atomic write-if-changed shape, so an
# unchanged revision keeps its mtime and rebuilds only its owner.
$(NDS_BUILD_REVISION): FORCE
	@tmp="$@.tmp"; \
	{ \
		echo '#ifndef NDS_BUILD_REVISION_H'; \
		echo '#define NDS_BUILD_REVISION_H'; \
		echo '#define NDS_TASK10_GIT_SHORT "$(NDS_TASK10_GIT_SHORT)"'; \
		echo '#endif'; \
	} > "$$tmp"; \
	if test -f "$@" && cmp -s "$$tmp" "$@"; then rm "$$tmp"; else mv "$$tmp" "$@"; fi

$(NDS_SCENE_HARNESS_CONFIG): FORCE
	@tmp="$@.tmp"; \
	{ \
		echo '#ifndef NDS_SCENE_HARNESS_CONFIG_H'; \
		echo '#define NDS_SCENE_HARNESS_CONFIG_H'; \
		echo '#define NDS_DEV_SCENE_HARNESS $(NDS_DEV_SCENE_HARNESS_ID)'; \
		echo '#define NDS_ENABLE_INISHIE_SOURCE_SCALE_SETUP $(NDS_ENABLE_INISHIE_SOURCE_SCALE_SETUP)'; \
		echo '#endif'; \
	} > "$$tmp"; \
	if test -f "$@" && cmp -s "$$tmp" "$@"; then rm "$$tmp"; else mv "$$tmp" "$@"; fi

$(NDS_TASK32_DRAW_HOT_FRAGMENT): FORCE
	@tmp="$@.tmp"; \
	{ \
		echo '*scene_backend.o(.text.ndsRendererAdapterFindDObjWorldMatrix)'; \
		echo '*scene_backend.o(.text.ndsRendererAdapterBuildDObjWorldMatrix)'; \
		echo '*nds_renderer.o(.text.ndsRendererNativeStagePrepareRun.constprop.0)'; \
		echo '*nds_renderer.o(.text.ndsRendererMtxMul20p12)'; \
		echo '*nds_renderer.o(.text.ndsRendererMtxMulAffine20p12)'; \
		echo '*nds_renderer.o(.text.ndsRendererMtxLoadN64ToDS20p12)'; \
		echo '*nds_renderer.o(.text.ndsRendererLoadHardwareMatrixPair.isra.0)'; \
		if test "$(NDS_TASK36_HW_COMPOSE)" != 2; then \
			echo '*nds_renderer.o(.text.ndsRendererCommitNativeStageSegment)'; \
		fi; \
		echo '*nds_renderer.o(.text.ndsRendererNativeStageLoadNoZMatrix)'; \
		echo '*nds_renderer.o(.text.ndsRendererNativeStageEmitNoZTriangle)'; \
		echo '*nds_renderer.o(.text.ndsRendererNativeApplyStateDelta.part.0)'; \
		echo '*nds_renderer.o(.text.ndsRendererNativeApplyStateSpan)'; \
		echo '*nds_renderer.o(.text.ndsRendererSyncTextureTile)'; \
	} > "$$tmp"; \
	if test -f "$@" && cmp -s "$$tmp" "$@"; then rm "$$tmp"; else mv "$$tmp" "$@"; fi

# The .inc carries only the sheet's path, size and hash; the 22,528 bytes
# themselves are the NitroFS payload, produced by the same invocation.
$(NDS_TASK39_HIT_SPARKS_INC) $(NDS_TASK39_HIT_SPARKS_ASSET) &: \
		$(PROJECT_ROOT)/scripts/2d_vfx/generate_task39_hit_sparks.py \
		$(BATTLESHIP_O2R)/particles/efcommon_particle_scb \
		$(BATTLESHIP_O2R)/particles/efcommon_particle_txb
	python "$(PROJECT_ROOT)/scripts/2d_vfx/generate_task39_hit_sparks.py"

$(NITROFS_DIR)/effects/task39_hit_sparks.rgb5a1.bin: $(NDS_TASK39_HIT_SPARKS_ASSET)
	@mkdir -p $(dir $@)
	@cp $< $@

# R2-07: the packed EFCommon particle bank. The reachable-script set is derived
# from the P1 effect seams in generate_task39_effect_census.py plus the bank's
# own spawn graph, so both scripts and efmanager.c are prerequisites. The header
# and the JSON report are committed; the .inc is a build product under the
# gitignored src/nds/generated, which is why the ELF depends on it.
#
# One generator invocation produces the .inc AND the texel/palette payload, so
# they are a grouped target (`&:`) rather than two rules that would each run it.
# The payload is not linked -- see the header comment: .rodata is taken out of
# the boot-time taskman arena one-for-one and this pack is big enough to push
# that search past its floor.
# The bake varies with NDS_P2_STAGE_YOSTER -- generate_nds_particle_banks.py:114
# reads it from the environment and this Makefile exports it -- while the .inc,
# all three payloads and the committed JSON land on SHARED paths. The per-BUILD
# config header below is therefore not enough on its own, for exactly the reason
# the mn_ui_kit stamp records: bake A touches the shared outputs, and build B
# whose own config header is OLDER then reuses A's bake at a different flag
# value. That is how the committed NDS_PARTICLE_BANKS.generated.json came to
# differ from the BattleShip sources on 2026-09-03. This stamp carries the flag
# value itself and is rewritten only when it changes, so the bake is keyed on
# what varies rather than on a timestamp another build controls.
NDS_PARTICLE_BANKS_FLAGS := $(NDS_P2_STAGE_YOSTER)
