/* Native renderer, the submit: texture binds and the owner memo, matrix loads
 * (composed, raw, the fighter split loader), the painter and the triangle
 * submit with its near-plane clip. A slice of nds_renderer.c. */

/* R2-08: the three thin wrappers below are the whole call surface of
 * ndsRendererHardwareResolveOrBindTexture, so bracketing them charges the
 * texture-RESOLVE phase (distinct from upload, which the previous probe already
 * eliminated at 6.7 ticks/frame) without touching a function that has a dozen
 * early returns. Charged only while the effect tree submit is on the stack. */
static s32 ndsRendererHardwareBindTexture(
    NDSRendererStats *stats,
    const NDSRendererConfig *config,
    NDSRendererTraversalState *state)
{
    return ndsRendererHardwareResolveOrBindTexture(
        stats, config, state, NULL, FALSE);
}

/* Native item owners' texture memo (P2-2p8, 2026-09-28). A native item owner
 * (the Mushroom Kingdom Piranha Plant is the first) records the same generated
 * texture state every draw and then binds through the full resolver above: on
 * Mushroom Kingdom that bind is ~7.4K ticks of each ~41K-tick Pakkun draw, two
 * Pakkuns a frame, and the answer is a resident cache hit every time.
 *
 * One memo belongs to one bind site whose texture state is generated and
 * fixed; the caller's id names the run or frame when one site binds several.
 * The key adds every live input: the texel and TLUT images and the load size
 * the setup recorded, prim, env, both othermode words, the combine and the
 * geometry mode. It remembers which cache entry the resolver landed on. A hit replays exactly the resolver's
 * cache-hit tail: the tile sync, the LRU stamp, the name and parameter binds,
 * the active entry, the pinned-hit record and the four stats fields. The entry
 * is revalidated by readiness, name and key generation, so an evicted or
 * recycled slot falls through to the full resolver and refills. */
#define NDS_RENDERER_OWNER_TEXMEMO_KEY_WORDS 11u
typedef struct NDSRendererOwnerTextureMemo
{
    u32 key[NDS_RENDERER_OWNER_TEXMEMO_KEY_WORDS];
    u32 entry_generation;
    u32 name;
    u32 format;
    u32 width;
    u32 height;
    u16 slot;
    u8 valid;
    u8 dedicated;   /* the bind named the dedicated PRIM-RGB/TEXEL0-alpha name */
    u32 params;     /* dedicated: the merged words its bind applied */
} NDSRendererOwnerTextureMemo;
/* P2-2p8 (2026-10-05): a bind that resolves to the dedicated PRIM-RGB/TEXEL0-
 * alpha name (the rebirth beam, Yoshi's Island's clouds) holds no cache entry,
 * so the memo never filled and every such bind ran the full resolver (~5K
 * cycles; three cloud binds a frame on Yoshi's Island). It now fills with the
 * name and the words that bind applied, valid while the dedicated name's
 * generation holds (every prepare and release bumps it). */

volatile u32 gNdsRendererOwnerTexMemoHits;
volatile u32 gNdsRendererOwnerTexMemoFills;
volatile u32 gNdsRendererOwnerTexMemoStale;
#if defined(NDS_LAB_FOURCPU_SWEEP) && NDS_LAB_FOURCPU_SWEEP
/* LAB: 1 = run the full resolver on every bind and count where a memo hit
 * would have answered differently. */
volatile u32 gNdsRendererOwnerTexMemoVerify
    __attribute__((used, section(".data"))) = 0u;
volatile u32 gNdsRendererOwnerTexMemoVerifyRuns;
volatile u32 gNdsRendererOwnerTexMemoVerifyFail;
#endif

static s32 ndsRendererHardwareBindTextureOwnerMemo(
    NDSRendererOwnerTextureMemo *memo, u32 owner_id,
    NDSRendererStats *stats, const NDSRendererConfig *config,
    NDSRendererTraversalState *state)
{
    NDSRendererHardwareTextureCacheEntry *entry;
    u32 key[NDS_RENDERER_OWNER_TEXMEMO_KEY_WORDS];
    u32 i;
    sb32 match;

    key[0] = owner_id;
    key[1] = stats->texture_image;
    key[2] = stats->texture_tlut_image;
    key[3] = stats->texture_load_texels;
    /* P2-6 (2026-10-03): prim and env without their alpha bytes. The
     * resolver's texture key never reads either alpha (it is live polygon
     * state; the PRIM/ENV bakes key `& 0xffffff00`), so an alpha-only
     * material animation resolves to the same entry -- yet keyed whole, every
     * step of Board the Platforms' blinking lights missed here and paid the
     * full resolver: 9 of 21 binds a frame on Purin's board, 88K ticks. */
    key[4] = stats->prim_color & 0xffffff00u;
    key[5] = stats->env_color & 0xffffff00u;
    key[6] = stats->othermode_h;
    key[7] = stats->othermode_l;
    key[8] = stats->texture_combine_w0;
    key[9] = stats->texture_combine_w1;
    key[10] = stats->geometry_mode;
    match = (memo->valid != 0u) ? TRUE : FALSE;
    for (i = 0u; (match != FALSE) && (i < NDS_RENDERER_OWNER_TEXMEMO_KEY_WORDS);
         i++)
    {
        if (memo->key[i] != key[i])
        {
            match = FALSE;
        }
    }
    if ((match != FALSE) && (memo->dedicated != 0u))
    {
        if ((memo->entry_generation == sNdsRendererPrimRgbTexel0AlphaGen) &&
            (memo->name != 0u) &&
            (memo->name == sNdsRendererHardwarePrimRgbTexel0AlphaName))
        {
            ndsRendererSyncTextureTile(stats);
            ndsRendererHardwareBindTextureName(stats, memo->name);
            ndsRendererHardwareApplyTextureParams(memo->params);
            sNdsRendererHardwareActiveTextureEntry = NULL;
            stats->hardware_texture_ready_count++;
            stats->hardware_texture_format = memo->format;
            stats->hardware_texture_width = memo->width;
            stats->hardware_texture_height = memo->height;
            NDS_DIAG(gNdsRendererOwnerTexMemoHits++);
            return TRUE;
        }
        memo->valid = 0u;
        NDS_DIAG(gNdsRendererOwnerTexMemoStale++);
        match = FALSE;
    }
    if (match != FALSE)
    {
        entry = &sNdsRendererHardwareTextureCache[memo->slot];
        if ((entry->ready == FALSE) || ((u32)entry->name != memo->name) ||
            (entry->key_generation != memo->entry_generation))
        {
            memo->valid = 0u;
            NDS_DIAG(gNdsRendererOwnerTexMemoStale++);
            match = FALSE;
        }
    }
#if defined(NDS_LAB_FOURCPU_SWEEP) && NDS_LAB_FOURCPU_SWEEP
    if ((match != FALSE) && (gNdsRendererOwnerTexMemoVerify != 0u))
    {
        s32 ok = ndsRendererHardwareBindTexture(stats, config, state);

        NDS_DIAG(gNdsRendererOwnerTexMemoVerifyRuns++);
        if ((ok == FALSE) ||
            (sNdsRendererHardwareActiveTextureEntry !=
             &sNdsRendererHardwareTextureCache[memo->slot]) ||
            (sNdsRendererHardwareBoundTextureName != memo->name) ||
            (stats->hardware_texture_format != memo->format) ||
            (stats->hardware_texture_width != memo->width) ||
            (stats->hardware_texture_height != memo->height))
        {
            NDS_DIAG(gNdsRendererOwnerTexMemoVerifyFail++);
        }
        return ok;
    }
#endif
    if (match != FALSE)
    {
        ndsRendererSyncTextureTile(stats);
        entry->last_used_frame = sNdsRendererHardwareFrameSerial + 1u;
        if (entry->pinned != 0u)
        {
            ndsRendererHardwareBindTextureName(stats, (u32)entry->name);
            ndsRendererHardwareApplyTextureParams(entry->params);
            sNdsRendererHardwareActiveTextureEntry = entry;
            ndsRendererHardwareRecordBattleStaticTextureHit(entry);
        }
        else if (sNdsRendererHardwareActiveTextureEntry != entry)
        {
            ndsRendererHardwareBindTextureName(stats, (u32)entry->name);
            ndsRendererHardwareApplyTextureParams(entry->params);
            sNdsRendererHardwareActiveTextureEntry = entry;
        }
        stats->hardware_texture_ready_count++;
        stats->hardware_texture_format = memo->format;
        stats->hardware_texture_width = memo->width;
        stats->hardware_texture_height = memo->height;
        NDS_DIAG(gNdsRendererOwnerTexMemoHits++);
        return TRUE;
    }
    if (ndsRendererHardwareBindTexture(stats, config, state) == FALSE)
    {
        memo->valid = 0u;
        return FALSE;
    }
    entry = sNdsRendererHardwareActiveTextureEntry;
    memo->valid = 0u;
    if ((entry != NULL) && (entry->ready != FALSE) &&
        ((u32)entry->name == sNdsRendererHardwareBoundTextureName) &&
        (entry->name != 0))
    {
        u32 slot = (u32)(entry - sNdsRendererHardwareTextureCache);

        if (slot < NDS_RENDERER_HW_TEXTURE_CACHE_COUNT)
        {
            for (i = 0u; i < NDS_RENDERER_OWNER_TEXMEMO_KEY_WORDS; i++)
            {
                memo->key[i] = key[i];
            }
            memo->slot = (u16)slot;
            memo->name = (u32)entry->name;
            memo->entry_generation = entry->key_generation;
            memo->format = stats->hardware_texture_format;
            memo->width = stats->hardware_texture_width;
            memo->height = stats->hardware_texture_height;
            memo->dedicated = 0u;
            memo->valid = 1u;
            NDS_DIAG(gNdsRendererOwnerTexMemoFills++);
        }
    }
    else if ((entry == NULL) &&
             (sNdsRendererHardwarePrimRgbTexel0AlphaName != 0u) &&
             (sNdsRendererHardwareBoundTextureName ==
              sNdsRendererHardwarePrimRgbTexel0AlphaName) &&
             (stats->hardware_texture_format ==
              sNdsRendererPrimRgbTexel0AlphaLastFormat) &&
             (stats->hardware_texture_width ==
              sNdsRendererPrimRgbTexel0AlphaLastWidth) &&
             (stats->hardware_texture_height ==
              sNdsRendererPrimRgbTexel0AlphaLastHeight))
    {
        for (i = 0u; i < NDS_RENDERER_OWNER_TEXMEMO_KEY_WORDS; i++)
        {
            memo->key[i] = key[i];
        }
        memo->slot = 0u;
        memo->name = sNdsRendererHardwarePrimRgbTexel0AlphaName;
        memo->entry_generation = sNdsRendererPrimRgbTexel0AlphaGen;
        memo->params = sNdsRendererPrimRgbTexel0AlphaLastParams;
        memo->format = sNdsRendererPrimRgbTexel0AlphaLastFormat;
        memo->width = sNdsRendererPrimRgbTexel0AlphaLastWidth;
        memo->height = sNdsRendererPrimRgbTexel0AlphaLastHeight;
        memo->dedicated = 1u;
        memo->valid = 1u;
        NDS_DIAG(gNdsRendererOwnerTexMemoFills++);
    }
    return TRUE;
}

