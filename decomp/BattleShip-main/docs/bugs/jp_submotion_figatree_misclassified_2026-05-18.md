# JP build: CSS/opening fighters T-pose — submotion figatree misclassified

**Date:** 2026-05-18
**Area:** `tools/generate_yamls.py` (JP asset-name generation) →
`port/resource/RelocFileTable.jp.cpp` → `port/bridge/lbreloc_bridge.cpp`
classification
**Symptom (JP only):** Mario, DK, and Fox T-pose on the VS
character-select screen; Fox T-poses through the opening "master hand
pulls fighter from the box" sequence (no FigurePulled/Stance motion).
**All fighters animate correctly in actual battle.** US build is fully
correct.

## Root cause

`generate_yamls.py` names non-header-named reloc files via three
**hardcoded US-ordinal** tables (`SUBMOTION_NAMES` 357–498,
`MAIN_MOTION_RANGES` 499–2131, `SUPPLEMENTAL_NAMES`). The JP ROM has 25
fewer files than US (2107 vs 2132), so every post-gap file_id is shifted
(~−25): the submotion block the decomp references as `ll_357..498_FileID`
lives at JP file_ids **332–473**, not 357–498.

Because the tables were keyed by raw US ordinal, JP file_ids 332–356 hit
no named table and fell through to the `file_{id:04d}` fallback →
`yamls/jp/reloc_misc.yml` `file_0332`…`file_0356` →
`RelocFileTable.jp.cpp[332..356] = "reloc_misc/file_0NNN"`.

This is **not cosmetic.** `port/bridge/lbreloc_bridge.cpp`
`portRelocIsFighterFigatreeFile()` (lbreloc_bridge.cpp:274–278) classifies
a reloc file as a fighter figatree purely by **resource-path prefix** —
`"reloc_animations/FT"` or `"reloc_submotions/FT"`. The placeholder
`"reloc_misc/file_0NNN"` matches neither → returns `false`. At
lbreloc_bridge.cpp:751–756 the `is_fighter_figatree` gate then skips both
`portRelocFixupFighterFigatree()` (the u16-pair halfswap) **and**
`port_aobj_register_halfswapped_range()`. With the range unregistered,
`port_aobj_fixup.cpp` `scan()`/`walk()` (lines 119/217) hit
`if (!is_in_halfswapped_range(head)) return;` and silently no-op, so the
AObj event streams stay BE-halfswapped on LE — joint transforms are never
applied → **T-pose**.

Battle (MainMotion) is unaffected: those files are pretty-named in
`reloc_data.jp.h` (`llFT…AnimWaitFileID` etc., header priority resolves
them at JP-correct ids with a `reloc_animations/FT` path). Submotion
file_ids ≥357 also kept a `reloc_submotions/FT` prefix (US name parked at
the wrong JP id — wrong fighter label, but still prefix-classified as
figatree, correct bytes, so they worked). Only the no-FT-prefix 332–356
band broke, which is exactly Mario (332–344), Fox (345–355), and Donkey's
first entry (356) — matching the report.

## Fix

`tools/generate_yamls.py`: build `_VERID_TO_USORD` from the version
header's numeric `ll_<N>_FileID` symbols (the decomp emits one per
otherwise-unnamed file; `<N>` is the US ordinal, the value is the
per-version file_id, captured by `parse_reloc_data_header` as clean name
`_<N>_`). `resolve_name()` now translates the version file_id back to its
US ordinal before consulting `SUPPLEMENTAL_NAMES` / `SUBMOTION_NAMES` /
`get_main_motion_name()`. Header-named (priority 1) and the `file_NNNN`
fallback still key on the real version file_id.

For US, `ll_<N>_FileID == N` for all N, so the map is the identity and
US output is **byte-for-byte unchanged** (regression guard:
`git diff yamls/us port/resource/RelocFileTable.us.*` empty after
regen — verified).

Regenerate + rebuild: `python3 tools/generate_yamls.py --version jp`,
`python3 tools/generate_reloc_table.py --version jp`, rebuild `ssb64`,
re-extract `BattleShip.o2r` (resource names changed, so the o2r must be
rebuilt to match the table).

## Verification

- US `yamls/us` and `RelocFileTable.us.{cpp,h}` byte-identical after
  regeneration (empty git diff).
- JP `RelocFileTable.jp.cpp[332]` = `reloc_submotions/FTMarioSubMotionAppearR`
  (was `reloc_misc/file_0332`); `[357]` = `reloc_submotions/FTDonkeySubMotionAppearL`
  (was the misplaced `FTMarioSubMotionAppearR`); **0** `reloc_misc/file_`
  entries remain (was 32); JP submotion count 142 == US.
- Runtime (JP, `SSB64_LOG_LBRELOC_LOAD=1`): the previously-broken band
  now classifies correctly, e.g.
  `file_id=338 path=reloc_submotions/FTMarioSubMotionVictory2 fig=1`,
  `file_id=353 …/FTFoxSubMotionResults2 fig=1`,
  `file_id=365 …/FTDonkeySubMotionResults1 fig=1` — `fig` flipped 0→1, so
  the halfswap fixup + AObj range registration now run.

## Notes

`generate_yamls.py` lives in the outer port tree (not a submodule). The
companion `port/resource/RelocFileTable.jp.{cpp,h}` and `yamls/jp/*` are
generated artifacts committed for the JP build. No decomp/torch/LUS
submodule change. Earlier triage (`docs/jp_runtime_triage_2026-05-18.md`
Issue C, now corrected) wrongly concluded the naming wart was cosmetic by
assuming figatree fixups were access-time; they are gated at load by
path-prefix classification.
