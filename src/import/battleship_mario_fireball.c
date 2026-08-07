#if NDS_IMPORT_BATTLESHIP_MARIO_FIREBALL

#include <common.h>
#include <ef/effect.h>
#include <ft/fighter.h>
#include <gm/gmsound.h>
#include <mp/map.h>
#include <nds/nds_startup.h>
#include <reloc_data.h>
#include <sc/scene.h>
#include <sys/audio.h>
#include <sys/taskman.h>
#include <wp/weapon.h>

#ifndef DObjGetStruct
#define DObjGetStruct(gobj) ((DObj *)((gobj)->obj))
#endif

#ifndef FTMARIO_FIREBALL_SPAWN_JOINT
#define FTMARIO_FIREBALL_SPAWN_JOINT 16
#endif

uintptr_t llMarioMainFireballWeaponAttributes;
uintptr_t llMarioSpecial1FireballWeaponAttributes;
uintptr_t llLuigiSpecial1FireballWeaponAttributes;

#if NDS_R2_FIREBALL_MAP_COLL_DEBUG
/* Live fireball collision-diamond snapshot, captured every frame from the
 * fireball's update proc and drawn as a world-space line outline from
 * lbParticleDrawTextures (which has the particle camera matrix loaded). Read
 * by the debug overlay; defined here because the fireball TU owns the capture. */
u32  gNdsFireballDebugCollActive;
f32  gNdsFireballDebugCollX;
f32  gNdsFireballDebugCollY;
f32  gNdsFireballDebugCollZ;
f32  gNdsFireballDebugCollTop;
f32  gNdsFireballDebugCollCenter;
f32  gNdsFireballDebugCollBottom;
f32  gNdsFireballDebugCollWidth;
#endif

extern f32 lbCommonMag2D(Vec3f *vec);
extern Vec3f *syVectorRotateAbout3D(Vec3f *dst, Vec3f *dir, f32 angle);
void ftMarioSpecialAirNSwitchStatusGround(GObj *fighter_gobj);
void ftMarioSpecialNSwitchStatusAir(GObj *fighter_gobj);

#if NDS_R2_FIREBALL_NATIVE_MAP_COLL
static sb32 ndsMarioFireballMapTestAll(GObj *weapon_gobj);
#define wpMapTestAll ndsMarioFireballMapTestAll
#endif
#define wpMarioFireballMakeWeapon battleship_wpMarioFireballMakeWeapon
#include "../../decomp/BattleShip-main/decomp/src/wp/wpmario/wpmariofireball.c"
#undef wpMarioFireballMakeWeapon
#if NDS_R2_FIREBALL_NATIVE_MAP_COLL
#undef wpMapTestAll
#endif

#if NDS_R2_FIREBALL_NATIVE_MAP_COLL
/*
 * DS-NATIVE DREAM LAND FIREBALL COLLISION
 * ----------------------------------------
 * The source fireball used wpMapTestAll -> mpProcessUpdateMain -> wpMapProcAll
 * every frame. Dream Land does not need that generality: relocData file 104 has
 * exactly one static collision yakumono and seven lines (four floors, one
 * ceiling, one right wall, one left wall). Bake those seven source lines here
 * in the exact world coordinates the live collision owner already uses.
 *
 * This routine owns only the map-test half. wpMarioFireballProcMap remains the
 * source function above, so rebound direction, 0.85 velocity loss, minimum
 * speed destruction, FireGrind creation, and all event timing continue through
 * wpMapCheckAllRebound unchanged. The native test publishes the same mask,
 * line-id, flags, normal, and snap state that rebound consumes.
 *
 * The fast path is deliberately Pupupu-only. Any other stage takes the real
 * wpMapTestAll fallback; this makes an unsupported stage slow, never subtly
 * wrong. Engagement/fallback counters are public diagnostics for playtest.
 */
typedef struct NDSFireballFloorLine
{
    s16 x_min;
    s16 x_max;
    s16 y;
    s16 line_id;
    u32 flags;
} NDSFireballFloorLine;

typedef struct NDSFireballWallPoint
{
    s16 x;
    s16 y;
} NDSFireballWallPoint;

