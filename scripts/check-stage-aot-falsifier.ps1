param()

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$paths = [ordered]@{
    MapHeader = Join-Path $root 'decomp/BattleShip-main/decomp/src/relocData/255_GRPupupuMap.c'
    StageFile2 = Join-Path $root 'decomp/BattleShip-main/decomp/src/relocData/104_StagePupupuFile2.c'
    StageFile3 = Join-Path $root 'decomp/BattleShip-main/decomp/src/relocData/152_StagePupupuFile3.c'
    Pupupu = Join-Path $root 'decomp/BattleShip-main/decomp/src/gr/grcommon/grpupupu.c'
    GRDisplay = Join-Path $root 'decomp/BattleShip-main/decomp/src/gr/grdisplay.c'
    ObjAnim = Join-Path $root 'decomp/BattleShip-main/decomp/src/sys/objanim.c'
    ObjDisplay = Join-Path $root 'decomp/BattleShip-main/decomp/src/sys/objdisplay.c'
    RelocData = Join-Path $root 'include/reloc_data.h'
    Renderer = Join-Path $root 'src/port/reloc_backend_renderer_dl.c'
}

function Assert-True {
    param(
        [bool]$Condition,
        [string]$Message
    )

    if (-not $Condition) {
        throw "M3 stage AOT structure falsified: $Message"
    }
}

function Assert-Equal {
    param(
        [object]$Actual,
        [object]$Expected,
        [string]$Message
    )

    if ($Actual -ne $Expected) {
        throw "M3 stage AOT structure falsified: $Message actual=$Actual expected=$Expected"
    }
}

function Get-ArrayBlock {
    param(
        [string]$Text,
        [string]$DeclarationPattern,
        [string]$Name
    )

    $escapedName = [regex]::Escape($Name)
    $pattern = "(?ms)^\s*${DeclarationPattern}${escapedName}\[(?<declared>\d+)\]\s*=\s*\{\s*(?<body>.*?)^\s*\};"
    $matches = [regex]::Matches($Text, $pattern)
    Assert-Equal $matches.Count 1 "array '$Name' must have one definition"
    return [PSCustomObject]@{
        Name = $Name
        Declared = [int]$matches[0].Groups['declared'].Value
        Body = $matches[0].Groups['body'].Value
    }
}

function Get-PointerEntries {
    param(
        [object]$Array,
        [string]$Role
    )

    $pattern = '(?m)^\s*(?:\([A-Za-z_][A-Za-z0-9_\s\*]*\)\s*)?(?<name>NULL|[A-Za-z_][A-Za-z0-9_]*)\s*,\s*(?://.*)?$'
    $entries = @([regex]::Matches($Array.Body, $pattern) | ForEach-Object {
        $_.Groups['name'].Value
    })
    Assert-Equal $entries.Count $Array.Declared "$Role pointer initializer count"
    return $entries
}

function Get-DObjArrayStats {
    param(
        [string]$Text,
        [string]$Name,
        [string]$Owner
    )

    $array = Get-ArrayBlock $Text 'DObjDesc\s+' $Name
    $pattern = '(?m)^\s*\{\s*(?<id>0x[0-9A-Fa-f]+|\d+)\s*,\s*(?<dl>NULL|\(void\s*\*\)\s*(?<symbol>[A-Za-z_][A-Za-z0-9_]*))\s*,'
    $entries = @([regex]::Matches($array.Body, $pattern))
    Assert-Equal $entries.Count $array.Declared "$Owner DObjDesc initializer count"

    $sentinels = @($entries | Where-Object {
        [int]$_.Groups['id'].Value -eq 18
    })
    Assert-Equal $sentinels.Count 1 "$Owner must have exactly one DObjDesc end sentinel"
    Assert-Equal ([int]$entries[-1].Groups['id'].Value) 18 "$Owner DObjDesc sentinel must be last"
    Assert-Equal $entries[-1].Groups['dl'].Value 'NULL' "$Owner DObjDesc sentinel DL"

    $dlSymbols = @($entries[0..($entries.Count - 2)] | Where-Object {
        $_.Groups['dl'].Value -ne 'NULL'
    } | ForEach-Object {
        $_.Groups['symbol'].Value
    })

    return [PSCustomObject]@{
        Owner = $Owner
        Name = $Name
        DObjs = $array.Declared - 1
        DLReferences = $dlSymbols.Count
        DLSymbols = $dlSymbols
    }
}

