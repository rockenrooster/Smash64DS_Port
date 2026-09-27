# P2-2p8 Phase 2: stage GX program costs (2026-09-27)

Dream Land's compiled program is 54 runs, 202 triangles, 297 patches
(22 VIEW, 76 NOZ, 69 UV, 40 COMPOSED_NOZ, 30 CORNER_NOZ, 54 MATERIAL,
6 COMPOSED) and 7,009 words; STG was ~263K ticks per frame on the stress.
Per-line profile (`builds/p2p8-tail-profile-bc35`): `ndsStageGxDraw` 65K, of
which the painter Z-column pass was ~16K.

## 1. No-Z W columns staged on the stack -- BANKED

The painter pass wrote each no-Z matrix's Z column from its W column, which it
read back from the body words the first pass had just written: a main-RAM line
fill per row pair (the dcache does not allocate on write), and a second scan of
every patch to find the no-Z ones. The first pass now keeps each no-Z W column
(at most 12 per run on any stage; 16 slots, longer runs take the old path) and
its patch index; the painter pass walks only those. Same values, same depth
order, so the emitted words are identical by construction.

`nozlocal` `70D3B9DE` against `fasttime` `E6DB1E1A` (route 1, frames 2..1973):
WORK-H P50/P95/P99 1,314,880/1,822,144/2,232,704 -> 1,302,080/1,808,576/
2,225,408; STG P50/P95 262,912/271,040 -> 253,056/260,928; two-VBlank
296 -> 323. 54 program draws per frame, 0 declines, 0 near-plane runs. Replay
digest IDENTICAL. Evidence: `nozlocal-route1` (json/rows/log).
