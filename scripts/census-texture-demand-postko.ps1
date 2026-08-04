[CmdletBinding()]
param(
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [int]$GdbPort = 4623,
    [int]$RunnerSlot = -1,
    [string]$Build = 'build-c71-texdemand',
    [switch]$NoBuild,
    # Presented frame at which the fighter is pushed across the upward blast
    # zone. Must be inside the match and far enough before the window that the
    # KO, the star ascent, and the respawn have all happened.
    [ValidateRange(30, 100000)][int]$DeathFrame = 200,
    # Window start, presented frames. Default sits ~5s of presented time after
    # the forced death, which is past the first rejection in every observed run.
    [ValidateRange(30, 1000000)][int]$StartFrame = 350,
    [ValidateRange(2, 600)][int]$WindowFrames = 40,
    [ValidateRange(30, 3600)][int]$TimeoutSeconds = 900,
    [string]$JsonOut = 'artifacts/performance/texture-demand-postko.json'
)

# HOW MANY DISTINCT TEXTURES DOES THE POST-KO SCENE ACTUALLY ASK FOR?
#
# BUGS row 6 is a texture cache that refuses allocations after the first KO.
# The obvious sizing input -- the reject census's `thisframe` -- CANNOT answer
# it. That census counts non-pinned entries touched this frame, and with
# pinned=28 of 48 there are only 20 such slots, so `thisframe` is clamped at 20
# by the very shortage being measured. It reported exactly 20 at five
# consecutive tags, which is what a censored instrument looks like.
#
# The uncensored quantity is the number of DISTINCT KEYS REQUESTED per frame,
# read at the request site before the cache decides anything. NDS_TASK93_
# TEXKEY_CENSUS already records exactly that: nds_renderer.c:14337 sits after
# the key is built and before ndsRendererHardwareFindTexture, so a request that
# goes on to be rejected is still in the trace. Nothing new is instrumented
# here; this script only aims that existing ring at a post-KO window and reads
# it per frame instead of once.
#
# TWO READS, AND THEY ANSWER DIFFERENT HALVES:
#
#   1. The ring CURSOR (gNdsTask93KeyTraceNext) sampled at every frame marker.
#      Its per-frame delta is requests/frame -- uncensored, one read per frame,
#      and an upper bound on distinct/frame. This runs for the whole window.
#   2. The ring CONTENTS dumped once at the end. Segmented by the cursor
#      samples, it gives distinct keys/frame for the last frames the 256-entry
#      ring still covers. That is the number Route A has to be sized against.
#
# WHY THE DEATH IS FORCED. The window has to sit after a KO: before the first
# one the reject reason mask reads 0, so a clean window measures the wrong
# scene entirely. Two level-3 CPUs do not reliably KO each other inside the
# minute -- every KO in the 2026-08-04 probe runs was forced -- so a "late
# window" would be betting the measurement on an event that may not occur.
# Pushing the fighter across the upward bound is the same lever probe-ko-vfx.ps1
# uses and the same argument applies: it changes WHEN the KO happens, not what
# the KO path then does. ftcommondead.c:635 tests pos->y against map_bound_top,
# so this enters the branch a knockback-launched fighter enters.

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'lib\melonds.ps1')
. (Join-Path $PSScriptRoot 'lib\build-output.ps1')

$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$target = 'smash64ds-battle-playable-tickhud-hwtri'

$context = Initialize-MelonDSVerifierContext `
    -Root $root -MelonDS $MelonDS -RunnerSlot $RunnerSlot `
    -GdbPort $GdbPort -GdbPortExplicit:$PSBoundParameters.ContainsKey('GdbPort') `
    -NoBuild:$NoBuild
$rom = Resolve-Smash64DSBuildOutput `
    -Root $root -Target $target -Build $Build -Extension '.nds'
