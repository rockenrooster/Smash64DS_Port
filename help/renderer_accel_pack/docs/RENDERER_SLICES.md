# Renderer slices

## Slice 1 — GBI state machine on the current Opening Room DObj

Goal: Convert the current `ndsRendererExecuteDisplayList` callback from “stats only” into a state-machine pass.

Scope:

- `G_SETPRIMCOLOR`
- `G_SETENVCOLOR`
- `G_SETBLENDCOLOR`
- `G_SETFOGCOLOR`
- `G_SETCOMBINE`
- `G_TEXTURE`
- `G_SETTIMG`
- `G_SETTILE`
- `G_LOADBLOCK`
- `G_SETTILESIZE`
- `G_VTX`
- `G_TRI1`
- `G_TRI2`
- sync commands

Stop after one selected DObj path shows better color/material handling.

## Slice 2 — Matrix/camera correctness for selected DObj

Goal: Use original DObj/XObj/CObj state to project one object more correctly.

Scope:

- CObj viewport/perspective samples already captured
- DObj translate/rotate/scale samples already captured
- one local matrix stack
- one object-to-screen transform path

Do not implement full animation or all XObj kinds yet.

## Slice 3 — Texture source discovery

Current verified `ORTX=0` means the selected material path is prim-color only. Before DS texture upload can be useful, the agent needs a texture-bearing original MObj source.

Goal:

- find one texture-bearing original MObj in Opening Room, Startup, or a narrow menu/movie path
- prove pointer validity and dimensions
- decode to a CPU buffer
- do not upload to VRAM yet unless decode is verified

## Slice 4 — Texture decoding/conversion

Goal: Convert N64 texture formats to DS-compatible 15-bit texels.

Start with CPU preview output:

- RGBA16
- RGBA32
- IA4/IA8/IA16
- I4/I8
- CI4/CI8 + RGBA16 palette

Only after decode is correct should the agent add VRAM upload.

## Slice 5 — DS texture upload/cache

Goal: Upload one decoded texture into DS VRAM and bind it for one material.

Scope:

- small static VRAM allocator
- power-of-two padded dimensions if required
- LRU/free list later, not now
- one selected texture only

## Slice 6 — SObj/sprite generalization

Goal: Move beyond the startup logo special-case preview toward reusable sprite drawing.

Scope:

- Sprite attr flags already seen in startup logo
- SP_TEXSHUF already handled for the logo preview
- build a reusable SObj sprite draw contract

## Slice 7 — Broaden bounded draw

Goal: Keep budgets but allow more than one selected DObj/SObj.

Scope:

- list traversal budget
- object count budget
- command count budget
- visible debug capture comparison

Do not unbound the full draw loop until the renderer can survive multiple frames.
