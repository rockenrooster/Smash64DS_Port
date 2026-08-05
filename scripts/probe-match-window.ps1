[CmdletBinding()]
param(
    [string]$MelonDS = '',
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [int]$GdbPort = 4660,
    [int]$RunnerSlot = 6,
    [Parameter(Mandatory=$true)][string]$Build,
    [Parameter(Mandatory=$true)][string]$Arm,
    [ValidateRange(1,60)][int]$TimeLimitMinutes = 1,
    [ValidateRange(1,1000000)][int]$StartFrame = 440,
    [ValidateRange(1,1000000)][int]$EndFrame = 2040,
    [ValidateRange(30,3600)][int]$TimeoutSeconds = 900,
    [string]$JsonOut = ''
)

# WHAT MATCH TIME DOES THE SAMPLING WINDOW ACTUALLY COVER?
#
# Every gate figure in this campaign is sampled at presented frames 440-2040
# and labelled "whole match". That label was never checked, and the two arms
# do not run the same match: scene_harness.c:182 seeds time_limit = 1 for
# Boundary, and :221 seeds time_limit = 7 under NDS_R2_BOTH_CPU -- a
# SEVEN-minute match, sized for the freeze soak, not for tick sampling.
#
# The frames-to-seconds conversion cannot be done by arithmetic from the VBI
# histogram, because that assumes a fixed logic-to-presented ratio and this
# ROM is over budget, so the ratio is exactly what is in question. Read it
# instead:
#
#   gNdsBattleTextHudTimeSeconds        the match clock the HUD displays
#   gNdsBattlePlayablePacingLogicFrames source-side logic frames
#   gNdsBattlePlayablePacingPresentedFrames
#
# Two stops, at the window's own edges, differenced. That gives the match time
# the window covers, the fraction of the configured match it represents, and
# the logic:presented ratio -- none of which any banked figure carries.
#
# Every value read is a code-published global; no stack object is touched, and
# the statics are avoided entirely. One breakpoint per phase, deleted between
# them, because a single breakpoint gated on StartFrame stops at the very next
# frame after the first sample and yields a 2-frame window reported under
# whatever EndFrame said.

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'lib\melonds.ps1')
. (Join-Path $PSScriptRoot 'lib\build-output.ps1')

$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$target = 'smash64ds-battle-playable-tickhud-hwtri'

$counters = @(
    'gNdsBattleTextHudTimeSeconds',
    'gNdsBattlePlayablePacingLogicFrames',
    'gNdsBattlePlayablePacingPresentedFrames'
)

$context = Initialize-MelonDSVerifierContext `
    -Root $root -MelonDS $MelonDS -RunnerSlot $RunnerSlot `
    -GdbPort $GdbPort -GdbPortExplicit:$PSBoundParameters.ContainsKey('GdbPort') `
    -NoBuild
$rom = Resolve-Smash64DSBuildOutput -Root $root -Target $target -Build $Build -Extension '.nds'
$elf = Resolve-Smash64DSBuildOutput -Root $root -Target $target -Build $Build -Extension '.elf'
$temp = Get-MelonDSVerifierTempDir -Root $root -RunnerSlot $RunnerSlot
$gdbScript = Join-Path $temp 'match-window.gdb'
$gdbOut = Join-Path $temp 'match-window.gdb.out'
$gdbErr = Join-Path $temp 'match-window.gdb.err'
$emulatorOut = Join-Path $temp 'match-window.melonds.out'
$emulatorErr = Join-Path $temp 'match-window.melonds.err'
$configState = $null
$emulator = $null

function New-PhaseCommands {
    param([string]$Tag, [int]$Frame)
    $format = (, '%u' * $counters.Count) -join ','
    @(
        'delete breakpoints',
        'break ndsBattlePlayableFrameCompleteMarker',
        'commands',
        'silent',
        "if gNdsBattlePlayablePacingPresentedFrames < $Frame",
        'continue',
        'end',
        'end',
        'continue',
        ("printf `"MW=$Tag,$format\n`", " + ($counters -join ', '))
    )
}

