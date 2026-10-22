/* Renderer adapter, the native owners' matrices: the frame's camera, root
 * composition, world origins, source rounding, animation-lock invariants
 * and the owners' matrices and hierarchies. A slice of
 * reloc_backend_renderer_dl.c. */

static void ndsRendererAdapterGetFrameCameraMatrices(
    CObj *cobj,
    NDSRendererMatrix20p12 *projection,
    u32 *projection_valid,
    NDSRendererMatrix20p12 *modelview,
    u32 *modelview_valid,
    s32 *billboard_right_q12,
    s32 *billboard_up_q12,
    u32 *billboard_basis_valid)
{
    u32 frame = gNdsRendererProfileFrameCount;
    u32 i;
    s32 built_billboard_right_q12[3] = { 0, 0, 0 };
    s32 built_billboard_up_q12[3] = { 0, 0, 0 };
    u32 built_billboard_basis_valid = FALSE;
    NDSRendererAdapterParticleCamera *particle = NULL;

    if ((projection == NULL) || (projection_valid == NULL) ||
        (modelview == NULL) || (modelview_valid == NULL))
    {
        return;
    }
    if (billboard_basis_valid != NULL)
    {
        *billboard_basis_valid = FALSE;
    }
    if (sNdsRendererAdapterCameraCacheFrame != frame)
    {
        sNdsRendererAdapterCameraCacheFrame = frame;
        sNdsRendererAdapterCameraCacheCount = 0u;
    }
    for (i = 0u; i < sNdsRendererAdapterCameraCacheCount; i++)
    {
        const NDSRendererAdapterCameraCacheEntry *entry =
            &sNdsRendererAdapterCameraCache[i];

        if (entry->cobj == cobj)
        {
            MTXCOPY(projection, &entry->projection);
            MTXCOPY(modelview, &entry->modelview);
            *projection_valid = entry->projection_valid;
            *modelview_valid = entry->modelview_valid;
            if ((billboard_right_q12 != NULL) &&
                (billboard_up_q12 != NULL) &&
                (billboard_basis_valid != NULL) &&
                (entry->billboard_basis_valid != FALSE))
            {
                memcpy(billboard_right_q12, entry->billboard_right_q12,
                       sizeof(entry->billboard_right_q12));
                memcpy(billboard_up_q12, entry->billboard_up_q12,
                       sizeof(entry->billboard_up_q12));
                *billboard_basis_valid = TRUE;
            }
            return;
        }
    }

    if (sNdsRendererAdapterCameraCacheCount <
        NDS_RENDERER_ADAPTER_CAMERA_CACHE_COUNT)
    {
        particle = &sNdsRendererAdapterCameraCache[
            sNdsRendererAdapterCameraCacheCount].particle;
    }
    ndsRendererAdapterBuildCameraMatrices(
        cobj, projection, projection_valid, modelview, modelview_valid,
        built_billboard_right_q12, built_billboard_up_q12,
        &built_billboard_basis_valid, particle);
    if ((*projection_valid == FALSE) && (*modelview_valid == FALSE))
    {
        ndsRendererAdapterBuildDefaultBattleCameraMatrices(
            projection, projection_valid, modelview, modelview_valid);
    }
    if (sNdsRendererAdapterCameraCacheCount <
        NDS_RENDERER_ADAPTER_CAMERA_CACHE_COUNT)
    {
        NDSRendererAdapterCameraCacheEntry *entry =
            &sNdsRendererAdapterCameraCache[
                sNdsRendererAdapterCameraCacheCount++];

        /* BattleShip updates CObj state before gcDrawAll; every DObj in that
         * camera's draw pass consumes the same matrices. The frame token
         * prevents fighter poses or camera motion from crossing presents. */
        entry->cobj = cobj;
        MTXCOPY(&entry->projection, projection);
        MTXCOPY(&entry->modelview, modelview);
        entry->projection_valid = *projection_valid;
        entry->modelview_valid = *modelview_valid;
        memcpy(entry->billboard_right_q12, built_billboard_right_q12,
               sizeof(entry->billboard_right_q12));
        memcpy(entry->billboard_up_q12, built_billboard_up_q12,
               sizeof(entry->billboard_up_q12));
        entry->billboard_basis_valid = built_billboard_basis_valid;
    }
    else
    {
    }
    if ((billboard_right_q12 != NULL) &&
        (billboard_up_q12 != NULL) &&
        (billboard_basis_valid != NULL) &&
        (built_billboard_basis_valid != FALSE))
    {
        memcpy(billboard_right_q12, built_billboard_right_q12,
               sizeof(built_billboard_right_q12));
        memcpy(billboard_up_q12, built_billboard_up_q12,
               sizeof(built_billboard_up_q12));
        *billboard_basis_valid = TRUE;
    }
}

s32 ndsRendererAdapterBeginParticleViewPass(void *camera_gobj)
{
    GObj *gobj = camera_gobj;
    CObj *cobj = (gobj != NULL) ? CObjGetStruct(gobj) : NULL;
    u32 i;

    /* gcDrawAll has prepared this camera before its particle callbacks. Never
     * reuse the preceding frame, nor rebuild a different camera behind the
     * caller's back. Non-0x4C cameras keep their existing native world path. */
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
            return ndsRendererBeginParticleViewPass(
                &entry->particle.projection, &entry->particle.modelview);
        }
    }
    return FALSE;
}

s32 ndsRendererAdapterSetWorldQuadCamera(void *camera_gobj)
{
    GObj *gobj = camera_gobj;
    CObj *cobj = (gobj != NULL) ? CObjGetStruct(gobj) : NULL;
    NDSRendererMatrix20p12 projection;
    NDSRendererMatrix20p12 modelview;
    u32 projection_valid = FALSE;
    u32 modelview_valid = FALSE;

    if (cobj == NULL)
    {
        return FALSE;
    }
    ndsRendererAdapterGetFrameCameraMatrices(
        cobj, &projection, &projection_valid, &modelview, &modelview_valid,
        NULL, NULL, NULL);
    if ((projection_valid == FALSE) && (modelview_valid == FALSE))
    {
        return FALSE;
    }
    /* The battle camera's 0x4C XObj stores look-at * perspective in the
     * projection result. Other camera shapes can retain split projection and
     * modelview matrices. Identity-fill only the absent side, preserving both
     * contracts without composing another matrix on ARM9. */
    if (projection_valid == FALSE)
    {
        ndsRendererAdapterMtxIdentity20p12(&projection);
    }
    if (modelview_valid == FALSE)
    {
        ndsRendererAdapterMtxIdentity20p12(&modelview);
    }
    ndsRendererSetParticleCamera(&projection, &modelview);
    return TRUE;
}

s32 ndsRendererAdapterSetWorldQuadCameraBasisQ12(void *camera_gobj,
                                                  s32 *right_q12,
                                                  s32 *up_q12)
{
    GObj *gobj = camera_gobj;
    CObj *cobj = (gobj != NULL) ? CObjGetStruct(gobj) : NULL;
    NDSRendererMatrix20p12 projection;
    NDSRendererMatrix20p12 modelview;
    u32 projection_valid = FALSE;
    u32 modelview_valid = FALSE;
    u32 billboard_basis_valid = FALSE;

    if ((cobj == NULL) || (right_q12 == NULL) || (up_q12 == NULL))
    {
        return FALSE;
    }
    ndsRendererAdapterGetFrameCameraMatrices(
        cobj, &projection, &projection_valid, &modelview, &modelview_valid,
        right_q12, up_q12, &billboard_basis_valid);
    if (((projection_valid == FALSE) && (modelview_valid == FALSE)) ||
        (billboard_basis_valid == FALSE))
    {
        return FALSE;
    }
    if (projection_valid == FALSE)
    {
        ndsRendererAdapterMtxIdentity20p12(&projection);
    }
    if (modelview_valid == FALSE)
    {
        ndsRendererAdapterMtxIdentity20p12(&modelview);
    }
    ndsRendererSetParticleCamera(&projection, &modelview);
    return TRUE;
}

/* Defined with the other stage statics further down; declared here because
 * ndsRendererAdapterPrepareInitialMatrices publishes its verdict only for the
 * effect submit. A tentative file-scope declaration, so this and the definition
 * name the same object. */
static sb32 sNdsRendererAdapterEffectSubmitActive;

static void ndsRendererAdapterPrepareInitialMatrices(
    DObj *dobj,
    CObj *cobj,
    u32 persistent_stage_world,
    NDSRendererMatrix20p12 *projection,
    const NDSRendererMatrix20p12 **projection_ptr,
    NDSRendererMatrix20p12 *modelview,
    const NDSRendererMatrix20p12 **modelview_ptr)
{
    NDSRendererMatrix20p12 camera_projection;
    NDSRendererMatrix20p12 camera_modelview;
    NDSRendererMatrix20p12 dobj_world;
    u32 mvp_recalc_kind;
    u32 camera_projection_valid = FALSE;
    u32 camera_modelview_valid = FALSE;
    u32 dobj_world_valid = FALSE;
#if defined(NDS_LAB_FOURCPU_SWEEP) && NDS_LAB_FOURCPU_SWEEP
    u32 lab_mark = 0u;
#endif

    if ((projection_ptr == NULL) || (modelview_ptr == NULL))
    {
        return;
    }
    *projection_ptr = NULL;
    *modelview_ptr = NULL;
    /* gcDrawDObjTreeDLLinksForGObj resets gGCScaleX to 1.0 before every tree;
     * the joint-attach build below is what raises it, and the recalc at the end
     * of this function is what consumes it. Same order, same scope. */
    sNdsRendererAdapterMvpRecalcScaleX = 1.0F;
    mvp_recalc_kind = ndsRendererAdapterDirectMvpRecalcKind(dobj);

    if ((projection == NULL) || (modelview == NULL))
    {
        return;
    }

    NDS_LAB_PIM_MARK(lab_mark);
    ndsRendererAdapterGetFrameCameraMatrices(
        cobj, &camera_projection, &camera_projection_valid,
        &camera_modelview, &camera_modelview_valid,
        NULL, NULL, NULL);
    NDS_LAB_PIM_ADD(0, lab_mark);
    NDS_LAB_PIM_MARK(lab_mark);
    if (dobj != NULL)
    {
        if (persistent_stage_world != FALSE)
        {
            dobj_world_valid =
                ndsRendererAdapterBuildPersistentStageWorldMatrix(
                    dobj, &dobj_world, FALSE);
        }
        else
        {
            dobj_world_valid =
                ndsRendererAdapterBuildDObjWorldMatrix(dobj, &dobj_world);
        }
    }
    NDS_LAB_PIM_ADD(1, lab_mark);
    NDS_LAB_PIM_MARK(lab_mark);

    if (camera_projection_valid != FALSE)
    {
        MTXCOPY(projection, &camera_projection);
        *projection_ptr = projection;
    }

    if ((camera_modelview_valid != FALSE) && (dobj_world_valid != FALSE))
    {
        ndsRendererMtxMul20p12(&dobj_world, &camera_modelview, modelview);
        *modelview_ptr = modelview;
    }
    else if (camera_modelview_valid != FALSE)
    {
        MTXCOPY(modelview, &camera_modelview);
        *modelview_ptr = modelview;
    }
    else if (dobj_world_valid != FALSE)
    {
        MTXCOPY(modelview, &dobj_world);
        *modelview_ptr = modelview;
    }
    /* WHICH OF THE THREE VERDICTS DIED, for the effect submit only. Cycle 53
     * read config.initial_projection AND initial_modelview as NULL at the
     * executor (raw words, max_commands 8192 proving the config), while the
     * camera cache entry for the same CObj read projection_valid 1 -- and the
     * code above cannot produce both of those. One of the two reads is about a
     * different moment, and a latch taken here, between the decision and the
     * config fill, is the only thing that can say which. Effect-only because
     * the stage submits hundreds of lists per frame through this same path. */
    if (sNdsRendererAdapterEffectSubmitActive != FALSE)
    {
        NDS_DIAG(gNdsRendererAdapterEffectPrepCount++);
        gNdsRendererAdapterEffectPrepMask =
            ((camera_projection_valid != FALSE) ? 1u : 0u) |
            ((camera_modelview_valid != FALSE) ? 2u : 0u) |
            ((dobj_world_valid != FALSE) ? 4u : 0u) |
            ((*projection_ptr != NULL) ? 8u : 0u) |
            ((*modelview_ptr != NULL) ? 16u : 0u);
    }
    NDS_LAB_PIM_ADD(2, lab_mark);
    NDS_LAB_PIM_MARK(lab_mark);
    ndsRendererAdapterApplyMvpRecalc(
        (mvp_recalc_kind != 0u) ? dobj : NULL, mvp_recalc_kind, cobj,
        projection, projection_ptr, modelview, modelview_ptr,
        ((mvp_recalc_kind != 0u) && (cobj != NULL)) ?
            ndsRendererAdapterSharedMvpCamera(cobj) : NULL);
    NDS_LAB_PIM_ADD(3, lab_mark);
    if (sNdsRendererAdapterEffectSubmitActive != FALSE)
    {
        gNdsRendererAdapterEffectPrepMask |=
            ((*projection_ptr != NULL) ? 32u : 0u) |
            ((*modelview_ptr != NULL) ? 64u : 0u);
    }
}

/* Slice 6 (P2-2p8 Phase 1): always inline. Its callers are the stage binding
 * matrix prep (every stage binding, every frame), the Fox blaster builder
 * and the fighter draw; a fourth call site (the lean path's blaster sidecar)
 * tipped GCC into outlining it, and the stage prep's code changed with it. */
static inline __attribute__((always_inline)) sb32
ndsRendererAdapterComposeNativeRootMatrix(
    const NDSRendererMatrix20p12 *modelview,
    const NDSRendererMatrix20p12 *projection,
    NDSRendererMatrix20p12 *out)
{
    if (out == NULL)
    {
        return FALSE;
    }
    if ((modelview != NULL) && (projection != NULL))
    {
        ndsRendererMtxMul20p12(modelview, projection, out);
        return TRUE;
    }
    if (modelview != NULL)
    {
        ndsRendererMatrixCopy20p12(out, modelview);
        return TRUE;
    }
    if (projection != NULL)
    {
        ndsRendererMatrixCopy20p12(out, projection);
        return TRUE;
    }
    return FALSE;
}


/* BUGS.md "Fox's pistol model is missing": joint 17's world matrix, for the
 * blaster overlay.
 *
 * This is the 0x4F builder's arithmetic without its DObj indirection -- the gun
 * has no attachment DObj because it has no GObj, which is the whole reason it
 * costs nothing but a matrix. func_ovl2_800EDBA4 FIRST, for the same reason the
 * shield needs it: mtx_translate is a per-frame cache that
 * ndsFTParamsInvalidateFighterParts clears, and the fighter's own draw does not
 * fill it. The chain probe read it as exactly zero at the setter
 * (artifacts/bugs/2026-08-12_r2-07-cluster/modelpart-chain.log, `MPJW w=0,0,0`),
 * so copying it without the rebuild would park the gun at the world origin.
 *
 * It deliberately does NOT write sNdsRendererAdapterMvpRecalcScaleX. That
 * global is the shield's billboard size, consumed by a recalc later in the same
 * call; the gun is not a billboard and must not move it. */
