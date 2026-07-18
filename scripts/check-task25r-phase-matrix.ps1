param(
    [Parameter(Mandatory = $true)][string[]]$PhaseJson,
    [Parameter(Mandatory = $true)][string]$Profile0Json,
    [Parameter(Mandatory = $true)][string]$Profile1Screenshot,
    [Parameter(Mandatory = $true)][string]$Profile0Screenshot,
    [Parameter(Mandatory = $true)][string]$OutputJson,
    [Parameter(Mandatory = $true)][string]$OutputMarkdown,
    [string]$IdentityDirectory = '',
    [switch]$RequireStable30
)

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$deadlineTicks = [int64]1120380
$phaseOrder = @(
    'countdown438-445', 'early600-607', 'whispy1398-1405',
    'late3300-3307', 'natural-ko', 'natural-rebirth', 'timeup-results')

function Assert-Task25R([bool]$Condition, [string]$Message) {
    if (-not $Condition) { throw $Message }
}

function Resolve-Task25RPath([string]$Path) {
    if ([System.IO.Path]::IsPathRooted($Path)) {
        return [System.IO.Path]::GetFullPath($Path)
    }
    return [System.IO.Path]::GetFullPath((Join-Path $root $Path))
}

function Convert-Task25RRow($Row) {
    return @($Row | ForEach-Object { [int64]$_ })
}

function Get-Task25RStats([int64[]]$Values) {
    $ordered = @($Values | Sort-Object)
    Assert-Task25R ($ordered.Count -gt 0) 'Task 25R cannot summarize an empty row.'
    $middle = [int]($ordered.Count / 2)
    $median = if (($ordered.Count % 2) -eq 0) {
        [int64](($ordered[$middle - 1] + $ordered[$middle]) / 2)
    } else { [int64]$ordered[$middle] }
    $p95Index = [Math]::Max(0, [Math]::Ceiling($ordered.Count * 0.95) - 1)
    return [ordered]@{
        p50 = $median
        p95 = [int64]$ordered[$p95Index]
        maximum = [int64]$ordered[-1]
        samples = $ordered.Count
    }
}

function Get-Task25RPhaseName($Json) {
    $event = [string]$Json.identity.sampling.requestedStartEvent
    $start = [int64]$Json.identity.sampling.requestedStartFrame
    if ($event -eq 'KO') { return 'natural-ko' }
    if ($event -eq 'Rebirth') { return 'natural-rebirth' }
    if ($event -eq 'Late') { return 'late3300-3307' }
    if ($event -eq 'TimeUp') { return 'timeup-results' }
    if ($event -eq 'None' -and $start -eq 438) { return 'countdown438-445' }
    if ($event -eq 'None' -and $start -eq 600) { return 'early600-607' }
    if ($event -eq 'None' -and $start -eq 1398) { return 'whispy1398-1405' }
    throw "Unrecognized Task 25R phase gate event=$event start=$start."
}

function Get-Task25RIntervalSummary([int64[]]$Intervals) {
    $histogram = [ordered]@{ vblank2 = 0; vblank3 = 0; vblank4 = 0; vblank5Plus = 0 }
    [int64]$slips = 0
    foreach ($interval in $Intervals) {
        Assert-Task25R ($interval -ge 2) "Invalid presentation interval $interval."
        if ($interval -eq 2) { $histogram.vblank2++ }
        elseif ($interval -eq 3) { $histogram.vblank3++ }
        elseif ($interval -eq 4) { $histogram.vblank4++ }
        else { $histogram.vblank5Plus++ }
        $slips += $interval - 2
    }
    $vblanks = [int64](($Intervals | Measure-Object -Sum).Sum)
    return [ordered]@{
        histogram = $histogram
        slips = $slips
        presentationsPerSecond = [Math]::Round(60.0 * $Intervals.Count / $vblanks, 3)
        sourceUpdatesPerSecond = [Math]::Round(120.0 * $Intervals.Count / $vblanks, 3)
    }
}

function Get-Task25RFileIdentity([string]$Path) {
    $resolved = Resolve-Task25RPath $Path
    $item = Get-Item -LiteralPath $resolved
    return [ordered]@{
        path = $resolved
        sha256 = (Get-FileHash -LiteralPath $resolved -Algorithm SHA256).Hash
        bytes = [int64]$item.Length
    }
}

function Write-Task25RToolOutput(
    [string]$Executable, [string[]]$Arguments, [string]$Path) {
    & $Executable @Arguments 2>&1 | Set-Content -LiteralPath $Path -Encoding utf8
    if ($LASTEXITCODE -ne 0) {
        throw "$Executable $($Arguments -join ' ') failed."
    }
}

function Compress-Task25RFile([string]$Source, [string]$Destination) {
    $input = [System.IO.File]::OpenRead($Source)
    try {
        $output = [System.IO.File]::Create($Destination)
        try {
            $gzip = [System.IO.Compression.GZipStream]::new(
                $output, [System.IO.Compression.CompressionLevel]::Optimal)
            try { $input.CopyTo($gzip) } finally { $gzip.Dispose() }
        } finally { $output.Dispose() }
    } finally { $input.Dispose() }
}

