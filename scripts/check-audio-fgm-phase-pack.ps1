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
    439, 292, 370, 289, 300, 154, 77, 215, 40, 38, 37, 34, 32, 31)
$actualIDs = @($metadata.entries | ForEach-Object { [int]$_.id })
if (($actualIDs -join ',') -ne ($expectedIDs -join ',')) {
    throw "Unexpected FGM mapping: $($actualIDs -join ',')"
}
if (([int64]$metadata.resident_bytes -ne 121720) -or
    ([int64]$metadata.resident_limit_bytes -ne 131072)) {
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
if (($voiceIDs -join ',') -ne '363,364,372,430') {
    throw "Unexpected fighter-voice subset: $($voiceIDs -join ',')"
}
if (($movementIDs -join ',') -ne '74,300') {
    throw "Unexpected Fox-movement subset: $($movementIDs -join ',')"
}
if (($marioIDs -join ',') -ne '77') {
    throw "Unexpected Mario movement subset: $($marioIDs -join ',')"
}
if (($attackIDs -join ',') -ne '215') {
    throw "Unexpected attack/activation subset: $($attackIDs -join ',')"
}
if (($hitIDs -join ',') -ne '40,38,37,34,32,31') {
    throw "Unexpected collision/contact subset: $($hitIDs -join ',')"
}
if (([int]$metadata.unique_sample_count -ne 18) -or
    ([int]$metadata.unique_sample_bytes -ne 120668)) {
    throw 'FGM sample deduplication fixture changed.'
}
if (([int]$metadata.format_version -ne 3) -or
    ([int]$metadata.entry_bytes -ne 32) -or
    ($metadata.entry_final_u16 -ne 'ds_loop_point_words') -or
    ([int]$metadata.envelope_point_bytes -ne 4)) {
    throw 'Unexpected FGM pack entry/envelope format.'
}
if (($metadata.source_region -ne 'REGION_US') -or
    ($metadata.mapping_sha256_lo -ne '0x310f7e8d') -or
    ($metadata.pack_sha256 -ne
        '303e6f13092fc4d17ff026cf6b75be4f48a397ba23bad7251c9e4daa2c4bbb18')) {
    throw 'FGM pack mapping or binary hash changed.'
}
if (($metadata.non_loop_sample_sha256 -ne
        'c6c0a7c1caa0480077be544c96028d0e082be36fdd19cd08f7d41059c9597da7') -or
    ($metadata.non_loop_envelope_sha256 -ne
        'e8eb7bcb0a86d60698d2c13cd9dd0f34c2e42286ba402286cf82db7877064a06')) {
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
        Samples = 3760; Volume = 46; Envelope = 0; Hash =
        'ddbbc182a942902e0af8a9f83e5fc7240eaff0df7a5ac7b4d6f83f1ba757835c' },
    @{ ID = 364; Sound = 102; Frequency = 16000; Duration = 65;
        Samples = 2912; Volume = 89; Envelope = 0; Hash =
        '91a709f230f061152e96b1988f6f5a5b3c00820d9bbb7b6c06cce995edfa9dbf' },
    @{ ID = 372; Sound = 105; Frequency = 16000; Duration = 46;
        Samples = 1680; Volume = 108; Envelope = 0; Hash =
        'ac3948daf50a93899233040c52153d84e75417f112487cc33679e40623804fcf' },
    @{ ID = 430; Sound = 174; Frequency = 16280; Duration = 236;
        Samples = 17232; Volume = 86; Envelope = 1; Hash =
        '88ed6606f9199d787398a6d3b8563803cd4eddb4ccf7b9e4120e32d1f4292acd' }
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
    ([int]$marioLanding[0].ds_volume -ne 58) -or
    ([int]$marioLanding[0].ds_pan -ne 64) -or
    ([int]$marioLanding[0].ds_loop_point_words -ne 0) -or
    ([int]$marioLanding[0].packed_envelope_count -ne 0) -or
    ((@($marioLanding[0].root_fork_programs) -join ',') -ne '72') -or
    ((@($marioLanding[0].source_volume_envelope | ForEach-Object {
                [int]$_.tick }) -join ',') -ne '0,1,2') -or
    ((@($marioLanding[0].source_volume_envelope | ForEach-Object {
                [int]$_.ds_volume }) -join ',') -ne '21,86,41') -or
    ($marioLanding[0].ima_adpcm_sha256 -ne
        '4a7080cf1d004179dce9f7c770b32e155c96c8f12e641f349b8b2ebebc378b75')) {
    throw 'Mario landing source fixture changed.'
}
if (@($marioLanding[0].runtime_fidelity_debt).Count -ne 0) {
    throw 'Mario landing no longer has exact bounded pitch/envelope metadata.'
}
foreach ($id in 430) {
    $entry = $metadata.entries | Where-Object { [int]$_.id -eq $id }
    if (-not (@($entry.runtime_fidelity_debt) -contains
            'ucd_pitch_automation')) {
        throw "Mario voice $id no longer records pitch-automation debt."
    }
}
$expectedAttackSounds = @(
    @{ ID = 215; Sound = 19; Frequency = 32000; Duration = 15;
        Samples = 2176; Volume = 121; Hash =
        '7ed82ac09a350207bb4107b598447567516fd03ad40324953d041507a234ef78' }
)
foreach ($expected in $expectedAttackSounds) {
    $entry = @($metadata.entries | Where-Object {
            [int]$_.id -eq $expected.ID
        })
    if (($entry.Count -ne 1) -or
        ([int]$entry[0].source_sound_index -ne $expected.Sound) -or
        ([int]$entry[0].ds_frequency_hz -ne $expected.Frequency) -or
        ([int]$entry[0].source_duration_ticks -ne $expected.Duration) -or
        ([int]$entry[0].ds_sample_count -ne $expected.Samples) -or
        ([int]$entry[0].ds_volume -ne $expected.Volume) -or
        ([int]$entry[0].packed_envelope_count -ne 0) -or
        ([int]$entry[0].ds_loop_point_words -ne 0) -or
        ($entry[0].ima_adpcm_sha256 -ne $expected.Hash) -or
        (@($entry[0].runtime_fidelity_debt).Count -ne 0)) {
        throw "Attack/activation FGM $($expected.ID) source fixture changed."
    }
}