typedef struct NDSFireballMapContact
{
    sb32 hit;
    f32 t;
    u16 mask;
    s16 line_id;
    u32 flags;
    Vec3f angle;
    f32 snap_value;
    u8 snap_axis; /* 0 = Y, 1 = X */
    u8 priority;  /* source wpMapProcAll order: L, R, ceil, floor */
} NDSFireballMapContact;

/* File 104, MPLineInfo group 0: lines 0..3. Floating-platform segments carry
 * MAP_VERTEX_COLL_PASS in source; the fireball accepts them because its
 * ignore_line_id is -1, exactly like mpProcessCheckTestFloorCollisionAdjNew. */
static const NDSFireballFloorLine sNdsFireballPupupuFloors[4] = {
    {  -570,   570, 1542, 0, MAP_VERTEX_COLL_PASS },
    {   951,  1892,  907, 1, MAP_VERTEX_COLL_PASS },
    { -1841,  -951,  904, 2, MAP_VERTEX_COLL_PASS },
    { -2318,  2318,    0, 3, 0x00008000u }
};

/* File 104 line 4: the underside of Dream Land's main body. */
static const NDSFireballFloorLine sNdsFireballPupupuCeil = {
    -1972, 1972, -1072, 4, 0u
};

/* File 104 line 5 (RWall) and line 6 (LWall), preserving source vertex order.
 * That order is important: mpCollisionGetLRAngle's outward normal is the
 * normalized (-dy, dx), yielding +X on the right wall and -X on the left. */
static const NDSFireballWallPoint sNdsFireballPupupuRWall[5] = {
    { 2318,     0 }, { 2307,  -124 }, { 2290,  -331 },
    { 2075,  -834 }, { 1972, -1072 }
};
static const NDSFireballWallPoint sNdsFireballPupupuLWall[5] = {
    { -1972, -1072 }, { -2075, -834 }, { -2290, -331 },
    { -2307,  -124 }, { -2318,    0 }
};

/* Normals for the four wall segments, precomputed AOT by evaluating the
 * source mpCollisionGetLRAngle + syVectorNorm3D float sequence. Keeping them
 * here removes both sqrt passes/divides from the hit event as well as all
 * generic line interpretation from the active frame. */
static const Vec3f sNdsFireballPupupuRWallNormal[4] = {
    {  0.996088386F, -0.088362679F, 0.0F },
    {  0.996644735F, -0.081850052F, 0.0F },
    {  0.919522822F, -0.393036604F, 0.0F },
    {  0.917743087F, -0.397174537F, 0.0F }
};
static const Vec3f sNdsFireballPupupuLWallNormal[4] = {
    { -0.917743087F, -0.397174537F, 0.0F },
    { -0.919522822F, -0.393036604F, 0.0F },
    { -0.996644735F, -0.081850052F, 0.0F },
    { -0.996088386F, -0.088362679F, 0.0F }
};

volatile u32 gNdsFireballNativeMapCallCount;
volatile u32 gNdsFireballNativeMapHandledCount;
volatile u32 gNdsFireballNativeMapFallbackCount;
volatile u32 gNdsFireballNativeMapCollisionCount;
volatile u32 gNdsFireballNativeMapFloorHitCount;
volatile u32 gNdsFireballNativeMapCeilHitCount;
volatile u32 gNdsFireballNativeMapLWallHitCount;
volatile u32 gNdsFireballNativeMapRWallHitCount;
volatile u32 gNdsFireballNativeMapLastMask;
volatile s32 gNdsFireballNativeMapLastLineID = -1;

static f32 ndsMarioFireballAbsF(f32 value)
{
    return (value < 0.0F) ? -value : value;
}

static void ndsMarioFireballConsiderContact(NDSFireballMapContact *best,
                                             const NDSFireballMapContact *next)
{
    const f32 tie_epsilon = 0.0001F;

    if ((best == NULL) || (next == NULL) || (next->hit == FALSE))
    {
        return;
    }
    if ((best->hit == FALSE) ||
        (next->t < (best->t - tie_epsilon)) ||
        ((ndsMarioFireballAbsF(next->t - best->t) <= tie_epsilon) &&
         (next->priority < best->priority)))
    {
        *best = *next;
    }
}

