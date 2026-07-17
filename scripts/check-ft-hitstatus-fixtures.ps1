param()
$ErrorActionPreference = 'Stop'

$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$localHeaderPath = Join-Path $root 'include/ft/fighter.h'
$localShimPath = Join-Path $root 'src/port/reloc_backend_compat_shims.c'
$upstreamRoot = Join-Path $root 'decomp/BattleShip-main/decomp'
$upstreamHeaderPath = Join-Path $upstreamRoot 'src/gm/gmdef.h'
$foxMotionPath = Join-Path $upstreamRoot 'src/relocData/208_FoxMainMotion.c'
$foxRelocDescriptionsPath = Join-Path $upstreamRoot `
    'tools/relocFileDescriptions.us.txt'
$foxO2RRoot = Join-Path $root `
    'decomp/BattleShip-main/BattleShip_o2r/reloc_animations'
$makefilePath = Join-Path $root 'Makefile'
$ndsRelocAssetsPath = Join-Path $root 'src/nds/nds_reloc_assets.c'
$relocBackendAssetsPath = Join-Path $root `
    'src/port/reloc_backend_assets.c'

function Assert-True {
    param(
        [bool]$Condition,
        [string]$Message
    )
    if (-not $Condition) {
        throw $Message
    }
}

function Get-HitStatusValues {
    param(
        [string]$Source,
        [string]$Label
    )

    $enumMatch = [regex]::Match(
        $Source,
        '(?s)(?:typedef\s+)?enum(?:\s+GMHitStatus)?\s*\{(?<body>[^{}]*nGMHitStatusNone[^{}]*)\}')
    Assert-True $enumMatch.Success "$Label GMHitStatus enum was not found."

    $values = @{}
    $nextValue = 0
    $entries = [regex]::Matches(
        $enumMatch.Groups['body'].Value,
        'nGMHitStatus(?<name>None|Normal|Invincible|Intangible)\s*(?:=\s*(?<value>\d+))?')
    foreach ($entry in $entries) {
        if ($entry.Groups['value'].Success) {
            $nextValue = [int]$entry.Groups['value'].Value
        }
        $values[$entry.Groups['name'].Value] = $nextValue
        $nextValue++
    }
    Assert-True ($values.Count -eq 4) "$Label GMHitStatus enum is incomplete."
    return $values
}

function Get-FunctionSlice {
    param(
        [string]$Source,
        [string]$FunctionName,
        [string]$NextFunctionName
    )

    $start = $Source.IndexOf("void $FunctionName", [StringComparison]::Ordinal)
    $end = $Source.IndexOf("void $NextFunctionName", [StringComparison]::Ordinal)
    Assert-True (($start -ge 0) -and ($end -gt $start)) `
        "Could not isolate $FunctionName in the compatibility shim."
    return $Source.Substring($start, $end - $start)
}

$localHeader = Get-Content $localHeaderPath -Raw
$upstreamHeader = Get-Content $upstreamHeaderPath -Raw
$localValues = Get-HitStatusValues $localHeader 'Local'
$upstreamValues = Get-HitStatusValues $upstreamHeader 'BattleShip'

foreach ($name in @('None', 'Normal', 'Invincible', 'Intangible')) {
    Assert-True ($localValues[$name] -eq $upstreamValues[$name]) `
        "GMHitStatus ABI drift for $name`: local=$($localValues[$name]) BattleShip=$($upstreamValues[$name])."
}

$foxMotion = Get-Content $foxMotionPath -Raw
$restoreCount = [regex]::Matches(
    $foxMotion,
    'ftMotionCommandSetHitStatusPartAll\(1\)').Count
