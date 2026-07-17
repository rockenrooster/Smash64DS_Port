param(
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [int]$GdbPort = 4333,
    [int]$RunnerSlot = -1,
    [switch]$NoBuild,
    [int]$DelaySeconds = 0
)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$owner = Join-Path $PSScriptRoot `
    'verify-battle-mariofox-gcrunall-loop-harness.ps1'
$baselinePath = Join-Path $root `
    'artifacts\performance\2026-07-17_task9-state-r0.json'
$candidatePath = Join-Path $root `
    'artifacts\performance\2026-07-17_task9-state-phase1-itcm.json'

function Invoke-Task9StateRun {
    param(
        [Parameter(Mandatory=$true)][string]$Target,
        [Parameter(Mandatory=$true)][string]$Build,
        [Parameter(Mandatory=$true)][ValidateRange(0,1)][int]$ItcmMode,
        [Parameter(Mandatory=$true)][string]$ExportPath
    )

    & $owner `
        -MelonDS $MelonDS `
        -Gdb $Gdb `
        -GdbPort $GdbPort `
        -RunnerSlot $RunnerSlot `
        -NoBuild:$NoBuild `
        -DelaySeconds $DelaySeconds `
        -BattlePlayable `
        -LiveInputPreview `
        -CPUOpponentProof `
        -MatchLifecycleProof `
        -HardwareTriangles `
        -RendererProfileLevel 0 `
        -IFCommonHybridOamMode 0 `
        -FoxCpuMode 1 `
        -RendererBenchmarkTimeoutSeconds 600 `
        -Task9FloatItcmMode $ItcmMode `
        -Task9StateHashMode 1 `
        -Task9StateHashExportPath $ExportPath `
        -Harness 'battle_playable_match_lifecycle' `
        -Target $Target `
        -Build $Build `
        -ExpectedMode 163 `
        -ExpectedHarnessSceneCurr 22 `
        -ExpectedHarnessScenePrev 21 `
        -Label "Task 9 state hash ITCM=$ItcmMode" `
        -HarnessSelectMessage 'Task 9 state hash run did not select Pupupu VSBattle from Maps.'
    if ($LASTEXITCODE -ne 0) {
        throw "Task 9 state hash run ITCM=$ItcmMode failed."
    }
}

$environment = @{
    NDS_RENDERER_FAST_RUN_DEFAULT = '9'
    NDS_SCENE_MIP_CACHE_LAB = '0'
    NDS_RENDERER_BATTLE_STATIC_TEXTURE_DEFAULT = '1'
    NDS_DEBUG_HUD = '0'
}
$savedEnvironment = @{}
foreach ($name in $environment.Keys) {
    $savedEnvironment[$name] =
        [Environment]::GetEnvironmentVariable($name, 'Process')
    [Environment]::SetEnvironmentVariable(
        $name, $environment[$name], 'Process')
}
try {
    Invoke-Task9StateRun `
        -Target 'smash64ds-task9-state-hash-r0-lab' `
        -Build 'builds/build-task9-state-hash-r0-lab' `
        -ItcmMode 0 `
        -ExportPath $baselinePath
    Invoke-Task9StateRun `
        -Target 'smash64ds-task9-state-hash-phase1-lab' `
        -Build 'builds/build-task9-state-hash-phase1-lab' `
        -ItcmMode 1 `
        -ExportPath $candidatePath
} finally {
    foreach ($name in $environment.Keys) {
        if ($null -eq $savedEnvironment[$name]) {
            Remove-Item "Env:$name" -ErrorAction SilentlyContinue
        } else {
            [Environment]::SetEnvironmentVariable(
                $name, $savedEnvironment[$name], 'Process')
        }
    }
}

$baseline = Get-Content -LiteralPath $baselinePath -Raw | ConvertFrom-Json
$candidate = Get-Content -LiteralPath $candidatePath -Raw | ConvertFrom-Json
if ($baseline.rows.Count -ne $candidate.rows.Count) {
    throw "Task 9 state hash count mismatch: baseline=$($baseline.rows.Count) candidate=$($candidate.rows.Count)."
}
if ($baseline.rows.Count -lt 3600) {
    throw "Task 9 state hash covered only $($baseline.rows.Count) updates."
}
for ($index = 0; $index -lt $baseline.rows.Count; $index++) {
    $a = @($baseline.rows[$index])
    $b = @($candidate.rows[$index])
    if (($a.Count -ne 6) -or ($b.Count -ne 6) -or
        (($a -join ',') -cne ($b -join ','))) {
        throw "Task 9 state divergence at update $index`nbaseline=$($a -join ',')`ncandidate=$($b -join ',')"
    }
}

$baselineSha = (Get-FileHash -LiteralPath $baselinePath -Algorithm SHA256).Hash
$candidateSha = (Get-FileHash -LiteralPath $candidatePath -Algorithm SHA256).Hash
Write-Output (
    "Task 9 supreme state gate PASS: $($baseline.rows.Count) per-update " +
    "full active game-state records identical; overflow=0; " +
    "baselineJson=$baselineSha candidateJson=$candidateSha")