$qualification = $metadata.attack_activation_qualification
if (($qualification.sha256 -ne
        '8e520123996038b06edbd9cd2c3194734b9d7d08bde89159271ff3872a15e69e') -or
    ($qualification.source_action_audit.sha256 -ne
        'ae7690adc1d646e8c0a755510064a324c6ff59f4f578a2f6fdd719351744c601') -or
    ((@($qualification.qualified_ids | ForEach-Object { [int]$_ }) -join ',') -ne
        '215') -or
    ((@($qualification.excluded_ids | ForEach-Object { [int]$_ }) -join ',') -ne
        '19,41,42,43,185,186,187,189,190,217,218,219') -or
    (@($qualification.cues).Count -ne 13) -or
    ([int]$qualification.source_action_audit.callsite_count -ne 60) -or
    ([int]$qualification.source_action_audit.action_count -ne 66) -or
    (@($qualification.source_action_audit.callsites).Count -ne 60) -or
    (@($qualification.source_action_audit.actions).Count -ne 66)) {
    throw 'Attack/activation source qualification set changed.'
}
$residentAttackIDs = @($attackIDs | ForEach-Object { [int]$_ })
if (@($qualification.excluded_ids | Where-Object {
            $residentAttackIDs -contains [int]$_
        }).Count -ne 0) {
    throw 'A source-incomplete attack cue leaked back into the resident pack.'
}

$expectedBlockers = @{
    '19' = 'ucd_pitch_schedule,articulation_pitch_and_volume_schedule,resident_pack_cap'
    '41' = 'source_custom_fx_bus,resident_pack_cap'
    '42' = 'source_custom_fx_bus'
    '43' = 'source_custom_fx_bus'
    '185' = 'source_sample_loop,articulation_infinite_pitch_and_volume_loop,resident_pack_cap'
    '186' = 'ucd_pitch_schedule,articulation_volume_schedule,resident_pack_cap'
    '187' = 'simultaneous_fork_voice_0,ucd_pitch_schedule,fork_volume_schedule,fork_source_custom_fx_bus,resident_pack_cap'
    '189' = 'ucd_t5_pitch_schedule,source_rate_above_u16,ucd_volume_schedule,source_custom_fx_bus'
    '190' = 'ucd_pitch_schedule,source_rate_above_u16,source_custom_fx_bus'
    '215' = ''
    '217' = 'ucd_pitch_schedule,articulation_spawn_mod_47,resident_pack_cap'
    '218' = 'ucd_pitch_schedule,aot_custom_fx_tail_exceeds_resident_pack_cap,source_overlap_exceeds_handle_capacity'
    '219' = 'ucd_pitch_schedule,source_rate_above_u16,source_custom_fx_bus'
}
foreach ($cue in $qualification.cues) {
    $id = ([int]$cue.id).ToString()
    if (-not $expectedBlockers.ContainsKey($id) -or
        ((@($cue.blockers) -join ',') -ne $expectedBlockers[$id]) -or
        ([bool]$cue.qualified -ne ($id -eq '215')) -or
        ([int]$cue.effective_fgm_pan -ne 64)) {
        throw "Attack/activation FGM $id blocker audit changed."
    }
}

$expectedDirectCounts = @{
    '19' = 4; '41' = 17; '42' = 21; '43' = 11; '185' = 2;
    '186' = 1; '187' = 2; '189' = 1; '190' = 7; '215' = 1;
    '217' = 1; '218' = 2; '219' = 2
}
foreach ($id in $expectedDirectCounts.Keys) {
    $property = $qualification.source_action_audit.direct_call_counts.PSObject.Properties[$id]
    if (($null -eq $property) -or
        ([int]$property.Value -ne $expectedDirectCounts[$id])) {
        throw "REGION_US FGM $id direct motion-call count changed."
    }
}

$actionFixtures = @(
    @{ Name = 'dMarioMainMotion_FireballGround'; ID = 215; Ticks = '16';
        Callsite = 'dMarioMainMotion_0x1688' },
    @{ Name = 'dMarioMainMotion_FireballAir'; ID = 215; Ticks = '16';
        Callsite = 'dMarioMainMotion_0x1688' },
    @{ Name = 'dFoxMainMotion_ReadyingFireFoxGround'; ID = 186; Ticks = '0';
        Callsite = 'dFoxMainMotion_FireFoxStartAerialBody' },
    @{ Name = 'dFoxMainMotion_ReadyingFireFoxAir'; ID = 186; Ticks = '0';
        Callsite = 'dFoxMainMotion_FireFoxStartAerialBody' },
    @{ Name = 'dFoxMainMotion_AttackAirD'; ID = 190;
        Ticks = '4,7,10,13,16,19,22'; Callsite = 'dFoxMainMotion_AttackAirD' },
    @{ Name = 'dMarioMainMotion_AttackAirD'; ID = 219;
        Ticks = '10,13,16,19,22,25,28,31';
        Callsite = 'dMarioMainMotion_AttackAirD' },
    @{ Name = 'dMarioMainMotion_MarioTornadoGround'; ID = 218;
        Ticks = '4,7,10,13,16,19,22,25,28,31,34,37,40';
        Callsite = 'dMarioMainMotion_MarioTornadoGround' }
)
foreach ($expected in $actionFixtures) {
    $action = @($qualification.source_action_audit.actions | Where-Object {
            $_.action -eq $expected.Name
        })
    $events = @($action.events | Where-Object {
            [int]$_.fgm_id -eq $expected.ID
        })
    if (($action.Count -ne 1) -or
        ((@($events.trigger_tick | ForEach-Object { [int]$_ }) -join ',') -ne
            $expected.Ticks) -or
        (@($events | Where-Object {
                $_.callsite -ne $expected.Callsite
            }).Count -ne 0)) {
        throw "REGION_US action timing changed: $($expected.Name)"
    }
}

$fireballAudit = $qualification.cues | Where-Object { [int]$_.id -eq 215 }
$fireballSound = $fireballAudit.voice.articulation.triggered_sounds[0]
if (([int]$fireballAudit.voice.duration_ticks -ne 15) -or
    ((@($fireballAudit.voice.notes.initial_source_frequency_hz) -join ',') -ne
        '32000,32000') -or
    ([int]$fireballAudit.voice.articulation.articulation_index -ne 42) -or
    ($fireballAudit.voice.articulation.program_sha256 -ne
        '78e320e6ee2a2832cb2f3635016b5b46d13fa820dccf4651d7effcd36ee5c7dd') -or
    ([int]$fireballSound.sound_index -ne 19) -or
    ([int]$fireballSound.source_pcm_samples -ne 2176) -or
    ([int]$fireballSound.ds_ima_bytes_if_resident -ne 1092) -or
    ($fireballSound.ds_ima_sha256_if_resident -ne
        '7ed82ac09a350207bb4107b598447567516fd03ad40324953d041507a234ef78') -or
    ($null -ne $fireballSound.source_loop) -or
    (@($fireballAudit.voice.forks).Count -ne 0)) {
    throw 'FGM 215 exact source-behavior audit changed.'
}
$shineAudit = $qualification.cues | Where-Object { [int]$_.id -eq 189 }
if (([int64]$shineAudit.voice.notes[6].initial_source_frequency_hz -ne 85430) -or
    (-not [bool]$shineAudit.voice.notes[6].frequency_exceeds_u16)) {
    throw 'Fox Shine source rate above u16 was lost or truncated.'
}
$fx = $qualification.source_custom_fx_bus_contract
if (($fx.source_fx_type -ne 'AL_FX_CUSTOM') -or
    ($fx.source_articulation_opcode -ne 'unk36') -or
    ([int]$fx.source_default_voice_fx -ne 64) -or
    ([int]$fx.source_effect_section_count -ne 14) -or
    ([int]$fx.source_effect_delay_samples -ne 19200) -or
    ([int]$fx.source_effect_latest_nonzero_gain_output_tap_samples -ne 17600) -or
    ([int]$fx.source_effect_nonzero_feedback_sections -ne 14) -or
    ($fx.source_effect_table_sha256 -ne
        '79e344b1ae8576be2a32997c49fabc5a8c1dcb1d2a87e89ec2a4e6d5c6abd934') -or
    ($fx.source_effect_state_scope -ne 'shared aux-bus circular delay') -or
    ([int]$fx.ds_resident_pack_fx_fields -ne 0) -or
    ($fx.ds_runtime_behavior -ne 'dry hardware channel')) {
    throw 'BattleShip custom FX-bus exclusion contract changed.'
}

