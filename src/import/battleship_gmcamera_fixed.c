/* The battle camera's interest box in fixed point (P2-2p8, 2026-10-05; owner:
 * "Software floating point should not exist, fixed point only"; ruling D13
 * re-baselines the digest).
 *
 * gm/gmcamera.c's gmCameraUpdateInterests runs every tick: per fighter three
 * zoom products and four bounds of `x -/+ 700 or 1000 * zoom`, per followed
 * weapon four more, then the min/max box and its centre -- ~150 soft-float
 * calls a frame in the float census (artifacts/performance/2026-10-05_float-
 * census, every roster). battleship_gmcamera.c makes the decomp definition
 * weak; this is the strong one. Positions enter at Q12 once (the bounds clamps
 * are the source's own float calls, which move nothing on the way), the zoom
 * product is Q16, the box is integers, and only the four results leave as
 * floats. Every branch, clamp and write is the source's, in its order. */
#include <ft/fighter.h>
#include <gm/generic.h>
#include <gr/ground.h>
#include <wp/weapon.h>
#include <sc/scene.h>
#include <sys/obj.h>
#include <stdint.h>

#include <nds/nds_r2_collision_mtx.h>
#include <nds/nds_r2_hwmath_unit.h>
#include <nds/nds_native_wallpaper.h>

#ifndef CObjGetStruct
#define CObjGetStruct(gobj) ((CObj *)((gobj)->obj))
#endif

extern f32 dGMCameraPlayerZoomRanges[];
extern f32 gGMCameraPauseCameraEyeX;
extern f32 gGMCameraPauseCameraEyeY;
extern u16 gSYSinTable[0x800];
f32 gmCameraGetPlayerNumZoomRange(s32 players_num);
void scManagerRunPrintGObjStatus(void);

/* Q12 and Q16 scales of the camera arithmetic. */
#define NDS_CAM_Q 12
#define NDS_CAM_ZQ 16
#define NDS_CAM_ONE_Q (INT32_C(1) << NDS_CAM_Q)
/* +-65536 units, the source's empty-box sentinels; also the saturation of a
 * position, far past every stage's blast zone. */
#define NDS_CAM_LIMIT_Q (INT32_C(65536) << NDS_CAM_Q)

/* A float at `bits` fraction bits, saturated (one ARM copy, called a few
 * times a fighter). */
static int32_t __attribute__((noinline, target("arm")))
ndsCamToFixed(f32 value, unsigned int bits, int32_t limit)
{
    int32_t q = ndsR2CollisionF32ToFixed(value, bits);

    if ((q == NDS_R2_COLLISION_F32_OVERFLOW) || (q >= limit) || (q <= -limit))
    {
        uint32_t sign;

        __builtin_memcpy(&sign, &value, sizeof(sign));
        return ((sign & 0x80000000u) != 0u) ? -limit : limit;
    }
    return q;
}

static inline int32_t ndsCamQ(f32 value)
{
    return ndsCamToFixed(value, NDS_CAM_Q, NDS_CAM_LIMIT_Q);
}

/* units * zoom, Q12, from a Q16 zoom. 32-bit: units <= 1000 and a zoom
 * below 2^18 (4x) keep the product under 2^28 (a 64-bit product is a libgcc
 * call in this Thumb TU). */
static inline int32_t ndsCamUnitsTimes(int32_t units, int32_t zoom_q16)
{
    return (units * zoom_q16) >> (NDS_CAM_ZQ - NDS_CAM_Q);
}

/* gmCameraCalcFighterZoomRange at Q16: the three products the source takes
 * one at a time, each rounded to Q16 from Q12 factors (32-bit: factors under
 * 8x keep each product under 2^30). */
static int32_t ndsCamFighterZoomQ16(FTStruct *fp, int32_t zoom_q16)
{
    const int32_t limit = INT32_C(1) << 19;    /* 8.0 at Q16 */
    int32_t z = (((zoom_q16 >> 4) *
                  (ndsCamToFixed(fp->camera_zoom_frame, NDS_CAM_ZQ, limit) >> 4)) +
                 (1 << 7)) >> 8;

    z = (((z >> 4) *
          (ndsCamToFixed(fp->camera_zoom_range, NDS_CAM_ZQ, limit) >> 4)) +
         (1 << 7)) >> 8;
    if ((fp->status_id == nFTCommonStatusWait) && (fp->status_total_tics >= 120))
    {
        z = (z * 3) >> 2;
    }
    return z;
}

