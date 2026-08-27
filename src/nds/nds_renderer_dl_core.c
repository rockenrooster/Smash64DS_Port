
#if NDS_RENDERER_SCREEN_SPACE_CENSUS
volatile NDSRendererScreenSpaceCensusRow
    gNdsRendererScreenSpaceCensus[NDS_RENDERER_PROFILE_OWNER_COUNT]
                                   [NDS_RENDERER_SCREEN_SPACE_CENSUS_PART_COUNT];
volatile u64 gNdsRendererScreenSpaceStageOwnerTicks[
    NDS_RENDERER_SCREEN_SPACE_CENSUS_STAGE_OWNER_COUNT];
volatile u32 gNdsRendererScreenSpaceCensusArmed;
volatile u32 gNdsRendererScreenSpaceCensusResetRequested;
volatile u32 gNdsRendererScreenSpaceCensusFrameCount;
volatile u32 gNdsRendererScreenSpaceCensusOverflowCount;
#endif

#if NDS_RENDER_ECONOMY
volatile u32 gNdsRendererEconomyConfiguredOwnerMask =
    (u32)NDS_RENDER_ECONOMY_OWNER_MASK;
volatile u32 gNdsRendererEconomyActiveOwnerMask;
volatile u32 gNdsRendererEconomyAppliedOwnerMask;
volatile u32 gNdsRendererEconomySkippedRunCount;
volatile u32 gNdsRendererEconomySkippedTriangleCount;
#endif

#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL >= 2)
typedef struct NDSRendererHardwarePendingPosTest
{
    NDSRendererMatrix20p12 matrix;
    NDSRendererInputVertex input;
    NDSRendererClipVertex20p12 clip;
    u32 generation;
    u32 matrix_word;
} NDSRendererHardwarePendingPosTest;

static NDSRendererHardwarePendingPosTest
    sNdsRendererHardwarePendingPosTests[NDS_RENDERER_HW_POS_TEST_MAX];
static u32 sNdsRendererHardwarePendingPosTestCount;
static u32 sNdsRendererHardwarePendingPosTestLastGeneration;
#endif

#if NDS_RENDERER_HW_TRIANGLES
static void ndsRendererHardwarePrepareLitDirection(
    const NDSRendererStats *stats,
    const NDSRendererMatrix20p12 *modelview,
    NDSRendererHardwareLightDirection *out);
static u32 ndsRendererHardwareLitShadeColorPrepared(
    NDSRendererStats *stats,
    const NDSRendererInputVertex *vtx,
    const NDSRendererHardwareLightDirection *direction);
#if NDS_RENDERER_PROFILE_LEVEL < 2
static const u32 *ndsRendererHardwareFindLightShadeLut(
    u32 diffuse, u32 ambient);
/* 2026-08-16 ITCM reclaim: this is the LUT BUILDER, reached only on a cache
 * miss; ndsRendererHardwareFindLightShadeLut is the lookup and stays where it
 * is. The shade-LUT set stabilises before the gate window, so the builder
 * executes ZERO instructions across frames 439-2038 (per-PC census whole-match
 * column) while holding 404 bytes of zero-wait ITCM. Moved to .main so
 * ndsR2AnimValueQ -- 370.6 entries a frame, 21,719 tk/fr of instruction fetch
 * -- can have the space. Placement only; the body is untouched. */
static const u32 *
ndsRendererHardwareGetLightShadeLut(
    u32 diffuse, u32 ambient);
#endif
static u32 ndsRendererHardwareLitShadeColor(
    NDSRendererStats *stats,
    const NDSRendererInputVertex *vtx,
    const NDSRendererMatrix20p12 *modelview);
#endif

static s32 ndsRendererClampS64ToS32(s64 value)
{
    if (value > (s64)INT_MAX)
    {
        return INT_MAX;
    }
    if (value < (s64)INT_MIN)
    {
        return INT_MIN;
    }
    return (s32)value;
}

static s32 ndsRendererRoundShiftS32(s32 value, u32 shift)
{
    s64 wide;
    s64 bias;

    if (shift == 0)
    {
        return value;
    }

    wide = value;
    bias = (s64)(1u << (shift - 1u));
    if (wide < 0)
    {
        return (s32)(-(((-wide) + bias) >> shift));
    }
    return (s32)((wide + bias) >> shift);
}

static s64 ndsRendererRoundShiftS64(s64 value, u32 shift)
{
    s64 bias;

    if (shift == 0)
    {
        return value;
    }

    bias = (s64)(1u << (shift - 1u));
    if (value < 0)
    {
        return -(((-value) + bias) >> shift);
    }
    return (value + bias) >> shift;
}

s32 ndsRendererMtxCellS16p16(const Mtx *mtx, u32 row, u32 col)
{
    const u32 *ai;
    const u32 *af;
    u32 pair;
    u32 hi;
    u32 lo;

    if ((mtx == NULL) || (row >= 4u) || (col >= 4u))
    {
        return 0;
    }

    ai = (const u32 *)&mtx->m[0][0];
    af = (const u32 *)&mtx->m[2][0];
    pair = (row * 2u) + (col / 2u);
    hi = ai[pair];
    lo = af[pair];

    if ((col & 1u) == 0)
    {
        return (s32)((hi & 0xffff0000u) | ((lo >> 16) & 0xffffu));
    }
    return (s32)(((hi << 16) & 0xffff0000u) | (lo & 0xffffu));
}

static s32 ndsRendererMtxCell20p12ToS16p16(s32 value)
{
    return ndsRendererClampS64ToS32(
        (s64)value << (NDS_RENDERER_N64_MTX_FRAC_BITS -
                       NDS_RENDERER_DS_MTX_FRAC_BITS));
}

static void ndsRendererMtxStoreCellS16p16(Mtx *mtx, u32 row, u32 col,
                                          s32 value)
{
    u32 *ai;
    u32 *af;
    u32 pair;
    u32 ui;

    if ((mtx == NULL) || (row >= 4u) || (col >= 4u))
    {
        return;
    }

    ai = (u32 *)&mtx->m[0][0];
    af = (u32 *)&mtx->m[2][0];
    pair = (row * 2u) + (col / 2u);
    ui = (u32)value;
    if ((col & 1u) == 0)
    {
        ai[pair] = (ai[pair] & 0x0000ffffu) | (ui & 0xffff0000u);
        af[pair] = (af[pair] & 0x0000ffffu) |
            ((ui << 16) & 0xffff0000u);
    }
    else
    {
        ai[pair] = (ai[pair] & 0xffff0000u) | ((ui >> 16) & 0xffffu);
        af[pair] = (af[pair] & 0xffff0000u) | (ui & 0xffffu);
    }
}

static void ndsRendererMtxStoreDS20p12ToN64(
    const NDSRendererMatrix20p12 *src, Mtx *dst)
{
    u32 row;
    u32 col;

    if (dst == NULL)
    {
        return;
    }

    memset(dst, 0, sizeof(*dst));
    if (src == NULL)
    {
        return;
    }

    for (row = 0; row < 4u; row++)
    {
        for (col = 0; col < 4u; col++)
        {
            ndsRendererMtxStoreCellS16p16(
                dst, row, col,
                ndsRendererMtxCell20p12ToS16p16(src->m[row][col]));
        }
    }
}

void NDS_FIGHTER_PACKET_EVICT(NDS_TASK82_ITCM_CODE)
ndsRendererMtxLoadN64ToDS20p12(const Mtx *src,
                                    NDSRendererMatrix20p12 *dst)
{
    u32 row;
    u32 col;
    const u32 shift =
        NDS_RENDERER_N64_MTX_FRAC_BITS - NDS_RENDERER_DS_MTX_FRAC_BITS;

    if (dst == NULL)
    {
        return;
    }

    memset(dst, 0, sizeof(*dst));
    if (src == NULL)
    {
        return;
    }

    for (row = 0; row < 4u; row++)
    {
        for (col = 0; col < 4u; col++)
        {
            dst->m[row][col] =
                ndsRendererRoundShiftS32(
                    ndsRendererMtxCellS16p16(src, row, col), shift);
        }
    }
}

void ndsRendererTransformVertex20p12(const NDSRendererMatrix20p12 *mtx,
                                     const NDSRendererInputVertex *vtx,
                                     NDSRendererClipVertex20p12 *out)
{
    /* s32, not s64. The source fields are s16 and the matrix is s32, so every
     * product here fits in 47 bits and SMULL computes it exactly; widening both
     * operands first only forced the full 64x64 helper. Same value, one
     * instruction instead of a call -- twelve times per vertex. */
    s32 x;
    s32 y;
    s32 z;

    if ((mtx == NULL) || (vtx == NULL) || (out == NULL))
    {
        return;
    }

    x = vtx->x;
    y = vtx->y;
    z = vtx->z;

    out->x = ndsRendererClampS64ToS32(
        (s64)mtx->m[0][0] * x + (s64)mtx->m[1][0] * y +
        (s64)mtx->m[2][0] * z + mtx->m[3][0]);
    out->y = ndsRendererClampS64ToS32(
        (s64)mtx->m[0][1] * x + (s64)mtx->m[1][1] * y +
        (s64)mtx->m[2][1] * z + mtx->m[3][1]);
    out->z = ndsRendererClampS64ToS32(
        (s64)mtx->m[0][2] * x + (s64)mtx->m[1][2] * y +
        (s64)mtx->m[2][2] * z + mtx->m[3][2]);
    out->w = ndsRendererClampS64ToS32(
        (s64)mtx->m[0][3] * x + (s64)mtx->m[1][3] * y +
        (s64)mtx->m[2][3] * z + mtx->m[3][3]);
}

