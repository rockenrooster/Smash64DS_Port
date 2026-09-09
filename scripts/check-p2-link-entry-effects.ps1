[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path

function Assert-LinkEntryCheck {
    param([bool]$Condition, [string]$Message)
    if (-not $Condition) { throw $Message }
}

$generator = Join-Path $root 'scripts\3d_vfx\generate_nds_entry_effects.py'
$generated = Join-Path $root 'src\nds\nds_entry_effects.generated.inc'
$o2r = Join-Path $root `
    'decomp\BattleShip-main\BattleShip_o2r\reloc_fighters_main\LinkSpecial2'

& python $generator --check
Assert-LinkEntryCheck ($LASTEXITCODE -eq 0) `
    'Link entry native packet generator failed.'

$o2rHash = (Get-FileHash -LiteralPath $o2r -Algorithm SHA256).Hash.ToLowerInvariant()
Assert-LinkEntryCheck ($o2rHash -eq
    '3decd2670e012cffb135b47b4caabf66db1b90fd637f45408a3e8641f1ea31f1') `
    "LinkSpecial2 O2R hash drifted: $o2rHash"

$generatedText = Get-Content -LiteralPath $generated -Raw
$generatedHash = (Get-FileHash -LiteralPath $generated -Algorithm SHA256).Hash.ToLowerInvariant()
Assert-LinkEntryCheck ($generatedHash -eq
    '95c2c549314a741f6b0d3d14a526847e4b6b1e99c7970b47eb7fe46d6fe8c792') `
    "Generated Link entry packet corpus drifted: $generatedHash"
foreach ($token in @(
    '#define NDS_ENTRY_EFFECT_ROOT_COUNT 47u',
    '#define NDS_ENTRY_EFFECT_GROUP_COUNT 91u',
    '#define NDS_ENTRY_EFFECT_VERTEX_COUNT 1494u',
    '#define NDS_ENTRY_EFFECT_POSITION_COUNT 347u',
    '#define NDS_ENTRY_EFFECT_COLOR_COUNT 283u',
    '#define NDS_ENTRY_EFFECT_TEXTURE_COUNT 54u',
    '#define NDS_ENTRY_EFFECT_LINK_ROOT_FIRST 23u',
    '#define NDS_ENTRY_EFFECT_LINK_SPIN_WEAPON_ROOT_FIRST 26u',
    '#define NDS_ENTRY_EFFECT_LINK_BOOMERANG_ROOT_FIRST 27u',
    'static const u16 sNdsEntryEffectCornerPosition[NDS_ENTRY_EFFECT_VERTEX_COUNT]',
    'static const u16 sNdsEntryEffectCornerColor[NDS_ENTRY_EFFECT_VERTEX_COUNT]',
    '{ 1188u, 16u, 18u, 255u, 23u,',
    '{ 1236u, 16u, 18u, 40u, 24u,',
    '{ 0x02d8u, 57u, 1u, 0u }',
    '{ 0x0698u, 58u, 1u, 0u }'
)) {
    Assert-LinkEntryCheck $generatedText.Contains($token) `
        "Generated Link entry packet corpus is missing: $token"
}

$generatorText = Get-Content -LiteralPath $generator -Raw
foreach ($token in @(
    'reloc_fighters_main/LinkSpecial2',
    '3decd2670e012cffb135b47b4caabf66db1b90fd637f45408a3e8641f1ea31f1',
    'LINK_SPECIAL2_ROOTS = (0x02D8, 0x0698, 0x1100)',
    'LINK_MODEL_SPIN_ROOTS = (0x11680,)',
    '("Position", corner_position, "u16")',
    '("Color", corner_color, "u16")'
)) {
    Assert-LinkEntryCheck $generatorText.Contains($token) `
        "Link entry generator contract is missing: $token"
}

$relocSource = Get-Content -LiteralPath (Join-Path $root `
    'decomp\BattleShip-main\decomp\src\relocData\353_LinkSpecial2.c') -Raw
foreach ($token in @(
    'dLinkSpecial2_EntryWaveDObjDesc',
    'dLinkSpecial2_Joint_0x02D8_DisplayList',
    'dLinkSpecial2_EntryBeamDObjDesc',
    'dLinkSpecial2_Joint_0x0698_DisplayList',
    'aobjEvent32SetValBlock(AOBJ_FLAG_ROTY, 120)'
)) {
    Assert-LinkEntryCheck $relocSource.Contains($token) `
        "BattleShip LinkSpecial2 source contract is missing: $token"
}
Assert-LinkEntryCheck (([regex]::Matches($relocSource,
    'aobjEvent32SetExtValBlock\(AOBJ_EXTFLAG_PRIMCOLOR, 40\)')).Count -eq 6) `
    'BattleShip Link entry material lifetime changed from six 40-tick blocks.'

$effectSource = Get-Content -LiteralPath (Join-Path $root `
    'decomp\BattleShip-main\decomp\src\ef\efmanager.c') -Raw
