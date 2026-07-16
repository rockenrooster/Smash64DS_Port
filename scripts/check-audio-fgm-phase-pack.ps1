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
$expectedIDs = @(626, 470, 469, 467, 490, 74, 363, 364, 372, 430,
    439, 292, 370, 289, 300, 154, 77, 429, 435,
    42, 43, 190, 215, 218, 219, 40, 38, 37, 34, 32, 31,
    216, 28, 2, 0)
$actualIDs = @($metadata.entries | ForEach-Object { [int]$_.id })
if (($actualIDs -join ',') -ne ($expectedIDs -join ',')) {
    throw "Unexpected FGM mapping: $($actualIDs -join ',')"
}
if (([int64]$metadata.resident_bytes -ne 168240) -or
    ([int64]$metadata.resident_limit_bytes -ne 168960)) {
    throw "FGM pack resident size changed: $($metadata.resident_bytes)"
}

$phaseIDs = @($metadata.entries | Where-Object {
        $_.entry_kind -eq 'phase'
    } | ForEach-Object { [int]$_.id })
$koIDs = @($metadata.entries | Where-Object {
        $_.entry_kind -eq 'ko'
    } | ForEach-Object { [int]$_.id })
$voiceIDs = @($metadata.entries | Where-Object {
        $_.entry_kind -eq 'voice'
    } | ForEach-Object { [int]$_.id })
$marioIDs = @($metadata.entries | Where-Object {
        $_.entry_kind -eq 'mario'
    } | ForEach-Object { [int]$_.id })
$attackIDs = @($metadata.entries | Where-Object {
        $_.entry_kind -eq 'attack'
    } | ForEach-Object { [int]$_.id })
$movementIDs = @($metadata.entries | Where-Object {
        $_.entry_kind -eq 'movement'
    } | ForEach-Object { [int]$_.id })
$hitIDs = @($metadata.entries | Where-Object {
        $_.entry_kind -eq 'hit'
    } | ForEach-Object { [int]$_.id })
if (($phaseIDs -join ',') -ne '626,470,469,467,490') {
    throw "Unexpected FGM phase subset: $($phaseIDs -join ',')"
}
if (($koIDs -join ',') -ne '439,292,370,289,154') {
    throw "Unexpected regular-KO subset: $($koIDs -join ',')"
}
if (($voiceIDs -join ',') -ne '363,364,372,430,429,435') {
    throw "Unexpected fighter-voice subset: $($voiceIDs -join ',')"
}
if (($movementIDs -join ',') -ne '74,300') {
    throw "Unexpected Fox-movement subset: $($movementIDs -join ',')"
}
if (($marioIDs -join ',') -ne '77') {
    throw "Unexpected Mario movement subset: $($marioIDs -join ',')"
}
if (($attackIDs -join ',') -ne '42,43,190,215,218,219') {
    throw "Unexpected attack/activation subset: $($attackIDs -join ',')"
}
if (($hitIDs -join ',') -ne '40,38,37,34,32,31,216,28,2,0') {
    throw "Unexpected collision/contact subset: $($hitIDs -join ',')"
}
if (([int]$metadata.unique_sample_count -ne 23) -or
    ([int]$metadata.unique_sample_bytes -ne 166488)) {
    throw 'FGM sample deduplication fixture changed.'
}
if (([int]$metadata.format_version -ne 3) -or
    ([int]$metadata.entry_bytes -ne 32) -or
    ($metadata.entry_final_u16 -ne 'ds_loop_point_words') -or
    ([int]$metadata.envelope_point_bytes -ne 4)) {
    throw 'Unexpected FGM pack entry/envelope format.'
}
if (($metadata.mapping_sha256_lo -ne '0x10a76e6a') -or
    ($metadata.pack_sha256 -ne
        'd178dd23702a3e391427916773271a4c098a3579f848fc9a7e6c507eddbf0c63')) {
    throw 'FGM pack mapping or binary hash changed.'
}
if (($metadata.non_loop_sample_sha256 -ne
        '66284762f1d7974dc0a4e9ef3661478ee6f81e6080812a4307b791dd8ae5f69a') -or
    ($metadata.non_loop_envelope_sha256 -ne
        '7907ebd91cf7d97517464db834e355e30930e8cf36438d9286dbdde63ec585c3')) {
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
        ([int64]$entry.ds_frequency_hz -gt [uint16]::MaxValue) -or
        ([int64]$entry.source_duration_microseconds -le 0)) {
        throw "FGM $($entry.id) has invalid runtime timing metadata."
    }
}

