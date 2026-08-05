[CmdletBinding()]
param(
    [string]$MelonDS = '',
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [int]$GdbPort = 4623,
    [int]$RunnerSlot = 5,
    [string]$Build = 'build-c75-tickhud-publish',
    [ValidateRange(1,1000000)][int]$StartFrame = 440,
    [ValidateRange(1,1000000)][int]$EndFrame = 2040,
    [ValidateRange(30,3600)][int]$TimeoutSeconds = 1500,
    [string]$JsonOut = ''
)

# G1 LIVENESS PROBE -- zero build, existing tick-HUD ROM.
#
# Two questions, board standing rule 3 (eliminate on an already-built ROM
# before spending a measuring run):
#
#   (a) Is the stage texture-site cache actually OFF at runtime?
#       The load-bearing read is gNdsRendererFastRunMode. Reading
#       sNdsRendererStageTextureSitesEnabled at a frame-complete breakpoint is
#       NOT independent evidence: ndsRendererProfileSetOwner recomputes it from
#       the owner on every traversal and EndStageTraversal sets owner NONE, so
#       outside a traversal it reads 0 whatever the mode is. That is a
#       self-validating read. The mode is sample-time independent, so mode == 9
#       proves the conjunction false for EVERY owner including STAGE.
#
#   (b) Does the effect path reach the memo's consult site at all?
#       gNdsEffectRendererTextureReadyCount is accumulated as a delta taken
#       around ndsRendererAdapterBeginStageTraversal() ...
#       EndStageTraversal() (reloc_backend_movement.c:13182-13231), so any
#       non-zero value counts texture binds that happened while
#       sNdsRendererRuntimeOwner == NDS_RENDERER_PROFILE_OWNER_STAGE. Its
#       writer is unguarded by NDS_RENDERER_PROFILE_LEVEL, so it is live in the
#       profile-0 tick-HUD build.
#
# This control CAN read zero and has: Task 81 (2026-07-26) measured stageCalls
# 0 across frames 439-567. That window is 128 frames and the 2026-08-05 era
# boundary rules it unusable -- it reads the cheapest 6% of the match, before
# effects fire. Hence whole-match stops here, 440 .. 2040.
#
# Statics are read by nm address, not by debug info: NDS_WEAK twins and
# file-local symbols resolve wrong or not at all through the stub. Every value
# printed is a code-published global or an absolute address -- no stack object
# is read.
#
# WINDOW MECHANICS -- one breakpoint per phase, deleted between them.
# A single breakpoint whose command list auto-continues while
# frame < StartFrame does NOT give a StartFrame..EndFrame window: after the
# sample-A stop, the top-level `continue` resumes, the breakpoint fires at the
# very next frame, its command list declines to continue (frame >= StartFrame),
# gdb stops, and the script's sample-B printf executes there. That yields a
# 2-frame window reported under whatever EndFrame said. Each phase therefore
# gets its own breakpoint gated on its own target frame, and the printed
# frame numbers are the check that the window is real.

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'lib\melonds.ps1')
. (Join-Path $PSScriptRoot 'lib\build-output.ps1')

$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$target = 'smash64ds-battle-playable-tickhud-hwtri'

# Globals readable by name (nm shows B/D -- external linkage).
$counters = @(
    'gNdsRendererFastRunMode',
    'gNdsEffectRendererDObjDrawCount',
    'gNdsEffectRendererSubmitCount',
    'gNdsEffectRendererTextureReadyCount',
    'gNdsEffectRendererTextureRejectCount',
    'gNdsEffectRendererRejectedDrawCount',
    'gNdsEffectRendererTriangleCount',
    'gNdsStageGCDrawAllLoopHardwareTextureReadyCount'
)

# File-local statics: nm addresses from the c75 tickhud ELF.
$statics = [ordered]@{
    'sNdsRendererStageTextureSitesEnabled' = '0x0219ff90'
    'sNdsRendererRuntimeOwner'             = '0x020cfd20'
}

$context = Initialize-MelonDSVerifierContext `
    -Root $root -MelonDS $MelonDS -RunnerSlot $RunnerSlot `
    -GdbPort $GdbPort -GdbPortExplicit:$PSBoundParameters.ContainsKey('GdbPort') `
    -NoBuild
$rom = Resolve-Smash64DSBuildOutput `
    -Root $root -Target $target -Build $Build -Extension '.nds'
