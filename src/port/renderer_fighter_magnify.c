/* The off-screen magnifier (owner 2026-10-22: "we are missing the fighter off
 * screen camera circle views at the edge of the screen. Please implement a
 * sufficiently optimized DS version"). A slice of renderer_adapter_fighter.c,
 * after the lean path.
 *
 * The source draws an off-screen fighter a second time through the magnify
 * camera (gmCameraMakePlayerMagnifyCamera): the main pass of
 * ftDisplayMainProcDisplay culls it and sets is_magnify_show; the magnify
 * pass draws ifCommonPlayerMagnifyUpdateRender's bubble at the screen edge
 * (ifCommonPlayerMagnifyGetPosition), sets an 18 x scale pixel viewport inside
 * it and draws the fighter there with its root's translation dropped
 * (TopN -> nGCMatrixKindRotRpyR) under an orthographic look at (0, 300, 0).
 * On this target a fighter is submitted once a frame, so that pass drew
 * nothing. Here, after the frame's fighters: each magnified fighter is
 * submitted again through its lean list, with four changes for the one
 * submit -- the bounds test passes, the once-a-frame guard and the afterimage
 * are skipped, and the list's projection is the magnify camera's with the
 * root translation folded in, P' = T(-root) x T(0, -300, -D) x ortho -- inside
 * the bubble's viewport; then the bubble's disc, rim and arrow
 * (ndsRendererDrawMagnifyFrames). The culled main pass did not draw the
 * fighter, so a frame draws no fighter twice and a magnified fighter costs
 * what it costs on screen.
 *
 * DS approximations: the source clips the fighter to the frame image's inner
 * disc through the Z buffer (its first texture rectangle writes the image into
 * Z); the DS clips to the viewport, so the box is the disc's inscribed square
 * (7.4 frame texels a side half, against the source's 9-texel square inside an
 * 8-texel disc); the soft-alpha rim is one translucent band. Fixed point
 * throughout; the source's float magnify_scale (gmCameraPlayerMagnifyFuncMatrix,
 * a camera matrix function this target never runs) is computed here. */

#define NDS_FIGHTER_MAGNIFY_MAX_SCALE_Q12 (3 << 12)
/* The source's ortho half extent (450 units) scaled to the inscribed box. */
#define NDS_FIGHTER_MAGNIFY_ORTHO_HALF 370
#define NDS_FIGHTER_MAGNIFY_BOX_HALF_X10 74   /* frame texels x 10 */
#define NDS_FIGHTER_MAGNIFY_FILL_X10 94
#define NDS_FIGHTER_MAGNIFY_RIM_X10 106
/* The replay's depth: the root's depth at z -0.62, 1/4096 a unit deeper
 * behind it, so +/-1,200 units stay within z -0.91..-0.33. */
#define NDS_FIGHTER_MAGNIFY_Z_CENTRE (-2540)
/* The replay shifts the projection's row 3 right by 8 (the list's eye
 * coordinates are world / 256, NDS_RENDERER_HW_WORLD_UNIT_SHIFT): an
 * orthographic matrix takes x256 throughout so its w stays 1. */
#define NDS_FIGHTER_MAGNIFY_ROW3_SCALE 256
#define NDS_FIGHTER_MAGNIFY_COLORS 5u

extern u8 dIFCommonPlayerMagnifyColorsR[];
extern u8 dIFCommonPlayerMagnifyColorsG[];
extern u8 dIFCommonPlayerMagnifyColorsB[];

volatile u32 gNdsFighterMagnifyDraws __attribute__((used));

static s32 ndsFighterMagnifyQ(f32 value, u32 frac_bits)
{
    s32 q = ndsF32ToFixed(value, frac_bits);

    return (q == INT32_MIN) ? 0 : q;
}

static s32 ndsFighterMagnifyClamp(s32 v, s32 lo, s32 hi)
{
    return (v < lo) ? lo : ((v > hi) ? hi : v);
}

/* gmCameraPlayerMagnifyFuncMatrix's scale: the screen height, in source
 * pixels, of 600 units at the main camera's distance D, over 18, at most 3:
 * 600 x P11 / w x (viewport height / 2) / 18, w = D x |P23|. */
