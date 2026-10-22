/* The general stage DL submit (its owner claims and submits are the two
 * .inc fragments it includes) and the tree walks that call it: DObj trees,
 * the Race's bombs and bumpers, item GObj trees with replay and Sector Z's
 * Arwing in two passes. Included in order by reloc_backend_renderer_dl.c;
 * not a translation unit of its own. */

static void __attribute__((noinline)) ndsRendererAdapterSubmitStageDLBody(
    DObj *dobj, const Gfx *dl, GObj *camera_gobj, u32 initial_geometry_mode)
{
    NDSRelocLoadedFile *loaded;
    NDSRendererConfig config = {0};
    NDSRendererStats stats;
    NDSRendererStats *render_stats;
    NDSFighterDLDrawState state;
    NDSRendererCommandCallback callback;
    void *callback_user;
    NDSRendererMatrix20p12 initial_projection;
    NDSRendererMatrix20p12 initial_modelview;
    const NDSRendererMatrix20p12 *initial_projection_ptr;
    const NDSRendererMatrix20p12 *initial_modelview_ptr;
    NDSRendererNativeMaterial impact_wave_material;
    sb32 impact_wave_native_candidate = FALSE;
    sb32 impact_wave_native_handled = FALSE;
    u32 rebirth_halo_root_offset = 0u;
    sb32 rebirth_halo_native_candidate = FALSE;
    sb32 rebirth_halo_native_handled = FALSE;
#if NDS_P2_STAGE_INISHIE
    NDSRendererNativeMaterial inishie_pakkun_material;
    NDSRelocLoadedFile *inishie_pakkun_palette_file = NULL;
    sb32 inishie_pakkun_native_candidate = FALSE;
    sb32 inishie_pakkun_native_handled = FALSE;
    const void *inishie_powblock_tlut = NULL;
    const void *inishie_powblock_image_a = NULL;
    const void *inishie_powblock_image_b = NULL;
    sb32 inishie_powblock_native_candidate = FALSE;
    sb32 inishie_powblock_native_handled = FALSE;
#endif
    sb32 charge_shot_native_candidate = FALSE;
    sb32 charge_shot_native_handled = FALSE;
    sb32 thunder_jolt_native_candidate = FALSE;
    sb32 thunder_jolt_native_handled = FALSE;
    NDSRendererNativeMaterial thunder_ground_material;
    u32 thunder_ground_root_index = 0u;
    sb32 thunder_ground_native_candidate = FALSE;
    sb32 thunder_ground_native_handled = FALSE;
    NDSRendererNativeMaterial thunder_fx_material;
    const void *thunder_fx_base = NULL;
    u32 thunder_fx_bytes = 0u;
    sb32 thunder_fx_native_candidate = FALSE;
    sb32 thunder_fx_native_handled = FALSE;
    NDSRendererNativeMaterial damage_slash_material;
    const void *damage_slash_base = NULL;
    u32 damage_slash_bytes = 0u;
    u32 damage_slash_root = 0u;
    sb32 damage_slash_native_seen = FALSE;
    sb32 damage_slash_native_candidate = FALSE;
    sb32 damage_slash_native_handled = FALSE;
    sb32 damage_slash_native_settled = FALSE;
    sb32 damage_fly_mdust_native_seen = FALSE;
    sb32 damage_fly_mdust_native_settled = FALSE;
    NDSRendererNativeMaterial ness_pkfire_materials[2];
    sb32 ness_pkfire_native_candidate = FALSE;
    sb32 ness_pkfire_native_handled = FALSE;
    NDSRendererNativeMaterial ness_pkthunder_material;
    u32 ness_pkthunder_root_index = 0u;
    u32 ness_pkthunder_trail_color = 0u;
    sb32 ness_pkthunder_native_candidate = FALSE;
    sb32 ness_pkthunder_native_handled = FALSE;
    NDSRendererNativeMaterial purin_sing_material;
    const void *purin_sing_image = NULL;
    u32 purin_sing_root_index = 0u;
    sb32 purin_sing_native_candidate = FALSE;
    sb32 purin_sing_native_handled = FALSE;
    sb32 kirby_vulcan_native_handled = FALSE;
    sb32 pikachu_thunder_native_handled = FALSE;
    sb32 samus_bomb_native_handled = FALSE;
    sb32 ness_pktail_native_handled = FALSE;
    NDSRendererNativeMaterial yoshi_entryegg_material;
    const void *yoshi_entryegg_palette = NULL;
    sb32 yoshi_entryegg_native_candidate = FALSE;
    sb32 yoshi_entryegg_native_handled = FALSE;
    sb32 yoshi_egg_is_weapon = FALSE;
    sb32 yoshi_egg_native_candidate = FALSE;
    sb32 yoshi_egg_native_handled = FALSE;
    const void *yoshi_egglay_palette = NULL;
    const void *yoshi_egglay_image = NULL;
    sb32 yoshi_egglay_native_candidate = FALSE;
    sb32 yoshi_egglay_native_handled = FALSE;
#if NDS_P2_STAGE_CASTLE
    NDSRendererNativeMaterial castle_bumper_material;
    sb32 castle_bumper_native_candidate = FALSE;
    sb32 castle_bumper_native_handled = FALSE;
#endif
#if NDS_P2_STAGE_SECTOR
    const void *sector_laser_tlut = NULL;
    const void *sector_laser_image = NULL;
    sb32 sector_laser_native_candidate = FALSE;
    sb32 sector_laser_native_handled = FALSE;
#endif
#if NDS_P2_LINK
    u32 link_bomb_root = 0u;
    sb32 link_bomb_native_candidate = FALSE;
    sb32 link_bomb_native_handled = FALSE;
#endif
#if NDS_P2_STAGE_YAMABUKI
    sb32 marumine_native_candidate = FALSE;
    sb32 marumine_native_handled = FALSE;
#endif
#if NDS_P2_STAGE_YAMABUKI
    sb32 glucky_native_candidate = FALSE;
    sb32 glucky_native_handled = FALSE;
    sb32 porygon_native_candidate = FALSE;
    sb32 porygon_native_handled = FALSE;
    NDSRendererNativeMaterial hitokage_material;
    sb32 hitokage_native_candidate = FALSE;
    sb32 hitokage_native_handled = FALSE;
    NDSRendererNativeMaterial fushigibana_material;
    sb32 fushigibana_native_candidate = FALSE;
    sb32 fushigibana_native_handled = FALSE;
#endif
    sb32 item_tomato_native_candidate = FALSE;
    sb32 item_tomato_native_handled = FALSE;
    NDSRendererNativeMaterial item_star_material0;
    NDSRendererNativeMaterial item_star_material1;
    sb32 item_star_native_candidate = FALSE;
    sb32 item_star_native_handled = FALSE;
    u32 item_sword_root = 0u;
    sb32 item_sword_native_candidate = FALSE;
    sb32 item_sword_native_handled = FALSE;
    sb32 item_hammer_native_candidate = FALSE;
    sb32 item_hammer_native_handled = FALSE;
    u32 item_mball_root = 0u;
    NDSRendererNativeMaterial item_mball_material;
    sb32 item_mball_native_candidate = FALSE;
    sb32 item_mball_native_handled = FALSE;
    sb32 item_mball_from_effect = FALSE;
    sb32 item_kirbystar_native_candidate = FALSE;
    sb32 item_kirbystar_native_handled = FALSE;
    NDSRendererNativeMaterial item_gshell_material;
    sb32 item_gshell_native_candidate = FALSE;
    sb32 item_gshell_native_handled = FALSE;
    NDSRendererNativeMaterial item_rshell_material;
    sb32 item_rshell_native_candidate = FALSE;
    sb32 item_rshell_native_handled = FALSE;
    u32 item_bat_root = 0u;
    sb32 item_bat_native_candidate = FALSE;
    sb32 item_bat_native_handled = FALSE;
    u32 item_capsule_root = 0u;
    sb32 item_capsule_native_candidate = FALSE;
    sb32 item_capsule_native_handled = FALSE;
    NDSRendererNativeMaterial item_bombhei_material;
    sb32 item_bombhei_native_candidate = FALSE;
    sb32 item_bombhei_native_handled = FALSE;
    sb32 item_lgun_native_candidate = FALSE;
    sb32 item_lgun_native_handled = FALSE;
    sb32 item_harisen_native_candidate = FALSE;
    sb32 item_harisen_native_handled = FALSE;
    sb32 item_heart_native_candidate = FALSE;
    sb32 item_heart_native_handled = FALSE;
    u32 item_starrod_root = 0u;
    sb32 item_starrod_native_candidate = FALSE;
    sb32 item_starrod_native_handled = FALSE;
    u32 item_fflower_root = 0u;
    NDSRendererNativeMaterial item_fflower_material;
    sb32 item_fflower_native_candidate = FALSE;
    sb32 item_fflower_native_handled = FALSE;
    u32 item_msbomb_root = 0u;
    sb32 item_msbomb_native_candidate = FALSE;
    sb32 item_msbomb_native_handled = FALSE;
    NDSRendererNativeMaterial item_nbumper_material;
    sb32 item_nbumper_native_candidate = FALSE;
    sb32 item_nbumper_native_handled = FALSE;
    sb32 item_box_native_candidate = FALSE;
    sb32 item_box_native_handled = FALSE;
    sb32 item_taru_native_candidate = FALSE;
    sb32 item_taru_native_handled = FALSE;
    sb32 item_egg_native_candidate = FALSE;
    sb32 item_egg_native_handled = FALSE;
    sb32 item_iwark_native_candidate = FALSE;
    sb32 item_iwark_native_handled = FALSE;
    sb32 item_baked_native_handled = FALSE;
    u32 visual_effect_template = 0u;
    sb32 visual_effect_native_candidate = FALSE;
    sb32 visual_effect_native_handled = FALSE;
    /* Wider than "handled" on purpose: TRUE when this owner either drew or
     * recorded its own precise REJECTED_PROGRAM failure. A decline must not
     * also trip the generic NO_PROGRAM guards below, or one object publishes
     * two reasons for one event and the first-failure record names the
     * wrong one. */
    sb32 visual_effect_native_settled = FALSE;
    u32 effect_seed_before = 0u;
    u32 effect_matrix_cmd_before = 0u;
    u32 effect_xform_before = 0u;
    u32 effect_hw_vertex_before = 0u;
    u32 effect_hw_triangle_before = 0u;
    void *saved_graphics_heap_ptr;
    sb32 detailed_output;
    sb32 inherited_texture = FALSE;
    sb32 inherited_tile = FALSE;
    sb32 inherited_segment = FALSE;

    if ((dobj == NULL) || (dl == NULL))
    {
        return;
    }

    /* Entry models execute their generated native owners (admitted by the
     * caller). Unhandled required roots below report a native failure; no
     * interpreter is available. */

    /* RebirthHalo is already identified by the effect-tree owner before any
     * child list reaches here. Its six generated groups contain every source
     * state/texture/vertex dependency, so do not pay the generic adapter's
     * loaded-file scan, segment-E material preparation, callback context and
     * command-interpreter setup merely to arrive at the native submitter.
     *
     * Keep this per-DObj for the experiment: it preserves the exact world
     * matrix of the child and rotating grandchild while isolating the cost of
     * generic adapter ceremony. A later all-tree owner can merge the duplicate
     * child matrix/load once this gate has a visual/tick verdict. */
    if ((gNdsStageDLHaloFirst == 0u) &&
        (sNdsRendererAdapterRebirthHaloNativeActive != FALSE) &&
        (gEFManagerFiles[2] != NULL) &&
        ((const u8 *)dl >= (const u8 *)gEFManagerFiles[2]))
    {
        uintptr_t rebirth_offset = (uintptr_t)((const u8 *)dl -
                                               (const u8 *)gEFManagerFiles[2]);

        if ((rebirth_offset == 0x2378u) || (rebirth_offset == 0x2a88u) ||
            (rebirth_offset == 0x27e8u))
        {
            NDSRendererConfig rebirth_config = {0};
            NDSRendererStats rebirth_stats;
            NDSRendererStats *rebirth_render_stats;
            NDSRendererMatrix20p12 rebirth_projection;
            NDSRendererMatrix20p12 rebirth_modelview;
            const NDSRendererMatrix20p12 *rebirth_projection_ptr;
            const NDSRendererMatrix20p12 *rebirth_modelview_ptr;
            void *rebirth_saved_graphics_heap_ptr = gSYTaskmanGraphicsHeap.ptr;

            /* 0x2378 and 0x2a88 are the two DL links on the SAME child DObj.
             * Once the first one has emitted both native roots with one matrix
             * setup, the tree walker will immediately offer 0x2a88 again. */
            if ((rebirth_offset == 0x2a88u) &&
                (sNdsRendererAdapterRebirthHaloSkipSecondChildList != FALSE))
            {
                sNdsRendererAdapterRebirthHaloSkipSecondChildList = FALSE;
                return;
            }

            ndsRendererAdapterPrepareInitialMatrices(
                dobj,
                (camera_gobj != NULL) ? CObjGetStruct(camera_gobj) :
                    ((gGCCurrentCamera != NULL) ? CObjGetStruct(gGCCurrentCamera) : NULL),
                TRUE,
                &rebirth_projection,
                &rebirth_projection_ptr,
                &rebirth_modelview,
                &rebirth_modelview_ptr);

            if (sNdsRendererAdapterStagePersistentActive != FALSE)
            {
                rebirth_render_stats = &sNdsRendererAdapterStagePersistentStats;
                ndsFighterDLDrawResetRuntimeRendererStats(rebirth_render_stats);
            }
            else
            {
                rebirth_render_stats = &rebirth_stats;
                ndsRendererInitStats(rebirth_render_stats);
            }
            if ((sNdsRendererAdapterEffectColorMask & 1u) != 0u)
            {
                rebirth_render_stats->prim_color = sNdsRendererAdapterEffectPrimColor;
            }
            if ((sNdsRendererAdapterEffectColorMask & 2u) != 0u)
            {
                rebirth_render_stats->env_color = sNdsRendererAdapterEffectEnvColor;
            }
            if (sNdsRendererAdapterEffectOtherModeValid != 0u)
            {
                rebirth_render_stats->othermode_l = sNdsRendererAdapterEffectOtherModeL;
            }

            rebirth_config.max_depth = 8u;
            rebirth_config.max_commands = 8192u;
            rebirth_config.max_list_commands = 512u;
            rebirth_config.initial_projection = rebirth_projection_ptr;
            rebirth_config.initial_modelview = rebirth_modelview_ptr;
            rebirth_config.initial_geometry_mode = initial_geometry_mode;
            rebirth_config.texture_data_layout = NDS_RENDERER_TEXTURE_DATA_O2R_WORD_SWAPPED;

            if (ndsRendererSubmitNativeRebirthHalo(
                    (u32)rebirth_offset, &rebirth_config,
                    rebirth_render_stats) != FALSE)
            {
                NDS_DIAG(gNdsRebirthHaloNativeDrawCount++);
                if (rebirth_offset == 0x2378u)
                {
                    /* Same DObj, same source matrix, adjacent source order.
                     * Keep the live renderer state produced by 0x2378 and emit
                     * its second linked list without rebuilding the adapter. */
                    if (ndsRendererSubmitNativeRebirthHalo(
                            0x2a88u, &rebirth_config,
                            rebirth_render_stats) != FALSE)
                    {
                        NDS_DIAG(gNdsRebirthHaloNativeDrawCount++);
                        sNdsRendererAdapterRebirthHaloSkipSecondChildList = TRUE;
                    }
                    else
                    {
                        NDS_DIAG(gNdsRebirthHaloNativeFallbackCount++);
                    }
                }
                gNdsStageGCDrawAllLoopHardwareTriangleCount +=
                    rebirth_render_stats->hardware_triangle_count;
                gNdsStageGCDrawAllLoopHardwareZBufferTriangleCount +=
                    rebirth_render_stats->hardware_zbuffer_triangle_count;
                gNdsStageGCDrawAllLoopHardwareProjectedDepthTriangleCount +=
                    rebirth_render_stats->hardware_projected_depth_triangle_count;
                gNdsStageGCDrawAllLoopHardwareDecalDepthTriangleCount +=
                    rebirth_render_stats->hardware_decal_depth_triangle_count;
                gNdsStageGCDrawAllLoopHardwareTextureBindCount +=
                    rebirth_render_stats->hardware_texture_bind_count;
                gNdsStageGCDrawAllLoopHardwareTextureUploadCount +=
                    rebirth_render_stats->hardware_texture_upload_count;
                gNdsStageGCDrawAllLoopHardwareTextureReadyCount +=
                    rebirth_render_stats->hardware_texture_ready_count;
                gNdsStageGCDrawAllLoopHardwareTextureRejectCount +=
                    rebirth_render_stats->hardware_texture_reject_count;
                ndsTaskmanSampleGraphicsHeap();
                gSYTaskmanGraphicsHeap.ptr = rebirth_saved_graphics_heap_ptr;
                return;
            }
            NDS_DIAG(gNdsRebirthHaloNativeFallbackCount++);
            ndsTaskmanSampleGraphicsHeap();
            gSYTaskmanGraphicsHeap.ptr = rebirth_saved_graphics_heap_ptr;
        }
    }

    loaded = ndsRelocFindLoadedFileContaining(dl, sizeof(*dl));
    /* A list in a range the battle lent out (a stage layer's, or a content
     * article's that declined its native draw) is other allocations' bytes
     * now: rejected unread. */
    if ((loaded == NULL) && (ndsRelocAddressInLentSpan(dl) != FALSE))
    {
        extern volatile u32 gNdsNativeStageLentLayerSkips;

        gNdsNativeStageLentLayerSkips++;
        ndsStageRejectNativeRender(dobj, dl, NDS_NATIVE_FAILURE_NO_PROGRAM, NULL);
        return;
    }
    if ((loaded == NULL) &&
        (ndsFighterDLScanRangeInTaskmanArena(dl, sizeof(*dl)) == FALSE))
    {
        ndsStageRejectNativeRender(dobj, dl, NDS_NATIVE_FAILURE_BAD_ASSET, NULL);
        return;
    }
#include "renderer_adapter_stage_claims.inc"

    NDS_LAB_SDL_MARK(0);
    detailed_output = (ndsRendererHardwareNoOracleEnabled() == FALSE) ?
        TRUE : FALSE;
    if (detailed_output != FALSE)
    {
        bzero(&state, sizeof(state));
    }
    else
    {
        /* Profile 0/1 submit with a null command callback. Only the compact
         * branch/data resolver context is live; software-preview vertices
         * are already retained by the renderer's persistent vertex cache. */
        state.segment_e_base = NULL;
        state.segment_e_end = NULL;
    }
    state.primary_file = loaded;
    state.slot = 0u;
    if (sNdsRendererAdapterStagePersistentActive != FALSE)
    {
        inherited_texture = ndsRendererAdapterStatsHasArmedTexture(
            &sNdsRendererAdapterStagePersistentStats);
        inherited_tile = ndsRendererAdapterStatsHasArmedTile(
            &sNdsRendererAdapterStagePersistentStats);
        if (detailed_output != FALSE)
        {
            ndsFighterDLDrawSeedPersistentState(
                &state, &sNdsRendererAdapterStagePersistentState);
        }
        else
        {
            state.segment_e_base =
                sNdsRendererAdapterStagePersistentState.segment_e_base;
            state.segment_e_end =
                sNdsRendererAdapterStagePersistentState.segment_e_end;
        }
        inherited_segment = (state.segment_e_base != NULL) ? TRUE : FALSE;
        NDS_DIAG(gNdsStageGCDrawAllLoopHardwareCarrySeedCount++);
        if (inherited_texture != FALSE)
        {
            NDS_DIAG(gNdsStageGCDrawAllLoopHardwareCarryTextureSeedCount++);
        }
        if (inherited_tile != FALSE)
        {
            NDS_DIAG(gNdsStageGCDrawAllLoopHardwareCarryTileSeedCount++);
        }
        if (inherited_segment != FALSE)
        {
            NDS_DIAG(gNdsStageGCDrawAllLoopHardwareCarrySegmentSeedCount++);
        }
    }
    NDS_LAB_SDL_MARK(4);
    saved_graphics_heap_ptr = gSYTaskmanGraphicsHeap.ptr;
#if NDS_P2_STAGE_CASTLE
    if (castle_bumper_native_candidate != FALSE)
    {
        /* The native item owner consumes the typed MObj snapshot directly.
         * Falling through to ndsRendererAdapterPrepareMaterialSegment would
         * build a segment-E Gfx stream for a DObj this owner is about to draw
         * natively, which is the capability the native-only review forbids. */
    }
    else
#endif
    if ((damage_slash_native_candidate != FALSE) ||
        (damage_fly_mdust_native_seen != FALSE))
    {
        /* The native owner consumes the typed live MObj snapshot directly at
         * the source segment-E position.  Building a Gfx material branch here
         * would reintroduce the generic N64 command path this owner removes. */
    }
    else
    if (visual_effect_native_candidate != FALSE)
    {
        /* No MObj exists on a template DObj and none is wanted: the owner's
         * combine, texture state and colours are all baked. Preparing a
         * segment-E material here would manufacture Gfx for a list nothing
         * executes. */
    }
    else
#if NDS_P2_STAGE_INISHIE
    if (inishie_pakkun_native_candidate != FALSE)
    {
        /* The native item owner consumes the typed MObj snapshot directly. */
    }
    else if (inishie_powblock_native_candidate != FALSE)
    {
        /* The POW block owner draws the list's own immutable material; the
         * DObj has no MObj, so preparing a segment-E stream here would
         * manufacture Gfx for a list nothing executes. */
    }
    else
#endif
    if (ness_pkfire_native_candidate != FALSE)
    {
        /* The owner already captured the two live LIGHT1|LIGHT2 materials as
         * typed snapshots.  Building a segment-E Gfx stream here would
         * reintroduce the generic command path and duplicate those updates. */
    }
    else
    if ((sNdsRendererAdapterImpactWaveNativeActive != FALSE) &&
        (dobj->mobj != NULL) &&
        (ndsRendererAdapterBuildNativeMaterial(
             dobj->mobj, &impact_wave_material) != FALSE))
    {
        impact_wave_native_candidate = TRUE;
    }
    else
    {
        ndsRendererAdapterPrepareMaterialSegment(dobj, &state);
    }
    NDS_LAB_SDL_MARK(5);
    ndsRendererAdapterPrepareInitialMatrices(dobj,
                                             (camera_gobj != NULL) ?
                                                 CObjGetStruct(camera_gobj) :
                                                 ((gGCCurrentCamera != NULL) ?
                                                      CObjGetStruct(
                                                          gGCCurrentCamera) :
                                                      NULL),
                                             TRUE,
                                             &initial_projection,
                                             &initial_projection_ptr,
                                             &initial_modelview,
                                             &initial_modelview_ptr);

    NDS_LAB_SDL_MARK(1);
    config.max_depth = 8u;
    config.max_commands = 8192u;
    config.max_list_commands = 512u;
    config.initial_projection = initial_projection_ptr;
    config.initial_modelview = initial_modelview_ptr;
    config.initial_geometry_mode = 0u;
    config.initial_geometry_mode = initial_geometry_mode;
    config.texture_data_layout = NDS_RENDERER_TEXTURE_DATA_O2R_WORD_SWAPPED;
    config.validate_range = ndsRendererAdapterStageValidateRange;
    config.immutable_command_span = ndsRendererAdapterImmutableCommandSpan;
    config.resolve_branch = ndsFighterDLDrawResolveBranch;
    config.resolve_data = ndsFighterDLDrawResolveRendererData;
    config.user = &state;
    callback = (ndsRendererHardwareNoOracleEnabled() != FALSE) ?
        NULL : ndsFighterMarioFoxVisitDLDrawCommand;
    callback_user = &state;

    render_stats = &stats;
    if (sNdsRendererAdapterStagePersistentActive != FALSE)
    {
        render_stats = &sNdsRendererAdapterStagePersistentStats;
        if (detailed_output != FALSE)
        {
            ndsFighterDLDrawResetTransientRendererStats(render_stats);
        }
        else
        {
            ndsFighterDLDrawResetRuntimeRendererStats(render_stats);
        }
    }
    else
    {
        ndsRendererInitStats(render_stats);
    }
    if (sNdsRendererAdapterEffectSubmitActive != FALSE)
    {
        /* Source order: the proc emits prim/env into the head stream and THEN
         * the model list, so these must land before the list executes and must
         * be allowed to be overridden by the list's own colour commands. */
        if ((sNdsRendererAdapterEffectColorMask & 1u) != 0u)
        {
            render_stats->prim_color = sNdsRendererAdapterEffectPrimColor;
        }
        if ((sNdsRendererAdapterEffectColorMask & 2u) != 0u)
        {
            render_stats->env_color = sNdsRendererAdapterEffectEnvColor;
        }
        /* THE ONLY WRITER OF THE CAPTURED BLEND STATE, and it is inside the
         * effect-submit guard. Without this the effect layer inherits whatever
         * othermode_l the previous list left, which is an opaque stage mode,
         * and ndsRendererHardwareAlpha takes its alpha-31 early return for
         * every effect polygon. */
        if (sNdsRendererAdapterEffectOtherModeValid != 0u)
        {
            render_stats->othermode_l = sNdsRendererAdapterEffectOtherModeL;
        }
        /* Latched HERE, not at the display-proc marker, because the marker
         * publishes the layer's sticky value and a probe stopping at one effect
         * would read another effect's mode. This is the mode THIS list starts
         * with; the Out latch below is what it finishes with. */
        gNdsEffectDLSubmitOtherModeIn = render_stats->othermode_l;
        effect_seed_before = render_stats->hardware_matrix_seed_count;
        effect_matrix_cmd_before = render_stats->matrix_command_count;
        effect_xform_before = render_stats->transformed_vertex_count;
        effect_hw_vertex_before = render_stats->hardware_vertex_count;
        effect_hw_triangle_before = render_stats->hardware_triangle_count;
    }
    if (sNdsRendererAdapterItemSubmitActive != FALSE)
    {
        u32 head = (sNdsRendererAdapterItemSubmitHead <
                    NDS_RENDERER_STAGE_DL_HEADS) ?
            sNdsRendererAdapterItemSubmitHead : 0u;

        gNdsItemRendererLastHead = head;
        gNdsItemRendererLastColorMask =
            sNdsRendererAdapterItemColorMask[head];
        gNdsItemRendererLastEnvColor =
            sNdsRendererAdapterItemEnvColor[head];
        gNdsItemRendererLastOtherModeL =
            sNdsRendererAdapterItemOtherModeL[head];
        gNdsItemRendererLastOtherModeH =
            sNdsRendererAdapterItemOtherModeH[head];

        if ((sNdsRendererAdapterItemColorMask[head] & 1u) != 0u)
        {
            render_stats->prim_color = sNdsRendererAdapterItemPrimColor[head];
        }
        if ((sNdsRendererAdapterItemColorMask[head] & 2u) != 0u)
        {
            render_stats->env_color = sNdsRendererAdapterItemEnvColor[head];
        }
        if (sNdsRendererAdapterItemOtherModeLValid[head] != 0u)
        {
            render_stats->othermode_l =
                sNdsRendererAdapterItemOtherModeL[head];
        }
        if (sNdsRendererAdapterItemOtherModeHValid[head] != 0u)
        {
            render_stats->othermode_h =
                sNdsRendererAdapterItemOtherModeH[head];
        }
    }
    NDS_LAB_SDL_MARK(2);
#include "renderer_adapter_stage_submits.inc"
    NDS_LAB_SDL_MARK(3);
    if (sNdsRendererAdapterEffectSubmitActive != FALSE)
    {
        /* The config's matrices read from OUR locals, and the executor's own
         * verdict on them read as a delta. Cycles 53-55 tried to read the same
         * two pointers out of the callee's argument register and got three
         * different answers; nds_effects.h records why that read can never
         * settle it. */
        /* Out - In names an asset list that carries its own render mode: the
         * rebirth halo's DObj entry[2] list-0 leaves set TEX_EDGE and restore
         * OPA_SURF (85.vpk0.bin 0x23a8/0x2490), while its list-1 beam
         * (0x2890) emits no G_SETOTHERMODE_L at all and leaves Out == In. */
        gNdsEffectDLSubmitOtherModeOut = render_stats->othermode_l;
        NDS_DIAG(gNdsEffectDLSubmitCount++);
        gNdsEffectDLCfgMask =
            ((config.initial_projection != NULL) ? 1u : 0u) |
            ((config.initial_modelview != NULL) ? 2u : 0u);
        if (config.initial_modelview != NULL)
        {
            gNdsEffectDLCfgMvT[0] = config.initial_modelview->m[3][0];
            gNdsEffectDLCfgMvT[1] = config.initial_modelview->m[3][1];
            gNdsEffectDLCfgMvT[2] = config.initial_modelview->m[3][2];
        }
        gNdsEffectDLMatrixSeed =
            render_stats->hardware_matrix_seed_count - effect_seed_before;
        gNdsEffectDLMatrixCmd =
            render_stats->matrix_command_count - effect_matrix_cmd_before;
        gNdsEffectDLXformVertexCount =
            render_stats->transformed_vertex_count - effect_xform_before;
        gNdsEffectDLHwVertexCount =
            render_stats->hardware_vertex_count - effect_hw_vertex_before;
        gNdsEffectDLHwTriangleCount =
            render_stats->hardware_triangle_count - effect_hw_triangle_before;
        if (sNdsRendererAdapterStagePersistentActive != FALSE)
        {
            gNdsEffectDLVtx0[0] =
                sNdsRendererAdapterStageVertexCache.transformed_vertices[0].x;
            gNdsEffectDLVtx0[1] =
                sNdsRendererAdapterStageVertexCache.transformed_vertices[0].y;
            gNdsEffectDLVtx0[2] =
                sNdsRendererAdapterStageVertexCache.transformed_vertices[0].z;
            gNdsEffectDLVtx0[3] =
                sNdsRendererAdapterStageVertexCache.transformed_vertices[0].w;
        }
        gNdsEffectDLBlocker = render_stats->blocker;
        gNdsEffectDLCommandCount = render_stats->command_count;
        gNdsEffectDLFirstOpcode = render_stats->first_opcode;
        gNdsEffectDLUnsupportedOpcode = render_stats->unsupported_opcode;
        /* vertex_count/triangle_count, NOT the *_command_count pair: every
         * site that increments those is wrapped in
         * NDS_RENDERER_RECORD_PROOF_ONLY, which is ((void)0) whenever
         * NDS_RENDERER_HW_TRIANGLES is set -- i.e. dead in every build that
         * can draw. Reading them cost this investigation one wrong conclusion. */
        gNdsEffectDLVertexCount = render_stats->vertex_count;
        gNdsEffectDLTriangleCount = render_stats->triangle_count;
        NDS_DIAG(gNdsEffectDLPublishCount++);
    }
    /* P2-3r13: the fighter's own graphics-heap peak, before it is rolled back
     * and becomes invisible to the end-of-frame sample. */
    ndsTaskmanSampleGraphicsHeap();
    gSYTaskmanGraphicsHeap.ptr = saved_graphics_heap_ptr;
    if (sNdsRendererAdapterStagePersistentActive != FALSE)
    {
        if (detailed_output != FALSE)
        {
            ndsFighterDLDrawCapturePersistentState(
                &sNdsRendererAdapterStagePersistentState, &state);
        }
        else
        {
            sNdsRendererAdapterStagePersistentState.segment_e_base =
                state.segment_e_base;
            sNdsRendererAdapterStagePersistentState.segment_e_end =
                state.segment_e_end;
        }
        NDS_DIAG(gNdsStageGCDrawAllLoopHardwareCarryCaptureCount++);
        if (render_stats->command_count <= 5u)
        {
            if (inherited_texture != FALSE)
            {
                NDS_DIAG(gNdsStageGCDrawAllLoopHardwareCarryShortTextureSeedCount++);
            }
            if (inherited_tile != FALSE)
            {
                NDS_DIAG(gNdsStageGCDrawAllLoopHardwareCarryShortTileSeedCount++);
            }
        }
    }
    gNdsStageGCDrawAllLoopHardwareTriangleCount +=
        render_stats->hardware_triangle_count;
    gNdsStageGCDrawAllLoopHardwareZBufferTriangleCount +=
        render_stats->hardware_zbuffer_triangle_count;
    gNdsStageGCDrawAllLoopHardwareProjectedDepthTriangleCount +=
        render_stats->hardware_projected_depth_triangle_count;
    gNdsStageGCDrawAllLoopHardwareDecalDepthTriangleCount +=
        render_stats->hardware_decal_depth_triangle_count;
    gNdsStageGCDrawAllLoopHardwareTextureBindCount +=
        render_stats->hardware_texture_bind_count;
    gNdsStageGCDrawAllLoopHardwareTextureUploadCount +=
        render_stats->hardware_texture_upload_count;
    gNdsStageGCDrawAllLoopHardwareTextureReadyCount +=
        render_stats->hardware_texture_ready_count;
    gNdsStageGCDrawAllLoopHardwareTextureRejectCount +=
        render_stats->hardware_texture_reject_count;
    if (render_stats->hardware_texture_ready_count != 0u)
    {
        if (render_stats->hardware_texture_format < 32u)
        {
            gNdsStageGCDrawAllLoopHardwareTextureFormatMask |=
                1u << render_stats->hardware_texture_format;
        }
        if (render_stats->hardware_texture_width >
            gNdsStageGCDrawAllLoopHardwareTextureMaxWidth)
        {
            gNdsStageGCDrawAllLoopHardwareTextureMaxWidth =
                render_stats->hardware_texture_width;
        }
        if (render_stats->hardware_texture_height >
            gNdsStageGCDrawAllLoopHardwareTextureMaxHeight)
        {
            gNdsStageGCDrawAllLoopHardwareTextureMaxHeight =
                render_stats->hardware_texture_height;
        }
    }
}