try {
    foreach ($path in @($rom, $elf, $Gdb)) {
        if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
            throw "Required match-window probe file is missing: $path"
        }
    }
    $nm = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-nm.exe'
    $symbols = & $nm $elf | ForEach-Object { ($_ -split '\s+')[-1] }
    $missing = $counters | Where-Object { $symbols -notcontains $_ }
    if ($missing.Count -gt 0) {
        throw ("Match-window symbols absent from {0}: {1}" -f $elf, ($missing -join ', '))
    }

    $configState = Enable-MelonDSGdbConfig `
        -MelonDSPath $context.MelonDSPath -GdbPort $context.GdbPort `
        -Arm7Port $context.Arm7Port `
        -Persistent:([bool]$context.PersistentConfig) -MuteAudio
    Remove-Item $gdbOut, $gdbErr, $emulatorOut, $emulatorErr -Force -ErrorAction SilentlyContinue
    $emulator = Start-Process -FilePath $context.MelonDSPath `
        -ArgumentList $rom `
        -WorkingDirectory (Split-Path -Parent $context.MelonDSPath) `
        -RedirectStandardOutput $emulatorOut -RedirectStandardError $emulatorErr `
        -WindowStyle Hidden -PassThru
    Wait-MelonDSGdbListener -Process $emulator -Port $context.GdbPort | Out-Null

    $gdbLines = @(
        'set pagination off', 'set confirm off',
        'set print elements 0', 'set print repeats 0', 'set print pretty off',
        'set remotetimeout 60',
        "target remote 127.0.0.1:$($context.GdbPort)"
    ) + (New-PhaseCommands -Tag 'A' -Frame $StartFrame) `
      + (New-PhaseCommands -Tag 'B' -Frame $EndFrame) + @('detach')

    [System.IO.File]::WriteAllLines($gdbScript,
        @($gdbLines | ForEach-Object { $_ } | Where-Object { -not [string]::IsNullOrEmpty($_) }))
    $gdbProcess = Start-Process -FilePath $Gdb `
        -ArgumentList @('-q', '-batch', '-x', $gdbScript, $elf) `
        -WorkingDirectory $root `
        -RedirectStandardOutput $gdbOut -RedirectStandardError $gdbErr `
        -WindowStyle Hidden -PassThru
    if (-not $gdbProcess.WaitForExit($TimeoutSeconds * 1000)) {
        Stop-Process -Id $gdbProcess.Id -Force
        throw "Match-window probe exceeded ${TimeoutSeconds}s."
    }

    $lines = Get-Content $gdbOut -ErrorAction SilentlyContinue
    $samples = @{}
    foreach ($tag in 'A', 'B') {
        $line = $lines | Where-Object { $_ -match "^MW=$tag," } | Select-Object -First 1
        if (-not $line) {
            Get-Content $gdbOut -ErrorAction SilentlyContinue | Write-Host
            Get-Content $gdbErr -ErrorAction SilentlyContinue | Write-Host
            throw "Match-window probe produced no sample $tag (window may not be reachable)."
        }
        $parts = ($line -replace "^MW=$tag,", '') -split ','
        $s = [ordered]@{}
        for ($i = 0; $i -lt $counters.Count; $i++) { $s[$counters[$i]] = [uint32]$parts[$i] }
        $samples[$tag] = $s
    }

    $secA = [int]$samples['A']['gNdsBattleTextHudTimeSeconds']
    $secB = [int]$samples['B']['gNdsBattleTextHudTimeSeconds']
    $logicA = [int64]$samples['A']['gNdsBattlePlayablePacingLogicFrames']
    $logicB = [int64]$samples['B']['gNdsBattlePlayablePacingLogicFrames']
    $presA = [int64]$samples['A']['gNdsBattlePlayablePacingPresentedFrames']
    $presB = [int64]$samples['B']['gNdsBattlePlayablePacingPresentedFrames']
    $matchSeconds = $TimeLimitMinutes * 60
    $elapsed = [math]::Abs($secA - $secB)
    $dPres = $presB - $presA
    $dLogic = $logicB - $logicA

    Write-Host ""
    Write-Host ("MATCH WINDOW -- arm $Arm, build $Build, frames $StartFrame..$EndFrame")
    Write-Host ("  clock            {0,8} s  ->{1,8} s   (elapsed {2} s of a {3} s match)" -f $secA, $secB, $elapsed, $matchSeconds)
    Write-Host ("  presented frames {0,8:N0}  ->{1,8:N0}   (delta {2:N0})" -f $presA, $presB, $dPres)
    Write-Host ("  logic frames     {0,8:N0}  ->{1,8:N0}   (delta {2:N0})" -f $logicA, $logicB, $dLogic)
    Write-Host ("  logic : presented = {0:N3}" -f $(if ($dPres) { $dLogic / $dPres } else { 0 }))
    Write-Host ("  WINDOW COVERS {0:P1} OF THE CONFIGURED MATCH" -f $(if ($matchSeconds) { $elapsed / $matchSeconds } else { 0 }))
    Write-Host ""

    if ($JsonOut) {
        $payload = [ordered]@{
            probe = 'match window coverage'; arm = $Arm; build = $Build
            rom = $rom; romSha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $rom).Hash
            timeLimitMinutes = $TimeLimitMinutes; matchSeconds = $matchSeconds
            startFrame = $StartFrame; endFrame = $EndFrame
            clockStart = $secA; clockEnd = $secB; elapsedSeconds = $elapsed
            presentedDelta = $dPres; logicDelta = $dLogic
            logicPerPresented = $(if ($dPres) { $dLogic / $dPres } else { 0 })
            fractionOfMatch = $(if ($matchSeconds) { $elapsed / $matchSeconds } else { 0 })
            capturedUtc = (Get-Date).ToUniversalTime().ToString('o')
            sampleA = $samples['A']; sampleB = $samples['B']
        }
        $jsonPath = if ([System.IO.Path]::IsPathRooted($JsonOut)) { $JsonOut } else { Join-Path $root $JsonOut }
        $payload | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $jsonPath
        Write-Host "Wrote $jsonPath"
    }
} finally {
    if ($null -ne $emulator) {
        $emulator.Refresh()
        if (-not $emulator.HasExited) { Stop-Process -Id $emulator.Id -Force; $emulator.WaitForExit() }
    }
    Restore-MelonDSGdbConfig -State $configState
}
