# 2026-10-05 Entry-effect no-Z groups through the GX

A no-Z entry-effect group (geometry mode without G_ZBUFFER) drew on the CPU
painter path: every corner transformed by the root's composed matrix
(`ndsRendererTransformVertex20p12`), divided by w and submitted as a
projected v16 vertex (`ndsRendererHardwareClipVertex`). On the gate that is
Link's Spin Attack weapon (nine 2-triangle ramp groups, 54 corners a frame)
and the KO blast's 8-triangle root -- profiled at ~1.4K cycles a corner with
the batch and matrix loads around it.

That path submits z = depth * 4096 / w, a constant clip-space z of
depth / 4096. The GX now gets the root's composed matrix at the GX clip scale
(`ndsRendererBuildRawHardwareMatrix`, CPU clip / 256) with its z column
replaced by that constant (`ndsRendererEntryEffectLoadPainter`) and draws the
corners as ordinary GX vertices; the group consumes the same painter depths.
Groups whose corners load under other roots' matrices (Mario's pipe) stay on
the CPU. Same-ROM A/B word `gNdsEntryEffectGxPainter` (0 = the CPU painter).

Gate, `build-gate-1005p` (gate-p1 ran on runner slot 10 after slot 8 produced
an empty log):

| run | WORK P50 | WORK P95 | two-VBlank presents |
|---|---:|---:|---:|
| gate-p0 (CPU painter) | 811,136 | 1,116,416 | 1,857 / 1,961 |
| gate-p1 (GX painter, default) | 810,176 | 1,111,552 | 1,861 / 1,961 |

Replay digest identical. Over-budget frames 97 -> 92. Link's spin frames
1,437-1,447 drop 19K-28K each, the KO burst's 1,186-1,196 9K-15K.

Captures (local, `artifacts/visibility/2026-10-05_gxpainter`): the KO frame
1,188 is pixel-identical; frame 1,444 (spin trail on screen in both) differs in
262 pixels along the trail; frames 1,437 and 1,440 caught different presented
frames (the camera differs, the UI-lag caveat).
