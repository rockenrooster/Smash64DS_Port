# EFDesc native owners — Falcon Punch / Falcon Kick

Date: 2026-09-12
Package: P2-3f53 (bounded run: Captain Falcon Punch/Kick + MBallThrown resolver)

## Source-derived owner facts

- Falcon Kick: `CaptainSpecial2` (asset 350), O2R SHA-256 `6cb72c3f7c0a161d30572c773c2316e18479c4e6bce182cd0d0777721e6d1f3c`, native root `0x0A30`.
  - Source EFDesc: `dEFManagerCaptainFalconKickEffectDesc`.
  - Live source DObj/AnimJoint/MatAnimJoint remain authoritative.
  - One live MObj uses flags `0x00A1` (`ALPHA | 0x20 | TEXTURE`): preserve current image, render-tile size and texture scale.
  - MatAnim selects two CI4 48x48 frames at source offsets `0x04E0` and `0x0058`.
  - Root DL supplies the immutable combine/blend/TLUT/geometry state; effective alpha is therefore the source root state plus the live selected image, not raw texture alpha in isolation.
- Falcon Punch: `CaptainSpecial3` (asset 333), O2R SHA-256 `d79b23c7ca0f6262c7481651ea9d4986acc0e7488f53fc29893f29ad6d555e33`, native root `0x0760`.
  - Source EFDesc: `dEFManagerCaptainFalconPunchEffectDesc`.
  - Single DObj remains attached through source `0x50 + RotRpyR` joint transform; MatAnim remains live.
  - One live MObj uses the same `0x00A1` material contract.
  - MatAnim selects three CI4 32x32 source frames (`0x0490`, `0x0288`, `0x0080`).
  - The root DL supplies the immutable combine/blend/TLUT/geometry state.
- MBallThrown resolver defect: `gITManagerCommonData` is linked and loaded from `llITCommonDataFileID`; `dEFManagerMBallThrownEffectDesc` must be included in resolver coverage with that loaded-file span.
- `dEFManagerPikachuUnkEffectDesc` remains unresolved because the current port has no reachable maker path for it; it is documented beside the MBallThrown resolver note rather than admitted.

## Implementation / verification status

- Generator/native lookup/adapter/resolver: complete for this bounded Falcon package; the generated TEXID image offsets are derived from the source `MObjSub` relocation graph and assert the expected Kick/Punch tuples.
- Generated include `--check`: PASS; regeneration remained byte-identical (`src/nds/nds_entry_effects.generated.inc` SHA-256 `f5ba5ae73851debbe109ecd6a1ff364c3c0372e833eab8e95a554b9fd14d5d14`).
- Host checks: PASS — `check-p2-falcon-efdesc-native.ps1`, `check-p2-link-entry-effects.ps1`, `check-untracked-dependencies.py`, `check-architecture.ps1`, `check-melonds-policy.ps1`, and `check-docs.ps1`.
- Build lane / shell build: PASS on 2026-09-13; acquired/released `codex-efdesc2`, target `smash64ds-p2-shell-hwtri`, build `build-p2-efdesc-proof`, `NDS_P2_PROOF_FIGHTER0=7`. Config has live input `1`, fast logic `0`, hardware triangles `1`, Captain `1`, item core `1`, realtime harness.
- Natural-path Falcon Kick proof: PASS on runner slot 10. Ordinary Down-B controller playback reached source status `230`; `gNdsFalconKickNativeSubmitCount` `0 -> 1`, `gNdsEffectRendererRejectedDrawCount` `0 -> 0`, presented frame `202`. Capture: 256x192, 5,538 colors, 25,954 non-clear pixels in the checked effect ROI.
- Natural-path Falcon Punch proof: PASS on runner slot 10. Ordinary Neutral-B controller playback reached source status `228`; `gNdsFalconPunchNativeSubmitCount` `0 -> 1`, `gNdsEffectRendererRejectedDrawCount` `0 -> 0`, presented frame `217`. Capture: 256x192, 5,148 colors, 25,990 non-clear pixels in the checked effect ROI.
- ROM SHA-256: `8f466bdb57a600382155db3ea5c5426a34c06321cad1cedac516f582573b23d4`.
- ELF SHA-256: `71d8f026af9f9dfae1f9bf50517a05f42bfdb5d57998e2a5946288e21b859355`.

## Required captures

- `artifacts/visibility/2026-09-12_effect-falcon-kick.png` — PASS; natural-path Kick frame, 25,954 non-clear ROI pixels.
- `artifacts/visibility/2026-09-12_effect-falcon-punch.png` — PASS; natural-path Punch frame, 25,990 non-clear ROI pixels.

## Remaining gaps

- No Falcon Punch/Kick runtime-engagement or visual-proof gap remains in this bounded package.
- Pikachu, Kirby and Yoshi EFDesc owners remain outside this bounded run.
