# Link CI4 palette/TMEM native texture closure — 2026-09-12

Status: **IN PROGRESS**. This report is updated as the bounded fix is verified.

## Scope

- Existing compact four-CPU battle ROM: `builds/build-p2-battle-core` / `smash64ds-p2-fourcpu-tickhud-hwtri`.
- Reproduction roster: Donkey human + Samus/Link/Kirby level-3 CPUs on Dream Land.
- Defect: Link LOW-detail native root `0x2C88` rejects its second CI4 texture at presented frame 152 because the TMEM-0 load history points at a static texture palette staging address instead of the texel image loaded by the root.

## Pre-fix evidence already established

- At the reject, direct renderer state is correct:
  - `texture_image = 0x023A0E58` = Link asset 324 source `0xDEB0` mapped to compact `+0x4758`.
  - `texture_tlut_image = 0x023A0E38` = Link source TLUT `0xDE88` mapped to compact `+0x4738`.
  - `texture_tlut_count = 16`.
  - loaded texel span = 1,152 bytes for 32x72 CI4, line=2.
- `ndsRendererHardwareResolveOrBindTexture()` resolves TEXEL0 through the newest per-TMEM load record. The selected TMEM-0 record instead contains `image = 0x021E9670`.
- Current ELF places `sNdsRendererStaticTexturePaletteBlock` at `0x021E9470`; therefore `0x021E9670` is exactly palette-block `+0x200`.
- `ndsRendererHardwarePrepareBattleStaticTextures()` does not directly write `texture_loads[]`; normal history writes come through `ndsRendererCaptureTextureLoad()`.
- `ndsRendererNativePreflightProductionOwner()` is read-only with respect to live renderer stats.
- BattleShip source for Link root `0x2C88` explicitly uses the 16-entry TLUT at `0xDE88`, then CI4 texels at `0xDEB0`.
- Frame-512 `NATIVEFAIL=6,1,22,262143,166,0,0,2` is a separate Samus slot-1 `REJECTED_PROGRAM` failure.

## Writer witness

Pending narrowed GDB trace on verifier runner slots 5/6 only. The trace will identify the exact call/write that stores `0x021E9670` in the TMEM-0 history entry.

## Fix

Pending writer witness. The fix will preserve per-TMEM provenance and will not fall back to the mutable global image when a TMEM record exists but is invalid for the owner.

## Host regressions

Pending:

- State helper sequence: SETTIMG(palette) -> LOADTLUT(16) -> SETTIMG(texel) -> LOADBLOCK(576) must make `ndsRendererHardwareFindTextureLoadForTmem(TMEM0)` return the texel image.
- Compact pack closure pins: Link source TLUT `0xDE88 -> 0x4738` for 32 bytes; source CI4 `0xDEB0 -> 0x4758` for 8,696 bytes, with the first 1,152 bytes resident for this load.

## Integrated verification

Pending serialized rebuild and ordinary sparse probes at frames 192 and 512, plus `-FirstTextureReject -Frame 192` where no reject is the expected pass outcome.

## Candidate identity

- ROM SHA-256: pending rebuilt candidate.
- ELF SHA-256: pending rebuilt candidate.

## Remaining gaps

- Samus status 166 rejection remains separate and is not part of this fix.