$elf = Resolve-Smash64DSBuildOutput `
    -Root $root -Target $target -Build $Build -Extension '.elf'
$temp = Get-MelonDSVerifierTempDir -Root $root -RunnerSlot $RunnerSlot
$gdbScript = Join-Path $temp 'g1-liveness.gdb'
$gdbOut = Join-Path $temp 'g1-liveness.gdb.out'
$gdbErr = Join-Path $temp 'g1-liveness.gdb.err'
$emulatorOut = Join-Path $temp 'g1-liveness.melonds.out'
$emulatorErr = Join-Path $temp 'g1-liveness.melonds.err'
$configState = $null
$emulator = $null

$allNames = @($counters) + @($statics.Keys)

function New-PhaseCommands {
    param([string]$Tag, [int]$Frame)

    $exprs = @($counters) + @($statics.Values | ForEach-Object { "*(unsigned int *)$_" })
    $format = (, '%u' * $exprs.Count) -join ','
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
        ("printf `"G1=$Tag,%u,$format\n`", " +
            "gNdsBattlePlayablePacingPresentedFrames, " + ($exprs -join ', '))
    )
}

try {
    foreach ($path in @($rom, $elf, $Gdb)) {
        if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
            throw "Required G1 liveness probe file is missing: $path"
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

    $gdbLines = @(
        'set pagination off',
        'set confirm off',
        'set print elements 0',
        'set print repeats 0',
        'set print pretty off',
        'set remotetimeout 30',
        "target remote 127.0.0.1:$($context.GdbPort)"
    ) + (New-PhaseCommands -Tag 'A' -Frame $StartFrame) `
      + (New-PhaseCommands -Tag 'B' -Frame $EndFrame) + @(
        'detach')

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
        throw "G1 liveness probe exceeded ${TimeoutSeconds}s."
    }
    if ($gdbProcess.ExitCode -ne 0) {
        Get-Content $gdbErr -ErrorAction SilentlyContinue | Write-Host
        throw "G1 liveness probe GDB run failed with exit code $($gdbProcess.ExitCode)."
    }

    $lines = Get-Content $gdbOut -ErrorAction SilentlyContinue
    $samples = @{}
    foreach ($tag in 'A', 'B') {
        $line = $lines | Where-Object { $_ -match "^G1=$tag," } | Select-Object -First 1
        if (-not $line) {
            Get-Content $gdbOut -ErrorAction SilentlyContinue | Write-Host
            throw "G1 liveness probe produced no sample $tag."
        }
        $parts = ($line -replace "^G1=$tag,", '') -split ','
        $s = [ordered]@{ frame = [uint32]$parts[0] }
        for ($i = 0; $i -lt $allNames.Count; $i++) {
            $s[$allNames[$i]] = [uint32]$parts[$i + 1]
        }
        $samples[$tag] = $s
    }

    $frames = [int]$samples['B'].frame - [int]$samples['A'].frame
    if ($frames -le 0) {
        throw "G1 liveness window is $frames frames; the two stops did not advance."
    }
    $delta = [ordered]@{}
    foreach ($c in $allNames) {
        $delta[$c] = [int64]$samples['B'][$c] - [int64]$samples['A'][$c]
    }

    Write-Host ""
    Write-Host ("G1 liveness -- whole match, {0} presented frames (frames {1} .. {2})" -f `
        $frames, $samples['A'].frame, $samples['B'].frame)
    Write-Host ("ROM {0}" -f $rom)
    Write-Host ""
    Write-Host "name                                              A          B      delta"
    Write-Host "----------------------------------------  ---------  ---------  ---------"
    foreach ($c in $allNames) {
        Write-Host ("{0,-40}  {1,9:N0}  {2,9:N0}  {3,9:N0}" -f `
            $c, $samples['A'][$c], $samples['B'][$c], $delta[$c])
    }
    Write-Host ""

    $mode = [int]$samples['B']['gNdsRendererFastRunMode']
    $texReady = [int64]$delta['gNdsEffectRendererTextureReadyCount']
    $dobj = [int64]$delta['gNdsEffectRendererDObjDrawCount']
    Write-Host ("(a) gNdsRendererFastRunMode = {0}  -> conjunct 1 {1}" -f `
        $mode, $(if ($mode -eq 9) { 'FALSE (cache off for every owner)' } else { 'NOT 9 -- re-read the premise' }))
    Write-Host ("(b) effect-path texture binds under owner STAGE = {0:N0} over {1} frames ({2:N2}/frame)" -f `
        $texReady, $frames, $(if ($frames) { $texReady / $frames } else { 0 }))
    Write-Host ("    effect DObj draws = {0:N0} ({1:N2}/frame)" -f `
        $dobj, $(if ($frames) { $dobj / $frames } else { 0 }))
    Write-Host ""

    if ($JsonOut) {
        $payload = [ordered]@{
            probe = 'G1 stage texture-site cache liveness'
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
        Write-Host "Wrote $jsonPath"
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
}