Assert-True ($restoreCount -ge 2) `
    'BattleShip Fox up/down-smash hit-status restore commands were not found.'

$foxRelocDescriptions = Get-Content $foxRelocDescriptionsPath -Raw
$foxCommonRows = @([regex]::Matches(
    $foxRelocDescriptions,
    '(?m)^-(?<id>64[2-9]|65[0-9]|66[01]): FTFoxAnim(?<name>[A-Za-z0-9]+)$') |
    ForEach-Object {
        [pscustomobject]@{
            Id = [int]$_.Groups['id'].Value
            Name = $_.Groups['name'].Value
        }
    } | Sort-Object Id)
Assert-True ($foxCommonRows.Count -eq 20) `
    'BattleShip Fox files 642..661 are no longer one complete common bank.'

# Files 751..771 are BattleShip's contiguous Fox Jab1-through-Down-Air bank.
# Pin both banks' semantic manifests, O2R identities, packaged paths, and
# token-table order so adjacent motions cannot silently share stale data.
$foxCombatRows = @([regex]::Matches(
    $foxRelocDescriptions,
    '(?m)^-(?<id>75[1-9]|76[0-9]|77[01]): FTFoxAnim(?<name>[A-Za-z0-9]+)$') |
    ForEach-Object {
        [pscustomobject]@{
            Id = [int]$_.Groups['id'].Value
            Name = $_.Groups['name'].Value
        }
    } | Sort-Object Id)
Assert-True ($foxCombatRows.Count -eq 21) `
    'BattleShip Fox files 751..771 are no longer one complete combat bank.'

$makefile = Get-Content $makefilePath -Raw
$ndsRelocAssets = Get-Content $ndsRelocAssetsPath -Raw
$relocBackendAssets = Get-Content $relocBackendAssetsPath -Raw
$foxCommonTableMatch = [regex]::Match(
    $relocBackendAssets,
    '(?s)sNdsRelocFoxCommonAnimFileIDs\[\]\s*=\s*\{(?<body>.*?)\};')
Assert-True $foxCommonTableMatch.Success `
    'The Fox common animation token table is missing.'
$foxCommonTableSymbols = @([regex]::Matches(
    $foxCommonTableMatch.Groups['body'].Value,
    '&(?<symbol>llFTFoxAnim[A-Za-z0-9]+FileID)') |
    ForEach-Object { $_.Groups['symbol'].Value })
Assert-True ($foxCommonTableSymbols.Count -eq $foxCommonRows.Count) `
    'The Fox common token table does not cover files 642..661 exactly.'

for ($i = 0; $i -lt $foxCommonRows.Count; $i++) {
    $row = $foxCommonRows[$i]
    $expectedId = 642 + $i
    $animName = 'FTFoxAnim{0:D3}' -f $i
    $o2rPath = Join-Path $foxO2RRoot $animName
    $expectedSymbol = "llFTFoxAnim$($row.Name)FileID"
    $hexId = '0x{0:x}' -f $expectedId

    Assert-True ($row.Id -eq $expectedId) `
        "Fox common semantic manifest skipped file $expectedId."
    Assert-True ($foxCommonTableSymbols[$i] -eq $expectedSymbol) `
        "Fox common token $expectedId maps $($foxCommonTableSymbols[$i]) instead of $expectedSymbol."
    Assert-True (Test-Path -LiteralPath $o2rPath -PathType Leaf) `
        "Fox common O2R asset is missing: $animName."
    [byte[]]$o2r = [System.IO.File]::ReadAllBytes($o2rPath)
    Assert-True (($o2r.Length -ge 0x50) -and
        ([BitConverter]::ToUInt32($o2r, 0x40) -eq $expectedId)) `
        "$animName does not carry BattleShip file ID $expectedId."
    Assert-True ($makefile.Contains("reloc_animations/$animName")) `
        "$animName is absent from NDS_MARIOFOX_FIGHTER_RELOC_FILES."
    $assetEntry = ('{{ 0x{0:x}, 0x{0:x}, "nitro:/reloc/reloc_animations/{1}" }}' -f
        $expectedId, $animName)
    Assert-True ($ndsRelocAssets.Contains($assetEntry)) `
        "$animName lacks its exact $hexId DS relocation entry."
}
Assert-True ($relocBackendAssets.Contains(
        '#define NDS_RELOC_ASSET_FOX_ANIM_EGGLAY 0x282u')) `
    'Fox common token-table base is no longer exact file 642.'
