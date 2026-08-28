[CmdletBinding()]
param(
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\\emulators\\melonds\\melonDS.exe'),
    [string]$Gdb = 'C:\\devkitPro\\devkitARM\\bin\\arm-none-eabi-gdb.exe',
    [int]$GdbPort = 4333,
    [int]$RunnerSlot = -1,
    [switch]$NoBuild,
    [int]$DelaySeconds = 0,
    [ValidateRange(60,1800)][int]$TimeoutSeconds = 300,
    [ValidateRange(1,120)][int]$StateHashStride = 10,
    [string]$Artifact = ''
)

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
. (Join-Path $PSScriptRoot 'lib\\melonds.ps1')
. (Join-Path $PSScriptRoot 'lib\\gdb-markers.ps1')

$target = 'smash64ds-battle-playable-proof-hwtri'
$build = 'builds/build-p2-samus-cpu-determinism'
$buildDir = Join-Path $root $build
$rom = Join-Path $buildDir "$target.nds"
$elf = Join-Path $buildDir "$target.elf"
$buildConfig = Join-Path $buildDir 'nds_build_config.h'
$runAPath = Join-Path $root 'artifacts\\performance\\2026-08-28_p2-3f29-samus-cpu-determinism-a.json'
$runBPath = Join-Path $root 'artifacts\\performance\\2026-08-28_p2-3f29-samus-cpu-determinism-b.json'
if ([string]::IsNullOrWhiteSpace($Artifact)) {
    $Artifact = Join-Path $root 'artifacts\\verification\\2026-08-28_p2-3f29-samus-cpu-determinism.txt'
}

function Assert-SamusDeterminism {
    param([bool]$Condition, [string]$Message, [string]$Evidence = '')
    if (-not $Condition) {
        if ($Evidence) { throw "$Message`n$Evidence" }
        throw $Message
    }
}

function Convert-SamusMarkerInt {
    param([Parameter(Mandatory=$true)][string]$Value)
    if ($Value.StartsWith('0x', [StringComparison]::OrdinalIgnoreCase)) {
        return [int64][Convert]::ToUInt32($Value.Substring(2), 16)
    }
    return [int64]$Value
}

