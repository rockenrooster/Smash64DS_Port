# Display capture: four exact cuts, 2026-10-04

## Why

A MISC-split gate ROM (`NDS_P2_MISC_SPLIT=1`, frames 1400-1900) put the camera
loop's per-GObj capture overhead at ~37K ticks a frame (~42 display GObjs) and
the late-window lab profile (`artifacts/task37-census/sz-lateprof04`) named
its pieces: `gcCaptureCameraGObj` 17K cycles a frame over 11.5 calls,
`ndsRendererAdapterNdlDispatchEffect` 13.7K (a hot-stack switch for every
effect/ground/weapon/item GObj, most of them bound as no NDL owner), the
ClassifyGObj-rejected branch of `ndsStageGCDrawAllLoopRecordDObjDraw` calling
all three of the weapon/item/effect submits (and timing each) for every
GObj, and the stage bindings' world copied out of the persistent cache before
the compose read it.

## Change

- The rejected branch calls only the submit of the GObj's kind (each submit's
  Is*Display tests `gobj->id` first) and times only that span; the actor span
  is timed only when a stage-actor submit is compiled in.
- `gcCaptureCameraGObj` (Thumb, no CLZ) skips whole empty bytes of the link
  mask; same links, same order.
- The NDL dispatcher declines a GObj whose record is already bound as no owner
  before the hot-stack switch (the body would return FALSE there too).
- `ndsRendererAdapterPersistentStageWorldPtr` returns the persistent world by
  pointer; the stage binding compose reads it in place (the copying wrapper
  remains for the PIM caller).

## Result (official gate, `build-gate-1004p`)

| | WORK P50 | WORK P95 | P99 | > 1.12M | two-VBlank |
|---|---:|---:|---:|---:|---:|
| `fps-console/gate-fps0` | 900,800 | 1,237,888 | 1,565,696 | 238 | 1,717 / 1,961 |
| capture cuts (`gate-cc1`) | 897,408 | 1,236,352 | 1,559,232 | 234 | 1,722 / 1,961 |

Paired by frame: median -3,136, 1,766 of 1,958 frames better (MCAM -1.7K,
STG -1.0K, MISC -2.0K). Replay digest IDENTICAL.