/* TRUE when ndsRendererHardwareBindTextureOwnerMemo would compare `memo`
 * equal for this bind: the same key words, in the same order. It does not
 * revalidate the cache entry; the bind still does. For callers that keep
 * several memos per site and pick one before binding. */
_Static_assert(NDS_RENDERER_OWNER_TEXMEMO_KEY_WORDS == 11u,
               "the matcher compares the bind's eleven key words");
static __attribute__((unused)) u32 ndsRendererOwnerTexMemoKeyMatches(
    const NDSRendererOwnerTextureMemo *memo, u32 owner_id,
    const NDSRendererStats *stats)
{
    return ((memo->valid != 0u) &&
            (memo->key[0] == owner_id) &&
            (memo->key[1] == stats->texture_image) &&
            (memo->key[2] == stats->texture_tlut_image) &&
            (memo->key[3] == stats->texture_load_texels) &&
            (memo->key[4] == (stats->prim_color & 0xffffff00u)) &&
            (memo->key[5] == (stats->env_color & 0xffffff00u)) &&
            (memo->key[6] == stats->othermode_h) &&
            (memo->key[7] == stats->othermode_l) &&
            (memo->key[8] == stats->texture_combine_w0) &&
            (memo->key[9] == stats->texture_combine_w1) &&
            (memo->key[10] == stats->geometry_mode)) ? TRUE : FALSE;
}

static s32 ndsRendererHardwareResolveResidentTexture(
    NDSRendererStats *stats,
    const NDSRendererConfig *config,
    NDSRendererTraversalState *state,
    NDSRendererHardwareResolvedTexture *resolved)
{
    if (resolved == NULL)
    {
        return FALSE;
    }
    memset(resolved, 0, sizeof(*resolved));
    return ndsRendererHardwareResolveOrBindTexture(
        stats, config, state, resolved, FALSE);
}

static s32 ndsRendererHardwareResolveStageSourceFrameTexture(
    NDSRendererStats *stats,
    const NDSRendererConfig *config,
    NDSRendererTraversalState *state,
    NDSRendererHardwareResolvedTexture *resolved)
{
    if (resolved == NULL)
    {
        return FALSE;
    }
    memset(resolved, 0, sizeof(*resolved));
    return ndsRendererHardwareResolveOrBindTexture(
        stats, config, state, resolved, TRUE);
}

static void ndsRendererHardwareEndBatch(void);

static void ndsRendererCopyMtx20p12ToM4x4(
    const NDSRendererMatrix20p12 *src, m4x4 *dst)
{
    u32 row;
    u32 col;

    if ((src == NULL) || (dst == NULL))
    {
        return;
    }

    for (row = 0u; row < 4u; row++)
    {
        for (col = 0u; col < 4u; col++)
        {
            dst->m[(row * 4u) + col] = src->m[row][col];
        }
    }
}

/* The copy above writes element i to element i: `NDSRendererMatrix20p12` is
 * `s32 m[4][4]` and libnds' `m4x4` is `int m[16]`, both row-major, both 64
 * bytes. A loader that only wants to hand the matrix to `glLoadMatrix4x4` does
 * not need the intermediate at all -- and the two per-root loaders were paying
 * for two of them, 128 bytes of stack traffic a call, on a function the census
 * prices at 1,064 cycles a call across 12,431 calls with no hot spot above 3.9%
 * and almost all of it MEMORY stall (it is absent from the census's non-mem
 * stall ranking entirely). Deleting the intermediate deletes the traffic.
 *
 * The access is through `int` lvalues on an object whose elements are `int`, so
 * this is not a type-punned read; the static assertions below are what keep the
 * two layouts from drifting apart silently. */
_Static_assert(sizeof(m4x4) == sizeof(NDSRendererMatrix20p12),
               "m4x4 and NDSRendererMatrix20p12 must be the same size");
_Static_assert(sizeof(((m4x4 *)0)->m[0]) ==
                   sizeof(((NDSRendererMatrix20p12 *)0)->m[0][0]),
               "m4x4 and NDSRendererMatrix20p12 elements must match");

static inline const m4x4 *ndsRendererMtx20p12AsM4x4(
    const NDSRendererMatrix20p12 *src)
{
    return (const m4x4 *)src;
}

static void ndsRendererBuildRawHardwareMatrix(
    const NDSRendererMatrix20p12 *composed,
    NDSRendererMatrix20p12 *hardware)
{
    u32 col;

    if ((composed == NULL) || (hardware == NULL))
    {
        return;
    }

    /* Source coordinates are submitted as source / 256 in DS 4.12. Keep
     * composed rows 0..2 unchanged and divide the complete homogeneous row 3
     * by the same factor. The GX clip vector is then CPU clip / 256, preserving
     * X/W, Y/W, and Z/W for arbitrary composed/matrix-word state. */
    *hardware = *composed;
    for (col = 0u; col < 4u; col++)
    {
        hardware->m[3][col] = ndsRendererRoundShiftS32Signed(
            hardware->m[3][col], NDS_RENDERER_HW_WORLD_UNIT_SHIFT);
    }
}

/* ITCM (2026-09-27 census: ~1.9K ticks/frame non-memory stall at 284 B). */
static s32 NDS_R2_ITCM_PACK2_CODE ndsRendererBuildShiftedRawHardwareMatrix(
    const NDSRendererMatrix20p12 *composed,
    NDSRendererMatrix20p12 *hardware,
    u32 coordinate_shift)
{
    u32 row;
    u32 col;

    if ((composed == NULL) || (hardware == NULL))
    {
        return FALSE;
    }
    *hardware = *composed;
    /* 32-bit form of the s64 range check below: this is Thumb code, where
     * every s64 shift and compare is an instruction sequence. For a shift
     * under 32, v << shift fits an s32 exactly when shifting back returns v. */
    if (coordinate_shift < 32u)
    {
        for (row = 0u; row < 3u; row++)
        {
            for (col = 0u; col < 4u; col++)
            {
                s32 v = hardware->m[row][col];
                s32 scaled = (s32)((u32)v << coordinate_shift);

                if ((scaled >> coordinate_shift) != v)
                {
                    return FALSE;
                }
                hardware->m[row][col] = scaled;
            }
        }
    }
    else
    for (row = 0u; row < 3u; row++)
    {
        for (col = 0u; col < 4u; col++)
        {
            s64 scaled = (s64)hardware->m[row][col] << coordinate_shift;

            if ((scaled > INT_MAX) || (scaled < INT_MIN))
            {
                return FALSE;
            }
            hardware->m[row][col] = (s32)scaled;
        }
    }
    for (col = 0u; col < 4u; col++)
    {
        hardware->m[3][col] = ndsRendererRoundShiftS32Signed(
            hardware->m[3][col], NDS_RENDERER_HW_WORLD_UNIT_SHIFT);
    }
    return TRUE;
}

static void ndsRendererLoadHardwareMatrixPair(
    const NDSRendererMatrix20p12 *projection,
    const NDSRendererMatrix20p12 *modelview,
    u32 mode, u32 generation, u32 scale_world)
{
    (void)scale_world;

    if ((sNdsRendererHardwareMatrixLoaded != 0u) &&
        (sNdsRendererHardwareMatrixMode == mode) &&
        (sNdsRendererHardwareMatrixGeneration == generation))
    {
        return;
    }

    ndsRendererHardwareEndBatch();
    ndsRendererHardwareSetMatrixMode(GL_PROJECTION);
    glLoadMatrix4x4(ndsRendererMtx20p12AsM4x4(projection));
    ndsRendererHardwareSetMatrixMode(GL_MODELVIEW);
    glLoadMatrix4x4(ndsRendererMtx20p12AsM4x4(modelview));

    ndsRendererProfileRecordMatrixLoad();

    sNdsRendererHardwareMatrixMode = mode;
    sNdsRendererHardwareMatrixGeneration = generation;
    sNdsRendererHardwareMatrixLoaded = TRUE;
}

static void ndsRendererLoadHardwareRawComposedMatrix(
    const NDSRendererMatrix20p12 *composed, u32 generation)
{
    NDSRendererMatrix20p12 projection;
    NDSRendererMatrix20p12 modelview;

    if ((sNdsRendererHardwareMatrixLoaded != 0u) &&
        (sNdsRendererHardwareMatrixMode ==
         NDS_RENDERER_HW_MATRIX_MODE_RAW_COMPOSED) &&
        (sNdsRendererHardwareMatrixGeneration == generation))
    {
        return;
    }

    ndsRendererMtxIdentity20p12(&projection);
    ndsRendererBuildRawHardwareMatrix(composed, &modelview);
    ndsRendererLoadHardwareMatrixPair(
        &projection, &modelview, NDS_RENDERER_HW_MATRIX_MODE_RAW_COMPOSED,
        generation, TRUE);
}

/* R2-03 E16b. The composed load hands the geometry engine one CPU-built
 * modelview x projection with an identity projection. libnds `GL_MODELVIEW` is
 * DS matrix mode 2, position AND vector, so a vector matrix is already being
 * written on every load -- but what lands in it is the *composed* MVP, and
 * normals must never be rotated by the projection. That is what makes hardware
 * lighting impossible today, not a missing mode.
 *
 * This loads the two matrices separately, so the vector matrix holds the
 * modelview alone and the engine performs the multiply for positions.
 *
 * Exactness: ndsRendererBuildRawHardwareMatrix divides row 3 of the composed
 * matrix by the world-unit shift. Under the row-vector convention
 * C[3] = M[3] x P, so scaling M's row 3 before the multiply yields the same
 * row 3 afterwards -- the scaling commutes with the right-multiply. The only
 * difference from the composed path is that the engine rounds the product in
 * its own internal precision rather than the CPU's 20.12, which is sub-pixel
 * and a screenshot question. */