static sb32 ndsMarioFireballHorizontalContact(
    const NDSFireballFloorLine *line,
    const Vec3f *prev,
    const Vec3f *curr,
    f32 prev_y,
    f32 curr_y,
    f32 offset_y,
    sb32 is_floor,
    NDSFireballMapContact *out)
{
    f32 delta_y;
    f32 t;
    f32 hit_x;

    if ((line == NULL) || (prev == NULL) || (curr == NULL) || (out == NULL))
    {
        return FALSE;
    }
    if (is_floor != FALSE)
    {
        if (!((prev_y >= (f32)line->y) && (curr_y < (f32)line->y)))
        {
            return FALSE;
        }
    }
    else if (!((prev_y <= (f32)line->y) && (curr_y > (f32)line->y)))
    {
        return FALSE;
    }
    delta_y = curr_y - prev_y;
    if (delta_y == 0.0F)
    {
        return FALSE;
    }
    t = ((f32)line->y - prev_y) / delta_y;
    if ((t < 0.0F) || (t > 1.0F))
    {
        return FALSE;
    }
    hit_x = prev->x + ((curr->x - prev->x) * t);
    if ((hit_x < (f32)line->x_min) || (hit_x > (f32)line->x_max))
    {
        return FALSE;
    }

    out->hit = TRUE;
    out->t = t;
    out->mask = (is_floor != FALSE) ? MAP_FLAG_FLOOR : MAP_FLAG_CEIL;
    out->line_id = line->line_id;
    out->flags = line->flags;
    out->angle.x = 0.0F;
    out->angle.y = (is_floor != FALSE) ? 1.0F : -1.0F;
    out->angle.z = 0.0F;
    out->snap_value = (f32)line->y - offset_y;
    out->snap_axis = 0u;
    out->priority = (is_floor != FALSE) ? 3u : 2u;
    return TRUE;
}

/* Horizontal radius of the source collision diamond at a world-space Y.
 * map_coll.center is retained even though Mario's source value is zero so this
 * stays correct for Luigi/retunes and for NDS_R2_FIREBALL_MAP_COLL_SCALE. */
static f32 ndsMarioFireballDiamondHalfWidth(const MPObjectColl *map_coll,
                                            f32 center_y,
                                            f32 world_y)
{
    f32 local_y;
    f32 denom;

    if (map_coll == NULL)
    {
        return 0.0F;
    }
    local_y = world_y - center_y;
    if ((local_y < map_coll->bottom) || (local_y > map_coll->top))
    {
        return 0.0F;
    }
    if (local_y <= map_coll->center)
    {
        denom = map_coll->center - map_coll->bottom;
        if (denom <= 0.0F)
        {
            return 0.0F;
        }
        return map_coll->width * ((local_y - map_coll->bottom) / denom);
    }
    denom = map_coll->top - map_coll->center;
    if (denom <= 0.0F)
    {
        return 0.0F;
    }
    return map_coll->width * ((map_coll->top - local_y) / denom);
}

static f32 ndsMarioFireballWallXAtY(const NDSFireballWallPoint *a,
                                    const NDSFireballWallPoint *b,
                                    f32 y)
{
    f32 dy = (f32)b->y - (f32)a->y;

    if (dy == 0.0F)
    {
        return ((f32)a->x + (f32)b->x) * 0.5F;
    }
    return (f32)a->x +
        (((y - (f32)a->y) / dy) * ((f32)b->x - (f32)a->x));
}

static void ndsMarioFireballWallLimitSample(
    const NDSFireballWallPoint *a,
    const NDSFireballWallPoint *b,
    const MPObjectColl *map_coll,
    f32 center_y,
    f32 sample_y,
    sb32 is_lwall,
    u32 segment,
    sb32 *found,
    f32 *limit,
    u32 *limit_segment)
{
    f32 wall_x;
    f32 radius;
    f32 candidate;

    wall_x = ndsMarioFireballWallXAtY(a, b, sample_y);
    radius = ndsMarioFireballDiamondHalfWidth(map_coll, center_y, sample_y);
    candidate = (is_lwall != FALSE) ? (wall_x - radius) : (wall_x + radius);
    if ((*found == FALSE) ||
        ((is_lwall != FALSE) ? (candidate < *limit) : (candidate > *limit)))
    {
        *found = TRUE;
        *limit = candidate;
        *limit_segment = segment;
    }
}

/* Return the center-X limit imposed by one of Dream Land's outer wall
 * polylines on the entire fireball diamond at center_y. This is the compact
 * equivalent of mpProcessRun{L,R}WallCollisionAdjNew's repeated wall samples
 * plus vertex loop: wall X +/- the diamond's horizontal radius, extremized over
 * every overlapping segment. */
