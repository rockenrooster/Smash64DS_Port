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

# THE ESCAPE EGG DREW AND NEVER TURNED (2026-09-22), and nothing above could
# see it: every assertion so far is about the egg's pixels, and the turn lives
# in its XObj list. A custom matrix kind (>= 66) runs sGCMatrixFuncList
# (objdisplay.c:1157-1170); one the port's XObj builder has no case for falls
# to `default:` and a TRS fallback without a word. 0x4A, func_ovl0_800CB2F0,
# which copies joint 5's pitch into the egg's roll for the kind-46 billboard,
# was exactly that. So the kinds are read from the SOURCE descs, not pinned:
# each custom one needs a builder case, and a place on the stage-world cache's
# ineligible list, because every custom kind these descs use reads a live
# fighter joint that the cache key does not cover.
$efmanager = Join-Path $root 'decomp\BattleShip-main\decomp\src\ef\efmanager.c'
$matrix = Join-Path $root 'src\port\renderer_adapter_matrix.c'
$efText = Get-Content -LiteralPath $efmanager -Raw
$matrixText = Get-Content -LiteralPath $matrix -Raw

$defines = @{}
foreach ($m in [regex]::Matches($matrixText,
        '#define (NDS_RENDERER_ADAPTER_\w+) (0x[0-9A-Fa-f]+|\d+)u')) {
    $defines[$m.Groups[1].Value] = [Convert]::ToInt32($m.Groups[2].Value,
        $(if ($m.Groups[2].Value.StartsWith('0x')) { 16 } else { 10 }))
}
$builder = [regex]::Match($matrixText,
    'static sb32 ndsRendererAdapterBuildDObjXObjMatrix\([\s\S]*?\n\}\n')
Assert-YoshiEggNative $builder.Success `
    'renderer_adapter_matrix.c lost ndsRendererAdapterBuildDObjXObjMatrix.'
$keyCapture = [regex]::Match($matrixText,
    'static sb32 ndsRendererAdapterCaptureStageWorldSourceKey\([\s\S]*?\n\}\n')
Assert-YoshiEggNative $keyCapture.Success `
    'renderer_adapter_matrix.c lost ndsRendererAdapterCaptureStageWorldSourceKey.'

$customKinds = @()
foreach ($desc in @('dEFManagerYoshiShieldEffectDesc',
                    'dEFManagerYoshiEggEscapeEffectDesc')) {
    $block = [regex]::Match($efText, "EFDesc $desc =\s*\{[\s\S]*?\n\};")
    Assert-YoshiEggNative $block.Success "efmanager.c lost $desc."
    $row = [regex]::Match($block.Value,
        'transformation struct 1\s*\{\s*(\w+),[^\n]*\n\s*(\w+),[^\n]*\n\s*(\w+)')
    Assert-YoshiEggNative $row.Success "$desc has no readable transform struct 1."
    foreach ($token in @($row.Groups[1].Value, $row.Groups[2].Value,
                         $row.Groups[3].Value)) {
        Assert-YoshiEggNative ($token -match '^(0x[0-9A-Fa-f]+|\d+)$') `
            "$desc transform kind '$token' is not a literal; teach this check."
        $kind = [Convert]::ToInt32($token,
            $(if ($token.StartsWith('0x')) { 16 } else { 10 }))
        if ($kind -lt 66) { continue }
        $names = @($defines.Keys | Where-Object { $defines[$_] -eq $kind })
        $cased = @($names | Where-Object {
            $builder.Value -match ('case ' + $_ + ':') })
        Assert-YoshiEggNative ($cased.Count -gt 0) `
            (("$desc uses custom matrix kind 0x{0:x} and the port's XObj " +
              "builder has no case for it: it falls to the TRS fallback.") -f $kind)
        $keyed = @($names | Where-Object {
            $keyCapture.Value -match ('\(xobj->kind == ' + $_ + '\)') })
        Assert-YoshiEggNative ($keyed.Count -gt 0) `
            (("Custom matrix kind 0x{0:x} ($desc) reads a live fighter joint " +
              'but is missing from the stage-world cache ineligible list.') -f $kind)
        $customKinds += ('0x{0:x}' -f $kind)
    }
}
$customKinds = @($customKinds | Select-Object -Unique)

# 0x4A is a WRITE, not a matrix: source returns 1, so gcPrepDObjMatrix emits no
# gSPMatrix, and its whole effect is lbcommon.c:2018-2019. The case must
# contribute no local transform, and the copy must keep the source's sign rule.
$pitchNames = @($defines.Keys | Where-Object { $defines[$_] -eq 0x4A })
Assert-YoshiEggNative ($pitchNames.Count -eq 1) `
    'Expected exactly one NDS_RENDERER_ADAPTER_* define for custom kind 0x4A.'
$pitchCase = [regex]::Match($builder.Value,
    ('case ' + $pitchNames[0] + ':([\s\S]*?)\n\s*(?:case |default:)'))
Assert-YoshiEggNative ($pitchCase.Success -and
    ($pitchCase.Groups[1].Value -match 'return FALSE;') -and
    ($pitchCase.Groups[1].Value -notmatch '\bbreak;')) `
    'The 0x4A builder case must return FALSE: source emits no matrix for it.'
$flatMatrix = $matrixText -replace '\s+', ''
Assert-YoshiEggNative ($flatMatrix.Contains(
    'dobj->rotate.vec.f.z=(parts->mtx_translate[0][2]>0.0F)?' +
    'attach->rotate.vec.f.x:-attach->rotate.vec.f.x;')) `
    'The 0x4A pitch-to-roll copy no longer matches lbcommon.c:2018-2019.'

Write-Output ("P2_YOSHI_EGG_EFDESC_NATIVE_OK root=0xa860 asset=338 " +
    "ordinal=$eggFirst/$rootCount group=$eggGroup triangles=$eggTriangles " +
    "shield_and_egg_escape_share_one_root " +
    "custom_kinds_handled=$($customKinds -join ',')")
