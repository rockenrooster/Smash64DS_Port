#include <stddef.h>
#include <stdint.h>

#include <ef/effect.h>
#include <ft/fighter.h>
#include <gm/generic.h>
#include <gr/ground.h>
#include <it/item.h>
#include <nds/nds_task9_state_hash.h>
#include <sc/scene.h>
#include <sys/controller.h>
#include <sys/obj.h>
#include <sys/objman.h>
#include <sys/taskman.h>
#include <wp/weapon.h>

#if NDS_TASK9_STATE_HASH

#define NDS_TASK9_STATE_MAX_GOBJS 256u
#define NDS_TASK9_STATE_MAX_PROCESSES 32u
#define NDS_TASK9_STATE_MAX_DOBJS 256u
#define NDS_TASK9_STATE_MAX_SOBJS 256u
#define NDS_TASK9_STATE_MAX_COBJS 4u
#define NDS_TASK9_STATE_MAX_MOBJS 16u
#define NDS_TASK9_STATE_MAX_AOBJS 96u

enum NDSTask9StateOverflow
{
    NDS_TASK9_STATE_OVERFLOW_GOBJ = 1u << 0,
    NDS_TASK9_STATE_OVERFLOW_PROCESS = 1u << 1,
    NDS_TASK9_STATE_OVERFLOW_DOBJ = 1u << 2,
    NDS_TASK9_STATE_OVERFLOW_SOBJ = 1u << 3,
    NDS_TASK9_STATE_OVERFLOW_COBJ = 1u << 4,
    NDS_TASK9_STATE_OVERFLOW_MOBJ = 1u << 5,
    NDS_TASK9_STATE_OVERFLOW_AOBJ = 1u << 6,
    NDS_TASK9_STATE_OVERFLOW_POINTER = 1u << 7,
    NDS_TASK9_STATE_OVERFLOW_CAPTURE = 1u << 8
};

enum NDSTask9StateRecordKind
{
    NDS_TASK9_STATE_RECORD_SCALARS = 1u,
    NDS_TASK9_STATE_RECORD_SCENE,
    NDS_TASK9_STATE_RECORD_BATTLE,
    NDS_TASK9_STATE_RECORD_CAMERA,
    NDS_TASK9_STATE_RECORD_GROUND,
    NDS_TASK9_STATE_RECORD_CONTROLLERS,
    NDS_TASK9_STATE_RECORD_COLLISION_BOUNDS,
    NDS_TASK9_STATE_RECORD_COLLISION_SPEEDS,
    NDS_TASK9_STATE_RECORD_GOBJ_LINK,
    NDS_TASK9_STATE_RECORD_GOBJ,
    NDS_TASK9_STATE_RECORD_PROCESS,
    NDS_TASK9_STATE_RECORD_DOBJ,
    NDS_TASK9_STATE_RECORD_XOBJ,
    NDS_TASK9_STATE_RECORD_AOBJ,
    NDS_TASK9_STATE_RECORD_MOBJ,
    NDS_TASK9_STATE_RECORD_SOBJ,
    NDS_TASK9_STATE_RECORD_COBJ,
    NDS_TASK9_STATE_RECORD_FIGHTER,
    NDS_TASK9_STATE_RECORD_ITEM,
    NDS_TASK9_STATE_RECORD_WEAPON,
    NDS_TASK9_STATE_RECORD_EFFECT,
    NDS_TASK9_STATE_RECORD_TRANSFORM
};

typedef struct NDSTask9StateHashContext
{
    u32 hash1;
    u32 hash2;
    u32 bytes;
    u32 records;
    u32 overflow;
} NDSTask9StateHashContext;

volatile u32 gNdsTask9StateHashArmed;
volatile u32 gNdsTask9StateHashCount;
volatile u32 gNdsTask9StateHashOverflow;
volatile NDSTask9StateHashRecord
    gNdsTask9StateHashes[NDS_TASK9_STATE_HASH_MAX_UPDATES];

extern s32 syUtilsRandSeed(void);

static sb32 ndsTask9StateRangeContains(const void *start, const void *end,
                                       uintptr_t value, size_t bytes)
{
    uintptr_t first = (uintptr_t)start;
    uintptr_t last = (uintptr_t)end;

    return (first != 0u) && (last >= first) && (value >= first) &&
           (value <= last) &&
           (bytes <= (size_t)(last - value));
}

