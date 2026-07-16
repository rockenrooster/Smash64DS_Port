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
if ([int64]$metadata.resident_bytes -ne 64848) {
    throw "FGM pack resident size changed: $($metadata.resident_bytes)"
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
if (([int]$metadata.format_version -ne 3) -or
    ([int]$metadata.entry_bytes -ne 32) -or
    ($metadata.entry_final_u16 -ne 'ds_loop_point_words') -or
    ([int]$metadata.envelope_point_bytes -ne 4)) {
    throw 'Unexpected FGM pack entry/envelope format.'
}
if (($metadata.mapping_sha256_lo -ne '0x3c77e937') -or
    ($metadata.pack_sha256 -ne
        'c6d7136ef377b52f1b6ac97045da87ef7d04792dfbc2913543e584734e65c747')) {
    throw 'FGM pack mapping or binary hash changed.'
}
if (($metadata.non_loop_sample_sha256 -ne
        '5485b291f399f7cdc4eb2beadcb6ff5b58f7caf5fd2a335f0cfd7ed1790a6786') -or
    ($metadata.non_loop_envelope_sha256 -ne
        '11c0720ad4a73e021cf216fd2ea4754b4c6e3e40e4c03f3b8d29b3ad04e537b1')) {
    throw 'Non-loop FGM sample or packed-envelope bytes changed.'
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
if (($publicExcited.ds_loop_strategy -ne 'finite_source_loop_aot') -or
    ([int]$publicExcited.ds_sample_count -ne 104204) -or
    ([int]$publicExcited.ds_loop_point_words -ne 0) -or
    ([int]$publicExcited.packed_envelope_count -ne 0)) {
    throw ('PublicExcited still uses a hardware loop or runtime envelope ' +
        'instead of the finite 104204-sample AOT cue.')
}
if ($publicExcited.ds_loop_strategy -ne
    'state_word_then_source_loop_nibbles_plus_alignment_guards') {
    throw 'PublicExcited DS loop-body/alignment strategy changed.'
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
if (($publicExcited.ima_adpcm_sha256 -ne
        'efb072be5dd4409901c4490c6d150b7c9b79a41f5368a61f90c021565002628c') -or
    ([int]$publicExcited.ima_adpcm_bytes -ne 14112) -or
    ([int]$publicExcited.ds_sample_count -ne 28214) -or
    ([int]$publicExcited.ds_loop_point_words -ne 1) -or
    ([int]$publicExcited.ds_loop_length_words -ne 3527) -or
    ([int]$publicExcited.ds_ima_header_predictor -ne -4553) -or
    ([int]$publicExcited.ds_ima_header_index -ne 65) -or
    ([int]$publicExcited.ds_ima_loop_body_nibbles -ne 28214) -or
    ((@($publicExcited.ds_ima_guard_nibbles) -join ',') -ne '8,9') -or
    ($publicExcited.ds_repeat_oracle_model -ne
        'header_once_pnt_latch_len_restore') -or
    ([int]$publicExcited.ds_repeat_oracle_cycles -ne 3) -or
    ([int]$publicExcited.ds_repeat_oracle_loop_predictor -ne -4553) -or
    ([int]$publicExcited.ds_repeat_oracle_loop_index -ne 65) -or
    ([int]$publicExcited.ds_repeat_oracle_cycle_end_predictor -ne -4409) -or
    ([int]$publicExcited.ds_repeat_oracle_cycle_end_index -ne 75) -or
    (-not [bool]$publicExcited.ds_repeat_oracle_missing_restore_detected) -or
    ($publicExcited.ds_repeat_oracle_missing_restore_cycle_2_pcm_sha256 -ne
        'afe23ae391f2cb8216d4a016d4b375851c63d91df412a1ecab87c7446d385c82') -or
    (-not [bool]$publicExcited.ds_repeat_oracle_wrong_pnt_detected) -or
    (-not [bool]$publicExcited.ds_repeat_oracle_wrong_len_detected) -or
    ([int]$publicExcited.ds_repeat_cycle_source_samples -ne 28214) -or
    ([int]$publicExcited.ds_repeat_cycle_alignment_debt_samples -ne 2) -or
    ([int]$publicExcited.ds_repeat_cycle_samples -ne 28216) -or
    ($publicExcited.ds_repeat_cycle_pcm_sha256 -ne
        '5a12ec9b957390cb025565f4774ab44ed5428f3af71b13752af0f84fdd21f8e8')) {
    throw 'PublicExcited DS IMA loop-state or repeat oracle changed.'
}
if ($publicExcited.ds_repeat_oracle_missing_restore_cycle_2_pcm_sha256 -eq
    $publicExcited.ds_repeat_cycle_pcm_sha256) {
    throw 'PublicExcited carry-state negative control no longer diverges.'
}
$alignmentDebt = @($metadata.known_runtime_fidelity_debt | Where-Object {
        $_ -match 'two word-alignment guard samples' -and
        $_ -match '28,216-sample cycle' -and
        $_ -match 'not exclusively'
    })
if ($alignmentDebt.Count -ne 1) {
    throw 'PublicExcited per-cycle alignment debt is not explicit.'
}

$expectedNonLoopPhaseHashes = @{
    470 = '668cade614e56e94bdd96454bf11e0cea8e86cc42bb28f887be67ec60ea453c6'
    469 = '16a68f0bcc93f389677358f2040c7659f59b44844158b2db976f30fca1917f13'
    467 = '8f440486dd3ca8cffadbe6c5ec436f391a6385696ddf766113d05ca45c7720ee'
    490 = '214bfa4c1964973bcf3efaa82a12e68c3b2b9d8489c8c0efea9bedddfd10927b'
}
foreach ($id in $expectedNonLoopPhaseHashes.Keys) {
    $entry = @($metadata.entries | Where-Object { [int]$_.id -eq $id })
    if (($entry.Count -ne 1) -or
        ($entry[0].ima_adpcm_sha256 -ne
            $expectedNonLoopPhaseHashes[$id]) -or
        ([int]$entry[0].ds_loop_point_words -ne 0)) {
        throw "Non-loop phase FGM $id sample or loop point changed."
    }
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
        ([int]$entry.ds_loop_point_words -ne 0) -or
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
