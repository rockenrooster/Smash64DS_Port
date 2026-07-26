[CmdletBinding()]
param(
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [int]$GdbPort = 4613,
    [int]$RunnerSlot = -1,
    [string]$Build = 'build-tick-hud-buckets',
    [switch]$NoBuild,
    # Requires a ROM built NDS_TASK68_FALLBACK_CENSUS=1. Off by default because
    # that flag adds BSS and this ROM's pacing is cache-placement sensitive, so a
    # census build is not comparable to an ordinary tick-HUD baseline.
    [switch]$FallbackCensus,
    [ValidateRange(1,512)][int]$Samples = 32,
    [ValidateRange(1,1000000)][int]$StartFrame = 438,
    [ValidateRange(30,3600)][int]$TimeoutSeconds = 900,
    [string]$JsonOut = ''
)

# Reads the eleven NDS_TICK_HUD buckets
# (ALL/FTR/STG/BG/AUD/HUD/SRC/MISC/OTHR/WAIT/WORK) straight out of
# gNdsTickHudBuckets over GDB, one sample per presented battle iteration, so the
# HUD instrument can be recorded instead of photographed.
#
# Task 66 appended WAIT and WORK. ALL is VBlank-quantized wall time and so
# cannot show a saving smaller than one 560,190-tick period; WORK = ALL - WAIT
# is not quantized and is the series to search against. PROJECT_GOAL.md gates
# the milestone on WORK's P95 (<= 1.12M), which is why it is sampled as its own
# series rather than derived from the ALL and WAIT percentiles.
#
# The renderer benchmark path cannot do this: it asserts TICK_HUD=0 and
# SHIP_TELEMETRY=1 because the profile counters are a different instrument. The
# tick HUD is the only one that reports the whole loop partitioned into named
# buckets, and it is the instrument the retail device columns were read
# from, so a like-for-like emulator comparison has to come from here.
#
# Ticks are guest cpuGetTiming() deltas, so they do not depend on how fast the
# host runs. They DO depend on which melonDS is running: the repo fork models
# ARMv5 icache/dcache, stock melonDS does not. See emulators/README.md.

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'lib\melonds.ps1')
. (Join-Path $PSScriptRoot 'lib\build-output.ps1')

$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$target = 'smash64ds-battle-playable-tickhud-hwtri'
$bucketNames = @('ALL', 'FTR', 'STG', 'BG', 'AUD', 'HUD', 'SRC', 'MISC', 'OTHR',
                 'WAIT', 'WORK')
# Must match enum NDSTickHudNativeOwnerFallbackReason in include/nds/nds_startup.h.
$fallbackReasons = if ($FallbackCensus) {
    @('inputs', 'contract', 'postGx', 'begin') } else { @() }

$context = Initialize-MelonDSVerifierContext `
    -Root $root -MelonDS $MelonDS -RunnerSlot $RunnerSlot `
    -GdbPort $GdbPort -GdbPortExplicit:$PSBoundParameters.ContainsKey('GdbPort') `
    -NoBuild:$NoBuild
$rom = Resolve-Smash64DSBuildOutput `
    -Root $root -Target $target -Build $Build -Extension '.nds'