/* ONE NODE IS NOT A TREE, AND THAT IS WHY THE SHIELD WAS A QUARTER CIRCLE.
 *
 * This submitted the DObj it was handed and stopped. The source does not:
 * gcDrawDObjTree (objdisplay.c) recurses into `child` and, from the first
 * sibling, walks the whole `sib_next` chain. Every multi-node effect model
 * therefore drew only its ROOT node here -- the owner's own words on the first
 * build that routed the real asset were "it is using the correct asset but its
 * only like 1/2 or 1/4 of it, like a 1/4 slice of the complete circle".
 *
 * That single omission is what four BUGS.md rows have in common. The shield,
 * Fox's reflector, the rebirth halo and the impact wave are all EFDescs whose
 * geometry is a DObj tree, so all four were being drawn one node deep, and no
 * amount of atlas resolution or palette work could ever have shown the rest of
 * them.
 *
 * The sibling rule is the source's, kept exactly: only a node with no
 * `sib_prev` walks the chain, so a tree is traversed once rather than once per
 * sibling. Recursion depth is the model's own node depth, which these effect
 * descs keep shallow.
 *
 * NOT copied from the source: gcPrepDObjMatrix and the matching gSPPopMatrix.
 * The DS path composes its transform inside ndsRendererAdapterSubmitStageDL
 * per display list rather than pushing an N64 matrix stack here, so adding a
 * push/pop pair around the recursion would double-transform every child. */
