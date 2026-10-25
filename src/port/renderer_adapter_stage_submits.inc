/* The general stage DL submit's owner submits: every candidate the claims
 * armed is drawn by its native owner on the prepared config and matrices.
 * A refusal records its loud NO_PROGRAM failure; nothing falls back to a
 * generic route.
 *
 * Included in place by ndsRendererAdapterSubmitStageDLBody
 * (renderer_adapter_stage_submit.c): its text reads and writes that
 * function's locals, so it is not a file of its own. */

    if (rebirth_halo_native_candidate != FALSE)
    {
        rebirth_halo_native_handled = ndsRendererSubmitNativeRebirthHalo(
            rebirth_halo_root_offset, &config, render_stats);
        if (rebirth_halo_native_handled != FALSE)
        {
            NDS_DIAG(gNdsRebirthHaloNativeDrawCount++);
        }
        else
        {
            NDS_DIAG(gNdsRebirthHaloNativeFallbackCount++);
        }
    }
#if NDS_STAGE_INISHIE
    if (inishie_pakkun_native_candidate != FALSE)
    {
        /* Same split-camera contract the entry-effect owner documents at
         * :5142: the default battle camera legitimately supplies the whole
         * transform on one side of the DS pair, and a fixed owner has no
         * matrix stream to fill the other implicitly. Measured here: this
         * item arrives with a live modelview and a NULL projection, and the
         * submit declined all 600 draws of a 300-present run on exactly
         * that. Make the identity explicit on a copy rather than mutating
         * the shared config the other owners below still read. */
        NDSRendererConfig pakkun_config = config;
        NDSRendererMatrix20p12 pakkun_identity;

        if ((pakkun_config.initial_projection == NULL) &&
            (pakkun_config.initial_modelview != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&pakkun_identity);
            pakkun_config.initial_projection = &pakkun_identity;
        }
        else if ((pakkun_config.initial_modelview == NULL) &&
                 (pakkun_config.initial_projection != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&pakkun_identity);
            pakkun_config.initial_modelview = &pakkun_identity;
        }
        inishie_pakkun_native_handled = ndsRendererSubmitNativeInishiePakkun(
            loaded->data, loaded->data_size,
            (const u8 *)inishie_pakkun_palette_file->data + 0x3620u,
            &inishie_pakkun_material, &pakkun_config, render_stats);
        if (inishie_pakkun_native_handled != FALSE)
        {
            NDS_DIAG(gNdsInishiePakkunDrawCount++);
            ndsStageDLRouteRecord(dl, loaded, 0x0b40u,
                                  NDS_SDL_ROUTE_INISHIE_PAKKUN);
        }
        else
        {
            NDS_DIAG(gNdsInishiePakkunSubmitFailCount++);
        }
    }
    if (inishie_powblock_native_candidate != FALSE)
    {
        /* Same split-camera contract the Pakkun owner documents above: fill
         * the identity on a COPY, because the owners below still read the
         * shared config. */
        NDSRendererConfig powblock_config = config;
        NDSRendererMatrix20p12 powblock_identity;

        if ((powblock_config.initial_projection == NULL) &&
            (powblock_config.initial_modelview != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&powblock_identity);
            powblock_config.initial_projection = &powblock_identity;
        }
        else if ((powblock_config.initial_modelview == NULL) &&
                 (powblock_config.initial_projection != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&powblock_identity);
            powblock_config.initial_modelview = &powblock_identity;
        }
        inishie_powblock_native_handled =
            ndsRendererSubmitNativeInishiePowblock(
                inishie_powblock_tlut, inishie_powblock_image_a,
                inishie_powblock_image_b, &powblock_config, render_stats);
        if (inishie_powblock_native_handled != FALSE)
        {
            NDS_DIAG(gNdsInishiePowblockDrawCount++);
            ndsStageDLRouteRecord(dl, loaded,
                                  NDS_NATIVE_INISHIE_POWBLOCK_ROOT,
                                  NDS_SDL_ROUTE_INISHIE_POWBLOCK);
        }
        else
        {
            /* No fallback: a refusal falls through to the loud NO_PROGRAM
             * record below, never to a generic route. */
            NDS_DIAG(gNdsInishiePowblockSubmitFailCount++);
        }
    }
