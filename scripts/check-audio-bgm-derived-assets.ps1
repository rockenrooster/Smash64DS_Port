param(
    [string]$Root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
)
$ErrorActionPreference = 'Stop'

$tracks = @(
    # P2-1L bug (b1), 2026-08-19: re-rendered after fixing
    # collect_loop_metadata() to stop leaving a flat ~1s trailing-silence
    # render pad inside the loop region (LoopSample/LoopPacket/LoopRecord
    # unchanged -- this track's channels already agreed on one loop
    # period, so only the stream length changed).
    [PSCustomObject]@{
        Name = 'Pupupu'; File = 'bgm_pupupu_ima.bin'; Sequence = 0
        Bytes = 711920; Sha256 = '4bedaa9f2fe62b4b28b0a75b389b0e9ccc6ce705930d63797dc73f2cb8c7b32c'
        SourceBytes = 2843290; SourceSha256 = '0cf096444de1bbf14c8da498697365bc36ae5b0d9c091356be065aa844dad8ea'
        Packets = 88; Looping = $true; LoopSample = 4399; LoopPacket = 1; LoopRecord = 2252
    },
    [PSCustomObject]@{
        Name = 'Mario winner'; File = 'bgm_win_mario_ima.bin'; Sequence = 12
        Bytes = 81860; Sha256 = '630d6cd4aaeef0972dbf7d8b0cf0a265c242055bf25e43578a689db631d45e75'
        SourceBytes = 326800; SourceSha256 = '3baf58e4dea713badfe257f42fa0057bd76e481d4792f4ddf18600c0a9239018'
        Packets = 10; Looping = $false; LoopSample = [uint32]::MaxValue; LoopPacket = [uint32]::MaxValue; LoopRecord = 0
    },
    [PSCustomObject]@{
        Name = 'Fox winner'; File = 'bgm_win_fox_ima.bin'; Sequence = 16
        Bytes = 72940; Sha256 = '8c513904a1d10642d6a27a6d92adb7e0688e5f43c89c8baf2f183080fe2501e9'
        SourceBytes = 291154; SourceSha256 = '3e7beea06b921ca8aaa9af55969d0deddb0aaef4e23805ce45f78485b784d66c'
        Packets = 9; Looping = $false; LoopSample = [uint32]::MaxValue; LoopPacket = [uint32]::MaxValue; LoopRecord = 0
    },
    # P2-1L bug (b1): same re-render as Pupupu above, same reason
    # (LoopSample/LoopPacket/LoopRecord unchanged).
    [PSCustomObject]@{
        Name = 'Results'; File = 'bgm_results_ima.bin'; Sequence = 22
        Bytes = 396588; Sha256 = 'fd963304d0c2f37d30023be2cf3d2ea3af7f329619a0eb52c4ead317f3833ed6'
        SourceBytes = 1583786; SourceSha256 = '895367c302e620a13447c170e783ae3849c5fad894b763993fc980969e8a2567'
        Packets = 50; Looping = $true; LoopSample = 17456; LoopPacket = 2; LoopRecord = 8792
    },
    # P2-1d-1: nSYAudioBGMModeSelect (id 44), rendered through the same script
    # (--sequence-index 44) as every track above -- the source's own S1_music_sbk
    # sequence index for the main menu track. mnmodeselect.c:882 plays it on
    # arrival at ModeSelect from a non-menu scene.
    # P2-1L bug (b1), 2026-08-19: LoopSample moved from 238691 to 1151965
    # (the old max()-over-every-channel reading was dominated by a
    # near-silent outlier channel with a much longer loop period than the
    # tune's own majority-agreeing channels -- see nds_audio_bgm.h).
    [PSCustomObject]@{
        Name = 'Mode select'; File = 'bgm_mode_select_ima.bin'; Sequence = 44
        Bytes = 718212; Sha256 = '209c92cda3d9fa45089249977f3c655d0ff225971cc4947e5de89334f568fce0'
        SourceBytes = 2868410; SourceSha256 = '019718b2485ee609d7785caeb5d064e90c7d842811990f2379dfae583b800e8b'
        Packets = 89; Looping = $true; LoopSample = 1151965; LoopPacket = 71; LoopRecord = 576876
    },
    # P2-1e-1: nSYAudioBGMBattleSelect (id 10), rendered through the same script
    # (--sequence-index 10) as every track above -- the source's own S1_music_sbk
    # sequence index for the CSS's own track. mnplayersvs.c:4899 plays it on
    # arrival at PlayersVS unless scene_prev is the stage select (nSCKindMaps).
    # P2-1L bug (b1): LoopSample/LoopPacket/LoopRecord unchanged -- this
    # track's channels already agreed on one loop period, so only the
    # stream length changed (the flat trailing-silence pad that was
    # sitting inside the loop is gone, see nds_audio_bgm.h).
    [PSCustomObject]@{
        Name = 'Battle select'; File = 'bgm_battle_select_ima.bin'; Sequence = 10
        Bytes = 157372; Sha256 = 'f4627806d2f1f9f3f34bd2933ccf59f2a72e9f459092bf3a4a2a20f6331b75d7'
        SourceBytes = 628352; SourceSha256 = '33658ec31440001346099ca8e9945f376e3f9202dab5e49364d037ab1bec162b'
        Packets = 20; Looping = $true; LoopSample = 46228; LoopPacket = 3; LoopRecord = 23192
    }
)

