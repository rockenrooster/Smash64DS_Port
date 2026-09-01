[CmdletBinding()]
param(
    [string]$MelonDS = '',
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [int]$GdbPort = 4624,
    [int]$RunnerSlot = 6,
    [string]$Build = 'build-tick-hud-buckets',
    [string]$Target = 'smash64ds-battle-playable-tickhud-hwtri',
    [ValidateRange(0,1)][int]$Route = 1,
    [ValidateRange(1,1000000)][int]$StartFrame = 440,
    [ValidateRange(1,1000000)][int]$EndFrame = 2040,
    [ValidateRange(30,3600)][int]$TimeoutSeconds = 1500,
    [string]$JsonOut = ''
)

# G1 CENSUS -- does the stage texture-site memo actually HIT, and does its
# 128-entry table hold the effect path's working set?
#
# One binary, two arms, selected by poking gNdsG1SiteCacheRoute before the
# window (board standing rule 7: this ROM's pacing is placement-sensitive and
# separately-linked A/B ROMs have confused two comparisons).
#
#   -Route 0  the shipped behaviour. gNdsG1SiteConsults MUST read 0. This is
#             the negative control, and it is a control that can be non-zero:
#             route 1 makes it non-zero in the same binary on the same frames.
#   -Route 1  mode 9 joins the enable list.
#
# OVERWRITES IS THE DECIDING NUMBER, NOT THE HIT RATE. The refuted
# (dl-pointer, bind-ordinal) memo took 471 hits on 10,336 consults (4.56%) with
# 7,517 evictions of 7,525 fills, against a working set estimated at ~175 keys.
# This table holds 128 and its overflow policy is a round-robin overwrite, so if
# the working set still overflows, the memo pays a probe walk in order to miss.
# gNdsG1SiteOverwrites counts only the case where a full table hands a slot from
# one site to a DIFFERENT site, which is capacity thrash and nothing else.
#
# gNdsG1SiteOccupancy is a lower bound on the working set, and it saturates at
# 128 by construction -- it cannot distinguish "fits exactly" from "far too
# large". Read it together with overwrites, never alone.
#
# Every value printed is a code-published global. No stack object is read.

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'lib\melonds.ps1')
. (Join-Path $PSScriptRoot 'lib\build-output.ps1')

$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$target = $Target

$counters = @(
    'gNdsG1SiteCacheRoute',
    'gNdsG1SiteConsults',
    'gNdsG1SiteHits',
    'gNdsG1SiteRemembers',
    'gNdsG1SiteOverwrites',
    'gNdsG1SiteOccupancy',
    'gNdsEffectRendererTextureReadyCount',
    'gNdsEffectRendererDObjDrawCount',
    'gNdsEffectRendererTriangleCount',
    # The mechanism claim. Tex ticks/list is what a 77% hit rate is supposed to
    # move; these two are the numerator and denominator behind the banked
    # 21.41% / 25,289-per-list phase figure, and unlike
    # gNdsRendererProfileTextureTicks (profile 2 only) they are guarded by
    # NDS_TICK_HUD, so they are live in the profile-0 measuring target.
    'gNdsEffectPhaseTexTicks',
    'gNdsEffectPhaseDLCount',
    'gNdsEffectPhaseExecTicks'
)

$context = Initialize-MelonDSVerifierContext `
    -Root $root -MelonDS $MelonDS -RunnerSlot $RunnerSlot `
    -GdbPort $GdbPort -GdbPortExplicit:$PSBoundParameters.ContainsKey('GdbPort') `
    -NoBuild
$rom = Resolve-Smash64DSBuildOutput `
    -Root $root -Target $target -Build $Build -Extension '.nds'
$elf = Resolve-Smash64DSBuildOutput `
    -Root $root -Target $target -Build $Build -Extension '.elf'
$temp = Get-MelonDSVerifierTempDir -Root $root -RunnerSlot $RunnerSlot
$gdbScript = Join-Path $temp 'g1-census.gdb'
$gdbOut = Join-Path $temp 'g1-census.gdb.out'
$gdbErr = Join-Path $temp 'g1-census.gdb.err'
$emulatorOut = Join-Path $temp 'g1-census.melonds.out'
$emulatorErr = Join-Path $temp 'g1-census.melonds.err'
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
        ("printf `"G1C=$Tag,%u,$format\n`", " +
            "gNdsBattlePlayablePacingPresentedFrames, " + ($counters -join ', '))
    )
}