void NDS_TASK82_ITCM_CODE
ndsRendererMtxMul20p12(const NDSRendererMatrix20p12 *lhs,
                            const NDSRendererMatrix20p12 *rhs,
                            NDSRendererMatrix20p12 *out)
{
    NDSRendererMatrix20p12 temp;
    u32 row;
    u32 col;
    u32 k;

    if ((lhs == NULL) || (rhs == NULL) || (out == NULL))
    {
        return;
    }

    for (row = 0; row < 4u; row++)
    {
        for (col = 0; col < 4u; col++)
        {
            s64 sum = 0;

            for (k = 0; k < 4u; k++)
            {
                sum += (s64)lhs->m[row][k] * rhs->m[k][col];
            }
            temp.m[row][col] = ndsRendererClampS64ToS32(
                ndsRendererRoundShiftS64(sum, NDS_RENDERER_DS_MTX_FRAC_BITS));
        }
    }

    *out = temp;
}

void NDS_R2_ITCM_PACK2_EVICTED_PLAIN_CODE
ndsRendererMtxMulAffine20p12(const NDSRendererMatrix20p12 *lhs,
                                  const NDSRendererMatrix20p12 *rhs,
                                  NDSRendererMatrix20p12 *out)
{
    NDSRendererMatrix20p12 temp;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    NDSRendererMatrix20p12 oracle;
    u32 mismatch = FALSE;
#endif
    u32 row;
    u32 col;

    if ((lhs == NULL) || (rhs == NULL) || (out == NULL))
    {
        return;
    }
    if ((lhs->m[0][3] != 0) || (lhs->m[1][3] != 0) ||
        (lhs->m[2][3] != 0) ||
        (lhs->m[3][3] != (1 << NDS_RENDERER_DS_MTX_FRAC_BITS)) ||
        (rhs->m[0][3] != 0) || (rhs->m[1][3] != 0) ||
        (rhs->m[2][3] != 0) ||
        (rhs->m[3][3] != (1 << NDS_RENDERER_DS_MTX_FRAC_BITS)))
    {
        ndsRendererMtxMul20p12(lhs, rhs, out);
        return;
    }

    for (row = 0u; row < 3u; row++)
    {
        for (col = 0u; col < 3u; col++)
        {
            s64 sum = (s64)lhs->m[row][0] * rhs->m[0][col] +
                (s64)lhs->m[row][1] * rhs->m[1][col] +
                (s64)lhs->m[row][2] * rhs->m[2][col];

            temp.m[row][col] = ndsRendererClampS64ToS32(
                ndsRendererRoundShiftS64(
                    sum, NDS_RENDERER_DS_MTX_FRAC_BITS));
        }
        temp.m[row][3] = 0;
    }
    for (col = 0u; col < 3u; col++)
    {
        s64 sum = (s64)lhs->m[3][0] * rhs->m[0][col] +
            (s64)lhs->m[3][1] * rhs->m[1][col] +
            (s64)lhs->m[3][2] * rhs->m[2][col] +
            (s64)lhs->m[3][3] * rhs->m[3][col];

        temp.m[3][col] = ndsRendererClampS64ToS32(
            ndsRendererRoundShiftS64(sum,
                                     NDS_RENDERER_DS_MTX_FRAC_BITS));
    }
    temp.m[3][3] = 1 << NDS_RENDERER_DS_MTX_FRAC_BITS;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    ndsRendererMtxMul20p12(lhs, rhs, &oracle);
    gNdsRendererProfileAffineMatrixSamples++;
    for (row = 0u; row < 4u; row++)
    {
        for (col = 0u; col < 4u; col++)
        {
            s64 delta = (s64)temp.m[row][col] - oracle.m[row][col];
            u32 magnitude;

            if (delta < 0)
            {
                delta = -delta;
            }
            magnitude = (delta > (s64)UINT_MAX) ? UINT_MAX : (u32)delta;
            if (magnitude != 0u)
            {
                mismatch = TRUE;
                if (magnitude > gNdsRendererProfileAffineMatrixMaxDelta)
                {
                    gNdsRendererProfileAffineMatrixMaxDelta = magnitude;
                }
            }
        }
    }
    if (mismatch != FALSE)
    {
        gNdsRendererProfileAffineMatrixMismatches++;
    }
#endif
    *out = temp;
}

static void ndsRendererMtxIdentity20p12(NDSRendererMatrix20p12 *out)
{
    if (out == NULL)
    {
        return;
    }

    /* R2-03 E69. See the header helper: memset plus a diagonal loop was a
     * library call to write twelve zeros. */
    ndsRendererMatrixIdentity20p12(out, 1 << NDS_RENDERER_DS_MTX_FRAC_BITS);
}

#if NDS_RENDERER_HW_TRIANGLES
static u32 ndsRendererNextMatrixGeneration(void)
{
    sNdsRendererMatrixGenerationSerial++;
    if (sNdsRendererMatrixGenerationSerial == 0u)
    {
        sNdsRendererMatrixGenerationSerial = 1u;
        sNdsRendererHardwareMatrixLoaded = FALSE;
        sNdsRendererHardwareMatrixMode = NDS_RENDERER_HW_MATRIX_MODE_NONE;
        sNdsRendererHardwareMatrixGeneration = 0u;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        sNdsRendererHardwareMatrixSignature = 0u;
#endif
    }
    return sNdsRendererMatrixGenerationSerial;
}
#endif

static u32 ndsRendererReadU32(const void *ptr)
{
    const u8 *bytes = ptr;

    return (u32)bytes[0] |
           ((u32)bytes[1] << 8) |
           ((u32)bytes[2] << 16) |
           ((u32)bytes[3] << 24);
}

typedef u32 NDSRendererAliasedU32 __attribute__((__may_alias__));

static void ndsRendererDecodeInputVertex(NDSRendererInputVertex *dst,
                                         const void *src)
{
    u32 xy;
    u32 zf;
    u32 st;
    u32 rgba;

    if ((dst == NULL) || (src == NULL))
    {
        return;
    }

    /* DS is little-endian, so an aligned may-alias word load is exactly the
     * bytewise payload decode below. Retain that fallback for arbitrary DLs. */
    if ((((uintptr_t)src) & 3u) == 0u)
    {
        const NDSRendererAliasedU32 *words =
            (const NDSRendererAliasedU32 *)src;

        xy = words[0];
        zf = words[1];
        st = words[2];
        rgba = words[3];
    }
    else
    {
        xy = ndsRendererReadU32(src);
        zf = ndsRendererReadU32((const u8 *)src + 4);
        st = ndsRendererReadU32((const u8 *)src + 8);
        rgba = ndsRendererReadU32((const u8 *)src + 12);
    }
    dst->x = (s16)(xy >> 16);
    dst->y = (s16)(xy & 0xffffu);
    dst->z = (s16)(zf >> 16);
    dst->s = (s16)(st >> 16);
    dst->t = (s16)(st & 0xffffu);
    dst->r = (u8)(rgba >> 24);
    dst->g = (u8)(rgba >> 16);
    dst->b = (u8)(rgba >> 8);
    dst->a = (u8)rgba;
    if (dst->a == 0u)
    {
        dst->a = 0xffu;
    }
}

static s32 ndsRendererValidateCommand(const Gfx *dl,
                                       const NDSRendererConfig *config)
{
    uintptr_t addr = (uintptr_t)dl;

#if NDS_RENDERER_PROFILE_LEVEL >= 2
    sNdsRendererProfileValidatedCommandCount++;
#endif

    if ((dl == NULL) || ((addr & 0x3u) != 0))
    {
        return FALSE;
    }
    if (config->validate_range == NULL)
    {
        return TRUE;
    }
    return config->validate_range(dl, sizeof(*dl), config->user);
}

static const void *ndsRendererResolveDataPointer(
    const NDSRendererConfig *config, const void *ptr, size_t bytes)
{
    uintptr_t addr = (uintptr_t)ptr;

    if ((ptr == NULL) || ((addr & 0x3u) != 0))
    {
        return NULL;
    }
    if ((config != NULL) && (config->resolve_data != NULL))
    {
        return config->resolve_data(ptr, bytes, config->user);
    }
    if ((config != NULL) && (config->validate_range != NULL) &&
        (config->validate_range((const Gfx *)ptr, bytes, config->user) ==
         FALSE))
    {
        return NULL;
    }
    return ptr;
}