static sb32 ndsTask9StateGeneralContains(const void *ptr, size_t bytes)
{
    return ndsTask9StateRangeContains(gSYTaskmanGeneralHeap.start,
                                      gSYTaskmanGeneralHeap.ptr,
                                      (uintptr_t)ptr, bytes);
}

static u32 ndsTask9StateCanonicalWord(u32 word)
{
    uintptr_t value = (uintptr_t)(word & ~1u);
    uintptr_t start;

    if (word == 0u)
    {
        return 0u;
    }
    if (ndsTask9StateRangeContains(gSYTaskmanGeneralHeap.start,
                                   gSYTaskmanGeneralHeap.ptr, value, 1u))
    {
        start = (uintptr_t)gSYTaskmanGeneralHeap.start;
        return 0x10000000u | (u32)(value - start) | (word & 1u);
    }
    if (ndsTask9StateRangeContains(gSYTaskmanGraphicsHeap.start,
                                   gSYTaskmanGraphicsHeap.end, value, 1u))
    {
        start = (uintptr_t)gSYTaskmanGraphicsHeap.start;
        return 0x18000000u | (u32)(value - start) | (word & 1u);
    }
    if ((value >= 0x02000000u) && (value < 0x02400000u))
    {
        /* Link addresses differ when unchanged libgcc code moves to ITCM.
         * Static code/data pointers are identity-neutral here; the separate
         * object-byte checker proves the selected machine code itself. */
        return 0x20000000u | (word & 1u);
    }
    if ((value >= 0x01ff8000u) && (value < 0x02000000u))
    {
        return 0x30000000u | (word & 1u);
    }
    if ((value >= 0x02ff0000u) && (value < 0x03000000u))
    {
        return 0x40000000u | (word & 1u);
    }
    if ((value >= 0x06000000u) && (value < 0x07000000u))
    {
        return 0x50000000u | (u32)(value - 0x06000000u) | (word & 1u);
    }
    return word;
}

static void ndsTask9StateMixWord(NDSTask9StateHashContext *ctx, u32 value)
{
    ctx->hash1 ^= value;
    ctx->hash1 *= 16777619u;

    ctx->hash2 += value + 0x9e3779b9u;
    ctx->hash2 += ctx->hash2 << 10;
    ctx->hash2 ^= ctx->hash2 >> 6;
}

/* Lab-only region filter for the Task 37 investigation.
 *
 * Every Task 37 arm that changes execution speed diverges in exactly the same
 * 692 of 3,892 records, while arms that change only layout pass. Guessing at
 * the responsible region one hypothesis at a time has now cost several full
 * runs (BGM, controllers) and found nothing, so this makes the instrument name
 * the region instead: include only the record kinds selected by the mask, and
 * whichever half of the state carries the divergence identifies itself.
 *
 * Bit N corresponds to NDSTask9StateRecordKind value N. Default is every kind.
 * Never set in a shipping or verification configuration -- a filtered hash is a
 * strictly weaker gate. */
#ifndef NDS_TASK9_STATE_HASH_REGION_MASK
#define NDS_TASK9_STATE_HASH_REGION_MASK 0xFFFFFFFFu
#endif

/* Lab-only raw FTStruct snapshot for the Task 37 investigation.
 *
 * The region bisect localized the divergence to FTStruct and then ran out of
 * resolution: FTStruct is hashed as one 3,012-byte blob, so "the fighter region
 * differs" cannot separate a differing position from a differing pointer. This
 * captures the raw bytes of both fighters so the differing offset names itself,
 * and offsets map back to members through the NDS_FTSTRUCT_OFF_* table in
 * <ft/fighter.h>.
 *
 * Two consecutive updates are captured: slot 0 is UPDATE-1, slot 1 is UPDATE.
 * Capturing the update *before* the first divergence is the point -- if slot 0
 * is byte-identical across the two builds and slot 1 differs at offset X, then
 * the state entering the divergence was identical and X is the origin rather
 * than a downstream consequence of something that already drifted.
 *
 * Never set in a shipping or verification configuration. */
#ifndef NDS_TASK9_FTSTRUCT_SNAPSHOT
#define NDS_TASK9_FTSTRUCT_SNAPSHOT 0
#endif
#ifndef NDS_TASK9_FTSTRUCT_SNAPSHOT_UPDATE
#define NDS_TASK9_FTSTRUCT_SNAPSHOT_UPDATE 0u
#endif

