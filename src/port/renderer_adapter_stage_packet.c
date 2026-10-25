/* The native stage packet: the stage select's 3D preview layers, the
 * topology walk that collects a stage's bindings, their revalidation and
 * world matrices, the once-a-frame owner prep, the material snapshot cache
 * and the packet emit. Included in order by reloc_backend_renderer_dl.c;
 * not a translation unit of its own. */

/* P2-4n1 step 6: every per-segment fact below reads the active stage's
 * capture row (renderer_adapter_matrix.c). Dream Land's rows are the switch,
 * arrays and ternary that used to live here, value for value. */
#if NDS_P2_STAGE_ZEBES
extern void *ndsGRZebesAcidGObj(void);
#endif
#if NDS_P2_STAGE_YAMABUKI
extern void *ndsGRYamabukiGateGObj(void);
#endif
#if NDS_P2_STAGE_ZEBES || NDS_P2_STAGE_YAMABUKI
extern void gcDrawDObjTreeDLLinksForGObj(GObj *gobj);
#endif
#if NDS_P2_STAGE_INISHIE
extern void *ndsGRInishieScaleStringGObj(u32 index);
extern void *ndsGRInishieScalePlatformGObj(u32 index);
#endif

/* THE STAGE SELECT'S 3D PREVIEW (2026-10-12, battleship_mnmaps.c). The
 * preview builds a stage's four display layers the way the battle does
 * (grDisplayMakeGeometryLayer's procs and links, the preview's own scale),
 * and the owner draws them from the same packet: while preview layers are
 * set (sNdsRendererAdapterStagePreviewMode, renderer_adapter_matrix.c), a
 * LAYER row names the preview's GObj and every other row the preview's
 * stand-in for that battle-only actor (built like the battle's, its root
 * hidden: the source preview shows the layers alone). Armed only around the
 * preview camera's gcDrawAll. */
volatile u32 gNdsRendererAdapterStagePreviewArmed;
static GObj *sNdsRendererAdapterStagePreviewLayers[4];
/* [capture source][index]; LAYER rows read the layers above. */
static GObj *sNdsRendererAdapterStagePreviewActors[6][4];

static GObj *ndsRendererAdapterNativeStageSegmentGObj(u32 segment_index)
{
    const NDSRendererAdapterNativeStageCaptureSegment *row =
        ndsRendererAdapterNativeStageCaptureRow(segment_index);

    if (row == NULL)
    {
        return NULL;
    }
    if (sNdsRendererAdapterStagePreviewMode != 0u)
    {
        if (row->index >= 4u)
        {
            return NULL;
        }
        if (row->source == NDS_RENDERER_ADAPTER_STAGE_CAPTURE_LAYER)
        {
            return sNdsRendererAdapterStagePreviewLayers[row->index];
        }
        return (row->source < 6u) ?
            sNdsRendererAdapterStagePreviewActors[row->source][row->index] :
            NULL;
    }
    switch (row->source)
    {
    case NDS_RENDERER_ADAPTER_STAGE_CAPTURE_LAYER:
        return (row->index < 4u) ? gGRCommonLayerGObjs[row->index] : NULL;
    case NDS_RENDERER_ADAPTER_STAGE_CAPTURE_PUPUPU_MAP:
        return (row->index < 4u) ?
            gGRCommonStruct.pupupu.map_gobj[row->index] : NULL;
#if NDS_P2_STAGE_ZEBES
    case NDS_RENDERER_ADAPTER_STAGE_CAPTURE_ZEBES_ACID:
        return (GObj *)ndsGRZebesAcidGObj();
#endif
#if NDS_P2_STAGE_YAMABUKI
    case NDS_RENDERER_ADAPTER_STAGE_CAPTURE_YAMABUKI_GATE:
        return (GObj *)ndsGRYamabukiGateGObj();
#endif
#if NDS_P2_STAGE_INISHIE
    case NDS_RENDERER_ADAPTER_STAGE_CAPTURE_INISHIE_SCALE_TREE:
        return (GObj *)ndsGRInishieScaleStringGObj(0u);
    case NDS_RENDERER_ADAPTER_STAGE_CAPTURE_INISHIE_SCALE_PLATFORM:
        return (GObj *)ndsGRInishieScalePlatformGObj(row->index);
#endif
    default:
        return NULL;
    }
}

static u32 ndsRendererAdapterNativeStageSegmentLink(u32 segment_index)
{
    const NDSRendererAdapterNativeStageCaptureSegment *row =
        ndsRendererAdapterNativeStageCaptureRow(segment_index);

    return (row != NULL) ? row->link : 0xffu;
}

static sb32 ndsRendererAdapterNativeStageProcMatches(
    u32 segment_index, GObj *gobj)
{
    if ((gobj == NULL) || (gobj->proc_display == NULL))
    {
        return FALSE;
    }
    {
        static void (*const layer_procs[4])(GObj *) = {
            grDisplayLayer0PriProcDisplay, grDisplayLayer1PriProcDisplay,
            grDisplayLayer2PriProcDisplay, grDisplayLayer3PriProcDisplay
        };
        static void (*const layer_procs_sec[4])(GObj *) = {
            grDisplayLayer0SecProcDisplay, grDisplayLayer1SecProcDisplay,
            grDisplayLayer2SecProcDisplay, grDisplayLayer3SecProcDisplay
        };
        const NDSRendererAdapterNativeStageCaptureSegment *row =
            ndsRendererAdapterNativeStageCaptureRow(segment_index);

#if NDS_P2_STAGE_ZEBES
        if ((row != NULL) &&
            (row->source == NDS_RENDERER_ADAPTER_STAGE_CAPTURE_ZEBES_ACID))
        {
            return (gobj->proc_display == gcDrawDObjTreeDLLinksForGObj) ? TRUE : FALSE;
        }
#endif
#if NDS_P2_STAGE_YAMABUKI
        if ((row != NULL) &&
            (row->source == NDS_RENDERER_ADAPTER_STAGE_CAPTURE_YAMABUKI_GATE))
        {
            return (gobj->proc_display == gcDrawDObjTreeDLLinksForGObj) ? TRUE : FALSE;
        }
#endif
#if NDS_P2_STAGE_INISHIE
        if ((row != NULL) &&
            (row->source == NDS_RENDERER_ADAPTER_STAGE_CAPTURE_INISHIE_SCALE_TREE))
        {
            return (gobj->proc_display == gcDrawDObjTreeForGObj) ? TRUE : FALSE;
        }
        if ((row != NULL) &&
            (row->source == NDS_RENDERER_ADAPTER_STAGE_CAPTURE_INISHIE_SCALE_PLATFORM))
        {
            return (gobj->proc_display == gcDrawDObjDLHead0) ? TRUE : FALSE;
        }
#endif
        if ((row == NULL) || (row->layer >= 4u))
        {
            return FALSE;
        }
        return (gobj->proc_display ==
                ((row->dl_links != 0u) ? layer_procs_sec : layer_procs)[row->layer]) ?
            TRUE : FALSE;
    }
}

static sb32 ndsRendererAdapterNativeStageGObjLinked(GObj *target, u32 link)
{
    GObj *gobj;
    u32 guard = 0u;

    if ((target == NULL) || (link >= GC_COMMON_MAX_DLLINKS))
    {
        return FALSE;
    }
    for (gobj = gGCCommonDLLinks[link];
         (gobj != NULL) && (guard < 256u);
         gobj = gobj->dl_link_next, guard++)
    {
        if (gobj == target)
        {
            return TRUE;
        }
    }
    return FALSE;
}

static sb32 ndsRendererAdapterNativeStageLayer0OrderMatches(
    GObj *const *segments)
{
    GObj *gobj;
    u32 next = 0u;
    u32 guard = 0u;
    const u32 layer0 = ndsRendererAdapterNativeStageLayer0Count();

    if (segments == NULL)
    {
        return FALSE;
    }
    /* Planet Zebes captures layer 1 only (its layer 0 has no DObjs), so
     * there is no layer-0 order to check; zero rows used to decline here and
     * that was the whole of its reason-4 reject (2026-09-07). */
    if (layer0 == 0u)
    {
        return TRUE;
    }
    for (gobj = gGCCommonDLLinks[4];
         (gobj != NULL) && (guard < 256u) && (next < layer0);
         gobj = gobj->dl_link_next, guard++)
    {
        if (gobj == segments[next])
        {
            next++;
        }
    }
    return (next == layer0) ? TRUE : FALSE;
}

static sb32 ndsRendererAdapterNativeStageTransformFlags(
    const DObj *dobj, u16 *out_flags)
{
    if ((dobj == NULL) || (out_flags == NULL))
    {
        return FALSE;
    }
    if ((dobj->xobjs_num == 1u) && (dobj->xobjs[0] != NULL) &&
        ((dobj->xobjs[0]->kind == nGCMatrixKindTraRotRpyR) ||
         (dobj->xobjs[0]->kind == nGCMatrixKindTraRotRpyRSca) ||
         (dobj->xobjs[0]->kind == nGCMatrixKindTra)))
    {
        /* These are non-camera descriptor shapes. The matrix builder still
         * executes the actual kind: Yamabuki's animated gate is TraRotRpyR;
         * Zebes acid uses translation only. */
        *out_flags = 0u;
        return TRUE;
    }
    if ((dobj->xobjs_num == 2u) && (dobj->xobjs[0] != NULL) &&
        (dobj->xobjs[1] != NULL) &&
        (dobj->xobjs[0]->kind == nGCMatrixKindTra))
    {
        if (dobj->xobjs[1]->kind == nGCMatrixKind48)
        {
            *out_flags = 2u;
            return TRUE;
        }
        if (dobj->xobjs[1]->kind == nGCMatrixKind46)
        {
            *out_flags = 4u;
            return TRUE;
        }
        if (dobj->xobjs[1]->kind == nGCMatrixKindRecalcRotRpyRSca)
        {
            *out_flags = 8u;
            return TRUE;
        }
    }
    return FALSE;
}

#if NDS_P2_1P_GAME
/* Board the Platforms' platforms (sc1PBonusStageInitPlatforms and
 * sc1PBonusStageUpdatePlatformCount, sc1pbonusstage.c:539/614) are DObj trees
 * from Bonus2Common hung under the board's yakumono DObjs at runtime. They are
 * not packet geometry -- every bonus2 descriptor excludes them -- so the
 * topology skips each one whole, and ndsStageGCDrawAllLoopSubmitForeignSubtrees
 * draws it through the per-DObj path, where generate_nds_native_item_baked.py's
 * roots own its lists. Recognised by the file its DLLink array lives in. */
static sb32 ndsRendererAdapterIsForeignStageSubtree(const DObj *dobj)
{
    const NDSRelocLoadedFile *loaded;

    /* A Board the Platforms board, the source's or a content's (P7 C2). */
    if ((gSCManagerBattleState == NULL) ||
        (ndsGrBonusClass(gSCManagerBattleState->gkind) != 2u) ||
        (dobj == NULL) || (dobj->dl_link == NULL))
    {
        return FALSE;
    }
    loaded = ndsRelocFindLoadedFileContaining(dobj->dl_link,
                                              sizeof(DObjDLLink));
    return ((loaded != NULL) &&
            (loaded->asset_id == NDS_RELOC_ASSET_BONUS2_COMMON)) ? TRUE
                                                                 : FALSE;
}
#endif