#endif
    if (ness_pkfire_native_candidate != FALSE)
    {
        NDSRendererConfig pkfire_config = config;
        NDSRendererMatrix20p12 pkfire_identity;

        if ((pkfire_config.initial_projection == NULL) &&
            (pkfire_config.initial_modelview != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&pkfire_identity);
            pkfire_config.initial_projection = &pkfire_identity;
        }
        else if ((pkfire_config.initial_modelview == NULL) &&
                 (pkfire_config.initial_projection != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&pkfire_identity);
            pkfire_config.initial_modelview = &pkfire_identity;
        }
        ness_pkfire_native_handled = ndsRendererSubmitNativeNessPKFire(
            ness_pkfire_materials, NDS_NATIVE_NESS_PKFIRE_GROUP_COUNT,
            &pkfire_config, render_stats);
    }
    if (ness_pkthunder_native_candidate != FALSE)
    {
        NDSRendererConfig pkthunder_config = config;
        NDSRendererMatrix20p12 pkthunder_identity;

        if ((pkthunder_config.initial_projection == NULL) &&
            (pkthunder_config.initial_modelview != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&pkthunder_identity);
            pkthunder_config.initial_projection = &pkthunder_identity;
        }
        else if ((pkthunder_config.initial_modelview == NULL) &&
                 (pkthunder_config.initial_projection != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&pkthunder_identity);
            pkthunder_config.initial_modelview = &pkthunder_identity;
        }
        ness_pkthunder_native_handled =
            ndsRendererSubmitNativeNessPKThunder(
                ness_pkthunder_root_index, &ness_pkthunder_material,
                ness_pkthunder_trail_color, &pkthunder_config, render_stats);
        if (ness_pkthunder_native_handled != FALSE)
        {
            ndsStageDLRouteRecord(dl, loaded,
                (ness_pkthunder_root_index ==
                 NDS_NATIVE_NESS_PKTHUNDER_HEAD_INDEX) ?
                    NDS_NATIVE_NESS_PKTHUNDER_HEAD_ROOT :
                    NDS_NATIVE_NESS_PKTHUNDER_TRAIL_ROOT,
                NDS_SDL_ROUTE_NESS_PKTHUNDER);
        }
    }
    if (purin_sing_native_candidate != FALSE)
    {
        NDSRendererConfig sing_config = config;
        NDSRendererMatrix20p12 identity;
        ndsRendererAdapterMtxIdentity20p12(&identity);
        if (sing_config.initial_projection == NULL) sing_config.initial_projection = &identity;
        if (sing_config.initial_modelview == NULL) sing_config.initial_modelview = &identity;
        purin_sing_native_handled = ndsRendererSubmitNativePurinSing(
            purin_sing_root_index, &purin_sing_material, purin_sing_image,
            &sing_config, render_stats);
    }
    if (loaded != NULL && (loaded->asset_id == NDS_NATIVE_PIKACHU_THUNDER_MODEL ||
                          loaded->asset_id == NDS_NATIVE_PIKACHU_THUNDER_SPECIAL))
    {
        pikachu_thunder_native_handled = ndsRendererAdapterPikachuThunder(
            loaded, dobj, dl, &config, render_stats);
        if (pikachu_thunder_native_handled != FALSE)
        {
            ndsStageDLRouteRecord(dl, loaded,
                                  ndsRelocNativeRootOffset(loaded, dl),
                                  NDS_SDL_ROUTE_PIKACHU_THUNDER);
        }
    }
    /* The last PK Thunder trail segment is an EFFECT, not a weapon
     * (efManagerNessPKThunderTrailMakeEffect): its own list at 0x8F98 with a
     * fixed image and fixed prim/env, drawn through gcDrawDObjDLLinksForGObj. */
    if ((loaded != NULL) && (loaded->asset_id == NDS_NATIVE_NESS_PKTAIL_ASSET) &&
        (dobj->parent_gobj != NULL) && (dobj->parent_gobj->id == nGCCommonKindEffect) &&
        (ndsRelocNativeRootOffset(loaded, dl) == NDS_NATIVE_NESS_PKTAIL_ROOT))
    {
        NDSRendererConfig tail_config = config;
        NDSRendererMatrix20p12 identity;
#if (NDS_1P_GAME || NDS_MENU_SHELL) || NDS_COMPACT_BATTLE_FIGHTERS
        const void *tail_image = ndsRelocNativeAssetAddress(loaded->data, NDS_NATIVE_NESS_PKTAIL_IMAGE);
#else
        const void *tail_image = (loaded->data_size >= NDS_NATIVE_NESS_PKTAIL_IMAGE_END) ?
            (const void *)((u8 *)loaded->data + NDS_NATIVE_NESS_PKTAIL_IMAGE) : NULL;
#endif
        ndsRendererAdapterMtxIdentity20p12(&identity);
        if (tail_config.initial_projection == NULL) tail_config.initial_projection = &identity;
        if (tail_config.initial_modelview == NULL) tail_config.initial_modelview = &identity;
        ness_pktail_native_handled = ndsRendererSubmitNativeNessPKTail(
            tail_image, &tail_config, render_stats);
    }
    /* Samus Bomb: one SamusModel billboard. The live MObj only swaps the
     * palette; texels, LOADTLUT and tile state are the source list's own. */
    if ((loaded != NULL) && (loaded->asset_id == NDS_NATIVE_SAMUS_BOMB_ASSET) &&
        (dobj->parent_gobj != NULL) && (dobj->parent_gobj->id == nGCCommonKindWeapon) &&
        (dobj->mobj != NULL) && (dobj->mobj->next == NULL) &&
        (ndsRelocNativeRootOffset(loaded, dl) == NDS_NATIVE_SAMUS_BOMB_ROOT))
    {
        NDSRendererNativeMaterial bomb_material;
        NDSRendererConfig bomb_config = config;
        NDSRendererMatrix20p12 identity;
#if (NDS_1P_GAME || NDS_MENU_SHELL) || NDS_COMPACT_BATTLE_FIGHTERS
        const void *bomb_image = ndsRelocNativeAssetAddress(loaded->data, NDS_NATIVE_SAMUS_BOMB_IMAGE);
#else
        const void *bomb_image = (loaded->data_size >= NDS_NATIVE_SAMUS_BOMB_IMAGE_END) ?
            (const void *)((u8 *)loaded->data + NDS_NATIVE_SAMUS_BOMB_IMAGE) : NULL;
#endif
        ndsRendererAdapterMtxIdentity20p12(&identity);
        if (bomb_config.initial_projection == NULL) bomb_config.initial_projection = &identity;
        if (bomb_config.initial_modelview == NULL) bomb_config.initial_modelview = &identity;
        if (ndsRendererAdapterBuildNativeMaterialSnapshot(dobj->mobj, &bomb_material, FALSE, NULL, NULL) != FALSE)
            samus_bomb_native_handled = ndsRendererSubmitNativeSamusBomb(
                &bomb_material, bomb_image, &bomb_config, render_stats);
    }
    if ((loaded != NULL) && (loaded->asset_id == NDS_NATIVE_KIRBY_VULCAN_ASSET) &&
        (dobj->parent_gobj != NULL) && (dobj->parent_gobj->id == nGCCommonKindEffect) &&
        (dobj->mobj == NULL))
    {
        u32 root = ndsRelocNativeRootOffset(loaded, dl);
        if (root == NDS_NATIVE_KIRBY_VULCAN_ROOT0 || root == NDS_NATIVE_KIRBY_VULCAN_ROOT1)
        {
            NDSRendererConfig vulcan_config = config;
            NDSRendererMatrix20p12 identity;
            ndsRendererAdapterMtxIdentity20p12(&identity);
            if (vulcan_config.initial_projection == NULL) vulcan_config.initial_projection = &identity;
            if (vulcan_config.initial_modelview == NULL) vulcan_config.initial_modelview = &identity;
            kirby_vulcan_native_handled = ndsRendererSubmitNativeKirbyVulcan(
                (root == NDS_NATIVE_KIRBY_VULCAN_ROOT0) ? 0u : 1u, &vulcan_config, render_stats);
        }
    }
    if (yoshi_entryegg_native_candidate != FALSE)
    {
        NDSRendererConfig entryegg_config = config;
        NDSRendererMatrix20p12 entryegg_identity;

        if ((entryegg_config.initial_projection == NULL) &&
            (entryegg_config.initial_modelview != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&entryegg_identity);
            entryegg_config.initial_projection = &entryegg_identity;
        }
        else if ((entryegg_config.initial_modelview == NULL) &&
                 (entryegg_config.initial_projection != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&entryegg_identity);
            entryegg_config.initial_modelview = &entryegg_identity;
        }
        yoshi_entryegg_native_handled = ndsRendererSubmitNativeYoshiEntryEgg(
            yoshi_entryegg_palette, &yoshi_entryegg_material,
            &entryegg_config, render_stats);
    }
    if (yoshi_egg_native_candidate != FALSE)
    {
        NDSRendererConfig egg_config = config;
        NDSRendererMatrix20p12 egg_identity;

        if ((egg_config.initial_projection == NULL) &&
            (egg_config.initial_modelview != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&egg_identity);
            egg_config.initial_projection = &egg_identity;
        }
        else if ((egg_config.initial_modelview == NULL) &&
                 (egg_config.initial_projection != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&egg_identity);
            egg_config.initial_modelview = &egg_identity;
        }
        if (yoshi_egg_is_weapon != FALSE)
        {
            /* wpYoshiEggThrowProcDisplay writes EnvColor(0,0,0,0) immediately
             * before wpDisplayDLHead1. Effects arrive with their callback ENV
             * already captured into render_stats. */
            render_stats->env_color = 0u;
        }
        yoshi_egg_native_handled = ndsRendererSubmitNativeYoshiEgg(
            &egg_config, render_stats);
    }
    if (yoshi_egglay_native_candidate != FALSE)
    {
        NDSRendererConfig egglay_config = config;
        NDSRendererMatrix20p12 egglay_identity;

        if ((egglay_config.initial_projection == NULL) &&
            (egglay_config.initial_modelview != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&egglay_identity);
            egglay_config.initial_projection = &egglay_identity;
        }
        else if ((egglay_config.initial_modelview == NULL) &&
                 (egglay_config.initial_projection != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&egglay_identity);
            egglay_config.initial_modelview = &egglay_identity;
        }
        yoshi_egglay_native_handled = ndsRendererSubmitNativeYoshiEggLay(
            yoshi_egglay_palette, yoshi_egglay_image,
            &egglay_config, render_stats);
    }
    if (charge_shot_native_candidate != FALSE)
    {
        NDSRendererConfig charge_shot_config = config;
        NDSRendererMatrix20p12 charge_shot_identity;

        if ((charge_shot_config.initial_projection == NULL) &&
            (charge_shot_config.initial_modelview != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&charge_shot_identity);
            charge_shot_config.initial_projection = &charge_shot_identity;
        }
        else if ((charge_shot_config.initial_modelview == NULL) &&
                 (charge_shot_config.initial_projection != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&charge_shot_identity);
            charge_shot_config.initial_modelview = &charge_shot_identity;
        }
        charge_shot_native_handled = ndsRendererSubmitNativeSamusChargeShot(
            loaded->data, loaded->data_size, &charge_shot_config,
            render_stats);
        if (charge_shot_native_handled != FALSE)
        {
            NDS_DIAG(gNdsChargeShotDrawCount++);
            ndsStageDLRouteRecord(dl, loaded, NDS_NATIVE_CHARGESHOT_ROOT,
                                  NDS_SDL_ROUTE_CHARGE_SHOT);
        }
        else
        {
            NDS_DIAG(gNdsChargeShotSubmitFailCount++);
        }
    }
    if (thunder_jolt_native_candidate != FALSE)
    {
        /* Same split-camera contract every fixed owner documents: fill the
         * identity on a COPY, because later code still reads the shared
         * config. */
        NDSRendererConfig jolt_config = config;
        NDSRendererMatrix20p12 jolt_identity;

        if ((jolt_config.initial_projection == NULL) &&
            (jolt_config.initial_modelview != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&jolt_identity);
            jolt_config.initial_projection = &jolt_identity;
        }
        else if ((jolt_config.initial_modelview == NULL) &&
                 (jolt_config.initial_projection != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&jolt_identity);
            jolt_config.initial_modelview = &jolt_identity;
        }
        thunder_jolt_native_handled = ndsRendererSubmitNativePikachuThunderJolt(
            loaded->data, loaded->data_size, &jolt_config, render_stats);
        if (thunder_jolt_native_handled != FALSE)
        {
            NDS_DIAG(gNdsThunderJoltDrawCount++);
        }
        else
        {
            /* No fallback: a refusal falls through to the loud NO_PROGRAM
             * record below, never to a generic route. */
            NDS_DIAG(gNdsThunderJoltSubmitFailCount++);
        }
    }
    if (thunder_ground_native_candidate != FALSE)
    {
        /* Same split-camera contract every fixed owner documents: fill the
         * identity on a COPY, because later code reads the shared config. */
        NDSRendererConfig ground_config = config;
        NDSRendererMatrix20p12 ground_identity;

        if ((ground_config.initial_projection == NULL) &&
            (ground_config.initial_modelview != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&ground_identity);
            ground_config.initial_projection = &ground_identity;
        }
        else if ((ground_config.initial_modelview == NULL) &&
                 (ground_config.initial_projection != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&ground_identity);
            ground_config.initial_modelview = &ground_identity;
        }
        thunder_ground_native_handled =
            ndsRendererSubmitNativePikachuThunderGround(
                thunder_ground_root_index, &thunder_ground_material,
                &ground_config, render_stats);
        if (thunder_ground_native_handled != FALSE)
        {
            NDS_DIAG(gNdsThunderGroundDrawCount++);
        }
        else
        {
            NDS_DIAG(gNdsThunderGroundSubmitFailCount++);
        }
    }
    if (thunder_fx_native_candidate != FALSE)
    {
        /* Same split-camera contract every fixed owner documents: fill the
         * identity on a COPY, because later code reads the shared config. */
        NDSRendererConfig fx_config = config;
        NDSRendererMatrix20p12 fx_identity;

        if ((fx_config.initial_projection == NULL) &&
            (fx_config.initial_modelview != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&fx_identity);
            fx_config.initial_projection = &fx_identity;
        }
        else if ((fx_config.initial_modelview == NULL) &&
                 (fx_config.initial_projection != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&fx_identity);
            fx_config.initial_modelview = &fx_identity;
        }
        thunder_fx_native_handled =
            ndsRendererSubmitNativePikachuThunderJoltEffect(
                thunder_fx_base, thunder_fx_bytes, &thunder_fx_material,
                &fx_config, render_stats);
        if (thunder_fx_native_handled != FALSE)
        {
            NDS_DIAG(gNdsThunderJoltFxDrawCount++);
        }
        else
        {
            NDS_DIAG(gNdsThunderJoltFxSubmitFailCount++);
        }
    }
    if (damage_fly_mdust_native_seen != FALSE)
    {
        if (ndsRendererAdapterSubmitDamageFlyMDust(
                dobj, loaded, &config, render_stats) == FALSE)
        {
            ndsStageRejectNativeRender(dobj, dl,
                NDS_NATIVE_FAILURE_REJECTED_PROGRAM, render_stats);
        }
        else
        {
            ndsStageDLRouteRecord(dl, loaded, NDS_NATIVE_DAMAGE_FLY_MDUST_ROOT,
                                  NDS_SDL_ROUTE_DAMAGE_FLY_MDUST);
        }
        damage_fly_mdust_native_settled = TRUE;
    }
    if (damage_slash_native_seen != FALSE)
    {
        if (damage_slash_native_candidate != FALSE)
        {
            NDSRendererConfig slash_config = config;
            NDSRendererMatrix20p12 slash_identity;

            if ((slash_config.initial_projection == NULL) &&
                (slash_config.initial_modelview != NULL))
            {
                ndsRendererAdapterMtxIdentity20p12(&slash_identity);
                slash_config.initial_projection = &slash_identity;
            }
            else if ((slash_config.initial_modelview == NULL) &&
                     (slash_config.initial_projection != NULL))
            {
                ndsRendererAdapterMtxIdentity20p12(&slash_identity);
                slash_config.initial_modelview = &slash_identity;
            }
            damage_slash_native_handled = ndsRendererSubmitNativeDamageSlash(
                damage_slash_base, damage_slash_bytes, damage_slash_root,
                &damage_slash_material, &slash_config, render_stats);
        }
        if (damage_slash_native_handled != FALSE)
        {
            NDS_DIAG(gNdsDamageSlashDrawCount++);
        }
        else
        {
            NDS_DIAG(gNdsDamageSlashSubmitFailCount++);
            ndsStageRejectNativeRender(dobj, dl,
                NDS_NATIVE_FAILURE_REJECTED_PROGRAM, render_stats);
        }
        /* Whether it drew or published its precise rejection, this source root
         * has been resolved by its owner.  Suppress the generic NO_PROGRAM
         * guard below so one event cannot record two contradictory reasons. */
        damage_slash_native_settled = TRUE;
    }
