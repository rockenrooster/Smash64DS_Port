# Empty particle passes return before their setup, 2026-10-04

## Why

`efDisplayInitAll` makes four particle display GObjs whose camera masks select
particle links {0, 2}, {1}, {3} and {4}; each runs `lbParticleDrawTextures`
once a frame. The late-window lab profile (`sz-lateprof05`) put the pass
body at ~22.7K cycles a frame (~5.7K a pass) for ~16 particle visits and
~6 quads a frame: the atlas, camera (`ndsRendererAdapterBeginParticleViewPass`)
and Whispy setup ran in every pass, including passes whose links were empty.

## Change

`src/import/battleship_lbparticle.c`: a pass whose links hold no particle
returns right after the FireGrind step, before that setup. The link-0 pass
also draws the Fox glow and FireGrind pools, so only the other three passes
may return. Exact: an empty pass submitted nothing before.

## Result (official gate, `build-gate-1004p`)

| | WORK P50 | WORK P95 | P99 | > 1.12M | two-VBlank |
|---|---:|---:|---:|---:|---:|
| `diag-out/gate-dg1` | 870,592 | 1,210,752 | 1,532,416 | 192 | 1,759 / 1,961 |
| `gate-pp1` | 864,192 | 1,204,480 | 1,524,096 | 179 | 1,767 / 1,961 |

Paired by frame: median -6,656, 1,916 of 1,960 frames better (p10 -10.0K,
p90 -3.6K). Replay digest IDENTICAL. (The measured ROM also carried the
inert stage-animation probe word, default 1, since removed.)