static void ndsRendererRecordUnsupported(NDSRendererStats *stats, u32 op)
{
    if (stats->unsupported_opcode == 0)
    {
        stats->unsupported_opcode = op;
    }
    stats->unsupported_command_count++;
}

static void ndsRendererRecordOtherMode(NDSRendererStats *stats,
                                       u32 op, u32 w0, u32 w1)
{
    u32 bits;
    u32 pos;
    u32 shift;
    u32 mask;

    NDS_RENDERER_RECORD_PROOF_ONLY(stats->state_command_count++);
    stats->othermode_command_count++;
    stats->ignored_state_command_count++;
    if (stats->first_othermode_opcode == 0)
    {
        stats->first_othermode_opcode = op;
        stats->first_othermode_w0 = w0;
        stats->first_othermode_w1 = w1;
    }

    if (op == NDS_RENDERER_OP_RDPSETOTHERMODE)
    {
        stats->othermode_h = w0 & 0x00ffffffu;
        stats->othermode_l = w1;
        return;
    }
    if ((op != NDS_RENDERER_OP_SETOTHERMODE_H) &&
        (op != NDS_RENDERER_OP_SETOTHERMODE_L))
    {
        return;
    }

    bits = (w0 & 0xffu) + 1u;
    pos = (w0 >> 8) & 0xffu;
    if ((bits > 32u) || (pos >= 32u) || ((bits + pos) > 32u))
    {
        return;
    }
    shift = 32u - pos - bits;
    mask = (bits >= 32u) ? 0xffffffffu : (((1u << bits) - 1u) << shift);
    if (op == NDS_RENDERER_OP_SETOTHERMODE_H)
    {
        stats->othermode_h = (stats->othermode_h & ~mask) | (w1 & mask);
    }
    else
    {
        stats->othermode_l = (stats->othermode_l & ~mask) | (w1 & mask);
    }
}

static void ndsRendererRecordPrimDepth(NDSRendererStats *stats, u32 w1)
{
    if (stats == NULL)
    {
        return;
    }

    stats->prim_depth = (w1 >> 16) & 0xffffu;
    stats->prim_depth_delta = w1 & 0xffffu;
    stats->prim_depth_command_count++;
    NDS_RENDERER_RECORD_PROOF_ONLY(stats->state_command_count++);
}

static void ndsRendererRecordCull(NDSRendererStats *stats, u32 w0, u32 w1)
{
    stats->cull_command_count++;
    NDS_RENDERER_RECORD_PROOF_ONLY(stats->skip_command_count++);
    if ((stats->first_cull_w0 == 0) && (stats->first_cull_w1 == 0))
    {
        stats->first_cull_w0 = w0;
        stats->first_cull_w1 = w1;
    }
}

static void ndsRendererSyncTextureTile(NDSRendererStats *stats);
#if NDS_TASK107_RENDER_STATE_CENSUS
static void ndsRendererTask107RecordTextureSync(
    NDSRendererStats *stats,
    u32 site);
#endif

static void NDS_R2_DELTA_PATH_CODE
ndsRendererRecordTextureState(NDSRendererStats *stats, u32 w0, u32 w1)
{
    if (stats == NULL)
    {
        return;
    }

    stats->texture_mask |= NDS_RENDERER_TEXTURE_TEXTURE;
    stats->texture_command_count++;
    stats->texture_xparam = (w0 >> 16) & 0xFFu;
    stats->texture_level = (w0 >> 11) & 0x7u;
    stats->texture_tile = (w0 >> 8) & 0x7u;
    stats->texture_on = (w0 >> 1) & 0x7Fu;
    stats->texture_scale_s = (w1 >> 16) & 0xFFFFu;
    stats->texture_scale_t = w1 & 0xFFFFu;
    stats->texture_state_flags = NDS_RENDERER_TEXTURE_STATE_SEEN;
    if (stats->texture_on != 0)
    {
        stats->texture_state_flags |= NDS_RENDERER_TEXTURE_STATE_ON;
    }
    if (stats->texture_scale_s != 0)
    {
        stats->texture_state_flags |= NDS_RENDERER_TEXTURE_STATE_SCALE_S;
    }
    if (stats->texture_scale_t != 0)
    {
        stats->texture_state_flags |= NDS_RENDERER_TEXTURE_STATE_SCALE_T;
    }
#if NDS_TASK107_RENDER_STATE_CENSUS
    ndsRendererTask107RecordTextureSync(stats, NDS_TASK107_SYNC_TEXTURE);
#endif
    ndsRendererSyncTextureTile(stats);
}

static u32 ndsRendererTileFlags(u32 cms, u32 cmt, u32 masks, u32 maskt)
{
    u32 flags = 0u;

    if ((cms & NDS_RENDERER_TX_CLAMP) != 0) { flags |= NDS_RENDERER_TILE_S_CLAMP; }
    if ((cms & NDS_RENDERER_TX_MIRROR) != 0) { flags |= NDS_RENDERER_TILE_S_MIRROR; }
    if (masks != 0u) { flags |= NDS_RENDERER_TILE_S_MASKED; }
    if ((cmt & NDS_RENDERER_TX_CLAMP) != 0) { flags |= NDS_RENDERER_TILE_T_CLAMP; }
    if ((cmt & NDS_RENDERER_TX_MIRROR) != 0) { flags |= NDS_RENDERER_TILE_T_MIRROR; }
    if (maskt != 0u) { flags |= NDS_RENDERER_TILE_T_MASKED; }
    return flags;
}

static u32 ndsRendererActiveTextureTile(const NDSRendererStats *stats)
{
    if ((stats != NULL) &&
        ((stats->texture_state_flags & NDS_RENDERER_TEXTURE_STATE_SEEN) != 0u))
    {
        return stats->texture_tile & 0x7u;
    }
    return NDS_RENDERER_RENDER_TILE;
}

#if NDS_TASK107_RENDER_STATE_CENSUS
static NDSRendererTask107SyncTrack *ndsRendererTask107FindSyncTrack(
    const NDSRendererStats *stats,
    s32 create)
{
    NDSRendererTask107SyncTrack *free_track = NULL;
    u32 i;

    for (i = 0u; i < NDS_TASK107_SYNC_TRACK_COUNT; i++)
    {
        NDSRendererTask107SyncTrack *track = &sNdsTask107SyncTrack[i];

        if (track->stats == stats)
        {
            return track;
        }
        if ((free_track == NULL) && (track->stats == NULL))
        {
            free_track = track;
        }
    }
    if ((create != FALSE) && (free_track != NULL))
    {
        free_track->stats = stats;
        free_track->valid_mask = 0u;
        return free_track;
    }
    if (create != FALSE)
    {
        gNdsTask107SyncTrackerOverflow++;
    }
    return NULL;
}

static void ndsRendererTask107ForgetSyncTrack(const NDSRendererStats *stats)
{
    NDSRendererTask107SyncTrack *track;

    if (stats == NULL)
    {
        return;
    }
    track = ndsRendererTask107FindSyncTrack(stats, FALSE);
    if (track != NULL)
    {
        track->stats = NULL;
        track->valid_mask = 0u;
    }
}

static void ndsRendererTask107RecordTextureSync(
    NDSRendererStats *stats,
    u32 site)
{
    NDSRendererTask107SyncTrack *track;
    const NDSRendererTileState *tile;
    u32 tile_index;
    u32 bit;
    u32 load_seen;

    if ((stats == NULL) || (site >= NDS_TASK107_SYNC_SITE_COUNT))
    {
        return;
    }
    gNdsTask107SyncCalls[site]++;
    tile_index = ndsRendererActiveTextureTile(stats);
    tile = &stats->texture_tiles[tile_index];
    load_seen = stats->texture_tiles[NDS_RENDERER_LOAD_TILE].set_seen;
    bit = 1u << tile_index;
    track = ndsRendererTask107FindSyncTrack(stats, TRUE);
    if (track == NULL)
    {
        return;
    }
    if (((track->valid_mask & bit) != 0u) &&
        (track->load_seen[tile_index] == load_seen) &&
        (memcmp(&track->tile[tile_index], tile, sizeof(*tile)) == 0))
    {
        gNdsTask107SyncUnchanged[site]++;
    }
    track->tile[tile_index] = *tile;
    track->load_seen[tile_index] = load_seen;
    track->valid_mask |= bit;
}
#endif