function Get-FunctionSlice {
    param(
        [string]$Text,
        [string]$Signature,
        [string]$NextMarker
    )

    $start = $Text.IndexOf($Signature, [StringComparison]::Ordinal)
    Assert-True ($start -ge 0) "function signature '$Signature' not found"
    $end = $Text.IndexOf($NextMarker, $start, [StringComparison]::Ordinal)
    Assert-True ($end -gt $start) "function terminator '$NextMarker' not found after '$Signature'"
    return $Text.Substring($start, $end - $start)
}

function Get-EnumMemberCount {
    param(
        [string]$Text,
        [string]$EnumName,
        [string]$MemberPrefix
    )

    $name = [regex]::Escape($EnumName)
    $match = [regex]::Match($Text, "(?ms)enum\s+$name\s*\{(?<body>.*?)\};")
    Assert-True $match.Success "enum '$EnumName' not found"
    $members = @([regex]::Matches(
        $match.Groups['body'].Value,
        "(?m)^\s*${MemberPrefix}[A-Za-z0-9_]+\s*,?\s*(?://.*)?$"
    ) | ForEach-Object { $_.Value.Trim() } | Where-Object {
        $_ -notmatch 'EnumCount'
    })
    return $members.Count
}

foreach ($entry in $paths.GetEnumerator()) {
    Assert-True (Test-Path -LiteralPath $entry.Value -PathType Leaf) "missing input '$($entry.Key)': $($entry.Value)"
}

$mapHeaderText = Get-Content -Raw -LiteralPath $paths.MapHeader
$file2Text = Get-Content -Raw -LiteralPath $paths.StageFile2
$file3Text = Get-Content -Raw -LiteralPath $paths.StageFile3
$pupupuText = Get-Content -Raw -LiteralPath $paths.Pupupu
$grDisplayText = Get-Content -Raw -LiteralPath $paths.GRDisplay
$objAnimText = Get-Content -Raw -LiteralPath $paths.ObjAnim
$objDisplayText = Get-Content -Raw -LiteralPath $paths.ObjDisplay
$relocDataText = Get-Content -Raw -LiteralPath $paths.RelocData
$rendererText = Get-Content -Raw -LiteralPath $paths.Renderer

# Resolve the four collision/display layers from the source MPGroundData rather
# than maintaining a second list of asset names in this checker.
$headerMatch = [regex]::Match(
    $mapHeaderText,
    '(?ms)MPGroundData\s+dGRPupupuMap_header\s*=\s*\{(?<body>.*?)^\s*\};'
)
Assert-True $headerMatch.Success 'dGRPupupuMap_header definition not found'
$headerBody = $headerMatch.Groups['body'].Value
$layerPattern = '(?m)^\s*\{\s*(?:\(void\s*\*\)\s*)?(?<dobj>NULL|[A-Za-z_][A-Za-z0-9_]*)\s*,\s*(?:\(void\s*\*\)\s*)?(?<anim>NULL|[A-Za-z_][A-Za-z0-9_]*)\s*,\s*(?:\(void\s*\*\)\s*)?(?<mobj>NULL|[A-Za-z_][A-Za-z0-9_]*)\s*,\s*(?:\(void\s*\*\)\s*)?(?<matanim>NULL|[A-Za-z_][A-Za-z0-9_]*)\s*\},'
$layerMatches = @([regex]::Matches($headerBody, $layerPattern))
Assert-Equal $layerMatches.Count 4 'MPGroundData geometry-layer count'
$layerMaskMatch = [regex]::Match($headerBody, '(?m)^\s*(?<mask>\d+)\s*,\s*/\*\s*layer_mask\s*\*/')
Assert-True $layerMaskMatch.Success 'MPGroundData layer_mask not found'
Assert-Equal ([int]$layerMaskMatch.Groups['mask'].Value) 0 'Pupupu layer_mask must select primary callbacks'

