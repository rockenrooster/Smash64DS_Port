/* The general stage DL submit's owner claims: each native owner recognises
 * its list (asset, root, parent kind, material contract) and arms its
 * candidate, with the scratch it needs, before the generic material-segment
 * setup. A claim draws nothing; renderer_adapter_stage_submits.inc does.
 *
 * Included in place by ndsRendererAdapterSubmitStageDLBody
 * (renderer_adapter_stage_submit.c): its text reads and writes that
 * function's locals, so it is not a file of its own. */

#if NDS_STAGE_INISHIE
    /* File 155 root 0x0B40 is not a scale-platform root.  BattleShip's
     * GRInishieMap points the scale map_nodes at 0x05F0; 0x0B40 is the
     * Pakkun ITAttributes DObjDesc child.  Item GObjs are outside the
     * whole-stage capture, so claim this exact item/root here before the
     * generic material-segment preparation can manufacture segment-E Gfx. */
    /* Step-witnessed, because the stage still records NO_PROGRAM at this root
     * and a single nine-clause test cannot say WHICH clause declined. The
     * highest step reached is the one that matters: 9 means every clause
     * passed and the decline is inside the submit itself. */
    if ((loaded != NULL) && (loaded->asset_id == 155u) &&
        (ndsRelocNativeRootOffset(loaded, dl) == 0x0b40u))
    {
        u32 pakkun_step = 1u;

        if (sNdsRendererAdapterItemSubmitActive != FALSE)
        {
            pakkun_step = 2u;
            if ((dobj->parent_gobj != NULL) &&
                (dobj->parent_gobj->id == nGCCommonKindItem))
            {
                pakkun_step = 3u;
                if ((dobj->mobj != NULL) && (dobj->mobj->next == NULL))
                {
                    pakkun_step = 4u;
                    gNdsInishiePakkunMaterialFlags =
                        ndsRendererAdapterMaterialFlags(dobj->mobj);
                    if (gNdsInishiePakkunMaterialFlags == 0x0001u)
                    {
                        pakkun_step = 5u;
                        if (ndsRendererAdapterBuildNativeMaterialSnapshot(
                                dobj->mobj, &inishie_pakkun_material, FALSE,
                                NULL, NULL) != FALSE)
                        {
                            pakkun_step = 6u;
                            gNdsInishiePakkunEffects =
                                inishie_pakkun_material.effects;
                            if (inishie_pakkun_material.effects ==
                                NDS_RENDERER_NATIVE_MATERIAL_CURRENT_IMAGE)
                            {
                                NDSRelocLoadedFile *image_file =
                                    ndsRelocFindLoadedFileContaining(
                                        (const void *)(uintptr_t)
                                            inishie_pakkun_material
                                                .current_image, 1u);

                                pakkun_step = 7u;
                                if (image_file == loaded)
                                {
                                    pakkun_step = 8u;
                                    inishie_pakkun_palette_file =
                                        ndsRelocFindLoadedFileByAsset(107u);
                                    if ((inishie_pakkun_palette_file != NULL) &&
                                        (inishie_pakkun_palette_file->data_size >=
                                         0x3620u + 32u))
                                    {
                                        pakkun_step = 9u;
                                        inishie_pakkun_native_candidate = TRUE;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        if (pakkun_step > gNdsInishiePakkunCandidateStep)
        {
            gNdsInishiePakkunCandidateStep = pakkun_step;
        }
    }
    /* File 155 root 0x10D0 is the POW block list, and it is the ONLY thing in
     * the game data that can arrive here with this asset and root.
     * Whole-image pointer census: the only pointer to 155:0x10D0 is file
     * 155's own internal fixup 0x1228 -- the DObjDesc_0x11F8 child that
     * GRInishieMap's PowerBlock ITAttributes name as their data -- and no
     * external fixup anywhere targets it.  p_mobjsubs is NULL and the list
     * has no segment-E call, so an MObj here would be a different draw.
     * Step-witnessed: step 8 means every clause passed and any decline is
     * inside the submit itself.
     *
     * The two SETTIMG words are NOT in size order: word 14 binds the CI4 16x8
     * at 0x0F50 and word 26 the CI4 32x16 at 0x0E48.  Both are measured from
     * the relocated words, not inferred from the tile dimensions. */
    if ((loaded != NULL) &&
        (loaded->asset_id == NDS_NATIVE_INISHIE_POWBLOCK_ASSET) &&
        (ndsRelocNativeRootOffset(loaded, dl) ==
             NDS_NATIVE_INISHIE_POWBLOCK_ROOT))
    {
        u32 powblock_step = 1u;

        if (sNdsRendererAdapterItemSubmitActive != FALSE)
        {
            powblock_step = 2u;
            if ((dobj->parent_gobj != NULL) &&
                (dobj->parent_gobj->id == nGCCommonKindItem))
            {
                powblock_step = 3u;
                if (dobj->mobj == NULL)
                {
                    powblock_step = 4u;
                    if ((loaded->data != NULL) &&
                        (loaded->data_size >=
                         (NDS_NATIVE_INISHIE_POWBLOCK_ROOT +
                          NDS_NATIVE_INISHIE_POWBLOCK_DL_BYTES)) &&
                        (loaded->data_size >=
                         NDS_NATIVE_INISHIE_POWBLOCK_IMAGE_B_END))
                    {
                        powblock_step = 5u;
                        if ((dl[8].words.w0 ==
                             NDS_NATIVE_INISHIE_POWBLOCK_TLUT_W0) &&
                            (dl[14].words.w0 ==
                             NDS_NATIVE_INISHIE_POWBLOCK_IMAGE_A_W0) &&
                            (dl[26].words.w0 ==
                             NDS_NATIVE_INISHIE_POWBLOCK_IMAGE_B_W0))
                        {
                            /* Resolve the palette file FROM the pointer the
                             * list carries, not by asset id: the same
                             * stronger test the laser owner documents above.
                             * An unrelocated chain word lands in no loaded
                             * file at all. */
                            NDSRelocLoadedFile *powblock_pal =
                                ndsRelocFindLoadedFileContaining(
                                    (const void *)(uintptr_t)dl[8].words.w1,
                                    1u);

                            powblock_step = 6u;
                            if ((powblock_pal != NULL) &&
                                (powblock_pal->data != NULL) &&
                                (powblock_pal->asset_id ==
                                 NDS_NATIVE_INISHIE_POWBLOCK_PAL_ASSET) &&
                                (powblock_pal->data_size >=
                                 NDS_NATIVE_INISHIE_POWBLOCK_TLUT_END))
                            {
                                const u8 *pal_base =
                                    (const u8 *)powblock_pal->data;
                                const u8 *pow_base =
                                    (const u8 *)loaded->data;

                                powblock_step = 7u;
                                /* COMPARE the relocated pointers, never
                                 * assume them: the TLUT word must land in
                                 * file 107 at 0x35f8 and both image words
                                 * must land in this loaded file. */
                                if ((dl[8].words.w1 ==
                                     (u32)(uintptr_t)(pal_base +
                                      NDS_NATIVE_INISHIE_POWBLOCK_TLUT_OFFSET)) &&
                                    (dl[14].words.w1 ==
                                     (u32)(uintptr_t)(pow_base +
                                      NDS_NATIVE_INISHIE_POWBLOCK_IMAGE_A_OFFSET)) &&
                                    (dl[26].words.w1 ==
                                     (u32)(uintptr_t)(pow_base +
                                      NDS_NATIVE_INISHIE_POWBLOCK_IMAGE_B_OFFSET)))
                                {
                                    powblock_step = 8u;
                                    inishie_powblock_tlut = pal_base +
                                        NDS_NATIVE_INISHIE_POWBLOCK_TLUT_OFFSET;
                                    inishie_powblock_image_a = pow_base +
                                        NDS_NATIVE_INISHIE_POWBLOCK_IMAGE_A_OFFSET;
                                    inishie_powblock_image_b = pow_base +
                                        NDS_NATIVE_INISHIE_POWBLOCK_IMAGE_B_OFFSET;
                                    inishie_powblock_native_candidate = TRUE;
                                }
                            }
                        }
                    }
                }
            }
        }
        if (powblock_step > gNdsInishiePowblockCandidateStep)
        {
            gNdsInishiePowblockCandidateStep = powblock_step;
        }
    }
#endif
#if NDS_STAGE_SECTOR
    /* File 153 root 0x1c50 is the ArwingLaser weapon list, and it is the ONLY
     * thing in the game data that can arrive here with this asset and root.
     * Whole-image pointer census: the only two pointers to 153:0x1c50 are
     * GRSectorMap external fixups 0x00bc and 0x00f0 -- the ArwingLaser2D/3D
     * WPAttributes.data fields -- and no internal fixup inside file 153
     * targets it.  Both kinds share the list and differ in no drawn respect,
     * so one owner serves both and the sharing is not an ambiguity.
     * Step-witnessed: a count alone cannot say WHICH clause declined, and
     * step 7 means every clause passed. */
    if ((loaded != NULL) &&
        (loaded->asset_id == NDS_NATIVE_SECTOR_LASER_ASSET) &&
        (ndsRelocNativeRootOffset(loaded, dl) == NDS_NATIVE_SECTOR_LASER_ROOT))
    {
        u32 laser_step = 1u;

        if ((dobj->parent_gobj != NULL) &&
            (dobj->parent_gobj->id == nGCCommonKindWeapon))
        {
            laser_step = 2u;
            /* material 0 in the recorded failure: this list carries its own
             * immutable binding, so an MObj here would be a different draw. */
            if (dobj->mobj == NULL)
            {
                laser_step = 3u;
                if (loaded->data_size >=
                    (NDS_NATIVE_SECTOR_LASER_ROOT +
                     NDS_NATIVE_SECTOR_LASER_DL_BYTES))
                {
                    laser_step = 4u;
                    if ((dl[8].words.w0 == NDS_NATIVE_SECTOR_LASER_TLUT_W0) &&
                        (dl[14].words.w0 == NDS_NATIVE_SECTOR_LASER_IMAGE_W0))
                    {
                        /* Resolve the texture file FROM the pointer the list
                         * carries, not by asset id: a by-asset lookup would
                         * still admit a list whose fixup never ran, and the
                         * only public accessor is the containing-file one the
                         * reject path already uses.  This is the stronger
                         * test -- an unrelocated chain word lands in no loaded
                         * file at all. */
                        NDSRelocLoadedFile *laser_tex =
                            ndsRelocFindLoadedFileContaining(
                                (const void *)(uintptr_t)dl[14].words.w1, 1u);

                        laser_step = 5u;
                        if ((laser_tex != NULL) && (laser_tex->data != NULL) &&
                            (laser_tex->asset_id ==
                             NDS_NATIVE_SECTOR_LASER_TEX_ASSET) &&
                            (laser_tex->data_size >=
                             NDS_NATIVE_SECTOR_LASER_TEX_END))
                        {
                            const u8 *tex_base = (const u8 *)laser_tex->data;

                            laser_step = 6u;
                            /* The loader's external-fixup pass rewrites these
                             * two words in place.  If it never ran they are
                             * still chain words, so COMPARE the relocated
                             * pointers -- never assume them, and never bind a
                             * chain word as an image. */
                            if ((dl[8].words.w1 == (u32)(uintptr_t)(tex_base +
                                    NDS_NATIVE_SECTOR_LASER_TLUT_OFFSET)) &&
                                (dl[14].words.w1 == (u32)(uintptr_t)(tex_base +
                                    NDS_NATIVE_SECTOR_LASER_IMAGE_OFFSET)))
                            {
                                laser_step = 7u;
                                sector_laser_tlut = tex_base +
                                    NDS_NATIVE_SECTOR_LASER_TLUT_OFFSET;
                                sector_laser_image = tex_base +
                                    NDS_NATIVE_SECTOR_LASER_IMAGE_OFFSET;
                                sector_laser_native_candidate = TRUE;
                            }
                        }
                    }
                }
            }
        }
        if (laser_step > gNdsSectorLaserCandidateStep)
        {
            gNdsSectorLaserCandidateStep = laser_step;
        }
    }
#endif
#if NDS_STAGE_CASTLE
    /* File 86 root 0x7558 is the bumper quad.  TWO item kinds reach it with
     * IDENTICAL evidence: 251_ITCommonData.c:934 (NBumper) and :1843
     * (GBumper) carry the same DObjDesc and the same p_mobjsubs, and file 86
     * holds exactly ONE pointer to this root.  itNBumperAttachedInitVars
     * swaps dobj->dl to the 0x7AF8 wait list in status 5 alone; in every
     * other NBumper status the drawn list IS this one.  Asset, root and MObj
     * therefore cannot discriminate, and nITKindNBumper is already registered
     * in the live maker table, so this is a present hazard and not a future
     * one.  Read the live kind. */
    if ((loaded != NULL) &&
        (loaded->asset_id == NDS_NATIVE_CASTLE_BUMPER_ASSET) &&
        (ndsRelocNativeRootOffset(loaded, dl) ==
             NDS_NATIVE_CASTLE_BUMPER_ROOT))
    {
        u32 bumper_step = 1u;

        if (sNdsRendererAdapterItemSubmitActive != FALSE)
        {
            bumper_step = 2u;
            if ((dobj->parent_gobj != NULL) &&
                (dobj->parent_gobj->id == nGCCommonKindItem))
            {
                ITStruct *bumper_ip = itGetStruct(dobj->parent_gobj);

                bumper_step = 3u;
                if (bumper_ip != NULL)
                {
                    bumper_step = 4u;
                    gNdsCastleBumperItemKind = (u32)bumper_ip->kind;
                    if (bumper_ip->kind != nITKindGBumper)
                    {
                        /* THE DISCRIMINATOR.  An NBumper is a different item
                         * with its own statuses, spin, throw physics and
                         * second display list; it gets its own owner, never
                         * this one.  Decline and let the loud NO_PROGRAM
                         * reject below record it, with a counter that says
                         * WHY rather than leaving it indistinguishable from
                         * a GBumper the submit refused. */
                        NDS_DIAG(gNdsCastleBumperForeignKindCount++);
                    }
                    else if ((dobj->mobj != NULL) &&
                             (dobj->mobj->next == NULL) &&
                             (ndsRendererAdapterMaterialFlags(dobj->mobj) ==
                                  NDS_NATIVE_CASTLE_BUMPER_MOBJ_FLAGS))
                    {
                        bumper_step = 5u;
                        if (ndsRendererAdapterBuildNativeMaterialSnapshot(
                                dobj->mobj, &castle_bumper_material, FALSE,
                                NULL, NULL) != FALSE)
                        {
                            bumper_step = 6u;
                            gNdsCastleBumperEffects =
                                castle_bumper_material.effects;
                            if (castle_bumper_material.effects ==
                                NDS_RENDERER_NATIVE_MATERIAL_PALETTE_IMAGE)
                            {
                                bumper_step = 7u;
                                if (loaded->data_size >=
                                    NDS_NATIVE_CASTLE_BUMPER_IMAGE_END)
                                {
                                    bumper_step = 8u;
                                    castle_bumper_native_candidate = TRUE;
                                }
                            }
                        }
                    }
                }
            }
        }
        if (bumper_step > gNdsCastleBumperCandidateStep)
        {
            gNdsCastleBumperCandidateStep = bumper_step;
        }
    }
#endif
    /* Ness PK Fire's WPAttributes live in NessSpecial1, but their `data`
     * pointer resolves to NessSpecial3 root 0x0168.  That root selects exactly
     * two live MObjs (segment-E slots 0 then 1), each LIGHT1|LIGHT2 only.
     * Preserve the source weapon/state machine and admit only that exact
     * cross-file root/material contract to the generated two-triangle owner. */
    if ((loaded != NULL) &&
        (loaded->asset_id == NDS_NATIVE_NESS_PKFIRE_ASSET) &&
        (ndsRelocNativeRootOffset(loaded, dl) == NDS_NATIVE_NESS_PKFIRE_ROOT) &&
        (dobj->parent_gobj != NULL) &&
        (dobj->parent_gobj->id == nGCCommonKindWeapon))
    {
        WPStruct *pkfire_wp = wpGetStruct(dobj->parent_gobj);
        MObj *mobj = dobj->mobj;
        u32 i;

        if ((pkfire_wp != NULL) && (pkfire_wp->kind == nWPKindPKFire))
        {
            for (i = 0u; i < NDS_NATIVE_NESS_PKFIRE_GROUP_COUNT; i++)
            {
                if ((mobj == NULL) ||
                    (ndsRendererAdapterBuildNativeMaterialSnapshot(
                         mobj, &ness_pkfire_materials[i], FALSE,
                         NULL, NULL) == FALSE) ||
                    (ness_pkfire_materials[i].effects !=
                         NDS_NATIVE_NESS_PKFIRE_MATERIAL_EFFECTS))
                {
                    break;
                }
                mobj = mobj->next;
            }
            if ((i == NDS_NATIVE_NESS_PKFIRE_GROUP_COUNT) && (mobj == NULL) &&
                (loaded->data_size >= (NDS_NATIVE_NESS_PKFIRE_ROOT +
                                       NDS_NATIVE_NESS_PKFIRE_DL_BYTES)))
            {
                ness_pkfire_native_candidate = TRUE;
            }
        }
    }
    /* PK Thunder's head and trail are NessModel-owned weapons.  Both are
     * immutable two-triangle quads with one live CURRENT_IMAGE MObj; the
     * trail's proc_display supplies prim/env from its live trail_id.  Claim
     * only the exact source asset/root/kind tuples, leaving BattleShip in
     * charge of steering, collision, texture-id animation and transforms. */
    if ((loaded != NULL) &&
        (loaded->asset_id == NDS_NATIVE_NESS_PKTHUNDER_ASSET) &&
        (dobj->parent_gobj != NULL) &&
        (dobj->parent_gobj->id == nGCCommonKindWeapon) &&
        (dobj->mobj != NULL) && (dobj->mobj->next == NULL))
    {
        WPStruct *pkthunder_wp = wpGetStruct(dobj->parent_gobj);
        u32 root_offset = ndsRelocNativeRootOffset(loaded, dl);
        sb32 root_match = FALSE;

        if ((pkthunder_wp != NULL) &&
            (pkthunder_wp->kind == nWPKindPKThunderHead) &&
            (root_offset == NDS_NATIVE_NESS_PKTHUNDER_HEAD_ROOT))
        {
            ness_pkthunder_root_index =
                NDS_NATIVE_NESS_PKTHUNDER_HEAD_INDEX;
            root_match = TRUE;
        }
        else if ((pkthunder_wp != NULL) &&
                 (pkthunder_wp->kind == nWPKindPKThunderTrail) &&
                 (root_offset == NDS_NATIVE_NESS_PKTHUNDER_TRAIL_ROOT) &&
                 ((u32)pkthunder_wp->weapon_vars.pkthunder_trail.trail_id <
                  NDS_NATIVE_NESS_PKTHUNDER_TRAIL_COLOR_COUNT))
        {
            ness_pkthunder_root_index =
                NDS_NATIVE_NESS_PKTHUNDER_TRAIL_INDEX;
            ness_pkthunder_trail_color =
                (u32)pkthunder_wp->weapon_vars.pkthunder_trail.trail_id;
            root_match = TRUE;
        }
        if ((root_match != FALSE) &&
            (ndsRendererAdapterBuildNativeMaterialSnapshot(
                 dobj->mobj, &ness_pkthunder_material, FALSE,
                 NULL, NULL) != FALSE) &&
            (ness_pkthunder_material.effects ==
                 NDS_NATIVE_NESS_PKTHUNDER_MATERIAL_EFFECTS))
        {
            ness_pkthunder_native_candidate = TRUE;
        }
    }
    /* The attached Up-B wave is the third NessModel root in the same packet.
     * It is an Effect GObj rather than a Weapon, but has the identical closed
     * CURRENT_IMAGE material contract. */
    if ((ness_pkthunder_native_candidate == FALSE) &&
        (loaded != NULL) &&
        (loaded->asset_id == NDS_NATIVE_NESS_PKTHUNDER_ASSET) &&
        (ndsRelocNativeRootOffset(loaded, dl) ==
             NDS_NATIVE_NESS_PKTHUNDER_WAVE_ROOT) &&
        (dobj->parent_gobj != NULL) &&
        (dobj->parent_gobj->id == nGCCommonKindEffect) &&
        (dobj->mobj != NULL) && (dobj->mobj->next == NULL) &&
        (ndsRendererAdapterBuildNativeMaterialSnapshot(
             dobj->mobj, &ness_pkthunder_material, FALSE,
             NULL, NULL) != FALSE) &&
        (ness_pkthunder_material.effects ==
             NDS_NATIVE_NESS_PKTHUNDER_MATERIAL_EFFECTS))
    {
        ness_pkthunder_root_index = NDS_NATIVE_NESS_PKTHUNDER_WAVE_INDEX;
        ness_pkthunder_native_candidate = TRUE;
    }
    if ((loaded != NULL) && (loaded->asset_id == NDS_NATIVE_PURIN_SING_ASSET) &&
        (dobj->parent_gobj != NULL) &&
        (dobj->parent_gobj->id == nGCCommonKindEffect) &&
        (dobj->mobj != NULL) && (dobj->mobj->next == NULL))
    {
        static const u32 roots[4] = { NDS_NATIVE_PURIN_SING_ROOT_0,
            NDS_NATIVE_PURIN_SING_ROOT_1, NDS_NATIVE_PURIN_SING_ROOT_2,
            NDS_NATIVE_PURIN_SING_ROOT_3 };
        static const u32 images[4] = { NDS_NATIVE_PURIN_SING_IMAGE_0,
            NDS_NATIVE_PURIN_SING_IMAGE_1, NDS_NATIVE_PURIN_SING_IMAGE_2,
            NDS_NATIVE_PURIN_SING_IMAGE_3 };
        u32 root = ndsRelocNativeRootOffset(loaded, dl);
        u32 i;
        for (i = 0u; i < 4u; i++)
        {
            if ((root == roots[i]) &&
                (ndsRendererAdapterBuildNativeMaterialSnapshot(dobj->mobj,
                    &purin_sing_material, FALSE, NULL, NULL) != FALSE) &&
                (purin_sing_material.effects == NDS_NATIVE_PURIN_SING_MATERIAL_EFFECTS))
            {
                purin_sing_root_index = i;
#if (NDS_1P_GAME || NDS_MENU_SHELL) || NDS_COMPACT_BATTLE_FIGHTERS
                purin_sing_image = ndsRelocNativeAssetAddress(loaded->data, images[i]);
#else
                purin_sing_image = (const u8 *)loaded->data + images[i];
#endif
                purin_sing_native_candidate = (purin_sing_image != NULL);
                break;
            }
        }
    }
    /* Yoshi's match intro is YoshiSpecial2 root 0x0530, not the YoshiModel
     * shield/escape egg. It has one live segment-E MObj whose MatAnimJoint
     * switches between the two source shell images. Keep that MObj live and
     * bake only the immutable DL/geometry. */
    if ((loaded != NULL) &&
        (loaded->asset_id == NDS_NATIVE_YOSHI_ENTRYEGG_ASSET) &&
        (ndsRelocNativeRootOffset(loaded, dl) ==
             NDS_NATIVE_YOSHI_ENTRYEGG_ROOT) &&
        (dobj->parent_gobj != NULL) &&
        (dobj->parent_gobj->id == nGCCommonKindEffect) &&
        (dobj->mobj != NULL) && (dobj->mobj->next == NULL))
    {
#if (NDS_1P_GAME || NDS_MENU_SHELL) || NDS_COMPACT_BATTLE_FIGHTERS
        const void *expected_palette = ndsRelocNativeAssetAddress(
            loaded->data, NDS_NATIVE_YOSHI_ENTRYEGG_PALETTE_OFFSET);
        const void *expected_vertex = ndsRelocNativeAssetAddress(
            loaded->data, NDS_NATIVE_YOSHI_ENTRYEGG_VERTEX_OFFSET);
#else
        const void *expected_palette = (const u8 *)loaded->data +
            NDS_NATIVE_YOSHI_ENTRYEGG_PALETTE_OFFSET;
        const void *expected_vertex = (const u8 *)loaded->data +
            NDS_NATIVE_YOSHI_ENTRYEGG_VERTEX_OFFSET;
#endif
        if ((expected_palette != NULL) && (expected_vertex != NULL) &&
            (dl[9].words.w0 == NDS_NATIVE_YOSHI_ENTRYEGG_PALETTE_W0) &&
            (dl[9].words.w1 == (u32)(uintptr_t)expected_palette) &&
            (dl[13].words.w0 == 0xde000000u) &&
            (dl[13].words.w1 == 0x0e000000u) &&
            (dl[17].words.w0 == NDS_NATIVE_YOSHI_ENTRYEGG_VERTEX_W0) &&
            (dl[17].words.w1 == (u32)(uintptr_t)expected_vertex) &&
            (ndsRendererAdapterBuildNativeMaterialSnapshot(
                 dobj->mobj, &yoshi_entryegg_material, FALSE,
                 NULL, NULL) != FALSE) &&
            (yoshi_entryegg_material.effects ==
                 NDS_NATIVE_YOSHI_ENTRYEGG_MATERIAL_EFFECTS))
        {
            yoshi_entryegg_palette = expected_palette;
            yoshi_entryegg_native_candidate = TRUE;
        }
    }
    /* YoshiModel 0xA860 is one shared, self-contained egg quad. YoshiMain's
     * EggThrow WPAttributes point here and both Yoshi egg EFDesc users point
     * here directly.
     *
     * The production compact battle pack intentionally PRUNES this root's
     * palette/image/vertex spans and replaces its Gfx program with one
     * ENDDL+source-offset identity cell.  ndsRelocNativeRootOffset() is the
     * compact-pack-aware proof of source identity.  The SHA-pinned generated
     * owner bakes the immutable palette, texels, state and geometry, so reading
     * dl[10]/dl[16]/dl[21] here would inspect beyond the compact root cell and
     * can never be a valid admission test.  Keep only live semantic identity:
     * exact source asset/root plus the source GObj kind / EggThrow WP kind. */
    if ((loaded != NULL) &&
        (loaded->asset_id == NDS_NATIVE_YOSHI_EGG_ASSET) &&
        (ndsRelocNativeRootOffset(loaded, dl) == NDS_NATIVE_YOSHI_EGG_ROOT) &&
        (dobj->parent_gobj != NULL) && (dobj->mobj == NULL))
    {
        if (dobj->parent_gobj->id == nGCCommonKindWeapon)
        {
            WPStruct *egg_wp = wpGetStruct(dobj->parent_gobj);

            if ((egg_wp != NULL) && (egg_wp->kind == nWPKindEggThrow))
            {
                yoshi_egg_is_weapon = TRUE;
                yoshi_egg_native_candidate = TRUE;
            }
        }
        else if (dobj->parent_gobj->id == nGCCommonKindEffect)
        {
            yoshi_egg_native_candidate = TRUE;
        }
    }
    /* Neutral-B's victim egg is a separate YoshiSpecial3 effect. Its DObjDesc
     * child points at the fixed root 0x0870. Validate the live relocated
     * palette/image/vertex pointers so this also remains correct if the file is
     * compact-packed. */
    if ((loaded != NULL) &&
        (loaded->asset_id == NDS_NATIVE_YOSHI_EGGLAY_ASSET) &&
        (ndsRelocNativeRootOffset(loaded, dl) == NDS_NATIVE_YOSHI_EGGLAY_ROOT) &&
        (dobj->parent_gobj != NULL) &&
        (dobj->parent_gobj->id == nGCCommonKindEffect) && (dobj->mobj == NULL))
    {
#if (NDS_1P_GAME || NDS_MENU_SHELL) || NDS_COMPACT_BATTLE_FIGHTERS
        const void *expected_palette = ndsRelocNativeAssetAddress(
            loaded->data, NDS_NATIVE_YOSHI_EGGLAY_PALETTE_OFFSET);
        const void *expected_image = ndsRelocNativeAssetAddress(
            loaded->data, NDS_NATIVE_YOSHI_EGGLAY_IMAGE_OFFSET);
        const void *expected_vertex = ndsRelocNativeAssetAddress(
            loaded->data, NDS_NATIVE_YOSHI_EGGLAY_VERTEX_OFFSET);
#else
        const void *expected_palette = (const u8 *)loaded->data +
            NDS_NATIVE_YOSHI_EGGLAY_PALETTE_OFFSET;
        const void *expected_image = (const u8 *)loaded->data +
            NDS_NATIVE_YOSHI_EGGLAY_IMAGE_OFFSET;
        const void *expected_vertex = (const u8 *)loaded->data +
            NDS_NATIVE_YOSHI_EGGLAY_VERTEX_OFFSET;
#endif

        if ((expected_palette != NULL) && (expected_image != NULL) &&
            (expected_vertex != NULL) &&
            (dl[11].words.w0 == NDS_NATIVE_YOSHI_EGGLAY_PALETTE_W0) &&
            (dl[17].words.w0 == NDS_NATIVE_YOSHI_EGGLAY_IMAGE_W0) &&
            (dl[21].words.w0 == NDS_NATIVE_YOSHI_EGGLAY_VERTEX_W0) &&
            (dl[11].words.w1 == (u32)(uintptr_t)expected_palette) &&
            (dl[17].words.w1 == (u32)(uintptr_t)expected_image) &&
            (dl[21].words.w1 == (u32)(uintptr_t)expected_vertex))
        {
            yoshi_egglay_palette = expected_palette;
            yoshi_egglay_image = expected_image;
            yoshi_egglay_native_candidate = TRUE;
        }
    }
    /* Samus Charge Shot, file 321 root 0x270.  Every pointer the program
     * carries is internal to file 321 and it has no MObj, so the admission is
     * asset, root, a Weapon GObj and a NULL MObj -- and that tuple is enough:
     * Kirby's copied Charge Shot reaches the same attributes and therefore the
     * same root, and it should be served by the same owner. */
    if ((loaded != NULL) &&
        (loaded->asset_id == NDS_NATIVE_CHARGESHOT_ASSET) &&
        (ndsRelocNativeRootOffset(loaded, dl) == NDS_NATIVE_CHARGESHOT_ROOT))
    {
        u32 shot_step = 1u;

        if ((dobj->parent_gobj != NULL) &&
            (dobj->parent_gobj->id == nGCCommonKindWeapon))
        {
            shot_step = 2u;
            if (dobj->mobj == NULL)
            {
                shot_step = 3u;
                if (loaded->data_size >= (NDS_NATIVE_CHARGESHOT_ROOT +
                                          NDS_NATIVE_CHARGESHOT_DL_BYTES))
                {
                    shot_step = 4u;
                    charge_shot_native_candidate = TRUE;
                }
            }
        }
        if (shot_step > gNdsChargeShotCandidateStep)
        {
            gNdsChargeShotCandidateStep = shot_step;
        }
    }
    /* Pikachu's air Thunder Jolt, file 342 root 0x0270.  Same admission shape
     * as the Charge Shot above and for the same reason: every pointer the
     * program carries is internal to file 342, it has no MObj, and the tuple of
     * asset, root, a Weapon GObj and a NULL MObj is enough on its own.  A
     * whole-image sweep of all 2,132 O2R files finds exactly ONE pointer
     * reaching this root -- PikachuSpecial1's external fixup at slot 0, the
     * WPAttributes whose `data` field IS this display list -- and no `ll`
     * constant names 0x0270 either, so no live-kind discriminator is needed.
     *
     * Note the two files: the attributes are in file 244 and the geometry in
     * file 342, which is why file 342 holds no pointer to its own root. */
    if ((loaded != NULL) &&
        (loaded->asset_id == NDS_NATIVE_THUNDERJOLT_ASSET) &&
        (ndsRelocNativeRootOffset(loaded, dl) == NDS_NATIVE_THUNDERJOLT_ROOT))
    {
        u32 jolt_step = 1u;

        if ((dobj->parent_gobj != NULL) &&
            (dobj->parent_gobj->id == nGCCommonKindWeapon))
        {
            jolt_step = 2u;
            if (dobj->mobj == NULL)
            {
                jolt_step = 3u;
                if ((loaded->data != NULL) &&
                    (loaded->data_size >= NDS_NATIVE_THUNDERJOLT_FILE_END) &&
                    (loaded->data_size >= (NDS_NATIVE_THUNDERJOLT_ROOT +
                                           NDS_NATIVE_THUNDERJOLT_DL_BYTES)))
                {
                    const u8 *jolt_base = (const u8 *)loaded->data;

                    jolt_step = 4u;
                    /* COMPARE the relocated pointers against this file's own
                     * base -- never assume the loader's fixup pass ran, and
                     * never bind a chain word as an image. */
                    if ((dl[11].words.w0 == NDS_NATIVE_THUNDERJOLT_TLUT_W0) &&
                        (dl[17].words.w0 == NDS_NATIVE_THUNDERJOLT_IMAGE_W0) &&
                        (dl[11].words.w1 ==
                             (u32)(uintptr_t)(jolt_base +
                                 NDS_NATIVE_THUNDERJOLT_TLUT_OFFSET)) &&
                        (dl[17].words.w1 ==
                             (u32)(uintptr_t)(jolt_base +
                                 NDS_NATIVE_THUNDERJOLT_IMAGE_OFFSET)) &&
                        (dl[21].words.w1 ==
                             (u32)(uintptr_t)(jolt_base +
                                 NDS_NATIVE_THUNDERJOLT_VERTEX_OFFSET)))
                    {
                        jolt_step = 5u;
                        thunder_jolt_native_candidate = TRUE;
                    }
                }
            }
        }
        if (jolt_step > gNdsThunderJoltCandidateStep)
        {
            gNdsThunderJoltCandidateStep = jolt_step;
        }
    }
    /* GROUND Thunder Jolt, file 342 DObjDesc 0x1888 reached from
     * llPikachuSpecial1ThunderJoltGroundWeaponAttributes (0x34). Six drawable
     * children at these roots, each one triangle with one segment-0xE hook and
     * its own MObjSub. This began as a reconnaissance witness owning nothing, and
     * the two things it measured are why the owner below is shaped as it is: a
     * root mask of 0x3f, so all six segments are walked and all six must be
     * owned, and an effects word of 0x200 on every one of them, which is
     * CURRENT_IMAGE alone. The witness stays and now drives the admission.
     * The record latched the FOURTH child rather than the first only because a
     * DOBJ_FLAG_HIDDEN child is skipped along with its subtree and records
     * nothing, which is also why the count was never a multiple of six. */
    if ((loaded != NULL) && (loaded->asset_id == NDS_NATIVE_THUNDERJOLT_ASSET) &&
        (dobj->mobj != NULL))
    {
        u32 ground_root = ndsRelocNativeRootOffset(loaded, dl);
        u32 ground_bit = 0u;

        switch (ground_root)
        {
        case 0x1490u: ground_bit = 1u << 0; break;
        case 0x1528u: ground_bit = 1u << 1; break;
        case 0x15c0u: ground_bit = 1u << 2; break;
        case 0x1660u: ground_bit = 1u << 3; break;
        case 0x16f8u: ground_bit = 1u << 4; break;
        case 0x1790u: ground_bit = 1u << 5; break;
        default: break;
        }
        if (ground_bit != 0u)
        {
            u32 ground_step = 1u;

            gNdsThunderGroundRootMask |= ground_bit;
            if ((dobj->parent_gobj != NULL) &&
                (dobj->parent_gobj->id == nGCCommonKindWeapon))
            {
                ground_step = 2u;
                if (ndsRendererAdapterBuildNativeMaterialSnapshot(
                        dobj->mobj, &thunder_ground_material, FALSE, NULL,
                        NULL) != FALSE)
                {
                    ground_step = 3u;
                    gNdsThunderGroundEffectsSeen |=
                        thunder_ground_material.effects;
                    /* The closed contract, measured at 0x200 across all six
                     * roots: CURRENT_IMAGE and nothing else.  Anything broader
                     * means this specialization would silently drop source
                     * material, so decline and let the loud NO_PROGRAM record
                     * below publish it. */
                    if (thunder_ground_material.effects ==
                        NDS_RENDERER_NATIVE_MATERIAL_CURRENT_IMAGE)
                    {
                        ground_step = 4u;
                        /* ground_bit is a one-hot of the six roots in
                         * DObjDesc order, so its trailing zero count IS the
                         * generator's root index. */
                        thunder_ground_root_index = 0u;
                        while (((ground_bit >> thunder_ground_root_index) & 1u)
                                   == 0u)
                        {
                            thunder_ground_root_index++;
                        }
                        thunder_ground_native_candidate = TRUE;
                    }
                }
                else
                {
                    NDS_DIAG(gNdsThunderGroundSnapshotFailCount++);
                }
            }
            if (ground_step > gNdsThunderGroundCandidateStep)
            {
                gNdsThunderGroundCandidateStep = ground_step;
            }
        }
    }
    /* The Thunder Jolt EFFECT, file 342 root 0x2170.  Same asset as both jolts
     * and a third root inside it, reached from
     * llPikachuSpecial3ThunderJoltDObjDesc (0x2258) by the one drawable child
     * of dEFManagerThunderJoltEffectDesc -- so the discriminator that separates
     * it from its two siblings is the EFFECT parent plus the root, and a sweep
     * of the whole image finds no other pointer to 0x2170 at all.
     *
     * Its material must be CURRENT_IMAGE and nothing else.  The list bakes its
     * own palette and takes only the image live, so a broader material here
     * would mean the specialization is dropping source presentation; decline
     * and let the loud NO_PROGRAM record below publish it rather than draw a
     * quietly wrong quad. */
    if ((loaded != NULL) &&
        (loaded->asset_id == NDS_NATIVE_THUNDERJOLTFX_ASSET) &&
        (ndsRelocNativeRootOffset(loaded, dl) == NDS_NATIVE_THUNDERJOLTFX_ROOT))
    {
        u32 fx_step = 1u;

        if ((dobj->parent_gobj != NULL) &&
            (dobj->parent_gobj->id == nGCCommonKindEffect) &&
            (dobj->mobj != NULL))
        {
            fx_step = 2u;
            if ((loaded->data != NULL) &&
                (loaded->data_size >= NDS_NATIVE_THUNDERJOLTFX_TLUT_END) &&
                (loaded->data_size >= (NDS_NATIVE_THUNDERJOLTFX_ROOT +
                                       NDS_NATIVE_THUNDERJOLTFX_DL_BYTES)))
            {
                const u8 *fx_base = (const u8 *)loaded->data;

                fx_step = 3u;
                /* COMPARE the relocated palette pointer against this file's own
                 * base -- never assume the loader's fixup pass ran -- and
                 * require word 17 to still be the segment-E hook rather than a
                 * baked image, because that one word is the whole difference
                 * between this owner and the air jolt's. */
                if ((dl[11].words.w0 == NDS_NATIVE_THUNDERJOLTFX_TLUT_W0) &&
                    (dl[11].words.w1 ==
                         (u32)(uintptr_t)(fx_base +
                             NDS_NATIVE_THUNDERJOLTFX_TLUT_OFFSET)) &&
                    ((dl[17].words.w0 >> 24) == 0xdeu) &&
                    (dl[21].words.w1 ==
                         (u32)(uintptr_t)(fx_base +
                             NDS_NATIVE_THUNDERJOLTFX_VERTEX_OFFSET)))
                {
                    fx_step = 4u;
                    if (ndsRendererAdapterBuildNativeMaterialSnapshot(
                            dobj->mobj, &thunder_fx_material, FALSE, NULL,
                            NULL) != FALSE)
                    {
                        fx_step = 5u;
                        gNdsThunderJoltFxEffectsSeen |=
                            thunder_fx_material.effects;
                        if (thunder_fx_material.effects ==
                            NDS_RENDERER_NATIVE_MATERIAL_CURRENT_IMAGE)
                        {
                            fx_step = 6u;
                            thunder_fx_base = loaded->data;
                            thunder_fx_bytes = loaded->data_size;
                            thunder_fx_native_candidate = TRUE;
                        }
                    }
                    else
                    {
                        NDS_DIAG(gNdsThunderJoltFxSnapshotFailCount++);
                    }
                }
            }
        }
        if (fx_step > gNdsThunderJoltFxCandidateStep)
        {
            gNdsThunderJoltFxCandidateStep = fx_step;
        }
    }

    /* BattleShip DamageSlash, EFCommonEffects1 file 83.  Its two source child
     * roots are 0x75A0 and 0x7668.  Both are immutable quad/list shells with a
     * single segment-E material hook; the live MObj supplies CURRENT_IMAGE,
     * PRIM and both light colours.  Asset+root is source-complete: each root has
     * exactly one referrer in file 83, its own DObjDLLink slot. */
    damage_fly_mdust_native_seen =
        ((loaded != NULL) &&
         (loaded->asset_id == NDS_NATIVE_DAMAGE_FLY_MDUST_ASSET) &&
         (ndsRelocNativeRootOffset(loaded, dl) ==
          NDS_NATIVE_DAMAGE_FLY_MDUST_ROOT)) ? TRUE : FALSE;
    if ((loaded != NULL) &&
        (loaded->asset_id == NDS_NATIVE_DAMAGE_SLASH_ASSET))
    {
        u32 slash_root = ndsRelocNativeRootOffset(loaded, dl);
        u32 slash_step = 0u;
        u32 palette_offset = 0u;
        u32 vertex_offset = 0u;
        u32 root_bit = 0u;

        if (slash_root == NDS_NATIVE_DAMAGE_SLASH_ROOT0)
        {
            palette_offset = NDS_NATIVE_DAMAGE_SLASH_PALETTE0_OFFSET;
            vertex_offset = NDS_NATIVE_DAMAGE_SLASH_VERTEX0_OFFSET;
            root_bit = 1u;
        }
        else if (slash_root == NDS_NATIVE_DAMAGE_SLASH_ROOT1)
        {
            palette_offset = NDS_NATIVE_DAMAGE_SLASH_PALETTE1_OFFSET;
            vertex_offset = NDS_NATIVE_DAMAGE_SLASH_VERTEX1_OFFSET;
            root_bit = 2u;
        }
        if (root_bit != 0u)
        {
            const u8 *slash_base = (const u8 *)loaded->data;

            damage_slash_native_seen = TRUE;
            damage_slash_root = slash_root;
            gNdsDamageSlashRootMask |= root_bit;
            slash_step = 1u;
            if ((dobj->parent_gobj != NULL) &&
                (dobj->parent_gobj->id == nGCCommonKindEffect) &&
                (dobj->mobj != NULL) && (dobj->mobj->next == NULL))
            {
                slash_step = 2u;
                if ((slash_base != NULL) &&
                    (loaded->data_size >= NDS_NATIVE_DAMAGE_SLASH_PALETTE_END) &&
                    (loaded->data_size >=
                         (slash_root + NDS_NATIVE_DAMAGE_SLASH_DL_BYTES)) &&
                    (dl[7].words.w1 ==
                         (u32)(uintptr_t)(slash_base + palette_offset)) &&
                    ((dl[13].words.w0 >> 24) == 0xdeu) &&
                    (dl[13].words.w1 == 0x0e000000u) &&
                    (dl[18].words.w1 ==
                         (u32)(uintptr_t)(slash_base + vertex_offset)))
                {
                    slash_step = 3u;
                    if (ndsRendererAdapterBuildNativeMaterialSnapshot(
                            dobj->mobj, &damage_slash_material, FALSE,
                            NULL, NULL) != FALSE)
                    {
                        const u32 want_effects =
                            NDS_RENDERER_NATIVE_MATERIAL_LIGHT1 |
                            NDS_RENDERER_NATIVE_MATERIAL_LIGHT2 |
                            NDS_RENDERER_NATIVE_MATERIAL_PRIM |
                            NDS_RENDERER_NATIVE_MATERIAL_CURRENT_IMAGE;

                        slash_step = 4u;
                        gNdsDamageSlashEffectsSeen |=
                            damage_slash_material.effects;
                        if (damage_slash_material.effects == want_effects)
                        {
                            slash_step = 5u;
                            damage_slash_base = loaded->data;
                            damage_slash_bytes = loaded->data_size;
                            damage_slash_native_candidate = TRUE;
                        }
                        else
                        {
                            gNdsDamageSlashEffectsRejected |=
                                damage_slash_material.effects;
                        }
                    }
                    else
                    {
                        NDS_DIAG(gNdsDamageSlashSnapshotFailCount++);
                    }
                }
            }
            if (slash_step > gNdsDamageSlashCandidateStep)
            {
                gNdsDamageSlashCandidateStep = slash_step;
            }
        }
    }
#if NDS_LINK
    /* Link's Bomb, file 353 roots 0x16f8 (DL head 0, body) and 0x17e8 (DL
     * head 1, fuse glow).  ONE OBJECT, TWO LISTS: LinkMain's ITAttributes at
     * 0x40 -- llLinkMainBombItemAttributes, named beside nITKindLinkBomb at
     * itlinkbomb.c:20-25 -- points at DObjDesc 353:0x18d8, whose entries 1
     * and 2 carry those two lists, and itDisplayColAnimXLU walks the tree once
     * (itdisplay.c:305) so both arrive per drawn frame, body first.  That
     * ordering is why the failure record named 0x16f8.
     *
     * ONE REFERRER EACH, GAME-WIDE.  A sweep of all 2,132 O2R files finds
     * exactly one pointer to each root -- file 353's own internal fixups
     * 0x18bc and 0x18cc -- and the only external pointers into file 353 at all
     * are LinkMain 0x0004/0x0040/0x0048.  No relocation constant names either
     * root.  So asset+root already discriminate and, unlike the Castle bumper,
     * no live-kind test is REQUIRED.  ITStruct.kind is still read, so that a
     * future second referrer records NO_PROGRAM loudly instead of being drawn
     * by a program baked for the bomb.
     *
     * material 0 in the recorded failure is ITAttributes.p_mobjsubs == NULL
     * (LinkMain 0x44), NOT "untextured": both lists carry their own binding.
     * Step-witnessed, because a count alone cannot say WHICH clause declined;
     * step 8 means every clause passed. */
    if ((loaded != NULL) &&
        (loaded->asset_id == NDS_NATIVE_LINK_BOMB_ASSET))
    {
        u32 bomb_root = ndsRelocNativeRootOffset(loaded, dl);

        if ((bomb_root == NDS_NATIVE_LINK_BOMB_BODY_ROOT) ||
            (bomb_root == NDS_NATIVE_LINK_BOMB_FUSE_ROOT))
        {
            u32 bomb_step = 1u;
            sb32 bomb_is_body =
                (bomb_root == NDS_NATIVE_LINK_BOMB_BODY_ROOT) ? TRUE : FALSE;

            if (sNdsRendererAdapterItemSubmitActive != FALSE)
            {
                bomb_step = 2u;
                if ((dobj->parent_gobj != NULL) &&
                    (dobj->parent_gobj->id == nGCCommonKindItem))
                {
                    ITStruct *bomb_ip = itGetStruct(dobj->parent_gobj);

                    bomb_step = 3u;
                    if (bomb_ip != NULL)
                    {
                        gNdsLinkBombItemKind = (u32)bomb_ip->kind;
                        if (bomb_ip->kind != nITKindLinkBomb)
                        {
                            /* Census says this cannot happen today.  If it
                             * ever does, say WHY rather than leave it
                             * indistinguishable from a submit refusal. */
                            NDS_DIAG(gNdsLinkBombForeignKindCount++);
                        }
                        /* The root ALONE discriminates the two lists, so this
                         * arm does not gate on the DL head.  The source's own
                         * head assignment is DObjDLLink 0x18b8 = list_id 0 and
                         * 0x18c8 = list_id 1, but the port does not reproduce
                         * it: sNdsRendererAdapterItemSubmitHead is written to
                         * 0u once in ndsRendererAdapterSubmitItemDObjTree and
                         * never advanced by the tree walk, so it reads 0 for
                         * BOTH lists.  Gating on it would have declined every
                         * fuse draw at step 3 and left the item half-drawn --
                         * exactly the silent-empty-draw outcome the native
                         * contract forbids.  Witness the observed head instead
                         * so the divergence stays visible; the env-colour
                         * selection above reads the same field, so if it ever
                         * starts tracking the source the witness says so. */
                        else
                        {
                            gNdsLinkBombHead =
                                sNdsRendererAdapterItemSubmitHead;
                            bomb_step = 4u;
                            /* material 0: p_mobjsubs is NULL, so an MObj
                             * here would be a different draw entirely. */
                            if (dobj->mobj == NULL)
                            {
                                bomb_step = 5u;
                                if ((loaded->data != NULL) &&
                                    (loaded->data_size >=
                                         NDS_NATIVE_LINK_BOMB_FILE_END) &&
                                    (loaded->data_size >=
                                         (bomb_root +
                                          ((bomb_is_body != FALSE) ?
                                               NDS_NATIVE_LINK_BOMB_BODY_DL_BYTES :
                                               NDS_NATIVE_LINK_BOMB_FUSE_DL_BYTES))))
                                {
                                    const u8 *bomb_base =
                                        (const u8 *)loaded->data;
                                    u32 bomb_tlut_w0;
                                    u32 bomb_image_w0;
                                    u32 bomb_tlut_w1;
                                    u32 bomb_image_w1;

                                    bomb_step = 6u;
                                    if (bomb_is_body != FALSE)
                                    {
                                        bomb_tlut_w0 = dl[11].words.w0;
                                        bomb_tlut_w1 = dl[11].words.w1;
                                        bomb_image_w0 = dl[17].words.w0;
                                        bomb_image_w1 = dl[17].words.w1;
                                    }
                                    else
                                    {
                                        bomb_tlut_w0 =
                                            NDS_NATIVE_LINK_BOMB_BODY_TLUT_W0;
                                        bomb_tlut_w1 = (u32)(uintptr_t)(
                                            bomb_base +
                                            NDS_NATIVE_LINK_BOMB_TLUT_OFFSET);
                                        bomb_image_w0 = dl[13].words.w0;
                                        bomb_image_w1 = dl[13].words.w1;
                                    }
                                    if ((bomb_tlut_w0 ==
                                             NDS_NATIVE_LINK_BOMB_BODY_TLUT_W0) &&
                                        (bomb_image_w0 ==
                                             ((bomb_is_body != FALSE) ?
                                                  NDS_NATIVE_LINK_BOMB_BODY_IMAGE_W0 :
                                                  NDS_NATIVE_LINK_BOMB_FUSE_IMAGE_W0)))
                                    {
                                        bomb_step = 7u;
                                        /* File 353's fixups are INTERNAL, but
                                         * an unrelocated word is still a chain
                                         * word.  COMPARE the relocated
                                         * pointers against this file's own
                                         * base -- never assume the loader's
                                         * fixup pass ran, and never bind a
                                         * chain word as an image. */
                                        if ((bomb_tlut_w1 ==
                                                 (u32)(uintptr_t)(bomb_base +
                                                     NDS_NATIVE_LINK_BOMB_TLUT_OFFSET)) &&
                                            (bomb_image_w1 ==
                                                 (u32)(uintptr_t)(bomb_base +
                                                     ((bomb_is_body != FALSE) ?
                                                          NDS_NATIVE_LINK_BOMB_BODY_IMAGE_OFFSET :
                                                          NDS_NATIVE_LINK_BOMB_FUSE_IMAGE_OFFSET))))
                                        {
                                            bomb_step = 8u;
                                            link_bomb_root = bomb_root;
                                            link_bomb_native_candidate = TRUE;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            if (bomb_step > gNdsLinkBombCandidateStep)
            {
                gNdsLinkBombCandidateStep = bomb_step;
            }
        }
    }
#endif
#if NDS_STAGE_YAMABUKI
    /* Saffron City's Marumine (Electrode), file 159 root 0x06a0.  ONE OBJECT,
     * ONE LIST: GRYamabukiMap's ITAttributes at 0x104 --
     * llGRYamabukiMapMarumineItemAttributes, named beside nITKindMarumine at
     * itmarumine.c:11-15 -- points at DObjDesc 159:0x0790, whose entry 0 has no
     * display list and whose entry 1 carries this one.  itmanager.c:392 then
     * calls lbCommonEjectTreeDObj, which DELETES that DL-less root and promotes
     * the child, so the GObj ends with a single DObj and
     * itDisplayOPAProcDisplay walks it once (itdisplay.c:189).  Unlike the Link
     * bomb this object offers exactly one list per drawn frame.
     *
     * ONE REFERRER, GAME-WIDE.  A sweep of all 2,132 O2R files finds exactly
     * one pointer to this root -- file 159's own internal fixup 0x07C0, which
     * IS DObjDesc 0x0790 entry 1 -- and the only external pointers into file
     * 159 at all are GRYamabukiMap's thirteen, none of which names 0x06a0.  No
     * relocation constant names the root either.  So asset+root already
     * discriminate and, like the Link bomb and unlike the Castle bumper, no
     * live-kind test is REQUIRED.  ITStruct.kind is still read, so that a
     * future second referrer records NO_PROGRAM loudly instead of being drawn
     * by a program baked for Electrode.
     *
     * material 0 in the recorded failure is ITAttributes.p_mobjsubs == NULL
     * (264_GRYamabukiMap.c:141), NOT "untextured": the list carries its own
     * TLUT and image out of file 159's own internal fixups.  Step-witnessed,
     * because a count alone cannot say WHICH clause declined; step 9 means
     * every clause passed and any decline is inside the submit itself.
     *
     * The DL head is NOT tested.  sNdsRendererAdapterItemSubmitHead is written
     * to 0u once in ndsRendererAdapterSubmitItemDObjTree and never advanced by
     * the tree walk, so it reads 0 for every list of every item; gating on it
     * would be a guard built on a field the port does not maintain.  Asset and
     * root already discriminate this owner completely. */
    if ((loaded != NULL) &&
        (loaded->asset_id == NDS_NATIVE_MARUMINE_ASSET) &&
        (ndsRelocNativeRootOffset(loaded, dl) == NDS_NATIVE_MARUMINE_ROOT))
    {
        u32 marumine_step = 1u;

        if (sNdsRendererAdapterItemSubmitActive != FALSE)
        {
            marumine_step = 2u;
            if ((dobj->parent_gobj != NULL) &&
                (dobj->parent_gobj->id == nGCCommonKindItem))
            {
                ITStruct *marumine_ip = itGetStruct(dobj->parent_gobj);

                marumine_step = 3u;
                if (marumine_ip != NULL)
                {
                    gNdsYamabukiMarumineItemKind = (u32)marumine_ip->kind;
                    if (marumine_ip->kind != nITKindMarumine)
                    {
                        /* The census says this cannot happen today.  If it ever
                         * does, say WHY rather than leave it indistinguishable
                         * from a submit refusal. */
                        NDS_DIAG(gNdsYamabukiMarumineForeignKindCount++);
                    }
                    else
                    {
                        marumine_step = 4u;
                        /* material 0: p_mobjsubs is NULL, so an MObj here would
                         * be a different draw entirely. */
                        if (dobj->mobj == NULL)
                        {
                            marumine_step = 5u;
                            if ((loaded->data != NULL) &&
                                (loaded->data_size >=
                                     NDS_NATIVE_MARUMINE_FILE_END) &&
                                (loaded->data_size >=
                                     (NDS_NATIVE_MARUMINE_ROOT +
                                      NDS_NATIVE_MARUMINE_DL_BYTES)))
                            {
                                const u8 *marumine_base =
                                    (const u8 *)loaded->data;

                                marumine_step = 6u;
                                if ((dl[11].words.w0 ==
                                         NDS_NATIVE_MARUMINE_TLUT_W0) &&
                                    (dl[17].words.w0 ==
                                         NDS_NATIVE_MARUMINE_IMAGE_W0))
                                {
                                    marumine_step = 7u;
                                    /* File 159's fixups are INTERNAL (it has
                                     * zero external fixups), but an unrelocated
                                     * word is still a chain word.  COMPARE the
                                     * relocated pointers against this file's
                                     * own base -- never assume the loader's
                                     * fixup pass ran, and never bind a chain
                                     * word as an image. */
                                    if ((dl[11].words.w1 ==
                                             (u32)(uintptr_t)(marumine_base +
                                                 NDS_NATIVE_MARUMINE_TLUT_OFFSET)) &&
                                        (dl[17].words.w1 ==
                                             (u32)(uintptr_t)(marumine_base +
                                                 NDS_NATIVE_MARUMINE_IMAGE_OFFSET)))
                                    {
                                        marumine_step = 8u;
                                        if (dl[21].words.w1 ==
                                                (u32)(uintptr_t)(marumine_base +
                                                    NDS_NATIVE_MARUMINE_VERTEX_OFFSET))
                                        {
                                            marumine_step = 9u;
                                            marumine_native_candidate = TRUE;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        if (marumine_step > gNdsYamabukiMarumineCandidateStep)
        {
            gNdsYamabukiMarumineCandidateStep = marumine_step;
        }
    }
#endif
#if NDS_STAGE_YAMABUKI
    /* GLucky and Porygon are the two bake-everything Saffron siblings.  Their
     * thirty-word roots contain no 0xDE call, their MObj is NULL, and both the
     * TLUT and image are immutable file-159 fixups.  Keep the same step-9
     * admission contract as Marumine so an inert owner is distinguishable
     * from a submit refusal. */
    if ((loaded != NULL) &&
        (loaded->asset_id == NDS_NATIVE_ITEM_GLUCKY_ASSET) &&
        (ndsRelocNativeRootOffset(loaded, dl) == NDS_NATIVE_ITEM_GLUCKY_ROOT))
    {
        u32 glucky_step = 1u;

        if (sNdsRendererAdapterItemSubmitActive != FALSE)
        {
            glucky_step = 2u;
            if ((dobj->parent_gobj != NULL) &&
                (dobj->parent_gobj->id == nGCCommonKindItem))
            {
                ITStruct *glucky_ip = itGetStruct(dobj->parent_gobj);

                glucky_step = 3u;
                if (glucky_ip != NULL)
                {
                    gNdsYamabukiGluckyItemKind = (u32)glucky_ip->kind;
                    if (glucky_ip->kind != nITKindGLucky)
                    {
                        NDS_DIAG(gNdsYamabukiGluckyForeignKindCount++);
                    }
                    else
                    {
                        glucky_step = 4u;
                        if (dobj->mobj == NULL)
                        {
                            glucky_step = 5u;
                            if ((loaded->data != NULL) &&
                                (loaded->data_size >= NDS_NATIVE_ITEM_GLUCKY_FILE_END) &&
                                (loaded->data_size >= (NDS_NATIVE_ITEM_GLUCKY_ROOT +
                                                       NDS_NATIVE_ITEM_GLUCKY_DL_BYTES)))
                            {
                                const u8 *glucky_base = (const u8 *)loaded->data;

                                glucky_step = 6u;
                                if ((dl[11].words.w0 == NDS_NATIVE_ITEM_GLUCKY_TLUT_W0) &&
                                    (dl[17].words.w0 == NDS_NATIVE_ITEM_GLUCKY_IMAGE_W0))
                                {
                                    glucky_step = 7u;
                                    if ((dl[11].words.w1 ==
                                             (u32)(uintptr_t)(glucky_base +
                                                 NDS_NATIVE_ITEM_GLUCKY_TLUT_OFFSET)) &&
                                        (dl[17].words.w1 ==
                                             (u32)(uintptr_t)(glucky_base +
                                                 NDS_NATIVE_ITEM_GLUCKY_IMAGE_OFFSET)))
                                    {
                                        glucky_step = 8u;
                                        if (dl[21].words.w1 ==
                                                (u32)(uintptr_t)(glucky_base +
                                                    NDS_NATIVE_ITEM_GLUCKY_VERTEX_OFFSET))
                                        {
                                            glucky_step = 9u;
                                            glucky_native_candidate = TRUE;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        if (glucky_step > gNdsYamabukiGluckyCandidateStep)
        {
            gNdsYamabukiGluckyCandidateStep = glucky_step;
        }
    }

    if ((loaded != NULL) &&
        (loaded->asset_id == NDS_NATIVE_ITEM_PORYGON_ASSET) &&
        (ndsRelocNativeRootOffset(loaded, dl) == NDS_NATIVE_ITEM_PORYGON_ROOT))
    {
        u32 porygon_step = 1u;

        if (sNdsRendererAdapterItemSubmitActive != FALSE)
        {
            porygon_step = 2u;
            if ((dobj->parent_gobj != NULL) &&
                (dobj->parent_gobj->id == nGCCommonKindItem))
            {
                ITStruct *porygon_ip = itGetStruct(dobj->parent_gobj);

                porygon_step = 3u;
                if (porygon_ip != NULL)
                {
                    gNdsYamabukiPorygonItemKind = (u32)porygon_ip->kind;
                    if (porygon_ip->kind != nITKindPorygon)
                    {
                        NDS_DIAG(gNdsYamabukiPorygonForeignKindCount++);
                    }
                    else
                    {
                        porygon_step = 4u;
                        if (dobj->mobj == NULL)
                        {
                            porygon_step = 5u;
                            if ((loaded->data != NULL) &&
                                (loaded->data_size >= NDS_NATIVE_ITEM_PORYGON_FILE_END) &&
                                (loaded->data_size >= (NDS_NATIVE_ITEM_PORYGON_ROOT +
                                                       NDS_NATIVE_ITEM_PORYGON_DL_BYTES)))
                            {
                                const u8 *porygon_base = (const u8 *)loaded->data;

                                porygon_step = 6u;
                                if ((dl[11].words.w0 == NDS_NATIVE_ITEM_PORYGON_TLUT_W0) &&
                                    (dl[17].words.w0 == NDS_NATIVE_ITEM_PORYGON_IMAGE_W0))
                                {
                                    porygon_step = 7u;
                                    if ((dl[11].words.w1 ==
                                             (u32)(uintptr_t)(porygon_base +
                                                 NDS_NATIVE_ITEM_PORYGON_TLUT_OFFSET)) &&
                                        (dl[17].words.w1 ==
                                             (u32)(uintptr_t)(porygon_base +
                                                 NDS_NATIVE_ITEM_PORYGON_IMAGE_OFFSET)))
                                    {
                                        porygon_step = 8u;
                                        if (dl[21].words.w1 ==
                                                (u32)(uintptr_t)(porygon_base +
                                                    NDS_NATIVE_ITEM_PORYGON_VERTEX_OFFSET))
                                        {
                                            porygon_step = 9u;
                                            porygon_native_candidate = TRUE;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        if (porygon_step > gNdsYamabukiPorygonCandidateStep)
        {
            gNdsYamabukiPorygonCandidateStep = porygon_step;
        }
    }

    /* Hitokage and Fushigibana have the complementary shape: one exact
     * segment-E call at word 17 and a live MObj whose only measured effect is
     * CURRENT_IMAGE.  The immutable palette and geometry stay pinned here;
     * the image is accepted only through the typed material snapshot. */
    if ((loaded != NULL) &&
        (loaded->asset_id == NDS_NATIVE_ITEM_HITOKAGE_ASSET) &&
        (ndsRelocNativeRootOffset(loaded, dl) == NDS_NATIVE_ITEM_HITOKAGE_ROOT))
    {
        u32 hitokage_step = 1u;

        if (sNdsRendererAdapterItemSubmitActive != FALSE)
        {
            hitokage_step = 2u;
            if ((dobj->parent_gobj != NULL) &&
                (dobj->parent_gobj->id == nGCCommonKindItem))
            {
                ITStruct *hitokage_ip = itGetStruct(dobj->parent_gobj);

                hitokage_step = 3u;
                if (hitokage_ip != NULL)
                {
                    gNdsYamabukiHitokageItemKind = (u32)hitokage_ip->kind;
                    if (hitokage_ip->kind != nITKindHitokage)
                    {
                        NDS_DIAG(gNdsYamabukiHitokageForeignKindCount++);
                    }
                    else
                    {
                        hitokage_step = 4u;
                        if (dobj->mobj != NULL)
                        {
                            hitokage_step = 5u;
                            if ((loaded->data != NULL) &&
                                (loaded->data_size >= NDS_NATIVE_ITEM_HITOKAGE_TLUT_END) &&
                                (loaded->data_size >= (NDS_NATIVE_ITEM_HITOKAGE_ROOT +
                                                       NDS_NATIVE_ITEM_HITOKAGE_DL_BYTES)))
                            {
                                const u8 *hitokage_base = (const u8 *)loaded->data;

                                hitokage_step = 6u;
                                if ((dl[11].words.w0 == NDS_NATIVE_ITEM_HITOKAGE_TLUT_W0) &&
                                    (dl[11].words.w1 ==
                                         (u32)(uintptr_t)(hitokage_base +
                                             NDS_NATIVE_ITEM_HITOKAGE_TLUT_OFFSET)) &&
                                    (dl[17].words.w0 == NDS_NATIVE_ITEM_HITOKAGE_HOOK_W0) &&
                                    (dl[17].words.w1 == NDS_NATIVE_ITEM_HITOKAGE_HOOK_W1) &&
                                    (dl[21].words.w1 ==
                                         (u32)(uintptr_t)(hitokage_base +
                                             NDS_NATIVE_ITEM_HITOKAGE_VERTEX_OFFSET)))
                                {
                                    hitokage_step = 7u;
                                    if (ndsRendererAdapterBuildNativeMaterialSnapshot(
                                            dobj->mobj, &hitokage_material, FALSE,
                                            NULL, NULL) != FALSE)
                                    {
                                        hitokage_step = 8u;
                                        gNdsYamabukiHitokageEffectsSeen |=
                                            hitokage_material.effects;
                                        gNdsYamabukiHitokageImage =
                                            hitokage_material.current_image;
                                        if (hitokage_material.effects ==
                                            NDS_RENDERER_NATIVE_MATERIAL_CURRENT_IMAGE)
                                        {
                                            hitokage_step = 9u;
                                            hitokage_native_candidate = TRUE;
                                        }
                                        else
                                        {
                                            gNdsYamabukiHitokageEffectsRejected =
                                                hitokage_material.effects;
                                        }
                                    }
                                    else
                                    {
                                        NDS_DIAG(gNdsYamabukiHitokageSnapshotFailCount++);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        if (hitokage_step > gNdsYamabukiHitokageCandidateStep)
        {
            gNdsYamabukiHitokageCandidateStep = hitokage_step;
        }
    }

    if ((loaded != NULL) &&
        (loaded->asset_id == NDS_NATIVE_ITEM_FUSHIGIBANA_ASSET) &&
        (ndsRelocNativeRootOffset(loaded, dl) == NDS_NATIVE_ITEM_FUSHIGIBANA_ROOT))
    {
        u32 fushigibana_step = 1u;

        if (sNdsRendererAdapterItemSubmitActive != FALSE)
        {
            fushigibana_step = 2u;
            if ((dobj->parent_gobj != NULL) &&
                (dobj->parent_gobj->id == nGCCommonKindItem))
            {
                ITStruct *fushigibana_ip = itGetStruct(dobj->parent_gobj);

                fushigibana_step = 3u;
                if (fushigibana_ip != NULL)
                {
                    gNdsYamabukiFushigibanaItemKind = (u32)fushigibana_ip->kind;
                    if (fushigibana_ip->kind != nITKindFushigibana)
                    {
                        NDS_DIAG(gNdsYamabukiFushigibanaForeignKindCount++);
                    }
                    else
                    {
                        fushigibana_step = 4u;
                        if (dobj->mobj != NULL)
                        {
                            fushigibana_step = 5u;
                            if ((loaded->data != NULL) &&
                                (loaded->data_size >= NDS_NATIVE_ITEM_FUSHIGIBANA_TLUT_END) &&
                                (loaded->data_size >= (NDS_NATIVE_ITEM_FUSHIGIBANA_ROOT +
                                                       NDS_NATIVE_ITEM_FUSHIGIBANA_DL_BYTES)))
                            {
                                const u8 *fushigibana_base = (const u8 *)loaded->data;

                                fushigibana_step = 6u;
                                if ((dl[11].words.w0 == NDS_NATIVE_ITEM_FUSHIGIBANA_TLUT_W0) &&
                                    (dl[11].words.w1 ==
                                         (u32)(uintptr_t)(fushigibana_base +
                                             NDS_NATIVE_ITEM_FUSHIGIBANA_TLUT_OFFSET)) &&
                                    (dl[17].words.w0 == NDS_NATIVE_ITEM_FUSHIGIBANA_HOOK_W0) &&
                                    (dl[17].words.w1 == NDS_NATIVE_ITEM_FUSHIGIBANA_HOOK_W1) &&
                                    (dl[21].words.w1 ==
                                         (u32)(uintptr_t)(fushigibana_base +
                                             NDS_NATIVE_ITEM_FUSHIGIBANA_VERTEX_OFFSET)))
                                {
                                    fushigibana_step = 7u;
                                    if (ndsRendererAdapterBuildNativeMaterialSnapshot(
                                            dobj->mobj, &fushigibana_material, FALSE,
                                            NULL, NULL) != FALSE)
                                    {
                                        fushigibana_step = 8u;
                                        gNdsYamabukiFushigibanaEffectsSeen |=
                                            fushigibana_material.effects;
                                        gNdsYamabukiFushigibanaImage =
                                            fushigibana_material.current_image;
                                        if (fushigibana_material.effects ==
                                            NDS_RENDERER_NATIVE_MATERIAL_CURRENT_IMAGE)
                                        {
                                            fushigibana_step = 9u;
                                            fushigibana_native_candidate = TRUE;
                                        }
                                        else
                                        {
                                            gNdsYamabukiFushigibanaEffectsRejected =
                                                fushigibana_material.effects;
                                        }
                                    }
                                    else
                                    {
                                        NDS_DIAG(gNdsYamabukiFushigibanaSnapshotFailCount++);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        if (fushigibana_step > gNdsYamabukiFushigibanaCandidateStep)
        {
            gNdsYamabukiFushigibanaCandidateStep = fushigibana_step;
        }
    }
#endif
    /* Five common-item owners selected from the player-visible end of the
     * spawn census. Every admission path proves the exact asset/root/kind and
     * the source material topology before arming a fixed native executor. */
    if ((loaded != NULL) &&
        (loaded->asset_id == NDS_NATIVE_ITEM_STAR_ASSET) &&
        (ndsRelocNativeRootOffset(loaded, dl) == NDS_NATIVE_ITEM_STAR_ROOT))
    {
        u32 star_step = 1u;

        if (sNdsRendererAdapterItemSubmitActive != FALSE)
        {
            star_step = 2u;
            if ((dobj->parent_gobj != NULL) &&
                (dobj->parent_gobj->id == nGCCommonKindItem))
            {
                ITStruct *ip = itGetStruct(dobj->parent_gobj);

                star_step = 3u;
                if (ip != NULL)
                {
                    gNdsItemStarKind = (u32)ip->kind;
                    if (ip->kind != nITKindStar)
                    {
                        NDS_DIAG(gNdsItemStarForeignKindCount++);
                    }
                    else if ((dobj->mobj != NULL) &&
                             (dobj->mobj->next != NULL) &&
                             (dobj->mobj->next->next == NULL))
                    {
                        const u8 *base = (const u8 *)loaded->data;

                        star_step = 4u;
                        if ((base != NULL) &&
                            (loaded->data_size >= NDS_NATIVE_ITEM_STAR_FILE_END))
                        {
                            star_step = 5u;
                            if ((dl[8].words.w0 == NDS_NATIVE_ITEM_STAR_HOOK0_W0) &&
                                (dl[8].words.w1 == NDS_NATIVE_ITEM_STAR_HOOK0_W1) &&
                                (dl[23].words.w0 == NDS_NATIVE_ITEM_STAR_HOOK1_W0) &&
                                (dl[23].words.w1 == NDS_NATIVE_ITEM_STAR_HOOK1_W1) &&
                                (dl[14].words.w0 == NDS_NATIVE_ITEM_STAR_IMAGE_W0) &&
                                (dl[14].words.w1 ==
                                     (u32)(uintptr_t)(base + NDS_NATIVE_ITEM_STAR_IMAGE_OFFSET)) &&
                                (dl[18].words.w1 ==
                                     (u32)(uintptr_t)(base + NDS_NATIVE_ITEM_STAR_VERTEX0_OFFSET)) &&
                                (dl[29].words.w1 ==
                                     (u32)(uintptr_t)(base + NDS_NATIVE_ITEM_STAR_VERTEX1_OFFSET)))
                            {
                                star_step = 6u;
                                if ((ndsRendererAdapterBuildNativeMaterialSnapshot(
                                         dobj->mobj, &item_star_material0, FALSE,
                                         NULL, NULL) != FALSE) &&
                                    (ndsRendererAdapterBuildNativeMaterialSnapshot(
                                         dobj->mobj->next, &item_star_material1, FALSE,
                                         NULL, NULL) != FALSE))
                                {
                                    star_step = 7u;
                                    gNdsItemStarEffectsSeen |=
                                        item_star_material0.effects |
                                        item_star_material1.effects;
                                    if ((item_star_material0.effects ==
                                             NDS_RENDERER_NATIVE_MATERIAL_PALETTE_IMAGE) &&
                                        (item_star_material1.effects ==
                                             NDS_RENDERER_NATIVE_MATERIAL_PALETTE_IMAGE))
                                    {
                                        star_step = 9u;
                                        item_star_native_candidate = TRUE;
                                    }
                                    else
                                    {
                                        gNdsItemStarEffectsRejected =
                                            item_star_material0.effects |
                                            item_star_material1.effects;
                                    }
                                }
                                else
                                {
                                    NDS_DIAG(gNdsItemStarSnapshotFailCount++);
                                }
                            }
                        }
                    }
                }
            }
        }
        if (star_step > gNdsItemStarCandidateStep)
        {
            gNdsItemStarCandidateStep = star_step;
        }
    }

    {
        u32 root = (loaded != NULL) ? ndsRelocNativeRootOffset(loaded, dl) : 0u;

        if ((loaded != NULL) &&
            (loaded->asset_id == NDS_NATIVE_ITEM_SWORD_ASSET) &&
            ((root == NDS_NATIVE_ITEM_SWORD_BLADE_ROOT) ||
             (root == NDS_NATIVE_ITEM_SWORD_HILT_ROOT)))
        {
            u32 sword_step = 1u;

            if (sNdsRendererAdapterItemSubmitActive != FALSE)
            {
                sword_step = 2u;
                if ((dobj->parent_gobj != NULL) &&
                    (dobj->parent_gobj->id == nGCCommonKindItem))
                {
                    ITStruct *ip = itGetStruct(dobj->parent_gobj);

                    sword_step = 3u;
                    if (ip != NULL)
                    {
                        gNdsItemSwordKind = (u32)ip->kind;
                        if (ip->kind != nITKindSword)
                        {
                            NDS_DIAG(gNdsItemSwordForeignKindCount++);
                        }
                        else if (dobj->mobj == NULL)
                        {
                            const u8 *base = (const u8 *)loaded->data;

                            sword_step = 4u;
                            if ((base != NULL) &&
                                (loaded->data_size >= NDS_NATIVE_ITEM_SWORD_FILE_END))
                            {
                                sb32 shape_ok = FALSE;

                                sword_step = 5u;
                                if (root == NDS_NATIVE_ITEM_SWORD_BLADE_ROOT)
                                {
                                    shape_ok = (dl[7].words.w1 ==
                                        (u32)(uintptr_t)(base +
                                            NDS_NATIVE_ITEM_SWORD_BLADE_VERTEX_OFFSET));
                                }
                                else
                                {
                                    shape_ok =
                                        (dl[10].words.w0 ==
                                             NDS_NATIVE_ITEM_SWORD_HILT_IMAGE_W0) &&
                                        (dl[10].words.w1 ==
                                             (u32)(uintptr_t)(base +
                                                 NDS_NATIVE_ITEM_SWORD_HILT_IMAGE_OFFSET)) &&
                                        (dl[15].words.w1 ==
                                             (u32)(uintptr_t)(base +
                                                 NDS_NATIVE_ITEM_SWORD_HILT_VERTEX_OFFSET));
                                }
                                if (shape_ok != FALSE)
                                {
                                    sword_step = 9u;
                                    item_sword_root = root;
                                    gNdsItemSwordRoot = root;
                                    item_sword_native_candidate = TRUE;
                                }
                            }
                        }
                    }
                }
            }
            if (sword_step > gNdsItemSwordCandidateStep)
            {
                gNdsItemSwordCandidateStep = sword_step;
            }
        }
    }

    if ((loaded != NULL) &&
        (loaded->asset_id == NDS_NATIVE_ITEM_HAMMER_ASSET) &&
        (ndsRelocNativeRootOffset(loaded, dl) == NDS_NATIVE_ITEM_HAMMER_ROOT))
    {
        u32 hammer_step = 1u;

        if (sNdsRendererAdapterItemSubmitActive != FALSE)
        {
            hammer_step = 2u;
            if ((dobj->parent_gobj != NULL) &&
                (dobj->parent_gobj->id == nGCCommonKindItem))
            {
                ITStruct *ip = itGetStruct(dobj->parent_gobj);

                hammer_step = 3u;
                if (ip != NULL)
                {
                    gNdsItemHammerKind = (u32)ip->kind;
                    if (ip->kind != nITKindHammer)
                    {
                        NDS_DIAG(gNdsItemHammerForeignKindCount++);
                    }
                    else if (dobj->mobj == NULL)
                    {
                        const u8 *base = (const u8 *)loaded->data;

                        hammer_step = 4u;
                        if ((base != NULL) &&
                            (loaded->data_size >= NDS_NATIVE_ITEM_HAMMER_FILE_END) &&
                            (dl[11].words.w0 == NDS_NATIVE_ITEM_HAMMER_TLUT_W0) &&
                            (dl[11].words.w1 ==
                                 (u32)(uintptr_t)(base + NDS_NATIVE_ITEM_HAMMER_TLUT_OFFSET)) &&
                            (dl[17].words.w0 == NDS_NATIVE_ITEM_HAMMER_IMAGE_W0) &&
                            (dl[17].words.w1 ==
                                 (u32)(uintptr_t)(base + NDS_NATIVE_ITEM_HAMMER_IMAGE_OFFSET)) &&
                            (dl[21].words.w1 ==
                                 (u32)(uintptr_t)(base + NDS_NATIVE_ITEM_HAMMER_VERTEX0_OFFSET)) &&
                            (dl[36].words.w1 ==
                                 (u32)(uintptr_t)(base + NDS_NATIVE_ITEM_HAMMER_VERTEX1_OFFSET)))
                        {
                            hammer_step = 9u;
                            item_hammer_native_candidate = TRUE;
                        }
                    }
                }
            }
        }
        if (hammer_step > gNdsItemHammerCandidateStep)
        {
            gNdsItemHammerCandidateStep = hammer_step;
        }
    }

    {
        u32 root = (loaded != NULL) ? ndsRelocNativeRootOffset(loaded, dl) : 0u;

        if ((loaded != NULL) &&
            (loaded->asset_id == NDS_NATIVE_ITEM_MBALL_ASSET) &&
            ((root == NDS_NATIVE_ITEM_MBALL_BAKED_ROOT) ||
             (root == NDS_NATIVE_ITEM_MBALL_LIVE_ROOT)))
        {
            u32 mball_step = 1u;
            /* BOTH POKE BALLS ARE ONE SOURCE DESCRIPTOR, SO THEY ARE ONE
             * NATIVE OWNER.
             *
             * These two roots are the two drawable children of the DObjDesc
             * at ITCommonObject+0x9430. The ground item reaches it as
             * ITAttributes.data at ITCommonData+0x6E4; BattleShip's entry
             * effect, efManagerMBallThrownMakeEffect (efmanager.c:5248),
             * reads that same fixed-up +0x6E4 pointer and SUBTRACTS 0x9430
             * from it to recover ITCommonObject's base -- so the subtrahend
             * IS this descriptor. Same tree, same MObjSub table at +0x9120,
             * same eight CURRENT_IMAGE frames; only the AnimJoint and
             * MatAnimJoint differ (the L/R pair the maker binds from lr).
             *
             * Requiring an ITEM GObj in the item display layer was therefore
             * the whole of the remaining P03 failure: the effect constructed,
             * arrived here as an EFFECT GObj in the effect layer, matched
             * nothing, and was published as DIAG_NATIVE domain 2 root 0x9340
             * reason 1 (NO_PROGRAM) -- which reads like missing geometry and
             * is not. Admit the effect owner against the same asset and the
             * same two roots; do NOT bake this geometry a second time.
             *
             * itGetStruct is deliberately not called on the effect GObj: an
             * EFFECT's obj is not an ITStruct. */
            const sb32 mball_is_effect =
                ((sNdsRendererAdapterEffectSubmitActive != FALSE) &&
                 (dobj->parent_gobj != NULL) &&
                 (dobj->parent_gobj->id == nGCCommonKindEffect)) ?
                    TRUE : FALSE;

            if ((sNdsRendererAdapterItemSubmitActive != FALSE) ||
                (mball_is_effect != FALSE))
            {
                mball_step = 2u;
                if ((dobj->parent_gobj != NULL) &&
                    ((dobj->parent_gobj->id == nGCCommonKindItem) ||
                     (mball_is_effect != FALSE)))
                {
                    ITStruct *ip = (mball_is_effect != FALSE) ? NULL :
                        itGetStruct(dobj->parent_gobj);

                    mball_step = 3u;
                    if ((ip != NULL) || (mball_is_effect != FALSE))
                    {
                        if (mball_is_effect != FALSE)
                        {
                            /* Bit 0 = 0x9250, bit 1 = 0x9340. One counter
                             * cannot say both halves of the ball arrived. */
                            gNdsEntryMBallThrownRootMask |=
                                (root == NDS_NATIVE_ITEM_MBALL_BAKED_ROOT) ?
                                    1u : 2u;
                        }
                        else
                        {
                            gNdsItemMBallKind = (u32)ip->kind;
                        }
                        if ((mball_is_effect == FALSE) &&
                            (ip->kind != nITKindMBall))
                        {
                            NDS_DIAG(gNdsItemMBallForeignKindCount++);
                        }
                        else if ((loaded->data != NULL) &&
                                 (loaded->data_size >= NDS_NATIVE_ITEM_MBALL_FILE_END))
                        {
                            const u8 *base = (const u8 *)loaded->data;

                            mball_step = 4u;
                            if (root == NDS_NATIVE_ITEM_MBALL_BAKED_ROOT)
                            {
                                if ((dobj->mobj == NULL) &&
                                    (dl[11].words.w0 ==
                                         NDS_NATIVE_ITEM_MBALL_BAKED_TLUT_W0) &&
                                    (dl[11].words.w1 ==
                                         (u32)(uintptr_t)(base +
                                             NDS_NATIVE_ITEM_MBALL_BAKED_TLUT_OFFSET)) &&
                                    (dl[17].words.w0 ==
                                         NDS_NATIVE_ITEM_MBALL_BAKED_IMAGE_W0) &&
                                    (dl[17].words.w1 ==
                                         (u32)(uintptr_t)(base +
                                             NDS_NATIVE_ITEM_MBALL_BAKED_IMAGE_OFFSET)) &&
                                    (dl[21].words.w1 ==
                                         (u32)(uintptr_t)(base +
                                             NDS_NATIVE_ITEM_MBALL_BAKED_VERTEX_OFFSET)))
                                {
                                    mball_step = 9u;
                                    item_mball_root = root;
                                    gNdsItemMBallRoot = root;
                                    item_mball_native_candidate = TRUE;
                                    item_mball_from_effect = mball_is_effect;
                                }
                            }
                            else if ((dobj->mobj != NULL) &&
                                     (dobj->mobj->next == NULL) &&
                                     (dl[10].words.w0 ==
                                          NDS_NATIVE_ITEM_MBALL_LIVE_TLUT_W0) &&
                                     (dl[10].words.w1 ==
                                          (u32)(uintptr_t)(base +
                                              NDS_NATIVE_ITEM_MBALL_LIVE_TLUT_OFFSET)) &&
                                     (dl[16].words.w0 ==
                                          NDS_NATIVE_ITEM_MBALL_LIVE_HOOK_W0) &&
                                     (dl[16].words.w1 ==
                                          NDS_NATIVE_ITEM_MBALL_LIVE_HOOK_W1) &&
                                     (dl[21].words.w1 ==
                                          (u32)(uintptr_t)(base +
                                              NDS_NATIVE_ITEM_MBALL_LIVE_VERTEX_OFFSET)))
                            {
                                mball_step = 5u;
                                if (ndsRendererAdapterBuildNativeMaterialSnapshot(
                                        dobj->mobj, &item_mball_material, FALSE,
                                        NULL, NULL) != FALSE)
                                {
                                    mball_step = 6u;
                                    gNdsItemMBallEffectsSeen |=
                                        item_mball_material.effects;
                                    if (item_mball_material.effects ==
                                            NDS_RENDERER_NATIVE_MATERIAL_CURRENT_IMAGE)
                                    {
                                        mball_step = 9u;
                                        item_mball_root = root;
                                        gNdsItemMBallRoot = root;
                                        item_mball_native_candidate = TRUE;
                                        item_mball_from_effect =
                                            mball_is_effect;
                                    }
                                    else
                                    {
                                        gNdsItemMBallEffectsRejected =
                                            item_mball_material.effects;
                                    }
                                }
                                else
                                {
                                    NDS_DIAG(gNdsItemMBallSnapshotFailCount++);
                                }
                            }
                        }
                    }
                }
            }
            if (mball_step > gNdsItemMBallCandidateStep)
            {
                gNdsItemMBallCandidateStep = mball_step;
            }
        }
    }

    {
        u32 root = (loaded != NULL) ? ndsRelocNativeRootOffset(loaded, dl) : 0u;

        if ((loaded != NULL) &&
            (loaded->asset_id == NDS_NATIVE_ITEM_KIRBYSTAR_ASSET) &&
            (root == NDS_NATIVE_ITEM_KIRBYSTAR_ROOT))
        {
            u32 kirbystar_step = 1u;
            /* FOUR SOURCE OWNERS, ONE ROOT, ONE BAKE -- AND NO KIND
             * DISCRIMINATOR IS NEEDED.
             *
             * ITCommonObject+0x5458 is dITCommonObject_StarRod_Weapon_data. A
             * whole-image referrer census over ITCommonData and ITCommonObject
             * finds exactly three pointers reaching it and none from any other
             * file: WPAttributes.data at ITCommonData+0x4D4 (StarRod) and
             * +0x508 (StarRodSmash), and the DObjDLLink pair at
             * ITCommonObject+0x550C. Kirby's two stars reach the same DL by
             * subtracting 0x5458 from that first pointer to recover the file
             * base -- the Poke Ball's trick at 0x9430, one file over. All four
             * draw the identical fixed quad with identical fixed material
             * state, so exact asset plus exact root IS the whole admission
             * test and the arm does not have to tell them apart.
             *
             * Admit from the WEAPON layer and the EFFECT layer. Requiring an
             * ITEM GObj was the entirety of the Poke Ball's remaining P03
             * failure (see the MBall arm above); this root has no item owner
             * at all, so the same clause would have refused every one of its
             * four users. itGetStruct is deliberately never called here.
             *
             * Counted separately: FromEffectCount isolates Kirby's two stars
             * from the Star Rod's swings, because a single draw count on a
             * shared root cannot say which owner drew and K04 is only proven
             * by the effect half. */
            const sb32 kirbystar_is_effect =
                ((sNdsRendererAdapterEffectSubmitActive != FALSE) &&
                 (dobj->parent_gobj != NULL) &&
                 (dobj->parent_gobj->id == nGCCommonKindEffect)) ?
                    TRUE : FALSE;

            if ((dobj->parent_gobj != NULL) && (loaded->data != NULL) &&
                (loaded->data_size >= NDS_NATIVE_ITEM_KIRBYSTAR_FILE_END))
            {
                const u8 *base = (const u8 *)loaded->data;

                kirbystar_step = 2u;
                /* No 0xDE and no MObjSub on any of the four owners, so a live
                 * MObj here means the source shape changed and the bake is
                 * stale. Refuse loudly rather than draw frozen material. */
                if ((dobj->mobj == NULL) &&
                    (dl[11].words.w0 == NDS_NATIVE_ITEM_KIRBYSTAR_IMAGE_W0) &&
                    (dl[11].words.w1 ==
                         (u32)(uintptr_t)(base +
                             NDS_NATIVE_ITEM_KIRBYSTAR_IMAGE_OFFSET)) &&
                    (dl[15].words.w1 ==
                         (u32)(uintptr_t)(base +
                             NDS_NATIVE_ITEM_KIRBYSTAR_VERTEX_OFFSET)))
                {
                    kirbystar_step = 9u;
                    item_kirbystar_native_candidate = TRUE;
                    if (kirbystar_is_effect != FALSE)
                    {
                        NDS_DIAG(gNdsItemKirbyStarFromEffectCount++);
                    }
                }
            }
            if (kirbystar_step > gNdsItemKirbyStarCandidateStep)
            {
                gNdsItemKirbyStarCandidateStep = kirbystar_step;
            }
        }
    }

    if ((loaded != NULL) &&
        (loaded->asset_id == NDS_NATIVE_ITEM_GSHELL_ASSET) &&
        (ndsRelocNativeRootOffset(loaded, dl) == NDS_NATIVE_ITEM_GSHELL_ROOT))
    {
        u32 gshell_step = 1u;

        if (sNdsRendererAdapterItemSubmitActive != FALSE)
        {
            gshell_step = 2u;
            if ((dobj->parent_gobj != NULL) &&
                (dobj->parent_gobj->id == nGCCommonKindItem))
            {
                ITStruct *ip = itGetStruct(dobj->parent_gobj);

                gshell_step = 3u;
                if (ip != NULL)
                {
                    gNdsItemGShellKind = (u32)ip->kind;
                    if (ip->kind == nITKindRShell)
                    {
                        /* Red Shell shares this exact source root and has its
                         * own kind-gated owner below. */
                    }
                    else if (ip->kind != nITKindGShell)
                    {
                        NDS_DIAG(gNdsItemGShellForeignKindCount++);
                    }
                    else if ((dobj->mobj != NULL) &&
                             (dobj->mobj->next == NULL) &&
                             (loaded->data != NULL) &&
                             (loaded->data_size >= NDS_NATIVE_ITEM_GSHELL_FILE_END))
                    {
                        const u8 *base = (const u8 *)loaded->data;

                        gshell_step = 4u;
                        if ((dl[12].words.w0 == NDS_NATIVE_ITEM_GSHELL_HOOK_W0) &&
                            (dl[12].words.w1 == NDS_NATIVE_ITEM_GSHELL_HOOK_W1) &&
                            (dl[17].words.w1 ==
                                 (u32)(uintptr_t)(base +
                                     NDS_NATIVE_ITEM_GSHELL_VERTEX_OFFSET)))
                        {
                            gshell_step = 5u;
                            if (ndsRendererAdapterBuildNativeMaterialSnapshot(
                                    dobj->mobj, &item_gshell_material, FALSE,
                                    NULL, NULL) != FALSE)
                            {
                                const u32 expected =
                                    NDS_RENDERER_NATIVE_MATERIAL_PALETTE_IMAGE |
                                    NDS_RENDERER_NATIVE_MATERIAL_PALETTE_TLUT |
                                    NDS_RENDERER_NATIVE_MATERIAL_CURRENT_IMAGE;

                                gshell_step = 6u;
                                gNdsItemGShellEffectsSeen |=
                                    item_gshell_material.effects;
                                if (item_gshell_material.effects == expected)
                                {
                                    gshell_step = 9u;
                                    item_gshell_native_candidate = TRUE;
                                }
                                else
                                {
                                    gNdsItemGShellEffectsRejected =
                                        item_gshell_material.effects;
                                }
                            }
                            else
                            {
                                NDS_DIAG(gNdsItemGShellSnapshotFailCount++);
                            }
                        }
                    }
                }
            }
        }
        if (gshell_step > gNdsItemGShellCandidateStep)
        {
            gNdsItemGShellCandidateStep = gshell_step;
        }
    }

    if ((loaded != NULL) &&
        (loaded->asset_id == NDS_NATIVE_ITEM_RSHELL_ASSET) &&
        (ndsRelocNativeRootOffset(loaded, dl) == NDS_NATIVE_ITEM_RSHELL_ROOT))
    {
        u32 rshell_step = 1u;

        if (sNdsRendererAdapterItemSubmitActive != FALSE)
        {
            rshell_step = 2u;
            if ((dobj->parent_gobj != NULL) &&
                (dobj->parent_gobj->id == nGCCommonKindItem))
            {
                ITStruct *ip = itGetStruct(dobj->parent_gobj);

                rshell_step = 3u;
                if (ip != NULL)
                {
                    gNdsItemRShellKind = (u32)ip->kind;
                    if (ip->kind == nITKindGShell)
                    {
                        /* Green Shell owns the same shared root above. */
                    }
                    else if (ip->kind != nITKindRShell)
                    {
                        NDS_DIAG(gNdsItemRShellForeignKindCount++);
                    }
                    else if ((dobj->mobj != NULL) &&
                             (dobj->mobj->next == NULL) &&
                             (loaded->data != NULL) &&
                             (loaded->data_size >= NDS_NATIVE_ITEM_RSHELL_FILE_END))
                    {
                        const u8 *base = (const u8 *)loaded->data;

                        rshell_step = 4u;
                        if ((dl[12].words.w0 == NDS_NATIVE_ITEM_RSHELL_HOOK_W0) &&
                            (dl[12].words.w1 == NDS_NATIVE_ITEM_RSHELL_HOOK_W1) &&
                            (dl[17].words.w1 ==
                                 (u32)(uintptr_t)(base +
                                     NDS_NATIVE_ITEM_RSHELL_VERTEX_OFFSET)))
                        {
                            rshell_step = 5u;
                            if (ndsRendererAdapterBuildNativeMaterialSnapshot(
                                    dobj->mobj, &item_rshell_material, FALSE,
                                    NULL, NULL) != FALSE)
                            {
                                const u32 expected =
                                    NDS_RENDERER_NATIVE_MATERIAL_PALETTE_IMAGE |
                                    NDS_RENDERER_NATIVE_MATERIAL_PALETTE_TLUT |
                                    NDS_RENDERER_NATIVE_MATERIAL_CURRENT_IMAGE;

                                rshell_step = 6u;
                                gNdsItemRShellEffectsSeen |=
                                    item_rshell_material.effects;
                                if (item_rshell_material.effects == expected)
                                {
                                    rshell_step = 9u;
                                    item_rshell_native_candidate = TRUE;
                                }
                                else
                                {
                                    gNdsItemRShellEffectsRejected =
                                        item_rshell_material.effects;
                                }
                            }
                            else
                            {
                                NDS_DIAG(gNdsItemRShellSnapshotFailCount++);
                            }
                        }
                    }
                }
            }
        }
        if (rshell_step > gNdsItemRShellCandidateStep)
        {
            gNdsItemRShellCandidateStep = rshell_step;
        }
    }

    {
        u32 root = (loaded != NULL) ? ndsRelocNativeRootOffset(loaded, dl) : 0u;

        if ((loaded != NULL) &&
            (loaded->asset_id == NDS_NATIVE_ITEM_BAT_ASSET) &&
            ((root == NDS_NATIVE_ITEM_BAT_HEADER_ROOT) ||
             (root == NDS_NATIVE_ITEM_BAT_SECOND_ROOT)))
        {
            u32 bat_step = 1u;

            if (sNdsRendererAdapterItemSubmitActive != FALSE)
            {
                bat_step = 2u;
                if ((dobj->parent_gobj != NULL) &&
                    (dobj->parent_gobj->id == nGCCommonKindItem))
                {
                    ITStruct *ip = itGetStruct(dobj->parent_gobj);

                    bat_step = 3u;
                    if (ip != NULL)
                    {
                        gNdsItemBatKind = (u32)ip->kind;
                        if (ip->kind != nITKindBat)
                        {
                            NDS_DIAG(gNdsItemBatForeignKindCount++);
                        }
                        else if ((dobj->mobj == NULL) &&
                                 (loaded->data != NULL) &&
                                 (loaded->data_size >= NDS_NATIVE_ITEM_BAT_FILE_END))
                        {
                            const u8 *base = (const u8 *)loaded->data;
                            sb32 shape_ok;

                            bat_step = 4u;
                            if (root == NDS_NATIVE_ITEM_BAT_HEADER_ROOT)
                            {
                                shape_ok =
                                    (dl[9].words.w0 ==
                                         NDS_NATIVE_ITEM_BAT_BRANCH_W0) &&
                                    (dl[9].words.w1 ==
                                         (u32)(uintptr_t)(base +
                                             NDS_NATIVE_ITEM_BAT_CALLEE_ROOT));
                            }
                            else
                            {
                                shape_ok =
                                    (dl[14].words.w1 ==
                                         (u32)(uintptr_t)(base +
                                             NDS_NATIVE_ITEM_BAT_VERTEX2_OFFSET));
                            }
                            if (shape_ok != FALSE)
                            {
                                bat_step = 9u;
                                item_bat_root = root;
                                gNdsItemBatRoot = root;
                                item_bat_native_candidate = TRUE;
                            }
                        }
                    }
                }
            }
            if (bat_step > gNdsItemBatCandidateStep)
            {
                gNdsItemBatCandidateStep = bat_step;
            }
        }
    }

    {
        u32 root = (loaded != NULL) ? ndsRelocNativeRootOffset(loaded, dl) : 0u;

        if ((loaded != NULL) &&
            (loaded->asset_id == NDS_NATIVE_ITEM_CAPSULE_ASSET) &&
            ((root == NDS_NATIVE_ITEM_CAPSULE_HEADER_ROOT) ||
             (root == NDS_NATIVE_ITEM_CAPSULE_SECOND_ROOT) ||
             (root == NDS_NATIVE_ITEM_CAPSULE_THIRD_ROOT)))
        {
            u32 capsule_step = 1u;

            if (sNdsRendererAdapterItemSubmitActive != FALSE)
            {
                capsule_step = 2u;
                if ((dobj->parent_gobj != NULL) &&
                    (dobj->parent_gobj->id == nGCCommonKindItem))
                {
                    ITStruct *ip = itGetStruct(dobj->parent_gobj);

                    capsule_step = 3u;
                    if (ip != NULL)
                    {
                        gNdsItemCapsuleKind = (u32)ip->kind;
                        if (ip->kind != nITKindCapsule)
                        {
                            NDS_DIAG(gNdsItemCapsuleForeignKindCount++);
                        }
                        else if ((dobj->mobj == NULL) &&
                                 (loaded->data != NULL) &&
                                 (loaded->data_size >= NDS_NATIVE_ITEM_CAPSULE_FILE_END))
                        {
                            const u8 *base = (const u8 *)loaded->data;
                            sb32 shape_ok;

                            capsule_step = 4u;
                            if (root == NDS_NATIVE_ITEM_CAPSULE_HEADER_ROOT)
                            {
                                shape_ok =
                                    (dl[10].words.w0 ==
                                         NDS_NATIVE_ITEM_CAPSULE_BRANCH_W0) &&
                                    (dl[10].words.w1 ==
                                         (u32)(uintptr_t)(base +
                                             NDS_NATIVE_ITEM_CAPSULE_CALLEE_ROOT));
                            }
                            else if (root == NDS_NATIVE_ITEM_CAPSULE_SECOND_ROOT)
                            {
                                shape_ok =
                                    (dl[17].words.w1 ==
                                         (u32)(uintptr_t)(base +
                                             NDS_NATIVE_ITEM_CAPSULE_VERTEX2_OFFSET));
                            }
                            else
                            {
                                shape_ok =
                                    (dl[9].words.w1 ==
                                         (u32)(uintptr_t)(base +
                                             NDS_NATIVE_ITEM_CAPSULE_VERTEX3_OFFSET));
                            }
                            if (shape_ok != FALSE)
                            {
                                capsule_step = 9u;
                                item_capsule_root = root;
                                gNdsItemCapsuleRoot = root;
                                item_capsule_native_candidate = TRUE;
                            }
                        }
                    }
                }
            }
            if (capsule_step > gNdsItemCapsuleCandidateStep)
            {
                gNdsItemCapsuleCandidateStep = capsule_step;
            }
            if ((sNdsRendererAdapterItemSubmitActive != FALSE) &&
                (item_capsule_native_candidate == FALSE))
            {
                NDS_DIAG(gNdsItemCapsuleSubmitFailCount++);
            }
        }
    }

    if ((loaded != NULL) &&
        (loaded->asset_id == NDS_NATIVE_ITEM_BOMBHEI_ASSET) &&
        (ndsRelocNativeRootOffset(loaded, dl) == NDS_NATIVE_ITEM_BOMBHEI_ROOT))
    {
        u32 bombhei_step = 1u;

        if (sNdsRendererAdapterItemSubmitActive != FALSE)
        {
            bombhei_step = 2u;
            if ((dobj->parent_gobj != NULL) &&
                (dobj->parent_gobj->id == nGCCommonKindItem))
            {
                ITStruct *ip = itGetStruct(dobj->parent_gobj);

                bombhei_step = 3u;
                if (ip != NULL)
                {
                    gNdsItemBombHeiKind = (u32)ip->kind;
                    if (ip->kind != nITKindBombHei)
                    {
                        NDS_DIAG(gNdsItemBombHeiForeignKindCount++);
                    }
                    else if ((dobj->mobj != NULL) &&
                             (dobj->mobj->next == NULL) &&
                             (loaded->data != NULL) &&
                             (loaded->data_size >= NDS_NATIVE_ITEM_BOMBHEI_FILE_END))
                    {
                        const u8 *base = (const u8 *)loaded->data;

                        bombhei_step = 4u;
                        if ((dl[11].words.w0 == NDS_NATIVE_ITEM_BOMBHEI_TLUT_W0) &&
                            (dl[11].words.w1 ==
                                 (u32)(uintptr_t)(base +
                                     NDS_NATIVE_ITEM_BOMBHEI_TLUT_OFFSET)) &&
                            (dl[17].words.w0 == NDS_NATIVE_ITEM_BOMBHEI_HOOK_W0) &&
                            (dl[17].words.w1 == NDS_NATIVE_ITEM_BOMBHEI_HOOK_W1) &&
                            (dl[21].words.w1 ==
                                 (u32)(uintptr_t)(base +
                                     NDS_NATIVE_ITEM_BOMBHEI_VERTEX_OFFSET)))
                        {
                            bombhei_step = 5u;
                            if (ndsRendererAdapterBuildNativeMaterialSnapshot(
                                    dobj->mobj, &item_bombhei_material, FALSE,
                                    NULL, NULL) != FALSE)
                            {
                                bombhei_step = 6u;
                                gNdsItemBombHeiEffectsSeen |=
                                    item_bombhei_material.effects;
                                if (item_bombhei_material.effects ==
                                        NDS_RENDERER_NATIVE_MATERIAL_CURRENT_IMAGE)
                                {
                                    bombhei_step = 9u;
                                    item_bombhei_native_candidate = TRUE;
                                }
                                else
                                {
                                    gNdsItemBombHeiEffectsRejected =
                                        item_bombhei_material.effects;
                                }
                            }
                            else
                            {
                                NDS_DIAG(gNdsItemBombHeiSnapshotFailCount++);
                            }
                        }
                    }
                }
            }
        }
        if (bombhei_step > gNdsItemBombHeiCandidateStep)
        {
            gNdsItemBombHeiCandidateStep = bombhei_step;
        }
    }

    if ((loaded != NULL) &&
        (loaded->asset_id == NDS_NATIVE_ITEM_LGUN_ASSET) &&
        (ndsRelocNativeRootOffset(loaded, dl) == NDS_NATIVE_ITEM_LGUN_ROOT))
    {
        u32 lgun_step = 1u;

        if (sNdsRendererAdapterItemSubmitActive != FALSE)
        {
            lgun_step = 2u;
            if ((dobj->parent_gobj != NULL) &&
                (dobj->parent_gobj->id == nGCCommonKindItem))
            {
                ITStruct *ip = itGetStruct(dobj->parent_gobj);

                lgun_step = 3u;
                if (ip != NULL)
                {
                    gNdsItemLGunKind = (u32)ip->kind;
                    if (ip->kind != nITKindLGun)
                    {
                        NDS_DIAG(gNdsItemLGunForeignKindCount++);
                    }
                    else if ((dobj->mobj == NULL) &&
                             (loaded->data != NULL) &&
                             (loaded->data_size >= NDS_NATIVE_ITEM_LGUN_FILE_END))
                    {
                        const u8 *base = (const u8 *)loaded->data;

                        lgun_step = 4u;
                        if ((dl[11].words.w0 == NDS_NATIVE_ITEM_LGUN_TLUT_W0) &&
                            (dl[11].words.w1 ==
                                 (u32)(uintptr_t)(base +
                                     NDS_NATIVE_ITEM_LGUN_TLUT_OFFSET)) &&
                            (dl[17].words.w1 ==
                                 (u32)(uintptr_t)(base +
                                     NDS_NATIVE_ITEM_LGUN_IMAGE0_OFFSET)) &&
                            (dl[22].words.w1 ==
                                 (u32)(uintptr_t)(base +
                                     NDS_NATIVE_ITEM_LGUN_VERTEX0_OFFSET)) &&
                            (dl[28].words.w1 ==
                                 (u32)(uintptr_t)(base +
                                     NDS_NATIVE_ITEM_LGUN_IMAGE1_OFFSET)) &&
                            (dl[32].words.w1 ==
                                 (u32)(uintptr_t)(base +
                                     NDS_NATIVE_ITEM_LGUN_VERTEX1_OFFSET)) &&
                            (dl[38].words.w1 ==
                                 (u32)(uintptr_t)(base +
                                     NDS_NATIVE_ITEM_LGUN_IMAGE2_OFFSET)) &&
                            (dl[43].words.w1 ==
                                 (u32)(uintptr_t)(base +
                                     NDS_NATIVE_ITEM_LGUN_VERTEX2_OFFSET)))
                        {
                            lgun_step = 9u;
                            item_lgun_native_candidate = TRUE;
                        }
                    }
                }
            }
        }
        if (lgun_step > gNdsItemLGunCandidateStep)
        {
            gNdsItemLGunCandidateStep = lgun_step;
        }
    }

    if ((loaded != NULL) &&
        (loaded->asset_id == NDS_NATIVE_ITEM_HARISEN_ASSET) &&
        (ndsRelocNativeRootOffset(loaded, dl) == NDS_NATIVE_ITEM_HARISEN_ROOT))
    {
        u32 harisen_step = 1u;

        if (sNdsRendererAdapterItemSubmitActive != FALSE)
        {
            harisen_step = 2u;
            if ((dobj->parent_gobj != NULL) &&
                (dobj->parent_gobj->id == nGCCommonKindItem))
            {
                ITStruct *ip = itGetStruct(dobj->parent_gobj);

                harisen_step = 3u;
                if (ip != NULL)
                {
                    gNdsItemHarisenKind = (u32)ip->kind;
                    if (ip->kind != nITKindHarisen)
                    {
                        NDS_DIAG(gNdsItemHarisenForeignKindCount++);
                    }
                    else if ((dobj->mobj == NULL) &&
                             (loaded->data != NULL) &&
                             (loaded->data_size >= NDS_NATIVE_ITEM_HARISEN_FILE_END))
                    {
                        const u8 *base = (const u8 *)loaded->data;

                        harisen_step = 4u;
                        if ((dl[11].words.w0 == NDS_NATIVE_ITEM_HARISEN_TLUT_W0) &&
                            (dl[11].words.w1 ==
                                 (u32)(uintptr_t)(base +
                                     NDS_NATIVE_ITEM_HARISEN_TLUT_OFFSET)) &&
                            (dl[17].words.w0 == NDS_NATIVE_ITEM_HARISEN_IMAGE_W0) &&
                            (dl[17].words.w1 ==
                                 (u32)(uintptr_t)(base +
                                     NDS_NATIVE_ITEM_HARISEN_IMAGE_OFFSET)) &&
                            (dl[22].words.w1 ==
                                 (u32)(uintptr_t)(base +
                                     NDS_NATIVE_ITEM_HARISEN_VERTEX_OFFSET)))
                        {
                            harisen_step = 9u;
                            item_harisen_native_candidate = TRUE;
                        }
                    }
                }
            }
        }
        if (harisen_step > gNdsItemHarisenCandidateStep)
        {
            gNdsItemHarisenCandidateStep = harisen_step;
        }
    }

    if ((loaded != NULL) &&
        (loaded->asset_id == NDS_NATIVE_ITEM_HEART_ASSET) &&
        (ndsRelocNativeRootOffset(loaded, dl) == NDS_NATIVE_ITEM_HEART_ROOT))
    {
        u32 heart_step = 1u;

        if (sNdsRendererAdapterItemSubmitActive != FALSE)
        {
            heart_step = 2u;
            if ((dobj->parent_gobj != NULL) &&
                (dobj->parent_gobj->id == nGCCommonKindItem))
            {
                ITStruct *ip = itGetStruct(dobj->parent_gobj);

                heart_step = 3u;
                if (ip != NULL)
                {
                    gNdsItemHeartKind = (u32)ip->kind;
                    if (ip->kind != nITKindHeart)
                    {
                        NDS_DIAG(gNdsItemHeartForeignKindCount++);
                    }
                    else if ((dobj->mobj == NULL) &&
                             (loaded->data != NULL) &&
                             (loaded->data_size >= NDS_NATIVE_ITEM_HEART_FILE_END))
                    {
                        const u8 *base = (const u8 *)loaded->data;

                        heart_step = 4u;
                        if ((dl[11].words.w0 == NDS_NATIVE_ITEM_HEART_TLUT_W0) &&
                            (dl[11].words.w1 ==
                                 (u32)(uintptr_t)(base +
                                     NDS_NATIVE_ITEM_HEART_TLUT_OFFSET)) &&
                            (dl[17].words.w0 == NDS_NATIVE_ITEM_HEART_IMAGE0_W0) &&
                            (dl[17].words.w1 ==
                                 (u32)(uintptr_t)(base +
                                     NDS_NATIVE_ITEM_HEART_IMAGE0_OFFSET)) &&
                            (dl[21].words.w1 ==
                                 (u32)(uintptr_t)(base +
                                     NDS_NATIVE_ITEM_HEART_VERTEX0_OFFSET)) &&
                            (dl[32].words.w0 == NDS_NATIVE_ITEM_HEART_IMAGE1_W0) &&
                            (dl[32].words.w1 ==
                                 (u32)(uintptr_t)(base +
                                     NDS_NATIVE_ITEM_HEART_IMAGE1_OFFSET)) &&
                            (dl[36].words.w1 ==
                                 (u32)(uintptr_t)(base +
                                     NDS_NATIVE_ITEM_HEART_VERTEX1_OFFSET)))
                        {
                            heart_step = 9u;
                            item_heart_native_candidate = TRUE;
                        }
                    }
                }
            }
        }
        if (heart_step > gNdsItemHeartCandidateStep)
        {
            gNdsItemHeartCandidateStep = heart_step;
        }
    }

    {
        u32 root = (loaded != NULL) ? ndsRelocNativeRootOffset(loaded, dl) : 0u;

        if ((loaded != NULL) &&
            (loaded->asset_id == NDS_NATIVE_ITEM_STARROD_ASSET) &&
            ((root == NDS_NATIVE_ITEM_STARROD_HEADER_ROOT) ||
             (root == NDS_NATIVE_ITEM_STARROD_SECOND_ROOT)))
        {
            u32 starrod_step = 1u;

            if (sNdsRendererAdapterItemSubmitActive != FALSE)
            {
                starrod_step = 2u;
                if ((dobj->parent_gobj != NULL) &&
                    (dobj->parent_gobj->id == nGCCommonKindItem))
                {
                    ITStruct *ip = itGetStruct(dobj->parent_gobj);

                    starrod_step = 3u;
                    if (ip != NULL)
                    {
                        gNdsItemStarRodKind = (u32)ip->kind;
                        if (ip->kind != nITKindStarRod)
                        {
                            NDS_DIAG(gNdsItemStarRodForeignKindCount++);
                        }
                        else if ((dobj->mobj == NULL) &&
                                 (loaded->data != NULL) &&
                                 (loaded->data_size >= NDS_NATIVE_ITEM_STARROD_FILE_END))
                        {
                            const u8 *base = (const u8 *)loaded->data;
                            sb32 shape_ok;

                            starrod_step = 4u;
                            if (root == NDS_NATIVE_ITEM_STARROD_HEADER_ROOT)
                            {
                                shape_ok =
                                    (dl[11].words.w0 ==
                                         NDS_NATIVE_ITEM_STARROD_BRANCH_W0) &&
                                    (dl[11].words.w1 ==
                                         (u32)(uintptr_t)(base +
                                             NDS_NATIVE_ITEM_STARROD_CALLEE_ROOT));
                            }
                            else
                            {
                                shape_ok =
                                    (dl[5].words.w1 ==
                                         (u32)(uintptr_t)(base +
                                             NDS_NATIVE_ITEM_STARROD_TLUT1_OFFSET)) &&
                                    (dl[10].words.w1 ==
                                         (u32)(uintptr_t)(base +
                                             NDS_NATIVE_ITEM_STARROD_IMAGE1_OFFSET)) &&
                                    (dl[15].words.w1 ==
                                         (u32)(uintptr_t)(base +
                                             NDS_NATIVE_ITEM_STARROD_VERTEX1_OFFSET));
                            }
                            if (shape_ok != FALSE)
                            {
                                starrod_step = 9u;
                                item_starrod_root = root;
                                gNdsItemStarRodRoot = root;
                                item_starrod_native_candidate = TRUE;
                            }
                        }
                    }
                }
            }
            if (starrod_step > gNdsItemStarRodCandidateStep)
            {
                gNdsItemStarRodCandidateStep = starrod_step;
            }
        }
    }

    {
        u32 root = (loaded != NULL) ? ndsRelocNativeRootOffset(loaded, dl) : 0u;

        if ((loaded != NULL) &&
            (loaded->asset_id == NDS_NATIVE_ITEM_FFLOWER_ASSET) &&
            ((root == NDS_NATIVE_ITEM_FFLOWER_BRANCH_ROOT) ||
             (root == NDS_NATIVE_ITEM_FFLOWER_LIVE_ROOT)))
        {
            u32 fflower_step = 1u;

            if (sNdsRendererAdapterItemSubmitActive != FALSE)
            {
                fflower_step = 2u;
                if ((dobj->parent_gobj != NULL) &&
                    (dobj->parent_gobj->id == nGCCommonKindItem))
                {
                    ITStruct *ip = itGetStruct(dobj->parent_gobj);

                    fflower_step = 3u;
                    if (ip != NULL)
                    {
                        gNdsItemFFlowerKind = (u32)ip->kind;
                        if (ip->kind != nITKindFFlower)
                        {
                            NDS_DIAG(gNdsItemFFlowerForeignKindCount++);
                        }
                        else if ((loaded->data != NULL) &&
                                 (loaded->data_size >= NDS_NATIVE_ITEM_FFLOWER_FILE_END))
                        {
                            const u8 *base = (const u8 *)loaded->data;

                            fflower_step = 4u;
                            if ((root == NDS_NATIVE_ITEM_FFLOWER_BRANCH_ROOT) &&
                                (dobj->mobj == NULL) &&
                                (dl[6].words.w1 ==
                                     (u32)(uintptr_t)(base +
                                         NDS_NATIVE_ITEM_FFLOWER_BRANCH_TLUT_OFFSET)) &&
                                (dl[9].words.w0 ==
                                     NDS_NATIVE_ITEM_FFLOWER_BRANCH_W0) &&
                                (dl[9].words.w1 ==
                                     (u32)(uintptr_t)(base +
                                         NDS_NATIVE_ITEM_FFLOWER_CALLEE_ROOT)))
                            {
                                fflower_step = 9u;
                                item_fflower_root = root;
                                gNdsItemFFlowerRoot = root;
                                item_fflower_native_candidate = TRUE;
                            }
                            else if ((root == NDS_NATIVE_ITEM_FFLOWER_LIVE_ROOT) &&
                                     (dobj->mobj != NULL) &&
                                     (dobj->mobj->next == NULL) &&
                                     (ndsRendererAdapterMaterialFlags(dobj->mobj) ==
                                          NDS_NATIVE_ITEM_FFLOWER_MOBJ_FLAGS) &&
                                     (dl[3].words.w0 == NDS_NATIVE_ITEM_FFLOWER_HOOK_W0) &&
                                     (dl[3].words.w1 == NDS_NATIVE_ITEM_FFLOWER_HOOK_W1) &&
                                     (dl[8].words.w1 ==
                                          (u32)(uintptr_t)(base +
                                              NDS_NATIVE_ITEM_FFLOWER_LIVE_IMAGE_OFFSET)) &&
                                     (dl[12].words.w1 ==
                                          (u32)(uintptr_t)(base +
                                              NDS_NATIVE_ITEM_FFLOWER_LIVE_VERTEX_OFFSET)))
                            {
                                fflower_step = 5u;
                                if (ndsRendererAdapterBuildNativeMaterialSnapshot(
                                        dobj->mobj, &item_fflower_material, FALSE,
                                        NULL, NULL) != FALSE)
                                {
                                    fflower_step = 6u;
                                    gNdsItemFFlowerEffectsSeen |=
                                        item_fflower_material.effects;
                                    if (item_fflower_material.effects ==
                                            NDS_RENDERER_NATIVE_MATERIAL_PALETTE_IMAGE)
                                    {
                                        fflower_step = 9u;
                                        item_fflower_root = root;
                                        gNdsItemFFlowerRoot = root;
                                        item_fflower_native_candidate = TRUE;
                                    }
                                    else
                                    {
                                        gNdsItemFFlowerEffectsRejected =
                                            item_fflower_material.effects;
                                    }
                                }
                                else
                                {
                                    NDS_DIAG(gNdsItemFFlowerSnapshotFailCount++);
                                }
                            }
                        }
                    }
                }
            }
            if (fflower_step > gNdsItemFFlowerCandidateStep)
            {
                gNdsItemFFlowerCandidateStep = fflower_step;
            }
        }
    }

    {
        u32 root = (loaded != NULL) ? ndsRelocNativeRootOffset(loaded, dl) : 0u;

        if ((loaded != NULL) &&
            (loaded->asset_id == NDS_NATIVE_ITEM_MSBOMB_ASSET) &&
            ((root == NDS_NATIVE_ITEM_MSBOMB_ROOT0) ||
             (root == NDS_NATIVE_ITEM_MSBOMB_ROOT1)))
        {
            u32 msbomb_step = 1u;

            if (sNdsRendererAdapterItemSubmitActive != FALSE)
            {
                msbomb_step = 2u;
                if ((dobj->parent_gobj != NULL) &&
                    (dobj->parent_gobj->id == nGCCommonKindItem))
                {
                    ITStruct *ip = itGetStruct(dobj->parent_gobj);

                    msbomb_step = 3u;
                    if (ip != NULL)
                    {
                        gNdsItemMSBombKind = (u32)ip->kind;
                        if (ip->kind != nITKindMSBomb)
                        {
                            NDS_DIAG(gNdsItemMSBombForeignKindCount++);
                        }
                        else if ((dobj->mobj == NULL) &&
                                 (loaded->data != NULL) &&
                                 (loaded->data_size >= NDS_NATIVE_ITEM_MSBOMB_FILE_END))
                        {
                            const u8 *base = (const u8 *)loaded->data;
                            sb32 shape_ok;

                            msbomb_step = 4u;
                            if (root == NDS_NATIVE_ITEM_MSBOMB_ROOT0)
                            {
                                shape_ok =
                                    (dl[14].words.w1 ==
                                         (u32)(uintptr_t)(base +
                                             NDS_NATIVE_ITEM_MSBOMB_TLUT0_OFFSET)) &&
                                    (dl[20].words.w1 ==
                                         (u32)(uintptr_t)(base +
                                             NDS_NATIVE_ITEM_MSBOMB_IMAGE0_OFFSET)) &&
                                    (dl[25].words.w1 ==
                                         (u32)(uintptr_t)(base +
                                             NDS_NATIVE_ITEM_MSBOMB_VERTEX0_OFFSET));
                            }
                            else
                            {
                                shape_ok =
                                    (dl[10].words.w1 ==
                                         (u32)(uintptr_t)(base +
                                             NDS_NATIVE_ITEM_MSBOMB_TLUT1_OFFSET)) &&
                                    (dl[16].words.w1 ==
                                         (u32)(uintptr_t)(base +
                                             NDS_NATIVE_ITEM_MSBOMB_IMAGE1_OFFSET)) &&
                                    (dl[21].words.w1 ==
                                         (u32)(uintptr_t)(base +
                                             NDS_NATIVE_ITEM_MSBOMB_VERTEX1_OFFSET));
                            }
                            if (shape_ok != FALSE)
                            {
                                msbomb_step = 9u;
                                item_msbomb_root = root;
                                gNdsItemMSBombRoot = root;
                                item_msbomb_native_candidate = TRUE;
                            }
                        }
                    }
                }
            }
            if (msbomb_step > gNdsItemMSBombCandidateStep)
            {
                gNdsItemMSBombCandidateStep = msbomb_step;
            }
        }
    }

    if ((loaded != NULL) &&
        (loaded->asset_id == NDS_NATIVE_ITEM_NBUMPER_ASSET) &&
        (ndsRelocNativeRootOffset(loaded, dl) == NDS_NATIVE_ITEM_NBUMPER_ROOT))
    {
        u32 nbumper_step = 1u;

        if (sNdsRendererAdapterItemSubmitActive != FALSE)
        {
            nbumper_step = 2u;
            if ((dobj->parent_gobj != NULL) &&
                (dobj->parent_gobj->id == nGCCommonKindItem))
            {
                ITStruct *ip = itGetStruct(dobj->parent_gobj);

                nbumper_step = 3u;
                if (ip != NULL)
                {
                    gNdsItemNBumperKind = (u32)ip->kind;
                    if (ip->kind != nITKindNBumper)
                    {
                        NDS_DIAG(gNdsItemNBumperForeignKindCount++);
                    }
                    else if ((dobj->mobj != NULL) &&
                             (dobj->mobj->next == NULL) &&
                             (ndsRendererAdapterMaterialFlags(dobj->mobj) ==
                                  NDS_NATIVE_ITEM_NBUMPER_MOBJ_FLAGS) &&
                             (loaded->data != NULL) &&
                             (loaded->data_size >= NDS_NATIVE_ITEM_NBUMPER_FILE_END))
                    {
                        const u8 *base = (const u8 *)loaded->data;

                        nbumper_step = 4u;
                        if ((dl[10].words.w0 == NDS_NATIVE_ITEM_NBUMPER_HOOK_W0) &&
                            (dl[10].words.w1 == NDS_NATIVE_ITEM_NBUMPER_HOOK_W1) &&
                            (dl[16].words.w1 ==
                                 (u32)(uintptr_t)(base +
                                     NDS_NATIVE_ITEM_NBUMPER_IMAGE_OFFSET)) &&
                            (dl[21].words.w1 ==
                                 (u32)(uintptr_t)(base +
                                     NDS_NATIVE_ITEM_NBUMPER_VERTEX_OFFSET)))
                        {
                            nbumper_step = 5u;
                            if (ndsRendererAdapterBuildNativeMaterialSnapshot(
                                    dobj->mobj, &item_nbumper_material, FALSE,
                                    NULL, NULL) != FALSE)
                            {
                                nbumper_step = 6u;
                                gNdsItemNBumperEffectsSeen |=
                                    item_nbumper_material.effects;
                                if (item_nbumper_material.effects ==
                                        NDS_RENDERER_NATIVE_MATERIAL_PALETTE_IMAGE)
                                {
                                    nbumper_step = 9u;
                                    item_nbumper_native_candidate = TRUE;
                                }
                                else
                                {
                                    gNdsItemNBumperEffectsRejected =
                                        item_nbumper_material.effects;
                                }
                            }
                            else
                            {
                                NDS_DIAG(gNdsItemNBumperSnapshotFailCount++);
                            }
                        }
                    }
                }
            }
        }
        if (nbumper_step > gNdsItemNBumperCandidateStep)
        {
            gNdsItemNBumperCandidateStep = nbumper_step;
        }
    }

    if ((loaded != NULL) &&
        (loaded->asset_id == NDS_NATIVE_ITEM_BOX_ASSET) &&
        (ndsRelocNativeRootOffset(loaded, dl) == NDS_NATIVE_ITEM_BOX_ROOT))
    {
        u32 box_step = 1u;

        if (sNdsRendererAdapterItemSubmitActive != FALSE)
        {
            box_step = 2u;
            if ((dobj->parent_gobj != NULL) &&
                (dobj->parent_gobj->id == nGCCommonKindItem))
            {
                ITStruct *ip = itGetStruct(dobj->parent_gobj);

                box_step = 3u;
                if (ip != NULL)
                {
                    gNdsItemBoxKind = (u32)ip->kind;
                    if (ip->kind != nITKindBox)
                    {
                        NDS_DIAG(gNdsItemBoxForeignKindCount++);
                    }
                    else if ((dobj->mobj == NULL) &&
                             (loaded->data != NULL) &&
                             (loaded->data_size >= NDS_NATIVE_ITEM_BOX_FILE_END))
                    {
                        const u8 *base = (const u8 *)loaded->data;

                        box_step = 4u;
                        if ((dl[8].words.w0 == NDS_NATIVE_ITEM_BOX_TLUT0_W0) &&
                            (dl[8].words.w1 == (u32)(uintptr_t)(base + NDS_NATIVE_ITEM_BOX_TLUT0_OFFSET)) &&
                            (dl[14].words.w0 == NDS_NATIVE_ITEM_BOX_IMAGE0_W0) &&
                            (dl[14].words.w1 == (u32)(uintptr_t)(base + NDS_NATIVE_ITEM_BOX_IMAGE0_OFFSET)) &&
                            (dl[18].words.w1 == (u32)(uintptr_t)(base + NDS_NATIVE_ITEM_BOX_VERTEX0_OFFSET)) &&
                            (dl[24].words.w0 == NDS_NATIVE_ITEM_BOX_TLUT1_W0) &&
                            (dl[24].words.w1 == (u32)(uintptr_t)(base + NDS_NATIVE_ITEM_BOX_TLUT1_OFFSET)) &&
                            (dl[28].words.w0 == NDS_NATIVE_ITEM_BOX_IMAGE1_W0) &&
                            (dl[28].words.w1 == (u32)(uintptr_t)(base + NDS_NATIVE_ITEM_BOX_IMAGE1_OFFSET)) &&
                            (dl[32].words.w1 == (u32)(uintptr_t)(base + NDS_NATIVE_ITEM_BOX_VERTEX1A_OFFSET)) &&
                            (dl[33].words.w1 == (u32)(uintptr_t)(base + NDS_NATIVE_ITEM_BOX_VERTEX1B_OFFSET)))
                        {
                            box_step = 9u;
                            item_box_native_candidate = TRUE;
                        }
                    }
                }
            }
        }
        if (box_step > gNdsItemBoxCandidateStep)
        {
            gNdsItemBoxCandidateStep = box_step;
        }
    }

    if ((loaded != NULL) &&
        (loaded->asset_id == NDS_NATIVE_ITEM_TARU_ASSET) &&
        (ndsRelocNativeRootOffset(loaded, dl) == NDS_NATIVE_ITEM_TARU_ROOT))
    {
        u32 taru_step = 1u;

        if (sNdsRendererAdapterItemSubmitActive != FALSE)
        {
            taru_step = 2u;
            if ((dobj->parent_gobj != NULL) &&
                (dobj->parent_gobj->id == nGCCommonKindItem))
            {
                ITStruct *ip = itGetStruct(dobj->parent_gobj);

                taru_step = 3u;
                if (ip != NULL)
                {
                    gNdsItemTaruKind = (u32)ip->kind;
                    if (ip->kind != nITKindTaru)
                    {
                        NDS_DIAG(gNdsItemTaruForeignKindCount++);
                    }
                    else if ((dobj->mobj == NULL) &&
                             (loaded->data != NULL) &&
                             (loaded->data_size >= NDS_NATIVE_ITEM_TARU_FILE_END))
                    {
                        const u8 *base = (const u8 *)loaded->data;

                        taru_step = 4u;
                        if ((dl[11].words.w0 == NDS_NATIVE_ITEM_TARU_TLUT0_W0) &&
                            (dl[11].words.w1 == (u32)(uintptr_t)(base + NDS_NATIVE_ITEM_TARU_TLUT0_OFFSET)) &&
                            (dl[17].words.w0 == NDS_NATIVE_ITEM_TARU_IMAGE0_W0) &&
                            (dl[17].words.w1 == (u32)(uintptr_t)(base + NDS_NATIVE_ITEM_TARU_IMAGE0_OFFSET)) &&
                            (dl[21].words.w1 == (u32)(uintptr_t)(base + NDS_NATIVE_ITEM_TARU_VERTEX0_OFFSET)) &&
                            (dl[29].words.w0 == NDS_NATIVE_ITEM_TARU_TLUT1_W0) &&
                            (dl[29].words.w1 == (u32)(uintptr_t)(base + NDS_NATIVE_ITEM_TARU_TLUT1_OFFSET)) &&
                            (dl[34].words.w0 == NDS_NATIVE_ITEM_TARU_IMAGE1_W0) &&
                            (dl[34].words.w1 == (u32)(uintptr_t)(base + NDS_NATIVE_ITEM_TARU_IMAGE1_OFFSET)) &&
                            (dl[38].words.w1 == (u32)(uintptr_t)(base + NDS_NATIVE_ITEM_TARU_VERTEX1_OFFSET)))
                        {
                            taru_step = 9u;
                            item_taru_native_candidate = TRUE;
                        }
                    }
                }
            }
        }
        if (taru_step > gNdsItemTaruCandidateStep)
        {
            gNdsItemTaruCandidateStep = taru_step;
        }
    }

    if ((loaded != NULL) &&
        (loaded->asset_id == NDS_NATIVE_ITEM_EGG_ASSET) &&
        (ndsRelocNativeRootOffset(loaded, dl) == NDS_NATIVE_ITEM_EGG_ROOT))
    {
        u32 egg_step = 1u;

        if (sNdsRendererAdapterItemSubmitActive != FALSE)
        {
            egg_step = 2u;
            if ((dobj->parent_gobj != NULL) &&
                (dobj->parent_gobj->id == nGCCommonKindItem))
            {
                ITStruct *ip = itGetStruct(dobj->parent_gobj);

                egg_step = 3u;
                if (ip != NULL)
                {
                    gNdsItemEggKind = (u32)ip->kind;
                    if (ip->kind != nITKindEgg)
                    {
                        NDS_DIAG(gNdsItemEggForeignKindCount++);
                    }
                    else if ((dobj->mobj == NULL) &&
                             (loaded->data != NULL) &&
                             (loaded->data_size >= NDS_NATIVE_ITEM_EGG_FILE_END))
                    {
                        const u8 *base = (const u8 *)loaded->data;

                        egg_step = 4u;
                        if ((dl[10].words.w0 == NDS_NATIVE_ITEM_EGG_TLUT_W0) &&
                            (dl[10].words.w1 == (u32)(uintptr_t)(base + NDS_NATIVE_ITEM_EGG_TLUT_OFFSET)) &&
                            (dl[16].words.w0 == NDS_NATIVE_ITEM_EGG_IMAGE_W0) &&
                            (dl[16].words.w1 == (u32)(uintptr_t)(base + NDS_NATIVE_ITEM_EGG_IMAGE_OFFSET)) &&
                            (dl[21].words.w1 == (u32)(uintptr_t)(base + NDS_NATIVE_ITEM_EGG_VERTEX_OFFSET)))
                        {
                            egg_step = 9u;
                            item_egg_native_candidate = TRUE;
                        }
                    }
                }
            }
        }
        if (egg_step > gNdsItemEggCandidateStep)
        {
            gNdsItemEggCandidateStep = egg_step;
        }
    }

    if ((loaded != NULL) &&
        (loaded->asset_id == NDS_NATIVE_ITEM_IWARK_ASSET) &&
        (ndsRelocNativeRootOffset(loaded, dl) == NDS_NATIVE_ITEM_IWARK_ROOT))
    {
        u32 iwark_step = 1u;

        if (sNdsRendererAdapterItemSubmitActive != FALSE)
        {
            iwark_step = 2u;
            if ((dobj->parent_gobj != NULL) &&
                (dobj->parent_gobj->id == nGCCommonKindItem))
            {
                ITStruct *ip = itGetStruct(dobj->parent_gobj);

                iwark_step = 3u;
                if (ip != NULL)
                {
                    gNdsItemIwarkKind = (u32)ip->kind;
                    if (ip->kind != nITKindIwark)
                    {
                        NDS_DIAG(gNdsItemIwarkForeignKindCount++);
                    }
                    else if ((dobj->mobj == NULL) &&
                             (loaded->data != NULL) &&
                             (loaded->data_size >= NDS_NATIVE_ITEM_IWARK_FILE_END))
                    {
                        const u8 *base = (const u8 *)loaded->data;

                        iwark_step = 4u;
                        if ((dl[10].words.w0 == NDS_NATIVE_ITEM_IWARK_TLUT_W0) &&
                            (dl[10].words.w1 == (u32)(uintptr_t)(base + NDS_NATIVE_ITEM_IWARK_TLUT_OFFSET)) &&
                            (dl[16].words.w0 == NDS_NATIVE_ITEM_IWARK_IMAGE_W0) &&
                            (dl[16].words.w1 == (u32)(uintptr_t)(base + NDS_NATIVE_ITEM_IWARK_IMAGE_OFFSET)) &&
                            (dl[21].words.w1 == (u32)(uintptr_t)(base + NDS_NATIVE_ITEM_IWARK_VERTEX_OFFSET)))
                        {
                            iwark_step = 9u;
                            item_iwark_native_candidate = TRUE;
                        }
                    }
                }
            }
        }
        if (iwark_step > gNdsItemIwarkCandidateStep)
        {
            gNdsItemIwarkCandidateStep = iwark_step;
        }
    }

    /* The Maxim Tomato, file 86 root 0x09c0.  Thirty words, four vertices, two
     * triangles, NO 0xDE opcode anywhere in the list and `p_mobjsubs` NULL --
     * so it owns its whole material and every word of it bakes.  The combiner
     * is TEXEL0 x SHADE in both cycles, with no PRIM or ENV, so the item
     * layer's seeded colours cannot reach this draw either.
     *
     * A whole-image sweep of all 2,132 O2R files finds 69 pointers into asset
     * 86 -- 68 of them file 251's own item attribute slots and one foreign
     * reference from YoshiMain into a different offset entirely -- so asset and
     * root discriminate this owner completely.  The kind is still read and
     * compared, for the same reason the Marumine's is: if a foreign kind ever
     * reaches this root, say WHY rather than leave it indistinguishable from a
     * submit refusal. */
    if ((loaded != NULL) &&
        (loaded->asset_id == NDS_NATIVE_ITEM_TOMATO_ASSET) &&
        (ndsRelocNativeRootOffset(loaded, dl) == NDS_NATIVE_ITEM_TOMATO_ROOT))
    {
        u32 tomato_step = 1u;

        if (sNdsRendererAdapterItemSubmitActive != FALSE)
        {
            tomato_step = 2u;
            if ((dobj->parent_gobj != NULL) &&
                (dobj->parent_gobj->id == nGCCommonKindItem))
            {
                ITStruct *tomato_ip = itGetStruct(dobj->parent_gobj);

                tomato_step = 3u;
                if (tomato_ip != NULL)
                {
                    gNdsItemTomatoKind = (u32)tomato_ip->kind;
                    if (tomato_ip->kind != nITKindTomato)
                    {
                        NDS_DIAG(gNdsItemTomatoForeignKindCount++);
                    }
                    else
                    {
                        tomato_step = 4u;
                        /* material 0 is the NULL MObj, NOT "untextured". */
                        if (dobj->mobj == NULL)
                        {
                            tomato_step = 5u;
                            if ((loaded->data != NULL) &&
                                (loaded->data_size >=
                                     NDS_NATIVE_ITEM_TOMATO_FILE_END) &&
                                (loaded->data_size >=
                                     (NDS_NATIVE_ITEM_TOMATO_ROOT +
                                      NDS_NATIVE_ITEM_TOMATO_DL_BYTES)))
                            {
                                const u8 *tomato_base =
                                    (const u8 *)loaded->data;

                                tomato_step = 6u;
                                if ((dl[11].words.w0 ==
                                         NDS_NATIVE_ITEM_TOMATO_TLUT_W0) &&
                                    (dl[17].words.w0 ==
                                         NDS_NATIVE_ITEM_TOMATO_IMAGE_W0))
                                {
                                    tomato_step = 7u;
                                    /* COMPARE the relocated pointers against
                                     * this file's own base -- never assume the
                                     * loader's fixup pass ran, and never bind a
                                     * chain word as an image. */
                                    if ((dl[11].words.w1 ==
                                             (u32)(uintptr_t)(tomato_base +
                                                 NDS_NATIVE_ITEM_TOMATO_TLUT_OFFSET)) &&
                                        (dl[17].words.w1 ==
                                             (u32)(uintptr_t)(tomato_base +
                                                 NDS_NATIVE_ITEM_TOMATO_IMAGE_OFFSET)))
                                    {
                                        tomato_step = 8u;
                                        if (dl[21].words.w1 ==
                                                (u32)(uintptr_t)(tomato_base +
                                                    NDS_NATIVE_ITEM_TOMATO_VERTEX_OFFSET))
                                        {
                                            tomato_step = 9u;
                                            item_tomato_native_candidate = TRUE;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        if (tomato_step > gNdsItemTomatoCandidateStep)
        {
            gNdsItemTomatoCandidateStep = tomato_step;
        }
    }
    /* The procedural visual templates. Claimed here, before the loaded-file
     * scan, because the owner needs nothing from `loaded`, from the material
     * segment or from the callback context -- and because the template GObj
     * carries exactly one DObj and exactly one list, so the per-GObj latch
     * cannot mis-attribute across nodes. dobj->child == NULL is an assertion,
     * not an assumption: this owner is baked for a single flat fan or ring and
     * must decline loudly rather than draw a shape it was not baked for. */
    if ((sNdsRendererAdapterVisualEffectNativeActive != FALSE) &&
        (dobj->dl == dl) && (dobj->child == NULL))
    {
        visual_effect_native_candidate = TRUE;
        visual_effect_template = sNdsRendererAdapterVisualEffectTemplate;
    }
    if ((sNdsRendererAdapterRebirthHaloNativeActive != FALSE) &&
        (gEFManagerFiles[2] != NULL) &&
        ((const u8 *)dl >= (const u8 *)gEFManagerFiles[2]))
    {
        uintptr_t offset = (uintptr_t)((const u8 *)dl -
                                       (const u8 *)gEFManagerFiles[2]);

        if ((offset == 0x2378u) || (offset == 0x2a88u) ||
            (offset == 0x27e8u))
        {
            rebirth_halo_root_offset = (u32)offset;
            rebirth_halo_native_candidate = TRUE;
        }
    }
