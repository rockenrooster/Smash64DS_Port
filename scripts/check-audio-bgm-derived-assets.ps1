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
    # P2-1L bug (b2), 2026-08-19: re-rendered again after fixing render()/
    # decode_wave() to honor each wavetable's own ALADPCMloop sustain
    # region and this bank's ALEnvelope attack/decay/release data instead
    # of a flat "+2200 samples, 700-sample fade" tail on every note (see
    # nds_audio_bgm.h) -- Bytes/SourceBytes/Packets/LoopSample/LoopPacket/
    # LoopRecord unchanged, only content.
    [PSCustomObject]@{
        Name = 'Pupupu'; File = 'bgm_pupupu_ima.bin'; Sequence = 0
        Bytes = 711920; Sha256 = '431298f12745f3bde9801fb010e76fe5bc658c570267b4d2ba08703426f98d91'
        SourceBytes = 2843290; SourceSha256 = '52e7bc862cd0ce96b276b2f9bce3fd082d18493e6b9919f602b18bfdbad487f5'
        Packets = 88; Looping = $true; LoopSample = 4399; LoopPacket = 1; LoopRecord = 2252
    },
    [PSCustomObject]@{
        Name = 'Mario winner'; File = 'bgm_win_mario_ima.bin'; Sequence = 12
        Bytes = 81860; Sha256 = '24880278ce38f9e0998296859320f526cb8128cfd94e4fad5779e67bbfd3eebb'
        SourceBytes = 326800; SourceSha256 = '9993f4ae91982df72e055f4c73019aa27c94b63f2c009cc88238cd99ca3f0a9b'
        Packets = 10; Looping = $false; LoopSample = [uint32]::MaxValue; LoopPacket = [uint32]::MaxValue; LoopRecord = 0
    },
    [PSCustomObject]@{
        Name = 'Fox winner'; File = 'bgm_win_fox_ima.bin'; Sequence = 16
        Bytes = 72940; Sha256 = '5880a4df609df643f406321a337db9fdd63c2efe8b20460799b7f5c1e9e3f999'
        SourceBytes = 291154; SourceSha256 = 'e97553d21148711a73d7d6c2f8e70d356b119d9070213f1f707fb9a065075ffb'
        Packets = 9; Looping = $false; LoopSample = [uint32]::MaxValue; LoopPacket = [uint32]::MaxValue; LoopRecord = 0
    },
    # P2-1L bug (b1)+(b2): same re-renders as Pupupu above, same reasons
    # (LoopSample/LoopPacket/LoopRecord unchanged both times).
    [PSCustomObject]@{
        Name = 'Results'; File = 'bgm_results_ima.bin'; Sequence = 22
        Bytes = 396588; Sha256 = '476f66508bd498f88c62a479f7f29e334137436550de45c92fcbaf42be577d04'
        SourceBytes = 1583786; SourceSha256 = 'ad0234ea446e6c2587079a40226a7e34d411c18ae05392796c2349d7a6528884'
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
    # P2-1L bug (b2): wavetable-loop + ADSR-envelope render fix, same as
    # Pupupu above -- LoopSample/LoopPacket/LoopRecord unchanged this time.
    [PSCustomObject]@{
        Name = 'Mode select'; File = 'bgm_mode_select_ima.bin'; Sequence = 44
        Bytes = 718212; Sha256 = '140ae20c342e70810d37b9176eaf0e3d60361ef746a9509a721e6b7c8e0fc895'
        SourceBytes = 2868410; SourceSha256 = 'a79c75dadd9a25f8ef03896116b129330c0fa286439c5d476e6d4ba7a6883d9e'
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
    # P2-1L bug (b2): the surviving quiet patch was channel 14's one long
    # sustained note running out of un-looped wavetable samples at 13958
    # of its 33249-sample duration; decode_wave()/render() now honor the
    # wavetable's own ALADPCMloop and this bank's ALEnvelope releases
    # (mostly 25-30 ms here) instead of the old flat tail. LoopSample/
    # LoopPacket/LoopRecord unchanged again.
    [PSCustomObject]@{
        Name = 'Battle select'; File = 'bgm_battle_select_ima.bin'; Sequence = 10
        Bytes = 157372; Sha256 = '043459ff9a78d6e1cbcc41c370dbbcae746068ef0c1c0993f3092ffa60c50b7e'
        SourceBytes = 628352; SourceSha256 = '53f26e8d9e574ef5e1587575076e8ea48411c240a3a0fd9bafaa6be9c6497abf'
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
