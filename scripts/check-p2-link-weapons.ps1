[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path

function Assert-LinkWeaponCheck {
    param([bool]$Condition, [string]$Message)
    if (-not $Condition) { throw $Message }
}

& (Join-Path $root 'scripts\check-p2-link-entry-effects.ps1')
Assert-LinkWeaponCheck ($LASTEXITCODE -eq 0) `
    'Shared Link native packet corpus failed before weapon checks.'
& python (Join-Path $root 'scripts\check_p2_link_weapon_attributes.py')
Assert-LinkWeaponCheck ($LASTEXITCODE -eq 0) `
    'Link weapon WPAttributes byte oracle failed.'

$o2rSpecs = @(
    @('LinkMain',
      '8a771a6e2b9e9d8e4b1b103d4c2be332e7290c36adf47c324822c2957797a005'),
    @('LinkSpecial1',
      'ffa3b112d4d57eb1c66604b16da7de72a7a538100de8278dd4ca50be5e1fa84d'),
    @('LinkSpecial2',
      '3decd2670e012cffb135b47b4caabf66db1b90fd637f45408a3e8641f1ea31f1'),
    @('LinkModel',
      '93c9ee108c0e8f1680c35d8d11ec980891850cadcac5eed5bd731c43e85f163e'),
    @('LinkSpecial3',
      '3d3224bd445090f8f4cf52937d2b0ea7a2740656aba63826a9d396b4599e18fb')
)
foreach ($spec in $o2rSpecs) {
    $path = Join-Path $root `
        "decomp\BattleShip-main\BattleShip_o2r\reloc_fighters_main\$($spec[0])"
    $hash = (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant()
    Assert-LinkWeaponCheck ($hash -eq $spec[1]) `
        "$($spec[0]) O2R hash drifted: $hash"
}

$generated = Get-Content -LiteralPath (Join-Path $root `
    'src\nds\nds_entry_effects.generated.inc') -Raw
foreach ($token in @(
    '#define NDS_ENTRY_EFFECT_ROOT_COUNT 29u',
    '#define NDS_ENTRY_EFFECT_GROUP_COUNT 71u',
    '#define NDS_ENTRY_EFFECT_TEXTURE_COUNT 44u',
    '#define NDS_ENTRY_EFFECT_LINK_SPIN_WEAPON_ROOT_FIRST 26u',
    '#define NDS_ENTRY_EFFECT_LINK_BOOMERANG_ROOT_FIRST 27u',
    '{ 0x1100u, 59u, 1u, 0u }',
    '{ 0x11680u, 60u, 9u, 0u }',
    '{ 0x0458u, 69u, 1u, 0u }',
    '{ 0x0580u, 70u, 1u, 0u }'
)) {
    Assert-LinkWeaponCheck $generated.Contains($token) `
        "Generated Link weapon packet corpus is missing: $token"
}

$generator = Get-Content -LiteralPath (Join-Path $root `
    'scripts\3d_vfx\generate_nds_entry_effects.py') -Raw
foreach ($token in @(
    'LINK_SPECIAL2_ROOTS = (0x02D8, 0x0698, 0x1100)',
    'LINK_MODEL_SPIN_ROOTS = (0x11680,)',
    'reloc_fighters_main/LinkModel',
    '93c9ee108c0e8f1680c35d8d11ec980891850cadcac5eed5bd731c43e85f163e',
    'LINK_SPECIAL3_ROOTS = (0x0458, 0x0580)',
    'reloc_fighters_main/LinkSpecial3',
    '3d3224bd445090f8f4cf52937d2b0ea7a2740656aba63826a9d396b4599e18fb'
)) {
    Assert-LinkWeaponCheck $generator.Contains($token) `
        "Link weapon packet generator is missing: $token"
}

$loader = Get-Content -LiteralPath (Join-Path $root `
    'src\port\reloc_backend_assets.c') -Raw
foreach ($token in @(
    'NDS_RELOC_SYMBOL_LINK_MAIN_SPIN_ATTACK_WEAPON_ATTRIBUTES 0x0cu',
    'NDS_RELOC_SYMBOL_LINK_SPECIAL1_BOOMERANG_WEAPON_ATTRIBUTES 0x00u',
    'NDS_RELOC_ASSET_LINK_SPECIAL1 0xe2u',
    'link_spin_attack_attr',
    'asset_id == NDS_RELOC_ASSET_LINK_SPECIAL1',
    'attr->map_coll_top == 150',
    'attr->map_coll_bottom == -150',
    'attr->map_coll_width == 150'
)) {
    Assert-LinkWeaponCheck $loader.Contains($token) `
        "Link weapon attribute normalization is missing: $token"
}

$weaponSource = Get-Content -LiteralPath (Join-Path $root `
    'src\import\battleship_link_weapons.c') -Raw
foreach ($token in @(
    'wplinkspinattack.c',
    'wplinkboomerang.c',
    'llLinkMainSpinAttackWeaponAttributes = 0x0cu',
    'llLinkSpecial1BoomerangWeaponAttributes = 0x00u'
)) {
    Assert-LinkWeaponCheck $weaponSource.Contains($token) `
        "BattleShip Link weapon import is missing: $token"
}

$movement = Get-Content -LiteralPath (Join-Path $root `
    'src\port\reloc_backend_movement.c') -Raw
Assert-LinkWeaponCheck ($movement -match
    '(?s)ndsStageGCDrawAllLoopSubmitWeaponDObj.*?root->child == NULL.*?DOBJ_DLHEAD1.*?DOBJ_TREE.*?DOBJ_TREE_DLLINKS') `
    'Shared weapon submit does not admit Link Boomerang/Spin Attack tree callbacks.'

$adapter = Get-Content -LiteralPath (Join-Path $root `
    'src\port\renderer_adapter_stage.c') -Raw
foreach ($token in @(
    'root_offset == 0x1100u',
    'NDS_RENDERER_NATIVE_MATERIAL_LIGHT1',
    'NDS_RENDERER_NATIVE_MATERIAL_LIGHT2',
    'NDS_RENDERER_NATIVE_MATERIAL_RENDER_TILE_SIZE',
    'NDS_RENDERER_NATIVE_MATERIAL_TEXTURE',
    'gFTDataLinkModel != NULL',
    'root_offset == 0x11680u',
    'owner_asset_id = 324u',
    'gFTDataLinkSpecial3 != NULL',
    'root_offset == 0x0458u',
    'root_offset == 0x0580u',
    'owner_asset_id = 325u'
)) {
    Assert-LinkWeaponCheck $adapter.Contains($token) `
        "Link weapon native admission is missing: $token"
}

$native = Get-Content -LiteralPath (Join-Path $root `
    'src\nds\nds_renderer_native_common.c') -Raw
Assert-LinkWeaponCheck ($native -match
    '(?s)owner_asset_id == 325u.*?NDS_ENTRY_EFFECT_LINK_BOOMERANG_ROOT_FIRST.*?owner_asset_id == 325u.*?NDS_ENTRY_EFFECT_ROOT_COUNT') `
    'Native packet lookup does not bound LinkSpecial3 asset 325.'
Assert-LinkWeaponCheck ($native -match
    '(?s)owner_asset_id == 324u.*?NDS_ENTRY_EFFECT_LINK_SPIN_WEAPON_ROOT_FIRST.*?owner_asset_id == 324u.*?NDS_ENTRY_EFFECT_LINK_BOOMERANG_ROOT_FIRST') `
    'Native packet lookup does not bound LinkModel asset 324 Spin weapon.'
Assert-LinkWeaponCheck ($native -match
    '(?s)LinkModel\+0x11680.*?material_count != 9u.*?NDS_RENDERER_NATIVE_MATERIAL_PRIM') `
    'LinkModel Spin native owner no longer enforces the exact nine-MObj PRIM contract.'
Assert-LinkWeaponCheck ($native -match
    '(?s)owner_asset_id == 353u.*?case 0x02d8u:.*?NDS_RENDERER_NATIVE_MATERIAL_RENDER_TILE_SIZE.*?NDS_RENDERER_NATIVE_MATERIAL_TEXTURE.*?case 0x0698u:.*?NDS_RENDERER_NATIVE_MATERIAL_LIGHT1.*?case 0x1100u:.*?NDS_RENDERER_NATIVE_MATERIAL_PRIM') `
    'LinkSpecial2 native owner no longer enforces the source Wave/Beam/Spin live-MObj contracts.'

$boomerangAttr = Get-Content -LiteralPath (Join-Path $root `
    'decomp\BattleShip-main\decomp\src\relocData\226_LinkSpecial1.c') -Raw
Assert-LinkWeaponCheck ($boomerangAttr -match
    '(?s)attack_offsets.*?150, 0, -150, 150.*?200,.*?70,.*?30,.*?9,') `
    'BattleShip Boomerang WPAttributes source contract drifted.'

$boomerang = Get-Content -LiteralPath (Join-Path $root `
    'decomp\BattleShip-main\decomp\src\wp\wplink\wplinkboomerang.c') -Raw
foreach ($token in @(
    'WPBOOMERANG_LIFETIME_SMASH',
    'WPBOOMERANG_LIFETIME_TILT',
    'wpLinkBoomerangCheckOwnerCatch',
    'ftLinkSpecialNGetSetStatus',
    'wpLinkBoomerangProcReflector'
)) {
    Assert-LinkWeaponCheck $boomerang.Contains($token) `
        "BattleShip Boomerang behavior contract is missing: $token"
}

$spin = Get-Content -LiteralPath (Join-Path $root `
    'decomp\BattleShip-main\decomp\src\wp\wplink\wplinkspinattack.c') -Raw
foreach ($token in @(
    'WPSPINATTACK_LIFETIME',
    'WPSPINATTACK_EXTEND_POS_COUNT',
    'wpLinkSpinAttackMakeWeapon',
    'wpLinkSpinAttackProcUpdate',
    'wpLinkSpinAttackProcMap'
)) {
    Assert-LinkWeaponCheck $spin.Contains($token) `
        "BattleShip Spin Attack behavior contract is missing: $token"
}

Write-Output ('P2_LINK_WEAPON_STATIC_OK boomerang_roots=2 ' +
    'spin_effect_roots=1 spin_weapon_roots=1 spin_weapon_groups=9 ' +
    'corpus_roots=29 corpus_groups=71 corpus_triangles=456 corpus_textures=44')