#if NDS_STAGE_CASTLE
    if (castle_bumper_native_candidate != FALSE)
    {
        /* Same split-camera contract the other fixed owners document: fill
         * the identity on a COPY, because the owners below still read the
         * shared config. */
        NDSRendererConfig castle_bumper_config = config;
        NDSRendererMatrix20p12 castle_bumper_identity;

        if ((castle_bumper_config.initial_projection == NULL) &&
            (castle_bumper_config.initial_modelview != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&castle_bumper_identity);
            castle_bumper_config.initial_projection = &castle_bumper_identity;
        }
        else if ((castle_bumper_config.initial_modelview == NULL) &&
                 (castle_bumper_config.initial_projection != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&castle_bumper_identity);
            castle_bumper_config.initial_modelview = &castle_bumper_identity;
        }
        castle_bumper_native_handled = ndsRendererSubmitNativeCastleBumper(
            loaded->data, loaded->data_size,
            &castle_bumper_material, &castle_bumper_config, render_stats);
        if (castle_bumper_native_handled != FALSE)
        {
            NDS_DIAG(gNdsCastleBumperDrawCount++);
            if ((sNdsRendererAdapterEffectSubmitActive == FALSE) &&
                (sNdsRendererAdapterStagePersistentActive != FALSE))
            {
                ndsStageDLRouteRecord(dl, loaded, 0u,
                                      NDS_SDL_ROUTE_CASTLE_BUMPER);
            }
        }
        else
        {
            NDS_DIAG(gNdsCastleBumperSubmitFailCount++);
        }
    }
#endif
#if NDS_STAGE_SECTOR
    if (sector_laser_native_candidate != FALSE)
    {
        /* Same split-camera contract the Pakkun owner documents above: the
         * battle camera can supply the whole transform on one side of the DS
         * pair, and a fixed owner has no matrix stream to fill the other
         * implicitly.  Fill the identity on a COPY -- the impact-wave submit
         * below and the effect witnesses at the end of this function still
         * read the shared config, so mutating it here would corrupt both. */
        NDSRendererConfig laser_config = config;
        NDSRendererMatrix20p12 laser_identity;

        if ((laser_config.initial_projection == NULL) &&
            (laser_config.initial_modelview != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&laser_identity);
            laser_config.initial_projection = &laser_identity;
        }
        else if ((laser_config.initial_modelview == NULL) &&
                 (laser_config.initial_projection != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&laser_identity);
            laser_config.initial_modelview = &laser_identity;
        }
        sector_laser_native_handled =
            ndsRendererSubmitNativeSectorArwingLaser(
                sector_laser_tlut, sector_laser_image, &laser_config,
                render_stats);
        if (sector_laser_native_handled != FALSE)
        {
            NDS_DIAG(gNdsSectorLaserDrawCount++);
            ndsStageDLRouteRecord(dl, loaded, NDS_NATIVE_SECTOR_LASER_ROOT,
                                  NDS_SDL_ROUTE_SECTOR_LASER);
        }
        else
        {
            /* No fallback: a refusal falls through to the loud NO_PROGRAM
             * record below, never to a generic route. */
            NDS_DIAG(gNdsSectorLaserSubmitFailCount++);
        }
    }
#endif
#if NDS_LINK
    if (link_bomb_native_candidate != FALSE)
    {
        /* Same split-camera contract the other fixed owners document: the
         * battle camera can supply the whole transform on one side of the DS
         * pair, and a fixed owner has no matrix stream to fill the other
         * implicitly.  Fill the identity on a COPY -- the impact-wave submit
         * below and the effect witnesses at the end of this function still
         * read the shared config, so mutating it here would corrupt both. */
        NDSRendererConfig link_bomb_config = config;
        NDSRendererMatrix20p12 link_bomb_identity;

        if ((link_bomb_config.initial_projection == NULL) &&
            (link_bomb_config.initial_modelview != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&link_bomb_identity);
            link_bomb_config.initial_projection = &link_bomb_identity;
        }
        else if ((link_bomb_config.initial_modelview == NULL) &&
                 (link_bomb_config.initial_projection != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&link_bomb_identity);
            link_bomb_config.initial_modelview = &link_bomb_identity;
        }
        /* env_color is ALREADY the item layer's live ColAnim colour by this
         * point (seeded above from sNdsRendererAdapterItemEnvColor[head]) and
         * the body combiner's cycle 1 consumes it.  Do not reseed it here. */
        link_bomb_native_handled = ndsRendererSubmitNativeLinkBomb(
            link_bomb_root, loaded->data, loaded->data_size,
            &link_bomb_config, render_stats);
        if (link_bomb_native_handled != FALSE)
        {
            NDS_DIAG(gNdsLinkBombDrawCount++);
        }
        else
        {
            /* No fallback: a refusal falls through to the loud NO_PROGRAM
             * record below, never to a generic route. */
            NDS_DIAG(gNdsLinkBombSubmitFailCount++);
        }
    }
