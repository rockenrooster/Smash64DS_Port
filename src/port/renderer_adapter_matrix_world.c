/* Renderer adapter, world matrices: the billboard (kind 46/48) operands
 * and their memo, the world caches and their budget, and the persistent
 * stage worlds. A slice of reloc_backend_renderer_dl.c. */

/* One camera's source-derived billboard operands, scoped to a stage frame.
 * Geometry bindings share these values; no persistent source-state mirror. */
/* The kind-48 Mod1's look-at, as its two direction cosines (Q30); `zero` is
 * the source's degenerate camera (syMatrixScaF 0). */
typedef struct NDSRendererAdapterMvpMod1Q
{
    s32 l1_q30;
    s32 l2_q30;
    u32 zero;
} NDSRendererAdapterMvpMod1Q;

typedef struct NDSRendererAdapterMvpCamera
{
    const NDSRendererMatrix20p12 *perspective;
    Mtx44f perspective_f;
    NDSRendererAdapterMvpMod1Q mod1;
    u32 perspective_f_valid;
    u32 mod1_valid;
} NDSRendererAdapterMvpCamera;

/* Billboard orientation memo (2026-09-27), used only with a stage camera and
 * emptied by ndsRendererAdapterMvpMemoReset wherever that camera's cached
 * Mod1/perspective are (once per stage prepare). Kind 48's rows are a function
 * of the camera's Mod1 and the two recalc scales; kind 46's of the camera's
 * perspective, rotate.z, both scales and the incoming gGCScaleX accumulator
 * (whose outgoing value is kept too). Bindings that share those inputs share
 * the rows, so a hit skips the float rows, syMatrixF2L and the N64 conversion
 * (~2.9K ticks a billboard). Same inputs, same bits. Static, not in the
 * camera: the stage prepare's camera lives on the DTCM stack. */
#define NDS_MVP_MEMO_48 4u
#define NDS_MVP_MEMO_46 8u
static u32 sNdsMvpMemoOn;
static u32 sNdsMvpMemo48Count;
static u32 sNdsMvpMemo48Key[NDS_MVP_MEMO_48][2];
static NDSRendererMatrix20p12 sNdsMvpMemo48Rows[NDS_MVP_MEMO_48];
static u32 sNdsMvpMemo46Count;
static u32 sNdsMvpMemo46Key[NDS_MVP_MEMO_46][4];
static u32 sNdsMvpMemo46AccAfter[NDS_MVP_MEMO_46];
static NDSRendererMatrix20p12 sNdsMvpMemo46Rows[NDS_MVP_MEMO_46];

/* The camera whose operands the rows were built from. The memo is shared by
 * the stage prepare's camera and the shared camera below, so a row is served
 * only to the camera that last reset it: whichever of the two built rows
 * last, the other one builds its own. */
static const NDSRendererAdapterMvpCamera *sNdsMvpMemoOwner;

static void ndsRendererAdapterMvpMemoReset(const NDSRendererAdapterMvpCamera *owner)
{
    sNdsMvpMemoOwner = owner;
    sNdsMvpMemoOn = 1u;
    sNdsMvpMemo48Count = 0u;
    sNdsMvpMemo46Count = 0u;
}

static inline sb32 ndsRendererAdapterMvpMemoFor(
    const NDSRendererAdapterMvpCamera *camera)
{
    return ((camera != NULL) && (camera == sNdsMvpMemoOwner) &&
            (sNdsMvpMemoOn != 0u)) ? TRUE : FALSE;
}

static void ndsRendererAdapterMvpPerspectiveF(
    CObj *cobj, Mtx44f out)
{
    u16 norm = cobj->projection.persp.norm;
    syMatrixPerspFastF(out, &norm, cobj->projection.persp.fovy,
        cobj->projection.persp.aspect, cobj->projection.persp.near,
        cobj->projection.persp.far, cobj->projection.persp.scale);
}

/* THE KIND-48 MOD1 IN INTEGERS (P2-2p8, 2026-10-06; owner: "Software floating
 * point should not exist, fixed point only"). The float form built
 * syMatrixLookAtF(0, eye.y, e; 0, at.y, 0; up 0, 1, 0) x the perspective
 * (objdisplay.c:3094-3118), e the eye's horizontal distance from the target:
 * three square roots, three reciprocals and a 4x4 product, ~5K ticks a frame
 * on Peach's Castle with syMatrixPerspFastF. That look-at turns about X alone.
 * With d = eye.y - at.y and m = sqrt(d^2 + e^2), L1 = d / m and L2 = e / m,
 * its rotation rows are (1, 0, 0), (0, L2, L1) and (0, -L1, L2), so the three
 * Mod1 rows kind 48 keeps are (P00, 0, 0, 0), (0, L2 P11, L1 P22, L1 P23) and
 * (0, -L1 P11, L2 P22, L2 P23): two cosines from the math unit's square root
 * and divider, and five terms of the camera's own 20.12 projection. A
 * horizontal distance under one Q12 step is the source's zero matrix (its
 * 0.0001F test). Render only. */
static void __attribute__((noinline, target("arm")))
ndsRendererAdapterMvpMod1Q(CObj *cobj, NDSRendererAdapterMvpMod1Q *out)
{
    const s64 dx = (s64)ndsF32ToFixed(cobj->vec.at.x, 12u) -
                   ndsF32ToFixed(cobj->vec.eye.x, 12u);
    const s64 dz = (s64)ndsF32ToFixed(cobj->vec.at.z, 12u) -
                   ndsF32ToFixed(cobj->vec.eye.z, 12u);
    const s64 d = (s64)ndsF32ToFixed(cobj->vec.eye.y, 12u) -
                  ndsF32ToFixed(cobj->vec.at.y, 12u);
    s64 e;
    s64 m;

    e = (s64)ndsR2HwMathSqrt64Fast((u64)((dx * dx) + (dz * dz)));
    out->zero = (e == 0) ? TRUE : FALSE;
    if (out->zero != FALSE)
    {
        out->l1_q30 = 0;
        out->l2_q30 = 0;
        return;
    }
    m = (s64)ndsR2HwMathSqrt64Fast((u64)((d * d) + (e * e)));
    out->l1_q30 = (s32)ndsR2HwMathDivideFast(d << 30, m);
    out->l2_q30 = (s32)ndsR2HwMathDivideFast(e << 30, m);
}

/* Q30 x Q12 -> Q12, rounded. */
static inline s32 ndsMvpMulQ30(s32 a_q30, s32 b_q12)
{
    return (s32)((((s64)a_q30 * b_q12) + (1 << 29)) >> 30);
}

/* Q12 x Q16 -> Q12, rounded. */
static inline s32 ndsMvpScaleQ16(s32 a_q12, s32 s_q16)
{
    return (s32)((((s64)a_q12 * s_q16) + (1 << 15)) >> 16);
}

/* The kind-48 rows: Mod1 rows 0 and 2 by the X recalc scale, row 1 by the Y
 * one (the source's `mod1_f[row][col] * scale`), straight into 20.12. */
static void __attribute__((noinline, target("arm")))
ndsRendererAdapterMvpKind48RowsQ(const NDSRendererAdapterMvpMod1Q *mod1,
                                 const NDSRendererMatrix20p12 *persp,
                                 f32 scale_x, f32 scale_y,
                                 NDSRendererMatrix20p12 *out)
{
    s32 sx = ndsF32ToFixed(scale_x, 16u);
    s32 sy = ndsF32ToFixed(scale_y, 16u);
    const s32 p00 = persp->m[0][0];
    const s32 p11 = persp->m[1][1];
    const s32 p22 = persp->m[2][2];
    const s32 p23 = persp->m[2][3];
    u32 row;
    u32 col;

    for (row = 0u; row < 4u; row++)
    {
        for (col = 0u; col < 4u; col++)
        {
            out->m[row][col] = 0;
        }
    }
    out->m[3][3] = 1 << 12;
    if ((mod1->zero != FALSE) || (sx == NDS_R2_COLLISION_F32_OVERFLOW) ||
        (sy == NDS_R2_COLLISION_F32_OVERFLOW))
    {
        return;
    }
    out->m[0][0] = ndsMvpScaleQ16(p00, sx);
    out->m[1][1] = ndsMvpScaleQ16(ndsMvpMulQ30(mod1->l2_q30, p11), sy);
    out->m[1][2] = ndsMvpScaleQ16(ndsMvpMulQ30(mod1->l1_q30, p22), sy);
    out->m[1][3] = ndsMvpScaleQ16(ndsMvpMulQ30(mod1->l1_q30, p23), sy);
    out->m[2][1] = ndsMvpScaleQ16(-ndsMvpMulQ30(mod1->l1_q30, p11), sx);
    out->m[2][2] = ndsMvpScaleQ16(ndsMvpMulQ30(mod1->l2_q30, p22), sx);
    out->m[2][3] = ndsMvpScaleQ16(ndsMvpMulQ30(mod1->l2_q30, p23), sx);
}

/* One camera memo for every billboard recalc outside the stage prepare
 * (P2-2p8, 2026-09-27). Without it each recalc rebuilt the float perspective
 * and, for kind 48, the look-at Mod1 (syMatrixLookAtF + guMtxCatF) from the
 * CObj, and never hit the row memo: Yoshi's Island's nine cloud drawables cost
 * ~45K ticks a frame there, and every billboarded item and weapon the stage
 * DL submit draws paid ~3-4K ticks a call more than with it (Mushroom
 * Kingdom's Meowth coins: same-ROM A/B -8K WORK-H median, replay
 * identical; the efground actors share it too). The operands
 * are kept while the CObj's own inputs (eye, at, perspective) are
 * bit-identical, so every value it serves is the one the recalc would
 * compute. It claims the row memo (resets it as its owner)
 * whenever the stage prepare's camera has claimed it since. */
typedef struct NDSRendererAdapterSharedMvpCameraKey
{
    Vec3f eye;
    Vec3f at;
    f32 fovy;
    f32 aspect;
    f32 near;
    f32 far;
    f32 scale;
} NDSRendererAdapterSharedMvpCameraKey;
static NDSRendererAdapterSharedMvpCameraKey sNdsSharedMvpCameraKey;
static const void *sNdsSharedMvpCameraCobj;
static NDSRendererAdapterMvpCamera sNdsSharedMvpCamera;
static NDSRendererMatrix20p12 sNdsSharedMvpPerspective;

