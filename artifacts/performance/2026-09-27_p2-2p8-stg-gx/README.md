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

## 2. Stage commit loop: witness off, pointer chain, hoisted count -- BANKED

Three exact changes, measured together:

- **Run witness behind a lab word.** Every visible run snapshotted seven
  counters (`ndsRendererNativeStageBindingHidden`), published its emission and
  ran `ndsRendererNativeStageAccountShortfall`, and every frame cleared the
  RoofSnap array -- a diagnostic read only by
  `scripts/diagnostics/probe-native-render-scene.ps1`. `gNdsNativeStageRunWitness`
  (default 0) gates all of it; the probe now sets it at boot.
- **Persistent stage world build** walks the chain by pointer to each cached
  ancestor world instead of copying 64 B into a local per link.
- **`ndsRendererAdapterCommitNativeStageDisplay`** reads the segment count once
  per display GObj instead of once per compared segment.

`stgb` `44F19756` against `hudonce` `C55BCBBD` (route 1): WORK-H P50/P95/P99
1,283,520/1,772,160/2,178,496 -> 1,264,192/1,750,784/2,155,008; STG P50/P95
255,104/263,552 -> 238,528/245,888; two-VBlank 373 -> 440. 54 program draws per
frame, 0 declines.

Replay: `compare-replay-digest.py --sequence --resync 4` IDENTICAL AFTER ONE
RESYNC -- from sample 44 (frame 46, pre-GO, where the digest is constant while
loading) every candidate sample equals the control's next sample, for all 1,927
later samples (one seam sample carries an unrecorded zero DGSB). The pre-GO load
wait ended one presented frame sooner; the tick state sequence is identical.
`--resync` is new for this case and is reported as such, never as plain
IDENTICAL. Evidence: `stgb-route1` (json/rows/log).
