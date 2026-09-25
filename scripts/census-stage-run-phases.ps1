[CmdletBinding()]
param(
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [int]$GdbPort = 4621,
    [int]$RunnerSlot = -1,
    [string]$Build = 'build-task103-census',
    [string]$Target = 'smash64ds-battle-playable-tickhud-hwtri',
    [switch]$NoBuild,
    [ValidateRange(1,1000000)][int]$StartFrame = 439,
    [ValidateRange(2,600)][int]$WindowFrames = 60,
    [ValidateRange(30,3600)][int]$TimeoutSeconds = 900,
    [string]$JsonOut = ''
)

# Diagnostic partition of the current stage path. Timer reads perturb the
# image and frame cost: use these spans to choose work, never as acceptance FPS.
# The two stops exclude scene loading and report positive compiled-GX activity.

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'lib\melonds.ps1')
. (Join-Path $PSScriptRoot 'lib\build-output.ps1')

$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path

$counters = @(
    'gNdsP2StageProgDraws',
    'gNdsP2StageProgWords',
    'gNdsP2StageProgDmas',
    'gNdsP2StageProgDeclines',
    'gNdsTask103GenericTicks',
    'gNdsTask103GenericRunCount',
    'gNdsTask103GenericTriangles',
    'gNdsTask103IterTicks',
    'gNdsTask103IterCount',
    'gNdsTask103CommitTicks',
    'gNdsTask103CommitCount',
    'gNdsTask103MaterialTicks',
    'gNdsTask103PrepareTicks',
    'gNdsTask103PrepareCount',
    'gNdsTask103TraversalTicks',
    'gNdsTask103TraversalCount',
    'gNdsTask103DisplayTicks',
    'gNdsTask103DisplayCount',
    'gNdsTask103FinishTicks',
    'gNdsTask103FinishCount',
    'gNdsTask103PrepAdmitTicks',
    'gNdsTask103PrepValidateTicks',
    'gNdsTask103PrepMatrixTicks',
    'gNdsTask103PrepMaterialTicks',
    'gNdsTask103PrepConfigTicks',
    'gNdsTask103PrepOwnerTicks',
    'gNdsTask103PrepCalls',
    'gNdsTask103OwnValidateTicks',
    'gNdsTask103OwnInitTicks',
    'gNdsTask103OwnInitCount',
    'gNdsTask103OwnStateSpanTicks',
    'gNdsTask103OwnStateSpanCount',
    'gNdsTask103OwnPrepareRunTicks',
    'gNdsTask103OwnPrepareRunCount',
    'gNdsTask103RunHeadTicks',
    'gNdsTask103RunDenseTicks',
    'gNdsTask103RunDenseCount',
    'gNdsTask103RunNearCount'
)

$context = Initialize-MelonDSVerifierContext `
    -Root $root -MelonDS $MelonDS -RunnerSlot $RunnerSlot `
    -GdbPort $GdbPort -GdbPortExplicit:$PSBoundParameters.ContainsKey('GdbPort') `
    -NoBuild:$NoBuild
$rom = Resolve-Smash64DSBuildOutput `
    -Root $root -Target $target -Build $Build -Extension '.nds'
$elf = Resolve-Smash64DSBuildOutput `
    -Root $root -Target $target -Build $Build -Extension '.elf'
$temp = Get-MelonDSVerifierTempDir -Root $root -RunnerSlot $RunnerSlot
$gdbScript = Join-Path $temp 'stagerun.gdb'
$gdbOut = Join-Path $temp 'stagerun.gdb.out'
$gdbErr = Join-Path $temp 'stagerun.gdb.err'
$emulatorOut = Join-Path $temp 'stagerun.melonds.out'
$emulatorErr = Join-Path $temp 'stagerun.melonds.err'
$configState = $null
$emulator = $null

function New-SampleCommands {
    param([string]$Tag, [int]$Frame)

    $fields = $counters -join ', '
    $format = (, '%u' * $counters.Count) -join ','
    # `while`, not `if`: a top-level `if ... continue ... end` resumes exactly
    # once, so the sample lands one frame past the previous stop rather than at
    # the requested frame (Task 96 standing rule).
    @(
        "while gNdsBattlePlayablePacingPresentedFrames < $Frame",
        'continue',
        'end',
        ("printf `"STGRUN=$Tag,%u,$format\n`", " +
            "gNdsBattlePlayablePacingPresentedFrames, $fields")
    )
}