static void ndsRendererAdapterSubmitStageDObjNode(DObj *dobj, u32 kind,
                                                  GObj *camera_gobj,
                                                  u32 initial_geometry_mode);

/* BOUNDED, because this walks a tree the PORT builds from resolved offsets and
 * not one the N64 shipped. Those offsets have held raw symbol addresses before
 * -- the note at Makefile:1393 records gcSetupCustomDObjs walking garbage and
 * allocating a DObj per bogus node until the allocator gave up -- so a cycle or
 * a wild pointer here is a real possibility, and unbounded recursion over one
 * is a hung handheld rather than a wrong picture. The first build of this walk
 * timed out a 300-second probe, which is exactly that failure.
 *
 * The limits are far above any real effect model (these descs are a root plus a
 * handful of parts) and both overruns are counted, so hitting one is a
 * diagnosable defect instead of a freeze. */
#define NDS_RENDERER_STAGE_DOBJ_MAX_DEPTH 16u
#define NDS_RENDERER_STAGE_DOBJ_MAX_SIBLINGS 64u

/* The three counters live in diagnostics.c, not here. Defining them inside this
 * `#if NDS_RENDERER_HW_TRIANGLES` block would make them exist only in the
 * configurations that increment them, and a probe naming an absent symbol loses
 * its whole gdb run. nds_effects.h declares them; cliff_ledge.c resets them,
 * which is also what keeps --gc-sections from collecting them. */

