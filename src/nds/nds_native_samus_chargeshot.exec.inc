/* Samus Charge Shot GX executor. TEXTUAL INCLUDE ONLY.
 * File 321 root 0x270 is twenty-eight Gfx words drawing one textured quad.
 * The root is three vertex records short of BombDL because the source packed
 * six real GBI words into the space ahead of it -- a pipesync, two other-mode
 * writes, a set-combine and a blend colour -- and Charge Shot's WPDesc flags
 * are 0x00, so its WPAttributes.data IS that display list.
 *
 * Everything it binds is internal to file 321, and there is no MObj at all, so
 * unlike the Peach's Castle bumper there is no live palette and unlike the
 * Sector Z laser there is no cross-file fixup to verify.  The whole program is
 * fixed; the only live input is the DObj transform, which reaches this owner
 * through config's two matrices.  There is no display-list scan or execution
 * on this path. */
#if NDS_RENDERER_HW_TRIANGLES

sb32 ndsRendererSubmitNativeSamusChargeShot(
    const void *actor_base_ptr, u32 actor_bytes,
    const NDSRendererConfig *config, NDSRendererStats *stats)
{
    const u8 *actor_base = (const u8 *)actor_base_ptr;
    const void *palette;
    const void *image;
    NDSRendererTraversalState state;
    s32 drawn;
    NDSRendererInputVertex vertices[NDS_NATIVE_CHARGESHOT_VERTEX_COUNT];
    u32 i;

    if ((actor_base == NULL) || (config == NULL) || (stats == NULL))
    {
        gNdsChargeShotSubmitStep = 11u;
        return FALSE;
    }
    gNdsChargeShotProjection = (u32)(uintptr_t)config->initial_projection;
    gNdsChargeShotModelview = (u32)(uintptr_t)config->initial_modelview;
    if ((config->initial_projection == NULL) ||
        (config->initial_modelview == NULL))
    {
        /* Both NULL is a different failure from the split-camera case the
         * caller repairs on its config copy; keep it loud. */
        gNdsChargeShotSubmitStep = 12u;
        return FALSE;
    }
    if (actor_bytes < (NDS_NATIVE_CHARGESHOT_ROOT +
                       NDS_NATIVE_CHARGESHOT_DL_BYTES))
    {
        gNdsChargeShotSubmitStep = 2u;
        return FALSE;
    }
    palette = actor_base + NDS_NATIVE_CHARGESHOT_PALETTE_OFFSET;
    image = actor_base + NDS_NATIVE_CHARGESHOT_IMAGE_OFFSET;

    for (i = 0u; i < NDS_NATIVE_CHARGESHOT_VERTEX_COUNT; i++)
    {
        const s16 *v = &sNdsNativeChargeShotVerts[i * 5u];
        u32 rgba = sNdsNativeChargeShotVertColors[i];

        vertices[i] = (NDSRendererInputVertex){
            v[0], v[1], v[2], v[3], v[4],
            rgba >> 24, rgba >> 16, rgba >> 8, rgba
        };
    }

    ndsRendererInitTraversalState(&state, config, stats, NULL, NULL, 0u);
    /* The program writes its own other-mode words, so unlike the Sector Z
     * laser this one does NOT inherit the blender from the weapon pass -- it
     * sets it and restores it, exactly as the source list does. */
    ndsNativeChargeShotSetup(stats, palette, image);
    drawn = ndsRendererNativeTexturedQuad(vertices, sNdsNativeChargeShotTriIndices,
        config, stats, &state);
    if (drawn < 0)
    {
        gNdsChargeShotSubmitStep = 3u;
        return FALSE;
    }
    gNdsChargeShotAlpha = (u32)drawn;
    ndsNativeChargeShotFinish(stats);
    gNdsChargeShotSubmitStep = 9u;
    return TRUE;
}

#endif
