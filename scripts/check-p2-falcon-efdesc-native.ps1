[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path

function Assert-FalconEFDescNative {
    param([bool]$Condition, [string]$Message)
    if (-not $Condition) { throw $Message }
}

$generator = Join-Path $root 'scripts\3d_vfx\generate_nds_entry_effects.py'
$generated = Join-Path $root 'src\nds\nds_entry_effects.generated.inc'
$kickO2R = Join-Path $root `
    'decomp\BattleShip-main\BattleShip_o2r\reloc_fighters_main\CaptainSpecial2'
$punchO2R = Join-Path $root `
    'decomp\BattleShip-main\BattleShip_o2r\reloc_fighters_main\CaptainSpecial3'

& python $generator --check
Assert-FalconEFDescNative ($LASTEXITCODE -eq 0) `
    'Falcon EFDesc native packet generator failed.'

$kickHash = (Get-FileHash -LiteralPath $kickO2R -Algorithm SHA256).Hash.ToLowerInvariant()
$punchHash = (Get-FileHash -LiteralPath $punchO2R -Algorithm SHA256).Hash.ToLowerInvariant()
Assert-FalconEFDescNative ($kickHash -eq
    '6cb72c3f7c0a161d30572c773c2316e18479c4e6bce182cd0d0777721e6d1f3c') `
    "CaptainSpecial2 O2R hash drifted: $kickHash"
Assert-FalconEFDescNative ($punchHash -eq
    'd79b23c7ca0f6262c7481651ea9d4986acc0e7488f53fc29893f29ad6d555e33') `
    "CaptainSpecial3 O2R hash drifted: $punchHash"

$generatedText = Get-Content -LiteralPath $generated -Raw
# Other effects can move table ordinals. Check the named roots against their
# source offsets and bound their texture slots; --check above verifies the bake.
$effectDefines = @{}
foreach ($m in [regex]::Matches($generatedText,
    '(?m)^#define NDS_ENTRY_EFFECT_(\w+) (\d+)u\s*$')) {
    $effectDefines[$m.Groups[1].Value] = [int]$m.Groups[2].Value
}
$rootTable = [regex]::Match($generatedText,
    '(?s)sNdsEntryEffectRoots\[[^\]]+\] = \{(.*?)\r?\n\};')
$rootRows = [regex]::Matches($rootTable.Groups[1].Value,
    '\{ (0x[0-9a-f]+)u, (\d+)u, (\d+)u, (\d+)u \}')
Assert-FalconEFDescNative ($rootTable.Success -and
    ($rootRows.Count -eq $effectDefines['ROOT_COUNT'])) `
    'Generated entry-effect root table is missing or incomplete.'
foreach ($family in @(
    @{ Name = 'FALCON_KICK'; Offset = '0x0a30'; Textures = 2 },
    @{ Name = 'FALCON_PUNCH'; Offset = '0x0760'; Textures = 3 }
)) {
    $name = $family.Name
    $first = $effectDefines["${name}_ROOT_FIRST"]
    Assert-FalconEFDescNative (($null -ne $first) -and
        ($effectDefines["${name}_ROOT_COUNT"] -eq 1) -and
        ($first -lt $rootRows.Count)) "$name must name one valid root."
    $row = $rootRows[$first]
    Assert-FalconEFDescNative (($row.Groups[1].Value -eq $family.Offset) -and
        ([int]$row.Groups[2].Value -lt $effectDefines['GROUP_COUNT']) -and
        ($row.Groups[3].Value -eq '1') -and ($row.Groups[4].Value -eq '0')) `
        "$name root no longer matches its source display list."
    $slots = @(for ($i = 0; $i -lt $family.Textures; $i++) {
        $slot = $effectDefines["${name}_TEXTURE${i}_SLOT"]
        Assert-FalconEFDescNative (($null -ne $slot) -and
            ($slot -lt $effectDefines['TEXTURE_COUNT'])) `
            "$name texture $i has no valid generated slot."
        $slot
    })
    Assert-FalconEFDescNative ((@($slots | Select-Object -Unique).Count) -eq
        $family.Textures) "$name lost a distinct source texture frame."
}