static sb32 __attribute__((noinline, cold))
ndsRendererAdapterFoxGunGameplayOwner(const FTStruct *fp)
{
    return ((fp != NULL) &&
            ((fp->pkind == nFTPlayerKindMan) ||
             (fp->pkind == nFTPlayerKindCom) ||
             (fp->pkind == nFTPlayerKindGameKey))) ? TRUE : FALSE;
}

static sb32 ndsRendererAdapterBuildFoxGunJointMtx(
    FTStruct *fp, CObj *cobj, NDSRendererMatrix20p12 *out)
{
    DObj *joint;
    FTParts *parts;
    Mtx mtx;
    NDSRendererMatrix20p12 world;
    NDSRendererMatrix20p12 camera_projection;
    NDSRendererMatrix20p12 camera_modelview;
    u32 camera_projection_valid = FALSE;
    u32 camera_modelview_valid = FALSE;

    /* Kirby's Fox copy selects dFoxUnknown_DL on this same joint 17 through
     * dKirbyMain_modelparts_desc_0x3C4, so the source does put a pistol in his
     * hand and this overlay follows it for both fighters. The collection strip
     * in renderer_adapter_fighter.c keys off the same two kinds; the two must
     * agree or one path draws a pistol the other still submitted.
     *
     * The gun was briefly Fox-only (2026-09-21) after the owner asked for "the
     * pistol shot VFX" to go. That was a misreading: the complaint was the
     * muzzle FLASH landing on Kirby's body, not the weapon model, and the
     * model is source-correct. The flash is suppressed at its own seam in
     * battleship_fox_blaster.c; the pistol is back. */
    if ((fp == NULL) || (out == NULL) ||
        ((fp->fkind != nFTKindFox) &&
         !((fp->fkind == nFTKindKirby) &&
           (fp->passive_vars.kirby.copy_id == (s32)nFTKindFox))) ||
        ((u32)NDS_FOX_GUN_HOLD_JOINT >= ARRAY_COUNT(fp->joints)) ||
        (ndsRendererAdapterFoxGunGameplayOwner(fp) == FALSE))
    {
        return FALSE;
    }
    /* Results constructs Demo fighters and its win/lose statuses do not own the
     * battle-only pistol overlay. modelpart_status can inherit a non-negative
     * value there, which used to draw a solid/stray gun over the Results pose.
     * Gate on fighter ownership rather than scene kind so other real gameplay
     * scenes can still use Fox's source blaster presentation. */
    if (fp->modelpart_status[NDS_FOX_GUN_HOLD_JOINT -
                             nFTPartsJointCommonStart].modelpart_id_curr < 0)
    {
        return FALSE;
    }
    joint = fp->joints[NDS_FOX_GUN_HOLD_JOINT];
    if ((joint == NULL) || (joint == DOBJ_PARENT_NULL) ||
        (joint->parent_gobj == NULL))
    {
        return FALSE;
    }
    parts = ftGetParts(joint);
    if (parts == NULL)
    {
        return FALSE;
    }
    func_ovl2_800EDBA4(joint);
    if (ndsRendererAdapterF2LFixedWExact(&parts->mtx_translate, &mtx) == FALSE)
    {
        syMatrixF2LFixedW(&parts->mtx_translate, &mtx);
    }
    ndsRendererAdapterMtxFromN64(&mtx, &world);

    /* The same two multiplies a fighter root gets, in the same order. The
     * binding pass seeds its forward compose with the camera modelview and then
     * hands the product to ndsRendererAdapterComposeNativeRootMatrix, so
     * `world x camera x projection` IS a root's matrix; the gun is simply a
     * root the baked plan does not know about. Both terms come from the per-
     * frame cache the fighter's own prepare already filled this frame, so this
     * costs two 4x4s and no camera work. */
    ndsRendererAdapterGetFrameCameraMatrices(
        cobj, &camera_projection, &camera_projection_valid,
        &camera_modelview, &camera_modelview_valid,
        NULL, NULL, NULL);
    if (camera_modelview_valid != FALSE)
    {
        ndsRendererMtxMulAffine20p12(&world, &camera_modelview, &world);
    }
    if (camera_projection_valid == FALSE)
    {
        return FALSE;
    }
    return ndsRendererAdapterComposeNativeRootMatrix(
        &world, &camera_projection, out);
}

/* A P4 content's held gun (Wolf's), drawn as a generated entry-effect root:
 * the same joint-17 world matrix as above, handed over as the modelview
 * (world x camera) and the camera projection the root's submit lights and
 * projects with, rather than one composed matrix. */
static sb32 ndsRendererAdapterBuildFoxGunJointMatrices(
    FTStruct *fp, CObj *cobj, NDSRendererMatrix20p12 *projection,
    NDSRendererMatrix20p12 *modelview)
{
    DObj *joint;
    FTParts *parts;
    Mtx mtx;
    NDSRendererMatrix20p12 camera_modelview;
    u32 camera_projection_valid = FALSE;
    u32 camera_modelview_valid = FALSE;

    if ((fp == NULL) || (projection == NULL) || (modelview == NULL) ||
        (ndsRendererAdapterFoxGunGameplayOwner(fp) == FALSE) ||
        (fp->modelpart_status[NDS_FOX_GUN_HOLD_JOINT -
                              nFTPartsJointCommonStart].modelpart_id_curr < 0))
    {
        return FALSE;
    }
    joint = fp->joints[NDS_FOX_GUN_HOLD_JOINT];
    if ((joint == NULL) || (joint == DOBJ_PARENT_NULL) ||
        (joint->parent_gobj == NULL))
    {
        return FALSE;
    }
    parts = ftGetParts(joint);
    if (parts == NULL)
    {
        return FALSE;
    }
    func_ovl2_800EDBA4(joint);
    if (ndsRendererAdapterF2LFixedWExact(&parts->mtx_translate, &mtx) == FALSE)
    {
        syMatrixF2LFixedW(&parts->mtx_translate, &mtx);
    }
    ndsRendererAdapterMtxFromN64(&mtx, modelview);
    ndsRendererAdapterGetFrameCameraMatrices(
        cobj, projection, &camera_projection_valid,
        &camera_modelview, &camera_modelview_valid,
        NULL, NULL, NULL);
    if (camera_modelview_valid != FALSE)
    {
        ndsRendererMtxMulAffine20p12(modelview, &camera_modelview, modelview);
    }
    return (camera_projection_valid != FALSE) ? TRUE : FALSE;
}

/* A custom Kirby power's hat (scripts/p4/p4_kirby_custom_hats.py), a P4
 * sidecar root on joint 6: the matrices above at that joint, gated on a
 * gameplay Kirby whose joint 6 shows his plain head. */
static sb32 ndsRendererAdapterBuildKirbyHatJointMatrices(
    FTStruct *fp, u32 joint_id, CObj *cobj, NDSRendererMatrix20p12 *projection,
    NDSRendererMatrix20p12 *modelview)
{
    DObj *joint;
    FTParts *parts;
    Mtx mtx;
    NDSRendererMatrix20p12 camera_modelview;
    u32 camera_projection_valid = FALSE;
    u32 camera_modelview_valid = FALSE;

    if ((fp == NULL) || (projection == NULL) || (modelview == NULL) ||
        (ndsRendererAdapterFoxGunGameplayOwner(fp) == FALSE))
    {
        return FALSE;
    }
    joint = fp->joints[joint_id];
    if ((joint == NULL) || (joint == DOBJ_PARENT_NULL) ||
        (joint->parent_gobj == NULL))
    {
        return FALSE;
    }
    parts = ftGetParts(joint);
    if (parts == NULL)
    {
        return FALSE;
    }
    func_ovl2_800EDBA4(joint);
    if (ndsRendererAdapterF2LFixedWExact(&parts->mtx_translate, &mtx) == FALSE)
    {
        syMatrixF2LFixedW(&parts->mtx_translate, &mtx);
    }
    ndsRendererAdapterMtxFromN64(&mtx, modelview);
    ndsRendererAdapterGetFrameCameraMatrices(
        cobj, projection, &camera_projection_valid,
        &camera_modelview, &camera_modelview_valid,
        NULL, NULL, NULL);
    if (camera_modelview_valid != FALSE)
    {
        ndsRendererMtxMulAffine20p12(modelview, &camera_modelview, modelview);
    }
    return (camera_projection_valid != FALSE) ? TRUE : FALSE;
}


/* A world-space list's matrix: a translation to `origin` (world units),
 * times the frame camera, times the projection -- the fighter-root compose
 * above with a translation for the joint matrix. The list's vertices are
 * world positions minus `origin`, so they stay inside a v16. The afterimage
 * trail (ndsFighterDrawAfterImage) is its one user. */
static sb32 ndsRendererAdapterBuildWorldOriginMtx(
    CObj *cobj, const s16 origin[3], NDSRendererMatrix20p12 *out)
{
    NDSRendererMatrix20p12 world;
    NDSRendererMatrix20p12 camera_projection;
    NDSRendererMatrix20p12 camera_modelview;
    u32 camera_projection_valid = FALSE;
    u32 camera_modelview_valid = FALSE;
    u32 row;

    if ((origin == NULL) || (out == NULL))
    {
        return FALSE;
    }
    memset(&world, 0, sizeof(world));
    for (row = 0u; row < 4u; row++)
    {
        world.m[row][row] = 1 << 12;
    }
    world.m[3][0] = (s32)origin[0] * (1 << 12);
    world.m[3][1] = (s32)origin[1] * (1 << 12);
    world.m[3][2] = (s32)origin[2] * (1 << 12);
    ndsRendererAdapterGetFrameCameraMatrices(
        cobj, &camera_projection, &camera_projection_valid,
        &camera_modelview, &camera_modelview_valid, NULL, NULL, NULL);
    if (camera_modelview_valid != FALSE)
    {
        ndsRendererMtxMulAffine20p12(&world, &camera_modelview, &world);
    }
    if (camera_projection_valid == FALSE)
    {
        return FALSE;
    }
    return ndsRendererAdapterComposeNativeRootMatrix(
        &world, &camera_projection, out);
}


/* R2-03 E32. The hitlag shuffle used to switch the whole native fighter owner
 * off (reloc_backend_renderer_dl.c's AnimLock fallback), dropping the fighter
 * to the generic interpreter for every hit -- E31 measured that at 5 fallbacks
 * over frames 460..500, one per burst frame, and the bursts are 41.9% of the
 * P95 tail's excess.
 *
 * The source does not need any of that. ftdisplaymain.c:1205 is one
 * G_MTX_PUSH plus syMatrixTra(x, y, 0) around the entire fighter draw and one
 * gSPPopMatrix, and lbcommon.c:1627 expresses the identical effect for the
 * attached-DObj path as `f[3][0] += x; f[3][1] += y;` on the part's *world*
 * matrix, before the camera is applied. This is that second form: the offset
 * lands on the world matrix in exactly the same space, so it is mechanically
 * equivalent rather than an approximation.
 *
 * 20.12, because that is what NDSRendererMatrix20p12 holds. */
extern Vec2f dFTDisplayMainShufflePositions[][4];
static s32 sNdsR2ShuffleWorldX;
static s32 sNdsR2ShuffleWorldY;
u32 gNdsR2ShuffleFoldedFrames;

static void ndsRendererAdapterSetShuffleOffset(const FTStruct *fp)
{
    sNdsR2ShuffleWorldX = 0;
    sNdsR2ShuffleWorldY = 0;
    if ((fp == NULL) || (fp->shuffle_tics == 0u))
    {
        return;
    }
    {
        const Vec2f *offset =
            &dFTDisplayMainShufflePositions[fp->is_shuffle_electric]
                                           [fp->shuffle_frame_index];

        sNdsR2ShuffleWorldX = (s32)(offset->x * 4096.0F);
        sNdsR2ShuffleWorldY = (s32)(offset->y * 4096.0F);
        NDS_DIAG(gNdsR2ShuffleFoldedFrames++);
    }
}

/* ---------------------------------------------------------------------------
 * The flat baked world compose.
 *
 * The shipped path asks `ndsRendererAdapterBuildDObjWorldMatrix` for each of the
 * 14/18 selected bindings independently. Because that entry point knows nothing
 * about the order it is being called in, it pays for the ignorance every time: a
 * linear-probed hash lookup on the binding, a walk all the way to the root, one
 * hash probe per ancestor looking for a prefix somebody already built, and a
 * hash store per composed step. The census prices the machinery at
 * `BuildDObjWorldMatrix` self 13,947 + `FindDObjWorldMatrix` 4,385 = 18,332
 * ticks/frame -- against ~50 local builds a frame whose arithmetic this does not
 * touch. That ratio is the whole point: the cost is the traversal and the cache
 * around the arithmetic, not the arithmetic.
 *
 * The order is not unknown, it is baked. `BindingParents` gives each binding's
 * nearest BOUND ancestor and the generator derives it from a preorder joint
 * list, so `binding_parents[i] < i` always -- one forward pass composes every
 * world with no cache at all, each binding starting from its parent binding's
 * finished world and walking only the one to three joints between them.
 *
 * Two things this deliberately does NOT do. It does not use `BindingParents` as
 * if it were the DObj parent -- the table skips unbound joints, so the live
 * chain between two bindings still has to be walked and composed, and treating
 * the table as a direct parent would silently drop those transforms. And it does
 * not stop storing prefixes: the generic display-list path shares this hash for
 * effects parented under fighter joints, so every joint it composes is still
 * published. What is deleted is the probing, not the publishing.
 *
 * Fail-closed: any disagreement between the baked table and the live tree
 * returns FALSE and the caller falls back to the per-binding path.
 *
 * MEASURED and graduated, cycle 110. Priced on ONE binary through a `.data`
 * route (identical `romSha256` in both arms, poke read back at end of run):
 * `FTR` mean 374,332 -> 366,597, P50 387,072 -> 379,328, P95 390,400 ->
 * 382,464, `WORK-H` mean 1,052,509 -> 1,044,687. Every unrelated bucket flat
 * within +/-40 -- `STG` -20, `SRC` -25, `SINT` +6, `SCPU` -8 -- and `FTR` agrees
 * with `WORK-H` to 87 ticks, so the whole delta lands in the bucket that owns
 * the change. Engagement was 3,951 calls and 0 rejects, so the route and its two
 * counters are deleted rather than left behind as proof-only machinery. */
/* `seed` is what the chain starts from, and it is the camera, not the identity.
 * Left-multiplying the whole chain into camera space costs nothing -- the first
 * multiply of a root chain used to be against the identity, which is a full
 * 4x4 that produces its own input -- and it deletes the per-binding
 * `world * camera` that ran once for every one of the ~31 bindings a frame.
 * Reassociating a fixed-point product is not bit-exact; these matrices reach
 * GX and nothing else, so that is a render-side difference by the doctrine in
 * PROJECT_GOAL.md, and the Boundary visual gate is what checks it. */
/* Slice 43 engagement. `Captures` counts bindings described, `Locals` the
 * MTX_MULT the backend will issue, `Declines` a fall-back to the CPU compose.
 * A cut with no counter is how cycle 110 read FTR -13,587 off a skip it could
 * not prove fired. */
