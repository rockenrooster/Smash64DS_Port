param(
    [string]$Root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
)
$ErrorActionPreference = 'Stop'

$tracks = @(
    [PSCustomObject]@{
        Name = 'Pupupu'; File = 'bgm_pupupu_pcm16.raw'; Sequence = 0
        Bytes = 2886710; Sha256 = '581191127a00c8ddbd4395cc00b5d4722bbeca734a0990e778a2ea5e9138effa'
        Looping = $true; LoopStart = 8798; LoopStartTick = 298; LoopStartTickMin = 298
    },
    [PSCustomObject]@{
        Name = 'Mario winner'; File = 'bgm_win_mario_pcm16.raw'; Sequence = 12
        Bytes = 326800; Sha256 = '3baf58e4dea713badfe257f42fa0057bd76e481d4792f4ddf18600c0a9239018'
        Looping = $false; LoopStart = 0; LoopStartTick = 0; LoopStartTickMin = 0
    },
    [PSCustomObject]@{
        Name = 'Fox winner'; File = 'bgm_win_fox_pcm16.raw'; Sequence = 16
        Bytes = 291154; Sha256 = '3e7beea06b921ca8aaa9af55969d0deddb0aaef4e23805ce45f78485b784d66c'
        Looping = $false; LoopStart = 0; LoopStartTick = 0; LoopStartTickMin = 0
    },
    [PSCustomObject]@{
        Name = 'Results'; File = 'bgm_results_pcm16.raw'; Sequence = 22
        Bytes = 1624750; Sha256 = 'a96c3e5b0c0348ce0a35baf7b1d29c38f9f771edaaf82ff51d6101a668d32bd8'
        Looping = $true; LoopStart = 34912; LoopStartTick = 779; LoopStartTickMin = 276
    }
)
$pupupuFirstRingPeak = 0
$pupupuFirstRingRms = 0.0

foreach ($track in $tracks) {
    $asset = Join-Path $Root "assets/audio/$($track.File)"
    $metadataPath = [IO.Path]::ChangeExtension($asset, '.json')
    if (-not (Test-Path -LiteralPath $asset -PathType Leaf)) {
        throw "$($track.Name) derived BGM asset is missing: $asset"
    }
    if (-not (Test-Path -LiteralPath $metadataPath -PathType Leaf)) {
        throw "$($track.Name) derived BGM metadata is missing: $metadataPath"
    }

    $file = Get-Item -LiteralPath $asset
    $sha = (Get-FileHash -LiteralPath $asset -Algorithm SHA256).Hash.ToLowerInvariant()
    $metadata = Get-Content -LiteralPath $metadataPath -Raw | ConvertFrom-Json

    if ($file.Length -ne $track.Bytes -or $sha -ne $track.Sha256) {
        throw "$($track.Name) PCM payload changed: bytes=$($file.Length) sha256=$sha"
    }
    if ($metadata.sequence_index -ne $track.Sequence -or
        $metadata.bytes -ne $track.Bytes -or
        $metadata.sha256 -ne $track.Sha256 -or
        $metadata.sample_rate -ne 22050 -or
        $metadata.format -ne 'signed PCM16LE mono raw') {
        throw "$($track.Name) metadata no longer matches its exact source-derived payload."
    }
    if ([bool]$metadata.looping -ne $track.Looping -or
        $metadata.loop_start_byte -ne $track.LoopStart -or
        $metadata.loop_start_tick -ne $track.LoopStartTick -or
        $metadata.loop_start_tick_min -ne $track.LoopStartTickMin) {
        throw "$($track.Name) loop metadata changed unexpectedly."
    }
    $expectedSource = "BattleShip_o2r/audio/S1_music_sbk sequence $($track.Sequence) + B1_sounds1_ctl/tbl"
    if ($metadata.source -ne $expectedSource -or
        $metadata.tool -ne 'scripts/render-audio-bgm-pupupu.py') {
        throw "$($track.Name) no longer records the exact BattleShip source/tool provenance."
    }
    if ($track.Sequence -eq 0) {
        $ring = New-Object byte[] 65536
        $stream = [IO.File]::OpenRead($asset)
        try {
            if ($stream.Read($ring, 0, $ring.Length) -ne $ring.Length) {
                throw 'Pupupu stream no longer fills the initial DS ring.'
            }
        } finally {
            $stream.Dispose()
        }
        [double]$sumSquares = 0.0
        for ($i = 0; $i -lt $ring.Length; $i += 2) {
            $sample = [BitConverter]::ToInt16($ring, $i)
            $magnitude = [Math]::Abs([int]$sample)
            if ($magnitude -gt $pupupuFirstRingPeak) {
                $pupupuFirstRingPeak = $magnitude
            }
            $sumSquares += [double]$sample * [double]$sample
        }
        $pupupuFirstRingRms = [Math]::Sqrt(
            $sumSquares / ($ring.Length / 2))
    }
}

if ($pupupuFirstRingPeak -ne 9928 -or
    [Math]::Abs($pupupuFirstRingRms - 2283.623071) -gt 0.000001) {
    throw ('Pupupu initial DS ring acoustic fixture changed: peak={0} rms={1:F6}' -f
        $pupupuFirstRingPeak, $pupupuFirstRingRms)
}

$headerPath = Join-Path $Root 'include/nds/nds_audio_bgm.h'
$header = Get-Content -LiteralPath $headerPath -Raw
$required = @(
    'NDS_AUDIO_BGM_TRACK_WIN_MARIO 12u',
    'NDS_AUDIO_BGM_TRACK_WIN_FOX 16u',
    'NDS_AUDIO_BGM_TRACK_RESULTS 22u',
    'NDS_AUDIO_BGM_WIN_MARIO_STREAM_BYTES 326800u',
    'NDS_AUDIO_BGM_WIN_FOX_STREAM_BYTES 291154u',
    'NDS_AUDIO_BGM_RESULTS_STREAM_BYTES 1624750u',
    'NDS_AUDIO_BGM_RESULTS_LOOP_START_BYTES 34912u'
)
foreach ($needle in $required) {
    if (-not $header.Contains($needle)) {
        throw "BGM runtime header is missing exact derived constant: $needle"
    }
}

Write-Output ('BattleShip-derived BGM assets passed: tracks=0/12/16/22 ' +
    'bytes=5125414 resident=65536 pupupu_ring_peak={0} rms={1:F6}.' -f
    $pupupuFirstRingPeak, $pupupuFirstRingRms)