# Resolve the four generic geometry callbacks and links exactly as BattleShip
# does in grDisplayMakeGeometryLayer.
$displayDescPattern = '(?ms)//\s*Layer\s+(?<layer>[0-3])\s*\{\s*(?<primary>[A-Za-z_][A-Za-z0-9_]*)\s*,\s*(?<secondary>[A-Za-z_][A-Za-z0-9_]*)\s*,\s*(?<link>\d+)\s*,\s*(?<update>[A-Za-z_][A-Za-z0-9_]*)\s*\}'
$displayDescs = @([regex]::Matches($grDisplayText, $displayDescPattern))
Assert-Equal $displayDescs.Count 4 'dGRDisplayDescs layer count'

$callbackRows = [System.Collections.Generic.List[object]]::new()
for ($i = 0; $i -lt 4; $i++) {
    Assert-Equal ([int]$displayDescs[$i].Groups['layer'].Value) $i 'dGRDisplayDescs layer order'
    $callback = $displayDescs[$i].Groups['primary'].Value
    $link = [int]$displayDescs[$i].Groups['link'].Value
    $callbackRows.Add([PSCustomObject]@{
        Owner = "layer$i"
        Callback = $callback
        Link = $link
    })
}

# Pupupu adds four source map objects after the four generic geometry layers.
$mapCallPattern = '(?m)^\s*gGRCommonStruct\.pupupu\.map_gobj\[(?<index>\d+)\]\s*=\s*grPupupuMakeMapGObj\(&(?<dobj>[A-Za-z_][A-Za-z0-9_]*),\s*(?<mobj>0x0|&[A-Za-z_][A-Za-z0-9_]*),\s*(?<callback>[A-Za-z_][A-Za-z0-9_]*),\s*(?<link>\d+)\s*\);'
$mapCalls = @([regex]::Matches($pupupuText, $mapCallPattern))
Assert-Equal $mapCalls.Count 4 'grPupupuInitAll map callback count'
for ($i = 0; $i -lt 4; $i++) {
    Assert-Equal ([int]$mapCalls[$i].Groups['index'].Value) $i 'Pupupu map callback order'
    $callbackRows.Add([PSCustomObject]@{
        Owner = "map$i"
        Callback = $mapCalls[$i].Groups['callback'].Value
        Link = [int]$mapCalls[$i].Groups['link'].Value
    })
}

$actualCallbackSignature = ($callbackRows | ForEach-Object {
    "$($_.Owner):$($_.Callback)@$($_.Link)"
}) -join ';'
$expectedCallbackSignature = @(
    'layer0:grDisplayLayer0PriProcDisplay@4'
    'layer1:grDisplayLayer1PriProcDisplay@6'
    'layer2:grDisplayLayer2PriProcDisplay@13'
    'layer3:grDisplayLayer3PriProcDisplay@17'
    'map0:grDisplayLayer0PriProcDisplay@4'
    'map1:grDisplayLayer0PriProcDisplay@4'
    'map2:grDisplayLayer0PriProcDisplay@4'
    'map3:grDisplayLayer3PriProcDisplay@16'
) -join ';'
Assert-Equal $actualCallbackSignature $expectedCallbackSignature 'exact source callback partition'
Assert-Equal $callbackRows.Count 8 'source stage callback instance count'

$uniqueCallbacks = @($callbackRows.Callback | Sort-Object -Unique)
Assert-Equal $uniqueCallbacks.Count 4 'unique source stage callback function count'
foreach ($callback in $uniqueCallbacks) {
    $callbackPattern = "(?s)void\s+$([regex]::Escape($callback))\s*\([^)]*\)\s*\{.*?gcDrawDObjTreeForGObj\s*\(\s*ground_gobj\s*\)\s*;.*?\}"
    Assert-True ([regex]::IsMatch($grDisplayText, $callbackPattern)) "callback '$callback' no longer has the primary DObj-tree draw contract"
}