Assert-True ($relocBackendAssets.Contains(
        '#define NDS_RELOC_ASSET_FOX_ANIM_LANDING_AIR_X 0x295u')) `
    'Fox common token-table end is no longer exact LandingAirX file 661.'
Assert-True ($relocBackendAssets.Contains(
        'return NDS_RELOC_ASSET_FOX_ANIM_EGGLAY + i;')) `
    'Fox common token-table order is not translated from its exact base.'
Assert-True ([regex]::IsMatch(
        $relocBackendAssets,
        '(?s)asset_id >= NDS_RELOC_ASSET_FOX_ANIM_EGGLAY\).*?asset_id <= NDS_RELOC_ASSET_FOX_ANIM_LANDING_AIR_X')) `
    'Fox common bank is not covered by AObj16 normalization.'
Assert-True (-not $relocBackendAssets.Contains(
        '#define NDS_RELOC_ASSET_FOX_ANIM_JUMP_F 0x292u')) `
    'The old JumpF-to-Crouch shifted mapping returned.'
Assert-True (-not $relocBackendAssets.Contains(
        '#define NDS_RELOC_ASSET_FOX_ANIM_JUMP_AERIAL_B 0x295u')) `
    'The old JumpAerialB-to-LandingAirX shifted mapping returned.'

$foxTableMatch = [regex]::Match(
    $relocBackendAssets,
    '(?s)sNdsRelocFoxCombatAnimFileIDs\[\]\s*=\s*\{(?<body>.*?)\};')
Assert-True $foxTableMatch.Success `
    'The Fox combat animation token table is missing.'
$foxTableSymbols = @([regex]::Matches(
    $foxTableMatch.Groups['body'].Value,
    '&(?<symbol>llFTFoxAnim[A-Za-z0-9]+FileID)') |
    ForEach-Object { $_.Groups['symbol'].Value })
Assert-True ($foxTableSymbols.Count -eq $foxCombatRows.Count) `
    'The Fox combat token table does not cover files 751..771 exactly.'

$aerialHashes = @{
    767 = '184d08bd10b21223c4195a9ea630176e5e7f4e76d21b2e05b115b0189d9c44b2'
    768 = '5e408b50973346340d73962693c8dab15793ccf965ff0d8172e4339fe3ed777a'
    769 = 'a35e7f06a3f4ca31b3022175ce85436be207cf49b99fd1a937b00524131d8710'
    770 = 'fbbb01e9912f9170c1e5979cd4098ab5f716ca2f62d2e326f490db944b632815'
    771 = '8f5a16e26b90ea7e55f33ae16d5ff05e1f0d8fb2a604446862c92e96d842827c'
}
for ($i = 0; $i -lt $foxCombatRows.Count; $i++) {
    $row = $foxCombatRows[$i]
    $expectedId = 751 + $i
    $animIndex = 109 + $i
    $animName = 'FTFoxAnim{0:D3}' -f $animIndex
    $o2rPath = Join-Path $foxO2RRoot $animName
    $expectedSymbol = "llFTFoxAnim$($row.Name)FileID"
    $hexId = '0x{0:x}' -f $expectedId

    Assert-True ($row.Id -eq $expectedId) `
        "Fox combat semantic manifest skipped file $expectedId."
    Assert-True ($foxTableSymbols[$i] -eq $expectedSymbol) `
        "Fox combat token $expectedId maps $($foxTableSymbols[$i]) instead of $expectedSymbol."
    Assert-True (Test-Path -LiteralPath $o2rPath -PathType Leaf) `
        "Fox combat O2R asset is missing: $animName."
    [byte[]]$o2r = [System.IO.File]::ReadAllBytes($o2rPath)
    Assert-True (($o2r.Length -ge 0x50) -and
        ([BitConverter]::ToUInt32($o2r, 0x40) -eq $expectedId)) `
        "$animName does not carry BattleShip file ID $expectedId."
    Assert-True ($makefile.Contains("reloc_animations/$animName")) `
        "$animName is absent from NDS_MARIOFOX_FIGHTER_RELOC_FILES."
    $assetEntry = ('{{ 0x{0:x}, 0x{0:x}, "nitro:/reloc/reloc_animations/{1}" }}' -f
        $expectedId, $animName)
    Assert-True ($ndsRelocAssets.Contains($assetEntry)) `
        "$animName lacks its exact $hexId DS relocation entry."

    if ($aerialHashes.ContainsKey($expectedId)) {
        $actualHash = (Get-FileHash -LiteralPath $o2rPath `
            -Algorithm SHA256).Hash.ToLowerInvariant()
        Assert-True ($actualHash -eq $aerialHashes[$expectedId]) `
            "$animName no longer matches its pinned BattleShip aerial payload."
    }
}
Assert-True ($relocBackendAssets.Contains(
        '#define NDS_RELOC_ASSET_FOX_ANIM_JAB1 0x2efu')) `
    'Fox combat token-table base is no longer exact file 751.'
