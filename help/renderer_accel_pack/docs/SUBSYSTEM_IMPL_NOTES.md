# Implementation notes by subsystem

## N64 display-list parser

Current code already walks `G_DL` branches and records command stats. Next improvement is to keep a mutable RDP/RSP state while walking commands.

Recommended API shape:

```c
ndsRdpStateInit(&state);
ndsRdpStateApplyCommand(&state, w0, w1);
```

Keep parsing independent from drawing so verifier scripts can inspect state without DS GPU involvement.

## RSP matrix stack behavior

Do not start with a perfect RSP microcode clone. Start with a local matrix stack used only by the selected DObj path.

Minimum:

- identity
- multiply
- translate
- scale
- Euler rotate Z/Y/X or the order observed in DObj setup
- N64 `Mtx` s15.16 load helper
- project one vertex to screen/debug preview

## RDP combiner/material behavior

Do not implement the full combiner first. Track the commands and map a few common cases:

- prim-color only
- env-color only
- texture modulated by prim/env
- alpha test / translucent flag as a debug boolean

Record unsupported combine words instead of guessing silently.

## Texture decoding/conversion

Keep CPU decode separate from DS upload.

First output format should be `u16` DS-style 15-bit texels with bit 15 as alpha-present.

Start with:

- RGBA16
- RGBA32
- IA4
- IA8
- IA16
- I4
- I8
- CI4/CI8 with RGBA16 palette

## Palette / CI texture handling

CI textures require two inputs:

- texel index stream
- TLUT/palette stream

Do not invent missing palettes. If palette pointer cannot be proven valid, stop and report.

## Sprite drawing

Startup logo proves a sprite path can work. Convert it from special-case preview to reusable:

```c
ndsSpritePreviewDraw(...)
```

Support clipping, scaling later. First keep native resolution.

## DObj/SObj/MObj traversal

Keep original object traversal. DS code should attach to display callbacks and renderer adapter seams only.

Useful rule:

- original decides what exists and when
- DS renderer decides how to draw it

## Camera transforms

Use the active CObj captured at the draw boundary. Start by respecting viewport and perspective near/far samples enough for stable placement. Perfect N64 projection can come later.

## Depth handling

First milestone: collect depth state and render with a simple painter/depth flag.

Second milestone: map depth enable/write to DS polygon attributes.

Do not block first visuals on exact RDP depth behavior.

## DS polygon/VRAM upload strategy

Use a tiny, intentionally dumb allocator first:

- static VRAM texture pool
- monotonic allocations for one scene
- reset on scene cleanup
- no eviction initially

Later:

- texture cache keys from source pointer + fmt + siz + width + height + palette pointer
- LRU or generation-based eviction