static sb32 ndsMarioFireballWallLimit(
    const NDSFireballWallPoint wall[5],
    const MPObjectColl *map_coll,
    f32 center_y,
    sb32 is_lwall,
    f32 *out_limit,
    u32 *out_segment)
{
    f32 diamond_min;
    f32 diamond_max;
    f32 center_sample;
    sb32 found = FALSE;
    f32 limit = 0.0F;
    u32 limit_segment = 0u;
    u32 i;

    if ((wall == NULL) || (map_coll == NULL) || (out_limit == NULL) ||
        (out_segment == NULL))
    {
        return FALSE;
    }
    diamond_min = center_y + map_coll->bottom;
    diamond_max = center_y + map_coll->top;
    center_sample = center_y + map_coll->center;

    for (i = 0u; i < 4u; i++)
    {
        const NDSFireballWallPoint *a = &wall[i];
        const NDSFireballWallPoint *b = &wall[i + 1u];
        f32 seg_min = ((f32)a->y < (f32)b->y) ? (f32)a->y : (f32)b->y;
        f32 seg_max = ((f32)a->y > (f32)b->y) ? (f32)a->y : (f32)b->y;
        f32 overlap_min = (diamond_min > seg_min) ? diamond_min : seg_min;
        f32 overlap_max = (diamond_max < seg_max) ? diamond_max : seg_max;

        if (overlap_min > overlap_max)
        {
            continue;
        }
        ndsMarioFireballWallLimitSample(
            a, b, map_coll, center_y, overlap_min, is_lwall, i,
            &found, &limit, &limit_segment);
        if (overlap_max != overlap_min)
        {
            ndsMarioFireballWallLimitSample(
                a, b, map_coll, center_y, overlap_max, is_lwall, i,
                &found, &limit, &limit_segment);
        }
        /* The diamond radius changes slope at map_coll.center, so include that
         * breakpoint whenever it falls inside this wall segment. */
        if ((center_sample > overlap_min) && (center_sample < overlap_max))
        {
            ndsMarioFireballWallLimitSample(
                a, b, map_coll, center_y, center_sample, is_lwall, i,
                &found, &limit, &limit_segment);
        }
    }
    if (found == FALSE)
    {
        return FALSE;
    }
    *out_limit = limit;
    *out_segment = limit_segment;
    return TRUE;
}

static sb32 ndsMarioFireballWallContact(
    const NDSFireballWallPoint wall[5],
    const Vec3f normals[4],
    const MPObjectColl *map_coll,
    const Vec3f *prev,
    const Vec3f *curr,
    sb32 is_lwall,
    NDSFireballMapContact *out)
{
    const f32 epsilon = 0.001F;
    f32 prev_limit;
    f32 curr_limit;
    f32 prev_clear;
    f32 curr_clear;
    f32 denom;
    u32 prev_segment;
    u32 curr_segment;

    if ((wall == NULL) || (normals == NULL) || (map_coll == NULL) ||
        (prev == NULL) || (curr == NULL) || (out == NULL) ||
        (ndsMarioFireballWallLimit(wall, map_coll, prev->y, is_lwall,
                                   &prev_limit, &prev_segment) == FALSE) ||
        (ndsMarioFireballWallLimit(wall, map_coll, curr->y, is_lwall,
                                   &curr_limit, &curr_segment) == FALSE))
    {
        return FALSE;
    }

    /* LWall is the left exterior face: legal space is center_x <= limit.
     * RWall is the right exterior face: legal space is center_x >= limit.
     * Only an outside->solid crossing collides, preserving the source's
     * one-sided walls instead of treating the stage body as an infinite slab. */
    if (is_lwall != FALSE)
    {
        prev_clear = prev_limit - prev->x;
        curr_clear = curr_limit - curr->x;
    }
    else
    {
        prev_clear = prev->x - prev_limit;
        curr_clear = curr->x - curr_limit;
    }
    if ((prev_clear < -epsilon) || (curr_clear >= -epsilon))
    {
        return FALSE;
    }
    denom = prev_clear - curr_clear;
    if (denom <= 0.0F)
    {
        return FALSE;
    }

    out->hit = TRUE;
    out->t = prev_clear / denom;
    if (out->t < 0.0F) { out->t = 0.0F; }
    if (out->t > 1.0F) { out->t = 1.0F; }
    out->mask = (is_lwall != FALSE) ? MAP_FLAG_LWALL : MAP_FLAG_RWALL;
    out->line_id = (is_lwall != FALSE) ? 6 : 5;
    out->flags = 0u;
    out->angle = normals[curr_segment];
    out->snap_value = curr_limit;
    out->snap_axis = 1u;
    out->priority = (is_lwall != FALSE) ? 0u : 1u;
    return TRUE;
}