/* noinline is load-bearing, not style: the caller
 * ndsRendererExecuteNativeFighterOwnerProduction lives in `.itcm.native_fighter`
 * and ITCM has no room -- inlining this overflowed the region by 72 bytes. It
 * runs once per root, so an out-of-line call costs nothing measurable. */

/* Capture-free matrix registers for the fighter-exclusive split loader.
 *
 * glMatrixMode and glLoadMatrix4x4 are #defined in this TU to the Task 29
 * wrappers, which funnel every write through ndsRendererTask29GXRecord. At the
 * shipped config that funnel still carries the NDS_STAGE_HW_COMPOSE == 2
 * replay recorder, and under the tick HUD an sNdsEffectPacketArmed test as
 * well: 791 + 454 ticks/frame over 161,603 executions in the c106 profile, for
 * a path neither recorder can ever observe. Task 36 capture is armed only
 * around a stage run (ndsRendererCommitNativeStageSegment); both callers of the
 * split loader are fighter-side. Same argument as the fighter emit writers in
 * slice 1 -- see the pointer above ndsRendererHardwareWriteColorWord.
 *
 * The modelview variant also folds the world-unit scaling into the FIFO write.
 * Scaling touches row 3 only, so copying the whole 64-byte matrix to change
 * four words was 1,794 ticks/frame over 124,310 executions, plus the 364 the
 * scaling loop spent reading the copy back. */
static inline void ndsRendererHardwareFighterSetMatrixMode(u32 mode)
{
    MATRIX_CONTROL = mode;
}

static inline void ndsRendererHardwareFighterLoadMatrix4x4(
    const NDSRendererMatrix20p12 *matrix)
{
    u32 row;

    for (row = 0u; row < 4u; row++)
    {
        MATRIX_LOAD4x4 = matrix->m[row][0];
        MATRIX_LOAD4x4 = matrix->m[row][1];
        MATRIX_LOAD4x4 = matrix->m[row][2];
        MATRIX_LOAD4x4 = matrix->m[row][3];
    }
}

static inline void ndsRendererHardwareFighterLoadModelviewWorldScaled(
    const NDSRendererMatrix20p12 *matrix)
{
    u32 row;
    u32 col;

    for (row = 0u; row < 3u; row++)
    {
        MATRIX_LOAD4x4 = matrix->m[row][0];
        MATRIX_LOAD4x4 = matrix->m[row][1];
        MATRIX_LOAD4x4 = matrix->m[row][2];
        MATRIX_LOAD4x4 = matrix->m[row][3];
    }
    for (col = 0u; col < 4u; col++)
    {
        s32 value = ndsRendererRoundShiftS32Signed(
            matrix->m[3][col], NDS_RENDERER_HW_WORLD_UNIT_SHIFT);

        MATRIX_LOAD4x4 = value;
    }
}

static void __attribute__((noinline)) NDS_R2_ITCM_PACK2_EVICTED_PLAIN_CODE
ndsRendererLoadHardwareSplitMatrices(
    const NDSRendererMatrix20p12 *projection,
    const NDSRendererMatrix20p12 *modelview,
    u32 generation)
{
    if ((projection == NULL) || (modelview == NULL))
    {
        return;
    }
    if ((sNdsRendererHardwareMatrixLoaded != 0u) &&
        (sNdsRendererHardwareMatrixMode ==
         NDS_RENDERER_HW_MATRIX_MODE_RAW_COMPOSED) &&
        (sNdsRendererHardwareMatrixGeneration == generation))
    {
        return;
    }


    ndsRendererHardwareEndBatch();
    /* R2-03 E23 tried skipping this half when unchanged -- E22 measured 29 of
     * the 30 per-root loads a frame re-push an identical projection. Engaged on
     * 93.8% of loads and worth -3,008 FTR P50, under the placement floor. The
     * FIFO writes are simply cheap; see the E22/E23 write-up. Reverted. */
    ndsRendererHardwareFighterSetMatrixMode(GL_PROJECTION);
    ndsRendererHardwareFighterLoadMatrix4x4(projection);
    ndsRendererHardwareFighterSetMatrixMode(GL_MODELVIEW);
    ndsRendererHardwareFighterLoadModelviewWorldScaled(modelview);

    ndsRendererProfileRecordMatrixLoad();
    sNdsRendererHardwareMatrixMode = NDS_RENDERER_HW_MATRIX_MODE_RAW_COMPOSED;
    sNdsRendererHardwareMatrixGeneration = generation;
    sNdsRendererHardwareMatrixLoaded = TRUE;
}

/* The adapter stops composing at all under this flag, so `modelview_matrix` is
 * the seed rather than a world. BindProductionRoot only leaves it unread when
 * HW_MTX and SHADE_SKIP_SOFT_LIGHT both hold; without them the software light
 * preparation would take a root's transform from a matrix that is no longer
 * one. The SHADE_SKIP half cannot be tested here -- it is #defined 4,000 lines
 * further down, next to the code it guards -- so it is asserted there. */

u32 gNdsR2GxComposeRoots;
u32 gNdsR2GxComposeMults;
u32 gNdsR2GxComposeRestores;
u32 gNdsR2GxComposeStores;
u32 gNdsR2GxComposeProjectionSkips;

/* Reset at the top of every execute, which is what makes pointer equality a
 * sound test for "the hardware already holds this projection". */
static const NDSRendererMatrix20p12 *sNdsR2GxLastProjection;

static const NDSRendererMatrix20p12 sNdsR2GxIdentity20p12 =
{
    {
        { 1 << NDS_RENDERER_DS_MTX_FRAC_BITS, 0, 0, 0 },
        { 0, 1 << NDS_RENDERER_DS_MTX_FRAC_BITS, 0, 0 },
        { 0, 0, 1 << NDS_RENDERER_DS_MTX_FRAC_BITS, 0 },
        { 0, 0, 0, 1 << NDS_RENDERER_DS_MTX_FRAC_BITS },
    }
};

static inline void ndsRendererHardwareFighterMultMatrix4x3(
    const NDSRendererMatrix20p12 *matrix)
{
    u32 row;

    for (row = 0u; row < 4u; row++)
    {
        MATRIX_MULT4x3 = matrix->m[row][0];
        MATRIX_MULT4x3 = matrix->m[row][1];
        MATRIX_MULT4x3 = matrix->m[row][2];
    }
}

/* diag(1, 1, 1, s) x current, s being the world-unit shift. MTX_MULT is a LEFT
 * multiply, so this scales row 3 -- translation and w together -- of whatever
 * chain has been composed, which is precisely what
 * ndsRendererHardwareFighterLoadModelviewWorldScaled writes today. It has to be
 * a separate final multiply rather than folded into the factors: scaling row 3
 * of every matrix in a chain does NOT compose to scaling the product's row 3,
 * because each factor's own m[3][3] then multiplies the next one's scaled row. */
static inline void ndsRendererHardwareFighterMultMatrixWorldScaled(
    const NDSRendererMatrix20p12 *matrix)
{
    u32 row;
    u32 col;

    for (row = 0u; row < 3u; row++)
    {
        MATRIX_MULT4x4 = matrix->m[row][0];
        MATRIX_MULT4x4 = matrix->m[row][1];
        MATRIX_MULT4x4 = matrix->m[row][2];
        MATRIX_MULT4x4 = matrix->m[row][3];
    }
    for (col = 0u; col < 4u; col++)
    {
        MATRIX_MULT4x4 = ndsRendererRoundShiftS32Signed(
            matrix->m[3][col], NDS_RENDERER_HW_WORLD_UNIT_SHIFT);
    }
}

/* Slice 43. The geometry engine composes the joint chain: restore the parent
 * binding's finished world from the palette (or seed a root binding), multiply
 * this binding's chain in the order the CPU pass used, leave the UNSCALED world
 * where a child or a cross-run corner can find it, then apply the world-unit
 * scale for the draw. GL_MODELVIEW is DS matrix mode 2, so every one of these
 * commands acts on the position and vector matrices together -- which is what
 * NDS_R2_FIGHTER_HW_LIGHT needs, and the scale is a no-op on the 3x3 vector
 * matrix because it only touches row 3. */

/* 2026-09-27: out of ITCM (old path only; the four-CPU census never
 * executed it). Its bytes went to the census's small high-stall admissions. */
static void __attribute__((noinline))
ndsRendererLoadHardwareGxComposedMatrices(
    const NDSRendererNativeFighterRoot *input, u32 generation)
{
    u32 i;

    if ((input == NULL) || (input->projection_matrix == NULL) ||
        (input->gx_seed == NULL) ||
        ((input->gx_local_count != 0u) && (input->gx_locals == NULL)))
    {
        return;
    }

    ndsRendererHardwareEndBatch();
    /* R2-03 E22 measured 29 of the 30 per-root loads a frame re-pushing an
     * IDENTICAL projection, and E23 measured skipping it at -3,008 -- under the
     * floor on its own, so it was reverted. It is not on its own here: this
     * slice's own gate put the FIFO word at ~24 cycles rather than the 12.2 E23
     * implied, which makes 31 elided 17-word pushes worth about twice what E23
     * priced. Pointer equality is sound because the cache is reset at the top of
     * each execute and the adapter cannot rewrite the projection buffer inside
     * one -- nothing between two roots changes GL_PROJECTION either, the light
     * vector write brackets GL_MODELVIEW only. */
    if (input->projection_matrix != sNdsR2GxLastProjection)
    {
        ndsRendererHardwareFighterSetMatrixMode(GL_PROJECTION);
        ndsRendererHardwareFighterLoadMatrix4x4(input->projection_matrix);
        sNdsR2GxLastProjection = input->projection_matrix;
    }
    else
    {
        NDS_DIAG(gNdsR2GxComposeProjectionSkips++);
    }
    ndsRendererHardwareFighterSetMatrixMode(GL_MODELVIEW);

    if (input->gx_parent_slot >= NDS_RENDERER_FIGHTER_GX_SLOT_NONE)
    {
        if (input->gx_seed_is_identity != 0u)
        {
            MATRIX_IDENTITY = 0;
        }
        else
        {
            ndsRendererHardwareFighterLoadMatrix4x4(input->gx_seed);
        }
    }
    else
    {
        MATRIX_RESTORE = input->gx_parent_slot;
        NDS_DIAG(gNdsR2GxComposeRestores++);
    }
    for (i = 0u; i < (u32)input->gx_local_count; i++)
    {
        ndsRendererHardwareFighterMultMatrix4x3(&input->gx_locals[i]);
    }
    NDS_DIAG(gNdsR2GxComposeMults += (u32)input->gx_local_count);
    if (input->gx_store_slot < NDS_RENDERER_FIGHTER_GX_SLOT_NONE)
    {
        MATRIX_STORE = input->gx_store_slot;
        NDS_DIAG(gNdsR2GxComposeStores++);
    }
    ndsRendererHardwareFighterMultMatrixWorldScaled(&sNdsR2GxIdentity20p12);
    NDS_DIAG(gNdsR2GxComposeRoots++);

    ndsRendererProfileRecordMatrixLoad();
    /* The generation memo cannot elide anything here: every binding leaves a
     * different matrix, and the restore/store pair is state the memo does not
     * model. Report "not loaded" so no later reader believes it knows what the
     * hardware holds. */
    sNdsRendererHardwareMatrixMode = NDS_RENDERER_HW_MATRIX_MODE_RAW_COMPOSED;
    sNdsRendererHardwareMatrixGeneration = generation;
    sNdsRendererHardwareMatrixLoaded = FALSE;
}


