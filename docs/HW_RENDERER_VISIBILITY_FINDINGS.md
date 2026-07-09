# HW Renderer Visibility Findings

2026-07-08 HW visibility and texture-color checkpoint for canonical realtime
HW battle visibility.

## Measurements

- Original canonical HW ROM frame-to-frame baseline:
  - 8s to 9s: 29.063% top-screen pixels changed.
  - 14s to 15s: 80.216% changed.
  - 30s to 31s: 78.125% changed.
  - Same-time reboot at 14s: 0.000% changed.
- Determinism is stable by scene time; the fault is deterministic frame-level
  presentation/state, not random memory corruption.
- Current canonical settled-frame gate:
  - `gxram=375/1163`
  - `RENDER_ORACLE=1080/0/0`
  - `RENDER_TEXFMT=0x100,0x100,0x100,0x0,0x0`
  - `RENDER_COMBINE=4723,2959,0,0,44330,...`
  - `artifacts/visibility/canonical-hwtri-verified.png`
  - 44723/49152 non-clear pixels, 10301/49152 dominant-green pixels,
    10239/49152 non-white/non-green pixels, 968/5616 fighter-region pixels,
    and 0/49152 changed pixels against the adjacent capture.
- The rebuilt shipped HUD-off ROM passes the same pixel assertion at
  `artifacts/visibility/shipped-hwtri-hudoff-fixed.png`.

## Findings

- The projected HW vertex path was issuing GX matrix loads inside
  `glBegin(GL_TRIANGLE)`. Replacing that with pre-divided CPU-oracle clip
  vertices removes the illegal FIFO pattern and keeps oracle mismatches at 0.
- `glFlush(GL_TRANS_MANUALSORT | GL_WBUFFERING)` stabilizes presentation:
  30s to 31s dropped to about 1.39% changed pixels.
- The normals-as-colors hypothesis was valid for lit display lists: vertex
  `cn` bytes are normals when `G_LIGHTING` is set. The HW submitter now tracks
  `G_LIGHTING`, decodes submitted `G_MOVEMEM/G_MV_LIGHT` and
  `G_MW_LIGHTCOL` state, and only uses raw vertex colors when lighting is off.
  Source references: `gbi.h` `G_LIGHTING`, `gSPLight`, and `gSPLightColor`;
  `scvsbattle.c:505-510`; `ftdisplaylights.c:10-26`; and
  `ftdisplaymain.c:205-206` for the source fallback colors.
- Texture conversion and S/T state were valid, but textures still did not reach
  pixels because untextured batches called `glDisable(GL_TEXTURE_2D)`.
  In libnds, that mutates global `GFX_CONTROL`, so later textured polygons in
  the frame were also drawn untextured. The fix follows the `sm64-nds`
  renderer pattern: keep texturing enabled and bind a `GL_NOTEXTURE` texture
  for untextured geometry.
- The per-cache diagnostic texel copy was removed after RegressionCore exposed
  memory pressure in a cliffstatus HW target. The verifier now uses conversion
  counters, in-range S/T sample counters, GX RAM, oracle, and screenshot pixels
  instead of keeping copied texture pixels resident.
- Probes that did not solve it:
  - Raw matrix submission: Dream Land reached pixels, but fighters, platforms,
    and background stayed missing.
  - Force-projected submit on a scratch branch: fighters/platforms/background
    appeared, proving their DLs reach the HW submitter and isolating the
    remaining missing-pixel class to the raw matrix/z path. The probe was
    reverted; the landed path records the projected-submit fallback count.
  - Clip-space Z for no-Z projected triangles: scene clipped out.
  - Skipping no-Z projected triangles: scene clipped out.
  - Forcing no-Z projected triangles translucent: stable but dropped most
    accepted geometry.
  - Removing the present-path no-oracle shortcut: still white and regressed the
    realtime pacing marker.
  - Removing the palette/rgba16 halfword lane swap: reintroduced high delta.

## Current Blocker

The frame is now identifiable as Dream Land with fighter pixels and the shipped
ROM is no longer blank, but fidelity and speed are still not demo-grade. The
raw DS matrix/depth path still needs repair, and immediate per-frame
`gcDrawAll` traversal plus projected fallback are below 60fps in the realtime
smoke. Renderer-cache/performance work remains the next slice after Tyler's
playtest.