u32 gNdsR2GxComposeCaptures;
u32 gNdsR2GxComposeLocals;
u32 gNdsR2GxComposeDeclines;

/* One palette slot per binding that is some other binding's baked parent, so the
 * backend can RESTORE it instead of the adapter composing into it. A binding that
 * already owns a cross-run slot reuses it -- the root loop stores there anyway --
 * and the rest are allocated DOWNWARD from 30. Downward on purpose:
 * MTX_STORE/MTX_RESTORE address ABSOLUTE stack levels while glPushMatrix writes
 * whatever level the stack pointer is at, so the low levels belong to anyone who
 * pushes inside the execute (ndsRendererR2WriteLightVector does, once). Mario
 * needs one new slot and Fox eleven.
 *
 * THE CROSS-RUN RANGE IS 16..25, not the 16..23 this comment claimed until
 * 2026-08-25: Donkey's generated table reaches slot 25 (P2-3r17 read it off
 * `decode_joint_topology`, and the loop below takes the real union rather than
 * a constant, so the code was right while the comment was two owners stale).
 * That leaves exactly FIVE free levels above the reserved band -- 26..30 -- and
 * this loop then skips down past the band to 15 and below. Donkey alone needs
 * ten parent slots, so the next owner is already allocating underneath the
 * cross-run range; keep an eye on how far down that reaches against the levels
 * a push can occupy. */
static u8 sNdsR2GxSlotTable[
    NDS_RENDERER_NATIVE_FIGHTER_OWNER_COUNT]
    [NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED];
static u8 sNdsR2GxSlotTableValid[
    NDS_RENDERER_NATIVE_FIGHTER_OWNER_COUNT];
static u8 sNdsR2GxSlotTableProgram[
    NDS_RENDERER_NATIVE_FIGHTER_OWNER_COUNT];

static void ndsRendererAdapterResetGxSlotTables(void)
{
    memset(sNdsR2GxSlotTableValid, 0, sizeof(sNdsR2GxSlotTableValid));
    memset(sNdsR2GxSlotTableProgram, 0, sizeof(sNdsR2GxSlotTableProgram));
}

static sb32 ndsRendererAdapterBuildGxSlotTable(u32 slot, u32 binding_count)
{
    const u8 *parents;
    u32 parent_count = 0u;
    u32 cross_reserved_mask = 0u;
    u32 next_free = NDS_RENDERER_FIGHTER_GX_SLOT_NONE - 1u;
    u8 *table;
    u32 root_program;
    u32 owner_slot;
    u32 i;

    if ((slot >= NDS_RENDERER_NATIVE_FIGHTER_OWNER_COUNT) ||
        (binding_count > NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED))
    {
        return FALSE;
    }
    table = sNdsR2GxSlotTable[slot];
    root_program = ndsRendererNativeFighterRootProgram(slot);
    if ((sNdsR2GxSlotTableValid[slot] != 0u) &&
        ((u32)sNdsR2GxSlotTableProgram[slot] == root_program))
    {
        return TRUE;
    }
    sNdsR2GxSlotTableValid[slot] = 0u;
    parents = ndsRendererNativeFighterBindingParents(slot, &parent_count);
    if ((parents == NULL) || (parent_count != binding_count))
    {
        return FALSE;
    }
    /* There is one hardware position/vector palette shared by every native
     * fighter owner. Parent slots are allocated per owner, so reserve the UNION
     * of every generated cross-run slot table. The original two-owner code used
     * `slot ^ 1`; admitting Luigi made that both incomplete and an invalid owner
     * lookup for slot 2. Parent-vs-parent overlap remains safe because every
     * parent slot is stored before a child restores it within one owner execute. */
    for (owner_slot = 0u;
         owner_slot < NDS_RENDERER_NATIVE_FIGHTER_OWNER_COUNT;
         owner_slot++)
    {
        const u8 *cross;
        u32 cross_count = 0u;
        u32 cross_index;

        cross = ndsRendererNativeFighterCrossPaletteSlots(
            owner_slot, &cross_count);
        if (cross == NULL)
        {
            return FALSE;
        }
        if ((owner_slot == slot) && (cross_count != binding_count))
        {
            return FALSE;
        }
        for (cross_index = 0u; cross_index < cross_count; cross_index++)
        {
            u32 cross_slot = cross[cross_index];

            if (cross_slot < NDS_RENDERER_FIGHTER_GX_SLOT_NONE)
            {
                cross_reserved_mask |= 1u << cross_slot;
            }
        }
    }

    /* A parent slot may NOT reuse the binding's cross-run slot. The cross-run
     * emitter restores a foreign binding's slot and then submits model-space
     * vertices, so what it needs there is the world with the world-unit scale
     * already applied -- and what a CHILD needs is the same world without it.
     * Two different matrices, so two different slots; the root loop's existing
     * glStoreMatrix keeps writing the scaled one. */
    for (i = 0u; i < binding_count; i++)
    {
        table[i] = (u8)NDS_RENDERER_FIGHTER_GX_SLOT_NONE;
    }
    for (i = 0u; i < binding_count; i++)
    {
        u32 parent = parents[i];

        if (parent == 0xffu)
        {
            continue;
        }
        /* The generator's contract, and what makes one forward pass correct. */
        if (parent >= i)
        {
            return FALSE;
        }
        if (table[parent] != (u8)NDS_RENDERER_FIGHTER_GX_SLOT_NONE)
        {
            continue;
        }
        while ((cross_reserved_mask & (1u << next_free)) != 0u)
        {
            if (next_free == 0u)
            {
                return FALSE;
            }
            next_free--;
        }
        table[parent] = (u8)next_free;
        if (next_free == 0u)
        {
            return FALSE;
        }
        next_free--;
    }
    sNdsR2GxSlotTableProgram[slot] = (u8)root_program;
    sNdsR2GxSlotTableValid[slot] = 1u;
    return TRUE;
}

/* The same forward pass as ndsRendererAdapterComposeOwnerWorldsFlat, with the
 * multiplies removed. Each binding's chain is copied out in the order that pass
 * would have multiplied it -- nearest ancestor first, the binding itself last --
 * because DS MTX_MULT is `current = M x current`, which is exactly the
 * `MtxMulAffine20p12(&local, out, out)` convention. Nothing is reassociated.
 *
 * The locals land in `hierarchy_storage.hierarchy_locals`, which mode 9 owns
 * and mode 10 does not
 * use: every joint sits on exactly one binding's chain, so JOINT_MAX is the
 * exact bound. */
static sb32 ndsRendererAdapterCaptureOwnerChainsGx(
    u32 slot,
    DObj *const *bindings,
    u32 binding_count,
    NDSRendererAdapterNativeOwnerWorkspace *workspace)
{
    const u8 *binding_parents;
    const u8 *slots;
    u32 parent_count = 0u;
    u32 next_local = 0u;
    u32 binding_index;

    binding_parents = ndsRendererNativeFighterBindingParents(slot,
                                                             &parent_count);
    if ((binding_parents == NULL) || (parent_count != binding_count) ||
        (bindings == NULL) || (workspace == NULL) ||
        (ndsRendererAdapterBuildGxSlotTable(slot, binding_count) == FALSE))
    {
        return FALSE;
    }
    slots = sNdsR2GxSlotTable[slot];

    for (binding_index = 0u; binding_index < binding_count; binding_index++)
    {
        DObj *chain[NDS_RENDERER_ADAPTER_DOBJ_PARENT_MAX];
        DObj *cursor = bindings[binding_index];
        DObj *stop = DOBJ_PARENT_NULL;
        u32 parent = binding_parents[binding_index];
        u32 depth = 0u;
        u32 count = 0u;
        u32 i;

        if (cursor == NULL)
        {
            return FALSE;
        }
        if (parent != 0xffu)
        {
            if ((parent >= binding_index) || (bindings[parent] == NULL) ||
                (slots[parent] >= NDS_RENDERER_FIGHTER_GX_SLOT_NONE))
            {
                return FALSE;
            }
            stop = bindings[parent];
        }
        while ((cursor != NULL) && (cursor != DOBJ_PARENT_NULL) &&
               (cursor != stop))
        {
            if (depth >= NDS_RENDERER_ADAPTER_DOBJ_PARENT_MAX)
            {
                return FALSE;
            }
            chain[depth++] = cursor;
            cursor = cursor->parent;
        }
        if (parent != 0xffu)
        {
            if (cursor != stop)
            {
                return FALSE;
            }
        }
        else if ((cursor != NULL) && (cursor != DOBJ_PARENT_NULL))
        {
            return FALSE;
        }

        workspace->gx_local_first[binding_index] = (u8)next_local;
        for (i = depth; i != 0u; i--)
        {
            if (next_local >= NDS_RENDERER_FIGHTER_GX_LOCAL_MAX)
            {
                return FALSE;
            }
            /* A joint whose local build fails contributes nothing, exactly as
             * the compose pass treats it. */
            if (ndsRendererAdapterBuildDObjLocalMatrix(
                    chain[i - 1u],
                    &workspace->gx_locals[next_local]) != FALSE)
            {
                next_local++;
                count++;
            }
        }
        workspace->gx_local_count[binding_index] = (u8)count;
        workspace->gx_parent_slot[binding_index] = (parent == 0xffu) ?
            (u8)NDS_RENDERER_FIGHTER_GX_SLOT_NONE : slots[parent];
        workspace->gx_store_slot[binding_index] = slots[binding_index];
        NDS_DIAG(gNdsR2GxComposeLocals += count);
    }
    NDS_DIAG(gNdsR2GxComposeCaptures += binding_count);
    return TRUE;
}

static sb32 ndsRendererAdapterComposeOwnerWorldsFlat(
    u32 slot,
    DObj *const *bindings,
    u32 binding_count,
    NDSRendererMatrix20p12 *worlds,
    const NDSRendererMatrix20p12 *seed,
    sb32 seed_is_identity)
{
    const u8 *binding_parents;
    u32 parent_count = 0u;
    u32 binding_index;

    binding_parents = ndsRendererNativeFighterBindingParents(slot,
                                                             &parent_count);
    if ((binding_parents == NULL) || (parent_count != binding_count) ||
        (bindings == NULL) || (worlds == NULL) || (seed == NULL))
    {
        return FALSE;
    }
    for (binding_index = 0u; binding_index < binding_count; binding_index++)
    {
        DObj *chain[NDS_RENDERER_ADAPTER_DOBJ_PARENT_MAX];
        NDSRendererMatrix20p12 *out = &worlds[binding_index];
        const NDSRendererMatrix20p12 *base;
        DObj *cursor = bindings[binding_index];
        DObj *stop = DOBJ_PARENT_NULL;
        u32 parent = binding_parents[binding_index];
        u32 depth = 0u;
        sb32 base_identity;
        sb32 applied;
        u32 i;

        if (cursor == NULL)
        {
            return FALSE;
        }
        if (parent != 0xffu)
        {
            /* Strictly lower index, so its world is already composed. */
            if ((parent >= binding_index) || (bindings[parent] == NULL))
            {
                return FALSE;
            }
            stop = bindings[parent];
        }
        while ((cursor != NULL) && (cursor != DOBJ_PARENT_NULL) &&
               (cursor != stop))
        {
            if (depth >= NDS_RENDERER_ADAPTER_DOBJ_PARENT_MAX)
            {
                return FALSE;
            }
            chain[depth++] = cursor;
            cursor = cursor->parent;
        }
        if (parent != 0xffu)
        {
            if (cursor != stop)
            {
                return FALSE;
            }
            base = &worlds[parent];
            base_identity = FALSE;
        }
        else
        {
            /* The root walk ends on either spelling of "no parent", exactly as
             * ndsRendererAdapterBuildDObjWorldMatrix accepts both. */
            if ((cursor != NULL) && (cursor != DOBJ_PARENT_NULL))
            {
                return FALSE;
            }
            base = seed;
            base_identity = seed_is_identity;
        }
        applied = FALSE;
        for (i = depth; i != 0u; i--)
        {
            NDSRendererMatrix20p12 local;

            /* objdisplay.c:1183-1191 left-multiplies each child local matrix,
             * and a joint whose local build fails contributes nothing -- both
             * exactly as the per-binding path does.
             *
             * The per-DObj world cache used to be written here. The c112 census
             * says ndsRendererAdapterFindDObjWorldMatrix and
             * ndsRendererAdapterBuildDObjWorldMatrix both execute ZERO cycles
             * over a whole match, so every one of those stores fed a cache with
             * no reader -- 4,744,740 cycles of them, and ~4 KB a frame of write
             * traffic streamed through a 4 KB D-cache. The cache still exists
             * and the fallback path still fills it for itself; nothing populates
             * it speculatively any more. */
            /* The base is not folded in until the first contributing joint,
             * which turns two operations into one. All 55.5
             * ndsRendererMtxMulAffine20p12 calls a frame come from this loop at
             * 687 cycles each, and one per binding used to be `copy the base
             * in, then multiply the base straight back out`. When the base is
             * the identity the joint's local matrix IS the world, so it is
             * built directly into `out` and neither the multiply nor the
             * 64-byte temporary happens. */
            if ((applied == FALSE) && (base_identity != FALSE))
            {
                if (ndsRendererAdapterBuildDObjLocalMatrix(chain[i - 1u],
                                                           out) != FALSE)
                {
                    applied = TRUE;
                }
                continue;
            }
            if (ndsRendererAdapterBuildDObjLocalMatrix(chain[i - 1u],
                                                       &local) != FALSE)
            {
                if (applied == FALSE)
                {
                    ndsRendererMtxMulAffine20p12(&local, base, out);
                    applied = TRUE;
                }
                else
                {
                    ndsRendererMtxMulAffine20p12(&local, out, out);
                }
            }
        }
        if (applied == FALSE)
        {
            /* No joint contributed, so the binding's world IS its base. */
            ndsRendererMatrixCopy20p12(out, base);
        }
    }
    return TRUE;
}

/* BUGS.md fighter seams -- source renderer precision, not a mesh weld.
 *
 * BattleShip's actual render contract is visible on both sides of gSPMatrix:
 * lbCommonFighterPartsFuncMatrix (lbcommon.c:1369-1441) produces one N64 Mtx
 * LOCAL per fighter joint, and Fast3D's GfxSpMatrix (interpreter.cpp:2511-2556)
 * decodes each 16.16 cell to float and concatenates those decoded locals in
 * float.  There is NO fixed-point re-quantisation between parent and child.
 *
 * The DS GX hierarchy had been feeding those same locals to MTX_MULT4x3 after
 * reducing them to Q20.12.  The geometry engine truncates every multiply at
 * 12 fractional bits, so a deep chain can drift by far more than one matrix
 * LSB.  That is the precision surface shared by the old software path and the
 * native GX path.  Do not weld joint-local vertices: DK has no exact-local
 * cross-binding aliases at all, so the BUGS.md weld-table hypothesis would
 * literally move source vertices that are not duplicates.
 *
 * Retain the source N64 local's 16 fractional bits and compose into a Q43.20
 * affine world.  The basis remains s32; translation is s64 so stage position
 * cannot consume the fixed-point integer range.  A 32x32->64 SMULL/SMLAL
 * kernel is radically cheaper on ARM9 than software IEEE-754, and postpones
 * the one DS Q20.12 boundary until the selected draw binding.  The host oracle
 * proves every drawn Mario/DK bind-pose vertex remains within one source 1/16
 * coordinate step and one final DS matrix LSB of Fast3D.  This is a measured
 * representation adaptation of the source matrix stack, not collision-world
 * substitution (FTParts::mtx_translate was tested and is observably different
 * because it skips the per-local 16.16 boundary). */