Assert-Task25R ($PhaseJson.Count -eq 7) `
    "Task 25R requires exactly seven profile-1 phase JSON files; got $($PhaseJson.Count)."
$packets = @{}
foreach ($path in $PhaseJson) {
    $resolved = Resolve-Task25RPath $path
    Assert-Task25R (Test-Path -LiteralPath $resolved -PathType Leaf) `
        "Task 25R phase JSON not found: $resolved"
    $json = Get-Content -LiteralPath $resolved -Raw | ConvertFrom-Json
    $name = Get-Task25RPhaseName $json
    Assert-Task25R (-not $packets.ContainsKey($name)) `
        "Duplicate Task 25R phase '$name'."
    $packets[$name] = [pscustomobject]@{
        Name = $name
        Path = $resolved
        Json = $json
    }
}
Assert-Task25R (@($phaseOrder | Where-Object { -not $packets.ContainsKey($_) }).Count -eq 0) `
    'Task 25R phase set is incomplete.'

$first = $packets[$phaseOrder[0]].Json
$profile1Rom = [string]$first.identity.artifacts.rom.sha256
$profile1Elf = [string]$first.identity.artifacts.elf.sha256
$sourceHead = [string]$first.identity.gitHead
$phaseResults = @()
$profile1ExactnessFailures = @()
$timerDefinitions = @(
    @{ key = 'input'; index = 2; class = 'child'; parent = 'wholeLoop' },
    @{ key = 'sourceUpdate1'; index = 24; class = 'child'; parent = 'sourceUpdateAggregate' },
    @{ key = 'sourceUpdate2'; index = 25; class = 'child'; parent = 'sourceUpdateAggregate' },
    @{ key = 'sourceUpdateAggregate'; index = 4; class = 'child'; parent = 'updateBatch' },
    @{ key = 'audioUpdateShell'; index = -1; class = 'child'; parent = 'updateBatch'; derivation = 'updateBatch-sourceUpdateAggregate' },
    @{ key = 'updateBatch'; index = 3; class = 'child'; parent = 'wholeLoop' },
    @{ key = 'frameBegin'; index = 8; class = 'child'; parent = 'presentActive' },
    @{ key = 'wallpaper'; index = 10; class = 'child'; parent = 'draw' },
    @{ key = 'stage'; index = 11; class = 'child'; parent = 'draw' },
    @{ key = 'mario'; index = 12; class = 'child'; parent = 'draw' },
    @{ key = 'fox'; index = 13; class = 'child'; parent = 'draw' },
    @{ key = 'foregroundEffects'; index = 14; class = 'child'; parent = 'draw' },
    @{ key = 'drawResidual'; index = 19; class = 'child'; parent = 'draw' },
    @{ key = 'draw'; index = 9; class = 'child'; parent = 'presentActive' },
    @{ key = 'gxFlush'; index = 16; class = 'child'; parent = 'presentActive' },
    @{ key = 'postVBlank'; index = 17; class = 'child'; parent = 'presentActive' },
    @{ key = 'runnableThreads'; index = 18; class = 'child'; parent = 'presentActive' },
    @{ key = 'presentActive'; index = 6; class = 'child'; parent = 'wholeLoop' },
    @{ key = 'vblankWait'; index = 7; class = 'child'; parent = 'wholeLoop' },
    @{ key = 'loopResidual'; index = 21; class = 'child'; parent = 'wholeLoop' },
    @{ key = 'wholeLoop'; index = 1; class = 'root'; parent = $null },
    @{ key = 'hud'; index = 15; class = 'child'; parent = 'presentActive' },
    @{ key = 'presentResidual'; index = 20; class = 'child'; parent = 'presentActive' },
    @{ key = 'audioBackend'; index = 5; class = 'nested'; parent = 'audioUpdateShell' }
)
$leafRankingKeys = @(
    'input', 'sourceUpdate1', 'sourceUpdate2', 'audioUpdateShell',
    'frameBegin', 'wallpaper', 'stage', 'mario', 'fox',
    'foregroundEffects', 'drawResidual', 'hud', 'gxFlush', 'postVBlank',
    'runnableThreads', 'presentResidual', 'vblankWait', 'loopResidual')

