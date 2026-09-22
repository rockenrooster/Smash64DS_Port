param(
    [string]$Root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
)
$ErrorActionPreference = 'Stop'
# 2026-09-14: scale-12 VADPCM + tempo-map pins; see docs/p2/BUG_NOTES.md section "Inishie and Yoster garble: the VADPCM decoder zeroes scale-12 frames".

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
        Bytes = 711920; Sha256 = '3a9963687cd2a6dad8e0cc07c880b7f02a97076ce17847c4d6bf5869efc11f7d'
        SourceBytes = 2843290; SourceSha256 = 'f80c025e0b106ca7f18604a6555967cf31d0f463d24240f7ce689c05120556b9'
        Packets = 88; Looping = $true; LoopSample = 4399; LoopPacket = 1; LoopRecord = 2252
    },
    [PSCustomObject]@{
        Name = 'Mario winner'; File = 'bgm_win_mario_ima.bin'; Sequence = 12
        Bytes = 81860; Sha256 = 'ccd64ccf648a600c3ca936ff33177d51537884e43463df469927ebe3bf893707'
        SourceBytes = 326800; SourceSha256 = '9b2c2088f47346e33511ca40aa5692b59ac9c26cad2ac535c320d1c50a5ba8cf'
        Packets = 10; Looping = $false; LoopSample = [uint32]::MaxValue; LoopPacket = [uint32]::MaxValue; LoopRecord = 0
    },
    [PSCustomObject]@{
        Name = 'Fox winner'; File = 'bgm_win_fox_ima.bin'; Sequence = 16
        Bytes = 72940; Sha256 = 'd5ab2a210ff071c9f11e701eb7d53aa9e7bf181748084c5691b86fdb7532c420'
        SourceBytes = 291154; SourceSha256 = '1b00847808250a4522bb047b52b2a91575ab6b9fb0cd5b46f374e07c1552a4ff'
        Packets = 9; Looping = $false; LoopSample = [uint32]::MaxValue; LoopPacket = [uint32]::MaxValue; LoopRecord = 0
    },
    # P2-1L bug (b1)+(b2): same re-renders as Pupupu above, same reasons
    # (LoopSample/LoopPacket/LoopRecord unchanged both times).
    [PSCustomObject]@{
        Name = 'Results'; File = 'bgm_results_ima.bin'; Sequence = 22
        Bytes = 396588; Sha256 = '16d2992b1761d00740f2bdd71684fd294d68fb0a0e82bdae9dcd38ce8b8f2d67'
        SourceBytes = 1583786; SourceSha256 = '528bd71dfe6b22d43513d3925392983970cb88d1a91f457f1a56ad37e7ef4de1'
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
        Bytes = 718212; Sha256 = '61906d77bc3e2865b6ba390f09ba0b376cc8d228c33f2de4fa1f49e9ba73fadf'
        SourceBytes = 2868410; SourceSha256 = '65e11c40146c3f477128b5c5ec503e61378461d8d34c37dd6691652f5bc319c5'
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
        Bytes = 157372; Sha256 = '81cb182cfe39e10b6ef67d03e25d918ea4b6869b9c019810a9bcb27ebebd185b'
        SourceBytes = 628352; SourceSha256 = '934113d37714601241181341a516c830769a61bcf7fe13e705ea9274f901f62b'
        Packets = 20; Looping = $true; LoopSample = 46228; LoopPacket = 3; LoopRecord = 23192
    }
)

function Get-U16([byte[]]$Data, [int]$Offset) {
    return [BitConverter]::ToUInt16($Data, $Offset)
}

function Get-U32([byte[]]$Data, [int]$Offset) {
    return [BitConverter]::ToUInt32($Data, $Offset)
}

