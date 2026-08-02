param([string]$Python = 'python')

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$generator = Join-Path $PSScriptRoot 'generate_nds_particle_banks.py'
$reportPath = Join-Path $root 'docs/optimization/NDS_PARTICLE_BANKS.generated.json'
$headerPath = Join-Path $root 'include/nds/generated/nds_particle_banks.generated.h'
$effectHeaderPath = Join-Path $root 'include/ef/effect.h'
$runtimePath = Join-Path $root 'src/import/battleship_lbparticle.c'
$rendererPath = Join-Path $root 'src/nds/nds_renderer.c'
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

# BattleShip stores the particle-list slot in bank_id >> 3. Its public macro
# maps source genlink 0..3 to slots 1..4; a prior <<16 port typo indexed slot
# 8192 for the KO burst and data-aborted on its first spawn.
$effectHeader = Get-Content -LiteralPath $effectHeaderPath -Raw
if (-not $effectHeader.Contains(
        '#define LBPARTICLE_MASK_GENLINK(link) (((link) + 1) * 8)')) {
    throw 'Particle generator-link encoding differs from BattleShip.'
}

# The DS draw must consume the source transform and tint, not only the local
# script position. Ignoring this is visually valid-looking but puts Whispy,
# hit, and KO effects at the stage origin.
$runtime = Get-Content -LiteralPath $runtimePath -Raw
foreach ($token in @(
    'dLBParticleCurrentTransformID++;',
    'syMatrixTraRotRpyRScaF(',
    'world_pos->x = (xf->affine[0][0] * pc->pos.x)',
    'ndsRendererSubmitParticleQuad(atlas_name, &world_pos, pc->size,')) {
    if (-not $runtime.Contains($token)) {
        throw "Particle draw lost its source transform contract: $token"
    }
}
if ($runtime.Contains(
        'ndsRendererSubmitParticleQuad(atlas_name, &pc->pos, pc->size,')) {
    throw 'Particle draw regressed to a script-local position.'
}
$renderer = Get-Content -LiteralPath $rendererPath -Raw
if (-not $renderer.Contains('glColor((rgb)color);')) {
    throw 'Particle draw lost source prim-color modulation.'
}

& $Python $generator --repo-root $root --check
if ($LASTEXITCODE -ne 0) {
    throw 'Generated particle bank pack differs from its BattleShip sources.'
}

$report = Get-Content -LiteralPath $reportPath -Raw | ConvertFrom-Json

# Enumeration. These are derived, not declared: the P1 effect seams are the
# explicit P1_PARTICLE_SEAMS list plus the census substitutes, and the rest is
# the bank's own spawn graph. A moved count is a finding to explain, never a
# number to edit into place. 2026-07-31: 22 -> 41 seams, 55 -> 87 scripts,
# because seeding from census.SUBSTITUTES alone described the effects Task 39
# REPLACES, not the ones that still need a script -- the runtime reject ring
# caught two P1 effects (Fox blaster glow, Results confetti) marked UNREACHABLE.
if (([int]$report.source.script_count -ne 119) -or
    ([int]$report.source.texture_count -ne 47) -or
    (@($report.reach.reachable_scripts).Count -ne 87) -or
    (@($report.reach.packed_textures).Count -ne 31) -or
    (@($report.reach.p1_seams).Count -ne 41)) {
    throw ('Particle bank enumeration changed: ' +
        "$(@($report.reach.reachable_scripts).Count)/$([int]$report.source.script_count) scripts, " +
        "$(@($report.reach.packed_textures).Count)/$([int]$report.source.texture_count) textures, " +
        "$(@($report.reach.p1_seams).Count) seams.")
}

