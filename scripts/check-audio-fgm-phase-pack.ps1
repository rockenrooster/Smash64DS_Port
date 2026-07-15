param(
    [string]$Python = 'python'
)

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$generator = Join-Path $PSScriptRoot 'render-audio-fgm-phase-pack.py'
$verifierPath = Join-Path $PSScriptRoot 'verify-audio-fgm-phase-pack.ps1'
$metadataPath = Join-Path $root 'assets/audio/fgm_phase_pack_ima.json'

if (-not (Test-Path -LiteralPath $generator -PathType Leaf)) {
    throw "FGM phase-pack generator not found: $generator"
}
if ($null -eq (Get-Command $Python -ErrorAction SilentlyContinue)) {
    throw "Python command not found: $Python"
}
$verifier = Get-Content -LiteralPath $verifierPath -Raw
if (($verifier -notmatch '-MuteAudio') -or
    ($verifier -notmatch 'Audio FGM verification must use isolated runner slot')) {
    throw 'Audio FGM verifier lost its host-mute or isolated-runner guard.'
}

& $Python $generator --repo-root $root --check
if ($LASTEXITCODE -ne 0) {
    throw 'The generated FGM phase pack differs from its BattleShip sources.'
}

$metadata = Get-Content -LiteralPath $metadataPath -Raw | ConvertFrom-Json
$expectedIDs = @(626, 470, 469, 467, 490, 439, 292, 370, 289, 154)
$actualIDs = @($metadata.entries | ForEach-Object { [int]$_.id })
if (($actualIDs -join ',') -ne ($expectedIDs -join ',')) {
    throw "Unexpected FGM mapping: $($actualIDs -join ',')"
}
if ([int64]$metadata.resident_bytes -gt 65536) {
    throw "FGM pack exceeds 64 KiB: $($metadata.resident_bytes)"
}

$phaseIDs = @($metadata.entries | Where-Object {
        $_.entry_kind -eq 'phase'
    } | ForEach-Object { [int]$_.id })
$koIDs = @($metadata.entries | Where-Object {
        $_.entry_kind -eq 'ko'
    } | ForEach-Object { [int]$_.id })
if (($phaseIDs -join ',') -ne '626,470,469,467,490') {
    throw "Unexpected FGM phase subset: $($phaseIDs -join ',')"
}
if (($koIDs -join ',') -ne '439,292,370,289,154') {
    throw "Unexpected regular-KO subset: $($koIDs -join ',')"
}
if (([int]$metadata.unique_sample_count -ne 9) -or
    ([int]$metadata.unique_sample_bytes -ne 64304)) {
    throw 'Regular-KO sample deduplication fixture changed.'
}
if (([int]$metadata.format_version -ne 2) -or
    ([int]$metadata.entry_bytes -ne 32) -or
    ([int]$metadata.envelope_point_bytes -ne 4)) {
    throw 'Unexpected FGM pack envelope format.'
}
if ([bool]$metadata.runtime_conversion) {
    throw 'FGM metadata says conversion still occurs at runtime.'
}

foreach ($entry in $metadata.entries) {
    if (([double]$entry.decoded_rms -le 0.0) -or
        ([int64]$entry.decoded_peak -le 0)) {
        throw "FGM $($entry.id) decoded waveform is silent."
    }
    if ([double]$entry.ima_snr_db -lt 20.0) {
        throw "FGM $($entry.id) IMA SNR fell below 20 dB: $($entry.ima_snr_db)"
    }
    if (([int64]$entry.ds_frequency_hz -le 0) -or
        ([int64]$entry.source_duration_microseconds -le 0)) {
        throw "FGM $($entry.id) has invalid runtime timing metadata."
    }
}

$publicExcited = $metadata.entries | Where-Object { [int]$_.id -eq 626 }
if ($publicExcited.ds_loop_strategy -ne
    'full_stream_is_exact_source_loop_range') {
    throw 'PublicExcited no longer preserves the source infinite loop range.'
}
if (([int]$publicExcited.source_loop_start -ne 1) -or
    ([int]$publicExcited.source_loop_end -ne 28215) -or
    (-not [bool]$publicExcited.source_loop_infinite)) {
    throw 'PublicExcited source loop fixture changed.'
}
if (($publicExcited.source_volume_envelope.Count -ne 29) -or
    ([int]$publicExcited.ds_initial_volume -ne 17) -or
    ([int]$publicExcited.ds_pan -ne 64)) {
    throw 'PublicExcited AOT volume envelope fixture changed.'
}