static s64 ndsRendererAdapterSourceRoundShiftS64(s64 value, u32 shift)
{
    s64 bias;

    if (shift == 0u)
    {
        return value;
    }
    bias = (s64)1 << (shift - 1u);
    if (value < 0)
    {
        return -(((-value) + bias) >> shift);
    }
    return (value + bias) >> shift;
}

static void ndsRendererAdapterSourceWorldIdentity(
    NDSRendererAdapterSourceWorld *out)
{
    u32 row;
    u32 col;

    memset(out, 0, sizeof(*out));
    for (row = 0u; row < 3u; row++)
    {
        for (col = 0u; col < 3u; col++)
        {
            out->basis[row][col] = (row == col) ?
                (1 << NDS_RENDERER_ADAPTER_SOURCE_WORLD_FRAC_BITS) : 0;
        }
    }
}

static void ndsRendererAdapterSourceWorldFromLocal(
    NDSRendererAdapterSourceWorld *out, const Mtx *local)
{
    u32 row;
    u32 col;

    for (row = 0u; row < 3u; row++)
    {
        for (col = 0u; col < 3u; col++)
        {
            out->basis[row][col] =
                ndsRendererMtxCellS16p16(local, row, col) * 16;
        }
    }
    for (col = 0u; col < 3u; col++)
    {
        out->translation[col] =
            (s64)ndsRendererMtxCellS16p16(local, 3u, col) * 16;
    }
}

static __attribute__((noinline, optimize("Os"), target("arm"))) void
ndsRendererAdapterSourceWorldMulLocal(
    NDSRendererAdapterSourceWorld *out,
    const Mtx *local,
    const NDSRendererAdapterSourceWorld *parent)
{
    NDSRendererAdapterSourceWorld temp;
    u32 row;
    u32 col;

    for (row = 0u; row < 3u; row++)
    {
        for (col = 0u; col < 3u; col++)
        {
            s64 sum =
                (s64)ndsRendererMtxCellS16p16(local, row, 0u) *
                    parent->basis[0][col] +
                (s64)ndsRendererMtxCellS16p16(local, row, 1u) *
                    parent->basis[1][col] +
                (s64)ndsRendererMtxCellS16p16(local, row, 2u) *
                    parent->basis[2][col];

            temp.basis[row][col] = (s32)
                ndsRendererAdapterSourceRoundShiftS64(
                    sum, NDS_RENDERER_ADAPTER_SOURCE_LOCAL_FRAC_BITS);
        }
    }
    for (col = 0u; col < 3u; col++)
    {
        s64 sum =
            (s64)ndsRendererMtxCellS16p16(local, 3u, 0u) *
                parent->basis[0][col] +
            (s64)ndsRendererMtxCellS16p16(local, 3u, 1u) *
                parent->basis[1][col] +
            (s64)ndsRendererMtxCellS16p16(local, 3u, 2u) *
                parent->basis[2][col];

        temp.translation[col] =
            ndsRendererAdapterSourceRoundShiftS64(
                sum, NDS_RENDERER_ADAPTER_SOURCE_LOCAL_FRAC_BITS) +
            parent->translation[col];
    }
    memcpy(out, &temp, sizeof(temp));
}

static sb32 ndsRendererAdapterSourceWorldTo20p12(
    const NDSRendererAdapterSourceWorld *source,
    NDSRendererMatrix20p12 *out)
{
    u32 row;
    u32 col;

    for (row = 0u; row < 3u; row++)
    {
        for (col = 0u; col < 3u; col++)
        {
            out->m[row][col] = (s32)
                ndsRendererAdapterSourceRoundShiftS64(
                    source->basis[row][col],
                    NDS_RENDERER_ADAPTER_SOURCE_TO_DS_SHIFT);
        }
        out->m[row][3] = 0;
    }
    for (col = 0u; col < 3u; col++)
    {
        s64 value = ndsRendererAdapterSourceRoundShiftS64(
            source->translation[col],
            NDS_RENDERER_ADAPTER_SOURCE_TO_DS_SHIFT);

        if ((value < (s64)(-2147483647 - 1)) ||
            (value > (s64)2147483647))
        {
            return FALSE;
        }
        out->m[3][col] = (s32)value;
    }
    out->m[3][3] = 1 << NDS_RENDERER_ADAPTER_MTX_FRAC_BITS;
    return TRUE;
}

/* BattleShip lbcommon.c:1369-1441 (lbCommonFighterPartsFuncMatrix, animlocks
 * branch) via lbcommon.c:491-615 (lbCommonMatrixTraRotScaInv).
 *
 * When fp->is_use_animlocks is set, a joint whose FTParts gameplay cache is
 * cold (transform_update_mode == 0) does NOT use its DObj track values
 * directly. Source multiplies the DObj scale by the running parent-scale
 * accumulator, emits the invariant kernel with the accumulator as the
 * inverse scale and the product as the forward scale, writes the product
 * back to the part, then publishes it for that joint's children
 * (lbcommon.c:1418-1420,1422-1437,1439). A locked joint with a warm cache
 * (mode != 0, not mode-3-only) reuses the gameplay matrix
 * unk_dobjtrans_0x10 exactly like the unlocked path, then publishes the
 * STORED parts->vec_scale: source updates gLBCommonScale after BOTH locked
 * branches (lbcommon.c:1439), never passing the parent accumulator through.
 * Sibling subtrees cannot observe each other because every draw function
 * saves/restores gLBCommonScale across siblings (ftdisplaymain.c:831,914,
 * 1053), so a per-joint accumulator indexed by topology order is the exact
 * equivalent. The per-fighter seed is unit (ftDisplayMainProcDisplay,
 * ftdisplaymain.c:1162). With unit scales throughout, this kernel reduces
 * bit-exactly to the TraRotRpyRSca kernel; the host oracle pins that
 * reduction.
 *
 * Source divides columns by the accumulator with no zero guard; a zero
 * component here is a corrupt chain, not a matrix, so decline instead of
 * producing UB. Matrix math only: no interpreter or display-list APIs. */
static sb32 ndsRendererAdapterBuildAnimLockInvariantMtx(
    DObj *dobj, const Vec3f *accum_scale, Vec3f *out_vec_scale, Mtx *out)
{
    s32 indexx, indexy, indexz;
    s32 sinx, cosx, siny, cosy, sinz, cosz;
    s32 scax_l, scay_l, scaz_l;
    s32 scax_inv_l, scay_inv_l, scaz_inv_l;
    f32 inv_x, inv_y, inv_z;
    f32 vec_x, vec_y, vec_z;
    s32 e_trax, e_tray, e_traz;
    u32 e1, e2;

    if ((dobj == NULL) || (accum_scale == NULL) ||
        (out_vec_scale == NULL) || (out == NULL))
    {
        return FALSE;
    }
    /* Read everything before writing: callers may alias accum/out. */
    inv_x = accum_scale->x;
    inv_y = accum_scale->y;
    inv_z = accum_scale->z;
    if ((inv_x == 0.0F) && (inv_y == 0.0F) && (inv_z == 0.0F))
    {
        /* Not a corrupt chain: this is how source collapses a subtree.
         * lbCommonMatrixTraRotScaInv (lbcommon.c:563-599) scales row c by
         * sca<c>_l and column c by sca<c>_inv_l, and lbcommon.c:1418-1420
         * makes sca<c> = dobj->scale.<c> * 0 = 0, so with all three zero
         * every row is multiplied by zero BEFORE the saturated 1/0 reaches
         * it: the 3x3 is exactly zero and only the translation survives
         * (lbcommon.c:604-614). Reproduce that, and publish the zero for
         * this joint's children exactly as source does at lbcommon.c:1439.
         * A degenerate subtree is the source's own invisibility, not an
         * empty draw, and declining it here is what left Yoshi rejecting
         * 350 times per 1,200 presents after the joint bound was fixed. */
        out_vec_scale->x = 0.0F;
        out_vec_scale->y = 0.0F;
        out_vec_scale->z = 0.0F;
        memset(out, 0, sizeof(*out));
        e_trax = (s32)(dobj->translate.vec.f.x * 65536.0F);
        e_tray = (s32)(dobj->translate.vec.f.y * 65536.0F);
        e_traz = (s32)(dobj->translate.vec.f.z * 65536.0F);
        out->m[1][2] = COMBINE_INTEGRAL((u32)e_trax, (u32)e_tray);
        out->m[3][2] = COMBINE_FRACTIONAL((u32)e_trax, (u32)e_tray);
        out->m[1][3] = COMBINE_INTEGRAL((u32)e_traz, 0x00010000u);
        out->m[3][3] = COMBINE_FRACTIONAL((u32)e_traz, 0u);
        return TRUE;
    }
    if ((inv_x == 0.0F) || (inv_y == 0.0F) || (inv_z == 0.0F))
    {
        /* One or two zero components: source multiplies a saturated 1/0 into
         * a non-zero row, which is an overflow, not a matrix. Keep declining
         * and let the compose witness (10 | mask << 8) name the mask. */
        return FALSE;
    }
    if ((ndsFighterMatrixAngleToIndexExact(dobj->rotate.vec.f.x, &indexx) == 0) ||
        (ndsFighterMatrixAngleToIndexExact(dobj->rotate.vec.f.y, &indexy) == 0) ||
        (ndsFighterMatrixAngleToIndexExact(dobj->rotate.vec.f.z, &indexz) == 0))
    {
        return FALSE;
    }
    vec_x = dobj->scale.vec.f.x * inv_x;
    vec_y = dobj->scale.vec.f.y * inv_y;
    vec_z = dobj->scale.vec.f.z * inv_z;
    out_vec_scale->x = vec_x;
    out_vec_scale->y = vec_y;
    out_vec_scale->z = vec_z;

    sinx = ndsRendererAdapterFighterSinFromIndex(indexx);
    cosx = ndsRendererAdapterFighterCosFromIndex(indexx);
    siny = ndsRendererAdapterFighterSinFromIndex(indexy);
    cosy = ndsRendererAdapterFighterCosFromIndex(indexy);
    sinz = ndsRendererAdapterFighterSinFromIndex(indexz);
    cosz = ndsRendererAdapterFighterCosFromIndex(indexz);

    scax_l = (s32)(vec_x * 256.0F);
    scay_l = (s32)(vec_y * 256.0F);
    scaz_l = (s32)(vec_z * 256.0F);
    scax_inv_l = (s32)((1.0F / inv_x) * 256.0F);
    scay_inv_l = (s32)((1.0F / inv_y) * 256.0F);
    scaz_inv_l = (s32)((1.0F / inv_z) * 256.0F);

    e1 = (u32)((((((cosy * cosz) >> 14) * scax_l) >> 8) * scax_inv_l) >> 8);
    e2 = (u32)((((((cosy * sinz) >> 14) * scax_l) >> 8) * scay_inv_l) >> 8);
    out->m[0][0] = COMBINE_INTEGRAL(e1, e2);
    out->m[2][0] = COMBINE_FRACTIONAL(e1, e2);

    e1 = (u32)(((((-siny) * scax_l) >> 7) * scaz_inv_l) >> 8);
    out->m[0][1] = COMBINE_INTEGRAL(e1, 0u);
    out->m[2][1] = COMBINE_FRACTIONAL(e1, 0u);

    e1 = (u32)(((((((((sinx * siny) >> 15) * cosz) >> 14) - ((cosx * sinz) >> 14)) * scay_l) >> 8) * scax_inv_l) >> 8);
    e2 = (u32)(((((((((sinx * siny) >> 15) * sinz) >> 14) + ((cosx * cosz) >> 14)) * scay_l) >> 8) * scay_inv_l) >> 8);
    out->m[0][2] = COMBINE_INTEGRAL(e1, e2);
    out->m[2][2] = COMBINE_FRACTIONAL(e1, e2);

    e1 = (u32)((((((sinx * cosy) >> 14) * scay_l) >> 8) * scaz_inv_l) >> 8);
    out->m[0][3] = COMBINE_INTEGRAL(e1, 0u);
    out->m[2][3] = COMBINE_FRACTIONAL(e1, 0u);

    e1 = (u32)(((((((((cosx * siny) >> 15) * cosz) >> 14) + ((sinx * sinz) >> 14)) * scaz_l) >> 8) * scax_inv_l) >> 8);
    e2 = (u32)(((((((((cosx * siny) >> 15) * sinz) >> 14) - ((sinx * cosz) >> 14)) * scaz_l) >> 8) * scay_inv_l) >> 8);
    out->m[1][0] = COMBINE_INTEGRAL(e1, e2);
    out->m[3][0] = COMBINE_FRACTIONAL(e1, e2);

    e1 = (u32)((((((cosx * cosy) >> 14) * scaz_l) >> 8) * scaz_inv_l) >> 8);
    out->m[1][1] = COMBINE_INTEGRAL(e1, 0u);
    out->m[3][1] = COMBINE_FRACTIONAL(e1, 0u);

    e_trax = (s32)(dobj->translate.vec.f.x * 65536.0F);
    e_tray = (s32)(dobj->translate.vec.f.y * 65536.0F);
    e_traz = (s32)(dobj->translate.vec.f.z * 65536.0F);
    out->m[1][2] = COMBINE_INTEGRAL((u32)e_trax, (u32)e_tray);
    out->m[3][2] = COMBINE_FRACTIONAL((u32)e_trax, (u32)e_tray);
    out->m[1][3] = COMBINE_INTEGRAL((u32)e_traz, 0x00010000u);
    out->m[3][3] = COMBINE_FRACTIONAL((u32)e_traz, 0u);
    return TRUE;
}

/* Defined below with the other compose witnesses; declared here because this
 * builder is the fail-5 site and must name which of its six declines fired.
 * Low byte = route, high byte = the offending quantity. */
extern volatile u32 gNdsFtrComposeSourceFail;
/* Raw bits of the refused rotation component. Mask 4 alone cannot separate a
 * finite 20-radian spin from an infinity from a NaN, and the three have
 * different owners; the bit pattern separates them in one run. */
extern volatile u32 gNdsFtrComposeSourceAngle;

