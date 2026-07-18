param(
    [string]$MelonDS = '',
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [string]$Objdump = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-objdump.exe',
    [ValidateRange(0,127)][int]$RunnerSlot = 10,
    [switch]$NoBuild,
    [switch]$CompareOnly
)

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$owner = Join-Path $PSScriptRoot `
    'verify-battle-mariofox-gcrunall-loop-harness.ps1'
$controlPath = Join-Path $root `
    'artifacts\performance\2026-07-18_task16-i2f-state-control.json'
$candidatePath = Join-Path $root `
    'artifacts\performance\2026-07-18_task16-i2f-state-candidate.json'

if ([string]::IsNullOrWhiteSpace($MelonDS)) {
    $MelonDS = Join-Path $root `
        "emulators\melonds-runners\slot$RunnerSlot\melonDS.exe"
}

function Invoke-Task16I2fStateRun {
    param(
        [Parameter(Mandatory=$true)][ValidateRange(0,1)][int]$I2fMode,
        [Parameter(Mandatory=$true)][string]$Target,
        [Parameter(Mandatory=$true)][string]$Build,
        [Parameter(Mandatory=$true)][string]$ExportPath
    )

    & $owner `
            -MelonDS $MelonDS `
            -Gdb $Gdb `
            -RunnerSlot $RunnerSlot `
            -NoBuild:$NoBuild `
            -DelaySeconds 0 `
            -BattlePlayable `
            -LiveInputPreview `
            -CPUOpponentProof `
            -MatchLifecycleProof `
            -HardwareTriangles `
            -RendererProfileLevel 0 `
            -IFCommonHybridOamMode 0 `
            -FoxCpuMode 1 `
            -RendererBenchmarkTimeoutSeconds 600 `
            -Task9FloatItcmMode 1 `
            -Task9FloatPhase2Mode 1 `
            -Task16FloatI2fMode $I2fMode `
            -Task9StateHashMode 1 `
            -Task9StateHashExportPath $ExportPath `
            -Harness 'battle_playable_match_lifecycle' `
            -Target $Target `
            -Build $Build `
            -ExpectedMode 163 `
            -ExpectedHarnessSceneCurr 22 `
            -ExpectedHarnessScenePrev 21 `
            -Label "Task 16 i2f state hash mode=$I2fMode" `
            -HarnessSelectMessage `
                'Task 16 i2f state run did not select Pupupu VSBattle.'
    if ($LASTEXITCODE -ne 0) {
        throw "Task 16 i2f state run mode=$I2fMode failed."
    }
}

$environment = @{
    NDS_RENDERER_FAST_RUN_DEFAULT = '9'
    NDS_SCENE_MIP_CACHE_LAB = '0'
    NDS_RENDERER_BATTLE_STATIC_TEXTURE_DEFAULT = '1'
    NDS_DEBUG_HUD = '0'
}
$savedEnvironment = @{}
if (-not $CompareOnly) {
    foreach ($name in $environment.Keys) {
        $savedEnvironment[$name] =
            [Environment]::GetEnvironmentVariable($name, 'Process')
        [Environment]::SetEnvironmentVariable(
            $name, $environment[$name], 'Process')
    }
    try {
        Invoke-Task16I2fStateRun `
            -I2fMode 0 `
            -Target 'smash64ds-task16-i2f-state-control' `
            -Build 'builds/build-task16-i2f-state-control' `
            -ExportPath $controlPath
        Invoke-Task16I2fStateRun `
            -I2fMode 1 `
            -Target 'smash64ds-task16-i2f-state-candidate' `
            -Build 'builds/build-task16-i2f-state-candidate' `
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
}

$control = Get-Content -LiteralPath $controlPath -Raw | ConvertFrom-Json
$candidate = Get-Content -LiteralPath $candidatePath -Raw | ConvertFrom-Json
foreach ($case in @(
    @{ Name='control'; Data=$control; Mode=0; Path=$controlPath },
    @{ Name='candidate'; Data=$candidate; Mode=1; Path=$candidatePath }
)) {
    $build = if ($case.Mode -eq 0) {
        Join-Path $root 'builds\build-task16-i2f-state-control'
    } else {
        Join-Path $root 'builds\build-task16-i2f-state-candidate'
    }
    $config = Join-Path $build 'nds_build_config.h'
    $elf = Join-Path $build "$($case.Data.target).elf"
    if ((Get-Content $config -Raw) -notmatch
        ("(?m)^#define NDS_TASK16_FLOAT_I2F {0}$" -f $case.Mode)) {
        throw "$($case.Name) build config has the wrong i2f mode."
    }
    if ((Get-FileHash $elf -Algorithm SHA256).Hash -cne
        $case.Data.artifacts.elf.Sha256) {
        throw "$($case.Name) state artifact ELF identity drifted."
    }
    $symbols = @(& $Objdump -t $elf)
    $candidateSize = if ($case.Mode -eq 0) { '00000020' } else { '0000005c' }
    if (@($symbols | Where-Object {
        $_ -match ("\sF\s+\.itcm\s+{0}.*\s__aeabi_i2f$" -f $candidateSize)
    }).Count -ne 1) {
        throw "$($case.Name) ELF has the wrong __aeabi_i2f definition."
    }
    $goldenCount = @($symbols | Where-Object {
        $_ -match '\sF\s+\.itcm\s+00000020.*\s__nds_task16_libgcc_i2f_golden$'
    }).Count
    if ($goldenCount -ne $case.Mode) {
        throw "$($case.Name) ELF has the wrong i2f golden count."
    }
}

if ($control.coverage.overflow -ne 0 -or
    $candidate.coverage.overflow -ne 0 -or
    $control.coverage.updates -ne $control.rows.Count -or
    $candidate.coverage.updates -ne $candidate.rows.Count -or
    $control.rows.Count -ne $candidate.rows.Count -or
    $control.rows.Count -lt 3600) {
    throw "Task 16 i2f state coverage mismatch: control=$($control.rows.Count) candidate=$($candidate.rows.Count)."
}
for ($index = 0; $index -lt $control.rows.Count; ++$index) {
    $a = @($control.rows[$index])
    $b = @($candidate.rows[$index])
    if (($a.Count -ne 6) -or ($b.Count -ne 6) -or
        (($a -join ',') -cne ($b -join ','))) {
        throw "Task 16 i2f state divergence at update $index`ncontrol=$($a -join ',')`ncandidate=$($b -join ',')"
    }
}

$controlSha = (Get-FileHash $controlPath -Algorithm SHA256).Hash
$candidateSha = (Get-FileHash $candidatePath -Algorithm SHA256).Hash
Write-Output (("Task 16 i2f supreme state gate PASS: rows={0} " +
    "overflow=0 control_json_sha256={1} candidate_json_sha256={2}") -f `
    $control.rows.Count, $controlSha, $candidateSha)