/* gmCameraGetTargetAtY on a Q12 distance, as the Q16 factor
 * 0.5 - target_at_y. */
static int32_t ndsCamAtYFactorQ16(int32_t dist_q)
{
    /* 0.0682 at Q16 (4,469.6), rounded. */
    const int32_t k_q16 = 4470;
    int32_t t_q16;

    if (dist_q > (2000 * NDS_CAM_ONE_Q))
    {
        return (1 << 15) - k_q16;
    }
    if (dist_q < (1000 * NDS_CAM_ONE_Q))
    {
        return 1 << 15;
    }
    /* (dist - 1000) / 1000, Q16. */
    t_q16 = (int32_t)ndsR2HwMathDivideFast(
        (int64_t)(dist_q - (1000 * NDS_CAM_ONE_Q)) << NDS_CAM_ZQ,
        1000 * NDS_CAM_ONE_Q);
    return (1 << 15) - (int32_t)(((int64_t)t_q16 * k_q16) >> NDS_CAM_ZQ);
}

/* gmCameraSetBoundsPosition (gmcamera.c:101) and the team form (:155) on a
 * Q12 position (2026-10-06). The bounds are the ground data's s16s, so each
 * test is an integer compare; the float forms converted the bound on every
 * compare (~1.1K ticks a frame of __aeabi_i2f and compares). The source's
 * loop fixes one axis a pass until no bound is crossed: a clamp of x, then
 * of y. */
static void ndsCamClampQ(int32_t *x, int32_t *y, s32 left, s32 right,
                         s32 bottom, s32 top)
{
    const int32_t l = (int32_t)left << NDS_CAM_Q;
    const int32_t r = (int32_t)right << NDS_CAM_Q;
    const int32_t b = (int32_t)bottom << NDS_CAM_Q;
    const int32_t t = (int32_t)top << NDS_CAM_Q;

    if (*x < l)
    {
        *x = l;
    }
    else if (*x > r)
    {
        *x = r;
    }
    if (*y < b)
    {
        *y = b;
    }
    else if (*y > t)
    {
        *y = t;
    }
}

static inline void ndsCamBoundsQ(int32_t *x, int32_t *y)
{
    ndsCamClampQ(x, y, gMPCollisionGroundData->camera_bound_left,
                 gMPCollisionGroundData->camera_bound_right,
                 gMPCollisionGroundData->camera_bound_bottom,
                 gMPCollisionGroundData->camera_bound_top);
}

static inline void ndsCamTeamBoundsQ(int32_t *x, int32_t *y)
{
    ndsCamClampQ(x, y, gMPCollisionGroundData->camera_bound_team_left,
                 gMPCollisionGroundData->camera_bound_team_right,
                 gMPCollisionGroundData->camera_bound_team_bottom,
                 gMPCollisionGroundData->camera_bound_team_top);
}

/* gmCameraSetDeadUpStarPosition (gmcamera.c:185): x scaled by
 * camera top / map top, y on the camera top. */
static void ndsCamDeadUpStarQ(int32_t *x, int32_t *y)
{
    const s32 top = gMPCollisionGroundData->camera_bound_top;
    const s32 map_top = gMPCollisionGroundData->map_bound_top;

    if (map_top != 0)
    {
        *x = (int32_t)ndsR2HwMathDivideFast((int64_t)*x * top, map_top);
    }
    *y = (int32_t)top << NDS_CAM_Q;
}

/* gmcamera.c:231 in Q12: the box centre (x, y) and the two half extents.
 * ARM state (2026-10-06): its 64-bit products were __aeabi_lmul calls in
 * Thumb. */
