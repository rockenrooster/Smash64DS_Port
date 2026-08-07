#include <nds/nds_firegrind.h>

#if NDS_R2_FIREGRIND_NATIVE

/* FireGrind, without the particle interpreter.
 *
 * One fixed pool, packed from [0, live_count). A dead spark is removed by
 * swapping the last live spark into its slot and decrementing the count, so the
 * live set is always contiguous and a draw pass is a plain forward loop.
 *
 * Everything source-derived is baked: colour ramps, initial velocities, and the
 * two-iterations-into-one physics update. The pool owns no GObj, no LBParticle,
 * no LBGenerator, no LBTransform, and no texture; it draws through the open
 * particle GX batch in lbParticleDrawTextures.
 *
 * PHYSICS, two source ticks combined into one presentation step.
 * Source per-tick (lbparticle.c interpreter integrate):
 *   vy -= gravity(3)
 *   vx *= friction(0.8); vy *= friction(0.8); vz *= friction(0.8)
 *   pos += vel
 * Two iterations fold (vel and pos both advance twice) to:
 *   pos += vel * 1.44
 *   vel  = vel * 0.64 + (gravity accumulated once per iter on Y)
 * The gravity term: after iter1 vy=(vy-3)*0.8; after iter2
 *   vy = ((vy-3)*0.8 - 3)*0.8 = vy*0.64 - 3*0.8*0.8 - 3*0.8 = vy*0.64 - 4.32.
 * And pos.y gets vy-3 then (vy-3)*0.8-3 contributions on top of the 1.44 vel
 * base, netting the constant -6.72 used below.
 *
 * COLOUR by presentation index (source age = 2*index), BGR555. Folded with
 * `channel >> 3` to be bit-identical to the generic draw path's conversion.
 */

/* sNdsFireGrindColors[variant][frame_index], BGR555.
 * Source lerps primcolour start->target over prim_length ticks; presentation
 * advances age by 2 source ticks/frame, so index i == source age 2*i. */
static const u16 sNdsFireGrindColors[3][NDS_FIREGRIND_COLOR_FRAMES] = {
    /* variant 0 -- source gen 8, size 120, ramp len 17 */
    { 0x67FF, 0x5FFF, 0x5BDF, 0x53BF, 0x4FBF, 0x479F, 0x437F, 0x3B7F,
      0x375F, 0x335F, 0x335F, 0x335F, 0x335F },
    /* variant 1 -- source gen 9, size 130, ramp len 25 */
    { 0x27FF, 0x23DF, 0x23BF, 0x1F9F, 0x1F7F, 0x1B5F, 0x1B3F, 0x1B1F,
      0x16FF, 0x16DF, 0x12DF, 0x12BF, 0x0E9E },
    /* variant 2 -- source gen 10, size 140, ramp len 25 */
    { 0x0A5E, 0x0A3E, 0x0A1E, 0x0A1E, 0x09FE, 0x09DF, 0x09BF, 0x09BF,
      0x099F, 0x097F, 0x097F, 0x095F, 0x093F },
};

/* Representative samples of the source spawn cone (lbparticle.c:2358-2419) at
 * each variant's speed, evaluated offline. The generator's vel is (0,0,speed),
 * so the world rotation is identity and the spark velocity is literally
 * (cos(pv0)*sin(pv1)*S, sin(pv0)*sin(pv1)*S, cos(pv1)*S): dominant thrust is +Z
 * (into the screen), X/Y carry the azimuthal spread, and gravity arcs Y down.
 * Eight octant directions per variant; a cheap incrementing burst serial picks
 * one per spawn for variation without runtime trig. */
static const Vec3f sNdsFireGrindInitialVel[3][8] = {
    /* variant 0, speed 80 */
    { { 33.809F, 0.000F, 72.505F }, { 23.907F, 23.907F, 72.505F },
      { 0.000F, 33.809F, 72.505F }, { -23.907F, 23.907F, 72.505F },
      { -33.809F, 0.000F, 72.505F }, { -23.907F, -23.907F, 72.505F },
      { 0.000F, -33.809F, 72.505F }, { 23.907F, -23.907F, 72.505F } },
    /* variant 1, speed 90 */
    { { 38.036F, 0.000F, 81.568F }, { 26.895F, 26.895F, 81.568F },
      { 0.000F, 38.036F, 81.568F }, { -26.895F, 26.895F, 81.568F },
      { -38.036F, 0.000F, 81.568F }, { -26.895F, -26.895F, 81.568F },
      { 0.000F, -38.036F, 81.568F }, { 26.895F, -26.895F, 81.568F } },
    /* variant 2, speed 100 */
    { { 42.262F, 0.000F, 90.631F }, { 29.884F, 29.884F, 90.631F },
      { 0.000F, 42.262F, 90.631F }, { -29.884F, 29.884F, 90.631F },
      { -42.262F, 0.000F, 90.631F }, { -29.884F, -29.884F, 90.631F },
      { 0.000F, -42.262F, 90.631F }, { 29.884F, -29.884F, 90.631F } },
};

/* One source-derived size per variant (generators 8/9/10). */
static const f32 sNdsFireGrindSize[3] = { 120.0F, 130.0F, 140.0F };

/* Deterministic lifetime table. Source SETLIFERAND gives 10..26 source ticks
 * (base 10, rand 16); presentation halves that to 5..13 frames. A short repeat
 * gives varied short/medium/long sparks across successive bursts without RNG. */
