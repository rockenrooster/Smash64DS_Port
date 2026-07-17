param(
    [ValidateRange(1,8)][int]$FastRunMode = 3,
    [int]$RunnerSlot = -1,
    [int]$DelaySeconds = 5,
    [ValidateRange(8,256)][int]$RendererBenchmarkSamples = 8,
    [ValidateRange(5,600)][int]$RendererBenchmarkTimeoutSeconds = 30,
    [switch]$NoBuild
)

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$scratch = Join-Path $root 'logs\fast-raw-compare'
$genericPath = Join-Path $scratch 'generic-profile2.json'
$fastPath = Join-Path $scratch "fast-mode$FastRunMode-profile2.json"
$benchmark = Join-Path $PSScriptRoot 'benchmark-renderer-fast-raw.ps1'

& $benchmark `
    -FastRunMode 0 `
    -RendererProfileLevel 2 `
    -RunnerSlot $RunnerSlot `
    -DelaySeconds $DelaySeconds `
    -RendererBenchmarkSamples $RendererBenchmarkSamples `
    -RendererBenchmarkTimeoutSeconds $RendererBenchmarkTimeoutSeconds `
    -RendererBenchmarkExportPath $genericPath `
    -NoBuild:$NoBuild
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

& $benchmark `
    -FastRunMode $FastRunMode `
    -RendererProfileLevel 2 `
    -RunnerSlot $RunnerSlot `
    -DelaySeconds $DelaySeconds `
    -RendererBenchmarkSamples $RendererBenchmarkSamples `
    -RendererBenchmarkTimeoutSeconds $RendererBenchmarkTimeoutSeconds `
    -RendererBenchmarkExportPath $fastPath `
    -NoBuild
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

$generic = Get-Content -LiteralPath $genericPath -Raw | ConvertFrom-Json
$fast = Get-Content -LiteralPath $fastPath -Raw | ConvertFrom-Json

function Assert-EqualJson {
    param([object]$Expected, [object]$Actual, [string]$Label)
    $expectedJson = $Expected | ConvertTo-Json -Compress -Depth 8
    $actualJson = $Actual | ConvertTo-Json -Compress -Depth 8
    if ($expectedJson -cne $actualJson) {
        throw "$Label differs between generic and fast mode $FastRunMode."
    }
}

function Convert-SampleRow {
    param([object]$Row)
    if ($Row -is [string]) {
        return @($Row.Trim() -split '\s+' | ForEach-Object { [int64]$_ })
    }
    return @($Row | ForEach-Object { [int64]$_ })
}

function Select-SampleFields {
    param(
        [System.Collections.IEnumerable]$Rows,
        [int[]]$Indices,
        [string]$Label
    )
    $maximumIndex = ($Indices | Measure-Object -Maximum).Maximum
    return @($Rows | ForEach-Object {
        $row = Convert-SampleRow $_
        if ($row.Count -le $maximumIndex) {
            throw "$Label row has $($row.Count) fields; expected index $maximumIndex."
        }
        ,@($Indices | ForEach-Object { $row[$_] })
    })
}

if ($generic.samples.semantic.Count -ne $RendererBenchmarkSamples -or
    $fast.samples.semantic.Count -ne $RendererBenchmarkSamples) {
    throw 'Profile-2 semantic trace export did not contain the requested frame count.'
}

Assert-EqualJson $generic.samples.semantic $fast.samples.semantic `
    'Dual semantic trace hashes, counts, and provenance'

for ($owner = 0; $owner -lt 3; $owner++) {
    $genericOwner = Select-SampleFields $generic.samples.owners[$owner] `
        (@(0, 1) + @(3..36)) "Generic owner $owner"
    $fastOwner = Select-SampleFields $fast.samples.owners[$owner] `
        (@(0, 1) + @(3..36)) "Fast owner $owner"
    Assert-EqualJson $genericOwner $fastOwner `
        "Owner $owner census, entry/exit state, cache, resolver, and signature trace"
}

$genericRenderContract = Select-SampleFields $generic.samples.renderer `
    (@(0, 1) + @(11..14)) 'Generic renderer contract'
$fastRenderContract = Select-SampleFields $fast.samples.renderer `
    (@(0, 1) + @(11..14)) 'Fast renderer contract'
Assert-EqualJson $genericRenderContract $fastRenderContract `
    'Frame/profile geometry, oracle, and texture-upload sequence'

$fastTriangles = @($fast.samples.fastRaw | ForEach-Object {
    (Convert-SampleRow $_)[3]
})
if (($fastTriangles | Measure-Object -Minimum).Minimum -le 0) {
    throw "Fast mode $FastRunMode did not execute fast triangles in every frame."
}

Write-Output (
    "Renderer fast raw semantic comparison passed: mode=$FastRunMode " +
    "frames=$RendererBenchmarkSamples semanticMismatches=0 ownerMismatches=0 " +
    "geometryMismatches=0 triangles=$(($fastTriangles | Measure-Object -Minimum).Minimum).." +
    "$(($fastTriangles | Measure-Object -Maximum).Maximum)")
