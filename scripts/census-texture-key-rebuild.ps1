[CmdletBinding()]
param(
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [int]$GdbPort = 4619,
    [int]$RunnerSlot = -1,
    [string]$Build = 'build-task93-texkey',
    [switch]$NoBuild,
    [ValidateRange(1,1000000)][int]$StartFrame = 439,
    [ValidateRange(2,600)][int]$WindowFrames = 30,
    [ValidateRange(30,3600)][int]$TimeoutSeconds = 900,
    [string]$JsonOut = ''
)

# Task 90. Sizes NDS_RENDERER_HW_LIGHT_SHADE_CACHE_COUNT from the measured
# working set instead of from a hit rate, and is the re-check that constant's
# comment names when the fighter or stage set changes.
#
# A hit rate alone cannot size a cache: 12% misses is consistent both with a
# working set that barely overflows and with one far too large to ever hold. So
# the ROM records the actual request sequence and this script replays it against
# a FIFO of each candidate size. The right size is the smallest one whose miss
# count reaches the compulsory floor -- the number of distinct (diffuse,
# ambient) pairs, which every cache must build once no matter how large.
#
# A miss is expensive here: it rebuilds a 128-entry table, three channels per
# entry, which is why 6 misses per frame were worth ~17,000 ticks.
#
# Two stops, WindowFrames apart, and a difference. A single stop would report
# cumulative totals since boot, which are dominated by the load and title
# frames rather than by steady battle.

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'lib\melonds.ps1')
. (Join-Path $PSScriptRoot 'lib\build-output.ps1')

$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$target = 'smash64ds-battle-playable-tickhud-hwtri'

$counters = @(
    'gNdsTask93BindCalls',
    'gNdsTask93PreflightCalls',
    'gNdsTask93ConsecutiveRepeat'
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
        make -C $root "TARGET=$target" "BUILD=$Build" NDS_TASK93_TEXKEY_CENSUS=1
        if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    }
    foreach ($path in @($rom, $elf, $Gdb)) {
        if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
            throw "Required light-shade LUT census file is missing: $path"
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
        "if gNdsBattlePlayablePacingPresentedFrames < $StartFrame",
        'continue',
        'end',
        'end',
        'continue'
    ) + (New-SampleCommands -Tag 'A' -Frame $StartFrame) + @(
        'continue'
    ) + (New-SampleCommands -Tag 'B' -Frame $endFrame) + @(
        'printf "LUTNEXT=%u\n", gNdsTask93KeyTraceNext',
        'printf "LUTDIFF="',
        'output gNdsTask93KeyTrace',
        'printf "\nLUTAMB="',
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
        -PassThru
    if (-not $gdbProcess.WaitForExit($TimeoutSeconds * 1000)) {
        Stop-Process -Id $gdbProcess.Id -Force
        throw "Light-shade LUT census exceeded ${TimeoutSeconds}s."
    }
    if ($gdbProcess.ExitCode -ne 0) {
        Get-Content $gdbErr -ErrorAction SilentlyContinue | Write-Host
        throw "Light-shade LUT census GDB run failed with exit code $($gdbProcess.ExitCode)."
    }

    $lines = Get-Content $gdbOut -ErrorAction SilentlyContinue
    $samples = @{}
    foreach ($tag in 'A', 'B') {
        $line = $lines | Where-Object { $_ -match "^SHADE=$tag," } | Select-Object -First 1
        if (-not $line) {
            Get-Content $gdbOut -ErrorAction SilentlyContinue | Write-Host
            throw "Light-shade LUT census produced no sample $tag."
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
        throw "Light-shade LUT census window is $frames frames; the two stops did not advance."
    }
    $delta = [ordered]@{}
    foreach ($c in $counters) {
        $delta[$c] = [int64]$samples['B'][$c] - [int64]$samples['A'][$c]
    }

    $perFrame = { param($v) [math]::Round($v / $frames, 1) }

    Write-Host ""
    Write-Host ("Task 93 E0 -- texture-key rebuild over $frames presented frames")
    Write-Host ("(frames {0} .. {1})" -f $samples['A'].frame, $samples['B'].frame)
    Write-Host ""
    Write-Host "counter                       total     per frame"
    Write-Host "--------------------------  ---------  ----------"
    foreach ($c in $counters) {
        Write-Host ("{0,-26}  {1,9:N0}  {2,10:N1}" -f `
            ($c -replace '^gNdsTask90', ''), $delta[$c], (& $perFrame $delta[$c]))
    }
    Write-Host ""
    if ($delta['gNdsTask93BindCalls'] -gt 0) {
        Write-Host ("consecutive repeats:  {0:N0} of {1:N0} bind calls = {2:P1}" -f `
            $delta['gNdsTask93ConsecutiveRepeat'], $delta['gNdsTask93BindCalls'],
            ($delta['gNdsTask93ConsecutiveRepeat'] / $delta['gNdsTask93BindCalls']))
    }
    Write-Host ""

    # Replay the captured request trace against a FIFO cache of each candidate
    # size. This is what decides the size instead of a guess: the answer is the
    # smallest cache whose miss count reaches the compulsory floor (the number
    # of distinct pairs, which no cache can avoid paying once).
    $trace = @()
    $nextLine = $lines | Where-Object { $_ -match '^LUTNEXT=' } | Select-Object -First 1
    $diffLine = $lines | Where-Object { $_ -match '^LUTDIFF=' } | Select-Object -First 1
    $ambLine = $lines | Where-Object { $_ -match '^LUTAMB=' } | Select-Object -First 1
    if ($nextLine -and $diffLine -and $ambLine) {
        $parseArray = {
            param($text)
            , (($text -replace '^[A-Z]+=', '').Trim('{', '}', ' ') -split ',' |
                ForEach-Object { [uint32]($_.Trim()) })
        }
        $d = & $parseArray $diffLine
        $a = & $parseArray $ambLine
        $next = [int](($nextLine -replace '^LUTNEXT=', '').Trim())
        # gNdsTask93KeyTraceNext points at the slot the next request will use,
        # so the oldest captured request is there and the ring unrolls from it.
        for ($i = 0; $i -lt $d.Count; $i++) {
            $slot = ($next + $i) % $d.Count
            $trace += , @($d[$slot], $a[$slot])
        }
        $distinct = [System.Collections.Generic.HashSet[string]]::new()
        foreach ($p in $trace) { [void]$distinct.Add("$($p[0]):$($p[1])") }

        Write-Host ("LUT request trace: {0} requests, {1} distinct (diffuse, ambient) pairs" -f `
            $trace.Count, $distinct.Count)
        Write-Host ""
        Write-Host "cache size   misses   miss rate"
        Write-Host "----------  -------  ----------"
        foreach ($size in 4, 8, 16, 32) {
            $fifo = New-Object string[] $size
            $nextSlot = 0
            $misses = 0
            foreach ($p in $trace) {
                $key = "$($p[0]):$($p[1])"
                if ($fifo -notcontains $key) {
                    $misses++
                    $fifo[$nextSlot] = $key
                    $nextSlot = ($nextSlot + 1) % $size
                }
            }
            Write-Host ("{0,10}  {1,7}  {2,10:P1}" -f `
                $size, $misses, ($misses / $trace.Count))
        }
        Write-Host ("{0,10}  {1,7}  {2,10}" -f 'compulsory', $distinct.Count, '(floor)')
        Write-Host ""
    }

    if ($JsonOut) {
        $payload = [ordered]@{
            task = 'Task 93 E0 - texture key rebuild working set'
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