static void ndsMarioFireballApplyNativeContact(WPStruct *wp,
                                                DObj *dobj,
                                                const NDSFireballMapContact *hit)
{
    MPCollData *coll;

    if ((wp == NULL) || (dobj == NULL) || (hit == NULL) || (hit->hit == FALSE))
    {
        return;
    }
    coll = &wp->coll_data;
    coll->mask_curr |= hit->mask;
    coll->mask_stat |= hit->mask;
    if (hit->snap_axis == 0u)
    {
        dobj->translate.vec.f.y = hit->snap_value;
    }
    else
    {
        dobj->translate.vec.f.x = hit->snap_value;
        coll->mask_unk |= hit->mask;
    }

    switch (hit->mask)
    {
    case MAP_FLAG_FLOOR:
        coll->floor_line_id = hit->line_id;
        coll->floor_flags = hit->flags;
        coll->floor_angle = hit->angle;
        coll->floor_dist = 0.0F;
        gNdsFireballNativeMapFloorHitCount++;
        break;
    case MAP_FLAG_CEIL:
        coll->ceil_line_id = hit->line_id;
        coll->ceil_flags = hit->flags;
        coll->ceil_angle = hit->angle;
        gNdsFireballNativeMapCeilHitCount++;
        break;
    case MAP_FLAG_LWALL:
        coll->lwall_line_id = hit->line_id;
        coll->lwall_flags = hit->flags;
        coll->lwall_angle = hit->angle;
        gNdsFireballNativeMapLWallHitCount++;
        break;
    case MAP_FLAG_RWALL:
        coll->rwall_line_id = hit->line_id;
        coll->rwall_flags = hit->flags;
        coll->rwall_angle = hit->angle;
        gNdsFireballNativeMapRWallHitCount++;
        break;
    default:
        break;
    }
    gNdsFireballNativeMapCollisionCount++;
    gNdsFireballNativeMapLastMask = hit->mask;
    gNdsFireballNativeMapLastLineID = hit->line_id;
}