static sb32 ndsRendererAdapterBuildSourceFighterLocalMtx(
    DObj *dobj, const Vec3f *accum_scale, Vec3f *out_vec_scale, Mtx *out,
    sb32 *has_local)
{
    FTParts *parts;
    FTStruct *fp;
    u32 xobj_index;
    u32 fighter_parts_count = 0u;

    if ((dobj == NULL) || (dobj == DOBJ_PARENT_NULL) ||
        (out == NULL) || (has_local == NULL) ||
        (dobj->parent_gobj == NULL))
    {
        gNdsFtrComposeSourceFail = 12u;
        return FALSE;
    }
    *has_local = FALSE;
    for (xobj_index = 0u; xobj_index < dobj->xobjs_num; xobj_index++)
    {
        XObj *xobj = dobj->xobjs[xobj_index];

        if (xobj == NULL)
        {
            continue;
        }
        if (xobj->kind == nGCMatrixKindNull)
        {
            /* gcAddDObj3TransformsKind does not create a Null XObj.  Accept an
             * explicit one as the same no-op rather than inventing TRS. */
            continue;
        }
        if (xobj->kind != NDS_RENDERER_ADAPTER_FIGHTER_PARTS_MTX_KIND)
        {
            gNdsFtrComposeSourceFail = 8u | ((u32)xobj->kind << 8);
            return FALSE;
        }
        fighter_parts_count++;
    }
    if (fighter_parts_count == 0u)
    {
        return TRUE;
    }
    if (fighter_parts_count != 1u)
    {
        gNdsFtrComposeSourceFail = 9u | (fighter_parts_count << 8);
        return FALSE;
    }

    fp = ftGetStruct(dobj->parent_gobj);
    parts = ftGetParts(dobj);
    if ((fp == NULL) || (parts == NULL) || (accum_scale == NULL) ||
        (out_vec_scale == NULL))
    {
        gNdsFtrComposeSourceFail = 13u;
        return FALSE;
    }
    /* The accumulator passes through untouched on the unlocked path, which
     * never touches gLBCommonScale. Both locked branches overwrite it
     * below: warm cache publishes the stored part scale, cold cache
     * publishes this joint's product (lbcommon.c:1412-1439). */
    out_vec_scale->x = accum_scale->x;
    out_vec_scale->y = accum_scale->y;
    out_vec_scale->z = accum_scale->z;
    if (fp->is_use_animlocks != FALSE)
    {
        if (parts->transform_update_mode != 0)
        {
            /* Locked with a warm gameplay cache (any nonzero mode, not
             * mode-3-only): reuse it exactly like the unlocked path, then
             * publish the stored scale for children (lbcommon.c:1439). */
            if (ndsRendererAdapterF2LFixedWExact(
                    &parts->unk_dobjtrans_0x10, out) == FALSE)
            {
                syMatrixF2LFixedW(&parts->unk_dobjtrans_0x10, out);
            }
            out_vec_scale->x = parts->vec_scale.x;
            out_vec_scale->y = parts->vec_scale.y;
            out_vec_scale->z = parts->vec_scale.z;
        }
        else if (ndsRendererAdapterBuildAnimLockInvariantMtx(
                     dobj, accum_scale, out_vec_scale, out) == FALSE)
        {
            /* Locked with a cold gameplay cache: the DObj track values are
             * not the source local (lbcommon.c:1410-1440). Never fall
             * through to the plain TRS kernel below.
             *
             * The kernel reads the whole accumulator before it writes
             * anything, so *accum_scale is still the inbound value here and
             * this re-derives which of its two declines fired. */
            u32 zero_mask = 0u;
            s32 angle_index;
            union
            {
                f32 f;
                u32 u;
            } refused;

            refused.u = 0u;
            if (accum_scale->x == 0.0F) { zero_mask |= 1u; }
            if (accum_scale->y == 0.0F) { zero_mask |= 2u; }
            if (accum_scale->z == 0.0F) { zero_mask |= 4u; }
            if (zero_mask != 0u)
            {
                gNdsFtrComposeSourceFail = 10u | (zero_mask << 8);
                return FALSE;
            }
            if (ndsFighterMatrixAngleToIndexExact(
                    dobj->rotate.vec.f.x, &angle_index) == 0)
            {
                zero_mask |= 1u;
                refused.f = dobj->rotate.vec.f.x;
            }
            if (ndsFighterMatrixAngleToIndexExact(
                    dobj->rotate.vec.f.y, &angle_index) == 0)
            {
                zero_mask |= 2u;
                refused.f = dobj->rotate.vec.f.y;
            }
            if (ndsFighterMatrixAngleToIndexExact(
                    dobj->rotate.vec.f.z, &angle_index) == 0)
            {
                zero_mask |= 4u;
                refused.f = dobj->rotate.vec.f.z;
            }
            /* Highest set mask bit wins, so the mask names the reader. */
            gNdsFtrComposeSourceAngle = refused.u;
            gNdsFtrComposeSourceFail = 11u | (zero_mask << 8);
            return FALSE;
        }
        else
        {
            /* The kernel published this joint's product above; source also
             * writes it back to the part (lbcommon.c:1418-1420). Render
             * phase only, and only while the lock flag is set. */
            parts->vec_scale.x = out_vec_scale->x;
            parts->vec_scale.y = out_vec_scale->y;
            parts->vec_scale.z = out_vec_scale->z;
        }
    }
    else if (parts->transform_update_mode != 0)
    {
        if (ndsRendererAdapterF2LFixedWExact(
                &parts->unk_dobjtrans_0x10, out) == FALSE)
        {
            syMatrixF2LFixedW(&parts->unk_dobjtrans_0x10, out);
        }
    }
    else if (ndsFcmpNeC(dobj->scale.vec.f.x, 1.0F) ||
             ndsFcmpNeC(dobj->scale.vec.f.y, 1.0F) ||
             ndsFcmpNeC(dobj->scale.vec.f.z, 1.0F))
    {
        syMatrixTraRotRpyRSca(
            out,
            dobj->translate.vec.f.x,
            dobj->translate.vec.f.y,
            dobj->translate.vec.f.z,
            dobj->rotate.vec.f.x,
            dobj->rotate.vec.f.y,
            dobj->rotate.vec.f.z,
            dobj->scale.vec.f.x,
            dobj->scale.vec.f.y,
            dobj->scale.vec.f.z);
    }
    else if (ndsRendererAdapterBuildFighterTraRotRpyExact(
                 out,
                 dobj->translate.vec.f.x,
                 dobj->translate.vec.f.y,
                 dobj->translate.vec.f.z,
                 dobj->rotate.vec.f.x,
                 dobj->rotate.vec.f.y,
                 dobj->rotate.vec.f.z) == FALSE)
    {
        syMatrixTraRotRpyR(
            out,
            dobj->translate.vec.f.x,
            dobj->translate.vec.f.y,
            dobj->translate.vec.f.z,
            dobj->rotate.vec.f.x,
            dobj->rotate.vec.f.y,
            dobj->rotate.vec.f.z);
    }
    *has_local = TRUE;
    return TRUE;
}

/* gNdsFtrDeclineStage = 7 says the production matrices contract failed; these
 * say which of ComposeOwnerWorldsSource's declines fired, because the animlock
 * arm has no GX fallback and a decline there is a hard native reject. */
__attribute__((used)) volatile u32 gNdsFtrComposeSourceFail;
__attribute__((used)) volatile u32 gNdsFtrComposeSourceJoints;
__attribute__((used)) volatile u32 gNdsFtrComposeSourceIndex;
__attribute__((used)) volatile u32 gNdsFtrComposeSourceAngle;

static __attribute__((noinline, optimize("Os"))) sb32
ndsRendererAdapterComposeOwnerWorldsSource(
    DObj *root,
    DObj *const *bindings,
    u32 binding_count,
    NDSRendererMatrix20p12 *worlds,
    const NDSRendererMatrix20p12 *seed,
    sb32 seed_is_identity)
{
    DObj *joints[NDS_RENDERER_NATIVE_FIGHTER_JOINT_MAX];
    u8 joint_parents[NDS_RENDERER_NATIVE_FIGHTER_JOINT_MAX];
    u8 joint_bindings[NDS_RENDERER_NATIVE_FIGHTER_JOINT_MAX];
    Vec3f lock_accum[NDS_RENDERER_NATIVE_FIGHTER_JOINT_MAX];
    NDSRendererAdapterSourceWorld *source_worlds =
        sNdsRendererAdapterNativeOwnerWorkspace.
            hierarchy_storage.source_worlds;
    u32 joint_count = 0u;
    u32 joint_index;
    u32 binding_index;

    if ((root == NULL) || (root == DOBJ_PARENT_NULL) ||
        (bindings == NULL) || (worlds == NULL) || (seed == NULL) ||
        (binding_count > NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED))
    {
        gNdsFtrComposeSourceFail = 1u;
        return FALSE;
    }
    memset(joints, 0, sizeof(joints));
    memset(joint_parents, 0xff, sizeof(joint_parents));
    memset(joint_bindings, 0xff, sizeof(joint_bindings));
    if ((ndsRendererAdapterCollectFighterTopology(
             root, 0xffu, joints, joint_parents, &joint_count) == FALSE) ||
        (joint_count == 0u) ||
        (joint_count > NDS_RENDERER_NATIVE_FIGHTER_JOINT_MAX))
    {
        gNdsFtrComposeSourceFail = 2u;
        gNdsFtrComposeSourceJoints = joint_count;
        return FALSE;
    }
    for (binding_index = 0u; binding_index < binding_count; binding_index++)
    {
        u32 found = NDS_RENDERER_NATIVE_FIGHTER_JOINT_MAX;

        for (joint_index = 0u; joint_index < joint_count; joint_index++)
        {
            if (joints[joint_index] == bindings[binding_index])
            {
                found = joint_index;
                break;
            }
        }
        if ((found >= joint_count) || (joint_bindings[found] != 0xffu))
        {
            gNdsFtrComposeSourceFail = 3u;
            gNdsFtrComposeSourceJoints = joint_count;
            gNdsFtrComposeSourceIndex = binding_index;
            return FALSE;
        }
        joint_bindings[found] = (u8)binding_index;
    }

    for (joint_index = 0u; joint_index < joint_count; joint_index++)
    {
        Mtx source_local;
        NDSRendererAdapterSourceWorld *parent_world;
        sb32 has_local;
        u32 parent = joint_parents[joint_index];

        if ((joints[joint_index] == NULL) ||
            ((parent == 0xffu) ?
                 (joints[joint_index]->parent != DOBJ_PARENT_NULL) :
                 ((parent >= joint_index) ||
                  (joints[joint_index]->parent != joints[parent]))))
        {
            gNdsFtrComposeSourceFail = 4u;
            gNdsFtrComposeSourceJoints = joint_count;
            gNdsFtrComposeSourceIndex = joint_index;
            return FALSE;
        }
        if (parent == 0xffu)
        {
            ndsRendererAdapterSourceWorldIdentity(
                &source_worlds[joint_index]);
            parent_world = NULL;
            /* ftDisplayMainProcDisplay seeds gLBCommonScale to unit before
             * each fighter draw (ftdisplaymain.c:1162). */
            lock_accum[joint_index].x = 1.0F;
            lock_accum[joint_index].y = 1.0F;
            lock_accum[joint_index].z = 1.0F;
        }
        else
        {
            parent_world = &source_worlds[parent];
            lock_accum[joint_index] = lock_accum[parent];
        }
        if (ndsRendererAdapterBuildSourceFighterLocalMtx(
                joints[joint_index], &lock_accum[joint_index],
                &lock_accum[joint_index], &source_local,
                &has_local) == FALSE)
        {
            /* The builder already set the route in gNdsFtrComposeSourceFail
             * (8..13); do not clobber it. A residual 5 here would mean a
             * FALSE path in that builder without a code, which is a bug. */
            gNdsFtrComposeSourceJoints = joint_count;
            gNdsFtrComposeSourceIndex = joint_index;
            return FALSE;
        }
        if (has_local != FALSE)
        {
            if (parent_world == NULL)
            {
                ndsRendererAdapterSourceWorldFromLocal(
                    &source_worlds[joint_index],
                    &source_local);
            }
            else
            {
                ndsRendererAdapterSourceWorldMulLocal(
                    &source_worlds[joint_index],
                    &source_local, parent_world);
            }
        }
        else if (parent_world != NULL)
        {
            memcpy(&source_worlds[joint_index],
                   parent_world, sizeof(*parent_world));
        }

        if (joint_bindings[joint_index] != 0xffu)
        {
            NDSRendererMatrix20p12 world;
            u32 out_index = joint_bindings[joint_index];

            /* The selected binding is the one DS representation boundary. */
            if (ndsRendererAdapterSourceWorldTo20p12(
                    &source_worlds[joint_index],
                    &world) == FALSE)
            {
                gNdsFtrComposeSourceFail = 6u;
                gNdsFtrComposeSourceJoints = joint_count;
                gNdsFtrComposeSourceIndex = joint_index;
                return FALSE;
            }
            if (seed_is_identity != FALSE)
            {
                ndsRendererMatrixCopy20p12(&worlds[out_index], &world);
            }
            else
            {
                ndsRendererMtxMulAffine20p12(
                    &world, seed, &worlds[out_index]);
            }
        }
    }
    return TRUE;
}

/* The per-binding world build, outlined. It is the fail-closed path for when
 * the flat compose declines, and in a whole-match census it declined zero times
 * in 49,422 binding visits -- yet it was inlined into
 * ndsFighterMarioFoxDLAllDrawForSlot and held ~3,150 of that function's 10,708
 * bytes, `ndsRendererAdapterBuildDObjWorldMatrix` and all. The 64-byte `world`
 * matrix moves out of the hot frame with it. Outlined, never deleted: the flat
 * compose is allowed to decline, and when it does this must still be correct. */
static sb32 __attribute__((noinline, cold, optimize("Os")))
ndsRendererAdapterPrepareOwnerMatricesPerBinding(
    DObj *const *bindings,
    u32 binding_count,
    const NDSRendererMatrix20p12 *camera_modelview,
    u32 camera_modelview_valid,
    const NDSRendererMatrix20p12 **modelview_ptrs
    )
{
    NDSRendererMatrix20p12 world;
    u32 binding_index;

    for (binding_index = 0u;
         binding_index < binding_count;
         binding_index++)
    {
        if (bindings[binding_index] != NULL)
        {
            if (ndsRendererAdapterBuildDObjWorldMatrix(
                    bindings[binding_index], &world) == FALSE)
            {
                return FALSE;
            }
            /* R2-03 E32, and this is the whole cut: lbcommon.c:1627's
             * `f[3][0] += x; f[3][1] += y;` on the world matrix, before the
             * camera multiply below. Zero when the fighter is not in hitlag. */
            world.m[3][0] += sNdsR2ShuffleWorldX;
            world.m[3][1] += sNdsR2ShuffleWorldY;
            if (camera_modelview_valid != FALSE)
            {
                ndsRendererMtxMulAffine20p12(
                    &world, camera_modelview,
                    &sNdsRendererAdapterNativeOwnerModelviews[
                        binding_index]);
            }
            else
            {
                sNdsRendererAdapterNativeOwnerModelviews[binding_index] =
                    world;
            }
            modelview_ptrs[binding_index] =
                &sNdsRendererAdapterNativeOwnerModelviews[binding_index];
        }
        else if (camera_modelview_valid != FALSE)
        {
            sNdsRendererAdapterNativeOwnerModelviews[binding_index] =
                *camera_modelview;
            modelview_ptrs[binding_index] =
                &sNdsRendererAdapterNativeOwnerModelviews[binding_index];
        }
    }
    return TRUE;
}

/* Do NOT add `noinline` here. It was tried (slice 14) on the theory that shrinking
 * the 10.5 KB driver toward the 8 KB I-cache is good regardless of what moves:
 * the driver did lose 916 bytes and FTR rose 2,192. Outlining code that RUNS is
 * not the same lever as outlining code that never runs -- slice 12 won 5,747 by
 * moving out bytes with zero executions, and this loses by moving out bytes with
 * many. */