try {
    foreach ($path in @($rom, $elf, $Gdb)) {
        if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
            throw "Required G1 census file is missing: $path"
        }
    }
    # A gdb batch aborts every command after the first absent symbol, so check
    # the whole read set here. --gc-sections collects a global whose only
    # consumer is a debugger; these all have in-ROM writers, and this is the
    # check that says so.
    $nm = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-nm.exe'
    $symbols = & $nm $elf | ForEach-Object { ($_ -split '\s+')[-1] }
    $missing = $counters | Where-Object { $symbols -notcontains $_ }
    if ($missing.Count -gt 0) {
        throw ("G1 census symbols absent from {0}: {1}" -f $elf, ($missing -join ', '))
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

    # Phase 0: stop at the first frame-complete marker and set the route there,
    # well before the memo is consulted, then confirm the poke landed.
    $gdbLines = @(
        'set pagination off',
        'set confirm off',
        'set print elements 0',
        'set print repeats 0',
        'set print pretty off',
        'set remotetimeout 30',
        "target remote 127.0.0.1:$($context.GdbPort)",
        'break ndsBattlePlayableFrameCompleteMarker',
        'continue',
        "set var gNdsG1SiteCacheRoute = $Route",
        'printf "G1ROUTE=%u\n", gNdsG1SiteCacheRoute'
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
        throw "G1 census exceeded ${TimeoutSeconds}s."
    }
    if ($gdbProcess.ExitCode -ne 0) {
        Get-Content $gdbErr -ErrorAction SilentlyContinue | Write-Host
        throw "G1 census GDB run failed with exit code $($gdbProcess.ExitCode)."
    }

    $lines = Get-Content $gdbOut -ErrorAction SilentlyContinue
    $routeLine = $lines | Where-Object { $_ -match '^G1ROUTE=' } | Select-Object -First 1
    if (-not $routeLine) {
        Get-Content $gdbOut -ErrorAction SilentlyContinue | Write-Host
        throw "G1 census never confirmed the route poke."
    }
    $routeSet = [int](($routeLine -replace '^G1ROUTE=', '').Trim())
    if ($routeSet -ne $Route) {
        throw "G1 census route poke did not land: asked $Route, read $routeSet."
    }

    $samples = @{}
    foreach ($tag in 'A', 'B') {
        $line = $lines | Where-Object { $_ -match "^G1C=$tag," } | Select-Object -First 1
        if (-not $line) {
            Get-Content $gdbOut -ErrorAction SilentlyContinue | Write-Host
            throw "G1 census produced no sample $tag."
        }
        $parts = ($line -replace "^G1C=$tag,", '') -split ','
        $s = [ordered]@{ frame = [uint32]$parts[0] }
        for ($i = 0; $i -lt $counters.Count; $i++) {
            $s[$counters[$i]] = [uint32]$parts[$i + 1]
        }
        $samples[$tag] = $s
    }

    $frames = [int]$samples['B'].frame - [int]$samples['A'].frame
    if ($frames -le 0) {
        throw "G1 census window is $frames frames; the two stops did not advance."
    }
    $delta = [ordered]@{}
    foreach ($c in $counters) {
        $delta[$c] = [int64]$samples['B'][$c] - [int64]$samples['A'][$c]
    }

    Write-Host ""
    Write-Host ("G1 census -- ROUTE $Route -- whole match, {0} presented frames (frames {1} .. {2})" -f `
        $frames, $samples['A'].frame, $samples['B'].frame)
    Write-Host ("ROM {0}" -f $rom)
    Write-Host ""
    Write-Host "name                                              A          B      delta"
    Write-Host "----------------------------------------  ---------  ---------  ---------"
    foreach ($c in $counters) {
        Write-Host ("{0,-40}  {1,9:N0}  {2,9:N0}  {3,9:N0}" -f `
            $c, $samples['A'][$c], $samples['B'][$c], $delta[$c])
    }
    Write-Host ""

    $consults = [int64]$delta['gNdsG1SiteConsults']
    $hits = [int64]$delta['gNdsG1SiteHits']
    $remembers = [int64]$delta['gNdsG1SiteRemembers']
    $overwrites = [int64]$delta['gNdsG1SiteOverwrites']
    if ($Route -eq 0) {
        if ($consults -eq 0) {
            Write-Host "NEGATIVE CONTROL PASSED: route 0 -> 0 consults (memo inert, as shipped)."
        } else {
            Write-Host ("NEGATIVE CONTROL FAILED: route 0 took {0:N0} consults." -f $consults)
        }
    } elseif ($consults -gt 0) {
        Write-Host ("hit rate        {0:N0} / {1:N0} = {2:P2}" -f $hits, $consults, ($hits / $consults))
        Write-Host ("overwrites      {0:N0} of {1:N0} stores = {2:P2}  (capacity thrash)" -f `
            $overwrites, $remembers, $(if ($remembers) { $overwrites / $remembers } else { 0 }))
        Write-Host ("occupancy       {0:N0} of 128 slots (lower bound on working set)" -f `
            [int64]$samples['B']['gNdsG1SiteOccupancy'])
    } else {
        Write-Host "ROUTE 1 TOOK ZERO CONSULTS -- the flip did not engage; do not read the rest."
    }
    # Tex ticks/list is the mechanism claim and is reported for BOTH routes, so
    # the two arms can be differenced directly. A high hit rate that leaves this
    # constant unmoved is a finding about where the cost actually sits, not a
    # measurement to discard.
    $lists = [int64]$delta['gNdsEffectPhaseDLCount']
    if ($lists -gt 0) {
        Write-Host ""
        Write-Host ("Tex   {0,12:N0} ticks over {1,6:N0} lists = {2,9:N0} /list" -f `
            [int64]$delta['gNdsEffectPhaseTexTicks'], $lists,
            ([int64]$delta['gNdsEffectPhaseTexTicks'] / $lists))
        Write-Host ("Exec  {0,12:N0} ticks over {1,6:N0} lists = {2,9:N0} /list" -f `
            [int64]$delta['gNdsEffectPhaseExecTicks'], $lists,
            ([int64]$delta['gNdsEffectPhaseExecTicks'] / $lists))
    } else {
        Write-Host "gNdsEffectPhaseDLCount delta is 0 -- no per-list constant available."
    }
    Write-Host ""

    if ($JsonOut) {
        $payload = [ordered]@{
            probe = 'G1 stage texture-site memo census'
            route = $Route
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
