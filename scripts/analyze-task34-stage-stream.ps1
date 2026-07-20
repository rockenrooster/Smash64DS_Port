param(
    [Parameter(Mandatory = $true)][string[]]$CapturePath,
    [string]$GeneratedStage = (Join-Path $PSScriptRoot '..\src\nds\nds_native_stage_owner.generated.inc'),
    [string]$ConsumedFields = (Join-Path $PSScriptRoot '..\docs\optimization\NDS_NATIVE_STAGE_CONSUMED_FIELDS.generated.json'),
    [Parameter(Mandatory = $true)][string]$OutputPath
)

$ErrorActionPreference = 'Stop'

function Get-Sha256Text([string]$Text) {
    $bytes = [Text.Encoding]::UTF8.GetBytes($Text)
    $hash = [Security.Cryptography.SHA256]::HashData($bytes)
    return [Convert]::ToHexString($hash)
}

foreach ($path in @($CapturePath) + @($GeneratedStage, $ConsumedFields)) {
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
        throw "Task 34 E1 input is missing: $path"
    }
}

$stageText = Get-Content -LiteralPath $GeneratedStage -Raw
$dobjBlock = [regex]::Match(
    $stageText,
    '(?s)static const NDSNativeStageDObj sNdsNativeStageDObjs\[57\] = \{(?<body>.*?)\n\};')
if (-not $dobjBlock.Success) {
    throw 'Generated stage DObj table was not found.'
}
$dobjRows = @([regex]::Matches(
    $dobjBlock.Groups['body'].Value,
    '\{\s*0x([0-9a-f]+)u,\s*0x([0-9a-f]+)u,\s*0x([0-9a-f]+)u,\s*0x([0-9a-f]+)u,\s*([0-9]+)u,\s*([0-9]+)u\s*\}'))
if ($dobjRows.Count -ne 57) {
    throw "Generated stage DObj table has $($dobjRows.Count) rows instead of 57."
}

$manifest = Get-Content -LiteralPath $ConsumedFields -Raw | ConvertFrom-Json
$cameraFields = @($manifest.root_frame_fields |
    Where-Object classification -eq 'live_camera_dependent' |
    ForEach-Object fields)
if (($cameraFields -notcontains 'projection') -or
    ($cameraFields -notcontains 'binding_composed') -or
    -not (@($manifest.invalidation_manifest) -match
        'camera projection or any of 42 composed matrices changes')) {
    throw 'Task 23R manifest no longer proves the live camera/matrix invalidation seam.'
}