static void ndsRendererAdapterSubmitStageDObjTreeDepth(
    DObj *dobj, u32 kind, GObj *camera_gobj, u32 initial_geometry_mode,
    u32 depth)
{
    DObj *sibling;
    u32 seen;

    if (dobj == NULL)
    {
        return;
    }
    if (depth >= NDS_RENDERER_STAGE_DOBJ_MAX_DEPTH)
    {
        NDS_DIAG(gNdsRendererStageDObjDepthOverrunCount++);
        return;
    }
    NDS_DIAG(gNdsRendererStageDObjNodeCount++);
    /* BattleShip gcDrawDObjTree makes HIDDEN a subtree visibility flag: the
     * node's own draw AND its child walk live inside the same !HIDDEN block.
     * The sibling walk is outside that block, so a hidden node must not hide
     * its siblings.  The port used to submit the node through the drawable
     * gate but recurse into its child unconditionally; Mario's pipe exposes
     * that at source frame 100, when the rim becomes HIDDEN for the final 20
     * frames but its barrel child was still emitted as a flat gray/white slab.
     * Keep NOTEXTURE behavior unchanged: it suppresses only this node's DL,
     * not descendants. */
    if ((dobj->flags & DOBJ_FLAG_HIDDEN) == 0u)
    {
        ndsRendererAdapterSubmitStageDObjNode(dobj, kind, camera_gobj,
                                              initial_geometry_mode);
        if (dobj->child != NULL)
        {
            ndsRendererAdapterSubmitStageDObjTreeDepth(
                dobj->child, kind, camera_gobj, initial_geometry_mode,
                depth + 1u);
        }
    }
    if (dobj->sib_prev == NULL)
    {
        seen = 0u;
        for (sibling = dobj->sib_next; sibling != NULL;
             sibling = sibling->sib_next)
        {
            if (++seen > NDS_RENDERER_STAGE_DOBJ_MAX_SIBLINGS)
            {
                NDS_DIAG(gNdsRendererStageDObjSiblingOverrunCount++);
                break;
            }
            ndsRendererAdapterSubmitStageDObjTreeDepth(
                sibling, kind, camera_gobj, initial_geometry_mode,
                depth + 1u);
        }
    }
}

