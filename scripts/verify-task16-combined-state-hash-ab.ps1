param(
    [string]$MelonDS = '',
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [string]$Objdump = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-objdump.exe',
    [ValidateRange(0,127)][int]$RunnerSlot = 31
)

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$owner = Join-Path $PSScriptRoot `
    'verify-battle-mariofox-gcrunall-loop-harness.ps1'
$itcmChecker = Join-Path $PSScriptRoot 'check-task9-float-itcm.ps1'
$controlPath = Join-Path $root `
    'artifacts\performance\2026-07-18_task16-combined-state-control.json'
$candidatePath = Join-Path $root `
    'artifacts\performance\2026-07-18_task16-combined-state-candidate.json'

if ([string]::IsNullOrWhiteSpace($MelonDS)) {
    $MelonDS = Join-Path $root `
        "emulators\melonds-runners\slot$RunnerSlot\melonDS.exe"
}

function Invoke-Task16CombinedStateRun {
    param(
        [Parameter(Mandatory=$true)][ValidateRange(0,1)][int]$Mode,
        [Parameter(Mandatory=$true)][string]$Target,
        [Parameter(Mandatory=$true)][string]$Build,
        [Parameter(Mandatory=$true)][string]$ExportPath
    )

    & $owner `
        -MelonDS $MelonDS `
        -Gdb $Gdb `
        -RunnerSlot $RunnerSlot `
        -DelaySeconds 0 `
        -BattlePlayable `
        -LiveInputPreview `
        -CPUOpponentProof `
        -MatchLifecycleProof `
        -HardwareTriangles `
        -RendererProfileLevel 0 `
        -IFCommonHybridOamMode 0 `
        -FoxCpuMode 1 `
        -RendererBenchmarkTimeoutSeconds 1200 `
        -Task9FloatItcmMode 1 `
        -Task9FloatPhase2Mode 1 `
        -Task16FloatCompareMode $Mode `
        -Task16FloatI2fMode $Mode `
        -Task16FloatAddSubMode $Mode `
        -Task9StateHashMode 1 `
        -Task9StateHashExportPath $ExportPath `
        -Harness 'battle_playable_match_lifecycle' `
        -Target $Target `
        -Build $Build `
        -ExpectedMode 163 `
        -ExpectedHarnessSceneCurr 22 `
        -ExpectedHarnessScenePrev 21 `
        -Label "Task 16 combined state mode=$Mode" `
        -HarnessSelectMessage `
            'Task 16 combined state run did not select Pupupu VSBattle.'
    if ($LASTEXITCODE -ne 0) {
        throw "Task 16 combined state run mode=$Mode failed."
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
    Invoke-Task16CombinedStateRun `
        -Mode 0 `
        -Target 'smash64ds-task16-combined-state-control' `
        -Build 'builds/build-task16-combined-state-control' `
        -ExportPath $controlPath
    Invoke-Task16CombinedStateRun `
        -Mode 1 `
        -Target 'smash64ds-task16-combined-state-candidate' `
        -Build 'builds/build-task16-combined-state-candidate' `
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

$control = Get-Content -LiteralPath $controlPath -Raw | ConvertFrom-Json
$candidate = Get-Content -LiteralPath $candidatePath -Raw | ConvertFrom-Json
foreach ($case in @(
    @{ Name='control'; Data=$control; Mode=0;
       Target='smash64ds-task16-combined-state-control';
       Build='builds/build-task16-combined-state-control' },
    @{ Name='candidate'; Data=$candidate; Mode=1;
       Target='smash64ds-task16-combined-state-candidate';
       Build='builds/build-task16-combined-state-candidate' }
)) {
    $build = Join-Path $root $case.Build
    $config = Join-Path $build 'nds_build_config.h'
    $elf = Join-Path $build "$($case.Data.target).elf"
    $rom = Join-Path $build "$($case.Data.target).nds"
    $configText = Get-Content -LiteralPath $config -Raw
    if (($case.Data.target -cne $case.Target) -or
        ($case.Data.build -cne $case.Build) -or
        ($case.Data.task9FloatItcmMode -ne 1) -or
        ($case.Data.task9FloatPhase2Mode -ne 1) -or
        ($case.Data.task9StateHashMode -ne 1)) {
        throw "$($case.Name) state artifact has the wrong target/Task9 identity."
    }
    foreach ($macro in @(
        'NDS_TASK16_FLOAT_COMPARE',
        'NDS_TASK16_FLOAT_I2F',
        'NDS_TASK16_FLOAT_ADDSUB'
    )) {
        if ($configText -notmatch
            ("(?m)^#define {0} {1}$" -f $macro, $case.Mode)) {
            throw "$($case.Name) build config has the wrong $macro mode."
        }
    }
    foreach ($property in @(
        'task16FloatCompareMode',
        'task16FloatI2fMode',
        'task16FloatAddSubMode'
    )) {
        if ($case.Data.$property -ne $case.Mode) {
            throw "$($case.Name) state artifact has the wrong $property."
        }
    }
    if ((Get-FileHash $elf -Algorithm SHA256).Hash -cne
        $case.Data.artifacts.elf.Sha256) {
        throw "$($case.Name) state artifact ELF identity drifted."
    }
    if ((Get-FileHash $rom -Algorithm SHA256).Hash -cne
        $case.Data.artifacts.rom.Sha256) {
        throw "$($case.Name) state artifact ROM identity drifted."
    }

    & $itcmChecker `
        -Elf $elf `
        -BuildDirectory $build `
        -Phase2Mode 1 `
        -Task16CompareMode $case.Mode `
        -Task16I2fMode $case.Mode `
        -Task16AddSubMode $case.Mode
    if ($LASTEXITCODE -ne 0) {
        throw "$($case.Name) combined ITCM/codegen placement failed."
    }
    $symbols = @(& $Objdump -t $elf)
    if ($LASTEXITCODE -ne 0 -or
        @($symbols | Where-Object {
            $_ -match '\sF\s+\.itcm\s+00000198.*\s__aeabi_fmul$'
        }).Count -ne 1 -or
        @($symbols | Where-Object {
            $_ -match '__nds_task16_libgcc_fmul_golden$'
        }).Count -ne 0) {
        throw "$($case.Name) ELF did not retain the stock 408-byte fmul."
    }
}

if (($control.artifacts.elf.sha256 -ceq
        $candidate.artifacts.elf.sha256) -or
    ($control.artifacts.rom.sha256 -ceq
        $candidate.artifacts.rom.sha256)) {
    throw 'Task 16 combined state A/B reused an ELF or ROM identity.'
}
if ($control.coverage.overflow -ne 0 -or
    $candidate.coverage.overflow -ne 0 -or
    $control.coverage.updates -ne 3892 -or
    $candidate.coverage.updates -ne 3892 -or
    $control.rows.Count -ne 3892 -or
    $candidate.rows.Count -ne 3892) {
    throw ("Task 16 combined state coverage mismatch: " +
        "control=$($control.rows.Count)/$($control.coverage.overflow) " +
        "candidate=$($candidate.rows.Count)/$($candidate.coverage.overflow).")
}
for ($index = 0; $index -lt 3892; ++$index) {
    $a = @($control.rows[$index])
    $b = @($candidate.rows[$index])
    if (($a.Count -ne 6) -or ($b.Count -ne 6) -or
        ($a[0] -ne $index) -or ($b[0] -ne $index) -or
        ($a[3] -le 0) -or ($b[3] -le 0) -or
        ($a[4] -le 0) -or ($b[4] -le 0) -or
        ($a[5] -ne 0) -or ($b[5] -ne 0) -or
        (($a -join ',') -cne ($b -join ','))) {
        throw ("Task 16 combined state divergence at update $index`n" +
            "control=$($a -join ',')`ncandidate=$($b -join ',')")
    }
}

$controlSha = (Get-FileHash $controlPath -Algorithm SHA256).Hash
$candidateSha = (Get-FileHash $candidatePath -Algorithm SHA256).Hash
Write-Output (('Task 16 combined SUPREME state gate PASS: rows=3892 ' +
    'overflow=0 modes=0/0/0->1/1/1 stock_fmul=408B ' +
    'control_json_sha256={0} candidate_json_sha256={1}') -f
    $controlSha, $candidateSha)
