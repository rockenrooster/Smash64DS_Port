# Sector Z Arwing: its eight roots replayed by DMA (2026-09-30)

## Why

After the flight table (`../2026-09-30_p2-2p8-arwing-frac/README.md`), the
Arwing's cost on its flight frames (~45% of a Sector Z match) is drawing its
eight FoxSpecial3 roots. The per-flight-frame profile (lab `sz4`, flight frames
minus the rest) put `ndsRendererSubmitNativeEntryEffect` at 29.4K ticks,
`ndsRendererEntryEffectEmitFastCorners` 16.8K and
`ndsRendererHardwareBeginTriangleBatch` 5.8K: the state cache
(`gNdsEntryEffectStateCache`) already replays every group's resolved state,
but the group loop still writes each group's texture, polygon and corner words
to the GX FIFO one CPU store at a time, reading scattered const tables that
miss the 4 KB data cache, and stalling on the geometry engine.

## Change

`src/nds/nds_renderer_native_common.c`: the first state-replayed draw of a root
also records the words its loop writes as a packed command list (the fighter
packet's format); later draws send the list by DMA and skip the loop.

- Recorded per drawn group, after its batch begins: TEXIMAGE_PARAM and
  PLTT_BASE as the polygons take them (libnds's bound object's merged word and
  its palette's base, 0 without one: `glBindTexture`/`glAssignColorTable`
  semantics read from the linked `libnds9.a`), DIF_AMB where the loop writes
  it, POLYGON_ATTR, BEGIN, the unlit COLOR, and the corners' NORMAL /
  TEX_COORD / VERTEX16 words from the exact expressions of libnds's inline
  writers.
- The lit-matrix load (light vector and position matrix from this draw's
  modelview) stays a CPU write between two DMA segments, at its place in the
  command order; the light direction is prepared while the first segment runs.
- Replayed only while the key holds: the state cache replayed this draw, each
  group binds the same texture and ramp names, and no scene texture reset,
  entry texture prepare/release or ramp palette bake happened since the
  record. A ramp-eligible texture is only ever drawn through a ramp, so no
  group reads a palette another group assigned.
- The registers outside the FIFO that the loop's batches leave (texturing on,
  the last threshold group's alpha reference, fog, alpha test off) are set on
  the CPU; every renderer and libnds texture/polygon tracker is invalidated;
  the DMA is left running for the next FIFO writer to wait on (P2-2p3).
- Arena: `src/import/battleship_grsector_ground.c` takes 1,888 words (7.5 KB)
  from the scene heap at stage setup, only with 96 KB left after it. Other
  stages allocate nothing; Fox's entry Arwing elsewhere is one fly-by.
- Same-ROM A/B word `gNdsEntryEffectPacket` (0 = the loop every draw).

## Results (lab ROM `build-p2p8-lab-cw`, Sector Z, run-s6, frames >= 64)

ROM SHA-256 `9382798092E12F95C02769A8D37F812F66ED9F978248B1188E90831A8D339057`.
Arms in `../2026-09-26_p2-2p8-ftr-item-tail/erp*_sz.json`.

| arm | roster | packet | P50 | P95 | P99 | 3-VBlank frames |
| --- | --- | --- | ---: | ---: | ---: | ---: |
| `erp1_sz` | owner | 1 | 1,054,144 | 1,390,080 | 1,588,992 | 669 |
| `erp1b_sz` | owner | 1 | 1,053,888 | 1,390,080 | 1,594,688 | 664 |
| `erp0_sz` | owner | 0 | 1,061,824 | 1,419,584 | 1,618,880 | 751 |
| `erp0b_sz` | owner | 0 | 1,061,440 | 1,418,752 | 1,618,048 | 749 |
| `erpd1_sz` | lab default | 1 | 1,076,608 | 1,497,920 | 1,887,360 | 795 |
| `erpd0_sz` | lab default | 0 | 1,100,608 | 1,520,000 | 1,917,056 | 864 |

Owner roster (Kirby/Fox/Yoshi/Pikachu): P50 -7.6K, **P95 -29.1K**, P99
-26.6K, mean -13.6K. Lab default roster: P50 -24.0K, **P95 -22.1K**, P99
-29.7K. Both repeats agree within 0.4K at P50 and 0 at P95.

The harness's own WORK-H summary (all 1,972 frames, the gate quantity):

| arm | ROM | packet | WORK-H P50 | WORK-H P95 |
| --- | --- | --- | ---: | ---: |
| `tab1_sz` / `tab1b_sz` | table build `7F2F6B22` | - | 1,029,440 / 1,028,544 | 1,381,248 / 1,380,544 |
| `erp1_sz` / `erp1b_sz` | this build `93827980` | 1 | 1,025,024 / 1,025,984 | 1,366,784 / 1,366,464 |
| `erp0_sz` / `erp0b_sz` | this build | 0 | 1,033,664 / 1,034,432 | 1,396,992 / 1,396,032 |
| `tabd1_sz` | table build, default roster | - | 1,065,792 | 1,473,600 |
| `erpd1_sz` | this build, default roster | 1 | 1,045,824 | 1,472,768 |
| `erpd0_sz` | this build, default roster | 0 | 1,069,056 | 1,500,288 |

Same ROM, the packet is worth -29.9K (owner) / -27.5K (default) at WORK-H
P95. Against the previous build (the flight table's), the shipped
configuration moved -14.3K / -0.8K: this build's packet-off arm is +15.6K /
+26.7K slower than the table build, which differs from it only by this
change's code and its 7.5 KB arena (allocated whatever the word, so every
later scene allocation sits 7,552 B further on). That part is layout, not
the packet, and it is what a cross-build number banks.

Layout experiment (lab ROM `C48730C2`, reverted, not shipped): the arena
rounded to whole kilobytes (the D-cache set period) and skipped while the word
is 0. Owner roster: padded arena on (`lay1_sz`) 1,364,544 vs 1,366,784
(-2.2K); no arena, packet off (`lay0_sz`) 1,392,576 vs arena, packet off
1,396,992 (-4.4K) and vs the table build 1,381,248 (+11.3K). Default roster:
`layd1_sz` 1,473,600 vs 1,472,768 (+0.8K); `layd0_sz` 1,481,920 vs 1,500,288
(-18.4K) and vs the table build 1,473,600 (+8.3K). So padding buys nothing,
most of the drift is the code layout, and the rest moves with timing: heap
low-water went 64,576 -> 53,712 when the arena was *removed* (idle-time clip
prefetch decisions follow frame timing). Digests identical throughout.

Counters (owner / default roster): 5,824 of 5,848 / 7,408 of 7,416 state
replays sent by DMA; records 24 / 8; **faults 0**; native failures 0 / 119
(the default roster's standing count) in both arms; heap low-water 86,664 /
64,576 in both arms (the arena is allocated whatever the word). Per-frame
state digests (rows' last two columns): **identical** packet 0 vs 1 on all
1,972 frames.

Pixels: same ROM, packet 1 vs 0, owner roster, presented frames 450, 600, 800,
1500 and 1700 (both Arwing flights; the ship is on screen in each), halted at
`ndsBattlePlayableFrameCompleteMarker` and captured with `tools/erpcap.ps1` +
`tools/capwin.ps1`: **0 differing pixels on the DS top screen in all five**
(`captures/compare.txt`; the window title and the tick HUD's FPS line are
timing text). Frame 1700 pair retained: `captures/pk1-f1700.png`,
`captures/pk0-f1700.png`.