static void ndsRendererLoadHardwareMatrices(
    const NDSRendererTraversalState *state, u32 scale_world)
{
    NDSRendererMatrix20p12 projection;
    NDSRendererMatrix20p12 modelview;

    if ((state != NULL) && (scale_world != 0u) &&
        (state->matrix_valid != 0u))
    {
        if ((sNdsRendererHardwareMatrixLoaded != 0u) &&
            (sNdsRendererHardwareMatrixMode ==
             NDS_RENDERER_HW_MATRIX_MODE_RAW_COMPOSED) &&
            (sNdsRendererHardwareMatrixGeneration ==
             state->matrix_generation))
        {
            return;
        }
        ndsRendererLoadHardwareRawComposedMatrix(
            &state->matrix, state->matrix_generation);
        return;
    }

    if ((sNdsRendererHardwareMatrixLoaded != 0u) &&
        (sNdsRendererHardwareMatrixMode ==
         NDS_RENDERER_HW_MATRIX_MODE_PROJECTED_IDENTITY) &&
        (sNdsRendererHardwareMatrixGeneration == 0u))
    {
        return;
    }

    ndsRendererMtxIdentity20p12(&projection);
    ndsRendererMtxIdentity20p12(&modelview);
    ndsRendererLoadHardwareMatrixPair(
        &projection, &modelview,
        NDS_RENDERER_HW_MATRIX_MODE_PROJECTED_IDENTITY, 0u, FALSE);
}


static s32 ndsRendererHardwareNextProjectedDepth(void)
{
    /* The stored counter is scaled by STEP so each painter primitive must
     * consume one complete submitted-v16 depth value.  Subtracting one here
     * made six consecutive no-Z triangles share a depth after division,
     * allowing an earlier stage triangle to reject a later grass/bush draw. */
    sNdsRendererHardwareProjectedDepth -=
        NDS_RENDERER_HW_PROJECTED_DEPTH_STEP;
    /* The foreground band leaves its nearest NDS_FTR_LEAN_FRONT_DEPTH_BAND
     * depths to a fighter drawn after it (renderer_fighter_lean.c); a frame
     * that would pass them reuses the last depth it may take. */
    if ((sNdsRendererHardwareProjectedBackground == FALSE) &&
        (sNdsRendererHardwareProjectedDepth <
         ((NDS_FTR_LEAN_FRONT_DEPTH_BAND - 0x1000) *
          NDS_RENDERER_HW_PROJECTED_DEPTH_STEP)))
    {
        sNdsRendererHardwareProjectedDepth =
            (NDS_FTR_LEAN_FRONT_DEPTH_BAND - 0x1000) *
            NDS_RENDERER_HW_PROJECTED_DEPTH_STEP;
    }
    return sNdsRendererHardwareProjectedDepth /
        NDS_RENDERER_HW_PROJECTED_DEPTH_STEP;
}

#if NDS_TICK_HUD
/* G3 step 5. Slots consumed in the background band before this frame's first
 * source-Z triangle flipped the counter. Captured at the transition because the
 * counter is reloaded there and the value is otherwise unrecoverable. */
static u32 sNdsPainterSlotBgUsed;

static u32 ndsRendererHardwarePainterSlotsUsed(s32 start)
{
    s32 used = (start - sNdsRendererHardwareProjectedDepth) /
        NDS_RENDERER_HW_PROJECTED_DEPTH_STEP;

    return (used > 0) ? (u32)used : 0u;
}
#endif

static void ndsRendererHardwareEnterProjectedForeground(void)
{
    if (sNdsRendererHardwareProjectedBackground == FALSE)
    {
        return;
    }

#if NDS_TICK_HUD
    sNdsPainterSlotBgUsed = ndsRendererHardwarePainterSlotsUsed(
        NDS_RENDERER_HW_PROJECTED_DEPTH_BACKGROUND_START);
#endif
    /* The DS cannot disable depth testing per polygon. Mirror sm64-nds'
     * source G_ZBUFFER transition: early no-Z background draws count down
     * from the far endpoint, then the first source-Z triangle moves later
     * no-Z painter passes in front of the source depth range. */
    sNdsRendererHardwareProjectedDepth =
        NDS_RENDERER_HW_PROJECTED_DEPTH_FOREGROUND_START;
    sNdsRendererHardwareProjectedBackground = FALSE;
}

#if NDS_TICK_HUD
/* Folded once per renderer-owned hardware frame, immediately before the depth
 * counter is reset, so gNdsPainterSlotFrames doubles as the engagement proof
 * (it must track the frame serial). Derived from the counter rather than
 * incremented inside ndsRendererHardwareNextProjectedDepth because the M3
 * replay path decrements it in bulk without calling the accessor. */
static void ndsRendererHardwarePainterSlotFoldFrame(void)
{
    u32 bg_used;
    u32 fg_used;
    u32 total;

    if (sNdsRendererHardwareProjectedBackground != FALSE)
    {
        bg_used = ndsRendererHardwarePainterSlotsUsed(
            NDS_RENDERER_HW_PROJECTED_DEPTH_BACKGROUND_START);
        fg_used = 0u;
    }
    else
    {
        bg_used = sNdsPainterSlotBgUsed;
        fg_used = ndsRendererHardwarePainterSlotsUsed(
            NDS_RENDERER_HW_PROJECTED_DEPTH_FOREGROUND_START);
    }
    total = bg_used + fg_used;

    NDS_DIAG(gNdsPainterSlotFrames++);
    NDS_DIAG(gNdsPainterSlotBgSum += bg_used);
    NDS_DIAG(gNdsPainterSlotFgSum += fg_used);
    if (bg_used > gNdsPainterSlotBgMax)
    {
        gNdsPainterSlotBgMax = bg_used;
    }
    if (fg_used > gNdsPainterSlotFgMax)
    {
        gNdsPainterSlotFgMax = fg_used;
    }
    if (total > gNdsPainterSlotTotalMax)
    {
        gNdsPainterSlotTotalMax = total;
    }
    if (bg_used > 128u)
    {
        NDS_DIAG(gNdsPainterSlotBgOverBand++);
    }
    if (fg_used > 128u)
    {
        NDS_DIAG(gNdsPainterSlotFgOverBand++);
    }
    sNdsPainterSlotBgUsed = 0u;
}
#endif

static void ndsRendererHardwareClipVertex(
    const NDSRendererClipVertex20p12 *vtx, s32 z
    )
{
    v16 x;
    v16 y;
    v16 out_z;

    if ((vtx == NULL) || (vtx->w == 0))
    {
        return;
    }

    x = ndsRendererHardwareProjectToV16(
        (s64)vtx->x * NDS_RENDERER_HW_PROJECTED_VERTEX, vtx->w);
    y = ndsRendererHardwareProjectToV16(
        (s64)vtx->y * NDS_RENDERER_HW_PROJECTED_VERTEX, vtx->w);
    out_z = ndsRendererHardwareSourceDepthToV16(
        (s64)z * NDS_RENDERER_HW_PROJECTED_VERTEX, vtx->w);
    ndsRendererProfileHWVertexRange(x, y, out_z);
    glVertex3v16(x, y, out_z);
}

static void ndsRendererHardwareClipVertexNdcDepth(
    const NDSRendererClipVertex20p12 *vtx, s32 z
    )
{
    v16 x;
    v16 y;
    v16 out_z;

    if ((vtx == NULL) || (vtx->w == 0))
    {
        return;
    }
    x = ndsRendererHardwareProjectToV16(
        (s64)vtx->x * NDS_RENDERER_HW_PROJECTED_VERTEX, vtx->w);
    y = ndsRendererHardwareProjectToV16(
        (s64)vtx->y * NDS_RENDERER_HW_PROJECTED_VERTEX, vtx->w);
    out_z = ndsRendererHardwareClampS64ToV16(z);
    ndsRendererProfileHWVertexRange(x, y, out_z);
    glVertex3v16(x, y, out_z);
}

/* Back in ITCM (2026-09-27): _arm_addsubsf3.o left for the port int->float
 * conversions, and the texture-bind path is materialization/hit-frame work. */
static void __attribute__((noinline, section(".itcm"))) ndsRendererHardwareEndBatch(void)
{
    if (sNdsRendererHardwareTriangleBatchOpen != 0u)
    {
        /* libnds documents glEnd() as a dummy FIFO write. A later glBegin()
         * starts the next primitive group, so only restore state owned by the
         * logical source-triangle batch here. */
        glDisable(GL_ALPHA_TEST);
        ndsRendererProfileRecordBatchEnd();
        sNdsRendererHardwareTriangleBatchOpen = FALSE;
        sNdsRendererHardwareTriangleBatchTextured = FALSE;
        sNdsRendererHardwareTriangleBatchTextureName = 0u;
        sNdsRendererHardwareTriangleBatchPolyFmt = 0u;
        sNdsRendererHardwareTriangleBatchAlphaKey = 0u;
        sNdsRendererHardwareTriangleBatchFogKey = 0u;
        sNdsRendererHardwareTriangleBatchMatrixMode =
            NDS_RENDERER_HW_MATRIX_MODE_NONE;
        sNdsRendererHardwareTriangleBatchMatrixGeneration = 0u;
    }
}

