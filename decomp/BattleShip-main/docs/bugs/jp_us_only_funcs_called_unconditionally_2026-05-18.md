# JP build: US-only functions referenced unconditionally → link failure

**Date:** 2026-05-18
**Area:** `decomp/src/ft/ftparam.c`, `decomp/src/lb/lbcommon.c`, JP
(`REGION_JP`) port link
**Symptom:** After all TUs compiled clean under `REGION_JP`, the final
`ssb64` link failed:

```
ftcommontarucann.c:113: undefined reference to `ftParamGetGroundHazardKnockback'
lbcommon.c:(.data+0x10f0): undefined reference to `func_ovl0_800C9F70'
```

(ld lists all undefined refs at once — these two were the complete set.)

## Root cause

Both functions are defined only inside `#if defined(REGION_US)` — with
**no JP `#else`** — in *both* `port-patches` and upstream `origin/main`:

- `ftParamGetGroundHazardKnockback` (ftparam.c) — called unconditionally
  from `ftCommonTaruCannShootFighter` (Kongo Jungle Barrel Cannon).
- `func_ovl0_800C9F70` (lbcommon.c) — referenced unconditionally by the
  function-pointer matrix table further down the same file (the
  `#if REGION_US` guard there wraps only the *preceding* `func_ovl0_800C9F30`
  pair, not these entries — identical in upstream main).

The decomp's own JP ROM build resolves these via
`symbols/jp_wip_linker.txt` / `symbols/not_found.txt` linker stubs (JP is
`jp_wip` — work-in-progress). The port has no MIPS-linker stub
mechanism, so the missing definitions fail the native link.

These are **byte-matching guards on region-agnostic behaviour**:
`ftParamGetGroundHazardKnockback` is a pure knockback-formula function;
`func_ovl0_800C9F70` is a render/matrix helper. The Barrel Cannon and the
matrix path both exist in JP gameplay — the `#if REGION_US` exists because
the JP ROM lays these out differently for the decomp's *byte-matching*
build, not because the behaviour is US-exclusive.

## Fix

Per CLAUDE.md DECOMP PRESERVATION ("preserve behavior, not byte-matching;
accuracy to game behavior > accuracy to ROM bytes; wrap a port fix in
`#ifdef PORT`"), relax both definition guards to:

```c
#if defined(REGION_US) || defined(PORT)
```

`PORT` is defined for every port build and never for the decomp's own
US/JP ROM (matching) builds, so:

- decomp US ROM build: `REGION_US` true → unchanged.
- decomp JP ROM build: `PORT` undefined, `REGION_US` false → still
  excluded (its `jp_wip` linker stubs still apply — matching unaffected).
- US port: `REGION_US` already true → byte-behaviour identical (verified
  by standalone US compile of both TUs).
- JP port: `PORT` true → the region-agnostic functions are compiled, the
  link completes, and the JP behaviour matches the US formula/helper
  (the closest correct behaviour; the JP ROM's exact variants are not
  decompiled upstream — a documented approximation, not a regression).

## Result

Full JP `ssb64` binary links (157 MB x86-64 ELF). Combined with the
gmcolscripts and Torch RelocFactory fixes, these were the only blockers to
a complete JP build. US unaffected (both TUs recompiled clean with the US
flags).

## Notes / follow-up

If pixel/frame-exact JP parity is later required for the Barrel Cannon
knockback or that matrix path, the JP ROM's actual variants would need
decompiling (upstream `jp_wip`). For "boots and is playable", the US
formula is behaviourally faithful. Related: `jp_gmcolscripts_*`,
`jp_torch_relocfactory_*` (same 2026-05-18 JP bring-up).