static NDSRendererAdapterMvpCamera *ndsRendererAdapterSharedMvpCamera(CObj *cobj)
{
    NDSRendererAdapterSharedMvpCameraKey key;
    u16 norm;

    memset(&key, 0, sizeof(key));
    key.eye = cobj->vec.eye;
    key.at = cobj->vec.at;
    key.fovy = cobj->projection.persp.fovy;
    key.aspect = cobj->projection.persp.aspect;
    key.near = cobj->projection.persp.near;
    key.far = cobj->projection.persp.far;
    key.scale = cobj->projection.persp.scale;
    if ((sNdsSharedMvpCameraCobj != (const void *)cobj) ||
        (memcmp(&key, &sNdsSharedMvpCameraKey, sizeof(key)) != 0))
    {
        norm = cobj->projection.persp.norm;
        ndsRendererAdapterCameraPerspFast(&sNdsSharedMvpPerspective, &norm,
                                          cobj->projection.persp.fovy,
                                          cobj->projection.persp.aspect,
                                          cobj->projection.persp.near,
                                          cobj->projection.persp.far,
                                          cobj->projection.persp.scale);
        sNdsSharedMvpCamera.perspective = &sNdsSharedMvpPerspective;
        sNdsSharedMvpCamera.perspective_f_valid = FALSE;
        sNdsSharedMvpCamera.mod1_valid = FALSE;
        sNdsSharedMvpCameraKey = key;
        sNdsSharedMvpCameraCobj = cobj;
        ndsRendererAdapterMvpMemoReset(&sNdsSharedMvpCamera);
    }
    else if (sNdsMvpMemoOwner != &sNdsSharedMvpCamera)
    {
        ndsRendererAdapterMvpMemoReset(&sNdsSharedMvpCamera);
    }
    return &sNdsSharedMvpCamera;
}

