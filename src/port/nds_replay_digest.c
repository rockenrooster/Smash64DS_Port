/* P2-2p8 Phase 0: the gameplay replay digest.
 *
 * docs/p2/FOUR_FIGHTER_30FPS_ARCHITECTURE.md section 5. Every later phase
 * replaces machinery around the source's gameplay rules and must leave those
 * rules' results untouched. This is the instrument that says so: after every
 * logic tick it folds the gameplay state into one 32-bit FNV-1a word, and the
 * battle host publishes the two words of each presented frame into the tick-HUD
 * ring (DGSA = the undrawn tick, DGSB = the drawn tick). A candidate ROM and its
 * control, run on the same deterministic four-CPU match, must produce identical
 * digest columns for the whole match; scripts/compare-replay-digest.py names the
 * first frame that differs.
 *
 * What is folded, per fighter in link order, per item and per weapon: identity,
 * status and motion, the counters the source's rules key on, and the RAW BITS
 * of position and velocity. Bit-exactness is the right bar for every planned
 * phase: renderer, residency, pre-bound motions and the guarded hurtbox kernel
 * all leave the float gameplay path producing the same bits. The RNG seed is
 * folded last, which catches a changed number or order of random draws even
 * when every visible field still agrees.
 *
 * Instrument only: compiled under NDS_TICK_HUD, never in a published ROM. */
#include "nds_scene_harness_config.h"

#if NDS_TICK_HUD

#include <sys/obj.h>
#include <ft/fighter.h>
#include <it/item.h>
#include <wp/weapon.h>
#include <nds/nds_startup.h>

extern GObj *gGCCommonLinks[];
extern s32 syUtilsRandSeed(void);

volatile u32 gNdsReplayDigestTicks;

static inline u32 ndsReplayDigestMix(u32 hash, u32 value)
{
    u32 i;

    for (i = 0u; i < 4u; i++)
    {
        hash ^= value & 0xffu;
        hash *= 16777619u;
        value >>= 8;
    }
    return hash;
}

static inline u32 ndsReplayDigestMixF32(u32 hash, f32 value)
{
    union
    {
        f32 f;
        u32 u;
    } bits;

    bits.f = value;
    return ndsReplayDigestMix(hash, bits.u);
}

static u32 ndsReplayDigestMixVec3(u32 hash, const Vec3f *v)
{
    hash = ndsReplayDigestMixF32(hash, v->x);
    hash = ndsReplayDigestMixF32(hash, v->y);
    return ndsReplayDigestMixF32(hash, v->z);
}

static u32 ndsReplayDigestMixPosition(u32 hash, GObj *gobj)
{
    DObj *dobj = DObjGetStruct(gobj);

    if (dobj == NULL)
    {
        return ndsReplayDigestMix(hash, 0xdeadd0b1u);
    }
    return ndsReplayDigestMixVec3(hash, &dobj->translate.vec.f);
}

u32 ndsReplayDigestTick(void)
{
    u32 hash = 2166136261u;
    GObj *gobj;

    gobj = gGCCommonLinks[nGCCommonLinkIDFighter];
    while (gobj != NULL)
    {
        const FTStruct *fp = (const FTStruct *)gobj->user_data.p;

        if (fp != NULL)
        {
            hash = ndsReplayDigestMix(hash, (u32)fp->fkind);
            hash = ndsReplayDigestMix(hash, (u32)fp->player);
            hash = ndsReplayDigestMix(hash, (u32)fp->status_id);
            hash = ndsReplayDigestMix(hash, (u32)fp->motion_id);
            hash = ndsReplayDigestMix(hash, fp->status_total_tics);
            hash = ndsReplayDigestMix(hash, (u32)fp->percent_damage);
            hash = ndsReplayDigestMix(hash, (u32)fp->stock_count);
            hash = ndsReplayDigestMix(hash, (u32)fp->shield_health);
            hash = ndsReplayDigestMix(hash, fp->hitlag_tics);
            hash = ndsReplayDigestMix(hash, (u32)fp->lr);
            hash = ndsReplayDigestMix(hash, (u32)fp->ga);
            hash = ndsReplayDigestMix(hash, (u32)fp->jumps_used);
            hash = ndsReplayDigestMix(hash,
                ((fp->catch_gobj != NULL) ? 1u : 0u) |
                ((fp->capture_gobj != NULL) ? 2u : 0u));
            hash = ndsReplayDigestMixVec3(hash, &fp->physics.vel_air);
            hash = ndsReplayDigestMixVec3(hash, &fp->physics.vel_damage_air);
            hash = ndsReplayDigestMixVec3(hash, &fp->physics.vel_ground);
            hash = ndsReplayDigestMixF32(hash, fp->physics.vel_damage_ground);
        }
        hash = ndsReplayDigestMixPosition(hash, gobj);
        gobj = gobj->link_next;
    }

    gobj = gGCCommonLinks[nGCCommonLinkIDItem];
    while (gobj != NULL)
    {
        const ITStruct *ip = (const ITStruct *)gobj->user_data.p;

        hash = ndsReplayDigestMix(hash, (ip != NULL) ? (u32)ip->kind : 0xffffffffu);
        hash = ndsReplayDigestMixPosition(hash, gobj);
        gobj = gobj->link_next;
    }

    gobj = gGCCommonLinks[nGCCommonLinkIDWeapon];
    while (gobj != NULL)
    {
        const WPStruct *wp = (const WPStruct *)gobj->user_data.p;

        hash = ndsReplayDigestMix(hash, (wp != NULL) ? (u32)wp->kind : 0xffffffffu);
        hash = ndsReplayDigestMixPosition(hash, gobj);
        gobj = gobj->link_next;
    }

    hash = ndsReplayDigestMix(hash, (u32)syUtilsRandSeed());
    gNdsReplayDigestTicks++;
    return hash;
}

#endif /* NDS_TICK_HUD */
