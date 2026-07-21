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
$foxRows = @([regex]::Matches(
    $foxRelocDescriptions,
    '(?m)^-(?<id>6(?:4[2-9]|[5-9][0-9])|7[0-9][0-9]): FTFoxAnim(?<name>[A-Za-z0-9]+)$') |
    ForEach-Object {
        [pscustomobject]@{
            Id = [int]$_.Groups['id'].Value
            Name = $_.Groups['name'].Value
        }
    } | Where-Object Id -le 799 | Sort-Object Id)
Assert-True ($foxRows.Count -eq 158) `
    'BattleShip Fox files 642..799 are no longer one complete animation bank.'

$makefile = Get-Content $makefilePath -Raw
$ndsRelocAssets = Get-Content $ndsRelocAssetsPath -Raw
$relocBackendAssets = Get-Content $relocBackendAssetsPath -Raw
$foxTableMatch = [regex]::Match(
    $relocBackendAssets,
    '(?s)sNdsRelocFoxAnimFileIDs\[\]\s*=\s*\{(?<body>.*?)\};')
Assert-True $foxTableMatch.Success `
    'The complete Fox animation token table is missing.'
$foxTableSymbols = @([regex]::Matches(
    $foxTableMatch.Groups['body'].Value,
    '&(?<symbol>llFTFoxAnim[A-Za-z0-9]+FileID)') |
    ForEach-Object { $_.Groups['symbol'].Value })
Assert-True ($foxTableSymbols.Count -eq $foxRows.Count) `
    'The Fox token table does not cover files 642..799 exactly.'

$aerialHashes = @{
    767 = '184d08bd10b21223c4195a9ea630176e5e7f4e76d21b2e05b115b0189d9c44b2'
    768 = '5e408b50973346340d73962693c8dab15793ccf965ff0d8172e4339fe3ed777a'
    769 = 'a35e7f06a3f4ca31b3022175ce85436be207cf49b99fd1a937b00524131d8710'
    770 = 'fbbb01e9912f9170c1e5979cd4098ab5f716ca2f62d2e326f490db944b632815'
    771 = '8f5a16e26b90ea7e55f33ae16d5ff05e1f0d8fb2a604446862c92e96d842827c'
}
for ($i = 0; $i -lt $foxRows.Count; $i++) {
    $row = $foxRows[$i]
    $expectedId = 642 + $i
    $animName = 'FTFoxAnim{0:D3}' -f $i
    $o2rPath = Join-Path $foxO2RRoot $animName
    $expectedSymbol = "llFTFoxAnim$($row.Name)FileID"

    Assert-True ($row.Id -eq $expectedId) `
        "Fox semantic manifest skipped file $expectedId."
    Assert-True ($foxTableSymbols[$i] -eq $expectedSymbol) `
        "Fox token $expectedId maps $($foxTableSymbols[$i]) instead of $expectedSymbol."
    Assert-True (Test-Path -LiteralPath $o2rPath -PathType Leaf) `
        "Fox O2R asset is missing: $animName."
    [byte[]]$o2r = [System.IO.File]::ReadAllBytes($o2rPath)
    Assert-True (($o2r.Length -ge 0x50) -and
        ([BitConverter]::ToUInt32($o2r, 0x40) -eq $expectedId)) `
        "$animName does not carry BattleShip file ID $expectedId."
    Assert-True ($makefile.Contains("reloc_animations/$animName")) `
        "$animName is absent from NDS_MARIOFOX_FIGHTER_RELOC_FILES."

    if ($aerialHashes.ContainsKey($expectedId)) {
        $actualHash = (Get-FileHash -LiteralPath $o2rPath `
            -Algorithm SHA256).Hash.ToLowerInvariant()
        Assert-True ($actualHash -eq $aerialHashes[$expectedId]) `
            "$animName no longer matches its pinned BattleShip aerial payload."
    }
}
Assert-True ($relocBackendAssets.Contains(
        '#define NDS_RELOC_ASSET_FOX_ANIM_FIRST 0x282u')) `
    'Fox animation-bank base is no longer exact file 642.'
Assert-True ($relocBackendAssets.Contains(
        '#define NDS_RELOC_ASSET_FOX_ANIM_LAST 0x31fu')) `
    'Fox animation-bank end is no longer exact file 799.'
Assert-True ($relocBackendAssets.Contains(
        'return NDS_RELOC_ASSET_FOX_ANIM_FIRST + i;')) `
    'Fox token-table order is not translated from its exact base.'
Assert-True ([regex]::IsMatch(
        $relocBackendAssets,
        '(?s)asset_id >= NDS_RELOC_ASSET_FOX_ANIM_FIRST\).*?asset_id <= NDS_RELOC_ASSET_FOX_ANIM_LAST')) `
    'The complete Fox bank is not covered by AObj16 normalization.'
Assert-True ($ndsRelocAssets.Contains(
        '#define NDS_RELOC_FOX_ANIM_FIRST 0x282u')) `
    'The Fox dynamic NitroFS path range has the wrong base.'
Assert-True ($ndsRelocAssets.Contains(
        '#define NDS_RELOC_FOX_ANIM_LAST 0x31fu')) `
    'The Fox dynamic NitroFS path range has the wrong end.'
Assert-True ($ndsRelocAssets.Contains(
        '"nitro:/reloc/reloc_animations/FTFoxAnim%03lu"')) `
    'The Fox dynamic NitroFS path owner is missing.'
Assert-True ([regex]::IsMatch(
        $relocBackendAssets,
        '(?s)memset\(heap, 0, asset_size\);.*?fail:.*?memset\(heap, 0, asset_size\);')) `
    'Failed fighter-animation loads can retain stale heap bytes.'


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
    "Intangible=$($localValues['Intangible']); all 158 Fox animations and smash restore semantics guarded.")
