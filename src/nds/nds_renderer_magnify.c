/* Native renderer, the off-screen magnifier's frame (owner 2026-10-22: "the
 * fighter off screen camera circle views at the edge of the screen"): for each
 * magnified fighter, the bubble the source draws with two texture rectangles
 * -- ifCommonPlayerMagnifyUpdateRender's IFCommonPlayerMagnifyFrameImage,
 * filled with the stage's fog colour and soft-rimmed in the player's colour --
 * and ifCommonPlayerMagnifyProcDisplay's arrow toward the fighter. The fighter
 * inside is a lean replay (renderer_fighter_magnify.c, scene backend). A
 * slice of nds_renderer.c.
 *
 * Untextured GX polygons in screen space, drawn after the frame's fighter
 * packets: identity matrices and NDC vertices, as the IFCommon cloud quads
 * (ndsRendererHardwareEmitIFCommonClouds). Depth places the frame in front of
 * the scene (whose depths sit above z 0.4) and behind the magnified fighter
 * (z -0.91..-0.33, the replay's projection); the arrow is in front of both. */

/* cos/sin of k * 22.5 degrees at Q12, the disc's 16 rim directions. */
static const s16 sNdsRendererMagnifyCircle[16][2] = {
    { 4096, 0 }, { 3784, 1567 }, { 2896, 2896 }, { 1567, 3784 },
    { 0, 4096 }, { -1567, 3784 }, { -2896, 2896 }, { -3784, 1567 },
    { -4096, 0 }, { -3784, -1567 }, { -2896, -2896 }, { -1567, -3784 },
    { 0, -4096 }, { 1567, -3784 }, { 2896, -2896 }, { 3784, -1567 },
};

#define NDS_RENDERER_MAGNIFY_FRAME_Z ((v16)-819)   /* z -0.2 */
#define NDS_RENDERER_MAGNIFY_ARROW_Z ((v16)-3891)  /* z -0.95 */
#define NDS_RENDERER_MAGNIFY_RIM_ALPHA 16u
#define NDS_RENDERER_MAGNIFY_POLY_ID 61u
#define NDS_RENDERER_MAGNIFY_RIM_POLY_ID 62u

/* The magnified fighter's viewport: screen pixels x0..x1, y0..y1 (y down),
 * after the previous fighter's packet has left the FIFO. */
void ndsRendererMagnifySetViewport(s32 x0, s32 y0, s32 x1, s32 y1)
{
    NDS_FIGHTER_PACKET_DMA_WAIT();
    glViewport((u8)x0, (u8)(SCREEN_HEIGHT - 1 - y1), (u8)x1,
               (u8)(SCREEN_HEIGHT - 1 - y0));
}

/* A DS screen pixel at Q12 -> the NDC vertex unit (4096 = 1.0). */
static v16 ndsRendererMagnifyX(s32 x_q12)
{
    s32 v = ((x_q12 * 32) >> 12) - 4096;

    return (v16)((v < -32768) ? -32768 : ((v > 32767) ? 32767 : v));
}

static v16 ndsRendererMagnifyY(s32 y_q12)
{
    s32 v = 4096 - (s32)(((s64)y_q12 * 128) / (3 * 4096));

    return (v16)((v < -32768) ? -32768 : ((v > 32767) ? 32767 : v));
}

static void ndsRendererMagnifyRimPoint(const NDSRendererMagnifyFrame *f, u32 k,
                                       s32 rx_q12, s32 ry_q12, v16 *x, v16 *y)
{
    const s16 *c = sNdsRendererMagnifyCircle[k & 15u];

    *x = ndsRendererMagnifyX(f->x_q12 + (s32)(((s64)rx_q12 * c[0]) >> 12));
    *y = ndsRendererMagnifyY(f->y_q12 - (s32)(((s64)ry_q12 * c[1]) >> 12));
}