static sb32 ndsMarioFireballMapTestAll(GObj *weapon_gobj)
{
    WPStruct *wp;
    DObj *dobj;
    MPCollData *coll;
    Vec3f previous;
    Vec3f current;
    NDSFireballMapContact best = { 0 };
    NDSFireballMapContact next = { 0 };
    f32 prev_bottom;
    f32 curr_bottom;
    f32 prev_top;
    f32 curr_top;
    u32 i;

    gNdsFireballNativeMapCallCount++;
    if (weapon_gobj == NULL)
    {
        gNdsFireballNativeMapFallbackCount++;
        return FALSE;
    }
    if ((gSCManagerBattleState == NULL) ||
        (gSCManagerBattleState->gkind != nGRKindPupupu))
    {
        gNdsFireballNativeMapFallbackCount++;
        return wpMapTestAll(weapon_gobj);
    }
    wp = wpGetStruct(weapon_gobj);
    dobj = DObjGetStruct(weapon_gobj);
    if ((wp == NULL) || (dobj == NULL) || (wp->kind != nWPKindFireball))
    {
        gNdsFireballNativeMapFallbackCount++;
        return FALSE;
    }

    coll = &wp->coll_data;
    previous = coll->pos_prev;
    current = dobj->translate.vec.f;
    prev_bottom = previous.y + coll->map_coll.bottom;
    curr_bottom = current.y + coll->map_coll.bottom;
    prev_top = previous.y + coll->map_coll.top;
    curr_top = current.y + coll->map_coll.top;

    /* Source order matters only for an exact shared corner; contact time wins
     * normally, and priority reproduces wpMapProcAll's L/R/ceil/floor tie order. */
    /* Pupupu's two outer walls exist only from y=-1072..0. Most of a
     * fireball's life is above that band, so reject both wall polylines with
     * four already-needed endpoint values instead of running two 4-segment
     * scans every frame. */
    if ((((prev_bottom < curr_bottom) ? prev_bottom : curr_bottom) <= 0.0F) &&
        (((prev_top > curr_top) ? prev_top : curr_top) >= -1072.0F))
    {
        if (ndsMarioFireballWallContact(
                sNdsFireballPupupuLWall, sNdsFireballPupupuLWallNormal,
                &coll->map_coll, &previous, &current, TRUE, &next) != FALSE)
        {
            ndsMarioFireballConsiderContact(&best, &next);
        }
        next.hit = FALSE;
        if (ndsMarioFireballWallContact(
                sNdsFireballPupupuRWall, sNdsFireballPupupuRWallNormal,
                &coll->map_coll, &previous, &current, FALSE, &next) != FALSE)
        {
            ndsMarioFireballConsiderContact(&best, &next);
        }
    }

    /* Horizontal source lines are one-sided. Direction-gate whole groups so
     * the common descending flight never even calls the ceiling test and a
     * post-rebound ascending flight skips all four floors. */
    if (current.y > previous.y)
    {
        next.hit = FALSE;
        if (ndsMarioFireballHorizontalContact(
                &sNdsFireballPupupuCeil, &previous, &current,
                prev_top, curr_top, coll->map_coll.top, FALSE, &next) != FALSE)
        {
            ndsMarioFireballConsiderContact(&best, &next);
        }
    }
    else if (current.y < previous.y)
    {
        for (i = 0u; i < ARRAY_COUNT(sNdsFireballPupupuFloors); i++)
        {
            next.hit = FALSE;
            if (ndsMarioFireballHorizontalContact(
                    &sNdsFireballPupupuFloors[i], &previous, &current,
                    prev_bottom, curr_bottom, coll->map_coll.bottom,
                    TRUE, &next) != FALSE)
            {
                ndsMarioFireballConsiderContact(&best, &next);
            }
        }
    }

    if (best.hit != FALSE)
    {
        ndsMarioFireballApplyNativeContact(wp, dobj, &best);
    }
    coll->update_tic = gMPCollisionUpdateTic;
    gNdsFireballNativeMapHandledCount++;
    return FALSE;
}
#endif /* NDS_R2_FIREBALL_NATIVE_MAP_COLL */

static sb32 ndsMarioFireballProcReflectorProbe(GObj *weapon_gobj)
{
    WPStruct *wp = wpGetStruct(weapon_gobj);
    sb32 result;

    if (wp != NULL)
    {
        gNdsFighterReflectorProofFireballVelXBefore =
            (s32)(wp->physics.vel_air.x * 1000.0F);
    }
    result = wpMarioFireballProcReflector(weapon_gobj);
    if (wp != NULL)
    {
        FTStruct *owner = ftGetStruct(wp->owner_gobj);

        gNdsFighterReflectorProofFireballProcCount++;
        gNdsFighterReflectorProofFireballVelXAfter =
            (s32)(wp->physics.vel_air.x * 1000.0F);
        if (owner != NULL)
        {
            gNdsFighterReflectorProofFireballOwnerKind = (u32)owner->fkind;
        }
    }
    return result;
}

static sb32 ndsMarioFireballProcUpdateProbe(GObj *weapon_gobj)
{
    sb32 result = wpMarioFireballProcUpdate(weapon_gobj);

    if (result != FALSE)
    {
        gNdsFighterProjectileProofUpdateDestroyCount++;
    }
#if NDS_R2_FIREBALL_MAP_COLL_DEBUG
    /* Capture the live collision diamond for the debug overlay drawn from
     * lbParticleDrawTextures. Only the most-recently-updated fireball is shown,
     * which is the one in flight. Values are read live each frame here so the
     * overlay tracks the fireball as it moves/rebounds. */
    if (result == FALSE)
    {
        WPStruct *wp = wpGetStruct(weapon_gobj);
        if (wp != NULL)
        {
            Vec3f *t = &DObjGetStruct(weapon_gobj)->translate.vec.f;
            gNdsFireballDebugCollActive = 1u;
            gNdsFireballDebugCollX = t->x;
            gNdsFireballDebugCollY = t->y;
            gNdsFireballDebugCollZ = t->z;
            gNdsFireballDebugCollTop    = wp->coll_data.map_coll.top;
            gNdsFireballDebugCollCenter = wp->coll_data.map_coll.center;
            gNdsFireballDebugCollBottom = wp->coll_data.map_coll.bottom;
            gNdsFireballDebugCollWidth  = wp->coll_data.map_coll.width;
        }
    }
    else
    {
        gNdsFireballDebugCollActive = 0u;
    }
#endif
    return result;
}