static void NDS_R2_DELTA_PATH_CODE
ndsRendererSyncTextureTile(NDSRendererStats *stats)
{
    u32 tile_index;
    const NDSRendererTileState *tile;
    u32 flags = 0u;

    if (stats == NULL)
    {
        return;
    }

    tile_index = ndsRendererActiveTextureTile(stats);
    /* The memo, and its exactness argument. Everything below is a pure function
     * of (tile_index, texture_tiles[tile_index], texture_tiles[LOAD_TILE]
     * .set_seen). texture_tile_write_serial is bumped by both writers of
     * texture_tiles[] whenever the write can reach either of those two inputs
     * for the LAST SYNCED tile, so serial equality proves both inputs are
     * unchanged, and index equality proves the last sync targeted this tile.
     * The initial state is exact too: a memset stats has serial 0 == 0, index
     * 0 == NDS_RENDERER_RENDER_TILE and all republished fields already 0. */
    if ((stats->texture_tile_sync_serial == stats->texture_tile_write_serial) &&
        (stats->texture_render_tile == tile_index))
    {
        NDS_R2_TILESYNC_COUNT_SKIP();
        if (NDS_R2_TILESYNC_MEMO_ON())
        {
            return;
        }
    }
    else
    {
        NDS_R2_TILESYNC_COUNT_RUN();
    }
    stats->texture_tile_sync_serial = stats->texture_tile_write_serial;
    tile = &stats->texture_tiles[tile_index];

    stats->texture_render_tile = tile_index;
    stats->texture_render_tile_format = tile->format;
    stats->texture_render_tile_size = tile->size;
    stats->texture_render_tile_line = tile->line;
    stats->texture_render_tile_tmem = tile->tmem;
    stats->texture_render_tile_palette = tile->palette;
    stats->texture_render_tile_cms = tile->cms;
    stats->texture_render_tile_cmt = tile->cmt;
    stats->texture_render_tile_masks = tile->masks;
    stats->texture_render_tile_maskt = tile->maskt;
    stats->texture_render_tile_shifts = tile->shifts;
    stats->texture_render_tile_shiftt = tile->shiftt;
    stats->texture_tile_size_tile = tile_index;
    stats->texture_tile_size_uls = tile->uls;
    stats->texture_tile_size_ult = tile->ult;
    stats->texture_tile_size_lrs = tile->lrs;
    stats->texture_tile_size_lrt = tile->lrt;
    stats->texture_tile_width = tile->width;
    stats->texture_tile_height = tile->height;
    if (tile->set_seen != 0u)
    {
        flags |= NDS_RENDERER_TILE_RENDER_SEEN | tile->flags;
    }
    if (stats->texture_tiles[NDS_RENDERER_LOAD_TILE].set_seen != 0u)
    {
        flags |= NDS_RENDERER_TILE_LOAD_SEEN;
    }
    stats->texture_render_tile_flags = flags;
}

static void NDS_R2_DELTA_PATH_CODE
ndsRendererRecordSetTile(NDSRendererStats *stats, u32 w0, u32 w1)
{
    u32 tile;
    u32 fmt;
    u32 siz;
    u32 line;
    u32 tmem;
    u32 palette;
    u32 cmt;
    u32 maskt;
    u32 shiftt;
    u32 cms;
    u32 masks;
    u32 shifts;
    NDSRendererTileState *tile_state;

    if (stats == NULL)
    {
        return;
    }

    tile = (w1 >> 24) & 0x7u;
    fmt = (w0 >> 21) & 0x7u;
    siz = (w0 >> 19) & 0x3u;
    line = (w0 >> 9) & 0x1FFu;
    tmem = w0 & 0x1FFu;
    palette = (w1 >> 20) & 0xFu;
    cmt = (w1 >> 18) & 0x3u;
    maskt = (w1 >> 14) & 0xFu;
    shiftt = (w1 >> 10) & 0xFu;
    cms = (w1 >> 8) & 0x3u;
    masks = (w1 >> 4) & 0xFu;
    shifts = w1 & 0xFu;

    stats->texture_mask |= NDS_RENDERER_TEXTURE_SETTILE;
    stats->texture_set_tile_count++;

    tile_state = &stats->texture_tiles[tile];
    tile_state->set_seen = 1u;
    tile_state->format = fmt;
    tile_state->size = siz;
    tile_state->line = line;
    tile_state->tmem = tmem;
    tile_state->palette = palette;
    tile_state->cms = cms;
    tile_state->cmt = cmt;
    tile_state->masks = masks;
    tile_state->maskt = maskt;
    tile_state->shifts = shifts;
    tile_state->shiftt = shiftt;
    tile_state->flags = ndsRendererTileFlags(cms, cmt, masks, maskt);

    /* Tile-sync memo invariant, and the ONLY rule either writer applies: bump
     * when the write targets the last-synced tile or the load tile, because
     * those are the two texture_tiles[] entries ndsRendererSyncTextureTile
     * reads. A write to any other tile cannot change that function's output
     * for the tile it last published, and if the active tile later moves to
     * this one the memo's index compare forces a full republish anyway. */
    if ((tile == stats->texture_render_tile) ||
        (tile == NDS_RENDERER_LOAD_TILE))
    {
        stats->texture_tile_write_serial++;
    }

    if (tile == NDS_RENDERER_LOAD_TILE)
    {
        stats->texture_load_tile = tile;
    }

#if NDS_TASK107_RENDER_STATE_CENSUS
    ndsRendererTask107RecordTextureSync(stats, NDS_TASK107_SYNC_SETTILE);
#endif
    ndsRendererSyncTextureTile(stats);
}

static void ndsRendererCaptureTextureLoad(NDSRendererStats *stats)
{
    NDSRendererTextureLoadState *load;
    u32 tile;

    if (stats == NULL)
    {
        return;
    }

    tile = stats->texture_load_tile & 0x7u;
    stats->texture_load_sequence++;
    load = &stats->texture_loads[
        (stats->texture_load_sequence - 1u) %
        NDS_RENDERER_TEXTURE_LOAD_HISTORY_COUNT];
    memset(load, 0, sizeof(*load));
    load->sequence = stats->texture_load_sequence;
    load->image = stats->texture_image;
    load->image_format = stats->texture_format;
    load->image_size = stats->texture_size;
    load->image_width = stats->texture_image_width;
    load->load_kind = stats->texture_load_kind;
    load->load_tile = tile;
    load->load_uls = stats->texture_load_block_uls;
    load->load_ult = stats->texture_load_block_ult;
    load->load_lrs = stats->texture_load_block_lrs;
    load->load_dxt = stats->texture_load_block_dxt;
    /* The compact per-TMEM record deliberately stores only bounded texture
     * loads. A large LOADTILE rectangle must not wrap into a plausible small
     * source span and later pass the residency checks. */
    load->load_texels = (stats->texture_load_texels <= 0xffffu) ?
        (u16)stats->texture_load_texels : 0u;
    load->load_tmem = stats->texture_tiles[tile].tmem;
    load->valid = ((load->image != 0u) &&
                   (load->load_texels != 0u) &&
                   (stats->texture_tiles[tile].set_seen != 0u)) ? TRUE : FALSE;
}

static void NDS_FIGHTER_PACKET_EVICT(NDS_R2_DELTA_PATH_CODE)
ndsRendererRecordLoadBlock(NDSRendererStats *stats, u32 w0, u32 w1)
{
    if (stats == NULL)
    {
        return;
    }

    stats->texture_mask |= NDS_RENDERER_TEXTURE_LOADBLOCK;
    stats->texture_load_kind = NDS_RENDERER_TEXTURE_LOADBLOCK;
    stats->texture_load_tile = (w1 >> 24) & 0x7u;
    stats->texture_load_block_uls = (w0 >> 12) & 0x0FFFu;
    stats->texture_load_block_ult = w0 & 0x0FFFu;
    stats->texture_load_block_lrs = (w1 >> 12) & 0x0FFFu;
    stats->texture_load_block_dxt = w1 & 0x0FFFu;
    stats->texture_load_texels = stats->texture_load_block_lrs + 1u;
    ndsRendererCaptureTextureLoad(stats);
}

static void ndsRendererRecordLoadTile(NDSRendererStats *stats,
                                      u32 w0, u32 w1)
{
    u32 uls;
    u32 ult;
    u32 lrs;
    u32 lrt;
    u32 width;
    u32 height;

    if (stats == NULL)
    {
        return;
    }

    uls = (w0 >> 12) & 0x0FFFu;
    ult = w0 & 0x0FFFu;
    lrs = (w1 >> 12) & 0x0FFFu;
    lrt = w1 & 0x0FFFu;

    stats->texture_mask |= NDS_RENDERER_TEXTURE_LOADTILE;
    stats->texture_load_kind = NDS_RENDERER_TEXTURE_LOADTILE;
    stats->texture_load_tile = (w1 >> 24) & 0x7u;
    stats->texture_load_block_uls = uls;
    stats->texture_load_block_ult = ult;
    stats->texture_load_block_lrs = lrs;
    stats->texture_load_block_dxt = lrt;
    stats->texture_load_texels = 0u;
    if ((lrs >= uls) && (lrt >= ult))
    {
        width = ((lrs - uls) >> 2) + 1u;
        height = ((lrt - ult) >> 2) + 1u;
        stats->texture_load_texels = width * height;
    }
    ndsRendererCaptureTextureLoad(stats);
}