foreach ($phaseName in $phaseOrder) {
    $packet = $packets[$phaseName]
    $json = $packet.Json
    Assert-Task25R ($json.kind -eq 'smash64ds-renderer-fast-raw-benchmark' -and
        $json.identity.rendererProfile -eq 1 -and $json.phaseMatrixMode -and
        $json.fastRunMode -eq 9 -and $json.staticTextureAotMode -eq 1 -and
        $json.foxCpuMode -eq 1 -and $json.wallpaperIncrementalMode -eq 1) `
        "$phaseName is not the production profile-1 Task 25R configuration."
    Assert-Task25R ($json.identity.gitHead -eq $sourceHead -and
        $json.identity.artifacts.rom.sha256 -eq $profile1Rom -and
        $json.identity.artifacts.elf.sha256 -eq $profile1Elf -and
        $json.identity.task9FloatItcmMode -eq 1 -and
        $json.identity.task9FloatPhase2Mode -eq 1 -and
        $json.identity.task16FloatCompareMode -eq 1 -and
        $json.identity.task16FloatI2fMode -eq 1 -and
        $json.identity.task16FloatAddSubMode -eq 1) `
        "$phaseName was not captured from the same exact profile-1 artifact and float configuration."
    $coarseRows = @($json.samples.coarse | ForEach-Object { ,(Convert-Task25RRow $_) })
    $fastRows = @($json.samples.fastRaw | ForEach-Object { ,(Convert-Task25RRow $_) })
    $m3Rows = @($json.samples.m3Stage | ForEach-Object { ,(Convert-Task25RRow $_) })
    $m2ShadeRows = @($json.samples.m2Shade | ForEach-Object { ,(Convert-Task25RRow $_) })
    $textureRows = @($json.samples.texturePhases | ForEach-Object { ,(Convert-Task25RRow $_) })
    $m4StaticRows = @($json.samples.m4Static | ForEach-Object { ,(Convert-Task25RRow $_) })
    $m4FenceRows = @($json.samples.m4Fence | ForEach-Object { ,(Convert-Task25RRow $_) })
    Assert-Task25R ($coarseRows.Count -eq 8 -and $fastRows.Count -eq 8 -and
        $m3Rows.Count -eq 8 -and $m2ShadeRows.Count -in @(0, 8) -and
        $textureRows.Count -eq 8 -and $m4StaticRows.Count -eq 8 -and
        $m4FenceRows.Count -eq 8) "$phaseName does not contain eight synchronized exactness rows."
    $phaseExactnessFailures = @()
    $observedFastGeometry = @()
    $observedFastFallback = @()
    $geometryFailureSamples = @()
    $m3FailureSamples = @()
    $m2FailureSamples = @()
    $textureFailureSamples = @()
    $m4FailureSamples = @()
    for ($index = 0; $index -lt 8; $index++) {
        $coarse = $coarseRows[$index]
        $fast = $fastRows[$index]
        $m3 = $m3Rows[$index]
        $texture = $textureRows[$index]
        $m4Static = $m4StaticRows[$index]
        $m4Fence = $m4FenceRows[$index]
        Assert-Task25R ($coarse.Count -eq 27 -and
            $coarse[24] + $coarse[25] -eq $coarse[4] -and
            $coarse[26] -ge 2 -and
            ($coarse[4] + $coarse[5]) -le $coarse[3] -and
            ($coarse[22] * 100) -le ($coarse[1] * 2)) `
            "$phaseName sample $index failed fixed-two or timing conservation."
        $fastGeometry = $fast[2..6] -join '/'
        $observedFastGeometry += $fastGeometry
        $observedFastFallback += ($fast[7..9] -join '/')
        $fastExact = (($fast -join ',') -match
            '^[0-9]+,9,121,828,202,320,306,0,0,0$')
        if (-not $fastExact) {
            $geometryFailureSamples += $index
        }
        $m3Exact = ($m3[4] -eq 8 -and $m3[5] -eq 255 -and
            $m3[6] -eq 0 -and $m3[7] -eq 57 -and $m3[8] -eq 42 -and
            $m3[9] -eq 54 -and $m3[10] -eq 202 -and $m3[11] -eq 49 -and
            $m3[12] -eq 4 -and $m3[13] -eq 4 -and
            $m3[14] -eq 5 -and $m3[15] -eq 10 -and $m3[16] -eq 15 -and
            $m3[17] -eq 1 -and $m3[18] -eq $m3[2] -and
            $m3[19] -eq 0 -and $m3[20] -eq 0 -and $m3[21] -eq 0 -and
            $m3[1] -eq ($m3[2] + $m3[3]))
        if (-not $m3Exact) {
            $m3FailureSamples += $index
        }
        $m2Exact = ($m2ShadeRows.Count -eq 0 -or
            $m2ShadeRows[$index][4] -eq 0)
        if (-not $m2Exact) {
            $m2FailureSamples += $index
        }
        $textureExact = ($texture[3] -eq 0 -and $texture[4] -eq 0 -and
            $texture[6] -eq 0 -and $texture[7] -eq 0 -and $texture[8] -eq 0 -and
            $texture[9] -eq 0 -and $texture[11] -eq 0 -and $texture[12] -eq 0)
        if (-not $textureExact) {
            $textureFailureSamples += $index
        }
        $m4Exact = ($m4Static[1] -eq 1 -and $m4Static[2] -eq 1 -and
            $m4Static[3] -eq 0 -and $m4Static[4] -eq 22 -and
            $m4Static[5] -eq 131072 -and $m4Static[6] -eq 1 -and
            $m4Static[7] -eq 0x3fffff -and $m4Static[8] -eq 0x7 -and
            $m4Static[9] -eq 0 -and $m4Static[10] -gt 0 -and
            @($m4Fence[1..12] | Where-Object { $_ -ne 0 }).Count -eq 0)
        if (-not $m4Exact) {
            $m4FailureSamples += $index
        }
    }
    if ($geometryFailureSamples.Count -gt 0) {
        $phaseExactnessFailures +=
            "geometry samples=$($geometryFailureSamples -join ',') observed=$(@($observedFastGeometry | Sort-Object -Unique) -join ',') fallback=$(@($observedFastFallback | Sort-Object -Unique) -join ',') expected=121/828/202/320/306 and 0/0/0"
    }
    if ($m3FailureSamples.Count -gt 0) {
        $phaseExactnessFailures += "M3 topology/state samples=$($m3FailureSamples -join ',')"
    }
    if ($m2FailureSamples.Count -gt 0) {
        $phaseExactnessFailures += "M2 shade collision samples=$($m2FailureSamples -join ',')"
    }
    if ($textureFailureSamples.Count -gt 0) {
        $phaseExactnessFailures +=
            "conversion/upload/fallback samples=$($textureFailureSamples -join ',')"
    }
    if ($m4FailureSamples.Count -gt 0) {
        $phaseExactnessFailures +=
            "static residency/fence samples=$($m4FailureSamples -join ',')"
    }
    if ($phaseExactnessFailures.Count -gt 0) {
        $profile1ExactnessFailures +=
            "$phaseName`: $($phaseExactnessFailures -join '; ')"
    }

    $timers = [ordered]@{}
    foreach ($definition in $timerDefinitions) {
        $values = if ($definition.index -ge 0) {
            @($coarseRows | ForEach-Object { [int64]$_[$definition.index] })
        } else {
            @($coarseRows | ForEach-Object { [int64]($_[3] - $_[4]) })
        }
        $entry = Get-Task25RStats $values
        $entry.timerClass = $definition.class
        $entry.parent = $definition.parent
        if ($definition.derivation) {
            $entry.derivation = $definition.derivation
        }
        $timers[$definition.key] = $entry
    }
    $intervals = @($coarseRows | ForEach-Object { [int64]$_[26] })
    $intervalSummary = Get-Task25RIntervalSummary $intervals
    $ranking = @($leafRankingKeys | ForEach-Object {
        [pscustomobject]@{
            owner = $_
            p50 = $timers[$_].p50
            p95 = $timers[$_].p95
            maximum = $timers[$_].maximum
        }
    } | Sort-Object p95 -Descending)
    $fighterPair = Get-Task25RStats @($coarseRows | ForEach-Object {
        [int64]($_[12] + $_[13])
    })
    $phaseResults += [pscustomobject]@{
        phase = $phaseName
        sampling = [ordered]@{
            requestedStartFrame = [int64]$json.identity.sampling.requestedStartFrame
            requestedStartEvent = [string]$json.identity.sampling.requestedStartEvent
            frameStart = [int64]$json.identity.sampling.frameStart
            frameEnd = [int64]$json.identity.sampling.frameEnd
            logicTickStart = [int64]$json.identity.sampling.logicTickStart
            logicTickEnd = [int64]$json.identity.sampling.logicTickEnd
        }
        source = Get-Task25RFileIdentity $packet.Path
        timers = $timers
        deadline = [ordered]@{
            ticks = $deadlineTicks
            p50Deficit = [int64]$timers.wholeLoop.p50 - $deadlineTicks
            p95Deficit = [int64]$timers.wholeLoop.p95 - $deadlineTicks
            maximumDeficit = [int64]$timers.wholeLoop.maximum - $deadlineTicks
        }
        intervals = $intervalSummary
        fighterPair = $fighterPair
        rankedLeafOwners = $ranking
        exactness = [ordered]@{
            pass = ($phaseExactnessFailures.Count -eq 0)
            failures = $phaseExactnessFailures
            observedFastGeometry = @($observedFastGeometry | Sort-Object -Unique)
            observedFastFallback = @($observedFastFallback | Sort-Object -Unique)
            synchronizedSamples = 8
            geometryRuns = 121
            triangles = 828
            ownerTriangles = @(202, 320, 306)
            fastFallbacks = 0
            textureOrResidencyFaults = 0
            conservationFailure = $false
        }
    }
}