static sb32 ndsMarioFireballProcMapProbe(GObj *weapon_gobj)
{
    sb32 result = wpMarioFireballProcMap(weapon_gobj);

    if (result != FALSE)
    {
        gNdsFighterProjectileProofMapDestroyCount++;
    }
    return result;
}

static sb32 ndsMarioFireballProcHitProbe(GObj *weapon_gobj)
{
    sb32 result = wpMarioFireballProcHit(weapon_gobj);

    if (result != FALSE)
    {
        gNdsFighterProjectileProofHitDestroyCount++;
    }
    return result;
}

static void ndsMarioFireballRecordCreatedWeapon(GObj *weapon_gobj)
{
    WPStruct *wp = wpGetStruct(weapon_gobj);

    if (wp == NULL)
    {
        return;
    }
    if ((wp->kind >= 0) && (wp->kind < 32))
    {
        gNdsFighterProjectileProofKindMask |= 1u << wp->kind;
    }
    if ((wp->attack_coll.attack_state >= 0) &&
        (wp->attack_coll.attack_state < 32))
    {
        gNdsFighterProjectileProofAttackStateMask |=
            1u << wp->attack_coll.attack_state;
    }
    if ((u32)wp->attack_coll.damage >
        gNdsFighterProjectileProofDamageMax)
    {
        gNdsFighterProjectileProofDamageMax =
            (u32)wp->attack_coll.damage;
    }
    gNdsFighterReflectorProofFireballCanReflect =
        (wp->attack_coll.can_reflect != FALSE) ? 1u : 0u;
    gNdsFighterReflectorProofFireballCanAbsorb =
        (wp->attack_coll.can_absorb != FALSE) ? 1u : 0u;
    gNdsFighterReflectorProofFireballCanShield =
        (wp->attack_coll.can_shield != FALSE) ? 1u : 0u;
    gNdsFighterReflectorProofFireballAttackCount =
        (u32)wp->attack_coll.attack_count;
    gNdsFighterReflectorProofFireballDamage =
        (u32)wp->attack_coll.damage;
    gNdsFighterReflectorProofFireballSizeMilli =
        (u32)(wp->attack_coll.size * 1000.0F);
    if ((u32)wp->lifetime > gNdsFighterProjectileProofLifetimeMax)
    {
        gNdsFighterProjectileProofLifetimeMax = (u32)wp->lifetime;
    }
}

/* wpmanager.c's free list, and it is an ordinary global rather than a static,
 * so the exact reason a make failed is readable from here without touching the
 * decomp TU. wpManagerMakeWeapon has exactly two NULL returns: the struct pool
 * is empty (head stays NULL), or gcMakeGObjSPAfter refused and the struct was
 * pushed back (head is non-NULL). Sampled AFTER the call, that is a decision,
 * not a heuristic. */
extern WPStruct *sWPManagerStructsAllocFree;

/* objman.c's GObj pool state. gcGetGObjSetNextAlloc (objman.c:398) refuses only
 * when sGCCommonsMaxNum is not -1 and the pool is at that cap, which is the
 * ifCommonSetMaxNumGObj latch. Reading it at END of run reads the Results
 * scene, where it is -1 again; reading it AT the refusal is the evidence. */
extern s16 sGCCommonsMaxNum;
extern s32 sGCCommonsActiveNum;

/* Weapons alive, exactly: WEAPON_ALLOC_MAX minus the free-list depth. The
 * counter this replaces was a fixed 1 -- it latched on the first weapon and
 * never moved -- so a 2026-08-01 soak reading `WeaponCountMax 1` alongside
 * `SpawnCall 11 / SpawnSuccess 7` looked like a one-at-a-time pool limit and
 * was not evidence of anything. Walking <=32 nodes eleven times a match is
 * free. */
