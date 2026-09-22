[CmdletBinding()]
param()

# Yoshi's egg is ONE source display list behind TWO owner-reported
# invisibilities, and the reason it went unnoticed is worth stating here rather
# than only in the artifact.
#
# dEFManagerYoshiShieldEffectDesc (efmanager.c:490) and
# dEFManagerYoshiEggEscapeEffectDesc (:1375) name the SAME
# &llYoshiModelShieldDObjDesc, differing only in flags and a matrix row. Both
# states hide Yoshi's entire drawable tree ON PURPOSE -- ftcommonguard1.c:391,
# ftcommonguard2.c:23 and efmanager.c:5441 each call ftParamHideModelPartAll
# behind `fp->fkind == nFTKindYoshi` -- so an egg is meant to draw in his place.
# With no native owner for that egg, nothing drew at all and Yoshi simply
# disappeared: the owner's "character intro is invisible (egg hatching)", and
# his shield too.
#
# The ordinary shield owner (FTManagerCommon, asset 163) cannot cover him,
# because those call sites are an if/else on fkind that routes Yoshi to
# efManagerYoshiShieldMakeEffect instead.
#
# 0xa860 is NOT a DObjDesc despite the decomp field being named one -- decoding
# it as such yields depths in the billions. Its MObjSub/AnimJoint/MatAnimJoint
# are all 0x0, and for that shape the field holds the immutable Gfx directly,
# exactly as the Fox reflector does. The decomp labels it
# `// DObj Setup attributes offset (?)` for this reason.

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path

function Assert-YoshiEggNative {
    param([bool]$Condition, [string]$Message)
    if (-not $Condition) { throw $Message }
}

$generator = Join-Path $root 'scripts\3d_vfx\generate_nds_entry_effects.py'
$generated = Join-Path $root 'src\nds\nds_entry_effects.generated.inc'
$modelO2R = Join-Path $root `
    'decomp\BattleShip-main\BattleShip_o2r\reloc_fighters_main\YoshiModel'
$lookup = Join-Path $root 'src\nds\nds_renderer_native_common.c'
$admission = Join-Path $root 'src\port\renderer_adapter_stage.c'

$generatorOutput = (& python $generator --check 2>&1 | Out-String)
Assert-YoshiEggNative ($LASTEXITCODE -eq 0) `
    'Yoshi egg EFDesc native packet generator failed.'
Write-Output $generatorOutput.TrimEnd()

$modelHash = (Get-FileHash -LiteralPath $modelO2R -Algorithm SHA256).Hash.ToLowerInvariant()
Assert-YoshiEggNative ($modelHash -eq
    'e2654cbdc969a473de1e78fa392211a4f465657a7c16ffc93e6bb2b073d4b04c') `
    "YoshiModel O2R hash drifted: $modelHash"

$generatedText = Get-Content -LiteralPath $generated -Raw

# THESE ASSERTIONS FOLLOW THE PRODUCER. They used to pin four absolute
# ordinals -- egg 62, Falcon Kick 60, Falcon Punch 61, root count 63, and the
# group index 106 inside the egg's own root row. Every one of those numbers
# moved when Samus Grapple and the Falcon Punch TEXID frames were added, so
# the checker went red on work that was entirely correct. A falsifier that
# holds a copy of the producer's old number fails the next honest change and
# teaches nothing; it has to assert the INVARIANT instead.
#
# The invariant is that the egg is appended at the TAIL, which is what keeps
# every previously accepted ordinal stable. So: its first ordinal is the root
# count minus its own count, and every other named family starts before it.
Assert-YoshiEggNative ($generatedText -match
    '#define NDS_ENTRY_EFFECT_YOSHI_EGG_ROOT_COUNT (\d+)u') `
    'Generated entry-effect packet lost NDS_ENTRY_EFFECT_YOSHI_EGG_ROOT_COUNT.'
$eggCount = [int]$Matches[1]
Assert-YoshiEggNative ($eggCount -eq 1) `
    "Yoshi egg root count is $eggCount, expected exactly 1 (one source list)."

Assert-YoshiEggNative ($generatedText -match
    '#define NDS_ENTRY_EFFECT_YOSHI_EGG_ROOT_FIRST (\d+)u') `
    'Generated entry-effect packet lost NDS_ENTRY_EFFECT_YOSHI_EGG_ROOT_FIRST.'
$eggFirst = [int]$Matches[1]