$profile0Path = Resolve-Task25RPath $Profile0Json
$profile0 = Get-Content -LiteralPath $profile0Path -Raw | ConvertFrom-Json
Assert-Task25R ($profile0.kind -eq 'smash64ds-task25r-profile0-lifecycle' -and
    $profile0.identity.gitHead -eq $sourceHead -and
    $profile0.identity.make.Profile -eq 0 -and
    $profile0.identity.make.Task9FloatItcmMode -eq 1 -and
    $profile0.identity.make.Task9FloatPhase2Mode -eq 1 -and
    $profile0.identity.make.Task16FloatCompareMode -eq 1 -and
    $profile0.identity.make.Task16FloatI2fMode -eq 1 -and
    $profile0.identity.make.Task16FloatAddSubMode -eq 1) `
    'Task 25R profile-0 lifecycle is not the exact source/config sibling.'
Assert-Task25R ($profile0.gates.exactTwoUpdatesPerPresentation -and
    $profile0.gates.exactlyOneTeardown -and
    $profile0.pacing.presentations * 2 -eq $profile0.pacing.sourceUpdates) `
    'Task 25R profile-0 lifecycle lost exact fixed-two or teardown behavior.'

$pixelOutput = @(& (Join-Path $PSScriptRoot 'assert-melonds-top-visible.ps1') `
    -Image (Resolve-Task25RPath $Profile1Screenshot) `
    -CompareImage (Resolve-Task25RPath $Profile0Screenshot) `
    -WindowScaledCapture `
    -CompareChannelThreshold 1 `
    -MaxCompareChangedFraction 0 `
    -MaxCompareMeanChannelDelta 0 2>&1 | ForEach-Object { "$_" })
if (-not $?) { throw ($pixelOutput -join "`n") }
$pixelMatch = [regex]::Match(($pixelOutput -join "`n"),
    'Top screen delta: raw=([0-9]+)/([0-9]+).*?mean=([0-9.]+)')
