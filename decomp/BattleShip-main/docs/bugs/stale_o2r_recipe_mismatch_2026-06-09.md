# Stale BattleShip.o2r after binary updates — recipe-stamp auto re-extraction

**Date:** 2026-06-09 (issue #217's attached log; #221/#225 attribution corrected 2026-06-10 — see
"Attribution correction" below)
**Status:** FIXED — superproject `fe9d421d` (CMake recipe hash + first_run sidecar)
**Platforms:** Windows / macOS / Linux (Android excluded — its Java extraction flow
has its own sentinel and deletes the staged ROM, so a native re-extract can't run)

## Symptom

After updating the binary (e.g. 0.9.x → 1.x), pressing ESC to open the menu
crashed (SteamOS #217, M4 Mac #221) or exited the app (#225). The #217 log
shows the signature: tens of thousands of
`RelocPointerTable: invalid/stale token` errors from the sprite-decode loop
(`lbCommonDecodeSpriteBitmapsSiz4b`), with "tokens" that are clearly raw
data — `0x3F800000` (1.0f), `0x41A00000` (20.0f), `0xFFFFFFFF`, and the low
halves of host heap pointers. Both reporters fixed it by deleting their
install and re-extracting assets.

## Root cause

`ExtractAssetsIfNeeded` only checked `fs::exists(o2r)`. A `BattleShip.o2r`
extracted by an older torch/yaml pipeline is format-incompatible with a
newer binary (the torch SHA ⇒ wipe-and-re-extract coupling has been a known
dev-side rule for weeks — see `project_torch_reloc_walker_dead` memory),
but end users had no signal: the game loaded the stale archive and walked
mismatched struct layouts as garbage.

## Fix

- CMake computes `SSB64_ASSET_RECIPE_HASH` at configure time: SHA1 over the
  torch submodule HEAD + `config.yml` + every `yamls/<region>/*.yml`.
- After a successful extraction, `first_run.cpp` writes the hash to a
  `<o2r>.recipe` sidecar.
- At boot, an existing archive whose sidecar is missing or mismatched
  triggers an automatic re-extraction. Existing installs (no sidecar)
  re-extract once on first boot of an updated build.
- All failure paths are non-blocking: the old archive is never touched
  until a replacement is fully built, so "no ROM available" or a failed
  re-extract logs a breadcrumb and keeps running on the stale archive
  rather than locking the user out.

## Attribution correction (2026-06-10)

Timeline check after the fact: extraction inputs last changed **2026-04-27**
(codegen regen of all reloc yamls + reloc_data.h; torch's 05-18 change is
JP-only), and they are **byte-identical from v0.9.4-beta through v1.2**. So
recipe skew cannot explain the "0.9.4 worked → 1.x crashes on ESC" reports.
The #217 log proving skew is a *rotated* log dated 05-01 (April-era build +
pre-04-27 archive) — real skew, wrong era. The more likely mechanism for the
v1.x ESC crashes is **stale config**: v1.0 moved/renamed menu sections, the
selected section is persisted by name in BattleShip.cfg.json, and
`GetVectorIndexOf` returns `size()` on a miss into unguarded `.at()` calls
(hardened in the v1.3 Menu.cpp fixes). Both reporters deleted config and
assets together, so re-extraction got the credit. Lesson recorded: check the
*date* of an attached log before letting it anchor a root cause — rotated
`.N` logs can predate the report by weeks.

## Audit hook

Dev-tree note: the recipe hash is configure-time; editing a yaml without
re-running CMake leaves a stale baked hash. The torch submodule SHA is the
dominant invalidator and submodule bumps always reconfigure in practice.
A one-time surprise re-extract after pulling is expected behavior, not a bug.
