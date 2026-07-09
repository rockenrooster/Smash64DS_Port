# HW Renderer Visibility Findings

2026-07-08 WIP checkpoint for canonical realtime HW battle visibility.

## Measurements

- Original canonical HW ROM frame-to-frame baseline:
  - 8s to 9s: 29.063% top-screen pixels changed.
  - 14s to 15s: 80.216% changed.
  - 30s to 31s: 78.125% changed.
  - Same-time reboot at 14s: 0.000% changed.
- Determinism is stable by scene time; the fault is deterministic frame-level
  presentation/state, not random memory corruption.

## Findings

- The projected HW vertex path was issuing GX matrix loads inside
  `glBegin(GL_TRIANGLE)`. Replacing that with pre-divided CPU-oracle clip
  vertices removes the illegal FIFO pattern and keeps oracle mismatches at 0.
- `glFlush(GL_TRANS_MANUALSORT | GL_WBUFFERING)` stabilizes presentation:
  30s to 31s dropped to about 1.39% changed pixels.
- The stable frame is still not playtest-ready. It has 0% dominant Dream Land
  green pixels and is dominated by white/gray no-Z projected stage geometry.
- Probes that did not solve it:
  - Raw matrix submission: GX RAM stayed empty.
  - Clip-space Z for no-Z projected triangles: scene clipped out.
  - Skipping no-Z projected triangles: scene clipped out.
  - Forcing no-Z projected triangles translucent: stable but dropped most
    accepted geometry.
  - Removing the present-path no-oracle shortcut: still white and regressed the
    realtime pacing marker.
  - Removing the palette/rgba16 halfword lane swap: reintroduced high delta.

## Current Blocker

No-Z projected stage geometry needs a source-shaped ordering/depth treatment on
DS. The current verifier now rejects the stable-white false positive with a
dominant-green pixel gate.
