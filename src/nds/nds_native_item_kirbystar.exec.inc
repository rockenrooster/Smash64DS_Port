/* Kirby capture/lose star and Star Rod weapon executor. TEXTUAL INCLUDE ONLY.
 *
 * ONE ROOT, FOUR SOURCE OWNERS, ONE BAKE.
 * ITCommonObject+0x5458 is dITCommonObject_StarRod_Weapon_data, 22 commands
 * with no 0xDE and no MObjSub anywhere in the four descriptors that reach it:
 *
 *   - dITCommonData_StarRod_WeaponAttributes      (ITCommonData+0x4D4)
 *   - dITCommonData_StarRodSmash_WeaponAttributes (ITCommonData+0x508)
 *   - dEFManagerCaptureKirbyStarEffectDesc        (efmanager.c:1588)
 *   - dEFManagerLoseKirbyStarEffectDesc           (efmanager.c:1618)
 *
 * The two effect descriptors reach it the same way the thrown Poke Ball
 * reaches 0x9430: the maker reads the fixed-up pointer at ITCommonData+0x4D4
 * and SUBTRACTS 0x5458 from it to recover ITCommonObject's base, so the
 * subtrahend IS this display list.  It is NOT a DObjDesc, despite the reloc
 * symbol being named llITCommonDataKirbyStarDObjDesc: neither star descriptor
 * sets EFDesc flag 0x4, so efManagerMakeEffect (efmanager.c:2000) hands the
 * derived address to gcAddChildForDObj as a DISPLAY LIST.  The generator
 * asserts both readings.
 *
 * So the whole material phase is fixed and there is no live-material arm to
 * contract: refuse any material the caller offers rather than silently
 * ignoring it.  Do not bake this geometry a second time for the Star Rod --
 * NDS_NATIVE_ITEM_STARROD_* is the Star Rod ITEM (0x49b0/0x4a18/0x4aa0), a
 * different tree that happens to live in the same file. */
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_ITEM_CORE

/* Defined beside the rest of the Kirby star closure in
 * src/import/battleship_efmanager.c, not in the renderer preamble, so a probe
 * reads a real zero on a roster without Kirby instead of a missing symbol. */
extern volatile u32 gNdsItemKirbyStarSubmitStep;
extern volatile u32 gNdsItemKirbyStarAlpha;

sb32 ndsRendererSubmitNativeItemKirbyStar(
    const void *file_base_ptr, u32 file_bytes,
    const NDSRendererNativeMaterial *material,
    const NDSRendererConfig *config, NDSRendererStats *stats)
{
    const u8 *base = (const u8 *)file_base_ptr;
    NDSRendererTraversalState state;

    if ((base == NULL) || (config == NULL) || (stats == NULL))
    {
        gNdsItemKirbyStarSubmitStep = 11u;
        return FALSE;
    }
    if ((config->initial_projection == NULL) ||
        (config->initial_modelview == NULL))
    {
        gNdsItemKirbyStarSubmitStep = 12u;
        return FALSE;
    }
    if (file_bytes < NDS_NATIVE_ITEM_KIRBYSTAR_FILE_END)
    {
        gNdsItemKirbyStarSubmitStep = 2u;
        return FALSE;
    }
    if (material != NULL)
    {
        gNdsItemKirbyStarSubmitStep = 13u;
        return FALSE;
    }

    ndsRendererInitTraversalState(&state, config, stats, NULL, NULL, 0u);
    ndsNativeItemKirbyStarSetup(
        stats, base + NDS_NATIVE_ITEM_KIRBYSTAR_IMAGE_OFFSET);
    if (ndsNativeItemWave1Emit(
            sNdsNativeItemKirbyStarVerts,
            sNdsNativeItemKirbyStarVertColors,
            sNdsNativeItemKirbyStarTriIndices,
            NDS_NATIVE_ITEM_KIRBYSTAR_VERTEX_COUNT,
            NDS_NATIVE_ITEM_KIRBYSTAR_TRIANGLE_COUNT,
            NDS_NATIVE_ITEM_KIRBYSTAR_CORNER_COUNT,
            config, stats, &state,
            &gNdsItemKirbyStarSubmitStep, &gNdsItemKirbyStarAlpha) == FALSE)
    {
        return FALSE;
    }
    ndsNativeItemKirbyStarFinish(stats);
    gNdsItemKirbyStarSubmitStep = 9u;
    return TRUE;
}

#endif