$fgm218 = $qualification.fgm_218_feasibility
$fgm218Ticks = '4,7,10,13,16,19,22,25,28,31,34,37,40'
if (($fgm218.decision -ne 'fail_closed') -or
    [bool]$fgm218.qualified -or
    ([int]$fgm218.id -ne 218) -or
    (@($fgm218.source_actions).Count -ne 2) -or
    ((@($fgm218.source_actions.action) -join ',') -ne
        'dMarioMainMotion_MarioTornadoGround,dMarioMainMotion_0x1884') -or
    (@($fgm218.source_actions | Where-Object {
                (@($_.trigger_ticks) -join ',') -ne $fgm218Ticks
            }).Count -ne 0) -or
    ([int]$fgm218.source_calls_per_action -ne 13) -or
    ([int]$fgm218.source_retrigger_period_ticks -ne 3) -or
    ([int]$fgm218.source_voice_duration_ticks -ne 27) -or
    ([int]$fgm218.source_max_live_voices -ne 9) -or
    ([int]$fgm218.ds_handle_capacity -ne 8) -or
    ([int]$fgm218.overlap_handle_shortfall -ne 1) -or
    ([int]$fgm218.source_articulation_fx -ne 100) -or
    ([int]$fgm218.source_inherited_voice_fx -ne 64) -or
    ([int]$fgm218.source_effective_fx_mix -ne 25) -or
    ([int]$fgm218.resident_bytes_before_candidate -ne 121720) -or
    ([int]$fgm218.resident_limit_bytes -ne 131072) -or
    ([int]$fgm218.resident_free_bytes_before_candidate -ne 9352) -or
    ([int]$fgm218.aot_pack_entry_bytes -ne 32) -or
    ([int]$fgm218.dry_aot_samples -ne 4968) -or
    ([int]$fgm218.dry_aot_ima_bytes -ne 2488) -or
    ([int]$fgm218.dry_projected_pack_bytes -ne 124240) -or
    ([int]$fgm218.dry_projected_headroom_bytes -ne 6832) -or
    ([int]$fgm218.minimum_wet_timeline_samples -ne 17601) -or
    ([int]$fgm218.minimum_wet_ima_bytes -ne 8804) -or
    ([int]$fgm218.minimum_wet_projected_pack_bytes -ne 130556) -or
    ([int]$fgm218.minimum_wet_pack_overflow_bytes -ne -516) -or
    [bool]$fgm218.runtime_conversion_allowed -or
    [bool]$fgm218.runtime_allocation_allowed -or
    ((@($fgm218.blockers) -join ',') -ne $expectedBlockers['218'])) {
    throw 'FGM 218 exact AOT feasibility boundary changed.'
}