# Resolve layer DObjDesc arrays directly from MPGroundData.
$dobjStats = [System.Collections.Generic.List[object]]::new()
for ($i = 0; $i -lt 4; $i++) {
    $name = $layerMatches[$i].Groups['dobj'].Value
    Assert-True ($name -ne 'NULL') "layer$i has no DObjDesc source"
    $dobjStats.Add((Get-DObjArrayStats $file2Text $name "layer$i"))
}

# Resolve map DObjDesc arrays through the compatibility symbol offsets used by
# the live source import. This also catches drift between the source calls and
# the DS relocation declarations without reading a ROM.
$file3DObjHeaders = @([regex]::Matches(
    $file3Text,
    '(?m)^\s*DObjDesc\s+(?<name>[A-Za-z_][A-Za-z0-9_]*)\[(?<declared>\d+)\]\s*='
))
for ($i = 0; $i -lt 4; $i++) {
    $symbol = $mapCalls[$i].Groups['dobj'].Value
    $symbolMatch = [regex]::Match(
        $relocDataText,
        "X\($([regex]::Escape($symbol)),\s*0x(?<offset>[0-9A-Fa-f]+)\)"
    )
    Assert-True $symbolMatch.Success "relocation offset for '$symbol' not found"
    $offset = $symbolMatch.Groups['offset'].Value.TrimStart('0')
    if ($offset.Length -eq 0) { $offset = '0' }
    $matchingArrays = @($file3DObjHeaders | Where-Object {
        $arrayOffset = [regex]::Match($_.Groups['name'].Value, '_0x(?<offset>[0-9A-Fa-f]+)$')
        $arrayOffset.Success -and
            ($arrayOffset.Groups['offset'].Value.TrimStart('0') -ieq $offset)
    })
    Assert-Equal $matchingArrays.Count 1 "map$i DObjDesc array at relocation offset 0x$offset"
    $name = $matchingArrays[0].Groups['name'].Value
    $dobjStats.Add((Get-DObjArrayStats $file3Text $name "map$i"))
}

$dobjCount = [int](($dobjStats | Measure-Object -Property DObjs -Sum).Sum)
$dlReferenceCount = [int](($dobjStats | Measure-Object -Property DLReferences -Sum).Sum)
$allDLSymbols = @($dobjStats | ForEach-Object { $_.DLSymbols })
$uniqueDLSymbols = @($allDLSymbols | Sort-Object -Unique)
Assert-Equal $dobjCount 57 'live DObj count (sentinels excluded)'
Assert-Equal $dlReferenceCount 42 'DObj display-list reference count'
Assert-Equal $uniqueDLSymbols.Count 42 'unique DObj display-list identity count'

# Count materials by walking the selected pointer tables. The layer table is
# selected by MPGroundData; the two map tables are the only MObjSub** roots in
# StagePupupuFile3 and correspond to the two nonzero map constructor arguments.
$layerMObjCount = 0
$layerMObjRoots = @($layerMatches | Where-Object {
    $_.Groups['mobj'].Value -ne 'NULL'
})
Assert-Equal $layerMObjRoots.Count 1 'selected geometry layers with MObj tables'
$layerMObjRootName = $layerMObjRoots[0].Groups['mobj'].Value
$layerMObjRoot = Get-ArrayBlock $file2Text 'MObjSub\s+\*\*\s*' $layerMObjRootName
$layerMObjLists = @(Get-PointerEntries $layerMObjRoot 'selected layer MObj root' | Where-Object { $_ -ne 'NULL' })
foreach ($listName in $layerMObjLists) {
    $list = Get-ArrayBlock $file2Text 'MObjSub\s+\*\s*' $listName
    $definitions = @(Get-PointerEntries $list "selected layer MObj list '$listName'" | Where-Object { $_ -ne 'NULL' })
    foreach ($definition in $definitions) {
        $material = Get-ArrayBlock $file2Text 'MObjSub\s+' $definition
        Assert-Equal $material.Declared 1 "selected layer material '$definition' cardinality"
        $layerMObjCount += $material.Declared
    }
}
Assert-Equal $layerMObjCount 2 'selected geometry-layer MObj count'