#if NDS_TASK9_FTSTRUCT_SNAPSHOT
#define NDS_TASK9_FTSTRUCT_SNAPSHOT_SLOTS 2u
#define NDS_TASK9_FTSTRUCT_SNAPSHOT_FIGHTERS 2u

/* [slot][fighter][byte]. Fighter index is GObj traversal order, which is the
 * same order the hash itself walks, so it matches between the two builds for
 * the same reason the record sequence does. */
volatile u8 gNdsTask9FTStructSnapshot[NDS_TASK9_FTSTRUCT_SNAPSHOT_SLOTS]
                                     [NDS_TASK9_FTSTRUCT_SNAPSHOT_FIGHTERS]
                                     [NDS_FTSTRUCT_LAYOUT_SIZE];
/* Per-slot fighters captured, so a short capture is visible in the dump
 * instead of being misread as a run of legitimately zero bytes. */
volatile u32 gNdsTask9FTStructSnapshotFilled[NDS_TASK9_FTSTRUCT_SNAPSHOT_SLOTS];

static void ndsTask9FTStructSnapshotCapture(const void *fighter)
{
    u32 update = gNdsTask9StateHashCount;
    const u8 *src = fighter;
    size_t offset;
    u32 slot;
    u32 which;

    if (update == ((u32)NDS_TASK9_FTSTRUCT_SNAPSHOT_UPDATE - 1u))
    {
        slot = 0u;
    }
    else if (update == (u32)NDS_TASK9_FTSTRUCT_SNAPSHOT_UPDATE)
    {
        slot = 1u;
    }
    else
    {
        return;
    }

    which = gNdsTask9FTStructSnapshotFilled[slot];
    if (which >= NDS_TASK9_FTSTRUCT_SNAPSHOT_FIGHTERS)
    {
        return;
    }
    /* Deliberately a byte loop and not memcpy: memcpy is one of the three libc
     * leaves Task 37 relocates, so calling it here would make the instrument
     * itself a function of the change under test. */
    for (offset = 0u; offset < (size_t)NDS_FTSTRUCT_LAYOUT_SIZE; offset++)
    {
        gNdsTask9FTStructSnapshot[slot][which][offset] = src[offset];
    }
    gNdsTask9FTStructSnapshotFilled[slot] = which + 1u;
}
#endif

static void ndsTask9StateHashBytes(NDSTask9StateHashContext *ctx, u32 kind,
                                   const void *data, size_t bytes)
{
    const u8 *src = data;
    size_t offset;

    if ((kind < 32u) &&
        (((u32)NDS_TASK9_STATE_HASH_REGION_MASK & (1u << kind)) == 0u))
    {
        return;
    }

    ndsTask9StateMixWord(ctx, 0x53544154u ^ kind);
    ndsTask9StateMixWord(ctx, (u32)bytes);
    for (offset = 0u; offset < bytes; offset += sizeof(u32))
    {
        u32 word = 0u;
        size_t lane;

        for (lane = 0u; (lane < sizeof(u32)) &&
                        ((offset + lane) < bytes); lane++)
        {
            word |= (u32)src[offset + lane] << (lane * 8u);
        }
        word = ndsTask9StateCanonicalWord(word);
        ndsTask9StateMixWord(ctx, word);
    }
    ctx->bytes += (u32)bytes;
    ctx->records++;
}

static sb32 ndsTask9StateHashGeneral(NDSTask9StateHashContext *ctx, u32 kind,
                                     const void *ptr, size_t bytes)
{
    if ((ptr == NULL) || !ndsTask9StateGeneralContains(ptr, bytes))
    {
        ctx->overflow |= NDS_TASK9_STATE_OVERFLOW_POINTER;
        return FALSE;
    }
    ndsTask9StateHashBytes(ctx, kind, ptr, bytes);
    return TRUE;
}