$generatorText = Get-Content -LiteralPath $generator -Raw
foreach ($token in @(
    'reloc_fighters_main/CaptainSpecial3',
    'FALCON_KICK_ROOTS = (0x0A30,)',
    'FALCON_KICK_MOBJSUB_OFFSET = 0x0960',
    'FALCON_KICK_EXPECTED_TEXTURE_OFFSETS = (0x04E0, 0x0058)',
    'FALCON_PUNCH_ROOTS = (0x0760,)',
    'FALCON_PUNCH_MOBJSUB_OFFSET = 0x0690',
    'FALCON_PUNCH_EXPECTED_TEXTURE_OFFSETS = (0x0490, 0x0288, 0x0080)',
    'def source_mobjsub_texture_offsets(',
    'falcon_kick_texture_offsets = source_mobjsub_texture_offsets(',
    'falcon_punch_texture_offsets = source_mobjsub_texture_offsets(',
    'material_texture_sizes={0: (48, 48)}',
    'material_texture_sizes={0: (32, 32)}'
)) {
    Assert-FalconEFDescNative $generatorText.Contains($token) `
        "Falcon EFDesc generator contract is missing: $token"
}

$effectSource = Get-Content -LiteralPath (Join-Path $root `
    'decomp\BattleShip-main\decomp\src\ef\efmanager.c') -Raw
foreach ($token in @(
    'dEFManagerCaptainFalconKickEffectDesc',
    '&llCaptainSpecial2FalconKickDObjDesc',
    '&llCaptainSpecial2FalconKickMatAnimJoint',
    'dEFManagerCaptainFalconPunchEffectDesc',
    '&llCaptainSpecial3FalconPunchDObjDesc',
    '&llCaptainSpecial3FalconPunchMatAnimJoint',
    'dEFManagerMBallThrownEffectDesc'
)) {
    Assert-FalconEFDescNative $effectSource.Contains($token) `
        "BattleShip EFDesc source contract is missing: $token"
}

$nativeSource = Get-Content -LiteralPath (Join-Path $root `
    'src\nds\nds_renderer_native_common.c') -Raw
foreach ($token in @(
    'NDS_ENTRY_EFFECT_FALCON_KICK_ROOT_FIRST',
    'NDS_ENTRY_EFFECT_FALCON_PUNCH_ROOT_FIRST',
    '(owner_asset_id == 350u) && (root_offset == 0x0a30u)',
    '(owner_asset_id == 333u) && (root_offset == 0x0760u)',
    '(materials[0].render_tile_size_w0 != 0xf2000000u)',
    '(materials[0].texture_w1 != 0xffffffffu)',
    'gNdsFalconKickNativeSubmitCount++',
    'gNdsFalconPunchNativeSubmitCount++'
)) {
    Assert-FalconEFDescNative $nativeSource.Contains($token) `
        "Native Falcon EFDesc lookup/submit contract is missing: $token"
}

$adapterSource = Get-Content -LiteralPath (Join-Path $root `
    'src\port\renderer_adapter_stage.c') -Raw
foreach ($token in @(
    'gFTDataCaptainSpecial2 != NULL',
    'gFTDataCaptainSpecial3 != NULL',
    'root_offset == 0x0a30u',
    'root_offset == 0x0760u',
    'NDS_RENDERER_NATIVE_MATERIAL_CURRENT_IMAGE',
    'NDS_RENDERER_NATIVE_MATERIAL_RENDER_TILE_SIZE',
    'NDS_RENDERER_NATIVE_MATERIAL_TEXTURE'
)) {
    Assert-FalconEFDescNative $adapterSource.Contains($token) `
        "Falcon EFDesc adapter admission is missing: $token"
}

$managerSource = Get-Content -LiteralPath (Join-Path $root `
    'src\import\battleship_efmanager.c') -Raw
