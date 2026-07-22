[CmdletBinding()]
param(
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [int]$GdbPort = 4613,
    [int]$RunnerSlot = -1,
    [string]$Build = 'build-tick-hud-buckets',
    [switch]$NoBuild,
    [ValidateRange(1,512)][int]$Samples = 32,
    [ValidateRange(1,1000000)][int]$StartFrame = 438,
    [ValidateRange(30,3600)][int]$TimeoutSeconds = 900,
    [string]$JsonOut = ''
)

# Reads the nine NDS_TICK_HUD buckets (ALL/FTR/STG/BG/AUD/HUD/SRC/MISC/OTHR)
# straight out of gNdsTickHudBuckets over GDB, one sample per presented battle
# iteration, so the HUD instrument can be recorded instead of photographed.
#
# The renderer benchmark path cannot do this: it asserts TICK_HUD=0 and
# SHIP_TELEMETRY=1 because the profile counters are a different instrument. The
# tick HUD is the only one that reports the whole loop partitioned into nine
# named buckets, and it is the instrument the retail device columns were read
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
$bucketNames = @('ALL', 'FTR', 'STG', 'BG', 'AUD', 'HUD', 'SRC', 'MISC', 'OTHR')

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
    $sampleFields = (0..8 | ForEach-Object { "gNdsTickHudBuckets[$_]" }) -join ', '
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
        ('printf "TICKHUD=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", ' +
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
        'TICKHUD=([0-9]+(?:,[0-9]+){9})') | ForEach-Object {
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

    $stats = @(0..8 | ForEach-Object {
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
    # ALL is measured wall ticks for the whole iteration, not a sum of the other
    # eight; OTHR is defined as the ALL remainder. Report the named share so a
    # future run can tell "the loop got slower" from "attribution drifted".
    $meanAll = ($stats | Where-Object { $_.bucket -eq 'ALL' }).mean
    $meanNamed = (($stats | Where-Object {
        $_.bucket -notin @('ALL', 'OTHR') } | Measure-Object -Property mean -Sum).Sum)

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