static sb32 ndsRendererAdapterPrepareNativeOwnerMatrices(
    u32 slot,
    u32 use_low_detail,
    DObj *root,
    DObj *const *bindings,
    u32 binding_count,
    CObj *cobj,
    const NDSRendererMatrix20p12 **projection_ptr,
    const NDSRendererMatrix20p12 **modelview_ptrs
    )
{
    NDSRendererMatrix20p12 camera_projection;
    NDSRendererMatrix20p12 camera_modelview;
    u32 camera_projection_valid = FALSE;
    u32 camera_modelview_valid = FALSE;
    u32 binding_index;
    NDSRendererMatrix20p12 compose_seed;
    sb32 seed_is_identity;
    u32 flat_worlds = FALSE;

    if ((bindings == NULL) || (projection_ptr == NULL) ||
        (modelview_ptrs == NULL) ||
        (binding_count > NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED))
    {
        return FALSE;
    }
    *projection_ptr = NULL;
    for (binding_index = 0u;
         binding_index < binding_count;
         binding_index++)
    {
        modelview_ptrs[binding_index] = NULL;
    }

    ndsRendererAdapterGetFrameCameraMatrices(
        cobj, &camera_projection, &camera_projection_valid,
        &camera_modelview, &camera_modelview_valid,
        NULL, NULL, NULL);
    if (camera_projection_valid != FALSE)
    {
        sNdsRendererAdapterNativeOwnerProjection = camera_projection;
        *projection_ptr = &sNdsRendererAdapterNativeOwnerProjection;
    }

    (void)root;
    /* The seed the whole binding forest hangs off. Camera when there is one, so
     * the compose lands in camera space directly; identity when there is not,
     * which is the old world-space result unchanged. */
    if (camera_modelview_valid != FALSE)
    {
        ndsRendererMatrixCopy20p12(&compose_seed, &camera_modelview);
        seed_is_identity = FALSE;
    }
    else
    {
        /* The measured case. NDS_R2_FIGHTER_HW_MTX hands the camera to the
         * hardware, so this path composes world matrices and the seed is the
         * identity -- which is why the compose can skip folding it in at all. */
        ndsRendererAdapterMtxIdentity20p12(&compose_seed);
        seed_is_identity = TRUE;
    }
    /* R2-03 E32's hitlag shuffle used to be added to every binding's world row
     * 3 before the camera multiply. `world * T * camera` reassociates to
     * `world * (T * camera)` and T is the same for every binding, so one 4x4 a
     * frame replaces one row-3 add per binding -- and it keeps the shuffle out
     * of the compose entirely. Zero whenever the fighter is not in hitlag. */
    if ((sNdsR2ShuffleWorldX != 0) || (sNdsR2ShuffleWorldY != 0))
    {
        NDSRendererMatrix20p12 shuffle;

        ndsRendererAdapterMtxIdentity20p12(&shuffle);
        shuffle.m[3][0] = sNdsR2ShuffleWorldX;
        shuffle.m[3][1] = sNdsR2ShuffleWorldY;
        ndsRendererMtxMulAffine20p12(&shuffle, &compose_seed, &compose_seed);
        seed_is_identity = FALSE;
    }
    /* Animation locks change the source local matrix and scale accumulator.
     * Compose those source matrices on ARM9, then submit the same native owner
     * geometry. A failed lock conversion must not fall into ordinary TRS. */
    if ((root != NULL) && (root->parent_gobj != NULL))
    {
        FTStruct *source_fp = ftGetStruct(root->parent_gobj);
        if ((source_fp != NULL) && (source_fp->is_use_animlocks != FALSE))
        {
            flat_worlds = ndsRendererAdapterComposeOwnerWorldsSource(
                root, bindings, binding_count,
                sNdsRendererAdapterNativeOwnerModelviews, &compose_seed,
                seed_is_identity);
            if (flat_worlds == FALSE)
            {
                return FALSE;
            }
        }
    }
    /* One forward pass over the baked binding order, composing straight into the
     * modelview array so the worlds need no second home -- and, since the seed
     * carries the camera, no second pass either. On failure nothing has been
     * consumed yet and the per-binding path runs unchanged. */
    /* Slice 43. Same forward pass, no multiplies: describe each binding's chain
     * and let the geometry engine compose it in the root loop. Declining leaves
     * nothing consumed, exactly as the compose does, so the CPU pass below is
     * still the fail-closed answer. */
    sNdsRendererAdapterNativeOwnerWorkspace.gx_valid = 0u;
    sNdsRendererAdapterNativeOwnerWorkspace.gx_modelview_mirror_valid = 0u;
    /* Source-world seam repair.  Keep this deliberately narrow until every
     * owner is visually/source-qualified: Mario is owner slot 0, and profile
     * owner ids are one-based so DK's native slot is DONKEY-1. */
    if ((flat_worlds != FALSE) || (((slot == 0u)
#if NDS_P2_DONKEY
         || (slot == NDS_RENDERER_NATIVE_FIGHTER_OWNER_DONKEY)
#endif
         ) &&
        (ndsRendererAdapterComposeOwnerWorldsSource(
             root, bindings, binding_count,
             sNdsRendererAdapterNativeOwnerModelviews, &compose_seed,
             seed_is_identity) != FALSE)))
    {
        flat_worlds = TRUE;
    }
    else if (ndsRendererAdapterCaptureOwnerChainsGx(
            slot, bindings, binding_count,
            &sNdsRendererAdapterNativeOwnerWorkspace) != FALSE)
    {
        sNdsRendererAdapterNativeOwnerWorkspace.gx_valid = 1u;
        ndsRendererMatrixCopy20p12(
            &sNdsRendererAdapterNativeOwnerWorkspace.gx_seed, &compose_seed);
        sNdsRendererAdapterNativeOwnerWorkspace.gx_seed_is_identity =
            (u8)((seed_is_identity != FALSE) ? 1u : 0u);
#if NDS_P2_LINK
        /* P2-3f31. Link's source model contains a regular G_TEXTURE_GEN epoch.
         * Keep GX as the hierarchy executor, but mirror the exact same forward
         * compose on ARM9 so Fast3D's LookAt transform has the finished current
         * modelview it semantically consumes. This is representation-only: the
         * hardware still loads gx_seed/gx_locals below, and the CPU mirror is
         * never submitted as the fighter matrix. If the mirror cannot be built,
         * decline GX and fall through to the already-correct CPU path.
         * P2-6: every owner whose tables set G_TEXTURE_GEN, not only Link --
         * the polygons and Metal Mario are environment-mapped whole, and
         * without the mirror each texgen run rejected the owner (the fighter
         * drew nothing). */
        if (ndsRendererNativeFighterOwnerUsesTexgen(slot, use_low_detail) !=
            FALSE)
        {
            if (ndsRendererAdapterComposeOwnerWorldsFlat(
                    slot, bindings, binding_count,
                    sNdsRendererAdapterNativeOwnerModelviews, &compose_seed,
                    seed_is_identity) == FALSE)
            {
                sNdsRendererAdapterNativeOwnerWorkspace.gx_valid = 0u;
            }
            else
            {
                sNdsRendererAdapterNativeOwnerWorkspace.
                    gx_modelview_mirror_valid = 1u;
            }
        }
#endif
        if (sNdsRendererAdapterNativeOwnerWorkspace.gx_valid == 0u)
        {
            goto gx_compose_declined;
        }
        for (binding_index = 0u;
             binding_index < binding_count;
             binding_index++)
        {
            if (sNdsRendererAdapterNativeOwnerWorkspace.
                    gx_modelview_mirror_valid != 0u)
            {
                modelview_ptrs[binding_index] =
                    &sNdsRendererAdapterNativeOwnerModelviews[binding_index];
            }
            else
            {
                /* Never loaded by ordinary GX owners, but every preflight
                 * NULL-checks it and the seed is the honest answer for what the
                 * CPU retained. */
                modelview_ptrs[binding_index] =
                    &sNdsRendererAdapterNativeOwnerWorkspace.gx_seed;
            }
        }
        /* `*projection_ptr` was already published above, or deliberately left
         * NULL when the camera had no projection; do not second-guess it. */
        return TRUE;
    }
gx_compose_declined:
    if (flat_worlds == FALSE)
    {
        NDS_DIAG(gNdsR2GxComposeDeclines++);
    }
    if (flat_worlds == FALSE)
    {
        flat_worlds = ndsRendererAdapterComposeOwnerWorldsFlat(
            slot, bindings, binding_count,
            sNdsRendererAdapterNativeOwnerModelviews, &compose_seed,
            seed_is_identity);
    }
    /* `flat_worlds` is decided once, above, and never changes inside the loop,
     * so hoisting it out of the loop is a pure transformation -- and it is what
     * lets the fallback leave the hot function entirely. */
    if (flat_worlds != FALSE)
    {
        /* Nothing left to do per binding but publish the pointer: the compose
         * already wrote camera-space modelviews in place. A NULL binding makes
         * the compose decline, so inside this arm every binding is non-NULL. */
        for (binding_index = 0u;
             binding_index < binding_count;
             binding_index++)
        {
            modelview_ptrs[binding_index] =
                &sNdsRendererAdapterNativeOwnerModelviews[binding_index];
        }
    }
    else
    {
        if (ndsRendererAdapterPrepareOwnerMatricesPerBinding(
                bindings, binding_count, &camera_modelview,
                camera_modelview_valid, modelview_ptrs
                ) == FALSE)
        {
            return FALSE;
        }
    }
    return ((*projection_ptr != NULL) ||
            (camera_modelview_valid != FALSE) ||
            (binding_count == 0u)) ? TRUE : FALSE;
}

static sb32 ndsRendererAdapterMatrixIsAffine20p12(
    const NDSRendererMatrix20p12 *matrix)
{
    return ((matrix != NULL) &&
            (matrix->m[0][3] == 0) &&
            (matrix->m[1][3] == 0) &&
            (matrix->m[2][3] == 0) &&
            (matrix->m[3][3] == (1 << 12))) ? TRUE : FALSE;
}

/* The ordinary camera adapter preserves BattleShip's historical combined
 * 0x4C look-at/projection matrix.  The hierarchy candidate needs the same
 * product split at its natural affine boundary so source-unit translation can
 * be scaled once in GX without CPU-composing every fighter root. */
/* One-entry memo of the look-at/perspective pair (P2-2p8, 2026-09-28). The
 * native actor slots (Yoshi's Island's three clouds, the barrel cannons)
 * each call this once per draw, and each call rebuilt both
 * matrices from the same camera: ~8K ticks a frame on Yoshi's Island. The
 * outputs are a pure function of the key below -- the eye/at/up and
 * perspective words and the float/fixed route word -- plus the norm the
 * perspective build writes back, which a hit requires to be unchanged. */
#define NDS_RENDERER_ADAPTER_HIERARCHY_CAMERA_KEY_WORDS 15u
typedef struct NDSRendererAdapterHierarchyCameraMemo
{
    const CObj *cobj;
    u32 key[NDS_RENDERER_ADAPTER_HIERARCHY_CAMERA_KEY_WORDS];
    NDSRendererMatrix20p12 projection;
    NDSRendererMatrix20p12 modelview;
    u16 norm;
    u16 valid;
} NDSRendererAdapterHierarchyCameraMemo;
static NDSRendererAdapterHierarchyCameraMemo
    sNdsRendererAdapterHierarchyCameraMemo;
volatile u32 gNdsRendererAdapterHierarchyCameraMemoHits;

static void ndsRendererAdapterHierarchyCameraKey(const CObj *cobj, u32 *key)
{
    const GCPersp *persp = &cobj->projection.persp;

    memcpy(&key[0], &cobj->vec.eye, 3u * sizeof(u32));
    memcpy(&key[3], &cobj->vec.at, 3u * sizeof(u32));
    memcpy(&key[6], &cobj->vec.up, 3u * sizeof(u32));
    memcpy(&key[9], &persp->fovy, sizeof(u32));
    memcpy(&key[10], &persp->aspect, sizeof(u32));
    memcpy(&key[11], &persp->near, sizeof(u32));
    memcpy(&key[12], &persp->far, sizeof(u32));
    memcpy(&key[13], &persp->scale, sizeof(u32));
    key[14] = 1u; /* the Q20.12 chain (formerly its route word) */
}

/* Cold with the hierarchy mode it serves: only
 * NDS_RENDERER_FAST_RUN_NATIVE_FIGHTERS reaches it, and the live mode is
 * NATIVE_FIGHTER_OWNER_PRODUCTION. */