static sb32 ndsRendererAdapterCollectNativeStageDObjs(
    DObj *dobj, u32 owner, u16 parent_index, u8 depth, u32 dl_links,
    NDSRendererAdapterNativeStageWorkspace *workspace)
{
    for (; dobj != NULL; dobj = dobj->sib_next)
    {
        NDSRendererNativeStageDObj *live;
        u32 index;
        u16 transform_flags;

#if NDS_P2_1P_GAME
        if (ndsRendererAdapterIsForeignStageSubtree(dobj) != FALSE)
        {
            continue;
        }
#endif
        /* A hidden DObj is part of the topology like any other: whether it
         * draws is a per-frame fact the binding mask applies
         * (ndsRendererAdapterStageDObjHiddenInTree). Refusing it here refused
         * the whole packet, so a map whose AnimJoints hide DObjs drew nothing
         * -- Kirby's Board the Platforms starts with a rail carrier and three
         * of its six animated rails hidden (owner r71: "started the stage
         * without fully rendering the map structure"; topology step 10, 5 of
         * 17 layer-1 DObjs collected). */
        if ((workspace->dobj_count >= NDS_RENDERER_ADAPTER_STAGE_DOBJ_COUNT) ||
            (depth > 31u) ||
            (ndsRendererAdapterNativeStageTransformFlags(
                 dobj, &transform_flags) == FALSE))
        {
            return FALSE;
        }
        index = workspace->dobj_count++;
        workspace->dobjs[index] = dobj;
        live = &workspace->live_dobjs[index];
        live->identity = dobj;
        live->parent_index = parent_index;
        live->transform_flags = transform_flags;
        live->owner = (u8)owner;
        live->depth = depth;
        live->binding_index = 0xffffu;
        if ((dl_links != 0u) && (dobj->dl_link != NULL))
        {
            /* P2-4n1 step 7: a Sec-callback layer draws DObjDLLink arrays --
             * gcDrawDObjTreeDLLinks walks each DObj's links in array order,
             * stopping at the sentinel list_id, and every link with a list
             * is one display list on one head. That is exactly one binding
             * per link, in that order, which is the order the generator
             * compiled the packet in (binding_dobjs / binding_heads). */
            DObjDLLink *dl_link = dobj->dl_link;
            u32 link;

            for (link = 0u; link < GC_COMMON_MAX_DLLINKS; link++, dl_link++)
            {
                u32 binding;

                if (dl_link->list_id == (s32)NDS_RENDERER_STAGE_DL_HEADS)
                {
                    break;
                }
                if ((dl_link->list_id < 0) ||
                    ((u32)dl_link->list_id >= NDS_RENDERER_STAGE_DL_HEADS) ||
                    (dl_link->dl == NULL))
                {
                    continue;
                }
                binding = workspace->binding_count++;
                if (binding >= NDS_RENDERER_ADAPTER_STAGE_BINDING_COUNT)
                {
                    return FALSE;
                }
                if (live->binding_index == 0xffffu)
                {
                    live->binding_index = (u16)binding;
                }
                workspace->binding_dobjs[binding] = dobj;
                workspace->binding_display_lists[binding] = dl_link->dl;
                workspace->binding_heads[binding] = (u8)dl_link->list_id;
                workspace->binding_link_index[binding] = (u8)link;
            }
        }
        else if ((dl_links == 0u) && (dobj->dv != NULL))
        {
            u32 binding = workspace->binding_count++;
            if (binding >= NDS_RENDERER_ADAPTER_STAGE_BINDING_COUNT)
            {
                return FALSE;
            }
            live->binding_index = (u16)binding;
            workspace->binding_dobjs[binding] = dobj;
            workspace->binding_display_lists[binding] = dobj->dv;
            workspace->binding_heads[binding] = 0u;
            workspace->binding_link_index[binding] = 0xffu;
        }
        if ((dobj->child != NULL) &&
            (ndsRendererAdapterCollectNativeStageDObjs(
                 dobj->child, owner, (u16)index, (u8)(depth + 1u), dl_links,
                 workspace) == FALSE))
        {
            return FALSE;
        }
    }
    return TRUE;
}

/* The source's tree walk skips a hidden DObj and everything under it
 * (gcDrawDObjTreeDLLinks, objdisplay.c:1707-1744; gcDrawDObjTree likewise),
 * so a binding is hidden while its DObj or any ancestor is. Stage maps are
 * one to three levels deep. */
static sb32 ndsRendererAdapterStageDObjHiddenInTree(const DObj *dobj)
{
    u32 depth = 0u;

    while ((dobj != NULL) && (dobj != DOBJ_PARENT_NULL) && (depth++ < 32u))
    {
        if ((dobj->flags & DOBJ_FLAG_HIDDEN) != 0u)
        {
            return TRUE;
        }
        dobj = dobj->parent;
    }
    return FALSE;
}

static u32 ndsRendererAdapterNativeStageStampValue(u32 stamp, uintptr_t value)
{
    stamp ^= (u32)value;
    stamp *= 16777619u;
    stamp ^= stamp >> 16;
    return stamp;
}

/* Which decline site of the three reason-4 builders ran last (1-based,
 * textual order across BuildNativeStageTopologyStamp, Collect and
 * CaptureTask36StageWorld; 0 = never declined) and the loop index there.
 * The adapter latches only reason 4 for all of them (2026-09-07). */
volatile u32 gNdsRendererAdapterStageTopologyFailStep;
volatile u32 gNdsRendererAdapterStageTopologyFailIndex;
volatile u32 gNdsRendererAdapterStageTopologyCollectMask;
volatile u32 gNdsRendererAdapterStageTopologyCollectCounts;

static sb32 ndsRendererAdapterBuildNativeStageTopologyStamp(
    NDSRendererAdapterNativeStageWorkspace *workspace,
    u32 generation,
    u32 *out_stamp)
{
    u32 stamp = 2166136261u;
    u32 i;

    if ((workspace == NULL) || (out_stamp == NULL) || (generation == 0u) ||
        (workspace->dobj_count !=
         ndsRendererAdapterNativeStageActiveDObjCount()) ||
        (workspace->binding_count !=
         ndsRendererAdapterNativeStageActiveBindingCount()))
    {
        gNdsRendererAdapterStageTopologyFailStep = 1u;
        gNdsRendererAdapterStageTopologyFailIndex = 0xffffffffu;
        return FALSE;
    }
    stamp = ndsRendererAdapterNativeStageStampValue(stamp, generation);
    for (i = 0u; i < ndsRendererAdapterNativeStageActiveAssetCount(); i++)
    {
        NDSRelocLoadedFile *loaded = workspace->loaded[i];

        if ((loaded == NULL) || (loaded->data == NULL) ||
            (loaded->owner_generation != generation))
        {
            gNdsRendererAdapterStageTopologyFailStep = 2u;
            gNdsRendererAdapterStageTopologyFailIndex = i;
            return FALSE;
        }
        stamp = ndsRendererAdapterNativeStageStampValue(
            stamp, (uintptr_t)loaded);
        stamp = ndsRendererAdapterNativeStageStampValue(
            stamp, (uintptr_t)loaded->data);
        stamp = ndsRendererAdapterNativeStageStampValue(
            stamp, loaded->asset_id);
        stamp = ndsRendererAdapterNativeStageStampValue(
            stamp, loaded->data_size);
        stamp = ndsRendererAdapterNativeStageStampValue(
            stamp, loaded->owner_generation);
    }
    for (i = 0u; i < ndsRendererAdapterNativeStageActiveSegmentCount(); i++)
    {
        GObj *gobj = ndsRendererAdapterNativeStageSegmentGObj(i);

        if ((gobj == NULL) || (gobj != workspace->segments[i]) ||
            (DObjGetStruct(gobj) == NULL) ||
            ((gobj->flags & GOBJ_FLAG_HIDDEN) != 0u) ||
            (gobj->dl_link_id != ndsRendererAdapterNativeStageSegmentLink(i)) ||
            (ndsRendererAdapterNativeStageProcMatches(i, gobj) == FALSE) ||
            (ndsRendererAdapterNativeStageGObjLinked(
                 gobj, gobj->dl_link_id) == FALSE))
        {
            gNdsRendererAdapterStageTopologyFailStep = 3u;
            gNdsRendererAdapterStageTopologyFailIndex = i;
            return FALSE;
        }
        stamp = ndsRendererAdapterNativeStageStampValue(
            stamp, (uintptr_t)gobj);
        stamp = ndsRendererAdapterNativeStageStampValue(
            stamp, (uintptr_t)DObjGetStruct(gobj));
        stamp = ndsRendererAdapterNativeStageStampValue(
            stamp, gobj->flags);
        stamp = ndsRendererAdapterNativeStageStampValue(
            stamp, gobj->dl_link_id);
        stamp = ndsRendererAdapterNativeStageStampValue(
            stamp, (uintptr_t)gobj->proc_display);
    }
    if (ndsRendererAdapterNativeStageLayer0OrderMatches(
            workspace->segments) == FALSE)
    {
        gNdsRendererAdapterStageTopologyFailStep = 4u;
        gNdsRendererAdapterStageTopologyFailIndex = i;
        return FALSE;
    }
    for (i = 0u; i < workspace->dobj_count; i++)
    {
        DObj *dobj = workspace->dobjs[i];
        NDSRendererNativeStageDObj *live = &workspace->live_dobjs[i];
        u16 transform_flags;
        u32 xobj_index;

        if ((dobj == NULL) || (live->identity != dobj) ||
            (ndsRendererAdapterNativeStageTransformFlags(
                 dobj, &transform_flags) == FALSE) ||
            (transform_flags != live->transform_flags))
        {
            gNdsRendererAdapterStageTopologyFailStep = 5u;
            gNdsRendererAdapterStageTopologyFailIndex = i;
            return FALSE;
        }
        stamp = ndsRendererAdapterNativeStageStampValue(
            stamp, (uintptr_t)dobj);
        stamp = ndsRendererAdapterNativeStageStampValue(
            stamp, (uintptr_t)dobj->parent_gobj);
        stamp = ndsRendererAdapterNativeStageStampValue(
            stamp, (uintptr_t)dobj->parent);
        stamp = ndsRendererAdapterNativeStageStampValue(
            stamp, (uintptr_t)dobj->child);
        stamp = ndsRendererAdapterNativeStageStampValue(
            stamp, (uintptr_t)dobj->sib_next);
        stamp = ndsRendererAdapterNativeStageStampValue(
            stamp, (uintptr_t)dobj->sib_prev);
        stamp = ndsRendererAdapterNativeStageStampValue(
            stamp, (uintptr_t)dobj->dv);
        stamp = ndsRendererAdapterNativeStageStampValue(
            stamp, (uintptr_t)dobj->mobj);
        stamp = ndsRendererAdapterNativeStageStampValue(stamp, dobj->flags);
        stamp = ndsRendererAdapterNativeStageStampValue(
            stamp, dobj->xobjs_num);
        stamp = ndsRendererAdapterNativeStageStampValue(
            stamp, live->parent_index);
        stamp = ndsRendererAdapterNativeStageStampValue(
            stamp, live->binding_index);
        stamp = ndsRendererAdapterNativeStageStampValue(
            stamp, live->owner);
        stamp = ndsRendererAdapterNativeStageStampValue(
            stamp, live->depth);
        for (xobj_index = 0u; xobj_index < dobj->xobjs_num; xobj_index++)
        {
            XObj *xobj = dobj->xobjs[xobj_index];

            if (xobj == NULL)
            {
                gNdsRendererAdapterStageTopologyFailStep = 6u;
                gNdsRendererAdapterStageTopologyFailIndex = i;
                return FALSE;
            }
            stamp = ndsRendererAdapterNativeStageStampValue(
                stamp, (uintptr_t)xobj);
            stamp = ndsRendererAdapterNativeStageStampValue(
                stamp, xobj->kind);
        }
    }
    for (i = 0u; i < workspace->binding_count; i++)
    {
        const DObj *binding_dobj = workspace->binding_dobjs[i];
        const void *live_list;

        if ((binding_dobj == NULL) ||
            (workspace->binding_display_lists[i] == NULL))
        {
            gNdsRendererAdapterStageTopologyFailStep = 7u;
            gNdsRendererAdapterStageTopologyFailIndex = i;
            return FALSE;
        }
        live_list = (workspace->binding_link_index[i] == 0xffu) ?
            (const void *)binding_dobj->dv :
            ((binding_dobj->dl_link != NULL) ?
                 (const void *)binding_dobj->dl_link[
                     workspace->binding_link_index[i]].dl : NULL);
        if (live_list != workspace->binding_display_lists[i])
        {
            gNdsRendererAdapterStageTopologyFailStep = 8u;
            gNdsRendererAdapterStageTopologyFailIndex = i;
            return FALSE;
        }
        stamp = ndsRendererAdapterNativeStageStampValue(
            stamp, (uintptr_t)workspace->binding_dobjs[i]);
        stamp = ndsRendererAdapterNativeStageStampValue(
            stamp, (uintptr_t)workspace->binding_display_lists[i]);
    }
    *out_stamp = (stamp != 0u) ? stamp : 1u;
    return TRUE;
}

/* Task 44 item 3: the cheap half of stage admission.
 *
 * The asset-mutation generation proves the four reloc payloads have not been
 * replaced or unloaded; it says nothing about the scene graph that hangs off
 * them. These eight checks cover the graph mutations the stage owner must fail
 * closed on — a segment GObj swapped, hidden, relinked, or given a different
 * display proc — and every one of them is a direct global load. What steady
 * state no longer pays for is the O(n) work: the four loaded-file table scans,
 * the eight DL-link list walks, the two layer-0 order walks, and the 57-DObj /
 * 42-binding stamp rebuild with its per-DObj transform-flag derivation. Any
 * failure here drops through to exactly that full validation. */
static sb32 ndsRendererAdapterNativeStageSegmentsUnchanged(
    const NDSRendererAdapterNativeStageWorkspace *workspace)
{
    u32 i;

    for (i = 0u; i < ndsRendererAdapterNativeStageActiveSegmentCount(); i++)
    {
        GObj *gobj = ndsRendererAdapterNativeStageSegmentGObj(i);

        if ((gobj == NULL) || (gobj != workspace->segments[i]) ||
            ((gobj->flags & GOBJ_FLAG_HIDDEN) != 0u) ||
            (gobj->dl_link_id !=
             ndsRendererAdapterNativeStageSegmentLink(i)) ||
            (DObjGetStruct(gobj) != workspace->steady_segment_roots[i]) ||
            (ndsRendererAdapterNativeStageProcMatches(i, gobj) == FALSE))
        {
            return FALSE;
        }
    }
    return TRUE;
}

#define NDS_STAGE_SEGMENT_BLOOM_BIT(gobj) \
    (1u << (((u32)(uintptr_t)(gobj) >> 3) & 31u))

static sb32 ndsRendererAdapterCollectNativeStageTopologyBody(
    NDSRendererAdapterNativeStageWorkspace *workspace);

/* The body is the only writer of segments[]; whichever way it returns, the
 * bloom then names every entry it left (P2-2p8, 2026-10-04). */
