param([string]$Python = 'python')

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$generator = Join-Path $PSScriptRoot 'sfx\render-audio-fgm-phase-pack.py'
$verifierPath = Join-Path $PSScriptRoot 'verify-audio-fgm-phase-pack.ps1'
$metadataPath = Join-Path $root 'assets/audio/fgm_phase_pack_ima.json'
$headerPath = Join-Path $root 'include/nds/nds_audio_fgm.h'
$runtimePath = Join-Path $root 'src/nds/nds_audio_fgm.c'

if ($null -eq (Get-Command $Python -ErrorAction SilentlyContinue)) {
    throw "Python command not found: $Python"
}
$verifier = Get-Content -LiteralPath $verifierPath -Raw
if (($verifier -notmatch '-MuteAudio') -or
    ($verifier -notmatch 'Audio FGM verification must use isolated runner slot')) {
    throw 'Audio FGM verifier lost its mute or isolated-runner guard.'
}
& $Python $generator --repo-root $root --check
if ($LASTEXITCODE -ne 0) {
    throw 'Generated FGM pack differs from its BattleShip sources.'
}

$metadata = Get-Content -LiteralPath $metadataPath -Raw | ConvertFrom-Json
$expectedIDs = @(626,470,469,467,490,74,363,364,372,373,374,430,439,292,
    370,289,300,303,154,77,215,40,38,37,34,32,31,375,429,431,435,440,
    19,41,42,43,185,186,187,189,190,217,218,219,216,28,2,0,188,
    436,432,362,433,360,12,285,
    # The announcer: TIME UP, GAME SET, "this game's winner is", and the two
    # fighter names the Results scene reads out.
    527,488,534,499,486,472,471,
    # And the crowd's win roar at Results scene start.
    621,
    # The reactive crowd: Fox/Mario chants, GaspL/M/S, Cheer, Amazed,
    # GaspClap, DamageL/M/S -- the eleven ft/ftpublic.c reaches in a P1 match.
    605, 609, 615, 616, 617, 618, 619, 620, 622, 623, 625,
    # The miss ring's three survivors: the ground grind, the altitude warning
    # (the pack's second DS hardware loop), and UnkGrind4 -- whose first note
    # asks for 90,510 Hz and therefore renders full-program AOT at 32,000.
    96, 153, 85,
    # And the five only a BOTH-CPU stress match reaches: dodge, shield on/off,
    # pause, and Fox's ledge teeter -- all core P1 gameplay.
    11, 13, 14, 278, 369)
