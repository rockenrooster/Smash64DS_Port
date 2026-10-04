# One walk to collect live MObjs (and a rejected stamp dedupe), 2026-10-04

## Change

`gcPlayAnimAll` (`src/import/battleship_sys_objanim.c`) collected the live
MObjs of the GObj's tree by walking it twice -- once to count, once into a
VLA of that size -- before the play and the colour passes (~14.4K cycles a
frame in `sz-lateprof05`). It now walks once into 24 stack slots; a tree with
more live MObjs takes a cold path that collects again into an exact-size
buffer. Same objects, same order, same passes.

## Result (official gate, `build-gate-1004p`)

| | WORK P50 | WORK P95 | P99 | > 1.12M | two-VBlank |
|---|---:|---:|---:|---:|---:|
| `particle-pass/gate-pp1` | 864,192 | 1,204,480 | 1,524,096 | 179 | 1,767 / 1,961 |
| `gate-mc1` (one walk) | 862,912 | 1,204,288 | 1,522,496 | 176 | 1,768 / 1,961 |

Paired by frame: median -768, 1,303 of 1,960 frames better. Replay digest
IDENTICAL.

## Rejected the same session

`gate-sd1`: the stage segment fast path stamping a texture entry once for
consecutive runs on the same texture and sampler. Paired against `gate-mc1`:
median +832, 681 of 1,960 frames better (consecutive runs rarely share an
entry, so the compare was pure cost). Reverted.