$frames = [Collections.Generic.List[object]]::new()
$captureIdentity = [Collections.Generic.List[object]]::new()
foreach ($path in $CapturePath) {
    $capture = Get-Content -LiteralPath $path -Raw | ConvertFrom-Json
    if (-not $capture.task34StageStreamCensus) {
        throw "Capture is not a Task 34 E1 stream census: $path"
    }
    $captureIdentity.Add([ordered]@{
        path = (Resolve-Path -LiteralPath $path).Path
        sha256 = (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash
        frame_start = $capture.identity.sampling.frameStart
        frame_end = $capture.identity.sampling.frameEnd
    })
    $entries = @($capture.samples.task34StageEntries)
    $words = @($capture.samples.task34StageWords)
    foreach ($meta in @($capture.samples.task34StageMeta)) {
        $frame = [int]$meta[0]
        $frameEntries = @($entries | Where-Object { [int]$_[0] -eq $frame } |
            Sort-Object { [int]$_[1] })
        $frameWords = @($words | Where-Object { [int]$_[0] -eq $frame } |
            Sort-Object { [int]$_[1] })
        if (($frameEntries.Count -ne [int]$meta[1]) -or
            ($frameWords.Count -ne [int]$meta[2]) -or
            ([int]$meta[3] -ne 0) -or ([int]$meta[4] -ne 0)) {
            throw "Frame $frame does not conserve its stream meta row."
        }
        for ($i = 0; $i -lt $frameWords.Count; $i++) {
            if ([int]$frameWords[$i][1] -ne $i) {
                throw "Frame $frame word $i is out of order."
            }
        }

        $perDObj = @{}
        foreach ($entry in $frameEntries) {
            $dobj = [int]$entry[2]
            if (-not $perDObj.ContainsKey($dobj)) {
                $perDObj[$dobj] = [ordered]@{
                    text = [Text.StringBuilder]::new()
                    entries = 0
                    words = 0
                    segments = [Collections.Generic.HashSet[int]]::new()
                }
            }
            $row = $perDObj[$dobj]
            $offset = [int]$entry[4]
            $count = [int]$entry[5]
            [void]$row.text.AppendFormat(
                'c{0}:n{1}:s{2};', [int]$entry[3], $count, [int]$entry[6])
            for ($i = 0; $i -lt $count; $i++) {
                [void]$row.text.AppendFormat('{0:x8},', [uint32]$frameWords[$offset + $i][2])
            }
            $row.entries++
            $row.words += $count
            [void]$row.segments.Add([int]$entry[6])
        }
        $frameRows = @{}
        foreach ($dobj in $perDObj.Keys) {
            $row = $perDObj[$dobj]
            $frameRows[$dobj] = [ordered]@{
                hash = Get-Sha256Text $row.text.ToString()
                entries = $row.entries
                words = $row.words
                serialized_bytes = 4 * [math]::Ceiling($row.entries / 4.0) +
                    4 * $row.words
                segments = @($row.segments | Sort-Object)
            }
        }
        $frames.Add([ordered]@{
            frame = $frame
            entries = [int]$meta[1]
            words = [int]$meta[2]
            serialized_bytes = 4 * [math]::Ceiling(([int]$meta[1]) / 4.0) +
                4 * [int]$meta[2]
            dobjs = $frameRows
        })
    }
}

if ($frames.Count -ne 24) {
    throw "Task 34 E1 requires 24 synchronized frames; captured $($frames.Count)."
}

$reference = $frames[0]
$identical = [Collections.Generic.List[int]]::new()
$varying = [Collections.Generic.List[int]]::new()
$noStream = [Collections.Generic.List[int]]::new()
$perDObjReport = [Collections.Generic.List[object]]::new()
for ($dobj = 0; $dobj -lt 57; $dobj++) {
    $samples = @($frames | ForEach-Object {
        if ($_.dobjs.ContainsKey($dobj)) { $_.dobjs[$dobj] }
    })
    $binding = [Convert]::ToUInt32($dobjRows[$dobj].Groups[3].Value, 16)
    if ($samples.Count -eq 0) {
        $noStream.Add($dobj)
        $classification = 'no_stream'
    } elseif (($samples.Count -eq $frames.Count) -and
              (@($samples.hash | Sort-Object -Unique).Count -eq 1)) {
        $identical.Add($dobj)
        $classification = 'bit_identical'
    } else {
        $varying.Add($dobj)
        $classification = 'varying'
    }
    $perDObjReport.Add([ordered]@{
        dobj = $dobj
        binding = if ($binding -eq 0xffff) { $null } else { [int]$binding }
        owner = [int]$dobjRows[$dobj].Groups[5].Value
        depth = [int]$dobjRows[$dobj].Groups[6].Value
        classification = $classification
        sample_count = $samples.Count
        distinct_hashes = @($samples.hash | Sort-Object -Unique).Count
        entries = if ($samples.Count) { $samples[0].entries } else { 0 }
        words = if ($samples.Count) { $samples[0].words } else { 0 }
        serialized_bytes = if ($samples.Count) {
            $samples[0].serialized_bytes
        } else { 0 }
        segments = if ($samples.Count) { @($samples[0].segments) } else { @() }
    })
}

$staticWords = [int64](($perDObjReport |
    Where-Object classification -eq 'bit_identical' |
    Measure-Object -Property words -Sum).Sum)
$staticBytes = [int64](($perDObjReport |
    Where-Object classification -eq 'bit_identical' |
    Measure-Object -Property serialized_bytes -Sum).Sum)
$totalWords = [int64]$reference.words
$wordPercent = if ($totalWords) { 100.0 * $staticWords / $totalWords } else { 0.0 }
$decision = if ($wordPercent -ge 60.0) { 'CONTINUE_TO_E2' } else { 'STOP_BELOW_60_PERCENT' }

$result = [ordered]@{
    schema = 'smash64ds.task34-stage-stream-e1.v1'
    decision = $decision
    capture_count = $CapturePath.Count
    frame_count = $frames.Count
    captures = @($captureIdentity)
    task23r_manifest = [ordered]@{
        path = (Resolve-Path -LiteralPath $ConsumedFields).Path
        sha256 = (Get-FileHash -LiteralPath $ConsumedFields -Algorithm SHA256).Hash
        live_camera_fields = $cameraFields
        matrix_invalidation = @($manifest.invalidation_manifest | Where-Object {
            $_ -match 'camera projection or any of 42 composed matrices changes'
        })
    }
    stream = [ordered]@{
        entries_per_frame = @($frames.entries | Sort-Object -Unique)
        words_per_frame = @($frames.words | Sort-Object -Unique)
        serialized_bytes_per_frame = @($frames.serialized_bytes | Sort-Object -Unique)
        bit_identical_dobjs = @($identical)
        varying_dobjs = @($varying)
        no_stream_dobjs = @($noStream)
        bit_identical_words = $staticWords
        bit_identical_serialized_bytes = $staticBytes
        bit_identical_word_percent = [math]::Round($wordPercent, 3)
        kill_threshold_percent = 60.0
    }
    dobjs = @($perDObjReport)
}

$parent = Split-Path -Parent $OutputPath
if ($parent -and -not (Test-Path -LiteralPath $parent)) {
    New-Item -ItemType Directory -Path $parent -Force | Out-Null
}
$result | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $OutputPath -Encoding utf8
Write-Output ("Task 34 E1: decision={0} identical={1} varying={2} no-stream={3} words={4}/{5} ({6:N3}%) bytes={7}." -f
    $decision, $identical.Count, $varying.Count, $noStream.Count,
    $staticWords, $totalWords, $wordPercent, $staticBytes)