$elf = Resolve-Smash64DSBuildOutput `
    -Root $root -Target $target -Build $Build -Extension '.elf'
$temp = Get-MelonDSVerifierTempDir -Root $root -RunnerSlot $RunnerSlot
$gdbScript = Join-Path $temp 'texdemand.gdb'
$gdbOut = Join-Path $temp 'texdemand.gdb.out'
$gdbErr = Join-Path $temp 'texdemand.gdb.err'
$emulatorOut = Join-Path $temp 'texdemand.melonds.out'
$emulatorErr = Join-Path $temp 'texdemand.melonds.err'
$configState = $null
$emulator = $null

try {
    if (-not $NoBuild) {
        if (-not $env:DEVKITPRO) { $env:DEVKITPRO = 'C:/devkitPro' }
        if (-not $env:DEVKITARM) { $env:DEVKITARM = 'C:/devkitPro/devkitARM' }
        # No -j and no MAKEFLAGS: the Makefile parallelises itself.
        make -C $root "TARGET=$target" "BUILD=$Build" NDS_TASK93_TEXKEY_CENSUS=1
        if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    }
    foreach ($path in @($rom, $elf, $Gdb)) {
        if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
            throw "Required texture-demand census file is missing: $path"
        }
    }

    # EVERY SYMBOL THIS READS, CHECKED AGAINST THE LINKED ELF FIRST. One absent
    # name makes gdb abandon the rest of a command batch silently, which reads
    # as a healthy transcript with the measurement missing.
    $nm = Join-Path (Split-Path -Parent $Gdb) 'arm-none-eabi-nm.exe'
    $symbols = & $nm $elf | ForEach-Object { ($_ -split '\s+')[-1] }
    $required = @(
        'gNdsTask93KeyTrace', 'gNdsTask93KeyTraceNext', 'gNdsTask93BindCalls',
        'gNdsBattlePlayablePacingPresentedFrames',
        'ndsBattlePlayableFrameCompleteMarker',
        'gGCCommonLinks', 'gMPCollisionGroundData'
    )
    $missing = @($required | Where-Object { $symbols -notcontains $_ })
    if ($missing.Count -gt 0) {
        throw ("census symbols absent from ${elf}: " + ($missing -join ', ') +
            ' -- was NDS_TASK93_TEXKEY_CENSUS=1 set for this build?')
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

        # Run to the death frame, then push the fighter past the upward bound.
        'break ndsBattlePlayableFrameCompleteMarker',
        'commands',
        'silent',
        "if gNdsBattlePlayablePacingPresentedFrames < $DeathFrame",
        'continue',
        'end',
        'end',
        'continue',
        'delete breakpoints',
        'set $g = gGCCommonLinks[3]',
        'set $forced = 0',
        'if $g != 0',
        'set ((DObj *)$g->obj)->translate.vec.f.y = gMPCollisionGroundData->map_bound_top + 500',
        'set $forced = 1',
        'end',
        'printf "TKFORCED=%d,%u\n", $forced, gNdsBattlePlayablePacingPresentedFrames',

        # Sample the ring cursor once per presented frame across the window.
        'break ndsBattlePlayableFrameCompleteMarker',
        'commands',
        'silent',
        "if gNdsBattlePlayablePacingPresentedFrames < $StartFrame",
        'continue',
        'end',
        'printf "TKCUR=%u,%u,%u\n", gNdsBattlePlayablePacingPresentedFrames, gNdsTask93KeyTraceNext, gNdsTask93BindCalls',
        "if gNdsBattlePlayablePacingPresentedFrames < $endFrame",
        'continue',
        'end',
        'end',
        'continue',

        # One ring dump at the end. The cursor samples above segment it.
        'printf "TKNEXT=%u\n", gNdsTask93KeyTraceNext',
        'printf "TKTRACE="',
        'output gNdsTask93KeyTrace',
        'printf "\n"',
        'detach')

    [System.IO.File]::WriteAllLines($gdbScript,
        @($gdbLines | Where-Object { -not [string]::IsNullOrEmpty($_) }))
    $gdbProcess = Start-Process -FilePath $Gdb `
        -ArgumentList @('-q', '-batch', '-x', $gdbScript, $elf) `
        -WorkingDirectory $root `
        -RedirectStandardOutput $gdbOut `
        -RedirectStandardError $gdbErr `
        -WindowStyle Hidden -PassThru
    $timedOut = -not $gdbProcess.WaitForExit($TimeoutSeconds * 1000)
    if ($timedOut) { Stop-Process -Id $gdbProcess.Id -Force }

    # READ THE TRANSCRIPT BEFORE THROWING. A run that timed out after streaming
    # its cursor samples still carries most of the measurement.
    $lines = @(Get-Content $gdbOut -ErrorAction SilentlyContinue)
    $cursors = @()
    foreach ($line in $lines) {
        if ($line -match '^TKCUR=(\d+),(\d+),(\d+)') {
            $cursors += , [pscustomobject]@{
                Frame = [int]$Matches[1]
                Cursor = [int]$Matches[2]
                Binds = [int]$Matches[3]
            }
        }
    }
    $forcedLine = $lines | Where-Object { $_ -match '^TKFORCED=' } | Select-Object -First 1
    $nextLine = $lines | Where-Object { $_ -match '^TKNEXT=' } | Select-Object -First 1
    $traceLine = $lines | Where-Object { $_ -match '^TKTRACE=' } | Select-Object -First 1

    if ($forcedLine -and ($forcedLine -notmatch '^TKFORCED=1')) {
        throw "The forced KO did not arm ($forcedLine); the window is not post-KO."
    }
    if ($cursors.Count -lt 2) {
        Get-Content $gdbErr -ErrorAction SilentlyContinue | Write-Host
        throw ("texture-demand census produced $($cursors.Count) cursor " +
            "samples; transcript at $gdbOut")
    }

    # Requests per frame, from the cursor delta modulo the ring size.
    $ringSize = 256
    $perFrame = @()
    for ($i = 1; $i -lt $cursors.Count; $i++) {
        $delta = ($cursors[$i].Cursor - $cursors[$i - 1].Cursor + $ringSize) % $ringSize
        # BindCalls is monotonic and un-wrapped, so it is the authority when the
        # ring lapped more than once inside one frame.
        $bindDelta = $cursors[$i].Binds - $cursors[$i - 1].Binds
        if ($bindDelta -ge $ringSize) { $delta = $bindDelta }
        elseif ($bindDelta -ne $delta) { $delta = $bindDelta }
        $perFrame += , [pscustomobject]@{
            Frame = $cursors[$i].Frame
            Requests = $delta
        }
    }

    $requests = @($perFrame | ForEach-Object { $_.Requests })
    $sorted = @($requests | Sort-Object)
    $pct = {
        param($p)
        if ($sorted.Count -eq 0) { return 0 }
        $idx = [int][math]::Ceiling(($p / 100.0) * $sorted.Count) - 1
        if ($idx -lt 0) { $idx = 0 }
        if ($idx -ge $sorted.Count) { $idx = $sorted.Count - 1 }
        $sorted[$idx]
    }

    Write-Host ""
    Write-Host ("Post-KO texture demand -- {0} presented frames (frames {1}..{2})" -f `
        $perFrame.Count, $perFrame[0].Frame, $perFrame[-1].Frame)
    Write-Host ("forced KO: {0}" -f $forcedLine)
    Write-Host ""
    Write-Host "requests/frame at the request site (uncensored, includes rejected)"
    Write-Host ("  min {0}   P50 {1}   P95 {2}   max {3}" -f `
        $sorted[0], (& $pct 50), (& $pct 95), $sorted[-1])
    Write-Host ""

    # Distinct keys per frame, for the tail of the window the ring still holds.
    $distinctRows = @()
    if ($nextLine -and $traceLine) {
        $trace = @((($traceLine -replace '^TKTRACE=', '').Trim('{', '}', ' ') -split ',') |
            ForEach-Object { [uint32]($_.Trim()) })
        $next = [int](($nextLine -replace '^TKNEXT=', '').Trim())
        if ($trace.Count -eq $ringSize) {
            # Unroll oldest-first: the cursor points at the slot the next
            # request will overwrite, so that slot is the oldest entry.
            $ordered = @()
            for ($i = 0; $i -lt $ringSize; $i++) {
                $ordered += , $trace[($next + $i) % $ringSize]
            }
            # Walk the per-frame request counts backwards from the newest frame,
            # taking each frame's slice off the end of the ring.
            $endIdx = $ringSize
            for ($i = $perFrame.Count - 1; $i -ge 0; $i--) {
                $n = $perFrame[$i].Requests
                if (($n -le 0) -or (($endIdx - $n) -lt 0)) { break }
                $slice = $ordered[($endIdx - $n)..($endIdx - 1)]
                $set = [System.Collections.Generic.HashSet[uint32]]::new()
                foreach ($k in $slice) { [void]$set.Add($k) }
                $distinctRows += , [pscustomobject]@{
                    Frame = $perFrame[$i].Frame
                    Requests = $n
                    Distinct = $set.Count
                }
                $endIdx -= $n
            }
            $distinctRows = @($distinctRows | Sort-Object Frame)
        }
    }

    if ($distinctRows.Count -gt 0) {
        $d = @($distinctRows | ForEach-Object { $_.Distinct } | Sort-Object)
        Write-Host ("distinct keys/frame, last {0} frames the 256-entry ring covers" -f `
            $distinctRows.Count)
        Write-Host "  frame   requests   distinct"
        Write-Host "  ------  ---------  --------"
        foreach ($row in $distinctRows) {
            Write-Host ("  {0,6}  {1,9}  {2,8}" -f $row.Frame, $row.Requests, $row.Distinct)
        }
        Write-Host ""
        Write-Host ("  distinct: min {0}   P50 {1}   max {2}" -f `
            $d[0], $d[[int][math]::Floor($d.Count / 2)], $d[-1])
        Write-Host ""
        $peak = $d[-1]
        Write-Host "Route A sizing (28 pins stay; evictable must cover the peak)"
        Write-Host ("  peak distinct/frame          : {0}" -f $peak)
        Write-Host ("  81 entries / 40 evictable    : {0}" -f `
            $(if ($peak -le 40) { 'SUFFICIENT' } else { 'INSUFFICIENT' }))
        Write-Host ("  68 entries / 30 evictable    : {0}" -f `
            $(if ($peak -le 30) { 'SUFFICIENT' } else { 'INSUFFICIENT' }))
        Write-Host ("  today 48 entries / 20        : {0}" -f `
            $(if ($peak -le 20) { 'SUFFICIENT' } else { 'INSUFFICIENT' }))
        Write-Host ""
    }
    else {
        Write-Host "No ring dump parsed; requests/frame above is the upper bound."
        Write-Host ""
    }

    if ($JsonOut) {
        $payload = [ordered]@{
            task = 'BUGS row 6 - post-KO texture demand at the request site'
            target = $target
            rom = $rom
            romSha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $rom).Hash
            deathFrame = $DeathFrame
            startFrame = $StartFrame
            windowFrames = $WindowFrames
            forced = $forcedLine
            timedOut = $timedOut
            capturedUtc = (Get-Date).ToUniversalTime().ToString('o')
            requestsPerFrame = $perFrame
            distinctPerFrame = $distinctRows
        }
        $jsonPath = if ([System.IO.Path]::IsPathRooted($JsonOut)) { $JsonOut }
                    else { Join-Path $root $JsonOut }
        New-Item -ItemType Directory -Force -Path (Split-Path -Parent $jsonPath) | Out-Null
        $payload | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $jsonPath
        Write-Host "Wrote $jsonPath"
    }

    if ($timedOut) {
        throw "texture-demand census exceeded ${TimeoutSeconds}s (results above are partial)."
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
    Remove-Item $gdbScript, $emulatorOut, $emulatorErr `
        -Force -ErrorAction SilentlyContinue
}