$expectedFoxMovement = @(
    @{ ID = 74; Sound = 1; Frequency = 35919; Duration = 3;
        Samples = 621; Volume = 58; Envelope = 0;
        Root = 74; Render = 72; Hash =
        '4a7080cf1d004179dce9f7c770b32e155c96c8f12e641f349b8b2ebebc378b75' },
    @{ ID = 300; Sound = 28; Frequency = 16000; Duration = 25;
        Samples = 2301; Volume = 31; Envelope = 0;
        Root = 300; Render = 298; Hash =
        '020235d7fa1a69f813ba829ef481ea96651002d5e04b3566ba0633dcd731cf13' }
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

$excluded = @($metadata.excluded_entries)
if ((@($excluded | ForEach-Object { [int]$_.id }) -join ',') -ne
        '429,435') {
    throw 'Mario pitch-automated exclusion set changed.'
}
$excludedSmash = @($excluded | Where-Object { [int]$_.id -eq 429 })[0]
$excludedJump = @($excluded | Where-Object { [int]$_.id -eq 435 })[0]
if (($excludedSmash.reason -ne
        'continuous_voice_pitch_schedule_not_representable') -or
    ([int]$excludedSmash.source_sound_index -ne 173) -or
    ([int]$excludedSmash.source_sample_count -ne 3648) -or
    ([int]$excludedSmash.source_duration_ticks -ne 96) -or
    ((@($excludedSmash.source_note_schedule | ForEach-Object {
                [int]$_.tick }) -join ',') -ne '0,6,26,56') -or
    ((@($excludedSmash.source_note_schedule | ForEach-Object {
                [int]$_.pitch_code }) -join ',') -ne '12,12,11,10') -or
    ((@($excludedSmash.source_articulation_pitch_schedule |
                ForEach-Object { "{0}:{1}" -f $_.tick, $_.cents }) -join ',') -ne
        '0:-1199') -or
    (@($excludedSmash.source_ucd_pan_ops).Count -ne 0) -or
    (@($excludedSmash.source_articulation_pan_ops).Count -ne 0) -or
    (@($excludedSmash.source_fork_programs).Count -ne 0) -or
    (@($excludedSmash.source_spawn_mod_ops).Count -ne 0)) {
    throw 'Excluded Mario Smash1 source behavior changed.'
}
if (($excludedJump.reason -ne
        'combined_ucd_and_articulation_pitch_schedule_not_representable') -or
    ([int]$excludedJump.source_sound_index -ne 179) -or
    ([int]$excludedJump.source_sample_count -ne 3344) -or
    ([int]$excludedJump.source_duration_ticks -ne 96) -or
    ((@($excludedJump.source_note_schedule | ForEach-Object {
                [int]$_.tick }) -join ',') -ne '0,6,26,56') -or
    ((@($excludedJump.source_note_schedule | ForEach-Object {
                [int]$_.pitch_code }) -join ',') -ne '12,12,12,9') -or
    ((@($excludedJump.source_articulation_pitch_schedule |
                ForEach-Object { "{0}:{1}" -f $_.tick, $_.cents }) -join ',') -ne
        '0:-1160,10:-1190') -or
    ((@($excludedJump.source_articulation_volume_schedule |
                ForEach-Object { "{0}:{1}:{2}" -f
                    $_.tick, $_.value, $_.duration_ticks }) -join ',') -ne
        '0:110:10,10:70:20,30:30:60') -or
    (@($excludedJump.source_ucd_pan_ops).Count -ne 0) -or
    (@($excludedJump.source_articulation_pan_ops).Count -ne 0) -or
    (@($excludedJump.source_fork_programs).Count -ne 0) -or
    (@($excludedJump.source_spawn_mod_ops).Count -ne 0)) {
    throw 'Excluded Mario Jump source behavior changed.'
}

$expectedFoxActions = @{
    74 = [PSCustomObject]@{
        File = 'decomp/src/relocData/208_FoxMainMotion.c'
        Forks = '72'
        Actions = @(
            'dFoxMainMotion_LandingAirX_0x010C|0|ftMotionPlayFGM',
            'dFoxMainMotion_LandingAirX_0x0124|0|ftMotionPlayFGM',
            'dFoxMainMotion_LandingAirX_0x018C|0|ftMotionPlayFGM',
            'dFoxMainMotion_LandingAirF|0|ftMotionPlayFGM',
            'dFoxMainMotion_LandingAirB|0|ftMotionPlayFGM',
            'dFoxMainMotion_LandingAirX_0x177C|0|ftMotionPlayFGM')
    }
    363 = [PSCustomObject]@{
        File = 'decomp/src/relocData/208_FoxMainMotion.c'
        Forks = ''
        Actions = @('dFoxMainMotion_JumpAerialB|0|ftMotionPlayVoice')
    }
    364 = [PSCustomObject]@{
        File = 'decomp/src/relocData/208_FoxMainMotion.c'
        Forks = ''
        Actions = @(
            'dFoxMainMotion_TechB_0x418|4|ftMotionPlayVoice',
            'dFoxMainMotion_RollB|4|ftMotionPlayVoice')
    }
    300 = [PSCustomObject]@{
        File = 'decomp/src/ft/ftcommon/ftcommondownwaitbounce.c'
        Forks = '298'
        Actions = @(
            'nFTCommonStatusDownBounceU|0|func_800269C0_275C0',
            'nFTCommonStatusDownBounceD|0|func_800269C0_275C0')
    }
}
foreach ($id in 74, 363, 364, 300) {
    $entry = $metadata.entries | Where-Object { [int]$_.id -eq $id }
    $expected = $expectedFoxActions[$id]
    $actualActions = @($entry.source_actions | ForEach-Object {
            '{0}|{1}|{2}' -f $_.action, $_.trigger_game_tick, $_.call
        })
    if (($entry.source_action_file -ne $expected.File) -or
        (($actualActions -join ',') -ne ($expected.Actions -join ',')) -or
        ((@($entry.root_fork_programs) -join ',') -ne $expected.Forks) -or
        (@($entry.omitted_fork_programs).Count -ne 0) -or
        (@($entry.runtime_fidelity_debt).Count -ne 0) -or
        ([int]$entry.sound_sample_pan -ne 63) -or
        ([int]$entry.ds_pan -ne 64) -or
        [bool]$entry.source_loop_infinite) {
        throw "Fox FGM $id source-action behavior audit changed."
    }
}
$foxLanding = $metadata.entries | Where-Object { [int]$_.id -eq 74 }
$foxJump = $metadata.entries | Where-Object { [int]$_.id -eq 363 }
$foxEscape = $metadata.entries | Where-Object { [int]$_.id -eq 364 }
if ((@($foxLanding.source_volume_envelope | ForEach-Object {
                "$($_.tick):$($_.ds_volume)" }) -join ',') -ne
        '0:21,1:86,2:41' -or
    (@($foxJump.source_volume_envelope | ForEach-Object {
                "$($_.tick):$($_.ds_volume)" }) -join ',') -ne '0:76' -or
    (@($foxEscape.source_volume_envelope | ForEach-Object {
                "$($_.tick):$($_.ds_volume)" }) -join ',') -ne
        '0:42,5:84,10:106,25:84,40:67,45:0') {
    throw 'Fox landing/jump/escape source volume schedules changed.'
}

$expectedSourceAot = @(
    @{ ID = 74; Samples = 621; Frequency = 35919; Volume = 58;
        Ticks = 3; ScheduleTicks = '0,1,2'; ArtVolumes = '30,125,60';
        Schedule = '38f8d400b4422667e462c7b42a82878f18e4bcbc11a73ce9584e5f52583c0cc7';
        Pcm = '5a1d8761715155f8bec55a5b2b8337ec153ac37f3834ecbe6403b7d8c7c2bd6c';
        Negative = '4aea57033e87121cb0958dfe6465c02395907039a1f9f748902846d584921d7b' },
    @{ ID = 77; Samples = 621; Frequency = 35919; Volume = 58;
        Ticks = 3; ScheduleTicks = '0,1,2'; ArtVolumes = '30,125,60';
        Schedule = '38f8d400b4422667e462c7b42a82878f18e4bcbc11a73ce9584e5f52583c0cc7';
        Pcm = '5a1d8761715155f8bec55a5b2b8337ec153ac37f3834ecbe6403b7d8c7c2bd6c';
        Negative = '4aea57033e87121cb0958dfe6465c02395907039a1f9f748902846d584921d7b' },
    @{ ID = 363; Samples = 3760; Frequency = 16000; Volume = 46;
        Ticks = 45; ScheduleTicks = '0'; ArtVolumes = '90';
        Schedule = 'e514fee28bb66ec0cb1617b4006f1c3e2c894c41b610ea0ae1e7ce906d986446';
        Pcm = '4c1c69a5a3819a914c59f4628f197b8c2f3eebfe5a2a032861703ea80f2c3f0a';
        Negative = '704d9d0651a842dc360ad0d6937c05b46715049694195e7be738e7b628aa87e9' },
    @{ ID = 364; Samples = 2912; Frequency = 16000; Volume = 89;
        Ticks = 65; ScheduleTicks = '0,5,10,25,40,45';
        ArtVolumes = '50,100,127,100,80,0';
        Schedule = '16a887d08ee23c397052706cd111caf8fcf46e40648cef8ebe7b2c580823ca2c';
        Pcm = '611f64f616ccf4798a60cc5efff85e3f6541c9e057c4825b50b6400f17f07dac';
        Negative = 'a6b0fdf87fe374da6ad2bc8b3353cb06ee65c1dc67040b592e719c959f41912a' }
)
foreach ($expected in $expectedSourceAot) {
    $entry = $metadata.entries | Where-Object { [int]$_.id -eq $expected.ID }
    $oracle = $entry.acoustic_oracle
    $schedule = @($oracle.source_effective_tick_schedule)
    if (($entry.trim_strategy -ne
            'source_articulation_pitch_volume_schedule_aot') -or
        (-not [bool]$entry.trim_applied) -or
        [bool]$entry.trim_retained_prefix_exact -or
        ([int]$entry.trim_proven_reachable_samples -ne $expected.Samples) -or
        ($oracle.aot_strategy -ne
            'source_articulation_pitch_volume_schedule') -or
        [bool]$oracle.aot_runtime_automation -or
        ([int]$oracle.aot_output_samples -ne $expected.Samples) -or
        ([int]$oracle.aot_output_frequency_hz -ne $expected.Frequency) -or
        ([int]$oracle.aot_constant_hardware_volume -ne $expected.Volume) -or
        ([int]$oracle.aot_full_tick_count -ne $expected.Ticks) -or
        ($null -ne $oracle.aot_modulator) -or
        ($oracle.aot_volume_model -ne
            'source_quadratic_n_micro_184_sample_ramps') -or
        ([int]$oracle.aot_ramp_output_rate_hz -ne 32000) -or
        ([int]$oracle.aot_ramp_samples -ne 184) -or
        ($oracle.aot_schedule_sha256 -ne $expected.Schedule) -or
        ($oracle.aot_rendered_pcm_sha256 -ne $expected.Pcm) -or
        (-not [bool]$oracle.aot_step_volume_negative_rejected) -or
        ($oracle.aot_step_volume_negative_pcm_sha256 -ne
            $expected.Negative) -or
        ((@($schedule | ForEach-Object { $_.tick }) -join ',') -ne
            $expected.ScheduleTicks) -or
        ((@($schedule | ForEach-Object { $_.articulation_volume }) -join
                ',') -ne $expected.ArtVolumes)) {
        throw "FGM $($expected.ID) source articulation AOT proof changed."
    }
}

$foxLandingBody = $metadata.entries | Where-Object { [int]$_.id -eq 74 }
$marioLandingBody = $metadata.entries | Where-Object { [int]$_.id -eq 77 }
if (([int]$foxLandingBody.pack_data_offset -ne
        [int]$marioLandingBody.pack_data_offset) -or
    ($foxLandingBody.ima_adpcm_sha256 -ne
        $marioLandingBody.ima_adpcm_sha256) -or
    (-not [bool]$marioLandingBody.sample_body_deduplicated) -or
    (@($marioLandingBody.runtime_fidelity_debt).Count -ne 0)) {
    throw 'Mario/Fox landing AOT sample-body reuse changed.'
}

$foxBounce = $metadata.entries | Where-Object { [int]$_.id -eq 300 }
$bounceOracle = $foxBounce.acoustic_oracle
$bounceSchedule = @($bounceOracle.source_effective_tick_schedule)
if (($foxBounce.trim_strategy -ne
        'source_articulation_pitch_volume_schedule_aot') -or
    (-not [bool]$foxBounce.trim_applied) -or
    [bool]$foxBounce.trim_retained_prefix_exact -or
    ([int]$foxBounce.trim_proven_reachable_samples -ne 2301) -or
    ([int]$bounceOracle.aot_output_samples -ne 2301) -or
    ([int]$bounceOracle.aot_output_frequency_hz -ne 16000) -or
    ([int]$bounceOracle.aot_constant_hardware_volume -ne 31) -or
    ($bounceOracle.aot_strategy -ne
        'source_articulation_pitch_volume_schedule') -or
    ([int]$bounceOracle.aot_full_tick_count -ne 25) -or
    [bool]$bounceOracle.aot_runtime_automation -or
    ([int]$bounceOracle.aot_modulator_index -ne 22) -or
    ([int]$bounceOracle.aot_modulator.shape -ne 0) -or
    ([int]$bounceOracle.aot_modulator.target -ne 11) -or
    ([int]$bounceOracle.aot_modulator.init_phase -ne 49) -or
    ([double]$bounceOracle.aot_modulator.period -ne 100.0) -or
    ([double]$bounceOracle.aot_modulator.amplitude -ne 50.0) -or
    ([double]$bounceOracle.aot_modulator.offset -ne 50.0) -or
    ($bounceOracle.aot_schedule_sha256 -ne
        'afc23889a04c1cc57a7e2c561180a2e27c0f2082324ab77855cfd921f9b7f733') -or
    ($bounceOracle.aot_rendered_pcm_sha256 -ne
        '5ed13fa3154454b96c92de82488f6bc6dd6af4badb75f1a679bc4ad9e04991ae') -or
    ($bounceOracle.aot_volume_model -ne
        'source_quadratic_n_micro_184_sample_ramps') -or
    ([int]$bounceOracle.aot_ramp_output_rate_hz -ne 32000) -or
    ([int]$bounceOracle.aot_ramp_samples -ne 184) -or
    (-not [bool]$bounceOracle.aot_step_volume_negative_rejected) -or
    ($bounceOracle.aot_step_volume_negative_pcm_sha256 -ne
        '4d23cd5c8ec8fc457c2a92a2571e1ef0d4bf8d1bbaae64bd2528928c0cb531de') -or
    ($bounceSchedule.Count -ne 25) -or
    ((@($bounceSchedule | ForEach-Object { $_.ds_volume }) -join ',') -ne
        '56,63,63,63,61,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,47,63,63') -or
    ((@($bounceSchedule | ForEach-Object { $_.frequency_hz }) -join ',') -ne
        '16000,16000,16000,16000,16000,16000,16000,16000,16000,16000,16000,16000,16000,16000,16000,16000,16000,16000,16000,16000,16000,16000,15545,15545,15545') -or
    ($metadata.sources.fgm_unk.raw_sha256 -ne
        'adcf2e1ae1a8e1756b2158139a9050511c7ced8dec276d52c44599e43f77ac40') -or
    ($metadata.sources.source_sine_table.sha256 -ne
        'bc184c0dbd76adecf7ff264d39cc58456546173beba727f189d2716dd8eabf16')) {
    throw 'Fox DownBounce source LFO/pitch AOT proof changed.'
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
    @{ ID = 372; Source = 1680; Schedule = 4233; Current = 4233;
        Retained = 1680; Removed = 0; Prefix =
        '95a4e1fe34793dd32060f5eb090ed4331b45893005f548a80b37e6b6d280e070' },
    @{ ID = 430; Source = 17232; Schedule = 19182; Current = 22093;
        Retained = 17232; Removed = 0; Prefix =
        'ccd7fc9b7add5f8527d0c9ae3c192c8500d5b603d63d8fc1f40b3c2a1a51ac0f' },
    @{ ID = 215; Source = 2176; Schedule = 2761; Current = 2761;
        Retained = 2176; Removed = 0; Prefix =
        '8f6664be97363bc3e73916e0c177597ff99657c7bcece9632606109829952c68' },
    @{ ID = 439; Source = 9136; Schedule = 8634; Current = 8838;
        Retained = 8838; Removed = 298; Prefix =
        '21b2a92339ebff5a17bbd1c733d37e93f415e8d9ccd65d8d080c7b9cb68687ec' },
    @{ ID = 292; Source = 6688; Schedule = 5168; Current = 5167;
        Retained = 5168; Removed = 1520; Prefix =
        '3ab193e4365f0742a7319a436e54a44c7a3901fdff10e8082fe6488b3a373715' },
    @{ ID = 370; Source = 9808; Schedule = 11041; Current = 11041;
        Retained = 9808; Removed = 0; Prefix =
        '981080097d24ccd16a8de1c417f409bb4a2a8b38d273ce57d8df60d16e1ba39d' },
    @{ ID = 289; Source = 6688; Schedule = 5168; Current = 5167;
        Retained = 5168; Removed = 1520; Prefix =
        '3ab193e4365f0742a7319a436e54a44c7a3901fdff10e8082fe6488b3a373715' },
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
$expectedKo = @(
    @{ ID = 439; Sound = 183; Frequency = 16009; Duration = 96;
        Samples = 8838;
        Root = 439; Render = 439; Hash =
        'a71ef24f6f54df209e7b782f6322481aa73f7dda31128a6cb4151b70fc5a0f7f' },
    @{ ID = 292; Sound = 28; Frequency = 16951; Duration = 53;
        Samples = 5168;
        Root = 292; Render = 287; Hash =
        '445530cd55f889e36c2cb025cfddea47136d09d9a38c8a9489c42d306d586d41' },
    @{ ID = 370; Sound = 104; Frequency = 16000; Duration = 120;
        Samples = 9808;
        Root = 370; Render = 370; Hash =
        'e518d1106a41cf9c99eac91ab053304a515a04d6f5fc5a9c5ea845365518ea77' },
    @{ ID = 289; Sound = 28; Frequency = 16951; Duration = 53;
        Samples = 5168;
        Root = 289; Render = 287; Hash =
        '445530cd55f889e36c2cb025cfddea47136d09d9a38c8a9489c42d306d586d41' },
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
    ([int]$marioSlam.pack_data_offset -eq
        [int]$foxBounce.pack_data_offset) -or
    [bool]$marioSlam.sample_body_deduplicated -or
    (-not [bool]$foxSlam.sample_body_deduplicated) -or
    [bool]$foxBounce.sample_body_deduplicated) {
    throw 'Slam dedup or Fox DownBounce AOT sample ownership changed.'
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
    -not ($koDebt -contains 'articulation_pitch_automation') -or
    -not ($koDebt -contains 'articulation_volume_modulation')) {
    throw 'Regular-KO pitch/volume automation debt is no longer explicit.'
}

$runtimeHitIDs = @(40, 38, 37, 34, 32, 31)
$excludedIDs = @(216, 28, 2, 0, 188)
if (($metadata.strict_hit_contact_status -ne 'punch_kick_primary_aot') -or
    ((@($metadata.runtime_excluded_hit_ids) -join ',') -ne
        ($excludedIDs -join ','))) {
    throw 'Collision/contact runtime exclusion contract changed.'
}
if (@($metadata.entries | Where-Object {
            [int]$_.id -in $excludedIDs
        }).Count -ne 0) {
    throw 'An excluded special/projectile contact cue entered the runtime pack.'
}

$expectedRuntimeHits = @(
    @{ ID = 40; Sound = 11; Frequency = 25398; Duration = 68; Volume = 72;
        Samples = 16416; Envelope = 4; Fork = 655; Dedup = $false;
        PCM = 'aeaa1a24ade3d329567fa7097dafe302548f17660da5110d88fafb5e496b3a4f';
        IMA = '38d0b65d29a4ff3c75a8508e15940bd41d9d3ed3ecb7f10381f4c565a8a40053' },
    @{ ID = 38; Sound = 11; Frequency = 34896; Duration = 111; Volume = 121;
        Samples = 16416; Envelope = 7; Fork = 654; Dedup = $true;
        PCM = 'aeaa1a24ade3d329567fa7097dafe302548f17660da5110d88fafb5e496b3a4f';
        IMA = '38d0b65d29a4ff3c75a8508e15940bd41d9d3ed3ecb7f10381f4c565a8a40053' },
    @{ ID = 37; Sound = 11; Frequency = 45255; Duration = 155; Volume = 124;
        Samples = 16416; Envelope = 8; Fork = 653; Dedup = $true;
        PCM = 'aeaa1a24ade3d329567fa7097dafe302548f17660da5110d88fafb5e496b3a4f';
        IMA = '38d0b65d29a4ff3c75a8508e15940bd41d9d3ed3ecb7f10381f4c565a8a40053' },
    @{ ID = 34; Sound = 16; Frequency = 12788; Duration = 70; Volume = 92;
        Samples = 11216; Envelope = 7; Fork = 658; Dedup = $false;
        PCM = '39e32eef9f5ffff46c80140586c5d5b103f76c065257d4ccef1aa2856e85e438';
        IMA = '9af7a90e3e4f0b175e36c0bd53e4bd21342fa4f535eea89e316eeb64e51894a7' },
    @{ ID = 32; Sound = 16; Frequency = 14254; Duration = 111; Volume = 111;
        Samples = 11216; Envelope = 8; Fork = 657; Dedup = $true;
        PCM = '39e32eef9f5ffff46c80140586c5d5b103f76c065257d4ccef1aa2856e85e438';
        IMA = '9af7a90e3e4f0b175e36c0bd53e4bd21342fa4f535eea89e316eeb64e51894a7' },
    @{ ID = 31; Sound = 16; Frequency = 16000; Duration = 151; Volume = 124;
        Samples = 11216; Envelope = 8; Fork = 656; Dedup = $true;
        PCM = '39e32eef9f5ffff46c80140586c5d5b103f76c065257d4ccef1aa2856e85e438';
        IMA = '9af7a90e3e4f0b175e36c0bd53e4bd21342fa4f535eea89e316eeb64e51894a7' }
)
foreach ($expected in $expectedRuntimeHits) {
    $entry = $metadata.entries | Where-Object { [int]$_.id -eq $expected.ID }
    if (($null -eq $entry) -or ($entry.entry_kind -ne 'hit') -or
        ([int]$entry.source_sound_index -ne $expected.Sound) -or
        ([int]$entry.ds_frequency_hz -ne $expected.Frequency) -or
        ([int]$entry.source_duration_ticks -ne $expected.Duration) -or
        ([int]$entry.ds_initial_volume -ne $expected.Volume) -or
        ([int]$entry.ds_sample_count -ne $expected.Samples) -or
        ([int]$entry.packed_envelope_count -ne $expected.Envelope) -or
        ([bool]$entry.sample_body_deduplicated -ne $expected.Dedup) -or
        ($entry.source_pcm_sha256 -ne $expected.PCM) -or
        ($entry.ima_adpcm_sha256 -ne $expected.IMA) -or
        ((@($entry.root_fork_programs) -join ',') -ne "$($expected.Fork)") -or
        ((@($entry.omitted_fork_programs) -join ',') -ne "$($expected.Fork)") -or
        -not (@($entry.runtime_fidelity_debt) -contains
            "omitted_fork_voice_$($expected.Fork)")) {
        throw "Runtime collision/contact FGM $($expected.ID) changed."
    }
}

$excludedCues = @($metadata.excluded_hit_cues)
if (($excludedCues.Count -ne $excludedIDs.Count) -or
    ((@($excludedCues | ForEach-Object { [int]$_.id }) -join ',') -ne
        ($excludedIDs -join ','))) {
    throw 'Collision/contact source-audit inventory changed.'
}
if (([int]$metadata.source_custom_fx.parameter_count -ne 114) -or
    ($metadata.source_custom_fx.parameters_sha256 -ne
        '79e344b1ae8576be2a32997c49fabc5a8c1dcb1d2a87e89ec2a4e6d5c6abd934') -or
    ($metadata.source_custom_fx.scene_selection -ne
        'scmanager.c:syAudioSetFXType(AL_FX_CUSTOM)') -or
    ([int]$metadata.source_custom_fx.parameters[0] -ne 14) -or
    ([int]$metadata.source_custom_fx.parameters[1] -ne 19200) -or
    ([int]$metadata.source_custom_fx.parameters[113] -ne 20480) -or
    ([int]$metadata.source_custom_fx.output_rate_hz -ne 32000) -or
    ([int]$metadata.source_custom_fx.parameter_ramp_microseconds -ne 5750) -or
    ([int]$metadata.source_custom_fx.parameter_ramp_samples -ne 184) -or
    ([int]$metadata.source_custom_fx.start_voice_attack_microseconds -ne 0) -or
    ($metadata.source_custom_fx.volume_curve -ne
        '(signed_volume * signed_volume) >> 15') -or
    ($metadata.source_custom_fx.fx_mix_formula -ne
        'articulation_unk36 * (root_unk2c >> 1) >> 7')) {
    throw 'BattleShip AL_FX_CUSTOM source fixture changed.'
}
$feasibility = $metadata.hit_contact_feasibility_experiment
if (([int]$feasibility.id -ne 34) -or
    ($feasibility.decision -ne 'primary_source_aot') -or
    ([int]$feasibility.source_root_program -ne 34) -or
    ([int]$feasibility.source_fork_program -ne 658) -or
    ([int]$feasibility.source_root_duration_ticks -ne 70) -or
    ([int]$feasibility.source_fork_duration_ticks -ne 200) -or
    ([int]$feasibility.fused_minimum_samples -ne 36800) -or
    ([int]$feasibility.pack_headroom_bytes -ne 9352) -or
    ([int]$feasibility.fused_minimum_add_bytes -ne 18436) -or
    ([int]$feasibility.fused_minimum_total_bytes -ne 140156) -or
    ([int]$feasibility.fused_minimum_over_limit_bytes -ne 9084) -or
    ([int]$feasibility.paired_minimum_add_bytes -ne 24912) -or
    ([int]$feasibility.paired_minimum_total_bytes -ne 146632) -or
    ([int]$feasibility.paired_minimum_over_limit_bytes -ne 15560)) {
    throw 'ID 34 bounded exact-AOT feasibility result changed.'
}

$expectedHitAudits = @(
    @{ ID = 216; Programs = '216,668'; FX = 'none,none';
        Rates = '30204,15102,40317,40317|65875,36971'; New = '1,1,1,0|1,0'; Loops = '10:1625|none';
        VAD = '894ee3f14b5533298e37ad014f6703bd0af28d57e8159e62b1b258b38e3db2e9|b8c5c059d805808963704a675ed60374830cca01690e53ec5bec0b4f2fa08dc3';
        PCM = '1c0313ee3d0fd6ddca702fb7ae45641b880dd909cdcbc7875592956e036429c0|5fb5428b52610d9ca61bcb853cc57855d89032ef3960cc245f82b9e7ff57618c' },
    @{ ID = 28; Programs = '28'; FX = 'none';
        Rates = '32000,32000,32000'; New = '1,0,0'; Loops = '13840:27456';
        VAD = 'b130e97986180605fc55a1607cf94423d1991fab2ac011687eac8879c2a98d55';
        PCM = 'f675d07dc6ddabaf543672573a1e1bb1ce6b97266ddb07e882d755d51b82b27c' },
    @{ ID = 2; Programs = '2'; FX = '25';
        Rates = '50797,50797'; New = '1,0'; Loops = 'none';
        VAD = '104533679b9fae954fa607b25c505ebed02ffb41754fd5dbeb4b58a37efc12a3';
        PCM = 'cc12d0f5d5cdb3a33c7f1c8b55d5a57756305afbd23bf02ea8e197ebb4d636be' },
    @{ ID = 0; Programs = '0'; FX = '25';
        Rates = '21357,21357,21357'; New = '1,0,0'; Loops = 'none';
        VAD = '104533679b9fae954fa607b25c505ebed02ffb41754fd5dbeb4b58a37efc12a3';
        PCM = 'cc12d0f5d5cdb3a33c7f1c8b55d5a57756305afbd23bf02ea8e197ebb4d636be' },
    @{ ID = 188; Programs = '188'; FX = '35';
        Rates = '21357,23973,15102,17959,21357,17959,11986,11986,14254,16000,17959'; New = '1,0,0,0,0,0,0,0,0,0,0'; Loops = '100:7664';
        VAD = '8b4a86a4ebcf8b5e99f620d977bcf14c1d793821ad1c20bf1db41d5c5092a8be';
        PCM = 'ef6d2a36c61d89b242c96ee1369c7aa5f7c166d33fa1e7ccf94419731eb0a32d' }
)
$expectedCuts = @{
    40 = '1,1|0'; 38 = '1,1,1|0'; 37 = '1,1,1,1|0,0,0,0,0'
    34 = '1,1|0'; 32 = '1,1,1|0'; 31 = '1,1,1,1|0,0,0,0,0'
    216 = '1,1,0,0|0,0'; 28 = '0,0,0'; 2 = '0,0'; 0 = '0,0,0'
    188 = '0,0,0,0,0,0,0,0,0,0,0'
}
$expectedReleaseTicks = @{
    40 = '47,67|none'; 38 = '47,95,110|none'
    37 = '44,89,134,154|none,none,none,none,none'
    34 = '49,69|none'; 32 = '47,95,110|none'
    31 = '43,88,133,150|none,none,none,none,none'
    216 = '9,11,none,none|none,none'; 28 = 'none,none,none'
    2 = 'none,none'; 0 = 'none,none,none'
    188 = 'none,none,none,none,none,none,none,none,none,none,none'
}
foreach ($expected in $expectedHitAudits) {
    $entry = @($excludedCues | Where-Object { [int]$_.id -eq $expected.ID })
    if (($entry.Count -ne 1) -or [bool]$entry[0].runtime_included -or
        ([string]::IsNullOrWhiteSpace($entry[0].action_contract)) -or
        (@($entry[0].source_callsites).Count -eq 0) -or
        (@($entry[0].runtime_excluded_reasons).Count -eq 0)) {
        throw "Collision/contact FGM $($expected.ID) is not explicitly fail-closed."
    }
    $voices = @($entry[0].voices)
    $programs = @($voices | ForEach-Object { [int]$_.program_id }) -join ','
    $fx = @($voices | ForEach-Object {
            $schedule = @($_.articulation_fx_mix_schedule)
            if ($schedule.Count -eq 0) { 'none' }
            else { [string][int]$schedule[0].effective_fx_mix }
        }) -join ','
    $rates = @($voices | ForEach-Object {
            @($_.note_schedule | ForEach-Object {
                    [int]$_.initial_frequency_hz
                }) -join ','
        }) -join '|'
    $newVoices = @($voices | ForEach-Object {
            @($_.note_schedule | ForEach-Object {
                    [int][bool]$_.starts_new_voice
                }) -join ','
        }) -join '|'
    $cuts = @($voices | ForEach-Object {
            @($_.note_schedule | ForEach-Object {
                    [int][bool]$_.cut_before_note_end
                }) -join ','
        }) -join '|'
    $releaseTicks = @($voices | ForEach-Object {
            @($_.note_schedule | ForEach-Object {
                    if ($null -eq $_.release_ramp_start_tick) { 'none' }
                    else { [string][int]$_.release_ramp_start_tick }
                }) -join ','
        }) -join '|'
    $loops = @($voices | ForEach-Object {
            if ($null -eq $_.source_loop) { 'none' }
            else { '{0}:{1}' -f $_.source_loop.start, $_.source_loop.end }
        }) -join '|'
    $vad = @($voices | ForEach-Object { $_.source_vadpcm_sha256 }) -join '|'
    $pcm = @($voices | ForEach-Object { $_.source_pcm_sha256 }) -join '|'
    if (($programs -ne $expected.Programs) -or ($fx -ne $expected.FX) -or
        ($rates -ne $expected.Rates) -or ($newVoices -ne $expected.New) -or
        ($cuts -ne $expectedCuts[$expected.ID]) -or
        ($releaseTicks -ne $expectedReleaseTicks[$expected.ID]) -or
        ($loops -ne $expected.Loops) -or ($vad -ne $expected.VAD) -or
        ($pcm -ne $expected.PCM)) {
        throw "Collision/contact FGM $($expected.ID) source behavior changed."
    }
    foreach ($voice in $voices) {
        if ((@($voice.ucd_program).Count -eq 0) -or
            (@($voice.articulation_program).Count -eq 0) -or
            ($voice.ucd_program_sha256.Length -ne 64) -or
            ($voice.articulation_program_sha256.Length -ne 64) -or
            ($voice.adpcm_book_sha256.Length -ne 64) -or
            ([string]::IsNullOrWhiteSpace($voice.source_stop_retrigger_policy))) {
            throw "Collision/contact FGM $($expected.ID) lost exact program evidence."
        }
    }
}

$ftMainPath = Join-Path $root 'decomp/BattleShip-main/decomp/src/ft/ftmain.c'
$lbCommonPath = Join-Path $root 'decomp/BattleShip-main/decomp/src/lb/lbcommon.c'
$marioSpecialPath = Join-Path $root 'decomp/BattleShip-main/decomp/src/relocData/204_MarioSpecial1.c'
$foxSpecialPath = Join-Path $root 'decomp/BattleShip-main/decomp/src/relocData/210_FoxSpecial1.c'
$foxMotionPath = Join-Path $root 'decomp/BattleShip-main/decomp/src/relocData/208_FoxMainMotion.c'
$fireballPath = Join-Path $root 'decomp/BattleShip-main/decomp/src/wp/wpmario/wpmariofireball.c'
$ftMain = Get-Content -LiteralPath $ftMainPath -Raw
$lbCommon = Get-Content -LiteralPath $lbCommonPath -Raw
$marioSpecial = Get-Content -LiteralPath $marioSpecialPath -Raw
$foxSpecial = Get-Content -LiteralPath $foxSpecialPath -Raw
$foxMotion = Get-Content -LiteralPath $foxMotionPath -Raw
$fireball = Get-Content -LiteralPath $fireballPath -Raw
foreach ($fragment in @(
        'nSYAudioFGMPunchS', 'nSYAudioFGMPunchM', 'nSYAudioFGMPunchL',
        'nSYAudioFGMKickS', 'nSYAudioFGMKickM', 'nSYAudioFGMKickL',
        'nSYAudioFGMMarioSpecialHiCoin',
        'lbCommonMakePositionFGM(dFTMainHitCollisionFGMs[attack_coll->fgm_kind][attack_coll->fgm_level], fp->joints[nFTPartsJointTopN]->translate.vec.f.x);',
        'func_800269C0_275C0(wp_attack_coll->fgm_id);')) {
    if (-not $ftMain.Contains($fragment)) {
        throw "BattleShip fighter/weapon hit callsite changed: $fragment"
    }
}
foreach ($fragment in @(
        's32 balance = ((pos / 8000.0F) * 60.0F);',
        'balance = 64 - balance;', 'snd->balance = balance;',
        'func_800267F4_273F4(snd);')) {
    if (-not $lbCommon.Contains($fragment)) {
        throw "BattleShip positional FGM behavior changed: $fragment"
    }
}
if ((-not $marioSpecial.Contains('28,  /* sfx              : 10 */')) -or
    (-not $foxSpecial.Contains('2,  /* sfx              : 10 */')) -or
    (-not $fireball.Contains('func_800269C0_275C0(nSYAudioFGMExplodeS);')) -or
    (-not $foxMotion.Contains('ftMotionPlayFGM(nSYAudioFGMFoxSpecialLwHit),'))) {
    throw 'Mario fireball or Fox blaster/reflector hit action mapping changed.'
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
    ('Audio FGM pack passed: {0} exact runtime IDs plus {1} ' +
    'primary-source common contacts ({2} fighter voices, {3} Fox movement, ' +
    '{4} regular-KO, {5} attack/activation); {6} special/projectile ' +
    'contacts fail-closed, {7} resident bytes, {8} unique samples, ' +
    'mapping {9}, pack {10}.') -f
    ($actualIDs.Count - $runtimeHitIDs.Count),
    $runtimeHitIDs.Count,
    $voiceIDs.Count,
    $movementIDs.Count,
    $koIDs.Count,
    $attackIDs.Count,
    $excludedIDs.Count,
    $metadata.resident_bytes,
    $metadata.unique_sample_count,
    $metadata.mapping_sha256_lo,
    $metadata.pack_sha256
)
