param()
$ErrorActionPreference = 'Stop'

$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$localHeaderPath = Join-Path $root 'include/ft/fighter.h'
$localShimPath = Join-Path $root 'src/port/reloc_backend_compat_shims.c'
$upstreamRoot = Join-Path $root 'decomp/BattleShip-main/decomp'
$upstreamHeaderPath = Join-Path $upstreamRoot 'src/gm/gmdef.h'
$foxMotionPath = Join-Path $upstreamRoot 'src/relocData/208_FoxMainMotion.c'

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
    "Intangible=$($localValues['Intangible']); Fox smash restore semantics guarded.")