static sb32 ndsRendererAdapterCollectNativeStageTopology(
    NDSRendererAdapterNativeStageWorkspace *workspace)
{
    const sb32 collected =
        ndsRendererAdapterCollectNativeStageTopologyBody(workspace);
    u32 bloom = 0u;
    u32 i;

    for (i = 0u; i < NDS_RENDERER_ADAPTER_STAGE_SEGMENT_COUNT; i++)
    {
        if (workspace->segments[i] != NULL)
        {
            bloom |= NDS_STAGE_SEGMENT_BLOOM_BIT(workspace->segments[i]);
        }
    }
    workspace->segment_bloom = bloom;
    return collected;
}

static sb32 ndsRendererAdapterCollectNativeStageTopologyBody(
    NDSRendererAdapterNativeStageWorkspace *workspace)
{
    const u32 segment_count = ndsRendererAdapterNativeStageActiveSegmentCount();
    u32 i;

    if (segment_count == 0u)
    {
        gNdsRendererAdapterStageTopologyFailStep = 9u;
        gNdsRendererAdapterStageTopologyFailIndex = 0xffffffffu;
        return FALSE;
    }
    /* Rows past the active count must read as absent to every later walk. */
    for (i = segment_count; i < NDS_RENDERER_ADAPTER_STAGE_SEGMENT_COUNT; i++)
    {
        workspace->segments[i] = NULL;
        workspace->steady_segment_roots[i] = NULL;
    }
    for (i = 0u; i < segment_count; i++)
    {
        const NDSRendererAdapterNativeStageCaptureSegment *row =
            ndsRendererAdapterNativeStageCaptureRow(i);
        u32 first_dobj = workspace->dobj_count;
        GObj *gobj = ndsRendererAdapterNativeStageSegmentGObj(i);

        workspace->segments[i] = gobj;
        if ((row == NULL) || (gobj == NULL) ||
            ((gobj->flags & GOBJ_FLAG_HIDDEN) != 0u) ||
            (gobj->dl_link_id != ndsRendererAdapterNativeStageSegmentLink(i)) ||
            (ndsRendererAdapterNativeStageProcMatches(i, gobj) == FALSE) ||
            (ndsRendererAdapterNativeStageGObjLinked(
                 gobj, gobj->dl_link_id) == FALSE) ||
            (ndsRendererAdapterCollectNativeStageDObjs(
                 DObjGetStruct(gobj), row->owner,
                 0xffffu, 0u, row->dl_links, workspace) == FALSE) ||
            ((workspace->dobj_count - first_dobj) != row->dobj_count))
        {
            /* Which operand declined, bits in operand order; bit 6 = the
             * DObj collection itself (inferred when nothing else did), and
             * the live/expected DObj counts packed for the same shot. */
            u32 mask = 0u;

            mask |= (row == NULL) ? 1u : 0u;
            mask |= (gobj == NULL) ? 2u : 0u;
            if (gobj != NULL)
            {
                mask |= ((gobj->flags & GOBJ_FLAG_HIDDEN) != 0u) ? 4u : 0u;
                mask |= (gobj->dl_link_id !=
                         ndsRendererAdapterNativeStageSegmentLink(i)) ?
                    8u : 0u;
                mask |= (ndsRendererAdapterNativeStageProcMatches(i, gobj) ==
                         FALSE) ? 16u : 0u;
                mask |= (ndsRendererAdapterNativeStageGObjLinked(
                             gobj, gobj->dl_link_id) == FALSE) ? 32u : 0u;
            }
            if ((mask == 0u) && (row != NULL) &&
                ((workspace->dobj_count - first_dobj) == row->dobj_count))
            {
                mask |= 64u;
            }
            gNdsRendererAdapterStageTopologyCollectMask = mask;
            gNdsRendererAdapterStageTopologyCollectCounts =
                ((workspace->dobj_count - first_dobj) << 16) |
                ((row != NULL) ? (u32)row->dobj_count : 0xffffu);
            gNdsRendererAdapterStageTopologyFailStep = 10u;
            gNdsRendererAdapterStageTopologyFailIndex = i;
            return FALSE;
        }
        workspace->steady_segment_roots[i] = DObjGetStruct(gobj);
    }
    /* P2-4n1 step 7: on a DLLink packet every captured binding must hang off
     * the DObj and draw into the head the generator compiled it from, or the
     * runs would be emitted under the wrong live world. Layer packets have
     * no head arrays and skip this. */
    if (ndsRendererNativeStageHasBindingHeads() != FALSE)
    {
        for (i = 0u; i < workspace->binding_count; i++)
        {
            u32 dobj_index;
            u32 head;

            if ((ndsRendererNativeStageBindingIdentity(i, &dobj_index,
                                                       &head) == FALSE) ||
                (dobj_index >= workspace->dobj_count) ||
                (workspace->dobjs[dobj_index] != workspace->binding_dobjs[i]) ||
                (head != workspace->binding_heads[i]))
            {
                gNdsRendererAdapterStageTopologyFailStep = 11u;
                gNdsRendererAdapterStageTopologyFailIndex = i;
                return FALSE;
            }
        }
    }
    {
        u32 mask = 0u;

        mask |= (workspace->dobj_count !=
                 ndsRendererAdapterNativeStageActiveDObjCount()) ? 1u : 0u;
        mask |= (workspace->binding_count !=
                 ndsRendererAdapterNativeStageActiveBindingCount()) ? 2u : 0u;
        mask |= (ndsRendererAdapterNativeStageLayer0OrderMatches(
                     workspace->segments) == FALSE) ? 4u : 0u;
        if (mask != 0u)
        {
            gNdsRendererAdapterStageTopologyCollectMask = 0x100u | mask;
            gNdsRendererAdapterStageTopologyCollectCounts =
                (workspace->dobj_count << 24) |
                (ndsRendererAdapterNativeStageActiveDObjCount() << 16) |
                (workspace->binding_count << 8) |
                ndsRendererAdapterNativeStageActiveBindingCount();
            gNdsRendererAdapterStageTopologyFailStep = 14u;
            gNdsRendererAdapterStageTopologyFailIndex = 0u;
            return FALSE;
        }
    }
    return TRUE;
}

typedef struct NDSRendererAdapterStageFrameCamera
{
    NDSRendererMatrix20p12 projection;
    NDSRendererMatrix20p12 modelview;
    u32 projection_valid;
    u32 modelview_valid;
    NDSRendererAdapterMvpCamera recalc;
} NDSRendererAdapterStageFrameCamera;


/* Quiet dynamic bindings: revalidated one frame in this many (1, 2, 4 or 8;
 * 1 = every frame, the behaviour before 2026-10-04). Same-ROM A/B word. */
volatile u32 gNdsStageDynStride __attribute__((used, section(".data"))) = 4u;
#define NDS_STAGE_DYN_QUIET_MIN 8u
#include <gr/ground.h>

/* Bindings whose DObj or an ancestor is a yakumono -- map collision that
 * moves. Their render must never lag the collision fighters stand on, so the
 * dynamic stride skips them. Once per topology capture. */
static u64 __attribute__((cold)) ndsRendererAdapterStageYakumonoBindings(
    const NDSRendererAdapterNativeStageWorkspace *workspace)
{
    u64 mask = 0u;
    u32 binding_index;

    if ((gMPCollisionYakumonoDObjs == NULL) || (gMPCollisionYakumonosNum <= 0))
    {
        return 0u;
    }
    for (binding_index = 0u;
         (binding_index < workspace->binding_count) && (binding_index < 64u);
         binding_index++)
    {
        DObj *dobj;

        for (dobj = workspace->binding_dobjs[binding_index];
             (dobj != NULL) && (dobj != DOBJ_PARENT_NULL);
             dobj = dobj->parent)
        {
            s32 i;

            for (i = 0; i < gMPCollisionYakumonosNum; i++)
            {
                if (gMPCollisionYakumonoDObjs->dobjs[i] == dobj)
                {
                    mask |= (u64)1u << binding_index;
                    break;
                }
            }
            if (((mask >> binding_index) & 1u) != 0u)
            {
                break;
            }
        }
    }
    return mask;
}

/* gGCScaleX as the source's tree walk leaves it at a billboard: every
 * ancestor whose matrix kind scales multiplies its scale.x in
 * (objdisplay.c:574-670). The battle's stage trees leave it at 1.0, which
 * the stage path assumes; the stage select's preview scales each layer's
 * root (mnMapsMakeLayer), so its billboards take that scale. */
static f32 ndsRendererAdapterStagePreviewRecalcScale(const DObj *dobj)
{
    const DObj *parent;
    f32 scale = 1.0F;
    u32 depth = 0u;

    if ((sNdsRendererAdapterStagePreviewMode == 0u) || (dobj == NULL))
    {
        return 1.0F;
    }
    for (parent = dobj->parent;
         (parent != NULL) && (parent != DOBJ_PARENT_NULL) && (depth < 32u);
         parent = parent->parent, depth++)
    {
        u32 i;

        for (i = 0u; i < (u32)parent->xobjs_num; i++)
        {
            const XObj *xobj = parent->xobjs[i];

            if ((xobj != NULL) &&
                ((xobj->kind == nGCMatrixKindTraRotRSca) ||
                 (xobj->kind == nGCMatrixKindTraRotRpyRSca) ||
                 (xobj->kind == nGCMatrixKindTraRotPyrRSca) ||
                 (xobj->kind == nGCMatrixKindSca)))
            {
                scale *= parent->scale.vec.f.x;
            }
        }
    }
    return scale;
}

static sb32 ndsRendererAdapterPrepareNativeStageBindingMatrix(
    CObj *cobj, NDSRendererAdapterNativeStageWorkspace *workspace,
    u32 binding_index
    , NDSRendererAdapterStageFrameCamera *camera
    )
{
    DObj *dobj = workspace->binding_dobjs[binding_index];
    NDSRendererMatrix20p12 world;
    const NDSRendererMatrix20p12 *world_ptr;
    NDSRendererMatrix20p12 *out = &workspace->binding_composed[binding_index];
    const NDSRendererMatrix20p12 *projection_ptr = NULL;
    const NDSRendererMatrix20p12 *modelview_ptr = out;
    u32 kind = ndsRendererAdapterDirectMvpRecalcKind(dobj);

    if (kind != 0u)
    {
        sNdsRendererAdapterMvpRecalcScaleX =
            ndsRendererAdapterStagePreviewRecalcScale(dobj);
    }
    /* The cached world itself, not a copy (2026-10-04). */
    /* P2-2p8 (2026-10-04, owner ruling D12c): a dynamic binding whose chain
     * walk rebuilt nothing for NDS_STAGE_DYN_QUIET_MIN frame validations in a
     * row is revalidated one frame in gNdsStageDynStride (the slice 44 cursor
     * spreads them evenly); on the others its persistent world is reused --
     * the camera compose below still runs every frame. A rebuild returns the
     * binding to every-frame validation, so a part that starts moving after a
     * quiet spell shows it at most stride - 1 frames late. */
    {
        u8 *quiet = &workspace->dyn_quiet[binding_index];
        const u32 stride = gNdsStageDynStride;
        const u32 rebuilds = sNdsRendererAdapterStageWorldRebuilds;
        const sb32 stale =
            ((stride > 1u) && (*quiet >= NDS_STAGE_DYN_QUIET_MIN) &&
             (((workspace->dyn_pinned_mask >> binding_index) & 1u) == 0u) &&
             (((binding_index + workspace->slice44_validate_cursor) &
               (stride - 1u)) != 0u)) ? TRUE : FALSE;

        world_ptr = ndsRendererAdapterPersistentStageWorldPtr(dobj, &world,
                                                              stale);
        if (stale == FALSE)
        {
            if (sNdsRendererAdapterStageWorldRebuilds != rebuilds)
            {
                *quiet = 0u;
            }
            else if (*quiet < 255u)
            {
                (*quiet)++;
            }
        }
    }
    if (world_ptr == NULL)
    { return FALSE; }
    /* Preserve the source multiplication order when a camera owns both parts.
     * Battle cameras normally supply LookAt*Persp in projection alone.
     *
     * P2-2p8 (2026-09-29): an MVP-recalc binding (kind != 0: the stage's
     * billboards, 11 a frame on Dream Land) has rows 0-2 replaced by
     * ApplyMvpRecalc below, which reads only the translation row of this
     * product -- so only that row is formed, bit for bit the full product's.
     * Not the persp-scale kind: its scale-conversion refusal returns with the
     * product itself as the result. */
    if ((kind != 0u) &&
        (kind != NDS_RENDERER_ADAPTER_MVP_RECALC_PERSP_SCA_KIND))
    {
        const NDSRendererMatrix20p12 *lhs = world_ptr;

        if (camera->modelview_valid != FALSE)
        {
            ndsRendererMtxMulRow3_20p12(lhs, &camera->modelview, out);
            lhs = out;
        }
        if (camera->projection_valid != FALSE)
        {
            ndsRendererMtxMulRow3_20p12(lhs, &camera->projection, out);
        }
        else if (camera->modelview_valid == FALSE)
        {
            *out = *world_ptr;
        }
    }
    else if (camera->modelview_valid != FALSE)
    {
        ndsRendererMtxMul20p12(world_ptr, &camera->modelview, out);
        if (camera->projection_valid != FALSE)
        { ndsRendererMtxMul20p12(out, &camera->projection, out); }
    }
    else if (camera->projection_valid != FALSE)
    { ndsRendererMtxMul20p12(world_ptr, &camera->projection, out); }
    else { *out = *world_ptr; }
    if (kind != 0u)
    {
        ndsRendererAdapterApplyMvpRecalc(dobj, kind, cobj,
            &workspace->projection, &projection_ptr, out, &modelview_ptr,
            &camera->recalc);
        if ((modelview_ptr == NULL) || (projection_ptr != NULL)) { return FALSE; }
    }
    return TRUE;
}