#endif
#if NDS_STAGE_YAMABUKI
    if (marumine_native_candidate != FALSE)
    {
        /* Same split-camera contract the other fixed owners document: fill the
         * identity on a COPY, because later code in this function still reads
         * the shared config. */
        NDSRendererConfig marumine_config = config;
        NDSRendererMatrix20p12 marumine_identity;

        if ((marumine_config.initial_projection == NULL) &&
            (marumine_config.initial_modelview != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&marumine_identity);
            marumine_config.initial_projection = &marumine_identity;
        }
        else if ((marumine_config.initial_modelview == NULL) &&
                 (marumine_config.initial_projection != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&marumine_identity);
            marumine_config.initial_modelview = &marumine_identity;
        }
        /* The combiner reads TEXEL0 and SHADE only, so the item layer's seeded
         * prim/env in render_stats cannot reach this draw.  Do not reseed. */
        marumine_native_handled = ndsRendererSubmitNativeYamabukiMarumine(
            loaded->data, loaded->data_size, &marumine_config, render_stats);
        if (marumine_native_handled != FALSE)
        {
            NDS_DIAG(gNdsYamabukiMarumineDrawCount++);
            ndsStageDLRouteRecord(dl, loaded, 0u,
                                  NDS_SDL_ROUTE_ITEM + nNDSStageDLItemMarumine);
        }
        else
        {
            /* No fallback: a refusal falls through to the loud NO_PROGRAM
             * record below, never to a generic route. */
            NDS_DIAG(gNdsYamabukiMarumineSubmitFailCount++);
        }
    }
#endif
#if NDS_STAGE_YAMABUKI
    if (glucky_native_candidate != FALSE)
    {
        NDSRendererConfig glucky_config = config;
        NDSRendererMatrix20p12 glucky_identity;

        if ((glucky_config.initial_projection == NULL) &&
            (glucky_config.initial_modelview != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&glucky_identity);
            glucky_config.initial_projection = &glucky_identity;
        }
        else if ((glucky_config.initial_modelview == NULL) &&
                 (glucky_config.initial_projection != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&glucky_identity);
            glucky_config.initial_modelview = &glucky_identity;
        }
        glucky_native_handled = ndsRendererSubmitNativeItemGLucky(
            loaded->data, loaded->data_size, &glucky_config, render_stats);
        if (glucky_native_handled != FALSE)
        {
            NDS_DIAG(gNdsYamabukiGluckyDrawCount++);
            ndsStageDLRouteRecord(dl, loaded, 0u,
                                  NDS_SDL_ROUTE_ITEM + nNDSStageDLItemGLucky);
        }
        else
        {
            NDS_DIAG(gNdsYamabukiGluckySubmitFailCount++);
        }
    }

    if (porygon_native_candidate != FALSE)
    {
        NDSRendererConfig porygon_config = config;
        NDSRendererMatrix20p12 porygon_identity;

        if ((porygon_config.initial_projection == NULL) &&
            (porygon_config.initial_modelview != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&porygon_identity);
            porygon_config.initial_projection = &porygon_identity;
        }
        else if ((porygon_config.initial_modelview == NULL) &&
                 (porygon_config.initial_projection != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&porygon_identity);
            porygon_config.initial_modelview = &porygon_identity;
        }
        porygon_native_handled = ndsRendererSubmitNativeItemPorygon(
            loaded->data, loaded->data_size, &porygon_config, render_stats);
        if (porygon_native_handled != FALSE)
        {
            NDS_DIAG(gNdsYamabukiPorygonDrawCount++);
            ndsStageDLRouteRecord(dl, loaded, 0u,
                                  NDS_SDL_ROUTE_ITEM + nNDSStageDLItemPorygon);
        }
        else
        {
            NDS_DIAG(gNdsYamabukiPorygonSubmitFailCount++);
        }
    }

    if (hitokage_native_candidate != FALSE)
    {
        NDSRendererConfig hitokage_config = config;
        NDSRendererMatrix20p12 hitokage_identity;

        if ((hitokage_config.initial_projection == NULL) &&
            (hitokage_config.initial_modelview != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&hitokage_identity);
            hitokage_config.initial_projection = &hitokage_identity;
        }
        else if ((hitokage_config.initial_modelview == NULL) &&
                 (hitokage_config.initial_projection != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&hitokage_identity);
            hitokage_config.initial_modelview = &hitokage_identity;
        }
        hitokage_native_handled = ndsRendererSubmitNativeItemHitokage(
            loaded->data, loaded->data_size, &hitokage_material,
            &hitokage_config, render_stats);
        if (hitokage_native_handled != FALSE)
        {
            NDS_DIAG(gNdsYamabukiHitokageDrawCount++);
            ndsStageDLRouteRecord(dl, loaded, NDS_NATIVE_ITEM_HITOKAGE_ROOT,
                                  NDS_SDL_ROUTE_HITOKAGE);
        }
        else
        {
            NDS_DIAG(gNdsYamabukiHitokageSubmitFailCount++);
        }
    }

    if (fushigibana_native_candidate != FALSE)
    {
        NDSRendererConfig fushigibana_config = config;
        NDSRendererMatrix20p12 fushigibana_identity;

        if ((fushigibana_config.initial_projection == NULL) &&
            (fushigibana_config.initial_modelview != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&fushigibana_identity);
            fushigibana_config.initial_projection = &fushigibana_identity;
        }
        else if ((fushigibana_config.initial_modelview == NULL) &&
                 (fushigibana_config.initial_projection != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&fushigibana_identity);
            fushigibana_config.initial_modelview = &fushigibana_identity;
        }
        fushigibana_native_handled = ndsRendererSubmitNativeItemFushigibana(
            loaded->data, loaded->data_size, &fushigibana_material,
            &fushigibana_config, render_stats);
        if (fushigibana_native_handled != FALSE)
        {
            NDS_DIAG(gNdsYamabukiFushigibanaDrawCount++);
            ndsStageDLRouteRecord(dl, loaded, NDS_NATIVE_ITEM_FUSHIGIBANA_ROOT,
                                  NDS_SDL_ROUTE_FUSHIGIBANA);
        }
        else
        {
            NDS_DIAG(gNdsYamabukiFushigibanaSubmitFailCount++);
        }
    }