static void ndsTask9StateHashAObjs(NDSTask9StateHashContext *ctx, AObj *aobj)
{
    u32 count = 0u;

    while (aobj != NULL)
    {
        AObj *next;

        if ((count++ >= NDS_TASK9_STATE_MAX_AOBJS) ||
            !ndsTask9StateGeneralContains(aobj, sizeof(*aobj)))
        {
            ctx->overflow |= NDS_TASK9_STATE_OVERFLOW_AOBJ;
            return;
        }
        next = aobj->next;
        ndsTask9StateHashBytes(ctx, NDS_TASK9_STATE_RECORD_AOBJ,
                               aobj, sizeof(*aobj));
        aobj = next;
    }
}

static void ndsTask9StateHashMObjs(NDSTask9StateHashContext *ctx, MObj *mobj)
{
    u32 count = 0u;

    while (mobj != NULL)
    {
        MObj *next;

        if ((count++ >= NDS_TASK9_STATE_MAX_MOBJS) ||
            !ndsTask9StateGeneralContains(mobj, sizeof(*mobj)))
        {
            ctx->overflow |= NDS_TASK9_STATE_OVERFLOW_MOBJ;
            return;
        }
        next = mobj->next;
        ndsTask9StateHashBytes(ctx, NDS_TASK9_STATE_RECORD_MOBJ,
                               mobj, sizeof(*mobj));
        ndsTask9StateHashAObjs(ctx, mobj->aobj);
        mobj = next;
    }
}

static void ndsTask9StateHashDObjs(NDSTask9StateHashContext *ctx, DObj *dobj)
{
    u32 count = 0u;

    while (dobj != NULL)
    {
        DObj *next;
        u32 xobj_count;
        u32 i;

        if ((count++ >= NDS_TASK9_STATE_MAX_DOBJS) ||
            !ndsTask9StateGeneralContains(dobj, sizeof(*dobj)))
        {
            ctx->overflow |= NDS_TASK9_STATE_OVERFLOW_DOBJ;
            return;
        }
        next = gcGetTreeDObjNext(dobj);
        ndsTask9StateHashBytes(ctx, NDS_TASK9_STATE_RECORD_DOBJ,
                               dobj, sizeof(*dobj));
        xobj_count = (dobj->xobjs_num <= 5u) ? dobj->xobjs_num : 5u;
        for (i = 0u; i < xobj_count; i++)
        {
            if (dobj->xobjs[i] != NULL)
            {
                ndsTask9StateHashGeneral(ctx, NDS_TASK9_STATE_RECORD_XOBJ,
                                         dobj->xobjs[i],
                                         sizeof(*dobj->xobjs[i]));
            }
        }
        ndsTask9StateHashAObjs(ctx, dobj->aobj);
        ndsTask9StateHashMObjs(ctx, dobj->mobj);
        dobj = next;
    }
}

static void ndsTask9StateHashSObjs(NDSTask9StateHashContext *ctx, SObj *sobj)
{
    u32 count = 0u;

    while (sobj != NULL)
    {
        SObj *next;

        if ((count++ >= NDS_TASK9_STATE_MAX_SOBJS) ||
            !ndsTask9StateGeneralContains(sobj, sizeof(*sobj)))
        {
            ctx->overflow |= NDS_TASK9_STATE_OVERFLOW_SOBJ;
            return;
        }
        next = sobj->next;
        ndsTask9StateHashBytes(ctx, NDS_TASK9_STATE_RECORD_SOBJ,
                               sobj, sizeof(*sobj));
        sobj = next;
    }
}

static void ndsTask9StateHashCObjs(NDSTask9StateHashContext *ctx, CObj *cobj)
{
    u32 count = 0u;

    while (cobj != NULL)
    {
        CObj *next;
        u32 xobj_count;
        u32 i;

        if ((count++ >= NDS_TASK9_STATE_MAX_COBJS) ||
            !ndsTask9StateGeneralContains(cobj, sizeof(*cobj)))
        {
            ctx->overflow |= NDS_TASK9_STATE_OVERFLOW_COBJ;
            return;
        }
        next = cobj->next;
        ndsTask9StateHashBytes(ctx, NDS_TASK9_STATE_RECORD_COBJ,
                               cobj, sizeof(*cobj));
        xobj_count = (cobj->xobjs_num <= 2) ? (u32)cobj->xobjs_num : 2u;
        for (i = 0u; i < xobj_count; i++)
        {
            if (cobj->xobjs[i] != NULL)
            {
                ndsTask9StateHashGeneral(ctx, NDS_TASK9_STATE_RECORD_XOBJ,
                                         cobj->xobjs[i],
                                         sizeof(*cobj->xobjs[i]));
            }
        }
        ndsTask9StateHashAObjs(ctx, cobj->aobj);
        cobj = next;
    }
}

