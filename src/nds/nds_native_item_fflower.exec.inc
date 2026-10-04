/* Fire Flower executor. TEXTUAL INCLUDE ONLY.
 * File 86 root 0x4520 branches at word 9 to relocated 0x4578. Root 0x4608
 * has a word-3 segment-0xE palette hook driven by its one live MObj. */
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_ITEM_CORE

#if defined(NDS_LAB_FOURCPU_SWEEP) && NDS_LAB_FOURCPU_SWEEP
/* LAB ONLY: the render state each Fire Flower root meets at its emit (the
 * first eight draws): root, texture-state flags, combine count and words,
 * active tile and its descriptor, TLUT image/count, texture image, othermode,
 * geometry mode, the modelview's diagonal and translation row, the
 * projection's diagonal, the native material's palette, then both whole
 * matrices (row-major, words 40-55 modelview, 56-71 projection). Flushed so a gdb
 * read is not cache residue. */
volatile u32 gNdsLabFFlowerState[8][72] __attribute__((used, aligned(32)));
volatile u32 gNdsLabFFlowerStateCount __attribute__((used, aligned(32)));

static void ndsLabFFlowerRecordState(u32 root, const NDSRendererStats *stats,
                                     const NDSRendererConfig *config,
                                     const NDSRendererNativeMaterial *material)
{
    const u32 n = gNdsLabFFlowerStateCount;
    const u32 active = ndsRendererActiveTextureTile(stats);
    const NDSRendererTileState *t = &stats->texture_tiles[active];
    volatile u32 *w;
    u32 i;

    if (n >= 8u)
    {
        return;
    }
    w = gNdsLabFFlowerState[n];
    w[0] = root;
    w[1] = stats->texture_state_flags;
    w[2] = stats->texture_combine_count;
    w[3] = stats->texture_combine_w0;
    w[4] = stats->texture_combine_w1;
    w[5] = active;
    w[6] = t->format;
    w[7] = t->size;
    w[8] = t->line;
    w[9] = t->tmem;
    w[10] = t->palette;
    w[11] = t->cms | (t->cmt << 8) | (t->masks << 16) | (t->maskt << 24);
    w[12] = t->width | (t->height << 16);
    w[13] = t->uls | (t->ult << 16);
    w[14] = stats->texture_tlut_image;
    w[15] = stats->texture_tlut_count;
    w[16] = stats->texture_image;
    w[17] = stats->othermode_h;
    w[18] = stats->othermode_l;
    w[19] = stats->geometry_mode;
    for (i = 0u; i < 4u; i++)
    {
        w[20u + i] = (u32)config->initial_modelview->m[i][i];
        w[24u + i] = (u32)config->initial_modelview->m[3][i];
        w[28u + i] = (u32)config->initial_projection->m[i][i];
    }
    w[32] = (u32)config->initial_projection->m[3][2];
    w[33] = (u32)config->initial_projection->m[2][3];
    w[34] = (material != NULL) ? material->effects : 0xffffffffu;
    w[35] = (material != NULL) ? material->palette_image : 0u;
    w[36] = stats->prim_color;
    w[37] = stats->env_color;
    w[38] = gNdsBattlePlayablePacingPresentedFrames;
    w[39] = 0x46464c57u;
    for (i = 0u; i < 16u; i++)
    {
        w[40u + i] = (u32)config->initial_modelview->m[i >> 2][i & 3u];
        w[56u + i] = (u32)config->initial_projection->m[i >> 2][i & 3u];
    }
    DC_FlushRange((const void *)w, 72u * sizeof(u32));
    gNdsLabFFlowerStateCount = n + 1u;
    DC_FlushRange((const void *)&gNdsLabFFlowerStateCount, 32u);
}
#endif

