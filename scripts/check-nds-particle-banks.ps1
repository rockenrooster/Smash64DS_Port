param([string]$Python = 'python')

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$generator = Join-Path $PSScriptRoot 'generate_nds_particle_banks.py'
$reportPath = Join-Path $root 'docs/optimization/NDS_PARTICLE_BANKS.generated.json'
$headerPath = Join-Path $root 'include/nds/generated/nds_particle_banks.generated.h'
$incPath = Join-Path $root 'src/nds/generated/nds_particle_banks.generated.inc'
$assetPath = Join-Path $root 'assets/particles/efcommon_particle_textures.ds.bin'

if ($null -eq (Get-Command $Python -ErrorAction SilentlyContinue)) {
    throw "Python command not found: $Python"
}

# The images in the efcommon .txb are stored linearly. The RDP applies its
# odd-line TMEM word swap on both the load and the fetch, so it cancels, and a
# converter that "un-swizzles" them comb-stripes every odd row -- which is what
# scripts/generate_task39_hit_sparks.py does. Pin the two guards that keep this
# generator honest: the recorded reason, and the decode-back self-check that
# would catch any texel-order regression.
$source = Get-Content -LiteralPath $generator -Raw
foreach ($guard in @(
    'Images are stored linearly',
    'decode-back error',
    'must not apply the swizzle')) {
    if (-not $source.Contains($guard)) {
        throw "Particle bank generator lost its texel-order guard: $guard"
    }
}
# An odd-row swizzle needs a row-parity test; the hit-sparks generator spells
# it `8 if y & 1 else 0`. This generator must never grow one.
if ($source -match 'y\s*&\s*1') {
    throw 'Particle bank generator grew an odd-row parity test.'
}

& $Python $generator --repo-root $root --check
if ($LASTEXITCODE -ne 0) {
    throw 'Generated particle bank pack differs from its BattleShip sources.'
}

$report = Get-Content -LiteralPath $reportPath -Raw | ConvertFrom-Json

# Enumeration. These are derived, not declared: the P1 effect seams come from
# generate_task39_effect_census.py and the rest is the bank's own spawn graph.
# A moved count is a finding to explain, never a number to edit into place.
if (([int]$report.source.script_count -ne 119) -or
    ([int]$report.source.texture_count -ne 47) -or
    (@($report.reach.reachable_scripts).Count -ne 55) -or
    (@($report.reach.packed_textures).Count -ne 23) -or
    (@($report.reach.p1_seams).Count -ne 22)) {
    throw ('Particle bank enumeration changed: ' +
        "$(@($report.reach.reachable_scripts).Count)/$([int]$report.source.script_count) scripts, " +
        "$(@($report.reach.packed_textures).Count)/$([int]$report.source.texture_count) textures, " +
        "$(@($report.reach.p1_seams).Count) seams.")
}

# These nine P1 seams route to the display-list effect path
# (efManagerMakeEffect*), not to the particle bank, so the bank cannot change
# what they draw. Pinned so a future edit cannot quietly claim otherwise.
$expectedDisplayListSeams = @(
    'efManagerCatchSwirlMakeEffect',
    'efManagerDamageSlashMakeEffect',
    'efManagerDamageSpawnMDustRandomMakeEffect',
    'efManagerDamageSpawnOrbsRandomMakeEffect',
    'efManagerDamageSpawnSparksRandomMakeEffect',
    'efManagerFoxReflectorMakeEffect',
    'efManagerImpactWaveMakeEffect',
    'efManagerRebirthHaloMakeEffect',
    'efManagerShieldMakeEffect')
$actualDisplayListSeams = @($report.reach.p1_seams_without_bank_scripts)
if (($actualDisplayListSeams -join ',') -ne ($expectedDisplayListSeams -join ',')) {
    throw "Particle-bank seam split changed: $($actualDisplayListSeams -join ',')"
}