static void ndsRendererHardwareBeginTriangleBatch(
    const NDSRendererStats *stats,
    u32 textured,
    u32 texture_name,
    u32 poly_fmt,
    u32 matrix_mode,
    u32 matrix_generation)
{
    u32 alpha_key;
    u32 fog_key;

    /* An open GX batch can only contain adjacent TRI1/TRI2 source commands.
     * Every opcode capable of changing alpha, fog, texture, or matrix state
     * closes it in ndsRendererScanList before mutation. Keep the per-triangle
     * reuse check to the values that can differ through vertex selection. */
    if ((sNdsRendererHardwareTriangleBatchOpen != 0u) &&
        (sNdsRendererHardwareTriangleBatchTextured == textured) &&
        (sNdsRendererHardwareTriangleBatchTextureName == texture_name) &&
        (sNdsRendererHardwareTriangleBatchPolyFmt == poly_fmt) &&
        (sNdsRendererHardwareTriangleBatchMatrixMode == matrix_mode) &&
        (sNdsRendererHardwareTriangleBatchMatrixGeneration ==
         matrix_generation))
    {
        ndsRendererProfileRecordBatchReuse();
        return;
    }

    alpha_key = ndsRendererHardwareAlphaStateKey(stats);
    fog_key = ndsRendererHardwareFogStateKey(stats);
    ndsRendererHardwareEndBatch();
    if (textured != 0u)
    {
        glEnable(GL_TEXTURE_2D);
    }
    else
    {
        glEnable(GL_TEXTURE_2D);
        ndsRendererHardwareBindNoTexture(NULL);
    }
    ndsRendererHardwareApplyAlphaTest(stats);
    ndsRendererHardwareApplyFog(stats);
    ndsRendererHardwareSetPolyFmt(poly_fmt);
    glBegin(GL_TRIANGLE);
    ndsRendererProfileRecordBatchBegin();

    sNdsRendererHardwareTriangleBatchOpen = TRUE;
    sNdsRendererHardwareTriangleBatchTextured = textured;
    sNdsRendererHardwareTriangleBatchTextureName = texture_name;
    sNdsRendererHardwareTriangleBatchPolyFmt = poly_fmt;
    sNdsRendererHardwareTriangleBatchAlphaKey = alpha_key;
    sNdsRendererHardwareTriangleBatchFogKey = fog_key;
    sNdsRendererHardwareTriangleBatchMatrixMode = matrix_mode;
    sNdsRendererHardwareTriangleBatchMatrixGeneration = matrix_generation;
}

/* c200's whole-match PC census never enters either of these position-emission
 * branches.  Keep the generic renderer exact for callers that need them, but
 * do not make the hot vertex submitter carry their inlined bodies in ITCM. */
static void __attribute__((noinline, cold))
ndsRendererHardwareSubmitVertexRawZCold(
    const NDSRendererInputVertex *vtx, u32 scale_world)
{
    v16 x = ndsRendererHardwareVertexCoord(vtx->x, scale_world);
    v16 y = ndsRendererHardwareVertexCoord(vtx->y, scale_world);
    v16 z = ndsRendererHardwareVertexCoord(vtx->z, scale_world);

    ndsRendererProfileVertexRange(vtx, x, y, z);
    glVertex3v16(x, y, z);
}

static void __attribute__((noinline, cold))
ndsRendererHardwareSubmitVertexDecalCold(
    const NDSRendererClipVertex20p12 *clip_vtx, s32 projected_z)
{
    (void)projected_z;
    if (clip_vtx != NULL)
    {
        ndsRendererHardwareClipVertex(
            clip_vtx, clip_vtx->z - NDS_RENDERER_HW_DECAL_DEPTH_BIAS);
    }
}

static void NDS_R2_ITCM_PACK2_EVICTED_CODE
ndsRendererHardwareSubmitVertex(
    NDSRendererStats *stats,
    NDSRendererTraversalState *state,
    u32 vertex_index,
    s32 projected_z
    )
{
    const NDSRendererInputVertex *vtx;
    const NDSRendererClipVertex20p12 *clip_vtx;
    u32 material_color;
    u32 scale_s;
    u32 scale_t;
    u32 texture_origin_s;
    u32 texture_origin_t;
    u32 context_flags;
    u32 scale_world;
    u32 vertex_color;
    s32 use_material_color;
    s32 use_vertex_color;
    s32 vertex_color_valid;
    s32 use_texture;
    s32 texture_offset;
    s32 zbuffered;
    s32 decal_depth;
    s32 prim_depth;
    s32 source_clip_depth;
    u16 hardware_color;

    if ((stats == NULL) || (state == NULL) ||
        (vertex_index >= NDS_RENDERER_MAX_VTX))
    {
        return;
    }
    vtx = &state->input_vertices[vertex_index];
    clip_vtx = &state->vertices[vertex_index];
    material_color = state->texture_prepare_material_color;
    scale_s = state->texture_prepare_scale_s;
    scale_t = state->texture_prepare_scale_t;
    texture_origin_s = state->texture_prepare_origin_s;
    texture_origin_t = state->texture_prepare_origin_t;
    context_flags = state->texture_prepare_vertex_flags;
    scale_world = context_flags & NDS_RENDERER_VERTEX_CONTEXT_SCALE_WORLD;
    vertex_color = state->vertex_colors[vertex_index];
    use_material_color =
        context_flags & NDS_RENDERER_VERTEX_CONTEXT_USE_MATERIAL;
    use_vertex_color =
        context_flags & NDS_RENDERER_VERTEX_CONTEXT_USE_VERTEX;
    vertex_color_valid =
        ((state->vertex_color_valid_mask & (1u << vertex_index)) != 0u) ?
            TRUE : FALSE;
    use_texture = context_flags & NDS_RENDERER_VERTEX_CONTEXT_USE_TEXTURE;
    texture_offset = state->texture_prepare_offset;
    zbuffered = context_flags & NDS_RENDERER_VERTEX_CONTEXT_ZBUFFERED;
    decal_depth = context_flags & NDS_RENDERER_VERTEX_CONTEXT_DECAL_DEPTH;
    prim_depth = context_flags & NDS_RENDERER_VERTEX_CONTEXT_PRIM_DEPTH;
    source_clip_depth =
        context_flags & NDS_RENDERER_VERTEX_CONTEXT_SOURCE_CLIP_DEPTH;

    if ((state != NULL) &&
        (vertex_index < NDS_RENDERER_MAX_VTX) &&
        ((state->prepared_vertex_color_valid_mask &
          (1u << vertex_index)) != 0u))
    {
        hardware_color = state->prepared_vertex_colors[vertex_index];
    }
    else
    {
        hardware_color = ndsRendererHardwarePackedVertexColor(
            stats, vtx, material_color, use_material_color,
            use_vertex_color, vertex_color, vertex_color_valid,
            state->color_modulate);
        if ((state != NULL) && (vertex_index < NDS_RENDERER_MAX_VTX))
        {
            state->prepared_vertex_colors[vertex_index] = hardware_color;
            state->prepared_vertex_color_valid_mask |= 1u << vertex_index;
        }
    }
    glColor(hardware_color);
    if (use_texture != FALSE)
    {
        s16 s;
        s16 t;

        if ((state != NULL) &&
            (vertex_index < NDS_RENDERER_MAX_VTX) &&
            ((state->prepared_texcoord_valid_mask &
              (1u << vertex_index)) != 0u))
        {
            s = state->prepared_texcoord_s[vertex_index];
            t = state->prepared_texcoord_t[vertex_index];
        }
        else
        {
            s = ndsRendererHardwareTexCoord(vtx->s, scale_s,
                                            texture_origin_s,
                                            texture_offset);
            t = ndsRendererHardwareTexCoord(vtx->t, scale_t,
                                            texture_origin_t,
                                            texture_offset);
            if ((state != NULL) && (vertex_index < NDS_RENDERER_MAX_VTX))
            {
                state->prepared_texcoord_s[vertex_index] = s;
                state->prepared_texcoord_t[vertex_index] = t;
                state->prepared_texcoord_valid_mask |= 1u << vertex_index;
            }
        }

        glTexCoord2t16(s, t);
    }
    if ((zbuffered != FALSE) &&
        (decal_depth == FALSE) &&
        (prim_depth == FALSE))
    {
        ndsRendererHardwareSubmitVertexRawZCold(vtx, scale_world);
    }
    else if (prim_depth != FALSE)
    {
        ndsRendererHardwareClipVertex(clip_vtx, projected_z
                                      );
    }
    else if (decal_depth != FALSE)
    {
        ndsRendererHardwareSubmitVertexDecalCold(clip_vtx, projected_z);
    }
    else
    {
        /* X, Y, Z, and W must come from the same composed matrix snapshot.
         * Matrix-word updates can make projection/modelview fields stale. */
        if ((source_clip_depth != FALSE) && (clip_vtx != NULL) &&
            (clip_vtx->w != 0))
        {
            projected_z = clip_vtx->z;
        }
        if ((state != NULL) &&
            (vertex_index < NDS_RENDERER_MAX_VTX) &&
            (clip_vtx != NULL) && (clip_vtx->w != 0))
        {
            u32 vertex_mask = 1u << vertex_index;
            v16 x;
            v16 y;
            v16 z;

            if ((state->prepared_projected_xy_valid_mask & vertex_mask) !=
                0u)
            {
                x = state->prepared_projected_x[vertex_index];
                y = state->prepared_projected_y[vertex_index];
            }
            else
            {
                x = ndsRendererHardwareProjectToV16(
                    (s64)clip_vtx->x * NDS_RENDERER_HW_PROJECTED_VERTEX,
                    clip_vtx->w);
                y = ndsRendererHardwareProjectToV16(
                    (s64)clip_vtx->y * NDS_RENDERER_HW_PROJECTED_VERTEX,
                    clip_vtx->w);
                state->prepared_projected_x[vertex_index] = x;
                state->prepared_projected_y[vertex_index] = y;
                state->prepared_projected_xy_valid_mask |= vertex_mask;
            }
            if (source_clip_depth != FALSE)
            {
                if ((state->prepared_projected_source_z_valid_mask &
                     vertex_mask) != 0u)
                {
                    z = state->prepared_projected_source_z[vertex_index];
                }
                else
                {
                    z = ndsRendererHardwareSourceDepthToV16(
                        (s64)projected_z *
                            NDS_RENDERER_HW_PROJECTED_VERTEX,
                        clip_vtx->w);
                    state->prepared_projected_source_z[vertex_index] = z;
                    state->prepared_projected_source_z_valid_mask |=
                        vertex_mask;
                }
            }
            else
            {
                z = ndsRendererHardwareClampS64ToV16(projected_z);
            }
            glVertex3v16(x, y, z);
        }
        else if (source_clip_depth != FALSE)
        {
            ndsRendererHardwareClipVertex(clip_vtx, projected_z);
        }
        else
        {
            ndsRendererHardwareClipVertexNdcDepth(clip_vtx, projected_z);
        }
    }
}