# These seventeen P1 seams route to the display-list effect path
# (efManagerMakeEffect*), not to the particle bank, so the bank cannot change
# what they draw. Pinned so a future edit cannot quietly claim otherwise.
$expectedDisplayListSeams = @(
    'efManagerCatchSwirlMakeEffect',
    'efManagerDamageSlashMakeEffect',
    'efManagerDamageSpawnMDustMakeEffect',
    'efManagerDamageSpawnMDustRandomMakeEffect',
    'efManagerDamageSpawnOrbsMakeEffect',
    'efManagerDamageSpawnOrbsRandomMakeEffect',
    'efManagerDamageSpawnSparksMakeEffect',
    'efManagerDamageSpawnSparksRandomMakeEffect',
    'efManagerFireSparkMakeEffect',
    'efManagerFoxEntryArwingMakeEffect',
    'efManagerFoxReflectorMakeEffect',
    'efManagerImpactAirWaveMakeEffect',
    'efManagerImpactWaveMakeEffect',
    'efManagerMarioEntryDokanMakeEffect',
    'efManagerRebirthHaloMakeEffect',
    'efManagerReflectBreakMakeEffect',
    'efManagerShieldMakeEffect')
$actualDisplayListSeams = @($report.reach.p1_seams_without_bank_scripts)
if (($actualDisplayListSeams -join ',') -ne ($expectedDisplayListSeams -join ',')) {
    throw "Particle-bank seam split changed: $($actualDisplayListSeams -join ',')"
}