/* EXPLICIT TREE OWNERS ONLY, AND THE MEASUREMENT IS WHY.
 *
 * The first version of this recursed inside ndsRendererAdapterSubmitStageDObj,
 * which is the STAGE entry point -- the stage, the weapons and the effects all
 * reach the hardware through it. A synchronized tick-HUD A/B on identical
 * frames priced that at one whole VBlank:
 *
 *     frame   control      candidate
 *       441   1,119,936    1,120,000
 *       443   1,119,872    1,120,000
 *       447   1,119,488    1,680,384   <- 2 VBlanks -> 3
 *       449   1,119,872    1,680,256   <- 2 VBlanks -> 3
 *
 * +560,896 ticks is 560,190-per-VBlank almost exactly, on frames that were
 * inside the 1.12M gate. The stage carries 57 DObjs (M3_NATIVE_STAGE_CHECK
 * dobjs=57) and the native stage path already handles their geometry, so
 * re-walking them bought nothing and cost a frame.
 *
 * The source models that actually use tree display callbacks still need their
 * descendants. So recursion lives on the explicit effect/item/weapon call
 * sites, while the shared stage entry stays a single-node submit. This keeps
 * the measured stage regression out of normal frames without flattening a
 * source DObj tree such as Link's Boomerang into its transform-only root. */
void ndsRendererAdapterSubmitEffectDObjTree(void *dobj_ptr, u32 kind,
                                            void *camera_gobj_ptr,
                                            u32 initial_geometry_mode)
{
    DObj *root = (DObj *)dobj_ptr;


    /* The procedural visual templates: proc plus vars, resolved once per GObj,
     * exactly as the impact wave's latch does. The DObj cannot answer this --
     * sNdsVisualTemplates is static to battleship_efmanager.c and the list has
     * no asset id -- so the effect owner is asked. */
    sNdsRendererAdapterVisualEffectTemplate = 0u;
    sNdsRendererAdapterVisualEffectNativeActive =
        ((root != NULL) && (root->parent_gobj != NULL) &&
         (ndsEFManagerVisualTemplateIndex(
              root->parent_gobj,
              &sNdsRendererAdapterVisualEffectTemplate) != FALSE)) ?
            TRUE : FALSE;

    sNdsRendererAdapterImpactWaveVariant = 0u;
    sNdsRendererAdapterImpactWaveNativeActive =
        ((root != NULL) && (root->parent_gobj != NULL) &&
         (ndsEFManagerImpactWaveVariant(
              root->parent_gobj,
              &sNdsRendererAdapterImpactWaveVariant) != FALSE)) ?
            TRUE : FALSE;
    sNdsRendererAdapterRebirthHaloNativeActive =
        ((root != NULL) && (root->xobjs_num != 0) &&
         (root->xobjs[0] != NULL) &&
         (root->xobjs[0]->kind == NDS_RENDERER_ADAPTER_JOINT_ATTACH_TRA_MTX_KIND) &&
         (root->child != NULL) && (gEFManagerFiles[2] != NULL) &&
         (root->child->dl_link ==
              (DObjDLLink *)((u8 *)gEFManagerFiles[2] + 0x2a98u))) ?
            TRUE : FALSE;
    sNdsRendererAdapterRebirthHaloSkipSecondChildList = FALSE;
    sNdsRendererAdapterEffectSubmitActive = TRUE;
    ndsRendererAdapterSubmitStageDObjTreeDepth(dobj_ptr, kind, camera_gobj_ptr,
                                               initial_geometry_mode, 0u);
    sNdsRendererAdapterEffectSubmitActive = FALSE;
    sNdsRendererAdapterVisualEffectNativeActive = FALSE;
    sNdsRendererAdapterVisualEffectTemplate = 0u;
    sNdsRendererAdapterImpactWaveNativeActive = FALSE;
    sNdsRendererAdapterImpactWaveVariant = 0u;
    sNdsRendererAdapterRebirthHaloNativeActive = FALSE;
    sNdsRendererAdapterRebirthHaloSkipSecondChildList = FALSE;
}

/* P2-6 (2026-10-03): the Race's rolling barrel bombs and the stage bumpers
 * (Race, the bonus boards, Peach's Castle) wholly outside the view, skipped
 * before any of their lists pays the route, its MObj snapshots and its world
 * matrix (~21K ticks an off-screen barrel, ~13K a bumper; the Race drew three
 * barrels and four bumpers every frame, a camera showing one or two). The
 * bound is the item's root position with a sphere around it: the model's
 * radius (barrel +-222 cube, bumper quad +-180), widened by each child joint's
 * offset and scale and by the root's scale (a bumper doubles on a hit), in
 * the 20.12 camera matrices the lists' own culls use (the baked roots' and the
 * bumper quad's planes, ndsNativeBakedRootOutsideView). A held item, a deeper
 * tree or another kind draws as before. */
__attribute__((used)) volatile u32 gNdsItemPreCulled;

static sb32 __attribute__((target("arm")))
ndsRendererAdapterItemOffscreen(DObj *root, GObj *camera_gobj)
{
    GObj *item_gobj;
    const ITStruct *ip;
    const DObj *child;
    CObj *cobj;
    NDSRendererMatrix20p12 projection;
    NDSRendererMatrix20p12 modelview;
    NDSRendererMatrix20p12 m;
    u32 projection_valid = FALSE;
    u32 modelview_valid = FALSE;
    f32 model_radius;
    f32 extent;
    f32 scale;
    s64 radius;
    s32 p[3];
    u32 plane;
    u32 axis;

    if ((root == NULL) || (camera_gobj == NULL) ||
        (root->parent != DOBJ_PARENT_NULL))
    {
        return FALSE;
    }
    item_gobj = root->parent_gobj;
    ip = (item_gobj != NULL) ? itGetStruct(item_gobj) : NULL;
    cobj = CObjGetStruct(camera_gobj);
    if ((ip == NULL) || (cobj == NULL) || (ip->is_hold != FALSE))
    {
        return FALSE;
    }
    if (ip->kind == nITKindGBumper)
    {
        model_radius = 256.0F;
    }
    else if (ip->kind == nITKindTaruBomb)
    {
        model_radius = 400.0F;
    }
    else
    {
        return FALSE;
    }
    extent = model_radius;
    for (child = root->child; child != NULL; child = child->sib_next)
    {
        f32 t = 0.0F;
        f32 s = 0.0F;

        if (child->child != NULL)
        {
            return FALSE;
        }
        for (axis = 0u; axis < 3u; axis++)
        {
            f32 tc = (&child->translate.vec.f.x)[axis];
            f32 sc = (&child->scale.vec.f.x)[axis];

            t += (tc < 0.0F) ? -tc : tc;
            sc = (sc < 0.0F) ? -sc : sc;
            s = (sc > s) ? sc : s;
        }
        t += model_radius * s;
        extent = (t > extent) ? t : extent;
    }
    scale = 0.0F;
    for (axis = 0u; axis < 3u; axis++)
    {
        f32 sr = (&root->scale.vec.f.x)[axis];

        sr = (sr < 0.0F) ? -sr : sr;
        scale = (sr > scale) ? sr : scale;
        p[axis] = (s32)(&root->translate.vec.f.x)[axis];
    }
    extent *= scale;
    /* A sibling of the root is another world-space DObj the tree draws: its
     * offset from the root, unscaled by it. */
    for (child = root->sib_next; child != NULL; child = child->sib_next)
    {
        f32 t = 0.0F;
        f32 s = 0.0F;

        if (child->child != NULL)
        {
            return FALSE;
        }
        for (axis = 0u; axis < 3u; axis++)
        {
            f32 tc = (&child->translate.vec.f.x)[axis] -
                     (&root->translate.vec.f.x)[axis];
            f32 sc = (&child->scale.vec.f.x)[axis];

            t += (tc < 0.0F) ? -tc : tc;
            sc = (sc < 0.0F) ? -sc : sc;
            s = (sc > s) ? sc : s;
        }
        t += model_radius * s;
        extent = (t > extent) ? t : extent;
    }
    /* +2: the position truncates toward zero. */
    radius = (s64)extent + 2;
    ndsRendererAdapterGetFrameCameraMatrices(cobj, &projection,
                                             &projection_valid, &modelview,
                                             &modelview_valid, NULL, NULL,
                                             NULL);
    if (projection_valid == FALSE)
    {
        return FALSE;
    }
    /* The battle camera folds its look-at into the projection and leaves no
     * camera modelview: the lists' modelview is then the world matrix alone
     * (ndsRendererAdapterPrepareInitialMatrices). */
    if (modelview_valid != FALSE)
    {
        ndsRendererMtxMul20p12(&modelview, &projection, &m);
    }
    else
    {
        m = projection;
    }
    for (plane = 0u; plane < 4u; plane++)
    {
        const u32 col = plane >> 1;
        const s64 sign = ((plane & 1u) != 0u) ? 1 : -1;
        s64 d = (s64)m.m[3][3] + (sign * (s64)m.m[3][col]);
        s64 span = 0;

        for (axis = 0u; axis < 3u; axis++)
        {
            s64 c = (s64)m.m[axis][3] + (sign * (s64)m.m[axis][col]);

            d += c * (s64)p[axis];
            span += (c >= 0) ? c : -c;
        }
        if ((d + (span * radius)) < 0)
        {
            NDS_DIAG(gNdsItemPreCulled++);
            return TRUE;
        }
    }
    return FALSE;
}