if (([int64]$report.bytes.script_bank_bytes -ne 10912) -or
    ([int64]$report.bytes.source_texture_bytes -ne 136248) -or
    ([int64]$report.bytes.ds_texture_bytes -ne 82752) -or
    ([int64]$report.bytes.ds_texture_data_bytes -ne 82176) -or
    ([int64]$report.bytes.ds_palette_bytes -ne 672) -or
    ([int64]$report.bytes.payload_bytes -ne 93760) -or
    ([int64]$report.bytes.index_table_bytes -ne 1283) -or
    ([int64]$report.bytes.pack_bytes -ne 95043) -or
    ([int64]$report.bytes.asset_bytes -ne 82848) -or
    ([int64]$report.bytes.linked_bytes -ne 12195) -or
    ([int64]$report.bytes.arena_headroom_bytes -ne 210320) -or
    ([int64]$report.bytes.spare_bytes -ne 198125)) {
    throw ('Particle bank footprint changed: pack ' +
        "$([int64]$report.bytes.pack_bytes), linked " +
        "$([int64]$report.bytes.linked_bytes), asset " +
        "$([int64]$report.bytes.asset_bytes), spare " +
        "$([int64]$report.bytes.spare_bytes).")
}
# Only the LINKED half competes with the boot-time taskman arena search. The
# whole pack did, until 2026-07-31: at 95,043 bytes the fine search bottomed out
# at its 0x130000 floor and the first battle allocation (4,896 asked, 4,032
# free) hung the ROM in syTaskmanMalloc. Charging the NitroFS payload here again
# would re-arm exactly that failure.
if ([int64]$report.bytes.linked_bytes -ge [int64]$report.bytes.arena_headroom_bytes) {
    throw 'Particle bank pack no longer fits measured arena headroom.'
}
if (([int64]$report.bytes.linked_bytes + [int64]$report.bytes.asset_bytes) -ne
    [int64]$report.bytes.pack_bytes) {
    throw 'Particle bank linked + asset bytes do not account for the pack.'
}
if ($report.checksums.source_sha256_lo -ne '0xa2a1e85f') {
    throw "efcommon source identity changed: $($report.checksums.source_sha256_lo)"
}
if ($report.checksums.table_sha256_lo -ne '0x8db9d3bd') {
    throw "Packed particle table changed: $($report.checksums.table_sha256_lo)"
}

# Fidelity gate. Particles are soft blobs, so a source with graded alpha may
# only land in a DS format that carries graded alpha; PAL4/PAL16/PAL256 own one
# transparency bit and would turn a dust puff into a stencil. The 1-bit-alpha
# CI4 sources must stay bit-exact.
$gradedFormats = @('A3I5', 'A5I3')
$paletteFormats = @('PAL4', 'PAL16', 'PAL256')
$packed = @($report.textures | Where-Object { $_.packed })
if ($packed.Count -ne 23) {
    throw "Expected 23 packed textures, found $($packed.Count)."
}
foreach ($texture in $packed) {
    if ($texture.source_graded_alpha) {
        if ($gradedFormats -notcontains $texture.ds_format) {
            throw ("Texture $($texture.texture) has graded source alpha but " +
                "packed as $($texture.ds_format).")
        }
        if ([double]$texture.max_error -gt 40.0) {
            throw ("Texture $($texture.texture) conversion error " +
                "$($texture.max_error) exceeds its budget.")
        }
    }
    else {
        if ($paletteFormats -notcontains $texture.ds_format) {
            throw ("Texture $($texture.texture) has 1-bit source alpha but " +
                "packed as $($texture.ds_format).")
        }
        if ([double]$texture.max_error -ne 0.0) {
            throw ("Texture $($texture.texture) is 1-bit alpha and must be " +
                "exact, error $($texture.max_error).")
        }
    }
    if (([int]$texture.ds_palette_entries -lt 1) -or
        ([int]$texture.ds_palette_entries -gt 255)) {
        throw ("Texture $($texture.texture) needs " +
            "$($texture.ds_palette_entries) palette entries, which does not " +
            'fit NDSParticleTexture.palette_entries.')
    }
}
$exact = @($packed | Where-Object { [double]$_.max_error -eq 0.0 }).Count
if ($exact -ne 6) {
    throw "Expected 6 bit-exact packed textures, found $exact."
}

