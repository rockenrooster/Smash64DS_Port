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
        Bytes = 711920; Sha256 = '5c8cb02e54f971df6177e35430295cd38ea922bdf33f92d3094c9027fc0b98e1'
        SourceBytes = 2843290; SourceSha256 = 'c2d048a32af21709610d9e48ef62ef1cee6b419c4ae90066090d292006ff5ab8'
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
        Bytes = 396588; Sha256 = '1373632644f9bb953ec63d5265543fd1d2a91dcf23252303a9fba9f16ba054c1'
        SourceBytes = 1583786; SourceSha256 = 'ca5b86b13606edb95e4c2e35dae068937d98382979227e47e1dc4d4eadc651d4'
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
        Bytes = 718212; Sha256 = '4d4f400a2668555ab1c88fd7376214aa899fc5d41350e982c2d654138c7ac2c6'
        SourceBytes = 2868410; SourceSha256 = 'c1df61e1d1359af01ab908418e2982d9f0365914262f72b037bb99e51d34cae1'
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
        Bytes = 157372; Sha256 = '127c5a6bdf2f4eb3162952850ca6ac1e65c48988491cd72622700dafb38a7cff'
        SourceBytes = 628352; SourceSha256 = '338d528263d926cc5c43bed1c822865ba1da6689edd43b5e3377cd1f546a98c4'
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
    $expectedTool = if ($track.Sequence -eq 0) {
        'scripts/sfx/bgm/render-audio-bgm.py'
    } else {
        'scripts/sfx/bgm/render-audio-bgm-pupupu.py'
    }
    if ($metadata.source -ne $expectedSource -or $metadata.tool -ne $expectedTool) {
        throw "$($track.Name) source/tool provenance changed."
    }
    if ($track.Sequence -eq 0 -and
        ($metadata.pitch_bend_range_controller -ne 20 -or
         $metadata.pitch_bend_events_applied -ne 3 -or
         $metadata.pitch_bend_max_abs_cents -ne 4)) {
        throw 'Pupupu pitch-bend witness changed: expected 3 applied events, max 4 cents.'
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
    'NDS_AUDIO_BGM_PUPUPU_STREAM_SHA256_LO 0x06ff5ab8u',
    'NDS_AUDIO_BGM_PUPUPU_ASSET_SHA256_LO 0xfc0b98e1u',
    'NDS_AUDIO_BGM_ZEBES_STREAM_SHA256_LO 0x8575d142u',
    'NDS_AUDIO_BGM_ZEBES_ASSET_SHA256_LO 0xecbbab64u',
    'NDS_AUDIO_BGM_INISHIE_ASSET_SHA256_LO 0x9d3e1b5bu',
    'NDS_AUDIO_BGM_SECTOR_STREAM_SHA256_LO 0x0aa83296u',
    'NDS_AUDIO_BGM_SECTOR_ASSET_SHA256_LO 0x31aea1e4u',
    'NDS_AUDIO_BGM_JUNGLE_STREAM_SHA256_LO 0x2da9efd9u',
    'NDS_AUDIO_BGM_JUNGLE_ASSET_SHA256_LO 0xb59bd93bu',
    'NDS_AUDIO_BGM_CASTLE_STREAM_SHA256_LO 0xc3fe25e7u',
    'NDS_AUDIO_BGM_CASTLE_ASSET_SHA256_LO 0xabc2c2e2u',
    'NDS_AUDIO_BGM_YAMABUKI_STREAM_SHA256_LO 0xadab1b58u',
    'NDS_AUDIO_BGM_YAMABUKI_ASSET_SHA256_LO 0x7a485484u',
    'NDS_AUDIO_BGM_HYRULE_STREAM_SHA256_LO 0x37cdbfa7u',
    'NDS_AUDIO_BGM_HYRULE_ASSET_SHA256_LO 0xde65fd67u',
    'NDS_AUDIO_BGM_WIN_MARIO_ASSET_BYTES 81860u',
    'NDS_AUDIO_BGM_WIN_FOX_ASSET_BYTES 72940u',
    'NDS_AUDIO_BGM_RESULTS_ASSET_BYTES 396588u',
    'NDS_AUDIO_BGM_MODE_SELECT_ASSET_BYTES 718212u',
    'NDS_AUDIO_BGM_BATTLE_SELECT_ASSET_BYTES 157372u',
    'NDS_AUDIO_BGM_FORMAT_PCM16 1u',
    'NDS_AUDIO_BGM_PCM16_CHUNK_SAMPLES 4098u',
    'NDS_AUDIO_BGM_PCM16_CHUNK_BYTES 8196u',
    'NDS_AUDIO_BGM_INISHIE_PCM16_ASSET_BYTES 3918852u',
    'NDS_AUDIO_BGM_INISHIE_PCM16_ASSET_SHA256_LO 0x80a5b000u',
    'NDS_AUDIO_BGM_INISHIE_PCM16_PACKET_COUNT 479u',
    'NDS_AUDIO_BGM_INISHIE_PCM16_LOOP_PACKET 14u',
    'NDS_AUDIO_BGM_INISHIE_PCM16_LOOP_RECORD 114322u',
    'NDS_AUDIO_BGM_YOSTER_ASSET_SHA256_LO 0x32a14852u',
    'NDS_AUDIO_BGM_INISHIE_HURRY_ASSET_BYTES 1803860u',
    'NDS_AUDIO_BGM_INISHIE_HURRY_ASSET_SHA256_LO 0x3155659cu',
    'NDS_AUDIO_BGM_INISHIE_HURRY_PACKET_COUNT 221u',
    'NDS_AUDIO_BGM_INISHIE_HURRY_LOOP_PACKET 48u',
    'NDS_AUDIO_BGM_INISHIE_HURRY_LOOP_RECORD 392008u'
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
if (-not $runtime.Contains('gNdsAudioBgmPcm16UnderrunCount')) {
    throw 'BGM PCM16 refill witness counter is missing.'
}

# 2026-09-09 stage census: pitch bend is source sequence state, not a global
# "Yoster has none" property. Keep a per-sequence generated witness beside the
# exact payload pins so an inert bend implementation cannot silently ship.
$stageRenderWitnesses = @(
    [PSCustomObject]@{ Name='Pupupu'; File='bgm_pupupu_ima.bin'; Sequence=0; Bytes=711920; Sha='5c8cb02e54f971df6177e35430295cd38ea922bdf33f92d3094c9027fc0b98e1'; SourceSha='c2d048a32af21709610d9e48ef62ef1cee6b419c4ae90066090d292006ff5ab8'; Mix=22050; Master='127'; Bend=3; MaxCents=4 },
    [PSCustomObject]@{ Name='Zebes'; File='bgm_zebes_ima.bin'; Sequence=1; Bytes=617580; Sha='3dbd4e036a6dbd87a8850ecaee795c3cb881f5e43b7eba9321ed4932ecbbab64'; SourceSha='6f47614ff0166f34cdab1f18790f63bdf36c0a04d60f9b56f72832f38575d142'; Mix=32000; Master='101'; Bend=0; MaxCents=0 },
    [PSCustomObject]@{ Name='Inishie'; File='bgm_inishie_ima.bin'; Sequence=2; Bytes=981212; Sha='bfe60516f483f9ab51ebb9f97bad21ea980d3c3e3281178b7e4249189d3e1b5b'; SourceSha='405d22f945e63206b87d32ad1eeae8d99e54600027f5d9064a4089e980a5b000'; Mix=32000; Master='99'; Bend=0; MaxCents=0 },
    [PSCustomObject]@{ Name='Sector'; File='bgm_sector_ima.bin'; Sequence=4; Bytes=1237984; Sha='b87d9391dced3e7a8729bd2fc3e91f53479fd48968dc87e8b5690cc031aea1e4'; SourceSha='718c470bad451d5b1cee6b2e3386bd365a9781214b18facf2462507c0aa83296'; Mix=32000; Master='97'; Bend=0; MaxCents=0 },
    [PSCustomObject]@{ Name='Jungle'; File='bgm_jungle_ima.bin'; Sequence=5; Bytes=2923840; Sha='56c29b4ff65cbe4dd4d54fed8adc0cf17b5356947177bd91a8878b52b59bd93b'; SourceSha='e6d10de835faa0fe8707f068a546203fe9f1b43564782ee154c413042da9efd9'; Mix=32000; Master='112'; Bend=89; MaxCents=99 },
    [PSCustomObject]@{ Name='Castle'; File='bgm_castle_ima.bin'; Sequence=6; Bytes=931400; Sha='fa644bba9d0b0cc4c223a5b014d41085904867364938a3ddad896c1babc2c2e2'; SourceSha='76b1facb31c81b52b43bc816d3281a11c830ec78fb8c0b26b6753d56c3fe25e7'; Mix=32000; Master='106'; Bend=5; MaxCents=14 },
    [PSCustomObject]@{ Name='Yamabuki'; File='bgm_yamabuki_ima.bin'; Sequence=7; Bytes=583596; Sha='3c285befeacbcc6dd981afa14174f5fce146238daed18aad6e63c63d7a485484'; SourceSha='7573f7f12e64a5aec5ef8e2ec807db05e054565a8fd3f871a86e4f7dadab1b58'; Mix=32000; Master='100,113'; Bend=88; MaxCents=98 },
    [PSCustomObject]@{ Name='Yoster'; File='bgm_yoster_ima.bin'; Sequence=8; Bytes=652292; Sha='1e2e989c3ab2e3147772ea78c641b9b016952af08ae17634cc6aa36132a14852'; SourceSha='e794fa1882ddf0624dc57d451111731351efea37862a2a907ea609be983aeb7a'; Mix=32000; Master='86'; Bend=0; MaxCents=0 },
    [PSCustomObject]@{ Name='Hyrule'; File='bgm_hyrule_ima.bin'; Sequence=9; Bytes=471132; Sha='1f8bdbef881d2c9f99a44566d945c473a1f7eab4d48619b9fa0a34a0de65fd67'; SourceSha='7893eea38d9b12ded637efb0eacce334d745f6bb85ce9e529d82844537cdbfa7'; Mix=32000; Master='108'; Bend=0; MaxCents=0 }
)
foreach ($stage in $stageRenderWitnesses) {
    $stageAsset = Join-Path $Root "assets/audio/$($stage.File)"
    $stageMetadataPath = [IO.Path]::ChangeExtension($stageAsset, '.json')
    if (-not (Test-Path -LiteralPath $stageAsset -PathType Leaf) -or
        -not (Test-Path -LiteralPath $stageMetadataPath -PathType Leaf)) {
        continue
    }
    $stageSha = (Get-FileHash -LiteralPath $stageAsset -Algorithm SHA256).Hash.ToLowerInvariant()
    $stageMetadata = Get-Content -LiteralPath $stageMetadataPath -Raw | ConvertFrom-Json
    $stageMaster = (@($stageMetadata.master_volume_values) | ForEach-Object { [string][int]$_ }) -join ','
    $expectedSource = "BattleShip_o2r/audio/S1_music_sbk sequence $($stage.Sequence) + B1_sounds1_ctl/tbl"
    if ((Get-Item -LiteralPath $stageAsset).Length -ne $stage.Bytes -or
        $stageSha -ne $stage.Sha -or
        $stageMetadata.sequence_index -ne $stage.Sequence -or
        $stageMetadata.sha256 -ne $stage.Sha -or
        $stageMetadata.source_pcm_sha256 -ne $stage.SourceSha -or
        $stageMetadata.source -ne $expectedSource -or
        $stageMetadata.tool -ne 'scripts/sfx/bgm/render-audio-bgm.py' -or
        $stageMetadata.sequence_bank_binding -ne 'sSYAudioSequenceBank2 -> B1_sounds1_ctl/tbl' -or
        $stageMetadata.mix_sample_rate -ne $stage.Mix -or
        $stageMetadata.master_volume_controller -ne 21 -or
        $stageMaster -ne $stage.Master -or
        $stageMetadata.pitch_bend_range_controller -ne 20 -or
        $stageMetadata.pitch_bend_events_applied -ne $stage.Bend -or
        $stageMetadata.pitch_bend_max_abs_cents -ne $stage.MaxCents) {
        throw "$($stage.Name) stage BGM payload/provenance/pitch-bend witness changed."
    }
}

# Inishie PCM16 raw asset (sequence 2, Mushroom Kingdom): rendered offline with
#   python scripts/sfx/bgm/render-audio-bgm.py --sequence-index 2 `
#          --format pcm16 --output assets/audio/bgm_inishie_pcm16.raw
# Conditional until the asset lands: header pins above always apply; file
# bytes/format below apply once rendered.
$pcm16Asset = Join-Path $Root 'assets/audio/bgm_inishie_pcm16.raw'
$pcm16MetadataPath = [IO.Path]::ChangeExtension($pcm16Asset, '.json')
if ((Test-Path -LiteralPath $pcm16Asset -PathType Leaf) -and
    (Test-Path -LiteralPath $pcm16MetadataPath -PathType Leaf)) {
    $pcm16Data = [IO.File]::ReadAllBytes($pcm16Asset)
    $pcm16Sha = (Get-FileHash -LiteralPath $pcm16Asset -Algorithm SHA256).Hash.ToLowerInvariant()
    $pcm16Metadata = Get-Content -LiteralPath $pcm16MetadataPath -Raw | ConvertFrom-Json
    if ($pcm16Data.Length -ne 3918852 -or
        $pcm16Sha -ne '405d22f945e63206b87d32ad1eeae8d99e54600027f5d9064a4089e980a5b000' -or
        $pcm16Metadata.sequence_index -ne 2 -or
        $pcm16Metadata.bytes -ne 3918852 -or
        $pcm16Metadata.source_pcm_bytes -ne 3918852 -or
        $pcm16Metadata.sha256 -ne $pcm16Sha -or
        $pcm16Metadata.source_pcm_sha256 -ne $pcm16Sha -or
        $pcm16Metadata.sample_rate -ne 22050 -or
        $pcm16Metadata.mix_sample_rate -ne 32000 -or
        $pcm16Metadata.format -ne 'signed PCM16LE mono raw' -or
        $pcm16Metadata.loop_start_byte -ne 114322) {
        throw 'Inishie PCM16 payload changed: bytes/format/sequence/loop mismatch.'
    }
    if ($pcm16Metadata.tool -ne 'scripts/sfx/bgm/render-audio-bgm.py' -or
        $pcm16Metadata.source -ne 'BattleShip_o2r/audio/S1_music_sbk sequence 2 + B1_sounds1_ctl/tbl' -or
        $pcm16Metadata.sequence_bank_binding -ne 'sSYAudioSequenceBank2 -> B1_sounds1_ctl/tbl' -or
        $pcm16Metadata.master_volume_controller -ne 21 -or
        @($pcm16Metadata.master_volume_values).Count -ne 1 -or
        [int]@($pcm16Metadata.master_volume_values)[0] -ne 99 -or
        $pcm16Metadata.resample_method -ne 'completed 32k mix -> 22.05k 32-tap Lanczos-windowed sinc low-pass') {
        throw 'Inishie PCM16 source/tool provenance changed.'
    }
}

# The two stage tracks repaired on 2026-09-09 are checked against their exact
# generated payloads when present. They are stage-gated assets, so keep the
# same conditional shape as Inishie above rather than making an unrelated
# flag-off asset checkout fail this checker.
$yosterAsset = Join-Path $Root 'assets/audio/bgm_yoster_ima.bin'
$yosterMetadataPath = [IO.Path]::ChangeExtension($yosterAsset, '.json')
if ((Test-Path -LiteralPath $yosterAsset -PathType Leaf) -and
    (Test-Path -LiteralPath $yosterMetadataPath -PathType Leaf)) {
    $yosterSha = (Get-FileHash -LiteralPath $yosterAsset -Algorithm SHA256).Hash.ToLowerInvariant()
    $yosterMetadata = Get-Content -LiteralPath $yosterMetadataPath -Raw | ConvertFrom-Json
    if ((Get-Item -LiteralPath $yosterAsset).Length -ne 652292 -or
        $yosterSha -ne '1e2e989c3ab2e3147772ea78c641b9b016952af08ae17634cc6aa36132a14852' -or
        $yosterMetadata.sequence_index -ne 8 -or
        $yosterMetadata.source -ne 'BattleShip_o2r/audio/S1_music_sbk sequence 8 + B1_sounds1_ctl/tbl' -or
        $yosterMetadata.sequence_bank_binding -ne 'sSYAudioSequenceBank2 -> B1_sounds1_ctl/tbl' -or
        $yosterMetadata.source_pcm_sha256 -ne 'e794fa1882ddf0624dc57d451111731351efea37862a2a907ea609be983aeb7a' -or
        $yosterMetadata.loop_start_byte -ne 313088 -or
        $yosterMetadata.mix_sample_rate -ne 32000 -or
        $yosterMetadata.master_volume_controller -ne 21 -or
        @($yosterMetadata.master_volume_values).Count -ne 1 -or
        [int]@($yosterMetadata.master_volume_values)[0] -ne 86 -or
        $yosterMetadata.resample_method -ne 'completed 32k mix -> 22.05k 32-tap Lanczos-windowed sinc low-pass' -or
        [Math]::Abs([double]$yosterMetadata.ima_snr_db - 26.171703980043457) -gt 1e-9) {
        throw 'Yoster BGM payload/master-volume/codec evidence changed.'
    }
}

$hurryAsset = Join-Path $Root 'assets/audio/bgm_inishie_hurry_pcm16.raw'
$hurryMetadataPath = [IO.Path]::ChangeExtension($hurryAsset, '.json')
if ((Test-Path -LiteralPath $hurryAsset -PathType Leaf) -and
    (Test-Path -LiteralPath $hurryMetadataPath -PathType Leaf)) {
    $hurrySha = (Get-FileHash -LiteralPath $hurryAsset -Algorithm SHA256).Hash.ToLowerInvariant()
    $hurryMetadata = Get-Content -LiteralPath $hurryMetadataPath -Raw | ConvertFrom-Json
    if ((Get-Item -LiteralPath $hurryAsset).Length -ne 1803860 -or
        $hurrySha -ne '98354125ce7a8a760309311ed4e3a4ae479bd0a9b0c33e9a9aacdc463155659c' -or
        $hurryMetadata.sequence_index -ne 3 -or
        $hurryMetadata.source -ne 'BattleShip_o2r/audio/S1_music_sbk sequence 3 + B1_sounds1_ctl/tbl' -or
        $hurryMetadata.sequence_bank_binding -ne 'sSYAudioSequenceBank2 -> B1_sounds1_ctl/tbl' -or
        $hurryMetadata.format -ne 'signed PCM16LE mono raw' -or
        $hurryMetadata.sha256 -ne $hurrySha -or
        $hurryMetadata.source_pcm_sha256 -ne $hurrySha -or
        $hurryMetadata.loop_start_byte -ne 392008 -or
        $hurryMetadata.mix_sample_rate -ne 32000 -or
        $hurryMetadata.master_volume_controller -ne 21 -or
        @($hurryMetadata.master_volume_values).Count -ne 1 -or
        [int]@($hurryMetadata.master_volume_values)[0] -ne 99 -or
        $hurryMetadata.resample_method -ne 'completed 32k mix -> 22.05k 32-tap Lanczos-windowed sinc low-pass') {
        throw 'Inishie Hurry PCM16 payload/master-volume evidence changed.'
    }
    if ($runtime -notmatch '(?s)nSYAudioBGMInishieHurry.*?NDS_AUDIO_BGM_FORMAT_PCM16' -or
        -not $runtime.Contains('nitro:/audio/bgm_inishie_hurry_pcm16.raw')) {
        throw 'Inishie Hurry runtime row is not the PCM16 stream.'
    }
}

$makefile = Get-Content -LiteralPath (Join-Path $Root 'Makefile') -Raw
foreach ($obsolete in @('bgm_pupupu_pcm16.raw', 'bgm_win_mario_pcm16.raw',
        'bgm_win_fox_pcm16.raw', 'bgm_results_pcm16.raw',
        'bgm_inishie_hurry_ima.bin')) {
    if (-not $makefile.Contains($obsolete)) {
        throw "Incremental NitroFS pruning lost obsolete asset: $obsolete"
    }
}
if ($makefile -notmatch '(?s)prune-obsolete-audio:\s*@rm -f .*NDS_AUDIO_OBSOLETE_DERIVED_FILES.*\$\(OUTPUT\)\.nds: prune-obsolete-audio') {
    throw 'Incremental builds can repack removed PCM BGM assets.'
}

Write-Output 'BattleShip-derived BGM ADPCM assets passed: tracks=0/12/16/22/44/10 compressed=2138892 source_pcm=8541792 resident=16392 packets=266.'