static s32 ndsFighterMagnifyScaleQ12(CObj *camera)
{
    NDSRendererMatrix20p12 persp;
    u16 norm = 0u;
    s64 dx = ndsFighterMagnifyQ(camera->vec.eye.x, 4u) - ndsFighterMagnifyQ(camera->vec.at.x, 4u);
    s64 dy = ndsFighterMagnifyQ(camera->vec.eye.y, 4u) - ndsFighterMagnifyQ(camera->vec.at.y, 4u);
    s64 dz = ndsFighterMagnifyQ(camera->vec.eye.z, 4u) - ndsFighterMagnifyQ(camera->vec.at.z, 4u);
    s64 d_q4 = ndsR2HwMathSqrt64Fast((u64)((dx * dx) + (dy * dy) + (dz * dz)));
    s64 w_q12;
    s64 scale;

    ndsRendererAdapterCameraPerspFast(&persp, &norm, camera->projection.persp.fovy,
                                      camera->projection.persp.aspect,
                                      camera->projection.persp.near,
                                      camera->projection.persp.far,
                                      camera->projection.persp.scale);
    w_q12 = (persp.m[2][3] < 0) ? -(s64)persp.m[2][3] : (s64)persp.m[2][3];
    if ((d_q4 == 0) || (w_q12 == 0))
    {
        return NDS_FIGHTER_MAGNIFY_MAX_SCALE_Q12;
    }
    /* Q12: P11 and P23 are Q12, D is Q4 (the x16). */
    scale = ndsR2HwMathDivideFast((s64)600 * persp.m[1][1] *
                                      (gGMCameraStruct.viewport_height / 2) * 4096 * 16,
                                  d_q4 * w_q12 * 18);
    return (s32)((scale > NDS_FIGHTER_MAGNIFY_MAX_SCALE_Q12) ?
                 NDS_FIGHTER_MAGNIFY_MAX_SCALE_Q12 : ((scale < 1) ? 1 : scale));
}

/* ifCommonPlayerMagnifyGetPosition: the bubble at the screen edge on the line
 * toward the fighter's projected point (px, py at any scale; source pixels
 * from the viewport centre, y up); the result at Q12. */
static void ndsFighterMagnifyPosition(s32 px, s32 py, s32 scale_q12, s32 *x, s32 *y)
{
    const s32 margin = 20 * scale_q12;
    const s32 half_w = (gGMCameraStruct.viewport_width / 2) * 4096;
    const s32 half_h = (gGMCameraStruct.viewport_height / 2) * 4096;
    const s32 left = -half_w + margin + (5 * 4096);
    const s32 right = half_w - margin - (5 * 4096);
    const s32 up = half_h - margin;
    const s32 down = -half_h + margin;
    const s64 apx = (px < 0) ? -(s64)px : (s64)px;
    const s64 apy = (py < 0) ? -(s64)py : (s64)py;

    if (px == 0)
    {
        *y = (py > 0) ? up : down;
        *x = 0;
    }
    else if ((right > 0) && ((apy * right) > ((s64)up * apx)))
    {
        /* |py / px| > up / right: the top or bottom edge. */
        *y = (py > 0) ? up : down;
        *x = ndsFighterMagnifyClamp(
            (s32)ndsR2HwMathDivideFast((s64)*y * px, py), left, right);
    }
    else
    {
        *x = (px > 0) ? right : left;
        *y = ndsFighterMagnifyClamp(
            (s32)ndsR2HwMathDivideFast((s64)*x * py, px), down, up);
    }
}

/* Source pixels about the viewport centre (y up) at Q12 -> DS pixels (y
 * down) at Q12, the battle 3D's mapping: 300 x 220 onto 256 x 192. */
static s32 ndsFighterMagnifyScreenX(s32 x_q12)
{
    return (128 << 12) + (s32)ndsR2HwMathDivideFast((s64)x_q12 * 256,
                                                     gGMCameraStruct.viewport_width);
}

static s32 ndsFighterMagnifyScreenY(s32 y_q12)
{
    return (96 << 12) - (s32)ndsR2HwMathDivideFast((s64)y_q12 * 192,
                                                    gGMCameraStruct.viewport_height);
}