void ndsRendererAdapterSubmitItemDObjTree(void *dobj_ptr, u32 kind,
                                          void *camera_gobj_ptr,
                                          u32 initial_geometry_mode)
{
#if defined(NDS_LAB_FOURCPU_SWEEP) && NDS_LAB_FOURCPU_SWEEP
    u32 lab_offscreen = cpuGetTiming();
#endif
    const sb32 offscreen = ndsRendererAdapterItemOffscreen(
        (DObj *)dobj_ptr, (GObj *)camera_gobj_ptr);

#if defined(NDS_LAB_FOURCPU_SWEEP) && NDS_LAB_FOURCPU_SWEEP
    NDS_DIAG(gNdsLabItemAcc[11] += cpuGetTiming() - lab_offscreen);
#endif

    if (offscreen != FALSE)
    {
        return;
    }
    sNdsRendererAdapterItemSubmitActive = TRUE;
    sNdsRendererAdapterItemSubmitHead = 0u;
    ndsRendererAdapterSubmitStageDObjTreeDepth(dobj_ptr, kind, camera_gobj_ptr,
                                               initial_geometry_mode, 0u);
    sNdsRendererAdapterItemSubmitActive = FALSE;
    sNdsRendererAdapterItemSubmitHead = 0u;
}

/* The lists ndsRendererAdapterSubmitStageDObjTreeDepth would submit, in its
 * order: hidden subtrees skipped, siblings walked from the first child, each
 * drawable node's list (or DLLink lists) with the head the submit names.
 * Returns the count, or NDS_ITEM_REPLAY_ROOTS + 1 when it cannot hold them. */
static u32 ndsItemReplayWalk(DObj *dobj, u32 kind, NDSItemReplayRoot *roots,
                             u32 count, u32 depth)
{
    DObj *sibling;
    u32 seen;

    if ((dobj == NULL) || (count > NDS_ITEM_REPLAY_ROOTS))
    {
        return count;
    }
    if (depth >= NDS_RENDERER_STAGE_DOBJ_MAX_DEPTH)
    {
        return NDS_ITEM_REPLAY_ROOTS + 1u;
    }
    if ((dobj->flags & DOBJ_FLAG_HIDDEN) == 0u)
    {
        if (ndsRendererAdapterStageDObjDrawable(dobj, kind) != FALSE)
        {
            if ((kind == NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_TREE) ||
                (kind == NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_DLHEAD0) ||
                (kind == NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_DLHEAD1))
            {
                if (dobj->dv != NULL)
                {
                    if (count >= NDS_ITEM_REPLAY_ROOTS)
                    {
                        return NDS_ITEM_REPLAY_ROOTS + 1u;
                    }
                    roots[count].dobj = dobj;
                    roots[count].dl = dobj->dl;
                    roots[count].head =
                        (kind == NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_DLHEAD1) ?
                            1u : 0u;
                    count++;
                }
            }
            else if ((kind == NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_TREE_DLLINKS) ||
                     (kind == NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_DLLINKS))
            {
                const DObjDLLink *dl_link = dobj->dl_link;
                u32 link;

                for (link = 0u; (dl_link != NULL) &&
                     (link < GC_COMMON_MAX_DLLINKS); link++, dl_link++)
                {
                    if (dl_link->list_id == (s32)NDS_RENDERER_STAGE_DL_HEADS)
                    {
                        break;
                    }
                    if ((dl_link->list_id >= 0) &&
                        ((u32)dl_link->list_id < NDS_RENDERER_STAGE_DL_HEADS) &&
                        (dl_link->dl != NULL))
                    {
                        if (count >= NDS_ITEM_REPLAY_ROOTS)
                        {
                            return NDS_ITEM_REPLAY_ROOTS + 1u;
                        }
                        roots[count].dobj = dobj;
                        roots[count].dl = dl_link->dl;
                        roots[count].head = (u8)dl_link->list_id;
                        count++;
                    }
                }
            }
            else
            {
                return NDS_ITEM_REPLAY_ROOTS + 1u;
            }
        }
        if (dobj->child != NULL)
        {
            count = ndsItemReplayWalk(dobj->child, kind, roots, count,
                                      depth + 1u);
        }
    }
    if (dobj->sib_prev == NULL)
    {
        seen = 0u;
        for (sibling = dobj->sib_next;
             (sibling != NULL) && (count <= NDS_ITEM_REPLAY_ROOTS);
             sibling = sibling->sib_next)
        {
            if (++seen > NDS_RENDERER_STAGE_DOBJ_MAX_SIBLINGS)
            {
                return NDS_ITEM_REPLAY_ROOTS + 1u;
            }
            count = ndsItemReplayWalk(sibling, kind, roots, count,
                                      depth + 1u);
        }
    }
    return count;
}

/* Everything an owner's GX output for these lists depends on besides the
 * matrices and the resident textures. Returns the key length, 0 when the
 * draw cannot be keyed. */
static u32 ndsItemReplayKey(const DObj *root, u32 item_kind,
                            u32 initial_geometry_mode,
                            const NDSItemReplayRoot *roots, u32 root_count,
                            u32 *key)
{
    u32 head_seen = 0u;
    u32 n = 0u;
    u32 i;

    key[n++] = (u32)(uintptr_t)root;
    key[n++] = item_kind;
    key[n++] = initial_geometry_mode;
    key[n++] = (u32)sNdsFighterDisplayCurrentLightValid;
    key[n++] = sNdsFighterDisplayCurrentLightCount;
    key[n++] = ((u32)(u8)sNdsFighterDisplayCurrentLight.l.dir[0]) |
               ((u32)(u8)sNdsFighterDisplayCurrentLight.l.dir[1] << 8) |
               ((u32)(u8)sNdsFighterDisplayCurrentLight.l.dir[2] << 16);
    key[n++] = (u32)ndsRendererHardwareNoOracleEnabled();
    for (i = 0u; i < root_count; i++)
    {
        const NDSItemReplayRoot *r = &roots[i];
        const NDSStageDLRoute *route = ndsStageDLRouteSlot(r->dl);
        const u32 head = r->head;

        if ((route->dl != r->dl) ||
            (ndsItemReplayRouteOk(route->route) == FALSE) ||
            (route->loaded == NULL) ||
            (route->loaded->data != route->data))
        {
            return 0u;
        }
        key[n++] = (u32)(uintptr_t)r->dobj;
        key[n++] = (u32)(uintptr_t)r->dl;
        key[n++] = head | ((u32)route->route << 8);
        key[n++] = (u32)(uintptr_t)route->data;
        key[n++] = route->root;
        if ((head_seen & (1u << head)) == 0u)
        {
            head_seen |= 1u << head;
            key[n++] = sNdsRendererAdapterItemColorMask[head];
            key[n++] = sNdsRendererAdapterItemPrimColor[head];
            key[n++] = sNdsRendererAdapterItemEnvColor[head];
            key[n++] = sNdsRendererAdapterItemOtherModeL[head];
            key[n++] = sNdsRendererAdapterItemOtherModeLValid[head];
            key[n++] = sNdsRendererAdapterItemOtherModeH[head];
            key[n++] = sNdsRendererAdapterItemOtherModeHValid[head];
        }
    }
    return n;
}

/* One item GObj's tree: replayed when a recorded draw matches, else drawn by
 * its owners inside the stage traversal (recording when every list is a
 * replayable owner). */