/* The stage select's preview camera (ndsRendererAdapterIsStagePreviewCamera):
 * the same eye, target and perspective as the battle's 0x4C camera, split
 * the way that camera is. */
static sb32 ndsRendererAdapterBuildStagePreviewCameraMatrices(
    CObj *cobj, NDSRendererMatrix20p12 *projection,
    NDSRendererMatrix20p12 *modelview)
{
    LookAt look_at;

    if (ndsRendererAdapterIsStagePreviewCamera(cobj) == FALSE)
    {
        return FALSE;
    }
    ndsRendererAdapterCameraLookAtReflect(
        modelview, &look_at,
        cobj->vec.eye.x, cobj->vec.eye.y, cobj->vec.eye.z,
        cobj->vec.at.x, cobj->vec.at.y, cobj->vec.at.z,
        cobj->vec.up.x, cobj->vec.up.y, cobj->vec.up.z);
    ndsRendererAdapterCameraPerspFast(
        projection, &cobj->projection.persp.norm,
        cobj->projection.persp.fovy, cobj->projection.persp.aspect,
        cobj->projection.persp.near, cobj->projection.persp.far,
        cobj->projection.persp.scale);
    return TRUE;
}

static sb32 ndsRendererAdapterPrepareNativeStageMatrices(
    CObj *cobj, NDSRendererAdapterNativeStageWorkspace *workspace)
{
    u32 binding_index;

    NDSRendererAdapterStageFrameCamera camera;
    /* The frame camera first: its cache entry then holds the split look-at
     * and perspective the stage camera is made of (see
     * ndsRendererAdapterStageCameraFromFrameCache). */
    ndsRendererAdapterGetFrameCameraMatrices(cobj,
        &camera.projection, &camera.projection_valid,
        &camera.modelview, &camera.modelview_valid, NULL, NULL, NULL);
    if ((ndsRendererAdapterStageCameraFromFrameCache(
             cobj, &workspace->projection,
             &workspace->camera_modelview) == FALSE) &&
        (ndsRendererAdapterBuildTask36StageCameraMatrices(
             cobj, &workspace->projection,
             &workspace->camera_modelview) == FALSE) &&
        (ndsRendererAdapterBuildStagePreviewCameraMatrices(
             cobj, &workspace->projection,
             &workspace->camera_modelview) == FALSE))
    {
        return FALSE;
    }
    camera.recalc.perspective = &workspace->projection;
    camera.recalc.perspective_f_valid = FALSE;
    camera.recalc.mod1_valid = FALSE;
    ndsRendererAdapterMvpMemoReset(&camera.recalc);

    for (binding_index = 0u;
         binding_index < workspace->binding_count;
         binding_index++)
    {
        if ((workspace->task36_runtime_rigid_mask &
             ((u64)1u << binding_index)) != 0u)
        {
            continue;
        }
        if (ndsRendererAdapterPrepareNativeStageBindingMatrix(
                cobj, workspace, binding_index
                , &camera
                ) == FALSE)
        {
            return FALSE;
        }
    }
    return TRUE;
}

/* Bumped by every rigid world capture; read by the stage GX cull
 * (nds_stage_gx.exec.inc). */
volatile u32 gNdsStageRigidWorldSerial;

/* The packet's rigid bindings, none while the stage select's preview owns
 * the packet: it composes every binding each draw (Planet Zebes' rigid
 * ground -- bindings 0, 2-7, 11, 15 and 24, mostly range runs -- drew
 * nothing under the preview's camera on the rigid path). */
static u64 ndsRendererAdapterStageRigidBindingMask(void)
{
    return (sNdsRendererAdapterStagePreviewMode != 0u) ? 0u :
        ndsRendererNativeStageRigidBindingMask();
}

static sb32 ndsRendererAdapterCaptureTask36StageWorld(
    NDSRendererAdapterNativeStageWorkspace *workspace)
{
    u32 binding_index;
    const u64 rigid_mask = ndsRendererAdapterStageRigidBindingMask();

    for (binding_index = 0u;
         binding_index < workspace->binding_count;
         binding_index++)
    {
        if (ndsRendererAdapterBuildDObjWorldMatrixUncached(
                workspace->binding_dobjs[binding_index],
                &workspace->binding_world[binding_index]) == FALSE)
        {
            gNdsRendererAdapterStageTopologyFailStep = 12u;
            gNdsRendererAdapterStageTopologyFailIndex = binding_index;
            return FALSE;
        }
        if (((rigid_mask &
              ((u64)1u << binding_index)) != 0u) &&
            (ndsRendererAdapterCaptureStageWorldSourceKey(
                 workspace->binding_dobjs[binding_index],
                 &workspace->task36_rigid_source_keys[binding_index]) ==
             FALSE))
        {
            gNdsRendererAdapterStageTopologyFailStep = 13u;
            gNdsRendererAdapterStageTopologyFailIndex = binding_index;
            return FALSE;
        }
    }
    workspace->task36_runtime_rigid_mask =
        rigid_mask;
    workspace->dyn_pinned_mask =
        ndsRendererAdapterStageYakumonoBindings(workspace);
    /* The stage GX cull keeps each rigid run's world-space bounds until the
     * worlds are captured again (nds_stage_gx.exec.inc). */
    gNdsStageRigidWorldSerial++;
    workspace->steady_rigid_binding_count = 0u;
    for (binding_index = 0u;
         binding_index < workspace->binding_count;
         binding_index++)
    {
        if ((rigid_mask &
             ((u64)1u << binding_index)) != 0u)
        {
            workspace->steady_rigid_bindings[
                workspace->steady_rigid_binding_count++] = (u8)binding_index;
        }
    }
    workspace->steady_binding_lists_valid = TRUE;
    return TRUE;
}

static void ndsRendererAdapterValidateTask36StageWorld(
    NDSRendererAdapterNativeStageWorkspace *workspace)
{
    u32 binding_index;
    const u64 rigid_mask = ndsRendererAdapterStageRigidBindingMask();
    u32 rigid_slot;

    /* Slice 44. Demotion is one-way within a topology: a binding that stopped
     * being rigid does not become rigid again until capture re-arms the mask at
     * ndsRendererAdapterCaptureTask36StageWorld. Before the stride the mask was
     * rebuilt from the constant here every frame and cleared again by the sweep,
     * which was equivalent only because the sweep was complete. With a partial
     * sweep, re-arming would resurrect a binding this frame's slice did not
     * look at. */
    if (workspace->task36_runtime_rigid_mask == 0u)
    {
        return;
    }
    workspace->task36_runtime_rigid_mask =
        rigid_mask;
    /* Task 44 item 4: walk the 26 rigid bindings directly. The list is only
     * consulted when capture built it for this topology; otherwise fall back
     * to the mask scan below so a torn workspace can never skip validation. */
    if (workspace->steady_binding_lists_valid != FALSE)
    {
        for (rigid_slot = 0u;
             rigid_slot < workspace->steady_rigid_binding_count;
             rigid_slot++)
        {
            if ((rigid_slot % NDS_R2_STAGE_VALIDATE_STRIDE) !=
                workspace->slice44_validate_cursor)
            {
                NDS_DIAG(gNdsR2Slice44RigidSkips++);
                continue;
            }
            NDS_DIAG(gNdsR2Slice44RigidChecks++);
            binding_index = workspace->steady_rigid_bindings[rigid_slot];
            if (ndsRendererAdapterStageWorldSourceKeyMatches(
                    workspace->binding_dobjs[binding_index],
                    &workspace->task36_rigid_source_keys[binding_index]) ==
                FALSE)
            {
                workspace->task36_runtime_rigid_mask = 0u;
                return;
            }
        }
        return;
    }
    for (binding_index = 0u;
         binding_index < workspace->binding_count;
         binding_index++)
    {
        if ((rigid_mask &
             ((u64)1u << binding_index)) == 0u)
        {
            continue;
        }
        if (ndsRendererAdapterStageWorldSourceKeyMatches(
                workspace->binding_dobjs[binding_index],
                &workspace->task36_rigid_source_keys[binding_index]) ==
            FALSE)
        {
            workspace->task36_runtime_rigid_mask = 0u;
            return;
        }
    }
}

static sb32 ndsRendererAdapterPrepareNativeStageMaterials(
    NDSRendererAdapterNativeStageWorkspace *workspace)
{
    const u32 material_count = ndsRendererAdapterNativeStageActiveMaterialCount();
    u32 i;

    /* Each slot's binding and expected MObj flags come from the packet's
     * material-event table; Dream Land's are {20,22,31,32}/{1,1,0x6b,0x6b}. */
    for (i = 0u; i < material_count; i++)
    {
        u32 binding_index;
        u32 flags;
        u32 mobj_index;
        MObj *mobj;

        if ((ndsRendererNativeStageMaterialBinding(i, &binding_index,
                                                   &flags, &mobj_index) ==
             FALSE) ||
            (binding_index >= workspace->binding_count) ||
            (workspace->binding_dobjs[binding_index] == NULL))
        {
            return FALSE;
        }
        /* The slot's own MObj: the DObj's list in draw order, as
         * gcDrawMObjForDObj walks it. */
        mobj = workspace->binding_dobjs[binding_index]->mobj;
        while ((mobj != NULL) && (mobj_index != 0u))
        {
            mobj = mobj->next;
            mobj_index--;
        }
        if ((mobj == NULL) ||
            (ndsRendererAdapterMaterialFlags(mobj) != (u16)flags) ||
            (ndsRendererAdapterBuildNativeMaterialSnapshot(
                 mobj, &workspace->materials[i], FALSE,
                 &workspace->material_curr[i],
                 &workspace->material_next[i]) == FALSE))
        {
            return FALSE;
        }
        workspace->material_mobjs[i] = mobj;
    }
    return TRUE;
}

static void ndsRendererAdapterCommitNativeStageMaterials(
    NDSRendererAdapterNativeStageWorkspace *workspace, u32 segment_index)
{
    u32 mask = ndsRendererNativeStageMaterialMask(segment_index);
    u32 i;

    /* Commit only the material slots owned by this packet's segment. Stages
     * with no material animation have no MObjs in this workspace. */
    for (i = 0u; mask != 0u; i++, mask >>= 1u)
    {
        if ((mask & 1u) == 0u)
        {
            continue;
        }
        workspace->material_mobjs[i]->texture_id_curr =
            workspace->material_curr[i];
        workspace->material_mobjs[i]->texture_id_next =
            workspace->material_next[i];
    }
}


static s32 ndsRendererAdapterPrepareNativeStageOwnerBody(
    void *camera_gobj_ptr);

static unsigned int ndsRendererAdapterPrepareNativeStageOwnerOnHotStack(
    void *camera_gobj_ptr)
{
    return (unsigned int)ndsRendererAdapterPrepareNativeStageOwnerBody(
        camera_gobj_ptr);
}

/* P2-2p8 (2026-09-27): the stage owner prep -- world matrices, MVP
 * recalc, material snapshot, once a frame -- runs on the DTCM hot stack
 * (port/coroutine.h), as the segment commit already did. Same-ROM A/B with
 * the fighter display proc: paired WORK-H -11.8K (STG -8.7K). It reads no
 * storage into a stack buffer and hands no stack address to DMA. */
s32 ndsRendererAdapterPrepareNativeStageOwner(void *camera_gobj_ptr)
{
    return (s32)ndsDtcmHotStackRun(
        ndsRendererAdapterPrepareNativeStageOwnerOnHotStack,
        camera_gobj_ptr);
}

/* The stage select's preview layers (NULL: the battle's own), see
 * sNdsRendererAdapterStagePreviewMode. A change re-validates the topology
 * from scratch: the preview reloads its stage into the same arena. */
void ndsRendererAdapterSetStagePreviewLayers(void *const *layers)
{
    NDSRendererAdapterNativeStageWorkspace *workspace =
        &sNdsRendererAdapterNativeStageWorkspace;
    u32 i;

    for (i = 0u; i < 4u; i++)
    {
        sNdsRendererAdapterStagePreviewLayers[i] =
            (layers != NULL) ? (GObj *)layers[i] : NULL;
    }
    if (layers == NULL)
    {
        memset(sNdsRendererAdapterStagePreviewActors, 0,
               sizeof(sNdsRendererAdapterStagePreviewActors));
    }
    sNdsRendererAdapterStagePreviewMode = (layers != NULL) ? 1u : 0u;
    workspace->active = FALSE;
    workspace->topology_valid = FALSE;
    workspace->steady_admission_generation = 0u;
}

/* A preview stand-in for a capture row's battle-only actor (source
 * NDS_RENDERER_ADAPTER_STAGE_CAPTURE_*, index as the row names it). Set
 * before ndsRendererAdapterSetStagePreviewLayers; cleared with it. */
