# mvOpeningRoom Transition Explosion: Opaque Center + Wrong Z Order — 2026-04-26

## Symptom

The desk → arena transition explosion in the opening attract sequence
(`sMVOpeningRoomTotalTimeTics ∈ [1040, 1080)`) rendered wrong on the port:

1. The center of the explosion was an opaque white blob instead of the
   red/white spike silhouette with a transparent core that the emulator
   shows.
2. The explosion drew *behind* every other piece of scene geometry —
   visible through the desk-room window in the wallpaper rather than
   in front of the desk.

Reference visual (emulator): a red-and-white spiky burst centered on the
desk, drawn on top of the dim desk-room wallpaper.

## Root Cause

Two independent issues, both rooted in libultraship's Fast3D not
emulating the N64 color-image-redirect-to-Z-buffer idiom that the
original code relies on.

### 1. Opaque white center

`mvOpeningRoomTransitionOverlayProcDisplay` (`mvopeningroom.c:1074`)
issues:

```c
gDPSetColorImage(... gSYVideoZBuffer);                  // redirect to Z
gDPSetPrimColor(0xFF,0xFF,0xFF,0xFF);                   // white
gDPSetCombineLERP(0,0,0,PRIMITIVE, 0,0,0,1, ...);       // solid PRIM
gDPSetRenderMode(G_RM_PASS, G_RM_OPA_SURF2);            // no Z, OPA
gcDrawDObjDLHead0(gobj);                                // draw mesh
gDPSetColorImage(... primary FB);                       // restore
```

On real hardware the redirect makes the white-PRIM combiner output
write into the Z buffer (≈Z = 1.0 / far) instead of the framebuffer —
the mesh is *invisible*; only its shape matters because it later acts
as a Z mask for the wallpaper's `Z_CMP` PrimDepth draw.

libultraship's Fast3D ignores `gDPSetColorImage` entirely (all draws
go to the primary FB regardless of `color_image_address`), so the
white-PRIM mesh painted directly onto the framebuffer as a solid white
blob — exactly what we saw in the center of the explosion.

A partial mitigation already exists: `interpreter.cpp:2334` flips
`depth_test`/`depth_mask` to `Z_CMP`/`Z_UPD` from `other_mode_l` when
the redirect is active, which makes the white-PRIM Overlay tris ignore
the stale Z buffer; but the FB write itself still happens.

### 2. Explosion behind all geometry

`mvOpeningRoomMakeTransitionCamera` originally created the transition
camera with `dl_link_priority = 95`. In this engine, **lower
`dl_link_priority` means later in the draw stack** (= drawn on top).
Concretely the opening cameras run at:

| Camera | dl_link_priority | DL link captured |
|---|---|---|
| Fighter | 40 | 27, 9 |
| Main scene | 80 | 6 |
| Wallpaper | 90 | 28 |
| Transition | **95** | 30 |

At priority 95 the transition camera processed *first*, so the Outline
silhouette tris ended up at the bottom of the draw stack and every
later camera (wallpaper, main, fighter) painted over them. Visually:
silhouette behind the desk-room window, hidden behind whatever the
other cameras drew.

On N64 this ordering was masked by the redirect-to-Z trick — the
Outline-fill cleared Z to a near value, the Overlay wrote Z=far inside
its mask, and the wallpaper's `Z_CMP` PrimDepth pass then drew or
skipped per-pixel. The "in front of geometry" effect was a
side-effect of depth manipulation, not draw order. With the redirect
unemulated, the original priority becomes wrong.

## Fix

`src/mv/mvopening/mvopeningroom.c`, two PORT-gated changes:

1. **`mvOpeningRoomTransitionOverlayProcDisplay`**: early-return under
   `#ifdef PORT`. The Overlay mesh contributes nothing visible when
   the redirect is correctly emulated; on the port it would paint a
   spurious white blob, so skipping it is strictly better than
   drawing it. The Outline silhouette by itself is the spike pattern
   the user expects to see.

2. **`mvOpeningRoomMakeTransitionCamera`**: lower `dl_link_priority`
   from `95` → `30` under `#ifdef PORT`, putting the transition
   camera below every other scene camera (fighter=40 is now the next
   lowest) so the Outline silhouette renders *last* = on top of all
   other geometry.

Both changes are local to opening-room scene code. No libultraship
changes required.

## Why not implement the redirect properly?

Three earlier attempts at libultraship-side fixes are documented in
`docs/intro_explosion_alpha_cutout_handoff_2026-04-25.md` (approaches
3–5). Each got partial visuals working but none reproduced the
intended look without significant new infrastructure (depth-attachment
writes from triangle draws keyed off `color_image_address ==
z_buf_address`, plus a `ClearFramebuffer(depth_value)` parameter for
the Outline-fill 1-cycle clear). The game-side workaround above
gives the right visual at the cost of two PORT-gated branches.

If the redirect ever does get emulated properly (approach 4 in the
handoff doc), both `#ifdef PORT` branches should be removed so the
Overlay's Z mask and the original camera priority both come back.

## Fingerprint

- Mid-attract opening-room transition (~tic 1050) shows opaque white
  center where the spiky red silhouette should be.
- Effect renders *through* the desk-room window in the wallpaper, not
  in front of the desk.
- `port_log` from the Outline/Overlay display callbacks shows
  `trans=(0,0,0)` and `scale=0.052 → 0.985` smoothly across the
  transition window — animation data is intact, the bug is purely in
  rendering.

## Related

- `docs/intro_explosion_alpha_cutout_handoff_2026-04-25.md` — full
  history of attempted fixes, mechanism analysis, and outstanding
  approach #4 (color-image-redirect emulation) for if/when this comes
  back.
- `docs/bugs/primdepth_unimplemented_2026-04-25.md` — sibling fix that
  unblocked the wallpaper layering at z=0.56 in this same scene.
- `docs/bugs/color_image_to_zbuffer_draws_2026-04-20.md` — same
  redirect idiom hit via `GfxDpTextureRectangle`; that one was guarded
  with a skip-when-redirect-active check.
