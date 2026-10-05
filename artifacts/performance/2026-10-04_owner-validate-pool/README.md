# Owner validation pool for lean events, 2026-10-04

## Change

`src/port/renderer_adapter_matrix.c`, `src/port/renderer_adapter_stage.c`:
`ndsRendererAdapterValidateNativeOwnerCached` kept one proven root set per
owner slot (25 entries x 288 B). A fighter whose model parts alternate (a
hand or face swap) changes root set on every lean event, so each switch
re-ran `ndsRendererValidateNativeFighterOwner` (~15K cycles a switch, ~30K
under a materialization in the gate profile `task37-census/gate-prof01`).
Now a 12-entry pool keyed by owner slot, owner file identity and the exact
root offsets and material counts, replaced least recently used: three root
sets for each of four fighters, in 12 x 296 B instead of 25 x 288 B. A hit
needs the same full comparison the single entry made.

## Result

Gate target, 1,960 presented frames.

| arm | build | WORK P50 | WORK P95 | P99 | > 1.12M | two-VBlank |
|---|---|---:|---:|---:|---:|---:|
| control (`../2026-10-04_lean-lab-out/gate-l1`) | build-gate-1004l | 845,504 | 1,181,952 | 1,502,976 | 157 | 1,792/1,961 |
| pool (`gate-v1`) | build-gate-1004v | 845,952 | 1,178,752 | 1,475,776 | 151 | 1,798/1,961 |

Paired by frame: median +192 (flat frames unchanged), mean -815; the event
frames drop 40-86K (frames 792, 1674, 1773, 1758, 1186, ...). Replay digest
identical over 1,960 samples. No render change.