$elf = Resolve-Smash64DSBuildOutput `
    -Root $root -Target $target -Build $Build -Extension '.elf'
$temp = Get-MelonDSVerifierTempDir -Root $root -RunnerSlot $RunnerSlot
$gdbScript = Join-Path $temp 'tick-hud-buckets.gdb'
$gdbOut = Join-Path $temp 'tick-hud-buckets.gdb.out'
$gdbErr = Join-Path $temp 'tick-hud-buckets.gdb.err'
$emulatorOut = Join-Path $temp 'tick-hud-buckets.melonds.out'
$emulatorErr = Join-Path $temp 'tick-hud-buckets.melonds.err'
$configState = $null
$emulator = $null

try {
    if (-not $NoBuild) {
        if (-not $env:DEVKITPRO) { $env:DEVKITPRO = 'C:/devkitPro' }
        if (-not $env:DEVKITARM) { $env:DEVKITARM = 'C:/devkitPro/devkitARM' }
        make -C $root "TARGET=$target" "BUILD=$Build" -j16
        if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    }
    foreach ($path in @($rom, $elf, $Gdb)) {
        if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
            throw "Required tick-HUD sample file is missing: $path"
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

    # One stop per presented iteration from boot, gated host-side until
    # StartFrame; this matches how the renderer benchmark reaches its own start
    # frame, and keeps every sample on a real settled combat frame.
    # Both loop bounds and the printf arity come off $bucketNames so adding a
    # bucket is a one-line change. Task 66 added two and the second hardcoded
    # bound was missed, which silently dropped WAIT and WORK from the table
    # while every other check still passed.
    # Task 67: the native-owner fallback counters ride along on the same stop.
    # They are cumulative, so the per-frame count is a difference between
    # consecutive samples -- which is the point, because it lines the fallback
    # up against the very frame whose WORK spiked instead of leaving the two to
    # be correlated across separate runs.
    #
    # Only the total rides per frame. Reading the four per-reason counters here
    # too meant five extra GDB round-trips on every stop, which stretched the
    # stop far enough that the game's own frame pacing skipped and repeated
    # presented frames -- the sampler's uniqueness check caught it. The reason
    # breakdown is a run-level question, so it is read once at the end instead.
    $fallbackFields = if ($FallbackCensus) {
        @('gNdsTickHudNativeOwnerFallbackCount') } else { @() }
    $sampleFields = ((0..($bucketNames.Count - 1) |
        ForEach-Object { "gNdsTickHudBuckets[$_]" }) + $fallbackFields) -join ', '
    $sampleColumnCount = $bucketNames.Count + $fallbackFields.Count
    $tickHudFormat = (, '%u' * ($sampleColumnCount + 1)) -join ','
    [System.IO.File]::WriteAllLines($gdbScript, @(
        'set pagination off',
        'set confirm off',
        'set remotetimeout 30',
        "target remote 127.0.0.1:$($context.GdbPort)",
        'set $tick_samples = 0',
        'break ndsBattlePlayableFrameCompleteMarker',
        'commands',
        'silent',
        "if gNdsBattlePlayablePacingPresentedFrames < $StartFrame",
        'continue',
        'end',
        ("printf `"TICKHUD=$tickHudFormat\n`", " +
            "gNdsBattlePlayablePacingPresentedFrames, $sampleFields"),
        'set $tick_samples = $tick_samples + 1',
        ('if $tick_samples < {0}' -f $Samples),
        'continue',
        'end',
        'end',
        'continue',
        ('printf "TICKVBI=%u,%u,%u,%u,%u\n", ' +
            'gNdsBattlePlayablePacingPresentIntervalBucket[2], ' +
            'gNdsBattlePlayablePacingPresentIntervalBucket[3], ' +
            'gNdsBattlePlayablePacingPresentIntervalBucket[4], ' +
            'gNdsBattlePlayablePacingPresentIntervalBucket[5], ' +
            'gNdsBattlePlayablePacingPresentIntervalMax'),
        'printf "TICKSLIP=%u\n", gNdsBattlePlayablePacingCadenceViolationCount',
        $(if ($FallbackCensus) {
            "printf `"TICKFB=$((, '%u' * $fallbackReasons.Count) -join ',')\n`", " +
                ((0..($fallbackReasons.Count - 1) | ForEach-Object {
                    "gNdsTickHudNativeOwnerFallbackByReason[$_]" }) -join ', ')
        }),
        'detach'))
    $gdbProcess = Start-Process -FilePath $Gdb `
        -ArgumentList @('-q', '-batch', '-x', $gdbScript, $elf) `
        -WorkingDirectory $root `
        -RedirectStandardOutput $gdbOut `
        -RedirectStandardError $gdbErr `
        -PassThru
    if (-not $gdbProcess.WaitForExit($TimeoutSeconds * 1000)) {
        Stop-Process -Id $gdbProcess.Id -Force
        throw "Tick-HUD GDB run exceeded ${TimeoutSeconds}s before $Samples samples."
    }
    if ($gdbProcess.ExitCode -ne 0) {
        throw "Tick-HUD GDB run failed: $(Get-Content $gdbErr -Raw)"
    }

    $output = Get-Content $gdbOut -Raw
    $rows = @([regex]::Matches($output,
        "TICKHUD=([0-9]+(?:,[0-9]+){$sampleColumnCount})") | ForEach-Object {
            , [uint64[]]($_.Groups[1].Value -split ',')
        })
    if ($rows.Count -ne $Samples) {
        throw ("Tick-HUD run produced $($rows.Count) of $Samples samples. " +
            "GDB output:`n$output")
    }
    $frames = @($rows | ForEach-Object { $_[0] })
    if (($frames | Select-Object -Unique).Count -ne $rows.Count) {
        throw "Tick-HUD samples repeated a presented frame: $($frames -join ',')"
    }
    if ([uint64]$frames[0] -lt [uint64]$StartFrame) {
        throw "Tick-HUD sampling began at frame $($frames[0]), before $StartFrame."
    }

    $stats = @(0..($bucketNames.Count - 1) | ForEach-Object {
        $bucket = $_
        $values = @($rows | ForEach-Object { $_[$bucket + 1] })
        $sorted = @($values | Sort-Object)
        [PSCustomObject]@{
            bucket = $bucketNames[$bucket]
            mean = [uint64][Math]::Round((
                $values | Measure-Object -Average).Average, 0)
            p50 = [uint64]$sorted[[int][Math]::Floor(($sorted.Count - 1) * 0.50)]
            p95 = [uint64]$sorted[[int][Math]::Floor(($sorted.Count - 1) * 0.95)]
            min = [uint64]($values | Measure-Object -Minimum).Minimum
            max = [uint64]($values | Measure-Object -Maximum).Maximum
        }
    })

    # WORK-H: work with the tick HUD's own console render taken back out.
    #
    # This ROM exists to be measured, and the measuring costs something: the
    # text HUD redraws about twice a second and each redraw is worth a few
    # hundred thousand ticks, which is why HUD's spread is over 300x. That
    # barely moves a mean but it lands squarely on a P95 -- and P95 is the
    # statistic PROJECT_GOAL.md gates the milestone on. Subtracting per sample
    # (not subtracting one percentile from another) gives the P95 the published
    # profile-0 ROM would have, since it carries no tick HUD at all.
    $workIndex = [array]::IndexOf($bucketNames, 'WORK') + 1
    $hudIndex = [array]::IndexOf($bucketNames, 'HUD') + 1
    $workNoHud = @($rows | ForEach-Object {
        [uint64]$_[$workIndex] - [uint64]$_[$hudIndex] })
    $sortedWorkNoHud = @($workNoHud | Sort-Object)
    $stats += [PSCustomObject]@{
        bucket = 'WORK-H'
        mean = [uint64][Math]::Round((
            $workNoHud | Measure-Object -Average).Average, 0)
        p50 = [uint64]$sortedWorkNoHud[
            [int][Math]::Floor(($sortedWorkNoHud.Count - 1) * 0.50)]
        p95 = [uint64]$sortedWorkNoHud[
            [int][Math]::Floor(($sortedWorkNoHud.Count - 1) * 0.95)]
        min = [uint64]($workNoHud | Measure-Object -Minimum).Minimum
        max = [uint64]($workNoHud | Measure-Object -Maximum).Maximum
    }

    # Task 67: fallbacks per frame, and how the frames that took one compare to
    # the frames that did not. If the P95 really is the renderer dropping out of
    # its native owner, the two medians separate here and nowhere else.
    $fbBase = $bucketNames.Count + 1
    $workCol = [array]::IndexOf($bucketNames, 'WORK') + 1
    $hudCol = [array]::IndexOf($bucketNames, 'HUD') + 1
    $fbPerFrame = @(if ($FallbackCensus) {
        for ($i = 1; $i -lt $rows.Count; $i++) {
            [PSCustomObject]@{
                frame = $rows[$i][0]
                total = [uint64]$rows[$i][$fbBase] - [uint64]$rows[$i - 1][$fbBase]
                workH = [uint64]$rows[$i][$workCol] - [uint64]$rows[$i][$hudCol]
            }
        }
    })
    $fbFrames = @($fbPerFrame | Where-Object { $_.total -gt 0 })
    $cleanFrames = @($fbPerFrame | Where-Object { $_.total -eq 0 })

    # ALL is measured wall ticks for the whole iteration, not a sum of the
    # others; OTHR is defined as the ALL remainder, and WAIT/WORK are two more
    # views of ALL rather than additional named work. All four are therefore
    # excluded from the named share, which exists so a future run can tell "the
    # loop got slower" from "attribution drifted".
    $meanAll = ($stats | Where-Object { $_.bucket -eq 'ALL' }).mean
    $meanNamed = (($stats | Where-Object {
        $_.bucket -notin @('ALL', 'OTHR', 'WAIT', 'WORK', 'WORK-H') } |
        Measure-Object -Property mean -Sum).Sum)

    $vbiMatch = [regex]::Match($output, 'TICKVBI=([0-9]+(?:,[0-9]+){4})')
    $slipMatch = [regex]::Match($output, 'TICKSLIP=([0-9]+)')
    $vbi = if ($vbiMatch.Success) {
        [uint64[]]($vbiMatch.Groups[1].Value -split ',')
    } else { @(0, 0, 0, 0, 0) }
    $vbiTotal = [uint64]($vbi[0] + $vbi[1] + $vbi[2] + $vbi[3])

    $melonVersion = (Get-Item -LiteralPath $context.MelonDSPath).VersionInfo.FileVersion
    $result = [PSCustomObject]@{
        target = $target
        rom = $rom
        romSha256 = (Get-FileHash -LiteralPath $rom -Algorithm SHA256).Hash
        melonDS = $context.MelonDSPath
        melonDSSha256 = (Get-FileHash -LiteralPath $context.MelonDSPath `
            -Algorithm SHA256).Hash
        melonDSVersion = $melonVersion
        gitShort = (git -C $root rev-parse --short HEAD)
        samples = $rows.Count
        startFrame = [uint64]$frames[0]
        endFrame = [uint64]$frames[-1]
        buckets = $stats
        # The per-frame series, not just its order statistics. A P95 says how
        # bad the bad frames are; only the series says whether they are periodic
        # (a scheduled refill or a cart read) or event-driven (a hit, a KO).
        # Those want opposite fixes, and the summary cannot tell them apart.
        bucketNames = $bucketNames
        rows = $rows
        meanAll = $meanAll
        meanNamed = $meanNamed
        vbi2 = $vbi[0]
        vbi3 = $vbi[1]
        vbi4 = $vbi[2]
        vbi5plus = $vbi[3]
        vbiMax = $vbi[4]
        vbiTotal = $vbiTotal
        cadenceViolations = if ($slipMatch.Success) {
            [uint64]$slipMatch.Groups[1].Value } else { 0 }
        capturedUtc = (Get-Date).ToUniversalTime().ToString('o')
    }

    Write-Output ("Tick-HUD buckets: target=$target samples=$($rows.Count) " +
        "frames=$($frames[0])..$($frames[-1]) melonDS=$melonVersion " +
        "sha=$($result.melonDSSha256.Substring(0,16)) git=$($result.gitShort)")
    # P50/P95 lead because they are the decision basis in docs/VERIFYING.md and
    # they survive the spikes (a text redraw or a respawn) that make a mean of a
    # few hundred frames unrepresentative. spread = p95/p50 names exactly which
    # buckets those are: anything above ~2 should not be compared by mean.
    # %ALL is p50-relative and deliberately does not sum to 100 - percentiles do
    # not add. The additive identity is the mean line below.
    $p50All = ($stats | Where-Object { $_.bucket -eq 'ALL' }).p50
    if ($Samples -lt 40) {
        # With n samples the p95 index resolves to 1/n, so a short run reports
        # something closer to p(100-100/n) and can miss the burst frames
        # entirely - which is exactly what makes AUD/HUD look tame.
        Write-Warning ("p95 resolution is 1/$Samples here; use -Samples 40 or " +
            'more before treating p95 or spread as a decision input.')
    }
    $stats | Format-Table `
        @{n='bucket';e={$_.bucket};w=6}, `
        @{n='p50';e={'{0,10:N0}' -f $_.p50}}, `
        @{n='p95';e={'{0,10:N0}' -f $_.p95}}, `
        @{n='spread';e={'{0,6:N2}' -f ($_.p95 / [double][Math]::Max(1, $_.p50))}}, `
        @{n='mean';e={'{0,10:N0}' -f $_.mean}}, `
        @{n='min';e={'{0,10:N0}' -f $_.min}}, `
        @{n='max';e={'{0,10:N0}' -f $_.max}}, `
        @{n='%ALLp50';e={'{0,7:N1}' -f (100.0 * $_.p50 / $p50All)}} -AutoSize
    Write-Output (("named={0:N0} ({1:N1}% of ALL)  VBI 2:{2} 3:{3} 4:{4} 5+:{5} " +
        "max:{6} total:{7}  slips={8}") -f
        $meanNamed, (100.0 * $meanNamed / $meanAll),
        $vbi[0], $vbi[1], $vbi[2], $vbi[3], $vbi[4], $vbiTotal,
        $result.cadenceViolations)

    if ($FallbackCensus) {
    $fbMatch = [regex]::Match($output,
        "TICKFB=([0-9]+(?:,[0-9]+){$($fallbackReasons.Count - 1)})")
    $fbTotals = if ($fbMatch.Success) {
        [uint64[]]($fbMatch.Groups[1].Value -split ',')
    } else { @(0) * $fallbackReasons.Count }
    $fbByReason = @(0..($fallbackReasons.Count - 1) | ForEach-Object {
        '{0}:{1}' -f $fallbackReasons[$_], $fbTotals[$_]
    })
    Write-Output (("native-owner fallback: {0} of {1} frames took one  [{2}]") -f
        $fbFrames.Count, $fbPerFrame.Count, ($fbByReason -join ' '))
    if (($fbFrames.Count -gt 0) -and ($cleanFrames.Count -gt 0)) {
        $fbMed = ((@($fbFrames.workH | Sort-Object))[
            [int][Math]::Floor(($fbFrames.Count - 1) * 0.5)])
        $clMed = ((@($cleanFrames.workH | Sort-Object))[
            [int][Math]::Floor(($cleanFrames.Count - 1) * 0.5)])
        Write-Output (("  WORK-H median: fallback {0:N0} vs clean {1:N0} " +
            "({2:N2}x)") -f $fbMed, $clMed, ($fbMed / [double]$clMed))
    }
    }

    if ($JsonOut) {
        $jsonDir = Split-Path -Parent $JsonOut
        if ($jsonDir) { New-Item -ItemType Directory -Force -Path $jsonDir | Out-Null }
        $result | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $JsonOut
        Write-Output "Wrote $JsonOut"
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
