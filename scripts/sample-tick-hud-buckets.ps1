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
    # Read the ROM's own 128-entry sample ring in a single stop instead of
    # stopping GDB once per presented frame. ndsPlatformTickHudSample already
    # records every bucket of every presented iteration into
    # sBattleTickHudRing -- the per-frame stop was collecting data the ROM was
    # keeping anyway. One stop cannot perturb pacing 128 times, and the ring is
    # indexed by iteration rather than by the presented-frame counter, so it is
    # not exposed to whatever makes that counter disagree with the marker.
    [switch]$RingDump,
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
    @('calls', 'eligible', 'animLock', 'selected', 'displayList',
      'materialCount', 'validate', 'matrices', 'materialPrep',
      'inputs', 'contract', 'postGx', 'begin') } else { @() }

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
    $fallbackFields = if ($FallbackCensus -and (-not $RingDump)) {
        @('gNdsTickHudNativeOwnerFallbackCount') } else { @() }
    $sampleFields = ((0..($bucketNames.Count - 1) |
        ForEach-Object { "gNdsTickHudBuckets[$_]" }) + $fallbackFields) -join ', '
    $sampleColumnCount = $bucketNames.Count + $fallbackFields.Count
    $tickHudFormat = (, '%u' * ($sampleColumnCount + 1)) -join ','
    $ringPath = Join-Path $temp 'tick-hud-ring.bin'
    $fbRingPath = Join-Path $temp 'tick-hud-fallback-ring.bin'
    $ringWindow = 128
    $ringBytes = $bucketNames.Count * $ringWindow * 4
    $ringEndFrame = $StartFrame + $Samples
    $gdbLines = if ($RingDump) {
        @(
        'set pagination off',
        'set confirm off',
        'set remotetimeout 30',
        "target remote 127.0.0.1:$($context.GdbPort)",
        # The fallback counters run from boot, so a single read at the end would
        # charge the census window with every fallback taken during boot, the
        # menu and the seeding presents. Stop once at the start frame to take a
        # baseline and difference it. Two stops, not 128.
        $(if ($FallbackCensus) { 'break ndsBattlePlayableFrameCompleteMarker' }),
        $(if ($FallbackCensus) { 'commands' }),
        $(if ($FallbackCensus) { 'silent' }),
        $(if ($FallbackCensus) {
            "if gNdsBattlePlayablePacingPresentedFrames < $StartFrame" }),
        $(if ($FallbackCensus) { 'continue' }),
        $(if ($FallbackCensus) { 'end' }),
        $(if ($FallbackCensus) { 'end' }),
        $(if ($FallbackCensus) { 'continue' }),
        $(if ($FallbackCensus) {
            "printf `"TICKFB0=$((, '%u' * $fallbackReasons.Count) -join ',')\n`", " +
                ((0..($fallbackReasons.Count - 1) | ForEach-Object {
                    "gNdsTickHudNativeOwnerFallbackByReason[$_]" }) -join ', ')
        }),
        $(if ($FallbackCensus) { 'delete' }),
        'break ndsBattlePlayableFrameCompleteMarker',
        'commands',
        'silent',
        "if gNdsBattlePlayablePacingPresentedFrames < $ringEndFrame",
        'continue',
        'end',
        'end',
        'continue',
        ('printf "TICKRING=%u,%u,%u\n", ' +
            'sBattleTickHudRingHead, sBattleTickHudRingCount, ' +
            'gNdsBattlePlayablePacingPresentedFrames'),
        # dump binary memory splits its arguments on whitespace, so the bounds
        # have to be single tokens -- a cast like "(char *)&ring" parses as two
        # arguments and fails. Index one past the last bucket row for the end
        # bound, which is the same form the Task 34 stage-stream dump uses.
        ("dump binary memory $ringPath &sBattleTickHudRing[0][0] " +
            "&sBattleTickHudRing[$($bucketNames.Count)][0]"),
        $(if ($FallbackCensus) {
            "dump binary memory $fbRingPath &sBattleTickHudFallbackRing[0] " +
                "&sBattleTickHudFallbackRing[$ringWindow]" }),
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
        'detach')
    } else {
        @(
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
        'detach')
    }
    # Drop the nulls the conditional lines leave behind. A null writes a blank
    # line, and a blank line in a GDB script re-executes the previous command --
    # which next to a 'continue' would silently run the emulator on past the
    # frame the dump was supposed to be taken at.
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
        throw "Tick-HUD GDB run exceeded ${TimeoutSeconds}s before $Samples samples."
    }
    if ($gdbProcess.ExitCode -ne 0) {
        throw "Tick-HUD GDB run failed: $(Get-Content $gdbErr -Raw)"
    }

    $output = Get-Content $gdbOut -Raw
    $rows = if ($RingDump) {
        $ringMatch = [regex]::Match($output, 'TICKRING=([0-9]+),([0-9]+),([0-9]+)')
        if (-not $ringMatch.Success) {
            throw "Tick-HUD ring dump produced no TICKRING line. GDB output:`n$output"
        }
        $ringHead = [int]$ringMatch.Groups[1].Value
        $ringCount = [int]$ringMatch.Groups[2].Value
        $ringFrame = [uint64]$ringMatch.Groups[3].Value
        if (-not (Test-Path -LiteralPath $ringPath -PathType Leaf)) {
            throw "Tick-HUD ring dump wrote no file at $ringPath."
        }
        $raw = [System.IO.File]::ReadAllBytes($ringPath)
        if ($raw.Length -ne $ringBytes) {
            throw ("Tick-HUD ring dump is $($raw.Length) bytes, expected " +
                "$ringBytes ($($bucketNames.Count) buckets x $ringWindow).")
        }
        if ($ringCount -lt $Samples) {
            throw ("Tick-HUD ring holds $ringCount iterations, fewer than the " +
                "$Samples requested. Raise -StartFrame or lower -Samples.")
        }
        # Oldest-first walk. Once the ring has wrapped the oldest entry is the
        # one the head is about to overwrite; before that it has not moved off
        # zero yet.
        $ringStart = if ($ringCount -lt $ringWindow) { 0 } else { $ringHead }
        $skip = $ringCount - $Samples
        # Task 70: the per-frame fallback deltas share the ring index, so they
        # append as one more column and every frame carries its own count.
        $fbRaw = if ($FallbackCensus) {
            if (-not (Test-Path -LiteralPath $fbRingPath -PathType Leaf)) {
                throw ("Fallback ring dump wrote no file at $fbRingPath. The " +
                    'ROM must be built NDS_TASK68_FALLBACK_CENSUS=1.')
            }
            $bytes = [System.IO.File]::ReadAllBytes($fbRingPath)
            if ($bytes.Length -ne ($ringWindow * 4)) {
                throw ("Fallback ring dump is $($bytes.Length) bytes, expected " +
                    "$($ringWindow * 4).")
            }
            $bytes
        } else { $null }
        @(0..($Samples - 1) | ForEach-Object {
            $slot = ($ringStart + $skip + $_) % $ringWindow
            # The label is derived from the presented-frame counter read at the
            # stop, and is only a label: in ring mode the identity of a sample
            # is its position in the ring, which advances exactly once per call
            # to ndsPlatformTickHudSample and therefore exactly once per
            # finalized iteration.
            $row = New-Object 'System.Collections.Generic.List[uint64]'
            $row.Add([uint64]($ringFrame - ($Samples - 1 - $_)))
            for ($b = 0; $b -lt $bucketNames.Count; $b++) {
                $row.Add([BitConverter]::ToUInt32($raw,
                    ((($b * $ringWindow) + $slot) * 4)))
            }
            if ($null -ne $fbRaw) {
                $row.Add([BitConverter]::ToUInt32($fbRaw, ($slot * 4)))
            }
            , [uint64[]]$row.ToArray()
        })
    } else {
        @([regex]::Matches($output,
            "TICKHUD=([0-9]+(?:,[0-9]+){$sampleColumnCount})") | ForEach-Object {
                , [uint64[]]($_.Groups[1].Value -split ',')
            })
    }
    if ($rows.Count -ne $Samples) {
        throw ("Tick-HUD run produced $($rows.Count) of $Samples samples. " +
            "GDB output:`n$output")
    }
    $frames = @($rows | ForEach-Object { $_[0] })
    if (($frames | Select-Object -Unique).Count -ne $rows.Count) {
        # The guest cannot produce this. ndsBattlePlayablePresentFrame increments
        # the counter and ndsBattlePlayableFinalizePresentedIteration calls the
        # marker immediately after, with no branch between them in the settled
        # loop -- so two marker hits at one counter value is not something the
        # game did, it is something the read saw. Report enough to tell the two
        # candidate causes apart instead of just failing:
        #   identical payload -> a stale read (GDB reads the emulated memory
        #     array directly, so a dirty ARM9 dcache line still holding the new
        #     value reads back as the old one)
        #   differing payload -> the loop really did iterate twice, and the
        #     frame counter is not the per-iteration identity we assumed.
        $dupIndex = @(1..($rows.Count - 1) | Where-Object {
            $rows[$_][0] -eq $rows[$_ - 1][0] })
        $detail = @($dupIndex | Select-Object -First 4 | ForEach-Object {
            $prev = $rows[$_ - 1] -join ','
            $cur = $rows[$_] -join ','
            "  [{0}] {1}`n  [{2}] {3}  payload {4}" -f
                ($_ - 1), $prev, $_, $cur,
                $(if ($prev -eq $cur) { 'IDENTICAL (stale read)' }
                  else { 'DIFFERS (real second iteration)' })
        })
        throw ("Tick-HUD samples repeated a presented frame " +
            "($($dupIndex.Count) of $($rows.Count)):`n$($frames -join ',')`n" +
            "columns: frame,$($bucketNames -join ',')" +
            "$(if ($fallbackFields) { ',fbTotal' })`n" +
            ($detail -join "`n"))
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
    # Ring mode rings the delta itself, so the column is already per-frame and
    # every sample counts. The per-stop column is cumulative and has to be
    # differenced, which costs the first sample.
    $fbPerFrame = @(if ($FallbackCensus -and $RingDump) {
        for ($i = 0; $i -lt $rows.Count; $i++) {
            [PSCustomObject]@{
                frame = $rows[$i][0]
                total = [uint64]$rows[$i][$fbBase]
                workH = [uint64]$rows[$i][$workCol] - [uint64]$rows[$i][$hudCol]
            }
        }
    } elseif ($FallbackCensus) {
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
        ringDump = [bool]$RingDump
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
    # Ring mode brackets the window with a baseline read, so the reported
    # numbers are fallbacks taken during the sampled frames rather than since
    # reset -- boot, the menu and the seeding presents no longer count.
    $fb0Match = [regex]::Match($output,
        "TICKFB0=([0-9]+(?:,[0-9]+){$($fallbackReasons.Count - 1)})")
    $fbBaseline = if ($fb0Match.Success) {
        [uint64[]]($fb0Match.Groups[1].Value -split ',')
    } else { @([uint64]0) * $fallbackReasons.Count }
    $fbWindow = @(0..($fallbackReasons.Count - 1) | ForEach-Object {
        [uint64]$fbTotals[$_] - [uint64]$fbBaseline[$_] })
    $fbByReason = @(0..($fallbackReasons.Count - 1) | ForEach-Object {
        '{0}:{1}' -f $fallbackReasons[$_], $fbWindow[$_]
    })
    if ($RingDump) {
        # calls and eligible lead the enum as denominators, so they are excluded
        # from the fallback total and reported as the rate's base instead --
        # summing them in would report 522 fallbacks out of 256 draws.
        $fbDenominators = 2
        $fbCalls = [double]$fbWindow[0]
        $fbReasonSum = (($fbWindow | Select-Object -Skip $fbDenominators |
            Measure-Object -Sum).Sum)
        Write-Output (("native-owner: {0} draws over {1} frames, {2} eligible, " +
            "{3} fell back ({4:N1}%)  [{5}]") -f
            $fbWindow[0], $rows.Count, $fbWindow[1], $fbReasonSum,
            (100.0 * $fbReasonSum / [Math]::Max(1.0, $fbCalls)),
            (($fbByReason | Select-Object -Skip $fbDenominators) -join ' '))
        Write-Output ("  frames with a fallback: {0} of {1}" -f
            $fbFrames.Count, $fbPerFrame.Count)
    } else {
        Write-Output (("native-owner fallback: {0} of {1} frames took one  [{2}]") -f
            $fbFrames.Count, $fbPerFrame.Count, ($fbByReason -join ' '))
    }
    if (($fbFrames.Count -gt 0) -and ($cleanFrames.Count -gt 0)) {
        $fbMed = ((@($fbFrames.workH | Sort-Object))[
            [int][Math]::Floor(($fbFrames.Count - 1) * 0.5)])
        $clMed = ((@($cleanFrames.workH | Sort-Object))[
            [int][Math]::Floor(($cleanFrames.Count - 1) * 0.5)])
        Write-Output (("  WORK-H median: fallback {0:N0} vs clean {1:N0} " +
            "({2:N2}x)") -f $fbMed, $clMed, ($fbMed / [double]$clMed))
        # A median over a handful of frames is a weak basis for "the fallback
        # frames are the expensive frames". What settles it is whether the two
        # populations separate: how many clean frames reach the cheapest
        # fallback frame. Zero overlap is the claim; anything else is not.
        $fbMin = ($fbFrames.workH | Measure-Object -Minimum).Minimum
        $cleanAbove = @($cleanFrames | Where-Object { $_.workH -ge $fbMin })
        $cleanSorted = @($cleanFrames.workH | Sort-Object)
        Write-Output (("  separation: cheapest fallback frame {0:N0}; " +
            "{1} of {2} clean frames reach it; clean P95 {3:N0}") -f
            $fbMin, $cleanAbove.Count, $cleanFrames.Count,
            $cleanSorted[[int][Math]::Floor(($cleanSorted.Count - 1) * 0.95)])
        Write-Output ("  fallback frames: " + (($fbFrames |
            ForEach-Object { '{0}({1}):{2:N0}' -f
                $_.frame, $_.total, $_.workH }) -join ' '))
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
    Remove-Item $gdbScript, $gdbOut, $gdbErr, $emulatorOut, $emulatorErr, `
        (Join-Path $temp 'tick-hud-ring.bin'), `
        (Join-Path $temp 'tick-hud-fallback-ring.bin') `
        -Force -ErrorAction SilentlyContinue
}