static s32 ndsRendererInputTriangleReady(
    const NDSRendererTraversalState *state, u32 packed,
    u32 *out_i0, u32 *out_i1, u32 *out_i2)
{
    u32 i0;
    u32 i1;
    u32 i2;
    u32 mask;

    ndsGBIDecodePackedTriIndices(packed, &i0, &i1, &i2);
    if (out_i0 != NULL) { *out_i0 = i0; }
    if (out_i1 != NULL) { *out_i1 = i1; }
    if (out_i2 != NULL) { *out_i2 = i2; }

    if ((state == NULL) ||
        (i0 >= NDS_RENDERER_MAX_VTX) ||
        (i1 >= NDS_RENDERER_MAX_VTX) ||
        (i2 >= NDS_RENDERER_MAX_VTX))
    {
        return FALSE;
    }

    mask = (1u << i0) | (1u << i1) | (1u << i2);
    return ((state->input_vertex_valid_mask & mask) == mask) ? TRUE : FALSE;
}

static s32 ndsRendererHardwareClipZWInsideNearPlane(s32 z, s32 w)
{
    return ((w > 0) && (((s64)z + (s64)w) >= 0)) ? TRUE : FALSE;
}

static s32 __attribute__((optimize("Os"))) ndsRendererHardwareClipLerpQ16(
    s32 from, s32 to, s32 ratio_q16)
{
    s64 delta = (s64)to - (s64)from;

    return (s32)((s64)from + ((delta * ratio_q16) >> 16));
}

static u16 __attribute__((optimize("Os")))
ndsRendererHardwareClipLerpColorQ16(
    u16 from, u16 to, s32 ratio_q16)
{
    u32 r = (u32)ndsRendererHardwareClipLerpQ16(
        from & 31u, to & 31u, ratio_q16);
    u32 g = (u32)ndsRendererHardwareClipLerpQ16(
        (from >> 5) & 31u, (to >> 5) & 31u, ratio_q16);
    u32 b = (u32)ndsRendererHardwareClipLerpQ16(
        (from >> 10) & 31u, (to >> 10) & 31u, ratio_q16);

    return (u16)(r | (g << 5) | (b << 10));
}

static void __attribute__((noinline, cold, optimize("Os")))
ndsRendererHardwareClipNearIntersection(
    const NDSRendererProjectedClipVertex *from,
    const NDSRendererProjectedClipVertex *to,
    NDSRendererProjectedClipVertex *out)
{
    s32 from_distance = (s32)(
        ((s64)from->clip.z + (s64)from->clip.w) / 4);
    s32 to_distance = (s32)(
        ((s64)to->clip.z + (s64)to->clip.w) / 4);
    s32 denominator = from_distance - to_distance;
    s32 ratio_q16;

    if (denominator == 0)
    {
        *out = *from;
        return;
    }
#if defined(__arm__)
    ratio_q16 = (s32)ndsR2HwMathDiv64((s64)from_distance * 65536, denominator);
#else
    ratio_q16 = (s32)(((s64)from_distance * 65536) / denominator);
#endif
    out->clip.x = ndsRendererHardwareClipLerpQ16(
        from->clip.x, to->clip.x, ratio_q16);
    out->clip.y = ndsRendererHardwareClipLerpQ16(
        from->clip.y, to->clip.y, ratio_q16);
    out->clip.w = ndsRendererHardwareClipLerpQ16(
        from->clip.w, to->clip.w, ratio_q16);
    out->clip.z = -out->clip.w;
    out->s = ndsRendererHardwareClipLerpQ16(
        from->s, to->s, ratio_q16);
    out->t = ndsRendererHardwareClipLerpQ16(
        from->t, to->t, ratio_q16);
    out->packed_color = ndsRendererHardwareClipLerpColorQ16(
        from->packed_color, to->packed_color, ratio_q16);
}

static u32 __attribute__((noinline, cold, optimize("Os")))
ndsRendererHardwareClipTriangleNearPlane(
    const NDSRendererProjectedClipVertex input[3],
    NDSRendererProjectedClipVertex output[4])
{
    const NDSRendererProjectedClipVertex *previous = &input[2];
    s32 previous_inside = ndsRendererHardwareClipZWInsideNearPlane(
        previous->clip.z, previous->clip.w);
    u32 input_index;
    u32 output_count = 0u;

    for (input_index = 0u; input_index < 3u; input_index++)
    {
        const NDSRendererProjectedClipVertex *current = &input[input_index];
        s32 current_inside = ndsRendererHardwareClipZWInsideNearPlane(
            current->clip.z, current->clip.w);

        if (current_inside != FALSE)
        {
            if (previous_inside == FALSE)
            {
                ndsRendererHardwareClipNearIntersection(
                    previous, current, &output[output_count++]);
            }
            output[output_count++] = *current;
        }
        else if (previous_inside != FALSE)
        {
            ndsRendererHardwareClipNearIntersection(
                previous, current, &output[output_count++]);
        }
        previous = current;
        previous_inside = current_inside;
    }
    return output_count;
}

static void __attribute__((noinline, cold, optimize("Os")))
ndsRendererHardwareEmitClippedVertex(
    const NDSRendererProjectedClipVertex *vertex,
    u32 context_flags,
    s32 projected_z)
{
    glColor(vertex->packed_color);
    if ((context_flags & NDS_RENDERER_VERTEX_CONTEXT_USE_TEXTURE) != 0u)
    {
        glTexCoord2t16((s16)vertex->s, (s16)vertex->t);
    }
    /* Same depth choice ndsRendererHardwareSubmitVertex makes, on the clipped
     * vertex instead of the input one.  Its remaining branch -- raw
     * model-space glVertex3v16 -- cannot occur here: it needs a zbuffered,
     * non-decal, non-prim triangle, and a CPU-projected one of those has
     * already had zbuffered cleared by the caller. */
    if ((context_flags & NDS_RENDERER_VERTEX_CONTEXT_PRIM_DEPTH) != 0u)
    {
        ndsRendererHardwareClipVertex(&vertex->clip, projected_z);
    }
    else if ((context_flags & NDS_RENDERER_VERTEX_CONTEXT_DECAL_DEPTH) != 0u)
    {
        ndsRendererHardwareClipVertex(
            &vertex->clip,
            vertex->clip.z - NDS_RENDERER_HW_DECAL_DEPTH_BIAS);
    }
    else if ((context_flags &
              NDS_RENDERER_VERTEX_CONTEXT_SOURCE_CLIP_DEPTH) != 0u)
    {
        ndsRendererHardwareClipVertex(&vertex->clip, vertex->clip.z);
    }
    else
    {
        ndsRendererHardwareClipVertexNdcDepth(&vertex->clip, projected_z);
    }
}

/* The source RSP clips at the near plane before the perspective divide, so a
 * triangle that straddles it still draws its front part.  The port used to
 * drop the whole triangle here, which is invisible in the automated capture
 * (the fixed camera never crosses the plane) but opens holes in geometry the
 * player can push into the camera -- BUGS.md #10.  Clip it the way the native
 * stage path already does, reusing that path's proven clipper. */
static u32 __attribute__((noinline, cold, optimize("Os")))
ndsRendererHardwareSubmitNearClippedTriangle(
    NDSRendererStats *stats,
    NDSRendererTraversalState *state,
    u32 i0, u32 i1, u32 i2,
    s32 projected_z)
{
    NDSRendererProjectedClipVertex input[3];
    NDSRendererProjectedClipVertex clipped[4];
    u32 corner_index[3];
    u32 context_flags = state->texture_prepare_vertex_flags;
    u32 corner;
    u32 clipped_count;
    u32 fan_index;
    u32 emitted;

    corner_index[0] = i0;
    corner_index[1] = i1;
    corner_index[2] = i2;
    for (corner = 0u; corner < 3u; corner++)
    {
        u32 vertex_index = corner_index[corner];
        u32 vertex_mask = 1u << vertex_index;
        const NDSRendererInputVertex *vtx =
            &state->input_vertices[vertex_index];

        input[corner].clip = state->vertices[vertex_index];
        if ((state->prepared_vertex_color_valid_mask & vertex_mask) != 0u)
        {
            input[corner].packed_color =
                state->prepared_vertex_colors[vertex_index];
        }
        else
        {
            input[corner].packed_color = ndsRendererHardwarePackedVertexColor(
                stats, vtx, state->texture_prepare_material_color,
                (s32)(context_flags &
                      NDS_RENDERER_VERTEX_CONTEXT_USE_MATERIAL),
                (s32)(context_flags &
                      NDS_RENDERER_VERTEX_CONTEXT_USE_VERTEX),
                state->vertex_colors[vertex_index],
                ((state->vertex_color_valid_mask & vertex_mask) != 0u) ?
                    TRUE : FALSE,
                state->color_modulate);
        }
        if ((state->prepared_texcoord_valid_mask & vertex_mask) != 0u)
        {
            input[corner].s = state->prepared_texcoord_s[vertex_index];
            input[corner].t = state->prepared_texcoord_t[vertex_index];
        }
        else
        {
            input[corner].s = ndsRendererHardwareTexCoord(
                vtx->s, state->texture_prepare_scale_s,
                state->texture_prepare_origin_s,
                state->texture_prepare_offset);
            input[corner].t = ndsRendererHardwareTexCoord(
                vtx->t, state->texture_prepare_scale_t,
                state->texture_prepare_origin_t,
                state->texture_prepare_offset);
        }
    }
    clipped_count = ndsRendererHardwareClipTriangleNearPlane(input, clipped);
    if (clipped_count < 3u)
    {
        /* Wholly behind the plane: nothing to draw, and the source would not
         * have drawn it either. */
        return 0u;
    }
    emitted = 0u;
    for (fan_index = 1u; fan_index + 1u < clipped_count; fan_index++)
    {
        /* The vertex emitters drop a w==0 vertex silently, which would leave
         * this triangle two vertices long and shift every later triangle in
         * the batch.  The old near-plane reject made that unreachable by
         * requiring w>0 on all three corners; keep it unreachable. */
        if ((clipped[0].clip.w == 0) ||
            (clipped[fan_index].clip.w == 0) ||
            (clipped[fan_index + 1u].clip.w == 0))
        {
            continue;
        }
        ndsRendererHardwareEmitClippedVertex(
            &clipped[0], context_flags, projected_z);
        ndsRendererHardwareEmitClippedVertex(
            &clipped[fan_index], context_flags, projected_z);
        ndsRendererHardwareEmitClippedVertex(
            &clipped[fan_index + 1u], context_flags, projected_z);
        emitted++;
    }
    return emitted;
}

