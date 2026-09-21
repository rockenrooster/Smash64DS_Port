$ErrorActionPreference = 'Stop'
$log = Join-Path $PSScriptRoot '..\artifacts\verification\2026-09-19_yoshi-full-natural-moveset.txt'
$out = & (Join-Path $PSScriptRoot 'verify-battle-mariofox-gcrunall-loop-harness.ps1') `
    -RunnerSlot 6 `
    -NoBuild `
    -BattlePlayable `
    -ImportBattleShipIFCommon `
    -ImportBattleShipNormalMoveset `
    -HardwareTriangles `
    -RealtimePresentation `
    -LiveInputPreview `
    -RendererProfileLevel 0 `
    -RendererFastRunMode 9 `
    -NativeStageGeneratedSegment0Enable 1 `
    -Task36HwComposeMode 2 `
    -StaticTextureAotMode 1 `
    -IFCommonHybridOamMode 0 `
    -FoxCpuMode 0 `
    -P2ProofFighter0Kind 6 `
    -Harness 'battle_playable_realtime' `
    -Target 'smash64ds-battle-playable-proof-hwtri' `
    -Build 'build-p2-yoshi-full' `
    -ExpectedMode 163 `
    -ExpectedHarnessSceneCurr 22 `
    -ExpectedHarnessScenePrev 21 `
    -Label 'Yoshi full-content natural moveset' `
    -HarnessSelectMessage 'Yoshi full-content direct battle descriptor mismatch.' 2>&1
$code = $LASTEXITCODE
@($out; "YOSHI_NATURAL_EXIT=$code") | Set-Content -LiteralPath $log
$out | ForEach-Object { Write-Host $_ }
Write-Host "YOSHI_NATURAL_EXIT=$code"
exit $code
