#include <ft/fighter.h>
#include <it/item.h>
#include <wp/weapon.h>

#ifndef DObjGetStruct
#define DObjGetStruct(gobj) ((DObj *)(gobj)->obj)
#endif

#ifndef WEAPON_ATKCOLL_NUM_MAX
#define WEAPON_ATKCOLL_NUM_MAX 2
#endif

#ifndef ITEM_ATKCOLL_NUM_MAX
#define ITEM_ATKCOLL_NUM_MAX 2
#endif

#ifndef SSB64_NDS_WP_ATTACK_COLL_DECLARED
typedef struct WPAttackPos {
    Vec3f pos_curr;
    Vec3f pos_prev;
    sb32 unk_wphitpos_0x18;
    Mtx44f mtx;
    f32 unk_wphitpos_0x5C;
} WPAttackPos;

typedef struct WPAttackColl {
    s32 attack_state;
    s32 damage;
    f32 stale;
    s32 element;
    Vec3f offsets[WEAPON_ATKCOLL_NUM_MAX];
    f32 size;
    s32 angle;
    u32 knockback_scale;
    u32 knockback_weight;
    u32 knockback_base;
    s32 shield_damage;
    s32 priority;
    u8 interact_mask;
    u16 fgm_id;
    ub32 can_setoff : 1;
    ub32 can_rehit_item : 1;
    ub32 can_rehit_fighter : 1;
    ub32 can_rehit_shield : 1;
    ub32 can_hop : 1;
    ub32 can_reflect : 1;
    ub32 can_absorb : 1;
    ub32 can_not_heal : 1;
    ub32 can_shield : 1;
    u32 motion_attack_id : 6;
    u16 motion_count;
    GMStatFlags stat_flags;
    u16 stat_count;
    s32 attack_count;
    WPAttackPos attack_pos[WEAPON_ATKCOLL_NUM_MAX];
    GMAttackRecord attack_records[GMATTACKREC_NUM_MAX];
} WPAttackColl;
#endif

#ifndef SSB64_NDS_IT_ATTACK_COLL_DECLARED
typedef struct ITAttackPos {
    Vec3f pos_curr;
    Vec3f pos_prev;
    sb32 unk_ithitpos_0x18;
    Mtx44f mtx;
    f32 unk_ithitpos_0x5C;
} ITAttackPos;

typedef struct ITAttackColl {
    s32 attack_state;
    s32 damage;
    f32 throw_mul;
    f32 stale;
    s32 element;
    Vec3f offsets[ITEM_ATKCOLL_NUM_MAX];
    f32 size;
    s32 angle;
    u32 knockback_scale;
    u32 knockback_weight;
    u32 knockback_base;
    s32 shield_damage;
    s32 priority;
    u8 interact_mask;
    u16 fgm_id;
    ub32 can_setoff : 1;
    ub32 can_rehit_item : 1;
    ub32 can_rehit_fighter : 1;
    ub32 can_rehit_shield : 1;
    ub32 can_hop : 1;
    ub32 can_reflect : 1;
    ub32 can_shield : 1;
    u32 motion_attack_id : 6;
    u16 motion_count;
    GMStatFlags stat_flags;
    u16 stat_count;
    s32 attack_count;
    ITAttackPos attack_pos[ITEM_ATKCOLL_NUM_MAX];
    GMAttackRecord attack_records[GMATTACKREC_NUM_MAX];
} ITAttackColl;

typedef struct ITDamageColl {
    u8 interact_mask;
    s32 hitstatus;
    Vec3f offset;
    Vec3f size;
} ITDamageColl;
#endif

f32 lbCommonSin(f32 angle);
f32 lbCommonCos(f32 angle);
void syDebugPrintf(const char *fmt, ...);
void scManagerRunPrintGObjStatus(void);

#include "../../decomp/BattleShip-main/decomp/src/gm/gmcollision.c"

#if NDS_R2_COLLISION_L7_ORACLE
/* R2-07 L7 step one. Lives in THIS translation unit and not in a port file
 * because it has to call the decomp's own gmCollisionGetWorldPosition as the
 * reference, and after the include above that symbol is right here. Comparing
 * against a transcription of the reference would prove the transcription.
 *
 * Read-only: it reads mtx_translate and unk_dobjtrans_0x9C, writes only its own
 * counters, and runs after the gameplay tick. The header explains what each
 * counter settles. */
#include <nds/nds_r2_collision_mtx.h>
#include <nds/nds_r2_collision_oracle.h>

extern DObj *gcGetTreeDObjNext(DObj *dobj);

volatile u32 gNdsR2CollisionOracleSamples;
volatile u32 gNdsR2CollisionOracleSingular;
volatile u32 gNdsR2CollisionOracleMaxDevQ12[NDS_R2_COLLISION_ORACLE_BUCKETS];
volatile u32 gNdsR2CollisionOracleOverBoundCount;
volatile u32 gNdsR2CollisionOracleScaleMinQ12 = 0xffffffffu;
volatile u32 gNdsR2CollisionOracleScaleMaxQ12;

static s32 ndsR2CollisionOracleToQ12(f32 value)
{
    /* Round-half-away-from-zero, matching ndsR2CollisionRoundShift so the two
     * representations agree on the boundary rather than differing by a unit
     * depending on which produced the value. */
    f32 scaled = value * (f32)NDS_R2_COLLISION_MTX_ONE;

    return (s32)((scaled < 0.0f) ? (scaled - 0.5f) : (scaled + 0.5f));
}