static s32 ndsRendererHardwareTriangleInsideNearPlane(
    const NDSRendererClipVertex20p12 *v0,
    const NDSRendererClipVertex20p12 *v1,
    const NDSRendererClipVertex20p12 *v2)
{
    if ((v0 == NULL) || (v1 == NULL) || (v2 == NULL))
    {
        return FALSE;
    }
    return ((ndsRendererHardwareClipZWInsideNearPlane(v0->z, v0->w) != FALSE) &&
            (ndsRendererHardwareClipZWInsideNearPlane(v1->z, v1->w) != FALSE) &&
            (ndsRendererHardwareClipZWInsideNearPlane(v2->z, v2->w) != FALSE)) ?
        TRUE : FALSE;
}

static s32 ndsRendererHardwareRawMatrixCompatible(
    const NDSRendererTraversalState *state)
{
    return ((state != NULL) && (state->matrix_valid != 0u) &&
            (state->matrix_generation != 0u)) ? TRUE : FALSE;
}

/* The bin macro is defined by the G3 capture block above, which is itself
 * nested inside the GX-record configuration guard. This classifier is not, so
 * the fallback has to be unconditional rather than an #else on that block --
 * otherwise a configuration that compiles the classifier without the capture
 * fails at the first bin call rather than simply not counting. */

static NDSRendererHWSubmitClass ndsRendererHardwareClassifySubmit(
    const NDSRendererTraversalState *state,
    u32 i0, u32 i1, u32 i2,
    s32 source_zbuffered, s32 decal_depth, s32 prim_depth,
    u32 *out_snapshot_id)
{
    u32 mask;
    u32 snapshot_id;

    if (out_snapshot_id != NULL)
    {
        *out_snapshot_id = NDS_RENDERER_MATRIX_SNAPSHOT_INVALID;
    }

    if (source_zbuffered == FALSE)
    {
        NDS_EFFECT_SUBMIT_BIN(0u);
        return NDS_RENDERER_HW_SUBMIT_PROJECTED_NO_Z;
    }
    if (decal_depth != FALSE)
    {
        NDS_EFFECT_SUBMIT_BIN(1u);
        return NDS_RENDERER_HW_SUBMIT_PROJECTED_DECAL;
    }
    if (prim_depth != FALSE)
    {
        NDS_EFFECT_SUBMIT_BIN(2u);
        return NDS_RENDERER_HW_SUBMIT_PROJECTED_PRIM_DEPTH;
    }
    mask = (1u << i0) | (1u << i1) | (1u << i2);
    if ((state->raw_vertex_fit_mask & mask) != mask)
    {
        ndsRendererProfileRecordRawCurrentRangeReject();
        NDS_EFFECT_SUBMIT_BIN(3u);
        return NDS_RENDERER_HW_SUBMIT_PROJECTED_RANGE_OR_MATRIX;
    }

    if ((state->current_transform_vertex_mask & mask) == mask)
    {
        if (ndsRendererHardwareRawMatrixCompatible(state) == FALSE)
        {
            /* Bin 4, NOT bin 3: both returns are the same enum value, so the
             * class alone cannot say whether the raw path was refused because
             * the vertices did not fit the v16 range or because the matrix was
             * unusable. Those are different repairs, so they get different
             * bins. */
            NDS_EFFECT_SUBMIT_BIN(4u);
            return NDS_RENDERER_HW_SUBMIT_PROJECTED_RANGE_OR_MATRIX;
        }
        ndsRendererProfileRecordRawCurrentCandidate();
        NDS_EFFECT_SUBMIT_BIN(5u);
        return NDS_RENDERER_HW_SUBMIT_RAW_Z_CURRENT_MATRIX;
    }

    snapshot_id = state->vertex_matrix_snapshot[i0];
    if ((snapshot_id != NDS_RENDERER_MATRIX_SNAPSHOT_INVALID) &&
        (state->vertex_matrix_snapshot[i1] == snapshot_id) &&
        (state->vertex_matrix_snapshot[i2] == snapshot_id) &&
        (ndsRendererGetMatrixSnapshot(state, snapshot_id) != NULL))
    {
        if (out_snapshot_id != NULL)
        {
            *out_snapshot_id = snapshot_id;
        }
        NDS_EFFECT_SUBMIT_BIN(6u);
        return NDS_RENDERER_HW_SUBMIT_RAW_Z_SNAPSHOT_MATRIX;
    }

    ndsRendererProfileRecordRawCrossMatrix();
    NDS_EFFECT_SUBMIT_BIN(7u);
    return NDS_RENDERER_HW_SUBMIT_PROJECTED_CROSS_MATRIX;
}

/* The c200 canonical census never takes either raw-matrix load arm or the
 * z-buffer accounting arm below.  Keep those exact generic-renderer paths
 * available in main RAM while leaving the projected/no-Z path contiguous. */
static void __attribute__((noinline, cold))
ndsRendererSubmitHardwareTriangleRawMatrixCold(
    NDSRendererTraversalState *state,
    NDSRendererHWSubmitClass submit_class,
    const NDSRendererMatrixSnapshot *raw_snapshot,
    s32 scale_world)
{
    if (submit_class == NDS_RENDERER_HW_SUBMIT_RAW_Z_CURRENT_MATRIX)
    {
        ndsRendererLoadHardwareMatrices(state, scale_world);
    }
    else if (raw_snapshot != NULL)
    {
        ndsRendererLoadHardwareRawComposedMatrix(
            &raw_snapshot->matrix, raw_snapshot->generation);
    }
}

static void __attribute__((noinline, cold))
ndsRendererSubmitHardwareTriangleDepthStatsCold(
    NDSRendererStats *stats, s32 decal_depth, s32 prim_depth)
{
    stats->hardware_zbuffer_triangle_count++;
    if (decal_depth != FALSE)
    {
        stats->hardware_decal_depth_triangle_count++;
    }
    if (prim_depth != FALSE)
    {
        stats->hardware_prim_depth_triangle_count++;
    }
}

