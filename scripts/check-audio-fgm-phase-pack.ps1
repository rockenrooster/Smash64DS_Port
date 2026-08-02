param([string]$Python = 'python')

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$generator = Join-Path $PSScriptRoot 'sfx\render-audio-fgm-phase-pack.py'
$verifierPath = Join-Path $PSScriptRoot 'verify-audio-fgm-phase-pack.ps1'
$metadataPath = Join-Path $root 'assets/audio/fgm_phase_pack_ima.json'
$packPath = Join-Path $root 'assets/audio/fgm_phase_pack_ima.bin'
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
    11, 13, 14, 278, 369,
    # And the last two the ring named on the soak after those five landed:
    # the zoom pulse and Fox's win voice at Results.
    271, 368,
    # And three from the run in which every fireball spawned and the match went
    # to SUDDEN DEATH: a light swing, the Sudden Death call, Fox's select voice.
    18, 365, 514)
$actualIDs = @($metadata.entries | ForEach-Object { [int]$_.id })
if (($actualIDs -join ',') -ne ($expectedIDs -join ',')) {
    throw "Unexpected FGM mapping: $($actualIDs -join ',')"
}
if (([int]$metadata.format_version -ne 4) -or
    ([int]$metadata.entry_bytes -ne 32) -or
    ([int]$metadata.envelope_point_bytes -ne 4) -or
    # 725900 -> 725896 on 2026-08-02: FGM 430 and 439 moved onto the source
    # note schedule, which re-renders them and lands four bytes shorter.
    # 725896 -> 887160 later the same day: the seven defective crowd cues
    # (615/616/618/619/620/623/625) joined FULL_PROGRAM_AOT_IDS, which leaves
    # the shared-sample-37 dedup behind and is the whole point of the change.
    # 887160 -> 913168 on 2026-08-02: FGM 153 AltitudeWarn left the DS
    # hardware-repeat path for the schedule-walking AOT render, because a
    # hardware repeat cannot reproduce the pitch sweep its articulation puts
    # inside the loop. 0.108 s monotone -> 1.725 s swept.
    ([int64]$metadata.resident_bytes -ne 913168) -or
    ([int64]$metadata.resident_limit_bytes -ne 204800) -or
    # ROM, not RAM: the runtime streams cues into resident_limit_bytes and never
    # holds the pack. 512 KiB blocked the five announcer lines and 768 KiB then
    # blocked the seven crowd cues, both for no runtime reason; the bound that
    # is real is the 53,248-byte cache-slot gate below.
    ([int64]$metadata.pack_limit_bytes -ne 1048576) -or
    # 0x984c7da6 -> 0x4fb97922 -> 0xb6be788e on 2026-08-02: this hash covers the
    # cue SELECTOR table. 430/439 gained "aot_source_schedule", then the seven
    # crowd cues gained the full-program AOT render. A mapping change is
    # expected whenever a cue's render strategy changes and must never be
    # repinned without one.
    ($metadata.mapping_sha256_lo -ne '0x28f8ec2c') -or
    # Repinned 2026-08-02: FGM 11 (the rolling dodge) dropped 127 -> 96 on the
    # owner's ear via FGM_OWNER_VOLUME_TRIM. The previous pin was
    # 81b94d1f3178b6b57d998fb7d01fe1316e20ac46ce22ccb82800c6b02d26cb75, and it
    # is worth knowing that the first attempt at that trim did NOT move this
    # hash -- it edited the metadata dict instead of the `records` entry that
    # PACK_ENTRY.pack writes, so the manifest claimed 96 while the ROM still
    # played 127. An unchanged hash after an intended payload change is the
    # signal that the change did not land.
    ($metadata.pack_sha256 -ne
        'b56488dcd844274e125cdf61d5f6854a09bc20b054f6b1bae2e80dbbf029d34b')) {
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
# ...and the paragraph above is what this pin USED to enforce. It is kept
# because it is the reasoning that has to be answered, not because it was right:
# it says the sample is 22,208 at 29,344 Hz (0.757 s), and the artifact never
# was. A hardware repeat stores PNT + LEN, and 153 shipped PNT 1 word + LEN 396
# words -- about 3,168 samples, 0.108 s. The attack was gone and the comment
# describing 0.757 s was aspirational.
#
# 2026-08-02, and the owner found this one by ear out of all 88: articulation
# 150 opens `pitch 550` and then steps `pitch 2390` INSIDE mark_loop/jump_loop,
# roughly an octave and a half of sweep on repeat. That sweep is the altitude
# warning. A DS hardware repeat replays its body bit-identically, which this
# file already says for 621/626 -- "cannot ramp" -- so the cue came out as a
# 0.108 s monotone blip looping, and read as an unfamiliar sound rather than a
# missing one, because it fires on exactly the right trigger.
#
# It renders through FULL_PROGRAM_AOT_IDS now: the schedule is walked and the
# sweep baked into 55,200 samples at 32,000 Hz = 1.725 s, matching the source's
# declared duration exactly. Measured after the change, zero-crossing rate falls
# 1,820 Hz -> 1,063 Hz across the first second, so the slide is really in there.
# STILL OPEN and deliberately not pinned as fixed: the render decays to silence
# at ~1.05 s and pads the rest, and it no longer repeats. The source loops
# infinitely, so if the warning has to persist while a fighter is out of bounds,
# that is the next piece of work.
$fgm153 = $metadata.entries | Where-Object { [int]$_.id -eq 153 }
if (($fgm153.ds_loop_strategy -ne 'source_program_aot') -or
    ([int]$fgm153.ds_frequency_hz -ne 32000) -or
    ([int]$fgm153.ds_sample_count -ne 55200) -or
    (@($fgm153.runtime_fidelity_debt).Count -ne 0) -or
    ($fgm153.acoustic_oracle.aot_strategy -ne
        'source_program_schedule_and_simultaneous_forks')) {
    throw ('FGM 153 AltitudeWarn is not on the schedule-walking AOT render; a ' +
        'hardware repeat cannot repeat its pitch sweep.')
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
    '#define NDS_AUDIO_FGM_ENTRY_COUNT 88u',
    '#define NDS_AUDIO_FGM_CACHE_BYTES 204800u')) {
    if (-not $header.Contains($token)) { throw "Runtime header lost: $token" }
}
# NDS_AUDIO_FGM_PACK_BYTES and NDS_AUDIO_FGM_PACK_MAPPING_SHA256_LO are DERIVED
# from the pack, never pinned as text. They used to be pinned in two places --
# the manifest assertions above and a literal #define string here -- and on
# 2026-08-02 the size was moved in both while the hash was moved in neither, so
# this check actively REQUIRED the stale hash and passed. The runtime rejects the
# whole pack on either mismatch, so the ROM booted with all 88 cues silent and
# gNdsAudioFgmFormatFailCount 1. Comparing against the artifact cannot drift.
$packBytes = (Get-Item -LiteralPath $packPath).Length
$packHeaderBlob = [System.IO.File]::ReadAllBytes($packPath)[0..15]
$packSizeField = [System.BitConverter]::ToUInt32($packHeaderBlob, 8)
$packMappingLo = '0x{0:x8}' -f [System.BitConverter]::ToUInt32($packHeaderBlob, 12)
if ($packSizeField -ne $packBytes) {
    throw "Pack header size field $packSizeField disagrees with its own length $packBytes."
}
foreach ($pair in @(
    @{ Name = 'NDS_AUDIO_FGM_PACK_BYTES'; Want = "${packBytes}u" },
    @{ Name = 'NDS_AUDIO_FGM_PACK_MAPPING_SHA256_LO'; Want = "${packMappingLo}u" })) {
    $found = [regex]::Match($header, "#define $($pair.Name)\s+(\S+)")
    if (-not $found.Success) { throw "Runtime header lost: #define $($pair.Name)" }
    if ($found.Groups[1].Value -ne $pair.Want) {
        throw ("$($pair.Name) is $($found.Groups[1].Value) but the pack says $($pair.Want). " +
               'The runtime rejects the whole pack on this mismatch: the ROM boots SILENT. ' +
               "Set it in $headerPath and rebuild.")
    }
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
    'nSYAudioFGMAltitudeWarn', 'nSYAudioFGMMagnify',
    'nSYAudioVoiceFoxWin',
    'nSYAudioVoiceAnnounceSuddenDeath')) {
    if (-not $runtime.Contains("case ${voice}:")) {
        throw "Runtime allowlist does not admit the packed cue $voice."
    }
}
foreach ($token in @('fread(sNdsAudioFgmCacheSlots[best].data',
    'sNdsAudioFgmCacheSlots[cache_slot].references++',
    'sNdsAudioFgmCacheSlots[(u32)handle->cache_slot].references--')) {
    if (-not $runtime.Contains($token)) { throw "Runtime cache lost: $token" }
}

# Counted, not spelled out. This line said "725896-byte pack" and "seven fused
# fork repairs" while the artifact was 887160 bytes with twelve, which is the
# same drift that shipped a silent ROM this morning: a hand-written summary is
# a second copy of a fact and it rots.
$fusedForks = @($metadata.entries | Where-Object {
    @($_.root_fork_programs).Count -gt 0 -and
    @($_.omitted_fork_programs).Count -eq 0 }).Count
$stillOmitting = @($metadata.entries | Where-Object {
    @($_.omitted_fork_programs).Count -gt 0 }).Count
Write-Output (("Audio FGM full coverage passed: $($metadata.entries.Count) IDs, " +
    "0 exclusions, $($metadata.resident_bytes)-byte pack, " +
    "$($metadata.resident_limit_bytes)-byte cache, $fusedForks fused fork " +
    "repairs ($stillOmitting cue(s) still omit a fork voice), FGM 285 wind on " +
    'a proven DS hardware loop, seven announcer lines, PublicWin 621 on ' +
    'PublicExcited''s AOT loop-and-ramp render.'))