try {
    if (-not $NoBuild) {
        if (-not $env:DEVKITPRO) { $env:DEVKITPRO = 'C:/devkitPro' }
        if (-not $env:DEVKITARM) { $env:DEVKITARM = 'C:/devkitPro/devkitARM' }
        make -C $root "TARGET=$target" "BUILD=$Build" NDS_TASK103_STAGE_RUN_PHASE=1
        if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    }
    foreach ($path in @($rom, $elf, $Gdb)) {
        if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
            throw "Required stage-run census file is missing: $path"
        }
    }

    $configState = Enable-MelonDSGdbConfig `
        -MelonDSPath $context.MelonDSPath -GdbPort $context.GdbPort `
        -Arm7Port $context.Arm7Port `
        -Persistent:([bool]$context.PersistentConfig) -MuteAudio
    Remove-Item $gdbOut, $gdbErr, $emulatorOut, $emulatorErr `
        -Force -ErrorAction SilentlyContinue
    $emulator = Start-Process -FilePath $context.MelonDSPath `
        -ArgumentList $rom `
        -WorkingDirectory (Split-Path -Parent $context.MelonDSPath) `
        -RedirectStandardOutput $emulatorOut `
        -RedirectStandardError $emulatorErr `
        -WindowStyle Hidden -PassThru
    Wait-MelonDSGdbListener -Process $emulator -Port $context.GdbPort | Out-Null

    $endFrame = $StartFrame + $WindowFrames
    $gdbLines = @(
        'set pagination off',
        'set confirm off',
        'set remotetimeout 30',
        "target remote 127.0.0.1:$($context.GdbPort)",
        'break ndsBattlePlayableFrameCompleteMarker',
        'commands',
        'silent',
        "if gNdsBattlePlayablePacingPresentedFrames < $StartFrame",
        'continue',
        'end',
        'end',
        'continue'
    ) + (New-SampleCommands -Tag 'A' -Frame $StartFrame) + @(
        'continue'
    ) + (New-SampleCommands -Tag 'B' -Frame $endFrame) + @('detach')

    [System.IO.File]::WriteAllLines($gdbScript,
        @($gdbLines | Where-Object { -not [string]::IsNullOrEmpty($_) }))
    $gdbProcess = Start-Process -FilePath $Gdb `
        -ArgumentList @('-q', '-batch', '-x', $gdbScript, $elf) `
        -WorkingDirectory $root `
        -RedirectStandardOutput $gdbOut `
        -RedirectStandardError $gdbErr `
        -WindowStyle Hidden -PassThru
    if (-not $gdbProcess.WaitForExit($TimeoutSeconds * 1000)) {
        Stop-Process -Id $gdbProcess.Id -Force
        throw "Stage-run census exceeded ${TimeoutSeconds}s."
    }
    if ($gdbProcess.ExitCode -ne 0) {
        Get-Content $gdbErr -ErrorAction SilentlyContinue | Write-Host
        throw "Stage-run census GDB run failed with exit code $($gdbProcess.ExitCode)."
    }

    $out = Get-Content $gdbOut -ErrorAction SilentlyContinue
    $samples = @{}
    foreach ($tag in 'A', 'B') {
        $line = $out | Where-Object { $_ -match "^STGRUN=$tag," } | Select-Object -First 1
        if (-not $line) {
            $out | Write-Host
            throw "Stage-run census produced no sample $tag."
        }
        $parts = ($line -replace "^STGRUN=$tag,", '') -split ','
        $s = [ordered]@{ frame = [uint32]$parts[0] }
        for ($i = 0; $i -lt $counters.Count; $i++) {
            $s[$counters[$i]] = [uint32]$parts[$i + 1]
        }
        $samples[$tag] = $s
    }

    $frames = [int]$samples['B'].frame - [int]$samples['A'].frame
    if ($frames -le 0) {
        throw "Stage-run census window is $frames frames; the two stops did not advance."
    }
    $delta = [ordered]@{}
    foreach ($c in $counters) {
        $delta[$c] = [int64]$samples['B'][$c] - [int64]$samples['A'][$c]
    }

    if ($delta['gNdsP2StageProgDraws'] -le 0 -or
        $delta['gNdsP2StageProgDmas'] -le 0 -or
        $delta['gNdsP2StageProgDeclines'] -ne 0) {
        throw 'Compiled stage did not submit cleanly in the diagnostic window.'
    }
    $perFrame = [ordered]@{}
    Write-Host "Stage diagnostic: $frames frames, $($samples['A'].frame)..$($samples['B'].frame)"
    foreach ($c in $counters) {
        $perFrame[$c] = [double]$delta[$c] / $frames
        Write-Host ('{0,-38} {1,12:N1} / frame' -f $c, $perFrame[$c])
    }
    Write-Host 'Diagnostic spans include timer overhead; not performance acceptance.'

    if ($JsonOut) {
        $payload = [ordered]@{
            task = 'Stage preparation and compiled submission diagnostic'
            target = $target
            rom = $rom
            romSha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $rom).Hash
            startFrame = [int]$samples['A'].frame
            endFrame = [int]$samples['B'].frame
            frames = $frames
            capturedUtc = (Get-Date).ToUniversalTime().ToString('o')
            sampleA = $samples['A']
            sampleB = $samples['B']
            delta = $delta
            perFrame = $perFrame
        }
        $jsonPath = if ([System.IO.Path]::IsPathRooted($JsonOut)) { $JsonOut }
                    else { Join-Path $root $JsonOut }
        $payload | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $jsonPath
        Write-Host "Wrote $JsonOut"
    }
} finally {
    if ($null -ne $emulator) {
        $emulator.Refresh()
        if (-not $emulator.HasExited) {
            Stop-Process -Id $emulator.Id -Force
            $emulator.WaitForExit()
        }
    }
    Restore-MelonDSGdbConfig -State $configState
    Remove-Item $gdbScript, $gdbOut, $gdbErr, $emulatorOut, $emulatorErr `
        -Force -ErrorAction SilentlyContinue
}
