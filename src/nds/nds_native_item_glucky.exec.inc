/* Saffron City GLucky GX executor. TEXTUAL INCLUDE ONLY.
 * File 159 root 0x0270 is thirty Gfx words drawing ONE camera-facing textured
 * quad: four vertices at 0x0230, a 16-entry RGBA16 TLUT at 0x0008 and a CI4
 * 32x32 image at 0x0030, all three internal to file 159 (the file has zero
 * external fixups).  GRYamabukiMap's ITAttributes at 0xBC has p_mobjsubs
 * NULL, so the DObj carries no MObj -- "material 0" is that NULL, NOT
 * "untextured" -- and a word-by-word scan of the program finds no 0xDE call
 * at all, hence no segment-E hook anywhere in it.
 *
 * NOTHING HERE IS LIVE.  is_display_colanim and is_display_xlu are both
 * clear, so itDisplayOPAProcDisplay emits no ColAnim env before the model,
 * and the combiner is (TEXEL0 - 0) * SHADE + 0 with alpha TEXEL0_A in BOTH
 * cycles: no PRIM and no ENV are read at all.  The item layer's seeded
 * prim/env in render_stats therefore cannot change this draw, and this owner
 * must not invent a colour of its own.  (Item-specific: the flag word reads
 * 0x58000000 -- is_item_dobjs SET, so the tree is built with
 * itManagerSetupItemDObjs rather than the common setup, but the DL-less root
 * is still ejected and the same single DL-carrying child is drawn; the extra
 * bits above the display field are hitlag/weight and do not reach the draw.)
 * The only live input is the DObj transform, which arrives through config's
 * two matrices.  There is no display-list scan or execution on this path.
 * (Item-specific: there is no material parameter at all -- a live-material
 * owner takes one and re-checks its contract here; a bake owner that
 * accepted one would decline forever or drop presentation.)
 *
 * The adapter has already COMPARED both relocated SETTIMG words against the
 * loaded file's own base, so nothing is re-derived here. */
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_ITEM_CORE

/* Witness globals this executor needs.  Declared here, defined by the
 * shared wiring; this file never defines them. */
extern volatile u32 gNdsYamabukiGluckySubmitStep;
extern volatile u32 gNdsYamabukiGluckyProjection;
extern volatile u32 gNdsYamabukiGluckyModelview;
extern volatile u32 gNdsYamabukiGluckyTlut;
extern volatile u32 gNdsYamabukiGluckyImage;
extern volatile u32 gNdsYamabukiGluckyAlpha;

sb32 ndsRendererSubmitNativeItemGLucky(
    const void *actor_base_ptr, u32 actor_bytes,
    const NDSRendererConfig *config, NDSRendererStats *stats)
{
    const u8 *actor_base = (const u8 *)actor_base_ptr;
    const void *tlut;
    const void *image;
    NDSRendererTraversalState state;
    s32 drawn;
    NDSRendererInputVertex vertices[NDS_NATIVE_ITEM_GLUCKY_VERTEX_COUNT];
    u32 i;

    /* Split so the witness names WHICH argument was wrong.  The adapter has
     * already proved the item kind, the NULL MObj, the DL head and both
     * relocated pointers by the time it calls here, so a step-11 decline was
     * never going to be one of those. */
    if ((actor_base == NULL) || (config == NULL) || (stats == NULL))
    {
        gNdsYamabukiGluckySubmitStep = 11u;
        return FALSE;
    }
    gNdsYamabukiGluckyProjection = (u32)(uintptr_t)config->initial_projection;
    gNdsYamabukiGluckyModelview = (u32)(uintptr_t)config->initial_modelview;
    if ((config->initial_projection == NULL) ||
        (config->initial_modelview == NULL))
    {
        /* Both NULL is a different failure from the split-camera case the
         * caller repairs on its config copy; keep it loud rather than
         * inventing a second identity here. */
        gNdsYamabukiGluckySubmitStep = 12u;
        return FALSE;
    }
    if ((actor_bytes < NDS_NATIVE_ITEM_GLUCKY_FILE_END) ||
        (actor_bytes < (NDS_NATIVE_ITEM_GLUCKY_ROOT +
                        NDS_NATIVE_ITEM_GLUCKY_DL_BYTES)))
    {
        gNdsYamabukiGluckySubmitStep = 2u;
        return FALSE;
    }
    tlut = actor_base + NDS_NATIVE_ITEM_GLUCKY_TLUT_OFFSET;
    image = actor_base + NDS_NATIVE_ITEM_GLUCKY_IMAGE_OFFSET;
    gNdsYamabukiGluckyTlut = (u32)(uintptr_t)tlut;
    gNdsYamabukiGluckyImage = (u32)(uintptr_t)image;

    for (i = 0u; i < NDS_NATIVE_ITEM_GLUCKY_VERTEX_COUNT; i++)
    {
        const s16 *v = &sNdsNativeItemGLuckyVerts[i * 5u];
        u32 rgba = sNdsNativeItemGLuckyVertColors[i];

        vertices[i] = (NDSRendererInputVertex){
            v[0], v[1], v[2], v[3], v[4],
            rgba >> 24, rgba >> 16, rgba >> 8, rgba
        };
    }

    ndsRendererInitTraversalState(&state, config, stats, NULL, NULL, 0u);
    /* The program writes its own three other-mode words, so it does NOT
     * inherit the item layer's blender: it sets it and restores it, exactly as
     * the source list does. */
    ndsNativeItemGLuckySetup(stats, tlut, image);
    drawn = ndsRendererNativeTexturedQuad(vertices, sNdsNativeItemGLuckyTriIndices,
        config, stats, &state);
    if (drawn < 0)
    {
        gNdsYamabukiGluckySubmitStep = 3u;
        return FALSE;
    }
    gNdsYamabukiGluckyAlpha = (u32)drawn;
    ndsNativeItemGLuckyFinish(stats);
    gNdsYamabukiGluckySubmitStep = 9u;
    return TRUE;
}

#endif
