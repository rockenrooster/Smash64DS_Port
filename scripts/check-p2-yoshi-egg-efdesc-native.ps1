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

& python $generator --check
Assert-YoshiEggNative ($LASTEXITCODE -eq 0) `
    'Yoshi egg EFDesc native packet generator failed.'

$modelHash = (Get-FileHash -LiteralPath $modelO2R -Algorithm SHA256).Hash.ToLowerInvariant()
Assert-YoshiEggNative ($modelHash -eq
    'e2654cbdc969a473de1e78fa392211a4f465657a7c16ffc93e6bb2b073d4b04c') `
    "YoshiModel O2R hash drifted: $modelHash"

$generatedText = Get-Content -LiteralPath $generated -Raw
foreach ($token in @(
    '#define NDS_ENTRY_EFFECT_YOSHI_EGG_ROOT_FIRST 62u',
    '#define NDS_ENTRY_EFFECT_YOSHI_EGG_ROOT_COUNT 1u',
    # The egg is appended at the tail, so the two Falcon ordinals that preceded
    # it must not have moved. A root tuple inserted anywhere but the end
    # silently renumbers every accepted ordinal after it.
    '#define NDS_ENTRY_EFFECT_FALCON_KICK_ROOT_FIRST 60u',
    '#define NDS_ENTRY_EFFECT_FALCON_PUNCH_ROOT_FIRST 61u',
    '{ 0xa860u, 106u, 1u, 0u }'
)) {
    Assert-YoshiEggNative ($generatedText -like "*$token*") `
        "Generated entry-effect packet lost token: $token"
}

# One group, two triangles: the source list holds a single VTX and a single
# F3DEX2 TRI2. A drift here means the egg stopped compiling to its own quad.
Assert-YoshiEggNative ($generatedText -match
    '#define NDS_ENTRY_EFFECT_ROOT_COUNT 63u') `
    'Entry-effect root count is no longer 63; the egg root may have been dropped.'

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
    "ordinal=62 shield_and_egg_escape_share_one_root")