static void ndsMarioFireballSampleWeaponPool(void)
{
    const WPStruct *node = sWPManagerStructsAllocFree;
    u32 free_depth = 0u;
    u32 live;

    while ((node != NULL) && (free_depth <= NDS_R2_WEAPON_POOL))
    {
        free_depth++;
        node = node->next;
    }
    live = (free_depth <= NDS_R2_WEAPON_POOL) ?
        ((u32)NDS_R2_WEAPON_POOL - free_depth) : 0u;
    if (live > gNdsFighterProjectileProofWeaponCountMax)
    {
        gNdsFighterProjectileProofWeaponCountMax = live;
    }
}

GObj *wpMarioFireballMakeWeapon(GObj *fighter_gobj, Vec3f *pos, s32 index)
{
    GObj *weapon_gobj;

    gNdsFighterProjectileProofSpawnCallCount++;
    dWPMarioFireballWeaponDesc.proc_update = ndsMarioFireballProcUpdateProbe;
    dWPMarioFireballWeaponDesc.proc_map = ndsMarioFireballProcMapProbe;
    dWPMarioFireballWeaponDesc.proc_hit = ndsMarioFireballProcHitProbe;
    dWPMarioFireballWeaponDesc.proc_shield = ndsMarioFireballProcHitProbe;
    dWPMarioFireballWeaponDesc.proc_setoff = ndsMarioFireballProcHitProbe;
    dWPMarioFireballWeaponDesc.proc_absorb = ndsMarioFireballProcHitProbe;
    dWPMarioFireballWeaponDesc.proc_reflector =
        ndsMarioFireballProcReflectorProbe;
    weapon_gobj = battleship_wpMarioFireballMakeWeapon(fighter_gobj, pos,
                                                       index);
    if (weapon_gobj != NULL)
    {
        WPStruct *wp = wpGetStruct(weapon_gobj);
        gNdsFighterProjectileProofSpawnSuccessCount++;
        ndsMarioFireballRecordCreatedWeapon(weapon_gobj);
        /* Port-side retune of the fireball's stage-collision diamond. The
         * source values come straight from ROM reloc data (WPAttributes
         * map_coll_*), loaded by wpManagerMakeWeapon above; the port has no
         * port-side entry to edit those.
         *
         * All four fields scaled: top (ceiling probe), center (wall Y
         * midpoint), bottom (floor probe), width (wall probe). 1.0F is
         * source-exact; >1.0 enlarges the whole diamond. Owner playtest
         * 2026-08-07: the source collision is too small, lets the sprite clip
         * into floors; scaling all sides 1.5x. */
        if (wp != NULL)
        {
            wp->coll_data.map_coll.top    *= NDS_R2_FIREBALL_MAP_COLL_SCALE;
            wp->coll_data.map_coll.center *= NDS_R2_FIREBALL_MAP_COLL_SCALE;
            wp->coll_data.map_coll.bottom *= NDS_R2_FIREBALL_MAP_COLL_SCALE;
            wp->coll_data.map_coll.width  *= NDS_R2_FIREBALL_MAP_COLL_SCALE;
        }
    }
    else if (sWPManagerStructsAllocFree != NULL)
    {
        gNdsFighterProjectileProofSpawnFailGObjCount++;
        gNdsFighterProjectileProofSpawnFailGObjMax =
            (u32)(u16)sGCCommonsMaxNum;
        gNdsFighterProjectileProofSpawnFailGObjActive =
            (u32)sGCCommonsActiveNum;
        /* The number the cap is actually about. ifCommonSetMaxNumGObj latches
         * when this drops under 25,600, so this says how far the battle is
         * from never latching -- i.e. how many bytes a pool trim has to
         * return before the fireball stops being refused. */
        gNdsFighterProjectileProofSpawnFailHeapFree =
            (u32)((uintptr_t)gSYTaskmanGeneralHeap.end -
                  (uintptr_t)gSYTaskmanGeneralHeap.ptr);
        gNdsWeaponStructBytes = (u32)sizeof(WPStruct);
    }
    else
    {
        gNdsFighterProjectileProofSpawnFailPoolCount++;
    }
    ndsMarioFireballSampleWeaponPool();
    return weapon_gobj;
}

#include "../../decomp/BattleShip-main/decomp/src/ft/ftchar/ftmario/ftmariospecialn.c"

#endif