void ndsRendererAdapterSetStagePreviewActor(u32 source, u32 index, void *gobj)
{
    if ((source < 6u) && (index < 4u))
    {
        sNdsRendererAdapterStagePreviewActors[source][index] = (GObj *)gobj;
    }
}

#if NDS_P2_STAGE_SECTOR
extern DObj *ndsGRSectorPlatformDObj(void);
#endif

static s32 ndsRendererAdapterPrepareNativeStageOwnerBody(
    void *camera_gobj_ptr)
{
    NDSRendererAdapterNativeStageWorkspace *workspace =
        &sNdsRendererAdapterNativeStageWorkspace;
    NDSRelocLoadedFile *loaded[NDS_RENDERER_ADAPTER_STAGE_ASSET_COUNT];
    const u32 asset_count = ndsRendererAdapterNativeStageActiveAssetCount();
    GObj *camera_gobj = camera_gobj_ptr;
    CObj *cobj = (camera_gobj != NULL) ? CObjGetStruct(camera_gobj) : NULL;
    u32 topology_generation = 0u;
    u32 topology_stamp = 0u;
    sb32 topology_cached = FALSE;
    u32 i;
    sb32 steady_admitted = FALSE;
    /* UNCONDITIONAL. The reject label this feeds calls
     * ndsRendererHardwareAbortBattleStaticTextures, which discards the whole
     * hardware texture cache and leaves it unable to re-arm for the rest of the
     * scene. Which branch got there therefore cannot be a profile-only fact --
     * it is the difference between a next cycle that reads one counter and a
     * next cycle that re-derives six branches. See nds_renderer.c's row 6 note. */
    u32 task36_reject_reason = 1u;

    workspace->active = FALSE;
    /* P2-4: stage wallpapers are source SObjs on their own display link, so
     * they do not exist in the native DObj packet committed below. Bind the
     * converted BG2 wallpaper to the same live battle camera once per present.
     * Keep this before packet admission: an unrelated stage-packet decline
     * must not blank an otherwise valid native background. */
    if ((cobj != NULL) && (gSCManagerBattleState != NULL) &&
        ((u32)gSCManagerBattleState->gkind <= 8u) &&
        (gSCManagerSceneData.scene_curr != nSCKind1PTrainingMode) &&
        (sNdsRendererAdapterStagePreviewMode == 0u))
    {
        (void)ndsNativeBattleWallpaperDraw(
            (u32)gSCManagerBattleState->gkind,
            cobj->vec.eye.x, cobj->vec.eye.y, cobj->vec.eye.z,
            cobj->vec.at.x, cobj->vec.at.y, cobj->vec.at.z);
    }
    if (gNdsRendererFastRunMode !=
        NDS_RENDERER_FAST_RUN_NATIVE_COMPLETE_STAGE)
    {
        return FALSE;
    }
    if ((asset_count == 0u) || (asset_count > NDS_RENDERER_ADAPTER_STAGE_ASSET_COUNT))
    {
        return FALSE;
    }
    if (cobj == NULL)
    {
        task36_reject_reason = 2u;
        goto reject;
    }
    /* Task 44 item 3: steady-state admission. One generation compare plus the
     * cheap segment guard replaces the four asset lookups and the whole
     * topology stamp rebuild. Everything the fast path would have recomputed
     * (loaded[], asset_bases[], topology_generation, topology_stamp) is
     * already recorded in the workspace and provably unchanged, because every
     * seam that can change it bumps sNdsRelocStageAssetMutation. */
    if ((workspace->topology_valid != FALSE) &&
        (workspace->steady_admission_generation != 0u) &&
        (workspace->steady_admission_generation ==
         sNdsRelocStageAssetMutation) &&
        (ndsRendererAdapterNativeStageSegmentsUnchanged(workspace) != FALSE))
    {
        steady_admitted = TRUE;
        topology_generation = workspace->topology_generation;
        topology_stamp = workspace->topology_stamp;
        topology_cached = TRUE;
    }
    if (steady_admitted == FALSE)
    {
    for (i = 0u; i < asset_count; i++)
    {
        loaded[i] = ndsRelocFindLoadedFileByAsset(
            ndsRendererAdapterNativeStageAssetId(i));
        if ((loaded[i] == NULL) || (loaded[i]->data == NULL) ||
            (loaded[i]->data_size !=
             ndsRendererAdapterNativeStageAssetSize(i)) ||
            (loaded[i]->owner_generation == 0u) ||
            ((i != 0u) &&
             (loaded[i]->owner_generation != topology_generation)))
        {
            task36_reject_reason = 3u;
            goto reject;
        }
        topology_generation = loaded[i]->owner_generation;
    }
    if ((workspace->topology_valid != FALSE) &&
        (workspace->topology_generation == topology_generation))
    {
        topology_cached = TRUE;
        for (i = 0u; i < asset_count; i++)
        {
            if (workspace->loaded[i] != loaded[i])
            {
                topology_cached = FALSE;
                break;
            }
        }
        if ((topology_cached != FALSE) &&
            ((ndsRendererAdapterBuildNativeStageTopologyStamp(
                  workspace, topology_generation, &topology_stamp) == FALSE) ||
             (topology_stamp != workspace->topology_stamp)))
        {
            topology_cached = FALSE;
        }
    }
    if (topology_cached == FALSE)
    {
        bzero(workspace, sizeof(*workspace));
        /* Slice 44. binding_dobjs[] is about to be re-collected from the live
         * tree, so every stage world entry keyed on an old DObj address is now
         * meaningless -- and a recycled heap address makes one of them *match*.
         * That was harmless while `validated_frame == frame` forced a rebuild
         * every frame; under the stride a collision would hand back the
         * previous topology's matrix. Free at runtime: this branch is the
         * once-per-scene rebuild that already bzeroes the whole workspace. */
        sNdsRendererAdapterStageWorldCacheCount = 0u;
        memset(sNdsRendererAdapterStageWorldIndex, 0,
               sizeof(sNdsRendererAdapterStageWorldIndex));
        for (i = 0u; i < asset_count; i++)
        {
            workspace->loaded[i] = loaded[i];
            workspace->frame.asset_bases[i] = loaded[i]->data;
        }
        if ((ndsRendererAdapterCollectNativeStageTopology(workspace) == FALSE) ||
            (ndsRendererAdapterCaptureTask36StageWorld(workspace) == FALSE) ||
            (ndsRendererAdapterBuildNativeStageTopologyStamp(
                 workspace, topology_generation, &topology_stamp) == FALSE))
        {
            task36_reject_reason = 4u;
            goto reject;
        }
        workspace->topology_generation = topology_generation;
        workspace->topology_stamp = topology_stamp;
        workspace->topology_valid = TRUE;
    }
    else
    {
        for (i = 0u; i < asset_count; i++)
        {
            workspace->frame.asset_bases[i] = loaded[i]->data;
        }
    }
    /* Static DS payloads are source-independent once converted, but their
     * cache keys contain the live O2R source pointers. A stage asset may be
     * replaced at a new taskman-heap address after pre-GO texture preparation;
     * Task 44 already routes every such mutation through this full-validation
     * arm. Refresh only those pointer-derived key words here. This performs no
     * file I/O, conversion, allocation, or VRAM upload. */
    if (ndsRendererHardwareRefreshBattleStaticTexturePointers() == FALSE)
    {
        task36_reject_reason = 3u;
        goto reject;
    }
    /* Only a completed full validation may arm the fast path. */
    workspace->steady_admission_generation = sNdsRelocStageAssetMutation;
    }
    /* Slice 44: one advance per frame, here, because this is the only site the
     * c120 profile shows running exactly once -- 26 rigid checks a frame against
     * a 26-entry list. Both the rigid sweep below and the dynamic chain walk in
     * ndsRendererAdapterPrepareNativeStageMatrices read the cursor, so they stay
     * in phase and every frame does the same amount of work. */
    workspace->slice44_validate_cursor =
        (u8)((workspace->slice44_validate_cursor + 1u) %
             NDS_R2_STAGE_VALIDATE_STRIDE);
    ndsRendererAdapterValidateTask36StageWorld(workspace);
    if (ndsRendererAdapterPrepareNativeStageMatrices(cobj, workspace) == FALSE)
    {
        task36_reject_reason = 5u;
        goto reject;
    }
    if (ndsRendererAdapterPrepareNativeStageMaterials(workspace) == FALSE)
    {
        task36_reject_reason = 7u;
        goto reject;
    }

    bzero(&workspace->resolver, sizeof(workspace->resolver));
    workspace->resolver.primary_file = workspace->loaded[0];
    workspace->config = (NDSRendererConfig){0};
    workspace->config.max_depth = 8u;
    workspace->config.max_commands = 2048u;
    workspace->config.max_list_commands = 512u;
    workspace->config.texture_data_layout =
        NDS_RENDERER_TEXTURE_DATA_O2R_WORD_SWAPPED;
    workspace->config.validate_range = ndsRendererAdapterStageValidateRange;
    workspace->config.immutable_command_span =
        ndsRendererAdapterImmutableCommandSpan;
    workspace->config.resolve_branch = ndsFighterDLDrawResolveBranch;
    workspace->config.resolve_data = ndsFighterDLDrawResolveRendererData;
    workspace->config.user = &workspace->resolver;
    workspace->frame.dobjs = workspace->live_dobjs;
    workspace->frame.binding_display_lists =
        workspace->binding_display_lists;
    workspace->frame.projection = &workspace->projection;
    workspace->frame.camera_modelview = &workspace->camera_modelview;
    workspace->frame.binding_world = workspace->binding_world;
    workspace->frame.rigid_binding_mask =
        workspace->task36_runtime_rigid_mask;
    workspace->frame.binding_composed = workspace->binding_composed;
    workspace->frame.materials = workspace->materials;
    workspace->frame.config = &workspace->config;
    workspace->frame.topology_generation = workspace->topology_generation;
    workspace->frame.topology_stamp = workspace->topology_stamp;
    workspace->frame.hidden_binding_mask = 0u;
    {
#if NDS_P2_STAGE_SECTOR
        /* Owner (r58): the Sector Z wing-platform collision proxy is not
         * drawn (ndsGRSectorPlatformDObj). */
        const DObj *hidden_dobj = ndsGRSectorPlatformDObj();
#else
        const DObj *hidden_dobj = NULL;
#endif

        for (i = 0u; i < workspace->binding_count; i++)
        {
            if (((workspace->binding_dobjs[i]->flags & DOBJ_FLAG_NOTEXTURE) !=
                 0u) ||
                (ndsRendererAdapterStageDObjHiddenInTree(
                     workspace->binding_dobjs[i]) != FALSE) ||
                ((hidden_dobj != NULL) &&
                 (workspace->binding_dobjs[i] == hidden_dobj)))
            {
                workspace->frame.hidden_binding_mask |= (u64)1u << i;
            }
        }
    }
    if (ndsRendererPrepareNativeStageOwner(
            &workspace->frame, &workspace->stats) == FALSE)
    {
        workspace->topology_valid = FALSE;
        task36_reject_reason = 6u;
        goto reject;
    }
    workspace->next_segment = 0u;
    workspace->active = TRUE;
    return TRUE;

reject:
    /* BUGS row 6. Published unconditionally, and the FIRST reason is latched
     * rather than only the last: the abort below is irreversible for the scene,
     * so the branch that caused it is the one worth keeping. A later reject is
     * a consequence of the first one, not independent evidence. */
    NDS_DIAG(gNdsRendererStageOwnerRejectCount++);
    gNdsRendererStageOwnerLastRejectReason = task36_reject_reason;
    if (gNdsRendererStageOwnerFirstRejectReason == 0u)
    {
        gNdsRendererStageOwnerFirstRejectReason = task36_reject_reason;
    }
    workspace->active = FALSE;
    /* A FAILED DISPLAY-GRAPH PREPARE SAYS NOTHING ABOUT TEXTURE RESIDENCY, AND
     * THIS IS WHERE IT USED TO CLAIM OTHERWISE.
     *
     * This block was:
     *
     *     if (gNdsRendererBattleStaticTextureArmCount != 0u)
     *         ndsRendererHardwareAbortBattleStaticTextures();
     *
     * which discards the entire hardware texture cache and clears
     * sNdsRendererBattleStaticTexturePrepared. Both Arm call sites
     * (taskman_seam.c:5251 and :7975) fire only on the Wait->Go TRANSITION, so
     * they never run again inside a match -- and Arm refuses to re-arm without
     * Prepared anyway. One transient frame therefore dropped the 24 pinned
     * statics for the remainder of the match with no path back.
     *
     * Measured on 2026-08-04 (build-row6-v1, two-death match): at frame 427,
     * right after a star KO, abort went 0->1, preparednow 1->0, and
     * staticpin froze at 7144 and never moved again through frame 2174. The
     * scene stayed correctly textured -- the dynamic cache absorbed it -- but
     * frame rate fell from 27.9 to 20.0 for the rest of the match, and the
     * pinned corpus was gone for good.
     *
     * Nothing here justified that. The pinned corpus is uploaded at scene
     * prepare and is untouched by whatever made this frame's display graph
     * unusable; workspace->active = FALSE above is already the whole fallback
     * contract, and the generic path does not read the workspace. So the
     * correct behaviour on a post-arm reject is to degrade for THIS FRAME and
     * leave every texture exactly where it is, which also leaves the next
     * frame free to succeed normally.
     *
     * The event stays loud rather than becoming silent: the reject counters
     * above fire unconditionally, and this one isolates the post-arm case that
     * used to be destructive, so a regression here is still one counter away.
     */
    if (gNdsRendererBattleStaticTextureArmCount != 0u)
    {
        NDS_DIAG(gNdsRendererStageOwnerPostArmRejectCount++);
    }
    return FALSE;
}