static void __attribute__((noinline, target("arm")))
ndsCamInterestsQ(int32_t vec_q[2], int32_t *hz_out, int32_t *vt_out)
{
    s32 players_num;
    s32 i;
    FTCamera cams[GMCOMMON_PLAYERS_MAX];
    int32_t cam_x[GMCOMMON_PLAYERS_MAX];
    int32_t cam_y[GMCOMMON_PLAYERS_MAX];
    FTStruct *fp;
    GObj *fighter_gobj;

    fighter_gobj = gGCCommonLinks[nGCCommonLinkIDFighter];

    players_num = 0;

    while (fighter_gobj != NULL)
    {
        fp = ftGetStruct(fighter_gobj);

        switch (fp->camera_mode)
        {
        default:
            if (players_num >= ARRAY_COUNT(cams))
            {
                while (TRUE)
                {
                    syDebugPrintf("Player Num is Over for Camera!\n");
                    scManagerRunPrintGObjStatus();
                }
            }
            cams[players_num].target_fp = fp;

            switch (fp->camera_mode)
            {
            default:
                cams[players_num].target_pos =
                    DObjGetStruct(fighter_gobj)->translate.vec.f;
                break;

            case nFTCameraModeEntry:
            case nFTCameraModeExplain:
                cams[players_num].target_pos = fp->entry_pos;
                break;

            case nFTCameraModeDeadUp:
                cams[players_num].target_pos =
                    fp->status_vars.common.dead.pos;
                break;
            }
            cams[players_num].target_pos.y += fp->attr->cam_offset_y;
            cam_x[players_num] = ndsCamQ(cams[players_num].target_pos.x);
            cam_y[players_num] = ndsCamQ(cams[players_num].target_pos.y);

            if ((gSCManagerBattleState->game_type == nSCBattleGameType1PGame) &&
                (gSCManagerBattleState->players[fp->player].is_spgame_enemy !=
                 FALSE))
            {
                ndsCamTeamBoundsQ(&cam_x[players_num], &cam_y[players_num]);
            }
            else switch (fp->camera_mode)
            {
            case nFTCameraModeDeadUp:
                ndsCamDeadUpStarQ(&cam_x[players_num], &cam_y[players_num]);
                break;

            default:
                ndsCamBoundsQ(&cam_x[players_num], &cam_y[players_num]);
                break;
            }
            players_num++;
            break;

        case nFTCameraModeGhost:
            break;
        }
        fighter_gobj = fighter_gobj->link_next;
    }
    if (players_num != 0)
    {
        int32_t ft_top = NDS_CAM_LIMIT_Q;
        int32_t ft_bottom = -NDS_CAM_LIMIT_Q;
        int32_t ft_left = -NDS_CAM_LIMIT_Q;
        int32_t ft_right = NDS_CAM_LIMIT_Q;
        int32_t gm_bottom = NDS_CAM_LIMIT_Q;
        int32_t gm_top = -NDS_CAM_LIMIT_Q;
        int32_t gm_left = NDS_CAM_LIMIT_Q;
        int32_t gm_right = -NDS_CAM_LIMIT_Q;
        const int32_t zoom_q16 = ndsCamToFixed(
            gmCameraGetPlayerNumZoomRange(players_num), NDS_CAM_ZQ,
            INT32_C(1) << 30);
        GObj *weapon_gobj;

        for (i = 0; i < players_num; i++)
        {
            FTStruct *cam_fp = cams[i].target_fp;
            const int32_t adjust = ndsCamFighterZoomQ16(cam_fp, zoom_q16);
            const int32_t x = cam_x[i];
            const int32_t y = cam_y[i];
            const int32_t reach_near = ndsCamUnitsTimes(700, adjust);
            const int32_t reach_far = ndsCamUnitsTimes(1000, adjust);
            s32 lr = ((cam_fp->camera_mode == nFTCameraModeEntry) ||
                      (cam_fp->camera_mode == nFTCameraModeExplain)) ?
                cam_fp->status_vars.common.entry.lr : cam_fp->lr;
            int32_t pos_left;
            int32_t pos_right;
            int32_t pos_top;
            int32_t pos_bottom;

            if (lr == -1)
            {
                pos_left = x - reach_far;
                pos_right = x + reach_near;
            }
            else
            {
                pos_left = x - reach_near;
                pos_right = x + reach_far;
            }
            if (gm_left > pos_left)
            {
                gm_left = pos_left;
            }
            if (gm_right < pos_right)
            {
                gm_right = pos_right;
            }
            pos_top = y + reach_near;
            pos_bottom = y - reach_near;

            if (gm_bottom > pos_bottom)
            {
                gm_bottom = pos_bottom;
            }
            if (gm_top < pos_top)
            {
                gm_top = pos_top;
            }
            if (x < ft_right)
            {
                ft_right = x;
            }
            if (x > ft_left)
            {
                ft_left = x;
            }
            if (y < ft_top)
            {
                ft_top = y;
            }
            if (y > ft_bottom)
            {
                ft_bottom = y;
            }
        }
        weapon_gobj = gGCCommonLinks[nGCCommonLinkIDWeapon];

        while (weapon_gobj != NULL)
        {
            WPStruct *wp = wpGetStruct(weapon_gobj);

            if (wp->is_camera_follow)
            {
                const Vec3f *weapon_pos =
                    &DObjGetStruct(weapon_gobj)->translate.vec.f;
                const int32_t wp_left = ft_right - (1000 * NDS_CAM_ONE_Q);
                const int32_t wp_right = ft_left + (1000 * NDS_CAM_ONE_Q);
                const int32_t wp_bottom = ft_top - (1000 * NDS_CAM_ONE_Q);
                const int32_t wp_top = ft_bottom + (1000 * NDS_CAM_ONE_Q);
                int32_t x = ndsCamQ(weapon_pos->x);
                int32_t y = ndsCamQ(weapon_pos->y);

                ndsCamBoundsQ(&x, &y);

                if (x < wp_left)
                {
                    x = wp_left;
                }
                if (x > wp_right)
                {
                    x = wp_right;
                }
                if (y < wp_bottom)
                {
                    y = wp_bottom;
                }
                if (y > wp_top)
                {
                    y = wp_top;
                }
                if ((x - (1000 * NDS_CAM_ONE_Q)) < gm_left)
                {
                    gm_left = x - (1000 * NDS_CAM_ONE_Q);
                }
                if ((x + (1000 * NDS_CAM_ONE_Q)) > gm_right)
                {
                    gm_right = x + (1000 * NDS_CAM_ONE_Q);
                }
                if ((y - (1000 * NDS_CAM_ONE_Q)) < gm_bottom)
                {
                    gm_bottom = y - (1000 * NDS_CAM_ONE_Q);
                }
                if ((y + (1000 * NDS_CAM_ONE_Q)) > gm_top)
                {
                    gm_top = y + (1000 * NDS_CAM_ONE_Q);
                }
            }
            weapon_gobj = weapon_gobj->link_next;
        }
        {
            const int32_t hz_q = (int32_t)(((int64_t)gm_right - gm_left) >> 1);
            const int32_t vt_q = (int32_t)(((int64_t)gm_top - gm_bottom) >> 1);
            const int32_t factor_q16 =
                ndsCamAtYFactorQ16((vt_q < hz_q) ? hz_q : vt_q);

            *hz_out = hz_q;
            *vt_out = vt_q;
            vec_q[0] = (int32_t)(((int64_t)gm_left + gm_right) >> 1);
            vec_q[1] = (int32_t)(((int64_t)factor_q16 *
                                  ((int64_t)gm_bottom + gm_top)) >> NDS_CAM_ZQ);
        }
    }
    else
    {
        vec_q[0] = vec_q[1] = 0;

        *hz_out = *vt_out = 2000 * NDS_CAM_ONE_Q;
    }
}

