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
$expectedIDs = @(626, 470, 469, 467, 490)
$actualIDs = @($metadata.entries | ForEach-Object { [int]$_.id })
if (($actualIDs -join ',') -ne ($expectedIDs -join ',')) {
    throw "Unexpected FGM phase mapping: $($actualIDs -join ',')"
}
if ([int64]$metadata.resident_bytes -gt 65536) {
    throw "FGM phase pack exceeds 64 KiB: $($metadata.resident_bytes)"
}
if (([int]$metadata.format_version -ne 2) -or
    ([int]$metadata.entry_bytes -ne 32) -or
    ([int]$metadata.envelope_point_bytes -ne 4)) {
    throw 'Unexpected FGM phase-pack envelope format.'
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

Write-Output (
    ('Audio FGM phase pack passed: {0} exact IDs, {1} resident bytes, ' +
    'mapping {2}, pack {3}.') -f
    $actualIDs.Count,
    $metadata.resident_bytes,
    $metadata.mapping_sha256_lo,
    $metadata.pack_sha256
)