# linked_bytes did NOT move when the reachable set went 55 -> 87. It cannot:
# the bytecode bank ships whole and byte-identical (it is position-independent
# source data), and reachability is expressed only in the offset table, whose
# size is fixed at one entry per source script. Growing the P1 seam list
# therefore costs ROM and NitroFS payload, never arena -- which is why the
# corrected seam list could be generous rather than minimal.
if (([int64]$report.bytes.script_bank_bytes -ne 10912) -or
    ([int64]$report.bytes.source_texture_bytes -ne 220248) -or
    ([int64]$report.bytes.ds_texture_bytes -ne 137040) -or
    ([int64]$report.bytes.ds_texture_data_bytes -ne 136256) -or
    ([int64]$report.bytes.ds_palette_bytes -ne 896) -or
    ([int64]$report.bytes.payload_bytes -ne 148064) -or
    ([int64]$report.bytes.index_table_bytes -ne 1283) -or
    ([int64]$report.bytes.pack_bytes -ne 149347) -or
    ([int64]$report.bytes.asset_bytes -ne 137152) -or
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
# The DRAW path's payload, which is a different question from the pack above:
# these bytes go into texture VRAM, not into NitroFS-and-forget.
#
# HALVING IT WAS TRIED AND REFUTED, 2026-08-01. With the 32,768-byte sheet
# resident, ndsRendererHardwareResolveStageSourceFrameTexture fails about one
# frame in ten -- reject site 2, 196 times in a 566-frame match -- each failure
# rejecting the native stage owner and dropping that frame onto the generic
# renderer at five or more VBlanks. 128x64 freed 16,384 bytes of texture VRAM
# and kept both textures a live match was measured drawing, and the rejections
# did not move AT ALL: 196 and 197, to the digit, on both sheets. So the
# stage's resolve is not failing for want of VRAM, and the coverage reduction
# (9 of 31 textures instead of 16) bought nothing and was reverted.
#
# A texture that is not admitted draws NOTHING -- it never draws something
# else -- and raises gNdsParticleQuadMissCount, so the excluded list is
# reported by name and a coverage loss is a number rather than a silent gap.
#
# 2026-08-01, second change: Dream Land's bank joined the sheet. Its texture 2
# -- 16x16 with four frames, the one grPupupuWhispyLeavesMakeEffect and
# grPupupuWhispyDustMakeEffect both draw -- is admitted at
# NDS_PARTICLE_QUAD_PUPUPU_STRIDE + 2 = 66, because texture 2 names a different
# image in each bank.
#
# 2026-08-01, third change, and it is a REGRESSION FIX. Admitting 66 evicted
# common textures 0 and 9, on the reasoning that "a measured match has never
# drawn them (the use mask has only ever reported 22 and 27)". That reasoning
# was reading a SINGLE-CPU mask, where Mario stands still. The first both-CPU
# soak on that atlas reported the mask as 0x08400007 -- bits 0, 1, 2, 22, 27 --
# and 127,989 QuadMisses against 2,725 emitted quads: texture 0 carries almost
# every particle a moving match draws. QUAD_MEASURED_LIVE is regraded to
# (0, 1, 2, 22, 27), and texture 0 only fits because atlas cells are now capped
# at 16x16 (QUAD_CELL_MAX): at its source 32x32 it takes a shelf of its own and
# wastes half of it, which is what pushed it off the sheet. Texture 1 is in the
# mask but has no image in the pack at all (width 0), so it can never be
# admitted and fails closed.
#
# 2026-08-01, fourth change: with the bank drawing correctly for the first time,
# a soak reported 3,741 strided draws of which 2,084 missed at pre-stride ids 0
# and 1 -- Dream Land draws ALL THREE of its textures, not only the sheet its
# two named scripts reference. All three are admitted now (64/65/66); it cost
# common textures 3 and 9, neither of which any measured match has drawn.
# 2026-08-01, fifth change: the upward-star KO runs efcommon script 0x5C,
# which requests texture 24. Its focused run reported one miss and zero quads;
# the 16x16 source cell fits the last 128 bytes when reduced to 8x8.
if (([int64]$report.quads.atlas_width -ne 64) -or
    ([int64]$report.quads.atlas_height -ne 64) -or
    ([int64]$report.quads.atlas_bytes -ne 8192) -or
    ([int64]$report.quads.bytes -ne 8192) -or
    ([int64]$report.quads.frame_count -ne 27) -or
    (@($report.quads.admitted).Count -ne 14) -or
    (@($report.quads.excluded).Count -ne 20)) {
    throw ('Particle quad sheet changed: ' +
        "$([int64]$report.quads.bytes) B, " +
        "$(@($report.quads.admitted).Count) admitted, " +
        "$(@($report.quads.excluded).Count) excluded.")
}
if ([int64]$report.quads.bytes -gt [int64]$report.quads.atlas_bytes) {
    throw 'Particle quad atlas holds more texels than it has.'
}
# Every common texture a both-CPU match was measured drawing and the pack can
# supply (1 has no image), plus Dream Land's leaf/dust sheet. If admission order
# ever drops one the effects stop appearing and nothing else would say so --
# which is exactly what happened on 2026-08-01 with texture 0.
foreach ($id in @(0, 2, 10, 13, 18, 19, 20, 21, 22, 24, 27, 64, 65, 66)) {
    if (@($report.quads.admitted) -notcontains $id) {
        throw "Particle quad sheet dropped measured texture $id."
    }
}
$koCells = @($report.quads.admitted_cells | Where-Object {
        [int]$_.texture -in @(10, 13, 18, 19, 20, 21, 24) })
if ($koCells.Count -ne 7) {
    throw "KO particle atlas closure has $($koCells.Count) textures, expected 7."
}
foreach ($cell in $koCells) {
    if (([int]$cell.width -ne 8) -or ([int]$cell.height -ne 8)) {
        throw "KO particle texture $($cell.texture) is not the measured-safe 8x8 cell."
    }
}
if ($report.checksums.source_sha256_lo -ne '0xa2a1e85f') {
    throw "efcommon source identity changed: $($report.checksums.source_sha256_lo)"
}
if ($report.checksums.table_sha256_lo -ne '0x1973edec') {
    throw "Packed particle table changed: $($report.checksums.table_sha256_lo)"
}

# Fidelity gate. Particles are soft blobs, so a source with graded alpha may
# only land in a DS format that carries graded alpha; PAL4/PAL16/PAL256 own one
# transparency bit and would turn a dust puff into a stencil. The 1-bit-alpha
# CI4 sources must stay bit-exact.
$gradedFormats = @('A3I5', 'A5I3')
$paletteFormats = @('PAL4', 'PAL16', 'PAL256')
$packed = @($report.textures | Where-Object { $_.packed })
if ($packed.Count -ne 31) {
    throw "Expected 31 packed textures, found $($packed.Count)."
}
foreach ($texture in $packed) {
    if ($texture.source_graded_alpha) {
        if ($gradedFormats -notcontains $texture.ds_format) {
            throw ("Texture $($texture.texture) has graded source alpha but " +
                "packed as $($texture.ds_format).")
        }
        # 40 -> 48 on 2026-07-31, when the corrected P1 seam list brought
        # texture 29 (ten 32x32 RGBA32 frames, 1,872 distinct opaque colours)
        # into the reachable set at 46. That is the DS ceiling, not a bad pick:
        # A3I5's 32-entry palette is the widest graded-alpha format the hardware
        # has, and every alternative measures worse on the same image --
        # A5I3 78, PAL256 128.8, DIRECT16 128.0, PAL16 139.1, PAL4 160.3. Mean
        # error is 8.07 on a soft ten-frame explosion. Raise this only with the
        # same kind of per-format measurement; never to make a build pass.
        if ([double]$texture.max_error -gt 48.0) {
            throw ("Texture $($texture.texture) conversion error " +
                "$($texture.max_error) exceeds its budget.")
        }
    }
    else {
        if ($paletteFormats -notcontains $texture.ds_format) {
            throw ("Texture $($texture.texture) has 1-bit source alpha but " +
                "packed as $($texture.ds_format).")
        }
        # A CI4 source IS a palette, so a palette format must reproduce it
        # exactly. An IA4 source is 3-bit greyscale, and its intensities are
        # not BGR555 values: 36/109/182/218 land between 5-bit steps, so 4.0
        # is the hardware colour-depth floor and demanding 0 would only force
        # DIRECT16 at four times the bytes for no visible gain. Anything above
        # the floor means the palette dropped a colour -- which is what this
        # caught on 2026-07-31, when textures 9 and 22 sat in 3-colour PAL4 at
        # error 28 because "nothing is exact, take the cheapest" picked the
        # worst survivor.
        $budget = if ($texture.source_format -eq 'CI4') { 0.0 } else { 4.0 }
        if ([double]$texture.max_error -gt $budget) {
            throw ("Texture $($texture.texture) is 1-bit alpha " +
                "$($texture.source_format) with error $($texture.max_error), " +
                "over its $budget budget.")
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
if ($exact -ne 7) {
    throw "Expected 7 bit-exact packed textures, found $exact."
}

$header = Get-Content -LiteralPath $headerPath -Raw
foreach ($token in @(
    '#define NDS_PARTICLE_SCRIPT_COUNT 119u',
    '#define NDS_PARTICLE_SCRIPT_REACHABLE_COUNT 87u',
    '#define NDS_PARTICLE_SCRIPT_UNREACHABLE 0xffffffffu',
    '#define NDS_PARTICLE_SCRIPT_BANK_BYTES 10912u',
    '#define NDS_PARTICLE_TEXTURE_COUNT 47u',
    '#define NDS_PARTICLE_TEXTURE_PACKED_COUNT 31u',
    '#define NDS_PARTICLE_TEXTURE_DATA_BYTES 136256u',
    '#define NDS_PARTICLE_PALETTE_ENTRIES 448u',
    '#define NDS_PARTICLE_TEXTURE_ASSET_PATH "nitro:/particles/efcommon_particle_textures.ds.bin"',
    '#define NDS_PARTICLE_TEXTURE_ASSET_BYTES 137152u',
    '#define NDS_PARTICLE_PALETTE_ASSET_OFFSET 136256u',
    '#define NDS_PARTICLE_LINKED_BYTES 12195u',
    '#define NDS_PARTICLE_QUAD_ASSET_PATH "nitro:/particles/efcommon_particle_quads.rgb5a1.bin"',
    '#define NDS_PARTICLE_QUAD_ATLAS_WIDTH 64u',
    '#define NDS_PARTICLE_QUAD_ATLAS_HEIGHT 64u',
    '#define NDS_PARTICLE_QUAD_ASSET_BYTES 8192u',
    '#define NDS_PARTICLE_QUAD_TEXEL_BYTES 8192u',
    '#define NDS_PARTICLE_QUAD_COUNT 14u',
    '#define NDS_PARTICLE_QUAD_FRAME_COUNT 27u',
    '#define NDS_PARTICLE_BANKS_SOURCE_CHECKSUM 0xa2a1e85fu',
    '#define NDS_PARTICLE_BANKS_TABLE_CHECKSUM 0x1973edecu',
    # NOT const, deliberately: the loader byte-swaps the bank in place instead
    # of spending 10,912 bytes of taskman arena on a writable copy.
    'extern u8 gNdsParticleScriptBank[NDS_PARTICLE_SCRIPT_BANK_BYTES];',
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
    if ($sentinels -ne (119 - 87)) {
        throw ("Script offset table holds $sentinels unreachable sentinels, " +
            "expected $(119 - 87).")
    }
    $textureBlock = [regex]::Match(
        $inc, 'gNdsParticleTextures\[[^\]]*\]\s*=\s*\{(?<body>.*?)\n\};',
        [System.Text.RegularExpressions.RegexOptions]::Singleline)
    if (-not $textureBlock.Success) {
        throw 'Generated particle bank .inc has no texture table.'
    }
    $unpacked = @([regex]::Matches($textureBlock.Groups['body'].Value,
        '\{\s*0,\s*0,\s*0,\s*0,\s*0xffffffffu,\s*0xffffffffu\s*\}')).Count
    if ($unpacked -ne (47 - 31)) {
        throw ("Texture table holds $unpacked sentinel rows, expected " +
            "$(47 - 31).")
    }
    foreach ($banned in @('gNdsParticleTextureData', 'gNdsParticlePaletteData')) {
        if ($inc.Contains("$banned[")) {
            throw ("Generated particle bank .inc defines $banned; the texel " +
                'and palette blocks ship as NitroFS payload, and linking them ' +
                'takes their 137152 bytes straight out of the taskman arena.')
        }
    }
    $incState = 'built'
}

# assets/ is gitignored, so the payload only exists once the generator has run.
# When it does, its size is the contract the offset tables were written against.
$assetState = 'not built'
if (Test-Path -LiteralPath $assetPath) {
    $assetBytes = (Get-Item -LiteralPath $assetPath).Length
    $quadPath = Join-Path $root 'assets/particles/efcommon_particle_quads.rgb5a1.bin'
$quadState = 'not built'
if (Test-Path -LiteralPath $quadPath) {
    $quadBytes = (Get-Item -LiteralPath $quadPath).Length
    if ($quadBytes -ne 8192) {
        throw "Particle quad atlas is $quadBytes bytes, expected 8192."
    }
    $quadState = 'built'
}
if ($assetBytes -ne 137152) {
        throw "Particle texture payload is $assetBytes bytes, expected 137152."
    }
    $assetState = 'built'
}

Write-Output (('Particle bank pack passed: 87/119 reachable efcommon scripts, ' +
    '31/47 textures, 220248 B N64 texture -> 137040 B DS, 12195 B linked ' +
    '(10912 script bank + 1283 index) of 210320 B arena headroom (198125 B ' +
    'spare) plus 137152 B NitroFS payload, 7 bit-exact CI4 textures, linear ' +
    "texel order pinned, .inc $incState, payload $assetState, " +
    "quad atlas 64x64, 14/34 textures in 27 frames $quadState."))