static const u8 sNdsFireGrindLifetimeFrames[8] = { 6u, 9u, 7u, 11u, 8u, 5u, 10u, 12u };

static NDSFireGrindParticle sNdsFireGrindPool[NDS_FIREGRIND_MAX_PARTICLES];
static u32 sNdsFireGrindLiveCount;
/* Incrementing burst serial: each bounce consumes three entries (one per
 * variant) and wraps 0..7. Gives directional variation across bounces. */
static u8 sNdsFireGrindBurstSerial;

void ndsFireGrindReset(void)
{
    sNdsFireGrindLiveCount = 0u;
    sNdsFireGrindBurstSerial = 0u;
}

/* Map a variant to its direction index for this burst. Staggering the three
 * variants across the octant table (N, N+2, N+5) spreads the three sparks so a
 * bounce reads as a spread, not a clump, without a second RNG. */
static u8 ndsFireGrindDirIndex(u8 variant)
{
    return (u8)(((u32)sNdsFireGrindBurstSerial
                 + (u32)variant * 2u + (variant == 2u ? 1u : 0u)) & 7u);
}

void ndsFireGrindSpawn(const Vec3f *pos)
{
    u32 variant;
    u8 dir_base;

    if (pos == NULL)
    {
        return;
    }
    /* Drop the whole burst if three slots are not free -- never a partial 1- or
     * 2-spark effect, which would read as a broken fireball. */
    if (sNdsFireGrindLiveCount > (NDS_FIREGRIND_MAX_PARTICLES - 3u))
    {
        return;
    }
    dir_base = sNdsFireGrindBurstSerial;
    for (variant = 0u; variant < 3u; variant++)
    {
        NDSFireGrindParticle *p = &sNdsFireGrindPool[sNdsFireGrindLiveCount];
        u8 dir = ndsFireGrindDirIndex((u8)variant);
        const Vec3f *vel = &sNdsFireGrindInitialVel[variant][dir];

        p->pos = *pos;
        p->vel = *vel;
        p->age = 0u;
        p->variant = (u8)variant;
        p->lifetime = sNdsFireGrindLifetimeFrames[dir_base & 7u];
        sNdsFireGrindLiveCount++;
    }
    sNdsFireGrindBurstSerial = (u8)(((u32)dir_base + 1u) & 7u);
}

void ndsFireGrindUpdate(void)
{
    u32 i = 0u;

    while (i < sNdsFireGrindLiveCount)
    {
        NDSFireGrindParticle *p = &sNdsFireGrindPool[i];

        /* Two source iterations folded into one presentation step. See the
         * block comment at the top of this file for the derivation. The draw
         * pass runs before this update, so a spark spawned this frame is drawn
         * at age 0 before its first integrate -- no newborn latch needed. */
        p->pos.x += p->vel.x * 1.44F;
        p->pos.z += p->vel.z * 1.44F;
        p->pos.y += p->vel.y * 1.44F - 6.72F;
        p->vel.x *= 0.64F;
        p->vel.z *= 0.64F;
        p->vel.y = p->vel.y * 0.64F - 4.32F;
        p->age++;
        if (p->age >= p->lifetime)
        {
            u32 last = sNdsFireGrindLiveCount - 1u;

            if (i != last)
            {
                sNdsFireGrindPool[i] = sNdsFireGrindPool[last];
            }
            sNdsFireGrindLiveCount = last;
            /* Do not advance i: the swapped-in spark still needs its update. */
        }
        else
        {
            i++;
        }
    }
}

const NDSFireGrindParticle *ndsFireGrindPool(u32 *count_out)
{
    if (count_out != NULL)
    {
        *count_out = sNdsFireGrindLiveCount;
    }
    return sNdsFireGrindPool;
}

u16 ndsFireGrindColor(u8 variant, u8 age)
{
    u32 idx = (u32)age;

    if (idx >= NDS_FIREGRIND_COLOR_FRAMES)
    {
        idx = NDS_FIREGRIND_COLOR_FRAMES - 1u;
    }
    if (variant > 2u)
    {
        variant = 0u;
    }
    return sNdsFireGrindColors[variant][idx];
}

f32 ndsFireGrindSize(u8 variant)
{
    if (variant > 2u)
    {
        variant = 0u;
    }
    return sNdsFireGrindSize[variant];
}

#else /* !NDS_R2_FIREGRIND_NATIVE */

/* The subsystem compiles to nothing when the flag is off, so the default ROM
 * keeps the source-faithful BattleShip FireGrind path and pays no code-size
 * cost for an effect it does not use. */
void ndsFireGrindReset(void) {}
void ndsFireGrindSpawn(const Vec3f *pos) { (void)pos; }
void ndsFireGrindUpdate(void) {}
const NDSFireGrindParticle *ndsFireGrindPool(u32 *count_out)
{
    if (count_out != NULL)
    {
        *count_out = 0u;
    }
    return NULL;
}
u16 ndsFireGrindColor(u8 variant, u8 age)
{
    (void)variant;
    (void)age;
    return 0u;
}
f32 ndsFireGrindSize(u8 variant)
{
    (void)variant;
    return 0.0F;
}

#endif /* NDS_R2_FIREGRIND_NATIVE */