static void ndsRendererAdapterApplyMvpRecalc(
    DObj *dobj,
    u32 kind,
    CObj *cobj,
    NDSRendererMatrix20p12 *projection,
    const NDSRendererMatrix20p12 **projection_ptr,
    NDSRendererMatrix20p12 *modelview,
    const NDSRendererMatrix20p12 **modelview_ptr,
    NDSRendererAdapterMvpCamera *camera)
{
    Mtx rotation_mtx;
    Mtx44f local_perspective_f;
    f32 (*perspective_f)[4] = local_perspective_f;
    Mtx44f zrot_f;
    Mtx44f source_orientation_f;
    NDSRendererMatrix20p12 rotation;
    NDSRendererMatrix20p12 local_perspective;
    const NDSRendererMatrix20p12 *perspective = &local_perspective;
    NDSRendererMatrix20p12 source_orientation;
    NDSRendererMatrix20p12 composed;
    s32 translate[4];
    s32 scale_x;
    s32 scale_y;
    f32 sinz;
    f32 cosz;
    f32 recalc_scale_x;
    f32 recalc_scale_y;
    u16 perspective_norm;
    u32 has_battle_camera = FALSE;
    u32 i;
    u32 row;
    u32 col;

    if ((dobj == NULL) || (ndsRendererAdapterIsMvpRecalcKind(kind) == FALSE))
    {
        return;
    }
    NDS_DIAG(gNdsRendererAdapterCustom47DetectedCount++);
    NDS_DIAG(gNdsRendererAdapterCustom47LastXObjsNum = dobj->xobjs_num);
    gNdsRendererAdapterCustom47LastKinds =
        ((dobj->xobjs_num > 0u) && (dobj->xobjs[0] != NULL) ?
            (u32)dobj->xobjs[0]->kind : 0u) |
        (((dobj->xobjs_num > 1u) && (dobj->xobjs[1] != NULL) ?
            (u32)dobj->xobjs[1]->kind : 0u) << 8);
    gNdsRendererAdapterCustom47LastRotateXBits =
        ndsFloatBits(dobj->rotate.vec.f.x);
    gNdsRendererAdapterCustom47LastRotateYBits =
        ndsFloatBits(dobj->rotate.vec.f.y);

    if ((cobj != NULL) && (cobj->xobjs_num > 0))
    {
        for (i = 0u; i < (u32)cobj->xobjs_num; i++)
        {
            if ((cobj->xobjs[i] != NULL) &&
                (cobj->xobjs[i]->kind ==
                    NDS_RENDERER_ADAPTER_GM_CAMERA_MTX_KIND))
            {
                has_battle_camera = TRUE;
                break;
            }
        }
    }
    if ((has_battle_camera == FALSE) &&
        (ndsRendererAdapterIsStagePreviewCamera(cobj) != FALSE))
    {
        has_battle_camera = TRUE;
    }
    if ((cobj == NULL) || (has_battle_camera == FALSE) ||
        (projection == NULL) || (projection_ptr == NULL) ||
        (modelview == NULL) || (modelview_ptr == NULL) ||
        ((*projection_ptr == NULL) && (*modelview_ptr == NULL)))
    {
        gNdsRendererAdapterCustom47RejectCount++;
        if (projection_ptr != NULL) { *projection_ptr = NULL; }
        if (modelview_ptr != NULL) { *modelview_ptr = NULL; }
        return;
    }

    /* Every kind below replaces rows 0-2 of the composed MVP and keeps its
     * translation row, so only that row is formed here, with the full
     * product's arithmetic (P2-2p8, 2026-09-29; the stage billboards'
     * ndsRendererMtxMulRow3_20p12). */
    if ((*modelview_ptr != NULL) && (*projection_ptr != NULL))
    {
        ndsRendererMtxMulRow3_20p12(
            *modelview_ptr, *projection_ptr, &composed);
    }
    else if (*modelview_ptr != NULL)
    {
        composed = **modelview_ptr;
    }
    else
    {
        composed = **projection_ptr;
    }
    for (col = 0u; col < 4u; col++)
    {
        translate[col] = composed.m[3][col];
    }

    /* Both callbacks replace all three orientation rows of the current MVP with
     * something built from gGCMatrixPerspF alone.  The translation row from
     * Tra * LookAt * Persp stays live.  The battle camera's adapter projection
     * contains LookAt * Persp, so seed the renderer with the completed MVP to
     * avoid applying LookAt a second time to the rewritten orientation. */
    perspective_norm = cobj->projection.persp.norm;
    if (camera != NULL)
    {
        perspective = camera->perspective;
    }
    else
    {
        ndsRendererAdapterCameraPerspFast(&local_perspective,
                                      &perspective_norm,
                                      cobj->projection.persp.fovy,
                                      cobj->projection.persp.aspect,
                                      cobj->projection.persp.near,
                                      cobj->projection.persp.far,
                                      cobj->projection.persp.scale);
    }
    if ((kind == nGCMatrixKind46) ||
        (kind == NDS_RENDERER_ADAPTER_MVP_RECALC_Z_0X46_KIND) ||
        (kind == NDS_RENDERER_ADAPTER_EF_GROUND_BILLBOARD_KIND))
    {
        if (camera != NULL)
        {
            if (camera->perspective_f_valid == FALSE)
            {
                ndsRendererAdapterMvpPerspectiveF(cobj, camera->perspective_f);
                camera->perspective_f_valid = TRUE;
            }
            perspective_f = camera->perspective_f;
        }
        else
        {
            ndsRendererAdapterMvpPerspectiveF(cobj, local_perspective_f);
        }
    }
    if (kind == NDS_RENDERER_ADAPTER_MVP_RECALC_PERSP_SCA_KIND)
    {
        NDSRendererMatrix20p12 scale;

        /* objdisplay.c:876. Only four elements of the perspective block
         * survive, each scaled: [0][0] and [2][2]/[2][3] by gGCScaleX, [1][1]
         * by gGCScaleX * scale.y. gGCScaleX is 1.0 at
         * gcDrawDObjTreeDLLinksForGObj and is multiplied by scale.x HERE, so
         * for a tree with one recalc node -- which is what the shield desc
         * builds -- gGCScaleX is exactly this DObj's scale.x. A chain with two
         * recalc nodes would accumulate; none exists in the P1 scene, and the
         * apply counter below is what would show one appearing.
         *
         * diag(sx, sy, sx) * Persp reproduces those four writes and nothing
         * else, because syMatrixPerspFast leaves every off-diagonal of rows 0-2
         * at zero -- so this reuses the shared fixed-point multiply instead of
         * hand-rolling four 20.12 products.
         *
         * Both factors are formed in FLOAT first, exactly as source does, so
         * the guard-size term (see sNdsRendererAdapterMvpRecalcScaleX) is not
         * quantized twice. */
        if ((ndsRendererAdapterFloatPow2ToS32(
                 sNdsRendererAdapterMvpRecalcScaleX * dobj->scale.vec.f.x,
                 12u, &scale_x) == FALSE) ||
            (ndsRendererAdapterFloatPow2ToS32(
                 sNdsRendererAdapterMvpRecalcScaleX * dobj->scale.vec.f.y,
                 12u, &scale_y) == FALSE))
        {
            gNdsRendererAdapterCustom47RejectCount++;
            return;
        }
        ndsRendererAdapterMtxIdentity20p12(&scale);
        scale.m[0][0] = scale_x;
        scale.m[1][1] = scale_y;
        scale.m[2][2] = scale_x;
        ndsRendererMtxMul20p12(&scale, perspective, &source_orientation);
        NDS_DIAG(gNdsRendererAdapterMvpRecalcPerspScaCount++);
    }
    else if (kind == nGCMatrixKind48)
    {
        f32 parent_scale_x;
        NDSRendererAdapterMvpMod1Q local_mod1;
        const NDSRendererAdapterMvpMod1Q *mod1 = &local_mod1;

        if (ndsRendererAdapterMvpParentScaleX(dobj, &parent_scale_x) == FALSE)
        {
            gNdsRendererAdapterCustom47RejectCount++;
            *projection_ptr = NULL;
            *modelview_ptr = NULL;
            return;
        }
        /* gmCameraDefaultProcDisplay selects camera matrix mode 3. Its Mod1
         * removes yaw, preserves the vertical eye/target relation, and then
         * multiplies by perspective (objdisplay.c:3094-3118); see
         * ndsRendererAdapterMvpMod1Q. */
        if (camera != NULL)
        {
            if (camera->mod1_valid == FALSE)
            {
                ndsRendererAdapterMvpMod1Q(cobj, &camera->mod1);
                camera->mod1_valid = TRUE;
            }
            mod1 = &camera->mod1;
        }
        else
        {
            ndsRendererAdapterMvpMod1Q(cobj, &local_mod1);
        }
        recalc_scale_x = parent_scale_x * dobj->scale.vec.f.x;
        recalc_scale_y = parent_scale_x * dobj->scale.vec.f.y;
        {
            const u32 sx_bits = ndsFloatBits(recalc_scale_x);
            const u32 sy_bits = ndsFloatBits(recalc_scale_y);
            u32 slot = 0u;
            sb32 cached = FALSE;

            if (ndsRendererAdapterMvpMemoFor(camera))
            {
                for (slot = 0u; slot < sNdsMvpMemo48Count; slot++)
                {
                    if ((sNdsMvpMemo48Key[slot][0] == sx_bits) &&
                        (sNdsMvpMemo48Key[slot][1] == sy_bits))
                    {
                        source_orientation = sNdsMvpMemo48Rows[slot];
                        cached = TRUE;
                        break;
                    }
                }
            }
            if (cached == FALSE)
            {
                ndsRendererAdapterMvpKind48RowsQ(mod1, perspective,
                                                 recalc_scale_x,
                                                 recalc_scale_y,
                                                 &source_orientation);
                if (ndsRendererAdapterMvpMemoFor(camera) &&
                    (sNdsMvpMemo48Count < NDS_MVP_MEMO_48))
                {
                    slot = sNdsMvpMemo48Count++;
                    sNdsMvpMemo48Key[slot][0] = sx_bits;
                    sNdsMvpMemo48Key[slot][1] = sy_bits;
                    sNdsMvpMemo48Rows[slot] = source_orientation;
                }
            }
        }
        sNdsRendererAdapterMvpRecalcScaleX = recalc_scale_x;
    }
    else if ((kind == nGCMatrixKind46) ||
             (kind == NDS_RENDERER_ADAPTER_MVP_RECALC_Z_0X46_KIND))
    {
        /* objdisplay.c:960-1004 (kind 46) and lbcommon.c:1702-1771
         * (custom 0x46).  Both callbacks rewrite rows 0-2 of the CURRENT MVP
         * from gGCMatrixPerspF and leave row 3 alone.  Build those source rows
         * in float, convert once like the source's FTOFIX32 writes, and let the
         * common tail below restore the already-composed translation row.
         *
         * The built-in kind also carries the source's odd gGCScaleX contract:
         * X and Z use prior_scale_x * scale.x, Y uses prior_scale_x * scale.y.
         * Custom 0x46 is the unscaled Z-only version used by Samus Bomb. */
        sb32 orientation_ready = FALSE;

        memset(&source_orientation_f, 0, sizeof(source_orientation_f));
        source_orientation_f[3][3] = 1.0F;

        if (kind == nGCMatrixKind46)
        {
            /* Row core shared above (EF-GROUND seam): the efground lr +/-3
             * drawables reach this arm with the same threading the kind72
             * drawables use. */
            float ef_ground_kind46_rows[3][4];
            const DObj *scale_ancestor;

            /* gGCScaleX is a TREE accumulator in the source: every
             * scale-bearing matrix kind an ancestor owns multiplies it
             * (objdisplay.c:590/627/664/670) before this billboard reads it.
             * The port resets it per DObj and only the joint attach raised
             * it, so a billboard under scaled parents drew at the wrong size:
             * Yoshi's Egg Lay root carries the captive's effect_size
             * (efManagerYoshiEggLayMakeEffect) and its child a
             * TraRotRpyRSca, and the egg came out far too small. */
            for (scale_ancestor = dobj->parent;
                 (scale_ancestor != NULL) &&
                 (scale_ancestor != DOBJ_PARENT_NULL);
                 scale_ancestor = scale_ancestor->parent)
            {
                u32 xobj_index;

                for (xobj_index = 0u;
                     xobj_index < (u32)scale_ancestor->xobjs_num;
                     xobj_index++)
                {
                    const XObj *xobj = scale_ancestor->xobjs[xobj_index];

                    if ((xobj != NULL) &&
                        ((xobj->kind == nGCMatrixKindTraRotRSca) ||
                         (xobj->kind == nGCMatrixKindTraRotRpyRSca) ||
                         (xobj->kind == nGCMatrixKindTraRotPyrRSca) ||
                         (xobj->kind == nGCMatrixKindSca)))
                    {
                        sNdsRendererAdapterMvpRecalcScaleX *=
                            scale_ancestor->scale.vec.f.x;
                    }
                }
            }

            {
                const u32 key[4] = {
                    ndsFloatBits(dobj->rotate.vec.f.z),
                    ndsFloatBits(dobj->scale.vec.f.x),
                    ndsFloatBits(dobj->scale.vec.f.y),
                    ndsFloatBits(sNdsRendererAdapterMvpRecalcScaleX)
                };
                const sb32 memo = (ndsRendererAdapterMvpMemoFor(camera) &&
                    (dobj != sNdsRendererAdapterCustom4ARollDObj)) ?
                    TRUE : FALSE;
                u32 slot;

                if (memo != FALSE)
                {
                    for (slot = 0u; slot < sNdsMvpMemo46Count; slot++)
                    {
                        if ((sNdsMvpMemo46Key[slot][0] == key[0]) &&
                            (sNdsMvpMemo46Key[slot][1] == key[1]) &&
                            (sNdsMvpMemo46Key[slot][2] == key[2]) &&
                            (sNdsMvpMemo46Key[slot][3] == key[3]))
                        {
                            union { u32 u; f32 f; } after;

                            after.u = sNdsMvpMemo46AccAfter[slot];
                            sNdsRendererAdapterMvpRecalcScaleX = after.f;
                            source_orientation = sNdsMvpMemo46Rows[slot];
                            orientation_ready = TRUE;
                            break;
                        }
                    }
                }
                if (orientation_ready == FALSE)
                {
                    ndsRendererAdapterEfGroundKind46Rows(perspective_f,
                        dobj->rotate.vec.f.z,
                        dobj->scale.vec.f.x, dobj->scale.vec.f.y,
                        &sNdsRendererAdapterMvpRecalcScaleX,
                        ef_ground_kind46_rows);
                    if (dobj == sNdsRendererAdapterCustom4ARollDObj)
                    {
                        /* These rows just consumed the roll kind 0x4A wrote
                         * during this prepare's world build. Bits, not a
                         * float compare. */
                        u32 roll_bits = ndsFloatBits(dobj->rotate.vec.f.z);

                        sNdsRendererAdapterCustom4ARollDObj = NULL;
                        gNdsRendererAdapterCustom4AWitness[2] = roll_bits;
                        if ((roll_bits & 0x7fffffffu) != 0u)
                        {
                            gNdsRendererAdapterCustom4AWitness[1]++;
                        }
                        DC_FlushRange(
                            (const void *)(uintptr_t)
                                gNdsRendererAdapterCustom4AWitness,
                            sizeof(gNdsRendererAdapterCustom4AWitness));
                    }
                    for (row = 0u; row < 3u; row++)
                    {
                        for (col = 0u; col < 4u; col++)
                        {
                            source_orientation_f[row][col] =
                                ef_ground_kind46_rows[row][col];
                        }
                    }
                    syMatrixF2L(&source_orientation_f, &rotation_mtx);
                    ndsRendererAdapterMtxFromN64(&rotation_mtx,
                                                 &source_orientation);
                    orientation_ready = TRUE;
                    if ((memo != FALSE) &&
                        (sNdsMvpMemo46Count < NDS_MVP_MEMO_46))
                    {
                        slot = sNdsMvpMemo46Count++;
                        sNdsMvpMemo46Key[slot][0] = key[0];
                        sNdsMvpMemo46Key[slot][1] = key[1];
                        sNdsMvpMemo46Key[slot][2] = key[2];
                        sNdsMvpMemo46Key[slot][3] = key[3];
                        sNdsMvpMemo46AccAfter[slot] =
                            ndsFloatBits(sNdsRendererAdapterMvpRecalcScaleX);
                        sNdsMvpMemo46Rows[slot] = source_orientation;
                    }
                }
            }
        }
        else
        {
            /* Only the custom kind reads the Z rotation's sine and cosine;
             * kind 46 used to evaluate them here too and discard them. */
            syMatrixRotRpyRF(&zrot_f, 0.0F, 0.0F, dobj->rotate.vec.f.z);
            cosz = zrot_f[0][0];
            sinz = zrot_f[0][1];
            for (col = 0u; col < 4u; col++)
            {
                source_orientation_f[0][col] =
                    (perspective_f[0][col] * cosz) +
                    (perspective_f[1][col] * sinz);
                source_orientation_f[1][col] =
                    (perspective_f[0][col] * -sinz) +
                    (perspective_f[1][col] * cosz);
                source_orientation_f[2][col] = perspective_f[2][col];
            }
        }
        if (orientation_ready == FALSE)
        {
            syMatrixF2L(&source_orientation_f, &rotation_mtx);
            ndsRendererAdapterMtxFromN64(&rotation_mtx, &source_orientation);
        }
    }
    else if (kind == NDS_RENDERER_ADAPTER_EF_GROUND_BILLBOARD_KIND)
    {
        /* func_ovl0_800CAB48 (lbcommon.c:1852-1932): persp rows x rotX/rotY,
         * rows 0+2 by scalex (the threaded global *= scale.x), row 1 by
         * scaley (scale.y * the pre-mutation global). Rows core shared above;
         * convert once like the source's FTOFIX32 writes, and let the common
         * tail below restore the already-composed translation row. */
        float ef_ground_rows[3][4];

        ndsRendererAdapterEfGroundBillboardRows(perspective_f,
            dobj->rotate.vec.f.x, dobj->rotate.vec.f.y,
            dobj->scale.vec.f.x, dobj->scale.vec.f.y,
            &sNdsRendererAdapterMvpRecalcScaleX, ef_ground_rows);
        memset(&source_orientation_f, 0, sizeof(source_orientation_f));
        source_orientation_f[3][3] = 1.0F;
        for (row = 0u; row < 3u; row++)
        {
            for (col = 0u; col < 4u; col++)
            {
                source_orientation_f[row][col] = ef_ground_rows[row][col];
            }
        }
        syMatrixF2L(&source_orientation_f, &rotation_mtx);
        ndsRendererAdapterMtxFromN64(&rotation_mtx, &source_orientation);
    }
    else
    {
        /* func_ovl0_800CA5C8: RotRpyR(x, y, 0) * gGCMatrixPerspF. */
        syMatrixRotRpyR(&rotation_mtx,
                        dobj->rotate.vec.f.x,
                        dobj->rotate.vec.f.y,
                        0.0F);
        ndsRendererAdapterMtxFromN64(&rotation_mtx, &rotation);
        ndsRendererMtxMul20p12(
            &rotation, perspective, &source_orientation);
    }
    for (row = 0u; row < 3u; row++)
    {
        for (col = 0u; col < 4u; col++)
        {
            composed.m[row][col] = source_orientation.m[row][col];
        }
    }
    for (col = 0u; col < 4u; col++)
    {
        if (composed.m[3][col] != translate[col])
        {
            gNdsRendererAdapterCustom47TranslationMismatchCount++;
            composed.m[3][col] = translate[col];
        }
    }
    *modelview = composed;
    *modelview_ptr = modelview;
    *projection_ptr = NULL;
    gNdsRendererAdapterCustom47LastTranslateX20p12 =
        (u32)modelview->m[3][0];
    gNdsRendererAdapterCustom47LastTranslateY20p12 =
        (u32)modelview->m[3][1];
    gNdsRendererAdapterCustom47LastTranslateZ20p12 =
        (u32)modelview->m[3][2];
    if (kind == nGCMatrixKind46)
    {
        NDS_DIAG(gNdsRendererAdapterKind46AppliedCount++);
    }
    else if (kind == NDS_RENDERER_ADAPTER_MVP_RECALC_Z_0X46_KIND)
    {
        NDS_DIAG(gNdsRendererAdapterCustom46AppliedCount++);
    }
    gNdsRendererAdapterCustom47AppliedCount++;
}