/* gmcamera.c:231, for the cameras that keep the source's float callers. */
void gmCameraUpdateInterests(Vec3f *vec, f32 *hz, f32 *vt)
{
    int32_t vec_q[2];
    int32_t hz_q;
    int32_t vt_q;

    ndsCamInterestsQ(vec_q, &hz_q, &vt_q);
    *hz = ndsR2CollisionFixedToF32(hz_q, NDS_CAM_Q);
    *vt = ndsR2CollisionFixedToF32(vt_q, NDS_CAM_Q);
    vec->x = ndsR2CollisionFixedToF32(vec_q[0], NDS_CAM_Q);
    vec->y = ndsR2CollisionFixedToF32(vec_q[1], NDS_CAM_Q);
    vec->z = 0.0F;
}

/* THE DEFAULT BATTLE CAMERA IN Q12 (2026-10-06; owner: "Software floating
 * point should not exist, fixed point only").
 *
 * gmcamera.c:624's gmCameraDefaultFuncCamera and the nine helpers it calls --
 * the field-of-view ease, the clamp dimensions (two table tangents and two
 * divides), the target distance ease, the pan of `at` toward the interest
 * box, the look direction from `at` (two angles and four table sines) and the
 * ease of `eye` toward `at + dist * dir` -- took ~6K soft-float ticks a frame
 * (artifacts/performance/2026-10-06_fixed-camera). Its state stays in the
 * source's floats (the CObj's eye and at, GMCamera's target_dist, fovy and
 * vel_at), whose readers are all draw side (renderer, HUD, magnify, effect
 * facing); it enters Q12 once a tick and leaves once. The pan and the eye
 * ease are `v += (target - v) * k`, which is what the source's diff, norm,
 * scale and add compute (a zero difference adds zero there too); the tables
 * are the ones lbCommonSin/Tan index, at the same 4096 steps a turn. Every
 * branch and clamp is the source's, in its order. Render only: the camera is
 * not in the replay digest. */
