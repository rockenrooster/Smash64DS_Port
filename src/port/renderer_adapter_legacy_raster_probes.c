/* The collections of a fighter's drawable DObjs (every DObj with a list,
 * selected by index) and the recursive walk that fills them. A slice of
 * reloc_backend_renderer_dl.c. */

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