/* Cycle 99, charter section 3.12: the baked fighter draw plan holds DObj and
 * loaded-file pointers, so it is re-derived at every scene entry -- including a
 * START-restart out of Results -- and never trusted across one. Defined with
 * the plan itself, next to the draw. */
static void ndsFighterDrawPlanInvalidate(void);
static void ndsRendererAdapterResetGxSlotTables(void);

static void ndsRendererAdapterResetSceneCaches(void)
{
    ndsRendererHardwareResetSourceCaches();
    ndsRendererResetNativeStageValidationCache();
#if NDS_P2_STAGE_SECTOR
    gNdsSectorArwingBasisDecline = 0u;
#endif
    ndsFighterDrawPlanInvalidate();
    ndsRendererAdapterResetGxSlotTables();
    sNdsRendererAdapterCameraCacheFrame = 0u;
    sNdsRendererAdapterCameraCacheCount = 0u;
    sNdsRendererAdapterDObjWorldCache = NULL;
    sNdsRendererAdapterDObjWorldCacheFrame = 0u;
    sNdsRendererAdapterDObjWorldCacheCount = 0u;
    sNdsRendererAdapterDObjWorldCacheDynamicLimit =
        NDS_RENDERER_ADAPTER_DOBJ_WORLD_CACHE_COUNT;
    sNdsRendererAdapterDObjWorldCacheAllocationAttempted = FALSE;
#if defined(NDS_LAB_FOURCPU_SWEEP) && NDS_LAB_FOURCPU_SWEEP
    gNdsWorldCacheAllocatedMask = 0u;
#endif
    memset(sNdsRendererAdapterDObjWorldIndex, 0,
           sizeof(sNdsRendererAdapterDObjWorldIndex));
    sNdsRendererAdapterStageWorldCache = NULL;
    sNdsRendererAdapterStageWorldCacheCount = 0u;
    sNdsRendererAdapterStageWorldCacheAllocationAttempted = FALSE;
    sNdsRendererAdapterStageWorldNextGeneration = 1u;
    memset(sNdsRendererAdapterStageWorldIndex, 0,
           sizeof(sNdsRendererAdapterStageWorldIndex));
    memset(&sNdsRendererAdapterNativeStageWorkspace, 0,
           sizeof(sNdsRendererAdapterNativeStageWorkspace));
}

static u32 ndsRendererAdapterDObjWorldIndexHash(const DObj *dobj)
{
    uintptr_t key = (uintptr_t)dobj >> 2;

    key ^= key >> 8;
    key ^= key >> 16;
    return (u32)key & NDS_RENDERER_ADAPTER_DOBJ_WORLD_INDEX_MASK;
}

/* The optional world caches' heap reserve (2026-09-30). The first battle
 * frame keeps the runtime reserve: the countdown still allocates (threads,
 * entry effects; ~60 KB on Sector Z's default roster). An attempt that
 * misses it is retried once at GO, where what the match still takes is small
 * on the short-heap rosters (8-13 KB after GO on the tightest lab arms) and
 * the larger post-GO allocations are caches that size themselves to the free
 * heap: that attempt keeps the GObj latch's 25,600 floor plus headroom.
 * Sector Z's default roster missed the first-frame reserve by ~6 KB and drew
 * every stage DObj without these caches (STG +23.5K a frame).
 * Receipt 2026-09-30_p2-2p8-native-owners. */
#define NDS_RENDERER_ADAPTER_WORLD_CACHE_GO_RESERVE (48u * 1024u)

/* 1 before GO, 2 from GO in a VS battle: the attempt phase. */
static u32 ndsRendererAdapterWorldCachePhase(void)
{
    return ((gSCManagerSceneData.scene_curr == nSCKindVSBattle) &&
            (gSCManagerBattleState != NULL) &&
            (gSCManagerBattleState->game_status == nSCBattleGameStatusGo)) ?
        2u : 1u;
}

static size_t ndsRendererAdapterWorldCacheReserve(u32 phase)
{
    return (phase >= 2u) ? (size_t)NDS_RENDERER_ADAPTER_WORLD_CACHE_GO_RESERVE :
                           (size_t)NDS_RELOC_MEMORY_LEDGER_RESERVE_BYTES;
}

static sb32 ndsRendererAdapterEnsureDObjWorldCache(void)
{
    uintptr_t aligned;
    size_t bytes = sizeof(NDSRendererAdapterDObjWorldCacheEntry) *
        NDS_RENDERER_ADAPTER_DOBJ_WORLD_CACHE_COUNT;
    u32 phase;

    if (sNdsRendererAdapterDObjWorldCache != NULL)
    {
        return TRUE;
    }
    phase = ndsRendererAdapterWorldCachePhase();
    if (sNdsRendererAdapterDObjWorldCacheAllocationAttempted >= phase)
    {
        return FALSE;
    }
    sNdsRendererAdapterDObjWorldCacheAllocationAttempted = phase;
#if defined(NDS_LAB_FOURCPU_SWEEP) && NDS_LAB_FOURCPU_SWEEP
    gNdsDObjWorldCacheFreeAtAttempt =
        (u32)((uintptr_t)gSYTaskmanGeneralHeap.end -
              (uintptr_t)gSYTaskmanGeneralHeap.ptr);
#endif
    if ((gSYTaskmanGeneralHeap.ptr == NULL) ||
        (gSYTaskmanGeneralHeap.end == NULL))
    {
        return FALSE;
    }
    aligned = ((uintptr_t)gSYTaskmanGeneralHeap.ptr + 0x0fu) &
        ~(uintptr_t)0x0fu;
    /* This is optional draw acceleration. Honor the scene ledger's runtime
     * reserve; otherwise it can trigger the source object cap
     * during the countdown. A miss uses the existing native matrix builder.
     * From GO the reserve is ndsRendererAdapterWorldCacheReserve's. */
    if ((aligned > (uintptr_t)gSYTaskmanGeneralHeap.end) ||
        (bytes > ((uintptr_t)gSYTaskmanGeneralHeap.end - aligned)) ||
        (((uintptr_t)gSYTaskmanGeneralHeap.end - aligned - bytes) <
         ndsRendererAdapterWorldCacheReserve(phase)))
    {
        return FALSE;
    }
    sNdsRendererAdapterDObjWorldCache =
        (NDSRendererAdapterDObjWorldCacheEntry *)syTaskmanMalloc(bytes, 0x10u);
#if defined(NDS_LAB_FOURCPU_SWEEP) && NDS_LAB_FOURCPU_SWEEP
    if (sNdsRendererAdapterDObjWorldCache != NULL)
    {
        gNdsWorldCacheAllocatedMask |= 1u;
    }
#endif
    /* This DS cache now consumes the original scene heap, so keep the P1
     * reserve ledger truthful after its lazy first-frame allocation. */
    ndsRelocUpdateMemoryLedger();
    return (sNdsRendererAdapterDObjWorldCache != NULL) ? TRUE : FALSE;
}

static const NDSRendererMatrix20p12 *
ndsRendererAdapterFindDObjWorldMatrix(const DObj *dobj)
{
    u32 frame = gNdsRendererProfileFrameCount;
    u32 slot;
    u32 probe;

    if (sNdsRendererAdapterDObjWorldCacheFrame != frame)
    {
        sNdsRendererAdapterDObjWorldCacheFrame = frame;
        sNdsRendererAdapterDObjWorldCacheCount = 0u;
        memset(sNdsRendererAdapterDObjWorldIndex, 0,
               sizeof(sNdsRendererAdapterDObjWorldIndex));
    }
    if (ndsRendererAdapterEnsureDObjWorldCache() == FALSE)
    {
        return NULL;
    }
    slot = ndsRendererAdapterDObjWorldIndexHash(dobj);
    for (probe = 0u;
         probe < NDS_RENDERER_ADAPTER_DOBJ_WORLD_INDEX_COUNT;
         probe++)
    {
        u32 encoded = sNdsRendererAdapterDObjWorldIndex[slot];
        u32 cache_index;

        if (encoded == 0u)
        {
            break;
        }
        cache_index = encoded - 1u;
        if ((cache_index < sNdsRendererAdapterDObjWorldCacheCount) &&
            (sNdsRendererAdapterDObjWorldCache[cache_index].dobj == dobj))
        {
            return &sNdsRendererAdapterDObjWorldCache[cache_index].world;
        }
        slot = (slot + 1u) & NDS_RENDERER_ADAPTER_DOBJ_WORLD_INDEX_MASK;
    }
    return NULL;
}

