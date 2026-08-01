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
    527,488,534,499,486,472,471)
$actualIDs = @($metadata.entries | ForEach-Object { [int]$_.id })
if (($actualIDs -join ',') -ne ($expectedIDs -join ',')) {
    throw "Unexpected FGM mapping: $($actualIDs -join ',')"
}
if (([int]$metadata.format_version -ne 4) -or
    ([int]$metadata.entry_bytes -ne 32) -or
    ([int]$metadata.envelope_point_bytes -ne 4) -or
    ([int64]$metadata.resident_bytes -ne 535280) -or
    ([int64]$metadata.resident_limit_bytes -ne 204800) -or
    # ROM, not RAM: the runtime streams cues into resident_limit_bytes and never
    # holds the pack. 512 KiB blocked the five announcer lines for no runtime
    # reason; the bound that is real is the 53,248-byte cache-slot gate below.
    ([int64]$metadata.pack_limit_bytes -ne 786432) -or
    ($metadata.mapping_sha256_lo -ne '0x3097cf44') -or
    ($metadata.pack_sha256 -ne
        'c4b9fac5626ece86209c3fbfeab038f8845ad3faf52ac8f635a041de09728626')) {
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
# BUGS.md #3.  Whispy's gust is the only DS hardware loop in the pack, and it
# is the whole reason the hazard sounds for its full 470 ticks instead of
# puffing once.  Losing the loop flag was the original defect, so pin the flag,
# the PNT/LEN geometry, and the three DS repeat proofs together.
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
$fgm218 = $metadata.entries | Where-Object { [int]$_.id -eq 218 }
if (($fgm218.acoustic_oracle.source_custom_fx_dry_only -ne $true) -or
    ([int]$metadata.attack_activation_qualification.fgm_218_feasibility.source_effective_fx_mix -ne 25)) {
    throw 'FGM 218 named dry custom-FX lever changed.'
}
$header = Get-Content -LiteralPath $headerPath -Raw
$runtime = Get-Content -LiteralPath $runtimePath -Raw
foreach ($token in @(
    '#define NDS_AUDIO_FGM_ENTRY_COUNT 63u',
    '#define NDS_AUDIO_FGM_PACK_BYTES 535280u',
    '#define NDS_AUDIO_FGM_PACK_MAPPING_SHA256_LO 0x3097cf44u',
    '#define NDS_AUDIO_FGM_CACHE_BYTES 204800u')) {
    if (-not $header.Contains($token)) { throw "Runtime header lost: $token" }
}
# A packed cue the allowlist never admits is dead ROM, and an admitted cue with
# no pack entry fails closed and is silent -- both were live defects. Keep the
# two lists in step for the announcer set at least.
foreach ($voice in @('nSYAudioVoiceAnnounceTimeUp', 'nSYAudioVoiceAnnounceGameSet',
    'nSYAudioVoiceAnnounceWinnerIs', 'nSYAudioVoiceAnnounceMario',
    'nSYAudioVoiceAnnounceFox', 'nSYAudioVoiceAnnounceFive',
    'nSYAudioVoiceAnnounceFour')) {
    if (-not $runtime.Contains("case ${voice}:")) {
        throw "Runtime allowlist does not admit the packed cue $voice."
    }
}
foreach ($token in @('fread(sNdsAudioFgmCacheSlots[best].data',
    'sNdsAudioFgmCacheSlots[cache_slot].references++',
    'sNdsAudioFgmCacheSlots[(u32)handle->cache_slot].references--')) {
    if (-not $runtime.Contains($token)) { throw "Runtime cache lost: $token" }
}

Write-Output (('Audio FGM full coverage passed: 63 IDs, 0 exclusions, ' +
    '535280-byte pack, 204800-byte cache, seven fused fork repairs, ' +
    'FGM 285 wind on a proven DS hardware loop, seven announcer lines.'))