/* The tick HUD's STG span for a committed segment (see the caller,
 * ndsStageGCDrawAllLoopRecordCapturedDisplay). */
#if NDS_TICK_HUD
extern volatile u32 gNdsTickHudStageTicks;
#define NDS_STAGE_DISPLAY_SPAN_BEGIN() u32 stage_span_start_ = NDS_TICK_HUD_SPAN_CLOCK()
#define NDS_STAGE_DISPLAY_SPAN_END() \
    (gNdsTickHudStageTicks += NDS_TICK_HUD_SPAN_CLOCK() - stage_span_start_)
#else
#define NDS_STAGE_DISPLAY_SPAN_BEGIN() ((void)0)
#define NDS_STAGE_DISPLAY_SPAN_END() ((void)0)
#endif

/* ndsRendererCommitNativeStageSegment on the DTCM hot stack. */
static unsigned int ndsRendererAdapterCommitSegmentOnHotStack(void *arg)
{
    return (unsigned int)ndsRendererCommitNativeStageSegment(
        *(const u32 *)arg);
}

s32 __attribute__((section(".itcm")))
ndsRendererAdapterCommitNativeStageDisplay(
    void *display_gobj_ptr, s32 link_id)
{
    NDSRendererAdapterNativeStageWorkspace *workspace =
        &sNdsRendererAdapterNativeStageWorkspace;
    GObj *display_gobj = display_gobj_ptr;
    u32 segment_count;
    u32 i;

    if (workspace->active == FALSE)
    {
        return FALSE;
    }
    /* Called for every display GObj of the stage camera, ~34 of ~42 of them
     * no segment: a GObj whose bit is clear matches no entry. */
    if ((workspace->segment_bloom &
         NDS_STAGE_SEGMENT_BLOOM_BIT(display_gobj)) == 0u)
    {
        return FALSE;
    }
    /* Read the loaded descriptor's count once, not once per compared
     * segment. */
    segment_count = ndsRendererAdapterNativeStageActiveSegmentCount();
    for (i = 0u; i < segment_count; i++)
    {
        if (display_gobj == workspace->segments[i])
        {
            NDS_STAGE_DISPLAY_SPAN_BEGIN();

            /* Presentation order, not packet order: the owner commits each
             * segment once, whenever its GObj is drawn (Saffron's gate,
             * segment 3, precedes segment 2). next_segment counts commits. */
            if ((u32)link_id != ndsRendererAdapterNativeStageSegmentLink(i))
            {
                (void)ndsRendererCommitNativeStageSegment(0xffffffffu);
                NDS_STAGE_DISPLAY_SPAN_END();
                return TRUE;
            }
            ndsRendererAdapterCommitNativeStageMaterials(workspace, i);
            if (ndsDtcmHotStackRun(ndsRendererAdapterCommitSegmentOnHotStack,
                                   &i) == FALSE)
            {
                ndsRendererFinishNativeStageOwner();
                workspace->active = FALSE;
                NDS_STAGE_DISPLAY_SPAN_END();
                return FALSE;
            }
            workspace->next_segment++;
            NDS_STAGE_DISPLAY_SPAN_END();
            return TRUE;
        }
    }
    return FALSE;
}

void ndsRendererAdapterFinishNativeStageOwner(void)
{
    NDSRendererAdapterNativeStageWorkspace *workspace =
        &sNdsRendererAdapterNativeStageWorkspace;

    if (workspace->active != FALSE)
    {
        ndsRendererFinishNativeStageOwner();
    }
    workspace->active = FALSE;
    workspace->next_segment = 0u;
}

/* Always compiled, unlike the validator below: this counts how often the
 * material write walk's capacity guard actually fires, and a guard whose hit
 * count nobody can read is indistinguishable from one that never runs. */
volatile u32 gNdsR2MaterialWalkBoundHits;


/* Cycle 110. The fighter material snapshot is a pure function of `mobj->sub`
 * plus texture_id_curr/next, lfrac and palette_id -- and it never varies. The
 * NDS_TICK_HUD census that has sat in this file since cycle 98 answered it on a
 * 60-second match: 20,100 builds, 20,069 of them byte-identical to the previous
 * snapshot for the same MObj, 31 first-sights, and **zero** variants. So ~761
 * cycles a build, about twelve times a frame, reconstruct a constant --
 * 13,176 ticks/frame of it, 2,124 of which is the single `mobj->sub.flags`
 * load missing cache at 139 cycles an execution.
 *
 * The key hashes the COMPLETE input set rather than the fields I believe can
 * animate. That is the difference between a skip that is correct by
 * construction and one that is correct until someone adds a texture-scroll
 * track: the builder reads nothing outside that set, so equal inputs means
 * equal output with only a 2^-32 collision to argue about. The heap generation
 * is in the key because MObj pointers are taskman-arena addresses and a scene
 * rewind reuses them.
 *
 * The build's side effect -- writing texture_id_curr/next back into the MObj --
 * is safe to skip for the same reason: the stored hash is taken AFTER the
 * write-back, so a match means the write would store what is already there.
 *
 * Only the production path passes keys. The hierarchy path's materials array is
 * a caller local that does not survive the frame, so it passes NULL and always
 * builds. After this the census counts REBUILDS, which makes gNdsFtrPreMatCalls
 * engagement proof: it should read tens, not tens of thousands. */
#if NDS_TICK_HUD
/* Engagement proof. gNdsFtrPreMatCalls cannot answer this: it counts calls that
 * reach the census inside the build wrapper, and the hierarchy call site passes
 * no keys, so a skipping production path and a never-skipping one can produce
 * the same census. These two count the decision itself. */
volatile u32 gNdsR2MatKeySkip;
volatile u32 gNdsR2MatKeyBuild;
/* And WHY a build happened, because the two answers point at different fixes.
 * Identity: the row holds a different MObj than last frame, so the block for
 * this one is sitting in some other row -- fix is a stable row assignment.
 * Inputs: same MObj, hash moved -- fix would be a narrower key, which is
 * already refuted. Guessing between them once cost a build. */
volatile u32 gNdsR2MatKeyMissIdentity;
volatile u32 gNdsR2MatKeyMissInputs;
#endif

/* Hashes all 30 words of MObjSub, including the six the builder never reads
 * (sub.unk48, sub.unk4C, sub.unk68..unk74). Narrowing it to the builder's exact
 * read set was tried and is REFUTED: the engagement counters came back
 * bit-identical -- 28,786 skips and 30,606 builds either way -- and FTR rose
 * 1,155. The rebuilds are not caused by those words at all. They are
 * `keys[count].mobj != mobj`: the materials array is indexed by (selected-root
 * slot, chain position), and which DObj lands in slot i rotates between frames,
 * so about half the lookups find the right block under the wrong index.
 *
 * Recovering that half needs a per-MObj store -- 33 live MObjs x 100 bytes plus
 * keys, about 7 KB of bss against an arena whose low-water is already under the
 * GObj-cap threshold -- or a stable slot assignment. Neither is this slice.
 * Since the narrow hash bought nothing, keep the one that needs no field audit
 * to stay correct. */
static u32 ndsRendererAdapterMaterialRow(DObj *dobj, u32 fallback_row)
{
    u32 base;
    u32 probe;

    if (dobj == NULL)
    {
        sNdsRendererAdapterMaterialRowClaimMask |= 1u << fallback_row;
        return fallback_row;
    }
    if (sNdsRendererAdapterMaterialRowGeneration != gNdsTaskmanHeapGeneration)
    {
        u32 j;

        for (j = 0u; j < NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED; j++)
        {
            sNdsRendererAdapterMaterialRowOwner[j] = NULL;
        }
        sNdsRendererAdapterMaterialRowGeneration = gNdsTaskmanHeapGeneration;
        sNdsRendererAdapterMaterialRowClaimMask = 0u;
    }
    /* Multiplicative, not `>> 4`: DObjs are allocated contiguously and a shift
     * hash strided them onto a handful of rows, so the probe loop ran several
     * iterations and the whole lookup measured 106 cycles a call (2,024
     * ticks/frame) on a 128-byte table that is always in cache. */
    base = ((u32)(uintptr_t)dobj * 2654435761u) >>
        (32u - NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED_LOG2);
    for (probe = 0u; probe < NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED; probe++)
    {
        u32 row = (base + probe) & (NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED - 1u);

        if (sNdsRendererAdapterMaterialRowOwner[row] == dobj)
        {
            /* The display contract can reference the same material DObj more
             * than once. Sharing in that case is source-equivalent and safe. */
            sNdsRendererAdapterMaterialRowClaimMask |= 1u << row;
            return row;
        }
        if ((sNdsRendererAdapterMaterialRowOwner[row] == NULL) &&
            ((sNdsRendererAdapterMaterialRowClaimMask & (1u << row)) == 0u))
        {
            sNdsRendererAdapterMaterialRowOwner[row] = dobj;
            sNdsRendererAdapterMaterialRowClaimMask |= 1u << row;
            return row;
        }
    }
    /* Every row is owned by some persistent DObj. Reclaim one that this CURRENT
     * owner has not claimed. The input key catches the changed identity and
     * rebuilds that row. selected_count cannot exceed the row count, so unless
     * duplicate DObjs reduced the number of claims, an unclaimed row is always
     * available here. */
    for (probe = 0u; probe < NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED; probe++)
    {
        u32 row = (base + probe) & (NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED - 1u);

        if ((sNdsRendererAdapterMaterialRowClaimMask & (1u << row)) == 0u)
        {
            sNdsRendererAdapterMaterialRowOwner[row] = dobj;
            sNdsRendererAdapterMaterialRowClaimMask |= 1u << row;
            return row;
        }
    }
    /* Defensive only: the collection bound above makes this unreachable. */
    return fallback_row;
}

/* The nine words that actually move during a match, in the two contiguous runs
 * they occupy. `primcolor` through `light2color` is `MObjSub` 0x50..0x67 -- the
 * five colour tracks gcPlayMObjMatAnim writes plus the prim level/min byte pair
 * that shares their run -- and texture_id_curr through palette_id is the twelve
 * bytes immediately after `sub`. Two cache lines instead of five, nine
 * multiply-accumulates instead of thirty-four. */
static u32 __attribute__((section(".itcm")))
ndsRendererAdapterMaterialAnimHash(const MObj *mobj)
{
    const u32 *colors = (const u32 *)(const void *)&mobj->sub.primcolor;
    u32 hash = 2166136261u;
    u32 i;

    for (i = 0u; i < 6u; i++)
    {
        hash = (hash ^ colors[i]) * 16777619u;
    }
    hash = (hash ^ (((u32)mobj->texture_id_curr << 16) |
                    (u32)mobj->texture_id_next)) * 16777619u;
    hash = (hash ^ *(const u32 *)(const void *)&mobj->lfrac) * 16777619u;
    hash = (hash ^ *(const u32 *)(const void *)&mobj->palette_id) * 16777619u;
    return hash;
}

/* `light2color` is the last field of the colour run, so the six words starting
 * at `primcolor` must land exactly on it. If MObjSub is ever reordered this
 * stops compiling rather than silently hashing the wrong bytes. */
_Static_assert(offsetof(MObjSub, light2color) ==
                   offsetof(MObjSub, primcolor) + 20u,
               "material anim hash assumes primcolor..light2color are six "
               "contiguous words");

/* P2-2 fighter packet: the identity of everything the current fighter's
 * material rows were built from -- every prepared MObj's animation hash and
 * pointer plus the colour modulate -- folded into one word per draw and handed
 * to the production owner as its packet key. The hashes already exist (the
 * material memo computes them per MObj per frame), so this is a few XORs. */
static u32 sNdsFighterPacketMaterialIdentity;

/* The animation-state hash and identity of every material the selected roots
 * carry, from the live MObj chains alone -- no rows, no snapshots -- so the
 * adapter can ask whether the packet will replay before it spends the
 * preparation on a frame whose replay reads none of it. Taken before any
 * build on the record path too, so the key means one thing on both paths. */
static u32 ndsRendererAdapterMaterialIdentity(
    DObj *const *material_dobjs, u32 count)
{
    u32 identity = 2166136261u;
    u32 i;

    for (i = 0u; i < count; i++)
    {
        const DObj *dobj = material_dobjs[i];
        const MObj *mobj;

        for (mobj = (dobj != NULL) ? dobj->mobj : NULL;
             mobj != NULL;
             mobj = mobj->next)
        {
            identity = (identity ^ ndsRendererAdapterMaterialAnimHash(mobj)) *
                       16777619u;
            identity ^= (u32)(uintptr_t)mobj;
        }
    }
    return identity;
}

