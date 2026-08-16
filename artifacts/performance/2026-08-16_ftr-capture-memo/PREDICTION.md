# Prediction, written before the counter ROM was built or run

**Date:** 2026-08-16. **Question:** how often does the fighter draw contract that
`ndsFighterDisplayContractCapture` re-derives every frame actually CHANGE?
`FTR_LANE.md` §5 priced the pass at 34,307 tk/fr and called it a ceiling because
this rate had never been measured.

The rule this file exists for: *entry-PC counts refuted a memo's estimate before
a build was spent here* — predict first, so being wrong is cheap.

## What the source says the contract depends on

`ndsBaseFTDisplayMainProcDisplay` **is** decomp's `ftDisplayMainProcDisplay`
(`src/import/battleship_ftdisplaymain.c:141` renames it), so the capture pass
runs `decomp/BattleShip-main/decomp/src/ft/ftdisplaymain.c:1069-1240` →
`ftDisplayMainDrawAll:929` → `ftDisplayMainDrawDefault:753`.

The emitted event list is decided by, per DObj: `dobj->flags` (`HIDDEN`,
`NOTEXTURE`), `parts->flags & 0xF`, `dobj->dl`, `dobj->dls[0..1]`, `dobj->dv`,
and the tree shape. The preamble is decided by fighter-level state:
`fp->colanim.is_use_color1/2`, `fp->is_use_fogcolor`, `fp->shade`,
`sFTDisplayMainSkyFogAlpha` (from `mpCollisionSetLightColorGetAlpha`), and
`parts->flags`'s `NOFOG`/`TOGGLEFOG` bits through
`ftDisplayMainDecideFogDraw:687`.

None of those is a per-frame continuous quantity. `dobj->dl` moves when a model
part id changes (hand/fist, face/blink); the preamble moves on colour-animation
and fog state. So the contract should be *stable with events*, not
frame-varying.

## The numbers I predict

Denominator is `gNdsFtrContractCaptures` = captures that had a previous capture
for the same slot to compare against (2 per frame, minus one bootstrap per slot).

| counter | predicted | band I would defend |
|---|---:|---|
| `Same / Captures` | **70%** | 55–85% |
| `CountSame / Captures` | 97% | ≥ 95% |
| `DObjSame / Captures` | 95% | ≥ 90% |
| `DLSame / Captures` | 72% | 60–88% |
| `PreSame / Captures` | 90% | ≥ 85% |
| `KeySameContractDiff / Captures` | 1–5% | **non-zero** — the tree key carries no `fp` state |
| `MaxRun` | ≥ 60 | ≥ 30 |
| `EventTotal / Captures` | **16.0** | this is an instrument check that can fail |

`EventTotal / Captures = 16.0` is not a guess: Boundary reads
`ftrContract=6784/6784` over 212 frames = 32.0 events/frame over two fighters,
and the profile independently measured `ndsRendererLoadHardwareSplitMatrices`
and `ndsRendererNativeApplyProductionPreamble` at 32.00 calls/frame. If the
census disagrees with 16.0 the census is wrong, not the contract.

## The decision rule I am committing to before seeing the answer

A key-guarded memo pays `hit_rate x 34,307 - key_cost`. The key is the DObj
tree walk `ndsFighterDisplayContractCountFlags` already performs plus its hash;
call it 6,000 tk/fr for both fighters (its own row is 4,117 today).

- **≥ 60% same** → the memo clears the ≥14,080 cross-build floor
  (0.60 x 34,307 − 6,000 = 14,584) and is worth specifying.
- **40–60%** → 7,723–14,584, under or at the floor; a rider at best.
- **< 40%** → dead at the counter, and that is a complete result.
- **`KeySameContractDiff` > 0** → the DObj-tree key alone is UNSOUND and any
  memo needs the `fp` preamble inputs in its key as well. This column decides
  the shape, not just the size.
