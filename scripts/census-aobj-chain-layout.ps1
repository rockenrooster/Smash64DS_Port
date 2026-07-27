[CmdletBinding()]
param(
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [int]$GdbPort = 4620,
    [int]$RunnerSlot = -1,
    [string]$Build = 'build-task96-e1',
    [switch]$NoBuild,
    [ValidateRange(1,1000000)][int]$StartFrame = 439,
    [ValidateRange(2,600)][int]$WindowFrames = 30,
    [ValidateRange(30,3600)][int]$TimeoutSeconds = 900,
    [string]$JsonOut = ''
)

# Task 96 E1. Decides whether the wholesale animation channel rewrite is aimed
# at the real cost, before it is worth a session.
#
# gcPlayDObjAnimJoint costs ~99,000 ticks/frame -- 33,900 of its own plus the
# ~65,000 of soft-float Task 92 attributed to it -- and Task 81 puts its class
# at 68.4% stall. The rewrite's premise is that the aobj->next walk is that
# stall. That premise has an arithmetic ceiling: a DS main-RAM miss is ~30-60
# ticks, so ~68,000 ticks of stall needs well over a thousand misses per frame.
# If the walk is only a few dozen nodes, it cannot be the cost and the rewrite
# is aimed at the wrong thing.
#
# Two corrections over E0, which was discarded:
#   - counts every 32-byte line a node SPANS, not the line its first byte falls
#     in. AObj is 36 bytes, so an unaligned node straddles two lines; counting
#     starts compared unlike quantities and made scattering look cheaper than
#     packing.
#   - instruments the joint player itself, so all three callers are counted --
#     gcPlayAnimAll, ftParamUpdateAnimKeys and ndsBaseFTCommonGuardUpdateJoints.
#     E0 saw only the first.
#
# This is a counting build. The census wrapper perturbs inlining, so the ROM it
# produces must never be used for timing.

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'lib\melonds.ps1')
. (Join-Path $PSScriptRoot 'lib\build-output.ps1')

$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$target = 'smash64ds-battle-playable-tickhud-hwtri'