$mapMObjArgs = @($mapCalls | Where-Object {
    $_.Groups['mobj'].Value -ne '0x0'
})
Assert-Equal $mapMObjArgs.Count 2 'Pupupu map constructors with MObj tables'
$mapMObjTableMatches = @([regex]::Matches(
    $file3Text,
    '(?ms)^\s*MObjSub\s+\*\*\s*(?<name>[A-Za-z_][A-Za-z0-9_]*)\[(?<declared>\d+)\]\s*=\s*\{\s*(?<body>.*?)^\s*\};'
))
Assert-Equal $mapMObjTableMatches.Count $mapMObjArgs.Count 'StagePupupuFile3 map MObj root-table count'
$mapMObjCount = 0
for ($i = 0; $i -lt $mapMObjTableMatches.Count; $i++) {
    $tableName = $mapMObjTableMatches[$i].Groups['name'].Value
    $table = Get-ArrayBlock $file3Text 'MObjSub\s+\*\*\s*' $tableName
    $listNames = @(Get-PointerEntries $table "map$i MObj root '$tableName'" | Where-Object { $_ -ne 'NULL' })
    Assert-Equal $listNames.Count 1 "map$i MObj root nonnull list count"
    foreach ($listName in $listNames) {
        $list = Get-ArrayBlock $file3Text 'MObjSub\s+\*\s*' $listName
        $definitions = @(Get-PointerEntries $list "map$i MObj list '$listName'" | Where-Object { $_ -ne 'NULL' })
        foreach ($definition in $definitions) {
            $material = Get-ArrayBlock $file3Text 'MObjSub\s+' $definition
            Assert-Equal $material.Declared 1 "map$i material '$definition' cardinality"
            $mapMObjCount += $material.Declared
        }
    }
}
Assert-Equal $mapMObjCount 2 'Pupupu map MObj count'
$mobjCount = $layerMObjCount + $mapMObjCount
Assert-Equal $mobjCount 4 'complete stage MObj count'

# Enumerate source-visible variant axes. These are blockers to treating the
# idle structure as an invariant; they deliberately do not produce AOT code.
$windStatusCount = Get-EnumMemberCount $pupupuText 'grPupupuWhispyWindStatus' 'nGRPupupuWhispyWindStatus'
$mouthStatusCount = Get-EnumMemberCount $pupupuText 'grPupupuWhispyMouthStatus' 'nGRPupupuWhispyMouthStatus'
$eyesStatusCount = Get-EnumMemberCount $pupupuText 'grPupupuWhispyEyesStatus' 'nGRPupupuWhispyEyesStatus'
$flowerStatusCount = Get-EnumMemberCount $pupupuText 'grPupupuFlowerStatus' 'nGRPupupuFlowerStatus'
Assert-Equal $windStatusCount 6 'Whispy wind status variants'
Assert-Equal $mouthStatusCount 4 'Whispy mouth animation variants'
Assert-Equal $eyesStatusCount 2 'Whispy eyes animation variants'
Assert-Equal $flowerStatusCount 6 'flower status variants'

$updateSlice = Get-FunctionSlice $pupupuText 'void grPupupuUpdateGObjAnims' '// 0x80106490'
Assert-Equal ([regex]::Matches($updateSlice, '\bgcAddAnimAll\s*\(').Count) 2 'runtime transform/material animation attachment sites'
Assert-Equal ([regex]::Matches($updateSlice, '\bgcAddAnimJointAll\s*\(').Count) 2 'runtime texture-animation attachment sites'
Assert-Equal ([regex]::Matches($updateSlice, '\bgcPlayAnimAll\s*\(').Count) 4 'runtime map animation restart sites'

