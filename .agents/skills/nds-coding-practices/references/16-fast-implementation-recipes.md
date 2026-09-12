# 16 — Small implementation routes

| Need | Start here | Critical boundary |
|---|---|---|
| Frame loop | [frame_loop.c](../examples/frame_loop.c) | One input owner; ordinary-context preparation; bounded video commit; source tick rate separate. |
| Tiled UI | [tiled_background.c](../examples/tiled_background.c) | Map/tile units, bank overlap, halfword stores, palette and priority. |
| Movable sprite | [sprite_oam.c](../examples/sprite_oam.c) | Native tile order, index-zero mask, shadow-OAM update, hide before free. |
| Minimal geometry | [gx_frame.c](../examples/gx_frame.c) | Fixed-point ranges, state owner, standalone opaque clear only. |
| Direct-color cutout | [gx_cutout.c](../examples/gx_cutout.c) | `GL_RGBA`, preserved bit 15, modulation, full polygon alpha, explicit global state. |
| RAM→DMA | [dma_cache.c](../examples/dma_cache.c) | Channel reservation, legal domains, flush direction, retained source. |
| Narrow video copy | [video_copy16.h](../examples/video_copy16.h) | Typed aligned nonoverlapping halfwords and caller-proven capacity. |
| IRQ snapshot/worker | [irq_handoff.c](../examples/irq_handoff.c), [irq_worker.c](../examples/irq_worker.c) | Prior IRQ state, nonblocking handler, intentional coalescing, schedulable worker. |
| ARM9/ARM7 command | [PXI components](../examples/pxi/README.md) | Existing ARM7 services, user channel, bounded protocol, matching stop/release. |
| Fixed arithmetic | [fixed_math.h](../examples/fixed_math.h) | Checked input domain, width and exact rounding policy. |

Examples are independent starting points. Resource assignments, heap availability and runtime initialization are not interchangeable across them. Integrate with the project's existing owners rather than combining their `main()` functions. Follow the linked reference for the affected boundary and the final [review gate](15-review-checklists.md).