#endif
    if (item_star_native_candidate != FALSE)
    {
        NDSRendererConfig item_config = config;
        NDSRendererMatrix20p12 identity;

        if ((item_config.initial_projection == NULL) &&
            (item_config.initial_modelview != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&identity);
            item_config.initial_projection = &identity;
        }
        else if ((item_config.initial_modelview == NULL) &&
                 (item_config.initial_projection != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&identity);
            item_config.initial_modelview = &identity;
        }
        item_star_native_handled = ndsRendererSubmitNativeItemStar(
            loaded->data, loaded->data_size,
            &item_star_material0, &item_star_material1,
            &item_config, render_stats);
        if (item_star_native_handled != FALSE)
        {
            NDS_DIAG(gNdsItemStarDrawCount++);
            if ((sNdsRendererAdapterEffectSubmitActive == FALSE) &&
                (sNdsRendererAdapterStagePersistentActive != FALSE))
            {
                ndsStageDLRouteRecord(dl, loaded, 0u, NDS_SDL_ROUTE_STAR);
            }
        }
        else
        {
            NDS_DIAG(gNdsItemStarSubmitFailCount++);
        }
    }

    if (item_sword_native_candidate != FALSE)
    {
        NDSRendererConfig item_config = config;
        NDSRendererMatrix20p12 identity;

        if ((item_config.initial_projection == NULL) &&
            (item_config.initial_modelview != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&identity);
            item_config.initial_projection = &identity;
        }
        else if ((item_config.initial_modelview == NULL) &&
                 (item_config.initial_projection != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&identity);
            item_config.initial_modelview = &identity;
        }
        item_sword_native_handled = ndsRendererSubmitNativeItemSword(
            item_sword_root, loaded->data, loaded->data_size,
            &item_config, render_stats);
        if (item_sword_native_handled != FALSE)
        {
            NDS_DIAG(gNdsItemSwordDrawCount++);
            ndsStageDLRouteRecord(dl, loaded, item_sword_root,
                                  NDS_SDL_ROUTE_ITEM + nNDSStageDLItemSword);
        }
        else
        {
            NDS_DIAG(gNdsItemSwordSubmitFailCount++);
        }
    }

    if (item_hammer_native_candidate != FALSE)
    {
        NDSRendererConfig item_config = config;
        NDSRendererMatrix20p12 identity;

        if ((item_config.initial_projection == NULL) &&
            (item_config.initial_modelview != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&identity);
            item_config.initial_projection = &identity;
        }
        else if ((item_config.initial_modelview == NULL) &&
                 (item_config.initial_projection != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&identity);
            item_config.initial_modelview = &identity;
        }
        item_hammer_native_handled = ndsRendererSubmitNativeItemHammer(
            loaded->data, loaded->data_size, &item_config, render_stats);
        if (item_hammer_native_handled != FALSE)
        {
            NDS_DIAG(gNdsItemHammerDrawCount++);
            ndsStageDLRouteRecord(dl, loaded, 0u,
                                  NDS_SDL_ROUTE_ITEM + nNDSStageDLItemHammer);
        }
        else
        {
            NDS_DIAG(gNdsItemHammerSubmitFailCount++);
        }
    }

    if (item_mball_native_candidate != FALSE)
    {
        NDSRendererConfig item_config = config;
        NDSRendererMatrix20p12 identity;
        const NDSRendererNativeMaterial *material =
            (item_mball_root == NDS_NATIVE_ITEM_MBALL_LIVE_ROOT) ?
                &item_mball_material : NULL;

        if ((item_config.initial_projection == NULL) &&
            (item_config.initial_modelview != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&identity);
            item_config.initial_projection = &identity;
        }
        else if ((item_config.initial_modelview == NULL) &&
                 (item_config.initial_projection != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&identity);
            item_config.initial_modelview = &identity;
        }
        item_mball_native_handled = ndsRendererSubmitNativeItemMBall(
            item_mball_root, loaded->data, loaded->data_size,
            material, &item_config, render_stats);
        if (item_mball_native_handled != FALSE)
        {
            NDS_DIAG(gNdsItemMBallDrawCount++);
            if (item_mball_from_effect != FALSE)
            {
                NDS_DIAG(gNdsEntryMBallThrownDrawCount++);
            }
            else if ((sNdsRendererAdapterItemSubmitActive != FALSE) &&
                     (sNdsRendererAdapterEffectSubmitActive == FALSE) &&
                     (sNdsRendererAdapterStagePersistentActive != FALSE))
            {
                ndsStageDLRouteRecord(dl, loaded, item_mball_root,
                                      NDS_SDL_ROUTE_MBALL);
            }
        }
        else
        {
            NDS_DIAG(gNdsItemMBallSubmitFailCount++);
            if (item_mball_from_effect != FALSE)
            {
                NDS_DIAG(gNdsEntryMBallThrownSubmitFailCount++);
            }
        }
    }

    if (item_kirbystar_native_candidate != FALSE)
    {
        NDSRendererConfig item_config = config;
        NDSRendererMatrix20p12 identity;

        if ((item_config.initial_projection == NULL) &&
            (item_config.initial_modelview != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&identity);
            item_config.initial_projection = &identity;
        }
        else if ((item_config.initial_modelview == NULL) &&
                 (item_config.initial_projection != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&identity);
            item_config.initial_modelview = &identity;
        }
        item_kirbystar_native_handled = ndsRendererSubmitNativeItemKirbyStar(
            loaded->data, loaded->data_size, NULL, &item_config,
            render_stats);
        if (item_kirbystar_native_handled != FALSE)
        {
            NDS_DIAG(gNdsItemKirbyStarDrawCount++);
            ndsStageDLRouteRecord(dl, loaded, NDS_NATIVE_ITEM_KIRBYSTAR_ROOT,
                                  NDS_SDL_ROUTE_KIRBYSTAR);
        }
        else
        {
            NDS_DIAG(gNdsItemKirbyStarSubmitFailCount++);
        }
    }

    if (item_gshell_native_candidate != FALSE)
    {
        NDSRendererConfig item_config = config;
        NDSRendererMatrix20p12 identity;

        if ((item_config.initial_projection == NULL) &&
            (item_config.initial_modelview != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&identity);
            item_config.initial_projection = &identity;
        }
        else if ((item_config.initial_modelview == NULL) &&
                 (item_config.initial_projection != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&identity);
            item_config.initial_modelview = &identity;
        }
        item_gshell_native_handled = ndsRendererSubmitNativeItemGShell(
            loaded->data, loaded->data_size, &item_gshell_material,
            &item_config, render_stats);
        if (item_gshell_native_handled != FALSE)
        {
            NDS_DIAG(gNdsItemGShellDrawCount++);
            if ((sNdsRendererAdapterEffectSubmitActive == FALSE) &&
                (sNdsRendererAdapterStagePersistentActive != FALSE))
            {
                ndsStageDLRouteRecord(dl, loaded, NDS_NATIVE_ITEM_GSHELL_ROOT,
                                      NDS_SDL_ROUTE_SHELL);
            }
        }
        else
        {
            NDS_DIAG(gNdsItemGShellSubmitFailCount++);
        }
    }

    if (item_rshell_native_candidate != FALSE)
    {
        NDSRendererConfig item_config = config;
        NDSRendererMatrix20p12 identity;

        if ((item_config.initial_projection == NULL) &&
            (item_config.initial_modelview != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&identity);
            item_config.initial_projection = &identity;
        }
        else if ((item_config.initial_modelview == NULL) &&
                 (item_config.initial_projection != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&identity);
            item_config.initial_modelview = &identity;
        }
        item_rshell_native_handled = ndsRendererSubmitNativeItemRShell(
            loaded->data, loaded->data_size, &item_rshell_material,
            &item_config, render_stats);
        if (item_rshell_native_handled != FALSE)
        {
            NDS_DIAG(gNdsItemRShellDrawCount++);
            if ((sNdsRendererAdapterEffectSubmitActive == FALSE) &&
                (sNdsRendererAdapterStagePersistentActive != FALSE))
            {
                ndsStageDLRouteRecord(dl, loaded, NDS_NATIVE_ITEM_RSHELL_ROOT,
                                      NDS_SDL_ROUTE_SHELL);
            }
        }
        else
        {
            NDS_DIAG(gNdsItemRShellSubmitFailCount++);
        }
    }

    if (item_bat_native_candidate != FALSE)
    {
        NDSRendererConfig item_config = config;
        NDSRendererMatrix20p12 identity;

        if ((item_config.initial_projection == NULL) &&
            (item_config.initial_modelview != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&identity);
            item_config.initial_projection = &identity;
        }
        else if ((item_config.initial_modelview == NULL) &&
                 (item_config.initial_projection != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&identity);
            item_config.initial_modelview = &identity;
        }
        item_bat_native_handled = ndsRendererSubmitNativeItemBat(
            item_bat_root, loaded->data, loaded->data_size,
            &item_config, render_stats);
        if (item_bat_native_handled != FALSE)
        {
            NDS_DIAG(gNdsItemBatDrawCount++);
            ndsStageDLRouteRecord(dl, loaded, item_bat_root,
                                  NDS_SDL_ROUTE_ITEM + nNDSStageDLItemBat);
        }
        else
        {
            NDS_DIAG(gNdsItemBatSubmitFailCount++);
        }
    }

    if (item_capsule_native_candidate != FALSE)
    {
        NDSRendererConfig item_config = config;
        NDSRendererMatrix20p12 identity;

        if ((item_config.initial_projection == NULL) &&
            (item_config.initial_modelview != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&identity);
            item_config.initial_projection = &identity;
        }
        else if ((item_config.initial_modelview == NULL) &&
                 (item_config.initial_projection != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&identity);
            item_config.initial_modelview = &identity;
        }
        item_capsule_native_handled = ndsRendererSubmitNativeItemCapsule(
            item_capsule_root, loaded->data, loaded->data_size,
            &item_config, render_stats);
        if (item_capsule_native_handled != FALSE)
        {
            NDS_DIAG(gNdsItemCapsuleDrawCount++);
            ndsStageDLRouteRecord(dl, loaded, item_capsule_root,
                                  NDS_SDL_ROUTE_ITEM + nNDSStageDLItemCapsule);
        }
        else
        {
            NDS_DIAG(gNdsItemCapsuleSubmitFailCount++);
        }
    }

    if (item_bombhei_native_candidate != FALSE)
    {
        NDSRendererConfig item_config = config;
        NDSRendererMatrix20p12 identity;

        if ((item_config.initial_projection == NULL) &&
            (item_config.initial_modelview != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&identity);
            item_config.initial_projection = &identity;
        }
        else if ((item_config.initial_modelview == NULL) &&
                 (item_config.initial_projection != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&identity);
            item_config.initial_modelview = &identity;
        }
        item_bombhei_native_handled = ndsRendererSubmitNativeItemBombHei(
            loaded->data, loaded->data_size, &item_bombhei_material,
            &item_config, render_stats);
        if (item_bombhei_native_handled != FALSE)
        {
            NDS_DIAG(gNdsItemBombHeiDrawCount++);
            if ((sNdsRendererAdapterEffectSubmitActive == FALSE) &&
                (sNdsRendererAdapterStagePersistentActive != FALSE))
            {
                ndsStageDLRouteRecord(dl, loaded, NDS_NATIVE_ITEM_BOMBHEI_ROOT,
                                      NDS_SDL_ROUTE_BOMBHEI);
            }
        }
        else
        {
            NDS_DIAG(gNdsItemBombHeiSubmitFailCount++);
        }
    }

    if (item_lgun_native_candidate != FALSE)
    {
        NDSRendererConfig item_config = config;
        NDSRendererMatrix20p12 identity;

        if ((item_config.initial_projection == NULL) &&
            (item_config.initial_modelview != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&identity);
            item_config.initial_projection = &identity;
        }
        else if ((item_config.initial_modelview == NULL) &&
                 (item_config.initial_projection != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&identity);
            item_config.initial_modelview = &identity;
        }
        item_lgun_native_handled = ndsRendererSubmitNativeItemLGun(
            loaded->data, loaded->data_size, &item_config, render_stats);
        if (item_lgun_native_handled != FALSE)
        {
            NDS_DIAG(gNdsItemLGunDrawCount++);
            ndsStageDLRouteRecord(dl, loaded, 0u,
                                  NDS_SDL_ROUTE_ITEM + nNDSStageDLItemLGun);
        }
        else
        {
            NDS_DIAG(gNdsItemLGunSubmitFailCount++);
        }
    }

    if (item_harisen_native_candidate != FALSE)
    {
        NDSRendererConfig item_config = config;
        NDSRendererMatrix20p12 identity;

        if ((item_config.initial_projection == NULL) &&
            (item_config.initial_modelview != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&identity);
            item_config.initial_projection = &identity;
        }
        else if ((item_config.initial_modelview == NULL) &&
                 (item_config.initial_projection != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&identity);
            item_config.initial_modelview = &identity;
        }
        item_harisen_native_handled = ndsRendererSubmitNativeItemHarisen(
            loaded->data, loaded->data_size, &item_config, render_stats);
        if (item_harisen_native_handled != FALSE)
        {
            NDS_DIAG(gNdsItemHarisenDrawCount++);
            ndsStageDLRouteRecord(dl, loaded, 0u,
                                  NDS_SDL_ROUTE_ITEM + nNDSStageDLItemHarisen);
        }
        else
        {
            NDS_DIAG(gNdsItemHarisenSubmitFailCount++);
        }
    }

    if (item_heart_native_candidate != FALSE)
    {
        NDSRendererConfig item_config = config;
        NDSRendererMatrix20p12 identity;

        if ((item_config.initial_projection == NULL) &&
            (item_config.initial_modelview != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&identity);
            item_config.initial_projection = &identity;
        }
        else if ((item_config.initial_modelview == NULL) &&
                 (item_config.initial_projection != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&identity);
            item_config.initial_modelview = &identity;
        }
        item_heart_native_handled = ndsRendererSubmitNativeItemHeart(
            loaded->data, loaded->data_size, &item_config, render_stats);
        if (item_heart_native_handled != FALSE)
        {
            NDS_DIAG(gNdsItemHeartDrawCount++);
            ndsStageDLRouteRecord(dl, loaded, 0u,
                                  NDS_SDL_ROUTE_ITEM + nNDSStageDLItemHeart);
        }
        else
        {
            NDS_DIAG(gNdsItemHeartSubmitFailCount++);
        }
    }

    if (item_starrod_native_candidate != FALSE)
    {
        NDSRendererConfig item_config = config;
        NDSRendererMatrix20p12 identity;

        if ((item_config.initial_projection == NULL) &&
            (item_config.initial_modelview != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&identity);
            item_config.initial_projection = &identity;
        }
        else if ((item_config.initial_modelview == NULL) &&
                 (item_config.initial_projection != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&identity);
            item_config.initial_modelview = &identity;
        }
        item_starrod_native_handled = ndsRendererSubmitNativeItemStarRod(
            item_starrod_root, loaded->data, loaded->data_size,
            &item_config, render_stats);
        if (item_starrod_native_handled != FALSE)
        {
            NDS_DIAG(gNdsItemStarRodDrawCount++);
            ndsStageDLRouteRecord(dl, loaded, item_starrod_root,
                                  NDS_SDL_ROUTE_ITEM + nNDSStageDLItemStarRod);
        }
        else
        {
            NDS_DIAG(gNdsItemStarRodSubmitFailCount++);
        }
    }

    if (item_fflower_native_candidate != FALSE)
    {
        NDSRendererConfig item_config = config;
        NDSRendererMatrix20p12 identity;
        const NDSRendererNativeMaterial *material =
            (item_fflower_root == NDS_NATIVE_ITEM_FFLOWER_LIVE_ROOT) ?
                &item_fflower_material : NULL;

        if ((item_config.initial_projection == NULL) &&
            (item_config.initial_modelview != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&identity);
            item_config.initial_projection = &identity;
        }
        else if ((item_config.initial_modelview == NULL) &&
                 (item_config.initial_projection != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&identity);
            item_config.initial_modelview = &identity;
        }
        item_fflower_native_handled = ndsRendererSubmitNativeItemFFlower(
            item_fflower_root, loaded->data, loaded->data_size, material,
            &item_config, render_stats);
        if (item_fflower_native_handled != FALSE)
        {
            NDS_DIAG(gNdsItemFFlowerDrawCount++);
            ndsStageDLRouteRecord(dl, loaded, item_fflower_root,
                (item_fflower_root == NDS_NATIVE_ITEM_FFLOWER_LIVE_ROOT) ?
                    NDS_SDL_ROUTE_FFLOWER_LIVE :
                    (NDS_SDL_ROUTE_ITEM + nNDSStageDLItemFFlower));
        }
        else
        {
            NDS_DIAG(gNdsItemFFlowerSubmitFailCount++);
        }
    }

    if (item_msbomb_native_candidate != FALSE)
    {
        NDSRendererConfig item_config = config;
        NDSRendererMatrix20p12 identity;

        if ((item_config.initial_projection == NULL) &&
            (item_config.initial_modelview != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&identity);
            item_config.initial_projection = &identity;
        }
        else if ((item_config.initial_modelview == NULL) &&
                 (item_config.initial_projection != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&identity);
            item_config.initial_modelview = &identity;
        }
        item_msbomb_native_handled = ndsRendererSubmitNativeItemMSBomb(
            item_msbomb_root, loaded->data, loaded->data_size,
            &item_config, render_stats);
        if (item_msbomb_native_handled != FALSE)
        {
            NDS_DIAG(gNdsItemMSBombDrawCount++);
            ndsStageDLRouteRecord(dl, loaded, item_msbomb_root,
                                  NDS_SDL_ROUTE_ITEM + nNDSStageDLItemMSBomb);
        }
        else
        {
            NDS_DIAG(gNdsItemMSBombSubmitFailCount++);
        }
    }

    if (item_nbumper_native_candidate != FALSE)
    {
        NDSRendererConfig item_config = config;
        NDSRendererMatrix20p12 identity;

        if ((item_config.initial_projection == NULL) &&
            (item_config.initial_modelview != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&identity);
            item_config.initial_projection = &identity;
        }
        else if ((item_config.initial_modelview == NULL) &&
                 (item_config.initial_projection != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&identity);
            item_config.initial_modelview = &identity;
        }
        item_nbumper_native_handled = ndsRendererSubmitNativeItemNBumper(
            loaded->data, loaded->data_size, &item_nbumper_material,
            &item_config, render_stats);
        if (item_nbumper_native_handled != FALSE)
        {
            NDS_DIAG(gNdsItemNBumperDrawCount++);
            ndsStageDLRouteRecord(dl, loaded,
                                  NDS_NATIVE_ITEM_NBUMPER_ROOT,
                                  NDS_SDL_ROUTE_NBUMPER);
        }
        else
        {
            NDS_DIAG(gNdsItemNBumperSubmitFailCount++);
        }
    }

    if (item_box_native_candidate != FALSE)
    {
        NDSRendererConfig item_config = config;
        NDSRendererMatrix20p12 identity;

        if ((item_config.initial_projection == NULL) &&
            (item_config.initial_modelview != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&identity);
            item_config.initial_projection = &identity;
        }
        else if ((item_config.initial_modelview == NULL) &&
                 (item_config.initial_projection != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&identity);
            item_config.initial_modelview = &identity;
        }
        item_box_native_handled = ndsRendererSubmitNativeItemBox(
            loaded->data, loaded->data_size, &item_config, render_stats);
        if (item_box_native_handled != FALSE)
        {
            NDS_DIAG(gNdsItemBoxDrawCount++);
            ndsStageDLRouteRecord(dl, loaded, 0u,
                                  NDS_SDL_ROUTE_ITEM + nNDSStageDLItemBox);
        }
        else
        {
            NDS_DIAG(gNdsItemBoxSubmitFailCount++);
        }
    }

    if (item_taru_native_candidate != FALSE)
    {
        NDSRendererConfig item_config = config;
        NDSRendererMatrix20p12 identity;

        if ((item_config.initial_projection == NULL) &&
            (item_config.initial_modelview != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&identity);
            item_config.initial_projection = &identity;
        }
        else if ((item_config.initial_modelview == NULL) &&
                 (item_config.initial_projection != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&identity);
            item_config.initial_modelview = &identity;
        }
        item_taru_native_handled = ndsRendererSubmitNativeItemTaru(
            loaded->data, loaded->data_size, &item_config, render_stats);
        if (item_taru_native_handled != FALSE)
        {
            NDS_DIAG(gNdsItemTaruDrawCount++);
            ndsStageDLRouteRecord(dl, loaded, 0u,
                                  NDS_SDL_ROUTE_ITEM + nNDSStageDLItemTaru);
        }
        else
        {
            NDS_DIAG(gNdsItemTaruSubmitFailCount++);
        }
    }

    if (item_egg_native_candidate != FALSE)
    {
        NDSRendererConfig item_config = config;
        NDSRendererMatrix20p12 identity;

        if ((item_config.initial_projection == NULL) &&
            (item_config.initial_modelview != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&identity);
            item_config.initial_projection = &identity;
        }
        else if ((item_config.initial_modelview == NULL) &&
                 (item_config.initial_projection != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&identity);
            item_config.initial_modelview = &identity;
        }
        item_egg_native_handled = ndsRendererSubmitNativeItemEgg(
            loaded->data, loaded->data_size, &item_config, render_stats);
        if (item_egg_native_handled != FALSE)
        {
            NDS_DIAG(gNdsItemEggDrawCount++);
            ndsStageDLRouteRecord(dl, loaded, 0u,
                                  NDS_SDL_ROUTE_ITEM + nNDSStageDLItemEgg);
        }
        else
        {
            NDS_DIAG(gNdsItemEggSubmitFailCount++);
        }
    }

    if (item_iwark_native_candidate != FALSE)
    {
        NDSRendererConfig item_config = config;
        NDSRendererMatrix20p12 identity;

        if ((item_config.initial_projection == NULL) &&
            (item_config.initial_modelview != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&identity);
            item_config.initial_projection = &identity;
        }
        else if ((item_config.initial_modelview == NULL) &&
                 (item_config.initial_projection != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&identity);
            item_config.initial_modelview = &identity;
        }
        item_iwark_native_handled = ndsRendererSubmitNativeItemIwark(
            loaded->data, loaded->data_size, &item_config, render_stats);
        if (item_iwark_native_handled != FALSE)
        {
            NDS_DIAG(gNdsItemIwarkDrawCount++);
            ndsStageDLRouteRecord(dl, loaded, 0u,
                                  NDS_SDL_ROUTE_ITEM + nNDSStageDLItemIwark);
        }
        else
        {
            NDS_DIAG(gNdsItemIwarkSubmitFailCount++);
        }
    }

    /* 2026-09-30: the baked roots (generate_nds_native_item_baked.py) -- the
     * roots a full-match lab census found drawn with no owner: PK Fire's
     * pillar, Bob-omb walking left, the placed Bumper, the Ray Gun's shot,
     * Razor Leaf -- and the Poke Ball Pokemon and their weapons, which it
     * could not reach. One lookup names the root for its GObj kind; its live
     * segment-E materials are the DObj's MObjs in order. */
    if ((loaded != NULL) && (dobj != NULL) && (dobj->parent_gobj != NULL) &&
        (NDS_NATIVE_BAKED_ASSET_MATCH(loaded->asset_id)
#if NDS_1P_GAME
         /* The ending's room (the table it loads into its own heap). */
         || NDS_NATIVE_BAKED_ROOM_ASSET_MATCH(loaded->asset_id)
#endif
        ))
    {
        u32 baked_slots = 0u;
        const void *baked = ndsNativeBakedItemFind(
            loaded->asset_id, ndsRelocNativeRootOffset(loaded, dl),
            (u32)dobj->parent_gobj->id, dl, &baked_slots);

        if (baked != NULL)
        {
            NDSRendererNativeMaterial baked_materials[
                NDS_NATIVE_BAKED_MATERIAL_SLOTS];
            NDSRendererConfig item_config = config;
            NDSRendererMatrix20p12 identity;
            MObj *mobj = dobj->mobj;
            u32 i;

            for (i = 0u; i < baked_slots; i++)
            {
                if ((mobj == NULL) ||
                    (ndsRendererAdapterBuildNativeMaterialSnapshot(
                         mobj, &baked_materials[i], FALSE,
                         NULL, NULL) == FALSE))
                {
                    break;
                }
                mobj = mobj->next;
            }
            if ((item_config.initial_projection == NULL) &&
                (item_config.initial_modelview != NULL))
            {
                ndsRendererAdapterMtxIdentity20p12(&identity);
                item_config.initial_projection = &identity;
            }
            else if ((item_config.initial_modelview == NULL) &&
                     (item_config.initial_projection != NULL))
            {
                ndsRendererAdapterMtxIdentity20p12(&identity);
                item_config.initial_modelview = &identity;
            }
            ndsNativeBakedSetSeedTranslate(&dobj->translate.vec.f);
            item_baked_native_handled = (i == baked_slots) ?
                ndsRendererSubmitNativeBaked(
                    baked, loaded->data, loaded->data_size,
                    baked_materials, i, &item_config, render_stats) :
                FALSE;
            ndsNativeBakedSetSeedTranslate(NULL);
            if (item_baked_native_handled != FALSE)
            {
                NDS_DIAG(gNdsItemBakedDrawCount++);
                /* A ground display's baked root draws through the fast lane
                 * from now on (NDS_SDL_ROUTE_BAKED); its admission is the
                 * GObj kind and the same submit context, checked per draw. */
                if ((dobj->parent_gobj->id == nGCCommonKindGroundDisplay) &&
                    (sNdsRendererAdapterItemSubmitActive == FALSE) &&
                    (sNdsRendererAdapterEffectSubmitActive == FALSE))
                {
                    ndsStageDLRouteRecord(dl, loaded, (u32)(uintptr_t)baked,
                                          NDS_SDL_ROUTE_BAKED);
                    ndsStageDLRouteSlot(dl)->pad = (u8)baked_slots;
                }
                else if ((dobj->parent_gobj->id == nGCCommonKindItem) &&
                         (sNdsRendererAdapterItemSubmitActive != FALSE) &&
                         (sNdsRendererAdapterEffectSubmitActive == FALSE) &&
                         (sNdsRendererAdapterStagePersistentActive != FALSE))
                {
                    ndsStageDLRouteRecord(dl, loaded, (u32)(uintptr_t)baked,
                                          NDS_SDL_ROUTE_BAKED_ITEM);
                    ndsStageDLRouteSlot(dl)->pad = (u8)baked_slots;
                }
                else if ((dobj->parent_gobj->id == nGCCommonKindWeapon) &&
                         (sNdsRendererAdapterItemSubmitActive == FALSE) &&
                         (sNdsRendererAdapterEffectSubmitActive == FALSE) &&
                         (sNdsRendererAdapterStagePersistentActive != FALSE))
                {
                    ndsStageDLRouteRecord(dl, loaded, (u32)(uintptr_t)baked,
                                          NDS_SDL_ROUTE_BAKED_WEAPON);
                    ndsStageDLRouteSlot(dl)->pad = (u8)baked_slots;
                }
#if NDS_1P_GAME
                else if ((sNdsRendererAdapterItemSubmitActive == FALSE) &&
                         (sNdsRendererAdapterEffectSubmitActive == FALSE) &&
                         (sNdsRendererAdapterStagePersistentActive != FALSE) &&
                         (ndsNativeBakedRootIsRoom(baked) != FALSE))
                {
                    ndsStageDLRouteRecord(dl, loaded, (u32)(uintptr_t)baked,
                                          NDS_SDL_ROUTE_BAKED_ROOM);
                    ndsStageDLRouteSlot(dl)->pad = (u8)baked_slots;
                }
#endif
            }
            else
            {
                NDS_DIAG(gNdsItemBakedSubmitFailCount++);
            }
        }
    }

    if (item_tomato_native_candidate != FALSE)
    {
        /* Same split-camera contract every fixed owner documents: fill the
         * identity on a COPY, because later code still reads the shared
         * config. */
        NDSRendererConfig tomato_config = config;
        NDSRendererMatrix20p12 tomato_identity;

        if ((tomato_config.initial_projection == NULL) &&
            (tomato_config.initial_modelview != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&tomato_identity);
            tomato_config.initial_projection = &tomato_identity;
        }
        else if ((tomato_config.initial_modelview == NULL) &&
                 (tomato_config.initial_projection != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&tomato_identity);
            tomato_config.initial_modelview = &tomato_identity;
        }
        item_tomato_native_handled = ndsRendererSubmitNativeItemTomato(
            loaded->data, loaded->data_size, &tomato_config, render_stats);
        if (item_tomato_native_handled != FALSE)
        {
            NDS_DIAG(gNdsItemTomatoDrawCount++);
            ndsStageDLRouteRecord(dl, loaded, 0u,
                                  NDS_SDL_ROUTE_ITEM + nNDSStageDLItemTomato);
        }
        else
        {
            NDS_DIAG(gNdsItemTomatoSubmitFailCount++);
        }
    }
    if (visual_effect_native_candidate != FALSE)
    {
        /* Same split-camera contract the other fixed owners document: the
         * battle camera can supply the whole transform on one side of the DS
         * pair, and a fixed owner has no matrix stream to fill the other
         * implicitly. Fill the identity on a COPY. */
        NDSRendererConfig visual_config = config;
        NDSRendererMatrix20p12 visual_identity;

        if ((visual_config.initial_projection == NULL) &&
            (visual_config.initial_modelview != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&visual_identity);
            visual_config.initial_projection = &visual_identity;
        }
        else if ((visual_config.initial_modelview == NULL) &&
                 (visual_config.initial_projection != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&visual_identity);
            visual_config.initial_modelview = &visual_identity;
        }
        visual_effect_native_handled = ndsRendererSubmitNativeVisualEffect(
            visual_effect_template, &visual_config, render_stats);
        if (visual_effect_native_handled == FALSE)
        {
            /* LOUD, AND WITH THE RIGHT REASON. An owner exists and refused, so
             * this is REJECTED_PROGRAM, not the generic NO_PROGRAM the guards
             * below publish for "nothing claimed this root". Recording it here
             * is what lets `settled` suppress those guards without turning a
             * refusal into a successful empty draw. */
            ndsStageRejectNativeRender(dobj, dl,
                NDS_NATIVE_FAILURE_REJECTED_PROGRAM, render_stats);
        }
        visual_effect_native_settled = TRUE;
    }
    if (
#if NDS_STAGE_INISHIE
        (inishie_pakkun_native_handled == FALSE) &&
        (inishie_powblock_native_handled == FALSE) &&
#endif
        (rebirth_halo_native_handled == FALSE) &&
        (charge_shot_native_handled == FALSE) &&
        (thunder_jolt_native_handled == FALSE) &&
        (thunder_ground_native_handled == FALSE) &&
        (thunder_fx_native_handled == FALSE) &&
        (damage_slash_native_settled == FALSE) &&
        (damage_fly_mdust_native_settled == FALSE) &&
        (ness_pkfire_native_handled == FALSE) &&
        (ness_pkthunder_native_handled == FALSE) &&
        (yoshi_entryegg_native_handled == FALSE) &&
        (yoshi_egg_native_handled == FALSE) &&
        (yoshi_egglay_native_handled == FALSE) &&
        (purin_sing_native_handled == FALSE) &&
        (kirby_vulcan_native_handled == FALSE) &&
        (pikachu_thunder_native_handled == FALSE) &&
        (samus_bomb_native_handled == FALSE) &&
        (ness_pktail_native_handled == FALSE) &&
#if NDS_STAGE_CASTLE
        (castle_bumper_native_handled == FALSE) &&
#endif
#if NDS_STAGE_SECTOR
        (sector_laser_native_handled == FALSE) &&
#endif
#if NDS_LINK
        (link_bomb_native_handled == FALSE) &&
#endif
#if NDS_STAGE_YAMABUKI
        (marumine_native_handled == FALSE) &&
#endif
#if NDS_STAGE_YAMABUKI
        (glucky_native_handled == FALSE) &&
        (porygon_native_handled == FALSE) &&
        (hitokage_native_handled == FALSE) &&
        (fushigibana_native_handled == FALSE) &&
#endif
        (item_star_native_handled == FALSE) &&
        (item_sword_native_handled == FALSE) &&
        (item_hammer_native_handled == FALSE) &&
        (item_mball_native_handled == FALSE) &&
        (item_gshell_native_handled == FALSE) &&
        (item_rshell_native_handled == FALSE) &&
        (item_bat_native_handled == FALSE) &&
        (item_capsule_native_handled == FALSE) &&
        (item_bombhei_native_handled == FALSE) &&
        (item_lgun_native_handled == FALSE) &&
        (item_harisen_native_handled == FALSE) &&
        (item_heart_native_handled == FALSE) &&
        (item_starrod_native_handled == FALSE) &&
        (item_fflower_native_handled == FALSE) &&
        (item_msbomb_native_handled == FALSE) &&
        (item_nbumper_native_handled == FALSE) &&
        (item_box_native_handled == FALSE) &&
        (item_taru_native_handled == FALSE) &&
        (item_egg_native_handled == FALSE) &&
        (item_iwark_native_handled == FALSE) &&
        (item_tomato_native_handled == FALSE) &&
        (item_kirbystar_native_handled == FALSE) &&
        (item_baked_native_handled == FALSE) &&
        (visual_effect_native_settled == FALSE) &&
        (impact_wave_native_candidate != FALSE))
    {
        impact_wave_native_handled = ndsRendererSubmitNativeImpactWave(
            sNdsImpactWaveVertices,
            (u32)(sizeof(sNdsImpactWaveVertices) /
                  sizeof(sNdsImpactWaveVertices[0])),
            sNdsImpactWaveTriangles,
            (u32)(sizeof(sNdsImpactWaveTriangles) / 3u),
            dl,
            &impact_wave_material,
            sNdsRendererAdapterImpactWaveVariant,
            &config,
            render_stats);
    }
    if (
        (rebirth_halo_native_handled == FALSE) &&
#if NDS_STAGE_INISHIE
        /* The Pakkun owner is checked here as well as in the impact-wave
         * OFF arm below. Without this term the item drew natively 600
         * times in a 300-present run and the stage still recorded 600
         * NO_PROGRAM failures at its own root, because the default build
         * has NDS_IMPACT_WAVE_NATIVE = 1 and takes this branch. */
        (inishie_pakkun_native_handled == FALSE) &&
        /* The POW block owner is checked here for the identical reason. */
        (inishie_powblock_native_handled == FALSE) &&
#endif
#if NDS_STAGE_SECTOR
        /* The laser owner is checked here AND in the OFF arm below, for the
         * identical reason the Pakkun comment above records. */
        (sector_laser_native_handled == FALSE) &&
#endif
        (charge_shot_native_handled == FALSE) &&
        (thunder_jolt_native_handled == FALSE) &&
        (thunder_ground_native_handled == FALSE) &&
        (thunder_fx_native_handled == FALSE) &&
        (damage_slash_native_settled == FALSE) &&
        (damage_fly_mdust_native_settled == FALSE) &&
        (ness_pkfire_native_handled == FALSE) &&
        (ness_pkthunder_native_handled == FALSE) &&
        (yoshi_entryegg_native_handled == FALSE) &&
        (yoshi_egg_native_handled == FALSE) &&
        (yoshi_egglay_native_handled == FALSE) &&
        (purin_sing_native_handled == FALSE) &&
        (kirby_vulcan_native_handled == FALSE) &&
        (pikachu_thunder_native_handled == FALSE) &&
        (samus_bomb_native_handled == FALSE) &&
        (ness_pktail_native_handled == FALSE) &&
#if NDS_STAGE_CASTLE
        (castle_bumper_native_handled == FALSE) &&
#endif
#if NDS_LINK
        /* The bomb owner is checked here AND in the OFF arm below, for the
         * identical reason the Pakkun comment above records. */
        (link_bomb_native_handled == FALSE) &&
#endif
#if NDS_STAGE_YAMABUKI
        /* The Marumine owner is checked here AND in the OFF arm below, for the
         * identical reason the Pakkun comment above records. */
        (marumine_native_handled == FALSE) &&
#endif
#if NDS_STAGE_YAMABUKI
        (glucky_native_handled == FALSE) &&
        (porygon_native_handled == FALSE) &&
        (hitokage_native_handled == FALSE) &&
        (fushigibana_native_handled == FALSE) &&
#endif
        (item_star_native_handled == FALSE) &&
        (item_sword_native_handled == FALSE) &&
        (item_hammer_native_handled == FALSE) &&
        (item_mball_native_handled == FALSE) &&
        (item_gshell_native_handled == FALSE) &&
        (item_rshell_native_handled == FALSE) &&
        (item_bat_native_handled == FALSE) &&
        (item_capsule_native_handled == FALSE) &&
        (item_bombhei_native_handled == FALSE) &&
        (item_lgun_native_handled == FALSE) &&
        (item_harisen_native_handled == FALSE) &&
        (item_heart_native_handled == FALSE) &&
        (item_starrod_native_handled == FALSE) &&
        (item_fflower_native_handled == FALSE) &&
        (item_msbomb_native_handled == FALSE) &&
        (item_nbumper_native_handled == FALSE) &&
        (item_box_native_handled == FALSE) &&
        (item_taru_native_handled == FALSE) &&
        (item_egg_native_handled == FALSE) &&
        (item_iwark_native_handled == FALSE) &&
        (item_tomato_native_handled == FALSE) &&
        (item_kirbystar_native_handled == FALSE) &&
        (item_baked_native_handled == FALSE) &&
        /* Unconditional: this owner has no build flag, so it must be excluded
         * from BOTH the impact-wave ON arm here and the OFF arm below. */
        (visual_effect_native_settled == FALSE) &&
        (impact_wave_native_handled == FALSE))
    {
        if (impact_wave_native_candidate != FALSE)
        {
            /* Keep the existing rejection witness; this cannot select a
             * different renderer or discard the failure as an empty draw. */
            NDS_DIAG(gNdsImpactWaveNativeFallbackCount++);
        }
        ndsStageRejectNativeRender(dobj, dl,
            NDS_NATIVE_FAILURE_NO_PROGRAM, render_stats);
    }
    else
    {
        NDS_DIAG(gNdsImpactWaveNativeDrawCount++);
    }