static void NDS_R2_ITCM_PACK2_EVICTED_CODE
ndsRendererSubmitHardwareTriangle(
    NDSRendererStats *stats,
    const NDSRendererConfig *config,
    NDSRendererTraversalState *state,
    u32 packed)
{
    u32 i0;
    u32 i1;
    u32 i2;
    const NDSRendererInputVertex *v0;
    const NDSRendererTileState *render_tile;
    s32 use_texture;
    s32 implicit_texture_on;
    u32 texture_scale_s;
    u32 texture_scale_t;
    u32 scale_world;
    u32 material_color;
    u32 poly_alpha;
    u32 poly_fmt;
    u32 texture_name;
    s32 use_material_color;
    s32 use_vertex_color;
    s32 texture_offset;
    s32 zbuffered;
    s32 source_zbuffered;
    s32 decal_depth;
    s32 prim_depth;
    s32 transformed_ready;
    s32 projected_submit;
    s32 raw_submit;
    s32 source_clip_depth;
    s32 no_oracle;
    s32 near_clipped = FALSE;
    u32 raw_snapshot_id = NDS_RENDERER_MATRIX_SNAPSHOT_INVALID;
    const NDSRendererMatrixSnapshot *raw_snapshot = NULL;
    NDSRendererHWSubmitClass submit_class;
    s32 projected_z[3] = { 0, 0, 0 };

    if (stats == NULL)
    {
        return;
    }
    if (ndsRendererInputTriangleReady(state, packed, &i0, &i1, &i2) == FALSE)
    {
        stats->hardware_oracle_reject_count++;
        ndsRendererProfileRecordSubmitClass(
            NDS_RENDERER_HW_SUBMIT_REJECT);
        return;
    }
    no_oracle = (sNdsRendererHardwareNoOracle != 0u) ? TRUE : FALSE;
    if (state->texture_prepare_valid != 0u)
    {
        source_zbuffered =
            (state->texture_prepare_source_zbuffered != 0u) ? TRUE : FALSE;
        zbuffered = source_zbuffered;
        decal_depth =
            (state->texture_prepare_decal_depth != 0u) ? TRUE : FALSE;
        prim_depth =
            (state->texture_prepare_prim_depth != 0u) ? TRUE : FALSE;
    }
    else
    {
        zbuffered =
            ((stats->geometry_mode & NDS_RENDERER_GEOM_ZBUFFER) != 0u) ?
                TRUE : FALSE;
        source_zbuffered = zbuffered;
        decal_depth = ((zbuffered != FALSE) &&
                       ((stats->othermode_l & NDS_RENDERER_ZMODE_DEC) ==
                        NDS_RENDERER_ZMODE_DEC)) ? TRUE : FALSE;
        prim_depth = ((zbuffered != FALSE) &&
                      (ndsRendererHardwareUsePrimDepth(stats) != FALSE)) ?
            TRUE : FALSE;
        state->texture_prepare_source_zbuffered =
            (source_zbuffered != FALSE) ? TRUE : FALSE;
        state->texture_prepare_decal_depth =
            (decal_depth != FALSE) ? TRUE : FALSE;
        state->texture_prepare_prim_depth =
            (prim_depth != FALSE) ? TRUE : FALSE;
    }
    v0 = &state->input_vertices[i0];
    submit_class = ndsRendererHardwareClassifySubmit(
        state, i0, i1, i2, source_zbuffered, decal_depth, prim_depth,
        &raw_snapshot_id);
    raw_submit =
        ((submit_class == NDS_RENDERER_HW_SUBMIT_RAW_Z_CURRENT_MATRIX) ||
         (submit_class == NDS_RENDERER_HW_SUBMIT_RAW_Z_SNAPSHOT_MATRIX)) ?
             TRUE : FALSE;
    projected_submit =
        ((source_zbuffered != FALSE) && (raw_submit == FALSE)) ? TRUE : FALSE;
    source_clip_depth = projected_submit;
    transformed_ready = TRUE;
    if (raw_submit == FALSE)
    {
        transformed_ready =
            ((ndsRendererEnsureTransformedVertex(stats, state, i0) != FALSE) &&
             (ndsRendererEnsureTransformedVertex(stats, state, i1) != FALSE) &&
             (ndsRendererEnsureTransformedVertex(stats, state, i2) != FALSE)) ?
                 TRUE : FALSE;
    }
    if (transformed_ready == FALSE)
    {
        stats->hardware_oracle_reject_count++;
        ndsRendererProfileRecordSubmitClass(
            NDS_RENDERER_HW_SUBMIT_REJECT);
        return;
    }
    if ((raw_submit == FALSE) &&
        (ndsRendererHardwareTriangleInsideNearPlane(
             &state->vertices[i0], &state->vertices[i1],
             &state->vertices[i2]) == FALSE))
    {
        /* The source RSP clips before the perspective divide, so a triangle
         * that straddles its near plane still draws its front part.  Clip it
         * the same way rather than dropping it (BUGS.md #10); emitting the
         * raw post-divide vertices is what would create a screen-spanning
         * primitive, and the clipper never does that. */
        near_clipped = TRUE;
    }
    if (no_oracle == FALSE)
    {
        stats->hardware_oracle_triangle_count++;
    }
    if (raw_snapshot_id != NDS_RENDERER_MATRIX_SNAPSHOT_INVALID)
    {
        raw_snapshot = ndsRendererGetMatrixSnapshot(state, raw_snapshot_id);
    }
    if (state->texture_prepare_valid == 0u)
    {
        material_color = ndsRendererHardwareColorSource(stats);
        use_material_color = ndsRendererHardwareUseMaterialColor(stats);
        use_vertex_color = ndsRendererHardwareUseVertexColor(stats);
        state->texture_prepare_material_color = material_color;
        state->texture_prepare_vertex_flags =
            ((use_material_color != FALSE) ?
                 NDS_RENDERER_VERTEX_CONTEXT_USE_MATERIAL : 0u) |
            ((use_vertex_color != FALSE) ?
                 NDS_RENDERER_VERTEX_CONTEXT_USE_VERTEX : 0u);
    }
    if (state->texture_prepare_valid == 0u)
    {
        poly_alpha = ndsRendererHardwareAlpha(stats, v0);
        state->texture_prepare_alpha_constant =
            (ndsRendererHardwareAlphaUsesVertex(stats) == FALSE) ?
                TRUE : FALSE;
        if (state->texture_prepare_alpha_constant != 0u)
        {
            state->texture_prepare_poly_alpha = poly_alpha;
            state->texture_prepare_poly_fmt =
                ndsRendererHardwarePolyFmt(stats, poly_alpha);
        }
    }
    else if (state->texture_prepare_alpha_constant != 0u)
    {
        poly_alpha = state->texture_prepare_poly_alpha;
    }
    else
    {
        poly_alpha = ndsRendererHardwareAlpha(stats, v0);
    }
    if (poly_alpha == 0u)
    {
        return;
    }
    ndsRendererProfileRecordSubmitClass(submit_class);
    if (projected_submit != FALSE)
    {
        ndsRendererProfileRecordProjectedSubmit();
    }
    if ((raw_submit == FALSE) &&
        (submit_class != NDS_RENDERER_HW_SUBMIT_PROJECTED_DECAL) &&
        (submit_class != NDS_RENDERER_HW_SUBMIT_PROJECTED_PRIM_DEPTH))
    {
        zbuffered = FALSE;
        decal_depth = FALSE;
        prim_depth = FALSE;
    }
    scale_world = (raw_submit != FALSE) ? TRUE : FALSE;
    if (state->texture_prepare_valid == 0u)
    {
        render_tile =
            &stats->texture_tiles[ndsRendererActiveTextureTile(stats)];
        implicit_texture_on =
            ndsRendererHardwareTextureImplicitStateOn(stats);
        use_texture = (ndsRendererHardwareUseTexture(stats) != FALSE) ?
            ndsRendererHardwareBindTexture(stats, config, state) : FALSE;
        state->texture_prepare_valid = TRUE;
        state->texture_prepare_enabled =
            (use_texture != FALSE) ? TRUE : FALSE;
        state->texture_prepare_name = (use_texture != FALSE) ?
            sNdsRendererHardwareBoundTextureName : 0u;
        ndsRendererProfileRecordTexturePrepare();

        texture_scale_s = stats->texture_scale_s;
        texture_scale_t = stats->texture_scale_t;
        if ((use_texture != FALSE) && (implicit_texture_on != FALSE))
        {
            if ((stats->texture_state_flags &
                 NDS_RENDERER_TEXTURE_STATE_SCALE_S) == 0u)
            {
                texture_scale_s = NDS_RENDERER_HW_IMPLICIT_TEXTURE_SCALE;
            }
            if ((stats->texture_state_flags &
                 NDS_RENDERER_TEXTURE_STATE_SCALE_T) == 0u)
            {
                texture_scale_t = NDS_RENDERER_HW_IMPLICIT_TEXTURE_SCALE;
            }
        }
        texture_offset = ndsRendererHardwareTextureFilterOffset(stats);
        state->texture_prepare_scale_s = texture_scale_s;
        state->texture_prepare_scale_t = texture_scale_t;
        state->texture_prepare_origin_s = render_tile->uls;
        state->texture_prepare_origin_t = render_tile->ult;
        state->texture_prepare_offset = texture_offset;
        if (use_texture != FALSE)
        {
            state->texture_prepare_vertex_flags |=
                NDS_RENDERER_VERTEX_CONTEXT_USE_TEXTURE;
        }
    }
    else
    {
        use_texture =
            (state->texture_prepare_enabled != 0u) ? TRUE : FALSE;
        ndsRendererProfileRecordTexturePrepareReuse();
    }
    if (submit_class == NDS_RENDERER_HW_SUBMIT_RAW_Z_CURRENT_MATRIX)
    {
        ndsRendererSubmitHardwareTriangleRawMatrixCold(
            state, submit_class, raw_snapshot, scale_world);
    }
    else if (raw_snapshot != NULL)
    {
        ndsRendererSubmitHardwareTriangleRawMatrixCold(
            state, submit_class, raw_snapshot, scale_world);
    }
    else
    {
        ndsRendererLoadHardwareMatrices(NULL, FALSE);
    }
    if (prim_depth != FALSE)
    {
        projected_z[0] = projected_z[1] = projected_z[2] =
            (s32)(stats->prim_depth & 0xffffu);
    }
    else if (source_zbuffered != FALSE)
    {
        /* Source-Z projected submissions use the composed clip Z below and
         * must not consume the synthetic no-Z painter counter. */
        projected_z[0] = projected_z[1] = projected_z[2] = 0;
    }
    else
    {
        projected_z[0] = projected_z[1] = projected_z[2] =
            ndsRendererHardwareNextProjectedDepth();
    }
    poly_fmt = (state->texture_prepare_alpha_constant != 0u) ?
        state->texture_prepare_poly_fmt :
        ndsRendererHardwarePolyFmt(stats, poly_alpha);
    texture_name = state->texture_prepare_name;
    ndsRendererHardwareBeginTriangleBatch(
        stats, (use_texture != FALSE) ? TRUE : FALSE,
        texture_name, poly_fmt, sNdsRendererHardwareMatrixMode,
        sNdsRendererHardwareMatrixGeneration);
    state->texture_prepare_vertex_flags =
        (state->texture_prepare_vertex_flags &
         NDS_RENDERER_VERTEX_CONTEXT_PREPARED_MASK) |
        ((scale_world != FALSE) ?
             NDS_RENDERER_VERTEX_CONTEXT_SCALE_WORLD : 0u) |
        ((zbuffered != FALSE) ?
             NDS_RENDERER_VERTEX_CONTEXT_ZBUFFERED : 0u) |
        ((decal_depth != FALSE) ?
             NDS_RENDERER_VERTEX_CONTEXT_DECAL_DEPTH : 0u) |
        ((prim_depth != FALSE) ?
             NDS_RENDERER_VERTEX_CONTEXT_PRIM_DEPTH : 0u) |
        ((source_clip_depth != FALSE) ?
             NDS_RENDERER_VERTEX_CONTEXT_SOURCE_CLIP_DEPTH : 0u);
    if (near_clipped != FALSE)
    {
        if (ndsRendererHardwareSubmitNearClippedTriangle(
                stats, state, i0, i1, i2, projected_z[0]) == 0u)
        {
            ndsRendererProfileRecordNearPlaneTriangleReject();
        }
    }
    else
    {
    ndsRendererHardwareSubmitVertex(stats, state, i0, projected_z[0]
                                    );
    ndsRendererHardwareSubmitVertex(stats, state, i1, projected_z[1]
                                    );
    ndsRendererHardwareSubmitVertex(stats, state, i2, projected_z[2]
                                    );
    }
    if (source_zbuffered != FALSE)
    {
        ndsRendererHardwareEnterProjectedForeground();
    }

    sNdsRendererHardwareSubmitted = TRUE;
    stats->hardware_triangle_count++;
    stats->hardware_vertex_count += 3u;
    ndsRendererProfileRecordHardwareTriangle();
    if (zbuffered != FALSE)
    {
        ndsRendererSubmitHardwareTriangleDepthStatsCold(
            stats, decal_depth, prim_depth);
    }
    else
    {
        stats->hardware_projected_depth_triangle_count++;
    }
}
