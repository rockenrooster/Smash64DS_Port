param(
    [string[]]$ChangedFiles
)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
function Normalize-ChangedPath {
    param([string]$Path)
    $normalized = "$Path".Trim().Trim('"')
    $normalized = $normalized -replace '\\', '/'
    $normalized = $normalized -replace '^\./', ''
    return $normalized.ToLowerInvariant()
}
function Get-ExplicitChangedFiles {
    param([string[]]$InputFiles)
    return @(
        $InputFiles |
            ForEach-Object { "$_" -split ',' } |
            ForEach-Object { "$_".Trim() } |
            Where-Object { -not [string]::IsNullOrWhiteSpace($_) }
    )
}
function Get-GitChangedFiles {
    try {
        $statusLines = @(& git -C $root status --porcelain=v1)
        if ($LASTEXITCODE -ne 0) { return @() }
    } catch {
        return @()
    }
    $paths = @()
    foreach ($line in $statusLines) {
        if ([string]::IsNullOrWhiteSpace($line) -or $line.Length -lt 4) { continue }
        $pathText = $line.Substring(3).Trim()
        if ($pathText -match ' -> ') {
            $pathText = @($pathText -split ' -> ')[-1]
        }
        $paths += $pathText
    }
    return @($paths)
}
function Test-AnyPath {
    param(
        [string[]]$Paths,
        [scriptblock]$Predicate
    )
    foreach ($path in $Paths) {
        if (& $Predicate $path) { return $true }
    }
    return $false
}
function Test-AllPaths {
    param(
        [string[]]$Paths,
        [scriptblock]$Predicate
    )
    if ($Paths.Count -eq 0) { return $false }
    foreach ($path in $Paths) {
        if (-not (& $Predicate $path)) { return $false }
    }
    return $true
}
function New-Recommendation {
    param(
        [string]$Name,
        [string]$Reason,
        [string[]]$Commands,
        [string]$Profile = 'none'
    )
    [PSCustomObject]@{
        Recommendation = $Name
        Profile = $Profile
        Reason = $Reason
        Commands = $Commands
    }
}
$rawChangedFiles = @(Get-ExplicitChangedFiles -InputFiles $ChangedFiles)
if ($rawChangedFiles.Count -eq 0) {
    $rawChangedFiles = @(Get-GitChangedFiles)
}
$paths = @($rawChangedFiles | ForEach-Object { Normalize-ChangedPath $_ } | Where-Object { $_ } | Select-Object -Unique)
if ($paths.Count -eq 0) {
    Write-Output 'No changed files were provided or detected by git.'
    Write-Output 'Recommendation: no verifier run is required.'
    exit 0
}
$isDocsPath = {
    param($path)
    $path -like 'docs/*' -or
    $path -like 'tasks/parallel/*.md' -or
    $path -eq 'agents.md' -or
    $path.EndsWith('.md')
}
$isSnapshotHygienePath = {
    param($path)
    $path -eq 'scripts/clean-generated.ps1' -or
    $path -eq 'scripts/check-snapshot-build-context.ps1' -or
    $path -like 'scripts/new-smash64dssnapshot*.ps1' -or
    $path -like 'docs/*snapshot*' -or
    $path -like 'docs/*handoff*'
}
$isVerifierPlumbingPath = {
    param($path)
    $path -eq 'makefile' -or
    $path -eq 'scripts/lib/harness-registry.ps1' -or
    $path -eq 'scripts/check-harness-registry.ps1' -or
    $path -like 'scripts/verify-*.ps1' -or
    $path -eq 'include/nds/nds_scene_harness.h'
}
$isAbiHeaderPath = {
    param($path)
    $path -like 'include/*' -or
    $path -like 'src/*.h' -or
    $path -like 'src/*/*.h' -or
    $path -like 'src/*/*/*.h'
}
$isGbiRendererPath = {
    param($path)
    $path -match '(^|/)(gbi|f3dex|f3dex2)(/|_|-|\.|$)' -or
    $path -match '(renderer|render|display|gcdrawall|dl-draw|dl_draw|dl-scan|dl_scan|dl-execute|dl_execute)' -or
    $path -eq 'scripts/check-gbi-decode-fixtures.ps1'
}
$isCurrentBoundaryHarnessPath = {
    param($path)
    $path -eq 'src/port/scene_harness.c'
}
$isSharedRuntimeBackendPath = {
    param($path)
    $path -eq 'src/port/scene_backend.c' -or
    $path -like 'src/port/scene_backend_*' -or
    $path -like 'src/import/*' -or
    $path -like 'src/nds/*' -or
    $path -match '(taskman|task|object|gobj|controller|reloc|collision|mpprocess|mpcommon|ftmain|display|backend)'
}
$allDocs = Test-AllPaths -Paths $paths -Predicate $isDocsPath
$allSnapshotHygiene = Test-AllPaths -Paths $paths -Predicate $isSnapshotHygienePath
$hasVerifierPlumbing = Test-AnyPath -Paths $paths -Predicate $isVerifierPlumbingPath
$hasAbiHeader = Test-AnyPath -Paths $paths -Predicate $isAbiHeaderPath
$hasGbiRenderer = Test-AnyPath -Paths $paths -Predicate $isGbiRendererPath
$hasCurrentBoundaryHarness = Test-AnyPath -Paths $paths -Predicate $isCurrentBoundaryHarnessPath
$hasSharedRuntimeBackend = Test-AnyPath -Paths $paths -Predicate $isSharedRuntimeBackendPath
if ($hasVerifierPlumbing) {
    $recommendation = New-Recommendation `
        -Name 'Retained verifier-plumbing validation' `
        -Profile 'Boundary' `
        -Reason 'Harness registry, verifier wrapper, Makefile, or mode-163 plumbing changed.' `
        -Commands @(
            '.\scripts\check-harness-registry.ps1',
            '.\scripts\verify-boundary.ps1'
        )
} elseif ($hasAbiHeader) {
    $recommendation = New-Recommendation `
        -Name 'Current two-ROM validation' `
        -Profile 'Latest' `
        -Reason 'Header or shared ABI changes can affect both published ROMs.' `
        -Commands @(
            '.\scripts\verify-current.ps1 -Build'
        )
} elseif ($hasGbiRenderer) {
    $recommendation = New-Recommendation `
        -Name 'GBI/renderer focused verification' `
        -Profile 'Latest' `
        -Reason 'GBI decode or renderer/display changes need fixtures plus the canonical battle boundary.' `
        -Commands @(
            '.\scripts\check-gbi-decode-fixtures.ps1',
            '.\scripts\verify-boundary.ps1'
        )
} elseif ($hasSharedRuntimeBackend) {
    $recommendation = New-Recommendation `
        -Name 'Shared runtime/backend check' `
        -Profile 'Latest' `
        -Reason 'Shared runtime, backend, import, task/object/controller/reloc/display, or collision paths can affect both retained ROMs.' `
        -Commands @(
            '.\scripts\verify-current.ps1 -Build'
        )
} elseif ($hasCurrentBoundaryHarness) {
    $recommendation = New-Recommendation `
        -Name 'Current boundary harness check' `
        -Profile 'Boundary' `
        -Reason 'Only canonical battle harness orchestration changed.' `
        -Commands @(
            '.\scripts\verify-boundary.ps1'
        )
} elseif ($allSnapshotHygiene) {
    $recommendation = New-Recommendation `
        -Name 'Snapshot/hygiene-only validation' `
        -Profile 'none' `
        -Reason 'Only generated-output cleanup or snapshot handoff tooling changed; emulator profiles are not required for that surface.' `
        -Commands @(
            '.\scripts\clean-generated.ps1 -DryRun',
            '.\scripts\check-snapshot-build-context.ps1 -Mode Lean -Source .'
        )
} elseif ($allDocs) {
    $recommendation = New-Recommendation `
        -Name 'Docs-only review' `
        -Profile 'none' `
        -Reason 'Only documentation changed; no build or emulator verifier is required unless the text claims new runtime evidence.' `
        -Commands @(
            '# no emulator verifier required'
        )
} else {
    $recommendation = New-Recommendation `
        -Name 'Default current-profile validation' `
        -Profile 'Latest' `
        -Reason 'Changed files do not match a narrower advisory route; use the maintained current profile as a conservative default.' `
        -Commands @(
            'make -j16',
            '.\scripts\verify-current.ps1'
        )
}
Write-Output 'Changed files:'
$paths | ForEach-Object { Write-Output "  $_" }
Write-Output ''
Write-Output "Recommendation: $($recommendation.Recommendation)"
Write-Output "Profile: $($recommendation.Profile)"
Write-Output "Why: $($recommendation.Reason)"
Write-Output 'Commands:'
$recommendation.Commands | ForEach-Object { Write-Output "  $_" }
