param(
    [Parameter(Mandatory)][string]$Arm,
    [int]$Route = 1,
    [int]$Samples = 96,
    [int]$StartFrame = 2
)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '../../../..')).Path
$art = Split-Path -Parent $PSScriptRoot
$extra = @('gNdsP2StageProg', 'gNdsP2StageProgDraws', 'gNdsP2StageProgWords',
    'gNdsP2StageProgLoads', 'gNdsP2StageProgBytes', 'gNdsP2StageProgDeclines',
    'gNdsP2StageProgReason', 'gNdsP2StageProgNearRuns', 'gNdsRendererNativeFailure.count',
    'gNdsRendererNativeFailure.reason', 'gNdsRendererNativeDirectReject.count',
    'gNdsTaskmanGeneralHeapFreeMin', 'gNdsTaskmanArenaChosenSize',
    'gNdsFighterPacketFaults', 'gNdsFighterPacketDeclines',
    'gNdsR2StagePrepareBuildCount', 'gNdsR2StagePrepareReuseCount',
    'gNdsR2StagePreflightElideCount', 'gNdsRendererTask36CaptureWordCount',
    'gNdsRendererTask36CaptureOutcome')
$log = Join-Path $art "$Arm-run.log"
& pwsh -NoProfile -File (Join-Path $root 'scripts/sample-tick-hud-buckets.ps1') `
    -NoBuild -Target smash64ds-p2-fourcpu-tickhud-hwtri -Build build-p2p8-s7 `
    -RingDump -Samples $Samples -StartFrame $StartFrame `
    -BootSetGlobals "gNdsP2StageProg=$Route" -ExtraGlobals ($extra -join ',') `
    -RowsCsv (Join-Path $art "$Arm-rows.csv") -JsonOut (Join-Path $art "$Arm.json") `
    -RunnerSlot 9 -GdbPort 3423 -TimeoutSeconds 3600 *> $log
$code = $LASTEXITCODE
Add-Content -LiteralPath $log -Value "RUN_EXIT=$code"
Get-Content -LiteralPath $log -Tail 18
if ($code -ne 0) { throw "Stage probe failed ($code): $log" }
