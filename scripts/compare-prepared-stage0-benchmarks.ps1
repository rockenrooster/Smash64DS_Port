param(
    [ValidateSet(1,2)][int]$RendererProfileLevel = 2,
    [Parameter(Mandatory=$true)][Alias('GenericJson')]
        [string]$GenericAJson,
    [Parameter(Mandatory=$true)][string]$PreparedJson,
    [string]$GenericBJson = '',
    [ValidateRange(0,1000000000)][int64]$MinDrawImprovementTicks = 50000,
    [ValidateRange(0,10000)][int64]$MinOwnerImprovementBasisPoints = 2500,
    [ValidateRange(0,10000)][int64]$MaxP95RegressionBasisPoints = 100,
    [ValidateRange(0,10000)][int64]$MaxBaselineDriftBasisPoints = 500
)
$ErrorActionPreference = 'Stop'

function Assert-Condition {
    param([bool]$Condition, [string]$Message)
    if (-not $Condition) { throw $Message }
}

function Read-BenchmarkResult {
    param([Parameter(Mandatory=$true)][string]$Path)

    $resolved = (Resolve-Path -LiteralPath $Path).Path
    $result = Get-Content -LiteralPath $resolved -Raw | ConvertFrom-Json
    Assert-Condition ($result.schema -eq 1 -and
        $result.kind -eq 'smash64ds-renderer-benchmark') `
        "Unsupported renderer benchmark JSON schema: $resolved"
    Assert-Condition ($null -ne $result.identity -and
        $null -ne $result.samples -and $null -ne $result.terminal) `
        "Renderer benchmark JSON is incomplete: $resolved"
    return $result
}

function Get-Rows {
    param([object]$Value)
    if ($null -eq $Value) { return @() }
    return @($Value)
}

function Get-RowValues {
    param([object]$Row)
    return @($Row | ForEach-Object { [int64]$_ })
}

function Compare-SelectedRows {
    param(
        [object]$Left,
        [object]$Right,
        [int[]]$FieldIndices,
        [string]$Label
    )

    $leftRows = @(Get-Rows $Left)
    $rightRows = @(Get-Rows $Right)
    Assert-Condition ($leftRows.Count -eq $rightRows.Count) `
        "$Label row count mismatch: $($leftRows.Count) != $($rightRows.Count)."
    for ($rowIndex = 0; $rowIndex -lt $leftRows.Count; $rowIndex++) {
        $leftValues = @(Get-RowValues $leftRows[$rowIndex])
        $rightValues = @(Get-RowValues $rightRows[$rowIndex])
        foreach ($fieldIndex in $FieldIndices) {
            Assert-Condition ($fieldIndex -lt $leftValues.Count -and
                $fieldIndex -lt $rightValues.Count) `
                "$Label row $rowIndex lacks field $fieldIndex."
            Assert-Condition ($leftValues[$fieldIndex] -eq
                $rightValues[$fieldIndex]) `
                "$Label mismatch at row $rowIndex field $fieldIndex`: $($leftValues[$fieldIndex]) != $($rightValues[$fieldIndex])."
        }
    }
}

function Compare-ValueArray {
    param([object]$Left, [object]$Right, [string]$Label)

    $leftValues = @(Get-RowValues $Left)
    $rightValues = @(Get-RowValues $Right)
    Assert-Condition ($leftValues.Count -eq $rightValues.Count) `
        "$Label field count mismatch: $($leftValues.Count) != $($rightValues.Count)."
    for ($index = 0; $index -lt $leftValues.Count; $index++) {
        Assert-Condition ($leftValues[$index] -eq $rightValues[$index]) `
            "$Label mismatch at field $index`: $($leftValues[$index]) != $($rightValues[$index])."
    }
}

function Compare-SelectedValues {
    param(
        [object]$Left,
        [object]$Right,
        [int[]]$FieldIndices,
        [string]$Label
    )

    $leftValues = @(Get-RowValues $Left)
    $rightValues = @(Get-RowValues $Right)
    foreach ($fieldIndex in $FieldIndices) {
        Assert-Condition ($fieldIndex -lt $leftValues.Count -and
            $fieldIndex -lt $rightValues.Count) `
            "$Label lacks field $fieldIndex."
        Assert-Condition ($leftValues[$fieldIndex] -eq
            $rightValues[$fieldIndex]) `
            "$Label mismatch at field $fieldIndex`: $($leftValues[$fieldIndex]) != $($rightValues[$fieldIndex])."
    }
}

