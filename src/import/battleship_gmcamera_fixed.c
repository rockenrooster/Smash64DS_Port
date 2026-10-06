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
#include <gr/ground.h>
#include <wp/weapon.h>
#include <sc/scene.h>
#include <sys/obj.h>
#include <stdint.h>

#include <nds/nds_r2_collision_mtx.h>
#include <nds/nds_r2_hwmath_unit.h>

extern f32 dGMCameraPlayerZoomRanges[];
void gmCameraSetBoundsPosition(Vec3f *pos);
void gmCameraSetTeamBoundsPosition(Vec3f *pos);
void gmCameraSetDeadUpStarPosition(Vec3f *pos);
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

/* gmcamera.c:231. ARM state (2026-10-06): its 64-bit products were
 * __aeabi_lmul calls in Thumb. */
void __attribute__((target("arm")))
gmCameraUpdateInterests(Vec3f *vec, f32 *hz, f32 *vt)
{
    s32 players_num;
    s32 i;
    FTCamera cams[GMCOMMON_PLAYERS_MAX];
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

            if ((gSCManagerBattleState->game_type == nSCBattleGameType1PGame) &&
                (gSCManagerBattleState->players[fp->player].is_spgame_enemy !=
                 FALSE))
            {
                gmCameraSetTeamBoundsPosition(&cams[players_num].target_pos);
            }
            else switch (fp->camera_mode)
            {
            case nFTCameraModeDeadUp:
                gmCameraSetDeadUpStarPosition(&cams[players_num].target_pos);
                break;

            default:
                gmCameraSetBoundsPosition(&cams[players_num].target_pos);
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
            const int32_t x = ndsCamQ(cams[i].target_pos.x);
            const int32_t y = ndsCamQ(cams[i].target_pos.y);
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
                Vec3f weapon_pos = DObjGetStruct(weapon_gobj)->translate.vec.f;
                const int32_t wp_left = ft_right - (1000 * NDS_CAM_ONE_Q);
                const int32_t wp_right = ft_left + (1000 * NDS_CAM_ONE_Q);
                const int32_t wp_bottom = ft_top - (1000 * NDS_CAM_ONE_Q);
                const int32_t wp_top = ft_bottom + (1000 * NDS_CAM_ONE_Q);
                int32_t x;
                int32_t y;

                gmCameraSetBoundsPosition(&weapon_pos);
                x = ndsCamQ(weapon_pos.x);
                y = ndsCamQ(weapon_pos.y);

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

            *hz = ndsR2CollisionFixedToF32(hz_q, NDS_CAM_Q);
            *vt = ndsR2CollisionFixedToF32(vt_q, NDS_CAM_Q);
            vec->x = ndsR2CollisionFixedToF32(
                ((int64_t)gm_left + gm_right) >> 1, NDS_CAM_Q);
            vec->y = ndsR2CollisionFixedToF32(
                ((int64_t)factor_q16 * ((int64_t)gm_bottom + gm_top)) >>
                    NDS_CAM_ZQ, NDS_CAM_Q);
            vec->z = 0.0F;
        }
    }
    else
    {
        vec->x = vec->y = vec->z = 0.0F;

        *hz = *vt = 2000.0F;
    }
}
