# Whispy Canopy / Dream Land Center Tree — Metal-Only Bug (2026-04-25)

## Lead

Switching the libultraship Fast3D backend from Metal to OpenGL **fully
fixes** the Dream Land (Pupupu) Whispy Woods canopy rendering on
macOS. The center tree section, which on Metal renders with the
known striped/missing-frame artifact, draws correctly under OpenGL.

This narrows the bug from "PORT figatree / matanim issue" to
"Metal-backend Fast3D shader/render-state divergence from OpenGL".
The earlier `SSB64_CURR_EQ_NEXT=1` workaround (memory:
`project_whispy_curr_eq_next.md`) and the `texture_id_next`-only
matanim path that prompted it were likely chasing a Metal symptom
rather than a bytecode/data bug.

## How to reproduce / toggle

The backend is force-pinned to Metal in `port/port.cpp` under
`#ifdef __APPLE__`:

```c
sContext->GetConfig()->SetInt(
    "Window.Backend.Id",
    static_cast<int>(Ship::WindowBackend::FAST3D_SDL_METAL));
sContext->GetConfig()->SetString("Window.Backend.Name", "Metal");
```

To run on OpenGL temporarily, swap `FAST3D_SDL_METAL` →
`FAST3D_SDL_OPENGL` and `"Metal"` → `"OpenGL"`, rebuild, run. The
canopy renders correctly. Revert to ship Metal as the default.

(Note: the Metal default exists because of an unrelated
`GLD_TEXTURE_INDEX_2D ... unloadable` warning on Apple Silicon's
OpenGL driver — see commit `715848a`. That warning is benign;
swapping back to OpenGL is safe for debugging.)

## Where to look in libultraship

The two backends share `Fast3dWindow.cpp` dispatch but diverge in
the renderer implementation:

- `libultraship/src/fast/gfx_metal.mm` — Metal renderer
- `libultraship/src/fast/gfx_opengl.cpp` — OpenGL renderer
- `libultraship/src/fast/Fast3dWindow.cpp:141,147` — backend
  selection switch

Likely suspects in the Metal path:

1. **Combine / shader translation**: a CC mode that Whispy uses
   (probably one with environment color or inverse-alpha blending)
   may be mistranslated in the Metal MSL emitter. OpenGL's GLSL
   emitter handles it correctly.
2. **TMEM / texture-load state**: the matanim path swaps the canopy
   texture each frame; if Metal's texture-cache invalidation differs,
   the wrong frame's texels could persist in the bound texture. The
   `SSB64_CURR_EQ_NEXT=1` workaround masking this is consistent with
   "texture_id_curr is stale on Metal but fresh on OpenGL".
3. **Render mode / blender**: Whispy's leaves use one of the
   AA_ZB_XLU_* modes. Metal's blend-state setup might differ for
   alpha-coverage or cycle-2 selection.

## Suggested next steps

- GBI-trace one frame of Dream Land canopy draw (per
  `docs/debug_gbi_trace.md`) on both backends. Compare what each
  renderer translates the same display list into. Diff the issued
  shader IDs / pipeline state.
- If shader IDs match: the bug is in the runtime state setup
  (texture binding, blend, scissor). Add per-draw logging in
  `gfx_metal.mm`'s `gfx_metal_draw_triangles`.
- If shader IDs differ: bug is in CC translation. Look at
  `gfx_cc.cpp` consumers in both renderers.

## Related memory entries (may need refresh)

- `project_whispy_curr_eq_next.md` — describes the curr=next
  workaround. Update to note backend dependency.
- `project_color_image_redirect.md` — different bug, both backends.
- `project_sprite_decode_stride.md` — sprite decoders, both
  backends.

## Status

Investigation lead only — no fix landed. Default ship config remains
Metal on macOS for the GLD warning reason. OpenGL is a usable
debugging fallback that incidentally fixes Whispy.
