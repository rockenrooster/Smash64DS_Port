[CmdletBinding()]
param(
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [int]$GdbPort = 4617,
    [int]$RunnerSlot = -1,
    [string]$Build = 'build-task91-phases',
    [switch]$NoBuild,
    [ValidateRange(1,1000000)][int]$StartFrame = 439,
    [ValidateRange(2,600)][int]$WindowFrames = 30,
    [ValidateRange(30,3600)][int]$TimeoutSeconds = 900,
    [string]$JsonOut = '',
    [string[]]$Counters = @(
        'gNdsTask91WalkTicks',
        'gNdsTask91ValidateTicks',
        'gNdsTask91DrawCalls',
        'gNdsTask91NativeEligible'
    ),
    [string[]]$ExtraDefines = @()
)

# Task 91 E1. Times the two phases COMPILER_FIRST_ARCHITECTURE.md's Task 79
# proposes to delete, on the tick-HUD ROM -- the configuration the P95 gate
# actually measures.
#
# ndsFighterMarioFoxDLAllDrawForSlot walks the generic DObj tree
# (ndsFighterCollectAllDObjsWithDL) unconditionally, then revalidates every
# collected display list against the loaded asset and walks each MObj chain,
# and only then runs the native owner path. The generated fighter program knows
# all of that at compile time; the walk and the revalidation exist to
# rediscover and re-prove it every frame, per fighter.
#
# The M2 phase ledger already measures this, but nds_renderer.h:39 restricts it
# to profile level 1 and its only such target overrides FAST_RUN_DEFAULT, so it
# cannot report the Boundary configuration. These counters can.
#
# Two stops, WindowFrames apart, and a difference: the counters accumulate from
# boot and would otherwise be dominated by load and title frames.

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'lib\melonds.ps1')
. (Join-Path $PSScriptRoot 'lib\build-output.ps1')

$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$target = 'smash64ds-battle-playable-tickhud-hwtri'

# The counter set and the extra build defines are parameters (-Counters,
# -ExtraDefines) because every experiment in this phase needs a different pair,
# and hand-editing them here was the routine step before each run. The rest of
# the script reads $counters, which is the same variable as $Counters.

$context = Initialize-MelonDSVerifierContext `
    -Root $root -MelonDS $MelonDS -RunnerSlot $RunnerSlot `
    -GdbPort $GdbPort -GdbPortExplicit:$PSBoundParameters.ContainsKey('GdbPort') `
    -NoBuild:$NoBuild
$rom = Resolve-Smash64DSBuildOutput `
    -Root $root -Target $target -Build $Build -Extension '.nds'
$elf = Resolve-Smash64DSBuildOutput `
    -Root $root -Target $target -Build $Build -Extension '.elf'
$temp = Get-MelonDSVerifierTempDir -Root $root -RunnerSlot $RunnerSlot
$gdbScript = Join-Path $temp 'shade-census.gdb'
$gdbOut = Join-Path $temp 'shade-census.gdb.out'
$gdbErr = Join-Path $temp 'shade-census.gdb.err'
$emulatorOut = Join-Path $temp 'shade-census.melonds.out'
$emulatorErr = Join-Path $temp 'shade-census.melonds.err'
$configState = $null
$emulator = $null

function New-SampleCommands {
    param([string]$Tag, [int]$Frame)

    $fields = ($counters | ForEach-Object { $_ }) -join ', '
    $format = (, '%u' * $counters.Count) -join ','
    @(
        "if gNdsBattlePlayablePacingPresentedFrames < $Frame",
        'continue',
        'end',
        ("printf `"SHADE=$Tag,%u,$format\n`", " +
            "gNdsBattlePlayablePacingPresentedFrames, $fields")
    )
}

try {
    if (-not $NoBuild) {
        if (-not $env:DEVKITPRO) { $env:DEVKITPRO = 'C:/devkitPro' }
        if (-not $env:DEVKITARM) { $env:DEVKITARM = 'C:/devkitPro/devkitARM' }
        make -C $root "TARGET=$target" "BUILD=$Build" `
            NDS_TASK91_DRAW_PHASE_CENSUS=1 @ExtraDefines -j16
        if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    }
    foreach ($path in @($rom, $elf, $Gdb)) {
        if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
            throw "Required fighter draw-phase census file is missing: $path"
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
        'set print elements 0',
        'set print repeats 0',
        'set print pretty off',
        'set remotetimeout 30',
        "target remote 127.0.0.1:$($context.GdbPort)",
        'break ndsBattlePlayableFrameCompleteMarker',
        'commands',
        'silent',
        # Stop only at the two sample frames. Without the second clause the
        # breakpoint stops on every frame after StartFrame, the script's single
        # `continue` returns one frame later, and the window silently collapses
        # to 2 frames however large -WindowFrames is.
        "if gNdsBattlePlayablePacingPresentedFrames < $StartFrame",
        'continue',
        'end',
        "if gNdsBattlePlayablePacingPresentedFrames > $StartFrame",
        "if gNdsBattlePlayablePacingPresentedFrames < $endFrame",
        'continue',
        'end',
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
        -PassThru
    if (-not $gdbProcess.WaitForExit($TimeoutSeconds * 1000)) {
        Stop-Process -Id $gdbProcess.Id -Force
        throw "Fighter draw-phase census exceeded ${TimeoutSeconds}s."
    }
    if ($gdbProcess.ExitCode -ne 0) {
        Get-Content $gdbErr -ErrorAction SilentlyContinue | Write-Host
        throw "Fighter draw-phase census GDB run failed with exit code $($gdbProcess.ExitCode)."
    }

    $lines = Get-Content $gdbOut -ErrorAction SilentlyContinue
    $samples = @{}
    foreach ($tag in 'A', 'B') {
        $line = $lines | Where-Object { $_ -match "^SHADE=$tag," } | Select-Object -First 1
        if (-not $line) {
            Get-Content $gdbOut -ErrorAction SilentlyContinue | Write-Host
            throw "Fighter draw-phase census produced no sample $tag."
        }
        $parts = ($line -replace "^SHADE=$tag,", '') -split ','
        $s = [ordered]@{ frame = [uint32]$parts[0] }
        for ($i = 0; $i -lt $counters.Count; $i++) {
            $s[$counters[$i]] = [uint32]$parts[$i + 1]
        }
        $samples[$tag] = $s
    }

    $frames = [int]$samples['B'].frame - [int]$samples['A'].frame
    if ($frames -le 0) {
        throw "Fighter draw-phase census window is $frames frames; the two stops did not advance."
    }
    $delta = [ordered]@{}
    foreach ($c in $counters) {
        $delta[$c] = [int64]$samples['B'][$c] - [int64]$samples['A'][$c]
    }

    $perFrame = { param($v) [math]::Round($v / $frames, 1) }

    Write-Host ""
    Write-Host ("Task 91 E1 -- fighter draw walk/validation over $frames presented frames")
    Write-Host ("(frames {0} .. {1})" -f $samples['A'].frame, $samples['B'].frame)
    Write-Host ""
    Write-Host "counter                       total     per frame"
    Write-Host "--------------------------  ---------  ----------"
    foreach ($c in $counters) {
        Write-Host ("{0,-26}  {1,9:N0}  {2,10:N1}" -f `
            ($c -replace '^gNdsTask91', ''), $delta[$c], (& $perFrame $delta[$c]))
    }
    Write-Host ""
    Write-Host ""

    if ($JsonOut) {
        $payload = [ordered]@{
            task = 'Task 91 E1 - fighter draw walk and revalidation cost'
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