# The BGA1 header and packet census the runtime's ndsAudioBgmReadHeader and
# ndsAudioBgmReadPacket enforce, for any track object carrying Name,
# SourceBytes, LoopSample, Packets, LoopPacket, LoopRecord and Looping.
function Assert-BgmContainer($Track, [byte[]]$Data) {
    $magic = [Text.Encoding]::ASCII.GetString($Data, 0, 4)
    if ($magic -ne 'BGA1' -or (Get-U16 $Data 4) -ne 1 -or
        (Get-U16 $Data 6) -ne 40 -or (Get-U32 $Data 8) -ne 22050 -or
        (Get-U32 $Data 12) -ne ($Track.SourceBytes / 2) -or
        (Get-U32 $Data 16) -ne $Track.LoopSample -or
        (Get-U32 $Data 20) -ne 16384 -or
        (Get-U32 $Data 24) -ne $Track.Packets -or
        (Get-U32 $Data 28) -ne $Track.LoopPacket -or
        (Get-U32 $Data 32) -ne $Track.LoopRecord -or
        (((Get-U32 $Data 36) -band 1) -ne [int]$Track.Looping)) {
        throw "$($Track.Name) container header changed or is malformed."
    }

    [int64]$sampleTotal = 0
    $offset = 40
    for ($packet = 0; $packet -lt $Track.Packets; $packet++) {
        if ($packet -eq $Track.LoopPacket -and $offset -ne $Track.LoopRecord) {
            throw "$($Track.Name) loop record does not point at its loop packet."
        }
        $samples = Get-U32 $Data $offset
        $payloadBytes = Get-U32 $Data ($offset + 4)
        $expectedPayload = 4 + ([int][Math]::Ceiling($samples / 8.0) * 4)
        $payloadOffset = $offset + 8
        if ($samples -lt 1 -or $samples -gt 16384 -or
            $payloadBytes -ne $expectedPayload -or $payloadBytes -gt 8196 -or
            ($payloadBytes -band 3) -ne 0 -or
            ($payloadOffset + $payloadBytes) -gt $Data.Length -or
            $Data[$payloadOffset + 2] -gt 88 -or $Data[$payloadOffset + 3] -ne 0) {
            throw "$($Track.Name) packet $packet is malformed."
        }
        $sampleTotal += $samples
        $offset = $payloadOffset + $payloadBytes
    }
    if ($sampleTotal -ne ($Track.SourceBytes / 2) -or $offset -ne $Data.Length) {
        throw "$($Track.Name) packet census does not exactly cover its source stream/container."
    }
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
    $expectedTool = 'scripts/sfx/bgm/render-audio-bgm.py'
    if ($metadata.source -ne $expectedSource -or $metadata.tool -ne $expectedTool) {
        throw "$($track.Name) source/tool provenance changed."
    }
    if ($track.Sequence -eq 0 -and
        ($metadata.pitch_bend_range_controller -ne 20 -or
         $metadata.pitch_bend_events_applied -ne 3 -or
         $metadata.pitch_bend_max_abs_cents -ne 4)) {
        throw 'Pupupu pitch-bend witness changed: expected 3 applied events, max 4 cents.'
    }

    Assert-BgmContainer $track $data
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
    'NDS_AUDIO_BGM_PUPUPU_STREAM_SHA256_LO 0x120556b9u',
    'NDS_AUDIO_BGM_PUPUPU_ASSET_SHA256_LO 0xefc11f7du',
    'NDS_AUDIO_BGM_ZEBES_STREAM_SHA256_LO 0xebaae72du',
    'NDS_AUDIO_BGM_ZEBES_ASSET_SHA256_LO 0x62b6ab33u',
    'NDS_AUDIO_BGM_INISHIE_ASSET_BYTES 981212u',
    'NDS_AUDIO_BGM_INISHIE_ASSET_SHA256_LO 0xf7d1ed1eu',
    'NDS_AUDIO_BGM_INISHIE_PACKET_COUNT 121u',
    'NDS_AUDIO_BGM_INISHIE_LOOP_PACKET 4u',
    'NDS_AUDIO_BGM_INISHIE_LOOP_RECORD 28672u',
    'NDS_AUDIO_BGM_SECTOR_STREAM_SHA256_LO 0x0aa83296u',
    'NDS_AUDIO_BGM_SECTOR_ASSET_SHA256_LO 0x31aea1e4u',
    'NDS_AUDIO_BGM_JUNGLE_STREAM_SHA256_LO 0xcaf359e8u',
    'NDS_AUDIO_BGM_JUNGLE_ASSET_SHA256_LO 0x3b6a7877u',
    'NDS_AUDIO_BGM_CASTLE_STREAM_SHA256_LO 0xffcf78b8u',
    'NDS_AUDIO_BGM_CASTLE_ASSET_SHA256_LO 0x8bcb2489u',
    'NDS_AUDIO_BGM_YAMABUKI_STREAM_SHA256_LO 0x6f39d8b0u',
    'NDS_AUDIO_BGM_YAMABUKI_ASSET_SHA256_LO 0x7a79d86cu',
    'NDS_AUDIO_BGM_HYRULE_STREAM_SHA256_LO 0x37cdbfa7u',
    'NDS_AUDIO_BGM_HYRULE_ASSET_SHA256_LO 0xde65fd67u',
    'NDS_AUDIO_BGM_WIN_MARIO_ASSET_BYTES 81860u',
    'NDS_AUDIO_BGM_WIN_FOX_ASSET_BYTES 72940u',
    'NDS_AUDIO_BGM_RESULTS_ASSET_BYTES 396588u',
    'NDS_AUDIO_BGM_MODE_SELECT_ASSET_BYTES 718212u',
    'NDS_AUDIO_BGM_BATTLE_SELECT_ASSET_BYTES 157372u',
    'NDS_AUDIO_BGM_YOSTER_ASSET_SHA256_LO 0x3b9e9569u',
    'NDS_AUDIO_BGM_INISHIE_HURRY_ASSET_BYTES 420548u',
    'NDS_AUDIO_BGM_INISHIE_HURRY_ASSET_SHA256_LO 0xab5240c5u',
    'NDS_AUDIO_BGM_INISHIE_HURRY_PACKET_COUNT 53u',
    'NDS_AUDIO_BGM_INISHIE_HURRY_LOOP_PACKET 9u',
    'NDS_AUDIO_BGM_INISHIE_HURRY_LOOP_RECORD 67056u'
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
# Owner, docs/BUGS.md Audio: "ALL BGM should be IMA-ADPCM". The PCM16 stream
# arm that carried Mushroom Kingdom from 2026-09-07 was retired 2026-09-22;
# no track may name a raw asset or prepare a PCM16 hardware channel again.
if ($runtime -match 'nitro:/audio/bgm_\w+\.raw' -or $runtime.Contains('SoundFmt_Pcm16')) {
    throw 'A BGM track streams raw PCM; every BGM track must be an IMA-ADPCM packet stream.'
}
if ($runtime -notmatch '\{\s*nSYAudioBGMInishie,\s*NDS_AUDIO_BGM_PATH_INISHIE,' -or
    -not $runtime.Contains('#define NDS_AUDIO_BGM_PATH_INISHIE "nitro:/audio/bgm_inishie_ima.bin"') -or
    $runtime -notmatch '\{\s*nSYAudioBGMInishieHurry,\s*NDS_AUDIO_BGM_PATH_INISHIE_HURRY,' -or
    -not $runtime.Contains('#define NDS_AUDIO_BGM_PATH_INISHIE_HURRY "nitro:/audio/bgm_inishie_hurry_ima.bin"')) {
    throw 'Mushroom Kingdom runtime rows do not stream the IMA pair.'
}

# 2026-09-09 stage census: pitch bend is source sequence state, not a global
# "Yoster has none" property. Keep a per-sequence generated witness beside the
# exact payload pins so an inert bend implementation cannot silently ship.
$stageRenderWitnesses = @(
    [PSCustomObject]@{ Name='Pupupu'; File='bgm_pupupu_ima.bin'; Sequence=0; Bytes=711920; Sha='3a9963687cd2a6dad8e0cc07c880b7f02a97076ce17847c4d6bf5869efc11f7d'; SourceSha='f80c025e0b106ca7f18604a6555967cf31d0f463d24240f7ce689c05120556b9'; Mix=22050; Master='127'; Bend=3; MaxCents=4 },
    [PSCustomObject]@{ Name='Zebes'; File='bgm_zebes_ima.bin'; Sequence=1; Bytes=617580; Sha='cfa8010b4105c7cb36b12913125ca879edd70c287b14b1831f39d29262b6ab33'; SourceSha='1c650f0f1ea8c7e312320f45a05e9dbda8b1102c53a92ea9127b7e10ebaae72d'; Mix=32000; Master='101'; Bend=0; MaxCents=0 },
    [PSCustomObject]@{ Name='Inishie'; File='bgm_inishie_ima.bin'; Sequence=2; Bytes=981212; Sha='1fa1c8bacc154361e05fc5d1d93da43dfb526b0bba982aa52fc8e495f7d1ed1e'; SourceSha='a1493ceef989f161971cd1bbb3840e1319a5e30c7e6d69f38fa42d458698ce72'; Mix=32000; Master='99'; Bend=0; MaxCents=0 },
    [PSCustomObject]@{ Name='Inishie Hurry'; File='bgm_inishie_hurry_ima.bin'; Sequence=3; Bytes=420548; Sha='a2ab007b4373acf6eeda5185aa8a3709e38c2c36c975939d7684640eab5240c5'; SourceSha='17162f6924a5e318115e47ec071847ef0611571af72ead90958373d4e2bdff1e'; Mix=32000; Master='99'; Bend=0; MaxCents=0 },
    [PSCustomObject]@{ Name='Sector'; File='bgm_sector_ima.bin'; Sequence=4; Bytes=1237984; Sha='b87d9391dced3e7a8729bd2fc3e91f53479fd48968dc87e8b5690cc031aea1e4'; SourceSha='718c470bad451d5b1cee6b2e3386bd365a9781214b18facf2462507c0aa83296'; Mix=32000; Master='97'; Bend=0; MaxCents=0 },
    [PSCustomObject]@{ Name='Jungle'; File='bgm_jungle_ima.bin'; Sequence=5; Bytes=3470676; Sha='16874880f744c36dd92af32d4cf64f16d7211371ddfd5b0129c918953b6a7877'; SourceSha='5a78a64bf44fdef6943600c97ffe5da37e38433f90ef4522c5e7c7e8caf359e8'; Mix=32000; Master='112'; Bend=89; MaxCents=99 },
    [PSCustomObject]@{ Name='Castle'; File='bgm_castle_ima.bin'; Sequence=6; Bytes=931400; Sha='414d84e0a9ae6c49ad99d7ec318cdfd7afb8ea55a3aabf40ddff3b5d8bcb2489'; SourceSha='9c4f2d0cd2b0e3a87f9223c8e3e0fb7b65237fe6c9f1e6dfdb9e123bffcf78b8'; Mix=32000; Master='106'; Bend=5; MaxCents=14 },
    [PSCustomObject]@{ Name='Yamabuki'; File='bgm_yamabuki_ima.bin'; Sequence=7; Bytes=629936; Sha='912918f803bfbaeb25786a950bb60922687e14224e673a6eda03cb0a7a79d86c'; SourceSha='017cde5d5379139936dc78154e3a687eea6b178d17d012a22522d2336f39d8b0'; Mix=32000; Master='100,113'; Bend=88; MaxCents=98 },
    [PSCustomObject]@{ Name='Yoster'; File='bgm_yoster_ima.bin'; Sequence=8; Bytes=652292; Sha='58cd811ea55ef13d29fd81a61dba55744a9f99c7ce991073e4d0680c3b9e9569'; SourceSha='45c42eb549e4262285ec6dcca42d23d6636d36175686af641cf1b81a759ed409'; Mix=32000; Master='86'; Bend=0; MaxCents=0 },
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

# Mushroom Kingdom, sequences 2 and 3 (owner, docs/BUGS.md Audio: "ALL BGM
# should be IMA-ADPCM"): IMA packet streams again since 2026-09-22, after the
# PCM16 streams of 2026-09-07/09. render-audio-bgm.py encodes exactly these
# two with its step-index Viterbi search (TRELLIS_IMA_SEQUENCES):
#   python scripts/sfx/bgm/render-audio-bgm.py --sequence-index 2 `
#          --output assets/audio/bgm_inishie_ima.bin
#   python scripts/sfx/bgm/render-audio-bgm.py --sequence-index 3 `
#          --output assets/audio/bgm_inishie_hurry_ima.bin
# Their source PCM is byte-identical to the retired PCM16 assets, so only the
# codec moved. Stage-gated, so conditional on the asset like Yoster below;
# the header pins above always apply. Each pins the payload, the container
# census the runtime header check enforces and the codec evidence (greedy
# measured 20.34 / 20.27 dB on the same PCM).
$inishieTracks = @(
    [PSCustomObject]@{
        Name = 'Inishie'; File = 'bgm_inishie_ima.bin'; Sequence = 2
        Bytes = 981212; Sha256 = '1fa1c8bacc154361e05fc5d1d93da43dfb526b0bba982aa52fc8e495f7d1ed1e'
        SourceBytes = 3918852; SourceSha256 = 'a1493ceef989f161971cd1bbb3840e1319a5e30c7e6d69f38fa42d458698ce72'
        Packets = 121; Looping = $true; LoopSample = 57161; LoopPacket = 4; LoopRecord = 28672
        SnrDb = 27.18556943369464; MaxError = 1525
    },
    [PSCustomObject]@{
        Name = 'Inishie Hurry'; File = 'bgm_inishie_hurry_ima.bin'; Sequence = 3
        Bytes = 420548; Sha256 = 'a2ab007b4373acf6eeda5185aa8a3709e38c2c36c975939d7684640eab5240c5'
        SourceBytes = 1679478; SourceSha256 = '17162f6924a5e318115e47ec071847ef0611571af72ead90958373d4e2bdff1e'
        Packets = 53; Looping = $true; LoopSample = 133813; LoopPacket = 9; LoopRecord = 67056
        SnrDb = 27.284038610851375; MaxError = 1757
    }
)
foreach ($track in $inishieTracks) {
    $asset = Join-Path $Root "assets/audio/$($track.File)"
    $metadataPath = [IO.Path]::ChangeExtension($asset, '.json')
    if (-not (Test-Path -LiteralPath $asset -PathType Leaf) -or
        -not (Test-Path -LiteralPath $metadataPath -PathType Leaf)) {
        continue
    }
    $data = [IO.File]::ReadAllBytes($asset)
    $sha = (Get-FileHash -LiteralPath $asset -Algorithm SHA256).Hash.ToLowerInvariant()
    $metadata = Get-Content -LiteralPath $metadataPath -Raw | ConvertFrom-Json
    if ($data.Length -ne $track.Bytes -or $sha -ne $track.Sha256 -or
        $metadata.sequence_index -ne $track.Sequence -or
        $metadata.bytes -ne $track.Bytes -or $metadata.sha256 -ne $track.Sha256 -or
        $metadata.source_pcm_bytes -ne $track.SourceBytes -or
        $metadata.source_pcm_sha256 -ne $track.SourceSha256 -or
        $metadata.format -ne 'Nintendo DS IMA-ADPCM packet stream' -or
        $metadata.packet_count -ne $track.Packets -or
        -not [bool]$metadata.looping -or
        $metadata.loop_start_byte -ne ($track.LoopSample * 2) -or
        $metadata.loop_packet_index -ne $track.LoopPacket -or
        $metadata.loop_record_offset -ne $track.LoopRecord -or
        $metadata.ima_encoder -ne 'viterbi-step-index' -or
        $metadata.ima_max_error -ne $track.MaxError -or
        [Math]::Abs([double]$metadata.ima_snr_db - $track.SnrDb) -gt 1e-9) {
        throw "$($track.Name) IMA payload/container/codec evidence changed."
    }
    if ($metadata.resample_method -ne 'completed 32k mix -> 22.05k 32-tap Lanczos-windowed sinc low-pass') {
        throw "$($track.Name) resampling provenance changed."
    }
    Assert-BgmContainer $track $data
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
        $yosterSha -ne '58cd811ea55ef13d29fd81a61dba55744a9f99c7ce991073e4d0680c3b9e9569' -or
        $yosterMetadata.sequence_index -ne 8 -or
        $yosterMetadata.source -ne 'BattleShip_o2r/audio/S1_music_sbk sequence 8 + B1_sounds1_ctl/tbl' -or
        $yosterMetadata.sequence_bank_binding -ne 'sSYAudioSequenceBank2 -> B1_sounds1_ctl/tbl' -or
        $yosterMetadata.source_pcm_sha256 -ne '45c42eb549e4262285ec6dcca42d23d6636d36175686af641cf1b81a759ed409' -or
        $yosterMetadata.loop_start_byte -ne 313088 -or
        $yosterMetadata.mix_sample_rate -ne 32000 -or
        $yosterMetadata.master_volume_controller -ne 21 -or
        @($yosterMetadata.master_volume_values).Count -ne 1 -or
        [int]@($yosterMetadata.master_volume_values)[0] -ne 86 -or
        $yosterMetadata.resample_method -ne 'completed 32k mix -> 22.05k 32-tap Lanczos-windowed sinc low-pass' -or
        [Math]::Abs([double]$yosterMetadata.ima_snr_db - 26.060505686403395) -gt 1e-9) {
        throw 'Yoster BGM payload/master-volume/codec evidence changed.'
    }
}

$makefile = Get-Content -LiteralPath (Join-Path $Root 'Makefile') -Raw
$obsoleteBlock = [regex]::Match($makefile,
    '(?s)export NDS_AUDIO_OBSOLETE_DERIVED_FILES :=(.*?)\n(?!\t)').Groups[1].Value
foreach ($obsolete in @('bgm_pupupu_pcm16.raw', 'bgm_win_mario_pcm16.raw',
        'bgm_win_fox_pcm16.raw', 'bgm_results_pcm16.raw',
        'bgm_inishie_pcm16.raw', 'bgm_inishie_hurry_pcm16.raw')) {
    if (-not $obsoleteBlock.Contains("audio/$obsolete")) {
        throw "Incremental NitroFS pruning lost obsolete asset: $obsolete"
    }
}
# A live asset in the prune list is deleted from NitroFS before every pack.
foreach ($live in @('bgm_inishie_ima.bin', 'bgm_inishie_hurry_ima.bin')) {
    if ($obsoleteBlock.Contains($live) -or
        -not $makefile.Contains("`taudio/$live") -or
        -not $makefile.Contains("`$(NITROFS_DIR)/audio/${live}: `$(PROJECT_ROOT)/assets/audio/$live")) {
        throw "Mushroom Kingdom IMA asset is not staged as a live NitroFS file: $live"
    }
}
if ($makefile -notmatch '(?s)prune-obsolete-audio:\s*@rm -f .*NDS_AUDIO_OBSOLETE_DERIVED_FILES.*\$\(OUTPUT\)\.nds: prune-obsolete-audio') {
    throw 'Incremental builds can repack removed PCM BGM assets.'
}

Write-Output 'BattleShip-derived BGM ADPCM assets passed: tracks=0/12/16/22/44/10 compressed=2138892 source_pcm=8541792 resident=16392 packets=266.'