$publicExcited = $metadata.entries | Where-Object { [int]$_.id -eq 626 }
if (($publicExcited.ds_loop_strategy -ne 'finite_source_loop_aot') -or
    ([int]$publicExcited.ds_sample_count -ne 104204) -or
    ([int]$publicExcited.ds_frequency_hz -ne 15102) -or
    ([int]$publicExcited.source_duration_ticks -ne 1200) -or
    ([int64]$publicExcited.source_duration_microseconds -ne 6900000) -or
    ([int]$publicExcited.ds_loop_flag -ne 0) -or
    ([int]$publicExcited.ds_loop_point_words -ne 0) -or
    ([int]$publicExcited.packed_envelope_count -ne 0)) {
    throw ('PublicExcited still uses a hardware loop or runtime envelope ' +
        'instead of the finite 104204-sample AOT cue.')
}
if (([int]$publicExcited.source_loop_start -ne 1) -or
    ([int]$publicExcited.source_loop_end -ne 28215) -or
    (-not [bool]$publicExcited.source_loop_infinite)) {
    throw 'PublicExcited source loop fixture changed.'
}
if (($publicExcited.source_volume_envelope.Count -ne 29) -or
    ([int]$publicExcited.ds_initial_volume -ne 92) -or
    ([int]$publicExcited.ds_pan -ne 64)) {
    throw 'PublicExcited AOT volume envelope fixture changed.'
}
$envelopeTicks = @($publicExcited.source_volume_envelope |
    ForEach-Object { [int]$_.tick })
$quadraticTargets = @($publicExcited.source_volume_envelope |
    ForEach-Object { [int]$_.source_quadratic_target })
if (($envelopeTicks -join ',') -ne
        '0,2,4,6,9,12,15,18,21,25,45,125,185,285,385,485,585,595,685,695,775,785,835,845,895,905,965,975,1075' -or
    ($quadraticTargets -join ',') -ne
        '586,1320,2347,3667,5281,7189,9389,11884,14672,17754,21127,23665,21127,17754,16176,14672,13241,11884,10601,9389,8253,7189,6198,5281,4438,3667,2347,1320,0') {
    throw 'PublicExcited source-derived quadratic target schedule changed.'
}

$oracle = $publicExcited.acoustic_oracle
$firstCommand = $oracle.command_points[0]
$peakCommand = $oracle.command_points[11]
$formerLinearPoint = $oracle.command_points[27]
$finalCommand = $oracle.command_points[28]
if (($oracle.model -ne
        'source_loop_then_quadratic_n_micro_184_sample_ramps') -or
    ([int]$oracle.sample_count -ne 104204) -or
    ([int]$oracle.source_first_pass_samples -ne 28215) -or
    ([int]$oracle.source_loop_start -ne 1) -or
    ([int]$oracle.source_loop_end -ne 28215) -or
    ([int]$oracle.source_loop_samples -ne 28214) -or
    ((@($oracle.source_former_loop_boundary_starts) -join ',') -ne
        '28215,56429,84643') -or
    (-not [bool]$oracle.source_pre_roll_present) -or
    (-not [bool]$oracle.source_order_exact) -or
    ($oracle.source_index_sha256 -ne
        '8c364955cb8dd8d7d639b7aad86141d23cf262c9d2efb10b48296952d6cecd01')) {
    throw 'PublicExcited finite source-loop ordering oracle changed.'
}
if (([int]$oracle.ramp_output_rate_hz -ne 32000) -or
    ([int]$oracle.ramp_samples -ne 184) -or
    ([int]$oracle.ramp_microseconds -ne 5750) -or
    ([int]$oracle.command_points.Count -ne 29) -or
    ([int]$firstCommand.tick -ne 0) -or
    ([int]$firstCommand.start_quadratic_target -ne 1) -or
    ([int]$firstCommand.end_quadratic_target -ne 586) -or
    ([int]$firstCommand.start_sample_ceiling -ne 0) -or
    ([int]$firstCommand.end_sample_ceiling -ne 87) -or
    ([int]$peakCommand.tick -ne 125) -or
    ([int]$peakCommand.end_quadratic_target -ne 23665) -or
    ([int]$formerLinearPoint.tick -ne 975) -or
    ([int]$formerLinearPoint.end_quadratic_target -ne 1320) -or
    ([int]$finalCommand.tick -ne 1075) -or
    ([int]$finalCommand.start_sample_ceiling -ne 93350) -or
    ([int]$finalCommand.end_sample_ceiling -ne 93437) -or
    ([int]$finalCommand.end_quadratic_target -ne 0)) {
    throw 'PublicExcited N_MICRO quadratic ramp endpoint oracle changed.'
}
if (([int]$oracle.maximum_quadratic_target -ne 23665) -or
    ([int]$oracle.constant_hardware_volume -ne 92) -or
    ([int]$oracle.constant_hardware_gain_numerator -ne 92) -or
    ([int]$oracle.constant_hardware_gain_denominator -ne 127) -or
    ([int]$oracle.silent_tail_start_sample -ne 93437) -or
    ([int]$oracle.decoded_silent_tail_samples -ne 10767) -or
    ([int]$oracle.decoded_silent_tail_peak -gt 5) -or
    ([double]$oracle.decoded_silent_tail_rms -gt 0.12)) {
    throw 'PublicExcited constant hardware gain or silent-tail oracle changed.'
}
if (($publicExcited.ima_adpcm_sha256 -ne
        'f1625186cf7b73e488bceb7dd65e6a697731f3160f1f59e91ff9d1de9b7cfb19') -or
    ([int]$publicExcited.ima_adpcm_bytes -ne 52108) -or
    ($oracle.rendered_pcm_sha256 -ne
        'c48fd539c835330a4319be5f94345e281e65ad12f05a1cbc58d7d1f588604374') -or
    ($oracle.decoded_pcm_sha256 -ne
        'aecb7d2a15884b89a49b4ec2d746deb0658efdf3974c36537a3a4cd55741c3c7') -or
    ([int]$oracle.rendered_clipped_sample_count -ne 0) -or
    ([int]$oracle.decoded_clipped_sample_count -ne 15) -or
    ([int]$oracle.old_hardware_loop_decoded_clipped_sample_count -ne 71) -or
    (-not [bool]$oracle.decoded_clipping_not_regressed) -or
    ([int]$publicExcited.decoded_peak -le 0) -or
    ([double]$publicExcited.decoded_rms -le 0.0)) {
    throw 'PublicExcited rendered/decoded acoustic metrics changed.'
}
if (((@($oracle.decoded_former_loop_boundary_deltas) -join ',') -ne
        '3235,2209,436') -or
    ([int]$oracle.decoded_former_loop_boundary_max_delta -ne 3235) -or
    ([int]$oracle.decoded_adjacent_max_delta -ne 28666) -or
    (-not [bool]$oracle.decoded_former_loop_boundaries_bounded)) {
    throw 'PublicExcited former-loop-boundary continuity changed.'
}
if (($oracle.linear_gain_negative_pcm_sha256 -ne
        'c74436def9b134f329149a8fad2722df7ff02dec02cff427e212c43279311c81') -or
    (-not [bool]$oracle.linear_gain_negative_rejected) -or
    ($oracle.missing_preroll_negative_pcm_sha256 -ne
        '8183b613abab27c5246daeaaa42983e927b639c8b8ba20db13d4ffc27e68b19b') -or
    (-not [bool]$oracle.missing_preroll_negative_rejected) -or
    ([int]$oracle.old_hardware_loop_negative_ima_bytes -ne 14112) -or
    ($oracle.old_hardware_loop_negative_ima_sha256 -ne
        'efb072be5dd4409901c4490c6d150b7c9b79a41f5368a61f90c021565002628c') -or
    (-not [bool]$oracle.old_hardware_loop_negative_rejected)) {
    throw 'PublicExcited acoustic negative control no longer rejects old behavior.'
}
if (@($metadata.known_runtime_fidelity_debt | Where-Object {
        $_ -match 'PublicExcited|pre-roll|guard samples|step volume'
    }).Count -ne 0) {
    throw 'Closed PublicExcited loop/envelope fidelity debt remains recorded.'
}