foreach ($descriptor in @('Wave', 'Beam')) {
    $pattern = "(?s)dEFManagerLinkEntry${descriptor}EffectDesc\s*=.*?" +
        'gcPlayAnimAll,.*?gcDrawDObjTreeDLLinksForGObj'
    Assert-LinkEntryCheck ($effectSource -match $pattern) `
        "BattleShip Link entry $descriptor descriptor update/draw contract drifted."
}

$entrySource = Get-Content -LiteralPath (Join-Path $root `
    'decomp\BattleShip-main\decomp\src\ft\ftcommon\ftcommonentry.c') -Raw
Assert-LinkEntryCheck ($entrySource -match
    '(?s)nFTKindLink.*?efManagerLinkEntryWaveMakeEffect.*?efManagerLinkEntryBeamMakeEffect') `
    'BattleShip Link Appear no longer creates both entry effects in source order.'

$nativeSource = Get-Content -LiteralPath (Join-Path $root `
    'src\nds\nds_renderer_native_common.c') -Raw
Assert-LinkEntryCheck ($nativeSource -match
    '(?s)owner_asset_id == 353u.*?NDS_ENTRY_EFFECT_LINK_ROOT_FIRST.*?owner_asset_id == 353u.*?NDS_ENTRY_EFFECT_LINK_BOOMERANG_ROOT_FIRST') `
    'Native entry root lookup does not bound Link asset 353 to its generated roots.'

$adapterSource = Get-Content -LiteralPath (Join-Path $root `
    'src\port\renderer_adapter_stage.c') -Raw
foreach ($token in @(
    'gFTDataLinkSpecial2 != NULL',
    'root_offset == 0x02d8u',
    'root_offset == 0x0698u',
    'owner_asset_id = 353u'
)) {
    Assert-LinkEntryCheck $adapterSource.Contains($token) `
        "Link entry native admission is missing: $token"
}

$managerSource = Get-Content -LiteralPath (Join-Path $root `
    'src\\import\\battleship_efmanager.c') -Raw
foreach ($token in @(
    'if (file_head == &gFTDataLinkSpecial2)',
    '#define NDS_EF_ROSTER_DESCS_LINK(X)',
    'X(dEFManagerLinkEntryWaveEffectDesc)',
    'X(dEFManagerLinkEntryBeamEffectDesc)',
    'X(dEFManagerLinkSpinAttackEffectDesc)',
    'NDS_EF_ROSTER_DESCS_LINK(X)'
)) {
    Assert-LinkEntryCheck $managerSource.Contains($token) `
        "Link effect descriptor relocation/residency seam is missing: $token"
}

# Read the corpus figures out of the generated header rather than restating
# them. A hard-coded summary line drifted silently for two owners here: the
# generated file went to 47 roots and this line still said 41.
$corpus = @{}
foreach ($name in @('ROOT_COUNT', 'GROUP_COUNT', 'VERTEX_COUNT', 'TEXTURE_COUNT')) {
    $match = [regex]::Match($generatedText,
        ('#define NDS_ENTRY_EFFECT_' + $name + ' (\d+)u'))
    Assert-LinkEntryCheck $match.Success `
        ("Generated Link entry packet corpus is missing NDS_ENTRY_EFFECT_$name")
    $corpus[$name] = $match.Groups[1].Value
}
Write-Output ('P2_LINK_ENTRY_NATIVE_PACKETS_OK roots=2 groups=2 ' +
    'triangles=32 textures=2 corpus_roots=' + $corpus['ROOT_COUNT'] +
    ' corpus_groups=' + $corpus['GROUP_COUNT'] +
    ' corpus_triangles=' + ([int]$corpus['VERTEX_COUNT'] / 3) +
    ' corpus_textures=' + $corpus['TEXTURE_COUNT'])
