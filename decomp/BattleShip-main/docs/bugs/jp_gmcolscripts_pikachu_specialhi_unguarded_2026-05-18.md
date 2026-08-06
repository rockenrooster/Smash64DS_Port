# JP build: unguarded US-only col-script symbol in port link table

**Date:** 2026-05-18
**Area:** `decomp/src/gm/gmcolscripts.c` (PORT block), JP (`REGION_JP`) build enablement
**Symptom:** Compiling `ssb64_game` with `-DSSB64_VERSION=jp` (`REGION_JP`)
failed with exactly one error:

```
gmcolscripts.c:1347: error: 'dGMColScriptsFighterPikachuSpecialHi'
  undeclared here (not in a function);
  did you mean 'dGMColScriptsFighterPikachuSpecialN'?
```

## Root cause

`dGMColScriptsFighterPikachuSpecialHi[]` is a US-only script — its
definition is wrapped in `#if defined(REGION_US)` (the JP game has no
equivalent Pikachu up-special collision script). The decomp's own script
table guards every reference to it with the same `#if defined(REGION_US)`
(e.g. the `sGMColScripts`-style table around line 1248).

The port adds a second, port-only table inside `#ifdef PORT`:

```c
static GMColScriptLink sGMColScriptLinks[] = { { script, operand_index, target }, ... };
```

iterated in `gmColScriptsRegisterPointers()` via `ARRAY_COUNT(...)` to
runtime-patch the 64-bit pointers that `gmColCommandGoto` truncates to
`u32` on LP64 (see the `gmColCommandGotoS2 → ((u32)0)` note in
`docs/build_and_tooling.md`). That port table referenced
`dGMColScriptsFighterPikachuSpecialHi` **unconditionally**, so it broke
the JP build even though the rest of the JP REGION divergence (135 files)
compiled cleanly. The table is port-specific (absent from upstream
`origin/main`), which is why upstream never hit it.

## Fix

Wrap that single entry in `#if defined(REGION_US) … #endif`, mirroring
the decomp's own guarding of the same symbol in its script table. The
array is sized via `ARRAY_COUNT` and consumed by a plain loop, so a
variable entry count is safe.

- **US:** under `REGION_US` the preprocessor yields the exact same tokens
  as before — behaviour byte-identical. Verified by compiling
  `gmcolscripts.c` standalone with the US build's recorded flags (0
  errors).
- **JP:** the US-only entry is omitted; there is no such script to
  pointer-fix, matching the decomp's own region treatment. Full
  `ssb64_game` then compiles clean under `REGION_JP` (0 errors, all
  TUs).

## Notes

This was the *only* compile fallout from first-ever compilation of the
`REGION_JP` code paths — the upstream decomp's JP divergence is otherwise
solid. The fix lives in the `#ifdef PORT` block, so it is a port-side
correction that does not touch decomp game logic or US ROM behaviour.