/* A length in frame texels x 10 at the magnify scale -> DS pixels at Q12. */
static s32 ndsFighterMagnifyLengthX(s32 scale_q12, s32 texels_x10)
{
    return ndsFighterMagnifyScreenX((s32)(((s64)scale_q12 * texels_x10) / 10)) - (128 << 12);
}

static s32 ndsFighterMagnifyLengthY(s32 scale_q12, s32 texels_x10)
{
    return (96 << 12) - ndsFighterMagnifyScreenY((s32)(((s64)scale_q12 * texels_x10) / 10));
}

static u16 ndsFighterMagnifyRgb15(u32 r, u32 g, u32 b)
{
    return (u16)((r >> 3) | ((g >> 3) << 5) | ((b >> 3) << 10));
}

/* The projection one magnified fighter's list loads: the camera's look at
 * (0, 300, 0) from D along z with the root's translation dropped, under the
 * box's orthographic half extent; z remapped to the front (row vectors, world
 * units, as the camera's projection). */
static void ndsFighterMagnifyProjection(const DObj *topn)
{
    const s64 k = NDS_FIGHTER_MAGNIFY_ROW3_SCALE;
    s32 tx = ndsFighterMagnifyQ(topn->translate.vec.f.x, 0u);
    s32 ty = ndsFighterMagnifyQ(topn->translate.vec.f.y, 0u);
    s32 tz = ndsFighterMagnifyQ(topn->translate.vec.f.z, 0u);
    NDSRendererMatrix20p12 *m = &sNdsFighterMagnifyProjection;

    memset(m, 0, sizeof(*m));
    m->m[0][0] = (s32)((k * 4096) / NDS_FIGHTER_MAGNIFY_ORTHO_HALF);
    m->m[1][1] = m->m[0][0];
    m->m[2][2] = (s32)-k;
    m->m[3][0] = (s32)((-(s64)tx * k * 4096) / NDS_FIGHTER_MAGNIFY_ORTHO_HALF);
    m->m[3][1] = (s32)((-((s64)ty + 300) * k * 4096) / NDS_FIGHTER_MAGNIFY_ORTHO_HALF);
    m->m[3][2] = (s32)(((s64)tz + NDS_FIGHTER_MAGNIFY_Z_CENTRE) * k);
    m->m[3][3] = (s32)(k * 4096);
}