$expectedNonLoopPhaseHashes = @{
    470 = 'da6ec8228ec6cdc90c7a68568fa2bb002b6f94601be6bfe08e6f459dcd3ac35c'
    469 = 'ceaec3acdbfc8321cd5c7149b87dd6fa895710514c74c2af95d345a992a1cf38'
    467 = '5d690f3a6ad9f2b682e2ba8e383e4aaa43942d0fa729b73169721d99670e5862'
    490 = 'f33b2eee06748d1984e89e9cef97606d9c1d8c644e104cbb3326e75999fcf37a'
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

$expectedFighterVoices = @(
    @{ ID = 363; Sound = 108; Frequency = 16000; Duration = 45;
        Samples = 3760; Volume = 76; Envelope = 0; Hash =
        'd632acb6b82b8b97798659791da4170c7c8a6b4a707ea3b37ecd582fbd5b3bc8' },
    @{ ID = 364; Sound = 102; Frequency = 16000; Duration = 65;
        Samples = 2912; Volume = 42; Envelope = 5; Hash =
        '141b7c3cdd1fe13e491fd087daaacd63d34f36c07dc589562e29e056e39e36ae' },
    @{ ID = 372; Sound = 105; Frequency = 16000; Duration = 46;
        Samples = 1680; Volume = 108; Envelope = 0; Hash =
        'ac3948daf50a93899233040c52153d84e75417f112487cc33679e40623804fcf' },
    @{ ID = 430; Sound = 174; Frequency = 16280; Duration = 236;
        Samples = 17232; Volume = 86; Envelope = 1; Hash =
        '88ed6606f9199d787398a6d3b8563803cd4eddb4ccf7b9e4120e32d1f4292acd' },
    @{ ID = 429; Sound = 173; Frequency = 15111; Duration = 96;
        Samples = 3648; Volume = 95; Envelope = 0; Hash =
        '5fad36ccee66dcc3c6519a85ed9612df4f5aacb29529e7943ba666734edc13f1' },
    @{ ID = 435; Sound = 179; Frequency = 15455; Duration = 96;
        Samples = 3344; Volume = 84; Envelope = 3; Hash =
        'b7e6bb1473bcea4766255482f01a7740a40896665027031a13ba0e5007676a1b' }
)
foreach ($expected in $expectedFighterVoices) {
    $entry = @($metadata.entries | Where-Object {
            [int]$_.id -eq $expected.ID
        })
    if (($entry.Count -ne 1) -or
        ([int]$entry[0].source_sound_index -ne $expected.Sound) -or
        ([int]$entry[0].ds_frequency_hz -ne $expected.Frequency) -or
        ([int]$entry[0].source_duration_ticks -ne $expected.Duration) -or
        ([int]$entry[0].ds_sample_count -ne $expected.Samples) -or
        ([int]$entry[0].ds_volume -ne $expected.Volume) -or
        ([int]$entry[0].packed_envelope_count -ne $expected.Envelope) -or
        ([int]$entry[0].ds_loop_point_words -ne 0) -or
        ($entry[0].ima_adpcm_sha256 -ne $expected.Hash)) {
        throw "Fighter voice $($expected.ID) source fixture changed."
    }
}

$marioLanding = @($metadata.entries | Where-Object { [int]$_.id -eq 77 })
if (($marioLanding.Count -ne 1) -or
    ([int]$marioLanding[0].root_ucd_program_id -ne 77) -or
    ([int]$marioLanding[0].render_ucd_program_id -ne 72) -or
    ([int]$marioLanding[0].articulation_index -ne 3) -or
    ([int]$marioLanding[0].source_sound_index -ne 1) -or
    ([int]$marioLanding[0].ds_frequency_hz -ne 35919) -or
    ([int]$marioLanding[0].source_duration_ticks -ne 3) -or
    ([int]$marioLanding[0].ds_sample_count -ne 621) -or
    ([int]$marioLanding[0].ds_volume -ne 21) -or
    ([int]$marioLanding[0].packed_envelope_count -ne 2) -or
    ($marioLanding[0].ima_adpcm_sha256 -ne
        '92d03fe38eb5de34abb1568891b72c7082c052551b84391fd3af9e3bbcb30c1e')) {
    throw 'Mario landing source fixture changed.'
}
if (@($marioLanding[0].runtime_fidelity_debt).Count -ne 0) {
    throw 'Mario landing no longer has exact bounded pitch/envelope metadata.'
}
foreach ($id in 429, 430, 435) {
    $entry = $metadata.entries | Where-Object { [int]$_.id -eq $id }
    if (-not (@($entry.runtime_fidelity_debt) -contains
            'ucd_pitch_automation')) {
        throw "Mario voice $id no longer records pitch-automation debt."
    }
}
$expectedAttackSounds = @(
    @{ ID = 42; Sound = 71; Frequency = 39170; Duration = 35;
        Samples = 5184; Volume = 87; Debt = $false; Hash =
        '32b143ee0f4d276d44b0f685c85ce82f5ff6c260a65c2d3cf39743e692e63ab1' },
    @{ ID = 43; Sound = 71; Frequency = 32938; Duration = 35;
        Samples = 5184; Volume = 73; Debt = $false; Hash =
        '32b143ee0f4d276d44b0f685c85ce82f5ff6c260a65c2d3cf39743e692e63ab1' },
    @{ ID = 190; Sound = 71; Frequency = 36971; Duration = 25;
        Samples = 5184; Volume = 73; Debt = $true; Hash =
        '32b143ee0f4d276d44b0f685c85ce82f5ff6c260a65c2d3cf39743e692e63ab1' },
    @{ ID = 215; Sound = 19; Frequency = 32000; Duration = 15;
        Samples = 2176; Volume = 121; Debt = $false; Hash =
        '7ed82ac09a350207bb4107b598447567516fd03ad40324953d041507a234ef78' },
    @{ ID = 218; Sound = 71; Frequency = 32938; Duration = 27;
        Samples = 5184; Volume = 73; Debt = $true; Hash =
        '32b143ee0f4d276d44b0f685c85ce82f5ff6c260a65c2d3cf39743e692e63ab1' },
    @{ ID = 219; Sound = 71; Frequency = 36971; Duration = 25;
        Samples = 5184; Volume = 73; Debt = $true; Hash =
        '32b143ee0f4d276d44b0f685c85ce82f5ff6c260a65c2d3cf39743e692e63ab1' }
)
foreach ($expected in $expectedAttackSounds) {
    $entry = @($metadata.entries | Where-Object {
            [int]$_.id -eq $expected.ID
        })
    $hasPitchDebt = $entry.Count -eq 1 -and
        (@($entry[0].runtime_fidelity_debt) -contains
            'ucd_pitch_automation')
    if (($entry.Count -ne 1) -or
        ([int]$entry[0].source_sound_index -ne $expected.Sound) -or
        ([int]$entry[0].ds_frequency_hz -ne $expected.Frequency) -or
        ([int]$entry[0].source_duration_ticks -ne $expected.Duration) -or
        ([int]$entry[0].ds_sample_count -ne $expected.Samples) -or
        ([int]$entry[0].ds_volume -ne $expected.Volume) -or
        ([int]$entry[0].packed_envelope_count -ne 0) -or
        ([int]$entry[0].ds_loop_point_words -ne 0) -or
        ($entry[0].ima_adpcm_sha256 -ne $expected.Hash) -or
        ($hasPitchDebt -ne $expected.Debt)) {
        throw "Attack/activation FGM $($expected.ID) source fixture changed."
    }
}
$swingEntries = @($metadata.entries | Where-Object {
        [int]$_.id -in 42, 43, 190, 218, 219
    })
if (($swingEntries.Count -ne 5) -or
    (@($swingEntries.pack_data_offset | Sort-Object -Unique).Count -ne 1) -or
    [bool]$swingEntries[0].sample_body_deduplicated -or
    (@($swingEntries | Select-Object -Skip 1 | Where-Object {
            -not [bool]$_.sample_body_deduplicated
        }).Count -ne 0)) {
    throw 'Shared light-swing sample-body deduplication changed.'
}

$expectedFoxMovement = @(
    @{ ID = 74; Sound = 1; Frequency = 35919; Duration = 3;
        Samples = 621; Volume = 21; Envelope = 2;
        Root = 74; Render = 72; Hash =
        '92d03fe38eb5de34abb1568891b72c7082c052551b84391fd3af9e3bbcb30c1e' },
    @{ ID = 300; Sound = 28; Frequency = 16000; Duration = 25;
        Samples = 6688; Volume = 20; Envelope = 2;
        Root = 300; Render = 298; Hash =
        'cf4ca538127d5175198077f877b390b19df27a3314eb0b6c8d5eb8e2b202cbf6' }
)
foreach ($expected in $expectedFoxMovement) {
    $entry = @($metadata.entries | Where-Object {
            [int]$_.id -eq $expected.ID
        })
    if (($entry.Count -ne 1) -or
        ([int]$entry[0].source_sound_index -ne $expected.Sound) -or
        ([int]$entry[0].ds_frequency_hz -ne $expected.Frequency) -or
        ([int]$entry[0].source_duration_ticks -ne $expected.Duration) -or
        ([int]$entry[0].ds_sample_count -ne $expected.Samples) -or
        ([int]$entry[0].ds_volume -ne $expected.Volume) -or
        ([int]$entry[0].packed_envelope_count -ne $expected.Envelope) -or
        ([int]$entry[0].root_ucd_program_id -ne $expected.Root) -or
        ([int]$entry[0].render_ucd_program_id -ne $expected.Render) -or
        ([int]$entry[0].ds_loop_point_words -ne 0) -or
        ($entry[0].ima_adpcm_sha256 -ne $expected.Hash)) {
        throw "Fox movement FGM $($expected.ID) source fixture changed."
    }
}

$expectedTrimProofs = @(
    @{ ID = 470; Source = 11248; Schedule = 9109; Current = 9109;
        Retained = 9109; Removed = 2139; Prefix =
        '801fa7c7cdb82e07dd4c481d6de97ab4a4d084b3e88bd811560aed8fb4fc9cf1' },
    @{ ID = 469; Source = 11472; Schedule = 9201; Current = 9201;
        Retained = 9201; Removed = 2271; Prefix =
        'ab09e8e41568ec2c6f9f013eb28a240f293e6e187a200e022a0417af3fe7c41b' },
    @{ ID = 467; Source = 10848; Schedule = 7821; Current = 7821;
        Retained = 7821; Removed = 3027; Prefix =
        'fbb5539fc312bf1f0e2be7f69c53792491f5ab2d4e672c75c525742b0b3452cd' },
    @{ ID = 490; Source = 15840; Schedule = 13801; Current = 13801;
        Retained = 13801; Removed = 2039; Prefix =
        '647566464d00d204664f6a13be5fd144194c9933ed7f011006faa80261435429' },
    @{ ID = 74; Source = 5232; Schedule = 621; Current = 621;
        Retained = 621; Removed = 4611; Prefix =
        '53e9fcd9f8c3a4908dea144b2774cf839ce9f27a6872a53984fe0517b7ec83d7' },
    @{ ID = 363; Source = 3760; Schedule = 4141; Current = 4141;
        Retained = 3760; Removed = 0; Prefix =
        '362ca3a2a7d4ba82775cef426ae1a1df528688edd014347322f2bc3de24e5270' },
    @{ ID = 364; Source = 2912; Schedule = 5981; Current = 5981;
        Retained = 2912; Removed = 0; Prefix =
        'b03e298f9643eef58c3db9c44c53078557d84b555e8389fb515122d52844a499' },
    @{ ID = 372; Source = 1680; Schedule = 4233; Current = 4233;
        Retained = 1680; Removed = 0; Prefix =
        '95a4e1fe34793dd32060f5eb090ed4331b45893005f548a80b37e6b6d280e070' },
    @{ ID = 430; Source = 17232; Schedule = 19182; Current = 22093;
        Retained = 17232; Removed = 0; Prefix =
        'ccd7fc9b7add5f8527d0c9ae3c192c8500d5b603d63d8fc1f40b3c2a1a51ac0f' },
    @{ ID = 77; Source = 5232; Schedule = 621; Current = 621;
        Retained = 621; Removed = 4611; Prefix =
        '53e9fcd9f8c3a4908dea144b2774cf839ce9f27a6872a53984fe0517b7ec83d7' },
    @{ ID = 429; Source = 3648; Schedule = 7819; Current = 8343;
        Retained = 3648; Removed = 0; Prefix =
        '6e501af0518563f47b999602ccd17447cd1a41f55557dbff30d2949adcc7f1c0' },
    @{ ID = 435; Source = 3344; Schedule = 7969; Current = 8533;
        Retained = 3344; Removed = 0; Prefix =
        '96463bf05640fc47c336d50ec47f18c510d69d878c0ea1ec52a19c346f949e9c' },
    @{ ID = 42; Source = 5184; Schedule = 7885; Current = 7884;
        Retained = 5184; Removed = 0; Prefix =
        'a6a1dc0e7cce3e5d63c1740a8951f3d6119b9becba0a1120a07a000daf4d268f' },
    @{ ID = 43; Source = 5184; Schedule = 6630; Current = 6630;
        Retained = 5184; Removed = 0; Prefix =
        'a6a1dc0e7cce3e5d63c1740a8951f3d6119b9becba0a1120a07a000daf4d268f' },
    @{ ID = 190; Source = 5184; Schedule = 5839; Current = 5316;
        Retained = 5184; Removed = 0; Prefix =
        'a6a1dc0e7cce3e5d63c1740a8951f3d6119b9becba0a1120a07a000daf4d268f' },
    @{ ID = 215; Source = 2176; Schedule = 2761; Current = 2761;
        Retained = 2176; Removed = 0; Prefix =
        '8f6664be97363bc3e73916e0c177597ff99657c7bcece9632606109829952c68' },
    @{ ID = 218; Source = 5184; Schedule = 6089; Current = 5115;
        Retained = 5184; Removed = 0; Prefix =
        'a6a1dc0e7cce3e5d63c1740a8951f3d6119b9becba0a1120a07a000daf4d268f' },
    @{ ID = 219; Source = 5184; Schedule = 5839; Current = 5316;
        Retained = 5184; Removed = 0; Prefix =
        'a6a1dc0e7cce3e5d63c1740a8951f3d6119b9becba0a1120a07a000daf4d268f' },
    @{ ID = 439; Source = 9136; Schedule = 8634; Current = 8838;
        Retained = 8838; Removed = 298; Prefix =
        '21b2a92339ebff5a17bbd1c733d37e93f415e8d9ccd65d8d080c7b9cb68687ec' },
    @{ ID = 370; Source = 9808; Schedule = 11041; Current = 11041;
        Retained = 9808; Removed = 0; Prefix =
        '981080097d24ccd16a8de1c417f409bb4a2a8b38d273ce57d8df60d16e1ba39d' },
    @{ ID = 154; Source = 25280; Schedule = 14913; Current = 14623;
        Retained = 14913; Removed = 10367; Prefix =
        '95b15298900185a62667d5a7ae9b5df854d0284ff1f9cc1f92990ef5fe0df4bf' }
)
foreach ($expected in $expectedTrimProofs) {
    $entry = $metadata.entries | Where-Object { [int]$_.id -eq $expected.ID }
    if (($entry.trim_strategy -ne
            'source_note_schedule_and_current_ds_consumption_with_one_sample_ceiling') -or
        ([int]$entry.source_pcm_samples -ne $expected.Source) -or
        ([int]$entry.trim_source_schedule_reach_samples -ne
            $expected.Schedule) -or
        ([int]$entry.trim_current_ds_consumption_reach_samples -ne
            $expected.Current) -or
        ([int]$entry.trim_proven_reachable_samples -ne $expected.Retained) -or
        ([int]$entry.trim_source_samples_removed -ne $expected.Removed) -or
        ([int]$entry.trim_one_sample_ceiling -ne 1) -or
        (-not [bool]$entry.trim_retained_prefix_exact) -or
        ($entry.trim_retained_source_prefix_pcm_sha256 -ne $expected.Prefix)) {
        throw "FGM $($expected.ID) duration-derived trim proof changed."
    }
}
foreach ($id in 292, 289, 300) {
    $entry = $metadata.entries | Where-Object { [int]$_.id -eq $id }
    if (($entry.trim_strategy -ne
            'untrimmed_articulation_pitch_modulation') -or
        ([int]$entry.trim_source_samples_removed -ne 0) -or
        ([int]$entry.trim_proven_reachable_samples -ne 6688) -or
        (-not [bool]$entry.trim_retained_prefix_exact)) {
        throw "Pitch-modulated FGM $id was trimmed without a source proof."
    }
}

$expectedKo = @(
    @{ ID = 439; Sound = 183; Frequency = 16009; Duration = 96;
        Samples = 8838;
        Root = 439; Render = 439; Hash =
        'a71ef24f6f54df209e7b782f6322481aa73f7dda31128a6cb4151b70fc5a0f7f' },
    @{ ID = 292; Sound = 28; Frequency = 16951; Duration = 53;
        Samples = 6688;
        Root = 292; Render = 287; Hash =
        'cf4ca538127d5175198077f877b390b19df27a3314eb0b6c8d5eb8e2b202cbf6' },
    @{ ID = 370; Sound = 104; Frequency = 16000; Duration = 120;
        Samples = 9808;
        Root = 370; Render = 370; Hash =
        'e518d1106a41cf9c99eac91ab053304a515a04d6f5fc5a9c5ea845365518ea77' },
    @{ ID = 289; Sound = 28; Frequency = 16951; Duration = 53;
        Samples = 6688;
        Root = 289; Render = 287; Hash =
        'cf4ca538127d5175198077f877b390b19df27a3314eb0b6c8d5eb8e2b202cbf6' },
    @{ ID = 154; Sound = 0; Frequency = 8476; Duration = 300;
        Samples = 14913;
        Root = 154; Render = 154; Hash =
        '45cfe954355dd4edc22c3cc62a60dea59672b57a7f6ef1df21366ee012abc87c' }
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
        ([int]$entry.ds_sample_count -ne $expected.Samples) -or
        ([int]$entry.root_ucd_program_id -ne $expected.Root) -or
        ([int]$entry.render_ucd_program_id -ne $expected.Render) -or
        ([int]$entry.ds_loop_point_words -ne 0) -or
        ($entry.ima_adpcm_sha256 -ne $expected.Hash)) {
        throw "Regular-KO FGM $($expected.ID) source fixture changed."
    }
}