Assert-True ($relocBackendAssets.Contains(
        '#define NDS_RELOC_ASSET_FOX_ANIM_ATTACK_AIR_D 0x303u')) `
    'Fox combat token-table end is no longer exact Down-Air file 771.'
Assert-True ($relocBackendAssets.Contains(
        'return NDS_RELOC_ASSET_FOX_ANIM_JAB1 + i;')) `
    'Fox combat token-table order is not translated from its exact base.'
Assert-True ([regex]::Matches(
        $relocBackendAssets,
        'asset_id <= NDS_RELOC_ASSET_FOX_ANIM_ATTACK_AIR_D').Count -eq 2) `
    'Fox Down-Air is not covered by both AObj16 and fighter ownership ranges.'

$localShim = Get-Content $localShimPath -Raw
$partAll = Get-FunctionSlice $localShim 'ftParamSetHitStatusPartAll' `
    'ftParamResetFighterDamageCollsAll'
$partID = Get-FunctionSlice $localShim 'ftParamSetHitStatusPartID' `
    'ftParamResetModelPartAll'
$all = Get-FunctionSlice $localShim 'ftParamSetHitStatusAll' `
    'func_ovl0_800C9A38'

Assert-True ($partAll.Contains(
        'fp->damage_colls[i].hitstatus != nGMHitStatusNone')) `
    'PartAll must skip inactive damage colliders like BattleShip.'
Assert-True ($partAll.Contains('fp->is_hitstatus_nodamage =')) `
    'PartAll must maintain the BattleShip no-damage reset flag.'
Assert-True ($partAll.Contains(
        '(hitstatus == nGMHitStatusNormal) ? FALSE : TRUE')) `
    'PartAll no-damage reset semantics drifted from BattleShip.'
Assert-True ($partID.Contains(
        'fp->damage_colls[i].hitstatus != nGMHitStatusNone')) `
    'PartID must skip inactive damage colliders like BattleShip.'
Assert-True ($partID.Contains('if (hitstatus != nGMHitStatusNormal)')) `
    'PartID must raise the no-damage reset flag for protected parts.'
Assert-True ($partID.Contains('fp->is_hitstatus_nodamage = TRUE;')) `
    'PartID no-damage reset flag assignment is missing.'
Assert-True ($partID.Contains('break;')) `
    'PartID must stop after the first matching damage collider.'
Assert-True ($all.Contains('fp->hitstatus = hitstatus;')) `
    'HitStatusAll must update the fighter hit status.'
Assert-True (-not $all.Contains('special_hitstatus')) `
    'HitStatusAll must not overwrite BattleShip special hit status.'

Write-Output ('FT hit-status fixtures passed: ' +
    "None=$($localValues['None']) Normal=$($localValues['Normal']) " +
    "Invincible=$($localValues['Invincible']) " +
    "Intangible=$($localValues['Intangible']); Fox files 642..661/751..771 and smash restore semantics guarded.")