static void ndsFighterMagnifyDrawAll(void)
{
    /* ifCommonPlayerMagnifyProcDisplay's arrow: the display list's triangle,
     * tip (0, 40) and base (+/-6, 26) in source pixels. */
    static const s8 arrow[3][2] = { { 0, 40 }, { 6, 26 }, { -6, 26 } };
    NDSRendererMagnifyFrame frames[GMCOMMON_PLAYERS_MAX];
    s32 scale = 0;
    u32 count = 0u;
    u32 slot;

    if ((gIFCommonPlayerInterface.is_magnify_display == FALSE) ||
        (gGMCameraGObj == NULL) || (gGMCameraStruct.viewport_width <= 0) ||
        (gGMCameraStruct.viewport_height <= 0))
    {
        return;
    }
    for (slot = 0u; slot < GMCOMMON_PLAYERS_MAX; slot++)
    {
        GObj *fighter_gobj = ndsFighterManagerLiveGObj(slot);
        FTStruct *fp;
        NDSRendererMagnifyFrame *f;
        s32 px, py, bx, by, cx, cy, hx, hy, ux, uy;
        s64 len;
        u32 color_id;
        u32 k;

        if (fighter_gobj == NULL)
        {
            continue;
        }
        fp = ftGetStruct(fighter_gobj);
        if ((fp == NULL) || (fp->is_magnify_show == FALSE) ||
            (fp->is_magnify_ignore != FALSE) || (fp->is_rebirth != FALSE) ||
            (fp->joints[nFTPartsJointTopN] == NULL))
        {
            continue;
        }
        if (scale == 0)
        {
            scale = ndsFighterMagnifyScaleQ12(CObjGetStruct(gGMCameraGObj));
        }
        px = ndsFighterMagnifyQ(fp->magnify_pos.x, 4u);
        py = ndsFighterMagnifyQ(fp->magnify_pos.y, 4u);
        ndsFighterMagnifyPosition(px, py, scale, &bx, &by);
        cx = ndsFighterMagnifyScreenX(bx);
        cy = ndsFighterMagnifyScreenY(by);

        /* The fighter, in the disc's inscribed square. */
        hx = ndsFighterMagnifyLengthX(scale, NDS_FIGHTER_MAGNIFY_BOX_HALF_X10);
        hy = ndsFighterMagnifyLengthY(scale, NDS_FIGHTER_MAGNIFY_BOX_HALF_X10);
        ndsFighterMagnifyProjection(fp->joints[nFTPartsJointTopN]);
        ndsRendererMagnifySetViewport(
            ndsFighterMagnifyClamp((cx - hx + 2048) >> 12, 0, 255),
            ndsFighterMagnifyClamp((cy - hy + 2048) >> 12, 0, 191),
            ndsFighterMagnifyClamp((cx + hx + 2048) >> 12, 0, 255),
            ndsFighterMagnifyClamp((cy + hy + 2048) >> 12, 0, 191));
        sNdsFighterMagnifyFp = fp;
        ndsFighterDisplayContractSubmit(fighter_gobj);
        sNdsFighterMagnifyFp = NULL;
        /* The submit's bounds test passed, so the source cleared the flag. */
        fp->is_magnify_show = TRUE;
        gNdsFighterMagnifyDraws++;

        /* The bubble's frame. */
        f = &frames[count++];
        f->x_q12 = cx;
        f->y_q12 = cy;
        f->radius_x_q12 = ndsFighterMagnifyLengthX(scale, NDS_FIGHTER_MAGNIFY_FILL_X10);
        f->radius_y_q12 = ndsFighterMagnifyLengthY(scale, NDS_FIGHTER_MAGNIFY_FILL_X10);
        f->rim_x_q12 = ndsFighterMagnifyLengthX(scale, NDS_FIGHTER_MAGNIFY_RIM_X10);
        f->rim_y_q12 = ndsFighterMagnifyLengthY(scale, NDS_FIGHTER_MAGNIFY_RIM_X10);
        color_id = (gSCManagerBattleState != NULL) ?
            (u32)gSCManagerBattleState->players[fp->player].color : 0u;
        if (color_id >= NDS_FIGHTER_MAGNIFY_COLORS)
        {
            color_id = NDS_FIGHTER_MAGNIFY_COLORS - 1u;
        }
        f->rim_rgb15 = ndsFighterMagnifyRgb15(dIFCommonPlayerMagnifyColorsR[color_id],
                                              dIFCommonPlayerMagnifyColorsG[color_id],
                                              dIFCommonPlayerMagnifyColorsB[color_id]);
        f->arrow_rgb15 = f->rim_rgb15;
        f->fill_rgb15 = (gMPCollisionGroundData != NULL) ?
            ndsFighterMagnifyRgb15(gMPCollisionGroundData->fog_color.r,
                                   gMPCollisionGroundData->fog_color.g,
                                   gMPCollisionGroundData->fog_color.b) : 0u;
        /* The arrow turned to atan2(py, px) - 90 degrees and scaled by
         * magnify_scale / 2, about the bubble's centre. */
        len = ndsR2HwMathSqrt64Fast((u64)(((s64)px * px) + ((s64)py * py)));
        ux = (len > 0) ? (s32)ndsR2HwMathDivideFast((s64)px * 4096, len) : 0;
        uy = (len > 0) ? (s32)ndsR2HwMathDivideFast((s64)py * 4096, len) : 4096;
        for (k = 0u; k < 3u; k++)
        {
            s64 mx = arrow[k][0];
            s64 my = arrow[k][1];
            s32 ox = (s32)((((mx * uy) + (my * ux)) * scale) >> 13);
            s32 oy = (s32)((((my * uy) - (mx * ux)) * scale) >> 13);

            f->arrow_q12[k][0] = ndsFighterMagnifyScreenX(bx + ox);
            f->arrow_q12[k][1] = ndsFighterMagnifyScreenY(by + oy);
        }
    }
    if (count != 0u)
    {
        ndsPlatformReset3DViewport();
        ndsRendererDrawMagnifyFrames(frames, count);
    }
}