static sb32 ndsRendererAdapterPrepareNativeMaterials(
    DObj *dobj, NDSRendererNativeMaterial *materials,
    u32 capacity, u32 *out_count,
    NDSRendererAdapterMaterialKey *keys,
    s32 *save_curr, s32 *save_next)
{
    MObj *mobj;
    u32 count = 0u;

    if ((materials == NULL) || (out_count == NULL))
    {
        return FALSE;
    }
    *out_count = 0u;
    if ((dobj == NULL) || (dobj->mobj == NULL))
    {
        return TRUE;
    }
    /* There is no counting pre-pass any more. It walked the whole MObj chain a
     * second time -- a dependent pointer chase, 37 chains a frame, 1,215
     * ticks/frame on its `mobj = mobj->next` alone in the c115 per-PC census --
     * purely so an over-capacity chain could be rejected before anything was
     * written. The write walk below already carries that bound of its own, and
     * since the rollback slice it also reports `*out_count` on rejection, so the
     * caller undoes exactly the entries this walk touched. Two passes proved one
     * fact; one pass proves it at the point of use.
     *
     * The chain validator measured 13,938 chains of ONE node against a capacity
     * of four, so the difference between rejecting before and rejecting during
     * is a path that has never been taken. */
    for (mobj = dobj->mobj; mobj != NULL; mobj = mobj->next)
    {
        /* Bound the WRITE walk too, not just the counting one above. The count
         * pass only constrains this pass if the list is identical across both,
         * and it is not: ndsRendererAdapterBuildNativeMaterial is the
         * advance_texture_ids=TRUE wrapper, so this loop writes
         * mobj->texture_id_curr/next into every node as it walks. A list that
         * turns cyclic or is corrupted mid-walk ran `materials[count]` off the
         * end of a `capacity`-entry array -- capacity is 4 -- with no check at
         * all. Returning FALSE hands the caller its existing generic fallback
         * instead of corrupting whatever follows the array.
         *
         * Found while reproducing the Sudden Death freeze (docs/BUGS.md): the
         * scene presents two frames and then none, with no overflow assert
         * firing anywhere, and an interrupt landing inside this loop's inlined
         * material build. */
        if (count >= capacity)
        {
            /* Engagement proof, and it is NOT decoration. The Sudden Death
             * freeze stopped when this guard went in, but the default-off chain
             * validator then found 13,938 clean chains of ONE node against a
             * capacity of four -- so on that evidence this branch can never be
             * reached, and "the guard fixed the freeze" and "the chain is fine"
             * cannot both be true. This counter is what tells them apart: a run
             * that is freeze-free with this still at zero proves the guard was
             * not the cure and the real cause is still live. */
            NDS_DIAG(gNdsR2MaterialWalkBoundHits++);
            /* Report what the snapshot holds so the caller rolls back exactly
             * the entries this walk mutated, no more and no fewer. */
            *out_count = count;
            return FALSE;
        }
        if (save_curr != NULL)
        {
            /* The rollback snapshot, taken here rather than in a walk of its
             * own. ndsRendererAdapterSaveNativeMaterialTextureIds was a second
             * pass over the same chain reading the same two fields -- 3,429
             * ticks/frame in the c110 profile -- and the hash below loads them
             * anyway, so this costs two stores into a line the caller owns. */
            save_curr[count] = mobj->texture_id_curr;
            save_next[count] = mobj->texture_id_next;
        }
        if (keys != NULL)
        {
            u32 hash = ndsRendererAdapterMaterialAnimHash(mobj);

            if ((keys[count].mobj == mobj) &&
                (keys[count].heap_generation == gNdsTaskmanHeapGeneration) &&
                (keys[count].hash == hash))
            {
#if NDS_TICK_HUD
                NDS_DIAG(gNdsR2MatKeySkip++);
#endif
                count++;
                continue;
            }
#if NDS_TICK_HUD
            NDS_DIAG(gNdsR2MatKeyBuild++);
            if (keys[count].mobj != mobj)
            {
                NDS_DIAG(gNdsR2MatKeyMissIdentity++);
            }
            else
            {
                NDS_DIAG(gNdsR2MatKeyMissInputs++);
            }
#endif
        }
        if (ndsRendererAdapterBuildNativeMaterial(
                mobj, &materials[count]) == FALSE)
        {
            if (keys != NULL)
            {
                keys[count].mobj = NULL;
            }
            /* count + 1: entry `count` was snapshotted above and the builder is
             * the advance_texture_ids=TRUE wrapper, so it may have written this
             * MObj's ids before failing. Restoring an untouched entry writes
             * back what is already there, so over-reporting by one is safe and
             * under-reporting is not. */
            *out_count = count + 1u;
            return FALSE;
        }
        if (keys != NULL)
        {
            /* After the build, so the stored hashes describe the MObj the build
             * left behind -- it writes texture_id_curr/next back. Both are
             * stored on every build: the full one is only CHECKED periodically,
             * but it has to be current whenever that check lands. */
            keys[count].mobj = mobj;
            keys[count].heap_generation = gNdsTaskmanHeapGeneration;
            keys[count].hash = ndsRendererAdapterMaterialAnimHash(mobj);
        }
        count++;
    }
    *out_count = count;
    return TRUE;
}

static void ndsRendererAdapterRestoreNativeMaterialTextureIds(
    DObj *dobj,
    const s32 *curr,
    const s32 *next,
    u32 count)
{
    MObj *mobj;
    u32 i = 0u;

    if ((dobj == NULL) || (curr == NULL) || (next == NULL))
    {
        return;
    }
    for (mobj = dobj->mobj;
         (mobj != NULL) && (i < count);
         mobj = mobj->next, i++)
    {
        mobj->texture_id_curr = curr[i];
        mobj->texture_id_next = next[i];
    }
}

static sb32 ndsRendererAdapterValidateNativeOwnerMaterials(
    const NDSRendererNativeMaterial *materials,
    u32 material_count)
{
    u32 i;

    if ((material_count != 0u) && (materials == NULL))
    {
        return FALSE;
    }
    for (i = 0u; i < material_count; i++)
    {
        const NDSRendererNativeMaterial *material = &materials[i];
        u32 effects = material->effects;

        if (((effects & NDS_RENDERER_NATIVE_MATERIAL_PALETTE_IMAGE) != 0u) &&
            (ndsRelocFindLoadedFileContaining(
                 (const void *)(uintptr_t)material->palette_image,
                 1u) == NULL))
        {
            return FALSE;
        }
        if (((effects & NDS_RENDERER_NATIVE_MATERIAL_BLOCK_IMAGE) != 0u) &&
            (ndsRelocFindLoadedFileContaining(
                 (const void *)(uintptr_t)material->block_image,
                 1u) == NULL))
        {
            return FALSE;
        }
        if (((effects & NDS_RENDERER_NATIVE_MATERIAL_CURRENT_IMAGE) != 0u) &&
            (ndsRelocFindLoadedFileContaining(
                 (const void *)(uintptr_t)material->current_image,
                 1u) == NULL))
        {
            return FALSE;
        }
    }
    return TRUE;
}

static void ndsRendererAdapterRestoreNativeOwnerMaterialTextureIds(
    DObj *const *material_dobjs,
    u32 root_count)
{
    if (material_dobjs == NULL)
    {
        return;
    }
    /* A contract may select one material DObj more than once. Roll back in
     * reverse event order so each saved pre-event state is restored and the
     * earliest snapshot remains live for the ordinary renderer fallback. */
    while (root_count != 0u)
    {
        u32 root_index = --root_count;

        ndsRendererAdapterRestoreNativeMaterialTextureIds(
            material_dobjs[root_index],
            sNdsRendererAdapterNativeOwnerTextureCurr[root_index],
            sNdsRendererAdapterNativeOwnerTextureNext[root_index],
            sNdsRendererAdapterNativeOwnerTextureCounts[root_index]);
    }
}

/* A plan hit skips this outright (see "THE DELETION" at the plan-hit branch), and
 * the plan hits every frame -- the c112 cold map found its body inside the
 * driver's third-largest never-executed run. */
static sb32 __attribute__((noinline, cold, optimize("Os")))
ndsRendererAdapterValidateNativeOwnerCached(
    u32 slot,
    u32 battle_slot,
    u32 use_low_detail,
    const NDSRelocLoadedFile *owner_file,
    u32 root_count,
    const u32 *root_offsets,
    const u32 *material_counts)
{
    NDSRendererAdapterNativeOwnerValidationCache *cache;
    NDSRendererAdapterNativeOwnerValidationCache *victim;
    u32 e;
    u32 i;

    if ((slot >= NDS_RENDERER_NATIVE_FIGHTER_OWNER_COUNT) ||
        (owner_file == NULL) ||
        (root_offsets == NULL) || (material_counts == NULL) ||
        (root_count > NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED))
    {
#if NDS_TICK_HUD
        gNdsFtrPreValidateReject++;
#endif
        return FALSE;
    }
    victim = &sNdsRendererAdapterNativeOwnerValidationCache[0];
    for (e = 0u; e < NDS_RENDERER_ADAPTER_OWNER_VALIDATION_POOL; e++)
    {
        cache = &sNdsRendererAdapterNativeOwnerValidationCache[e];
        if (cache->valid == 0u)
        {
            victim = cache;
            continue;
        }
        if ((victim->valid != 0u) && (cache->stamp < victim->stamp))
        {
            victim = cache;
        }
        if ((cache->slot != slot) ||
            (cache->data != owner_file->data) ||
            (cache->asset_id != owner_file->asset_id) ||
            (cache->owner_generation != owner_file->owner_generation) ||
            (cache->data_size != owner_file->data_size) ||
            (cache->root_count != root_count) ||
            (cache->battle_slot != battle_slot) ||
            (cache->use_low_detail != use_low_detail))
        {
            continue;
        }
        for (i = 0u; i < root_count; i++)
        {
            if ((cache->root_offsets[i] != root_offsets[i]) ||
                (cache->material_counts[i] != material_counts[i]))
            {
                break;
            }
        }
        if (i == root_count)
        {
#if NDS_TICK_HUD
            /* Cycle 98. THE counter this row existed to add: without it a
             * cache hit and a cache miss are indistinguishable here, and
             * deleting work whose cache already hits is the mistake cycle 93
             * avoided on the stage. */
            gNdsFtrPreValidateReuse++;
#endif
            cache->stamp = ++sNdsRendererAdapterNativeOwnerValidationStamp;
            return TRUE;
        }
    }

#if NDS_TICK_HUD
    gNdsFtrPreValidateBuild++;
#endif
    if (ndsRendererValidateNativeFighterOwner(
            slot, battle_slot, use_low_detail,
            ndsRelocNativeSourceSize(owner_file), root_count,
            root_offsets, material_counts) == FALSE)
    {
        return FALSE;
    }
    cache = victim;
    cache->slot = slot;
    cache->stamp = ++sNdsRendererAdapterNativeOwnerValidationStamp;
    cache->data = owner_file->data;
    cache->asset_id = owner_file->asset_id;
    cache->owner_generation = owner_file->owner_generation;
    cache->data_size = owner_file->data_size;
    cache->root_count = root_count;
    cache->battle_slot = battle_slot;
    cache->use_low_detail = use_low_detail;
    for (i = 0u; i < root_count; i++)
    {
        cache->root_offsets[i] = root_offsets[i];
        cache->material_counts[i] = material_counts[i];
    }
    cache->valid = TRUE;
    return TRUE;
}