void ndsRendererAdapterSubmitItemDObjTreeReplay(void *dobj_ptr, u32 kind,
                                                void *camera_gobj_ptr,
                                                u32 initial_geometry_mode,
                                                u32 item_kind)
{
    DObj *root = (DObj *)dobj_ptr;
    GObj *camera_gobj = (GObj *)camera_gobj_ptr;
    NDSItemReplayRoot roots[NDS_ITEM_REPLAY_ROOTS];
    u32 key[NDS_ITEM_REPLAY_KEY_WORDS];
    NDSItemReplayDraw *draw = NULL;
    NDSItemReplayDraw *victim = &sNdsItemReplayDraws[0];
#if defined(NDS_LAB_FOURCPU_SWEEP) && NDS_LAB_FOURCPU_SWEEP
    const NDSItemReplayDraw *verify_against = NULL;
#endif
    sb32 same_root = FALSE;
    u32 root_count = 0u;
    u32 key_count = 0u;
    u32 i;

    if ((sNdsItemReplayRecording == NULL) && (root != NULL) &&
        (ndsRendererAdapterItemOffscreen(root, camera_gobj) == FALSE))
    {
        root_count = ndsItemReplayWalk(root, kind, roots, 0u, 0u);
        if ((root_count != 0u) && (root_count <= NDS_ITEM_REPLAY_ROOTS) &&
            ((7u + (root_count * 12u)) <= NDS_ITEM_REPLAY_KEY_WORDS))
        {
            key_count = ndsItemReplayKey(root, item_kind,
                                         initial_geometry_mode, roots,
                                         root_count, key);
        }
    }
    if (key_count != 0u)
    {
        for (i = 0u; i < NDS_ITEM_REPLAY_DRAWS; i++)
        {
            NDSItemReplayDraw *d = &sNdsItemReplayDraws[i];

            if ((d->valid != 0u) && (d->root == root) &&
                (d->key_count == key_count) && (d->root_count == root_count) &&
                (memcmp(d->key, key, key_count * sizeof(u32)) == 0))
            {
                /* valid 2: this key failed its recording -- its owners draw
                 * it, unrecorded, until the key changes. */
                draw = (d->valid == 1u) ? d : NULL;
                if (draw == NULL)
                {
                    d->last_used = gNdsRendererProfileFrameCount;
                    key_count = 0u;
                }
                break;
            }
            if (d->root == root)
            {
                victim = d;       /* this item's stale draw: replace it */
                same_root = TRUE;
            }
            else if ((same_root == FALSE) &&
                     ((d->valid == 0u) ||
                      ((victim->valid != 0u) &&
                       (d->last_used < victim->last_used))))
            {
                victim = d;
            }
        }
        if ((draw != NULL) &&
            (ndsNativeItemReplayEmitsResident(draw->emits, draw->emit_count) ==
             FALSE))
        {
            NDS_DIAG(gNdsItemReplayNotResident++);
            victim = draw;
            draw = NULL;
        }
#if defined(NDS_LAB_FOURCPU_SWEEP) && NDS_LAB_FOURCPU_SWEEP
        if ((draw != NULL) && (gNdsItemReplayVerify != 0u))
        {
            /* Draw through the owners into a scratch recording and compare
             * it with the cached one (below). */
            verify_against = draw;
            victim = &sNdsItemReplayVerifyDraw;
            draw = NULL;
        }
#endif
    }
    if (draw != NULL)
    {
        CObj *cobj = (camera_gobj != NULL) ? CObjGetStruct(camera_gobj) :
            ((gGCCurrentCamera != NULL) ? CObjGetStruct(gGCCurrentCamera) :
                                          NULL);
        u32 triangles = 0u;

        sNdsRendererAdapterItemSubmitActive = TRUE;
        for (i = 0u; i < draw->root_count; i++)
        {
            const NDSItemReplayRoot *r = &draw->roots[i];
            NDSRendererMatrix20p12 projection;
            NDSRendererMatrix20p12 modelview;
            NDSRendererMatrix20p12 identity;
            const NDSRendererMatrix20p12 *projection_ptr;
            const NDSRendererMatrix20p12 *modelview_ptr;
            void *saved_graphics_heap_ptr = gSYTaskmanGraphicsHeap.ptr;

#if defined(NDS_LAB_FOURCPU_SWEEP) && NDS_LAB_FOURCPU_SWEEP
            u32 lab_replay_mark = cpuGetTiming();
            const u32 lab_pim0 = gNdsLabPimAcc[0];
            const u32 lab_pim1 = gNdsLabPimAcc[1];
            const u32 lab_pim2 = gNdsLabPimAcc[2];
#endif
            /* The fast lane's matrix preparation for this list, unchanged:
             * a held item's attach builds (and latches) exactly here. */
            ndsRendererAdapterPrepareInitialMatrices(
                r->dobj, cobj, TRUE, &projection, &projection_ptr,
                &modelview, &modelview_ptr);
#if defined(NDS_LAB_FOURCPU_SWEEP) && NDS_LAB_FOURCPU_SWEEP
            {
                const u32 lab_now = cpuGetTiming();

                NDS_DIAG(gNdsLabItemAcc[14] += lab_now - lab_replay_mark);
                NDS_DIAG(gNdsLabItemAcc[5] += gNdsLabPimAcc[0] - lab_pim0);
                NDS_DIAG(gNdsLabItemAcc[6] += gNdsLabPimAcc[1] - lab_pim1);
                NDS_DIAG(gNdsLabItemAcc[7] += gNdsLabPimAcc[2] - lab_pim2);
                lab_replay_mark = lab_now;
            }
#endif
            if ((projection_ptr == NULL) && (modelview_ptr != NULL))
            {
                ndsRendererAdapterMtxIdentity20p12(&identity);
                projection_ptr = &identity;
            }
            else if ((modelview_ptr == NULL) && (projection_ptr != NULL))
            {
                ndsRendererAdapterMtxIdentity20p12(&identity);
                modelview_ptr = &identity;
            }
            if ((projection_ptr != NULL) && (modelview_ptr != NULL))
            {
                /* The persistent stats are dead between traversals (every
                 * reader begins one first): the batch opens borrow them. */
                triangles += ndsNativeItemReplayEmits(
                    &draw->emits[r->emit_first], r->emit_count, draw->pool,
                    projection_ptr, modelview_ptr,
                    &sNdsRendererAdapterStagePersistentStats);
            }
            gSYTaskmanGraphicsHeap.ptr = saved_graphics_heap_ptr;
#if defined(NDS_LAB_FOURCPU_SWEEP) && NDS_LAB_FOURCPU_SWEEP
            NDS_DIAG(gNdsLabItemAcc[3] += cpuGetTiming() - lab_replay_mark);
#endif
        }
        sNdsRendererAdapterItemSubmitActive = FALSE;
        sNdsRendererAdapterItemSubmitHead = 0u;
        gNdsStageGCDrawAllLoopHardwareTriangleCount += triangles;
        NDS_DIAG(gNdsStageGCDrawAllLoopHardwareZBufferTriangleCount += triangles);
        draw->last_used = gNdsRendererProfileFrameCount;
        NDS_DIAG(gNdsItemReplayDraws++);
        return;
    }
    if (key_count != 0u)
    {
        /* Record this draw while its owners run. */
        victim->valid = 0u;
        victim->root = root;
        victim->root_count = (u8)root_count;
        victim->key_count = (u8)key_count;
        victim->emit_count = 0u;
        for (i = 0u; i < root_count; i++)
        {
            victim->roots[i] = roots[i];
            victim->roots[i].emit_first = 0u;
            victim->roots[i].emit_count = 0u;
        }
        memcpy(victim->key, key, key_count * sizeof(u32));
        sNdsItemReplaySinkState.emits = victim->emits;
        sNdsItemReplaySinkState.pool = victim->pool;
        sNdsItemReplaySinkState.emit_count = 0u;
        sNdsItemReplaySinkState.word_count = 0u;
        sNdsItemReplaySinkState.failed = 0u;
        sNdsItemReplayRecording = victim;
        sNdsItemReplayRecordRoots = 0u;
        sNdsItemReplayRecordEmpty = 0u;
        ndsNativeItemReplaySetSink(&sNdsItemReplaySinkState);
    }
    ndsRendererAdapterBeginStageTraversal();
    ndsRendererAdapterSubmitItemDObjTree(dobj_ptr, kind, camera_gobj_ptr,
                                         initial_geometry_mode);
    ndsRendererAdapterEndStageTraversal();
    if (key_count != 0u)
    {
        ndsNativeItemReplaySetSink(NULL);
        sNdsItemReplayRecording = NULL;
        if (sNdsItemReplayRecordEmpty != 0u)
        {
            victim->valid = 0u;
        }
        else if ((sNdsItemReplaySinkState.failed == 0u) &&
                 (sNdsItemReplayRecordRoots == root_count))
        {
            victim->emit_count = (u8)sNdsItemReplaySinkState.emit_count;
            victim->last_used = gNdsRendererProfileFrameCount;
            victim->valid = 1u;
            NDS_DIAG(gNdsItemReplayRecords++);
        }
        else
        {
            victim->last_used = gNdsRendererProfileFrameCount;
            victim->valid = 2u;
            NDS_DIAG(gNdsItemReplayRecordFailed++);
        }
#if defined(NDS_LAB_FOURCPU_SWEEP) && NDS_LAB_FOURCPU_SWEEP
        if (verify_against != NULL)
        {
            NDS_DIAG(gNdsItemReplayVerifyRuns++);
            if (ndsItemReplaySameRecording(victim, verify_against) == FALSE)
            {
                NDS_DIAG(gNdsItemReplayVerifyFail++);
            }
            victim->valid = 0u;
        }
#endif
    }
}

void ndsRendererAdapterSubmitWeaponDObjTree(void *dobj_ptr, u32 kind,
                                            void *camera_gobj_ptr,
                                            u32 initial_geometry_mode)
{
    ndsRendererAdapterSubmitStageDObjTreeDepth(dobj_ptr, kind, camera_gobj_ptr,
                                               initial_geometry_mode, 0u);
}

void ndsRendererAdapterSubmitStageDObj(void *dobj_ptr, u32 kind,
                                       void *camera_gobj_ptr,
                                       u32 initial_geometry_mode)
{
    ndsRendererAdapterSubmitStageDObjNode(dobj_ptr, kind, camera_gobj_ptr,
                                          initial_geometry_mode);
}

/* P2 (2026-10-04): SECTOR Z'S ARWING IN TWO PASSES.
 *
 * The Arwing draws seven FoxSpecial3 roots a frame (the body and six glow
 * quads), each through the scan, the stage DL submit, the entry-effect
 * adapter, matrix preparation and the 15 KB entry-effect executor -- 20-25 KB
 * of code per list against an 8 KB instruction cache. The lab profile of the
 * owner's 4P all-items match (artifacts/task37-census/sz-szprof01) put the
 * Arwing frames' premium at ~230K cycles, nearly all of it memory stall
 * (7-18 cycles per instruction), and a two-triangle glow cost 13-18K ticks.
 *
 * So the tree is walked once, every root's matrices are prepared in one pass
 * (the same ndsRendererAdapterPrepareInitialMatrices call, results kept per
 * list), and then every list is submitted in source order through the
 * unchanged path, which takes the prepared matrices instead of preparing them
 * again. Matrix preparation reads only the world, camera and recalc caches,
 * none of which a submit touches, so moving it ahead changes no value. The
 * walk mirrors ndsStageGCDrawAllLoopScanDObjs + SubmitStageDObjNode for
 * TREE_DLLINKS (hidden subtrees skipped, NOTEXTURE lists skipped, list ids
 * below NDS_RENDERER_STAGE_DL_HEADS); anything it cannot hold declines before
 * drawing. */
#define NDS_ARWING_TWO_PASS_MAX 12u
static NDSRendererAdapterEntryPrepared
    sNdsRendererAdapterArwingItems[NDS_ARWING_TWO_PASS_MAX];
volatile u32 gNdsArwingTwoPassDraws;
volatile u32 gNdsArwingTwoPassDeclines;