static sb32 __attribute__((noinline, cold, optimize("Os")))
ndsRendererAdapterGetHierarchyCameraMatrices(
    CObj *cobj,
    NDSRendererMatrix20p12 *projection,
    NDSRendererMatrix20p12 *modelview)
{
    u32 projection_valid = FALSE;
    u32 modelview_valid = FALSE;
    u32 i;

    if ((projection == NULL) || (modelview == NULL))
    {
        return FALSE;
    }
    ndsRendererAdapterMtxIdentity20p12(projection);
    ndsRendererAdapterMtxIdentity20p12(modelview);
    if (cobj == NULL)
    {
        return FALSE;
    }
    for (i = 0u; i < (u32)cobj->xobjs_num; i++)
    {
        XObj *xobj = cobj->xobjs[i];

        if ((xobj != NULL) &&
            (xobj->kind == NDS_RENDERER_ADAPTER_GM_CAMERA_MTX_KIND))
        {
            LookAt look_at;
            NDSRendererAdapterHierarchyCameraMemo *memo =
                &sNdsRendererAdapterHierarchyCameraMemo;
            u32 key[NDS_RENDERER_ADAPTER_HIERARCHY_CAMERA_KEY_WORDS];

            ndsRendererAdapterHierarchyCameraKey(cobj, key);
            if ((memo->valid != 0u) && (memo->cobj == cobj) &&
                (memcmp(memo->key, key, sizeof(key)) == 0) &&
                (cobj->projection.persp.norm == memo->norm))
            {
                ndsRendererMatrixCopy20p12(projection, &memo->projection);
                ndsRendererMatrixCopy20p12(modelview, &memo->modelview);
                NDS_DIAG(gNdsRendererAdapterHierarchyCameraMemoHits++);
                return TRUE;
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
            memo->cobj = cobj;
            memcpy(memo->key, key, sizeof(key));
            memo->norm = cobj->projection.persp.norm;
            ndsRendererMatrixCopy20p12(&memo->projection, projection);
            ndsRendererMatrixCopy20p12(&memo->modelview, modelview);
            memo->valid = 1u;
            return TRUE;
        }
    }
    ndsRendererAdapterGetFrameCameraMatrices(
        cobj, projection, &projection_valid,
        modelview, &modelview_valid,
        NULL, NULL, NULL);
    if (projection_valid == FALSE)
    {
        ndsRendererAdapterMtxIdentity20p12(projection);
    }
    if (modelview_valid == FALSE)
    {
        ndsRendererAdapterMtxIdentity20p12(modelview);
    }
    return ((projection_valid != FALSE) ||
            (modelview_valid != FALSE)) ? TRUE : FALSE;
}

/* The alternate matrix mode, selected only by
 * NDS_RENDERER_FAST_RUN_NATIVE_FIGHTERS. The c112 cold map found it and its
 * camera helper inlined into ndsFighterMarioFoxDLAllDrawForSlot across two cold
 * runs totalling ~1,100 bytes, none of which executes in a whole match. It is
 * still a live mode and still correct; it just stops renting I-cache lines from
 * the mode that does run. */
static sb32 __attribute__((noinline, cold, optimize("Os")))
ndsRendererAdapterPrepareNativeOwnerHierarchy(
    u32 slot,
    FTStruct *fp,
    DObj *root,
    DObj *const *matrix_bindings,
    u32 binding_count,
    CObj *cobj,
    NDSRendererAdapterNativeOwnerWorkspace *workspace
    )
{
    u32 expected_joint_count = (slot == 0u) ? 25u : 27u;
    u32 expected_binding_count = (slot == 0u) ? 14u : 18u;
    u32 joint_count = 0u;
    u32 joint_index;
    u32 binding_index;

    if ((slot > 1u) || (fp == NULL) || (root == NULL) ||
        (matrix_bindings == NULL) || (workspace == NULL) ||
        (binding_count != expected_binding_count) ||
        (fp->is_use_animlocks != FALSE) || (fp->shuffle_tics != 0u))
    {
        return FALSE;
    }
    memset(workspace->hierarchy_joints, 0,
           sizeof(workspace->hierarchy_joints));
    memset(workspace->hierarchy_parents, 31,
           sizeof(workspace->hierarchy_parents));
    memset(workspace->hierarchy_bindings, 31,
           sizeof(workspace->hierarchy_bindings));
    if ((ndsRendererAdapterCollectFighterTopology(
             root, 31u, workspace->hierarchy_joints,
             workspace->hierarchy_parents, &joint_count) == FALSE) ||
        (joint_count != expected_joint_count))
    {
        return FALSE;
    }
    for (binding_index = 0u; binding_index < binding_count; binding_index++)
    {
        u32 found = NDS_RENDERER_NATIVE_FIGHTER_JOINT_MAX;

        for (joint_index = 0u; joint_index < joint_count; joint_index++)
        {
            if (workspace->hierarchy_joints[joint_index] ==
                matrix_bindings[binding_index])
            {
                found = joint_index;
                break;
            }
        }
        if ((found >= joint_count) ||
            (workspace->hierarchy_bindings[found] != 31u))
        {
            return FALSE;
        }
        workspace->hierarchy_bindings[found] = (u8)binding_index;
    }
    for (joint_index = 0u; joint_index < joint_count; joint_index++)
    {
        DObj *joint = workspace->hierarchy_joints[joint_index];
        u32 xobj_index;

        if ((joint == NULL) ||
            ((workspace->hierarchy_parents[joint_index] == 31u) ?
                 (joint->parent != DOBJ_PARENT_NULL) :
                 (joint->parent != workspace->hierarchy_joints[
                     workspace->hierarchy_parents[joint_index]])))
        {
            return FALSE;
        }
        for (xobj_index = 0u; xobj_index < joint->xobjs_num; xobj_index++)
        {
            XObj *xobj = joint->xobjs[xobj_index];

            if (xobj == NULL)
            {
                continue;
            }
            if ((xobj->kind != nGCMatrixKindNull) &&
                (xobj->kind !=
                 NDS_RENDERER_ADAPTER_FIGHTER_PARTS_MTX_KIND))
            {
                return FALSE;
            }
            if ((xobj->kind ==
                 NDS_RENDERER_ADAPTER_FIGHTER_PARTS_MTX_KIND) &&
                (ftGetParts(joint) == NULL))
            {
                return FALSE;
            }
        }
        if ((ndsRendererAdapterBuildDObjLocalMatrix(
                 joint,
                 &workspace->hierarchy_storage.hierarchy_locals[joint_index]) ==
             FALSE) ||
            (ndsRendererAdapterMatrixIsAffine20p12(
                 &workspace->hierarchy_storage.
                      hierarchy_locals[joint_index]) == FALSE))
        {
            return FALSE;
        }
    }
    if (ndsRendererAdapterGetHierarchyCameraMatrices(
            cobj, &workspace->hierarchy_projection,
            &workspace->hierarchy_camera_modelview) == FALSE)
    {
        return FALSE;
    }
    workspace->hierarchy.projection = &workspace->hierarchy_projection;
    workspace->hierarchy.camera_modelview =
        &workspace->hierarchy_camera_modelview;
    workspace->hierarchy.joint_locals =
        workspace->hierarchy_storage.hierarchy_locals;
    workspace->hierarchy.joint_parents = workspace->hierarchy_parents;
    workspace->hierarchy.joint_bindings = workspace->hierarchy_bindings;
    workspace->hierarchy.joint_count = joint_count;
    return TRUE;
}

/* P2-4 native stage actor slot (Congo Jungle barrel cannon first). Same
 * hierarchy executor machinery as the fighter slot above, but the expected
 * joint/binding counts come from the actor packet
 * (include/nds/generated/nds_native_actor_tarucann.generated.h), not the
 * fighter slot constants: the barrel is 2 joints / 2 bindings against the
 * fighters' 25-27 / 14-18. The XObj gate is the actor's own allowlist from
 * dGRJungleTaruCannTransformKinds (grjungle.c:12-16): Null, RotRpyR and
 * TraRotRpyRSca plus the 33..40 billboard range for the root kind 0x28.
 * Kind 40 is the source Y-billboard-with-translate (objdisplay.c
 * func_80010918 TRUE arm, case 40) and the port builds it with identical
 * math (BuildBillboardMtxF 39/40, even kind => translate), so the barrel's
 * camera-facing base times its live rotate.z sweep needs no special case:
 * the yaw comes out right through BuildDObjLocalMatrix like every other
 * XObj. Non-static: the route lives in reloc_backend_movement.c (its own
 * TU); the declaration rides the generated actor header.
 *
 * The barrel needs two local matrices, not the fighter owner's full material,
 * topology and animation scratch. It is consumed synchronously by its native
 * draw below and owns no scene pointers between calls. */
#if NDS_P2_STAGE_JUNGLE
typedef struct NDSRendererAdapterNativeActorWorkspace
{
    DObj *hierarchy_joints[2];
    u8 hierarchy_parents[2];
    u8 hierarchy_bindings[2];
    struct { NDSRendererMatrix20p12 hierarchy_locals[2]; } hierarchy_storage;
    NDSRendererMatrix20p12 hierarchy_projection;
    NDSRendererMatrix20p12 hierarchy_camera_modelview;
    NDSRendererNativeFighterHierarchy hierarchy;
} NDSRendererAdapterNativeActorWorkspace;

static NDSRendererAdapterNativeActorWorkspace
    sNdsRendererAdapterNativeActorWorkspace;

static sb32 ndsRendererAdapterPrepareNativeActorHierarchy(
    void *root_ptr,
    void *const *matrix_bindings_ptr,
    u32 binding_count,
    u32 expected_joint_count,
    u32 expected_binding_count,
    void *cobj_ptr,
    void *workspace_ptr)
{
    DObj *root = (DObj *)root_ptr;
    DObj *const *matrix_bindings = (DObj *const *)matrix_bindings_ptr;
    CObj *cobj = (CObj *)cobj_ptr;
    NDSRendererAdapterNativeActorWorkspace *workspace =
        (NDSRendererAdapterNativeActorWorkspace *)workspace_ptr;
    u32 joint_count = NDS_NATIVE_ACTOR_TARUCANN_JOINT_COUNT;
    u32 joint_index;
    u32 binding_index;

    if (workspace == NULL)
    {
        workspace = &sNdsRendererAdapterNativeActorWorkspace;
    }
    if ((root == NULL) || (matrix_bindings == NULL) ||
        (expected_joint_count != NDS_NATIVE_ACTOR_TARUCANN_JOINT_COUNT) ||
        (expected_binding_count != NDS_NATIVE_ACTOR_TARUCANN_BINDING_COUNT) ||
        (binding_count != expected_binding_count) ||
        (binding_count > expected_joint_count))
    {
        return FALSE;
    }
    memset(workspace->hierarchy_joints, 0,
           sizeof(workspace->hierarchy_joints));
    memset(workspace->hierarchy_parents, 31,
           sizeof(workspace->hierarchy_parents));
    memset(workspace->hierarchy_bindings, 31,
           sizeof(workspace->hierarchy_bindings));
    if ((root->sib_next != NULL) || (root->child == NULL) ||
        (root->child->sib_next != NULL) || (root->child->child != NULL))
    {
        return FALSE;
    }
    workspace->hierarchy_joints[0] = root;
    workspace->hierarchy_joints[1] = root->child;
    workspace->hierarchy_parents[0] = NDS_NATIVE_ACTOR_TARUCANN_JOINT0_PARENT;
    workspace->hierarchy_parents[1] = NDS_NATIVE_ACTOR_TARUCANN_JOINT1_PARENT;
    for (binding_index = 0u; binding_index < binding_count; binding_index++)
    {
        u32 found = NDS_RENDERER_NATIVE_FIGHTER_JOINT_MAX;

        for (joint_index = 0u; joint_index < joint_count; joint_index++)
        {
            if (workspace->hierarchy_joints[joint_index] ==
                matrix_bindings[binding_index])
            {
                found = joint_index;
                break;
            }
        }
        if ((found >= joint_count) ||
            (workspace->hierarchy_bindings[found] != 31u))
        {
            return FALSE;
        }
        workspace->hierarchy_bindings[found] = (u8)binding_index;
    }
    for (joint_index = 0u; joint_index < joint_count; joint_index++)
    {
        DObj *joint = workspace->hierarchy_joints[joint_index];
        u32 xobj_index;

        if ((joint == NULL) ||
            ((workspace->hierarchy_parents[joint_index] == 31u) ?
                 (joint->parent != DOBJ_PARENT_NULL) :
                 (joint->parent != workspace->hierarchy_joints[
                     workspace->hierarchy_parents[joint_index]])))
        {
            return FALSE;
        }
        if (joint->xobjs_num != ((joint_index == 0u) ? 2u : 1u))
        {
            return FALSE;
        }
        for (xobj_index = 0u; xobj_index < joint->xobjs_num; xobj_index++)
        {
            XObj *xobj = joint->xobjs[xobj_index];
            u32 kind;

            if (xobj == NULL)
            {
                return FALSE;
            }
            kind = xobj->kind;
            if (kind != ((joint_index == 0u) ?
                    ((xobj_index == 0u) ?
                        NDS_NATIVE_ACTOR_TARUCANN_JOINT0_KIND0 :
                        NDS_NATIVE_ACTOR_TARUCANN_JOINT0_KIND1) :
                    NDS_NATIVE_ACTOR_TARUCANN_JOINT1_KIND0))
            {
                return FALSE;
            }
        }
        if ((ndsRendererAdapterBuildDObjLocalMatrix(
                 joint,
                 &workspace->hierarchy_storage.hierarchy_locals[joint_index]) ==
             FALSE) ||
            (ndsRendererAdapterMatrixIsAffine20p12(
                 &workspace->hierarchy_storage.
                      hierarchy_locals[joint_index]) == FALSE))
        {
            return FALSE;
        }
    }
    if (ndsRendererAdapterGetHierarchyCameraMatrices(
            cobj, &workspace->hierarchy_projection,
            &workspace->hierarchy_camera_modelview) == FALSE)
    {
        return FALSE;
    }
    workspace->hierarchy.projection = &workspace->hierarchy_projection;
    workspace->hierarchy.camera_modelview =
        &workspace->hierarchy_camera_modelview;
    workspace->hierarchy.joint_locals =
        workspace->hierarchy_storage.hierarchy_locals;
    workspace->hierarchy.joint_parents = workspace->hierarchy_parents;
    workspace->hierarchy.joint_bindings = workspace->hierarchy_bindings;
    workspace->hierarchy.joint_count = joint_count;
    return TRUE;
}

volatile u32 gNdsNativeTaruCannFailStep;
volatile u32 gNdsNativeTaruCannFailAsset;
volatile u32 gNdsNativeTaruCannFailDvOffset;

sb32 ndsRendererAdapterSubmitNativeTaruCann(void *root_ptr, void *cobj,
    u32 initial_geometry_mode, NDSRendererStats *stats)
{
    DObj *root = root_ptr;
    void *bindings[2];
    NDSRelocLoadedFile *loaded;
    const void *live_data = NULL;
    u32 live_size = 0u;

    if ((root == NULL) || (root->child == NULL) || (stats == NULL))
    {
        return FALSE;
    }
    /* gcDrawDObjTree skips hidden subtrees and NOTEXTURE display lists.
     * These are ordinary animation states, not a broken topology. */
    if (((root->flags | root->child->flags) & DOBJ_FLAG_HIDDEN) != 0u ||
        (root->child->flags & DOBJ_FLAG_NOTEXTURE) != 0u)
    {
        return TRUE;
    }
    loaded = ndsRelocFindLoadedFileContaining(root->child->dv, sizeof(Gfx));
    /* Fail-step witness: this arm was unreachable until the classifier stopped
     * aliasing the barrel to Dream Land's map GObjs (2026-09-07), so its
     * checks had never run live. The liveness test is the loader's own
     * (owner scene + reloc scene generation, ndsRelocGetLoadedAssetView); the
     * first live run compared owner_generation against the TASKMAN heap
     * generation, a different counter, and rejected every frame (step 3). */
    gNdsNativeTaruCannFailStep =
        (loaded == NULL) ? 1u :
        (loaded->asset_id != 158u) ? 2u :
        ((ndsRelocGetLoadedAssetView(158u, &live_data, &live_size) == FALSE) ||
         (live_data != loaded->data)) ? 3u :
        (root->child->dv != (void *)((u8 *)loaded->data + 0x0a08u)) ? 4u :
        (root->dv != NULL) ? 5u :
        (root->mobj != NULL) ? 6u :
        (root->child->mobj != NULL) ? 7u : 0u;
    gNdsNativeTaruCannFailAsset = (loaded != NULL) ? loaded->asset_id : 0u;
    gNdsNativeTaruCannFailDvOffset = (loaded != NULL) ?
        (u32)((u8 *)root->child->dv - (u8 *)loaded->data) : 0u;
    if (gNdsNativeTaruCannFailStep != 0u)
    {
        return FALSE;
    }
    bindings[0] = root;
    bindings[1] = root->child;
    if (ndsRendererAdapterPrepareNativeActorHierarchy(root, bindings, 2u,
            NDS_NATIVE_ACTOR_TARUCANN_JOINT_COUNT,
            NDS_NATIVE_ACTOR_TARUCANN_BINDING_COUNT, cobj, NULL) == FALSE)
    {
        gNdsNativeTaruCannFailStep = 8u;
        return FALSE;
    }
    return ndsRendererSubmitNativeTaruCann(loaded->data, loaded->data_size,
        &sNdsRendererAdapterNativeActorWorkspace.hierarchy,
        initial_geometry_mode, stats);
}
#endif
/* YOSTER-CLOUDS: 7-joint hierarchy slot for one cloud instance. Same
 * executor machinery as the barrel slot: expected counts come from the
 * cloud packet header, the XObj allowlist is root/mids Tra(18) plus
 * drawable Tra+kind48 (gryoster.c:242-243), locals are built live with
 * ndsRendererAdapterBuildDObjLocalMatrix so kind48 stays a live recalc,
 * and the three prim alphas are the drawables' live MObj anim state
 * (mobj->sub.primcolor.s.a, the field objdisplay.c submits). */
#if NDS_P2_STAGE_YOSTER
typedef struct NDSRendererAdapterNativeYosterCloudWorkspace
{
    DObj *hierarchy_joints[7];
    u8 hierarchy_parents[7];
    u8 hierarchy_bindings[7];
    struct { NDSRendererMatrix20p12 hierarchy_locals[7]; } hierarchy_storage;
    NDSRendererMatrix20p12 hierarchy_projection;
    NDSRendererMatrix20p12 hierarchy_camera_modelview;
    NDSRendererMatrix20p12 drawable_mvps[3];
    NDSRendererNativeFighterHierarchy hierarchy;
} NDSRendererAdapterNativeYosterCloudWorkspace;

static NDSRendererAdapterNativeYosterCloudWorkspace
    sNdsRendererAdapterNativeYosterCloudWorkspace;

volatile u32 gNdsNativeYosterCloudFailStep;

/* P2-2p8 (2026-10-05): the cloud's matrices without the generic builders.
 * Every local is a Tra -- the root and mids carry Tra alone, a drawable Tra
 * beside the kind-48 recalc its build skips -- so each is the identity with
 * row 3 = RoundShift20p12(FTOFIX32(translate)) (syMatrixTra, then the 16.16 ->
 * 20.12 load; ndsRendererAdapterFloatPow2ToS32 is FTOFIX32 bit for bit). Such
 * products are exact: a drawable's chain is the identity with the three
 * translations summed in row 3, and the kind-48 recalc keeps only row 3 of
 * chain x camera modelview, formed here with ndsRendererMtxMulRow3_20p12's
 * arithmetic. 7 local builds (syMatrixTra, a Mtx split and a 16-cell load
 * each) and 9 full multiplies become 21 integer conversions and 3 row
 * products; the matrices the executor reads are the old path's. A
 * translation at or past 2^14 units takes the old path. */

static sb32 ndsRendererAdapterYosterCloudTra20p12(const DObj *joint,
                                                  s32 out[3])
{
    const f32 *v = &joint->translate.vec.f.x;
    u32 c;

    for (c = 0u; c < 3u; c++)
    {
        union
        {
            f32 f;
            u32 u;
        } bits = { v[c] };
        s32 e;

        /* |v| < 2^14: FTOFIX32 stays inside an s32 with no saturation. */
        if ((((bits.u >> 23) & 0xffu) >= (127u + 14u)) ||
            (ndsRendererAdapterFloatPow2ToS32(v[c], 16u, &e) == FALSE))
        {
            return FALSE;
        }
        /* ndsRendererMtxLoadN64ToDS20p12's round shift by 4: half away from
         * zero. */
        out[c] = (e < 0) ? -((-e + 8) >> 4) : ((e + 8) >> 4);
    }
    return TRUE;
}

sb32 ndsRendererAdapterSubmitNativeYosterCloud(void *root_ptr, void *cobj,
    u32 initial_geometry_mode, NDSRendererStats *stats)
{
    DObj *root = (DObj *)root_ptr;
    DObj *mids[3];
    DObj *draws[3];
    NDSRendererAdapterNativeYosterCloudWorkspace *workspace =
        &sNdsRendererAdapterNativeYosterCloudWorkspace;
    NDSRelocLoadedFile *loaded = NULL;
    NDSRelocLoadedFile *loaded0 = NULL;
    u32 alphas[3];
    u32 i;
    const void *live_data = NULL;
    u32 live_size = 0u;
    sb32 view_ok;
    s32 tra[7][3];
    sb32 fast;

    if ((root == NULL) || (stats == NULL))
    {
        gNdsNativeYosterCloudFailStep = 1u;
        return FALSE;
    }
    /* Template shape: root with exactly 3 mid children, each with exactly
     * one drawable child; drawables are leaves. */
    if ((root->sib_next != NULL) || (root->child == NULL) ||
        (root->dv != NULL) || (root->mobj != NULL))
    {
        gNdsNativeYosterCloudFailStep = 2u;
        return FALSE;
    }
    mids[0] = root->child;
    for (i = 0u; i < 3u; i++)
    {
        DObj *mid = (i == 0u) ? mids[0] : mids[i - 1u]->sib_next;
        mids[i] = mid;
        if ((mid == NULL) || (mid->child == NULL) ||
            (mid->child->sib_next != NULL) || (mid->child->child != NULL) ||
            (mid->dv != NULL) || (mid->mobj != NULL))
        {
            gNdsNativeYosterCloudFailStep = 3u;
            return FALSE;
        }
        draws[i] = mid->child;
        if ((i == 2u) && (mid->sib_next != NULL))
        {
            gNdsNativeYosterCloudFailStep = 4u;
            return FALSE;
        }
    }
    /* The live view of bank 154 and the file holding a drawable's list are
     * pure queries: asked once, not once a drawable (the three share the
     * list). */
    view_ok = ndsRelocGetLoadedAssetView(154u, &live_data, &live_size);
    for (i = 0u; i < 3u; i++)
    {
        /* Every drawable executes the shared bank-154 DL with its own MObj. */
        if ((i == 0u) || (draws[i]->dv != draws[0]->dv))
        {
            loaded = ndsRelocFindLoadedFileContaining(draws[i]->dv,
                                                      sizeof(Gfx));
        }
        if (i == 0u)
        {
            loaded0 = loaded;
        }
        if ((loaded == NULL) || (loaded->asset_id != 154u) ||
            ((view_ok == FALSE) || (live_data != loaded->data)) ||
            (draws[i]->dv != (void *)((u8 *)loaded->data + 0x0580u)) ||
            (draws[i]->mobj == NULL))
        {
            gNdsNativeYosterCloudFailStep = 5u;
            return FALSE;
        }
        alphas[i] = (u32)draws[i]->mobj->sub.primcolor.s.a;
    }
    workspace->hierarchy_joints[0] = root;
    for (i = 0u; i < 3u; i++)
    {
        workspace->hierarchy_joints[1u + i] = mids[i];
        workspace->hierarchy_joints[4u + i] = draws[i];
    }
    for (i = 0u; i < 7u; i++)
    {
        DObj *joint = workspace->hierarchy_joints[i];
        u32 want_num = (i < 4u) ? 1u : 2u;
        u32 x;

        workspace->hierarchy_parents[i] =
            (i == 0u) ? 31u : ((i < 4u) ? 0u : (u8)(i - 3u));
        workspace->hierarchy_bindings[i] = (u8)i;
        if (joint == NULL)
        {
            gNdsNativeYosterCloudFailStep = 6u;
            return FALSE;
        }
        if ((i == 0u) ? (joint->parent != DOBJ_PARENT_NULL) :
            (joint->parent != workspace->hierarchy_joints[
                workspace->hierarchy_parents[i]]))
        {
            gNdsNativeYosterCloudFailStep = 7u;
            return FALSE;
        }
        if (joint->xobjs_num != want_num)
        {
            gNdsNativeYosterCloudFailStep = 8u;
            return FALSE;
        }
        for (x = 0u; x < want_num; x++)
        {
            XObj *xobj = joint->xobjs[x];
            u32 want = (x == 0u) ?
                NDS_NATIVE_ACTOR_YOSTER_CLOUD_XOBJ_KIND0 :
                NDS_NATIVE_ACTOR_YOSTER_CLOUD_XOBJ_KIND1;
            if ((xobj == NULL) || (xobj->kind != want))
            {
                gNdsNativeYosterCloudFailStep = 9u;
                return FALSE;
            }
        }
    }
    /* A fully evaporated cloud (all three prim alphas 0) draws nothing:
     * ndsRendererSubmitNativeYosterCloud returns TRUE before emitting. The
     * matrices below only feed that emit, and every local here is a
     * Tra/kind-48 build, which cannot refuse; so the same TRUE comes back
     * without building them (P2-2p8, 2026-09-28). */
    if (((alphas[0] | alphas[1] | alphas[2]) == 0u) && (cobj != NULL))
    {
        gNdsNativeYosterCloudFailStep = 0u;
        return TRUE;
    }
    fast = TRUE;
    for (i = 0u; (fast != FALSE) && (i < 7u); i++)
    {
        fast = ndsRendererAdapterYosterCloudTra20p12(
            workspace->hierarchy_joints[i], tra[i]);
    }
    for (i = 0u; i < 7u; i++)
    {
        DObj *joint = workspace->hierarchy_joints[i];
        NDSRendererMatrix20p12 *local =
            &workspace->hierarchy_storage.hierarchy_locals[i];

        if (fast != FALSE)
        {
            ndsRendererAdapterMtxIdentity20p12(local);
            local->m[3][0] = tra[i][0];
            local->m[3][1] = tra[i][1];
            local->m[3][2] = tra[i][2];
        }
        else if ((ndsRendererAdapterBuildDObjLocalMatrix(joint, local) ==
                  FALSE) ||
                 (ndsRendererAdapterMatrixIsAffine20p12(local) == FALSE))
        {
            gNdsNativeYosterCloudFailStep = 10u;
            return FALSE;
        }
    }
    if (ndsRendererAdapterGetHierarchyCameraMatrices(cobj,
            &workspace->hierarchy_projection,
            &workspace->hierarchy_camera_modelview) == FALSE)
    {
        gNdsNativeYosterCloudFailStep = 11u;
        return FALSE;
    }
    workspace->hierarchy.projection = &workspace->hierarchy_projection;
    workspace->hierarchy.camera_modelview = &workspace->hierarchy_camera_modelview;
    workspace->hierarchy.joint_locals = workspace->hierarchy_storage.hierarchy_locals;
    workspace->hierarchy.joint_parents = workspace->hierarchy_parents;
    workspace->hierarchy.joint_bindings = workspace->hierarchy_bindings;
    workspace->hierarchy.joint_count =
        NDS_NATIVE_ACTOR_YOSTER_CLOUD_JOINT_COUNT;
    for (i = 0u; i < 3u; i++)
    {
        NDSRendererMatrix20p12 *mvp = &workspace->drawable_mvps[i];
        const NDSRendererMatrix20p12 *modelview_ptr = mvp;
        const NDSRendererMatrix20p12 *projection_ptr =
            &workspace->hierarchy_projection;

        if (fast != FALSE)
        {
            /* |each| < 2^26 (20.12 of < 2^14 units): the sums cannot clamp,
             * so the chain's row 3 is (sum, 1) and rows 0-2 are the camera
             * modelview's. */
            NDSRendererMatrix20p12 chain;
            u32 c;

            for (c = 0u; c < 3u; c++)
            {
                chain.m[3][c] = tra[4u + i][c] + tra[1u + i][c] + tra[0][c];
                mvp->m[0][c] = workspace->hierarchy_camera_modelview.m[0][c];
                mvp->m[1][c] = workspace->hierarchy_camera_modelview.m[1][c];
                mvp->m[2][c] = workspace->hierarchy_camera_modelview.m[2][c];
            }
            chain.m[3][3] = 1 << 12;
            mvp->m[0][3] = workspace->hierarchy_camera_modelview.m[0][3];
            mvp->m[1][3] = workspace->hierarchy_camera_modelview.m[1][3];
            mvp->m[2][3] = workspace->hierarchy_camera_modelview.m[2][3];
            ndsRendererMtxMulRow3_20p12(
                &chain, &workspace->hierarchy_camera_modelview, mvp);
        }
        else
        {
            ndsRendererMtxMul20p12(
                &workspace->hierarchy_storage.hierarchy_locals[4u + i],
                &workspace->hierarchy_storage.hierarchy_locals[1u + i], mvp);
            ndsRendererMtxMul20p12(mvp,
                &workspace->hierarchy_storage.hierarchy_locals[0], mvp);
            ndsRendererMtxMul20p12(mvp,
                &workspace->hierarchy_camera_modelview, mvp);
        }
        /* BuildDObjLocalMatrix deliberately skips kind 48. The source then
         * replaces MVP orientation while retaining the composed translation;
         * reuse that owning seam for each drawable's live scale and camera. */
        sNdsRendererAdapterMvpRecalcScaleX = 1.0F;
        ndsRendererAdapterApplyMvpRecalc(draws[i], nGCMatrixKind48, cobj,
            &workspace->hierarchy_projection, &projection_ptr,
            mvp, &modelview_ptr,
            ndsRendererAdapterSharedMvpCamera((CObj *)cobj));
        if ((modelview_ptr == NULL) || (projection_ptr != NULL))
        {
            gNdsNativeYosterCloudFailStep = 12u;
            return FALSE;
        }
    }
    loaded = loaded0;
    gNdsNativeYosterCloudFailStep = 0u;
    return ndsRendererSubmitNativeYosterCloud(loaded->data, loaded->data_size,
        &workspace->hierarchy, workspace->drawable_mvps,
        alphas[0], alphas[1], alphas[2],
        initial_geometry_mode, stats);
}
#endif

static u32 ndsRendererAdapterNormalizeNativeGeometryMode(u32 geometry_mode)
{
    const u32 legacy_cull_front = 0x00001000u;
    const u32 legacy_cull_back = G_CULL_BACK;
    u32 legacy = geometry_mode &
        (legacy_cull_front | legacy_cull_back);

    if (legacy != 0u)
    {
        geometry_mode &= ~(legacy_cull_front | legacy_cull_back);
        if ((legacy & legacy_cull_front) != 0u)
        {
            geometry_mode |= NDS_RENDERER_GEOM_CULL_FRONT;
        }
        if ((legacy & legacy_cull_back) != 0u)
        {
            geometry_mode |= NDS_RENDERER_GEOM_CULL_BACK;
        }
    }
    return geometry_mode;
}

/* Memory syTaskmanMalloc can hand out: the taskman arena, or in a VS battle
 * the frontend overlay's loan, which it takes first (2026-10-18). */
static s32 ndsFighterDLScanRangeInTaskmanArena(const void *ptr, size_t bytes)
{
    const u8 *arena = ndsTaskmanArenaStart();
    uintptr_t base = (uintptr_t)arena;
    uintptr_t addr = (uintptr_t)ptr;
    size_t arena_size = ndsTaskmanArenaSize();

    if (ptr == NULL)
    {
        return FALSE;
    }
    if ((arena == NULL) || (addr < base) || (addr > (base + arena_size)))
    {
        return ndsFrontendOverlayLoanContains(ptr, bytes);
    }
    return (bytes <= (arena_size - (size_t)(addr - base))) ? TRUE : FALSE;
}

static size_t ndsRendererAdapterImmutableCommandSpan(const Gfx *dl,
                                                      void *user)
{
    NDSRelocLoadedFile *loaded;
    uintptr_t base;
    uintptr_t addr;

    (void)user;
    loaded = ndsRelocFindLoadedFileContaining(dl, sizeof(*dl));
    if (loaded == NULL)
    {
        return 0u;
    }
    base = (uintptr_t)loaded->data;
    addr = (uintptr_t)dl;
    return loaded->data_size - (size_t)(addr - base);
}