Assert-Task25R ($pixelMatch.Success -and
    [int64]$pixelMatch.Groups[1].Value -eq 0 -and
    [int64]$pixelMatch.Groups[2].Value -eq 49152 -and
    [double]$pixelMatch.Groups[3].Value -eq 0.0) `
    'Task 25R synchronized profile-1/profile-0 top screens were not 0/49,152 exact.'
$pixelEvidence = [ordered]@{
    changedPixels = 0
    comparedPixels = 49152
    meanMaxChannelDelta = 0.0
    profile1 = Get-Task25RFileIdentity $Profile1Screenshot
    profile0 = Get-Task25RFileIdentity $Profile0Screenshot
}

$largestStage = @($phaseResults | Sort-Object { $_.timers.stage.p95 } -Descending)[0]
$largestFighters = @($phaseResults | Sort-Object { $_.fighterPair.p95 } -Descending)[0]
$recommendation = if ($largestStage.timers.stage.p95 -ge $largestFighters.fighterPair.p95) {
    'M3-first'
} else { 'M2-first' }
$identityEvidence = $null

if (-not [string]::IsNullOrWhiteSpace($IdentityDirectory)) {
    $identityDir = Resolve-Task25RPath $IdentityDirectory
    New-Item -ItemType Directory -Path $identityDir -Force | Out-Null
    $profile1ElfPath = [string]$first.identity.artifacts.elf.path
    $profile0ElfPath = [string]$profile0.identity.artifacts.elf.path
    $profile1Build = Split-Path -Parent $profile1ElfPath
    $profile0Build = Split-Path -Parent $profile0ElfPath
    $toolDir = 'C:\devkitPro\devkitARM\bin'
    $nm = Join-Path $toolDir 'arm-none-eabi-nm.exe'
    $readelf = Join-Path $toolDir 'arm-none-eabi-readelf.exe'
    $objdump = Join-Path $toolDir 'arm-none-eabi-objdump.exe'
    $size = Join-Path $toolDir 'arm-none-eabi-size.exe'
    $gcc = Join-Path $toolDir 'arm-none-eabi-gcc.exe'
    foreach ($tool in @($nm, $readelf, $objdump, $size, $gcc)) {
        Assert-Task25R (Test-Path -LiteralPath $tool -PathType Leaf) `
            "Task 25R tool not found: $tool"
    }
    $nmPath = Join-Path $identityDir 'profile1-nm-size-sort.txt'
    $readelfPath = Join-Path $identityDir 'profile1-readelf-sections-symbols-relocs-attributes.txt'
    $sizePath = Join-Path $identityDir 'profile1-size-sections.txt'
    $profile0NmPath = Join-Path $identityDir 'profile0-nm-size-sort.txt'
    Write-Task25RToolOutput $nm @('-S', '--size-sort', $profile1ElfPath) $nmPath
    Write-Task25RToolOutput $readelf @('-W', '-S', '-s', '-r', '-A', $profile1ElfPath) $readelfPath
    Write-Task25RToolOutput $size @('-A', $profile1ElfPath) $sizePath
    Write-Task25RToolOutput $nm @('-S', '--size-sort', $profile0ElfPath) $profile0NmPath
    $profile0NmText = Get-Content -LiteralPath $profile0NmPath -Raw
    foreach ($symbol in @(
        'gNdsRendererProfileSourceUpdate1Ticks',
        'gNdsRendererProfileSourceUpdate2Ticks',
        'gNdsRendererProfilePresentIntervalVBlanks')) {
        Assert-Task25R (-not $profile0NmText.Contains($symbol)) `
            "Profile-1 Task 25R symbol leaked into profile 0: $symbol"
    }
    $objdumpTemp = Join-Path $identityDir 'profile1-objdump.tmp.txt'
    Write-Task25RToolOutput $objdump @('-d', '-S', '-r', '-w', $profile1ElfPath) $objdumpTemp
    $objdumpGzip = Join-Path $identityDir 'profile1-objdump-d-S-r-w.txt.gz'
    Compress-Task25RFile $objdumpTemp $objdumpGzip

    $stackRows = @()
    $currentFunction = $null
    $stackLocalBytes = 0
    $stackSavedBytes = 0
    $spillStores = 0
    foreach ($line in [System.IO.File]::ReadLines($objdumpTemp)) {
        if ($line -match '^([0-9a-fA-F]+) <([^>]+)>:$') {
            if ($null -ne $currentFunction) {
                $stackRows += [pscustomobject]@{
                    function = $currentFunction
                    stackBytes = $stackLocalBytes + $stackSavedBytes
                    spillStores = $spillStores
                }
            }
            $currentFunction = $Matches[2]
            $stackLocalBytes = 0
            $stackSavedBytes = 0
            $spillStores = 0
        } elseif ($null -ne $currentFunction) {
            if ($line -match '\bsub(?:\.w)?\s+sp,\s*(?:sp,\s*)?#(0x[0-9a-fA-F]+|[0-9]+)') {
                $value = if ($Matches[1].StartsWith('0x')) {
                    [Convert]::ToInt32($Matches[1].Substring(2), 16)
                } else { [int]$Matches[1] }
                if ($value -gt $stackLocalBytes) { $stackLocalBytes = $value }
            }
            if ($line -match '\bpush\s+\{([^}]+)\}') {
                $savedRegisters = @($Matches[1] -split ',').Count
                $savedBytes = 4 * $savedRegisters
                if ($savedBytes -gt $stackSavedBytes) {
                    $stackSavedBytes = $savedBytes
                }
                $spillStores += $savedRegisters
            } elseif ($line -match '\b(?:str|stm)[^\[]*\[sp') {
                $spillStores++
            }
        }
    }
    if ($null -ne $currentFunction) {
        $stackRows += [pscustomobject]@{
            function = $currentFunction
            stackBytes = $stackLocalBytes + $stackSavedBytes
            spillStores = $spillStores
        }
    }
    Remove-Item -LiteralPath $objdumpTemp -Force

    $readelfLines = Get-Content -LiteralPath $readelfPath
    $sections = @()
    foreach ($line in $readelfLines) {
        if ($line -match '^\s*\[\s*(\d+)\]\s+(\S+)\s+\S+\s+([0-9a-fA-F]+)\s+[0-9a-fA-F]+\s+([0-9a-fA-F]+)\s+') {
            $sections += [pscustomobject]@{
                index = [int]$Matches[1]
                name = $Matches[2]
                address = ('0x' + $Matches[3].ToUpperInvariant())
                bytes = [Convert]::ToInt64($Matches[4], 16)
            }
        }
    }
    $hotSection = @($sections | Where-Object name -eq '.text.hot')
    $hotFunctions = @()
    $armFunctions = 0
    $thumbFunctions = 0
    $stubFunctions = @()
    foreach ($line in $readelfLines) {
        if ($line -match '^\s*\d+:\s+([0-9a-fA-F]+)\s+(\d+)\s+FUNC\s+\S+\s+\S+\s+(\d+)\s+(\S+)\s*$') {
            $value = [Convert]::ToInt64($Matches[1], 16)
            $sectionIndex = [int]$Matches[3]
            $name = $Matches[4]
            if (($value -band 1) -eq 0) { $armFunctions++ } else { $thumbFunctions++ }
            if ($hotSection.Count -eq 1 -and $sectionIndex -eq $hotSection[0].index) {
                $hotFunctions += [pscustomobject]@{ address = $value -band (-bnot 1); name = $name }
            }
            if ($name -match '(?i)veneer|stub|__.*from_') { $stubFunctions += $name }
        }
    }
    $mapPath = Join-Path $profile1Build '.map'
    $mapGzip = $null
    if (Test-Path -LiteralPath $mapPath -PathType Leaf) {
        $mapGzip = Join-Path $identityDir 'profile1-linker.map.gz'
        Compress-Task25RFile $mapPath $mapGzip
    }
    $toolVersionsPath = Join-Path $identityDir 'toolchain-versions.txt'
    $toolVersions = @(
        & $gcc --version 2>&1
        & (Join-Path $toolDir 'arm-none-eabi-ld.exe') --version 2>&1
        & $readelf --version 2>&1
        & $objdump --version 2>&1
    ) | ForEach-Object { "$_" }
    $pacman = 'C:\devkitPro\msys2\usr\bin\pacman.exe'
    if (Test-Path -LiteralPath $pacman -PathType Leaf) {
        $toolVersions += @(& $pacman -Qi devkitARM libnds calico 2>&1 |
            ForEach-Object { "$_" })
    }
    [System.IO.File]::WriteAllLines($toolVersionsPath, $toolVersions)

    $objectNames = @(
        'nds_renderer.o', 'nds_platform.o', 'scene_backend.o',
        'battle_playable_static_textures.o', 'nds_task16_float_compare.o',
        'nds_task16_float_i2f.o', 'nds_task16_float_addsub.o',
        'nds_task9_float_phase2.o',
        '_arm_cmpsf2.itcm.o', '_arm_unordsf2.itcm.o', '_arm_fixsfsi.itcm.o',
        '_arm_fixunssfsi.itcm.o', '_arm_addsubsf3.itcm.o', '_arm_muldivsf3.itcm.o')
    $objectIdentities = @($objectNames | ForEach-Object {
        $candidate = Join-Path $profile1Build $_
        if (Test-Path -LiteralPath $candidate -PathType Leaf) {
            Get-Task25RFileIdentity $candidate
        }
    })
    $generatedIdentities = @(
        'src/nds/nds_native_stage_owner.generated.inc',
        'src/nds/nds_native_fighter_owner.generated.inc',
        'src/nds/generated/battle_playable_static_textures.generated.inc'
    ) | ForEach-Object { Get-Task25RFileIdentity $_ }
    $hotFunctionNames = @($hotFunctions | ForEach-Object name)
    $identity = [ordered]@{
        schema = 1
        sourceHead = $sourceHead
        gitStatus = @($first.identity.gitStatus)
        focusedDiff = @($first.identity.focusedDiff)
        submodules = @($first.identity.submodules)
        profile1 = [ordered]@{
            rom = $first.identity.artifacts.rom
            elf = $first.identity.artifacts.elf
            buildCommand = @($first.identity.makeCommand)
        }
        profile0 = [ordered]@{
            rom = $profile0.identity.artifacts.rom
            elf = $profile0.identity.artifacts.elf
            buildCommand = @($profile0.identity.makeCommand)
        }
        relevantObjects = $objectIdentities
        generatedTables = @($generatedIdentities)
        sections = $sections
        instructionState = [ordered]@{
            armFunctions = $armFunctions
            thumbFunctions = $thumbFunctions
            hotTextOrder = @($hotFunctions | Sort-Object address)
            veneersAndStubs = @($stubFunctions | Sort-Object -Unique)
        }
        largestStackFrames = @($stackRows | Sort-Object stackBytes, spillStores -Descending |
            Select-Object -First 25)
        largestHotStackFrames = @($stackRows |
            Where-Object { $hotFunctionNames -contains $_.function } |
            Sort-Object stackBytes, spillStores -Descending)
        reserveBytes = [int64]$profile0.gates.reserveBytes
        raw = [ordered]@{
            nm = Get-Task25RFileIdentity $nmPath
            readelf = Get-Task25RFileIdentity $readelfPath
            objdumpGzip = Get-Task25RFileIdentity $objdumpGzip
            size = Get-Task25RFileIdentity $sizePath
            profile0Nm = Get-Task25RFileIdentity $profile0NmPath
            linkerMapGzip = if ($mapGzip) { Get-Task25RFileIdentity $mapGzip } else { $null }
            toolchain = Get-Task25RFileIdentity $toolVersionsPath
        }
    }
    $identityJsonPath = Join-Path $identityDir 'identity.json'
    Set-Content -LiteralPath $identityJsonPath -Encoding utf8 `
        -Value ($identity | ConvertTo-Json -Depth 9)
    $identityEvidence = Get-Task25RFileIdentity $identityJsonPath
}