static sb32 ndsRendererAdapterIsArwingRoot(const Gfx *dl)
{
    u32 offset;

    if ((gFTDataFoxSpecial3 == NULL) ||
        ((const u8 *)dl < (const u8 *)gFTDataFoxSpecial3))
    {
        return FALSE;
    }
    offset = (u32)((const u8 *)dl - (const u8 *)gFTDataFoxSpecial3);
    switch (offset)
    {
    case 0x1fa0u:
    case 0x2920u:
    case 0x29d0u:
    case 0x29f0u:
    case 0x2a20u:
    case 0x2868u:
    case 0x2a50u:
    case 0x2b00u:
        return TRUE;
    default:
        return FALSE;
    }
}

/* A prepared Arwing root whose recorded packet replays: what the ordinary
 * submit does for it (the stage DL submit admits FoxSpecial3 lists to
 * ndsRendererAdapterTryNativeEntryEffect, whose stats seed, config and
 * counters these are) around the executor's replay branch alone
 * (ndsRendererReplayNativeEntryEffectFox). FALSE before any GX write
 * whenever that branch is not the one the executor would take; the caller
 * then takes the ordinary submit. */
static sb32 ndsRendererAdapterSubmitArwingRootReplay(
    const NDSRendererAdapterEntryPrepared *item, u32 initial_geometry_mode)
{
    NDSRendererConfig config;
    NDSRendererStats stats;
    NDSRendererMatrix20p12 identity;
    const NDSRendererMatrix20p12 *projection_ptr =
        (item->projection_valid != 0u) ? &item->projection : NULL;
    const NDSRendererMatrix20p12 *modelview_ptr =
        (item->modelview_valid != 0u) ? &item->modelview : NULL;

    if ((gFTDataFoxSpecial3 == NULL) ||
        ((const u8 *)item->dl < (const u8 *)gFTDataFoxSpecial3))
    {
        return FALSE;
    }
    /* The adapter's split-camera fill. */
    if ((projection_ptr == NULL) && (modelview_ptr != NULL))
    {
        ndsRendererAdapterMtxIdentity20p12(&identity);
        projection_ptr = &identity;
    }
    else if ((modelview_ptr == NULL) && (projection_ptr != NULL))
    {
        ndsRendererAdapterMtxIdentity20p12(&identity);
        modelview_ptr = &identity;
    }
    if ((projection_ptr == NULL) || (modelview_ptr == NULL))
    {
        return FALSE;
    }
    ndsRendererAdapterEntryEffectSeedStats(item->dobj, &stats);
    memset(&config, 0, sizeof(config));
    config.max_depth = 4u;
    config.max_commands = 1u;
    config.max_list_commands = 1u;
    config.initial_projection = projection_ptr;
    config.initial_modelview = modelview_ptr;
    config.initial_geometry_mode = initial_geometry_mode;
    config.texture_data_layout = NDS_RENDERER_TEXTURE_DATA_O2R_WORD_SWAPPED;
    if (ndsRendererReplayNativeEntryEffectFox(
            (u32)((const u8 *)item->dl - (const u8 *)gFTDataFoxSpecial3),
            &config, &stats) == FALSE)
    {
        return FALSE;
    }
    ndsRendererAdapterEntryEffectAccumulate(&stats);
    return TRUE;
}

s32 ndsRendererAdapterSubmitArwingTwoPass(void *root_ptr, void *camera_gobj_ptr,
                                          u32 initial_geometry_mode,
                                          u32 *submitted_dobjs)
{
    DObj *root = (DObj *)root_ptr;
    GObj *camera_gobj = (GObj *)camera_gobj_ptr;
    NDSRendererAdapterEntryPrepared *items = sNdsRendererAdapterArwingItems;
    DObj *stack[32];
    CObj *cobj;
    u32 stack_count = 0u;
    u32 scanned = 0u;
    u32 count = 0u;
    u32 dobjs = 0u;
    u32 i;

    if ((root == NULL) ||
        (submitted_dobjs == NULL))
    {
        return FALSE;
    }
    stack[stack_count++] = root;
    while ((stack_count != 0u) && (scanned < ARRAY_COUNT(stack)))
    {
        DObj *dobj = stack[--stack_count];

        if (dobj == NULL)
        {
            continue;
        }
        scanned++;
        /* Two pushes at most below; a walk that could drop one declines. */
        if (stack_count + 2u > ARRAY_COUNT(stack))
        {
            NDS_DIAG(gNdsArwingTwoPassDeclines++);
            return FALSE;
        }
        if ((dobj->flags & DOBJ_FLAG_HIDDEN) != 0)
        {
            if (dobj->sib_next != NULL)
            {
                stack[stack_count++] = dobj->sib_next;
            }
            continue;
        }
        if ((dobj->dv != NULL) && ((dobj->flags & DOBJ_FLAG_NOTEXTURE) == 0))
        {
            DObjDLLink *dl_link = dobj->dl_link;

            dobjs++;
            for (i = 0u; (dl_link != NULL) && (i < GC_COMMON_MAX_DLLINKS);
                 i++, dl_link++)
            {
                if (dl_link->list_id == (s32)NDS_RENDERER_STAGE_DL_HEADS)
                {
                    break;
                }
                if ((dl_link->list_id >= 0) &&
                    ((u32)dl_link->list_id < NDS_RENDERER_STAGE_DL_HEADS) &&
                    (dl_link->dl != NULL))
                {
                    if (count >= NDS_ARWING_TWO_PASS_MAX)
                    {
                        NDS_DIAG(gNdsArwingTwoPassDeclines++);
                        return FALSE;
                    }
                    items[count].dobj = dobj;
                    items[count].dl = dl_link->dl;
                    items[count].list_id = (u32)dl_link->list_id;
                    count++;
                }
            }
        }
        if (dobj->sib_next != NULL)
        {
            stack[stack_count++] = dobj->sib_next;
        }
        if (dobj->child != NULL)
        {
            stack[stack_count++] = dobj->child;
        }
    }
    if (stack_count != 0u)
    {
        /* More DObjs than the walk holds: let the scan draw it. */
        NDS_DIAG(gNdsArwingTwoPassDeclines++);
        return FALSE;
    }

    cobj = (camera_gobj != NULL) ? CObjGetStruct(camera_gobj) :
        ((gGCCurrentCamera != NULL) ? CObjGetStruct(gGCCurrentCamera) : NULL);
    for (i = 0u; i < count; i++)
    {
        NDSRendererAdapterEntryPrepared *item = &items[i];
        const NDSRendererMatrix20p12 *projection_ptr = NULL;
        const NDSRendererMatrix20p12 *modelview_ptr = NULL;

        item->prepared = FALSE;
        if (ndsRendererAdapterIsArwingRoot(item->dl) == FALSE)
        {
            continue;
        }
        ndsRendererAdapterPrepareInitialMatrices(
            item->dobj, cobj, FALSE, &item->projection, &projection_ptr,
            &item->modelview, &modelview_ptr);
        item->projection_valid = (projection_ptr != NULL) ? 1u : 0u;
        item->modelview_valid = (modelview_ptr != NULL) ? 1u : 0u;
        item->prepared = TRUE;
    }
    for (i = 0u; i < count; i++)
    {
        NDSRendererAdapterEntryPrepared *item = &items[i];

        sNdsRendererAdapterEffectSubmitHead = item->list_id;
        if ((item->prepared != FALSE) &&
            (ndsRendererAdapterSubmitArwingRootReplay(
                 item, initial_geometry_mode) != FALSE))
        {
            continue;
        }
        sNdsRendererAdapterEntryPrepared =
            (item->prepared != FALSE) ? item : NULL;
        ndsRendererAdapterSubmitStageDL(item->dobj, item->dl, camera_gobj,
                                        initial_geometry_mode);
        sNdsRendererAdapterEntryPrepared = NULL;
    }
    *submitted_dobjs = dobjs;
    NDS_DIAG(gNdsArwingTwoPassDraws++);
    return TRUE;
}

static void ndsRendererAdapterSubmitStageDObjNode(DObj *dobj, u32 kind,
                                                  GObj *camera_gobj,
                                                  u32 initial_geometry_mode)
{
    DObjDLLink *dl_link;
    u32 i;

    if (ndsRendererAdapterStageDObjDrawable(dobj, kind) == FALSE)
    {
        return;
    }

    switch (kind)
    {
    case NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_TREE:
    case NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_DLHEAD0:
    case NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_DLHEAD1:
        if (dobj->dv != NULL)
        {
            if (sNdsRendererAdapterItemSubmitActive != FALSE)
            {
                sNdsRendererAdapterItemSubmitHead =
                    (kind == NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_DLHEAD1) ?
                        1u : 0u;
            }
            sNdsRendererAdapterEffectSubmitHead =
                (kind == NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_DLHEAD1) ? 1u : 0u;
            ndsRendererAdapterSubmitStageDL(dobj, dobj->dl, camera_gobj,
                                            initial_geometry_mode);
        }
        break;

    case NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_TREE_DLLINKS:
    case NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_DLLINKS:
        dl_link = dobj->dl_link;
        if (dl_link == NULL)
        {
            return;
        }
        for (i = 0u; i < GC_COMMON_MAX_DLLINKS; i++, dl_link++)
        {
            if (dl_link->list_id == (s32)NDS_RENDERER_STAGE_DL_HEADS)
            {
                break;
            }
            if ((dl_link->list_id >= 0) &&
                ((u32)dl_link->list_id < NDS_RENDERER_STAGE_DL_HEADS) &&
                (dl_link->dl != NULL))
            {
                if (sNdsRendererAdapterItemSubmitActive != FALSE)
                {
                    sNdsRendererAdapterItemSubmitHead =
                        (u32)dl_link->list_id;
                }
                sNdsRendererAdapterEffectSubmitHead = (u32)dl_link->list_id;
                ndsRendererAdapterSubmitStageDL(dobj, dl_link->dl,
                                                camera_gobj,
                                                initial_geometry_mode);
            }
        }
        break;

    default:
        break;
    }
}