function Get-U16([byte[]]$Data, [int]$Offset) {
    return [BitConverter]::ToUInt16($Data, $Offset)
}

function Get-U32([byte[]]$Data, [int]$Offset) {
    return [BitConverter]::ToUInt32($Data, $Offset)
}

[int64]$compressedTotal = 0
foreach ($track in $tracks) {
    $asset = Join-Path $Root "assets/audio/$($track.File)"
    $metadataPath = [IO.Path]::ChangeExtension($asset, '.json')
    if (-not (Test-Path -LiteralPath $asset -PathType Leaf) -or
        -not (Test-Path -LiteralPath $metadataPath -PathType Leaf)) {
        throw "$($track.Name) ADPCM asset or metadata is missing."
    }

    $data = [IO.File]::ReadAllBytes($asset)
    $sha = (Get-FileHash -LiteralPath $asset -Algorithm SHA256).Hash.ToLowerInvariant()
    $metadata = Get-Content -LiteralPath $metadataPath -Raw | ConvertFrom-Json
    if ($data.Length -ne $track.Bytes -or $sha -ne $track.Sha256) {
        throw "$($track.Name) ADPCM payload changed: bytes=$($data.Length) sha256=$sha"
    }
    if ($metadata.sequence_index -ne $track.Sequence -or
        $metadata.bytes -ne $track.Bytes -or $metadata.sha256 -ne $track.Sha256 -or
        $metadata.source_pcm_bytes -ne $track.SourceBytes -or
        $metadata.source_pcm_sha256 -ne $track.SourceSha256 -or
        $metadata.sample_rate -ne 22050 -or
        $metadata.format -ne 'Nintendo DS IMA-ADPCM packet stream' -or
        $metadata.container_magic -ne 'BGA1' -or
        $metadata.container_version -ne 1 -or $metadata.header_bytes -ne 40 -or
        $metadata.packet_samples -ne 16384 -or
        $metadata.packet_count -ne $track.Packets -or
        [bool]$metadata.looping -ne $track.Looping -or
        $metadata.loop_packet_index -ne $track.LoopPacket -or
        $metadata.loop_record_offset -ne $track.LoopRecord) {
        throw "$($track.Name) metadata no longer matches its exact source-derived ADPCM payload."
    }
    $expectedSource = "BattleShip_o2r/audio/S1_music_sbk sequence $($track.Sequence) + B1_sounds1_ctl/tbl"
    if ($metadata.source -ne $expectedSource -or
        $metadata.tool -ne 'scripts/sfx/bgm/render-audio-bgm-pupupu.py') {
        throw "$($track.Name) source/tool provenance changed."
    }

    $magic = [Text.Encoding]::ASCII.GetString($data, 0, 4)
    if ($magic -ne 'BGA1' -or (Get-U16 $data 4) -ne 1 -or
        (Get-U16 $data 6) -ne 40 -or (Get-U32 $data 8) -ne 22050 -or
        (Get-U32 $data 12) -ne ($track.SourceBytes / 2) -or
        (Get-U32 $data 16) -ne $track.LoopSample -or
        (Get-U32 $data 20) -ne 16384 -or
        (Get-U32 $data 24) -ne $track.Packets -or
        (Get-U32 $data 28) -ne $track.LoopPacket -or
        (Get-U32 $data 32) -ne $track.LoopRecord -or
        (((Get-U32 $data 36) -band 1) -ne [int]$track.Looping)) {
        throw "$($track.Name) container header changed or is malformed."
    }

    [int64]$sampleTotal = 0
    $offset = 40
    for ($packet = 0; $packet -lt $track.Packets; $packet++) {
        if ($packet -eq $track.LoopPacket -and $offset -ne $track.LoopRecord) {
            throw "$($track.Name) loop record does not point at its loop packet."
        }
        $samples = Get-U32 $data $offset
        $payloadBytes = Get-U32 $data ($offset + 4)
        $expectedPayload = 4 + ([int][Math]::Ceiling($samples / 8.0) * 4)
        $payloadOffset = $offset + 8
        if ($samples -lt 1 -or $samples -gt 16384 -or
            $payloadBytes -ne $expectedPayload -or $payloadBytes -gt 8196 -or
            ($payloadBytes -band 3) -ne 0 -or
            ($payloadOffset + $payloadBytes) -gt $data.Length -or
            $data[$payloadOffset + 2] -gt 88 -or $data[$payloadOffset + 3] -ne 0) {
            throw "$($track.Name) packet $packet is malformed."
        }
        $sampleTotal += $samples
        $offset = $payloadOffset + $payloadBytes
    }
    if ($sampleTotal -ne ($track.SourceBytes / 2) -or $offset -ne $data.Length) {
        throw "$($track.Name) packet census does not exactly cover its source stream/container."
    }
    $compressedTotal += $data.Length
}

