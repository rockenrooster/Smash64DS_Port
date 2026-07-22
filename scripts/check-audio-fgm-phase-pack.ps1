param([string]$Python = 'python')

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$generator = Join-Path $PSScriptRoot 'render-audio-fgm-phase-pack.py'
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
    19,41,42,43,185,186,187,189,190,217,218,219,216,28,2,0,188)
$actualIDs = @($metadata.entries | ForEach-Object { [int]$_.id })
if (($actualIDs -join ',') -ne ($expectedIDs -join ',')) {
    throw "Unexpected FGM mapping: $($actualIDs -join ',')"
}
if (([int]$metadata.format_version -ne 4) -or
    ([int]$metadata.entry_bytes -ne 32) -or
    ([int]$metadata.envelope_point_bytes -ne 4) -or
    ([int64]$metadata.resident_bytes -ne 415432) -or
    ([int64]$metadata.resident_limit_bytes -ne 204800) -or
    ([int64]$metadata.pack_limit_bytes -ne 524288) -or
    ($metadata.mapping_sha256_lo -ne '0xde193efa') -or
    ($metadata.pack_sha256 -ne
        '8025f6bf2ec86656f42d4c8e5d25ac456fd2caf1e288c1e34d7ebb369b1e797d')) {
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
}
foreach ($id in @(154,40,38,37,34,32,31)) {
    $entry = $metadata.entries | Where-Object { [int]$_.id -eq $id }
    if ((@($entry.omitted_fork_programs).Count -ne 0) -or
        ($entry.acoustic_oracle.aot_strategy -ne
            'source_program_schedule_and_simultaneous_forks')) {
        throw "FGM $id did not ship its fused source fork."
    }
}
$fgm218 = $metadata.entries | Where-Object { [int]$_.id -eq 218 }
if (($fgm218.acoustic_oracle.source_custom_fx_dry_only -ne $true) -or
    ([int]$metadata.attack_activation_qualification.fgm_218_feasibility.source_effective_fx_mix -ne 25)) {
    throw 'FGM 218 named dry custom-FX lever changed.'
}
$header = Get-Content -LiteralPath $headerPath -Raw
$runtime = Get-Content -LiteralPath $runtimePath -Raw
foreach ($token in @(
    '#define NDS_AUDIO_FGM_ENTRY_COUNT 49u',
    '#define NDS_AUDIO_FGM_PACK_BYTES 415432u',
    '#define NDS_AUDIO_FGM_PACK_MAPPING_SHA256_LO 0xde193efau',
    '#define NDS_AUDIO_FGM_CACHE_BYTES 204800u')) {
    if (-not $header.Contains($token)) { throw "Runtime header lost: $token" }
}
foreach ($token in @('fread(sNdsAudioFgmCacheSlots[best].data',
    'sNdsAudioFgmCacheSlots[cache_slot].references++',
    'sNdsAudioFgmCacheSlots[(u32)handle->cache_slot].references--')) {
    if (-not $runtime.Contains($token)) { throw "Runtime cache lost: $token" }
}

Write-Output (('Audio FGM full coverage passed: 49 IDs, 0 exclusions, ' +
    '415432-byte pack, 204800-byte cache, seven fused fork repairs.'))