static void ndsTask9StateHashProcesses(NDSTask9StateHashContext *ctx,
                                       GObjProcess *process)
{
    u32 count = 0u;

    while (process != NULL)
    {
        GObjProcess *next;

        if ((count++ >= NDS_TASK9_STATE_MAX_PROCESSES) ||
            !ndsTask9StateGeneralContains(process, sizeof(*process)))
        {
            ctx->overflow |= NDS_TASK9_STATE_OVERFLOW_PROCESS;
            return;
        }
        next = process->link_next;
        ndsTask9StateHashBytes(ctx, NDS_TASK9_STATE_RECORD_PROCESS,
                               process, sizeof(*process));
        process = next;
    }
}

static void ndsTask9StateHashUserData(NDSTask9StateHashContext *ctx,
                                      u32 link, GObj *gobj)
{
    void *user_data = gobj->user_data.p;

    if (user_data == NULL)
    {
        return;
    }
    switch (link)
    {
    case nGCCommonLinkIDFighter:
#if NDS_TASK9_FTSTRUCT_SNAPSHOT
        ndsTask9FTStructSnapshotCapture(user_data);
#endif
        ndsTask9StateHashGeneral(ctx, NDS_TASK9_STATE_RECORD_FIGHTER,
                                 user_data, sizeof(FTStruct));
        break;
    case nGCCommonLinkIDItem:
        ndsTask9StateHashGeneral(ctx, NDS_TASK9_STATE_RECORD_ITEM,
                                 user_data, sizeof(ITStruct));
        break;
    case nGCCommonLinkIDWeapon:
        ndsTask9StateHashGeneral(ctx, NDS_TASK9_STATE_RECORD_WEAPON,
                                 user_data, sizeof(WPStruct));
        break;
    case nGCCommonLinkIDEffect:
    case nGCCommonLinkIDSpecialEffect:
        if (ndsTask9StateHashGeneral(ctx, NDS_TASK9_STATE_RECORD_EFFECT,
                                     user_data, sizeof(EFStruct)))
        {
            EFStruct *effect = user_data;

            if (effect->xf != NULL)
            {
                ndsTask9StateHashGeneral(ctx,
                                         NDS_TASK9_STATE_RECORD_TRANSFORM,
                                         effect->xf, sizeof(*effect->xf));
            }
        }
        break;
    default:
        break;
    }
}

static void ndsTask9StateHashGObjs(NDSTask9StateHashContext *ctx)
{
    u32 link;
    u32 total = 0u;

    for (link = 0u; link < GC_COMMON_MAX_LINKS; link++)
    {
        GObj *gobj = gGCCommonLinks[link];

        ndsTask9StateHashBytes(ctx, NDS_TASK9_STATE_RECORD_GOBJ_LINK,
                               &link, sizeof(link));
        while (gobj != NULL)
        {
            GObj *next;

            if ((total++ >= NDS_TASK9_STATE_MAX_GOBJS) ||
                !ndsTask9StateGeneralContains(gobj, sizeof(*gobj)))
            {
                ctx->overflow |= NDS_TASK9_STATE_OVERFLOW_GOBJ;
                return;
            }
            next = gobj->link_next;
            ndsTask9StateHashBytes(ctx, NDS_TASK9_STATE_RECORD_GOBJ,
                                   gobj, sizeof(*gobj));
            ndsTask9StateHashProcesses(ctx, gobj->gobjproc_head);
            switch (gobj->obj_kind)
            {
            case nGCCommonAppendDObj:
                ndsTask9StateHashDObjs(ctx, DObjGetStruct(gobj));
                break;
            case nGCCommonAppendSObj:
                ndsTask9StateHashSObjs(ctx, SObjGetStruct(gobj));
                break;
            case nGCCommonAppendCamera:
                ndsTask9StateHashCObjs(ctx, CObjGetStruct(gobj));
                break;
            default:
                break;
            }
            ndsTask9StateHashUserData(ctx, link, gobj);
            gobj = next;
        }
    }
}