void ndsRendererDrawMagnifyFrames(const NDSRendererMagnifyFrame *frames,
                                  u32 count)
{
    u32 i;
    u32 k;

    if ((frames == NULL) || (count == 0u))
    {
        return;
    }
    NDS_FIGHTER_PACKET_DMA_WAIT();
    ndsRendererHardwareEndBatch();
    /* The packets wrote POLY_FMT and the texture words behind the shadow. */
    ndsRendererHardwareInvalidateGXState(NDS_RENDERER_GX_STATE_ALL);
    ndsRendererHardwareSetMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    ndsRendererHardwareSetMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glBindTexture(GL_TEXTURE_2D, 0);
    for (i = 0u; i < count; i++)
    {
        const NDSRendererMagnifyFrame *f = &frames[i];
        v16 cx = ndsRendererMagnifyX(f->x_q12);
        v16 cy = ndsRendererMagnifyY(f->y_q12);
        v16 x0, y0, x1, y1, x2, y2;

        /* The fill: eight convex quads fanned from the centre. */
        ndsRendererHardwareSetPolyFmt(POLY_ALPHA(31) | POLY_CULL_NONE |
                                      POLY_ID(NDS_RENDERER_MAGNIFY_POLY_ID));
        glColor(f->fill_rgb15);
        glBegin(GL_QUAD);
        for (k = 0u; k < 16u; k += 2u)
        {
            ndsRendererMagnifyRimPoint(f, k, f->radius_x_q12, f->radius_y_q12, &x0, &y0);
            ndsRendererMagnifyRimPoint(f, k + 1u, f->radius_x_q12, f->radius_y_q12, &x1, &y1);
            ndsRendererMagnifyRimPoint(f, k + 2u, f->radius_x_q12, f->radius_y_q12, &x2, &y2);
            glVertex3v16(cx, cy, NDS_RENDERER_MAGNIFY_FRAME_Z);
            glVertex3v16(x0, y0, NDS_RENDERER_MAGNIFY_FRAME_Z);
            glVertex3v16(x1, y1, NDS_RENDERER_MAGNIFY_FRAME_Z);
            glVertex3v16(x2, y2, NDS_RENDERER_MAGNIFY_FRAME_Z);
        }
        glEnd();
        /* The rim: a translucent band in the player's colour, the source's
         * soft alpha edge (G_RM_AA_XLU_SURF over BLENDPEDECALA). */
        ndsRendererHardwareSetPolyFmt(POLY_ALPHA(NDS_RENDERER_MAGNIFY_RIM_ALPHA) |
                                      POLY_CULL_NONE |
                                      POLY_ID(NDS_RENDERER_MAGNIFY_RIM_POLY_ID));
        glColor(f->rim_rgb15);
        glBegin(GL_QUAD_STRIP);
        for (k = 0u; k <= 16u; k++)
        {
            ndsRendererMagnifyRimPoint(f, k, f->radius_x_q12, f->radius_y_q12, &x0, &y0);
            ndsRendererMagnifyRimPoint(f, k, f->rim_x_q12, f->rim_y_q12, &x1, &y1);
            glVertex3v16(x0, y0, NDS_RENDERER_MAGNIFY_FRAME_Z);
            glVertex3v16(x1, y1, NDS_RENDERER_MAGNIFY_FRAME_Z);
        }
        glEnd();
        /* The arrow toward the fighter, opaque in the player's colour. */
        ndsRendererHardwareSetPolyFmt(POLY_ALPHA(31) | POLY_CULL_NONE |
                                      POLY_ID(NDS_RENDERER_MAGNIFY_POLY_ID));
        glColor(f->arrow_rgb15);
        glBegin(GL_TRIANGLE);
        for (k = 0u; k < 3u; k++)
        {
            glVertex3v16(ndsRendererMagnifyX(f->arrow_q12[k][0]),
                         ndsRendererMagnifyY(f->arrow_q12[k][1]),
                         NDS_RENDERER_MAGNIFY_ARROW_Z);
        }
        glEnd();
    }
    glColor(RGB15(31, 31, 31));
    ndsRendererHardwareInvalidateGXState(NDS_RENDERER_GX_STATE_ALL);
    sNdsRendererHardwareBoundTextureName = 0u;
    sNdsRendererHardwareActiveTextureEntry = NULL;
    sNdsRendererHardwareMatrixLoaded = FALSE;
    sNdsRendererHardwareMatrixMode = NDS_RENDERER_HW_MATRIX_MODE_NONE;
    sNdsRendererHardwareMatrixGeneration = 0u;
    sNdsRendererHardwareSubmitted = TRUE;
}