$header = Get-Content -LiteralPath $headerPath -Raw
foreach ($token in @(
    '#define NDS_PARTICLE_SCRIPT_COUNT 119u',
    '#define NDS_PARTICLE_SCRIPT_REACHABLE_COUNT 55u',
    '#define NDS_PARTICLE_SCRIPT_UNREACHABLE 0xffffffffu',
    '#define NDS_PARTICLE_SCRIPT_BANK_BYTES 10912u',
    '#define NDS_PARTICLE_TEXTURE_COUNT 47u',
    '#define NDS_PARTICLE_TEXTURE_PACKED_COUNT 23u',
    '#define NDS_PARTICLE_TEXTURE_DATA_BYTES 82176u',
    '#define NDS_PARTICLE_PALETTE_ENTRIES 336u',
    '#define NDS_PARTICLE_TEXTURE_ASSET_PATH "nitro:/particles/efcommon_particle_textures.ds.bin"',
    '#define NDS_PARTICLE_TEXTURE_ASSET_BYTES 82848u',
    '#define NDS_PARTICLE_PALETTE_ASSET_OFFSET 82176u',
    '#define NDS_PARTICLE_LINKED_BYTES 12195u',
    '#define NDS_PARTICLE_BANKS_SOURCE_CHECKSUM 0xa2a1e85fu',
    '#define NDS_PARTICLE_BANKS_TABLE_CHECKSUM 0x8db9d3bdu',
    'extern const u8 gNdsParticleScriptBank[NDS_PARTICLE_SCRIPT_BANK_BYTES];',
    'extern const u32 gNdsParticleScriptBankBytes;',
    'extern const u32 gNdsParticleScriptOffsets[NDS_PARTICLE_SCRIPT_COUNT];',
    'extern const NDSParticleTexture gNdsParticleTextures[NDS_PARTICLE_TEXTURE_COUNT];',
    'extern const u32 gNdsParticleTextureCount;',
    'extern const u8 gNdsParticleTextureFrames[NDS_PARTICLE_TEXTURE_COUNT];',
    'BIG-ENDIAN N64 data')) {
    if (-not $header.Contains($token)) {
        throw "Generated particle bank header lost: $token"
    }
}
# The texels must not come back into the image under any name. A declaration is
# how that regression would start.
foreach ($banned in @('gNdsParticleTextureData', 'gNdsParticlePaletteData')) {
    if ($header.Contains("$banned[")) {
        throw ("Generated particle bank header re-declares $banned as an " +
            'array; the texel/palette blocks ship as NitroFS payload.')
    }
}

# The .inc lives under the gitignored src/nds/generated, so it only exists once
# the Makefile rule has run. When it does exist, the sentinel count is the
# fail-closed contract and must match the enumeration exactly.
$incState = 'not built'
if (Test-Path -LiteralPath $incPath) {
    $inc = Get-Content -LiteralPath $incPath -Raw
    $offsetsBlock = [regex]::Match(
        $inc, 'gNdsParticleScriptOffsets\[[^\]]*\]\s*=\s*\{(?<body>[^}]*)\}')
    if (-not $offsetsBlock.Success) {
        throw 'Generated particle bank .inc has no script offset table.'
    }
    $entries = @([regex]::Matches($offsetsBlock.Groups['body'].Value,
        '0x[0-9a-f]{8}u') | ForEach-Object { $_.Value })
    if ($entries.Count -ne 119) {
        throw "Script offset table holds $($entries.Count) entries, expected 119."
    }
    $sentinels = @($entries | Where-Object { $_ -eq '0xffffffffu' }).Count
    if ($sentinels -ne (119 - 55)) {
        throw ("Script offset table holds $sentinels unreachable sentinels, " +
            "expected $(119 - 55).")
    }
    $textureBlock = [regex]::Match(
        $inc, 'gNdsParticleTextures\[[^\]]*\]\s*=\s*\{(?<body>.*?)\n\};',
        [System.Text.RegularExpressions.RegexOptions]::Singleline)
    if (-not $textureBlock.Success) {
        throw 'Generated particle bank .inc has no texture table.'
    }
    $unpacked = @([regex]::Matches($textureBlock.Groups['body'].Value,
        '\{\s*0,\s*0,\s*0,\s*0,\s*0xffffffffu,\s*0xffffffffu\s*\}')).Count
    if ($unpacked -ne (47 - 23)) {
        throw ("Texture table holds $unpacked sentinel rows, expected " +
            "$(47 - 23).")
    }
    foreach ($banned in @('gNdsParticleTextureData', 'gNdsParticlePaletteData')) {
        if ($inc.Contains("$banned[")) {
            throw ("Generated particle bank .inc defines $banned; the texel " +
                'and palette blocks ship as NitroFS payload, and linking them ' +
                'takes their 82848 bytes straight out of the taskman arena.')
        }
    }
    $incState = 'built'
}

# assets/ is gitignored, so the payload only exists once the generator has run.
# When it does, its size is the contract the offset tables were written against.
$assetState = 'not built'
if (Test-Path -LiteralPath $assetPath) {
    $assetBytes = (Get-Item -LiteralPath $assetPath).Length
    if ($assetBytes -ne 82848) {
        throw "Particle texture payload is $assetBytes bytes, expected 82848."
    }
    $assetState = 'built'
}

Write-Output (('Particle bank pack passed: 55/119 reachable efcommon scripts, ' +
    '23/47 textures, 136248 B N64 texture -> 82752 B DS, 12195 B linked ' +
    '(10912 script bank + 1283 index) of 210320 B arena headroom (198125 B ' +
    'spare) plus 82848 B NitroFS payload, 6 bit-exact CI4 textures, linear ' +
    "texel order pinned, .inc $incState, payload $assetState."))