static u32 ndsR2CollisionOracleAbsQ12(f32 reference, s32 candidate_q12)
{
    s32 delta = ndsR2CollisionOracleToQ12(reference) - candidate_q12;

    return (u32)((delta < 0) ? -delta : delta);
}

static void ndsR2CollisionOracleSamplePart(FTParts *parts)
{
    static const s32 offsets[NDS_R2_COLLISION_ORACLE_BUCKETS] = {
        NDS_R2_COLLISION_ORACLE_OFFSET_0,
        NDS_R2_COLLISION_ORACLE_OFFSET_1,
        NDS_R2_COLLISION_ORACLE_OFFSET_2,
    };
    NDSR2CollisionMtx source;
    NDSR2CollisionMtx frame;
    u32 bucket;
    u32 row;
    u32 col;

    for (row = 0u; row < 4u; row++)
    {
        for (col = 0u; col < 3u; col++)
        {
            source.m[row][col] =
                ndsR2CollisionOracleToQ12(parts->mtx_translate[row][col]);
        }
    }
    if (ndsR2CollisionInvertFrame(&frame, &source) == 0)
    {
        gNdsR2CollisionOracleSingular++;
        return;
    }
    gNdsR2CollisionOracleSamples++;

    /* The domain question, free: func_ovl2_800EDE5C already computed the row
     * magnitudes into vec_scale, and its latch is checked by the caller. */
    {
        f32 scales[3];
        u32 i;

        scales[0] = parts->vec_scale.x;
        scales[1] = parts->vec_scale.y;
        scales[2] = parts->vec_scale.z;
        for (i = 0u; i < 3u; i++)
        {
            s32 q12 = ndsR2CollisionOracleToQ12(scales[i]);

            if (q12 <= 0)
            {
                continue;
            }
            if ((u32)q12 < gNdsR2CollisionOracleScaleMinQ12)
            {
                gNdsR2CollisionOracleScaleMinQ12 = (u32)q12;
            }
            if ((u32)q12 > gNdsR2CollisionOracleScaleMaxQ12)
            {
                gNdsR2CollisionOracleScaleMaxQ12 = (u32)q12;
            }
        }
    }

    for (bucket = 0u; bucket < NDS_R2_COLLISION_ORACLE_BUCKETS; bucket++)
    {
        u32 axis;

        /* One probe per axis at this distance. The joint origin itself is not
         * probed: (p - t) is then exactly zero and the multiply that carries
         * all of the error never happens, so it would report a flattering
         * zero for every joint. */
        for (axis = 0u; axis < 3u; axis++)
        {
            Vec3f probe;
            Vec3f reference;
            s32 point[3];
            s32 local[3];
            u32 component;
            u32 worst = 0u;

            probe.x = parts->mtx_translate[3][0];
            probe.y = parts->mtx_translate[3][1];
            probe.z = parts->mtx_translate[3][2];
            switch (axis)
            {
            case 0u: probe.x += (f32)offsets[bucket]; break;
            case 1u: probe.y += (f32)offsets[bucket]; break;
            default: probe.z += (f32)offsets[bucket]; break;
            }
            point[0] = ndsR2CollisionOracleToQ12(probe.x);
            point[1] = ndsR2CollisionOracleToQ12(probe.y);
            point[2] = ndsR2CollisionOracleToQ12(probe.z);

            reference = probe;
            gmCollisionGetWorldPosition(parts->unk_dobjtrans_0x9C, &reference);
            ndsR2CollisionWorldToLocal(local, &frame, point);

            worst = ndsR2CollisionOracleAbsQ12(reference.x, local[0]);
            component = ndsR2CollisionOracleAbsQ12(reference.y, local[1]);
            if (component > worst) { worst = component; }
            component = ndsR2CollisionOracleAbsQ12(reference.z, local[2]);
            if (component > worst) { worst = component; }

            if (worst > gNdsR2CollisionOracleMaxDevQ12[bucket])
            {
                gNdsR2CollisionOracleMaxDevQ12[bucket] = worst;
            }
            if (worst > NDS_R2_COLLISION_ORACLE_BOUND_Q12)
            {
                gNdsR2CollisionOracleOverBoundCount++;
            }
        }
    }
}

/* Walk the LIVE fighter tree, not the port's parts pool. The first draft walked
 * sNdsFighterPartsPool because the board recorded that ftGetParts resolves to
 * it; the linker map says that pool is not linked at all in the shipping-shaped
 * build, and referencing it costs 33,152 bytes -- eight arena steps, enough by
 * itself to put the battle under the 25 KiB GObj latch. The DObj tree is where
 * the FTParts collision actually reads live, in every configuration, and it
 * costs no storage. */
void ndsR2CollisionOracleSampleFrame(void)
{
    GObj *fighter_gobj = gGCCommonLinks[nGCCommonLinkIDFighter];

    while (fighter_gobj != NULL)
    {
        DObj *dobj;

        for (dobj = DObjGetStruct(fighter_gobj); dobj != NULL;
             dobj = gcGetTreeDObjNext(dobj))
        {
            FTParts *parts = ftGetParts(dobj);

            /* 0x7 is the invert latch and 0x6 the scale latch. Requiring both
             * means the part was inverted by collision THIS frame and its
             * vec_scale is live, so every sample is a joint the hit path
             * actually used -- not a stale entry, and not a joint the float
             * path never touched. */
            if ((parts == NULL) || (parts->unk_dobjtrans_0x7 == 0) ||
                (parts->unk_dobjtrans_0x6 == 0))
            {
                continue;
            }
            ndsR2CollisionOracleSamplePart(parts);
        }
        fighter_gobj = fighter_gobj->link_next;
    }
}
#endif
