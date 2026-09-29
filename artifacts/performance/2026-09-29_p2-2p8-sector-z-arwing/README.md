# P2-2p8: Sector Z's Arwing, and the entry-effect owner that draws it (2026-09-29)

## Why Sector Z

The worst-case search covers every stage. Sector Z's lab P95 is ~1.71M against
Dream Land's ~1.26M, and its median (1.23M) is already over the 1.12M gate:
59% of its frames are over. A whole-match per-frame-region profile of the lab
ROM on Sector Z (`builds/p2p8-prof-sz2`, default lab roster, frames 100..1900)
ranks the levers (`sz-counterfactual-p95.txt`: P95 with one symbol removed):

| symbol | P95 drop | mean/frame |
|---|---:|---:|
| `__aeabi_fadd` | 128.5K | 91.3K |
| `ndsRendererSubmitNativeEntryEffect` | 96.3K | 46.6K |
| `ndsFtrLeanKernelCompose` | 55.0K | 56.9K |
| `__mulsf3` | 54.8K | 34.3K |
| `ndsStageGxDraw` | 44.8K | 43.6K |
| `ndsMPFCSegmentCrossesKernel` | 26.0K | 20.2K |
| `ndsInterpGetFracFrameMemo` + `ndsInterpQuart` + `syInterpGetQuartSum` | 50K together | 18K together |

`sz-float-callers.txt`: in over-gate frames `syInterpGetQuartSum` alone makes
~1,600 soft-float calls (fmul 1,013, fadd 579) against ~130 in the rest, and
the floor-crossing kernel ~700 every frame.

## The entry-effect owner is the stage Arwing

Sector Z's Arwing is a ground object that draws FoxSpecial3's lists (file 161,
pulled in by GRSectorMap with or without Fox in the match), so it goes through
`ndsRendererSubmitNativeEntryEffect` -- the owner written for fighter entry
props. `sz-entry-root-draws.txt` (run `ef_sz_g1`): roots 2-9, the Arwing body
(20 groups) and its seven small lists, are drawn 929 times each in the match;
nothing else comes close. `sz-entry-effect-per-frame.txt`: the cost arrives in
runs -- 475 and 381 consecutive frames -- at ~94K ticks a frame while the
Arwing is out, and the whole owner family (submit, CPU shade, adapter, batch
open, coordinate helper) is ~253M cycles a match, ~136K ticks per Arwing frame.

`sz-pc-entry-effect.txt`: ~305K corners a match, ~189 instructions and a CPU
light evaluation (`ndsRendererHardwareLitShadeColorPrepared`, ~124 cycles) per
corner, each emitted in immediate mode (`glColor`, `glTexCoord`, `glVertex`
with three out-of-line coordinate conversions).

## Banked: the static fence once per root

`src/nds/nds_renderer_native_common.c`: before its first GX write the submit
proved every generated table index of every corner of the root -- on every
draw, although the tables are const. A root's static half is now proven once
and remembered (`sNdsEntryEffectRootStaticOk`); the checks that read live
state (materials, texture names, the traversal's modelview mask) still run on
every draw. A proven root's corners decode without re-checking, and the
coordinate conversion is inlined into the corner loop. Exact.

Lab ROM on Sector Z, same ROM, A/B word `gNdsEntryEffectStaticOnce` 0 -> 1
(`es0_sz_g1` / `es1_sz_g1`): P50 1,232,192 -> 1,226,368, **P95 1,711,488 ->
1,698,944**, P99 2,030,528 -> 2,015,104; entry draws 8,188 both, 0 fallbacks;
replay IDENTICAL.

## Banked: the entry props lit by the geometry engine

`src/nds/nds_renderer_native_common.c`. Every lit group of the owner shaded each
corner on the CPU (`ndsRendererHardwareLitShadeColorPrepared`) against a light
direction the submit re-derived with soft float for every group (~17 calls a
frame of `ndsRendererHardwarePrepareLitDirection` in `sz-float-callers.txt`).

- The direction is prepared once per root: the battle light and the root's
  modelview are its only inputs, and no group changes either. Exact.
- A lit group the engine transforms itself (not CPU-projected, no ramp palette,
  no PRIM x SHADE fold, both light colours and the direction present) is lit
  by light 0. The prepared direction (model space, normalised to 127, the
  vector the CPU dot used) is stored while the vector matrix is identity, and
  the modelview then goes to the position matrix alone
  (`ndsRendererEntryEffectLoadLitMatrices`), so the engine dots the source
  normal with that vector and the modelview's scale never enters. Diffuse and
  ambient are the two source light colours at five bits, the fighters'
  no-material mapping, and each corner sends `GFX_NORMAL` instead of a colour.
  Not exact: the engine's five-bit light arithmetic, the approximation every
  fighter already draws with (R2-03 E16).

Same ROM, A/B word `gNdsEntryEffectHwLight` (0 = old path, 1 = direction once
per root, 2 = engine light), 1,972 samples each (`hwlight-ab.txt`):

| run | P50 | P95 | P99 |
|---|---:|---:|---:|
| Sector Z lab, 0 | 1,227,456 | 1,703,488 | 2,021,376 |
| Sector Z lab, 1 | 1,225,664 | 1,694,272 | 2,007,232 |
| Sector Z lab, 2 | **1,218,944** | **1,681,664** | **1,991,296** |
| gate, 0 | 927,552 | 1,260,288 | 1,455,552 |
| gate, 2 | 926,720 | 1,260,672 | 1,456,320 |

Sector Z: arm 2 lit 19,805 groups on the engine and none on the CPU; 8,188
entry draws and 0 fallbacks in every arm; native failures 119 in every arm
(the stage's standing count); heap low-water 163,568 in every arm. The gate
(no Arwing; the entry props of DK, Samus, Link and Kirby) is flat: 296 groups
moved, 0 native failures. Replay digest IDENTICAL for every pair.

Pixels (`artifacts/visibility/2026-09-29-sz-hwlight/`, same ROM, the word
poked at battle start, exact presented-frame lock):
`gate-f60-crop-cpu-hw-diff.png` is DK's entry barrel, arm 0 / arm 2 / the
difference x20: the two are indistinguishable, and the 873 differing pixels
sit on the barrel's shaded side, at most 9 of 255 (one five-bit step).
`f900-crop-cpu-hw-diff.png` is Sector Z's Arwing entering at the left edge
(81 pixels, at most 9 of 255). The Arwing is off screen at frames 300, 460,
520, 700, 1300 and 1500, where the arms are pixel-identical.

`scripts/check-r2-shade-twin.py` lists the new DIF_AMB site
(`ndsRendererEntryEffectDiffuseAmbient`) as a known unfolded site. The
checker's four other failures (the lean path's `ndsFtrLeanEntryResetShade` /
`ndsFighterPacketApplyTintPrims`) predate this change: they fail identically
on the `8bc4684098a` source.

## Next

With the light on the engine, a lit root's corners are static: the normal,
the texture coordinate and the vertex come from const tables alone. What
remains per corner is the immediate-mode decode and emit; a root can be
recorded once as packed GX words and replayed by DMA, keyed on the few
inherited states its groups read.
