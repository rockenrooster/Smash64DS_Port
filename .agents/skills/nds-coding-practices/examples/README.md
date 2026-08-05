# Reusable Nintendo DS Code Patterns

These examples are deliberately small and ownership-focused. They use current
libnds-style APIs where practical, but the consuming project's installed headers
are authoritative.

| File | Demonstrates |
|---|---|
| `frame_loop.c` | one input snapshot, update/render/commit separation |
| `dma_cache.c` | ARM9 cache publication, async DMA lifetime, aligned inbound buffers |
| `sprite_oam.c` | main OBJ VRAM, shadow OAM, one VBlank commit |
| `tiled_background.c` | tiled BG allocation, non-overlapping bases, dirty state update |
| `gx_frame.c` | explicit GX baseline, matrix discipline, one finalization |
| `fixed_math.h` | named Q format, wide intermediates, negative rounding |
| `irq_handoff.c` | same-CPU IRQ/main coherent snapshot with a sequence lock |
| `shared_mailbox.h` | cache-line-owned ARM9/ARM7 mailbox layout and generations |

## Use rules

- Copy a pattern only after checking the project's headers and resource owners.
- Replace demo loops and hard-coded bank choices with the project's platform
  layer.
- Do not combine examples blindly: they may reserve the same VRAM bank, DMA
  channel, timer, or display owner.
- Examples do not claim successful compilation in every libnds generation.
- Direct hardware access is intentionally minimized because library shadow state
  and project wrappers must remain coherent.