static void NDS_FIGHTER_PACKET_EVICT(NDS_R2_DELTA_PATH_CODE)
ndsRendererRecordSetTileSize(NDSRendererStats *stats, u32 w0, u32 w1)
{
    u32 tile_index;
    u32 uls;
    u32 ult;
    u32 lrs;
    u32 lrt;
    NDSRendererTileState *tile;

    if (stats == NULL)
    {
        return;
    }

    stats->texture_mask |= NDS_RENDERER_TEXTURE_SETTILESIZE;

    uls = (w0 >> 12) & 0x0FFFu;
    ult = w0 & 0x0FFFu;
    lrs = (w1 >> 12) & 0x0FFFu;
    lrt = w1 & 0x0FFFu;

    tile_index = (w1 >> 24) & 0x7u;
    tile = &stats->texture_tiles[tile_index];
    tile->size_seen = 1u;
    tile->uls = uls;
    tile->ult = ult;
    tile->lrs = lrs;
    tile->lrt = lrt;
    tile->width = 0u;
    tile->height = 0u;
    if (lrs >= uls)
    {
        tile->width = ((lrs - uls) >> 2) + 1u;
    }
    if (lrt >= ult)
    {
        tile->height = ((lrt - ult) >> 2) + 1u;
    }
    /* Same memo invariant as ndsRendererRecordSetTile. */
    if ((tile_index == stats->texture_render_tile) ||
        (tile_index == NDS_RENDERER_LOAD_TILE))
    {
        stats->texture_tile_write_serial++;
    }
#if NDS_TASK107_RENDER_STATE_CENSUS
    ndsRendererTask107RecordTextureSync(stats, NDS_TASK107_SYNC_SETTILESIZE);
#endif
    ndsRendererSyncTextureTile(stats);
}

static void NDS_R2_DELTA_PATH_CODE
ndsRendererRecordSetImage(NDSRendererStats *stats, u32 w0, u32 w1)
{
    if (stats == NULL)
    {
        return;
    }

    stats->texture_mask |= NDS_RENDERER_TEXTURE_SETTIMG;
    stats->texture_format = (w0 >> 21) & 0x7u;
    stats->texture_size = (w0 >> 19) & 0x3u;
    stats->texture_image_width = (w0 & 0x0FFFu) + 1u;
    stats->texture_image = w1;
}

static void NDS_R2_DELTA_PATH_CODE
ndsRendererRecordLoadTlut(NDSRendererStats *stats, u32 w1)
{
    if (stats == NULL)
    {
        return;
    }

    stats->texture_command_count++;
    stats->texture_tlut_tile = (w1 >> 24) & 0x7u;
    stats->texture_tlut_count = ((w1 >> 14) & 0x3ffu) + 1u;
    if (stats->texture_image != 0u)
    {
        stats->texture_tlut_image = stats->texture_image;
    }
}

#if NDS_RENDERER_PROFILE_LEVEL >= 2
static void ndsRendererProfileCombineMode(u32 w0, u32 w1)
{
    gNdsRendererProfileCombineModeCount++;
    if (((gNdsRendererProfileCombineMode0W0 == w0) &&
         (gNdsRendererProfileCombineMode0W1 == w1)) ||
        ((gNdsRendererProfileCombineMode1W0 == w0) &&
         (gNdsRendererProfileCombineMode1W1 == w1)) ||
        ((gNdsRendererProfileCombineMode2W0 == w0) &&
         (gNdsRendererProfileCombineMode2W1 == w1)) ||
        ((gNdsRendererProfileCombineMode3W0 == w0) &&
         (gNdsRendererProfileCombineMode3W1 == w1)))
    {
        return;
    }

    switch (gNdsRendererProfileCombineModeDistinctCount)
    {
    case 0:
        gNdsRendererProfileCombineMode0W0 = w0;
        gNdsRendererProfileCombineMode0W1 = w1;
        break;
    case 1:
        gNdsRendererProfileCombineMode1W0 = w0;
        gNdsRendererProfileCombineMode1W1 = w1;
        break;
    case 2:
        gNdsRendererProfileCombineMode2W0 = w0;
        gNdsRendererProfileCombineMode2W1 = w1;
        break;
    case 3:
        gNdsRendererProfileCombineMode3W0 = w0;
        gNdsRendererProfileCombineMode3W1 = w1;
        break;
    default:
        break;
    }
    gNdsRendererProfileCombineModeDistinctCount++;
}
#endif

static void NDS_R2_DELTA_PATH_CODE
ndsRendererRecordSetCombine(NDSRendererStats *stats, u32 w0, u32 w1)
{
    if (stats == NULL)
    {
        return;
    }

    stats->texture_mask |= NDS_RENDERER_TEXTURE_SETCOMBINE;
    stats->texture_combine_count++;
    stats->texture_combine_w0 = w0;
    stats->texture_combine_w1 = w1;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    ndsRendererProfileCombineMode(w0, w1);
#endif
}

#if NDS_RENDERER_HW_TRIANGLES
static u32 ndsRendererMatrixSnapshotSignature(
    const NDSRendererMatrix20p12 *matrix)
{
    u32 signature = 2166136261u;
    u32 row;
    u32 col;

    if (matrix == NULL)
    {
        return 0u;
    }
    for (row = 0u; row < 4u; row++)
    {
        for (col = 0u; col < 4u; col++)
        {
            signature ^= (u32)matrix->m[row][col];
            signature *= 16777619u;
        }
    }
    return signature;
}

static const NDSRendererMatrixSnapshot *ndsRendererGetMatrixSnapshot(
    const NDSRendererTraversalState *state, u32 snapshot_id)
{
    if ((state == NULL) || (state->matrix_snapshots == NULL) ||
        (snapshot_id == NDS_RENDERER_MATRIX_SNAPSHOT_INVALID) ||
        (snapshot_id > state->matrix_snapshot_count))
    {
        return NULL;
    }
    return &state->matrix_snapshots[snapshot_id - 1u];
}

static u32 ndsRendererAcquireCurrentMatrixSnapshot(
    NDSRendererTraversalState *state)
{
    NDSRendererMatrixSnapshot *snapshot;
    u32 signature;
    u32 i;

    if ((state == NULL) || (state->matrix_valid == 0u) ||
        (state->matrix_snapshots == NULL))
    {
        return NDS_RENDERER_MATRIX_SNAPSHOT_INVALID;
    }
    if (state->current_matrix_snapshot !=
        NDS_RENDERER_MATRIX_SNAPSHOT_INVALID)
    {
        return state->current_matrix_snapshot;
    }

    signature = ndsRendererMatrixSnapshotSignature(&state->matrix);
    for (i = 0u; i < state->matrix_snapshot_count; i++)
    {
        snapshot = &state->matrix_snapshots[i];
        if ((snapshot->signature == signature) &&
            (memcmp(&snapshot->matrix, &state->matrix,
                    sizeof(snapshot->matrix)) == 0))
        {
            state->current_matrix_snapshot = i + 1u;
            ndsRendererProfileRecordMatrixSnapshotReuse();
            return state->current_matrix_snapshot;
        }
    }
    if (state->matrix_snapshot_count >=
        NDS_RENDERER_MATRIX_SNAPSHOT_CAPACITY)
    {
        ndsRendererProfileRecordMatrixSnapshotOverflow();
        return NDS_RENDERER_MATRIX_SNAPSHOT_INVALID;
    }

    snapshot = &state->matrix_snapshots[state->matrix_snapshot_count++];
    snapshot->matrix = state->matrix;
    snapshot->generation = state->matrix_generation;
    snapshot->signature = signature;
    state->current_matrix_snapshot = state->matrix_snapshot_count;
    ndsRendererProfileRecordMatrixSnapshotCreate();
    return state->current_matrix_snapshot;
}

static s32 ndsRendererTransformCachedVertex(
    NDSRendererStats *stats, NDSRendererTraversalState *state, u32 index,
    const NDSRendererMatrix20p12 *matrix, u32 snapshot_id)
{
    NDSRendererClipVertex20p12 *out;
    u32 mask;

    if ((stats == NULL) || (state == NULL) || (matrix == NULL) ||
        (index >= NDS_RENDERER_MAX_VTX))
    {
        return FALSE;
    }
    mask = 1u << index;
    if ((state->input_vertex_valid_mask & mask) == 0u)
    {
        return FALSE;
    }

    out = &state->vertices[index];
    ndsRendererTransformVertex20p12(matrix, &state->input_vertices[index], out);
    state->vertex_valid_mask |= mask;
    state->vertex_clip_snapshot[index] = (u8)snapshot_id;
    stats->matrix_transform_count++;
    stats->transformed_vertex_count++;
    if (stats->transformed_vertex_count == 1u)
    {
        stats->first_transformed_x = out->x;
        stats->first_transformed_y = out->y;
        stats->first_transformed_z = out->z;
        stats->first_transformed_w = out->w;
    }
    ndsRendererProfileRecordCPUTransform();
    return TRUE;
}