static void ndsRendererAdapterStoreDObjWorldMatrix(
    const DObj *dobj, const NDSRendererMatrix20p12 *world)
{
    NDSRendererAdapterDObjWorldCacheEntry *entry;
    u32 cache_index;
    u32 slot;
    u32 probe;

    if ((dobj == NULL) || (world == NULL))
    {
        return;
    }
    if ((sNdsRendererAdapterDObjWorldCache == NULL) ||
        (sNdsRendererAdapterDObjWorldCacheCount >=
         sNdsRendererAdapterDObjWorldCacheDynamicLimit))
    {
        return;
    }
    cache_index = sNdsRendererAdapterDObjWorldCacheCount++;
    entry = &sNdsRendererAdapterDObjWorldCache[cache_index];
    entry->dobj = dobj;
    /* R2-03 E69. 64-byte struct assignment -> `bl memcpy`; this function is
     * inlined into ndsRendererAdapterBuildDObjWorldMatrix, which E68b measured
     * at 9.4% of the memset/memcpy class. E68's first run misattributed these
     * very samples to the hash below, which copies nothing. */
    ndsRendererMatrixCopy20p12(&entry->world, world);
    slot = ndsRendererAdapterDObjWorldIndexHash(dobj);
    for (probe = 0u;
         probe < NDS_RENDERER_ADAPTER_DOBJ_WORLD_INDEX_COUNT;
         probe++)
    {
        if (sNdsRendererAdapterDObjWorldIndex[slot] == 0u)
        {
            sNdsRendererAdapterDObjWorldIndex[slot] = (u8)(cache_index + 1u);
            return;
        }
        slot = (slot + 1u) & NDS_RENDERER_ADAPTER_DOBJ_WORLD_INDEX_MASK;
    }
}

static u32 ndsRendererAdapterStageWorldIndexHash(const DObj *dobj)
{
    uintptr_t key = (uintptr_t)dobj >> 2;

    key ^= key >> 7;
    key ^= key >> 15;
    return (u32)key & NDS_RENDERER_ADAPTER_STAGE_WORLD_INDEX_MASK;
}

static sb32 ndsRendererAdapterEnsureStageWorldCache(void)
{
    uintptr_t aligned;
    size_t bytes = sizeof(NDSRendererAdapterStageWorldCacheEntry) *
        NDS_RENDERER_ADAPTER_STAGE_WORLD_CACHE_COUNT;
    u32 phase;

    if (sNdsRendererAdapterStageWorldCache != NULL)
    {
        return TRUE;
    }
    phase = ndsRendererAdapterWorldCachePhase();
    if (sNdsRendererAdapterStageWorldCacheAllocationAttempted >= phase)
    {
        return FALSE;
    }
    if ((ndsRendererAdapterEnsureDObjWorldCache() == FALSE) ||
        (sNdsRendererAdapterDObjWorldCacheCount >
         NDS_RENDERER_ADAPTER_STAGE_WORLD_SLOT_BASE))
    {
        return FALSE;
    }
    sNdsRendererAdapterStageWorldCacheAllocationAttempted = phase;
#if defined(NDS_LAB_FOURCPU_SWEEP) && NDS_LAB_FOURCPU_SWEEP
    gNdsStageWorldCacheFreeAtAttempt =
        (u32)((uintptr_t)gSYTaskmanGeneralHeap.end -
              (uintptr_t)gSYTaskmanGeneralHeap.ptr);
#endif
    if ((gSYTaskmanGeneralHeap.ptr == NULL) ||
        (gSYTaskmanGeneralHeap.end == NULL))
    {
        return FALSE;
    }
    aligned = ((uintptr_t)gSYTaskmanGeneralHeap.ptr + 0x0fu) &
        ~(uintptr_t)0x0fu;
    if ((aligned > (uintptr_t)gSYTaskmanGeneralHeap.end) ||
        (bytes > ((uintptr_t)gSYTaskmanGeneralHeap.end - aligned)) ||
        (((uintptr_t)gSYTaskmanGeneralHeap.end - aligned - bytes) <
         ndsRendererAdapterWorldCacheReserve(phase)))
    {
        return FALSE;
    }
    sNdsRendererAdapterStageWorldCache =
        (NDSRendererAdapterStageWorldCacheEntry *)
            syTaskmanMalloc(bytes, 0x10u);
#if defined(NDS_LAB_FOURCPU_SWEEP) && NDS_LAB_FOURCPU_SWEEP
    if (sNdsRendererAdapterStageWorldCache != NULL)
    {
        gNdsWorldCacheAllocatedMask |= 2u;
    }
#endif
    if (sNdsRendererAdapterStageWorldCache != NULL)
    {
        memset(sNdsRendererAdapterStageWorldCache, 0, bytes);
        sNdsRendererAdapterDObjWorldCacheDynamicLimit =
            NDS_RENDERER_ADAPTER_STAGE_WORLD_SLOT_BASE;
        ndsRelocUpdateMemoryLedger();
        return TRUE;
    }
    return FALSE;
}

static void ndsRendererAdapterPrepareWorldCaches(void)
{
    /* Reserve the existing 8,704/4,608-byte caches before the animation
     * cache sizes itself from the arena remainder. First-draw allocation
     * otherwise spends the runtime headroom that sizing just preserved. */
    (void)ndsRendererAdapterEnsureDObjWorldCache();
    (void)ndsRendererAdapterEnsureStageWorldCache();
}

static NDSRendererAdapterStageWorldCacheEntry *
__attribute__((section(".itcm")))
ndsRendererAdapterFindStageWorldEntry(const DObj *dobj)
{
    u32 slot;
    u32 probe;

    if ((dobj == NULL) ||
        (ndsRendererAdapterEnsureStageWorldCache() == FALSE))
    {
        return NULL;
    }
    slot = ndsRendererAdapterStageWorldIndexHash(dobj);
    for (probe = 0u;
         probe < NDS_RENDERER_ADAPTER_STAGE_WORLD_INDEX_COUNT;
         probe++)
    {
        u32 encoded = sNdsRendererAdapterStageWorldIndex[slot];
        u32 cache_index;

        if (encoded == 0u)
        {
            return NULL;
        }
        cache_index = encoded - 1u;
        if ((cache_index < sNdsRendererAdapterStageWorldCacheCount) &&
            (sNdsRendererAdapterStageWorldCache[cache_index].dobj == dobj))
        {
            return &sNdsRendererAdapterStageWorldCache[cache_index];
        }
        slot = (slot + 1u) & NDS_RENDERER_ADAPTER_STAGE_WORLD_INDEX_MASK;
    }
    return NULL;
}

static NDSRendererAdapterStageWorldCacheEntry *
ndsRendererAdapterFindOrAllocateStageWorldEntry(const DObj *dobj)
{
    NDSRendererAdapterStageWorldCacheEntry *entry;
    u32 cache_index;
    u32 slot;
    u32 probe;

    entry = ndsRendererAdapterFindStageWorldEntry(dobj);
    if (entry != NULL)
    {
        return entry;
    }
    if ((sNdsRendererAdapterStageWorldCache == NULL) ||
        (sNdsRendererAdapterStageWorldCacheCount >=
         NDS_RENDERER_ADAPTER_STAGE_WORLD_CACHE_COUNT))
    {
        return NULL;
    }
    cache_index = sNdsRendererAdapterStageWorldCacheCount++;
    entry = &sNdsRendererAdapterStageWorldCache[cache_index];
    memset(entry, 0, sizeof(*entry));
    entry->dobj = dobj;
    /* A zero frame is valid in accelerated harnesses. Ensure a freshly
     * allocated entry cannot take the same-frame shortcut before its first
     * exact world build; no u32 equals its own bitwise complement. */
    entry->validated_frame = ~gNdsRendererProfileFrameCount;
    entry->world_slot = (u8)(NDS_RENDERER_ADAPTER_STAGE_WORLD_SLOT_BASE +
                             cache_index);
    sNdsRendererAdapterDObjWorldCache[entry->world_slot].dobj = dobj;
    slot = ndsRendererAdapterStageWorldIndexHash(dobj);
    for (probe = 0u;
         probe < NDS_RENDERER_ADAPTER_STAGE_WORLD_INDEX_COUNT;
         probe++)
    {
        if (sNdsRendererAdapterStageWorldIndex[slot] == 0u)
        {
            sNdsRendererAdapterStageWorldIndex[slot] =
                (u8)(cache_index + 1u);
            return entry;
        }
        slot = (slot + 1u) & NDS_RENDERER_ADAPTER_STAGE_WORLD_INDEX_MASK;
    }
    return NULL;
}

static NDSRendererMatrix20p12 *
ndsRendererAdapterStageWorldEntryMatrix(
    const NDSRendererAdapterStageWorldCacheEntry *entry)
{
    return &sNdsRendererAdapterDObjWorldCache[entry->world_slot].world;
}