$header = Get-Content -LiteralPath (Join-Path $Root 'include/nds/nds_audio_bgm.h') -Raw
$runtime = Get-Content -LiteralPath (Join-Path $Root 'src/nds/nds_audio_bgm.c') -Raw
$required = @(
    'NDS_AUDIO_BGM_CONTAINER_MAGIC 0x31414742u',
    'NDS_AUDIO_BGM_CONTAINER_VERSION 1u',
    'NDS_AUDIO_BGM_CONTAINER_HEADER_BYTES 40u',
    'NDS_AUDIO_BGM_PACKET_SAMPLES 16384u',
    'NDS_AUDIO_BGM_PACKET_BYTES 8196u',
    'NDS_AUDIO_BGM_BUFFER_COUNT 2u',
    'NDS_AUDIO_BGM_PUPUPU_ASSET_BYTES 711920u',
    'NDS_AUDIO_BGM_WIN_MARIO_ASSET_BYTES 81860u',
    'NDS_AUDIO_BGM_WIN_FOX_ASSET_BYTES 72940u',
    'NDS_AUDIO_BGM_RESULTS_ASSET_BYTES 396588u',
    'NDS_AUDIO_BGM_MODE_SELECT_ASSET_BYTES 718212u',
    'NDS_AUDIO_BGM_BATTLE_SELECT_ASSET_BYTES 157372u'
)
foreach ($needle in $required) {
    if (-not $header.Contains($needle)) {
        throw "BGM runtime header is missing exact ADPCM constant: $needle"
    }
}
if ($compressedTotal -ne 2138892) {
    throw "ADPCM asset total changed: $compressedTotal"
}
if (-not $runtime.Contains('#define NDS_AUDIO_BGM_TIMER 0u') -or
    $runtime -match '#define NDS_AUDIO_BGM_TIMER [23]u') {
    throw 'BGM seam scheduling must not overwrite Calico cpuGetTiming timers 2/3.'
}

$makefile = Get-Content -LiteralPath (Join-Path $Root 'Makefile') -Raw
foreach ($obsolete in @('bgm_pupupu_pcm16.raw', 'bgm_win_mario_pcm16.raw',
        'bgm_win_fox_pcm16.raw', 'bgm_results_pcm16.raw')) {
    if (-not $makefile.Contains($obsolete)) {
        throw "Incremental NitroFS pruning lost obsolete asset: $obsolete"
    }
}
if ($makefile -notmatch '(?s)prune-obsolete-audio:\s*@rm -f .*NDS_AUDIO_OBSOLETE_DERIVED_FILES.*\$\(OUTPUT\)\.nds: prune-obsolete-audio') {
    throw 'Incremental builds can repack removed PCM BGM assets.'
}

Write-Output 'BattleShip-derived BGM ADPCM assets passed: tracks=0/12/16/22/44/10 compressed=2138892 source_pcm=8541792 resident=16392 packets=266.'