#define NDS_CAM_TENTH_Q16 6554          /* 0.1 */
#define NDS_CAM_EASE_DIST_Q16 4915      /* 0.075 */
#define NDS_CAM_PAN_LOW_Q16 3277        /* 0.05 */
/* F_CLC_DTOR32(5), F_CLC_DTOR32(-7) and F_CLC_DTOR32(17.5) at Q12. */
#define NDS_CAM_LOOK_UP_Q 357
#define NDS_CAM_LOOK_DOWN_Q (-500)
#define NDS_CAM_LOOK_SIDE_Q 1251

static inline int32_t ndsCamMulQ16(int32_t value, int32_t factor_q16)
{
    return (int32_t)((((int64_t)value * factor_q16) + (1 << 15)) >> 16);
}

/* The source tables' sine at a 4096-step index, Q15. */
static inline int32_t ndsCamSinQ15(int32_t index)
{
    const uint32_t id = (uint32_t)index & 0xfffu;
    const int32_t value = (int32_t)gSYSinTable[id & 0x7ffu];

    return ((id & 0x800u) != 0u) ? -value : value;
}

/* (s32)(radians * 651.8986206F), from Q12 radians. */
static inline int32_t ndsCamAngleIndex(int32_t radians_q)
{
    return (int32_t)(((int64_t)radians_q * INT64_C(683565276)) >> 32);
}

/* gmCameraGetClampDimensionsMax (gmcamera.c:469): the extents over the
 * field of view's half tangent (and the viewport aspect), the larger one,
 * clamped to [2500, 30000]. */
static int32_t ndsCamClampDimensionsMaxQ(int32_t hz, int32_t vt, int32_t fovy)
{
    /* (s32)(F_CLC_DTOR32(fovy * 0.5F) * 651.8986206F) = fovy * 5.68889. */
    const int32_t id = (int32_t)(((int64_t)fovy * INT64_C(5965232)) >> 32);
    int32_t sn = ndsCamSinQ15(id);
    const int32_t cs = ndsCamSinQ15(id + 0x400);
    const int32_t w = gGMCameraStruct.viewport_width;
    const int32_t h = gGMCameraStruct.viewport_height;
    int32_t maxd;

    if (sn == 0)
    {
        sn = 1;
    }
    vt = (int32_t)ndsR2HwMathDivideFast((int64_t)vt * cs, sn);
    if ((w != 0) && (h != 0))
    {
        hz = (int32_t)ndsR2HwMathDivideFast((int64_t)hz * cs * h,
                                            (int64_t)sn * w);
    }
    maxd = (hz > vt) ? hz : vt;
    if (maxd < (2500 * NDS_CAM_ONE_Q))
    {
        maxd = 2500 * NDS_CAM_ONE_Q;
    }
    if (maxd > (30000 * NDS_CAM_ONE_Q))
    {
        maxd = 30000 * NDS_CAM_ONE_Q;
    }
    return maxd;
}