static sb32 ndsRendererAdapterCaptureStageWorldSourceKey(
    DObj *dobj, NDSRendererAdapterStageWorldSourceKey *source_key)
{
    u32 i;

    if ((dobj == NULL) || (source_key == NULL) ||
        (dobj->xobjs_num > 5u) || (dobj->vec != NULL))
    {
        return FALSE;
    }
    memset(source_key, 0, sizeof(*source_key));
    memcpy(source_key->base_translate, &dobj->translate.vec.f,
           sizeof(source_key->base_translate));
    memcpy(&source_key->base_rotate[0], &dobj->rotate.a,
           sizeof(source_key->base_rotate[0]));
    memcpy(&source_key->base_rotate[1], &dobj->rotate.vec.f,
           sizeof(source_key->base_rotate) -
               sizeof(source_key->base_rotate[0]));
    memcpy(source_key->base_scale, &dobj->scale.vec.f,
           sizeof(source_key->base_scale));

    source_key->xobjs_num = dobj->xobjs_num;
    for (i = 0u; i < dobj->xobjs_num; i++)
    {
        XObj *xobj = dobj->xobjs[i];

        if (xobj == NULL)
        {
            continue;
        }
        source_key->xobj_present_mask |= (u8)(1u << i);
        source_key->xobj_kinds[i] = xobj->kind;
        /* Kind 1 consumes an arbitrary 64-byte matrix. Kinds 33-40 consume
         * live camera state, and 0x4B and 0x4F consume live FTParts state. Keep
         * those exact paths frame-local instead of enlarging this first
         * stage-only proof cache.
         *
         * 0x4F was measured missing from this list on 2026-08-04 and it is the
         * reason the shield's attachment fix produced a byte-identical frame:
         * the key hashes the DObj's OWN translate/rotate/scale, which for an
         * attached effect root are the constants 0/0/1, so the entry matched on
         * every later frame and the world matrix was built exactly once per
         * guard -- one LOCAL build against ten submits, on the probe. A cached
         * matrix is only sound when the local build is a pure function of what
         * the key covers, and both fighter-parts kinds read state outside it.
         *
         * 0x50 joined the list on 2026-08-04 for the identical reason, measured
         * the identical way: the rebirth halo's root carries it, its own
         * translate/rotate/scale are the same constants 0/0/1, and the probe saw
         * ndsRendererAdapterBuildDObjLocalMatrix fire ONCE per spawn -- one
         * build for the whole ~90-tic life of the platform. Adding the kind case
         * without adding this line would have moved a frozen matrix instead of
         * tracking the fighter, which is exactly what cycle 52 already paid for
         * once. Sector's 0x53 has the same cache contract: its basis comes from
         * live TraI AObjs and its x translation also reads arwing_target_x,
         * neither of which is represented in this key. Any future custom kind
         * that reads live state outside the DObj TRS belongs here in the same
         * commit that teaches the builder about it. 0x4A reads the bound
         * joint's pitch and world matrix, and writes this DObj's own rotate.z
         * after the key is captured. */
        if ((xobj->kind == 1u) ||
            ((xobj->kind >= 33u) && (xobj->kind <= 40u)) ||
            (xobj->kind == NDS_RENDERER_ADAPTER_FIGHTER_PARTS_MTX_KIND) ||
            (xobj->kind == NDS_RENDERER_ADAPTER_JOINT_ATTACH_MTX_KIND) ||
            (xobj->kind == NDS_RENDERER_ADAPTER_JOINT_ATTACH_TRA_MTX_KIND) ||
            (xobj->kind == NDS_RENDERER_ADAPTER_JOINT_PITCH_TO_ROLL_KIND) ||
            (xobj->kind == NDS_RENDERER_ADAPTER_ITEM_ATTACH_MTX_KIND) ||
            (xobj->kind == NDS_RENDERER_ADAPTER_SECTOR_ARWING_MTX_KIND))
        {
            return FALSE;
        }
    }
    return TRUE;
}

/* 2026-09-07 this 222-byte compare left ITCM to fix a 16-byte link overflow.
 * 2026-09-27: back in (census: ~1.0K ticks/frame non-memory stall in main
 * RAM), in the room two never-executed old-path residents left. */
/* The four base-vector compares read the float fields as words: the same
 * bitwise equality memcmp tested, without four library calls a node (129 a
 * frame on the four-CPU census, ~3.4K ticks of memcmp). */
typedef u32 __attribute__((may_alias)) NDSRendererAdapterAliasU32;

static inline sb32 ndsRendererAdapterWords3Equal(const u32 *key,
                                                 const void *live)
{
    const NDSRendererAdapterAliasU32 *w =
        (const NDSRendererAdapterAliasU32 *)live;

    return ((key[0] == w[0]) && (key[1] == w[1]) && (key[2] == w[2])) ?
        TRUE : FALSE;
}

static sb32 NDS_R2_ITCM_PACK2_CODE ndsRendererAdapterStageWorldSourceKeyMatches(
    DObj *dobj, const NDSRendererAdapterStageWorldSourceKey *source_key)
{
    u32 i;

    if ((dobj == NULL) || (source_key == NULL) ||
        (dobj->xobjs_num > 5u) || (dobj->vec != NULL) ||
        (dobj->xobjs_num != source_key->xobjs_num) ||
        (ndsRendererAdapterWords3Equal(source_key->base_translate,
                                       &dobj->translate.vec.f) == FALSE) ||
        (source_key->base_rotate[0] !=
         *(const NDSRendererAdapterAliasU32 *)&dobj->rotate.a) ||
        (ndsRendererAdapterWords3Equal(&source_key->base_rotate[1],
                                       &dobj->rotate.vec.f) == FALSE) ||
        (ndsRendererAdapterWords3Equal(source_key->base_scale,
                                       &dobj->scale.vec.f) == FALSE))
    {
        return FALSE;
    }
    for (i = 0u; i < dobj->xobjs_num; i++)
    {
        XObj *xobj = dobj->xobjs[i];
        u32 expected_present =
            (source_key->xobj_present_mask & (u8)(1u << i));

        if (xobj == NULL)
        {
            if (expected_present != 0u)
            {
                return FALSE;
            }
            continue;
        }
        if ((expected_present == 0u) ||
            (source_key->xobj_kinds[i] != xobj->kind) ||
            (xobj->kind == 1u) ||
            ((xobj->kind >= 33u) && (xobj->kind <= 40u)) ||
            (xobj->kind == NDS_RENDERER_ADAPTER_FIGHTER_PARTS_MTX_KIND))
        {
            return FALSE;
        }
    }
    return TRUE;
}

static u32 ndsRendererAdapterNextStageWorldGeneration(void)
{
    u32 generation = sNdsRendererAdapterStageWorldNextGeneration++;

    if (generation == 0u)
    {
        generation = sNdsRendererAdapterStageWorldNextGeneration++;
    }
    return generation;
}

/* Local-matrix memo for the uncached world build (P2-2p8, 2026-09-28). On
 * Jungle, Zebes, Yoshi's Island and Saffron the heap refuses the DObj and
 * stage world caches (95-113K free when they are sized, against a ~137K
 * need), so every item, weapon and effect rebuilt its whole DObj chain from
 * float TRS every draw: a lying Beam Sword's two DObjs cost ~18K ticks each,
 * ~26K a frame of world builds on Saffron. A node's local matrix is a pure
 * function of what the stage world source key captures (the capture refuses
 * every kind that reads camera, fighter-part or other live state), so a hit
 * on that key returns the matrix the builder produced for it -- the same
 * reuse the persistent stage world cache already makes on the other stages.
 * 16 direct-mapped rows, 1,920 B. */
#define NDS_RENDERER_ADAPTER_LOCAL_MEMO_COUNT 16u
typedef struct NDSRendererAdapterLocalMemo
{
    const DObj *dobj;
    NDSRendererAdapterStageWorldSourceKey key;
    NDSRendererMatrix20p12 local;
    u32 valid;
} NDSRendererAdapterLocalMemo;
static NDSRendererAdapterLocalMemo
    sNdsRendererAdapterLocalMemo[NDS_RENDERER_ADAPTER_LOCAL_MEMO_COUNT];
volatile u32 gNdsRendererAdapterLocalMemoHits;
volatile u32 gNdsRendererAdapterLocalMemoFills;

static sb32 ndsRendererAdapterUncachedLocalMatrix(
    DObj *node, NDSRendererMatrix20p12 *local)
{
    NDSRendererAdapterLocalMemo *m = &sNdsRendererAdapterLocalMemo[
        ((u32)(uintptr_t)node >> 4) & (NDS_RENDERER_ADAPTER_LOCAL_MEMO_COUNT - 1u)];
    NDSRendererAdapterStageWorldSourceKey key;
    sb32 key_valid;

    if ((m->valid != 0u) && (m->dobj == node) &&
        (ndsRendererAdapterStageWorldSourceKeyMatches(node, &m->key) != FALSE))
    {
        ndsRendererMatrixCopy20p12(local, &m->local);
        NDS_DIAG(gNdsRendererAdapterLocalMemoHits++);
        return TRUE;
    }
    key_valid = ndsRendererAdapterCaptureStageWorldSourceKey(node, &key);
    if (ndsRendererAdapterBuildDObjLocalMatrix(node, local) == FALSE)
    {
        m->valid = 0u;
        return FALSE;
    }
    if (key_valid != FALSE)
    {
        m->dobj = node;
        m->key = key;
        ndsRendererMatrixCopy20p12(&m->local, local);
        m->valid = 1u;
        NDS_DIAG(gNdsRendererAdapterLocalMemoFills++);
    }
    else if (m->dobj == node)
    {
        m->valid = 0u;
    }
    return TRUE;
}

static sb32 ndsRendererAdapterBuildDObjWorldMatrixUncached(
    DObj *dobj, NDSRendererMatrix20p12 *out)
{
    DObj *chain[NDS_RENDERER_ADAPTER_DOBJ_PARENT_MAX];
    DObj *cursor = dobj;
    NDSRendererMatrix20p12 local;
    u32 depth = 0u;
    u32 i;

    if ((dobj == NULL) || (out == NULL))
    {
        return FALSE;
    }
    while ((cursor != NULL) && (cursor != DOBJ_PARENT_NULL) &&
           (depth < NDS_RENDERER_ADAPTER_DOBJ_PARENT_MAX))
    {
        chain[depth++] = cursor;
        cursor = cursor->parent;
    }
    if ((cursor != NULL) && (cursor != DOBJ_PARENT_NULL))
    {
        return FALSE;
    }
    ndsRendererAdapterMtxIdentity20p12(out);
    for (i = depth; i != 0u; i--)
    {
        if (ndsRendererAdapterUncachedLocalMatrix(chain[i - 1u], &local) !=
            FALSE)
        {
            ndsRendererMtxMulAffine20p12(&local, out, out);
        }
    }
    return TRUE;
}


static sb32 ndsRendererAdapterBuildDObjWorldMatrix(
    DObj *dobj, NDSRendererMatrix20p12 *out)
{
    DObj *chain[NDS_RENDERER_ADAPTER_DOBJ_PARENT_MAX];
    DObj *cursor = dobj;
    NDSRendererMatrix20p12 local;
    u32 depth = 0u;
    u32 i;
    const NDSRendererMatrix20p12 *cached;

    if ((dobj == NULL) || (out == NULL))
    {
        return FALSE;
    }
    cached = ndsRendererAdapterFindDObjWorldMatrix(dobj);
    if (cached != NULL)
    {
        ndsRendererMatrixCopy20p12(out, cached);
        return TRUE;
    }

    while ((cursor != NULL) && (cursor != DOBJ_PARENT_NULL) &&
           (depth < NDS_RENDERER_ADAPTER_DOBJ_PARENT_MAX))
    {
        chain[depth++] = cursor;
        cursor = cursor->parent;
    }
    if ((cursor != NULL) && (cursor != DOBJ_PARENT_NULL))
    {
        return FALSE;
    }

    for (i = 1u; i < depth; i++)
    {
        cached = ndsRendererAdapterFindDObjWorldMatrix(chain[i]);
        if (cached != NULL)
        {
            MTXCOPY(out, cached);
            break;
        }
    }
    if (i == depth)
    {
        ndsRendererAdapterMtxIdentity20p12(out);
    }
    for (; i != 0u; i--)
    {
        if (ndsRendererAdapterBuildDObjLocalMatrix(chain[i - 1u], &local) !=
            FALSE)
        {
            /* objdisplay.c:1183-1191 left-multiplies each child local matrix. */
            ndsRendererMtxMulAffine20p12(&local, out, out);
            /* DObj transforms are finalized before the camera's gcDrawAll
             * pass. Cache each prefix for sibling/child draws, and reset at
             * the next presented frame so live fighter poses remain live. */
            ndsRendererAdapterStoreDObjWorldMatrix(chain[i - 1u], out);
        }
    }
    return TRUE;
}


