param(
    [Parameter(Mandatory)][string]$Arm,
    [int]$Samples = 96,
    [int]$StartFrame = 2,
    [string]$Build = 'build-p2p8-s7',
    [ValidateRange(-1,8)][int]$Stage = -1
)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '../../../..')).Path
$art = Split-Path -Parent $PSScriptRoot
$extra = @('gNdsP2StageProgDraws', 'gNdsP2StageProgWords',
    'gNdsP2StageProgLoads', 'gNdsP2StageProgBytes', 'gNdsP2StageProgDeclines',
    'gNdsP2StageProgReason', 'gNdsP2StageProgNearRuns', 'gNdsP2StageProgDmas', 'gNdsRendererNativeFailure.count',
    'gNdsRendererNativeFailure.reason', 'gNdsRendererNativeDirectReject.count',
    'gNdsTaskmanGeneralHeapFreeMin', 'gNdsTaskmanArenaChosenSize',
    'gNdsFighterPacketFaults', 'gNdsFighterPacketDeclines',
    'gNdsR2StagePrepareBuildCount', 'gNdsR2StagePrepareReuseCount',
    'gNdsR2StagePreflightElideCount', 'gNdsMatchConfig.gkind',
    'gSCManagerSceneData.gkind', 'gNdsNativeStagePrepareRunFailStep',
    'gNdsNativeStagePrepareRunFailRun',
    'gNdsStageGCDrawAllLoopCapturedDisplayCount',
    'gNdsParticleQuadEmitCount', 'gNdsDamageSlashTextureUpdateCount',
    'gNdsEffectDLSubmitCount',
    'gNdsMiscProcDisplayKindTicks[0]', 'gNdsMiscProcDisplayKindTicks[1]',
    'gNdsMiscProcDisplayKindTicks[2]', 'gNdsMiscProcDisplayKindTicks[3]',
    'gNdsMiscProcDisplayKindTicks[4]', 'gNdsMiscProcDisplayKindTicks[5]',
    'gNdsMiscProcDisplayKindTicks[6]',
    'gNdsMiscProcDisplayKindCount[0]', 'gNdsMiscProcDisplayKindCount[1]',
    'gNdsMiscProcDisplayKindCount[2]', 'gNdsMiscProcDisplayKindCount[3]',
    'gNdsMiscProcDisplayKindCount[4]', 'gNdsMiscProcDisplayKindCount[5]',
    'gNdsMiscProcDisplayKindCount[6]')
$inputArgs = @{}
if ($Stage -ge 0) {
    $inputArgs = @{ BootBreak = 'ndsMatchConfigApply'; BootSetGlobals = "gNdsMatchConfig.gkind=$Stage" }
}
$log = Join-Path $art "$Arm-run.log"
try { & (Join-Path $root 'scripts/sample-tick-hud-buckets.ps1') `
    -NoBuild -Target smash64ds-p2-fourcpu-tickhud-hwtri -Build $Build @inputArgs `
    -RingDump -Samples $Samples -StartFrame $StartFrame `
    -ExtraGlobals ($extra -join ',') `
    -RowsCsv (Join-Path $art "$Arm-rows.csv") -JsonOut (Join-Path $art "$Arm.json") `
    -RunnerSlot 9 -GdbPort 3423 -TimeoutSeconds 3600 *> $log
} catch { $_ | Out-String | Add-Content -LiteralPath $log; throw }
$code = $LASTEXITCODE
Add-Content -LiteralPath $log -Value "RUN_EXIT=$code"
Get-Content -LiteralPath $log -Tail 18
if ($code -ne 0) { throw "Stage probe failed ($code): $log" }