/* func_ovl2_8010C4D0 (gmcamera.c:541): the pan factor, Q16. */
static int32_t ndsCamPanScaleQ16(int32_t dist)
{
    if (dist > (15000 * NDS_CAM_ONE_Q))
    {
        return NDS_CAM_TENTH_Q16;
    }
    if (dist < (2000 * NDS_CAM_ONE_Q))
    {
        return NDS_CAM_PAN_LOW_Q16;
    }
    /* ((1 - (dist - 2000) / 13000) * 0.05) + 0.05 */
    return NDS_CAM_PAN_LOW_Q16 +
           (int32_t)ndsR2HwMathDivideFast(
               (int64_t)NDS_CAM_PAN_LOW_Q16 *
                   ((15000 * NDS_CAM_ONE_Q) - dist),
               13000 * NDS_CAM_ONE_Q);
}

void __attribute__((target("arm")))
gmCameraDefaultFuncCamera(GObj *camera_gobj)
{
    CObj *cobj = CObjGetStruct(camera_gobj);
    int32_t box[2];
    int32_t hz;
    int32_t vt;
    int32_t fovy;
    int32_t dist;
    int32_t pan_q16;
    int32_t at[3];
    int32_t eye[3];
    int32_t dir[3];
    int32_t angle_x;
    int32_t angle_y;
    int32_t id;
    u32 i;

    ndsCamInterestsQ(box, &hz, &vt);

    /* gmCameraAdjustFOV(38.0F) */
    fovy = ndsCamQ(gGMCameraStruct.fovy);
    fovy += ndsCamMulQ16((38 * NDS_CAM_ONE_Q) - fovy, NDS_CAM_TENTH_Q16);

    /* gmCameraGetClampDimensionsMax, then func_ovl2_8010C670 */
    {
        const int32_t maxd = ndsCamClampDimensionsMaxQ(hz, vt, fovy);
        int32_t d;

        dist = ndsCamQ(gGMCameraStruct.target_dist);
        d = dist - maxd;
        if (d <= ndsCamMulQ16(d, NDS_CAM_EASE_DIST_Q16))
        {
            dist = maxd;
        }
        else
        {
            dist -= ndsCamMulQ16(d, NDS_CAM_EASE_DIST_Q16);
        }
    }

    /* gmCameraPan(cobj, &box, func_ovl2_8010C4D0()): the box's z is 0. */
    pan_q16 = ndsCamPanScaleQ16(dist);
    at[0] = ndsCamQ(cobj->vec.at.x);
    at[1] = ndsCamQ(cobj->vec.at.y);
    at[2] = ndsCamQ(cobj->vec.at.z);
    at[0] += ndsCamMulQ16(box[0] - at[0], pan_q16);
    at[1] += ndsCamMulQ16(box[1] - at[1], pan_q16);
    at[2] += ndsCamMulQ16(-at[2], pan_q16);

    /* func_ovl2_8010C3C0: the look angles from the panned `at`,
     * -F_CLC_DTOR32(v / 133.0F) = v * -1.3120567e-4 (Q32 563,536). */
    angle_y = -(int32_t)(((int64_t)(at[1] - (900 * NDS_CAM_ONE_Q)) *
                          INT64_C(563536)) >> 32);
    if (angle_y > NDS_CAM_LOOK_UP_Q)
    {
        angle_y = NDS_CAM_LOOK_UP_Q;
    }
    if (angle_y < NDS_CAM_LOOK_DOWN_Q)
    {
        angle_y = NDS_CAM_LOOK_DOWN_Q;
    }
    angle_x = -(int32_t)(((int64_t)at[0] * INT64_C(563536)) >> 32);
    if (angle_x > NDS_CAM_LOOK_SIDE_Q)
    {
        angle_x = NDS_CAM_LOOK_SIDE_Q;
    }
    if (angle_x < -NDS_CAM_LOOK_SIDE_Q)
    {
        angle_x = -NDS_CAM_LOOK_SIDE_Q;
    }

    /* gmCameraGetAdjustAtAngle(at, &dir, x = angle_x, y = angle_y), Q15. */
    id = ndsCamAngleIndex(ndsCamQ(gGMCameraPauseCameraEyeY) + angle_y +
                          ndsCamQ(gMPCollisionGroundData->light_angle.z));
    dir[1] = -ndsCamSinQ15(id);
    dir[2] = ndsCamSinQ15(id + 0x400);
    id = ndsCamAngleIndex(ndsCamQ(gGMCameraPauseCameraEyeX) + angle_x);
    dir[0] = (ndsCamSinQ15(id) * dir[2]) >> 15;
    dir[2] = (dir[2] * ndsCamSinQ15(id + 0x400)) >> 15;

    /* func_ovl2_8010C5C0: eye eased a tenth of the way to at + dist * dir. */
    eye[0] = ndsCamQ(cobj->vec.eye.x);
    eye[1] = ndsCamQ(cobj->vec.eye.y);
    eye[2] = ndsCamQ(cobj->vec.eye.z);
    for (i = 0u; i < 3u; i++)
    {
        const int32_t pan =
            at[i] + (int32_t)(((int64_t)dist * dir[i]) >> 15);

        eye[i] += ndsCamMulQ16(pan - eye[i], NDS_CAM_TENTH_Q16);
    }

    /* gmCameraApplyVel */
    at[0] += ndsCamQ(gGMCameraStruct.vel_at.x);
    at[1] += ndsCamQ(gGMCameraStruct.vel_at.y);
    at[2] += ndsCamQ(gGMCameraStruct.vel_at.z);
    gGMCameraStruct.vel_at.x = gGMCameraStruct.vel_at.y =
        gGMCameraStruct.vel_at.z = 0.0F;

    cobj->vec.at.x = ndsR2CollisionFixedToF32(at[0], NDS_CAM_Q);
    cobj->vec.at.y = ndsR2CollisionFixedToF32(at[1], NDS_CAM_Q);
    cobj->vec.at.z = ndsR2CollisionFixedToF32(at[2], NDS_CAM_Q);
    cobj->vec.eye.x = ndsR2CollisionFixedToF32(eye[0], NDS_CAM_Q);
    cobj->vec.eye.y = ndsR2CollisionFixedToF32(eye[1], NDS_CAM_Q);
    cobj->vec.eye.z = ndsR2CollisionFixedToF32(eye[2], NDS_CAM_Q);
    gGMCameraStruct.target_dist = ndsR2CollisionFixedToF32(dist, NDS_CAM_Q);
    gGMCameraStruct.fovy = ndsR2CollisionFixedToF32(fovy, NDS_CAM_Q);

    /* gmCameraApplyFOV */
    cobj->projection.persp.fovy = gGMCameraStruct.fovy;
}

/* gr/grwallpaper.c:45 grWallpaperCalcPersp, the source SObj's position and
 * scale from the battle camera, through the integer kernel the native BG2
 * owner draws with (nds_native_wallpaper.c; battleship_grwallpaper.c makes
 * the decomp definition weak). Render only. */
void grWallpaperCalcPersp(SObj *wallpaper_sobj)
{
    CObj *cobj = CObjGetStruct(gGMCameraGObj);
    s32 pos_x;
    s32 pos_y;
    s32 scale;

    if (ndsWallpaperPerspQ(0u, &cobj->vec.eye, &cobj->vec.at, &pos_x, &pos_y,
                           &scale) == FALSE)
    {
        return;
    }
    wallpaper_sobj->sprite.scalex = wallpaper_sobj->sprite.scaley =
        ndsR2CollisionFixedToF32(scale, 16u);
    wallpaper_sobj->pos.x = ndsR2CollisionFixedToF32(pos_x, 16u);
    wallpaper_sobj->pos.y = ndsR2CollisionFixedToF32(pos_y, 16u);
}
