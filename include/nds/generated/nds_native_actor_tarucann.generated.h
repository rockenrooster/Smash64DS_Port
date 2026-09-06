/* Congo Jungle barrel cannon native actor rows (generated).
 *
 * New rows only: this header declares the tarucann packet counts,
 * the joint rows the hierarchy slot validates the live DObj tree
 * against, and the native runtime entry points and draw counters.
 * Do not hand-edit: regenerate with
 *   python scripts/fighters/generate_nds_native_owners.py
 */
#ifndef NDS_NATIVE_ACTOR_TARUCANN_GENERATED_H
#define NDS_NATIVE_ACTOR_TARUCANN_GENERATED_H

#include <nds/nds_renderer.h>

#define NDS_NATIVE_ACTOR_TARUCANN_JOINT_COUNT 2u
#define NDS_NATIVE_ACTOR_TARUCANN_BINDING_COUNT 2u
#define NDS_NATIVE_ACTOR_TARUCANN_RUN_COUNT 1u
#define NDS_NATIVE_ACTOR_TARUCANN_TRIANGLE_COUNT 2u
#define NDS_NATIVE_ACTOR_TARUCANN_VERT_COUNT 6u
#define NDS_NATIVE_ACTOR_TARUCANN_TEXTURE_EPOCH_COUNT 1u
#define NDS_NATIVE_ACTOR_TARUCANN_SLAB_BYTES 136u
#define NDS_NATIVE_ACTOR_TARUCANN_JOINT0_PARENT 31u
#define NDS_NATIVE_ACTOR_TARUCANN_JOINT1_PARENT 0u
#define NDS_NATIVE_ACTOR_TARUCANN_JOINT0_KIND0 40u
#define NDS_NATIVE_ACTOR_TARUCANN_JOINT0_KIND1 26u
#define NDS_NATIVE_ACTOR_TARUCANN_JOINT1_KIND0 28u
#define NDS_NATIVE_ACTOR_TARUCANN_JOINT1_KIND1 0u

/* Hierarchy slot entry (renderer_adapter_matrix.c) and the route's
 * commit wrapper (reloc_backend_movement.c). The imported Jungle TU
 * owns the GObj pointer accessor; both live behind NDS_P2_STAGE_JUNGLE.
 */
sb32 ndsRendererAdapterSubmitNativeTaruCann(void *root, void *cobj,
    u32 initial_geometry_mode, NDSRendererStats *stats);
sb32 ndsRendererSubmitNativeTaruCann(const void *asset_base,
    u32 asset_bytes, const NDSRendererNativeFighterHierarchy *hierarchy,
    u32 initial_geometry_mode, NDSRendererStats *stats);
void *ndsGRJungleTaruCannGObj(void);
extern volatile u32 gNdsStageGCDrawAllLoopActorDisplayCallbackCount;
extern volatile u32 gNdsStageGCDrawAllLoopActorTriangleCount;

#endif /* NDS_NATIVE_ACTOR_TARUCANN_GENERATED_H */