static s32 ndsRendererEnsureTransformedVertex(
    NDSRendererStats *stats, NDSRendererTraversalState *state, u32 index)
{
    const NDSRendererMatrixSnapshot *snapshot;
    u32 snapshot_id;
    u32 mask;

    if ((state == NULL) || (index >= NDS_RENDERER_MAX_VTX))
    {
        return FALSE;
    }
    mask = 1u << index;
    snapshot_id = state->vertex_matrix_snapshot[index];
    if (((state->vertex_valid_mask & mask) != 0u) &&
        (state->vertex_clip_snapshot[index] == snapshot_id))
    {
        ndsRendererProfileRecordTransformCacheHit();
        return TRUE;
    }

    snapshot = ndsRendererGetMatrixSnapshot(state, snapshot_id);
    if (snapshot == NULL)
    {
        return FALSE;
    }
    return ndsRendererTransformCachedVertex(
        stats, state, index, &snapshot->matrix, snapshot_id);
}
#endif

static void ndsRendererComposeMatrix(NDSRendererTraversalState *state)
{
    NDSRendererMatrix20p12 identity;

    if (state == NULL)
    {
        return;
    }

    if ((state->projection_valid != 0u) &&
        (state->modelview_valid != 0u))
    {
        ndsRendererMtxMul20p12(&state->modelview,
                               &state->projection,
                               &state->matrix);
    }
    else if (state->modelview_valid != 0u)
    {
        state->matrix = state->modelview;
    }
    else if (state->projection_valid != 0u)
    {
        state->matrix = state->projection;
    }
    else
    {
        ndsRendererMtxIdentity20p12(&identity);
        state->matrix = identity;
    }
    state->matrix_valid =
        ((state->projection_valid != 0u) ||
         (state->modelview_valid != 0u)) ? TRUE : FALSE;
    state->matrix_word_valid = FALSE;
#if NDS_RENDERER_HW_TRIANGLES
    /* Cached RSP vertices retain the transform active when they were loaded. */
    state->current_transform_vertex_mask = 0u;
    state->current_matrix_snapshot = NDS_RENDERER_MATRIX_SNAPSHOT_INVALID;
    state->matrix_generation = ndsRendererNextMatrixGeneration();
    NDS_RENDERER_INVALIDATE_LIGHT_DIRECTION(state);
#endif
}

static void ndsRendererInitMatrixWordRaw(NDSRendererTraversalState *state)
{
    NDSRendererMatrix20p12 identity;

    if (state == NULL)
    {
        return;
    }

    if (state->matrix_valid == 0u)
    {
        ndsRendererMtxIdentity20p12(&identity);
        ndsRendererMtxStoreDS20p12ToN64(&identity, &state->matrix_word_raw);
    }
    else
    {
        ndsRendererMtxStoreDS20p12ToN64(&state->matrix,
                                        &state->matrix_word_raw);
    }
    state->matrix_word_valid = TRUE;
}

static void ndsRendererInitTraversalState(NDSRendererTraversalState *state,
                                          const NDSRendererConfig *config,
                                          NDSRendererStats *stats,
                                          NDSRendererTraversalVertexStorage
                                              *vertex_storage,
                                          NDSRendererMatrixSnapshot *snapshots,
                                          u32 snapshot_count)
{
    if (state == NULL)
    {
        return;
    }

    /* Valid masks and stack depth own every scratch read. Initialize that
     * compact control plane instead of clearing matrix, stack, and derived
     * arrays that will be overwritten before their first valid use. */
    state->modelview_stack_depth = 0u;
    state->vertex_valid_mask = 0u;
    state->vertices = (vertex_storage != NULL) ?
        vertex_storage->vertices : NULL;
#if NDS_RENDERER_HW_TRIANGLES
    state->input_vertices = (vertex_storage != NULL) ?
        vertex_storage->input_vertices : NULL;
    state->vertex_colors = (vertex_storage != NULL) ?
        vertex_storage->vertex_colors : NULL;
    state->vertex_matrix_snapshot = (vertex_storage != NULL) ?
        vertex_storage->vertex_matrix_snapshot : NULL;
    state->vertex_clip_snapshot = (vertex_storage != NULL) ?
        vertex_storage->vertex_clip_snapshot : NULL;
    state->matrix_snapshots = snapshots;
    state->source_command_site = NULL;
    state->matrix_snapshot_count =
        (snapshot_count <= NDS_RENDERER_MATRIX_SNAPSHOT_CAPACITY) ?
            snapshot_count : NDS_RENDERER_MATRIX_SNAPSHOT_CAPACITY;
    state->input_vertex_valid_mask = 0u;
    state->vertex_color_valid_mask = 0u;
    state->current_transform_vertex_mask = 0u;
    state->matrix_generation = 0u;
    state->current_matrix_snapshot = NDS_RENDERER_MATRIX_SNAPSHOT_INVALID;
    state->color_modulate = (config != NULL) ? config->color_modulate : 0u;
    state->prepared_light_direction_valid = 0u;
    state->texture_prepare_valid = 0u;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    state->texture_prepare_key_hash = 0u;
    state->texture_prepare_params = 0u;
    state->semantic_branch_path =
        sNdsRendererSemanticSourceProvenance.root_branch_path;
    if (state->semantic_branch_path == 0u)
    {
        state->semantic_branch_path = ndsRendererSemanticBranchPath(
            (u32)sNdsRendererProfileOwner,
            sNdsRendererSemanticSourceProvenance.owner_occurrence,
            sNdsRendererSemanticSourceProvenance.list_ordinal,
            FALSE);
    }
    state->semantic_command_index = 0u;
    state->semantic_tri2_half = 0u;
#endif
    state->prepared_vertex_color_valid_mask = 0u;
    state->prepared_texcoord_valid_mask = 0u;
    state->prepared_projected_xy_valid_mask = 0u;
    state->prepared_projected_source_z_valid_mask = 0u;
    state->raw_vertex_fit_mask = 0u;
#else
    (void)snapshots;
    (void)snapshot_count;
#endif
    state->projection_valid = 0u;
    state->modelview_valid = 0u;
    state->matrix_valid = 0u;
    state->matrix_word_valid = 0u;
    if (config == NULL)
    {
        return;
    }
    if ((stats != NULL) && (config->initial_geometry_mode != 0u))
    {
        stats->geometry_mode = config->initial_geometry_mode;
    }
    if (config->initial_projection != NULL)
    {
        state->projection = *config->initial_projection;
        state->projection_valid = TRUE;
    }
    if (config->initial_modelview != NULL)
    {
        state->modelview = *config->initial_modelview;
        state->modelview_valid = TRUE;
    }
    ndsRendererComposeMatrix(state);
    if ((stats != NULL) && (state->matrix_valid != 0u))
    {
        stats->hardware_matrix_seed_count++;
    }
}

static void ndsRendererPushModelview(NDSRendererStats *stats,
                                     NDSRendererTraversalState *state)
{
    u32 depth;

    if ((stats == NULL) || (state == NULL))
    {
        return;
    }

    depth = state->modelview_stack_depth;
    if (depth >= NDS_RENDERER_MODELVIEW_STACK_SIZE)
    {
        NDS_RENDERER_RECORD_PROOF_ONLY(stats->skip_command_count++);
        return;
    }

    state->modelview_stack[depth] = state->modelview;
    state->modelview_valid_stack[depth] = state->modelview_valid;
    state->modelview_stack_depth = depth + 1u;
}

static void ndsRendererApplyMatrixCommand(
    const NDSRendererConfig *config,
    NDSRendererStats *stats,
    NDSRendererTraversalState *state,
    u32 w0,
    u32 w1)
{
    const Mtx *src;
    NDSRendererMatrix20p12 incoming;
    NDSRendererMatrix20p12 *target;
    u32 *target_valid;
    u32 flags;

    if ((stats == NULL) || (state == NULL))
    {
        return;
    }

    flags = (w0 & 0xffu) ^ NDS_RENDERER_MTX_PUSH_XOR;
    NDS_RENDERER_RECORD_PROOF_ONLY(stats->state_command_count++);
    stats->matrix_command_count++;
    stats->matrix_flags = flags;
    if ((flags & NDS_RENDERER_MTX_PROJECTION) != 0u)
    {
        stats->matrix_projection_count++;
    }
    else
    {
        stats->matrix_modelview_count++;
    }
    if ((flags & NDS_RENDERER_MTX_PUSH) != 0u)
    {
        stats->matrix_push_count++;
    }

    src = ndsRendererResolveDataPointer(config,
                                        (const void *)(uintptr_t)w1,
                                        sizeof(Mtx));
    if (src == NULL)
    {
        NDS_RENDERER_RECORD_PROOF_ONLY(stats->skip_command_count++);
        return;
    }
    ndsRendererMtxLoadN64ToDS20p12(src, &incoming);
    if ((flags & NDS_RENDERER_MTX_PROJECTION) != 0u)
    {
        target = &state->projection;
        target_valid = &state->projection_valid;
    }
    else
    {
        target = &state->modelview;
        target_valid = &state->modelview_valid;
        if ((flags & NDS_RENDERER_MTX_PUSH) != 0u)
        {
            ndsRendererPushModelview(stats, state);
        }
    }

    if ((flags & NDS_RENDERER_MTX_LOAD) != 0u)
    {
        *target = incoming;
        *target_valid = TRUE;
        stats->matrix_load_count++;
    }
    else
    {
        if (*target_valid != 0u)
        {
            ndsRendererMtxMul20p12(target, &incoming, target);
        }
        else
        {
            *target = incoming;
            *target_valid = TRUE;
        }
        stats->matrix_mul_count++;
    }
    ndsRendererComposeMatrix(state);
}

