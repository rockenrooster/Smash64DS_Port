#ifndef NDS_HOST_GRAPHICS_REFERENCE_H
#define NDS_HOST_GRAPHICS_REFERENCE_H
#if defined(ARM9) || defined(ARM7) || defined(__NDS__)
#error Reference graphics APIs are host-only
#endif
#include <nds/nds_renderer.h>

void ndsRendererScanDisplayList(const Gfx *dl, const NDSRendererConfig *config,
                                NDSRendererStats *stats);
void ndsRendererExecuteDisplayList(const Gfx *dl, const NDSRendererConfig *config,
                                   NDSRendererCommandCallback callback,
                                   void *callback_user, NDSRendererStats *stats);
void ndsRendererExecuteDisplayListWithVertexCache(
    const Gfx *dl, const NDSRendererConfig *config,
    NDSRendererCommandCallback callback, void *callback_user,
    NDSRendererStats *stats, NDSRendererVertexCache *vertex_cache);
#endif