$expectedKo = @(
    @{ ID = 439; Sound = 183; Frequency = 16009; Duration = 96;
        Root = 439; Render = 439; Hash =
        'e66ca536167618e8916f2c36d789742c785946e7f4793d8248cb584a90e23189' },
    @{ ID = 292; Sound = 28; Frequency = 16951; Duration = 53;
        Root = 292; Render = 287; Hash =
        'cf4ca538127d5175198077f877b390b19df27a3314eb0b6c8d5eb8e2b202cbf6' },
    @{ ID = 370; Sound = 104; Frequency = 16000; Duration = 120;
        Root = 370; Render = 370; Hash =
        'e518d1106a41cf9c99eac91ab053304a515a04d6f5fc5a9c5ea845365518ea77' },
    @{ ID = 289; Sound = 28; Frequency = 16951; Duration = 53;
        Root = 289; Render = 287; Hash =
        'cf4ca538127d5175198077f877b390b19df27a3314eb0b6c8d5eb8e2b202cbf6' },
    @{ ID = 154; Sound = 0; Frequency = 8476; Duration = 300;
        Root = 154; Render = 154; Hash =
        '676ebc7efd26cea3e6c477c9756aff2fe3d1d20ebe460074cc75086399fe08b9' }
)
foreach ($expected in $expectedKo) {
    $entry = @($metadata.entries | Where-Object {
            [int]$_.id -eq $expected.ID
        })
    if ($entry.Count -ne 1) {
        throw "Regular-KO FGM $($expected.ID) mapping count changed."
    }
    $entry = $entry[0]
    if (([int]$entry.source_sound_index -ne $expected.Sound) -or
        ([int]$entry.ds_frequency_hz -ne $expected.Frequency) -or
        ([int]$entry.source_duration_ticks -ne $expected.Duration) -or
        ([int]$entry.root_ucd_program_id -ne $expected.Root) -or
        ([int]$entry.render_ucd_program_id -ne $expected.Render) -or
        ($entry.ima_adpcm_sha256 -ne $expected.Hash)) {
        throw "Regular-KO FGM $($expected.ID) source fixture changed."
    }
}

$marioSlam = $metadata.entries | Where-Object { [int]$_.id -eq 292 }
$foxSlam = $metadata.entries | Where-Object { [int]$_.id -eq 289 }
if ((@($marioSlam.root_fork_programs) -join ',') -ne '287' -or
    (@($foxSlam.root_fork_programs) -join ',') -ne '287' -or
    ([int]$marioSlam.pack_data_offset -ne
        [int]$foxSlam.pack_data_offset) -or
    [bool]$marioSlam.sample_body_deduplicated -or
    (-not [bool]$foxSlam.sample_body_deduplicated)) {
    throw 'Mario/Fox shared slam program or resident sample dedup changed.'
}
$deadExplode = $metadata.entries | Where-Object { [int]$_.id -eq 154 }
if ((@($deadExplode.root_fork_programs) -join ',') -ne '685' -or
    (@($deadExplode.omitted_fork_programs) -join ',') -ne '685' -or
    -not (@($deadExplode.runtime_fidelity_debt) -contains
        'omitted_fork_voice_685')) {
    throw 'DeadExplodeL fork-685 fidelity-debt fixture changed.'
}
$koDebt = @($metadata.entries | Where-Object {
        $_.entry_kind -eq 'ko'
    } | ForEach-Object { @($_.runtime_fidelity_debt) })
if (-not ($koDebt -contains 'ucd_pitch_automation') -or
    -not ($koDebt -contains 'articulation_pitch_modulation')) {
    throw 'Regular-KO pitch-automation fidelity debt is no longer explicit.'
}

Write-Output (
    ('Audio FGM pack passed: {0} exact IDs ({1} regular-KO), {2} ' +
    'resident bytes, {3} unique samples, mapping {4}, pack {5}.') -f
    $actualIDs.Count,
    $koIDs.Count,
    $metadata.resident_bytes,
    $metadata.unique_sample_count,
    $metadata.mapping_sha256_lo,
    $metadata.pack_sha256
)