function Assert-CompatibleIdentity {
    param([object]$Reference, [object]$Other, [string]$Label)

    Assert-Condition ([int]$Reference.identity.rendererProfile -eq
        $RendererProfileLevel -and
        [int]$Other.identity.rendererProfile -eq $RendererProfileLevel) `
        "$Label does not use renderer profile $RendererProfileLevel."
    Assert-Condition ($Reference.identity.harness.name -eq
        $Other.identity.harness.name -and
        [int]$Reference.identity.harness.id -eq
        [int]$Other.identity.harness.id) `
        "$Label harness identity differs."
    foreach ($flagSet in @('common', 'renderer', 'scene')) {
        Assert-Condition ($Reference.identity.effectiveCFlags.$flagSet -ceq
            $Other.identity.effectiveCFlags.$flagSet) `
            "$Label effective $flagSet C flags differ."
    }
    Assert-Condition ($Reference.identity.melonDS.executableSha256 -eq
        $Other.identity.melonDS.executableSha256 -and
        $Reference.identity.melonDS.version -eq
        $Other.identity.melonDS.version) `
        "$Label melonDS executable identity differs."
    Assert-Condition (-not [bool]$Reference.identity.melonDS.settings.limitFPS -and
        -not [bool]$Other.identity.melonDS.settings.limitFPS -and
        -not [bool]$Reference.identity.melonDS.settings.jit -and
        -not [bool]$Other.identity.melonDS.settings.jit) `
        "$Label must use uncapped interpreter-mode melonDS settings."
}

function Assert-MatchedWindow {
    param([object]$Reference, [object]$Other, [string]$Label)

    Compare-SelectedRows $Reference.samples.renderer `
        $Other.samples.renderer @(1) "$Label renderer frame"
    Compare-SelectedRows $Reference.samples.coarse `
        $Other.samples.coarse @(0, 23) "$Label coarse frame/logic"
    Compare-SelectedRows $Reference.samples.gxBoundary `
        $Other.samples.gxBoundary @(0) "$Label GX frame"
    Compare-SelectedRows $Reference.samples.stageLayer0 `
        $Other.samples.stageLayer0 @(0) "$Label layer-0 frame"
}

function Assert-PreparedExecution {
    param(
        [object]$Result,
        [ValidateSet(1,2)][int]$ExpectedMode,
        [string]$Label
    )

    $values = @(Get-RowValues $Result.terminal.preparedStage0Execution)
    Assert-Condition ($values.Count -eq 9) `
        "$Label prepared execution marker has $($values.Count) fields."
    Assert-Condition ($values[0] -eq $ExpectedMode) `
        "$Label prepared execution mode is $($values[0]), expected $ExpectedMode."
    Assert-Condition ($values[1] -eq $values[2] -and
        $values[2] -eq $values[3] -and
        $values[3] -eq $values[4] -and
        $values[5] -eq (20 * $values[4]) -and
        $values[6] -eq 0 -and $values[7] -eq 0 -and
        $values[8] -eq 0) `
        "$Label prepared whole-owner preflight/execution accounting failed."
    if ($ExpectedMode -eq 2) {
        Assert-Condition ($values[4] -gt 0) `
            "$Label did not complete a prepared stage layer-0 owner."
    }
}

function Compare-ExactNonTimingResult {
    param([object]$Reference, [object]$Other, [string]$Label)

    Compare-SelectedRows $Reference.samples.renderer `
        $Other.samples.renderer @(0, 1, 11, 12, 13, 14) `
        "$Label renderer counters/uploads"
    Compare-SelectedRows $Reference.samples.gxBoundary `
        $Other.samples.gxBoundary @(0, 1, 2, 3, 4, 5) `
        "$Label GX boundary"
    Compare-SelectedValues $Reference.terminal.renderProfile `
        $Other.terminal.renderProfile @(0, 12, 13, 14, 15, 16, 17) `
        "$Label terminal renderer counters"

    $terminalExact = @(
        'platformHardware', 'stageHardware', 'stageCarry',
        'stageHardwareFighter', 'fighterDisplayContract', 'renderBatch',
        'renderTopology', 'renderCi4Lut', 'renderCi4Map',
        'renderTexHash', 'renderOracle', 'renderMatrix',
        'renderAdapterCache', 'renderRawMatrix', 'renderSubmit',
        'renderHardwareDivide', 'renderLazy', 'renderVertex', 'renderDepth',
        'renderClip', 'renderTexture', 'renderTexel1', 'renderTexUse',
        'renderTexFmt', 'renderTexLane', 'renderCombine', 'renderLight',
        'preparedStage0'
    )
    foreach ($marker in $terminalExact) {
        Compare-ValueArray $Reference.terminal.$marker `
            $Other.terminal.$marker "$Label terminal $marker"
    }
    Assert-Condition ($Reference.derived.uploadSequenceSha256 -ceq
        $Other.derived.uploadSequenceSha256) `
        "$Label texture upload sequence differs."
}

function Compare-Profile2SemanticResult {
    param([object]$Reference, [object]$Other, [string]$Label)

    Compare-ExactNonTimingResult $Reference $Other $Label
    foreach ($owner in @('stage', 'mario', 'fox')) {
        Compare-SelectedRows $Reference.samples.owners.$owner `
            $Other.samples.owners.$owner (@(0, 1) + @(3..36)) `
            "$Label $owner owner census/state/cache/signatures"
    }
    Compare-SelectedRows $Reference.samples.semantic `
        $Other.samples.semantic @(0..37) "$Label semantic trace"
}

function Get-SampleColumn {
    param([object]$Rows, [int]$FieldIndex, [string]$Label)

    $values = @()
    $rowIndex = 0
    foreach ($row in @(Get-Rows $Rows)) {
        $rowValues = @(Get-RowValues $row)
        Assert-Condition ($FieldIndex -lt $rowValues.Count) `
            "$Label row $rowIndex lacks field $FieldIndex."
        $values += [int64]$rowValues[$FieldIndex]
        $rowIndex++
    }
    Assert-Condition ($values.Count -gt 0) "$Label has no samples."
    return [int64[]]$values
}

function Get-Median {
    param([int64[]]$Values)
    $ordered = @($Values | Sort-Object)
    $middle = [int]($ordered.Count / 2)
    if (($ordered.Count % 2) -eq 0) {
        return [int64](($ordered[$middle - 1] + $ordered[$middle]) / 2)
    }
    return [int64]$ordered[$middle]
}

function Get-Percentile95 {
    param([int64[]]$Values)
    $ordered = @($Values | Sort-Object)
    $index = [Math]::Max(
        0, [Math]::Ceiling($ordered.Count * 0.95) - 1)
    return [int64]$ordered[$index]
}

function Get-DriftBasisPoints {
    param([int64]$A, [int64]$B)
    $denominator = [Math]::Min($A, $B)
    if ($denominator -le 0) { return [int64]10000 }
    return [int64](([Math]::Abs($A - $B) * 10000) / $denominator)
}

function Get-ImprovementBasisPoints {
    param([int64]$Baseline, [int64]$Candidate)
    if ($Baseline -le 0) { return [int64]0 }
    return [int64]((($Baseline - $Candidate) * 10000) / $Baseline)
}

$genericA = Read-BenchmarkResult $GenericAJson
$prepared = Read-BenchmarkResult $PreparedJson
Assert-CompatibleIdentity $genericA $prepared 'generic-A/prepared'
Assert-MatchedWindow $genericA $prepared 'generic-A/prepared'
Assert-PreparedExecution $genericA 1 'generic-A'
Assert-PreparedExecution $prepared 2 'prepared'

if ($RendererProfileLevel -eq 2) {
    Compare-Profile2SemanticResult $genericA $prepared 'generic-A/prepared'
    $rendererRows = @(Get-Rows $genericA.samples.renderer)
    $coarseRows = @(Get-Rows $genericA.samples.coarse)
    $firstFrame = [int64](Get-RowValues $rendererRows[0])[1]
    $lastFrame = [int64](Get-RowValues $rendererRows[-1])[1]
    $firstLogic = [int64](Get-RowValues $coarseRows[0])[23]
    $lastLogic = [int64](Get-RowValues $coarseRows[-1])[23]
    Write-Output (
        "Prepared stage0 profile-2 A/B gate passed: samples=$($rendererRows.Count) " +
        "frames=$firstFrame..$lastFrame logic=$firstLogic..$lastLogic " +
        "uploadSequenceSha256=$($genericA.derived.uploadSequenceSha256)")
    exit 0
}

Assert-Condition (-not [string]::IsNullOrWhiteSpace($GenericBJson)) `
    'Renderer profile 1 requires GenericBJson for an A/B/A timing gate.'
$genericB = Read-BenchmarkResult $GenericBJson
Assert-CompatibleIdentity $genericA $genericB 'generic-A/generic-B'
Assert-MatchedWindow $genericA $genericB 'generic-A/generic-B'
Assert-PreparedExecution $genericB 1 'generic-B'
Compare-ExactNonTimingResult $genericA $prepared 'generic-A/prepared'
Compare-ExactNonTimingResult $genericA $genericB 'generic-A/generic-B'

$drawA = Get-SampleColumn $genericA.samples.coarse 9 'generic-A draw'
$drawPrepared = Get-SampleColumn $prepared.samples.coarse 9 'prepared draw'
$drawB = Get-SampleColumn $genericB.samples.coarse 9 'generic-B draw'
$ownerA = Get-SampleColumn $genericA.samples.stageLayer0 1 `
    'generic-A stage layer-0 owner'
$ownerPrepared = Get-SampleColumn $prepared.samples.stageLayer0 1 `
    'prepared stage layer-0 owner'
$ownerB = Get-SampleColumn $genericB.samples.stageLayer0 1 `
    'generic-B stage layer-0 owner'

$drawMedianA = Get-Median $drawA
$drawMedianPrepared = Get-Median $drawPrepared
$drawMedianB = Get-Median $drawB
$drawP95A = Get-Percentile95 $drawA
$drawP95Prepared = Get-Percentile95 $drawPrepared
$drawP95B = Get-Percentile95 $drawB
$ownerMedianA = Get-Median $ownerA
$ownerMedianPrepared = Get-Median $ownerPrepared
$ownerMedianB = Get-Median $ownerB
$ownerP95A = Get-Percentile95 $ownerA
$ownerP95Prepared = Get-Percentile95 $ownerPrepared
$ownerP95B = Get-Percentile95 $ownerB

$drawMedianDrift = Get-DriftBasisPoints $drawMedianA $drawMedianB
$drawP95Drift = Get-DriftBasisPoints $drawP95A $drawP95B
$ownerMedianDrift = Get-DriftBasisPoints $ownerMedianA $ownerMedianB
$ownerP95Drift = Get-DriftBasisPoints $ownerP95A $ownerP95B
Assert-Condition ($drawMedianDrift -le $MaxBaselineDriftBasisPoints -and
    $drawP95Drift -le $MaxBaselineDriftBasisPoints -and
    $ownerMedianDrift -le $MaxBaselineDriftBasisPoints -and
    $ownerP95Drift -le $MaxBaselineDriftBasisPoints) `
    "Generic A/B/A drift exceeded $MaxBaselineDriftBasisPoints bp: draw median/p95=$drawMedianDrift/$drawP95Drift owner median/p95=$ownerMedianDrift/$ownerP95Drift."

$drawBaselineMedian = [Math]::Min($drawMedianA, $drawMedianB)
$drawBaselineP95 = [Math]::Min($drawP95A, $drawP95B)
$ownerBaselineMedian = [Math]::Min($ownerMedianA, $ownerMedianB)
$ownerBaselineP95 = [Math]::Min($ownerP95A, $ownerP95B)
$drawImprovement = $drawBaselineMedian - $drawMedianPrepared
$ownerImprovementBasisPoints = Get-ImprovementBasisPoints `
    $ownerBaselineMedian $ownerMedianPrepared
Assert-Condition ($drawImprovement -ge $MinDrawImprovementTicks -or
    $ownerImprovementBasisPoints -ge $MinOwnerImprovementBasisPoints) `
    "Prepared stage0 missed both median gates: draw improvement=$drawImprovement ticks (need $MinDrawImprovementTicks) owner improvement=$ownerImprovementBasisPoints bp (need $MinOwnerImprovementBasisPoints)."
Assert-Condition (($drawP95Prepared * 10000) -le
    ($drawBaselineP95 * (10000 + $MaxP95RegressionBasisPoints))) `
    "Prepared draw P95 regressed beyond $MaxP95RegressionBasisPoints bp: $drawP95Prepared vs baseline $drawBaselineP95."
Assert-Condition (($ownerP95Prepared * 10000) -le
    ($ownerBaselineP95 * (10000 + $MaxP95RegressionBasisPoints))) `
    "Prepared stage layer-0 P95 regressed beyond $MaxP95RegressionBasisPoints bp: $ownerP95Prepared vs baseline $ownerBaselineP95."

Write-Output (
    'Prepared stage0 profile-1 A/B/A timing gate passed: ' +
    "draw median A/B/A=$drawMedianA/$drawMedianPrepared/$drawMedianB " +
    "p95=$drawP95A/$drawP95Prepared/$drawP95B improvement=$drawImprovement; " +
    "owner median A/B/A=$ownerMedianA/$ownerMedianPrepared/$ownerMedianB " +
    "p95=$ownerP95A/$ownerP95Prepared/$ownerP95B " +
    "improvementBp=$ownerImprovementBasisPoints")
