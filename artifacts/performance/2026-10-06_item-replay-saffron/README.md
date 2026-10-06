# Item draw replay: Saffron's gate Pokemon drawn again (2026-10-06)

Owner: "Saffron city: Regression, pokemon hazards are not visible".

Since 3911899aa71 (2026-10-04) every MObj-less stage-DL item route took the
item draw replay, which records an item's `ndsNativeItemWave1Emit` output once
and replays it. Marumine (Electrode), GLucky (Chansey) and Porygon draw
through their own traversal and never call that sink: their recordings held
zero emits and every later frame replayed nothing. On the shipping walk ROM
(walk-1006e) the GLucky and Marumine owners ran exactly twice per monster and
never again; the bodies were invisible while their effects drew.

Fix (`src/port/renderer_adapter_stage.c`): the three routes leave
`ndsItemReplayRouteOk`, and a recording in which any list drew without a sink
emit is dropped instead of kept (a baked root culled outside the view at
record time would otherwise replay as nothing once back in view).

walk-1006g: the GLucky owner runs every frame (20 calls in 20 frames, 80 in
80) and Chansey and Electrode are drawn
(`artifacts/visibility/2026-10-06_saffron-monsters/sw8-*`, local).

Lab A/B q47 -> q48 (draw-only change):

| config | digest | P50 | P95 | over 1.12M | paired median |
|---|---|---|---|---|---|
| gate | identical | 762,304 -> 763,584 | 1,071,232 -> 1,070,912 | 68 -> 67 | +1,344 |
| Castle | identical | 799,936 -> 800,384 | 1,094,976 -> 1,094,464 | 77 -> 78 | +512 |
| Sector Z | identical | 851,840 -> 852,096 | 1,184,448 -> 1,186,112 | 140 -> 137 | +1,472 |