Assert-YoshiEggNative ($generatedText -match
    '#define NDS_ENTRY_EFFECT_ROOT_COUNT (\d+)u') `
    'Generated entry-effect packet lost NDS_ENTRY_EFFECT_ROOT_COUNT.'
$rootCount = [int]$Matches[1]

Assert-YoshiEggNative (($eggFirst + $eggCount) -eq $rootCount) `
    ("Yoshi egg is no longer the LAST entry-effect root: first=$eggFirst " +
     "count=$eggCount rootCount=$rootCount. A root tuple inserted anywhere " +
     "but the end silently renumbers every accepted ordinal after it.")

foreach ($family in @('FALCON_KICK', 'FALCON_PUNCH', 'SHIELD', 'MBALLRAYS')) {
    Assert-YoshiEggNative ($generatedText -match
        ("#define NDS_ENTRY_EFFECT_{0}_ROOT_FIRST (\d+)u" -f $family)) `
        "Generated entry-effect packet lost NDS_ENTRY_EFFECT_${family}_ROOT_FIRST."
    $familyFirst = [int]$Matches[1]
    Assert-YoshiEggNative ($familyFirst -lt $eggFirst) `
        ("NDS_ENTRY_EFFECT_${family}_ROOT_FIRST is $familyFirst, at or after " +
         "the Yoshi egg at $eggFirst -- the egg is no longer appended at the tail.")
}

# The egg's own root row: source offset, one root, and a group index that
# actually exists. The group index is derived, never pinned.
Assert-YoshiEggNative ($generatedText -match
    '\{ 0xa860u, (\d+)u, 1u, 0u \}') `
    'Generated entry-effect root table lost the { 0xa860u, ..., 1u, 0u } egg row.'
$eggGroup = [int]$Matches[1]
Assert-YoshiEggNative ($generatedText -match
    '#define NDS_ENTRY_EFFECT_GROUP_COUNT (\d+)u') `
    'Generated entry-effect packet lost NDS_ENTRY_EFFECT_GROUP_COUNT.'
$groupCount = [int]$Matches[1]
Assert-YoshiEggNative (($eggGroup -ge 0) -and ($eggGroup -lt $groupCount)) `
    "Yoshi egg group index $eggGroup is outside the generated group table ($groupCount)."

# One group, two triangles: the source list holds a single VTX and a single
# F3DEX2 TRI2. A drift here means the egg stopped compiling to its own quad.
# Read the producer's OWN summary line rather than a pinned ordinal or a
# hand-parsed table row -- the generator is the authority on what it emitted.
Assert-YoshiEggNative ($generatorOutput -match 'yoshi_egg_groups=(\d+)') `
    'Generator no longer reports yoshi_egg_groups.'
$eggGroups = [int]$Matches[1]
Assert-YoshiEggNative ($generatorOutput -match 'yoshi_egg_triangles=(\d+)') `
    'Generator no longer reports yoshi_egg_triangles.'
$eggTriangles = [int]$Matches[1]
Assert-YoshiEggNative (($eggGroups -eq 1) -and ($eggTriangles -eq 2)) `
    ("Yoshi egg compiled to $eggGroups group(s) and $eggTriangles triangle(s); " +
     'the source list is one VTX and one F3DEX2 TRI2, so it must be 1 and 2.')

$lookupText = Get-Content -LiteralPath $lookup -Raw
Assert-YoshiEggNative ($lookupText -match
    '\(owner_asset_id == 338u\)\s*&&\s*\(root_offset == 0xa860u\)') `
    'ndsRendererEntryEffectRoot lost its asset-338 Yoshi egg branch.'
Assert-YoshiEggNative ($lookupText -like
    '*NDS_ENTRY_EFFECT_YOSHI_EGG_ROOT_FIRST*') `
    'ndsRendererEntryEffectRoot no longer indexes the Yoshi egg root.'

$admissionText = Get-Content -LiteralPath $admission -Raw
Assert-YoshiEggNative ($admissionText -match
    'gFTDataYoshiModel \+ 0xa860u') `
    'renderer_adapter_stage lost the Yoshi egg admission arm.'
Assert-YoshiEggNative ($admissionText -match
    'owner_asset_id = 338u;') `
    'Yoshi egg admission arm no longer claims asset 338.'
# gFTDataYoshiModel is defined in ftyoshi.c, which is not built at every
# roster, so the arm must stay guarded or a Yoshi-less config fails to link.
Assert-YoshiEggNative ($admissionText -match
    '#if NDS_P2_YOSHI[\s\S]{0,1400}gFTDataYoshiModel \+ 0xa860u') `
    'Yoshi egg admission arm is no longer inside its NDS_P2_YOSHI guard.'

Write-Output ("P2_YOSHI_EGG_EFDESC_NATIVE_OK root=0xa860 asset=338 " +
    "ordinal=$eggFirst/$rootCount group=$eggGroup triangles=$eggTriangles " +
    "shield_and_egg_escape_share_one_root")
