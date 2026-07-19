[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$Control,
    [Parameter(Mandatory = $true)][string]$Candidate
)

$ErrorActionPreference = 'Stop'

function Read-Task26Artifact {
    param([string]$Path, [int]$ExpectedSelector)

    $resolved = (Resolve-Path -LiteralPath $Path).Path
    $artifact = Get-Content -LiteralPath $resolved -Raw | ConvertFrom-Json
    if ([int]$artifact.nativeStageGeneratedSegment0Enable -ne $ExpectedSelector) {
        throw "$resolved has selector $($artifact.nativeStageGeneratedSegment0Enable), expected $ExpectedSelector."
    }
    if ([int]$artifact.identity.rendererBenchmarkMode -ne 2) {
        throw "$resolved is not CPU_PREP_NO_GX evidence."
    }
    return $artifact
}

function Assert-ExactJson {
    param([object]$Expected, [object]$Actual, [string]$Label)

    $expectedJson = $Expected | ConvertTo-Json -Compress -Depth 12
    $actualJson = $Actual | ConvertTo-Json -Compress -Depth 12
    if ($expectedJson -cne $actualJson) {
        throw "Task 26 $Label differs between selector 0 and selector 1."
    }
}

$controlArtifact = Read-Task26Artifact $Control 0
$candidateArtifact = Read-Task26Artifact $Candidate 1
$controlSamples = $controlArtifact.samples
$candidateSamples = $candidateArtifact.samples

if ($controlSamples.m3GeneratedSegment0TraceWords.Count -eq 0 -or
    $controlSamples.m3GeneratedSegment0TraceRuns.Count -ne 26 -or
    $candidateSamples.m3GeneratedSegment0TraceWords.Count -eq 0 -or
    $candidateSamples.m3GeneratedSegment0TraceRuns.Count -ne 26) {
    throw 'Task 26 artifacts do not contain a complete typed segment-0 trace.'
}

Assert-ExactJson $controlSamples.m3GeneratedSegment0TraceSummary `
    $candidateSamples.m3GeneratedSegment0TraceSummary 'trace summary'
Assert-ExactJson $controlSamples.m3GeneratedSegment0TraceWords `
    $candidateSamples.m3GeneratedSegment0TraceWords 'typed GX/state words'
Assert-ExactJson $controlSamples.m3GeneratedSegment0TraceRuns `
    $candidateSamples.m3GeneratedSegment0TraceRuns `
    'normalized prepared-descriptor run checkpoints'

$controlSegmentHashes = @($controlSamples.m3GeneratedSegment0Gx |
    ForEach-Object { , @($_[0], $_[4], $_[5], $_[6], $_[7]) })
$candidateSegmentHashes = @($candidateSamples.m3GeneratedSegment0Gx |
    ForEach-Object { , @($_[0], $_[4], $_[5], $_[6], $_[7]) })
Assert-ExactJson $controlSegmentHashes $candidateSegmentHashes `
    'per-frame segment word counts, hashes, and arm-fault state'

$wordCount = $controlSamples.m3GeneratedSegment0TraceWords.Count
$frameCount = $controlSegmentHashes.Count
Write-Output (
    "TASK26_SEGMENT0_TRACE_EXACT frames=$frameCount words=$wordCount " +
    'runs=26 mismatches=0 arm_faults=0'
)