$actualIDs = @($metadata.entries | ForEach-Object { [int]$_.id })
if (($actualIDs -join ',') -ne ($expectedIDs -join ',')) {
    throw "Unexpected FGM mapping: $($actualIDs -join ',')"
}
if (([int]$metadata.format_version -ne 4) -or
    ([int]$metadata.entry_bytes -ne 32) -or
    ([int]$metadata.envelope_point_bytes -ne 4) -or
    ([int64]$metadata.resident_bytes -ne 700892) -or
    ([int64]$metadata.resident_limit_bytes -ne 204800) -or
    # ROM, not RAM: the runtime streams cues into resident_limit_bytes and never
    # holds the pack. 512 KiB blocked the five announcer lines for no runtime
    # reason; the bound that is real is the 53,248-byte cache-slot gate below.
    ([int64]$metadata.pack_limit_bytes -ne 786432) -or
    ($metadata.mapping_sha256_lo -ne '0x333a47fb') -or
    ($metadata.pack_sha256 -ne
        '068631fd264dce18223e63966d8b4f883007e56d8f4dae5800237132bd74e447')) {
    throw 'FGM pack format, budget, mapping, or binary identity changed.'
}
if ((@($metadata.excluded_entries).Count -ne 0) -or
    (@($metadata.runtime_excluded_hit_ids).Count -ne 0)) {
    throw 'A battle-reachable FGM remains excluded.'
}
foreach ($entry in $metadata.entries) {
    if (([double]$entry.decoded_rms -le 0.0) -or
        ([int64]$entry.decoded_peak -le 0) -or
        ([double]$entry.ima_snr_db -lt 14.0) -or
        ([int64]$entry.ima_adpcm_bytes -gt 53248) -or
        ([int]$entry.packed_envelope_count -gt 32)) {
        throw "FGM $($entry.id) failed its acoustic/cache gate."
    }
    # A REST MUST NEVER SET THE PLAYBACK RATE. The DS pack plays one sample at
    # one rate, so the rate comes from a cue's first note -- and pitch code 0 is
    # a rest, not a note. FGM 488 GAME SET is the one P1 cue whose program opens
    # with one (a 60-tick rest, then the line at code 13), and it shipped at
    # 7,565 Hz against every other announcer line's 16,000: thirteen semitones
    # low, which is the owner's "sounds really low pitched" row in BUGS.md.
    #
    # Nothing caught it because the pack self-checks against its own derivation
    # and the derivation had the same bug, so the guard has to be an external
    # bound. A rest at the announcer articulation lands on 7,565; the lowest
    # legitimately packed rate in this set is 15,102 (FGM 285, 626). 12,000 sits
    # between them with room on both sides.
    if ([int]$entry.ds_frequency_hz -lt 12000) {
        throw ("FGM $($entry.id) plays at $($entry.ds_frequency_hz) Hz, below " +
            'the 12,000 floor -- a rest pitch code most likely set the rate.')
    }
}
foreach ($id in @(154,40,38,37,34,32,31)) {
    $entry = $metadata.entries | Where-Object { [int]$_.id -eq $id }
    if ((@($entry.omitted_fork_programs).Count -ne 0) -or
        ($entry.acoustic_oracle.aot_strategy -ne
            'source_program_schedule_and_simultaneous_forks')) {
        throw "FGM $id did not ship its fused source fork."
    }
}
# BUGS.md #3.  Whispy's gust was the FIRST DS hardware loop in the pack (153
# AltitudeWarn is the second, pinned below), and it is the whole reason the
# hazard sounds for its full 470 ticks instead of puffing once.  Losing the
# loop flag was the original defect, so pin the flag, the PNT/LEN geometry,
# and the three DS repeat proofs together.
$fgm285 = $metadata.entries | Where-Object { [int]$_.id -eq 285 }
$oracle285 = $fgm285.acoustic_oracle
if (($fgm285.ds_loop_strategy -ne 'source_loop_ds_hardware') -or
    ([int]$fgm285.ds_loop_flag -ne 1) -or
    ([int]$fgm285.ds_loop_point_words -ne 1) -or
    ([int]$fgm285.ds_loop_length_words -ne 1663) -or
    ([int]$fgm285.ds_ima_loop_body_nibbles -ne 13304) -or
    (@($fgm285.ds_ima_guard_nibbles).Count -ne 0) -or
    ([int]$fgm285.ds_ima_header_predictor -ne 335) -or
    ([int]$fgm285.ds_ima_header_index -ne 56) -or
    ($oracle285.ds_repeat_oracle_model -ne
        'header_once_pnt_latch_len_restore') -or
    ($oracle285.ds_repeat_oracle_missing_restore_detected -ne $true) -or
    ($oracle285.ds_repeat_oracle_wrong_pnt_detected -ne $true) -or
    ($oracle285.ds_repeat_oracle_wrong_len_detected -ne $true)) {
    throw 'FGM 285 lost its proven DS hardware wind loop.'
}
# 153 AltitudeWarn, the second hardware loop and the cue that turned the
# WHISPY_* module constants into a per-cue spec. Its schedule (300 ticks,
# 1.725 s) outlives its sample (22,208 at 29,344 Hz, 0.757 s), so losing the
# loop flag here cuts the warning off two thirds of the way through -- exactly
# 285's failure, on a cue the owner would hear every time a fighter drops
# below the stage.
$fgm153 = $metadata.entries | Where-Object { [int]$_.id -eq 153 }
$oracle153 = $fgm153.acoustic_oracle
if (($fgm153.ds_loop_strategy -ne 'source_loop_ds_hardware') -or
    ([int]$fgm153.ds_loop_flag -ne 1) -or
    ([int]$fgm153.ds_loop_point_words -ne 1) -or
    ([int]$fgm153.ds_loop_length_words -ne 396) -or
    ([int]$fgm153.ds_ima_loop_body_nibbles -ne 3168) -or
    (@($fgm153.ds_ima_guard_nibbles).Count -ne 0) -or
    ([int]$fgm153.ds_ima_header_predictor -ne 5401) -or
    ([int]$fgm153.ds_ima_header_index -ne 43) -or
    ($oracle153.ds_repeat_oracle_model -ne
        'header_once_pnt_latch_len_restore') -or
    ($oracle153.ds_repeat_oracle_missing_restore_detected -ne $true) -or
    ($oracle153.ds_repeat_oracle_wrong_pnt_detected -ne $true) -or
    ($oracle153.ds_repeat_oracle_wrong_len_detected -ne $true)) {
    throw 'FGM 153 lost its DS hardware altitude-warning loop.'
}
# 85 UnkGrind4 is the pack's answer to `source_rate_above_u16`, and the answer
# is "do not store that rate at all". Its first note asks for 90,510 Hz --
# 32,000 * 2^(1800/1200) -- against a u16 `frequency` field, so it renders its
# whole three-note schedule AOT and the entry stores FGM_OUTPUT_RATE. Pin both
# halves: a regression that reverted the AOT strategy would store a truncated
# rate and the cue would play at some unrelated pitch rather than fail.
$fgm85 = $metadata.entries | Where-Object { [int]$_.id -eq 85 }
if (([int]$fgm85.ds_frequency_hz -ne 32000) -or
    ([int]$fgm85.net_pitch_cents -ne 1800) -or
    ($fgm85.ds_loop_strategy -ne 'source_program_aot') -or
    ([int]$fgm85.ds_loop_flag -ne 0) -or
    ([int]$fgm85.acoustic_oracle.aot_output_frequency_hz -ne 32000) -or
    ([int]$fgm85.acoustic_oracle.aot_output_samples -ne 2576)) {
    throw 'FGM 85 no longer renders its above-u16 note schedule AOT.'
}
$fgm218 = $metadata.entries | Where-Object { [int]$_.id -eq 218 }
if (($fgm218.acoustic_oracle.source_custom_fx_dry_only -ne $true) -or
    ([int]$metadata.attack_activation_qualification.fgm_218_feasibility.source_effective_fx_mix -ne 25)) {
    throw 'FGM 218 named dry custom-FX lever changed.'
}
$header = Get-Content -LiteralPath $headerPath -Raw
$runtime = Get-Content -LiteralPath $runtimePath -Raw
foreach ($token in @(
    '#define NDS_AUDIO_FGM_ENTRY_COUNT 83u',
    '#define NDS_AUDIO_FGM_PACK_BYTES 700892u',
    '#define NDS_AUDIO_FGM_PACK_MAPPING_SHA256_LO 0x333a47fbu',
    '#define NDS_AUDIO_FGM_CACHE_BYTES 204800u')) {
    if (-not $header.Contains($token)) { throw "Runtime header lost: $token" }
}
# A packed cue the allowlist never admits is dead ROM, and an admitted cue with
# no pack entry fails closed and is silent -- both were live defects. Keep the
# two lists in step for the announcer set at least.
foreach ($voice in @('nSYAudioVoiceAnnounceTimeUp', 'nSYAudioVoiceAnnounceGameSet',
    'nSYAudioVoiceAnnounceWinnerIs', 'nSYAudioVoiceAnnounceMario',
    'nSYAudioVoiceAnnounceFox', 'nSYAudioVoiceAnnounceFive',
    'nSYAudioVoiceAnnounceFour', 'nSYAudioVoicePublicWin',
    'nSYAudioVoicePublicFox', 'nSYAudioVoicePublicMario',
    'nSYAudioVoicePublicGaspL', 'nSYAudioVoicePublicGaspM',
    'nSYAudioVoicePublicGaspS', 'nSYAudioVoicePublicCheer',
    'nSYAudioVoicePublicAmazed', 'nSYAudioVoicePublicGaspClap',
    'nSYAudioVoicePublicDamageL', 'nSYAudioVoicePublicDamageM',
    'nSYAudioVoicePublicDamageS', 'nSYAudioFGMGroundGrind2',
    'nSYAudioFGMAltitudeWarn')) {
    if (-not $runtime.Contains("case ${voice}:")) {
        throw "Runtime allowlist does not admit the packed cue $voice."
    }
}
foreach ($token in @('fread(sNdsAudioFgmCacheSlots[best].data',
    'sNdsAudioFgmCacheSlots[cache_slot].references++',
    'sNdsAudioFgmCacheSlots[(u32)handle->cache_slot].references--')) {
    if (-not $runtime.Contains($token)) { throw "Runtime cache lost: $token" }
}

Write-Output (('Audio FGM full coverage passed: 83 IDs, 0 exclusions, ' +
    '700892-byte pack, 204800-byte cache, seven fused fork repairs, ' +
    'FGM 285 wind on a proven DS hardware loop, seven announcer lines, ' +
    'PublicWin 621 on PublicExcited''s AOT loop-and-ramp render.'))