$profile0SlowIntervals = [int64]$profile0.pacing.histogram.vblank3 +
    [int64]$profile0.pacing.histogram.vblank4 +
    [int64]$profile0.pacing.histogram.vblank5Plus
$pacingStable30 = ($profile0SlowIntervals -eq 0 -and
    [int64]$profile0.pacing.slips -eq 0)
$profile1ExactnessPass = ($profile1ExactnessFailures.Count -eq 0)
$stable30 = ($pacingStable30 -and $profile1ExactnessPass -and
    [bool]$profile0.gates.reservePass -and
    [bool]$profile0.gates.exactKoFgmTriplet -and
    [bool]$profile0.gates.zeroPostGoTextureFence)
$matrix = [ordered]@{
    schema = 1
    kind = 'smash64ds-task25r-authoritative-phase-matrix'
    sourceHead = $sourceHead
    accounting = [ordered]@{
        rule = 'parents and nested timers are never summed into leaf-owner totals'
        deadlineTicks = $deadlineTicks
        profile1DetailedRowsOnly = $true
        profile0ProductionPacingOnly = $true
    }
    artifacts = [ordered]@{
        profile1RomSha256 = $profile1Rom
        profile1ElfSha256 = $profile1Elf
        profile0 = Get-Task25RFileIdentity $profile0Path
        identityPacket = $identityEvidence
        pixels = $pixelEvidence
    }
    phases = $phaseResults
    productionPacing = [ordered]@{
        presentations = [int64]$profile0.pacing.presentations
        sourceUpdates = [int64]$profile0.pacing.sourceUpdates
        presentationsPerSecond = [Math]::Round(
            [double]$profile0.pacing.presentationRateX10 / 10.0, 1)
        sourceUpdatesPerSecond = [Math]::Round(
            [double]$profile0.pacing.sourceUpdateRateX10 / 10.0, 1)
        histogram = $profile0.pacing.histogram
        phases = $profile0.pacing.phases
        slips = [int64]$profile0.pacing.slips
    }
    readiness = [ordered]@{
        stable30 = $stable30
        pacingStable30 = $pacingStable30
        profile1ExactnessPass = $profile1ExactnessPass
        profile1ExactnessFailures = $profile1ExactnessFailures
        slowPresentationIntervals = $profile0SlowIntervals
        slips = [int64]$profile0.pacing.slips
        reserveBytes = [int64]$profile0.gates.reserveBytes
        reserveFloorBytes = [int64]$profile0.gates.reserveFloorBytes
        reservePass = [bool]$profile0.gates.reservePass
        exactTwoUpdatesPerPresentation = [bool]$profile0.gates.exactTwoUpdatesPerPresentation
        exactlyOneTeardown = [bool]$profile0.gates.exactlyOneTeardown
        exactKoFgmTriplet = [bool]$profile0.gates.exactKoFgmTriplet
        zeroPostGoTextureFence = [bool]$profile0.gates.zeroPostGoTextureFence
        changedPixels = 0
    }
    recommendation = [ordered]@{
        lane = $recommendation
        largestM3Phase = $largestStage.phase
        largestM3StageP95 = [int64]$largestStage.timers.stage.p95
        largestM2Phase = $largestFighters.phase
        largestM2FighterPairP95 = [int64]$largestFighters.fighterPair.p95
        basis = 'same profile-1 ROM leaf-owner P95 only'
    }
    taskBounds = [ordered]@{
        task20R = 'stack/DTCM only; no timing candidate before lifecycle reserve and stack evidence'
        task21R_task27 = [ordered]@{
            owner = 'Mario+Fox M2'; maximumP95 = [int64]$largestFighters.fighterPair.p95
            phase = $largestFighters.phase
        }
        task22R = [ordered]@{
            owner = 'wallpaper'; maximumP95 = [int64](@($phaseResults |
                ForEach-Object { $_.timers.wallpaper.p95 } | Measure-Object -Maximum).Maximum)
        }
        task23R_task26 = [ordered]@{
            owner = 'stage M3'; maximumP95 = [int64]$largestStage.timers.stage.p95
            phase = $largestStage.phase
        }
    }
}

