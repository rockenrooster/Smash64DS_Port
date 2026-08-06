# Whispy Canopy Stripe / Matanim Feedback Artifact (Metal-only)

**Resolved 2026-04-25.** Port had this since VS mode first booted; OpenGL
backend rendered the canopy correctly, Metal backend showed the
striped / "wrong frame's texels" artifact on the Dream Land (Pupupu)
canopy.  Fix lives in libultraship's Metal backend.

## Symptom

In Pupupu, Whispy Woods' middle-tree canopy section renders with
visible horizontal striping / smearing.  The earlier `SSB64_CURR_EQ_NEXT=1`
runtime override at `src/sys/objdisplay.c` masked it by forcing
`mobj->texture_id_curr` to equal `mobj->texture_id_next` so the cycle-2
combine mixed two copies of the same texture instead of two genuinely
different figatree frames.

OpenGL backend (toggle `port/port.cpp` to `FAST3D_SDL_OPENGL`) renders
correctly without the workaround.

## Why OpenGL was OK and Metal was not

The Whispy canopy CC declares TEXEL1, but in our display-list emission
the GBI sequence does not always produce a `LoadBlock` that fills TMEM
tile 1 before the cycle-2 draw.  In libultraship's interpreter that
means `mRdp->textures_changed[1]` stays `false`, so `ImportTexture(1, ...)`
never runs, so `mCurrentTextureIds[1]` keeps whatever value it was
initialized to.

`mCurrentTextureIds[]` is zero-initialized.  In `gfx_metal.cpp::Init()`
the **first** `NewTexture()` call belongs to the screen framebuffer's
`CreateFramebuffer()`, so `mTextures[0]` permanently holds the live
`CAMetalLayer` drawable.  When `DrawTriangles` then bound
`mTextures[mCurrentTextureIds[1]].texture` for the cycle-2 draw, slot 1
got the **screen color buffer** — the very texture the encoder was
rendering into.  The fragment shader sampled the in-progress screen and
mixed it with the canopy via `mix(curr, garbage, frac)`, producing
the stripe.

OpenGL handled the same situation differently: Apple's GLD driver
detects the declared-but-unbound sampler and substitutes a zero
texture, logging the
`"GLD_TEXTURE_INDEX_2D is unloadable and bound to sampler type (Float)
- using zero texture because texture unloadable"` warning that the
port-side commit `715848a` documents.  Metal-cpp does no such
substitution and just samples whatever is at the slot.

The pattern is rare in other LUS games (SoH, SM64), which is why the
Metal backend appeared to work fine for them — their CC modes that
declare TEXEL1 always have a matching `LoadBlock` for tile 1.  SSB64's
matanim path leans on the original GL behavior of "unbound TEXEL1 = 0".

## Fix

`libultraship/src/fast/backends/gfx_metal.cpp` (commit on the `ssb64`
branch of the libultraship fork):

1. After `Init()` finishes setting up the screen framebuffer, allocate a
   1x1 transparent-black RGBA texture and store its id in
   `mFallbackTextureId`.  Point every entry of `mCurrentTextureIds[]`
   at the fallback so the default slot binding samples zeros, not the
   screen.
2. In `DrawTriangles`, when iterating shader-used texture slots, if the
   resolved `tid == 0` *or* the slot's `MTL::Texture` / `MTL::SamplerState`
   is null, redirect to the fallback before calling `setFragmentTexture`
   / `setFragmentSamplerState`.
3. Hoisted the sampler-changed check out of the texture-changed branch
   while in there — sampler updates shouldn't be skipped just because
   the texture pointer happens to match.

The fallback is created once at Init.  Cost is one persistent 1x1 RGBA
texture and one persistent sampler — negligible.

## Diagnostic that pinned this down

Added a temporary `SSB64_METAL_TEX_DEBUG=1` env-gated logger in
`DrawTriangles` that printed the slot-1 texture id, MTL pointer, and
sampler whenever the shader's `usedTextures[1]` was true but the
binding looked stale (`tid == 0`, null tex, null sampler).  Running the
auto-cycle demo into Pupupu produced 29 hits within a single Whispy
draw burst, all reporting `tid=0` (= screen FB) at slot 1.  That made
the feedback loop concrete and confirmed the OpenGL/GLD-zero-texture
hypothesis from `port/port.cpp`'s comment.

The diagnostic was removed once the fallback fix was verified.

## Related

- `docs/whispy_canopy_metal_only_2026-04-25.md` — earlier handoff that
  narrowed the bug to the Metal backend; superseded by this entry.
- `port/port.cpp:49-65` — comment block that called out the
  GLD-zero-texture substitution and forced Metal as the macOS default
  to silence the GLD warning.  Still valid; the warning was a hint at
  the real divergence.
- `src/sys/objdisplay.c` — `MOBJ_FLAG_FRAC` / `MOBJ_FLAG_SPLIT` /
  `MOBJ_FLAG_ALPHA` matanim cycle-2 emission.  The
  `SSB64_CURR_EQ_NEXT=1` workaround there can stay as an emergency
  toggle but is no longer needed for default rendering.