/* P2-2p8 (2026-10-04): A KEYED NODE'S LOCAL, KEPT ACROSS A PARENT CHANGE.
 * The world cache below reuses a node's world only while its parent's
 * generation holds, so a node under a moving parent -- a held item's blade
 * and hilt under the attach joint, which rebuilds every frame -- rebuilt its
 * local from its unchanged TRS every frame (~2-3K ticks a list on the gate's
 * held Beam Sword). The local is a pure function of what the source key
 * holds, which is the reuse path's own premise, so a node whose key matches
 * the one its kept local was built from takes that local and only
 * recomposes. Four slots: the keyed lists under moving parents are few. */
#define NDS_RENDERER_ADAPTER_STAGE_LOCAL_MEMO 4u
typedef struct NDSRendererAdapterStageLocalMemo
{
    const DObj *dobj;
    NDSRendererAdapterStageWorldSourceKey key;
    NDSRendererMatrix20p12 local;
} NDSRendererAdapterStageLocalMemo;
static NDSRendererAdapterStageLocalMemo
    sNdsRendererAdapterStageLocalMemo[NDS_RENDERER_ADAPTER_STAGE_LOCAL_MEMO];
static u32 sNdsRendererAdapterStageLocalMemoNext;
__attribute__((used)) volatile u32 gNdsStageLocalMemoHits;

/* The node's local, from the memo when its key matches, else built (and
 * kept when keyed). FALSE when the build fails. */
static sb32 ndsRendererAdapterStageLocalFor(
    DObj *node, const NDSRendererAdapterStageWorldSourceKey *key,
    u32 key_valid, NDSRendererMatrix20p12 *local)
{
    NDSRendererAdapterStageLocalMemo *slot = NULL;
    u32 i;

    if (key_valid != FALSE)
    {
        for (i = 0u; i < NDS_RENDERER_ADAPTER_STAGE_LOCAL_MEMO; i++)
        {
            if (sNdsRendererAdapterStageLocalMemo[i].dobj == node)
            {
                slot = &sNdsRendererAdapterStageLocalMemo[i];
                break;
            }
        }
        if ((slot != NULL) &&
            (memcmp(&slot->key, key, sizeof(*key)) == 0))
        {
            ndsRendererMatrixCopy20p12(local, &slot->local);
            NDS_DIAG(gNdsStageLocalMemoHits++);
            return TRUE;
        }
    }
    if (ndsRendererAdapterBuildDObjLocalMatrix(node, local) == FALSE)
    {
        return FALSE;
    }
    if (key_valid != FALSE)
    {
        if (slot == NULL)
        {
            slot = &sNdsRendererAdapterStageLocalMemo[
                sNdsRendererAdapterStageLocalMemoNext];
            sNdsRendererAdapterStageLocalMemoNext =
                (sNdsRendererAdapterStageLocalMemoNext + 1u) &
                (NDS_RENDERER_ADAPTER_STAGE_LOCAL_MEMO - 1u);
        }
        slot->dobj = node;
        slot->key = *key;
        ndsRendererMatrixCopy20p12(&slot->local, local);
    }
    return TRUE;
}

/* allow_stale: slice 44. TRUE means "this DObj's chain is not in this frame's
 * stride class, so a world matrix built on an earlier frame is accepted as-is".
 * Only the stage's own dynamic bindings may pass TRUE. The general caller at
 * the effect/fighter seam must not: kinds 0x4F and 0x50 are on the source-key
 * reject list precisely because their locals read live FTParts state, and a
 * matrix held over even one frame there is the frozen-attachment bug cycle 52
 * already paid for. */
/* P2-2p8 (2026-10-04): the world by pointer -- the entry's own matrix when
 * the persistent cache holds it, `scratch` when the uncached builder fills it,
 * NULL on failure -- so a caller that only reads it (the stage bindings'
 * compose) skips the 64-byte copy. */
static const NDSRendererMatrix20p12 *ndsRendererAdapterPersistentStageWorldPtr(
    DObj *dobj, NDSRendererMatrix20p12 *scratch, sb32 allow_stale)
{
    DObj *chain[NDS_RENDERER_ADAPTER_DOBJ_PARENT_MAX];
    DObj *cursor = dobj;
    /* The chain composes from the cached parent entries in place: pointing at
     * an ancestor's stored world instead of copying it into a local for every
     * link (the copies were a third of this function's profile). */
    NDSRendererMatrix20p12 identity;
    const NDSRendererMatrix20p12 *parent_world = &identity;
    NDSRendererMatrix20p12 local;
    NDSRendererAdapterStageWorldCacheEntry *entry;
    NDSRendererAdapterStageWorldSourceKey source_key;
    u32 parent_generation = 0u;
    u32 depth = 0u;
    u32 frame = gNdsRendererProfileFrameCount;
    u32 i;

    if ((dobj == NULL) || (scratch == NULL))
    {
        return NULL;
    }
    entry = ndsRendererAdapterFindStageWorldEntry(dobj);
    if ((entry != NULL) &&
        ((entry->validated_frame == frame)
         /* generation, not validated_frame, is the "has a world matrix at all"
          * test: ndsRendererAdapterNextStageWorldGeneration never returns 0,
          * and frame 0 is a real frame. */
         || ((allow_stale != FALSE) && (entry->generation != 0u))
         ))
    {
        const NDSRendererMatrix20p12 *hit =
            ndsRendererAdapterStageWorldEntryMatrix(entry);

        if (entry->validated_frame != frame)
        {
            NDS_DIAG(gNdsR2Slice44StaleReuse++);
        }
        return hit;
    }
    while ((cursor != NULL) && (cursor != DOBJ_PARENT_NULL) &&
           (depth < NDS_RENDERER_ADAPTER_DOBJ_PARENT_MAX))
    {
        chain[depth++] = cursor;
        cursor = cursor->parent;
    }
    if ((cursor != NULL) && (cursor != DOBJ_PARENT_NULL))
    {
        return NULL;
    }
    if (ndsRendererAdapterEnsureStageWorldCache() == FALSE)
    {
#if defined(NDS_LAB_FOURCPU_SWEEP) && NDS_LAB_FOURCPU_SWEEP
        gNdsLabPimAcc[4] += 1u;
#endif
        return (ndsRendererAdapterBuildDObjWorldMatrixUncached(
                    dobj, scratch) != FALSE) ? scratch : NULL;
    }

    ndsRendererAdapterMtxIdentity20p12(&identity);
    for (i = depth; i != 0u; i--)
    {
        DObj *node = chain[i - 1u];
        u32 source_key_valid;
        u32 reuse = FALSE;

        entry = ndsRendererAdapterFindOrAllocateStageWorldEntry(node);
        if (entry == NULL)
        {
#if defined(NDS_LAB_FOURCPU_SWEEP) && NDS_LAB_FOURCPU_SWEEP
            gNdsLabPimAcc[4] += 0x10000u;
#endif
            return (ndsRendererAdapterBuildDObjWorldMatrixUncached(
                        dobj, scratch) != FALSE) ? scratch : NULL;
        }
        if (entry->validated_frame == frame)
        {
            parent_world = ndsRendererAdapterStageWorldEntryMatrix(entry);
            parent_generation = entry->generation;
            continue;
        }

        source_key_valid = FALSE;
        if ((entry->source_key_valid != FALSE) &&
            (entry->parent == node->parent) &&
            (entry->parent_generation == parent_generation) &&
            (ndsRendererAdapterStageWorldSourceKeyMatches(
                 node, &entry->source_key) != FALSE))
        {
            reuse = TRUE;
        }
        if (reuse == FALSE)
        {
            source_key_valid = ndsRendererAdapterCaptureStageWorldSourceKey(
                node, &source_key);
            if (ndsRendererAdapterStageLocalFor(node, &source_key,
                                                source_key_valid,
                                                &local) == FALSE)
            {
                return NULL;
            }
            ndsRendererMtxMulAffine20p12(
                &local, parent_world,
                ndsRendererAdapterStageWorldEntryMatrix(entry));
            entry->parent = node->parent;
            entry->parent_generation = parent_generation;
            entry->generation = ndsRendererAdapterNextStageWorldGeneration();
            sNdsRendererAdapterStageWorldRebuilds++;
            entry->source_key_valid = source_key_valid;
            if (source_key_valid != FALSE)
            {
                entry->source_key = source_key;
            }
            else
            {
            }
        }
        entry->validated_frame = frame;
        parent_world = ndsRendererAdapterStageWorldEntryMatrix(entry);
        parent_generation = entry->generation;
    }
    if (parent_world == &identity)
    {
        /* Never hand out the local. */
        ndsRendererMatrixCopy20p12(scratch, &identity);
        return scratch;
    }
    return parent_world;
}

static sb32 ndsRendererAdapterBuildPersistentStageWorldMatrix(
    DObj *dobj, NDSRendererMatrix20p12 *out, sb32 allow_stale)
{
    const NDSRendererMatrix20p12 *world =
        ndsRendererAdapterPersistentStageWorldPtr(dobj, out, allow_stale);

    if (world == NULL)
    {
        return FALSE;
    }
    if (world != out)
    {
        ndsRendererMatrixCopy20p12(out, world);
    }
    return TRUE;
}