$outputJsonPath = Resolve-Task25RPath $OutputJson
$outputMarkdownPath = Resolve-Task25RPath $OutputMarkdown
foreach ($outputPath in @($outputJsonPath, $outputMarkdownPath)) {
    $parent = Split-Path -Parent $outputPath
    if ($parent -and -not (Test-Path -LiteralPath $parent)) {
        New-Item -ItemType Directory -Path $parent -Force | Out-Null
    }
}
Set-Content -LiteralPath $outputJsonPath -Encoding utf8 `
    -Value ($matrix | ConvertTo-Json -Depth 10)

$markdown = @(
    '# Task 25R authoritative same-artifact matrix',
    '',
    "Source HEAD: ``$sourceHead``  ",
    "Profile-1 ROM: ``$profile1Rom``  ",
    "Recommendation: **$recommendation**  ",
    '',
    '| Phase | Frame / logic window | Exact | Loop P50/P95/max | Deficit P50/P95/max | Update P95 | Wallpaper P95 | Stage P95 | Mario/Fox P95 | 2/3/4/5+ | Present/s | Update/s |',
    '|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|'
)
foreach ($phase in $phaseResults) {
    $h = $phase.intervals.histogram
    $markdown += "| $($phase.phase) | $($phase.sampling.frameStart)-$($phase.sampling.frameEnd) / $($phase.sampling.logicTickStart)-$($phase.sampling.logicTickEnd) | $($phase.exactness.pass) | $($phase.timers.wholeLoop.p50)/$($phase.timers.wholeLoop.p95)/$($phase.timers.wholeLoop.maximum) | $($phase.deadline.p50Deficit)/$($phase.deadline.p95Deficit)/$($phase.deadline.maximumDeficit) | $($phase.timers.updateBatch.p95) | $($phase.timers.wallpaper.p95) | $($phase.timers.stage.p95) | $($phase.timers.mario.p95)/$($phase.timers.fox.p95) | $($h.vblank2)/$($h.vblank3)/$($h.vblank4)/$($h.vblank5Plus) | $($phase.intervals.presentationsPerSecond) | $($phase.intervals.sourceUpdatesPerSecond) |"
}
$markdown += @('', '## Profile-1 exactness', '')
if ($profile1ExactnessPass) {
    $markdown += '- All seven synchronized phase gates passed.'
} else {
    $markdown += @($profile1ExactnessFailures | ForEach-Object { "- $_" })
}
$markdown += @('', '## Detailed profile-1 rows', '')
$markdown += '| Timer | Class / parent | ' + (($phaseResults | ForEach-Object {
    $_.phase
}) -join ' | ') + ' |'
$markdown += '|---|---|' + (($phaseResults | ForEach-Object { '---:|' }) -join '')
foreach ($definition in $timerDefinitions) {
    $classParent = if ($definition.parent) {
        "$($definition.class) / $($definition.parent)"
    } else { [string]$definition.class }
    if ($definition.derivation) {
        $classParent += " / $($definition.derivation)"
    }
    $cells = @($phaseResults | ForEach-Object {
        $row = $_.timers.($definition.key)
        "$($row.p50)/$($row.p95)/$($row.maximum)/$($row.samples)"
    })
    $markdown += "| $($definition.key) | $classParent | $($cells -join ' | ') |"
}
$markdown += @('', '## Ranked leaf owners by P95', '')
foreach ($phase in $phaseResults) {
    $rankingText = @($phase.rankedLeafOwners | ForEach-Object {
        "$($_.owner)=$($_.p95)"
    }) -join ', '
    $markdown += "- $($phase.phase): $rankingText"
}
$ph = $profile0.pacing.histogram
$markdown += @(
    '',
    '## Production profile-0 lifecycle',
    '',
    "- Presentations/source updates: $($profile0.pacing.presentations)/$($profile0.pacing.sourceUpdates)",
    "- Presentations/s and updates/s: $([double]$profile0.pacing.presentationRateX10 / 10.0) / $([double]$profile0.pacing.sourceUpdateRateX10 / 10.0)",
    "- Interval histogram 2/3/4/5+: $($ph.vblank2)/$($ph.vblank3)/$($ph.vblank4)/$($ph.vblank5Plus)",
    "- Slip events: $($profile0.pacing.slips)",
    "- Reserve: $($profile0.gates.reserveBytes)/$($profile0.gates.reserveFloorBytes) bytes (pass=$($profile0.gates.reservePass))",
    "- Exact KO FGM triplet: $($profile0.gates.exactKoFgmTriplet)",
    "- Zero post-GO texture fence: $($profile0.gates.zeroPostGoTextureFence)",
    '- Synchronized pixels: 0/49,152 changed',
    "- Stable-30 ready: $stable30"
)
$markdown += @('', '| Lifecycle phase | 2 | 3 | 4 | 5+ | Slips |',
    '|---|---:|---:|---:|---:|---:|')
foreach ($phase in $profile0.pacing.phases) {
    $markdown += "| $($phase.phase) | $($phase.vblank2) | $($phase.vblank3) | $($phase.vblank4) | $($phase.vblank5Plus) | $($phase.slips) |"
}
$markdown += @(
    '',
    '## Task bounds',
    '',
    "- Task 20R: $($matrix.taskBounds.task20R)",
    "- Tasks 21R/27: Mario+Fox P95 <= $($matrix.taskBounds.task21R_task27.maximumP95) ticks in $($matrix.taskBounds.task21R_task27.phase).",
    "- Task 22R: wallpaper P95 <= $($matrix.taskBounds.task22R.maximumP95) ticks.",
    "- Tasks 23R/26: stage P95 <= $($matrix.taskBounds.task23R_task26.maximumP95) ticks in $($matrix.taskBounds.task23R_task26.phase)."
)
Set-Content -LiteralPath $outputMarkdownPath -Encoding utf8 -Value $markdown

Write-Output ("Task 25R matrix report produced: phases=7 profile1=$profile1Rom " +
    "profile0=$($profile0.identity.artifacts.rom.sha256) " +
    "pixels=0/49152 exact=$profile1ExactnessPass stable30=$stable30 intervals3plus=$profile0SlowIntervals " +
    "slips=$($profile0.pacing.slips) reserve=$($profile0.gates.reserveBytes) " +
    "recommendation=$recommendation")
if ($RequireStable30 -and -not $stable30) {
    throw "Task 25R stable-30 gate failed: 3+ intervals=$profile0SlowIntervals slips=$($profile0.pacing.slips) profile1-exact=$profile1ExactnessPass reserve=$($profile0.gates.reservePass) KO-FGM=$($profile0.gates.exactKoFgmTriplet) fence=$($profile0.gates.zeroPostGoTextureFence)."
}
