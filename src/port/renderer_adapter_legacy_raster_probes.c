typedef struct NDSFighterDLMultiDrawCollection {
    DObj *dobjs[NDS_FIGHTER_DL_MULTI_DRAW_MAX_SELECTED];
    u32 indices[NDS_FIGHTER_DL_MULTI_DRAW_MAX_SELECTED];
    u32 total_count;
    u32 selected_count;
    u32 selected_index_mask;
} NDSFighterDLMultiDrawCollection;










typedef struct NDSFighterDLAllDrawCollection {
    DObj *dobjs[NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED];
    u32 indices[NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED];
    u32 total_count;
    u32 selected_count;
    u32 selected_index_mask;
} NDSFighterDLAllDrawCollection;

#if NDS_TICK_HUD
/* CYCLE 98 -- does the DObj walk rebuild the same collection every frame?
 *
 * The board's seam table calls the walk "match-load constant for Mario/Fox --
 * poses move, topology does not", and flags that it needs proof no status or
 * motion alters the DObj set. This is that proof, and it costs one u32 of state
 * per slot: hash the collection's identity and compare it with the previous
 * frame's hash for the same slot.
 *
 * The dl pointer is in the hash deliberately. A collection whose DObj set is
 * unchanged but whose display lists have been re-pointed is NOT a constant for
 * any purpose a baked collection order would serve, and hashing only the DObj
 * pointers would report it as one. */
static u32 sNdsFtrPreWalkHash[GMCOMMON_PLAYERS_MAX];
static u32 sNdsFtrPreWalkSeen[GMCOMMON_PLAYERS_MAX];

static void ndsFtrPreWalkCensus(
    u32 slot, const NDSFighterDLAllDrawCollection *collection)
{
    u32 hash = 2166136261u;
    u32 i;

    if ((slot >= GMCOMMON_PLAYERS_MAX) || (collection == NULL))
    {
        return;
    }
#define NDS_FTR_PRE_MIX(v) \
    do { hash ^= (u32)(v); hash *= 16777619u; } while (0)
    NDS_FTR_PRE_MIX(collection->total_count);
    NDS_FTR_PRE_MIX(collection->selected_count);
    NDS_FTR_PRE_MIX(collection->selected_index_mask);
    for (i = 0u; (i < collection->selected_count) &&
                 (i < NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED); i++)
    {
        const DObj *dobj = collection->dobjs[i];

        NDS_FTR_PRE_MIX(collection->indices[i]);
        NDS_FTR_PRE_MIX((uintptr_t)dobj);
        NDS_FTR_PRE_MIX((dobj != NULL) ? (uintptr_t)dobj->dl : 0u);
    }
#undef NDS_FTR_PRE_MIX
    if (sNdsFtrPreWalkSeen[slot] == 0u)
    {
        gNdsFtrPreWalkFirst++;
    }
    else if (sNdsFtrPreWalkHash[slot] == hash)
    {
        gNdsFtrPreWalkSame++;
    }
    else
    {
        gNdsFtrPreWalkVariant++;
    }
    sNdsFtrPreWalkHash[slot] = hash;
    sNdsFtrPreWalkSeen[slot] = 1u;
}
#endif

#define NDS_FIGHTER_DL_ALL_FAIL_BLOCKER 0x1u
#define NDS_FIGHTER_DL_ALL_FAIL_UNSUPPORTED_OPCODE 0x2u
#define NDS_FIGHTER_DL_ALL_FAIL_UNSUPPORTED_COUNT 0x4u
#define NDS_FIGHTER_DL_ALL_FAIL_VERTEX_RANGE 0x8u
#define NDS_FIGHTER_DL_ALL_FAIL_NO_VERTS 0x10u
#define NDS_FIGHTER_DL_ALL_FAIL_NO_VALID_TRIS 0x20u
#define NDS_FIGHTER_DL_ALL_FAIL_UNKNOWN 0x80000000u

static void ndsFighterCollectAllDObjsWithDLRecursive(
    DObj *dobj, NDSFighterDLAllDrawCollection *collection,
    u32 *traversal_index)
{
    while (dobj != NULL)
    {
        u32 current_index = (traversal_index != NULL) ? *traversal_index : 0u;

        if (traversal_index != NULL)
        {
            (*traversal_index)++;
        }

        if ((collection != NULL) && (dobj->dl != NULL))
        {
            if (collection->selected_count <
                NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED)
            {
                u32 selected = collection->selected_count;
                collection->dobjs[selected] = dobj;
                collection->indices[selected] = current_index;
                collection->selected_count++;
                if (current_index < 32u)
                {
                    collection->selected_index_mask |= 1u << current_index;
                }
            }
            collection->total_count++;
        }

        ndsFighterCollectAllDObjsWithDLRecursive(dobj->child,
                                                 collection,
                                                 traversal_index);
        dobj = dobj->sib_next;
    }
}

/* Four pointers, 16 bytes, and nothing else.
 *
 * This used to carry the render preamble too -- geometry mode, cycle type,
 * render mode, prim and env colour, the Light and its valid flag -- which made
 * it 56 bytes and put its two halves on different cache lines. The halves have
 * opposite access patterns: the pointers are read by three tight per-root loops
 * in the same pass that walks the collection, while the preamble is read once
 * per root by ndsRendererAdapterBuildNativeProductionInputs, a pass later,
 * after the matrix and material work has evicted it. c106 priced that eviction
 * at 2,966 ticks/frame on `root->preamble.geometry_mode = event->geometry_mode`
 * and 1,759 on `if (event->light_valid)`: ~110 cycles an event of pure miss.
 *
 * So the preamble moved to its own array, in the consumer's own struct layout,
 * written by the producer. 32 events is 512 bytes of pointers plus 768 of
 * preamble instead of 1,792 bytes interleaved, and the consumer's read is one
 * 24-byte struct copy out of a dense array rather than a field-by-field gather
 * across two cold lines. */