$marioSlam = $metadata.entries | Where-Object { [int]$_.id -eq 292 }
$foxSlam = $metadata.entries | Where-Object { [int]$_.id -eq 289 }
$foxBounce = $metadata.entries | Where-Object { [int]$_.id -eq 300 }
if ((@($marioSlam.root_fork_programs) -join ',') -ne '287' -or
    (@($foxSlam.root_fork_programs) -join ',') -ne '287' -or
    (@($foxBounce.root_fork_programs) -join ',') -ne '298' -or
    ([int]$marioSlam.pack_data_offset -ne
        [int]$foxSlam.pack_data_offset) -or
    ([int]$marioSlam.pack_data_offset -ne
        [int]$foxBounce.pack_data_offset) -or
    [bool]$marioSlam.sample_body_deduplicated -or
    (-not [bool]$foxSlam.sample_body_deduplicated) -or
    (-not [bool]$foxBounce.sample_body_deduplicated)) {
    throw 'Mario/Fox slam/bounce resident sample dedup changed.'
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

$expectedHits = @(
    @{ ID = 40; Sound = 11; Frequency = 25398; Duration = 68;
        Samples = 16416; Bytes = 8212; Volume = 72; Envelope = 4;
        Hash = '38d0b65d29a4ff3c75a8508e15940bd41d9d3ed3ecb7f10381f4c565a8a40053' },
    @{ ID = 38; Sound = 11; Frequency = 34896; Duration = 111;
        Samples = 16416; Bytes = 8212; Volume = 121; Envelope = 7;
        Hash = '38d0b65d29a4ff3c75a8508e15940bd41d9d3ed3ecb7f10381f4c565a8a40053' },
    @{ ID = 37; Sound = 11; Frequency = 45255; Duration = 155;
        Samples = 16416; Bytes = 8212; Volume = 124; Envelope = 8;
        Hash = '38d0b65d29a4ff3c75a8508e15940bd41d9d3ed3ecb7f10381f4c565a8a40053' },
    @{ ID = 34; Sound = 16; Frequency = 12788; Duration = 70;
        Samples = 11216; Bytes = 5612; Volume = 92; Envelope = 7;
        Hash = '9af7a90e3e4f0b175e36c0bd53e4bd21342fa4f535eea89e316eeb64e51894a7' },
    @{ ID = 32; Sound = 16; Frequency = 14254; Duration = 111;
        Samples = 11216; Bytes = 5612; Volume = 111; Envelope = 8;
        Hash = '9af7a90e3e4f0b175e36c0bd53e4bd21342fa4f535eea89e316eeb64e51894a7' },
    @{ ID = 31; Sound = 16; Frequency = 16000; Duration = 151;
        Samples = 11216; Bytes = 5612; Volume = 124; Envelope = 8;
        Hash = '9af7a90e3e4f0b175e36c0bd53e4bd21342fa4f535eea89e316eeb64e51894a7' },
    @{ ID = 216; Sound = 22; Frequency = 30204; Duration = 92;
        Samples = 20458; Bytes = 10236; Volume = 82; Envelope = 11;
        Hash = 'be33e0e365b534772558b69a35379c864325f38be16b22bc91c792c69db41fa1' },
    @{ ID = 28; Sound = 27; Frequency = 32000; Duration = 160;
        Samples = 29441; Bytes = 14724; Volume = 72; Envelope = 23;
        Hash = '0bd0766e8bb94f9833b439f266babcb89fd74d1d5b156fe765c109983e6fea2a' },
    @{ ID = 2; Sound = 4; Frequency = 50797; Duration = 190;
        Samples = 30304; Bytes = 15156; Volume = 97; Envelope = 23;
        Hash = 'b9494fab2cdf707c622105d1df10edc32a9a18768a90b5fc6010e172bbac71d6' },
    @{ ID = 0; Sound = 4; Frequency = 21357; Duration = 135;
        Samples = 30304; Bytes = 15156; Volume = 106; Envelope = 16;
        Hash = 'b9494fab2cdf707c622105d1df10edc32a9a18768a90b5fc6010e172bbac71d6' }
)
foreach ($expected in $expectedHits) {
    $entry = @($metadata.entries | Where-Object {
            [int]$_.id -eq $expected.ID
        })
    if (($entry.Count -ne 1) -or
        ($entry[0].entry_kind -ne 'hit') -or
        ([int]$entry[0].source_sound_index -ne $expected.Sound) -or
        ([int]$entry[0].ds_frequency_hz -ne $expected.Frequency) -or
        ([int]$entry[0].source_duration_ticks -ne $expected.Duration) -or
        ([int]$entry[0].ds_sample_count -ne $expected.Samples) -or
        ([int]$entry[0].ima_adpcm_bytes -ne $expected.Bytes) -or
        ([int]$entry[0].ds_volume -ne $expected.Volume) -or
        ([int]$entry[0].packed_envelope_count -ne $expected.Envelope) -or
        ($entry[0].ima_adpcm_sha256 -ne $expected.Hash)) {
        throw "Collision/contact FGM $($expected.ID) source fixture changed."
    }
}

$punch = @(40, 38, 37 | ForEach-Object {
        $id = $_
        $metadata.entries | Where-Object { [int]$_.id -eq $id }
    })
$kick = @(34, 32, 31 | ForEach-Object {
        $id = $_
        $metadata.entries | Where-Object { [int]$_.id -eq $id }
    })
$fire = @(2, 0 | ForEach-Object {
        $id = $_
        $metadata.entries | Where-Object { [int]$_.id -eq $id }
    })
if ((@($punch | ForEach-Object pack_data_offset | Sort-Object -Unique).Count -ne 1) -or
    ((@($punch | ForEach-Object sample_body_deduplicated) -join ',') -ne
        'False,True,True') -or
    (@($kick | ForEach-Object pack_data_offset | Sort-Object -Unique).Count -ne 1) -or
    ((@($kick | ForEach-Object sample_body_deduplicated) -join ',') -ne
        'False,True,True') -or
    (@($fire | ForEach-Object pack_data_offset | Sort-Object -Unique).Count -ne 1) -or
    ((@($fire | ForEach-Object sample_body_deduplicated) -join ',') -ne
        'False,True')) {
    throw 'Collision/contact exact sample-body reuse changed.'
}

$coin = $metadata.entries | Where-Object { [int]$_.id -eq 216 }
$burn = $metadata.entries | Where-Object { [int]$_.id -eq 28 }
$explode = $metadata.entries | Where-Object { [int]$_.id -eq 0 }
if (($coin.ds_loop_strategy -ne 'finite_source_loop_aot') -or
    ([int]$coin.finite_source_loop_replay_samples -ne 18833) -or
    ($burn.ds_loop_strategy -ne 'finite_source_loop_aot') -or
    ([int]$burn.finite_source_loop_replay_samples -ne 1985) -or
    ([int]$burn.root_ucd_program_id -ne 28) -or
    ([int]$explode.root_ucd_program_id -ne 0) -or
    ([int]$explode.render_ucd_program_id -ne 0)) {
    throw 'Coin/Fireball finite source-loop or direct ExplodeS program changed.'
}
if (@($metadata.entries | Where-Object { [int]$_.id -eq 188 }).Count -ne 0) {
    throw 'Fox reflector-hit ID 188 entered the pack without a natural mode-163 call.'
}
$hitDebt = @($metadata.entries | Where-Object {
        $_.entry_kind -eq 'hit'
    } | ForEach-Object { @($_.runtime_fidelity_debt) })
if (-not ($hitDebt -contains 'ucd_pitch_automation') -or
    -not ($hitDebt -contains 'ucd_volume_automation') -or
    -not ($hitDebt -contains 'articulation_pitch_modulation') -or
    -not ($hitDebt -contains 'omitted_fork_voice_668')) {
    throw 'Collision/contact pitch, volume, or fork debt is no longer explicit.'
}

$shim = Get-Content -LiteralPath (
    Join-Path $root 'src/port/reloc_backend_compat_shims.c') -Raw
$fgmBackend = Get-Content -LiteralPath (
    Join-Path $root 'src/nds/nds_audio_fgm.c') -Raw
foreach ($fragment in @(
        's32 balance = (s32)((pos / 8000.0F) * 60.0F);',
        'balance = 60;',
        'balance = -60;',
        'return ndsPlayFGMAtPan(fgm, (u8)(64 - balance));')) {
    if (-not $shim.Contains($fragment)) {
        throw "Source positional-pan seam changed: $fragment"
    }
}
if ((-not $fgmBackend.Contains(
        'alSoundEffect *ndsAudioFgmPlayAtPan(u16 fgm_id, u8 pan)')) -or
    (-not $fgmBackend.Contains(
        'entry->frequency, entry->volume, pan,')) -or
    (-not $fgmBackend.Contains('handle->effect.balance = pan;'))) {
    throw 'Source positional pan is not reaching the DS channel and handle.'
}

Write-Output (
    ('Audio FGM pack passed: {0} exact IDs ({1} fighter voices, ' +
    '{2} Fox movement, {3} regular-KO, {4} attack/activation, ' +
    '{5} collision/contact), {6} resident bytes, {7} unique samples, ' +
    'mapping {8}, pack {9}.') -f
    $actualIDs.Count,
    $voiceIDs.Count,
    $movementIDs.Count,
    $koIDs.Count,
    $attackIDs.Count,
    $hitIDs.Count,
    $metadata.resident_bytes,
    $metadata.unique_sample_count,
    $metadata.mapping_sha256_lo,
    $metadata.pack_sha256
)