sb32 ndsRendererSubmitNativeItemFFlower(
    u32 root_offset, const void *file_base_ptr, u32 file_bytes,
    const NDSRendererNativeMaterial *material,
    const NDSRendererConfig *config, NDSRendererStats *stats)
{
    const u8 *base = (const u8 *)file_base_ptr;
    NDSRendererTraversalState state;
    u32 palette_ok = FALSE;
    u32 i;

    if ((base == NULL) || (config == NULL) || (stats == NULL))
    {
        gNdsItemFFlowerSubmitStep = 11u;
        return FALSE;
    }
    if ((config->initial_projection == NULL) ||
        (config->initial_modelview == NULL))
    {
        gNdsItemFFlowerSubmitStep = 12u;
        return FALSE;
    }
    if ((file_bytes < NDS_NATIVE_ITEM_FFLOWER_FILE_END) ||
        ((root_offset != NDS_NATIVE_ITEM_FFLOWER_BRANCH_ROOT) &&
         (root_offset != NDS_NATIVE_ITEM_FFLOWER_LIVE_ROOT)))
    {
        gNdsItemFFlowerSubmitStep = 2u;
        return FALSE;
    }
    if (root_offset == NDS_NATIVE_ITEM_FFLOWER_LIVE_ROOT)
    {
        if (material == NULL)
        {
            gNdsItemFFlowerSubmitStep = 13u;
            return FALSE;
        }
        if (material->effects != NDS_RENDERER_NATIVE_MATERIAL_PALETTE_IMAGE)
        {
            gNdsItemFFlowerEffectsRejected = material->effects;
            gNdsItemFFlowerSubmitStep = 14u;
            return FALSE;
        }
        for (i = 0u; i < NDS_NATIVE_ITEM_FFLOWER_PALETTE_COUNT; i++)
        {
            u32 expected = (u32)(uintptr_t)(
                base + sNdsNativeItemFFlowerPaletteOffsets[i]);
            if (material->palette_image == expected)
            {
                palette_ok = TRUE;
            }
        }
        if (palette_ok == FALSE)
        {
            gNdsItemFFlowerSubmitStep = 15u;
            return FALSE;
        }
    }

    ndsRendererInitTraversalState(&state, config, stats, NULL, NULL, 0u);
    if (root_offset == NDS_NATIVE_ITEM_FFLOWER_BRANCH_ROOT)
    {
        ndsNativeItemFFlowerBranchSetup(stats, &state,
            base + NDS_NATIVE_ITEM_FFLOWER_BRANCH_TLUT_OFFSET,
            base + NDS_NATIVE_ITEM_FFLOWER_BRANCH_IMAGE_OFFSET);
#if defined(NDS_LAB_FOURCPU_SWEEP) && NDS_LAB_FOURCPU_SWEEP
        ndsLabFFlowerRecordState(root_offset, stats, config, material);
#endif
        if (ndsNativeItemWave1Emit(
                sNdsNativeItemFFlower0Verts, sNdsNativeItemFFlower0VertColors,
                sNdsNativeItemFFlower0TriIndices,
                NDS_NATIVE_ITEM_FFLOWER_VERTEX0_COUNT,
                NDS_NATIVE_ITEM_FFLOWER_TRIANGLE0_COUNT,
                NDS_NATIVE_ITEM_FFLOWER_CORNER0_COUNT,
                config, stats, &state,
                &gNdsItemFFlowerSubmitStep, &gNdsItemFFlowerAlpha) == FALSE)
        {
            return FALSE;
        }
    }
    else
    {
        ndsNativeItemFFlowerLiveBeforeMaterial(stats);
        ndsRendererNativeApplyMaterial(material, stats, &state);
        ndsNativeItemFFlowerLiveAfterMaterial(stats,
            base + NDS_NATIVE_ITEM_FFLOWER_LIVE_IMAGE_OFFSET);
#if defined(NDS_LAB_FOURCPU_SWEEP) && NDS_LAB_FOURCPU_SWEEP
        ndsLabFFlowerRecordState(root_offset, stats, config, material);
#endif
        if (ndsNativeItemWave1Emit(
                sNdsNativeItemFFlower1Verts, sNdsNativeItemFFlower1VertColors,
                sNdsNativeItemFFlower1TriIndices,
                NDS_NATIVE_ITEM_FFLOWER_VERTEX1_COUNT,
                NDS_NATIVE_ITEM_FFLOWER_TRIANGLE1_COUNT,
                NDS_NATIVE_ITEM_FFLOWER_CORNER1_COUNT,
                config, stats, &state,
                &gNdsItemFFlowerSubmitStep, &gNdsItemFFlowerAlpha) == FALSE)
        {
            return FALSE;
        }
        ndsNativeItemFFlowerLiveFinish(stats);
    }
    gNdsItemFFlowerSubmitStep = 9u;
    return TRUE;
}

#endif