static void ndsRendererApplyMvpRecalcCommand(
    NDSRendererStats *stats,
    NDSRendererTraversalState *state,
    u32 w0,
    u32 w1)
{
    if ((stats == NULL) || (state == NULL))
    {
        return;
    }

    NDS_RENDERER_RECORD_PROOF_ONLY(stats->state_command_count++);
    if ((((w0 >> NDS_RENDERER_SPECIAL_1_OFFSET_SHIFT) &
          NDS_RENDERER_SPECIAL_1_OFFSET_MASK) != 0u) ||
        ((w0 & NDS_RENDERER_SPECIAL_1_INDEX_MASK) != 1u) ||
        (w1 != 0u))
    {
        ndsRendererRecordUnsupported(stats, NDS_RENDERER_OP_SPECIAL_1);
        return;
    }

    stats->matrix_command_count++;
    stats->matrix_mvp_recalc_count++;
    ndsRendererInitMatrixWordRaw(state);
}

static void ndsRendererRecordFogMoveWord(NDSRendererStats *stats, u32 w1)
{
    s32 mul;
    s32 ofs;
    s32 fog_min;
    s32 fog_max;

    if ((stats == NULL) || (stats->fog_status >= 2u))
    {
        return;
    }

    mul = (s16)(w1 >> 16);
    ofs = (s16)w1;
    if (mul == 0)
    {
        stats->ignored_state_command_count++;
        return;
    }

    fog_min = 500 - ((ofs * 500) / mul);
    fog_max = (128000 / mul) + fog_min;
    if ((stats->fog_status == 0u) ||
        (stats->fog_min != fog_min) ||
        (stats->fog_max != fog_max))
    {
        stats->fog_status++;
        stats->fog_min = fog_min;
        stats->fog_max = fog_max;
    }
}

static void ndsRendererRecordFogColor(NDSRendererStats *stats, u32 w1)
{
    if ((stats != NULL) && (stats->fog_status < 2u))
    {
        stats->fog_color = w1;
    }
}

static u32 ndsRendererPackLightColor(const u8 color[3])
{
    return ((u32)color[0] << 24) |
        ((u32)color[1] << 16) |
        ((u32)color[2] << 8);
}

static void ndsRendererRecordLightColor(NDSRendererStats *stats,
                                        u32 light, u32 color)
{
    if (stats == NULL)
    {
        return;
    }
    if (light == 1u)
    {
        stats->light_color_1 = color;
        stats->light_color_mask |= NDS_RENDERER_LIGHT_COLOR_1_MASK;
        stats->light_color_command_count++;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        gNdsRendererProfileLightColorCommands++;
#endif
    }
    else if (light == 2u)
    {
        stats->light_color_2 = color;
        stats->light_color_mask |= NDS_RENDERER_LIGHT_COLOR_2_MASK;
        stats->light_color_command_count++;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        gNdsRendererProfileLightColorCommands++;
#endif
    }
}

static void ndsRendererRecordLightColorMoveWord(NDSRendererStats *stats,
                                                u32 offset, u32 color)
{
    switch (offset)
    {
    case NDS_RENDERER_MOVEWORD_LIGHTCOL_LIGHT_1_A:
    case NDS_RENDERER_MOVEWORD_LIGHTCOL_LIGHT_1_B:
        ndsRendererRecordLightColor(stats, 1u, color);
        break;

    case NDS_RENDERER_MOVEWORD_LIGHTCOL_LIGHT_2_A:
    case NDS_RENDERER_MOVEWORD_LIGHTCOL_LIGHT_2_B:
        ndsRendererRecordLightColor(stats, 2u, color);
        break;

    default:
        if (stats != NULL)
        {
            stats->ignored_state_command_count++;
        }
        break;
    }
}

static void ndsRendererRecordLightMoveMem(
    const NDSRendererConfig *config, NDSRendererStats *stats, u32 w0, u32 w1)
{
    u32 index = w0 & 0xffu;
    u32 offset =
        ((w0 >> NDS_RENDERER_MOVEMEM_OFFSET_SHIFT) &
         NDS_RENDERER_MOVEMEM_OFFSET_MASK) * 8u;
    u32 length =
        (((w0 >> NDS_RENDERER_MOVEMEM_LENGTH_SHIFT) &
          NDS_RENDERER_MOVEMEM_LENGTH_MASK) + 1u) * 8u;
    u32 light;
    const Light *src;

    if (stats == NULL)
    {
        return;
    }
    NDS_RENDERER_RECORD_PROOF_ONLY(stats->state_command_count++);
    if ((index != NDS_RENDERER_MOVEMEM_LIGHT) ||
        (offset < NDS_RENDERER_MOVEMEM_LIGHT_BASE_OFFSET) ||
        (((offset - NDS_RENDERER_MOVEMEM_LIGHT_BASE_OFFSET) %
          NDS_RENDERER_MOVEMEM_LIGHT_STRIDE) != 0u) ||
        (length < sizeof(Light)))
    {
        stats->ignored_state_command_count++;
        return;
    }

    light = (offset - NDS_RENDERER_MOVEMEM_LIGHT_BASE_OFFSET) /
        NDS_RENDERER_MOVEMEM_LIGHT_STRIDE;
    src = ndsRendererResolveDataPointer(
        config, (const void *)(uintptr_t)w1, sizeof(*src));
    if (src == NULL)
    {
        stats->ignored_state_command_count++;
        return;
    }

    ndsRendererRecordLightColor(stats, light,
                                ndsRendererPackLightColor(src->l.col));
    if (light == 1u)
    {
        stats->light_dir_x = src->l.dir[0];
        stats->light_dir_y = src->l.dir[1];
        stats->light_dir_z = src->l.dir[2];
        stats->light_dir_mask |= NDS_RENDERER_LIGHT_DIR_1_MASK;
        stats->light_direction_command_count++;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        gNdsRendererProfileLightDirectionCommands++;
#endif
    }
}

static void ndsRendererApplyMatrixMoveWordCommand(
    NDSRendererStats *stats,
    NDSRendererTraversalState *state,
    u32 w0,
    u32 w1)
{
    u32 index;
    u32 offset;
    u32 word_index;
    u32 *words;

    if ((stats == NULL) || (state == NULL))
    {
        return;
    }

    NDS_RENDERER_RECORD_PROOF_ONLY(stats->state_command_count++);
    index = (w0 >> NDS_RENDERER_MOVEWORD_INDEX_SHIFT) &
        NDS_RENDERER_MOVEWORD_INDEX_MASK;
    offset = w0 & NDS_RENDERER_MOVEWORD_OFFSET_MASK;
    if ((index == NDS_RENDERER_MOVEWORD_FOG) &&
        (offset == NDS_RENDERER_MOVEWORD_FOG_OFFSET))
    {
        ndsRendererRecordFogMoveWord(stats, w1);
        return;
    }
    if (index == NDS_RENDERER_MOVEWORD_LIGHTCOL)
    {
        ndsRendererRecordLightColorMoveWord(stats, offset, w1);
        return;
    }
    if ((index != NDS_RENDERER_MOVEWORD_MATRIX) ||
        ((offset % NDS_RENDERER_MATRIX_WORD_BYTES) != 0u) ||
        ((offset / NDS_RENDERER_MATRIX_WORD_BYTES) >=
         NDS_RENDERER_MATRIX_WORD_COUNT))
    {
        stats->ignored_state_command_count++;
        return;
    }

    if (state->matrix_word_valid == 0u)
    {
        ndsRendererInitMatrixWordRaw(state);
    }

    word_index = offset / NDS_RENDERER_MATRIX_WORD_BYTES;
    words = (u32 *)&state->matrix_word_raw.m[0][0];
    words[word_index] = w1;
    ndsRendererMtxLoadN64ToDS20p12(&state->matrix_word_raw, &state->matrix);
    state->matrix_valid = TRUE;
#if NDS_RENDERER_HW_TRIANGLES
    state->current_transform_vertex_mask = 0u;
    state->current_matrix_snapshot = NDS_RENDERER_MATRIX_SNAPSHOT_INVALID;
    state->matrix_generation = ndsRendererNextMatrixGeneration();
    NDS_RENDERER_INVALIDATE_LIGHT_DIRECTION(state);
#endif
    stats->matrix_command_count++;
    stats->matrix_move_word_count++;
}