void ndsTask9StateHashRecordUpdate(void)
{
    NDSTask9StateHashContext ctx = {
        2166136261u, 0x6a09e667u, 0u, 0u, 0u
    };
    u32 scalars[3];
    u32 index;

    if (gNdsTask9StateHashArmed == 0u)
    {
        return;
    }
    index = gNdsTask9StateHashCount;
    if (index >= NDS_TASK9_STATE_HASH_MAX_UPDATES)
    {
        gNdsTask9StateHashOverflow |= NDS_TASK9_STATE_OVERFLOW_CAPTURE;
        return;
    }

    /* Absolute scheduler counters and hardware timer ticks are execution
     * history, not match-local game state.  The record index aligns updates;
     * the RNG and live state below cover gameplay determinism. */
    scalars[0] = (u32)syUtilsRandSeed();
    scalars[1] = (u32)((uintptr_t)gSYTaskmanGeneralHeap.ptr -
                       (uintptr_t)gSYTaskmanGeneralHeap.start);
    scalars[2] = index;
    ndsTask9StateHashBytes(&ctx, NDS_TASK9_STATE_RECORD_SCALARS,
                           scalars, sizeof(scalars));
    ndsTask9StateHashBytes(&ctx, NDS_TASK9_STATE_RECORD_SCENE,
                           &gSCManagerSceneData,
                           sizeof(gSCManagerSceneData));
    if (gSCManagerBattleState != NULL)
    {
        ndsTask9StateHashBytes(&ctx, NDS_TASK9_STATE_RECORD_BATTLE,
                               gSCManagerBattleState,
                               sizeof(*gSCManagerBattleState));
    }
    else
    {
        ctx.overflow |= NDS_TASK9_STATE_OVERFLOW_POINTER;
    }
    ndsTask9StateHashBytes(&ctx, NDS_TASK9_STATE_RECORD_CAMERA,
                           &gGMCameraStruct, sizeof(gGMCameraStruct));
    ndsTask9StateHashBytes(&ctx, NDS_TASK9_STATE_RECORD_GROUND,
                           &gGRCommonStruct, sizeof(gGRCommonStruct));
#if !NDS_TASK9_STATE_HASH_SKIP_CONTROLLERS
    /* Lab-only exclusion for Task 37. Under NDS_HARNESS_FAST_LOGIC the ARM9 runs
     * unpaced while the ARM7 owns input on real time, so a build that executes
     * faster samples these devices at a different phase. That makes this record
     * a function of execution speed, which is precisely what a placement change
     * alters -- so with it included the gate cannot referee one. Never set in a
     * shipping or verification configuration; it weakens the gate. */
    ndsTask9StateHashBytes(&ctx, NDS_TASK9_STATE_RECORD_CONTROLLERS,
                           gSYControllerDevices,
                           sizeof(gSYControllerDevices));
#endif
    ndsTask9StateHashBytes(&ctx, NDS_TASK9_STATE_RECORD_COLLISION_BOUNDS,
                           &gMPCollisionBounds, sizeof(gMPCollisionBounds));
    if ((gMPCollisionSpeeds != NULL) &&
        (gMPCollisionYakumonosNum > 0) &&
        (gMPCollisionYakumonosNum <= 64))
    {
        /* The DS backend owns this as a fixed static array, not a taskman
         * allocation.  Hash the live bytes directly; treating it as a
         * general-heap object falsely reported pointer overflow every frame. */
        ndsTask9StateHashBytes(
            &ctx, NDS_TASK9_STATE_RECORD_COLLISION_SPEEDS,
            gMPCollisionSpeeds,
            sizeof(*gMPCollisionSpeeds) * (size_t)gMPCollisionYakumonosNum);
    }
    ndsTask9StateHashGObjs(&ctx);

    ctx.hash2 += ctx.hash2 << 3;
    ctx.hash2 ^= ctx.hash2 >> 11;
    ctx.hash2 += ctx.hash2 << 15;
    gNdsTask9StateHashes[index].hash1 = ctx.hash1;
    gNdsTask9StateHashes[index].hash2 = ctx.hash2;
    gNdsTask9StateHashes[index].bytes = ctx.bytes;
    gNdsTask9StateHashes[index].records = ctx.records;
    gNdsTask9StateHashes[index].overflow = ctx.overflow;
    gNdsTask9StateHashOverflow |= ctx.overflow;
    gNdsTask9StateHashCount = index + 1u;
}

#endif