$counters = @(
    'gNdsTask96Calls',
    'gNdsTask96Nodes',
    'gNdsTask96LineSpans',
    'gNdsTask96FlatSpans',
    'gNdsTask96AdjacentPairs',
    'gNdsTask96Pairs',
    'gNdsTask96MaxChain',
    'gNdsTask96AObjBytes'
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
$gdbScript = Join-Path $temp 'aobj.gdb'
$gdbOut = Join-Path $temp 'aobj.gdb.out'
$gdbErr = Join-Path $temp 'aobj.gdb.err'
$emulatorOut = Join-Path $temp 'aobj.melonds.out'
$emulatorErr = Join-Path $temp 'aobj.melonds.err'
$configState = $null
$emulator = $null

function New-SampleCommands {
    param([string]$Tag, [int]$Frame)

    $fields = $counters -join ', '
    $format = (, '%u' * $counters.Count) -join ','
    # `while`, not `if`: a top-level `if ... continue ... end` resumes exactly
    # once, so the sample lands one frame past the previous stop rather than at
    # the requested frame. The first run of this script reported a 2-frame
    # window for a 30-frame request that way.
    @(
        "while gNdsBattlePlayablePacingPresentedFrames < $Frame",
        'continue',
        'end',
        ("printf `"AOBJ=$Tag,%u,$format\n`", " +
            "gNdsBattlePlayablePacingPresentedFrames, $fields")
    )
}

try {
    if (-not $NoBuild) {
        if (-not $env:DEVKITPRO) { $env:DEVKITPRO = 'C:/devkitPro' }
        if (-not $env:DEVKITARM) { $env:DEVKITARM = 'C:/devkitPro/devkitARM' }
        make -C $root "TARGET=$target" "BUILD=$Build" NDS_TASK96_AOBJ_CENSUS=1 -j16
        if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    }
    foreach ($path in @($rom, $elf, $Gdb)) {
        if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
            throw "Required AObj census file is missing: $path"
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
        -PassThru
    if (-not $gdbProcess.WaitForExit($TimeoutSeconds * 1000)) {
        Stop-Process -Id $gdbProcess.Id -Force
        throw "AObj census exceeded ${TimeoutSeconds}s."
    }
    if ($gdbProcess.ExitCode -ne 0) {
        Get-Content $gdbErr -ErrorAction SilentlyContinue | Write-Host
        throw "AObj census GDB run failed with exit code $($gdbProcess.ExitCode)."
    }

    $out = Get-Content $gdbOut -ErrorAction SilentlyContinue
    $samples = @{}
    foreach ($tag in 'A', 'B') {
        $line = $out | Where-Object { $_ -match "^AOBJ=$tag," } | Select-Object -First 1
        if (-not $line) {
            $out | Write-Host
            throw "AObj census produced no sample $tag."
        }
        $parts = ($line -replace "^AOBJ=$tag,", '') -split ','
        $s = [ordered]@{ frame = [uint32]$parts[0] }
        for ($i = 0; $i -lt $counters.Count; $i++) {
            $s[$counters[$i]] = [uint32]$parts[$i + 1]
        }
        $samples[$tag] = $s
    }

    $frames = [int]$samples['B'].frame - [int]$samples['A'].frame
    if ($frames -le 0) {
        throw "AObj census window is $frames frames; the two stops did not advance."
    }
    $delta = [ordered]@{}
    foreach ($c in $counters) {
        $delta[$c] = [int64]$samples['B'][$c] - [int64]$samples['A'][$c]
    }

    Write-Host ""
    Write-Host ("Task 96 E1 -- AObj chain, all callers, over $frames presented frames")
    Write-Host ("(frames {0} .. {1})" -f $samples['A'].frame, $samples['B'].frame)
    Write-Host ""

    $nodes = $delta['gNdsTask96Nodes']
    if ($nodes -gt 0) {
        $spans = $delta['gNdsTask96LineSpans']
        $flat = [math]::Max(1, $delta['gNdsTask96FlatSpans'])
        $pairs = [math]::Max(1, $delta['gNdsTask96Pairs'])
        $nodesPerFrame = $nodes / $frames
        Write-Host ("AObj size             {0} bytes" -f $samples['B']['gNdsTask96AObjBytes'])
        Write-Host ("joint-player calls    {0:N1} / frame" -f ($delta['gNdsTask96Calls'] / $frames))
        Write-Host ("nodes walked          {0:N1} / frame" -f $nodesPerFrame)
        Write-Host ("longest chain         {0} nodes" -f $samples['B']['gNdsTask96MaxChain'])
        Write-Host ("adjacent pairs        {0:N0} of {1:N0} = {2:P1}" -f `
            $delta['gNdsTask96AdjacentPairs'], $pairs,
            ($delta['gNdsTask96AdjacentPairs'] / $pairs))
        Write-Host ""
        Write-Host ("line spans touched    {0:N1} / frame" -f ($spans / $frames))
        Write-Host ("if packed flat        {0:N1} / frame" -f ($flat / $frames))
        Write-Host ("spans saved by flat   {0:N1} / frame" -f (($spans - $flat) / $frames))
        Write-Host ""
        # The arithmetic the whole task exists to settle.
        $saved = ($spans - $flat) / $frames
        Write-Host ("Ceiling on flattening, at 30-60 ticks per avoided miss:")
        Write-Host ("  {0:N0} - {1:N0} ticks/frame" -f ($saved * 30), ($saved * 60))
        Write-Host ("Animation class stall to explain: ~68,000 ticks/frame")
        Write-Host ""
    }

    if ($JsonOut) {
        $payload = [ordered]@{
            task = 'Task 96 E1 - AObj chain layout, all callers'
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