static void ndsRendererApplyPopMatrixCommand(NDSRendererStats *stats,
                                             NDSRendererTraversalState *state,
                                             u32 w1)
{
    u32 count;

    if ((stats == NULL) || (state == NULL))
    {
        return;
    }

    count = w1 / 64u;
    if (count == 0u)
    {
        count = 1u;
    }

    NDS_RENDERER_RECORD_PROOF_ONLY(stats->state_command_count++);
    stats->matrix_command_count++;
    stats->matrix_modelview_count++;
    stats->matrix_pop_count += count;

    while (count != 0u)
    {
        u32 depth = state->modelview_stack_depth;

        if (depth == 0u)
        {
            NDS_RENDERER_RECORD_PROOF_ONLY(stats->skip_command_count++);
            break;
        }

        depth--;
        state->modelview = state->modelview_stack[depth];
        state->modelview_valid = state->modelview_valid_stack[depth];
        state->modelview_stack_depth = depth;
        count--;
    }
    ndsRendererComposeMatrix(state);
}

static void NDS_RENDERER_HOT_CODE
ndsRendererApplyVertexCommand(
    const NDSRendererConfig *config,
    NDSRendererStats *stats,
    NDSRendererTraversalState *state,
    u32 w0,
    u32 w1)
{
    u32 v0;
    u32 count;
    const u8 *src;
    u32 i;
#if NDS_RENDERER_HW_TRIANGLES
    const NDSRendererHardwareLightDirection *prepared_light_direction = NULL;
#if NDS_RENDERER_PROFILE_LEVEL < 2
    const u32 *prepared_light_shade_lut = NULL;
#endif
    u32 matrix_snapshot = NDS_RENDERER_MATRIX_SNAPSHOT_INVALID;
#endif

    if ((stats == NULL) || (state == NULL))
    {
        return;
    }
    NDS_RENDERER_RECORD_PROOF_ONLY(stats->vertex_command_count++);
    NDS_RENDERER_RECORD_PROOF_ONLY(stats->state_command_count++);
    if (ndsGBIDecodeF3DEX2Vtx(w0, NDS_RENDERER_MAX_VTX, &v0,
                              &count) == FALSE)
    {
        NDS_RENDERER_RECORD_PROOF_ONLY(stats->skip_command_count++);
        return;
    }
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    stats->source_vertex_count += count;
#endif
    if ((v0 + count) > stats->vertex_count)
    {
        stats->vertex_count = v0 + count;
    }
#if !NDS_RENDERER_HW_TRIANGLES
    if (state->matrix_valid == 0u)
    {
        return;
    }
#endif

    src = ndsRendererResolveDataPointer(config,
                                        (const void *)(uintptr_t)w1,
                                        (size_t)count * 16u);
    if (src == NULL)
    {
        NDS_RENDERER_RECORD_PROOF_ONLY(stats->skip_command_count++);
        return;
    }
#if NDS_RENDERER_HW_TRIANGLES
    if (state->matrix_valid != 0u)
    {
        matrix_snapshot = ndsRendererAcquireCurrentMatrixSnapshot(state);
    }
    if (((stats->geometry_mode & NDS_RENDERER_GEOM_LIGHTING) != 0u) &&
        ((stats->light_dir_mask & NDS_RENDERER_LIGHT_DIR_1_MASK) != 0u))
    {
        if (state->prepared_light_direction_valid == 0u)
        {
            /* Matrix and MOVEMEM handlers invalidate this exact source-state
             * value; adjacent VTX commands can share its float normalization. */
            ndsRendererHardwarePrepareLitDirection(
                stats,
                (state->modelview_valid != 0u) ? &state->modelview : NULL,
                &state->prepared_light_direction);
            state->prepared_light_direction_valid = TRUE;
        }
        prepared_light_direction = &state->prepared_light_direction;
#if NDS_RENDERER_PROFILE_LEVEL < 2
        if ((stats->light_color_mask &
             (NDS_RENDERER_LIGHT_COLOR_1_MASK |
              NDS_RENDERER_LIGHT_COLOR_2_MASK)) ==
            (NDS_RENDERER_LIGHT_COLOR_1_MASK |
             NDS_RENDERER_LIGHT_COLOR_2_MASK))
        {
            prepared_light_shade_lut = ndsRendererHardwareGetLightShadeLut(
                stats->light_color_1, stats->light_color_2);
        }
#endif
    }
#endif

    for (i = 0u; i < count; i++)
    {
        u32 index = v0 + i;
#if NDS_RENDERER_HW_TRIANGLES
        NDSRendererInputVertex *input = &state->input_vertices[index];
        u32 mask = 1u << index;
#else
        NDSRendererInputVertex input_storage;
        NDSRendererInputVertex *input = &input_storage;
        NDSRendererClipVertex20p12 *out = &state->vertices[index];
#endif

        ndsRendererDecodeInputVertex(input, src + (i * 16u));
#if NDS_RENDERER_HW_TRIANGLES
        ndsRendererProfileRecordSourceVertexLoad();
        state->input_vertex_valid_mask |= mask;
        if (ndsRendererHardwareRawVertexFits(input) != FALSE)
        {
            state->raw_vertex_fit_mask |= mask;
        }
        else
        {
            state->raw_vertex_fit_mask &= ~mask;
        }
#if NDS_RENDERER_PROFILE_LEVEL < 2
        if (prepared_light_shade_lut != NULL)
        {
            state->vertex_colors[index] =
                ndsRendererHardwareLitShadeColorLut(
                    input, prepared_light_direction,
                    prepared_light_shade_lut);
        }
        else
#endif
        {
            state->vertex_colors[index] =
                ndsRendererHardwareLitShadeColorPrepared(
                    stats, input, prepared_light_direction);
        }
        state->vertex_color_valid_mask |= mask;
        state->vertex_matrix_snapshot[index] = (u8)matrix_snapshot;
        state->vertex_clip_snapshot[index] =
            NDS_RENDERER_MATRIX_SNAPSHOT_INVALID;
        state->vertex_valid_mask &= ~mask;
        state->current_transform_vertex_mask &= ~mask;
        if (state->matrix_valid != 0u)
        {
            state->current_transform_vertex_mask |= mask;
        }
#endif
        if (state->matrix_valid == 0u)
        {
            continue;
        }
#if NDS_RENDERER_HW_TRIANGLES
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        (void)ndsRendererTransformCachedVertex(
            stats, state, index, &state->matrix, matrix_snapshot);
#else
        /* Profile 0/1 keep source vertices raw for GX. Only an exhausted
         * snapshot table needs the eager clip fallback retained here. */
        if (matrix_snapshot == NDS_RENDERER_MATRIX_SNAPSHOT_INVALID)
        {
            (void)ndsRendererTransformCachedVertex(
                stats, state, index, &state->matrix, matrix_snapshot);
        }
#endif
#else
        ndsRendererTransformVertex20p12(&state->matrix, input, out);
        state->vertex_valid_mask |= 1u << index;
        stats->matrix_transform_count++;
        stats->transformed_vertex_count++;
        if (stats->transformed_vertex_count == 1u)
        {
            stats->first_transformed_x = out->x;
            stats->first_transformed_y = out->y;
            stats->first_transformed_z = out->z;
            stats->first_transformed_w = out->w;
        }
#endif
    }
}

static void ndsRendererApplyModifyVertexCommand(
    NDSRendererStats *stats,
    NDSRendererTraversalState *state,
    u32 w0,
    u32 w1)
{
    u32 where;
    u32 packed_index;
    u32 index;

    if ((stats == NULL) || (state == NULL))
    {
        return;
    }

    NDS_RENDERER_RECORD_PROOF_ONLY(stats->state_command_count++);
    where = (w0 >> 16) & 0xffu;
    packed_index = w0 & 0xffffu;
    index = packed_index / 2u;

#if NDS_RENDERER_HW_TRIANGLES
    if ((where == NDS_RENDERER_MWO_POINT_ST) &&
        ((packed_index & 1u) == 0u) &&
        (index < NDS_RENDERER_MAX_VTX) &&
        ((state->input_vertex_valid_mask & (1u << index)) != 0u))
    {
        state->input_vertices[index].s = (s16)(w1 >> 16);
        state->input_vertices[index].t = (s16)(w1 & 0xffffu);
        return;
    }
#else
    (void)where;
    (void)packed_index;
    (void)index;
    (void)w1;
#endif

    NDS_RENDERER_RECORD_PROOF_ONLY(stats->skip_command_count++);
}

static s32 ndsRendererTransformedTriangleReady(
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
    return ((state->vertex_valid_mask & mask) == mask) ? TRUE : FALSE;
}
