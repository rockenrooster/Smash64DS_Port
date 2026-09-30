# libnds texture identity reset after direct FIFO producers (2026-09-30)

## Change

Three producers write GX commands straight to the geometry FIFO by DMA:
the stage GX program flush (`ndsStageGxFlush`, `src/nds/nds_stage_gx.exec.inc`),
the old-path fighter packet replay and the lean fighter packet DMA
(`src/nds/nds_renderer_native_common.c`). Their packets set TEXIMAGE_PARAM and
PLTT_BASE behind libnds. Each already forgot the port's own tracker
(`sNdsRendererHardwareBoundTextureName = 0`), but not libnds's
`glGlobalData.activeTexture` / `activePalette`. libnds `glBindTexture` returns
early when asked for the name it believes is active, so the next bind of that
name after a packet wrote no registers and the draw used the packet's texture
and palette. The repair clears both libnds fields beside the existing tracker
reset. No texture, palette, geometry or colour data changes.

Found by the impact-wave investigator as an unconfirmed state-inheritance
candidate (`artifacts/bugs/2026-09-30_impact-wave-green/REPORT.md`, "Root
cause" / "Minimal proposed fix").

## Owner-visible effect (r57 playtest, 2026-09-30)

- Yoshi's guard egg: invisible in r55, visible in r57 ("seems fixed"). The
  egg's native owner binds its 64x64 CI4 atlas by name
  (`ndsYoshiEggBindTexture`), which is exactly the elided path. The guard egg
  was not driven by a controlled capture before or after; the owner's two
  matches are the evidence.
- Impact wave missing green: intermittent, never reproduced by the
  investigator; the owner did not notice it in two r57 matches.

## Gate cost (same instrument, cross-build)

`build-p2p8-b4` (smash64ds-p2-fourcpu-tickhud-hwtri), run-s6, frames >= 64:

| Arm | Build | P50 | P95 | P99 | native failures | heap free min |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| hdm_a / hdm_b | HEAD `1e086261f37` | 899,072 | 1,212,224 | 1,462,016 | 0 | 340,140 |
| lnr_a / lnr_b | + this reset (+ Results budget, battle-inert) | 898,688 | 1,212,736 | 1,461,888 | 0 | 340,140 |

P50 -384, P95 +512, P99 -128: inside the single-run spread (P95 ~5.6K).
Replay digest `hdm_a` vs `lnr_a`: IDENTICAL over 1,972 samples
(`compare-replay-digest.py --sequence --resync 4`).
