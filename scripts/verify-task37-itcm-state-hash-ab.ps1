param(
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [int]$GdbPort = 4333,
    [int]$RunnerSlot = -1,
    [switch]$NoBuild,
    [int]$DelaySeconds = 0
)

# Task 37 exactness gate.
#
# Task 37 moves seven measured hot leaves from .main into ITCM and changes
# nothing else: the library members are byte-identical objects extracted from
# SHA-pinned archives and re-sectioned, and the port functions carry a section
# attribute with no ISA or optimization change. That is the claim. This proves
# it, rather than arguing it from the diff, by running the full match lifecycle
# on both builds and requiring every per-update game-state record to match.
#
# Modelled on verify-task9-state-hash-ab.ps1, which established the pattern for
# a placement change that must be behaviour-neutral.

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$owner = Join-Path $PSScriptRoot `
    'verify-battle-mariofox-gcrunall-loop-harness.ps1'
$baselinePath = Join-Path $root `
    'artifacts\performance\2026-07-22_task37-state-itcm-off.json'
$candidatePath = Join-Path $root `
    'artifacts\performance\2026-07-22_task37-state-itcm-on.json'

function Invoke-Task37StateRun {
    param(
        [Parameter(Mandatory=$true)][string]$Target,
        [Parameter(Mandatory=$true)][string]$Build,
        [Parameter(Mandatory=$true)][ValidateRange(0,1)][int]$ItcmLeaves,
        [Parameter(Mandatory=$true)][string]$ExportPath
    )

    # NDS_TASK37_ITCM_LEAVES is declared with ?= so the environment wins, which
    # is how the two arms of this A/B differ.
    [Environment]::SetEnvironmentVariable(
        'NDS_TASK37_ITCM_LEAVES', "$ItcmLeaves", 'Process')
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
        -Task9StateHashMode 1 `
        -Task9StateHashExportPath $ExportPath `
        -Harness 'battle_playable_match_lifecycle' `
        -Target $Target `
        -Build $Build `
        -ExpectedMode 163 `
        -ExpectedHarnessSceneCurr 22 `
        -ExpectedHarnessScenePrev 21 `
        -Label "Task 37 state hash ITCM leaves=$ItcmLeaves" `
        -HarnessSelectMessage 'Task 37 state hash run did not select Pupupu VSBattle from Maps.'
    if ($LASTEXITCODE -ne 0) {
        throw "Task 37 state hash run ITCM leaves=$ItcmLeaves failed."
    }
}

$environment = @{
    NDS_RENDERER_FAST_RUN_DEFAULT = '9'
    NDS_SCENE_MIP_CACHE_LAB = '0'
    NDS_RENDERER_BATTLE_STATIC_TEXTURE_DEFAULT = '1'
    NDS_DEBUG_HUD = '0'
}
$savedEnvironment = @{}
foreach ($name in @($environment.Keys) + @('NDS_TASK37_ITCM_LEAVES')) {
    $savedEnvironment[$name] =
        [Environment]::GetEnvironmentVariable($name, 'Process')
}
foreach ($name in $environment.Keys) {
    [Environment]::SetEnvironmentVariable(
        $name, $environment[$name], 'Process')
}
try {
    Invoke-Task37StateRun `
        -Target 'smash64ds-task37-state-itcm-off-lab' `
        -Build 'builds/build-task37-state-itcm-off-lab' `
        -ItcmLeaves 0 `
        -ExportPath $baselinePath
    Invoke-Task37StateRun `
        -Target 'smash64ds-task37-state-itcm-on-lab' `
        -Build 'builds/build-task37-state-itcm-on-lab' `
        -ItcmLeaves 1 `
        -ExportPath $candidatePath
} finally {
    foreach ($name in $savedEnvironment.Keys) {
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
if (($baseline.target -cne 'smash64ds-task37-state-itcm-off-lab') -or
    ($candidate.target -cne 'smash64ds-task37-state-itcm-on-lab') -or
    ($baseline.task9StateHashMode -ne 1) -or
    ($candidate.task9StateHashMode -ne 1)) {
    throw 'Task 37 state A/B identity is not the off/on lab pair with the hash instrument on.'
}
if (($baseline.artifacts.elf.sha256 -ceq $candidate.artifacts.elf.sha256) -or
    ($baseline.artifacts.rom.sha256 -ceq $candidate.artifacts.rom.sha256)) {
    throw 'Task 37 state A/B accidentally used an identical ELF or ROM identity.'
}
if ($baseline.rows.Count -ne $candidate.rows.Count) {
    throw "Task 37 state hash count mismatch: baseline=$($baseline.rows.Count) candidate=$($candidate.rows.Count)."
}
if ($baseline.rows.Count -lt 3600) {
    throw "Task 37 state hash covered only $($baseline.rows.Count) updates."
}
for ($index = 0; $index -lt $baseline.rows.Count; $index++) {
    $a = @($baseline.rows[$index])
    $b = @($candidate.rows[$index])
    if (($a.Count -ne 6) -or ($b.Count -ne 6) -or
        (($a -join ',') -cne ($b -join ','))) {
        throw "Task 37 state divergence at update $index`nbaseline=$($a -join ',')`ncandidate=$($b -join ',')"
    }
}

$baselineSha = (Get-FileHash -LiteralPath $baselinePath -Algorithm SHA256).Hash
$candidateSha = (Get-FileHash -LiteralPath $candidatePath -Algorithm SHA256).Hash
Write-Output (
    "Task 37 placement exactness gate PASS: $($baseline.rows.Count) per-update " +
    "full active game-state records identical across the ITCM move; " +
    "baselineJson=$baselineSha candidateJson=$candidateSha")
