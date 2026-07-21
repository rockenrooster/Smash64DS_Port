# Task 40 fighter-animation Phase 0

Verdict: **STOP_FOR_PHASE0_REVIEW**. The task requires this pipeline report before
the cycler, counters, or fixes, and its stated build path is not the live one.

## Actual pipeline

```text
BattleShip O2R reloc_animations
  -> Makefile NitroFS staging
  -> src/nds/nds_reloc_assets.c asset IDs
  -> src/port/reloc_backend_assets.c symbol-address mapping/load
  -> dFTMarioMotionDescs / dFTFoxMotionDescs
  -> lbRelocGetForceExternHeapFile
  -> lbCommonAddFighterPartsFigatree
  -> imported battleship_sys_objanim.c interpreter
  -> DObj translate/rotate/scale
  -> reloc_backend_renderer_dl.c fighter matrix adapter / GX
```

`scripts/generate_nds_native_owners.py` owns fighter model geometry/renderer
certificates; it does not generate animation data. Likewise, the fighter adapter
is `src/port/reloc_backend_renderer_dl.c`, not the planner's
`src/nds/nds_renderer.c` path.

The authoritative REGION_US motion tables are
`decomp/BattleShip-main/decomp/src/ft/ftdata.c:139` (Mario) and `:875` (Fox),
with FTData counts `0xCC` and `0xDB` at `:377` and `:1144`.

## Static completeness

| Fighter | Motion rows | Data / null rows | Unique referenced symbols | Mapped data rows | Unique mapped / missing symbols |
|---|---:|---:|---:|---:|---:|
| Mario | 204 | 195 / 9 | 143 | 195 | 143 / 0 |
| Fox | 219 | 209 / 10 | 158 | 64 | 56 / 102 |

Makefile stages all 143 Mario animation assets (`Makefile:626-768`), but only 56
Fox assets: ranges 0-19, 90-93, 109-129, and 137-147 (`Makefile:769-824`). The
port mapping mirrors that selection: Mario's address table begins at
`src/port/reloc_backend_assets.c:1434`; Fox common 0-19 begins at `:1599` and
Fox combat 109-129 at `:1643`. Symbol identity is address-token based
(`:1122`), so an unmapped symbol cannot resolve by name.

## Confirmed loss/stale seams

| Seam | Failure mode |
|---|---|
| NitroFS staging | 102 unique Fox animation resources are absent from the selected staged set. |
| Symbol-to-asset map | The same symbols resolve to `NDS_RELOC_ASSET_INVALID`; data is dropped before interpretation. |
| `lbRelocGetExternHeapFile` (`reloc_backend_assets.c:5219-5234`) | Invalid asset IDs return the supplied heap unchanged. |
| `lbRelocGetForceExternHeapFile` (`:5363-5389`) | The unchanged heap can be accepted as the requested motion file. |
| Figatree attach (`reloc_backend_compat_shims.c:7885-7930`) | `ftMainSetStatus` can therefore attach stale bytes left by a prior animation, explaining a missing or wrong Fox pose without an interpreter defect. |
| Interpreter (`src/import/battleship_sys_objanim.c:639-739`) | Original playback remains live after pointer normalization; no separate port animation format/parser was found. |
| Renderer adapter (`src/port/reloc_backend_renderer_dl.c:536,773-1198`) | Consumes the already-updated DObj transform. Its fallback matrix and DL unsupported-opcode counters concern matrix/DL emission, not missing motion-resource decoding. |

## Review gate

The first fix candidate is build-time completion of the 102 missing Fox mappings
plus a fail-closed invalid-asset result; both can be gameplay-adjacent because
animated joints carry hurtboxes/hitboxes. Per Phase 0, no cycler, profile counter,
asset import, stale-heap behavior change, or screenshot strip was started. Review
this map and the gameplay-adjacent verification scope before Phase 1.