function Get-SamusFileIdentity {
    param([Parameter(Mandatory=$true)][string]$Path)
    $item = Get-Item -LiteralPath $Path
    return [ordered]@{
        sha256 = (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash
        bytes = [int64]$item.Length
    }
}

if (-not $NoBuild) {
    & make -C $root "TARGET=$target" "BUILD=$build" `
        'NDS_P2_LUIGI=1' 'NDS_P2_DONKEY=1' 'NDS_P2_CAPTAIN=1' `
        'NDS_P2_SAMUS=1' 'NDS_P2_PROOF_FIGHTER0=3' `
        'NDS_R2_BOTH_CPU=1' 'NDS_TASK9_STATE_HASH=1' `
        "NDS_TASK9_STATE_HASH_STRIDE=$StateHashStride"
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

foreach ($path in @($rom, $elf, $buildConfig)) {
    Assert-SamusDeterminism (Test-Path -LiteralPath $path -PathType Leaf) `
        "Samus determinism proof input is missing: $path"
}

$configText = Get-Content -LiteralPath $buildConfig -Raw
foreach ($definition in @(
    '#define NDS_DEV_LIVE_INPUT_PREVIEW 1',
    '#define NDS_HARNESS_FAST_LOGIC 0',
    '#define NDS_P2_SAMUS 1',
    '#define NDS_P2_PROOF_FIGHTER0 3',
    '#define NDS_R2_BOTH_CPU 1',
    '#define NDS_IMPORT_BATTLESHIP_FTCOMPUTER 1',
    '#define NDS_IMPORT_BATTLESHIP_BATTLE_PLAYABLE 1',
    '#define NDS_TASK9_STATE_HASH 1',
    "#define NDS_TASK9_STATE_HASH_STRIDE $($StateHashStride)u"
)) {
    Assert-SamusDeterminism $configText.Contains($definition) `
        "Samus determinism build is missing required definition: $definition" $buildConfig
}

$contextParams = @{
    Root = $root
    MelonDS = $MelonDS
    RunnerSlot = $RunnerSlot
    GdbPort = $GdbPort
    NoBuild = $true
}
if ($PSBoundParameters.ContainsKey('GdbPort')) {
    $contextParams.GdbPortExplicit = $true
}
$context = Initialize-MelonDSVerifierContext @contextParams

function Invoke-SamusDeterminismRun {
    param(
        [Parameter(Mandatory=$true)][string]$Name,
        [Parameter(Mandatory=$true)][string]$ExportPath
    )

    $configState = $null
    $emulator = $null
    try {
        $configState = Enable-MelonDSGdbConfig -MelonDSPath $context.MelonDSPath `
            -GdbPort $context.GdbPort -Persistent -MuteAudio
        $emulator = Start-Process -FilePath $context.MelonDSPath -ArgumentList $rom `
            -WorkingDirectory (Split-Path $context.MelonDSPath) `
            -WindowStyle Hidden -PassThru
        Wait-MelonDSGdbListener -Process $emulator -Port $context.GdbPort | Out-Null
        if ($DelaySeconds -gt 0) { Start-Sleep -Seconds $DelaySeconds }

        # BattleShip ftmain.c calls ftComputerProcessAll once per COM fighter per
        # logical update. Its imported ftcomputer.c owns trait/behavior/objective,
        # controller output, and shared syUtilsRand* consumption. This proof never
        # drives fighter state, controller state, or RNG; it only observes the
        # natural Samus-vs-Fox level-3 CPU match and the complete post-update state.
        $commands = @(
            'set pagination off',
            'set confirm off',
            'set remotetimeout 20',
            ("target remote 127.0.0.1:{0}" -f $context.GdbPort),
            'tbreak scVSBattleStartBattle',
            'continue',
            'printf "CPU_CONFIG=%u,%u,%u,%u,%u,%u,%#x,%u,%u\n", gSCManagerBattleState->players[0].pkind, gSCManagerBattleState->players[1].pkind, gSCManagerBattleState->players[1].level, gSCManagerBattleState->pl_count, gSCManagerBattleState->cp_count, gSCManagerBattleState->time_limit, gSCManagerBattleState->item_toggles, gSCManagerBattleState->item_appearance_rate, gNdsBattlePlayableFoxCpuEnabled',
            'set variable gNdsBattlePlayableFoxCpuEnabled = 1',
            'set variable gNdsTask9StateHashArmed = 0',
            'set variable gNdsTask9StateHashCount = 0',
            'set variable gNdsTask9StateHashSourceUpdateCount = 0',
            'set variable gNdsTask9StateHashOverflow = 0',
            'set variable gNdsTask9StateHashArmed = 1',
            'tbreak ndsMNVSResultsRecordFrame',
            'continue',
            'set variable gNdsTask9StateHashArmed = 0',
            ('printf "TASK9_STATE_SUMMARY=%u,%u,%u,%u,%u\n", gNdsTask9StateHashCount, gNdsTask9StateHashOverflow, 4096, gNdsTask9StateHashSourceUpdateCount, {0}' -f $StateHashStride),
            'set $task9_state_index = 0',
            'while $task9_state_index < gNdsTask9StateHashCount',
            'printf "TASK9_STATE=%u,%u,%u,%u,%u,%u\n", $task9_state_index, gNdsTask9StateHashes[$task9_state_index].hash1, gNdsTask9StateHashes[$task9_state_index].hash2, gNdsTask9StateHashes[$task9_state_index].bytes, gNdsTask9StateHashes[$task9_state_index].records, gNdsTask9StateHashes[$task9_state_index].overflow',
            'set $task9_state_index = $task9_state_index + 1',
            'end',
            'printf "CPU_AI=%u,%u,%u,%u,%#x,%#x,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%d,%d,%d,%d\n", gNdsFTComputerSetupCount, gNdsFTComputerDamageDetectCount, gNdsFTComputerProcessCount, gNdsFTComputerTargetFrames, gNdsFTComputerObjectiveMask, gNdsFTComputerBehaviorMask, gNdsFTComputerInputChangeCount, gNdsFTComputerStickFrames, gNdsFTComputerButtonAFrames, gNdsFTComputerButtonBFrames, gNdsFTComputerButtonZFrames, gNdsFTComputerAttackFrames, gNdsFTComputerHitboxFrames, gNdsFTComputerGuardFrames, gNdsFTComputerRecoveryFrames, gNdsFTComputerStatusChangeCount, gNdsFTComputerFinalStatus, gNdsFTComputerFinalGA, gNdsFTComputerFinalInputKind, gNdsFTComputerMarioDamageMax, gNdsFTComputerFloorLineCount, gNdsFTComputerStartXMilli, gNdsFTComputerMinXMilli, gNdsFTComputerMaxXMilli, gNdsFTComputerFinalXMilli',
            'printf "SCENE=%u,%u,%u\n", gSCManagerSceneData.scene_curr, gSCManagerSceneData.scene_prev, gSCManagerSceneData.gkind',
            'detach',
            'quit'
        )
        $scriptName = "p2-samus-cpu-determinism-$Name.gdb"
        Invoke-GdbMarkerScript -Gdb $Gdb -Elf $elf -Root $root `
            -Commands $commands -ScriptName $scriptName `
            -TimeoutSeconds $TimeoutSeconds | Out-Null

        $stdoutPath = Join-Path $env:SMASH64DS_VERIFY_TEMP_DIR ($scriptName + '.out')
        $stdout = Get-Content -LiteralPath $stdoutPath -Raw
        $summaryMatch = [regex]::Match($stdout,
            'TASK9_STATE_SUMMARY=(\d+),(\d+),(\d+),(\d+),(\d+)')
        Assert-SamusDeterminism $summaryMatch.Success `
            'Samus determinism state-hash summary is missing.' $stdout
        $summary = @($summaryMatch.Groups[1..5] | ForEach-Object { [int64]$_.Value })
        $sourceUpdates = $summary[3]
        $stride = $summary[4]
        $expectedSamples = if ($sourceUpdates -gt 0) {
            [int64][Math]::Floor(($sourceUpdates - 1) / $stride) + 1
        } else { 0 }
        Assert-SamusDeterminism ($sourceUpdates -ge 3600 -and
            $stride -eq $StateHashStride -and $summary[0] -eq $expectedSamples -and
            $summary[0] -le $summary[2] -and $summary[1] -eq 0) `
            "Samus determinism state hash did not cover the complete match at stride $StateHashStride." `
            $summaryMatch.Value

        $rowMatches = [regex]::Matches($stdout,
            'TASK9_STATE=(\d+),(\d+),(\d+),(\d+),(\d+),(\d+)')
        Assert-SamusDeterminism ($rowMatches.Count -eq $summary[0]) `
            "Samus determinism expected $($summary[0]) state rows but found $($rowMatches.Count)." $stdout
        $rows = @()
        for ($i = 0; $i -lt $rowMatches.Count; $i++) {
            $row = @($rowMatches[$i].Groups[1..6] | ForEach-Object { [int64]$_.Value })
            Assert-SamusDeterminism ($row[0] -eq $i -and $row[3] -gt 0 -and
                $row[4] -gt 0 -and $row[5] -eq 0) `
                "Samus determinism row $i is incomplete or overflowed." $rowMatches[$i].Value
            $rows += ,$row
        }

        $configMatch = [regex]::Match($stdout,
            'CPU_CONFIG=(\d+),(\d+),(\d+),(\d+),(\d+),(\d+),(0x[0-9a-fA-F]+|\d+),(\d+),(\d+)')
        Assert-SamusDeterminism $configMatch.Success 'Samus CPU configuration marker is missing.' $stdout
        $cpuConfig = @($configMatch.Groups[1..9] | ForEach-Object {
            Convert-SamusMarkerInt $_.Value
        })
        Assert-SamusDeterminism ($cpuConfig[0] -eq 1 -and $cpuConfig[1] -eq 1 -and
            $cpuConfig[2] -eq 3 -and $cpuConfig[3] -eq 0 -and $cpuConfig[4] -eq 2 -and
            $cpuConfig[5] -eq 1 -and $cpuConfig[6] -eq 0 -and $cpuConfig[7] -eq 0 -and
            $cpuConfig[8] -eq 1) `
            'Samus determinism run was not the items-off level-3 CPU-vs-CPU one-minute Time match.' `
            $configMatch.Value

        $cpuMatch = [regex]::Match($stdout, 'CPU_AI=([^\r\n]+)')
        Assert-SamusDeterminism $cpuMatch.Success 'Samus CPU engagement marker is missing.' $stdout
        $cpu = @($cpuMatch.Groups[1].Value.Split(',') | ForEach-Object {
            Convert-SamusMarkerInt $_
        })
        Assert-SamusDeterminism ($cpu.Count -eq 25 -and $cpu[0] -ge 2 -and
            $cpu[1] -ge 2 -and $cpu[2] -ge 3600 -and $cpu[3] -gt 0 -and
            $cpu[6] -gt 0 -and $cpu[7] -gt 0 -and
            ($cpu[8] + $cpu[9] + $cpu[10]) -gt 0 -and $cpu[11] -gt 0 -and
            $cpu[12] -gt 0 -and $cpu[15] -gt 0 -and $cpu[20] -gt 0) `
            'Samus determinism run did not naturally exercise the imported BattleShip CPU decision/input/attack path.' `
            $cpuMatch.Value

        $sceneMatch = [regex]::Match($stdout, 'SCENE=(\d+),(\d+),(\d+)')
        Assert-SamusDeterminism $sceneMatch.Success 'Samus determinism scene marker is missing.' $stdout
        $scene = @($sceneMatch.Groups[1..3] | ForEach-Object { [int64]$_.Value })
        Assert-SamusDeterminism ($scene[0] -eq 24 -and $scene[1] -eq 22 -and $scene[2] -eq 6) `
            'Samus determinism replay did not naturally transition VSBattle to VS Results.' `
            $sceneMatch.Value

        $export = [ordered]@{
            schema = 1
            kind = 'smash64ds-p2-samus-cpu-determinism'
            target = $target
            build = $build
            coverage = [ordered]@{
                source = 'post-scVSBattleFuncUpdate'
                sourceUpdates = $sourceUpdates
                samples = [int64]$summary[0]
                stride = $stride
                overflow = [int64]$summary[1]
            }
            cpu = [ordered]@{
                setup = $cpu[0]
                damageDetect = $cpu[1]
                process = $cpu[2]
                targetFrames = $cpu[3]
                inputChanges = $cpu[6]
                stickFrames = $cpu[7]
                buttonAFrames = $cpu[8]
                buttonBFrames = $cpu[9]
                buttonZFrames = $cpu[10]
                attackFrames = $cpu[11]
                hitboxFrames = $cpu[12]
            }
            artifacts = [ordered]@{
                rom = Get-SamusFileIdentity -Path $rom
                elf = Get-SamusFileIdentity -Path $elf
            }
            rows = $rows
        }
        New-Item -ItemType Directory -Force -Path (Split-Path -Parent $ExportPath) | Out-Null
        Set-Content -LiteralPath $ExportPath -Encoding utf8 `
            -Value ($export | ConvertTo-Json -Depth 8)
        return [PSCustomObject]@{ Export = $export; CpuMarker = $cpuMatch.Value }
    }
    finally {
        if (($null -ne $emulator) -and -not $emulator.HasExited) {
            Stop-Process -Id $emulator.Id -Force -ErrorAction SilentlyContinue
        }
        if ($null -ne $configState) { Restore-MelonDSGdbConfig -State $configState }
    }
}

$runA = Invoke-SamusDeterminismRun -Name 'a' -ExportPath $runAPath
$runB = Invoke-SamusDeterminismRun -Name 'b' -ExportPath $runBPath
$a = $runA.Export
$b = $runB.Export

Assert-SamusDeterminism ($a.artifacts.rom.sha256 -ceq $b.artifacts.rom.sha256) `
    'Samus determinism replay used two different ROMs.'
Assert-SamusDeterminism ($a.coverage.sourceUpdates -eq $b.coverage.sourceUpdates -and
    $a.coverage.samples -eq $b.coverage.samples -and $a.coverage.stride -eq $b.coverage.stride) `
    'Samus determinism replay did not cover the same source-update span twice.'
Assert-SamusDeterminism (($a.cpu | ConvertTo-Json -Compress) -ceq
    ($b.cpu | ConvertTo-Json -Compress)) `
    'Samus determinism replay produced different imported CPU telemetry across the two runs.'
Assert-SamusDeterminism ($a.rows.Count -eq $b.rows.Count) `
    "Samus determinism row counts differ: $($a.rows.Count) vs $($b.rows.Count)."
for ($i = 0; $i -lt $a.rows.Count; $i++) {
    $rowA = @($a.rows[$i])
    $rowB = @($b.rows[$i])
    Assert-SamusDeterminism (($rowA -join ',') -ceq ($rowB -join ',')) `
        "Samus CPU determinism diverged at sampled source update $($i * $StateHashStride)." `
        "A=$($rowA -join ',')`nB=$($rowB -join ',')"
}

$summaryText = ("P2-3 Samus CPU determinism PASS: $($a.coverage.samples) complete " +
    "active-game-state samples identical across two natural one-minute level-3 " +
    "CPU-vs-CPU runs of the same ROM ($($a.coverage.sourceUpdates) source updates, " +
    "stride $($a.coverage.stride)); CPU process=$($a.cpu.process), " +
    "inputs=$($a.cpu.inputChanges), attacks/hitboxes=$($a.cpu.attackFrames)/" +
    "$($a.cpu.hitboxFrames), overflow=0; ROM=$($a.artifacts.rom.sha256).")
$artifactParent = Split-Path -Parent $Artifact
if ($artifactParent) { New-Item -ItemType Directory -Force -Path $artifactParent | Out-Null }
Set-Content -LiteralPath $Artifact -Encoding utf8 -Value @(
    $summaryText,
    $runA.CpuMarker,
    "runA=$runAPath",
    "runB=$runBPath"
)
Write-Output $summaryText
