# Parallel task queue

These are safe lanes that can be prepared without touching active Opening Room verification. Only one should be integrated at a time.

## A. Renderer-state helper integration

Files to consider:

- `include/nds/renderer_work/nds_rdp_state_work.h`
- `src/nds/renderer_work/nds_rdp_state_work.c`

Integrate into the existing `ndsRendererExecuteDisplayList` callback path to track material state for the selected Opening Room DObj.

Stop when OR* verifier markers prove prim/env/combine/texture/tile command state.

## B. Matrix stack helper integration

Files to consider:

- `include/nds/renderer_work/nds_matrix_stack_work.h`
- `src/nds/renderer_work/nds_matrix_stack_work.c`

Use only for the selected Opening Room DObj/CObj first. Do not try to replicate every XObj kind in one task.

Stop when the preview placement changes from debug-space to camera/object-space, or when a precise transform blocker is identified.

## C. Texture decode CPU preview

Files to consider:

- `include/nds/renderer_work/nds_texture_decode_work.h`
- `src/nds/renderer_work/nds_texture_decode_work.c`

Use after a verified texture-bearing MObj is found. Do not use it with invented texture pointers.

Stop when one original texture decodes to a CPU preview buffer.

## D. DS VRAM upload prototype

Do not start until C is complete.

Build a tiny monotonic texture allocator and cache key:

```c
source pointer + palette pointer + fmt + siz + width + height
```

Reset on scene cleanup. No eviction at first.

## E. Sprite generalization

Use the startup logo preview as the seed. Extract the special-case logic into a reusable sprite preview/draw helper.

Stop when the same startup logo still renders through the generic helper with identical verifier counts.

## F. Stage lane

Keep separate from renderer work. Stage geometry will not look useful until renderer DObj/texture support improves.