$layerMatAnimRoots = @($layerMatches | Where-Object {
    $_.Groups['matanim'].Value -ne 'NULL'
})
Assert-Equal $layerMatAnimRoots.Count 1 'selected geometry layers with material animation'
$matAnimRootName = $layerMatAnimRoots[0].Groups['matanim'].Value
$matAnimRoot = Get-ArrayBlock $file2Text 'AObjEvent32\s+\*\*\s*' $matAnimRootName
$matAnimLists = @(Get-PointerEntries $matAnimRoot 'selected layer material-animation root' | Where-Object { $_ -ne 'NULL' })
$matAnimScripts = [System.Collections.Generic.List[string]]::new()
foreach ($listName in $matAnimLists) {
    $list = Get-ArrayBlock $file2Text 'AObjEvent32\s+\*\s*' $listName
    foreach ($scriptName in @(Get-PointerEntries $list "selected material-animation list '$listName'" | Where-Object { $_ -ne 'NULL' })) {
        $matAnimScripts.Add($scriptName)
    }
}
Assert-Equal $matAnimScripts.Count 2 'selected material-animation script count'
$materialFlags = [System.Collections.Generic.List[string]]::new()
foreach ($scriptName in $matAnimScripts) {
    $script = Get-ArrayBlock $file2Text 'u32\s+' $scriptName
    foreach ($flag in [regex]::Matches($script.Body, 'AOBJ_MATFLAG_[A-Z0-9_]+')) {
        $materialFlags.Add($flag.Value)
    }
}
$materialFlagSet = @($materialFlags | Sort-Object -Unique)
$expectedMaterialFlagSet = @(
    'AOBJ_MATFLAG_LFRAC'
    'AOBJ_MATFLAG_SCRU'
    'AOBJ_MATFLAG_SCRV'
    'AOBJ_MATFLAG_TEXID'
    'AOBJ_MATFLAG_TEXIDNEXT'
    'AOBJ_MATFLAG_TRAU'
)
Assert-Equal ($materialFlagSet -join ',') ($expectedMaterialFlagSet -join ',') 'selected water material dynamic-track set'

Assert-True ($objAnimText -match 'dobj->flags\s*=\s*dobj->anim_joint\.event32->command\.flags') 'BattleShip DObj animation flag mutation path missing'
Assert-True ($objDisplayText -match 'DOBJ_FLAG_HIDDEN') 'BattleShip display-time DObj flag predicate missing'
Assert-True ($rendererText -match 'ndsRendererAdapterStageDObjDrawable') 'DS stage drawable predicate missing'
Assert-True ($rendererText -match 'DOBJ_FLAG_HIDDEN') 'DS stage hidden-DObj predicate missing'
Assert-True ($rendererText -match 'DOBJ_FLAG_NOTEXTURE') 'DS stage no-texture predicate missing'
Assert-True ($rendererText -match 'state->segment_e_base\s*=\s*table') 'DS live segment-E material construction missing'
Assert-True ($rendererText -match 'MOBJ_FLAG_FRAC') 'DS live MObj fraction handling missing'

$selectedSetFlagsSites = [regex]::Matches(
    $file2Text + "`n" + $file3Text,
    '\baobjEvent32SetFlags\s*\('
).Count

$dobjBreakdown = ($dobjStats | ForEach-Object {
    "$($_.Owner)=$($_.DObjs)/$($_.DLReferences)"
}) -join ' '
$callbackBreakdown = ($callbackRows | ForEach-Object {
    "$($_.Owner):$($_.Callback)@$($_.Link)"
}) -join ' '

Write-Output ("M3 stage AOT structural falsifier passed: callbacks={0} DObjs={1} lists={2} MObjs={3}." -f $callbackRows.Count, $dobjCount, $dlReferenceCount, $mobjCount)
Write-Output ("M3 callback partition: {0}" -f $callbackBreakdown)
Write-Output ("M3 DObj/list partition: {0}" -f $dobjBreakdown)
Write-Output ("M3 dynamic axes: Whispy wind={0}, eyes={1}x2 LR, mouth={2}x2 LR, flower={3}; map reattachments=2 transform/material+2 texture; material tracks={4}." -f $windStatusCount, $eyesStatusCount, $mouthStatusCount, $flowerStatusCount, ($materialFlagSet -join '|'))
Write-Output ("M3 runtime-only guards: selected asset SetFlags sites={0}, but live DObj flags, animation state, MObj/FRAC, segment-E state, callback/DL identity, effects, and weapons remain unobserved by this host check." -f $selectedSetFlagsSites)
Write-Output 'M3_AOT_READY=UNPROVED (structure passes; device/profile-2 variant, parity, timing, and screenshot gates are still required).'