static sb32 ndsRendererAdapterBuildCameraMatrices(
    CObj *cobj,
    NDSRendererMatrix20p12 *projection,
    u32 *projection_valid,
    NDSRendererMatrix20p12 *modelview,
    u32 *modelview_valid,
    s32 *billboard_right_q12,
    s32 *billboard_up_q12,
    u32 *billboard_basis_valid,
    NDSRendererAdapterParticleCamera *particle)
{
    NDSRendererMatrix20p12 incoming;
    XObj *xobj;
    Mtx mtx;
    u32 i;

    if ((projection == NULL) || (projection_valid == NULL) ||
        (modelview == NULL) || (modelview_valid == NULL))
    {
        return FALSE;
    }

    *projection_valid = FALSE;
    *modelview_valid = FALSE;
    if (particle != NULL) { particle->valid = FALSE; }
    if (billboard_basis_valid != NULL)
    {
        *billboard_basis_valid = FALSE;
    }

    if (cobj == NULL)
    {
        return FALSE;
    }
    if (cobj->xobjs_num <= 0)
    {
        return FALSE;
    }

    for (i = 0u; i < (u32)cobj->xobjs_num; i++)
    {
        xobj = cobj->xobjs[i];
        if (xobj == NULL)
        {
            continue;
        }

        switch (xobj->kind)
        {
        case NDS_RENDERER_ADAPTER_GM_CAMERA_MTX_KIND:
        {
            LookAt look_at;
            NDSRendererMatrix20p12 lookat;
            NDSRendererMatrix20p12 persp;

            ndsRendererAdapterCameraLookAtReflect(
                &lookat, &look_at,
                cobj->vec.eye.x, cobj->vec.eye.y,
                cobj->vec.eye.z, cobj->vec.at.x,
                cobj->vec.at.y, cobj->vec.at.z,
                cobj->vec.up.x, cobj->vec.up.y,
                cobj->vec.up.z);
            if ((billboard_right_q12 != NULL) &&
                (billboard_up_q12 != NULL) &&
                (billboard_basis_valid != NULL))
            {
                billboard_right_q12[0] = lookat.m[0][0];
                billboard_right_q12[1] = lookat.m[1][0];
                billboard_right_q12[2] = lookat.m[2][0];
                billboard_up_q12[0] = lookat.m[0][1];
                billboard_up_q12[1] = lookat.m[1][1];
                billboard_up_q12[2] = lookat.m[2][1];
                *billboard_basis_valid = TRUE;
            }
            ndsRendererAdapterCameraPerspFast(
                &persp, &cobj->projection.persp.norm,
                cobj->projection.persp.fovy,
                cobj->projection.persp.aspect,
                cobj->projection.persp.near,
                cobj->projection.persp.far,
                cobj->projection.persp.scale);
            if ((particle != NULL) && (cobj->xobjs_num == 1))
            {
                MTXCOPY(&particle->projection, &persp);
                MTXCOPY(&particle->modelview, &lookat);
                particle->valid = TRUE;
            }
            ndsRendererMtxMul20p12(&lookat, &persp, projection);
            *projection_valid = TRUE;
            break;
        }
        case nGCMatrixKindPerspFastF:
            ndsRendererAdapterCameraPerspFast(
                projection, &cobj->projection.persp.norm,
                cobj->projection.persp.fovy,
                cobj->projection.persp.aspect,
                cobj->projection.persp.near,
                cobj->projection.persp.far,
                cobj->projection.persp.scale);
            *projection_valid = TRUE;
            break;
        case nGCMatrixKindPerspF:
            syMatrixPersp(&mtx, &cobj->projection.persp.norm,
                          cobj->projection.persp.fovy,
                          cobj->projection.persp.aspect,
                          cobj->projection.persp.near,
                          cobj->projection.persp.far,
                          cobj->projection.persp.scale);
            ndsRendererAdapterMtxFromN64(&mtx, projection);
            *projection_valid = TRUE;
            break;
        case nGCMatrixKindOrtho:
            syMatrixOrtho(&mtx,
                          cobj->projection.ortho.l,
                          cobj->projection.ortho.r,
                          cobj->projection.ortho.b,
                          cobj->projection.ortho.t,
                          cobj->projection.ortho.n,
                          cobj->projection.ortho.f,
                          cobj->projection.ortho.scale);
            ndsRendererAdapterMtxFromN64(&mtx, projection);
            *projection_valid = TRUE;
            break;
        case 6:
        case 7:
            syMatrixLookAt(&mtx,
                           cobj->vec.eye.x, cobj->vec.eye.y,
                           cobj->vec.eye.z, cobj->vec.at.x,
                           cobj->vec.at.y, cobj->vec.at.z,
                           cobj->vec.up.x, cobj->vec.up.y,
                           cobj->vec.up.z);
            ndsRendererAdapterMtxFromN64(&mtx, &incoming);
            if (xobj->kind == 6)
            {
                ndsRendererAdapterMulBefore(projection, &incoming,
                                            projection_valid);
            }
            else
            {
                *modelview = incoming;
                *modelview_valid = TRUE;
            }
            break;
        case 8:
        case 9:
            syMatrixModLookAt(&mtx,
                              cobj->vec.eye.x, cobj->vec.eye.y,
                              cobj->vec.eye.z, cobj->vec.at.x,
                              cobj->vec.at.y, cobj->vec.at.z,
                              cobj->vec.up.x, 0.0F, 1.0F, 0.0F);
            ndsRendererAdapterMtxFromN64(&mtx, &incoming);
            if (xobj->kind == 8)
            {
                ndsRendererAdapterMulBefore(projection, &incoming,
                                            projection_valid);
            }
            else
            {
                *modelview = incoming;
                *modelview_valid = TRUE;
            }
            break;
        case 10:
        case 11:
            syMatrixModLookAt(&mtx,
                              cobj->vec.eye.x, cobj->vec.eye.y,
                              cobj->vec.eye.z, cobj->vec.at.x,
                              cobj->vec.at.y, cobj->vec.at.z,
                              cobj->vec.up.x, 0.0F, 0.0F, 1.0F);
            ndsRendererAdapterMtxFromN64(&mtx, &incoming);
            if (xobj->kind == 10)
            {
                ndsRendererAdapterMulBefore(projection, &incoming,
                                            projection_valid);
            }
            else
            {
                *modelview = incoming;
                *modelview_valid = TRUE;
            }
            break;
        case 12:
        case 13:
        case 14:
        case 15:
        case 16:
        case 17:
        {
            /* objdisplay.c:2950-2983: the Reflect forms of 6-11 -- the same
             * view, plus the LookAt texgen reads
             * (ndsRendererAdapterCurrentLookAt). The 1P intro's stage cameras
             * are kind 15; without this arm their fighters had a projection
             * and no view and drew off screen. */
            LookAt look_at;

            ndsRendererAdapterReflectCameraView(cobj, (u32)xobj->kind, &mtx,
                                                &look_at);
            ndsRendererAdapterMtxFromN64(&mtx, &incoming);
            if ((xobj->kind & 1u) == 0u)
            {
                ndsRendererAdapterMulBefore(projection, &incoming,
                                            projection_valid);
            }
            else
            {
                *modelview = incoming;
                *modelview_valid = TRUE;
            }
            break;
        }
        default:
            break;
        }
    }

    return ((*projection_valid != 0u) || (*modelview_valid != 0u)) ?
        TRUE : FALSE;
}

static void ndsRendererAdapterBuildDefaultBattleCameraMatrices(
    NDSRendererMatrix20p12 *projection,
    u32 *projection_valid,
    NDSRendererMatrix20p12 *modelview,
    u32 *modelview_valid)
{
    u16 norm = 0u;
    Mtx mtx;

    if ((projection == NULL) || (projection_valid == NULL) ||
        (modelview == NULL) || (modelview_valid == NULL))
    {
        return;
    }

    ndsRendererAdapterCameraPerspFast(projection, &norm, 38.0F, 15.0F / 11.0F,
                                      256.0F, 39936.0F, 1.0F);
    *projection_valid = TRUE;

    syMatrixLookAt(&mtx,
                   0.0F, 300.0F, 1600.0F,
                   0.0F, 300.0F, 0.0F,
                   0.0F, 1.0F, 0.0F);
    ndsRendererAdapterMtxFromN64(&mtx, modelview);
    *modelview_valid = TRUE;
}

/* P2-2p8 (2026-10-04): the stage prepare's split camera from this frame's
 * camera cache. The 0x4C producer in ndsRendererAdapterBuildCameraMatrices
 * keeps look-at and perspective apart for the particle pass, and they are the
 * same two calls on the same CObj that
 * ndsRendererAdapterBuildTask36StageCameraMatrices makes -- so the stage
 * built the battle camera twice a frame (gate STG P50 -1.3K without the
 * second build). Valid only for the frame that filled the entry. */

static sb32 ndsRendererAdapterStageCameraFromFrameCache(
    const CObj *cobj,
    NDSRendererMatrix20p12 *projection,
    NDSRendererMatrix20p12 *modelview)
{
    u32 i;

    if ((cobj == NULL) ||
        (sNdsRendererAdapterCameraCacheFrame != gNdsRendererProfileFrameCount))
    {
        return FALSE;
    }
    for (i = 0u; i < sNdsRendererAdapterCameraCacheCount; i++)
    {
        const NDSRendererAdapterCameraCacheEntry *entry =
            &sNdsRendererAdapterCameraCache[i];

        if ((entry->cobj == cobj) && (entry->particle.valid != FALSE))
        {
            MTXCOPY(projection, &entry->particle.projection);
            MTXCOPY(modelview, &entry->particle.modelview);
            return TRUE;
        }
    }
    return FALSE;
}

static sb32 ndsRendererAdapterBuildTask36StageCameraMatrices(
    CObj *cobj,
    NDSRendererMatrix20p12 *projection,
    NDSRendererMatrix20p12 *modelview)
{
    LookAt look_at;
    u32 i;

    if ((cobj == NULL) || (projection == NULL) || (modelview == NULL))
    {
        return FALSE;
    }
    for (i = 0u; i < (u32)cobj->xobjs_num; i++)
    {
        if ((cobj->xobjs[i] == NULL) ||
            (cobj->xobjs[i]->kind != NDS_RENDERER_ADAPTER_GM_CAMERA_MTX_KIND))
        {
            continue;
        }
        ndsRendererAdapterCameraLookAtReflect(
            modelview, &look_at,
            cobj->vec.eye.x, cobj->vec.eye.y,
            cobj->vec.eye.z, cobj->vec.at.x,
            cobj->vec.at.y, cobj->vec.at.z,
            cobj->vec.up.x, cobj->vec.up.y,
            cobj->vec.up.z);
        ndsRendererAdapterCameraPerspFast(
            projection, &cobj->projection.persp.norm,
            cobj->projection.persp.fovy,
            cobj->projection.persp.aspect,
            cobj->projection.persp.near,
            cobj->projection.persp.far,
            cobj->projection.persp.scale);
        return TRUE;
    }
    return FALSE;
}