static Gfx *ndsRendererAdapterEmitMaterialCommands(Gfx *branch_dl, MObj *mobj)
{
    u32 flags = ndsRendererAdapterMaterialFlags(mobj);
    f32 scau = 0.0F;
    f32 scav = 0.0F;
    f32 trau = 0.0F;
    f32 trav = 0.0F;
    f32 scrollu = 0.0F;
    f32 scrollv = 0.0F;
    s32 uls;
    s32 ult;
    s32 s;
    s32 t;

    if ((branch_dl == NULL) || (mobj == NULL))
    {
        return branch_dl;
    }

    ndsRendererAdapterMaterialTextureState(
        mobj, flags, &scau, &scav, &trau, &trav, &scrollu, &scrollv);

    if (((flags & MOBJ_FLAG_PALETTE) == 0u) &&
        (mobj->sub.palettes != NULL))
    {
        const void *palette = ndsRendererAdapterReadPointerEntry(
            mobj->sub.palettes, (s32)mobj->palette_id);

        if (palette != NULL)
        {
            ndsRendererAdapterEmitTextureImage(
                branch_dl++, G_IM_FMT_RGBA, G_IM_SIZ_16b, 1u, palette);
        }
    }
    if ((flags & MOBJ_FLAG_PALETTE) != 0)
    {
        ndsRendererAdapterEmitTextureImage(
            branch_dl++, G_IM_FMT_RGBA, G_IM_SIZ_16b, 1u,
            ndsRendererAdapterReadPointerEntry(
                mobj->sub.palettes, (s32)mobj->palette_id));
        if ((flags & (MOBJ_FLAG_SPLIT | MOBJ_FLAG_ALPHA)) != 0)
        {
            ndsRendererAdapterEmitSync(branch_dl++,
                                       NDS_FIGHTER_DL_OP_RDPTILESYNC);
            ndsRendererAdapterEmitSetTile(
                branch_dl++, G_IM_FMT_RGBA, G_IM_SIZ_4b, 0u, 0x0100u, 5u,
                0u, NDS_RENDERER_ADAPTER_G_TX_WRAP,
                NDS_RENDERER_ADAPTER_G_TX_NOMASK,
                NDS_RENDERER_ADAPTER_G_TX_NOLOD,
                NDS_RENDERER_ADAPTER_G_TX_WRAP,
                NDS_RENDERER_ADAPTER_G_TX_NOMASK,
                NDS_RENDERER_ADAPTER_G_TX_NOLOD);
            ndsRendererAdapterEmitSync(branch_dl++,
                                       NDS_FIGHTER_DL_OP_RDPLOADSYNC);
            ndsRendererAdapterEmitLoadTlut(
                branch_dl++, 5u,
                (mobj->sub.siz == G_IM_SIZ_8b) ? 0xffu : 0x0fu);
            ndsRendererAdapterEmitSync(branch_dl++,
                                       NDS_FIGHTER_DL_OP_RDPPIPESYNC);
        }
    }
    if ((flags & MOBJ_FLAG_LIGHT1) != 0)
    {
        branch_dl = ndsRendererAdapterEmitLightColor(
            branch_dl, 1u,
            ndsRendererAdapterPackColor(&mobj->sub.light1color));
    }
    if ((flags & MOBJ_FLAG_LIGHT2) != 0)
    {
        branch_dl = ndsRendererAdapterEmitLightColor(
            branch_dl, 2u,
            ndsRendererAdapterPackColor(&mobj->sub.light2color));
    }
    if ((flags & (MOBJ_FLAG_PRIMCOLOR | MOBJ_FLAG_FRAC | 0x8u)) != 0)
    {
        if ((flags & MOBJ_FLAG_FRAC) != 0)
        {
            s32 trunc = (s32)mobj->lfrac;

            ndsRendererAdapterEmitPrimColor(
                branch_dl++, mobj->sub.prim_m,
                ndsRendererAdapterClampU8F32(
                    (mobj->lfrac - (f32)trunc) * 256.0F),
                mobj->sub.primcolor.s.r,
                mobj->sub.primcolor.s.g,
                mobj->sub.primcolor.s.b,
                mobj->sub.primcolor.s.a);
            mobj->texture_id_curr = trunc;
            mobj->texture_id_next = trunc + 1;
        }
        else
        {
            ndsRendererAdapterEmitPrimColor(
                branch_dl++, mobj->sub.prim_m,
                ndsRendererAdapterClampU8F32(mobj->lfrac * 255.0F),
                mobj->sub.primcolor.s.r,
                mobj->sub.primcolor.s.g,
                mobj->sub.primcolor.s.b,
                mobj->sub.primcolor.s.a);
        }
    }
    if ((flags & MOBJ_FLAG_ENVCOLOR) != 0)
    {
        ndsRendererAdapterEmitColor(
            branch_dl++, NDS_FIGHTER_DL_OP_SETENVCOLOR,
            mobj->sub.envcolor.s.r, mobj->sub.envcolor.s.g,
            mobj->sub.envcolor.s.b, mobj->sub.envcolor.s.a);
    }
    if ((flags & MOBJ_FLAG_BLENDCOLOR) != 0)
    {
        ndsRendererAdapterEmitColor(
            branch_dl++, NDS_FIGHTER_DL_OP_SETBLENDCOLOR,
            mobj->sub.blendcolor.s.r, mobj->sub.blendcolor.s.g,
            mobj->sub.blendcolor.s.b, mobj->sub.blendcolor.s.a);
    }
    if ((flags & (MOBJ_FLAG_FRAC | MOBJ_FLAG_SPLIT)) != 0)
    {
        s32 block_siz = (mobj->sub.block_siz == G_IM_SIZ_32b) ?
            G_IM_SIZ_32b : G_IM_SIZ_16b;

        ndsRendererAdapterEmitTextureImage(
            branch_dl++, mobj->sub.block_fmt, (u32)block_siz, 1u,
            ndsRendererAdapterReadPointerEntry(
                mobj->sub.sprites, mobj->texture_id_next));
        if ((flags & (MOBJ_FLAG_FRAC | MOBJ_FLAG_ALPHA)) != 0)
        {
            u32 texels = 0u;
            u32 dxt = 0u;

            ndsRendererAdapterMaterialLoadBlock(mobj, &texels, &dxt);
            ndsRendererAdapterEmitSync(branch_dl++,
                                       NDS_FIGHTER_DL_OP_RDPLOADSYNC);
            ndsRendererAdapterEmitLoadBlock(branch_dl++, 6u, 0u, 0u,
                                            texels, dxt);
            ndsRendererAdapterEmitSync(branch_dl++,
                                       NDS_FIGHTER_DL_OP_RDPLOADSYNC);
        }
    }
    if ((flags & (MOBJ_FLAG_FRAC | MOBJ_FLAG_ALPHA)) != 0)
    {
        ndsRendererAdapterEmitTextureImage(
            branch_dl++, mobj->sub.fmt, mobj->sub.siz, 1u,
            ndsRendererAdapterReadPointerEntry(
                mobj->sub.sprites, mobj->texture_id_curr));
    }
    if ((flags & 0x20u) != 0)
    {
        if (mobj->sub.unk10 == 2)
        {
            uls = (ABSF(scau) > (1.0F / 65535.0F)) ?
                (s32)((((f32)mobj->sub.unk0C * trau) / scau) * 4.0F) : 0;
            ult = (ABSF(scav) > (1.0F / 65535.0F)) ?
                (s32)((((f32)mobj->sub.unk0E * trav) / scav) * 4.0F) : 0;
            if (uls < 0)
            {
                uls = 0;
            }
            if (ult < 0)
            {
                ult = 0;
            }
        }
        else
        {
            uls = (ABSF(scau) > (1.0F / 65535.0F)) ?
                (s32)(((((f32)mobj->sub.unk0C * trau) +
                         (f32)mobj->sub.unk0A) / scau) * 4.0F) : 0;
            ult = (ABSF(scav) > (1.0F / 65535.0F)) ?
                (s32)((((((1.0F - scav) - trav) *
                          (f32)mobj->sub.unk0E) +
                         (f32)mobj->sub.unk0A) / scav) * 4.0F) : 0;
        }
        ndsRendererAdapterEmitTileSize(
            branch_dl++, NDS_RENDERER_ADAPTER_G_TX_RENDERTILE, uls, ult,
            (((s32)mobj->sub.unk0C - 1) << 2) + uls,
            (((s32)mobj->sub.unk0E - 1) << 2) + ult);
    }
    if ((flags & 0x40u) != 0)
    {
        uls = (ABSF(scau) > (1.0F / 65535.0F)) ?
            (s32)(((((f32)mobj->sub.unk38 * scrollu) +
                     (f32)mobj->sub.unk0A) / scau) * 4.0F) : 0;
        ult = (ABSF(scav) > (1.0F / 65535.0F)) ?
            (s32)((((((1.0F - scav) - scrollv) *
                      (f32)mobj->sub.unk3A) +
                     (f32)mobj->sub.unk0A) / scav) * 4.0F) : 0;
        ndsRendererAdapterEmitTileSize(
            branch_dl++, 1u, uls, ult,
            (((s32)mobj->sub.unk38 - 1) << 2) + uls,
            (((s32)mobj->sub.unk3A - 1) << 2) + ult);
    }
    if ((flags & MOBJ_FLAG_TEXTURE) != 0)
    {
        if (mobj->sub.unk10 == 2)
        {
            s = (ABSF(scau) > (1.0F / 65535.0F)) ?
                (s32)(((f32)mobj->sub.unk0C * 64.0F) / scau) : 0;
            t = (ABSF(scav) > (1.0F / 65535.0F)) ?
                (s32)(((f32)mobj->sub.unk0E * 64.0F) / scav) : 0;
        }
        else
        {
            s = ((mobj->sub.unk08 != 0) &&
                 (ABSF(scau) > (1.0F / 65535.0F))) ?
                (s32)((2097152.0F / (f32)mobj->sub.unk08) / scau) : 0;
            t = ((mobj->sub.unk08 != 0) &&
                 (ABSF(scav) > (1.0F / 65535.0F))) ?
                (s32)((2097152.0F / (f32)mobj->sub.unk08) / scav) : 0;
        }
        if (s > 0xffff)
        {
            s = 0xffff;
        }
        if (t > 0xffff)
        {
            t = 0xffff;
        }
        ndsRendererAdapterEmitTexture(
            branch_dl++, (u32)s, (u32)t, 0u,
            NDS_RENDERER_ADAPTER_G_TX_RENDERTILE,
            NDS_RENDERER_ADAPTER_G_ON);
    }

    ndsRendererAdapterEmitEndDL(branch_dl++);
    return branch_dl;
}

/* Defined beside the other graphics-heap counters in src/port/diagnostics.c.
 * Declared here rather than in a header for the same reason the decomp bodies
 * declare gSYTaskmanGraphicsHeap locally: this file has no startup header. */
extern volatile u32 gNdsTaskmanGraphicsHeapNoRoomCount;

static sb32 ndsRendererAdapterPrepareMaterialSegment(
    DObj *dobj, NDSFighterDLDrawState *state)
{
    MObj *mobj;
    Gfx *table;
    Gfx *branch_dl;
    uintptr_t heap_start;
    uintptr_t heap_end;
    uintptr_t heap_ptr;
    u32 mobj_count = 0u;
    u32 branch_commands = 0u;
    size_t heap_bytes;
    u32 i = 0u;

    if ((dobj == NULL) || (state == NULL) || (dobj->mobj == NULL))
    {
        return FALSE;
    }
    if (ndsRendererAdapterCountMaterialCommands(
            dobj, &mobj_count, &branch_commands) == FALSE)
    {
        return FALSE;
    }
    if ((mobj_count == 0u) ||
        (gSYTaskmanGraphicsHeap.ptr == NULL) ||
        (gSYTaskmanGraphicsHeap.start == NULL) ||
        (gSYTaskmanGraphicsHeap.end == NULL))
    {
        return FALSE;
    }

    heap_start = (uintptr_t)gSYTaskmanGraphicsHeap.start;
    heap_end = (uintptr_t)gSYTaskmanGraphicsHeap.end;
    heap_ptr = (uintptr_t)gSYTaskmanGraphicsHeap.ptr;
    heap_bytes = (size_t)(mobj_count + branch_commands) * sizeof(Gfx);
    if ((heap_ptr < heap_start) || (heap_ptr > heap_end))
    {
        return FALSE;
    }
    if (heap_bytes > (size_t)(heap_end - heap_ptr))
    {
        /* P2-3f9. THE ONE UNBOUNDED GRAPHICS-HEAP WRITER, MADE COUNTABLE.
         * Every other writer on this port has a source bound (see the note on
         * NDS_R2_VSBATTLE_GRAPHICS_ARENA_BYTES in battleship_scvsbattle.c);
         * this table is sized by the DObj's own material chain and so cannot
         * be bounded from a header. It has always refused rather than
         * overrun -- which is why an undersized heap shows up here as a
         * MISSING material branch and not as corruption -- but a refusal that
         * nothing counts is indistinguishable from a DObj that had no
         * materials. gNdsTaskmanGraphicsHeapOverflowCount cannot see it: the
         * pointer never passes `end`, so the sampler has nothing to report.
         * The four-CPU stress harness asserts this at 0. */
        NDS_DIAG(gNdsTaskmanGraphicsHeapNoRoomCount++);
        return FALSE;
    }

    table = (Gfx *)gSYTaskmanGraphicsHeap.ptr;
    branch_dl = table + mobj_count;
    for (mobj = dobj->mobj; mobj != NULL; mobj = mobj->next, i++)
    {
        ndsRendererAdapterEmitBranchTableCommand(&table[i], branch_dl);
        branch_dl = ndsRendererAdapterEmitMaterialCommands(branch_dl, mobj);
    }

    gSYTaskmanGraphicsHeap.ptr = branch_dl;
    state->segment_e_base = table;
    state->segment_e_end = branch_dl;
    return TRUE;
}



static void ndsStageRejectNativeRender(DObj *dobj, const Gfx *dl,
    u32 reason, NDSRendererStats *stats)
{
    NDSRelocLoadedFile *loaded = (dl != NULL) ?
        ndsRelocFindLoadedFileContaining(dl, sizeof(*dl)) : NULL;
    u32 kind = ((dobj != NULL) && (dobj->parent_gobj != NULL)) ?
        dobj->parent_gobj->id : 0xffffu;
    u32 identity = (kind << 16) |
        ((loaded != NULL) ? (loaded->asset_id & 0xffffu) : 0xffffu);

    ndsRendererRecordNativeFailure(NDS_NATIVE_FAILURE_STAGE,
        (u32)gSCManagerSceneData.scene_curr, identity,
        (gSCManagerBattleState != NULL) ? (u32)gSCManagerBattleState->gkind : 0xffffu,
        (loaded != NULL) ? ndsRelocNativeRootOffset(loaded, dl) :
            (u32)(uintptr_t)dl,
        (dobj != NULL) ? (u32)(uintptr_t)dobj->mobj : 0u, reason);
    if (stats != NULL)
    {
        stats->blocker = NDS_RENDERER_BLOCKER_UNSUPPORTED;
    }
}
