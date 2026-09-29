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

## Next

The per-corner work that remains is the CPU light and the immediate-mode
emit. The fighters already moved the same shade formula
(`ambient + diffuse * max(0, N.L) / 127`) onto the DS geometry engine's
lighting (R2-03 E16, `NDS_R2_FIGHTER_HW_LIGHT`); with that, a root's corners
are static and can be recorded once and replayed by DMA.