foreach ($token in @(
    'if (file_head == &gITManagerCommonData)',
    'return ndsRelocGetLoadedFileSize(&llITCommonDataFileID);',
    'X(dEFManagerMBallThrownEffectDesc)',
    '#if NDS_P2_PIKACHU || NDS_P2_PURIN',
    '#define NDS_EF_ROSTER_DESCS_MBALL_RAYS(X)',
    'NDS_EF_ROSTER_DESCS_MBALL_RAYS(X)',
    'X(dEFManagerMBallRaysEffectDesc)',
    'dEFManagerPikachuUnkEffectDesc remains intentionally unresolved'
)) {
    Assert-FalconEFDescNative $managerSource.Contains($token) `
        "EFDesc resolver contract is missing: $token"
}
$entrySource = Get-Content -LiteralPath (Join-Path $root `
    'src\\import\\battleship_ftcommon_entry.c') -Raw
foreach ($token in @(
    'if (fp->fkind == nFTKindPikachu)',
    'if (fp->fkind == nFTKindPurin)',
    'efManagerMBallRaysMakeEffect(&fp->entry_pos)'
)) {
    Assert-FalconEFDescNative $entrySource.Contains($token) `
        "Master-Ball rays source-reachability contract is missing: $token"
}
# The port's shared lbCommonDObjScaleXProcDisplay is a deliberate no-op (weapon
# and effect users own their own seam), so the single-DObj Punch descriptor
# reaches the DS renderer only through this Captain-only DLHEAD1 bridge; the
# native owner engages inside that submit.
Assert-FalconEFDescNative $managerSource.Contains(
    'dEFManagerCaptainFalconPunchEffectDesc.proc_display = gcDrawDObjDLHead1') `
    'Falcon Punch lost its DLHEAD1 draw bridge; the native owner would be unreachable.'

$platformHeader = Get-Content -LiteralPath (Join-Path $root `
    'include\nds\nds_platform.h') -Raw
$platformSource = Get-Content -LiteralPath (Join-Path $root `
    'src\nds\nds_platform.c') -Raw
foreach ($token in @(
    '#define NDS_EFDESC_NATIVE_GROUP(X)',
    'X(gNdsFalconKickNativeSubmitCount)',
    'X(gNdsFalconPunchNativeSubmitCount)',
    'X(gNdsEffectRendererRejectedDrawCount)'
)) {
    Assert-FalconEFDescNative $platformHeader.Contains($token) `
        "Falcon EFDesc debugger publication group is missing: $token"
}
Assert-FalconEFDescNative $platformSource.Contains(
    'NDS_PUBLISH_DEBUGGER_GROUP(NDS_EFDESC_NATIVE_GROUP);') `
    'Falcon EFDesc counters are not published through their debugger group.'

$verifyAll = Get-Content -LiteralPath (Join-Path $root 'scripts\verify-all.ps1') -Raw
Assert-FalconEFDescNative $verifyAll.Contains(
    "'check-p2-falcon-efdesc-native.ps1'") `
    'Falcon EFDesc checker is not registered in verify-all.ps1.'

$itemSource = Get-Content -LiteralPath (Join-Path $root `
    'src\import\battleship_item_link_core.c') -Raw
foreach ($token in @(
    'void *gITManagerCommonData;',
    'gITManagerCommonData = lbRelocGetExternHeapFile(',
    '&llITCommonDataFileID'
)) {
    Assert-FalconEFDescNative $itemSource.Contains($token) `
        "Item-common residency proof is missing: $token"
}

Write-Output 'P2_FALCON_EFDESC_NATIVE_OK kick_root=0x0A30 punch_root=0x0760 kick_tex=2 punch_tex=3 mball_resolver=1'
