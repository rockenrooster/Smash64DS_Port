#ifndef NDS_ITEM_REPLAY_H
#define NDS_ITEM_REPLAY_H

/* P2-2p8 (2026-10-04): item draw replay.
 *
 * The gate's late match draws Beam Swords only (one lying, one held by Link)
 * and each sword root paid ~15K ticks of per-root machinery -- the fast lane's
 * config and stats seeding, the executor's generated N64 state setup, the
 * per-vertex colour and coordinate words -- around eleven triangles. A
 * fixed-geometry item owner emits its triangles only through
 * ndsNativeItemWave1Emit, so one recorded draw holds everything the GX sees
 * except the matrices: per emit the texture it bound, the polygon format and
 * the alpha-test state its batch opened with, and the per-vertex words.
 *
 * The renderer TU records into a sink while the adapter runs an ordinary
 * draw, and replays the recorded emits under freshly prepared matrices; the
 * adapter decides, per item draw, whether every root can replay
 * (src/port/renderer_adapter_stage.c, ndsRendererAdapterSubmitItemDObjTreeReplay). */

#include <nds/ndstypes.h>

#define NDS_ITEM_REPLAY_EMITS 4u
#define NDS_ITEM_REPLAY_POOL_WORDS 128u

typedef struct NDSItemReplayEmit
{
    const u16 *indices;
    u32 tex_name;
    u32 tex_generation;
    u32 tex_format;
    u32 tex_width;
    u32 tex_height;
    u32 poly_fmt;
    u32 othermode_l;        /* the batch's alpha-test and state keys */
    u32 blend_color;
    u16 word_first;         /* into the sink's pool: 4 words a vertex */
    u16 corner_count;
    u16 triangle_count;
    u16 tex_slot;
    u8 vertex_count;
    u8 use_texture;
    u8 drawn;               /* poly alpha != 0 */
    u8 pad;
} NDSItemReplayEmit;

typedef struct NDSItemReplaySink
{
    NDSItemReplayEmit *emits;
    u32 *pool;
    u32 emit_count;
    u32 word_count;
    u32 failed;
} NDSItemReplaySink;

struct NDSRendererStats;

/* Renderer TU (nds_native_item_wave1_emit.exec.inc). `scratch` is a stats
 * block the batch opens may read and write (the adapter's persistent stats,
 * dead between traversals): its alpha-test and fog words are set per emit. */
void ndsNativeItemReplaySetSink(NDSItemReplaySink *sink);
sb32 ndsNativeItemReplayEmitsResident(const NDSItemReplayEmit *emits,
                                      u32 count);
u32 ndsNativeItemReplayEmits(const NDSItemReplayEmit *emits, u32 count,
                             const u32 *pool, const void *projection,
                             const void *modelview,
                             struct NDSRendererStats *scratch);

#endif
